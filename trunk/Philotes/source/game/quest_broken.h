//----------------------------------------------------------------------------
// FILE: quest_broken.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_BROKEN_H_
#define __QUEST_BROKEN_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitBroken(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeBroken(
	const QUEST_FUNCTION_PARAM &tParam);

#endif
