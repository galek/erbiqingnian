//----------------------------------------------------------------------------
// FILE: s_questsave.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quests.h"
#include "s_quests.h"
#include "s_questsave.h"
#include "statspriv.h"
#include "statssave.h"
#include "quest_template.h"
#include "offer.h"

//----------------------------------------------------------------------------
// LOCAL
//----------------------------------------------------------------------------
#define MAX_SAVE_STATS			20		// max number of stats to save quest system state in

//----------------------------------------------------------------------------
// The following is the version tracking for the quest save data system itself
//----------------------------------------------------------------------------
#define QUEST_STAT_SYSTEM_SAVE_VERSION		(15 + GLOBAL_FILE_VERSION)

//----------------------------------------------------------------------------
// The following is the version tracking for all quest DATA, if you
// need to version a particular quest, increase this number and modify 
// an appropriate version function
//----------------------------------------------------------------------------
#define	QUEST_SAVE_DATA_VERSION_CURRENT		1
#define	QUEST_SAVE_DATA_VERSION_INVALID		0

//----------------------------------------------------------------------------
enum QUEST_DATA_VERSION
{
	QDV_CURRENT_VERSION = 1
};

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

int QuestDataGetCurrentVersion(
	void)
{
	return QDV_CURRENT_VERSION;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestVersionData(
	QUEST *pQuest)
{
	ASSERTV_RETFALSE( pQuest, "Expected quest" );
	
	// you cannot version a quest that is already being versioned, that will likely
	// cause infinite recursion, you have to find another way to do it, like version passes or something
	ASSERTX_RETFALSE( TESTBIT( pQuest->dwQuestFlags, QF_VERSIONING_BIT ) == FALSE, "Quest already being versioned" );

	// set bit that we're in the process of updating this quest
	SETBIT( pQuest->dwQuestFlags, QF_VERSIONING_BIT );
		
	// get version
	int nVersion = QuestGetVersion( pQuest );
	int nCurrentVersion = QuestGetCurrentVersion( pQuest );
	if (nVersion != nCurrentVersion)
	{
	
		// run custom version function if we have one
		const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
		if (pQuestDef->pfnVersionFunction)
		{
		
			// setup param
			QUEST_FUNCTION_PARAM tParam;
			tParam.pQuest = pQuest;
			tParam.pPlayer = QuestGetPlayer( pQuest );
						
			// run function
			pQuestDef->pfnVersionFunction( tParam );
			
		}

		// quest is now current version
		pQuest->nVersion = nCurrentVersion;

	}
	
	// OK we're done
	CLEARBIT( pQuest->dwQuestFlags, QF_VERSIONING_BIT );
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestBackupToLastRestorePoint(
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERTX_RETURN( pQuestDef, "Expected quest definition" );

	// only consider active quests
	if (QuestIsActive( pQuest ))
	{
	
		// go through all states, starting at the end and backing up to the beginning
		for (int i = pQuest->nNumStates - 1; i >= 0; --i)
		{
			QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
			QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
			
			// can this state value be considered for a restore point?
			if (eStateValue == QUEST_STATE_ACTIVE || eStateValue == QUEST_STATE_COMPLETE)
			{
				const QUEST_STATE_DEFINITION *pQuestStateDef = QuestStateGetDefinition( pState->nQuestState );
				if (pQuestStateDef->bRestorePoint == TRUE)
				{
					// yep, restore quest to this state
					s_QuestRestoreToState( pQuest, pState->nQuestState, QUEST_STATE_ACTIVE );
					return;
				}
				
			}
			
		}		
		
		// didn't find any restore points, deactivate the quest, and activate anew
		s_QuestReactivate( pQuest );
		
	}
		
}

//----------------------------------------------------------------------------
// optimization to decrease save file size
//----------------------------------------------------------------------------
static BOOL sQuestStatusIsSaved(
	QUEST *pQuest)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	
	switch (eStatus)
	{

		//----------------------------------------------------------------------------	
		case QUEST_STATUS_ACTIVE:
		case QUEST_STATUS_COMPLETE:
		case QUEST_STATUS_OFFERING:
		case QUEST_STATUS_CLOSED:
		{
			return TRUE;
		}

		//----------------------------------------------------------------------------		
		case QUEST_STATUS_FAIL:
		case QUEST_STATUS_INVALID:
		case QUEST_STATUS_INACTIVE:
		default:
		{
			return FALSE;
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearQuestSaveSystemData( 
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	int nQuest = pQuest->nQuest;
	int nDifficulty = QuestGetDifficulty( pQuest );
	
	// clear quest status stat
	PARAM dwParam = StatParam( STATS_SAVE_QUEST_STATUS, nQuest, nDifficulty );
	UnitClearStat( pPlayer, STATS_SAVE_QUEST_STATUS, dwParam );
		
	// clear all quest state data
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
	
		// get the stat for this state
		int nStateStat = STATS_SAVE_QUEST_STATE_1 + i;		
		ASSERTX_CONTINUE( nStateStat >= STATS_SAVE_QUEST_STATE_1 && nStateStat <= STATS_SAVE_QUEST_STATE_LAST, "Not enough stats to hold quest save states" );
		
		// get the state
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		ASSERTX_CONTINUE( pState, "Expected quest state" );
		
		// setup param and clear
		PARAM dwParam = StatParam( nStateStat, nQuest, pState->nQuestState, nDifficulty );
		UnitClearStat( pPlayer, nStateStat, dwParam );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sClearClosedQuestPrereqSaveData(
	QUEST *pQuest,
	void *pCallbackData)
{

	// if this quest is closed, we can clear all the save data from 
	// all of the prereqs
	if (QuestIsClosed( pQuest ))
	{
	
		const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
		if (pQuestDef)
		{
		
			// get difficulty of the quest in question, we only want to
			// find prereqs at the same difficulty level
			int nDifficulty = QuestGetDifficulty( pQuest );
			UNIT *pPlayer = QuestGetPlayer( pQuest );
			
			// look for prereqs
			for (int i = 0; i < MAX_QUEST_PREREQS; ++i)
			{
				int nQuestPrereq = pQuestDef->nQuestPrereqs[ i ];
				if (nQuestPrereq != INVALID_LINK)
				{
					QUEST *pQuestPrereq = QuestGetQuestByDifficulty( pPlayer, nQuestPrereq, nDifficulty );
					ASSERTX_CONTINUE( pQuestPrereq, "Expected quest" );
					sClearQuestSaveSystemData( pQuestPrereq );
				}
				
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestsWriteToStats(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );

	if (AppIsHellgate() == FALSE)
	{
		return TRUE;
	}
	
	// write quest system save version
	UnitSetStat( pPlayer, STATS_SAVE_QUEST_VERSION, QUEST_STAT_SYSTEM_SAVE_VERSION );

	// write the quest data save version
	UnitSetStat( pPlayer, STATS_SAVE_QUEST_DATA_VERSION, QUEST_SAVE_DATA_VERSION_CURRENT );

	// go through all difficultites (we should make an iterator for this)		
	for (int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
	{
	
		// go through all quests	
		for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
			if (pQuest == NULL)
			{
				continue;
			}
			
			// quest should be the most current version
			int nQuestDataCurrentVersion = QuestDataGetCurrentVersion();
			ASSERTV_RETFALSE( 
				pQuest->nVersion == nQuestDataCurrentVersion, 
				"Quest '%s' version is not current '%d' (should be '%d')",
				pQuest->pQuestDef->szName,
				pQuest->nVersion,
				nQuestDataCurrentVersion);

			// there was a time where some quests were not properly marked as close on complete,
			// if we find any quests that are complete that should be closed, we will do it now
			if (QuestIsComplete( pQuest, FALSE ))
			{
				const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
				if (pQuestDef)
				{
					if (pQuestDef->bCloseOnComplete == TRUE)
					{
						s_QuestClose( pQuest );
					}
				}		
			}
								
			// write the status
			if (sQuestStatusIsSaved( pQuest ))
			{
				QUEST_STATUS eStatus = QuestGetStatus( pQuest );
				UnitSetStat( pPlayer, STATS_SAVE_QUEST_STATUS, nQuest, nDifficulty, eStatus );
			}
			else
			{
				PARAM dwParam = StatParam( STATS_SAVE_QUEST_STATUS, nQuest, nDifficulty );
				UnitClearStat( pPlayer, STATS_SAVE_QUEST_STATUS, dwParam );
			}

			// go through all states for this quest
			for (int j = 0; j < pQuest->nNumStates; ++j)
			{
				int nStateStat = STATS_SAVE_QUEST_STATE_1 + j;
				ASSERTX_RETFALSE( nStateStat >= STATS_SAVE_QUEST_STATE_1 && nStateStat <= STATS_SAVE_QUEST_STATE_LAST, "Not enough stats to hold quest save states" );
				QUEST_STATE *pState = QuestStateGetByIndex( pQuest, j );
				QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
		
				// if quest doiesn't matter, we will erase this stat
				QUEST_STATUS eStatus = QuestGetStatus( pQuest );
				if (s_QuestStatusAllowsPlayerInteraction( eStatus ) == FALSE)
				{
					PARAM dwParam = StatParam( nStateStat, nQuest, pState->nQuestState, nDifficulty );
					UnitClearStat( pPlayer, nStateStat, dwParam );
				}
				else
				{
					UnitSetStat( pPlayer, nStateStat, nQuest, pState->nQuestState, nDifficulty, eStateValue );
				}
				
			}				

			// try to clean up the quest stats before writing
			s_QuestCleanupForSave( pQuest );
			
		}
		
	}

	// as an optimization, if a quest is closed, we do not need to save any quest
	// save system information about any of that quest's prereqs because on load we can
	// simply mark all the prereqs as closed as well.
	DWORD dwQuestIterateFlags = 0;
	SETBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES );
	QuestIterateQuests( pPlayer, sClearClosedQuestPrereqSaveData, NULL, dwQuestIterateFlags );
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sQuestUpdatesPartyKillOnSetup(
	QUEST *pQuest)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERTX_RETFALSE( pQuestDef, "Expected quest definition" );
	
	// check for quests that explicitly allow it
	if (pQuestDef->bUpdatePartyKillOnSetup == TRUE)
	{
		return TRUE;
	}
	
	// check for template quests
	const QUEST_TEMPLATE_DEFINITION *pQuestTemplateDef = QuestTemplateGetDefinition( pQuest );
	if (pQuestTemplateDef)
	{
		if (pQuestTemplateDef->bUpdatePartyKillOnSetup == TRUE)
		{
			return TRUE;
		}
	}

	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestClosePrereqs(
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( QuestIsClosed( pQuest ), "Quest should be closed" );
			
	// get def
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	if (pQuestDef)
	{
	
		// get difficulty for this quest
		int nDifficulty = QuestGetDifficulty( pQuest );
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		ASSERTX_RETURN( pPlayer, "Expected quest unit" );
		
		// search for prereqs
		for (int i = 0; i < MAX_QUEST_PREREQS; ++i)
		{
			int nQuestPrereq = pQuestDef->nQuestPrereqs[ i ];
			if (nQuestPrereq != INVALID_LINK)
			{
				QUEST *pQuestPrereq = QuestGetQuestByDifficulty( pPlayer, nQuestPrereq, nDifficulty, FALSE );
				ASSERTX_CONTINUE( pQuestPrereq, "Expected quest" );
								
				// close it
				s_QuestClose( pQuestPrereq );

				// close its prereqs
				sQuestClosePrereqs( pQuestPrereq );
				
			}
			
		}

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClosedQuestClosePrereqs(
	QUEST *pQuest,
	void *pCallbackData)
{

	// if this quest is closed, close it's prereqs
	if (QuestIsClosed( pQuest ))
	{
		sQuestClosePrereqs( pQuest );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestLoadFromStats( 
	UNIT *pPlayer,
	BOOL bFirstGameJoin)
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETFALSE(pQuestGlobals);
	
	// read quest system save version
	int nSystemVersion = UnitGetStat( pPlayer, STATS_SAVE_QUEST_VERSION );
	REF( nSystemVersion );  // this the version of the quest save system as a whole, not each individual quest

	// read quest data version
	int nDataVersion = UnitGetStat( pPlayer, STATS_SAVE_QUEST_DATA_VERSION );
	BOOL bCancelOffers = FALSE;

	// loop through all quests and restore status
	for( int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
	{
		for( int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			// get quest (do not version since we're loading it now)
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty, FALSE );
			if (!pQuest)
			{
				continue;
			}
			
			// set the data version for this quest
			pQuest->nVersion = nDataVersion;
			
			// read status
			QUEST_STATUS eStatus = (QUEST_STATUS)UnitGetStat( pPlayer, STATS_SAVE_QUEST_STATUS, nQuest, nDifficulty );
			QUEST_STATUS eOldStatus = pQuest->eStatus;
			if (eStatus == QUEST_STATUS_INVALID)
			{
				pQuest->eStatus = QUEST_STATUS_INACTIVE;
			}
			else if (eStatus == QUEST_STATUS_OFFERING)
			{
				// an offer dialog may have exited abnormally, clean up
				bCancelOffers = TRUE;
				pQuest->eStatus = QUEST_STATUS_ACTIVE;
			}
			else
			{
				pQuest->eStatus = eStatus;
			}

			if (pQuest->eStatus != eOldStatus)
			{
				// send the status changed event
				s_QuestEventStatus(pQuest, eOldStatus);
			}
			
			// attempt to cleanup unnecessary quest data on load
			s_QuestCleanupForSave( pQuest );
			
		}
		
	}

	if (bCancelOffers)
		s_OfferCancel( pPlayer );

	// for all quests that are closed, also mark all their prereqs as closed, note we
	// do not version the quest here because we're still constructing them
	DWORD dwQuestIterateFlags = 0;
	SETBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES );
	QuestIterateQuests( pPlayer, sClosedQuestClosePrereqs, NULL, dwQuestIterateFlags, FALSE );

	// read all states values
	int	nNumSaveStats = STATS_SAVE_QUEST_STATE_LAST - STATS_SAVE_QUEST_STATE_1;
	for (int i = 0; i < nNumSaveStats; ++i)
	{
		int nStat = STATS_SAVE_QUEST_STATE_1 + i;
		const STATS_DATA *pStatsData = StatsGetData( NULL, nStat );
		
		// this this state value for all quests
		STATS_ENTRY tStatsQuestStateValues[ NUM_QUESTS ];
		int nStatCount = UnitGetStatsRange( pPlayer, nStat, 0, tStatsQuestStateValues, NUM_QUESTS );
		
		// go through all entries here
		for (int j = 0; j < nStatCount; ++j)
		{
			const STATS_ENTRY *pEntry = &tStatsQuestStateValues[ j ];
			QUEST_STATE_VALUE eStateValue = (QUEST_STATE_VALUE)pEntry->value;
			PARAM dwParam = pEntry->GetParam();
			int nQuest = StatGetParam( pStatsData, dwParam, 0 );
			int nQuestState = StatGetParam( pStatsData, dwParam, 1 );
			int nDifficulty = StatGetParam( pStatsData, dwParam, 2 );
			
			// get this quest (do not version since we're not fully loaded)
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty, FALSE );
			if (pQuest)
			{
				
				// get state and set value
				QUEST_STATE *pState = QuestStateGetPointer( pQuest, nQuestState );
				ASSERT_CONTINUE( pState );
				pState->eValue = eStateValue;
				
			}
			
		}

		// now that all the data is loaded into in game constructs, clear stat information
		UnitClearStatsRange( pPlayer, nStat );
		
	}
	
	// the first time we're joining a game, we need to version all the quests and possibly
	// roll them back to a safe restore point.  At present, we won't do this when this
	// player is just switching game instances, we'll just leave the states as is
	// and assume that each quest state is as valid as we can make it.  
	// If the instance hopping player gets their quests into a state that can't 
	// be completed because this player was on another server for some amount of time, 
	// at least the next time they play the game again their quests will rollback 
	// to a valid state ... there are lots of decisions when finding something that
	// will make this better, we could do intergame communication to tell the player
	// they got credit, or that they can't complete the quest for instance
	if (bFirstGameJoin == TRUE)
	{
		for(int nDifficulty = DIFFICULTY_NORMAL; nDifficulty < NUM_DIFFICULTIES; nDifficulty++)
		{

	
			// version quests, this will automatically happen by just fetching them once
			// after we have loaded all the state values from the stats from above
			for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
			{
				QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
			}
			
			// now that all quests are ready, restore them as necessary
			for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
			{
				QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty, FALSE );		
				if (pQuest)
				{
				
					// back up this quest to the last restore point that is possible
					s_sQuestBackupToLastRestorePoint( pQuest );
										
				}
								
			}
				
		}

		// deactivate any quests that have been instructed to be removed on join game
		for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
			if (pQuest)
			{
				if (QuestRemoveOnJoinGame( pQuest ) == TRUE)
				{
					s_QuestDeactivate( pQuest );
				}
				
			}	
				
		}

	}

	// because we don't save the specific value of each state for completed quests, we
	// will automatically set each state value to completed if the quest status is completed,
	// this allows us to check the completion of any given quest state without having
	// to look at the quest status each time
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{	
		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
		if (pQuest)
		{
			if (QuestIsComplete( pQuest ))
			{
				for (int i = 0; i < pQuest->nNumStates; ++i)
				{
					QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
					QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
					if (eStateValue != QUEST_STATE_COMPLETE)
					{
						QuestStateSet( pQuest, pState->nQuestState, QUEST_STATE_COMPLETE, FALSE, TRUE, FALSE );
					}
				}
			}
		}		
	}

	// do on enter game logic
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
		if (pQuest)
		{
			const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
			ASSERTX_BREAK( pQuestDef, "Expected quest definition" );
			
			// update party kill flags for quests that want it
			if (sQuestUpdatesPartyKillOnSetup( pQuest ))
			{
				QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pQuest->pPlayer, QUEST_STATUS_INVALID );
			}

			// run any on enter game function (if any)
			QUEST_FUNCTION_PARAM tParam;
			tParam.pQuest = pQuest;
			tParam.pPlayer = pPlayer;
			SETBIT( tParam.dwQuestParamFlags, QPF_FIRST_GAME_JOIN_BIT, bFirstGameJoin );
			QuestRunFunction( pQuest, QFUNC_ON_ENTER_GAME, tParam );
						
		}
		
	}
				
	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestRestoreToState( 
	QUEST *pQuest, 
	int nState,
	QUEST_STATE_VALUE nStateValue)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	
	// first hide all quest states
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *ptState = QuestStateGetByIndex( pQuest, i );
		s_QuestStateRestore( pQuest, ptState->nQuestState, QUEST_STATE_HIDDEN );
	}

	// activate all the "on activate" states
	s_QuestDoOnActivateStates( pQuest, TRUE );
	
	// complete all states up to the last one that we want to be active
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *ptState = QuestStateGetByIndex( pQuest, i );
		if (ptState->nQuestState == nState)
		{
			s_QuestStateRestore( pQuest, ptState->nQuestState, nStateValue );
			break;
		}
		else
		{
			s_QuestStateRestore( pQuest, ptState->nQuestState, QUEST_STATE_COMPLETE );
		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatQuestSaveState(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit)
{
	int nStat = tStatsFile.tDescription.nStat;
	
	// quest save state stats
	if (nStat >= STATS_SAVE_QUEST_STATE_1 && nStat < STATS_SAVE_QUEST_STATE_LAST)
	{
		ASSERTX_RETVAL( UnitIsPlayer( pUnit ), SVR_ERROR, "Expected player" );
		
		// quest state stats that were saved using state index instead of param to represent the state	
		if (nStatsVersion < 2)
		{
			int nStateIndex = nStat - STATS_SAVE_QUEST_STATE_1;
			DWORD dwQuestCode = tStatsFile.tParamValue[ 0 ].nValue;
			int nQuest = ExcelGetLineByCode( NULL, DATATABLE_QUEST, dwQuestCode );
			QUEST *pQuest = QuestGetQuest( pUnit, nQuest, FALSE );
			ASSERTX_RETVAL( pQuest, SVR_ERROR, "Expected quest" );
			QUEST_STATE *pState =QuestStateGetByIndex( pQuest, nStateIndex );
			ASSERTX_RETVAL( pState, SVR_ERROR, "Expected quest state" );
			
			// introduction of second param to represent quest state
			DWORD dwQuestStateCode = ExcelGetCode( NULL, DATATABLE_QUEST_STATE, pState->nQuestState );
			tStatsFile.tParamValue[ 1 ].nValue = dwQuestStateCode;
			
			// we are now next version
			nStatsVersion = 2;
			
		}		

	}

	if (nStatsVersion < 3)
	{
		// introduction of third param to represent game difficulty level
		DWORD dwQuestStateCode = ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NORMAL );

		switch (nStat)
		{
		case STATS_TUTORIAL:
		case STATS_QUEST_ITEM:
			tStatsFile.tParamValue[ 0 ].nValue = dwQuestStateCode;
			break;

		case STATS_SAVE_QUEST_HUNT_VERSION:
		case STATS_SAVE_QUEST_HUNT_TARGET_LEVEL_DEFINITION:
		case STATS_SAVE_QUEST_HUNT_TARGET_LEVEL_AREA:
		case STATS_SAVE_QUEST_HUNT_TARGET_COUNT:
		case STATS_SAVE_QUEST_HUNT_TARGET1_NAME:
		case STATS_SAVE_QUEST_HUNT_TARGET1_MONSTER_CLASS:
		case STATS_SAVE_QUEST_HUNT_TARGET1_STATUS:
		case STATS_SAVE_QUEST_HUNT_TARGET2_NAME:
		case STATS_SAVE_QUEST_HUNT_TARGET2_MONSTER_CLASS:
		case STATS_SAVE_QUEST_HUNT_TARGET2_STATUS:
		case STATS_SAVE_QUEST_HUNT_TARGET3_NAME:
		case STATS_SAVE_QUEST_HUNT_TARGET3_MONSTER_CLASS:
		case STATS_SAVE_QUEST_HUNT_TARGET3_STATUS:
		case STATS_SAVE_QUEST_INFESTATION_KILLS:
		case STATS_QUEST_OPERATED_OBJS:
		case STATS_QUEST_OPERATED_OBJ0_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ1_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ2_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ3_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ4_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ5_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ6_UNIT_ID:
		case STATS_QUEST_OPERATED_OBJ7_UNIT_ID:
		case STATS_QUEST_ESCORT_DESTINATION_LEVEL:
		case STATS_SAVE_QUEST_STATUS:
			tStatsFile.tParamValue[ 1 ].nValue = dwQuestStateCode;
			break;

		default:
			tStatsFile.tParamValue[ 2 ].nValue = dwQuestStateCode;
			break;
		}

		// we are now next version
		nStatsVersion = 3;

	}	
	if(nStatsVersion < 5)
	{
		DWORD dwDifficultyCode;
		STATS_PARAM_DESCRIPTION* pParamDesc;
		STATS_FILE_PARAM_VALUE* pValue;
		
		switch(nStat)
		{
		case STATS_QUEST_REWARD:
		case STATS_REMOVE_WHEN_DEACTIVATING_QUEST:
		case STATS_QUEST_STARTING_ITEM:
			//tStatsFile.tParamValue[0].nValue = dwQuestStateCode;
			//tStatsFile.tDescription.nParamCount++;
			//break;
		case STATS_SAVE_QUEST_DATA_VERSION_OBSOLETE:  // NOTE: This stat is now obsolete
		case STATS_QUEST_PLAYER_TRACKING: // This already HAD a 2nd param, but it was unused...
			//tStatsFile.tParamValue[1].nValue = dwQuestStateCode;
			//tStatsFile.tDescription.nParamCount++;

			pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];

			dwDifficultyCode = ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NORMAL );
			//ASSERT(tStatsFile.tDescription.nParamCount == 1);
			pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];
			pValue = &tStatsFile.tParamValue[tStatsFile.tDescription.nParamCount];
			pValue->nValue = dwDifficultyCode;

			//DWORD dwDiffcultyTable = ExcelGetCode( NULL, DATATABLE_QUEST_STATE, pState->nQuestState );
			pParamDesc->idParamTable = DATATABLE_DIFFICULTY;
			tStatsFile.tDescription.nParamCount++;
			nStatsVersion = 5;
			break;

			/*
			pParamDesc = &tStatsFile.tDescription.tParamDescriptions[1];

			dwDifficultyCode = ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NORMAL );
			//ASSERT(tStatsFile.tDescription.nParamCount == 1);
			pParamDesc = &tStatsFile.tDescription.tParamDescriptions[1];
			pValue = &tStatsFile.tParamValue[1];
			pValue->nValue = dwDifficultyCode;

			//DWORD dwDiffcultyTable = ExcelGetCode( NULL, DATATABLE_QUEST_STATE, pState->nQuestState );
			pParamDesc->idParamTable = DATATABLE_DIFFICULTY;
			tStatsFile.tDescription.nParamCount++;
			nStatsVersion = 5;
			break;
			*/
		default:
			break;
		}
		nStatsVersion = 5;

	}
	if(nStatsVersion < 6)
	{
		/*
		DWORD dwDifficultyCode;
		STATS_PARAM_DESCRIPTION* pParamDesc;
		STATS_FILE_PARAM_VALUE* pValue;
		*/

		switch(nStat)
		{
		case STATS_SAVE_QUEST_DATA_VERSION_OBSOLETE:  // NOTE: This state is now obsolete
			tStatsFile.tDescription.nParamCount = 1;
			break;
		default:
			break;
		}
		nStatsVersion = 6;
	}
	
	nStatsVersion = STATS_CURRENT_VERSION;

	return SVR_OK;
		
}