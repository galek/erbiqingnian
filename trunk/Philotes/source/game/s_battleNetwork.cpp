#include "stdafx.h"
#if ISVERSION(SERVER_VERSION)
#include "svrstd.h"
#include "ehm.h"
#include "GameServer.h"
#include "ServerSuite/Battle/BattleGameServerRequestMsg.h"
#include "ServerSuite/Battle/BattleGameServerResponseMsg.h"
#include "c_message.h"
#include "s_network.h"
#include "s_battleNetwork.h"
#include "player.h"

#include "s_battleNetwork.cpp.tmh"

//----------------------------------------------------------------------------
// Currently, we assume the BATTLE_SERVER is a singleton with
// SVRINSTANCE 0.  If this changes, we will have to store a set of instances
// to communicate with.
//----------------------------------------------------------------------------
BOOL GameServerToBattleServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command)
{
	return (SvrNetSendRequestToService(
		ServerId(BATTLE_SERVER, 0), msg, command) == SRNET_SUCCESS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GetBattleTypeFlags(UNIT *pUnit)
{
	ASSERT_RETINVALID(pUnit);
	return GameVariantFlagsGetStaticUnit( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCopyGameClientInfo(
	GAMECLIENT_INFO &tGameClientInfo,
	UNIT *pUnit)
{
	tGameClientInfo.guidCharacter = UnitGetGuid(pUnit);
	tGameClientInfo.idAccount = UnitGetAccountId(pUnit);
	if(pUnit->szName) 
	{
		PStrCopy(tGameClientInfo.szCharName, pUnit->szName, MAX_CHARACTER_NAME);
	}
	else
	{
		tGameClientInfo.szCharName[0] = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCopyBattleClientInfo(
	BATTLECLIENT_INFO &tBattleClientInfo,
	UNIT *pUnit)
{
	tBattleClientInfo.dwCharacterTypeFlags = GetBattleTypeFlags(pUnit);
	tBattleClientInfo.nClass = UnitGetClass(pUnit);
	tBattleClientInfo.nCharacterLevel = UnitGetExperienceLevel(pUnit);
}

//----------------------------------------------------------------------------
// Architecturally, we never reply to the battle server telling us to cancel.
// If we cancel for our own reasons, though, he will send a cancel to both
// game servers involved.
//
// Thus, this function is to be called upon hitting an error in duel creation,
// but not when we cancel a duel because the battle server so orders.
//----------------------------------------------------------------------------
BOOL SendBattleCancel(
	DWORD idBattle, 
	PGUID guidCanceller /*= INVALID_GUID*/,
	WCHAR *szCharCanceller /*= NULL*/,
	BOOL bForfeit /* = FALSE */)
{
	MSG_BATTLE_GAMESERVER_REQUEST_BATTLE_CANCEL msg;

	msg.idBattle = idBattle;
	msg.bForfeit = bForfeit;
	msg.guidCanceller = guidCanceller;
	if(szCharCanceller)
	{
		PStrCopy(msg.szCharCanceller, szCharCanceller, MAX_CHARACTER_NAME);
	}
	else
	{
		msg.szCharCanceller[0] = 0;
	}
	return GameServerToBattleServerSendMessage(&msg, BATTLE_GAMESERVER_REQUEST_BATTLE_CANCEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendSeekCancel(
	UNIT *pUnit)
{
	ASSERT_RETFALSE(pUnit);

	MSG_BATTLE_GAMESERVER_REQUEST_SEEK_CANCEL msg;
	sCopyGameClientInfo(msg.tGameClientInfo, pUnit);

	return GameServerToBattleServerSendMessage(
		&msg, BATTLE_GAMESERVER_REQUEST_SEEK_CANCEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendSeekAutoMatch(
	UNIT *pUnit)
{
	ASSERT_RETFALSE(pUnit);

	MSG_BATTLE_GAMESERVER_REQUEST_SEEK_AUTOMATCH msg;
	sCopyGameClientInfo(msg.tGameClientInfo, pUnit);
	sCopyBattleClientInfo(msg.tBattleClientInfo, pUnit);

	return GameServerToBattleServerSendMessage(
		&msg, BATTLE_GAMESERVER_REQUEST_SEEK_AUTOMATCH);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendBattleIndividualLoss(
	GAME *pGame,
	UNIT *pUnit)
{
	ASSERT_RETFALSE(pUnit);
	MSG_BATTLE_GAMESERVER_REQUEST_BATTLE_RESULT msg;

	sCopyGameClientInfo(msg.tGameClientInfoLoser, pUnit);
	msg.idGame = GameGetId(pGame);

	return GameServerToBattleServerSendMessage(
		&msg, BATTLE_GAMESERVER_REQUEST_BATTLE_RESULT);		
}
//----------------------------------------------------------------------------
// COMMAND HANDLER IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Data transfer macro.  Find the associated game client.  Transfer message
// data from specified battle message to specified game message.
// Additional data beyond character info, as well as error cases, are handled
// outside this macro.
// TODO: factor battle messages to use GAMECLIENT_INFO, but leave battleid sep
//----------------------------------------------------------------------------
#define TRANSFER_MSG( BATTLE_MESSAGE, GAME_MESSAGE) \
	 BATTLE_MESSAGE * msg = ( BATTLE_MESSAGE *)msgdata; \
	GameServerContext * pServerContext = (GameServerContext*)svrContext; \
	BOOL bRet = TRUE; \
	GAME_MESSAGE gameMsg; \
	\
	CBRA(pServerContext); \
	CBRA(msg); \
	\
	NETCLIENTID64 idNetClient = GameServerGetNetClientId64FromAccountId(msg->tGameClientInfo.idAccount); \
	CBRA(idNetClient != INVALID_NETCLIENTID64); \
	GAMEID idGame = NetClientGetGameId(idNetClient); \
	CBRA(!GAMEID_IS_INVALID(idGame)); \
	\
	gameMsg.guidCharacter = msg->tGameClientInfo.guidCharacter; \
	gameMsg.idBattle = msg->idBattle; \
	PStrCopy(gameMsg.szCharName, msg->tGameClientInfo.szCharName, MAX_CHARACTER_NAME);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBattleCmdHostMatch(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	TRANSFER_MSG(MSG_BATTLE_GAMESERVER_RESPONSE_HOST_MATCH,
		MSG_APPCMD_DUEL_AUTOMATED_HOST);

	CBRA(SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_DUEL_AUTOMATED_HOST, &gameMsg));
	
Error:
	if(!bRet)
	{
		SendBattleCancel(msg->idBattle);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBattleCmdGuestMatch(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	TRANSFER_MSG(MSG_BATTLE_GAMESERVER_RESPONSE_GUEST_MATCH,
		MSG_APPCMD_DUEL_AUTOMATED_GUEST);

	PStrCopy(gameMsg.szCharHost, msg->tInstanceInfo.szCharHost, MAX_CHARACTER_NAME);
	gameMsg.guidHost = msg->tInstanceInfo.guidHost;
	gameMsg.idGameHost = msg->tInstanceInfo.idGameHost;
	gameMsg.tWarpSpec = msg->tInstanceInfo.tWarpSpec;

	CBRA(SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_DUEL_AUTOMATED_GUEST, &gameMsg));

Error:
	if(!bRet)
	{
		SendBattleCancel(msg->idBattle);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBattleCmdCancelMatch(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	UNREFERENCED_PARAMETER(sender);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBattleCmdNewRatingForResult(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	TRANSFER_MSG(MSG_BATTLE_GAMESERVER_RESPONSE_NEW_RATING_FOR_RESULT, 
		MSG_APPCMD_DUEL_NEW_RATING);

	gameMsg.fNewPvpLevelDelta = msg->fNewPvpLevelDelta;

	CBRA(SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_DUEL_NEW_RATING, &gameMsg));

Error:
	if(!bRet && msg)
	{
		TraceGameInfo("Could not find client for new rating message intended for %ls",
			msg->tGameClientInfo.szCharName);
	}
}

//----------------------------------------------------------------------------
// Command Table
//----------------------------------------------------------------------------
FP_NET_RESPONSE_COMMAND_HANDLER sBattleToGameResponseHandlers[] =
{
	sBattleCmdHostMatch,
	sBattleCmdGuestMatch,
	sBattleCmdCancelMatch,
	sBattleCmdNewRatingForResult,
};

//above, some commands which basically post it to the game mailbox under the involved player, or post a cancel if the involved player is not found.
#endif