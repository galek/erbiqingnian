//----------------------------------------------------------------------------
// FILE: quest_deep.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DEEP_H_
#define __QUEST_DEEP_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitDeep(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeDeep(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif