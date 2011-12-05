//----------------------------------------------------------------------------
// FILE: tasks.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "clients.h"
#include "combat.h"
#include "console.h"
#include "district.h"
#include "dungeon.h"
#include "excel.h"
#include "excel_private.h"
#include "faction.h"
#include "game.h"
#include "globalindex.h"
#include "inventory.h"
#include "items.h"
#include "level.h"
#include "log.h"
#include "units.h"
#include "monsters.h"
#include "npc.h"
#include "objects.h"
#include "picker.h"
#include "s_message.h"
#include "s_reward.h"
#include "script.h"
#include "spawnclass.h"
#include "stats.h"
#include "dialog.h"
#include "tasks.h"
#include "treasure.h"
#include "unit_priv.h"
#include "uix.h"
#include "room_layout.h"
#include "room_path.h"
#include "LevelAreas.h"
#include "Currency.h"
//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
BOOL gbGenerateTimeDelay = TRUE;
static const DWORD TASK_SAVE_VERSION = (4 + GLOBAL_FILE_VERSION);
static const DWORD TASK_TOO_OLD_TIME = GAME_TICKS_PER_SECOND * 60 * 5;
static const DWORD TASK_TOO_OLD_TIME_TUGBOAT = GAME_TICKS_PER_SECOND * 5;
static const DWORD MIN_TIME_BETWEEN_TASK_GENERATIONS = TASK_TOO_OLD_TIME;	//TODO: This seems like it has to be a # define. Tugboat has it at 300. TASK_TOO_OLD_TIME = 6000
static const DWORD MIN_TIME_BETWEEN_TASK_GENERATIONS_TUGBOAT = TASK_TOO_OLD_TIME_TUGBOAT;	//TODO: This seems like it has to be a # define. Tugboat has it at 300. TASK_TOO_OLD_TIME = 6000
static const int FACTION_POINTS_AID_MIN = 20;		// doing a task that helps a faction gets you this many points
static const int FACTION_POINTS_AID_MAX = 30;		// doing a task that helps a faction gets you this many points
	
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum TASK_CREATE_FLAGS
{
	TC_RESTORING_BIT,				// task is being restored
};

//----------------------------------------------------------------------------
enum TASK_COMPLETE_CONDITION_FLAGS
{
	CONDITION_ALL_TARGET_UNITS_DEAD_BIT,		// all target units must be killed
	CONDITION_ANY_TARGET_UNIT_DEAD_BIT,			// any target unit must be killed
	CONDITION_KILLS_BY_OWNER_BIT,				// any target units that must be killed, must be killed by the task owner
	CONDITION_COLLECTED_EVERYTHING_BIT,			// everything to collect has been collected
	CONDITION_KILLED_REQUIRED_NUMBER_OF_BIT,	// has killed at least X many target types
	CONDITION_TRIGGERED_ALL_BIT,				// has triggered all objects of a specific type
};

//----------------------------------------------------------------------------
enum TASK_UNIT_STATUS
{
	TASK_UNIT_STATUS_INVALID = -1,	// invalid (or unused) status
	
	TASK_UNIT_STATUS_ALIVE,		// do NOT change, values are written to save files
	TASK_UNIT_STATUS_DEAD,		// do NOT change, values are written to save files
	
	NUM_TASK_UNIT_STATUS			// keep this last
};

//----------------------------------------------------------------------------
struct TASK_UNIT_TARGET
{
	UNITID idTarget;
	TASK_UNIT_STATUS eStatus;
	UNITID idKiller;
};

typedef void (*PFN_TASK_ACTIVATE_LEVEL)(GAME * pGame, struct TASK * pTask, LEVEL * pLevel);
typedef void (*PFN_TASK_ACTIVATE_ROOM)(GAME * pGame, struct TASK * pTask, ROOM * pLevel);

//----------------------------------------------------------------------------
struct TASK
{

	// the task template is the "basic" information for task that we can communicate
	// to a client so the can select a task out of N choices
	TASK_TEMPLATE tTaskTemplate;		// type of task and specifiecs

	GAMETASKID idTask;					// task instance id	
	GAME* pGame;						// game task is a part of (for convenience)
	const TASK_DEFINITION* pTaskDefinition;	// from tasks.xls
	int nTaskDefinition;				// task definition index
	TASK_STATUS eStatus;				// task status

	UNITID idPlayer;					// player owner of the task
	SPECIES speciesGiver;				// who gave the task
	int nFactionPrimary;				// primary faction associated with task
	DWORD dwGeneratedTick;				// time task was generated
	DWORD dwAcceptTick;					// time task was accepted

	DWORD dwCompleteConditions;			// see TASK_COMPLETE_CONDITION_FLAGS

	int nRewardExperience;				// Tugboat
	int nRewardGold;					// Tugboat
	int nLevelDepth;						// Tugboat
	
	UNITID idRewards[ MAX_TASK_REWARDS ];	// task rewards

	// unit targets	
	TASK_UNIT_TARGET unitTargets[ MAX_TASK_UNIT_TARGETS ];
	int nNumUnitTargets;				// count of unitTargets[]

	// pending target/spawning
	int nPendingLevelSpawnDefinition;	// we don't have a level id to do pending spawn, but we do have a definition

	int nPendingLevelSpawnDepth;		// Tugboat does have a depth, which will be converted into an index
	int nPendingLevelSpawnArea;			// which area is this level in?
	LEVELID idPendingLevelSpawn;		// level we need to do some pending spawning in
	ROOMID idPendingRoomSpawn;			// room we need to do some pending spawning in
	PENDING_UNIT_TARGET tPendingUnitTargets[ MAX_TASK_UNIT_TARGETS ];
	int nNumPendingTargets;				// count of tPendingUnitTargets[]
	
	PFN_TASK_ACTIVATE_LEVEL pfnLevelActivate;	// Function to call on level activation
	//PFN_TASK_ACTIVATE_ROOM pfnRoomActivate;		// Function to call on room activation

	// collecting
	TASK_COLLECT tTaskCollect[ MAX_TASK_COLLECT_TYPES ]; // things to collect for this task
	int nNumCollect;					// number of entries in tTaskCollect[]

	// triggering
	int nTriggerCount;					// number of objects to trigger

	TASK* pNext;						// next task in master task list
	TASK* pPrev;						// prev task in master task list
	
};

//----------------------------------------------------------------------------
struct TASK_GLOBALS
{
	TASK* pTaskList;		// master task list
	int nNextTaskID;		// master id counter
};

//----------------------------------------------------------------------------
struct TARGET_SAVE_LOOKUP
{
	STATS_TYPE eStatName;			// unique name of target
	STATS_TYPE eStatMonsterClass;		// monster class
	STATS_TYPE eStatStatus;			// status of target
};

//----------------------------------------------------------------------------
struct RESTORE_TARGET
{
	SPECIES speciesTarget;
	int nTargetUniqueName;
	TASK_UNIT_STATUS eTargetStatus;
};
static const TARGET_SAVE_LOOKUP sgtTargetSaveLookup[ MAX_TASK_UNIT_TARGETS ] = 
{
	{	STATS_SAVE_TASK_TARGET1_NAME, 
		STATS_SAVE_TASK_TARGET1_MONSTER_CLASS, 
		STATS_SAVE_TASK_TARGET1_STATUS },
		
	{	STATS_SAVE_TASK_TARGET2_NAME, 
		STATS_SAVE_TASK_TARGET2_MONSTER_CLASS, 
		STATS_SAVE_TASK_TARGET2_STATUS },
		
	{	STATS_SAVE_TASK_TARGET3_NAME, 
		STATS_SAVE_TASK_TARGET3_MONSTER_CLASS, 
		STATS_SAVE_TASK_TARGET3_STATUS },
};

//----------------------------------------------------------------------------
struct COLLECT_SAVE_LOOKUP
{
	STATS_TYPE eStatClass;			// stat to save class of what to collect
	STATS_TYPE eStatCount;			// stat to save how many of these to collect
};
static const COLLECT_SAVE_LOOKUP sgtCollectSaveLookup[ MAX_TASK_COLLECT_TYPES ] = 
{
	{ STATS_SAVE_TASK_COLLECT1_CLASS, STATS_SAVE_TASK_COLLECT1_COUNT },
	{ STATS_SAVE_TASK_COLLECT2_CLASS, STATS_SAVE_TASK_COLLECT2_COUNT },
	{ STATS_SAVE_TASK_COLLECT3_CLASS, STATS_SAVE_TASK_COLLECT3_COUNT },
};

//----------------------------------------------------------------------------
struct EXTERMINATE_SAVE_LOOKUP
{
	STATS_TYPE eStatMonsterClass;
	STATS_TYPE eStatCount;
};
static const EXTERMINATE_SAVE_LOOKUP sgtExterminateSaveLookup[ MAX_TASK_EXTERMINATE ] = 
{
	{ STATS_SAVE_TASK_EXTERMINATE1_MONSTER_CLASS, STATS_SAVE_TASK_EXTERMINATE1_COUNT },
	{ STATS_SAVE_TASK_EXTERMINATE2_MONSTER_CLASS, STATS_SAVE_TASK_EXTERMINATE2_COUNT },
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoSpawnOneObjectPerRoom(
	GAME * pGame,
	TASK * pTask,
	LEVEL * pLevel);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskGlobalsInit(
	TASK_GLOBALS* pTaskGlobals)
{
	pTaskGlobals->pTaskList = NULL;
	pTaskGlobals->nNextTaskID = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TASK_GLOBALS* sTaskGlobalsGet(
	GAME* pGame)
{
	return pGame->m_pTaskGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TasksInitForGame(
	GAME* pGame)
{
	ASSERTX( IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETURN( pGame->m_pTaskGlobals == NULL, "Expected NULL task globals" );

	// allocate globals
	pGame->m_pTaskGlobals = (TASK_GLOBALS*)GMALLOCZ( pGame, sizeof( TASK_GLOBALS ) );
	sTaskGlobalsInit( pGame->m_pTaskGlobals );

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TASK* sTaskFind(
	GAME* pGame,
	GAMETASKID idTask,
	UNIT* pPlayer)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	UNITID idPlayer = INVALID_ID;
	
	// get owner id
	if (pPlayer)
	{
		idPlayer = UnitGetId( pPlayer );
	}	
	
	// just a simple list for now
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		if (pTask->idTask == idTask)
		{
			if (idPlayer == INVALID_ID || idPlayer == pTask->idPlayer)
			{
				return pTask;
			}
		}
		
	}
	
	return NULL;  // task id not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskRewardsDestroy( 
	GAME* pGame,
	TASK* pTask)
{

	// go through items generated as rewards for this task
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
	
		UNITID idReward = pTask->idRewards[ i ];
		if (idReward != INVALID_ID)
		{
			UNIT *pReward = UnitGetById( pGame, pTask->idRewards[ i ] );
			if (pReward)
			{
				UnitFree( pReward, UFF_SEND_TO_CLIENTS );
			}
							
			// break link
			pTask->idRewards[ i ] = INVALID_ID;

		}
				
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskFree(
	GAME* pGame,
	TASK_GLOBALS* pTaskGlobals,
	TASK* pTask)
{
	ASSERTX_RETURN( pTask, "Expected task" );

	// remove any rewards for this task from the player
	sTaskRewardsDestroy( pGame, pTask );
	
	// remove link to next
	if (pTask->pNext)
	{
		pTask->pNext->pPrev = pTask->pPrev;
	}

	// remove prev link	
	if (pTask->pPrev)
	{
		pTask->pPrev->pNext = pTask->pNext;
	}
	else
	{
		pTaskGlobals->pTaskList = pTask->pNext;
	}
	
	// free task
	GFREE( pGame, pTask );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TasksCloseForGame(
	GAME* pGame)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	if (pTaskGlobals)
	{
	
		// free each task
		while (pTaskGlobals->pTaskList)
		{
			sTaskFree( pGame, pTaskGlobals, pTaskGlobals->pTaskList );
		}
			
		// free globals
		GFREE( pGame, pGame->m_pTaskGlobals );
		pGame->m_pTaskGlobals = NULL;
	
	}
		
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkRewardToTask(
	GAME* pGame,
	TASK* pTask,
	UNIT* pReward,
	int nRewardIndex,
	BOOL bAddToInventory)
{		
	UNITID idReward = UnitGetId( pReward );			

	// save this reward as part of this task 
	ASSERTX_RETURN( nRewardIndex < MAX_TASK_REWARDS, "Invalid task reward index" );
	pTask->idRewards[ nRewardIndex ] = idReward;
	pTask->tTaskTemplate.idRewards[ nRewardIndex ] = idReward;
				
	// set stat linking to this task
	UnitSetStat( pReward, STATS_TASK_REWARD, pTask->idTask );

	// this item is now ready to do network communication
	UnitSetCanSendMessages( pReward, TRUE );
	
	// put in reward inventory slot on player
	if(bAddToInventory)
	{
	
		// which player will it go on
		UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
		
		// where will the item go
		int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS );
		
		// put in inventory
		UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pReward, nInventorySlot);
		
		// tell clients about the item
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			if (ClientCanKnowUnit( pClient, pReward ))
			{
				s_SendUnitToClient( pReward, pClient );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sValidateRewards(	
	TASK* pTask,
	UNIT** pRewards,
	int nNumRewards)
{
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	
	if (pTaskDefinition->nMinSlotsOnReward > 0)
	{
		
		// see if each item has mod slots
		BOOL bAllHaveSlots = TRUE;
		for(int i = 0; i < nNumRewards; ++i)
		{
			UNIT* pReward = pRewards[ i ];
			ASSERTX_RETFALSE( pReward, "Expected reward unit" );
			
			if (ItemGetModSlots( pReward ) < pTaskDefinition->nMinSlotsOnReward)
			{
				bAllHaveSlots = FALSE;
				break;
			}				
			
		}
		
		// if all rewards have slots we're ok
		return bAllHaveSlots;
				
	}
	
	return TRUE;	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFillRewardSlots(
	GAME* pGame,
	TASK* pTask,
	UNIT** pRewards,
	int nNumRewards)
{
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	
	// fill the rewards with mods	
	for (int i = 0; i < nNumRewards; ++i)
	{
		UNIT* pReward = pRewards[ i ];
		ASSERTX_RETFALSE( pReward, "Expected task reward object" );

		// add mods to this reward item
		if (ItemGetModSlots( pReward ) > 0)
		{
			if (s_ItemModGenerateAll( pReward, pPlayer, INVALID_LINK, NULL, NULL ) == FALSE)
			{
				return FALSE;
			}
		}
										
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFactionRewardsInit( 
	FACTION_REWARDS *pFactionRewards)
{

	pFactionRewards->nCount = 0;
	for (int i = 0; i < MAX_TASK_FACTION_REWARDS; ++i)
	{
		pFactionRewards->tReward[ i ].nFaction = INVALID_LINK;
		pFactionRewards->tReward[ i ].nPoints = 0;
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sTaskRewardsCreate( 
	GAME* pGame,
	TASK* pTask)
{
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
			
	// mark player has not taken any items from the reward for a task of this type
	UnitClearStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask );

	if( AppIsTugboat() )
	{
		pTask->nRewardExperience = RandGetNum( pGame, pTask->nLevelDepth * 100, pTask->nLevelDepth * 500 );
		pTask->nRewardGold = RandGetNum( pGame, pTask->nLevelDepth * 500, pTask->nLevelDepth * 1200 );
	}
	// get treasure class
	int nTreasureClassReward = pTaskDefinition->nTreasureClassReward;
	if (nTreasureClassReward > INVALID_LINK)
	{
		
		// the only reason we have a loop of tries here is because the treasure classes
		// don't yet support a minimum number of slots generated ... when we have that
		// functionality we can just do the treasure class and now have to keep rerolling it here
		UNIT* pRewards[ MAX_TASK_REWARDS ];
		int nNumRewards = 0;
		int nMaxTries = AppIsHellgate() ? 64 : 1024;
		while (nMaxTries-- > 0)
		{
					
			// setup spawn spec
			ITEM_SPEC tItemSpec;
			tItemSpec.unitOperator = pPlayer;
			SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );  // always identify item			
			SETBIT( tItemSpec.dwFlags, ISF_USEABLE_BY_OPERATOR_BIT ); // always make stuff they can use
			
			// setup spawn result
			ITEMSPAWN_RESULT spawnResult;
			spawnResult.pTreasureBuffer = pRewards;
			spawnResult.nTreasureBufferSize = MAX_TASK_REWARDS;
			if( AppIsTugboat() )
			{
				tItemSpec.nLevel = pTask->nLevelDepth;
			}
			
			// create task rewards 
			s_TreasureSpawnClass( pPlayer, nTreasureClassReward, &tItemSpec, &spawnResult );
			if( AppIsHellgate() )
			{
				ASSERTX( spawnResult.nTreasureBufferCount > 0, "No rewards were generated for task" );
			}
			else
			{
				if( spawnResult.nTreasureBufferCount == 0 )
				{
					continue;
				}
			}

			// mark each of these reward items and add to inventory
			nNumRewards = spawnResult.nTreasureBufferCount;
			if (sValidateRewards( pTask, pRewards, nNumRewards ) == TRUE)
			{
				break;
			}
			else
			{
				// destroy all rewards and try again
				for (int i = 0; i < nNumRewards; ++i)
				{
					UnitFree( pRewards[ i ], 0 );
					pRewards[ i ] = NULL;
				}
				nNumRewards = 0;
			}
			
		}

		if (nNumRewards > 0)
		{
			BOOL bSuccess = TRUE;
			
			// fill rewards with mods if we're supposed to
			if (pTaskDefinition->bFillAllRewardSlots)
			{
				if (sFillRewardSlots( pGame, pTask, pRewards, nNumRewards ) == FALSE)
				{
					bSuccess = FALSE;
				}
			}
					
			// mark each of these reward items and add to inventory
			if (bSuccess == TRUE)
			{
				for (int i = 0; i < nNumRewards; ++i)
				{
					UNIT* pReward = pRewards[ i ];
					sLinkRewardToTask( pGame, pTask, pReward, i, TRUE );			
				}
				
			}
			else
			{
				// if we did not create rewards, destroy any partially created reward items
				for (int i = 0; i < nNumRewards; ++i)
				{
					UnitFree( pRewards[ i ], 0 );
					pRewards[ i ] = NULL;
				}		
				return FALSE;  // failure
			}
						
		}
		else
		{
			return FALSE;  // unable to generate rewards is an error
		}
				
	}
	
	// create faction rewards
	int nFactionPrimary = pTask->nFactionPrimary;
	sFactionRewardsInit( &pTask->tTaskTemplate.tFactionRewards );
	if (nFactionPrimary != INVALID_LINK)
	{
		FACTION_REWARDS *pFactionRewards = &pTask->tTaskTemplate.tFactionRewards;
		
		// give some positive points to the primary aided faction
		pFactionRewards->tReward[ pFactionRewards->nCount ].nFaction = nFactionPrimary;
		pFactionRewards->tReward[ pFactionRewards->nCount ].nPoints = RandGetNum( pGame, FACTION_POINTS_AID_MIN, FACTION_POINTS_AID_MAX );
		pFactionRewards->nCount++;
										
	}
		
	return TRUE;
				
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskCollectInit(
	TASK_COLLECT* pTaskCollect)
{		
	pTaskCollect->nCount = 0;
	pTaskCollect->nItemClass = INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitTargetInit( 
	TASK_UNIT_TARGET* pUnitTarget)
{
	pUnitTarget->idTarget = INVALID_ID;
	pUnitTarget->eStatus = TASK_UNIT_STATUS_INVALID;
	pUnitTarget->idKiller = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPendingUnitTargetInit( 
	PENDING_UNIT_TARGET* pPendingUnitTarget)
{
	pPendingUnitTarget->speciesTarget = SPECIES_NONE;
	pPendingUnitTarget->nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskExterminateInit(
	TASK_EXTERMINATE* pExterminate)
{
	pExterminate->speciesTarget = SPECIES_NONE;
	pExterminate->nCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRestoreTargetInit(
	RESTORE_TARGET* pRestoreTarget)
{
	pRestoreTarget->speciesTarget = SPECIES_NONE;
	pRestoreTarget->nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
	pRestoreTarget->eTargetStatus = TASK_UNIT_STATUS_INVALID;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TaskTemplateInit( 
	TASK_TEMPLATE* pTaskTemplate)
{

	pTaskTemplate->idTask = GAMETASKID_INVALID;
	pTaskTemplate->wCode = (WORD)TASKCODE_INVALID;
	pTaskTemplate->nLevelDefinition = INVALID_LINK;
	pTaskTemplate->speciesGiver = SPECIES_NONE;
	pTaskTemplate->speciesRewarder = SPECIES_NONE;
	//Tugboat
	pTaskTemplate->nLevelDepth = 0;
	pTaskTemplate->nLevelArea = INVALID_LINK;
	pTaskTemplate->nGoldReward = 0;
	pTaskTemplate->nExperienceReward = 0;

	pTaskTemplate->nNumRewardsTaken = 0;	
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
		pTaskTemplate->idRewards[ i ] = INVALID_ID;
	}	

	for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
	{
		TASK_COLLECT* pTaskCollect = &pTaskTemplate->tTaskCollect[ i ];
		sTaskCollectInit( pTaskCollect );
	}

	for (int i = 0; i < MAX_TASK_UNIT_TARGETS; ++i)
	{
		PENDING_UNIT_TARGET* pPendingUnitTarget = &pTaskTemplate->tPendingUnitTargets[ i ];
		sPendingUnitTargetInit( pPendingUnitTarget );
	}

	pTaskTemplate->nTimeLimitInMinutes = 0;
	
	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
	{
		TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
		sTaskExterminateInit( pExterminate );
	}
	pTaskTemplate->nExterminatedCount = 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TaskTemplateGetExterminateGoal(
	const TASK_TEMPLATE* pTaskTemplate)
{
	ASSERTX_RETZERO( pTaskTemplate, "Expected task template" );
	int nGoal = 0;
	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
	{
		const TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
		nGoal += pExterminate->nCount;
	}
	return nGoal;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static TASK* sNewTask( 
	GAME* pGame,
	const TASK_DEFINITION* pTaskDefinition,
	UNIT* pPlayer, 
	SPECIES speciesGiver,
	int nFactionPrimary,
	DWORD dwCreateFlags)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	// allocate new task (poolify this someday)
	TASK* pTask = (TASK*)GMALLOCZ( pGame, sizeof( TASK ) );
		
	// add to master list
	if (pTaskGlobals->pTaskList)
	{
		pTaskGlobals->pTaskList->pPrev = pTask;
	}
	pTask->pNext = pTaskGlobals->pTaskList;
	pTask->pPrev = NULL;
	pTaskGlobals->pTaskList = pTask;

	// assign initial data data
	pTask->idTask = pTaskGlobals->nNextTaskID++;
	pTask->pGame = pGame;
	pTask->pTaskDefinition = pTaskDefinition;
	pTask->nTaskDefinition = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_TASKS, pTaskDefinition->szName);
	pTask->eStatus = TASK_STATUS_GENERATED;
	pTask->speciesGiver = speciesGiver;
	pTask->nFactionPrimary = nFactionPrimary;
	pTask->idPlayer = UnitGetId( pPlayer );
	pTask->dwGeneratedTick = GameGetTick( pGame );
	pTask->dwAcceptTick = 0;
	pTask->dwCompleteConditions = 0;
	pTask->nPendingLevelSpawnDefinition = INVALID_LINK;
	pTask->nPendingLevelSpawnDepth = INVALID_LINK;			//Tugboat
	pTask->idPendingLevelSpawn = INVALID_ID;
	pTask->idPendingRoomSpawn = INVALID_ID;
		
	pTask->nNumUnitTargets = 0;
	pTask->nNumPendingTargets = 0;
	for (int i = 0 ;i < MAX_TASK_UNIT_TARGETS; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		sUnitTargetInit( pUnitTarget );
		PENDING_UNIT_TARGET* pPendingUnitTarget = &pTask->tPendingUnitTargets[ i ];
		sPendingUnitTargetInit( pPendingUnitTarget );
	}

	// rewards
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
		pTask->idRewards[ i ] = INVALID_ID;
	}	

	pTask->nNumCollect = 0;
	for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
	{
		TASK_COLLECT* pTaskCollect = &pTask->tTaskCollect[ i ];
		sTaskCollectInit( pTaskCollect );
	}
	if( AppIsTugboat() )
	{
		pTask->nLevelDepth = UnitGetStat( pPlayer, STATS_LEVEL_DEPTH ) + 1 + RandGetNum( pGame, 2 );
	}
	
	// init template info
	TASK_TEMPLATE* pTaskTemplate = &pTask->tTaskTemplate;
	TaskTemplateInit( pTaskTemplate );
	pTaskTemplate->idTask = pTask->idTask;
	pTaskTemplate->wCode = pTask->pTaskDefinition->wCode;
	pTaskTemplate->speciesGiver = pTask->speciesGiver;
	pTaskTemplate->speciesRewarder = pTask->speciesGiver;  // for now, just make the giver/rewarder the same

	// generate reward item(s)
	if (TESTBIT( dwCreateFlags, TC_RESTORING_BIT ) == FALSE)
	{
		if (sTaskRewardsCreate( pGame, pTask ) == FALSE)
		{
			sTaskFree( pGame, pTaskGlobals, pTask );
			return NULL;
		}
	}
	if( AppIsTugboat() )
	{
		pTaskTemplate->nGoldReward = pTask->nRewardGold;
		pTaskTemplate->nExperienceReward = pTask->nRewardExperience;
	}

	// pick a time limit if it's supposed to have one
	if (pTaskDefinition->codeTimeLimit != NULL_CODE)
	{
		int nCodeLength = 0;
		BYTE* pCode = ExcelGetScriptCode( pGame, DATATABLE_TASKS, pTaskDefinition->codeTimeLimit, &nCodeLength );
		if (pCode)
		{
			int nTimeLimitInMinutes = VMExecI( pGame, pCode, nCodeLength );
			if (nTimeLimitInMinutes > 0)
			{
				// save time limit in template
				pTaskTemplate->nTimeLimitInMinutes = nTimeLimitInMinutes;										
			}
			
		}
		
	}

	return pTask;  // return new task
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
struct TASK_PICK_ROOM_CALLBACK_DATA
{
	TASK* pTask;
	ROOM* pRoomExempt;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomCanHaveTask(
	ROOM* pRoom,
	void* pCallbackData)
{
	TASK_PICK_ROOM_CALLBACK_DATA* pPickRoomData = (TASK_PICK_ROOM_CALLBACK_DATA*)pCallbackData;
	
	// cannot be the exempt room
	if (pRoom == pPickRoomData->pRoomExempt)
	{
		return FALSE;
	}
	
	// check to see if there is a task too close to this one already
	// TODO: write me!!
	
	return TRUE;  // all OK
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM* sGetSpawnRoom(
	GAME* pGame,
	LEVEL* pLevel,
	TASK* pTask,
	ROOM* pRoomExempt)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	SUBLEVEL *pSubLevel = LevelGetPrimarySubLevel( pLevel );
	
	// setup callback data for room filter
	TASK_PICK_ROOM_CALLBACK_DATA callbackData;
	callbackData.pTask = pTask;
	callbackData.pRoomExempt = pRoomExempt;
	
_tryagain:

	// setup preferred flags
	DWORD dwFlags = 0;
	SETBIT( dwFlags, RRF_PLAYERLESS_BIT );
	if( AppIsHellgate() )
	{
		SETBIT( dwFlags, RRF_HAS_MONSTERS_BIT );
		SETBIT( dwFlags, RRF_SPAWN_POINTS_BIT );
		SETBIT( dwFlags, RRF_MUST_ALLOW_MONSTER_SPAWN_BIT );
	}
	
	// get rook	
	ROOM* pRoomSpawn = s_SubLevelGetRandomRoom( pSubLevel, dwFlags, sRoomCanHaveTask, &callbackData );
	
	// try fallback room search options
	if (pRoomSpawn == NULL)
	{
		CLEARBIT( dwFlags, RRF_HAS_MONSTERS_BIT );
		pRoomSpawn = s_SubLevelGetRandomRoom( pSubLevel, dwFlags, sRoomCanHaveTask, &callbackData );
		if (pRoomSpawn == NULL)
		{
			CLEARBIT( dwFlags, RRF_PLAYERLESS_BIT );		
			pRoomSpawn = s_SubLevelGetRandomRoom( pSubLevel, dwFlags, sRoomCanHaveTask, &callbackData );
			if (pRoomSpawn == NULL && callbackData.pRoomExempt != NULL)
			{
				callbackData.pRoomExempt = NULL;  // remove exempt room and try again
				goto _tryagain;
			}
		}
	}

	return pRoomSpawn;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sCanBeTaskKillTarget(
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
static BOOL sIsLevelAllowedForTask(
	GAME* pGame,
	UNIT *pPlayer,
	LEVEL* pLevel,
	UNITID idWarpToLevel,
	TASK* pTask)
{
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;

	// special case for tutorial level (a not town, not safe level)
	const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pLevel );
	if (pLevelDefinition->bTutorial == TRUE)
	{
		return FALSE;
	}

	// check for hostile areas only		
	if (pTaskDefinition->bHostileAreasOnly == TRUE)
	{
		if (LevelIsSafe( pLevel ))
		{
			return FALSE;
		}
	}
	else
	{
		if (LevelIsSafe( pLevel ) == FALSE)
		{
			return FALSE;
		}
	}

	// check for accessible areas only
	if (pTaskDefinition->bAccessibleAreaOnly == TRUE)
	{
		if (idWarpToLevel != INVALID_ID)
		{
			UNIT* pWarpToLevel = UnitGetById( pGame, idWarpToLevel );
			ASSERTX_RETFALSE( pWarpToLevel, "Unable to get warp unit" );
			if (ObjectCanOperate( pPlayer, pWarpToLevel )  != OPERATE_RESULT_OK)
			{
				return FALSE;  // warp is restricted
			}
		}
	}

	// might want to say "no" to any level the player has visited
	// TODO: write me for all projects
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LEVEL* sGetTaskLevel(
	GAME* pGame,
	TASK* pTask)
{
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	
	// try the level that the player is in
	LEVEL* pLevel = UnitGetLevel( pPlayer );
	if ( sIsLevelAllowedForTask( pGame, pPlayer, pLevel, INVALID_ID, pTask ) == FALSE)
	{
		LEVEL* pAvailableLevels[ MAX_CONNECTED_LEVELS ];
		int nNumAvailableLevels = 0;

		// tugboat will consider entire level chains
		BOOL bGetEntireChain = AppIsTugboat();
		
		// get the connected levels to this area
		LEVEL_CONNECTION pConnectedLevels[ MAX_CONNECTED_LEVELS ];
		int nNumConnected = s_LevelGetConnectedLevels( 
			pLevel, 
			pLevel->m_idWarpCreator, 
			pConnectedLevels, 
			MAX_CONNECTED_LEVELS,
			bGetEntireChain);
		
		// create pool of levels that can have this task in it
		if (nNumConnected > 0)
		{
			
			for (int i = 0; i < nNumConnected; ++i)
			{
				LEVEL* pLevel = pConnectedLevels[ i ].pLevel;
				UNITID idWarpToLevel = pConnectedLevels[ i ].idWarp;
				if (sIsLevelAllowedForTask( pGame, pPlayer, pLevel, idWarpToLevel, pTask ))
				{	
					pAvailableLevels[ nNumAvailableLevels++ ] = pLevel;
				}
			}
			
		}
		
		// pick one of the available levels
		if ( nNumAvailableLevels )
		{
			int nPick = RandGetNum( pGame, nNumAvailableLevels );
			pLevel = pAvailableLevels[ nPick ];
		} 
		else
		{
			pLevel = NULL;
		}
						
	}
	
	// return level to use (if any)
	return pLevel;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetTaskArea(
	GAME* pGame,
	TASK* pTask)
{

	int nAreas = ExcelGetNumRows( NULL, DATATABLE_LEVEL_AREAS );
			
	// create pool of areas that can have this task in it
	if (nAreas > 0)
	{
		int nAvailableLevels[ 100 ];
		int nNumAvailableLevels = 0;
			
		for (int i = 0; i < nAreas; ++i)
		{			
			if( MYTHOS_LEVELAREAS::LevelAreaIsHostile( i ) )
			{
				nAvailableLevels[ nNumAvailableLevels++ ] = i;
			}
		
		}
		// pick one of the available levels
		int nPick = RandGetNum( pGame, nNumAvailableLevels );
		return nAvailableLevels[ nPick ];
	}
	
	// return level to use (if any)
	return INVALID_ID;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddPendingTarget(
	TASK* pTask,
	SPECIES speciesTarget,
	int nTargetUniqueName)
{	
	ASSERTX_RETURN( pTask->nNumPendingTargets < MAX_TASK_UNIT_TARGETS, "Too many targets" );
	PENDING_UNIT_TARGET* pPendingUnitTarget = &pTask->tPendingUnitTargets[ pTask->nNumPendingTargets ];
	pPendingUnitTarget->speciesTarget = speciesTarget;
	pPendingUnitTarget->nTargetUniqueName = nTargetUniqueName;

	// save in template to send to client
	TASK_TEMPLATE* pTaskTemplate = &pTask->tTaskTemplate;
	pTaskTemplate->tPendingUnitTargets[ pTask->nNumPendingTargets ] = *pPendingUnitTarget;

	// have another pending target now
	pTask->nNumPendingTargets++;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskHuntOnCreate(
	GAME* pGame,
	TASK* pTask,
	DWORD dwCreateFlags)
{

	// to complete this task, all target units must be dead
	SETBIT( pTask->dwCompleteConditions, CONDITION_ALL_TARGET_UNITS_DEAD_BIT );
	//SETBIT( pTask->dwCompleteConditions, CONDITION_KILLS_BY_OWNER_BIT );

	int nLevelDepth = pTask->nLevelDepth;
		
	// get area for this kill special task
	if (TESTBIT( dwCreateFlags, TC_RESTORING_BIT ) == FALSE)
	{
		BOOL bSpawnedSpecial = FALSE;		
		LEVEL *pLevel = sGetTaskLevel( pGame, pTask );
		if (pLevel != NULL)
		{

			int nMonsterClassToSpawn = INVALID_ID;
			if( AppIsHellgate() )
			{
				nMonsterClassToSpawn = s_LevelSelectRandomSpawnableMonster( pGame, pLevel, sCanBeTaskKillTarget );
			}
			else
			{
				nMonsterClassToSpawn = s_LevelSelectRandomSpawnableQuestMonster( 
					pGame, 
					pLevel, 
					nLevelDepth,  // <-- this seems wrong to me, why not use the pLevel? -Colin
					nLevelDepth, 
					sCanBeTaskKillTarget );
			}
			
			if (nMonsterClassToSpawn != INVALID_LINK)
			{
							
				// save this in the pending units to spawn
				SPECIES speciesTarget = MAKE_SPECIES( GENUS_MONSTER, nMonsterClassToSpawn );
				int nTargetUniqueName = MonsterProperNamePick( pGame );
				sAddPendingTarget( pTask, speciesTarget, nTargetUniqueName );

				// set level area for this task to that which the target monster is in
				pTask->tTaskTemplate.nLevelDefinition = LevelGetDefinitionIndex( pLevel );
				pTask->tTaskTemplate.nLevelArea = LevelGetLevelAreaID( pLevel );
				pTask->tTaskTemplate.nLevelDepth = nLevelDepth;
				pTask->idPendingLevelSpawn = LevelGetID( pLevel );				
				pTask->nPendingLevelSpawnArea = pTask->tTaskTemplate.nLevelArea;
				pTask->nPendingLevelSpawnDepth = pTask->tTaskTemplate.nLevelDepth;

				// success
				bSpawnedSpecial = TRUE;
				
			}
						
		}
		
		if (bSpawnedSpecial == FALSE)
		{
			return FALSE;
		}
		
	}
				
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddCollect( 
	TASK* pTask,
	int nItemClass)
{

	// search for existing item class in collect of task
	TASK_COLLECT* pCollect = NULL;
	for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
	{
		TASK_COLLECT* pCollectTest = &pTask->tTaskCollect[ i ];
		if (pCollectTest->nItemClass == nItemClass)
		{
			pCollect = pCollectTest;
			break;
		}
	}
	
	// if collect not found, this is a new type of thing to collect
	if (pCollect == NULL)
	{
		ASSERTX_RETURN( pTask->nNumCollect < MAX_TASK_COLLECT_TYPES, "Adding too many items to task for collection" );
		pCollect = &pTask->tTaskCollect[ pTask->nNumCollect++ ];
		
		// init this collect bin
		pCollect->nItemClass = nItemClass;
		pCollect->nCount = 0;
		
	}
	ASSERTX_RETURN( pCollect, "Unable to find place to store what to collect for task" );

	// bump up the count of these things to collect
	pCollect->nCount++;

	// set the task template to reflect the data in the task
	MemoryCopy( &pTask->tTaskTemplate.tTaskCollect, sizeof( TASK_COLLECT ) * MAX_TASK_COLLECT_TYPES, pTask->tTaskCollect, sizeof( TASK_COLLECT ) * MAX_TASK_COLLECT_TYPES );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskForgeOnCreate(
	GAME* pGame,
	TASK* pTask,
	DWORD dwCreateFlags)
{
	ASSERTX_RETFALSE(pTask, "No task.");
	// set complete conditions
	SETBIT( pTask->dwCompleteConditions, CONDITION_COLLECTED_EVERYTHING_BIT );

	if (TESTBIT( dwCreateFlags, TC_RESTORING_BIT ) == FALSE)
	{
		const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
				
		// see which mods we will collect from the rewards
		UNITID idModsToCollect[ MAX_TASK_COLLECT_TYPES ];
		int nModsToCollectCount = 0;
		for (int i = 0; i < MAX_TASK_REWARDS; ++i)
		{
			UNITID idReward = pTask->idRewards[ i ] ;
			if (idReward != INVALID_ID)
			{
				UNIT* pReward = UnitGetById( pGame, idReward );
				ASSERTX_RETFALSE( pReward, "Unable to get forge weapon reward unit" );

				// add mods to this reward item
				ASSERTX_CONTINUE( ItemGetModSlots( pReward ) >= pTaskDefinition->nMinSlotsOnReward, "Item has not enough mod slots" );

				// find all the mods on this reward
				#define MAX_MODS_PER_ITEM (32)
				UNIT* pAvailableMods[ MAX_MODS_PER_ITEM ];
				int nNumAvailableMods = 0;
				UNIT* pItem = NULL;
				while ((pItem = UnitInventoryIterate( pReward, pItem )) != NULL)
				{
					if (UnitIsA( pItem, UNITTYPE_MOD ))
					{
						ASSERTX_CONTINUE( nNumAvailableMods < MAX_MODS_PER_ITEM - 1, "Too many mods on item" );
						pAvailableMods[ nNumAvailableMods++ ] = pItem;
					}
				}
				ASSERTX_CONTINUE( nNumAvailableMods >= pTaskDefinition->nMinSlotsOnReward, "Reward mod inventory did not populate" );
				
				// pick how many of these we must collect
				int nModsToCollectForThisItem = RandGetNum( pGame, 1, nNumAvailableMods - pTaskDefinition->nFilledSlotsOnForgeReward );
				
				// clamp to max mods to collect
				if (nModsToCollectForThisItem + nModsToCollectCount >= MAX_TASK_COLLECT_TYPES)
				{
					nModsToCollectForThisItem = MAX_TASK_COLLECT_TYPES - nModsToCollectCount;
				}
				
				// iterate item inventory and save the type of the mods to collect for this item	
				if (nModsToCollectForThisItem > 0)
				{
									
					// pick mods
					for (int nMod = 0; nMod < nModsToCollectForThisItem; ++nMod)
					{
						ASSERTX_CONTINUE( nNumAvailableMods > 0, "No available mods" );
						int nModIndex = RandGetNum( pGame, 0, nNumAvailableMods - 1 );
						UNIT* pMod = pAvailableMods[ nModIndex ];
						
						// remove mod from available mods
						pAvailableMods[ nModIndex ] = pAvailableMods[ nNumAvailableMods - 1 ];
						nNumAvailableMods--;
						
						// add mod to array of mods to collect
						idModsToCollect[ nModsToCollectCount++ ] = UnitGetId( pMod );
						
					}
										
				}
			
			}
									
		}

		// we must have *something* to collect	
		ASSERTX_RETFALSE( nModsToCollectCount > 0, "Reward items have no mods on them, therefore there is nothing to collect" );

		// setup the collect struct of the task
		for (int i = 0; i < nModsToCollectCount; ++i)
		{
			UNITID idMod = idModsToCollect[ i ];
			UNIT* pMod = UnitGetById( pGame, idMod );
			
			// add to task collect information
			sAddCollect( pTask, UnitGetClass( pMod ) );
			
			// remove these mods from the reward item (when the player brings back
			// the required mods, we'll put them in the weapon in their place)
			UnitFree( pMod, UFF_SEND_TO_CLIENTS );
			
		}

	}
				
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskQuickNeedOnCreate(
	GAME* pGame,
	TASK* pTask,
	DWORD dwCreateFlags)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	ASSERTX_RETFALSE( pTask, "Expected task" );
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	
	// set complete conditions
	SETBIT( pTask->dwCompleteConditions, CONDITION_COLLECTED_EVERYTHING_BIT );

	// what do we need to collect
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	int nTreasureClassCollect = pTaskDefinition->nTreasureClassCollect;

	// setup spawn spec
	ITEM_SPEC tItemSpec;

	// setup buffer to hold all the items generated by treasure class
	const int MAX_COLLECT_ITEMS = MAX_TASK_COLLECT_TYPES * 16;
	UNIT* pItems[ MAX_COLLECT_ITEMS ];	
	
	// setup spawn result
	ITEMSPAWN_RESULT spawnResult;
	spawnResult.pTreasureBuffer = pItems;
	spawnResult.nTreasureBufferSize = MAX_COLLECT_ITEMS;
	
	// create task rewards 
	s_TreasureSpawnClass( pPlayer, nTreasureClassCollect, &tItemSpec, &spawnResult );
	ASSERTX( spawnResult.nTreasureBufferCount > 0, "No collect items were generated for task" );

	// use these as the collect items
	for (int i = 0; i < spawnResult.nTreasureBufferCount; ++i)
	{
		UNIT* pItem = spawnResult.pTreasureBuffer[ i ];
		ASSERTX_RETFALSE( pItem, "Cannot get item from treasure spawn" );
			
		// add item to collect
		ASSERTX_CONTINUE( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
		sAddCollect( pTask, UnitGetClass( pItem ) );
		
		// destroy the item instances
		UnitFree( pItem, 0 );
		
	}
	return TRUE;

#else
	REF( pGame );
	REF( pTask );
	REF( dwCreateFlags );
	return TRUE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetExterminateGoal( 
	const TASK* pTask)
{
	ASSERT_RETZERO(pTask);
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	if (pTaskDefinition->codeExterminateCount != NULL_CODE)
	{
		int nCodeLength = 0;
		BYTE *pCode = ExcelGetScriptCode( pTask->pGame, DATATABLE_TASKS, pTaskDefinition->codeExterminateCount, &nCodeLength );
		if (pCode)
		{
			return VMExecI( pTask->pGame, pCode, nCodeLength );
		}		
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddExterminate( 
	TASK* pTask, 
	int nTargetMonsterClass, 
	int nExterminateGoal)
{

	// make species for target
	SPECIES speciesTarget = MAKE_SPECIES( GENUS_MONSTER, nTargetMonsterClass );
	
	// find free exterminate entry (or one with the same species for us to add to)
	TASK_EXTERMINATE* pExterminate = NULL;
	for( int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
	{
		TASK_EXTERMINATE* pTest = &pTask->tTaskTemplate.tExterminate[ i ];
		if (pTest->speciesTarget == SPECIES_NONE ||
			pTest->speciesTarget == speciesTarget)
		{
			pExterminate = pTest;
			break;
		}
	}
	ASSERTX_RETURN( pExterminate, "Too many exterminate entries" );
	
	// assign target and increase count
	pExterminate->speciesTarget = speciesTarget;
	pExterminate->nCount += nExterminateGoal;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskInfestationOnCreate(
	GAME* pGame,
	TASK* pTask,
	DWORD dwCreateFlags)
{
	ASSERT_RETFALSE(pTask);
	// to complete this task, the player must kill X monsters of target type
	SETBIT( pTask->dwCompleteConditions, CONDITION_KILLED_REQUIRED_NUMBER_OF_BIT );
	int nLevelDepth = pTask->nLevelDepth;		
		
	// get area for this task
	if (TESTBIT( dwCreateFlags, TC_RESTORING_BIT ) == FALSE)
	{
		LEVEL* pLevel = sGetTaskLevel( pGame, pTask );
		if (pLevel == NULL)
		{
			return FALSE;
		}
		int nArea = (AppIsHellgate())?INVALID_ID:LevelGetLevelAreaID( pLevel );
		// save target level		
		if( AppIsHellgate() )
		{
			pTask->tTaskTemplate.nLevelDefinition = LevelGetDefinitionIndex( pLevel );
		}
		else
		{
			pTask->tTaskTemplate.nLevelDepth = nLevelDepth;
			pTask->tTaskTemplate.nLevelArea = nArea;
		}		
		// how many different types of things will we exterminate
		int nNumTypes = RandGetNum( pGame, 1, MAX_TASK_EXTERMINATE );
		ASSERTX_RETFALSE( nNumTypes >= 1, "Must have at least one type of thing to exterminate" );
		
		// pick the total count of things to exterminate
		int nExterminateGoal = sGetExterminateGoal( pTask );
		
		// how many of each type will we need to exterminate
		int nExterminateGoalEachType = nExterminateGoal / nNumTypes;
		
		// pick all the types of things to exterminate
		int nTries = 32;
		int nNumSelected = 0;
		while (nNumSelected != nNumTypes && nTries-- > 0)
		{
		
			// pick a spawn class to get a target monster type from
			int nTargetMonsterClass = INVALID_ID;
			if( AppIsHellgate() )
			{
				nTargetMonsterClass = s_LevelSelectRandomSpawnableMonster( pGame, pLevel, sCanBeTaskKillTarget );
			}
			else
			{
				
				nTargetMonsterClass = s_LevelSelectRandomSpawnableQuestMonster( 
					pGame, 
					pLevel, 
					nLevelDepth, // <-- this seems wrong to me, why not use the pLevel? -Colin
					nLevelDepth, 
					sCanBeTaskKillTarget );
			}
			if (nTargetMonsterClass != INVALID_LINK)
			{
			
				// add to exterminate targets
				sAddExterminate( pTask, nTargetMonsterClass, nExterminateGoalEachType );
				nNumSelected++;
				
			}
									
		}
		
		// if couldn't select anything, that is an error
		if (nNumSelected == 0)
		{
			return FALSE;
		}
				
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetTriggerPercent( 
	const TASK* pTask)
{
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	if (pTaskDefinition->codeTriggerPercent != NULL_CODE)
	{
		int nCodeLength = 0;
		BYTE *pCode = ExcelGetScriptCode( pTask->pGame, DATATABLE_TASKS, pTaskDefinition->codeTriggerPercent, &nCodeLength );
		if (pCode)
		{
			return VMExecI( pTask->pGame, pCode, nCodeLength );
		}		
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskExploreOnCreate(
	GAME* pGame,
	TASK* pTask,
	DWORD dwCreateFlags)
{
	// to complete this task, the player must trigger all objects of a particular class
	SETBIT( pTask->dwCompleteConditions, CONDITION_TRIGGERED_ALL_BIT );

	LEVEL* pLevel = sGetTaskLevel( pGame, pTask );
	if (pLevel == NULL)
	{
		return FALSE;
	}
	if( AppIsHellgate() )
	{
		// save target level		
		pTask->tTaskTemplate.nLevelDefinition = LevelGetDefinitionIndex( pLevel );
	}
	else
	{
		int nArea = LevelGetLevelAreaID( pLevel );//sGetTaskArea( pGame, pTask );
		// save target level		
		pTask->tTaskTemplate.nLevelDepth = LevelGetDepth( pLevel );
		pTask->tTaskTemplate.nLevelArea = nArea;
	}

	pTask->pfnLevelActivate = sDoSpawnOneObjectPerRoom;

	pTask->tTaskTemplate.nTriggerPercent = sGetTriggerPercent(pTask);
	if(pTask->tTaskTemplate.nTriggerPercent <= 0)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
struct CREATE_FUNCTION_LOOKUP
{
	const char* pszName;
	PFN_CREATE_FUNCTION pfnFunction;
};

//----------------------------------------------------------------------------
static CREATE_FUNCTION_LOOKUP sgTableCreateFunction[] = 
{
	{ "TaskHuntOnCreate",			sTaskHuntOnCreate },
	{ "TaskForgeOnCreate",			sTaskForgeOnCreate },
	{ "TaskQuickNeedOnCreate",		sTaskQuickNeedOnCreate },
	{ "TaskInfestationOnCreate",	sTaskInfestationOnCreate },
	{ "TaskExploreOnCreate",		sTaskExploreOnCreate },
	{ NULL, NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_CREATE_FUNCTION sLookupCreateFunction(
	const char* pszName)
{
	for (int i = 0; sgTableCreateFunction[ i ].pfnFunction != NULL; ++i)
	{
		const CREATE_FUNCTION_LOOKUP* pLookup = &sgTableCreateFunction[ i ];
		if (PStrICmp( pLookup->pszName, pszName ) == 0)
		{
			return pLookup->pfnFunction;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TasksExcelPostProcess( 
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		TASK_DEFINITION * task_definition = (TASK_DEFINITION *)ExcelGetDataPrivate(table, ii);
		
		// set create function
		const char* pszCreateFunction = task_definition->szCreateFunction;
		if (pszCreateFunction != NULL && PStrLen( pszCreateFunction ) > 0)
		{
			task_definition->pfnOnCreate = sLookupCreateFunction(pszCreateFunction);
		}
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const TASK_DEFINITION* TaskDefinitionGet(	
	int nTaskIndex)
{
	return (const TASK_DEFINITION*)ExcelGetData( NULL, DATATABLE_TASKS, nTaskIndex );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const TASK_DEFINITION* TaskDefinitionGetByCode( 	
	WORD wCode)
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_TASKS );
	for (int i = 0; i < nNumRows; ++i)
	{
		const TASK_DEFINITION* pTaskDefinition = TaskDefinitionGet( i );
		if (pTaskDefinition->wCode == wCode)
		{
			return pTaskDefinition;
		}
	}
	return NULL;  // not found
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetGeneratedTasks( 
	GAME* pGame, 
	UNIT* pPlayer, 
	UNIT* pGiver, 
	TASK* pGeneratedTasks[], 
	int nBufferSize)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	int nNumTasks = 0;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
	
		// if this task belongs to the player and was generated from the NPC in question add it
		if (pTask->idPlayer == UnitGetId( pPlayer ) &&
			pTask->speciesGiver == UnitGetSpecies( pGiver ) &&
			pTask->eStatus == TASK_STATUS_GENERATED)
		{
			ASSERTX( nNumTasks < nBufferSize, "Buffer size too small" );
			pGeneratedTasks[ nNumTasks++ ] = pTask;			
		}
		
	}

	return nNumTasks;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetActiveTasks( 
	GAME* pGame, 
	UNIT* pPlayer, 
	UNIT* pGiver, 
	TASK* pActiveTasks[], 
	int nBufferSize)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	int nNumTasks = 0;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
	
		// if this task belongs to the player and was generated from the NPC in question add it
		if (pTask->idPlayer == UnitGetId( pPlayer ) &&
			pTask->speciesGiver == UnitGetSpecies( pGiver ) &&
			pTask->eStatus == TASK_STATUS_ACTIVE)
		{
			ASSERTX( nNumTasks < nBufferSize, "Buffer size too small" );
			if( pActiveTasks )
			{
				pActiveTasks[ nNumTasks++ ] = pTask;			
			}
			else
			{
				nNumTasks++;
			}
		}
		
	}

	return nNumTasks;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetActiveTasks( 
	GAME* pGame, 
	UNIT* pPlayer )
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	int nNumTasks = 0;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
	
		// if this task belongs to the player and was generated from the NPC in question add it
		if (pTask->idPlayer == UnitGetId( pPlayer ) &&
			( pTask->eStatus == TASK_STATUS_ACTIVE ||
			  pTask->eStatus == TASK_STATUS_COMPLETE ) )
		{
			nNumTasks++;

		}
		
	}
	return nNumTasks;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sExpireOldTasks(
	GAME* pGame,
	UNIT* pPlayer, 
	UNIT* pGiver, 
	TASK* pTasks[], 
	int nNumTasks)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	DWORD dwNow = GameGetTick( pGame );
	int nTooToldTime = AppIsHellgate() ? TASK_TOO_OLD_TIME : TASK_TOO_OLD_TIME_TUGBOAT;
		
	// go through tasks and expire any old ones that were never accepted
	for (int i = 0; i < nNumTasks; ++i)
	{
		TASK* pTask = pTasks[ i ];
		int nElapsedTicks = dwNow - pTask->dwGeneratedTick;
		if (nElapsedTicks >= nTooToldTime)
		{
		
			// free this task
			sTaskFree( pGame, pTaskGlobals, pTask );
			
			// move all other tasks in the array up one
			for (int j = i; j < nNumTasks; ++j)
			{
				if (j + 1 < nNumTasks)
				{
					pTasks[ j ] = pTasks[ j + 1 ];
				}
			}
			nNumTasks--;
			
		}
		
	}
	
	// return new number of tasks
	return nNumTasks;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSaveSimilarCompleted(
	TASK *pTask)
{			
	const TASK_TEMPLATE *ptTemplate = &pTask->tTaskTemplate;
	
	// save level/task pairs
	if (ptTemplate->nLevelDefinition != INVALID_LINK)
	{
		UNIT *pPlayer = UnitGetById( pTask->pGame, pTask->idPlayer );
		
		UnitSetStat( 
			pPlayer, 
			STATS_TASK_ON_LEVEL_COMPLETE, 
			ptTemplate->nLevelDefinition,
			pTask->nTaskDefinition,
			1);
			
	}

	// more here someday maybe
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsSimilarCompleted(
	UNIT *pPlayer,
	TASK *pTask)
{
	const TASK_TEMPLATE *ptTemplate = &pTask->tTaskTemplate;

	// check level/task pairs
	BOOL bDone = UnitGetStat( 
		pPlayer,
		STATS_TASK_ON_LEVEL_COMPLETE,
		ptTemplate->nLevelDefinition,
		pTask->nTaskDefinition);
	return bDone == TRUE;
	
	// more here someday maybe
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskFilterSimilar( 
	UNIT *pPlayer, 
	TASK *pTask)
{
	const TASK_DEFINITION *ptTaskDef = pTask->pTaskDefinition;
	
	if (ptTaskDef->bDoNotOfferSimilarTask == TRUE)
	{
		if (sIsSimilarCompleted( pPlayer, pTask ))
		{
			return FALSE;  // similar task already has been done by this player
		}				
	}
	
	return TRUE;  // all OK
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static TASK* sCreateNewTask( 
	GAME* pGame, 
	UNIT* pPlayer, 
	SPECIES speciesGiver,
	int nFactionPrimary,
	const TASK_DEFINITION* pTaskDefinition,
	DWORD dwCreateFlags)
{
	ASSERTX_RETNULL( pTaskDefinition, "No task definition.");
	ASSERTX_RETNULL( pTaskDefinition->pfnOnCreate, "No create function for task" );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
		
	// create a new task
	TASK* pTask = sNewTask( pGame, pTaskDefinition, pPlayer, speciesGiver, nFactionPrimary, dwCreateFlags );
	
	// run task creation	 
	if (pTask)
	{
	
		// run create function
		BOOL bSuccess = pTaskDefinition->pfnOnCreate( pGame, pTask, dwCreateFlags );
		
		// some tasks are valid tasks, but are invalid for this player
		if (bSuccess)
		{
			bSuccess = sTaskFilterSimilar( pPlayer, pTask );
		}
		
		// if we failed the creation function, delete task
		if (bSuccess == FALSE)
		{
			sTaskFree( pGame, pTaskGlobals, pTask );
			pTask = NULL;
		}
		
	}
	
	// return the newly created task (if any)
	return pTask;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
int s_TaskGetAvailableTasks( 
	GAME* pGame,
	UNIT* pPlayer,
	UNIT* pGiver, 
	TASK_TEMPLATE* pAvailableTasks, 
	int nBufferSize)
{
	ASSERTX( pAvailableTasks, "Invalid params for available tasks" );
		
	// previously generated tasks at this giver
	TASK* pGeneratedTasks[ MAX_AVAILABLE_TASKS_AT_GIVER ];
	int nNumGeneratedTasks = sGetGeneratedTasks( pGame, pPlayer, pGiver, pGeneratedTasks, MAX_AVAILABLE_TASKS_AT_GIVER );
	
	// see if it's time to expire any old tasks generated
	nNumGeneratedTasks = sExpireOldTasks( pGame, pPlayer, pGiver, pGeneratedTasks, nNumGeneratedTasks );	

	// add the existing tasks to the available task array
	int nNumTasks = 0;
	for (int i = 0; i < nNumGeneratedTasks; ++i)
	{
		const TASK* pTask = pGeneratedTasks[ i ];
		ASSERTX( nNumTasks < nBufferSize, "Available task buffer size too small" );
		pAvailableTasks[ nNumTasks++ ] = pTask->tTaskTemplate;
	}
	
	// when did this task giver last generate tasks for this player
	int nClassGiver = UnitGetClass( pGiver );
	DWORD dwTickLastGenerate = UnitGetStat( pPlayer, STATS_TASKS_GENERATED_TICK, nClassGiver );

	// has enough time passed inbetween task generations to make some more
	int nTooOldTime = AppIsHellgate() ? MIN_TIME_BETWEEN_TASK_GENERATIONS : MIN_TIME_BETWEEN_TASK_GENERATIONS_TUGBOAT;
	DWORD dwTickCurrent = GameGetTick( pGame );
	int nElapsedTime = dwTickCurrent - dwTickLastGenerate;
	if (gbGenerateTimeDelay == FALSE || dwTickLastGenerate == 0 || nElapsedTime > nTooOldTime)
	{
	
		// generate more tasks up to the max allowed
		BOOL bGeneratedNewTask = FALSE;
		if (nNumGeneratedTasks < MAX_AVAILABLE_TASKS_AT_GIVER)
		{
			
			// start the picker
			PickerStart( pGame, picker );

			// get available task list
			int nNumTaskTypes = ExcelGetNumRows( pGame, DATATABLE_TASKS );
			for (int i = 0; i < nNumTaskTypes; ++i)
			{
				const TASK_DEFINITION* pTaskDefinition = TaskDefinitionGet( i );
				ASSERTX_CONTINUE( pTaskDefinition, "Invalid task definition" );
				
				// TODO cmarch: I am going to diable tasks here temporarily for Hellgate
				//				until I migrate them all to the QUEST system.
				if (AppIsHellgate())
				{
					continue;
				}

				// skip ones that are not implemnted (remove this later)
				if (pTaskDefinition->bImplemented == FALSE)
				{
					continue;
				}

				// add to picker
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pTaskDefinition->nRarity));
						
			}
						
			// generate tasks
			for (int i = nNumGeneratedTasks; i < MAX_AVAILABLE_TASKS_AT_GIVER; ++i)
			{
				if( AppIsHellgate() )
				{
					int nNumTries = 16;
					while (nNumTries--)
					{
					
						// get picked
						int nIndex = PickerChoose( pGame );
						
						// create particulars of task and record for caller
						if (nIndex != INVALID_INDEX)
						{
							const TASK_DEFINITION* pTaskDefinition = TaskDefinitionGet( nIndex );
							ASSERTX( pTaskDefinition, "Expected task definition" );
							
							// generate new task, do not activate
							SPECIES speciesGiver = UnitGetSpecies( pGiver );
							int nFactionPrimary = UnitGetPrimaryFaction( pGiver );
							TASK* pTask = sCreateNewTask( 
								pGame, 
								pPlayer, 
								speciesGiver, 
								nFactionPrimary, 
								pTaskDefinition, 
								0 );
								
							if (pTask)
							{
						
								// add template to available tasks
								ASSERTX( nNumTasks < nBufferSize, "Available task buffer size too small" );
								pAvailableTasks[ nNumTasks++ ] = pTask->tTaskTemplate;
								bGeneratedNewTask = TRUE;
								break;  // exit while
											
							}
											
						}
	
					}
				}
				else
				{
					// get picked
					int nIndex = PickerChoose( pGame );
					
					// create particulars of task and record for caller
					if (nIndex != INVALID_INDEX)
					{
						const TASK_DEFINITION* pTaskDefinition = TaskDefinitionGet( nIndex );
						ASSERTX( pTaskDefinition, "Expected task definition" );
						
						// generate new task, do not activate
						SPECIES speciesGiver = UnitGetSpecies( pGiver );
						int nFactionPrimary = UnitGetPrimaryFaction( pGiver );
						TASK* pTask = sCreateNewTask( 
							pGame, 
							pPlayer, 
							speciesGiver, 
							nFactionPrimary, 
							pTaskDefinition, 
							0 );
							
						if (pTask)
						{
					
							// add template to available tasks
							ASSERTX( nNumTasks < nBufferSize, "Available task buffer size too small" );
							pAvailableTasks[ nNumTasks++ ] = pTask->tTaskTemplate;
							bGeneratedNewTask = TRUE;
						}
						else
						{
							ASSERT_MSG("Task was not created");
						}
					}
					else
					{
							//something broke
						ASSERT_MSG("Task was -1"); 
					}

				}
								
			}
			
		}
	
		// if we generated new tasks, record when they were generated
		if (bGeneratedNewTask == TRUE)
		{
			UnitSetStat( pPlayer, STATS_TASKS_GENERATED_TICK, nClassGiver, dwTickCurrent );		
		}
		
	}
						
	// return number of tasks
	return nNumTasks;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TaskSendAvailableTasksToClient( 
	GAME* pGame,
	GAMECLIENTID idClient, 
	const TASK_TEMPLATE* pAvailableTasks, 
	const int nAvailableTaskCount,
	UNIT *pTaskGiver)
{
	
	// init message
	MSG_SCMD_AVAILABLE_TASKS tMessage;
	for (int i = 0; i < MAX_AVAILABLE_TASKS_AT_GIVER; ++i)
	{
		TASK_TEMPLATE* pTaskTemplate = &tMessage.tTaskTemplate[ i ];
		TaskTemplateInit( pTaskTemplate );
	}
	
	// set each task
	for (int i = 0; i < nAvailableTaskCount; ++i)
	{
		const TASK_TEMPLATE* pTaskTemplate = &pAvailableTasks[ i ];		
		tMessage.tTaskTemplate[ i ] = *pTaskTemplate;
	}

	// set giver
	tMessage.idTaskGiver = UnitGetId( pTaskGiver );
	
	// send it
	s_SendMessage( pGame, idClient, SCMD_AVAILABLE_TASKS, &tMessage );	
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskIsSavable(
	const TASK* pTask)
{	
	ASSERTX_RETFALSE( pTask, "Expected task" );
	const TASK_DEFINITION* pTaskDefinition = pTask->pTaskDefinition;
	
	// must be savable by definition
	if (pTaskDefinition->bCanSave == FALSE)
	{
		return FALSE;
	}
		
	// task must be live
	//NOTE: Looked in the revision history and appears to have been changed for quite sometime
	//so I believe Travis made this change on purpose.
	if ( ( AppIsHellgate() &&
		 ( pTask->eStatus == TASK_STATUS_INVALID || 
		   pTask->eStatus == TASK_STATUS_GENERATED ) ) ||
		 ( AppIsTugboat() &&
		   pTask->eStatus != TASK_STATUS_ACTIVE &&
	       pTask->eStatus != TASK_STATUS_COMPLETE &&
		   pTask->eStatus != TASK_STATUS_FAILED ) )
	{
		return FALSE;
	}

	return TRUE;
			 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_TaskCanBeSaved(	
	GAME* pGame,
	GAMETASKID idTask)
{
	const TASK* pTask = sTaskFind( pGame, idTask, NULL );
	if (pTask)
	{
		return sTaskIsSavable( pTask );
	}
	return FALSE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAnyTargetUnitDead(
	TASK* pTask)
{
	for (int i = 0; i < pTask->nNumUnitTargets; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		if (pUnitTarget->eStatus == TASK_UNIT_STATUS_DEAD)
		{
			return TRUE;
		}
	}
	return FALSE;  // no target is dead
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAnyTargetsKilledBy( 
	TASK* pTask, 
	UNITID idKiller)
{
	for (int i = 0; i < pTask->nNumUnitTargets; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		if (pUnitTarget->eStatus == TASK_UNIT_STATUS_DEAD &&
			pUnitTarget->idKiller == idKiller)
		{
			return TRUE;
		}		
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskSendStatus(
	GAME* pGame,
	TASK* pTask)
{

	MSG_SCMD_TASK_STATUS message;
	message.idTask = pTask->idTask;
	message.eStatus = pTask->eStatus;

	// send it
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	s_SendMessage( pGame, UnitGetClientId(pPlayer), SCMD_TASK_STATUS, &message );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskFailed( 
	GAME* pGame,
	TASK* pTask)
{

	// do nothing if failed already
	if (pTask->eStatus == TASK_STATUS_FAILED)
	{
		return;
	}
	
	// mark task as completed
	ASSERTX( pTask->eStatus == TASK_STATUS_ACTIVE || pTask->eStatus == TASK_STATUS_COMPLETE, "Task must be a live task" );	
	pTask->eStatus = TASK_STATUS_FAILED;
	
	// send to client
	sTaskSendStatus( pGame, pTask );	

}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskCompleted( 
	GAME* pGame,
	TASK* pTask)
{

	// mark task as completed (it must be active to do this)
	if (pTask->eStatus == TASK_STATUS_ACTIVE)
	{
	
		// set new status
		pTask->eStatus = TASK_STATUS_COMPLETE;
		
		// send new status to client
		sTaskSendStatus( pGame, pTask );
		if( AppIsHellgate() )
		{
			// some tasks save that they were done on a particular level
			const TASK_DEFINITION *ptTaskDef = pTask->pTaskDefinition;
			if (ptTaskDef->bDoNotOfferSimilarTask)
			{
				sSaveSimilarCompleted( pTask );			
			}
		}
				
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskReActivate(
	GAME* pGame,
	TASK* pTask)
{

	// only completed tasks can be shifted back into an active state
	if (pTask->eStatus == TASK_STATUS_COMPLETE)
	{
	
		// set new status
		pTask->eStatus = TASK_STATUS_ACTIVE;
		
		// send new status to client
		sTaskSendStatus( pGame, pTask );
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAllTargetUnitsDead(
	TASK* pTask)
{

	// if there are pending spawns this cannot succeed
	if (pTask->nNumPendingTargets > 0)
	{
		return FALSE;
	}
	
	for (int i = 0; i < pTask->nNumUnitTargets; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		if (pUnitTarget->eStatus != TASK_UNIT_STATUS_DEAD)
		{
			return FALSE;
		}
	}
	return TRUE;  // all targets daed
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAllTargetsKilledBy( 
	TASK* pTask, 
	UNITID idKiller)
{

	// if there are pending spawns this cannot succeed
	if (pTask->nNumPendingTargets > 0)
	{
		return FALSE;
	}

	for (int i = 0; i < pTask->nNumUnitTargets; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		if (pUnitTarget->eStatus != TASK_UNIT_STATUS_DEAD ||
			pUnitTarget->idKiller != idKiller)
		{
			return FALSE;
		}		
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAllTaskMarkersTriggered(
	TASK * pTask)
{
	if(pTask->idPendingLevelSpawn == INVALID_LINK)
	{
		return FALSE;
	}

	LEVEL * pLevel = LevelGetByID(pTask->pGame, pTask->idPendingLevelSpawn);
	ROOM * pRoom = LevelGetFirstRoom(pLevel);
	int nIncompleteCount = 0;
	while(pRoom)
	{
		for (UNIT *pObject = pRoom->tUnitList.ppFirst[ TARGET_OBJECT ]; pObject; pObject = pObject->tRoomNode.pNext)
		{
			if(UnitGetClass(pObject) == pTask->pTaskDefinition->nObjectClass)
			{
				UNITID idTriggerer = UnitGetStat(pObject, STATS_TRIGGER_UNIT_ID);
				if(pTask->idPlayer == idTriggerer)
				{
					nIncompleteCount++;
				}
			}
		}
		pRoom = LevelGetNextRoom(pRoom);
	}

	int nPercentComplete = 100 * (pTask->nTriggerCount - nIncompleteCount) / pTask->nTriggerCount;
	if(nPercentComplete < pTask->tTaskTemplate.nTriggerPercent)
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckTaskComplete(
	GAME* pGame,
	TASK* pTask)
{

	// a failed task can never be completed
	if( pTask->eStatus == TASK_STATUS_FAILED)
	{
		return;
	}
			
	// check all completion conditions

	// check target conditions
	if (pTask->eStatus == TASK_STATUS_ACTIVE)
	{
				
		// check any target dead
		if (TESTBIT( pTask->dwCompleteConditions, CONDITION_ANY_TARGET_UNIT_DEAD_BIT ))
		{
			if (sTaskAnyTargetUnitDead( pTask ) == TRUE)
			{
				if (TESTBIT( pTask->dwCompleteConditions, CONDITION_KILLS_BY_OWNER_BIT ))
				{
					if (sTaskAnyTargetsKilledBy( pTask, pTask->idPlayer ) == FALSE)
					{
						sTaskFailed( pGame, pTask );
						return;
					}
				}
			}
			else
			{
				return;
			}
		}

		// check all targets dead
		if (TESTBIT( pTask->dwCompleteConditions, CONDITION_ALL_TARGET_UNITS_DEAD_BIT ))
		{
			if (sTaskAllTargetUnitsDead( pTask ) == TRUE)
			{
				if (TESTBIT( pTask->dwCompleteConditions, CONDITION_KILLS_BY_OWNER_BIT ))
				{
					if (sTaskAllTargetsKilledBy( pTask, pTask->idPlayer ) == FALSE)
					{
						sTaskFailed( pGame, pTask );
						return;
					}					
				}
			}
			else
			{
				if (sTaskAnyTargetUnitDead( pTask ) == TRUE)
				{
					if (TESTBIT( pTask->dwCompleteConditions, CONDITION_KILLS_BY_OWNER_BIT ))
					{
						// some targets are dead, they must be killed by owner
						if (sTaskAllTargetsKilledBy( pTask, pTask->idPlayer ) == FALSE)
						{
							sTaskFailed( pGame, pTask );
							return;
						}
					}			
				}
				return; // all targets not dead
			}
		}

		// check extermination complete conditions
		if (TESTBIT( pTask->dwCompleteConditions, CONDITION_KILLED_REQUIRED_NUMBER_OF_BIT ))
		{
			
			// what is the desired extermination count ... note that we are using a goal count
			// of all the separate types all added together, this means you don't have to actually
			// kill X# of Y creatures and A# of B creatures, you just have to 
			// kill X# + A# of Y OR B creatures.
			int nExterminateGoal = TaskTemplateGetExterminateGoal( &pTask->tTaskTemplate );
			if (pTask->tTaskTemplate.nExterminatedCount < nExterminateGoal)
			{
				return;
			}
			
		}
		
		// check all objects of specific type on target level triggered
		if(TESTBIT( pTask->dwCompleteConditions, CONDITION_TRIGGERED_ALL_BIT ))
		{
			if(!sTaskAllTaskMarkersTriggered(pTask))
			{
				return;
			}
		}
	}
	
	// check collect conditions (this is done in both active and completed task states)
	if (TESTBIT( pTask->dwCompleteConditions, CONDITION_COLLECTED_EVERYTHING_BIT ))
	{
		UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
		BOOL bHasEverything = TRUE;
		for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
		{	
			TASK_COLLECT* pCollect = &pTask->tTaskCollect[ i ];
			if (pCollect->nCount > 0)
			{
			
				// what are we looking for
				SPECIES spItem = MAKE_SPECIES( GENUS_ITEM, pCollect->nItemClass );
				
				// it must actually be on their person
				DWORD dwFlags = 0;
				SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
				SETBIT( dwFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );
				
				int nCount = UnitInventoryCountUnitsOfType( pPlayer, spItem, dwFlags );
				if (nCount < pCollect->nCount)
				{
					bHasEverything = FALSE;
					break;
				}
			}
		}

		// if don't have everything, the task is active instead of complete
		if (bHasEverything == FALSE )
		{
			if (pTask->eStatus == TASK_STATUS_COMPLETE)
			{
				sTaskReActivate( pGame, pTask );
			}			
			return;			
		}
		
	}
	
	// task is complete
	sTaskCompleted( pGame, pTask );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendExterminatedCount( 
	TASK* pTask)
{
	ASSERTX_RETURN( pTask, "Expected task" );
	
	// setup message
	MSG_SCMD_TASK_EXTERMINATED_COUNT tMessage;
	tMessage.idTask = pTask->idTask;
	tMessage.nCount = pTask->tTaskTemplate.nExterminatedCount;
	
	// send
	GAME* pGame = pTask->pGame;
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TASK_EXTERMINATED_COUNT, &tMessage );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoMonsterKill( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_MONSTER_KILL* pMessage)
{
	UNITID idKiller = pMessage->idKiller;
	UNITID idVictim = pMessage->idVictim;
	UNIT* pVictim = UnitGetById( pGame, idVictim );
	SPECIES speciesVictim = UnitGetSpecies( pVictim );
	LEVEL* pLevelVictim = UnitGetLevel( pVictim );
	
	// is this victim a target of the task
	BOOL bKilledTarget = TRUE;
	for (int i = 0; i < pTask->nNumUnitTargets; ++i)
	{
		TASK_UNIT_TARGET* pUnitTarget = &pTask->unitTargets[ i ];
		
		if (pUnitTarget->idTarget == idVictim)
		{
			ASSERT( pUnitTarget->eStatus != TASK_UNIT_STATUS_DEAD );
			pUnitTarget->eStatus = TASK_UNIT_STATUS_DEAD;
			pUnitTarget->idKiller = idKiller;
			bKilledTarget = TRUE;
		}
		
	}

	// check for eliminating any of the extermination targets on the level of the infestation
	TASK_TEMPLATE* pTaskTemplate = &pTask->tTaskTemplate;
	int nExterminationGoal = TaskTemplateGetExterminateGoal( pTaskTemplate );
	if (pTaskTemplate->nExterminatedCount < nExterminationGoal)
	{
		// TRAVIS: for now, let's not check the depth. just kill 'em anywhere!
		// we aren't spawning the group on a specific level yet anyway...
		int nLevelDefinitionVictim = LevelGetDefinitionIndex( pLevelVictim );
		REF(nLevelDefinitionVictim);
		if ( AppIsTugboat() ||
			 nLevelDefinitionVictim == pTask->tTaskTemplate.nLevelDefinition )
		{
			BOOL bDidExtermination = FALSE;
			for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
			{
				TASK_EXTERMINATE* pExterminate = &pTask->tTaskTemplate.tExterminate[ i ];
				if (pExterminate->speciesTarget == speciesVictim)
				{
					pTask->tTaskTemplate.nExterminatedCount++;
					bDidExtermination = TRUE;
				}
			}
			
			// if we exterminated something, send the updated count to client
			if (bDidExtermination == TRUE)
			{
				sSendExterminatedCount( pTask );
			}
			
		}

	}
			
	// see if task is complete
	if (bKilledTarget == TRUE)
	{
		sCheckTaskComplete( pGame, pTask );
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskClose( 
	GAME* pGame,
	TASK* pTask)
{
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	ASSERT( pPlayer );
	
	// tell client to remove this task from their UI
	MSG_SCMD_TASK_CLOSE message;
	message.idTask = pTask->idTask;

	// send it
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TASK_CLOSE, &message );	

	// mark player has not taken any items from the reward for a task of this type
	UnitClearStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask );
	
	// remove the task from the system
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );	
	sTaskFree( pGame, pTaskGlobals, pTask );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSTaskForceClose(
	GAME* pGame,
	GAMETASKID idTask)
{
	TASK* pTask = sTaskFind( pGame, idTask, NULL );
	if (pTask)
	{
		UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
		if (pPlayer)
		{
			sTaskClose( pGame, pTask );
		}
				
	}
	
}

//----------------------------------------------------------------------------
struct SPAWN_MONSTER_CALLBACK_DATA
{
	TASK* pTask;
	PENDING_UNIT_TARGET* pPendingUnitTarget;
	int nUnitTargetIndex;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPendingMonsterTargetUnitSpawnedCallback(
	UNIT* pMonster,
	void* pCallbackData)
{
	GAME *pGame = UnitGetGame( pMonster );
	
	// only the only targets that count are the unique named ones
	int nQuality = UnitGetStat( pMonster, STATS_MONSTERS_QUALITY );
	if (nQuality == MonsterQualityPick( pGame, MQT_TOP_CHAMPION ))
	{
		SPAWN_MONSTER_CALLBACK_DATA	*pSpawnMonsterCallbackData = (SPAWN_MONSTER_CALLBACK_DATA*)pCallbackData;
		TASK* pTask = pSpawnMonsterCallbackData->pTask;
		PENDING_UNIT_TARGET* pPendingUnitTarget = pSpawnMonsterCallbackData->pPendingUnitTarget;
		ASSERTX_RETURN( (GENUS)GET_SPECIES_GENUS( pPendingUnitTarget->speciesTarget ) == GENUS_MONSTER, "Expected monster for pending unit target" );
		int nUnitTargetIndex = pSpawnMonsterCallbackData->nUnitTargetIndex;
		
		// what is the unit target index for this spawn (note that a spawn can cause more than
		// just the single target monster to be	spawned cause they come in packs)
		TASK_UNIT_TARGET* pTaskUnitTarget = &pTask->unitTargets[ nUnitTargetIndex ];

		// if there is not already a target here, then this is now the target		
		if (pTaskUnitTarget->idTarget == INVALID_ID)
		{
		
			// set as target
			pTaskUnitTarget->idTarget = UnitGetId( pMonster );
			pTaskUnitTarget->eStatus = TASK_UNIT_STATUS_ALIVE;

			// set name on monster if specified
			if (pPendingUnitTarget->nTargetUniqueName != MONSTER_UNIQUE_NAME_INVALID)
			{
				MonsterProperNameSet( pMonster, pPendingUnitTarget->nTargetUniqueName, TRUE );
			}
			
			// task now has an additional target
			pTask->nNumUnitTargets++;
			
		}

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoPendingRoomSpawns(
	GAME* pGame,
	TASK* pTask,
	ROOM* pRoom)
{

	if (pTask->idPendingRoomSpawn == RoomGetId( pRoom ))
	{

		// setup callback data
		SPAWN_MONSTER_CALLBACK_DATA callbackData;
		callbackData.pTask = pTask;
		callbackData.pPendingUnitTarget = NULL;
		callbackData.nUnitTargetIndex = -1;
		
		// go through any pending units to create
		BOOL bSpawnedEverything = TRUE;
		for (int i = 0; i < pTask->nNumPendingTargets; ++i)
		{
			PENDING_UNIT_TARGET* pPendingTargetUnit = &pTask->tPendingUnitTargets[ i ];

			// skip this unit if its instance is already dead (this will happen when we restore a task)
			TASK_UNIT_TARGET* pTaskUnitTarget = &pTask->unitTargets[ i ];
			if (pTaskUnitTarget->eStatus == TASK_UNIT_STATUS_DEAD)
			{
				// task now has an additional target even tho its already dead
				ASSERT( pTask->nNumUnitTargets == i );
				pTask->nNumUnitTargets++;
				continue;
			}			
			
			// update pending target index for callback data
			callbackData.pPendingUnitTarget = pPendingTargetUnit;

			// when this target is instanced, where will we track the target unit
			callbackData.nUnitTargetIndex = i; 
			
			// pick a max quality for this unique
			int nMonsterQuality = MonsterQualityPick( pGame, MQT_TOP_CHAMPION );
			int nTries = (AppIsHellgate())?32:256;
			int nNumSpawned = 0;
			SPAWN_LOCATION tSpawnLocation;
			while (nNumSpawned == 0 && nTries-- > 0)
			{
				if( AppIsTugboat() )
				{
					VECTOR Position;
					Position.fX = pRoom->pHull->aabb.center.fX + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fX, pRoom->pHull->aabb.halfwidth.fX );
					Position.fY = pRoom->pHull->aabb.center.fY + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fY, pRoom->pHull->aabb.halfwidth.fY );
					Position.fZ = .1f;
					pRoom = RoomGetFromPosition( pGame, pRoom, &Position );
					ROOM_PATH_NODE * pPathNode = RoomGetNearestFreePathNode( pGame, RoomGetLevel(pRoom), Position, &pRoom, NULL );
					if( !pPathNode )
					{
						continue;
					}
					Position = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);

					pRoom = RoomGetFromPosition( pGame, pRoom, &Position );					
					tSpawnLocation.vSpawnPosition = Position;
					tSpawnLocation.vSpawnUp = VECTOR( 0, 0, 1 );
					tSpawnLocation.vSpawnDirection = VECTOR( 1, 0, 0 );
					tSpawnLocation.fRadius = 0.0f;
				}
				else
				{	
					// pick a random spawn point (change this to use nodes maybe)
					int nSpawnPointIndex = RandGetNum( pGame, pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].nCount );
					ASSERTX( nSpawnPointIndex >= 0 && nSpawnPointIndex < pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].nCount, "Invalid spawn point picked" );
					ROOM_LAYOUT_GROUP* pSpawnPoint = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].pGroups[ nSpawnPointIndex ];
					ROOM_LAYOUT_SELECTION_ORIENTATION* pSpawnOrientation = &pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].pOrientations[ nSpawnPointIndex ];
					SpawnLocationInit( &tSpawnLocation, pRoom, pSpawnPoint, pSpawnOrientation );
				}
				
				// create a spawn entry just for this single target
				SPAWN_ENTRY tSpawn;
				tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
				tSpawn.nIndex = GET_SPECIES_CLASS( pPendingTargetUnit->speciesTarget );
				tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;
			
				// spawn monster
				ASSERTX( (GENUS)GET_SPECIES_GENUS( pPendingTargetUnit->speciesTarget ) == GENUS_MONSTER, "Expected pending unit target to be a monster" );
				nNumSpawned = s_RoomSpawnMonsterByMonsterClass( 
					pGame, 
					&tSpawn, 
					nMonsterQuality, 
					pRoom, 
					&tSpawnLocation,
					sPendingMonsterTargetUnitSpawnedCallback,
					(void*)&callbackData);
								
			}
			ASSERTX( nNumSpawned >= 1, "Unable to spawn task unit" );
					
		}
		
		// if nothing was spawned signal error
		if (bSpawnedEverything == FALSE && pTask->nNumPendingTargets > 0)
		{
			sSTaskForceClose( pGame, pTask->idTask );
		}
		else
		{
		
			// there are no pending target units now
			pTask->nNumPendingTargets = 0;

			// this task no longer has a room to do pending spawning in
			pTask->idPendingRoomSpawn = INVALID_ID;

			//ConsoleString( 
			//	CONSOLE_SYSTEM_COLOR, 
			//	"Task special spawned in '%s' on '%s'",
			//	pRoom->pDefinition->tHeader.pszName,
			//	RoomGetLevel( pRoom )->m_szLevelName);

		}
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoPendingLevelSpawns(
	GAME* pGame,
	TASK* pTask,
	LEVEL* pLevel)
{

	// does task have a pending level definition for spawns but no level instance,
	// in that case we assume that any level of the same definition type will do
	if (AppIsHellgate() &&
		pTask->nPendingLevelSpawnDefinition != INVALID_LINK &&
		pTask->nPendingLevelSpawnDefinition == LevelGetDefinitionIndex( pLevel ))
	{
		pTask->nPendingLevelSpawnDefinition = INVALID_LINK;
		pTask->idPendingLevelSpawn = LevelGetID( pLevel );
	}
	
	// do we have any pending spawning to do in this level
	if ( ( AppIsHellgate() && 
		   LevelGetID( pLevel ) == pTask->idPendingLevelSpawn ) ||
		 ( AppIsTugboat() && 
		   LevelGetDepth( pLevel ) == pTask->nPendingLevelSpawnDepth &&
		   LevelGetLevelAreaID( pLevel ) == pTask->nPendingLevelSpawnArea ) )
	{
		ASSERT_RETURN( LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ) );

		// pick a room in the level to do spawning in
		ROOM* pRoomSpawn = sGetSpawnRoom( pGame, pLevel, pTask, NULL );
		if (pRoomSpawn == NULL)
		{
			ASSERTX( 0, "Unable to pick room for pending spawn in level" );
			sSTaskForceClose( pGame, pTask->idTask );
			return;
		}
		
		// save this room as having work for us to do when it becomes active
		pTask->idPendingRoomSpawn = RoomGetId( pRoomSpawn );
		pTask->nPendingLevelSpawnDepth = (AppIsTugboat())?-1:pTask->nPendingLevelSpawnDepth;

		// no more pending level spawn for this task
		pTask->idPendingLevelSpawn = INVALID_ID;
		
		// if the room is already active, run the active logic now
		if (RoomIsActive(pRoomSpawn))
		{
			sDoPendingRoomSpawns( pGame, pTask, pRoomSpawn );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoSpawnOneObjectPerRoom(
	GAME * pGame,
	TASK * pTask,
	LEVEL * pLevel)
{
	ASSERT_RETURN( LevelTestFlag(pLevel, LEVEL_POPULATED_BIT) );
	if(pTask->pTaskDefinition->nObjectClass == INVALID_LINK)
	{
		return;
	}

	pTask->nTriggerCount = 0;
	ROOM * pRoom = LevelGetFirstRoom(pLevel);
	while(pRoom)
	{
		ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoom);
		if(pPathNodeSet->nPathNodeCount > 0)
		{
			
			VECTOR vWorldPosition = (AppIsHellgate())?RoomPathNodeGetExactWorldPosition(pGame, pRoom, RoomGetRoomPathNode(pRoom, 0)):RoomPathNodeGetWorldPosition( pGame, pRoom, RoomGetRoomPathNode(pRoom, 0) );
			
			OBJECT_SPEC tSpec;
			tSpec.nClass = pTask->pTaskDefinition->nObjectClass;
			tSpec.nLevel = 0;
			tSpec.pRoom = pRoom;
			tSpec.vPosition = vWorldPosition;
			tSpec.pvFaceDirection = &cgvYAxis;
			UNIT * pObject = s_ObjectSpawn( pGame, tSpec );
			if(pObject)
			{
				pTask->nTriggerCount++;
				UnitSetStat(pObject, STATS_TRIGGER_UNIT_ID, pTask->idPlayer);
			}
		}
		pRoom = LevelGetNextRoom(pRoom);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoLevelActivated( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_LEVEL_ACTIVATE* pMessage)
{
	LEVEL* pLevel = pMessage->pLevel;
	
	// do any pending level spawns
	sDoPendingLevelSpawns( pGame, pTask, pLevel );

	if(pTask->pfnLevelActivate)
	{
		// do we have any pending operations to do in this level
		if ( ( AppIsHellgate() &&
			   pTask->tTaskTemplate.nLevelDefinition != INVALID_ID && 
			   LevelGetDefinitionIndex( pLevel ) == pTask->tTaskTemplate.nLevelDefinition ) ||
			 ( AppIsTugboat() &&
			   pTask->tTaskTemplate.nLevelDepth != 0 && 
			   LevelGetDepth( pLevel ) == pTask->tTaskTemplate.nLevelDepth &&
			   pTask->tTaskTemplate.nLevelArea == LevelGetLevelAreaID(  pLevel ) ) )
		{
			pTask->idPendingLevelSpawn = LevelGetID( pLevel );
			pTask->pfnLevelActivate(pGame, pTask, pLevel);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoRoomActivated( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_ROOM_ACTIVATE* pMessage)
{
	ROOM* pRoom = pMessage->pRoom;
	
	// do any pending level spawns
	if (pTask->idPendingRoomSpawn == RoomGetId( pRoom ))
	{
		sDoPendingRoomSpawns( pGame, pTask, pRoom );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoRoomEntered( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_ROOM_ENTER* pMessage)
{
	if(pTask->idPlayer == UnitGetId(pMessage->pPlayer))
	{
		sCheckTaskComplete( pGame, pTask );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoInventoryAdd( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_INVENTORY_ADD* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETURN( pPlayer == UnitGetById( pGame, pTask->idPlayer ), "Player should be the owner of this task" );
	
	// check for completing a collect task
	if (pTask->eStatus != TASK_STATUS_COMPLETE &&
		pTask->eStatus != TASK_STATUS_REWARD)
	{
		UNIT* pAdded = pMessage->pAdded;
		if (UnitIsA( pAdded, UNITTYPE_ITEM ))
		{
			for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
			{
				TASK_COLLECT* pCollect = &pTask->tTaskCollect[ i ];
				if (pCollect->nCount > 0 && pCollect->nItemClass == UnitGetClass( pAdded ))
				{
					sCheckTaskComplete( pGame, pTask );
				}
				
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskDoInventoryRemove( 
	GAME* pGame,
	TASK* pTask,
	const TASK_MESSAGE_INVENTORY_REMOVE* pMessage)
{

	// check for 'uncompleting' a collect task
	if (pTask->eStatus == TASK_STATUS_COMPLETE)
	{
		UNIT* pRemoved = pMessage->pRemoved;
		if (UnitIsA( pRemoved, UNITTYPE_ITEM ))
		{
			for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
			{
				TASK_COLLECT* pCollect = &pTask->tTaskCollect[ i ];
				if (pCollect->nCount > 0 && pCollect->nItemClass == UnitGetClass( pRemoved ))
				{
					sCheckTaskComplete( pGame, pTask );
				}
				
			}
			
		}
			
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskProcessMessage( 
	GAME* pGame,
	TASK* pTask, 
	TASK_MESSAGE eTaskMessage, 
	void* pMessage )
{
	switch (eTaskMessage)
	{
		
		//----------------------------------------------------------------------------
		case TM_MONSTER_KILL:
		{
			const TASK_MESSAGE_MONSTER_KILL* pMessageMonsterKill = (const TASK_MESSAGE_MONSTER_KILL*)pMessage;
			sTaskDoMonsterKill( pGame, pTask, pMessageMonsterKill );
			break;
		}

		//----------------------------------------------------------------------------
		case TM_LEVEL_ACTIVATED:
		{
			const TASK_MESSAGE_LEVEL_ACTIVATE* pMessageLevelActivate = (const TASK_MESSAGE_LEVEL_ACTIVATE*)pMessage;
			sTaskDoLevelActivated( pGame, pTask, pMessageLevelActivate );
			break;
		}

		//----------------------------------------------------------------------------
		case TM_ROOM_ACTIVATED:
		{
			const TASK_MESSAGE_ROOM_ACTIVATE* pMessageRoomActivate = (const TASK_MESSAGE_ROOM_ACTIVATE*)pMessage;
			sTaskDoRoomActivated( pGame, pTask, pMessageRoomActivate );
			break;
		}

		//----------------------------------------------------------------------------
		case TM_INVENTORY_ADD:
		{
			const TASK_MESSAGE_INVENTORY_ADD* pMessageInventoryAdd = (const TASK_MESSAGE_INVENTORY_ADD*)pMessage;
			ItemIsInEquipLocation( pMessageInventoryAdd->pPlayer, pMessageInventoryAdd->pAdded );
			sTaskDoInventoryAdd( pGame, pTask, pMessageInventoryAdd );
			break;
		}

		//----------------------------------------------------------------------------
		case TM_INVENTORY_REMOVE:
		{
			const TASK_MESSAGE_INVENTORY_REMOVE* pMessageInventoryRemove = (const TASK_MESSAGE_INVENTORY_REMOVE*)pMessage;
			sTaskDoInventoryRemove( pGame, pTask, pMessageInventoryRemove );
			break;
		}
				
		//----------------------------------------------------------------------------
		case TM_ROOM_ENTERED:
		{
			const TASK_MESSAGE_ROOM_ENTER* pMessageRoomEntered = (const TASK_MESSAGE_ROOM_ENTER*)pMessage;
			sTaskDoRoomEntered( pGame, pTask, pMessageRoomEntered );
			break;
		}
				
		//----------------------------------------------------------------------------
		default:
		{
			break;
		}
		
	}
	
}

//----------------------------------------------------------------------------
struct MESSAGE_STATUS_LOOKUP
{
	TASK_MESSAGE eMessage;
	BOOL bActive;
	BOOL bComplete;
};
static MESSAGE_STATUS_LOOKUP sgtMessageStatusLookup[] = 
{
	// Message type				ok for active		ok for complete
	{ TM_MONSTER_KILL,			TRUE,				FALSE },
	{ TM_LEVEL_ACTIVATED,		TRUE,				FALSE },
	{ TM_ROOM_ACTIVATED,		TRUE,				FALSE },
	{ TM_INVENTORY_ADD,			TRUE,				TRUE },
	{ TM_INVENTORY_REMOVE,		TRUE,				TRUE },
	
	{ TM_INVALID,				FALSE,				FALSE }  // keep this last please
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const MESSAGE_STATUS_LOOKUP* sLookupMessageStatus(
	TASK_MESSAGE eMessage)
{
	for (int i = 0; sgtMessageStatusLookup[ i ].eMessage != TM_INVALID; ++i)
	{
		const MESSAGE_STATUS_LOOKUP* pLookup = &sgtMessageStatusLookup[ i ];
		if (pLookup->eMessage == eMessage)
		{
			return pLookup;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskMessageAllowedForTaskStatus(
	TASK_MESSAGE eTaskMessage, 
	TASK_STATUS eStatus)
{
	const MESSAGE_STATUS_LOOKUP* pLookup = sLookupMessageStatus( eTaskMessage );
	if (pLookup)
	{
		if (eStatus == TASK_STATUS_ACTIVE && pLookup->bActive == TRUE)
		{
			return TRUE;
		}
		else if(eStatus == TASK_STATUS_COMPLETE && pLookup->bComplete == TRUE)
		{
			return TRUE;
		}
		return FALSE;
	}
	else
	{
		// if no lookup is set, the default is active or complete
		if (eStatus == TASK_STATUS_ACTIVE || eStatus == TASK_STATUS_COMPLETE)
		{
			return TRUE;
		}
		return FALSE;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendMessageToSingleTask(
	GAME* pGame,
	TASK* pTask,
	TASK_MESSAGE eTaskMessage,
	void* pMessage)
{
	if (pTask->eStatus == TASK_STATUS_ACTIVE || 
		pTask->eStatus == TASK_STATUS_COMPLETE)
	{
	if (sTaskMessageAllowedForTaskStatus( eTaskMessage, pTask->eStatus ))
		sTaskProcessMessage( pGame, pTask, eTaskMessage, pMessage );
	}
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSTaskSendMessageAll(
	GAME* pGame,
	TASK_MESSAGE eTaskMessage,
	void* pMessage)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	if (pTaskGlobals == NULL)
	{
		return;
	}
	
	// go through all tasks
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		sSendMessageToSingleTask( pGame, pTask, eTaskMessage, pMessage );
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSTaskSendMessageWithPlayer(
	GAME* pGame,
	UNIT* pPlayer,
	TASK_MESSAGE eTaskMessage,
	void* pMessage)
{
	if ( ! AppIsHellgate() )
		return;

	ASSERTX_RETURN( pPlayer, "Expected player" );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	if (pTaskGlobals == NULL)
	{
		return;
	}
	
	// go through all tasks
	UNITID idPlayer = UnitGetId( pPlayer );
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		if (pTask->idPlayer == idPlayer)
		{
			sSendMessageToSingleTask( pGame, pTask, eTaskMessage, pMessage );			
		}
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskTimeLimitExpired(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& tEventData)
{
	UNIT* pPlayer = pUnit;
	ASSERTX_RETTRUE( pPlayer && UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player for task expiration" );
	GAMETASKID idTask = (GAMETASKID)tEventData.m_Data1;
	
	// find task
	TASK* pTask = sTaskFind( pGame, idTask, pPlayer );
	if (pTask)
	{
		if (pTask->eStatus != TASK_STATUS_FAILED)
		{
			sTaskFailed( pGame, pTask );
		}
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskAccept(
	GAME* pGame,
	UNIT* pPlayer,
	TASK* pTask)
{

	// set task to active
	ASSERTX( pTask->eStatus == TASK_STATUS_GENERATED, "Task expected to be in generated state" );	
	pTask->eStatus = TASK_STATUS_ACTIVE;
	pTask->dwAcceptTick = GameGetTick( pGame );
	
	// try to start the unit instancing process for levels that are already active
	if (pTask->idPendingLevelSpawn != INVALID_ID)
	{
		LEVEL* pLevel = LevelGetByID( pGame, pTask->idPendingLevelSpawn );
		if (LevelIsActive(pLevel))
		{
			sDoPendingLevelSpawns( pGame, pTask, pLevel );
		}
	}

	// send status to player
	sTaskSendStatus( pGame, pTask );

	// check for task complete immediately (this could happen with a collect task for instance)
	sCheckTaskComplete( pGame, pTask );

	// if task has a time limit, register it now
	const TASK_TEMPLATE* pTaskTemplate = &pTask->tTaskTemplate;
	if (pTaskTemplate->nTimeLimitInMinutes > 0)
	{
		int nTimeLimitInTicks = GAME_TICKS_PER_SECOND * 60 * pTaskTemplate->nTimeLimitInMinutes;
								
		// setup event to expire task in time limit
		EVENT_DATA tEventData( (DWORD_PTR)pTask->idTask );				
		UnitRegisterEventTimed(
			pPlayer, 
			sTaskTimeLimitExpired,
			&tEventData,
			nTimeLimitInTicks);
	
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskAbandon(
	GAME* pGame,
	UNIT* pPlayer,
	TASK* pTask)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
			
	// remove the task from the system
	sTaskFree( pGame, pTaskGlobals, pTask );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskAccept(
	UNIT* pPlayer,
	GAMETASKID idTask)
{
	GAME* pGame = UnitGetGame( pPlayer );
	TASK* pTask = sTaskFind( pGame, idTask, pPlayer );	
	if (pTask != NULL)
	{
		sTaskAccept( pGame, pPlayer, pTask );
	}
	
}	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskAbandon(
	UNIT* pPlayer,
	GAMETASKID idTask)
{
	GAME* pGame = UnitGetGame( pPlayer );
	TASK* pTask = sTaskFind( pGame, idTask, pPlayer );	
	if (pTask != NULL)
	{
		sTaskAbandon( pGame, pPlayer, pTask );
	}
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskAbanbdonAll(
	UNIT * pPlayer)
{
	GAME * pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(pGame);
	TASK_GLOBALS * pTaskGlobals = sTaskGlobalsGet(pGame);
	if (!pTaskGlobals)
	{
		return;
	}
	
	UNITID idPlayer = UnitGetId( pPlayer );
	// go through all tasks for this player, and cancel them all
	TASK* pNext = NULL;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pNext)
	{
		
		// save next task
		pNext = pTask->pNext;
		
		// is match
		if (pTask->idPlayer == idPlayer)
		{
			// cancel it
			sTaskAbandon( pGame, pPlayer, pTask );			
		}
		
	}
	
}	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskCloseAll(
	UNIT * pPlayer)
{
	GAME * pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(pGame);
	TASK_GLOBALS * pTaskGlobals = sTaskGlobalsGet(pGame);
	if (!pTaskGlobals)
	{
		return;
	}
	
	UNITID idPlayer = UnitGetId( pPlayer );
	// go through all tasks for this player, and cancel them all
	TASK* pNext = NULL;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pNext)
	{
		
		// save next task
		pNext = pTask->pNext;
		
		// is match
		if (pTask->idPlayer == idPlayer)
		{
			// cancel it
			sTaskClose( pGame, pTask );
		}
		
	}
	
}	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TASK* sFindTask( 
	UNIT* pPlayer, 
	UNIT* pGiver, 
	TASK_STATUS eStatus)
{
	GAME* pGame = UnitGetGame( pPlayer );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	UNITID idPlayer = UnitGetId( pPlayer );
	SPECIES speciesGiver = UnitGetSpecies( pGiver );
	
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
	
		// match params
		if (pTask->idPlayer == idPlayer &&
			pTask->speciesGiver == speciesGiver && 
			pTask->eStatus == eStatus)
		{
			return pTask;
		}
		
	}
	
	// no match found
	return NULL; 
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
UNIT_INTERACT s_TaskTalkToGiver(
	UNIT* pPlayer,
	UNIT* pGiver)
{

	// not a task giver
	UNIT_INTERACT eResult = UNIT_INTERACT_NONE;	
	if (UnitIsTaskGiver( pGiver ) == FALSE)
	{
		return eResult;
	}

	GAME* pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );		

	// does the player have any tasks complete from this task giver?
	TASK* pTaskComplete = sFindTask( pPlayer, pGiver, TASK_STATUS_COMPLETE );
	
	//TUGBOAT did not have this line. But seems ok to leave in.
	if (pTaskComplete == NULL)
	{
		pTaskComplete = sFindTask( pPlayer, pGiver, TASK_STATUS_REWARD );
	}

	TASK* pTaskActive = sFindTask( pPlayer, pGiver, TASK_STATUS_ACTIVE );
	if (pTaskComplete != NULL)
	{
	
		// mark them as talking to the giver (order is important so we know we're talking
		// when we bring up the reward dialog)
		s_TalkSetTalkingTo( pPlayer, pGiver, INVALID_LINK );
	
		// offer the player the reward dialog
		MSG_SCMD_TASK_OFFER_REWARD tMessage;
		tMessage.idTaskGiver = UnitGetId( pGiver );
		tMessage.idTask = pTaskComplete->idTask;

		// send it
		s_SendMessage( pGame, idClient, SCMD_TASK_OFFER_REWARD, &tMessage );	

		// we did task interaction
		eResult = UNIT_INTERACT_TASK;				
		
	}
	else if (pTaskActive != NULL)
	{
		// offer the player the reward dialog
		MSG_SCMD_TASK_INCOMPLETE pMessage;
		pMessage.idTask = pTaskActive->idTask;

		// mark them as talking to the giver (order is important so we know we're talking
		// when we bring up the reward dialog)
		s_TalkSetTalkingTo( pPlayer, pGiver, INVALID_LINK );	

		// send it
		s_SendMessage( pGame, idClient, SCMD_TASK_INCOMPLETE, &pMessage );	

		// we did task interaction
		eResult = UNIT_INTERACT_TASK; //NPC_INTERACT_TASK;				
		
	}
	else
	{

		// if this task giver is faction aligned, the player must be in a reasonable standing with
		// that faction in order to get any tasks from this task giver
		int nFactions[ FACTION_MAX_FACTIONS ];
		int nFactionCount = UnitGetFactions( pGiver, nFactions );
		if (nFactionCount > 0)
		{
		
			// setup an array to hold which factions we are in bad standing with
			int nFactionsBad[ FACTION_MAX_FACTIONS ];
			int nNumBadFactions = 0;
			
			// find all the factions that we are in "bad" moods with
			for (int i = 0; i < nFactionCount; ++i)
			{
				int nFaction = nFactions[ i ];
				int nFactionStanding = FactionGetStanding( pPlayer, nFaction );
				if (FactionStandingGetMood( nFactionStanding ) == FSM_MOOD_BAD )
				{
					nFactionsBad[ nNumBadFactions++ ] = nFaction;
				}
			}
			
			// if there were any bad standing moods, this task giver cannot give tasks to the player
			if (nNumBadFactions > 0)
			{
			
				// offer the player the reward dialog
				MSG_SCMD_TASK_RESTRICTED_BY_FACTION tMessage;
				tMessage.idGiver = UnitGetId( pGiver );
				for (int i = 0; i < FACTION_MAX_FACTIONS; ++i)
				{
					if (i < nNumBadFactions)
					{
						tMessage.nFactionBad[ i ] = nFactionsBad[ i ];					
					}
					else
					{
						tMessage.nFactionBad[ i ] = INVALID_LINK;
					}
				}

				// send it
				s_SendMessage( pGame, idClient, SCMD_TASK_RESTRICTED_BY_FACTION, &tMessage );	
			
				// we've used the interaction
				eResult = UNIT_INTERACT_TASK;							
				return eResult;
				
			}
			
		}
				
		// mark them as talking to the giver
		s_TalkSetTalkingTo( pPlayer, pGiver, INVALID_LINK );
		
		// get the available tasks for this player from this NPC
		TASK_TEMPLATE availableTasks[ MAX_AVAILABLE_TASKS_AT_GIVER ];
		int nCount = s_TaskGetAvailableTasks( pGame, pPlayer, pGiver, availableTasks, MAX_AVAILABLE_TASKS_AT_GIVER );
					
		// if there are tasks available, tell send them to the client
		if (nCount > 0)
		{
			if( AppIsTugboat() &&
				sGetActiveTasks( pGame, pPlayer ) >= TaskMaxTasksPerPlayer() )
			{
				// too many already to accept this!
				s_NPCTalkStart( pPlayer, pGiver, NPC_DIALOG_OK, DIALOG_TASKS_TOO_BUSY );
				eResult = UNIT_INTERACT_TASK;			
			}
			else
			{
				s_TaskSendAvailableTasksToClient( pGame, idClient, availableTasks, nCount, pGiver );
				eResult = UNIT_INTERACT_TASK;				
			}

		}
		else
		{
			if ( AppIsHellgate() )
			{
				// no tasks available, return a result which allows other interactions (quests for example)
				eResult = UNIT_INTERACT_NONE;
			}
			else
			{
				// for now, task givers do nothing but give tasks
				s_NPCTalkStart( pPlayer, pGiver, NPC_DIALOG_OK, DIALOG_TASKS_NO_TASKS );
				eResult = UNIT_INTERACT_TASK;
			}
		}

	}
	
	return eResult;
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetSavableTaskCount( 
	UNIT* pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected player" );	
	GAME* pGame = UnitGetGame( pPlayer );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	UNITID idPlayer = UnitGetId( pPlayer );
	
	// count tasks owned by this player
	int nTaskCount = 0;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		if (pTask->idPlayer == idPlayer && sTaskIsSavable( pTask ))
		{
			nTaskCount++;
		}
		
	}	

	return nTaskCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTaskGetRewardCount( 
	TASK* pTask)
{
	int nRewardCount = 0;
	
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
		if (pTask->idRewards[ i ] != INVALID_ID)
		{
			nRewardCount++;
		}
	}
	return nRewardCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSaveTask(
	UNIT* pPlayer,
	TASK* pTask,
	int nTaskIndex)
{	
	ASSERTX_RETURN( sTaskIsSavable( pTask ) == TRUE, "Task is not savable" );
	
	// save task definition code
	UnitSetStat( pPlayer, STATS_SAVE_TASK_DEFINITION_CODE, nTaskIndex, pTask->pTaskDefinition->wCode );	
	
	// save task status
	ASSERTX_RETURN( 
		pTask->eStatus == TASK_STATUS_ACTIVE ||	
		pTask->eStatus == TASK_STATUS_COMPLETE ||
		pTask->eStatus == TASK_STATUS_FAILED ||
		pTask->eStatus == TASK_STATUS_REWARD,
		"Invalid task status to be saved" );	
	UnitSetStat( pPlayer, STATS_SAVE_TASK_STATUS, nTaskIndex, pTask->eStatus );

	// save task id
	UnitSetStat( pPlayer, STATS_SAVE_TASK_ID, nTaskIndex, pTask->idTask );
		
	// save giver species
	int nClass = GET_SPECIES_GENUS( pTask->speciesGiver );
	UnitSetStat( pPlayer, STATS_SAVE_TASK_GIVER_MONSTER_CLASS, nTaskIndex, nClass );

	// save target level
	const TASK_TEMPLATE* pTaskTemplate = &pTask->tTaskTemplate;
	UnitSetStat( pPlayer, 
				 STATS_SAVE_TASK_TARGET_LEVEL_DEFINITION, 
				 nTaskIndex,
				 ((AppIsHellgate())?pTaskTemplate->nLevelDefinition:pTaskTemplate->nLevelDepth) );
	if( AppIsTugboat() )
	{
		UnitSetStat( pPlayer, STATS_SAVE_TASK_TARGET_LEVEL_AREA, nTaskIndex, pTaskTemplate->nLevelArea );
	}
	
	// how many targets are there (they are either pending or instanced)
	ASSERTX( pTask->nNumUnitTargets == 0 || pTask->nNumPendingTargets == 0, "Task pending or instanced targets should be zero" );
	int nNumTargets = pTask->nNumUnitTargets + pTask->nNumPendingTargets;
		
	// save target count
	UnitSetStat( pPlayer, STATS_SAVE_TASK_TARGET_COUNT, nTaskIndex, nNumTargets );
	
	if( AppIsTugboat() )
	{
		// save xp and gold rewards
		UnitSetStat( pPlayer, STATS_SAVE_TASK_EXPERIENCE_REWARD, nTaskIndex, pTask->nRewardExperience );
		UnitSetStat( pPlayer, STATS_SAVE_TASK_GOLD_REWARD, nTaskIndex, pTask->nRewardGold );
	}
	// save all targets
	for (int i = 0; i < nNumTargets; ++i)
	{
		const TASK_UNIT_TARGET* pTaskTarget = &pTask->unitTargets[ i ];
		const PENDING_UNIT_TARGET* pPendingTarget = &pTask->tPendingUnitTargets[ i ];
		
		// lookup the stat save structure that we use for this target index
		const TARGET_SAVE_LOOKUP* pTargetSaveLookup = &sgtTargetSaveLookup[ i ];

		// save target status, the target is assumed to be alive unless it has
		// actually been instanced and killed
		TASK_UNIT_STATUS eTargetStatus = TASK_UNIT_STATUS_ALIVE;
		if (pTaskTarget != NULL)
		{
			if (pTaskTarget->eStatus == TASK_UNIT_STATUS_DEAD )
			{
				eTargetStatus = pTaskTarget->eStatus;
			}
		}
		UnitSetStat( pPlayer, pTargetSaveLookup->eStatStatus, nTaskIndex, eTargetStatus );
		
		// save target name
		UnitSetStat( pPlayer, pTargetSaveLookup->eStatName, nTaskIndex, pPendingTarget->nTargetUniqueName );		
		
		// save target species
		UnitSetStat( 
			pPlayer, 
			pTargetSaveLookup->eStatMonsterClass, 
			nTaskIndex, 
			GET_SPECIES_CLASS( pPendingTarget->speciesTarget ) );
		
	}

	// save numer of things to collect
	UnitSetStat( pPlayer, STATS_SAVE_TASK_COLLECT_COUNT, nTaskIndex, pTask->nNumCollect );
	
	// save each collect entry
	for (int i = 0; i < pTask->nNumCollect; ++i)
	{
		TASK_COLLECT* pCollect = &pTask->tTaskCollect[ i ];

		// lookup the stat save structure that we use for this collect index
		const COLLECT_SAVE_LOOKUP* pCollectSaveLookup = &sgtCollectSaveLookup[ i ];
		
		// save class
		UnitSetStat( pPlayer, pCollectSaveLookup->eStatClass, nTaskIndex, pCollect->nItemClass );
		
		// save count
		UnitSetStat( pPlayer, pCollectSaveLookup->eStatCount, nTaskIndex, pCollect->nCount );
		
	}
	
	// save num rewards
	int nRewardCount = sTaskGetRewardCount( pTask );
	UnitSetStat( pPlayer, STATS_SAVE_TASK_REWARD_COUNT, nTaskIndex, nRewardCount );

	// save count of types of things to exterminate
	int nNumExterminateTypes = 0;
	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
	{
		const TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
		if (pExterminate->speciesTarget != SPECIES_NONE)
		{
			nNumExterminateTypes++;
		}
	}

	// save faction rewards
	UnitSetStat( pPlayer, STATS_SAVE_TASK_PRIMARY_FACTION, nTaskIndex, pTask->nFactionPrimary );	
	const FACTION_REWARDS *pFactionRewards = &pTask->tTaskTemplate.tFactionRewards;
	int nFactionRewardCount = pFactionRewards->nCount;
	UnitSetStat( pPlayer, STATS_SAVE_TASK_FACTION_REWARD_COUNT, nFactionRewardCount );
	for (int i = 0; i < FactionGetNumFactions(); ++i)
	{
		UnitClearStat( pPlayer, STATS_SAVE_TASK_FACTION_REWARD, i );
	}
	for (int i = 0; i < nFactionRewardCount; ++i)
	{
		UnitSetStat( 
			pPlayer, 
			STATS_SAVE_TASK_FACTION_REWARD, 
			pFactionRewards->tReward[ i ].nFaction, 
			pFactionRewards->tReward[ i ].nPoints );
	}
	
	// save exterminate types
	UnitSetStat( pPlayer, STATS_SAVE_TASK_EXTERMINATE_TYPES, nTaskIndex, nNumExterminateTypes );	
	for (int i = 0; i < nNumExterminateTypes; ++i)
	{
		const EXTERMINATE_SAVE_LOOKUP* pExterminateLookup = &sgtExterminateSaveLookup[ i ];
		const TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
		
		// save target class
		GENUS eGenusTarget = (GENUS)GET_SPECIES_GENUS( pExterminate->speciesTarget );
		ASSERTX( eGenusTarget == GENUS_MONSTER, "Expected monster genus" );
		int nClassTarget = GET_SPECIES_CLASS( pExterminate->speciesTarget );
		UnitSetStat( pPlayer, pExterminateLookup->eStatMonsterClass, nTaskIndex, nClassTarget );
		
		// save count
		UnitSetStat( pPlayer, pExterminateLookup->eStatCount, nTaskIndex, pExterminate->nCount );
		
	}
	
	// save count of exterminated
	int nExterminatedCount = pTaskTemplate->nExterminatedCount;
	UnitSetStat( pPlayer, STATS_SAVE_TASK_EXTERMINATED_COUNT, nTaskIndex, nExterminatedCount );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TaskWriteToPlayer(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );	
	GAME* pGame = UnitGetGame( pPlayer );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	UNITID idPlayer = UnitGetId( pPlayer );

	// write task save version
	UnitSetStat( pPlayer, STATS_SAVE_TASK_VERSION, TASK_SAVE_VERSION );
	
	// find all tasks associated with this player
	int nTaskCount = sGetSavableTaskCount( pPlayer );
	
	// save task count
	UnitSetStat( pPlayer, STATS_SAVE_TASK_COUNT, nTaskCount );
		
	// save each task
	int nTaskIndex = 0;
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		if (pTask->idPlayer == idPlayer && sTaskIsSavable( pTask ))
		{
			sSaveTask( pPlayer, pTask, nTaskIndex++ );
		}
		
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendRestoredTaskToPlayer(
	GAME* pGame,
	TASK* pTask,
	UNIT* pPlayer)
{
	MSG_SCMD_TASK_RESTORE message;
	message.tTaskTemplate = pTask->tTaskTemplate;
	message.eStatus = pTask->eStatus;
	
	// send it
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TASK_RESTORE, &message );	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static BOOL sRestoreTask(
	UNIT* pPlayer,
	int nTaskIndex,
	DWORD& dwRewardTouched )
{
	GAME* pGame = UnitGetGame( pPlayer );
	
	// read task definition code
	WORD wCode = (WORD)UnitGetStat( pPlayer, STATS_SAVE_TASK_DEFINITION_CODE, nTaskIndex );	
	const TASK_DEFINITION* pTaskDefinition = TaskDefinitionGetByCode(  wCode );
	ASSERTX_RETFALSE( pTaskDefinition, "Unknown code for task definition in player save file" );
	
	// read task status
	TASK_STATUS eStatus = (TASK_STATUS)UnitGetStat( pPlayer, STATS_SAVE_TASK_STATUS, nTaskIndex );
	ASSERTX_RETFALSE( eStatus > TASK_STATUS_INVALID && eStatus < NUM_TASK_STATUS, "Invalid task status in player save file" );

	// read id (this is the id instance of this task in the last game this player was in)
	GAMETASKID idTaskOld = (GAMETASKID)UnitGetStat( pPlayer, STATS_SAVE_TASK_ID, nTaskIndex );
				
	// read giver info
	int nClassGiver = UnitGetStat( pPlayer, STATS_SAVE_TASK_GIVER_MONSTER_CLASS, nTaskIndex );
	GENUS eGenusGiver = GENUS_MONSTER;
	SPECIES speciesGiver = MAKE_SPECIES( eGenusGiver, nClassGiver );
	ASSERTX_RETFALSE( eGenusGiver >= 0 && eGenusGiver < NUM_GENUS, "Invalid task giver genus in player save file" );
	ASSERTX_RETFALSE((unsigned int)nClassGiver < ExcelGetNumRows( pGame, UnitGenusGetDatatableEnum( eGenusGiver ) ), "Invalid task giver class read from save file" );
	int nTargetLevelDepth(-1),
		nTargetLevelArea(-1),
		nTargetLevelDefinition(-1);
	if( AppIsHellgate() )
	{
		// read target level
		nTargetLevelDefinition = UnitGetStat( pPlayer, STATS_SAVE_TASK_TARGET_LEVEL_DEFINITION, nTaskIndex );

	}
	else
	{
		// read target level
		nTargetLevelDepth = UnitGetStat( pPlayer, STATS_SAVE_TASK_TARGET_LEVEL_DEFINITION, nTaskIndex );
	
		// read target level
		nTargetLevelArea = UnitGetStat( pPlayer, STATS_SAVE_TASK_TARGET_LEVEL_AREA, nTaskIndex );
	}

	
	// read target count
	int nTargetCount = UnitGetStat( pPlayer, STATS_SAVE_TASK_TARGET_COUNT, nTaskIndex );
	int nRewardExperience = 0;
	int nRewardGold = 0;
	if( AppIsTugboat() )
	{
		// restore xp and gold rewards
		nRewardExperience = UnitGetStat( pPlayer, STATS_SAVE_TASK_EXPERIENCE_REWARD, nTaskIndex );
		nRewardGold = UnitGetStat( pPlayer, STATS_SAVE_TASK_GOLD_REWARD, nTaskIndex );
	}
	// init memory for targets
	RESTORE_TARGET tRestoreTargets[ MAX_TASK_UNIT_TARGETS ];
	for (int i = 0; i < MAX_TASK_UNIT_TARGETS; ++i)
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
		pRestoreTarget->eTargetStatus = (TASK_UNIT_STATUS)UnitGetStat( pPlayer, pTargetSaveLookup->eStatStatus, nTaskIndex );
		ASSERTX_RETFALSE( pRestoreTarget->eTargetStatus > TASK_UNIT_STATUS_INVALID && pRestoreTarget->eTargetStatus < NUM_TASK_UNIT_STATUS, "Invalid task unit target status read from player save file" );
		
		pRestoreTarget->nTargetUniqueName = UnitGetStat( pPlayer, pTargetSaveLookup->eStatName, nTaskIndex );
		ASSERTX_RETFALSE( pRestoreTarget->nTargetUniqueName < MonsterNumProperNames(), "Invalid task unit target unique name read from player save file" );

		GENUS eGenusTarget = GENUS_MONSTER;
		int nClassTarget = UnitGetStat( pPlayer, pTargetSaveLookup->eStatMonsterClass, nTaskIndex );
		pRestoreTarget->speciesTarget = MAKE_SPECIES( eGenusTarget, nClassTarget );
		ASSERTX_RETFALSE( eGenusTarget >= GENUS_NONE && eGenusTarget < NUM_GENUS, "Invalid task unit target genus read from player save file" );
		ASSERTX_RETFALSE((unsigned int)nClassTarget < ExcelGetNumRows( pGame, UnitGenusGetDatatableEnum( eGenusTarget ) ), "Invalid task unit target class read from save file" );
			
	}		

	// read numer of things to collect
	int nNumCollect = UnitGetStat( pPlayer, STATS_SAVE_TASK_COLLECT_COUNT, nTaskIndex );

	// read each collect entry
	TASK_COLLECT tRestoreCollects[ MAX_TASK_COLLECT_TYPES ];	
	for (int i = 0; i < nNumCollect; ++i)
	{
		TASK_COLLECT* pCollect = &tRestoreCollects[ i ];

		// lookup the stat save structure that we use for this collect index
		const COLLECT_SAVE_LOOKUP* pCollectSaveLookup = &sgtCollectSaveLookup[ i ];
		
		// read class
		pCollect->nItemClass = UnitGetStat( pPlayer, pCollectSaveLookup->eStatClass, nTaskIndex );
		ASSERTX_RETFALSE((unsigned int)pCollect->nItemClass < ExcelGetNumRows( pGame, DATATABLE_ITEMS ), "Invalid collect item class" );
		
		// read lcount
		pCollect->nCount = UnitGetStat( pPlayer, pCollectSaveLookup->eStatCount, nTaskIndex );
		ASSERTX_RETFALSE( pCollect->nCount > 0, "Collect count should be more than 0" );
		
	}

	// read num rewards
	// int nNumRewards = UnitGetStat( pPlayer, STATS_SAVE_TASK_REWARD_COUNT, nTaskIndex );

	// read faction rewards
	int nFactionPrimary = UnitGetStat( pPlayer, STATS_SAVE_TASK_PRIMARY_FACTION, nTaskIndex );
	FACTION_REWARDS tFactionRewards;
	sFactionRewardsInit( &tFactionRewards );
	tFactionRewards.nCount = UnitGetStat( pPlayer, STATS_SAVE_TASK_FACTION_REWARD_COUNT, nTaskIndex );
	UNIT_ITERATE_STATS_RANGE( pPlayer, STATS_SAVE_TASK_FACTION_REWARD, tStatsEntry, i, MAX_TASK_FACTION_REWARDS )
	{
		tFactionRewards.tReward[ i ].nFaction = tStatsEntry[ i ].GetParam();
		tFactionRewards.tReward[ i ].nPoints = tStatsEntry[ i ].value;
	}
	UNIT_ITERATE_STATS_END;
	
	// read exterminate types
	int nNumExterminateTypes = UnitGetStat( pPlayer, STATS_SAVE_TASK_EXTERMINATE_TYPES, nTaskIndex );
	TASK_EXTERMINATE tTaskExterminate[ MAX_TASK_EXTERMINATE ];
	for (int i = 0; i < nNumExterminateTypes; ++i)
	{
		const EXTERMINATE_SAVE_LOOKUP* pExterminateSaveLookup = &sgtExterminateSaveLookup[ i ];
		TASK_EXTERMINATE* pExterminate = &tTaskExterminate[ i ];
		
		// read garget
		GENUS eGenusTarget = GENUS_MONSTER;
		int nClassTarget = UnitGetStat( pPlayer, pExterminateSaveLookup->eStatMonsterClass, nTaskIndex );
		pExterminate->speciesTarget = MAKE_SPECIES( eGenusTarget, nClassTarget );
		
		// read count
		pExterminate->nCount = UnitGetStat( pPlayer, pExterminateSaveLookup->eStatCount, nTaskIndex );
		
	}
				
	// read num exterminated so far
	int nExterminatedCount = UnitGetStat( pPlayer, STATS_SAVE_TASK_EXTERMINATED_COUNT, nTaskIndex );
			
	// create a new task (don't pick targets or generate rewards etc)
	DWORD dwCreateFlags = 0;
	SETBIT( dwCreateFlags, TC_RESTORING_BIT );
	TASK* pTask = sCreateNewTask( pGame, pPlayer, speciesGiver, nFactionPrimary, pTaskDefinition, dwCreateFlags );
	ASSERTX_RETFALSE( pTask, "Unable to create task for restoration" );

	// convert num items taken from old task id to new one
	int nNumTaken = UnitGetStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, idTaskOld );
	UnitClearStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, idTaskOld );
	UnitSetStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask, nNumTaken );	
			
	// restore status
	pTask->eStatus = eStatus;

	// restore level
	pTask->tTaskTemplate.nLevelDefinition = nTargetLevelDefinition;
	pTask->tTaskTemplate.nLevelArea = nTargetLevelArea;
	pTask->tTaskTemplate.nLevelDepth = nTargetLevelDepth;
	pTask->idPendingLevelSpawn = INVALID_ID;
	pTask->idPendingRoomSpawn = INVALID_ID;	
	pTask->nRewardExperience = nRewardExperience;
	pTask->nRewardGold = nRewardGold;
	pTask->nPendingLevelSpawnDefinition = nTargetLevelDefinition;
	pTask->nPendingLevelSpawnDepth = nTargetLevelDepth;
	pTask->nPendingLevelSpawnArea = nTargetLevelArea;
	
	// restore collect items
	ASSERTX_RETFALSE( pTask->nNumCollect == 0, "Expected no collect items" );
	for (int i = 0; i < nNumCollect; ++i)
	{
		TASK_COLLECT* pCollect = &tRestoreCollects[ i ];
		
		for (int j = 0; j < pCollect->nCount; ++j)
		{
			sAddCollect( pTask, pCollect->nItemClass );	
		}
		
	}
	ASSERTX_RETFALSE( pTask->nNumCollect == nNumCollect, "Collect items not restored to task properly" );
	
	// iterate the reward inventory of the player and update the link on each item
	// to the newly created task id
	int nRewardIndex = 0;	
	int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS );
	UNIT* pReward = NULL;
	DWORD Index = 1;
	while ( ( pReward = UnitInventoryLocationIterate( pPlayer, nInventorySlot, pReward, 0, FALSE ) ) != NULL )
	{
	
		int Mask = MAKE_MASK(Index);
		// was this reward for this task .. check by looking at the link on the 
		// item to the old task id
		if ( UnitGetStat( pReward, STATS_TASK_REWARD ) == idTaskOld &&
			 ( AppIsHellgate() ||
			   ( AppIsTugboat() && !( dwRewardTouched & Mask ) ) ) )
		{
			// link reward to task (note we do not add to player inventory since it's already there)
			sLinkRewardToTask( pGame, pTask, pReward, nRewardIndex++, FALSE );
			dwRewardTouched |= Mask;
		}
		Index++;
	}
	// the following assert is no longer valid cause we have a reward interface where you can
	// take some of the reward and not all of it	
//	ASSERTX_RETFALSE( nRewardIndex == nNumRewards, "Didn't find right amount of rewards in player inventory for task rewards on restored task" );
	
	// setup the targets as pending targets
	ASSERTX_RETFALSE( pTask->nNumPendingTargets == 0, "Expected no pending unit targets" );
	ASSERTX_RETFALSE( pTask->nNumUnitTargets == 0, "Expected no unit targets" );	
	for (int i = 0; i < nTargetCount; ++i)
	{
		RESTORE_TARGET* pRestoreTarget = &tRestoreTargets[ i ];
		
		// put target information back into the pending task units
		sAddPendingTarget( pTask, pRestoreTarget->speciesTarget, pRestoreTarget->nTargetUniqueName );		
		
		// restore status in live target
		TASK_UNIT_TARGET* pTaskUnitTarget = &pTask->unitTargets[ i ];
		pTaskUnitTarget->eStatus = pRestoreTarget->eTargetStatus;
		ASSERT( pTaskUnitTarget->eStatus == TASK_UNIT_STATUS_DEAD || pTaskUnitTarget->eStatus == TASK_UNIT_STATUS_ALIVE );
		
	}
	ASSERTX_RETFALSE( pTask->nNumPendingTargets == nTargetCount, "Pending targets not restored properly" );
	
	// if there is a level matching the target level for spawning, and it is active
	// do the spawning logic now
	if ( ( AppIsHellgate() && pTask->nPendingLevelSpawnDefinition != INVALID_LINK) ||
		 ( AppIsTugboat() && pTask->nPendingLevelSpawnDepth != INVALID_LINK ) )
	{	
	
		// setup level type
		LEVEL_TYPE tLevelType;
		tLevelType.nLevelDef = pTask->nPendingLevelSpawnDefinition;
		tLevelType.nLevelArea = pTask->nPendingLevelSpawnArea;
		tLevelType.nLevelDepth = pTask->nPendingLevelSpawnDepth;		
		
		// find one
		LEVEL *pLevel = LevelFindFirstByType( pGame, tLevelType );
		if (pLevel && LevelIsActive(pLevel))
		{
			sDoPendingLevelSpawns( pGame, pTask, pLevel );
		}
		
	}

	// restore extermination/infestation settings
	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
	{
		pTask->tTaskTemplate.tExterminate[ i ] = tTaskExterminate[ i ];
	}
	pTask->tTaskTemplate.nExterminatedCount = nExterminatedCount;
	if( AppIsTugboat() )
	{
		pTask->tTaskTemplate.nExperienceReward = pTask->nRewardExperience;
		pTask->tTaskTemplate.nGoldReward = pTask->nRewardGold;
	}
	// restore faction rewards
	pTask->tTaskTemplate.tFactionRewards = tFactionRewards;
			
	// send task to player
	sSendRestoredTaskToPlayer( pGame, pTask, pPlayer );
	
	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDestroyAllTaskRewards( 
	UNIT* pPlayer)
{
	int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS );
	UNIT* pReward = NULL;
	while ((pReward = UnitInventoryLocationIterate( pPlayer, nInventorySlot, NULL )) != NULL)
	{
		UnitFree( pReward, UFF_SEND_TO_CLIENTS );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sSTaskReadFromPlayer(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );	
	// TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );

	// read task save version
	int nVersion = UnitGetStat( pPlayer, STATS_SAVE_TASK_VERSION );
	if (nVersion != TASK_SAVE_VERSION)
	{
		// destroy all task rewards in the players inventory
		sDestroyAllTaskRewards( pPlayer );
		return;
	}
	
	// read task count
	int nTaskCount = UnitGetStat( pPlayer, STATS_SAVE_TASK_COUNT );
		
	// restore each task
	BOOL bAllRestored = TRUE;
	DWORD dwRewardTouched = 0;
	for (int i = 0; i < nTaskCount; ++i)
	{
		if (sRestoreTask( pPlayer, i, dwRewardTouched ) == FALSE)
		{
			bAllRestored = FALSE;
		}
	}

	// if not all properly restored, remove them all
	if (bAllRestored == FALSE)
	{
		s_TaskAbanbdonAll( pPlayer );
	}
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskPlayerJoinGame(
	UNIT* pPlayer)
{
	REF(pPlayer);

	// restore this players tasks
	sSTaskReadFromPlayer( pPlayer );
	
	// other join logic here ... possibly insert player into other tasks that
	// might be in the world where appropriate
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sModRewards( 
	GAME* pGame, 
	TASK* pTask, 
	UNIT* pMod,
	UNIT** pRewardsTaken,
	int nNumRewards)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pTask, "Expected task" );
	ASSERTX_RETURN( pMod && UnitIsA( pMod, UNITTYPE_MOD ), "Expected mod" );
	
	// go through each reward
	for (int i = 0; i < nNumRewards; ++i)
	{
		UNIT* pReward = pRewardsTaken[ i ];
		ASSERTX_CONTINUE( pReward, "Cannot find reward taken for task" );
		
		// can this reward take this mod
		if (ItemModIsAllowedOn( pMod, pReward ))
		{
			ASSERTX( s_ItemModAdd( pReward, pMod, TRUE ), "Unable to add mod to task reward" );				
		}
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTakeCollectItems( 
	GAME* pGame, 
	TASK* pTask,
	UNIT** pRewardsTaken,
	int nNumRewards)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pTask, "Expected task" );
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// go through all the collect items
	for (int i = 0; i < pTask->nNumCollect; ++i)
	{
		TASK_COLLECT* pCollect = &pTask->tTaskCollect[ i ];
		
		// go through each items in of this collect type
		for (int j = 0; j < pCollect->nCount; ++j)
		{
		
			// find one of these items in the player inventory
			UNIT* pItem = NULL;
			while ((pItem = UnitInventoryIterate( pPlayer, pItem )) != NULL)
			{
				if (UnitGetGenus( pItem ) == GENUS_ITEM &&
					UnitGetClass( pItem ) == pCollect->nItemClass)
				{
					if( AppIsTugboat() )
					{
						// make sure we aren't trying to 'collect' one of our rewards! Yikes!
						int location, x, y;
						UnitGetInventoryLocation( pItem, &location, &x, &y );
						if( location != GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS ) )
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
				
			}
			ASSERTX_CONTINUE( pItem, "Unable to find collect item to take for task" );

			// remove from inventory
			UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_SINGLE );
			
			// if we're supposed to add to reward items, do so now, otherwise destroy item
			if (pTask->pTaskDefinition->bCollectModdedToRewards && UnitIsA( pRemoved, UNITTYPE_MOD ))
			{
				sModRewards( pGame, pTask, pRemoved, pRewardsTaken, nNumRewards );
			}
			else
			{
				UnitFree( pRemoved, 0 );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnlinkRewardItem(
	UNIT* pItem,
	TASK* pTask)
{

	// unlink this item
	UnitSetStat( pItem, STATS_TASK_REWARD, INVALID_ID );
	
	// unlink any inventory
	UNIT* pContained = NULL;
	while ((pContained = UnitInventoryIterate( pItem, pContained )) != NULL)
	{
		sUnlinkRewardItem( pContained, pTask );
	}

	// find reward in task and remove (if found)
	UNITID idItem = UnitGetId( pItem );
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
		if (pTask->idRewards[ i ] == idItem)
		{
			pTask->idRewards[ i ] = INVALID_ID;
		}
	}

	// tell the client the item has been unlinked
	// setup message
	MSG_SCMD_TASK_REWARD_TAKEN tMessage;
	tMessage.idTask = pTask->idTask;
	tMessage.idReward = UnitGetId( pItem );
	
	// send
	GAME* pGame = pTask->pGame;
	UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TASK_REWARD_TAKEN, &tMessage );	

	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnlinkRewards(
	TASK* pTask,
	UNIT** pUnitRewardsTaken,
	int nNumRewards)
{

	// unlink each reward
	for (int i = 0; i < nNumRewards; ++i)
	{
		UNIT* pItem = pUnitRewardsTaken[ i ];
		if( UnitGetStat( pItem , STATS_TASK_REWARD ) == pTask->idTask )
		{
			sUnlinkRewardItem( pItem, pTask );
		}
		
	}

}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMoveRewardToRegularInventory(
	UNIT *pPlayer,
	UNIT *pReward)
{	
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pReward, "Expected reward" );	

	// reward must be in reward slot
	int nInvSlot, nX, nY;
	UnitGetInventoryLocation( pReward, &nInvSlot, &nX, &nY );
	ASSERTX_RETURN( nInvSlot == GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS ), "Reward is not in reward inventory slot" );

	// handle auto stacking
	if (ItemAutoStack( pPlayer, pReward ) == pReward)
	{
	
		// not auto stacked, find a spot for it
		if (!ItemGetOpenLocation( pPlayer, pReward, TRUE, &nInvSlot, &nX, &nY ))
		{
			ASSERTX_RETURN( 0, "Unable to move task reward to regular inventory" );
		}

		// change location to that new spot
		InventoryChangeLocation( pPlayer, pReward, nInvSlot, nX, nY, 0 );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskItemTakenCallback(
	TAKE_ITEM &tTakeItem)
{
	GAMETASKID idTask = (GAMETASKID)CAST_PTR_TO_INT(tTakeItem.pUserData);
	UNIT *pUnit = tTakeItem.pTaker;
	UNIT *pItem = tTakeItem.pItem;
	GAME *pGame = UnitGetGame( pUnit );
	
	// find the task
	TASK *pTask = sTaskFind( pGame, idTask, pUnit );
	ASSERTX_RETURN( pTask, "Expected task when closing reward ui" );	
	const TASK_DEFINITION *pTaskDef = pTask->pTaskDefinition;
	
	// unlink the item from task
	sUnlinkRewardItem( pItem, pTask );

	// mark that we've taken a reward
	TASK_TEMPLATE *pTaskTemplate = &pTask->tTaskTemplate;
	pTaskTemplate->nNumRewardsTaken++;

	// if we've hit the max picks allowed for a reward from this task,
	// destroy any items they player did not choose
	if (pTaskDef->nNumRewardTakes != REWARD_TAKE_ALL &&
		pTaskTemplate->nNumRewardsTaken >= pTaskDef->nNumRewardTakes)
	{
		sTaskRewardsDestroy( pGame, pTask );
	}
	
	// if no more rewards to take, close the task
	int nRewardLocation = InventoryGetRewardLocation( pUnit );	
	if (UnitInventoryGetCount( pUnit, nRewardLocation ) == 0)
	{
		// all reward items were taken, close out this task
		sTaskClose( pGame, pTask );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetTaskRewards(
	TASK *pTask,
	UNIT **pRewards)
{				
	GAME *pGame = pTask->pGame;
	
	int nNumRewards = 0;
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{
		UNITID idReward = pTask->idRewards[ i ];
		if (idReward != INVALID_ID)
		{
			pRewards[ nNumRewards++ ] = UnitGetById( pGame, idReward );
		}
	}
	
	return nNumRewards;
	
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskAcceptReward(
	UNIT* pPlayer,
	GAMETASKID idTask)
{
	GAME* pGame = UnitGetGame( pPlayer );
	TASK* pTask = sTaskFind( pGame, idTask, pPlayer );
	if (pTask)
	{
		const TASK_DEFINITION *pTaskDef = pTask->pTaskDefinition;
		
		// get the task giver the player is talking to
		UNIT *pTaskGiver = s_TalkGetTalkingTo( pPlayer );

		// how many rewards has the player taken from this task reward
		int nNumTaken = UnitGetStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask );
				
		// setup trade actions
		OFFER_ACTIONS tActions;
		tActions.pfnItemTaken = sTaskItemTakenCallback;
		tActions.pItemTakenUserData = CAST_TO_VOIDPTR( idTask );
	
		// task in completed state
		if (pTask->eStatus == TASK_STATUS_COMPLETE)
		{
			if( AppIsHellgate() )
			{
				// change status to reward
				pTask->eStatus = TASK_STATUS_REWARD;
				sTaskSendStatus( pGame, pTask );
				// get array of rewards
				UNIT* pRewards[ MAX_TASK_REWARDS ];						
				int nNumRewards = sGetTaskRewards( pTask, pRewards );
			
				// take the collect items from the player (and possibly mod to rewards)
				sTakeCollectItems( pGame, pTask, pRewards, nNumRewards );

				// give faction rewards
				const FACTION_REWARDS *pFactionRewards = &pTask->tTaskTemplate.tFactionRewards;
				for (int i = 0; i < pFactionRewards->nCount; ++i)
				{
					int nFaction = pFactionRewards->tReward[ i ].nFaction;
					int nPoints = pFactionRewards->tReward[ i ].nPoints;
					FactionScoreChange( pPlayer, nFaction, nPoints );
				}
				
				// open the reward interface
				s_RewardOpen( 
					STYLE_REWARD,
					pPlayer, 
					pTaskGiver,
					pRewards, 
					nNumRewards, 
					pTaskDef->nNumRewardTakes,
					nNumTaken,
					INVALID_LINK,
					tActions);
			}
			else	//tugboat
			{
				UNIT* pPlayer = UnitGetById( pGame, pTask->idPlayer );
	
				// build array of rewards
				int nNumRewards = 0;
				UNIT* pRewards[ MAX_TASK_REWARDS ];						
				for (int i = 0; i < MAX_TASK_REWARDS; ++i)
				{					
					UNITID idReward = pTask->idRewards[ i ];
					if (idReward != INVALID_ID &&
						UnitGetStat(  UnitGetById( pGame,idReward ) , STATS_TASK_REWARD ) == pTask->idTask )
					{
						pRewards[ nNumRewards++ ] = UnitGetById( pGame, idReward );
					}
				}
	
				s_UnitGiveExperience( pPlayer, pTask->nRewardExperience );
				UnitAddCurrency( pPlayer, cCurrency( pTask->nRewardGold, 0 ) );
				
				// can the player pickup all the reward objects
				if (UnitInventoryCanPickupAll( pPlayer, pRewards, nNumRewards ))
				{
					// take the collect items from the player (and possibly mod to rewards)
					sTakeCollectItems( pGame, pTask, pRewards, nNumRewards );
	
					// remove link to task from reward items
					sUnlinkRewards( pTask, pRewards, nNumRewards );
					
					// move reward item from reward inventory to regular inventory				
					for (int i = 0; i < nNumRewards; ++i)
					{
						UNIT* pReward = pRewards[ i ];
						sMoveRewardToRegularInventory( pPlayer, pReward );
					}
						
					// give faction rewards
					const FACTION_REWARDS *pFactionRewards = &pTask->tTaskTemplate.tFactionRewards;
					for (int i = 0; i < pFactionRewards->nCount; ++i)
					{
						int nFaction = pFactionRewards->tReward[ i ].nFaction;
						int nPoints = pFactionRewards->tReward[ i ].nPoints;
						FactionScoreChange( pPlayer, nFaction, nPoints );
					}
						
					// all reward items were taken, close out this task
					sTaskClose( pGame, pTask );
	
				}
				else	//can't pick up
				{
					// take the collect items from the player (and possibly mod to rewards)
					sTakeCollectItems( pGame, pTask, pRewards, nNumRewards );
		
					// remove link to task from reward items
					sUnlinkRewards( pTask, pRewards, nNumRewards );
					
					// pickup all rewards
					for (int i = 0; i < nNumRewards; ++i)
					{
						UNIT* pReward = pRewards[ i ];
						
						// pickup item (note that this may absorbe the pReward into an item stack
						//UNIT* pPickedUp = ItemDoPickup( pPlayer, pReward );
						UnitUpdateRoom( pReward, UnitGetRoom( pPlayer ), TRUE );
						pReward->vPosition = pPlayer->vPosition;
						UnitDrop( pGame, pReward, pPlayer->vPosition );
						// sanity
						//ASSERTX( UnitIsContainedBy( pPickedUp, pPlayer ), "Reward not actually picked up by player" );
						
					}
						
					// give faction rewards
					const FACTION_REWARDS *pFactionRewards = &pTask->tTaskTemplate.tFactionRewards;
					for (int i = 0; i < pFactionRewards->nCount; ++i)
					{
						int nFaction = pFactionRewards->tReward[ i ].nFaction;
						int nPoints = pFactionRewards->tReward[ i ].nPoints;
						FactionScoreChange( pPlayer, nFaction, nPoints );
					}
					// all reward items were taken, close out this task
					sTaskClose( pGame, pTask );

				}
			}												
		}
		else if ( AppIsHellgate() && pTask->eStatus == TASK_STATUS_REWARD)
		{

			// build array of rewards
			UNIT* pRewards[ MAX_TASK_REWARDS ];						
			int nNumRewards = sGetTaskRewards( pTask, pRewards );
			
			// open the reward interface
			s_RewardOpen(
				STYLE_REWARD, 
				pPlayer, 
				pTaskGiver,
				pRewards, 
				nNumRewards,
				pTaskDef->nNumRewardTakes,
				nNumTaken,
				INVALID_LINK,
				tActions);
		}
	}
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventMonsterKill(
	UNIT* pKiller,
	UNIT* pVictim)
{

	// get the ultimate killer (if any)
	UNITID idUltimateKiller = INVALID_ID;
	if (pKiller)
	{
		UNIT *pUltimateKiller = UnitGetUltimateOwner( pKiller );
		idUltimateKiller = UnitGetId( pUltimateKiller );
	}
	
	// setup message
	TASK_MESSAGE_MONSTER_KILL tMessage;
	tMessage.idKiller = idUltimateKiller;
	tMessage.idVictim = UnitGetId( pVictim );

	// send to all tasks	
	GAME* pGame = UnitGetGame( pVictim );
	sSTaskSendMessageAll( pGame, TM_MONSTER_KILL, &tMessage );
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventLevelActivated(
	LEVEL* pLevel)
{
	TASK_MESSAGE_LEVEL_ACTIVATE tMessage;
	tMessage.pLevel = pLevel;
	
	GAME* pGame = LevelGetGame( pLevel );
	sSTaskSendMessageAll( pGame, TM_LEVEL_ACTIVATED, &tMessage );
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventRoomActivated(
	ROOM* pRoom)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	GAME *pGame = RoomGetGame( pRoom );		
	TASK_MESSAGE_ROOM_ACTIVATE tMessage;
	tMessage.pRoom = pRoom;
	sSTaskSendMessageAll( pGame, TM_ROOM_ACTIVATED, &tMessage );
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventRoomEntered(
	GAME* pGame,
	UNIT * pPlayer,
	ROOM* pRoom)
{
	TASK_MESSAGE_ROOM_ENTER tMessage;
	tMessage.pRoom = pRoom;
	tMessage.pPlayer = pPlayer;
	sSTaskSendMessageAll( pGame, TM_ROOM_ENTERED, &tMessage );
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventInventoryAdd(
	UNIT* pPlayer,
	UNIT* pAdded)
{
	TASK_MESSAGE_INVENTORY_ADD tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pAdded = pAdded;
	
	GAME* pGame = UnitGetGame( pPlayer );
	sSTaskSendMessageWithPlayer( pGame, pPlayer, TM_INVENTORY_ADD, &tMessage );
	
}	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskEventInventoryRemove(
	UNIT* pPlayer,
	UNIT* pRemoved)
{
	TASK_MESSAGE_INVENTORY_REMOVE tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pRemoved = pRemoved;
	
	GAME* pGame = UnitGetGame( pPlayer );
	sSTaskSendMessageWithPlayer( pGame, pPlayer, TM_INVENTORY_REMOVE, &tMessage );
	
}	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
void Cheat_s_TaskKillAllTargets(
	GAME* pGame)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	// go through all tasks
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		for (int i = 0; i < pTask->nNumUnitTargets; ++i)
		{
			TASK_UNIT_TARGET* pTarget = &pTask->unitTargets[ i ];
			if (pTarget->eStatus == TASK_UNIT_STATUS_ALIVE)
			{
				UNIT* pUnitTarget = UnitGetById( pGame, pTarget->idTarget );
				if (pUnitTarget)
				{
					UnitDie( pUnitTarget, NULL );
				}
			}
		}
	}	
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
void Cheat_s_TaskKillTarget(
	GAME* pGame,
	int nIndex)
{
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	// go through all tasks
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
		TASK_UNIT_TARGET* pTarget = &pTask->unitTargets[ nIndex ];
		if (pTarget->eStatus == TASK_UNIT_STATUS_ALIVE)
		{
			UNIT* pUnitTarget = UnitGetById( pGame, pTarget->idTarget );
			if (pUnitTarget)
			{
				UnitDie( pUnitTarget, NULL );
				ConsoleString( CONSOLE_SYSTEM_COLOR, "Killed '%s'", UnitGetDevName( pUnitTarget ) );
			}
		}
	}	
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
void Cheat_s_TaskCompleteAll(
	UNIT *pPlayer)
{
	GAME* pGame = UnitGetGame( pPlayer );
	UNITID idPlayer = UnitGetId( pPlayer );
	TASK_GLOBALS* pTaskGlobals = sTaskGlobalsGet( pGame );
	
	// go through all tasks for this player, and cancel them all
	for (TASK* pTask = pTaskGlobals->pTaskList; pTask; pTask = pTask->pNext)
	{
				
		// is match
		if (pTask->idPlayer == idPlayer)
		{
			// complete it
			sTaskCompleted( pGame, pTask );
		}
		
	}
	
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TaskRewardItemTaken(
	UNIT *pPlayer,
	UNIT *pItem)
{	
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pItem, "Expected item" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// do task reward tracking
	GAMETASKID idTask = UnitGetStat( pItem, STATS_TASK_REWARD );
	TASK *pTask = sTaskFind( pGame, idTask, pPlayer );
	int nNumTaken = UnitGetStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask );		
	UnitSetStat( pPlayer, STATS_TASK_REWARD_NUM_ITEMS_TAKEN, pTask->idTask, nNumTaken + 1);
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
