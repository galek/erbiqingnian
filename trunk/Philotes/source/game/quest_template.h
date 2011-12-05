//----------------------------------------------------------------------------
// FILE: quest_template.h
//
// The quest_template.* files contain code used by multiple "template" quests,
// such as quest_hunt and quest_collect.
// 
// author: Chris March, 1-29-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __QUEST_TEMPLATE_H_
#define __QUEST_TEMPLATE_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "quests.h"

//----------------------------------------------------------------------------
// FORWARD REFERENCES	
//----------------------------------------------------------------------------
struct UNIT;
struct QUEST;
struct GAME;
struct LEVEL;
struct QUEST_MESSAGE_INTERACT_INFO;
struct QUEST_MESSAGE_TASK_GIVER_INIT;
struct QUEST_MESSAGE_STATUS;
enum QUEST_MESSAGE_RESULT;
enum QUEST_STATUS;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_TEMPLATE_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];
	DWORD dwCode;
	BOOL bUpdatePartyKillOnSetup;
	BOOL bRemoveOnJoinGame;

	QUEST_FUNCTION_TABLE tFunctions;
			
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

BOOL QuestTemplateExcelPostProcess(
	EXCEL_TABLE *pTable);

const QUEST_TEMPLATE_DEFINITION *QuestTemplateGetDefinition(
	int nQuestTemplate);

const QUEST_TEMPLATE_DEFINITION *QuestTemplateGetDefinition(
	QUEST *pQuest);
	
QUEST_MESSAGE_RESULT QuestTemplateMessageStatus( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATUS * pMessageStatus );

void QuestTemplateUpdatePlayerFlagPartyKill(
	QUEST *pQuest,
	UNIT *pPlayer,
	QUEST_STATUS eStatusOld );

LEVEL* s_QuestTemplatePickDestinationLevel(
	GAME* pGame,
	UNIT* pPlayer,
	QUEST* pQuest) ;

#endif