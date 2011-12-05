//----------------------------------------------------------------------------
// FILE: quest_holdfast.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_HOLDFAST_H
#define __QUEST_HOLDFAST_H

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitHoldFast(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeHoldFast(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif