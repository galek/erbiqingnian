//----------------------------------------------------------------------------
// FILE: quest_truth_e.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRUTH_E_H_
#define __QUEST_TRUTH_E_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTruthE(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTruthE(
	const QUEST_FUNCTION_PARAM &tParam);
	
void s_QuestTruthEBegin( QUEST * pQuest );

#endif