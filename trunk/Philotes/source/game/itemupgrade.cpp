//----------------------------------------------------------------------------
// FILE: itemupgrade.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "gameglobals.h"
#include "globalindex.h"
#include "inventory.h"
#include "items.h"
#include "itemupgrade.h"
#include "unit_priv.h"
#include "units.h"
#include "Economy.h"
#include "Currency.h"
#include "gamevariant.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Common Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemUpgradeGetItemSlot(
	void)
{
	return GlobalIndexGet( GI_INVENTORY_LOCATION_ITEM_UPGRADE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemUnModGetResultsSlot(
	void)
{
	return GlobalIndexGet( GI_INVENTORY_LOCATION_ITEM_UNMOD_RESULTS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *ItemUpgradeGetItem(
	UNIT *pPlayer)
{
	UNIT *pItemToUpgrade = NULL;
	
	// get the inv slot for the upgrade
	int nInvSlotItem = ItemUpgradeGetItemSlot();
	
	// iterate items in this inventory slot
	// iterate the trade offer 
	DWORD dwInvIterateFlags = 0;
	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryLocationIterate( pPlayer, nInvSlotItem, pItem, dwInvIterateFlags, FALSE )) != NULL)
	{
		
		// can only have one item in the upgrade slot
		if (pItemToUpgrade != NULL)
		{
			return NULL;
		}
		
		// save this item, but continue iterating so that we can catch more items that are in this slot
		pItemToUpgrade = pItem;
							
	}

	return pItemToUpgrade;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *ItemUnModGetItem(
	UNIT *pPlayer)
{
	// does the same thing
	return ItemUpgradeGetItem(pPlayer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddResource(
	UPGRADE_RESOURCES *pResources,
	int nItemClass,
	int nItemQuality,
	int nCount)
{
	ASSERTX_RETURN( pResources, "Expected resources" );
			
	if (pResources->nNumResources < MAX_ITEM_UPGRADE_RESOURCES && nCount > 0)
	{
		ITEM_UPGRADE_RESOURCE *pUpgradeResource = &pResources->tResources[ pResources->nNumResources++ ];
		pUpgradeResource->nItemClass = nItemClass;
		pUpgradeResource->nItemQuality = nItemQuality;
		pUpgradeResource->nCount = nCount;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetUpgradeScrapQuantity(
	const SCRAP_LOOKUP *pScrap,
	int nItemLevel)
{
	ASSERTX_RETINVALID( pScrap, "Expected scrap lookup" );
	const ITEM_LEVEL *pItemLevel = ItemLevelGetData( NULL, nItemLevel );
	ASSERTX_RETINVALID( pItemLevel, "Expected item level" );
	
	BOOL bIsSpecial = FALSE;
	if (pScrap->nItemQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( pScrap->nItemQuality );
		if (pItemQualityData)
		{
			if (pItemQualityData->bIsSpecialScrapQuality)
			{
				bIsSpecial = TRUE;
			}
		}
	}
	
	if (bIsSpecial)
	{
		return pItemLevel->nQuantityScrapSpecial;	
	}
	else
	{
		return pItemLevel->nQuantityScrap;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemUpgradeGetRequiredResources(
	UNIT *pItem,
	UPGRADE_RESOURCES *pResources)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( pResources, "Expected resources" );

	// init num resources
	pResources->nNumResources = 0;

	// get the item level
	int nItemLevel = UnitGetExperienceLevel( pItem );
		
	// setup scrap buffer
	SCRAP_LOOKUP tScrapLookupBuffer[ MAX_SCRAP_PIECES ];
	int nNumScrapPieces = 0;
	ItemGetScrapComponents( pItem, tScrapLookupBuffer, MAX_SCRAP_PIECES, &nNumScrapPieces, TRUE );
	
	BOOL bIsElite = FALSE;
	GAME * pGame = UnitGetGame( pItem );
	if ( pGame && GameIsVariant( pGame, GV_ELITE ) )
	{
		bIsElite = TRUE;
	}
	// add each of the special scrap pieces as required resources
	for (int i = 0; i < nNumScrapPieces; ++i)
	{
		const SCRAP_LOOKUP *pScrap = &tScrapLookupBuffer[ i ];
		
		int nQuantity = 0;

		if (pScrap->bExtraResource)
		{
			nQuantity = 1 + UnitGetStat(pItem, STATS_ITEM_UPGRADED_COUNT);
		}
		else
		{
			nQuantity = sGetUpgradeScrapQuantity( pScrap, nItemLevel );
		}

		if ( bIsElite )
		{
			nQuantity += PCT( nQuantity, ELITE_NANO_FORGE_COST_PERCENT );
		}
		// add to resources
		sAddResource( 
			pResources, 
			pScrap->nItemClass, 
			pScrap->nItemQuality,
			nQuantity);
		
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// returns the difference between what the player has and how many are required
int ItemUpgradeHasUpgradeResource(	
	UNIT *pPlayer,
	const ITEM_UPGRADE_RESOURCE *pUpgradeResource)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pUpgradeResource, "Expected upgrade resource" );

	// count the items matching this resource in the specified inventory slot
	DWORD dwInvIterateFlags = 0;
	SETBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);
	int nCount = UnitInventoryCountItemsOfType( 
		pPlayer, 
		pUpgradeResource->nItemClass, 
		dwInvIterateFlags, 
		pUpgradeResource->nItemQuality,
		INVLOC_NONE);

	return nCount - pUpgradeResource->nCount;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemUpgradeHasResources(
	UNIT *pPlayer,
	UPGRADE_RESOURCES *pResources)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pResources, "Expected resources" );
	
	// go through each resource
	for (int i = 0; i < pResources->nNumResources; ++i)
	{
		const ITEM_UPGRADE_RESOURCE *pUpgradeResource = &pResources->tResources[ i ];
		if (ItemUpgradeHasUpgradeResource( pPlayer, pUpgradeResource ) < 0)
		{
			return FALSE;
		}
	}
		
	return TRUE;  // has all required resources
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetNumLevelsForUpgrade( 
	int nItemLevel)
{
	const ITEM_LEVEL *pItemLevel = ItemLevelGetData( NULL, nItemLevel );
	ASSERTX_RETVAL( pItemLevel, 1, "Expected item level data" );
	return pItemLevel->nItemLevelsPerUpgrade;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_UPGRADE_INFO ItemUpgradeCanUpgrade(
	UNIT *pItem)
{
	ASSERTX_RETVAL( pItem, IFI_RESTRICED_UNKNOWN, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), IFI_RESTRICED_UNKNOWN, "Expected item" );

	// must be piece of weapon or armor
	if (UnitIsA( pItem, UNITTYPE_WEAPON ) == FALSE &&
		UnitIsA( pItem, UNITTYPE_ARMOR ) == FALSE)
	{
		return IFI_RESTRICED_UNKNOWN;
	}
	
	// item must be identified
	if (ItemIsUnidentified( pItem ) == TRUE)
	{
		return IFI_NOT_IDENTIFIED;
	}

	// unit data flag
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	ASSERTX_RETVAL( pItemData, IFI_RESTRICED_UNKNOWN, "Expected unit data" );
	if (UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_CANNOT_BE_UPGRADED ))
	{
		return IFI_RESTRICED_UNKNOWN;
	}
	
	// check to see if item has been upgraded too many times
	int nUpgradeCount = UnitGetStat( pItem, STATS_ITEM_UPGRADED_COUNT );
	if (nUpgradeCount >= MAX_ITEM_UPGRADES)
	{
		return IFI_LIMIT_REACHED;
	}

	// cannot exceed the experience level of the owner
	UNIT *pOwner = UnitGetUltimateOwner( pItem );
	ASSERTX_RETVAL( pOwner, IFI_RESTRICED_UNKNOWN, "Expected owner unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pOwner ), IFI_RESTRICED_UNKNOWN, "Expected player" );
	int nItemLevel = UnitGetExperienceLevel( pItem );
	int nOwnerLevel = UnitGetExperienceLevel( pOwner );
	if (nItemLevel > nOwnerLevel)
	{
		return IFI_MAX_OWNER_LEVEL;
	}

	// the new level cannot exceed the max item level
	GAME *pGame = UnitGetGame( pItem );
	int nItemLevelMax = ItemGetMaxPossibleLevel( pGame );
	int nLevelsPerUpgrade = ItemUpgradeGetNumItemLevelsPerUpgrade( nItemLevel );
	if (nItemLevel + nLevelsPerUpgrade >= nItemLevelMax)
	{
		return IFI_LIMIT_REACHED;
	}
	
	return IFI_UPGRADE_OK;
						
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemIsBuggedUniqueMissingAffixes( UNIT *pItem )
{
	ASSERTX_RETVAL( pItem, FALSE, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), FALSE, "Expected item" );
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	ASSERTX_RETVAL( pItemData, FALSE, "Expected item data" );

	// must be piece of weapon or armor
	if (UnitIsA( pItem, UNITTYPE_WEAPON ) == FALSE &&
		UnitIsA( pItem, UNITTYPE_ARMOR ) == FALSE)
	{
		return FALSE;
	}

	// Hellgate had a bug where uniques were dropping with no affixes, except
	// the legendary levelmod affix (which was common to all uniques), and the
	// affixes in the "required affix group" for the item. Both of these types
	// of afixes are hidden to players. The beneficial affixes which make the
	// uniques "unique" were missing. People
	// kept their broken uniques around hoping for the day we would fix them.
	// Rather than try to automatically detect and fix this weird case, I've 
	// decided to allow the players to choose to fix their items with a free
	// augmentation, which will apply all the expected affixes for their unique.
	// An automatic fix could be problematic, since it could increase the feeds,
	// and some players may like their broken items just fine -cmarch
	if (AppIsHellgate())
	{
		// A bugged unique will only have affixes from the "required affix groups"
		// and the legendary level mod affix, which all uniques have.
		// All the affixes will be applied and not attached.
		if (ItemGetQuality( pItem ) == GlobalIndexGet( GI_ITEM_QUALITY_UNIQUE ) &&
			AffixGetAffixCount( pItem, AF_ATTACHED ) == 0)
		{
			const int nAffixCount = AffixGetAffixCount( pItem, AF_APPLIED );
			for (int i = 0; i < nAffixCount; ++i)
			{
				int nAffix = AffixGetByIndex( pItem, AF_APPLIED, i );
				if (nAffix != INVALID_ID)
				{
					const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
					// the legendary levelmod affix is fine, skip (it was added as a random affix for unique quality items, probably as a result of the "normal" quality unit data being used by accident, which caused this whole mess)
					if (pAffixData && !AffixIsType( pAffixData, GlobalIndexGet( GI_AFFIX_TYPE_LEGENDARY_LEVELMOD ) ))
					{
						// if the affix is not in a required affix group for the item, reject this item (return false)
						BOOL bMatchesRequiredAffixGroup = FALSE;
						for (int j = 0; j < MAX_REQ_AFFIX_GROUPS; ++j)
						{
							int nRequiredAffixGroup = pItem->pUnitData->nReqAffixGroup[j];
							if (nRequiredAffixGroup != INVALID_LINK &&
								pAffixData->nGroup == nRequiredAffixGroup)
							{
								bMatchesRequiredAffixGroup = TRUE;
								break;
							}
						}

						if (!bMatchesRequiredAffixGroup)
							return FALSE;
					}
				}
			}
			// all applied affixes match the search criteria, this is a bugged unique
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemAugmentCanApplyFreeBugFix( UNIT *pItem )
{
	if (sItemIsBuggedUniqueMissingAffixes( pItem ))
	{
		// item must not have any mods in it
		UNIT *pMod = UnitInventoryIterate(pItem, NULL, 0, FALSE);
		return pMod == NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemAugmentCanApplyFreeUnMod( UNIT *pItem )
{
	if (!TESTBIT( UnitGetStat( pItem, STATS_ITEM_FIX_FLAGS ), IFF_FREE_UNMOD_APPLIED_BIT ) &&
		sItemIsBuggedUniqueMissingAffixes( pItem ))
	{
		// item must have mods in it
		UNIT *pMod = UnitInventoryIterate(pItem, NULL, 0, FALSE);
		return pMod != NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_AUGMENT_INFO ItemAugmentCanAugment( UNIT *pItem, 
										 BOOL bIncludeAffixCount ) //this probably should be turned into flags
{
	ASSERTX_RETVAL( pItem, IAI_RESTRICTED_UNKNOWN, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), IAI_RESTRICTED_UNKNOWN, "Expected item" );
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	ASSERTX_RETVAL( pItemData, IAI_RESTRICTED_UNKNOWN, "Expected item data" );

	// must be piece of weapon or armor
	if (UnitIsA( pItem, UNITTYPE_WEAPON ) == FALSE &&
		UnitIsA( pItem, UNITTYPE_ARMOR ) == FALSE)
	{
		return IAI_RESTRICTED_UNKNOWN;
	}

	// check for bugged uniques that can be fixed
	if (sItemIsBuggedUniqueMissingAffixes( pItem ))
	{
		// item must not have any mods in it
		return (UnitInventoryIterate(pItem, NULL, 0, FALSE) == NULL) ?
			IAI_AUGMENT_OK : 
			IAI_RESTRICTED_UNKNOWN;
	}

	if(AppIsTugboat())
	{
		if (ItemGetQuality(pItem) == GlobalIndexGet( GI_ITEM_QUALITY_UNIQUE ))
		{
			return IAI_RESTRICTED_UNIQUE;
		}
	}
	

	// unit data flag
	ASSERTX_RETVAL( pItemData, IAI_RESTRICTED_UNKNOWN, "Expected unit data" );
	if (UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_CANNOT_BE_AUGMENTED ))
	{
		return IAI_RESTRICTED_UNKNOWN;
	}

	// must be identified
	if (ItemIsUnidentified( pItem ) == TRUE)
	{
		return IAI_NOT_IDENTIFIED;
	}

	// item must have enough open affix slots
	if ( bIncludeAffixCount && AffixGetAffixCount( pItem, AF_ALL_FAMILIES ) >= AffixGetMaxAffixCount())
	{
		return IAI_PROPERTIES_FULL;
	}

	// check to see if item has been upgraded too many times
	int nAugmentCount = UnitGetStat( pItem, STATS_ITEM_AUGMENTED_COUNT );
	if (nAugmentCount >= MAX_ITEM_AUGMENTS)
	{
		return IAI_LIMIT_REACHED;
	}
	return IAI_AUGMENT_OK;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_AUGMENT_INFO ItemAugmentCanAugment(
	UNIT *pItem,
	ITEM_AUGMENT_TYPE eAugmentType)
{
	ITEM_AUGMENT_INFO nInfo = ItemAugmentCanAugment( pItem, TRUE );
	if( nInfo != IAI_AUGMENT_OK )
		return nInfo;



	// get the augmentation cost
	cCurrency Cost = ItemAugmentGetCost( pItem, eAugmentType );
	ASSERTX_RETVAL(!Cost.IsNegative(), IAI_RESTRICTED_UNKNOWN, "Negative augment cost");
	
	// the owner must be able to pay the cost
	UNIT *pOwner = UnitGetUltimateOwner( pItem );
	ASSERTX_RETVAL( pOwner, IAI_RESTRICTED_UNKNOWN, "Expected owner unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pOwner ), IAI_RESTRICTED_UNKNOWN, "Expected player" );
	if (UnitGetCurrency( pOwner ) < Cost)
	{
		return IAI_NOT_ENOUGH_MONEY;
	}	
			
	// augment OK
	return IAI_AUGMENT_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_UNMOD_INFO ItemCanUnMod(
	UNIT *pItem)
{
	ASSERTX_RETVAL( pItem, IUI_RESTRICTED_UNKNOWN, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), IUI_RESTRICTED_UNKNOWN, "Expected item" );

	// must be piece of weapon or armor
	if (UnitIsA( pItem, UNITTYPE_WEAPON ) == FALSE &&
		UnitIsA( pItem, UNITTYPE_ARMOR ) == FALSE)
	{
		return IUI_RESTRICTED_UNKNOWN;
	}

	// unit data flag
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	ASSERTX_RETVAL( pItemData, IUI_RESTRICTED_UNKNOWN, "Expected unit data" );
	if (UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_CANNOT_BE_DEMODDED ))
	{
		return IUI_RESTRICTED_UNKNOWN;
	}

	UNIT *pOwner = UnitGetUltimateOwner( pItem );
	ASSERTX_RETVAL( pOwner, IUI_RESTRICTED_UNKNOWN, "Expected owner unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pOwner ), IUI_RESTRICTED_UNKNOWN, "Expected player" );

	// any previous results must be removed first.
	int nResultsLoc = ItemUnModGetResultsSlot();
	ASSERTX_RETVAL( nResultsLoc != INVLOC_NONE, IUI_RESTRICTED_UNKNOWN, "Could not find results inventory" );
	UNIT *pUnit = UnitInventoryLocationIterate(pOwner, nResultsLoc, NULL);
	if (pUnit)
	{
		return IUI_RESULTS_NOT_EMPTY;
	}

	// item must have something to remove
	pUnit = UnitInventoryIterate(pItem, NULL, 0, FALSE);
	if (!pUnit)
	{
		return IUI_NO_MODS;
	}

		
	// the owner must be able to pay the cost
	if (UnitGetCurrency( pOwner ) <  ItemUnModGetCost( pItem ))
	{
		return IUI_NOT_ENOUGH_MONEY;
	}	
			
	// unmod OK
	return IUI_UNMOD_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetAugmentationCost(
	int nItemLevel,
	ITEM_AUGMENT_TYPE eAugmentType)
{
	const ITEM_LEVEL *pItemLevel = ItemLevelGetData( NULL, nItemLevel );
	ASSERTX_RETVAL( pItemLevel, 999999, "Expected item level" );
	return pItemLevel->nAugmentCost[ eAugmentType ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency ItemAugmentGetCost(
	UNIT *pItem,
	ITEM_AUGMENT_TYPE eAugmentType)
{
	ASSERTX_RETVAL( pItem, cCurrency( 0, 0 ), "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), cCurrency( 0, 0 ), "Expected item" );
	ASSERTX_RETVAL( eAugmentType > IAUGT_INVALID && eAugmentType < IAUGT_NUM_TYPES, cCurrency( 0, 0 ), "Invalid augmentation type" );

	// free fix for broken uniques
	if (ItemAugmentCanApplyFreeBugFix( pItem ))
	{
		return cCurrency( 0, 0 );
	}

	// get the cost per item level
	int nItemLevel = UnitGetExperienceLevel( pItem );
	int nCost = sGetAugmentationCost( nItemLevel, eAugmentType );
	if( eAugmentType == IAUGT_ADD_RANDOM )
	{
		float fPrice = (float)nCost;
		int nAffixes = AffixGetAffixCount( pItem, AF_APPLIED );
		fPrice *= ( ItemGetModSlots( pItem ) * .5f ) + 1;
		int nQuality = ItemGetQuality( pItem );
		if (nQuality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData( nQuality );
			if (quality_data && quality_data->fPriceMultiplier > 0.0f )
			{
				fPrice *= quality_data->fPriceMultiplier;
			}
		}
		fPrice *= ( (float)nAffixes ) + 1;
		nCost = (int)fPrice;
	}
	GAME * pGame = UnitGetGame( pItem );
	if ( pGame && GameIsVariant( pGame, GV_ELITE ) )
	{
		nCost += PCT( nCost, ELITE_AUGMENT_COST_PERCENT );
	}
	return cCurrency( nCost, 0 );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency ItemUnModGetCost(
	UNIT *pItem)
{
	ASSERTX_RETVAL( pItem, cCurrency( -1, 0 ), "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pItem, UNITTYPE_ITEM ), cCurrency( -1, 0 ), "Expected item" );

	// free fix for broken uniques
	if (ItemAugmentCanApplyFreeUnMod( pItem ))
	{
		return cCurrency( 0, 0 );
	}

	return EconomyItemSellPrice( pItem ); // ItemGetSellPrice(pItem);		// for now
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemUpgradeGetCanDoStat(
	ITEM_UPGRADE_TYPE eType)
{
	switch (eType)
	{
		case IUT_UPGRADE:	return STATS_CAN_ITEM_UPGRADE;
		case IUT_AUGMENT:	return STATS_CAN_ITEM_AUGMENT;
		case IUT_UNMOD:		return STATS_CAN_ITEM_UNMOD;
		break;
	}
	ASSERTV_RETINVALID( 0, "Unknown stat for item upgrade type '%d'", eType );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemUpgradeCanDo(
	UNIT *pPlayer,
	ITEM_UPGRADE_TYPE eType)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTV_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player (%s)", UnitGetDevName( pPlayer ) );

	// get the stat for the requested item upgrade type
	int nStat = ItemUpgradeGetCanDoStat( eType );
	ASSERTX_RETFALSE( nStat != INVALID_LINK, "Expected item upgrade stat" );
	return UnitGetStat( pPlayer, nStat );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemUpgradeGetNumItemLevelsPerUpgrade(
	int nItemLevel)
{
	const ITEM_LEVEL *pItemLevel = ItemLevelGetData( NULL, nItemLevel );
	ASSERTX_RETZERO( pItemLevel, "Expected item level" );
	return pItemLevel->nItemLevelsPerUpgrade;
}
