//----------------------------------------------------------------------------
// FILE: quest_stitch_a.cpp
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

struct QUEST_DATA_STITCH_A
{
	int		nLucious;
	int		nTech314;
	int		nLordArphaun;

	int		nLevelGPS;
	int		nLevelDeathsSewers;
	int		nLevelCCStation;

	int		nAltarPain;
	
	int		nObjectWarpCGS_CGA;
	int		nObjectWarpGPS_DS;
		
	int		nDRLGMiniCity;
	int		nDRLGSewers;

	QUEST_DATA_COLLECT tCollect;	// data for shared quest_collect code
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_STITCH_A * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_STITCH_A *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_COLLECT * sQuestDataGetCollect( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	QUEST_DATA_STITCH_A *pData = sQuestDataGet( pQuest );
	ASSERT_RETNULL( pData );
	return &pData->tCollect;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
						   QUEST *pQuest, 
						   QUEST_DATA_STITCH_A * pQuestData)
{
	pQuestData->nLucious			= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nTech314			= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314 );
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelGPS			= QuestUseGI( pQuest, GI_LEVEL_GREEN_PARK_STATION );
	pQuestData->nLevelDeathsSewers	= QuestUseGI( pQuest, GI_LEVEL_DEATHS_SEWERS );
	pQuestData->nLevelCCStation		= QuestUseGI( pQuest, GI_LEVEL_CHARING_CROSS_STATION );
	pQuestData->nAltarPain			= QuestUseGI( pQuest, GI_OBJECT_ALTAR_PAIN );
	pQuestData->nObjectWarpCGS_CGA	= QuestUseGI( pQuest, GI_OBJECT_WARP_CCSTATION_GPAPPROACH );
	pQuestData->nObjectWarpGPS_DS	= QuestUseGI( pQuest, GI_OBJECT_WARP_GPS_DS );
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
	QUEST_DATA_STITCH_A * pQuestData = sQuestDataGet( pQuest );
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
		int nLevelIndex = UnitGetLevelDefinitionIndex( pSubject );
		if ( nLevelIndex == pQuestData->nLevelCCStation )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
		}
		else if ( nLevelIndex == pQuestData->nLevelGPS )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS2 ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_RETURN_GPS ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
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
	QUEST_DATA_STITCH_A * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_STITCH_ARPHAUN_START );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject,  pQuestData->nLucious ))
	{
		int nLevelIndex = UnitGetLevelDefinitionIndex( pSubject );
		if ( nLevelIndex == pQuestData->nLevelCCStation )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_START, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
				return QMR_NPC_TALK;
			}
		}
		else if ( nLevelIndex == pQuestData->nLevelGPS )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS2 ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_TWO_OK, DIALOG_STITCH_LUCIOUS_GPS1, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
				return QMR_NPC_TALK;
			}
			else if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_RETURN_GPS ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_STITCH_LUCIOUS_GPS2 );
				return QMR_NPC_TALK;
			}
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
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_STITCH_A *pQuestData = sQuestDataGet( pQuest );

	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_STITCH_ARPHAUN_START:
		{
			if ( s_QuestActivate( pQuest ) )
			{
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_STITCH_LUCIOUS_START:
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_STITCH_A_TRAVEL_GPS, QUEST_STATE_ACTIVE );
			s_QuestMapReveal( pQuest, GI_LEVEL_GREEN_PARK_STATION );
			// force open portal
			s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nObjectWarpCGS_CGA );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_STITCH_LUCIOUS_GPS1:
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS2, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_STITCH_A_COLLECT, QUEST_STATE_ACTIVE );
			// force open portal
			s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nObjectWarpGPS_DS );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_STITCH_LUCIOUS_GPS2:
		{
			QuestStateSet( pQuest, QUEST_STATE_STITCH_A_RETURN_GPS, QUEST_STATE_COMPLETE );
			if ( s_QuestComplete( pQuest ) )
				s_QuestActivateStitchB( pPlayer );
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
	QUEST_DATA_STITCH_A *pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpCGS_CGA ) )
	{	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TRAVEL_GPS ) >= QUEST_STATE_ACTIVE )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
		}
	}

	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpGPS_DS ) )
	{	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT ) >= QUEST_STATE_ACTIVE )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
		}
	}

	if ( UnitIsObjectClass( pObject, pQuestData->nAltarPain ) )
	{
		 return ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_FIND_PAIN ) == QUEST_STATE_ACTIVE ) ?
			QMR_OPERATE_OK :
			QMR_OPERATE_FORBIDDEN;
	}

	return QMR_OK;

}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOraclePartATaken(
	void *pUserData )
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_STITCH_A_FIND_PAIN, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_A_USE_PAIN, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_A_USE_PAIN, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_STITCH_A_RETURN_GPS, QUEST_STATE_ACTIVE );
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

	QUEST_DATA_STITCH_A * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nAltarPain )
	{
		s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );

		// remove collection items
		s_QuestRemoveQuestItems( pQuest->pPlayer, pQuest );

		// setup actions
		OFFER_ACTIONS tActions;
		tActions.pfnAllTaken = sOraclePartATaken;
		tActions.pAllTakenUserData = pQuest;

		// offer to player
		s_OfferGive( pQuest->pPlayer, pTarget, OFFERID_QUEST_STITCH_A_ORACLE_PART, tActions );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_STITCH_A * pData = sQuestDataGet( pQuest );

	if ( pData->nLevelDeathsSewers == pMessage->nLevelDefId )
	{
		if ( PStrCmp( pMessage->szRuleName, "SR_Rule_05", CAN_RUN_DRLG_RULE_NAME_SIZE ) == 0 )
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
QUEST_MESSAGE_RESULT s_QuestStitchMessagePickup( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	UNIT* pItem = pMessage->pPickedUp;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );

	// check for completing the collection
	if ( ( ( pQuest->nQuest == QUEST_STITCH_A && 
		     QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT) == QUEST_STATE_ACTIVE ) ||
		   ( pQuest->nQuest == QUEST_STITCH_B && 
			   QuestStateGetValue( pQuest, QUEST_STATE_STITCH_B_COLLECT) == QUEST_STATE_ACTIVE ) ) &&
		 UnitIsA( pItem, UNITTYPE_ITEM ) &&
		 pQuest->pQuestDef->nCollectItem == UnitGetClass( pItem ))	
	{
		int nNumCollected = UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ));
		++nNumCollected;
		UnitSetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nNumCollected );
		if ( nNumCollected >= pQuest->pQuestDef->nObjectiveCount ) // TODO cmarch multiple item classes?
		{
			if ( pQuest->nQuest == QUEST_STITCH_A)
			{
				QuestStateSet( pQuest, QUEST_STATE_STITCH_A_COLLECT, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_STITCH_A_FIND_PAIN, QUEST_STATE_ACTIVE );
			}
			else if ( pQuest->nQuest == QUEST_STITCH_B)
			{
				QuestStateSet( pQuest, QUEST_STATE_STITCH_B_COLLECT, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_STITCH_B_FIND_HORROR, QUEST_STATE_ACTIVE );
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
		case QM_SERVER_CREATE_LEVEL:
		{
			const QUEST_MESSAGE_CREATE_LEVEL * pMessageCreateLevel = ( const QUEST_MESSAGE_CREATE_LEVEL * )pMessage;
			return s_QuestCollectMessageCreateLevel( pQuest, pMessageCreateLevel, sQuestDataGetCollect( pQuest ) );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;

			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				QUEST_DATA_STITCH_A * pQuestData = sQuestDataGet( pQuest );
				if ( pMessageEnterLevel->nLevelNewDef == pQuestData->nLevelGPS )
				{
					if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_TRAVEL_GPS ) == QUEST_STATE_ACTIVE )
					{
						QuestStateSet( pQuest, QUEST_STATE_STITCH_A_TRAVEL_GPS, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_STITCH_A_TALK_LUCIOUS2, QUEST_STATE_ACTIVE );
					}
				}
			}

			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT ) == QUEST_STATE_ACTIVE )
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
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT ) == QUEST_STATE_ACTIVE )
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
			if ( QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT ) == QUEST_STATE_ACTIVE )
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
				 QuestStateGetValue( pQuest, QUEST_STATE_STITCH_A_COLLECT) != QUEST_STATE_ACTIVE )
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
void QuestFreeStitchA(
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
void QuestInitStitchA(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_STITCH_A * pQuestData = ( QUEST_DATA_STITCH_A * )GMALLOCZ( pGame, sizeof( QUEST_DATA_STITCH_A ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// this quest requires monster drops, so party kill message on
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLucious );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestOnEnterGameStitchA(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pQuest->pPlayer, QUEST_STATUS_INVALID );
}
