//----------------------------------------------------------------------------
// FILE: s_itemupgrade.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __S_ITEMUPGRADE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __S_ITEMUPGRADE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum ITEM_AUGMENT_TYPE;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Item upgrades
//----------------------------------------------------------------------------

void s_ItemUpgradeOpen(
	UNIT *pPlayer);
	
void s_ItemUpgradeClose(
	UNIT *pPlayer);

void s_ItemUpgrade(
	UNIT *pPlayer);

//----------------------------------------------------------------------------
// Item augment
//----------------------------------------------------------------------------

void s_ItemAugmentOpen(
	UNIT *pPlayer);

void s_ItemAugmentClose(
	UNIT *pPlayer);

void s_ItemAugment(
	UNIT *pPlayer,
	ITEM_AUGMENT_TYPE eType);

//----------------------------------------------------------------------------
// Item unmod
//----------------------------------------------------------------------------
	
void s_ItemUnModOpen(
	UNIT *pPlayer);

void s_ItemUnModClose(
	UNIT *pPlayer);
	
void s_ItemUnMod(
	UNIT *pPlayer);

//----------------------------------------------------------------------------


#endif