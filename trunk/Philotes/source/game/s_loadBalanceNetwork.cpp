//----------------------------------------------------------------------------
// s_chatNetwork.cpp
// (C)Copyright 2007, Ping0. All rights reserved.
//----------------------------------------------------------------------------
// Handlers for load balance server messages.  Closed server only.
// Also message-sending function for load balance server.

#include "stdafx.h"

#if ISVERSION(SERVER_VERSION)

#include "prime.h"
#include "svrstd.h"
#include "GameServer.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceGameServerRequestMsg.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceGameServerResponseMsg.h"

//----------------------------------------------------------------------------
// GAME_SERVER to GAME_LOADBALANCE_SERVER requests
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Currently, we assume the GAME_LOADBALANCE_SERVER is a singleton with
// SVRINSTANCE 0.  If this changes, we will have to store a Set of instances
// to communicate with.
//----------------------------------------------------------------------------

BOOL GameServerToGameLoadBalanceServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command)
{
	return SvrNetSendRequestToService(ServerId(GAME_LOADBALANCE_SERVER, 0), msg, command);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendGameLoadBalanceHeartBeat(
	GameServerContext *pServerContext)
{
	ASSERT_RETFALSE(pServerContext);
	MSG_GAMELOADBALANCE_GAMESERVER_REQUEST_HEARTBEAT msg;
	msg.attachedClients = pServerContext->m_clients;
	return GameServerToGameLoadBalanceServerSendMessage(
		&msg, GAMELOADBALANCE_GAMESERVER_REQUEST_HEARTBEAT);
}
//----------------------------------------------------------------------------
// We must work directly on the NETCLT_USER here, because he may not have
// finished attaching.
//----------------------------------------------------------------------------
BOOL SendGameLoadBalanceLogin(
	GameServerContext *pServerContext, 
	NETCLT_USER *pClientContext)
{
	MSG_GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_LOGGED_IN msg;
	//Copy all needed info.
	pClientContext->m_cReadWriteLock.ReadLock();

	PStrCopy(msg.szCharName, MAX_CHARACTER_NAME, 
		pClientContext->szCharacter, MAX_CHARACTER_NAME);
	msg.accountId = pClientContext->m_idAccount;
	msg.idNet = pClientContext->m_netId;

	pClientContext->m_cReadWriteLock.EndRead();

	return GameServerToGameLoadBalanceServerSendMessage(
		&msg, GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_LOGGED_IN);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendGameLoadBalanceLogout(
	GameServerContext *pServerContext, 
	NETCLT_USER *pClientContext)
{
	MSG_GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_LOGGED_OUT msg;
	//Copy all needed info.
	pClientContext->m_cReadWriteLock.ReadLock();

	PStrCopy(msg.szCharName, MAX_CHARACTER_NAME, 
		pClientContext->szCharacter, MAX_CHARACTER_NAME);
	msg.accountId = pClientContext->m_idAccount;
	msg.idNet = pClientContext->m_netId;

	pClientContext->m_cReadWriteLock.EndRead();

	return GameServerToGameLoadBalanceServerSendMessage(
		&msg, GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_LOGGED_OUT);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLoadBalanceCmdForceHeartBeat(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	MSG_GAMELOADBALANCE_GAMESERVER_RESPONSE_FORCE_HEARTBEAT * msg =
		(MSG_GAMELOADBALANCE_GAMESERVER_RESPONSE_FORCE_HEARTBEAT *)msgdata;
	GameServerContext * pServerContext = (GameServerContext *) svrContext;

	//If we need more than one GAME_LOADBALANCE_SERVER, we should add the svrid to the Set here.
	ASSERTV(sender == ServerId(GAME_LOADBALANCE_SERVER, 0), 
		"Got loadbalance heartbeat response from incorrect SVRID %#010x", sender);
	//For now we assume there is only one, and assert otherwise.

	//Send  a heartbeat
	SendGameLoadBalanceHeartBeat(pServerContext);
}

//----------------------------------------------------------------------------
// Find the client and post a PLAYERREMOVE message to the appropriate mailbox.
//----------------------------------------------------------------------------
static void sLoadBalanceCmdForceLogout(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	UNREFERENCED_PARAMETER(sender);
	MSG_GAMELOADBALANCE_GAMESERVER_RESPONSE_FORCE_LOGOUT * msg =
		(MSG_GAMELOADBALANCE_GAMESERVER_RESPONSE_FORCE_LOGOUT *)msgdata;
	GameServerContext * pServerContext = (GameServerContext*)svrContext;

	GameServerBootAccountId(pServerContext, msg->accountId);
}

//----------------------------------------------------------------------------
// Command Table
//----------------------------------------------------------------------------
FP_NET_RESPONSE_COMMAND_HANDLER sLoadBalanceToGameResponseHandlers[] =
{
	sLoadBalanceCmdForceHeartBeat,
	sLoadBalanceCmdForceLogout
};

#endif