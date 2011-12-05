//----------------------------------------------------------------------------
// FILE: quest_infestation.cpp
//
// The infestation quest is a template quest, in which the player must kill 
// monsters from a specified class. You can also specify a level to find the 
// monsters in.
//
// Code is adapted from the extermination task, in tasks.cpp.
// 
// author: Chris March, 1-31-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_infestation.h"
#include "quests.h"
#include "s_quests.h"
#include "npc.h"
#include "quest_template.h"
#include "monsters.h"

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// s_sQuestAttemptToSpawnMissingObjectiveMonsters
// Attempts to spawn monsters of QUEST_DEFINITION::nObjectiveMonster class,
// until there is an amount in the level that meets requirements set by
// the QUEST_DEFINITION data. 
// Returns the number of monsters spawned.
//----------------------------------------------------------------------------
static int s_sQuestAttemptToSpawnMissingObjectiveMonsters(
	QUEST *pQuest,
	LEVEL *pLevel )
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = UnitGetGame( pPlayer);

	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	if ( pQuestDef->bDisableSpawning )
		return 0;

	if ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		 pQuestDef->nLevelDefDestinations[0] == LevelGetDefinitionIndex( pLevel ) )
	{

		int nKills = UnitGetStat( pPlayer, STATS_SAVE_QUEST_INFESTATION_KILLS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) );
		// need to kill more? make sure there are enough in the level
		int nDesiredMonstersInLevel =
			( pQuestDef->nSpawnCount > 0 ) ?
				pQuestDef->nSpawnCount - nKills : 
				pQuestDef->nObjectiveCount - nKills ;
		if ( nDesiredMonstersInLevel > 0 )
		{
			int nMonstersInLevel = 
				LevelCountMonsters( pLevel, pQuestDef->nObjectiveMonster, pQuestDef->nObjectiveUnitType );
			if ( nMonstersInLevel < nDesiredMonstersInLevel)
			{
				int nMonstersToSpawn = nDesiredMonstersInLevel - nMonstersInLevel; 
				int nMonsterQuality = INVALID_LINK;
				// the objective monster may be invalid, in which case killing
				// any kind of monster counts
				if ( pQuestDef->nObjectiveMonster != INVALID_LINK )
				{		
					const UNIT_DATA * pMonsterData = 
						UnitGetData(pGame, GENUS_MONSTER, pQuestDef->nObjectiveMonster);
					ASSERTV_RETZERO( pMonsterData != NULL, "%s quest: NULL UNIT_DATA for Objective Monster", pQuestDef->szName );
					nMonsterQuality = pMonsterData->nMonsterQualityRequired;
				}

				return s_LevelSpawnMonsters( 
					pGame, 
					pLevel, 
					pQuestDef->nObjectiveMonster, 
					nMonstersToSpawn,
					nMonsterQuality);
			}
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pEnterLevelMessage)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	if ( QuestStateGetValue( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL ) == 
		 QUEST_STATE_ACTIVE )
		s_sQuestAttemptToSpawnMissingObjectiveMonsters(
			pQuest,
			UnitGetLevel( pPlayer ) );
	return QMR_OK;
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
		if ( QuestStateGetValue( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL ) == QUEST_STATE_COMPLETE )
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
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL, QUEST_STATE_ACTIVE );
					QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL_COUNT, QUEST_STATE_ACTIVE );

					s_sQuestAttemptToSpawnMissingObjectiveMonsters(
						pQuest,
						UnitGetLevel( pPlayer ) );
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{	
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_RETURN, QUEST_STATE_COMPLETE );

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
static QUEST_MESSAGE_RESULT sQuestMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage)
{
	UNIT *pVictim = pMessage->pVictim;
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	// is this victim a target of the quest
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;

	// must match the unit type if specified, else match the monster 
	// class (but not if checking unit type, since multiple monster 
	// class hierarchies can match same unit type)
	if ( QuestStateGetValue( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL ) == QUEST_STATE_ACTIVE &&
		 ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == 
				UnitGetLevelDefinitionIndex( pVictim ) ) )
	{
		BOOL bMonsterMatches = FALSE;
		// must match the unit type if specified, else match the monster 
		// class (but not if checking unit type, since multiple monster 
		// class hierarchies can match same unit type)
		if ( pQuestDef->nObjectiveUnitType != INVALID_LINK )
		{
			if ( UnitIsA( pVictim, pQuestDef->nObjectiveUnitType ) )
				bMonsterMatches = TRUE;
		}
		else if ( UnitIsA( pVictim, UNITTYPE_MONSTER ) &&			// no destructibles
				  ( pQuestDef->nObjectiveMonster == INVALID_LINK ||
					UnitIsInHierarchyOf( 
						GENUS_MONSTER, 
						UnitGetClass( pVictim ),
						&pQuestDef->nObjectiveMonster,
						1 ) ) )
		{
			bMonsterMatches = TRUE;
		}

		if (bMonsterMatches)
		{
			// TODO cmarch: check if party member who killed monster is nearby?
			int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
			int nKills = UnitGetStat( pPlayer, STATS_SAVE_QUEST_INFESTATION_KILLS, pQuest->nQuest, nDifficulty );
			++nKills;
			UnitSetStat( pPlayer, STATS_SAVE_QUEST_INFESTATION_KILLS, pQuest->nQuest, nDifficulty, nKills );

			if ( nKills == 1 )
				QuestAutoTrack( pQuest );

			if (nKills >= pQuestDef->nObjectiveCount)
			{
				// done killing
				QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_KILL_COUNT, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_INFESTATIONTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
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
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			return sQuestMessagePartyKill( pQuest, pPartyKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL *pEnterRoomMessage = (const QUEST_MESSAGE_ENTER_LEVEL *)pMessage;
			return s_sQuestMessageEnterLevel( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pEnterRoomMessage);
		}		

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				UnitClearStat( pPlayer, STATS_SAVE_QUEST_INFESTATION_KILLS, StatParam( STATS_SAVE_QUEST_INFESTATION_KILLS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) );
			}
			// fall through

		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return QuestTemplateMessageStatus( pQuest, pMessageStatus );
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInfestationFree(
	const QUEST_FUNCTION_PARAM &tParam)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInfestationInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );

	// get credit for monster kills by party members
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

	ASSERTV_RETURN(pQuest->pQuestDef->nObjectiveMonster != INVALID_LINK, "quest \"%s\": Objective Monster in quest.xls is blank or not found in monsters.xls", pQuest->pQuestDef->szName);
}
