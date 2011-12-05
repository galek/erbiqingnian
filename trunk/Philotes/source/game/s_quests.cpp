//----------------------------------------------------------------------------
// FILE: s_quests.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers

#include "quests.h"
#include "s_quests.h"
#include "quest.h"  // tugboat
#include "s_QuestServer.h"
#include "npc.h"
#include "clients.h"
#include "states.h"
#include "pets.h"
#include "faction.h"
#include "monsters.h"
#include "treasure.h"
#include "inventory.h"
#include "items.h"
#include "room_layout.h"
#include "offer.h"
#include "objects.h"
#include "gameunits.h"
#include "uix.h"
#include "picker.h"
#include "room_path.h"
#include "stats.h"
#include "s_questsave.h"
#include "Currency.h"
#include "unittag.h"
#include "quest_rescueride.h"
#include "quest_testhope.h"
#include "quest_useitem.h"
#include "s_questgames.h"
#include "warp.h"
#include "achievements.h"
#include "global_themes.h"
#include "unitfileversion.h"

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCastMemberValidate( 
	GAME *pGame,
	CAST_MEMBER *pCastMember,
	LEVEL * pLevel)
{
	ASSERTX_RETURN( pCastMember, "Expected cast member" );

	// if we have a unit it, it must still be around
	if (pCastMember->idUnit != INVALID_ID)
	{
		UNIT * pUnit = UnitGetById( pGame, pCastMember->idUnit );
		if ( pUnit == NULL)
		{
			pCastMember->idUnit = INVALID_ID;
		}
		else if ( UnitGetLevel( pUnit ) != pLevel )
		{
			pCastMember->idUnit = INVALID_ID;
		}
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sQuestCastMemberGetInstance( 
	QUEST *pQuest,
	CAST_MEMBER *pCastMember)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	ASSERTX_RETNULL( pCastMember, "Expected cast member" );
	GAME *pGame = QuestGetGame( pQuest );
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	
	// validate the cast member
	sCastMemberValidate( pGame, pCastMember, pLevel );
	
	// get unit instance if we havne't found it yet
	if (pCastMember->idUnit == INVALID_ID)
	{
		GENUS eGenus = (GENUS)GET_SPECIES_GENUS( pCastMember->spMember );
		int nClass = GET_SPECIES_CLASS( pCastMember->spMember );
		UNIT *pUnit = LevelFindFirstUnitOf( pLevel, NULL, eGenus, nClass );
		if (pUnit)
		{
			pCastMember->idUnit = UnitGetId( pUnit );
		}
	}
	
	if (pCastMember->idUnit != INVALID_ID)
	{
		return UnitGetById( pGame, pCastMember->idUnit );
	}

	return NULL;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_QuestCastMemberGetInstanceBySpecies( 
	QUEST *pQuest,
	SPECIES spMember)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	ASSERTX_RETNULL( spMember != SPECIES_NONE, "Expected expected species" );
	
	// get cast	
	CAST *pCast = &pQuest->tCast;

	// go through cast members	
	for (int i = 0; i < pCast->nNumCastMembers; ++i)
	{
		CAST_MEMBER *pMember = &pCast->tCastMember[ i ];
		if (pMember->spMember == spMember)
		{
			return sQuestCastMemberGetInstance( pQuest, pMember );
		}
	}
	
	ASSERTX_RETNULL( 0, "Species not a cast member of quest" );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_STATE * sQuestFindLastActiveState( QUEST * pQuest )
{
	ASSERT_RETNULL( pQuest );
	// find last active state
	QUEST_STATE * pState = NULL;
	for ( int i = 0; i < pQuest->nNumStates; i++ )
	{
		QUEST_STATE * pCurrent = QuestStateGetByIndex( pQuest, i );
		if ( QuestStateGetValue( pQuest, pCurrent ) == QUEST_STATE_ACTIVE )
			pState = pCurrent;
	}

	// did we find a state and it has a state def?
	if ( !pState )
		return NULL;

	const QUEST_STATE_DEFINITION * pStateDef = QuestStateGetDefinition( pState->nQuestState );
	if ( !pStateDef )
		return NULL;

	return pState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sGossipInfoUpdate( QUEST * pQuest, UNIT * pNPC )
{
	ASSERT_RETVAL( pQuest, QMR_OK );

	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETVAL( pPlayer, QMR_OK );

	// only for active quests
	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// only monster NPCs
	if ( UnitGetGenus( pNPC ) != GENUS_MONSTER )
		return QMR_OK;

	// get deepest active state
	QUEST_STATE * pState = sQuestFindLastActiveState( pQuest );
	if ( !pState )
		return QMR_OK;

	const QUEST_STATE_DEFINITION * pStateDef = QuestStateGetDefinition( pState->nQuestState );
	if ( !pStateDef )
		return QMR_OK;

	// go through all (3) gossips
	int nClass = UnitGetClass( pNPC );
	for ( int i = 0; i < MAX_QUEST_GOSSIPS; i++ )
	{
		// npc column filled out?
		if ( ( pStateDef->nGossipNPC[i] == nClass ) && !pState->bGossipComplete[i] )
		{
			SETBIT( pQuest->dwQuestFlags, QF_GOSSIP );
			return QMR_NPC_STORY_GOSSIP;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDoQuestCastInfoUpdate(
	QUEST *pQuest,
	BOOL bUpdatingAllQuests = FALSE)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );

	if ( AppIsHellgate() )
	{
		// reset the gossip flag, all gossip NPCs are in the cast, so the flag 
		// will be set if one of them has gossip from their info update below
		CLEARBIT( pQuest->dwQuestFlags, QF_GOSSIP );
	}

	CAST *pCast = &pQuest->tCast;	
	
	for( int i = 0; i < pCast->nNumCastMembers; ++i)
	{
		CAST_MEMBER *pCastMember = &pCast->tCastMember[ i ];
		UNIT *pUnit = sQuestCastMemberGetInstance( pQuest, pCastMember );
		if (pUnit)
		{
			s_InteractInfoUpdate( pQuest->pPlayer, pUnit, bUpdatingAllQuests );
		}
	}

	// we are now up to date
	CLEARBIT( pQuest->dwQuestFlags, QF_CAST_NEEDS_UPDATE_BIT );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDelayNext(
	QUEST * pQuest,
	int nDurationTicks )
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( nDurationTicks > 0, "Duration must be greater than 0" );
	// This function used to update the cast before setting the delay flag,
	// I've changes this to fix bug 33024 - "Brandon Lann's quest prompt is
	// appearing before he will give the quest (test monkey)" -cmarch

	// first, flag this quest to delay setting new states for subsequent quests (quests where this is the pre-req)
	SETBIT( pQuest->dwQuestFlags, QF_DELAY_NEXT );
	pQuest->nDelay = nDurationTicks;
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
	ASSERTX_RETURN( pQuestGlobals->nNumActiveQuestDelays < MAX_QUEST_DELAYS_ACTIVE, "Too many active delayed quests. Increase of array needed" );
	pQuestGlobals->nQuestDelayIds[pQuestGlobals->nNumActiveQuestDelays] = pQuest->nQuest;
	pQuestGlobals->nQuestDelayBit[pQuestGlobals->nNumActiveQuestDelays] = QF_DELAY_NEXT;
	pQuestGlobals->nNumActiveQuestDelays++;
	UnitSetFlag( pQuest->pPlayer, UNITFLAG_QUESTS_UPDATE, TRUE );

	// update all information icon states on NPCs
	s_sDoQuestCastInfoUpdate( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDelayCurrent(
	QUEST * pQuest,
	int nDurationTicks )
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( nDurationTicks > 0, "Duration must be greater than 0" );

	// first, flag this quest to delay setting new states for subsequent quests (quests where this is the pre-req)
	SETBIT( pQuest->dwQuestFlags, QF_DELAY_CURRENT );
	pQuest->nDelay = nDurationTicks;
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
	ASSERTX_RETURN( pQuestGlobals->nNumActiveQuestDelays < MAX_QUEST_DELAYS_ACTIVE, "Too many active delayed quests. Increase of array needed" );
	pQuestGlobals->nQuestDelayIds[pQuestGlobals->nNumActiveQuestDelays] = pQuest->nQuest;
	pQuestGlobals->nQuestDelayBit[pQuestGlobals->nNumActiveQuestDelays] = QF_DELAY_CURRENT;
	pQuestGlobals->nNumActiveQuestDelays++;
	UnitSetFlag( pQuest->pPlayer, UNITFLAG_QUESTS_UPDATE, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDoLevelTransitionUpdate(
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );

	// are we done yet?
	if (UnitIsInLimbo( pQuest->pPlayer ))
	{
		return;
	}

	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );

	ASSERTX_RETURN( pQuestGlobals, "Expected quest globals" );
	
	// setup message
	QUEST_MESSAGE_ENTER_LEVEL tMessage;
	tMessage.nLevelOldDef = pQuestGlobals->nLevelOldDef;
	tMessage.nLevelNewDef = pQuestGlobals->nLevelNewDef;
	tMessage.nLevelNewDRLGDef = pQuestGlobals->nLevelNewDRLGDef;

	// send to quest this quest
	QuestSendMessageToQuest( pQuest, QM_SERVER_ENTER_LEVEL, &tMessage );

	// we are now up to date
	CLEARBIT( pQuest->dwQuestFlags, QF_LEVEL_TRANSITION );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDoRoomsActivatedUpdate(	
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );

	// all the data is stored in the quest globals, I don't need a message structure

	// for now, this message is only sent to the activating player, and only for rooms in the default sublevel
	// send to quest this quest
	QuestSendMessageToQuest( pQuest, QM_SERVER_ROOMS_ACTIVATED, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDoQuestDelayUpdate(
	QUEST * pQuest,
	int index )
{
	ASSERTX_RETURN( pQuest, "Expected quest for quest delay" );
	ASSERTX_RETURN( ( index >= 0 ) && ( index < MAX_QUEST_DELAYS_ACTIVE ), "Invalid index to quest delay" );

	// are we done yet?
	if (UnitIsInLimbo( pQuest->pPlayer ))
	{
		return;
	}

	// count down
	pQuest->nDelay--;
	if ( pQuest->nDelay )
		return;

	// all done with delay
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
	ASSERTX_RETURN( pQuestGlobals->nQuestDelayBit[index] >= 0, "Invalid delay bit in quest delay" );
	CLEARBIT( pQuest->dwQuestFlags, pQuestGlobals->nQuestDelayBit[index] );

	// remove from list
	pQuestGlobals->nNumActiveQuestDelays--;
	if ( pQuestGlobals->nNumActiveQuestDelays > 0 )
	{
		// replace me with the last one
		pQuestGlobals->nQuestDelayIds[index] = pQuestGlobals->nQuestDelayIds[pQuestGlobals->nNumActiveQuestDelays];
		pQuestGlobals->nQuestDelayBit[index] = pQuestGlobals->nQuestDelayBit[pQuestGlobals->nNumActiveQuestDelays];
		pQuestGlobals->nQuestDelayIds[pQuestGlobals->nNumActiveQuestDelays] = INVALID_ID;
		pQuestGlobals->nQuestDelayBit[pQuestGlobals->nNumActiveQuestDelays] = INVALID_ID;
	}
	else
	{
		pQuestGlobals->nQuestDelayIds[0] = INVALID_ID;
		pQuestGlobals->nQuestDelayBit[0] = INVALID_ID;
	}

	// update the cast now...
	s_QuestsUpdateAvailability( pQuest->pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestsUpdate(
	GAME *pGame)
{
	if(AppIsTugboat())
	{
		// I guess that Tugboat doesn't do quest logic?
		return;
	}
#if HELLGATE_ONLY
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetPlayerUnit( pClient );
		if (!pPlayer)
			continue; // client may not be fully connected and in-game yet

		// get globals
		QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
		ASSERTX_RETURN( pQuestGlobals != NULL, "Expected non NULL quest globals" );

		if ( ( UnitTestFlag( pPlayer, UNITFLAG_QUESTS_UPDATE ) == TRUE ) && !UnitIsInLimbo( pPlayer ) )
		{
			// clear
			UnitSetFlag( pPlayer, UNITFLAG_QUESTS_UPDATE, FALSE );

			//BOOL bCheckForGiverItemDrop = FALSE;

			int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

			// run each quests update function
			for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
			{		
				QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
				if (!pQuest)		
					continue;

				// check for needing a cast update
				if (TESTBIT( pQuest->dwQuestFlags, QF_CAST_NEEDS_UPDATE_BIT ) == TRUE)
				{
					s_sDoQuestCastInfoUpdate( pQuest, TRUE );
				}

				if (TESTBIT( pQuest->dwQuestFlags, QF_LEVEL_TRANSITION ) == TRUE)
				{
					s_sDoLevelTransitionUpdate( pQuest );
				}

				if (TESTBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_ROOMS_ACTIVATED))
				{
					s_sDoRoomsActivatedUpdate( pQuest );						
				}

				// Update (if required) the player's flag which activates the 
				// check to see if a monster kill should spawn a giver item for a 
				// quest

				// drb - this is really odd... I'm not sure why this is here and what is going on. breaking for the moment...

				/*
				if ( !UnitTestFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE ) &&
					 !bCheckForGiverItemDrop &&
					 pQuest->pQuestDef->nGiverItem != INVALID_LINK &&
					 s_QuestCanBeActivated( pQuest ) )
				{
					bCheckForGiverItemDrop = TRUE;
				}
				*/
			}

			/*
			if ( !UnitTestFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE ) )
			{
				UnitSetFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM, bCheckForGiverItemDrop );
				UnitSetFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE, TRUE );
			}
			*/

			CLEARBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_ROOMS_ACTIVATED );
		}
		// quest delay updates
		if ( pQuestGlobals->nNumActiveQuestDelays != 0 )
		{
			int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
			for ( int i = 0; i < pQuestGlobals->nNumActiveQuestDelays; i++ )
			{
				ASSERTX_CONTINUE( pQuestGlobals->nQuestDelayIds[i] != INVALID_ID, "Quest Delay List is screwed up. Invalid Quest ID" );
				QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, pQuestGlobals->nQuestDelayIds[i], nDifficulty );
				ASSERTX_CONTINUE( pQuest, "Couldn't find quest associated with ID in Delay List" );
				ASSERT_CONTINUE( ( pQuestGlobals->nQuestDelayBit[i] == QF_DELAY_NEXT ) || ( pQuestGlobals->nQuestDelayBit[i] == QF_DELAY_CURRENT ) );
				s_sDoQuestDelayUpdate( pQuest, i );
			}
		}
		if ( pQuestGlobals->bTestOfHopeCheck )
		{
			QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_TESTHOPE );
			ASSERTX_CONTINUE( pQuest, "Trying to check test of hope and can't find it." );
			s_QuestToHCheck( pQuest );
		}
	}
#endif
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAddCastMember(
	QUEST *pQuest,
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	
	// we have no need to do this on the client (can't hurt tho if we want to)
	if (IS_CLIENT( QuestGetGame( pQuest ) ))
	{
		return;
	}
		
	// don't add invalid stuff
	EXCELTABLE eTable = UnitGenusGetDatatableEnum( eGenus );	
	if ((unsigned int)nClass >= ExcelGetCount(EXCEL_CONTEXT(), eTable))
	{
		return;
	}

	// get cast	
	CAST *pCast = &pQuest->tCast;
	
	// make species of member
	SPECIES spMember = MAKE_SPECIES( eGenus, nClass );
		
	// search for cast member already
	for (int i = 0; i < pCast->nNumCastMembers; ++i)
	{
		const CAST_MEMBER *pCastMember = &pCast->tCastMember[ i ];
		if (pCastMember->spMember == spMember )
		{
			return;
		}
	}
	
	// not found, add new cast member	
	ASSERTX_RETURN( pCast->nNumCastMembers < MAX_QUEST_CAST_MEMBERS - 1, "Too many cast members for quest" );
	CAST_MEMBER *pCastMember = &pCast->tCastMember[ pCast->nNumCastMembers++ ];
	pCastMember->spMember= spMember;
	pCastMember->idUnit = INVALID_ID;
		
	// the new cast member needs an update
	s_QuestUpdateCastInfo( pQuest );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestUpdateCastInfo(
	QUEST *pQuest)
{				
	SETBIT( pQuest->dwQuestFlags, QF_CAST_NEEDS_UPDATE_BIT );
	UnitSetFlag( pQuest->pPlayer, UNITFLAG_QUESTS_UPDATE, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestsUpdateCastInfo(
	UNIT *pPlayer)
{
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if ( pQuest && !QuestIsComplete( pQuest ) )
		{
			s_QuestUpdateCastInfo( pQuest );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestUpdateAvailability(
	QUEST *pQuest)
{				
	s_QuestUpdateCastInfo( pQuest );
	UnitClearFlag( QuestGetPlayer( pQuest ), UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestsUpdateAvailability(
	UNIT *pPlayer)
{
	s_sQuestsUpdateCastInfo( pPlayer );
	UnitClearFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestAllStatesComplete(
	QUEST *pQuest,
	BOOL bIgnoreLastState)
{
	ASSERT_RETFALSE( pQuest );
	
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		
		// ignore last state if requested
		if (bIgnoreLastState == TRUE && i == pQuest->nNumStates - 1)
		{
			continue;
		}
		
		// check state
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		if (QuestStateGetValue( pQuest, pState ) != QUEST_STATE_COMPLETE)
		{
			return FALSE;
		}
		
	}
	
	return TRUE;
	
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb - this will need to be fixed. no tech yet...
void s_QuestStateSetForAll(
	QUEST *pQuest, 
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bUpdateCastInfo /*= TRUE*/)
{

	// Set a state for everyone in the game
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	// for single player, just do this player
	if (AppIsSinglePlayer() || UnitGetPartyId( pPlayer ) == INVALID_ID)
	{
		QuestStateSet( pQuest, nQuestState, eValue, bUpdateCastInfo );
	}
	else
	{
		PARTYID idParty = UnitGetPartyId( pPlayer );
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			
			// get unit
			UNIT *pUnit = ClientGetControlUnit( pClient );
			
			// must match party id
			if (UnitGetPartyId( pUnit ) == idParty)
			{
				QUEST *pQuestOther = QuestGetQuest( pUnit, pQuest->nQuest );
				if (pQuestOther)
					QuestStateSet( pQuestOther, nQuestState, eValue, bUpdateCastInfo );
			}
			
		}
					
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb - this will need to be fixed. no tech yet...
void s_QuestStateSetFromClient(
	UNIT * player,
	int nQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bUpdate )
{
	QUEST * pQuest = QuestGetQuest( player, nQuest );
	if ( !pQuest )
		return;

	QuestStateSet( pQuest, nQuestState, eValue, bUpdate );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// NOTE: This should only be called during a restore. Please don't use during
// active quests
BOOL s_QuestStateRestore( 
	QUEST *pQuest, 
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bSend)
{
	// only active quests can set state
	if (QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE)
	{
		QUEST_STATE *pState = QuestStateGetPointer( pQuest, nQuestState );
			
		// set new value for state
		pState->eValue = eValue;

		if ( bSend )
		{
			QuestSendState( pQuest, nQuestState, TRUE );
		}
		return TRUE;  // state was set

	}
		
	return FALSE;  // state was not set
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestSendStatus(
	QUEST *pQuest,
	BOOL bRestore = FALSE)
{
	UNIT * pPlayer = pQuest->pPlayer;
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN(pGame);
	
	// don't send messages if restoring
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	if ( !pQuestGlobals )
	{
		return;
	}
	if ( !pQuestGlobals->bQuestsRestored )
	{
		return;
	}

	// setup state message
	MSG_SCMD_QUEST_STATUS tMessage;
	tMessage.nQuest = pQuest->nQuest;
	tMessage.nStatus = pQuest->eStatus;
	tMessage.bRestore = (BYTE)bRestore;
	tMessage.nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	
	// send to single client
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage(pGame, idClient, SCMD_QUEST_STATUS, &tMessage);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestStatusAllowsPlayerInteraction(
	QUEST_STATUS eStatus)
{
	switch (eStatus)
	{
		case QUEST_STATUS_ACTIVE:
		case QUEST_STATUS_COMPLETE:
		case QUEST_STATUS_OFFERING:
			return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestCleanupForSave( 
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected player" );
		
	// if quest status doesn't allow interaction, we have some stuff to cleanup
	if (s_QuestStatusAllowsPlayerInteraction( pQuest->eStatus ) == FALSE)
	{

		// get difficulty of this quest
		int nDifficulty = QuestGetDifficulty( pQuest );
			
		// clear the tracking stat for this quest
		PARAM dwTrackingParam = StatParam( STATS_QUEST_PLAYER_TRACKING, pQuest->nQuest, nDifficulty );
		UnitClearStat( pPlayer, STATS_QUEST_PLAYER_TRACKING, dwTrackingParam );
		
		// clear the offer num items taken
		PARAM dwItemsTakenParam = StatParam( STATS_OFFER_NUM_ITEMS_TAKEN, pQuest->nQuest, nDifficulty );
		UnitClearStat( pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, dwItemsTakenParam );
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestSetStatus(
	QUEST *pQuest,
	QUEST_STATUS eStatus,
	BOOL bAllowTrackingChanges = TRUE)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	QUEST_STATUS eOldStatus = pQuest->eStatus;
	BOOL bRepeating = FALSE;

	// this will not have the intended effect if this is being done before the quests
	// are setup from any save file data
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	if (IS_SERVER( pGame ))
	{
		QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
		ASSERTX_RETURN( pQuestGlobals, "Expected quest globals" );
		ASSERTV( 
			TESTBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_QUESTS_SETUP ),
			"Player '%s' unable to set quest '%s' state value because quests are not yet setup!",
			UnitGetDevName( pPlayer ),
			QuestGetDevName( pQuest ));
	}

	// some quests go directly to the closed status on complete
	if ( ( eStatus == QUEST_STATUS_COMPLETE )&& pQuestDef->bCloseOnComplete )
	{
		eStatus = QUEST_STATUS_CLOSED;  // used closed status instead
	}
	// check if we are completing or closing the quest and it is supposed to be repeatable
	if ( ( ( eStatus == QUEST_STATUS_COMPLETE ) || ( eStatus == QUEST_STATUS_CLOSED ) ) && pQuestDef->bRepeatable )
	{
		if ( eStatus == QUEST_STATUS_COMPLETE )
		{
			// re-init quest states
			s_QuestClearAllStates( pQuest, TRUE );
			bRepeating = TRUE;
		}
		eStatus = QUEST_STATUS_INACTIVE;  // used inactive status instead
	}
	
	// set the status
	pQuest->eStatus = eStatus;

	if (bAllowTrackingChanges == TRUE && s_QuestStatusAllowsPlayerInteraction( eStatus ) == FALSE)
	{
		ASSERTX_RETURN( pQuest->pPlayer, "Expected player" );
		s_QuestTrack(pQuest, FALSE);
	}
		
	// send to clients
	sQuestSendStatus( pQuest );

	if ( bRepeating && pQuestDef->nRepeatRate != 0 )
	{
		s_QuestDelayCurrent( pQuest, pQuestDef->nRepeatRate * GAME_TICKS_PER_SECOND );
	}

	// everybody needs info update
	s_QuestUpdateAvailability( pQuest );

	// send event to this server quest
	if ( pQuest->eStatus != eOldStatus )
	{
		s_QuestEventStatus( pQuest, eOldStatus );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDoOnActivateStates(
	QUEST *pQuest,
	BOOL bRestoring)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *ptState = QuestStateGetByIndex( pQuest, i );
		const QUEST_STATE_DEFINITION *ptStateDef = QuestStateGetDefinition( ptState->nQuestState );
		if (ptStateDef->bActivateWithQuest == TRUE)
		{

			// either activate state normally, or restore it when restoring		
			if (bRestoring == FALSE)
			{
				QuestStateSet( pQuest, ptState->nQuestState, QUEST_STATE_ACTIVE );
			}
			else
			{
				s_QuestStateRestore( pQuest, ptState->nQuestState, QUEST_STATE_ACTIVE );
			}
			
		}
		
	}
	
}				
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sQuestGiveItems(
	QUEST * pQuest,
	int nTreasureClass, 
	BOOL bRemoveOnDeactivateQuest = FALSE,
	BOOL bStartingItems = FALSE,
	DWORD dwQuestGiveItemFlags = 0)		// see QUEST_GIVE_ITEM_FLAGS
{
	BOOL bPickupSucceeded = FALSE;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	// check state of quest for everyone
	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETFALSE( IS_SERVER( game ) );

	ASSERTV_RETFALSE( nTreasureClass != INVALID_LINK, "quest \"%s\": invalid treasure class to give", pQuest->pQuestDef->szName );

	// setup buffer to hold all the items generated by treasure class
	UNIT * pItems[ MAX_QUEST_GIVEN_ITEMS ];	

	// setup spawn result
	ITEMSPAWN_RESULT tResult;
	tResult.pTreasureBuffer = pItems;
	tResult.nTreasureBufferSize = MAX_QUEST_GIVEN_ITEMS;

	// set item spec
	ITEM_SPEC tSpec;
	tSpec.unitOperator = player;
	SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
	if (TESTBIT( dwQuestGiveItemFlags, QGIF_IGNORE_GLOBAL_THEME_RULES_BIT ))
	{
		SETBIT( tSpec.dwFlags, ISF_IGNORE_GLOBAL_THEME_RULES_BIT );
	}
	s_TreasureSpawnClass( player, nTreasureClass, &tSpec, &tResult );
	ASSERTV_RETFALSE( tResult.nTreasureBufferCount > 0, "quest \"%s\": No items generated for quest to give from treasure class %i", pQuest->pQuestDef->szName, nTreasureClass );
	DWORD dwFreeFlags = 0;

	// check if the player has room to take the items
	if ( UnitInventoryCanPickupAll( 
		player, tResult.pTreasureBuffer, tResult.nTreasureBufferCount, TRUE ) )
	{
		BOOL bPlayerCanSendMsgs = UnitCanSendMessages( player );
		ROOM *pRoom = NULL;
		const VECTOR &position = UnitGetPosition( player );
		VECTOR faceDirection;
		VECTOR vupDirection;
		bPickupSucceeded = TRUE;
		if ( bPlayerCanSendMsgs )
		{		
			pRoom = UnitGetRoom( player );
			if ( pRoom == NULL )
			{
				ASSERTV(FALSE, "quest \"%s\": Trying to treasure spawn and pickup into a container that is not in the world but the container thinks its ready to send network messages", pQuest->pQuestDef->szName );
				bPickupSucceeded = FALSE;
				dwFreeFlags |= UFF_SEND_TO_CLIENTS;
			}
			faceDirection = UnitGetFaceDirection( player, FALSE );
			vupDirection = UnitGetVUpDirection( player );
		}

		// move all the items to the player's inventory
		if ( bPickupSucceeded )
			for (int i = 0; i < tResult.nTreasureBufferCount; ++i)
			{
				UNIT *pItem = tResult.pTreasureBuffer[i];
				if (bPlayerCanSendMsgs)
				{
					// item is ready for messages
					UnitSetCanSendMessages( pItem, TRUE );

					// for now, put item in world and pick it up
					UnitWarp( 
						pItem,
						pRoom,
						position,
						faceDirection,
						vupDirection,
						0,
						FALSE);
				}

				if ( bStartingItems )
				{
					int nDifficulty = UnitGetStat(player, STATS_DIFFICULTY_CURRENT);
					UnitSetStat( pItem, STATS_QUEST_STARTING_ITEM, nDifficulty, pQuest->nQuest );
#if HELLGATE_ONLY
#if ISVERSION(DEVELOPMENT) 
					const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
					if (pQuestDef->tFunctions.tTable[QFUNC_INIT].pfnFunction == QuestInitUseItem)
					{
						// sanity check data
						const UNIT_DATA *pItemData = UnitGetData( pItem );
						ASSERTV_RETFALSE( pItemData, "quest \"%s\": the \"Starting Item Treasure class\" item in quest.xls has the wrong version_package", pQuestDef->szName );
						ASSERTV_RETFALSE( UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE ), "quest starting item \"%s\" for UseItem quest must have InformQuestsOnPickup in items.xls set to 1", pItemData->szName );
						ASSERTV_RETFALSE( UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_NO_DROP), "quest starting item \"%s\" for UseItem quest must have NoDrop in items.xls set to 1", pItemData->szName );
					}
					// this doesn't seem to be used ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP ), "quest item \"%s\" must have AskQuestsForPickup in items.xls set to 1", pCollectItemData->szName );
#endif //ISVERSION(DEVELOPMENT)
#endif
				}
				
				if ( bRemoveOnDeactivateQuest )
				{
					int nDifficulty = UnitGetStat(player, STATS_DIFFICULTY_CURRENT);
					UnitSetStat( pItem, STATS_REMOVE_WHEN_DEACTIVATING_QUEST, nDifficulty, pQuest->nQuest );
				}

				// pick up item
				UNIT *pPickedUpItem	= pPickedUpItem = s_ItemPickup( player, pItem );

				// must have the item in the inventory if it was not stacked during the pickup
				if ( pPickedUpItem == NULL ||
					UnitGetContainer( pPickedUpItem ) != player )
				{
					ASSERTV( FALSE, "quest \"%s\": failed to give item to player after checking if they had enough room", pQuest->pQuestDef->szName );
					bPickupSucceeded = FALSE;
					dwFreeFlags |= UFF_SEND_TO_CLIENTS;
					break;
				}
			}
	}

	if ( !bPickupSucceeded )
	{
		// send a message to the client to give feedback (text message, etc.)
		// since the items could not be picked up
		// setup message		
		MSG_SCMD_UISHOWMESSAGE tMessage;
		tMessage.bType = (BYTE)QUIM_DIALOG;
		tMessage.nDialog = DIALOG_NO_SPACE_PICKUP;
		tMessage.bFlags = (BYTE)0;

		// send message
		GAMECLIENTID idClient = UnitGetClientId( player );
		s_SendMessage( game, idClient, SCMD_UISHOWMESSAGE, &tMessage );

		// destroy the items
		for (int i = 0; i < tResult.nTreasureBufferCount; ++i)
			UnitFree( tResult.pTreasureBuffer[i], dwFreeFlags );
	}

#endif
	return bPickupSucceeded;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestRemoveItemsOnDeactivate(
	QUEST * pQuest )
{
	UNIT *player = QuestGetPlayer( pQuest );
	ASSERT_RETURN( player );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( player ) ) );

	// if no flags provided, use defaults
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );

	UNIT * item = UnitInventoryIterate( player, NULL, dwFlags );
	UNIT * next;
	// TODO cmarch add a difficulty param to the stat below, and rename to "deactivate" int nDifficulty = UnitGetStat( player, STATS_DIFFICULTY_CURRENT );
	// then mark collect, doorfix, and stitch drops with that stat
	while ( item != NULL )
	{
		next = UnitInventoryIterate( player, item, dwFlags );		
		int nDifficulty = UnitGetStat(player, STATS_DIFFICULTY_CURRENT);
		if ( UnitGetStat( item, STATS_REMOVE_WHEN_DEACTIVATING_QUEST, nDifficulty ) == pQuest->nQuest )
		{
			UnitFree( item, UFF_SEND_TO_CLIENTS );
		}
		item = next;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestGiveItem(
	QUEST * pQuest,
	GLOBAL_INDEX eTreasure,
	DWORD dwQuestGiveItemFlags /*= 0*/)		// see QUEST_GIVE_ITEM_FLAGS)
{
	return s_sQuestGiveItems( pQuest, GlobalIndexGet( eTreasure ), FALSE, FALSE, dwQuestGiveItemFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestGiveGold(
	QUEST * pQuest,
	int nAmount )
{
	// check state of quest for everyone
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETURN( IS_SERVER( player ) );

	// change money
	UnitAddCurrency( player, cCurrency( nAmount, 0 ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * s_QuestFindFirstItemOfType( 
	UNIT *player, 
	int nItemClass,
	DWORD dwFlags )
{
	ASSERT_RETFALSE( player || UnitIsNPC( player ) );
	ASSERT_RETFALSE( IS_SERVER( UnitGetGame( player ) ) );

	// if no flags provided, use defaults
	if (dwFlags == NONE)
	{
		dwFlags = 0;
		SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	}
	else if (dwFlags == 0)
	{
		WARNX(FALSE, "s_QuestFindFirstItemOfType called with dwFlags set to 0, this would count items in hidden inventory slots (like offeringstorage). Setting to IIF_ON_PERSON_AND_STASH_ONLY_BIT");
		SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	}

	UNIT * item = UnitInventoryIterate( player, NULL, dwFlags );
	while ( item != NULL )
	{
		if ( UnitGetClass( item ) == nItemClass )
		{
			return item;
		}
		item = UnitInventoryIterate( player, item, dwFlags );
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestCheckForItem( 
	UNIT *player, 
	int nItemClass,
	DWORD dwFlags /*= NONE*/)
{
	return s_QuestFindFirstItemOfType( player, nItemClass, dwFlags ) != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestRemoveItem( 
	UNIT * player, 
	int nItemClass,
	DWORD dwFlags /*= NONE*/,
	int nQuantity /* = -1 */) // -1 means remove all
{
	ASSERT_RETFALSE( player );
	GAME *pGame = UnitGetGame( player );
	ASSERT_RETFALSE( IS_SERVER( pGame ) );

	// if no flags provided, use defaults
	if (dwFlags == NONE)
	{
		dwFlags = 0;
		SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	}

	// if the nQuantity argument is 0, remove all items of the given class
	return QuestRemoveItemsFromPlayer( pGame, player, nItemClass, nQuantity, dwFlags );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestRemoveItemForAll( 
	QUEST *pQuest,
	int nItemClass,
	DWORD dwFlags/* = NONE*/)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( nItemClass != INVALID_LINK, "Expected item class link" );
	UNIT *pQuestPlayer = QuestGetPlayer( pQuest );
	PARTYID idParty = UnitGetPartyId( pQuestPlayer );

	GAME *pGame = UnitGetGame( pQuestPlayer );	
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{

		// get unit
		UNIT *pPlayer = ClientGetControlUnit( pClient );

		// must match party id
		if (UnitGetPartyId( pPlayer ) == idParty)
		{
			s_QuestRemoveItem( pPlayer, nItemClass, dwFlags );
		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestRemoveQuestItems( 
	UNIT * pPlayer, 
	QUEST * pQuest,
	DWORD dwFlags /*= NONE*/)
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( IS_SERVER( UnitGetGame( pPlayer ) ) );
	ASSERT_RETFALSE( pQuest );

	// if no flags provided, use defaults
	if (dwFlags == NONE)
	{
		dwFlags = 0;
		SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	}

	BOOL bFound = FALSE;
	UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags );
	UNIT * next;
	int nQuest = pQuest->nQuest;
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	while ( item != NULL )
	{
		next = UnitInventoryIterate( pPlayer, item, dwFlags );		
		if ( UnitGetStat(item, STATS_QUEST_ITEM, nDifficulty) == nQuest )
		{
			UnitFree( item, UFF_SEND_TO_CLIENTS );
			bFound = TRUE;
		}
		item = next;
	}

#if (0)
	// how many of the item do we have
	int nQuantity = ItemGetQuantity( pItem );

	// remove from inventory
	UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_SINGLE );

#if (0) // TODO cmarch: mod rewards?
	// if we're supposed to add to reward items, do so now, otherwise destroy item
	if (pQuest->pTaskDefinition->bCollectModdedToRewards && UnitIsA( pRemoved, UNITTYPE_MOD ))
	{
		sModRewards( pGame, pQuest, pRemoved, pRewardsTaken, nNumRewards );
	}
	else
#endif
	{
		UnitFree( pRemoved, 0 );
	}
#endif
	return bFound;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestActivate(
	QUEST *pQuest,
	BOOL bAllowTrackingChanges /*= TRUE*/)
{

	if (pQuest->eStatus == QUEST_STATUS_INACTIVE)
	{
		// activation may fail if the player doesn't have enough room for
		// starting quest items (minus any space taken by an activation item)
		UNIT *pPlayer = QuestGetPlayer( pQuest );

		// first we make sure the player is not at the limit of non-story quests
		if (!QuestPlayCanAcceptQuest(pPlayer, pQuest))
		{
			return FALSE;
		}

		// do not activate the quest if they don't have the prereqs complete
		if (s_QuestIsPrereqComplete( pQuest ) == FALSE)
		{
			return FALSE;
		}
		
		// save the location of any giver item, and remove it from inventory
		const int nGiverItem = pQuest->pQuestDef->nGiverItem;
		UNIT * pGiverItem = NULL;
		int nOriginalLocation = INVLOC_NONE, nOriginalX = 0, nOriginalY = 0;
		if ( nGiverItem != INVALID_LINK )
		{
			pGiverItem = s_QuestFindFirstItemOfType( pPlayer, nGiverItem );
			ASSERTV_RETFALSE( pGiverItem, "quest \"%s\": expected giver item in inventory on quest activation", pQuest->pQuestDef->szName );
			UnitGetInventoryLocation(pGiverItem, &nOriginalLocation, &nOriginalX, &nOriginalY);
			// remove the giver item, but don't delete yet, in case activation fails
			pGiverItem = UnitInventoryRemove(pGiverItem, ITEM_SINGLE, CLF_FORCE_SEND_REMOVE_MESSAGE);
		}

		int nStartingItemsTreasureClass = pQuest->pQuestDef->nStartingItemsTreasureClass;
		if ( nStartingItemsTreasureClass != INVALID_LINK )
		{
			if ( !s_sQuestGiveItems( 
					pQuest, 
					nStartingItemsTreasureClass, 
					pQuest->pQuestDef->bRemoveStartingItemsOnComplete,
					TRUE ) )
			{
				// Failed to give starting items
				// Put the giver item back where it was
				if ( pGiverItem )
					UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pPlayer, pGiverItem, nOriginalLocation, nOriginalX, nOriginalY );
				return FALSE;
			}
		}
		// free the activation item, if any 
		if ( pGiverItem )
		{
			BOOL bPlayerCanSendMsgs = UnitCanSendMessages( pPlayer );
			ROOM *pRoom = NULL;
			const VECTOR &position = UnitGetPosition( pPlayer );
			VECTOR faceDirection;
			VECTOR vupDirection;
			if ( bPlayerCanSendMsgs )
			{		
				pRoom = UnitGetRoom( pPlayer );

				faceDirection = UnitGetFaceDirection( pPlayer, FALSE );
				vupDirection = UnitGetVUpDirection( pPlayer );

				UnitSetCanSendMessages( pGiverItem, TRUE );

				// for now, put item in world so it can be freed properly
				UnitWarp( 
					pGiverItem,
					pRoom,
					position,
					faceDirection,
					vupDirection,
					0,
					FALSE);
			}

			UnitFree( pGiverItem, UFF_SEND_TO_CLIENTS );
		}
			
		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_ACTIVE, bAllowTrackingChanges );

		// play some effects on the player
		if ( !UnitIsInLimbo( pPlayer ) && !pQuest->pQuestDef->bSkipActivateFanfare )
			s_StateSet( pPlayer, pPlayer, STATE_QUEST_ACCEPT, 0 );

		// unlock the destination level for quests that aren't story quests
//  		if ( pQuest->pQuestDef->eStyle != QS_STORY && pQuest->pQuestDef->nLevelDefDestinations[0] != INVALID_LINK)
// 			LevelMarkVisited( pPlayer, pQuest->pQuestDef->nLevelDefDestinations[0], WORLDMAP_REVEALED );

		// activate the on activate states
		s_QuestDoOnActivateStates( pQuest, FALSE );

		// send activated message to quests
		s_QuestEventActivated( pQuest );

		// unlock warp
		if ( pQuest->pQuestDef->nWarpToOpenActivate != INVALID_LINK )
		{
			s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuest->pQuestDef->nWarpToOpenActivate );
		}

		return TRUE;  // quest was activated
		
	}

	return FALSE;  // quest was *not* activated		
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDeactivate(
	QUEST *pQuest,
	BOOL bAllowTrackingChanges /*= TRUE*/)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	
	// hide all states
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *ptState = QuestStateGetByIndex( pQuest, i );
		s_QuestStateRestore( pQuest, ptState->nQuestState, QUEST_STATE_HIDDEN );
	}	
	
	// set status to inactive
	sQuestSetStatus( pQuest, QUEST_STATUS_INACTIVE, bAllowTrackingChanges );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestReactivate(
	QUEST *pQuest)
{
	
	// deactivate and reactivate, but preserve any tracking state they had
	s_QuestDeactivate( pQuest, FALSE );
	s_QuestActivate( pQuest );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestActivateForAll(
	QUEST *pQuest )
{
	
	// Set a state for everyone in the game
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	if (AppIsSinglePlayer() || UnitGetPartyId( pPlayer ) == INVALID_ID)
	{
		s_QuestActivate( pQuest );
	}
	else
	{
		PARTYID idParty = UnitGetPartyId( pPlayer );
		
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pUnit = ClientGetControlUnit( pClient );
			if (UnitGetPartyId( pUnit ) == idParty)
			{
				QUEST *pQuestOther = QuestGetQuest( pUnit, pQuest->nQuest );
				if ( pQuestOther )
					s_QuestActivate( pQuestOther );
			}
			
		}
		
	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb - this will need to be fixed. no tech yet...
BOOL s_QuestActivateFromClient(
	UNIT * player,
	int nQuest)
{
	QUEST * pQuest = QuestGetQuest( player, nQuest );
	if ( !pQuest )
		return FALSE;

	return s_QuestActivate( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestIsPrereqComplete(
	QUEST *pQuest)
{
	ASSERT_RETFALSE(pQuest);
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	ASSERT_RETFALSE(pQuestDef);

	if ( TESTBIT( pQuest->dwQuestFlags, QF_DELAY_CURRENT ) )
	{
		return FALSE;
	}

	UNIT *pPlayer = pQuest->pPlayer;
	ASSERT_RETFALSE( UnitIsA( pPlayer, UNITTYPE_PLAYER ) );
	GAME *pGame = UnitGetGame(pPlayer);

	if ( pQuestDef->bSubscriberOnly && !PlayerIsSubscriber( pPlayer ) )
	{
		return FALSE;
	}

	if ( pQuestDef->nGlobalThemeRequired >= 0 && !GlobalThemeIsEnabled(pGame, pQuestDef->nGlobalThemeRequired) )
	{
		return FALSE;
	}

	if ( pQuestDef->bMultiplayerOnly && !AppIsMultiplayer() )
	{
		return FALSE;
	}

	// is this quest currently turned off?
	if ( pQuestDef->bUnavailable )
	{
		return FALSE;
	}

	// prereqs must be complete
	for (int i = 0; i < arrsize( pQuestDef->nQuestPrereqs ); ++i)
	{
		int nQuestPrereq = pQuestDef->nQuestPrereqs[i];
		if (nQuestPrereq != INVALID_LINK)
		{
			// check for complete status for prereq
			QUEST *pQuestPrereq = QuestGetQuest( pPlayer, nQuestPrereq );
			ASSERTV_CONTINUE( pQuestPrereq, "quest \"%s\": Expected quest prereq", pQuestDef->szName );
			//if (QuestIsDone( pQuestPrereq ) == FALSE || pQuestPrereq->pQuestDef->bBeatGameOnComplete) // halt progression if a quest is flagged for beating the game - cmarch
			if ( QuestIsDone( pQuestPrereq ) == FALSE )
			{
				return FALSE;
			}
		}
	}

	if (pQuestDef->nFactionTypePrereq != INVALID_LINK)
	{

		if (FactionScoreGet( pPlayer, pQuestDef->nFactionTypePrereq ) <
				pQuestDef->nFactionAmountPrereq)
			return FALSE;

	}

	// make sure the level destination for a non-story quest is accessible
	if (pQuestDef->eStyle != QS_STORY && pQuestDef->nLevelDefDestinations[0] != INVALID_LINK)
	{
		LEVEL *pLevelCur = UnitGetLevel(pPlayer);
		if (pLevelCur)
		{

			// get the connected levels to this area
			LEVEL_CONNECTION pConnectedLevels[ MAX_CONNECTED_LEVELS ];
			int nNumConnected = 
				s_LevelGetConnectedLevels( 
					pLevelCur, 
					pLevelCur->m_idWarpCreator, 
					pConnectedLevels, 
					MAX_CONNECTED_LEVELS,
					TRUE);

			for (int i = 0; i < nNumConnected; ++i)
			{
				LEVEL* pLevel = pConnectedLevels[ i ].pLevel;
				if (LevelGetDefinitionIndex(pLevel) == pQuestDef->nLevelDefDestinations[0])
				{	
					UNITID idWarpToLevel = pConnectedLevels[ i ].idWarp;
					UNIT *pWarp = UnitGetById(pGame, idWarpToLevel);
					if (pWarp && ObjectCanOperate(pPlayer, pWarp) != OPERATE_RESULT_OK)
					{
						return FALSE;
					}
				}
			}
		}
	}

	// is this quest available only once ever, regardless of difficulty?
	if ( pQuestDef->bOneDifficulty )
	{
		// check if they completed this quest on any difficulty
		int quest_index = pQuest->nQuest;
		for ( int nDifficulty = DIFFICULTY_NORMAL; nDifficulty < NUM_DIFFICULTIES; nDifficulty++ )
		{
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, quest_index, nDifficulty, FALSE );
			if ( pQuest )
			{
				QUEST_STATUS eStatus = QuestGetStatus( pQuest );
				if ( eStatus >= QUEST_STATUS_COMPLETE )
					return FALSE;
			}
		}
	}

	int nExperienceLevel = UnitGetExperienceLevel( pPlayer );

	if ( ( pQuestDef->nMaxLevelPrereq != -1 ) && ( nExperienceLevel > pQuestDef->nMaxLevelPrereq ) )
		return FALSE;

	return nExperienceLevel >= pQuestDef->nMinLevelPrereq;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEnableNextQuests(
	QUEST *pQuestCompleted)
{
	UNIT *pPlayer = pQuestCompleted->pPlayer;

	// go through all quests
	unsigned int nNumQuests = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_QUEST);
	for (unsigned int i = 0; i < nNumQuests; ++i)
	{
		const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( i );
		if (!ptQuestDef)
			continue;

		// is the quest being completed a prereq for this quest def
		for (int j = 0; j < arrsize( ptQuestDef->nQuestPrereqs ); ++j)
			if (ptQuestDef->nQuestPrereqs[j] == pQuestCompleted->nQuest)
			{
				QUEST *pQuest = QuestGetQuest( pPlayer, i );
				ASSERTV_CONTINUE( pQuest, "quest \"%s\": Expected quest", ptQuestDef->szName );
				
				// update the cast for this quest (including the giver)
				s_QuestUpdateAvailability( pQuest );
			}		
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRewardFaction( 
	QUEST *pQuest)
{
	ASSERTX_RETURN(	pQuest, "Expected quest" );
	
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	if (ptQuestDef->nFactionTypeReward != INVALID_LINK && 
		ptQuestDef->nFactionAmountReward != 0)
	{
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		FactionScoreChange( 
			pPlayer, 
			ptQuestDef->nFactionTypeReward, 
			ptQuestDef->nFactionAmountReward);
	}
}

//----------------------------------------------------------------------------
// Grants the player any stat rewards (experience, gold, skill points)
//----------------------------------------------------------------------------
static void sRewardStats( QUEST *pQuest )
{
	ASSERT_RETURN( pQuest != NULL );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETURN( pPlayer != NULL );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETURN( ptQuestDef != NULL );

	PlayerGiveSkillPoint( pPlayer, QuestGetSkillPointReward( pQuest ) );

	PlayerGiveStatPoints( pPlayer, QuestGetStatPointReward( pQuest ) );

	int nExperience = QuestGetExperienceReward( pQuest );
	s_UnitGiveExperience( pPlayer, nExperience );

	s_QuestGiveGold( pQuest, QuestGetMoneyReward( pQuest ) );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestComplete(
	QUEST *pQuest)
{
	if ( QuestIsActive( pQuest ) || QuestIsOffering( pQuest ) )
	{
		const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;

		// run the quest specific complete function, if any
		QUEST_FUNCTION_PARAM tParam;
		tParam.pQuest = pQuest;
		tParam.pPlayer = QuestGetPlayer( pQuest );
		QuestRunFunction( pQuest, QFUNC_ON_COMPLETE, tParam );

		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_COMPLETE );

		// maybe complete all states here one day maybe?
		
		// play some effects on the player
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		if ( !UnitIsInLimbo( pPlayer ) && !pQuestDef->bSkipCompleteFanfare )
			s_StateSet( pPlayer, pPlayer, STATE_QUEST_COMPLETE, 0 );

		// remove all quest items
		for( int i = 0; i < MAX_QUEST_COMPLETE_ITEMS; i++ )
		{
			if( pQuestDef->nCompleteItemList[i] != INVALID_ID )
			{
				s_QuestRemoveItem( pPlayer, pQuestDef->nCompleteItemList[i] );
			}
		}

		s_sQuestRemoveItemsOnDeactivate( pQuest );

		// if collecting world drops, remove the exact quantity required by the quest
		if ( pQuestDef->nCollectItem != INVALID_LINK && 
			 pQuestDef->fCollectDropRate <= 0.0f &&
			 pQuestDef->nObjectiveCount > 0)
		{
			s_QuestRemoveItem( pPlayer, pQuestDef->nCollectItem, 0, pQuestDef->nObjectiveCount ); 
		}

		// unlock warp
		if ( pQuest->pQuestDef->nWarpToOpenComplete != INVALID_LINK )
		{
			s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuest->pQuestDef->nWarpToOpenComplete );
		}

		// grant the player any faction points for completing the quest		
		sRewardFaction( pQuest );

		// grant the player any stat rewards (experience, gold, skill points)
		sRewardStats( pQuest );

		// see if there any more quests ready to become active now
		if ( !pQuestDef->bRepeatable )
			sEnableNextQuests( pQuest );

		// update all quest availability
		s_sQuestsUpdateCastInfo( pQuest->pPlayer );

#if !ISVERSION(CLIENT_ONLY_VERSION)
		// the minigame can involve quest completion
		s_PlayerCheckMinigame(UnitGetGame(pPlayer), pPlayer, MINIGAME_QUEST_FINISH, 0);
#endif

		// tell the achievements system
		s_AchievementsSendQuestComplete( pQuest->nQuest, pPlayer );

		// last story quest, beat current mode when player completes this quest 
		if ( pQuest->pQuestDef->bBeatGameOnComplete )
		{
			//Unlock higher difficulty for a character if they've earned it
			GAME* pGame = UnitGetGame(pPlayer);
			ASSERTV(pGame, "GAME* for game on final quest complete is NULL");
			if ( pGame->nDifficulty+1 < NUM_DIFFICULTIES &&
				 UnitGetStat(pPlayer, STATS_DIFFICULTY_MAX) <= pGame->nDifficulty )
			{
				UnitSetStat( pPlayer, STATS_DIFFICULTY_MAX, pGame->nDifficulty+1 );
				//int maxDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_MAX);
				//UnitSetStat(pPlayer, STATS_DIFFICULTY_CURRENT, maxDifficulty);

				// let them know the next difficulty level is unlocked, and they
				// can go switch to it in the character select screen
				const DIFFICULTY_DATA *pDifficultyData = DifficultyGetData( pGame->nDifficulty+1 );
				if ( pDifficultyData && pDifficultyData->nUnlockedString != INVALID_LINK )
				{
					// send message to client to display anti fatigue message
					MSG_SCMD_UISHOWMESSAGE tMessage;
					tMessage.bType = (BYTE)QUIM_STRING;
					tMessage.nDialog = pDifficultyData->nUnlockedString;
					tMessage.bFlags = (BYTE)UISMS_FADEOUT;
					tMessage.nParam1 = 5 * GAME_TICKS_PER_SECOND; // duration

					// send message
					GAMECLIENTID idClient = UnitGetClientId( pPlayer );
					s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_UISHOWMESSAGE, &tMessage );
				}

			}

			// check badges, send "mode beaten" message to account servers
			GAMECLIENT *pClient = UnitGetClient( pPlayer );
			if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_HARDCORE ) )
			{
				if ( !ClientHasBadge( pClient, ACCT_ACCOMPLISHMENT_HARDCORE_MODE_BEATEN ) )
					GameClientAddAccomplishmentBadge( pClient, ACCT_ACCOMPLISHMENT_HARDCORE_MODE_BEATEN );
			}
			else
			{
				if ( !ClientHasBadge( pClient, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN ) )
					GameClientAddAccomplishmentBadge( pClient, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN );

			}


		}	
	}
	
	BOOL bRetVal = FALSE;
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus == QUEST_STATUS_COMPLETE )
		bRetVal = TRUE;
	if ( eStatus == QUEST_STATUS_CLOSED )
		bRetVal = TRUE;
	if ( ( eStatus == QUEST_STATUS_INACTIVE ) && ( pQuest->pQuestDef->bRepeatable ) )
		bRetVal = TRUE;
	return bRetVal;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestOffering(
	QUEST *pQuest)
{

	// set new status
	sQuestSetStatus( pQuest, QUEST_STATUS_OFFERING );
	return TRUE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestClose(
	QUEST *pQuest)
{
	// set new status
	sQuestSetStatus( pQuest, QUEST_STATUS_CLOSED );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestFail(
	QUEST *pQuest)
{
	if (pQuest->eStatus == QUEST_STATUS_ACTIVE)
	{

		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_FAIL );
				
	}
	
	return QuestGetStatus( pQuest ) == QUEST_STATUS_FAIL;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_QuestGetGiver( 
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	
	if (ptQuestDef->nCastGiver != INVALID_LINK)
	{
		SPECIES spGiver = QuestCastGetSpecies( pQuest, ptQuestDef->nCastGiver );
		return s_QuestCastMemberGetInstanceBySpecies( pQuest, spGiver );					
	}
	
	return NULL;	
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_QuestGetRewarder(
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	
	if (ptQuestDef->nCastRewarder != INVALID_LINK)
	{
		SPECIES spRewarder = QuestCastGetSpecies( pQuest, ptQuestDef->nCastRewarder );
		return s_QuestCastMemberGetInstanceBySpecies( pQuest, spRewarder );					
	}
	
	return NULL;	
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventMonsterDying(
	UNIT *pKiller,
	UNIT *pVictim)
{
	ASSERTX_RETURN( pVictim, "Expected victim" );
	ASSERTX_RETURN( UnitGetGenus( pVictim ) == GENUS_MONSTER, "Victim is not a monster" );	
	GAME *pGame = UnitGetGame( pVictim );
	
	// tugboat wants to know about all monsters that die
	const UNIT_DATA *pMonsterData = UnitGetData( pVictim );
	if (UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_INFORM_QUESTS_ON_DEATH))
	{
	
		// setup message
		QUEST_MESSAGE_MONSTER_DYING tMessage;
		tMessage.idKiller = pKiller ? UnitGetId( pKiller ) : INVALID_ID;
		tMessage.idVictim = UnitGetId( pVictim );
		
		// send to quests for all participating players		
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pPlayer = ClientGetPlayerUnit( pClient );
			ASSERTX_CONTINUE( pPlayer, "No player for in game client" );
			MetagameSendMessage( pPlayer, QM_SERVER_MONSTER_DYING, &tMessage );		
		}	
		
	}

}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventMonsterKill(
	UNIT *pKiller,
	UNIT *pVictim)
{
	ASSERTX_RETURN( pVictim, "Invalid victim" );
	ASSERTX_RETURN( UnitGetGenus( pVictim ) == GENUS_MONSTER, "Victim is not a monster" );
	if (!pKiller)
	{
		return;
	}

	// Send a message to each quest of all players in the party of, and in the 
	// same level as, the killer (or the owner of the killer if it is a pet), 
	// if the quest tracks party monster kills.
	UNIT *pPlayer = NULL;
	GAME *pGame = UnitGetGame( pVictim );
	if ( AppIsHellgate())
	{
		if ( UnitDataTestFlag( UnitGetData( pKiller ), UNIT_DATA_FLAG_DESTRUCTIBLE ) )
		{
			UNITID idAttackerUltimate = UnitGetStat( pKiller, STATS_LAST_ULTIMATE_ATTACKER_ID );
			if ( idAttackerUltimate != INVALID_ID )
			{
				pKiller = UnitGetById( pGame, idAttackerUltimate );
				if (!pKiller)
				{
					return;
				}
			}
		}

	}
	if ( UnitIsA( pKiller, UNITTYPE_PLAYER ) )
	{
		pPlayer = pKiller;
	}
	else if ( PetIsPlayerPet( pGame, pKiller ) )
	{
		UNITID idOwner = PetGetOwner( pKiller );
		ASSERTX_RETURN( idOwner != INVALID_ID, "unexpected invalid owner ID for player pet");
		pPlayer = UnitGetById( pGame, idOwner );
	}
	

	if ( pPlayer != NULL )
	{
		PARTYID idParty = INVALID_ID;
		if ( AppIsMultiplayer() )
			idParty = UnitGetPartyId( pPlayer );

		if ( idParty == INVALID_ID )
		{
			// go through each of the killer's quests
			// drb - We should set up a list of active party kill quests and just traverse those here...
			if ( UnitTestFlag( pPlayer, UNITFLAG_QUEST_PARTY_KILL ) )
			{
				int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
				for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
				{
					QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
					if ( !pQuest )
						continue;
					if ( TESTBIT( pQuest->dwQuestFlags, QF_PARTY_KILL ) &&
						 QuestIsActive( pQuest ) )
					{
						QUEST_MESSAGE_PARTY_KILL tMessage;
						tMessage.pKiller = pPlayer;
						tMessage.pVictim = pVictim;
						QuestSendMessageToQuest( pQuest, QM_SERVER_PARTY_KILL, &tMessage );
					}
				}
			}
		}
		else
		{
			LEVEL *pKillerLevel = UnitGetLevel( pPlayer );
			ASSERTX_RETURN( pKillerLevel != NULL, "Unexpected NULL LEVEL* from UNIT" );

			// send a message to each party member's quest which is flagged for the party kill message

			// TODO cmarch: use PARTY_ITER in party.h, when it is implemented? 
			for ( GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
				  pClient;
				  pClient = ClientGetNextInGame( pClient ))
			{
				UNIT *pUnit = ClientGetControlUnit( pClient );
				ASSERTX_RETURN( pUnit != NULL, "Unexpected NULL Control Unit from Client" );
				// must be in the same level, and flagged to receive a party kill message
				if ( UnitTestFlag( pUnit, UNITFLAG_QUEST_PARTY_KILL ) &&
					 UnitGetPartyId( pUnit ) == idParty &&
					 UnitsAreInSameLevel( pUnit, pPlayer ))
				{
					// TODO cmarch: check proximity?
					int nDifficulty = UnitGetStat( pUnit, STATS_DIFFICULTY_CURRENT );
					for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
					{
						QUEST * pQuest = QuestGetQuestByDifficulty( pUnit, nQuest, nDifficulty );
						if ( !pQuest )
							continue;

						if ( TESTBIT( pQuest->dwQuestFlags, QF_PARTY_KILL ) &&
							 QuestIsActive( pQuest ) )
						{
							QUEST_MESSAGE_PARTY_KILL tMessage;
							tMessage.pKiller = pPlayer;
							tMessage.pVictim = pVictim;
							QuestSendMessageToQuest( 
								pQuest, QM_SERVER_PARTY_KILL, &tMessage );
						}
					}
				}
			}
		}

		// go through each of the killer's quests, checking if a giver item
		// should be spawned
		if ( UnitTestFlag( pPlayer, UNITFLAG_QUEST_GIVER_ITEM ) )
		{
			int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
			for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
			{
				QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
				if ( !pQuest )
					continue;
				if ( pQuest->pQuestDef->nGiverItem != INVALID_ID &&
					 s_QuestCanBeActivated( pQuest ) )
				{
					QUEST_MESSAGE_KILL_DROP_GIVER_ITEM tMessage;
					tMessage.pKiller = pPlayer;
					tMessage.pVictim = pVictim;
					QuestSendMessageToQuest( pQuest, QM_SERVER_KILL_DROP_GIVER_ITEM, &tMessage );
				}
			}
		}
	}

	// should I even continue?
	// tugboat wants to know about all monsters that die
	const UNIT_DATA * monster_data = MonsterGetData( UnitGetGame( pVictim ), UnitGetClass( pVictim ) );
	REF( monster_data );
	if ( AppIsHellgate() &&
		 !UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_INFORM_QUESTS_ON_DEATH) )
	{
		return;
	}

	// setup message
	QUEST_MESSAGE_MONSTER_KILL tMessage;
	tMessage.idKiller = UnitGetId( pKiller );
	tMessage.idVictim = UnitGetId( pVictim );
	if( AppIsTugboat() )
	{
		MetagameSendMessage( UnitGetUltimateOwner( pKiller ), QM_SERVER_MONSTER_KILL, &tMessage );			
	}
	else
	{
		// send to quests for all participating players		
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pPlayer = ClientGetPlayerUnit( pClient );
			ASSERTX_CONTINUE( pPlayer, "No player for in game client" );
			MetagameSendMessage( pPlayer, QM_SERVER_MONSTER_KILL, &tMessage );		
		}	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventPlayerDie(
	UNIT * pPlayer,
	UNIT * pKiller)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );
	ASSERTX_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER, "Expected player" );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ), "Server only" );

	// send message
	QUEST_MESSAGE_PLAYER_DIE tData;
	tData.pKiller = pKiller;
	MetagameSendMessage( pPlayer, QM_SERVER_PLAYER_DIE, &tData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventLevelCreated( 
	GAME *pGame, 
	LEVEL *pLevel )
{
	ASSERTX_RETURN( pGame, "Invalid arguments" );
	ASSERTX_RETURN( pLevel, "Expected player" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// setup message
	QUEST_MESSAGE_CREATE_LEVEL tMessage;
	tMessage.pGame = pGame;
	tMessage.pLevel = pLevel;

	// send to all players
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( pPlayer, "No player for in game client" );
		MetagameSendMessage( pPlayer, QM_SERVER_CREATE_LEVEL, &tMessage );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTransitionLevel(
	UNIT* pPlayer,
	LEVEL* pLevelOld,
	LEVEL* pLevelNew)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );
	ASSERTX_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER, "Expected player" );
	ASSERTX_RETURN( pLevelNew, "Expected new level" );

	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	if ( pLevelOld )
		pQuestGlobals->nLevelOldDef = pLevelOld->m_nLevelDef;
	else
		pQuestGlobals->nLevelOldDef = INVALID_ID;
	pQuestGlobals->nLevelNewDef = pLevelNew->m_nLevelDef;
	pQuestGlobals->nLevelNewDRLGDef = pLevelNew->m_nDRLGDef;

	ZeroMemory( pQuestGlobals->dwRoomsActivated, sizeof(pQuestGlobals->dwRoomsActivated) );
	pQuestGlobals->nDefaultSublevelRoomsActivated = 0;

	// set each quests flag to do a level transition update
	// drb - there has got to be a better way of doing this... this seems like an extreme amount of messages
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{		
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if (pQuest)
		{
			SETBIT( pQuest->dwQuestFlags, QF_LEVEL_TRANSITION );
		}
	}
	UnitSetFlag( pPlayer, UNITFLAG_QUESTS_UPDATE, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTransitionSubLevel(
	UNIT *pPlayer,
	LEVEL *pLevel,
	SUBLEVELID idSubLevelOld,
	SUBLEVELID idSubLevelNew,
	ROOM * pOldRoom )
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// setup message
	QUEST_MESSAGE_SUBLEVEL_TRANSITION tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pSubLevelOld = idSubLevelOld != INVALID_ID ? SubLevelGetById( pLevel, idSubLevelOld ) : NULL;
	tMessage.pSubLevelNew = idSubLevelNew != INVALID_ID ? SubLevelGetById( pLevel, idSubLevelNew ) : NULL;
	tMessage.pOldRoom = pOldRoom;
	
	// send message
	MetagameSendMessage( pPlayer, QM_SERVER_SUB_LEVEL_TRANSITION, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventRoomEntered(
	GAME *pGame,
	UNIT *pPlayer,
	ROOM *pRoom)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );

	// setup message
	QUEST_MESSAGE_ENTER_ROOM tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pRoom = pRoom;

	// send message
	MetagameSendMessage( pPlayer, QM_SERVER_ENTER_ROOM, &tMessage );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventRoomActivated( 
	ROOM * pRoom, 
	UNIT * pActivator )
{
	ASSERTX_RETURN( pActivator, "Expected room activator" );

	// Pets might run off and activate rooms? I only care about where the player is moving
	if ( UnitIsA( pActivator, UNITTYPE_PLAYER ) )
	{
		UNIT * pPlayer = pActivator;
		QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );

		if ( !TESTBIT( pQuestGlobals->dwRoomsActivated, pRoom->nIndexInLevel ) )
		{
			SETBIT( pQuestGlobals->dwRoomsActivated, pRoom->nIndexInLevel, TRUE );

			if ( RoomIsInDefaultSubLevel( pRoom ) )
			{
				++pQuestGlobals->nDefaultSublevelRoomsActivated;

				// I only need the message to happen for rooms in the default sublevel for now
				SETBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_ROOMS_ACTIVATED, TRUE );
				UnitSetFlag( pPlayer, UNITFLAG_QUESTS_UPDATE, TRUE );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// this can get called to actually do the triggering, or an update saying something
// has been triggered
void s_QuestEventObjectOperated(
	UNIT *pOperator,
	UNIT *pTarget,
	BOOL bDoTrigger )
{
	ASSERTX_RETURN( pOperator, "Invalid operator" );
	ASSERTX_RETURN( pTarget, "Invalid target" );
	ASSERTX_RETURN( UnitGetGenus( pTarget ) == GENUS_OBJECT, "Target is not an object" );

	// setup message
	QUEST_MESSAGE_OBJECT_OPERATED tMessage;
	tMessage.pOperator = pOperator;
	tMessage.pTarget = pTarget;
	tMessage.bDoTrigger = bDoTrigger;
		
	MetagameSendMessage( pOperator, QM_SERVER_OBJECT_OPERATED, &tMessage );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventLootSpawned(
	UNIT *pSpawner,
	ITEMSPAWN_RESULT *pLoot)
{
	ASSERTX_RETURN( pSpawner, "Invalid spawner" );
	ASSERTX_RETURN( pLoot, "Invalid loot" );

	// setup message
	QUEST_MESSAGE_LOOT tMessage;
	tMessage.pSpawer = pSpawner;
	tMessage.pLoot = pLoot;
		
	// send to all players
	GAME *pGame = UnitGetGame( pSpawner );
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( pPlayer, "No player for in game client" );
		MetagameSendMessage( pPlayer, QM_SERVER_LOOT, &tMessage );	
	}	
	
}

//----------------------------------------------------------------------------
// we want to make sure we can pick up certain items.
//----------------------------------------------------------------------------
BOOL s_QuestEventCanPickup(
	UNIT *pPlayer,
	UNIT *pAttemptingToPickedUp )
{

	ASSERTX_RETFALSE( pPlayer , "Expected player" );
	ASSERTX_RETFALSE( pAttemptingToPickedUp, "Invalid picked up unit" );
	if( UnitIsA( pPlayer, UNITTYPE_PLAYER ) == FALSE )
		return TRUE;
	// setup message
	QUEST_MESSAGE_ATTEMPTING_PICKUP tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pPickedUp = pAttemptingToPickedUp;
	tMessage.nQuestIndex = UnitGetStat( pAttemptingToPickedUp, STATS_QUEST_ITEM, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) );
		
	// send to quests
	QUEST_MESSAGE_RESULT result = MetagameSendMessage( pPlayer, QM_SERVER_ATTEMPTING_PICKUP, &tMessage );
	return result == QMR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTutorialPickup(
	UNIT * pPlayer,
	UNIT * pPickedUp)
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pPickedUp );
	QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_TUTORIAL );
	if (!pQuest)
		return;

	// setup message
	QUEST_MESSAGE_PICKUP tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pPickedUp = pPickedUp;

	// send it
	QuestSendMessageToQuest( pQuest, QM_SERVER_PICKUP, &tMessage );		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventPickup(
	UNIT *pPlayer,
	UNIT *pPickedUp)
{
	ASSERTX_RETURN( pPlayer && UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );
	ASSERTX_RETURN( pPickedUp, "Invalid picked up unit" );
	

	//tugboat wants to know about all items picked up!
	if ( AppIsHellgate() )
	{
		// is this the init new player?
		if ( !pPlayer->m_pQuestGlobals )
			return;

		// send all pickups to the tutorial
		if ( UnitIsInTutorial( pPlayer ) )
		{
			sSendTutorialPickup( pPlayer, pPickedUp );
			return;
		}

		// does this item need/want to be sent?
		if ( !UnitDataTestFlag( pPickedUp->pUnitData, UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP ) )
			return;
	}

	// setup message
	QUEST_MESSAGE_PICKUP tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pPickedUp = pPickedUp;
		
	// send to quests
	MetagameSendMessage( pPlayer, QM_SERVER_PICKUP, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestEventDroppedItem(
	QUEST *pQuest,
	UNIT *pPlayer,
	int nItemClass)
{
	ASSERTX_RETURN( pPlayer && UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );
	ASSERTX_RETURN( nItemClass != INVALID_ID, "Invalid item class to drop" );


	//tugboat wants to know about all items picked up!
	if ( AppIsHellgate() )
	{
		// is this the init new player?
		if ( !pPlayer->m_pQuestGlobals )
			return;
	}

	// setup message
	QUEST_MESSAGE_DROPPED_ITEM tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.nItemClass = nItemClass;

	// send to quests
	QuestSendMessageToQuest( pQuest, QM_SERVER_DROPPED_ITEM, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventItemCrafted(
	UNIT *pPlayer,
	UNIT *pCrafted)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));
	ASSERTX_RETURN( pCrafted, "Expected unit" );
	ASSERTV_RETURN( UnitIsA( pCrafted, UNITTYPE_ITEM ), "Expected item, got '%s'", UnitGetDevName( pCrafted ));
	
	// setup message
	QUEST_MESSAGE_ITEM_CRAFTED tMessage;
	tMessage.idItemCrafted = UnitGetId( pCrafted );
		
	// send to quests
	MetagameSendMessage( pPlayer, QM_SERVER_ITEM_CRAFTED, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventApproachPlayer(
	UNIT *pPlayer,
	UNIT *pTarget)
{
	ASSERTX_RETURN( pPlayer, "Invalid player" );
	ASSERTX_RETURN( pTarget, "Invalid target" );

	if ( UnitIsGhost( pPlayer ) )
		return;

	// setup message
	QUEST_MESSAGE_PLAYER_APPROACH tMessage;
	tMessage.idPlayer = UnitGetId( pPlayer );
	tMessage.idTarget = UnitGetId( pTarget );
		
	// send to quests
	MetagameSendMessage( pPlayer, QM_SERVER_PLAYER_APPROACH, &tMessage, MAKE_MASK( QSMF_APPROACH_RADIUS ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventUnitStateChange(
	UNIT * pPlayer,
	int nState,
	BOOL bStateSet )
{
	ASSERTX_RETURN( pPlayer, "Invalid player" );

	QUEST_MESSAGE_UNIT_STATE_CHANGE tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.nState = nState;
	tMessage.bStateSet = bStateSet;

	// send to quests
	MetagameSendMessage( pPlayer, QM_SERVER_UNIT_STATE_CHANGE, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventClientQuestStateChange(
	int nQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	UNIT *pPlayer)
{

	// setup message	
	QUEST_MESSAGE_STATE tMessage;
	tMessage.nQuestState = nQuestState;
	tMessage.eValue = eValue;
	tMessage.pPlayer = pPlayer;

	// send to server quest
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if (pQuest)
	{
		QuestSendMessageToQuest( pQuest, QM_SERVER_QUEST_STATE_CHANGE_ON_CLIENT, &tMessage );
	}
		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestGossipInteract( UNIT * pPlayer, UNIT * pNPC, BOOL bSend = FALSE )
{
	ASSERT_RETVAL( pPlayer, QMR_INVALID );
	ASSERT_RETVAL( pNPC, QMR_INVALID );

	if ( QuestsAreDisabled( pPlayer ) )
		return QMR_OK;

	// does this even apply?
	if ( UnitGetGenus( pNPC ) != GENUS_MONSTER )
		return QMR_OK;

	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETVAL( pQuestGlobals, QMR_INVALID );

	// go through each active story quest
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if ( !pQuest )
			continue;

		if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
			continue;

		if ( pQuest->pQuestDef->eStyle != QS_STORY )
			continue;

		if ( TESTBIT( pQuest->dwQuestFlags, QF_GOSSIP ) )
		{
			QUEST_STATE * pState = sQuestFindLastActiveState( pQuest );
			if ( !pState )
				continue;

			const QUEST_STATE_DEFINITION * pStateDef = QuestStateGetDefinition( pState->nQuestState );
			if ( !pStateDef )
				continue;

			int nClass = UnitGetClass( pNPC );
			for ( int i = 0; i < MAX_QUEST_GOSSIPS; i++ )
			{
				if ( pStateDef->nGossipNPC[i] == nClass )
				{
					if ( bSend && !pState->bGossipComplete[i] )
					{
						// send the quest info the client, so dialog strings can be generated and displayed
						// init message
						MSG_SCMD_QUEST_DISPLAY_DIALOG tMessage;
						tMessage.nQuest = pQuest->nQuest;
						tMessage.idQuestGiver = UnitGetId( pNPC );
						tMessage.nGossipString = pStateDef->nGossipString[i];
						tMessage.nDialogType = NPC_DIALOG_OK_CANCEL;

						// send it
						GAMECLIENTID idClient = UnitGetClientId( pPlayer );
						GAME *pGame = UnitGetGame( pPlayer );
						s_SendMessage( pGame, idClient, SCMD_QUEST_DISPLAY_DIALOG, &tMessage );	
					}
					return QMR_NPC_STORY_GOSSIP;
				}
			}
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT_INTERACT s_QuestEventInteract(
	UNIT *pPlayer,
	UNIT *pSubject,
	int nQuestID )
{
	ASSERTX_RETVAL( pPlayer, UNIT_INTERACT_NONE, "Invalid unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), UNIT_INTERACT_NONE, "Invalid player" );
	ASSERTX_RETVAL( pSubject, UNIT_INTERACT_NONE, "Invalid interact subject" );
	
	QUEST_MESSAGE_RESULT eResult = QMR_INVALID;
	ASSERTX_RETVAL( UnitCanInteractWith( pSubject, pPlayer ), UNIT_INTERACT_NONE, "Unit cannot interact with player" ); 

	//Send Quest message
	// setup message
	QUEST_MESSAGE_INTERACT tMessage;
	tMessage.idPlayer = UnitGetId( pPlayer );
	tMessage.idSubject = UnitGetId( pSubject );
	tMessage.nForcedQuestID = nQuestID;
	
	// send a message to each quest
	eResult = MetagameSendMessage( pPlayer, QM_SERVER_INTERACT, &tMessage );
		
	if (eResult == QMR_NPC_TALK)
	{
		return UNIT_INTERACT_TALK;
	}
	else if (eResult == QMR_OFFERING)
	{
		return UNIT_INTERACT_TRADE;
	}
	else if (eResult == QMR_NPC_OPERATED)
	{
		return UNIT_INTERACT_OPERATED;
	}
	else
	{
		// if the quests didn't have anything to say,
		// check if this is a new story gossip thing...
		if ( AppIsHellgate() )
		{
			eResult = sQuestGossipInteract( pPlayer, pSubject );
			if ( eResult == QMR_NPC_STORY_GOSSIP )
				return UNIT_INTERACT_STORY_GOSSIP;
		}

		return UNIT_INTERACT_NONE;
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventInteractGeneric(
	UNIT *pPlayer,
	UNIT *pSubject,
	UNIT_INTERACT eInteract,
	int nQuestID )
{
	ASSERTX_RETURN( pPlayer, "Invalid unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Invalid player" );
	ASSERTX_RETURN( pSubject, "Invalid interact subject" );

	QUEST_MESSAGE_RESULT eResult = QMR_INVALID;

	if ( AppIsHellgate() && ( eInteract == UNIT_INTERACT_STORY_GOSSIP ) )
	{
		sQuestGossipInteract( pPlayer, pSubject, TRUE );
		return;
	}

	//Send Quest message
	// setup message
	QUEST_MESSAGE_INTERACT_GENERIC tMessage;
	tMessage.idPlayer = UnitGetId( pPlayer );
	tMessage.idSubject = UnitGetId( pSubject );
	tMessage.eInteract = eInteract;
	tMessage.nForcedQuestID = nQuestID;

	// send a message to each quest
	eResult = MetagameSendMessage( pPlayer, QM_SERVER_INTERACT_GENERIC, &tMessage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAccept(
	GAME* pGame,
	UNIT* pPlayer,
	UNIT* pNPC,
	int nQuestID )
{
	//first check for quest
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask( NULL );// = QuestNPCHasQuestTask( pGame, pPlayer, pNPC );		
	pQuestTask = QuestGetActiveTask( pPlayer, nQuestID );
	//if( !pQuestTask &&
//		UnitIsGodMessanger( pNPC ) )
//	{
//		pQuestTask = QuestGetActiveTask( pPlayer, nQuestID );
//	}
	if( pQuestTask )
	{		
		// ok we should have a quest
		BOOL bComplete;
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestDefinition = s_QuestUpdate( pGame, pPlayer, pNPC, pQuestTask->nParentQuest, bComplete );	
		if( pQuestDefinition )
		{
			if( QuestGetQuestAccepted( pGame, pPlayer, pQuestDefinition ) == false )
			{
				////////////////////////////////////
				//I need these to happen after the server has recieved the OK button to accept. I have moved
				//this to the Quest Event player stopped talking.
				//s_QuestAccepted( pGame, pPlayer, pNPC, pQuestDefinition );
				//s_QuestEventInteract( pPlayer, pNPC, nQuestID );
				/////////////////////////////////////

			}
			else
			{
				// this quest has been visited		
				//s_QuestSetVisited( pPlayer, pQuestDefinition  );
			}
		}
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestDialogMakeRewardItemsAvailableToClient(
	QUEST *pQuest,
	BOOL bMakeAvailable )
{
	ASSERT_RETURN(pQuest);
	int nOffer = pQuest->pQuestDef->nOfferReward;
	if (nOffer == INVALID_LINK)
		return;

	UNIT *pUnit = QuestGetPlayer( pQuest );

	int nSourceLocation = INVALID_ID;		
	int nDestinationLocation = INVALID_ID;
	if ( bMakeAvailable )
	{
		nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );		
		nDestinationLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
	}
	else
	{
		nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
		nDestinationLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );		
	}

	int nDifficulty = UnitGetStat( QuestGetPlayer( pQuest ), STATS_DIFFICULTY_CURRENT );

	UNIT * pItem = NULL;
	pItem = UnitInventoryLocationIterate( pUnit, nSourceLocation, pItem, 0, FALSE );
	while (pItem != NULL)
	{
		UNIT * pItemNext = UnitInventoryLocationIterate( pUnit, nSourceLocation, pItem, 0, FALSE );
		if (UnitGetStat( pItem, STATS_OFFERID, nDifficulty ) == nOffer)
		{
			// find a place for the item
			int x, y;
			if (UnitInventoryFindSpace( pUnit, pItem, nDestinationLocation, &x, &y ))
			{
				// move the item to the reward inventory
				InventoryChangeLocation( pUnit, pItem, nDestinationLocation, x, y, 0 );
			}
			else
			{
				ASSERTX( 0, "Failed to change quest dialog reward item location" );
			}
		}
		pItem = pItemNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGossipDone(
	UNIT *pPlayer,
	UNIT *pNPC )
{
	ASSERT_RETFALSE( AppIsHellgate() );
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pNPC );
	
	// only monster NPCs
	if ( UnitGetGenus( pNPC ) != GENUS_MONSTER )
		return FALSE;

	int nClass = UnitGetClass( pNPC );

	// search all quests for completed gossip
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if ( !pQuest )
			continue;

		// only for active quests
		if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
			continue;

		// get deepest active state
		QUEST_STATE * pState = sQuestFindLastActiveState( pQuest );
		if ( !pState )
			continue;

		const QUEST_STATE_DEFINITION * pStateDef = QuestStateGetDefinition( pState->nQuestState );
		if ( !pStateDef )
			continue;

		// go through all (3) gossips
		for ( int i = 0; i < MAX_QUEST_GOSSIPS; i++ )
		{
			// npc column filled out?
			if ( pStateDef->nGossipNPC[i] == nClass )
			{
				pState->bGossipComplete[i] = TRUE;
				s_QuestUpdateCastInfo( pQuest );
				return TRUE;
			}
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTalkStopped(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog,
	int nRewardSelection)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pTalkingTo, "Expected talking to" );

	if ( AppIsHellgate() && ( nDialog == INVALID_LINK ) )
	{
		// check if this was a gossip
		if ( sGossipDone( pPlayer, pTalkingTo ) )
			return;
	}

	QUEST_MESSAGE_TALK_STOPPED tMessage;
	tMessage.idTalkingTo = UnitGetId( pTalkingTo );
	tMessage.nDialog = nDialog;
	tMessage.nRewardSelection = nRewardSelection;

	
	MetagameSendMessage( pPlayer, QM_SERVER_TALK_STOPPED, &tMessage );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTalkCancelled(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pTalkingTo, "Expected talking to" );

	s_OfferCancel( pPlayer );

	// when canceling offering status, go back to active
	if( AppIsHellgate() )
	{
		int nQuestID = UnitGetStat( pPlayer, STATS_QUEST_CURRENT );	
		if ( nQuestID != INVALID_ID )
		{
			QUEST *pQuest = QuestGetQuest( pPlayer, nQuestID );
			if ( pQuest && QuestIsOffering( pQuest ) )
			{
				sQuestSetStatus( pQuest, QUEST_STATUS_ACTIVE );
			}
		}
	}

	QUEST_MESSAGE_TALK_CANCELLED tMessage;
	tMessage.idTalkingTo = UnitGetId( pTalkingTo );
	tMessage.nDialog = nDialog;


	MetagameSendMessage( pPlayer, QM_SERVER_TALK_CANCELLED, &tMessage );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventDialogClosed(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pTalkingTo, "Expected talking to" );

	
	if( AppIsTugboat() )
	{
		// make quest reward offers available to them
		s_QuestDialogMakeRewardItemsAvailableToClient( pPlayer, NULL, FALSE );
	}
	else // move any items from quest dialog inventory back to offer inventory
	{
		int nQuest = UnitGetStat( pPlayer, STATS_QUEST_CURRENT );	
		if ( nQuest != INVALID_LINK )
		{
			QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
			if (pQuest)
				s_sQuestDialogMakeRewardItemsAvailableToClient( pQuest, FALSE );
		}
	}




	UnitSetStat( pPlayer, STATS_QUEST_CURRENT, INVALID_LINK );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTipStopped(
	UNIT *pPlayer,
	int nTip )
{
	ASSERTX_RETURN( pPlayer, "Expected player" );

	QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_TUTORIAL );

	QUEST_MESSAGE_TIP_STOPPED tMessage;
	tMessage.nTip = nTip;

	QuestSendMessageToQuest( pQuest, QM_SERVER_TIP_STOPPED, &tMessage );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventStatus( 
	QUEST *pQuest, 
	QUEST_STATUS eStatusOld )
{
	QUEST_MESSAGE_STATUS tMessage;
	tMessage.eStatusOld = eStatusOld;

	// send it only to this quest (we could send to others later if we want)
	QuestSendMessageToQuest( pQuest, QM_SERVER_QUEST_STATUS, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventActivated(
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	QuestSendMessageToQuest( pQuest, QM_SERVER_QUEST_ACTIVATED, NULL );	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventTogglePortal( 
	UNIT * pPlayer, 
	UNIT * pPortal )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pPortal );
	GAME * pGame = UnitGetGame( pPlayer );
	LEVEL *pLevelDest = LevelGetByID( pGame, WarpGetDestinationLevelID( pPortal ) );
	if ( pLevelDest )
	{
		QUEST_MESSAGE_TOGGLE_PORTAL tMessage;
		tMessage.pPortal = pPortal;
		tMessage.nLevelDefDestination = LevelGetDefinitionIndex( pLevelDest );
		MetagameSendMessage( pPlayer, QM_SERVER_TOGGLE_PORTAL, &tMessage );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static INTERACT_INFO sQuestNPCResultToNPCInfo(
	QUEST_MESSAGE_RESULT eResult)
{
	switch (eResult)
	{
		case QMR_NPC_INFO_UNAVAILABLE:	return INTERACT_INFO_UNAVAILABLE;break;
		case QMR_NPC_INFO_NEW:			return INTERACT_INFO_NEW;		break;
		case QMR_NPC_INFO_WAITING:		return INTERACT_INFO_WAITING;	break;
		case QMR_NPC_INFO_RETURN:		return INTERACT_INFO_RETURN;	break;
		case QMR_NPC_INFO_MORE:			return INTERACT_INFO_MORE;		break;
		case QMR_NPC_STORY_NEW:			return INTERACT_STORY_NEW;		break;
		case QMR_NPC_STORY_RETURN:		return INTERACT_STORY_RETURN;	break;
		case QMR_NPC_STORY_GOSSIP:		return INTERACT_STORY_GOSSIP;	break;
		default:						return INTERACT_INFO_NONE;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INTERACT_INFO s_QuestsGetInteractInfo(
	UNIT *pPlayer,
	UNIT *pSubject)
{

	if(AppIsTugboat())
	{
		// I guess that tugboat doesn't do quests?
		return INTERACT_INFO_NONE;
	}
	ASSERT_RETVAL( pPlayer, INTERACT_INFO_NONE );
	ASSERT_RETVAL( pSubject, INTERACT_INFO_NONE );
	ASSERT_RETVAL( pSubject->pUnitData, INTERACT_INFO_NONE );

	if ( UnitDataTestFlag( pSubject->pUnitData, UNIT_DATA_FLAG_QUESTS_CAST_MEMBER ) == FALSE )
	{
		return INTERACT_INFO_NONE;
	}
	
	// setup message
	QUEST_MESSAGE_INTERACT_INFO tMessage;
	tMessage.idSubject = UnitGetId( pSubject );
	
	// pass message to quests
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETVAL(pQuestGlobals, INTERACT_INFO_NONE);
	
	// go through and ask each quest, saving the highest priority answer
	INTERACT_INFO eInfoHighestPriority = INTERACT_INFO_NONE;
	int nHighestPriority = NONE;

	// search all quests for update info
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		
		// check pre-reqs
		if ( pQuest )
		{
			// send it
			QUEST_MESSAGE_RESULT eResult = QuestSendMessageToQuest( pQuest, QM_SERVER_INTERACT_INFO, &tMessage );
			// gossip
			if ( AppIsHellgate() && ( ( eResult == QMR_OK ) || ( eResult == QMR_NPC_INFO_WAITING ) ) )
			{
				QUEST_MESSAGE_RESULT eGossipResult = sGossipInfoUpdate( pQuest, pSubject );
				if ( eResult == QMR_OK )
					eResult = eGossipResult;
				else if ( eGossipResult != QMR_OK )
					eResult = eGossipResult;
			}
			INTERACT_INFO eInfo = sQuestNPCResultToNPCInfo( eResult );		
			// Discard "new quest available" info, if the quest prereqs aren't done.
			// Other info may still be valid, as in odd cases where the player
			// abandoned the prereqs after starting a quest.
			if ( ( eInfo != INTERACT_INFO_NEW && eInfo != INTERACT_STORY_NEW && ( QuestIsActive( pQuest ) || QuestIsOffering( pQuest ) ) ) ||
				 ( eInfo == INTERACT_INFO_UNAVAILABLE && pQuest->pQuestDef->nUnavailableDialog != INVALID_LINK ) ||
				 s_QuestIsPrereqComplete( pQuest ) )
			{
				int nPriority = s_NPCInfoPriority( eInfo );
				if ( nPriority > nHighestPriority )
				{
					nHighestPriority = nPriority;
					eInfoHighestPriority = eInfo;
				}
			}
		}
	}

	
	return eInfoHighestPriority;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT s_QuestsUseItem(
	UNIT * pPlayer,
	UNIT * pItem )
{
	ASSERTX_RETVAL( pPlayer, USE_RESULT_INVALID, "Invalid Operator" );
	ASSERTX_RETVAL( UnitGetGenus( pPlayer ) == GENUS_PLAYER, USE_RESULT_INVALID, "Operator is not a player" );
	ASSERTX_RETVAL( pItem, USE_RESULT_INVALID, "Invalid Target" );
	ASSERTX_RETVAL( UnitGetGenus( pItem ) == GENUS_ITEM, USE_RESULT_INVALID, "Target is not an item" );
	const UNIT_DATA * data = ItemGetData( pItem );
	ASSERTX_RETVAL( UnitDataTestFlag(data, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE), USE_RESULT_INVALID, "Item is not quest usable" );
	ASSERTX_RETVAL( IS_SERVER( UnitGetGame( pPlayer ) ), USE_RESULT_INVALID, "Server only" );

	// must be able to use
	if (QuestsCanUseItem( pPlayer, pItem ) == USE_RESULT_OK)
	{
	
		// setup message
		QUEST_MESSAGE_USE_ITEM tMessage;
		tMessage.idPlayer = UnitGetId( pPlayer );
		tMessage.idItem = UnitGetId( pItem );

		// send to quests
		QUEST_MESSAGE_RESULT eResult = MetagameSendMessage( pPlayer, QM_SERVER_USE_ITEM, &tMessage );
		if (eResult == QMR_OK )
		{
			return USE_RESULT_OK;
		}

	}
	
	return USE_RESULT_CONDITION_FAILED;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestCanBeActivated(
	QUEST *pQuest)
{
	return QuestIsInactive( pQuest ) && s_QuestIsPrereqComplete( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestSendKnownQuestsToClient( 
	QUEST *pQuest)
{

	// we only care about quests that are active or completed
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if (eStatus != QUEST_STATUS_INACTIVE)
	{
	
		// send status
		sQuestSendStatus( pQuest, TRUE );
		
		// send all states to player
		for (int i = 0; i < pQuest->nNumStates; ++i)
		{
			QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
			QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
			if (eStateValue == QUEST_STATE_ACTIVE || eStateValue == QUEST_STATE_COMPLETE)
			{
				QuestSendState( pQuest, pState->nQuestState, TRUE );
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestSendKnownQuestsToClient( 
	UNIT *pPlayer)
{
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if (pQuest)
			s_sQuestSendKnownQuestsToClient( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestRemoveAllOldQuestItems(
	UNIT * pPlayer )
{
	ASSERT_RETURN( pPlayer );

	// go through each quest
	int nCurrentDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for ( int i = 0; i < NUM_QUESTS; i++ )
	{
		BOOL bComplete = FALSE;
		BOOL bActive = FALSE;
		for ( int nDifficulty = DIFFICULTY_NORMAL; nDifficulty < NUM_DIFFICULTIES; nDifficulty++ )
		{
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, i, nDifficulty, FALSE );
			if ( pQuest )
			{
				QUEST_STATUS eStatus = QuestGetStatus( pQuest );
				if ( eStatus >= QUEST_STATUS_COMPLETE )
					bComplete = TRUE;
				if ( eStatus == QUEST_STATUS_ACTIVE )
					bActive = TRUE;
			}
		}
		// remove all old quest items
		if ( bComplete && !bActive )
		{
			QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, i, nCurrentDifficulty, FALSE );
			ASSERT_CONTINUE( pQuest );
			const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;
			ASSERT_CONTINUE( pQuestDef );

			for( int i = 0; i < MAX_QUEST_COMPLETE_ITEMS; i++ )
			{
				if( pQuestDef->nCompleteItemList[i] != INVALID_ID )
				{
					s_QuestRemoveItem( pPlayer, pQuestDef->nCompleteItemList[i] );
				}
			}
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestFixFawkes(
	UNIT * pPlayer )
{
	ASSERT_RETURN( pPlayer );

	// check if they completed Guy Fawkes quest 3
	BOOL bComplete = FALSE;
	for ( int nDifficulty = DIFFICULTY_NORMAL; nDifficulty < NUM_DIFFICULTIES; nDifficulty++ )
	{
		QUEST * pQuest = QuestGetQuestByDifficulty( pPlayer, QUEST_GFN3, nDifficulty, FALSE );
		if ( pQuest )
		{
			QUEST_STATUS eStatus = QuestGetStatus( pQuest );
			if ( eStatus >= QUEST_STATUS_COMPLETE )
				bComplete = TRUE;
		}
	}

	if ( !bComplete )
		return;

	// check inventory for FF5 blueprint
	DWORD dwFindFirstFlags = 0;
	SETBIT( dwFindFirstFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
	if ( s_QuestFindFirstItemOfType( pPlayer, GlobalIndexGet( GI_ITEM_FAWKES_FORCE_FIVE_BLUEPRINT ), dwFindFirstFlags ) != NULL )
		return;

	// check inventory for FF5 item
	int nFawkesForceFive = GlobalIndexGet( GI_ITEM_FAWKES_FORCE_FIVE );
	if ( s_QuestFindFirstItemOfType( pPlayer, nFawkesForceFive, dwFindFirstFlags ) != NULL )
		return;

	QUEST * pQuest = QuestGetQuest( pPlayer, QUEST_GFN3, FALSE );
	if ( pQuest )
	{
		DWORD dwQuestGiveItemFlags = 0;
		SETBIT( dwQuestGiveItemFlags, QGIF_IGNORE_GLOBAL_THEME_RULES_BIT );
		s_QuestGiveItem( pQuest, GI_TREASURE_FAWKES_FORCE_FIVE, dwQuestGiveItemFlags );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestDoSetup(
	UNIT *pPlayer,
	BOOL bFirstGameJoin)
{
	BOOL bSuccess = TRUE;
	
	if(AppIsTugboat())
	{
		// I guess that tugboat doesn't do quests?
		return bSuccess;
	}

	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETFALSE(pQuestGlobals);

	// quest globals for this player have now being setup
	SETBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_QUESTS_SETUP );
	
	// if the questsn't haven't been initialized to the save state of the first player
	// to join the server, do so now
	if ( pQuestGlobals->bQuestsRestored == FALSE )
	{
	
		// restore the state of quests on this player from the data in their save file
		if (s_QuestLoadFromStats( pPlayer, bFirstGameJoin ) == FALSE)
		{
			// we will flag this, but we won't bail out and invalidate the character file
			bSuccess = FALSE;
		}
		
		// now that quests have been restored to their saved state, see if
		// this game server wants to cheat us forward if necessary
		s_QuestCompleteToStart( pPlayer );

		// mark the quests as restored, not that it is important to do this after the we have
		// done the complete to start cheat so that it doesn't try to send any messages to clients
		pQuestGlobals->bQuestsRestored = TRUE;
				
	}

	return bSuccess;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestPrereqForceComplete( 
	QUEST *pQuest )
{
	ASSERTX_RETURN( pQuest->nQuest != INVALID_LINK, "Invalid quest" );
	const QUEST_DEFINITION *ptQuestDef = pQuest->pQuestDef;
	UNIT *pPlayer = pQuest->pPlayer;
	
	// do this quests prereqs too	
	for (int i = 0; i < arrsize( ptQuestDef->nQuestPrereqs ); ++i)
		if (ptQuestDef->nQuestPrereqs[i] != INVALID_LINK)
		{
			QUEST *pQuestPrereq = QuestGetQuest( pPlayer, ptQuestDef->nQuestPrereqs[i] );
			if (pQuestPrereq)
				sQuestPrereqForceComplete( pQuestPrereq );
		}

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
		return;
	
	// complete all states
	s_QuestForceStatesComplete( pQuest->pPlayer, pQuest->nQuest, TRUE );

	// force to complete status if not set
	if (pQuest->eStatus != QUEST_STATUS_COMPLETE)
	{
		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_COMPLETE );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// used mainly for testing... if the starting quest isn't normal, forces all
// pre-req quests to be completed
void s_QuestCompleteToStart(
	UNIT *pPlayer)
{
	BOOL bCheatLevels = FALSE;

#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) 
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( ( pGlobals->dwFlags & GLOBAL_FLAG_CHEAT_LEVELS ) != 0 )
		bCheatLevels = TRUE;
#endif

	if ( !bCheatLevels )
		return;

	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN( pQuestGlobals );
		
	// find starting quest
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{		
		const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
		if ( ptQuestDef && ptQuestDef->bStartingQuestCheat && ( ptQuestDef->eStyle == QS_STORY ) )
		{
			for (int i = 0; i < arrsize( ptQuestDef->nQuestPrereqs ); ++i)
			{
				int nQuestPrereq = ptQuestDef->nQuestPrereqs[i];
				if (nQuestPrereq != INVALID_LINK)
				{
					// complete the prereq quest (and all it's prereqs)			
					QUEST *pQuestPrereq = QuestGetQuest( pPlayer, nQuestPrereq );
					if (pQuestPrereq)
						sQuestPrereqForceComplete( pQuestPrereq );
				}			
			}
		}

#if ISVERSION(CHEATS_ENABLED)
		if ( ptQuestDef && ptQuestDef->bQuestCheatCompleted && ( ptQuestDef->eStyle == QS_FACTION ) )
		{
			QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
			if (QuestGetStatus( pQuest ) < QUEST_STATUS_COMPLETE)
			{
				s_QuestForceComplete( pPlayer, nQuest );
			}
		}
#endif
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestPlayerJoinGame(
	UNIT *pPlayer)
{
	if(AppIsHellgate())
	{
		// skip tutorial if marked. everyone will know about the player now...
/*		if ( UnitGetStat( pPlayer, STATS_TUTORIAL, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) )
		{
			QUEST * pTutorial = QuestGetQuest( pPlayer, QUEST_TUTORIAL );
			if ( !pTutorial )
				return;
			if ( QuestGetStatus( pTutorial ) != QUEST_STATE_COMPLETE )
			{
				s_QuestSkipTutorial( pPlayer );
			}
		}*/

		// send all the active or completed quests to the newly joined player
		s_QuestSendKnownQuestsToClient( pPlayer );

		// tell this client that all info has been loaded and send to them
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		MSG_SCMD_END_QUEST_SETUP tMessage;	
		s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_END_QUEST_SETUP, &tMessage );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestPlayerLeaveGame(
	UNIT *pPlayer)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestClearAllStates(
	QUEST *pQuest,
	BOOL bSendMessage )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	// set all states to beginning values
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		QuestStateInit( pState, pState->nQuestState );
	}
	
	if ( bSendMessage )
	{
		// setup state message
		MSG_SCMD_QUEST_CLEAR_STATES tMessage;
		tMessage.nQuest = pQuest->nQuest;
		// send to single client
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		s_SendMessage( pGame, idClient, SCMD_QUEST_CLEAR_STATES, &tMessage );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestOpenWarp(
	GAME * pGame,
	UNIT * pPlayer,
	int nWarpClass )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERT_RETURN( pPlayer );
	UNIT *pWarp = UnitGetFirstByClassSearchAll( pGame, GENUS_OBJECT, nWarpClass );
	if (pWarp != NULL)
	{
		MSG_SCMD_BLOCKING_STATE msg;
		msg.idUnit = UnitGetId( pWarp );
		msg.nState = BSV_OPEN;
		s_SendMessage( pGame, UnitGetClientId( pPlayer ), SCMD_BLOCKING_STATE, &msg );
		s_QuestEventTogglePortal( pPlayer, pWarp );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestCloseWarp(
	GAME * pGame,
	UNIT * pPlayer,
	int nWarpClass )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERT_RETURN( pPlayer );
	UNIT *pWarp = UnitGetFirstByClassSearchAll( pGame, GENUS_OBJECT, nWarpClass );
	if (pWarp != NULL)
	{
		MSG_SCMD_BLOCKING_STATE msg;
		msg.idUnit = UnitGetId( pWarp );
		msg.nState = BSV_BLOCKING;
		s_SendMessage( pGame, UnitGetClientId( pPlayer ), SCMD_BLOCKING_STATE, &msg );
		s_QuestEventTogglePortal( pPlayer, pWarp );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAbortCurrent( UNIT * player )
{
	ASSERT_RETURN( player );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( player ) ) );

	// setup message
	QUEST_MESSAGE_ABORT tMessage;
	tMessage.pPlayer = player;

	// send to quests
	MetagameSendMessage( player, QM_SERVER_ABORT, &tMessage );			
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestSkipTutorial(
	UNIT * pPlayer )
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	
	QUEST *pQuest = QuestGetQuest( pPlayer, QUEST_TUTORIAL );
	if ( !pQuest )
	{
		return;
	}

	// complete all states
	s_QuestForceStatesComplete( pPlayer, QUEST_TUTORIAL, TRUE );
		
	// force to complete status if not set
	if (pQuest->eStatus != QUEST_STATUS_COMPLETE)
	{
		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_COMPLETE );	
	}
		
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestVideoToAll(
	QUEST * pQuest,
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog)
{
	// Set a state for everyone in the game
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame =QuestGetGame( pQuest );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	if (AppIsSinglePlayer() || UnitGetPartyId( pPlayer ) == INVALID_ID)
	{
		if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
		{
			s_NPCVideoStart( pGame, pPlayer, eNPC, eType, nDialog );
		}
	}
	else
	{
		PARTYID idParty = UnitGetPartyId( pPlayer );
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pUnit = ClientGetControlUnit( pClient );
			if (UnitGetPartyId( pUnit ) == idParty)
			{
				QUEST *pQuestOther = QuestGetQuest( pUnit, pQuest->nQuest );
				if (pQuestOther && QuestIsActive( pQuestOther ))
				{
					s_NPCVideoStart( pGame, pUnit, eNPC, eType, nDialog );
				}
			}
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb - this will need to be fixed. no tech yet...
void s_QuestVideoFromClient(
	UNIT * player,
	int nQuest,
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog)
{
	// check state of quest for everyone
	GAME * game = UnitGetGame( player );
	ASSERT_RETURN( IS_SERVER( game ) );

	QUEST * pQuest = QuestGetQuest( player, nQuest );
	if ( !pQuest )
		return;

	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{
		s_NPCVideoStart( game, player, eNPC, eType, nDialog );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventAboutToLeaveLevel(
	UNIT * pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid Unit" );
	if ( UnitGetGenus( pPlayer ) != GENUS_PLAYER )
		return;
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ), "Server only" );

	// send to metagames
	QUEST_MESSAGE_RESULT eResult = MetagameSendMessage( pPlayer, QM_SERVER_ABOUT_TO_LEAVE_LEVEL, NULL );
	ASSERT( eResult == QMR_OK )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEventLeaveLevel(
	UNIT * pPlayer,
	LEVEL * pLevelDest )
{
	ASSERTX_RETURN( pPlayer, "Invalid Unit" );
	ASSERTX_RETURN( pLevelDest, "Invalid Level" );
	if ( UnitGetGenus( pPlayer ) != GENUS_PLAYER )
		return;
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ), "Server only" );

	// setup message
	QUEST_MESSAGE_LEAVE_LEVEL tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.nLevelNextDef = pLevelDest->m_nLevelDef;
	tMessage.nLevelArea = LevelGetLevelAreaID( pLevelDest );
	// send to quests
	QUEST_MESSAGE_RESULT eResult = MetagameSendMessage( pPlayer, QM_SERVER_LEAVE_LEVEL, &tMessage );
	ASSERT( eResult == QMR_OK )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestTakeAllItemsAndClose(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	s_NPCTalkStop( UnitGetGame( pPlayer ), pPlayer );
	s_QuestClose( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnlinkRewardItem(
	UNIT* pItem,
	QUEST* pQuest)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX(pPlayer, "Couldn't get player from quest while unliking reward item");
	int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);

	// unlink this item
	UnitSetStat( pItem, STATS_QUEST_REWARD, nDifficulty, INVALID_ID );

	// unlink any inventory
	UNIT* pContained = NULL;
	while ((pContained = UnitInventoryIterate( pItem, pContained )) != NULL)
	{
		sUnlinkRewardItem( pContained, pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnlinkRewardItem(
							  UNIT* pItem,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestDef )
{
	// unlink this item
	UnitSetStat( pItem, STATS_QUEST_TASK_REF, INVALID_ID );
	UnitSetStat( pItem, STATS_QUEST_REWARD_GUARANTEED, INVALID_ID );
	// unlink any inventory
	UNIT* pContained = NULL;
	while ((pContained = UnitInventoryIterate( pItem, pContained )) != NULL)
	{
		sUnlinkRewardItem( pContained, pQuestDef );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sQuestItemTakenCallback(
	TAKE_ITEM &tTakeItem)
{
	UNIT *pItem = tTakeItem.pItem;

	QUEST *pQuest = (QUEST *) tTakeItem.pUserData;
	ASSERTX_RETURN( pQuest, "Expected quest when closing reward ui" );	

	// unlink the item from quest
	sUnlinkRewardItem( pItem, pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sQuestRewardItemsExist( 
	QUEST *pQuest )
{
	ASSERTX_RETVAL( pQuest, 0, "Expected quest" );

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int nOffer = pQuest->pQuestDef->nOfferReward;
	if ( nOffer != INVALID_ID )
	{
		// Get the existing offer items, then check if they are associated
		// with the quest.
		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);
		OfferGetExistingItems( pPlayer, nOffer, tOfferItems );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];
			int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
			if ( UnitGetStat( pReward, STATS_QUEST_REWARD, nDifficulty) == pQuest->nQuest )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_sQuestCreateRewardItems( QUEST *pQuest )
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int nOffer = pQuest->pQuestDef->nOfferReward;
	if ( nOffer != INVALID_ID && !s_OfferIsEmpty( pPlayer, nOffer ) )
	{
		// Create the quest reward items through the offer system.
		// Creating the item UNITs early makes it possible to display them in quest 
		// NPC dialogs, and to save/restore items which were randomly created.
		int nDifficulty = pQuest->nDifficulty;
		if (nDifficulty < 0 || nDifficulty >= arrsize( pQuest->pQuestDef->nLevel ))
		{
			nDifficulty = DIFFICULTY_NORMAL;
			ASSERTX(FALSE, "Illegal difficulty, defaulting to normal difficulty rewards.");
		}

		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);
		s_OfferCreateItems( pPlayer, nOffer, &tOfferItems, INVALID_LINK, INVALID_LINK, pQuest->pQuestDef->nLevel[ nDifficulty ] );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];
			int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
			UnitSetStat( pReward, STATS_QUEST_REWARD, nDifficulty, pQuest->nQuest );
			ASSERTX( s_UnitCanBeSaved( pReward, FALSE ), "reward items should be able to be saved");
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDisplayDialog( 
	QUEST *pQuest,
	UNIT *pDialogActivator,	// NPC (GENUS_MONSTER), or GENUS_ITEM
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward /* = INVALID_LINK */,
	GLOBAL_INDEX eSecondNPC /* = GI_INVALID */,
	BOOL bLeaveUIOpenOnOk /*= FALSE*/)
{
	ASSERT_RETURN(pQuest && pQuest->pQuestDef);
	int nOffer = pQuest->pQuestDef->nOfferReward;
	// check if the reward panel should be activated
	if ( s_QuestIsRewarder( pQuest, pDialogActivator ) &&
		 QuestIsActive( pQuest ) &&
		 s_QuestAllStatesComplete( pQuest, TRUE ) )
	{
		// this quest is complete
		if (nOffer != INVALID_LINK)
		{
			// if the quest is already in the offer reward state, we just display the offer
			if (!QuestIsOffering( pQuest ))
			{
				// quest is now in offering status, reward panel unlocked
				s_QuestOffering( pQuest );
			}
		}
	}

	// make sure the reward item UNITs are created, so they can appear in
	// the dialog
	if ( !sQuestRewardItemsExist( pQuest ) )
		s_sQuestCreateRewardItems( pQuest );

#if !ISVERSION(SERVER_VERSION)
	// hide the inventory screen, since the dialog does not draw over it properly
	UIComponentActivate(UICOMP_INVENTORY, FALSE);
#endif // !ISVERSION(SERVER_VERSION)

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	if (QuestIsOffering( pQuest ))
	{
		ASSERT_RETURN(nOffer != INVALID_LINK);

		// move any items from quest dialog inventory back to offer inventory
		s_sQuestDialogMakeRewardItemsAvailableToClient( pQuest, FALSE );

 		// setup actions
 		OFFER_ACTIONS tActions;
 		tActions.pfnAllTaken = s_QuestTakeAllItemsAndClose;
 		tActions.pAllTakenUserData = pQuest;
 
 		// display offer
 		s_OfferDisplay( pPlayer, pDialogActivator, nOffer, tActions );

	}
	else
	{
		// move any items from offer inventory to quest dialog inventory temporarily
		s_sQuestDialogMakeRewardItemsAvailableToClient( pQuest, TRUE );
	}
	{
		// send the quest info the client, so dialog strings can be generated and displayed
		// init message
		MSG_SCMD_QUEST_DISPLAY_DIALOG tMessage;
		QuestTemplateInit( &tMessage.tQuestTemplate );

		tMessage.nQuest = pQuest->nQuest;
		tMessage.tQuestTemplate = pQuest->tQuestTemplate;
		tMessage.idQuestGiver = UnitGetId( pDialogActivator );
		tMessage.nDialog = nDialog;
		tMessage.nDialogReward = nDialogReward;
		tMessage.nGossipString = INVALID_LINK;
		tMessage.nDialogType = eType;
		tMessage.nGISecondNPC = eSecondNPC;
		tMessage.bLeaveUIOpenOnOk = (BYTE)bLeaveUIOpenOnOk;

		// send it
		GAMECLIENTID idClient = UnitGetClientId( pPlayer );
		GAME *pGame = UnitGetGame( pPlayer );
		s_SendMessage( pGame, idClient, SCMD_QUEST_DISPLAY_DIALOG, &tMessage );	
	}

	// mark them as talking to the giver
	s_TalkSetTalkingTo( pPlayer, pDialogActivator, nDialog );
	UnitSetStat( pPlayer, STATS_QUEST_CURRENT, pQuest->nQuest );

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestOfferReward(
	QUEST *pQuest,
	UNIT *pOfferer,
	BOOL bCreateOffer /* = TRUE */)
{
	// get the reward for this player class
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	int nOffer = pQuestDef->nOfferReward;

	if (nOffer != INVALID_LINK)
	{
		return TRUE;  // offer was made in s_QuestDisplayDialog
	}

	// no offer, close quest
	s_QuestClose( pQuest );

	return FALSE;  // no offer made
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestOfferReward(int nTaskID,
						UNIT *pOfferer,
						BOOL bCreateOffer /* = TRUE */)
{
	// get the reward for this player class

	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestDef = QuestTaskDefinitionGet( nTaskID ); 
	ASSERT_RETFALSE(pQuestDef);
	int nOffer = pQuestDef->nQuestGivesItems;

	if (nOffer != INVALID_LINK)
	{
		return TRUE;  // offer was made in s_QuestDisplayDialog
	}

	// no offer, close quest
	// TODO: - what does mythos need to do here?
	//s_QuestClose( pQuest );

	return FALSE;  // no offer made
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestIsGiver(
	QUEST *pQuest,
	UNIT *pUnit)
{
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );

	// if a Giver Item is specified, give priority to that
	if (ptQuestDef->nGiverItem != INVALID_LINK)
	{
		return UnitGetGenus( pUnit ) == GENUS_ITEM &&
			UnitGetClass( pUnit ) == ptQuestDef->nGiverItem;
	}

	// otherwise, use the Cast Giver
	if (ptQuestDef->nCastGiver != INVALID_LINK)
	{
		SPECIES spUnit = UnitGetSpecies( pUnit );
		SPECIES spGiver = QuestCastGetSpecies( pQuest, ptQuestDef->nCastGiver );
		if (spGiver == spUnit)
		{
			return TRUE;
		}
				
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestIsRewarder(
	QUEST *pQuest,
	UNIT *pUnit)
{
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	
	// check for class specific givers
	if (ptQuestDef->nCastRewarder != INVALID_LINK)
	{
		SPECIES spUnit = UnitGetSpecies( pUnit );
		SPECIES spGiver = QuestCastGetSpecies( pQuest, ptQuestDef->nCastRewarder );
		if (spGiver == spUnit)
		{
			return TRUE;
		}
				
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_GROUP * s_QuestNodeSetGetNode( 
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * set, 
	ROOM_LAYOUT_SELECTION_ORIENTATION ** pOrientationOut, 
	int node )
{
	if(pOrientationOut)
	{
		*pOrientationOut = NULL;
	}
	if ( node <= 0 || node > set->nGroupCount )
	{
		return NULL;
	}
	for ( int n = 0; n < set->nGroupCount; n++ )
	{
		ROOM_LAYOUT_GROUP * cur = &set->pGroups[n];
		char szNodeName[32];
		int c;
		for ( c = 0; cur->pszName[c] && cur->pszName[c] != '|'; c++ )
		{
			szNodeName[c] = cur->pszName[c];
		}
		szNodeName[c+1] = 0;
		int nodeval = PStrToInt( szNodeName );
		if ( nodeval == node )
		{
			if(pOrientationOut)
			{
				*pOrientationOut = RoomLayoutGetOrientationFromGroup(pRoom, cur);
			}
			return cur;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_GROUP *s_QuestNodeSetGetNodeByName( 
	ROOM_LAYOUT_GROUP * nodeset, 
	char * name )
{
	for ( int n = 0; n < nodeset->nGroupCount; n++ )
	{
		ROOM_LAYOUT_GROUP * node = &nodeset->pGroups[n];
		if ( _stricmp( node->pszName, name ) == 0 )
		{
			return node;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_GROUP *s_QuestNodeNext( 
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * nodeset, 
	ROOM_LAYOUT_SELECTION_ORIENTATION ** pOrientationOut, 
	ROOM_LAYOUT_GROUP * node )
{
	char szNodeName[32];
	int c;
	for ( c = 0; node->pszName[c] && node->pszName[c] != '|'; c++ )
	{
		szNodeName[c] = node->pszName[c];
	}
	szNodeName[c+1] = 0;
	int nextval = PStrToInt( szNodeName ) + 1;
	return s_QuestNodeSetGetNode( pRoom, nodeset, pOrientationOut, nextval );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sQuestNodeExtract( 
	ROOM_LAYOUT_GROUP * node, 
	char cFilter )
{
	// skip name
	int c;
	for ( c = 0; node->pszName[c] && node->pszName[c] != '|'; c++ );
	c++;	// skip separator
	// is there a filter value?
	while ( node->pszName[c] )
	{
		if ( node->pszName[c] == cFilter )
		{
			c++;		// skip filter
			char szVal[32];
			int i;
			for ( i = 0; node->pszName[c] && node->pszName[c] != '|'; i++, c++ )
			{
				szVal[i] = node->pszName[c];
			}
			szVal[i] = 0;
			return (float)PStrToFloat( szVal );
		}
		else
		{
			for ( ; node->pszName[c] && node->pszName[c] != '|'; c++ );
			c++;
		}
	}
	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestNodeGetPosAndDir( 
	ROOM * room, 
	ROOM_LAYOUT_SELECTION_ORIENTATION * node, 
	VECTOR * pos, 
	VECTOR * dir, 
	float * angle, 
	float * pitch )
{
	ASSERT_RETURN(node);
	if ( angle || pitch || dir )
	{

		VECTOR vDirection;
		MATRIX zRot;
		VECTOR vRotated;
		MatrixRotationAxis(zRot, cgvZAxis, (node->fRotation * 180.0f) / PI);
		MatrixMultiplyNormal( &vRotated, &cgvYAxis, &zRot );
		MatrixMultiplyNormal( &vDirection, &vRotated,   & room->tWorldMatrix );
		if ( dir )
		{
			*dir = vDirection;
		}
		if ( angle )
		{
			*angle = atan2( vDirection.fY, vDirection.fX );
		}
		if ( pitch )
		{
			float fXYDistance = sqrtf( vDirection.fX * vDirection.fX + vDirection.fY * vDirection.fY );
			*pitch = atan2( vDirection.fZ, fXYDistance );
		}
	}
	if ( pos )
	{
		MatrixMultiply( pos, &node->vPosition, &room->tWorldMatrix );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
BOOL s_QuestNodeNextScript( 
	GAME * game, 
	UNIT * unit, 
	const struct EVENT_DATA& tEventData )
{
	QuestNodeScript( game, (ROOM_LAYOUT_GROUP *)tEventData.m_Data1, ( ROOM_LAYOUT_GROUP *)tEventData.m_Data2, unit );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestNodeCameraMove( 
	GAME * game, 
	ROOM_LAYOUT_GROUP * nodeset, 
	ROOM_LAYOUT_GROUP * node, 
	UNIT * unit )
{
	float fTime = sQuestNodeExtract( node, 'c' );
	if ( fTime != 0.0f )
	{
		ROOM_LAYOUT_GROUP * next = QuestNodeNext( nodeset, node );
		if ( next )
		{
			VECTOR vPosition;
			float fAngle, fPitch;
			QuestNodeGetPosAndDir( UnitGetRoom( unit ), next, &vPosition, NULL, &fAngle, &fPitch );
			//c_CameraFollowPathDestination( game, vPosition, fAngle, fPitch, fTime );
			int nFrames = (int)FLOOR( fTime * GAME_TICKS_PER_SECOND_FLOAT );
			UnitRegisterEventTimed( unit, QuestNodeNextScript, &EVENT_DATA( (DWORD_PTR)nodeset, (DWORD_PTR)next ), nFrames );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestNodeContinue( 
	GAME * game, 
	UNIT * unit, 
	const struct EVENT_DATA& tEventData )
{
	QuestNodeCameraMove( game, (ROOM_LAYOUT_GROUP *)tEventData.m_Data1, ( ROOM_LAYOUT_GROUP *)tEventData.m_Data2, unit );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestNodeScript( 
	GAME * game, 
	ROOM_LAYOUT_GROUP * nodeset, 
	ROOM_LAYOUT_GROUP * node, 
	UNIT * unit )
{
	float fDelay = sQuestNodeExtract( node, 'd' );
	if ( fDelay != 0.0f )
	{
		int nFrames = (int)FLOOR( ( fDelay * GAME_TICKS_PER_SECOND_FLOAT ) + 0.5f );
		UnitRegisterEventTimed( unit, QuestNodeContinue, &EVENT_DATA( (DWORD_PTR)nodeset, (DWORD_PTR)node ), nFrames );
	}
	else
	{
		QuestNodeCameraMove( game, nodeset, node, unit );
	}
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestNodeGetPathPosition( 
	ROOM * room, 
	char * szPathName, 
	int nPathNode, 
	VECTOR & vPosition )
{
	ASSERT_RETFALSE( room );
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * nodeset = RoomGetNodeSet( room, szPathName, &pOrientation );
	ASSERT_RETFALSE( nodeset );
	ASSERT_RETFALSE( pOrientation );
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientationNode = NULL;
	ROOM_LAYOUT_GROUP * node = s_QuestNodeSetGetNode( room, nodeset, &pOrientationNode, nPathNode );
	ASSERT_RETFALSE( node );
	ASSERT_RETFALSE( pOrientationNode );
	s_QuestNodeGetPosAndDir( room, pOrientationNode, &vPosition, NULL, NULL, NULL );	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_QuestForceComplete(
	UNIT * pPlayer,
	int nQuest)
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if ( !pQuest )
	{
		return;
	}

	// complete all states
	s_QuestForceStatesComplete( pPlayer, nQuest, TRUE );
			
	// force to complete status if not set
	if (pQuest->eStatus != QUEST_STATUS_COMPLETE)
	{
		// set new status
		sQuestSetStatus( pQuest, QUEST_STATUS_COMPLETE );	
	}
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestForceStatesComplete(
	UNIT * pPlayer,
	int nQuest,
	BOOL bMakeQuestStatusComplete)
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if ( !pQuest )
	{
		return;
	}

	// force to active so we can complete the states
	s_QuestForceActivate( pPlayer, nQuest );
	
	// complete all substates
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
		if (eStateValue != QUEST_STATE_COMPLETE)
		{
			QuestStateSet( pQuest, pState->nQuestState, QUEST_STATE_COMPLETE );
		}
	}
			
	// now complete the status if requested
	if (bMakeQuestStatusComplete)
	{
		s_QuestComplete( pQuest );
	}

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// #if ISVERSION(CHEATS_ENABLED)  -- phu: currently in use by release app to skip tutorial
void s_QuestForceActivate(
	UNIT * pPlayer,
	int nQuest)
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if ( !pQuest )
	{
		return;
	}

	// complete any quest prereqs
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	for (int i = 0; i < arrsize( ptQuestDef->nQuestPrereqs ); ++i)
		if (ptQuestDef->nQuestPrereqs[i] != INVALID_LINK)
		{
			QUEST *pQuestPrereq = QuestGetQuest( pPlayer, ptQuestDef->nQuestPrereqs[i] );
			ASSERTV_CONTINUE(pQuestPrereq, "failed to get QUEST pointer for valid link to prereq for quest %s", ptQuestDef->szName);
			sQuestPrereqForceComplete( pQuestPrereq );
		}
	
	// force inactive	
	pQuest->eStatus = QUEST_STATUS_INACTIVE;
	
	// reset states
	s_QuestClearAllStates( pQuest );

	// do normal activation logic
	BOOL bSuccess = s_QuestActivate( pQuest );
	ASSERTV_RETURN( bSuccess, "quest \"%s\": failed to force activation, inventory may be full", pQuest->pQuestDef->szName );
	UNREFERENCED_PARAMETER( bSuccess );
			
}
// #endif
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Note: Used for command console cheat and in-game abandon function
static void sDoAbandon( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );

	UNIT * pPlayer = pQuest->pPlayer;

	// remove all quest items
	for( int i = 0; i < MAX_QUEST_ABANDON_ITEMS; i++ )
	{
		if( pQuest->pQuestDef->nAbandonItemList[i] != INVALID_ID )
		{
			s_QuestRemoveItem( pPlayer, pQuest->pQuestDef->nAbandonItemList[i] );
		}
	}

	// remove items from the starting item treasure class, if they were flagged for removal
	s_sQuestRemoveItemsOnDeactivate( pQuest );

	// always remove quest items for now, in the future this could be different from the "remove on quest deactivate" stat
	s_QuestRemoveQuestItems( pPlayer, pQuest );

	// re-init quest states
	s_QuestClearAllStates( pQuest, TRUE );

	// re-init the status
	sQuestSetStatus( pQuest, QUEST_STATUS_INACTIVE );

	// reset warps
	if ( pQuest->pQuestDef->nWarpToOpenActivate != INVALID_LINK )
	{
		s_QuestCloseWarp( UnitGetGame( pPlayer ), pPlayer, pQuest->pQuestDef->nWarpToOpenActivate );
	}

	// send a message to this quest
	QuestSendMessageToQuest( pQuest, QM_SERVER_ABANDON, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAbandon(
	QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );

	if (!QuestCanAbandon(pQuest->nQuest))
	{
		return;
	}

	// must be active
	ASSERTX_RETURN( QuestIsActive( pQuest ), "Quest must be active to abandon" );

	// can't abandon tutorial
	ASSERT_RETURN( pQuest->nQuest != QUEST_TUTORIAL );

	sDoAbandon( pQuest );

	const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;
	// exit game if this is a story quest
	if ( pQuestDef && ( pQuestDef->eStyle == QS_STORY ) )
	{
		UNIT * pPlayer = pQuest->pPlayer;
		ASSERT_RETURN( pPlayer );
		GAMECLIENT * pClient = UnitGetClient( pPlayer );
		ASSERT_RETURN( pClient );
		s_GameExit( QuestGetGame( pQuest ), pClient );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_QuestForceReset(
	UNIT * pPlayer,
	int nQuest)
{
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);

	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if ( !pQuest )
	{
		return;
	}

	sDoAbandon( pQuest );
}
#endif

#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAdvance(
	QUEST *pQuest)
{

	// find the most completed state
	int nNumStates = pQuest->nNumStates;
	int iLastComplete = -1;
	for (int i = 0; i < nNumStates; ++i)
	{
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		if (pState->eValue == QUEST_STATE_ACTIVE)
		{
			QuestStateSet( pQuest, pState->nQuestState, QUEST_STATE_COMPLETE );
			iLastComplete = i;
		}
		
	}
	
	if (iLastComplete > -1)
	{
		for (int j = iLastComplete + 1; j < nNumStates; ++j)
		{
			QUEST_STATE *pStateNext = QuestStateGetByIndex( pQuest, j );
			if (pStateNext->eValue == QUEST_STATE_HIDDEN)
			{
				QuestStateSet( pQuest, pStateNext->nQuestState, QUEST_STATE_ACTIVE );
				break;
			}

		}
	}
}
	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestAdvanceTo( 
	QUEST *pQuest, 
	int nQuestState)
{
	QUEST_STATE *pStateLast = QuestStateGetPointer( pQuest, nQuestState );

	// set all states before this one as complete
	int nNumStates = pQuest->nNumStates;
	for (int i = 0; i < nNumStates; ++i)
	{
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		
		// if we found the last one, stop
		if (pState == pStateLast)
		{
			break;
		}
		else if (pState->eValue != QUEST_STATE_COMPLETE)
		{
			QuestStateSet( pQuest, pState->nQuestState, QUEST_STATE_COMPLETE );
		}
		
	}
	
	// set this one as active
	QuestStateSet( pQuest, nQuestState, QUEST_STATE_ACTIVE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestInactiveAdvanceTo(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( nQuestState != INVALID_LINK, "Expected quest state link" );
	
	if (QuestIsInactive( pQuest ))
	{
		s_QuestActivate( pQuest );
		s_QuestAdvanceTo( pQuest, nQuestState );
	}
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestCanRunDRLGRule(
	UNIT * player,
	LEVEL * level,
	char * pszRuleName )
{
	ASSERTX_RETFALSE( player, "Invalid player" );
	ASSERTX_RETFALSE( level, "Not a valid level" );

	// setup message
	QUEST_MESSAGE_CAN_RUN_DRLG_RULE msg;
	msg.nLevelDefId = level->m_nLevelDef;
	msg.nDRLGDefId = level->m_nDRLGDef;
	PStrCopy( msg.szRuleName, pszRuleName, CAN_RUN_DRLG_RULE_NAME_SIZE );

	// send message
	QUEST_MESSAGE_RESULT result = MetagameSendMessage( player, QM_SERVER_CAN_RUN_DRLG_RULE, &msg );

	if ( result == QMR_DRLG_RULE_FORBIDDEN )
	{
		return FALSE;
	}
	return TRUE;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestMapReveal(
	QUEST * pQuest,
	GLOBAL_INDEX eGILevel )
{
	ASSERTX_RETURN( pQuest, "Expected Quest" );
	UNIT * player = pQuest->pPlayer;
	ASSERTX_RETURN( player, "Expected player" );

	int nLevelDefId = GlobalIndexGet( eGILevel );
	if (nLevelDefId != INVALID_LINK)
	{
		LevelMarkVisited( player, nLevelDefId, WORLDMAP_REVEALED );
	}
	
}

//----------------------------------------------------------------------------
// s_QuestAttemptToSpawnMissingObjectiveObjects
// Attempts to spawn objects of QUEST_DEFINITION::nObjectiveObject class,
// until there is an amount in the level that meets requirements set by
// the QUEST_DEFINITION data. 
// Returns the number of objects spawned.
//----------------------------------------------------------------------------
struct COUNT_OBJECTS_DATA
{
	SPECIES spSpecies;
	int nCount;

	COUNT_OBJECTS_DATA::COUNT_OBJECTS_DATA( )
	{
		spSpecies = SPECIES_NONE;
		nCount = 0;
	}
};

static ROOM_ITERATE_UNITS sCountObjects(
	UNIT *pUnit,
	void *pCallbackData)
{
	COUNT_OBJECTS_DATA *pData = (COUNT_OBJECTS_DATA *)pCallbackData;

	if (UnitGetSpecies( pUnit ) == pData->spSpecies)
		++pData->nCount;

	return RIU_CONTINUE;
}

int s_QuestAttemptToSpawnMissingObjectiveObjects(
	QUEST* pQuest,
	LEVEL* pLevel)
{
	GAME* pGame = QuestGetGame( pQuest );
	if ( pQuest->pQuestDef->bDisableSpawning ||
		pQuest->pQuestDef->nObjectiveObject == INVALID_LINK )
	{
		return 0;
	}

	// does quest have a spawnable level definition for spawns but no level instance,
	// in that case we assume that any level of the same definition type will do
	int nLevelDefinition = pQuest->pQuestDef->nLevelDefDestinations[0];
	if (nLevelDefinition == INVALID_LINK ||
		nLevelDefinition == LevelGetDefinitionIndex( pLevel ))
	{
		ASSERTV_RETZERO( LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ), "quest \"%s\": trying to spawn Objective Object in a level that isn't populated", pQuest->pQuestDef->szName );

		const UNIT_DATA *pUnitData = ObjectGetData( pGame, pQuest->pQuestDef->nObjectiveObject );
		ASSERTV_RETZERO( pUnitData != NULL, "quest \"%s\": Could not find unit data for Objective Object", pQuest->pQuestDef->szName );

		// count how many existing objects of the same class are in the level
		COUNT_OBJECTS_DATA tData;
		tData.spSpecies = MAKE_SPECIES( GENUS_OBJECT, pQuest->pQuestDef->nObjectiveObject );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		LevelIterateUnits( pLevel, eTargetTypes, sCountObjects, &tData );

		// spawn the greatest of ( number to spawn specified, number required for quest specified, 1 ) - num in level
		const int nObjectsToSpawn = 
			MAX( 1, MAX( pQuest->pQuestDef->nSpawnCount, pQuest->pQuestDef->nObjectiveCount ) ) - tData.nCount;

		if ( nObjectsToSpawn <= 0 )
			return 0;

		// first try to spawn on label nodes, if specified
		ROOM* pSpawnRooms[MAX_RANDOM_ROOMS];
		ROOM_LAYOUT_GROUP* pSpawnLayoutGroups[MAX_RANDOM_ROOMS];
		ROOM_LAYOUT_SELECTION_ORIENTATION* pSpawnLayoutOrientations[MAX_RANDOM_ROOMS];
		int nSpawnRoomCount = 0;
		ZeroMemory(pSpawnRooms, sizeof pSpawnRooms);
		ZeroMemory(pSpawnLayoutGroups, sizeof pSpawnLayoutGroups);

		ROOM* pLabelNodeRooms[MAX_RANDOM_ROOMS];
		ROOM_LAYOUT_GROUP* pLayoutGroups[MAX_RANDOM_ROOMS];
		ROOM_LAYOUT_SELECTION_ORIENTATION* pOrientations[MAX_RANDOM_ROOMS];
		int nLabelNodeRoomCount = 0;
		if (pQuest->pQuestDef->szSpawnNodeLabel[0] != 0)
		{		
			ROOM * pRoom = LevelGetFirstRoom(pLevel);
			// save all the matching label nodes and their rooms that can fit in the array
			while (pRoom && nLabelNodeRoomCount < arrsize( pLabelNodeRooms ) )
			{
				// TODO cmarch: check if an object is already in this room (created by something other than this code)
				ROOM_LAYOUT_SELECTION_ORIENTATION* pOrientation = NULL;
				ROOM_LAYOUT_GROUP * pNodeset = 
					RoomGetLabelNode(
					pRoom, 
					pQuest->pQuestDef->szSpawnNodeLabel, 
					&pOrientation);

				if ( pNodeset && pOrientation )
				{
					if (pNodeset->nGroupCount  == 0)
					{
						pLayoutGroups[nLabelNodeRoomCount] = pNodeset;
						ASSERTV_RETZERO( pLayoutGroups[nLabelNodeRoomCount], "quest \"%s\": failed to get label node from set", pQuest->pQuestDef->szName );
						pOrientations[nLabelNodeRoomCount] = pOrientation;
						pLabelNodeRooms[nLabelNodeRoomCount++] = pRoom;
					}
					else
					{
						for ( int i = 0; i < pNodeset->nGroupCount && nLabelNodeRoomCount < arrsize( pLabelNodeRooms ) ; i++ )
						{
							ROOM_LAYOUT_SELECTION_ORIENTATION* pOrientationNode = NULL;
							pLayoutGroups[nLabelNodeRoomCount] = s_QuestNodeSetGetNode( pRoom, pNodeset, &pOrientationNode, i+1 );
							ASSERTV_RETZERO( pLayoutGroups[nLabelNodeRoomCount], "quest \"%s\": failed to get label node from set", pQuest->pQuestDef->szName );
							pOrientations[nLabelNodeRoomCount] = pOrientationNode;
							ASSERTV_RETZERO( pOrientations[nLabelNodeRoomCount], "quest \"%s\": failed to get label node orientation from set", pQuest->pQuestDef->szName );
							pLabelNodeRooms[nLabelNodeRoomCount++] = pRoom;
						}
					}
				}

				pRoom = LevelGetNextRoom(pRoom);
			}

			if (nLabelNodeRoomCount > 0)
			{
				// pick label nodes at random, up to the amount needed, w/o duplicate picks
				PickerStart( pGame, picker );	
				for (int i = 0; i < nLabelNodeRoomCount; ++i)
				{
					PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
				}

				for (nSpawnRoomCount = 0; nSpawnRoomCount < MIN(nLabelNodeRoomCount, int(arrsize(pSpawnRooms))); ++nSpawnRoomCount)
				{
					int nPick = PickerChooseRemove( pGame );
					ASSERTV_BREAK( nPick >= 0 && nPick < nLabelNodeRoomCount, "quest \"%s\": Invalid room picked to spawn object", pQuest->pQuestDef->szName );

					ASSERTV_BREAK( pLabelNodeRooms[nPick], "quest \"%s\": Invalid room pointer for picked room", pQuest->pQuestDef->szName );
					pSpawnRooms[nSpawnRoomCount] = pLabelNodeRooms[nPick];
					ASSERTV_BREAK( pLayoutGroups[nPick], "quest \"%s\": Invalid layout group pointer for picked label node", pQuest->pQuestDef->szName );
					pSpawnLayoutGroups[nSpawnRoomCount] = pLayoutGroups[nPick];
					ASSERTV_BREAK( pOrientations[nPick], "quest \"%s\": Invalid layout group orientation pointer for picked label node", pQuest->pQuestDef->szName );
					pSpawnLayoutOrientations[nSpawnRoomCount] = pOrientations[nPick];
				}
			}
		}

		if (nSpawnRoomCount < nObjectsToSpawn)
		{
			// not enough (maybe none) label nodes found in level to spawn all 
			// the objects, pick random rooms in the level 
			int nRandomRoomCount = 
				s_LevelGetRandomSpawnRooms( 
				pGame, 
				pLevel, 
				pSpawnRooms + nSpawnRoomCount, 
				MIN( nObjectsToSpawn - nSpawnRoomCount, int(arrsize(pSpawnRooms)) - nSpawnRoomCount ) );
			ASSERTV_RETZERO( nRandomRoomCount > 0, "quest \"%s\": Unable to pick random rooms for object spawn", pQuest->pQuestDef->szName );
			nSpawnRoomCount += nRandomRoomCount;
		}

		ASSERTV_RETZERO( nSpawnRoomCount > 0, "quest \"%s\": Failed to pick any rooms for object spawn", pQuest->pQuestDef->szName );

		// go through any spawnable units to create
		DWORD dwRandomNodeFlags = 0;
		SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
		int nNumSpawned = 0;
		int nSpawnRoomIndex = 0;
		
		// If picking a random node, try to get one that has a fair amount
		// of free space around it. Otherwise, small objects may be hidden
		// away in hard to see and reach spots.
		ASSERTV( pUnitData->fCollisionRadius <= 2.f, "quest object \"%s\" must have a CollisionRadius in objects.xls set to 2 or less, or it may fail to fit in a random location.", pUnitData->szName );
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		float fDesiredNodeRadius = MAX(pUnitData->fCollisionRadius, 5.0f);
		float fDesiredNodeHeight = MAX(pUnitData->fCollisionHeight, 2.5f);
		const float PLAYER_COLLISION_FUDGE = 0.4f;
		float fMinNodeRadius = MAX(pUnitData->fCollisionRadius, (UnitGetCollisionRadius( pPlayer ) + PLAYER_COLLISION_FUDGE));
		float fMinNodeHeight = MAX(pUnitData->fCollisionHeight, (UnitGetCollisionHeight( pPlayer ) + PLAYER_COLLISION_FUDGE));

		const int MAX_RANDOM_OBJECTS = 8;
		ROOM *pOccupiedNodeRooms[MAX_RANDOM_OBJECTS];
		int nOccupiedNodes[MAX_RANDOM_OBJECTS];
		int nNumOccupiedNodes = 0;
		
		for (int i = 0; i < nObjectsToSpawn; ++i)
		{
			const int MAX_TRIES = 32;
			int nTries = MAX_TRIES;
			SPAWN_LOCATION tSpawnLocation;
			UNIT *pSpawnedObject = NULL;
			while (nTries-- > 0)
			{
				nSpawnRoomIndex = ( ( nSpawnRoomIndex + 1 ) % nSpawnRoomCount );
				ROOM *pRoom = pSpawnRooms[nSpawnRoomIndex];
				int nNodeIndex = INVALID_ID;
				if (pSpawnLayoutGroups[nSpawnRoomIndex] == NULL)
				{
					BOOL bAllowSmallNodeRadius = (nTries < MAX_TRIES /2);

					// pick a random node to spawn at
					nNodeIndex = 
						s_RoomGetRandomUnoccupiedNode( 
							pGame, 
							pRoom, 
							dwRandomNodeFlags, 
							bAllowSmallNodeRadius ? fMinNodeRadius : fDesiredNodeRadius,
							bAllowSmallNodeRadius ? fMinNodeHeight : fDesiredNodeHeight);
					if ( nNodeIndex < 0 )
						continue;

					// don't use the same node twice. I've decided not to set
					// the objects as blocking, since they are randomly placed, and
					// I don't want them to block doorways and such
					BOOL bSkipNode = FALSE;
					for (int j = 0; j < nNumOccupiedNodes; ++j)
						if (pOccupiedNodeRooms[j] == pRoom &&
							nOccupiedNodes[j] == nNodeIndex)
						{
							bSkipNode = TRUE;
							break;
						}
					
					if (bSkipNode)
						continue;

					ASSERTV_CONTINUE(nNumOccupiedNodes < arrsize( nOccupiedNodes ), "quest %s: tried to spawn too many objective objects, increase MAX_RANDOM_OBJECTS", pQuest->pQuestDef->szName);
					pOccupiedNodeRooms[nNumOccupiedNodes] = pRoom;
					nOccupiedNodes[nNumOccupiedNodes] = nNodeIndex;
					++nNumOccupiedNodes;

					SpawnLocationInit( &tSpawnLocation, pRoom, nNodeIndex );
				}
				else
				{
					// spawn at the label node which someone placed in the layout
					s_QuestNodeGetPosAndDir( 
						pRoom, 
						pSpawnLayoutOrientations[nSpawnRoomIndex], 
						&tSpawnLocation.vSpawnPosition, 
						&tSpawnLocation.vSpawnDirection, 
						NULL, 
						NULL );
					tSpawnLocation.vSpawnUp = cgvZAxis; // could use SpawnLocationInit with the label node orientation to solve for UP, but I am following example usage of s_QuestNodeGetPosAndDir with monster spawn
					pSpawnLayoutGroups[nSpawnRoomIndex] = NULL; // don't use the same node twice
				}
	
				pSpawnedObject = 
					s_RoomSpawnObject( 
						pGame,
						pRoom,
						&tSpawnLocation,
						pUnitData->wCode );		

				if ( pSpawnedObject != NULL )
				{
					++nNumSpawned;
					#if ISVERSION(DEVELOPMENT) 
					trace( "s_QuestAttemptToSpawnMissingObjectiveObjects: spawned Unit '%s' [%d] in room %d, node %d, at (%.3f, %.3f, %.3f)\n", UnitGetDevName( pSpawnedObject ), UnitGetId( pSpawnedObject ), RoomGetId(UnitGetRoom( pSpawnedObject )), nNodeIndex, UnitGetPosition(pSpawnedObject).x, UnitGetPosition(pSpawnedObject).y, UnitGetPosition(pSpawnedObject).z);
					#endif //	ISVERSION(DEVELOPMENT) 
					break;
				}
			}
			ASSERTV( pSpawnedObject, "quest \"%s\": Failed to spawn quest object", pQuest->pQuestDef->szName );
		}

		ASSERTV( nNumSpawned == nObjectsToSpawn, "quest \"%s\": Failed to spawn quest objects", pQuest->pQuestDef->szName );
		return nNumSpawned;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_QuestSpawnObject( 
	int nObjectClass, 
	UNIT *pLocation, 
	UNIT *pFacing)
{
	ASSERTX_RETNULL( nObjectClass != INVALID_LINK, "Expected object class" );
	ASSERTX_RETNULL( pLocation, "Expected location unit" );
	GAME *pGame = UnitGetGame( pLocation );

	LEVEL *pLevel = UnitGetLevel( pLocation );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	UNIT *pObject = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, nObjectClass );	
	if (pObject)
	{
		return NULL;
	}
	
	// the portal will face toward the killer
	VECTOR vToFacing;
	if (pFacing != NULL)
	{
		vToFacing = UnitGetPosition( pFacing ) - UnitGetPosition( pLocation );
		VectorNormalize( vToFacing );		
	}
	
	// spawn object
	OBJECT_SPEC tSpec;
	tSpec.nClass = nObjectClass;
	tSpec.pRoom = UnitGetRoom( pLocation );
	tSpec.vPosition = UnitGetPosition( pLocation );
	tSpec.pvFaceDirection = pFacing ? &vToFacing : NULL;
	return s_ObjectSpawn( pGame, tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

typedef void FP_QUEST_OPERATE_DELAY( QUEST * pQuest, UNITID idUnit );

static BOOL sOperateDone( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	if ( !UnitHasState( game, unit, STATE_OPERATE_OBJECT_DELAY ) )
		return TRUE;

	s_StateClear( unit, UnitGetId( unit ), STATE_OPERATE_OBJECT_DELAY, 0 );

	FP_QUEST_OPERATE_DELAY * pfOperate = ( FP_QUEST_OPERATE_DELAY * )tEventData.m_Data3;
	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	UNITID idUnit = ( UNITID )tEventData.m_Data2;

	pfOperate( pQuest, idUnit );
	return TRUE;
}

static BOOL sMonsterOperateDone( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	if ( !UnitHasState( game, unit, STATE_OPERATE_OBJECT_DELAY_MONSTER ) )
		return TRUE;

	s_StateClear( unit, UnitGetId( unit ), STATE_OPERATE_OBJECT_DELAY_MONSTER, 0 );

	FP_QUEST_OPERATE_DELAY * pfOperate = ( FP_QUEST_OPERATE_DELAY * )tEventData.m_Data3;
	QUEST * pQuest = ( QUEST * )tEventData.m_Data1;
	UNITID idUnit = ( UNITID )tEventData.m_Data2;

	pfOperate( pQuest, idUnit );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestOperateDelay(
	QUEST * pQuest,
	FP_QUEST_OPERATE_DELAY * pfCallback,
	UNITID idUnit,
	int nTicks )
{
	ASSERTX_RETURN( pQuest, "Quest needed" );
	ASSERTX_RETURN( pfCallback, "Operate Delay callback function needed" );
	ASSERTX_RETURN( idUnit != INVALID_ID, "Operate Delay needs operated unit id" );
	ASSERTX_RETURN( nTicks > 0, "Operate Delay must be > 0" );

	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ), "Server only" );

	// get rid of any old ones
	UnitUnregisterTimedEvent( pPlayer, sOperateDone );

	// set the state and data on the player
	// must set stat first
	UnitSetStat( pPlayer, STATS_QUEST_OPERATE_DELAY_IN_TICKS, nTicks );
	// then state
	s_StateSet( pPlayer, pPlayer, STATE_OPERATE_OBJECT_DELAY, 0 );
	UnitRegisterEventTimed( pPlayer, sOperateDone, &EVENT_DATA( (DWORD_PTR)pQuest, (DWORD_PTR)idUnit, (DWORD_PTR)pfCallback ), nTicks );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestNonPlayerOperateDelay(
	QUEST * pQuest,
	UNIT * pUnit,
	FP_QUEST_OPERATE_DELAY * pfCallback,
	UNITID idUnitOperated,
	int nTicks )
{
	ASSERTX_RETURN( pQuest, "Quest needed" );
	ASSERTX_RETURN( pfCallback, "Operate Delay callback function needed" );
	ASSERTX_RETURN( idUnitOperated != INVALID_ID, "Operate Delay needs operated unit id" );
	ASSERTX_RETURN( nTicks > 0, "Operate Delay must be > 0" );

	ASSERT_RETURN( pUnit );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pUnit ) ), "Server only" );

	// get rid of any old ones
	UnitUnregisterTimedEvent( pUnit, sOperateDone );

	// set the state and data on the player
	// must set stat first
	UnitSetStat( pUnit, STATS_QUEST_OPERATE_DELAY_IN_TICKS, nTicks );
	// then state
	s_StateSet( pUnit, pUnit, STATE_OPERATE_OBJECT_DELAY_MONSTER, 0 );
	UnitRegisterEventTimed( pUnit, sMonsterOperateDone, &EVENT_DATA( (DWORD_PTR)pQuest, (DWORD_PTR)idUnitOperated, (DWORD_PTR)pfCallback ), nTicks );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestsDroppedItem( 
	UNIT *pDropper, 
	int nItemClass)
{
	ASSERTX_RETURN( pDropper, "Expected unit" );
	ASSERTX_RETURN( nItemClass != INVALID_LINK, "Expected item class" );
	ASSERTX_RETURN( AppIsHellgate(), "Hellgate only" );
	
	// only care for players
	if (UnitIsPlayer( pDropper ))
	{
		// is this the init new player?
		if ( !pDropper->m_pQuestGlobals )
			return;

		// go through each quest
		int nDifficulty = UnitGetStat( pDropper, STATS_DIFFICULTY_CURRENT );
		for (int i = 0; i < NUM_QUESTS; ++i)
		{
			QUEST *pQuest = QuestGetQuestByDifficulty( pDropper, i, nDifficulty );
			if (pQuest)
			{
			
				// does this quest care about items of this type
				if (QuestUsesUnitClass( pQuest, GENUS_ITEM, nItemClass ))
				{					
					s_sQuestEventDroppedItem(
						pQuest,
						pDropper,
						nItemClass);

					// yes it does, give cast members a chance to update
					s_QuestUpdateCastInfo( pQuest );

				}
				
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestEndTrainRide( GAME * pGame )
{
	if ( !IS_SERVER( pGame ) )
		return;
#if HELLGATE_ONLY
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, QUEST_RESCUERIDE );
		if ( !quest )
			continue;

		s_QuestTrainRideOver( quest );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestAIUpdate(
	UNIT * pMonster,
	int nQuestIndex )
{
	ASSERTX_RETURN( pMonster, "Monster needed for game-level AI states" );
	GAME * pGame = UnitGetGame( pMonster );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// find first client in quest game...
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT * player = ClientGetPlayerUnit( pClient );
		ASSERTX_CONTINUE( player, "No player for in game client" );

		// get quest
		QUEST * quest = QuestGetQuest( player, nQuestIndex );
		if ( !quest )
			continue;

		if ( QuestGamePlayerInGame( player ) )
		{
			// setup message
			QUEST_MESSAGE_GAME_MESSAGE tMessage;
			tMessage.nCommand = (int)QGM_GAME_AIUPDATE;
			tMessage.pUnit = pMonster;
			tMessage.dwData = 0;

			// send to quest this quest
			QuestSendMessageToQuest( quest, QM_SERVER_GAME_MESSAGE, &tMessage );
			return;
		}
		else
		{
			// send message and have this player run the game-level AI
			QUEST_MESSAGE_AI_UPDATE tMessage;
			tMessage.pUnit = pMonster;

			// send to quest this quest
			QuestSendMessageToQuest( quest, QM_SERVER_AI_UPDATE, &tMessage );
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestSkillNotify(
	UNIT * pPlayer,
	int nQuestIndex,
	int nSkillIndex )
{
	ASSERTX_RETURN( pPlayer, "Player needed for Quest skill notification" );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// get quest
	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestIndex );
	if ( !pQuest )
		return;

	// send message and have this player run the game-level AI
	QUEST_MESSAGE_SKILL_NOTIFY tMessage;
	tMessage.nSkillIndex = nSkillIndex;

	// send to quest this quest
	QuestSendMessageToQuest( pQuest, QM_SERVER_SKILL_NOTIFY, &tMessage );
}

//----------------------------------------------------------------------------
// Manually activates the "track quest" check box functionality in the quest log.
//----------------------------------------------------------------------------
void s_QuestTrack( QUEST * pQuest, 
	BOOL bEnableTracking)
{
	ASSERT_RETURN(pQuest);
	UNIT * pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETURN(pPlayer);
	if (!bEnableTracking || QuestIsActive(pQuest))
	{
		int nDifficulty = QuestGetDifficulty( pQuest );
		UnitSetStat(pPlayer, STATS_QUEST_PLAYER_TRACKING, pQuest->nQuest, nDifficulty, bEnableTracking);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestBackFromMovie(
	UNIT * pPlayer )
{
	// setup message
	QUEST_MESSAGE_BACK_FROM_MOVIE tMessage;
	tMessage.idPlayer = UnitGetId( pPlayer );

	// send to quests
	MetagameSendMessage( pPlayer, QM_SERVER_BACK_FROM_MOVIE, &tMessage, MAKE_MASK( QSMF_BACK_FROM_MOVIE ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestVersionPlayer(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// get current flags
	DWORD dwFlags = UnitGetStat( pPlayer, STATS_QUEST_GLOBAL_FIX_FLAGS );

	// delete old quest items
	if ( TESTBIT( dwFlags, QGFF_OLD_QUEST_ITEMS ) == 0 ||
		 TESTBIT( dwFlags, QGFF_OLD_PORTALBOOST_QUEST_ITEMS ) == 0 )
	{
		s_QuestRemoveAllOldQuestItems( pPlayer );
		SETBIT( dwFlags, QGFF_OLD_QUEST_ITEMS );
		SETBIT( dwFlags, QGFF_OLD_PORTALBOOST_QUEST_ITEMS );
	}

	// give people the fawkes blueprint
	if ( TESTBIT( dwFlags, QGFF_BROKEN_FAWKES_DAY_QUEST_REWARD ) == 0 )
	{
		s_QuestFixFawkes( pPlayer );
		SETBIT( dwFlags, QGFF_BROKEN_FAWKES_DAY_QUEST_REWARD );
	}

	// save current flags
	UnitSetStat( pPlayer, STATS_QUEST_GLOBAL_FIX_FLAGS, dwFlags );
	
}
