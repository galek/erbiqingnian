//----------------------------------------------------------------------------
// FILE: Quest.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "Quest.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "QuestObjectSpawn.h"
#include "QuestMonsterSpawn.h"
#include "unittag.h"
#include "script.h"
#include "script_priv.h"
#include "unittypes.h"
#include "s_QuestServer.h"
#include "c_QuestClient.h"
#include "s_quests.h"
#include "excel_private.h"
#include "skills.h"
#include "Player.h"
#include "QuestProperties.h"
/*
This to do:
1) DONE - Make a state for quest complete (done ) and a state for quest task complete. Call when they are complete.
2) DONE - Add sound effects and particles for complete states
3) DONE - Make levels look at the quests for themes
4) DON'T NEED - Make quest be able to rename dungeons and reset number of levels.
5) Decide if we want people to conitinue to get XP for bosses even after they killed it 
6) DONE - Make dungeon based off of when Quest was given.
7) Be able to set states on objects when an even occurs
*/


//lets clean things up.
void s_QuestForTugboatFree(
	GAME *pGame )
{
	
}




// sets the unit as a Quest Unit
void s_SetUnitAsQuestUnit( UNIT *unit,			
								   UNIT *pClient,
								   const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
								   int nIndex )
{	
	//must be server
	if( IS_CLIENT( UnitGetGame( unit ) ) )
		return;
	ASSERT_RETURN(pQuestTask);
	UnitSetStat( unit, STATS_QUEST_TASK_REF, pQuestTask->nTaskIndexIntoTable );
	UnitSetStat( unit, STATS_QUEST_ID_REF, nIndex );
	if( pClient )
	{
		PGUID guidUnit = (pClient ? UnitGetGuid(pClient) : INVALID_GUID);
		UnitAddPickupTag( UnitGetGame( unit ), unit, guidUnit, INVALID_GUID );
	}
}

// returns if the unit has been created or modified by the quest system
inline BOOL QuestUnitIsQuestUnit( UNIT *unit )
{
	return ( UnitGetStat( unit, STATS_QUEST_TASK_REF ) != -1);
}

BOOL isQuestRandom( const QUEST_DEFINITION_TUGBOAT *pQuest )
{
	ASSERT_RETFALSE( pQuest );
	return ( pQuest->nQuestRandomID == INVALID_ID )?FALSE:TRUE;
}

BOOL isQuestRandom( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pQuestTask );
	return isQuestRandom( QuestDefinitionGet( pQuestTask->nParentQuest ) );	
}



// returns if an item can be dropped by the player
BOOL QuestUnitCanDropQuestItem( UNIT *pItem )
{
	if( !pItem ||
		QuestUnitIsUnitType_QuestItem( pItem ) == FALSE ||
		QuestUnitIsQuestUnit( pItem ) == FALSE )
		return TRUE;
	UNIT *pPlayer = UnitGetUltimateOwner( pItem );
	if( !pPlayer ||
		!UnitIsPlayer( pPlayer ) )
		return TRUE;
	//check to see if it's a quest map. Maps work a bit different. The can drop only if the active task is done.
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pItem );
	if( pQuestTask )
	{
		if( QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ) )
		{
			return TRUE;	// Quest is complete we can drop it
		}
		//if it's an active quest we can't drop it
		if( QuestPlayerHasTask( pPlayer, pQuestTask ) != INVALID_ID )
			return FALSE;
	}
	return TRUE;
}

// the number of active quests for this player
int QuestGetNumOfActiveQuests( GAME *pGame,
						  UNIT *pPlayer )
{
	int Count = 0;
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		if( MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, i ) != INVALID_ID )
		{
			Count++;
		}
	}
	return Count;
}


//this returns if the unit has the quest.
BOOL QuestUnitHasQuest( UNIT *unit,
					    int questID )
{	
	return (MYTHOS_QUESTS::QuestGetQuestQueueIndex( unit, questID ) != INVALID_ID)?TRUE:FALSE;
}


//this returns the mask for the Quest's Task
int QuestGetMaskForTask( QUEST_MASK_TYPES type,
								const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	if( !questTask ) return 0;	
	return 1 << (( questTask->nQuestMaskLocal * QUEST_MASK_COUNT ) + type );
}
//this returns if the mask is set on the players stat
BOOL QuestIsMaskSet( QUEST_MASK_TYPES type,
						    UNIT *pPlayer,
							const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	if( !questTask ) return FALSE;		
	switch( type )
	{
	case QUEST_MASK_COMPLETED:
		if( QuestIsQuestTaskComplete( pPlayer, questTask ) )
			return TRUE;
		break;
	}
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, questTask );
	//this can happen when the player doesn't have the quest yet. Which means it return FALSE!
	if( nQuestQueueIndex == INVALID_ID )
	{
		return FALSE;
	}	
	return ( UnitGetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex ) & QuestGetMaskForTask( type, questTask ) );
}





				   



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestIndexGet(
	UNIT *pPlayer,
	int nQuestID)
{
	if ((unsigned int)nQuestID >= ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_QUEST_TITLES_FOR_TUGBOAT))
	{
		return INVALID_INDEX;
	}
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		if( UnitGetStat( pPlayer, STATS_QUEST_PLAYER_INDEXING, t ) == nQuestID )
			return t;
		//if( UnitGetStat( pPlayer, g_QuestStatIDs[ t ] ) == nQuestID )
		//	return t;
	}
	return INVALID_INDEX;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestTaskGetMaskValue( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID(pQuestTask);
	int nIndex = QuestIndexGet( pPlayer, pQuestTask->nParentQuest );
	ASSERTX_RETVAL( nIndex != INVALID_INDEX, INVALID_ID, "Trying to get Quest Task save mask passed back invalid index." );
	//return g_QuestTaskSaveIDs[ nIndex ];
	return UnitGetStat( pPlayer, STATS_QUEST_TASK_PLAYER_INDEXING, nIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_DEFINITION_TUGBOAT *QuestDefinitionGet( int nQuestIndex)
{
	if ((unsigned int)nQuestIndex >= ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_QUEST_TITLES_FOR_TUGBOAT))
	{
		return NULL;
	}
	return (const QUEST_DEFINITION_TUGBOAT*)ExcelGetData( NULL, DATATABLE_QUEST_TITLES_FOR_TUGBOAT, nQuestIndex );
}	

const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet( int nQuestTaskIndex )
{
	return (const QUEST_TASK_DEFINITION_TUGBOAT *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, nQuestTaskIndex );
}

const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet(	
	const char *name )
{
	return (const QUEST_TASK_DEFINITION_TUGBOAT *)ExcelGetDataByStringIndex(EXCEL_CONTEXT(), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, name );
}


const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet( DWORD dwCode )
{
	if( dwCode == 0 )
		return NULL;
	int index = ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, dwCode);
	if( index == INVALID_ID )
		return NULL;
	return QuestTaskDefinitionGet( index );
}

const QUEST_TASK_DEFINITION_TUGBOAT *QuestUnitGetTask( UNIT *pUnit )
{
	if( !QuestUnitIsQuestUnit( pUnit ) )
		return NULL;
	return QuestTaskDefinitionGet( UnitGetStat( pUnit, STATS_QUEST_TASK_REF ) );
}
													   

inline void sQuestTaskFillDungeons( QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask )
{
	ASSERT_RETURN(pQuestTask);
	//invalidate the dungeons that are visible
	for( int t = 0; t < MAX_DUNGEONS_VISIBLE; t++ )
		pQuestTask->nQuestTaskDungeonsVisible[ t ] = INVALID_ID;
	
	int count( 0 );
	
	for( int t = 0; t < MAX_MONSTERS_TO_SPAWN; t++ )
	{
		if( pQuestTask->nQuestDungeonsToSpawnMonsters[ t ] != INVALID_LINK )
		{
			pQuestTask->nQuestTaskDungeonsVisible[ count++ ] = pQuestTask->nQuestDungeonsToSpawnMonsters[ t ];
		}
	}
	for( int t = 0; t < MAX_OBJECTS_TO_SPAWN; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( NULL, pQuestTask, t ) != INVALID_LINK )
		{
			pQuestTask->nQuestTaskDungeonsVisible[ count++ ] = MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( NULL, pQuestTask, t );
		}
	}
	for( int t = 0; t < MAX_OBJECTS_TO_SPAWN; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( NULL, pQuestTask, t ) != INVALID_LINK )
		{
			pQuestTask->nQuestTaskDungeonsVisible[ count++ ] = MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( NULL, pQuestTask, t );
		}

	}
	
}

inline int GetCountOfValidArray( const int *nArray, const int arraySize )
{
	int count( 0 );
	BOOL done( FALSE );
	for( int i = 0; i < arraySize; i++ )
	{
		if( nArray[ i ] != INVALID_ID )
		{
			ASSERTX_RETVAL( !done, count, "An array linking to another table found an invalid id" );
			count++;
		}
		else
		{
			done = TRUE;
		}

	}
	return count;
}

BOOL QuestCountForTugboatExcelPostProcess( struct EXCEL_TABLE * table )
{
	ASSERT_RETFALSE(table->m_Index == DATATABLE_QUEST_COUNT_TUGBOAT);
	MAX_ACTIVE_QUESTS_PER_UNIT = ExcelGetCountPrivate(table);
	return TRUE;
}

BOOL QuestRandomForTugboatExcelPostProcess( struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Index == DATATABLE_QUEST_RANDOM_TASKS_FOR_TUGBOAT);
	unsigned int nNumOfTasks = ExcelGetCountPrivate(table);
	ASSERTX_RETFALSE(nNumOfTasks != 0, "Number of random tasks was zero");
	
	//first things first. Link all quest task
	for(unsigned int t = 0; t < nNumOfTasks; t++)
	{
		QUEST_RANDOM_TASK_DEFINITION_TUGBOAT * pRandomQuestTask = (QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(table, t);		
		ASSERT_RETFALSE(pRandomQuestTask);
		pRandomQuestTask->nCollectItemFromSpawnClassCount = GetCountOfValidArray( pRandomQuestTask->nCollectItemFromSpawnClass, KQUEST_RANDOM_TASK_STAT_SPAWN_CLASS_COUNT );
		pRandomQuestTask->nBossSpawnClassesCount = GetCountOfValidArray( pRandomQuestTask->nBossSpawnClasses, KQUEST_RANDOM_TASK_STAT_SPAWN_CLASS_COUNT );
		pRandomQuestTask->nRewardOfferingsCount = GetCountOfValidArray( pRandomQuestTask->nRewardOfferings, KQUEST_RANDOM_TASK_STAT_REWARD_COUNT );
		pRandomQuestTask->nLevelAreasCount = GetCountOfValidArray( pRandomQuestTask->nLevelAreas, KQUEST_RANDOM_TASK_STAT_LEVELAREAS_COUNT );
		pRandomQuestTask->nItemsToCollectCount = GetCountOfValidArray( pRandomQuestTask->nItemsToCollect, KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT );
		for( int t = 0; t < 2; t++ )
		{
			pRandomQuestTask->nGoldPCT[ t ] = ( pRandomQuestTask->nGoldPCT[ t ] <= 0 )?100:pRandomQuestTask->nGoldPCT[ t ];
			pRandomQuestTask->nXPPCT[ t ] = ( pRandomQuestTask->nXPPCT[ t ] <= 0 )?100:pRandomQuestTask->nXPPCT[ t ];
		}
		for( int t = 0; t < KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_COUNT; t++ )
		{
			BOOL bDone( FALSE );
			for( int c = 0; c < KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_SIZE; c++ )
			{
				if( !bDone && pRandomQuestTask->nItemSpawnFromGroupOfUnitTypes[ t ][ c ] == INVALID_ID )
				{
					pRandomQuestTask->nItemSpawnFromGroupOfUnitTypesCount[ t ] = c;
					bDone = TRUE;
				}
				else if( bDone )
				{
					ASSERTX( bDone && pRandomQuestTask->nItemSpawnFromGroupOfUnitTypes[ t ][ c ] == INVALID_ID, "Trying to count Random Item Unit TYpe Spawn Count. One of the items is invalid." );
				}	
			}
			if( !bDone )
			{
				pRandomQuestTask->nItemSpawnFromGroupOfUnitTypesCount[ t ] = KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_SIZE;
			}
		}

	}	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestForTugboatExcelPostProcess( 
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Index == DATATABLE_QUESTS_TASKS_FOR_TUGBOAT);

	unsigned int nNumOfTasks = ExcelGetCountPrivate(table);
	ASSERTX_RETFALSE(nNumOfTasks != 0, "Number of tasks was null");

	//first things first. Link all quest task
	for(unsigned int t = 0; t < nNumOfTasks; t++)
	{
		QUEST_TASK_DEFINITION_TUGBOAT * pQuestTaskDefinition = (QUEST_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(table, t);		
		ASSERT_RETFALSE(pQuestTaskDefinition);
		sQuestTaskFillDungeons( pQuestTaskDefinition );
		pQuestTaskDefinition->nTaskIndexIntoTable = t;
		pQuestTaskDefinition->pNext = NULL;				
		//See if we have any themes for this task
		pQuestTaskDefinition->m_bHasThemes = FALSE;
		for( int ii = 0; ii < MAX_THEMES_PER_LEVEL; ii++ )
		{
			if( pQuestTaskDefinition->m_QuestThemes.nQuestDungeonThemes[ ii ] != -1 )
			{
				pQuestTaskDefinition->m_bHasThemes = TRUE;
			}
		}
		//now we need to look for the next task			
		for (unsigned int y = 0; y < nNumOfTasks; y++ )
		{
			QUEST_TASK_DEFINITION_TUGBOAT * pQuestTaskDefinitionNext = (QUEST_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(table, y);
			ASSERT_CONTINUE(pQuestTaskDefinitionNext);
			//lets do some error checking...
			
			//this is not really needed anymore.
			for( int e = 0; e < MAX_COLLECT_PER_QUEST_TASK; e++ )
			{
				if( pQuestTaskDefinition->nQuestItemsIDToCollect[ e ] == INVALID_ID &&
					pQuestTaskDefinition->nQuestItemsNumberToCollect[ e ] > 0 )
				{
					pQuestTaskDefinition->nQuestItemsNumberToCollect[ e ] = 0;
					//ASSERTV_MSG("Error in Quest System. Item was not found for quest %i, task %i", pQuestTaskDefinition->nParentQuest, y);
				}
			}
			
			if( y != t &&  //we aren't the same task
				pQuestTaskDefinition->nParentQuest == pQuestTaskDefinitionNext->nParentQuest && //same parent
				pQuestTaskDefinition->nQuestMaskLocal + 1 == pQuestTaskDefinitionNext->nQuestMaskLocal ) //the next quest
			{
				pQuestTaskDefinition->pNext = pQuestTaskDefinitionNext; //we need to make links
				pQuestTaskDefinitionNext->pPrev = pQuestTaskDefinition; //we need to make links
				break; //break out of for loop for y
			}
		}
	}
	//end linking

	//now set the quest definitions to point to the first task in the quest series.
	EXCEL_TABLE * tableQuestTitles = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_TITLES_FOR_TUGBOAT);
	ASSERT_RETFALSE(tableQuestTitles);
	for (unsigned int i = 0; i < ExcelGetCountPrivate(tableQuestTitles); ++i)
	{
		//Quest definition - top root of quests
		QUEST_DEFINITION_TUGBOAT * pQuestDefinition = (QUEST_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(tableQuestTitles, i);
		ASSERT_RETFALSE(pQuestDefinition);
		pQuestDefinition->nID = i;	

		//Number of tasks for all quests		
		for(unsigned int t = 0; t < nNumOfTasks; t++)
		{
			QUEST_TASK_DEFINITION_TUGBOAT * pQuestTaskDefinition = (QUEST_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(table, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, t );
			ASSERT_RETFALSE(pQuestTaskDefinition);

			//if the quest tasks parent is equal to our Quest, then
			//we need to see if  the task is the first task. If so, then
			//set the quest to point to the first task it start with.
			if( !pQuestDefinition->pFirst && //we already have the first itme, so don't worry about it
				(unsigned int)pQuestTaskDefinition->nParentQuest == i &&  //make sure it's the parent item
				pQuestTaskDefinition->nQuestMaskLocal == 0 )
			{
				pQuestDefinition->pFirst = pQuestTaskDefinition;
				break; //break out of the for loop
			}
		}
		//now we need to create the mask for the quest to be complete
		const QUEST_TASK_DEFINITION_TUGBOAT *nextTask = pQuestDefinition->pFirst;
		while( nextTask )
		{
			pQuestDefinition->nMaskForCompletion = pQuestDefinition->nMaskForCompletion | QuestGetMaskForTask( QUEST_MASK_COMPLETED, nextTask );
			nextTask = nextTask->pNext;
		}

		//figure out the rewards
		if( pQuestDefinition->pFirst )
		{
			pQuestDefinition->nQuestRewardXP = MYTHOS_QUESTS::QuestGetExperienceEarned( NULL, pQuestDefinition->pFirst );
			pQuestDefinition->nQuestRewardGold = MYTHOS_QUESTS::QuestGetGoldEarned( NULL, pQuestDefinition->pFirst );	
		}
		else
		{
			pQuestDefinition->nQuestRewardXP = -1;
			pQuestDefinition->nQuestRewardGold = -1;
		}
	}

	//lets do random quests now	
	EXCEL_TABLE * tableQuestRandom = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_RANDOM_FOR_TUGBOAT);
	EXCEL_TABLE * tableQuestTaskRandom = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_RANDOM_TASKS_FOR_TUGBOAT);
	ASSERT_RETFALSE(tableQuestRandom);
	ASSERT_RETFALSE(tableQuestTaskRandom);
	for (unsigned int i = 0; i < ExcelGetCountPrivate(tableQuestRandom); ++i)
	{
		//Quest definition - top root of quests
		QUEST_RANDOM_DEFINITION_TUGBOAT * pQuestRandomDefinition = (QUEST_RANDOM_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(tableQuestRandom, i);
		ASSERT_RETFALSE(pQuestRandomDefinition);
		pQuestRandomDefinition->nId = i;		
		//Number of random tasks for the quests - since we don't care about order here this is much easier then the normal quest tasks
		for(unsigned int t = 0; t < ExcelGetCountPrivate(tableQuestTaskRandom); t++)
		{
			QUEST_RANDOM_TASK_DEFINITION_TUGBOAT * pQuestRandomTask = (QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(tableQuestTaskRandom, t);	
			ASSERT_RETFALSE(pQuestRandomTask);						
			if( pQuestRandomTask->nParentID == pQuestRandomDefinition->nId )			
			{
				pQuestRandomTask->nId = t;				
				if( pQuestRandomDefinition->pFirst == NULL )
				{
					pQuestRandomDefinition->pFirst = pQuestRandomTask;					
				}
				else
				{					
					const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pConstChild = pQuestRandomDefinition->pFirst;
					while( pConstChild->pNext != NULL )
					{
						pConstChild = pConstChild->pNext;
					}
					//we need to get the random task as a none-const variable
					QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pChild = (QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *)ExcelGetDataPrivate(tableQuestTaskRandom, pConstChild->nId);	
					pQuestRandomTask->pPrev = pChild;
					pChild->pNext = pQuestRandomTask;
				}
				pQuestRandomDefinition->nRandomTaskCount++;
			}
		}
	}
	
	return TRUE;
}




//returns if the quest is complete or not
BOOL QuestIsQuestComplete( UNIT *pPlayer,
						   int questId )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( questId >= 0 );
	//check to see if the whole quest is complete....
	if( UnitGetStat( pPlayer, STATS_QUEST_COMPLETED_TUGBOAT, questId ) )
		return TRUE;
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( questId );
	//get the queue index
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, questId );
	if( nQuestQueueIndex == INVALID_ID )
	{
		return FALSE; //we don't have it so we must not of completed it yet
	}
	//this returns the mask to test against if it's complete
	int questTaskMask = UnitGetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex );
	//if the masks match it's complete
	return ( (pQuest->nMaskForCompletion & questTaskMask) == pQuest->nMaskForCompletion );
}

BOOL QuestIsQuestTaskComplete( UNIT *pPlayer,
						       int questTaskId )
{
	return QuestIsQuestTaskComplete( pPlayer, QuestTaskDefinitionGet( questTaskId ) ); 
}

BOOL QuestIsQuestTaskComplete( UNIT *pPlayer,
						       const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pQuestTask );
	if( QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ) ) //if the parent quest is complete then it's complete too.
		return TRUE;
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask );
	if( nQuestQueueIndex == INVALID_ID )
	{
		return FALSE;	//the quest is not complete and we currently don't have the task so it can't be complete.
	}
	ASSERT_RETFALSE( nQuestQueueIndex != INVALID_ID );
	//this returns the mask to test against if it's complete
	int questTaskMask = QuestGetMaskForTask( QUEST_MASK_COMPLETED, pQuestTask );
	//this returns the actual stat value to test against
	int questMask = UnitGetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex );
	return (questMask & questTaskMask )?TRUE:FALSE;
}

//we just need to make sure the quest can start
BOOL QuestTaskCanStart( GAME *pGame,
						UNIT *pPlayer,
						int questId,
						int questStartFlags )
{
	//we can only see if it starts by getting the global quest stat
	const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = QuestGetActiveTask( pPlayer, questId );
	if( activeTask == NULL )
		return FALSE;
	for( int t = 0; t < MAX_GLOBAL_QUESTS_COMPLETED; t++ )
	{
		if( activeTask->nQuestCanStartOnQuestsCom[ t ] != INVALID_ID &&
			!QuestIsQuestComplete( pPlayer, activeTask->nQuestCanStartOnQuestsCom[ t ] ) )
			return FALSE;
	}	

	for( int t = 0; t < MAX_GLOBAL_QUESTS_COMPLETED; t++ )
	{
		//this checks parent quests to see if the parent quest is active. 
		if( activeTask->nQuestCanStartOnQuestActive[ t ] != INVALID_ID &&
			!QuestUnitHasQuest(pPlayer,activeTask->nQuestCanStartOnQuestActive[ t ]))
			return FALSE;
	}


	//lets check to see if we have room to start the quest
	if( ( questStartFlags & QUEST_START_FLAG_IGNORE_ITEM_COUNT ) == 0 )	
	{
		if( QuestCanGiveItemsOnAccept( pPlayer, activeTask ) == FALSE )
			return FALSE;
	}
	return TRUE;
}



//returns the active task for a quest
//could return NULL, if so, the quest should be updated as complete.
const QUEST_TASK_DEFINITION_TUGBOAT * QuestGetActiveTask( UNIT *unit,
														   int questIndex )
{
	const QUEST_DEFINITION_TUGBOAT *activeQuest = QuestDefinitionGet( questIndex );
	if( activeQuest == NULL ) return NULL; //quest not found	
	if( QuestIsQuestComplete( unit, questIndex ) )
		return NULL;	//the quest is already complete
	const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = activeQuest->pFirst;
	if( activeTask == NULL ) return NULL; //task not found - something BROKE.		
	while( activeTask )
	{
		if( !QuestIsMaskSet( QUEST_MASK_COMPLETED, unit, activeTask ) )
			return activeTask; //this task is not complete yet so return it		
		activeTask = activeTask->pNext; //this task must be complete so lets go to the next on
	}
	return NULL; //returning NULL means all the tasks are done and that the quest should be done.
}

//returns true if the task is complete or not

BOOL QuestGetIsTaskComplete( UNIT *unit,	//unit trying to complete quest
							 const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask ) 
{	
	return QuestIsMaskSet( QUEST_MASK_COMPLETED, unit, pQuestTask );
}

//returns if any quest task has tasks to complete. 
BOOL QuestTaskHasTasksToComplete( UNIT *pPlayer,
								  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pQuestTask );
	//lets check inventory items first
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, t ) != INVALID_ID )
			return TRUE; //have to collect somthing or other
	}

	//ok check for if we talk to all the people we are ment to...
	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break; //must be ok
		if( pQuestTask->nQuestNPCTalkToCompleteQuest[ t ] > 0   )
		{
			return TRUE; //we have to talk to this person
		}
	}

	//ok lets check to see if other quests are done
	for( int t = 0; t < MAX_GLOBAL_QUESTS_COMPLETED; t++ )
	{
		if( pQuestTask->nQuestQuestsToFinish[ t ] != INVALID_ID )
			return TRUE;
	}

	for( int i = 0; i < MAX_OBJECTS_TO_SPAWN; i++ )
	{
		int objectSpawn = MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( pQuestTask, i );
		int monsterSpawn = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, i );
		if( objectSpawn != INVALID_ID ||
			monsterSpawn != INVALID_ID )
		{
			return TRUE; //we have to find an item or kill a boss
		}
	}

	//rule if a task isn't first and that task doesn't have an intro dialog and a comeback dialog, just a complete
	//dialog then we are complete. How can this happen? When you talk to one person and have to go do a bunch of tasks
	// and when those tasks are done the quest automatically continues on. You are able at that point to actually
	//talk to a different person. Such as go collect X for person V and return X to person Z.
	if( pQuestTask->pPrev ) //not first quest
	{
		if( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_ACCEPTED ) == INVALID_ID &&
			MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_RETURN ) == INVALID_ID &&
			MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_COMPLETE ) != INVALID_ID)
		{
			return QuestTaskHasTasksToComplete( pPlayer, pQuestTask->pPrev ); //the previous quest
		}
	}

	return FALSE; //nothin' to do here!
}
//---------------------------------------------------------------------
//Validates an item as an item needed in quests
//---------------------------------------------------------------------
BOOL QuestValidateItemForQuestTask( UNIT *pPlayer,
							  UNIT *pItem,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pItem );
	ASSERT_RETFALSE( pQuestTask );
	if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, 0 ) == INVALID_ID )
		return FALSE; //we don't have any items to collect so it failed to validate

	int bExecuteValidation( FALSE );
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		int nClassID = MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, t );
		if( nClassID == INVALID_ID )
			break;
		if( UnitGetClass( pItem ) ==  nClassID )
		{
			bExecuteValidation = TRUE;
			break;
		}			
	}

	if( bExecuteValidation )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( UnitGetGame( pPlayer ), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, pQuestTask->pQuestItemValidationCode, &code_len);				
		if( !code_ptr  )
		{
			return ( VMExecI( UnitGetGame( pPlayer ), pItem, pPlayer, NULL, pQuestTask->nTaskIndexIntoTable, INVALID_LINK, INVALID_LINK, INVALID_SKILL_LEVEL, INVALID_LINK, code_ptr, code_len) == 1 )?TRUE:FALSE;
		}
	}
	return FALSE;
}

inline BOOL sQuestItemsPlayerOwnsAreValidForTaskComplete( UNIT *pPlayer,
												   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pQuestTask );
	if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, 0 ) == INVALID_ID )
		return TRUE; //we don't have any items to collect so it must be fine
	if( pQuestTask->pQuestItemValidationCode == NULL )
		return TRUE; //there is no validation needed

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( UnitGetGame( pPlayer ), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, pQuestTask->pQuestItemValidationCode, &code_len);				
	
	int nItemsValid[ MAX_COLLECT_PER_QUEST_TASK ];	
	ZeroMemory( nItemsValid, sizeof( int ) * MAX_COLLECT_PER_QUEST_TASK );
	DWORD dwInvIterateFlags = 0;
	//SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	//SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );			
	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags );	
	while ( pItem != NULL)
	{		
		int nItemClass = UnitGetClass( pItem );		
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			if( nItemClass == MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, t ) )
			{
				BOOL bValid( TRUE );
				if( code_ptr )				
					bValid = ( VMExecI( UnitGetGame( pPlayer ), pItem, pPlayer, NULL, pQuestTask->nTaskIndexIntoTable, INVALID_LINK, INVALID_LINK, INVALID_SKILL_LEVEL, INVALID_LINK, code_ptr, code_len) != 0 );
				if( bValid )
				{
					nItemsValid[ t ]++;
				}
			}
		}
		pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags );
	}
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{		
		if( MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pPlayer, pQuestTask, t ) != 0 && 
			nItemsValid[ t ] < MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pPlayer, pQuestTask, t ) )
		{
			return FALSE; //don't have enough of the item to complete the quest
		}		
	}
	
	return TRUE;

}

//returns true if the task in a given quest task are complete or not
BOOL QuestAllTasksCompleteForQuestTask( UNIT *unit,
										const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										BOOL bIgnoreVisited,
										BOOL bReturnFalseIfNoTasks )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( pQuestTask );

	if( !bIgnoreVisited &&	//why do this? Basically for UI purposes we need to know a quest is complete if all they need to do is talk to somebody
		QuestPlayerHasTask( unit, pQuestTask ) == FALSE ) 
	{
		//we have to at least visit the Task....
		return FALSE;
	}
	
	BOOL bHasTasksToComplete( FALSE );
	//lets check inventory items first
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unit, pQuestTask, t ) == INVALID_ID )
			break; //no more items to check against
		
		bHasTasksToComplete = TRUE;
		//lets first validate the items
		DWORD dwInvIterateFlags = 0;
		//SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
		//SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		
		int count = UnitInventoryCountItemsOfType( unit, MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unit, pQuestTask, t ), dwInvIterateFlags );
		if( MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( unit, pQuestTask, t ) != 0 && 
			count < MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( unit, pQuestTask, t ) )
		{
			return FALSE; //don't have enough of the item to complete the quest
		}

	}

	//this is broken out do to how slow it is. 
	if( sQuestItemsPlayerOwnsAreValidForTaskComplete( unit, pQuestTask ) == FALSE )
	{
		return FALSE;
	}


	if( QuestObjectSpawnIsComplete( unit, pQuestTask ) == FALSE )
	{
		return FALSE;
	}
	else
	{
		bHasTasksToComplete = MYTHOS_QUESTS::QuestRequiresBossOrObjectInteractionToComplete( unit, pQuestTask );
	}
	
	//ok check for if we talk to all the people we are ment to...
	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break; //must be ok
		if( pQuestTask->nQuestNPCTalkToCompleteQuest[ t ] > 0   )
		{			
			if( QuestTaskGetMask( unit, pQuestTask, t, QUESTTASK_SAVE_NPC ) == FALSE )
			{
				return FALSE; //we haven't talk to them yet
			}
		}
	}

	//ok lets check to see if other quests are done
	for( int t = 0; t < MAX_GLOBAL_QUESTS_COMPLETED; t++ )
	{
		if( pQuestTask->nQuestQuestsToFinish[ t ] == INVALID_ID )
			break; //we are done checking so it must be ok		
		bHasTasksToComplete = TRUE;
		if( !QuestIsQuestComplete( unit, pQuestTask->nQuestQuestsToFinish[ t ] ) )
			return FALSE; //quest is not complete so the task is not finished
	}	
	if( bReturnFalseIfNoTasks )
	{
		return bHasTasksToComplete;
	}
	return TRUE;
}

//returns true if the task in a given quest task are complete or not
BOOL QuestAllTasksCompleteForQuestTask( UNIT *unit,	//unit trying to complete quest
									    int questId, //quest id
									    BOOL bIgnoreVisited,    //Ignore Visited
									    BOOL bReturnFalseIfNoTasks )
{		
	const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = QuestGetActiveTask( unit, questId );
	if( activeTask == NULL ) return TRUE; //could not find active quest task
	return QuestAllTasksCompleteForQuestTask( unit, activeTask, bIgnoreVisited, bReturnFalseIfNoTasks );

}


//this returns the quest dialog ID - call after update quest
int QuestGetDialogID( GAME *pGame,
					  UNIT *unit,
					  const QUEST_TASK_DEFINITION_TUGBOAT *questTask,
					  BOOL ignoreComplete )
{
	if( questTask == NULL ) return INVALID_ID;
	if( QuestPlayerHasTask( unit, questTask ) == FALSE )
	{
		return MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_ACCEPTED );
	}

	//first check to see if it's completed or not
	if( !ignoreComplete &&
		QuestTaskGetMask( unit, questTask,0, QUESTTASK_SAVE_GIVE_REWARD ) )
	{
		//now we basically say if we don't have a complete text, just show the next tasks quests			
		if( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_COMPLETE ) != INVALID_ID )
			return MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_COMPLETE );
		else if( questTask->pNext )
		{
			int dID = QuestGetDialogID( pGame, unit, questTask->pNext, TRUE ); //show next quests text
			//s_QuestSetVisited( unit, questTask->pNext ); //we have to set this or we'll get the message twice.
			return dID;
		}
	}
	//check now to see if we have visited before yet
	int nComebackID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_RETURN );
	if( nComebackID != INVALID_ID )
		return nComebackID; //first visit


	//show standard text	
	if( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_ACCEPTED ) == INVALID_ID )
	{
		if( nComebackID == -1 )
		{
			return MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_COMPLETE );
		}
		else
		{
			return nComebackID; //if it's invalid we'll show first visit
		}
	}
	
	return MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( unit, questTask, KQUEST_STRING_ACCEPTED ); //standard text
}





//this returns if the Quest has been accepted
BOOL QuestGetQuestAccepted( GAME *pGame,
							UNIT *unit,
							 const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	ASSERT_RETFALSE(questTask);
	if( questTask->pPrev == NULL )
		return  QuestIsMaskSet( QUEST_MASK_ACCEPTED, unit, questTask );
	return TRUE;
}


BOOL QuestHasBeenVisited( UNIT *pPlayer,
						  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE(pQuestTask);	
	ASSERT_RETFALSE(pPlayer);	
	return  QuestIsMaskSet( QUEST_MASK_VISITED, pPlayer, pQuestTask );	
}




//this function caculates the new weight for an item being pased in
int QuestGetAdditionItemWeight(	GAME *pGame,
								UNIT *unit,
								const UNIT_DATA *item )
{
	int weightToReturn( 0 );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *task = QuestGetUnitTaskByQuestQueueIndex(unit, t );
		weightToReturn = max( weightToReturn, GetWeightAdditionalWeightOfUnit( pGame, unit, task, item ) );
	}
	return weightToReturn;
}

//just returns the active task of a quest by the unit's saved stats
const QUEST_TASK_DEFINITION_TUGBOAT * QuestGetUnitTaskByQuestQueueIndex( UNIT *unit,
																	int questQueueIndex )
{
	int questID = MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( unit, questQueueIndex );
	if( questID == INVALID_ID )
		return NULL;
	const QUEST_DEFINITION_TUGBOAT *quest = QuestDefinitionGet( questID );
	if( !quest )
		return NULL;
	return QuestGetActiveTask( unit, questID );
}

//this function looks for an item that the task is looking for and then returns a weight if need be
int GetWeightAdditionalWeightOfUnit( GAME *pGame,
									 UNIT *pPlayer,
									 const QUEST_TASK_DEFINITION_TUGBOAT *questTask,
									 const UNIT_DATA *item )
{
	if( questTask == NULL ) return 0; //bad quest task
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, questTask, t ) == INVALID_ID )
			break; //we are done, we aren't looking for any other items
		const UNIT_DATA *itemUnitData = ItemGetData( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, questTask, t ) );
		if ( item->nUnitType == itemUnitData->nUnitType )
			return questTask->nQuestItemsWeight[ t ]; //we found the item. return it's weight 
	}
	return 0;
}

//lets check to see if a quest can actually even start. We must 
//have a free QuestIndex or if the quest is already active
BOOL QuestCanStart( GAME *pGame,
					UNIT *pPlayer,
					int questId,
					int questStartFlags )
{
	const QUEST_DEFINITION_TUGBOAT *activeQuest = QuestDefinitionGet( questId );
	if( activeQuest == NULL )
		return FALSE;		
	if( QuestIsQuestComplete( pPlayer, questId ) )
		return FALSE;
	if( !QuestTaskCanStart( pGame, pPlayer, questId, questStartFlags ) ) return false; //we can't start the quest yet if the given quests are finished
	
	//now lets check for the quest	
	if( QuestUnitHasQuest( pPlayer, questId ) )
		return TRUE;

	// lets check the level if need be
	if( ( questStartFlags & QUEST_START_FLAG_IGNORE_LEVEL_REQ ) == 0 )
	{
		
		int iLevel = UnitGetExperienceLevel( pPlayer );
		//int iEndLevel = iLevel;
		//we want to know if the quest is avaible five levels before you can get it.
		if( (questStartFlags & QUEST_START_FLAG_LOWER_LEVEL_REQ_BY_5 ) )
		{
			iLevel += 5;			
		}
		if( activeQuest->nQuestPlayerStartLevel != -1 )
		{
			if( activeQuest->nQuestPlayerStartLevel >= 0 &&
				( iLevel < activeQuest->nQuestPlayerStartLevel ) )// ||
				 //( activeQuest->nQuestPlayerEndLevel != -1 &&
				 // iEndLevel > activeQuest->nQuestPlayerEndLevel ) ) )
			{
				return FALSE;
			}
		}
	}
	//lets check to see if there is an item that I have to have to do the quest
	if( activeQuest->nQuestItemNeededToStart != INVALID_ID)
	{
	
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
		SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		

		if (UnitInventoryCountItemsOfType( pPlayer, activeQuest->nQuestItemNeededToStart, dwInvIterateFlags ) <= 0 )	
		{
			return FALSE;
		}
		
	}
	//lets check states
	for( int t = 0; t < MAX_STATES_TO_CHECK; t++ )
	{
		if( activeQuest->nQuestStatesNeeded[ t ] == INVALID_ID )
			break;
		if( UnitHasState( pGame, pPlayer, activeQuest->nQuestStatesNeeded[ t ] ) == FALSE )
		{
			return FALSE;
		}
	}
	//lets check Unittypes
	for( int t = 0; t < MAX_UNITTYPES_TO_CHECK; t++ )
	{
		if( activeQuest->nQuestUnitTypeNeeded[ t ] == INVALID_ID )
			break;
		if( UnitIsA( pPlayer, activeQuest->nQuestUnitTypeNeeded[ t ] ) == FALSE )
		{
			return FALSE;
		}
	}

	if( ( questStartFlags & QUEST_START_FLAG_IGNORE_QUEST_COUNT ) )
	{
		//we are ignoring the number of quests
		return TRUE;
	}
	//last but not lest we better check to see if there is any open save spots to save the quest.
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) == INVALID_ID )
			return TRUE;
	}	
	return FALSE;
}



//this returns the total number of items needed for all active tasks.
int QuestGetTotalNumberOfItemsNeeded( GAME *pGame, 
									  UNIT * pUnit,
									  int itemID )
{
	if( itemID == INVALID_ID )
		return 0;
	const QUEST_TASK_DEFINITION_TUGBOAT *questTask( NULL ); //task we are on
	int totalCount( 0 );
	for( int y = 0; y < MAX_ACTIVE_QUESTS_PER_UNIT; y++ )
	{
		questTask = QuestGetUnitTaskByQuestQueueIndex( pUnit, y );
		if( !questTask )
			continue;  //we don't have a task for that given quest
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			if ( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pUnit, questTask, t ) == itemID &&
				 MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pUnit, questTask, t ) > 0 )
				totalCount += MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pUnit, questTask, t );
		}
		for( int t = 0; t < MAX_OBJECTS_TO_SPAWN; t++ )
		{
			if ( MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( questTask, t ) == itemID ||
				 MYTHOS_QUESTS::QuestGetBossIDSpawnsItemIndex( pUnit, questTask, t ) == itemID )
				totalCount++;
		}

		
	}
	return totalCount;
}


//this returns the total number of items needed for all active tasks.
int QuestGetTotalNumberOfItemsToCollect( GAME *pGame, 
									  UNIT * pUnit,
									  int itemID )
{
	if( itemID == INVALID_ID )
		return 0;
	const QUEST_TASK_DEFINITION_TUGBOAT *questTask( NULL ); //task we are on
	int totalCount( 0 );
	for( int y = 0; y < MAX_ACTIVE_QUESTS_PER_UNIT; y++ )
	{
		questTask = QuestGetUnitTaskByQuestQueueIndex( pUnit, y );
		if( !questTask )
			continue;  //we don't have a task for that given quest
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			if ( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pUnit, questTask, t ) == itemID &&
				 MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pUnit, questTask, t ) > 0 )
				totalCount += MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pUnit, questTask, t );
		}

	}
	return totalCount;
}
inline UNIT * s_SpawnItemFromUnit( UNIT *unit,
								 UNIT *unitSpawningTo,
								 int itemClass )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME *pGame = UnitGetGame( unit );
	UNIT *returnUnit( NULL );
	const UNIT_DATA *unitData = ItemGetData( itemClass );
	if( unitData )
	{
		// spawn the item
		// init spawn spec
		ITEM_SPEC tSpec;	
		//if we want to spawn the item only to a single client
		if( unitSpawningTo )
		{
			SETBIT(tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT);
			tSpec.guidRestrictedTo = UnitGetGuid( unitSpawningTo );
		}
		SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		tSpec.pvPosition = &unit->vPosition;
		tSpec.nItemClass = itemClass;
		tSpec.pSpawner = unit;
		returnUnit = s_ItemSpawn( pGame, tSpec, NULL);					
		if( returnUnit )
		{
			UnitUpdateRoom( returnUnit, UnitGetRoom( unit ), TRUE );
			returnUnit->vPosition = unit->vPosition;
			UnitDrop( pGame, returnUnit, unit->vPosition );
		}
	}
	return returnUnit;
#else
	REF( unit );
	REF( unitSpawningTo );
	REF( itemClass );
	return NULL;
#endif
}

//When a unit is killed, called this function. It will spawn any items needed to be spawned
void QuestSpawnQuestItems( GAME *pGame,
						   UNIT *unitDefender,
						   UNIT *unitAttacker,
						   UNIT *unitClient )
{
	if( unitAttacker == NULL ||
		unitDefender == NULL ||
		unitClient == NULL )
		return;
	const QUEST_TASK_DEFINITION_TUGBOAT *questTask( NULL );
	int defendersQuality = MonsterGetQuality( unitDefender );
	for( int y = 0; y < MAX_ACTIVE_QUESTS_PER_UNIT; y++ )
	{
		questTask = QuestGetUnitTaskByQuestQueueIndex( unitClient, y );
		
		int levelDefID = LevelGetLevelAreaID( UnitGetLevel( unitDefender ) ); //some quest require you to kill monsters in certain dungeons/levels
		if( !questTask )
			continue; 
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			//Lets first check to see if we have a valid monster to spawn the item from
			BOOL AllowSpawn( FALSE );
			if( defendersQuality < questTask->nUnitQualitiesToSpawnFrom[ t ] )
				continue; //must be a given quality
			
			//time to check to see if they are on the correct level to spawn. Lets make it easier for the testers. You kills something intown your set.
			if( !UnitIsInTown( unitDefender ) &&
				( MYTHOS_QUESTS::QuestGetItemSpawnDropsInDungeonByIndex( unitClient, questTask, t ) != INVALID_ID &&							
				levelDefID != MYTHOS_QUESTS::QuestGetItemSpawnDropsInDungeonByIndex( unitClient, questTask, t ) ))
				continue;
			
			//check to see if we only spawn it for the attacker
			if( questTask->nQuestItemsSpawnUniqueForAttacker[ t ] > 0 && //then it only works for the attacker
				unitAttacker != unitClient ) //not the same
				continue;	//then continue - must be the same if UniqueForAttacker is set

			if( MYTHOS_QUESTS::QuestGetItemSpawnFromUnitTypeByIndex( unitClient, questTask, t ) > 0 )
			{
				AllowSpawn = ( UnitIsA( unitDefender, MYTHOS_QUESTS::QuestGetItemSpawnFromUnitTypeByIndex( unitClient, questTask, t ) ) == TRUE ); //unit must be of type
			}
			if( AllowSpawn == FALSE && MYTHOS_QUESTS::QuestGetItemSpawnFromMonsterClassByIndex( unitClient, questTask, t ) != INVALID_ID )
			{
				UNIT_DATA *data = UnitGetData( pGame, GENUS_MONSTER, MYTHOS_QUESTS::QuestGetItemSpawnFromMonsterClassByIndex( unitClient, questTask, t ) );
				if( data )
				{
					AllowSpawn = ( data->wCode == unitDefender->pUnitData->wCode );	//we have the correct monster
				}
			}
			if( !AllowSpawn )
			{
				continue;//we don't have a valid object or monster to spawn from. So it must be just an item to collect
			}
			//end check for valid monster to spawn from
			
			//spawn item			
			if( RandGetFloat( pGame ) * 100 <= (( MYTHOS_QUESTS::QuestGetItemSpawnChance( unitClient, questTask, t ) < 0 )?100:MYTHOS_QUESTS::QuestGetItemSpawnChance( unitClient, questTask, t ) ) )
			{

				if ( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unitClient, questTask, t ) != INVALID_ID )		
				{

					DWORD dwInvIterateFlags = 0;
					SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
					SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		
					
					int count = UnitInventoryCountItemsOfType( unitClient, MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unitClient, questTask, t ), dwInvIterateFlags );
					int numNeeded = MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( unitClient, questTask, t );// QuestGetTotalNumberOfItemsNeeded( pGame, unitClient, MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unitClient, questTask, t ) );
					if( numNeeded <= 0 || count >= numNeeded )
						continue;

					//spawns the item from the defender
					UNIT *unitCreated = s_SpawnItemFromUnit( unitDefender,
															 unitClient,
															 MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unitClient, questTask, t ) );
					if( unitCreated )
					{
						s_SetUnitAsQuestUnit( unitCreated, unitClient, questTask, t ); 
					}

				}
			}
			//end sapwn

		}
	}
	
}


//returns the count of quest tasks for seconday quest tasks added to the array
int QuestGetAvailableNPCSecondaryDialogQuestTasks( GAME *pGame,
											       UNIT *pPlayer,
											       UNIT *pNPC,
												   const QUEST_TASK_DEFINITION_TUGBOAT **pQTArrayToFill,
												   int nOffset ) // = 0)
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return nOffset;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return nOffset;
	int count(nOffset);
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer,
																						i );

		if( pQuestTask )
		{
			for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
			{
				if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
					break;
				if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPCUnitData->nNPC )
				{		
					pQTArrayToFill[ count++ ] = pQuestTask;					
				}
			}
		}
	}
	return count;	
}
//returns an array filled with Quest Tasks the NPC has dialog for
int QuestGetAvailableNPCQuestTasks( GAME *pGame,
								   UNIT *pPlayer,
								   UNIT *pNPC,
								   const QUEST_TASK_DEFINITION_TUGBOAT **pQTArrayToFill,
								   BOOL bLowerLevelReq, /*=TRUE*/
								   int nOffset /*=0*/)
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return 0;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return 0;

	int count(nOffset);
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	if( QuestUnitIsQuestUnit( pNPC ) )
	{
		//check to see if the Quest Object has a dialog
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pNPC );
		ASSERT_RETFALSE(pQuestTask);
		if( pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_ID )
		{
			pQTArrayToFill[ count++ ] = pQuestTask;
		}
			
	}

	if( pNPCUnitData->nNPC == INVALID_LINK )
		return count;

	//FIRST we need to see if the NPC is needed for our previous quests
	//if the NPC is needed
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		if( pQuestTask &&
			( pQuestTask->nNPC == pNPC->pUnitData->nNPC ||
			  ( pQuestTask->nNPC == -1 && UnitIsGodMessanger( pNPC ) )))
		{
			//arggg got to make sure there are no dupes
			bool bAdd( TRUE );
			for( int tt = 0; tt < count; tt++ )
			{
				if( pQTArrayToFill[ tt ] &&
					pQTArrayToFill[ tt ] == pQuestTask )
				{
					bAdd = false;
					break;
				}
			}
			if( bAdd )
			{
				pQTArrayToFill[ count++ ] = pQuestTask;			
			}
		}
	}
	DWORD dwFlags = QUEST_START_FLAG_IGNORE_QUEST_COUNT | QUEST_START_FLAG_IGNORE_ITEM_COUNT;
	if( bLowerLevelReq )
	{
		dwFlags |= QUEST_START_FLAG_LOWER_LEVEL_REQ_BY_5;
	}
	//Second we check to see if any new quests are avaible
	unsigned int nNumRows = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUEST_TITLES_FOR_TUGBOAT );
	for (unsigned int i = 0; i < nNumRows; ++i)
	{
		//Quest definition
		QUEST_DEFINITION_TUGBOAT *pQuestDefinition = (QUEST_DEFINITION_TUGBOAT*)ExcelGetData( pGame, DATATABLE_QUEST_TITLES_FOR_TUGBOAT, i );
		const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask = pQuestDefinition->pFirst;

		if( pQuestTask != NULL )
		{
			if( pQuestTask->nNPC == pNPCUnitData->nNPC ||
				( UnitIsGodMessanger( pNPC ) &&
				  pQuestTask->nNPC == INVALID_ID ))	//they are a god messanger
			{
				if( QuestCanStart( pGame, pPlayer, i, dwFlags ) &&
					!QuestGetIsTaskComplete( pPlayer, pQuestTask ) ) //make sure the quest can start
				{
					//arggg got to make sure there are no dupes
					bool bAdd( TRUE );
					for( int tt = 0; tt < count; tt++ )
					{
						if( pQTArrayToFill[ tt ] &&
							pQTArrayToFill[ tt ] == pQuestTask )
						{
							bAdd = false;
							break;
						}
					}
					if( bAdd )
					{
						pQTArrayToFill[ count++ ] = pQuestTask;			
					}
				}
			}
		}
	}

	return count;
}


const QUEST_TASK_DEFINITION_TUGBOAT * QuestNPCGetQuestTask( GAME *pGame,
															UNIT *pPlayer,
															UNIT *pNPC,
															int nQuestID /*=INVALID_ID*/,
															BOOL bLowerLevelReq /*=TRUE*/)
{
	//if( nQuestID == INVALID_ID )
	//	return NULL;
	const QUEST_TASK_DEFINITION_TUGBOAT *pQTArrayToFill[ MAX_NPC_QUEST_TASK_AVAILABLE ];
	pQTArrayToFill[0] = NULL;
	int count = QuestGetAvailableNPCQuestTasks( pGame, pPlayer, pNPC, pQTArrayToFill, bLowerLevelReq );
	count = QuestGetAvailableNPCSecondaryDialogQuestTasks( UnitGetGame( pPlayer ),
															 pPlayer,
															 pNPC,
															 pQTArrayToFill,
															 count );
	ASSERT_RETNULL( count < MAX_NPC_QUEST_TASK_AVAILABLE );
	for( int t = 0; t < count; t++ )
	{
		if( pQTArrayToFill[ t ]->nParentQuest == nQuestID )
		{
			return pQTArrayToFill[ t ];
		}
	}
	if( nQuestID != INVALID_ID )
	{
		return NULL;
	}
	return pQTArrayToFill[0];
}

//Call this to see if the NPC has a quest task for the player
/*
const QUEST_TASK_DEFINITION_TUGBOAT * QuestNPCHasQuestTask( GAME *pGame,
															 UNIT *pPlayer,
															 UNIT *pNPC )
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return NULL;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return NULL;
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	if( QuestUnitIsQuestUnit( pNPC ) )
	{
		//check to see if the Quest Object has a dialog
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pNPC );
		if( pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_ID )
			return pQuestTask;
	}


	if( pNPCUnitData->nNPC == INVALID_LINK )
		return NULL;

	//FIRST we need to see if the NPC is needed for our previous quests
	//if the NPC is needed
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestIndex( pPlayer, i );
		if( pQuestTask &&
			( pQuestTask->nNPC == pNPC->pUnitData->nNPC ||
			  ( pQuestTask->nNPC == -1 && UnitIsGodMessanger( pNPC ) )))
			return pQuestTask;
	}
	//Second we check to see if any new quests are avaible
	unsigned int nNumRows = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUEST_TITLES_FOR_TUGBOAT );
	for (unsigned int i = 0; i < nNumRows; ++i)
	{
		//Quest definition
		QUEST_DEFINITION_TUGBOAT *pQuestDefinition = (QUEST_DEFINITION_TUGBOAT*)ExcelGetData( pGame, DATATABLE_QUEST_TITLES_FOR_TUGBOAT, i );
		const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask = pQuestDefinition->pFirst;

		if( pQuestTask != NULL )
		{
			if( pQuestTask->nNPC == pNPCUnitData->nNPC ||
				( UnitIsGodMessanger( pNPC ) &&
				  pQuestTask->nNPC == INVALID_ID ))	//they are a god messanger
			{
				if( QuestCanStart( pGame, pPlayer, i, QUEST_START_FLAG_LOWER_LEVEL_REQ_BY_5 | QUEST_START_FLAG_IGNORE_QUEST_COUNT | QUEST_START_FLAG_IGNORE_ITEM_COUNT ) &&
					!QuestGetIsTaskComplete( pPlayer, pQuestTask ) ) //make sure the quest can start
				{
					return pQuestTask;
				}
			}
		}
	}

	return NULL;
}
*/

BOOL QuestWasSecondaryNPCMainNPCPreviously( GAME *pGame,
											  UNIT *pPlayer,
											  UNIT *pNPC,
											  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return FALSE;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return FALSE;
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	//for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	//{
		//const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestIndex( pPlayer,
		//																				i );

		if( pQuestTask )
		{
			for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
			{
				if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
					break;
				if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPCUnitData->nNPC )
				{						
					if( pQuestTask->pPrev != NULL &&
						pQuestTask->pPrev->nNPC == pNPCUnitData->nNPC )
					{
						return TRUE;
					}
				}
			}
		}
	//}
	return FALSE;
}


//this function returns the index of the dialog but do
/*
BOOL QuestDoesNPCHaveSecondaryDialogForQuest( GAME *pGame,
											  UNIT *pPlayer,
											  UNIT *pNPC )
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return FALSE;
	if( UnitCanInteractWith( pNPC, pPlayer ) == FALSE )
		return FALSE;
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestIndex( pPlayer,
																						i );

		if( pQuestTask )
		{
			for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
			{
				if( pQuestTask->nQuestNPCTalkTo[ t ] == INVALID_ID )
					break;
				if( pQuestTask->nQuestNPCTalkTo[ t ] == pNPCUnitData->nNPC )
				{						
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
*/
BOOL QuestIsLevelAreaNeededByTask( UNIT *pPlayer,
								   int nLvlAreaId,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( pQuestTask )
	{

		if( isQuestRandom( pQuestTask ) )
		{
			for( int t = 0; t < MAX_DUNGEONS_VISIBLE; t++ )
			{
				int nLevelAreaID = MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, t );
				if( nLevelAreaID == INVALID_ID ) 
					break;
				if( nLevelAreaID == nLvlAreaId )
					return TRUE;
			}
		}		
		for( int t = 0; t < MAX_DUNGEONS_VISIBLE; t++ )
		{
			if( pQuestTask->nQuestTaskDungeonsVisible[ t ] == INVALID_ID )
				break;
			if( pQuestTask->nQuestTaskDungeonsVisible[ t ] == nLvlAreaId )
				return TRUE;
		}
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			if( pQuestTask->nQuestItemsAppearOnLevel[ t ] == nLvlAreaId )
				return TRUE;			
		}
		for( int t = 0; t < MAX_OBJECTS_TO_SPAWN; t++ )
		{
			if( pQuestTask->nQuestDungeonsToSpawnBossObjects[ t ] == nLvlAreaId )
				return TRUE;			
			if( pQuestTask->nQuestDungeonsToSpawnObjects[ t ] == nLvlAreaId )
				return TRUE;									
		}
	}
	return FALSE;
}

//Checks to see if a dungeon is visible by a player for there active quests
BOOL sQuestIsLevelAreaNeededByQuestSystem( UNIT *pPlayer,
										   int nLvlAreaID )
{
	ASSERTX_RETFALSE( pPlayer, "Must pass a valid player to check if dungeon is visible" );
	ASSERTX_RETFALSE( nLvlAreaID != INVALID_ID , "LEVEL_AREA_DEFINITION is NULL in s_QuestIsDungeonVisible" );	
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		if( QuestIsLevelAreaNeededByTask( pPlayer, nLvlAreaID, pQuestTask ) )
			return TRUE;
	}
	return FALSE;
}


BOOL QuestNPCSecondaryNPCHasBeenVisited( UNIT *pPlayer,
										 UNIT *pNPC,
										 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	//const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = s_QuestNPCGetSecondaryDialogTask( UnitGetGame( pPlayer ), pPlayer, pNPC );
	if( !pQuestTask )
		return FALSE;
	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break;
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPC->pUnitData->nNPC )
		{
			return QuestTaskGetMask( pPlayer, pQuestTask, t, QUESTTASK_SAVE_NPC );
		}
	}
	return FALSE;

}

//this is the call back for iterating the units in a room.
static ROOM_ITERATE_UNITS sQuestUpdateNPCsByCallBack( UNIT *pUnit,
													  void *pCallbackData)
{
	if( pCallbackData == NULL )
		return RIU_CONTINUE;
	UNIT *pPlayer = (UNIT *)pCallbackData;
	if( pUnit->pUnitData->nNPC != INVALID_LINK  )
	{
		QuestUpdateQuestNPCs( UnitGetGame( pUnit ), pPlayer, pUnit );

	}
	return RIU_CONTINUE;
}

void QuestUpdateNPCsInLevel( GAME *pGame,
							 UNIT *pPlayer,
							 LEVEL *pLevel  )
{
	if( !pLevel ) 
		return;
	//first find the units taht are NPC's	
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_DEAD, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sQuestUpdateNPCsByCallBack, (void *)pPlayer );

}


BOOL QuestUpdateQuestNPCs( GAME *pGame,
						   UNIT *pPlayer,
						   UNIT *pNPC )
{
	if( IS_SERVER( pGame ) )
	{
		return s_QuestUpdateQuestNPCs( pGame, pPlayer, pNPC );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		BOOL bSuccess = c_QuestUpdateQuestNPCs( pGame, pPlayer, pNPC );
		
		if( AppIsTugboat() )
		{
			if( bSuccess )
			{
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_MERCHANT, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GUIDE, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_TRAINER, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_HEALER, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_MAPVENDOR, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_CRAFTER, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GAMBLER, 0 );
				c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GUILDMASTER, 0 );
			}
			else
			{
				
			}
		}
		return bSuccess;
#else
		ASSERT_RETFALSE(FALSE);
#endif
	}
}


inline void QuestExecuteCode( GAME *pGame,
							  UNIT *pPlayer,
							  LEVEL *pLevel,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  QUEST_WORLD_UPDATES worldUpdate,
							  UNIT *pObject )
{
	if( pQuestTask == NULL ||
		pLevel == NULL )
		return;
	PCODE code( 0 );
	//lets not worry about server versus other things
	//if( IS_SERVER( pGame ) )
	{
		code = pQuestTask->pQuestCodes_Server[ worldUpdate ];
	}
	if( code )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, code, &code_len);		
		VMExecI( pGame, pPlayer, pObject, NULL, pQuestTask->nTaskIndexIntoTable, pLevel->m_idLevel, INVALID_LINK, INVALID_SKILL_LEVEL, INVALID_LINK, code_ptr, code_len);
	}

}

inline void QuestExecuteCodeOnAllQuests( GAME *pGame,
							  UNIT *pPlayer,
							  LEVEL *pLevel,							  
							  QUEST_WORLD_UPDATES worldUpdate,
							  UNIT *pObject )
{
	//First we check to see if any new quests are avaible
	unsigned int nNumRows = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUEST_TITLES_FOR_TUGBOAT );
	for (unsigned int i = 0; i < nNumRows; ++i)
	{
		//Quest definition
		QUEST_DEFINITION_TUGBOAT *pQuestDefinition = (QUEST_DEFINITION_TUGBOAT*)ExcelGetData( pGame, DATATABLE_QUEST_TITLES_FOR_TUGBOAT, i );
		ASSERT_CONTINUE(pQuestDefinition);
		const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask = pQuestDefinition->pFirst;
		QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
	}
}


inline void QuestExecuteCodeOnAllActiveTasks( GAME *pGame,
											  UNIT *pPlayer,
											  LEVEL *pLevel,							  
											  QUEST_WORLD_UPDATES worldUpdate,
											  UNIT *pObject )
{
	//First we check to see if any new quests are avaible
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		//Quest task definition
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		
		QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
	}
}


//Gets called when the player comes out of limbo
void QuestPlayerComesOutOfLimbo( GAME *pGame,
								 UNIT *pPlayer )
{
	if( !pPlayer  )//||
		//UnitIsInTown( pPlayer ) )
		return;	//No need
	QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), NULL, QUEST_WORLD_UPDATE_QSTATE_BEFORE, NULL );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), QuestGetActiveTask( pPlayer, MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) ), QUEST_WORLD_UPDATE_QSTATE_START, NULL );
	}
}

//Checks to see if there is enough inventory space
BOOL QuestCanGiveItemsOnAccept( UNIT *pPlayer,
								  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETFALSE( pQuestTask, "Quest Task passed in is NULL( s_QuestCanGiveItemsOnAccept )" );
	ASSERTX_RETFALSE( pPlayer, "Player passed in is NULL( s_QuestCanGiveItemsOnAccept )" );
	const int MAX_ITEMS_TO_GIVE  = MAX_DUNGEONS_TO_GIVE + MAX_ITEMS_TO_GIVE_ACCEPTING_TASK;
	UNIT *unitsCreated[ MAX_ITEMS_TO_GIVE ];
	ZeroMemory( unitsCreated, sizeof( UNIT * ) * MAX_ITEMS_TO_GIVE );
	int count( 0 );
	BOOL canPickUp( TRUE );

	for( int i = 0; i < MAX_ITEMS_TO_GIVE_ACCEPTING_TASK; i++ )
	{
		if( pQuestTask->nQuestGiveItemsOnAccept[ i ] == INVALID_ID )
			break;
		ITEM_SPEC tSpec;
		tSpec.nItemClass = pQuestTask->nQuestGiveItemsOnAccept[ i ];
		tSpec.pSpawner = pPlayer;
		unitsCreated[ count++ ] = s_ItemSpawn( UnitGetGame( pPlayer ), tSpec, NULL);			
	}

	if( pQuestTask->bDoNotAutoGiveMaps == FALSE )
	{
		count += s_CreateMapsForPlayerOnAccept( pPlayer, pQuestTask,  FALSE, FALSE, &unitsCreated[ count ] );		
	}
	
	if( count > 0 )	
		canPickUp = UnitInventoryCanPickupAll( pPlayer, unitsCreated, count, TRUE );
	
	for( int t = 0; t < count; t++ )	
		UnitFree( unitsCreated[ t ] );	
	
	return canPickUp;
#else
	REF( pPlayer );
	REF( pQuestTask );
	return FALSE;
#endif
}


//Updates the world
void QuestUpdateWorld( GAME *pGame,
					   UNIT *pPlayer,
					   LEVEL *pLevel,
					   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					   QUEST_WORLD_UPDATES worldUpdate,
					   UNIT *pObject )
{
	if( !pPlayer ||
		!UnitIsPlayer( pPlayer ) )
	{
		return;
	}
	switch( worldUpdate )
	{
	case QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED:
		{
			QuestExecuteCodeOnAllActiveTasks( pGame, pPlayer, pLevel, worldUpdate, pObject );
		}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_AFTER_TALK_NPC:
		{
			if( pQuestTask )
			{
				QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
			}
			else
			{
				QuestExecuteCodeOnAllActiveTasks( pGame, pPlayer, pLevel, worldUpdate, pObject );
			}
		}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_INTERACT_QUESTUNIT:
	case QUEST_WORLD_UPDATE_QSTATE_TALK_NPC:
		{
			QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
		}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_BEFORE:
		{
			QuestExecuteCodeOnAllQuests( pGame, pPlayer, pLevel, worldUpdate, pObject );
		}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_START:
			if( pQuestTask )
			{
				QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
			}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_TASK_COMPLETE:
			if( pQuestTask )
			{
				QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
			}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT:
	case QUEST_WORLD_UPDATE_QSTATE_COMPLETE:
			if( pQuestTask )
			{
				QuestExecuteCode( pGame, pPlayer, pLevel, pQuestTask, worldUpdate, pObject );
			}
		break;
	case QUEST_WORLD_UPDATE_QSTATE_OBJECT_OPERATED:
	default:
		break;
	}
	if( IS_CLIENT( pGame ) )
	{
		QuestUpdateNPCsInLevel( pGame, pPlayer, pLevel );
		cLevelUpdateUnitsVisByThemes( UnitGetLevel( pPlayer ) );	
	}
	else
	{
		QuestUpdateNPCsInLevel( pGame, pPlayer, pLevel );
	}
}






//returns if a bit is set or not
BOOL QuestGetMaskRandomStat( UNIT *pUnit,
							  const QUEST_DEFINITION_TUGBOAT *pQuest,
							  const int &rndStatShift )
{
	if( !pQuest ||
		rndStatShift < 0 ||
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return FALSE;	
	}	
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pUnit, pQuest );
	ASSERT_RETFALSE( nQuestQueueIndex != INVALID_ID );
	int stat = UnitGetStat( pUnit, STATS_QUEST_RANDOM_DATA_TUGBOAT, nQuestQueueIndex );
	return ( stat & ( 1 << rndStatShift ) );
}

BOOL QuestTaskGetMask( UNIT *pUnit,
					   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					   const int &rndStatShift,
					   QUESTTASK_SAVE_OFFSETS nType )
{
	if( !pQuestTask ||
		rndStatShift < 0 ||
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return FALSE;	
	}	
	int offSet = nType + rndStatShift;
	int nValue = QuestTaskGetMaskValue( pUnit, pQuestTask );
	//int nValue = UnitGetStat( pUnit, QuestTaskGetMaskStat( pUnit, pQuestTask ) );
	return ( nValue & ( 1 << offSet ) )?TRUE:FALSE;
}



//returns if the quest task is an active task
BOOL QuestPlayerHasTask( UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pActiveQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		if( pActiveQuestTask  == pQuestTask )
		{
			if( isQuestRandom( pQuestTask ) )
			{
				//if it's random we have to do a few more little things here....
				if( MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex(pPlayer, pQuestTask, 0 ) == MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex(pPlayer, pActiveQuestTask, 0 ) &&
					MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex(pPlayer, pQuestTask, 0 ) == MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex(pPlayer, pActiveQuestTask, 0 ) &&
					MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex(pPlayer, pQuestTask, 0 ) == MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex(pPlayer, pActiveQuestTask, 0 ) &&
					MYTHOS_QUESTS::QuestGetGoldEarned(pPlayer, pQuestTask ) == MYTHOS_QUESTS::QuestGetGoldEarned(pPlayer, pActiveQuestTask ) &&
					MYTHOS_QUESTS::QuestGetExperienceEarned(pPlayer, pQuestTask ) == MYTHOS_QUESTS::QuestGetExperienceEarned(pPlayer, pActiveQuestTask ) &&
					MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex(pPlayer, pQuestTask, 0 ) == MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex(pPlayer, pActiveQuestTask, 0 ) )
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			return TRUE;
		}
	}	
	return FALSE;
}


//This returns false if the unit passed in is a Quest map and it's not a active task
BOOL QuestCanQuestMapDrop( UNIT *pPlayer,
						   UNIT *pUnit )
{
	if( !pPlayer ||
		!pUnit )
	{
		return TRUE;
	}

	if( !UnitIsPlayer( pPlayer ) )
	{
		return TRUE;
	}
	
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pUnit );
	if( !pQuestTask )
		return TRUE;

	return !QuestPlayerHasTask( pPlayer, pQuestTask );
	
}

inline void QuestUpdateThemes( const QUEST_THEMES &questThemes, LEVEL *pLevel, BOOL clientOnly )
{
	int count( 0 );
	while( count < MAX_THEMES_PER_LEVEL &&
		   questThemes.nQuestDungeonAreaThemes[ count ] != INVALID_ID )
	{	
		if( questThemes.nQuestDungeonAreaThemes[ count ] == LevelGetLevelAreaID( pLevel ) )
		{

			switch( questThemes.nQuestDungeonThemeProperties[ count ] )
			{
			default:
			case QUEST_THEME_PROP_ADD:		
				if( !clientOnly )
				{
					pLevel->m_pnThemes[pLevel->m_nThemeIndexCount++] = questThemes.nQuestDungeonThemes[count];
				}
				break;
			case QUEST_THEME_PROP_CLIENT_ADD:
				if( clientOnly && IS_CLIENT( LevelGetGame( pLevel ) ) )
				{
					pLevel->m_pnThemes[pLevel->m_nThemeIndexCount++] = questThemes.nQuestDungeonThemes[count];
				}
				break;
			case QUEST_THEME_PROP_CLIENT_REMOVE:
				if( clientOnly && IS_CLIENT( LevelGetGame( pLevel ) ) )
				{
					for( int tt = 0; tt < MAX_THEMES_PER_LEVEL; tt++)
					{
						if( pLevel->m_nThemeIndexCount > 0 &&
							pLevel->m_pnThemes[ tt ] == questThemes.nQuestDungeonThemes[count] )
						{
							pLevel->m_pnThemes[ tt ] = pLevel->m_pnThemes[ --pLevel->m_nThemeIndexCount ];
							tt--;
						}
					}
				}
				break;
			}
		}
		count++;
	}
}
//on the server player can be null
void QuestGetThemes( LEVEL *pLevel )
{
	ASSERTX( pLevel, "No level supplied." );
	GAME *pGame = LevelGetGame( pLevel );
	int QuestCount = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT );
	for( int i = 0; i < QuestCount; i++ )
	{
		QUEST_TASK_DEFINITION_TUGBOAT* pQuestTaskDefinition = (QUEST_TASK_DEFINITION_TUGBOAT*)ExcelGetData( pGame, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, i );		
		if( pQuestTaskDefinition &&
			pQuestTaskDefinition->m_bHasThemes )
		{
			QuestUpdateThemes( pQuestTaskDefinition->m_QuestThemes, pLevel, FALSE );
		}

	}	
#if !ISVERSION(SERVER_VERSION)
	if( IS_CLIENT( pGame ) )
	{
		UNIT *pPlayer = UIGetControlUnit();  //only for the client
		if( pPlayer) 
		{
			for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
			{
				int nID = MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, i );
				if( nID != INVALID_ID )
				{
					const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( nID );
					if( pQuest )
					{
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetActiveTask( pPlayer, nID );
						if( pQuestTask != NULL )
						{
							QuestUpdateThemes( pQuestTask->m_QuestThemes, pLevel, TRUE );
						}
					}
				}
			}
		}
		
	}
#endif
}


BOOL QuestCanNPCEndQuestTask( UNIT *pNPC,
							  UNIT *pPlayer,
						      const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{	
	ASSERTX_RETFALSE( pPlayer, "No player" );
	ASSERT_RETFALSE(pQuestTask);
	if( !pNPC ) //this can happen. ( Could be an item that is not an NPC and hence the NPC that gets passed in could be NULL'ed out.)
		return FALSE;	
	if( QuestNPCIsASecondaryNPCToTask( UnitGetGame( pPlayer ), pNPC, pQuestTask ) )
	{		
		//ok check for if we talk to all the people we are ment to...
		for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
		{
			if( pQuestTask->nQuestNPCTalkToCompleteQuest[ t ] > 0 &&
				pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPC->pUnitData->nNPC   )			
				return TRUE;				
		}
	}
	else
	{
		if( pQuestTask->nNPC == pNPC->pUnitData->nNPC )
			return TRUE;
		if( pQuestTask->nNPC == -1 &&
			UnitIsGodMessanger( pNPC ) )
			return TRUE;
	}
	return FALSE;
}

//this doesn't check to see if you talked to them already
BOOL QuestNPCIsASecondaryNPCToTask( GAME *pGame,
								    UNIT *pNPC,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( pNPC == NULL ||
		pQuestTask == NULL )
		return FALSE;
	if( UnitIsInteractive( pNPC ) == FALSE )
		return FALSE;
	//first lets check to see if it's a Quest Object and if so if it has a dialog to show
	const UNIT_DATA *pNPCUnitData = UnitGetData( pNPC );	

	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break;
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == pNPCUnitData->nNPC )
		{
			return TRUE;
		}
	}

	return FALSE;
}


BOOL QuestHasRewards( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pQuestTask );
	if( pQuestTask->pNext == NULL )
	{
		return ( pQuestTask->nQuestGivesItems >= 0 || MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask ) != INVALID_ID );
	}
	return ( pQuestTask->nQuestGivesItems >= 0 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestCanAbandon( 
	int nQuestId )
{
	return nQuestId != QUEST_TUTORIAL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetPossibleTasks( UNIT *pPlayer,
							UNIT *pNPC )
{
	if( pPlayer == NULL ||
		pNPC == NULL )
		return 0;

	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskArray[MAX_NPC_QUEST_TASK_AVAILABLE];	
	int nQuestTaskCount = QuestGetAvailableNPCQuestTasks( UnitGetGame( pPlayer ),
															pPlayer,
															pNPC,
															pQuestTaskArray,
															FALSE,
															0 );

	int nQuestTaskSecondaryCount = QuestGetAvailableNPCSecondaryDialogQuestTasks( UnitGetGame( pPlayer ),
																				 pPlayer,
																				 pNPC,
																				 pQuestTaskArray,
																				 nQuestTaskCount );
	return nQuestTaskSecondaryCount;
	
}


