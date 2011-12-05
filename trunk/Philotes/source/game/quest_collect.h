//----------------------------------------------------------------------------
// FILE: quest_collect.h
//
// The collect quest is a template quest, in which the player must collect 
// items from a specified treasure class. You can also specify a class of 
// monster to get the drops from, and a level to find them in.
//
// Code is adapted from the forge and urgent collect tasks, in tasks.cpp.
// 
// author: Chris March, 1-29-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_COLLECT_H_
#define __QUEST_COLLECT_H_

#define MAX_QUEST_COLLECT_SPAWN_ROOMS		(32)
#define MAX_QUEST_COLLECT_OBJECTIVE_COUNT	(32)
#define MAX_QUEST_COLLECT_MONSTER_SPAWN		(256.0f)

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;
struct LEVEL;
struct QUEST_MESSAGE_CREATE_LEVEL;
struct QUEST_MESSAGE_ENTER_LEVEL;
struct QUEST_MESSAGE_PARTY_KILL;
enum QUEST_MESSAGE_RESULT;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_COLLECT
{
	BOOL bDataCalcAttempted;									// TRUE after an attempt was made to calc the data values in this struct

	BOOL bDidKill;												// at least one kill since sQuestDataInit was last called
	TIME tiLastKillTime;										// time of last kill, or zero		

	int nKillCountsForDrops[MAX_QUEST_COLLECT_OBJECTIVE_COUNT]; // drop a quest item when the player has killed this many quest monsters, sorted array of length QUEST_DEFINITION::nObjectiveCount
	ROOMID idSpawnRooms[MAX_QUEST_COLLECT_SPAWN_ROOMS];			// ROOMIDs for picked spawn rooms
	int nSpawnRoomCount;										// how many spawn room indexes are in the array
	int nMaxMonsterSpawnsPerRoom;								// the max number of monsters to spawn in a spawn room when it is first activated

	int nKillCount;												// how many quest monsters has the player killed since this level was (re)created
	int nDropCount;												// how many quest items the quest has dropped in this level since it was last created
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestCollectInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestCollectFree(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestCollectOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam);
	
QUEST_MESSAGE_RESULT s_QuestCollectMessageCreateLevel(
	QUEST *pQuest,
	const QUEST_MESSAGE_CREATE_LEVEL *pMessageCreateLevel,
	QUEST_DATA_COLLECT *pData );

// the functions below are shared with story quests that do collection
QUEST_MESSAGE_RESULT s_QuestCollectMessageRoomsActivated( 
	GAME* pGame,
	QUEST* pQuest,
	QUEST_DATA_COLLECT *pData);

QUEST_MESSAGE_RESULT s_QuestCollectMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel,
	QUEST_DATA_COLLECT *pData );

QUEST_MESSAGE_RESULT s_QuestCollectMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage,
	QUEST_DATA_COLLECT *pQuestData);

void QuestCollectDataInit(
	QUEST *pQuest,
	QUEST_DATA_COLLECT *pQuestData);


#endif