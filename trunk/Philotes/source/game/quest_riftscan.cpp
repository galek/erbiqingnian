//----------------------------------------------------------------------------
// FILE: quest_riftscan.cpp
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
#include "quest_riftscan.h"
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
#include "room_path.h"
#include "states.h"
#include "colors.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define NUM_RIFTS_TO_SPAWN				5

//----------------------------------------------------------------------------

struct QUEST_DATA_RIFTSCAN
{
	int		nMaxim;

	int		nHellriftBall;
	int		nHellriftPriest;

	int		nLevelLudgateHill;

	int		nRiftScanner;
	int		nRiftRevealer;

	UNITID	idRifts[ NUM_RIFTS_TO_SPAWN ];
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_RIFTSCAN * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_RIFTSCAN *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasScanner(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	if ( s_QuestActivate( pQuest ) )
	{
		s_QuestMapReveal( pQuest, GI_LEVEL_BELL_YARD );
		s_QuestMapReveal( pQuest, GI_LEVEL_FLEET_STREET );
		s_QuestMapReveal( pQuest, GI_LEVEL_LUDGATE_HILL );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_RIFT_LOCATIONS				32

static void sCreateRifts(
	QUEST * pQuest )
{
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );

	VECTOR vPosition[ MAX_RIFT_LOCATIONS ];
	VECTOR vDirection[ MAX_RIFT_LOCATIONS ];
	ROOM * pRoomList[ MAX_RIFT_LOCATIONS ];
	int nNumLocations = 0;

	int nNumToSpawn = NUM_RIFTS_TO_SPAWN;
	for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
		pQuestData->idRifts[i] = INVALID_ID;

	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// check to see if spawned
		BOOL bAddRoom = TRUE;
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitIsMonsterClass( test, pQuestData->nHellriftBall ) )
			{
				// fill in data
				UNITID idTest = UnitGetId( test );
				BOOL bFound = FALSE;
				for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
				{
					if ( idTest == pQuestData->idRifts[i] )
					{
						// found one
						bAddRoom = FALSE;
						bFound = TRUE;
						nNumToSpawn--;
					}
				}
				// add
				if ( !bFound )
				{
					int index = 0;
					while ( ( index < NUM_RIFTS_TO_SPAWN ) && ( pQuestData->idRifts[index] != INVALID_ID ) )
						index++;
					if ( index < NUM_RIFTS_TO_SPAWN )
					{
						pQuestData->idRifts[index] = idTest;
						s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, test, 0, GFXCOLOR_WHITE );
						bAddRoom = FALSE;
						nNumToSpawn--;
					}
				}
			}
		}

		// save the nodes
		if ( ( nNumLocations < MAX_RIFT_LOCATIONS ) && bAddRoom )
		{
			ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
			ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "riftscan_location", &pOrientation );
			if ( nodeset && pOrientation )
			{
				s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition[ nNumLocations ], &vDirection[ nNumLocations ], NULL, NULL );
				pRoomList[ nNumLocations ] = room;
				nNumLocations++;
			}
		}
	}

	ASSERT_RETURN( nNumLocations >= nNumToSpawn );

	GAME * pGame = UnitGetGame( pPlayer );
	for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
	{
		// don't respawn all
		if ( pQuestData->idRifts[i] != INVALID_ID )
			continue;

		// choose a location
		int index = RandGetNum( pGame->rand, nNumLocations );

		// and spawn the crate objects
		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nHellriftBall;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 21 );
		ROOM * pSpawnRoom = pRoomList[index];
		tSpec.pRoom = pSpawnRoom;
		tSpec.vPosition = vPosition[index];
		tSpec.vDirection = vDirection[index];
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		UNIT * pRift = s_MonsterSpawn( pGame, tSpec );
		ASSERT_RETURN( pRift );
		s_StateSet( pRift, pRift, STATE_QUEST_HIDDEN, 0 );
		s_StateSet( pRift, pRift, STATE_DONT_RUN_AI, 0 );
		s_StateSet( pRift, pRift, STATE_NPC_GOD, 0 );
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_ADD_UNIT, pRift, 0, GFXCOLOR_WHITE );
		pQuestData->idRifts[i] = UnitGetId( pRift );

		// remove/replace in array
		nNumToSpawn--;
		nNumLocations--;
		pRoomList[index] = pRoomList[nNumLocations];
		vPosition[index] = vPosition[nNumLocations];
		vDirection[index] = vDirection[nNumLocations];

		// remove all in array from same room
		for ( int l = 0; l < nNumLocations; l++ )
		{
			if ( pRoomList[l] == pSpawnRoom )
			{
				nNumLocations--;
				pRoomList[l] = pRoomList[nNumLocations];
				vPosition[l] = vPosition[nNumLocations];
				vDirection[l] = vDirection[nNumLocations];
				l--;
				// still enough?
				ASSERT_RETURN( nNumLocations >= nNumToSpawn );
			}
		}
	}
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
	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nMaxim ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_RIFTSCAN_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nMaxim ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_RIFTSCAN_RETURN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_RIFTSCAN_COMPLETE );
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
	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );

	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_RIFTSCAN_INTRO:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasScanner;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTRIFTSCANOFFER, tActions );

			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_RIFTSCAN_COMPLETE:
		{
			// complete the quest
			QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_RETURN, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nRiftScanner );
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nRiftRevealer );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelLudgateHill )
	{
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_RESET );
		QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_USESCANNER, QUEST_STATE_ACTIVE );
		sCreateRifts( pQuest );
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

	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	// turn off radar
	if ( pLevel->m_nLevelDef == pQuestData->nLevelLudgateHill )
	{
		s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
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
		QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( ( item_class == pQuestData->nRiftScanner ) || ( item_class == pQuestData->nRiftRevealer ) )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
				if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelLudgateHill )
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
static void sSpawnHellriftPriest(
	QUEST * pQuest,
	UNIT * pRift )
{
	ASSERT_RETURN( pQuest );

	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );

	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	ROOM * pRoom = UnitGetRoom( pRift );

	VECTOR vPriestPosition;
	VECTOR vPriestDirection;
	ROOM * pPriestRoom;

	// get hellrift ball location
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pRiftNode = RoomGetLabelNode( pRoom, "riftscan_location", &pOrientation );
	ASSERT_RETURN( pRiftNode );
	ASSERT_RETURN( pOrientation );

	// get hellrift priest location
	ROOM_LAYOUT_GROUP * pPriestNode = pRiftNode->pGroups;
	ASSERT_RETURN( pPriestNode );
	ASSERT_RETURN( pPriestNode->pGroups );
	ROOM_LAYOUT_SELECTION_ORIENTATION * pPriestOrientation = RoomLayoutGetOrientationFromGroup(pRoom, pPriestNode->pGroups);
	ASSERT_RETURN(pPriestOrientation);
	s_QuestNodeGetPosAndDir( pRoom, pPriestOrientation, &vPriestPosition, &vPriestDirection, NULL, NULL );

	ROOM_PATH_NODE * pPriestPathNode = RoomGetNearestPathNode( pGame, pRoom, vPriestPosition, &pPriestRoom );
	ASSERT_RETURN( pPriestPathNode );

	// spawn hellrift priest
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pPriestRoom, pPriestPathNode->nIndex );

	MONSTER_SPEC tPriestSpec;
	tPriestSpec.nClass = pQuestData->nHellriftPriest;
	tPriestSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pPriestRoom ), tPriestSpec.nClass, 21 );
	tPriestSpec.pRoom = pPriestRoom;
	tPriestSpec.vPosition = tLocation.vSpawnPosition;
	tPriestSpec.vDirection = vPriestDirection;
	tPriestSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * priest = s_MonsterSpawn( pGame, tPriestSpec );
	ASSERT_RETURN( priest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define RIFT_REVEAL_RADIUS				10.0f
#define RIFT_REVEAL_RADIUS_SQUARED		( RIFT_REVEAL_RADIUS * RIFT_REVEAL_RADIUS )

static void sUseRevealer( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
	ASSERT_RETURN( pQuestData );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );

	for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
	{
		// has this one not spawned yet?
		if ( pQuestData->idRifts[i] != INVALID_ID )
		{
			UNIT * pRift = UnitGetById( pGame, pQuestData->idRifts[i] );
			if ( pRift )
			{
				if ( UnitHasState( pGame, pRift, STATE_QUEST_HIDDEN ) )
				{
					float fRadiusSquared = RIFT_REVEAL_RADIUS_SQUARED;
					float fDistanceSquared = VectorDistanceSquaredXY( pPlayer->vPosition, pRift->vPosition );
					if ( fDistanceSquared < fRadiusSquared )
					{
						s_StateClear( pRift, pQuestData->idRifts[i], STATE_QUEST_HIDDEN, 0 );
						s_StateClear( pRift, pQuestData->idRifts[i], STATE_DONT_RUN_AI, 0 );
						s_StateClear( pRift, pQuestData->idRifts[i], STATE_NPC_GOD, 0 );
						sSpawnHellriftPriest( pQuest, pRift );
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pLevel && pItem)
	{
		QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nRiftScanner )
		{
			if ( pLevel->m_nLevelDef == pQuestData->nLevelLudgateHill )
			{
				s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TOGGLE );
				QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_USESCANNER, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_DESTROY, QUEST_STATE_ACTIVE );
				return QMR_USE_OK;
			}
			return QMR_USE_FAIL;
		}
		if ( item_class == pQuestData->nRiftRevealer )
		{
			if ( pLevel->m_nLevelDef == pQuestData->nLevelLudgateHill )
			{
				// try and operate a node
				sUseRevealer( pQuest );
				return QMR_USE_OK;
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHellriftDestroyed( QUEST * pQuest, UNIT * pTarget, int index )
{
	ASSERT_RETURN( pQuest );
	ASSERT_RETURN( pTarget );
	ASSERT_RETURN( index >= 0 );
	ASSERT_RETURN( index < NUM_RIFTS_TO_SPAWN );

	// remove radar message and mark finished
	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_REMOVE_UNIT, pTarget );
	pQuestData->idRifts[index] = INVALID_ID;

	// check if completed
	BOOL bDone = TRUE;
	for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
		bDone = bDone && ( pQuestData->idRifts[i] == INVALID_ID );

	if ( bDone )
	{
		QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_DESTROY, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_RIFTSCAN_RETURN, QUEST_STATE_ACTIVE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_RIFTSCAN * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// if a hellrift ball died, mark it
	if ( UnitIsMonsterClass( pVictim, pQuestData->nHellriftBall ))
	{
		UNITID idRift = UnitGetId( pVictim );
		for ( int i = 0; i < NUM_RIFTS_TO_SPAWN; i++ )
		{
			if ( pQuestData->idRifts[i] == idRift )
				sHellriftDestroyed( pQuest, pVictim, i );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageAbandon(
	QUEST * pQuest )
{
	s_QuestSendRadarMessage( pQuest, QUEST_UI_RADAR_TURN_OFF );
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
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pLeaveLevelMessage = (const QUEST_MESSAGE_LEAVE_LEVEL *)pMessage;
			return sQuestMessageLeaveLevel( pQuest, pLeaveLevelMessage );
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
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
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
void QuestFreeRiftScan(
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
	QUEST_DATA_RIFTSCAN * pQuestData)
{
	pQuestData->nMaxim						= QuestUseGI( pQuest, GI_MONSTER_LORD_MAXIM );
	pQuestData->nRiftScanner				= QuestUseGI( pQuest, GI_ITEM_RIFT_SCANNER );
	pQuestData->nRiftRevealer				= QuestUseGI( pQuest, GI_ITEM_RIFT_REVEALER );
	pQuestData->nHellriftBall				= QuestUseGI( pQuest, GI_MONSTER_HELLRIFT_BALL );
	pQuestData->nHellriftPriest				= QuestUseGI( pQuest, GI_MONSTER_HELLRIFT_PRIEST );
	pQuestData->nLevelLudgateHill			= QuestUseGI( pQuest, GI_LEVEL_LUDGATE_HILL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitRiftScan(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_RIFTSCAN * pQuestData = ( QUEST_DATA_RIFTSCAN * )GMALLOCZ( pGame, sizeof( QUEST_DATA_RIFTSCAN ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
}
