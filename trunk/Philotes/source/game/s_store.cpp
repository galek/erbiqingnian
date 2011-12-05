//----------------------------------------------------------------------------
// FILE: s_store.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "clients.h"
#include "globalindex.h"
#include "items.h"
#include "npc.h"
#include "quests.h"
#include "s_message.h"
#include "s_store.h"
#include "s_trade.h"
#include "treasure.h"
#include "unit_priv.h"
#include "units.h"
#include "Quest.h"
#include "gameglobals.h"
#include "Economy.h"
#include "Currency.h"
#include "statssave.h"
#if ISVERSION(SERVER_VERSION)
#include "s_store.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// File globals
//----------------------------------------------------------------------------
static const int MAX_MERCHANTS = 256;  // arbitrary


//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sDestroyItemsFromMerchant(
	UNIT *pPlayer, 
	int nMerchantClass,
	DWORD dwInvIterateFlags)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	int nDestroyedCount = 0;

	// iterate all items in the all personal store slots
	UNIT *pItem = UnitInventoryIterate( pPlayer, NULL, dwInvIterateFlags );
	UNIT *pNextItem = NULL;
	while (pItem)
	{
	
		// get next
		pNextItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags );
		
		// free item if it's from this merchant
		if (ItemBelongsToSpecificMerchant( pItem, nMerchantClass ))
		{
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );
			nDestroyedCount++;
		}
		
		// go to next
		pItem = pNextItem;
		
	}

	return nDestroyedCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sDestroyGoodsFromMerchant(
	UNIT *pPlayer,
	int nMerchantClass)
{	
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	int nDestroyedItemCount = 0;
	
	// destroy warehouse items
	nDestroyedItemCount += sDestroyItemsFromMerchant( pPlayer, nMerchantClass, MAKE_MASK( IIF_MERCHANT_WAREHOUSE_ONLY_BIT ) );
	
	// destroy any in the active store
	nDestroyedItemCount += sDestroyItemsFromMerchant( pPlayer, nMerchantClass, MAKE_MASK( IIF_ACTIVE_STORE_SLOTS_ONLY_BIT ) );

	return nDestroyedItemCount;  // return the # of items destroyed
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExpireOldestStore( 
	UNIT *pPlayer,
	int nInvLocWarehouse, 
	UNIT *pMerchantIgnore)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nInvLocWarehouse != INVLOC_NONE, "Expected warehouse location" );
	ASSERTX_RETFALSE( pMerchantIgnore, "Expected merchant ignore unit" );

	// find the merchant class of the oldest store
	// get all of the merchants that we've visited
	STATS_ENTRY tStatsList[ MAX_MERCHANTS ];
	int nNumMerchants = UnitGetStatsRange(
		pPlayer, 
		STATS_MERCHANT_REFRESH_TICK, 
		0, 
		tStatsList, 
		MAX_MERCHANTS);
	
	// go through all merchants
	int nMerchantClassOldest = INVALID_LINK;
	DWORD dwOldestRefreshTick = 0;
	for (int i = 0; i < nNumMerchants; ++i)
	{
	
		// get merchant monster class
		int nMerchantClass = tStatsList[ i ].GetParam();

		// don't consider the ignore merchant
		if (nMerchantClass != UnitGetClass( pMerchantIgnore ))
		{
		
			// when was this store last refreshed
			DWORD dwLastRefreshTick = tStatsList[ i ].value;
			if (nMerchantClassOldest == INVALID_LINK || dwLastRefreshTick < dwOldestRefreshTick)
			{
				nMerchantClassOldest = nMerchantClass;
				dwOldestRefreshTick = dwLastRefreshTick;
			}
			
		}
			
	}

	// expire store from this merchant
	if (nMerchantClassOldest != INVALID_LINK)
	{
		return sDestroyGoodsFromMerchant( pPlayer, nMerchantClassOldest ) != 0;
	}
	else
	{
		return FALSE;  // did not expire any store
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ITEM_SPAWN_ACTION sAddItemToMerchantWarehouse(
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pItem,
	int nItemClass,
	ITEM_SPEC *pItemSpawnSpec, 
	ITEMSPAWN_RESULT *pSpawnResult,
	void *pUserData)
{
	ASSERTX_RETVAL( pItem, ITEM_SPAWN_DESTROY, "Expected item unit" );
	ASSERTX_RETVAL( pPlayer, ITEM_SPAWN_DESTROY, "Expected unit" );	
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), ITEM_SPAWN_DESTROY, "Expected player" );	
		
	// get merchant
	UNIT *pMerchant = pItemSpawnSpec->pMerchant;
	ASSERTX_RETVAL( pMerchant, ITEM_SPAWN_DESTROY, "Expected unit" );
	ASSERTX_RETVAL( UnitIsMerchant( pMerchant ), ITEM_SPAWN_DESTROY, "Expected merchant unit" );

	// if the merchant only makes items that are good for this player class,
	// destroy any items that aren't usable by this class
	if (InventoryItemMeetsClassReqs( pItem, pPlayer ) == FALSE)
	{
		return ITEM_SPAWN_DESTROY;
	}

	// tie the item to the merchant that generated it
	int nMerchantClass = UnitGetClass( pMerchant );
	UnitSetStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS, nMerchantClass );

	// put it into the personal store inventory location
	int nInvLocWarehouse = GlobalIndexGet( GI_INVENTORY_LOCATION_MERCHANT_WAREHOUSE );		
	ASSERTX_RETVAL( nInvLocWarehouse != INVALID_LINK, ITEM_SPAWN_DESTROY, "Expected inventory location for merchant warehouse" );
	
	if (InventoryMoveToAnywhereInLocation( pPlayer, pItem, nInvLocWarehouse, CLF_ALLOW_NEW_CONTAINER_BIT ) == FALSE)
	{
	
		// unable to add item to warehouse, perhaps we have too many items from other
		// merchants in our inventory slot ... lets expire the oldest merchant
		if (sExpireOldestStore( pPlayer, nInvLocWarehouse, pMerchant ) == TRUE)
		{
			
			// OK, we have freed up some space, try again
			if (InventoryMoveToAnywhereInLocation( pPlayer, pItem, nInvLocWarehouse, CLF_ALLOW_NEW_CONTAINER_BIT ) == FALSE)
			{
				ASSERTX( 0, "Unable to add item item to merchant warehouse, even after expiring oldest store" );
				return ITEM_SPAWN_DESTROY;			
			}
			
		}
		else
		{				
			ASSERTX( 0, "Unable to add item to merchant warehouse and there were no old stores to expire" );
			return ITEM_SPAWN_DESTROY;
		}
	}

	// item is ready to send messages
	UnitSetCanSendMessages( pItem, TRUE );
	
	// send unit to client
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	s_SendUnitToClient( pItem, pClient );
	
	return ITEM_SPAWN_OK;
			
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sCreateMerchantGoodsInWarehouse(
	UNIT *pPlayer,
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pMerchant, "Expected unit" );
	ASSERTX_RETURN( UnitIsMerchant( pMerchant ) , "Expected merchant" );
		
	// only proceed if we have starting treasure
	const UNIT_DATA *pMerchantUnitData = UnitGetData( pMerchant );	
	if (pMerchantUnitData->nStartingTreasure != INVALID_LINK)
	{
			
		// create contents
		ITEM_SPEC tItemSpec;
		if( !UnitIsGambler( pMerchant ) )
		{
			SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );
		}
		SETBIT( tItemSpec.dwFlags, ISF_AVAILABLE_AT_MERCHANT_ONLY_BIT );
		tItemSpec.pMerchant = pMerchant;
		tItemSpec.pfnSpawnCallback = sAddItemToMerchantWarehouse;
		s_TreasureSpawnClass( pPlayer, pMerchantUnitData->nStartingTreasure, &tItemSpec, NULL, UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT) );

	}

	// save the last tick the shop was refreshed on
	GAME *pGame = UnitGetGame( pMerchant );
	DWORD dwNow = GameGetTick( pGame );
	int nMerchantClass = UnitGetClass( pMerchant );	
	UnitSetStat( pPlayer, STATS_MERCHANT_REFRESH_TICK, nMerchantClass, dwNow );
		
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StoreTryExpireOldItems( 
	UNIT *pPlayer,
	BOOL bForce)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );

	// get all of the merchants that we've visited
	STATS_ENTRY tStatsList[ MAX_MERCHANTS ];
	int nNumMerchants = UnitGetStatsRange(
		pPlayer, 
		STATS_MERCHANT_REFRESH_TICK, 
		0, 
		tStatsList, 
		MAX_MERCHANTS);
	
	// go through all merchants
	for (int i = 0; i < nNumMerchants; ++i)
	{
	
		// get merchant monster class
		int nMerchantClass = tStatsList[ i ].GetParam();

		// don't expire items in store that we're currently looking at
		UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
		if (pTradingWith && 
			UnitIsA( pTradingWith, UNITTYPE_MONSTER ) && 
			UnitGetClass( pTradingWith ) == nMerchantClass)
		{
			continue;
		}

		// when was this store last refreshed
		DWORD dwLastRefreshTick = tStatsList[ i ].value;
		
		// has it been long enough since our last refresh (or not ever)
		DWORD dwNow = GameGetTick( pGame );
		if (bForce == TRUE || 
			(dwNow - dwLastRefreshTick >= MERCHANT_REFRESH_TIME_IN_TICKS))
		{
		
			// has it been long enough since we were last browsed this merchant
			DWORD dwLastBrowseTick = UnitGetStat( pPlayer, STATS_MERCHANT_LAST_BROWSED_TICK, nMerchantClass );
			if (bForce == TRUE || 
				(dwNow - dwLastBrowseTick >= MERCHANT_REFRESH_REQUIRED_TIME_BETWEEN_BROWSE_IN_TICKS))
			{
				sDestroyGoodsFromMerchant( pPlayer, nMerchantClass );
				UnitClearStat( pPlayer, STATS_MERCHANT_REFRESH_TICK, nMerchantClass );
				UnitClearStat( pPlayer, STATS_MERCHANT_LAST_BROWSED_TICK, nMerchantClass );
			}
			
		}	
		
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetStoreItemCount( 
	UNIT *pPlayer, 
	UNIT *pMerchant)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETINVALID( pMerchant, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsMerchant( pMerchant ), "Expected merchant" );
	int nMerchantClass = UnitGetClass( pMerchant );
	int nCount = 0;

	// setup flags	
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_MERCHANT_WAREHOUSE_ONLY_BIT);

	// iterate inventory
	UNIT *pItem = NULL;
	while ( (pItem = UnitInventoryIterate( pPlayer, pItem, dwFlags, FALSE )) != NULL )
	{
		if (ItemBelongsToSpecificMerchant( pItem, nMerchantClass ))
		{
			nCount++;
		}
	}
	
	// return the count we found
	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void sRefreshSharedStore(
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pMerchant, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pMerchant );
	
	// only proceed if we have starting treasure
	if (pUnitData->nStartingTreasure != INVALID_LINK)
	{
	
		// remove all items the merchant is carrying
		UnitInventoryContentsFree( pMerchant, TRUE );
		
		// re-create contents
		ITEM_SPEC tItemSpec;
		ItemSpawnSpecInit( tItemSpec );
		SETBIT( tItemSpec.dwFlags, ISF_STARTING_BIT );
		SETBIT( tItemSpec.dwFlags, ISF_PICKUP_BIT );
		s_TreasureSpawnClass( pMerchant, pUnitData->nStartingTreasure, &tItemSpec, NULL );

		// save the last tick the shop was refreshed on
		GAME *pGame = UnitGetGame( pMerchant );
		DWORD dwNow = GameGetTick( pGame );
		UnitSetStat( pMerchant, STATS_SHARED_MERCHANT_REFRESH_TICK, dwNow );
		
	}		
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanRefreshSharedMerchant(
	UNIT *pMerchant)
{
	ASSERTX_RETFALSE( pMerchant, "Expected merchant" );
	GAME *pGame = UnitGetGame( pMerchant );

	// some merchants don't refresh - such as traveling sales people
	if( UnitDataTestFlag(pMerchant->pUnitData, UNIT_DATA_FLAG_MERCHANT_NO_REFRESH) )
		return FALSE;
	
	// you cannot refresh a store if a player currently looking at it
	if (s_TradeIsAnyPlayerTradingWith( pMerchant ))
	{
		return FALSE;
	}

	// enough time must have passed since the merchant was last browsed by anybody
	DWORD dwNow = GameGetTick( pGame );
	DWORD dwLastBrowseTick = UnitGetStat( pMerchant, STATS_SHARED_MERCHANT_LAST_BROWSED_TICK );	
	if (dwNow - dwLastBrowseTick >= MERCHANT_REFRESH_REQUIRED_TIME_BETWEEN_BROWSE_IN_TICKS)
	{
	
		// must have been long enough since the store was last repopulated
		DWORD dwLastRefreshTick = UnitGetStat( pMerchant, STATS_SHARED_MERCHANT_REFRESH_TICK );
		if (dwNow - dwLastRefreshTick >= MERCHANT_REFRESH_TIME_IN_TICKS)
		{
			return TRUE;
		}
		
	}
	
	// cannot refresh
	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sTryStoreRefresh( 
	UNIT *pPlayer,
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pMerchant, "Expected unit" );

	// destroy items from *any* old merchants that we haven't seen for a while that would
	// refresh the next time we go to the store anyway
	s_StoreTryExpireOldItems( pPlayer, FALSE );
	
	// is a shared inventory merchant
	BOOL bSharedMerchant = UnitMerchantSharesInventory( pMerchant );
	
	// if there are no items from this merchant we are about to go see, make some new stuff
	if (bSharedMerchant)
	{	
		if( sCanRefreshSharedMerchant( pMerchant ))
		{
			sRefreshSharedStore( pMerchant );
		}	
	}
	else
	{
	
		// is a personal store, get the number of items that this merchant has for us
		int nNumItems = sGetStoreItemCount( pPlayer, pMerchant );
		if (nNumItems == 0)
		{
			// refresh our store contents
			sCreateMerchantGoodsInWarehouse( pPlayer, pMerchant );	
		}	
		
	}
						
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPutItemInActiveStore(
	UNIT *pItem,
	UNIT *pPlayer,
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pItem, "Expected item unit" );
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pMerchant, "Expected merchant unit" );

	GAME * game = UnitGetGame(pItem);
	ASSERT_RETURN(game);

	// get inventory index of player
	const INVENTORY_DATA * inventory_data = InventoryGetData(pPlayer);
	ASSERT_RETURN(inventory_data);

	// go through all inventory locations
	BOOL bMovedToStore = FALSE;
	for (int ii = inventory_data->nFirstLoc; ii < inventory_data->nFirstLoc + inventory_data->nNumLocs; ++ii)
	{	
		const INVLOC_DATA * invloc_data = InvLocGetData(game, ii);
		if(!invloc_data)
			continue;
	
		// get the location
		int nInvLocation = invloc_data->nLocation;

		// is this an active store slot
		if (InvLocIsStoreLocation( pPlayer, nInvLocation ))
		{
					
			// find a spot in this inventory slot
			bMovedToStore = InventoryMoveToAnywhereInLocation( 
				pPlayer, 
				pItem, 
				nInvLocation, 
				CLF_ALLOW_NEW_CONTAINER_BIT | CLF_CACHE_ITEM_ON_SEND);
			if (bMovedToStore)
			{
				break;  // don't continue searching
			}
						
		}
		
	}

	// check for errors
	ASSERTX_RETURN( bMovedToStore, "Could not move item to any active store inventory location" );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sReturnActiveStoreItemsToWarehouse(	
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );

	// get the inventory slot of the personal store stock
	int nInvLocWarehouse = GlobalIndexGet( GI_INVENTORY_LOCATION_MERCHANT_WAREHOUSE );
	ASSERTX_RETURN( nInvLocWarehouse != INVALID_LINK, "Expected inventory location for merchant warehouse" );	
	
	// go through all items here
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ACTIVE_STORE_SLOTS_ONLY_BIT );	
	UNIT *pItem = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
	UNIT *pNextItem = NULL;
	while (pItem)
	{
	
		// get next
		pNextItem = UnitInventoryIterate( pPlayer, pItem, dwFlags, FALSE );

		// return to stock
		InventoryMoveToAnywhereInLocation( pPlayer, pItem, nInvLocWarehouse, CLF_ALLOW_NEW_CONTAINER_BIT );
				
		// goto next item
		pItem = pNextItem;
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMoveMerchantGoodsToActiveStore( 
	UNIT *pPlayer, 
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Player param is not a player" );
	ASSERTX_RETURN( pMerchant, "Expected merchant" );
	ASSERTX_RETURN( UnitIsMerchant( pMerchant ), "Merchant param is not a merchant" );
	int nMerchantClass = UnitGetClass( pMerchant );

	// put any items that are lingering in the active personal store back into the warehouse
	sReturnActiveStoreItemsToWarehouse( pPlayer );
		
	// go through all items here
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_MERCHANT_WAREHOUSE_ONLY_BIT);	
	UNIT *pItem = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
	UNIT *pNextItem = NULL;
	while (pItem)
	{
	
		// get next
		pNextItem = UnitInventoryIterate( pPlayer, pItem, dwFlags, FALSE );

		// is this item from this merchant
		if (ItemBelongsToSpecificMerchant( pItem, nMerchantClass ))
		{
			// put item into active store
			sPutItemInActiveStore( pItem, pPlayer, pMerchant );
		}
		
		// goto next item
		pItem = pNextItem;
		
	}
	
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_StoreEnter(
	UNIT* pPlayer,
	UNIT* pMerchant,
	const TRADE_SPEC &tTradeSpec)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Player param is not a player" );
	ASSERTX_RETURN( pMerchant, "Expected merchant" );
	ASSERTX_RETURN( UnitIsMerchant( pMerchant ), "Merchant param is not a merchant" );

	// if player is already trading, do nothing
	if (TradeIsTrading( pPlayer ) == TRUE)
	{
		return;
	}

	// try to refresh store contents
	sTryStoreRefresh( pPlayer, pMerchant );
			
	// set flag that player is in a store
	UNITID idMerchant = UnitGetId( pMerchant );
	UnitSetStat( pPlayer, STATS_TRADING, idMerchant );

	if (UnitMerchantSharesInventory( pMerchant ))
	{
		// send the contents of the store to the client trading
		DWORD dwFlags = 0;
		SETBIT( dwFlags, ISF_USABLE_BY_CLASS_ONLY_BIT );
		GAMECLIENT *pClient = UnitGetClient( pPlayer );
		ASSERTX_RETURN( pClient, "Player entering store has no client!" );
		s_InventorySendToClient( pMerchant, pClient, dwFlags );		
	}
	else
	{
		// move this merchants inventory into the active store	
		sMoveMerchantGoodsToActiveStore( pPlayer, pMerchant );
	}
			
	// setup message to open merchant UI
	MSG_SCMD_TRADE_START tMessage;
	tMessage.idTradingWith = idMerchant;

	tMessage.tTradeSpec.eStyle = tTradeSpec.eStyle;
	// send message
	GAME* pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TRADE_START, &tMessage );	
		
}	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_StoreExit(
	UNIT *pPlayer,
	UNIT *pMerchant)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Player param is not a player" );
	GAME *pGame = UnitGetGame( pPlayer );

	// set tick the store was last browsed on
	DWORD dwNow = GameGetTick( pGame );
	BOOL bSharedMerchant = UnitMerchantSharesInventory( pMerchant );
	if (bSharedMerchant)
	{
		UnitSetStat( pMerchant, STATS_SHARED_MERCHANT_LAST_BROWSED_TICK, dwNow );
	}
	else
	{
		int nMerchantClass = UnitGetClass( pMerchant );
		UnitSetStat( pPlayer, STATS_MERCHANT_LAST_BROWSED_TICK, nMerchantClass, dwNow );
	}

	// no longer talking to merchant
	s_NPCTalkStop( pGame, pPlayer, CCT_OK, -1, FALSE );
		
	// clear store flag on player
	UnitClearStat( pPlayer, STATS_TRADING, 0 );

	// Tugboat is more aggressive about forcing cancels - we have to tie up some UI loose
	// ends on the client in certain cases.
	if( AppIsTugboat() )
	{
		if (UnitHasClient( pPlayer ) == TRUE )
		{
			MSG_SCMD_TRADE_FORCE_CANCEL tMessage;				
			GAMECLIENTID idClient = UnitGetClientId( pPlayer );		
			s_SendUnitMessageToClient( pPlayer, idClient, SCMD_TRADE_FORCE_CANCEL, &tMessage );		
		}
	}

	// move items to warehouse or remove from client
	if (bSharedMerchant)
	{
		// tell the client to forget about the inventory of the merchant, we could optimize this
		// someday if necessary so that we aren't sending and resending objects over and over
		GAMECLIENT *pClient = UnitGetClient( pPlayer );
		ASSERTX_RETURN( pClient, "Player entering store has no client!" );	

		s_InventoryRemoveFromClient( pMerchant, pClient, 0 );	
	}
	else
	{
		// move them items back to the store
		sReturnActiveStoreItemsToWarehouse( pPlayer );
	}
				
}	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetAllMerchantStoreParams(
	UNIT *pPlayer,
	int nLastRefreshTick,
	int nLastBrowsTick)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer), "Expected player" );	

	// get all of the merchants that we've visited
	STATS_ENTRY tStatsList[ MAX_MERCHANTS ];
	int nNumMerchants = UnitGetStatsRange(
		pPlayer, 
		STATS_MERCHANT_REFRESH_TICK, 
		0, 
		tStatsList, 
		MAX_MERCHANTS);
	
	// go through all merchants
	for (int i = 0; i < nNumMerchants; ++i)
	{
	
		// get merchant monster class
		int nMerchantClass = tStatsList[ i ].GetParam();

		// set the time stats
		UnitSetStat( pPlayer, STATS_MERCHANT_REFRESH_TICK, nMerchantClass, nLastRefreshTick );
		UnitSetStat( pPlayer, STATS_MERCHANT_LAST_BROWSED_TICK, nMerchantClass, nLastBrowsTick );
				
	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_StoreInit(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pUnit ), "Expected player" );	
	GAME *pGame = UnitGetGame( pUnit );
	DWORD dwNow = GameGetTick( pGame );
	
	// if we have ticks for the merchant tracking we will set them to now, this cause
	// the store to be the same from game to game, cause we don't want to encourage
	// people to quit and join games to get a new store if they don't like the items they have	
	// we do a now +1 so that we are guaranteed to keep a stat for all the merchants
	// that we have (cause 0 can clear the stats)
	sSetAllMerchantStoreParams( pUnit, dwNow + 1, dwNow + 1 );
				
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_StoreBuyItem(	
	UNIT *pPlayer,
	UNIT *&pItem,
	UNIT *pMerchant,
	const INVENTORY_LOCATION &tInvLocSuggested)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(pItem);
	ASSERT_RETFALSE(ItemGetMerchant(pItem) == pMerchant);
	
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE(IS_SERVER(pGame));	
		
	// must be a merchant
	if ( UnitIsMerchant( pMerchant ) == FALSE )
	{
		return FALSE;
	}

	// must be close enough to the merchant
	if (VectorDistanceSquared( pPlayer->vPosition, pMerchant->vPosition ) > UNIT_INTERACT_DISTANCE_SQUARED)	
	{
		return FALSE;
	}
	
	// item must have buy price
	cCurrency buyPrice = EconomyItemBuyPrice( pItem );
	cCurrency playerCurrency = UnitGetCurrency( pPlayer );
	if ( buyPrice.IsZero() )
	{
		return FALSE;
	}
	//int nBuyPrice = currency.GetCurrency( KCURRENCY_VALUE_INGAME );

	// buyer must have enough gold for it	
	if( playerCurrency < buyPrice  )
	{
		return FALSE;
	}

	// non-subscribers cannot buy subscriber-only items
	const UNIT_DATA *pItemData = UnitGetData(pItem);
	if (pItemData && UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY) && !PlayerIsSubscriber(pPlayer))
	{
		return FALSE;
	}

	INVENTORY_LOCATION tInvLoc;	
	tInvLoc.nInvLocation = INVLOC_NONE;
	tInvLoc.nX = 0;
	tInvLoc.nY = 0;
	if (tInvLocSuggested.nInvLocation != INVLOC_NONE)
	{
		// can this item go in this slot?
		if (UnitInventoryTest(
				pPlayer, 
				pItem, 
				tInvLocSuggested.nInvLocation, 
				tInvLocSuggested.nX, 
				tInvLocSuggested.nY))
		{
			// is the slot currently unoccupied?
			if (UnitInventoryGetByLocationAndXY(
					pPlayer, 
					tInvLocSuggested.nInvLocation, 
					tInvLocSuggested.nX, 
					tInvLocSuggested.nY) == NULL)
			{

				if (InvLocIsEquipLocation(tInvLocSuggested.nInvLocation, pPlayer))
				{
					DWORD dwItemEquipFlags = 0;
					SETBIT( dwItemEquipFlags, IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT );
					if (InventoryItemCanBeEquipped(pPlayer, pItem, tInvLocSuggested.nInvLocation, NULL, 0, dwItemEquipFlags ))
					{
						tInvLoc = tInvLocSuggested;
					}
				}
				else
				{
					tInvLoc = tInvLocSuggested;
				}
			}
		}
		else
		{
			// check again without the X and Y
			if (UnitInventoryFindSpace(pPlayer, pItem, tInvLocSuggested.nInvLocation, &tInvLoc.nX, &tInvLoc.nY))
			{
				tInvLoc.nInvLocation = tInvLocSuggested.nInvLocation;
			}
		}
	}

	if (tInvLoc.nInvLocation == INVLOC_NONE)
	{
		// if this returns false then there's a location, but an item is in it.  We don't want that.
		if (!ItemGetOpenLocation(pPlayer, pItem, TRUE, &tInvLoc.nInvLocation, &tInvLoc.nX, &tInvLoc.nY))
		{
			tInvLoc.nInvLocation = INVLOC_NONE;
		}
	}

	// no location found
	if (tInvLoc.nInvLocation == INVLOC_NONE)
	{
		return FALSE;
	}

	int nCurInvLocItem = INVLOC_NONE;
	UnitGetInventoryLocation( pItem, &nCurInvLocItem );
	if( UnitIsGambler( pMerchant ) )
	{
		if (nCurInvLocItem != INVLOC_MERCHANT_BUYBACK )
		{
			BOOL bRet = s_TreasureGambleRecreateItem( pItem, pMerchant, pPlayer );
			if (!bRet)
			{
				ASSERTX_RETFALSE( bRet, "Failed to create gambling result item." );
			}
		}
	}

	// take the cost of the item from the player	
	UNITLOG_ASSERT_RETFALSE(UnitRemoveCurrencyValidated( pPlayer, buyPrice ), pPlayer );

	// save the old container
	UNIT *pOldContainer = UnitGetContainer(pItem);

	// if we want the merchant to have a limitless supply of this item
	BOOL bAddToClients = FALSE;
	if (UnitGetStat(pItem, STATS_UNLIMITED_IN_MERCHANT_INVENTORY) != 0 &&
		nCurInvLocItem != INVLOC_MERCHANT_BUYBACK)
	{
		// make a new version (not a copy) of item with the same class.
		//  We're not planning on using this for items with random properties, so this should be sufficient.
		int nClass = UnitGetClass(pItem);

		ITEM_SPEC tSpec;
		tSpec.nItemClass = nClass;
		tSpec.pSpawner = pMerchant;
		UNIT *pNewItem = s_ItemSpawn(pGame, tSpec, NULL);
		ASSERTX_RETFALSE( pNewItem, "Unable to create a new item to give to the buyer" );

		// put in player inventory

		if (UnitGetStat(pNewItem, STATS_ITEM_QUANTITY_MAX) > 1 &&
			ItemAutoStack(pPlayer, pNewItem) != pNewItem)
		{
			// The item went into an existing stack
			bAddToClients = FALSE;
			pNewItem = NULL;		// it's most likely been freed
		}
		else
		{
			UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pNewItem, tInvLoc.nInvLocation, tInvLoc.nX, tInvLoc.nY, 0, TIME_ZERO, INV_CONTEXT_BUY);
			ASSERTX_RETFALSE( UnitGetContainer( pNewItem ) == pPlayer, "Unable to put item into buyers inventory" );

			// this item can now be sent to the client
			UnitSetCanSendMessages( pNewItem, TRUE );

			// point to the new item instead of the old
			pItem = pNewItem;
			bAddToClients = TRUE;
		}
		
	}
	else
	{

		// unlink item from any merchant source
		UnitClearStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS, 0 );		
		
		// if buying from shared merchant, remove this item from all other client stores	
		BOOL bFromSharedMerchant = UnitGetUltimateOwner( pItem ) == pMerchant;
		if (bFromSharedMerchant == TRUE)
		{
	
			if (UnitGetStat(pItem, STATS_ITEM_QUANTITY_MAX) > 1 &&
				ItemAutoStack(pPlayer, pItem) != pItem)
			{
				// The item went into an existing stack
				bAddToClients = FALSE;
				pItem = NULL;		// it's most likely been freed
			}
			else
			{
				// take item out of shopkeeper inventory
				UnitInventoryRemove( pItem, ITEM_ALL );
				ASSERTX_RETFALSE( UnitGetContainer( pItem ) == NULL, "Unable to remove item to buy from buyers inventory" );

				// put in player inventory
				UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pItem, tInvLoc.nInvLocation, tInvLoc.nX, tInvLoc.nY, 0, TIME_ZERO, INV_CONTEXT_BUY);
				ASSERTX_RETFALSE( UnitGetContainer( pItem ) == pPlayer, "Unable to put item into buyers inventory" );

				// need to tell clients
				bAddToClients = TRUE;
			}						
		}
		else
		{
		
			if (UnitGetStat(pItem, STATS_ITEM_QUANTITY_MAX) > 1 &&
				ItemAutoStack(pPlayer, pItem) != pItem)
			{
				// The item went into an existing stack
				bAddToClients = FALSE;
				pItem = NULL;		// it's most likely been freed
			}
			else
			{
				// just change inventory location
				s_InventoryPut( pPlayer, pItem, tInvLoc, CLF_ALLOW_EQUIP_WHEN_IN_PLAYER_PUT_RESTRICTED_LOC_BIT, INV_CONTEXT_BUY );
			}		
		}
						
	}

	// send units to clients if needed
	if (bAddToClients)
	{
	
		// tell those that care the the item is now in the buyers inventory
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
		
			// should this client know about the inventory of this unit
			if (ClientCanKnowUnit( pClient, pItem ))
			{
				s_SendUnitToClient( pItem, pClient );
			}
			
		}
	
	}
	
	// TRAVIS : moved after the unit send so that the wardrobe updates and picks up the new unit
	// in the case that this was directly equipped
	// inventory has changed
	s_InventoryChanged( pPlayer );
	if (pOldContainer != pPlayer)
	{
		s_InventoryChanged( pOldContainer );
	}
				
	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StoreMerchantWillBuyItem(	
	UNIT * pSellToUnit,
	UNIT * pItem,
	int *pnLocation/* = NULL*/)
{

	// you cannot sell no drop items
	if ( UnitIsNoDrop( pItem ) )
	{
		return FALSE;
	}
	
	// you cannot sell quest items
	if (QuestUnitIsUnitType_QuestItem( pItem ))
	{
		return FALSE;
	}
		 	
	// you cannot sell items that are in inventory slots that you cannot take them out of
	if (ItemIsInCanTakeLocation( pItem ) == FALSE)
	{
		return FALSE;
	}
			
	cCurrency currency = EconomyItemSellPrice(pItem);
	int nSellPrice = currency.GetValue( KCURRENCY_VALUE_INGAME );
	if (nSellPrice > 0)
	{
		return TRUE;

		// this location checking is only valid if the item will actually go into the merchant unit's inventory, which no one is doing anymore

		//int nLocation = INVLOC_NONE;
		//if (!pnLocation)
		//{
		//	pnLocation = &nLocation;
		//}
		//int x, y;
		//if (ItemGetOpenLocation(pSellToUnit, pItem, TRUE, pnLocation, &x, &y))
		//{
		//	return TRUE;
		//}
		//else
		//{
		//	if (*pnLocation != INVLOC_NONE)
		//	{
		//		UNIT *pOldUnit = UnitInventoryGetByLocationAndXY(pSellToUnit, *pnLocation, x, y);
		//		if (pOldUnit)
		//		{
		//			if (IS_SERVER(UnitGetGame(pSellToUnit)))
		//			{
		//				//Deeestroy eeet
		//				UnitFree(pOldUnit, UFF_SEND_TO_CLIENTS);
		//			}
		//			return TRUE;
		//		}
		//	}
		//}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddItemToShop(
	UNIT *pMerchant,
	UNIT *pItem,
	int nInvLocation)
{	

	// add item to shop inventory
	UnitInventoryAdd(INV_CMD_PICKUP, pMerchant, pItem, nInvLocation );

	// tell clients about the new item
	GAME *pGame = UnitGetGame( pMerchant );
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
	
		// should this client know about the inventory of this unit
		if (ClientCanKnowUnit( pClient, pItem ))
		{
			s_SendUnitToClient( pItem, pClient );
		}
		
	}

}					

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_StoreSellItem(
	UNIT *pPlayer,
	UNIT *pItem,
	UNIT *pMerchant)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( pMerchant, "Expected merchant" );
	
	// server only
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );

	// look for possible cheating
	UNITLOG_ASSERT_RETFALSE( UnitGetUltimateOwner(pItem) == pPlayer, pPlayer );
	UNITLOG_ASSERT_RETFALSE( !ItemBelongsToAnyMerchant(pItem), pPlayer );
	UNITLOG_ASSERT_RETFALSE( StoreMerchantWillBuyItem(pMerchant, pItem), pPlayer );
	const float fDist = VectorDistanceSquared( pPlayer->vPosition, pMerchant->vPosition );
	UNITLOG_ASSERT_RETFALSE( fDist <= ( UNIT_INTERACT_DISTANCE_SQUARED * 4 ), pPlayer );
	// check that it's in a takeable location.
	INVENTORY_LOCATION currInvLoc;
	UnitGetInventoryLocation(pItem, &currInvLoc);
	if (currInvLoc.nInvLocation != INVLOC_NONE &&
		InventoryLocPlayerCanTake(UnitGetContainer(pItem), pItem, currInvLoc.nInvLocation) == FALSE)
	{
		UNITLOG_ASSERTX_RETFALSE(FALSE, 
			"Player attempting to sell item in untakeable location", pPlayer);
	}

	ASSERTX_RETFALSE( fDist <= ( UNIT_INTERACT_DISTANCE_SQUARED ), "Player too far from merchant to sell" );

	// may want to make some provision here for nLocation == INVLOC_NONE 
	//  in which case the merchant will give money for the item, but it won't go in his stock
	cCurrency sellPrice = EconomyItemSellPrice( pItem );
	//int nSellPrice = currency.GetValue( KCURRENCY_VALUE_INGAME );
	ASSERTX_RETFALSE( !sellPrice.IsZero(), "Selling an item with no value" );

	// where is the item currently
	UNIT *pOldContainer = UnitGetContainer( pItem );
	
	// see if they can't hold anymore money, note that we're not going
	// to prevent them from selling the item, but we will tell them
	// every time that can't hold anymore
	if (UnitCanHoldMoreMoney( pPlayer ) == FALSE)
	{
		s_ItemSendCannotPickup( pPlayer, NULL, PR_FULL_MONEY );
	}

	// we are removing an item from a can take location
	ItemRemovingFromCanTakeLocation( pPlayer, pItem );
	
	// for now we'll just always put the sold items in the buyback slot
	//   we may want to make that conditional though
	// TRAVIS - Mythos does buyback too now. Maybe we'll need this conditional
	// for something else?
	BOOL bTakeItem = TRUE;//AppIsHellgate();
	if (bTakeItem)
	{
		// move it to shop buy-back inventory
		int nX, nY;		
		while (!UnitInventoryFindSpace( pPlayer, pItem, INVLOC_MERCHANT_BUYBACK, &nX, &nY ) )
		{
			// remove the oldest item
			UNIT * pOtherItem = UnitInventoryGetOldestItem(pPlayer, INVLOC_MERCHANT_BUYBACK);
			ASSERTX_RETFALSE( pOtherItem, "Unable to find room in buyback inventory and there are no items to remove" );
			UnitInventoryRemove( pOtherItem, ITEM_ALL, CLF_FORCE_SEND_REMOVE_MESSAGE );
			ASSERTX_RETFALSE( UnitGetContainer( pOtherItem ) == NULL, "Unable to remove item to sell out of sellers inventory" );

			UnitFree(pOtherItem, UFF_SEND_TO_CLIENTS);
		}

		UNITLOG_ASSERT_DO(InventoryChangeLocation(pPlayer, pItem, INVLOC_MERCHANT_BUYBACK, nX, nY, 0, INV_CONTEXT_SELL), pPlayer )
		{
			TraceError("Location change on item sell failed for item %d at loc %d for player %d",
				UnitGetId(pItem), currInvLoc.nInvLocation, UnitGetId(pPlayer));
			/* We could free the item and proceed with the sale here, but this would allow players to sell items from undesired locations, e.g. the stash.
			UnitInventoryRemove( pItem, ITEM_ALL );
			UnitFree(pItem, UFF_SEND_TO_CLIENTS);*/

			return FALSE;
		}
	}
	else
	{
		// take item out of sellers inventory
		UnitInventoryRemove( pItem, ITEM_ALL );
		ASSERTX_RETFALSE( UnitGetContainer( pItem ) == NULL, "Unable to remove item to sell out of sellers inventory" );

		UnitFree(pItem, UFF_SEND_TO_CLIENTS);
	}

	// link item to this merchant source
	// the item likely is freed here, but the pointer is valid until the end of the frame
	int nMerchantClass = UnitGetClass( pMerchant );
	UnitSetStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS, nMerchantClass );
	
	// give the seller money					
	UnitAddCurrency( pPlayer, sellPrice );
	
	// inventory has changed
	s_InventoryChanged( pPlayer );
	if (pOldContainer != pPlayer)
	{
		s_InventoryChanged( pOldContainer );
	}
	
	return TRUE;
}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatMerchantLastBrowsedTick(
	struct STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit)
{
	
	// for stats that were saved without the merchant monster class as a param, we will discard it
	if (nStatsVersion <= STATS_VERSION_MERCHANT_LAST_BROWSED_TICK_NO_PARAM)
	{
		return SVR_IGNORE;
	}

	return SVR_OK;
		
}
