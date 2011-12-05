//----------------------------------------------------------------------------
// FILE: quest_helprescue.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_HELPRESCUE_H_
#define __QUEST_HELPRESCUE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitHelpRescue(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeHelpRescue(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif