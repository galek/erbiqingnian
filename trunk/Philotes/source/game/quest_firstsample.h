//----------------------------------------------------------------------------
// FILE: quest_firstsample.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_FIRSTSAMPLE_H_
#define __QUEST_FIRSTSAMPLE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitFirstSample(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeFirstSample(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif