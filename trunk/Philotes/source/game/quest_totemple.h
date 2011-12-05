//----------------------------------------------------------------------------
// FILE: quest_totemple.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TOTEMPLE_H_
#define __QUEST_TOTEMPLE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitToTemple(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeToTemple(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif