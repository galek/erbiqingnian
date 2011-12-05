//----------------------------------------------------------------------------
// FILE: quest_thesigil.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_THESIGIL_H_
#define __QUEST_THESIGIL_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTheSigil(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTheSigil(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif