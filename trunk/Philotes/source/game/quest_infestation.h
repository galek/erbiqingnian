//----------------------------------------------------------------------------
// FILE: quest_infestation.h
//
// The infestation quest is a template quest, in which the player must kill 
// monsters from a specified class. You can also specify a level to find the 
// monsters in.
//
// Code is adapted from the extermination task, in tasks.cpp.
// 
// author: Chris March, 1-31-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_INFESTATION_H_
#define __QUEST_INFESTATION_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInfestationInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestInfestationFree(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif