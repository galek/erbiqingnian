//----------------------------------------------------------------------------
// FILE: c_Tasks - Client side task logic
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "c_font.h"
#include "clients.h"
#include "c_message.h"
#include "c_tasks.h"
#include "console.h"
#include "fontcolor.h"
#include "game.h"
#include "globalIndex.h"
#include "items.h"
#include "log.h"
#include "units.h"
#include "monsters.h"
#include "s_message.h"
#include "stringtable.h"
#include "dialog.h"
#include "tasks.h"
#include "uix.h"
#include "uix_components.h"
#include "unit_priv.h"
#include "c_sound.h"		//tugboat
#include "c_sound_util.h"	//tugboat
#include "globalindex.h"
#include "s_reward.h"
#include "states.h"			//tugboat
#include "npc.h"
#include "states.h"
#include "c_QuestClient.h"			// tugboat
#include "LevelAreas.h"
#include "c_LevelAreasClient.h"
#include "QuestProperties.h"
#include "stringreplacement.h"
#include "QuestStructs.h"

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum TASK_UI
{
	TASK_UI_NEW_TASK,				// new task ui for when we are offered new tasks
	TASK_UI_ACTIVE_TASK,			// task ui for tasks we already have
	TASK_UI_TASK_SCREEN,			// task ui for active quest screen (Tugboat)
};

//----------------------------------------------------------------------------
struct CTASK
{
	TASK_TEMPLATE tTaskTemplate;// particulars of task
	TASK_STATUS eStatus;		// status
	DWORD dwAcceptedTick;		// tick task was accepted
};

//----------------------------------------------------------------------------
struct CLIENT_TASK_GLOBALS
{

	CTASK activeTasks[ MAX_ACTIVE_TASKS ];	// active tasks
	int nNumActiveTasks;			// count of activeTask[]
	GAMETASKID idActiveTaskCurrent;		// "current" active task to see in UI
	GAMETASKID idActiveQuestCurrent;		// "current" active quest to see in UI
	
	TASK_TEMPLATE availableTasks[ MAX_AVAILABLE_TASKS_AT_GIVER ];	// available tasks
	int nNumAvailableTasks;			// count of availableTasks[]
	GAMETASKID idAvailableTaskCurrent;	// "current" available task to see in UI
	GAMETASKID idAvailableQuestCurrent;	// "current" available quest to see in UI
	UNITID	idAvailableQuestGiver;	// current available questgiver
			
};

//----------------------------------------------------------------------------
enum TASK_DESCRIPTION_PART
{
	TDP_TITLE_BIT,
	TDP_DESC_BIT,
	TDP_REWARD_IMAGE_BIT,
	TDP_REWARD_TEXT_BIT,
	TDP_REWARD_HEADER_BIT,
	TDP_STATUS_BIT,
	TDP_STATUS_HEADER_BIT,
	TDP_NO_NEWLINES_BIT,
};

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------
static CLIENT_TASK_GLOBALS sgClientTaskGlobals;

//----------------------------------------------------------------------------
struct TASK_STATUS_LABEL
{
	TASK_STATUS eStatus;
	const char *pszStringKey;
};
static TASK_STATUS_LABEL sgTaskStatusLabelTable[] = 
{
	{ TASK_STATUS_GENERATED,	NULL },	// no ui representation
	{ TASK_STATUS_ACTIVE,		"task status active" },
	{ TASK_STATUS_COMPLETE,		"task status complete" },
	{ TASK_STATUS_FAILED,		"task status failed" },
	{ TASK_STATUS_REWARD,		"task status complete" },	
	
	{ TASK_STATUS_INVALID,		NULL },  // keep this last
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CLIENT_TASK_GLOBALS *sClientTaskGlobalsGet(
	void)
{
	return &sgClientTaskGlobals;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientTaskInit( 
	CTASK *pCTask)
{
	pCTask->eStatus = TASK_STATUS_INVALID;
	pCTask->dwAcceptedTick = 0;
	TaskTemplateInit( &pCTask->tTaskTemplate );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientTaskGlobalsInit(
	CLIENT_TASK_GLOBALS *pCTaskGlobals)
{
	pCTaskGlobals->nNumActiveTasks = 0;
	pCTaskGlobals->idActiveTaskCurrent = GAMETASKID_INVALID;
	pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
	pCTaskGlobals->nNumAvailableTasks = 0;
	pCTaskGlobals->idAvailableTaskCurrent = GAMETASKID_INVALID;
	pCTaskGlobals->idAvailableQuestCurrent = GAMETASKID_INVALID;
	pCTaskGlobals->idAvailableQuestGiver = INVALID_ID;
	
	for (int i = 0; i < TaskMaxTasksPerPlayer(); ++i)
	{
		CTASK *pCTask = &pCTaskGlobals->activeTasks[ i ];
		sClientTaskInit( pCTask );
	}	
	
	for (int i = 0; i < MAX_AVAILABLE_TASKS_AT_GIVER; ++i)
	{
		TASK_TEMPLATE *pTaskTemplate = &pCTaskGlobals->availableTasks[ i ];
		TaskTemplateInit( pTaskTemplate );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksInitForGame(
	GAME *pGame)
{
	
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	sClientTaskGlobalsInit( pCTaskGlobals );

	// hide accept button on reward interface	
	//UIComponentSetVisibleByEnum(UICOMP_TASK_BUTTON_ACCEPT_REWARD, FALSE);					

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksCloseForGame(
	GAME *pGame)
{
	if (IS_SERVER( pGame ))
	{
		return;
	}
	
	// do stuff here maybe
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CTASK *sCTaskFind( 
	const GAMETASKID idTaskServer)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	
	for (int i = 0; i < pCTaskGlobals->nNumActiveTasks; ++i)
	{
		CTASK *pCTask = &pCTaskGlobals->activeTasks[ i ];
		if (pCTask->tTaskTemplate.idTask == idTaskServer)
		{
			return pCTask;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TASK_TEMPLATE *sAvailableTaskFind( 
	GAMETASKID idTaskServer)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	
	for (int i = 0; i < pCTaskGlobals->nNumAvailableTasks; ++i)
	{
		TASK_TEMPLATE *pTaskTemplate = &pCTaskGlobals->availableTasks[ i ];
		if (pTaskTemplate->idTask == idTaskServer)
		{
			return pTaskTemplate;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//tugboat
//----------------------------------------------------------------------------
void c_UpdateActiveTaskButtons(
	void)
{

	// if the player is talking to the giver of this task, and the task is complete
	// the take reward button is available, otherwise it's hidden
	BOOL bTaskExsists = FALSE;
	BOOL bShowAbandonButton = FALSE;
//	BOOL bShowRewardButton = FALSE;
	GAME *pGame = AppGetCltGame();
	
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit( pGame );
		if (pPlayer)
		{
			CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
			
			// get the active task
			CTASK *pCTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
			if( MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pCTaskGlobals->idActiveQuestCurrent ) != INVALID_ID )
			{
				// we have a task, we can abandon it now
				bTaskExsists = TRUE;
				bShowAbandonButton = TRUE;
			}
			else if (pCTask != NULL)
			{
			
				// we have a task, we can abandon it now
				bTaskExsists = TRUE;
				bShowAbandonButton = TRUE;
							
			}

			
		}
		
	}

	// show or hide ui pieces for having a task at all
	UIComponentSetVisibleByEnum( UICOMP_TASK_DESC_LABEL_HEADER, bTaskExsists );
	UIComponentSetVisibleByEnum( UICOMP_TASK_REWARD_LABEL_HEADER, bTaskExsists );
	UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS_HEADER, bTaskExsists );
	UIComponentSetVisibleByEnum( UICOMP_TASK_NO_TASK_LABEL, bTaskExsists == FALSE );	
	UIComponentSetVisibleByEnum( UICOMP_TASK_GOLDREWARD, bTaskExsists );
	UIComponentSetVisibleByEnum( UICOMP_TASK_EXPERIENCEREWARD, bTaskExsists );
		
	// show or hide buttons
	//UIComponentSetActiveByEnum(UICOMP_TASK_BUTTON_ACCEPT_REWARD, bShowRewardButton);		
	//UIComponentSetVisibleByEnum(UICOMP_TASK_BUTTON_ACCEPT_REWARD, bShowRewardButton);		

	UIComponentSetActiveByEnum(UICOMP_TASK_BUTTON_ABANDON, bShowAbandonButton);		
	UIComponentSetVisibleByEnum(UICOMP_TASK_BUTTON_ABANDON, bShowAbandonButton);		

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sTaskGetTitleString( 
	const TASK_TEMPLATE *pTaskTemplate, 
	WCHAR *puszTitleBuffer, 
	int nTitleBufferSize)
{
	const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pTaskTemplate->wCode );
	ASSERT_RETURN( pTaskDefinition );

	// get title
	PStrPrintf( 
		puszTitleBuffer, 
		nTitleBufferSize, 
		StringTableGetStringByIndex( pTaskDefinition->nNameStringKey ) );
		
}	

//----------------------------------------------------------------------------
struct TASK_DESCRIPTION_REPLACEMENT
{
	const WCHAR *puszToken;
	const WCHAR *puszReplacement;
	FONTCOLOR eColor;
};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
 static void sTaskGetDescString(
	const TASK_TEMPLATE *pTaskTemplate,
	WCHAR *puszDescBuffer,
	int nDescBufferSize )
{		
	#define MAX_TASK_ELEMENT_STRING_LENGTH (256)
   	
   	GAME *pGame = AppGetCltGame();
   	const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pTaskTemplate->wCode );
   	ASSERT_RETURN( pTaskDefinition );
   			
   	// build string of targets
   	WCHAR uszTargets[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszTargets[ 0 ] = 0;
   	BOOL bFirstTarget = TRUE;
   	for (int i = 0; i < MAX_TASK_UNIT_TARGETS; ++i)
   	{
   		const PENDING_UNIT_TARGET *pPendingUnitTarget = &pTaskTemplate->tPendingUnitTargets[ i ];
   		
   		// skip invalid targets
   		if (pPendingUnitTarget->speciesTarget == SPECIES_NONE)
   		{
   			continue;
   		}
   				
   		// look for unique names
   		if (pPendingUnitTarget->nTargetUniqueName != MONSTER_UNIQUE_NAME_INVALID)
   		{
   			// for now we only support monster targets - this is just to remind me to fix this if we have other targets someday - colin
   			GENUS eGenusTarget = (GENUS)GET_SPECIES_GENUS( pPendingUnitTarget->speciesTarget );
   			int nClassTarget = GET_SPECIES_CLASS( pPendingUnitTarget->speciesTarget );
   			ASSERTX_RETURN( eGenusTarget == GENUS_MONSTER, "Only monster targets are supported right now" );
   			
   			// add to descriptive string of all our targets			
   			#define MAX_NAME_LENGTH	(128)
   			WCHAR uszNameWithTitle[ MAX_NAME_LENGTH ];
   			MonsterGetUniqueNameWithTitle( 
   				pPendingUnitTarget->nTargetUniqueName, 
   				nClassTarget, 
   				uszNameWithTitle,
   				MAX_NAME_LENGTH );
   
   			// add + between additional targets				
   			if (bFirstTarget == FALSE)
   			{
				if( AppIsHellgate() )
				{
   					PStrCat( uszTargets, L" + ", MAX_TASK_ELEMENT_STRING_LENGTH );			
				}
				else
				{
					PStrCat( uszTargets, L"s and ", MAX_TASK_ELEMENT_STRING_LENGTH );
					PStrCat( uszTargets, uszNameWithTitle, MAX_TASK_ELEMENT_STRING_LENGTH );
					PStrCat( uszTargets, L"s", MAX_TASK_ELEMENT_STRING_LENGTH );
				}
   			}
			else if( AppIsTugboat() )
			{
				PStrCopy( uszTargets, uszNameWithTitle, MAX_TASK_ELEMENT_STRING_LENGTH );
			}			
   			// add name
			if( AppIsHellgate() )
			{
   				PStrCat( uszTargets, uszNameWithTitle, MAX_TASK_ELEMENT_STRING_LENGTH );			
			}
   			
   			// not the first target
   			bFirstTarget = FALSE;
   						
   		}
   		
   	}
   
   	// get level string
   	WCHAR uszLevel[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszLevel[ 0 ] = 0;  // init to empty string	
   	if ( AppIsHellgate() && pTaskTemplate->nLevelDefinition != INVALID_LINK)
   	{
   		const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pTaskTemplate->nLevelDefinition );
   		ASSERTX( pLevelDefinition, "Unable to get level definition for task" );
   		PStrCopy( 
   			uszLevel, 
   			StringTableGetStringByIndex( pLevelDefinition->nLevelDisplayNameStringIndex ), 
   			MAX_TASK_ELEMENT_STRING_LENGTH );
   	}
	else if( AppIsTugboat() )
	{
		PStrPrintf( uszLevel, MAX_TASK_ELEMENT_STRING_LENGTH, L"%d", pTaskTemplate->nLevelDepth );
	}
   	
	WCHAR uszArea[ MAX_TASK_ELEMENT_STRING_LENGTH ];
	uszArea[ 0 ] = 0;  // init to empty string	
	if( AppIsTugboat() )
	{
		//ASSERTX((unsigned int)pTaskTemplate->nLevelArea < ExcelGetCount(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_LEVEL_AREAS ), "Invalid area!");		
		//const WCHAR * puszAreaName = StringTableGetStringByIndex( MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( pTaskTemplate->nLevelArea ) );
		WCHAR puszAreaName[ 1024 ];
		MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( pTaskTemplate->nLevelArea, -1, &puszAreaName[0], 1024 );

		PStrCvt( uszArea, &puszAreaName[0], MAX_TASK_ELEMENT_STRING_LENGTH );
	}

	// how many different item types are there to collect
	int nNumCollectTypes = 0;
	for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
	{
   		const TASK_COLLECT* pCollect = &pTaskTemplate->tTaskCollect[ i ];
   		if (pCollect->nCount > 0)
   		{
   			nNumCollectTypes++;
   		}	
	}
	
   	// build string of things to collect
   	WCHAR uszCollect[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszCollect[ 0 ] = 0;
   	int nCurrentCollect = 0;
   	for (int i = 0; i < MAX_TASK_COLLECT_TYPES; ++i)
   	{
   		const TASK_COLLECT* pCollect = &pTaskTemplate->tTaskCollect[ i ];
   		if (pCollect->nCount > 0)
   		{
   		
   			// get name
   			DWORD dwFlags = 0;
   			SETBIT( dwFlags, INIF_MODIFY_NAME_BY_ANY_QUANTITY );
   			ITEM_NAME_INFO tItemNameInfo;
   			ItemGetNameInfo( tItemNameInfo, pCollect->nItemClass, pCollect->nCount, dwFlags );		
   			   			   				
   			if (nCurrentCollect == 0)
   			{
   				// this is the first item in the list
   				PStrCopy( uszCollect, tItemNameInfo.uszName, MAX_TASK_ELEMENT_STRING_LENGTH );
   			}
   			else
   			{
   			
   				// get the separator, this is the nth or the last item in the list
				const WCHAR *puszSep = NULL;
   				if (nCurrentCollect == nNumCollectTypes - 1)
   				{
   					puszSep = GlobalStringGet( GS_ITEM_LIST_SEPARATOR_LAST );  // last collect
   				}
   				else
   				{
   					puszSep = GlobalStringGet( GS_ITEM_LIST_SEPARATOR );  // nth collect
   				}

				// cat the separator
				PStrCat( uszCollect, puszSep, MAX_TASK_ELEMENT_STRING_LENGTH );				   					   				
   				
   				// cat the name
   				PStrCat( uszCollect, tItemNameInfo.uszName, MAX_TASK_ELEMENT_STRING_LENGTH );
   			}
   			nCurrentCollect++;
   		}
   				
   	}
   
   	// build string for time limit
   	WCHAR uszTimeLimit[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszTimeLimit[ 0 ] = 0;
   	if (pTaskTemplate->nTimeLimitInMinutes > 0)
   	{
   		const WCHAR* puszMinutesFormat = GlobalStringGet( GS_TIME_MINUTES );
   		PStrPrintf( uszTimeLimit, MAX_TASK_ELEMENT_STRING_LENGTH, puszMinutesFormat, pTaskTemplate->nTimeLimitInMinutes );
   	}
   
   	// build string for target types
   	WCHAR uszTargetType[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszTargetType[ 0 ] = 0;
   	BOOL bFirstExterminate = TRUE;
   	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
   	{
   		const TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
   		if (pExterminate->speciesTarget != SPECIES_NONE)
   		{
   			// get target info
   			GENUS eGenus = (GENUS)GET_SPECIES_GENUS( pExterminate->speciesTarget );
   			int nClass = GET_SPECIES_CLASS( pExterminate->speciesTarget );
   			const UNIT_DATA* pUnitData = UnitGetData( pGame, eGenus, nClass );
   			const WCHAR* puszName = StringTableGetStringByIndex( pUnitData->nString );
   				
   			if (bFirstExterminate == TRUE)
   			{
   				PStrCopy( uszTargetType, puszName, MAX_TASK_ELEMENT_STRING_LENGTH );
				if( AppIsTugboat() )
				{
					PStrCat( uszTargetType, L"s", MAX_TASK_ELEMENT_STRING_LENGTH );
				}
   			}
   			else if( AppIsHellgate() )
   			{
				
   				PStrCat( uszTargetType, L" + ", MAX_TASK_ELEMENT_STRING_LENGTH );
   				PStrCat( uszTargetType, puszName, MAX_TASK_ELEMENT_STRING_LENGTH );
   			}
			else //tugboat
			{
				PStrCat( uszTargetType, L" and ", MAX_TASK_ELEMENT_STRING_LENGTH );
				PStrCat( uszTargetType, puszName, MAX_TASK_ELEMENT_STRING_LENGTH );
				PStrCat( uszTargetType, L"s", MAX_TASK_ELEMENT_STRING_LENGTH );
			}
   			bFirstExterminate = FALSE;
   		
   		}
   		
   	}
   
   	// build string for target number
   	WCHAR uszTargetNumber[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszTargetNumber[ 0 ] = 0;
   	int nTargetNumber = 0;
   	for (int i = 0; i < MAX_TASK_EXTERMINATE; ++i)
   	{
   		const TASK_EXTERMINATE* pExterminate = &pTaskTemplate->tExterminate[ i ];
   		if (pExterminate->speciesTarget != SPECIES_NONE)
   		{
   			nTargetNumber += pExterminate->nCount;		
   		}		
   	}
   	PStrPrintf( uszTargetNumber, MAX_TASK_ELEMENT_STRING_LENGTH, L"%d", nTargetNumber );
   
   	// build string for trigger percent
   	WCHAR uszTriggerPercent[ MAX_TASK_ELEMENT_STRING_LENGTH ];
   	uszTriggerPercent[ 0 ] = 0;	
	if( AppIsHellgate() )
	{
   		PStrPrintf( uszTriggerPercent, MAX_TASK_ELEMENT_STRING_LENGTH, L"%d", pTaskTemplate->nTriggerPercent );
	}
   
   	// build other strings here
   	// TODO: --->
   		
   	// get description string
   	const WCHAR *puszDescFormat = c_DialogTranslate( pTaskDefinition->nDescriptionDialog );
   	PStrCopy( puszDescBuffer, puszDescFormat, nDescBufferSize );
   
   	// build replacement table
   	TASK_DESCRIPTION_REPLACEMENT replacementTable[] = 
   	{
   
   		// token			replace with		color of replacement
   		{ StringReplacementTokensGet( SR_TARGETS ),			uszTargets,			FONTCOLOR_TASK_FOCUS },
   		{ StringReplacementTokensGet( SR_LEVEL ),			uszLevel,			FONTCOLOR_TASK_FOCUS },
   		{ StringReplacementTokensGet( SR_COLLECT ),			uszCollect,			FONTCOLOR_TASK_FOCUS },
   		{ StringReplacementTokensGet( SR_TIME_LIMIT ),		uszTimeLimit,		FONTCOLOR_TASK_FOCUS },
   		{ StringReplacementTokensGet( SR_TARGET_TYPE ),		uszTargetType,		FONTCOLOR_TASK_FOCUS },		
   		{ StringReplacementTokensGet( SR_TARGET_NUMBER ),	uszTargetNumber,	FONTCOLOR_TASK_FOCUS },		
   		{ StringReplacementTokensGet( SR_PERCENT ),			uszTriggerPercent,	FONTCOLOR_TASK_FOCUS },		
   		{ StringReplacementTokensGet( SR_AREA ),			uszArea,			FONTCOLOR_TASK_FOCUS },
   		{ NULL,				NULL,				FONTCOLOR_INVALID }			// keep this last
   		
   	};

   
   	// replace any tokens found with pieces we built (if we have them)
   	FONTCOLOR eRegularColor = FONTCOLOR_WHITE;
   	REF(eRegularColor);
   	for (int i = 0; replacementTable[ i ].puszToken != NULL; ++i)
   	{
   		const TASK_DESCRIPTION_REPLACEMENT *pReplacement = &replacementTable[ i ];
   		if (pReplacement->puszReplacement != NULL)
   		{
   		
			// must have actual characters to replace
			if (PStrLen( pReplacement->puszReplacement ) > 0)
			{
				if (pReplacement->eColor != FONTCOLOR_INVALID)
				{
					WCHAR uszReplacement[MAX_TASK_ELEMENT_STRING_LENGTH] = L"\0";
					UIAddColorToString( uszReplacement, pReplacement->eColor, arrsize(uszReplacement));
					PStrCat(uszReplacement, pReplacement->puszReplacement, arrsize(uszReplacement));
					UIAddColorReturnToString(uszReplacement, arrsize(uszReplacement));

					PStrReplaceToken( puszDescBuffer, nDescBufferSize, pReplacement->puszToken, uszReplacement);
				}
				else
				{
					// do replacement	
					PStrReplaceToken( puszDescBuffer, nDescBufferSize, pReplacement->puszToken, pReplacement->puszReplacement);
				}
			}
   		
   		}
   		
   	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetTaskDescription(
	GAME *pGame,
	const TASK_TEMPLATE *pTaskTemplate,
	UI_COMPONENT_ENUM eTaskDescLabel)
{
	WCHAR uszDesc[ 2048 ];	
	sTaskGetDescString( pTaskTemplate, uszDesc, 2048 );
	// set into ui elemnet	
	UILabelSetTextByEnum( eTaskDescLabel, uszDesc );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetTaskDescription(
	GAME *pGame,
	const QUEST_TASK_DEFINITION_TUGBOAT *pTaskTemplate,
	UI_COMPONENT_ENUM eTaskDescLabel)
{
	WCHAR uszDesc[ 2048 ];	
	// get description string
	c_QuestFillFlavoredText( pTaskTemplate, INVALID_ID, uszDesc, 2048, QUEST_FLAG_FILLTEXT_QUEST_LOG );	
	// set into ui elemnet	
	UILabelSetTextByEnum( eTaskDescLabel, uszDesc );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateTaskUICommon(
	const TASK_TEMPLATE *pTaskTemplate,
	UI_COMPONENT_ENUM eTaskNameLabel,
	UI_COMPONENT_ENUM eTaskDescLabel,
	UI_COMPONENT_ENUM eTaskCompleteLabel,
	UI_COMPONENT_ENUM eTaskIncompleteLabel,
	UI_COMPONENT_ENUM eTaskRewardLabel,
	UI_COMPONENT_ENUM eTaskRewardGold,
	UI_COMPONENT_ENUM eTaskRewardXP )
{

	// if no template, just clear all elements
	if (pTaskTemplate == NULL)
	{
		if( eTaskNameLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskNameLabel, L"" );
		}
		if( eTaskDescLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskDescLabel, L"" );
		}
		if( eTaskRewardLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardLabel, L"" );
		}
		if( eTaskRewardGold != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardGold, L"" );
		}
		if( eTaskRewardXP != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardXP, L"" );
		}
		UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, FALSE );
	}
	else
	{
	
		// get task definition
		GAME *pGame = AppGetCltGame();	
		
		
		const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pTaskTemplate->wCode );
		ASSERT_RETURN(pTaskDefinition);
		
		// set name
		const WCHAR *puszName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
		if( eTaskNameLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskNameLabel, puszName );
		}
				
		// set description
		if( eTaskDescLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskTemplate,eTaskDescLabel );
		}

		// set description
		if( eTaskCompleteLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskTemplate, eTaskCompleteLabel );
		}

		// set description
		if( eTaskIncompleteLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskTemplate, eTaskIncompleteLabel );
		}

		
		// set reward(s)
		#define MAX_REWARD_DESCRIPTION ((MAX_ITEM_NAME_STRING_LENGTH + 2) * MAX_AVAILABLE_TASKS_AT_GIVER)
		WCHAR uszRewardDesc[ MAX_REWARD_DESCRIPTION ];
		uszRewardDesc[ 0 ] = 0;		
		for (int i = 0; i < MAX_TASK_REWARDS; ++i)
		{
			UNITID idReward = pTaskTemplate->idRewards[ i ];
			if (idReward != INVALID_ID)
			{
				UNIT *pReward = UnitGetById( pGame, idReward );
				ASSERTX( pReward, "Unable to get reward unit for task on client" );
		
				// get item description
				WCHAR uszItemDesc[ MAX_ITEM_NAME_STRING_LENGTH ];
				UnitGetName( pReward, uszItemDesc, MAX_ITEM_NAME_STRING_LENGTH, 0 );
				//UnitGetLongDescription(	pReward, uszItemDesc, MAX_ITEM_NAME_STRING_LENGTH );

				// append to reward descriptions
				PStrCat( uszRewardDesc, uszItemDesc, MAX_REWARD_DESCRIPTION );
				PStrCat( uszRewardDesc, L"\n", MAX_REWARD_DESCRIPTION );
								
			}
		}
		if( eTaskRewardLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardLabel, uszRewardDesc );
		}

		WCHAR uszAmount[ MAX_REMAINING_STRING ];									
		PStrPrintf( uszAmount, 
					MAX_REMAINING_STRING, 
					GlobalStringGet( GS_GOLD_REWARD ), 
					pTaskTemplate->nGoldReward );
		UILabelSetTextByEnum( eTaskRewardGold, uszAmount );

		PStrPrintf( uszAmount, 
					MAX_REMAINING_STRING, 
					GlobalStringGet( GS_XP_REWARD ), 
					pTaskTemplate->nExperienceReward );
		UILabelSetTextByEnum( eTaskRewardXP, uszAmount );

		
	}

	// update reward ui components
	c_UpdateActiveTaskButtons();
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateTaskUICommon(
	const QUEST_TASK_DEFINITION_TUGBOAT *pTaskDefinition,
	UI_COMPONENT_ENUM eTaskNameLabel,
	UI_COMPONENT_ENUM eTaskDescLabel,
	UI_COMPONENT_ENUM eTaskCompleteLabel,
	UI_COMPONENT_ENUM eTaskIncompleteLabel,
	UI_COMPONENT_ENUM eTaskRewardLabel,
	UI_COMPONENT_ENUM eTaskRewardGold,
	UI_COMPONENT_ENUM eTaskRewardXP )
{

	// if no template, just clear all elements
	if (pTaskDefinition == NULL)
	{
		if( eTaskNameLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskNameLabel, L"" );
		}
		if( eTaskDescLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskDescLabel, L"" );
		}
		/*if( eTaskRewardLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardLabel, L"" );
		}
		UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, FALSE );*/
	}
	else
	{
	
		// get task definition
		GAME *pGame = AppGetCltGame();	
		
		
		// set name
		int nTaskNameId = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( UIGetControlUnit(), pTaskDefinition, KQUEST_STRING_TITLE );
		const WCHAR *puszName = StringTableGetStringByIndex( nTaskNameId );
		if( eTaskNameLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskNameLabel, puszName );
			c_QuestFillRandomQuestTagsUIDialogText( UIComponentGetByEnum( eTaskNameLabel ), pTaskDefinition );
		}
				
		// set description
		if( eTaskDescLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskDefinition,eTaskDescLabel );
		}

		// set description
		if( eTaskCompleteLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskDefinition, eTaskCompleteLabel );
		}

		// set description
		if( eTaskIncompleteLabel != UICOMP_INVALID )
		{
			sSetTaskDescription( pGame, pTaskDefinition, eTaskIncompleteLabel );
		}
/*
		// set reward(s)
		#define MAX_REWARD_DESCRIPTION ((MAX_ITEM_NAME_STRING_LENGTH + 2) * MAX_AVAILABLE_TASKS_AT_GIVER)
		WCHAR uszRewardDesc[ MAX_REWARD_DESCRIPTION ];
		uszRewardDesc[ 0 ] = 0;		
		
		if( eTaskRewardLabel != UICOMP_INVALID )
		{
			UILabelSetTextByEnum( eTaskRewardLabel, uszRewardDesc );
		}

		WCHAR uszAmount[ MAX_REMAINING_STRING ];									
		PStrPrintf( uszAmount, 
					MAX_REMAINING_STRING, 
					GlobalStringGet( GS_GOLD_REWARD ), 
					pTaskDefinition->nGold );
		UILabelSetTextByEnum( eTaskRewardGold, uszAmount );

		PStrPrintf( uszAmount, 
					MAX_REMAINING_STRING, 
					GlobalStringGet( GS_XP_REWARD ), 
					pTaskDefinition->nExperience );
		UILabelSetTextByEnum( eTaskRewardXP, uszAmount );
		*/

		
	}
	UI_COMPONENT* pRewardPane = UIComponentGetByEnum(UICOMP_TASK_REWARD_FOOTER);
	c_QuestRewardUpdateUI( pRewardPane,
		FALSE,
		TRUE,
		pTaskDefinition ? pTaskDefinition->nTaskIndexIntoTable : -1 );

	// update reward ui components
	c_UpdateActiveTaskButtons();
	
	UIComponentHandleUIMessage( UIComponentGetByEnum( UICOMP_PANEMENU_QUESTS ), UIMSG_PAINT, 0, 0, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskGetRewardString(
	const TASK_TEMPLATE *pTaskTemplate,
	WCHAR *puszRewardBuffer,
	int nRewardBufferSize,
	DWORD dwFlags)
{
	GAME *pGame = AppGetCltGame();
	
	// clear text
	puszRewardBuffer[ 0 ] = 0;		
	
	// go through rewards
	for (int i = 0; i < MAX_TASK_REWARDS; ++i)
	{	
		UNITID idReward = pTaskTemplate->idRewards[ i ];
		if (idReward != INVALID_ID)
		{
			UNIT *pReward = UnitGetById( pGame, idReward );
			ASSERTX( pReward, "Unable to get reward unit for task on client" );

			WCHAR uszItemTextDesc[ MAX_ITEM_NAME_STRING_LENGTH ];
			if (TESTBIT( dwFlags, TDP_REWARD_IMAGE_BIT ) && AppIsHellgate())
			{			
				ItemGetEmbeddedTextModelString( pReward, uszItemTextDesc, arrsize(uszItemTextDesc), TRUE );
			}
			else
			{
				DWORD dwNameFlags = 0;			
				SETBIT( dwNameFlags, UNF_EMBED_COLOR_BIT );									
				UnitGetName( pReward, uszItemTextDesc, arrsize(uszItemTextDesc), dwNameFlags );
			}
											
			// add a '+' between rewards for text versions
			if (i != 0 && TESTBIT( dwFlags, TDP_REWARD_IMAGE_BIT ) == FALSE)
			{
				PStrCat( puszRewardBuffer, L" + ", nRewardBufferSize );			
			}
			
			// append to reward descriptions
			PStrCat( puszRewardBuffer, uszItemTextDesc, nRewardBufferSize );

		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TASK_STATUS_LABEL *sFindTaskStatusLabel( 
	TASK_STATUS eStatus)
{
	for (int i = 0; sgTaskStatusLabelTable[ i ].eStatus != TASK_STATUS_INVALID; ++i)
	{
		TASK_STATUS_LABEL *pTaskStatusLabel = &sgTaskStatusLabelTable[ i ];
		if (pTaskStatusLabel->eStatus == eStatus)
		{
			return pTaskStatusLabel;
		}
	}
	return NULL;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskGetTimerStatusString(
	GAME* pGame,
	const TASK_TEMPLATE *pTaskTemplate,
	TASK_STATUS eStatus,
	DWORD dwAcceptedTick,
	WCHAR *puszStatusBuffer, 
	int nStatusBufferSize)
{					
	DWORD dwTickCurrent = GameGetTick( pGame );
	DWORD dwTicksElapsed = dwTickCurrent - dwAcceptedTick;
	DWORD dwSecondsElapsed = dwTicksElapsed / GAME_TICKS_PER_SECOND;
	DWORD dwTimeRemainingInSeconds = (pTaskTemplate->nTimeLimitInMinutes * SECONDS_PER_MINUTE) - dwSecondsElapsed;
	
	int nSeconds = dwTimeRemainingInSeconds;
	int nHours = nSeconds / SECONDS_PER_MINUTE / MINUTES_PER_HOUR;
	nSeconds -= nHours * MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
	int nMinutes = nSeconds / SECONDS_PER_MINUTE;
	nSeconds -= nMinutes * SECONDS_PER_MINUTE;
							
	// set color based on task status
	FONTCOLOR eTimeColor = FONTCOLOR_INVALID;
	if (eStatus == TASK_STATUS_COMPLETE || eStatus == TASK_STATUS_REWARD)
	{
		eTimeColor = FONTCOLOR_TASK_TIME_COMPLETE;
	}
	else
	{
		eTimeColor = FONTCOLOR_TASK_TIME_ACTIVE;
	}

	// get the grouping separator
	const WCHAR *puszSeparator = GlobalStringGet( GS_TIME_GROUPING_SEPARATOR );
	
	// format time string (we should localize this time format, but I can't remember
	// what the win32 function to get the local time format is

	WCHAR uszTime[256];
	if (nHours > 0)
	{
		PStrPrintf( 
			uszTime, 
			arrsize(uszTime), 
			L" %02d%s%02d%s%02d", 
			nHours, 
			puszSeparator,
			nMinutes, 
			puszSeparator,
			nSeconds );		
	}
	else
	{
		PStrPrintf( 
			uszTime, 
			arrsize(uszTime), 
			L" %02d%s%02d", 
			nMinutes, 
			puszSeparator,
			nSeconds );					
	}

	puszStatusBuffer[0] = L'\0';
	UIColorCatString(puszStatusBuffer, nStatusBufferSize, eTimeColor, uszTime);
}

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static void sUpdateTimerUI(
	GAME* pGame,
	CTASK* pCTask)
{
	// get status string
	WCHAR uszStatus[ MAX_QUEST_STATUS_STRING_LENGTH ];
	sTaskGetTimerStatusString( pGame, 
							   &pCTask->tTaskTemplate,
							   pCTask->eStatus,
							   pCTask->dwAcceptedTick,
							   uszStatus,
							   MAX_QUEST_STATUS_STRING_LENGTH );
	// update the timer in the status field
	UILabelSetTextByEnum( UICOMP_TASK_STATUS, uszStatus );							   
}
//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static void sUpdateTaskUI(
	TASK_UI eTaskUI,
	const TASK_TEMPLATE *pTaskTemplate,
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTemplate,
	TASK_STATUS eStatus,
	CTASK* pCTask)
{
	//CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	GAME* pGame = AppGetCltGame();
	UNIT* pPlayer = GameGetControlUnit( pGame );
	switch (eTaskUI)
	{
		//----------------------------------------------------------------------------	
		case TASK_UI_TASK_SCREEN:
		{
		
			sUpdateTaskUICommon( 
				pQuestTemplate, 
				UICOMP_TASKSCREEN_NAME_LABEL,	
				UICOMP_TASKSCREEN_DESC_LABEL,	
				UICOMP_INVALID,	
				UICOMP_INVALID,	
				UICOMP_TASKSCREEN_REWARD_LABEL,
				UICOMP_TASKSCREEN_GOLDREWARD,
				UICOMP_TASKSCREEN_EXPERIENCEREWARD);

			//updates the task Text UI		
			if( pQuestTemplate )
			{
				sSetTaskDescription( pGame, pQuestTemplate, UICOMP_TASKSCREEN_DESC_LABEL );
			}


			UI_COMPONENT* panel =UIComponentGetByEnum(UICOMP_QUESTBUTTONS);
			UISetQuestButton( UICastToPanel(panel)->m_nCurrentTab );
			int nIndex;			
			nIndex = 0;
			UI_COMPONENT *pChild = panel->m_pFirstChild;
			//do the quest system
			while ( pChild &&
					nIndex < MAX_ACTIVE_QUESTS_PER_UNIT )
			{
				int nQuestId = MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, nIndex );//QuestGetQuestIDByIndex( pGame, pPlayer, nIndex );
				const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = QuestGetActiveTask( pPlayer, nQuestId );
	
				if( activeTask )
				{
					WCHAR szTemp[128];
					ZeroMemory( szTemp, sizeof( WCHAR ) * 128 );
					UI_COMPONENT *pLabel = UIComponentFindChildByName( pChild, "questname");
					
					//PStrCopy(szTemp, StringTableGetStringByIndex( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, activeTask, KQUEST_STRING_TITLE ) ), 128);
					FONTCOLOR eFontColor = c_QuestGetDifficultyTextColor( pPlayer, activeTask );
					UIColorCatString(szTemp, 128, eFontColor, StringTableGetStringByIndex( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, activeTask, KQUEST_STRING_TITLE ) ));
					c_QuestFillFlavoredText( activeTask, INVALID_ID, szTemp, 128, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES | QUEST_FLAG_FILLTEXT_NO_COLOR );					
					if( activeTask->pNext == NULL && //only the last task can be complete!
						QuestTaskHasTasksToComplete( pPlayer, activeTask ) &&
						QuestAllTasksCompleteForQuestTask( pPlayer, activeTask ) )
					{
						UIColorCatString(szTemp, arrsize(szTemp), FONTCOLOR_FAWKES_GOLD, L" - Complete");
					}

					UILabelSetText( pLabel, szTemp );
					UIComponentSetActive(pChild, TRUE);
					UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
				}
				else
				{
					UIComponentSetActive(pChild, FALSE);
					UIComponentSetVisible(pChild, FALSE );
					UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
				}
				if( activeTask )
				{
					pChild = pChild->m_pNextSibling;
				}
				nIndex++;
				
			}
			// hide any remaining tasks
			while ( pChild)
			{
				UIComponentSetActive(pChild, FALSE);
				UIComponentSetVisible(pChild, FALSE );;
				UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
				pChild = pChild->m_pNextSibling;
			}
			const QUEST_DEFINITION_TUGBOAT *pQuest( NULL );
			if( pQuestTemplate )
			{
				pQuest = QuestDefinitionGet( pQuestTemplate->nParentQuest );
			}
			if( QuestGetNumOfActiveQuests( pGame, pPlayer ) > 0 &&
				( pQuest && pQuest->bQuestCannnotBeAbandoned == FALSE ) )
			{
				UI_COMPONENT* pButton = UIComponentGetByEnum(UICOMP_TASK_BUTTON_ABANDON);
				UIComponentSetActive(pButton, TRUE);
				UIComponentSetVisible(pButton, TRUE );
				//pButton = UIComponentGetByEnum(UICOMP_TASK_REWARD_LABEL_HEADER);
				//UIComponentSetActive(pButton, TRUE);
				//UIComponentSetVisible(pButton, TRUE );
			}
			else
			{
				UI_COMPONENT* pButton  =UIComponentGetByEnum(UICOMP_TASK_BUTTON_ABANDON);
				UIComponentSetActive(pButton, FALSE);
				UIComponentSetVisible(pButton, FALSE );
				//pButton = UIComponentGetByEnum(UICOMP_TASK_REWARD_LABEL_HEADER);
				//UIComponentSetActive(pButton, FALSE);
				//UIComponentSetVisible(pButton, FALSE );
			}

			BOOL bDisplayStatusString = TRUE;
			if (pCTask && pTaskTemplate != NULL)
			{				
				int nExterminateGoal = TaskTemplateGetExterminateGoal( pTaskTemplate );
				
				// time limited tasks display a timer instead of the status string			
				if ( pTaskTemplate->nTimeLimitInMinutes > 0 &&
					(eStatus == TASK_STATUS_ACTIVE || eStatus == TASK_STATUS_COMPLETE))
				{
					// update task timer when we have an actual instance (if necessary)
					sUpdateTimerUI( pGame, pCTask );
					bDisplayStatusString = FALSE;	
					UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, TRUE );

				}

				// extermination tasks display the number left to exterminate instead 
				// of status string when the task is active
				else if(eStatus == TASK_STATUS_ACTIVE && nExterminateGoal > 0)
				{
					int nExterminateRemaining = nExterminateGoal - pTaskTemplate->nExterminatedCount;
					if (nExterminateRemaining < 0)
					{
						nExterminateRemaining = 0;
					}
					WCHAR uszRemaining[ MAX_REMAINING_STRING ];									
					PStrPrintf( 
						uszRemaining, 
						MAX_REMAINING_STRING, 
						GlobalStringGet( GS_NUMBER_REMAINING ), 
						nExterminateRemaining );
					UILabelSetTextByEnum( UICOMP_TASK_STATUS, uszRemaining );
					bDisplayStatusString = FALSE;
					UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, TRUE );

				}
				else
				{
					UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, FALSE );

				}
				
			}
			else
			{
				UIComponentSetVisibleByEnum( UICOMP_TASK_STATUS, FALSE );
			}

					
			break;
		}
		//----------------------------------------------------------------------------	
		case TASK_UI_ACTIVE_TASK:
		{
		
			// update the common stuff

			sUpdateTaskUICommon( 
					pQuestTemplate, 
					UICOMP_TASKSCREEN_NAME_LABEL,	
					UICOMP_TASKSCREEN_DESC_LABEL,	
					UICOMP_INVALID,	
					UICOMP_INVALID,	
					UICOMP_TASKSCREEN_REWARD_LABEL,
					UICOMP_TASKSCREEN_GOLDREWARD,
					UICOMP_TASKSCREEN_EXPERIENCEREWARD);
			
			BOOL bDisplayStatusString = TRUE;
			if (pCTask && pTaskTemplate != NULL)
			{				
				int nExterminateGoal = TaskTemplateGetExterminateGoal( pTaskTemplate );
				
				// time limited tasks display a timer instead of the status string			
				if ( pTaskTemplate->nTimeLimitInMinutes > 0 &&
					(eStatus == TASK_STATUS_ACTIVE || eStatus == TASK_STATUS_COMPLETE))
				{
					// update task timer when we have an actual instance (if necessary)
					sUpdateTimerUI( pGame, pCTask );
					bDisplayStatusString = FALSE;					
				}

				// extermination tasks display the number left to exterminate instead 
				// of status string when the task is active
				else if(eStatus == TASK_STATUS_ACTIVE && nExterminateGoal > 0)
				{
					int nExterminateRemaining = nExterminateGoal - pTaskTemplate->nExterminatedCount;
					if (nExterminateRemaining < 0)
					{
						nExterminateRemaining = 0;
					}
					WCHAR uszRemaining[ MAX_REMAINING_STRING ];									
					PStrPrintf( 
						uszRemaining, 
						MAX_REMAINING_STRING, 
						GlobalStringGet( GS_NUMBER_REMAINING ), 
						nExterminateRemaining );
					UILabelSetTextByEnum( UICOMP_TASK_STATUS, uszRemaining );
					bDisplayStatusString = FALSE;
				}
				
			}
						
			// set default status description
			/*if (bDisplayStatusString)
			{
				const WCHAR *puszStatus = L"";
				TASK_STATUS_LABEL *pTaskStatusLabel = sFindTaskStatusLabel( eStatus );
				if (pTaskStatusLabel && pTaskStatusLabel->pszDialog != NULL)
				{
					puszStatus = StringTableGetStringByKey( pTaskStatusLabel->pszDialog );
				}
				UILabelSetTextByEnum( UICOMP_TASK_STATUS, puszStatus );
			}*/
					
			break;
		}
		
		//----------------------------------------------------------------------------
		case TASK_UI_NEW_TASK:
		{
		

				sUpdateTaskUICommon( 
					pQuestTemplate, 
					UICOMP_TASKSCREEN_NAME_LABEL,	
					UICOMP_TASKSCREEN_DESC_LABEL,	
					UICOMP_INVALID,	
					UICOMP_INVALID,	
					UICOMP_TASKSCREEN_REWARD_LABEL,
					UICOMP_TASKSCREEN_GOLDREWARD,
					UICOMP_TASKSCREEN_EXPERIENCEREWARD);

		
			break;
			
		}
		
	}
				
}

//----------------------------------------------------------------------------
//tugboat
//----------------------------------------------------------------------------
static int sCTasksGetTaskIndex(
	CTASK* pCTask)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	for( int i = 0; i < pCTaskGlobals->nNumActiveTasks; i++ )
	{
		if( &pCTaskGlobals->activeTasks[ i ] == pCTask )
		{
			return i;
		}
	}
	return 0;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskGetStatusString( 
	const TASK_TEMPLATE *pTaskTemplate,
	TASK_STATUS eStatus, 
	DWORD dwAcceptedTick,
	WCHAR *puszStatusBuffer, 
	int nStatusBufferSize)
{	
	GAME *pGame = AppGetCltGame();
	
	BOOL bUseDefaultLabelStatus = TRUE;
	if (pTaskTemplate != NULL)
	{				
		int nExterminateGoal = TaskTemplateGetExterminateGoal( pTaskTemplate );
		
		// time limited tasks display a timer instead of the status string			
		if ( pTaskTemplate->nTimeLimitInMinutes > 0 &&
			(eStatus == TASK_STATUS_ACTIVE || eStatus == TASK_STATUS_COMPLETE))
		{
			// update task timer when we have an actual instance (if necessary)
			sTaskGetTimerStatusString( 
				pGame, 
				pTaskTemplate, 
				eStatus, 
				dwAcceptedTick,
				puszStatusBuffer, 
				nStatusBufferSize );
			bUseDefaultLabelStatus = FALSE;					
		}

		// extermination tasks display the number left to exterminate instead 
		// of status string when the task is active
		else if(eStatus == TASK_STATUS_ACTIVE && nExterminateGoal > 0)
		{
			int nExterminateRemaining = nExterminateGoal - pTaskTemplate->nExterminatedCount;
			if (nExterminateRemaining < 0)
			{
				nExterminateRemaining = 0;
			}
			PStrPrintf( 
				puszStatusBuffer, 
				nStatusBufferSize, 
				GlobalStringGet( GS_NUMBER_REMAINING ), 
				nExterminateRemaining );
			bUseDefaultLabelStatus = FALSE;
		}
		
	}
				
	// set default status description
	if (bUseDefaultLabelStatus)
	{
		const WCHAR *puszStatusText = L"";
		TASK_STATUS_LABEL *pTaskStatusLabel = sFindTaskStatusLabel( eStatus );
		if (pTaskStatusLabel && pTaskStatusLabel->pszStringKey != NULL)
		{
			puszStatusText = StringTableGetStringByKey( pTaskStatusLabel->pszStringKey );
		}
		PStrPrintf( puszStatusBuffer, nStatusBufferSize, puszStatusText);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sViewingActiveTask( CTASK *pCTask,
								const QUEST_TASK_DEFINITION_TUGBOAT *pQuestDefinition = NULL )
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	
	
	// set active task we're viewing
	TASK_STATUS eStatus = TASK_STATUS_INVALID;
	TASK_TEMPLATE *pTaskTemplate = NULL;
	GAME* pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	int ActiveQuests = QuestGetNumOfActiveQuests( pGame, pPlayer );
	if( pQuestDefinition != NULL )
	{		
		int nID = pQuestDefinition->nParentQuest;
		// save active task
		pCTaskGlobals->idActiveQuestCurrent = nID;
		pCTaskGlobals->idActiveTaskCurrent = GAMETASKID_INVALID;
		//UISetQuestButton( nIndex );
		eStatus = TASK_STATUS_ACTIVE;
		if( QuestAllTasksCompleteForQuestTask(pPlayer, pQuestDefinition->nParentQuest ) )
		{
			eStatus = TASK_STATUS_COMPLETE;
		}
	}
	else if (pCTask != NULL)
	{
	
		// save active task
		pCTaskGlobals->idActiveTaskCurrent = pCTask->tTaskTemplate.idTask;
		pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
		if( AppIsTugboat() )
		{
			UISetQuestButton( sCTasksGetTaskIndex( pCTask ) + ActiveQuests );
			//UITasksSelectQuest( UIComponentGetByEnum(UICOMP_QUESTBUTTONS), sCTasksGetTaskIndex( pCTask ) );

		}
		// use this task status and template
		eStatus = pCTask->eStatus;
		pTaskTemplate = &pCTask->tTaskTemplate;

	}
	else
	{
		pCTaskGlobals->idActiveTaskCurrent = GAMETASKID_INVALID;
		pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
	}
		
	// update ui
	if (AppIsHellgate())
	{
		UIUpdateQuestLog( QUEST_LOG_UI_VISIBLE );		
	}	
	else if( AppIsTugboat() )
	{	
	
		if( pQuestDefinition )
		{
			sUpdateTaskUI( TASK_UI_ACTIVE_TASK, NULL, pQuestDefinition, eStatus, NULL );
			sUpdateTaskUI( TASK_UI_TASK_SCREEN, NULL, pQuestDefinition, eStatus, NULL );
		}
		else
		{
			sUpdateTaskUI( TASK_UI_ACTIVE_TASK, pTaskTemplate, NULL, eStatus, pCTask );
			sUpdateTaskUI( TASK_UI_TASK_SCREEN, pTaskTemplate, NULL, eStatus, pCTask );
		}

		if( !pCTask && !pQuestDefinition )
		{
			UILabelSetTextByEnum( UICOMP_TASKSCREEN_DESC_LABEL, L"" );
			//UILabelSetTextByEnum( UICOMP_TASKSCREEN_REWARD_LABEL, L"" );
			//UILabelSetTextByEnum( UICOMP_TASKSCREEN_GOLDREWARD, L"" );	
			//UILabelSetTextByEnum( UICOMP_TASKSCREEN_EXPERIENCEREWARD, L"" );
			//UILabelSetTextByEnum( UICOMP_TASK_STATUS, L"" );		
			UI_COMPONENT* pButton  = UIComponentGetByEnum(UICOMP_TASK_BUTTON_ABANDON);
			UIComponentSetActive(pButton, FALSE);
			UIComponentSetVisible(pButton, FALSE );
			//pButton = UIComponentGetByEnum(UICOMP_TASK_REWARD_LABEL_HEADER);
			//UIComponentSetActive(pButton, FALSE);
			//UIComponentSetVisible(pButton, FALSE );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sViewingAvailableTask(
	const TASK_TEMPLATE *pTaskTemplate)
{		
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	

	// set available task we're viewing
	pCTaskGlobals->idAvailableTaskCurrent = pTaskTemplate->idTask;
	
	// update ui
	sUpdateTaskUI( TASK_UI_NEW_TASK, pTaskTemplate, NULL, TASK_STATUS_INVALID, NULL );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sUpdateNPCs(
	UNIT *pUnit,
	void *pCallbackData)
{

	if( UnitIsTaskGiver( pUnit ) ||
		( AppIsTugboat() ) )
	{
		SPECIES speciesGiver = UnitGetSpecies( pUnit );
		GAME* pGame = (GAME *)pCallbackData;
	
		if( UnitHasState( pGame, pUnit, STATE_NPC_INFO_NEW ) )
		{
			c_StateClear( pUnit, UnitGetId( pUnit ), STATE_NPC_INFO_NEW, 0 );
		}
		if( UnitHasState( pGame, pUnit, STATE_NPC_INFO_WAITING ) )
		{
			c_StateClear( pUnit, UnitGetId( pUnit ), STATE_NPC_INFO_WAITING, 0 );
		}
		if( UnitHasState( pGame, pUnit, STATE_NPC_INFO_RETURN ) )
		{
			c_StateClear( pUnit, UnitGetId( pUnit ), STATE_NPC_INFO_RETURN, 0 );
		}
		//call tugboat quest system
		if( AppIsTugboat() )
		{
			if( QuestUpdateQuestNPCs( UnitGetGame( pUnit ), 
									  GameGetControlUnit( UnitGetGame( pUnit ) ), 
									  pUnit ) ||
				!UnitIsTaskGiver( pUnit ) ) 
			{
				return RIU_CONTINUE;
			}			

			
		}
		//end tugboat quest system
		CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
		for (int i = 0; i < pCTaskGlobals->nNumActiveTasks; ++i)
		{
			CTASK *pTask = &pCTaskGlobals->activeTasks[ i ];

			// match params
			if ( pTask->tTaskTemplate.speciesGiver == speciesGiver )
			{
				switch( pTask->eStatus )
				{
				case TASK_STATUS_ACTIVE :
				case TASK_STATUS_GENERATED :
					{
						c_StateSet( pUnit, pUnit, STATE_NPC_INFO_WAITING, 0, 0, INVALID_ID );	
					}
					break;
				case TASK_STATUS_COMPLETE :
					{
						c_StateSet( pUnit, pUnit, STATE_NPC_INFO_RETURN, 0, 0, INVALID_ID );
					}
					break;
				default:
					if( pCTaskGlobals->nNumActiveTasks >= TaskMaxTasksPerPlayer() )
					{
					}
					else
					{
						c_StateSet( pUnit, pUnit, STATE_NPC_INFO_NEW, 0, 0, INVALID_ID );
					}
					break;
				}

				return RIU_CONTINUE;
			}

		}

		if( pCTaskGlobals->nNumActiveTasks >= TaskMaxTasksPerPlayer() )
		{
		}
		else
		{
			c_StateSet( pUnit, pUnit, STATE_NPC_INFO_NEW, 0, 0, INVALID_ID );
		}
	}

	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskUpdateNPCs(
	GAME *pGame,
	ROOM* pRoom )
{
	RoomIterateUnits( pRoom, NULL, sUpdateNPCs, pGame );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskUpdateNPCs( 
	GAME *pGame,
	LEVEL* pLevel )
{
	ROOM* pRoom = pLevel->m_pRooms;
	while( pRoom != NULL )
	{
		RoomIterateUnits( pRoom, NULL, sUpdateNPCs, pGame );
		pRoom = pRoom->pNextRoomInLevel;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TBRefreshTasks( int nTaskIndex )
{
	c_TasksSetActiveTaskByIndex( nTaskIndex );

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskAbandon(
	CTASK* pCTask)
{
	ASSERTX_RETFALSE( pCTask, "Expected client task" );
	TASK_TEMPLATE* pTaskTemplate = &pCTask->tTaskTemplate;
	
	// must be valid task id
	if (pTaskTemplate->idTask != GAMETASKID_INVALID)
	{
		CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
		
		// setup message and send to server
		MSG_CCMD_TASK_TRY_ABANDON message;
		message.nTaskID = pTaskTemplate->idTask;
		c_SendMessage( CCMD_TASK_TRY_ABANDON, &message );
		
		// remove from client
		if (pCTaskGlobals->nNumActiveTasks > 1)
		{
		
			// find where our active task resides
			int nToAbandonIndex = -1;
			for (int i = 0; i < pCTaskGlobals->nNumActiveTasks; ++i)
			{
				CTASK *pCTaskMatch = &pCTaskGlobals->activeTasks[ i ];
				if (pCTaskMatch->tTaskTemplate.idTask == pTaskTemplate->idTask)
				{
					nToAbandonIndex = i;
					break;
				}
			}
			
			// copy "last" task into index we're are abandoning
			ASSERT( nToAbandonIndex >= 0 );
			CTASK *pCTaskLast = &pCTaskGlobals->activeTasks[ pCTaskGlobals->nNumActiveTasks - 1 ];
			pCTaskGlobals->activeTasks[ nToAbandonIndex ] = *pCTaskLast;
			
			// clear the last index
			if( AppIsHellgate() )
			{
				sClientTaskInit( &pCTaskGlobals->activeTasks[ pCTaskGlobals->nNumActiveTasks - 1 ] );
			}
			
		}
		
		// one less task now
		pCTaskGlobals->nNumActiveTasks--;
		if( AppIsTugboat() )
		{
			CTASK* pCNextTask = NULL;
			if( pCTaskGlobals->nNumActiveTasks > 0 )
			{
				pCTaskGlobals->idActiveTaskCurrent = pCTaskGlobals->activeTasks[ 0 ].tTaskTemplate.idTask;
				pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
				pCNextTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
			}

			
		}
		// update the ui
		sViewingActiveTask( NULL );
		if( AppIsTugboat() )
		{		
			//UISetQuestButton( 0 );
			UITasksSelectQuest( 0 );

			GAME* pGame = AppGetCltGame();
			UNIT *pPlayer = GameGetControlUnit( pGame );
			c_TaskUpdateNPCs( pGame, UnitGetLevel( pPlayer ) );
		}
		return TRUE;  // abandon succesfull
		
	}

	return FALSE;  // abandon failed
	
}

//----------------------------------------------------------------------------
//confirm you want to drop it.
//----------------------------------------------------------------------------
static void sCallbackConfirmAbandon( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuest = (const QUEST_TASK_DEFINITION_TUGBOAT *)pUserData;
	ASSERTX_RETURN( pQuest, "Not quest passed in." );
	GAME* pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	// setup message and send to server
	MSG_CCMD_QUEST_TRY_ABANDON message;
	message.nQuestID = pQuest->nParentQuest;
	c_SendMessage( CCMD_QUEST_TRY_ABANDON, &message );
	
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
// update the ui
	sViewingActiveTask( NULL );
	if( AppIsTugboat() )
	{		
		//UISetQuestButton( 0 );
		UITasksSelectQuest( 0 );

		GAME* pGame = AppGetCltGame();		
		c_TaskUpdateNPCs( pGame, UnitGetLevel( pPlayer ) );
	}
	
	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sQuestAbandon( const QUEST_TASK_DEFINITION_TUGBOAT *pQuest )
{
	ASSERTX_RETFALSE( pQuest, "Expected client quest" );	
	int nID = pQuest->nParentQuest;
	if( nID == INVALID_ID )
	{
		return FALSE;
	}


	const QUEST_DEFINITION_TUGBOAT *pQuestDef = QuestDefinitionGet( pQuest->nParentQuest );
	if( !pQuestDef )
	{
		return FALSE;
	}
	if( pQuestDef->bQuestCannnotBeAbandoned )
	{
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_ALERT ),  GlobalStringGet( GS_QUEST_CANNOT_ABANDON ), FALSE );
		return FALSE;
	}
	if( AppIsTugboat() )
	{
		DIALOG_CALLBACK tCallbackOK;
		DialogCallbackInit( tCallbackOK );
		tCallbackOK.pfnCallback = sCallbackConfirmAbandon;
		//lame
		tCallbackOK.pCallbackData = (QUEST_TASK_DEFINITION_TUGBOAT*)ExcelGetData( NULL, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, pQuest->nTaskIndexIntoTable ) ;
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_WARNING ), StringTableGetStringByIndex( GlobalStringGetStringIndex( GS_QUEST_CONFIRM_ABANDON ) ), TRUE, &tCallbackOK );
	}else{
		CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
		pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
	// update the ui
		sViewingActiveTask( NULL );
	}
	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAbandonOldForNewOK(
	void *pUserData, 
	DWORD dwCallbackData)
{

	// abandon existing task
	if (c_TasksTryAbandon() == TRUE)
	{

		// accept the new one
		c_TasksTryAccept();

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTooManyTasks(
	void)
{		
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );

	if (AppIsHellgate())
	{
				
		// there are too many tasks, ask the user if they want to abandon their old one			
		NPC_DIALOG_SPEC tSpec;
		tSpec.idTalkingTo = UnitGetId( pPlayer );
		tSpec.nDialog = DIALOG_TASKS_ABANDON_OLD_FOR_NEW;
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sAbandonOldForNewOK;
		c_NPCTalkStart( pGame, &tSpec );

	}
	else
	{
		const WCHAR *puszTooManyTasks = GlobalStringGet( GS_TOO_MANY_TASKS );
		ConsoleString( CONSOLE_SYSTEM_COLOR, puszTooManyTasks );

		static int nErrorSound = INVALID_LINK;
		if (nErrorSound == INVALID_LINK)
		{
			nErrorSound = GlobalIndexGet( GI_SOUND_ERROR );
		}

		if (nErrorSound != INVALID_LINK)
		{
			UNIT *pPlayer = GameGetControlUnit( pGame );
			c_SoundPlay(nErrorSound, &c_UnitGetPosition(pPlayer), NULL);
		}	
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAcceptTask(
	const TASK_TEMPLATE *pTaskTemplate,
	BOOL bSendToServer)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	GAME* pGame = AppGetCltGame();
	
	// if player has too many tasks, automatically abandon failed tasks so we can accept a new one
	if (pCTaskGlobals->nNumActiveTasks >= TaskMaxTasksPerPlayer())
	{

		// go through all active tasks
		for (int i = 0; i < TaskMaxTasksPerPlayer(); ++i)
		{
			// find the first failed task and abandon it
			CTASK* pCTaskFailed = &pCTaskGlobals->activeTasks[ i ];
			if (pCTaskFailed->eStatus == TASK_STATUS_FAILED)
			{
				if (sTaskAbandon( pCTaskFailed ) == TRUE)
				{
					break;
				}
				
			}
			
		}
					
	}

	// if the player still has too many tasks then tell tell them
	if (pCTaskGlobals->nNumActiveTasks >= TaskMaxTasksPerPlayer())
	{
		sTooManyTasks();
		return FALSE;
	}
			
	// setup message and send to server
	if (bSendToServer == TRUE)
	{
		MSG_CCMD_TASK_TRY_ACCEPT message;
		message.nTaskID = pTaskTemplate->idTask;
		c_SendMessage( CCMD_TASK_TRY_ACCEPT, &message );
	}
			
	// get client task
	ASSERTX( pCTaskGlobals->nNumActiveTasks < TaskMaxTasksPerPlayer(), "Too many client tasks accepted" );
	CTASK *pCTask = &pCTaskGlobals->activeTasks[ pCTaskGlobals->nNumActiveTasks++ ];
	
	// init 
	sClientTaskInit( pCTask );
	pCTask->tTaskTemplate = *pTaskTemplate;
	pCTask->eStatus = TASK_STATUS_GENERATED;
	pCTask->dwAcceptedTick = GameGetTick( pGame );
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksRestoreTask( 
	const TASK_TEMPLATE* pTaskTemplate,
	TASK_STATUS eStatus)
{

	// accept this task
	sAcceptTask( pTaskTemplate, FALSE );

	// get task
	CTASK *pCTask = sCTaskFind( pTaskTemplate->idTask );
	ASSERTX_RETURN( pCTask, "Unable to find task after restoring it" );
	
	// restore status
	pCTask->eStatus = eStatus;
		
	// set this task as the one we will view the log
	sViewingActiveTask( pCTask );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTaskPrintTemplateInfo(
	const TASK_TEMPLATE *pTaskTemplate,
	TASK_STATUS eStatus,
	DWORD dwAcceptedTick,
	DWORD dwFlags,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERT(puszBuffer);
	
	// zero buffer
	puszBuffer[ 0 ] = 0;
	
	// title
	if (TESTBIT( dwFlags, TDP_TITLE_BIT ))
	{
	
		// get title
		WCHAR uszTitle[ MAX_QUEST_TITLE_LENGTH ];		
		sTaskGetTitleString( pTaskTemplate, uszTitle, MAX_QUEST_TITLE_LENGTH );
		
		//set color
		UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_TITLE, nBufferSize );
		
		// add to buffer
		PStrCat( puszBuffer, uszTitle, nBufferSize );
		
	}
		
	// description
	if (TESTBIT( dwFlags, TDP_DESC_BIT ))
	{
	
		// get desc
		WCHAR uszDesc[ MAX_QUEST_DESC_STRING_LENGTH ];
		sTaskGetDescString( pTaskTemplate, uszDesc, MAX_QUEST_DESC_STRING_LENGTH );

		// add newline
		if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE &&
			puszBuffer[0] != 0)
		{
			PStrCat( puszBuffer, L"\n", nBufferSize );
		}
		
		// add to buffer
		UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_DEFAULT, nBufferSize );
		PStrCat( puszBuffer, uszDesc, nBufferSize );

	}
			
	// reward(s)
	if (TESTBIT( dwFlags, TDP_REWARD_IMAGE_BIT ) ||
		TESTBIT( dwFlags, TDP_REWARD_TEXT_BIT ))
	{
	
		// do reward header if requested
		BOOL bAddedNewLine = FALSE;
		if (TESTBIT( dwFlags, TDP_REWARD_HEADER_BIT ))
		{
			if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE &&
				puszBuffer[0] != 0)
			{
				PStrCat( puszBuffer, L"\n\n", nBufferSize );
			}		
			bAddedNewLine = TRUE;
			
			UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_HEADER, nBufferSize );
			PStrCat( puszBuffer, GlobalStringGet( GS_REWARD_HEADER ), nBufferSize );
			PStrCat( puszBuffer, L"\n", nBufferSize );
			
			// add line that says how many they can pick from if necessary
			const TASK_DEFINITION *pTaskDef = TaskDefinitionGetByCode( pTaskTemplate->wCode );
			ASSERT_RETURN(pTaskDef);
			if (pTaskDef->nNumRewardTakes != REWARD_TAKE_ALL)
			{
				int nNumChoicesLeft = pTaskDef->nNumRewardTakes - pTaskTemplate->nNumRewardsTaken;
				if (nNumChoicesLeft == 1)
				{
					PStrCat( puszBuffer, GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_ONE ), nBufferSize );
				}
				else
				{
					const int MAX_MESSAGE = 128;
					WCHAR uszMessage[ MAX_MESSAGE ];
					PStrPrintf( 
						uszMessage, 
						MAX_MESSAGE, 
						GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_SOME ), 
						nNumChoicesLeft);
					PStrCat( puszBuffer, uszMessage, nBufferSize );
				}
				PStrCat( puszBuffer, L"\n", nBufferSize );
			}
			
		}
			
		// get reward string
		WCHAR uszReward[ MAX_REWARD_DESCRIPTION_LENGTH ];
		sTaskGetRewardString( pTaskTemplate, uszReward, MAX_REWARD_DESCRIPTION_LENGTH, dwFlags );
		
		// add new line if we didn't do it before
		if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE &&
			puszBuffer[0] != 0)
		{
			if (bAddedNewLine == FALSE)
			{
				PStrCat( puszBuffer, L"\n", nBufferSize );		
			}
		}
				
		// add to buffer
		UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_DEFAULT, nBufferSize );
		PStrCat( puszBuffer, uszReward, nBufferSize );

	}
	
	// status
	if (TESTBIT( dwFlags, TDP_STATUS_BIT ))
	{

		// do reward header if requested
		BOOL bAddedNewLine = FALSE;
		if (TESTBIT( dwFlags, TDP_REWARD_HEADER_BIT ))
		{
			if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE &&
				puszBuffer[0] != 0)
			{
				PStrCat( puszBuffer, L"\n", nBufferSize );
			}
			bAddedNewLine = TRUE;

			UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_HEADER, nBufferSize );
			PStrCat( puszBuffer, GlobalStringGet( GS_STATUS_HEADER ), nBufferSize );
			PStrCat( puszBuffer, L" ", nBufferSize );
		}
	
		// get status string
		WCHAR uszStatus[ MAX_QUEST_STATUS_STRING_LENGTH ];
		sTaskGetStatusString( pTaskTemplate, eStatus, dwAcceptedTick, uszStatus, MAX_QUEST_STATUS_STRING_LENGTH );

		// add new line if we didn't do it before
		if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE)
		{
			if (bAddedNewLine == FALSE &&
				puszBuffer[0] != 0)
			{
				PStrCat( puszBuffer, L"\n", nBufferSize );		
			}
		}
				
		// add to buffer
		UIAddColorToString( puszBuffer, FONTCOLOR_TASK_LOG_DEFAULT, nBufferSize );
		PStrCat( puszBuffer, uszStatus, nBufferSize );

	}
		
	if (TESTBIT( dwFlags, TDP_NO_NEWLINES_BIT ) == FALSE &&
		puszBuffer[0] != 0)
	{
		PStrCat( puszBuffer, L"\n", nBufferSize );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackTaskAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{
	c_TasksTryAccept();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackTaskReject(
	void *pUserData, 
	DWORD dwCallbackData)
{
	c_TasksTryReject();
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskAvailableTasks( 
	const MSG_SCMD_AVAILABLE_TASKS *pMessage)
{
	GAME *pGame = AppGetCltGame();
	UNITID idTaskGiver = pMessage->idTaskGiver;
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	ASSERT_RETURN(pCTaskGlobals);
	
	// clear out any previously available tasks
	pCTaskGlobals->nNumAvailableTasks = 0;
	
	// go through task templates and add to available tasks
	for (int i = 0; i < MAX_AVAILABLE_TASKS_AT_GIVER; ++i)
	{
		const TASK_TEMPLATE *pTaskTemplate = &pMessage->tTaskTemplate[ i ];
		pCTaskGlobals->availableTasks[ pCTaskGlobals->nNumAvailableTasks ] = *pTaskTemplate;
		pCTaskGlobals->nNumAvailableTasks++;
	}

	// sanity, the server should not be sending us a message with no tasks
	ASSERTX( pCTaskGlobals->nNumAvailableTasks > 0, "Server explicitly told us no tasks!" );
	{

		// set available task we're viewing
		const TASK_TEMPLATE *pTaskTemplate = &pCTaskGlobals->availableTasks[ 0 ];
		ASSERT_RETURN(pTaskTemplate);
		pCTaskGlobals->idAvailableTaskCurrent = pTaskTemplate->idTask;
	
		// set flags for desired parts
		DWORD dwFlags = 0;
	
		// get task description string
		WCHAR uszDescBuffer[ MAX_QUESTLOG_STRLEN ];
		SETBIT( dwFlags, TDP_DESC_BIT );
		sTaskPrintTemplateInfo( 
			pTaskTemplate, 
			TASK_STATUS_GENERATED, 
			0, 
			dwFlags, 
			uszDescBuffer, 
			MAX_QUESTLOG_STRLEN );
	
		// get reward string
		WCHAR uszRewardBuffer[ MAX_QUESTLOG_STRLEN ];
		dwFlags = 0;
		SETBIT( dwFlags, TDP_REWARD_IMAGE_BIT );		
		SETBIT( dwFlags, TDP_REWARD_HEADER_BIT );
		SETBIT( dwFlags, TDP_NO_NEWLINES_BIT );
		sTaskPrintTemplateInfo( 
			pTaskTemplate, 
			TASK_STATUS_GENERATED, 
			0, 
			dwFlags, 
			uszRewardBuffer, 
			MAX_QUESTLOG_STRLEN );
		
		const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pTaskTemplate->wCode );
		ASSERT_RETURN(pTaskDefinition);
	
		// setup dialog spec
		NPC_DIALOG_SPEC tSpec;
		tSpec.nDialog = pTaskDefinition->nDescriptionDialog;
		tSpec.idTalkingTo = idTaskGiver;
		tSpec.puszText = uszDescBuffer;
		tSpec.puszRewardText = uszRewardBuffer;
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sCallbackTaskAccept;
		tSpec.tCallbackCancel.pfnCallback = sCallbackTaskReject;
				
		// bring up dialog interface with a choice to accept or not
		c_NPCTalkStart( pGame, &tSpec );
	}
								
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTaskTimer(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA& tEventData)
{
	GAMETASKID idTaskServer = (GAMETASKID)tEventData.m_Data1;
	GAME* pGameTask = (GAME*)tEventData.m_Data2;
	ASSERTX_RETTRUE( pUnit == NULL, "Expected NULL unit" );
	
	// make sure this is for same game
	if (pGame == pGameTask)
	{
	
		// find task
		CTASK* pCTask = sCTaskFind( idTaskServer );
		if (pCTask)
		{

			// only continue if task is not failed
			if (pCTask->eStatus != TASK_STATUS_FAILED)
			{
			
				// is this task currently being viewed
				CLIENT_TASK_GLOBALS* pClientTaskGlobals = sClientTaskGlobalsGet();
				if (pClientTaskGlobals->idActiveTaskCurrent == idTaskServer)
				{
					if( AppIsHellgate() )
					{
						UIUpdateQuestLog( QUEST_LOG_UI_UPDATE );
					}
					else
					{
						sUpdateTimerUI( pGame, pCTask );
					}
				}
				
				// how frequently does this clock update
				DWORD dwUpdateDelayInTicks = GAME_TICKS_PER_SECOND * 1;
				
				// schedule another event to update the time in the future
				EVENT_DATA tEventData( (DWORD_PTR)idTaskServer, (DWORD_PTR)pGame );
				GameEventRegister( 
					pGame, 
					sTaskTimer, 
					NULL, 
					NULL, 
					&tEventData, 
					dwUpdateDelayInTicks);

			}
						
		}
	
	}
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCTaskCompleted( 
	CTASK *pCTask)
{
	const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pCTask->tTaskTemplate.wCode );
	const WCHAR *puszTaskName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
	if( AppIsHellgate() )
	{
		ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TASK_COMPLETED ), puszTaskName );
	}
	else
	{

		GAME *pGame = AppGetCltGame();
		GENUS eGenusGiver = (GENUS)GET_SPECIES_GENUS( pCTask->tTaskTemplate.speciesGiver );
		int nClassGiver = GET_SPECIES_CLASS( pCTask->tTaskTemplate.speciesGiver );
		const UNIT_DATA * giver_data = UnitGetData(pGame, eGenusGiver, nClassGiver);
		WCHAR szTemp[128];
		if (giver_data)
		{
			PStrCopy(szTemp, StringTableGetStringByIndex( giver_data->nString ), 128);
			PStrCat(szTemp, L"'s Quest Completed!", 128);
			UIShowTaskCompleteMessage( szTemp );
		}
		else
		{
			UIShowTaskCompleteMessage( puszTaskName );
		}
		UNIT *pPlayer = GameGetControlUnit( pGame );
		c_TaskUpdateNPCs( pGame, UnitGetLevel( pPlayer ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTasksIncomplete( 
	CTASK *pCTask)
{
	
	GAME *pGame = AppGetCltGame();
	//const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pCTask->tTaskTemplate.wCode );
//	const WCHAR *puszTaskName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
	//ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TASK_COMPLETED ), puszTaskName );
	UIShowGenericDialog( 
		GlobalStringGet(GS_INCOMPLETE_TASK_TITLE),
		GlobalStringGet(GS_INCOMPLETE_TASK),
		//L"Task Incomplete", 
		//L"Please come back and see me when you've completed the task", 
		FALSE, NULL, NULL );

	if (AppIsTugboat())
	{
		const TASK_TEMPLATE* pTaskTemplate = &pCTask->tTaskTemplate;			
		sSetTaskDescription( pGame, pTaskTemplate, UICOMP_DIALOGBOXTEXT ); //pTaskDefinition, pTaskDefinition->nIncompleteDialog,
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCTaskActivated( 
	CTASK *pCTask,
	TASK_STATUS eStatusOld)
{

	// only display a message when accepting a task (going from generated to active) ... we do
	// this because it's possible for some tasks to go from completed to active (like a collect
	// task where you have all the collect items and then get rid of one of them)
	if (eStatusOld == TASK_STATUS_GENERATED)
	{	
		GAME *pGame = AppGetCltGame();
		if( AppIsHellgate() )
		{
			const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pCTask->tTaskTemplate.wCode );
			const WCHAR *puszTaskName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
			ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TASK_ACCEPTED ), puszTaskName );
		}
		
		// if task has a time limit start a timer ticking down
		const TASK_TEMPLATE* pTaskTemplate = &pCTask->tTaskTemplate;
		if (pTaskTemplate->nTimeLimitInMinutes > 0)
		{
			GAMETASKID idTaskServer = pTaskTemplate->idTask;
			EVENT_DATA tEventData( (DWORD_PTR)idTaskServer, (DWORD_PTR)pGame );		
			sTaskTimer( pGame, NULL, tEventData );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCTaskFailed( 
	CTASK *pCTask)
{
	if( AppIsTugboat() )
	{
		return;
	}
	const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pCTask->tTaskTemplate.wCode );
	const WCHAR *puszTaskName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
	ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TASK_FAILED ), puszTaskName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksSetStatus( 
	const MSG_SCMD_TASK_STATUS *pMessage)
{
	CTASK *pCTask = sCTaskFind( pMessage->idTask );
	if (pCTask)
	{
	
		// set new stats
		TASK_STATUS eStatusOld = pCTask->eStatus;
		pCTask->eStatus = (TASK_STATUS)pMessage->eStatus;
		
		// when task becomes certain states we care
		switch (pCTask->eStatus)
		{

			//----------------------------------------------------------------------------
			case TASK_STATUS_ACTIVE:
			{
				sCTaskActivated( pCTask, eStatusOld );			
				break;
			}
			
			//----------------------------------------------------------------------------
			case TASK_STATUS_COMPLETE:
			{
				sCTaskCompleted( pCTask );			
				break;
			}
			
			//----------------------------------------------------------------------------
			case TASK_STATUS_FAILED:
			{
				sCTaskFailed( pCTask );
				break;
			}
			
			//----------------------------------------------------------------------------
			default:
			{	
				break;
			}
			
		}
		
		// update the ui to reflect the new status
		if( AppIsTugboat() )
		{
			sUpdateTaskUI( TASK_UI_ACTIVE_TASK, &pCTask->tTaskTemplate, NULL, pCTask->eStatus, pCTask );
			sUpdateTaskUI( TASK_UI_TASK_SCREEN, &pCTask->tTaskTemplate, NULL, pCTask->eStatus, pCTask );
		}
		else   //hellgate
		{
			UIUpdateQuestLog( QUEST_LOG_UI_VISIBLE );			
		}
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksIncomplete( 
	const MSG_SCMD_TASK_INCOMPLETE *pMessage)
{
	CTASK *pCTask = sCTaskFind( pMessage->idTask );
	if (pCTask)
	{
	
		
		// when task becomes certain states we care
		switch (pCTask->eStatus)
		{

			//----------------------------------------------------------------------------
			case TASK_STATUS_ACTIVE:
			{
				sTasksIncomplete( pCTask );			
				break;
			}
			//----------------------------------------------------------------------------
			default:
			{	
				break;
			}
			
		}
						
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksTryAccept(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();

	// get task we are thinking about accepting
	BOOL bAccepted = FALSE;
	GAMETASKID idTaskToAccept = pCTaskGlobals->idAvailableTaskCurrent;
	TASK_TEMPLATE *pTaskTemplateToAccept = sAvailableTaskFind( idTaskToAccept );
	if (pTaskTemplateToAccept)
	{
		bAccepted = sAcceptTask( pTaskTemplateToAccept, TRUE );
	}

	if (bAccepted == TRUE)
	{

		// remove all the available tasks just to be clean
		pCTaskGlobals->nNumAvailableTasks = 0;
	
		// set this task as the one we will view the log
		CTASK *pCTask = sCTaskFind( pTaskTemplateToAccept->idTask );
		ASSERTX( pCTask, "Unable to find CTask after accepting it" );
		sViewingActiveTask( pCTask );
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksTryReject(
	void)
{

	// to be clean, remove the available tasks
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	pCTaskGlobals->nNumAvailableTasks = 0;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TasksTryAbandon(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	
	// we currently only support one task so we'll assume it's the first one
	CTASK *pCTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
	if (pCTask)
	{
		return sTaskAbandon( pCTask );
	}

	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuest = QuestGetActiveTask( pPlayer, pCTaskGlobals->idActiveQuestCurrent );
	if( pQuest )
	{
		return sQuestAbandon( pQuest );
	}
	return TRUE;  // no task to abandon so it's "successful"
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksTryAcceptReward(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();

	// get task we are looking at
	GAMETASKID idTaskWithReward = pCTaskGlobals->idActiveTaskCurrent;
	CTASK *pCTask = sCTaskFind( idTaskWithReward );
	if (pCTask)
	{
	
		// setup message and send to server
		MSG_CCMD_TASK_TRY_ACCEPT_REWARD message;
		message.nTaskID = pCTask->tTaskTemplate.idTask;
		c_SendMessage( CCMD_TASK_TRY_ACCEPT_REWARD, &message );
		if( AppIsTugboat() )
		{
			static int nRewardSound = INVALID_LINK;
			GAME *pGame = AppGetCltGame();
			if (nRewardSound == INVALID_LINK)
			{
				nRewardSound = GlobalIndexGet( GI_SOUND_REWARD );
			}
	
			if (nRewardSound != INVALID_LINK)
			{
				UNIT *pPlayer = GameGetControlUnit( pGame );
				c_SoundPlay(nRewardSound, &c_UnitGetPosition(pPlayer), NULL);
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOfferRewardOK(
	void *pUserData, 
	DWORD dwCallbackData)
{
	c_TasksTryAcceptReward();
}
//----------------------------------------------------------------------------
//tugboat
//----------------------------------------------------------------------------
void c_TasksSetActiveTask(
	GAMETASKID idTaskServer)
{
	CTASK *pCTask = sCTaskFind( idTaskServer );
	sViewingActiveTask( pCTask );
		// set this as the active task we're viewing
		//sUpdateTaskUI( TASK_UI_ACTIVE_TASK, &pCTask->tTaskTemplate, pCTask->eStatus, pCTask );
		//sUpdateTaskUI( TASK_UI_TASK_SCREEN, &pCTask->tTaskTemplate, pCTask->eStatus, pCTask );
	
}	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksOfferReward(
	UNITID idTaskGiver,
	GAMETASKID idTaskServer)
{
	CTASK *pCTask = sCTaskFind( idTaskServer );
	if (pCTask)
	{
		GAME *pGame = AppGetCltGame();
		
		// set this as the active task we're viewing
		sViewingActiveTask( pCTask );
		
		// get text for task
		WCHAR uszDesc[ MAX_QUESTLOG_STRLEN ];				
		DWORD dwFlags = 0;
		SETBIT( dwFlags, TDP_DESC_BIT );
		SETBIT( dwFlags, TDP_REWARD_IMAGE_BIT );
		SETBIT( dwFlags, TDP_REWARD_HEADER_BIT );
		sTaskPrintTemplateInfo(
			&pCTask->tTaskTemplate,
			pCTask->eStatus,
			pCTask->dwAcceptedTick,
			dwFlags,
			uszDesc,
			MAX_QUESTLOG_STRLEN);
			
		// add instructions for accepting the reward
		PStrCat( uszDesc, L"\n\n", MAX_QUESTLOG_STRLEN );		
		PStrCat( uszDesc, GlobalStringGet( GS_ACCEPT_REWARD_INSTRUCTIONS ), MAX_QUESTLOG_STRLEN );
				
		// start the conversation
		NPC_DIALOG_SPEC tSpec;
		tSpec.idTalkingTo = idTaskGiver;
		tSpec.puszText = uszDesc;
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sOfferRewardOK;
		tSpec.tCallbackCancel.pfnCallback = NULL;
		UIDisplayNPCDialog( pGame, &tSpec );
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksSetActiveTaskByIndex(
	int nIndex)
{
	if( AppIsHellgate() )
	{
		return;
	}
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "No Game for task display refresh" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "No player for task display refresh" );



	int nOldIndex = nIndex;	
	int nQuestId( INVALID_ID );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		nQuestId = MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t );
		if( nQuestId != INVALID_ID )
		{
			nOldIndex--;			
		}
		if( nOldIndex < 0 )
			break;
	}		
	
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	if( nQuestId != INVALID_ID )
	{		
		const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = QuestGetActiveTask( pPlayer, nQuestId );

		sViewingActiveTask( NULL, activeTask );
	}
	else if( nIndex > 0 )
	{
		nIndex--;
		CTASK *pCTask = &pCTaskGlobals->activeTasks[ nIndex ];
		c_TasksSetActiveTask( pCTask->tTaskTemplate.idTask );
	}	
	else	
	{
		sViewingActiveTask( NULL );
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksCloseTask(
	GAMETASKID idTaskServer)
{
	CTASK *pCTask = sCTaskFind( idTaskServer );
	if (pCTask)
	{
		CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	
		// remove task from active tasks
		if (pCTaskGlobals && pCTaskGlobals->nNumActiveTasks > 0)
		{
			if( AppIsHellgate() )
			{
				// move last task into the front spot
				pCTaskGlobals->activeTasks[ 0 ] = pCTaskGlobals->activeTasks[ pCTaskGlobals->nNumActiveTasks - 1 ];
				pCTaskGlobals->nNumActiveTasks--;  // one less active task now
				// update ui		
				sViewingActiveTask( NULL );
				// update ui		
				sViewingActiveTask( NULL );

				const TASK_DEFINITION *pTaskDefinition = TaskDefinitionGetByCode( pCTask->tTaskTemplate.wCode );
				ASSERT_RETURN(pTaskDefinition);
				const WCHAR *puszTaskName = StringTableGetStringByIndex( pTaskDefinition->nNameStringKey );
				ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TASK_CLOSED ), puszTaskName );

			}
			else
			{
				int TaskIndex = 0;
				for( int i = 0; i < pCTaskGlobals->nNumActiveTasks; i++ )
				{
					if( &pCTaskGlobals->activeTasks[ i ] == pCTask )
					{
						TaskIndex = i;
					}
				}
				// remove task from active tasks
				if (pCTaskGlobals->nNumActiveTasks > 1)
				{
					// move last task into the vacant spot
					pCTaskGlobals->activeTasks[ TaskIndex ] = pCTaskGlobals->activeTasks[ pCTaskGlobals->nNumActiveTasks - 1 ];
				}

				pCTaskGlobals->nNumActiveTasks--;  // one less active task now

				CTASK* pCNextTask = NULL;
				if( pCTaskGlobals->nNumActiveTasks > 0 )
				{
					pCTaskGlobals->idActiveTaskCurrent = pCTaskGlobals->activeTasks[ 0 ].tTaskTemplate.idTask;
					pCTaskGlobals->idActiveQuestCurrent = GAMETASKID_INVALID;
					pCNextTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
				}
				
				// update ui
				sViewingActiveTask( pCNextTask );
				//UISetQuestButton( 0 );
				UITasksSelectQuest( 0 );
				GAME *pGame = AppGetCltGame();
				UNIT *pPlayer = GameGetControlUnit( pGame );
				c_TaskUpdateNPCs( pGame, UnitGetLevel( pPlayer ) );			

			}
		}
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksActiveTaskPDAOnActivate(
	void)
{	
	// update the take reward ui
	c_UpdateActiveTaskButtons();
}

//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
void c_TasksTalkingTo( 
	UNITID idTalkingTo)
{

	// update the take reward ui
	c_UpdateActiveTaskButtons();
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksUpdateExterminatedCount(
	MSG_SCMD_TASK_EXTERMINATED_COUNT* pMessage)
{
	GAMETASKID idTask = pMessage->idTask;
	int nCount = pMessage->nCount;
	
	// get client task
	CTASK* pCTask = sCTaskFind( idTask );
	if (pCTask)
	{
	
		// update count
		TASK_TEMPLATE* pTaskTemplate = &pCTask->tTaskTemplate;
		pTaskTemplate->nExterminatedCount = nCount;

		// update UI
		if( AppIsHellgate() )
		{
			UIUpdateQuestLog( QUEST_LOG_UI_VISIBLE );			
		}
		else
		{
			sUpdateTaskUI( TASK_UI_ACTIVE_TASK, &pCTask->tTaskTemplate, NULL, pCTask->eStatus, pCTask );
			sUpdateTaskUI( TASK_UI_TASK_SCREEN, &pCTask->tTaskTemplate, NULL, pCTask->eStatus, pCTask );
		}
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskRewardTaken(
	GAMETASKID idTaskServer,
	UNITID idReward)
{
	CTASK *pCTask = sCTaskFind( idTaskServer );
	if (pCTask)
	{
		TASK_TEMPLATE *pTaskTemplate = &pCTask->tTaskTemplate;
		
		// unlink reward from task
		for (int i = 0; i < MAX_TASK_REWARDS; ++i)
		{
			if (pTaskTemplate->idRewards[ i ] == idReward)
			{
				pTaskTemplate->idRewards[ i ] = INVALID_ID;
			}
		}
		
		// increment num taken
		pTaskTemplate->nNumRewardsTaken++;		
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TasksRestrictedByFaction(
	UNITID idGiver,
	int *pnFactionBad)
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	
	// get dialog text
	const WCHAR *puszDialog = c_DialogTranslate( DIALOG_TASKS_RESTRICTED_BY_FACTION );

	// copy to a buffer we can modify
	WCHAR uszText[ MAX_STRING_ENTRY_LENGTH ];
	PStrCopy( uszText, puszDialog, MAX_STRING_ENTRY_LENGTH );

	// add each of the factions in the bad faction array to the end of the text
	PStrCat( uszText, L"\n\n", MAX_STRING_ENTRY_LENGTH );
	for (int i = 0; i < FACTION_MAX_FACTIONS; ++i)
	{
		int nFaction = pnFactionBad[ i ];
		if (nFaction != INVALID_LINK)
		{
			int nFactionStanding = FactionGetStanding( pPlayer, nFaction );
			
			PStrCat( uszText, FactionGetDisplayName( nFaction ), MAX_STRING_ENTRY_LENGTH );
			PStrCat( uszText, L" (", MAX_STRING_ENTRY_LENGTH );
			PStrCat( uszText, FactionStandingGetDisplayName( nFactionStanding ), MAX_STRING_ENTRY_LENGTH );			
			PStrCat( uszText, L")", MAX_STRING_ENTRY_LENGTH );
			PStrCat( uszText, L"\n", MAX_STRING_ENTRY_LENGTH );
			
		}
		
	}
	
	// display the conversation dialog		
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = idGiver;
	tSpec.puszText = uszText;
	UIDisplayNPCDialog( pGame, &tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TaskIsTaskActive(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	
	return pCTaskGlobals->idActiveTaskCurrent != INVALID_ID;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_TaskIsTaskActiveInThisLevel(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	if(pCTaskGlobals->idActiveTaskCurrent == INVALID_ID)
	{
		return FALSE;
	}
	UNIT * pPlayer = GameGetControlUnit(AppGetCltGame());
	ASSERT_RETFALSE(pPlayer);
	LEVEL * pLevel = UnitGetLevel(pPlayer);
	ASSERT_RETFALSE(pLevel);
	if( AppIsHellgate() )
	{
		return LevelGetDefinitionIndex(pLevel) == pCTaskGlobals->activeTasks[pCTaskGlobals->idActiveTaskCurrent].tTaskTemplate.nLevelDefinition;
	}
	else
	{
		return LevelGetDepth(pLevel) == pCTaskGlobals->activeTasks[pCTaskGlobals->idActiveTaskCurrent].tTaskTemplate.nLevelDepth &&
			   LevelGetLevelAreaID(pLevel ) == pCTaskGlobals->activeTasks[pCTaskGlobals->idActiveTaskCurrent].tTaskTemplate.nLevelArea;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskPrintActiveTaskInfo( 
	WCHAR *puszBuffer, 
	int nBufferSize)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	
	
	if (pCTaskGlobals->idActiveTaskCurrent != INVALID_ID)
	{
		CTASK *pCTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
		ASSERTX_RETURN( pCTask, "Unable to find client task" );

		// set flags for desired parts
		DWORD dwFlags = 0;
		SETBIT( dwFlags, TDP_DESC_BIT );
		SETBIT( dwFlags, TDP_REWARD_HEADER_BIT );				
		SETBIT( dwFlags, TDP_REWARD_TEXT_BIT );
		SETBIT( dwFlags, TDP_STATUS_HEADER_BIT );
		SETBIT( dwFlags, TDP_STATUS_BIT );

		if( AppIsTugboat() )
		{
			SETBIT( dwFlags, TDP_TITLE_BIT );
			return;
		}
		
		// get info
		sTaskPrintTemplateInfo( 
			&pCTask->tTaskTemplate,
			pCTask->eStatus,
			pCTask->dwAcceptedTick,
			dwFlags,
			puszBuffer,
			nBufferSize);			
								
	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TaskPrintActiveTaskTitle( 
	WCHAR *puszBuffer, 
	int nBufferSize)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();	
	
	if (pCTaskGlobals->idActiveTaskCurrent != INVALID_ID)
	{
		CTASK *pCTask = sCTaskFind( pCTaskGlobals->idActiveTaskCurrent );
		ASSERTX_RETURN( pCTask, "Unable to find client task" );

		// set flags for desired parts
		DWORD dwFlags = 0;
		SETBIT( dwFlags, TDP_TITLE_BIT );
		SETBIT( dwFlags, TDP_NO_NEWLINES_BIT );

		// get info
		sTaskPrintTemplateInfo( 
			&pCTask->tTaskTemplate,
			pCTask->eStatus,
			pCTask->dwAcceptedTick,
			dwFlags,
			puszBuffer,
			nBufferSize);			
								
	}	

}

/// OK, now some 'quest' stuff that is sort of sharing space with tasks right now



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAcceptQuest(
	int nQuestID,
	UNITID nQuestGiver,
	BOOL bSendToServer)
{

	// setup message and send to server
	if (bSendToServer == TRUE)
	{
		MSG_CCMD_QUEST_TRY_ACCEPT message;
		message.idQuestGiver = nQuestGiver;
		message.nQuestID = nQuestID;
		c_SendMessage( CCMD_QUEST_TRY_ACCEPT, &message );
	}
	
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CQuestTryAccept(
	void)
{
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();

	// get task we are thinking about accepting
	BOOL bAccepted = FALSE;

	if ( pCTaskGlobals->idAvailableQuestCurrent != INVALID_ID )
	{
		bAccepted = sAcceptQuest( pCTaskGlobals->idAvailableQuestCurrent, 
								  pCTaskGlobals->idAvailableQuestGiver, 
								  TRUE );
	}

	if (bAccepted == TRUE)
	{

		UIHideNPCDialog(CCT_OK);
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CQuestTryReject(
	void)
{

	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	pCTaskGlobals->idAvailableQuestCurrent = INVALID_ID;
	pCTaskGlobals->idAvailableQuestGiver = INVALID_ID;

		
	UIHideNPCDialog(CCT_CANCEL);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackQuestAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{
	CQuestTryAccept();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackQuestReject(
	void *pUserData, 
	DWORD dwCallbackData)
{
	CQuestTryReject();
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CQuestAvailableQuests( GAME *pGame,
						    const MSG_SCMD_AVAILABLE_QUESTS *pMessage)
{
	UNITID idQuestGiver = pMessage->idQuestGiver;
	CLIENT_TASK_GLOBALS *pCTaskGlobals = sClientTaskGlobalsGet();
	pCTaskGlobals->idAvailableQuestCurrent = pMessage->nQuest;
	pCTaskGlobals->idAvailableQuestGiver = pMessage->idQuestGiver;
	UNIT *pNPC = UnitGetById( pGame, idQuestGiver );
	UNIT *pPlayer = GameGetControlUnit( pGame );

	if( pNPC && pGame && pMessage->nQuest == INVALID_ID )
	{
		// first - find out if there are multiple dialogs available
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

		if( nQuestTaskSecondaryCount > 1 )
		{
			// setup a dialog to list all available tasks
			NPC_DIALOG_SPEC tSpec;

			tSpec.nDialog = pMessage->nDialog;
			tSpec.idTalkingTo = idQuestGiver;
			tSpec.eType = NPC_DIALOG_LIST;	
			tSpec.tCallbackOK.pfnCallback = sCallbackQuestAccept;
			tSpec.tCallbackCancel.pfnCallback = sCallbackQuestReject;
			// bring up dialog interface with a choice of tasks to speak about
			c_NPCTalkStart( pGame, &tSpec );
			return;	
		}
	}
	

	
	// setup dialog specfini
	NPC_DIALOG_SPEC tSpec;

	tSpec.nDialog = pMessage->nDialog;
	tSpec.idTalkingTo = idQuestGiver;
	tSpec.eType = NPC_DIALOG_OK_CANCEL;
	tSpec.tCallbackOK.pfnCallback = sCallbackQuestAccept;
	tSpec.tCallbackCancel.pfnCallback = sCallbackQuestReject;
	tSpec.nQuestDefId = pMessage->nQuestTask;				
		// bring up dialog interface with a choice to accept or not
	c_NPCTalkStart( pGame, &tSpec );
								
}

#endif //!SERVER_VERSION
