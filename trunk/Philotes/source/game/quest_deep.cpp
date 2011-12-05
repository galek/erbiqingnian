//----------------------------------------------------------------------------
// FILE: quest_deep.cpp
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
#include "quest_deep.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "states.h"
#include "room_path.h"
#include "ai.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_DEEP
{
	int		nLordArphaun;
	int		nJessicaSumerisle;
	int		nJessicaSumerisleCombat;
	int		nEmmera;

	int		nTheMarker;
	int		nMarkerPOI;

	int		nLevelLiverpoolStreetStation;
	int		nLevelNecropolisLevel1;
	int		nLevelNecropolisLevel5;

	int		nAIIdle;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_DEEP * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_DEEP *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnJessica(
	QUEST * pQuest )
{
	// Jessica already on level?
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );
	UNIT * pJessica = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nJessicaSumerisleCombat );
	if ( pJessica )
		return;

	ROOM * pRoom = UnitGetRoom( pPlayer );
	ROOM * pSpawnRoom = RoomGetFirstConnectedRoomOrSelf(pRoom);
	
	GAME * pGame = UnitGetGame( pPlayer );
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );

	// find ground location
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nJessicaSumerisleCombat;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 28 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	pJessica = s_MonsterSpawn( pGame, tSpec );
	ASSERT_RETURN( pJessica );
	UnitSetStat( pJessica, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pPlayer ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeJessicaAI( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * pJessica = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nJessicaSumerisleCombat );
	if ( !pJessica )
		return;

	// change the AI on Jessica
	if ( UnitGetStat( pJessica, STATS_AI ) != pQuestData->nAIIdle )
	{
		AI_Free( pGame, pJessica );
		UnitSetStat( pJessica, STATS_AI, pQuestData->nAIIdle );
		AI_Init( pGame, pJessica );

		// this NPC is now interactive
		UnitSetInteractive( pJessica, UNIT_INTERACTIVE_ENABLED );
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

	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nJessicaSumerisle ) ||
		UnitIsMonsterClass( pSubject, pQuestData->nJessicaSumerisleCombat ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_TALK_JESSICA ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}

		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_POST_ACTIVATE_TALK ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEEP_INTRO, INVALID_LINK, GI_MONSTER_EMMERA );
			return QMR_NPC_TALK;
		}
	}

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if (UnitIsMonsterClass( pSubject, pQuestData->nJessicaSumerisle ) ||
		UnitIsMonsterClass( pSubject, pQuestData->nJessicaSumerisleCombat ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_TALK_JESSICA ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEEP_JESSICA );
			return QMR_NPC_TALK;
		}

		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_POST_ACTIVATE_TALK ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEEP_JESSICA_POST_MARKER );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEEP_EMMERA_COMPLETE, INVALID_LINK, GI_MONSTER_LORD_ARPHAUN );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasGoggles(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_DEEP_TALK_JESSICA, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_DEEP_TRAVEL_NECRO, QUEST_STATE_ACTIVE );
	s_QuestMapReveal( pQuest, GI_LEVEL_PRIMROSE );
	s_QuestMapReveal( pQuest, GI_LEVEL_SHOREDITCH );
	s_QuestMapReveal( pQuest, GI_LEVEL_NECROPOLIS_LEVEL_1 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_DEEP_INTRO:
		{
			s_QuestActivate( pQuest );
			s_QuestMapReveal( pQuest, GI_LEVEL_MARK_LANE_APPROACH );
			s_QuestMapReveal( pQuest, GI_LEVEL_MARK_LANE_STATION );
			s_QuestMapReveal( pQuest, GI_LEVEL_LIVERPOOL_APPROACH );
			s_QuestMapReveal( pQuest, GI_LEVEL_LIVERPOOL_STREET_STATION );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DEEP_JESSICA:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasGoggles;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTTHEDEEPOFFER, tActions );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DEEP_JESSICA_POST_MARKER:
		{
			QuestStateSet( pQuest, QUEST_STATE_DEEP_POST_ACTIVATE_TALK, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_DEEP_RETURN, QUEST_STATE_ACTIVE );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DEEP_EMMERA_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_DEEP_RETURN, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			return QMR_FINISHED;
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

	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );

	// spawn jessica?
	static GLOBAL_INDEX sgLevelGIList[] =
	{
		GI_LEVEL_PRIMROSE,
		GI_LEVEL_SHOREDITCH,
		GI_LEVEL_NECROPOLIS_LEVEL_1,
		GI_LEVEL_NECROPOLIS_LEVEL_2,
		GI_LEVEL_NECROPOLIS_LEVEL_3,
		GI_LEVEL_NECROPOLIS_LEVEL_4,
		GI_LEVEL_NECROPOLIS_LEVEL_5,
		GI_INVALID,
	};
	for ( int i = 0; sgLevelGIList[i] != GI_INVALID; i++ )
	{
		if ( pMessage->nLevelNewDef == GlobalIndexGet( sgLevelGIList[i] ) )
		{
			sSpawnJessica( pQuest );
		}
	}

	if ( ( pMessage->nLevelNewDef == pQuestData->nLevelLiverpoolStreetStation ) &&
		 ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_TRAVEL_LSS ) == QUEST_STATE_ACTIVE ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_DEEP_TRAVEL_LSS, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_DEEP_TALK_JESSICA, QUEST_STATE_ACTIVE );
		return QMR_OK;
	}

	if ( ( pMessage->nLevelNewDef == pQuestData->nLevelNecropolisLevel1 ) &&
		 ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_TRAVEL_NECRO ) == QUEST_STATE_ACTIVE ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_DEEP_TRAVEL_NECRO, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_DEEP_TRAVEL_BOTTOM, QUEST_STATE_ACTIVE );
		return QMR_OK;
	}

	if ( ( pMessage->nLevelNewDef == pQuestData->nLevelNecropolisLevel5 ) &&
		 ( QuestStateGetValue( pQuest, QUEST_STATE_DEEP_TRAVEL_BOTTOM ) == QUEST_STATE_ACTIVE ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_DEEP_TRAVEL_BOTTOM, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_DEEP_FIND, QUEST_STATE_ACTIVE );
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nTheMarker )
	{
		// bling!
		s_StateSet( pTarget, pTarget, STATE_QUEST_ACCEPT, 0 );
		sChangeJessicaAI( pQuest );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEEP_FIND, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEEP_ACTIVATE, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEEP_POST_ACTIVATE_TALK, QUEST_STATE_ACTIVE );
		return QMR_FINISHED;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_DEEP * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if (UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nMarkerPOI ))
	{
		QuestStateSet( pQuest, QUEST_STATE_DEEP_FIND, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_DEEP_ACTIVATE, QUEST_STATE_ACTIVE );
		return QMR_FINISHED;
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
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeDeep(
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
	QUEST_DATA_DEEP * pQuestData)
{
	pQuestData->nLordArphaun					= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nJessicaSumerisle				= QuestUseGI( pQuest, GI_MONSTER_JESSICA_SUMERISLE );
	pQuestData->nJessicaSumerisleCombat			= QuestUseGI( pQuest, GI_MONSTER_JESSICA_SUMERISLE_COMBAT );
	pQuestData->nEmmera							= QuestUseGI( pQuest, GI_MONSTER_EMMERA );

	pQuestData->nTheMarker 						= QuestUseGI( pQuest, GI_OBJECT_THE_MARKER );
	pQuestData->nMarkerPOI 						= QuestUseGI( pQuest, GI_OBJECT_MARKER_POI );

	pQuestData->nLevelLiverpoolStreetStation	= QuestUseGI( pQuest, GI_LEVEL_LIVERPOOL_STREET_STATION );
	pQuestData->nLevelNecropolisLevel1			= QuestUseGI( pQuest, GI_LEVEL_NECROPOLIS_LEVEL_1 );
	pQuestData->nLevelNecropolisLevel5			= QuestUseGI( pQuest, GI_LEVEL_NECROPOLIS_LEVEL_5 );

	pQuestData->nAIIdle							= QuestUseGI( pQuest, GI_AI_IDLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitDeep(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_DEEP * pQuestData = ( QUEST_DATA_DEEP * )GMALLOCZ( pGame, sizeof( QUEST_DATA_DEEP ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nJessicaSumerisle );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nJessicaSumerisleCombat );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreDeep(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
