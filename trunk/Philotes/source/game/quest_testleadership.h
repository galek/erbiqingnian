//----------------------------------------------------------------------------
// FILE: quest_testleadership.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TESTLEADERSHIP_H_
#define __QUEST_TESTLEADERSHIP_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTestLeadership(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTestLeadership(
	const QUEST_FUNCTION_PARAM &tParam);
	
void s_QuestToLBegin( QUEST * pQuest );

#endif