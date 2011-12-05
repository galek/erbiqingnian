//----------------------------------------------------------------------------
// FILE: quest_listen.cpp
//
// This is a template quest. The player first listens to Cast Giver (in quest.xls)
// 
// author: Chris March, 12-10-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_listen.h"

#include "quests.h"
#include "s_quests.h"
#include "npc.h"


//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ))
		{
			
			if (s_QuestCanBeActivated( pQuest ))
			{
				// Activate the quest
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nDescriptionDialog );

				return QMR_NPC_TALK;
			}
			else if (QuestIsInactive( pQuest ) && 
					 pQuestDefinition->nUnavailableDialog != INVALID_LINK)
			{
				s_NPCTalkStart(
					pPlayer,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nUnavailableDialog );

				return QMR_NPC_TALK;
			}
		}
	}

#endif
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pListeningTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( s_QuestIsGiver( pQuest, pListeningTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					s_QuestComplete( pQuest );
				}
			}
		}

	}

	return QMR_OK;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	switch (eMessageType)
	{
		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pListenStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pListenStoppedMessage );
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestListenFree(
	const QUEST_FUNCTION_PARAM &tParam)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestListenInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	
	// sanity check quest excel data
	ASSERTX_RETURN( pQuest->pQuestDef, "expected quest definition for listen template quest" );
}
