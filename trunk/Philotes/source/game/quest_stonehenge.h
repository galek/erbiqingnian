//----------------------------------------------------------------------------
// FILE: quest_stonehenge.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_STONEHENGE_H_
#define __QUEST_STONEHENGE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitStonehenge(const QUEST_FUNCTION_PARAM &tParam);
void QuestFreeStonehenge(const QUEST_FUNCTION_PARAM &tParam);
void s_QuestStonehengeOnEnterGame(const QUEST_FUNCTION_PARAM &tParam);


#endif
