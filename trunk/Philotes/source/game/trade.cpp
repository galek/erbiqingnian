//----------------------------------------------------------------------------
// FILE: trade.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "game.h"
#include "c_trade.h"
#include "items.h"
#include "player.h"
#include "trade.h"
#include "s_trade.h"
#include "stats.h"
#include "units.h"

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TradeInit(
	GAME* pGame)
{
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT( pGame ))
	{
		c_TradeInit( pGame );
	}
	else
#endif// !ISVERSION(SERVER_VERSION)
	{
		s_TradeInit( pGame );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TradeFree(
	GAME* pGame)
{
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT( pGame ))
	{	
		c_TradeFree( pGame );
	}
	else
#endif// !ISVERSION(SERVER_VERSION)
	{
		s_TradeFree( pGame );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TradeCanTradeWith(
	UNIT *pUnit,
	UNIT *pUnitToTradeWith,
	int *nOverrideTooltipId )
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pUnitToTradeWith, "Expected unit to trade with" );
	
	// dead players cannot trade
	if (UnitIsA( pUnit, UNITTYPE_PLAYER ) && 
		(IsUnitDeadOrDying( pUnit ) || 
		 UnitIsGhost( pUnit )))
	{
		return FALSE;
	}

	if (UnitIsA( pUnitToTradeWith, UNITTYPE_PLAYER ) && 
		(IsUnitDeadOrDying( pUnitToTradeWith ) || 
		 UnitIsGhost( pUnitToTradeWith )))
	{
		return FALSE;
	}

	// trial players cannot trade with other players
	if( (UnitIsA( pUnit, UNITTYPE_PLAYER ) && UnitIsA( pUnitToTradeWith, UNITTYPE_PLAYER ) )
		&& (UnitHasBadge(pUnit, ACCT_TITLE_TRIAL_USER) 
#if ISVERSION(SERVER_VERSION)
		// We can't check other people's badges on the client side.
		|| UnitHasBadge(pUnitToTradeWith, ACCT_TITLE_TRIAL_USER)
#endif
		))
	{
		// KCK: Change the tooltip to explain why this option is enabled (note that this
		// probably won't ever change back until a restart - We may need to fix that)
		if (nOverrideTooltipId)
		{
			*nOverrideTooltipId = GS_TRIAL_PLAYERS_CANNOT_ACCESS;
		}
		return FALSE;
	}

	// ignore distance that are too big (should be checked on the client, but could be hacked)
	if (UnitsGetDistanceSquared( pUnit, pUnitToTradeWith ) >= TRADE_DISTANCE_SQ)
	{
		return FALSE;
	}

	// the person asking can't be trading with anybody (the other unit could be an NPC, so that's OK)
	if (TradeIsTrading( pUnit ))
	{
		return FALSE;
	}

	// the asker cannot trade if they are already asking somebody else
	UNITID idAskingToTrade = UnitGetStat( pUnit, STATS_ASKING_TO_TRADE );
	if (idAskingToTrade != INVALID_ID && UnitGetId( pUnitToTradeWith ) != idAskingToTrade)
	{
		return FALSE;
	}
	
	// when trading with other players
	if (UnitIsPlayer( pUnitToTradeWith ))
	{
	
		// you can't trade if either is already trading
		if (TradeIsTrading( pUnitToTradeWith ) == TRUE)
		{
			return FALSE;
		}

		// if both are players, if either player is a hardcore player, both must be hardcore players
		if (PlayerIsHardcore( pUnit ) != PlayerIsHardcore( pUnitToTradeWith ))
		{
			return FALSE;
		}
	
		// same goes for some game variants
		if (PlayerIsElite( pUnit ) != PlayerIsElite( pUnitToTradeWith ))
		{
			return FALSE;
		}

		// same goes for some game variants
		if (PlayerIsLeague( pUnit ) != PlayerIsLeague( pUnitToTradeWith ))
		{
			return FALSE;
		}
		
	}
		
	// ok to goooooooo!!!	
	return TRUE;		
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TradeIsTrading(
	UNIT* pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	
	UNITID idTradingWith = TradeGetTradingWithID( pPlayer );
	return idTradingWith != INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TradeIsTradingWith(
	UNIT* pPlayer,
	UNIT* pTradingWith)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( pTradingWith, "Expected trading with " );
	
	UNITID idTradingWith = TradeGetTradingWithID( pPlayer );
	return UnitGetId( pTradingWith ) == idTradingWith;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID TradeGetTradingWithID(
	UNIT *pUnit)
{
	return UnitGetStat( pUnit, STATS_TRADING );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *TradeGetTradingWith(
	UNIT *pUnit)
{
	GAME *pGame = UnitGetGame( pUnit );
	UNITID idTradingWith = TradeGetTradingWithID( pUnit );
	return UnitGetById( pGame, idTradingWith );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TradeCanTradeItem(
	UNIT *pPlayer,
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	
	// cannot trade no trade items
	if (ItemIsNoTrade( pItem ))
	{
		return FALSE;
	}

	if(AppIsTugboat()  )
	{
		if( UnitIsA( pItem, UNITTYPE_ANCHORSTONE ) ||
			UnitIsA( pItem, UNITTYPE_QUEST_ITEM ) ||
			UnitIsA( pItem, UNITTYPE_STARTING_ITEM ) )
		{
			return FALSE;
		}
	}
	
	// must be able to drop item
	if (InventoryCanDropItem( pPlayer, pItem ) != DR_OK)
	{
		return FALSE;
	}

	return TRUE;			
	
}
