//----------------------------------------------------------------------------
// FILE: quest_portalboost.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_PORTALBOOST_H_
#define __QUEST_PORTALBOOST_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitPortalBoost(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreePortalBoost(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif