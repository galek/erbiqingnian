//----------------------------------------------------------------------------
// FILE: quest_listen.h
//
// This is a template quest. The player first listens to Cast Giver (in quest.xls)
// 
// author: Chris March, 12-10-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_LISTEN_H_
#define __QUEST_LISTEN_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestListenInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestListenFree(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif