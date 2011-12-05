//----------------------------------------------------------------------------
// FILE: quest_truth_c.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRUTH_C_H_
#define __QUEST_TRUTH_C_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void s_QuestTruthCStart(
	QUEST * pQuest );

void QuestInitTruthC(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTruthC(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif