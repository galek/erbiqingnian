//----------------------------------------------------------------------------
// FILE: quest_secretmsgs.cpp
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
#include "quest_secretmsgs.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_SECRETMSGS
{
	int		nMaxim;
	int		nLann;
	int		nAltair;
	int		nArphaun;

	int		nLannMessage;
	int		nAltairMessage;

	int		nLevelBishopsCourt;
	int		nLevelOldBailey;
	int		nLevelMonumentStation;
	int		nLevelTemplarBase;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_SECRETMSGS * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_SECRETMSGS *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasNotes(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	if ( s_QuestActivate( pQuest ) )
	{
		s_QuestMapReveal( pQuest, GI_LEVEL_KINGSWAY );
		s_QuestMapReveal( pQuest, GI_LEVEL_STONECUTTER );
		s_QuestMapReveal( pQuest, GI_LEVEL_BISHOPS_COURT );
		s_QuestMapReveal( pQuest, GI_LEVEL_OLD_BAILEY );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	
	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nAltair ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKAERON ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nLann ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKLANN ) == QUEST_STATE_ACTIVE )
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nMaxim ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_SECRETMESSAGES_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nAltair ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKAERON ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_SECRETMESSAGES_ALTAIR );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLann ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKLANN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_SECRETMESSAGES_LANN );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKARPHAUN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_SECRETMESSAGES_COMPLETE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoComplete( QUEST * pQuest )
{
	QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TRAVELMS, QUEST_STATE_ACTIVE );
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
		case DIALOG_SECRETMESSAGES_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasNotes;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTSECRETMESSAGESOFFER, tActions );

			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_SECRETMESSAGES_ALTAIR:
		{
			UNIT * pPlayer = pQuest->pPlayer;
			GAME * pGame = UnitGetGame( pPlayer );
			QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );
			s_QuestRemoveItem( pPlayer, pQuestData->nAltairMessage );
			s_NPCEmoteStart( pGame, pPlayer, DIALOG_SECRETMESSAGES_ALTAIR_EMOTE );

			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKAERON, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TRAVELOB, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_SECRETMESSAGES_LANN:
		{
			UNIT * pPlayer = pQuest->pPlayer;
			GAME * pGame = UnitGetGame( pPlayer );
			QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );
			s_QuestRemoveItem( pPlayer, pQuestData->nLannMessage );
			s_NPCEmoteStart( pGame, pPlayer, DIALOG_SECRETMESSAGES_LANN_EMOTE );

			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKLANN, QUEST_STATE_COMPLETE );

			s_NPCVideoStart( pGame, pPlayer, GI_MONSTER_LORD_MAXIM, NPC_VIDEO_INCOMING, DIALOG_SECRETMESSAGES_VIDEO, INVALID_ID, GI_MONSTER_LORD_ARPHAUN );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_SECRETMESSAGES_VIDEO:
		{
			sVideoComplete( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_SECRETMESSAGES_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKARPHAUN, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			s_QuestDelayNext( pQuest, 15 * GAME_TICKS_PER_SECOND );
			break;
		}

	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if ( pMessage->nDialog == DIALOG_SECRETMESSAGES_VIDEO )
	{
		sVideoComplete( pQuest );
		return QMR_FINISHED;
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

	QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelBishopsCourt )
	{
		QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TRAVELBC, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKAERON, QUEST_STATE_ACTIVE );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelOldBailey )
	{
		QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TRAVELOB, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKLANN, QUEST_STATE_ACTIVE );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelMonumentStation )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_SECRETMSGS_TALKLANN ) >= QUEST_STATE_COMPLETE )
		{
			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TRAVELMS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_SECRETMSGS_TALKARPHAUN, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageIsUnitVisible( 
	QUEST *pQuest,
	const QUEST_MESSAGE_IS_UNIT_VISIBLE * pMessage)
{
	if ( !QuestIsComplete( pQuest ) )
		return QMR_OK;

	QUEST_DATA_SECRETMSGS * pQuestData = sQuestDataGet( pQuest );
	UNIT * pUnit = pMessage->pUnit;

	if ( UnitGetGenus( pUnit ) != GENUS_MONSTER )
		return QMR_OK;

	int nUnitClass = UnitGetClass( pUnit );
	//int nLevelIndex = UnitGetLevelDefinitionIndex( pUnit );

	if ( nUnitClass == pQuestData->nMaxim )
	{
		return QMR_UNIT_HIDDEN;
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
		case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_IS_UNIT_VISIBLE:
		{
			const QUEST_MESSAGE_IS_UNIT_VISIBLE *pVisibleMessage = (const QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
			return sQuestMessageIsUnitVisible( pQuest, pVisibleMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeSecretMessages(
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
	QUEST_DATA_SECRETMSGS * pQuestData)
{
	pQuestData->nMaxim					= QuestUseGI( pQuest, GI_MONSTER_LORD_MAXIM );
	pQuestData->nArphaun				= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );

	pQuestData->nLann					= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN_COMBAT );
	pQuestData->nAltair					= QuestUseGI( pQuest, GI_MONSTER_AERON_ALTAIR_COMBAT );

	pQuestData->nLannMessage			= QuestUseGI( pQuest, GI_ITEM_SECRETMESSAGES_LANN );
	pQuestData->nAltairMessage			= QuestUseGI( pQuest, GI_ITEM_SECRETMESSAGES_ALTAIR );
	
	pQuestData->nLevelBishopsCourt		= QuestUseGI( pQuest, GI_LEVEL_BISHOPS_COURT );
	pQuestData->nLevelOldBailey			= QuestUseGI( pQuest, GI_LEVEL_OLD_BAILEY );
	pQuestData->nLevelMonumentStation	= QuestUseGI( pQuest, GI_LEVEL_MONUMENT_STATION );
	pQuestData->nLevelTemplarBase		= QuestUseGI( pQuest, GI_LEVEL_TEMPLAR_BASE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitSecretMessages(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_SECRETMSGS * pQuestData = ( QUEST_DATA_SECRETMSGS * )GMALLOCZ( pGame, sizeof( QUEST_DATA_SECRETMSGS ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nAltair );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLann );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_BACK_FROM_MOVIE ) | MAKE_MASK( QSMF_IS_UNIT_VISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreSecretMessages(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
