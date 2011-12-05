//----------------------------------------------------------------------------
// FILE: gossip.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "dialog.h"
#include "gossip.h"
#include "interact.h"
#include "npc.h"
#include "picker.h"
#include "quests.h"
#include "Quest.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static const int MAX_GOSSIP = 128;  // arbitrary

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const GOSSIP_DATA *GossipGetData(
	int nGossip)
{
	return (const GOSSIP_DATA *) ExcelGetData( NULL, DATATABLE_GOSSIP, nGossip );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetAvailableGossip(
	UNIT *pPlayer,
	UNIT *pNPC,
	int *nGossipBuffer,
	int nBufferSize)
{
	ASSERTX_RETVAL( pPlayer, UNIT_INTERACT_NONE, "Invalid player" );
	ASSERTX_RETVAL( pNPC, UNIT_INTERACT_NONE, "Invalid npc" );
	ASSERTX_RETVAL( UnitCanInteractWith( pNPC, pPlayer ), UNIT_INTERACT_NONE, "Unit cannot interact with player" );
	ASSERTX_RETVAL( UnitGetGenus( pNPC ) == GENUS_MONSTER, UNIT_INTERACT_NONE, "Expected monster" );
	int nNPCMonsterClass = UnitGetClass( pNPC );
		
	// init our count
	int nNumAvailable = 0;

	// go through all rows in the gossip table
	int nGossipRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_GOSSIP);
	int topPriority( 0 );
	for (int nGossip = 0; nGossip < nGossipRows; ++nGossip)
	{
		const GOSSIP_DATA *pGossip = GossipGetData( nGossip );
		ASSERT_CONTINUE(pGossip);

		// dialog must be valid
		if (pGossip->nDialog == INVALID_LINK)
		{
			continue;
		}

		// skip rows that don't match the unit		
		if ( pGossip->nNPCMonsterClass != nNPCMonsterClass )
		{
			continue;
		}

		// check for quest gossip
		if (pGossip->nQuest != INVALID_ID)
		{		

			QUEST *pQuest = QuestGetQuest( pPlayer, pGossip->nQuest );

			// some quest gossip requires a particular status of the quest
			if (pQuest && pGossip->eQuestStatus != QUEST_STATUS_INVALID)
			{
				if (QuestGetStatus( pQuest ) != pGossip->eQuestStatus)
				{
					continue;
				}
			}
		}

		if( AppIsTugboat() )
		{
			if( pGossip->nPriority < topPriority )
				continue;
			if( UnitGetStat( pPlayer, STATS_LEVEL )< pGossip->nPlayerLevel )
				continue;
		}


		if(pGossip->nQuestTugboat != INVALID_ID )
		{
			const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( pGossip->nQuestTugboat );
			ASSERT_CONTINUE( pQuestTask );
			BOOL ignoreGossip( TRUE );
			switch( pGossip->eQuestStatus )
			{
			case QUEST_STATUS_ACTIVE:
				ignoreGossip = !QuestPlayerHasTask( pPlayer, pQuestTask );
				break;
			default:
			case QUEST_STATUS_COMPLETE:
				ignoreGossip = !QuestIsQuestTaskComplete( pPlayer, pQuestTask );
				break;
			}

			if( ignoreGossip )
				continue;			
		}

				
		// add to buffer
		if( AppIsHellgate() )
		{
			ASSERTX_RETVAL( nNumAvailable < nBufferSize - 1, nNumAvailable, "Gossip buffer too small" );
			nGossipBuffer[ nNumAvailable++ ] = nGossip;
		}
		else if( AppIsTugboat() )
		{
			if( topPriority < pGossip->nPriority )
			{
				nNumAvailable = 0;
				nGossipBuffer[ nNumAvailable++ ] = nGossip;
				topPriority = pGossip->nPriority;
			}
			else if( topPriority == pGossip->nPriority )
			{
				ASSERTX_RETVAL( nNumAvailable < nBufferSize - 1, nNumAvailable, "Gossip buffer too small" );
				nGossipBuffer[ nNumAvailable++ ] = nGossip;
			}
		}
				
	}

	// return how many we found
	return nNumAvailable;

}

//----------------------------------------------------------------------------
// TODO: make this cycle possible gossips instead of randomly choosing... 
// not sure where to store what has been seen...
//----------------------------------------------------------------------------
UNIT_INTERACT s_GossipStart(
	UNIT *pPlayer,
	UNIT *pNPC)
{
	if( UnitGetGenus( pNPC ) == GENUS_ITEM )
		return UNIT_INTERACT_NONE;
	ASSERTX_RETVAL( pPlayer, UNIT_INTERACT_NONE, "Invalid player" );
	ASSERTX_RETVAL( pNPC, UNIT_INTERACT_NONE, "Invalid npc" );
	ASSERTX_RETVAL( UnitCanInteractWith( pNPC, pPlayer ), UNIT_INTERACT_NONE, "Unit cannot interact with player" );
	ASSERTX_RETVAL( UnitGetGenus( pNPC ) == GENUS_MONSTER, UNIT_INTERACT_NONE, "Expected monster unit" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// get the available gossip
	int nGossipAvailable[ MAX_GOSSIP ];
	int nGossipCount = sGetAvailableGossip( pPlayer, pNPC, nGossipAvailable, MAX_GOSSIP );

	// do we have any gossip actions available	
	if (nGossipCount > 0)
	{
		int nPick = RandGetNum( pGame, 0, nGossipCount - 1 );
		int nGossip = nGossipAvailable[ nPick ];
		ASSERTX_RETVAL( nGossip != INVALID_ID, UNIT_INTERACT_NONE, "Expected gossip index" );
		
		const GOSSIP_DATA *ptGossipData = GossipGetData( nGossip );
		int nDialog = ptGossipData->nDialog;
		ASSERTX_RETVAL( nDialog != INVALID_LINK, UNIT_INTERACT_NONE, "Invalid dialog in gossip" );
		s_NPCTalkStart( pPlayer, pNPC, NPC_DIALOG_OK, nDialog );
		return UNIT_INTERACT_TALK;
	
	}
	
	// no gossip
	return UNIT_INTERACT_NONE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_GossipHasGossip(
	UNIT *pPlayer,
	UNIT *pNPC)
{

	// get the available gossip
	int nGossipAvailable[ MAX_GOSSIP ];
	int nGossipCount = sGetAvailableGossip( pPlayer, pNPC, nGossipAvailable, MAX_GOSSIP );

	// do we have at least one thing to say
	return nGossipCount > 0;
	
}
	