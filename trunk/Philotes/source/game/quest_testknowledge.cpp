//----------------------------------------------------------------------------
// FILE: quest_testknowledge.cpp
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
#include "quest_testknowledge.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "room_layout.h"
#include "states.h"
#include "s_questgames.h"
#include "room_path.h"
#include "unitmodes.h"
#include "movement.h"
#include "ai.h"
#include "colors.h"
#include "quest_testleadership.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define NUM_TESTKNOWLEDGE_ENEMIES			4
#define TOK_ENEMY_LEVEL						28

//----------------------------------------------------------------------------
struct QUEST_DATA_TESTKNOWLEDGE
{
	int		nTruthNPC;

	int		nLevelTestOfKnowledge;

	int		nTower;
	int		nBook;
	int		nMonsterGoal;
	int		nPlayerGoal;
	int		nSigil;

	int		nStartLocation;

	int		nImpUber;

	UNITID	idImps[ NUM_TESTKNOWLEDGE_ENEMIES ];
	UNITID	idBook;

	int		nInfoTowerBuffCount;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TESTKNOWLEDGE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TESTKNOWLEDGE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnBookInCenter( QUEST * pQuest )
{
	UNIT * player = pQuest->pPlayer;
	LEVEL * level = UnitGetLevel( player );
	for ( ROOM * room = LevelGetFirstRoom( level ); room; room = LevelGetNextRoom( room ) )
	{
		// find the node
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "BookSpawn", &pOrientation );
		if ( nodeset && pOrientation )
		{
			VECTOR vPosition, vDirection;
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );

			QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
			OBJECT_SPEC tSpec;
			tSpec.nClass = pQuestData->nBook;
			tSpec.pRoom = room;
			tSpec.vPosition = vPosition;
			tSpec.pvFaceDirection = &vDirection;
			UNIT * book = s_ObjectSpawn( UnitGetGame( player ), tSpec );
			if ( book )
			{
				pQuestData->idBook = UnitGetId( book );
			}
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sRespawnBookAtUnit( QUEST * pQuest, UNIT * pUnit )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pUnit ) ), "Server only" );

	// clear book state
	s_StateClear( pUnit, UnitGetId( pUnit ), STATE_QUEST_TK_BOOK, 0 );

	// drop new book
	ROOM * room = UnitGetRoom( pUnit );

	VECTOR vPosition, vDirection;
	VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );

	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
	OBJECT_SPEC tSpec;
	tSpec.nClass = pQuestData->nBook;
	tSpec.pRoom = room;
	tSpec.vPosition = pUnit->vPosition;
	tSpec.pvFaceDirection = &pUnit->vFaceDirection;
	UNIT * book = s_ObjectSpawn( UnitGetGame( pUnit ), tSpec );
	ASSERTX_RETURN( book, "Had trouble dropping the book" );
	// send id to everyone
	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOK_SET_BOOK, book );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnEnemies( QUEST * pQuest )
{
	UNIT * player = pQuest->pPlayer;
	LEVEL * level = UnitGetLevel( player );
	for ( ROOM * room = LevelGetFirstRoom( level ); room; room = LevelGetNextRoom( room ) )
	{
		// find the node
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "MonsterSpawn", &pOrientation );
		if ( nodeset && pOrientation )
		{
			VECTOR vPosition, vDirection;
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );

			QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

			for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
			{
				GAME * pGame = UnitGetGame( player );
				ROOM * pDestRoom;
				ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, room, vPosition, &pDestRoom );

				if ( !pNode )
					return;

				// init location
				SPAWN_LOCATION tLocation;
				SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

				MONSTER_SPEC tSpec;
				tSpec.nClass = pQuestData->nImpUber;
				tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pDestRoom ), tSpec.nClass, TOK_ENEMY_LEVEL );
				tSpec.pRoom = pDestRoom;
				tSpec.vPosition = tLocation.vSpawnPosition;
				tSpec.vDirection = vDirection;
				tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
				UNIT * imp = s_MonsterSpawn( pGame, tSpec );
				ASSERT( imp );
				if ( imp )
				{
					pQuestData->idImps[i] = UnitGetId( imp );
					UnitSetStat( imp, STATS_AI_ACTIVE_QUEST_ID, pQuest->nQuest );
				}
			}
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCopyGameData( QUEST * pQuest, UNIT * pFromPlayer )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server only" );
	ASSERT_RETURN( pFromPlayer );

	QUEST * pFromQuest = QuestGetQuest( pFromPlayer, pQuest->nQuest );
	if ( !pFromQuest )
		return;

	QUEST_DATA_TESTKNOWLEDGE * pToQuestData = sQuestDataGet( pQuest );
	QUEST_DATA_TESTKNOWLEDGE * pFromQuestData = sQuestDataGet( pFromQuest );

	for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
	{
		pToQuestData->idImps[i] = pFromQuestData->idImps[i];
	}

	pToQuestData->idBook = pFromQuestData->idBook;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sShutdownGame( QUEST * pQuest )
{
	// delete all the enemies and book
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

	for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
	{
		UNIT * imp = UnitGetById( pGame, pQuestData->idImps[i] );
		if ( imp )
		{
			UnitFree( imp, UFF_SEND_TO_CLIENTS );
		}
		pQuestData->idImps[i] = INVALID_ID;
	}

	UNIT * book = UnitGetById( pGame, pQuestData->idBook );
	if ( book )
	{
		UnitFree( book, UFF_SEND_TO_CLIENTS );
	}
	pQuestData->idBook = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTestWin( QUEST * pQuest, UNIT * pPlayer )
{
	if ( UnitGetGenus( pPlayer ) != GENUS_PLAYER )
		return;
	GAME * pGame = UnitGetGame( pPlayer );
	if ( UnitHasState( pGame, pPlayer, STATE_QUEST_TK_BOOK ) )
	{
		// point!
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_TK_BOOK, 0 );
		s_QuestGameVictory( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sLoseGame( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	s_QuestGameFailed( pQuest );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sGrabBook( QUEST * pQuest, UNITID idUnit )
{
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

	// someone might have grabbed it already...
	if ( pQuestData->idBook == INVALID_ID )
		return;

	UNIT * pUnit = UnitGetById( UnitGetGame( pQuest->pPlayer ), idUnit );

	if ( IsUnitDeadOrDying( pUnit ) )
		return;

	UNIT * book = UnitGetById( UnitGetGame( pUnit ), pQuestData->idBook );
	if ( book )
	{
		UnitFree( book, UFF_SEND_TO_CLIENTS );
		s_StateSet( pUnit, pUnit, STATE_QUEST_TK_BOOK, 0 );
		s_StateSet( pUnit, pUnit, STATE_QUEST_ACCEPT, 0 );
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TOK_CLEAR_BOOK, pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define QUEST_MAX_NEARBY_UNITS			16

static void sAIUpdate( QUEST * pQuest, UNIT * monster )
{
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( monster );
	ASSERTX_RETURN( UnitGetClass( monster ) == pQuestData->nImpUber, "Test of Knowledge Strategy is Imp_uber only" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	if ( IsUnitDeadOrDying( monster ) )
		return;

	// clear previous flags
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_ATTACK, 0 );

	// find objects and players around me
	QUEST_GAME_NEARBY_INFO	tNearbyInfo;
	s_QuestGameNearbyInfo( monster, &tNearbyInfo );

	// am i near:
	// 1) the book to pick it up?
	// 2) the goal?
	if ( tNearbyInfo.nNumNearbyObjects > 0 )
	{
		for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
		{
			if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nBook )
			{
				// find distance
				if ( s_QuestGameInOperateDistance( monster, tNearbyInfo.pNearbyObjects[i] ) )
				{
					sGrabBook( pQuest, UnitGetId( monster ) );
					s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );
					return;
				}
			}
			if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nMonsterGoal )
			{
				if ( UnitHasState( pGame, monster, STATE_QUEST_TK_BOOK ) && ( s_QuestGameInOperateDistance( monster, tNearbyInfo.pNearbyObjects[i] ) ) )
				{
					// register an event to lose and destroy the game out of the AI loop... otherwise it frees the unit. Ai doesn't likey...
					UnitRegisterEventTimed( tNearbyInfo.pNearbyObjects[i], sLoseGame, &EVENT_DATA( pQuest->nQuest ), 1 );
					return;
				}
			}
		}
	}
	else
	{
		// stop tracking if nothing nearby
		s_StateClear( monster, UnitGetId( monster ), STATE_UBER_IMP_RUN_TO, 0 );
	}

	// do I have the book and the goal is nearby?
	if ( UnitHasState( pGame, monster, STATE_QUEST_TK_BOOK ) && ( tNearbyInfo.nNumNearbyObjects > 0 ) )
	{
		for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
		{
			if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nMonsterGoal )
			{
				// path to the goal!
				UnitSetStat( monster, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( tNearbyInfo.pNearbyObjects[i] ) );
				s_StateSet( monster, monster, STATE_UBER_IMP_RUN_TO, 0 );
				return;
			}
		}
	}

	// any players nearby with book?
	for ( int i = 0; i < tNearbyInfo.nNumNearbyPlayers; i++ )
	{
		if ( UnitHasState( pGame, tNearbyInfo.pNearbyPlayers[i], STATE_QUEST_TK_BOOK ) )
		{
			// attack player with book!
			UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[i] ) );
			s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
			return;
		}
	}

	// attack nearby players
	if ( tNearbyInfo.nNumNearbyPlayers > 0 )
	{
		// attack first player
		UnitSetStat( monster, STATS_AI_TARGET_ID, UnitGetId( tNearbyInfo.pNearbyPlayers[0] ) );
		s_StateSet( monster, monster, STATE_UBER_IMP_ATTACK, 0 );
		return;
	}

	// book nearby?
	for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
	{
		if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nBook )
		{
			// path to the book!
			UnitSetStat( monster, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( tNearbyInfo.pNearbyObjects[i] ) );
			s_StateSet( monster, monster, STATE_UBER_IMP_RUN_TO, 0 );
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageGameMessage( 
	QUEST *pQuest,
	const QUEST_MESSAGE_GAME_MESSAGE *pMessage)
{
	QUEST_GAME_MESSAGE_TYPE eCommand = (QUEST_GAME_MESSAGE_TYPE)pMessage->nCommand;

	switch ( eCommand )
	{
		//----------------------------------------------------------------------------
		case QGM_GAME_NEW:
		{
			// first one in quest-game, so start it up!
			sSpawnBookInCenter( pQuest );
			sSpawnEnemies( pQuest );
			UNIT *pPlayer = QuestGetPlayer( pQuest );
			s_StateSet( pPlayer, pPlayer, STATE_NO_MAJOR_DEATH_PENALTIES, 0 );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_COPY_CURRENT_STATE:
		{
			// copy scores, data, etc for current state of the game
			sCopyGameData( pQuest, pMessage->pUnit );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_DESTROY:
		{
			// everyone has left, so delete all the game-specific data
			sShutdownGame( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_JOIN:
		{
			// this is a respawn in the arena game
			s_StateSet( pQuest->pPlayer, pQuest->pPlayer, STATE_QUEST_GAME_ARENA_RESPAWN, 0 );
			// send me into the game
			s_QuestGameWarpPlayerIntoArena( pQuest, pQuest->pPlayer );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_VICTORY:
		{
			if ( QuestIsActive( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_TESTKNOWLEDGE_PASS, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TESTKNOWLEDGE_SIGIL, QUEST_STATE_ACTIVE );
			}
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOK_VICTORY );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "VictorySpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_FAILED:
		{
			s_QuestGameRespawnDeadPlayer( pQuest->pPlayer );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TOK_FAIL );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "FailSpawn" );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_AIUPDATE:
		{
			sAIUpdate( pQuest, pMessage->pUnit );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_ARENA_RESPAWN:
		{
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		// custom Test of Knowledge only messages
		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOK_CLEAR_BOOK:
		{
			// display pick-up message
			int nDialog;
			if ( UnitIsA( pMessage->pUnit, UNITTYPE_PLAYER ) )
				nDialog = DIALOG_TOK_TEAM_BOOK;
			else
				nDialog = DIALOG_TOK_DEMON_BOOK;
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, nDialog );
			// sync my data
			QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
			ASSERT( pQuestData->idBook != INVALID_ID );
			pQuestData->idBook = INVALID_ID;
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TOK_SET_BOOK:
		{
			// drop the book message?
			// sync my data ;)
			QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
			ASSERT( pQuestData->idBook == INVALID_ID );
			pQuestData->idBook = UnitGetId( pMessage->pUnit );
			break;
		}
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
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

	if (UnitIsMonsterClass( pSubject, pQuestData->nTruthNPC ))
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelTestOfKnowledge )
			return QMR_OK;

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOK_RULES );		
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
	//UNIT *pTalkingTo = pMessage->pTalkingTo;
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TOK_RULES:
		{
			s_QuestToKBegin( pQuest );
			s_QuestGameJoin( pQuest );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sInfoTowerUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

	pQuestData->nInfoTowerBuffCount--;
	if ( pQuestData->nInfoTowerBuffCount <= 0 )
	{
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
		return TRUE;
	}

	for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
	{
		if ( pQuestData->idImps[i] != INVALID_ID )
		{
			UNIT * imp = UnitGetById( game, pQuestData->idImps[i] );
			if ( imp ) s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_UPDATE_UNIT, imp, 0, GFXCOLOR_RED );
		}
	}

	UnitRegisterEventTimed( unit, sInfoTowerUpdate, &tEventData, GAME_TICKS_PER_SECOND / 4 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInfoTowerActivate( QUEST * pQuest, UNIT * pTower )
{
	ASSERT_RETURN ( pQuest );

	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );

	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
	for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
	{
		if ( pQuestData->idImps[i] != INVALID_ID )
		{
			UNIT * imp = UnitGetById( pGame, pQuestData->idImps[i] );
			if ( imp ) s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, imp, 0, GFXCOLOR_RED );
		}
	}
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_ON );

	pQuestData->nInfoTowerBuffCount = 50;

	// setup event
	UnitRegisterEventTimed( pPlayer, sInfoTowerUpdate, &EVENT_DATA( (DWORD_PTR)pQuest ), GAME_TICKS_PER_SECOND / 4 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nTower )
	{
		sInfoTowerActivate( pQuest, pTarget );
		return QMR_FINISHED;
	}

	if ( nTargetClass == pQuestData->nBook )
	{
		s_QuestOperateDelay( pQuest, sGrabBook, UnitGetId( pQuest->pPlayer ), 3 * GAME_TICKS_PER_SECOND );
		return QMR_FINISHED;
	}

	if ( nTargetClass == pQuestData->nPlayerGoal )
	{
		sTestWin( pQuest, pMessage->pOperator );
		return QMR_FINISHED;
	}

	if ( nTargetClass == pQuestData->nSigil )
	{
		if ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelTestOfKnowledge )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTKNOWLEDGE_SIGIL, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			QUEST * pTest = QuestGetQuest( pQuest->pPlayer, QUEST_TESTLEADERSHIP );
			s_QuestToLBegin( pTest );
			s_QuestGameWarpUnitToNode( pQuest->pPlayer, "FailSpawn" );
			return QMR_FINISHED;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageLeaveLevel(
	QUEST *pQuest,
	const QUEST_MESSAGE_LEAVE_LEVEL * pMessage)
{
	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETVAL( IS_SERVER( pGame ), QMR_OK, "Server only" );

	if ( !QuestGamePlayerInGame( pPlayer ) )
		return QMR_OK;

	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	if ( pLevel->m_nLevelDef == pQuestData->nLevelTestOfKnowledge )
	{
		// drop flag if I leave
		if ( UnitHasState( pGame, pPlayer, STATE_QUEST_TK_BOOK ) )
		{
			sRespawnBookAtUnit( pQuest, pPlayer );
		}
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_NO_MAJOR_DEATH_PENALTIES, 0 );

		s_QuestGameLeave( pQuest );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// if the uber imp died, respawn...
	if ( UnitIsMonsterClass( pVictim, pQuestData->nImpUber ) )
	{
		if ( UnitGetLevelDefinitionIndex( pVictim ) == pQuestData->nLevelTestOfKnowledge )
		{
			if ( !UnitHasTimedEvent( pVictim, s_QuestGameRespawnMonster ) )
			{
				UNITID idVictim = UnitGetId( pVictim );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_DEFEND, 0 );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_ATTACK, 0 );
				s_StateClear( pVictim, idVictim, STATE_UBER_IMP_RUN_TO, 0 );
				if ( UnitHasState( pGame, pVictim, STATE_QUEST_TK_BOOK ) )
				{
					sRespawnBookAtUnit( pQuest, pVictim );
				}
				UnitRegisterEventTimed( pVictim, s_QuestGameRespawnMonster, &EVENT_DATA(), GAME_TICKS_PER_SECOND * 3 );
			}
			return QMR_FINISHED;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerDie( 
	QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest && pQuest->pPlayer, QMR_OK );
	UNIT * pPlayer = pQuest->pPlayer;
	if ( QuestGamePlayerInGame( pPlayer ) )
	{
		GAME * pGame = UnitGetGame( pPlayer );
		QUEST_DATA_TESTKNOWLEDGE * pQuestData = sQuestDataGet( pQuest );
		if ( ( UnitGetLevelDefinitionIndex( pPlayer ) == pQuestData->nLevelTestOfKnowledge ) && ( UnitHasState( pGame, pPlayer, STATE_QUEST_TK_BOOK ) ) )
		{
			// spawn again
			sRespawnBookAtUnit( pQuest, pPlayer );
			return QMR_FINISHED;
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
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pLeaveLevelMessage = (const QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
			return sQuestMessageLeaveLevel( pQuest, pLeaveLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_GAME_MESSAGE:
		{
			const QUEST_MESSAGE_GAME_MESSAGE * pQuestGameMessage = (const QUEST_MESSAGE_GAME_MESSAGE *)pMessage;
			return sQuestMessageGameMessage( pQuest, pQuestGameMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_DIE:
		{
			return sQuestMessagePlayerDie( pQuest );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTestKnowledge(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
	if ( pQuest->pGameData )
	{
		s_QuestGameDataFree( pQuest );
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_TESTKNOWLEDGE * pQuestData)
{
	pQuestData->nTruthNPC				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_A_SAGE_NEW );
	pQuestData->nLevelTestOfKnowledge	= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_KNOWLEDGE );
	pQuestData->nTower					= QuestUseGI( pQuest, GI_OBJECT_TESTKNOWLEDGE_INFO_TOWER );
	pQuestData->nBook					= QuestUseGI( pQuest, GI_OBJECT_TESTKNOWLEDGE_BOOK );
	pQuestData->nMonsterGoal			= QuestUseGI( pQuest, GI_OBJECT_TESTKNOWLEDGE_MONSTER_GOAL );
	pQuestData->nPlayerGoal				= QuestUseGI( pQuest, GI_OBJECT_TESTKNOWLEDGE_PLAYER_GOAL );
	pQuestData->nSigil					= QuestUseGI( pQuest, GI_OBJECT_SIGIL );
	pQuestData->nStartLocation			= QuestUseGI( pQuest, GI_OBJECT_QUEST_GAME_START_LOCATION );
	pQuestData->nImpUber				= QuestUseGI( pQuest, GI_MONSTER_IMP_UBER );

	for ( int i = 0; i < NUM_TESTKNOWLEDGE_ENEMIES; i++ )
	{
		pQuestData->idImps[i] = INVALID_ID;
	}
	pQuestData->idBook = INVALID_ID;

	pQuestData->nInfoTowerBuffCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestKnowledge(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TESTKNOWLEDGE * pQuestData = ( QUEST_DATA_TESTKNOWLEDGE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TESTKNOWLEDGE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
	if ( IS_SERVER( pGame ) )
	{
		s_QuestGameDataInit( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreX(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestToKBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTKNOWLEDGE, "Wrong quest sent to function. Need Test of Knowledge" );

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_ACTIVE )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
}
