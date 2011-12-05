//----------------------------------------------------------------------------
// FILE: quest_trainfix.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRAINFIX_H_
#define __QUEST_TRAINFIX_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTrainFix(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTrainFix(
	const QUEST_FUNCTION_PARAM &tParam);
	
void s_QuestTrainFixOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam);

#endif