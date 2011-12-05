//----------------------------------------------------------------------------
// FILE: quest_redout.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_REDOUT_H
#define __QUEST_REDOUT_H

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitRedout(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeRedout(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif