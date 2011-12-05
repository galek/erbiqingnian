#ifndef _S_AUCTIONNETWORK_H_
#define _S_AUCTIONNETWORK_H_

struct AUCTION_ITEMSALE_INFO
{
	PGUID idEmail;
	PGUID idItem;
	PGUID idSeller;
	GAMEID idGame;
	DWORD dwPrice;
	DWORD dwFee;
	DWORD dwItemVariant;
	DWORD dwItemQuality;
	DWORD dwItemLevel;
	DWORD dwItemType;
	DWORD dwItemClass;
	WCHAR szSellerName[MAX_CHARACTER_NAME];

#if ISVERSION(DEBUG_VERSION)
	CHAR szItemTypeName[MAX_CHARACTER_NAME];
#endif

	BYTE* pItemBlob;
	DWORD dwItemBlobLen;
	MEMORYPOOL* pPool;
	CCriticalSection cs;

	AUCTION_ITEMSALE_INFO() : cs(TRUE)
	{
		idEmail = INVALID_GUID;
		idItem = INVALID_GUID;
		idSeller = INVALID_GUID;
		pItemBlob = NULL;
	}
};

BOOL UtilGameAuctionItemSaleHandleEmailMetaData(
	GameServerContext* pSvrCtx,
	GAMEID idGame,
	PGUID idEmail,
	PGUID idItem,
	PGUID idSeller,
	LPCWSTR szSellerName);

BOOL UtilGameAuctionItemSaleHandleEmailData(
	GameServerContext* pSvrCtx,
	PGUID idEmail,
	LPCWSTR szEmailTitle,
	LPCWSTR szEmailBody);

BOOL UtilGameAuctionItemSaleHandleItem(
	GameServerContext* pSvrCtx,
	GAME* pGame,
	UNIT* pItem);

BOOL UtilGameAuctionItemSaleRemoveItemByItemID(
	GameServerContext* pSvrCtx,
	PGUID idItem);

/*
BOOL UtilGameHandleItemForAuctionHouse(
	GAME* pGame,
	CLIENTSYSTEM* pClientSystem,
	APPCLIENTID idAppClient,
	UNIT_COLLECTION_ID idCollection);
*/

DWORD x_AuctionGetCostToSell(
	const int nSellPrice,
	const DWORD dwFeeRate);

DWORD x_AuctionGetMinSellPrice(
	UNIT * pItem);

#endif
