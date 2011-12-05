//----------------------------------------------------------------------------
// FILE: quest_wisdomchaos.cpp
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
#include "quest_wisdomchaos.h"
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
struct QUEST_DATA_WISDOMCHAOS
{
	int		nLevelCGS;
	int		nLevelCCS;

	int		nBrandonLann;
	int		AlayPenn;
	int		Nasim;
	int		nLordArphaun;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_WISDOMCHAOS * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_WISDOMCHAOS *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	QUEST_DATA_WISDOMCHAOS * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WISDOMCHAOS_TALK_LORD ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
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
	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );
	QUEST_DATA_WISDOMCHAOS * pQuestData = sQuestDataGet( pQuest );

	if ( s_QuestIsGiver( pQuest, pSubject ) && ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE ) )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		int nDialog = INVALID_LINK;
		if (UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
		{
			nDialog = DIALOG_WISDOMCHAOS_LANN;
		}
		else if (UnitIsMonsterClass( pSubject, pQuestData->AlayPenn ))
		{
			nDialog = DIALOG_WISDOMCHAOS_ALAY;
		}
		else
		{
			nDialog = DIALOG_WISDOMCHAOS_NASIM;	
		}
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, nDialog );
		return QMR_NPC_TALK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;


	if ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) && 
		( QuestStateGetValue( pQuest, QUEST_STATE_WISDOMCHAOS_TALK_LORD ) == QUEST_STATE_ACTIVE ) )
	{
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_WISDOMCHAOS_LORD, INVALID_LINK );
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
	int nDialogStopped = pMessage->nDialog;

	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_WISDOMCHAOS_LANN:
		case DIALOG_WISDOMCHAOS_ALAY:
		case DIALOG_WISDOMCHAOS_NASIM:	
		{
			// activate quest
			if ( s_QuestActivate( pQuest ) )
			{
				// reveal map path
				s_QuestMapReveal( pQuest, GI_LEVEL_LEICESTER_SQUARE );
				s_QuestMapReveal( pQuest, GI_LEVEL_ST_MARTINS );
				s_QuestMapReveal( pQuest, GI_LEVEL_CHARING_CROSS_APPROACH );
				s_QuestMapReveal( pQuest, GI_LEVEL_CHARING_CROSS_STATION );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_WISDOMCHAOS_LORD:
		{
			QuestStateSet( pQuest, QUEST_STATE_WISDOMCHAOS_TALK_LORD, QUEST_STATE_COMPLETE );
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
		QUEST_DATA_WISDOMCHAOS * pQuestData = sQuestDataGet( pQuest );
		if ( pMessage->nLevelNewDef == pQuestData->nLevelCCS )
		{
			if ( QuestStateSet( pQuest, QUEST_STATE_WISDOMCHAOS_TRAVEL_CCSTATION, QUEST_STATE_COMPLETE ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_WISDOMCHAOS_TALK_LORD, QUEST_STATE_ACTIVE );
			}
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
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeWisdomChaos(
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
	QUEST_DATA_WISDOMCHAOS * pQuestData)
{
	pQuestData->nLevelCGS 			= QuestUseGI( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );
	pQuestData->nLevelCCS 			= QuestUseGI( pQuest, GI_LEVEL_CHARING_CROSS_STATION );

	pQuestData->nBrandonLann		= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN );
	pQuestData->AlayPenn			= QuestUseGI( pQuest, GI_MONSTER_ALAY_PENN );
	pQuestData->Nasim				= QuestUseGI( pQuest, GI_MONSTER_NASIM );
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitWisdomChaos(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_WISDOMCHAOS * pQuestData = ( QUEST_DATA_WISDOMCHAOS * )GMALLOCZ( pGame, sizeof( QUEST_DATA_WISDOMCHAOS ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLordArphaun );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreWisdomChaos(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

