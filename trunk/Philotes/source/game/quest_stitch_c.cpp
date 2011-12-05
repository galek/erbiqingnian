//----------------------------------------------------------------------------
// FILE: quest_stitch_c.cpp
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
#include "quest_stitch_c.h"
#include "quest_chocolate.h"
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

struct QUEST_DATA_STITCH_C
{
	int		nLucious;
	int		nTech314;
	int		nLordArphaun;

	int		nLevelGPS;
	int		nLevelDeathsSewers;
	int		nLevelDeathsTunnels;
	int		nLevelDeathsCity;
	int		nAltarPain;
	int		nAltarHorror;
	int		nAltarDespair;
	
	int		nItemOracleHead;
	int		nItemOraclePartA;
	int		nItemOraclePartB;
	
	int		nDRLGMiniCity;
	int		nDRLGSewers;
	
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_STITCH_C * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_STITCH_C *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	QUEST_DATA_STITCH_C * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_C_TALK_LUCIOUS3 ) == QUEST_STATE_ACTIVE ) )
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
	{
		return QMR_OK;
	}

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );
	QUEST_DATA_STITCH_C * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestCanBeActivated( pQuest ) )
			return QMR_OK;

		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && UnitIsMonsterClass( pSubject,  pQuestData->nLucious ) )
		{
			// quest was abandoned. completing this dialog will reactive this
			// quest, from the "talk stopped" handler in the previous quest
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_GPS3 );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject,  pQuestData->nLucious ))
	{
		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_C_TALK_LUCIOUS3 ) == QUEST_STATE_ACTIVE ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_GPS4 );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
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
		case DIALOG_STITCH_LUCIOUS_GPS4:
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_C_TALK_LUCIOUS3, QUEST_STATE_COMPLETE );
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
	{
		return QMR_OK;
	}

	QUEST_DATA_STITCH_C * pQuestData = sQuestDataGet( pQuest );
	if ( pMessage->nLevelNewDef == pQuestData->nLevelGPS )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_C_RETURN_GPS3 ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_C_RETURN_GPS3, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_STITCH_C_TALK_LUCIOUS3, QUEST_STATE_ACTIVE );
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
	QUEST_DATA_STITCH_C *pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nAltarDespair ) )
	{
		return ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_C_FIND_DESPAIR ) == QUEST_STATE_ACTIVE ) ?
			QMR_OPERATE_OK :
			QMR_OPERATE_FORBIDDEN;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOracleTaken(
	void *pUserData )

{
	QUEST *pQuest = (QUEST *)pUserData;
	
	QuestStateSet( pQuest, QUEST_STATE_STITCH_C_FIND_DESPAIR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_C_USE_DESPAIR, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_C_USE_DESPAIR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_C_RETURN_GPS3, QUEST_STATE_ACTIVE );
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

	QUEST_DATA_STITCH_C * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nAltarDespair )
	{
		s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );

		// remove oracle pieces
		s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nItemOracleHead );
		s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nItemOraclePartA );
		s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nItemOraclePartB );

		// setup actions
		OFFER_ACTIONS tActions;
		tActions.pfnAllTaken = sOracleTaken;
		tActions.pAllTakenUserData = pQuest;

		// offer to player
		s_OfferGive( pQuest->pPlayer, pTarget, OFFERID_QUEST_STITCH_ORACLE_COMPLETE, tActions );

	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_STITCH_C * pData = sQuestDataGet( pQuest );

	if ( pData->nLevelDeathsSewers == pMessage->nLevelDefId )
	{
		if ( PStrCmp( pMessage->szRuleName, "SR_Rule_05", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
			return QMR_DRLG_RULE_FORBIDDEN;
	}
	else if ( pData->nLevelDeathsCity == pMessage->nLevelDefId )
	{
		if ( PStrCmp( pMessage->szRuleName, "Rule_mc03", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
			return QMR_DRLG_RULE_FORBIDDEN;

	}
	else if ( pMessage->nDRLGDefId == pData->nDRLGMiniCity )
	{
		if ( PStrCmp( pMessage->szRuleName, "Rule_mc03_altar", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
			return QMR_DRLG_RULE_FORBIDDEN;
	}
	else if ( pMessage->nDRLGDefId == pData->nDRLGSewers )
	{
		if ( PStrCmp( pMessage->szRuleName, "SR_Rule_10", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
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

		//----------------------------------------------------------------------------
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeStitchC(
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
	QUEST_DATA_STITCH_C * pQuestData)
{
	pQuestData->nLucious			= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nTech314			= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314 );
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelGPS			= QuestUseGI( pQuest, GI_LEVEL_GREEN_PARK_STATION );
	pQuestData->nLevelDeathsSewers	= QuestUseGI( pQuest, GI_LEVEL_DEATHS_SEWERS );
	pQuestData->nLevelDeathsTunnels = QuestUseGI( pQuest, GI_LEVEL_DEATHS_TUNNELS );
	pQuestData->nLevelDeathsCity	= QuestUseGI( pQuest, GI_LEVEL_DEATHS_CITY );
	pQuestData->nAltarPain			= QuestUseGI( pQuest, GI_OBJECT_ALTAR_PAIN );
	pQuestData->nAltarHorror		= QuestUseGI( pQuest, GI_OBJECT_ALTAR_HORROR );
	pQuestData->nAltarDespair		= QuestUseGI( pQuest, GI_OBJECT_ALTAR_DESPAIR );
	pQuestData->nItemOracleHead		= QuestUseGI( pQuest, GI_ITEM_ORACLE_HEAD );
	pQuestData->nItemOraclePartA	= QuestUseGI( pQuest, GI_ITEM_ORACLE_PART_A );
	pQuestData->nItemOraclePartB	= QuestUseGI( pQuest, GI_ITEM_ORACLE_PART_B );
	pQuestData->nDRLGMiniCity		= QuestUseGI( pQuest, GI_DRLG_MINICITY );
	pQuestData->nDRLGSewers			= QuestUseGI( pQuest, GI_DRLG_SEWERS );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitStitchC(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_STITCH_C * pQuestData = ( QUEST_DATA_STITCH_C * )GMALLOCZ( pGame, sizeof( QUEST_DATA_STITCH_C ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLucious );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreStitch_c(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestActivateStitchC(
	UNIT *pPlayer)
{
	QUEST * pQuest = QuestGetQuest( pPlayer, QUEST_STITCH_C );
	ASSERT_RETURN( pQuest );
	s_QuestActivate( pQuest );
}