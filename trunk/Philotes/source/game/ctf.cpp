//----------------------------------------------------------------------------
// ctf.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "ctf.h"
#include "units.h"
#include "teams.h"
#include "player.h"
#include "console.h"
#include "chat.h"
#include "warp.h"
#include "level.h"
#include "pets.h"
#include "states.h"
#include "metagame.h"
#include "room_layout.h"
#include "objects.h"
#include "quests.h"
#include "uix.h"
#include "language.h"
#include "combat.h"
#include "room_path.h"
#include "monsters.h"
#include "c_sound.h"
#include "uix_components.h"
#include "uix_social.h"
#include "pvp.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

enum
{
	CTF_NUM_TEAMS = 2,
	CTF_MAX_PLAYERS_PER_TEAM = 5
};

enum
{
	NUM_TROOPS_PER_WAVE	= 4,
	MAX_WAVES = 3,
	DELAY_BETWEEN_WAVES_IN_SECONDS = 180,
	RESPAWN_DELAY_SECONDS = 5,
	COUNTDOWN_TO_START_SECONDS = 45,
	GAME_TIME_LIMIT_SECONDS = 600,			// set to zero to disable the time limit
	FLAG_CAPS_FOR_THE_WIN = 5,
};

enum CTF_GAME_STATE
{
	CTF_GAME_STATE_INVALID,
	CTF_GAME_STATE_STATIC_UNITS_UNINITIALIZED,
	CTF_GAME_STATE_WAITING_FOR_PLAYERS,
	CTF_GAME_STATE_COUNTDOWN_TO_START,
	CTF_GAME_STATE_PLAYING,
	CTF_GAME_STATE_ENDING_SCOREBOARD,
};

//----------------------------------------------------------------------------
struct METAGAME_DATA_CTF
{
//	int		nTower;
//	int		nInfoTowerBuffCount;
	int		nFlags[ CTF_NUM_TEAMS ];
	int		nTeamGoals[ CTF_NUM_TEAMS ];
	int		nStartLocations[ CTF_NUM_TEAMS ];
	int		nTeams[ CTF_NUM_TEAMS ];
	int		nTeamPlayerStates[ CTF_NUM_TEAMS ];

	int		nTroops[ CTF_NUM_TEAMS ];
	int		nTroopStartLocations[ CTF_NUM_TEAMS ];
	int		nTroopGoals[ CTF_NUM_TEAMS ];
	int		nTroopLevel;

	int		nTeamScoreForTheWin;			// score that will win a match for a team, or -1
	BOOL	bAllowFlagPickup;

	// server only variables below
	GAME_TICK	tGameClockTicks;			// game clock to display, in ticks. includes countdown
	UNITID	idTroops[ NUM_TROOPS_PER_WAVE ][ MAX_WAVES ][ CTF_NUM_TEAMS ];
	UNITID	idTroopGoals[ CTF_NUM_TEAMS ];
	UNITID	idTroopStartLocations[ CTF_NUM_TEAMS ];

	UNITID	idFlags[ CTF_NUM_TEAMS ];
	UNITID	idTeamGoals[ CTF_NUM_TEAMS ];
	UNITID	idStartLocations[ CTF_NUM_TEAMS ];
	UNITID	idGameEventsUnit;	
	CTF_GAME_STATE	eGameState;
};

static void s_sCTFStartCountdown(LEVEL *pLevel,int nCountdownSeconds);
static BOOL s_sCountdownUpdate( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData );
static void s_sCTFSetState( CTF_GAME_STATE eState, GAME *pGame, LEVEL *pLevel, METAGAME_DATA_CTF *pMetagameData );

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static METAGAME_DATA_CTF * sMetagameDataGet( METAGAME *pMetagame )
{
	ASSERT_RETNULL( pMetagame && pMetagame->pData );
	return (METAGAME_DATA_CTF *)pMetagame->pData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static METAGAME_DATA_CTF * sMetagameDataGetFromPlayer( UNIT *pPlayer )
{
	ASSERT_RETNULL( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETNULL( pGame && GameGetMetagame( pGame ) );
	return sMetagameDataGet( GameGetMetagame( pGame ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFSendEndOfGameClientEvents(
	GAME * game)
{
	ASSERT_RETURN(game);
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( game ) );
	ASSERT_RETURN(pMetagameData && pMetagameData->nTeams[0] && pMetagameData->nTeams[1]);

	STATS * team0_stats = TeamGetStatsList(game, pMetagameData->nTeams[0]);
	ASSERT_RETURN(team0_stats);
	int team0_score = StatsGetStat(team0_stats, STATS_TEAM_SCORE, pMetagameData->nTeams[0]);

	STATS * team1_stats = TeamGetStatsList(game, pMetagameData->nTeams[1]);
	ASSERT_RETURN(team1_stats);
	int team1_score = StatsGetStat(team1_stats, STATS_TEAM_SCORE, pMetagameData->nTeams[1]);	

	const WCHAR * strTeam0Name = TeamGetName(game, pMetagameData->nTeams[0]);
	ASSERTX_RETURN(strTeam0Name && strTeam0Name[0], "expected a non empty string for team name in CTF games"); 
	const WCHAR * strTeam1Name = TeamGetName(game, pMetagameData->nTeams[1]);
	ASSERTX_RETURN(strTeam1Name && strTeam1Name[0], "expected a non empty string for team name in CTF games"); 

	// announce winning team, or tie
	if (team0_score == team1_score)
	{
		const WCHAR *pstrTie = GlobalStringGet( GS_PVP_ANNOUNCE_MATCH_TIED );
		if (pstrTie)
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, pstrTie);

		s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_GOOD_WINS_GAME ), pMetagameData->nTeams[0]);
		s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_GOOD_WINS_GAME ), pMetagameData->nTeams[1]);

	}
	else
	{
		const WCHAR *pstrWinnerFormatText = GlobalStringGet( GS_PVP_ANNOUNCE_WINNING_TEAM );
		if (pstrWinnerFormatText)
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, pstrWinnerFormatText, ((team0_score > team1_score) ? strTeam0Name : strTeam1Name));

		s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_GOOD_WINS_GAME ), (team0_score > team1_score) ? pMetagameData->nTeams[0] : pMetagameData->nTeams[1]);
		s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_BAD_WINS_GAME ), (team0_score > team1_score) ? pMetagameData->nTeams[1] : pMetagameData->nTeams[0]);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sCTFEndMatchOnClient( UNIT *pUnit, void *pCallbackData )
{
	ASSERTX_RETURN( UnitHasClient( pUnit ), "CTF player has no client" );
	GAMECLIENTID idClient = UnitGetClientId( pUnit );	
	GAME *pGame = UnitGetGame(pUnit);
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETURN(pMetagameData);

	STATS * team0_stats = TeamGetStatsList(pGame, pMetagameData->nTeams[0]);
	ASSERT_RETURN(team0_stats);
	int team0_score = StatsGetStat(team0_stats, STATS_TEAM_SCORE, pMetagameData->nTeams[0]);

	STATS * team1_stats = TeamGetStatsList(pGame, pMetagameData->nTeams[1]);
	ASSERT_RETURN(team1_stats);
	int team1_score = StatsGetStat(team1_stats, STATS_TEAM_SCORE, pMetagameData->nTeams[1]);	

	MSG_SCMD_PVP_TEAM_END_MATCH msg_outgoing;
	msg_outgoing.wScoreTeam0 = (WORD)team0_score;
	msg_outgoing.wScoreTeam1 = (WORD)team1_score;
	s_SendMessage(pGame, idClient, SCMD_PVP_TEAM_END_MATCH, &msg_outgoing);
}

// delete all the enemies and book
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreeTroops( GAME *pGame )
{
	// delete all the troops
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETURN(pMetagameData);

	for ( int i = 0; i < NUM_TROOPS_PER_WAVE; i++ )
		for ( int j = 0; j < MAX_WAVES; j++ )
			for ( int k = 0; k < CTF_NUM_TEAMS; k++ )
			{
				if (pMetagameData->idTroops[i][j][k] != INVALID_ID)
				{
					UNIT * troop = UnitGetById( pGame, pMetagameData->idTroops[i][j][k] );
					if ( troop && UnitIsMonsterClass( troop, pMetagameData->nTroops[k] ))
					{
						UnitFree( troop, UFF_SEND_TO_CLIENTS );
					}
					pMetagameData->idTroops[i][j][k] = INVALID_ID;
				}
			}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sEndScoreboardState( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	// respawn everything
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETFALSE(pMetagameData);

	s_sCTFSetState(CTF_GAME_STATE_WAITING_FOR_PLAYERS, pGame, UnitGetLevel(unit), pMetagameData);


#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFSetState( CTF_GAME_STATE eState, GAME *pGame, LEVEL *pLevel, METAGAME_DATA_CTF *pMetagameData )
{
	ASSERT_RETURN(IS_SERVER(pGame));
	ASSERT_RETURN( (pLevel || eState == CTF_GAME_STATE_STATIC_UNITS_UNINITIALIZED) && pMetagameData );
	pMetagameData->eGameState = eState;
	switch(eState)
	{
	case CTF_GAME_STATE_INVALID:
		break;

	case CTF_GAME_STATE_STATIC_UNITS_UNINITIALIZED:
		break;

	case CTF_GAME_STATE_WAITING_FOR_PLAYERS:
		if (IS_SERVER(pGame))
		{
			if (s_LevelGetPlayerCount(pLevel) > 1) // counts only other characters at this point, before this character is in the level
			{
				// start the countdown
				s_sCTFSetState(CTF_GAME_STATE_COUNTDOWN_TO_START, pGame, pLevel, pMetagameData);
			}
		}
		break;

	case CTF_GAME_STATE_COUNTDOWN_TO_START:
		if (IS_SERVER(pGame))
		{
			s_sCTFStartCountdown(pLevel, COUNTDOWN_TO_START_SECONDS);
		}
		break;

	case CTF_GAME_STATE_PLAYING:
		break;

	case CTF_GAME_STATE_ENDING_SCOREBOARD:
		{
			s_sCTFSendEndOfGameClientEvents(pGame);

			// send "countdown started" client events to everyone in the game
			LevelIteratePlayers(pLevel, s_sCTFEndMatchOnClient, NULL);

			UNIT *pGameEventsUnit = UnitGetById(pGame, pMetagameData->idGameEventsUnit);
			ASSERTX_RETURN(pGameEventsUnit, "failed to get game event unit for CTF, missing goal object for team 0 in level")

			UnitUnregisterTimedEvent(pGameEventsUnit, s_sEndScoreboardState);
			UnitRegisterEventTimed( 
				pGameEventsUnit, 
				s_sEndScoreboardState, 
				&EVENT_DATA( ), 
				30 * GAME_TICKS_PER_SECOND);

		}
		break;
	}

	pMetagameData->bAllowFlagPickup = (pMetagameData->eGameState == CTF_GAME_STATE_PLAYING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CTF_GAME_STATE sCTFGetState( const METAGAME_DATA_CTF *pMetagameData )
{
	ASSERT_RETVAL( pMetagameData, CTF_GAME_STATE_INVALID );
	return pMetagameData->eGameState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSpawnFlagAtUnit( 
	UNIT * pUnit, 
	int nTeamIndexInData )
{
	// drop new book
	ROOM * room = UnitGetRoom( pUnit );

	VECTOR vPosition, vDirection;
	VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );

	GAME *pGame = UnitGetGame( pUnit );
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETNULL(pMetagameData);
	OBJECT_SPEC tSpec;
	tSpec.nClass = pMetagameData->nFlags[nTeamIndexInData];
	tSpec.pRoom = room;
	tSpec.vPosition = pUnit->vPosition;
	tSpec.pvFaceDirection = &pUnit->vFaceDirection;

	// check that it doesn't already exist
	if ( pMetagameData->idFlags[nTeamIndexInData] == INVALID_ID )
	{
		UNIT *pFlag = s_ObjectSpawn( pGame, tSpec );
		ASSERTX_RETNULL( pFlag, "Had trouble spawning the flag" );
		pMetagameData->idFlags[nTeamIndexInData] = UnitGetId(pFlag);
		return pFlag;
	}
	else
	{
		UNIT *pFlag = UnitGetById(pGame, pMetagameData->idFlags[nTeamIndexInData]);
		if (pFlag)
		{
			DWORD dwWarpFlags = 0;
			UnitWarp(pFlag, room, pUnit->vPosition, pUnit->vFaceDirection, VECTOR(0, 0, 1), dwWarpFlags);

			return pFlag;
		}
	}
	return NULL;
}

struct SpawnTroopCallbackData
{
	METAGAME_DATA_CTF * pMetagameData;
	int nTroopIndex;
	int nWaveIndex;
	int nTeamIndex;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSpawnTroopCallback( UNIT *pSpawned, void *pCallbackData )
{
	ASSERT_RETURN(pSpawned && pCallbackData);
	SpawnTroopCallbackData *pData = (SpawnTroopCallbackData*)pCallbackData;
	ASSERT_RETURN(pData->pMetagameData && pData->nTeamIndex >= 0 && pData->nTeamIndex < CTF_NUM_TEAMS && pData->nWaveIndex == 0 && pData->nTroopIndex < NUM_TROOPS_PER_WAVE);
	ASSERT_RETURN(pData->pMetagameData->idTroops[pData->nTroopIndex][pData->nWaveIndex][pData->nTeamIndex] == INVALID_ID);
	pData->pMetagameData->idTroops[pData->nTroopIndex][pData->nWaveIndex][pData->nTeamIndex] = UnitGetId(pSpawned);

	TeamAddUnit(pSpawned, pData->pMetagameData->nTeams[pData->nTeamIndex]);
	// this doesn't work: s_StateSet(pSpawned, pSpawned, pData->pMetagameData->nTeamPlayerStates[pData->nTeamIndex] , 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnTroopsAtUnit( 
	UNIT * pUnit, 
	int nTeamIndexInData )
{
	ASSERT_RETURN(pUnit && nTeamIndexInData >= 0 && nTeamIndexInData < CTF_NUM_TEAMS);
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETURN(pMetagameData);

	SpawnTroopCallbackData tData;
	tData.pMetagameData = pMetagameData;
	tData.nTeamIndex = nTeamIndexInData;
	tData.nWaveIndex = 0;

	// count how many are left in this wave, spawn the max per wave minus this amount
	for ( tData.nTroopIndex = 0; tData.nTroopIndex < NUM_TROOPS_PER_WAVE; ++tData.nTroopIndex )
	{
		UNIT * troop = NULL;
		UNITID idTroop = pMetagameData->idTroops[tData.nTroopIndex][tData.nWaveIndex][tData.nTeamIndex];
		if (idTroop != INVALID_ID)
		{
			troop = UnitGetById( pGame, idTroop );
			if ( !UnitIsMonsterClass( troop, pMetagameData->nTroops[tData.nTeamIndex] ))
			{
				troop = NULL;
			}
		}

		if (!troop)
		{
			ROOM * pDestRoom;
			VECTOR vDirection;
			float fAngle = RandGetFloat( pGame->rand, TWOxPI );
			vDirection.fX = cosf( fAngle );
			vDirection.fY = sinf( fAngle );
			vDirection.fZ = 0.0f;
			VectorNormalize( vDirection );
			VECTOR vDelta = vDirection * RandGetFloat( pGame->rand, 1.5f, 3.0f );
			VECTOR vDestination = pUnit->vPosition + vDelta;	
			ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, UnitGetRoom( pUnit ), vDestination, &pDestRoom );
			ASSERT_CONTINUE( pPathNode );

			// init location
			SPAWN_LOCATION tLocation;
			SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

			MONSTER_SPEC tSpec;
			tSpec.nClass = pMetagameData->nTroops[nTeamIndexInData];
			tSpec.nExperienceLevel = pMetagameData->nTroopLevel;
			tSpec.pRoom = pDestRoom;
			tSpec.vPosition = tLocation.vSpawnPosition;
			tSpec.vDirection = vDirection;
			tSpec.dwFlags = 0;
			UNIT * protector = s_MonsterSpawn( pGame, tSpec );
			ASSERT_CONTINUE( protector );
			sSpawnTroopCallback( protector, &tData );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sMetagameDataInitForLevel(
	METAGAME_DATA_CTF * pMetagameData,
	LEVEL *pLevel)
{
	ASSERT_RETURN(pLevel);
	ASSERTX_RETURN( IS_SERVER( LevelGetGame( pLevel ) ), "Server only" );
	ASSERT_RETURN(pMetagameData);

	for ( int i = 0; i < CTF_NUM_TEAMS; ++i )
	{
		// check that it doesn't already exist
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT * pTeamGoal =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pMetagameData->nTeamGoals[i] );
		ASSERTX_CONTINUE(pTeamGoal, "missing team goal in CTF level");
		if ( pTeamGoal )
		{
			pMetagameData->idTeamGoals[i] = UnitGetId(pTeamGoal);
		}

		UNIT * pStartLocation =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pMetagameData->nStartLocations[i] );
		if ( pStartLocation )
		{
			pMetagameData->idStartLocations[i] = UnitGetId(pStartLocation);
		}

		UNIT * pTroopGoal =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pMetagameData->nTroopGoals[i] );
		if ( pTroopGoal )
		{
			pMetagameData->idTroopGoals[i] = UnitGetId(pTroopGoal);
		}

		UNIT * pTroopStartLocation =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pMetagameData->nTroopStartLocations[i] );
		if ( pTroopStartLocation )
		{
			pMetagameData->idTroopStartLocations[i] = UnitGetId(pTroopStartLocation);
		}
	}

	pMetagameData->idGameEventsUnit = pMetagameData->idTeamGoals[0];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct ROOM * s_CTFGetStartPosition(
	GAME * pGame,
	UNIT * pPlayer,
	LEVELID idLevel,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection, 
	int * pnRotation)
{
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(pPlayer);

	METAGAME_DATA_CTF *pData = sMetagameDataGetFromPlayer(pPlayer);
	ASSERT_RETNULL(pData);
	int nTeamIndexForData = (pData->nTeams[0] == UnitGetTeam(pPlayer)) ? 0 : 1;
	if (pData->idStartLocations[nTeamIndexForData] != INVALID_ID)
	{
		UNIT *pStartLocation = UnitGetById(pGame, pData->idStartLocations[nTeamIndexForData]);
		if (pStartLocation)
		{
			vPosition = UnitGetPosition(pStartLocation);
			vFaceDirection = UnitGetFaceDirection(pStartLocation, TRUE);
			if (pnRotation)
				*pnRotation = 0;
			return UnitGetRoom(pStartLocation);
		}
	}

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CTFWarpToStart(
	GAME * game,
	UNIT * player)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(player);

	ROOM * cur_room = UnitGetRoom(player);
	ASSERT_RETURN(cur_room);
	LEVEL * level = RoomGetLevel(cur_room);
	ASSERT_RETURN(level);

	VECTOR vPosition;
	VECTOR vFaceDirection;
	int nRotation;

	ROOM * room = s_CTFGetStartPosition(game, player, LevelGetID(level), vPosition, vFaceDirection, &nRotation);
	ASSERT_RETURN(room);

	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp(player, room, vPosition, vFaceDirection, VECTOR(0, 0, 1), dwWarpFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sCTFRespawnPlayer( UNIT *pUnit, void *pCallbackData )
{
	if (pCallbackData)
	{
		BOOL bResetIndividualScore = *(BOOL*)pCallbackData;
		if (bResetIndividualScore)
		{
			// reset individual score stats
			UnitClearStat(pUnit, STATS_PLAYER_KILLS, 0);
			UnitClearStat(pUnit, STATS_FLAG_CAPTURES, 0);
		}
	}
	StateClearAllOfType(pUnit, STATE_PVP_CTF_PLAYER_HAS_FLAG);
	s_PlayerRestart(pUnit, PLAYER_RESPAWN_METAGAME_CONTROLLED );
	CTFWarpToStart(UnitGetGame(pUnit), pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFResetFlags(
	METAGAME_DATA_CTF * pMetagameData,
	LEVEL *pLevel)
{
	ASSERTX_RETURN( IS_SERVER( LevelGetGame( pLevel ) ), "Server only" );
	ASSERT_RETURN(pMetagameData);
	GAME *pGame = LevelGetGame(pLevel);
	for ( int i = 0; i < CTF_NUM_TEAMS; ++i )
	{
		UNIT * pTeamGoal =  UnitGetById( pGame, pMetagameData->idTeamGoals[i] );
		ASSERTX_CONTINUE(pTeamGoal, "missing team goal in CTF level");
		if ( pTeamGoal )
		{
			UNIT * pFlag =  sSpawnFlagAtUnit(pTeamGoal, i);
			if ( pFlag )
			{
				pMetagameData->idFlags[i] = UnitGetId(pFlag);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDropEnemyFlagAtPlayerPosition( UNIT *pPlayer, UNIT * pUnit )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pUnit ) ), "Server only" );

	// clear book state
	GAME *pGame = UnitGetGame( pUnit );
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETURN(pMetagameData);
	StateClearAllOfType(pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG);
	int nEnemyTeamIndexInData = (UnitGetTeam( pPlayer ) == pMetagameData->nTeams[0]) ? 1 : 0;
	UNIT *pFlag = sSpawnFlagAtUnit(pUnit, nEnemyTeamIndexInData);
	ASSERT_RETURN(pFlag);
	s_StateSet( 
		pFlag, 
		pPlayer, 
		UnitGetTeam(pPlayer) == pMetagameData->nTeams[0] ? 
		STATE_PVP_CTF_FLAG_DROPPED_TEAM1 :
		STATE_PVP_CTF_FLAG_DROPPED_TEAM0,
		0 );
}

#if (0)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnEnemies( UNIT *player )
{
	LEVEL * level = UnitGetLevel( player );
	for ( ROOM * room = LevelGetFirstRoom( level ); room; room = LevelGetNextRoom( room ) )
	{
		// find the node
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "MonsterSpawn", &pOrientation );
		if ( nodeset && pOrientation )
		{
			VECTOR vPosition, vDirection;
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );

			METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
			ASSERT_RETURN(pMetagameData);

			for ( int i = 0; i < NUM_TROOPS_PER_WAVE; i++ )
			{
				GAME * pGame = UnitGetGame( player );
				ROOM * pDestRoom;
				ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, room, vPosition, &pDestRoom );

				if ( !pNode )
					return;

				// init location
				SPAWN_LOCATION tLocation;
				SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

				MONSTER_SPEC tSpec;
				tSpec.nClass = pMetagameData->nImpUber;
				tSpec.nExperienceLevel = QuestGetMonsterLevel( pMetagame, RoomGetLevel( pDestRoom ), tSpec.nClass, TOK_ENEMY_LEVEL );
				tSpec.pRoom = pDestRoom;
				tSpec.vPosition = tLocation.vSpawnPosition;
				tSpec.vDirection = vDirection;
				tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
				UNIT * imp = s_MonsterSpawn( pGame, tSpec );
				ASSERT( imp );
				if ( imp )
				{
					pMetagameData->idTroops[i] = UnitGetId( imp );
					UnitSetStat( imp, STATS_AI_ACTIVE_QUEST_ID, pMetagame->nQuest );
				}
			}
			return;
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sGrabFlag( UNIT *pPlayer, UNITID idUnit )
{
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETURN(pMetagameData);

	// make sure it's the book
	int nFlagIndex = -1;
	for (int i=0; i < CTF_NUM_TEAMS; ++i)
		if (pMetagameData->idFlags[i] == idUnit)
			nFlagIndex = i;

	if ( nFlagIndex < 0 )
		return;

	pMetagameData->idFlags[nFlagIndex] = INVALID_ID;
	GAME *pGame = UnitGetGame( pPlayer );
	UNIT * flag = UnitGetById( pGame, idUnit );
	if ( IsUnitDeadOrDying( flag ) )
		return;

	if ( flag )
	{
		UnitFree( flag, UFF_SEND_TO_CLIENTS );
		s_StateSet( 
			pPlayer, 
			pPlayer, 
			UnitGetTeam(pPlayer) == pMetagameData->nTeams[0] ? 
				STATE_PVP_CTF_PLAYER_HAS_TEAM1_FLAG :
				STATE_PVP_CTF_PLAYER_HAS_TEAM0_FLAG, 
			0 );

		// sparkles, etc.
		s_StateSet( pPlayer, pPlayer, STATE_QUEST_ACCEPT, 0 );

		// TODO send game data update to clients

		
		s_TeamPlaySound(pGame, GlobalIndexGet( GI_SOUND_CTF_BAD_PICKUP_FLAG ), pMetagameData->nTeams[nFlagIndex]);
		s_TeamPlaySound(pGame, GlobalIndexGet( GI_SOUND_CTF_GOOD_PICKUP_FLAG ), pMetagameData->nTeams[!nFlagIndex]);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_NEARBY_UNITS			16
static void sAIUpdate( UNIT *pPlayer, UNIT * monster )
{
#if (0)
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETURN(pMetagameData);
	GAME * pGame = UnitGetGame( monster );
	ASSERTX_RETURN( UnitGetClass( monster ) == pMetagameData->nImpUber, "Test of Knowledge Strategy is Imp_uber only" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( IsUnitDeadOrDying( monster ) )
		return;

	// clear previous flags
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_ATTACK, 0 );

	// find objects and players around me
	QUEST_GAME_NEARBY_INFO	tNearbyInfo;
	s_QuestGameNearbyInfo( monster, &tNearbyInfo );

	// any players nearby with book?
	for ( int i = 0; i < tNearbyInfo.nNumNearbyPlayers; i++ )
	{
		if ( UnitHasState( pGame, tNearbyInfo.pNearbyPlayers[i], STATE_QUEST_TK_BOOK ) )
		{
			// attack player with book!
			UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[i] ) );
			s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
			return;
		}
	}

	// attack nearby players
	if ( tNearbyInfo.nNumNearbyPlayers > 0 )
	{
		// attack first player
		UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[0] ) );
		s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
		return;
	}

	// book nearby? (defend)
	for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
	{
		if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pMetagameData->nFlag )
		{
			// path to the book!
			UnitSetStat( monster, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( tNearbyInfo.pNearbyObjects[i] ) );
			s_StateSet( monster, monster, STATE_UBER_IMP_RUN_TO, 0 );
			return;
		}
	}
#endif
}

#if (0)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sInfoTowerUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	METAGAME * pMetagame = ( METAGAME * )tEventData.m_Data1;
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( unit );

	pMetagameData->nInfoTowerBuffCount--;
	if ( pMetagameData->nInfoTowerBuffCount <= 0 )
	{
		s_QuestSendRadarMessage( pMetagame, QUEST_UI_RADAR_TURN_OFF );
		return TRUE;
	}

	for ( int i = 0; i < NUM_TROOPS_PER_WAVE; i++ )
	{
		if ( pMetagameData->idTroops[i] != INVALID_ID )
		{
			UNIT * imp = UnitGetById( game, pMetagameData->idTroops[i] );
			if ( imp ) s_QuestSendRadarMessage( pMetagame, QUEST_UI_RADAR_UPDATE_UNIT, imp, 0, GFXCOLOR_RED );
		}
	}

	UnitRegisterEventTimed( unit, sInfoTowerUpdate, &tEventData, GAME_TICKS_PER_SECOND / 4 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInfoTowerActivate( UNIT *pPlayer, UNIT * pTower )
{
	ASSERT_RETURN ( pMetagame );

	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETURN(pMetagameData);

	UNIT * pPlayer = pMetagame->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	s_QuestSendRadarMessage( pMetagame, QUEST_UI_RADAR_RESET );
	for ( int i = 0; i < NUM_TROOPS_PER_WAVE; i++ )
	{
		if ( pMetagameData->idTroops[i] != INVALID_ID )
		{
			UNIT * imp = UnitGetById( pGame, pMetagameData->idTroops[i] );
			if ( imp ) s_QuestSendRadarMessage( pMetagame, QUEST_UI_RADAR_ADD_UNIT, imp, 0, GFXCOLOR_RED );
		}
	}
	s_QuestSendRadarMessage( pMetagame, QUEST_UI_RADAR_TURN_ON );

	pMetagameData->nInfoTowerBuffCount = 50;

	// setup event
	UnitRegisterEventTimed( pPlayer, sInfoTowerUpdate, &EVENT_DATA( (DWORD_PTR)pMetagame ), GAME_TICKS_PER_SECOND / 4 );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sMetagameMessageCanOperateObject( 
	UNIT *pPlayer,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
 	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETVAL(pMetagameData, QMR_OK);
 	UNITID idTarget = pMessage->idObject;
	GAME *pGame = UnitGetGame(pPlayer);
	UNIT *pTarget = UnitGetById( pGame, idTarget );
	if (!pTarget) return QMR_OPERATE_FORBIDDEN;
	int nTargetClass = UnitGetClass( pTarget );

	for ( int i = 0; i < CTF_NUM_TEAMS; ++i )
	{
		if ( pMetagameData->nFlags[i] == nTargetClass )
		{
			if (!pMetagameData->bAllowFlagPickup)
				return QMR_OPERATE_FORBIDDEN;

			if ( UnitGetTeam( pPlayer ) == pMetagameData->nTeams[i] )
			{
				// my team's flag, only allow pickup if it is dropped (not when it is at your base)
				return UnitHasState(pGame, pTarget, STATE_PVP_CTF_FLAG_DROPPED)? QMR_OPERATE_OK : QMR_OPERATE_FORBIDDEN;
			}
			else
			{
				// other team's flag
				return QMR_OPERATE_OK;
			}
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSendGameData(GAME *pGame, METAGAME_DATA_CTF *pData, UNIT *pUnit)
{
	for ( int i = 0; i < CTF_NUM_TEAMS; i++ )
	{
		int nTeam = pData->nTeams[i];
		STATS *team_stats = TeamGetStatsList(pGame, nTeam);
		ASSERT_CONTINUE(team_stats);
		UnitSetStat(pUnit, STATS_TEAM_SCORE_TO_DISPLAY, nTeam, StatsGetStat(team_stats, STATS_TEAM_SCORE, nTeam));
	}

	// HACK flag countdown clock for special treatment by making it negative
	int nClockTicks = (int)pData->tGameClockTicks;
	UnitSetStat(pUnit, STATS_GAME_CLOCK_TICKS, (pData->eGameState == CTF_GAME_STATE_COUNTDOWN_TO_START) ? -nClockTicks: nClockTicks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_sSendGameDataCB(
	UNIT *pUnit, 
	void *pCallbackData )
{
	s_sSendGameData(UnitGetGame(pUnit), (METAGAME_DATA_CTF *)pCallbackData, pUnit);
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSendGameDataToAll(GAME *pGame, METAGAME_DATA_CTF *pData, LEVEL *pLevel)
{
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LevelIterateUnits(pLevel, eTargetTypes, s_sSendGameDataCB, pData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFEventFlagCapture(
	GAME * game,
	UNIT * capturer)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(capturer);

	UnitChangeStat(capturer, STATS_FLAG_CAPTURES, 0, 1);

	WCHAR capturer_name[MAX_CHARACTER_NAME];
	capturer_name[0] = 0;
	UnitGetName(capturer, capturer_name, arrsize(capturer_name), 0);

	METAGAME_DATA_CTF *pData = sMetagameDataGetFromPlayer( capturer );
	ASSERT_RETURN(pData);
	int capturer_team = UnitGetTeam(capturer);
	ASSERT_RETURN(capturer_team != INVALID_TEAM);

	const WCHAR *pstrTeamName = GlobalStringGet( ((capturer_team == pData->nTeams[0]) ? GS_PVP_TEAM_NAME_0 : GS_PVP_TEAM_NAME_1) );
	const WCHAR *pstrFlagCaptured = GlobalStringGet( GS_PVP_ANNOUNCE_FLAG_CAPTURE );
	if (pstrFlagCaptured && pstrTeamName)
	{
		s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, pstrFlagCaptured, capturer_name, pstrTeamName);
	}

	STATS * capturer_team_stats = TeamGetStatsList(game, capturer_team);
	ASSERT_RETURN(capturer_team_stats);
	StatsChangeStat(game, capturer_team_stats, STATS_TEAM_SCORE, capturer_team, 1);
	int capturer_team_score = StatsGetStat(capturer_team_stats, STATS_TEAM_SCORE, capturer_team);
	s_sSendGameDataToAll(game, pData, UnitGetLevel(capturer));

	s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_GOOD_CAPTURES_FLAG ), capturer_team);
	s_TeamPlaySound(game, GlobalIndexGet( GI_SOUND_CTF_BAD_CAPTURES_FLAG ), (capturer_team == pData->nTeams[0]) ? pData->nTeams[1] : pData->nTeams[0]);

	LEVEL *pLevel = UnitGetLevel(capturer);
	if (pData->nTeamScoreForTheWin > 0 && capturer_team_score >= pData->nTeamScoreForTheWin)
	{
		s_sCTFSetState(CTF_GAME_STATE_ENDING_SCOREBOARD, game, pLevel, pData);
	}
	else
	{
		s_sCTFResetFlags(pData, pLevel);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sMetagameMessageObjectOperated( 
	UNIT *pPlayer,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETVAL(pMetagameData, QMR_OK);
	UNIT *pTarget = pMessage->pTarget;
	UNITID idTarget = UnitGetId(pTarget);
	GAME * pGame = UnitGetGame( pPlayer );

#if (0)
	int nTargetClass = UnitGetClass( pTarget );
	if ( nTargetClass == pMetagameData->nTower )
	{
		sInfoTowerActivate( pMetagame, pTarget );
		return QMR_FINISHED;
	}
#endif

	for ( int i = 0; i < CTF_NUM_TEAMS; i++ )
		if ( idTarget == pMetagameData->idFlags[i] )
		{
			if (!pMetagameData->bAllowFlagPickup)
				return QMR_FINISHED;

			StateClearAllOfType(pTarget, STATE_PVP_CTF_FLAG_DROPPED);
			if (UnitGetTeam( pPlayer ) == pMetagameData->nTeams[i])
			{
				// return my team's flag to our goal
				UNIT * pTeamGoal =  UnitGetById( UnitGetGame(pPlayer), pMetagameData->idTeamGoals[i] );
				ASSERTX_CONTINUE(pTeamGoal, "missing team goal in CTF level");
				UNIT * pFlag =  sSpawnFlagAtUnit(pTeamGoal, i);
				if (pFlag)
				{
					pMetagameData->idFlags[i] = UnitGetId(pFlag);
					s_TeamPlaySound(pGame, GlobalIndexGet( GI_SOUND_CTF_GOOD_FLAG_RETURNED ), pMetagameData->nTeams[i]);
					s_TeamPlaySound(pGame, GlobalIndexGet( GI_SOUND_CTF_BAD_FLAG_RETURNED ), pMetagameData->nTeams[!i]);
				}
			}
			else
			{
				// grab the enemy flag
				//s_QuestOperateDelay( pMetagame, s_sGrabFlag, UnitGetId( pTarget ), 3 * GAME_TICKS_PER_SECOND );
				s_sGrabFlag( pPlayer, idTarget );
			}
			return QMR_FINISHED;
		}

		// reached my team's goal, cap enemy flag if carrying and my flag is safe at the goal
	for ( int i = 0; i < CTF_NUM_TEAMS; i++ )
		if ( UnitGetTeam( pPlayer ) == pMetagameData->nTeams[i] &&
			 idTarget == pMetagameData->idTeamGoals[i] )
		{
			if ( !UnitIsPlayer( pPlayer ) )
				continue;

			if (pMetagameData->eGameState != CTF_GAME_STATE_PLAYING)
				return QMR_FINISHED;

			if ( UnitHasState( pGame, pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG ) &&
				 pMetagameData->idFlags[i] != INVALID_ID)
			{

				UNIT *pFlag = UnitGetById(pGame, pMetagameData->idFlags[i]);
				if (pFlag && !UnitHasState(pGame, pFlag, STATE_PVP_CTF_FLAG_DROPPED))
				{
					// point!
					StateClearAllOfType(pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG);
					s_sCTFEventFlagCapture( pGame, pPlayer );
				}
			}
			return QMR_FINISHED;
		}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_CTFStartGameClock(int nGameTimelimitSeconds)
{
#if !ISVERSION(SERVER_VERSION)
	GAME *game = AppGetCltGame();
	if (game)
	{
		UNIT * unit = GameGetControlUnit(game);
		if (unit)
		{
			METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( game ) );
			ASSERT_RETFALSE(pMetagameData);

			pMetagameData->bAllowFlagPickup = TRUE;
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sCountdownUpdateDisplay( )
{
#if !ISVERSION(SERVER_VERSION)

	// show on screen
	GAME *pGame = AppGetCltGame();
	UNIT * pUnit = GameGetControlUnit(pGame);
	ASSERT_RETURN(pUnit);
	int nCountdown = UnitGetStat(pUnit, STATS_GAME_CLOCK_TICKS)/GAME_TICKS_PER_SECOND;
	nCountdown = ABS(nCountdown);

	if ( nCountdown == 1 )
	{
		UIShowQuickMessage( GlobalStringGet( GS_PVP_MATCH_START_COUNTDOWN_1_SECOND ) );
	}
	else
	{
		const int MAX_STRING = 1024;
		WCHAR timestr[ MAX_STRING ] = L"\0";
		PStrPrintf( timestr, MAX_STRING, GlobalStringGet( GS_PVP_MATCH_START_COUNTDOWN ), nCountdown );
		UIShowQuickMessage( timestr );
	}

#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sCountdownUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_CLIENT( game ) );

	GAME *pGame = AppGetCltGame();
	UNIT * pUnit = GameGetControlUnit(pGame);
	ASSERT_RETFALSE(pUnit);
	int nCountdown = UnitGetStat(pUnit, STATS_GAME_CLOCK_TICKS)/GAME_TICKS_PER_SECOND;
	nCountdown = ABS(nCountdown);

	if ( nCountdown > 0 )
	{
		c_sCountdownUpdateDisplay();
		UnitRegisterEventTimed( unit, c_sCountdownUpdate, &tEventData, GAME_TICKS_PER_SECOND );
	}
	else
	{
		UnitUnregisterTimedEvent(unit, c_sCountdownUpdate);
		UIHideQuickMessage();
		METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( game ) );
		ASSERT_RETFALSE(pMetagameData);
		pMetagameData->bAllowFlagPickup = TRUE;
		c_CTFStartGameClock(GAME_TIME_LIMIT_SECONDS);
	}

#endif// !ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sCountdownStop(
	GAME* game,
	UNIT* unit,
	UNIT* unitNotUsed,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(game);
	ASSERT_RETURN(IS_CLIENT( game ));

	UnitUnregisterTimedEvent(unit, c_sCountdownUpdate);
	UIHideQuickMessage();

#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_CTFStartCountdown(int nCountdownSeconds)
{
#if !ISVERSION(SERVER_VERSION)
	GAME *game = AppGetCltGame();
	if (game)
	{
		UNIT * unit = GameGetControlUnit(game);
		if (unit)
		{
			// stop the clock(!)
			// set display timer for the countdown
			c_sCountdownUpdateDisplay();
			UnitUnregisterTimedEvent(unit, c_sCountdownUpdate);
			UnitRegisterEventTimed( 
				unit, 
				c_sCountdownUpdate, 
				&EVENT_DATA( ), 
				GAME_TICKS_PER_SECOND);

			UnitUnregisterEventHandler( game, unit, EVENT_WARPED_LEVELS, c_sCountdownStop );
			UnitRegisterEventHandler( game, unit, EVENT_WARPED_LEVELS, c_sCountdownStop, &EVENT_DATA() );

			METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( game ) );
			ASSERT_RETFALSE(pMetagameData);
			pMetagameData->bAllowFlagPickup = TRUE;

			// hide scoreboard
			UICloseScoreboard();
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CTFEndMatch(int nScoreTeam0, int nScoreTeam1)
{
#if !ISVERSION(SERVER_VERSION)
	GAME *game = AppGetCltGame();
	if (game)
	{
		UNIT * unit = GameGetControlUnit(game);
		if (unit)
		{
			METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( game ) );
			ASSERT_RETURN(pMetagameData);

			// set display timer for the countdown
			UIHideQuickMessage();
			UnitUnregisterTimedEvent(unit, c_sCountdownUpdate);

			UnitUnregisterEventHandler( game, unit, EVENT_WARPED_LEVELS, c_sCountdownStop );

			pMetagameData->bAllowFlagPickup = FALSE;

			// display scoreboard
			UIOpenScoreboard();
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sGameReachedTimelimit( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	// respawn everything
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	s_sCTFSetState(CTF_GAME_STATE_ENDING_SCOREBOARD, pGame, UnitGetLevel(unit), pMetagameData);

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sRepawnTroops( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETFALSE(pMetagameData);

	for ( int i = 0; i < CTF_NUM_TEAMS; ++i )
	{
		// check that it doesn't already exist
		ASSERT_RETFALSE(pMetagameData->idTroopStartLocations[i] != INVALID_ID);
		UNIT * pTroopStartLocation =  UnitGetById(pGame, pMetagameData->idTroopStartLocations[i]);
		if ( pTroopStartLocation )
		{
			// spawn the first wave at their start location, with guard AI
			sSpawnTroopsAtUnit(pTroopStartLocation, i);
		}
	}

	if (pMetagameData->tGameClockTicks < GAME_TICKS_PER_SECOND * (GAME_TIME_LIMIT_SECONDS - DELAY_BETWEEN_WAVES_IN_SECONDS))
	{
		UnitRegisterEventTimed( 
			unit, 
			s_sRepawnTroops, 
			&EVENT_DATA( ), 
			GAME_TICKS_PER_SECOND * DELAY_BETWEEN_WAVES_IN_SECONDS);
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFResetGame(
	METAGAME_DATA_CTF * pMetagameData,
	LEVEL *pLevel)
{
	GAME *pGame = LevelGetGame(pLevel);
	ASSERTX_RETURN(IS_SERVER(pGame), "Server only");
	ASSERT_RETURN(pMetagameData);

	s_sCTFResetFlags(pMetagameData, pLevel);

	// respawn all the players at their team spawns
	BOOL bResetIndividualScore = TRUE;
	LevelIteratePlayers(pLevel, s_sCTFRespawnPlayer,&bResetIndividualScore);

	for	(int i=0; i<CTF_NUM_TEAMS; ++i)
	{
		ASSERT_CONTINUE(pMetagameData->nTeams[i] != INVALID_TEAM);
		STATS * team_stats = TeamGetStatsList(pGame, pMetagameData->nTeams[i]);
		ASSERT_CONTINUE(team_stats);
		StatsSetStat(pGame, team_stats, STATS_TEAM_SCORE, pMetagameData->nTeams[i], 0);
	}

	sFreeTroops(pGame);

	for ( int i = 0; i < CTF_NUM_TEAMS; ++i )
	{
		// check that it doesn't already exist
		ASSERT_RETURN(pMetagameData->idTroopStartLocations[i] != INVALID_ID);
		UNIT * pTroopStartLocation =  UnitGetById(pGame, pMetagameData->idTroopStartLocations[i]);
		if ( pTroopStartLocation )
		{
			// spawn the first wave at their start location, with an idle AI
			sSpawnTroopsAtUnit(pTroopStartLocation, i);
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sCTFStartCountdownOnClient( UNIT *pUnit, void *pCallbackData )
{
	ASSERT_RETURN(pCallbackData);
	ASSERTX_RETURN( UnitHasClient( pUnit ), "CTF player has no client" );
	GAMECLIENTID idClient = UnitGetClientId( pUnit );	
	MSG_SCMD_PVP_TEAM_START_COUNTDOWN msg_outgoing;
	msg_outgoing.wCountdownSeconds = (WORD)(*((int*)pCallbackData));
	s_SendMessage(UnitGetGame(pUnit), idClient, SCMD_PVP_TEAM_START_COUNTDOWN, &msg_outgoing);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sGameClockUpdate( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	// respawn everything
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETFALSE(pMetagameData);

	if (pMetagameData->tGameClockTicks >= GAME_TIME_LIMIT_SECONDS * GAME_TICKS_PER_SECOND)
	{
		s_sGameReachedTimelimit(pGame, unit, tEventData);
	}
	else
	{
		UnitRegisterEventTimed( 
			unit, 
			s_sGameClockUpdate, 
			&EVENT_DATA( GAME_TICKS_PER_SECOND ), 
			GAME_TICKS_PER_SECOND);
	}

	// update the clock on all the clients
	pMetagameData->tGameClockTicks += (DWORD)tEventData.m_Data1;
	s_sSendGameDataToAll(pGame, pMetagameData, UnitGetLevel(unit));

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sCountdownFinished( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	// respawn everything
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETFALSE(pMetagameData);
	s_sCTFResetGame(pMetagameData, UnitGetLevel(unit));
	s_sCTFSetState(CTF_GAME_STATE_PLAYING, pGame, UnitGetLevel(unit), pMetagameData);
	pMetagameData->tGameClockTicks = 0;
	UnitUnregisterTimedEvent(unit, s_sCountdownUpdate);

	if (GAME_TIME_LIMIT_SECONDS > 0)
	{
		UnitUnregisterTimedEvent(unit, s_sGameClockUpdate);
		UnitRegisterEventTimed( 
			unit, 
			s_sGameClockUpdate, 
			&EVENT_DATA( GAME_TICKS_PER_SECOND ), 
			GAME_TICKS_PER_SECOND);
	}

	UnitUnregisterTimedEvent(unit, s_sRepawnTroops);
	UnitRegisterEventTimed( 
		unit, 
		s_sRepawnTroops, 
		&EVENT_DATA( ), 
		GAME_TICKS_PER_SECOND * DELAY_BETWEEN_WAVES_IN_SECONDS);

	for (int i = 0; i < CTF_NUM_TEAMS; ++i)
		s_TeamPlaySound(pGame, GlobalIndexGet( GI_SOUND_CTF_START_BUZZER ), pMetagameData->nTeams[i]);

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sCountdownUpdate( GAME * pGame, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame && unit );
	ASSERT( IS_SERVER( pGame ) );

	// respawn everything
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETFALSE(pMetagameData);

	if (pMetagameData->tGameClockTicks <= 0)
	{
		s_sCountdownFinished(pGame, unit, tEventData);
	}
	else
	{
		UnitUnregisterTimedEvent(unit, s_sCountdownUpdate);
		UnitRegisterEventTimed( 
			unit, 
			s_sCountdownUpdate, 
			&EVENT_DATA( GAME_TICKS_PER_SECOND ), 
			GAME_TICKS_PER_SECOND);
	}

	GAME_TICK tDeltaTicks = (GAME_TICK)((int)tEventData.m_Data1); // the compiler doesn't appreciate a direct cast from DWORD_PTR to DWORD
	pMetagameData->tGameClockTicks = (pMetagameData->tGameClockTicks > tDeltaTicks) ? (pMetagameData->tGameClockTicks - tDeltaTicks) : 0;

	// update the clock on all the clients
	s_sSendGameDataToAll(pGame, pMetagameData, UnitGetLevel(unit));

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFStartCountdown(
	LEVEL *pLevel,
	int nCountdownSeconds)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME *pGame = LevelGetGame(pLevel);
	ASSERT_RETURN(pGame);

	// send "countdown started" client events to everyone in the game
	LevelIteratePlayers(pLevel, s_sCTFStartCountdownOnClient,&nCountdownSeconds);

	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	ASSERT_RETURN(pMetagameData);
	UNIT *pGameEventsUnit = UnitGetById(pGame, pMetagameData->idGameEventsUnit);
	ASSERTX_RETURN(pGameEventsUnit, "failed to start CTF game, missing goal object for team 0 in level")
	pMetagameData->tGameClockTicks = nCountdownSeconds * GAME_TICKS_PER_SECOND;
	
	// update the clock on all the clients	
	s_sSendGameDataToAll(pGame, pMetagameData, UnitGetLevel(pGameEventsUnit));

	// add timed event to change to dueling teams
	UnitUnregisterTimedEvent(pGameEventsUnit, s_sGameClockUpdate);
	UnitUnregisterTimedEvent(pGameEventsUnit, s_sCountdownUpdate);
	UnitRegisterEventTimed( 
		pGameEventsUnit, 
		s_sCountdownUpdate, 
		&EVENT_DATA( GAME_TICKS_PER_SECOND ), 
		GAME_TICKS_PER_SECOND);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sMetagameMessageExitLimbo(
	UNIT *pPlayer)
{
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), QMR_OK, "Server only" );
	METAGAME_DATA_CTF * pMetagameData = sMetagameDataGetFromPlayer( pPlayer );
	ASSERT_RETVAL(pMetagameData, QMR_OK);

	LEVEL *pLevel = UnitGetLevel(pPlayer);
	
	if (pMetagameData->eGameState == CTF_GAME_STATE_WAITING_FOR_PLAYERS)
	{
		if (s_LevelGetPlayerCount(pLevel) >= 2)
		{
			// start the countdown
			s_sCTFSetState(CTF_GAME_STATE_COUNTDOWN_TO_START, pGame, pLevel, pMetagameData);
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sCTFOnPlayerDead(
	GAME* game,
	UNIT* unit,
	UNIT* unitNotUsed,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game && unit);
	ASSERT_RETURN(IS_SERVER( game ));

	s_PlayerRestart( unit, PLAYER_RESPAWN_METAGAME_CONTROLLED );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sMetagameMessageRemoveFromTeam(
	UNIT *pPlayer,
	const QUEST_MESSAGE_REMOVE_FROM_TEAM * pMessage)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME * pGame = UnitGetGame(pPlayer);
	ASSERT_RETVAL(pGame && GameGetMetagame(pGame), QMR_OK);
	ASSERTX_RETVAL(IS_SERVER(pGame), QMR_OK, "Server only");

	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet(GameGetMetagame(pGame));
	ASSERT_RETVAL(pMetagameData, QMR_OK);

	int team = pMessage->nTeam;

	BOOL bPlayerWasOnCTFTeam = FALSE;
	for ( int k = 0; k < CTF_NUM_TEAMS; k++ )
	{
		if (team == pMetagameData->nTeams[k])
		{
			bPlayerWasOnCTFTeam = TRUE;
			break;
		}
	}

	if (bPlayerWasOnCTFTeam)
	{
		if (!UnitTestFlag(pPlayer, UNITFLAG_JUSTFREED))
		{
			UnitUnregisterEventHandler(pGame, pPlayer, EVENT_DEAD, s_sCTFOnPlayerDead);
			if (IsUnitDeadOrDying(pPlayer))
			{
				s_PlayerRestart( pPlayer, PLAYER_RESPAWN_METAGAME_CONTROLLED );
			}
		}

		// drop flag if I leave
		if ( UnitHasState( pGame, pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG ) )
		{
			sDropEnemyFlagAtPlayerPosition( pPlayer, pPlayer );
		}

		LEVEL *pLevel = UnitGetLevel(pPlayer);
		if ((pMetagameData->eGameState == CTF_GAME_STATE_COUNTDOWN_TO_START ||
			pMetagameData->eGameState == CTF_GAME_STATE_PLAYING) && 
			s_LevelGetPlayerCount(pLevel) <= 1)
		{
			// end the game and show the scoreboard if all but one player leaves
			s_sCTFSetState(CTF_GAME_STATE_ENDING_SCOREBOARD, pGame, pLevel, pMetagameData);
		}

		WCHAR unit_name[MAX_CHARACTER_NAME];
		unit_name[0] = 0;
		UnitGetName(pPlayer, unit_name, arrsize(unit_name), 0);
		const WCHAR * strTeamName = TeamGetName(pGame, team);
		ASSERTX_RETVAL( strTeamName && strTeamName[0], QMR_OK, "expected a non empty string for team name in custom games" ); 
		const WCHAR *pstrPlayerLeftFormatText = GlobalStringGet( GS_PVP_TEAM_PLAYER_LEFT );
		ASSERT_RETVAL(pstrPlayerLeftFormatText, QMR_OK);
		s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, pGame, NULL, CONSOLE_SYSTEM_COLOR, pstrPlayerLeftFormatText, unit_name, strTeamName);
	}
#endif

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sMetagameMessageAboutToLeaveLevel( 
	UNIT *pPlayer)
{
	ASSERT_RETVAL( pPlayer, QMR_OK );

	GAME * pGame = UnitGetGame(pPlayer);
	if (pGame && UnitHasState(UnitGetGame(pPlayer), pPlayer, STATE_PVP_MATCH))
	{
		StateClearAllOfType(pPlayer, STATE_PVP_ENABLED);

		if (s_PlayerIsAutoPartyEnabled(pPlayer))
		{
			s_PlayerEnableAutoParty(pPlayer, FALSE);
			s_StateSet(pPlayer, pPlayer, STATE_REMOVE_FROM_PARTY_ON_EXIT_LIMBO, 0);
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sMetagameMessagePlayerDie( 
	UNIT *pPlayer,
	QUEST_MESSAGE_PLAYER_DIE *pData)
{
	ASSERT_RETVAL( pPlayer, QMR_OK );
	ASSERT_RETVAL( pData, QMR_OK );
//	UnitRegisterEventTimed(pPlayer, s_sCTFRespawn, EVENT_DATA(), RESPAWN_DELAY_SECONDS * GAME_TICKS_PER_SECOND);
	GAME *pGame = UnitGetGame(pPlayer);

	// announce the death
	WCHAR strPlayerName[MAX_CHARACTER_NAME];
	strPlayerName[0] = 0;
	UnitGetName(pPlayer, strPlayerName, arrsize(strPlayerName), 0);
	if (!pData->pKiller)
	{
		const WCHAR *pstrPlayerDied = GlobalStringGet( GS_PVP_ANNOUNCE_DIE );
		if (pstrPlayerDied)
		{
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, pGame, NULL, CONSOLE_SYSTEM_COLOR, pstrPlayerDied, strPlayerName);
		}

	}
	else if (pData->pKiller == pPlayer)
	{
		const WCHAR *pstrPlayerSuicide = GlobalStringGet( GS_PVP_ANNOUNCE_SUICIDE );
		if (pstrPlayerSuicide)
		{
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, pGame, NULL, CONSOLE_SYSTEM_COLOR, pstrPlayerSuicide, strPlayerName);
		}
	}
	else
	{
		UnitChangeStat(pData->pKiller, STATS_PLAYER_KILLS, 0, 1);

		WCHAR strKillerName[MAX_CHARACTER_NAME];
		strKillerName[0] = 0;
		UnitGetName(pData->pKiller, strKillerName, arrsize(strKillerName), 0);

		const WCHAR *pstrPlayerKilledByOther = GlobalStringGet( GS_PVP_ANNOUNCE_KILLED );
		if (pstrPlayerKilledByOther)
		{
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, pGame, NULL, CONSOLE_SYSTEM_COLOR, pstrPlayerKilledByOther, strKillerName, strPlayerName);
		}			
	}

	UnitUnregisterEventHandler(pGame, pPlayer, EVENT_DEAD, s_sCTFOnPlayerDead);
	UnitRegisterEventHandler(pGame, pPlayer, EVENT_DEAD, s_sCTFOnPlayerDead, &EVENT_DATA());

	if (UnitHasState(pGame, pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG))
	{
		// spawn again
		sDropEnemyFlagAtPlayerPosition( pPlayer, pPlayer );
		return QMR_FINISHED;
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sMetagameMessageMonsterKill( 
	UNIT *pPlayer,
	const QUEST_MESSAGE_MONSTER_KILL * pMonsterKillMessage )
{
	// check if game is in playing state and then do: s_CTFEventKill( UnitGetGame( pPlayer), );
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sCTFMessageHandler(
	UNIT *pPlayer,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	GAME *pGame = UnitGetGame(pPlayer);
	if (GameGetSubType(pGame) != GAME_SUBTYPE_PVP_CTF)
	{
		return QMR_OK;
	}

 	switch (eMessageType)
	{
		//----------------------------------------------------------------------------
	case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sMetagameMessageCanOperateObject( pPlayer, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sMetagameMessageObjectOperated( pPlayer, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_REMOVED_FROM_TEAM:
		{	
			const QUEST_MESSAGE_REMOVE_FROM_TEAM * pRemoveFromTeamMessage = (const QUEST_MESSAGE_REMOVE_FROM_TEAM *)pMessage;
			return s_sMetagameMessageRemoveFromTeam( pPlayer, pRemoveFromTeamMessage );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_ABOUT_TO_LEAVE_LEVEL:
		{
			return s_sMetagameMessageAboutToLeaveLevel( pPlayer );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_EXIT_LIMBO:
		{
			return s_sMetagameMessageExitLimbo( pPlayer );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_PLAYER_DIE:
		{
			QUEST_MESSAGE_PLAYER_DIE *pData = (QUEST_MESSAGE_PLAYER_DIE *)pMessage;
			return s_sMetagameMessagePlayerDie( pPlayer, pData );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL * pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sMetagameMessageMonsterKill( pPlayer, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_RESPAWN_WARP_REQUEST:
		{
			CTFWarpToStart(pGame, pPlayer);
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_LEVEL_GET_START_LOCATION:
		{
			QUEST_MESSAGE_LEVEL_GET_START_LOCATION *pLevelGetStartLocationMessage = (QUEST_MESSAGE_LEVEL_GET_START_LOCATION *)pMessage;

			s_CTFGetStartPosition(
				pGame, 
				pPlayer, 
				pLevelGetStartLocationMessage->idLevel, 
				pLevelGetStartLocationMessage->vPosition, 
				pLevelGetStartLocationMessage->vFaceDirection, 
				NULL);
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_FELL_OUT_OF_WORLD:
		{
			METAGAME_DATA_CTF * pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
			if ( UnitHasState( pGame, pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG ) )
			{
				ASSERT_RETVAL(pMetagameData, QMR_OK);
				GAME *pGame = UnitGetGame( pPlayer );
				StateClearAllOfType(pPlayer, STATE_PVP_CTF_PLAYER_HAS_FLAG);
				int nEnemyTeamIndexInData = (UnitGetTeam( pPlayer ) == pMetagameData->nTeams[0]) ? 1 : 0;
				ASSERT(pMetagameData->idTeamGoals[nEnemyTeamIndexInData] != INVALID_ID);
				if (pMetagameData->idTeamGoals[nEnemyTeamIndexInData] != INVALID_ID)
				{
					UNIT *pGoal = UnitGetById(pGame, pMetagameData->idTeamGoals[nEnemyTeamIndexInData]);
					UNIT *pFlag = sSpawnFlagAtUnit(pGoal, nEnemyTeamIndexInData);
					ASSERT(pFlag);
				}
			}

			UnitDie(pPlayer, NULL);
			return QMR_OK;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMetagameDataInit(
	GAME *pGame,
	METAGAME *pMetagame,
	METAGAME_DATA_CTF * pMetagameData)
{
	ASSERT_RETURN(pMetagameData);
//	pMetagameData->nTower					= GlobalIndexGet( GI_OBJECT_TESTKNOWLEDGE_INFO_TOWER );
	pMetagameData->nFlags[0]				= GlobalIndexGet( GI_OBJECT_PVP_CTF_FLAG_TEAM0 );
	pMetagameData->nFlags[1]				= GlobalIndexGet( GI_OBJECT_PVP_CTF_FLAG_TEAM1 );
	pMetagameData->nTeamGoals[0]			= GlobalIndexGet( GI_OBJECT_PVP_CTF_GOAL_TEAM0 );
	pMetagameData->nTeamGoals[1]			= GlobalIndexGet( GI_OBJECT_PVP_CTF_GOAL_TEAM1 );
	pMetagameData->nStartLocations[0]		= GlobalIndexGet( GI_OBJECT_PVP_CTF_SPAWN_TEAM0 );
	pMetagameData->nStartLocations[1]		= GlobalIndexGet( GI_OBJECT_PVP_CTF_SPAWN_TEAM1 );
	pMetagameData->nTeamPlayerStates[0]		= GlobalIndexGet( GI_STATE_PVP_TEAM_RED );
	pMetagameData->nTeamPlayerStates[1]		= GlobalIndexGet( GI_STATE_PVP_TEAM_BLUE );

	pMetagameData->nTroops[0]				= GlobalIndexGet( GI_MONSTER_PVP_CTF_TROOPS_TEAM0 );
	pMetagameData->nTroops[1]				= GlobalIndexGet( GI_MONSTER_PVP_CTF_TROOPS_TEAM1 );
	pMetagameData->nTroopStartLocations[0]	= GlobalIndexGet( GI_OBJECT_PVP_CTF_TROOP_SPAWN_TEAM0 );
	pMetagameData->nTroopStartLocations[1]	= GlobalIndexGet( GI_OBJECT_PVP_CTF_TROOP_SPAWN_TEAM1 );
	pMetagameData->nTroopGoals[0]			= GlobalIndexGet( GI_OBJECT_PVP_CTF_TROOP_GOAL_TEAM0 );
	pMetagameData->nTroopGoals[1]			= GlobalIndexGet( GI_OBJECT_PVP_CTF_TROOP_GOAL_TEAM1 );

	for ( int i = 0; i < NUM_TROOPS_PER_WAVE; i++ )
		for ( int j = 0; j < MAX_WAVES; j++ )
			for ( int k = 0; k < CTF_NUM_TEAMS; k++ )
			{
				pMetagameData->idTroops[i][j][k] = INVALID_ID;
			}

	for ( int k = 0; k < CTF_NUM_TEAMS; k++ )
	{
		pMetagameData->idTroopStartLocations[k] = INVALID_ID;
		pMetagameData->idTroopGoals[k] = INVALID_ID;

		pMetagameData->idFlags[k] = INVALID_ID;
		pMetagameData->idTeamGoals[k] = INVALID_ID;
		pMetagameData->idStartLocations[k] = INVALID_ID;

		pMetagameData->nTeams[k] = INVALID_ID;
	}

	pMetagameData->bAllowFlagPickup = FALSE;

	if (IS_SERVER(pGame))
	{
		s_sCTFSetState(CTF_GAME_STATE_STATIC_UNITS_UNINITIALIZED, pGame, NULL, pMetagameData);
	}

	//pMetagameData->nInfoTowerBuffCount = 0;
}


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CTFInit(
	GAME * game,
	const GAME_SETUP * game_setup)
{
	ASSERT_RETFALSE(game);

	if (!game_setup)
	{
		return TRUE;
	}
	
	METAGAME *pMetagame = game->m_pMetagame;
	ASSERT_RETFALSE(pMetagame);
	pMetagame->pfnMessageHandler = sCTFMessageHandler;

	// allocate quest specific data
	ASSERTX( pMetagame->pData == NULL, "Expected NULL quest data" );
	METAGAME_DATA_CTF * pData = ( METAGAME_DATA_CTF * )GMALLOCZ( game, sizeof( METAGAME_DATA_CTF ) );
	ASSERT_RETFALSE(pData);
	sMetagameDataInit( game, pMetagame, pData );
	pMetagame->pData = pData;

	GameSetGameFlag(game, GAMEFLAG_DISABLE_PLAYER_INTERACTION, TRUE);

	const WCHAR *pstrTeam0Name= GlobalStringGet( GS_PVP_TEAM_NAME_0 );
	const WCHAR *pstrTeam1Name= GlobalStringGet( GS_PVP_TEAM_NAME_1 );
	pData->nTeams[0] = TeamsAdd(game, pstrTeam0Name, "Team_Color_Red", TEAM_PLAYER_START, RELATION_HOSTILE, FALSE);	// reserver these special teams for this game, even if empty
	pData->nTeams[1] = TeamsAdd(game, pstrTeam1Name, "Team_Color_Blue", TEAM_PLAYER_START, RELATION_HOSTILE, FALSE);

	// these values could be adjusted by... players? voting?
	pData->tGameClockTicks = 0;	
	pData->nTeamScoreForTheWin = FLAG_CAPS_FOR_THE_WIN;			// score that will win a match for a team, or -1
	pData->nTroopLevel = -1;

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))
		c_SoundLoad(GlobalIndexGet(GI_SOUND_CTF_GROUP_LOAD));
#endif //!ISVERSION(SERVER_VERSION)

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetAddToOwnerTeam(
	UNIT *pPet,
	void *pUserData )
{
	UNIT * pOwner = (UNIT*)pUserData;
	ASSERT_RETURN( pOwner );
	ASSERT_RETURN( pPet );
	TeamAddUnit( pPet, pOwner->nTeam );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_CTFEventAddPlayer(
	GAME * game,
	UNIT * player)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERT_RETURN(game);

	if (GameGetSubType(game) != GAME_SUBTYPE_PVP_CTF)
	{
		return;
	}

	if (!IS_SERVER(game))
	{
		return;
	}
	
	// for now... figure out what team the player ought to be on
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet(GameGetMetagame(game));
	ASSERT_RETURN(pMetagameData);
	int teamWithLeast = INVALID_TEAM;
	unsigned int least = UINT_MAX;

	for (unsigned int ii = 0; ii < CTF_NUM_TEAMS; ++ii)
	{
		int team = pMetagameData->nTeams[ii];
		ASSERT_CONTINUE(team != INVALID_TEAM);
		ASSERT_CONTINUE(TeamGetBaseType(game, team) == TEAM_PLAYER_START);
		unsigned int player_count = TeamGetPlayerCount(game, team);
		if (player_count >= least)
		{
			continue;
		}
		teamWithLeast = team;
		least = player_count;
	}

	StateClearAllOfType( player, STATE_PVP_ENABLED );
	s_StateSet(player, player, STATE_PVP_MATCH, 0 );

	// add the player to a team
	ASSERT_RETURN(teamWithLeast != INVALID_TEAM);
	ASSERT_RETURN(TeamAddUnit(player, teamWithLeast));
	PetIteratePets(player, sPetAddToOwnerTeam, player);

	// set the team color state
 	int nTeamPlayerState = teamWithLeast == pMetagameData->nTeams[0] ? pMetagameData->nTeamPlayerStates[0] : pMetagameData->nTeamPlayerStates[1];
 	if (nTeamPlayerState != INVALID_LINK)
 		s_StateSet(player, player, nTeamPlayerState, 0 );

	WCHAR unit_name[MAX_CHARACTER_NAME];
	unit_name[0] = 0;
	UnitGetName(player, unit_name, arrsize(unit_name), 0);

	const WCHAR * strTeamName = TeamGetName(game, teamWithLeast);
	ASSERTX_RETURN( strTeamName && strTeamName[0], "expected a non empty string for team name in custom games" ); 
	const WCHAR *pstrJoinedFormatText = GlobalStringGet( GS_PVP_TEAM_PLAYER_JOINED );
	ASSERT(pstrJoinedFormatText);
	if (pstrJoinedFormatText)
		s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, pstrJoinedFormatText, unit_name, strTeamName);

	if (s_LevelGetPlayerCount(UnitGetLevel(player)) <= 1)
	{
		const WCHAR *pstrWaitingFormatText = GlobalStringGet( GS_PVP_ANNOUNCE_WAITING_FOR_PLAYERS );
		if (pstrWaitingFormatText)
			s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, pstrWaitingFormatText);
	}

	if (pMetagameData->eGameState == CTF_GAME_STATE_STATIC_UNITS_UNINITIALIZED)
	{
		LEVEL *pLevel = UnitGetLevel(player);
		s_sMetagameDataInitForLevel(pMetagameData, pLevel);
		s_sCTFSetState(CTF_GAME_STATE_WAITING_FOR_PLAYERS, game, pLevel, pMetagameData);
	}

	// TODO support side parties?
	if (UnitGetPartyId(player) != INVALID_ID)
		s_PlayerRemoveFromParty(player);

	s_PlayerEnableAutoParty(player, TRUE);

	// TODO set the game level so we can have a set troop level
	int nOffsetPlayerLevel = UnitGetExperienceLevel(player)-3;
	pMetagameData->nTroopLevel = MIN(pMetagameData->nTroopLevel, nOffsetPlayerLevel);
	if (pMetagameData->nTroopLevel <= 0)
		pMetagameData->nTroopLevel = nOffsetPlayerLevel;

	s_sSendGameData(game, pMetagameData, player);

	// TODO make them spectators
	CTFWarpToStart(game, player);
#endif;
}

struct CTFGetPlayerDataIterator
{
	PvPGameData *pGameData;
	METAGAME_DATA_CTF *pMetagameData;
	UNIT *pControlUnit;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sGetPlayerData(
	UNIT *pUnit, 
	void *pCallbackData )
{
	GAME *pGame = UnitGetGame(pUnit);
	CTFGetPlayerDataIterator *pGetPlayerDataIterator = (CTFGetPlayerDataIterator*)pCallbackData;
	ASSERT_RETVAL(pGetPlayerDataIterator && pGetPlayerDataIterator->pMetagameData && pGetPlayerDataIterator->pControlUnit, RIU_STOP);

	for (int t=0; t<CTF_NUM_TEAMS; ++t)
	{
		int nTeam = UnitGetTeam(pUnit);
		int nPlayerIndex = pGetPlayerDataIterator->pGameData->tTeamData[t].nNumPlayers;
		if (nTeam != INVALID_TEAM && 
			nTeam == pGetPlayerDataIterator->pMetagameData->nTeams[t] && 
			nPlayerIndex < CTF_MAX_PLAYERS_PER_TEAM)
		{
			PvPPlayerData *pPlayerData = pGetPlayerDataIterator->pGameData->tTeamData[t].tPlayerData + nPlayerIndex;
			pPlayerData->bIsControlUnit = (pGetPlayerDataIterator->pControlUnit == pUnit);
			pPlayerData->bCarryingFlag = UnitHasState(pGame, pUnit, STATE_PVP_CTF_PLAYER_HAS_FLAG);

			// player kills and flag captures
			pPlayerData->nPlayerKills = UnitGetStat(pUnit, STATS_PLAYER_KILLS);
			pPlayerData->nFlagCaptures = UnitGetStat(pUnit, STATS_FLAG_CAPTURES);
			pPlayerData->szName[0] = 0;
			UnitGetName(pUnit, pPlayerData->szName, arrsize(pPlayerData->szName), 0);

			++(pGetPlayerDataIterator->pGameData->tTeamData[t].nNumPlayers);
		}
	}

	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_CTFGetGameData(PvPGameData *pPvPGameData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(pPvPGameData);

	GAME *pGame = AppGetCltGame();
	ASSERT_RETFALSE(pGame && GameGetSubType(pGame) == GAME_SUBTYPE_PVP_CTF);

	UNIT * pUnit = GameGetControlUnit(pGame);
	METAGAME_DATA_CTF *pMetagameData = sMetagameDataGet( GameGetMetagame( pGame ) );
	if (pUnit && pMetagameData)
	{
		pPvPGameData->nSecondsRemainingInMatch = MAX(0, UnitGetStat(pUnit, STATS_GAME_CLOCK_TICKS)/GAME_TICKS_PER_SECOND);
		pPvPGameData->bHasTimelimit = (GAME_TIME_LIMIT_SECONDS > 0);
		pPvPGameData->nNumTeams = CTF_NUM_TEAMS;

		for ( int t = 0; t < CTF_NUM_TEAMS; ++t )
		{
			PvPTeamData *pTeamData = pPvPGameData->tTeamData + t;
			pTeamData->nScore = UnitGetStat(pUnit, STATS_TEAM_SCORE_TO_DISPLAY, pMetagameData->nTeams[t]);

			const WCHAR * szTeamName = TeamGetName(pGame, pMetagameData->nTeams[t]);
			ASSERTX_CONTINUE(szTeamName && szTeamName[0], "expected a non empty string for team name in PvP games"); 
			PStrCopy(pTeamData->szName, szTeamName, MAX_TEAM_NAME);

			// get flag status
			UNIT * pFlag = (pMetagameData->idFlags[t] == INVALID_ID) ? NULL : UnitGetById( pGame, pMetagameData->idFlags[t] );
			if ( pFlag && IsUnitDeadOrDying( pFlag ) )
				pFlag = NULL;

			if ( pFlag )
			{

				if (UnitHasState(pGame, pFlag, STATE_PVP_CTF_FLAG_DROPPED))
				{
					pTeamData->eFlagStatus = CTF_FLAG_STATUS_DROPPED_IN_WORLD;
				}
				else
				{
					pTeamData->eFlagStatus = CTF_FLAG_STATUS_SAFE;
				}
			}
			else
			{
				pTeamData->eFlagStatus = CTF_FLAG_STATUS_CARRIED_BY_ENEMY;
			}
		}

		CTFGetPlayerDataIterator tGetPlayerDataIterator;
		ZeroMemory(&tGetPlayerDataIterator, sizeof(CTFGetPlayerDataIterator));
		tGetPlayerDataIterator.pGameData = pPvPGameData;
		tGetPlayerDataIterator.pMetagameData = pMetagameData;
		tGetPlayerDataIterator.pControlUnit = pUnit;

		pPvPGameData->tTeamData[0].nNumPlayers = 0;
		pPvPGameData->tTeamData[1].nNumPlayers = 0;

		TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
		LevelIterateUnits(UnitGetLevel(pUnit), eTargetTypes, sGetPlayerData, &tGetPlayerDataIterator);

		return TRUE;
	}

#endif //!ISVERSION(SERVER_VERSION)

	return FALSE;
}