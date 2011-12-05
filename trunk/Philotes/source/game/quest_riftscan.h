//----------------------------------------------------------------------------
// FILE: quest_riftscan.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_RIFTSCAN_H_
#define __QUEST_RIFTSCAN_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitRiftScan(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeRiftScan(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif