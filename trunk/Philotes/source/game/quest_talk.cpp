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
#include "quest_talk.h"

#include "quests.h"
#include "s_quests.h"
#include "npc.h"


//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

static void sAttemptToSpawnMissingRewarder( QUEST * pQuest, LEVEL *pLevel )
{
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( !pQuestDef->bDisableSpawning &&
		( pQuest->pQuestDef->nLevelDefDestinations[0] == INVALID_LINK ||
		  pQuest->pQuestDef->nLevelDefDestinations[0] == LevelGetDefinitionIndex( pLevel ) ) &&
		  pQuestDef->nCastRewarder != INVALID_LINK )
	{
		SPECIES speciesRewarder = QuestCastGetSpecies( pQuest, pQuestDef->nCastRewarder );
		ASSERTV_RETURN( speciesRewarder != SPECIES_NONE, "quest \"%s\" has an invalid Cast Rewarder, couldn't determine species", pQuest->pQuestDef->szName );
		if ( !s_QuestCastMemberGetInstanceBySpecies( pQuest, speciesRewarder) )
		{
			int classRewarder = GET_SPECIES_CLASS( speciesRewarder ) ;
			const UNIT_DATA * pMonsterData = 
				UnitGetData( UnitGetGame( QuestGetPlayer( pQuest ) ), GENUS_MONSTER, classRewarder);
			ASSERTV_RETURN( pMonsterData != NULL, "%s quest: Cast Rewarder is in the wrong version_package", pQuestDef->szName );
			int nMonsterQuality = pMonsterData->nMonsterQualityRequired;

			s_LevelSpawnMonsters( 
				QuestGetGame( pQuest ), 
				pLevel, 
				classRewarder, 
				1,
				nMonsterQuality );
		}
	}
}

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
		if (s_QuestIsGiver( pQuest, pSubject ) &&
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
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TALKTEMPLATE_TALK ) == QUEST_STATE_ACTIVE )
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
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
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
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
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					if ( pQuestDefinition->nLevelDefDestinations[0] != INVALID_LINK &&
						 UnitGetLevelDefinitionIndex( QuestGetPlayer( pQuest ) ) != pQuestDefinition->nLevelDefDestinations[0] )
					{
						QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL, QUEST_STATE_ACTIVE );
					}
					else
					{
						QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TALK, QUEST_STATE_ACTIVE );
						sAttemptToSpawnMissingRewarder( pQuest, UnitGetLevel( QuestGetPlayer( pQuest ) ) );
					}
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TALKTEMPLATE_TALK ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{	
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TALK, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{								
					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}					
			}
		}
	}

	return QMR_OK;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if ( pQuest->pQuestDef->nLevelDefDestinations[0] != INVALID_LINK )
	{
		if ( pQuest->pQuestDef->nLevelDefDestinations[0] == pMessageEnterLevel->nLevelNewDef )
		{		
			// arrived at the level destination
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL ) == 
				 QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TALK, QUEST_STATE_ACTIVE );
				sAttemptToSpawnMissingRewarder( pQuest, UnitGetLevel( QuestGetPlayer( pQuest ) ) );
			}
		}
		else
		{
			// arrived at a level that is not the destination
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TALKTEMPLATE_TALK ) == 
				 QUEST_STATE_ACTIVE )
			{
				// reset to hidden to hack around the "complete can only be set to hidden, not active" limitation
				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL, QUEST_STATE_HIDDEN );			
				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TALK, QUEST_STATE_HIDDEN );

				QuestStateSet( pQuest, QUEST_STATE_TALKTEMPLATE_TRAVEL, QUEST_STATE_ACTIVE );			
			}
		}
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
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
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestTalkFree(
	const QUEST_FUNCTION_PARAM &tParam)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestTalkInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	
	// sanity check quest excel data
	ASSERTX_RETURN( pQuest->pQuestDef, "expected quest definition for talk template quest" );
	ASSERTV_RETURN( pQuest->pQuestDef->nLevelDefDestinations[0] != INVALID_LINK, "quest \"%s\" \"Level Destinations\" is blank or has an invalid level", pQuest->pQuestDef->szName );
}
