//----------------------------------------------------------------------------
// globals.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _GLOBALS_H_
#define _GLOBALS_H_

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

// I don't think that either of these belongs here, but they don't really have
// any other home right now, so they'll stay here until we can find a better place.
// - GS

#define MAX_CHARACTER_NAME				32
#define MAX_GUILD_NAME					32
#define	PARTY_NAME_SIZE					20
#define PARTY_DESC_SIZE					100
#define MAX_CHARACTER_LEVEL				999

enum
{
	WEAPON_INDEX_RIGHT_HAND = 0,
	WEAPON_INDEX_LEFT_HAND,
	MAX_WEAPONS_PER_UNIT
};

// Task/Quest stuff
//----------------------------------------------------------------------------
enum TASK_MESSAGE_CONSTANTS
{
	MAX_AVAILABLE_TASKS_AT_GIVER = 1,	// max tasks a player can select from at any task giver
	MAX_TASK_REWARDS = 3,				// max task rewards (increase this and you must also add stats to save the items)
	MAX_TASK_UNIT_TARGETS = 3,			// max task unit targets (increase this and you need to add stats to save the targets)
	MAX_TASK_COLLECT_TYPES = 3,			// when collecting stuff, this is what is needed (increase this and you must also add stats to save collect items)
	MAX_TASK_EXTERMINATE = 2,			// when exterminating lots of a type of thing, this is the thing and how many to exterminate (if you increase this you must also add saving stats)
	MAX_TASK_FACTION_REWARDS = 4,		// max faction scoes that can be affected when task is complete
};

#define MAX_ITEM_NAME_STRING_LENGTH		256

const int MAX_TIME_STRING = 32;
const int MAX_REMAINING_STRING = 32;
const int MAX_TIME_COLOR_STRING = 3;
const int MINUTES_PER_HOUR = 60;
const int MAX_COLOR_STRING = 3;

const int MAX_QUEST_TITLE_LENGTH = 128;
const int MAX_QUEST_DESC_STRING_LENGTH = 2048;
const int MAX_REWARD_DESCRIPTION_LENGTH = (MAX_ITEM_NAME_STRING_LENGTH + 400) * MAX_TASK_REWARDS;
const int MAX_QUEST_STATUS_STRING_LENGTH = 256;


//#define DRB_3RD_PERSON				1

#endif // _GLOBALS_H_