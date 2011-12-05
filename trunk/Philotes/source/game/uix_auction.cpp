//----------------------------------------------------------------------------
// uix_auction.cpp
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_auction.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_chat.h"
#include "stringreplacement.h"
#include "globalindex.h"
#include "c_message.h"
#include "c_unitnew.h"
#include "items.h"
#include "language.h"
#include "array.h"
#include "c_trade.h"

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define AUCTION_RESULT_PAGE_SIZE	12
#define AUCTION_RESULT_CACHE_SIZE	6
#define AUCTION_PRELOAD_PAGES		1
#define AUCTION_USE_FREE_LIST		TRUE
#define AUCTION_FREE_LIST_INIT_SIZE	30

struct T_RESULT_LINE
{
	PGUID				m_guid;
	WCHAR				m_szItemName[MAX_ITEM_NAME_STRING_LENGTH];
	int					m_nQuality;
	int					m_nLevel;
	int					m_nPrice;
	WCHAR *				m_szSellerName;
	UNIT *				m_pUnit;
	UI_COMPONENT *		m_pPanel;
};

struct T_CACHE_PAGE
{
	int					m_nPageNum;
	BOOL				m_bFullyLoaded;
	T_RESULT_LINE		m_ResultLines[AUCTION_RESULT_PAGE_SIZE];
};

struct T_RESULT_SET
{
	T_CACHE_PAGE		m_pCachePages[AUCTION_RESULT_CACHE_SIZE];
	T_CACHE_PAGE *		m_pCurCachePage;
	int					m_nNumResults;
	int					m_nNumPages;
	int					m_nCurPage;	
	int					m_nAwaitingResults;
};

static T_RESULT_SET		s_SearchResults;
static T_RESULT_SET		s_MyForSaleItems;

static T_RESULT_SET *	s_pCurResultSet = &s_SearchResults;

static BOOL s_bFirstPage = TRUE;

static CArrayIndexed<UNITID> s_FreeList;
static BOOL s_bUseFreeList = FALSE;

static PGUID s_nSellItem = INVALID_GUID;
static int s_nSellMoney = 0;

void UIAuctionSearchUpdatePageInfo(void);

static DWORD s_dwFeeRate = (DWORD)INVALID_LINK;
static DWORD s_dwMaxItemSaleCount = (DWORD)INVALID_LINK;

static BOOL s_bBuyButtonEnabled = TRUE;

// externs

extern DWORD x_AuctionGetCostToSell(const int nItemPrice, const DWORD dwFeeRate);
extern DWORD x_AuctionGetMinSellPrice(UNIT * pItem);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionInit(
	void)
{
	UNIT * pPlayer = UIGetControlUnit();
	if (pPlayer && UnitIsInTown(pPlayer))
	{
		MSG_CCMD_AH_GET_INFO msgGetInfo;
		c_SendMessage(CCMD_AH_GET_INFO, &msgGetInfo);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionInfo(
	MSG_SCMD_AH_INFO * msg)
{
	ASSERT_RETURN(msg);

	s_dwFeeRate = msg->dwFeeRate;
	s_dwMaxItemSaleCount = msg->dwMaxItemSaleCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleAuctionHouse(
	void)
{
#if !ISVERSION(DEVELOPMENT)
	if (AppIsSinglePlayer())
	{
		return;
	}
#endif
	
	if( AppIsHellgate() )
	{
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
		if (pComponent)
		{
			if (UIComponentGetVisible(pComponent))
			{
				UIComponentActivate(pComponent, FALSE);
			}
			else
			{
				UIComponentActivate(pComponent, TRUE);
				UIAuctionUpdateSellButtonState();
				UIAuctionSearchUpdatePageInfo();
			}

			UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
		}

		pComponent = UIComponentGetByEnum(UICOMP_AUCTION_BODY_PANEL);
		if (pComponent)
		{
			UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleAuctionHouse(
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIToggleAuctionHouse();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionFreeListStart(
	void)
{
	if (!s_FreeList.Initialized())
	{
#if ISVERSION(DEBUG_VERSION)
		s_FreeList.Init(NULL, AUCTION_FREE_LIST_INIT_SIZE, __FILE__, __LINE__);
#else
		s_FreeList.Init(NULL, AUCTION_FREE_LIST_INIT_SIZE);
#endif
	}

	s_bUseFreeList = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionFreeListAdd(
	UNITID unitid)
{
	ASSERT_RETURN(s_bUseFreeList);

	int nId;
	UNITID * pNewNode = s_FreeList.Add( &nId );
	ASSERT( pNewNode );
	*pNewNode = unitid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionFreeListRemove(
	UNITID unitid)
{
	ASSERT_RETURN(s_bUseFreeList);

	for ( int nId = s_FreeList.GetFirst(); nId != INVALID_ID; nId = s_FreeList.GetNextId( nId ) )
	{
		UNITID * pUnitId = s_FreeList.Get( nId );
		UNITID thisUnitId = *pUnitId;
		if (thisUnitId == unitid)
		{
			s_FreeList.Remove(nId);
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionFreeListEnd(
	void)
{
	if (!s_bUseFreeList)
	{
		return;
	}

	for ( int nId = s_FreeList.GetFirst(); nId != INVALID_ID; nId = s_FreeList.GetNextId( nId ) )
	{
		UNITID * pUnitId = s_FreeList.Get( nId );
		UNIT * pUnit = UnitGetById(AppGetCltGame(), *pUnitId);
		if (pUnit)
		{
			UnitFree(pUnit);
		}
	}

	s_FreeList.Clear();
	s_bUseFreeList = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITTYPE sUIAuctionGetSelectedFaction()
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETVAL(pPanel, UNITTYPE_NONE);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction search faction combo");
	ASSERT_RETVAL(pComponent, UNITTYPE_NONE);

	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	if (UIListBoxGetSelectedIndex(pCombo->m_pListBox) != INVALID_INDEX)
	{
		return (UNITTYPE)UIListBoxGetSelectedData(pCombo->m_pListBox);
	}

	return UNITTYPE_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITTYPE sUIAuctionGetSelectedType()
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETVAL(pPanel, UNITTYPE_NONE);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction search item type combo");
	ASSERT_RETVAL(pComponent, UNITTYPE_NONE);

	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	if (UIListBoxGetSelectedIndex(pCombo->m_pListBox) != INVALID_INDEX)
	{
		return (UNITTYPE)UIListBoxGetSelectedData(pCombo->m_pListBox);
	}

	return UNITTYPE_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchUnitTypeOnChange(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	GAME * pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETVAL( pPanel, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction item types");
	ASSERT_RETVAL( pComponent, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT *pButton = NULL, *pLabel = NULL;
	while ((pButton = UIComponentIterateChildren(pComponent, pButton, UITYPE_BUTTON, FALSE)) != NULL)
	{
		UIComponentSetActive(pButton, FALSE);
		UIComponentSetVisible(pButton, FALSE);
	}

	const UNITTYPE utFaction = sUIAuctionGetSelectedFaction();
	const UNITTYPE utType = sUIAuctionGetSelectedType();

	if (utFaction == UNITTYPE_NONE || utType == UNITTYPE_NONE)
	{
		return UIMSG_RET_HANDLED;
	}

	int nRows = ExcelGetNumRows( pGame, DATATABLE_UNITTYPES );
	ASSERT( nRows > 0 );
	pButton = NULL;

	for ( int i = 0; i < nRows; i++ )
	{
		UNITTYPE_DATA* pUnitTypeData = (UNITTYPE_DATA*)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_UNITTYPES, i);
		if (!pUnitTypeData) continue;

		const UNITTYPE utThisFaction = (UNITTYPE)(AppIsHellgate() ? pUnitTypeData->nHellgateAuctionFaction : pUnitTypeData->nMythosAuctionFaction);
		if (utThisFaction != utFaction) continue;

		const UNITTYPE utThisType = (UNITTYPE)(AppIsHellgate() ? pUnitTypeData->nHellgateAuctionType : pUnitTypeData->nMythosAuctionType);
		if (utThisType != utType) continue;

		const int nString = AppIsHellgate() ? pUnitTypeData->nHellgateAuctionName : pUnitTypeData->nMythosAuctionName;

		const BOOL bDown = (pButton == NULL);
		pButton = UIComponentIterateChildren(pComponent, pButton, UITYPE_BUTTON, FALSE);
		ASSERT_BREAK(pButton);

		UIComponentSetActive(pButton, TRUE);
		UIComponentSetVisible(pButton, TRUE);
		UIButtonSetDown(UICastToButton(pButton), bDown);
		UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

		if ((pLabel = UIComponentIterateChildren(pButton, NULL, UITYPE_LABEL, FALSE)) != NULL)
		{
			UILabelSetText(pLabel, StringTableGetStringByIndex(nString));
		}
		pButton->m_dwData = (DWORD)i;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITTYPE UIAuctionSearchGetSelectedUnitType()
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETVAL( pPanel, UNITTYPE_NONE );

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction item types");
	ASSERT_RETVAL( pComponent, UNITTYPE_NONE );

	UI_COMPONENT *pButton = NULL;
	while ((pButton = UIComponentIterateChildren(pComponent, pButton, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (UIComponentGetVisible(pButton) && UIComponentGetActive(pButton))
		{
			if (UIButtonGetDown(UICastToButton(pButton)))
			{
				return (UNITTYPE)pButton->m_dwData;
			}
		}
	}

	return UNITTYPE_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUIAuctionGetPlayerFactionIdx(
	void)
{
	UNIT * pPlayer = UIGetControlUnit();
	ASSERT_RETVAL(pPlayer, -1);

	if (UnitIsA(pPlayer, UNITTYPE_CABALIST))
	{
		return 0;
	}
	else if (UnitIsA(pPlayer, UNITTYPE_HUNTER))
	{
		return 1;
	}
	else if (UnitIsA(pPlayer, UNITTYPE_TEMPLAR))
	{
		return 2;
	}
	else
	{
		return 3;
	}
}

static BOOL s_bOnChange = FALSE;
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchFactionComboOnChange(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (s_bOnChange) return UIMSG_RET_HANDLED;
	s_bOnChange = TRUE;

	UI_MSG_RETVAL ret = UIAuctionSearchUnitTypeOnChange(component, nMessage, wParam, lParam);
	if (!ResultIsHandled(ret))
	{
		s_bOnChange = FALSE;
		return ret;
	}

	if (UIAuctionSearchGetSelectedUnitType() == UNITTYPE_NONE)
	{
		UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
		ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction search item type combo");
		ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

		UI_COMBOBOX *pCombo = UICastToComboBox(pComponent);
		UI_LISTBOX *pListBox = pCombo->m_pListBox;

		for (unsigned int i=0; i<pListBox->m_LinesList.Count(); ++i)
		{
			UIListBoxSetSelectedIndex(pCombo->m_pListBox, i);
			ret = UIAuctionSearchUnitTypeOnChange(component, nMessage, wParam, lParam);
			if (UIAuctionSearchGetSelectedUnitType() != UNITTYPE_NONE)
			{
				break;
			}
		}
	}

	s_bOnChange = FALSE;
	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchTypeComboOnChange(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (s_bOnChange) return UIMSG_RET_HANDLED;
	s_bOnChange = TRUE;

	UI_MSG_RETVAL ret = UIAuctionSearchUnitTypeOnChange(component, nMessage, wParam, lParam);
	if (!ResultIsHandled(ret))
	{
		s_bOnChange = FALSE;
		return ret;
	}

	if (UIAuctionSearchGetSelectedUnitType() == UNITTYPE_NONE)
	{
		int nNewFactionIdx;

		if (sUIAuctionGetSelectedFaction() == UNITTYPE_ANY)
		{
			nNewFactionIdx = sUIAuctionGetPlayerFactionIdx();
		}
		else
		{
			nNewFactionIdx = 3; // "any"
		}

		UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
		ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "auction search faction combo");
		ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

		UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
		UIListBoxSetSelectedIndex(pCombo->m_pListBox, nNewFactionIdx);

		ret = UIAuctionSearchUnitTypeOnChange(component, nMessage, wParam, lParam);
	}

	s_bOnChange = FALSE;
	return ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchFactionComboOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	UITextBoxClear(pCombo->m_pListBox);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_FACTION_CABALIST), (QWORD)UNITTYPE_CABALIST);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_FACTION_HUNTER), (QWORD)UNITTYPE_HUNTER);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_FACTION_TEMPLAR), (QWORD)UNITTYPE_TEMPLAR);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_FACTION_ANY), (QWORD)UNITTYPE_ANY);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchFactionComboOnPostActivate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	int nPlayerFaction = sUIAuctionGetPlayerFactionIdx();
	UIListBoxSetSelectedIndex(pCombo->m_pListBox, nPlayerFaction);
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchItemTypeComboOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	UITextBoxClear(pCombo->m_pListBox);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_ITEM_TYPE_WEAPONS), (QWORD)UNITTYPE_WEAPON);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_ITEM_TYPE_ARMOR), (QWORD)UNITTYPE_ARMOR);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_ITEM_TYPE_MODS), (QWORD)UNITTYPE_MOD);
	if (AppIsTugboat())
	{
		UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_ITEM_TYPE_INGREDIENTS), (QWORD)UNITTYPE_CRAFTING_ITEM);
	}
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_ITEM_TYPE_MISC), (QWORD)UNITTYPE_CONSUMABLE);

	UIListBoxSetSelectedIndex(pCombo->m_pListBox, 0);
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchQualityComboOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	UITextBoxClear(pCombo->m_pListBox);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_MINIMUM_QUALITY_NORMAL), 1);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_MINIMUM_QUALITY_UNCOMMON), 2);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_MINIMUM_QUALITY_RARE) ,3);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_MINIMUM_QUALITY_LEGENDARY), 4);
	UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_AUCTION_MINIMUM_QUALITY_UNIQUE), 6);

	UIListBoxSetSelectedIndex(pCombo->m_pListBox, 0);
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchDoSearch(
	const DWORD dwSortMethod)
{
	MSG_CCMD_AH_SEARCH_ITEMS msgSearch;

	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED);

	const UNITTYPE utUnitType = UIAuctionSearchGetSelectedUnitType();
	if (utUnitType == UNITTYPE_NONE)
	{
		return UIMSG_RET_HANDLED;
	}

	UI_COMPONENT *pComponent = NULL;
	const WCHAR *wszText = NULL;
	int i;

	msgSearch.ItemType = utUnitType;

	if ((pComponent = UIComponentFindChildByName(pPanel, "auction search level min edit")) != NULL)
	{
		if ((wszText = UILabelGetText(UICastToLabel(pComponent))) != NULL)
		{
			PStrToInt(wszText, i);
			msgSearch.ItemMinLevel = (DWORD)i;
		}
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "auction search level max edit")) != NULL)
	{
		if ((wszText = UILabelGetText(UICastToLabel(pComponent))) != NULL)
		{
			PStrToInt(wszText, i);
			msgSearch.ItemMaxLevel = (DWORD)i;
		}
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "auction search price min edit")) != NULL)
	{
		if ((wszText = UILabelGetText(UICastToLabel(pComponent))) != NULL)
		{
			PStrToInt(wszText, i);
			msgSearch.ItemMinPrice = (DWORD)i;
		}
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "auction search price max edit")) != NULL)
	{
		if ((wszText = UILabelGetText(UICastToLabel(pComponent))) != NULL)
		{
			PStrToInt(wszText, i);
			msgSearch.ItemMaxPrice = (DWORD)i;
		}
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "auction search quality combo")) != NULL)
	{
		UI_COMBOBOX *pCombo = UICastToComboBox(pComponent);
		UI_LISTBOX *pListBox = UICastToListBox(pCombo->m_pListBox);
		msgSearch.ItemMinQuality = (DWORD)UIListBoxGetSelectedData(pListBox);
	}

	msgSearch.ItemSortMethod = dwSortMethod;

	c_SendMessage(CCMD_AH_SEARCH_ITEMS, &msgSearch);

	s_pCurResultSet->m_nCurPage = 0;
	UIAuctionSearchUpdatePageInfo();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchButtonOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	s_SearchResults.m_pCurCachePage = &s_SearchResults.m_pCachePages[0];
	s_SearchResults.m_nNumResults = 0;
	s_SearchResults.m_nNumPages = 0;
	s_SearchResults.m_nCurPage = 0;
	s_SearchResults.m_nAwaitingResults = 0;

	s_pCurResultSet = &s_SearchResults;

	return UIAuctionSearchDoSearch(AUCTION_RESULT_SORT_BY_PRICE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchQualityOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (s_pCurResultSet->m_nNumResults > 0)
	{
		return UIAuctionSearchDoSearch(AUCTION_RESULT_SORT_BY_QUALITY);
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchLevelOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (s_pCurResultSet->m_nNumResults > 0)
	{
		return UIAuctionSearchDoSearch(AUCTION_RESULT_SORT_BY_LEVEL);
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchPriceOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (s_pCurResultSet->m_nNumResults > 0)
	{
		return UIAuctionSearchDoSearch(AUCTION_RESULT_SORT_BY_PRICE);
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUIAuctionGetCostToSell(
								   PGUID guid,
								   const int nSellPrice)
{
	REF(guid);
	ASSERT_RETZERO(s_dwFeeRate != (DWORD)INVALID_LINK);

	return (int)x_AuctionGetCostToSell(nSellPrice, s_dwFeeRate);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sUIAuctionGetMinSellPrice(
									 const PGUID nSellItem)
{
	UNIT *pSellItem = UnitGetByGuid(AppGetCltGame(), s_nSellItem);
	ASSERT_RETZERO(pSellItem);

	return (int)x_AuctionGetMinSellPrice(pSellItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionRequestResultPage(const int nPage);

void UIAuctionError(
	MSG_SCMD_AH_ERROR * msg)
{
	ASSERT_RETURN(msg);

	switch (msg->ErrorCode)
	{
		case AH_ERROR_NOT_INIT:
		{
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_ERROR_NOT_INIT));
			break;
		}
		case AH_ERROR_SEARCH_BUSY:
		{
			// TODO: this should probably auto-retry a few times before erroring
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_ERROR_SEARCH_BUSY));
			break;
		}
		case AH_ERROR_SEARCH_FAILED:
		{
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_ERROR_SEARCH_FAILED));
			break;
		}
		case AH_ERROR_SEARCH_RANGE_INVALID:
		{
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_ERROR_SEARCH_RANGE_INVALID));
			break;
		}
		case AH_ERROR_SEARCH_ITEM_DOES_NOT_EXIST:
		{
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_ERROR_SEARCH_ITEM_DOES_NOT_EXIST));
			break;
		}
		case AH_ERROR_SELL_ITEM_PRICE_TOO_LOW:
		{
			WCHAR szStringBuf[256], szNum[32];
			int nMinSellPrice = sUIAuctionGetMinSellPrice(s_nSellItem);
			LanguageFormatIntString(szNum, arrsize(szNum), (int)nMinSellPrice);
			PStrCopy(szStringBuf, GlobalStringGet(GS_AUCTION_MINIMUM_SELL_PRICE_IS), arrsize(szStringBuf));
			PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_MONEY), szNum );
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), szStringBuf);
			break;
		}
		case AH_ERROR_BUY_ITEM_SUCCESS:
		case AH_ERROR_WITHDRAW_ITEM_SUCCESS:
		{
			UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
			break;
		}
		case AH_ERROR_SELL_ITEM_SUCCESS:
		{
			break;
		}
		default:
		{
			WCHAR szStringBuf[256], szNum[32];
			LanguageFormatIntString(szNum, arrsize(szNum), (int)msg->ErrorCode);
			PStrCopy(szStringBuf, GlobalStringGet(GS_AUCTION_ERROR_NUMBER), arrsize(szStringBuf));
			PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_NUMBER), szNum );
			UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), szStringBuf);
			break;
		}
	};

	UIAuctionEnableBuyButtons();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAuctionFlushResultLine(
	T_RESULT_LINE * pResultLine)
{
	UNIT * pUnit = UnitGetByGuid(AppGetCltGame(), pResultLine->m_guid);
	if (pUnit)
	{
		if (s_bUseFreeList)
		{
			sUIAuctionFreeListAdd(UnitGetId(pUnit));
		}
		else
		{
			UnitFree(pUnit);
		}
	}
	pResultLine->m_guid = INVALID_GUID;
	pResultLine->m_pUnit = NULL;

	PStrCopy(pResultLine->m_szItemName, L"", arrsize(pResultLine->m_szItemName));
	pResultLine->m_nQuality = 0;
	pResultLine->m_nLevel = 0;
	pResultLine->m_nPrice = 0;
	pResultLine->m_szSellerName = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAuctionFlushCachePage(
	T_CACHE_PAGE * pCachePage)
{
	ASSERT_RETURN(pCachePage);

	for (int i=0; i<AUCTION_RESULT_PAGE_SIZE; ++i)
	{
		if (pCachePage == s_pCurResultSet->m_pCurCachePage && pCachePage->m_ResultLines[i].m_pPanel)
		{
			UIComponentSetVisible(pCachePage->m_ResultLines[i].m_pPanel, FALSE);
			pCachePage->m_ResultLines[i].m_pPanel->m_qwData = INVALID_GUID;
			pCachePage->m_ResultLines[i].m_pPanel->m_dwData = 0;
		}
		sAuctionFlushResultLine(&pCachePage->m_ResultLines[i]);
	}

	pCachePage->m_nPageNum = -1;
	pCachePage->m_bFullyLoaded = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAuctionFlushCache(
	void)
{
	for (int i=0; i<AUCTION_RESULT_CACHE_SIZE; ++i)
	{
		sAuctionFlushCachePage(&s_pCurResultSet->m_pCachePages[i]);
	}

	s_pCurResultSet->m_pCurCachePage = &s_pCurResultSet->m_pCachePages[0];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static T_RESULT_LINE * sAuctionFindResultLineByGuid(
	const PGUID guid,
	const BOOL bExcludeLoadedPages = FALSE,
	T_CACHE_PAGE ** pCachePage = NULL)
{
	for (int i=0; i<AUCTION_RESULT_CACHE_SIZE; ++i)
	{
		if (!bExcludeLoadedPages || !s_pCurResultSet->m_pCachePages[i].m_bFullyLoaded)
		{
			for (int j=0; j<AUCTION_RESULT_PAGE_SIZE; ++j)
			{
				if (s_pCurResultSet->m_pCachePages[i].m_ResultLines[j].m_guid == guid)
				{
					if (pCachePage)
					{
						*pCachePage = &s_pCurResultSet->m_pCachePages[i];
					}
					return &s_pCurResultSet->m_pCachePages[i].m_ResultLines[j];
				}
			}
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static T_CACHE_PAGE * sAuctionFindCachePage(
	const int nPage)
{
	for (int i=0; i<AUCTION_RESULT_CACHE_SIZE; ++i)
	{
		if (s_pCurResultSet->m_pCachePages[i].m_nPageNum == nPage)
		{
			return &s_pCurResultSet->m_pCachePages[i];
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static T_CACHE_PAGE * sAuctionFindAvailableCachePage(
	const int nPage)
{
	for (int i=0; i<AUCTION_RESULT_CACHE_SIZE; ++i)
	{
		if (s_pCurResultSet->m_pCachePages[i].m_nPageNum == -1 ||
			s_pCurResultSet->m_pCachePages[i].m_nPageNum == nPage)
		{
			s_pCurResultSet->m_pCachePages[i].m_nPageNum = nPage;
			return &s_pCurResultSet->m_pCachePages[i];
		}
	}

	// find oldest page (except first and last pages) and flush it

	int iOldestAge = 0;
	T_CACHE_PAGE * pOldestPage = NULL;

	for (int i=0; i<AUCTION_RESULT_CACHE_SIZE; ++i)
	{
		if (s_pCurResultSet->m_pCachePages[i].m_nPageNum != 0 &&
			s_pCurResultSet->m_pCachePages[i].m_nPageNum != (s_pCurResultSet->m_nNumPages-1))
		{
			const int iAge = abs(s_pCurResultSet->m_pCachePages[i].m_nPageNum - nPage);
			if (iAge > iOldestAge)
			{
				iOldestAge = iAge;
				pOldestPage = &s_pCurResultSet->m_pCachePages[i];
			}
		}
	}

	sAuctionFlushCachePage(pOldestPage);
	pOldestPage->m_nPageNum = nPage;

	return pOldestPage;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAuctionInitCache(
	void)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_BODY_PANEL);
	ASSERT_RETURN(pPanel);

	UI_COMPONENT *pList = UIComponentFindChildByName(pPanel, "search results list");
	ASSERT_RETURN(pList);

	UI_COMPONENT *pComponent = NULL;

	for (int i=0; i<AUCTION_RESULT_PAGE_SIZE; ++i)
	{
		pComponent = UIComponentIterateChildren(pList, pComponent, UITYPE_PANEL, FALSE);
		ASSERT_RETURN(pComponent);

		for (int j=0; j<AUCTION_RESULT_CACHE_SIZE; ++j)
		{
			s_pCurResultSet->m_pCachePages[j].m_ResultLines[i].m_pPanel = pComponent;
			pComponent->m_qwData = INVALID_GUID;
			pComponent->m_dwData = 0;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResultUpdateUI(
	PGUID guid)
{
	UNIT *pUnit = UnitGetByGuid(AppGetCltGame(), guid);

	if (pUnit)
	{
		T_RESULT_LINE * pResultLine = NULL;

		if ((pResultLine = sAuctionFindResultLineByGuid(guid)) != NULL)
		{
			UI_COMPONENT *pLabel = NULL;
			UI_COMPONENT *pPanel = pResultLine->m_pPanel;
			ASSERT_RETURN(pPanel);
			WCHAR szText[50];

			UIComponentSetActive(pPanel, TRUE);
			pPanel->m_qwData = guid;
			pPanel->m_dwData = pResultLine->m_nPrice;

			WCHAR szQuality[MAX_ITEM_NAME_STRING_LENGTH] = L"";
			ItemGetQualityString(pUnit, szQuality, arrsize(szQuality), 0);

			UnitGetName(pUnit, pResultLine->m_szItemName, arrsize(pResultLine->m_szItemName), 0);
			pResultLine->m_nQuality = ItemGetQuality(pUnit);

			const int nLevel = UnitGetExperienceLevel(pUnit);
			pResultLine->m_nLevel = nLevel;
			pResultLine->m_pUnit = pUnit;

			if ((pLabel = UIComponentFindChildByName(pPanel, "search results item itembox")) != NULL)
			{
				UIComponentSetFocusUnit(pLabel, pUnit->unitid);
			}

			if ((pLabel = UIComponentFindChildByName(pPanel, "search results item name label")) != NULL)
			{
				UILabelSetText(pLabel, pResultLine->m_szItemName);
			}

			if ((pLabel = UIComponentFindChildByName(pPanel, "search results item quality label")) != NULL)
			{
				UILabelSetText(pLabel, szQuality);
			}

			if ((pLabel = UIComponentFindChildByName(pPanel, "search results item level label")) != NULL)
			{
				PIntToStr(szText, arrsize(szText), pResultLine->m_nLevel);
				UILabelSetText(pLabel, szText);
			}

			if ((pLabel = UIComponentFindChildByName(pPanel, "search results item price label")) != NULL)
			{
				PIntToStr(szText, arrsize(szText), pResultLine->m_nPrice);
				UILabelSetText(pLabel, szText);
			}

			if ((pLabel = UIComponentFindChildByName(pPanel, "search result buy button")) != NULL)
			{
				cCurrency playerCurrency = UnitGetCurrency( UIGetControlUnit() );
				const int nPlayerMoney = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );

				BOOL bEnableButton = TRUE;

				if (s_pCurResultSet == &s_SearchResults)
				{
					if (nPlayerMoney < pResultLine->m_nPrice)
					{
						bEnableButton = FALSE;
					}
					else if (!s_bBuyButtonEnabled)
					{
						bEnableButton = FALSE;
					}
				}

				UI_COMPONENT *pButtonLabel = UIComponentIterateChildren(pLabel, NULL, UITYPE_LABEL, FALSE);
				if (pButtonLabel)
				{
					if (s_pCurResultSet == &s_MyForSaleItems)
					{
						UILabelSetText(pButtonLabel, GlobalStringGet(GS_AUCTION_CANCEL_SALE_BUTTON));
					}
					else
					{
						UILabelSetText(pButtonLabel, GlobalStringGet(GS_AUCTION_BUY_BUTTON));
					}
				}

				UIComponentSetActive(pLabel, bEnableButton);
				UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
			}

			UIComponentSetVisible(pPanel, TRUE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResultUpdateUI(
	T_CACHE_PAGE * pCachePage)
{
	ASSERT_RETURN(pCachePage);

	for (int i=0; i<AUCTION_RESULT_PAGE_SIZE; ++i)
	{
		if (pCachePage->m_ResultLines[i].m_guid != INVALID_GUID)
		{
			UIAuctionSearchResultUpdateUI(pCachePage->m_ResultLines[i].m_guid);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchUpdatePageInfo(
	void)
{
	// enable/disable first/prev/next/last buttons appropriately

	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_BODY_PANEL);
	ASSERT_RETURN(pPanel);
	UI_COMPONENT *pComponent = NULL;

	if ((pComponent = UIComponentFindChildByName(pPanel, "search result first page button")) != NULL)
	{
		UIComponentSetActive(pComponent, s_pCurResultSet->m_nCurPage > 0);
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "search result prev page button")) != NULL)
	{
		UIComponentSetActive(pComponent, s_pCurResultSet->m_nCurPage > 0);
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "search result next page button")) != NULL)
	{
		UIComponentSetActive(pComponent, s_pCurResultSet->m_nCurPage+1 < s_pCurResultSet->m_nNumPages);
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "search result last page button")) != NULL)
	{
		UIComponentSetActive(pComponent, s_pCurResultSet->m_nCurPage+1 < s_pCurResultSet->m_nNumPages);
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

	if ((pComponent = UIComponentFindChildByName(pPanel, "search results page info")) != NULL)
	{
		if (s_pCurResultSet->m_nNumPages > 0)
		{
			UIComponentSetVisible(pComponent, TRUE);

			WCHAR szStringBuf[500], szNum[32];
			PStrCopy(szStringBuf, GlobalStringGet(GS_AUCTION_PAGE_N_OF_N), arrsize(szStringBuf));
			LanguageFormatIntString(szNum, arrsize(szNum), s_pCurResultSet->m_nCurPage+1);
			PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_NUMBER), szNum );
			LanguageFormatIntString(szNum, arrsize(szNum), s_pCurResultSet->m_nNumPages);
			PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_MAX), szNum );
			UILabelSetText(pComponent, szStringBuf);
		}
		else
		{
			UIComponentSetVisible(pComponent, FALSE);
		}
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionRequestResultPage(
	const int nPage)
{
	const int nNumResults = min(s_pCurResultSet->m_nNumResults - (nPage * AUCTION_RESULT_PAGE_SIZE), AUCTION_RESULT_PAGE_SIZE);
	ASSERT_RETURN(nNumResults > 0 || !s_pCurResultSet->m_nNumResults);
	ASSERT_RETURN(nNumResults <= s_pCurResultSet->m_nNumResults);

	if (s_pCurResultSet->m_nCurPage == nPage)
	{
		for (int i=0; i<AUCTION_RESULT_PAGE_SIZE; ++i)
		{
			if (s_pCurResultSet->m_pCachePages[0].m_ResultLines[i].m_pPanel)
			{
				UIComponentSetActive(s_pCurResultSet->m_pCachePages[0].m_ResultLines[i].m_pPanel, FALSE);
				UIComponentSetVisible(s_pCurResultSet->m_pCachePages[0].m_ResultLines[i].m_pPanel, FALSE);
				s_pCurResultSet->m_pCachePages[0].m_ResultLines[i].m_pPanel->m_qwData = INVALID_GUID;
				s_pCurResultSet->m_pCachePages[0].m_ResultLines[i].m_pPanel->m_dwData = 0;
			}
		}
	}

	T_CACHE_PAGE * pCachePage = sAuctionFindCachePage(nPage);

	if (pCachePage)
	{
		if (s_pCurResultSet->m_nCurPage == nPage)
		{
			UIAuctionSearchResultUpdateUI(pCachePage);
		}
	}
	else if (nNumResults)
	{
		pCachePage = sAuctionFindAvailableCachePage(nPage);
		ASSERT_RETURN(pCachePage);

		MSG_CCMD_AH_SEARCH_ITEMS_NEXT msgSearchItemsNext;
		msgSearchItemsNext.SearchIndex = AUCTION_RESULT_PAGE_SIZE * nPage;
		msgSearchItemsNext.SearchSize = nNumResults;
		msgSearchItemsNext.SearchOwnItem = (s_pCurResultSet == &s_MyForSaleItems);

		c_SendMessage(CCMD_AH_SEARCH_ITEMS_NEXT, &msgSearchItemsNext);

		s_pCurResultSet->m_nAwaitingResults += nNumResults;
	}

	if (s_pCurResultSet->m_nCurPage == nPage)
	{
		s_pCurResultSet->m_pCurCachePage = pCachePage;
		UIAuctionSearchUpdatePageInfo();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionRequestResultPagesBefore(
	const int nPage,
	const int nNumPages)
{
	for (int i=0; i<nNumPages; ++i)
	{
		int nThisPage = nPage-1-i;

		if ( nThisPage >= 0 )
		{
			UIAuctionRequestResultPage(nThisPage);
		}
		else
		{
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionRequestResultPagesAfter(
	const int nPage,
	const int nNumPages)
{
	for (int i=0; i<nNumPages; ++i)
	{
		int nThisPage = nPage+1+i;

		if ( nThisPage < s_pCurResultSet->m_nNumPages )
		{
			UIAuctionRequestResultPage(nThisPage);
		}
		else
		{
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionSearchResult(
	const int nResultSize)
{
	s_pCurResultSet->m_nNumResults = nResultSize;
	s_pCurResultSet->m_nNumPages = s_pCurResultSet->m_nNumResults ? ((s_pCurResultSet->m_nNumResults-1) / AUCTION_RESULT_PAGE_SIZE) + 1 : 0;

	sAuctionInitCache();

	if (AUCTION_USE_FREE_LIST)
	{
		sUIAuctionFreeListStart();
	}

	sAuctionFlushCache();

	s_bFirstPage = TRUE;
	s_pCurResultSet->m_nAwaitingResults = 0;

	if (s_pCurResultSet->m_nNumResults)
	{
		s_pCurResultSet->m_nCurPage = 0;
		UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
	}
	else if (s_pCurResultSet == &s_SearchResults)
	{
		UIShowGenericDialog(GlobalStringGet(GS_AUCTION_AUCTION_HOUSE), GlobalStringGet(GS_AUCTION_NO_SEARCH_RESULTS_FOUND));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResult(
	MSG_SCMD_AH_SEARCH_RESULT * msg)
{
	ASSERT_RETURN(msg);

	T_RESULT_SET * pOldResultSet = s_pCurResultSet;
	s_pCurResultSet = &s_SearchResults;

	sUIAuctionSearchResult(msg->ResultSize);

	s_pCurResultSet = pOldResultSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionPlayerItemSaleList(
	MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST * msg)
{
	ASSERT_RETURN(msg);

	T_RESULT_SET * pOldResultSet = s_pCurResultSet;
	s_pCurResultSet = &s_MyForSaleItems;

	sUIAuctionSearchResult(msg->ItemCount);

	s_pCurResultSet = pOldResultSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionSearchResultPageReceived(
	const int nResultCurIndex,
	const int nResultCurCount,
	PGUID * ItemGUIDs)
{
	const int nPage = nResultCurIndex / AUCTION_RESULT_PAGE_SIZE;
	T_CACHE_PAGE * pCachePage = sAuctionFindCachePage(nPage);

	ASSERT_RETURN(pCachePage);

	for (int i=0; i < nResultCurCount; ++i)
	{
		ASSERT_BREAK(i < AUCTION_SEARCH_MAX_RESULT);
		pCachePage->m_ResultLines[i].m_guid = ItemGUIDs[i];

		MSG_CCMD_AH_REQUEST_ITEM_INFO msgItemInfo;
		msgItemInfo.ItemGUID = ItemGUIDs[i];
		c_SendMessage(CCMD_AH_REQUEST_ITEM_INFO, &msgItemInfo);

		UNIT * pUnit = NULL;
		if ((pUnit = UnitGetByGuid(AppGetCltGame(), ItemGUIDs[i])) != NULL)
		{
			if (s_bUseFreeList)
			{
				sUIAuctionFreeListRemove(UnitGetId(pUnit));
			}
			if (pCachePage == s_pCurResultSet->m_pCurCachePage)
			{
				UIAuctionSearchResultUpdateUI(ItemGUIDs[i]);
			}
		}
		else
		{
			MSG_CCMD_AH_REQUEST_ITEM_BLOB msgItemBlob;
			msgItemBlob.ItemGUID = ItemGUIDs[i];
			c_SendMessage(CCMD_AH_REQUEST_ITEM_BLOB, &msgItemBlob);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResultNext(
	MSG_SCMD_AH_SEARCH_RESULT_NEXT * msg)
{
	T_RESULT_SET * pOldResultSet = s_pCurResultSet;
	s_pCurResultSet = msg->ResultOwnItem ? &s_MyForSaleItems : &s_SearchResults;

	sUIAuctionSearchResultPageReceived(
		msg->ResultCurIndex,
		msg->ResultCurCount,
		msg->ItemGUIDs);

	s_pCurResultSet = pOldResultSet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResultItemInfo(
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO * msg)
{
	T_RESULT_LINE * pResultLine = NULL;
	T_CACHE_PAGE * pCachePage = NULL;

	if ((pResultLine = sAuctionFindResultLineByGuid(msg->ItemGUID, FALSE, &pCachePage)) != NULL)
	{
		pResultLine->m_nPrice = msg->ItemPrice;
		pResultLine->m_szSellerName = msg->szSellerName;
	}

	if (pCachePage == s_pCurResultSet->m_pCurCachePage)
	{
		UIAuctionSearchResultUpdateUI(msg->ItemGUID);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionSearchResultItemBlob(
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB * msg)
{
	UNIT *pUnit = UnitGetByGuid(AppGetCltGame(), msg->ItemGUID);
	if (!pUnit)
	{
		DWORD dwThisDataSize = MSG_GET_BLOB_LEN( msg, ItemBlob );
		pUnit = c_CreateNewClientOnlyUnit(msg->ItemBlob, dwThisDataSize);
		ASSERT_RETURN(pUnit);
		UIItemInitInventoryGfx(pUnit, TRUE, FALSE);
	}

	T_CACHE_PAGE * pCachePage = NULL;
	sAuctionFindResultLineByGuid(msg->ItemGUID, FALSE, &pCachePage);

	if (pCachePage == s_pCurResultSet->m_pCurCachePage)
	{
		UIAuctionSearchResultUpdateUI(msg->ItemGUID);

		if (s_pCurResultSet->m_nAwaitingResults)
		{
			--s_pCurResultSet->m_nAwaitingResults;
			if (!s_pCurResultSet->m_nAwaitingResults)
			{
				if (s_bFirstPage)
				{
//					UIAuctionRequestResultPagesAfter(s_pCurResultSet->m_nCurPage, AUCTION_RESULT_CACHE_SIZE-2);
					UIAuctionRequestResultPagesAfter(s_pCurResultSet->m_nCurPage, AUCTION_PRELOAD_PAGES);
					if ((AUCTION_PRELOAD_PAGES) > 0)
					{
						UIAuctionRequestResultPage(s_pCurResultSet->m_nNumPages-1);
					}
					if (s_bUseFreeList)
					{
						sUIAuctionFreeListEnd();
					}
					s_bFirstPage = FALSE;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultFirstPageButtonOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(s_pCurResultSet->m_nCurPage > 0, UIMSG_RET_NOT_HANDLED);

	s_pCurResultSet->m_nCurPage = 0;
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
	UIAuctionRequestResultPagesAfter(s_pCurResultSet->m_nCurPage, AUCTION_PRELOAD_PAGES);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultPrevPageButtonOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(s_pCurResultSet->m_nCurPage > 0, UIMSG_RET_NOT_HANDLED);

	--s_pCurResultSet->m_nCurPage;
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
	UIAuctionRequestResultPagesBefore(s_pCurResultSet->m_nCurPage, AUCTION_PRELOAD_PAGES);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultNextPageButtonOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(s_pCurResultSet->m_nCurPage < s_pCurResultSet->m_nNumPages, UIMSG_RET_NOT_HANDLED);

	++s_pCurResultSet->m_nCurPage;
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
	UIAuctionRequestResultPagesAfter(s_pCurResultSet->m_nCurPage, AUCTION_PRELOAD_PAGES);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultLastPageButtonOnClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(s_pCurResultSet->m_nCurPage < s_pCurResultSet->m_nNumPages, UIMSG_RET_NOT_HANDLED);

	s_pCurResultSet->m_nCurPage = (s_pCurResultSet->m_nNumPages-1);
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
	UIAuctionRequestResultPagesBefore(s_pCurResultSet->m_nCurPage, AUCTION_PRELOAD_PAGES);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionOnPostInactivate(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	sAuctionFlushCache();
	if (s_bUseFreeList)
	{
		sUIAuctionFreeListEnd();
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchItemBoxOnRClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UNIT *pTarget = UIComponentGetFocusUnit(component);
	ASSERT_RETVAL(pTarget, UIMSG_RET_NOT_HANDLED);

	UIPlaceMod(pTarget, NULL);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultItemOnLClick(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pItemBox = UIComponentIterateChildren(component, NULL, UITYPE_ITEMBOX, TRUE);
	ASSERT_RETVAL(pItemBox, UIMSG_RET_NOT_HANDLED);
	UNIT *pTarget = UIComponentGetFocusUnit(pItemBox);

	if (pTarget)
	{
		GameSetDebugUnit(AppGetCltGame(), pTarget);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAuctionUpdateSellButtonState(
	void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	if (pComponent)
	{
		UI_COMPONENT *pButton;
		if ((pButton = UIComponentFindChildByName(pComponent, "auction sell button")) != NULL)
		{
			if (s_nSellItem == INVALID_GUID || s_nSellMoney == 0)
			{
				UIComponentSetActive(pButton, FALSE);
			}
			else
			{
				cCurrency playerCurrency = UnitGetCurrency( UIGetControlUnit() );
				const int nPlayerMoney = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );
				const int nSellCost = sUIAuctionGetCostToSell(s_nSellItem, s_nSellMoney);
				const int nMinSellPrice = sUIAuctionGetMinSellPrice(s_nSellItem);

				UIComponentSetActive(pButton, nSellCost <= nPlayerMoney && s_nSellMoney >= nMinSellPrice);

				if (!pButton->m_szTooltipText)
				{
					pButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
				}

				if (s_nSellMoney < nMinSellPrice)
				{
					// set tooltip to "The minimum asking price for this item is n."

					WCHAR szStringBuf[256], szNum[32];
					int nMinSellPrice = sUIAuctionGetMinSellPrice(s_nSellItem);
					LanguageFormatIntString(szNum, arrsize(szNum), (int)nMinSellPrice);
					PStrCopy(szStringBuf, GlobalStringGet(GS_AUCTION_MINIMUM_SELL_PRICE_IS), arrsize(szStringBuf));
					PStrReplaceToken( szStringBuf, arrsize(szStringBuf), StringReplacementTokensGet(SR_MONEY), szNum );
					PStrCopy(pButton->m_szTooltipText, szStringBuf, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
				}
				else if (nSellCost > nPlayerMoney)
				{
					PStrCopy(pButton->m_szTooltipText, GlobalStringGet(GS_AUCTION_NOT_ENOUGH_MONEY), UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
				}
				else
				{
					PStrCopy(pButton->m_szTooltipText, L"", UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
				}
			}
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateSellCostLabel(
	void)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	if (pComponent)
	{
		UI_COMPONENT *pLabel;
		if ((pLabel = UIComponentFindChildByName(pComponent, "price to list label")) != NULL)
		{
			const int nSellCost = sUIAuctionGetCostToSell(s_nSellItem, s_nSellMoney);
			if (nSellCost > 0)
			{
				if( AppIsHellgate() )
				{
					WCHAR szNum[32];
					LanguageFormatIntString(szNum, arrsize(szNum), nSellCost);
					UILabelSetText(pLabel, szNum);
					UIComponentSetVisible(pLabel, TRUE);
					UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
				}
				else
				{
					cCurrency nCost( nSellCost, 0 );
					WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];		
					const WCHAR *uszFormat = GlobalStringGet(GS_PRICE_COPPER_SILVER_GOLD);
					if (uszFormat)
					{
						PStrCopy(uszTextBuffer, uszFormat, arrsize(uszTextBuffer));
					}
					const int MAX_MONEY_STRING = 128;
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

					UILabelSetText( pLabel, (nCost.IsZero() == FALSE)?uszTextBuffer:L"" );
				}

			}
			else
			{
				UIComponentSetVisible(pLabel, FALSE);
			}
		}
	}

	UIAuctionUpdateSellButtonState();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionSetMoney(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	// clean up money string (leading zeros for instance)
	int nMoney = c_GetMoneyValueInComponent( pComponent );

	if (s_nSellItem != INVALID_GUID)
	{
		int nMinSellPrice = sUIAuctionGetMinSellPrice(s_nSellItem);
		if (nMinSellPrice != nMoney)
		{
			c_SetMoneyValueInComponent( pComponent, nMinSellPrice );
			nMoney = nMinSellPrice;
		}
	}

	s_nSellMoney = nMoney > 0 ? nMoney : 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionSetMoney(
	void)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
	ASSERT_RETURN(pPanel);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pPanel, "asking price entry edit");
	ASSERT_RETURN(pComponent);

	sUIAuctionSetMoney(pComponent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionOutboxSlotOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_INVSLOT *pInvSlot = UICastToInvSlot(component);
	int location = (int)lParam;

	if (location == pInvSlot->m_nInvLocation)
	{
		UNITID unitid = (UNITID)wParam;
		UNIT * pContainer = UnitGetById(AppGetCltGame(), unitid);
		ASSERT_RETVAL(pContainer, UIMSG_RET_NOT_HANDLED);

		UNIT * pUnit = UnitInventoryGetByLocation(pContainer, location);
		if (pUnit)
		{
			s_nSellItem = UnitGetGuid(pUnit);
		}
		else
		{
			s_nSellItem = INVALID_GUID;
		}

		sUIAuctionSetMoney();
		sUIUpdateSellCostLabel();
	}

	return UIInvSlotOnInventoryChange(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSellPriceOnChar(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ))
	{
		WCHAR ucCharacter = (WCHAR)wParam;
		if (UIEditCtrlOnKeyChar( pComponent, nMessage, wParam, lParam ) != UIMSG_RET_NOT_HANDLED)
		{
			if (ucCharacter != VK_RETURN)
			{
				int nMoney = c_GetMoneyValueInComponent( pComponent );
				s_nSellMoney = nMoney > 0 ? nMoney : 0;

				sUIUpdateSellCostLabel();
			}

			return UIMSG_RET_HANDLED_END_PROCESS;  // input used
		}
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSellButtonOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL( s_nSellItem != INVALID_GUID, UIMSG_RET_NOT_HANDLED );
	ASSERT_RETVAL( s_nSellMoney > 0, UIMSG_RET_NOT_HANDLED );

	// send a "sell" message

	MSG_CCMD_AH_SELL_ITEM msgSell;  
	msgSell.ItemGUID = s_nSellItem; 
	msgSell.ItemPrice = s_nSellMoney;  
	c_SendMessage(CCMD_AH_SELL_ITEM, &msgSell); 

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionCallbackWithdrawItem( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	ASSERT_RETURN(pUserData);

	MSG_CCMD_AH_WITHDRAW_ITEM withdrawMsg;
	withdrawMsg.ItemGUID = *((PGUID*)pUserData);
	c_SendMessage(CCMD_AH_WITHDRAW_ITEM, &withdrawMsg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSearchResultBuyCancelButtonOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (s_pCurResultSet == &s_SearchResults)
	{
		MSG_CCMD_AH_BUY_ITEM buyMsg;
		buyMsg.ItemGUID = (PGUID)pComponent->m_pParent->m_qwData;
		buyMsg.ItemPrice = (DWORD)pComponent->m_pParent->m_dwData;
		c_SendMessage(CCMD_AH_BUY_ITEM, &buyMsg);

		UIAuctionDisableBuyButtons();
	}
	else
	{
		DIALOG_CALLBACK tCallbackOK;
		DialogCallbackInit( tCallbackOK );
		tCallbackOK.pfnCallback = sUIAuctionCallbackWithdrawItem;
		tCallbackOK.pCallbackData = &pComponent->m_pParent->m_qwData;

		UIShowGenericDialog(
			GlobalStringGet(GS_AUCTION_AUCTION_HOUSE),
			GlobalStringGet(GS_AUCTION_REMOVE_ARE_YOU_SURE),
			TRUE,
			&tCallbackOK);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionRequestMySoldItemList(
	void)
{
	MSG_CCMD_AH_RETRIEVE_SALE_ITEMS msgRetrieve;
	c_SendMessage(CCMD_AH_RETRIEVE_SALE_ITEMS, &msgRetrieve); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionBuyTabOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	s_pCurResultSet = &s_SearchResults;

	sAuctionInitCache();

	UIAuctionSearchUpdatePageInfo();
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
//	UIAuctionSearchResultUpdateUI(&s_pCurResultSet->m_pCachePages[s_pCurResultSet->m_nCurPage]);

	return UIButtonOnButtonDown(pComponent, nMessage, wParam, lParam );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAuctionSellTabOnClick(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	s_pCurResultSet = &s_MyForSaleItems;

	sAuctionInitCache();

	UIAuctionSearchUpdatePageInfo();
	UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
//	UIAuctionSearchResultUpdateUI(&s_pCurResultSet->m_pCachePages[s_pCurResultSet->m_nCurPage]);

	sUIAuctionRequestMySoldItemList();

	return UIButtonOnButtonDown(pComponent, nMessage, wParam, lParam );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIAuctionSetBuyButtons(
									const BOOL bButtonState)
{
	if (s_bBuyButtonEnabled != bButtonState)
	{
		s_bBuyButtonEnabled = bButtonState;
		if (s_pCurResultSet == &s_SearchResults)
		{
			UIAuctionRequestResultPage(s_pCurResultSet->m_nCurPage);
		}
	}
}

void UIAuctionDisableBuyButtons(
								void)
{
	sUIAuctionSetBuyButtons(FALSE);
}

void UIAuctionEnableBuyButtons(
							   void)
{
	sUIAuctionSetBuyButtons(TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesAuction(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name									function pointer
		{ "UIToggleAuctionHouse",							UIToggleAuctionHouse },
		{ "UIAuctionSearchFactionComboOnPostCreate",		UIAuctionSearchFactionComboOnPostCreate },
		{ "UIAuctionSearchFactionComboOnPostActivate",		UIAuctionSearchFactionComboOnPostActivate },
		{ "UIAuctionSearchItemTypeComboOnPostCreate",		UIAuctionSearchItemTypeComboOnPostCreate },
		{ "UIAuctionSearchQualityComboOnPostCreate",		UIAuctionSearchQualityComboOnPostCreate },
		{ "UIAuctionSearchUnitTypeOnChange",				UIAuctionSearchUnitTypeOnChange },
		{ "UIAuctionSearchFactionComboOnChange",			UIAuctionSearchFactionComboOnChange },
		{ "UIAuctionSearchTypeComboOnChange",				UIAuctionSearchTypeComboOnChange },
		{ "UIAuctionSearchButtonOnClick",					UIAuctionSearchButtonOnClick },
		{ "UIAuctionSearchQualityOnClick",					UIAuctionSearchQualityOnClick },
		{ "UIAuctionSearchLevelOnClick",					UIAuctionSearchLevelOnClick },
		{ "UIAuctionSearchPriceOnClick",					UIAuctionSearchPriceOnClick },
		{ "UIAuctionSearchResultFirstPageButtonOnClick",	UIAuctionSearchResultFirstPageButtonOnClick },
		{ "UIAuctionSearchResultPrevPageButtonOnClick",		UIAuctionSearchResultPrevPageButtonOnClick },
		{ "UIAuctionSearchResultNextPageButtonOnClick",		UIAuctionSearchResultNextPageButtonOnClick },
		{ "UIAuctionSearchResultLastPageButtonOnClick",		UIAuctionSearchResultLastPageButtonOnClick },
		{ "UIAuctionOnPostInactivate",						UIAuctionOnPostInactivate },
		{ "UIAuctionSearchItemBoxOnRClick",					UIAuctionSearchItemBoxOnRClick },
		{ "UIAuctionSearchResultItemOnLClick",				UIAuctionSearchResultItemOnLClick },
		{ "UIAuctionOutboxSlotOnInventoryChange",			UIAuctionOutboxSlotOnInventoryChange },
		{ "UIAuctionSellPriceOnChar",						UIAuctionSellPriceOnChar },
		{ "UIAuctionSellButtonOnClick",						UIAuctionSellButtonOnClick },
		{ "UIAuctionSearchResultBuyCancelButtonOnClick",	UIAuctionSearchResultBuyCancelButtonOnClick },
		{ "UIAuctionBuyTabOnClick",							UIAuctionBuyTabOnClick },
		{ "UIAuctionSellTabOnClick",						UIAuctionSellTabOnClick }
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif
