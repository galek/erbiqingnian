//----------------------------------------------------------------------------
// FILE: quest_timeorb.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TIMEORB_H_
#define __QUEST_TIMEORB_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTimeOrb(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTimeOrb(
	const QUEST_FUNCTION_PARAM &tParam);


#endif
