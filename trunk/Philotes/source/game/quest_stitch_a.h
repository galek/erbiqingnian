//----------------------------------------------------------------------------
// FILE: quest_stitch_a.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_STITCH_A_H_
#define __QUEST_STITCH_A_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;
struct QUEST_MESSAGE_ENTER_ROOM;
struct QUEST_MESSAGE_PARTY_KILL;
struct QUEST_MESSAGE_PICKUP;

enum QUEST_MESSAGE_RESULT;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitStitchA(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeStitchA(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestOnEnterGameStitchA(
	const QUEST_FUNCTION_PARAM &tParam);

QUEST_MESSAGE_RESULT s_QuestStitchMessagePickup( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage);

#endif