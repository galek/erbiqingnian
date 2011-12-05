//----------------------------------------------------------------------------
// FILE: c_itemupgrade.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_itemupgrade.h"
#include "c_message.h"
#include "inventory.h"
#include "itemupgrade.h"
#include "items.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_components_complex.h"
#include "uix_money.h"
#include "uix_scheduler.h"
#include "s_message.h"
#include "fontcolor.h"
#include "Currency.h"
#include "stringreplacement.h"
#include "language.h"
#include "c_font.h"

//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Forward References
//----------------------------------------------------------------------------
static void sSetAugmentStatus( const WCHAR *puszStatus, BOOL bDoReset );
static void sSetUpgradeStatus( const WCHAR *puszStatus, BOOL bDoReset );
static void sUpdateAugmentUI( void );
static void sUpdateUpgradeUI( void );
static void sUpdateUnModUI( void );

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct ITEM_UPGRADE_GLOBALS
{
	UNITID idResources[ MAX_ITEM_UPGRADE_RESOURCES ];
	DWORD dwAugmentStatusResetEvent;
	DWORD dwUpgradeStatusResetEvent;
	DWORD dwUnModStatusResetEvent;
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static const int STATUS_CLEAR_DELAY_IN_MS = 2000;
static ITEM_UPGRADE_GLOBALS tUpgradeGlobals;
static const UI_COMPONENT_ENUM sgeResourceCompLookup[] = 
{
	UICOMP_ITEM_UPGRADE_RESOURCE_1,
	UICOMP_ITEM_UPGRADE_RESOURCE_2,
	UICOMP_ITEM_UPGRADE_RESOURCE_3,
	UICOMP_ITEM_UPGRADE_RESOURCE_4,
	UICOMP_ITEM_UPGRADE_RESOURCE_5,
	UICOMP_INVALID
};
const int sgnNumResourceComponents = arrsize( sgeResourceCompLookup ) - 1;

typedef void (*PFN_UPDATE_UPGRADE_UI)(void);

struct UPGRADE_INTERFACE_INFO
{
	ITEM_UPGRADE_TYPE		eUpgradeType;
	UI_COMPONENT_ENUM		eComponent;
	PFN_UPDATE_UPGRADE_UI	pfnUpdateFunc;
};

static const UPGRADE_INTERFACE_INFO sgInterfaceInfo[] = 
{
	{IUT_AUGMENT,	UICOMP_ITEM_AUGMENT_PANEL,	sUpdateAugmentUI},
	{IUT_UPGRADE,	UICOMP_ITEM_UPGRADE_PANEL,	sUpdateUpgradeUI},
	{IUT_UNMOD,		UICOMP_ITEM_UNMOD_PANEL,	sUpdateUnModUI},
};

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_UPGRADE_GLOBALS *sItemUpgradeGetGlobals(
	void)
{
	return &tUpgradeGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemUpgradeInitForGame(
	GAME *pGame)
{
	tUpgradeGlobals.dwAugmentStatusResetEvent = 0;
	tUpgradeGlobals.dwUpgradeStatusResetEvent = 0;
	for (int i = 0; i < MAX_ITEM_UPGRADE_RESOURCES; ++i)
	{
		tUpgradeGlobals.idResources[ i ] = INVALID_ID;
	}
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sFreeResourceItems(
	GAME * game)
{
	ASSERT_RETURN(game);
	
	for (unsigned int ii = 0; ii < MAX_ITEM_UPGRADE_RESOURCES; ++ii)
	{
		UNIT * item = UnitGetById(game, tUpgradeGlobals.idResources[ii]);
		if (item)
		{
			UnitFree(item);
		}
	}

	// hide all components
	UIComponentArrayDoAction(sgeResourceCompLookup, UIA_HIDE, INVALID_ID);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemUpgradeFreeForGame(
	GAME * game)
{
	sFreeResourceItems(game);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetResourcePoolInvLoc(
	void)
{
	return GlobalIndexGet( GI_INVENTORY_LOCATION_ITEM_UPGRADE_RESOURCES_POOL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreateResourceItem( 
	const ITEM_UPGRADE_RESOURCE *pUpgradeResource, 
	int nItemNumber)
{
	ASSERTX_RETURN( pUpgradeResource, "Expected upgrade resource" );
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "Expected game" );
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// create the item
	ITEM_SPEC tSpec;
	tSpec.nItemClass = pUpgradeResource->nItemClass;
	tSpec.nQuality = pUpgradeResource->nItemQuality;
	tSpec.nNumber = pUpgradeResource->nCount;
	SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
	tSpec.eLoadType = UDLT_INVENTORY;
	UNIT *pResource = ItemInit( pGame, tSpec );
	ASSERTX_RETURN( pResource, "Unable to create client only unit for item upgrade resource" );
	UnitSetStat( pResource, STATS_IDENTIFIED, TRUE );
	UnitSetStat( pResource, STATS_ITEM_QUANTITY_MAX, 999999 );
	ItemSetQuantity( pResource, pUpgradeResource->nCount );

	// add item to temporary inventory slot
	int nInvLoc = sGetResourcePoolInvLoc();
	int nX, nY;
	ASSERTX_RETURN( UnitInventoryFindSpace( pPlayer, pResource, nInvLoc, &nX, &nY ), "Unable to find space for resource in pool" );
	ASSERTX_RETURN( UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pResource, nInvLoc, nX, nY), "Unable to add resource to pool" );
		
	// save item id
	ASSERTX_RETURN( nItemNumber < MAX_ITEM_UPGRADE_RESOURCES, "Too many upgrade resources" );
	ITEM_UPGRADE_GLOBALS *pGlobals = sItemUpgradeGetGlobals();
	pGlobals->idResources[ nItemNumber ] = UnitGetId( pResource );
	
	// which UI element should have this item picture in it
	UI_COMPONENT_ENUM eCompResource = UICOMP_INVALID;
	if ( nItemNumber >= 0 && nItemNumber < sgnNumResourceComponents)
	{
		eCompResource = sgeResourceCompLookup[ nItemNumber ];
	}
	ASSERTX_RETURN( eCompResource != UICOMP_INVALID, "Expected ui component for item upgrade resource" );
	
	// show the component
	UIComponentSetActiveByEnum( eCompResource, TRUE );

	// set the focus unit for the picture	
	UI_COMPONENT *pCompResource = UIComponentGetByEnum( eCompResource );
	ASSERTX_RETURN( pCompResource, "Expected ui component for resource" );
	UIComponentSetFocusUnit( pCompResource, UnitGetId( pResource ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetResourceItems(
	GAME * game,
	UPGRADE_RESOURCES *pResources)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(pResources);
	
	// destroy any resource items we had before
	sFreeResourceItems(game);

	// go through each resource
	for (int i = 0; i < pResources->nNumResources; ++i)
	{
		const ITEM_UPGRADE_RESOURCE *pUpgradeResource = &pResources->tResources[ i ];
		sCreateResourceItem( pUpgradeResource, i );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sGetUpgradeStatusText(
	void)
{
	UNIT *pPlayer = UIGetControlUnit();
	
	if (pPlayer)
	{
		UNIT *pItem = ItemUpgradeGetItem( pPlayer );
		if (pItem != NULL)
		{
			ITEM_UPGRADE_INFO eInfo = ItemUpgradeCanUpgrade( pItem );
			if (eInfo == IFI_MAX_OWNER_LEVEL)
			{
				return GlobalStringGet( GS_ITEM_UPGRADE_STATUS_CANNOT_UPGRADE_MAX_OWNER_LEVEL );
			}
			else if (eInfo == IFI_NOT_IDENTIFIED)
			{
				return GlobalStringGet( GS_ITEM_UPGRADE_STATUS_CANNOT_UPGRADE_NOT_IDENTIFIED );
			}
			else if (eInfo == IFI_LIMIT_REACHED)
			{
				return GlobalStringGet( GS_ITEM_UPGRADE_STATUS_CANNOT_UPGRADE_LIMIT_REACHED );
			}
			else if (eInfo != IFI_UPGRADE_OK)
			{
				return GlobalStringGet( GS_ITEM_UPGRADE_STATUS_CANNOT_UPGRADE_UNKNOWN );
			}
		}
	}
			
	return NULL;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoUpgradeStatusReset(
	GAME *pGame,
	const CEVENT_DATA &tCEventData,
	DWORD)
{

	// clear the event
	tUpgradeGlobals.dwUpgradeStatusResetEvent = 0;
	
	// clear the status
	const WCHAR *puszStatus = sGetUpgradeStatusText();
	sSetUpgradeStatus( puszStatus, FALSE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetUpgradeStatus(
	const WCHAR *puszStatus, 
	BOOL bDoReset)
{	

	// if we have a clear event scheduled, cancel it
	if (tUpgradeGlobals.dwUpgradeStatusResetEvent != 0)
	{
		CSchedulerCancelEvent( tUpgradeGlobals.dwUpgradeStatusResetEvent );
		tUpgradeGlobals.dwUpgradeStatusResetEvent = 0;
	}
	
	// set new status message
	UILabelSetTextByEnum( UICOMP_ITEM_UPGRADE_STATUS, puszStatus ? puszStatus : L"" );
	
	// start event to clear the status message in a little while
	if (bDoReset == TRUE)
	{
		tUpgradeGlobals.dwUpgradeStatusResetEvent = 
			CSchedulerRegisterEvent( AppCommonGetCurTime() + STATUS_CLEAR_DELAY_IN_MS, sDoUpgradeStatusReset, NULL );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateUpgradeUI(
	void)
{
	UNIT * pPlayer = UIGetControlUnit();
	ASSERT_RETURN(pPlayer);

	GAME * game = UnitGetGame(pPlayer);
	ASSERT_RETURN(game);

	BOOL bCanUpgrade = FALSE;

	// get the item to upgrade
	UNIT *pItem = ItemUpgradeGetItem( pPlayer );
	if (pItem)
	{
		ITEM_UPGRADE_INFO eCanUpgrade = ItemUpgradeCanUpgrade( pItem );
		if (eCanUpgrade == IFI_UPGRADE_OK || eCanUpgrade == IFI_MAX_OWNER_LEVEL)
		{
				
			// get the resources necessary to upgrade this item
			UPGRADE_RESOURCES tResources;
			ItemUpgradeGetRequiredResources( pItem, &tResources );

			// set the resource information in the UI
			sSetResourceItems(game, &tResources);
			
			// check the contents of the resources slot and see if the player has them all there
			bCanUpgrade = ItemUpgradeHasResources( pPlayer, &tResources );

		}
						
	}
	else
	{
		// clear any items we had in the UI before
		sFreeResourceItems(game);
	}

	// enable/disable the upgrade button
	UIComponentSetActiveByEnum( UICOMP_ITEM_UPGRADE_BUTTON_UPGRADE, bCanUpgrade);

	// set status text
	const WCHAR *puszStatus = sGetUpgradeStatusText();	
	sSetUpgradeStatus( puszStatus, FALSE );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemUpgradeOpen(
	ITEM_UPGRADE_TYPE eUpgradeType)
{
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// close any previously displayed UI and reset back to starting state
	//c_ItemUpgradeClose(eUpgradeType);

	int i=0; 
	for (; i < arrsize(sgInterfaceInfo); i++)
	{
		if (sgInterfaceInfo[i].eUpgradeType == eUpgradeType)
			break;
	}
	if (i >= arrsize(sgInterfaceInfo))
		return;

	// open UI if not open already
	//TRADE_SPEC tTradeSpec;
	//tTradeSpec.eStyle = STYLE_ITEM_UPGRADE;		
	//UIShowTradeScreen( UnitGetId( pPlayer ), FALSE, &tTradeSpec );		

	if( AppIsTugboat() )
	{
		UIHideAllPaneMenus();
		// set the focus
		UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_ITEMAUGMENT, UnitGetId( pPlayer ), TRUE );

		UITogglePaneMenu( KPaneMenuItemAugment );
		UITogglePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );
	}
	else
	{
		UI_COMPONENT *pComponent = UIComponentGetByEnum(sgInterfaceInfo[i].eComponent);
		if (!pComponent)
		{
			return;
		}
		if (UIComponentActivate(pComponent, TRUE))
		{
			UIComponentSetFocusUnit(pComponent, UnitGetId( pPlayer ), TRUE);
		}
	}
	

	// do update of the buttons
	if (sgInterfaceInfo[i].pfnUpdateFunc)
		sgInterfaceInfo[i].pfnUpdateFunc();
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendUpgradeClosed(
	ITEM_UPGRADE_TYPE eType)
{

	MSG_CCMD_ITEM_UPGRADE_CLOSE tMessage;
	tMessage.bItemUpgradeType = (BYTE)eType;
	c_SendMessage( CCMD_ITEM_UPGRADE_CLOSE, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemUpgradeClose(
	ITEM_UPGRADE_TYPE eUpgradeType)
{
	if(AppIsTugboat() )
	{
		UITogglePaneMenu( KPaneMenuItemAugment );
	}
	else
	{
		int i=0; 
		for (; i < arrsize(sgInterfaceInfo); i++)
		{
			if (sgInterfaceInfo[i].eUpgradeType == eUpgradeType)
				break;
		}
		if (i >= arrsize(sgInterfaceInfo))
			return;

		// hide panel
		UIComponentActivate(sgInterfaceInfo[i].eComponent, FALSE);
	}
	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemUpgradeItemUpgraded(
	UNIT *pItem)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	
	// update the ui
	sUpdateUpgradeUI();
	
	// set the status message
	const WCHAR *puszStatus = GlobalStringGet( GS_ITEM_UPGRADE_STATUS_UPGRADED );
	sSetUpgradeStatus( puszStatus, TRUE );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sGetAugmentStatusString(
	void)
{
	UNIT *pPlayer = UIGetControlUnit();
	if (pPlayer)
	{
		UNIT *pItem = ItemUpgradeGetItem( pPlayer );
		if (pItem)
		{
		
			// check to see if we can augment ... we should technically be asking for
			// every augment type, but we don't have a status bar to explain that, so
			// we'll just ask for one type and ignore money restrictions in the status bar
			ITEM_AUGMENT_INFO eInfo = ItemAugmentCanAugment( pItem, IAUGT_ADD_COMMON );
			if (eInfo == IAI_PROPERTIES_FULL)	
			{
				return GlobalStringGet( GS_ITEM_AUGMENT_STATUS_CANNOT_AUGMENT_PROPERTIES_FULL );
			}
			else if (eInfo == IAI_NOT_IDENTIFIED)
			{
				return GlobalStringGet( GS_ITEM_AUGMENT_STATUS_CANNOT_AUGMENT_NOT_IDENTIFIED );
			}
			else if (eInfo == IAI_LIMIT_REACHED)
			{
				return GlobalStringGet( GS_ITEM_AUGMENT_STATUS_CANNOT_AUGMENT_LIMIT_REACHED );
			}
			else if (eInfo == IAI_NOT_ENOUGH_MONEY)
			{
				return NULL; // no message, let the buttons do the talking
			}
			else if (eInfo != IAI_AUGMENT_OK)
			{
				return GlobalStringGet( GS_ITEM_AUGMENT_STATUS_CANNOT_AUGMENT_UNKNOWN );
			}
			
		}
		
	}
	
	return NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAugmentStatusReset(
	GAME *pGame,
	const CEVENT_DATA &tCEventData,
	DWORD)
{

	// clear the event
	tUpgradeGlobals.dwAugmentStatusResetEvent = 0;
	
	// clear the status
	sSetAugmentStatus( sGetAugmentStatusString(), FALSE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetAugmentStatus(
	const WCHAR *puszStatus,
	BOOL bDoReset)
{

	// if we have a clear event scheduled, cancel it
	if (tUpgradeGlobals.dwAugmentStatusResetEvent != 0)
	{
		CSchedulerCancelEvent( tUpgradeGlobals.dwAugmentStatusResetEvent );
		tUpgradeGlobals.dwAugmentStatusResetEvent = 0;
	}
	
	// set new status message
	UILabelSetTextByEnum( UICOMP_ITEM_AUGMENT_STATUS, puszStatus ? puszStatus : L"" );
	
	// start event to clear the status message in a little while
	if (bDoReset == TRUE)
	{
		tUpgradeGlobals.dwAugmentStatusResetEvent = 
			CSchedulerRegisterEvent( AppCommonGetCurTime() + STATUS_CLEAR_DELAY_IN_MS, sDoAugmentStatusReset, NULL );
	}
		
}
	
//----------------------------------------------------------------------------
struct AUGMENT_ACTION
{
	BOOL bCanDoAction;
	UI_COMPONENT_ENUM eCompButton;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateAugmentUI(
	void)
{
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// setup lookup table for each of the augment possibilities
	AUGMENT_ACTION tAugmentAction[ IAUGT_NUM_TYPES ] = 
	{
		{ FALSE, UICOMP_ITEM_AUGMENT_BUTTON_ADD_COMMON },
		{ FALSE, UICOMP_ITEM_AUGMENT_BUTTON_ADD_RARE },
		{ FALSE, UICOMP_ITEM_AUGMENT_BUTTON_ADD_LEGENDARY },
		{ FALSE, UICOMP_ITEM_AUGMENT_BUTTON_ADD_RANDOM }
	};
		
	// get the item to augment
	UNIT *pItem = ItemUpgradeGetItem( pPlayer );
	if (pItem)
	{
	
		// check the validity of each augmentation action
		for (int i = 0; i < IAUGT_NUM_TYPES; ++i)
		{
			ITEM_AUGMENT_TYPE eAugmentType = (ITEM_AUGMENT_TYPE)i;
			AUGMENT_ACTION *pAction = &tAugmentAction[ eAugmentType ];
			pAction->bCanDoAction = ItemAugmentCanAugment( pItem, eAugmentType ) == IAI_AUGMENT_OK;
		}
		
	}

	if( AppIsHellgate() )
	{
		// enable/disable all augment action buttons
		for (int i = 0; i < IAUGT_NUM_TYPES; ++i)
		{
			const AUGMENT_ACTION *pAction = &tAugmentAction[ i ];

			// find the cost label for this action
			cCurrency nCost;
			if( pItem )
				nCost = ItemAugmentGetCost( pItem, (ITEM_AUGMENT_TYPE)i );		
			const int MAX_MONEY_STRING = 128;
			WCHAR uszMoneyString[ MAX_MONEY_STRING ];
			UIMoneyGetString( uszMoneyString, MAX_MONEY_STRING, nCost.GetValue( KCURRENCY_VALUE_INGAME ) );

			// set cost in UI (or blank)		
			UI_COMPONENT *pCompActionButton = UIComponentGetByEnum( pAction->eCompButton );
			// so you don't get messed up by the random action button
			if( pCompActionButton )
			{
				ASSERTX_RETURN( pCompActionButton, "Expected component for action button" );
				UI_COMPONENT *pCompLabelCost = UIComponentFindChildByName( pCompActionButton, "label cost" );
				UILabelSetText( pCompLabelCost, (nCost.IsZero() == FALSE)?uszMoneyString:L"" );

				// enalbe/disable
				UIComponentSetActiveByEnum( pAction->eCompButton, pAction->bCanDoAction );

			}

		}
	}
	else
	{
		const AUGMENT_ACTION *pAction = &tAugmentAction[ IAUGT_ADD_RANDOM ];

		cCurrency nCost;
		if( pItem )
			nCost = ItemAugmentGetCost( pItem, (ITEM_AUGMENT_TYPE)IAUGT_ADD_RANDOM );		
		const int MAX_MONEY_STRING = 128;


		UI_COMPONENT *pComp = UIComponentGetByEnum( UICOMP_PANEMENU_ITEMAUGMENT );
		UI_COMPONENT *pCompButton = UIComponentFindChildByName( pComp, "item upgrade button create" );
		UI_COMPONENT *pCompLabelCost = UIComponentFindChildByName( pComp, "label cost" );



		WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];		
		const WCHAR *uszFormat = GlobalStringGet(GS_PRICE_COPPER_SILVER_GOLD);
		if (uszFormat)
		{
			PStrCopy(uszTextBuffer, uszFormat, arrsize(uszTextBuffer));
		}
		WCHAR uszMoney[ MAX_MONEY_STRING ];
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
		// replace any money token with the money value
		const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
		// replace any money token with the money value
		puszGoldToken = StringReplacementTokensGet( SR_SILVER );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
		// replace any money token with the money value
		puszGoldToken = StringReplacementTokensGet( SR_GOLD );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		UILabelSetText( pCompLabelCost, (nCost.IsZero() == FALSE)?uszTextBuffer:L"" );

		// enalbe/disable
		UIComponentSetActive( pCompButton, pAction->bCanDoAction );

	}
	
	// set status text
	sSetAugmentStatus( sGetAugmentStatusString(), FALSE );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemAugmentItemAugmented(
	UNIT *pItem,
	int nAffixAugmented)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	
	// update the ui
	sUpdateAugmentUI();
	
	// set status message for augmentation
	const WCHAR *puszStatus = NULL;
	if (nAffixAugmented != INVALID_LINK)
	{
		puszStatus = GlobalStringGet( GS_ITEM_AUGMENT_STATUS_AUGMENTED );
	}
	else
	{
		puszStatus = GlobalStringGet( GS_ITEM_AUGMENT_STATUS_AUGMENT_FAILED );
	}
	
	// set status message
	sSetAugmentStatus( puszStatus, TRUE );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUpgradeOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_INVGRID *pInvGrid = UICastToInvGrid( pComponent );
	
	// must be this grid ... but really, the UI should not be calling this callback at all
	// for grids that are not clicked ...!  grrrrr!
	int nInvLoc = (int)lParam;
	if (nInvLoc != pInvGrid->m_nInvLocation)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// must be in bounds
	if (!UIComponentGetActive( pComponent )/* || !UIComponentCheckBounds( pComponent )*/)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// call the default grid on change function
	UIInvGridOnInventoryChange( pComponent, nMessage, wParam, lParam );

	// update the action buttons
	sUpdateUpgradeUI();
	
	return UIMSG_RET_HANDLED;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUpgradeUpgrade( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{

	// must be in bounds
	if (!UIComponentGetActive( pComponent ) || !UIComponentCheckBounds( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// tell server to upgrade item
	MSG_CCMD_TRY_ITEM_UPGRADE tMessage;
	c_SendMessage( CCMD_TRY_ITEM_UPGRADE, &tMessage );
	
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIItemUpgradeCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	c_ItemUpgradeClose(IUT_UPGRADE);
	
	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUnModButtonOnClick( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	// tell server to remove mods from the item
	MSG_CCMD_TRY_ITEM_UNMOD tMessage;
	c_SendMessage( CCMD_TRY_ITEM_UNMOD, &tMessage );
	
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sGetUnModStatusText(
	void)
{
	UNIT *pPlayer = UIGetControlUnit();
	if (pPlayer)
	{
		UNIT *pItem = ItemUnModGetItem( pPlayer );
		if (pItem)
		{
		
			// check to see if we can UnMod ... we should technically be asking for
			// every UnMod type, but we don't have a status bar to explain that, so
			// we'll just ask for one type and ignore money restrictions in the status bar
			ITEM_UNMOD_INFO eInfo = ItemCanUnMod( pItem );
			if (eInfo == IUI_NO_MODS)	
			{
				return GlobalStringGet( GS_ITEM_UNMOD_STATUS_NOMODS );
			}
			if (eInfo == IUI_RESULTS_NOT_EMPTY)	
			{
				return GlobalStringGet( GS_ITEM_UNMOD_STATUS_TAKE_RESULTS );
			}
			if (eInfo == IUI_NOT_ENOUGH_MONEY)	
			{
				return NULL;		//we'll set the cost in the status panel later
			}
			else if (eInfo != IUI_UNMOD_OK)
			{
				return GlobalStringGet( GS_ITEM_UNMOD_STATUS_FAILED );
			}
			
		}
		
	}
	
	return NULL;
	
}

static void sSetUnModStatus(
	const WCHAR *puszStatus,
	BOOL bDoReset);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoUnModStatusReset(
	GAME *pGame,
	const CEVENT_DATA &tCEventData,
	DWORD)
{

	// clear the event
	tUpgradeGlobals.dwUnModStatusResetEvent = 0;
	
	// clear the status
	sSetUnModStatus( sGetUnModStatusText(), FALSE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSetUnModStatus(
	const WCHAR *puszStatus,
	BOOL bDoReset)
{

	// if we have a clear event scheduled, cancel it
	if (tUpgradeGlobals.dwUnModStatusResetEvent != 0)
	{
		CSchedulerCancelEvent( tUpgradeGlobals.dwUnModStatusResetEvent );
		tUpgradeGlobals.dwUnModStatusResetEvent = 0;
	}
	
	// set new status message
	UILabelSetTextByEnum( UICOMP_ITEM_UNMOD_STATUS, puszStatus ? puszStatus : L"" );
	
	// start event to clear the status message in a little while
	if (bDoReset == TRUE)
	{
		tUpgradeGlobals.dwUnModStatusResetEvent = 
			CSchedulerRegisterEvent( AppCommonGetCurTime() + STATUS_CLEAR_DELAY_IN_MS, sDoUnModStatusReset, NULL );
	}
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateUnModUI(
	void)
{
	UNIT *pPlayer = UIGetControlUnit();

	// get the item to upgrade
	UNIT *pItem = ItemUnModGetItem( pPlayer );
	BOOL bCanUnMod = ((pItem && ItemCanUnMod( pItem ) == IUI_UNMOD_OK));

	// enable/disable the upgrade button
	UIComponentSetActiveByEnum( UICOMP_ITEM_UNMOD_BUTTON, bCanUnMod);

	// set status text
	const WCHAR *puszStatus = sGetUnModStatusText();	
	if (puszStatus == NULL && pItem)
	{
		// find the cost label for this action
		cCurrency cost = pItem ? ItemUnModGetCost( pItem ) : cCurrency();
		WCHAR uszBuf[ 128 ] = L"";
		WCHAR uszMoneyBuf[ 128 ] = L"";
		WCHAR uszMoneyString[ 128 ] = L"";
		UIMoneyGetString( uszMoneyString, arrsize(uszMoneyString), cost.GetValue( KCURRENCY_VALUE_INGAME ) );

		if ( UnitGetCurrency( UIGetControlUnit() ) < cost )
		{
			UIColorCatString(uszMoneyBuf, arrsize(uszMoneyBuf), FONTCOLOR_LIGHT_RED, uszMoneyString);
		}
		else
		{
			PStrCopy(uszMoneyBuf, uszMoneyString, arrsize(uszMoneyBuf));
		}

		const WCHAR *uszFormat = GlobalStringGet(GS_ITEM_UNMOD_STATUS_COST);
		if (uszFormat)
		{
			PStrCopy(uszBuf, uszFormat, arrsize(uszBuf));
			PStrReplaceToken(uszBuf, arrsize(uszBuf), StringReplacementTokensGet( SR_STRING1 ), uszMoneyBuf);
		}
		else
		{
			PStrCopy(uszBuf, uszMoneyBuf, arrsize(uszBuf));
		}

		puszStatus = &(uszBuf[0]);
	}
	sSetUnModStatus( puszStatus, FALSE );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUnModOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_INVGRID *pInvGrid = UICastToInvGrid( pComponent );
	
	// must be this grid ... but really, the UI should not be calling this callback at all
	// for grids that are not clicked ...!  grrrrr!
	int nInvLoc = (int)lParam;
	if (nInvLoc != pInvGrid->m_nInvLocation)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// must be in bounds
	if (!UIComponentGetActive( pComponent )/* || !UIComponentCheckBounds( pComponent )*/)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// call the default grid on change function
	UIInvGridOnInventoryChange( pComponent, nMessage, wParam, lParam );

	// update the controls
	sUpdateUnModUI();
	
	return UIMSG_RET_HANDLED;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentOnChangeItem( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{

	UI_INVGRID *pInvGrid = UICastToInvGrid( pComponent );
	
	int nInvLoc = (int)lParam;
	if (nInvLoc != pInvGrid->m_nInvLocation)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentGetActive( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// call the default grid on change function
	UIInvGridOnInventoryChange( pComponent, nMessage, wParam, lParam );

	// update the action buttons
	sUpdateAugmentUI();
	
	return UIMSG_RET_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTryAugment(
	ITEM_AUGMENT_TYPE eAugmentType)
{
	
	// setup message
	MSG_CCMD_TRY_ITEM_AUGMENT tMessage;
	tMessage.bAugmentType = (BYTE)eAugmentType;
	
	// send to server
	c_SendMessage( CCMD_TRY_ITEM_AUGMENT, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentCommon( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		sSendTryAugment( IAUGT_ADD_COMMON );
		return UIMSG_RET_HANDLED;		
	}
	return UIMSG_RET_NOT_HANDLED;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentRare( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		sSendTryAugment( IAUGT_ADD_RARE );
		return UIMSG_RET_HANDLED;		
	}
	return UIMSG_RET_NOT_HANDLED;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentLegendary( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		sSendTryAugment( IAUGT_ADD_LEGENDARY );
		return UIMSG_RET_HANDLED;		
	}
	return UIMSG_RET_NOT_HANDLED;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentRandom( 
									 UI_COMPONENT *pComponent, 
									 int nMessage, 
									 DWORD wParam, 
									 DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		sSendTryAugment( IAUGT_ADD_RANDOM );
		return UIMSG_RET_HANDLED;		
	}
	return UIMSG_RET_NOT_HANDLED;
}	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	// hide UI
	c_ItemUpgradeClose(IUT_AUGMENT);
	
	return UIMSG_RET_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUpgradeOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (!pComponent->m_bUserActive)
	{
		// tell server its closed now		
		sSendUpgradeClosed( IUT_UPGRADE );
	}
		
	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemAugmentOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (!pComponent->m_bUserActive)
	{
		// tell server its closed now		
		sSendUpgradeClosed( IUT_AUGMENT );
	}

	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUnModOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if (!pComponent->m_bUserActive)
	{
		// tell server its closed now		
		sSendUpgradeClosed( IUT_UNMOD );
	}

	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemUpgradePostActivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	sUpdateUpgradeUI();

	return UIMSG_RET_HANDLED;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIItemUnModCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	c_ItemUpgradeClose(IUT_UNMOD);
	
	return UIMSG_RET_HANDLED;	
}

#endif
