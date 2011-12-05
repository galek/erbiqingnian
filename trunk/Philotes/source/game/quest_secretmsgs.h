//----------------------------------------------------------------------------
// FILE: quest_secretmsgs.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_SECRETMSGS_H_
#define __QUEST_SECRETMSGS_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitSecretMessages(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeSecretMessages(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif