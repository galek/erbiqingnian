//----------------------------------------------------------------------------
// FILE: quest_secondsample.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_SECONDSAMPLE_H_
#define __QUEST_SECONDSAMPLE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitSecondSample(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeSecondSample(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif