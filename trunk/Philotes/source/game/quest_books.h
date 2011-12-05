//----------------------------------------------------------------------------
// FILE: quest_books.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_BOOKS_H_
#define __QUEST_BOOKS_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct GAME;
struct QUEST;
struct QUEST_FUNCTION_PARAM;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void QuestInitBooks(
	const QUEST_FUNCTION_PARAM &tParam);

void QuestFreeBooks(
	const QUEST_FUNCTION_PARAM &tParam);
	
#endif