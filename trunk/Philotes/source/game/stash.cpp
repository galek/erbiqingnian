//----------------------------------------------------------------------------
// FILE: stash.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_message.h"
#include "c_ui.h"
#include "globalindex.h"
#include "inventory.h"
#include "player.h"
#include "s_message.h"
#include "stash.h"
#include "uix_priv.h"
#include "units.h"

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// COMMON
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StashIsOpen(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	return UnitGetStat( pPlayer, STATS_LAST_STASH_UNIT_ID ) != INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StashIsItemIn(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected unit" );

	// get inv loc	
	INVENTORY_LOCATION tInvLoc;
	UnitGetInventoryLocation( pItem, &tInvLoc );
	
	// check loc
	UNIT *pContainer = UnitGetContainer( pItem );
	ASSERTX_RETFALSE( pContainer, "Expected container" );
	return InvLocIsStashLocation( pContainer, tInvLoc.nInvLocation );
	
}	

//----------------------------------------------------------------------------
// CLIENT
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_StashClose(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	UNIT *pPlayer = UIGetControlUnit();
	
	// tell server we close the UI
	if (StashIsOpen( pPlayer ))
	{
		MSG_CCMD_STASH_CLOSE tMessage;
		c_SendMessage( CCMD_STASH_CLOSE, &tMessage );		
		return TRUE;
	}
#endif	
	return FALSE;
}

//----------------------------------------------------------------------------
// SERVER
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//static int sGetStashInvLoc(
//	UNIT *pPlayer)
//{
//	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
//	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
//	return GlobalIndexGet( GI_INVENTORY_LOCATION_STASH );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Now located in Inventory.cpp

//static void s_sSendStashToClient( 
//	UNIT *pPlayer)
//{
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Now located in Inventory.cpp

//static void s_sRemoveStashFromClient( 
//	UNIT *pPlayer)
//{
//}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StashOpen(
	UNIT *pPlayer,
	UNIT *pStash)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pStash, "Expected stash unit" );
	BOOL bStashWasOpen = StashIsOpen( pPlayer );
	
	// mark the stash as open on the server so the server can verify stash inventory moves.
	UnitSetStat( pPlayer, STATS_LAST_STASH_UNIT_ID, UnitGetId( pStash ));

	// send stash units to client if the stash was not open
	if (bStashWasOpen == FALSE)
	{
		s_sSendStashToClient( pPlayer );
	}
	
	// tell client to open stash UI element
	s_PlayerToggleUIElement( pPlayer, UIE_STASH, UIEA_OPEN );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_StashClose(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// server only	
	GAME *pGame = UnitGetGame( pPlayer );
	if (IS_SERVER( pGame ) && StashIsOpen( pPlayer ) == TRUE)
	{
				
		// clear active stash unit
		UnitSetStat( pPlayer, STATS_LAST_STASH_UNIT_ID, INVALID_ID );

		// remove stash inventory contents from client
		s_sRemoveStashFromClient( pPlayer );

	}
				
}	

