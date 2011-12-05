//----------------------------------------------------------------------------
// FILE: gamemail.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "gag.h"
#include "gamemail.h"
#include "units.h"
#include "trade.h"
#include "player.h"

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameMailCanSendItem(
	UNIT *pPlayer,
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// cannot send items unless in town
	if (UnitIsInTown( pPlayer ) == FALSE)
	{
		return FALSE;
	}

	// hardcore dead cannot send items
	if (PlayerIsHardcoreDead( pPlayer ))
	{
		return FALSE;
	}

	// if there is a specific item in question, check the login on it
	if (pItem)
	{
	
		// check for the "ok to be in email slot" override ... this is bound to break
		// something one day, but we absolutely need a way to override the normal
		// game logic of what is a legal item to send so that we are able to 
		// restore any item via a CSR item restore email
		if (UnitGetStat( pItem, STATS_ALWAYS_ALLOW_IN_EMAIL_SLOTS ))
		{
			return TRUE;
		}
	
		if (TradeCanTradeItem( pPlayer, pItem ) == FALSE)
		{
			return FALSE;
		}	
	}

	return TRUE;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameMailCanReceiveItem(
	UNIT *pPlayer,
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	return GameMailCanSendItem( pPlayer, pItem );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MAIL_CAN_COMPOSE_RESULT GameMailCanComposeMessage(
	UNIT *pPlayer)
{
	ASSERTX_RETVAL( pPlayer, MCCR_FAILED_UNKNOWN, "Expected player" );
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), MCCR_FAILED_UNKNOWN, "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
		
	// gagged players cannot compose messages
	if (IS_SERVER( pGame ))
	{
		if (s_GagIsGagged( pPlayer ))
		{
			return MCCR_FAILED_GAGGED;
		}
	}
	else
	{	
		if (c_GagIsGagged( pPlayer ))
		{
			return MCCR_FAILED_GAGGED;
		}
	}
	
	return MCCR_OK;
	
}
