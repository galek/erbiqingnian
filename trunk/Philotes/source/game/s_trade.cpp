//----------------------------------------------------------------------------
// FILE: s_trade.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_input.h"
#include "c_message.h"
#include "c_trade.h"
#include "console.h"
#include "inventory.h"
#include "items.h"
#include "npc.h"
#include "s_message.h" // also includes prime.h
#include "s_reward.h"
#include "s_store.h"
#include "s_trade.h"
#include "trade.h"
#include "treasure.h"
#include "uix.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "Currency.h"
#if ISVERSION(SERVER_VERSION)
#include "s_trade.cpp.tmh"
#endif
//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STRADE_SESSION *sTradeSessionGet(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	ASSERTX_RETNULL( UnitHasClient( pUnit ), "Can only get trade sessions for units that have clients" );
	GAMECLIENT *pClient = UnitGetClient( pUnit );
	return pClient->m_pTradeSession;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TradeSessionInit(
	STRADE_SESSION *pSession)
{
	ASSERTX_RETURN( pSession, "Expected session" );
	cCurrency currency( 0, 0 );
	pSession->nCurrency = currency;		// we want to reset the cash amounts when we start
	pSession->nTradeOfferVersion = 0;	// we track how many times the trade offer changes.
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TradeInit(
	GAME* pGame)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TradeFree(
	GAME* pGame)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTradeStyleCanOffer(
	UNIT *pTrader,
	UNIT *pTradingWith)
{

	// can the trader offer items to who they are trading with
	if (UnitHasClient( pTrader ))
	{

		// check for player/player trade	
		if (UnitHasClient( pTradingWith ))
		{
			return TRUE; 
		}
		
		// check for trading with trader (which is different than trading with a merchant)
		if (UnitIsTrader( pTradingWith ))
		{
			return TRUE;
		}
		
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSnapshotInventory(
	UNIT *pContainer)
{
	ASSERTX_RETURN( pContainer, "Expected unit" );

	// one day we might support this ... but I won't do it until we decide that yeah, trade is cool
		
	//// iterate the entire inventory and mark where every object is right now
	//UNIT *pItem = NULL;
	//while (pContainer = UnitInventoryIterate( pContainer, pItem ))
	//{
	//
	//	// record this location
	//	INVENTORY_LOCATION tLocation;
	//	UnitGetInventoryLocation( pItem, &tLocation );

	//	// save in stat (maybe store this somewhere else, but, eh?)
	//	UnitSetStat( pItem, STATS_INVENTORY_SNAPSHOT_LOCATION, tLocation.nInvLocation );
	//	UnitSetStat( pItem, STATS_INVENTORY_SNAPSHOT_X, tLocation.nX );
	//	UnitSetStat( pItem, STATS_INVENTORY_SNAPSHOT_Y, tLocation.nY );
	//	UnitSetStat( pItem, STATS_INVENTORY_SNAPSHOT_CONTAINER_UNITID, UnitGetId( pContainer ) );
	//	
	//	// snapshot anything it contains
	//	sSnapshotInventory( pItem );
	//	
	//}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInitUnitTradeSession(
	UNIT *pUnit)
{
	if (pUnit)
	{
		// clear any trade session variables
		if (UnitHasClient( pUnit ))
		{
			STRADE_SESSION *pTradeSession = sTradeSessionGet( pUnit );
			s_TradeSessionInit( pTradeSession );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TradeStart(
	UNIT* pTrader,
	UNIT* pTradingWith,
	const TRADE_SPEC &tTradeSpec)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pTrader, "Expected trader" );
	ASSERTX_RETURN( UnitIsA( pTrader, UNITTYPE_PLAYER ), "One trader must be a player" );	
	ASSERTX_RETURN( pTradingWith, "Expected trading with" );
	GAME *pGame = UnitGetGame( pTrader );

	// if no longer able to trade, forget it
	if (TradeCanTradeWith( pTrader, pTradingWith ) == FALSE)
	{
		return;
	}

	// clear out any requests to trade that might be out there
	s_TradeRequestNewCancel( pTrader );
	s_TradeRequestNewCancel( pTradingWith );
	
	// init trade sessions for units
	sInitUnitTradeSession( pTrader );
	sInitUnitTradeSession( pTradingWith );
	
	if (UnitIsMerchant( pTradingWith ) &&
		!UnitIsTrainer( pTradingWith ) )
	{
		s_StoreEnter( pTrader, pTradingWith, tTradeSpec);
	}
	else
	{
		
		// allow simultaneous trades between certain NPCs and multiple players

		// do nothing if either is already trading
		if (TradeIsTrading( pTrader ) || (!tTradeSpec.bNPCAllowsSimultaneousTrades && TradeIsTrading( pTradingWith )))
		{
			return;
		}

		// set flag that players are trading
		UnitSetStat( pTrader, STATS_TRADING, UnitGetId( pTradingWith ) );
		if( !UnitIsTrainer( pTradingWith ) && !tTradeSpec.bNPCAllowsSimultaneousTrades )
		{
			UnitSetStat( pTradingWith, STATS_TRADING, UnitGetId( pTrader ) );
		}

		// tell trader to open trade interface
		ASSERTX_RETURN( UnitHasClient( pTrader ), "Trader must have a client" );
		MSG_SCMD_TRADE_START tTradeStartMessage;
		tTradeStartMessage.idTradingWith = UnitGetId( pTradingWith );
		tTradeStartMessage.tTradeSpec = tTradeSpec;
		GAMECLIENTID idClient = UnitGetClientId( pTrader );		
		s_SendMessage( pGame, idClient, SCMD_TRADE_START, &tTradeStartMessage );

		// send trader inventory contents to trader
		if (pTrader != pTradingWith &&
			!UnitIsTrainer( pTradingWith ))
		{
			// TODO: we should not do this and instead send only items in the trade 
			// windows as the are put into the trade window -Colin
			s_InventorySendToClient( pTradingWith, UnitGetClient( pTrader ), 0 );
		}

		// snapshot inventory for types of trade that we can give offers
		if (sTradeStyleCanOffer( pTrader, pTradingWith ))
		{
			sSnapshotInventory( pTrader );
		}
		
		// when trading with another player
		if (UnitHasClient( pTradingWith ) && pTradingWith != pTrader)
		{


			// tell the other player to open trade interface
			MSG_SCMD_TRADE_START tTradeStartMessage;
			tTradeStartMessage.idTradingWith = UnitGetId( pTrader );
			tTradeStartMessage.tTradeSpec = tTradeSpec;
			GAMECLIENTID idClient = UnitGetClientId( pTradingWith );		
			s_SendMessage( pGame, idClient, SCMD_TRADE_START, &tTradeStartMessage );
			
			// send inventory of trader to them
			// TODO: we should not do this and instead send only items in the trade 
			// windows as the are put into the trade window -Colin
			s_InventorySendToClient( pTrader, UnitGetClient( pTradingWith ), 0 );					
			
			// shapshot inventory
			if (sTradeStyleCanOffer( pTradingWith, pTrader))			
			{
				sSnapshotInventory( pTradingWith );
			}

		}
		
	}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sExitTrade(
	UNIT *pUnit,
	BOOL bSendCancelToClient)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// must be trading	
	if (TradeIsTrading( pUnit ))
	{
			
		// send cancel message to client (if any)
		if (UnitHasClient( pUnit ) == TRUE && bSendCancelToClient == TRUE)
		{
			MSG_SCMD_TRADE_FORCE_CANCEL tMessage;				
			GAMECLIENTID idClient = UnitGetClientId( pUnit );		
			s_SendUnitMessageToClient( pUnit, idClient, SCMD_TRADE_FORCE_CANCEL, &tMessage );		
		}
				
		// remove our inventory from who we're trading with if they are still around
		UNIT *pTradingWith = TradeGetTradingWith( pUnit );
		if(!pTradingWith)
		{
			return;
		}

		if (pUnit != pTradingWith && 
			UnitHasClient( pTradingWith ) &&
			UnitHasClient( pUnit ))
		{
			DWORD dwFlags = 0;
			if( AppIsTugboat() )
			{
				SETBIT( dwFlags, ISF_CLIENT_CANT_KNOW_BIT );
			}	
			GAMECLIENT *pClient = UnitGetClient( pTradingWith );
			s_InventoryRemoveFromClient( pUnit, pClient, dwFlags );
		}

		// clear trading stat
		UnitClearStat( pUnit, STATS_TRADING, 0 );

		// clear any session variables
		if (UnitHasClient( pUnit ))
		{
		
			// clear reward variables ... cause when you are offered rewards, you are actually
			// just "trading" with yourself
			SREWARD_SESSION *pRewardSession = s_RewardSessionGet( pUnit );
			s_RewardSessionInit( pRewardSession );
			
			// clear any player/player trade session info
			STRADE_SESSION *pTradeSession = sTradeSessionGet( pUnit );
			s_TradeSessionInit( pTradeSession );
			
		}
		
		// close rewards when we were "trading" with ourselves, it's important to do this
		// after the trade session has been cleared so that when we move the reward items
		// around we don't call the take item callback
		if (pTradingWith == pUnit)
		{
			s_RewardClose( pUnit );
		}
		
		// snapback trade location
		int nTradeLocation = InventoryGetTradeLocation( pUnit );
		s_ItemsRestoreToStandardLocations( pUnit, nTradeLocation );
		
	}
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TradeCancel(
	UNIT* pPlayer,
	BOOL bForce)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pPlayer, "Expected player" );

	// cancel any request to trade we might have had out there (this can happen if two
	// player request each other at exactly the same time and we want to gracefully handle it)
	s_TradeRequestNewCancel( pPlayer );
	
	// who is this player trading with
	UNITID idTradingWith = UnitGetStat( pPlayer, STATS_TRADING );
	if( bForce || idTradingWith != INVALID_ID)
	{
		GAME* pGame = UnitGetGame( pPlayer );
		
		UNIT* pTradingWith = UnitGetById( pGame, idTradingWith );
		if (pTradingWith && pTradingWith != pPlayer)
		{

			if (UnitIsMerchant( pTradingWith ) &&
				!UnitIsTrainer( pTradingWith ) )
			{
				// exit merchant store
				s_StoreExit( pPlayer, pTradingWith );
			}
			else
			{
				// force cancel for the player we were trading with
				sExitTrade( pTradingWith, TRUE );
			}
										
		}
						
	}

	// have this player exit trade, when forcing trade to exit we need to tell their client
	// to exit trade too
	sExitTrade( pPlayer, TRUE );

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanAcceptTrade(
	UNIT *pTrader)
{
	ASSERTX_RETFALSE( pTrader, "Expected unit" );
	UNIT *pTradingWith = TradeGetTradingWith( pTrader );
	if (pTradingWith == NULL)
	{
		return FALSE;
	}

	// if who we're trading with is offering money, they must actually have it
	STRADE_SESSION *pSession = sTradeSessionGet( pTradingWith );
	ASSERT_RETFALSE(pSession);
	if (pSession->nCurrency.IsZero() == FALSE )		
	{
		if (  UnitGetCurrency( pTradingWith ) < pSession->nCurrency )
		{
			return FALSE;
		}
	}
	
	const int MAX_TRADE_ITEMS = 64;
	UNIT *pItems[ MAX_TRADE_ITEMS ];
	int nNumItems = 0;
	
	// iterate the trade offer 
	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryIterate(pTradingWith, pItem)) != NULL)
	{
		if (InventoryIsInTradeLocation( pItem ))
		{
			ASSERTX_BREAK( nNumItems < MAX_TRADE_ITEMS - 1, "Too many items in trade offer" );
			
			// add to array
			pItems[ nNumItems++ ] = pItem;
			
		}
		
	}

	if (nNumItems > 0)
	{
		// see if we can pickup all the items	
		return UnitInventoryCanPickupAll( pTrader, pItems, nNumItems );	
	}
	else
	{
		return TRUE;  // no items, no problem
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMoveTradeItem(
	UNIT *pSource,
	UNIT *pItem,
	UNIT *pDest)
{
	ASSERTX_RETFALSE( pSource, "Expected source" );
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( pDest, "Expected dest" );

	// item must be owned by the source
	ASSERTV_RETFALSE( 
		UnitGetUltimateOwner( pItem ) == pSource,
		"Cannot move trade item.  Item '%s' is owned by '%s', but expected it to be owned by '%s'",
		UnitGetDevName( pItem ),
		UnitGetDevName( UnitGetUltimateOwner( pItem ) ),
		UnitGetDevName( pSource ));
		
	BOOL bSuccess = s_ItemRestoreToStandardInventory( pItem, pDest, INV_CONTEXT_TRADE );
	ASSERTX( bSuccess, "Unable to put trade item in destination unit inventory" );
	
	// we can't let it fail, and if we're doing the item moves here anyway, it should indeed
	// not be failing, but just super in case we will handle this error condition	
	if (bSuccess == FALSE)
	{

		// remove from old container if we're still in one at all
		if (UnitGetContainer( pItem ))
		{
			// remove from unit
			UNIT *pRemoved = UnitInventoryRemove( pItem, ITEM_ALL, 0, TRUE, INV_CONTEXT_TRADE );
			ASSERTX( pRemoved == pItem, "Could not remove item from source inventory" );			
			
		}
		
		// must not be in a container
		ASSERTX_RETFALSE( UnitGetContainer( pItem ) == NULL, "In the error case of moving a trade item, still unable to remove item from inventory of unit" );

		// set the item to only be for the dest unit
		UnitSetRestrictedToGUID( pItem, UnitGetGuid( pDest ) );
		
		// put the item at the feet of the dest unit
		UnitWarp(
			pItem,
			UnitGetRoom( pDest ),
			UnitGetPosition( pDest ),
			UnitGetFaceDirection( pDest, FALSE ),
			UnitGetUpDirection( pDest ),
			0);
									
	}

	return bSuccess;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTradeErrors( 
	UNIT *pUnit,
	TRADE_ERROR eError,
	UNIT *pTradingWith)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pTradingWith, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// setup message
	MSG_SCMD_TRADE_ERROR tMessage;
	tMessage.idTradingWith = UnitGetId( pTradingWith );
	tMessage.nTradeError = eError;
	
	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendMessage( pGame, idClient, SCMD_TRADE_ERROR, &tMessage );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoTrade(
	UNIT *pUnitA,
	UNIT *pUnitB)
{

	// sanity
	if (sCanAcceptTrade( pUnitA ) == FALSE ||
		sCanAcceptTrade( pUnitB ) == FALSE)
	{
		s_TradeCancel( pUnitA, TRUE );
		return;
	}

	// give money from A to B
	STRADE_SESSION *pSessionA = sTradeSessionGet( pUnitA );
	ASSERT_RETURN(pSessionA);
	STRADE_SESSION *pSessionB = sTradeSessionGet( pUnitB );
	ASSERT_RETURN(pSessionB);

	//Note: this is also checked in sCanAcceptTrade.
	cCurrency currencyA = UnitGetCurrency( pUnitA );
	cCurrency currencyB = UnitGetCurrency( pUnitB );
	UNITLOG_ASSERTX_RETURN( currencyA >= pSessionA->nCurrency, "Unit is trading money it does not possess.", pUnitA);
	UNITLOG_ASSERTX_RETURN( pSessionA->nCurrency.IsNegative() == FALSE, "Unit is trading negative money.", pUnitA);
	UNITLOG_ASSERTX_RETURN( currencyB >= pSessionB->nCurrency, "Unit is trading money it does not possess.", pUnitB);
	UNITLOG_ASSERTX_RETURN( pSessionB->nCurrency.IsNegative() == FALSE, "Unit is trading negative money.", pUnitB);

	// give money from A to B	
	if ( pSessionA->nCurrency.IsZero() == FALSE )
	{
		UnitRemoveCurrencyUnsafe( pUnitA, pSessionA->nCurrency );
		UnitAddCurrency( pUnitB, pSessionA->nCurrency );
	}

	// give money from B to A
	if ( pSessionB->nCurrency.IsZero() == FALSE )
	{
		UnitRemoveCurrencyUnsafe( pUnitB, pSessionB->nCurrency );
		UnitAddCurrency( pUnitA, pSessionB->nCurrency );
	}
	
	// prepare to iterate inventory
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_TRADE_SLOTS_ONLY_BIT );

	// give unit A items to unit B	
	UNIT *pItemNext = NULL;	
	TRADE_ERROR eTradeErrorForUnitB = TE_NONE;
	UNIT *pItem = UnitInventoryIterate( pUnitA, NULL, dwFlags );
	while (pItem)
	{
		
		// get next item
		pItemNext = UnitInventoryIterate( pUnitA, pItem, dwFlags );
		
		// move to dest
		if (sMoveTradeItem( pUnitA, pItem, pUnitB ) == FALSE)
		{
			eTradeErrorForUnitB = TE_UNABLE_TO_RECEIVE_ITEMS;
		}
		
		// goto next item
		pItem = pItemNext;
		
	}
	
	// give unit B items to unit A	
	pItemNext = NULL;
	TRADE_ERROR eTradeErrorForUnitA = TE_NONE;
	pItem = UnitInventoryIterate( pUnitB, NULL, dwFlags );
	while (pItem)
	{
		
		// get next item
		pItemNext = UnitInventoryIterate( pUnitB, pItem, dwFlags );
		
		// move to dest
		if (sMoveTradeItem( pUnitB, pItem, pUnitA ) == FALSE)
		{
			eTradeErrorForUnitA = TE_UNABLE_TO_RECEIVE_ITEMS;
		}
		
		// goto next item
		pItem = pItemNext;
		
	}
	
	// trade is done, stop trading
	s_TradeCancel( pUnitA, TRUE );

	// if there were errors, inform each client
	if (eTradeErrorForUnitA != TE_NONE)
	{
		sSendTradeErrors( pUnitA, eTradeErrorForUnitA, pUnitB );
	}
	if (eTradeErrorForUnitB != TE_NONE)
	{
		sSendTradeErrors( pUnitB, eTradeErrorForUnitB, pUnitA );
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTradeStatus(
	UNIT *pUnitWithStatus,
	UNIT *pUnitWithClientDestination)
{			
	GAME *pGame = UnitGetGame( pUnitWithClientDestination );
	
	// setup trade message
	MSG_SCMD_TRADE_STATUS tMessage;
	tMessage.idTrader = UnitGetId( pUnitWithStatus );
	tMessage.nStatus = UnitGetStat( pUnitWithStatus, STATS_TRADE_STATUS );
	if (UnitHasClient ( pUnitWithStatus ))
	{
		STRADE_SESSION *pTradeSession = sTradeSessionGet( pUnitWithStatus );
		if(pTradeSession) tMessage.nTradeOfferVersion = pTradeSession->nTradeOfferVersion;
	}
	
	// send to client, they should always have a client id, but for testing purposes it's
	// been nice to simulate player trading with an NPC, so we'll allow it
	if (UnitHasClient( pUnitWithClientDestination ))
	{
		GAMECLIENTID idClient = UnitGetClientId( pUnitWithClientDestination );
		s_SendMessage( pGame, idClient, SCMD_TRADE_STATUS, &tMessage );
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendCannotAccept( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	if (UnitHasClient( pUnit ))
	{
		
		// setup message
		MSG_SCMD_TRADE_CANNOT_ACCEPT tMessage;
		
		// send to client
		GAME *pGame = UnitGetGame( pUnit );
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		s_SendMessage( pGame, idClient, SCMD_TRADE_CANNOT_ACCEPT, &tMessage );
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeSetStatus(
	UNIT *pTrader,
	TRADE_STATUS eStatus,
	int nTradeOfferVersion)
{
	ASSERTX_RETURN( pTrader, "Expected unit" );
	
	// must be trading
	if (TradeIsTrading( pTrader ))
	{
		// who are we trading with
		UNIT *pTradingWith = TradeGetTradingWith( pTrader );
		
		// to go to an accept state, we have to be able to take all the stuff into our inventory
		if (sCanAcceptTrade( pTrader ) == FALSE)
		{
			sSendCannotAccept( pTrader );
			return;
		}
		// Check whether the status change is for the correct trade offer version.
		STRADE_SESSION *pTradeSession = sTradeSessionGet( pTrader );
		if(pTradeSession && pTradeSession->nTradeOfferVersion != nTradeOfferVersion)
		{
			TraceGameInfo("Rejecting trade status change %d because client offer version of %d does not match server offer version of %d.",
				(int)eStatus, nTradeOfferVersion, pTradeSession->nTradeOfferVersion);
			return;
		}

		// set the new accept state
		UnitSetStat( pTrader, STATS_TRADE_STATUS, eStatus );
		
		// if both players have accepted, do the trade
		TRADE_STATUS eStatusA = (TRADE_STATUS)UnitGetStat( pTrader, STATS_TRADE_STATUS );		
		TRADE_STATUS eStatusB = (TRADE_STATUS)UnitGetStat( pTradingWith, STATS_TRADE_STATUS );
		if (eStatusA == TRADE_STATUS_ACCEPT && eStatusB == TRADE_STATUS_ACCEPT)
		{
			sDoTrade( pTrader, pTradingWith );
		}
		else
		{
		
			// send a message to "trading with" with the new status of the trade
			sSendTradeStatus( pTrader, pTradingWith );
		
		}
				
	}
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeOfferModified( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// if trading
	if (TradeIsTrading( pUnit ))
	{
		// who are we trading with
		UNIT *pTradingWith = TradeGetTradingWith( pUnit );
		STRADE_SESSION *pTradeSessionMine = sTradeSessionGet( pUnit );
		STRADE_SESSION *pTradeSessionTheirs = sTradeSessionGet( pTradingWith );
		ASSERT_RETURN(pTradeSessionTheirs);
		ASSERT_RETURN(pTradeSessionMine);
		pTradeSessionMine->nTradeOfferVersion++;
		pTradeSessionTheirs->nTradeOfferVersion++;

		// always clear both statuses, even if they're already clear.  this allows
		// us to put in clientside UI delays to prevent last second changes.
		{
		
			// clear both status
			UnitSetStat( pUnit, STATS_TRADE_STATUS, TRADE_STATUS_NONE );
			UnitSetStat( pTradingWith, STATS_TRADE_STATUS, TRADE_STATUS_NONE );

			// tell both clients about the new status of each person
			sSendTradeStatus( pUnit, pUnit );
			sSendTradeStatus( pUnit, pTradingWith );
			sSendTradeStatus( pTradingWith, pTradingWith );			
			sSendTradeStatus( pTradingWith, pUnit );
			
		}
		
	}
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeRequestNew( 
	UNIT *pUnit, 
	UNIT *pUnitToTradeWith)
{
	ASSERTX_RETURN( pUnit, "Expected unit asking to trade" );
	ASSERTX_RETURN( pUnitToTradeWith, "Expected unit to ask to trade with" );
	GAME *pGame = UnitGetGame( pUnit );

	// cancel any previous request we have out there (if any)
	s_TradeRequestNewCancel( pUnit );
		
	// must be able to trade with
	int	nOverride = -1;
	if (TradeCanTradeWith( pUnit, pUnitToTradeWith, &nOverride ) == FALSE)
	{	
		ASSERTX_RETURN( UnitHasClient( pUnit ), "Unit requesting to trade has no client" );
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
	
		// the unit requested is busy doing something else	
		MSG_SCMD_TRADE_ERROR tMessage;
		if (nOverride != -1)		// KCK Hacky. If TradeCanTradeWith returned an override tooltip, it's because the target player is a trial player
			tMessage.nTradeError = TE_PLAYER_IS_TRIAL_ONLY;
		else
			tMessage.nTradeError = TE_PLAYER_BUSY;
		s_SendMessage( pGame, idClient, SCMD_TRADE_ERROR, &tMessage );
		//Here, we may want to check if the other player is trial and give a different message.
	}				
	else
	{
	
		// set a stat of who we're asking to trade
		UnitSetStat( pUnit, STATS_ASKING_TO_TRADE, UnitGetId( pUnitToTradeWith ) );
		
		// send message to client that somebody wants to trade
		MSG_SCMD_TRADE_REQUEST_NEW tMessage;
		tMessage.idToTradeWith = UnitGetId( pUnit );
		GAMECLIENTID idClient = UnitGetClientId( pUnitToTradeWith );
		s_SendMessage( pGame, idClient, SCMD_TRADE_REQUEST_NEW, &tMessage );

	}
		
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeRequestNewCancel( 
	UNIT *pUnit)
{

	// get who we are requesting to trade with
	UNITID idToTradeWith = UnitGetStat( pUnit, STATS_ASKING_TO_TRADE );
	if (idToTradeWith != INVALID_ID)
	{
		GAME *pGame = UnitGetGame( pUnit );
		UNIT *pUnitToTradeWith = UnitGetById( pGame, idToTradeWith );
		
		// tell the unit we wanted to trade with that we cancelled
		MSG_SCMD_TRADE_REQUEST_NEW_CANCEL tMessage;
		tMessage.idInitialRequester = UnitGetId( pUnit );
		GAMECLIENTID idClient = UnitGetClientId( pUnitToTradeWith );
		s_SendMessage( pGame, idClient, SCMD_TRADE_REQUEST_NEW_CANCEL, &tMessage );
		
		// clear that we're asking anybody to trade
		UnitClearStat( pUnit, STATS_ASKING_TO_TRADE, 0 );
		
	}
	
}	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeRequestAccept(
	UNIT *pUnit,
	UNIT *pAskedToTradeBy)
{

	// make sure who were were asked to trade by is still asking
	UNITID idAskedToTrade = UnitGetStat( pAskedToTradeBy, STATS_ASKING_TO_TRADE );
	if (idAskedToTrade == UnitGetId( pUnit ))
	{
	
		// clear that we were being asked
		UnitClearStat( pAskedToTradeBy, STATS_ASKING_TO_TRADE, 0 );
		
		// setup trade spec
		TRADE_SPEC tTradeSpec;
		tTradeSpec.eStyle = STYLE_TRADE;
		
		// ok, start a trade session
		s_TradeStart( pAskedToTradeBy, pUnit, tTradeSpec );
		
	}
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeRequestReject(
	UNIT *pUnit,
	UNIT *pAskedToTradeBy)	
{

	// make sure who were were asked to trade by is still asking
	UNITID idAskedToTrade = UnitGetStat( pAskedToTradeBy, STATS_ASKING_TO_TRADE );
	if (idAskedToTrade == UnitGetId( pUnit ))
	{
		GAME *pGame = UnitGetGame( pUnit );
			
		// clear that we were being asked
		UnitClearStat( pAskedToTradeBy, STATS_ASKING_TO_TRADE, 0 );

		// tell them that we're not interested
		MSG_SCMD_TRADE_ERROR tMessage;
		GAMECLIENTID idClient = UnitGetClientId( pAskedToTradeBy );
		tMessage.nTradeError = TE_PLAYER_REJECTED_OFFER;
		s_SendMessage( pGame, idClient, SCMD_TRADE_ERROR, &tMessage );
		
	}

}	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_TradeIsAnyPlayerTradingWith(
	UNIT *pUnit)
{
	GAME *pGame = UnitGetGame( pUnit );
	
	// this could maybe be slow for like a kagillion clients or something
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		if (TradeIsTradingWith( pPlayer, pUnit ))
		{
			return TRUE;
		}
		
	}	
	return FALSE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTradeMoney( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UNIT *pTradingWith = TradeGetTradingWith( pUnit );
	if (pTradingWith)
	{
		ASSERTX_RETURN( UnitIsPlayer( pUnit ) && UnitIsPlayer( pTradingWith ), "Not a player-player trade" );
		
		// get each trade session
		STRADE_SESSION *pTradeSessionMine = sTradeSessionGet( pUnit );
		STRADE_SESSION *pTradeSessionTheirs = sTradeSessionGet( pTradingWith );
		ASSERT_RETURN(pTradeSessionTheirs);
		ASSERT_RETURN(pTradeSessionMine);
				
		// fill out message
		MSG_SCMD_TRADE_MONEY tMessage;		
		tMessage.nMoneyYours = pTradeSessionMine->nCurrency.GetValue( KCURRENCY_VALUE_INGAME );// nMoneyOffer;
		tMessage.nMoneyTheirs = pTradeSessionTheirs->nCurrency.GetValue( KCURRENCY_VALUE_INGAME );
		tMessage.nMoneyRealWorldYours = pTradeSessionMine->nCurrency.GetValue( KCURRENCY_VALUE_REALWORLD );
		tMessage.nMoneyRealWorldTheirs = pTradeSessionTheirs->nCurrency.GetValue( KCURRENCY_VALUE_REALWORLD );
		

		// send to this client
		GAME *pGame = UnitGetGame( pUnit );
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		s_SendMessage( pGame, idClient, SCMD_TRADE_MONEY, &tMessage );

	}
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_TradeModifyMoney( 
	UNIT *pUnit, 
	cCurrency &currencyToTrade )
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	UNITLOG_ASSERTX_RETURN( currencyToTrade.IsNegative() == FALSE, "Attempting to trade negative amounts of money.", pUnit);
	
	// must be trading
	UNIT *pTradingWith = TradeGetTradingWith( pUnit );
	if (pTradingWith)
	{
		STRADE_SESSION *pSession = sTradeSessionGet( pUnit );
		cCurrency unitCurrency = UnitGetCurrency( pUnit );		
		if ( unitCurrency >= currencyToTrade )
		{

			// the trade offer has been modified
			s_TradeOfferModified( pUnit );
		
			// set the new money for this trade session
			if (pSession)
			{
				pSession->nCurrency = currencyToTrade;
			}

			// send the money value to both traders
			sSendTradeMoney( pUnit );
			sSendTradeMoney( pTradingWith );
			
		}
		
	}
	
}	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )	