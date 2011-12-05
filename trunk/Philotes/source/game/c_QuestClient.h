//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#ifndef __c_QUEST_H_
#define __c_QUEST_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __QUEST_H_
#include "Quest.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
//STATIC FUNCTIONS and Defines
struct UI_COMPONENT;
class UI_LINE;

const static int			QUEST_FLAG_FILLTEXT_TASKS_OBJECTS			= MAKE_MASK( 1 );
const static int			QUEST_FLAG_FILLTEXT_TASKS_ITEMS				= MAKE_MASK( 2 );
const static int			QUEST_FLAG_FILLTEXT_TASKS_BOSSES			= MAKE_MASK( 3 );
const static int			QUEST_FLAG_FILLTEXT_SHOW_ALL_TASKS			= QUEST_FLAG_FILLTEXT_TASKS_OBJECTS | QUEST_FLAG_FILLTEXT_TASKS_ITEMS | QUEST_FLAG_FILLTEXT_TASKS_BOSSES;
const static int			QUEST_FLAG_FILLTEXT_QUEST_ITEM				= MAKE_MASK( 4 );
const static int			QUEST_FLAG_FILLTEXT_OBJECT_INTERACTED		= MAKE_MASK( 5 ) | QUEST_FLAG_FILLTEXT_QUEST_ITEM;
const static int			QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES		= MAKE_MASK( 6 );
const static int			QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_COMPLETE	= MAKE_MASK( 7 );
const static int			QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_ACCEPT   = MAKE_MASK( 8 );
const static int			QUEST_FLAG_FILLTEXT_QUEST_DIALOGS		    = MAKE_MASK( 9 );
const static int			QUEST_FLAG_FILLTEXT_QUEST_LOG				= MAKE_MASK( 10 ) | QUEST_FLAG_FILLTEXT_QUEST_DIALOGS | QUEST_FLAG_FILLTEXT_SHOW_ALL_TASKS | QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES;
const static int			QUEST_FLAG_FILLTEXT_NO_COLOR			    = MAKE_MASK( 11 );



const static int			QUEST_UI_SHOW_ON_SCREEN_COUNT = 2048;
const static int			QUEST_MAX_POINTS_OF_INTEREST = 500;
static int					QUEST_UI_POINTS_OF_INTEREST_COUNT = 0;	
static int					QUEST_UI_POINTS_OF_INTEREST[ QUEST_MAX_POINTS_OF_INTEREST ];
static BOOL					QUEST_UI_SHOW_ON_SCREEN[ QUEST_UI_SHOW_ON_SCREEN_COUNT ];
static BOOL					QUEST_UI_SHOW_BOSS_DEFEATED[ QUEST_UI_SHOW_ON_SCREEN_COUNT ][MAX_OBJECTS_TO_SPAWN];

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct QUEST_FILLTEXT_DATA
{
	QUEST_FILLTEXT_DATA( UNIT *player,
						 WCHAR *szText, 
						 int len, 						 
						 const QUEST_TASK_DEFINITION_TUGBOAT *questTask,
						 int flags = 0, 
						 UNIT *unit = NULL, 
						 const UNIT_DATA *unitData = NULL )
	{
		pPlayer = player;
		szFlavor = szText;
		nTextLen = len;
		nFlags = flags;
		pUnitData = unitData;
		pUnit = unit;
		pQuest = NULL;
		pQuestTask = questTask;
		if( pUnitData == NULL && pUnit != NULL )
			pUnitData = pUnit->pUnitData;
		if( pQuestTask != NULL )
			pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	}
	int			nFlags;
	UNIT		*pPlayer;
	UNIT		*pUnit;
	const UNIT_DATA						*pUnitData;
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask;
	const QUEST_DEFINITION_TUGBOAT *pQuest;
	WCHAR		*szFlavor;
	int			nTextLen;
};



//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
//returns the index of the point of interest
int c_QuestGetPointsOfInterestByIndex( UNIT *pPlayer, int nIndex );

//caculates the points of interest
int c_QuestCaculatePointsOfInterest( UNIT *pPlayer );


void c_QuestFillRandomQuestTagsUIDialogText( UI_COMPONENT *pUITextComponent,
											 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );


void c_QuestHandleQuestTaskMessage( GAME *pGame,
								    MSG_SCMD_QUEST_TASK_STATUS *message );


int c_QuestGetFlavoredText( UNIT *pUnit,
						  UNIT *pPlayer );

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  int nItemClassID,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags );

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  UNIT_DATA *pUnit,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags );

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  UNIT *pUnit,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags );

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,							  
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags );

void c_QuestFillFlavoredText( const QUEST_DEFINITION_TUGBOAT *pQuest,
							  UNIT_DATA *pUnitData,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags );

void c_QuestFillDialogText( UNIT *pPlayer,
						    UI_LINE *pLine );

BOOL c_QuestUpdateQuestNPCs( GAME *pGame,
							 UNIT *pPlayer,
							 UNIT *pNPC );


int c_GetQuestNPCState( GAME *pGame,
					   UNIT *pPlayer,
					   UNIT *pNPC,
					   const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask);

void c_QuestDisplayMessage( GAME *pGame,						    
							UNIT *pTarget,
							const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							EQUEST_TASK_MESSAGES message );

void c_QuestDisplayQuestUnitInfo( GAME *pGame,
								  UNIT *pItem,
								  EQUEST_TASK_MESSAGES message );

void c_QuestRewardUpdateUI( UI_COMPONENT* pComponent,
							BOOL bAllowRewardSelection,
							BOOL bTaskScreen,
							int nQuestDefID );

FONTCOLOR c_QuestGetDifficultyTextColor( UNIT *pPlayer,
										const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );

void c_QuestsFreeClientLocalValues( UNIT *pPlayer );

BOOL c_QuestShowsInGame( int nQuestID );
BOOL c_QuestShowsInGame( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask );
void c_QuestSetQuestShowInGame( int nQuestID, BOOL bShow );
void c_QuestSetQuestShowInGame( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask, BOOL bShow );
void c_QuestClearBossDefeated( int nQuestID = INVALID_ID );
void c_QuestUpdateAfterChangingInstances( int nQuestID = INVALID_ID );
void c_QuestsUpdate( BOOL bUpdateAllObjectsInWorld,
					 BOOL bUpdateAllNPCS );


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#endif
