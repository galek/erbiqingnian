//----------------------------------------------------------------------------
// FILE: quest_stitch_b.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_STITCH_B_H_
#define __QUEST_STITCH_B_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitStitchB(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeStitchB(
	const QUEST_FUNCTION_PARAM &tParam);
	
void s_QuestOnEnterGameStitchB(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestActivateStitchB(
	UNIT *pPlayer);

#endif