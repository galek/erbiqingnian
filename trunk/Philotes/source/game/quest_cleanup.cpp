//----------------------------------------------------------------------------
// FILE: quest_cleanup.cpp
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
#include "quest_cleanup.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "quest_truth_c.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_CLEANUP
{
	int		nRorke;
	int		nSerSing;
	int		nLevelAngelPassage;
	int		nFuruncle;
	int		nItemCleanser;
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_CLEANUP * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_CLEANUP *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	if ( QuestIsInactive( pQuest ))
		return QMR_OK;

	QUEST_DATA_CLEANUP * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
	{
		DWORD dwInventoryIterateFlagsCleanserCheck = 0;  
		SETBIT( dwInventoryIterateFlagsCleanserCheck, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
		BOOL bHasCleanser = s_QuestCheckForItem( pPlayer, pQuestData->nItemCleanser, dwInventoryIterateFlagsCleanserCheck );
		if (bHasCleanser == FALSE)
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
	QUEST_DATA_CLEANUP * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_CLEANUP_INTRO );
			return QMR_NPC_TALK;
		}
	}

	// give the cleanser if they don't have it
	if ( !QuestIsInactive( pQuest ) )
	{

		if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
		{
		
			// do they have the cleanser weapon
			DWORD dwInventoryIterateFlagsCleanserCheck = 0;  
			SETBIT( dwInventoryIterateFlagsCleanserCheck, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
			BOOL bHasCleanser = s_QuestCheckForItem( pPlayer, pQuestData->nItemCleanser, dwInventoryIterateFlagsCleanserCheck );	

			// give them the cleanser if they've never got it before
			if (QuestStateGetValue( pQuest, QUEST_STATE_CLEANUP_GETWEAPON ) == QUEST_STATE_ACTIVE ||
				bHasCleanser == FALSE)
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_CLEANUP_GETWEAPON );
				return QMR_NPC_TALK;
			}
			return QMR_OK;
		}
		
	}
	
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasWeapon(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_CLEANUP_GETWEAPON, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_CLEANUP_TRAVEL, QUEST_STATE_ACTIVE );
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
		case DIALOG_CLEANUP_INTRO:
		{
			if ( s_QuestActivate( pQuest ) )
			{
				s_QuestMapReveal( pQuest, GI_LEVEL_MANSION_HOUSE );
				s_QuestMapReveal( pQuest, GI_LEVEL_CANNON_STREET );
				s_QuestMapReveal( pQuest, GI_LEVEL_ANGEL_PASSAGE );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CLEANUP_GETWEAPON:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasWeapon;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTCLEANUPOFFER, tActions );

			return QMR_OFFERING;
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

	QUEST_DATA_CLEANUP * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelAngelPassage )
	{
		QuestStateSet( pQuest, QUEST_STATE_CLEANUP_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_CLEANUP_CLEANSE, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_CLEANUP * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// turn taint into interactive NPC if player doesn't have leg already
	if ( UnitIsMonsterClass( pVictim, pQuestData->nFuruncle ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_CLEANUP_CLEANSE, QUEST_STATE_COMPLETE );
		s_QuestComplete( pQuest );
		s_QuestTruthCStart( pQuest );
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
void QuestFreeCleanup(
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
	QUEST_DATA_CLEANUP * pQuestData)
{
	pQuestData->nRorke				= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nSerSing			= QuestUseGI( pQuest, GI_MONSTER_SER_SING );
	pQuestData->nLevelAngelPassage	= QuestUseGI( pQuest, GI_LEVEL_ANGEL_PASSAGE );
	pQuestData->nFuruncle			= QuestUseGI( pQuest, GI_MONSTER_FURUNCLE );
	pQuestData->nItemCleanser		= QuestUseGI( pQuest, GI_ITEM_CLEANSER );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitCleanup(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_CLEANUP * pQuestData = ( QUEST_DATA_CLEANUP * )GMALLOCZ( pGame, sizeof( QUEST_DATA_CLEANUP ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nSerSing );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreCleanup(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
