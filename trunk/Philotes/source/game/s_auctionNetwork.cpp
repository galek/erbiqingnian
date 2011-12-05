#include "stdafx.h"

#include "Economy.h"
#include "Currency.h"

#if ISVERSION(SERVER_VERSION)

#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "treasure.h"
#include "unitfile.h"
#include "s_network.h"
#include "c_message.h"
#include "GameServer.h"
#include "ServerRunnerAPI.h"
#include "utilitygame.h"
#include "s_auctionNetwork.h"
#include "GameAuctionCommunication.h"
#include "GameChatCommunication.h"
#include "dbunit.h"
#include "items.h"

#include <hash_map>

// Defined in clients.cpp
struct CLIENTSYSTEM;

// I should probably put these somewhere else
/*
struct AUCTION_EMAIL_SALE_INFO
{
	// Info from email metadata
	PGUID	idEmail;
	PGUID	idSender;
	WCHAR	szSenderName[MAX_CHARACTER_NAME];
	
	// Info from UNIT
	DWORD	dwQuality;
	DWORD	dwLevel;
	DWORD	dwType;
	BYTE*	pItemBlob;

#if ISVERSION(DEBUG_VERSION)
	CHAR	szItemTypeName[MAX_CHARACTER_NAME];
#endif

	// Info from email data
	DWORD	dwPrice;
	DWORD	dwItemVariant;
};


stdext::hash_map<PGUID, AUCTION_EMAIL_SALE_INFO*> mapItemSaleInfo;
CCriticalSection csItemSaleInfo(TRUE);
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AUCTION_ITEMSALE_INFO* sUtilGameGetOrCreateItemSaleInfoByEmailID(
	GameServerContext* pSvrCtx,
	PGUID idEmail)
{
	ASSERT_RETNULL(pSvrCtx != NULL);
	ASSERT_RETNULL(idEmail != INVALID_GUID);

	CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);
	for (UINT32 i = 0; i < pSvrCtx->m_listAuctionItemSales.Count(); i++) {
		if (pSvrCtx->m_listAuctionItemSales[i].idEmail == idEmail) {
			pSvrCtx->m_listAuctionItemSales[i].cs.Enter();
			return &pSvrCtx->m_listAuctionItemSales[i];
		}
	}
	for (UINT32 i = 0; i < pSvrCtx->m_listAuctionItemSales.Count(); i++) {
		if (pSvrCtx->m_listAuctionItemSales[i].idEmail == INVALID_GUID) {
			pSvrCtx->m_listAuctionItemSales[i].idEmail = idEmail;
			pSvrCtx->m_listAuctionItemSales[i].cs.Enter();
			return &pSvrCtx->m_listAuctionItemSales[i];
		}
	}
	UINT32 index = ArrayAddItem(pSvrCtx->m_listAuctionItemSales, AUCTION_ITEMSALE_INFO());
	pSvrCtx->m_listAuctionItemSales[index].idEmail = idEmail;
	pSvrCtx->m_listAuctionItemSales[index].cs.Enter();
	return &pSvrCtx->m_listAuctionItemSales[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AUCTION_ITEMSALE_INFO* sUtilGameGetItemSaleInfoByEmailID(
	GameServerContext* pSvrCtx,
	PGUID idEmail)
{
	ASSERT_RETNULL(pSvrCtx != NULL);
	ASSERT_RETNULL(idEmail != INVALID_GUID);

	CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);
	for (UINT32 i = 0; i < pSvrCtx->m_listAuctionItemSales.Count(); i++) {
		if (pSvrCtx->m_listAuctionItemSales[i].idEmail == idEmail) {
			pSvrCtx->m_listAuctionItemSales[i].cs.Enter();
			return &pSvrCtx->m_listAuctionItemSales[i];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AUCTION_ITEMSALE_INFO* sUtilGameGetItemSaleInfoByItemID(
	GameServerContext* pSvrCtx,
	PGUID idItem)
{
	ASSERT_RETNULL(pSvrCtx != NULL);
	ASSERT_RETNULL(idItem != INVALID_GUID);

	CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);
	for (UINT32 i = 0; i < pSvrCtx->m_listAuctionItemSales.Count(); i++) {
		if (pSvrCtx->m_listAuctionItemSales[i].idItem == idItem) {
			pSvrCtx->m_listAuctionItemSales[i].cs.Enter();
			return &pSvrCtx->m_listAuctionItemSales[i];
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilGameAuctionItemSaleRemoveItemByItemID(
	GameServerContext* pSvrCtx,
	PGUID idItem)
{
	ASSERT_RETFALSE(pSvrCtx != NULL);
	ASSERT_RETFALSE(idItem != INVALID_GUID);

	CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);

	for (UINT32 i = 0; i < pSvrCtx->m_listAuctionItemSales.Count(); i++) {
		if (pSvrCtx->m_listAuctionItemSales[i].idItem == idItem) {
			CSAutoLock autoLock(&pSvrCtx->m_listAuctionItemSales[i].cs);
			if (pSvrCtx->m_listAuctionItemSales[i].pItemBlob) {
				FREE(pSvrCtx->m_listAuctionItemSales[i].pPool, pSvrCtx->m_listAuctionItemSales[i].pItemBlob);
				pSvrCtx->m_listAuctionItemSales[i].pItemBlob = NULL;
			}
			pSvrCtx->m_listAuctionItemSales[i].idEmail = INVALID_GUID;
			pSvrCtx->m_listAuctionItemSales[i].idItem = INVALID_GUID;
			pSvrCtx->m_listAuctionItemSales[i].idSeller = INVALID_GUID;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilGameAuctionItemSaleHandleItem(
	GameServerContext* pSvrCtx,
	GAME* pGame,
	UNIT* pItem)
{
	ASSERT_RETFALSE(pSvrCtx != NULL);
	ASSERT_RETFALSE(pGame != NULL);
	ASSERT_RETFALSE(pItem != NULL);

	PGUID idItem = UnitGetGuid(pItem);
	AUCTION_ITEMSALE_INFO* pItemSaleInfo = sUtilGameGetItemSaleInfoByItemID(pSvrCtx, idItem);
	ASSERT_RETFALSE(pItemSaleInfo);

	CSAutoLeave autoLeave(&pItemSaleInfo->cs);
	pItemSaleInfo->dwItemType = UnitGetType(pItem);
	pItemSaleInfo->dwItemClass = UnitGetClass(pItem);
	pItemSaleInfo->dwItemLevel = UnitGetExperienceLevel(pItem);
	pItemSaleInfo->dwItemQuality = 0;
	const ITEM_QUALITY_DATA* pQualityData = ItemQualityGetData(pGame, ItemGetQuality(pItem));
	if (pQualityData != NULL) {
		pItemSaleInfo->dwItemQuality = pQualityData->nQualityLevel;
	}

	// Calculate equivalent types
	SIMPLE_DYNAMIC_ARRAY<int> nEquivTypes;
	ArrayInit(nEquivTypes, GameGetMemPool(pGame), 8);
	ASSERT_RETFALSE(UnitTypeGetAllEquivTypes(UnitGetType(pItem), nEquivTypes));

#if ISVERSION(DEBUG_VERSION)
	const UNITTYPE_DATA* pUnitTypeData = UnitTypeGetData(UnitGetType(pItem));
	if (pUnitTypeData != NULL) {
		PStrCopy(pItemSaleInfo->szItemTypeName, pUnitTypeData->szName, MAX_CHARACTER_NAME);
	} else {
		pItemSaleInfo->szItemTypeName[0] = '\0';
	}
#endif

	BYTE pDataBuffer[UNITFILE_MAX_SIZE_SINGLE_UNIT];
	DWORD dwDataBufferSize = PlayerSaveToBuffer(
		pGame,
		NULL,
		pItem,
		UNITSAVEMODE_CLIENT_ONLY,
		pDataBuffer,
		UNITFILE_MAX_SIZE_SINGLE_UNIT);
	ASSERT_RETFALSE(dwDataBufferSize > 0);

	pItemSaleInfo->pPool = GameGetMemPool(pGame);
	pItemSaleInfo->dwItemBlobLen = dwDataBufferSize;
	pItemSaleInfo->pItemBlob = (BYTE*)MALLOC(pItemSaleInfo->pPool, dwDataBufferSize);
	ASSERT_RETFALSE(pItemSaleInfo->pItemBlob != NULL);

	MemoryCopy(pItemSaleInfo->pItemBlob, dwDataBufferSize, pDataBuffer, dwDataBufferSize);

	GAME_AUCTION_REQUEST_LIST_ITEM_FOR_SALE_MSG msgSellItem;
	msgSellItem.PlayerGUID = pItemSaleInfo->idSeller;
	msgSellItem.GameID = pItemSaleInfo->idGame;
	msgSellItem.ItemPrice = pItemSaleInfo->dwPrice;
	msgSellItem.ItemVariant = pItemSaleInfo->dwItemVariant;
	msgSellItem.itemInfoData.ItemGuid = pItemSaleInfo->idItem;
	msgSellItem.itemInfoData.SellerGuid = pItemSaleInfo->idSeller;
	msgSellItem.itemInfoData.ItemQuality = pItemSaleInfo->dwItemQuality;
	msgSellItem.itemInfoData.ItemLevel = pItemSaleInfo->dwItemLevel;
	msgSellItem.itemInfoData.ItemType = pItemSaleInfo->dwItemType;
	msgSellItem.itemInfoData.ItemClass = pItemSaleInfo->dwItemClass;
	msgSellItem.itemInfoData.ItemPrice = pItemSaleInfo->dwPrice;
	msgSellItem.itemInfoData.ItemVariant = pItemSaleInfo->dwItemVariant;

	UINT32 i = 0;
	for (i = 0; i < nEquivTypes.Count() && i < MAX_ITEM_EQUIV_TYPES; i++) {
		msgSellItem.itemInfoData.ItemEquivTypes[i] = nEquivTypes[i];
	}
	for (; i < MAX_ITEM_EQUIV_TYPES; i++) {
		msgSellItem.itemInfoData.ItemEquivTypes[i] = INVALID_AUCTION_ITEM_TYPE;
	}

#if ISVERSION(DEBUG_VERSION)
	PStrCopy(msgSellItem.itemInfoData.ItemName, pItemSaleInfo->szItemTypeName, MAX_CHARACTER_NAME);
#endif

	GameServerToAuctionServerSendMessage(&msgSellItem, GAME_AUCTION_REQUEST_LIST_ITEM_FOR_SALE);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilGameAuctionItemSaleHandleEmailMetaData(
	GameServerContext* pSvrCtx,
	GAMEID idGame,
	PGUID idEmail,
	PGUID idItem,
	PGUID idSeller,
	LPCWSTR szSellerName)
{
	ASSERT_RETFALSE(pSvrCtx != NULL);
	ASSERT_RETFALSE(idEmail != INVALID_GUID);
	ASSERT_RETFALSE(idItem != INVALID_GUID);
	ASSERT_RETFALSE(idSeller != INVALID_GUID);
	ASSERT_RETFALSE(szSellerName != NULL);

	AUCTION_ITEMSALE_INFO* pItemSaleInfo = sUtilGameGetOrCreateItemSaleInfoByEmailID(pSvrCtx, idEmail);
	ASSERT_RETFALSE(pItemSaleInfo != NULL);

	CSAutoLeave autoLeave(&pItemSaleInfo->cs);
	pItemSaleInfo->idItem = idItem;
	pItemSaleInfo->idSeller = idSeller;
	pItemSaleInfo->idGame = idGame;
	PStrCopy(pItemSaleInfo->szSellerName, szSellerName, MAX_CHARACTER_NAME);
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilGameAuctionItemSaleHandleEmailData(
	GameServerContext* pSvrCtx,
	PGUID idEmail,
	LPCWSTR szEmailTitle,
	LPCWSTR szEmailBody)
{
	ASSERT_RETFALSE(pSvrCtx != NULL);
	ASSERT_RETFALSE(szEmailTitle != NULL);
	ASSERT_RETFALSE(szEmailBody != NULL);

	AUCTION_ITEMSALE_INFO* pItemSaleInfo = sUtilGameGetItemSaleInfoByEmailID(pSvrCtx, idEmail);
	ASSERT_RETFALSE(pItemSaleInfo != NULL);

	CSAutoLeave autoLeave(&pItemSaleInfo->cs);
	ASSERT_RETFALSE(swscanf_s(szEmailBody, L"%x %x", &pItemSaleInfo->dwPrice, &pItemSaleInfo->dwItemVariant) == 2);
	return TRUE;
}


/*
BOOL UtilGameHandleEmailMetaDataForAuction(
	MEMORYPOOL* pPool,
	MSG_APPCMD_EMAIL_METADATA* pMetaData)
{
	ASSERT_RETFALSE(pMetaData != NULL);
	ASSERT_RETFALSE(pMetaData->eMessageStatus == EMAIL_STATUS_ACCEPTED);

	CCriticalSection autoLock(&csItemSaleInfo);
	AUCTION_EMAIL_SALE_INFO* pSaleInfo = mapItemSaleInfo[pMetaData->idEmailMessageId];
	ASSERT_RETFALSE(pSaleInfo == NULL);

	pSaleInfo = (AUCTION_EMAIL_SALE_INFO*)MALLOCZ(pPool, sizeof(AUCTION_EMAIL_SALE_INFO));
	ASSERT_RETFALSE(pSaleInfo != NULL);

	mapItemSaleInfo[pMetaData->idEmailMessageId] = pSaleInfo;

	pSaleInfo->idEmail = pMetaData->idEmailMessageId;
	pSaleInfo->idSender = pMetaData->idSenderCharacterId;
	PStrCopy(pSaleInfo->szSenderName, pMetaData->wszSenderCharacterName, MAX_CHARACTER_NAME);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UtilGameHandleEmailDataForAuction(
	MEMORYPOOL* pPool,
	MSG_APPCMD_ACCEPTED_EMAIL_DATA* pEmailData)
{
	ASSERT_RETFALSE(pEmailData != NULL);

	CCriticalSection autoLock(&csItemSaleInfo);
	AUCTION_EMAIL_SALE_INFO* pSaleInfo = mapItemSaleInfo[pEmailData->idEmailMessageId];
	ASSERT_RETFALSE(pSaleInfo != NULL);
	ASSERT_GOTO(pSaleInfo->idEmail == pEmailData->idEmailMessageId, _err);

	ASSERT_GOTO(swscanf_s(pEmailData->wszEmailBody, L"%x %x", &pSaleInfo->dwPrice, &pSaleInfo->dwItemVariant) == 2, _err);




	return TRUE;

_err:
	mapItemSaleInfo.erase(pEmailData->idEmailMessageId);
	return FALSE;
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_ITEM_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_RANDOM_ITEM_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	MSG_APPCMD_GENERATE_RANDOM_ITEM msg_;
	SERVER_MAILBOX* pMailBox = NULL;
	GAMEID idUtilGame = INVALID_ID;

	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_GOTO(pSvrCtx != NULL, _err);

	idUtilGame = UtilityGameGetGameId(pSvrCtx);
	ASSERT_GOTO(idUtilGame != INVALID_ID, _err);

	pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, idUtilGame);
	ASSERT_GOTO(pMailBox != NULL, _err);

	ASSERT_GOTO(SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msg_, APPCMD_GENERATE_RANDOM_ITEM), _err);

	return;

_err:
	GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED_MSG msgFail;
	GameServerToAuctionServerSendMessage(&msgFail, GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED);
	return;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_RANDOM_ITEM_BLOB_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	SERVER_MAILBOX* pMailBox = NULL;
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_GOTO(pSvrCtx != NULL, _err);

	AUCTION_GAME_REQUEST_RANDOM_ITEM_BLOB_MSG* pMsg = (AUCTION_GAME_REQUEST_RANDOM_ITEM_BLOB_MSG*)msg;
	ASSERT_GOTO(pMsg != NULL, _err);

	pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_GOTO(pMailBox != NULL, _err);

	{
		MSG_APPCMD_GENERATE_RANDOM_ITEM_GETBLOB msgGetBlob;
		msgGetBlob.ItemGUID = pMsg->ItemGUID;
		ASSERT_GOTO(SvrNetPostFakeClientRequestToMailbox(
			pMailBox, ServiceUserId(USER_SERVER, 0, 0),
			&msgGetBlob, APPCMD_GENERATE_RANDOM_ITEM_GETBLOB), _err);
	}

	return;

_err:
	GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED_MSG msgFail;
	GameServerToAuctionServerSendMessage(&msgFail, GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED);
	return;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	AUCTION_GAME_RESPONSE_INFO_MSG* pMsg = (AUCTION_GAME_RESPONSE_INFO_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);
	pSvrCtx->m_dwAuctionHouseFeeRate = pMsg->dwFeeRate;
	pSvrCtx->m_dwAuctionHouseMaxItemSaleCount = pMsg->dwMaxItemSaleCount;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_ERROR_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_ERROR_MSG* pMsg = (AUCTION_GAME_RESPONSE_ERROR_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	if (pMsg->ErrorCode == AH_ERROR_SELL_ITEM_INTERNAL_ERROR ||
		pMsg->ErrorCode == AH_ERROR_SELL_ITEM_SUCCESS) {

		AUCTION_ITEMSALE_INFO* pItemSaleInfo = sUtilGameGetItemSaleInfoByItemID(pSvrCtx, pMsg->ItemGUID);
		if (pItemSaleInfo != NULL) {
			// Deletes the email with the attached item
			CHAT_REQUEST_MSG_EMAIL_ACTION msgDeleteEmail;
			msgDeleteEmail.idAccountId = GameServerGetAuctionHouseAccountID(pSvrCtx);
			msgDeleteEmail.idCharacterId = GameServerGetAuctionHouseCharacterID(pSvrCtx);
			msgDeleteEmail.idTargetGameId = UtilityGameGetGameId(pSvrCtx);
			msgDeleteEmail.idEmailMessageId = pItemSaleInfo->idEmail;
			GameServerToChatServerSendMessage(&msgDeleteEmail, GAME_REQUEST_DELETE_MESSAGE);

			// the fields for internal_error and success message can be invalid
			if (pMsg->PlayerGUID == INVALID_GUID) {
				pMsg->PlayerGUID = pItemSaleInfo->idSeller;
			}
			if (pMsg->GameID == INVALID_GAMEID) {
				pMsg->GameID = pItemSaleInfo->idGame;
			}
		
			pItemSaleInfo->cs.Leave();
		}
		UtilGameAuctionItemSaleRemoveItemByItemID(pSvrCtx, pMsg->ItemGUID);

	}

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);

	MSG_APPCMD_AH_ERROR msgError;
	msgError.PlayerGUID = pMsg->PlayerGUID;
	msgError.ErrorCode = pMsg->ErrorCode;
	msgError.ItemGUID = pMsg->ItemGUID;
	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgError, APPCMD_AH_ERROR);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_SEARCH_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_SEARCH_RESULT_MSG* pMsg = (AUCTION_GAME_RESPONSE_SEARCH_RESULT_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT msgResult;
	msgResult.PlayerGUID = pMsg->PlayerGUID;
	msgResult.ResultSize = pMsg->ResultSize;
	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgResult, APPCMD_AH_SEARCH_RESULT);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_SEARCH_RESULT_NEXT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_SEARCH_RESULT_NEXT_MSG* pMsg = (AUCTION_GAME_RESPONSE_SEARCH_RESULT_NEXT_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_NEXT msgResult;
	msgResult.PlayerGUID = pMsg->PlayerGUID;
	msgResult.ResultSize = pMsg->ResultSize;
	msgResult.ResultCurIndex = pMsg->ResultCurIndex;
	msgResult.ResultCurCount = pMsg->ResultCurCount;
	msgResult.ResultOwnItem = pMsg->ResultOwnItem;

	for (UINT32 i = 0; i < msgResult.ResultCurCount; i++) {
		msgResult.ItemGUIDs[i] = pMsg->ItemGUIDs[i];
	}

	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgResult, APPCMD_AH_SEARCH_RESULT_NEXT);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_PLAYER_SALE_LIST_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_PLAYER_SALE_LIST_MSG* pMsg =
		(AUCTION_GAME_RESPONSE_PLAYER_SALE_LIST_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);


	MSG_APPCMD_AH_PLAYER_ITEM_SALE_LIST msgSaleList;
	msgSaleList.PlayerGUID = pMsg->PlayerGUID;
	msgSaleList.ItemCount = pMsg->ItemCount;

	for (UINT32 i = 0; i < pMsg->ItemCount && i < AUCTION_MAX_ITEM_SALE_COUNT; i++) {
		msgSaleList.ItemGUIDs[i] = pMsg->ItemGUIDs[i];
	}
	
	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgSaleList, APPCMD_AH_PLAYER_ITEM_SALE_LIST);
#endif

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_ITEM_INFO_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_ITEM_INFO_MSG* pMsg = (AUCTION_GAME_RESPONSE_ITEM_INFO_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_ITEM_INFO msgItemInfo;
	msgItemInfo.PlayerGUID = pMsg->PlayerGUID;
	msgItemInfo.ItemGUID = pMsg->ItemGUID;
	msgItemInfo.ItemPrice = pMsg->ItemPrice;
	PStrCopy(msgItemInfo.szSellerName, pMsg->szSellerName, MAX_CHARACTER_NAME);

	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgItemInfo, APPCMD_AH_SEARCH_RESULT_ITEM_INFO);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_ITEM_BLOB_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_ITEM_BLOB_MSG* pMsg = (AUCTION_GAME_RESPONSE_ITEM_BLOB_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	ASSERT_RETURN(pMailBox != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_ITEM_BLOB msgItemBlob;
	msgItemBlob.PlayerGUID = pMsg->PlayerGUID;
	msgItemBlob.ItemGUID = pMsg->ItemGUID;
	MemoryCopy(msgItemBlob.ItemBlob, DEFAULT_MAX_ITEM_BLOB_MSG_SIZE, pMsg->ItemBlob, MSG_GET_BLOB_LEN(pMsg, ItemBlob));
	MSG_SET_BLOB_LEN(msgItemBlob, ItemBlob, MSG_GET_BLOB_LEN(pMsg, ItemBlob));

	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgItemBlob, APPCMD_AH_SEARCH_RESULT_ITEM_BLOB);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_ITEM_SALE_BLOB_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_REQUEST_ITEM_SALE_BLOB_MSG* pMsg = (AUCTION_GAME_REQUEST_ITEM_SALE_BLOB_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	AUCTION_ITEMSALE_INFO* pItemSaleInfo = sUtilGameGetItemSaleInfoByItemID(pSvrCtx, pMsg->ItemGUID);
	ASSERT_RETURN(pItemSaleInfo != NULL);

	CSAutoLeave autoLeave(&pItemSaleInfo->cs);
	ASSERT_RETURN(pItemSaleInfo->pItemBlob != NULL);
	ASSERT_RETURN(pItemSaleInfo->dwItemBlobLen > 0);

	GAME_AUCTION_RESPONSE_ITEM_SALE_BLOB_MSG msgBlob;
	msgBlob.ItemGuid = pMsg->ItemGUID;
	MemoryCopy(msgBlob.ItemBlob, DEFAULT_MAX_ITEM_BLOB_MSG_SIZE, pItemSaleInfo->pItemBlob, pItemSaleInfo->dwItemBlobLen);
	MSG_SET_BLOB_LEN(msgBlob, ItemBlob, pItemSaleInfo->dwItemBlobLen);

	GameServerToAuctionServerSendMessage(&msgBlob, GAME_AUCTION_RESPONSE_ITEM_SALE_BLOB);
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_SEND_ITEM_TO_PLAYER_HANDLER(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(msg);

#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_REQUEST_SEND_ITEM_TO_PLAYER_MSG* pMsg = (AUCTION_GAME_REQUEST_SEND_ITEM_TO_PLAYER_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	GAMEID idUtilGame = UtilityGameGetGameId(pSvrCtx);
	ASSERT_RETURN(idUtilGame != INVALID_GAMEID);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, idUtilGame);
	ASSERT_RETURN(pMailBox != NULL);

	UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER msgEmail;
	msgEmail.idPlayer = pMsg->PlayerGUID;
	msgEmail.idItem = pMsg->ItemGUID;
	msgEmail.bIsWithdrawn = pMsg->ItemIsWithdrawn;
	PStrCopy(msgEmail.szPlayerName, pMsg->PlayerName, MAX_CHARACTER_NAME);

	MSG_APPCMD_UTILITY_GAME_REQUEST msgRequest;
	msgRequest.dwUtilityGameAction = UGA_AH_EMAIL_SEND_ITEM_TO_PLAYER;
	MemoryCopy(msgRequest.byMessageData, MAX_UTILITY_GAME_MESSAGE_SIZE, &msgEmail, sizeof(msgEmail));
	msgRequest.dwMessageSize = sizeof(msgEmail);

	SvrNetPostFakeClientRequestToMailbox(
		pMailBox, ServiceUserId(USER_SERVER, 0, 0),
		&msgRequest, APPCMD_UTILITY_GAME_REQUEST);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_RESPONSE_BUY_ITEM_CHECK_RESULT_HANDLER(
	__notnull LPVOID svrContext,
	SVRID /*sender*/,
	__notnull MSG_STRUCT * msg )
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)svrContext;
	ASSERT_RETURN(pSvrCtx != NULL);

	AUCTION_GAME_RESPONSE_BUY_ITEM_CHECK_RESULT_MSG* pMsg = (AUCTION_GAME_RESPONSE_BUY_ITEM_CHECK_RESULT_MSG*)msg;
	ASSERT_RETURN(pMsg != NULL);

	SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, pMsg->GameID);
	NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(pMsg->PlayerAccountId);

	if (pMailBox == NULL || clientConnectionId == INVALID_NETCLIENTID64)
		return;

	MSG_APPCMD_AH_OK_TO_BUY buyMsg;
	buyMsg.ItemGUID = pMsg->ItemGUID;
	buyMsg.ItemPrice = pMsg->ItemPrice;

	ASSERT(SvrNetPostFakeClientRequestToMailbox(pMailBox, clientConnectionId, &buyMsg, APPCMD_AH_OK_TO_BUY));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AUCTION_GAME_REQUEST_ITEM_EQUIV_TYPE_HANDLER(
	__notnull LPVOID svrContext,
	SVRID /*sender*/,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(svrContext);
	UNREFERENCED_PARAMETER(msg);


#if ISVERSION(SERVER_VERSION)
	UINT32 iUnitTypeCount = UnitTypeGetCount();

	GAME_AUCTION_RESPONSE_ITEM_EQUIV_TYPE_MSG msgEquivType;
	msgEquivType.ItemTypeCount = iUnitTypeCount;

	for (UINT32 i = 0; i < iUnitTypeCount; i++) {
		msgEquivType.ItemTypeIndex = i;
		
		const UNITTYPE_DATA* pUnitTypeData = UnitTypeGetData(i);
		if (pUnitTypeData == NULL) {
			msgEquivType.ItemEquivTypeCount = 0;
		} else {
			UINT32 k = 0;
			for (UINT32 j = 0; j < MAX_UNITTYPE_EQUIV && k < MAX_ITEM_EQUIV_TYPES; j++) {
				if (pUnitTypeData->nTypeEquiv[j] != EXCEL_LINK_INVALID) {
					msgEquivType.ItemEquivTypes[k] = pUnitTypeData->nTypeEquiv[j];
					k++;
				}
			}
			msgEquivType.ItemEquivTypeCount = k;
		}
		GameServerToAuctionServerSendMessage(&msgEquivType, GAME_AUCTION_RESPONSE_ITEM_EQUIV_TYPE);
	}
#endif
}


//----------------------------------------------------------------------------
// RESPONSE HANDLERS ARRAY
//----------------------------------------------------------------------------
#undef   _GAME_AUCTION_RESPONSE_TABLE_
#undef   NET_MSG_TABLE_BEGIN
#undef   NET_MSG_TABLE_DEF
#undef   NET_MSG_TABLE_END
#define  NET_MSG_TABLE_BEGIN(x)			FP_NET_RESPONSE_COMMAND_HANDLER sAuctionToGameResponseHandlers[] = {
#define  NET_MSG_TABLE_DEF(cmd,msg,s,r)	cmd##_HANDLER,
#define  NET_MSG_TABLE_END(x)			};
#include "GameAuctionCommunication.h"

#endif

DWORD x_AuctionGetCostToSell(
	const int nSellPrice,
	const DWORD dwFeeRate)
{
	ASSERT_RETZERO(nSellPrice >= 0);
	return (DWORD)(((DWORD)nSellPrice+(dwFeeRate-1)) * (dwFeeRate / 100.0));
}

DWORD x_AuctionGetMinSellPrice(
	UNIT * pItem)
{
	ASSERT_RETZERO(pItem);
	return (DWORD)EconomyItemSellPrice(pItem).GetValue(KCURRENCY_VALUE_INGAME);
}
