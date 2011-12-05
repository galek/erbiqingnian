//----------------------------------------------------------------------------
// s_network.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_WEAPONCONFIG_H_
#define _WEAPONCONFIG_H_

void s_UnitSetWeaponConfigChanged(
	UNIT *pUnit,
	BOOL bNeedsUpdate);
		
BOOL s_UnitNeedsWeaponConfigUpdate(
	UNIT *pUnit);

void s_UnitUpdateWeaponConfig(
	UNIT *pUnit);

void s_WeaponConfigDoHotkey(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	struct MSG_CCMD_HOTKEY * msg,
	BOOL bDoNetworkUpdate,
	BOOL bIgnoreSkills = FALSE);

void s_WeaponConfigAdd(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	struct MSG_CCMD_ADD_WEAPONCONFIG* msg );

void s_WeaponConfigSelect(
	GAME * game, 
	UNIT * unit,
	struct MSG_CCMD_SELECT_WEAPONCONFIG * msg,
	BOOL bFromClient = TRUE );

void s_WeaponConfigInitialize(
	GAME * game, 
	UNIT * unit );

BOOL ItemInMultipleWeaponConfigs(
	UNIT *pItem);

BOOL WeaponConfigCanSwapTo(
	GAME * game, 
	UNIT * unit,
	int nConfig );

BOOL WeaponConfigRemoveItemReference(
	UNIT * pUnit,
	UNITID idItem,
	int nConfig);

BOOL IsItemInWeaponconfig(
	UNIT * unit,
	UNITID idItem,
	int nExcludeSelector = -1);

BOOL s_WeaponconfigSetItem(
	UNIT * unit,
	UNITID idItem,
	int selector,
	int slot);

void s_WeaponconfigUpdateSkills(
	UNIT * unit,
	int selector);

int GetWeaponconfigCorrespondingInvLoc(
	int index);

#endif // _WEAPONCONFIG_H_
