//----------------------------------------------------------------------------
// FILE: quest_hellyard.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "prime.h"
#include "c_ui.h"
#include "clients.h"
#include "combat.h"
#include "fontcolor.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "objects.h"
#include "offer.h"
#include "player.h"
#include "quest_hellyard.h"
#include "quests.h"
#include "dialog.h"
#include "uix.h"
#include "items.h"
#include "c_quests.h"
#include "s_quests.h"
#include "treasure.h"
#include "level.h"
#include "colors.h"
#include "c_font.h"
#include "room_layout.h"
#include "states.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_HELLRIFT_LOCATIONS	32
#define HELLRIFT_BOMB_FUSE		30			// (in seconds)
#define HELLRIFT_RESET_DELAY_INITIAL	(GAME_TICKS_PER_SECOND * 60 * 2)
#define HELLRIFT_RESET_DELAY			(GAME_TICKS_PER_SECOND * 10)
	
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_HELLYARD
{

	int		nHellriftLevel;

	int		nCGStation;
	int		nHolbornStation;

	int		nMurmur;
	int		nDrFawkes;

	int		nBrandonLann;
	
	int		nItemBoxArtifacts;
	int		nItemFawkesRemains;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_HELLYARD * sQuestDataGet(
	QUEST * pQuest)
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return ( QUEST_DATA_HELLYARD * )pQuest->pQuestData;
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
	QUEST_DATA_HELLYARD * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELLYARD_MURMUR );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nDrFawkes ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLYARD_FIND_FAWKES ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_HELLYARD_FAWKES );
			return QMR_NPC_TALK;
		}
	}
	
	if (UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLYARD_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELLYARD_TEMPLAR );
			return QMR_NPC_TALK;
		}
	}

	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasFawkes(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;
	s_QuestAdvanceTo( pQuest, QUEST_STATE_HELLYARD_RETURN );
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	QUEST_DATA_HELLYARD *pQuestData = sQuestDataGet( pQuest );
		
	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_HELLYARD_MURMUR:
		{
			if ( s_QuestActivate( pQuest ) )
			{
				// take no-drop box
				UNIT * pPlayer = pQuest->pPlayer;
				s_QuestRemoveItem( pPlayer, pQuestData->nItemBoxArtifacts );
				s_QuestMapReveal( pQuest, GI_LEVEL_COVENT_GARDEN_MARKET );
			}
			break;			
		}

		//----------------------------------------------------------------------------
		case DIALOG_HELLYARD_FAWKES:
		{

			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasFawkes;
			tActions.pAllTakenUserData = pQuest;

			// do offer
			s_OfferGive( pPlayer, pTalkingTo, OFFERID_QUEST_HELLYARD_FAWKES, tActions );
			return QMR_OFFERING;

		}

		//----------------------------------------------------------------------------
		case DIALOG_HELLYARD_TEMPLAR:
		{
			QuestStateSet( pQuest, QUEST_STATE_HELLYARD_RETURN, QUEST_STATE_COMPLETE );
			// take fawkes remains
			s_QuestRemoveItem( pPlayer, pQuestData->nItemFawkesRemains );
			s_QuestComplete( pQuest );
			// have npc rummage through the crap and delay the start of test monkey
			s_NPCEmoteStart( UnitGetGame( pPlayer ), pPlayer, DIALOG_HELLYARD_TEMPLAR_EMOTE );
			s_QuestDelayNext( pQuest, 30 * GAME_TICKS_PER_SECOND );
		}
	}

	return QMR_OK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{

	if (QuestIsActive( pQuest ))
	{
		QUEST_DATA_HELLYARD * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );
		
		int nHellriftLevel = UnitGetLevelDefinitionIndex( pTarget );
		if ( nHellriftLevel == pQuestData->nHellriftLevel )
		{
			if (UnitIsA( pTarget, UNITTYPE_HELLRIFT_PORTAL ))
			{
				if ( QuestStateSet( pQuest, QUEST_STATE_HELLYARD_INVESTIGATE, QUEST_STATE_COMPLETE ) )
				{
					QuestStateSet( pQuest, QUEST_STATE_HELLYARD_ENTER, QUEST_STATE_ACTIVE );
				}
			}

		}
		
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{

	if (QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE)
	{
		QUEST_DATA_HELLYARD * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = pMessage->pTarget;

		int nHellriftLevel = UnitGetLevelDefinitionIndex( pTarget );
		if ( nHellriftLevel == pQuestData->nHellriftLevel )
		{

			if (UnitIsA( pTarget, UNITTYPE_HELLRIFT_PORTAL ))
			{
				QuestStateSet( pQuest, QUEST_STATE_HELLYARD_ENTER, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_HELLYARD_FIND_FAWKES, QUEST_STATE_ACTIVE );
			}

		}
		
	}
	
	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( eStatus == QUEST_STATUS_ACTIVE )
	{
		QUEST_DATA_HELLYARD * pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if ( UnitIsMonsterClass( pSubject, pQuestData->nBrandonLann ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_HELLYARD_RETURN ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
			return QMR_OK;
		}

		if (UnitIsMonsterClass( pSubject, pQuestData->nDrFawkes ) && 
			QuestStateGetValue( pQuest, QUEST_STATE_HELLYARD_FIND_FAWKES ) != QUEST_STATE_COMPLETE)
		{
			return QMR_NPC_STORY_RETURN;
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
		QUEST_DATA_HELLYARD * pQuestData = sQuestDataGet( pQuest );
		if ( pMessage->nLevelNewDef == pQuestData->nHellriftLevel )
		{
			if ( QuestStateSet( pQuest, QUEST_STATE_HELLYARD_TRAVEL_CGM, QUEST_STATE_COMPLETE ) )
			{
				UNIT * player = pQuest->pPlayer;
				s_NPCVideoStart( UnitGetGame( player ), player, GI_MONSTER_DARK_TEMPLAR, NPC_VIDEO_INCOMING, DIALOG_HELLYARD_TRANSMISSION );
				QuestStateSet( pQuest, QUEST_STATE_HELLYARD_INVESTIGATE, QUEST_STATE_ACTIVE );
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
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
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
void QuestFreeHellyard(
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
static void sQuestDataHellyardInit(
	QUEST *pQuest,
	QUEST_DATA_HELLYARD *pQuestData)
{	
	pQuestData->nHellriftLevel		= QuestUseGI( pQuest, GI_LEVEL_COVENT_GARDEN_MARKET );
	pQuestData->nCGStation			= QuestUseGI( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );
	pQuestData->nHolbornStation		= QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
	pQuestData->nDrFawkes			= QuestUseGI( pQuest, GI_MONSTER_DR_FAWKES );
	pQuestData->nMurmur				= QuestUseGI( pQuest, GI_MONSTER_MURMUR );
	pQuestData->nBrandonLann		= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN );
	pQuestData->nItemBoxArtifacts	= QuestUseGI( pQuest, GI_ITEM_BOX_OF_ARTIFACTS );
	pQuestData->nItemFawkesRemains	= QuestUseGI( pQuest, GI_ITEM_FAWKES_REMAINS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitHellyard(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
		
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_HELLYARD * pQuestData = ( QUEST_DATA_HELLYARD * )GMALLOCZ( pGame, sizeof( QUEST_DATA_HELLYARD ) );
	sQuestDataHellyardInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDrFawkes );
	
	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}
