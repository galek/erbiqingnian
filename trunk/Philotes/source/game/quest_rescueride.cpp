//----------------------------------------------------------------------------
// FILE: quest_rescueride.cpp
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
#include "quest_rescueride.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "states.h"
#include "colors.h"
#include "ai.h"
#include "movement.h"
#include "quest_truth_d.h"
#include "room_layout.h"
#include "room_path.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define NUM_TRAIN_BLOCKERS_TO_SPAWN			4

//----------------------------------------------------------------------------
struct QUEST_DATA_RESCUERIDE
{
	int		nTechsmith99;

	int		nLevelExodus;

	int		nTrainEngine;
	int		nTrainPassenger;

	int		nMonsterTrainBlocker;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_RESCUERIDE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_RESCUERIDE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_BLOCKER_LOCATIONS				32

static void sCreateTrainBlockers(
	QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	VECTOR vPosition[ MAX_BLOCKER_LOCATIONS ];
	ROOM * pRoomList[ MAX_BLOCKER_LOCATIONS ];
	VECTOR vFinalPosition;
	ROOM * pFinalRoom = NULL;
	int nNumLocations = 0;
		
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// check to see if spawned
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitIsMonsterClass( test, pQuestData->nMonsterTrainBlocker ) )
			{
				// found one already
				return;
			}
		}

		// save the nodes
		if ( nNumLocations < MAX_BLOCKER_LOCATIONS )
		{
			ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
			ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "train_blocker", &pOrientation );
			if ( nodeset && pOrientation )
			{
				s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition[ nNumLocations ], NULL, NULL, NULL );
				pRoomList[ nNumLocations ] = room;
				nNumLocations++;
			}
			nodeset = RoomGetLabelNode( room, "final_train_blocker", &pOrientation );
			if ( nodeset && pOrientation )
			{
				s_QuestNodeGetPosAndDir( room, pOrientation, &vFinalPosition, NULL, NULL, NULL );
				pFinalRoom = room;
			}
		}
	}

	ASSERT_RETURN( nNumLocations >= NUM_TRAIN_BLOCKERS_TO_SPAWN );
	ASSERT_RETURN( pFinalRoom != NULL );

	GAME * pGame = UnitGetGame( pPlayer );
	VECTOR vDirection = VECTOR( 1.0f, 0.0f, 0.0f );

	for ( int i = 0; i < NUM_TRAIN_BLOCKERS_TO_SPAWN; i++ )
	{
		ASSERT_RETURN( NUM_TRAIN_BLOCKERS_TO_SPAWN - i <= nNumLocations );

		// choose a location
		int index = RandGetNum( pGame->rand, nNumLocations );

		ROOM * pDestRoom;
		ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pRoomList[index], vPosition[index], &pDestRoom );

		if ( pNode )
		{
			// init location
			SPAWN_LOCATION tLocation;
			SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

			MONSTER_SPEC tSpec;
			tSpec.nClass = pQuestData->nMonsterTrainBlocker;
			tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 25 );
			tSpec.pRoom = pDestRoom;
			tSpec.vPosition = tLocation.vSpawnPosition;
			tSpec.vDirection = vDirection;
			tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
			s_MonsterSpawn( pGame, tSpec );
		}

		// remove/replace in array
		if ( nNumLocations > 0 )
		{
			nNumLocations--;
			pRoomList[index] = pRoomList[nNumLocations];
			vPosition[index] = vPosition[nNumLocations];
		}
	}

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pFinalRoom, vFinalPosition, &pDestRoom );

	if ( pNode )
	{
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nMonsterTrainBlocker;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 25 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sMoveTrain( QUEST * pQuest )
{
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT * pTrain =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nTrainEngine );
	ASSERT_RETURN( pTrain );
	s_ObjectStartMove( pTrain, pQuest->pPlayer );
	pTrain =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nTrainPassenger );
	ASSERT_RETURN( pTrain );
	s_ObjectStartMove( pTrain, pQuest->pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestTrainRideOver( QUEST * pQuest )
{
	// complete quest and spawn bad-boy
	QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_TALKTECH, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_CLEAR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_DEFEND, QUEST_STATE_COMPLETE );
	s_QuestComplete( pQuest );
	QUEST * pTruth = QuestGetQuest( pQuest->pPlayer, QUEST_TRUTH_D );
	s_QuestTruthDBegin( pTruth );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) )
		{
			if ( !s_QuestIsPrereqComplete( pQuest ) )
				return QMR_OK;

			if ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelExodus )
			{
				return QMR_NPC_STORY_NEW;
			}
			return QMR_OK;
		}
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_RESCUERIDE_TALKTECH ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendStartupVideo( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );
	s_NPCVideoStart( UnitGetGame( pQuest->pPlayer ), pQuest->pPlayer, GI_MONSTER_LORD_ARPHAUN, NPC_VIDEO_INSTANT_INCOMING, DIALOG_RESCUERIDE_VIDEO );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) )
	{
		if ( eStatus == QUEST_STATUS_INACTIVE )
		{
			if ( !s_QuestIsPrereqComplete( pQuest ) )
				return QMR_OK;

			if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelExodus )
				return QMR_OK;

			sSendStartupVideo( pQuest );
			return QMR_FINISHED;
		}
		else if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_RESCUERIDE_TALKTECH ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_RESCUERIDE_TECH );
				return QMR_NPC_TALK;
			}
			return QMR_OK;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoComplete( QUEST * pQuest )
{
	// set states to complete talking and activate tech talk
	s_QuestActivate( pQuest );
	QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_TALKTECH, QUEST_STATE_ACTIVE );
	sCreateTrainBlockers( pQuest );
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
		case DIALOG_RESCUERIDE_VIDEO:
		{
			sVideoComplete( pQuest );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_RESCUERIDE_TECH:
		{
			QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_TALKTECH, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_CLEAR, QUEST_STATE_ACTIVE );
			return QMR_FINISHED;
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
	if ( pMessage->nDialog == DIALOG_RESCUERIDE_VIDEO )
	{
		sVideoComplete( pQuest );
		return QMR_FINISHED;
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelExodus )
		return QMR_OK;

	// is this a train blocking monster?
	if ( UnitIsMonsterClass( pVictim, pQuestData->nMonsterTrainBlocker ) )
	{
		LEVEL * pLevel = UnitGetLevel( pVictim );
		for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
		{
			// check to see if any still alive
			for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
			{
				if ( UnitIsMonsterClass( test, pQuestData->nMonsterTrainBlocker ) && !IsUnitDeadOrDying( test ) )
				{
					return QMR_FINISHED;
				}
			}
		}
		QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_CLEAR, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_RESCUERIDE_DEFEND, QUEST_STATE_ACTIVE );
		s_NPCVideoStart( UnitGetGame( pQuest->pPlayer ), pQuest->pPlayer, GI_MONSTER_LORD_ARPHAUN, NPC_VIDEO_INSTANT_INCOMING, DIALOG_RESCUERIDE_TECH_VIDEO );
		sMoveTrain( pQuest );
		return QMR_FINISHED;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	QUEST_DATA_RESCUERIDE * pQuestData = sQuestDataGet( pQuest );

	if ( ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
	{
		if ( pMessage->nLevelNewDef == pQuestData->nLevelExodus )
		{
			sSendStartupVideo( pQuest );
			return QMR_FINISHED;
		}
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelExodus )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_RESCUERIDE_CLEAR ) != QUEST_STATE_COMPLETE )
		{
			sCreateTrainBlockers( pQuest );
			return QMR_FINISHED;
		}
		else
		{
			sMoveTrain( pQuest );
			return QMR_FINISHED;
		}
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
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeRescueRide(
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
	QUEST_DATA_RESCUERIDE * pQuestData)
{
	pQuestData->nTechsmith99			= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_99 );
	pQuestData->nLevelExodus			= QuestUseGI( pQuest, GI_LEVEL_EXODUS );
	pQuestData->nTrainEngine			= QuestUseGI( pQuest, GI_OBJECT_BATTLETRAIN_ENGINE );
	pQuestData->nTrainPassenger			= QuestUseGI( pQuest, GI_OBJECT_BATTLETRAIN_PASSENGER );
	pQuestData->nMonsterTrainBlocker	= QuestUseGI( pQuest, GI_MONSTER_TRAIN_BLOCKER );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitRescueRide(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_RESCUERIDE * pQuestData = ( QUEST_DATA_RESCUERIDE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_RESCUERIDE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nTechsmith99 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreRescueRide(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
