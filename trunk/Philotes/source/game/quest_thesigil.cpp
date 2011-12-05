//----------------------------------------------------------------------------
// FILE: quest_thesigil.cpp
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
#include "quest_thesigil.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "quest_testknowledge.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_THESIGIL
{
	int		nEmmera;

	int		nPDA_Data;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_THESIGIL * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_THESIGIL *)pQuest->pQuestData;
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
	QUEST_DATA_THESIGIL * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_THESIGIL_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_THESIGIL_TALK_EMMERA ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_THESIGIL_COMPLETE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasPDA(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;
	s_QuestActivate( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_THESIGIL_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasPDA;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTTHESIGILOFFER, tActions );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_THESIGIL_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_THESIGIL_TALK_EMMERA, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			QUEST * pTest = QuestGetQuest( pQuest->pPlayer, QUEST_TESTKNOWLEDGE );
			s_QuestToKBegin( pTest );
			return QMR_FINISHED;
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
		QUEST_DATA_THESIGIL * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nPDA_Data )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				return QMR_USE_OK;
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUseDataCrystal( QUEST * pQuest )
{
	// use it
	QUEST_DATA_THESIGIL * pQuestData = sQuestDataGet( pQuest );
	s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nPDA_Data );
	// data map effects
	s_QuestMapReveal( pQuest, GI_LEVEL_TEST_OF_KNOWLEDGE );
	s_QuestMapReveal( pQuest, GI_LEVEL_TEST_OF_LEADERSHIP );
	s_QuestMapReveal( pQuest, GI_LEVEL_TEST_OF_BEAUTY );
	s_QuestMapReveal( pQuest, GI_LEVEL_TEST_OF_FELLOWSHIP );
	s_QuestMapReveal( pQuest, GI_LEVEL_TEST_OF_HOPE );
	s_QuestMapReveal( pQuest, GI_LEVEL_FINSBURY_SQUARE );
	// set quest states
	QuestStateSet( pQuest, QUEST_STATE_THESIGIL_USEDATA, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_THESIGIL_TALK_EMMERA, QUEST_STATE_ACTIVE );
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
		QUEST_DATA_THESIGIL * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nPDA_Data )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_THESIGIL_USEDATA ) == QUEST_STATE_ACTIVE )
				{
					sUseDataCrystal( pQuest );
					s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nPDA_Data );
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
void QuestFreeTheSigil(
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
	QUEST_DATA_THESIGIL * pQuestData)
{
	pQuestData->nEmmera		= QuestUseGI( pQuest, GI_MONSTER_EMMERA );
	pQuestData->nPDA_Data	= QuestUseGI( pQuest, GI_ITEM_THE_SIGIL_DATA );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTheSigil(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_THESIGIL * pQuestData = ( QUEST_DATA_THESIGIL * )GMALLOCZ( pGame, sizeof( QUEST_DATA_THESIGIL ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTheSigil(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
