//----------------------------------------------------------------------------
// weaponconfig.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "c_message.h"
#include "items.h"
#include "units.h"
#include "unittag.h"
#include "unit_priv.h"
#include "player.h"
#include "bookmarks.h"
#include "hotkeys.h"
#include "weaponconfig.h"
#include "states.h"

#if ISVERSION(SERVER_VERSION)
#include "weaponconfig.cpp.tmh"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitSetWeaponConfigChanged(
	UNIT *pUnit,
	BOOL bNeedsUpdate)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UNIT *pOwner = UnitGetUltimateOwner( pUnit );
	
	if (UnitHasClient( pOwner ))
	{
		STATS_TYPE eStat = STATS_WEAPON_CONFIG_UPDATED;
		UnitSetStat( pOwner, eStat, bNeedsUpdate );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitNeedsWeaponConfigUpdate(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	UNIT *pOwner = UnitGetUltimateOwner( pUnit );
	
	if (UnitHasClient( pOwner ))
	{
		STATS_TYPE eStat = STATS_WEAPON_CONFIG_UPDATED;
		return UnitGetStat( pOwner, eStat );
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitUpdateWeaponConfig(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	UNIT *pOwner = UnitGetUltimateOwner( pUnit );
		
	// does the weapon config need update
	if (s_UnitNeedsWeaponConfigUpdate( pOwner ) &&
		UnitCanSendMessages( pOwner ))
	{
		GAMECLIENT *pClient = UnitGetClient( pOwner );
		ASSERTX_RETURN( pClient, "Expected client" );
		
		// send keys
		HotkeysSendAll(
			pGame,
			pClient,
			pOwner,
			TAG_SELECTOR_WEAPCONFIG_HOTKEY1, 
			(TAG_SELECTOR)TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);
					
		// client now knows about the current weapon config
		s_UnitSetWeaponConfigChanged( pOwner, FALSE );
		
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WeaponConfigDoHotkey(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	MSG_CCMD_HOTKEY * msg,
	BOOL bDoNetworkUpdate,
	BOOL bIgnoreSkills /*= FALSE*/)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	int slot = msg->bHotkey;
	ASSERT_RETURN(slot >= 0 && slot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);

	UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(unit, slot);

	BOOL bEmptyTag = (msg->idItem[0] == INVALID_ID && 
		msg->idItem[1] == INVALID_ID);
		
	if (bEmptyTag && !bIgnoreSkills)
	{
		bEmptyTag = (msg->nSkillID[0] == INVALID_ID &&
					 msg->nSkillID[1] == INVALID_ID);
	}

	if (!tag && bEmptyTag)
	{
		// We're being told to clear out a hotkey, but there's not one there so s'all good.
		return;		
	}

	if (!tag && !bEmptyTag)
	{
		tag = UnitAddHotkeyTag(unit, slot);
		ASSERT_RETURN(tag);
	}

	BOOL bChange = FALSE;
	for (int i = 0; i < MAX_HOTKEY_ITEMS; ++i)
	{
		UNIT * item = UnitGetById(game, msg->idItem[i]);	
		
		// in order to put an item into a hotkey, the item must be in either
		// the secret weapon config inventory or a standard inventory slot
		if (slot >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && slot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST)
		{
			if (item && InventoryIsInAllowedHotkeySourceLocation( item ) == FALSE)
			{
				bChange = FALSE;
				break;  // forget it
			}
		}
		
		if ((msg->idItem[i] == INVALID_ID || (item && UnitGetOwnerId(item) == UnitGetId(unit))) &&
			tag->m_idItem[i] != msg->idItem[i])
		{
			tag->m_idItem[i] = msg->idItem[i];
			tag->m_nLastUnittype = (item ? UnitGetType(item) : INVALID_ID);
			bChange = TRUE;
		}	
	}

	if (!bIgnoreSkills)
	{
		for (int i = 0; i < MAX_HOTKEY_ITEMS; ++i)
		{
			if (msg->nSkillID[i] != tag->m_nSkillID[i])
			{
				bChange = TRUE;
				tag->m_nSkillID[i] = msg->nSkillID[i];
			}
		}
	}
	
	if (bChange && tag)
	{
		s_WeaponconfigUpdateSkills( unit, slot );

		// for weapon configs
		if (slot >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && slot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST)
		{
			s_UnitSetWeaponConfigChanged(unit, TRUE);
			if (bDoNetworkUpdate)
			{
				s_UnitUpdateWeaponConfig(unit);
			}
		}
		else
		{	
			HotkeySend( client, unit, (TAG_SELECTOR)slot );					
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sDebugTraceWeaponConfig(
	GAME * game, 
	UNIT * unit)
{
#if 0	//ISVERSION(DEBUG_VERSION)
	TraceDebugOnly("Weapon Configs:");
	for (int iSlot = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSlot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSlot++)
	{
		UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, iSlot);
		if (pTag)
		{
			UNIT *pItem1 = (pTag->m_idItem[0] == INVALID_ID ? NULL : UnitGetById(game, pTag->m_idItem[0]));
			UNIT *pItem2 = (pTag->m_idItem[1] == INVALID_ID ? NULL : UnitGetById(game, pTag->m_idItem[1]));
			TraceDebugOnly("  Slot %d: %d [%s] \t\t %d [%s]", iSlot - TAG_SELECTOR_WEAPCONFIG_HOTKEY1, 
				pTag->m_idItem[0], (pItem1 ? pItem1->pUnitData->szName : "NULL"), 
				pTag->m_idItem[1], (pItem2 ? pItem2->pUnitData->szName : "NULL"));
		}
		else
		{
			TraceDebugOnly("  Slot %d: NO TAG!", iSlot - TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
		}
	}

	TraceDebugOnly("Right Hand:");
	UNIT *pRightItem = UnitInventoryGetByLocation(unit, INVLOC_RHAND);
	TraceDebugOnly("    %d [%s]", (pRightItem ? pRightItem->unitid : -1), (pRightItem ? pRightItem->pUnitData->szName : "NULL"));
	TraceDebugOnly("Left Hand:");
	UNIT *pLeftItem = UnitInventoryGetByLocation(unit, INVLOC_LHAND);
	TraceDebugOnly("    %d [%s]", (pLeftItem ? pLeftItem->unitid : -1), (pLeftItem ? pLeftItem->pUnitData->szName : "NULL"));

	TraceDebugOnly("Secret Inventory:");
	UNIT* pTestItem = NULL;
	while ((pTestItem = UnitInventoryLocationIterate(unit, INVLOC_WEAPONCONFIG, pTestItem)) != NULL)
	{
		TraceDebugOnly("    %d [%s]", pTestItem->unitid, pTestItem->pUnitData->szName);
	}
	TraceDebugOnly("\n");

	// let's just make sure we didn't accidentally leave anyone behind
	UNIT* pTestItem = NULL;
	while ((pTestItem = UnitInventoryLocationIterate(unit, INVLOC_WEAPONCONFIG, pTestItem)) != NULL)
	{
		if (UnitTestFlag(pTestItem, UNITFLAG_TEMP_WEAPONCONFIG))
		{
			continue;
		}

		BOOL bFoundIt = FALSE;
		for (int iSlot = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSlot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSlot++)
		{
			UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, iSlot);
			if (pTag)
			{
				for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
				{
					if (UnitGetId(pTestItem) == pTag->m_idItem[i])
					{
						bFoundIt = TRUE;
						break;
					}
				}
			}
		}
		ASSERTX_RETFALSE(bFoundIt, "An item was left behind in the secret inventory!");
	}
#else
	REF(unit);
	REF(game);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WeaponConfigAdd(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	MSG_CCMD_ADD_WEAPONCONFIG* msg )
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	UNIT *pSwapItem = NULL;
	int slot = msg->bHotkey;
	UNITLOG_ASSERT_RETURN(slot >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && slot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST, unit);
	UNITLOG_ASSERT_RETURN(msg->idItem != INVALID_ID, unit);
	UNITLOG_ASSERT_RETURN(msg->bSuggestedPos < MAX_HOTKEY_ITEMS, unit);
	UNIT *pItem = UnitGetById(game, msg->idItem);
	UNITLOG_ASSERT_RETURN(pItem, unit);
	UNITLOG_ASSERT_RETURN(UnitGetUltimateContainer( pItem ) == unit, unit);
	MSG_CCMD_HOTKEY HotkeyMessage;

	TraceDebugOnly("\n\nWeaponConfig Msg %d [%s] to %d:%d from config %d\n", msg->idItem, pItem->pUnitData->szName, 
		slot-TAG_SELECTOR_WEAPCONFIG_HOTKEY1, msg->bSuggestedPos, msg->nFromWeaponConfig-TAG_SELECTOR_WEAPCONFIG_HOTKEY1);

	if (ItemBelongsToAnyMerchant(pItem))
	{
		goto _failout;
	}

	int nCurrentConfig = UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG) + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;

	// Here's a little trick.  If it's coming from your right or left hand, count it as coming from the currently selected weapon config, cause technically it is
	int nFromLocation = INVLOC_NONE;
	if (UnitGetInventoryLocation(pItem, &nFromLocation) &&
		(GetWeaponconfigCorrespondingInvLoc(0) == nFromLocation ||
		 GetWeaponconfigCorrespondingInvLoc(1) == nFromLocation))
	{
		msg->nFromWeaponConfig = nCurrentConfig;
	}
	// check that it's in a takeable location.
	if (nFromLocation != INVLOC_NONE &&
		InventoryLocPlayerCanTake(UnitGetContainer(pItem), pItem, nFromLocation) == FALSE)
	{
		UNITLOG_ASSERT_RETURN(FALSE, unit);
	}

	// These work specially with items in the inventory so we need to do some validation and moving

	// this is for the eventual (if everything passes muster) change to the hotkey
	HotkeyMessage.bHotkey = msg->bHotkey;

	// First make sure we're even allowed in that location
	if (!IsAllowedUnitInv(pItem, GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos), unit ))
	{
		if (msg->bSuggestedPos == 1 &&
			IsAllowedUnitInv(pItem, GetWeaponconfigCorrespondingInvLoc(0), unit ))		// check the other slot too
		{
			msg->bSuggestedPos = 0;
		}
		else
		{
			goto _failout;
		}
	}

	// the item must not be in an inventory slot that is considered "on the player", 
	// which we consider to be either the curor or any standard inventory location
	//if (InventoryIsInLocationType( pItem, LT_STANDARD ) == FALSE && ItemIsInCursor( pItem ) == FALSE)
	//{
	//	goto _failout;
	//}
			
	// now ladies and germs, for our first trick, we will see if this item is already in a weapon config
	BOOL bAlreadyInAWeaponConfig = FALSE;
	for (int iSlot = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSlot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSlot++)
	{
		UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, iSlot);
		if (pTag)
		{
			for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
			{
				if (msg->idItem == pTag->m_idItem[i])
				{
					if (iSlot == slot)
					{
						if (i != msg->bSuggestedPos &&
							!UnitIsA(pItem, UNITTYPE_TWOHAND))
						{
							// First make sure the other item is allowed in that location
							UNIT *pOtherItem = UnitGetById(game, pTag->m_idItem[msg->bSuggestedPos]);
							if (pOtherItem && !IsAllowedUnitInv(pOtherItem, GetWeaponconfigCorrespondingInvLoc(!msg->bSuggestedPos), unit ))
							{
								goto _failout;
							}

							// we can just swap the hotkey positions
							HotkeyMessage.idItem[0] = pTag->m_idItem[1];
							HotkeyMessage.idItem[1] = pTag->m_idItem[0];

							// if we swapping inside the currently selected config, the items are equipped, so swap hands
							if (iSlot - TAG_SELECTOR_WEAPCONFIG_HOTKEY1 == UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG))
							{
								// if the other hand is empty, just move the item
								if (!pOtherItem)
								{
									int nInvLoc = GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos);
									//if (!UnitInventoryAdd(unit, pItem, GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos)))
									if (InventoryChangeLocation(unit, pItem, nInvLoc, -1, -1, 0))
									{
										return;
									}
								}
								else
								{
									if (!InventorySwapLocation( pItem, pOtherItem ))
									{
										ASSERTX_RETURN(FALSE, "Weaponconfig inventory swap failed");
										goto _failout;
									}
								}
							}

							s_WeaponConfigDoHotkey(game, client, unit, &HotkeyMessage, TRUE, TRUE);
							return;
						}

						// Dammit Jim, you can't put it where it already is!
						goto _failout;
					}
					else
					{
						bAlreadyInAWeaponConfig = TRUE;
					}
				}
			}
		}
	}

	//if (bAlreadyInAWeaponConfig != (msg->nFromWeaponConfig != -1))
	//{
	//	// The client told us this was in a weaponconfig but the client is a goddamn liar!
	//	return;		// screw this.  I don't have to take this from you people.
	//}

	// Ok, so we've agreed to add it to the weaponconfig.  Now, are we gonna hafta pop someone else out?
	UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, slot);
	if (!pTag)
	{
		pTag = UnitAddHotkeyTag(unit, slot);
	}
	ASSERT_RETURN(pTag);

	UNIT *pWCItem[MAX_HOTKEY_ITEMS];
	pWCItem[msg->bSuggestedPos] = (pTag->m_idItem[msg->bSuggestedPos] == INVALID_ID ? NULL : UnitGetById(game, pTag->m_idItem[msg->bSuggestedPos]));
	pWCItem[!msg->bSuggestedPos] = (pTag->m_idItem[!msg->bSuggestedPos] == INVALID_ID ? NULL : UnitGetById(game, pTag->m_idItem[!msg->bSuggestedPos]));

	BOOL bPut = FALSE;

	if (UnitIsA(pItem, UNITTYPE_TWOHAND))	//two-hand items go in the right hand
	{
		msg->bSuggestedPos = 0;
	}

	if (pTag->m_idItem[msg->bSuggestedPos] == INVALID_ID &&
		pTag->m_idItem[!msg->bSuggestedPos] == INVALID_ID)
	{
		// it's empty.  Just go!
		bPut = TRUE;
	}
	else if (!UnitIsA(pItem, UNITTYPE_TWOHAND) &&
		pTag->m_idItem[msg->bSuggestedPos] == INVALID_ID)
	{
		if (pTag->m_idItem[!msg->bSuggestedPos] == INVALID_ID ||
			!UnitIsA(pWCItem[!msg->bSuggestedPos], UNITTYPE_TWOHAND))
		{
			// the one slot we need is open and the other slot is one or nothing.  Go!
			bPut = TRUE;
		}
		else
		{
			// we need to swap with the other item.
			pSwapItem = pWCItem[!msg->bSuggestedPos];
		}
	}
	else if (!UnitIsA(pItem, UNITTYPE_TWOHAND) &&
		pTag->m_idItem[msg->bSuggestedPos] != INVALID_ID)
	{
		// the one slot we need is occupied.  Swap with that one item.
		pSwapItem = pWCItem[msg->bSuggestedPos];
	}
	else if (UnitIsA(pItem, UNITTYPE_TWOHAND) &&
		pTag->m_idItem[msg->bSuggestedPos] != INVALID_ID &&
		pTag->m_idItem[!msg->bSuggestedPos] != INVALID_ID)
	{
		//ok, this can be some complex shit.  Let's see if we can just punt on this case
		trace("WeaponConfig both slots full!\n");
		goto _failout;
	}
	else if (UnitIsA(pItem, UNITTYPE_TWOHAND) &&
		pTag->m_idItem[msg->bSuggestedPos] != INVALID_ID ||
		pTag->m_idItem[!msg->bSuggestedPos] != INVALID_ID)
	{
		// there's one items in this weaponconfig and we need both spaces... swap with it.
		if (pTag->m_idItem[msg->bSuggestedPos] != INVALID_ID)
			pSwapItem = pWCItem[msg->bSuggestedPos];
		else
			pSwapItem = pWCItem[!msg->bSuggestedPos];
	}
	else 
	{
		// what else is there?
		ASSERTX_RETURN(FALSE, "Unexpected scenario in weapon configs");
	}

	ASSERTX_RETURN(pSwapItem || bPut, "Unexpected scenario in weapon configs");


	if (pSwapItem)
	{
		BOOL bBreakOut = FALSE;
		for (int iSlot = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSlot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST && !bBreakOut; iSlot++)
		{
			UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, iSlot);
			if (pTag)
			{
				for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
				{
					if (UnitGetId(pSwapItem) == pTag->m_idItem[i])
					{
						if (iSlot == msg->bHotkey)			// if it's the hotkey we're currently on, we have to swap.  Don't allow it to do a put
						{
							bPut = FALSE;
							bBreakOut = TRUE;
							break;
						}
						else
						{
							bPut = TRUE;					// the item we're placing on is in another weaponconfig.  Let's just replace that one instance with the new item instead of trying to swap them.
						}
					}
				}
			}
		}
	}

	if (bPut)
	{

// BSP TODO			need to do some requirements checking

		int nConfig = slot - TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
		if (!bAlreadyInAWeaponConfig ||	// if it's already in another weapon config then it's already in the proper inventory location so we don't need to move it
			nConfig == UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG))
		{

			int nConfig = slot - TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
			if (nConfig == UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG))
			{
				int nLocation = GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos);

				// Since this is the current config we have to do a requirements check because it's going to go into your hands
				if (!InventoryItemCanBeEquipped(unit, pItem, nLocation))
				{
					goto _failout;
				}

				if (!InventoryChangeLocation(unit, pItem, nLocation, -1, -1, CLF_CONFIG_SWAP_BIT))
				{
					goto _failout;
				}
				
			}
			else
			{
				int x, y;
				if (!UnitInventoryFindSpace(unit, pItem, INVLOC_WEAPONCONFIG, &x, &y))
				{
					goto _failout;
				}
				//if (!InventoryTestPut(unit, pItem, INVLOC_WEAPONCONFIG, x, y))	//double-check
				//{
				//	goto _failout;
				//}
				//if (!UnitInventoryAdd(unit, pItem, INVLOC_WEAPONCONFIG, x, y))
				//{
				//	goto _failout;
				//}
				if (!InventoryChangeLocation(unit, pItem, INVLOC_WEAPONCONFIG, x, y, 0))
				{
					goto _failout;
				}
			}
		}
	}
	else
	{

// BSP TODO			need to do some requirements checking


		UNIT_TAG_HOTKEY* pSwapSourceTag = NULL;
		int nPos = -1;
		UNITID idOldSourceItems[MAX_HOTKEY_ITEMS];
		if (bAlreadyInAWeaponConfig && msg->nFromWeaponConfig >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && msg->nFromWeaponConfig <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST)
		{
			// if we're moving from one weaponconfig to another, we need to update the other config too
			// so find out the position we're going to be swapping the alias to
			//ASSERT_RETURN(msg->nFromWeaponConfig >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && msg->nFromWeaponConfig <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);
			pSwapSourceTag = UnitGetHotkeyTag(unit, msg->nFromWeaponConfig);
			ASSERT_RETURN(pSwapSourceTag);

			for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
			{
				idOldSourceItems[i] = pSwapSourceTag->m_idItem[i];		// we need to save these off in case the invswap takes them out of the tag
				if (pSwapSourceTag->m_idItem[i] == msg->idItem)
				{
					nPos = i;
				}
			}
		}

		if (slot == nCurrentConfig)
		{
			// Since this is the current config we have to do a requirements check because it's going to go into your hands
			int nLocation = GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos);
			if (!InventoryItemCanBeEquipped(unit, pItem, pSwapItem, NULL, nLocation))
			{
				goto _failout;
			}
		}

		int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
		int nItemLoc = INVLOC_NONE;
		UnitGetInventoryLocation(pItem, &nItemLoc);

		BOOL bItemPhysicallyInCursor = (nItemLoc == nCursorLocation);
		BOOL bPhysicalSwap = FALSE;
		BOOL bPhysicalPut = FALSE;
		BOOL bRemoveCurrentFromCursor = FALSE;
		BOOL bPutSwapItemInSecretInventory = FALSE;
		BOOL bAddCursorReference = FALSE;
		BOOL bAddCursorPhysical = FALSE;
		BOOL bSwapInMulitpleWC = ItemInMultipleWeaponConfigs(pSwapItem);
		BOOL bItemInMulitpleWC = ItemInMultipleWeaponConfigs(pItem);
		if (bSwapInMulitpleWC)
		{
			bAddCursorReference = TRUE;
		}
		else
		{
			bAddCursorReference = FALSE;
		}
		
		if (bItemPhysicallyInCursor)
		{
			if (pSwapItem != pWCItem[msg->bSuggestedPos])		// we're swapping with the other position in the WC
			{
				bPhysicalPut = TRUE;			// put the cursor item either in secret inventory or a hand
				if (bSwapInMulitpleWC)
				{
					if (slot==nCurrentConfig)
					{					
						bPutSwapItemInSecretInventory = TRUE;
					}
					bAddCursorReference = TRUE;
				}
				else
				{
					bRemoveCurrentFromCursor = TRUE;
					bAddCursorPhysical = TRUE;
				}

			}
			else
			{
				if (bSwapInMulitpleWC)
				{
					bPhysicalPut = TRUE;
					if (slot == nCurrentConfig)
					{
						bPutSwapItemInSecretInventory = TRUE;
					}
				}
				else
				{
					bPhysicalSwap = TRUE;
				}
			}
		}
		else
		{
			if (!bAlreadyInAWeaponConfig)
			{
				// the incoming item is not in the cursor and not in the weaponconfigs
				bPhysicalPut = TRUE;	// the incoming item must be put physically in the WCs
				if (bSwapInMulitpleWC)
				{
					if (slot==nCurrentConfig)
					{
						// swap item must be put back in the secret inventory
						bPutSwapItemInSecretInventory = TRUE;
					}
					else
					{
						// just let it get replaced
					}
				}
				else
				{
					// put the swap item in the cursor
					bAddCursorPhysical = TRUE;
				}
			}
			else
			if (slot==nCurrentConfig)
			{
				// this is the current config and involves the hands so something physical has to happen
				if (bSwapInMulitpleWC)
				{
					// candidate needs to be put in the hand, and the swap item is in another WC so put it back in secret inventory
					bPhysicalSwap = TRUE;
				}
				else
				{
					bPhysicalPut = TRUE;
				}
			}
		}

		if (!bPhysicalSwap && !bSwapInMulitpleWC)
		{
			bAddCursorPhysical = TRUE;
		}
		ASSERT_RETURN(!(bPhysicalPut && bPhysicalSwap));
//		ASSERT_RETURN(bItemInMulitpleWC || bPhysicalPut || bPhysicalSwap);

#if !ISVERSION(RETAIL_VERSION)
		{
			char str[256]; 
			PStrPrintf(str, arrsize(str), "  bItemPhysicallyInCursor=%d\n  bPhysicalSwap=%d\n  bPhysicalPut=%d\n  bAddCursorReference=%d\n  bAddCursorPhysical=%d\n  bSwapInMulitpleWC=%d\n  bItemInMulitpleWC=%d\n", bItemPhysicallyInCursor, bPhysicalSwap, bPhysicalPut, bAddCursorReference, bAddCursorPhysical, bSwapInMulitpleWC, bItemInMulitpleWC);
			trace(str);
		}
#endif

		if (bRemoveCurrentFromCursor)
		{
			// move it to the secret inventory temporarily
			if (!InventoryChangeLocation(unit, pItem, INVLOC_WEAPONCONFIG, -1, -1, CLF_CONFIG_SWAP_BIT))
			{
				goto _failout;
			}
		}

		if (bAddCursorPhysical)
		{
			if (!InventoryChangeLocation(unit, pSwapItem, nCursorLocation, -1, -1, CLF_CONFIG_SWAP_BIT))
			{
				goto _failout;
			}
		}

		if (bPhysicalSwap)
		{
			// swap the actual items
			sDebugTraceWeaponConfig(game, unit);
			if (!InventorySwapLocation( pItem, pSwapItem, -1, -1, CLF_CONFIG_SWAP_BIT ))
			{
				//ASSERTX_RETURN(FALSE, "Weaponconfig inventory swap failed");

				// *beep*
				goto _failout;
			}
		}
		else 
		{
			if (bPutSwapItemInSecretInventory)
			{
				if (!InventoryChangeLocation(unit, pSwapItem, INVLOC_WEAPONCONFIG, -1, -1, CLF_CONFIG_SWAP_BIT))
				{
					goto _failout;
				}
			}
			if (bPhysicalPut)
			{
				int nLocation = INVLOC_WEAPONCONFIG;
				if (slot == nCurrentConfig)
					nLocation = GetWeaponconfigCorrespondingInvLoc(msg->bSuggestedPos);

				if (!InventoryChangeLocation(unit, pItem, nLocation, -1, -1, CLF_CONFIG_SWAP_BIT))
				{
					goto _failout;
				}
			}
		}

		if (bAddCursorReference)
		{
			// tell the client to put a reference to the swapped item in the cursor
			MSG_SCMD_CURSOR_ITEM_REFERENCE msg;
			msg.nFromWeaponConfig = -1;
			msg.idItem = UnitGetId(pSwapItem);

			s_SendMessage(game, ClientGetId(client), SCMD_CURSOR_ITEM_REFERENCE, &msg);
		}
	}

	for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
	{
		// start with what was already in the WC
		HotkeyMessage.idItem[i] = pTag->m_idItem[i];
	}

	// if the item we're putting is swapped with an item from the other slot, clear that slot from the tag.
	if (!bPut &&
		pSwapItem != pWCItem[msg->bSuggestedPos])
	{
		HotkeyMessage.idItem[!msg->bSuggestedPos] = INVALID_ID;
	}

	HotkeyMessage.idItem[msg->bSuggestedPos] = msg->idItem;			// put the alias to the item where it wants to be

	s_WeaponConfigDoHotkey(game, client, unit, &HotkeyMessage, TRUE, TRUE);
		
	sDebugTraceWeaponConfig(game, unit);

	return;

_failout:

	s_InventoryClearWaitingForMove(game, client, pItem);
	if (pSwapItem)
		s_InventoryClearWaitingForMove(game, client, pSwapItem);

	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSwapWeaponConfigs(
	GAME * game,
	UNIT * unit,
	int nConfigLeaving, 
	int nConfigComing)
{
	int nMovedToSecret = 0;
	int nMovedToEquip = 0;

	if(UnitHasState(game, unit, STATE_CANNOT_CHANGE_WEAPONS))
		return FALSE;
	TraceDebugOnly("- Swapping Weaponconfigs from %d to %d\n\n", nConfigLeaving, nConfigComing);

	// Take the previous items out of the hands and put them in the hidden inventory
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(unit, nConfigLeaving + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	if (pTag)
	{
		for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
		{
			if (pTag->m_idItem[i] != INVALID_ID)
			{
				UNIT *item = UnitGetById(game, pTag->m_idItem[i]);
				UNITLOG_ASSERT_RETFALSE(item, unit);
				int x, y;
				if (!UnitInventoryFindSpace(unit, item, INVLOC_WEAPONCONFIG, &x, &y))
				{
					goto _failout;
				}
				if (!InventoryTestPut(unit, item, INVLOC_WEAPONCONFIG, x, y, 0))	//double-check
				{
					UNITLOG_ASSERT(FALSE, unit);
					goto _failout;
				}				
				if (!InventoryChangeLocation(unit, item, INVLOC_WEAPONCONFIG, x, y, CLF_CONFIG_SWAP_BIT))
				{
					goto _failout;
				}
			}
			nMovedToSecret++;
		}
	}

	// Now put the new items in the hands
	pTag = UnitGetHotkeyTag(unit, nConfigComing + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	if (!pTag)
	{
		pTag = UnitAddHotkeyTag(unit, nConfigComing + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	}
	ASSERT_RETFALSE(pTag);

	for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
	{
		if (pTag->m_idItem[i] != INVALID_ID)
		{
			UNIT *item = UnitGetById(game, pTag->m_idItem[i]);
			ASSERT_RETFALSE(item);

			int nLoc = GetWeaponconfigCorrespondingInvLoc(i);			
			
			//if (!InventoryTestPut(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT) ||		// <-- we should remove this check once we can have items in equip slots that are unusable
			//	!InventoryChangeLocation(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT))			
			if (!InventoryChangeLocation(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT))			
			{
				goto _failout;
			}
		}
		nMovedToEquip++;
	}

	sDebugTraceWeaponConfig(game, unit);

	return TRUE;

_failout:
	// try to put things back where they came from
	// this should be the reverse of the above

	pTag = UnitGetHotkeyTag(unit, nConfigComing + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	if (!pTag)
	{
		pTag = UnitAddHotkeyTag(unit, nConfigComing + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	}
	ASSERT_RETFALSE(pTag);

	// move the items that were put in hands back in secret inventory
	for (int i=0; i < nMovedToEquip; i++)
	{
		if (pTag->m_idItem[i] != INVALID_ID)
		{
			UNIT *item = UnitGetById(game, pTag->m_idItem[i]);
			UNITLOG_ASSERT_RETFALSE(item, unit);
			int x, y;
			if (!UnitInventoryFindSpace(unit, item, INVLOC_WEAPONCONFIG, &x, &y))
			{
				ASSERT_RETFALSE(FALSE);	// ok we're in a really bad state here.  Bail. 
			}
			if (!InventoryTestPut(unit, item, INVLOC_WEAPONCONFIG, x, y, 0))	
			{
				UNITLOG_ASSERT(FALSE, unit);
				ASSERT_RETFALSE(FALSE);	// ok we're in a really bad state here.  Bail. 
			}				
			if (!InventoryChangeLocation(unit, item, INVLOC_WEAPONCONFIG, x, y, CLF_CONFIG_SWAP_BIT))
			{
				ASSERT_RETFALSE(FALSE);	// ok we're in a really bad state here.  Bail. 
			}
		}
	}

	// move the items that were put in secret inventory back in the hands
	pTag = UnitGetHotkeyTag(unit, nConfigLeaving + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	ASSERT_RETFALSE(pTag);

	for (int i=0; i < nMovedToSecret; i++)
	{
		if (pTag->m_idItem[i] != INVALID_ID)
		{
			UNIT *item = UnitGetById(game, pTag->m_idItem[i]);
			ASSERT_RETFALSE(item);

			int nLoc = GetWeaponconfigCorrespondingInvLoc(i);			
			
			//if (!InventoryTestPut(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT) ||		// <-- we should remove this check once we can have items in equip slots that are unusable
			//	!InventoryChangeLocation(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT))			
			if (!InventoryChangeLocation(unit, item, nLoc, 0, 0, CLF_CONFIG_SWAP_BIT))			
			{
				ASSERT_RETFALSE(FALSE);	// ok we're in a really bad state here.  Bail. 
			}
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WeaponConfigSelect(
	GAME * game, 
	UNIT * unit,
	MSG_CCMD_SELECT_WEAPONCONFIG * msg,
	BOOL bFromClient )
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN( ! bFromClient || !IsUnitDeadOrDying(unit));

	ASSERT_RETURN(msg->bHotkey >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && msg->bHotkey <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);

	if ( bFromClient && msg->bHotkey > TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST_SET_BY_USER )
		return; // the client can't ask to switch to this hotkey

	int nConfig = msg->bHotkey - TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
	int nCurrentConfig = UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG);
	// int nCurrentHotkey = nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
	if (nConfig == nCurrentConfig)
	{
		return;
	}

	// check to see if we're able to equip the config we're switching to.  If not,
	//   We don't want to allow it.

	if (!WeaponConfigCanSwapTo(game, unit, nConfig))
	{
		return;
	}

	if (sSwapWeaponConfigs(game, unit, nCurrentConfig, nConfig))
	{
		int nPreviousConfig = UnitGetStat( unit, STATS_CURRENT_WEAPONCONFIG );
		UNIT_TAG_HOTKEY *pSwitchToTag = UnitGetHotkeyTag(unit, nConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
		ASSERT_RETURN(pSwitchToTag);
		
		UnitSetStat( unit, STATS_PREVIOUS_WEAPONCONFIG, nPreviousConfig );
		UnitSetStat( unit, STATS_CURRENT_WEAPONCONFIG, nConfig);
		UnitSetStat( unit, STATS_SKILL_LEFT, pSwitchToTag->m_nSkillID[0] );
		UnitSetStat( unit, STATS_SKILL_RIGHT, pSwitchToTag->m_nSkillID[1] );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WeaponConfigInitialize(
	GAME * pGame, 
	UNIT * pUnit )
{
	for (int iSelector = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSelector <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSelector++)
	{
		UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pUnit, iSelector);
		if (pTag)
		{
			for (int i=0; i<MAX_HOTKEY_ITEMS; i++)
			{
				if (pTag->m_idItem[i] != INVALID_ID)
				{
					UNIT *pItem = UnitGetById(pGame, pTag->m_idItem[i]);
					if (!pItem || UnitGetOwnerId(pItem) != UnitGetId(pUnit))
					{
						pTag->m_idItem[i] = INVALID_ID;
					}
				}
			}
			if ( pTag->m_nSkillID[0] == INVALID_ID )
				pTag->m_nSkillID[0] = 0;
			if ( pTag->m_nSkillID[1] == INVALID_ID )
				pTag->m_nSkillID[1] = AppIsTugboat() ? 0 : 1;
		}
	}

	int nCurrentConfig = UnitGetStat( pUnit, STATS_CURRENT_WEAPONCONFIG );
	
	if ( nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 > TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST_SET_BY_USER )
	{
		int nPreviousConfig = UnitGetStat( pUnit, STATS_PREVIOUS_WEAPONCONFIG );
		if ( nPreviousConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 > TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST_SET_BY_USER )
			nPreviousConfig = 0;
		nPreviousConfig = max( 0, nPreviousConfig );

		MSG_CCMD_SELECT_WEAPONCONFIG tMsg;
		tMsg.bHotkey = (BYTE)(TAG_SELECTOR_WEAPCONFIG_HOTKEY1 + nPreviousConfig);
		s_WeaponConfigSelect( pGame, pUnit, &tMsg, FALSE );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemInMultipleWeaponConfigs(
	UNIT *pItem)
{
	ASSERT_RETFALSE(pItem);
	UNIT *pContainer = UnitGetUltimateContainer(pItem);
	ASSERT_RETFALSE(pContainer);
	ASSERT_RETFALSE(UnitIsA(pContainer, UNITTYPE_PLAYER));

	UNITID idItem = UnitGetId(pItem);
	int nCount = 0;
	for (int iSelector = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSelector <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSelector++)
	{
		UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pContainer, iSelector);
		if (pTag)
		{
			for (int i=0; i<MAX_HOTKEY_ITEMS; i++)
			{
				if (pTag->m_idItem[i] == idItem)
				{
					nCount++;
				}
			}
		}
	}

	return nCount > 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WeaponConfigCanSwapTo(
	GAME * game, 
	UNIT * unit,
	int nConfig )
{
	ASSERT_RETFALSE(unit);

	ASSERT_RETFALSE(nConfig >= 0 && nConfig <= (TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST - TAG_SELECTOR_WEAPCONFIG_HOTKEY1));

// we're now allowing you to switch to weapons you can no longer equip with the
//	new in-equip-slot-but-not-equipped system

	//int nCurrentConfig = UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG);
	//if (nConfig == nCurrentConfig)
	//{
	//	return TRUE;
	//}

	//UNIT_TAG_HOTKEY *pSwitchToTag = UnitGetHotkeyTag(unit, nConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	//ASSERT_RETFALSE(pSwitchToTag);
	//
	//// check to see if we're able to equip the config we're switching to.  If not,
	////   We don't want to allow it.
	//if (pSwitchToTag->m_idItem[0] != INVALID_ID || pSwitchToTag->m_idItem[1] != INVALID_ID)
	//{
	//	UNIT *pSwitchToItem1 = (pSwitchToTag->m_idItem[0] != INVALID_ID ? UnitGetById(game, pSwitchToTag->m_idItem[0]) : NULL);
	//	UNIT *pSwitchToItem2 = (pSwitchToTag->m_idItem[1] != INVALID_ID ? UnitGetById(game, pSwitchToTag->m_idItem[1]) : NULL);

	//	UNIT_TAG_HOTKEY *pCurrentTag = UnitGetHotkeyTag(unit, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	//	ASSERT_RETFALSE(pCurrentTag);
	//	
	//	UNIT *pCurrentItem1 = (pCurrentTag->m_idItem[0] != INVALID_ID ? UnitGetById(game, pCurrentTag->m_idItem[0]) : NULL);
	//	UNIT *pCurrentItem2 = (pCurrentTag->m_idItem[1] != INVALID_ID ? UnitGetById(game, pCurrentTag->m_idItem[1]) : NULL);

	//	if (pSwitchToItem1 &&
	//		!InventoryItemCanBeEquipped(unit, pSwitchToItem1, pCurrentItem1, pCurrentItem2, GetWeaponconfigCorrespondingInvLoc(0)))
	//	{
	//		return FALSE;
	//	}
	//	if (pSwitchToItem2 &&
	//		!InventoryItemCanBeEquipped(unit, pSwitchToItem2, pCurrentItem1, pCurrentItem2, GetWeaponconfigCorrespondingInvLoc(1)))
	//	{
	//		return FALSE;
	//	}
	//}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WeaponConfigRemoveItemReference(
	UNIT * pUnit,
	UNITID idItem,
	int nConfig)
{
	ASSERT_RETFALSE(pUnit);
	ASSERT_RETFALSE (nConfig >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && nConfig <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);

	GAME *pGame = UnitGetGame(pUnit);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	UNIT *pItem = UnitGetById(pGame, idItem);
	ASSERT_RETFALSE(pItem);

	if (!ItemInMultipleWeaponConfigs(pItem))
	{
		return FALSE;
	}

	int nCurrentConfig = UnitGetStat(pUnit, STATS_CURRENT_WEAPONCONFIG) + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;

	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pUnit, nConfig);
	if (pTag)
	{
		for (int i=0; i<MAX_HOTKEY_ITEMS; i++)
		{
			if (pTag->m_idItem[i] == idItem)
			{
				if (nConfig == nCurrentConfig)
				{
					// this is the current config, so we need to move the item back into the secret inventory.
					if (!InventoryChangeLocation(pUnit, pItem, INVLOC_WEAPONCONFIG, -1, -1, CLF_CONFIG_SWAP_BIT))
					{
						ASSERTX_RETFALSE(FALSE, "Unexpected failure trying to move a weapon to the weaponconfig secret inventory.");
					}
				}

				pTag->m_idItem[i] = INVALID_ID;
				s_WeaponconfigUpdateSkills( pUnit, nConfig );

				s_UnitSetWeaponConfigChanged(pUnit, TRUE);
				s_UnitUpdateWeaponConfig(pUnit);

				return TRUE;
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GetWeaponconfigCorrespondingInvLoc(
	int index) 
{
	return (index == 0 ? INVLOC_RHAND : INVLOC_LHAND);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsItemInWeaponconfig(
	UNIT * unit,
	UNITID idItem,
	int nExcludeSelector /*= -1*/) 
{
	for (int iSlot = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSlot <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSlot++)
	{
		if (iSlot != nExcludeSelector)
		{
			UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, iSlot);
			if (pTag)
			{
				for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
				{
					if (idItem == pTag->m_idItem[i])
					{
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WeaponconfigSetItem(
	UNIT * unit,
	UNITID idItem,
	int selector,
	int slot)
{
	ASSERT_RETFALSE(unit);
	GAME *game = UnitGetGame(unit);
	ASSERT_RETFALSE(IS_SERVER(game));
	ASSERT_RETFALSE(selector >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1 && selector <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);
	ASSERT_RETFALSE(slot >= 0 && slot < MAX_HOTKEY_ITEMS);

	UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, selector);
	if (!pTag)
	{
		pTag = UnitAddHotkeyTag(unit, selector);
	}
	ASSERT_RETFALSE(pTag);

	ASSERT_RETFALSE(pTag->m_idItem[slot] == INVALID_ID);
	pTag->m_idItem[slot] = idItem;

	s_WeaponconfigUpdateSkills( unit, selector );

	s_UnitSetWeaponConfigChanged( unit, TRUE );
	
	sDebugTraceWeaponConfig(game, unit);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WeaponconfigUpdateSkills(
	UNIT * unit,
	int selector)
{
	ASSERT_RETURN(unit);
	GAME *game = UnitGetGame(unit);
	ASSERT_RETURN(IS_SERVER(game));

	UNIT_TAG_HOTKEY* pTag = UnitGetHotkeyTag(unit, selector);
	if ( ! pTag )
		return;

	ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
	int pnSkills[ 2 ] = { pTag->m_nSkillID[0], pTag->m_nSkillID[1] };
	//UnitGetPreferedSkills( unit, pTag->m_idItem, pnSkills );
	pTag->m_nSkillID[0] = pnSkills[ 0 ];
	pTag->m_nSkillID[1] = pnSkills[ 1 ];

	int nCurrentConfig = UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG);
	int nCurrentHotkey = nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;

	if ( selector == nCurrentHotkey )
	{
		UnitSetStat(unit, STATS_SKILL_LEFT,  pTag->m_nSkillID[0]);
		UnitSetStat(unit, STATS_SKILL_RIGHT, pTag->m_nSkillID[1]);
	}
}

