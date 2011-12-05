//----------------------------------------------------------------------------
// uix_auction.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_AUCTION_H_
#define _UIX_AUCTION_H_


//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "uix_components.h"
#include "s_message.h"

void UIAuctionInit(
	void);

void UIToggleAuctionHouse(
	void);

void UIAuctionUpdateSellButtonState(
	void);

void UIAuctionDisableBuyButtons(
	void);

void UIAuctionEnableBuyButtons(
	void);

void InitComponentTypesAuction(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

//----------------------------------------------------------------------------
// Callback functions for network messages
//----------------------------------------------------------------------------

void UIAuctionInfo(
	MSG_SCMD_AH_INFO * msg);

void UIAuctionError(
	MSG_SCMD_AH_ERROR * msg);

void UIAuctionSearchResult(
	MSG_SCMD_AH_SEARCH_RESULT * msg);

void UIAuctionSearchResultNext(
	MSG_SCMD_AH_SEARCH_RESULT_NEXT * msg);

void UIAuctionPlayerItemSaleList(
	MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST * msg);

void UIAuctionSearchResultItemInfo(
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO * msg);

void UIAuctionSearchResultItemBlob(
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB * msg);

#endif
