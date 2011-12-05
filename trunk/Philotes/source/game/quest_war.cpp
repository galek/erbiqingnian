//----------------------------------------------------------------------------
// FILE: quest_war.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_dark.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "stringtable.h"
#include "monsters.h"
#include "states.h"
#include "ai.h"
#include "unitmodes.h"
#include "room_layout.h"
#include "room_path.h"
#include "c_quests.h"
#include "s_quests.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
#define QUEST_WAR_SUMMON_TIME		3			// in seconds

//----------------------------------------------------------------------------
struct QUEST_DATA_WAR
{
	int		nCabalistLead;
	int		nCabalistMale;
	int		nCabalistMale2;
	int		nCabalistFemale;
	int		nScroll;
	int		nThief;
	int		nPestilence;
	int		nUberGuardAI;
	int		nIdleAI;
	int		nDRLG;
	int		nLevelMillenium;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_WAR * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_WAR *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_THIEF_ROOMS		32

static void sSpawnThief( 
	GAME * pGame,
	QUEST * pQuest )
{
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );

	// check the level for a thief
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	if (LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nThief ))
	{
		return;
	}
	
	// spawn nearby
	ROOM * pSpawnRoom = NULL;	
	ROOM * room = UnitGetRoom( pQuest->pPlayer );
	ROOM_LIST tNearbyRooms;
	RoomGetNearbyRoomList(room, &tNearbyRooms);
	if ( tNearbyRooms.nNumRooms)
	{
		int index = RandGetNum( pGame->rand, tNearbyRooms.nNumRooms - 1 );
		pSpawnRoom = tNearbyRooms.pRooms[index];
	}
	else
	{
		pSpawnRoom= room;
	}

	// and spawn the thief
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nThief;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 5 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnPestilence( QUEST * pQuest, UNIT * cabalist )
{
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
	GAME * game = UnitGetGame( cabalist );

	// check the level for a pestilence
	VECTOR vPosition;
	VECTOR vDirection;
	BOOL bFound = FALSE;
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetClass( test ) == pQuestData->nPestilence )
			{
				// found one already
				return;
			}
		}

		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
		ROOM_LAYOUT_GROUP * node = RoomGetLabelNode( room, "Pestilence", &pOrientation );
		if ( node && pOrientation )
		{
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );
			bFound = TRUE;
		}
	}

	ROOM * room = UnitGetRoom( cabalist );
	int nFound = 0;
	for ( UNIT * test = room->tUnitList.ppFirst[TARGET_GOOD]; test; test = test->tRoomNode.pNext )
	{
		int test_class = UnitGetClass( test );
		if ( ( test_class == pQuestData->nCabalistMale ) ||
			 ( test_class == pQuestData->nCabalistMale2 ) ||
			 ( test_class == pQuestData->nCabalistFemale ) )
		{
			// clear npc_god
			s_StateClear( test, UnitGetId( pQuest->pPlayer ), STATE_NPC_GOD, 0 );
			nFound++;
		}

		if ( test_class == pQuestData->nCabalistLead )
		{
			// change her AI
			nFound++;
		}
	}

	// find all 4 npcs?
	if ( nFound != 4 )
		return;

	// spawn! =)
	if ( !bFound )
		return;

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nPestilence;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 5 );
	tSpec.pRoom = room;
	tSpec.vPosition = vPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( game, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRevertCabalistAI( QUEST * pQuest )
{
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );

	// check the level for cabalist lead
	GAME * game = UnitGetGame( pQuest->pPlayer );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_GOOD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetClass( test ) == pQuestData->nCabalistLead )
			{
				// change her AI
				int ai = UnitGetStat( test, STATS_AI );
				if ( ai == pQuestData->nIdleAI )
					return;
				AI_Free( game, test );
				UnitSetStat( test, STATS_AI, pQuestData->nIdleAI );
				AI_Init( game, test );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartWar( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
	REF(pQuestData);

	GAME * game = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( game ) );

	// activate quest
	s_QuestActivateForAll( pQuest );

	// spawn thief
	sSpawnThief( game, pQuest );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	
	switch (pQuest->eStatus)
	{
		//----------------------------------------------------------------------------
		case QUEST_STATUS_INACTIVE:
		{
			if ( UnitIsMonsterClass( pSubject, pQuestData->nCabalistLead ))
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WAR_INIT );
				return QMR_NPC_TALK;
			}
			break;
		}
				
		//----------------------------------------------------------------------------
		case QUEST_STATUS_ACTIVE:
		{
			if (UnitIsMonsterClass( pSubject, pQuestData->nCabalistLead ))
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_RETURN_SCROLL ) == QUEST_STATE_ACTIVE )
				{
					if ( s_QuestCheckForItem( pPlayer, pQuestData->nScroll ) )
					{
						s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_WAR_SCROLLFOUND );
						return QMR_NPC_TALK;
					}
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_WAR_LOOKING );
					return QMR_NPC_TALK;
				}
				if ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_TALK ) == QUEST_STATE_ACTIVE )
				{
					sRevertCabalistAI( pQuest );					
					s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_WAR_COMPLETE, DIALOG_WAR_REWARD );
					return QMR_NPC_TALK;
				}
			}
			break;
		}
	}
	
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWarEmote2( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_EMOTE2, QUEST_STATE_COMPLETE );
	s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_DEFEND, QUEST_STATE_ACTIVE );

	UNITID id = (UNITID)tEventData.m_Data2;
	UNIT * cabalist = UnitGetById( game, id );
	if ( cabalist )
	{
		sSpawnPestilence( pQuest, cabalist );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_WAR_INIT:
		{
			if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
				sStartWar( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_WAR_SCROLLFOUND:
		{
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_RETURN_SCROLL ) == QUEST_STATE_ACTIVE ) &&
				 ( s_QuestCheckForItem( pPlayer, pQuestData->nScroll ) ) )
			{
				s_QuestRemoveItem( pPlayer, pQuestData->nScroll );
				s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_RETURN_SCROLL , QUEST_STATE_COMPLETE );
				s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_EMOTE1, QUEST_STATE_ACTIVE );
				s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_EMOTE2, QUEST_STATE_ACTIVE );
				s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_EMOTE1, QUEST_STATE_COMPLETE );
				// set delay for pestilence spawn
				GAME * game = UnitGetGame( pTalkingTo );
				AI_Free( game, pTalkingTo );
				UnitSetStat( pTalkingTo, STATS_AI, pQuestData->nUberGuardAI );
				AI_Init( game, pTalkingTo );
				UnitRegisterEventTimed( pPlayer, sWarEmote2, EVENT_DATA((DWORD_PTR)pQuest, UnitGetId(pTalkingTo)), GAME_TICKS_PER_SECOND * QUEST_WAR_SUMMON_TIME );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_WAR_COMPLETE:
		{
			QuestStateSet( pQuest, QUEST_STATE_WAR_TALK, QUEST_STATE_COMPLETE );
			if ( s_QuestComplete( pQuest ) )
			{
				s_QuestOfferReward( pQuest, pTalkingTo );			
			}
			break;
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
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{

		QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if (UnitIsMonsterClass( pSubject, pQuestData->nCabalistLead ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_RETURN_SCROLL ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_RETURN;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_FIND_SCROLL ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_WAITING;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_WAR_TALK ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_RETURN;
			}
		}

	}
	
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePickup(
	QUEST *pQuest,
	const QUEST_MESSAGE_PICKUP *pMessage )
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_WAR * pQuestData = sQuestDataGet( pQuest );
	UNIT * item = pMessage->pPickedUp;

	if ( UnitGetClass( item ) == pQuestData->nScroll )
	{
		s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_FIND_SCROLL, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_WAR_RETURN_SCROLL, QUEST_STATE_ACTIVE );
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_WAR * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// turn taint into interactive NPC
	if ( UnitIsMonsterClass( pVictim, pData->nPestilence ) &&
		QuestStateGetValue( pQuest, QUEST_STATE_WAR_DEFEND ) == QUEST_STATE_ACTIVE )
	{
		// revert cabalist lead AI
		QuestStateSet( pQuest, QUEST_STATE_WAR_DEFEND, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_WAR_TALK, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_WAR * pData = sQuestDataGet( pQuest );

	if ( pData->nDRLG != pMessage->nDRLGDefId )
		return QMR_OK;

	if ( PStrCmp( pMessage->szRuleName, "Rule_22", CAN_RUN_DRLG_RULE_NAME_SIZE ) != 0 )
		return QMR_OK;

	if ( pMessage->nLevelDefId == pData->nLevelMillenium )
		return QMR_DRLG_RULE_FORBIDDEN;

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
// client got a message from server about a new state
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	UNIT * player = pQuest->pPlayer;
	ASSERT_RETVAL( player, QMR_OK );
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( game, QMR_OK );
	ASSERT_RETVAL( IS_CLIENT( game ), QMR_OK );

	// do time warp
#if !ISVERSION(SERVER_VERSION)
	if ( ( nQuestState == QUEST_STATE_WAR_EMOTE1 ) && ( eValue == QUEST_STATE_COMPLETE ) )
	{
		c_NPCEmoteStart( game, DIALOG_WAR_EMOTE1 );
	}

	if ( ( nQuestState == QUEST_STATE_WAR_EMOTE2 ) && ( eValue == QUEST_STATE_COMPLETE ) )
	{
		c_NPCEmoteStart( game, DIALOG_WAR_EMOTE2 );
	}
#endif //!SERVER_VERSION

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
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP * pHasInfoMessage = (const QUEST_MESSAGE_PICKUP *)pMessage;
			return sQuestMessagePickup( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}
	}

	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeWar(
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
	QUEST_DATA_WAR* pQuestData)
{
	pQuestData->nCabalistLead	= QuestUseGI( pQuest, GI_MONSTER_WAR_LEAD );
	pQuestData->nCabalistMale	= QuestUseGI( pQuest, GI_MONSTER_WAR_MALE1 );
	pQuestData->nCabalistMale2	= QuestUseGI( pQuest, GI_MONSTER_WAR_MALE2 );
	pQuestData->nCabalistFemale = QuestUseGI( pQuest, GI_MONSTER_WAR_FEMALE );
	pQuestData->nScroll			= QuestUseGI( pQuest, GI_ITEM_WAR_SCROLL );
	pQuestData->nThief			= QuestUseGI( pQuest, GI_MONSTER_WAR_THIEF );
	pQuestData->nPestilence		= QuestUseGI( pQuest, GI_MONSTER_WAR_SUMMONED );
	pQuestData->nUberGuardAI	= QuestUseGI( pQuest, GI_AI_SUMMON_UBER_GUARD );
	pQuestData->nIdleAI			= QuestUseGI( pQuest, GI_AI_IDLE );
	pQuestData->nDRLG			= QuestUseGI( pQuest, GI_DRLG_RIVER );
	pQuestData->nLevelMillenium = QuestUseGI( pQuest, GI_LEVEL_MILLENNIUM );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitWar(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
		
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_WAR * pQuestData = ( QUEST_DATA_WAR * )GMALLOCZ( pGame, sizeof( QUEST_DATA_WAR ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

}
