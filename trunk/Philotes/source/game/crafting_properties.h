//----------------------------------------------------------------------------
// crafting.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CRAFTING_PROPERTIES_H_
#define _CRAFTING_PROPERTIES_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "excel.h"
#include "unitevent.h"
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;

//----------------------------------------------------------------------------
// NAMESPACE
//----------------------------------------------------------------------------
namespace CRAFTING
{

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
const enum CRAFTING_PROPERTIES
{
	CRAFTING_PROP_MAX_MODS = 10
};


struct CRAFTING_SLOT_DATA
{
	char							szName[DEFAULT_INDEX_SIZE];	// name of slot
	WORD							wCode;						// code for slot
	int								nStringName;				// ui display name of slot
	int								nUnitTypesAllowed;			// unittype allowed in slot
	int								nMaxSlotsAllowed;			// max slots allowed for this type
	int								nUnitTypesAllowedtoHaveSlot; // unittypes the can have this slot
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------



//excel post process
BOOL CraftingExcelPostProcess(EXCEL_TABLE * table);

//returns true if the item is a crafting item
BOOL CraftingIsCraftingModItem( UNIT *pCraftingModItem );

//returns the Total number of modification slots 
int CraftingGetNumOfCraftingSlots( UNIT *pItem );

//returns the Total number of used modification slots 
int CraftingGetNumOfActiveCraftingSlots( UNIT *pItem );

//returns the Level of a given crafting slot
int CraftingGetLevelOfCraftingSlot( UNIT *pItem );

//returns the Level of a given crafting mod
int CraftingGetLevelOfCraftingMod( UNIT *pCraftingMod );


//returns if the slot is filled with another mod
BOOL CraftingGetCraftingSlotIsEmpty( UNIT *pItem );


//returns true if the item can be mod'ed by an item
BOOL CraftingCanModItem( UNIT *pUnitToMod,
						 UNIT *pItemDoingMod );

//returns the unit event the crafted item listens to
UNIT_EVENT CraftingGetUnitEventForMod( UNIT *pItemDoingMod );

//returns the count for the items use
int CraftingGetEventCountForMod( UNIT *pItemDoingMod );


//returns if the item can be broken down or not
BOOL CraftingItemCanBeBrokenDown( UNIT *pItem, UNIT *pPlayer );

//returns true if it can have a mod level( Heraldry level )
BOOL CraftingItemCanHaveModLevel( UNIT *pItem );

inline BOOL CraftingItemCanHaveCreatorsName( UNIT *pItem ){ return CRAFTING::CraftingItemCanHaveModLevel( pItem ); }


extern int g_sCRAFTING_SLOTS_TYPE_COUNT;

//----------------------------------------------------------------------------
// END NAMESPACE
//----------------------------------------------------------------------------
}
#endif