//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __S_QUESTSERVER_H_
#define __S_QUESTSERVER_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __DIALOG_H_
#include "dialog.h"
#endif

#ifndef _LEVEL_H_
#include "level.h"
#endif

#ifndef __INTERACT_H_
#include "interact.h"
#endif

#ifndef __QUEST_H_
#include "Quest.h"
#endif

#ifndef __s_QUESTNPCSERVER_H_
#include "s_QuestNPCServer.h"
#endif

#ifndef __S_GAMEMAPSSERVER_H_
#include "s_GameMapsServer.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
struct ROOM;

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//call this when the player accepted the quest
void s_QuestAccepted( GAME *pGame,
							  UNIT *pPlayer,	
							  UNIT *pQuestBearer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *questTask );


void s_QuestSetMask( QUEST_MASK_TYPES type,
				  UNIT *pPlayer,
				  const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

inline void s_QuestTaskCompletedTextSetShowed( UNIT *pPlayer,
											   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	s_QuestSetMask( QUEST_MASK_COMPLETED_TEXT, pPlayer, pQuestTask );
}


const QUEST_TASK_DEFINITION_TUGBOAT * s_QuestUpdate( GAME *pGame,
													 UNIT *pPlayer,
													 UNIT *pQuestBearer,
													 int questId,
													 BOOL &bComplete );
void s_QuestUpdate( GAME *pGame,
						   UNIT *pPlayer,
						   UNIT *pQuestBearer );
USE_RESULT s_QuestsTBUseItem(
	UNIT * pPlayer,
	UNIT * pItem );



void s_QuestSetMaskRandomStat( UNIT *pUnit,
							   const QUEST_DEFINITION_TUGBOAT *pQuest,
							   const int &rndStatShift );

void s_QuestTaskSetMask( UNIT *pUnit,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuest,
					       const int &rndStatShift,
						   QUESTTASK_SAVE_OFFSETS nType );

void s_QuestTaskClearMask( UNIT *pUnit,
					     const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					     const int &rndStatShift,
						 QUESTTASK_SAVE_OFFSETS nType );

//this zero's out the task. This needs to happen when a task is complete and is moving onto another...
void s_QuestTaskClearMask( UNIT *pUnit,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuest );

void s_QuestLevelPopulate( GAME *pGame,
						  LEVEL *pLevelCreated,
						  UNIT *pPlayer );

void s_QuestLevelActivated( GAME *pGame,
						    UNIT *pUnit,
							LEVEL *pLevel );	

						  
//Creates a quest unit by it's unit data
BOOL s_QuestCreateItemQuestUnit( UNIT *pPlayer,
								 int nItemClass,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								 int nQuestUnitIndex,
								 BOOL givePlayer = TRUE );
//Gives the player items for the quest
int s_QuestGiveItemsOnAccepted( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

int s_QuestCompleteTaskForPlayer( GAME *pGame,
								   UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT * questTask,
								   int nRewardSelection,
								   BOOL bForce = FALSE );





//Rewards the player for a quest complete
int s_QuestRewardPlayerForTaskComplete( GAME *pGame,
									   UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT * questTask,
									   int nRewardSelection );
//Sets the quest as visited
void s_QuestSetVisited( UNIT *pPlayer,						
						const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

void s_QuestSetVistedClear( UNIT *pPlayer,						
							const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestTaskPotentiallyComplete( UNIT *pPlayer );

void s_QuestTaskPotentiallyComplete( UNIT *pPlayer,
									 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestClearFromUser( UNIT *unit,
				   const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

void s_QuestPlayerTalkedToQuestNPC( UNIT *pPlayer,
								    UNIT *pNPC,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nDialogID,
									int nRewardSelection );

BOOL s_QuestItemPickedUp( UNIT *itemPickedUp,
						  UNIT *playerPickingUp );

void s_SetTaskObjectCompleted( GAME *pGame,
							  UNIT *playerUnit,
							  const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
							  int ObjectIndex,
							  BOOL Monster );

void s_QuestNoneQuestItemNeedsToBeQuestItem( UNIT *pPlayer,
											 UNIT *pUnit );

void s_QuestRemoveItemsOfTypeQuestComplete( UNIT *pPlayer,
										    const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
											UNITTYPE nType );

BOOL s_QuestIsWaitingToGiveRewards( UNIT *pPlayer,
								    const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask );

int s_CreateMapsForPlayerOnAccept( UNIT *pPlayer,
									const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
									BOOL bGivePlayer,
									BOOL bPlaceInWorld,
									UNIT **pMapArrayBack);

void s_QuestDialogMakeRewardItemsAvailableToClient( UNIT *pPlayer,
													const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													BOOL bMakeAvailable );

void s_QuestCreateRewardItems( UNIT *pPlayer,
							   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

BOOL s_QuestRewardItemsExist( UNIT *pPlayer,
							 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestTimerUpdate( UNIT *pPlayer,
						 int nQuest,
						 int nCurrentValue,
						 int nNewValue );
						 
void s_QuestTimerStart( UNIT *pPlayer,
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestTimerClear( UNIT *pPlayer,
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestBroadcastMessageOnObjectOperate( UNIT *unitSpawning,									    
											 UNIT *pPlayer );

BOOL s_ObjectOperatedUseSkill( UNIT *pPlayer,
							  UNIT *pObject );

BOOL s_ObjectRemoveItemOnInteract( UNIT *pPlayer,
								   UNIT *pObject );

BOOL QuestRemoveItemsFromPlayer( GAME *pGame,
								UNIT *pPlayer,
								int nItemClass,
								int nCount,
								DWORD dwFlags = 0 );

void RemoveQuestTaskItems( GAME *pGame,
						  UNIT *unit,
						  const QUEST_TASK_DEFINITION_TUGBOAT *questTask );

void s_QuestRegisterItemCreationOnUnitDeath( GAME *pGame,
											   LEVEL *pLevelCreated,
											   UNIT *pPlayer );

void QuestAssignQueueSpot( GAME *pGame,
						   UNIT *unit,
						   int questId );

void QuestRemoveFromQueueSpot( GAME *pGame,
							   UNIT *unit,
							   int questId );

//sets the quest ID
void QuestSetQuestIDByQuestQueueIndex( UNIT *unit, int questIndex, int questID ); 

void QuestStampRandomQuestSeed( UNIT *pPlayer, 
								 int nSeed,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void QuestStampRandomQuestSeed( UNIT *pPlayer, 
								 UNIT *pNPC,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuest );

void QuestSetRandomQuestFree( UNIT *pPlayer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestSendMessageBossDefeated( UNIT *pPlayer,
									 UNIT *pMonster,
									 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void s_QuestSendMessageLevelBeingRemoved( UNIT *pPlayer, LEVEL *pLevel );


void s_QuestRoomAsBeenActivated( ROOM *pRoom );

void s_QuestAddTrackableQuestUnit( UNIT *pUnit );

void s_QuestPlayerLeftArea( UNIT *pPlayer, int nLevelAreaAt, int nLevelAreaGoingTo );

#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
extern BOOL				gQuestForceComplete;
#endif
#endif