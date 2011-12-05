//----------------------------------------------------------------------------
// FILE: quest_cube.cpp
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
#include "quest_cube.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "s_message.h"
#include "uix.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_CUBE
{
	BOOL	bStartupMessage;
	int		nNemo;
	int		nHolbornStation;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_CUBE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_CUBE *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	if ( !QuestIsInactive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_CUBE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nNemo ) )
	{
		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nHolbornStation )
			return QMR_OK;

		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		return QMR_NPC_INFO_NEW;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	if ( QuestIsComplete( pQuest ) )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_CUBE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nNemo ) )
	{
		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nHolbornStation )
			return QMR_OK;

		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		s_QuestActivate( pQuest );

		const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

		s_QuestDisplayDialog(
			pQuest,
			pSubject,
			NPC_DIALOG_OK_CANCEL,
			pQuestDefinition->nDescriptionDialog );

		return QMR_OFFERING;
	}
#endif
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( s_QuestComplete( pQuest ) )
			{								
				// offer reward
				if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
				{					
					return QMR_OFFERING;
				}
			}					
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
	if ( !QuestIsInactive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_CUBE * pQuestData = sQuestDataGet( pQuest );
	if ( pQuestData->bStartupMessage )
		return QMR_OK;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	// send message
	MSG_SCMD_UISHOWMESSAGE tMessage;
	tMessage.bType = (BYTE)QUIM_DIALOG;
	tMessage.nDialog = DIALOG_CUBE_START;
	tMessage.nParam1 = pQuest->nQuest;
	tMessage.nParam2 = 5 * GAME_TICKS_PER_SECOND;
	tMessage.bFlags = (BYTE)UISMS_DIALOG_QUEST;
	GAMECLIENTID idClient = UnitGetClientId( player );
	s_SendMessage( game, idClient, SCMD_UISHOWMESSAGE, &tMessage );

	pQuestData->bStartupMessage = TRUE;

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
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeCube(
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
	QUEST_DATA_CUBE * pQuestData)
{
	pQuestData->bStartupMessage = FALSE;
	pQuestData->nNemo = QuestUseGI( pQuest, GI_MONSTER_NEMO );
	pQuestData->nHolbornStation = QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitCube(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_CUBE * pQuestData = ( QUEST_DATA_CUBE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_CUBE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nNemo );
}
