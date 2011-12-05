//----------------------------------------------------------------------------
// FILE: quest_wisdomchaos.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_WISDOMCHAOS_H_
#define __QUEST_WISDOMCHAOS_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitWisdomChaos(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeWisdomChaos(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif