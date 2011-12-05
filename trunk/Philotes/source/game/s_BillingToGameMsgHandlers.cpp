//----------------------------------------------------------------------------------------------------------------------
// s_BillingToGameMsgHandlers.cpp
// (C)Copyright 2007, Ping0. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#if ISVERSION(SERVER_VERSION)

#include "prime.h"
#include "svrstd.h"
#include "s_message.h"
#include "GameServer.h"
#include "ServerSuite/BillingProxy/BillingMsgs.h" 
#include "clients.h"
#include "s_BillingToGameMsgHandlers.cpp.tmh"
#include "c_network.h"
#include "c_message.h"
#include "s_network.h"
//----------------------------------------------------------------------------------------------------------------------
// Billing Proxy To Game Server Response Handlers

void BillingResponseAccountStatusHandler(void* pServerContext, SVRID /*SenderId*/, struct MSG_STRUCT* pMsg)
{
	ASSERT_RETURN(pServerContext && pMsg);
	GameServerContext& ServerContext = *static_cast<GameServerContext*>(pServerContext);
	const BILLING_RESPONSE_ACCOUNT_STATUS& MsgIn = static_cast<const BILLING_RESPONSE_ACCOUNT_STATUS&>(*pMsg);

	NETCLIENTID64 NetClientId64 = GameServerGetNetClientId64FromAccountId(MsgIn.AccountId);
	
	if (NetClientId64 == INVALID_NETCLIENTID64) {
		TraceVerbose(TRACE_FLAG_GAME_NET, "BillingResponseAccountStatusHandler: invalid account %I64d\n", MsgIn.AccountId);
		return;
	}
	MSG_CCMD_ACCOUNT_STATUS_UPDATE mailBoxMsg;
	mailBoxMsg.nSubscriptionType = MsgIn.eSubscriptionType;
	mailBoxMsg.nBillingTimer = MsgIn.eBillingTimer;
	mailBoxMsg.nRemainingTime = MsgIn.iRemainingTime;
	mailBoxMsg.RemainingTimeRef = MsgIn.RemainingTimeRef;
	mailBoxMsg.nCurrencyBalance = MsgIn.iCurrencyBalance;
	NETCLT_USER *user = (NETCLT_USER *)SvrNetGetClientContext( NetClientId64 );
	ASSERT( user );
	if( user )
	{
		SrvPostMessageToMailbox(user->m_idGame, user->m_idNetClient, CCMD_ACCOUNT_STATUS_UPDATE, &mailBoxMsg);
	}
}


void BillingResponseRealMoneyTxnHandler(void* /*pServerContext*/, SVRID /*SenderId*/, struct MSG_STRUCT* pMsg)
{
	ASSERT_RETURN(pMsg);
	const BILLING_RESPONSE_REAL_MONEY_TXN& MsgIn = static_cast<const BILLING_RESPONSE_REAL_MONEY_TXN&>(*pMsg);

	NETCLIENTID64 NetClientId64 = GameServerGetNetClientId64FromAccountId(MsgIn.AccountId);	
	if (NetClientId64 == INVALID_NETCLIENTID64) {
		TraceVerbose(TRACE_FLAG_GAME_NET, "BillingResponseAccountStatusHandler: invalid account %I64d\n", MsgIn.AccountId);
		return;
	}
	MSG_CCMD_ACCOUNT_BALANCE_UPDATE mailBoxMsg;
	mailBoxMsg.nCurrencyBalance = MsgIn.iCurrencyBalance;
	NETCLT_USER *user = (NETCLT_USER *)SvrNetGetClientContext( NetClientId64 );
	ASSERT( user );
	if( user )
	{
		SrvPostMessageToMailbox(user->m_idGame, user->m_idNetClient, CCMD_ACCOUNT_BALANCE_UPDATE, &mailBoxMsg);
	}

	// FIXME:  this should move into the game server's store code so that player inventory can be updated
	MSG_SCMD_REAL_MONEY_TXN_RESULT MsgOut;
	MsgOut.bResult = MsgIn.bResult;
	MsgOut.iCurrencyBalance = MsgIn.iCurrencyBalance;
	MsgOut.iItemCode = MsgIn.iItemCode;
	PStrCopy(MsgOut.szItemDesc, MsgIn.szItemDesc, arrsize(MsgOut.szItemDesc));

	//UpdateCurrencyOnClientUnits( ClientGetNetId(AppGetClientSystem(), MsgIn.Context) );

	s_SendMessageAppClient(MsgIn.Context, SCMD_REAL_MONEY_TXN_RESULT, &MsgOut);
}


FP_NET_RESPONSE_COMMAND_HANDLER BillingToGameServerResponseHandlers[] =
{
	BillingResponseAccountStatusHandler,
	BillingResponseRealMoneyTxnHandler,
};


#endif // ISVERSION(SERVER_VERSION)