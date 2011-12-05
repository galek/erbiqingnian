//----------------------------------------------------------------------------
// FILE: quest_cleanup.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_CLEANUP_H_
#define __QUEST_CLEANUP_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitCleanup(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeCleanup(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif