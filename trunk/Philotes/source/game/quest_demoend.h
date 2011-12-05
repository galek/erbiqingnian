//----------------------------------------------------------------------------
// FILE: quest_demoend.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DEMOEND_H_
#define __QUEST_DEMOEND_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitDemoEnd(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeDemoEnd(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestDemoEndForceComplete( UNIT * pPlayer );

#endif