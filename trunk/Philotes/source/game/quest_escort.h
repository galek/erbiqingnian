//----------------------------------------------------------------------------
// FILE: quest_escort.h
//
// A template quest. Escort the Cast Giver to the level warp to the next 
// area. Then talk to him to finish the quest.
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_ESCORT_H_
#define __QUEST_ESCORT_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestEscortInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestEscortFree(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestEscortOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam);
#endif

