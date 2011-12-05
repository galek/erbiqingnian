//----------------------------------------------------------------------------
// FILE: quest_doorfix.h
//
// Story quest for Act 1. Collect items to open up the passageway. Based on 
// collect template code.
//
// author: Chris March, 6-13-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_doorfix.h"
#include "quest_template.h"
#include "quest_collect.h"
#include "quests.h"
#include "dialog.h"
#include "offer.h"
#include "level.h"
#include "s_quests.h"
#include "Quest.h"
#include "items.h"
#include "treasure.h"
#include "monsters.h"
#include "states.h"
#include "objects.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
struct QUEST_DATA_DOORFIX
{
	int nObjectWarpH_CGA;
	QUEST_DATA_COLLECT tCollect;	// data for shared quest_collect code
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_DOORFIX * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_DOORFIX *)pQuest->pQuestData;
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_COLLECT * sQuestDataGetCollect( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_DOORFIX *pData = sQuestDataGet( pQuest );
	ASSERT_RETNULL( pData );
	return &pData->tCollect;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_DOORFIX *pQuestData)
{
	ASSERT_RETURN( pQuestData != NULL );
	pQuestData->nObjectWarpH_CGA	= QuestUseGI( pQuest, GI_OBJECT_WARP_HOLBORN_CGAPPROACH );
	QuestCollectDataInit( pQuest, &pQuestData->tCollect );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsADoorFixItem(
	UNIT *pItem,
	QUEST *pQuest)
{
	if (UnitIsA( pItem, UNITTYPE_ITEM ) &&
		pQuest->pQuestDef->nCollectItem == UnitGetClass( pItem ))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestDoorFixMessagePickup( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );

	// check for completing the doorfixion
	if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) == QUEST_STATE_ACTIVE &&
		 sUnitIsADoorFixItem( pMessage->pPickedUp, pQuest ) )
	{
		int nNumCollected = UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ));
		++nNumCollected;
		UnitSetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nNumCollected );

		if ( nNumCollected == 1 )
			s_QuestTrack( pQuest, TRUE );

		if ( nNumCollected >= pQuest->pQuestDef->nObjectiveCount ) // TODO cmarch multiple item classes?
		{
			QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT_COUNT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_DOORFIX_RETURN, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerUpdateDoorFixedCount(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest)
{
	// what are we looking for
	SPECIES spItem = MAKE_SPECIES( GENUS_ITEM, pQuest->pQuestDef->nCollectItem );

	// it must actually be on their person
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	SETBIT( dwFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

	int nNumDoorFixItemsInInventory = 
		UnitInventoryCountUnitsOfType( pPlayer, spItem, dwFlags );
	UnitSetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nNumDoorFixItemsInInventory ); // TODO cmarch multiple item classes?
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestOnEnterGameDoorFix(
	const QUEST_FUNCTION_PARAM &tParam)
{
	UNIT *pPlayer = tParam.pPlayer;
	QUEST *pQuest = tParam.pQuest;

	if ( QuestIsActive( pQuest ) &&
		 QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) >= QUEST_STATE_ACTIVE )
	{
		// reset item count stat
		GAME *pGame = UnitGetGame( pPlayer );
		sPlayerUpdateDoorFixedCount( pPlayer, pGame, pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageNPCInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest->nQuest );
	ASSERT_RETVAL( pQuestDefinition, QMR_OK );

	// Quest giver
	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// Activate the quest			
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nDescriptionDialog );

			return QMR_NPC_TALK;
		}
	}
	else if ( QuestIsActive( pQuest ) && s_QuestIsRewarder( pQuest,  pSubject ) )
	{
		DWORD dwFlags = 0;
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) == QUEST_STATE_COMPLETE &&
			 UnitInventoryCountUnitsOfType( 
				pPlayer, 
				MAKE_SPECIES( GENUS_ITEM, pQuest->pQuestDef->nCollectItem ), 
				SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT ) ) >= pQuestDefinition->nObjectiveCount )
		{
			// Reward: tell the player good job, and offer rewards
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nCompleteDialog,
				pQuestDefinition->nRewardDialog );

			return QMR_NPC_TALK;
		}
		else
		{
			// Incomplete: tell the player to go kill the monster
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK,
				pQuestDefinition->nIncompleteDialog );

			return QMR_NPC_TALK;
		}
	}
#endif			
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT, QUEST_STATE_ACTIVE );
					QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT_COUNT, QUEST_STATE_ACTIVE );

					// reset item count
					GAME *pGame = UnitGetGame( pPlayer );
					sPlayerUpdateDoorFixedCount( pPlayer, pGame, pQuest );

					// check for immediate completion
					if ( UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT )) >= pQuestDefinition->nObjectiveCount ) // TODO cmarch multiple item classes?
					{
						QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_DOORFIX_COLLECT_COUNT, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_DOORFIX_RETURN, QUEST_STATE_ACTIVE );
					}
					else
					{
						// starting in the quest destination? spawn monsters in the rooms already activated by the player
						s_QuestCollectMessageRoomsActivated(UnitGetGame( pPlayer ), pQuest, sQuestDataGetCollect( pQuest ) );
					}
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_DOORFIX_RETURN, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{	
					if ( AppIsDemo() )
					{
						// force open portal
						UNIT * pPlayer = pQuest->pPlayer;
						QUEST_DATA_DOORFIX * pQuestData = sQuestDataGet( pQuest );
						s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nObjectWarpH_CGA );
					}

					s_QuestRemoveQuestItems( pPlayer, pQuest );

					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}	
			}
		}
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	return QMR_OK;		
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	if ( !AppIsDemo() )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_DOORFIX *pQuestData = sQuestDataGet( pQuest );

	if (UnitIsObjectClass( pObject, pQuestData->nObjectWarpH_CGA ))
	{	
		QUEST_STATUS eStatus = QuestGetStatus( pQuest );

		if ( eStatus < QUEST_STATUS_COMPLETE )
			return QMR_OPERATE_FORBIDDEN;

		return QMR_OPERATE_OK;
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
			return s_sQuestMessageNPCInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return s_sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_CREATE_LEVEL:
		{
			const QUEST_MESSAGE_CREATE_LEVEL * pMessageCreateLevel = ( const QUEST_MESSAGE_CREATE_LEVEL * )pMessage;
			return s_QuestCollectMessageCreateLevel( pQuest, pMessageCreateLevel, sQuestDataGetCollect( pQuest ) );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageEnterLevel( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest, 
							pMessageEnterLevel, 
							sQuestDataGetCollect( pQuest ) );

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ROOMS_ACTIVATED:
		{
			// "message data" is in the quest globals
			// check if a spawn room was activated, spawn baddies
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageRoomsActivated( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest,
							sQuestDataGetCollect( pQuest ) );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessagePartyKill( pQuest, pPartyKillMessage, sQuestDataGetCollect( pQuest ) );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_ATTEMPTING_PICKUP:
		case QM_SERVER_ATTEMPTING_PICKUP: // check client and server, because of latency issues
		{
			const QUEST_MESSAGE_ATTEMPTING_PICKUP *pMessagePickup = (const QUEST_MESSAGE_ATTEMPTING_PICKUP*)pMessage;
			// if the item was spawned for this quest, but the doorfixion state is not active, disallow pickup
			if ( pMessagePickup->nQuestIndex == pQuest->nQuest &&
				 QuestStateGetValue( pQuest, QUEST_STATE_DOORFIX_COLLECT ) != QUEST_STATE_ACTIVE )
			{
				return QMR_PICKUP_FAIL;
			}

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestDoorFixMessagePickup( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessagePickup);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				UnitClearStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, StatParam( STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) );
			}
			// fall through

		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return QuestTemplateMessageStatus( pQuest, pMessageStatus );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}
		
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeDoorFix(
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
void QuestInitDoorFix(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_DOORFIX * pQuestData = ( QUEST_DATA_DOORFIX * )GMALLOCZ( pGame, sizeof( QUEST_DATA_DOORFIX ) );
	pQuest->pQuestData = pQuestData;
	sQuestDataInit( pQuest, pQuestData );

	// chance to drop quest items on kills by party members
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );
}
