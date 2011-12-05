//----------------------------------------------------------------------------
// FILE: quest_wormhunt.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_WORMHUNT_H_
#define __QUEST_WORMHUNT_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitWormHunt(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeWormHunt(
	const QUEST_FUNCTION_PARAM &tParam);

		
#endif