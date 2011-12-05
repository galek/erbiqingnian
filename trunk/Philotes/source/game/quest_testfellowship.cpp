//----------------------------------------------------------------------------
// FILE: quest_testfellowship.cpp
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
#include "quest_testfellowship.h"
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
#include "quest_testhope.h"
#include "uix.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define TOF_ENEMY_LEVEL					21

#define NUM_TESTFELLOWSHIP_ENEMIES		4

#define TOF_MONSTER_SPAWN_DELAY			( 25 * GAME_TICKS_PER_SECOND )		
#define TOF_SCORE_UPDATE_DELAY			(  5 * GAME_TICKS_PER_SECOND )		// how many ticks a team needs to hold a node to get a point
#define TOF_PLAYER_OPERATE_DELAY		(  5 * GAME_TICKS_PER_SECOND )
#define TOF_MONSTER_OPERATE_DELAY		(  8 * GAME_TICKS_PER_SECOND )
#define TOF_MONSTER_START_DELAY			( 10 * GAME_TICKS_PER_SECOND )

#define TOF_VICTORY_SCORE				100

//----------------------------------------------------------------------------

enum TOF_NODE_INDEX
{
	QUEST_TOF_NODE_UL = 0,
	QUEST_TOF_NODE_UR,
	QUEST_TOF_NODE_MID,
	QUEST_TOF_NODE_LL,
	QUEST_TOF_NODE_LR,

	NUM_TESTFELLOWSHIP_NODES
};

//----------------------------------------------------------------------------

enum TOF_TEAMS
{
	TOF_TEAM_NEUTRAL = 0,
	TOF_TEAM_PLAYER,
	TOF_TEAM_DEMON
};

//----------------------------------------------------------------------------

struct QUEST_GAME_DATA_TESTFELLOWSHIP
{
	UNITID			idNodes[ NUM_TESTFELLOWSHIP_NODES ];
	int				nNodeTeam[ NUM_TESTFELLOWSHIP_NODES ];
	UNITID			idTakingNode[ NUM_TESTFELLOWSHIP_NODES ];
	UNITID			idImps[ NUM_TESTFELLOWSHIP_ENEMIES ];
	int				nPlayerScore;
	int				nDemonScore;
};

//----------------------------------------------------------------------------

struct QUEST_DATA_TESTFELLOWSHIP
{
	int		nTruthNPC;
	int		nLevelTestOfFellowship;

	int		nImpUber;

	int		nNode;
	int		nOperatingNodeState;		// used to keep current record of state at the start of an operate

	int		nSigil;

	QUEST_GAME_DATA_TESTFELLOWSHIP tGameData;
};

//----------------------------------------------------------------------------

static char * sgszToBLayoutLabels[ NUM_TESTFELLOWSHIP_NODES ] =
{
	"ToF_UL",
	"ToF_UR",
	"ToF_Mid",
	"ToF_LL",
	"ToF_LR",
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TESTFELLOWSHIP * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TESTFELLOWSHIP *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_GAME_DATA_TESTFELLOWSHIP * sQuestGameDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = (QUEST_DATA_TESTFELLOWSHIP *)pQuest->pQuestData;
	return (QUEST_GAME_DATA_TESTFELLOWSHIP *)&pQuestData->tGameData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitializeGameData( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		pGameData->idNodes[i] = INVALID_ID;
		pGameData->idTakingNode[i] = INVALID_ID;
		pGameData->nNodeTeam[i] = TOF_TEAM_NEUTRAL;
	}
	for ( int e = 0; e < NUM_TESTFELLOWSHIP_ENEMIES; e++ )
	{
		pGameData->idImps[e] = INVALID_ID;
	}
	pGameData->nPlayerScore = 0;
	pGameData->nDemonScore = 0;
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

	QUEST_DATA_TESTFELLOWSHIP * pToQuestData = sQuestDataGet( pQuest );
	QUEST_DATA_TESTFELLOWSHIP * pFromQuestData = sQuestDataGet( pFromQuest );

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
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );

	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( pGameData->idNodes[i] != INVALID_ID )
		{
			// delete node
			sFreeUnitById( pGame, pGameData->idNodes[i] );
		}
	}

	for ( int e = 0; e < NUM_TESTFELLOWSHIP_ENEMIES; e++ )
	{
		sFreeUnitById( pGame, pGameData->idImps[e] );
	}
	sInitializeGameData( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnNodes( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		QUEST_GAME_LABEL_NODE_INFO tInfo;
		BOOL bRetVal = s_QuestGameFindLayoutLabel( UnitGetLevel( pPlayer ), sgszToBLayoutLabels[i], &tInfo );
		ASSERT_RETURN( bRetVal );

		OBJECT_SPEC tSpec;
		tSpec.nClass = pQuestData->nNode;
		tSpec.pRoom = tInfo.pRoom;
		tSpec.vPosition = tInfo.vPosition;
		tSpec.pvFaceDirection = &tInfo.vDirection;
		UNIT * node = s_ObjectSpawn( UnitGetGame( pPlayer ), tSpec );
		ASSERT_RETURN( node );
		pGameData->idNodes[i] = UnitGetId( node );

		pGameData->nNodeTeam[i] = TOF_TEAM_NEUTRAL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnInitialEnemies( QUEST * pQuest )
{
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	int nTOFEnemyLevel = QuestGetMonsterLevel( pQuest, UnitGetLevel( pQuest->pPlayer ), pQuestData->nImpUber, TOF_ENEMY_LEVEL );

	for ( int e = 0; e < NUM_TESTFELLOWSHIP_ENEMIES; e++ )
	{
		pGameData->idImps[e] = s_QuestGameSpawnEnemy( pQuest, "MonsterSpawn", pQuestData->nImpUber, nTOFEnemyLevel );
		if ( pGameData->idImps[e] != INVALID_ID )
		{
			GAME * pGame = UnitGetGame( pQuest->pPlayer );
			UNIT * imp = UnitGetById( pGame, pGameData->idImps[e] );
			UnitSetStat( imp, STATS_AI_ACTIVE_QUEST_ID, pQuest->nQuest );
			s_StateSet( imp, imp, STATE_QUEST_GAME_STARTING, TOF_MONSTER_START_DELAY);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUpdateRadar( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		ASSERT_RETURN( pGameData->idNodes[i] != INVALID_ID );
		UNIT * pNode = UnitGetById( UnitGetGame( pQuest->pPlayer ), pGameData->idNodes[i] );
		ASSERT_RETURN( pNode );

		DWORD dwColor = GFXCOLOR_WHITE;
		switch ( pGameData->nNodeTeam[i] )
		{
		case TOF_TEAM_NEUTRAL:
			dwColor = GFXCOLOR_GRAY;
			break;
		case TOF_TEAM_DEMON:
			dwColor = GFXCOLOR_RED;
			break;
		case TOF_TEAM_PLAYER:
			dwColor = GFXCOLOR_BLUE;
			break;
		}
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, pNode, 0, dwColor );
	}
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_ON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUpdateScore( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	s_QuestGameUIMessage( pQuest, QUIM_GLOBAL_STRING_SCORE, GS_TOF_SCORE_MESSAGE, pGameData->nPlayerScore, pGameData->nDemonScore );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static TOF_TEAMS sNodeTeam( QUEST * pQuest, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find which node first...
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( UnitGetId( pNode ) == pGameData->idNodes[i] )
		{
			return (TOF_TEAMS)pGameData->nNodeTeam[i];
		}
	}
	ASSERTX( 0, "Could not find node in test of fellowship" );
	return TOF_TEAM_NEUTRAL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sAddPlayerScore( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERTX_RETFALSE( IS_SERVER( game ), "Server only" );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERTX_RETFALSE( pGameData, "No Quest Game Data found" );

	int nNewScore = pGameData->nPlayerScore + 1;
	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOF_PLAYER_SCORE, unit, nNewScore );

	if ( nNewScore < TOF_VICTORY_SCORE )
	{
		UnitRegisterEventTimed( unit, sAddPlayerScore, &tEventData, TOF_SCORE_UPDATE_DELAY );
	}
	else
	{
		s_QuestGameVictory( pQuest );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sAddDemonScore( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERTX_RETFALSE( IS_SERVER( game ), "Server only" );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERTX_RETFALSE( pGameData, "No Quest Game Data found" );

	int nNewScore = pGameData->nDemonScore + 1;
	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOF_DEMON_SCORE, unit, nNewScore );

	if ( nNewScore < TOF_VICTORY_SCORE )
	{
		UnitRegisterEventTimed( unit, sAddDemonScore, &tEventData, TOF_SCORE_UPDATE_DELAY );
	}
	else
	{
		s_QuestGameFailed( pQuest );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDemonChangingNode( QUEST * pQuest, UNITID idUnit )
{
	ASSERT_RETURN(pQuest);
	if (!QuestGamePlayerInGame( QuestGetPlayer(pQuest) ))
		return;

	ASSERT_RETURN(idUnit != INVALID_ID);
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN(pGameData);

	// node may have switched by someone else
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( idUnit == pGameData->idNodes[i] )
		{
			UNIT * pNode = UnitGetById( UnitGetGame( pQuest->pPlayer ), idUnit );
			if (!pNode)
				continue;

			if ( pGameData->nNodeTeam[i] == TOF_TEAM_PLAYER )
			{
				pGameData->idTakingNode[i] = INVALID_ID;
				pGameData->nNodeTeam[i] = TOF_TEAM_NEUTRAL;
				s_StateClear(pNode, idUnit, STATE_QUEST_GAME_NODE_PLAYER, 0);
				UnitUnregisterTimedEvent( pNode, sAddPlayerScore );
				UnitUnregisterTimedEvent( pNode, sAddDemonScore );
			}
			else if ( pGameData->nNodeTeam[i] == TOF_TEAM_NEUTRAL )
			{
				pGameData->idTakingNode[i] = INVALID_ID;
				pGameData->nNodeTeam[i] = TOF_TEAM_DEMON;
				s_StateSet(pNode, pNode, STATE_QUEST_GAME_NODE_DEMON, 0);
				UnitRegisterEventTimed( pNode, sAddDemonScore, &EVENT_DATA( pQuest->nQuest ), TOF_SCORE_UPDATE_DELAY );
			}
			else
			{
				// clicked on demon?
				return;
			}

			s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOF_CHANGE_NODE, pNode, pGameData->nNodeTeam[i] );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sDemonStartGrabbingNode( QUEST * pQuest, UNIT *pMonster, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find which node first...
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( UnitGetId( pNode ) == pGameData->idNodes[i] )
		{
			// find out state
			if ( pGameData->nNodeTeam[i] != TOF_TEAM_DEMON /*&&
				 pGameData->idTakingNode[i] == INVALID_ID*/)
			{
				pGameData->idTakingNode[i] = UnitGetId(pMonster);
				s_QuestNonPlayerOperateDelay( pQuest, pMonster, sDemonChangingNode, UnitGetId( pNode ), TOF_MONSTER_OPERATE_DELAY );
				return TRUE;
			}
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAIUpdate( QUEST * pQuest, UNIT * monster )
{
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( monster );
	ASSERTX_RETURN( UnitGetClass( monster ) == pQuestData->nImpUber, "Test of Knowledge Strategy is Imp_uber only" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( IsUnitDeadOrDying( monster ) )
		return;

	// clear previous flags
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_ATTACK, 0 );

	// find objects and players around me
	QUEST_GAME_NEARBY_INFO	tNearbyInfo;
	s_QuestGameNearbyInfo( monster, &tNearbyInfo );

	if (UnitHasState(pGame, monster, STATE_OPERATE_OBJECT_DELAY_MONSTER))
	{
		return;
	}

	// am i near the node to activate it?
	if ( tNearbyInfo.nNumNearbyObjects > 0 )
	{
		for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
		{
			if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nNode )
			{
				// check if it is a node that I should try and use
				if ( sNodeTeam( pQuest, tNearbyInfo.pNearbyObjects[i] ) != TOF_TEAM_DEMON )
				{
					// find distance
					if ( s_QuestGameInOperateDistance( monster, tNearbyInfo.pNearbyObjects[i] ) )
					{
						if (sDemonStartGrabbingNode( pQuest, monster, tNearbyInfo.pNearbyObjects[i] ))
						{
							s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );
							return;
						}
					}
				}
			}
		}
	}
	else
	{
		// stop tracking if nothing nearby
		s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );
	}

	// attack nearby players
	if ( tNearbyInfo.nNumNearbyPlayers > 0 )
	{
		// attack first player
		UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[0] ) );
		s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
		return;
	}

	// node nearby?
	for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
	{
		if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nNode )
		{
			// node already owned by us?
			if ( sNodeTeam( pQuest, tNearbyInfo.pNearbyObjects[i] ) != TOF_TEAM_DEMON )
			{
				// path to the node!
				UnitSetStat( monster, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( tNearbyInfo.pNearbyObjects[i] ) );
				s_StateSet( monster, monster, STATE_UBER_IMP_RUN_TO, 0 );
				return;
			}
		}
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
			sSpawnNodes( pQuest );
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
			sUpdateScore( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_LEAVE:
		{
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
			s_QuestGameUIMessage( pQuest, QUIM_GLOBAL_STRING_SCORE, GS_INVALID, 0, 0, UISMS_FORCEOFF );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_VICTORY:
		{
			if ( QuestIsActive( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_TESTFELLOWSHIP_PASS, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TESTFELLOWSHIP_SIGIL, QUEST_STATE_ACTIVE );
			}
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOF_VICTORY );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "VictorySpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_FAILED:
		{
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOF_FAIL );
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
		// custom Test of Fellowship only messages
		//----------------------------------------------------------------------------

		case QGM_CUSTOM_TOF_CHANGE_NODE:
		{
			QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
			UNITID idUnit = UnitGetId( pMessage->pUnit );
			for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
			{
				if ( idUnit == pGameData->idNodes[i] )
				{
					pGameData->nNodeTeam[i] = pMessage->dwData;
				}
			}
			sUpdateRadar( pQuest );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------

		case QGM_CUSTOM_TOF_PLAYER_SCORE:
		{
			QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
			pGameData->nPlayerScore++;
			ASSERT( pGameData->nPlayerScore == (int)pMessage->dwData );
			sUpdateScore( pQuest );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------

		case QGM_CUSTOM_TOF_DEMON_SCORE:
		{
			QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
			pGameData->nDemonScore++;
			ASSERT( pGameData->nDemonScore == (int)pMessage->dwData );
			sUpdateScore( pQuest );
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
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTruthNPC ))
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelTestOfFellowship )
			return QMR_OK;

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOF_RULES );
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
		case DIALOG_TOF_RULES:
		{
			s_QuestToFBegin( pQuest );
			s_QuestGameJoin( pQuest );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPlayerChangingNode( QUEST * pQuest, UNITID idUnit )
{
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );

	// node may have switched by someone else
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( idUnit == pGameData->idNodes[i] )
		{
			// if it isn't the starting state, abort!
			if ( pQuestData->nOperatingNodeState != pGameData->nNodeTeam[i] )
				return;

			UNIT * pNode = UnitGetById( UnitGetGame( pQuest->pPlayer ), idUnit );

			if ( pGameData->nNodeTeam[i] == TOF_TEAM_DEMON )
			{
				pGameData->nNodeTeam[i] = TOF_TEAM_NEUTRAL;
				s_StateClear(pNode, idUnit, STATE_QUEST_GAME_NODE_DEMON, 0);
				UnitUnregisterTimedEvent( pNode, sAddPlayerScore );
				UnitUnregisterTimedEvent( pNode, sAddDemonScore );
			}
			else if ( pGameData->nNodeTeam[i] == TOF_TEAM_NEUTRAL )
			{
				pGameData->nNodeTeam[i] = TOF_TEAM_PLAYER;
				s_StateSet(pNode, pNode, STATE_QUEST_GAME_NODE_PLAYER, 0);
				UnitRegisterEventTimed( pNode, sAddPlayerScore, &EVENT_DATA( pQuest->nQuest ), TOF_SCORE_UPDATE_DELAY );
			}
			else
			{
				// clicked on player?
				return;
			}

			s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOF_CHANGE_NODE, pNode, pGameData->nNodeTeam[i] );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sNodeActivate( QUEST * pQuest, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTFELLOWSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find which node first...
	for ( int i = 0; i < NUM_TESTFELLOWSHIP_NODES; i++ )
	{
		if ( UnitGetId( pNode ) == pGameData->idNodes[i] )
		{
			// find out state
			if ( pGameData->nNodeTeam[i] != TOF_TEAM_PLAYER )
			{
				QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
				pQuestData->nOperatingNodeState = pGameData->nNodeTeam[i];
				s_QuestOperateDelay( pQuest, sPlayerChangingNode, UnitGetId( pNode ), TOF_PLAYER_OPERATE_DELAY );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nNode )
	{
		sNodeActivate( pQuest, pTarget );
		return QMR_FINISHED;
	}

	if ( nTargetClass == pQuestData->nSigil )
	{
		if ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelTestOfFellowship )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTFELLOWSHIP_SIGIL, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			QUEST * pTest = QuestGetQuest( pQuest->pPlayer, QUEST_TESTHOPE );
			s_QuestToHBegin( pTest );
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

	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	if ( pLevel->m_nLevelDef == pQuestData->nLevelTestOfFellowship )
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
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelTestOfFellowship )
		return QMR_OK;

	int nMonsterClass = UnitGetClass( pVictim );

	// if the uber imp died, respawn...
	if ( nMonsterClass == pQuestData->nImpUber )
	{
		if ( !UnitHasTimedEvent( pVictim, s_QuestGameRespawnMonster ) )
		{
			UNITID idVictim = UnitGetId( pVictim );
			s_StateClear( pVictim, idVictim, STATE_UBER_IMP_DEFEND, 0 );
			s_StateClear( pVictim, idVictim, STATE_UBER_IMP_ATTACK, 0 );
			s_StateClear( pVictim, idVictim, STATE_UBER_IMP_RUN_TO, 0 );
			UnitRegisterEventTimed( pVictim, s_QuestGameRespawnMonster, &EVENT_DATA(), TOF_MONSTER_SPAWN_DELAY );
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

		//----------------------------------------------------------------------------
		case QM_SERVER_RELOAD_UI:
		{
			if (pQuest->pPlayer && 
				IS_SERVER(UnitGetGame( pQuest->pPlayer )) &&
				(QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE || QuestGetStatus( pQuest ) == QUEST_STATUS_COMPLETE))
			{														// ^^^ cause you can do it multiple times
				QUEST_DATA_TESTFELLOWSHIP * pQuestData = sQuestDataGet( pQuest );
				if (UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelTestOfFellowship)
				{
					sUpdateRadar(pQuest);
				}
			}
			return QMR_OK;
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTestFellowship(
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
	QUEST_DATA_TESTFELLOWSHIP* pQuestData)
{
	pQuestData->nTruthNPC				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_D_SAGE_NEW );
	pQuestData->nLevelTestOfFellowship	= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_FELLOWSHIP );
	pQuestData->nImpUber				= QuestUseGI( pQuest, GI_MONSTER_IMP_UBER );
	pQuestData->nSigil					= QuestUseGI( pQuest, GI_OBJECT_SIGIL );
	pQuestData->nNode					= QuestUseGI( pQuest, GI_OBJECT_TESTFELLOWSHIP_NODE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestFellowship(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTFELLOWSHIP * pQuestData = ( QUEST_DATA_TESTFELLOWSHIP * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TESTFELLOWSHIP ) );
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
void s_QuestRestoreTestFellowship(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestToFBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTFELLOWSHIP, "Wrong quest sent to function. Need Test of Fellowship" );

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_ACTIVE )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
}
