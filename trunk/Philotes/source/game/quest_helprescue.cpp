//----------------------------------------------------------------------------
// FILE: quest_helprescue.cpp
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
#include "quest_helprescue.h"
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
#include "ai.h"
#include "states.h"
#include "unitmodes.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_HELPRESCUE
{
	int		nLordArphaun;
	int		nLann;
	int		nHighLordMaxim;

	int		nAltair;
	int		nAltairTroop;

	int		nLevelTudorStreet;
	int		nLevelTemplePlace;
	int		nLevelTemplarBase;

	int		nMedKit;

	int		nAttackAI;
	int		nIdleAI;
	
	int		nObjectWarpMonumentEastcheap;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_HELPRESCUE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_HELPRESCUE *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsQuestClassNPC(
	QUEST * pQuest,
	UNIT * npc )
{
	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	return UnitIsMonsterClass( npc, pQuestData->nLordArphaun );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawn(
	GAME * pGame,
	ROOM * pDestRoom,
	int nClass,
	QUEST * pQuest )
{
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pDestRoom, 0 );

	// find ground location
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = nClass;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pDestRoom ), tSpec.nClass, 19 );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * pTroop = s_MonsterSpawn( pGame, tSpec );
	if ( pTroop )
	{
		// who to follow
		UnitSetStat( pTroop, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pQuest->pPlayer ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSpawnCheckAltair(
	QUEST * pQuest )
{
	// check the level for Altair
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pMob = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nAltair );
	return pMob;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_ALTAIR_TROOPS		4

static void sSpawnAltairAndTroops(
	QUEST * pQuest,
	ROOM * pDestRoom )
{
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( pPlayer );

	sSpawn( pGame, pDestRoom, pQuestData->nAltair, pQuest );
	for ( int i = 0; i < NUM_ALTAIR_TROOPS; i++ )
	{
		sSpawn( pGame, pDestRoom, pQuestData->nAltairTroop, pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnAeronAltairAtExit(
	QUEST * pQuest )
{
	// Altair already on level?
	UNIT * pAltair = sSpawnCheckAltair( pQuest );
	if ( pAltair )
	{
		// if he is already on the level and you are trying to talk to him, make sure you can!
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON ) == QUEST_STATE_ACTIVE )
		{
			UnitSetInteractive( pAltair, UNIT_INTERACTIVE_ENABLED );
			// make sure he is idle
			QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
			GAME * pGame = UnitGetGame( pAltair );
			AI_Free( pGame, pAltair );
			UnitSetStat( pAltair, STATS_AI, pQuestData->nIdleAI );
			AI_Init( pGame, pAltair );
		}
		return;
	}

	// get exit room
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT * pExit = LevelFindClosestUnitOfType( pLevel, cgvNone, eTargetTypes, UNITTYPE_WARP_NEXT_LEVEL );
	ASSERT_RETURN( pExit );

	GAME * pGame = UnitGetGame( pPlayer );
	ROOM * pRoom = UnitGetRoom( pExit );
	ROOM * pSpawnRoom = RoomGetRandomNearbyRoom(pGame, pRoom);
	
	sSpawnAltairAndTroops( pQuest, pSpawnRoom );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnAeronAltairAtEntrance(
	QUEST * pQuest )
{
	// Altair already on level?
	if ( sSpawnCheckAltair( pQuest ) != NULL )
		return;

	ROOM * pRoom = UnitGetRoom( pQuest->pPlayer );
	ROOM * pSpawnRoom = RoomGetFirstConnectedRoomOrSelf(pRoom);
	
	sSpawnAltairAndTroops( pQuest, pSpawnRoom );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeAIs( QUEST * pQuest, int nAI )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		// check to see if spawned
		for ( UNIT * test = pRoom->tUnitList.ppFirst[TARGET_GOOD]; test; test = test->tRoomNode.pNext )
		{
			int test_class = UnitGetClass( test );
			if ( ( test_class == pQuestData->nAltair ) || ( test_class == pQuestData->nAltairTroop ) )
			{
				// change the AI on Aeron Altair and his troops
				if ( UnitGetStat( test, STATS_AI ) != nAI )
				{
					AI_Free( pGame, test );
					UnitSetStat( test, STATS_AI, nAI );
					AI_Init( pGame, test );
					// follow this player!!
					UnitSetStat( test, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pQuest->pPlayer ) );
					// this NPC is not interactive anymore
					if ( test_class == pQuestData->nAltair )
						UnitSetInteractive( test, UNIT_INTERACTIVE_RESTRICED );
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeLannAI( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * pLann = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nLann );
	// change the AI on Lann after he is healed
	if ( UnitGetStat( pLann, STATS_AI ) != pQuestData->nAttackAI )
	{
		AI_Free( pGame, pLann );
		UnitSetStat( pLann, STATS_AI, pQuestData->nAttackAI );
		AI_Init( pGame, pLann );

		// follow this player!!
		UnitSetStat( pLann, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( pQuest->pPlayer ) );

		// this NPC is not interactive anymore
		UnitSetInteractive( pLann, UNIT_INTERACTIVE_RESTRICED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasMedKit(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	s_QuestMapReveal( pQuest, GI_LEVEL_EASTCHEAP );
	s_QuestMapReveal( pQuest, GI_LEVEL_TUDOR_STREET );
	s_QuestMapReveal( pQuest, GI_LEVEL_TEMPLE_PLACE );

	QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKARPHAUN, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TRAVELTUDORST, QUEST_STATE_ACTIVE );
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nObjectWarpMonumentEastcheap );
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

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nAltair ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKARPHAUN ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( sIsQuestClassNPC( pQuest, pSubject ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELPRESCUE_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nAltair ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELPRESCUE_AERON );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKARPHAUN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELPRESCUE_VIDEO, INVALID_LINK, GI_MONSTER_AERON_ALTAIR );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nHighLordMaxim ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKMAXIM ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_HELPRESCUE_COMPLETE );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoComplete( QUEST * pQuest )
{
	QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_DISCOVER, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_HELLRIFT, QUEST_STATE_ACTIVE );
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
		case DIALOG_HELPRESCUE_INTRO:
		{
			s_QuestActivate( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HELPRESCUE_VIDEO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasMedKit;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTHELPRESCUEOFFER, tActions );

			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_HELPRESCUE_AERON:
		{
			QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TRAVELTEMPLEPL, QUEST_STATE_ACTIVE );
			// Change AIs of Altair and group to follow you
			QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
			sChangeAIs( pQuest, pQuestData->nAttackAI );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HELPRESCUE_DETECTIONVIDEO:
		{
			sVideoComplete( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HELPRESCUE_COMPLETE:
		{
			// complete the quest
			QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKMAXIM, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			break;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if ( pMessage->nDialog == DIALOG_HELPRESCUE_DETECTIONVIDEO )
	{
		sVideoComplete( pQuest );
		return QMR_FINISHED;
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define USE_MEDKIT_DIST		3.0f

static QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if ( pItem )
	{
		QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nMedKit )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
				if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelTemplePlace )
				{
					// check the level for Lann
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
					UNIT * lann =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nLann );
					if ( lann )
					{
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, lann->vPosition );
						if ( distsq <= ( USE_MEDKIT_DIST * USE_MEDKIT_DIST ) )
						{
							return QMR_USE_OK;
						}
					}
				}
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFinishMedkit( QUEST * pQuest, UNITID idUnit )
{
	ASSERT_RETURN( pQuest );
	ASSERT_RETURN( idUnit != INVALID_ID );

	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pLann = UnitGetById( pGame, idUnit );
	ASSERT_RETURN( pLann );

	s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nMedKit );
	s_QuestStateSetForAll( pQuest, QUEST_STATE_HELPRESCUE_HEAL, QUEST_STATE_COMPLETE );
	s_QuestStateSetForAll( pQuest, QUEST_STATE_HELPRESCUE_ESCORT, QUEST_STATE_ACTIVE );
	s_StateSet( pLann, pPlayer, STATE_QUEST_ACCEPT, 0 );
	s_NPCEmoteStart( pGame, pPlayer, DIALOG_HELPRESCUE_EMOTE );
	sChangeLannAI( pQuest );
	s_StateClear( pLann, UnitGetId( pLann ), STATE_LANN_INJURED, 0 );
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
		QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nMedKit )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
				if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelTemplePlace )
				{
					// check the level for Lann
					UNIT * pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
					UNIT * lann =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nLann );
					if ( lann )
					{
						float distsq = VectorDistanceSquaredXY( pPlayer->vPosition, lann->vPosition );
						if ( distsq <= ( USE_MEDKIT_DIST * USE_MEDKIT_DIST ) )
						{
							// start medkit
							s_QuestOperateDelay( pQuest, sFinishMedkit, UnitGetId( lann ), 5 * GAME_TICKS_PER_SECOND );
							return QMR_USE_OK;
						}
					}
				}
			}
			return QMR_USE_FAIL;
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

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTudorStreet )
	{
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TRAVELTUDORST, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON, QUEST_STATE_ACTIVE );
		sSpawnAeronAltairAtExit( pQuest );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTemplePlace )
	{
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TRAVELTUDORST, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKAERON, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TRAVELTEMPLEPL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_DISCOVER, QUEST_STATE_ACTIVE );
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_DISCOVER ) == QUEST_STATE_ACTIVE  )
		{
			// discover video
			s_NPCVideoStart( game, player, GI_MONSTER_ALTAIR_TROOP, NPC_VIDEO_INSTANT_INCOMING, DIALOG_HELPRESCUE_DETECTIONVIDEO, INVALID_LINK, GI_MONSTER_AERON_ALTAIR );
		}
		sSpawnAeronAltairAtEntrance( pQuest );
		// change AIs for him and his troops
		sChangeAIs( pQuest, pQuestData->nAttackAI );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTemplarBase )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_HEAL ) == QUEST_STATE_COMPLETE )
		{
			QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_ESCORT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_TALKMAXIM, QUEST_STATE_ACTIVE );
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
	QUEST_DATA_HELPRESCUE *pQuestData = sQuestDataGet( pQuest );	
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpMonumentEastcheap ) )
	{
		if ( eStatus == QUEST_STATUS_INACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_TALKARPHAUN ) >= QUEST_STATE_COMPLETE )
				return QMR_OPERATE_OK;

			return QMR_OPERATE_FORBIDDEN;
		}

		return QMR_OPERATE_OK;
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
		QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
		UNIT *pTarget = pMessage->pTarget;

		int nHellriftLevel = UnitGetLevelDefinitionIndex( pTarget );
		if ( nHellriftLevel == pQuestData->nLevelTemplePlace )
		{
			if (UnitIsA( pTarget, UNITTYPE_HELLRIFT_PORTAL ))
			{
				if (QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_HELLRIFT ) == QUEST_STATE_ACTIVE)
				{
					QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_DISCOVER, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_HELLRIFT, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_HELPRESCUE_HEAL, QUEST_STATE_ACTIVE );
				}
				else if (QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_ESCORT ) == QUEST_STATE_ACTIVE)
				{
					// warp Brandon Lann out of the hellrift room to the player
					UNIT *pPlayer = pQuest->pPlayer;
					LEVEL * pLevel = UnitGetLevel( pPlayer );
					TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
					UNIT * pLann = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nLann );
					ROOM *pPlayerRoom = UnitGetRoom( pPlayer );
					if ( pLann && pPlayerRoom && ( UnitGetRoom( pLann ) != pPlayerRoom ) && !UnitHasState( UnitGetGame( pLann ), pLann, STATE_LANN_INJURED ) )
					{
						VECTOR vPosition = UnitGetPosition( pPlayer );
						ROOM_PATH_NODE * pPathNode = NULL;
						ROOM *pDestinationRoom = NULL;
						DWORD dwNearestNodeFlags = 0;
						GAME *pGame = UnitGetGame( pPlayer );
						pPathNode = RoomGetNearestFreePathNode( pGame, pLevel, vPosition, &pDestinationRoom, pLann, 0.0f, 0.0f, NULL, dwNearestNodeFlags );
						if( pPathNode )
						{
							vPosition = RoomPathNodeGetExactWorldPosition(pGame, pDestinationRoom, pPathNode);
						}
						else
						{
							vPosition = pPlayer->vPosition;
						}
						VECTOR vFaceDirection = UnitGetFaceDirection(pPlayer, FALSE);
						VECTOR vUpDirection = UnitGetUpDirection(pPlayer);
						UnitWarp(pLann, pPlayerRoom, vPosition, vFaceDirection, vUpDirection, 0);
					}

				}
			}

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

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if ( UnitIsMonsterClass( pTarget, pQuestData->nLann ) )
	{
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_HELPRESCUE_HEAL ) >= QUEST_STATE_COMPLETE ) &&
			 ( UnitHasState( UnitGetGame( pTarget ), pTarget, STATE_LANN_INJURED ) ) )
		{
			sChangeLannAI( pQuest );
			s_StateClear( pTarget, UnitGetId( pTarget ), STATE_LANN_INJURED, 0 );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageAbandon(
	QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest, QMR_OK );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETVAL( pPlayer, QMR_OK );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETVAL( pGame, QMR_OK );

	QUEST_DATA_HELPRESCUE * pQuestData = sQuestDataGet( pQuest );
	s_QuestCloseWarp( pGame, pPlayer, pQuestData->nObjectWarpMonumentEastcheap );
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
		case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
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
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ABANDON:
		{
			return sQuestMessageAbandon( pQuest );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeHelpRescue(
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
	QUEST_DATA_HELPRESCUE * pQuestData)
{
	pQuestData->nLordArphaun		= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLann				= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN_INJURED );
	pQuestData->nHighLordMaxim		= QuestUseGI( pQuest, GI_MONSTER_LORD_MAXIM );

	pQuestData->nAltair				= QuestUseGI( pQuest, GI_MONSTER_AERON_ALTAIR_COMBAT );
	pQuestData->nAltairTroop		= QuestUseGI( pQuest, GI_MONSTER_ALTAIR_TROOP );

	pQuestData->nLevelTudorStreet	= QuestUseGI( pQuest, GI_LEVEL_TUDOR_STREET );
	pQuestData->nLevelTemplePlace	= QuestUseGI( pQuest, GI_LEVEL_TEMPLE_PLACE );
	pQuestData->nLevelTemplarBase	= QuestUseGI( pQuest, GI_LEVEL_TEMPLAR_BASE );

	pQuestData->nMedKit				= QuestUseGI( pQuest, GI_ITEM_HELPRESCUE_MEDKIT );

	pQuestData->nAttackAI			= QuestUseGI( pQuest, GI_AI_TEMPLAR_ESCORT );
	pQuestData->nIdleAI				= QuestUseGI( pQuest, GI_AI_IDLE );
	
	pQuestData->nObjectWarpMonumentEastcheap = QuestUseGI( pQuest, GI_OBJECT_WARP_MONUMENT_EASTCHEAP );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitHelpRescue(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_HELPRESCUE * pQuestData = ( QUEST_DATA_HELPRESCUE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_HELPRESCUE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLordArphaun );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nAltair );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLann );

	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreHelpRescue(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
