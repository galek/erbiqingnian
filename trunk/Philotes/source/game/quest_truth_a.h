//----------------------------------------------------------------------------
// FILE: quest_truth_a.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRUTH_A_H_
#define __QUEST_TRUTH_A_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct UNIT;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTruthA(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTruthA(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif
