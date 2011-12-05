//----------------------------------------------------------------------------
// FILE: quest_testleadership.cpp
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
#include "quest_testleadership.h"
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
#include "quest_testbeauty.h"

//----------------------------------------------------------------------------
// Enums, defines
//----------------------------------------------------------------------------

#define TOL_ENEMY_LEVEL					28
#define NUM_TESTLEADERSHIP_ENEMIES		6

//----------------------------------------------------------------------------

enum TOL_NODE_INDEX
{
	QUEST_TOL_NODE_UL = 0,
	QUEST_TOL_NODE_UR,
	QUEST_TOL_NODE_LL,
	QUEST_TOL_NODE_LR,

	QUEST_TOL_NUM_NODES
};

//----------------------------------------------------------------------------

enum TOL_TEAMS
{
	TOL_TEAM_NEUTRAL = 0,
	TOL_TEAM_PLAYER,
	TOL_TEAM_DEMON
};

//----------------------------------------------------------------------------

enum TOL_SPAWN_MODES
{
	TOL_SPAWN_MODE_GUARD = 0,
	TOL_SPAWN_MODE_FOLLOW,
};

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------

struct QUEST_TOL_NODE
{
	int		nTeamState;
	UNITID	idSpawn1;
	UNITID	idSpawn2;
	int		nSpawnMode;
	UNITID	idCommander;
};

//----------------------------------------------------------------------------

struct QUEST_GAME_DATA_TESTLEADERSHIP
{
	UNITID			idNodes[ QUEST_TOL_NUM_NODES ];
	QUEST_TOL_NODE	tNodeInfo[ QUEST_TOL_NUM_NODES ];
	UNITID			idImps[ NUM_TESTLEADERSHIP_ENEMIES ];
	UNITID			idImpColonel1;
	UNITID			idImpColonel2;
	UNITID			idImpGeneral;
	UNITID			idToLColonel1;
	UNITID			idToLColonel2;
	UNITID			idToLGeneral;
};

//----------------------------------------------------------------------------

struct QUEST_DATA_TESTLEADERSHIP
{
	int		nTruthNPC;

	int		nNode;
	int		nSigil;

	int		nLevelTestOfLeadership;

	int		nImpTroop;
	int		nImpUber;
	int		nImpColonel1;
	int		nImpColonel2;
	int		nImpGeneral;

	int		nToLTroop;
	int		nToLColonel1;
	int		nToLColonel2;
	int		nToLGeneral;

	int		nAI_Follow;
	int		nAI_Defend;

	int		nOperatingNodeState;		// used to hold the current state of the node you are operating when you started to click it

	QUEST_GAME_DATA_TESTLEADERSHIP	tGameData;
};

//----------------------------------------------------------------------------
// Static Data
//----------------------------------------------------------------------------

static char * sgszToLLayoutLabels[ QUEST_TOL_NUM_NODES ] =
{
	"node_ul",
	"node_ur",
	"node_ll",
	"node_lr",
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TESTLEADERSHIP * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TESTLEADERSHIP *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_GAME_DATA_TESTLEADERSHIP * sQuestGameDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_TESTLEADERSHIP * pQuestData = (QUEST_DATA_TESTLEADERSHIP *)pQuest->pQuestData;
	return (QUEST_GAME_DATA_TESTLEADERSHIP *)&pQuestData->tGameData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sClearAllBuffs( QUEST * pQuest, UNIT * pUnit )
{
	ASSERT_RETURN( pQuest );
	ASSERT_RETURN( pUnit );
	UNITID idUnit = UnitGetId( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_SPEED ) )
		s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_SPEED, 0 );
	if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_HITPOINTS ) )
		s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_HITPOINTS, 0 );
	if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_COOLDOWN ) )
		s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_COOLDOWN, 0 );
	if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_SHIELDS ) )
		s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_SHIELDS, 0 );
	if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_DAMAGE ) )
		s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_DAMAGE, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUpdateBuffs( QUEST * pQuest, UNITID idUnit, TOL_TEAMS eTeam )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	ASSERT_RETURN( idUnit != INVALID_ID );
	UNIT * pUnit = UnitGetById( pGame, idUnit );
	ASSERT_RETURN( pUnit );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );

	int nCount = 0;
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( pGameData->tNodeInfo[i].nTeamState == eTeam )
			nCount++;
	}

	if ( nCount >= 1 )
	{
		s_StateSet( pUnit, pUnit, STATE_QUEST_TOL_SPEED, 0 );
	}
	else
	{
		if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_SPEED ) )
			s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_SPEED, 0 );
	}
	if ( nCount >= 2 )
	{
		s_StateSet( pUnit, pUnit, STATE_QUEST_TOL_HITPOINTS, 0 );
	}
	else
	{
		if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_HITPOINTS ) )
			s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_HITPOINTS, 0 );
	}
	if ( nCount >= 3 )
	{
		s_StateSet( pUnit, pUnit, STATE_QUEST_TOL_SHIELDS, 0 );
	}
	else
	{
		if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_SHIELDS ) )
			s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_SHIELDS, 0 );
	}
	if ( nCount >= 4 )
	{
		s_StateSet( pUnit, pUnit, STATE_QUEST_TOL_DAMAGE, 0 );
	}
	else
	{
		if ( UnitHasState( pGame, pUnit, STATE_QUEST_TOL_DAMAGE ) )
			s_StateClear( pUnit, idUnit, STATE_QUEST_TOL_DAMAGE, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitNodeInfo( QUEST * pQuest, TOL_NODE_INDEX nIndex )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	pGameData->idNodes[nIndex] = INVALID_ID;
	pGameData->tNodeInfo[nIndex].idSpawn1 = INVALID_ID;
	pGameData->tNodeInfo[nIndex].idSpawn2 = INVALID_ID;
	pGameData->tNodeInfo[nIndex].nTeamState = TOL_TEAM_NEUTRAL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitializeGameData( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		pGameData->idNodes[i] = INVALID_ID;
		sInitNodeInfo( pQuest, (TOL_NODE_INDEX)i );
	}
	for ( int e = 0; e < NUM_TESTLEADERSHIP_ENEMIES; e++ )
	{
		pGameData->idImps[e] = INVALID_ID;
	}
	pGameData->idImpColonel1 = INVALID_ID;
	pGameData->idImpColonel2 = INVALID_ID;
	pGameData->idImpGeneral = INVALID_ID;
	pGameData->idToLColonel1 = INVALID_ID;
	pGameData->idToLColonel2 = INVALID_ID;
	pGameData->idToLGeneral = INVALID_ID;
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

	QUEST_DATA_TESTLEADERSHIP * pToQuestData = sQuestDataGet( pQuest );
	QUEST_DATA_TESTLEADERSHIP * pFromQuestData = sQuestDataGet( pFromQuest );

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
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( pGameData->idNodes[i] != INVALID_ID )
		{
			// delete node ( could be no troops )
			sFreeUnitById( pGame, pGameData->tNodeInfo[i].idSpawn1 );
			sFreeUnitById( pGame, pGameData->tNodeInfo[i].idSpawn2 );
			// delete node
			sFreeUnitById( pGame, pGameData->idNodes[i] );
		}
	}

	for ( int e = 0; e < NUM_TESTLEADERSHIP_ENEMIES; e++ )
	{
		sFreeUnitById( pGame, pGameData->idImps[e] );
	}
	sFreeUnitById( pGame, pGameData->idImpColonel1 );
	sFreeUnitById( pGame, pGameData->idImpColonel2 );
	sFreeUnitById( pGame, pGameData->idImpGeneral );
	sFreeUnitById( pGame, pGameData->idToLColonel1 );
	sFreeUnitById( pGame, pGameData->idToLColonel2 );
	sFreeUnitById( pGame, pGameData->idToLGeneral );
	sInitializeGameData( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UNITID sSpawnProtector( QUEST * pQuest, TOL_NODE_INDEX nIndex )
{
	ASSERT_RETVAL( pQuest, INVALID_ID );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), INVALID_ID, "Server only" );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETVAL( pGameData, INVALID_ID );

	// neutral don't get protectors
	if ( pGameData->tNodeInfo[nIndex].nTeamState == TOL_TEAM_NEUTRAL )
		return INVALID_ID;

	ASSERT_RETVAL( pGameData->idNodes[nIndex] != INVALID_ID, INVALID_ID );
	UNIT * pNode = UnitGetById( pGame, pGameData->idNodes[nIndex] );
	ROOM * pDestRoom;
	VECTOR vDirection;
	float fAngle = RandGetFloat( pGame->rand, TWOxPI );
	vDirection.fX = cosf( fAngle );
	vDirection.fY = sinf( fAngle );
	vDirection.fZ = 0.0f;
	VectorNormalize( vDirection );
	VECTOR vDelta = vDirection * RandGetFloat( pGame->rand, 1.5f, 3.0f );
	VECTOR vDestination = pNode->vPosition + vDelta;	
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, UnitGetRoom( pNode ), vDestination, &pDestRoom );
	ASSERT_RETVAL( pPathNode, INVALID_ID );

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	MONSTER_SPEC tSpec;
	if ( pGameData->tNodeInfo[nIndex].nTeamState == TOL_TEAM_DEMON )
		tSpec.nClass = pQuestData->nImpTroop;
	else
		tSpec.nClass = pQuestData->nToLTroop;

	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pDestRoom ), tSpec.nClass, TOL_ENEMY_LEVEL );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = vDirection;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * protector = s_MonsterSpawn( pGame, tSpec );
	ASSERT( protector );
	return UnitGetId( protector );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnNodeProtectors( QUEST * pQuest, TOL_NODE_INDEX nIndex, BOOL bSend )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );

	ASSERT_RETURN( pGameData->tNodeInfo[nIndex].idSpawn1 == INVALID_ID );
	ASSERT_RETURN( pGameData->tNodeInfo[nIndex].idSpawn2 == INVALID_ID );

	UNITID idNewProtector = sSpawnProtector( pQuest, nIndex );
	if ( bSend )
	{
		UNIT * protector = UnitGetById( UnitGetGame( pQuest->pPlayer ), idNewProtector );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_ADD_PROTECTOR, protector, ( nIndex << 8 ) + 0 );
	}
	else
	{
		pGameData->tNodeInfo[nIndex].idSpawn1 = idNewProtector;
	}
	idNewProtector = sSpawnProtector( pQuest, nIndex );
	if ( bSend )
	{
		UNIT * protector = UnitGetById( UnitGetGame( pQuest->pPlayer ), idNewProtector );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_ADD_PROTECTOR, protector, ( nIndex << 8 ) + 1 );
	}
	else
	{
		pGameData->tNodeInfo[nIndex].idSpawn2 = idNewProtector;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnNodes( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		sInitNodeInfo( pQuest, (TOL_NODE_INDEX)i );

		QUEST_GAME_LABEL_NODE_INFO tInfo;
		BOOL bRetVal = s_QuestGameFindLayoutLabel( UnitGetLevel( pPlayer ), sgszToLLayoutLabels[i], &tInfo );
		ASSERT_RETURN( bRetVal );

		QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
		OBJECT_SPEC tSpec;
		tSpec.nClass = pQuestData->nNode;
		tSpec.pRoom = tInfo.pRoom;
		tSpec.vPosition = tInfo.vPosition;
		tSpec.pvFaceDirection = &tInfo.vDirection;
		UNIT * node = s_ObjectSpawn( UnitGetGame( pPlayer ), tSpec );
		ASSERT_RETURN( node );
		pQuestData->tGameData.idNodes[i] = UnitGetId( node );
		switch ( i )
		{
			case QUEST_TOL_NODE_UL:
			case QUEST_TOL_NODE_UR:
				pQuestData->tGameData.tNodeInfo[i].nTeamState = TOL_TEAM_DEMON;
				break;
			case QUEST_TOL_NODE_LL:
			case QUEST_TOL_NODE_LR:
				pQuestData->tGameData.tNodeInfo[i].nTeamState = TOL_TEAM_PLAYER;
				break;
			default:
				pQuestData->tGameData.tNodeInfo[i].nTeamState = TOL_TEAM_NEUTRAL;
				break;
		}
		sSpawnNodeProtectors( pQuest, (TOL_NODE_INDEX)i, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnInitialEnemies( QUEST * pQuest )
{
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	int nTOLEnemyLevel = QuestGetMonsterLevel( pQuest, UnitGetLevel( pQuest->pPlayer ), pQuestData->nImpUber, TOL_ENEMY_LEVEL );

	for ( int e = 0; e < NUM_TESTLEADERSHIP_ENEMIES; e++ )
	{
		pGameData->idImps[e] = s_QuestGameSpawnEnemy( pQuest, "MonsterSpawn", pQuestData->nImpUber, nTOLEnemyLevel );
		if ( pGameData->idImps[e] != INVALID_ID )
		{
			GAME * pGame = UnitGetGame( pQuest->pPlayer );
			UNIT * imp = UnitGetById( pGame, pGameData->idImps[e] );
			UnitSetStat( imp, STATS_AI_ACTIVE_QUEST_ID, pQuest->nQuest );
		}
	}
	pGameData->idImpColonel1 = s_QuestGameSpawnEnemy( pQuest, "imp_colonel_1", pQuestData->nImpColonel1, nTOLEnemyLevel );
	pGameData->idImpColonel2 = s_QuestGameSpawnEnemy( pQuest, "imp_colonel_2", pQuestData->nImpColonel2, nTOLEnemyLevel );
	pGameData->idImpGeneral = s_QuestGameSpawnEnemy( pQuest, "imp_general", pQuestData->nImpGeneral, nTOLEnemyLevel );
	pGameData->idToLColonel1 = s_QuestGameSpawnEnemy( pQuest, "tol_colonel_1", pQuestData->nToLColonel1, nTOLEnemyLevel );
	pGameData->idToLColonel2 = s_QuestGameSpawnEnemy( pQuest, "tol_colonel_2", pQuestData->nToLColonel2, nTOLEnemyLevel );
	pGameData->idToLGeneral = s_QuestGameSpawnEnemy( pQuest, "tol_general", pQuestData->nToLGeneral, nTOLEnemyLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUpdateRadar( QUEST * pQuest )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		ASSERT_RETURN( pGameData->idNodes[i] != INVALID_ID );
		UNIT * pNode = UnitGetById( UnitGetGame( pQuest->pPlayer ), pGameData->idNodes[i] );
		ASSERT_RETURN( pNode );

		DWORD dwColor = GFXCOLOR_WHITE;
		switch ( pGameData->tNodeInfo[i].nTeamState )
		{
			case TOL_TEAM_NEUTRAL:
				dwColor = GFXCOLOR_GRAY;
				break;
			case TOL_TEAM_DEMON:
				dwColor = GFXCOLOR_RED;
				break;
			case TOL_TEAM_PLAYER:
				dwColor = GFXCOLOR_BLUE;
				break;
		}
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, pNode, 0, dwColor );
	}
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_ON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sProectorChangeAI( QUEST * pQuest, UNIT * pProtector, int nAi )
{
	ASSERT_RETURN( pProtector );
	GAME * pGame = UnitGetGame( pProtector );

	// change the AI on protectors
	if ( UnitGetStat( pProtector, STATS_AI ) != nAi )
	{
		AI_Free( pGame, pProtector );
		UnitSetStat( pProtector, STATS_AI, nAi );
		AI_Init( pGame, pProtector );

		QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
		if ( nAi == pQuestData->nAI_Follow )
		{
			UnitSetStat( pProtector, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pQuest->pPlayer ) );
		}
		else
		{
			UnitSetStat( pProtector, STATS_AI_TARGET_OVERRIDE_ID, INVALID_ID );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sFreeProtectors( QUEST * pQuest, TOL_NODE_INDEX nIndex )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	ASSERT_RETURN( pGameData );

	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	if ( pGameData->tNodeInfo[nIndex].idSpawn1 != INVALID_ID )
	{
		UNITID idHold = pGameData->tNodeInfo[nIndex].idSpawn1;
		UNIT * pProtector = UnitGetById( pGame, idHold );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_REMOVE_PROTECTOR, pProtector, (DWORD)nIndex );
		sFreeUnitById( pGame, idHold );
	}
	if ( pGameData->tNodeInfo[nIndex].idSpawn2 != INVALID_ID )
	{
		UNITID idHold = pGameData->tNodeInfo[nIndex].idSpawn2;
		UNIT * pProtector = UnitGetById( pGame, idHold );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_REMOVE_PROTECTOR, pProtector, (DWORD)nIndex );
		sFreeUnitById( pGame, idHold );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sGrabNode( QUEST * pQuest, TOL_NODE_INDEX eIndex, TOL_TEAMS eMyTeam )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	ASSERT_RETURN( pGameData->tNodeInfo[eIndex].nTeamState != eMyTeam );

	// neutral to mine!
	if ( pGameData->tNodeInfo[eIndex].nTeamState == TOL_TEAM_NEUTRAL )
	{
		pGameData->tNodeInfo[eIndex].nTeamState = (int)eMyTeam;
		sSpawnNodeProtectors( pQuest, eIndex, TRUE );
	}
	else
	{
		sFreeProtectors( pQuest, eIndex );
		pGameData->tNodeInfo[eIndex].nTeamState = TOL_TEAM_NEUTRAL;
	}

	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	UNIT * pNode = UnitGetById( pGame, pGameData->idNodes[eIndex] );
	ASSERT_RETURN( pNode );
	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_CHANGE_NODE, pNode, pGameData->tNodeInfo[eIndex].nTeamState );

	switch ( pGameData->tNodeInfo[eIndex].nTeamState )
	{
		case TOL_TEAM_NEUTRAL:
		{
			if ( UnitHasState( pGame, pNode, STATE_QUEST_GAME_NODE_PLAYER ) )
				s_StateClear( pNode, pGameData->idNodes[eIndex], STATE_QUEST_GAME_NODE_PLAYER, 0 );
			if ( UnitHasState( pGame, pNode, STATE_QUEST_GAME_NODE_DEMON ) )
				s_StateClear( pNode, pGameData->idNodes[eIndex], STATE_QUEST_GAME_NODE_DEMON, 0 );
			break;
		}
		case TOL_TEAM_PLAYER:
		{
			s_StateSet( pNode, pNode, STATE_QUEST_GAME_NODE_PLAYER, 0 );
			break;
		}
		case TOL_TEAM_DEMON:
		{
			s_StateSet( pNode, pNode, STATE_QUEST_GAME_NODE_DEMON, 0 );
			break;
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static TOL_TEAMS sNodeTeam( QUEST * pQuest, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find which node first...
	UNITID idNode = UnitGetId( pNode );
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( idNode == pGameData->idNodes[i] )
		{
			return (TOL_TEAMS)pGameData->tNodeInfo[i].nTeamState;
		}
	}
	ASSERTX( 0, "Could not find node in test of leadership" );
	return TOL_TEAM_NEUTRAL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sMonsterGrabNode( QUEST * pQuest, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find node in game data...
	UNITID idNode = UnitGetId( pNode );
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( idNode == pGameData->idNodes[i] )
		{
			sGrabNode( pQuest, (TOL_NODE_INDEX)i, TOL_TEAM_DEMON );
			return;
		}
	}
	ASSERTX( 0, "Could not find node in test of leadership" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAIUpdate( QUEST * pQuest, UNIT * monster )
{
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( monster );
	ASSERTX_RETURN( UnitGetClass( monster ) == pQuestData->nImpUber, "Test of Leadership Strategy is Imp_uber only" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( IsUnitDeadOrDying( monster ) )
		return;

	// clear previous flags
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_ATTACK, 0 );

	// find objects and players around me
	QUEST_GAME_NEARBY_INFO	tNearbyInfo;
	s_QuestGameNearbyInfo( monster, &tNearbyInfo, TRUE );

	// am i near the node to activate it?
	if ( tNearbyInfo.nNumNearbyObjects > 0 )
	{
		for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
		{
			if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nNode )
			{
				// check if it is a node that I should try and use
				if ( sNodeTeam( pQuest, tNearbyInfo.pNearbyObjects[i] ) != TOL_TEAM_DEMON )
				{
					// find distance
					if ( s_QuestGameInOperateDistance( monster, tNearbyInfo.pNearbyObjects[i] ) )
					{
						// grab it!
						sMonsterGrabNode( pQuest, tNearbyInfo.pNearbyObjects[i] );
						s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );
						return;
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

	// attack player minions/colonels/general
	if ( tNearbyInfo.nNumNearbyEnemies > 0 )
	{
		// need to check class here and determine if general, colonel, etc...
		int pri = -1;
		int index = -1;
		for ( int i = 0; i < tNearbyInfo.nNumNearbyEnemies; i++ )
		{
			int curr_pri = 0;
			int nClass = UnitGetClass( tNearbyInfo.pNearbyEnemies[i] );
			if ( ( nClass == pQuestData->nToLColonel1 ) || ( nClass == pQuestData->nToLColonel2 ) )
			{
				curr_pri = 1;
			}
			if ( ( nClass == pQuestData->nToLGeneral ) && ( !UnitHasState( pGame, tNearbyInfo.pNearbyEnemies[i], STATE_QUEST_TOL_INVULNERABLE ) ) )
			{
				curr_pri = 2;
			}
			if ( curr_pri > pri )
			{
				pri = curr_pri;
				index = i;
			}
		}
		if ( index >= 0 )
		{
			UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyEnemies[index] ) );
			s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
			return;
		}
	}

	// node nearby to path to?
	for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
	{
		if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nNode )
		{
			// node already owned by us?
			if ( sNodeTeam( pQuest, tNearbyInfo.pNearbyObjects[i] ) != TOL_TEAM_DEMON )
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
			sUpdateBuffs( pQuest, UnitGetId( pQuest->pPlayer ), TOL_TEAM_PLAYER );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_LEAVE:
		{
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
			sClearAllBuffs( pQuest, pQuest->pPlayer );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_VICTORY:
		{
			if ( QuestIsActive( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_TESTLEADERSHIP_PASS, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TESTLEADERSHIP_SIGIL, QUEST_STATE_ACTIVE );
			}
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOL_VICTORY );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "VictorySpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_FAILED:
		{
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOL_FAIL );
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
		// custom Test of Leadership only messages
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOL_ADD_PROTECTOR:
		{
			QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
			ASSERT_RETVAL( pGameData, QMR_OK );
			UNITID idProtector = UnitGetId( pMessage->pUnit );
			int nIndex = pMessage->dwData >> 8;
			if ( ( pMessage->dwData & 0xff ) == 0 )
			{
				ASSERT_RETVAL( pGameData->tNodeInfo[nIndex].idSpawn1 == INVALID_ID, QMR_OK );
				pGameData->tNodeInfo[nIndex].idSpawn1 = idProtector;
			}
			else
			{
				ASSERT_RETVAL( pGameData->tNodeInfo[nIndex].idSpawn2 == INVALID_ID, QMR_OK );
				pGameData->tNodeInfo[nIndex].idSpawn2 = idProtector;
			}
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOL_REMOVE_PROTECTOR:
		{
			QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
			ASSERT_RETVAL( pGameData, QMR_OK );
			UNITID idProtector = UnitGetId( pMessage->pUnit );
			int nIndex = pMessage->dwData;
			if ( pGameData->tNodeInfo[nIndex].idSpawn1 == idProtector )
			{
				pGameData->tNodeInfo[nIndex].idSpawn1 = INVALID_ID;
			}
			else
			{
				ASSERT_RETVAL( pGameData->tNodeInfo[nIndex].idSpawn2 == idProtector, QMR_OK );
				pGameData->tNodeInfo[nIndex].idSpawn2 = INVALID_ID;
			}
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOL_CHANGE_NODE:
		{
			QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
			UNITID idUnit = UnitGetId( pMessage->pUnit );
			for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
			{
				if ( idUnit == pGameData->idNodes[i] )
				{
					pGameData->tNodeInfo[i].nTeamState = pMessage->dwData;
					if ( pGameData->tNodeInfo[i].nTeamState == TOL_TEAM_PLAYER )
					{
						s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOL_TEAM_NODE );
					}
					else if ( pGameData->tNodeInfo[i].nTeamState == TOL_TEAM_DEMON )
					{
						s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOL_DEMON_NODE );
					}
				}
			}
			sUpdateRadar( pQuest );
			sUpdateBuffs( pQuest, UnitGetId( pQuest->pPlayer ), TOL_TEAM_PLAYER );
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
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );

	if (UnitIsMonsterClass( pSubject, pQuestData->nTruthNPC ))
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelTestOfLeadership )
			return QMR_OK;

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOL_RULES );		
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
		case DIALOG_TOL_RULES:
		{
			s_QuestToLBegin( pQuest );
			s_QuestGameJoin( pQuest );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangingNode( QUEST * pQuest, UNITID idUnit )
{
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	// node may have switched by someone else
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( idUnit == pGameData->idNodes[i] )
		{
			// if it isn't the starting state, abort!
			if ( pQuestData->nOperatingNodeState != pGameData->tNodeInfo[i].nTeamState )
				return;

			sGrabNode( pQuest, (TOL_NODE_INDEX)i, TOL_TEAM_PLAYER );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sProtectorsFollow( QUEST * pQuest, int nNodeIndex )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	BOOL bFollowOk = FALSE;
	if ( pGameData->tNodeInfo[nNodeIndex].idSpawn1 != INVALID_ID )
	{
		UNIT * pProtector = UnitGetById( pGame, pGameData->tNodeInfo[nNodeIndex].idSpawn1 );
		sProectorChangeAI( pQuest, pProtector, pQuestData->nAI_Follow );
		bFollowOk = TRUE;
	}

	if ( pGameData->tNodeInfo[nNodeIndex].idSpawn2 != INVALID_ID )
	{
		UNIT * pProtector = UnitGetById( pGame, pGameData->tNodeInfo[nNodeIndex].idSpawn2 );
		sProectorChangeAI( pQuest, pProtector, pQuestData->nAI_Follow );
		bFollowOk = TRUE;
	}

	if ( bFollowOk )
	{
		s_NPCEmoteStart( pGame, pPlayer, DIALOG_TOL_FOLLOW_EMOTE );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_PROTECTOR_FOLLOW, pPlayer, nNodeIndex );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sProtectorsDefend( QUEST * pQuest, int nNodeIndex )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	BOOL bDefendOk = FALSE;
	if ( pGameData->tNodeInfo[nNodeIndex].idSpawn1 != INVALID_ID )
	{
		UNIT * pProtector = UnitGetById( pGame, pGameData->tNodeInfo[nNodeIndex].idSpawn1 );
		sProectorChangeAI( pQuest, pProtector, pQuestData->nAI_Defend );
		bDefendOk = TRUE;
	}

	if ( pGameData->tNodeInfo[nNodeIndex].idSpawn2 != INVALID_ID )
	{
		UNIT * pProtector = UnitGetById( pGame, pGameData->tNodeInfo[nNodeIndex].idSpawn2 );
		sProectorChangeAI( pQuest, pProtector, pQuestData->nAI_Defend );
		bDefendOk = TRUE;
	}

	if ( bDefendOk )
	{
		s_NPCEmoteStart( pGame, pPlayer, DIALOG_TOL_DEFEND_EMOTE );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOL_PROTECTOR_DEFEND, pPlayer, nNodeIndex );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sNodeActivate( QUEST * pQuest, UNIT * pNode )
{
	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );
	// find which node first...
	for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
	{
		if ( UnitGetId( pNode ) == pGameData->idNodes[i] )
		{
			// find out state
			if ( pGameData->tNodeInfo[i].nTeamState == TOL_TEAM_PLAYER )
			{
				if ( pGameData->tNodeInfo[i].nSpawnMode == TOL_SPAWN_MODE_GUARD )
					sProtectorsFollow( pQuest, i );
				else
					sProtectorsDefend( pQuest, i );
			}
			else
			{
				QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
				pQuestData->nOperatingNodeState = pGameData->tNodeInfo[i].nTeamState;
				s_QuestOperateDelay( pQuest, sChangingNode, UnitGetId( pNode ), 5 * GAME_TICKS_PER_SECOND );
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
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nNode )
	{
		sNodeActivate( pQuest, pTarget );
		return QMR_FINISHED;
	}

	if ( nTargetClass == pQuestData->nSigil )
	{
		if ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelTestOfLeadership )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTLEADERSHIP_SIGIL, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			QUEST * pTest = QuestGetQuest( pQuest->pPlayer, QUEST_TESTBEAUTY );
			s_QuestToBBegin( pTest );
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

	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	if ( pLevel->m_nLevelDef == pQuestData->nLevelTestOfLeadership )
	{
		s_QuestGameLeave( pQuest );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckImpGeneralUnlock( QUEST * pQuest, QUEST_GAME_DATA_TESTLEADERSHIP * pGameData )
{
	if ( pGameData->idImpColonel1 != INVALID_ID )
		return;

	if ( pGameData->idImpColonel2 != INVALID_ID )
		return;

	// unlock it!
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	UNIT * pGeneral = UnitGetById( pGame, pGameData->idImpGeneral );
	ASSERT_RETURN( pGeneral );

	if ( UnitHasState( pGame, pGeneral, STATE_QUEST_TOL_INVULNERABLE ) )
	{
		s_StateClear( pGeneral, pGameData->idImpGeneral, STATE_QUEST_TOL_INVULNERABLE, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckToLGeneralUnlock( QUEST * pQuest, QUEST_GAME_DATA_TESTLEADERSHIP * pGameData )
{
	if ( pGameData->idToLColonel1 != INVALID_ID )
		return;

	if ( pGameData->idToLColonel2 != INVALID_ID )
		return;

	// unlock it!
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	UNIT * pGeneral = UnitGetById( pGame, pGameData->idToLGeneral );
	ASSERT_RETURN( pGeneral );

	if ( UnitHasState( pGame, pGeneral, STATE_QUEST_TOL_INVULNERABLE ) )
	{
		s_StateClear( pGeneral, pGameData->idToLGeneral, STATE_QUEST_TOL_INVULNERABLE, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelTestOfLeadership )
		return QMR_OK;

	QUEST_GAME_DATA_TESTLEADERSHIP * pGameData = sQuestGameDataGet( pQuest );

	int nMonsterClass = UnitGetClass( pVictim );
	UNITID idVictim = UnitGetId( pVictim );

	if ( ( nMonsterClass == pQuestData->nImpTroop ) || ( nMonsterClass == pQuestData->nToLTroop ) )
	{
		// remove from quest data
		for ( int i = 0; i < QUEST_TOL_NUM_NODES; i++ )
		{
			if ( pGameData->tNodeInfo[i].idSpawn1 == idVictim )
			{
				pGameData->tNodeInfo[i].idSpawn1 = INVALID_ID;
				return QMR_FINISHED;
			}
			if ( pGameData->tNodeInfo[i].idSpawn2 == idVictim )
			{
				pGameData->tNodeInfo[i].idSpawn2 = INVALID_ID;
				return QMR_FINISHED;
			}
		}
		return QMR_FINISHED;
	}

	// if the uber imp died, respawn...
	if ( nMonsterClass == pQuestData->nImpUber )
	{
		if ( UnitGetLevelDefinitionIndex( pVictim ) == pQuestData->nLevelTestOfLeadership )
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

	if ( nMonsterClass == pQuestData->nImpColonel1 )
	{
		pGameData->idImpColonel1 = INVALID_ID;
		sCheckImpGeneralUnlock( pQuest, pGameData );
		return QMR_FINISHED;
	}

	if ( nMonsterClass == pQuestData->nImpColonel2 )
	{
		pGameData->idImpColonel2 = INVALID_ID;
		sCheckImpGeneralUnlock( pQuest, pGameData );
		return QMR_FINISHED;
	}

	if ( nMonsterClass == pQuestData->nImpGeneral )
	{
		pGameData->idImpGeneral = INVALID_ID;
		s_QuestGameVictory( pQuest );
		return QMR_FINISHED;
	}

	if ( nMonsterClass == pQuestData->nToLColonel1 )
	{
		pGameData->idToLColonel1 = INVALID_ID;
		sCheckToLGeneralUnlock( pQuest, pGameData );
		return QMR_FINISHED;
	}

	if ( nMonsterClass == pQuestData->nToLColonel2 )
	{
		pGameData->idToLColonel2 = INVALID_ID;
		sCheckToLGeneralUnlock( pQuest, pGameData );
		return QMR_FINISHED;
	}

	if ( nMonsterClass == pQuestData->nToLGeneral )
	{
		pGameData->idToLGeneral = INVALID_ID;
		s_QuestGameFailed( pQuest );
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
				QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE)
			{
				QUEST_DATA_TESTLEADERSHIP * pQuestData = sQuestDataGet( pQuest );
				if (UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelTestOfLeadership)
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
void QuestFreeTestLeadership(
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
	QUEST_DATA_TESTLEADERSHIP * pQuestData)
{
	pQuestData->nTruthNPC				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_B_SAGE_NEW );
	pQuestData->nNode					= QuestUseGI( pQuest, GI_OBJECT_TESTLEADERSHIP_NODE );
	pQuestData->nSigil					= QuestUseGI( pQuest, GI_OBJECT_SIGIL );
	pQuestData->nLevelTestOfLeadership	= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_LEADERSHIP );
	pQuestData->nImpTroop				= QuestUseGI( pQuest, GI_MONSTER_IMP_TROOP );
	pQuestData->nImpUber				= QuestUseGI( pQuest, GI_MONSTER_IMP_UBER );
	pQuestData->nImpColonel1			= QuestUseGI( pQuest, GI_MONSTER_IMP_COLONEL_1 );
	pQuestData->nImpColonel2			= QuestUseGI( pQuest, GI_MONSTER_IMP_COLONEL_2 );
	pQuestData->nImpGeneral				= QuestUseGI( pQuest, GI_MONSTER_IMP_GENERAL );
	pQuestData->nToLTroop				= QuestUseGI( pQuest, GI_MONSTER_TOL_TROOP );
	pQuestData->nToLColonel1			= QuestUseGI( pQuest, GI_MONSTER_TOL_COLONEL_1 );
	pQuestData->nToLColonel2			= QuestUseGI( pQuest, GI_MONSTER_TOL_COLONEL_2 );
	pQuestData->nToLGeneral				= QuestUseGI( pQuest, GI_MONSTER_TOL_GENERAL );
	pQuestData->nAI_Follow				= QuestUseGI( pQuest, GI_AI_TEMPLAR_ESCORT );
	pQuestData->nAI_Defend				= QuestUseGI( pQuest, GI_AI_UBER_GUARD_SHORTRANGE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestLeadership(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTLEADERSHIP * pQuestData = ( QUEST_DATA_TESTLEADERSHIP * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TESTLEADERSHIP ) );
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
void s_QuestRestoreTestLeadership(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestToLBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTLEADERSHIP, "Wrong quest sent to function. Need Test of Leadership" );

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_ACTIVE )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
}
