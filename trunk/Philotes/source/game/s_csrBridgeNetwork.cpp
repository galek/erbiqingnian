//----------------------------------------------------------------------------
// s_csrBridgeNetwork.cpp
// (C)Copyright 2007, Ping0. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"

#if ISVERSION(SERVER_VERSION)
#include "svrstd.h"
#include "GameServer.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameRequestMsgs.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameResponseMsgs.h"
#include "s_csrBridgeNetwork.cpp.tmh"
#include "utilitygame.h"
#include "gamelist.h"

//----------------------------------------------------------------------------
// CSR Bridge to Game Server Response Handlers
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBridgeCmdBoot(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);
	MSG_CSRBRIDGE_TO_GAMESERVER_ACTION_BOOT * response =
		(MSG_CSRBRIDGE_TO_GAMESERVER_ACTION_BOOT *)msg;
	// may want to change this trace flag...
//	TraceWarn(TRACE_FLAG_CSR_BRIDGE_SERVER, "Received boot command from CSR bridge for account %#018I64x", response->accountId);

	GameServerContext * pServerContext = (GameServerContext*)svrContext;

	MSG_GAMESERVER_TO_CSRBRIDGE_ACTION_RESULT_BOOT Request;
	Request.bSuccess = GameServerBootAccountId(pServerContext, response->accountId); //  this function was labeled as untested... hmmm
	Request.pipeId = response->pipeId;

	ASSERT(SvrNetSendRequestToService
			(ServerId(CSR_BRIDGE_SERVER, 0), &Request, GAMESERVER_TO_CSRBRIDGE_ACTION_RESULT_BOOT) == SRNET_SUCCESS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBridgeCmdGag(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	UNREFERENCED_PARAMETER(sender);
	MSG_CSRBRIDGE_TO_GAMESERVER_ACTION_GAG * response =
		(MSG_CSRBRIDGE_TO_GAMESERVER_ACTION_GAG *)msg;
	// may want to change this trace flag...
	TraceWarn(TRACE_FLAG_CSR_BRIDGE_SERVER, "Received gag command from CSR bridge for account %#018I64x", response->accountId);

	// do the gag in game
	GameServerContext * pServerContext = (GameServerContext*)svrContext;
	GAG_ACTION eGagAction = (GAG_ACTION)response->nGagAction;
	BOOL bSucccess = GameServerGagAccountId(pServerContext, response->accountId, eGagAction);

	MSG_GAMESERVER_TO_CSRBRIDGE_ACTION_RESPONSE_GAG Request;
	Request.bSuccess = bSucccess;
	Request.pipeId = response->pipeId;

	ASSERT(SvrNetSendRequestToService
			(ServerId(CSR_BRIDGE_SERVER, 0), &Request, GAMESERVER_TO_CSRBRIDGE_ACTION_RESPONSE_GAG) == SRNET_SUCCESS);
}

//----------------------------------------------------------------------------
static void sBridgeUtilityGameMessage(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msg )
{
	GameServerContext *pServerContext = (GameServerContext*)svrContext;
	MSG_CSRBRIDGE_TO_GAMESERVER_UTILITY_GAME *pUtilityGameMessage = (MSG_CSRBRIDGE_TO_GAMESERVER_UTILITY_GAME *)msg;
	BOOL bSent = FALSE;

	// get the utility game id
	GAMEID idGameUtility = UtilityGameGetGameId( pServerContext );
	if (idGameUtility != INVALID_ID)
	{

		// setup message
		MSG_APPCMD_UTILITY_GAME_REQUEST tMessage = pUtilityGameMessage->tRequest;

		// get the mailbox to send to
		SERVER_MAILBOX *pMailBox = GameListGetPlayerToGameMailbox( pServerContext->m_GameList, idGameUtility );
		ASSERTX_RETURN( pMailBox, "Expected mailbox" );

		// send it
		bSent = SvrNetPostFakeClientRequestToMailbox(
			pMailBox, 
			ServiceUserId( USER_SERVER, 0, 0 ),
			&tMessage, 
			APPCMD_UTILITY_GAME_REQUEST);
				
	}
	
	//
	// setup a message back to the CSR bridge with the result of this action ...
	// ideally would like to have the following response structure instead of this
	// blanket one ... we're not quite there yet because there are so many failure
	// cases along the way there, we don't want to leave the CSR portal
	// hanging waiting for a response ... we might have to change this one day tho
	// and it out
	// GameInstaNce -> GameServer -> CSRBridge -> CSRPortal
	//
//	if (bSent == FALSE)
	{
	
		//// setup message
		//MSG_GAMESERVER_TO_CSRBRIDGE_UTILITY_ACTION_SENT tMessage;
		//tMessage.bSuccess = idGameUtility != INVALID_ID;
		//tMessage.idCSRAccount = pUtilityGameMessage->idCSRAccount;
		//tMessage.dwCSRPipeID = pUtilityGameMessage->dwCSRPipeID;
		//tMessage.dwUtilityGameAction = pUtilityGameMessage->dwUtilityGameAction;
		//tMessage.tRequest = pUtilityGameMessage->tRequest;
		//
		//// send to bridge server
		//ASSERT(SvrNetSendRequestToService
		//		(ServerId(CSR_BRIDGE_SERVER, 0), 
		//		&tMessage, 
		//		GAMESERVER_TO_CSRBRIDGE_UTILITY_ACTION_SENT) == SRNET_SUCCESS);
				
	}
	
		
}

//----------------------------------------------------------------------------
// Command Table
//----------------------------------------------------------------------------
FP_NET_RESPONSE_COMMAND_HANDLER sBridgeToGameResponseHandlers[] =
{
	sBridgeCmdBoot,
	sBridgeCmdGag,
	sBridgeUtilityGameMessage,
};

#endif