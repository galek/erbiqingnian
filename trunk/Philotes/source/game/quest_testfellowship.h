//----------------------------------------------------------------------------
// FILE: quest_testfellowship.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TESTFELLOWSHIP_H_
#define __QUEST_TESTFELLOWSHIP_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTestFellowship(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTestFellowship(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestToFBegin( QUEST * pQuest );

#endif