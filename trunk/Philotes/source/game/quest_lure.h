//----------------------------------------------------------------------------
// FILE: quest_lure.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_LURE_H_
#define __QUEST_LURE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitLure(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeLure(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif