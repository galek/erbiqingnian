//----------------------------------------------------------------------------
// c_connmgr.h
// 
// Modified by: $Author: mkalika $
// at         : $DateTime: 2007/12/11 11:35:38 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

#include "servers.h"
#include "ServerRunnerAPI.h"
#include "appcommon.h"


// This file contains the interface to a very simple connection creation API.
//
// The only wrinkle is that the "channels" created here support the concept of
// stacks for the incoming and outgoing message tables, as well as the
// mailslot to which incoming messages are being delivered. 
//
// If you "push" a table, it becomes current and the previous table is saved
// for later restoration via "pop".
//
// This mechanism allows the handing off of channels to other entities that
// may wish to take over message processing temporarily.
//

struct CHANNEL;


// These are the possible states a channel can be in. Use
// ConnectionManagerGetChannelState() to interrogate a channel.
enum CHANNEL_STATE
{
    CHANNEL_STATE_INVALID,
    CHANNEL_STATE_DISCONNECTED,
    CHANNEL_STATE_CONNECTING,
    CHANNEL_STATE_CONNECTED,
    CHANNEL_STATE_AUTHENTICATED,
	CHANNEL_STATE_CHARACTER_SELECTED,
    CHANNEL_STATE_CONNECT_FAILED
};

inline BOOL IsChannelDisconnected(CHANNEL_STATE state)
{
    return (state == CHANNEL_STATE_INVALID || 
            state == CHANNEL_STATE_DISCONNECTED ||
            state == CHANNEL_STATE_CONNECT_FAILED);
}

#define MAX_TABLE_LIST 16

//
// To create a channel, the caller must fill this structure out. Note that a
// mailslot is necessary. A default mailslot is no longer automatically
// created for the channel.
struct ServerInformation {
    char              *szIP; //set to NULL for "local" connections.
    unsigned short     port;
    SVRTYPE            type;
    NET_COMMAND_TABLE *request_table; 
    NET_COMMAND_TABLE *response_table;
    MAILSLOT          *mailslot;
	ServerInformation *pSecondaryServerInformation;
};

// ConnectionManager functions for creating/destroying channels.


// Init the ConnectionManager, with a pool type and an app type. 
#ifdef _BOT
BOOL ConnectionManagerInit(__in __notnull MEMORYPOOL *pool,APP_TYPE appType,UINT max_clients);
#else
BOOL ConnectionManagerInit(__in __notnull MEMORYPOOL *pool,APP_TYPE appType);
#endif

//
// Clean up ConnectionManager. Closes all active channels.
void ConnectionManagerClose(void);

//
// Open a channel to the server described by "info".
__checkReturn CHANNEL *ConnectionManagerOpenChannel(const ServerInformation& info,
								  struct AuthTicketContext *pContext = NULL);


// Close an open channel
void ConnectionManagerCloseChannel(__in __notnull CHANNEL* channel);

//
// Send a msg on a channel. The response table at top of the stack is used.
BOOL ConnectionManagerSend(__in __notnull CHANNEL *channel,
                           __in __notnull MSG_STRUCT *msg,
                                          NET_CMD command);

// Returns the channel's connection state.
CHANNEL_STATE ConnectionManagerGetChannelState(
                                    __in __notnull CHANNEL *channel);


// CHANNEL functions.
//

// Get the current mailslot.
MAILSLOT *ConnectionManagerGetMailSlotForChannel(__in __notnull CHANNEL *);
BOOL ConnectionManagerTranslateMessageForRecv(__in __notnull CHANNEL *,
                                       BYTE *,
                                       unsigned int,
                                       MSG_STRUCT*);


//
// Push a request table on the stack,making it the one used to translate all 
// outgoing messages.
BOOL ConnectionManagerChannelPushRequestTable(__in __notnull CHANNEL *,
                                          __in __notnull NET_COMMAND_TABLE*);

//
// Push a response table on the stack, making it the one used to translate all
// incoming messages.
BOOL ConnectionManagerChannelPushResponseTable(__in __notnull CHANNEL *,
                                          __in __notnull NET_COMMAND_TABLE*);

//
// Push a mailslot on the stack, making it the current mailslot that messages
// will be delivered to.
BOOL ConnectionManagerChannelPushMailSlot(__in __notnull CHANNEL *,
                                          __in __notnull MAILSLOT*);

//
// Pop a request table from the stack, restoring the previous table.
BOOL ConnectionManagerChannelPopRequestTable(__in __notnull CHANNEL *,
                                              __out NET_COMMAND_TABLE*&);
//
// Pop a response table from the stack,restoring the previous table.
BOOL ConnectionManagerChannelPopResponseTable(__in __notnull CHANNEL *,
                                              __out NET_COMMAND_TABLE*&);

//
// Pop a mailslot from the stack, restoring the previous mailslot.
// now on the top of the stack.
BOOL ConnectionManagerChannelPopMailSlot(__in __notnull CHANNEL *,
                                         __out MAILSLOT*&);

void ConnectionManagerSetDisconnectCallback(
	CHANNEL* pChannel,
	void* (*fnDisconnect)(void* pArg));