//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
/*
Marsh Lefler
	Quest Definition->
						Quest Task
						.
						.
						.
						X Quest Tasks

	Quest Definition defines the quest; name, id and stats that will get effected. it 
also points to the first Quest Task, and the active Quest Task. Each Quest task knows about the prev
and next quest tasks. Each quest task keeps track of items, kills and whatever else the
quest needs to know about. Also, if the quest is completed or not.

	Quests need a global definition stat. This stat will allow people to have multiple task that 
link with one another. For instance, joe needs you to talk to Jack and get Jack to give you a certain
family jewl. However to get the jewl you'll need to complete a task from Jack. Once Jack's quest is complete,
Jack sets a global stat, triggers it, and Joe's quest can continue on. - I don't think we'll need to many
of this quest types
*/

#ifndef __QUEST_H_
#define __QUEST_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __DIALOG_H_
#include "dialog.h"
#endif

#ifndef __QUESTS_H_
#include "quests.h"
#endif

#ifndef __QUESTMESSAGEPUMP_H_
#include "QuestMessagePump.h"
#endif

#ifndef	_LEVEL_H_
#include "level.h"
#endif

#ifndef __LEVELAREAS_H_
#include "LevelAreas.h"
#endif

#ifndef __QUESTSTRUCTS_H_
#include "QuestStructs.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
struct MSG_SCMD_AVAILABLE_QUESTS;
struct MSG_SCMD_QUEST_TASK_STATUS;


//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
	
inline BOOL QuestUnitIsUnitType_QuestItem( UNIT *pItem )
{
	return UnitIsA( pItem, UNITTYPE_QUEST_ITEM );
}

const QUEST_DEFINITION_TUGBOAT *QuestDefinitionGet( int nQuestIndex);

BOOL isQuestRandom( const QUEST_DEFINITION_TUGBOAT *pQuest );
BOOL isQuestRandom( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


BOOL QuestPlayerHasTask( UNIT *pPlayer,
						 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL QuestCountForTugboatExcelPostProcess( struct EXCEL_TABLE * table);
BOOL QuestRandomForTugboatExcelPostProcess( struct EXCEL_TABLE * table);
BOOL QuestForTugboatExcelPostProcess( struct EXCEL_TABLE * table);

BOOL QuestUnitHasQuest( UNIT *unit,
					    int questID );

int QuestIndexGet(
	UNIT *pPlayer,
	int nQuestID);

int QuestTaskGetMaskValue( UNIT *pPlayer,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet(	
	int nQuestTaskIndex);

const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet(	
	const char *name );

const QUEST_TASK_DEFINITION_TUGBOAT *QuestTaskDefinitionGet( DWORD dwCode );


const QUEST_TASK_DEFINITION_TUGBOAT * QuestGetActiveTask( UNIT *unit,
														  int questIndex );
BOOL QuestAllTasksCompleteForQuestTask( UNIT *unit,
										const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										BOOL bIgnoreVisited = FALSE,
										BOOL bReturnFalseIfNoTasks = FALSE  );

BOOL QuestAllTasksCompleteForQuestTask( UNIT *unit,
								   int questId,
								   BOOL bIgnoreVisited = FALSE,
								   BOOL bReturnFalseIfNoTasks = FALSE );

BOOL QuestTaskHasTasksToComplete( UNIT *pPlayer,
								  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


BOOL QuestIsQuestComplete( UNIT *unit,
						   int questId );

BOOL QuestIsQuestTaskComplete( UNIT *unit,
						       int questTaskId );

BOOL QuestIsQuestTaskComplete( UNIT *unit,
						       const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );
BOOL QuestCanStart( GAME *pGame,
					UNIT *pPlayer,
					int questId,
					int questStartFlags = 0 );

BOOL QuestTaskCanStart( GAME *pGame,
						UNIT *pPlayer,
						int questId,
						int questStartFlags = 0 );


BOOL QuestIsMaskSet( QUEST_MASK_TYPES type,
						    UNIT *unit,
							const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

int QuestGetNumOfActiveQuests( GAME *pGame,
							   UNIT *pPlayer );




int QuestGetDialogID( GAME *pGame,
					  UNIT *unit,
					  const QUEST_TASK_DEFINITION_TUGBOAT *questTask,
					  BOOL ignoreComplete = FALSE );


BOOL QuestGetQuestAccepted( GAME *pGame,
							UNIT *unit,
							 const QUEST_TASK_DEFINITION_TUGBOAT *questTask );



int QuestGetAdditionItemWeight(	GAME *pGame,
								UNIT *unit,
								const UNIT_DATA *item );


const QUEST_TASK_DEFINITION_TUGBOAT * QuestGetUnitTaskByQuestQueueIndex( UNIT *unit,
																	int questQueueIndex );
int GetWeightAdditionalWeightOfUnit( GAME *pGame,
									 UNIT *pPlayer,
									 const QUEST_TASK_DEFINITION_TUGBOAT *questTask,
									 const UNIT_DATA *item );


void QuestSpawnQuestItems( GAME *pGame,
						   UNIT *unitDefender,
						   UNIT *unitAttacker,
						   UNIT *unitClient );






int QuestGetTotalNumberOfItemsNeeded( GAME *pGame,
									  UNIT *pUnit,
									  int itemID ); //returns the total number of an item needed for all tasks 

int QuestGetTotalNumberOfItemsToCollect( GAME *pGame,
									  UNIT *pUnit,
									  int itemID ); //returns the total number of an item needed for all tasks 



int s_QuestGetMonstersThatMustBeOnLevel( GAME *pGame,
										 int levelID );

int QuestGetAvailableNPCSecondaryDialogQuestTasks( GAME *pGame,
											       UNIT *pPlayer,
											       UNIT *pNPC,
												   const QUEST_TASK_DEFINITION_TUGBOAT **pQTArrayToFill,
												   int nOffset = 0);


int QuestGetAvailableNPCQuestTasks( GAME *pGame,
								   UNIT *pPlayer,
								   UNIT *pNPC,
								   const QUEST_TASK_DEFINITION_TUGBOAT **pQTArrayToFill,
								   BOOL bLowerLevelReq = TRUE,
								   int nOffset = 0 );


const QUEST_TASK_DEFINITION_TUGBOAT * QuestNPCGetQuestTask( GAME *pGame,
															UNIT *pPlayer,
															UNIT *pNPC,
															int nQuestID = INVALID_ID,
															BOOL bLowerLevelReq = TRUE);



//spawns item from a unit
inline UNIT * s_SpawnItemFromUnit( UNIT *unit,
								 UNIT *unitSpawningTo,
								 int itemClass );


BOOL QuestIsLevelAreaNeededByTask( UNIT *pPlayer,
								   int nLvlAreaId,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

//Checks to see if a dungeon is visible by a player for there active quests
BOOL sQuestIsLevelAreaNeededByQuestSystem( UNIT *pPlayer,
										   int nLvlAreaId );

//sets the Unit as a quest Unit
void s_SetUnitAsQuestUnit( UNIT *unit,		
								   UNIT *pClient,
								   const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
								   int nIndex );

//CHecks to see if the unit is a Quest Unit
inline BOOL QuestUnitIsQuestUnit( UNIT *unit );


BOOL QuestUnitCanDropQuestItem( UNIT *pItem );


//returns the QuestUnits Task ID
inline int QuestUnitGetTaskIndex( UNIT *pUnit )
{
	return UnitGetStat( pUnit, STATS_QUEST_TASK_REF );
}

//returns the questunits index id
inline int QuestUnitGetIndex( UNIT *pUnit )
{
	return UnitGetStat( pUnit, STATS_QUEST_ID_REF );
}

inline BOOL QuestTaskCompletedTextShowed( UNIT *pPlayer,
											const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	return QuestIsMaskSet( QUEST_MASK_COMPLETED_TEXT, pPlayer, pQuestTask );
}

//Returns the Quest Units Quest Task
const QUEST_TASK_DEFINITION_TUGBOAT *QuestUnitGetTask( UNIT *pUnit );

////////////////////////////////////////
//Updates the world, NPCs, Objects, Units and more

void QuestUpdateWorld( GAME *pGame,
					   UNIT *pPlayer,
					   LEVEL *pLevel,
					   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					   QUEST_WORLD_UPDATES worldUpdate,
					   UNIT *pObject );

void QuestPlayerComesOutOfLimbo( GAME *pGame,
								 UNIT *pPlayer );

void QuestUpdateNPCsInLevel( GAME *pGame,
  						     UNIT *pPlayer,
						     LEVEL *pLevel );
//End update of world functions
///////////////////////////////////////



BOOL QuestGetMaskRandomStat( UNIT *pUnit,
							  const QUEST_DEFINITION_TUGBOAT *pQuest,
							  const int &rndStatShift );

BOOL QuestTaskGetMask( UNIT *pUnit,
					   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					   const int &rndStatShift,					   
					   QUESTTASK_SAVE_OFFSETS nType);




BOOL QuestCanQuestMapDrop( UNIT *pPlayer,
						   UNIT *pUnit );


BOOL QuestCanGiveItemsOnAccept( UNIT *pPlayer,
								const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );



BOOL QuestWasSecondaryNPCMainNPCPreviously( GAME *pGame,
											  UNIT *pPlayer,
											  UNIT *pNPC,
											  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

//free's memory
void s_QuestForTugboatFree(
	GAME *pGame );

BOOL QuestUpdateQuestNPCs( GAME *pGame,
						   UNIT *pPlayer,
						   UNIT *pNPC );

BOOL QuestNPCSecondaryNPCHasBeenVisited( UNIT *pPlayer,
										 UNIT *pNPC,
										 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );		

BOOL QuestGetIsTaskComplete( UNIT *unit,	//unit trying to complete quest
							 const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask );

BOOL QuestCanNPCEndQuestTask( UNIT *pNPC,
							  UNIT *pPlayer,
						      const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL QuestNPCIsASecondaryNPCToTask( GAME *pGame,
								    UNIT *pNPC,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


BOOL QuestHasBeenVisited( UNIT *pPlayer,
						  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );



int QuestGetMaskForTask( QUEST_MASK_TYPES type,
								const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

void QuestGetThemes( LEVEL *pLevel );

BOOL QuestHasRewards( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL QuestCanAbandon( 
	int nQuestId );


int QuestGetPossibleTasks( UNIT *pPlayer,
							UNIT *pNPC );



BOOL QuestValidateItemForQuestTask( UNIT *pPlayer,
									UNIT *pItem,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );



//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#endif
