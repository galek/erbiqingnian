//----------------------------------------------------------------------------
// FILE: quest_books.cpp
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
#include "quest_books.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "ai.h"
#include "uix.h"
#include "states.h"
#include "monsters.h"
#include "room.h"
#include "room_path.h"
#include "offer.h"
#include "e_portal.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_BOOKS
{
	int		nLordArphaun;
	int		nLevelCCStation;
	int		nLevelPicBooks;
	int		nLevelPicHell;
	int		nOracleStand;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_BOOKS * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_BOOKS *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	
	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_BOOKS * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
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

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );
	QUEST_DATA_BOOKS * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_BOOKS_INTRO );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	// quest active
	if ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) && 
		QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_TALK_LORD ) == QUEST_STATE_ACTIVE)
	{
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_TWO_OK, DIALOG_BOOKS_LORD, INVALID_LINK, GI_MONSTER_ORACLE );
		return QMR_NPC_TALK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{

		//----------------------------------------------------------------------------
		case DIALOG_BOOKS_INTRO:
		{
			// activate quest
			if ( s_QuestActivate( pQuest ) )
			{
				// reveal map path
				s_QuestMapReveal( pQuest, GI_LEVEL_PICCADILLY_APPROACH );
				s_QuestMapReveal( pQuest, GI_LEVEL_PICCADILLY_CIRCUS );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_BOOKS_LORD:
		{
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_TALK_LORD, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
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
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{
		QUEST_DATA_BOOKS * pQuestData = sQuestDataGet( pQuest );
		if ( ( pMessage->nLevelNewDef == pQuestData->nLevelPicBooks ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_TRAVEL_PB ) == QUEST_STATE_ACTIVE ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_TRAVEL_PB, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_BREACH, QUEST_STATE_ACTIVE );
		}

		if ( ( pMessage->nLevelNewDef == pQuestData->nLevelPicHell ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_ENTER ) == QUEST_STATE_ACTIVE ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_ENTER, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_RECOVER, QUEST_STATE_ACTIVE );
		}

		if ( ( pMessage->nLevelNewDef == pQuestData->nLevelCCStation ) &&
			 ( QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_RETURN_CCSTATION ) == QUEST_STATE_ACTIVE ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_RETURN_CCSTATION, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_BOOKS_TALK_LORD, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_BOOKS * pQuestData = sQuestDataGet( pQuest );

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nOracleStand ) )
	{
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_RECOVER ) == QUEST_STATE_ACTIVE ) &&
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
static void sOracleTaken(
	void *pUserData )

{
	QUEST *pQuest = (QUEST *)pUserData;
	
	// finished getting leg for now...
	QuestStateSet( pQuest, QUEST_STATE_BOOKS_RECOVER, QUEST_STATE_COMPLETE );

	// activate rest of quest
	QuestStateSet( pQuest, QUEST_STATE_BOOKS_RETURN_CCSTATION, QUEST_STATE_ACTIVE );
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{

	if (QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE)
	{
		return QMR_OK;
	}

	QUEST_DATA_BOOKS * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nHellriftLevel = UnitGetLevelDefinitionIndex( pTarget );
	if ( ( nHellriftLevel == pQuestData->nLevelPicBooks ) && UnitIsA( pTarget, UNITTYPE_HELLRIFT_PORTAL ) && !pMessage->bDoTrigger )
	{
		QuestStateSet( pQuest, QUEST_STATE_BOOKS_BREACH, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_BOOKS_ENTER, QUEST_STATE_ACTIVE );
	}

	int nTargetClass = UnitGetClass( pTarget );

	if ( ( nTargetClass == pQuestData->nOracleStand ) ) // && pMessage->bDoTrigger ) // cmarch: bDoTrigger is always false now that sTriggerQuestObject in object.cpp is commented out
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_BOOKS_RECOVER ) == QUEST_STATE_ACTIVE )
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sOracleTaken;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTarget, OFFERID_QUEST_BOOKS_ORACLE, tActions );
			return QMR_OFFERING;
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
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
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
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeBooks(
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
	QUEST_DATA_BOOKS * pQuestData)
{
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelCCStation		= QuestUseGI( pQuest, GI_LEVEL_CHARING_CROSS_STATION );
	pQuestData->nLevelPicBooks		= QuestUseGI( pQuest, GI_LEVEL_PICCADILLY_CIRCUS );
	pQuestData->nLevelPicHell		= QuestUseGI( pQuest, GI_LEVEL_PICCADILLY_HELL );
	pQuestData->nOracleStand		= QuestUseGI( pQuest, GI_OBJECT_ORACLE_STAND );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitBooks(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_BOOKS * pQuestData = ( QUEST_DATA_BOOKS * )GMALLOCZ( pGame, sizeof( QUEST_DATA_BOOKS ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreBooks(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

