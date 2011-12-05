//----------------------------------------------------------------------------
// FILE: quest_hellyard.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_HELLYARD_H_
#define __QUEST_HELLYARD_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitHellyard(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeHellyard(
	const QUEST_FUNCTION_PARAM &tParam);

#endif