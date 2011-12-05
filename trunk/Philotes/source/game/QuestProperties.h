// FILE: QuestProperties
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __QUESTPROPERTIES_H_
#define __QUESTPROPERTIES_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _GAME_H_
#include "game.h"
#endif

#ifndef _LEVEL_H_
#include "level.h"
#endif

#ifndef _ROOM_H_
#include "room.h"
#endif

#ifndef _EXCEL_H_
#include "excel.h"
#endif

#ifndef __QUESTS_H_
#include "quests.h"
#endif

#ifndef __QUESTSTRUCTS_H_
#include "QuestStructs.h"
#endif


namespace MYTHOS_QUESTS
{
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
int QuestGetStringOrDialogIDForQuest( UNIT *pPlayer,
									  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									  EQUEST_STRINGS nStringType );

int QuestGetBossIDToSpawnByIndex( UNIT *pPlayer,
								  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								  int nIndex );

int QuestGetBossPrefixNameByIndex( UNIT *pPlayer,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nIndex );

int QuestGetBossUniqueNameByIndex( UNIT *pPlayer,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nIndex );

int QuestGetBossQualityIDToSpawnByIndex( UNIT *pPlayer,
										 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										 int nIndex );

BOOL QuestGetBossNotNeededToCompleteQuest( UNIT *pPlayer,
										  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										  int nIndex );

BOOL QuestRequiresBossOrObjectInteractionToComplete( UNIT *pPlayer,
													 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL QuestBossShowsOnTaskScreen( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								 int nIndex );

int QuestGetMonsterMinionIDToSpawnByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										   int nIndex );

int QuestGetBossItemFlavorIDByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								int nIndex );

int QuestGetBossSpawnDungeonByIndex( UNIT *pPlayer,
									 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									 int nIndex );

int QuestGetBossSpawnDungeonFloorByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										  int nIndex );

int QuestGetBossIDSpawnsItemIndex( UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								   int nIndex );

int QuestGetObjectIDToSpawnByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nIndex );

int QuestGetObjectSpawnDungeonByIndex( UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									   int nIndex );

int QuestGetObjectSpawnDungeonFloorByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									        int nIndex );

int QuestGetObjectSpawnItemToSpawnByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										   int nIndex );

int QuestGetObjectSpawnItemDirectToPlayerByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												  int nIndex );


int QuestGetLevelAreaIDOnTaskAcceptedByIndex( UNIT *pPlayer,
	                                          const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
											  int nIndex );

int QuestGetRecipeIDOnTaskAcceptedByIndex( UNIT *pPlayer,
										   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										   int nIndex );

int QuestGetItemIDToCollectByIndex( UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								   int nIndex );

int QuestGetNumberOfItemsToCollectByIndex( UNIT *pPlayer,
										   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										   int nIndex );

int QuestGetNumberToCollectForItem( UNIT *pPlayer,
								    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nItemClassID );

int QuestGetItemSpawnFromUnitTypeByIndex( UNIT *pPlayer,
										  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										  int nIndex );

int QuestGetItemSpawnFromMonsterClassByIndex( UNIT *pPlayer,
											  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										      int nIndex );

int QuestGetItemSpawnDropsInDungeonByIndex( UNIT *pPlayer,
										    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										    int nIndex );

int QuestGetItemSpawnChance( UNIT *pPlayer,
							 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							 int nIndex );

int QuestGetRewardLevel( UNIT *pPlayer,
						 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int QuestGetExperienceEarned( UNIT *pPlayer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );
int QuestGetGoldEarned( UNIT *pPlayer,
					   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


int QuestGetRandomQuestRewardTreasure( UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int QuestGetPlayerOfferRewardsIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


int QuestGetTimeDurationMS( UNIT *pPlayer,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int QuestGetTimeRemainingMS( UNIT *pPlayer,
						    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int QuestGetLevelAreaDifficultyOverride( UNIT *pPlayer,
										 int nLevelAreaID,
										 BOOL askForMax );

int QuestGetObjectInteractedStringID( UNIT *pQuestUnitObject );
int QuestGetObjectInteractedStringID( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask, int nObjectIndex );

int QuestObjectGetSkillIDOnInteract( UNIT *pQuestUnitObject );

int QuestObjectGetItemToRemoveOnInteract( UNIT *pQuestUnitObject );

int QuestGetQuestQueueIndex( UNIT *pPlayer,
							 int nQuestID );

int QuestGetQuestQueueIndex( UNIT *pPlayer,
							 const QUEST_DEFINITION_TUGBOAT *pQuest );



//gets the quest ID
int QuestGetQuestIDByQuestQueueIndex( UNIT *unit, int questIndex ); 


int QuestGetQuestQueueIndex( UNIT *pPlayer,
							 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int QuestGetQuestInQueueByOrder( UNIT *pPlayer,
								 int nOrderIndex );

int QuestGetMinLevel( UNIT *pPlayer,
					  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


int QuestGetMaxLevel( UNIT *pPlayer,
					  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL QuestTestFlag( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
				    EQUESTTB_FLAGS nFlag );

int QuestGetPointOfInterestByIndex( UNIT *pPlayer,
								    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									BOOL bCheckObjects,
									int nIndex );
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
}//end namespace
#endif