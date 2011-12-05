//----------------------------------------------------------------------------
// FILE: s_itemupgrade.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "bookmarks.h"
#include "c_ui.h"
#include "clients.h"
#include "Currency.h"
#include "dbunit.h"
#include "gameglobals.h"
#include "inventory.h"
#include "itemupgrade.h"
#include "items.h"
#include "player.h"
#include "s_itemupgrade.h"
#include "s_message.h"
#include "unitfile.h"
#include "units.h"
#include "achievements.h"

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTakeResources(
	UNIT *pPlayer,
	const UPGRADE_RESOURCES *pResources)

{
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pResources, "Expected resources" );

	DWORD dwInvIterateFlags = 0;
	SETBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);

	// go through each of the resource requirements
	for (int i = 0; i < pResources->nNumResources; ++i)
	{
		const ITEM_UPGRADE_RESOURCE *pUpgradeResource = &pResources->tResources[ i ];
		int nNumNeeded = pUpgradeResource->nCount;
				
		// iterate items in the required slot
		UNIT *pItem = UnitInventoryIterate( pPlayer, NULL, dwInvIterateFlags );
		while (pItem && nNumNeeded > 0)
		{
		
			// get next item
			UNIT *pNext = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags );
			
			if (UnitIsA( pItem, UNITTYPE_ITEM ))
			{
			
				//  must match class
				if (UnitGetClass( pItem ) == pUpgradeResource->nItemClass)
				{
				
					// must match quality (if set)
					if (pUpgradeResource->nItemQuality == INVALID_LINK ||
						ItemGetQuality( pItem ) == pUpgradeResource->nItemQuality)
					{
					
						// take the count away from the item
						int nQuantity = ItemGetQuantity( pItem );
						if (nQuantity > nNumNeeded)
						{
							nQuantity -= nNumNeeded;
#if ISVERSION(SERVER_VERSION)
							// save unit-related data so we'll have it when the logging occurs
							UNIT *pUltimateOwner = UnitGetUltimateOwner( pItem );
							DBOPERATION_CONTEXT opContext(pItem, pUltimateOwner, DBOC_UPGRADE);
							ItemSetQuantity( pItem, nQuantity, &opContext );
#else
							ItemSetQuantity( pItem, nQuantity );
#endif
							break;
						}
						else
						{
							nNumNeeded -= nQuantity;
							UnitFree( pItem, UFF_SEND_TO_CLIENTS, UFC_ITEM_UPGRADE );
							if (nNumNeeded == 0)
							{
								break;
							}
						}
					}
				}
			}
			
			// go to next item
			pItem = pNext;
			
		}	
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoItemUpgrade( 
	UNIT *pPlayer,
	UNIT *pItem,
	const UPGRADE_RESOURCES *pResources,
	BOOL bPlayerInitiatedUpgrade = TRUE)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( !bPlayerInitiatedUpgrade || ItemUpgradeCanUpgrade( pItem ) == IFI_UPGRADE_OK, "Cannot upgrade item anymore" );
	ASSERTX_RETURN( !bPlayerInitiatedUpgrade || pResources, "Expected resources" );
	GAME *pGame = UnitGetGame( pItem );
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	
	// how many times has this item been upgraded
	int nUpgradeCount = UnitGetStat( pItem, STATS_ITEM_UPGRADED_COUNT );
	ASSERTX_RETURN( nUpgradeCount < MAX_ITEM_UPGRADES, "Item upgraded too many times already" );
	
	// how many item levels are there
	int nItemLevelMax = ItemGetMaxPossibleLevel( pGame );
	
	// setup spec for upgraded item
	const UNIT_DATA * item_data = UnitGetData(pItem);
	ASSERT_RETURN(item_data);

	int curlevel = UnitGetStat(pItem, STATS_LEVEL);
	ASSERT_RETURN(curlevel < nItemLevelMax);

	int levelbonus = ItemUpgradeGetNumItemLevelsPerUpgrade(curlevel);
	ASSERT_RETURN(levelbonus > 0);
	if (curlevel + levelbonus > nItemLevelMax)
	{
		levelbonus = nItemLevelMax - curlevel;
	}

	ITEM_SPEC itemspec;
	itemspec.nItemClass = UnitGetClass(pItem);
	itemspec.nItemLookGroup = UnitGetLookGroup(pItem);
	itemspec.flScale = UnitGetScale(pItem);
	itemspec.nLevel = curlevel + levelbonus;
	itemspec.nQuality = ItemGetQuality(pItem); // ItemGetUpgradeQuality( pItem );
	ASSERT_RETURN(itemspec.nLevel <= nItemLevelMax);	// not really necessary

	// init the level based stats with the new level
	ITEM_INIT_CONTEXT tItemInitContext(pGame, pItem, pPlayer, itemspec.nItemClass, item_data, &itemspec);
	tItemInitContext.level = itemspec.nLevel;
	UnitChangeStat(pItem, STATS_LEVEL, levelbonus);
	s_ItemInitPropertiesSetLevelBasedStats( tItemInitContext );

	// set the upgrade count on the new item
	UnitSetStat( pItem, STATS_ITEM_UPGRADED_COUNT, nUpgradeCount + 1 );
		
	// you cannot trade this item to others
//	ItemSetNoTrade( pItemNew, TRUE );
	
	if (bPlayerInitiatedUpgrade)
	{
		// take all the resources away from the player
		sTakeResources( pPlayer, pResources );

		// tell client the item has been upgraded
		MSG_SCMD_ITEM_UPGRADED tMessage;
		tMessage.idItem = UnitGetId( pItem );
		s_SendMessage( pGame, ClientGetId( pClient ), SCMD_ITEM_UPGRADED, &tMessage );

		// inform the achievements system
		s_AchievementsSendItemUpgrade(pItem, pPlayer);

		UnitTriggerEvent( pGame, EVENT_ITEM_UPGRADE, pItem, pPlayer, NULL );
	}

#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUpgrade(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// if doesn't have access to item upgrade anymore, forget it
	if (ItemUpgradeCanDo( pPlayer, IUT_UPGRADE ) == FALSE)
	{
		return;  // not cheating, this a valid case with repeated ui clicks
	}
	
	// get item in the upgrade slot
	UNIT *pItem = ItemUpgradeGetItem( pPlayer );
	if (pItem && ItemUpgradeCanUpgrade( pItem ) == IFI_UPGRADE_OK)
	{
	
		// get the resources necessary to upgrade this item
		UPGRADE_RESOURCES tResources;
		ItemUpgradeGetRequiredResources( pItem, &tResources );
		
		// check the contents of the resources slot and see if the player has them all there
		if (ItemUpgradeHasResources( pPlayer, &tResources ))
		{
			sDoItemUpgrade( pPlayer, pItem, &tResources );
		}
			
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUnMod(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// if doesn't have access to item upgrade anymore, forget it
	if (ItemUpgradeCanDo( pPlayer, IUT_UNMOD ) == FALSE)
	{
		return;  // not cheating, this a valid case with repeated ui clicks
	}

	// get item in the upgrade slot
	UNIT *pItem = ItemUpgradeGetItem( pPlayer );
	if (pItem && ItemCanUnMod( pItem ) == IUI_UNMOD_OK)
	{
		BOOL bFreeUnModForBugFix = ItemAugmentCanApplyFreeUnMod( pItem );

		int nResultLoc = ItemUnModGetResultsSlot();
		ASSERTX_RETURN(nResultLoc != INVLOC_NONE, "Could not find location to place removed mods");

		// take money from player
		cCurrency Cost = ItemUnModGetCost( pItem );
		UNITLOG_ASSERT_RETURN(UnitRemoveCurrencyValidated( pPlayer, Cost ), pPlayer);

		UNIT *pMod = NULL;
		while ((pMod = UnitInventoryIterate(pItem, NULL, 0, FALSE)) != NULL)
		{
			UnitClearStat( pMod, STATS_ITEM_LOCKED_IN_INVENTORY, 0 );
			if (!InventoryMoveToAnywhereInLocation(pPlayer, pMod, nResultLoc, 0))
			{
				// bad news, do some damage control
				ASSERTX_RETURN(FALSE, "Failed to remove a mod from the item");
				//  awww, do something better than that.
			}
		}

		if (bFreeUnModForBugFix)
		{
			DWORD dwFixItemFlags = UnitGetStat( pItem, STATS_ITEM_FIX_FLAGS );
			SETBIT( dwFixItemFlags, IFF_FREE_UNMOD_APPLIED_BIT );
			UnitSetStat( pItem, STATS_ITEM_FIX_FLAGS, dwFixItemFlags );
		}

		GAME * pGame = UnitGetGame( pItem );
		ASSERT_RETURN( pGame );
		UnitTriggerEvent( pGame, EVENT_ITEM_UNMOD, pItem, pPlayer, NULL );
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoItemAugment( 
	UNIT *pPlayer,
	UNIT *pItem, 
	ITEM_AUGMENT_TYPE eType)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( eType > IAUGT_INVALID && eType < IAUGT_NUM_TYPES, "Invalid augmentation type" );
	ASSERTX_RETURN( ItemAugmentCanAugment( pItem, eType ) == IAI_AUGMENT_OK, "Cannot augment item" );	

	// how many times has this item been augmented
	int nAugmentCount = UnitGetStat( pItem, STATS_ITEM_AUGMENTED_COUNT );
	ASSERTX_RETURN( nAugmentCount < MAX_ITEM_AUGMENTS, "Item augmented too many times already" );

	// what type of affix will we add
	int nAffixType = INVALID_LINK;
	switch (eType)
	{
		case IAUGT_ADD_COMMON:		nAffixType = GlobalIndexGet( GI_AFFIX_TYPE_COMMON ); break;
		case IAUGT_ADD_RARE:		nAffixType = GlobalIndexGet( GI_AFFIX_TYPE_RARE ); break;
		case IAUGT_ADD_LEGENDARY:	nAffixType = GlobalIndexGet( GI_AFFIX_TYPE_LEGENDARY ); break;
		case IAUGT_ADD_RANDOM:		nAffixType = INVALID_ID; break;
		default: break;
	}
	ASSERTX_RETURN( eType == IAUGT_ADD_RANDOM || nAffixType != INVALID_LINK, "Invalid affix type for augmentation" );
	
	// find a property to add
	int nAffix = INVALID_LINK;
	int nTries = 32;
	while (nTries--)
	{
	
		// pick an affix ... this will attach the affix to the item, it will not apply it
		DWORD dwAffixPickFlags = 0;
		SETBIT( dwAffixPickFlags, APF_ALLOW_PROCS_BIT );
		SETBIT( dwAffixPickFlags, APF_FOR_AUGMENTATION_BIT);
		nAffix = s_AffixPick( pItem, dwAffixPickFlags, nAffixType, NULL, INVALID_LINK );
		if (nAffix != INVALID_LINK)
		{
			break;
		}
		
	}


	// must have an affix to continue
	if (nAffix != INVALID_LINK)
	{
		// take money from player
		cCurrency Cost = ItemAugmentGetCost( pItem, eType );
		UNITLOG_ASSERT_RETURN(UnitRemoveCurrencyValidated( pPlayer, Cost ), pPlayer );

		// update the count
		UnitSetStat( pItem, STATS_ITEM_AUGMENTED_COUNT, nAugmentCount + 1 );
				
		// apply the attached affixes that we picked
		s_AffixApplyAttached( pItem );
					
		// resend to clients
		s_ItemResend( pItem );
		
		// commit to DB
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pItem );
		s_DatabaseUnitOperation( pUltimateOwner, pItem, DBOP_UPDATE, 0, DBUF_FULL_UPDATE );

	}
	
	// tell the client that we're done
	MSG_SCMD_ITEM_AUGMENTED tMessage;
	tMessage.idItem = UnitGetId( pItem );
	tMessage.nAffixAugmented = nAffix;

	// send	
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_ITEM_AUGMENTED, &tMessage );

	UnitTriggerEvent( pGame, EVENT_ITEM_AUGMENT, pItem, pPlayer, NULL );
								
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoUniqueItemAffixBugFix( 
	UNIT *pPlayer,
	UNIT *pItem )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( ItemAugmentCanApplyFreeBugFix( pItem ), "Cannot fix affixes for item" );	

	GAME *pGame = UnitGetGame( pPlayer );

	const UNIT_DATA * item_data = UnitGetData(pItem);
	ASSERT_RETURN(item_data);

	int nUpgradeCount = UnitGetStat( pItem, STATS_ITEM_UPGRADED_COUNT );

	ITEM_SPEC itemspec;
	itemspec.nItemClass = UnitGetClass(pItem);
	itemspec.nItemLookGroup = UnitGetLookGroup(pItem);
	itemspec.flScale = UnitGetScale(pItem);
	// this item level will be ignored when using the fixed level
	itemspec.nLevel = PIN((UnitGetStat(pItem, STATS_LEVEL) - 2 * nUpgradeCount), item_data->nItemExperienceLevel, MAX(item_data->nMaxLevel, item_data->nItemExperienceLevel));
	itemspec.nQuality = ItemGetQuality(pItem);
	SETBIT( itemspec.dwFlags, ISF_IDENTIFY_BIT, TRUE );


	// spawn the item
	UNIT * pNewItem = s_ItemSpawn( pGame, itemspec, NULL );
	if (pNewItem)
	{		
		// this item is now ready to do network communication
		UnitSetCanSendMessages( pNewItem, TRUE );

		ROOM *pRoom = UnitGetRoom( pPlayer );
		if ( pRoom )
		{
			// put in world (yeah, kinda lame, but it keeps the pickup code path clean)
			UnitWarp( 
				pNewItem,
				pRoom, 
				UnitGetPosition( pPlayer ),
				cgvYAxis,
				cgvZAxis,
				0);
		}

		// pick up item
		UNIT *pPickedUp = s_ItemPickup( pPlayer, pNewItem );
		if (!pPickedUp)
		{
			UnitFree( pNewItem, 0 );
			pNewItem = NULL;
		}
		else
		{
			if (!InventorySwapLocation( pPickedUp, pItem ))
			{
				if (pPickedUp != pNewItem)
					UnitFree( pPickedUp, 0 );
				pPickedUp = NULL;
				UnitFree( pNewItem, 0 );
				pNewItem = NULL;
			}
			else
			{
				DWORD dwItemFixFlags = 0;
				SETBIT( dwItemFixFlags, IFF_UNIQUE_MISSING_AFFIXES_REROLLED_BIT );
				UnitSetStat( pPickedUp, STATS_ITEM_FIX_FLAGS, dwItemFixFlags );

				for (int i = 0; i < nUpgradeCount; ++i)
				{
					sDoItemUpgrade( pPlayer, pPickedUp, NULL, FALSE );
				}

				UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_ALL, CLF_FORCE_SEND_REMOVE_MESSAGE );		
				UnitFree( pRemoved, 0 );
				pRemoved = NULL;
				pItem = NULL;

				// tell the client that we're done
				MSG_SCMD_ITEM_AUGMENTED tMessage;
				tMessage.idItem = UnitGetId( pNewItem );
				tMessage.nAffixAugmented = AffixGetByIndex( pNewItem, AF_APPLIED, 0 );

				// send	
				GAMECLIENTID idClient = UnitGetClientId( pPlayer );
				s_SendMessage( pGame, idClient, SCMD_ITEM_AUGMENTED, &tMessage );
			}
		}
	}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemAugment(
	UNIT *pPlayer,
	ITEM_AUGMENT_TYPE eType)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// if doesn't have access to item upgrade anymore, forget it
	if (ItemUpgradeCanDo( pPlayer, IUT_AUGMENT ) == FALSE)
	{
		return;  // not cheating, this a valid case with repeated ui clicks
	}

	// get the item to augment
	UNIT *pItem = ItemUpgradeGetItem( pPlayer );
	if (pItem)
	{
		
		// must be able to augment it		
		if (ItemAugmentCanApplyFreeBugFix( pItem ))
		{
			sDoUniqueItemAffixBugFix( pPlayer, pItem );
		}
		else if (ItemAugmentCanAugment( pItem, eType ) == IAI_AUGMENT_OK)
		{
			// do the augmentation
			sDoItemAugment( pPlayer, pItem, eType );
		}
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetCanUpgradeItems(
	UNIT *pPlayer,
	ITEM_UPGRADE_TYPE eType,
	BOOL bValue)
{
	int nStat = ItemUpgradeGetCanDoStat( eType );
	ASSERTX_RETURN( nStat != INVALID_LINK, "Expected stat for item upgrade type" );
	UnitSetStat( pPlayer, nStat, bValue );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUpgradeOpen(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// close any previous upgrade session that was open or items that are stuck in the slots
	s_ItemUpgradeClose( pPlayer );
	
	// mark that we now have the ability to do item upgrades
	sSetCanUpgradeItems( pPlayer, IUT_UPGRADE, TRUE );

	// we need to make sure the client knows about their stash so they know how many ingredients they have there
	s_sSendStashToClient( pPlayer );

	// tell client to open the item upgrade UI
	s_PlayerToggleUIElement( pPlayer, UIE_ITEM_UPGRADE, UIEA_OPEN );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUpgradeClose(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// no longer can we do upgrades
	sSetCanUpgradeItems( pPlayer, IUT_UPGRADE, FALSE );

	// move any remaining items in the upgrade slots back to the player backpack
	int nInvLocItem = ItemUpgradeGetItemSlot();
	s_ItemsRestoreToStandardLocations( pPlayer, nInvLocItem );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemAugmentOpen(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// close any previous upgrade session that was open or items that are stuck in the slots
	s_ItemAugmentClose( pPlayer );

	// mark that we now have the ability to do item augmentations
	sSetCanUpgradeItems( pPlayer, IUT_AUGMENT, TRUE );

	// tell client to open UI
	s_PlayerToggleUIElement( pPlayer, UIE_ITEM_AUGMENT, UIEA_OPEN );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemAugmentClose(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// no longer can we do augments
	sSetCanUpgradeItems( pPlayer, IUT_AUGMENT, FALSE );

	// move items left over in UI back to player backpack
	int nInvLocItem = ItemUpgradeGetItemSlot();
	s_ItemsRestoreToStandardLocations( pPlayer, nInvLocItem );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUnModOpen(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// close any previous upgrade session that was open or items that are stuck in the slots
	s_ItemUnModClose( pPlayer );

	// mark that we now have the ability to do item de-modding
	sSetCanUpgradeItems( pPlayer, IUT_UNMOD, TRUE );

	// tell client to open UI
	s_PlayerToggleUIElement( pPlayer, UIE_ITEM_UNMOD, UIEA_OPEN );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUnModClose(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// no longer can we do augments
	sSetCanUpgradeItems( pPlayer, IUT_UNMOD, FALSE );

	// move items left over in UI back to player backpack
	int nInvLocItem = ItemUpgradeGetItemSlot();
	s_ItemsRestoreToStandardLocations( pPlayer, nInvLocItem );

	nInvLocItem = ItemUnModGetResultsSlot();
	s_ItemsRestoreToStandardLocations( pPlayer, nInvLocItem );
}	