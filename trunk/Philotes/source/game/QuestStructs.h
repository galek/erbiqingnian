// FILE: QuestProperties
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __QUESTSTRUCTS_H_
#define __QUESTSTRUCTS_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef __FONTCOLOR_H_
#include "fontcolor.h"
#endif
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
#define MAX_THEMES_PER_QUEST		40
//rem this out to enable maps again for quests
#define MYTHOS_QUEST_MAPS_DISABLED
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
struct MSG_SCMD_AVAILABLE_QUESTS;
struct MSG_SCMD_QUEST_TASK_STATUS;
struct ROOM;



const static int			QUEST_START_FLAG_IGNORE_LEVEL_REQ		= MAKE_MASK( 1 );
const static int			QUEST_START_FLAG_IGNORE_QUEST_COUNT		= MAKE_MASK( 2 );
const static int			QUEST_START_FLAG_LOWER_LEVEL_REQ_BY_5	= MAKE_MASK( 3 );
const static int			QUEST_START_FLAG_IGNORE_ITEM_COUNT		= MAKE_MASK( 4 );

enum QUEST_THEME_PROPERTIES
{
	QUEST_THEME_PROP_CLIENT_REMOVE,
	QUEST_THEME_PROP_CLIENT_ADD,
	QUEST_THEME_PROP_ADD,
	QUEST_THEME_COUNT
};






//STATIC FUNCTIONS and Defines
enum QUEST_MASK_TYPES
{
	QUEST_MASK_COMPLETED,
	QUEST_MASK_VISITED,
	QUEST_MASK_ACCEPTED,
	QUEST_MASK_COMPLETED_TEXT,	
	QUEST_MASK_ITEMS_REMOVED,
	QUEST_MASK_COUNT
};

			

//----------------------------------------------------------------------------
enum QUEST_CONSTANTS_FOR_TUGBOAT
{
	QUESTID_INVALID = -1,
	QUESTCODE_INVALID = -1,	
	MAX_COLLECT_PER_QUEST_TASK = 10,		// number of task items per section the quest can have ( collect 300 goblin heads, 400 nightflier wings, ect ect )
	MAX_QUEST_REWARDS = 6,					// number of quest rewards allowed
	MAX_GLOBAL_QUESTS_COMPLETED = 5,		// Number of global quest that has to be completed before quest task can continue
	MAX_QUEST_FUNCTION_NAME_LEN = 128,		// max function name length			
	MAX_MONSTERS_TO_SPAWN = 25,				// how many different monsters to spawn for a given quest
	MAX_OBJECTS_TO_SPAWN = 16,				// MAX 25 allowed!!! how many different objects to spawn for a given quest				
	MAX_PEOPLE_TO_TALK_TO = 5,				// Max num of people you can talk to and complete the quest
	MAX_ITEMS_TO_GIVE_ACCEPTING_TASK = 5,	// Max num of items to give the player if they accept the task	
	MAX_DUNGEONS_TO_GIVE = 5,				// This is the max number of dungeons to give people
	MAX_STATES_TO_CHECK = 3,				// this is the number of states the quest checks to be able to start the quest.
	MAX_UNITTYPES_TO_CHECK = 10,			// the number of unittypes to check against	
	MAX_DUNGEONS_VISIBLE = MAX_MONSTERS_TO_SPAWN + MAX_OBJECTS_TO_SPAWN,
	MAX_NPC_QUEST_TASK_AVAILABLE = 20,		// this is the number of tasks available for a single NPC at any one time.	
	MAX_POINTS_OF_INTEREST = 5,				// the max number of points of interest to have on a quest
};

enum QUESTTASK_SAVE_OFFSETS
{
	QUESTTASK_SAVE_NPC = 0,
	QUESTTASK_SAVE_MONSTERS = MAX_PEOPLE_TO_TALK_TO,
	QUESTTASK_SAVE_GIVE_REWARD,
	QUESTTASK_SAVE_HAS_GIVE_ACCEPT_ITEMS,
	QUESTTASK_SAVE_FREE_FOR_DESIGNER = 20	//why 20? Just because I awanted to save some more room for other bits that can be saved.
};
//For the different types of Scripting codes we can have
const enum QUEST_WORLD_UPDATES
{
	QUEST_WORLD_UPDATE_QSTATE_BEFORE,
	QUEST_WORLD_UPDATE_QSTATE_START,
	QUEST_WORLD_UPDATE_QSTATE_COMPLETE,
	QUEST_WORLD_UPDATE_QSTATE_TALK_NPC,
	QUEST_WORLD_UPDATE_QSTATE_AFTER_TALK_NPC,
	QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED,
	QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED_CLIENT,
	QUEST_WORLD_UPDATE_QSTATE_INTERACT_QUESTUNIT,
	QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT,
	QUEST_WORLD_UPDATE_QSTATE_TASK_COMPLETE,
	QUEST_WORLD_UPDATE_QSTATE_OBJECT_OPERATED,
	QUEST_WORLD_UPDATE_QSTATE_OBJECT_OPERATED_CLIENT,
	QUEST_WORLD_UPDATE_QSTATE_ABANDONED,
	QUEST_WORLD_UPDATE_QSTATE_ABANDONED_CLIENT,
	QUEST_WORLD_UPDATE_COUNT
};


enum EQUEST_TASK_MESSAGES
{
	QUEST_TASK_MSG_INVALID,
	QUEST_TASK_MSG_TASKS_COMPLETE,
	QUEST_TASK_MSG_QUEST_COMPLETE,
	QUEST_TASK_MSG_QUEST_ABANDON,
	QUEST_TASK_MSG_QUEST_ITEM_FOUND,
	QUEST_TASK_MSG_UI_INVENTORY,
	QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_ACCEPT,
	QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_COMPLETE,
	QUEST_TASK_MSG_QUEST_ACCEPTED,
	QUEST_TASK_MSG_COMPLETE,
	QUEST_TASK_MSG_OBJECT_OPERATED,
	QUEST_TASK_MSG_BOSS_DEFEATED,
	QUEST_TASK_MSG_LEVEL_BEING_REMOVED,
	QUEST_TASK_MSG_MAP_LEARNED,
	QUEST_TASK_MSG_COUNT
};

enum EQUESTTB_FLAGS
{
	QUESTTB_FLAG_MUST_FINISH_IN_DUNGEON
};




//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//used for counting the max number of quests a player is allowed to own.
struct QUEST_COUNT_TUGBOAT
{
	WORD							wCode;
};
//----------------------------------------------------------------------------
struct QUEST_DICTIONARY_DEFINITION_TUGBOAT
{
	int								nStringDefTableLink;
	int								nStringTableLink;
	WORD							wCode;					//code for the definition
};

struct QUEST_THEMES
{
	int								nQuestDungeonAreaThemes[ MAX_THEMES_PER_QUEST ];
	int								nQuestDungeonThemes[ MAX_THEMES_PER_QUEST ];
	int								nQuestDungeonThemeLevels[ MAX_THEMES_PER_QUEST ];
	int								nQuestDungeonThemeProperties[ MAX_THEMES_PER_QUEST ];
};

struct QUEST_REWARD_LEVEL_TUGBOAT
{
	int								nExpMin;
	int								nExpMax;
	int								nGoldMin;
	int								nGoldMax;
};

struct QUEST_TASK_DEFINITION_TUGBOAT
{
	char							szName[DEFAULT_INDEX_SIZE];	// unique task name for quest
	int								nNameStringKey;					// key for name of task
	WORD							wCode;							// unique quest task id	
	int								nNPC;							// NPC who you talk to for task
	int								nTaskIndexIntoTable;
	BOOL							bDoNotShowCompleteMessage;		// this makes the complete message not show
	
	int								nQuestTaskCompleteTextID;		// this appears in the dungeon when the task is complete
	int								nQuestDialogOnTaskCompleteAnywhere;	// Dialog that pops up when the player has completed the tasks for a given Quest Task - NOTE: this appears where the player finish's the task; not when talking with an NPC.
	int								nQuestDialogID;					// quest dialog ID	
	int								nQuestDialogComeBackID;			// quest dialog ID when the player comes back
	int								nQuestDialogCompleteID;			// quest dialog ID on quest completion
	int								nQuestMaskLocal;				// mask for quest	
	int								nDescriptionBeforeMeeting;		// what shows up in the quest tracking before talking to the person
	int								nDescriptionString;				// what shows up in the quest tracking panel
	int								nCompleteDescriptionString;		// what shows up in the quest tracking panel when the task is complete
	BOOL							bQuestNPCDialogToParty;			// THis is if a NPC is talked to it will broadcast it to the entire party
	BOOL							bQuestSecondaryNPCDialogToParty;// THis is if a NPC is talked to it will broadcast it to the entire party
	/////Items to give if accepting task
	int								nQuestGiveItemsOnAccept[ MAX_ITEMS_TO_GIVE_ACCEPTING_TASK ];			//Items to give the player if they accept a task
	BOOL							bDoNotAutoGiveMaps;
	/////Dungeons to give
	int								nQuestLevelAreaOnTaskAccepted[MAX_DUNGEONS_TO_GIVE];						//number of level areas to give on Task Accepted
	int								nQuestLevelAreaOnTaskCompleteNoRemove[MAX_DUNGEONS_TO_GIVE];				//tells the dungeon not to remove after the task is accepted.
	int								nQuestLevelAreaOnTaskCompleted[MAX_DUNGEONS_TO_GIVE];						//number of level areas to give on Task Completed
	int								nQuestLevelAreaNPCCommunicate[MAX_DUNGEONS_TO_GIVE];						//level area to give once you talk to somebody
	int								nQuestLevelAreaNPCToCommunicateWith[MAX_DUNGEONS_TO_GIVE];					//The actual NPC to talk with
	////end additional 
	///Level Area definitions
	int								nQuestTaskDungeonsVisible[ MAX_DUNGEONS_VISIBLE ];	
	
	///end level area defintions
	int								nQuestQuestsToFinish[MAX_GLOBAL_QUESTS_COMPLETED];			// Guests to finish to complete task
	int								nQuestCanStartOnQuestsCom[ MAX_GLOBAL_QUESTS_COMPLETED ];	// the quests that must be completed before the task will start
	int								nQuestCanStartOnQuestActive[ MAX_GLOBAL_QUESTS_COMPLETED ];	// can only be active at the  same time as the current task
	int								nQuestItemsIDToCollect[MAX_COLLECT_PER_QUEST_TASK];			// List of items ID to collect
	int								nQuestItemsFlavoredTextToCollect[MAX_COLLECT_PER_QUEST_TASK];			// List of item names to collect
	int								nQuestItemsSpawnUniqueForAttacker[MAX_COLLECT_PER_QUEST_TASK];	//if this is set, the item will only spawn for a specific player
	PCODE							pQuestItemValidationCode;											//script that validates the item when attempting to complete the quest
	int								nQuestItemsAppearOnLevel[MAX_COLLECT_PER_QUEST_TASK];		// List of level IDs that the items will appear
	int								nQuestItemsWeight[ MAX_COLLECT_PER_QUEST_TASK ];			// this holds the weight to add for items.
	int								nQuestItemsNumberToCollect[MAX_COLLECT_PER_QUEST_TASK];			// List of items ID to collect	
	int								nQuestRewardItemID[MAX_QUEST_REWARDS];							//number of rewards allowed		
	int								nQuestRewardChoicesItemID[MAX_QUEST_REWARDS];					//number of rewards to choose from
	int								nQuestRewardChoiceCount;										//number of rewards the player is allowed to choose.
	int								nQuestGivesItems;												// quest gives items

	int								nUnitMonstersToSpawnFrom[ MAX_COLLECT_PER_QUEST_TASK ];			//the monster to spawn the items from.
	int								nUnitTypesToSpawnFrom[ MAX_COLLECT_PER_QUEST_TASK ];			//the unit type to spawn the items from.
	int								nUnitQualitiesToSpawnFrom[ MAX_COLLECT_PER_QUEST_TASK ];			//the units to spawn the items from.	
	int								nUnitPercentChanceOfDrop[ MAX_COLLECT_PER_QUEST_TASK ];			//the units to spawn the items from.

	//////////////////Recipes to give and remove
	int								nQuestGiveRecipesOnQuestAccept[MAX_COLLECT_PER_QUEST_TASK];			// recipes to give
	int								nQuestRemoveRecipesAtQuestEnd[MAX_COLLECT_PER_QUEST_TASK];			// recipes to remove

	/////////////////Items to remove
	int								nQuestItemsRemoveAtQuestEnd[ MAX_COLLECT_PER_QUEST_TASK ];		// if the items that got spawned should be removed at the end of the task	
	int								nQuestAdditionalItemsToRemove[MAX_COLLECT_PER_QUEST_TASK];			// Additional items to remove
	int								nQuestAdditionalItemsToRemoveCount[MAX_COLLECT_PER_QUEST_TASK];			// Additional items to remove

	int								nParentQuest;
	/////////////////Monster Creation Variables
	int								nQuestMonstersIDsToSpawn[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestDungeonsToSpawnMonsters[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestLevelsToSpawnMonstersIn[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestMonstersToSpawnQuality[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestPacksCountToSpawn[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestNumOfMonstersPerPack[ MAX_MONSTERS_TO_SPAWN ];
	int								nQuestMonstersUniqueNames[ MAX_MONSTERS_TO_SPAWN ];			
	/////////////////End Monster Creation Variables
	/////////////////Object Creation Variables
	int								nQuestObjectIDsToSpawn[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectCreateItemIDs[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectCreateItemFlavoredText[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestDungeonsToSpawnObjects[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestDungeonsToSpawnBossObjects[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestLevelsToSpawnObjectsIn[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestLevelsToSpawnBossObjectsIn[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossIDsToSpawn[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossQuality[ MAX_OBJECTS_TO_SPAWN ];	
	int								nQuestBossItemDrop[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossNotNeededToCompleteQuest[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossNoTaskDescription[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossItemFlavoredText[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBossUniqueNames[ MAX_MONSTERS_TO_SPAWN ];	
	int								nQuestBossSpawnFromUnitType[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBucketToIncrement[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBucketToLockBy[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestBucketToUnLockByCount[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectTextWhenUsed[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectSkillWhenUsed[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectRemoveItemWhenUsed[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectRequiresItemWhenUsed[ MAX_OBJECTS_TO_SPAWN ];
	int								nQuestObjectGiveItemDirectlyToPlayer[ MAX_OBJECTS_TO_SPAWN ];	

	/////////////////End Object Creation Variables
	/////////////////Script Variables	
	PCODE							pQuestCodes_Server[ QUEST_WORLD_UPDATE_COUNT ];
	/////////////////End Script Variables
	/////////////////People to talk to
	int								nQuestSecondaryNPCTalkTo[ MAX_PEOPLE_TO_TALK_TO ];
	int								nQuestNPCTalkToDialog[ MAX_PEOPLE_TO_TALK_TO ];
	int								nQuestNPCTalkQuestCompleteDialog[ MAX_PEOPLE_TO_TALK_TO ];
	int								nQuestNPCTalkToCompleteQuest[ MAX_PEOPLE_TO_TALK_TO ];	
	/////////////////End people to talk to
	/////////////////Points of Interest
	int								nQuestPointsOfInterestMonsters[ MAX_POINTS_OF_INTEREST ];
	int								nQuestPointsOfInterestObjects[ MAX_POINTS_OF_INTEREST ];
	int								nQuestPointsOfInterestMonstersOnComplete[ MAX_POINTS_OF_INTEREST ];
	int								nQuestPointsOfInterestObjectsOnComplete[ MAX_POINTS_OF_INTEREST ];

	/////////////////End Points of Interest
	/////////////////Themes
	QUEST_THEMES					m_QuestThemes;
	BOOL							m_bHasThemes;
	/*
	int								nQuestDungeonAreaThemes[ MAX_THEMES_PER_LEVEL ];
	int								nQuestDungeonThemes[ MAX_THEMES_PER_LEVEL ];
	int								nQuestDungeonThemeLevels[ MAX_THEMES_PER_LEVEL ];
	int								nQuestDungeonThemeProperties[ MAX_THEMES_PER_LEVEL ];
	*/
	/////////////////End Themes
	/////////////////Flags
	DWORD							dwFlags;
	/////////////////Duration
	int								nQuestDuration;
	/////////////////End Duration
	SAFE_PTR(const QUEST_TASK_DEFINITION_TUGBOAT*, pPrev);
	SAFE_PTR(const QUEST_TASK_DEFINITION_TUGBOAT*, pNext);
};


const enum EQUEST_RANDOM_TASK_STATS
{
	KQUEST_RANDOM_TASK_STAT_SPAWN_CLASS_COUNT = 10,
	KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT = 25,
	KQUEST_RANDOM_TASK_STAT_LEVELAREAS_COUNT = 10,
	KQUEST_RANDOM_TASK_STAT_REWARD_COUNT = 5,
	KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_COUNT = 5,
	KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_SIZE = 5,
};

const enum EQUEST_STRINGS
{
	KQUEST_STRING_TITLE,					//task
	KQUEST_STRING_INTRO,					//task
	KQUEST_STRING_ACCEPTED,					//task
	KQUEST_STRING_RETURN,					//task
	KQUEST_STRING_COMPLETE,					//task
	KQUEST_STRING_LOG,						//task
	KQUEST_STRING_LOG_COMPLETE,				//task
	KQUEST_STRING_LOG_BEFORE_MEET,			//task
	KQUEST_STRING_TASK_COMPLETE_MSG,		//message in center of screen
	KQUEST_STRING_QUEST_COMPLETE_MSG,		//message in center of screen				
	KQUEST_STRING_QUEST_TITLE,				//quest
	KQUEST_STRING_COUNT,	
};



struct QUEST_RANDOM_TASK_DEFINITION_TUGBOAT
{
	char		szName[DEFAULT_INDEX_SIZE];														// unique name
	WORD		wCode;																			// unique id		
	int			nId;																			// Id - Excel index
	int			nParentID;																		// parent id
	int			nStrings[ KQUEST_STRING_COUNT ];									// the number of strings used in the random task
	BOOL		bFindRandomLevelAreaInZone;														// finds a dungeon that matches the requirements of hte quest
	int			nNumOfLevelAreasToFind;															// It'll go out and find X number of level areas
	BOOL		bIsGlobal;																		// global or not
	BOOL		bSpawnFromBoss;																	// if the items spawns from a boss
	BOOL		bSpawnFromChest;																// if the item spawns from a chest
	int			nLevelAreas[ KQUEST_RANDOM_TASK_STAT_LEVELAREAS_COUNT ];						// number of random level areas to choose
	int			nLevelAreasCount;																// Number of level areas to choose from
	int			nNumOfItemsToCollect;															// the number of items to collect 
	int			nItemsToCollect[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];				// Bucket of items to choose to collect
	int			nItemsToCollectCount;
	int			nItemSpawnGroupIndex[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];			// specifies if it's spawning from a group or not.
	int			nItemsSpawnFrom[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];				// Unit types to spawn from	
	int			nItemSpawnFromGroupOfUnitTypes[ KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_COUNT ][ KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_SIZE ];	//the group of monsters to spawn from
	int			nItemSpawnFromGroupOfUnitTypesCount[ KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_COUNT ];		// this stores how many unittypes per group.

	int			nItemsMinCollect[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];				// Min number to collect
	int			nItemsMaxCollect[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];				// Max number to collect
	int			nItemsChanceToSpawn[ KQUEST_RANDOM_TASK_STAT_ITEM_COLLECTION_COUNT ];			// Chance to spawn item
	int			nCollectItemFromSpawnClass[ KQUEST_RANDOM_TASK_STAT_SPAWN_CLASS_COUNT ];		// This will choose an item from the spawn class to collect
	int			nCollectItemFromSpawnClassCount;												// Number of spawn classes
	int			nBossSpawnClasses[ KQUEST_RANDOM_TASK_STAT_SPAWN_CLASS_COUNT ];					// Spawn classes to choose Boss from
	int			nBossSpawnClassesCount;															// Number of spawn classes
	BOOL		bBossNotRequiredtoComplete;															// Does the boss have to be killed to complete the quest?
	int			nRewardOfferings[ KQUEST_RANDOM_TASK_STAT_REWARD_COUNT ];						// This will overwrite the reward spawn from quests
	int			nRewardOfferingsCount;															// Number of reward Offerings.
	int			nDurationLength[ 2 ];															// THe min and max duration length
	int			nGoldPCT[ 2 ];																	// The Gold PCT of the level
	int			nXPPCT[ 2 ];																	// The XP PCT of the level
	SAFE_PTR(const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT*, pPrev);								// pointer to prev random task
	SAFE_PTR(const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT*, pNext);								// pointer to next random task

};


struct QUEST_RANDOM_DEFINITION_TUGBOAT
{
	char							szName[DEFAULT_INDEX_SIZE];		// unique name
	WORD							wCode;							// unique id		
	int								nRandomTaskCount;				// random task count
	int								nId;							// id of the Random quest ( index into Excel file )
	SAFE_PTR(const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT*, pFirst);	// pointer to first random task
};

struct QUEST_DEFINITION_TUGBOAT
{
	char		szName[DEFAULT_INDEX_SIZE];								// task name
	WORD		wCode;													// code
	int			nID;													// ID of the quest
	int			nQuestTitleID;											// quest title
	int			nQuestDialogIntroID;									// quest dialog ID for intro
	int			nMaskForCompletion;										// this is the completion mask
	int			nQuestCompleteTextID;									// this appears when you talk to an NPC
	BOOL		bQuestAlwaysOn;						
	int			nQuestPlayerStartLevel;									// Level player has to be to start the quest
	int			nQuestPlayerEndLevel;									// Level in which the player has to be at to get the quest.		
	int			nQuestItemNeededToStart;								// Item needed to start Quest
	int			nQuestStatesNeeded[MAX_STATES_TO_CHECK];				// States that the player needs to start
	int			nQuestUnitTypeNeeded[MAX_UNITTYPES_TO_CHECK];			// Unittypes that the player has to be to start
	BOOL		bQuestCannnotBeAbandoned;								// some quests cannot be abandoned.
	int			nQuestRandomID;											// Random Quest ID
	int			nQuestRewardOfferClass;									// Reward Treasure
	int			nQuestRewardGold_PCT;									// Reward percent of gold
	int			nQuestRewardGold;										// Actual Gold Reward
	int			nQuestRewardXP_PCT;										// Reward percent of XP
	int			nQuestRewardXP;											// Actual XP reward
	int			nQuestRewardLevel;										// Reward Level
	SAFE_PTR( const QUEST_RANDOM_DEFINITION_TUGBOAT*, pRandomQuest );	// this is for random quests.
	SAFE_PTR(const QUEST_TASK_DEFINITION_TUGBOAT*, pFirst);
};


//----------------------------------------------------------------------------
//Used in levels to store certain info for respawning monsters
#define QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS			20
struct QUEST_LEVEL_INFO_TUGBOAT
{
	int nUnitsTracking;													//number of units tracking
	int nUnitIDsTracking[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];		//Unit id's
	int nFloorIndexForUnitIDs[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];  //Floor for the unit
	const ROOM *pRoomOfUnit[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];
	const QUEST_DEFINITION_TUGBOAT *pQuestDefForUnit[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskDefForUnit[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];	
	int nQuestTypeInQuest[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];
	int nQuestIndexOfUnit[ QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS ];
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern int g_QuestStatIDs[ 5 ];
extern int g_QuestTaskSaveIDs[ 5 ];
extern int MAX_ACTIVE_QUESTS_PER_UNIT;			// how many quests a player can have active at any time	
#endif