//----------------------------------------------------------------------------
// crafting_properties.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "crafting_properties.h"
#include "unit_priv.h"
#include "itemupgrade.h"
#include "excel_private.h"
#include "inventory.h"
#include "items.h"

int CRAFTING::g_sCRAFTING_SLOTS_TYPE_COUNT( 0 );


//excel post process
BOOL CRAFTING::CraftingExcelPostProcess(EXCEL_TABLE * table)
{
	CRAFTING::g_sCRAFTING_SLOTS_TYPE_COUNT = ExcelGetCountPrivate(table);
	return TRUE;
}


//returns true if the item is a crafting item
BOOL CRAFTING::CraftingIsCraftingModItem( UNIT *pCraftingModItem )
{
	return UnitIsA( pCraftingModItem, UNITTYPE_CRAFTING_MOD );
}

//returns the Total number of modification slots 
int CRAFTING::CraftingGetNumOfCraftingSlots( UNIT *pItem )
{
	ASSERT_RETINVALID( pItem );	
	ASSERT_RETINVALID( UnitGetGenus( pItem ) == GENUS_ITEM );	
	int nInventoryType = UnitGetInventoryType( pItem );
	return UnitGetStat(pItem, STATS_ITEM_SLOTS, nInventoryType );
}

//returns the Total number of used modification slots 
int CRAFTING::CraftingGetNumOfActiveCraftingSlots( UNIT *pItem )
{
	ASSERT_RETINVALID( pItem );	
	ASSERT_RETINVALID( UnitGetGenus( pItem ) == GENUS_ITEM );			
	return UnitInventoryGetCount( pItem );
}


//returns the Level of a given crafting slot by slot type
int CRAFTING::CraftingGetLevelOfCraftingSlot( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );	
	// get the stats list		
	return UnitGetStat( pItem, STATS_ITEM_CRAFTING_LEVEL );	
}


//returns the Level of a given crafting slot by slot type
int CRAFTING::CraftingGetLevelOfCraftingMod( UNIT *pCraftingMod )
{
	ASSERT_RETFALSE( pCraftingMod );	
	// get the stats list		
	return UnitGetStat( pCraftingMod, STATS_ITEM_CRAFTING_LEVEL );	
}


//returns if the slot is filled with another mod
BOOL CRAFTING::CraftingGetCraftingSlotIsEmpty( UNIT *pItem )
{
	ASSERT_RETINVALID( pItem );	
	ASSERT_RETINVALID( UnitGetGenus( pItem ) == GENUS_ITEM );			
	return (UnitInventoryGetCount( pItem ) == 0 )?TRUE:FALSE;

}




//returns true if the item can be mod'ed by an item
BOOL CRAFTING::CraftingCanModItem( UNIT *pUnitToMod,
								   UNIT *pItemDoingMod )
{
	ASSERT_RETFALSE( pUnitToMod );
	ASSERT_RETFALSE( pItemDoingMod );
	int nItemLevel = UnitGetStat( pUnitToMod, STATS_ITEM_CRAFTING_LEVEL );
	int nModLevel = UnitGetStat( pItemDoingMod, STATS_ITEM_CRAFTING_LEVEL );
	if( nItemLevel < nModLevel )
		return FALSE;

	PARAM dwParam = 0;
	int Location = INVALID_ID;
	int GridIdx = INVALID_ID;
	BOOL Done = FALSE;
	while ( !Done )
	{
		STATS_ENTRY stats_entry[16];
		int count = UnitGetStatsRange(pUnitToMod, STATS_ITEM_SLOTS, dwParam, stats_entry, 16);
		if (count <= 0)
		{
			break;
		}
		for (int ii = 0; ii < count; ii++)
		{
			int location = stats_entry[ii].GetParam();

			int slots = stats_entry[ii].value;
			if (slots <= 0)
			{
				continue;
			}

			for (int jj = 0; jj < slots; jj++)
			{

				Location = location;
				GridIdx = jj;
				Done = TRUE;
			}
		}
		if (count < 64)
		{
			break;
		}
		dwParam = stats_entry[count-1].GetParam() + 1;
	}
	return UnitInventoryTestIgnoreExisting( pUnitToMod, pItemDoingMod, Location, 0, 0 );
}

//returns the unit event the crafted item listens to
UNIT_EVENT CRAFTING::CraftingGetUnitEventForMod( UNIT *pItemDoingMod )
{
	ASSERT_RETVAL( pItemDoingMod, EVENT_INVALID );
	return (UNIT_EVENT)UnitGetStat( pItemDoingMod, STATS_ITEM_CRAFTING_EVENT_TYPE );
}
//returns the count for the items use
int CRAFTING::CraftingGetEventCountForMod( UNIT *pItemDoingMod )
{
	ASSERT_RETINVALID( pItemDoingMod );
	return UnitGetStat( pItemDoingMod, STATS_ITEM_CRAFTING_EVENT_COUNT );
}

//returns if the item can be broken down or not
BOOL CRAFTING::CraftingItemCanBeBrokenDown( UNIT *pItem, UNIT *pPlayer )
{
	ASSERT_RETINVALID( pItem );
	if( ItemBelongsToAnyMerchant( pItem ) )
		return FALSE;
	if( UnitGetUltimateOwner( pItem ) != pPlayer )
		return FALSE;
	return UnitIsA( pItem, UNITTYPE_EQUIPABLE );
}

//returns true if it can have a mod level( Heraldry level )
BOOL CRAFTING::CraftingItemCanHaveModLevel( UNIT *pItem )
{
	//only equipable and crafting mods can have crafting levels
	if( UnitIsA( pItem, UNITTYPE_EQUIPABLE ) ||
		CRAFTING::CraftingIsCraftingModItem(pItem ) )
	{
		return TRUE;	//we can't set crafting levels on anythign but crafting mods and equipable items
	}	
	return FALSE;
}
