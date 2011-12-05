//----------------------------------------------------------------------------
// FILE: quest_triage.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TRIAGE_H_
#define __QUEST_TRIAGE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void s_QuestTriageBegin( QUEST * pQuest );

void QuestInitTriage(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTriage(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif