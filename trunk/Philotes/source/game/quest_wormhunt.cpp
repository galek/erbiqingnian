//----------------------------------------------------------------------------
// FILE: quest_wormhunt.cpp
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
#include "quest_wormhunt.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "room_path.h"
#include "room_layout.h"
#include "ai.h"
#include "uix_components_hellgate.h"
#include "colors.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
#define NUM_WORMHUNT_TROOPS			4

struct QUEST_DATA_WORMHUNT
{
	int		nRorke;
	int		nSerSing;

	int		nRelic;
	int		nDevice;

	int		nLevelBargeHouse;

	int		nAttackAI;
	int		nGuardAI;

	int		nHunter[ NUM_WORMHUNT_TROOPS ];

	int		nWurm;			// worm monster class
	int		nButterfly;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_WORMHUNT * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_WORMHUNT *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnTroops(
	QUEST * pQuest )
{
	// check the level for any commander
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nHunter[0] ) )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pPlayer );
	ROOM * pRoom = UnitGetRoom( pPlayer );

	for ( int i = 0; i < NUM_WORMHUNT_TROOPS; i++ )
	{
		int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );

		// find ground location
		if (nNodeIndex < 0)
			return;

		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pRoom, nNodeIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nHunter[i];
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 15 );
		tSpec.pRoom = pRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnButterfly(
	QUEST * pQuest,
	UNIT * pWurm )
{
	// check the level for a butterfly already
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nButterfly ) )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pWurm );
	ROOM * pRoom = UnitGetRoom( pWurm );
	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pRoom, UnitGetPosition( pWurm ), &pDestRoom );

	if ( !pNode )
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nButterfly;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 17 );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeAIs( QUEST * pQuest, int nAI )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );

	for ( int i = 0; i < NUM_WORMHUNT_TROOPS; i++ )
	{
		UNIT * hunter = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nHunter[i] );
		if ( hunter )
		{
			// change the AI on the hunters
			if ( UnitGetStat( hunter, STATS_AI ) != nAI )
			{
				AI_Free( pGame, hunter );
				UnitSetStat( hunter, STATS_AI, nAI );
				AI_Init( pGame, hunter );

				if ( nAI == pQuestData->nAttackAI )
					UnitSetStat( hunter, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pQuest->pPlayer ) );

				// this NPC is not interactive anymore
				UnitSetInteractive( hunter, UNIT_INTERACTIVE_RESTRICED );
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasRelic(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	if ( s_QuestActivate( pQuest ) )
	{
		s_QuestMapReveal( pQuest, GI_LEVEL_WATERLOO );
		s_QuestMapReveal( pQuest, GI_LEVEL_UPPER_GROUND );
		s_QuestMapReveal( pQuest, GI_LEVEL_BARGE_HOUSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasDevice(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_RORKE, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_TRAVEL, QUEST_STATE_ACTIVE );
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

	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nRorke ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_RORKE ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_RETURN ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nHunter[0] ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_MEET ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WORMHUNT_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nRorke ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_RORKE ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WORMHUNT_RORKE );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WORMHUNT_COMPLETE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nHunter[0] ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_MEET ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WORMHUNT_TROOP );
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
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_WORMHUNT_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasRelic;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTWORMHUNTRELIC, tActions );

			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_WORMHUNT_RORKE:
		{
			QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );

			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nRelic );

			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasDevice;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTWORMHUNTDEVICE, tActions );

			return QMR_OFFERING;
		}

		//----------------------------------------------------------------------------
		case DIALOG_WORMHUNT_TROOP:
		{
			QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );

			s_QuestStateSetForAll( pQuest, QUEST_STATE_WORMHUNT_MEET, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_WORMHUNT_USETRACKING, QUEST_STATE_ACTIVE );

			sChangeAIs( pQuest, pQuestData->nAttackAI );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_WORMHUNT_COMPLETE:
		{
			// remove radar
			QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nDevice );

			// make sure it was off
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );

			// complete the quest
			QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_RETURN, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			s_QuestDelayNext( pQuest, 10 * GAME_TICKS_PER_SECOND );
			break;
		}

	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendWormLocation( 
	QUEST *pQuest )
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected quest player" );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );

	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	UNIT * pWurm = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nWurm );
	if ( pWurm )
	{
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, pWurm, 0, GFXCOLOR_YELLOW );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sResetInteractive( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_MEET ) != QUEST_STATE_ACTIVE )
		return;

	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT * pUnit = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nHunter[0] );
	if ( pUnit )
	{
		// this NPC is now interactive
		UnitSetInteractive( pUnit, UNIT_INTERACTIVE_ENABLED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnOrDestroyWurm( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	// first let's see if there is one on the level
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	UNIT * pWurm = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nWurm );
	if ( pWurm )
	{
		return;
	}

	// spawn a wurm
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "wurm", &pRoom, &pOrientation );
	ASSERTX_RETURN( pNode && pRoom && pOrientation, "Couldn't find Wurm spawn location" );

	VECTOR vPosition, vDirection, vUp;
	RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, pRoom, vPosition, &pDestRoom );

	if ( pPathNode )
	{
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nWurm;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 17 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
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

	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelBargeHouse )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_KILL ) <= QUEST_STATE_ACTIVE )
		{
			sSpawnOrDestroyWurm( pQuest );
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
			sSendWormLocation( pQuest );
		}
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_MEET, QUEST_STATE_ACTIVE );
		sSpawnTroops( pQuest );

		// if the player is coming onto the level and they have already talked to the
		// hunter NPCs, but haven't killed or completed the quest, then change them to follow right away
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_MEET ) >= QUEST_STATE_COMPLETE )
		{
			sChangeAIs( pQuest, pQuestData->nAttackAI );
		}
		else
		{
			// make sure you can talk to the right person ;)
			sResetInteractive( pQuest );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK; 

	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if (UnitIsMonsterClass( pTarget, pQuestData->nWurm ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_WORMHUNT_USETRACKING ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_MEET, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_USETRACKING, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_KILL, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if ( pItem )
	{
		QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nDevice )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				ASSERT_RETVAL(pQuestData, QMR_USE_FAIL);
				LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
				if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelBargeHouse )
				{
					return QMR_USE_OK;
				}
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nDevice )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
				if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelBargeHouse )
				{
					s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TOGGLE );
					return QMR_USE_OK;
				}
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_WORMHUNT * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// if the wurm died, spawn the butterfly
	if (UnitIsMonsterClass( pVictim, pQuestData->nWurm ) &&
		QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_MEET, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_USETRACKING, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_KILL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_WORMHUNT_RETURN, QUEST_STATE_ACTIVE );
		sChangeAIs( pQuest, pQuestData->nGuardAI );
		sSpawnButterfly( pQuest, pVictim );
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_REMOVE_UNIT, pVictim );
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
		case QM_CAN_USE_ITEM:
		{
			const QUEST_MESSAGE_CAN_USE_ITEM *pCanUseItemMessage = ( const QUEST_MESSAGE_CAN_USE_ITEM * )pMessage;
			return sQuestMessageCanUseItem( pQuest, pCanUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_USE_ITEM:
		{
			const QUEST_MESSAGE_USE_ITEM *pUseItemMessage = ( const QUEST_MESSAGE_USE_ITEM * )pMessage;
			return sQuestMessageUseItem( pQuest, pUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
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
void QuestFreeWormHunt(
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
	QUEST_DATA_WORMHUNT * pQuestData)
{
	pQuestData->nRorke				= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nSerSing			= QuestUseGI( pQuest, GI_MONSTER_SER_SING );
	pQuestData->nRelic				= QuestUseGI( pQuest, GI_ITEM_WORMHUNT_RELIC );
	pQuestData->nDevice				= QuestUseGI( pQuest, GI_ITEM_WORMHUNT_DEVICE );
	pQuestData->nLevelBargeHouse	= QuestUseGI( pQuest, GI_LEVEL_BARGE_HOUSE );
	pQuestData->nHunter[0] 			= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_1 );
	pQuestData->nHunter[1] 			= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_2 );
	pQuestData->nHunter[2] 			= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_3 );
	pQuestData->nHunter[3] 			= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_4 );
	pQuestData->nAttackAI			= QuestUseGI( pQuest, GI_AI_WORM_HUNTER );
	pQuestData->nGuardAI			= QuestUseGI( pQuest, GI_AI_UBER_GUARD );
	pQuestData->nWurm				= QuestUseGI( pQuest, GI_MONSTER_WURM );
	pQuestData->nButterfly			= QuestUseGI( pQuest, GI_MONSTER_BUTTERFLY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitWormHunt(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_WORMHUNT * pQuestData = ( QUEST_DATA_WORMHUNT * )GMALLOCZ( pGame, sizeof( QUEST_DATA_WORMHUNT ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// xtras!
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nRorke );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nHunter[0] );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreWormHunt(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/