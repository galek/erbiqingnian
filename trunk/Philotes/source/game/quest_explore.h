//----------------------------------------------------------------------------
// FILE: quest_explore.h
//
// The explore quest is a template quest, in which the player must visit a
// specified precentage of the rooms in a level. 
//
// Code is adapted from the explore task, in tasks.cpp.
// 
// author: Chris March, 2-1-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_EXPLORE_H_
#define __QUEST_EXPLORE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestExploreInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestExploreFree(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif