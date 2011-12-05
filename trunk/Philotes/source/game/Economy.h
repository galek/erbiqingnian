//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __ECONOMY_H_
#define __ECONOMY_H_
#include "Currency.h"

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;


cCurrency EconomyItemBuyPrice( UNIT *pItem,
						   UNIT * merchant = NULL,
						   BOOL bForceIdentified = FALSE,
						   BOOL bIgnoreMerchantCheck = FALSE);

cCurrency EconomyItemSellPrice( UNIT *pItem );






#endif