//----------------------------------------------------------------------------
// FILE: quest_testbeauty.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_testbeauty.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "s_questgames.h"
#include "states.h"
#include "room_layout.h"
#include "colors.h"
#include "room_path.h"
#include "ai.h"
#include "quest_testfellowship.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define TOB_ENEMY_LEVEL					28

#define NUM_TESTBEAUTY_ENEMIES			4

#define NUM_TOB_KNOT_SPAWNS				6

#define TOB_DEFAULT_BOIL_RESPAWN_TIME	( 10 * GAME_TICKS_PER_SECOND )		// 10 second boil respawn

//----------------------------------------------------------------------------

enum TOB_KNOT_INDEX
{
	QUEST_TOB_KNOT_LOW = 0,
	QUEST_TOB_KNOT_LM,
	QUEST_TOB_KNOT_MID,
	QUEST_TOB_KNOT_UM,
	QUEST_TOB_KNOT_UP,

	QUEST_TOB_NUM_KNOTS
};

//----------------------------------------------------------------------------

enum TOB_TEAMS
{
	TOB_TEAM_NEUTRAL = 0,
	TOB_TEAM_PLAYER,
	TOB_TEAM_DEMON
};

//----------------------------------------------------------------------------

struct QUEST_TOB_KNOT
{
	int		nTeamState;
	UNITID	idSpawns[ NUM_TOB_KNOT_SPAWNS ];
};

//----------------------------------------------------------------------------

struct QUEST_GAME_DATA_TESTBEAUTY
{
	UNITID			idKnots[ QUEST_TOB_NUM_KNOTS ];
	QUEST_TOB_KNOT	tKnotInfo[ QUEST_TOB_NUM_KNOTS ];
	UNITID			idImps[ NUM_TESTBEAUTY_ENEMIES ];
	int				nNextKnot;
};

//----------------------------------------------------------------------------
struct QUEST_DATA_TESTBEAUTY
{
	int		nTruthNPC;
	int		nLevelTestOfBeauty;

	int		nImpUber;
	int		nBoilLarge;
	int		nBoilMedium;

	int		nKnot;
	int		nOperatingKnotState;		// used to keep current record of state at the start of an operate

	int		nSigil;

	QUEST_GAME_DATA_TESTBEAUTY tGameData;
};

//----------------------------------------------------------------------------

static char * sgszToBLayoutLabels[ QUEST_TOB_NUM_KNOTS ] =
{
	"knot1",
	"knot2",
	"knot3",
	"knot4",
	"knot5",
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TESTBEAUTY * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TESTBEAUTY *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_GAME_DATA_TESTBEAUTY * sQuestGameDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_TESTBEAUTY * pQuestData = (QUEST_DATA_TESTBEAUTY *)pQuest->pQuestData;
	return (QUEST_GAME_DATA_TESTBEAUTY *)&pQuestData->tGameData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitNodeInfo( QUEST * pQuest, TOB_KNOT_INDEX nIndex )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	pGameData->idKnots[nIndex] = INVALID_ID;
	pGameData->tKnotInfo[nIndex].nTeamState = TOB_TEAM_NEUTRAL;
	for ( int i = 0; i < NUM_TOB_KNOT_SPAWNS; i++ )
	{
		pGameData->tKnotInfo[nIndex].idSpawns[i] = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitializeGameData( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		sInitNodeInfo( pQuest, (TOB_KNOT_INDEX)i );
	}
	for ( int e = 0; e < NUM_TESTBEAUTY_ENEMIES; e++ )
	{
		pGameData->idImps[e] = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCopyGameData( QUEST * pQuest, UNIT * pFromPlayer )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	ASSERT_RETURN( pFromPlayer );

	QUEST * pFromQuest = QuestGetQuest( pFromPlayer, pQuest->nQuest );
	if ( !pFromQuest )
		return;

	QUEST_DATA_TESTBEAUTY * pToQuestData = sQuestDataGet( pQuest );
	QUEST_DATA_TESTBEAUTY * pFromQuestData = sQuestDataGet( pFromQuest );

	pToQuestData->tGameData = pFromQuestData->tGameData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sFreeUnitById( GAME * pGame, UNITID idEnemy )
{
	// could have been killed or deleted
	if ( idEnemy == INVALID_ID )
		return;

	UNIT * enemy = UnitGetById( pGame, idEnemy );
	if ( enemy )
	{
		UnitFree( enemy, UFF_SEND_TO_CLIENTS );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sShutdownGame( QUEST * pQuest )
{
	// delete all the enemies
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );

	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		if ( pGameData->idKnots[i] != INVALID_ID )
		{
			// delete node
			sFreeUnitById( pGame, pGameData->idKnots[i] );
		}
	}

	for ( int e = 0; e < NUM_TESTBEAUTY_ENEMIES; e++ )
	{
		sFreeUnitById( pGame, pGameData->idImps[e] );
	}
	sInitializeGameData( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCheckKnotChange( QUEST * pQuest, TOB_KNOT_INDEX eKnot )
{
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	BOOL bEmpty = TRUE;
	BOOL bFull = TRUE;
	for ( int i = 0; i < NUM_TOB_KNOT_SPAWNS; i++ )
	{
		if ( pGameData->tKnotInfo[eKnot].idSpawns[i] != INVALID_ID )
			bEmpty = FALSE;
		else
			bFull = FALSE;
	}

	// that knot is now empty... change it to player control
	UNIT * pKnot = UnitGetById( UnitGetGame( pQuest->pPlayer ), pGameData->idKnots[eKnot] );

	if ( bEmpty && ( pGameData->tKnotInfo[eKnot].nTeamState != TOB_TEAM_PLAYER ) )
	{
		pGameData->tKnotInfo[eKnot].nTeamState = TOB_TEAM_PLAYER;
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_CHANGE_KNOT, pKnot, pGameData->tKnotInfo[eKnot].nTeamState );
		s_StateSet( pKnot, pKnot, STATE_BEACON, 0 );
	}
	else if ( bFull && ( pGameData->tKnotInfo[eKnot].nTeamState != TOB_TEAM_DEMON ) )
	{
		pGameData->tKnotInfo[eKnot].nTeamState = TOB_TEAM_DEMON;
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_CHANGE_KNOT, pKnot, pGameData->tKnotInfo[eKnot].nTeamState );
		s_StateClear( pKnot, pGameData->idKnots[eKnot], STATE_BEACON, 0 );
	}
	else if ( !bEmpty && !bFull && ( pGameData->tKnotInfo[eKnot].nTeamState != TOB_TEAM_NEUTRAL ) )
	{
		pGameData->tKnotInfo[eKnot].nTeamState = TOB_TEAM_NEUTRAL;
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_CHANGE_KNOT, pKnot, pGameData->tKnotInfo[eKnot].nTeamState );
		s_StateClear( pKnot, pGameData->idKnots[eKnot], STATE_BEACON, 0 );
	}

	// check victory or fail
	BOOL bWin = TRUE;
	BOOL bLose = TRUE;
	for ( int k = 0; k < QUEST_TOB_NUM_KNOTS; k++ )
	{
		if ( pGameData->tKnotInfo[k].nTeamState != TOB_TEAM_PLAYER )
			bWin = FALSE;
		if ( pGameData->tKnotInfo[k].nTeamState != TOB_TEAM_DEMON )
			bLose = FALSE;
	}

	// victory!
	if ( bWin )
	{
		s_QuestGameVictory( pQuest );
		return;
	}

	// lose!
	if ( bLose )
	{
		s_QuestGameFailed( pQuest );
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSetNextKnot( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );

	pGameData->nNextKnot = 0;
	while ( ( pGameData->nNextKnot < QUEST_TOB_NUM_KNOTS ) && ( pGameData->tKnotInfo[pGameData->nNextKnot].nTeamState == TOB_TEAM_PLAYER ) )
	{
		pGameData->nNextKnot++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UNITID sSpawnBoil( QUEST * pQuest, TOB_KNOT_INDEX nIndex )
{
	ASSERT_RETVAL( pQuest, INVALID_ID );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), INVALID_ID, "Server only" );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETVAL( pGameData, INVALID_ID );

	ASSERT_RETVAL( pGameData->idKnots[nIndex] != INVALID_ID, INVALID_ID );
	UNIT * pKnot = UnitGetById( pGame, pGameData->idKnots[nIndex] );
	ROOM * pDestRoom;
	VECTOR vDirection;
	float fAngle = RandGetFloat( pGame->rand, TWOxPI );
	vDirection.fX = cosf( fAngle );
	vDirection.fY = sinf( fAngle );
	vDirection.fZ = 0.0f;
	VectorNormalize( vDirection );
	VECTOR vDelta = vDirection * RandGetFloat( pGame->rand, 3.0f, 10.0f );
	VECTOR vDestination = pKnot->vPosition + vDelta;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, UnitGetRoom( pKnot ), vDestination, &pDestRoom );
	ASSERT_RETVAL( pPathNode, INVALID_ID );

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	MONSTER_SPEC tSpec;
	if ( RandGetNum( pGame->rand, 100 ) < 50 )
		tSpec.nClass = pQuestData->nBoilLarge;
	else
		tSpec.nClass = pQuestData->nBoilMedium;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pDestRoom ), tSpec.nClass, TOB_ENEMY_LEVEL );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = vDirection;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * boil = s_MonsterSpawn( pGame, tSpec );
	ASSERT( boil );
	return UnitGetId( boil );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnKnotBoils( QUEST * pQuest, TOB_KNOT_INDEX nIndex, BOOL bSend )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );

	for ( int i = 0; i < NUM_TOB_KNOT_SPAWNS; i++ )
	{
		ASSERT_RETURN( pGameData->tKnotInfo[nIndex].idSpawns[i] == INVALID_ID );

		UNITID idNewBoil = sSpawnBoil( pQuest, nIndex );
		if ( bSend )
		{
			UNIT * boil = UnitGetById( UnitGetGame( pQuest->pPlayer ), idNewBoil );
			s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_ADD_BOIL, boil, ( nIndex << 8 ) + i );
		}
		else
		{
			pGameData->tKnotInfo[nIndex].idSpawns[i] = idNewBoil;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sBoilRespawn( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERTX_RETFALSE( IS_SERVER( game ), "Server only" );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERTX_RETFALSE( pGameData, "No Quest Game Data found" );

	// find first knot to respawn at...
	for ( int k = QUEST_TOB_NUM_KNOTS-1; k >=0; k-- )
	{
		for ( int b = 0; b < NUM_TOB_KNOT_SPAWNS; b++ )
		{
			if ( pGameData->tKnotInfo[k].idSpawns[b] == INVALID_ID )
			{
				UNITID idNewBoil = sSpawnBoil( pQuest, (TOB_KNOT_INDEX)k );
				UNIT * boil = UnitGetById( game, idNewBoil );
				s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_ADD_BOIL, boil, ( k << 8 ) + b );
				UnitRegisterEventTimed( unit, sBoilRespawn, &tEventData, TOB_DEFAULT_BOIL_RESPAWN_TIME );
				sCheckKnotChange( pQuest, (TOB_KNOT_INDEX)k );
				return TRUE;
			}
		}
	}

	ASSERTX_RETFALSE( 0, "Could not find a free boil slot. should never happen" );
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnKnots( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		sInitNodeInfo( pQuest, (TOB_KNOT_INDEX)i );

		QUEST_GAME_LABEL_NODE_INFO tInfo;
		BOOL bRetVal = s_QuestGameFindLayoutLabel( UnitGetLevel( pPlayer ), sgszToBLayoutLabels[i], &tInfo );
		ASSERT_RETURN( bRetVal );

		QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
		OBJECT_SPEC tSpec;
		tSpec.nClass = pQuestData->nKnot;
		tSpec.pRoom = tInfo.pRoom;
		tSpec.vPosition = tInfo.vPosition;
		tSpec.pvFaceDirection = &tInfo.vDirection;
		UNIT * node = s_ObjectSpawn( UnitGetGame( pPlayer ), tSpec );
		ASSERT_RETURN( node );
		pQuestData->tGameData.idKnots[i] = UnitGetId( node );
		switch ( i )
		{
		case QUEST_TOB_KNOT_UP:
		case QUEST_TOB_KNOT_UM:
			pQuestData->tGameData.tKnotInfo[i].nTeamState = TOB_TEAM_DEMON;
			sSpawnKnotBoils( pQuest, (TOB_KNOT_INDEX)i, FALSE );
			break;
		case QUEST_TOB_KNOT_LOW:
		case QUEST_TOB_KNOT_LM:
			pQuestData->tGameData.tKnotInfo[i].nTeamState = TOB_TEAM_PLAYER;
			s_StateSet( node, node, STATE_BEACON, 0 );
			break;
		case QUEST_TOB_KNOT_MID:
			pQuestData->tGameData.tKnotInfo[i].nTeamState = TOB_TEAM_NEUTRAL;
			break;
		}
		// set boil respawn controller to 0 knot...
		if ( i == 0 )
		{
			UnitRegisterEventTimed( node, sBoilRespawn, &EVENT_DATA( pQuest->nQuest ), TOB_DEFAULT_BOIL_RESPAWN_TIME );
		}
	}


	sSetNextKnot( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnInitialEnemies( QUEST * pQuest )
{
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	int nTOBEnemyLevel = QuestGetMonsterLevel( pQuest, UnitGetLevel( pQuest->pPlayer ), pQuestData->nImpUber, TOB_ENEMY_LEVEL );

	for ( int e = 0; e < NUM_TESTBEAUTY_ENEMIES; e++ )
	{
		pGameData->idImps[e] = s_QuestGameSpawnEnemy( pQuest, "MonsterSpawn", pQuestData->nImpUber, nTOBEnemyLevel );
		if ( pGameData->idImps[e] != INVALID_ID )
		{
			GAME * pGame = UnitGetGame( pQuest->pPlayer );
			UNIT * imp = UnitGetById( pGame, pGameData->idImps[e] );
			UnitSetStat( imp, STATS_AI_ACTIVE_QUEST_ID, pQuest->nQuest );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUpdateRadar( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		ASSERT_RETURN( pGameData->idKnots[i] != INVALID_ID );
		UNIT * pNode = UnitGetById( UnitGetGame( pQuest->pPlayer ), pGameData->idKnots[i] );
		ASSERT_RETURN( pNode );

		DWORD dwColor = GFXCOLOR_WHITE;
		switch ( pGameData->tKnotInfo[i].nTeamState )
		{
		case TOB_TEAM_NEUTRAL:
			dwColor = GFXCOLOR_GRAY;
			break;
		case TOB_TEAM_DEMON:
			dwColor = GFXCOLOR_RED;
			break;
		case TOB_TEAM_PLAYER:
			dwColor = GFXCOLOR_BLUE;
			break;
		}
		// special case: next knot to activate
		//if ( i == pGameData->nNextKnot )
		//{
		//	dwColor = GFXCOLOR_GREEN;
		//}
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, pNode, 0, dwColor );
	}
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_ON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAIUpdate( QUEST * pQuest, UNIT * monster )
{
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( monster );
	ASSERTX_RETURN( UnitGetClass( monster ) == pQuestData->nImpUber, "Test of Beauty Strategy is Imp_uber only" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( IsUnitDeadOrDying( monster ) )
		return;

	// clear previous flags
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_ATTACK, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );

	// find objects and players around me
	QUEST_GAME_NEARBY_INFO	tNearbyInfo;
	s_QuestGameNearbyInfo( monster, &tNearbyInfo );

	// attack nearby players
	if ( tNearbyInfo.nNumNearbyPlayers > 0 )
	{
		// attack first player
		UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[0] ) );
		s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageGameMessage( 
	QUEST *pQuest,
	const QUEST_MESSAGE_GAME_MESSAGE *pMessage)
{
	QUEST_GAME_MESSAGE_TYPE eCommand = (QUEST_GAME_MESSAGE_TYPE)pMessage->nCommand;

	switch ( eCommand )
	{
		//----------------------------------------------------------------------------
		case QGM_GAME_NEW:
		{
			// first one in quest-game, so start it up!
			sInitializeGameData( pQuest );
			sSpawnKnots( pQuest );
			sSpawnInitialEnemies( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_COPY_CURRENT_STATE:
		{
			// copy scores, data, etc for current state of the game
			sInitializeGameData( pQuest );
			sCopyGameData( pQuest, pMessage->pUnit );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_DESTROY:
		{
			// everyone has left, so delete all the game-specific data
			sShutdownGame( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_JOIN:
		{
			// this is a respawn in the arena game
			s_StateSet( pQuest->pPlayer, pQuest->pPlayer, STATE_QUEST_GAME_ARENA_RESPAWN, 0 );
			// send me into the game
			s_QuestGameWarpPlayerIntoArena( pQuest, pQuest->pPlayer );
			sUpdateRadar( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_LEAVE:
		{
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_VICTORY:
		{
			if ( QuestIsActive( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_TESTBEAUTY_PASS, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TESTBEAUTY_SIGIL, QUEST_STATE_ACTIVE );
			}
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOB_VICTORY );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "VictorySpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_FAILED:
		{
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOB_FAIL );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "FailSpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_AIUPDATE:
		{
			sAIUpdate( pQuest, pMessage->pUnit );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_ARENA_RESPAWN:
		{
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		// custom Test of Beauty only messages
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOB_ADD_BOIL:
		{
			QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
			ASSERT_RETVAL( pGameData, QMR_OK );
			UNITID idBoil = UnitGetId( pMessage->pUnit );
			int nIndex = pMessage->dwData >> 8;
			int nSpawnNum = pMessage->dwData & 0xff;
			ASSERT_RETVAL( pGameData->tKnotInfo[nIndex].idSpawns[nSpawnNum] == INVALID_ID, QMR_OK );
			pGameData->tKnotInfo[nIndex].idSpawns[nSpawnNum] = idBoil;
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOB_REMOVE_BOIL:
		{
			QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
			ASSERT_RETVAL( pGameData, QMR_OK );
			UNITID idBoil = UnitGetId( pMessage->pUnit );
			int nIndex = pMessage->dwData >> 8;
			int nSpawnNum = pMessage->dwData & 0xff;
			ASSERT_RETVAL( pGameData->tKnotInfo[nIndex].idSpawns[nSpawnNum] == idBoil, QMR_OK );
			pGameData->tKnotInfo[nIndex].idSpawns[nSpawnNum] = INVALID_ID;
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOB_CHANGE_KNOT:
		{
			QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
			UNITID idUnit = UnitGetId( pMessage->pUnit );
			for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
			{
				if ( idUnit == pGameData->idKnots[i] )
				{
					pGameData->tKnotInfo[i].nTeamState = pMessage->dwData;
					if ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_PLAYER )
					{
						s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOB_CLEAN );
					}
					else if ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_DEMON )
					{
						s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOB_DEMONCORRUPT );
					}
				}
			}
			sSetNextKnot( pQuest );
			sUpdateRadar( pQuest );
			return QMR_OK;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTruthNPC ))
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelTestOfBeauty )
			return QMR_OK;

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOB_RULES );
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	//UNIT *pTalkingTo = pMessage->pTalkingTo;
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TOB_RULES:
		{
			s_QuestToBBegin( pQuest );
			s_QuestGameJoin( pQuest );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
static void sChangingKnot( QUEST * pQuest, UNITID idUnit )
{
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );

	// node may have switched by someone else
	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		if ( idUnit == pGameData->idKnots[i] )
		{
			// if it isn't the starting state, abort!
			if ( pQuestData->nOperatingKnotState != pGameData->tKnotInfo[i].nTeamState )
				return;

			if ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_DEMON )
			{
				//sFreeProtectors( pQuest, (TOL_NODE_INDEX)i );
				pGameData->tKnotInfo[i].nTeamState = TOB_TEAM_NEUTRAL;
			}
			else if ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_NEUTRAL )
			{
				pGameData->tKnotInfo[i].nTeamState = TOB_TEAM_PLAYER;
				//sSpawnNodeProtectors( pQuest, (TOL_NODE_INDEX)i, TRUE );
				//victory!
				if ( i == QUEST_TOB_NUM_KNOTS-1 )
				{
					s_QuestGameVictory( pQuest );
					return;
				}
			}
			else
			{
				// clicked on player?
				return;
			}

			UNIT * pKnot = UnitGetById( UnitGetGame( pQuest->pPlayer ), idUnit );
			s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_CHANGE_KNOT, pKnot, pGameData->tKnotInfo[i].nTeamState );
		}
	}
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
static void sKnotActivate( QUEST * pQuest, UNIT * pKnot )
{
	QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
	// find which knot first...
	for ( int i = 0; i < QUEST_TOB_NUM_KNOTS; i++ )
	{
		if ( UnitGetId( pKnot ) == pGameData->idKnots[i] )
		{
			// find out state
			BOOL bOk = FALSE;
			if ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_DEMON )
			{
				bOk = TRUE;
			}
			if ( ( pGameData->tKnotInfo[i].nTeamState == TOB_TEAM_NEUTRAL ) && ( i == pGameData->nNextKnot ) )
			{
				bOk = TRUE;
			}
			if ( bOk )
			{
				QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
				pQuestData->nOperatingKnotState = pGameData->tKnotInfo[i].nTeamState;
				//s_QuestOperateDelay( pQuest, sChangingKnot, UnitGetId( pKnot ), 5 * GAME_TICKS_PER_SECOND );
				// bug with clicking... make instant right now. fix tomorrow - drb
				sChangingKnot( pQuest, UnitGetId( pKnot ) );
			}
		}
	}
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nTargetClass = UnitGetClass( pTarget );

/*	if ( nTargetClass == pQuestData->nKnot )
	{
		sKnotActivate( pQuest, pTarget );
		return QMR_FINISHED;
	}*/

	if ( nTargetClass == pQuestData->nSigil )
	{
		if ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelTestOfBeauty )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTBEAUTY_SIGIL, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			QUEST * pTest = QuestGetQuest( pQuest->pPlayer, QUEST_TESTFELLOWSHIP );
			s_QuestToFBegin( pTest );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "FailSpawn" );
			return QMR_FINISHED;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageLeaveLevel(
	QUEST *pQuest,
	const QUEST_MESSAGE_LEAVE_LEVEL * pMessage)
{
	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), QMR_OK, "Server only" );

	if ( !QuestGamePlayerInGame( pPlayer ) )
		return QMR_OK;

	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	if ( pLevel->m_nLevelDef == pQuestData->nLevelTestOfBeauty )
	{
		s_QuestGameLeave( pQuest );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_TESTBEAUTY * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelTestOfBeauty )
		return QMR_OK;

	//QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );

	int nMonsterClass = UnitGetClass( pVictim );
	//UNITID idVictim = UnitGetId( pVictim );

	// if the uber imp died, respawn...
	if ( nMonsterClass == pQuestData->nImpUber )
	{
		if ( UnitGetLevelDefinitionIndex( pVictim ) == pQuestData->nLevelTestOfBeauty )
		{
			if ( !UnitHasTimedEvent( pVictim, s_QuestGameRespawnMonster ) )
			{
				UNITID idVictim = UnitGetId( pVictim );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_DEFEND, 0 );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_ATTACK, 0 );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_RUN_TO, 0 );
				UnitRegisterEventTimed( pVictim, s_QuestGameRespawnMonster, &EVENT_DATA(), GAME_TICKS_PER_SECOND * 3 );
			}
			return QMR_FINISHED;
		}
		return QMR_OK;
	}

	if ( ( nMonsterClass == pQuestData->nBoilLarge ) || ( nMonsterClass == pQuestData->nBoilMedium ) )
	{
		ASSERT_RETVAL( UnitGetLevelDefinitionIndex( pVictim ) == pQuestData->nLevelTestOfBeauty, QMR_OK );
		QUEST_GAME_DATA_TESTBEAUTY * pGameData = sQuestGameDataGet( pQuest );
		UNITID idVictim = UnitGetId( pVictim );
		for ( int k = 0; k < QUEST_TOB_NUM_KNOTS; k++ )
		{
			for ( int i = 0; i < NUM_TOB_KNOT_SPAWNS; i++ )
			{
				if ( pGameData->tKnotInfo[k].idSpawns[i] == idVictim )
				{
					s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOB_REMOVE_BOIL, pVictim, (DWORD)( ( k << 8 ) + i ) );
					sCheckKnotChange( pQuest, (TOB_KNOT_INDEX)k );
				}
			}
		}
		return QMR_FINISHED;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	switch (eMessageType)
	{

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pLeaveLevelMessage = (const QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
			return sQuestMessageLeaveLevel( pQuest, pLeaveLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_GAME_MESSAGE:
		{
			const QUEST_MESSAGE_GAME_MESSAGE * pQuestGameMessage = (const QUEST_MESSAGE_GAME_MESSAGE *)pMessage;
			return sQuestMessageGameMessage( pQuest, pQuestGameMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTestBeauty(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
	if ( pQuest->pGameData )
	{
		s_QuestGameDataFree( pQuest );
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_TESTBEAUTY * pQuestData)
{
	pQuestData->nTruthNPC			= QuestUseGI( pQuest, GI_MONSTER_TRUTH_C_SAGE_NEW );
	pQuestData->nLevelTestOfBeauty	= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_BEAUTY );
	pQuestData->nImpUber			= QuestUseGI( pQuest, GI_MONSTER_IMP_UBER );
	pQuestData->nBoilLarge			= QuestUseGI( pQuest, GI_MONSTER_TOB_BOIL_LARGE );
	pQuestData->nBoilMedium			= QuestUseGI( pQuest, GI_MONSTER_TOB_BOIL_MEDIUM );
	pQuestData->nKnot				= QuestUseGI( pQuest, GI_OBJECT_TESTBEAUTY_KNOT );
	pQuestData->nSigil				= QuestUseGI( pQuest, GI_OBJECT_SIGIL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestBeauty(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTBEAUTY * pQuestData = ( QUEST_DATA_TESTBEAUTY * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TESTBEAUTY ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
	if ( IS_SERVER( pGame ) )
	{
		s_QuestGameDataInit( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTestBeauty(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestToBBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTBEAUTY, "Wrong quest sent to function. Need Test of Beauty" );

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_ACTIVE )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
}
