//----------------------------------------------------------------------------
// FILE: quest_stitch_b.cpp
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
#include "quest_stitch_a.h"
#include "quest_stitch_b.h"
#include "quest_stitch_c.h"
#include "quest_chocolate.h"
#include "quest_collect.h"
#include "quests.h"
#include "Quest.h"
#include "quest_template.h"
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

struct QUEST_DATA_STITCH_B
{
	int		nLucious;
	int		nTech314;
	int		nLordArphaun;

	int		nLevelGPS;
	int		nLevelDeathsCity;
	int		nAltarHorror;
	
	int		nDRLGMiniCity;
	int		nDRLGSewers;

	QUEST_DATA_COLLECT tCollect;	// data for shared quest_collect code
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_STITCH_B * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_STITCH_B *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_COLLECT * sQuestDataGetCollect( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_STITCH_B *pData = sQuestDataGet( pQuest );
	ASSERT_RETNULL( pData );
	return &pData->tCollect;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest, 
	QUEST_DATA_STITCH_B * pQuestData)
{
	pQuestData->nLucious			= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nTech314			= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314 );
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelGPS			= QuestUseGI( pQuest, GI_LEVEL_GREEN_PARK_STATION );
	pQuestData->nLevelDeathsCity	= QuestUseGI( pQuest, GI_LEVEL_DEATHS_CITY );
	pQuestData->nAltarHorror		= QuestUseGI( pQuest, GI_OBJECT_ALTAR_HORROR );
	pQuestData->nDRLGMiniCity		= QuestUseGI( pQuest, GI_DRLG_MINICITY );
	pQuestData->nDRLGSewers			= QuestUseGI( pQuest, GI_DRLG_SEWERS );

	QuestCollectDataInit( pQuest, &pQuestData->tCollect );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	QUEST_DATA_STITCH_B * pQuestData = sQuestDataGet( pQuest );
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
		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_RETURN_GPS2 ) == QUEST_STATE_ACTIVE ) )
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
	QUEST_DATA_STITCH_B * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestCanBeActivated( pQuest ) )
			return QMR_OK;

		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && UnitIsMonsterClass( pSubject,  pQuestData->nLucious ) )
		{
			// quest was abandoned. completing this dialog will reactive this
			// quest, from the "talk stopped" handler in the previous quest
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_GPS2 );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject,  pQuestData->nLucious ))
	{
		if ( ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelGPS ) && ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_RETURN_GPS2 ) == QUEST_STATE_ACTIVE ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_GPS3 );
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;

	switch( nDialogStopped )
	{

		//----------------------------------------------------------------------------
		case DIALOG_STITCH_LUCIOUS_GPS3:
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_B_RETURN_GPS2, QUEST_STATE_COMPLETE );
			if ( s_QuestComplete( pQuest ) )
				s_QuestActivateStitchC( QuestGetPlayer( pQuest ) );
			break;
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
	QUEST_DATA_STITCH_B *pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nAltarHorror ) )
	{
		return ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_FIND_HORROR ) == QUEST_STATE_ACTIVE ) ?
			QMR_OPERATE_OK :
			QMR_OPERATE_FORBIDDEN;
	}

	return QMR_OK;

}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOraclePartBTaken(
	void *pUserData )
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_STITCH_B_FIND_HORROR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_B_USE_HORROR, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_B_USE_HORROR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_B_RETURN_GPS2, QUEST_STATE_ACTIVE );
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

	QUEST_DATA_STITCH_B * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nAltarHorror )
	{
		s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );
		// remove collection items
		s_QuestRemoveQuestItems( pQuest->pPlayer, pQuest );

		// setup actions
		OFFER_ACTIONS tActions;
		tActions.pfnAllTaken = sOraclePartBTaken;
		tActions.pAllTakenUserData = pQuest;

		// offer to player
		s_OfferGive( pQuest->pPlayer, pTarget, OFFERID_QUEST_STITCH_B_ORACLE_PART, tActions );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_STITCH_B * pData = sQuestDataGet( pQuest );

	if ( pData->nLevelDeathsCity == pMessage->nLevelDefId )
	{
		if ( PStrCmp( pMessage->szRuleName, "Rule_mc03", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
			return QMR_DRLG_RULE_FORBIDDEN;

	}
	else if ( pMessage->nDRLGDefId == pData->nDRLGMiniCity )
	{
		if ( PStrCmp( pMessage->szRuleName, "Rule_mc03_altar", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
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
		case QM_SERVER_CREATE_LEVEL:
		{
			const QUEST_MESSAGE_CREATE_LEVEL * pMessageCreateLevel = ( const QUEST_MESSAGE_CREATE_LEVEL * )pMessage;
			return s_QuestCollectMessageCreateLevel( pQuest, pMessageCreateLevel, sQuestDataGetCollect( pQuest ) );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;

			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageEnterLevel( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest, 
							pMessageEnterLevel, 
							sQuestDataGetCollect( pQuest ) );

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ROOMS_ACTIVATED:
		{
			// "message data" is in the quest globals
			// check if a spawn room was activated, spawn baddies
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageRoomsActivated( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest,
							sQuestDataGetCollect( pQuest ) );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessagePartyKill( pQuest, pPartyKillMessage, sQuestDataGetCollect( pQuest ) );
			return QMR_OK;
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

		//----------------------------------------------------------------------------
		case QM_CLIENT_ATTEMPTING_PICKUP:
		case QM_SERVER_ATTEMPTING_PICKUP: // check client and server, because of latency issues
		{
			const QUEST_MESSAGE_ATTEMPTING_PICKUP *pMessagePickup = (const QUEST_MESSAGE_ATTEMPTING_PICKUP*)pMessage;
			// if the item was spawned for this quest, but the collection state is not active, disallow pickup
			if ( pMessagePickup->nQuestIndex == pQuest->nQuest &&
				QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_COLLECT) != QUEST_STATE_ACTIVE )
			{
				return QMR_PICKUP_FAIL;
			}

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_QuestStitchMessagePickup( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessagePickup);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				UnitClearStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, StatParam( STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) );
			}
			// fall through

		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return QuestTemplateMessageStatus( pQuest, pMessageStatus );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeStitchB(
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
void QuestInitStitchB(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_STITCH_B * pQuestData = ( QUEST_DATA_STITCH_B * )GMALLOCZ( pGame, sizeof( QUEST_DATA_STITCH_B ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// this quest requires monster drops, so party kill message on
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLucious );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestOnEnterGameStitchB(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pQuest->pPlayer, QUEST_STATUS_INVALID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestActivateStitchB(
	UNIT *pPlayer)
{
	QUEST * pQuest = QuestGetQuest( pPlayer, QUEST_STITCH_B );
	ASSERT_RETURN( pQuest );
	s_QuestActivate( pQuest );
}
