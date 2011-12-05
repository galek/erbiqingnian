//----------------------------------------------------------------------------
// FILE: quest_intro.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_INTRO_H_
#define __QUEST_INTRO_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitIntro(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeIntro(
	const QUEST_FUNCTION_PARAM &tParam);

#endif
