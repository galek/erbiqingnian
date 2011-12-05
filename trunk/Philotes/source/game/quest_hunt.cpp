//----------------------------------------------------------------------------
// FILE: quest_hunt.cpp
//
// The hunt quest is a template quest, in which the player must kill a 
// a monster with a proper name in a specified level. If the monster does
// not exist in the level when the player enters (with the quest active) the
// monster is spawned.
//
// Code is adapted from the hunt task, in tasks.cpp.
// 
// author: Chris March, 1-10-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_hunt.h"
#include "quests.h"
#include "s_quests.h"
#include "monsters.h"
#include "npc.h"
#include "quest_template.h"
#include "room_path.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const DWORD QUEST_HUNT_SAVE_VERSION = 515;

//----------------------------------------------------------------------------
enum QUEST_HUNT_MESSAGE_CONSTANTS
{
	MAX_QUEST_HUNT_UNIT_TARGETS = 3,		// max quest unit targets (increase this and you need to add stats to save the targets)
};

//----------------------------------------------------------------------------
enum QUEST_HUNT_UNIT_STATUS
{
	QUEST_HUNT_UNIT_STATUS_INVALID = -1,	// invalid (or unused) status

	QUEST_HUNT_UNIT_STATUS_ALIVE,	// do NOT renumber, value is written to save files
	QUEST_HUNT_UNIT_STATUS_DEAD,	// do NOT renumber, value is written to save files

	NUM_QUEST_HUNT_UNIT_STATUS				// keep this last
};

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_HUNT_UNIT_TARGET
{
	UNITID idTarget;
	QUEST_HUNT_UNIT_STATUS eStatus;
	UNITID idKiller;
};

//----------------------------------------------------------------------------
struct TARGET_SAVE_LOOKUP
{
	STATS_TYPE eStatName;			// unique name of target
	STATS_TYPE eStatMonsterClass;	// target monster class
	STATS_TYPE eStatStatus;			// status of target
};

//----------------------------------------------------------------------------
struct RESTORE_TARGET
{
	SPECIES speciesTarget;
	int nTargetUniqueName;
	QUEST_HUNT_UNIT_STATUS eTargetStatus;
};

static const TARGET_SAVE_LOOKUP sgtTargetSaveLookup[ MAX_QUEST_HUNT_UNIT_TARGETS ] = 
{
	{	STATS_SAVE_QUEST_HUNT_TARGET1_NAME, 
		STATS_SAVE_QUEST_HUNT_TARGET1_MONSTER_CLASS, 
		STATS_SAVE_QUEST_HUNT_TARGET1_STATUS },

	{	STATS_SAVE_QUEST_HUNT_TARGET2_NAME, 
		STATS_SAVE_QUEST_HUNT_TARGET2_MONSTER_CLASS, 
		STATS_SAVE_QUEST_HUNT_TARGET2_STATUS },

	{	STATS_SAVE_QUEST_HUNT_TARGET3_NAME, 
		STATS_SAVE_QUEST_HUNT_TARGET3_MONSTER_CLASS, 
		STATS_SAVE_QUEST_HUNT_TARGET3_STATUS },
};

//----------------------------------------------------------------------------
struct QUEST_DATA_HUNT
{
	// unit targets	
	QUEST_HUNT_UNIT_TARGET unitTargets[ MAX_QUEST_HUNT_UNIT_TARGETS ];
	int nNumUnitTargets;				// count of unitTargets[]
 
	// spawnable target/spawning
	SPAWNABLE_UNIT_TARGET tSpawnableUnitTargets[ MAX_QUEST_HUNT_UNIT_TARGETS ];
	int mNumSpawnableUnitTargets;				// count of tSpawnableUnitTargets[]
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_HUNT * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_HUNT *)pQuest->pQuestData;
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitTargetInit( 
	QUEST_HUNT_UNIT_TARGET* pUnitTarget)
{
	pUnitTarget->idTarget = INVALID_ID;
	pUnitTarget->eStatus = QUEST_HUNT_UNIT_STATUS_INVALID;
	pUnitTarget->idKiller = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnableUnitTargetInit( 
	SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget)
{
	pSpawnableUnitTarget->speciesTarget = SPECIES_NONE;
	pSpawnableUnitTarget->nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRestoreTargetInit(
	RESTORE_TARGET* pRestoreTarget)
{
	pRestoreTarget->speciesTarget = SPECIES_NONE;
	pRestoreTarget->nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
	pRestoreTarget->eTargetStatus = QUEST_HUNT_UNIT_STATUS_INVALID;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSaveQuestData(
	UNIT* pPlayer,
	QUEST* pQuest )
{
	int nQuestIndex = pQuest->nQuest;
	QUEST_DATA_HUNT *pQuestData = (QUEST_DATA_HUNT *)pQuest->pQuestData;

	UnitSetStat( pPlayer, STATS_SAVE_QUEST_HUNT_VERSION, QUEST_HUNT_SAVE_VERSION );

	// how many targets are there (they are either spawnable or instanced)
	int nNumTargets = pQuestData->mNumSpawnableUnitTargets;

	// save target count
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	UnitSetStat( pPlayer, STATS_SAVE_QUEST_HUNT_TARGET_COUNT, nQuestIndex, nDifficulty, nNumTargets );

	// save all targets
	for (int i = 0; i < nNumTargets; ++i)
	{
		const QUEST_HUNT_UNIT_TARGET* pQuestTarget = &pQuestData->unitTargets[ i ];
		const SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = &pQuestData->tSpawnableUnitTargets[ i ];

		// lookup the stat save structure that we use for this target index
		const TARGET_SAVE_LOOKUP* pTargetSaveLookup = &sgtTargetSaveLookup[ i ];

		// save target status, the target is assumed to be alive unless it has
		// actually been instanced and killed
		QUEST_HUNT_UNIT_STATUS eTargetStatus = QUEST_HUNT_UNIT_STATUS_ALIVE;
		if (pQuestTarget != NULL)
		{
			if (pQuestTarget->eStatus == QUEST_HUNT_UNIT_STATUS_DEAD )
			{
				eTargetStatus = pQuestTarget->eStatus;
			}
		}
		UnitSetStat( pPlayer, pTargetSaveLookup->eStatStatus, nQuestIndex, nDifficulty, eTargetStatus );

		// save target name
		UnitSetStat( pPlayer, pTargetSaveLookup->eStatName, nQuestIndex, nDifficulty, pSpawnableUnitTarget->nTargetUniqueName );		

		// save target class
		UnitSetStat( pPlayer, pTargetSaveLookup->eStatMonsterClass, nQuestIndex, nDifficulty, GET_SPECIES_CLASS( pSpawnableUnitTarget->speciesTarget ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sCanBeQuestKillTarget(
	GAME* pGame, 
	int nClass, 
	const UNIT_DATA* pUnitData)
{
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GOOD))
	{
		return FALSE;
	}
	if (!UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CAN_BE_CHAMPION))
	{
		return FALSE;
	}
	return TRUE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sQuestAllTargetUnitsDead(
	QUEST_DATA_HUNT* pQuest)
{
	for (int i = 0; i < pQuest->nNumUnitTargets; ++i)
	{
		QUEST_HUNT_UNIT_TARGET* pUnitTarget = &pQuest->unitTargets[ i ];
		if (pUnitTarget->eStatus != QUEST_HUNT_UNIT_STATUS_DEAD)
		{
			return FALSE;
		}
	}
	return TRUE;  // all targets daed
}

//----------------------------------------------------------------------------
struct SPAWN_MONSTER_CALLBACK_DATA
{
	QUEST_DATA_HUNT* pQuestData;
	SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget;
	int nUnitTargetIndex;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnableUnitTargetSpawnedCallback(
	UNIT* pMonster,
	void* pCallbackData)
{
	SPAWN_MONSTER_CALLBACK_DATA	*pSpawnMonsterCallbackData = (SPAWN_MONSTER_CALLBACK_DATA*)pCallbackData;
	QUEST_DATA_HUNT* pQuestData = pSpawnMonsterCallbackData->pQuestData;
	SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = pSpawnMonsterCallbackData->pSpawnableUnitTarget;
	ASSERTX_RETURN( (GENUS)GET_SPECIES_GENUS( pSpawnableUnitTarget->speciesTarget ) == GENUS_MONSTER, "Expected monster for spawnable unit target" );
	int nUnitTargetIndex = pSpawnMonsterCallbackData->nUnitTargetIndex;

	// what is the unit target index for this spawn (note that a spawn can cause more than
	// just the single target monster to be	spawned cause they come in packs)
	QUEST_HUNT_UNIT_TARGET* pQuestUnitTarget = &pQuestData->unitTargets[ nUnitTargetIndex ];

	// if there is not already a target here, then this is now the target		
	if (pQuestUnitTarget->idTarget == INVALID_ID)
	{
		// set as target
		pQuestUnitTarget->idTarget = UnitGetId( pMonster );
		pQuestUnitTarget->eStatus = QUEST_HUNT_UNIT_STATUS_ALIVE;

		// set name on monster if specified
		if (pSpawnableUnitTarget->nTargetUniqueName != MONSTER_UNIQUE_NAME_INVALID)
		{
			MonsterProperNameSet( pMonster, pSpawnableUnitTarget->nTargetUniqueName, TRUE );
		}

		// quest now has an additional target
		pQuestData->nNumUnitTargets++;
	}
}

static UNIT *sFindMonsterInLevel(
	LEVEL *pLevel,
	int nMonsterClass,
	int nTargetUniqueName )
{
	// check the level for death
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetClass( test ) == nMonsterClass &&
				 ( nTargetUniqueName == NULL ||
				   MonsterUniqueNameGet( test ) == nTargetUniqueName ) )
			{
				// found one already
				return test;
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAttemptPendingLevelSpawns(
    GAME* pGame,
    QUEST* pQuest,
    QUEST_DATA_HUNT* pQuestData,
    LEVEL* pLevel)
{
	if ( !QuestIsActive( pQuest ) ||
		 pQuest->pQuestDef->bDisableSpawning )
	{
		return;
	}

	// does quest have a spawnable level definition for spawns but no level instance,
	// in that case we assume that any level of the same definition type will do
	int nLevelDefinition = pQuest->pQuestDef->nLevelDefDestinations[0];
	if (nLevelDefinition == INVALID_LINK ||
		nLevelDefinition == LevelGetDefinitionIndex( pLevel ))
	{
		ASSERT_RETURN( LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ) );

		// pick a room in the level to do spawning in
		ROOM* pRoom = NULL;
		s_LevelGetRandomSpawnRooms(pGame, pLevel, &pRoom, 1);
		if (pRoom == NULL)
		{
			ASSERTX( 0, "Unable to pick room for spawnable spawn in level" );
			s_QuestFail( pQuest );
			return;
		}

		// setup callback data
		SPAWN_MONSTER_CALLBACK_DATA callbackData;
		callbackData.pQuestData = pQuestData;
		callbackData.pSpawnableUnitTarget = NULL;
		callbackData.nUnitTargetIndex = -1;
		
		// reset the count of spawned unit targets, then count again in the loop below
		pQuestData->nNumUnitTargets = 0;

		// go through any spawnable units to create
		BOOL bSpawnedEverything = TRUE;
		DWORD dwRandomNodeFlags = 0;
		SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
		for (int i = 0; i < pQuestData->mNumSpawnableUnitTargets; ++i)
		{
			SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = &pQuestData->tSpawnableUnitTargets[ i ];

			// skip this unit if its instance is already dead (this will happen when we restore a quest)
			QUEST_HUNT_UNIT_TARGET* pQuestUnitTarget = &pQuestData->unitTargets[ i ];
			if (pQuestUnitTarget->eStatus == QUEST_HUNT_UNIT_STATUS_DEAD)
			{
				// quest now has an additional target even tho its already dead
				ASSERT( pQuestData->nNumUnitTargets == i );
				pQuestData->nNumUnitTargets++;
				continue;
			}			

			// skip this spawn if it has a unique name and class combination that already exists
			UNIT *pExistingMonster =
				sFindMonsterInLevel( 
					pLevel, 
					GET_SPECIES_CLASS( pSpawnableUnitTarget->speciesTarget ),
					pSpawnableUnitTarget->nTargetUniqueName );
			if ( pExistingMonster != NULL )
			{
				// make sure the unit target points to this unit
				// (don't check if it's dead, because if that's the case, it would have
				// been counted on the death event, if this player was eligible to count the kill)
				pQuestUnitTarget->idTarget = UnitGetId( pExistingMonster );
				pQuestData->nNumUnitTargets++;
				continue;
			}

			// update spawnable target index for callback data
			callbackData.pSpawnableUnitTarget = pSpawnableUnitTarget;

			// when this target is instanced, where will we track the target unit
			callbackData.nUnitTargetIndex = i; 

			// pick a max quality for this unique
			int nMonsterClassToSpawn = GET_SPECIES_CLASS( pSpawnableUnitTarget->speciesTarget );
			const UNIT_DATA * pMonsterData = 
				UnitGetData(pGame, GENUS_MONSTER, nMonsterClassToSpawn);
			ASSERTX_CONTINUE( pMonsterData != NULL, "NULL UNIT_DATA for hunt Objective Monster");
			int nMonsterQuality = pMonsterData->nMonsterQualityRequired;
			if ( MonsterQualityGetType( nMonsterQuality ) < MQT_UNIQUE )
			{
				nMonsterQuality = MonsterQualityPick( pGame, MQT_TOP_CHAMPION );
			}

			int nTries = 32;
			int nNumSpawned = 0;
			SPAWN_LOCATION tSpawnLocation;
			tSpawnLocation.fRadius = 0.0f;
			// create a spawn entry just for this single target
			SPAWN_ENTRY tSpawn;
			tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
			tSpawn.nIndex = nMonsterClassToSpawn;
			tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;

			while (nNumSpawned == 0 && nTries-- > 0)
			{
				// pick a random node to spawn at
				int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, dwRandomNodeFlags );
				if ( nNodeIndex < 0 )
					continue;
				SpawnLocationInit( &tSpawnLocation, pRoom, nNodeIndex );

				// spawn monster
				ASSERTX( (GENUS)GET_SPECIES_GENUS( pSpawnableUnitTarget->speciesTarget ) == GENUS_MONSTER, "Expected spawnable unit target to be a monster" );
				nNumSpawned = s_RoomSpawnMonsterByMonsterClass( 
					pGame, 
					&tSpawn, 
					nMonsterQuality, 
					pRoom, 
					&tSpawnLocation,
					sSpawnableUnitTargetSpawnedCallback,
					(void*)&callbackData);

			}
			ASSERTX( nNumSpawned >= 1, "Unable to spawn quest unit" );
		}

		// if nothing was spawned signal error
		if (bSpawnedEverything == FALSE && pQuestData->mNumSpawnableUnitTargets > 0)
		{
			ASSERTX( FALSE, "Failed spawn quest unit" );

			s_QuestFail( pQuest );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	QUEST_DATA_HUNT* pQuestData,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	UNIT *pPlayer = pQuest->pPlayer;
	LEVEL *pLevel = UnitGetLevel( pPlayer );

	// do any spawnable level spawns
	sAttemptPendingLevelSpawns( pGame, pQuest, pQuestData, pLevel );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	GAME *pGame,
	UNIT *pPlayer,
	QUEST *pQuest,
	QUEST_DATA_HUNT *pQuestData)
{
	pQuestData->nNumUnitTargets = 0;
	pQuestData->mNumSpawnableUnitTargets = 0;
	for (int i = 0 ;i < MAX_QUEST_HUNT_UNIT_TARGETS; ++i)
	{
		QUEST_HUNT_UNIT_TARGET* pUnitTarget = &pQuestData->unitTargets[ i ];
		sUnitTargetInit( pUnitTarget );
		SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = &pQuestData->tSpawnableUnitTargets[ i ];
		sSpawnableUnitTargetInit( pSpawnableUnitTarget );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddSpawnableUnitTarget(
	QUEST* pQuest,
	QUEST_DATA_HUNT* pQuestData,
	SPECIES speciesTarget,
	int nTargetUniqueName)
{	
	ASSERTX_RETURN( pQuestData->mNumSpawnableUnitTargets < MAX_QUEST_HUNT_UNIT_TARGETS, "Too many targets" );
	SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = &pQuestData->tSpawnableUnitTargets[ pQuestData->mNumSpawnableUnitTargets ];
	pSpawnableUnitTarget->speciesTarget = speciesTarget;
	pSpawnableUnitTarget->nTargetUniqueName = nTargetUniqueName;

	// save in template to send to client
	QUEST_TEMPLATE* pQuestTemplate = &pQuest->tQuestTemplate;
	pQuestTemplate->tSpawnableUnitTargets[ pQuestData->mNumSpawnableUnitTargets ] = *pSpawnableUnitTarget;

	// have another spawnable target now
	pQuestData->mNumSpawnableUnitTargets++;

}

static void sQuestDataPrepareSpawnInfo(
	GAME *pGame,
	QUEST_DATA_HUNT *pQuestData,
	QUEST *pQuest )
{
	sQuestDataInit( pGame, QuestGetPlayer( pQuest ), pQuest, pQuestData );

	const QUEST_DEFINITION *pQuestDefinition = pQuest->pQuestDef;
	int nMonsterClassToSpawn = pQuestDefinition->nObjectiveMonster;
	ASSERTX_RETURN(nMonsterClassToSpawn != INVALID_LINK, "hunt quest template requires a valid Objective Monster in quest.xls");

	// save this in the spawnable units to spawn
	SPECIES speciesTarget = MAKE_SPECIES( GENUS_MONSTER, nMonsterClassToSpawn );

	// pick a unique name, if the monster is not a unique class
	int nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
	const UNIT_DATA * pMonsterData = 
		UnitGetData(pGame, GENUS_MONSTER, nMonsterClassToSpawn);
	ASSERTX_RETURN( pMonsterData != NULL, "NULL UNIT_DATA for hunt Objective Monster, wrong version_package?");
	if ( MonsterQualityGetType( 
			 pMonsterData->nMonsterQualityRequired ) !=
			 MQT_UNIQUE )
	{
		nTargetUniqueName = MonsterProperNamePick( pGame );
	}
	sAddSpawnableUnitTarget( pQuest, pQuestData, speciesTarget, nTargetUniqueName );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sDisplayDescriptionDialog(
	QUEST *pQuest,
	const QUEST_DEFINITION *pQuestDefinition,
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pDialogActivator ) // NPC or item
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	// Setup data for the quest dialog, so that it will read the same
	// if the player does not accept now, and comes back later.
	// reset quest data, in case the player canceled the quest before
	QUEST_DATA_HUNT *pQuestData = (QUEST_DATA_HUNT*) pQuest->pQuestData;
	sQuestDataPrepareSpawnInfo( pGame, pQuestData, pQuest );
	sSaveQuestData( pPlayer, pQuest );

	s_QuestDisplayDialog(
		pQuest,
		pDialogActivator,
		NPC_DIALOG_OK_CANCEL,
		pQuestDefinition->nDescriptionDialog );
#endif
	return QMR_NPC_TALK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestHuntOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	UNIT *pPlayer = tParam.pPlayer;
	QUEST *pQuest = tParam.pQuest;
	int nQuestIndex = pQuest->nQuest;
	GAME *pGame = UnitGetGame( pPlayer );

	// Maybe you can just remove all this logic to begin with and just use the
	// stats themselves to store the necessary data instead of having to convert it after load

	if ( !QuestIsActive( pQuest ) )
		return;

	// read target count
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	int nTargetCount = UnitGetStat( pPlayer, STATS_SAVE_QUEST_HUNT_TARGET_COUNT, nQuestIndex, nDifficulty );

	// init memory for targets
	RESTORE_TARGET tRestoreTargets[ MAX_QUEST_HUNT_UNIT_TARGETS ];
	for (int i = 0; i < MAX_QUEST_HUNT_UNIT_TARGETS; ++i)
	{
		sRestoreTargetInit( &tRestoreTargets[ i ] );
	}

	// read all targets
	for (int i = 0; i < nTargetCount; ++i)
	{		

		// lookup the stat save structure that we use for this target index
		const TARGET_SAVE_LOOKUP* pTargetSaveLookup = &sgtTargetSaveLookup[ i ];

		// get where we'll save the data read
		RESTORE_TARGET* pRestoreTarget = &tRestoreTargets[ i ];

		// read all target data
		pRestoreTarget->eTargetStatus = (QUEST_HUNT_UNIT_STATUS)UnitGetStat( pPlayer, pTargetSaveLookup->eStatStatus, nQuestIndex, nDifficulty );
		ASSERTX( pRestoreTarget->eTargetStatus > QUEST_HUNT_UNIT_STATUS_INVALID && pRestoreTarget->eTargetStatus < NUM_QUEST_HUNT_UNIT_STATUS, "Invalid quest unit target status read from player save file" );

		pRestoreTarget->nTargetUniqueName = UnitGetStat( pPlayer, pTargetSaveLookup->eStatName, nQuestIndex, nDifficulty );
		ASSERTX( pRestoreTarget->nTargetUniqueName < MonsterNumProperNames(), "hunt quest target unique name too large, read from player save file" );

		GENUS eGenusTarget = GENUS_MONSTER;
		int nClassTarget = UnitGetStat( pPlayer, pTargetSaveLookup->eStatMonsterClass, nQuestIndex, nDifficulty );
		pRestoreTarget->speciesTarget = MAKE_SPECIES( eGenusTarget, nClassTarget );
		ASSERTX_RETURN(eGenusTarget >= GENUS_NONE && eGenusTarget < NUM_GENUS, "Invalid quest unit target genus read from player save file");
		ASSERTX_RETURN((unsigned int)nClassTarget < ExcelGetCount(EXCEL_CONTEXT(pGame), UnitGenusGetDatatableEnum(eGenusTarget)), "Invalid quest unit target class read from save file" );

	}		

	// restore level
	QUEST_DATA_HUNT *pQuestData = (QUEST_DATA_HUNT *)pQuest->pQuestData;

	// setup the targets as spawnable targets
	ASSERTX( pQuestData->mNumSpawnableUnitTargets == 0, "Expected no spawnable unit targets" );
	ASSERTX( pQuestData->nNumUnitTargets == 0, "Expected no unit targets" );	
	for (int i = 0; i < nTargetCount; ++i)
	{
		RESTORE_TARGET* pRestoreTarget = &tRestoreTargets[ i ];

		// put target information back into the spawnable quest units
		sAddSpawnableUnitTarget( pQuest, pQuestData, pRestoreTarget->speciesTarget, pRestoreTarget->nTargetUniqueName );		

		// restore status in live target
		QUEST_HUNT_UNIT_TARGET* pQuestUnitTarget = &pQuestData->unitTargets[ i ];
		pQuestUnitTarget->eStatus = pRestoreTarget->eTargetStatus;
		ASSERT( pQuestUnitTarget->eStatus == QUEST_HUNT_UNIT_STATUS_DEAD || pQuestUnitTarget->eStatus == QUEST_HUNT_UNIT_STATUS_ALIVE );

	}

	// there may be a problem reading nightmare quest stats from versions
	// before the difficulty param was added to the stats, in that case
	// re-pick unique names for the monster targets, and fix up the
	// quest data
	if ( pQuestData->mNumSpawnableUnitTargets <= 0 || 
		 pQuestData->mNumSpawnableUnitTargets != nTargetCount )
	{
		sQuestDataPrepareSpawnInfo( pGame, pQuestData, pQuest );
		sSaveQuestData( pPlayer, pQuest );
	}

	ASSERTX( pQuestData->mNumSpawnableUnitTargets == UnitGetStat( pPlayer, STATS_SAVE_QUEST_HUNT_TARGET_COUNT, nQuestIndex, nDifficulty ), "Pending targets not restored properly" );

	MSG_SCMD_QUEST_TEMPLATE_RESTORE tMessage;
	QuestTemplateInit( &tMessage.tQuestTemplate );

	tMessage.nQuest = pQuest->nQuest;
	tMessage.idPlayer = UnitGetId( pPlayer );
	tMessage.tQuestTemplate = pQuest->tQuestTemplate;

	// send it
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_QUEST_TEMPLATE_RESTORE, &tMessage );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

	// Quest giver
	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ) && s_QuestCanBeActivated( pQuest ))
		{
			return s_sDisplayDescriptionDialog(pQuest, pQuestDefinition, pGame, pPlayer, pSubject );
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_HUNTTEMPLATE_KILL ) == QUEST_STATE_COMPLETE )
		{
			if ( s_QuestIsRewarder( pQuest, pSubject ) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nCompleteDialog,
					pQuestDefinition->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( s_QuestIsRewarder( pQuest, pSubject ) )
			{
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDefinition->nIncompleteDialog );

				return QMR_NPC_TALK;
			}
		}
	}
#endif	
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{	
					QuestStateSet( pQuest, QUEST_STATE_HUNTTEMPLATE_KILL, QUEST_STATE_ACTIVE );
					
					// try to start the unit instancing process for levels that are already active
					QUEST_DATA_HUNT *pQuestData = (QUEST_DATA_HUNT *)pQuest->pQuestData;

					GAME* pGame = UnitGetGame( pPlayer );
					LEVEL* pLevel = UnitGetLevel( pPlayer );
					if (LevelIsActive(pLevel) == TRUE)
					{
						sAttemptPendingLevelSpawns( pGame, pQuest, pQuestData, pLevel );
					}

					sSaveQuestData( pPlayer, pQuest );
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_HUNTTEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest,  pTalkingTo ) )
			{	
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_HUNTTEMPLATE_RETURN, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{								
					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}
			}
		}
	}

	return QMR_OK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage)
{
	UNIT *pVictim = pMessage->pVictim;
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	QUEST_DATA_HUNT * pQuestData = sQuestDataGet( pQuest );

	// is the quest state for killing monsters active, and is the victim
	// in the required level, if any?
	if ( QuestStateGetValue( pQuest, QUEST_STATE_HUNTTEMPLATE_KILL ) == 
		 QUEST_STATE_ACTIVE )
	{
		BOOL bKilledTarget = FALSE;
		if ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		     pQuestDef->nLevelDefDestinations[0] == 
			 UnitGetLevelDefinitionIndex( pVictim ) )
		{
			UNITID idVictim = UnitGetId( pVictim );
			UNITID idKiller = UnitGetId( pMessage->pKiller );

			ASSERTV(pQuestData->nNumUnitTargets > 0, "quest %s: no monster target UNITs known for active hunt quest", (pQuestDef && pQuestDef->szName) ? pQuestDef->szName : "[unknown]");
			// is this victim a target of the quest
			for (int i = 0; i < pQuestData->nNumUnitTargets; ++i)
			{
				QUEST_HUNT_UNIT_TARGET* pUnitTarget = &pQuestData->unitTargets[ i ];

				// it should either be very rare, or impossible for a second 
				// monster with the same UNITID to spawn after the quest target is 
				// killed, but it might happen, so don't kill twice
				if (pUnitTarget->idTarget == idVictim &&
					pUnitTarget->eStatus != QUEST_HUNT_UNIT_STATUS_DEAD)
				{
					pUnitTarget->eStatus = QUEST_HUNT_UNIT_STATUS_DEAD;
					pUnitTarget->idKiller = idKiller;
					bKilledTarget = TRUE;
					break; // there should only be one entry for each UNITID
				}
			}
		}

		// see if quest is complete
		if (bKilledTarget == TRUE)
		{
			sSaveQuestData( pMessage->pKiller, pQuest );
			// TODO cmarch: check if party member who killed monster is nearby?
			if (sQuestAllTargetUnitsDead( pQuestData ) == TRUE)
			{
				QuestStateSet( pQuest, QUEST_STATE_HUNTTEMPLATE_KILL, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_HUNTTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
			}
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestHuntClearStats( 
	QUEST * pQuest )
{
	UNIT * pPlayer = QuestGetPlayer( pQuest );
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	int nQuestIndex = pQuest->nQuest;
	UnitClearStat( pPlayer, STATS_SAVE_QUEST_HUNT_TARGET_COUNT, StatParam(STATS_SAVE_QUEST_HUNT_TARGET_COUNT, nQuestIndex, nDifficulty) );
	for (int i = 0; i < arrsize( sgtTargetSaveLookup ); ++i)
	{
		// lookup the stat save structure that we use for this target index
		const TARGET_SAVE_LOOKUP* pTargetSaveLookup = &sgtTargetSaveLookup[ i ];
		UnitClearStat( pPlayer, pTargetSaveLookup->eStatStatus, StatParam( pTargetSaveLookup->eStatStatus, nQuestIndex, nDifficulty ) );
		UnitClearStat( pPlayer, pTargetSaveLookup->eStatName, StatParam( pTargetSaveLookup->eStatName, nQuestIndex, nDifficulty ) );		
		UnitClearStat( pPlayer, pTargetSaveLookup->eStatMonsterClass, StatParam( pTargetSaveLookup->eStatMonsterClass, nQuestIndex, nDifficulty ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( pItem )
	{
		if ( pQuestDef->nGiverItem != INVALID_LINK &&
			pQuestDef->nGiverItem == UnitGetClass( pItem ) )
		{
			if ( QuestIsInactive( pQuest ) &&
				s_QuestCanBeActivated( pQuest ) )
			{
				return s_sDisplayDescriptionDialog(
					pQuest, 
					pQuestDef, 
					pGame, 
					QuestGetPlayer( pQuest ), 
					pItem );
			}
			return QMR_USE_FAIL;
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
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			return s_sQuestMessagePartyKill( pQuest, pPartyKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return s_sQuestMessageEnterLevel( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						sQuestDataGet( pQuest ),
						pMessageEnterLevel );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				sQuestHuntClearStats( pQuest );
			}
			// fall through

		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return QuestTemplateMessageStatus( pQuest, pMessageStatus );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_ACTIVATED:
		{
			QUEST_DATA_HUNT *pQuestData = sQuestDataGet( pQuest );
			ASSERTX_RETVAL( pQuest->pQuestDef, QMR_OK, "expected quest definition");
			ASSERTV_RETVAL( pQuestData, QMR_OK, "quest \"%\" expected quest data", pQuest->pQuestDef->szName );
			if ( pQuestData->mNumSpawnableUnitTargets <= 0 )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				ASSERTV_RETVAL( pPlayer, QMR_OK, "quest \"%\" expected player", pQuest->pQuestDef->szName );
				ASSERTV_RETVAL( UnitGetGame( pPlayer ), QMR_OK, "quest \"%\" expected game", pQuest->pQuestDef->szName );
				sQuestDataPrepareSpawnInfo( UnitGetGame( pPlayer ), pQuestData, pQuest );
				sSaveQuestData( pPlayer, pQuest );
			}
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_USE_ITEM:
		{
			const QUEST_MESSAGE_USE_ITEM *pUseItemMessage = ( const QUEST_MESSAGE_USE_ITEM * )pMessage;
			return s_sQuestMessageUseItem( pQuest, pUseItemMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestHuntFree(
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
void QuestHuntInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_HUNT * pQuestData = ( QUEST_DATA_HUNT * )GMALLOCZ( pGame, sizeof( QUEST_DATA_HUNT ) );
	pQuest->pQuestData = pQuestData;
	sQuestDataInit( pGame, tParam.pPlayer, pQuest, pQuestData );

	// get credit for monster kills by party members
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	ASSERTV_RETURN(pQuest->pQuestDef->nObjectiveMonster != INVALID_LINK, "quest \"%s\": Objective Monster in quest.xls is blank or not found in monsters.xls", pQuest->pQuestDef->szName);
}
