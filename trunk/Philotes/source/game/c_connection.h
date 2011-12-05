#ifndef _C_CONNECTION_H_
#define _C_CONNECTION_H_

// get the auth ticket
// connect to patch
// finish patching
// connect to chat
// connect to game loadbalancer
// finish selecting your character (enforce you are still connected to chat)
// connected to game, chat, and patch now.  CONNECTED!

enum CONNECTION_STATE
{
	CONNECTION_STATE_STARTUP,
	CONNECTION_STATE_GETTING_TICKET,
	CONNECTION_STATE_CONNECTING_TO_PATCH,
	CONNECTION_STATE_PATCHING,
	CONNECTION_STATE_CONNECTING_TO_BILLING,
	CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME,
	CONNECTION_STATE_SELECTING_CHARACTER,
	CONNECTION_STATE_CONNECTED,

	CONNECTION_STATE_DISCONNECTED,
	CONNECTION_STATE_AUTHTICKET_FAILED,
	CONNECTION_STATE_BILLING_FAILED,
	CONNECTION_STATE_FIRST_SERVER_CONNECTION_FAILED, //Currently, this would be patching.

#if ISVERSION(DEVELOPMENT)
	CONNECTION_STATE_CONNECTING_TO_FILLPAK,
	CONNECTION_STATE_CONNECTED_TO_FILLPAK,
#endif
};

BOOL ConnectionStateInit(MEMORYPOOL *pPool);

void ConnectionStateFree();

CONNECTION_STATE ConnectionStateGetState();

BOOL ConnectionStateVerifyExpectedChannelStates();

void SendChannelKeepAliveMsgs();

CONNECTION_STATE ConnectionStateUpdate(BOOL bAdvanceState = TRUE);

#if ISVERSION(DEVELOPMENT)
CONNECTION_STATE FillPakClient_ConnectionStateUpdate(BOOL bAdvanceState = TRUE);
#endif

BOOL ConnectionStateAdvanceToState(CONNECTION_STATE eStateDesired);

void ConnectionStateSetCharacterName(const WCHAR *szName, unsigned int nMaxSize);

void ConnectionStateGetCharacterName(WCHAR *szName, unsigned int nMaxSize);

#endif //_C_CONNECTION_H_
