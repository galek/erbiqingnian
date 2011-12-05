//----------------------------------------------------------------------------
// FILE: quest_hellrift.cpp
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
#include "quest_testmonkey.h"
#include "quests.h"
#include "dialog.h"
#include "uix.h"
#include "items.h"
#include "treasure.h"
#include "level.h"
#include "colors.h"
#include "c_font.h"
#include "room_layout.h"
#include "room_path.h"
#include "c_quests.h"
#include "s_quests.h"
#include "quest_truth_a.h"
#include "states.h"
#include "monsters.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_HELLRIFT_LOCATIONS	32
#define HELLRIFT_BOMB_FUSE_IN_SECONDS 30			// (in seconds)
#define HELLRIFT_RESET_DELAY_INITIAL_IN_TICKS	(GAME_TICKS_PER_SECOND * 60 * 2)
#define HELLRIFT_RESET_DELAY_IN_TICKS			(GAME_TICKS_PER_SECOND * 10)
	
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_TESTMONKEY
{

	int nHellriftLevel;
	int nHellriftRelicClass;
	int nHellriftBombClass;
	int nBombFuseTime;
	BOOL bEnteredHellrift;

	ROOM *pHellriftEntranceRoom;

	int nMonsterMurmur;

	int	nObjectRiftEnter;
	int nObjectRiftExit;

	int	nExplosionMonster;
	int nExplosionStatic;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TESTMONKEY * sQuestDataGet(
	QUEST * pQuest)
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return ( QUEST_DATA_TESTMONKEY * )pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAlivePlayersInHellrift( 
	GAME *pGame, 
	LEVEL *pLevel)
{

	// go through players
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); pClient; pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		ROOM *pRoom = UnitGetRoom( pPlayer );
		if (RoomIsHellrift( pRoom ))
		{
			if (IsUnitDeadOrDying( pPlayer ) == FALSE)
			{
				return TRUE;  // a player is alive in the hellrift
			}
		}
	}
	return FALSE;  // no players are alive in the hellrift for this level
}
				
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sResetSubLevel(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	LEVELID idLevel = (LEVELID)tEventData.m_Data1;
	LEVEL *pLevel = LevelGetByID( pGame, idLevel );
	SUBLEVELID idSubLevel = (SUBLEVELID)tEventData.m_Data2;
	
	// check to see if all players in the hellrift rooms are dead
	BOOL bAlivePlayersInHellrift = sAlivePlayersInHellrift( pGame, pLevel );
	if (bAlivePlayersInHellrift == TRUE)
	{
	
		// schedule event in the future to check again
		GameEventRegister( 
			pGame, 
			sResetSubLevel, 
			NULL, 
			NULL, 
			&tEventData, 
			HELLRIFT_RESET_DELAY_IN_TICKS);
					
	}
	else
	{

		// this hellrift level will allow resurrection again
		SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
		ASSERTX_RETTRUE( pSubLevel, "Expected sublevel" );
		SubLevelSetStatus( pSubLevel, SLS_RESURRECT_RESTRICTED_BIT, FALSE );
			
		// go reset sublevel rooms
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
			
			if (RoomGetSubLevelID( pRoom ) == idSubLevel)
			{
			
				// clear cannot reset flag
				RoomSetFlag( pRoom, ROOM_CANNOT_RESET_BIT, FALSE );
					
				// reset
				s_RoomReset( pGame, pRoom );
							
			}
			
		}
		
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreezeAndResetHellrift( 
	GAME *pGame,
	LEVEL *pLevel,
	SUBLEVELID idSubLevelHellrift)
{	

	// freeze all rooms in the hellrift
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if (RoomGetSubLevelID( pRoom ) == idSubLevelHellrift)
		{
			RoomSetFlag( pRoom, ROOM_CANNOT_RESET_BIT );
		}
		
	}
	
	// this hellrift will be able to be playable again in a couple of minutes, that should
	// be enough time for everybody left in the room to die and restart
	EVENT_DATA tEventData;
	tEventData.m_Data1 = LevelGetID( pLevel );
	tEventData.m_Data2 = idSubLevelHellrift;
	GameEventRegister( 
		pGame, 
		sResetSubLevel, 
		NULL, 
		NULL, 
		&tEventData, 
		HELLRIFT_RESET_DELAY_INITIAL_IN_TICKS);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveBomb(
	QUEST *pQuest,
	UNIT *pPlayer)
{
	QUEST_DATA_TESTMONKEY *ptQuestData = sQuestDataGet( pQuest );
	s_QuestRemoveItem( pPlayer, ptQuestData->nHellriftBombClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCompleteQuest(
	QUEST *pQuest,
	BOOL bEmergingFromHellrift)
{
	QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_EXIT, QUEST_STATE_COMPLETE );
	
	// complete the quest
	if (s_QuestComplete( pQuest ))
	{
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		
		// take any unplaced bombs out of the player inventory
		sRemoveBomb( pQuest, pPlayer );

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sExplosionDeath( UNIT * pMonster, void * pCallbackData )
{
	UnitDie( pMonster, NULL );
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomIsInHellrift(
	ROOM *pRoom,
	int nLevelDefHellrift)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	if (LevelGetDefinitionIndex( pLevel ) == nLevelDefHellrift)
	{	
		const SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
		const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
		if (pSubLevelDef->eType == ST_HELLRIFT)
		{
			return TRUE;
		}
		
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_STATIC_EXPLOSIONS			8
static void sSpawnHellriftExplode( 
	QUEST * pQuest,
	int nLevelDefHellrift)
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );

	// already spawned?
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nExplosionStatic ) )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pPlayer );
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		// is this a room in the hellrift sublevel?
		if (sRoomIsInHellrift( pRoom, nLevelDefHellrift ))
		{
		
			// first kill all the monsters
			RoomIterateUnits( pRoom, eTargetTypes, sExplosionDeath, NULL );

			// spawn some explosions!
			for ( int i = 0; i < NUM_STATIC_EXPLOSIONS; i++ )
			{
				int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );
				if (nNodeIndex < 0)
					return;

				// init location
				SPAWN_LOCATION tLocation;
				SpawnLocationInit( &tLocation, pRoom, nNodeIndex );

				MONSTER_SPEC tSpec;
				tSpec.nClass = pQuestData->nExplosionStatic;
				tSpec.nExperienceLevel = 1;
				tSpec.pRoom = pRoom;
				tSpec.vPosition = tLocation.vSpawnPosition;
				tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
				tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
				s_MonsterSpawn( pGame, tSpec );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBombExploded(
	UNIT *pPlayer,
	void *pCallbackData)
{	
	QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_TESTMONKEY );
	const QUEST_DATA_TESTMONKEY *ptQuestData = sQuestDataGet( pQuest );

	// complete quest if it's active and we've been into the hellrift
	if (pQuest && QuestIsActive( pQuest ))
	{
		if (ptQuestData->bEnteredHellrift == TRUE)
		{
			sCompleteQuest( pQuest, FALSE );
		}
	}

	// check if they are in the hellrift when the bomb goes off...
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if (pRoom && sRoomIsInHellrift( pRoom, ptQuestData->nHellriftLevel ))
	{
		// kill them...
		UnitDie( pPlayer, NULL );		
	}

	// kill stuff in the hellrift
	sSpawnHellriftExplode( pQuest, ptQuestData->nHellriftLevel );
			
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBombTimer( 
	GAME *pGame, 
	UNIT *pUnit, 
	const struct EVENT_DATA &tEventData )
{
	ASSERT_RETFALSE( pGame && pUnit );
	ASSERT( IS_SERVER( pGame ) );
	
	// this quest is complete for everybody who entered the hellrift at all
	PlayersIterate( pGame, sBombExploded, NULL );

	// the hellrift sublevel is the sublevel with the relic device that is blowing up
	SUBLEVEL *pSubLevelHellrift = UnitGetSubLevel( pUnit );
	SUBLEVELID idSubLevelHellrift = SubLevelGetId( pSubLevelHellrift );
	
	// close the and exit portals to the hellrift sub level 
	// note that we will recreate them both later when the hellrift resets
	LEVEL *pLevel = UnitGetLevel( pUnit );
	const SUBLEVEL_DOORWAY *pSubLevelEntrance = s_SubLevelGetDoorway( pSubLevelHellrift, SD_ENTRANCE );
	ASSERTX_RETFALSE( pSubLevelEntrance, "Expected sublevel entrance" );
	UNIT *pEntrance = UnitGetById( pGame, pSubLevelEntrance->idDoorway );
	if (pEntrance)
	{
		s_StateSet( pEntrance, pEntrance, STATE_QUEST_HIDDEN, 0 );
	}
	
	const SUBLEVEL_DOORWAY *pSubLevelExit = s_SubLevelGetDoorway( pSubLevelHellrift, SD_EXIT );
	ASSERTX_RETFALSE( pSubLevelExit, "Expected sublevel exit" );
	UNIT *pExit = UnitGetById( pGame, pSubLevelExit->idDoorway );
	if (pExit)
	{
		s_StateSet( pExit, pExit, STATE_QUEST_HIDDEN, 0 );
	}
		
	// freeze all sublevel rooms and start timer to respawn after a little bit in
	// multiplayer only
	if (AppIsMultiplayer())
	{
		sFreezeAndResetHellrift( pGame, pLevel, idSubLevelHellrift );
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHellriftUpdateFuseDisplay( QUEST * pQuest )
{
#if !ISVERSION(SERVER_VERSION)

	QUEST_DATA_TESTMONKEY * data = sQuestDataGet( pQuest );
	const int MAX_STRING = 1024;
	WCHAR timestr[ MAX_STRING ] = L"\0";

	
	if ( data->nBombFuseTime > 1 )
	{
		const WCHAR *puszFuseString = GlobalStringGet( GS_HELLRIFT_FUSE_MULTI_SECOND );
		
		PStrPrintf( 
			timestr, 
			MAX_STRING, 
			puszFuseString,
			//L"%c%cEscape! Exit portal will collapse in: %i seconds", 
			data->nBombFuseTime );
	}
	else
	{
		PStrCopy(timestr, GlobalStringGet( GS_HELLRIFT_FUSE_SINGLE_SECOND ), MAX_STRING);
	}
	
	// show on screen
	UIShowQuickMessage( timestr );
#endif //!ISVERSION(SERVER_VERSION)

	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sFuseUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_CLIENT( game ) );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	QUEST_DATA_TESTMONKEY * data = sQuestDataGet( pQuest );
	data->nBombFuseTime--;
	if ( data->nBombFuseTime )
	{
		sHellriftUpdateFuseDisplay( pQuest );
		UnitRegisterEventTimed( unit, sFuseUpdate, &tEventData, GAME_TICKS_PER_SECOND );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		UIHideQuickMessage();
#endif// !ISVERSION(SERVER_VERSION)
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{

	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
		ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
		ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
						
		if (s_QuestIsGiver( pQuest, pSubject ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TESTMONKEY_LANN );
			return QMR_NPC_TALK;
		}

	}
		
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NUM_EXPLOSIONS			8

static void sSpawnExplosions( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );
	ROOM * pRoom = UnitGetRoom( pQuest->pPlayer );
	GAME * pGame = RoomGetGame( pRoom );
	for ( int i = 0; i < NUM_EXPLOSIONS; i++ )
	{
		// spawn nearby
		ROOM * pSpawnRoom = NULL;	
		ROOM_LIST tNearbyRooms;
		RoomGetNearbyRoomList(pRoom, &tNearbyRooms);
		if ( tNearbyRooms.nNumRooms)
		{
			int index = RandGetNum( pGame->rand, tNearbyRooms.nNumRooms - 1 );
			pSpawnRoom = tNearbyRooms.pRooms[index];
		}
		else
		{
			pSpawnRoom = pRoom;
		}

		// and spawn the explosions
		int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
		if (nNodeIndex < 0)
			return;

		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nExplosionMonster;
		tSpec.nExperienceLevel = 1;
		tSpec.pRoom = pSpawnRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasBomb(
	void *pUserData)
{	
	QUEST *pQuest = (QUEST *)pUserData;
	
	// activate quest
	if ( s_QuestActivate( pQuest ) )
	{
		// set state				
		QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_BLOOMSBURY, QUEST_STATE_ACTIVE );
		
		// reveal points of interest
		s_QuestMapReveal( pQuest, GI_LEVEL_BLOOMSBURY );
		s_QuestMapReveal( pQuest, GI_LEVEL_BRITISH_MUSEUM );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	UNIT *pPlayer = pQuest->pPlayer;
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_TESTMONKEY_LANN:
		case DIALOG_TESTMONKEY_ALAY:
		case DIALOG_TESTMONKEY_NASIM:
		{

			// give hellrift bomb
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasBomb;
			tActions.pAllTakenUserData = pQuest;
			s_OfferGive( pPlayer, pTalkingTo, OFFERID_QUEST_TESTMONKEY_BOMB, tActions );

			break;
			
		}
		//----------------------------------------------------------------------------
		case DIALOG_TESTMONKEY_PLACE_DEVICE:
		{
			ASSERT_RETVAL( QuestStateIsActive( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ), QMR_OK );
			QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );
			ASSERT_RETVAL( s_QuestCheckForItem( pPlayer, pQuestData->nHellriftBombClass ), QMR_OK );

			// This is the delay theory dave would like, but I think it looks strange to have a delay
			//int nDelayInTicks = GAME_TICKS_PER_SECOND * 3;
			//s_QuestOperateDelay( pQuest, sUseBomb, UnitGetId( pPlayer ),  );
			
			// remove bomb
			s_StateSet( pPlayer, pPlayer, STATE_OPERATE_OBJECT_FX, 0 );

			sRemoveBomb( pQuest, pPlayer );
		
			// set state on relic
			s_StateSet( pTalkingTo, pTalkingTo, STATE_FAWKES_DEVICE_ATTACHED, 0 );

			// this hellrift sublevel no longer allows resurrects (because people will get
			// stuck in them after the entrance goes away)
			SUBLEVEL *pSubLevel = UnitGetSubLevel( pTalkingTo );
			ASSERTX( pSubLevel, "Expected sublevel" );
			if (pSubLevel)
			{
				SubLevelSetStatus( pSubLevel, SLS_RESURRECT_RESTRICTED_BIT, TRUE );
			}
										
			// the bomb has been activated, set this state for all who have the quest active
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TESTMONKEY_EXIT, QUEST_STATE_ACTIVE );
			
			// set timer for blow-up
			UnitRegisterEventTimed( pTalkingTo, sBombTimer, NULL, HELLRIFT_BOMB_FUSE_IN_SECONDS * GAME_TICKS_PER_SECOND );

			// spawn explosion monsters
			sSpawnExplosions( pQuest );
			break;
			
		}
		
	}

	return QMR_OK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sClientQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	if ( ( nQuestState == QUEST_STATE_TESTMONKEY_PLACE_DEVICE ) &&
		 ( eValue == QUEST_STATE_COMPLETE ) )
		 //( QuestStateGetValue( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ) < QUEST_STATE_COMPLETE ) )
	{
		// set display timer
		QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );
		pQuestData->nBombFuseTime = HELLRIFT_BOMB_FUSE_IN_SECONDS;
		sHellriftUpdateFuseDisplay( pQuest );
		UnitRegisterEventTimed( 
			pQuest->pPlayer, 
			sFuseUpdate, 
			&EVENT_DATA( (DWORD_PTR)pQuest ), 
			GAME_TICKS_PER_SECOND);
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
	QUEST_DATA_TESTMONKEY *pQuestData = sQuestDataGet( pQuest );
		
	if ( UnitIsObjectClass( pObject, pQuestData->nObjectRiftEnter ) || UnitIsObjectClass( pObject, pQuestData->nObjectRiftExit ) )
	{
		if ( UnitHasState( pGame, pObject, STATE_QUEST_HIDDEN ) )
		{
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
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );
	UNIT *pPlayer = pQuest->pPlayer;
	UNIT *pTarget = pMessage->pTarget;
	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nHellriftRelicClass )
	{
		if (QuestStateIsActive( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ))
		{
			// check for bomb in inventory
			if ( s_QuestCheckForItem( pPlayer, pQuestData->nHellriftBombClass ) )
			{
				s_NPCQuestTalkStart( pQuest, pTarget, NPC_DIALOG_OK, DIALOG_TESTMONKEY_PLACE_DEVICE );
			}
			else
			{
				s_NPCQuestTalkStart( pQuest, pTarget, NPC_DIALOG_OK, DIALOG_TESTMONKEY_DEVICE_NEEDED );
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
		QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if (UnitIsMonsterClass( pSubject, pQuestData->nHellriftRelicClass ))
		{
			if (QuestStateIsActive( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ))
			{
				return QMR_NPC_STORY_RETURN;
			}
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckHellriftStates( QUEST * pQuest )
{
	// this will probably need some extra stuff for multiplayer, so make it single for now...
	if ( AppIsMultiplayer() )
		return;

	if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ) < QUEST_STATE_COMPLETE )
	{
		// make sure that everything is set
		UNIT * pPlayer = pQuest->pPlayer;
		LEVEL * pLevel = UnitGetLevel( pPlayer );
		QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );

		// find the relic
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT * pRelic = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nHellriftRelicClass );
		if ( pRelic )
		{
			// clear state on relic
			s_StateClear( pRelic, UnitGetId( pRelic ), STATE_FAWKES_DEVICE_ATTACHED, 0 );

			// make sure the timer callback is gone
			if ( UnitHasTimedEvent( pRelic, sBombTimer ) )
			{
				UnitUnregisterTimedEvent( pRelic, sBombTimer );
			}
		}

		// find the hellrift entrance
		UNIT * pEntrance = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nObjectRiftEnter );
		if ( pEntrance )
		{
			// clear state on relic
			s_StateClear( pEntrance, UnitGetId( pEntrance ), STATE_QUEST_HIDDEN, 0 );
		}

		// find the hellrift exit
		UNIT * pExit = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nObjectRiftExit );
		if ( pExit )
		{
			// clear state on relic
			s_StateClear( pExit, UnitGetId( pExit ), STATE_QUEST_HIDDEN, 0 );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	ASSERT_RETVAL( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), QMR_OK );

	QUEST_DATA_TESTMONKEY * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nHellriftLevel )
	{		
		// entered the level with the hellrift they are supposed to find
		QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_BLOOMSBURY, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_ENTER, QUEST_STATE_ACTIVE );
		sCheckHellriftStates( pQuest );	
	}

	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageSubLevelTransition( 
	QUEST *pQuest,
	const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pMessage)
{
	QUEST_DATA_TESTMONKEY *ptQuestData = sQuestDataGet( pQuest );
	
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	LEVEL *pLevel = UnitGetLevel( pPlayer );	
	if (ptQuestData->nHellriftLevel == LevelGetDefinitionIndex( pLevel ))
	{
	
		// entering hellrift
		SUBLEVEL *pSubLevelNew = pMessage->pSubLevelNew;
		if (SubLevelGetType( pSubLevelNew ) == ST_HELLRIFT)
		{
		
			// save that we've entered the hellrift
			ptQuestData->bEnteredHellrift = TRUE;
		
			// advance states
			QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_ENTER, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE, QUEST_STATE_ACTIVE );

			// save the room that has the entrace of the hellrift
			ptQuestData->pHellriftEntranceRoom = pMessage->pOldRoom;
			
		}
		
		// exiting hellrift
		SUBLEVEL *pSubLevelOld = pMessage->pSubLevelOld;
		if (pSubLevelOld && SubLevelGetType( pSubLevelOld ) == ST_HELLRIFT)
		{
			if ( QuestStateIsComplete( pQuest, QUEST_STATE_TESTMONKEY_PLACE_DEVICE ) )
			{
				// complete quest
				sCompleteQuest( pQuest, TRUE );
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
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sClientQuestMesssageStateChanged( pQuest, pStateMessage );		
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
		
		//----------------------------------------------------------------------------
		case QM_SERVER_SUB_LEVEL_TRANSITION:
		{
			const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pSubLevelMessage = (const QUEST_MESSAGE_SUBLEVEL_TRANSITION *)pMessage;
			return sQuestMessageSubLevelTransition( pQuest, pSubLevelMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTestMonkey(
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
static void sQuestDataTestMonkeyInit(
	QUEST *pQuest,
	QUEST_DATA_TESTMONKEY * pQuestData)
{	
	pQuestData->nHellriftLevel		= QuestUseGI( pQuest, GI_LEVEL_BRITISH_MUSEUM );
	pQuestData->nHellriftRelicClass = QuestUseGI( pQuest, GI_OBJECT_HELLRIFT_RELIC );
	pQuestData->nHellriftBombClass	= QuestUseGI( pQuest, GI_ITEM_HELLRIFT_BOMB );
	pQuestData->nMonsterMurmur		= QuestUseGI( pQuest, GI_MONSTER_MURMUR );

	pQuestData->nObjectRiftEnter	= QuestUseGI( pQuest, GI_OBJECT_HELLBOMB_ENTER );
	pQuestData->nObjectRiftExit		= QuestUseGI( pQuest, GI_OBJECT_HELLBOMB_EXIT );

	pQuestData->nExplosionMonster	= QuestUseGI( pQuest, GI_MONSTER_EXPLOSION );
	pQuestData->nExplosionStatic	= QuestUseGI( pQuest, GI_MONSTER_EXPLOSION_STATIC );
	
	pQuestData->nBombFuseTime = HELLRIFT_BOMB_FUSE_IN_SECONDS;
	pQuestData->bEnteredHellrift = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestMonkey(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
	
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTMONKEY * pQuestData = (QUEST_DATA_TESTMONKEY *)GMALLOCZ( pGame, sizeof( QUEST_DATA_TESTMONKEY ) );
	sQuestDataTestMonkeyInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
	
	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMonsterMurmur );	
	s_QuestAddCastMember( pQuest, GENUS_OBJECT, pQuestData->nHellriftRelicClass );

}
