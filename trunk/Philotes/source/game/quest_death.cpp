//----------------------------------------------------------------------------
// FILE: quest_death.cpp
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
#include "quest_death.h"
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
#include "e_region.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_DEATH
{
	int		nDRLG;
	int		nDeathWounded;
	int		nDeath;
	int		nDeadTech;
	int		nTemplarBody;
	int		nTechBody;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_DEATH * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_DEATH *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// client-side only dead units...

static void sSpawnDeadPeople(
	QUEST * pQuest )
{
	return;

/*	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );

	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );

	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		float room_size = room->pHull->aabb.halfwidth.fX * room->pHull->aabb.halfwidth.fY;
		int density = (int)FLOOR( room_size * 0.05f );
		int nNumSpawns = ( RandGetNum( pGame->rand, density / 2 ) + density / 2 ) + 1;
		for ( int s = 0; s < nNumSpawns; s++ )
		{

			// and spawn death
			int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, room, 0 );
			if(nNodeIndex < 0)
				continue;

			// init location
			SPAWN_LOCATION tLocation;
			float fAngle = RandGetFloat( pGame, -180.0f, 180.0f);
			SpawnLocationInit( &tLocation, room, nNodeIndex, &fAngle );

			int nMonsterClass;
			if ( RandGetNum( pGame->rand, 100 ) > 10 )
				nMonsterClass = pQuestData->nTemplarBody;
			else
				nMonsterClass = pQuestData->nTechBody;

			MonsterSpawnClient( pGame, nMonsterClass, room, tLocation.vSpawnPosition, tLocation.vSpawnDirection );
		}
	}*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnDeathAndNPC(
	GAME * pGame,
	QUEST * pQuest )
{
	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );

	// check the level for death
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	ROOM * portal_room = NULL;
	ROOM * hold_room = NULL;
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			int test_class = UnitGetClass( test );
			if ( test_class == pQuestData->nDeath )
			{
				// found one already
				return;
			}
			if (UnitIsA( test, UNITTYPE_WARP_NEXT_LEVEL ))
			{
				portal_room = room;
			}
		}
		if ( room->idSubLevel == 0 )
		{
			hold_room = room;
		}
	}

	// spawn nearby
	ROOM * room = UnitGetRoom( pQuest->pPlayer );
	ROOM * pSpawnRoom = NULL;
	int nNodeIndex = INVALID_ID;

	// get nearby rooms	
	ROOM_LIST tNearbyRooms;
	RoomGetNearbyRoomList(room, &tNearbyRooms);
	if (tNearbyRooms.nNumRooms)	
	{
		for (int i=0; i<tNearbyRooms.nNumRooms; i++)
		{
			ASSERT_BREAK(i < MAX_ROOMS_PER_LEVEL);
			pSpawnRoom = tNearbyRooms.pRooms[ i ];
			nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
			if (nNodeIndex >= 0)
			{
				break;
			}
		}
	}
	else
	{
		pSpawnRoom = room;
		nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
	}

	// and spawn death
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nDeath;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 8 ); 
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );

	// and spawn dead tech (*sniff*)
	if ( !portal_room )
	{
		portal_room = hold_room;
	}
	if (!portal_room)
	{
		SUBLEVEL *pSubLevel = LevelGetPrimarySubLevel( pLevel );
		DWORD dwRandomRoomFlags = 0;
		SETBIT(dwRandomRoomFlags, RRF_PATH_NODES_BIT);
		portal_room = s_SubLevelGetRandomRoom(pSubLevel, dwRandomRoomFlags, NULL, NULL);
	}
	ASSERT_RETURN( portal_room );
	nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, portal_room, 0 );
	if (nNodeIndex < 0)
		return;

	// init location
	SpawnLocationInit( &tLocation, portal_room, nNodeIndex );

	MonsterSpecInit( tSpec );
	tSpec.nClass = pQuestData->nDeadTech;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 1 );
	tSpec.pRoom = portal_room;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartDeath( QUEST * pQuest, UNIT * pTemplar )
{
	ASSERT_RETURN( pQuest && pTemplar );
	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );
	REF(pQuestData);
	GAME * game = UnitGetGame( pTemplar );
	ASSERT_RETURN( IS_SERVER( game ) );

	// spawn death!
	sSpawnDeathAndNPC( game, pQuest );
	// set new env
	CLT_VERSION_ONLY(c_SetEnvironmentDef( e_GetCurrentRegionID(), "sewers\\death_env.xml", "", TRUE ));
	QuestStateSet( pQuest, QUEST_STATE_DEATH_FIND_WEAPON, QUEST_STATE_ACTIVE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasGun(
	void *pUserData)

{
	QUEST *pQuest = (QUEST *)pUserData;
	
	// finished getting leg for now...
	QuestStateSet( pQuest, QUEST_STATE_DEATH_FIND_WEAPON, QUEST_STATE_COMPLETE );

	// activate rest of quest
	QuestStateSet( pQuest, QUEST_STATE_DEATH_KILL_DEATH, QUEST_STATE_ACTIVE );	
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
	QUEST_DATA_DEATH * pData = sQuestDataGet( pQuest );
	REF(pData);
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if (UnitIsMonsterClass( pSubject, pData->nDeathWounded ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_FIND_WEAPON ) == QUEST_STATE_HIDDEN )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEATH_INIT );
			return QMR_NPC_TALK;
		}
		else if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_KILL_DEATH ) == QUEST_STATE_COMPLETE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEATH_COMPLETE );
			return QMR_NPC_TALK;
		}
		else
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DEATH_ACTIVE );
			return QMR_NPC_TALK;
		}
	}
	else if (UnitIsMonsterClass( pSubject, pData->nDeadTech ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_FIND_WEAPON ) == QUEST_STATE_ACTIVE )
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasGun;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pSubject, OFFERID_QUEST_DEATH_TECH_BODY, tActions);
			return QMR_OFFERING;
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
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;
	REF(nDialogStopped);
	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );
	REF(pQuestData);

	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_DEATH_INIT:
		{
			sStartDeath( pQuest, pTalkingTo );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DEATH_COMPLETE:
		{
			// finish  up the quest
			QuestStateSet( pQuest, QUEST_STATE_DEATH_TALK, QUEST_STATE_COMPLETE );
			if ( s_QuestComplete( pQuest ) )
			{
				// give reward
				if (s_QuestOfferReward( pQuest, pTalkingTo ))
				{
					return QMR_OFFERING;
				}
				
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

	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if (UnitIsMonsterClass( pSubject, pQuestData->nDeathWounded ))
	{
		if ( eStatus >= QUEST_STATUS_COMPLETE )
		{
			return QMR_OK;
		}
		else
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_FIND_WEAPON ) == QUEST_STATE_HIDDEN )
			{
				return QMR_NPC_INFO_NEW;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_TALK ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_RETURN;
			}
			return QMR_NPC_INFO_WAITING;
		}
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nDeadTech ))
	{
		if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DEATH_FIND_WEAPON ) != QUEST_STATE_ACTIVE )
			{
				return QMR_OK;
			}
			return QMR_NPC_INFO_RETURN;
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

	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	if ( UnitIsMonsterClass( pVictim, pQuestData->nDeath ))
	{
		QuestStateSet( pQuest, QUEST_STATE_DEATH_KILL_DEATH, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_DEATH_TALK, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_DEATH * pData = sQuestDataGet( pQuest );

	if ( ( pData->nDRLG == pMessage->nDRLGDefId ) && ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE ) )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
/*	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_DEATH * pQuestData = sQuestDataGet( pQuest );
	if ( ( pMessage->nLevelNewDRLGDef == pQuestData->nDRLG ) && ( QuestGetStatus( pQuest ) < QUEST_STATUS_COMPLETE ) )
	{
		// activate quest
		s_QuestActivateForAll( pQuest );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEATH_ACTIVATE, QUEST_STATE_HIDDEN );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEATH_ACTIVATE, QUEST_STATE_ACTIVE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEATH_ACTIVATE, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DEATH_FIND_WEAPON, QUEST_STATE_HIDDEN );
	}*/
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	if ( nQuestState == QUEST_STATE_DEATH_ACTIVATE && eValue == QUEST_STATE_COMPLETE )
	{
		// spawn the dead folks
		sSpawnDeadPeople( pQuest );
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
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pUIKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pUIKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
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
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeDeath(
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
	QUEST_DATA_DEATH * pQuestData)
{
	pQuestData->nDRLG			= QuestUseGI( pQuest, GI_DRLG_SEWERSB );
	pQuestData->nDeathWounded	= QuestUseGI( pQuest, GI_MONSTER_DEATH_WOUNDED );
	pQuestData->nDeath			= QuestUseGI( pQuest, GI_MONSTER_DEATH );
	pQuestData->nDeadTech		= QuestUseGI( pQuest, GI_MONSTER_DEATH_DEADTECH );
	pQuestData->nTechBody		= QuestUseGI( pQuest, GI_MONSTER_DEATH_TECHBODY );
	pQuestData->nTemplarBody	= QuestUseGI( pQuest, GI_MONSTER_DEATH_TEMPLARBODY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitDeath(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_DEATH * pQuestData = ( QUEST_DATA_DEATH * )GMALLOCZ( pGame, sizeof( QUEST_DATA_DEATH ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDeadTech );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreDeath(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

