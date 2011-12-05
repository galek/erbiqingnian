//----------------------------------------------------------------------------
// FILE: quest_stitch_c.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_STITCH_C_H_
#define __QUEST_STITCH_C_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitStitchC(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeStitchC(
	const QUEST_FUNCTION_PARAM &tParam);
	
void s_QuestActivateStitchC(
	UNIT *pPlayer);

#endif