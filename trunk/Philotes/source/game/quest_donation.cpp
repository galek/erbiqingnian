//----------------------------------------------------------------------------
// FILE: quest_talk.cpp
//
// This is a template quest. The player first talks to Cast Giver (in quest.xls),
// and then must speak with Cast Rewarder. Optionally, the quest giver gives 
// items to the player to take to the rewarder.
// 
// author: Chris March, 3-6-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_donation.h"

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
		if (UnitIsA(pSubject, UNITTYPE_DONATOR) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// Activate the quest
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nDescriptionDialog );

			return QMR_NPC_TALK;
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_DONATE_DONATE ) == QUEST_STATE_ACTIVE )
		{
			if ( UnitIsA(pSubject, UNITTYPE_DONATOR) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nCompleteDialog,
					pQuestDefinition->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( UnitIsA(pSubject, UNITTYPE_DONATOR) )
			{
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDefinition->nIncompleteDialog );

				return QMR_NPC_TALK;
			}
		}
	}
#endif
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
		/*
	case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
	case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return s_sQuestMessageEnterLevel( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessageEnterLevel );
		}
		*/
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestDonateFree(
				   const QUEST_FUNCTION_PARAM &tParam)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestDonateInit(
				   const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );

	// sanity check quest excel data
	ASSERTX_RETURN( pQuest->pQuestDef, "expected quest definition for talk template quest" );
	//ASSERTV_RETURN( pQuest->pQuestDef->nLevelDefDestinations[0] != INVALID_LINK, "quest \"%s\" \"Level Destinations\" is blank or has an invalid level", pQuest->pQuestDef->szName );
}
