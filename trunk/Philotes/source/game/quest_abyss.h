//----------------------------------------------------------------------------
// FILE: quest_abyss.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_ABYSS_H_
#define __QUEST_ABYSS_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitAbyss(const QUEST_FUNCTION_PARAM &tParam);
void QuestFreeAbyss(const QUEST_FUNCTION_PARAM &tParam);
void s_QuestAbyssOnEnterGame(const QUEST_FUNCTION_PARAM &tParam);


#endif
