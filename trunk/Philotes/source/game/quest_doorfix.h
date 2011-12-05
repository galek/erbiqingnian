//----------------------------------------------------------------------------
// FILE: quest_doorfix.h
//
// Story quest for Act 1. Collect items to open up the passageway. Based on 
// collect template code.
//
// author: Chris March, 6-13-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DOORFIX_H_
#define __QUEST_DOORFIX_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitDoorFix(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeDoorFix(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestOnEnterGameDoorFix(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif