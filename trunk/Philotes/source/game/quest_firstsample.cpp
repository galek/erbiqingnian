//----------------------------------------------------------------------------
// FILE: quest_firstsample.cpp
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
#include "quest_firstsample.h"
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
struct QUEST_DATA_FIRSTSAMPLE
{
	int		nSerSing;
	int		nRorke;

	int		nLevelAldwych;

	int		nFreshBoil;

	int		nSampleContainer;
	int		nFilledSample;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_FIRSTSAMPLE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_FIRSTSAMPLE *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasGoggles(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	if ( s_QuestActivate( pQuest ) )
	{
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
	QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nRorke ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_FIRSTSAMPLE_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_FIRSTSAMPLE_RETURN ) == QUEST_STATE_ACTIVE )
		{
			if ( s_QuestCheckForItem( pPlayer, pQuestData->nFilledSample ) )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_FIRSTSAMPLE_COMPLETE );
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
		case DIALOG_FIRSTSAMPLE_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasGoggles;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTFIRSTSAMPLEOFFER, tActions );
				
			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_FIRSTSAMPLE_VIDEO:
		{
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_VIDEO, QUEST_STATE_COMPLETE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_FIRSTSAMPLE_COMPLETE:
		{
			UNIT * pPlayer = pQuest->pPlayer;
			// remove the sample
			QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );
			if ( s_QuestCheckForItem( pPlayer, pQuestData->nFilledSample ) )
			{
				s_QuestRemoveItem( pPlayer, pQuestData->nFilledSample );
				QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_RETURN, QUEST_STATE_COMPLETE );
				s_QuestComplete( pQuest );
				s_NPCEmoteStart( UnitGetGame( pPlayer ), pPlayer, DIALOG_FIRSTSAMPLE_EMOTE );
				s_QuestDelayNext( pQuest, 15 * GAME_TICKS_PER_SECOND );
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

	QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelAldwych )
	{
		QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_TRAVEL, QUEST_STATE_COMPLETE );
		if ( QuestStateGetValue( pQuest, QUEST_STATE_FIRSTSAMPLE_VIDEO ) != QUEST_STATE_COMPLETE )
		{
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_VIDEO, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_FIND, QUEST_STATE_ACTIVE );
			s_NPCVideoStart( game, player, GI_MONSTER_SER_SING, NPC_VIDEO_INCOMING, DIALOG_FIRSTSAMPLE_VIDEO );
			return QMR_OK;
		}
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
		QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

		if ( UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nFreshBoil ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_VIDEO, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_FIND, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_COLLECT, QUEST_STATE_ACTIVE );
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
		QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nSampleContainer )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_FIRSTSAMPLE_COLLECT ) == QUEST_STATE_ACTIVE )
				{
					// check the level for a fresh boil
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
					UNIT * freshboil =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nFreshBoil );
					if ( freshboil )
					{
						const UNIT_DATA * freshboil_data = UnitGetData( freshboil );
						float use_dist = freshboil_data->flMonitorApproachRadius;
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, freshboil->vPosition );
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
		QUEST_DATA_FIRSTSAMPLE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nSampleContainer )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_FIRSTSAMPLE_COLLECT ) == QUEST_STATE_ACTIVE )
				{
					// check the level for a fresh boil
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
					UNIT * freshboil =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nFreshBoil );
					if ( freshboil )
					{
						const UNIT_DATA * freshboil_data = UnitGetData( freshboil );
						float use_dist = freshboil_data->flMonitorApproachRadius;
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, freshboil->vPosition );
						if ( distsq <= ( use_dist * use_dist ) )
						{
							QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_COLLECT, QUEST_STATE_COMPLETE );
							QuestStateSet( pQuest, QUEST_STATE_FIRSTSAMPLE_RETURN, QUEST_STATE_ACTIVE );
							s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nSampleContainer );
							s_QuestGiveItem( pQuest, GI_TREASURE_FIRSTSAMPLE_FILLED, 0 );
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
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
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

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeFirstSample(
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
	QUEST_DATA_FIRSTSAMPLE * pQuestData)
{
	pQuestData->nSerSing			= QuestUseGI( pQuest, GI_MONSTER_SER_SING );
	pQuestData->nRorke				= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nLevelAldwych		= QuestUseGI( pQuest, GI_LEVEL_ALDWYCH );
	pQuestData->nFreshBoil			= QuestUseGI( pQuest, GI_OBJECT_FRESH_BOIL );
	pQuestData->nSampleContainer	= QuestUseGI( pQuest, GI_ITEM_FIRSTSAMPLE_CONTAINER );
	pQuestData->nFilledSample		= QuestUseGI( pQuest, GI_ITEM_FIRSTSAMPLE_FILLED );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitFirstSample(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_FIRSTSAMPLE * pQuestData = ( QUEST_DATA_FIRSTSAMPLE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_FIRSTSAMPLE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreFirstSample(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
