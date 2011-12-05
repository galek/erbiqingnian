//----------------------------------------------------------------------------
// FILE: quest_default.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_DEFAULT_H_
#define __QUEST_DEFAULT_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct QUEST;
enum QUEST_MESSAGE_TYPE;
enum QUEST_MESSAGE_RESULT;

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

QUEST_MESSAGE_RESULT s_QuestsDefaultMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage);

#endif
