//----------------------------------------------------------------------------
// s_crafting.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _S_CRAFTING_H_
#define _S_CRAFTING_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _CRAFTING_PROPERTIES_H_
#include "crafting_properties.h"
#endif

//----------------------------------------------------------------------------
// NAMESPACE
//----------------------------------------------------------------------------
namespace CRAFTING
{

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)


//registers events for all items the player holds
void s_CraftingRegisterModsForUpdating( UNIT *pPlayer );


//registers the player for events on mod'ed items
void s_CraftingPlayerRegisterMods( UNIT *pPlayer );

//registers the player for events on mod'ed items
void s_CraftingPlayerRegisterNewMod( UNIT *pPlayer, UNIT *pItem );



//removes the mod at a given slot
void s_CraftingRemoveMods( UNIT *pItemHoldingMod );




#endif
//----------------------------------------------------------------------------
// END NAMESPACE
//----------------------------------------------------------------------------
}
#endif