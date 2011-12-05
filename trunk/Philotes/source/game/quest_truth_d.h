//----------------------------------------------------------------------------
// FILE: quest_truth_d.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRUTH_D_H_
#define __QUEST_TRUTH_D_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void s_QuestTruthDBegin( QUEST * pQuest );

void QuestInitTruthD(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTruthD(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif