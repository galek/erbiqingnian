//----------------------------------------------------------------------------
// FILE: quest_testbeauty.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TESTBEAUTY_H_
#define __QUEST_TESTBEAUTY_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTestBeauty(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTestBeauty(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestToBBegin( QUEST * pQuest );

#endif