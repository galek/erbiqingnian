//----------------------------------------------------------------------------
// FILE: quest_intro.cpp
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
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "s_quests.h"
#include "stringtable.h"
#include "c_sound.h"
#include "player.h"
#include "states.h"
#include "monsters.h"
#include "treasure.h"
#include "items.h"
#include "level.h"
#include "room_layout.h"
#include "faction.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_INTRO
{
	int		nMurmur;
	int		nBrandonLann;
	int		nLevelHolborn;
	int		nLevelCGStation;
	int		nItemFawkesMessage;
	BOOL	bLannPause;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_INTRO *sQuestDataGet(
	QUEST *pQuest)
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_INTRO *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus == QUEST_STATUS_INACTIVE )
	{
		QUEST_DATA_INTRO * pQuestData = sQuestDataGet( pQuest );
		if ( UnitIsMonsterClass( pSubject, pQuestData->nMurmur ) )
		{
			if ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelHolborn )
			{
				return QMR_NPC_STORY_NEW;
			}
			return QMR_OK;
		}
	}

	if ( eStatus == QUEST_STATUS_ACTIVE )
	{
		QUEST_DATA_INTRO * pQuestData = sQuestDataGet( pQuest );

		if (UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_INTRO_TALK_LANN ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
			return QMR_OK;
		}

		if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
		{
			int current_level = UnitGetLevelDefinitionIndex( pSubject );
			
			if (current_level == pQuestData->nLevelCGStation && 
				QuestStateGetValue( pQuest, QUEST_STATE_INTRO_TALK_MURMUR ) == QUEST_STATE_ACTIVE)
			{
				return QMR_NPC_STORY_RETURN;
			}
			return QMR_OK;
		}
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
	QUEST_DATA_INTRO * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		int current_level = UnitGetLevelDefinitionIndex( pPlayer );
		if ( current_level == pQuestData->nLevelHolborn )
		{
			if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_INTRO_MURMUR_HOLBORN );
				return QMR_NPC_TALK;
			}
		}
	}
	else if ( pQuest->eStatus == QUEST_STATUS_ACTIVE )
	{
		// quest active
		if (UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_INTRO_TALK_LANN ) == QUEST_STATE_ACTIVE )
			{
				if ( pQuestData->bLannPause )
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_INTRO_LANN_PT2 );
				else
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_INTRO_LANN );
				return QMR_NPC_TALK;
			}
		}

		if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
		{
			int current_level = UnitGetLevelDefinitionIndex( pPlayer );
			if ( ( current_level == pQuestData->nLevelCGStation ) && ( QuestStateGetValue( pQuest, QUEST_STATE_INTRO_TALK_MURMUR ) == QUEST_STATE_ACTIVE ) )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_INTRO_MURMUR );
				return QMR_NPC_TALK;
			}
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLannPause( 
	GAME *pGame, 
	UNIT *pUnit, 
	const struct EVENT_DATA &tEventData )
{
	ASSERT_RETFALSE( pGame && pUnit );
	ASSERT( IS_SERVER( pGame ) );
	
	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	UNITID idLann = (UNITID)tEventData.m_Data2;
	MSG_SCMD_INTERACT_INFO tMessage;
	tMessage.idSubject = idLann;
	tMessage.bInfo = (BYTE)INTERACT_STORY_RETURN;
	s_SendMessage( pGame, UnitGetClientId( pQuest->pPlayer ), SCMD_INTERACT_INFO, &tMessage );

	QUEST_DATA_INTRO * pQuestData = sQuestDataGet( pQuest );
	pQuestData->bLannPause = TRUE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	int nDialogStopped = pMessage->nDialog;
	QUEST_DATA_INTRO *pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_INTRO_MURMUR_HOLBORN:
		{
			if ( s_QuestActivate( pQuest ) )
			{
				s_QuestMapReveal( pQuest, GI_LEVEL_CG_APPROACH );
				s_QuestMapReveal( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_INTRO_MURMUR:
		{
			QuestStateSet( pQuest, QUEST_STATE_INTRO_TALK_MURMUR, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_INTRO_TALK_LANN, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_INTRO_LANN:
		{
			// remove fawkes message from player inventory
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nItemFawkesMessage );

			// clear Lann's '?'
			UNITID idLann = UnitGetId( pTalkingTo );
			MSG_SCMD_INTERACT_INFO tMessage;
			tMessage.idSubject = idLann;
			tMessage.bInfo = (BYTE)INTERACT_INFO_NONE;
			s_SendMessage( UnitGetGame( pTalkingTo ), UnitGetClientId( pQuest->pPlayer ), SCMD_INTERACT_INFO, &tMessage );

			// set an event to put it back after 5 seconds...
			UnitRegisterEventTimed( pQuest->pPlayer, sLannPause, &EVENT_DATA( (DWORD_PTR)pQuest, (DWORD_PTR)idLann ), 5 * GAME_TICKS_PER_SECOND );			
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_INTRO_LANN_PT2:
		{
			QuestStateSet( pQuest, QUEST_STATE_INTRO_TALK_LANN, QUEST_STATE_COMPLETE );
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
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_INTRO * pQuestData = sQuestDataGet( pQuest );
	if ( pMessage->nLevelNewDef == pQuestData->nLevelCGStation )
	{
		QuestStateSet( pQuest, QUEST_STATE_INTRO_TRAVEL_CG, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_INTRO_TALK_MURMUR, QUEST_STATE_ACTIVE );
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
void QuestFreeIntro(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	GAME *pGame = QuestGetGame( pQuest );
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( pGame, pQuest->pQuestData );
	pQuest->pQuestData = NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataIntro(
	QUEST *pQuest,
	QUEST_DATA_INTRO * pQuestData )
{
	pQuestData->nMurmur				= QuestUseGI( pQuest, GI_MONSTER_MURMUR );
	pQuestData->nBrandonLann		= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN );
	pQuestData->nLevelHolborn		= QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
	pQuestData->nLevelCGStation		= QuestUseGI( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );
	pQuestData->nItemFawkesMessage	= QuestUseGI( pQuest, GI_ITEM_FAWKES_MESSAGE );
	pQuestData->bLannPause = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitIntro(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	GAME *pGame = QuestGetGame( pQuest );
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
	
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	QUEST_DATA_INTRO * pQuestData = ( QUEST_DATA_INTRO * )GMALLOCZ( pGame, sizeof( QUEST_DATA_INTRO ) );
	sQuestDataIntro( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMurmur );
}
