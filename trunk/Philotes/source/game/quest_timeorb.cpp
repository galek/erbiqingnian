//----------------------------------------------------------------------------
// FILE: quest_timeorb.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "game.h"
#include "globalindex.h"
#include "fontcolor.h"
#include "colors.h"
#include "npc.h"
#include "quest_timeorb.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "offer.h"
#include "level.h"
#include "stringtable.h"
#include "ai.h"
#include "items.h"
#include "treasure.h"
#include "uix.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "states.h"
#include "clients.h"
#include "c_quests.h"
#include "s_quests.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define TIME_ORB_ACTIVE_SECONDS			90									// length of time warp
#define TIME_ORB_TIME_DIVISOR			2									// make sure these two match. one server, one client
#define TIME_ORB_STEP_SPEED				(APP_STEP_SPEED_DEFAULT + 1)		// 1/2 speed (should match divisor above!!)

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_TIME_ORB
{
	int		nDeadCabalist;
	int		nRewardCabalist;
	int		nRewardTemplar;
	int		nTimeDevice;
	int		nCabalistPDA;

	int		nBlurClass;
	int		nBlurAIActive;
	int		nBlurAIPassive;
	int		nBlurTentacle;

	int		nDRLG;
	int		nFuseTime;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TIME_ORB * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return ( QUEST_DATA_TIME_ORB * )pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTimeOrbChangeBlurAI( QUEST * pQuest, int nAI, BOOL bClear )
{
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETURN( player );
	GAME * game = UnitGetGame( player );
	ASSERT_RETURN( game );

	// check the level for a blur
	LEVEL * level = UnitGetLevel( player );
	for ( ROOM * room = LevelGetFirstRoom( level ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetClass( test ) == pQuestData->nBlurClass )
			{
				// found!
				AI_Free( game, test );
				UnitSetStat( test, STATS_AI, nAI );
				if ( bClear )
				{
					s_StateClear( test, UnitGetId( test ), STATE_BLUR_START, 0 );
					s_StateSet( test, test, STATE_ORBILE_ANGRY, 0 );
				}
				else
				{
					s_StateClear( test, UnitGetId( test ), STATE_ORBILE_ANGRY, 0 );
					s_StateSet( test, test, STATE_BLUR_START, 0 );
				}
				AI_Init( game, test );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCompleteTimeOrb( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETURN( player );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( player ) ) );

	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	if ( s_QuestComplete( pQuest ) )
	{
		// remove pda
		if ( s_QuestCheckForItem( player, pQuestData->nCabalistPDA ) )
		{
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nCabalistPDA );
		}
		
		// remove tentacles
		while ( s_QuestCheckForItem( player, pQuestData->nBlurTentacle ) )
		{
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nBlurTentacle );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sServerTimeOrbDone( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	// clear up the time stuff for the server
	GameClearTimeSlow( game );
	UnitClearFlag( unit, UNITFLAG_TIMESLOW_CONTROLLER );

	// reset Blur AIs
	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	sTimeOrbChangeBlurAI( pQuest, pQuestData->nBlurAIPassive, FALSE );

	// finish time event
	s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_COLLECT, QUEST_STATE_COMPLETE );
	s_QuestVideoToAll( pQuest, GI_MONSTER_TIME_ORB_CABALIST, NPC_VIDEO_INCOMING, DIALOG_TIME_ORB_TIMEOVER );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sQuestTimeOrbAbort( GAME * game, UNIT * player )
{
	ASSERT_RETURN( game );
	ASSERT_RETURN( player );
	ASSERT_RETURN( UnitTestFlag( player, UNITFLAG_TIMESLOW_CONTROLLER ) );

	ASSERT_RETURN( UnitHasTimedEvent( player, sServerTimeOrbDone ) );

	UnitUnregisterTimedEvent( player, sServerTimeOrbDone );
	QUEST * pQuest = QuestGetQuest( player, QUEST_TIMEORB );
	if ( !pQuest )
		return;
	EVENT_DATA event_data;
	event_data.m_Data1 = (DWORD_PTR)pQuest;
	sServerTimeOrbDone( game, player, event_data );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sServerTimeOrbActivate( GAME * game, UNIT * player, QUEST * pQuest )
{
	// set flags for time stuffs
	GameSetTimeSlow( game, TIME_ORB_TIME_DIVISOR );
	UnitSetFlag( player, UNITFLAG_TIMESLOW_CONTROLLER );

	// change AIs of all blur
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	sTimeOrbChangeBlurAI( pQuest, pQuestData->nBlurAIActive, TRUE );

	// set event to end it all
	UnitRegisterEventTimed( player, sServerTimeOrbDone, EVENT_DATA( (DWORD_PTR)pQuest ), ( GAME_TICKS_PER_SECOND * TIME_ORB_ACTIVE_SECONDS ) / TIME_ORB_TIME_DIVISOR );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
	QUEST_DATA_TIME_ORB * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	
	switch (pQuest->eStatus)
	{

		//----------------------------------------------------------------------------
		case QUEST_STATUS_INACTIVE:
		{
			if (UnitIsMonsterClass( pSubject, pData->nDeadCabalist ))
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TIME_ORB_INIT );
				return QMR_NPC_TALK;
			}
			break;
		}

		//----------------------------------------------------------------------------
		case QUEST_STATUS_ACTIVE:
		{
			if (UnitIsMonsterClass( pSubject, pData->nDeadCabalist ))
			{
				// search the cabalist messages
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_SEARCH ) == QUEST_STATE_ACTIVE )
				{
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TIME_ORB_SEARCH, DIALOG_TIME_ORB_SEARCH_REWARD );
				}
				else
				{
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TIME_ORB_EMPTY );
				}
				return QMR_NPC_TALK;
			}
			if (UnitIsMonsterClass( pSubject, pData->nRewardCabalist ))
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_RETURN ) == QUEST_STATE_ACTIVE )
				{
					if ( s_QuestCheckForItem( pPlayer, pData->nBlurTentacle ) )
					{
						s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TIME_ORB_CABALIST_COMPLETE, DIALOG_TIME_ORB_CABALIST_REWARD );
					}
					else
					{
						s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TIME_ORB_NO_TENTACLES, DIALOG_TIME_ORB_NO_TENTACLES_REWARD );
					}
				}
			}
			if (UnitIsMonsterClass( pSubject, pData->nRewardTemplar ))
			{
				if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_RETURN ) == QUEST_STATE_ACTIVE ) &&
					 ( s_QuestCheckForItem( pPlayer, pData->nBlurTentacle ) ) )
				{
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TIME_ORB_ALT_COMPLETE, DIALOG_TIME_ORB_ALT_REWARD );
				}
			}
			break;
		}
	}
	
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasPDA(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	// complete search, set next state active
	ASSERT_RETURN( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_SEARCH ) == QUEST_STATE_ACTIVE );
	s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_SEARCH, QUEST_STATE_COMPLETE );

	// just set this state on the person that picked up the pda
	QuestStateSet( pQuest, QUEST_STATE_TIMEORB_USE_PDA, QUEST_STATE_ACTIVE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	UNIT *pPlayer = pQuest->pPlayer;
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_INIT:
		{
			// activate quest
			s_QuestActivateForAll( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_SEARCH:
		{
		
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasPDA;
			tActions.pAllTakenUserData = pQuest;
			
			// do offer
			s_OfferGive( pPlayer, pTalkingTo, OFFERID_QUEST_TIMEORB_PDA, tActions );
			return QMR_OFFERING;
			
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_ACTIVATE:
		{
			ASSERT_RETVAL( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_ACTIVATE ) == QUEST_STATE_ACTIVE, QMR_OK );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_ACTIVATE, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_COLLECT, QUEST_STATE_ACTIVE );

			// set up the time freeze
			sServerTimeOrbActivate( UnitGetGame( pPlayer ), pPlayer, pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_TIMEOVER:
		{
			QuestStateSet( pQuest, QUEST_STATE_TIMEORB_RETURN, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_NO_TENTACLES:
		{
			QuestStateSet( pQuest, QUEST_STATE_TIMEORB_RETURN, QUEST_STATE_COMPLETE );
			sCompleteTimeOrb( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_CABALIST_COMPLETE:
		{
			// cabalist reward
			s_QuestGiveGold( pQuest, 1000 );
			sCompleteTimeOrb( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TIME_ORB_ALT_COMPLETE:
		{
			sCompleteTimeOrb( pQuest );
			if (s_QuestOfferReward( pQuest, pTalkingTo ))
			{
				return QMR_OFFERING;
			}
			break;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTimeOrbUpdateDisplay( QUEST * pQuest )
{
#if !ISVERSION(SERVER_VERSION)

	QUEST_DATA_TIME_ORB * data = sQuestDataGet( pQuest );
	const int MAX_STRING = 1024;
	WCHAR timestr[ MAX_STRING ];

	if ( data->nFuseTime > 1 )
	{
		const WCHAR *puszFuseString = GlobalStringGet( GS_TIME_REMAINING_MULTI_SECOND );

		PStrPrintf( 
			timestr, 
			MAX_STRING, 
			puszFuseString,
			data->nFuseTime );
	}
	else
	{
		const WCHAR *puszFuseString = GlobalStringGet( GS_TIME_REMAINING_SINGLE_SECOND );
		PStrCopy( timestr, puszFuseString, MAX_STRING );
	}

	// show on screen
	UIShowQuickMessage( timestr );
#endif //!ISVERSION(SERVER_VERSION)


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sFuseUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_CLIENT( game ) );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	QUEST_DATA_TIME_ORB * data = sQuestDataGet( pQuest );
	data->nFuseTime--;
	if ( data->nFuseTime )
	{
		sTimeOrbUpdateDisplay( pQuest );
		UnitRegisterEventTimed( unit, sFuseUpdate, &tEventData, GAME_TICKS_PER_SECOND / TIME_ORB_TIME_DIVISOR );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		UIHideQuickMessage();
#endif// !ISVERSION(SERVER_VERSION)
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nTimeDevice ) )
	{
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_ACTIVATE ) == QUEST_STATE_ACTIVE ) &&
			 ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE ) )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
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
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	//UNIT * pPlayer = pQuest->pPlayer;
	UNIT * pTarget = pMessage->pTarget;
	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nTimeDevice )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_ACTIVATE ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pTarget, NPC_DIALOG_OK, DIALOG_TIME_ORB_ACTIVATE );
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( eStatus == QUEST_STATUS_ACTIVE )
	{
		QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if (UnitIsMonsterClass( pSubject, pQuestData->nDeadCabalist ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_SEARCH ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_RETURN;
			}
			return QMR_OK;
		}
		if (UnitIsMonsterClass( pSubject, pQuestData->nRewardTemplar ))
		{
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_RETURN ) == QUEST_STATE_ACTIVE ) &&
				 ( s_QuestCheckForItem( pQuest->pPlayer, pQuestData->nBlurTentacle ) ) )
			{
				return QMR_NPC_INFO_RETURN;
			}
		}
		// hide any "?" or "!" from reward cabalist before the end of the quest
		if (UnitIsMonsterClass( pSubject, pQuestData->nRewardCabalist ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_RETURN ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_RETURN;
			}
			return QMR_OK;
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
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nCabalistPDA )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_USE_PDA ) == QUEST_STATE_ACTIVE )
				{
					return QMR_USE_OK;
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
	QUEST_DATA_TIME_ORB * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nCabalistPDA )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TIMEORB_USE_PDA ) == QUEST_STATE_ACTIVE )
				{
					// send video to all
					s_QuestVideoToAll( pQuest, GI_MONSTER_TIME_ORB_CABALIST, NPC_VIDEO_OK, DIALOG_TIME_ORB_VIDEO );
					// mark all quest logs
					s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_USE_PDA, QUEST_STATE_ACTIVE );
					s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_USE_PDA, QUEST_STATE_COMPLETE );
					s_QuestStateSetForAll( pQuest, QUEST_STATE_TIMEORB_ACTIVATE, QUEST_STATE_ACTIVE );
				}
				return QMR_OK;			
			}
			return QMR_USE_FAIL;
		}
	}
	return QMR_OK;			
}

//----------------------------------------------------------------------------
// client got a message from server about a new state
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	UNIT * player = pQuest->pPlayer;
	ASSERT_RETVAL( player, QMR_OK );
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( game, QMR_OK );
	ASSERT_RETVAL( IS_CLIENT( game ), QMR_OK );

	// do time warp
	if ( nQuestState == QUEST_STATE_TIMEORB_COLLECT )
	{
		if ( eValue == QUEST_STATE_ACTIVE )
		{
			// set display timer
			sTimeOrbUpdateDisplay( pQuest );
			UnitRegisterEventTimed( pQuest->pPlayer, sFuseUpdate, &EVENT_DATA( (DWORD_PTR)pQuest ), GAME_TICKS_PER_SECOND );
			AppCommonSetStepSpeed(TIME_ORB_STEP_SPEED);
		}
		else if ( eValue == QUEST_STATE_COMPLETE )
		{
			AppCommonSetStepSpeed(APP_STEP_SPEED_DEFAULT);
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageLeaveLevel(
	QUEST *pQuest,
	const QUEST_MESSAGE_LEAVE_LEVEL * pMessage)
{
	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	if ( ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE ) &&
		 UnitTestFlag( player, UNITFLAG_TIMESLOW_CONTROLLER ) )
	{
		sQuestTimeOrbAbort( game, player );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_TIME_ORB * pData = sQuestDataGet( pQuest );

	if ( ( pData->nDRLG == pMessage->nDRLGDefId ) && ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE ) )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
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
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}
		
		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pLeaveLevelMessage = (const QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
			return sQuestMessageLeaveLevel( pQuest, pLeaveLevelMessage );
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
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTimeOrb(
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
	QUEST_DATA_TIME_ORB * pQuestData)
{
	pQuestData->nDeadCabalist		= QuestUseGI( pQuest, GI_MONSTER_TIME_ORB_DEAD );
	pQuestData->nRewardCabalist 	= QuestUseGI( pQuest, GI_MONSTER_TIME_ORB_CABALIST );
	pQuestData->nRewardTemplar		= QuestUseGI( pQuest, GI_MONSTER_TIME_ORB_TEMPLAR );
	pQuestData->nTimeDevice			= QuestUseGI( pQuest, GI_OBJECT_TIME_ORB );
	pQuestData->nCabalistPDA		= QuestUseGI( pQuest, GI_ITEM_TIME_ORB_PDA );
	pQuestData->nBlurClass			= QuestUseGI( pQuest, GI_MONSTER_BLUR );
	pQuestData->nBlurAIActive		= QuestUseGI( pQuest, GI_AI_BLUR );
	pQuestData->nBlurAIPassive		= QuestUseGI( pQuest, GI_AI_BLUR_START );
	pQuestData->nBlurTentacle		= QuestUseGI( pQuest, GI_ITEM_BLUR_TENTACLE );
	pQuestData->nDRLG				= QuestUseGI( pQuest, GI_DRLG_PARK );
	
	pQuestData->nFuseTime = TIME_ORB_ACTIVE_SECONDS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTimeOrb(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
		
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TIME_ORB * pQuestData = ( QUEST_DATA_TIME_ORB * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TIME_ORB ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nRewardTemplar);

}
