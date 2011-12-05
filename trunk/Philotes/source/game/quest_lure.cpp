//----------------------------------------------------------------------------
// FILE: quest_lure.cpp
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
#include "quest_lure.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "ai.h"
#include "room_path.h"
#include "room_layout.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define NUM_LURE_TROOPS			4

//----------------------------------------------------------------------------
struct QUEST_DATA_LURE
{
	int		nArphaun;
	int		nLtGray;

	int		nBeastofAbbadon;

	int		nHunter[ NUM_LURE_TROOPS ];

	int		nAttackAI;
	int		nGuardAI;

	int		nLevelTowerOfLondon;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_LURE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_LURE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsQuestClassNPC(
	QUEST * pQuest,
	UNIT * npc )
{
	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	return UnitIsMonsterClass( npc, pQuestData->nArphaun );
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

	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLtGray ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_LURE_TALK_TROOP ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( sIsQuestClassNPC( pQuest, pSubject ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_LURE_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLtGray ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_LURE_TALK_TROOP ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_LURE_TALK_TROOP );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( sIsQuestClassNPC( pQuest, pSubject ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_LURE_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_LURE_COMPLETE );
			return QMR_NPC_TALK;
		}
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeAIs( QUEST * pQuest, int nAI )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );

	for ( int i = 0; i < NUM_LURE_TROOPS; i++ )
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

				// this NPC is not interactive anymore
				UnitSetInteractive( hunter, UNIT_INTERACTIVE_RESTRICED );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sResetInteractive( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	if ( QuestStateGetValue( pQuest, QUEST_STATE_LURE_TALK_TROOP ) != QUEST_STATE_ACTIVE )
		return;

	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT * pLtGray = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nLtGray );
	if ( pLtGray )
	{
		// this NPC is now interactive
		UnitSetInteractive( pLtGray, UNIT_INTERACTIVE_ENABLED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// if the beast of abbadon died, wrap up quest
	if (UnitIsMonsterClass( pVictim, pQuestData->nBeastofAbbadon ) &&
		QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{
		QuestStateSet( pQuest, QUEST_STATE_LURE_KILL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_LURE_RETURN, QUEST_STATE_ACTIVE );
		sChangeAIs( pQuest, pQuestData->nGuardAI );
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
		case DIALOG_LURE_INTRO:
		{
			s_QuestActivate( pQuest );
			s_QuestMapReveal( pQuest, GI_LEVEL_LOWER_THAMES );
			s_QuestMapReveal( pQuest, GI_LEVEL_TOWER_GATEWAY );
			s_QuestMapReveal( pQuest, GI_LEVEL_TOWER_OF_LONDON );
			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_LURE_TALK_TROOP:
		{
			QuestStateSet( pQuest, QUEST_STATE_LURE_TALK_TROOP, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_LURE_KILL, QUEST_STATE_ACTIVE );
			QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
			sChangeAIs( pQuest, pQuestData->nAttackAI );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_LURE_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_LURE_RETURN, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			s_QuestDelayNext( pQuest, 15 * GAME_TICKS_PER_SECOND );
			break;
		}

	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSpawnBeast( QUEST * pQuest )
{
	ASSERT_RETFALSE( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETFALSE( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( IS_SERVER( pGame ) );

	// first let's see if there is one on the level
	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETFALSE( pLevel );
	UNIT * pBeast = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nBeastofAbbadon );
	if ( pBeast )
	{
		return FALSE;
	}

	// spawn a beast
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "beast_of_abbadon", &pRoom, &pOrientation );
	ASSERTX_RETFALSE( pNode && pRoom && pOrientation, "Couldn't find Beast of Abbadon spawn location" );

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
		tSpec.nClass = pQuestData->nBeastofAbbadon;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 23 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		return s_MonsterSpawn( pGame, tSpec ) != NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_LURE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTowerOfLondon )
	{
		QuestStateSet( pQuest, QUEST_STATE_LURE_TRAVEL_TOL, QUEST_STATE_COMPLETE );
		if (sSpawnBeast( pQuest ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_LURE_TALK_TROOP ) >= QUEST_STATE_COMPLETE )
			{
				sChangeAIs( pQuest, pQuestData->nAttackAI );
			}
			else
			{
				QuestStateSet( pQuest, QUEST_STATE_LURE_TALK_TROOP, QUEST_STATE_ACTIVE );
				sResetInteractive( pQuest );
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
void QuestFreeLure(
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
	QUEST_DATA_LURE * pQuestData)
{
	pQuestData->nArphaun						= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLtGray							= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_1 );

	pQuestData->nBeastofAbbadon					= QuestUseGI( pQuest, GI_MONSTER_BEAST_OF_ABBADON );

	pQuestData->nHunter[0] 						= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_1 );
	pQuestData->nHunter[1] 						= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_2 );
	pQuestData->nHunter[2] 						= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_3 );
	pQuestData->nHunter[3] 						= QuestUseGI( pQuest, GI_MONSTER_WORMHUNTER_4 );

	pQuestData->nAttackAI						= QuestUseGI( pQuest, GI_AI_WORM_HUNTER );
	pQuestData->nGuardAI						= QuestUseGI( pQuest, GI_AI_UBER_GUARD );

	pQuestData->nLevelTowerOfLondon				= QuestUseGI( pQuest, GI_LEVEL_TOWER_OF_LONDON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitLure(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_LURE * pQuestData = ( QUEST_DATA_LURE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_LURE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLtGray );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreLure(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
