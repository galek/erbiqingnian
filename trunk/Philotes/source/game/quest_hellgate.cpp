//----------------------------------------------------------------------------
// FILE: quest_hellgate.cpp
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
#include "quest_hellgate.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "room_layout.h"
#include "room_path.h"
#include "ai.h"
#include "states.h"
#include "combat.h"
#include "gameclient.h"
#include "picker.h"
#include "astar.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

/*
truth_e_talk_emmera

hellgate_talk_emmera
hellgate_travel
hellgate_enter
hellgate_sieze
hellgate_kill_sydonai

*/

//----------------------------------------------------------------------------

#define NUM_QUEST_HELLGATE_MINI_BOSSES		5

struct QUEST_DATA_HELLGATE
{
	int		nEmmera;
	int		nEmmeraOutside;
	int		nEmmeraFlying;

	int		nFollowAI;

	// five lies
	int		nMiniBoss[NUM_QUEST_HELLGATE_MINI_BOSSES];
	UNITID	idMiniBoss[NUM_QUEST_HELLGATE_MINI_BOSSES];

	// the bad boy
	int		nSydonai;
	BOOL	bSydonaiDead;

	int		nLevelStPaulsHellgate;
	int		nLevelHell;
	int		nLevelStPaulsStation;

	int		nExitPortal;
	int		nWarpHelltoStPauls;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_HELLGATE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_HELLGATE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnEmmeraAtLabel(
	QUEST * pQuest,
	BOOL bFollow )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	// already spawned emmera?
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nEmmeraFlying ) )
		return;

	// spawn at church
	VECTOR vPosition, vDirection, vUp;
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "emmera", &pRoom, &pOrientation );
	ASSERTX( pNode && pRoom && pOrientation, "Couldn't find Emmera spawn location" );

	if ( pNode && pRoom && pOrientation )
	{
		RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );
	}
	else
	{
		pRoom = pPlayer->pRoom;
		vPosition = pPlayer->vPosition;
	}

	ROOM * pDestRoom;
	GAME * pGame = UnitGetGame( pPlayer );
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, pRoom, vPosition, &pDestRoom );

	if ( pPathNode )
	{
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nEmmeraFlying;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 30 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		UNIT * emmera = s_MonsterSpawn( pGame, tSpec );
		ASSERTX_RETURN( emmera, "Unable to spawn Emmera" );
		if ( bFollow && ( UnitGetStat( emmera, STATS_AI ) != pQuestData->nFollowAI ) )
		{
			AI_Free( pGame, emmera );
			UnitSetStat( emmera, STATS_AI, pQuestData->nFollowAI );
			AI_Init( pGame, emmera );
			UnitSetAITarget( emmera, pQuest->pPlayer, TRUE );			
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnSydonai(
	QUEST * pQuest,
	ROOM * pLastLieKilledRoom = NULL)
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );
	GAME * pGame = UnitGetGame( pPlayer );

	TARGET_TYPE eBadTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	if (LevelFindFirstUnitOf( pLevel, eBadTargetTypes, GENUS_MONSTER, pQuestData->nSydonai))
		return;

	// destroy hellgate
	TARGET_TYPE eObjectTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT * pHellgate = LevelFindFirstUnitOf( pLevel, eObjectTargetTypes, GENUS_OBJECT, pQuestData->nWarpHelltoStPauls );
	if ( pHellgate )
	{
		if ( UnitHasState( pGame, pHellgate, STATE_THE_HELLGATE ) )
		{
			s_StateClear( pHellgate, UnitGetId( pHellgate ), STATE_THE_HELLGATE, 0 );
			s_StateSet( pHellgate, pHellgate, STATE_THE_HELLGATE_DESTROY, 0 );
		}
	}

	// spawn near emmera (if she is on the level)
	ROOM * pSpawnRoom = NULL;
	int nNodeIndex = INVALID_ID;
	TARGET_TYPE eGoodTargetTypes[] = { TARGET_DEAD, TARGET_INVALID };
	UNIT * pEmmera = LevelFindFirstUnitOf( pLevel, eGoodTargetTypes, GENUS_MONSTER, pQuestData->nEmmeraFlying);
	if ( pEmmera )
	{
		// spawn in emmera's room
		pSpawnRoom = UnitGetRoom( pEmmera );
	}
	else
	{
		// spawn nearby
		ROOM * pRoom = UnitGetRoom( pPlayer );
		ROOM_LIST tNearbyRooms;
		RoomGetNearbyRoomList(pRoom, &tNearbyRooms);
		if ( tNearbyRooms.nNumRooms )
		{
			PickerStart( pGame, spawnRoomPicker );
			for (int i = 0; i < tNearbyRooms.nNumRooms ; ++i)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( tNearbyRooms.pRooms[i] ), 1));
			}

			while (PickerGetCount( pGame ) > 0 && nNodeIndex < 0)
			{
				pSpawnRoom = RoomGetByID( pGame, PickerChooseRemove( pGame ) );
				nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
			}
		}
		else
		{
			pSpawnRoom = pRoom;
		}
	}

	// and spawn sydonai
	if (nNodeIndex < 0)
	{
		if (pSpawnRoom)
			nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );

		if (nNodeIndex < 0 && pLastLieKilledRoom)
		{
			pSpawnRoom = pLastLieKilledRoom;
			nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
		}

		BOUNDED_WHILE(nNodeIndex < 0, 1000)
		{
			s_LevelGetRandomSpawnRooms(pGame, pLevel, &pSpawnRoom, 1);
			nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
		}

		ASSERTX_RETURN(nNodeIndex >= 0,"FAILED TO SPAWN SYDONAI!");
	}

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nSydonai;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 32 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CLOSEST_EXIT_DISTANCE			3.0f
#define CLOSEST_EXIT_DISTANCE_SQUARED	( CLOSEST_EXIT_DISTANCE * CLOSEST_EXIT_DISTANCE )

static void sSpawnExit( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nExitPortal ) )
		return;

	// spawn near sydonai
	TARGET_TYPE eDeadTargetTypes[] = { TARGET_DEAD, TARGET_INVALID };
	UNIT * pSydonai = LevelFindFirstUnitOf( pLevel, eDeadTargetTypes, GENUS_MONSTER, pQuestData->nSydonai );
	ASSERTX_RETURN( pSydonai, "Couldn't find Sydonai on the level. Couldn't spawn exit" );

	ROOM * pRoom = UnitGetRoom( pSydonai );
	ROOMID idRoom = RoomGetId( pRoom );
	GAME * pGame = UnitGetGame( pSydonai );

	// start picker
	PickerStart( pGame, picker );

	DWORD dwBlockedNodeFlags = 0;
	SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );

	// put all nodes in a picker
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );
	int nValidNodeCount = 0;
	for (int i = 0; i < pRoomPathNodes->nPathNodeCount; ++i)
	{
		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pRoom, i);
		ASSERTX_CONTINUE(pPathNode, "NULL path node for index");

		// don't pick blocked nodes, or nodes that do not have enough free space
		if ( s_RoomNodeIsBlocked( pRoom, i, dwBlockedNodeFlags, NULL ) != NODE_FREE )
			continue;

		// add to picker
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
		nValidNodeCount++;
	}
	ASSERTX_RETURN( nValidNodeCount > 0, "Couldn't find a location to place the exit" );
	
	float fDistanceSquared = 0.0f;
	VECTOR vExitPos;
	int nTries = 0;
	do
	{
		int nNode = PickerChoose( pGame );
		vExitPos = AStarGetWorldPosition( pGame, idRoom, nNode );
		fDistanceSquared = VectorDistanceSquaredXY( pPlayer->vPosition, vExitPos );
		nTries++;
	} while ( ( fDistanceSquared < CLOSEST_EXIT_DISTANCE_SQUARED ) && ( nTries < 10 ) );

	if ( nTries >= 10 )
		vExitPos = pSydonai->vPosition;

	OBJECT_SPEC tSpec;
	tSpec.nClass = pQuestData->nExitPortal;
	tSpec.pRoom = pRoom;
	tSpec.vPosition = vExitPos;
	tSpec.pvFaceDirection = &pSydonai->vFaceDirection;
	s_ObjectSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_QUEST_HELLGATE_SPAWN_ROOMS			32

static void sSpawnFiveLies( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );

	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );
	// find already spawned bosses or sydonai
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// check to see if spawned
		TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_DEAD };
		for ( int t = 0; t < 2; t++ )
		{
			TARGET_TYPE eType = eTargetTypes[t];
			for ( UNIT * test = room->tUnitList.ppFirst[eType]; test; test = test->tRoomNode.pNext )
			{
				if ( UnitGetGenus( test ) != GENUS_MONSTER )
					continue;

				// have we already spawned the end guy?
				int nClass = UnitGetClass( test );
				if ( nClass == pQuestData->nSydonai )
					return;
				for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
				{
					if ( nClass == pQuestData->nMiniBoss[mb] )
					{
						pQuestData->idMiniBoss[mb] = UnitGetId( test );
					}
				}
			}
		}
	}

	ROOM * pRoomList[ MAX_QUEST_HELLGATE_SPAWN_ROOMS ];
	int nNumSpawnRooms = s_LevelGetRandomSpawnRooms( pGame, pLevel, pRoomList, MAX_QUEST_HELLGATE_SPAWN_ROOMS );
	ASSERTX_RETURN( nNumSpawnRooms > 0, "Can't spawn the Five Lies! There are no possible rooms!!" );
	for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
	{
		// spawn each boss we don't have an ID for
		if ( pQuestData->idMiniBoss[mb] == INVALID_ID )
		{
			int index = RandGetNum( pGame->rand, nNumSpawnRooms );
			ASSERT_CONTINUE( index >= 0 && index < nNumSpawnRooms );

			ROOM * pSpawnRoom = pRoomList[index];
			ASSERT_CONTINUE( pSpawnRoom );

			// and spawn lie
			int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
			if (nNodeIndex < 0)
				continue;

			// init location
			SPAWN_LOCATION tLocation;
			SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

			MONSTER_SPEC tSpec;
			tSpec.nClass = pQuestData->nMiniBoss[mb];
			tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 30 ); 
			tSpec.pRoom = pSpawnRoom;
			tSpec.vPosition = tLocation.vSpawnPosition;
			tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
			tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
			UNIT * pMiniBoss = s_MonsterSpawn( pGame, tSpec );
			ASSERTX_CONTINUE( pMiniBoss, "Failed to spawn a mini-boss in hell" );
			pQuestData->idMiniBoss[mb] = UnitGetId( pMiniBoss );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sKillEmmera( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	// Find emmera
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * pEmmera = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nEmmeraFlying);
	ASSERTX_RETURN( pEmmera, "Couldn't find Emmera on the level. Couldn't kill her" );

	// and.. die...
	if ( !IsUnitDeadOrDying( pEmmera ) )
		UnitDie( pEmmera, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	if ( QuestIsComplete( pQuest ) )
		return QMR_OK;

	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( UnitGetGenus( pSubject ) != GENUS_MONSTER )
		return QMR_OK;

	// emmera
	if ( UnitIsMonsterClass( pSubject, pQuestData->nEmmera ) || UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) )
	{
		if ( !QuestIsActive( pQuest ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	if ( QuestIsComplete( pQuest ) )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nEmmera ) || UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) )
	{
		if ( !QuestIsActive( pQuest ) )
		{
			if ( !s_QuestIsPrereqComplete( pQuest ) )
				return QMR_OK;

			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELLGATE_START );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTravelVideoComplete( QUEST * pQuest )
{
	QuestStateSet( pQuest, QUEST_STATE_HELLGATE_TRAVEL, QUEST_STATE_ACTIVE );
	s_QuestMapReveal( pQuest, GI_LEVEL_GUILDHALL );
	s_QuestMapReveal( pQuest, GI_LEVEL_ST_PAULS_APPROACH );
	s_QuestMapReveal( pQuest, GI_LEVEL_ST_PAULS_STATION );
	s_QuestMapReveal( pQuest, GI_LEVEL_ST_PAULS_HELLGATE );
	s_QuestMapReveal( pQuest, GI_LEVEL_HELL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_HELLGATE_START:
		{
			ASSERT_RETVAL( s_QuestIsPrereqComplete( pQuest ), QMR_OK );
			s_QuestActivate( pQuest );
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_TALK_EMMERA, QUEST_STATE_COMPLETE );
			s_NPCVideoStart( UnitGetGame( pQuest->pPlayer ), pQuest->pPlayer, GI_MONSTER_EMMERA, NPC_VIDEO_INSTANT_INCOMING, DIALOG_HELLGATE_TRAVEL );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HELLGATE_TRAVEL:
		{
			sTravelVideoComplete( pQuest );
			break;
		}
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	switch( pMessage->nDialog )
	{
		//----------------------------------------------------------------------------
		case DIALOG_HELLGATE_TRAVEL:
		{
			sTravelVideoComplete( pQuest );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );

	// spawn the lies!
	if ( pMessage->nLevelNewDef == pQuestData->nLevelHell )
	{
		sSpawnFiveLies( pQuest );
	}

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelStPaulsHellgate )
	{
		sSpawnEmmeraAtLabel( pQuest, FALSE );
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_TRAVEL, QUEST_STATE_COMPLETE );
		}
		return QMR_OK;
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelHell )
	{
		sSpawnEmmeraAtLabel( pQuest, TRUE );
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_ENTER ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_ENTER, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_SIEZE, QUEST_STATE_ACTIVE );
			s_NPCVideoStart( game, pQuest->pPlayer, GI_MONSTER_EMMERA, NPC_VIDEO_INCOMING, DIALOG_HELLGATE_KILL );
		}
		return QMR_OK;
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayMovie( QUEST * pQuest, GLOBAL_INDEX eMovieList )
{
	UNIT * pPlayer = pQuest->pPlayer;
	s_StateSet( pPlayer, pPlayer, STATE_MOVIE_GOD, 0 );

	MSG_SCMD_PLAY_MOVIELIST tMessage;
	tMessage.nMovieListIndex = GlobalIndexGet( eMovieList );

	// send it
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_PLAY_MOVIELIST, &tMessage );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckToSpawnSydonai( QUEST * pQuest, UNITID idVictim )
{
	ASSERT_RETFALSE( pQuest );
	ASSERT_RETFALSE( idVictim != INVALID_ID );
	GAME * pGame = QuestGetGame( pQuest );
	UNIT * pVictim = UnitGetById( pGame, idVictim );
	if ( UnitGetGenus( pVictim ) != GENUS_MONSTER )
		return FALSE;

	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );

	// was this the right level?
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelHell )
		return FALSE;

	// did syd die?
	int nVictimClass = UnitGetClass( pVictim );

	// was this a mini-boss that died?
	BOOL bCheck = FALSE;
	for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
	{
		if ( nVictimClass == pQuestData->nMiniBoss[mb] )
			bCheck = TRUE;
	}
	if ( !bCheck )
		return FALSE;

	// run a check to see if all the mini-bosses are dead
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETFALSE( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );

	// init
	BOOL bDead[ NUM_QUEST_HELLGATE_MINI_BOSSES ];
	for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
		bDead[mb] = FALSE;

	// search
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		TARGET_TYPE eTargetTypes[2] = { TARGET_BAD, TARGET_DEAD };
		for ( int t = 0; t < 2; t++ )
		{
			TARGET_TYPE eTT = eTargetTypes[t];
			for ( UNIT * pUnit = pRoom->tUnitList.ppFirst[eTT]; pUnit; pUnit = pUnit->tRoomNode.pNext )
			{
				if ( ( UnitGetGenus( pUnit ) == GENUS_MONSTER ) && IsUnitDeadOrDying( pUnit ) )
				{
					int nClass = UnitGetClass( pUnit );
					for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
					{
						if ( nClass == pQuestData->nMiniBoss[mb] )
							bDead[mb] = TRUE;
					}
				}
			}
		}
	}

	// check results
	BOOL bSpawn = TRUE;
	for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
		bSpawn = bSpawn && bDead[mb];

	return bSpawn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sSyndonaiMovieDelay( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	sPlayMovie( pQuest, GI_MOVIE_HELLGATE3 );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelHell )
		return QMR_OK;

	int nMonsterClass = UnitGetClass( pVictim );

	if ( !QuestIsActive( pQuest ) )
	{
		if ( nMonsterClass == pQuestData->nSydonai )
		{
			sSpawnExit( pQuest );
		}
		else if ( sCheckToSpawnSydonai( pQuest, pMessage->idVictim ) )
		{
			sSpawnSydonai( pQuest, UnitGetRoom( pVictim ) );
		}

		return QMR_OK;
	}

	for ( int mb = 0; mb < NUM_QUEST_HELLGATE_MINI_BOSSES; mb++ )
	{
		if ( nMonsterClass == pQuestData->nMiniBoss[mb] )
		{
			ASSERTX_RETVAL( pQuestData->idMiniBoss[mb] == UnitGetId( pVictim ), QMR_FINISHED, "UNITID of dead mini-boss didn't match what I thought it should" );
			if ( sCheckToSpawnSydonai( pQuest, pMessage->idVictim ) )
			{
				// destroyed the hellgate!!
				QuestStateSet( pQuest, QUEST_STATE_HELLGATE_SIEZE, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_HELLGATE_KILL_SYDONAI, QUEST_STATE_ACTIVE );

				// play the emmera die movie
				sPlayMovie( pQuest, GI_MOVIE_HELLGATE2 );
			}
			return QMR_FINISHED;
		}
	}

	// if sydonai died...
	if ( nMonsterClass == pQuestData->nSydonai )
	{
		// Completed the game!!
		UnitRegisterEventTimed( pQuest->pPlayer, sSyndonaiMovieDelay, &EVENT_DATA( (DWORD_PTR)pQuest ), 3 * GAME_TICKS_PER_SECOND );
		pQuestData->bSydonaiDead = TRUE;
		return QMR_FINISHED;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if ( UnitIsMonsterClass( pTarget, pQuestData->nEmmeraFlying ) )
	{
		if ( ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelStPaulsHellgate ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_ENTER ) == QUEST_STATE_HIDDEN ) )
		{
			UNIT * pPlayer = pQuest->pPlayer;
			s_NPCVideoStart( UnitGetGame( pPlayer ), pPlayer, GI_MONSTER_EMMERA, NPC_VIDEO_INSTANT_INCOMING, DIALOG_HELLGATE_ENTER );
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_ENTER, QUEST_STATE_ACTIVE );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), QMR_OK, "Server only" );

	int nNextLevelIndex = pMessage->nLevelNextDef;

	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	if ( pQuestData->nLevelStPaulsHellgate == nNextLevelIndex )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			int nMovieList = GlobalIndexGet( GI_MOVIE_HELLGATE1 );
			UnitSetStat( pQuest->pPlayer, STATS_MOVIE_TO_PLAY_AFTER_LEVEL_LOAD, nMovieList );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageBackFromMovie(
	QUEST *pQuest,
	const QUEST_MESSAGE_BACK_FROM_MOVIE *pMessage )
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus < QUEST_STATUS_ACTIVE )
		return QMR_OK;

	UNIT * pPlayer = pQuest->pPlayer;

	if ( !UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_MOVIE_GOD ) )
		return QMR_OK;

	// clear my movie god state
	UNITID idPlayer = UnitGetId( pPlayer );
	s_StateClear( pPlayer, idPlayer, STATE_MOVIE_GOD, 0 );

	if ( ( eStatus == QUEST_STATUS_ACTIVE ) && 
		 ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_SIEZE ) == QUEST_STATE_COMPLETE ) &&
		 ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_KILL_SYDONAI ) == QUEST_STATE_ACTIVE ) )
	{
		QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
		if ( !pQuestData->bSydonaiDead )
		{
			// come back from emmera video?
			// kill emmera
			sKillEmmera( pQuest );
			// spawn sydonai
			sSpawnSydonai( pQuest );
		}
		else
		{
			// come back from murmur video?
			sSpawnExit( pQuest );
			QuestStateSet( pQuest, QUEST_STATE_HELLGATE_KILL_SYDONAI, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GLOBAL_INDEX eStPaulsStationNPCs[] =
{
	GI_MONSTER_SPS_ALAY_PENN,
	GI_MONSTER_SPS_NASIM,
	GI_MONSTER_SPS_SER_SING,
	GI_MONSTER_SPS_GUNNY,
	GI_MONSTER_SPS_BRANDON_LANN,
	GI_MONSTER_SPS_LUCIOUS_ALDIN,
	GI_MONSTER_SPS_TECHSMITH_314,
	GI_MONSTER_SPS_LORD_ARPHAUN,
	GI_MONSTER_SPS_RORKE,
	GI_MONSTER_SPS_AERON_ALTAIR,
	GI_MONSTER_SPS_JESSICA_SUMMERISLE,
	GI_INVALID,
};

static QUEST_MESSAGE_RESULT sQuestMessageIsUnitVisible( 
	QUEST *pQuest,
	const QUEST_MESSAGE_IS_UNIT_VISIBLE * pMessage)
{
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pUnit = pMessage->pUnit;

	if ( UnitGetGenus( pUnit ) != GENUS_MONSTER )
		return QMR_OK;

	int nUnitClass = UnitGetClass( pUnit );
	int nLevelIndex = UnitGetLevelDefinitionIndex( pUnit );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( ( nUnitClass == pQuestData->nEmmera ) || ( nUnitClass == pQuestData->nEmmeraOutside ) || ( nUnitClass == pQuestData->nEmmeraFlying ) )
	{
		if ( eStatus >= QUEST_STATUS_COMPLETE )
			return QMR_UNIT_HIDDEN;
		else if ( ( eStatus == QUEST_STATUS_ACTIVE ) && ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_KILL_SYDONAI ) >= QUEST_STATE_ACTIVE ) )
			return QMR_UNIT_HIDDEN;

		return QMR_UNIT_VISIBLE;
	}

	if ( nLevelIndex == pQuestData->nLevelStPaulsStation )
	{
		for ( int index = 0; eStPaulsStationNPCs[index] != GI_INVALID; index++ )
		{
			if ( nUnitClass == GlobalIndexGet( eStPaulsStationNPCs[index] ) )
			{
				if ( eStatus >= QUEST_STATUS_COMPLETE )
					return QMR_UNIT_HIDDEN;
				if ( eStatus < QUEST_STATUS_ACTIVE )
					return QMR_UNIT_HIDDEN;
				if ( ( eStatus == QUEST_STATUS_ACTIVE ) && ( QuestStateGetValue( pQuest, QUEST_STATE_HELLGATE_ENTER ) >= QUEST_STATE_ACTIVE ) )
					return QMR_UNIT_HIDDEN;
				return QMR_UNIT_VISIBLE;
			}
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	QUEST_DATA_HELLGATE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nExitPortal )
	{
		UNIT * pPlayer = pQuest->pPlayer;
		ASSERT_RETVAL( pPlayer, QMR_OK );
		GAME * pGame = UnitGetGame( pPlayer );
		ASSERT_RETVAL( pGame, QMR_OK );
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		ASSERT_RETVAL( idClient != INVALID_ID, QMR_OK );
		GAMECLIENT * pClient = ClientGetById( pGame, idClient );
		ASSERT_RETVAL( pClient, QMR_OK );
		s_GameExit( pGame, pClient, TRUE );
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
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

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
		case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pLeaveLevelMessage = (const QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
			return sQuestMessageLeaveLevel( pQuest, pLeaveLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_BACK_FROM_MOVIE:
		{
			const QUEST_MESSAGE_BACK_FROM_MOVIE * pBackFromMovieMessage = (const QUEST_MESSAGE_BACK_FROM_MOVIE *)pMessage;
			return sQuestMessageBackFromMovie( pQuest, pBackFromMovieMessage );
		}

		//----------------------------------------------------------------------------
		case QM_IS_UNIT_VISIBLE:
		{
			const QUEST_MESSAGE_IS_UNIT_VISIBLE *pVisibleMessage = (const QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
			return sQuestMessageIsUnitVisible( pQuest, pVisibleMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeHellgate(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST * pQuest,
	QUEST_DATA_HELLGATE * pQuestData)
{
	pQuestData->nEmmera					= QuestUseGI( pQuest, GI_MONSTER_EMMERA );
	pQuestData->nEmmeraOutside			= QuestUseGI( pQuest, GI_MONSTER_EMMERA_COMBAT );
	pQuestData->nEmmeraFlying			= QuestUseGI( pQuest, GI_MONSTER_EMMERA_FLYING );

	pQuestData->nFollowAI				= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW );

	ASSERT_RETURN( NUM_QUEST_HELLGATE_MINI_BOSSES >= 5 );
	pQuestData->nMiniBoss[0]			= QuestUseGI( pQuest, GI_MONSTER_HELL_MINI_1 );
	pQuestData->nMiniBoss[1]			= QuestUseGI( pQuest, GI_MONSTER_HELL_MINI_2 );
	pQuestData->nMiniBoss[2]			= QuestUseGI( pQuest, GI_MONSTER_HELL_MINI_3 );
	pQuestData->nMiniBoss[3]			= QuestUseGI( pQuest, GI_MONSTER_HELL_MINI_4 );
	pQuestData->nMiniBoss[4]			= QuestUseGI( pQuest, GI_MONSTER_HELL_MINI_5 );

	for ( int i = 0; i < NUM_QUEST_HELLGATE_MINI_BOSSES; i++ )
	{
		pQuestData->idMiniBoss[i] = INVALID_ID;
	}

	pQuestData->nSydonai				= QuestUseGI( pQuest, GI_MONSTER_SYDONAI );

	pQuestData->nLevelStPaulsHellgate	= QuestUseGI( pQuest, GI_LEVEL_ST_PAULS_HELLGATE );
	pQuestData->nLevelHell				= QuestUseGI( pQuest, GI_LEVEL_HELL );
	pQuestData->nLevelStPaulsStation	= QuestUseGI( pQuest, GI_LEVEL_ST_PAULS_STATION );

	pQuestData->nExitPortal				= QuestUseGI( pQuest, GI_OBJECT_GAME_EXIT_PORTAL );
	pQuestData->nWarpHelltoStPauls		= QuestUseGI( pQuest, GI_OBJECT_WARP_HELL_STPAULSHELLGATE );

	pQuestData->bSydonaiDead			= FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitHellgate(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_HELLGATE * pQuestData = ( QUEST_DATA_HELLGATE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_HELLGATE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmera );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmeraOutside );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS ) | MAKE_MASK( QSMF_BACK_FROM_MOVIE ) | MAKE_MASK( QSMF_IS_UNIT_VISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreHellgate(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
