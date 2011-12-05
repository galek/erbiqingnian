//----------------------------------------------------------------------------
// FILE: itemupgrade.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __ITEMUPGRADE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __ITEMUPGRADE_H_

#ifndef __CURRENCY_H_
#include "Currency.h"
#endif
//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum ITEM_UPGRADE_TYPE
{
	IUT_INVALID = -1,
	
	IUT_UPGRADE,
	IUT_AUGMENT,
	IUT_UNMOD,
	
	IUT_NUM_TYPES,				// keep this last
	
};

//----------------------------------------------------------------------------
enum ITEM_UPGRADE_CONSTANTS
{
	MAX_ITEM_UPGRADE_RESOURCES = 5
};

//----------------------------------------------------------------------------
struct ITEM_UPGRADE_RESOURCE
{
	int nItemClass;			// resource excel row
	int nItemQuality;		// resource quality excel row
	int nCount;				// number of resources needed
};

//----------------------------------------------------------------------------
struct UPGRADE_RESOURCES
{
	ITEM_UPGRADE_RESOURCE tResources[ MAX_ITEM_UPGRADE_RESOURCES ];
	int nNumResources;
};

//----------------------------------------------------------------------------
enum ITEM_UPGRADE_INFO
{
	IFI_UPGRADE_OK,				// ok to upgrade item
	IFI_MAX_OWNER_LEVEL,		// can't upgrade higher than owner level
	IFI_NOT_IDENTIFIED,			// can't upgrade unidentified items
	IFI_LIMIT_REACHED,			// can't upgrade item, its been upgraded too many times already
	
	IFI_RESTRICED_UNKNOWN		// can't upgrade unknown reason
};

//----------------------------------------------------------------------------
enum ITEM_AUGMENT_INFO
{
	IAI_AUGMENT_OK,				// ok to augment item
	IAI_NOT_IDENTIFIED,			// can't augment - item must be identified
	IAI_PROPERTIES_FULL,		// can't augment, no property slots on item open
	IAI_NOT_ENOUGH_MONEY,		// need more money to augment
	IAI_LIMIT_REACHED,			// max number of augments reached
	IAI_RESTRICTED_UNIQUE,		// the item is unique - no augmenting (tugboat)

	IAI_RESTRICTED_UNKNOWN,		// can't augment - unknown reason
};

//----------------------------------------------------------------------------
enum ITEM_UNMOD_INFO
{
	IUI_UNMOD_OK,				// ok to unmod item
	IUI_NO_MODS,				// can't unmod, no mods to remove
	IUI_NOT_ENOUGH_MONEY,		// need more money to unmod
	IUI_RESULTS_NOT_EMPTY,		// the results inventory is not empty
	IUI_RESTRICTED_UNKNOWN,		// can't unmod - unknown reason
};

//----------------------------------------------------------------------------
enum ITEM_AUGMENT_TYPE
{	
	IAUGT_INVALID = -1,
	
	IAUGT_ADD_COMMON,				// add a common property
	IAUGT_ADD_RARE,				// add a rare property
	IAUGT_ADD_LEGENDARY,			// add a legendary property
	IAUGT_ADD_RANDOM,			// add a random property
	
	IAUGT_NUM_TYPES				// keep this last please
	
};


//----------------------------------------------------------------------------
// 32 bit max for stat (STATS_ITEM_FIX_FLAGS)
enum ITEMFIX_FLAGS
{
	IFF_UNIQUE_MISSING_AFFIXES_REROLLED_BIT,		// rerolled a bugged unique, see comments to sItemIsBuggedUniqueMissingAffixes in itemupgrade.cpp			
	IFF_FREE_UNMOD_APPLIED_BIT,						// gave a free unmod for an item, probaly because it is a bugged unique that needs a reroll
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

int ItemUpgradeGetItemSlot(
	void);

int ItemUnModGetResultsSlot(
	void);

UNIT *ItemUpgradeGetItem(
	UNIT *pPlayer);

UNIT *ItemUnModGetItem(
	UNIT *pPlayer);

void ItemUpgradeGetRequiredResources(
	UNIT *pItem,
	UPGRADE_RESOURCES *pResources);

BOOL ItemUpgradeHasResources(
	UNIT *pPlayer,
	UPGRADE_RESOURCES *pResources);

ITEM_UPGRADE_INFO ItemUpgradeCanUpgrade(
	UNIT *pItem);

ITEM_AUGMENT_INFO ItemAugmentCanAugment(
	UNIT *pItem, 
	BOOL bIncludeAffixCount );

ITEM_AUGMENT_INFO ItemAugmentCanAugment(
	UNIT *pItem,
	ITEM_AUGMENT_TYPE eAugmentType);

cCurrency ItemAugmentGetCost(
	UNIT *pItem,
	ITEM_AUGMENT_TYPE eAugmentType);

BOOL ItemAugmentCanApplyFreeBugFix( 
	UNIT *pItem );

BOOL ItemAugmentCanApplyFreeUnMod( 
	UNIT *pItem );

ITEM_UNMOD_INFO ItemCanUnMod(
	UNIT *pItem);

cCurrency ItemUnModGetCost(
	UNIT *pItem);

int ItemUpgradeGetCanDoStat(
	ITEM_UPGRADE_TYPE eType);

BOOL ItemUpgradeCanDo(
	UNIT *pPlayer,
	ITEM_UPGRADE_TYPE eType);

int ItemUpgradeGetNumItemLevelsPerUpgrade(
	int nItemLevel);

int ItemUpgradeHasUpgradeResource(			// returns the difference between what the player has and how many are required
	UNIT *pPlayer,
	const ITEM_UPGRADE_RESOURCE *pUpgradeResource);

#endif