//----------------------------------------------------------------------------
// s_playerEmail.cpp
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameRequestMsgs.h"
#include "svrstd.h"
#include "s_playerEmail.h"
#include "s_message.h"
#include "c_message.h"
#include "ChatServer.h"
#include "game.h"
#include "clients.h"
#include "units.h"
#include "GameChatCommunication.h"
#include "GameServer.h"
#include "inventory.h"
#include "itemrestore.h"
#include "economy.h"
#include "GlobalIndex.h"
#include "gameglobals.h"
#include "dbunit.h"
#include "database.h"
#include "DatabaseManager.h"
#include "gamelist.h"
#include "items.h"
#include "email.h"
#include "utilitygame.h"
#include "array.h"
#include "units.h"

#if ISVERSION(SERVER_VERSION)
#include "playertrace.h"
#include "s_playerEmailDB.h"
#include "s_playerEmail.cpp.tmh"
#include "s_auctionNetwork.h"
#include "asyncrequest.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
enum EMAIL_ACTION_CONTEXT
{
	EMAIL_CONTEXT_SENDING,
	EMAIL_CONTEXT_ACCEPTING,
	EMAIL_CONTEXT_RECOVERING_FAILED_SEND_ITEM,
	EMAIL_CONTEXT_RECOVERING_FAILED_SEND_MONEY
};

enum EMAIL_CONTEXT_VERSIONS
{
	CONTEXT_V1 = 1,
};

#define CALL_CMD_PUSH(fp, m, b)		(((static_cast<BASE_MSG_STRUCT*>(m))->*(fp)))(b)
#define CALL_CMD_POP(fp, m, b)		(((static_cast<BASE_MSG_STRUCT*>(m))->*(fp)))(b)


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
typedef vector_mp<wstring_mp>::type NAME_LIST;

//----------------------------------------------------------------------------
struct EMAIL_SPEC
{

	// the context of who is constructing this email spec
	EMAIL_SPEC_SOURCE	SpecSource;
	EMAIL_SPEC_SUB_TYPE SpecSubType;

	// email identifiers	
	ULONGLONG			EmailMessageId;
	BOOL				EmailFinalized;

	// who is sending the email
	UNIQUE_ACCOUNT_ID	PlayerAccountId;
	PGUID				PlayerCharacterId;
	WCHAR				PlayerCharacterName[MAX_CHARACTER_NAME];
	
	// for character recipients
	NAME_LIST			RecipientCharacterNameList;

	// for account recipient
	UNIQUE_ACCOUNT_ID	AccountIDRecipient;
	
	// for guild recipients	
	BOOL				SendToGuildMembers;
	WCHAR				RecipientGuildName[MAX_CHAT_CNL_NAME];

	// the email data
	WCHAR				EmailSubject[MAX_EMAIL_SUBJECT];
	WCHAR				EmailBody[MAX_EMAIL_BODY];
	PGUID				AttachedItemId;
	DWORD				AttachedMoney;
	PGUID				AttachedMoneyUnitId;

	// information about the sender
	MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1 SendContext;

	// internal email system information
	BOOL				ItemMoved;
	BOOL				MoneyRemoved;
	BOOL				AttachmentError;

	// CSR email information
	DWORD				dwCSRPipeID;			// the pipe from the CSR bridge to the CSR Portal web app
	ASYNC_REQUEST_ID	idRequestCSRItemRestore;// where the data for the CSR item restore information is (if any)

	// system email information	
	EMAIL_SPEC			*pSystemNext;		// for sytem emails (not sent by players)
	EMAIL_SPEC			*pSystemPrev;		// for sytem emails (not sent by players)
	time_t				timeCreatedUTC;		// the UTC time this was created	
};


//----------------------------------------------------------------------------
// DB HELPER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEmailSourceIsPlayer(
	EMAIL_SPEC_SOURCE eSpecSource)
{

	switch (eSpecSource)
	{
	
		//--------------------------------------------------------------------
		case ESS_PLAYER:
		case ESS_PLAYER_ITEMSALE:
		case ESS_PLAYER_ITEMBUY:
		{
			return TRUE;
		}

		//--------------------------------------------------------------------		
		case ESS_CSR:
		case ESS_AUCTION:
		case ESS_CONSIGNMENT:
		{
			return FALSE;
		}	
	}
	
	ASSERTX_RETFALSE( 0, "Unknown spec source" );
	
}

//----------------------------------------------------------------------------
static EMAIL_SPEC *sGameFindSystemEmail(
	GAME *pGame,
	ULONGLONG idEmail)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( idEmail != INVALID_GUID, "Invalid GUID for email message id" );

	CSAutoLock autoLock(&pGame->m_csSystemEmails);

	// search the list
	for (EMAIL_SPEC *pSpec = pGame->m_pSystemEmails; pSpec; pSpec = pSpec->pSystemNext)
	{
		if (pSpec->EmailMessageId == idEmail)
		{
			return pSpec;
		}
	}
	return NULL;  // not found
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EMAIL_SPEC *sGetEmailSpec(	
	GAME *pGame,
	EMAIL_SPEC_SOURCE eSpecSource, 
	ULONGLONG messageId, 
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( messageId != INVALID_GUID, "Expected mail id" );

	if (sEmailSourceIsPlayer( eSpecSource ))
	{	
		// must have a unit
		ASSERTX_RETNULL( pUnit, "Expected unit" );		
		return pUnit->pEmailSpec;			
	}
	else
	{
		// system emails
		return sGameFindSystemEmail( pGame, messageId );					
	}	
	
}

//----------------------------------------------------------------------------
static void sMoveItemCallbackApp(
	void * context,
	BASE_ITEM_MOVE_DATA * pData,
	BOOL bSuccess )
{
	ASSERT_RETURN(context && pData);
	GameServerContext * serverContext = (GameServerContext*)context;
	ASSERT_RETURN(serverContext != NULL);

	UNIQUE_ACCOUNT_ID idAccountAuctionOwner = GameServerGetAuctionHouseAccountID(serverContext);
	
	SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, pData->RequestingGameId);
	if (pMailbox != NULL) 
	{
		MSG_APPCMD_EMAIL_ITEM_TRANSFER_RESULT msg;
		msg.ullRequestCode = pData->RequestingContext;
		msg.idEmailMessageId = pData->MessageId;
		msg.bTransferSuccess = bSuccess;
		msg.idItemGuid = pData->RootItemId;
		msg.eEmailSpecSource = pData->SpecSource;

		if (pData->RequestingAccountId == idAccountAuctionOwner ||
			(pData->RequestingAccountId == INVALID_UNIQUE_ACCOUNT_ID &&
			 pData->SpecSource != ESS_INVALID && 
			 sEmailSourceIsPlayer( pData->SpecSource ) == FALSE))
		{
			// route to utility game
			SvrNetPostFakeClientRequestToMailbox(
				pMailbox, 
				ServiceUserId(USER_SERVER, 0, 0),
				&msg, 
				APPCMD_UTIL_GAME_EMAIL_ITEM_TRANSFER_RESULT);		
		}
		else
		{
			NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(pData->RequestingAccountId);
			if(clientConnectionId != INVALID_NETCLIENTID64) 
			{
				SvrNetPostFakeClientRequestToMailbox(pMailbox, clientConnectionId, &msg, APPCMD_EMAIL_ITEM_TRANSFER_RESULT);
			}
		} 
	}

	InterlockedDecrement(&serverContext->m_outstandingDbRequests);
}

//----------------------------------------------------------------------------
static BOOL sPostDbReqForMoveItemTreeIntoEscrow(
	GAMEID requestingGameId,
	EMAIL_SPEC *email,
	ULONGLONG requestingContext,
	__notnull UNIT * item )
{
	ASSERTX_RETFALSE( email, "Expected email" );
	ULONGLONG emailMessageId = email->EmailMessageId;
	UNIQUE_ACCOUNT_ID requestingAccountId = email->PlayerAccountId;
	PGUID currentOwnerCharacterId = email->PlayerCharacterId;
	
	// get new owner name ... note we might not have one when sending
	// items to accounts (like CSR item restore)	
	const WCHAR * wszNewOwnerCharacterName = NULL;
	if (email->RecipientCharacterNameList.size() > 0)
	{
		wszNewOwnerCharacterName = email->RecipientCharacterNameList[0].c_str();
	}

	PlayerEmailDBMoveItemTreeIntoEscrow::InitData initData;
	initData.MessageId = emailMessageId;
	initData.SpecSource = email->SpecSource;
	initData.RequestingAccountId = requestingAccountId;
	initData.RequestingGameId = requestingGameId;
	initData.RequestingContext = requestingContext;
	initData.CurrentOwnerCharacterId = currentOwnerCharacterId;
	initData.NewOwnerCharacterName[ 0 ] = 0;
	if (wszNewOwnerCharacterName)
	{
		PStrCopy(initData.NewOwnerCharacterName, wszNewOwnerCharacterName, MAX_CHARACTER_NAME);
	}

	const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_ESCROW));
	ASSERT_RETFALSE(pInvLoc);
	initData.NewInventoryLocation = pInvLoc->wCode;

	initData.RootItemId = UnitGetGuid(item);
	initData.ItemTreeNodeCount = 0;
	UNIT * itrUnit = NULL;
	while ((itrUnit = UnitInventoryIterate(item, itrUnit)) != NULL)
	{
		ASSERT_RETFALSE(initData.ItemTreeNodeCount < arrsize(initData.ItemTreeNodes));

		initData.ItemTreeNodes[initData.ItemTreeNodeCount] = UnitGetGuid(itrUnit);
		++initData.ItemTreeNodeCount;
	}

	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(svrContext);

	InterlockedIncrement(&svrContext->m_outstandingDbRequests);

	DB_REQUEST_TICKET nullTicket;
	ASSERT_DO(
		DatabaseManagerQueueRequest<
			PlayerEmailDBMoveItemTreeIntoEscrow>(
			SvrGetGameDatabaseManager(),
			FALSE,
			nullTicket,
			svrContext,
			(void (*)(LPVOID, PlayerEmailDBMoveItemTreeIntoEscrow::DataType*, BOOL))sMoveItemCallbackApp,
			FALSE,
			&initData,
			currentOwnerCharacterId))
	{
		InterlockedDecrement(&svrContext->m_outstandingDbRequests);
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sPostDbReqForMoveSingleItemIntoEscrow(
	GAMEID requestingGameId,
	EMAIL_SPEC *pEmailSpec,
	ULONGLONG requestingContext)
{
	ASSERTX_RETFALSE( pEmailSpec, "Expected email spec" );
	ULONGLONG emailMessageId = pEmailSpec->EmailMessageId;
	UNIQUE_ACCOUNT_ID requestingAccountId = pEmailSpec->PlayerAccountId;
	PGUID currentOwnerCharacterId = pEmailSpec->PlayerCharacterId;
	
	// get new owner name ... note we might not have one when sending
	// items to accounts (like CSR item restore)
	const WCHAR * wszNewOwnerCharacterName = NULL;
	if (pEmailSpec->RecipientCharacterNameList.size() > 0)
	{
		wszNewOwnerCharacterName = pEmailSpec->RecipientCharacterNameList[0].c_str();
	}
	PGUID itemGUID = pEmailSpec->AttachedItemId;

	PlayerEmailDBMoveItemTreeIntoEscrow::InitData initData;
	initData.MessageId = emailMessageId;
	initData.SpecSource = pEmailSpec->SpecSource;
	initData.RequestingAccountId = requestingAccountId;
	initData.RequestingGameId = requestingGameId;
	initData.RequestingContext = requestingContext;
	initData.CurrentOwnerCharacterId = currentOwnerCharacterId;
	initData.NewOwnerCharacterName[ 0 ] = 0;
	if (wszNewOwnerCharacterName)
	{
		PStrCopy(initData.NewOwnerCharacterName, wszNewOwnerCharacterName, MAX_CHARACTER_NAME);
	}

	const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_ESCROW));
	ASSERT_RETFALSE(pInvLoc);
	initData.NewInventoryLocation = pInvLoc->wCode;

	initData.RootItemId = itemGUID;
	initData.ItemTreeNodeCount = 0;

	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(svrContext);

	InterlockedIncrement(&svrContext->m_outstandingDbRequests);

	DB_REQUEST_TICKET nullTicket;
	ASSERT_DO(
		DatabaseManagerQueueRequest<
		PlayerEmailDBMoveItemTreeIntoEscrow>(
		SvrGetGameDatabaseManager(),
		FALSE,
		nullTicket,
		svrContext,
		(void (*)(LPVOID, PlayerEmailDBMoveItemTreeIntoEscrow::DataType*, BOOL))sMoveItemCallbackApp,
		FALSE,
		&initData,
		currentOwnerCharacterId))
	{
		InterlockedDecrement(&svrContext->m_outstandingDbRequests);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
static BOOL sPostDbReqForRemoveItemTreeFromEscrow(
	ULONGLONG emailMessageId,
	UNIQUE_ACCOUNT_ID requestingAccountId,
	GAMEID requestingGameId,
	ULONGLONG requestingContext,
	PGUID newOwnerCharacterId,
	PGUID itemToTransfer )
{
	PlayerEmailDBRemoveItemTreeFromEscrow::InitData initData;
	initData.MessageId = emailMessageId;
	initData.SpecSource = ESS_INVALID;  // not needed at present, and not here at this point anyway, need to pass it along through the whole system or something
	initData.RequestingAccountId = requestingAccountId;
	initData.RequestingGameId = requestingGameId;
	initData.RequestingContext = requestingContext;
	initData.CharacterId = newOwnerCharacterId;
	initData.RootItemId = itemToTransfer;

	const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_INBOX));
	ASSERT_RETFALSE(pInvLoc);
	initData.NewInventoryLocation = pInvLoc->wCode;

	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(svrContext);

	InterlockedIncrement(&svrContext->m_outstandingDbRequests);

	DB_REQUEST_TICKET nullTicket;
	ASSERT_DO(
		DatabaseManagerQueueRequest<
			PlayerEmailDBRemoveItemTreeFromEscrow>(
			SvrGetGameDatabaseManager(),
			FALSE,
			nullTicket,
			svrContext,
			(void (*)(LPVOID, PlayerEmailDBRemoveItemTreeFromEscrow::DataType*, BOOL))sMoveItemCallbackApp,
			FALSE,
			&initData,
			newOwnerCharacterId ))
	{
		InterlockedDecrement(&svrContext->m_outstandingDbRequests);
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
static void sMoveMoneyIntoEscrowUnitCallback(
	void * context,
	PlayerEmailDBMoveMoneyIntoEscrow::DataType * pData,
	BOOL bSuccess )
{
	ASSERT_RETURN(context && pData);
	GameServerContext * serverContext = (GameServerContext*)context;

	SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, pData->RequestingGameId);
	NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(pData->RequestingAccountId);
	if( pMailbox && clientConnectionId != INVALID_NETCLIENTID64 )
	{
		MSG_APPCMD_EMAIL_MONEY_DEDUCTION_RESULT msg;
		msg.bTransferSuccess = bSuccess;

		SvrNetPostFakeClientRequestToMailbox(pMailbox, clientConnectionId, &msg, APPCMD_EMAIL_MONEY_DEDUCTION_RESULT);
	}

	InterlockedDecrement(&serverContext->m_outstandingDbRequests);
}

//----------------------------------------------------------------------------
static BOOL sPostDbReqForMoveMoneyIntoEscrowUnit(
	ULONGLONG emailMessageId,
	UNIQUE_ACCOUNT_ID requestingAccountId,
	GAMEID requestingGameId,
	PGUID currentOwnerCharacterId,
	__notnull const WCHAR * wszNewOwnerCharacterName,
	int moneyAmmount,
	PGUID newMoneyUnitId )
{
	PlayerEmailDBMoveMoneyIntoEscrow::InitData initData;
	initData.MessageId = emailMessageId;
	initData.RequestingAccountId = requestingAccountId;
	initData.RequestingGameId = requestingGameId;
	initData.CurrentOwnerCharacterId = currentOwnerCharacterId;
	PStrCopy(initData.NewOwnerCharacterName, wszNewOwnerCharacterName, MAX_CHARACTER_NAME);
	initData.MoneyAmmount = moneyAmmount;
	initData.MoneyUnitId = newMoneyUnitId;

	const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_ESCROW));
	ASSERT_RETFALSE(pInvLoc);
	initData.NewInventoryLocation = pInvLoc->wCode;

	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(svrContext);

	InterlockedIncrement(&svrContext->m_outstandingDbRequests);

	DB_REQUEST_TICKET nullTicket;
	ASSERT_DO(
		DatabaseManagerQueueRequest<
			PlayerEmailDBMoveMoneyIntoEscrow>(
			SvrGetGameDatabaseManager(),
			FALSE,
			nullTicket,
			svrContext,
			sMoveMoneyIntoEscrowUnitCallback,
			FALSE,
			&initData,
			currentOwnerCharacterId))
	{
		InterlockedDecrement(&svrContext->m_outstandingDbRequests);
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
static void sRecoverMoneyFromEscrowCallback(
	void * context,
	PlayerEmailDBRemoveMoneyFromEscrow::DataType * pData,
	BOOL success )
{
	ASSERT_RETURN(context && pData);
	GameServerContext * serverContext = (GameServerContext*)context;

	SERVER_MAILBOX * pMailbox =	GameListGetPlayerToGameMailbox(serverContext->m_GameList, pData->RequestingGameId);
	NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(pData->RequestingAccountId);
	if( pMailbox && clientConnectionId != INVALID_NETCLIENTID64 )
	{
		MSG_APPCMD_EMAIL_MONEY_REMOVAL_RESULT resultMsg;
		resultMsg.idEmailMessageId = pData->MessageId;
		resultMsg.dwMoneyAmmount = pData->MoneyAmmount;
		resultMsg.bSuccess = success;

		SvrNetPostFakeClientRequestToMailbox(pMailbox, clientConnectionId, &resultMsg, APPCMD_EMAIL_MONEY_REMOVAL_RESULT);
	}

	InterlockedDecrement(&serverContext->m_outstandingDbRequests);
}

//----------------------------------------------------------------------------
static BOOL sPostDbReqForRecoverMoneyFromEscrow(
	ULONGLONG emailMessageId,
	UNIQUE_ACCOUNT_ID requestingAccountId,
	GAMEID requestingGameId,
	PGUID newOwnerCharacterId,
	int moneyAmmount,
	PGUID moneyUnitId )
{
	PlayerEmailDBRemoveMoneyFromEscrow::InitData initData;
	initData.MessageId = emailMessageId;
	initData.RequestingAccountId = requestingAccountId;
	initData.RequestingGameId = requestingGameId;
	initData.CharacterId = newOwnerCharacterId;
	initData.MoneyAmmount = moneyAmmount;
	initData.MoneyUnitId = moneyUnitId;

	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(svrContext);

	InterlockedIncrement(&svrContext->m_outstandingDbRequests);

	DB_REQUEST_TICKET nullTicket;
	ASSERT_DO(
		DatabaseManagerQueueRequest<
			PlayerEmailDBRemoveMoneyFromEscrow>(
			SvrGetGameDatabaseManager(),
			FALSE,
			nullTicket,
			svrContext,
			sRecoverMoneyFromEscrowCallback,
			FALSE,
			&initData,
			newOwnerCharacterId))
	{
		InterlockedDecrement(&svrContext->m_outstandingDbRequests);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// PRIVATE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
static void sFreeEmail(
	__notnull GAME * game,
	__notnull EMAIL_SPEC * toFree )
{
	toFree->RecipientCharacterNameList.~NAME_LIST();
	GFREE(game, toFree);
}

//----------------------------------------------------------------------------
static void sGameRemoveSystemEmail(
	GAME *pGame,
	EMAIL_SPEC *pEmailSpec)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pEmailSpec, "Expected email spec" );
	
	CSAutoLock autoLock(&pGame->m_csSystemEmails);

	if (pEmailSpec->pSystemNext)
	{
		pEmailSpec->pSystemNext->pSystemPrev = pEmailSpec->pSystemPrev;
	}
	if (pEmailSpec->pSystemPrev)
	{
		pEmailSpec->pSystemPrev->pSystemNext = pEmailSpec->pSystemNext;
	}
	else
	{
		pGame->m_pSystemEmails = pEmailSpec->pSystemNext;  // set a new head
	}

	pEmailSpec->pSystemNext = NULL;
	pEmailSpec->pSystemPrev = NULL;
		
}

//----------------------------------------------------------------------------
static void sGameAddSystemEmail( 
	GAME *pGame,
	EMAIL_SPEC *pEmail)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pEmail, "Expected email" );
	ASSERTX_RETURN( pEmail->pSystemNext == NULL, "Email is already on system list (next)" );
	ASSERTX_RETURN( pEmail->pSystemPrev == NULL, "Email is already on system list (prev)" );
	const time_t SYTEM_EMAIL_SPEC_LIFETIME = SECONDS_PER_MINUTE * 5;

	CSAutoLock autoLock(&pGame->m_csSystemEmails);

	// clean up any old emails that we might have sitting around from responses
	// that never came back with a success code
	time_t timeNow = time( 0 );
	EMAIL_SPEC *pCurrent = NULL;
	EMAIL_SPEC *pNext = NULL;
	for (pCurrent = pGame->m_pSystemEmails; pCurrent; pCurrent = pNext)
	{
	
		// get the next
		pNext = pCurrent->pSystemNext;
		
		// see if this spec is too old to keep around
		if (timeNow - pCurrent->timeCreatedUTC >= SYTEM_EMAIL_SPEC_LIFETIME)
		{
			sGameRemoveSystemEmail( pGame, pCurrent );
		}
		
	}
		
	// add to the game system list
	pEmail->pSystemPrev = NULL;
	pEmail->pSystemNext = pGame->m_pSystemEmails;
	if (pGame->m_pSystemEmails)
	{
		pGame->m_pSystemEmails->pSystemPrev = pEmail;
	}
	pGame->m_pSystemEmails = pEmail;
		
}	

//----------------------------------------------------------------------------
static EMAIL_SPEC * sNewEmail(
	__notnull GAME * game,
	EMAIL_SPEC_SOURCE eEmailSpecSource,
	EMAIL_SPEC_SUB_TYPE eEmailSpecSubType = ESST_INVALID)
{
	ASSERTX_RETNULL(game, "Expected game" );
	EMAIL_SPEC * toRet = (EMAIL_SPEC*)GMALLOCZ(game, sizeof(EMAIL_SPEC));
	ASSERT_RETNULL(toRet);
	
	toRet->EmailMessageId = GameServerGenerateGUID();
	toRet->SpecSource = eEmailSpecSource;
	toRet->SpecSubType = eEmailSpecSubType;
	toRet->EmailFinalized = FALSE;
	toRet->PlayerAccountId = INVALID_UNIQUE_ACCOUNT_ID;
	toRet->PlayerCharacterId = INVALID_GUID;
	toRet->PlayerCharacterName[0] = 0;
	new (&toRet->RecipientCharacterNameList) NAME_LIST(game->m_MemPool);
	toRet->AccountIDRecipient = INVALID_UNIQUE_ACCOUNT_ID;
	toRet->SendToGuildMembers = FALSE;
	toRet->EmailSubject[0] = 0;
	toRet->EmailBody[0] = 0;
	toRet->AttachedItemId = INVALID_GUID;
	toRet->AttachedMoney = 0;
	toRet->ItemMoved = FALSE;
	toRet->MoneyRemoved = FALSE;
	toRet->AttachmentError = FALSE;
	toRet->SendContext.zz_init_msg_struct();

	// the following are for system email bookkeeping
	toRet->timeCreatedUTC = time( 0 );
	toRet->pSystemNext = NULL;
	toRet->pSystemPrev = NULL;

	// for CSR sent emails
	toRet->dwCSRPipeID = INVALID_ID;
	toRet->idRequestCSRItemRestore = INVALID_ID;
		
	return toRet;
}

//----------------------------------------------------------------------------
static void sSendEmailResult(
	__notnull GAME * game,
	__null GAMECLIENT * client,
	DWORD action,
	DWORD result,
	EMAIL_SPEC_SOURCE errorContext )
{

	if (client)
	{	
		MSG_SCMD_EMAIL_RESULT msg;
		msg.actionType = action;
		msg.actionResult = result;
		msg.actionSenderContext = (BYTE)errorContext;
		
		ASSERT(SendMessageToClient(game, client, SCMD_EMAIL_RESULT, &msg));		
	}
	
}

//----------------------------------------------------------------------------
static BOOL sSerializeSendContext(
	__notnull MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1 * context,
	__notnull BYTE * buffer,
	DWORD bufferLen,
	WORD & dataLen )
{
	MFP_PUSHPOP pushFunc = MFP_PUSHPOP(&MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1::Push);
	MSG_STRUCT * msg = context;

	BYTE_BUF_NET bbuf(buffer, bufferLen);

	msg->hdr.cmd = CONTEXT_V1;
	msg->hdr.size = (MSG_SIZE)0;
	msg->hdr.Push(bbuf);

	ASSERT_RETFALSE(CALL_CMD_PUSH(pushFunc, msg, bbuf));

	unsigned int cur = bbuf.GetCursor();
	bbuf.SetCursor(msg->hdr.GetSizeOffset());
	bbuf.Push((MSG_SIZE)cur);
	bbuf.SetCursor(cur);
	dataLen = cur;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSendEmailCreationMessages(
	__notnull GAME * game,
	GAMECLIENT * client,
	__notnull EMAIL_SPEC * email )
{
	{
		CHAT_REQUEST_MSG_CREATE_EMAIL_MESSAGE_START initialMsg;
		initialMsg.idEmailMessageId = email->EmailMessageId;
		initialMsg.idSenderAccountId = email->PlayerAccountId;
		initialMsg.idSenderCharacterId = email->PlayerCharacterId;
		PStrCopy(initialMsg.wszSenderCharacterName, email->PlayerCharacterName, MAX_CHARACTER_NAME);
		initialMsg.idSenderGameId = GameGetId(game);
		initialMsg.eEmailSpecSource = email->SpecSource;
		
		// set the message type
		switch (email->SpecSource)
		{
			case ESS_PLAYER:
			{
				initialMsg.eMessageType = (email->SendToGuildMembers) ? PLAYER_EMAIL_TYPE_GUILD : PLAYER_EMAIL_TYPE_PLAYER;
				break;
			}
			case ESS_PLAYER_ITEMSALE:
			{
				initialMsg.eMessageType = PLAYER_EMAIL_TYPE_ITEMSALE;
				break;
			}
			case ESS_PLAYER_ITEMBUY:
			{
				initialMsg.eMessageType = PLAYER_EMAIL_TYPE_ITEMBUY;
				break;
			}
			case ESS_CSR:
			{
				if (email->AttachedItemId != INVALID_GUID)
				{
					initialMsg.eMessageType = PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE;
				}
				else
				{
					initialMsg.eMessageType = PLAYER_EMAIL_TYPE_CSR;
				}
				break;
			}
			case ESS_AUCTION:
			{
				initialMsg.eMessageType = PLAYER_EMAIL_TYPE_AUCTION;	
				break;
			}
			case ESS_CONSIGNMENT:
			{
				initialMsg.eMessageType = PLAYER_EMAIL_TYPE_CONSIGNMENT;	
				break;			
			}
		}

		//	set the initial message state
		if (email->SpecSource == ESS_PLAYER)
		{
			initialMsg.eInitialMessageState = (email->AttachedMoney == 0 && email->AttachedItemId == INVALID_GUID) ? EMAIL_STATUS_ACCEPTED : EMAIL_STATUS_PENDING_ACCEPTANCE;
		}
		else
		{
			initialMsg.eInitialMessageState = EMAIL_STATUS_ACCEPTED;
		}
				
		ASSERT_RETFALSE(sSerializeSendContext(&email->SendContext, initialMsg.MessageContextData, MAX_EMAIL_CONTEXT_DATA, initialMsg.DEF_MSG_BLOBLEN(MessageContextData)));
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&initialMsg, GAME_REQUEST_CREATE_EMAIL_MESSAGE_START));
	}

	// issue character recipients
	for (size_t ii = 0; ii < email->RecipientCharacterNameList.size(); ++ii)
	{
		CHAT_REQUEST_MSG_ADD_EMAIL_RECIPIENT_BY_CHARACTER_NAME recipientName;
		recipientName.idEmailMessageId = email->EmailMessageId;
		PStrCopy(recipientName.wszTargetCharacterName, email->RecipientCharacterNameList[ii].c_str(), MAX_CHARACTER_NAME);
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&recipientName, GAME_REQUEST_ADD_EMAIL_RECIPIENT_BY_CHARACTER_NAME));
	}

	// issue account recipients
	if (email->AccountIDRecipient != INVALID_UNIQUE_ACCOUNT_ID)
	{
		CHAT_REQUEST_MSG_ADD_EMAIL_RECIPIENT_BY_ID recipientAccount;
		recipientAccount.idEmailMessageId = email->EmailMessageId;
		recipientAccount.idRecipientId = email->AccountIDRecipient;
		recipientAccount.eRecipientType = ERT_ACCOUNT;
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&recipientAccount, GAME_REQUEST_ADD_EMAIL_RECIPIENT_BY_ID));
	}
	
	// issue guild recipients
	if (email->SendToGuildMembers)
	{
		CHAT_REQUEST_MSG_ADD_EMAIL_RECIPIENTS_BY_GUILD_NAME guildMsg;
		guildMsg.idEmailMessageId = email->EmailMessageId;
		BYTE eGuildRank = (BYTE)GUILD_RANK_INVALID;
		
		if (email->SpecSource == ESS_PLAYER)
		{
			WCHAR rankName[MAX_CHARACTER_NAME];
			ASSERT_RETFALSE(s_ClientGetGuildAssociation(client, guildMsg.wszGuildName, MAX_CHAT_CNL_NAME, eGuildRank, rankName, MAX_CHARACTER_NAME));
		}
		else if (email->SpecSource == ESS_CSR)
		{
			PStrCopy( guildMsg.wszGuildName, email->RecipientGuildName, MAX_CHAT_CNL_NAME );
		}
		else
		{
			ASSERTX_RETFALSE( 0, "Invalid email source" );
		}
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&guildMsg, GAME_REQUEST_ADD_EMAIL_RECIPIENTS_BY_GUILD_NAME));
	}

	{
		CHAT_REQUEST_MSG_SET_EMAIL_CONTENTS contentsMsg;
		contentsMsg.idEmailMessageId = email->EmailMessageId;
		PStrCopy(contentsMsg.wszEmailSubject, email->EmailSubject, MAX_EMAIL_SUBJECT);
		PStrCopy(contentsMsg.wszEmailBody, email->EmailBody, MAX_EMAIL_BODY);
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&contentsMsg, GAME_REQUEST_SET_EMAIL_CONTENTS));
	}

	if (email->AttachedMoney > 0 || email->AttachedItemId != INVALID_GUID)
	{
		CHAT_REQUEST_MSG_SET_EMAIL_ATTACHMENTS attachmentsMsg;
		attachmentsMsg.idEmailMessageId = email->EmailMessageId;
		attachmentsMsg.idAttachedItemId = email->AttachedItemId;
		attachmentsMsg.dwAttachedMoney = email->AttachedMoney;
		attachmentsMsg.idAttachedMoneyUnitId = email->AttachedMoneyUnitId;
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&attachmentsMsg, GAME_REQUEST_SET_EMAIL_ATTACHMENTS));
	}

	{
		CHAT_REQUEST_MSG_FINALIZE_EMAIL_CREATION finalizeMsg;
		finalizeMsg.idEmailMessageId = email->EmailMessageId;
		ASSERT_RETFALSE(GameServerToChatServerSendMessage(&finalizeMsg, GAME_REQUEST_FINALIZE_EMAIL_CREATION));
	}

	return TRUE;
}

//----------------------------------------------------------------------------
static int sGetInventoryLocationCode(
	__notnull UNIT * unit,
	GLOBAL_INDEX location )
{
	const INVLOC_DATA* pInvLocData = UnitGetInvLocData(unit, GlobalIndexGet(location));
	ASSERT_RETINVALID(pInvLocData);
	const INVLOCIDX_DATA* pInvLocIdxData = InvLocIndexGetData(pInvLocData->nLocation);
	ASSERT_RETINVALID(pInvLocIdxData);
	return pInvLocIdxData->wCode;
}

//----------------------------------------------------------------------------
static int sGetInventoryLocationSize(
	__notnull UNIT * unit,
	GLOBAL_INDEX location )
{
	const INVLOC_DATA* pInvLocData = UnitGetInvLocData(unit, GlobalIndexGet(location));
	ASSERT_RETZERO(pInvLocData);
	return (pInvLocData->nWidth * pInvLocData->nHeight);
}

//----------------------------------------------------------------------------
static BOOL sMoveAttachedItem(
	__notnull GAME * game,
	__null GAMECLIENT * client,
	__null UNIT * player,
	__notnull EMAIL_SPEC * email )
{
	BOOL success = FALSE;

	if (player)
	{
		s_DBUnitOperationsImmediateCommit(player);
	}

	ONCE
	{
		//	make sure it's still a valid item to email
		UNIT * pAttachedItem = UnitGetByGuid( game, email->AttachedItemId );
		ASSERT_DO(
			pAttachedItem && 
			(UnitGetUltimateContainer(pAttachedItem) == player || player == NULL) && 
			(!UnitIsA(pAttachedItem, UNITTYPE_BACKPACK) || player == NULL))
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ITEM, email->SpecSource);
			break;
		}
		
		// verify that the item is in an allowed "outbox"
		if (email->SpecSource != ESS_CSR)
		{
			INVENTORY_LOCATION oldLoc;
			int invLocEmailOutbox = GlobalIndexGet(GI_INVENTORY_EMAIL_OUTBOX);
			int invLocAuctionOutbox = GlobalIndexGet(GI_INVENTORY_AUCTION_OUTBOX);
			ASSERT_DO(UnitGetInventoryLocation(pAttachedItem, &oldLoc) &&
				(oldLoc.nInvLocation == invLocEmailOutbox ||
				oldLoc.nInvLocation == invLocAuctionOutbox))
			{
				sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ITEM, email->SpecSource);
				break;
			}
			
			//	remove it, disabling the normal db update channels
			s_DatabaseUnitEnable(player, false);
			ASSERT_DO(s_RemoveUnitFromClient(pAttachedItem, client, 0) == TRUE)
			{
				sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ITEM, email->SpecSource);
			}
			ASSERT_DO(UnitInventoryRemove(pAttachedItem, ITEM_ALL) != 0)
			{
				sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ITEM, email->SpecSource);
			}
			s_DatabaseUnitEnable(player, true);
			
		}

		//	queue our own db request to update it's owner and location
		success = sPostDbReqForMoveItemTreeIntoEscrow(
			GameGetId(game),
			email,
			EMAIL_CONTEXT_SENDING,
			pAttachedItem );
	}

	return success;
}

//----------------------------------------------------------------------------
static BOOL sCreateAndDeductMoneyUnit(
	__notnull GAME * game,
	__notnull GAMECLIENT * client,
	__notnull UNIT * unit,
	__notnull EMAIL_SPEC * email )
{
	TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
		"Attempting to deduct money for email attachment. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
		email->EmailMessageId, email->PlayerCharacterId, email->AttachedMoney);

	s_DBUnitOperationsImmediateCommit(unit);

	if (email->AttachedMoney > 0)
	{
		return sPostDbReqForMoveMoneyIntoEscrowUnit(
			email->EmailMessageId,
			email->PlayerAccountId,
			GameGetId(game),
			email->PlayerCharacterId,
			email->RecipientCharacterNameList[0].c_str(),
			email->AttachedMoney,
			email->AttachedMoneyUnitId);
	}
	else
	{
		return TRUE;
	}
}

//----------------------------------------------------------------------------
static BOOL sCommitEmailDelivery(
	ULONGLONG idEmailMessage,
	MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1 *pSenderContext)
{				
	CHAT_REQUEST_MSG_DELIVER_EMAIL_MESSAGE deliverMsg;
	
	deliverMsg.idEmailMessageId = idEmailMessage;
	sSerializeSendContext(
		pSenderContext, 
		deliverMsg.MessageContextData, 
		MAX_EMAIL_CONTEXT_DATA, 
		deliverMsg.DEF_MSG_BLOBLEN( MessageContextData ) );
		
	return GameServerToChatServerSendMessage(&deliverMsg, GAME_REQUEST_DELIVER_EMAIL_MESSAGE);
		
}

//----------------------------------------------------------------------------
static void sProccessItemOrMoneyDone(
	__notnull GAME * game,
	__null	  GAMECLIENT * client,
	__null	  UNIT * player,
	__notnull EMAIL_SPEC * email )
{
	ASSERTX_RETURN( game, "Expected game" );
	ASSERTX_RETURN( email, "Expected email" );
	BOOL bSuccess = FALSE;	
		
	// NOTE that client/player can be NULL here when sending system emails
	// for instance from a CSR or auction house

	if ((email->ItemMoved || email->AttachedItemId == INVALID_GUID) &&
		(email->MoneyRemoved || email->AttachedMoney == 0))
	{
		if (!email->AttachmentError)
		{
			bSuccess = sCommitEmailDelivery( email->EmailMessageId, &email->SendContext );			
		}

		ASSERT_DO(bSuccess)
		{
			if (player)
			{
				//	don't do anything here, it's too iffy, wait until they log back in and sweep up failed sends for a unified cleanup approach
				sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_UNKNOWN_ERROR_SILENT, email->SpecSource);
				sFreeEmail(game, email);
				player->pEmailSpec = NULL;
			}			
		}
		
	}

	// if didn't successfully send, send back results
	if (bSuccess == FALSE)
	{
		if (email->SpecSource == ESS_CSR)
		{
			char szMessage[ MAX_CSR_RESULT_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_CSR_RESULT_MESSAGE, 
				"Error committing email.  Attachment:%I64d, AttachError:%d, AttachMoved:%d",
				email->AttachedItemId,
				email->AttachmentError,
				email->ItemMoved);
				
			// send CSR results			
			s_PlayerEmailCSRSendComplete( game, email->EmailMessageId, FALSE, szMessage, TRUE );
			
		}
		
	}
			
}

//----------------------------------------------------------------------------
static void sRecoverFailedDeliveryAttachments(
	__notnull GAME * game,
	__notnull GAMECLIENT * client,
	__notnull UNIT * unit,
	PGUID itemId,
	PGUID moneyItemId,
	ULONGLONG emailMessageId )
{
	if (itemId == INVALID_GUID && moneyItemId == INVALID_GUID)
		return;

	TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
		"Attempting to recover attachments from bounced or lost email. Item Id: %#018I64x, Money Item Id: %#018I64x",
		itemId, moneyItemId);

	UNIQUE_ACCOUNT_ID accountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	GAMEID gameId = GameGetId(game);
	PGUID unitId = UnitGetGuid(unit);

	if (itemId != INVALID_GUID && UnitGetByGuid(game, itemId) == NULL)
	{
		ASSERT(
			sPostDbReqForRemoveItemTreeFromEscrow(
				emailMessageId,
				accountId,
				gameId,
				EMAIL_CONTEXT_RECOVERING_FAILED_SEND_ITEM,
				unitId,
				itemId));
		ArrayAddItem(unit->tEmailAttachmentsBeingLoaded,itemId);
	}
}

//----------------------------------------------------------------------------
static BOOL sDeserializeSendContext(
	__notnull MSG_STRUCT * msg,
	NET_CMD expectedCmd,
	MFP_PUSHPOP fpPop,
	__notnull BYTE * context,
	WORD contextLength )
{
	BYTE_BUF_NET bbuf(context, contextLength);

	msg->hdr.Pop(bbuf);
	ASSERT_RETFALSE(msg->hdr.cmd == expectedCmd);

	ASSERT_RETFALSE(fpPop);
	ASSERT_RETFALSE(CALL_CMD_POP(fpPop, msg, bbuf));

	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sShouldBounceEmail(
	__notnull GAME * game,
	__notnull GAMECLIENT * client,
	__notnull UNIT * unit,
	__notnull BYTE * context,
	WORD contextLength,
	PGUID attachedItemId,
	DWORD attachedMoney,
	ULONGLONG emailId,
	PLAYER_EMAIL_TYPES emailType)
{
	if (attachedItemId == INVALID_GUID && attachedMoney == 0)
		return FALSE;

	switch (emailType) 
	{
		case PLAYER_EMAIL_TYPE_CSR:
		case PLAYER_EMAIL_TYPE_CSR_ITEM_RESTORE:
		case PLAYER_EMAIL_TYPE_SYSTEM:
		case PLAYER_EMAIL_TYPE_AUCTION:
		case PLAYER_EMAIL_TYPE_CONSIGNMENT:
			return FALSE;
		default:
			break;
	}

	//	!!!NOTE!!!	someday we will need a slightly better solution here to handle several different context versions.
	//				dealing directly with command tables is a pain, so keep it simple!

	MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1 sendContext;
	ASSERT_RETTRUE(
		sDeserializeSendContext(
			&sendContext,
			CONTEXT_V1,
			MFP_PUSHPOP(&MSG_APPCMD_SERIALIZED_EMAIL_SEND_CONTEXT_V1::Pop),
			context,
			contextLength));

	GAME_VARIANT receiverVariant;
	GameVariantInit(receiverVariant, unit);
	DWORD dwFlagsSender = GameVariantFlagsGetStatic(sendContext.tSenderGameVariant.dwGameVariantFlags);
	DWORD dwFlagsReceiver = GameVariantFlagsGetStatic(receiverVariant.dwGameVariantFlags);

	if (dwFlagsSender != dwFlagsReceiver)
	{
		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
			"Rejecting email message with attachments because player game variants don't match. Email Id: %#018I64x",
			emailId);
		return TRUE;
	}

	if (PlayerIsHardcoreDead(unit))
	{
		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
			"Rejecting email message with attachments because player is hardcore dead. Email Id: %#018I64x",
			emailId);
		return TRUE;
	}

	if (attachedItemId != INVALID_GUID)
	{
		if (sendContext.bAttachedItemIsSubscriberOnly && !PlayerIsSubscriber(unit))
		{
			TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
				"Rejecting email message with attachments because the item is subscriber only and the recipient is not a subscriber. Email Id: %#018I64x",
				emailId);
			return TRUE;
		}

		if (UnitInventoryGetCount(unit, INVLOC_EMAIL_INBOX) >= sGetInventoryLocationSize(unit, GI_INVENTORY_EMAIL_INBOX))
		{
			TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
				"Rejecting email message with attachments because the recipient's inbox is full. Email Id: %#018I64x",
				emailId);
			return TRUE;
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
// PUBLIC METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void s_PlayerEmailFreeEmail(
	GAME * game,
	EMAIL_SPEC * email)
{
	ASSERT_RETURN(game && email);

	CHAT_REQUEST_MSG_ABORT_EMAIL_CREATION abortMsg;
	abortMsg.idEmailMessageId = email->EmailMessageId;
	ASSERT(GameServerToChatServerSendMessage(&abortMsg, GAME_REQUEST_ABORT_EMAIL_CREATION));

	sFreeEmail(game, email);
}

//----------------------------------------------------------------------------
void s_PlayerEmailFreeEmail(
	GAME * game,
	UNIT * unit )
{
	ASSERT_RETURN(game && unit);
	if (!UnitIsPlayer(unit))
		return;

	EMAIL_SPEC * email = unit->pEmailSpec;
	if (!email)
		return;

	CHAT_REQUEST_MSG_ABORT_EMAIL_CREATION abortMsg;
	abortMsg.idEmailMessageId = email->EmailMessageId;
	ASSERT(GameServerToChatServerSendMessage(&abortMsg, GAME_REQUEST_ABORT_EMAIL_CREATION));

	sFreeEmail(game, email);
	unit->pEmailSpec = NULL;
}

//----------------------------------------------------------------------------
PGUID s_PlayerEmailCreateNewEmail(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	const WCHAR * emailSubject,
	const WCHAR * emailBody,
	EMAIL_SPEC_SOURCE emailSpecSource)
{
	ASSERT_RETVAL(game && client && unit && emailSubject && emailBody, INVALID_GUID);
	ASSERT_RETVAL(UnitIsPlayer(unit), INVALID_GUID);

	if (unit->pEmailSpec)
		s_PlayerEmailFreeEmail(game, unit);

	EMAIL_SPEC * email = sNewEmail(game, emailSpecSource);
	ASSERT_RETVAL(email, INVALID_GUID);

	email->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	email->PlayerCharacterId = UnitGetGuid(unit);
	UnitGetName(unit, email->PlayerCharacterName, MAX_CHARACTER_NAME, 0);
	PStrCopy(email->EmailSubject, emailSubject, MAX_EMAIL_SUBJECT);
	PStrCopy(email->EmailBody, emailBody, MAX_EMAIL_BODY);

	unit->pEmailSpec = email;
	return email->EmailMessageId;
}

//----------------------------------------------------------------------------
static void sEmailSpecAddEmailRecipientByCharacterName(
	EMAIL_SPEC * email,
	const WCHAR * playerName)
{
	ASSERT_RETURN(email && !email->EmailFinalized);
	ASSERT_RETURN(email->RecipientCharacterNameList.size() < MAX_INDIVIDUAL_TARGET_CHARACTERS);
	
	email->RecipientCharacterNameList.push_back(playerName);
}

//----------------------------------------------------------------------------
void s_PlayerEmailAddEmailRecipientByCharacterName(
	UNIT * unit,
	const WCHAR * playerName )
{
	ASSERT_RETURN(unit && playerName);
	ASSERT_RETURN(UnitIsPlayer(unit));
	ASSERT_RETURN(playerName[0]);

	EMAIL_SPEC * email = unit->pEmailSpec;
	sEmailSpecAddEmailRecipientByCharacterName(email, playerName);
	
}

//----------------------------------------------------------------------------
void s_PlayerEmailAddGuildMembersAsRecipients(
	UNIT * unit )
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETURN(email && !email->EmailFinalized);

	email->SendToGuildMembers = TRUE;
}

//----------------------------------------------------------------------------
void s_PlayerEmailAddEmailAttachments(
	UNIT * unit,
	PGUID idItemToAttach,
	DWORD dwMoneyToAttach )
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETURN(email && !email->EmailFinalized);

	email->AttachedItemId = idItemToAttach;
	email->AttachedMoney = dwMoneyToAttach;
}

//----------------------------------------------------------------------------
BOOL s_PlayerEmailSend(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	DWORD dwFee)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(UnitIsPlayer(unit));

	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETFALSE(email && !email->EmailFinalized);

	email->EmailFinalized = TRUE;

	UNIT *pAttachedItem = NULL;

	GameVariantInit(email->SendContext.tSenderGameVariant, unit);
	email->SendContext.bAttachedItemIsSubscriberOnly = FALSE;

	//	validate target count
	if (email->RecipientCharacterNameList.size() == 0 && !email->SendToGuildMembers)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_NO_RECIPIENTS_SPECIFIED, email->SpecSource);
		goto _CLEANUP;
	}

	//	validate guild sends
	if (email->SendToGuildMembers)
	{
		WCHAR guildName[MAX_GUILD_NAME];
		BYTE eGuildRank;
		WCHAR rankName[MAX_CHARACTER_NAME];
		ASSERT_DO(s_ClientGetGuildAssociation(client, guildName, MAX_GUILD_NAME, eGuildRank, rankName, MAX_CHARACTER_NAME) && guildName[0])
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_NOT_A_GUILD_MEMBER, email->SpecSource);
			goto _CLEANUP;
		}
	}

	//	validate no item or money if sending to more than one person
	if (email->RecipientCharacterNameList.size() > 1 || email->SendToGuildMembers)
	{
		ASSERT_DO(email->AttachedItemId == INVALID_GUID && email->AttachedMoney == 0)
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_NO_MULTSEND_ATTACHEMENTS, email->SpecSource);
			goto _CLEANUP;
		}
	}

	//	get and validate attached item
	if (email->AttachedItemId != INVALID_GUID)
	{
		pAttachedItem = UnitGetByGuid( game, email->AttachedItemId );

		ASSERT_DO(pAttachedItem && UnitGetUltimateContainer(pAttachedItem) == unit && !UnitIsA(pAttachedItem, UNITTYPE_BACKPACK) && !UnitIsPlayer(pAttachedItem))
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ITEM, email->SpecSource);
			goto _CLEANUP;
		}

		email->SendContext.bAttachedItemIsSubscriberOnly = UnitIsSubscriberOnly(pAttachedItem);

		INVENTORY_LOCATION oldLoc;
		ASSERT_GOTO(UnitGetInventoryLocation(pAttachedItem, &oldLoc), _CLEANUP);
		ASSERT_GOTO(oldLoc.nInvLocation == GlobalIndexGet(GI_INVENTORY_EMAIL_OUTBOX) ||
					oldLoc.nInvLocation == GlobalIndexGet(GI_INVENTORY_AUCTION_OUTBOX), _CLEANUP);
	}
	
	//	get the cost to send and validate the resulting totals
	if (dwFee == (DWORD)-1) {
		email->SendContext.dwMailFee = x_PlayerEmailGetCostToSend(unit, email->AttachedItemId, email->AttachedMoney);
	} else {
		email->SendContext.dwMailFee = dwFee;
	}

	int totalCost = (email->SendContext.dwMailFee + (int)email->AttachedMoney);
	ASSERT_DO(email->SendContext.dwMailFee != -1 && totalCost >= 0)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_ATTACHMENTS, email->SpecSource);
		goto _CLEANUP;
	}

	//	go for send, take the fee then send the requests to the chat server
	if (email->SendContext.dwMailFee > 0)
	{
		ASSERT_DO(UnitRemoveCurrencyValidated(unit, cCurrency(email->SendContext.dwMailFee, 0)))
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_INVALID_MONEY, email->SpecSource);
			goto _CLEANUP;
		}
	}

	if (email->AttachedMoney > 0)
		email->AttachedMoneyUnitId = GameServerGenerateGUID();

	ASSERT_DO(sSendEmailCreationMessages(game, client, email))
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_UNKNOWN_ERROR, email->SpecSource);
		goto _UNDO_AND_CLEANUP;
	}

	return TRUE;

_UNDO_AND_CLEANUP:
	UnitAddCurrency(unit, cCurrency(email->SendContext.dwMailFee, 0));

_CLEANUP:
	//	some error occurred whereby we should cancel the email entirely
	s_PlayerEmailFreeEmail(game, unit);

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerEmailAHSendItemToPlayer(
	struct GAME * pGame,
	const UTILITY_GAME_AH_EMAIL_SEND_ITEM_TO_PLAYER* pEmailInfo)
{
	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(pSvrCtx != NULL);

	ASSERTX_RETFALSE(pGame, "Expected game");
	ASSERTX_RETFALSE(GameIsUtilityGame(pGame), "Auction email should only be sent from utility game at present");

	EMAIL_SPEC* pEmail = sNewEmail(pGame, ESS_AUCTION);
	ASSERTX_RETFALSE(pEmail, "Unable to allocate new player email");

	sEmailSpecAddEmailRecipientByCharacterName(pEmail, pEmailInfo->szPlayerName);

	// TODO: replace these with localized messages
	// Ray: the localization should not happen here, it should instead happen
	// when on the client when they read the email unless you modify the server
	// to know which language is selected and get the appropriate string for that
	// language -Colin
	if (pEmailInfo->bIsWithdrawn) {
		PStrCopy(pEmail->EmailSubject, L"<NOT LOCALIZED> Item Withdraw", MAX_EMAIL_SUBJECT );
		PStrCopy(pEmail->EmailBody, L"<NOT LOCALIZED> Item Withdraw", MAX_EMAIL_BODY );
	} else {
		PStrCopy(pEmail->EmailSubject, L"<NOT LOCALIZED> Item Bought", MAX_EMAIL_SUBJECT );
		PStrCopy(pEmail->EmailBody, L"<NOT LOCALIZED> Item Bought", MAX_EMAIL_BODY );
	}

	// who is sending the email
	pEmail->PlayerAccountId = GameServerGetAuctionHouseAccountID(pSvrCtx);
	pEmail->PlayerCharacterId = GameServerGetAuctionHouseCharacterID(pSvrCtx);
	PStrCopy(pEmail->PlayerCharacterName, AUCTION_OWNER_NAME, MAX_CHARACTER_NAME);

	pEmail->AttachedItemId = pEmailInfo->idItem;
	pEmail->AttachedMoney = 0;
	pEmail->SendContext.dwMailFee = 0;

	pEmail->EmailFinalized = TRUE;

	if (!sSendEmailCreationMessages(pGame, NULL, pEmail)) {
		// Which one do I use?
		//sFreeEmail(pGame, pEmail);
		s_PlayerEmailFreeEmail(pGame, pEmail);
		return FALSE;
	}

	sGameAddSystemEmail(pGame, pEmail);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerEmailCSRSend(
	struct GAME * game,
	const struct UTILITY_GAME_CSR_EMAIL * csrEmail,
	ASYNC_REQUEST_ID idRequest /*= INVALID_ID*/,
	UNIT * item /*= NULL*/)
{
	ASSERTX_RETFALSE( game, "Expected game" );
	ASSERTX_RETFALSE( GameIsUtilityGame( game ), "CSR email should only be sent from utility game at present - we can change this when necessary" );

	// if we have an attachment, we must have an async request id so we 
	// can find that data later
	if (item)
	{
		ASSERTX_RETFALSE( idRequest != INVALID_ID, "Expected async request id for CSR item email" );
	}

	// setup our subtype of email ... for now, if there is an item at all
	// we assume it is an item restore.  We will want to change this in the future
	// if we have CSRs be able to create items out of thin air and send them to 
	// players
	EMAIL_SPEC_SUB_TYPE eSubType = ESST_INVALID;
	if (item)
	{
		eSubType = ESST_CSR_ITEM_RESTORE;
	}
		
	// setup a player email message from the CSR email data
	EMAIL_SPEC *email = sNewEmail( game, ESS_CSR, eSubType );
	ASSERTX_RETFALSE( email, "Unable to allocate new player email" );

	// save pipe
	email->dwCSRPipeID = csrEmail->dwCSRPipeID;
	
	// from CSR rep "character"	(this name is replaced by the client UI
	// using a localized name anyway tho)
	PStrCopy( email->PlayerCharacterName, L"CSR", MAX_CHARACTER_NAME );
	
	// recipient character, account, or guild
	BOOL bSuccess = TRUE;
	switch (csrEmail->eRecipientType)
	{
		//----------------------------------------------------------------------------
		case CERT_TO_PLAYER:
		{
			ASSERTX_RETFALSE( csrEmail->uszRecipient[ 0 ], "Expected csr email recipient string name" );
			sEmailSpecAddEmailRecipientByCharacterName( email, csrEmail->uszRecipient );
			break;
		}
		
		//----------------------------------------------------------------------------
		case CERT_TO_ACCOUNT:
		{
			email->AccountIDRecipient = csrEmail->idRecipient;
			ASSERTX_RETFALSE( email->AccountIDRecipient != INVALID_UNIQUE_ACCOUNT_ID, "Invalid account recipient id for CSR email" );
			break;	// not yet supported
		}
		
		//----------------------------------------------------------------------------
		case CERT_TO_GUILD:
		{
			ASSERTX_RETFALSE( csrEmail->uszRecipient[ 0 ], "Expected csr email recipient string name" );		
			email->SendToGuildMembers = TRUE;
			PStrCopy( email->RecipientGuildName, csrEmail->uszRecipient, MAX_CHAT_CNL_NAME );
			break;
		}
		
		//----------------------------------------------------------------------------
		case CERT_TO_SHARD:
		{
			bSuccess = FALSE;
			break; // not yet supported
		}
		
	}

	if (bSuccess)
	{
		
		// subject
		PStrCopy( email->EmailSubject, csrEmail->uszSubject, MAX_EMAIL_SUBJECT );
		
		// body
		PStrCopy( email->EmailBody, csrEmail->uszBody, MAX_EMAIL_BODY );

		// attachment
		email->AttachedItemId = item ? UnitGetGuid( item ) : INVALID_GUID;

		// async request
		email->idRequestCSRItemRestore = idRequest;
				
		// email is now finalized
		email->EmailFinalized = TRUE;
		
		// send the email
		bSuccess = sSendEmailCreationMessages( game, NULL, email );
		ASSERTX( bSuccess, "Unable to send email create message" );

	}
	
	// if we sent it, save the email so we can have something to look at
	// when we receive all the result callbacks from that server
	if (bSuccess)
	{
		sGameAddSystemEmail( game, email );
	}
	else
	{
		// forget it, we failed!
		sFreeEmail( game, email );
	}
		
	// return the sent result
	return bSuccess;
	
}

//----------------------------------------------------------------------------
BOOL s_PlayerEmailCSRSendItemRestore(
	GAME *pGame,
	UNIT *pItem,
	const UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreData,
	ASYNC_REQUEST_ID idRequest)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( pRestoreData, "Expected restore data" );
	ASSERTX_RETFALSE( idRequest != INVALID_ID, "Expected asyns request id" );
	ASSERTX_RETFALSE( GameIsUtilityGame( pGame ), "CSR item restore is a utility game action only" );

	// setup a utility game email struct
	UTILITY_GAME_CSR_EMAIL tCsrEmail;
	tCsrEmail.dwCSRPipeID = pRestoreData->dwCSRPipeID;
	
	// send to account ... we might want to change this to be a specific character
	// instead of the blanket account.
	tCsrEmail.eRecipientType = CERT_TO_ACCOUNT;
	tCsrEmail.idRecipient = pRestoreData->idAccount;
	
	// construct a subject
	PStrPrintf( tCsrEmail.uszSubject, MAX_CSR_EMAIL_SUBJECT, L"CSR item restore [CSR ONLY TEXT]" );

	// construct body
	PStrPrintf( 
		tCsrEmail.uszBody, 
		MAX_CSR_EMAIL_BODY, 
		L"CSR item restore.  CharacterGUID=%I64d, ItemGUID=%I64d.  NOTE that this body and subject are only viewable to the CSR email tools, end users see a localized subject and body saying 'An item is attached to this email' etc",
		pRestoreData->guidCharacter,
		pRestoreData->guidItem);	

	// send the email	
	return s_PlayerEmailCSRSend( pGame, &tCsrEmail, idRequest, pItem );
			
}

//----------------------------------------------------------------------------
void s_PlayerEmailCSRSendComplete(
	GAME *pGame,
	ULONGLONG idEmail,
	BOOL bSuccess,
	const char *pszMessage,		// may be NULL
	BOOL bRemoveSpec)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( idEmail != INVALID_GUID, "Invalid email id" );
	
	// find the email spec
	EMAIL_SPEC *pSpec = sGameFindSystemEmail( pGame, idEmail );
	ASSERTX_RETURN( pSpec, "Unable to find email spec for CSR send complete" );

	// get the async request id (if any)
	ASYNC_REQUEST_ID idRequest = pSpec->idRequestCSRItemRestore;
		
	// send result back to bridge server
	if (pSpec->SpecSubType == ESST_CSR_ITEM_RESTORE)
	{
		ASSERTX_GOTO( pSpec->AttachedItemId != INVALID_GUID, "Expected attached item id to email for CSR item restore", _CLEANUP );
		
		// get the async request for this email
		ASSERTX_GOTO( idRequest != INVALID_ID, "Expected async request id for CSR item restore", _CLEANUP );

		// the restore is now complete
		s_ItemRestoreComplete( pGame, idRequest, bSuccess, pszMessage );
				
	}
	else
	{
	
		// setup message	
		MSG_GAMESERVER_TO_CSRBRIDGE_EMAIL_RESULT tMessage;
		tMessage.bSuccess = bSuccess;
		tMessage.dwCSRPipeID = pSpec->dwCSRPipeID;
		if (pszMessage)
		{
			PStrCopy( tMessage.szMessage, pszMessage, MAX_CSR_RESULT_MESSAGE );
		}
			
		// send it
		SRNET_RETURN_CODE eResult = 
			SvrNetSendRequestToService(
				ServerId( CSR_BRIDGE_SERVER, 0 ), 
				&tMessage, 
				GAMESERVER_TO_CSRBRIDGE_EMAIL_RESULT);
		ASSERTX( eResult == SRNET_SUCCESS, "Unable to send csr email result to bridge" );

	}
	
_CLEANUP:

	// delete any async request data (if any)
	if (idRequest != INVALID_ID)
	{
		AsyncRequestDelete( pGame, idRequest, TRUE );	
	}

	// remove the spec if requested
	if (bRemoveSpec)
	{
		sGameRemoveSystemEmail( pGame, pSpec );
	}
		
}

//----------------------------------------------------------------------------
static void sPlayerEmailHandleCreationResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG idEmailMessageId,
	DWORD emailCreationResultFlags,
	const vector_mp<wstring_mp>::type & ignoringPlayersList )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	BOOL bContinue = FALSE;
	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETURN(email && email->EmailFinalized);
	ASSERT_RETURN(email->EmailMessageId == idEmailMessageId);

	if		(emailCreationResultFlags & EMAIL_RESULT_INITIAL_CREATE_FAILED ||
			 emailCreationResultFlags & EMAIL_RESULT_UNKNOWN_ERROR)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_UNKNOWN_ERROR, email->SpecSource);
	}
	else if (emailCreationResultFlags & EMAIL_RESULT_TARGET_IS_TRIAL_USER)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_TARGET_IS_TRIAL_USER, email->SpecSource);
	}
	else if (emailCreationResultFlags & EMAIL_RESULT_GUILD_RANK_TOO_LOW)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_GUILD_RANK_TOO_LOW, email->SpecSource);
	}
	else if (emailCreationResultFlags & EMAIL_RESULT_RECIPIENT_NOT_FOUND)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_TARGET_NOT_FOUND, email->SpecSource);
	}
	else if (emailCreationResultFlags & EMAIL_RESULT_NO_EMAIL_RECIPIENTS)
	{
		if (ignoringPlayersList.size() > 0)
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_TARGET_IGNORING_SENDER, email->SpecSource);
		}
		else
		{
			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_NO_RECIPIENTS_SPECIFIED, email->SpecSource);
		}
	}
	else if (emailCreationResultFlags & EMAIL_RESULT_BAD_TARGET_GUILD)
	{
		sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_NOT_A_GUILD_MEMBER, email->SpecSource);
	}
	else if (emailCreationResultFlags == EMAIL_RESULT_SUCCESS)
	{
		if (email->AttachedItemId != INVALID_GUID || email->AttachedMoney > 0)
		{
			bContinue = TRUE;
			if (email->AttachedItemId != INVALID_GUID)
			{
				bContinue = sMoveAttachedItem(game, client, unit, email);
				if (bContinue && PlayerActionLoggingIsEnabled() && UnitIsLogged(unit))
				{
					// Log the item loss via mail send here
					ITEM_PLAYER_LOG_DATA tItemData;
					ItemPlayerLogDataInitFromUnit( tItemData, unit );

					WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
					UNIT * pCharacterUnit = UnitGetUltimateOwner( unit );
					UnitGetName( pCharacterUnit, wszCharacterName, MAX_CHARACTER_NAME, 0 );

					TracePlayerItemAction(
						PLAYER_LOG_ITEM_LOST_MAIL,
						UnitGetAccountId( pCharacterUnit ),
						UnitGetGuid( pCharacterUnit ),
						wszCharacterName,
						&tItemData );
				}
			}
			if (bContinue && email->AttachedMoney > 0)
			{
				bContinue = sCreateAndDeductMoneyUnit(game, client, unit, email);
			}
			ASSERT_DO(bContinue)
			{
				sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, PLAYER_EMAIL_UNKNOWN_ERROR_SILENT, email->SpecSource);
				sFreeEmail(game, email);
				unit->pEmailSpec = NULL;
				return;
			}
		}
		else
		{
			bContinue = sCommitEmailDelivery(email->EmailMessageId, &email->SendContext);
		}
	}

	if (!bContinue)
	{
		UnitAddCurrency(unit, cCurrency(email->SendContext.dwMailFee, 0));
		s_PlayerEmailFreeEmail(game, unit);
	}
}

//----------------------------------------------------------------------------
static void sPlayerItemBuyOrSellEmailHandleCreationResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG idEmailMessageId,
	DWORD emailCreationResultFlags,
	const vector_mp<wstring_mp>::type & ignoringPlayersList )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	BOOL bContinue = FALSE;
	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETURN(email && email->EmailFinalized);
	ASSERT_RETURN(email->EmailMessageId == idEmailMessageId);

	BOOL bReturn = FALSE;
	BOOL bCreated = FALSE;

	if (emailCreationResultFlags != EMAIL_RESULT_SUCCESS) {
		goto _err;
	}
	bCreated = TRUE;
	
	ASSERT_GOTO(email->AttachedItemId != INVALID_GUID || email->AttachedMoney > 0, _err);

	if (email->AttachedItemId != INVALID_GUID)
	{
		if (!sMoveAttachedItem(game, client, unit, email)) {
			goto _err;
		}
	
		if (PlayerActionLoggingIsEnabled() && UnitIsLogged(unit)) {
			// Log the item loss via mail send here
			ITEM_PLAYER_LOG_DATA tItemData;
			ItemPlayerLogDataInitFromUnit( tItemData, unit );

			WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
			UNIT * pCharacterUnit = UnitGetUltimateOwner( unit );
			UnitGetName( pCharacterUnit, wszCharacterName, MAX_CHARACTER_NAME, 0 );

			TracePlayerItemAction(
				PLAYER_LOG_ITEM_LOST_MAIL,
				UnitGetAccountId( pCharacterUnit ),
				UnitGetGuid( pCharacterUnit ),
				wszCharacterName,
				&tItemData );
		}
	}

	if (!sCreateAndDeductMoneyUnit(game, client, unit, email)) {
		goto _err;
	}
	bReturn = TRUE;
_err:
	if (!bReturn) {
		MSG_SCMD_AH_ERROR msgError;
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_INTERNAL_ERROR;
		s_SendMessage(game, ClientGetId(client), SCMD_AH_ERROR, &msgError);

		if (bCreated) {
			sFreeEmail(game, email);
			unit->pEmailSpec = NULL;
		} else {
			UnitAddCurrency(unit, cCurrency(email->SendContext.dwMailFee, 0));
			s_PlayerEmailFreeEmail(game, unit);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCSREmailHandleCreationResult(
	GAME *pGame,
	ULONGLONG idEmailMessageId)
{			
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( idEmailMessageId != INVALID_GUID, "Expected email id" );
	EMAIL_SPEC* pEmailSpec = sGameFindSystemEmail( pGame, idEmailMessageId );
	ASSERTX_RETURN( pEmailSpec, "Uanble to find email spec for CSR email" );

	if (pEmailSpec->AttachedItemId == INVALID_GUID)
	{
	
		// there is no item or money, skip forward to go and collect $200
		sProccessItemOrMoneyDone( pGame, NULL, NULL, pEmailSpec );
				
	}
	else
	{
				
		// we have an item, move it to the correct location for email escrow	
		sMoveAttachedItem( pGame, NULL, NULL, pEmailSpec );
	
	}
	
}

//----------------------------------------------------------------------------
static void sAHEmailHandleCreationResult(
	GAME * pGame,
	EMAIL_SPEC* pEmailSpec,
	ULONGLONG idEmailMessageId,
	DWORD emailCreationResultFlags)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pEmailSpec != NULL);
	ASSERT_RETURN(pEmailSpec->EmailFinalized);
	ASSERT_RETURN(pEmailSpec->EmailMessageId == idEmailMessageId);

	ASSERT_RETURN(pEmailSpec->AttachedItemId != INVALID_GUID);
	ASSERT_RETURN(pEmailSpec->AttachedMoney == 0); // change to handle money-emails later

	BOOL bReturn = FALSE;

	if (emailCreationResultFlags != EMAIL_RESULT_SUCCESS) {
		goto _err;
	}

	{
		const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_ESCROW));
		ASSERT_GOTO(pInvLoc != NULL, _err);

		//	queue our own db request to update it's owner and location
		if (!sPostDbReqForMoveSingleItemIntoEscrow(
			GameGetId(pGame),
			pEmailSpec,
			EMAIL_CONTEXT_SENDING))
		{
			goto _err;
		}
	}
	bReturn = TRUE;
_err:

	if (!bReturn) {
		// Which one should I call?
//		sFreeEmail(pGame, pEmailSpec);
		s_PlayerEmailFreeEmail(pGame, pEmailSpec);
	}
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleCreationResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG idEmailMessageId,
	DWORD emailCreationResultFlags,
	EMAIL_SPEC_SOURCE eEmailSpecSource,
	DWORD dwUserContext,	
	const vector_mp<wstring_mp>::type & ignoringPlayersList )
{

	switch (eEmailSpecSource)
	{
		//----------------------------------------------------------------------------	
		case ESS_PLAYER:
		{
			sPlayerEmailHandleCreationResult( 
				game, 
				client, 
				unit, 
				idEmailMessageId,
				emailCreationResultFlags, 
				ignoringPlayersList );
			break;
		}
		
		//----------------------------------------------------------------------------
		case ESS_PLAYER_ITEMSALE:
		case ESS_PLAYER_ITEMBUY:
		{
			sPlayerItemBuyOrSellEmailHandleCreationResult(
				game, 
				client, 
				unit, 
				idEmailMessageId,
				emailCreationResultFlags, 
				ignoringPlayersList );
			break;
		}

		//----------------------------------------------------------------------------
		case ESS_CSR:
		{
		
			// handle creation result
			sCSREmailHandleCreationResult( game, idEmailMessageId );
			break;
			
		}

		//----------------------------------------------------------------------------		
		case ESS_AUCTION:
		case ESS_CONSIGNMENT:
		{
			EMAIL_SPEC* pEmailSpec = sGameFindSystemEmail(game, idEmailMessageId);
			ASSERT_BREAK(pEmailSpec != NULL);

			sAHEmailHandleCreationResult(game, pEmailSpec, idEmailMessageId, emailCreationResultFlags);

			break;
		}
		
		//----------------------------------------------------------------------------
		default:		
		{
			// maybe do something for these other ones someday
			break;
		}
		
	}
	
}

//----------------------------------------------------------------------------
void s_AuctionOwnerEmailHandleItemMoved(
	GAME * game,
	ULONGLONG moveContext,
	PGUID itemId,
	BOOL bSuccess,
	ULONGLONG messageId)
{
	ASSERT_RETURN(game != NULL);
	ASSERT_RETURN(itemId != INVALID_GUID);

	GameServerContext* pSvrCtx = (GameServerContext*) CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	// only accepts EMAIL_CONTEXT_ACCEPTING for now
	if (moveContext == EMAIL_CONTEXT_ACCEPTING) 
	{
		CHAT_REQUEST_MSG_PENDING_EMAIL_MESSAGE_ACTION actionMsg;
		actionMsg.idAccountId = GameServerGetAuctionHouseAccountID(pSvrCtx);
		actionMsg.idCharacterId = GameServerGetAuctionHouseCharacterID(pSvrCtx);
		actionMsg.idTargetGameId = GameGetId(game);
		actionMsg.idPendingEmailMessageId = messageId;

		if (bSuccess) 
		{
			//	accept now that it's moved to our accessible inventory
			//	load the actual item from the database later
			//ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_ACCEPT_PENDING_EMAIL_MESSAGE));

/*
			if (PlayerActionLoggingIsEnabled()) {
				// Log the item gain (mail) here
				// Does this work??
				ITEM_PLAYER_LOG_DATA tItemData;
				tItemData.nLevel = 0;
				tItemData.pszDevName = AUCTION_OWNER_NAME_A;
				tItemData.guidUnit = GameServerGetAuctionHouseCharacterID(pSvrCtx);
				tItemData.pszGenus = "";
				tItemData.pszQuality = "";
				tItemData.wszDescription[0] = L'\0';
				PStrCopy(tItemData.wszTitle, AUCTION_OWNER_NAME, MAX_ITEM_NAME_STRING_LENGTH);

				TracePlayerItemAction(
					PLAYER_LOG_ITEM_GAIN_MAIL,
					GameServerGetAuctionHouseAccountID(pSvrCtx),
					GameServerGetAuctionHouseCharacterID(pSvrCtx),
					AUCTION_OWNER_NAME,
					&tItemData );
			}
			*/
		} 
		else 
		{
			TraceError("Failed to move item %#018I64x while trying to accept email %#018I64x", itemId, messageId);
			ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_BOUNCE_PENDING_EMAIL_MESSAGE));
		}
	} 
	else if (moveContext == EMAIL_CONTEXT_SENDING) 
	{
		if (!bSuccess) 
		{
			TraceError("Failed to move item %#018I64x while trying to send email %#018I64x", itemId, messageId);
		}

		EMAIL_SPEC* pEmail = sGameFindSystemEmail(game, messageId);
		ASSERT_RETURN(pEmail != NULL);

		ASSERT_RETURN(pEmail != NULL);
		ASSERT_RETURN(pEmail->EmailFinalized);
		ASSERT_RETURN(pEmail->AttachedItemId == itemId);

		pEmail->ItemMoved = TRUE;
		pEmail->AttachmentError = (pEmail->AttachmentError || !bSuccess);

		// Assume there's no money involved, just commit now
		sCommitEmailDelivery(messageId, &pEmail->SendContext);	
		
	}
	
}

//----------------------------------------------------------------------------
void s_SystemEmailHandleItemMoved(
	GAME * game,
	EMAIL_SPEC_SOURCE eSpecSource,	
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG moveContext,
	PGUID itemId,
	BOOL bSuccess,
	ULONGLONG messageId )
{
	ASSERTX_RETURN( game, "Expected game" );

	// remove tracking
	if (unit)
	{
		ArrayRemoveByValue(unit->tEmailAttachmentsBeingLoaded, itemId);
	}

	// first handle failing to send an item, we do this first before the
	// other sending/accepting email states because it's a case where we are
	// trying to recover from an error and the player/unit is not 
	// sending an email so there is no email spec -cday
	if (moveContext == EMAIL_CONTEXT_RECOVERING_FAILED_SEND_ITEM && unit)
	{
		if (!bSuccess)
		{
			TraceError("Failed to move item %#018I64x while trying to recover failed email %#018I64x", itemId, messageId);
		}
		else
		{
			//	failed messages are auto accepted, so just load the item for display
			WCHAR name[MAX_CHARACTER_NAME];
			UnitGetName(unit, name, MAX_CHARACTER_NAME, 0);
			PlayerLoadUnitTreeFromDatabase(
				ClientGetAppId(client),
				UnitGetGuid(unit),
				name,
				itemId );

			if (PlayerActionLoggingIsEnabled() && UnitIsLogged(unit))
			{
				// Log the item gain (mail bounced) here
				ITEM_PLAYER_LOG_DATA tItemData;
				ItemPlayerLogDataInitFromUnit( tItemData, unit );

				WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
				UNIT * pCharacterUnit = UnitGetUltimateOwner( unit );
				UnitGetName( pCharacterUnit, wszCharacterName, MAX_CHARACTER_NAME, 0 );

				TracePlayerItemAction(
					PLAYER_LOG_ITEM_GAIN_MAILBOUNCE,
					UnitGetAccountId( pCharacterUnit ),
					UnitGetGuid( pCharacterUnit ),
					wszCharacterName,
					&tItemData );
			}
		}
		
		return;
		
	}
	else if (moveContext == EMAIL_CONTEXT_ACCEPTING && unit)
	{
	
		CHAT_REQUEST_MSG_PENDING_EMAIL_MESSAGE_ACTION actionMsg;
		actionMsg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
		actionMsg.idCharacterId = UnitGetGuid(unit);
		actionMsg.idTargetGameId = GameGetId(game);
		actionMsg.idPendingEmailMessageId = messageId;

		if (!bSuccess)
		{
			TraceError("Failed to move item %#018I64x while trying to accept email %#018I64x", itemId, messageId);
			ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_BOUNCE_PENDING_EMAIL_MESSAGE));
		}
		else
		{
			//	accept now that it's moved to our accessible inventory
			ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_ACCEPT_PENDING_EMAIL_MESSAGE));

			//	load it as well
			WCHAR name[MAX_CHARACTER_NAME];
			UnitGetName(unit, name, MAX_CHARACTER_NAME, 0);
			PlayerLoadUnitTreeFromDatabase(
				ClientGetAppId(client),
				UnitGetGuid(unit),
				name,
				itemId );

			if (PlayerActionLoggingIsEnabled() && UnitIsLogged(unit))
			{
				// Log the item gain (mail) here
				ITEM_PLAYER_LOG_DATA tItemData;
				ItemPlayerLogDataInitFromUnit( tItemData, unit );

				WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
				UNIT * pCharacterUnit = UnitGetUltimateOwner( unit );
				UnitGetName( pCharacterUnit, wszCharacterName, MAX_CHARACTER_NAME, 0 );

				TracePlayerItemAction(
					PLAYER_LOG_ITEM_GAIN_MAIL,
					UnitGetAccountId( pCharacterUnit ),
					UnitGetGuid( pCharacterUnit ),
					wszCharacterName,
					&tItemData );
			}
			
		}
		
	}
	else
	{
				
		// get the email spec
		EMAIL_SPEC *email = sGetEmailSpec( game, eSpecSource, messageId, unit );
		ASSERTX_RETURN( email, "Expected email spec" );
							
		// player emails must have units and clients
		BOOL bIsPlayerEmail = sEmailSourceIsPlayer( eSpecSource );
		if (bIsPlayerEmail)
		{
			ASSERT_RETURN(client);
			ASSERT_RETURN(unit);
			ASSERT_RETURN(UnitIsPlayer(unit));			
		}

		if (moveContext == EMAIL_CONTEXT_SENDING)
		{
			if (!bSuccess)
			{
				TraceError("Failed to move item %#018I64x while trying to send email %#018I64x", itemId, messageId);
			}

			ASSERT_RETURN(email->EmailFinalized);
			ASSERT_RETURN(email->AttachedItemId == itemId);

			email->ItemMoved = TRUE;
			email->AttachmentError = (email->AttachmentError || !bSuccess);

			UNIT *pAttachment = UnitGetByGuid( game, itemId );
			if (pAttachment)
			{
			
				if (email->AttachmentError)
				{
					
					if (bIsPlayerEmail)
					{
						// return item to player's outbox without saving to the database
						s_DatabaseUnitEnable( unit, FALSE );
						bool bChangeLocResult = InventoryChangeLocation(
							unit,
							pAttachment,
							INVLOC_EMAIL_OUTBOX,0,0,
							CLF_ALLOW_NEW_CONTAINER_BIT) != 0;	
						s_DatabaseUnitEnable( unit, TRUE );

						if (bChangeLocResult == FALSE) 
						{ 
							// drop the item, but restrict to this player so nobody else can take it
							// FIXME:  will this work, since the item has already been removed from inventory???
							UnitSetRestrictedToGUID( pAttachment, UnitGetGuid( unit ) );
							s_ItemDrop( unit, pAttachment, TRUE, TRUE );
						}
					}
					else if (eSpecSource == ESS_CSR)
					{
						// item restoration attachment error
						s_ItemRestoreAttachmentError( game, pAttachment );
					}
					
				}
				else
				{
				
					// the unit has been successfully attached to the email, remove
					// it from this game server instance
					UnitFree( pAttachment );
					
				}
				
			}
			else
			{
				TraceError("Unable to find item for email attachment. Item: %#018I64x", itemId);
			}

			sProccessItemOrMoneyDone(game, client, unit, email);
		
		}

	}
		
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleMoneyDeducted(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BOOL bSuccess )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	EMAIL_SPEC * email = unit->pEmailSpec;
	ASSERT_RETURN(email && email->EmailFinalized);

	email->MoneyRemoved = TRUE;
	email->AttachmentError = (email->AttachmentError || !bSuccess);

	if (bSuccess)
	{
		s_DatabaseUnitEnable(unit, FALSE);
		bSuccess = UnitRemoveCurrencyValidated(unit, cCurrency(email->AttachedMoney, 0));
		s_DatabaseUnitEnable(unit, TRUE);

		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
			"Successfully deducted money for email attachment. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
			email->EmailMessageId, email->PlayerCharacterId, email->AttachedMoney);
	}
	else
	{
		TraceError(
			"Failed to deduct money for email attachment. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
			email->EmailMessageId, email->PlayerCharacterId, email->AttachedMoney);
	}

	sProccessItemOrMoneyDone(game, client, unit, email);
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleDeliveryResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG idEmailMessage,
	EMAIL_SPEC_SOURCE eSpecSource,
	BOOL bSuccess )
{
	ASSERT_RETURN(game);
	
	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx);

	EMAIL_SPEC * email = NULL;
	switch (eSpecSource)
	{

		//----------------------------------------------------------------------------	
		case ESS_PLAYER:
		{		
			ASSERT_RETURN(client);
			ASSERT_RETURN(unit);
			ASSERT_RETURN(UnitIsPlayer(unit));

			email = unit->pEmailSpec;
			ASSERT_RETURN(email && email->EmailFinalized);
			ASSERT_RETURN(email->EmailMessageId == idEmailMessage);

			//	if this fails, don't do anything here, it's too iffy, wait until they log back in and sweep up failed sends for a unified cleanup approach

			sSendEmailResult(game, client, CCMD_EMAIL_SEND_COMMIT, (bSuccess) ? PLAYER_EMAIL_SUCCESS : PLAYER_EMAIL_UNKNOWN_ERROR_SILENT, email->SpecSource);
			break;
			
		}
		
		//----------------------------------------------------------------------------
		case ESS_PLAYER_ITEMSALE:
		case ESS_PLAYER_ITEMBUY:
		{
			ASSERT_RETURN(client);
			ASSERT_RETURN(unit);
			ASSERT_RETURN(UnitIsPlayer(unit));
			email = unit->pEmailSpec;
			ASSERT_RETURN(email && email->EmailFinalized);
			ASSERT_RETURN(email->EmailMessageId == idEmailMessage);

			if (bSuccess) {
				// Notify the utility game to pick up this email

				MSG_APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL msgEmailCheck;
				msgEmailCheck.EmailGUID = idEmailMessage;
				GAMEID idUtilGame = UtilityGameGetGameId(pSvrCtx);
				if (idUtilGame != INVALID_GAMEID) {
					SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(pSvrCtx->m_GameList, idUtilGame);
					if (pMailBox != NULL) {
						SvrNetPostFakeClientRequestToMailbox(
							pMailBox, ServiceUserId(USER_SERVER, 0, 0),
							&msgEmailCheck, APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL);
					}
				}
				if (eSpecSource == ESS_PLAYER_ITEMBUY)
				{
					MSG_SCMD_AH_ERROR msgError;
					msgError.ErrorCode = AH_ERROR_BUY_ITEM_SUCCESS;
					s_SendMessage(game, ClientGetId(client), SCMD_AH_ERROR, &msgError);
				}
			} else {
				MSG_SCMD_AH_ERROR msgError;
				msgError.ErrorCode = (eSpecSource == ESS_PLAYER_ITEMSALE) ? AH_ERROR_SELL_ITEM_INTERNAL_ERROR : AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
				s_SendMessage(game, ClientGetId(client), SCMD_AH_ERROR, &msgError);
			}

			break;
		}

		//----------------------------------------------------------------------------
		case ESS_CSR:
		{
			// call the complete function, note that we're not removing the spec inside
			// this function because we will handle it in the case below
			s_PlayerEmailCSRSendComplete( game, idEmailMessage, bSuccess, "Delivery result", FALSE );
			break;
		}
		
		//----------------------------------------------------------------------------
		case ESS_AUCTION:
		case ESS_CONSIGNMENT:
		{
			// Ray, this is the final success case for sending an auction item
			// via the game utility game, there will probably be something here for you to do
			break;			
		}
				
	}

	// remove the spec for system emails, this will remove auction, csr, and
	// other system emails that are not sent by players
	if (sEmailSourceIsPlayer( eSpecSource ) == FALSE)
	{
		EMAIL_SPEC *pSpec = sGameFindSystemEmail( game, idEmailMessage );
		if (pSpec)
		{
			sGameRemoveSystemEmail( game, pSpec );
		}
		
	}
	else
	{
	
		// free email
		ASSERTX_RETURN( email, "expected email spec" );
		sFreeEmail(game, email);
		
		// clear in unit
		ASSERTX_RETURN( unit, "Expected unit" );
		unit->pEmailSpec = NULL;
		
	}
		
}

//----------------------------------------------------------------------------
void s_PlayerEmailRetrieveMessages(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId /*= (ULONGLONG)-1*/ )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	CHAT_REQUEST_MSG_RETRIEVE_EMAIL_MESSAGES requestMsg;
	requestMsg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	requestMsg.idCharacterId = UnitGetGuid(unit);
	requestMsg.idTargetGameId = GameGetId(game);
	requestMsg.idOptionalEmailMessageId = messageId;

	ASSERT(
		GameServerToChatServerSendMessage(
			&requestMsg,
			GAME_REQUEST_RETRIEVE_EMAIL_MESSAGES));
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleEmailMetadata(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	MSG_APPCMD_EMAIL_METADATA * message )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(message);

	// Chris: seems like we want to check something in the email spec itself and 
	// not the type of game that this email happens to be in? -cday
	if (GameIsVariant(game, GV_UTILGAME)) 
	{
		ASSERT_RETURN(client == NULL);
		ASSERT_RETURN(unit == NULL);
	} 
	else 
	{
		ASSERT_RETURN(client);
		ASSERT_RETURN(unit);
		ASSERT_RETURN(UnitIsPlayer(unit));
	}

	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	switch (message->eMessageStatus)
	{
	case EMAIL_STATUS_PENDING_ACCEPTANCE:
		{
			CHAT_REQUEST_MSG_PENDING_EMAIL_MESSAGE_ACTION actionMsg;
			actionMsg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
			actionMsg.idCharacterId = UnitGetGuid(unit);
			actionMsg.idTargetGameId = GameGetId(game);
			actionMsg.idPendingEmailMessageId = message->idEmailMessageId;

			if (sShouldBounceEmail(game, client, unit,
				message->MessageContextData,
				MSG_GET_BLOB_LEN(message, MessageContextData),
				message->idAttachedItemId,
				message->dwAttachedMoney,
				message->idEmailMessageId,
				(PLAYER_EMAIL_TYPES) message->eMessageType))
			{
				ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_BOUNCE_PENDING_EMAIL_MESSAGE));
			}
			else
			{
				if (message->idAttachedItemId == INVALID_GUID)
				{
					ASSERT(GameServerToChatServerSendMessage(&actionMsg, GAME_REQUEST_ACCEPT_PENDING_EMAIL_MESSAGE));
				}
				else
				{
					ASSERT(
						sPostDbReqForRemoveItemTreeFromEscrow(
							message->idEmailMessageId,
							actionMsg.idAccountId,
							actionMsg.idTargetGameId,
							EMAIL_CONTEXT_ACCEPTING,
							actionMsg.idCharacterId,
							message->idAttachedItemId));
					ArrayAddItem(unit->tEmailAttachmentsBeingLoaded,message->idAttachedItemId);
				}
			}
		}
		break;

	case EMAIL_STATUS_BOUNCED:
	case EMAIL_STATUS_ABANDONED_COMPOSING:
		// these two conditions are treated as auto-accepted emails, move any items to an accessible inv loc and then load them for viewing
		// handle the case where the AH failed to send items to the client later
	case EMAIL_STATUS_ACCEPTED:
		// If this is a regular game then send the metadata to the client
		if (!GameIsVariant(game, GV_UTILGAME))
		{																								//	if
			if (message->idAttachedItemId != INVALID_GUID &&											//		an item is attached
				!UnitGetByGuid(game, message->idAttachedItemId) &&										//		and it's not already loaded
				unit->tEmailAttachmentsBeingLoaded.GetIndex(message->idAttachedItemId) == INVALID_ID)	//		and we're not already trying to load it
			{
				sRecoverFailedDeliveryAttachments(game, client, unit, message->idAttachedItemId, message->idAttachedMoneyUnitId, message->idEmailMessageId);
			}

			MSG_SCMD_EMAIL_METADATA clientMsg;
			clientMsg.idEmailMessageId = message->idEmailMessageId;
			clientMsg.idSenderCharacterId = message->idSenderCharacterId;
			PStrCopy(clientMsg.wszSenderCharacterName, message->wszSenderCharacterName, MAX_CHARACTER_NAME);
			clientMsg.eMessageStatus = message->eMessageStatus;
			clientMsg.eMessageType = message->eMessageType;
			clientMsg.timeSentUTC = message->timeSentUTC;
			clientMsg.timeOfManditoryDeletionUTC = message->timeOfManditoryDeletionUTC;
			clientMsg.bIsMarkedRead = message->bIsMarkedRead;
			clientMsg.idAttachedItemId = message->idAttachedItemId;
			clientMsg.dwAttachedMoney = message->dwAttachedMoney;

			ASSERT_BREAK(SendMessageToClient(game, client, SCMD_EMAIL_METADATA, &clientMsg));
		}
		else
		{
			if (message->eMessageType == PLAYER_EMAIL_TYPE_ITEMSALE)
			{
				ASSERT_BREAK(message->idAttachedItemId != INVALID_GUID);

				sPostDbReqForRemoveItemTreeFromEscrow(
					message->idEmailMessageId,
					GameServerGetAuctionHouseAccountID(pSvrCtx),
					GameGetId(game),
					EMAIL_CONTEXT_ACCEPTING,
					GameServerGetAuctionHouseCharacterID(pSvrCtx),
					message->idAttachedItemId);
			}

			if (message->eMessageType == PLAYER_EMAIL_TYPE_ITEMBUY)
			{
				ASSERT_BREAK(message->dwAttachedMoney > 0);
			}

			ASSERT_BREAK(AsyncRequestNewByKey(game, message->idEmailMessageId, message, sizeof(MSG_APPCMD_EMAIL_METADATA)));
		}

		break;

	case EMAIL_STATUS_COMPOSING:
	default:
		TraceError("Received email meta data with an invalid status. Email Id: %#018I64x, Email Status: %u",
			message->idEmailMessageId,
			message->eMessageStatus);
		break;
	}
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleEmailData(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	MSG_APPCMD_ACCEPTED_EMAIL_DATA * message )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(message);

	if (!GameIsVariant(game, GV_UTILGAME))
	{
		ASSERT_RETURN(client);
		ASSERT_RETURN(unit);
		ASSERT_RETURN(UnitIsPlayer(unit));

		MSG_SCMD_EMAIL_DATA dataMsg;
		dataMsg.idEmailMessageId = message->idEmailMessageId;
		PStrCopy(dataMsg.wszEmailSubject, message->wszEmailSubject, MAX_EMAIL_SUBJECT);
		PStrCopy(dataMsg.wszEmailBody, message->wszEmailBody, MAX_EMAIL_BODY);

		ASSERT(SendMessageToClient(game, client, SCMD_EMAIL_DATA, &dataMsg));
	}
	else
	{
		ASSERT_RETURN(client == NULL);
		ASSERT_RETURN(unit == NULL);

		void * pData = AsyncRequestGetDataByKey(game, message->idEmailMessageId);
		ASSERT_RETURN(pData);
		MSG_APPCMD_EMAIL_METADATA metaData;
		memcpy_s(&metaData, sizeof(metaData), pData, sizeof(metaData));
		ASSERT_RETURN(AsyncRequestDeleteByKey(game, message->idEmailMessageId, TRUE));

		GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
		ASSERT_RETURN(pSvrCtx != NULL);

		if (metaData.eMessageType == PLAYER_EMAIL_TYPE_ITEMSALE)
		{
			ASSERT_RETURN(metaData.idAttachedItemId != INVALID_GUID);

			if (UtilGameAuctionItemSaleHandleEmailData(
				pSvrCtx,
				message->idEmailMessageId,
				message->wszEmailSubject,
				message->wszEmailBody)) {

					//	load the actual item from the database now
					PlayerLoadUnitTreeFromDatabase(
						GameServerGetAuctionHouseAppClientID(pSvrCtx),
						GameServerGetAuctionHouseCharacterID(pSvrCtx),
						AUCTION_OWNER_NAME,
						metaData.idAttachedItemId,
						GameServerGetAuctionHouseAccountID(pSvrCtx),
						PLT_AH_ITEMSALE_SEND_TO_AH);
			} else {
				// TODO: Email the item back?
			}
		}

		if (metaData.eMessageType == PLAYER_EMAIL_TYPE_ITEMBUY)
		{
			ASSERT_RETURN(metaData.dwAttachedMoney > 0);

			//	TODO: now we can send the purchase request to the auction house
		}
	}
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleEmailDataUpdate(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	MSG_APPCMD_EMAIL_UPDATE * message )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));
	ASSERT_RETURN(message);

	MSG_SCMD_EMAIL_UPDATE updateMsg;
	updateMsg.idEmailMessageId = message->idEmailMessageId;
	updateMsg.timeOfManditoryDeletionUTC = message->timeOfManditoryDeletionUTC;
	updateMsg.bIsMarkedRead = message->bIsMarkedRead;
	updateMsg.idAttachedItemId = message->idAttachedItemId;
	updateMsg.dwAttachedMoney = message->dwAttachedMoney;

	ASSERT(SendMessageToClient(game, client, SCMD_EMAIL_UPDATE, &updateMsg));
}

//----------------------------------------------------------------------------
void s_PlayerEmailRestoreOutboxItemLocation(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	s_ItemsRestoreToStandardLocations(unit, INVLOC_EMAIL_OUTBOX);
}

//----------------------------------------------------------------------------
void s_PlayerEmailMarkMessageAsRead(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	CHAT_REQUEST_MSG_EMAIL_ACTION msg;
	msg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	msg.idCharacterId = UnitGetGuid(unit);
	msg.idTargetGameId = GameGetId(game);
	msg.idEmailMessageId = messageId;

	ASSERT(
		GameServerToChatServerSendMessage(
			&msg,
			GAME_REQUEST_MARK_EMAIL_READ));
}

//----------------------------------------------------------------------------
void s_PlayerEmailRemoveAttachedMoney(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	CHAT_REQUEST_MSG_EMAIL_ACTION msg;
	msg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	msg.idCharacterId = UnitGetGuid(unit);
	msg.idTargetGameId = GameGetId(game);
	msg.idEmailMessageId = messageId;

	ASSERT(
		GameServerToChatServerSendMessage(
			&msg,
			GAME_REQUEST_GET_ATTACHED_MONEY_INFO));
}

//----------------------------------------------------------------------------
void s_PlayerEmailHandleAttachedMoneyInfoForRemoval(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId,
	DWORD dwMoneyAmmount,
	PGUID idMoneyUnitId )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	if (dwMoneyAmmount == 0 || idMoneyUnitId == INVALID_GUID)
		return;

	cCurrency currentAmmount = UnitGetCurrency(unit);

	if (!currentAmmount.CanCarry(game, cCurrency(dwMoneyAmmount)))
		return;

	TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
		"Starting to retrieve money from email. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
		messageId, UnitGetGuid(unit), dwMoneyAmmount);

	s_DBUnitOperationsImmediateCommit(unit);
	
	ASSERT(
		sPostDbReqForRecoverMoneyFromEscrow(
			messageId,
			ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client)),
			GameGetId(game),
			UnitGetGuid(unit),
			dwMoneyAmmount,
			idMoneyUnitId));
}

//----------------------------------------------------------------------------
void 	s_PlayerEmailHandleAttachedMoneyRemovalResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId,
	DWORD dwMoneyAmmount,
	BOOL bSuccess )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	if (bSuccess)
	{
		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,
			"Successfully retrieved money from email. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
			messageId, UnitGetGuid(unit), dwMoneyAmmount);

		CHAT_REQUEST_MSG_CLEAR_EMAIL_ATTACHMENTS msg;
		msg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
		msg.idCharacterId = UnitGetGuid(unit);
		msg.idTargetGameId = GameGetId(game);
		msg.idEmailMessageId = messageId;
		msg.bClearItem = FALSE;
		msg.bClearMoney = TRUE;

		ASSERT(
			GameServerToChatServerSendMessage(
				&msg,
				GAME_REQUEST_CLEAR_ATTACHMENTS));

		UnitAddCurrency(unit, cCurrency(dwMoneyAmmount));
	}
	else
	{
		TraceError(
			"Failed to retrieve money from email. Email Id: %#018I64x, Owner Id: %#018I64x, Money Ammount: %u",
			messageId, UnitGetGuid(unit), dwMoneyAmmount);
	}
}

//----------------------------------------------------------------------------
void s_PlayerEmailClearAttachedItem(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	CHAT_REQUEST_MSG_CLEAR_EMAIL_ATTACHMENTS msg;
	msg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	msg.idCharacterId = UnitGetGuid(unit);
	msg.idTargetGameId = GameGetId(game);
	msg.idEmailMessageId = messageId;
	msg.bClearItem = TRUE;
	msg.bClearMoney = FALSE;

	ASSERT(
		GameServerToChatServerSendMessage(
			&msg,
			GAME_REQUEST_CLEAR_ATTACHMENTS));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerEmailItemAttachmenTaken(
	struct UNIT *pPlayer,
	struct UNIT *pItem)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );

	// clear the stat that can always allow items in email slots, we use
	// this for CSR restored items so we can email literally anything
	UnitClearStat( pItem, STATS_ALWAYS_ALLOW_IN_EMAIL_SLOTS, 0 );
	
}


//----------------------------------------------------------------------------
void s_PlayerEmailDeleteMessage(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ULONGLONG messageId )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(UnitIsPlayer(unit));

	CHAT_REQUEST_MSG_EMAIL_ACTION delMsg;
	delMsg.idAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(client));
	delMsg.idCharacterId = UnitGetGuid(unit);
	delMsg.idTargetGameId = GameGetId(game);
	delMsg.idEmailMessageId = messageId;

	ASSERT(
		GameServerToChatServerSendMessage(
			&delMsg,
			GAME_REQUEST_DELETE_MESSAGE));
}

#endif	//	ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
int x_PlayerEmailGetCostToSend(
	UNIT * pSourcePlayerUnit,
	PGUID attachedItem,
	UINT32 attachedMoney )
{
	ASSERT_RETX(pSourcePlayerUnit, -1);

	float fCost = (float)EMAIL_BASE_COST_TO_SEND;

	if (attachedItem != INVALID_GUID)
	{
		GAME * pGame = UnitGetGame(pSourcePlayerUnit);
		ASSERT_RETX(pGame, -1);
		UNIT * pAttachedItem = UnitGetByGuid(pGame, attachedItem);
		ASSERT_RETX(pAttachedItem, -1);

		cCurrency currency = EconomyItemBuyPrice( pAttachedItem, NULL, TRUE, TRUE );
		int nItemVal = currency.GetValue( KCURRENCY_VALUE_INGAME );
		fCost += (float)nItemVal * (EMAIL_COST_ITEM_VAL_PERCENT / 100.0f);
	}

	if (attachedMoney > 0)
	{
		fCost += (float)attachedMoney * (EMAIL_COST_MONEY_PERCENT / 100.0f);
	}

	ASSERT_RETX(fCost >= 0, -1);

	return FLOAT_TO_INT_CEIL(fCost);
}
