//----------------------------------------------------------------------------
// FILE: c_itemupgrade.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __C_ITEMUPGRADE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __C_ITEMUPGRADE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum UI_MSG_RETVAL;
struct UI_COMPONENT;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void c_ItemUpgradeInitForGame(
	GAME *pGame);

void c_ItemUpgradeFreeForGame(
	GAME *pGame);

void c_ItemUpgradeOpen(
	enum ITEM_UPGRADE_TYPE eUpgradeType);

void c_ItemUpgradeClose(
	enum ITEM_UPGRADE_TYPE eUpgradeType);

void c_ItemUpgradeItemUpgraded(
	UNIT *pItem);

void c_ItemAugmentItemAugmented(
	UNIT *pItem,
	int nAffixAugmented);
	
//----------------------------------------------------------------------------
// UI Callback Prototypes
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIItemUpgradeOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUpgradeUpgrade( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUpgradeCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUnModButtonOnClick( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUnModOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentCommon( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentRare( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentLegendary( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentRandom( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);
	
UI_MSG_RETVAL UIItemUnModCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUpgradeOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemAugmentOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUnModOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIItemUpgradePostActivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

#endif