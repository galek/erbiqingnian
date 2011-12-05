//----------------------------------------------------------------------------
// FILE: quest_talk.h
//
// This is a template quest. The player first talks to "Cast Giver" (in quest.xls),
// who gives them quest items from "Starting Items Treasure Class" to use 
// (right-click) on "Objective Monster" or "Objective Object". The player must
// use the item within "MonitorApproachRadius" of the monster or object, unless
// it was not specified. When they use the item, the "Mostly Complete Function" 
// is called, if specified, to do cool stuff.
// Finally, the player must go speak with Cast Rewarder. 
// 
// author: Chris March, 3-7-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_USEITEM_H_
#define __QUEST_USEITEM_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitUseItem(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeUseItem(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif