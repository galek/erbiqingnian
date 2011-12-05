//----------------------------------------------------------------------------
// FILE: quest_testhope.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TESTHOPE_H_
#define __QUEST_TESTHOPE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitTestHope(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeTestHope(
	const QUEST_FUNCTION_PARAM &tParam);

void s_QuestToHBegin( QUEST * pQuest );
void s_QuestToHCheck( QUEST * pQuest );

BOOL QuestIsMurmurDead( UNIT * pPlayer );

#endif