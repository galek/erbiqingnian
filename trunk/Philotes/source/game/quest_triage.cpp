//----------------------------------------------------------------------------
// FILE: quest_triage.cpp
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
#include "quest_triage.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "s_questgames.h"
#include "room_path.h"
#include "room_layout.h"
#include "objects.h"
#include "uix.h"
#include "ai.h"
#include "states.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

#define MAX_TRIAGE_ENEMIES			4
#define MAX_TRIAGE_INJURED			6

#define QUEST_TRIAGE_NEWSPAWN_INJURED_SECONDS			30
#define QUEST_TRIAGE_NEWSPAWN_ENEMY_SECONDS				180

#define QUEST_TRIAGE_NUM_INJURED_SAVE_VICTORY		10
#define QUEST_TRIAGE_NUM_INJURED_DEAD_LOSE			5

#define QUEST_TRIAGE_MIN_GRAVELY_DEATH_TICKS		( 30 * GAME_TICKS_PER_SECOND )
#define QUEST_TRIAGE_MAX_GRAVELY_DEATH_TICKS		( 40 * GAME_TICKS_PER_SECOND )

#define QUEST_TRIAGE_MIN_SEVERELY_DEATH_TICKS		( 120 * GAME_TICKS_PER_SECOND )
#define QUEST_TRIAGE_MAX_SEVERELY_DEATH_TICKS		( 150 * GAME_TICKS_PER_SECOND )

#define QUEST_TRIAGE_USE_DISTANCE					2.0f

//----------------------------------------------------------------------------

struct QUEST_GAME_SPAWN_NODE
{
	ROOM *		pRoom;
	VECTOR		vPosition;
	VECTOR		vDirection;
};

struct QUEST_DATA_TRIAGE
{
	int		nEmmera;
	int		nLordArphaun;

	int		nLevelTemplarBase;
	int		nLevelCrownOfficeRow;

	int		nMedkit;

	int		nHellball;
	int		nGravelyInjured;
	int		nSeverelyInjured;

	int		nEmmeraAI;
	//----------------------------------------------------------------------------
	// shared quest-game data
	BOOL					bSuccess;
	int						nNumSaved;
	int						nNumDead;

	QUEST_GAME_SPAWN_NODE	tHellballNodes[ MAX_TRIAGE_ENEMIES ];
	UNITID					idHellball[ MAX_TRIAGE_ENEMIES ];
	int						nNumHellballs;

	QUEST_GAME_SPAWN_NODE	tInjuredNodes[ MAX_TRIAGE_INJURED ];
	UNITID					idInjured[ MAX_TRIAGE_INJURED ];
	int						nNumInjured;

	UNITID					idEmmera;
	int						nInjuredSeconds;
	int						nHellballSeconds;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TRIAGE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRIAGE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sEmmeraSuperNova( QUEST * pQuest )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	UNIT * pEmmera = UnitGetById( pGame, pQuestData->idEmmera );
	ASSERT_RETURN( pEmmera );

	// change the AI on Aeron Altair and his troops
	if ( UnitGetStat( pEmmera, STATS_AI ) != pQuestData->nEmmeraAI )
	{
		AI_Free( pGame, pEmmera );
		UnitSetStat( pEmmera, STATS_AI, pQuestData->nEmmeraAI );
		AI_Init( pGame, pEmmera );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sMedkitUse( QUEST * pQuest, UNITID idUnit )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	// injured may have died...
	int index = -1;
	for ( int i = 0; ( i < MAX_TRIAGE_INJURED )&& ( index == -1 ); i++ )
	{
		if ( pQuestData->idInjured[i] == idUnit )
			index = i;
	}
	if ( index == -1 )
		return;

	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	UNIT * pInjured = UnitGetById( pGame, idUnit );

	// heal!!
	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TRIAGE_INJURED_SAVED, pInjured, (DWORD)index );

	if ( pQuestData->nNumSaved >= QUEST_TRIAGE_NUM_INJURED_SAVE_VICTORY )
	{
		// emmera does her thing here!!
		sEmmeraSuperNova( pQuest );
		s_QuestGameVictory( pQuest ); // cmarch: this will clear the quest data, and must be done after sEmmeraSuperNova
	}

	UnitFree( pInjured, UFF_SEND_TO_CLIENTS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestTriageBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TRIAGE, "Wrong quest sent to function. Need Triage" );

	if ( QuestIsComplete( pQuest ) )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	UNIT * player = pQuest->pPlayer;
	s_NPCVideoStart( UnitGetGame( player ), player, GI_MONSTER_LORD_ARPHAUN, NPC_VIDEO_INCOMING, DIALOG_TRIAGE_VIDEO_INTRO );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitGameData( QUEST_DATA_TRIAGE * pQuestData )
{
	ASSERT_RETURN( pQuestData );

	pQuestData->bSuccess = FALSE;
	pQuestData->nNumSaved = 0;
	pQuestData->nNumDead = 0;

	for ( int i = 0; i < MAX_TRIAGE_ENEMIES; i++ )
		pQuestData->idHellball[i] = INVALID_ID;
	pQuestData->nNumHellballs = 0;

	for ( int i = 0; i < MAX_TRIAGE_INJURED; i++ )
		pQuestData->idInjured[i] = INVALID_ID;
	pQuestData->nNumInjured = 0;

	pQuestData->idEmmera = INVALID_ID;
	pQuestData->nInjuredSeconds = 0;
	pQuestData->nHellballSeconds = 0;
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

	QUEST_DATA_TRIAGE * pToQuestData = sQuestDataGet( pQuest );
	QUEST_DATA_TRIAGE * pFromQuestData = sQuestDataGet( pFromQuest );

	pToQuestData->bSuccess = pFromQuestData->bSuccess;
	pToQuestData->nNumSaved = pFromQuestData->nNumSaved;
	pToQuestData->nNumDead = pFromQuestData->nNumDead;

	for ( int i = 0; i < MAX_TRIAGE_ENEMIES; i++ )
	{
		pToQuestData->tHellballNodes[i] = pFromQuestData->tHellballNodes[i];
		pToQuestData->idHellball[i] = pFromQuestData->idHellball[i];
	}
	pToQuestData->nNumHellballs = pFromQuestData->nNumHellballs;

	for ( int i = 0; i < MAX_TRIAGE_INJURED; i++ )
	{
		pToQuestData->tInjuredNodes[i] = pFromQuestData->tInjuredNodes[i];
		pToQuestData->idInjured[i] = pFromQuestData->idInjured[i];
	}
	pToQuestData->nNumInjured = pFromQuestData->nNumInjured;

	pToQuestData->idEmmera = pFromQuestData->idEmmera;
	pToQuestData->nInjuredSeconds = pFromQuestData->nInjuredSeconds;
	pToQuestData->nHellballSeconds = pFromQuestData->nHellballSeconds;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sShowScore( QUEST * pQuest )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	s_QuestGameUIMessage( pQuest, QUIM_GLOBAL_STRING_SCORE, GS_TRIAGE_STATUS_MESSAGE, pQuestData->nNumSaved, pQuestData->nNumDead );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnHellball( QUEST * pQuest, BOOL bSend )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );

	// make a list of locations
	int nNumLocations = 0;
	int nLocationList[ MAX_TRIAGE_ENEMIES ];
	for ( int i = 0; i < MAX_TRIAGE_ENEMIES; i++ )
	{
		if ( pQuestData->idHellball[i] == INVALID_ID )
		{
			nLocationList[nNumLocations] = i;
			nNumLocations++;
		}
	}
	ASSERT_RETURN( nNumLocations > 0 );

	// choose a location
	int index = nLocationList[ RandGetNum( pGame->rand, nNumLocations ) ];

	// and spawn the hellball monsters!
	ROOM * pSpawnRoom = NULL;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pQuestData->tHellballNodes[index].pRoom, pQuestData->tHellballNodes[index].vPosition, &pSpawnRoom );
	ASSERT_RETURN( pNode );
	ASSERT_RETURN( pSpawnRoom );

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, pNode->nIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nHellball;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pSpawnRoom ), tSpec.nClass, 25 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = pQuestData->tHellballNodes[index].vDirection;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	UNIT * pHellball = s_MonsterSpawn( pGame, tSpec );
	ASSERT_RETURN( pHellball );

	if ( bSend )
	{
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TRIAGE_NEW_HELLBALL, pHellball, (DWORD)index );
	}
	else
	{
		pQuestData->idHellball[index] = UnitGetId( pHellball );
		pQuestData->nNumHellballs++;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sInjuredDie( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	ASSERTX_RETFALSE( pQuestData, "No Quest Data found" );

	s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TRIAGE_INJURED_DEAD, unit );

	if ( pQuestData->nNumDead >= QUEST_TRIAGE_NUM_INJURED_DEAD_LOSE )
	{
		s_QuestGameFailed( pQuest );
	}

	UnitFree( unit, UFF_SEND_TO_CLIENTS );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnInjured( QUEST * pQuest, BOOL bSend )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );

	// make a list of locations
	int nNumLocations = 0;
	int nLocationList[ MAX_TRIAGE_INJURED ];
	for ( int i = 0; i < MAX_TRIAGE_INJURED; i++ )
	{
		if ( pQuestData->idInjured[i] == INVALID_ID )
		{
			nLocationList[nNumLocations] = i;
			nNumLocations++;
		}
	}
	ASSERT_RETURN( nNumLocations > 0 );

	// choose a location
	int index = nLocationList[ RandGetNum( pGame->rand, nNumLocations ) ];

	// and spawn the injured templar
	OBJECT_SPEC tSpec;

	if ( RandGetNum( pGame->rand, 1000 ) < 500 )
		tSpec.nClass = pQuestData->nGravelyInjured;
	else
		tSpec.nClass = pQuestData->nSeverelyInjured;

	tSpec.pRoom = pQuestData->tInjuredNodes[index].pRoom;
	tSpec.vPosition = pQuestData->tInjuredNodes[index].vPosition;
	tSpec.pvFaceDirection = &pQuestData->tInjuredNodes[index].vDirection;

	UNIT * pInjured = s_ObjectSpawn( pGame, tSpec );
	ASSERT_RETURN( pInjured );

	s_StateSet( pInjured, pInjured, STATE_NPC_TRIAGE, 0 );

	if ( bSend )
	{
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TRIAGE_NEW_INJURED, pInjured, (DWORD)index );
	}
	else
	{
		pQuestData->idInjured[index] = UnitGetId( pInjured );
		pQuestData->nNumInjured++;
	}

	int nDeathTime;
	if ( tSpec.nClass == pQuestData->nGravelyInjured )
		nDeathTime = RandGetNum( pGame->rand, QUEST_TRIAGE_MIN_GRAVELY_DEATH_TICKS, QUEST_TRIAGE_MAX_GRAVELY_DEATH_TICKS );
	else
		nDeathTime = RandGetNum( pGame->rand, QUEST_TRIAGE_MIN_SEVERELY_DEATH_TICKS, QUEST_TRIAGE_MAX_SEVERELY_DEATH_TICKS );

	UnitRegisterEventTimed( pInjured, sInjuredDie, &EVENT_DATA( pQuest->nQuest ), nDeathTime );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sEmmeraTriageControl( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	int nQuestId = (int)tEventData.m_Data1;
	UNIT * pPlayer = s_QuestGameGetFirstPlayer( game, nQuestId );
	ASSERTX_RETFALSE( pPlayer, "No players found in quest game" );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId );
	ASSERTX_RETFALSE( pQuest, "Quests not found for player" );
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	ASSERTX_RETFALSE( pQuestData, "No Quest Data found" );

	// has it been 20 seconds? spawn an injured player
	pQuestData->nInjuredSeconds++;
	if ( pQuestData->nInjuredSeconds >= QUEST_TRIAGE_NEWSPAWN_INJURED_SECONDS )
	{
		if ( pQuestData->nNumInjured < MAX_TRIAGE_INJURED )
			sSpawnInjured( pQuest, TRUE );
		else
			pQuestData->nInjuredSeconds -= 5;
	}

	// has it been 2 mins? spawn a hellball
	pQuestData->nHellballSeconds++;
	if ( pQuestData->nHellballSeconds >= QUEST_TRIAGE_NEWSPAWN_ENEMY_SECONDS )
	{
		if ( pQuestData->nNumHellballs < MAX_TRIAGE_ENEMIES )
			sSpawnHellball( pQuest, TRUE );
		else
			pQuestData->nHellballSeconds -= 5;
	}

	// keep event control going
	UnitRegisterEventTimed( unit, sEmmeraTriageControl, tEventData, GAME_TICKS_PER_SECOND );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSetupEmmera( QUEST * pQuest )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	// first set id
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * pEmmera = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nEmmera );
	ASSERT_RETURN( pEmmera );
	pQuestData->idEmmera = UnitGetId( pEmmera );

	// next set periodic event
	UnitRegisterEventTimed( pEmmera, sEmmeraTriageControl, &EVENT_DATA( pQuest->nQuest ), GAME_TICKS_PER_SECOND );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSetSpawnLocations( QUEST * pQuest )
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	int nHellballCount = 0;
	int nInjuredCount = 0;
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( pRoom, "Hellball Spawn", &pOrientation );
		if ( nodeset && pOrientation )
		{
			ASSERTX_RETURN( nHellballCount < MAX_TRIAGE_ENEMIES, "More Hellball spawn locations on level than expected" );
			pQuestData->tHellballNodes[nHellballCount].pRoom = pRoom;
			s_QuestNodeGetPosAndDir( pRoom, pOrientation, &pQuestData->tHellballNodes[nHellballCount].vPosition, &pQuestData->tHellballNodes[nHellballCount].vDirection, NULL, NULL );
			nHellballCount++;
		}

		nodeset = RoomGetLabelNode( pRoom, "Bed Spawns", &pOrientation );
		if ( nodeset && pOrientation )
		{
			ASSERTX_RETURN( nInjuredCount < MAX_TRIAGE_INJURED, "More bed locations on level than expected" )
				ASSERT_RETURN( nodeset->nGroupCount == MAX_TRIAGE_INJURED );
			for ( int i = 0; i < nodeset->nGroupCount; i++ )
			{
				ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientationNode = NULL;
				ROOM_LAYOUT_GROUP * node = s_QuestNodeSetGetNode( pRoom, nodeset, &pOrientationNode, i+1 );
				ASSERT_RETURN( node );
				ASSERT_RETURN( pOrientationNode );
				pQuestData->tInjuredNodes[i].pRoom = pRoom;
				s_QuestNodeGetPosAndDir( pRoom, pOrientationNode, &pQuestData->tInjuredNodes[i].vPosition, &pQuestData->tInjuredNodes[i].vDirection, NULL, NULL );
				nInjuredCount++;
				ASSERT_RETURN( nInjuredCount <= MAX_TRIAGE_INJURED );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnInitalInjured( QUEST * pQuest )
{
	for ( int i = 0; i < 4; i++ )
	{
		sSpawnInjured( pQuest, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sShutdownGame( QUEST * pQuest )
{
	// delete all the enemies and injured
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	for ( int i = 0; i < MAX_TRIAGE_INJURED; i++ )
	{
		if ( pQuestData->idInjured[i] != INVALID_ID )
		{
			UNIT * injured = UnitGetById( pGame, pQuestData->idInjured[i] );
			if ( injured )
			{
				UnitFree( injured, UFF_SEND_TO_CLIENTS );
			}
			pQuestData->idInjured[i] = INVALID_ID;
		}
	}

	for ( int i = 0; i < MAX_TRIAGE_ENEMIES; i++ )
	{
		if ( pQuestData->idHellball[i] != INVALID_ID )
		{
			UNIT * hellball = UnitGetById( pGame, pQuestData->idHellball[i] );
			if ( hellball )
			{
				UnitFree( hellball, UFF_SEND_TO_CLIENTS );
			}
			pQuestData->idHellball[i] = INVALID_ID;
		}
	}

	// remove emmera event
	ASSERT_RETURN( pQuestData->idEmmera != INVALID_ID );
	UNIT * pEmmera = UnitGetById( pGame, pQuestData->idEmmera );
	UnitUnregisterTimedEvent( pEmmera, sEmmeraTriageControl );

	// re-init data
	sInitGameData( pQuestData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus < QUEST_STATUS_ACTIVE )
	{
		if ( ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) ) &&
			 ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelTemplarBase ) &&
			 ( s_QuestIsPrereqComplete( pQuest ) ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		return QMR_OK;
	}
	else if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TALK_EMMERA ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_UPDATE ) == QUEST_STATE_ACTIVE ) &&  !QuestGamePlayerInGame( pQuest->pPlayer ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TALK_ARPHAUN ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasMedkit(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;

	QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TALK_EMMERA, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_TRIAGE_SAVE, QUEST_STATE_ACTIVE );

	// begin event
	s_QuestGameJoin( pQuest );
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
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) ) &&
			( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelTemplarBase ) &&
			( s_QuestIsPrereqComplete( pQuest ) ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_VIDEO_INTRO );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TALK_EMMERA ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_TALK_EMMERA );
			return QMR_NPC_TALK;
		}
		if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_SAVE ) == QUEST_STATE_ACTIVE ) && !QuestGamePlayerInGame( pPlayer ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_TALK_EMMERA_REPEAT );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_UPDATE ) == QUEST_STATE_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_SAVE ) == QUEST_STATE_COMPLETE  )
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_SAVED_EMMERA );
			else
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_FAILED_EMMERA );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TALK_ARPHAUN ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRIAGE_ARPHAUN_COMPLETE, INVALID_LINK, GI_MONSTER_EMMERA );
			return QMR_NPC_TALK;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoComplete( QUEST * pQuest )
{
	s_QuestActivate( pQuest );
	s_QuestMapReveal( pQuest, GI_LEVEL_CROWN_OFFICE_ROW );
	// complete next step if already at COR
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	if ( UnitGetLevelDefinitionIndex( pPlayer ) == pQuestData->nLevelCrownOfficeRow )
	{
		QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TRAVEL_COR, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TALK_EMMERA, QUEST_STATE_ACTIVE );
	}
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
		case DIALOG_TRIAGE_VIDEO_INTRO:
		{
			sVideoComplete( pQuest );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRIAGE_TALK_EMMERA:
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasMedkit;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTTRIAGEOFFER, tActions );
			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_TRIAGE_TALK_EMMERA_REPEAT:
		{
			// if I have a medkit, just join the event
			QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
			if ( s_QuestFindFirstItemOfType( pQuest->pPlayer, pQuestData->nMedkit ) == NULL )
			{
				// give player another medkit
				OFFER_ACTIONS tActions;
				tActions.pfnAllTaken = sHasMedkit;
				tActions.pAllTakenUserData = pQuest;
				s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUESTTRIAGEOFFER, tActions );
				return QMR_OFFERING;
			}
			s_QuestGameJoin( pQuest );
			s_QuestUpdateCastInfo( pQuest );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRIAGE_SAVED_EMMERA:
		{
			// take med kit
			QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
			s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nMedkit );
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_UPDATE, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TRAVEL_BASE, QUEST_STATE_ACTIVE );
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRIAGE_FAILED_EMMERA:
		{
			return QMR_FINISHED;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRIAGE_ARPHAUN_COMPLETE:
		{
			s_QuestComplete( pQuest );
			return QMR_FINISHED;
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
	if ( pMessage->nDialog == DIALOG_TRIAGE_VIDEO_INTRO )
	{
		sVideoComplete( pQuest );
		return QMR_FINISHED;
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

	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTemplarBase )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TRAVEL_BASE ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TRAVEL_BASE, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TALK_ARPHAUN, QUEST_STATE_ACTIVE );
			return QMR_OK;
		}
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelCrownOfficeRow )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_TRAVEL_COR ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TRAVEL_COR, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_TALK_EMMERA, QUEST_STATE_ACTIVE );
			return QMR_OK;
		}
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageGameMessage( 
	QUEST *pQuest,
	const QUEST_MESSAGE_GAME_MESSAGE *pMessage)
{
	QUEST_GAME_MESSAGE_TYPE eCommand = (QUEST_GAME_MESSAGE_TYPE)pMessage->nCommand;
	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );

	switch ( eCommand )
	{
		//----------------------------------------------------------------------------
		case QGM_GAME_NEW:
		{
			// first one in quest-game, so start it up!
			sInitGameData( pQuestData );
			sSetSpawnLocations( pQuest );
			sSetupEmmera( pQuest );
			sSpawnInitalInjured( pQuest );
			sSpawnHellball( pQuest, FALSE );
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
			// send score
			sShowScore( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_LEAVE:
		{
			// turn off display
			s_QuestGameUIMessage( pQuest, QUIM_GLOBAL_STRING_SCORE, GS_INVALID, 0, 0, UISMS_FORCEOFF );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_VICTORY:
		{
			pQuestData->bSuccess = TRUE;
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_SAVE, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_UPDATE, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_FAILED:
		{
			pQuestData->bSuccess = FALSE;
			QuestStateSet( pQuest, QUEST_STATE_TRIAGE_UPDATE, QUEST_STATE_ACTIVE );
			s_QuestUpdateCastInfo( pQuest );
			s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TRIAGE_FAILED_MESSAGE );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_GAME_ARENA_RESPAWN:
		{
			return QMR_INVALID;
		}

		//----------------------------------------------------------------------------
		// custom Triage messages
		//----------------------------------------------------------------------------

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TRIAGE_NEW_INJURED:
		{
			int index = pMessage->dwData;
			ASSERT_RETVAL( ( index >= 0 ) && ( index < MAX_TRIAGE_INJURED ), QMR_OK );
			ASSERT_RETVAL( pQuestData->idInjured[index] == INVALID_ID, QMR_OK );
			ASSERT_RETVAL( pMessage->pUnit, QMR_OK );
			pQuestData->idInjured[index] = UnitGetId( pMessage->pUnit );
			pQuestData->nNumInjured++;
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TRIAGE_INJURED_DEAD:
		{
			pQuestData->nNumDead++;
			int index = -1;
			for ( int i = 0; ( i < MAX_TRIAGE_INJURED) && ( index == -1 ); i++ )
			{
				if ( pQuestData->idInjured[i] == UnitGetId( pMessage->pUnit ) )
				{
					index = i;
				}
			}
			ASSERTX_RETVAL( index != -1, QMR_OK, "Could not find a dying injured triage object" );
			ASSERT_RETVAL( pQuestData->idInjured[index] != INVALID_ID, QMR_OK );
			pQuestData->idInjured[index] = INVALID_ID;
			pQuestData->nNumInjured--;
			sShowScore( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TRIAGE_INJURED_SAVED:
		{
			pQuestData->nNumSaved++;
			int index = pMessage->dwData;
			ASSERT_RETVAL( ( index >= 0 ) && ( index < MAX_TRIAGE_INJURED ), QMR_OK );
			ASSERT_RETVAL( pQuestData->idInjured[index] != INVALID_ID, QMR_OK );
			pQuestData->idInjured[index] = INVALID_ID;
			pQuestData->nNumInjured--;
			sShowScore( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TRIAGE_NEW_HELLBALL:
		{
			int index = pMessage->dwData;
			ASSERT_RETVAL( ( index >= 0 ) && ( index < MAX_TRIAGE_ENEMIES ), QMR_OK );
			ASSERT_RETVAL( pQuestData->idHellball[index] == INVALID_ID, QMR_OK );
			ASSERT_RETVAL( pMessage->pUnit, QMR_OK );
			pQuestData->idHellball[index] = UnitGetId( pMessage->pUnit );
			pQuestData->nNumHellballs++;
			break;
		}

		//----------------------------------------------------------------------------
		case QGM_CUSTOM_TRIAGE_HELLBALL_KILL:
		{
			int index = -1;
			for ( int i = 0; ( i < MAX_TRIAGE_ENEMIES ) && ( index == -1 ); i++ )
			{
				if ( pQuestData->idHellball[i] == UnitGetId( pMessage->pUnit ) )
				{
					index = i;
				}
			}
			ASSERTX_RETVAL( index != -1, QMR_OK, "Could not find a dying hellball in triage" );
			pQuestData->idHellball[index] = INVALID_ID;
			pQuestData->nNumHellballs--;
			break;
		}

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
	{
		return QMR_OK;
	}

	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	if ( !pLevel )
	{
		return QMR_OK;
	}

	if ( pLevel->m_nLevelDef == pQuestData->nLevelCrownOfficeRow )
	{
		s_QuestGameLeave( pQuest );
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
		QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nMedkit )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				UNIT * pPlayer = pQuest->pPlayer;
				if ( !pPlayer )
					return QMR_USE_FAIL;
				ROOM * pRoom = UnitGetRoom( pPlayer );
				if ( !pRoom )
					return QMR_USE_FAIL;
				for ( UNIT * test = pRoom->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
				{
					if ( UnitGetGenus( test ) != GENUS_OBJECT )
						continue;
					int nClass = UnitGetClass( test );
					if ( ( nClass == pQuestData->nGravelyInjured ) || ( nClass == pQuestData->nSeverelyInjured ) )
					{
						float distsq = UnitsGetDistanceSquaredXY( pPlayer, test );
						if ( distsq <= ( QUEST_TRIAGE_USE_DISTANCE * QUEST_TRIAGE_USE_DISTANCE ) )
						{
							return QMR_USE_OK;
						}
					}
				}
				return QMR_USE_FAIL;
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
		QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nMedkit )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_SAVE ) == QUEST_STATE_ACTIVE )
				{
					// check for a nearby injured
					UNIT * pPlayer = pQuest->pPlayer;
					for ( int i = 0; i < MAX_TRIAGE_INJURED; i++ )
					{
						if ( pQuestData->idInjured[i] != INVALID_ID )
						{
							UNIT * pInjured = UnitGetById( pGame, pQuestData->idInjured[i] );
							float distsq = UnitsGetDistanceSquaredXY( pPlayer, pInjured );
							if ( distsq <= ( QUEST_TRIAGE_USE_DISTANCE * QUEST_TRIAGE_USE_DISTANCE ) )
							{
								s_QuestOperateDelay( pQuest, sMedkitUse, UnitGetId( pInjured ), 3 * GAME_TICKS_PER_SECOND );
								return QMR_USE_OK;
							}
						}
					}
					s_QuestGameUIMessage( pQuest, QUIM_DIALOG, DIALOG_TRIAGE_TOO_FAR_AWAY );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// if the hellball died let everyone know
	if ( UnitIsMonsterClass( pVictim, pQuestData->nHellball ) )
	{
		s_QuestGameCustomMessage( pQuest, QGM_CUSTOM_TRIAGE_HELLBALL_KILL, pVictim );
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TRIAGE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	int nTargetClass = UnitGetClass( pTarget );

	if ( ( nTargetClass == pQuestData->nGravelyInjured ) || ( nTargetClass == pQuestData->nSeverelyInjured ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRIAGE_SAVE ) == QUEST_STATE_ACTIVE )
		{
			s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );
			s_QuestOperateDelay( pQuest, sMedkitUse, UnitGetId( pTarget ), 3 * GAME_TICKS_PER_SECOND );
			return QMR_FINISHED;
		}
		return QMR_OK;
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
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTriage(
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
	QUEST_DATA_TRIAGE * pQuestData)
{
	pQuestData->nEmmera					= QuestUseGI( pQuest, GI_MONSTER_EMMERA );
	pQuestData->nLordArphaun			= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelTemplarBase		= QuestUseGI( pQuest, GI_LEVEL_TEMPLAR_BASE );
	pQuestData->nLevelCrownOfficeRow	= QuestUseGI( pQuest, GI_LEVEL_CROWN_OFFICE_ROW );
	pQuestData->nMedkit					= QuestUseGI( pQuest, GI_ITEM_TRIAGE_MEDKIT );
	pQuestData->nHellball				= QuestUseGI( pQuest, GI_MONSTER_HELLGATE_INSTANT );
	pQuestData->nGravelyInjured			= QuestUseGI( pQuest, GI_OBJECT_TRIAGE_GRAVELY_INJURED );
	pQuestData->nSeverelyInjured		= QuestUseGI( pQuest, GI_OBJECT_TRIAGE_SEVERELY_INJURED );
	pQuestData->nEmmeraAI				= QuestUseGI( pQuest, GI_AI_EMMERA_SUPERNOVA );

	sInitGameData( pQuestData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTriage(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRIAGE * pQuestData = ( QUEST_DATA_TRIAGE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TRIAGE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmera );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLordArphaun );

	if ( IS_SERVER( pGame ) )
	{
		s_QuestGameDataInit( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTriage(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
