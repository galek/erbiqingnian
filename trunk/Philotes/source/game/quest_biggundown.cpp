//----------------------------------------------------------------------------
// FILE: quest_biggundown.cpp
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
#include "quest_biggundown.h"
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
struct QUEST_DATA_BIGGUNDOWN
{
	int		nRorke;
	int		nLevelGundown;
	int		nExospector;
	int		nTurret;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_BIGGUNDOWN * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_BIGGUNDOWN *)pQuest->pQuestData;
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
	QUEST_DATA_BIGGUNDOWN * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_BIGGUNDOWN_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_BIGGUNDOWN_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_BIGGUNDOWN_COMPLETE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteractGeneric( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_GENERIC *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_BIGGUNDOWN * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nTurret ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_BIGGUNDOWN_OPERATE ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_OPERATE, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_KILL, QUEST_STATE_ACTIVE );
			return QMR_OK;
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
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_BIGGUNDOWN_INTRO:
		{
			if ( s_QuestActivate( pQuest ) )
			{
				s_QuestMapReveal( pQuest, GI_LEVEL_NEW_BRIDGE );
				s_QuestMapReveal( pQuest, GI_LEVEL_BIG_GUNDOWN );
			}

			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_BIGGUNDOWN_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_RETURN, QUEST_STATE_COMPLETE );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_BIGGUNDOWN * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelGundown )
	{
		QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_OPERATE, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_BIGGUNDOWN * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// turn taint into interactive NPC if player doesn't have leg already
	if (UnitIsMonsterClass( pVictim, pQuestData->nExospector ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_OPERATE, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_KILL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_BIGGUNDOWN_RETURN, QUEST_STATE_ACTIVE );
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
		case QM_SERVER_INTERACT_GENERIC:
		{
			const QUEST_MESSAGE_INTERACT_GENERIC *pInteractMessage = (const QUEST_MESSAGE_INTERACT_GENERIC *)pMessage;
			return sQuestMessageInteractGeneric( pQuest, pInteractMessage );
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
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeBigGundown(
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
	QUEST_DATA_BIGGUNDOWN * pQuestData)
{
	pQuestData->nRorke				= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nLevelGundown		= QuestUseGI( pQuest, GI_LEVEL_BIG_GUNDOWN );
	pQuestData->nExospector			= QuestUseGI( pQuest, GI_MONSTER_EXOSPECTOR );
	pQuestData->nTurret				= QuestUseGI( pQuest, GI_MONSTER_CANNON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitBigGundown(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_BIGGUNDOWN * pQuestData = ( QUEST_DATA_BIGGUNDOWN * )GMALLOCZ( pGame, sizeof( QUEST_DATA_BIGGUNDOWN ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreBigGundown(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
