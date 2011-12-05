//----------------------------------------------------------------------------
// FILE: quest_mind314.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_MIND314_H
#define __QUEST_MIND314_H

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitMind314(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeMind314(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestMind314Version(
	const QUEST_FUNCTION_PARAM &tParam);
		
#endif