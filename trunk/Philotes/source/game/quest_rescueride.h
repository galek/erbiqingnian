//----------------------------------------------------------------------------
// FILE: quest_rescueride.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_RESCUERIDE_H_
#define __QUEST_RESCUERIDE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void s_QuestTrainRideOver( QUEST * pQuest );

void QuestInitRescueRide(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeRescueRide(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif