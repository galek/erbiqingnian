//----------------------------------------------------------------------------
// FILE: s_questgames.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "game.h"
#include "globalindex.h"
#include "quests.h"
#include "s_questgames.h"
#include "clients.h"
#include "s_message.h"
#include "states.h"
#include "room_path.h"
#include "room_layout.h"
#include "unitmodes.h"
#include "s_quests.h"
#include "monsters.h"
#include "quest_rescueride.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define QUEST_GAME_START_DELAY				5				// in seconds

struct QUEST_GAME_DATA
{
	// back links
	GAME *	pGame;
	QUEST * pQuest;

	// data
	BOOL	bInGame;
	int		nNewGameDelay;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGamesInit(
	UNIT * pPlayer )
{
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	if ( UnitHasState( pGame, pPlayer, STATE_QUEST_IN_QUESTGAME ) )
	{
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_IN_QUESTGAME, 0 );
	}
	if ( UnitHasState( pGame, pPlayer, STATE_QUEST_GAME_ARENA_RESPAWN ) )
	{
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_GAME_ARENA_RESPAWN, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameDataInit(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest && !pQuest->pGameData );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_GAME_DATA * pGameData = ( QUEST_GAME_DATA * )GMALLOCZ( pGame, sizeof( QUEST_GAME_DATA ) );
	pGameData->pGame = pGame;
	pGameData->pQuest = pQuest;

	pGameData->bInGame = FALSE;
	pGameData->nNewGameDelay = 0;

	pQuest->pGameData = pGameData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameDataFree(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest && pQuest->pGameData );
	GAME * pGame = pQuest->pGameData->pGame;
	ASSERT_RETURN( pGame );
	GFREE( pGame, pQuest->pGameData );
	pQuest->pGameData = NULL;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sQuestGameSendMessageToAll(
	QUEST * pQuest,
	QUEST_GAME_MESSAGE_TYPE eMessageType,
	UNIT * pUnit = NULL,
	DWORD dwData = 0)
{
	ASSERT_RETURN( pQuest );
	QUEST_GAME_DATA * pGameData = pQuest->pGameData;
	ASSERTX_RETURN( pGameData, "Quest Game Data not initialized" );
	ASSERTX_RETURN( IS_SERVER( pGameData->pGame ), "Server only" );

	int nQuestId = pQuest->nQuest;

	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGameData->pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, nQuestId );
		if ( !quest )
			continue;

		ASSERTX_CONTINUE( quest->pGameData, "Expected Quest Game Data" );
		if ( !quest->pGameData->bInGame )
			continue;

		// setup message
		QUEST_MESSAGE_GAME_MESSAGE tMessage;
		tMessage.nCommand = (int)eMessageType;
		tMessage.pUnit = pUnit;
		tMessage.dwData = dwData;

		// send to quest this quest
		QuestSendMessageToQuest( quest, QM_SERVER_GAME_MESSAGE, &tMessage );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestGameSendMessage(
	QUEST * pQuest,
	QUEST_GAME_MESSAGE_TYPE eMessageType,
	UNIT * pUnit = NULL,
	DWORD dwData = 0)
{
	ASSERT_RETVAL( pQuest, QMR_INVALID );
	ASSERTX_RETVAL( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), QMR_INVALID, "Server only" );

	// setup message
	QUEST_MESSAGE_GAME_MESSAGE tMessage;
	tMessage.nCommand = (int)eMessageType;
	tMessage.pUnit = pUnit;
	tMessage.dwData = dwData;

	// send to quest this quest
	return QuestSendMessageToQuest( pQuest, QM_SERVER_GAME_MESSAGE, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNIT * s_QuestGameGetFirstPlayer(
	GAME * pGame,
	int nQuestId )
{
	ASSERT_RETNULL( ( nQuestId >= 0 ) && ( nQuestId < NUM_QUESTS ) );
	ASSERTX_RETNULL( IS_SERVER( pGame ), "Server only" );

	// double check to see if someone has already initialized it
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, nQuestId );
		if ( !quest )
			continue;

		QUEST_GAME_DATA * game_data = quest->pGameData;
		ASSERTX_CONTINUE( game_data, "Missing Quest Game Data" );

		if ( game_data->bInGame )
		{
			return player;
		}
	}

	// nope?
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSendGameDelayString( QUEST * pQuest, QUEST_GAME_DATA * pGameData )
{
	GLOBAL_STRING eString;
	if ( pGameData->nNewGameDelay == 1 )
		eString = GS_TOK_BEGIN_SINGLE_SECOND;
	else
		eString = GS_TOK_BEGIN_MULTI_SECOND;

	s_QuestGameUIMessage( pQuest, QUIM_GLOBAL_STRING_WITH_INT, (int)eString, pGameData->nNewGameDelay );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sQuestGameStarting( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game );

	int nQuestId = (int)tEventData.m_Data1;
	int nCount = -1;

	BOOL bSomeoneStillInGame = FALSE;
	for (GAMECLIENT *pClient = ClientGetFirstInGame( game );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, nQuestId );
		if ( !quest )
			continue;

		QUEST_GAME_DATA * game_data = quest->pGameData;
		ASSERTX_CONTINUE( game_data, "Quest Game Data not initialized" );

		if ( game_data->bInGame )
		{
			bSomeoneStillInGame = TRUE;
			// send UI message for beginning
			game_data->nNewGameDelay--;
			if ( nCount == -1 )
			{
				nCount = game_data->nNewGameDelay;
			}
			ASSERTX( nCount == game_data->nNewGameDelay, "Quest games don't have same start delay" );
			if ( nCount == 0 )
			{
				s_StateClear( player, UnitGetId( player ), STATE_QUEST_GAME_STARTING, 0 );
				sQuestGameSendMessageToAll( quest, QGM_GAME_START );
			}
			else
			{
				sSendGameDelayString( quest, game_data );
			}
		}
	}

	if ( bSomeoneStillInGame && ( nCount > 0 ) )
	{
		UnitRegisterEventTimed( unit, sQuestGameStarting, &tEventData, GAME_TICKS_PER_SECOND );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameJoin(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_GAME_DATA * pGameData = pQuest->pGameData;
	ASSERTX_RETURN( pGameData, "Quest Game Data not initialized" );
	ASSERTX_RETURN( IS_SERVER( pGameData->pGame ), "Server only" );

	UNIT * pPlayer = pQuest->pPlayer;
	UNIT * pOther = s_QuestGameGetFirstPlayer( pGameData->pGame, pQuest->nQuest );
	if ( !pOther )
	{
		// setup new quest game starting data
		sQuestGameSendMessage( pQuest, QGM_GAME_NEW );
		pGameData->nNewGameDelay = QUEST_GAME_START_DELAY;
		// set callback events for delay
		LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT * pStartLoc = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_QUEST_GAME_START_LOCATION ) );
		if ( pStartLoc )
		{
			UnitRegisterEventTimed( pStartLoc, sQuestGameStarting, &EVENT_DATA( pQuest->nQuest ), GAME_TICKS_PER_SECOND );
		}
		else
		{
			// if no start location, game will begin right away
			pGameData->nNewGameDelay = 0;
		}
	}
	else
	{
		sQuestGameSendMessage( pQuest, QGM_GAME_COPY_CURRENT_STATE, pOther );
		QUEST * pQuestOther = QuestGetQuest( pOther, pQuest->nQuest, FALSE );
		ASSERT( pQuestOther );
		if ( pQuestOther )
		{
			QUEST_GAME_DATA * pGameDataOther = pQuestOther->pGameData;
			ASSERT( pGameDataOther );
			if ( pGameDataOther )
				pGameData->nNewGameDelay = pGameDataOther->nNewGameDelay;
		}
	}
	pGameData->bInGame = TRUE;
	s_StateSet( pQuest->pPlayer, pQuest->pPlayer, STATE_QUEST_IN_QUESTGAME, 0 );
	sQuestGameSendMessage( pQuest, QGM_GAME_JOIN );
	if ( pGameData->nNewGameDelay > 0 )
	{
		s_StateSet( pPlayer, pPlayer, STATE_QUEST_GAME_STARTING, 0 );
		sSendGameDelayString( pQuest, pGameData );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameLeave(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_GAME_DATA * pGameData = pQuest->pGameData;
	ASSERTX_RETURN( pGameData, "Quest Game Data not initialized" );
	ASSERTX_RETURN( IS_SERVER( pGameData->pGame ), "Server only" );

	if ( pGameData->bInGame )
	{
		sQuestGameSendMessage( pQuest, QGM_GAME_LEAVE );
		// i am no longer in...
		pGameData->bInGame = FALSE;
		s_StateClear( pQuest->pPlayer, UnitGetId( pQuest->pPlayer ), STATE_QUEST_IN_QUESTGAME, 0 );
		s_StateClear( pQuest->pPlayer, UnitGetId( pQuest->pPlayer ), STATE_QUEST_GAME_STARTING, 0 );
		s_StateClear( pQuest->pPlayer, UnitGetId( pQuest->pPlayer ), STATE_QUEST_GAME_ARENA_RESPAWN, 0 );
		// anyone left?
		if ( s_QuestGameGetFirstPlayer( pGameData->pGame, pQuest->nQuest ) == NULL )
		{
			sQuestGameSendMessage( pQuest, QGM_GAME_DESTROY );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sQuestGameAllLeave(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_GAME_DATA * pGameData = pQuest->pGameData;
	ASSERTX_RETURN( pGameData, "Quest Game Data not initialized" );
	ASSERTX_RETURN( IS_SERVER( pGameData->pGame ), "Server only" );

	int nQuestId = pQuest->nQuest;

	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGameData->pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, nQuestId );
		if ( !quest )
			continue;

		s_QuestGameLeave( quest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameVictory(
	QUEST * pQuest )
{
	sQuestGameSendMessageToAll( pQuest, QGM_GAME_VICTORY );
	sQuestGameAllLeave( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameFailed(
	QUEST * pQuest )
{
	sQuestGameSendMessageToAll( pQuest, QGM_GAME_FAILED );
	sQuestGameAllLeave( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameCustomMessage(
	QUEST * pQuest,
	QUEST_GAME_MESSAGE_TYPE eMessageType,
	UNIT * pUnit,
	DWORD dwData )
{
	ASSERTX_RETURN( eMessageType >= QGM_CUSTOM_LIST_BEGIN, "Expecting custom messages only" );
	sQuestGameSendMessageToAll( pQuest, eMessageType, pUnit, dwData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL QuestGamePlayerInGame(
	UNIT * pPlayer )
{
	ASSERT_RETFALSE( pPlayer );
	return UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_IN_QUESTGAME );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL c_QuestGamePlayerArenaRespawn(
	UNIT * pPlayer )
{
	ASSERT_RETFALSE( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( IS_CLIENT( pGame ) );
	return ( UnitHasState( pGame, pPlayer, STATE_QUEST_IN_QUESTGAME ) && UnitHasState( pGame, pPlayer, STATE_QUEST_GAME_ARENA_RESPAWN ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL s_QuestGamePlayerArenaRespawn(
	UNIT * pPlayer )
{
	ASSERT_RETFALSE( pPlayer );
	if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_QUEST_IN_QUESTGAME ) )
	{
		QUEST * pQuest = NULL;
		for ( int q = 0; q < NUM_QUESTS; q++ )
		{
			pQuest = QuestGetQuest( pPlayer, q );
			if ( !pQuest )
				continue;
			if ( pQuest->pGameData && pQuest->pGameData->bInGame )
				q = NUM_QUESTS;
			else
				pQuest = NULL;
		}
		ASSERTX_RETFALSE( pQuest, "Couldn't find quest game for player" );
		QUEST_MESSAGE_RESULT eResult = sQuestGameSendMessage( pQuest, QGM_GAME_ARENA_RESPAWN );
		if ( eResult == QMR_OK )
			return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// if the game ends while the player is dead, respawn them.
// (Further code will warp them to victory/fail location)
void s_QuestGameRespawnDeadPlayer(
	UNIT * pPlayer )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( QuestGamePlayerInGame( pPlayer ) );

	if ( IsUnitDeadOrDying( pPlayer ) == FALSE )
	{
		return;
	}

	s_PlayerRestart( pPlayer, PLAYER_RESPAWN_ARENA );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameUIMessage(
	QUEST * pQuest,
	QUEST_UI_MESSAGE_TYPE eUIMessageType,
	int nDialog,
	int nParam1,
	int nParam2,
	DWORD dwFlags )
{
	ASSERT_RETURN( pQuest );
	QUEST_GAME_DATA * pGameData = pQuest->pGameData;
	ASSERTX_RETURN( pGameData, "Quest Game Data not initialized" );
	ASSERTX_RETURN( IS_SERVER( pGameData->pGame ), "Server only" );

	// if you are not in the game, don't send you the message
	if ( !pGameData->bInGame )
		return;

	// send message
	MSG_SCMD_UISHOWMESSAGE tMessage;
	tMessage.bType = (BYTE)eUIMessageType;
	tMessage.nDialog = nDialog;
	tMessage.nParam1 = nParam1;
	tMessage.nParam2 = nParam2;
	tMessage.bFlags = (BYTE)( dwFlags & 0xff );
	s_SendMessage( pGameData->pGame, UnitGetClientId( pQuest->pPlayer ), SCMD_UISHOWMESSAGE, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL s_QuestGameInOperateDistance( UNIT * pOperator, UNIT * pObject )
{
	ASSERT_RETFALSE( pOperator );
	ASSERT_RETFALSE( pObject );
	float dist = sqrtf( UnitsGetDistanceSquared( pOperator, pObject ) );
	float u_col_dist = UnitGetCollisionRadius( pOperator );
	float o_col_dist = UnitGetCollisionRadius( pObject );
	dist = dist - u_col_dist - o_col_dist;
	return ( dist < QUEST_OPERATE_OBJECT_DISTANCE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameWarpPlayerIntoArena( QUEST * pQuest, UNIT * player )
{
	ASSERT_RETURN( player  );
	GAME * pGame = UnitGetGame( player );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	LEVEL * level = UnitGetLevel( player );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT * startloc = LevelFindFirstUnitOf( level, eTargetTypes, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_QUEST_GAME_START_LOCATION ) );
	ASSERT_RETURN( startloc );

	VECTOR vUp( 0.0f, 0.0f, 1.0f );
	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp( player, UnitGetRoom( startloc ), startloc->vPosition, startloc->vFaceDirection, vUp, dwWarpFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameWarpUnitToNode( UNIT * unit, char * label )
{
	ASSERT_RETURN( unit  );
	GAME * pGame = UnitGetGame( unit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	LEVEL * level = UnitGetLevel( unit );
	for ( ROOM * room = LevelGetFirstRoom( level ); room; room = LevelGetNextRoom( room ) )
	{
		// find the node
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, label, &pOrientation );
		if ( nodeset && pOrientation )
		{
			VECTOR vPosition, vDirection;
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );
			DWORD dwWarpFlags = 0;
			SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
			UnitWarp( unit, room, vPosition, vDirection, vUp, dwWarpFlags );
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL s_QuestGameRespawnMonster( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	// i need to be dead...
	if ( !IsUnitDeadOrDying( unit ) )
		return TRUE;

	// clear my death
	UnitClearFlag(unit, UNITFLAG_DEAD_OR_DYING);
	UnitChangeTargetType(unit, TARGET_BAD);
	UnitSetFlag(unit, UNITFLAG_COLLIDABLE);
	s_StateClearBadStates(unit);

	// reset my modes
	UnitEndMode( unit, MODE_DYING, 0, TRUE );
	UnitEndMode( unit, MODE_DEAD, 0, TRUE );
	s_UnitSetMode( unit, MODE_IDLE );

	// reset my stats
	s_UnitRestoreVitals( unit, FALSE );

	// warp me to starting loc
	s_QuestGameWarpUnitToNode( unit, "MonsterSpawn" );

	return TRUE;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL s_QuestGameFindLayoutLabel( LEVEL * pLevel, char * pszNodeName, QUEST_GAME_LABEL_NODE_INFO * pInfo )
{
	ASSERT_RETFALSE( pLevel );
	ASSERT_RETFALSE( pszNodeName );
	ASSERT_RETFALSE( pInfo );

	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// find the node
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, pszNodeName, &pOrientation );
		if ( nodeset && pOrientation )
		{
			VECTOR vPosition, vDirection;
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			s_QuestNodeGetPosAndDir( room, pOrientation, &pInfo->vPosition, &pInfo->vDirection, NULL, NULL );
			pInfo->pRoom = room;
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNITID s_QuestGameSpawnEnemy( QUEST * pQuest, char * pszLabelName, int nClass, int nLevel )
{
	ASSERT_RETVAL( pQuest, INVALID_ID );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), INVALID_ID, "Server only" );

	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	QUEST_GAME_LABEL_NODE_INFO tInfo;
	BOOL bRetVal = s_QuestGameFindLayoutLabel( pLevel, pszLabelName, &tInfo );
	ASSERT_RETVAL( bRetVal, INVALID_ID );

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, tInfo.pRoom, tInfo.vPosition, &pDestRoom );
	ASSERT_RETVAL( pPathNode, INVALID_ID );

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = nClass;
	tSpec.nExperienceLevel = nLevel;
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = tInfo.vDirection;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * enemy = s_MonsterSpawn( pGame, tSpec );
	ASSERT( enemy );
	return UnitGetId( enemy );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestGameNearbyInfo( UNIT * monster, QUEST_GAME_NEARBY_INFO * pNearbyInfo, BOOL bIncludeEnemies )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( monster ) ), "Server only" );

	pNearbyInfo->nNumNearbyObjects = 0;
	pNearbyInfo->nNumNearbyPlayers = 0;
	pNearbyInfo->nNumNearbyEnemies = 0;
	ROOM * my_room = UnitGetRoom(monster);
	int nearbyRoomCount = RoomGetNearbyRoomCount(my_room);
	for (int i = -1; i < nearbyRoomCount; i++)
	{
		ROOM * test_room;
		if ( i == -1 )
			test_room = my_room;
		else
			test_room = RoomGetNearbyRoom(my_room, i);
		ASSERTX_CONTINUE( test_room, "Invalid nearby room list" );
		TARGET_TYPE eTypeList[] = { TARGET_GOOD, TARGET_OBJECT };
		for ( int types = 0; types < 2; types++ )
		{
			TARGET_TYPE eTestType = eTypeList[types];
			for ( UNIT * test = test_room->tUnitList.ppFirst[eTestType]; test; test = test->tRoomNode.pNext )
			{
				if ( UnitIsA( test, UNITTYPE_PLAYER ) )
				{
					if ( pNearbyInfo->nNumNearbyPlayers < QUEST_GAME_MAX_NEARBY_UNITS )
					{
						pNearbyInfo->pNearbyPlayers[pNearbyInfo->nNumNearbyPlayers] = test;
						pNearbyInfo->nNumNearbyPlayers++;
					}
				}
				if ( UnitIsA( test, UNITTYPE_OBJECT ) )
				{
					if ( pNearbyInfo->nNumNearbyObjects < QUEST_GAME_MAX_NEARBY_UNITS )
					{
						pNearbyInfo->pNearbyObjects[pNearbyInfo->nNumNearbyObjects] = test;
						pNearbyInfo->nNumNearbyObjects++;
					}
				}
				if ( bIncludeEnemies && UnitIsA( test, UNITTYPE_MONSTER ) )
				{
					if ( pNearbyInfo->nNumNearbyEnemies < QUEST_GAME_MAX_NEARBY_UNITS )
					{
						pNearbyInfo->pNearbyEnemies[pNearbyInfo->nNumNearbyEnemies] = test;
						pNearbyInfo->nNumNearbyEnemies++;
					}
				}
			}
		}
	}
}
