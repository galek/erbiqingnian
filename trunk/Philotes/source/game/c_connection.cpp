//-------------------------------------------------------------------------------------------------
// c_connection.cpp
//
// (C)Copyright 2007, Ping0. All rights reserved.
//-------------------------------------------------------------------------------------------------
//
// Contains factored out connection state machine for clients.
// Accessed mainly by ConnectionStateInit() (for initialization) and ConnectionStateUpdate()
// Freed by ConnectionStateFree().  Do this then init to reset it.
// ConnectionStateGetState should get the state without advancing things, if needed.
//
// For a more "general" connection state machine, see BotClient/BotConnection.cpp

#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "c_connmgr.h"
#include "c_authticket.h"
#include "c_authentication.h"
#include "c_characterselect.h"
#include "pakfile.h"
#include "patchclient.h"
#include "c_connection.h"
#include "c_chatNetwork.h"
#include "ServerSuite/BillingProxy/c_BillingClient.h"

#define CLOSED_CLIENT_ONLY(x) if(AppGetType() == APP_TYPE_CLOSED_CLIENT) { x; }
#define NOT_CLOSED_CLIENT(x)  if(AppGetType() != APP_TYPE_CLOSED_CLIENT) { x; }
#define SINGLE_PLAYER_ONLY(x) if(AppGetType() == APP_TYPE_SINGLE_PLAYER) { x; }

#define CC_ONLY(x) CLOSED_CLIENT_ONLY(x)
#define NOT_CC(x)  NOT_CLOSED_CLIENT(x)
#define SP_ONLY(x) SINGLE_PLAYER_ONLY(x)

//----------------------------------------------------------------------------
// ENUM
//----------------------------------------------------------------------------
//See c_connection.h for CONNECTION_STATE enum

enum CONNECTION_RESULT
{
	CONNECTION_RESULT_FAILURE,
	CONNECTION_RESULT_CONTINUING,
	CONNECTION_RESULT_DONE
};

//----------------------------------------------------------------------------
// STRUCT
//----------------------------------------------------------------------------
struct ConnectionState
{
	CONNECTION_STATE m_eState;

	MAILSLOT		m_GameLoadBalanceMailSlot;
	WCHAR			m_szChararacter[MAX_CHARACTER_NAME];

	//At some point, we may add an auth server address override here.
};

static ConnectionState sConnectionState;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionStateInit(MEMORYPOOL *pPool)
{
	MailSlotInit(
		sConnectionState.m_GameLoadBalanceMailSlot, 
		pPool,
		MAX_SMALL_MSG_SIZE);
	ConnectionManagerInit(pPool, AppGetType());

	sConnectionState.m_eState = CONNECTION_STATE_STARTUP;

	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionStateFree()
{
	AuthTicketFree();

	if(AppGetChatChannel())
	{
		ConnectionManagerCloseChannel(AppGetChatChannel());
		AppSetChatChannel(NULL);
	}
	if(AppGetBillingChannel())
	{
		ConnectionManagerCloseChannel(AppGetBillingChannel());
		AppSetBillingChannel(NULL);
	}
	if(AppGetGameChannel())
	{
		ConnectionManagerCloseChannel(AppGetGameChannel());
		AppSetGameChannel(NULL);
	}
	if(AppGetPatchChannel())
	{
		PatchClientDisconnect();
		ConnectionManagerCloseChannel(AppGetPatchChannel());
		AppSetPatchChannel(NULL);
	}
#if 0
	if (AppGetFillPakChannel())
	{
		ConnectionManagerCloseChannel(AppGetFillPakChannel());
		AppSetFillPakChannel(NULL);
	}
#endif
	ConnectionManagerClose();
	MailSlotFree(gApp.m_GameMailSlot);
	MailSlotFree(gApp.m_ChatMailSlot);
	MailSlotFree(gApp.m_BillingMailSlot);
	MailSlotFree(gApp.m_PatchMailSlot);
	MailSlotFree(sConnectionState.m_GameLoadBalanceMailSlot);

	sConnectionState.m_eState = CONNECTION_STATE_DISCONNECTED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONNECTION_STATE ConnectionStateGetState()
{
	return sConnectionState.m_eState;
}

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
// TODO: Delete all these function's counterparts in prime.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Allows initialization of an arbitrary amount of command tables 
// on one channel.
// Warning!  Incredibly ugly code.

// Each command table, in turn, is "pushed" onto the command table stack,
// so the first one is really the last you get to.  LIFO for the win!
//----------------------------------------------------------------------------
static CHANNEL* sInitNetworkChannel(
	SVRTYPE iServerType,
	MAILSLOT** mailbox,
	NET_COMMAND_TABLE** cmdTableServer,
	NET_COMMAND_TABLE** cmdTableClient,
	int nTableCount = 1)
{
	CHAR szClosedServerIp[256];
	BOOL bSinglePlayer = (AppGetType() == APP_TYPE_SINGLE_PLAYER);
	ServerInformation serverInfo[MAX_TABLE_LIST] =
	{
		{(bSinglePlayer ? NULL : gApp.szServerIP),
		gApp.serverPort,
		iServerType,
		NULL,
		NULL,
		NULL}
	};

	ASSERT_DO(nTableCount <= MAX_TABLE_LIST) {nTableCount = MAX_TABLE_LIST;}

	if(AppGetType() == APP_TYPE_CLOSED_CLIENT && *(gApp.szServerIP) == 0) {
		AuthTicketGetUserServer(iServerType, szClosedServerIp, sizeof(szClosedServerIp), &serverInfo[0].port);
		serverInfo[0].szIP = szClosedServerIp;
	}

	for(int i = 0; i < nTableCount; i++)
	{
		if(i > 0) serverInfo[i-1].pSecondaryServerInformation = &serverInfo[i];
		MailSlotInit(*mailbox[i], NULL, (iServerType == PATCH_SERVER ? MAX_LARGE_MSG_SIZE : MAX_SMALL_MSG_SIZE));

		serverInfo[i].response_table = cmdTableServer[i];
		serverInfo[i].request_table = cmdTableClient[i];
		serverInfo[i].mailslot = mailbox[i];
	}
	return ConnectionManagerOpenChannel(serverInfo[0]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInitNetworkGameChannel()
{
	CHANNEL* pChannel;

	ASSERT_RETFALSE(AppGetGameChannel() == NULL);
	MAILSLOT *mailslots[] = {&gApp.m_GameMailSlot, &sConnectionState.m_GameLoadBalanceMailSlot};
	NET_COMMAND_TABLE * serverCommandTables[] = 
	{gApp.m_GameServerCommandTable, gApp.m_GameLoadBalanceServerCommandTable };
	NET_COMMAND_TABLE * clientCommandTables[] =
	{gApp.m_GameClientCommandTable, gApp.m_GameLoadBalanceClientCommandTable};

	pChannel = sInitNetworkChannel(
		GAME_SERVER,
		mailslots,
		serverCommandTables,
		clientCommandTables,
		((AppGetType()==APP_TYPE_CLOSED_CLIENT)?2:1));
	ASSERT_RETFALSE(pChannel != NULL);
	AppSetGameChannel(pChannel);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInitNetworkChatChannel(
							   void)
{
	CHANNEL* pChannel;

	ASSERT_RETFALSE(AppGetChatChannel() == NULL);

	MAILSLOT * pMailSlot = &gApp.m_ChatMailSlot;

	pChannel = sInitNetworkChannel(
		CHAT_SERVER,
		&pMailSlot,
		&gApp.m_ChatServerCommandTable,
		&gApp.m_ChatClientCommandTable);
	ASSERT_RETFALSE(pChannel != NULL);
	AppSetChatChannel(pChannel);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInitNetworkBillingChannel(void)
{
	ASSERT_RETFALSE(AppGetBillingChannel() == 0);
	MAILSLOT* pMailSlot = &gApp.m_BillingMailSlot;
	CHANNEL* pChannel = sInitNetworkChannel(BILLING_PROXY, &pMailSlot,
		&gApp.m_BillingServerCommandTable, &gApp.m_BillingClientCommandTable);
	ASSERT_RETFALSE(pChannel != 0);
	AppSetBillingChannel(pChannel);
	return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInitNetworkPatchChannel(
								void)
{
	CHANNEL* pChannel;

	ASSERT_RETFALSE(AppGetPatchChannel() == NULL);

	MAILSLOT * pMailSlot = &gApp.m_PatchMailSlot;

	pChannel = sInitNetworkChannel(
		PATCH_SERVER,
		&pMailSlot,
		&gApp.m_PatchServerCommandTable,
		&gApp.m_PatchClientCommandTable);
	ASSERT_RETFALSE(pChannel != NULL);
	AppSetPatchChannel(pChannel);
	ConnectionManagerSetDisconnectCallback(pChannel, PatchClientDisconnectCallback);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInitAuthTicket(
	void)
{
#if !ISVERSION(DEVELOPMENT)
	return AuthTicketInit(NULL, AppGetAuthenticationIp(), gApp.userName, gApp.passWord);
#else
	BOOL bUseGlobalDefinition = AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT); // or the global definition tells us to use it.

	if ( bUseGlobalDefinition ) 
	{
		GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal();
		return AuthTicketInit(NULL, pDefinition->szAuthenticationServer, gApp.userName, gApp.passWord);
	}

	return AuthTicketInit(NULL, /**/AppGetAuthenticationIp()/*/"10.1.40.127"/**/, gApp.userName, gApp.passWord);
#endif
}
/*
Actions on state transition:
	AuthTicketInit(); (sInitAuthTicket)
	ConnectionManagerInit();
	sInitNetworkPatchChannel();
	//PatchClientConnect(); //NOT!
	sInitNetworkChatChannel();
	sInitNetworkGameChannel();
	CharacterSelectStartLogin();
	GameClientInit()?  should be done by prime loop?  nah.

Polling actions during states:
AuthTicketGetState()
AuthProcessMessages()
Checks if various channels are authenticated
CharacterSelectProcessMessages()
Checks if the game channel is charselected
*/


//----------------------------------------------------------------------------
// Perform whatever state transition action should occur.
// Currently, the state machine is purely linear with no branches.
// Thus, we can enforce state ordering.
//----------------------------------------------------------------------------
static BOOL sConnectionStateInitNewState(CONNECTION_STATE eNewState)
{
	ASSERTX_RETFALSE(eNewState > sConnectionState.m_eState, "Out of order connection state change.");
	
	switch(eNewState)
	{
	case CONNECTION_STATE_STARTUP:
		ASSERT_MSG("Switching to this state isn't supported.  Just Free() and Init()");
		break;
	case CONNECTION_STATE_GETTING_TICKET:
		{
			CC_ONLY(ASSERT_RETFALSE(sInitAuthTicket() ) );
			break;
		}
	case CONNECTION_STATE_CONNECTING_TO_PATCH:
		{
			ASSERT_RETFALSE(sInitNetworkPatchChannel() );
			break;
		}
	case CONNECTION_STATE_PATCHING:
		{
			//PatchClientConnect() is called repeatedly--no need to do anything here
			break;
		}
	case CONNECTION_STATE_CONNECTING_TO_BILLING:
		{
			ASSERT_RETFALSE(sInitNetworkBillingChannel());
			c_BillingBeginLogin();
			break;
		}
	case CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME:
		{
			ASSERT_RETFALSE(sInitNetworkChatChannel() );
			ASSERT_RETFALSE(sInitNetworkGameChannel() );
			break;
		}
	case CONNECTION_STATE_SELECTING_CHARACTER:
		{
			CC_ONLY(ASSERT_RETFALSE(CharacterSelectStartLogin(AppGetGameChannel() ) ));
			break;
		}
	case CONNECTION_STATE_CONNECTED:
		{
			extern BOOL GameClientInit();
			ASSERT_RETFALSE(GameClientInit());
			break;
		}

	default:
		ASSERT_MSG("Unhandled new connection state!");
		return FALSE;
	}
	sConnectionState.m_eState = eNewState;
	return TRUE;
}

//----------------------------------------------------------------------------
// verify that channels are in the expected state for current connection state
//----------------------------------------------------------------------------
BOOL ConnectionStateVerifyExpectedChannelStates()
{
	CONNECTION_STATE eState = sConnectionState.m_eState;

	if (AppIsPatching() && eState >= CONNECTION_STATE_PATCHING && eState <= CONNECTION_STATE_CONNECTED)
		if (ConnectionManagerGetChannelState(AppGetPatchChannel()) != CHANNEL_STATE_AUTHENTICATED)
			return FALSE;
	if (c_BillingIsEnabled() && eState >= CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME && eState <= CONNECTION_STATE_CONNECTED)
		if (ConnectionManagerGetChannelState(AppGetBillingChannel()) != CHANNEL_STATE_AUTHENTICATED)
			return FALSE;
	if (eState >= CONNECTION_STATE_SELECTING_CHARACTER && eState <= CONNECTION_STATE_CONNECTED)
		if (ConnectionManagerGetChannelState(AppGetChatChannel()) != CHANNEL_STATE_AUTHENTICATED)
			return FALSE;
	if (eState == CONNECTION_STATE_CONNECTED)
		if (ConnectionManagerGetChannelState(AppGetGameChannel()) != CHANNEL_STATE_CHARACTER_SELECTED)
			return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SendChannelKeepAliveMsgs(void)
{
	static const time_t uKeepAlivePeriod = 60; // seconds
	static time_t uLastKeepAliveTime = 0;

	time_t uCurrentTime = time(0);
	if (uCurrentTime - uLastKeepAliveTime < uKeepAlivePeriod)
		return;
	uLastKeepAliveTime = uCurrentTime;
	
	CONNECTION_STATE eState = sConnectionState.m_eState;
	// do NOT send patch keep alives while patching, because we want to be able to patch backwards to old versions
	// of the server that don't support the message (message was added in HGL patch .6)
	if (AppIsPatching() && eState >= CONNECTION_STATE_CONNECTING_TO_BILLING && eState <= CONNECTION_STATE_CONNECTED)
		PatchClientSendKeepAliveMsg();
	if (c_BillingIsEnabled() && eState >= CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME && eState <= CONNECTION_STATE_CONNECTED)
		c_BillingSendKeepAliveMsg();
	if (eState >= CONNECTION_STATE_SELECTING_CHARACTER && eState <= CONNECTION_STATE_CONNECTED)
		c_ChatNetSendKeepAlive();
}

/*
Polling actions during states:
AuthTicketGetState()
AuthProcessMessages()
PatchClientConnect() //Yes, that function is poorly named.
Checks if various channels are authenticated
CharacterSelectProcessMessages()
Checks if the game channel is charselected
*/

//----------------------------------------------------------------------------
// Repeatedly called while in a given state.  
// Returns CONNECTION_RESULT_DONE if we should advance in state,
// CONNECTION_RESULT_CONTINUING if we need to stay in the state,
// and CONNECTION_RESULT_FAILURE if everything went to hell.
//----------------------------------------------------------------------------
static CONNECTION_RESULT sConnectionStateProcessState()
{
	CONNECTION_STATE eState = sConnectionState.m_eState;

	switch(eState)
	{
	case CONNECTION_STATE_STARTUP:
		return CONNECTION_RESULT_DONE; //always!
	case CONNECTION_STATE_GETTING_TICKET:
		{
			NOT_CC(return CONNECTION_RESULT_DONE);
			AuthTicketState eAuthState = AuthTicketGetState();
			switch(eAuthState)
			{
			case AUTH_TICKET_STATE_IN_PROGRESS:
				return CONNECTION_RESULT_CONTINUING;
			case AUTH_TICKET_STATE_SUCCEEDED:
				return CONNECTION_RESULT_DONE;
			default:
				return CONNECTION_RESULT_FAILURE;
			}
		}
	case CONNECTION_STATE_CONNECTING_TO_PATCH:
		{
			ASSERT(AppIsPatching());
			AuthProcessMessages();
			switch(ConnectionManagerGetChannelState(AppGetPatchChannel()))
			{
			case     CHANNEL_STATE_CONNECTING:
			case	  CHANNEL_STATE_CONNECTED:
				return CONNECTION_RESULT_CONTINUING;
			case	  CHANNEL_STATE_AUTHENTICATED:
				return CONNECTION_RESULT_DONE;
			default:
				return CONNECTION_RESULT_FAILURE;
			}
		}
	case CONNECTION_STATE_PATCHING:
		{
			ASSERT(AppIsPatching());
			BOOL bDone = FALSE, bRestart = FALSE;
			if(!PatchClientConnect(bDone, bRestart))//, bIsFillPakClient))
			{
				return CONNECTION_RESULT_FAILURE;
			}

			if(bRestart)
			{
				extern void AppRestartForPatchServer();
				AppRestartForPatchServer();

				return CONNECTION_RESULT_CONTINUING; //only so we can get back to the main loop and restart!
			}
			if(bDone) 
			{
				return CONNECTION_RESULT_DONE;
			}
			else return CONNECTION_RESULT_CONTINUING;
		}
	case CONNECTION_STATE_CONNECTING_TO_BILLING:
		{
			ASSERT(c_BillingIsEnabled());
			AuthProcessMessages();
			c_BillingNetProcessMessages();
			CHANNEL_STATE eChannelState = ConnectionManagerGetChannelState(AppGetBillingChannel());
			switch (eChannelState) {
				case CHANNEL_STATE_CONNECTING:
				case CHANNEL_STATE_CONNECTED:
					return CONNECTION_RESULT_CONTINUING;
				case CHANNEL_STATE_AUTHENTICATED:
					if (c_BillingLoginInProgress()) {
						// must send an account status request to begin communication on the channel. the server can't 
						// send this unsolicited, or it will cause a bullshit race condition, in which the message can 
						// be received by the client before the sAuthCleanupChannel has switched command tables.
						if (!c_BillingHasPendingRequestAccountStatus())
							c_BillingSendMsgRequestAccountStatus();
						return CONNECTION_RESULT_CONTINUING;
					}
					if (SubscriptionTypeAllowsService(c_BillingGetSubscriptionType()))
						return CONNECTION_RESULT_DONE;
			}
			return CONNECTION_RESULT_FAILURE;
		}
	case CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME:
		{
			AuthProcessMessages();
			CHANNEL_STATE eChatState = ConnectionManagerGetChannelState(AppGetChatChannel());
			CHANNEL_STATE eGameState = ConnectionManagerGetChannelState(AppGetGameChannel());
			if(eGameState == CHANNEL_STATE_AUTHENTICATED &&
				eChatState == CHANNEL_STATE_AUTHENTICATED)
			{
				return CONNECTION_RESULT_DONE;
			}
			if((eChatState != CHANNEL_STATE_AUTHENTICATED	&&
				eChatState != CHANNEL_STATE_CONNECTED		&&
				eChatState != CHANNEL_STATE_CONNECTING)	||
				(eGameState != CHANNEL_STATE_AUTHENTICATED	&&
				eGameState != CHANNEL_STATE_CONNECTED		&&
				eGameState != CHANNEL_STATE_CONNECTING) )
			{
				return CONNECTION_RESULT_FAILURE;
			}
			return CONNECTION_RESULT_CONTINUING;
		}

	case CONNECTION_STATE_SELECTING_CHARACTER:
		{
			CharacterSelectProcessMessages(
				&sConnectionState.m_GameLoadBalanceMailSlot,
				AppGetGameChannel() );
			switch(ConnectionManagerGetChannelState(AppGetGameChannel()) )
			{
			case CHANNEL_STATE_AUTHENTICATED:
				return CONNECTION_RESULT_CONTINUING;
			case CHANNEL_STATE_CHARACTER_SELECTED:
				return CONNECTION_RESULT_DONE;
			default:
				return CONNECTION_RESULT_FAILURE;
			}
		}
	case CONNECTION_STATE_CONNECTED:
		return CONNECTION_RESULT_CONTINUING; //We're connected, don't need to change states.

	default:
		{
			ASSERT_MSG("Unhandled connection state!");
			return CONNECTION_RESULT_FAILURE;
		}
	}
}

//----------------------------------------------------------------------------
// Where the rubber hits the road.  Poll the connection state via
// sConnectionStateProcessState().  If we're done, switch states.
// Otherwise, continue processing.  Fail if, we, uuh, fail.
//
// bAdvanceState allows us to not initialize the next state once we finish
// with the present, hopefully letting the game catch up and adjust its
// state before we advance ours.
//----------------------------------------------------------------------------
CONNECTION_STATE ConnectionStateUpdate(BOOL bAdvanceState)
{
	CONNECTION_RESULT eResult = sConnectionStateProcessState();
	if (!ConnectionStateVerifyExpectedChannelStates()) 
		eResult = CONNECTION_RESULT_FAILURE;

	if(eResult == CONNECTION_RESULT_FAILURE)
	{
		TraceVerbose(0, "Connection loop failed in state %d", sConnectionState.m_eState);
		if (sConnectionState.m_eState == CONNECTION_STATE_GETTING_TICKET)
			sConnectionState.m_eState = CONNECTION_STATE_AUTHTICKET_FAILED;
		else if (sConnectionState.m_eState == CONNECTION_STATE_CONNECTING_TO_BILLING)
			sConnectionState.m_eState = CONNECTION_STATE_BILLING_FAILED;
		else if (sConnectionState.m_eState == CONNECTION_STATE_CONNECTING_TO_PATCH)
			sConnectionState.m_eState = CONNECTION_STATE_FIRST_SERVER_CONNECTION_FAILED;
		else
			sConnectionState.m_eState = CONNECTION_STATE_DISCONNECTED;
	}
	else if(eResult == CONNECTION_RESULT_DONE)
	{
		CONNECTION_STATE eNewConnectionState = CONNECTION_STATE(sConnectionState.m_eState + 1);

		// conditionally skip patch and/or billing states
		if (!AppIsPatching() && eNewConnectionState == CONNECTION_STATE_CONNECTING_TO_PATCH)
			eNewConnectionState = CONNECTION_STATE_CONNECTING_TO_BILLING;
		if (!c_BillingIsEnabled() && eNewConnectionState == CONNECTION_STATE_CONNECTING_TO_BILLING)
			eNewConnectionState = CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME;

		if(!bAdvanceState) return eNewConnectionState; //If requested, don't advance the state, just let them know we're ready to.

		ASSERT_DO(sConnectionStateInitNewState(eNewConnectionState))
		{
			TraceError("Connection loop failed initializing state %d while in state %d\n",
				eNewConnectionState, sConnectionState.m_eState);
			sConnectionState.m_eState = CONNECTION_STATE_DISCONNECTED;
		}

	}
	//If the result == CONNECTION_RESULT_CONTINUING, do nothing.

	return sConnectionState.m_eState;	
}

//----------------------------------------------------------------------------
// Repeatedly updates the state until we get to the desired state.
// Note: this is primarily for single player.  For closed server, almost
// all state advances require outside input, so this will fail.
//----------------------------------------------------------------------------
#define MAX_UPDATES_PER_ADVANCE 20

BOOL ConnectionStateAdvanceToState(CONNECTION_STATE eStateDesired)
{
	ASSERT(AppGetType() == APP_TYPE_SINGLE_PLAYER);

	CONNECTION_STATE eState = sConnectionState.m_eState;

	ASSERT_RETFALSE(eStateDesired >= eState);

	int i = 0;
	while(i < MAX_UPDATES_PER_ADVANCE &&
		eState != eStateDesired)
	{
		eState = ConnectionStateUpdate();
		i++;
	}
	if(eState != eStateDesired)
		return FALSE;
	else
		return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionStateSetCharacterName(const WCHAR *szName, unsigned int nMaxSize)
{
	PStrCopy(sConnectionState.m_szChararacter, MAX_CHARACTER_NAME,
		szName, nMaxSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionStateGetCharacterName(WCHAR *szName, unsigned int nMaxSize)
{
	PStrCopy(szName, nMaxSize,
		sConnectionState.m_szChararacter, MAX_CHARACTER_NAME);
}
#endif //!ISVERSION(SERVER_VERSION)
