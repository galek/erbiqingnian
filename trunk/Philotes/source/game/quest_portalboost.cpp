//----------------------------------------------------------------------------
// FILE: quest_portalboost.cpp
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
#include "quest_portalboost.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "treasure.h"
#include "items.h"
#include "quest_template.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_PORTALBOOST
{
	int		nArphaun;

	int		nTechsmith415;
	int		nAlec;

	int		nLevelAldgate;

	int		nPowerSupplyRRG;
	int		nPowerSupplyRGB;
	int		nPowerSupplyGBB;
	int		nPowerSupplyRBB;
	int		nPowerSupplyRGG;

	int		nGlandTreasureClass;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_PORTALBOOST * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_PORTALBOOST *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsQuestClassNPC(
	QUEST * pQuest,
	UNIT * npc )
{
	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );
	return UnitIsMonsterClass( npc, pQuestData->nArphaun );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckPowerSuppliesComplete(
	QUEST * pQuest )
{
	ASSERT_RETFALSE( pQuest );
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETFALSE( player );
	ASSERT_RETFALSE( IS_SERVER( UnitGetGame( player ) ) );

	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );

	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );

	int nPowerSupplyCount = 0;
	UNIT * item = UnitInventoryIterate( player, NULL, dwFlags );
	while ( item != NULL )
	{
		int item_class = UnitGetClass( item );
		int count = ItemGetQuantity( item );

		if ( item_class == pQuestData->nPowerSupplyRRG )
			nPowerSupplyCount += count;
		else if ( item_class == pQuestData->nPowerSupplyRGB )
			nPowerSupplyCount += count;
		else if ( item_class == pQuestData->nPowerSupplyGBB )
			nPowerSupplyCount += count;
		else if ( item_class == pQuestData->nPowerSupplyRBB )
			nPowerSupplyCount += count;
		else if ( item_class == pQuestData->nPowerSupplyRGG )
			nPowerSupplyCount += count;

		item = UnitInventoryIterate( player, item, dwFlags );
	}

	// should be impossible to have more than 3
	ASSERT_RETFALSE( nPowerSupplyCount <= 3 );

	if ( nPowerSupplyCount == 3 )
		return TRUE;

	return FALSE;
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

	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nTechsmith415 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TALKTECH ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_MAKEPS ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_INFO_WAITING;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TURNIN ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nAlec ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TALKALEC ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( sIsQuestClassNPC( pQuest, pSubject ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_PORTALBOOST_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nTechsmith415 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TALKTECH ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_PORTALBOOST_TECH );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TURNIN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_PORTALBOOST_TECH_FINISHED );
			return QMR_NPC_TALK;
		}
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nAlec ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_TALKALEC ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_PORTALBOOST_ALEC );
			return QMR_NPC_TALK;
		}
	}

	if ( sIsQuestClassNPC( pQuest, pSubject ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_PORTALBOOST_COMPLETE );
			return QMR_NPC_TALK;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasBlueprints(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TALKTECH, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TALKALEC, QUEST_STATE_ACTIVE );
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
		case DIALOG_PORTALBOOST_INTRO:
		{
			s_QuestActivate( pQuest );
			s_QuestMapReveal( pQuest, GI_LEVEL_FENCHURCH );
			s_QuestMapReveal( pQuest, GI_LEVEL_TRINITY_SQUARE );
			s_QuestMapReveal( pQuest, GI_LEVEL_ALDGATE );
			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_PORTALBOOST_TECH:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasBlueprints;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTPORTALBOOSTOFFER, tActions );
			return QMR_OFFERING;
		}

		//----------------------------------------------------------------------------
		case DIALOG_PORTALBOOST_ALEC:
		{
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TALKALEC, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_MAKEPS, QUEST_STATE_ACTIVE );
			// set flag on player
			QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pQuest->pPlayer, QUEST_STATUS_INVALID );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_PORTALBOOST_TECH_FINISHED:
		{
			// check one last time...
			ASSERT_RETVAL( sCheckPowerSuppliesComplete( pQuest ), QMR_OK );

			// remove the power supplies
			UNIT * pPlayer = pQuest->pPlayer;
			QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );
			s_QuestRemoveItem( pPlayer, pQuestData->nPowerSupplyRRG );
			s_QuestRemoveItem( pPlayer, pQuestData->nPowerSupplyRGB );
			s_QuestRemoveItem( pPlayer, pQuestData->nPowerSupplyGBB );
			s_QuestRemoveItem( pPlayer, pQuestData->nPowerSupplyRBB );
			s_QuestRemoveItem( pPlayer, pQuestData->nPowerSupplyRGG );

			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TURNIN, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_RETURN, QUEST_STATE_ACTIVE );

			s_NPCEmoteStart( UnitGetGame( pPlayer ), pPlayer, DIALOG_PORTALBOOST_TECH_EMOTE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_PORTALBOOST_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_RETURN, QUEST_STATE_COMPLETE );
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

	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelAldgate )
	{
		QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TALKTECH, QUEST_STATE_ACTIVE );
		// set flag on player
		QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pQuest->pPlayer, QUEST_STATUS_INVALID );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	ASSERT_RETVAL( pQuest, QMR_OK );
	ASSERT_RETVAL( pMessage, QMR_OK );

	QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );

	if ( UnitGetLevelDefinitionIndex( pMessage->pVictim ) != pQuestData->nLevelAldgate )
		return QMR_OK;

	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );

	//spawn item			
	if( pQuest->pQuestDef->fCollectDropRate > 0.0f && 
		RandGetFloat( pGame ) <= pQuest->pQuestDef->fCollectDropRate )
	{
		// setup buffer to hold all the items generated by treasure class
		UNIT * pItems[ MAX_QUEST_GIVEN_ITEMS ];	

		// setup spawn result
		ITEMSPAWN_RESULT tResult;
		tResult.pTreasureBuffer = pItems;
		tResult.nTreasureBufferSize = MAX_QUEST_GIVEN_ITEMS;

		// set item spec
		ITEM_SPEC tSpec;
		tSpec.unitOperator = pPlayer;
		tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
		SETBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
		SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
		s_TreasureSpawnClass( pMessage->pVictim, pQuestData->nGlandTreasureClass, &tSpec, &tResult );
		ASSERTV_RETVAL( tResult.nTreasureBufferCount > 0, QMR_OK, "quest \"%s\": No items generated for quest to give from treasure class %i", pQuest->pQuestDef->szName, pQuestData->nGlandTreasureClass );
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemPickedUpOrCrafted(
	QUEST *pQuest,
	UNIT *pItem)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( pItem, "Expected unit" );
	
	if ( !QuestIsActive( pQuest ) )
		return;

	if ( QuestStateGetValue( pQuest, QUEST_STATE_PORTALBOOST_MAKEPS ) == QUEST_STATE_ACTIVE )
	{
		BOOL bContinue = FALSE;
		int item_class = UnitGetClass( pItem );
		QUEST_DATA_PORTALBOOST * pQuestData = sQuestDataGet( pQuest );
		if ( item_class == pQuestData->nPowerSupplyRRG )
			bContinue = TRUE;
		if ( item_class == pQuestData->nPowerSupplyRGB )
			bContinue = TRUE;
		if ( item_class == pQuestData->nPowerSupplyGBB )
			bContinue = TRUE;
		if ( item_class == pQuestData->nPowerSupplyRBB )
			bContinue = TRUE;
		if ( item_class == pQuestData->nPowerSupplyRGG )
			bContinue = TRUE;
		if ( bContinue && sCheckPowerSuppliesComplete( pQuest ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_MAKEPS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_PORTALBOOST_TURNIN, QUEST_STATE_ACTIVE );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessagePickup( 
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	UNIT *pItem = pMessage->pPickedUp;
	if (pItem)
	{
		sItemPickedUpOrCrafted( pQuest, pItem );
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageItemCrafted( 
	QUEST* pQuest,
	const QUEST_MESSAGE_ITEM_CRAFTED *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItemCrafted );
	if (pItem)
	{
		sItemPickedUpOrCrafted( pQuest, pItem );
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
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			return s_sQuestMessagePartyKill( pQuest, pPartyKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestMessagePickup( pQuest, pMessagePickup);
		}
		
		//----------------------------------------------------------------------------
		case QM_SERVER_ITEM_CRAFTED:
		{
			const QUEST_MESSAGE_ITEM_CRAFTED *pMessageCrafted = (const QUEST_MESSAGE_ITEM_CRAFTED *)pMessage;
			return s_sQuestMessageItemCrafted( pQuest, pMessageCrafted );		
		}
		
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreePortalBoost(
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
	QUEST_DATA_PORTALBOOST * pQuestData)
{
	pQuestData->nArphaun						= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nTechsmith415					= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_415 );
	pQuestData->nAlec							= QuestUseGI( pQuest, GI_MONSTER_ALEC );
	pQuestData->nLevelAldgate					= QuestUseGI( pQuest, GI_LEVEL_ALDGATE );
	pQuestData->nPowerSupplyRRG 				= QuestUseGI( pQuest, GI_ITEM_POWERSUPPLY_RRG );
	pQuestData->nPowerSupplyRGB 				= QuestUseGI( pQuest, GI_ITEM_POWERSUPPLY_RGB );
	pQuestData->nPowerSupplyGBB 				= QuestUseGI( pQuest, GI_ITEM_POWERSUPPLY_GBB );
	pQuestData->nPowerSupplyRBB 				= QuestUseGI( pQuest, GI_ITEM_POWERSUPPLY_RBB );
	pQuestData->nPowerSupplyRGG 				= QuestUseGI( pQuest, GI_ITEM_POWERSUPPLY_RGG );
	pQuestData->nGlandTreasureClass				= QuestUseGI( pQuest, GI_TREASURE_PORTALBOOST_GLAND );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitPortalBoost(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_PORTALBOOST * pQuestData = ( QUEST_DATA_PORTALBOOST * )GMALLOCZ( pGame, sizeof( QUEST_DATA_PORTALBOOST ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// this quest requires monster drops, so party kill message on
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nTechsmith415 );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nAlec );
}
