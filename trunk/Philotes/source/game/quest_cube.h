//----------------------------------------------------------------------------
// FILE: quest_cube.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_CUBE_H_
#define __QUEST_CUBE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitCube(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeCube(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif
