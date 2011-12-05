//----------------------------------------------------------------------------
// FILE: quest_death.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DEATH_H_
#define __QUEST_DEATH_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitDeath(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeDeath(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif