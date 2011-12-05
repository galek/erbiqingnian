//----------------------------------------------------------------------------
// FILE: quest_operate.h
//
// The operate quest is a template quest, in which the player must interact
// with a number of objects of a specific class. A level can also be specified.
// Optionally, the objects can be spawned by the quest.
// 
// author: Chris March, 2-8-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_OPERATE_H_
#define __QUEST_OPERATE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestOperateInit(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestOperateFree(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestOperateComplete(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif