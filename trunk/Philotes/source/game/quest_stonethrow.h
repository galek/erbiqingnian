//----------------------------------------------------------------------------
// FILE: quest_stonethrow.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_STONETHROW_H_
#define __QUEST_STONETHROW_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitStoneThrow(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeStoneThrow(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif
