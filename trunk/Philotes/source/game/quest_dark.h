//----------------------------------------------------------------------------
// FILE: quest_dark.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DARK_H_
#define __QUEST_DARK_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitDark(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeDark(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif