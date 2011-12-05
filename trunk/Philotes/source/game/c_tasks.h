//----------------------------------------------------------------------------
// FILE: c_tasks.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __C_TASKS_H_
#define __C_TASKS_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __TASKS_H_
#include "tasks.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct MSG_SCMD_AVAILABLE_TASKS;
struct MSG_SCMD_TASK_STATUS;
struct UI_COMPONENT;
struct MSG_SCMD_TASK_INCOMPLETE;			//tugboat
struct MSG_SCMD_TASK_EXTERMINATED_COUNT;

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void c_TasksInitForGame(
	GAME *pGame);

void c_TasksCloseForGame(
	GAME *pGame);

void c_TasksRestoreTask( 
	const TASK_TEMPLATE* pTaskTemplate,
	TASK_STATUS eStatus);
		
void c_TaskAvailableTasks( 
	const MSG_SCMD_AVAILABLE_TASKS *pMessage);

void CQuestAvailableQuests( GAME *pGame,
							const MSG_SCMD_AVAILABLE_QUESTS *pMessage );

void c_TasksSetStatus( 
	const MSG_SCMD_TASK_STATUS *pMessage);

void c_TasksIncomplete( 
	const MSG_SCMD_TASK_INCOMPLETE *pMessage);

void c_TasksTryAccept(
	void);

void c_TasksTryReject(
	void);

BOOL c_TasksTryAbandon(
	void);

void c_TasksTryAcceptReward(
	void);

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
void c_UpdateActiveTaskButtons(
	void);

void c_TasksOfferReward(
	UNITID idTaskGiver,
	GAMETASKID idTaskServer);

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
void c_TasksSetActiveTask(
	GAMETASKID idTaskServer);

void c_TaskUpdateNPCs( 
	GAME *pGame,
	ROOM* pRoom );

void c_TasksSetActiveTaskByIndex(
	int nIndex);

void c_TaskUpdateNPCs( 
	GAME *pGame,
	LEVEL* pLevel );

void c_TBRefreshTasks( int nTaskIndex );


//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------

void c_TasksCloseTask(
	GAMETASKID idTaskServer);

void c_TasksActiveTaskPDAOnActivate(
	void);

void c_TasksTalkingTo( 
	UNITID idTalkingTo);

void c_TasksUpdateExterminatedCount(
	MSG_SCMD_TASK_EXTERMINATED_COUNT* pMessage);

void c_TaskRewardTaken(
	GAMETASKID idTaskServer,
	UNITID idReward);
	
void c_TasksRestrictedByFaction(
	UNITID idGiver,
	int *pnFactionBad);
	
BOOL c_TaskIsTaskActive(
	void);

BOOL c_TaskIsTaskActiveInThisLevel(
	void);

void c_TaskPrintActiveTaskTitle( 
	WCHAR *puszBuffer, 
	int nBufferSize);

void c_TaskPrintActiveTaskInfo( 
	WCHAR *puszBuffer, 
	int nBufferSize);
			
#endif  // end __C_TASKS_H_
