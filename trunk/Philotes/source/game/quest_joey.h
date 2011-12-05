//----------------------------------------------------------------------------
// FILE: quest_joey.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_JOEY_H_
#define __QUEST_JOEY_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitJoey(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeJoey(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif