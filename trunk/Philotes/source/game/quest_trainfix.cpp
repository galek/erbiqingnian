//----------------------------------------------------------------------------
// FILE: quest_trainfix.cpp
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
#include "quest_trainfix.h"
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
#include "states.h"
#include "warp.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define NUM_QUEST_TRAIN_PARTS		3

//----------------------------------------------------------------------------
struct QUEST_DATA_TRAINFIX
{
	int		nArphaun;
	int		nTechsmith99;
	int		nLevelCannonStreetRail;

	int		nTrainPart[ NUM_QUEST_TRAIN_PARTS ];
	int		nTrainCrate[ NUM_QUEST_TRAIN_PARTS ];
	BOOL	bPartClassUsed[ NUM_QUEST_TRAIN_PARTS ];

	int		nTurningInItem;	// temporary, indicates that a turn in operation is in progress
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TRAINFIX * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRAINFIX *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_CRATE_LOCATIONS				32

static void sCreateCrates(
	QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	VECTOR vPosition[ MAX_CRATE_LOCATIONS ];
	VECTOR vDirection[ MAX_CRATE_LOCATIONS ];
	ROOM * pRoomList[ MAX_CRATE_LOCATIONS ];
	int nNumLocations = 0;

	int nNumToSpawn = NUM_QUEST_TRAIN_PARTS;
	BOOL bExists[ NUM_QUEST_TRAIN_PARTS ];
	for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
		bExists[i] = FALSE;
		
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// check to see if spawned
		BOOL bAddRoom = TRUE;
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetGenus( test ) == GENUS_MONSTER )
			{
				for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
				{
					if ( UnitGetClass(test) == pQuestData->nTrainCrate[i] )
					{
						// found one
						bExists[i] = TRUE;
						bAddRoom = FALSE;
						nNumToSpawn--;
					}
				}
			}
		}

		// save the nodes
		if ( ( nNumLocations < MAX_CRATE_LOCATIONS ) && bAddRoom )
		{
			ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
			ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "trainfix_crate", &pOrientation );
			if ( nodeset && pOrientation )
			{
				s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition[ nNumLocations ], &vDirection[ nNumLocations ], NULL, NULL );
				pRoomList[ nNumLocations ] = room;
				nNumLocations++;
			}
		}
	}

	ASSERT_RETURN( nNumLocations >= nNumToSpawn );

	GAME * pGame = UnitGetGame( pPlayer );
	for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
	{
		// don't respawn all
		if ( bExists[i] )
			continue;

		// choose a location
		int index = RandGetNum( pGame->rand, nNumLocations );

		// and spawn the crate objects
		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nTrainCrate[i];
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 1 );
		ROOM * pSpawnRoom = pRoomList[index];
		tSpec.pRoom = pSpawnRoom;
		tSpec.vPosition = vPosition[index];
		tSpec.vDirection = vDirection[index];
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );

		// remove/replace in array
		nNumToSpawn--;
		nNumLocations--;
		pRoomList[index] = pRoomList[nNumLocations];
		vPosition[index] = vPosition[nNumLocations];
		vDirection[index] = vDirection[nNumLocations];

		// remove all in array from same room
		for ( int l = 0; l < nNumLocations; l++ )
		{
			if ( pRoomList[l] == pSpawnRoom )
			{
				nNumLocations--;
				pRoomList[l] = pRoomList[nNumLocations];
				vPosition[l] = vPosition[nNumLocations];
				vDirection[l] = vDirection[nNumLocations];
				l--;
				// still enough?
				ASSERT_RETURN( nNumLocations >= nNumToSpawn );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWarpPlayerToExodus( UNIT * pPlayer )
{
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = GlobalIndexGet( GI_LEVEL_EXODUS );
	tSpec.dwWarpFlags = WF_ARRIVE_AT_START_LOCATION_BIT;
	
	// warp!
	s_WarpToLevelOrInstance( pPlayer, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus >= QUEST_STATUS_COMPLETE )
	{
		if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) && ( UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelCannonStreetRail ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_TALKTECH ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3 ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3 ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_INFO_WAITING;
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitIsMonsterClass( pSubject, pQuestData->nArphaun ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRAINFIX_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
	{
		if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) && ( UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelCannonStreetRail ) )
		{
			sWarpPlayerToExodus( pQuest->pPlayer );
			return QMR_FINISHED;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ))
	{
		if ( UnitGetLevelDefinitionIndex( pQuest->pPlayer ) != pQuestData->nLevelCannonStreetRail )
		{
			return QMR_OK;
		}

		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_TALKTECH ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRAINFIX_TECH );
			return QMR_NPC_TALK;
		}
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2 ) == QUEST_STATE_ACTIVE ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRAINFIX_TECH_TURNIN );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3 ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRAINFIX_TECH_FINISHED );
			return QMR_NPC_TALK;
		}
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
		case DIALOG_TRAINFIX_INTRO:
		{
			s_QuestActivate( pQuest );
			s_QuestMapReveal( pQuest, GI_LEVEL_KING_WILLIAM_STREET );
			s_QuestMapReveal( pQuest, GI_LEVEL_THREADNEEDLE );
			s_QuestMapReveal( pQuest, GI_LEVEL_CANNON_STREET_RAIL );
			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_TRAINFIX_TECH:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_TALKTECH, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRAINFIX_TECH_TURNIN:
		{
			// remove parts
			QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
			int num_removed = 0;
			for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
			{
				UNIT * item = s_QuestFindFirstItemOfType( pQuest->pPlayer, pQuestData->nTrainPart[i] );
				if ( item != NULL )
				{
					UNIT * pPlayer = pQuest->pPlayer;
					// remove state
					s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_ENCUMBER, 0 );

					// remove item
					pQuestData->nTurningInItem = pQuestData->nTrainPart[i];
					s_QuestRemoveItem( pPlayer, pQuestData->nTrainPart[i] );
					pQuestData->nTurningInItem = INVALID_LINK;

					// inc count
					num_removed++;
				}
			}
			ASSERT_RETVAL( num_removed > 0, QMR_OK );
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1 ) == QUEST_STATE_ACTIVE )
			{
				if ( num_removed >= 1 )
				{
					QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_ACTIVE );
				}
				if ( num_removed >= 2 )
				{
					QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3, QUEST_STATE_ACTIVE );
				}
				ASSERT_RETVAL( num_removed < 3, QMR_OK );
			}
			else if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2 ) == QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3, QUEST_STATE_ACTIVE );
				ASSERT_RETVAL( num_removed == 1, QMR_OK );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRAINFIX_TECH_FINISHED:
		{
			// remove all parts
			QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
			for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
				s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nTrainPart[i] );
			// remove all states
			UNIT * pPlayer = pQuest->pPlayer;
			s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_ENCUMBER, 0 );

			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3, QUEST_STATE_COMPLETE );

			// new!
			s_QuestComplete( pQuest );

			sWarpPlayerToExodus( pPlayer );
			break;
		}
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetPartsUsed( QUEST * pQuest )
{
	UNIT *player = QuestGetPlayer( pQuest );
	ASSERT_RETURN( player );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( player ) ) );
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	UNIT * item = UnitInventoryIterate( player, NULL, dwFlags );
	while ( item != NULL )
	{
		int item_class = UnitGetClass( item );

		for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
		{
			if ( item_class == pQuestData->nTrainPart[i] )
				pQuestData->bPartClassUsed[i] = TRUE;
		}

		item = UnitInventoryIterate( player, item, dwFlags );		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelCannonStreetRail )
	{
		QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_TALKTECH, QUEST_STATE_ACTIVE );
		sCreateCrates( pQuest );
		sSetPartsUsed( pQuest );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sEncumber( QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	if ( !UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_ENCUMBER ) )
	{
		s_StateSet( pPlayer, pPlayer, STATE_QUEST_ENCUMBER, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT s_sQuestMessagePickup( 
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	int item_class = UnitGetClass( pMessage->pPickedUp );
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	BOOL bContinue = FALSE;
	int index = 0;
	for ( index = 0; !bContinue && ( index < NUM_QUEST_TRAIN_PARTS ); index++ )
	{
		if ( item_class == pQuestData->nTrainPart[index] )
			bContinue = TRUE;
	}

	// set state?
	if ( bContinue )
	{
		index--;
		ASSERT_RETVAL( index >= 0 && index < NUM_QUEST_TRAIN_PARTS, QMR_OK );

		// did we already pick up this part?
		if ( pQuestData->bPartClassUsed[index] )
			return QMR_OK;

		pQuestData->bPartClassUsed[index] = TRUE;

		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1 ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1, QUEST_STATE_ACTIVE );
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2 ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2, QUEST_STATE_ACTIVE );
		}
		else
		{
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3, QUEST_STATE_ACTIVE );
		}

		sEncumber( pQuest );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageDroppingItem( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_DROPPED_ITEM* pMessage)
{
	// first check if this is a trainfix item
	int item_class = pMessage->nItemClass;
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	BOOL bContinue = FALSE;
	int index = 0;
	for ( index = 0; !bContinue && ( index < NUM_QUEST_TRAIN_PARTS ); index++ )
	{
		if ( item_class == pQuestData->nTrainPart[index] )
			bContinue = TRUE;
	}

	// set state?
	if ( bContinue )
	{
		index--;
		ASSERT_RETVAL( index >= 0 && index < NUM_QUEST_TRAIN_PARTS, QMR_OK );

		if (pQuestData->nTurningInItem != item_class)
		{
			pQuestData->bPartClassUsed[index] = FALSE;

			if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3 ) == QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS3, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3, QUEST_STATE_ACTIVE );
			}
			else if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2 ) == QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS2, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS2, QUEST_STATE_ACTIVE );
			}
			else if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1 ) == QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_RETURNPARTS1, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS1, QUEST_STATE_ACTIVE );
			}
		}

		UNIT *pPlayer = QuestGetPlayer( pQuest );
		if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_ENCUMBER ) )
		{
			s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_ENCUMBER, 0 );
		}	
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageAttemptPickup(
	QUEST * pQuest,
	const QUEST_MESSAGE_ATTEMPTING_PICKUP * pMessage )
{
	// first check if this is a trainfix item
	int item_class = UnitGetClass( pMessage->pPickedUp );
	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );

	BOOL bContinue = FALSE;
	for ( int i = 0; !bContinue && ( i < NUM_QUEST_TRAIN_PARTS ); i++ )
	{
		if ( item_class == pQuestData->nTrainPart[i] )
			bContinue = TRUE;
	}

	if ( bContinue )
	{
		if ( !QuestIsActive( pQuest ) )
		{
			return QMR_PICKUP_FAIL;
		}

		// they already have one?
		UNIT * pPlayer = pQuest->pPlayer;
		if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_ENCUMBER ) )
		{
			return QMR_PICKUP_FAIL;
		}

		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRAINFIX_FINDPARTS3 ) == QUEST_STATE_COMPLETE )
		{
			return QMR_PICKUP_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageAbandon(
	QUEST * pQuest )
{
	ASSERT_RETVAL( pQuest, QMR_OK );
	UNIT *pPlayer = pQuest->pPlayer;
	ASSERT_RETVAL( pPlayer, QMR_OK );

	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
	for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
		pQuestData->bPartClassUsed[i] = FALSE;

	if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_ENCUMBER ) )
	{
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_ENCUMBER, 0 );
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
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestMessagePickup( pQuest, pMessagePickup);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_DROPPED_ITEM:
		{
			const QUEST_MESSAGE_DROPPED_ITEM *pMessageDropping = (const QUEST_MESSAGE_DROPPED_ITEM*)pMessage;
			return s_sQuestMessageDroppingItem( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessageDropping);
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_ATTEMPTING_PICKUP:
		case QM_SERVER_ATTEMPTING_PICKUP: // check client and server, because of latency issues
		{
			const QUEST_MESSAGE_ATTEMPTING_PICKUP *pMessagePickup = (const QUEST_MESSAGE_ATTEMPTING_PICKUP*)pMessage;
			return sQuestMessageAttemptPickup( pQuest, pMessagePickup );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ABANDON:
		{
			return sQuestMessageAbandon( pQuest );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTrainFix(
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
	QUEST *pQuest,
	QUEST_DATA_TRAINFIX * pQuestData)
{
	pQuestData->nArphaun					= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nTechsmith99				= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_99 );
	pQuestData->nLevelCannonStreetRail		= QuestUseGI( pQuest, GI_LEVEL_CANNON_STREET_RAIL );

	pQuestData->nTrainPart[0]				= QuestUseGI( pQuest, GI_ITEM_TRAINFIX_PART1 );
	pQuestData->nTrainPart[1]				= QuestUseGI( pQuest, GI_ITEM_TRAINFIX_PART2 );
	pQuestData->nTrainPart[2]				= QuestUseGI( pQuest, GI_ITEM_TRAINFIX_PART3 );

	pQuestData->nTrainCrate[0]				= QuestUseGI( pQuest, GI_MONSTER_TRAINFIX_CRATE1 );
	pQuestData->nTrainCrate[1]				= QuestUseGI( pQuest, GI_MONSTER_TRAINFIX_CRATE2 );
	pQuestData->nTrainCrate[2]				= QuestUseGI( pQuest, GI_MONSTER_TRAINFIX_CRATE3 );

	for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
		pQuestData->bPartClassUsed[i] = FALSE;

	pQuestData->nTurningInItem = INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTrainFix(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRAINFIX * pQuestData = ( QUEST_DATA_TRAINFIX * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TRAINFIX ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nTechsmith99 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestTrainFixOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST * pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );

	QUEST_DATA_TRAINFIX * pQuestData = sQuestDataGet( pQuest );
	ASSERTX_RETURN( pQuestData, "Quest not properly initialized" );
	
	for ( int i = 0; i < NUM_QUEST_TRAIN_PARTS; i++ )
	{
		UNIT * item = s_QuestFindFirstItemOfType( pQuest->pPlayer, pQuestData->nTrainPart[i] );
		if ( item != NULL )
			sEncumber( pQuest );
	}
}
