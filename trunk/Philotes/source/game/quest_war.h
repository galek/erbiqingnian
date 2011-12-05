//----------------------------------------------------------------------------
// FILE: quest_war.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_WAR_H_
#define __QUEST_WAR_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitWar(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeWar(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif
