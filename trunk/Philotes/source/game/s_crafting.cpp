//----------------------------------------------------------------------------
// s_crafting.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "s_crafting.h"
#include "player.h"
#include "itemupgrade.h"
#include "items.h"
#include "dbunit.h"
#include "stats.h"
#include "script.h"
#include "unitevent.h"
#include "inventory.h"
#if !ISVERSION(CLIENT_ONLY_VERSION)



static BOOL sCraftingIncrementModsOnItem(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{	
	ASSERT_RETTRUE( pGame );
	ASSERT_RETTRUE( pUnit );	
	int nEventCount = UnitGetStat( pUnit, STATS_ITEM_CRAFTING_EVENT_COUNT );
	nEventCount--;	//dec this
	UnitSetStat( pUnit, STATS_ITEM_CRAFTING_EVENT_COUNT, nEventCount );
	if( nEventCount <= 0 )
	{
		CRAFTING::s_CraftingRemoveMods( UnitGetOwnerUnit( pUnit ) ); //expects the owner
		return FALSE;
	}

	return TRUE;
}



//Event call back for crafted items with mods
static void sCraftingModsUpdate(
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETURN( pEventData );	
	UNIT_EVENT eEvent = (UNIT_EVENT)pEventData->m_Data1;
	int nUnitID = (int)pEventData->m_Data2;
	UNIT *pItem = UnitGetById( pGame, nUnitID );
	ASSERT_RETURN( pItem ); 
	ASSERT_RETURN( CRAFTING::CraftingIsCraftingModItem( pItem ) );	
	
	UNIT *pWeapon = UnitGetOwnerUnit( pItem );
	if( pWeapon == pPlayer )	//simple check to see if it's equiped
		return;	
	if( UnitGetUltimateOwner( pWeapon ) != pPlayer ) //in trade window
		return;
	if( ItemIsEquipped( pWeapon, pPlayer ) == FALSE  )	//just to be sure, if not equipped return
		return;
	

	//we do this because sometimes the events occure on a stat update
	UnitRegisterEventTimed( pItem, sCraftingIncrementModsOnItem, EVENT_DATA(eEvent), 2 );  		
	
}

inline void sCraftingClearAllEventsOnUnit( UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );
	for( int eEvent = 0; eEvent < (int)NUM_UNIT_EVENTS; eEvent++ )
	{
		//clear all event handlers first
		UnitUnregisterEventHandler( UnitGetGame( pUnit ), pUnit, (UNIT_EVENT)eEvent, sCraftingModsUpdate );
	}

}
//registers events for a given item 
inline void sCraftingRegisterEventUpdate( UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );	
	ASSERT_RETURN( CRAFTING::CraftingIsCraftingModItem( pUnit ) );
	UNIT *pPlayer = UnitGetUltimateOwner( pUnit );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	
	int nEventCount = UnitGetStat( pUnit, STATS_ITEM_CRAFTING_EVENT_COUNT );
	int nEventID = UnitGetStat( pUnit, STATS_ITEM_CRAFTING_EVENT_TYPE );
	if( nEventCount > 0 && nEventID >= 0 )
	{
		UNIT_EVENT eEvent = (UNIT_EVENT)nEventID;
		EVENT_DATA eventData( nEventID, UnitGetId( pUnit ) );
		UnitRegisterEventHandler( pGame, pPlayer, eEvent, sCraftingModsUpdate, &eventData );
	}
}

int sCraftingWalkItemsAndRegisterUpdate( UNIT *pUnit )
{
	int nCount( 0 );
	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( pUnit, pItem, 0 );	
	while ( pItem != NULL)
	{		
		if( CRAFTING::CraftingIsCraftingModItem( pItem ) )
		{
			sCraftingRegisterEventUpdate( pItem );
			nCount++;
		}
		else
		{
			nCount += sCraftingWalkItemsAndRegisterUpdate( pItem );
		}
		pItem = UnitInventoryIterate( pUnit, pItem, 0 );
	}
	return nCount;
}


//registers mods for getting updated for events
void CRAFTING::s_CraftingRegisterModsForUpdating( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	//clear first	
	sCraftingClearAllEventsOnUnit( pPlayer );
	sCraftingWalkItemsAndRegisterUpdate( pPlayer );
}

//broadcasts the event data on the player load
static BOOL sCraftingEventPerMinute(
	GAME *pGame,
	UNIT *pPlayer,
	const struct EVENT_DATA &tEventData)
{
	UnitTriggerEvent( pGame, EVENT_ITEM_CRAFTING_TIMER, pPlayer, NULL, NULL);
	UnitRegisterEventTimed( pPlayer, sCraftingEventPerMinute, NULL, GAME_TICKS_PER_SECOND * 60);
	return TRUE;
}

//registers the player for events on mod'ed items
void CRAFTING::s_CraftingPlayerRegisterMods( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	CRAFTING::s_CraftingRegisterModsForUpdating( pPlayer );
	//register player event timer crafting
	UnitRegisterEventTimed( pPlayer, sCraftingEventPerMinute, NULL, GAME_TICKS_PER_SECOND * 60);  		
}


void CRAFTING::s_CraftingPlayerRegisterNewMod( UNIT *pPlayer, UNIT *pItem )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pItem );
	if( CRAFTING::CraftingIsCraftingModItem( pItem ) )
	{
		sCraftingRegisterEventUpdate( pItem );
	}

}

//removes the mod at a given slot
void CRAFTING::s_CraftingRemoveMods( UNIT *pItemHoldingMod )
{
	ASSERT_RETURN( pItemHoldingMod );
	ASSERT_RETURN( UnitIsA( pItemHoldingMod, UNITTYPE_EQUIPABLE ) );
	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( pItemHoldingMod, pItem, 0 );	
	while ( pItem != NULL)
	{		
		UNIT* pNext = UnitInventoryIterate( pItemHoldingMod, pItem, 0 );
		if( CRAFTING::CraftingIsCraftingModItem( pItem ) )
		{
			//remove all events for this item
			sCraftingClearAllEventsOnUnit( pItem );
			//make sure we tell the client the unit is gone
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );			
		}
		pItem = pNext;
	}	
}


#endif