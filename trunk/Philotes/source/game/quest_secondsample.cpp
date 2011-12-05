//----------------------------------------------------------------------------
// FILE: quest_secondsample.cpp
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
#include "quest_secondsample.h"
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

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_SECONDSAMPLE
{
	int		nRorke;
	int		nSerSing;

	int		nLevelPuddleDock;
	int		nLevelExospector;

	int		nExospectorPOI;
	int		nHeartPOI;

	int		nContainer;
	int		nFilledSample;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_SECONDSAMPLE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_SECONDSAMPLE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasContainer(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	if ( s_QuestActivate( pQuest ) )
	{
		s_QuestMapReveal( pQuest, GI_LEVEL_THE_STRAND );
		s_QuestMapReveal( pQuest, GI_LEVEL_BLACKFRIARS );
		s_QuestMapReveal( pQuest, GI_LEVEL_PUDDLE_DOCK );
	}
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
	QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_SECONDSAMPLE_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECONDSAMPLE_RETURN ) == QUEST_STATE_ACTIVE )
		{
			if ( s_QuestCheckForItem( pPlayer, pQuestData->nFilledSample ) )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_SECONDSAMPLE_COMPLETE );
				return QMR_NPC_TALK;
			}
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_SECONDSAMPLE_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasContainer;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTSECONDSAMPLEOFFER, tActions );
				
			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_SECONDSAMPLE_COMPLETE:
		{
			UNIT * pPlayer = pQuest->pPlayer;
			// remove the sample
			QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );
			if ( s_QuestCheckForItem( pPlayer, pQuestData->nFilledSample ) )
			{
				s_QuestRemoveItem( pPlayer, pQuestData->nFilledSample );
				QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_RETURN, QUEST_STATE_COMPLETE );
				s_QuestComplete( pQuest );
				s_NPCEmoteStart( UnitGetGame( pPlayer ), pPlayer, DIALOG_SECONDSAMPLE_EMOTE );
				s_QuestDelayNext( pQuest, 20 * GAME_TICKS_PER_SECOND );
				s_StateSet( pTalkingTo, pTalkingTo, STATE_NPC_SUMMON, 0 );
			}
			break;
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelPuddleDock )
	{
		QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_FIND, QUEST_STATE_ACTIVE );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelExospector )
	{
		QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_BREACH, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_HEART, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{

	if (QuestIsActive( pQuest ))
	{
		QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

		if ( UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nExospectorPOI ))
		{
			QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_FIND, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_BREACH, QUEST_STATE_ACTIVE );
		}

		if ( UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nHeartPOI ))
		{
			QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_HEART, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_COLLECT, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if ( pItem )
	{
		QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nContainer )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_SECONDSAMPLE_COLLECT ) == QUEST_STATE_ACTIVE )
				{
					// check the level for a fresh boil
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
					UNIT * heart =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nHeartPOI );
					if ( heart )
					{
						const UNIT_DATA * heart_data = UnitGetData( heart );
						float use_dist = heart_data->fCollisionRadius;
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, heart->vPosition );
						if ( distsq <= ( use_dist * use_dist ) )
						{
							return QMR_USE_OK;
						}
					}
				}
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		QUEST_DATA_SECONDSAMPLE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nContainer )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_SECONDSAMPLE_COLLECT ) == QUEST_STATE_ACTIVE )
				{
					// check the level for a fresh boil
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
					UNIT * heart =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nHeartPOI );
					if ( heart )
					{
						const UNIT_DATA * heart_data = UnitGetData( heart );
						float use_dist = heart_data->fCollisionRadius;
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, heart->vPosition );
						if ( distsq <= ( use_dist * use_dist ) )
						{
							QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_COLLECT, QUEST_STATE_COMPLETE );
							QuestStateSet( pQuest, QUEST_STATE_SECONDSAMPLE_RETURN, QUEST_STATE_ACTIVE );
							s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nContainer );
							s_QuestGiveItem( pQuest, GI_TREASURE_SECONDSAMPLE_FILLED, 0 );
							return QMR_USE_OK;
						}
					}
				}
			}
			return QMR_USE_FAIL;
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
		case QM_CAN_USE_ITEM:
		{
			const QUEST_MESSAGE_CAN_USE_ITEM *pCanUseItemMessage = ( const QUEST_MESSAGE_CAN_USE_ITEM * )pMessage;
			return sQuestMessageCanUseItem( pQuest, pCanUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_USE_ITEM:
		{
			const QUEST_MESSAGE_USE_ITEM *pUseItemMessage = ( const QUEST_MESSAGE_USE_ITEM * )pMessage;
			return sQuestMessageUseItem( pQuest, pUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeSecondSample(
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
	QUEST_DATA_SECONDSAMPLE * pQuestData)
{
	pQuestData->nRorke						= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nSerSing					= QuestUseGI( pQuest, GI_MONSTER_SER_SING );
	pQuestData->nLevelPuddleDock			= QuestUseGI( pQuest, GI_LEVEL_PUDDLE_DOCK );
	pQuestData->nLevelExospector			= QuestUseGI( pQuest, GI_LEVEL_EXOSPECTOR );
	pQuestData->nExospectorPOI				= QuestUseGI( pQuest, GI_OBJECT_EXOSPECTOR_POI );
	pQuestData->nHeartPOI					= QuestUseGI( pQuest, GI_OBJECT_HEART_POI );
	pQuestData->nContainer					= QuestUseGI( pQuest, GI_ITEM_SECONDSAMPLE_CONTAINER );
	pQuestData->nFilledSample				= QuestUseGI( pQuest, GI_ITEM_SECONDSAMPLE_FILLED );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitSecondSample(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_SECONDSAMPLE * pQuestData = ( QUEST_DATA_SECONDSAMPLE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_SECONDSAMPLE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreSecondSample(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
