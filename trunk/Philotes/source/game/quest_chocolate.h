//----------------------------------------------------------------------------
// FILE: quest_chocolate.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_CHOCOLATE_H
#define __QUEST_CHOCOLATE_H

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitChocolate(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeChocolate(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif