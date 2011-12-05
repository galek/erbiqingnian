//----------------------------------------------------------------------------
// FILE: quest_holdfast.cpp
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
#include "quest_holdfast.h"
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
struct QUEST_DATA_HOLDFAST
{
	int		nDRLG;
	int		nArphaun;
	int		nCommander;
	int		nLevelMB;
	int		nTurrets;
	int		nMiniBosses;
};

//----------------------------------------------------------------------------
struct TURRET_CALLBACK_DATA
{
	int		nTurrets;
	BOOL	bAllowTurretActivation;
	UNIT	*pPlayer;

	TURRET_CALLBACK_DATA::TURRET_CALLBACK_DATA( void )
		:	nTurrets( INVALID_LINK ),
		bAllowTurretActivation( TRUE ),
		pPlayer( NULL )
	{ }
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_HOLDFAST * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_HOLDFAST *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
enum TURRET_POSITION_METHOD
{
	TPM_FLAT,
	TPM_RANDOM
};

static BOOL sFindMonsterSpawnPosition(GAME *game, ROOM *pRoom, int nMonsterClass, TURRET_POSITION_METHOD eMethod, VECTOR &vSpawnPosition)
{
	// get unit data for monster to spawn
	const UNIT_DATA *pUnitData = UnitGetData( NULL, GENUS_MONSTER, nMonsterClass );

	if (eMethod == TPM_FLAT)
	{

		// find location
		const float flFlatZTolerance = 1.0f;	
		return RoomGetClearFlatArea( 
			pRoom, 
			pUnitData->fCollisionHeight,
			pUnitData->fCollisionRadius * 2.5f,
			&vSpawnPosition,
			NULL,
			FALSE,
			flFlatZTolerance);

	}
	else
	{

		int nNodeIndex = s_RoomGetRandomUnoccupiedNode( game, pRoom, 0 );
		if (nNodeIndex < 0)
		{
			return FALSE;
		}

		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pRoom, nNodeIndex );
		vSpawnPosition = tLocation.vSpawnPosition;
		return TRUE;

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSpawnSingleTurret(
	QUEST * pQuest,
	TURRET_POSITION_METHOD eMethod)
{
	GAME * game = UnitGetGame( pQuest->pPlayer );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	SUBLEVEL * pSubLevel = LevelGetPrimarySubLevel( pLevel );
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );
	
	// pick a room suitable for this monster
	DWORD dwRandomRoomFlags = 0;
	SETBIT( dwRandomRoomFlags, RRF_PATH_NODES_BIT );
	SETBIT( dwRandomRoomFlags, RRF_MUST_ALLOW_MONSTER_SPAWN_BIT );
	SETBIT( dwRandomRoomFlags, RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT );
	ROOM *pRoom = s_SubLevelGetRandomRoom( pSubLevel, dwRandomRoomFlags, NULL, NULL );
	ASSERTX_RETFALSE( pRoom, "Unable to get random room for unqiue to spawn in" );

	// attempt to spawn the turret
	VECTOR vSpawnPosition;
	if (sFindMonsterSpawnPosition(game, pRoom, pQuestData->nTurrets, eMethod, vSpawnPosition))
	{
		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nTurrets;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 15 );
		tSpec.pRoom = pRoom;
		tSpec.vPosition = vSpawnPosition;
		tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		UNIT * turret = s_MonsterSpawn( game, tSpec );
		ASSERT_RETFALSE( turret );

		s_StateSet( turret, turret, STATE_NPC_INFO_RETURN, 0 );		

		// now attempt to spawn a big monster in the same room, so the player has something meaty to zap with the turret -cmarch
		if (sFindMonsterSpawnPosition(game, pRoom, pQuestData->nMiniBosses, eMethod, vSpawnPosition))
		{

			MONSTER_SPEC tSpec;
			tSpec.nClass = pQuestData->nMiniBosses;
			tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 13 );
			tSpec.pRoom = pRoom;
			tSpec.vPosition = vSpawnPosition;
			tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
			UNIT * monster = s_MonsterSpawn( game, tSpec );

			ASSERT_RETFALSE( monster );
			UnitSetStatVector( monster, STATS_AI_ANCHOR_POINT_X, 0, UnitGetPosition( turret ) ); // guard the turret

		}

		return TRUE;

	}
	else
	{
		return FALSE;
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnAllTurrets(
	QUEST * pQuest )
{
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );

	// check the level for any turret
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	if (LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nTurrets ))
	{
		return;
	}

	TURRET_POSITION_METHOD eMethod = TPM_FLAT;	
	int nTurretsSpawned = 0;
	const int MAX_TRIES = 256;
	int nNumTries = MAX_TRIES;
	while (nNumTries--)
	{
	
		// if it's bee too long, try the fallback method
		if (nNumTries < (MAX_TRIES / 2))
		{
			eMethod = TPM_RANDOM;
		}
		
		// spawn single turret
		if (sSpawnSingleTurret( pQuest, eMethod ))
		{
		
			// we created one, see if we should stop
			nTurretsSpawned++;
			if (nTurretsSpawned == pQuest->pQuestDef->nSpawnCount)
			{
				break;
			}
			
		}
		
	}
	
	// sanity
	ASSERTX_RETURN( nTurretsSpawned == pQuest->pQuestDef->nSpawnCount, "Could not spawn all turrets for holdfast quest" );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetAllTurretsInactive(
	QUEST * pQuest )
{
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );

	// all turrets have been activated now
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT * pPlayer = QuestGetPlayer( pQuest );
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_GOOD]; test; test = test->tRoomNode.pNext )
		{
			if ( ( UnitGetClass( test ) == pQuestData->nTurrets ) && UnitCanInteractWith( test, pPlayer ) )
			{
				s_StateClear( test, UnitGetId( test ), STATE_NPC_INFO_RETURN, 0 );
				UnitSetInteractive( test, UNIT_INTERACTIVE_RESTRICED );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnCommander(
	QUEST * pQuest )
{
	// check the level for any commander
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );
	if (LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nCommander))
	{
		return;
	}

	GAME * pGame = UnitGetGame( pPlayer );
	ROOM * pRoom = UnitGetRoom( pPlayer );
	int nNodeIndex = -1;

	// find warp
	TARGET_TYPE eTargetObjectTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT * warp = LevelFindClosestUnitOfType( pLevel, cgvNone, eTargetObjectTypes, UNITTYPE_WARP_PREV_LEVEL );
	if ( warp )
	{
		VECTOR vPos = UnitGetPosition( warp );
		VECTOR vDir = UnitGetDirectionXY( warp );
		vDir *= 10.0f;
		vPos += vDir;
		ROOM_PATH_NODE * node = RoomGetNearestFreePathNode( pGame, pLevel, vPos, &pRoom );
		if ( node )
		{
			nNodeIndex = node->nIndex;
		}
	}
	else
	{
		nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );
	}

	// find ground location
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nCommander;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 15 );
	tSpec.pRoom = pRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_HOLDFAST * pData = sQuestDataGet( pQuest );

	if ( pData->nDRLG != pMessage->nDRLGDefId )
		return QMR_OK;

	if ( PStrCmp( pMessage->szRuleName, "Rule_22_Hold_Fast", CAN_RUN_DRLG_RULE_NAME_SIZE ) != 0 )
		return QMR_OK;

	if ( pMessage->nLevelDefId != pData->nLevelMB )
		return QMR_DRLG_RULE_FORBIDDEN;

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
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
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nArphaun ))
	{
		if ( ( eStatus == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		else if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_RETURN ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
		}
		return QMR_OK;
	}

	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTurrets ) && 
		 UnitCanInteractWith( pSubject, QuestGetPlayer( pQuest ) ) )
	{
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE1 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE2 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE3 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE4 ) == QUEST_STATE_ACTIVE ) ||
			 ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE5 ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nCommander ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_SITUATION ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	// quest activate
	if (UnitIsMonsterClass( pSubject, pQuestData->nArphaun ))
	{
		if ( ( eStatus == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_HOLDFAST_LORD );
			return QMR_NPC_TALK;
		}
		else if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_RETURN ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_HOLDFAST_LORD_RETURN );
				return QMR_NPC_TALK;
			}
			return QMR_OK;
		}
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTurrets ) && UnitCanInteractWith( pSubject, pPlayer ) )
	{
		// clear interactive and state
		s_StateClear( pSubject, UnitGetId( pSubject ), STATE_NPC_INFO_RETURN, 0 );
		UnitSetInteractive( pSubject, UNIT_INTERACTIVE_RESTRICED );

		// advance quest state
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE1 ) == QUEST_STATE_ACTIVE )
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_TALK_LORD, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_TRAVEL, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_SITUATION, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_AID, QUEST_STATE_ACTIVE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE1, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE2, QUEST_STATE_ACTIVE );
			return QMR_NPC_OPERATED;
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE2 ) == QUEST_STATE_ACTIVE )
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE2, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE3, QUEST_STATE_ACTIVE );
			return QMR_NPC_OPERATED;
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE3 ) == QUEST_STATE_ACTIVE )
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE3, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE4, QUEST_STATE_ACTIVE );
			return QMR_NPC_OPERATED;
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE4 ) == QUEST_STATE_ACTIVE )
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE4, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE5, QUEST_STATE_ACTIVE );
			return QMR_NPC_OPERATED;
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE5 ) == QUEST_STATE_ACTIVE )
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_AID, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE5, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_HOLDFAST_RETURN, QUEST_STATE_ACTIVE );
			sSetAllTurretsInactive( pQuest );
			return QMR_NPC_OPERATED;
		}

		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nCommander ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_SITUATION ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_HOLDFAST_COMMANDER );
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
		case DIALOG_HOLDFAST_LORD:
		{
			s_QuestActivate( pQuest );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_TALK_LORD, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_TALK_LORD, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_TRAVEL, QUEST_STATE_ACTIVE );
			// reveal path to battle
			s_QuestMapReveal( pQuest, GI_LEVEL_WHITEHALL );
			s_QuestMapReveal( pQuest, GI_LEVEL_DOWNING_ST );
			s_QuestMapReveal( pQuest, GI_LEVEL_MILLENNIUM );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HOLDFAST_COMMANDER:
		{
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_SITUATION, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_AID, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE1, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_HOLDFAST_LORD_RETURN:
		{
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_RETURN, QUEST_STATE_COMPLETE );
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
	// drb - kludge for playday
	/*	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
	if ( pMessage->nLevelNewDef == pQuestData->nLevelCCStation )
	{
		if ( s_QuestActivate( pQuest ) )
		{
			s_QuestStateRestore( pQuest, QUEST_STATE_REDOUT, QUEST_STATE_COMPLETE, TRUE );
			s_QuestStateRestore( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS, QUEST_STATE_COMPLETE, TRUE );
			s_QuestStateRestore( pQuest, QUEST_STATE_CHOCOLATE_TRAVEL_CP, QUEST_STATE_ACTIVE, TRUE );
			s_QuestMapReveal( pQuest, GI_LEVEL_ADMIRALTY_ARCH );
			s_QuestMapReveal( pQuest, GI_LEVEL_CHOCOLATE_PARK );
		}
	}

	return QMR_OK;
	}*/

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelMB )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_TRAVEL, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_HOLDFAST_SITUATION, QUEST_STATE_ACTIVE );
		}
		sSpawnAllTurrets( pQuest );
		sSpawnCommander( pQuest );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS c_sTurretInteractInfoSet(
	UNIT *pUnit,
	void *pCallbackData)
{
#if !ISVERSION(SERVER_VERSION)
	TURRET_CALLBACK_DATA *pTurretCallbackData = (TURRET_CALLBACK_DATA *)pCallbackData;
	INTERACT_INFO eDesiredInteractInfo = 
		pTurretCallbackData->bAllowTurretActivation ? 
			INTERACT_STORY_RETURN : INTERACT_INFO_NONE;

	if ( UnitIsMonsterClass( pUnit, pTurretCallbackData->nTurrets ) && 
		 UnitCanInteractWith( pUnit, pTurretCallbackData->pPlayer ) &&
		 c_NPCGetInfoState( pUnit ) != eDesiredInteractInfo )
	{
		c_InteractInfoSet( 
			UnitGetGame( pUnit ), 
			UnitGetId( pUnit ), 
			eDesiredInteractInfo );
	}
#endif //!ISVERSION(SERVER_VERSION)

	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT c_sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;

	QUEST_DATA_HOLDFAST * pQuestData = sQuestDataGet( pQuest );
	switch (nQuestState)
	{
	case QUEST_STATE_HOLDFAST_ACTIVATE1:
	case QUEST_STATE_HOLDFAST_ACTIVATE2:
	case QUEST_STATE_HOLDFAST_ACTIVATE3:
	case QUEST_STATE_HOLDFAST_ACTIVATE4:
	case QUEST_STATE_HOLDFAST_ACTIVATE5:
		// turrets need their interaction info updated, but this doesn't work 
		// with the quest cast stuff, since there all multiple units with the 
		// same monster class
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		TURRET_CALLBACK_DATA tTurretCBData;
		tTurretCBData.nTurrets = pQuestData->nTurrets;
		tTurretCBData.pPlayer = pPlayer;
		tTurretCBData.bAllowTurretActivation =
			 ( ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE1 ) == QUEST_STATE_ACTIVE ) ||
			   ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE2 ) == QUEST_STATE_ACTIVE ) ||
			   ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE3 ) == QUEST_STATE_ACTIVE ) ||
			   ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE4 ) == QUEST_STATE_ACTIVE ) ||
			   ( QuestStateGetValue( pQuest, QUEST_STATE_HOLDFAST_ACTIVATE5 ) == QUEST_STATE_ACTIVE ) );

		TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
		LevelIterateUnits( UnitGetLevel( pPlayer ), eTargetTypes, c_sTurretInteractInfoSet, &tTurretCBData );
		break;
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
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
		}

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
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return c_sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeHoldFast(
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
	QUEST_DATA_HOLDFAST * pQuestData)
{
	pQuestData->nDRLG						= QuestUseGI( pQuest, GI_DRLG_RIVER );
	pQuestData->nArphaun					= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nCommander					= QuestUseGI( pQuest, GI_MONSTER_HOLDFAST_CMDR );
	pQuestData->nLevelMB					= QuestUseGI( pQuest, GI_LEVEL_MILLENNIUM );
	pQuestData->nTurrets					= QuestUseGI( pQuest, GI_MONSTER_TURRET );
	pQuestData->nMiniBosses					= QuestUseGI( pQuest, GI_MONSTER_HOLDFAST_MINI_BOSS ); // big guys to zap with the turrets. The shaman_imp_vortex_goliath's glow is works well in the dark level -cmarch
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitHoldFast(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_HOLDFAST * pQuestData = ( QUEST_DATA_HOLDFAST * )GMALLOCZ( pGame, sizeof( QUEST_DATA_HOLDFAST ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nCommander );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreHoldFast(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
