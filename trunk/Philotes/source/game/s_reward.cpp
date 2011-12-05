
//----------------------------------------------------------------------------
// FILE: s_reward.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "clients.h"
#include "inventory.h"
#include "items.h"
#include "offer.h"
#include "s_message.h"
#include "s_reward.h"
#include "stats.h"
#include "tasks.h"
#include "units.h"
#include "npc.h"

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void OfferActionsInit( 
	OFFER_ACTIONS &tActions)
{
	tActions.pfnItemTaken = NULL;
	tActions.pItemTakenUserData = NULL;
	tActions.pfnAllTaken = NULL;
	tActions.pAllTakenUserData = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTakeItemInit( 
	TAKE_ITEM &tTakeItem)
{
	tTakeItem.nItemsLeftInSlot = -1;
	tTakeItem.pItem = NULL;
	tTakeItem.pTaker = NULL;
	tTakeItem.pUserData = NULL;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RewardSessionInit(
	SREWARD_SESSION *pRewardSession)
{
	ASSERTX_RETURN( pRewardSession, "Expected trade session" );
	
	OfferActionsInit( pRewardSession->tActions );
	sTakeItemInit( pRewardSession->tTakeItem );
	pRewardSession->nNumTaken = 0;
	pRewardSession->nMaxTakes = REWARD_TAKE_ALL;
	pRewardSession->nMoneyOffer = 0;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SREWARD_SESSION *s_RewardSessionGet(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	ASSERTX_RETNULL( UnitHasClient( pUnit ), "Can only get trade sessions for units that have clients" );
	GAMECLIENT *pClient = UnitGetClient( pUnit );
	return pClient->m_pRewardSession;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemRemovedFromActiveReward(
	UNIT *pItem)
{

	// item is no longer in offering slot
	UnitClearStat( pItem, STATS_IN_ACTIVE_OFFERING, 0 );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRewardNoItems( 
	UNIT *pUnit,
	SREWARD_SESSION *pSession)
{
	const OFFER_ACTIONS *pActions = &pSession->tActions;

	// do callback for all taken	
	if (pActions->pfnAllTaken != NULL)
	{
		pActions->pfnAllTaken( pActions->pAllTakenUserData );
	}

	// automatically close the reward ui
	s_TradeCancel( pUnit, TRUE );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RewardOpen(
	INVENTORY_STYLE eStyle,
	UNIT *pUnit,
	UNIT *pOfferer,
	UNIT **pRewards,
	int nNumRewards,
	int nOriginalMaxTakes,
	int nNumTaken,
	int nOfferDefinition,
	const OFFER_ACTIONS &tActions)
{

	// given the original max takes allowed ... and the number taken, what is the 
	// current takes left the player is allowed to have
	int nTakesLeft = nOriginalMaxTakes;
	if (nTakesLeft != REWARD_TAKE_ALL)
	{
		nTakesLeft = nOriginalMaxTakes - nNumTaken;
	}
	
	// adjust takes left if there aren't enough items to make it matter
	if (nNumRewards <= nTakesLeft)
	{
		nTakesLeft = REWARD_TAKE_ALL;
	}
	
	// save the callback
	SREWARD_SESSION *pRewardSession = s_RewardSessionGet( pUnit );

	// init session
	s_RewardSessionInit( pRewardSession );
	
	// save actions (if any)
	pRewardSession->tActions = tActions;
	pRewardSession->nNumTaken = nNumTaken;
	pRewardSession->nMaxTakes = nTakesLeft;
		
	// get the reward location for this unit
	int nRewardLocation = InventoryGetRewardLocation( pUnit );
	ASSERTX_RETURN( UnitInventoryGetCount( pUnit, nRewardLocation ) == 0, "Reward inventory is not empty but should be" );
	
	// iterate the rewards
	int nItemCount = 0;
	for (int i = 0; i < nNumRewards; ++i)
	{
		UNIT *pItem = pRewards[ i ];

		// save where the reward is coming from
		int nLocation;
		UnitGetInventoryLocation( pItem, &nLocation );

		// save on the item, where it came from so we can put it back there if we need to
		UnitSetStat( pItem, STATS_REWARD_ORIGINAL_LOCATION, nLocation );

		// this item is now in the offering area
		UnitSetStat( pItem, STATS_IN_ACTIVE_OFFERING, TRUE );			

		// find a place for the item
		int x, y;
		if (UnitInventoryFindSpace( pUnit, pItem, nRewardLocation, &x, &y ))
		{
			// move the item to the reward inventory
			InventoryChangeLocation( pUnit, pItem, nRewardLocation, x, y, 0 );
		}
		else
		{
			ASSERTX( 0, "Could not find space in the reward inventory" );
		}

		// we have another item
		nItemCount++;

	}

	// save take item info
	TAKE_ITEM *pTakeItem = &pRewardSession->tTakeItem;
	pTakeItem->pUserData = tActions.pItemTakenUserData;
	pTakeItem->nItemsLeftInSlot = nItemCount;

	// setup trade spec
	TRADE_SPEC tTradeSpec;
	tTradeSpec.eStyle = eStyle;
	tTradeSpec.nOfferDefinition = nOfferDefinition;
	tTradeSpec.nNumRewardTakes = nTakesLeft;
	tTradeSpec.bNPCAllowsSimultaneousTrades = (UnitIsPlayer( pUnit ) && !UnitIsPlayer( pOfferer ));

	
	// start trading with the either the offerer or yourself
	UNIT *pTradingWith = pOfferer ? pOfferer : pUnit;
	s_TradeStart( pUnit, pTradingWith, tTradeSpec );

	// if we have no items, and we have a callback to do when we take all the items
	// do that callback now ... we might want to change this behavior to distinguish
	// between taking items when there were items and the case where there were
	// never items to begin with -Colin
	if (nItemCount == 0)
	{
		sRewardNoItems( pUnit, pRewardSession);
	}
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RewardCancel(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// close the reward offer
	s_RewardClose( pUnit );
	
	// cancel the trade
	s_TradeCancel( pUnit, FALSE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RewardClose(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// get the reward location
	int nRewardLocation = InventoryGetRewardLocation( pUnit );

	// take any rewards that are left in the reward slot and put them back
	// into the slot they originally came from
	UNIT *pItem = UnitInventoryLocationIterate( pUnit, nRewardLocation, NULL, 0, FALSE );
	UNIT *pNext = NULL;
	while (pItem)
	{
	
		// get next
		pNext = UnitInventoryLocationIterate( pUnit, nRewardLocation, pItem, 0, FALSE );
		
		// get the location the item used to be in
		int nOriginalLocation = UnitGetStat( pItem, STATS_REWARD_ORIGINAL_LOCATION );
		ASSERTX_CONTINUE( nOriginalLocation != INVLOC_NONE, "Reward item has no original location" );
		
		// find a spot for it
		int x, y;
		if (UnitInventoryFindSpace( pUnit, pItem, nOriginalLocation, &x, &y ) == FALSE)
		{
			ASSERTX_CONTINUE( 0, "Unable to find space in original location for reward item" );
		}
		
		// put in location
		InventoryChangeLocation( pUnit, pItem, nOriginalLocation, x, y, 0 );
	
		// item has been removed from active reward
		sItemRemovedFromActiveReward( pItem );

		// goto next
		pItem = pNext;
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RewardTakeAll(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	int nRewardLocation = InventoryGetRewardLocation( pUnit );

	// see if we can take all the items
	if ( s_RewardCanTakeAll( pUnit ) == FALSE)
	{
	
		// send can't pickup
		UNIT *pItem = UnitInventoryLocationIterate( pUnit, nRewardLocation, NULL, 0, FALSE );
		if (pItem)
		{
			s_ItemSendCannotPickup( pUnit, pItem, PR_FULL_ITEMS );
		}
		
	}
	else
	{

		// make sure the reward dialog closes, which triggers quest completion, note that
		// since this is the reward take all we specifically will not cancel any offers
		s_NPCTalkStop( pGame, pUnit, CCT_OK, -1, FALSE );

		// you can't do this if the trade session isn't a take all session
		SREWARD_SESSION *pRewardSession = s_RewardSessionGet( pUnit );
		ASSERT_RETURN(pRewardSession);

		// take any rewards that are left in the reward slot and put them back
		// into the slot they originally came from
		UNIT *pItem = UnitInventoryLocationIterate( pUnit, nRewardLocation, NULL, 0, FALSE );
		
		// don't store pointers while iterating, items may be removed if the player
		// only gets to pick a limited subset of items (usually 1 of 3)
		UNITID idItem = pItem ? UnitGetId( pItem ) : INVALID_ID;
		while (idItem != INVALID_ID &&
			   (pRewardSession->nMaxTakes == REWARD_TAKE_ALL ||
				pRewardSession->nNumTaken < pRewardSession->nMaxTakes))
		{
			UNITID idNext = INVALID_ID;
			pItem = UnitGetById( pGame, idItem );
			if ( pItem )
			{
				// get next
				UNIT *pNext = UnitInventoryLocationIterate( pUnit, nRewardLocation, pItem, 0, FALSE );
				if ( pNext )
					idNext = UnitGetId( pNext );

				// find a location for the item
				//  If you can't take them all, only take the ones you can use.
				int nInvSlot, nX, nY;		
				if (ItemGetOpenLocation( pUnit, pItem, TRUE, &nInvSlot, &nX, &nY ) &&
					(pRewardSession->nMaxTakes == REWARD_TAKE_ALL ||
					(InventoryItemMeetsClassReqs(pItem, pUnit))))
				{
					InventoryChangeLocation( pUnit, pItem, nInvSlot, nX, nY, 0 );		
				}
			}
			
			// goto next
			idItem = idNext;
		}

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RewardCanTakeAll(
	UNIT *pUnit)
{

	// Don't allow the player to do this with an item in their cursor
	UNIT *pCursorItem = UnitInventoryLocationIterate( pUnit, INVLOC_CURSOR, NULL, 0, FALSE );
	if (pCursorItem)
	{
		return FALSE;
	}

	// you can't do this if the trade session isn't a take all session
	SREWARD_SESSION *pRewardSession = s_RewardSessionGet( pUnit );
	ASSERT_RETFALSE(pRewardSession);

	// get the reward location
	int nRewardLocation = InventoryGetRewardLocation( pUnit );

	// take any rewards that are left in the reward slot and put them back
	// into the slot they originally came from
	GAME *pGame = UnitGetGame( pUnit );
	UNIT *pItem = UnitInventoryLocationIterate( pUnit, nRewardLocation, NULL, 0, FALSE );

	// don't store pointers while iterating, items may be removed if the player
	// only gets to pick a limited subset of items (usually 1 of 3)
	UNITID idItem = pItem ? UnitGetId( pItem ) : INVALID_ID;
	while (idItem != INVALID_ID &&
		(pRewardSession->nMaxTakes == REWARD_TAKE_ALL ||
		pRewardSession->nNumTaken < pRewardSession->nMaxTakes))
	{
		UNITID idNext = INVALID_ID;
		pItem = UnitGetById( pGame, idItem );
		if ( pItem )
		{
			// get next
			UNIT *pNext = UnitInventoryLocationIterate( pUnit, nRewardLocation, pItem, 0, FALSE );
			if ( pNext )
				idNext = UnitGetId( pNext );

			// find a location for the item
			//  If you can't take them all, only take the ones you can use.
			int nInvSlot, nX, nY;		
			if (!ItemGetOpenLocation( pUnit, pItem, TRUE, &nInvSlot, &nX, &nY ) &&
				(pRewardSession->nMaxTakes == REWARD_TAKE_ALL ||
				(InventoryItemMeetsClassReqs(pItem, pUnit))))
			{
				return FALSE;
			}
		}

		// goto next
		idItem = idNext;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDestroyRemainingRewards(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// get reward location
	int nRewardLocation = InventoryGetRewardLocation( pUnit );

	// iterate units in reward location and destroy them all
	UNIT *pItem = UnitInventoryLocationIterate( pUnit, nRewardLocation, NULL );
	UNIT *pNext = NULL;
	while (pItem)
	{
		pNext = UnitInventoryLocationIterate( pUnit, nRewardLocation, pItem );
		sItemRemovedFromActiveReward( pItem );		
		UnitFree( pItem, UFF_SEND_TO_CLIENTS );
		pItem = pNext;		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_RewardItemTaken(
	UNIT *pUnit,
	UNIT *pItem)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pItem, "Expected item" );	

	// do nothing if item was not taken to a standard slot
	if (InventoryIsInLocationType( pItem, LT_STANDARD ) == FALSE)
	{
		return;
	}
	
	// item removed from system
	sItemRemovedFromActiveReward( pItem );
				
	// get trade session
	SREWARD_SESSION *pRewardSession = s_RewardSessionGet( pUnit );

	// have now taken one more item
	pRewardSession->nNumTaken++;
	
	// there is now one less item in the slot
	TAKE_ITEM *pTakeItem = &pRewardSession->tTakeItem;
	pTakeItem->nItemsLeftInSlot--;

	// do offer tracking (must occur before callback as the callback may unlink the item)
	int nDifficulty = UnitGetStat( pUnit, STATS_DIFFICULTY_CURRENT );
	int nOffer = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
	if (nOffer != INVALID_LINK)
	{
		s_OfferItemTaken( pUnit, pItem );
	}

	// do task reward tracking (must occur before callback as the callback may unlink the item)
	GAMETASKID idTask = UnitGetStat( pItem, STATS_TASK_REWARD );
	if (idTask != GAMETASKID_INVALID)
	{
		s_TaskRewardItemTaken( pUnit, pItem );
	}
			
	// do callback (if any)
	OFFER_ACTIONS *pActions = &pRewardSession->tActions;
	if (pActions->pfnItemTaken != NULL)
	{
		
		// init take item info
		pTakeItem->pTaker = pUnit;
		pTakeItem->pItem = pItem;
		pTakeItem->pUserData = pActions->pItemTakenUserData;
						
		// do callback
		pActions->pfnItemTaken( *pTakeItem );
		
	}

	// tell client another item has been taken (if we have a limit and they haven't taken them all yet)
	if (pRewardSession->nMaxTakes != REWARD_TAKE_ALL &&
		pRewardSession->nNumTaken != pRewardSession->nMaxTakes)
	{
		MSG_SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN tMessage;
		tMessage.nNumTaken = pRewardSession->nNumTaken;
		tMessage.nMaxTakes = pRewardSession->nMaxTakes;
		tMessage.nOfferDefinition = nOffer;
		s_SendUnitMessageToClient( pUnit, UnitGetClientId( pUnit ), SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN, &tMessage );
	}
	
	// if the player has a limited choice of items they can take, see if they
	// chose all that we're allowing them to
	if (pRewardSession->nMaxTakes != REWARD_TAKE_ALL &&
		pRewardSession->nNumTaken >= pRewardSession->nMaxTakes)
	{
		sDestroyRemainingRewards( pUnit );
		pTakeItem->nItemsLeftInSlot = 0;		
	}
	
	// if there are no more items, do the all items taken callback
	if (pTakeItem->nItemsLeftInSlot == 0)
	{
		sRewardNoItems( pUnit, pRewardSession );
	}
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
