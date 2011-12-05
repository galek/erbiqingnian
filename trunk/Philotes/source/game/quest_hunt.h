//----------------------------------------------------------------------------
// FILE: quest_hunt.h
//
// The hunt quest is a template quest, in which the player must kill a 
// a monster with a proper name in a specified level. If the monster does
// not exist in the level when the player enters (with the quest active) the
// monster is spawned.
//
// Code is adapted from the hunt task, in tasks.cpp.
// 
// author: Chris March, 1-10-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_HUNT_H_
#define __QUEST_HUNT_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestHuntInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestHuntFree(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestHuntOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif