//----------------------------------------------------------------------------
// c_connmgr.cpp
//
// contains....Connection Manager !
//
// Last modified : $Author: kklemmick $
//            at : $DateTime: 2008/03/06 11:23:40 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "list.h"
#include "c_connmgr.h"
#include "c_authticket.h"
#include "c_authentication.h"
#include "EmbeddedServerRunner.h"
#include "mailslot.h"
#include <ServerSuite/Login/LoginDef.h>

static SIMPLE_DYNAMIC_ARRAY<CHANNEL*> sChannels;
static MEMORYPOOL                    *sPool;
static NETCOM                        *sNetCom;
static BOOL                           sbInitialized = FALSE;
static CCriticalSection				  sLock;


struct CHANNEL {
    protected:
    long m_refCount;
    public:
    BOOL               bIsLocal;
    BOOL               bAuthPending;
    NETCLIENTID        netClientId;
    SVRTYPE            serverType;
    CHANNEL_STATE      channelState;

    NET_COMMAND_TABLE *current_request_table;
    NET_COMMAND_TABLE *current_response_table;
    MAILSLOT          *current_mailslot;

    PList<NET_COMMAND_TABLE*> requestTableStack;
    PList<NET_COMMAND_TABLE*> responseTableStack;
    PList<MAILSLOT*>          mailslotStack;
    MEMORYPOOL               *pPool;

	void* (*fnDisconnect)(void* pArg);

	struct AuthTicketContext *pContext; //rjd added
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    ~CHANNEL()
    {
        requestTableStack.Destroy(pPool);
        responseTableStack.Destroy(pPool);
        mailslotStack.Destroy(pPool);
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    explicit CHANNEL(void) : 
            m_refCount(1),
            bIsLocal(FALSE),
            bAuthPending(FALSE),
            netClientId(INVALID_NETCLIENTID),
            channelState(CHANNEL_STATE_INVALID),
            current_request_table(NULL),
            current_response_table(NULL),
            current_mailslot(NULL),
			fnDisconnect(NULL)
    {
    }
    void AddRef(void)
    {
        InterlockedExchangeAdd(&m_refCount,1);
    }
    void Release(void)
    {
        if(InterlockedDecrement(&m_refCount) == 0)
        {
            FREE_DELETE(sPool,this,CHANNEL);
        }
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    void Init(__in __notnull MEMORYPOOL *pool, 
			  struct AuthTicketContext *context = NULL)
    {

        this->pPool = pool;
		this->pContext = context;
        requestTableStack.Init();
        responseTableStack.Init();
        mailslotStack.Init();
		
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PushRequestTable(NET_COMMAND_TABLE *tbl)
    {
       NET_COMMAND_TABLE *prev = current_request_table;
       current_request_table = tbl;

       if(!requestTableStack.PListPushHeadPool(pPool,tbl))
       {
            current_request_table = prev;
            return FALSE;
       }
       TraceVerbose(0,"Channel %p (server type %d) pushing request table %p\n",this,this->serverType,tbl);
                    
       return TRUE;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PopRequestTable(NET_COMMAND_TABLE *& tbl)
    {
       if(! requestTableStack.PopHead(tbl))
       {
            return FALSE;
       }
       PList<NET_COMMAND_TABLE*>::USER_NODE *node = NULL;

       node = requestTableStack.GetNext(NULL);

       if(node)
       {
           current_request_table = node->Value;
       }
       else
       {
            current_request_table = NULL;
       }
       TraceVerbose(0,"Channel %p popping request table to %p was %p\n",this,
                   current_request_table,tbl);

       return TRUE;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PushResponseTable(NET_COMMAND_TABLE *tbl)
    {
       NET_COMMAND_TABLE *prev = current_response_table;
       current_response_table = tbl;

       if(!responseTableStack.PListPushHeadPool(pPool,tbl))
       {
            current_response_table = prev;
            return FALSE;
       }

       TraceVerbose(0,"Channel %p pushing response table %p\n",this,tbl);
       return TRUE;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PopResponseTable(NET_COMMAND_TABLE *& tbl)
    {
       if(! responseTableStack.PopHead(tbl))
       {
            return FALSE;
       }
       PList<NET_COMMAND_TABLE*>::USER_NODE *node = NULL;

       node = responseTableStack.GetNext(NULL);

       ASSERTX_RETFALSE(node,"could not pop response table");
       if(node)
       {
           current_response_table = node->Value;
       }


       TraceVerbose(0,"Channel %p popping response table to %p, was %p\n",this,
                   current_response_table,tbl);

       return TRUE;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PushMailSlot(MAILSLOT *mailslot)
    {
       MAILSLOT *prev = current_mailslot;
       current_mailslot = mailslot;

       if(!mailslotStack.PListPushHeadPool(pPool,mailslot))
       {
            current_mailslot = prev;
            return FALSE;
       }


       TraceVerbose(0,"Channel %p pushing mailslot %p\n",this,mailslot);
       return TRUE;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    BOOL PopMailSlot(MAILSLOT *& mailslot)
    {
       if(! mailslotStack.PopHead(mailslot))
       {
            return FALSE;
       }
       PList<MAILSLOT*>::USER_NODE *node = NULL;

       node = mailslotStack.GetNext(NULL);

       ASSERTX_RETFALSE(node,"could not pop mailslot");

       current_mailslot = node->Value;


       TraceVerbose(0,"Channel %p popping mailslot to %p, was %p\n",this,
                   current_mailslot,mailslot);

        return TRUE;
    }
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	inline void InitSendEncryption(
		BYTE * key,
		size_t nBytes,
		ENCRYPTION_TYPE eEncryptionTypeClientToServer)
	{
		NetInitClientSendEncryption(sNetCom, netClientId, key, nBytes,
			eEncryptionTypeClientToServer);
	}

	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	inline void InitReceiveEncryption(
		BYTE * key,
		size_t nBytes,
		ENCRYPTION_TYPE eEncryptionTypeServerToClient)
	{
		NetInitClientReceiveEncryption(sNetCom, netClientId, key, nBytes,
			eEncryptionTypeServerToClient);
	}
};

static BOOL sConnectionManagerOpenLocalChannel(CHANNEL *,const ServerInformation&);
static void sConnectionManagerConnectCallback(NETCLIENTID idClient, void *data, DWORD status);
static void sInitSendEncryption(__in __notnull CHANNEL *, __in __notnull BYTE *key, size_t nBytes, ENCRYPTION_TYPE);
static void sInitReceiveEncryption(__in __notnull CHANNEL *, __in __notnull BYTE *key, size_t nBytes, ENCRYPTION_TYPE);
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _BOT
BOOL ConnectionManagerInit(MEMORYPOOL *pool, APP_TYPE appType, UINT max_clients)
#else
BOOL ConnectionManagerInit(MEMORYPOOL *pool, APP_TYPE appType)
#endif
{
    ConnectionManagerClose();

	sLock.Init();

    AuthInit(pool);

    ArrayInit(sChannels, pool, 4);

    sPool = pool;

#ifdef _BOT
    sNetCom = EmbeddedServerRunnerInitNetCom(NULL,"prime",sizeof(CHANNEL**), max_clients + SERVER_ACCEPT_BACKLOG);
#else
    sNetCom = EmbeddedServerRunnerInitNetCom(NULL,"prime",sizeof(CHANNEL**));
#endif

    if(!sNetCom)
    {
        TraceError("Could not init netcom in %s\n",__FUNCTION__);
        return sbInitialized;
    }
    switch (appType)
    {
        case APP_TYPE_SINGLE_PLAYER:
            if(!EmbeddedServerRunnerInit(NULL,
                                         sNetCom,
                                         TRUE,
                                         NULL,
                                         0))
            {
                TraceError("EmbeddedServerRunnerInit failed\n");
            }
            sbInitialized = TRUE;
            break;
#if !ISVERSION(CLIENT_ONLY_VERSION) && !ISVERSION(RETAIL_VERSION)
        case APP_TYPE_OPEN_SERVER:
            if(!EmbeddedServerRunnerInit(NULL,
                                         sNetCom,
                                         FALSE,
                                         AppGetServerIp(),
                                         AppGetServerPort()))
            {
                TraceError("EmbeddedServerRunnerInit failed\n");
            }
            sbInitialized = TRUE;
            break;
#endif
        case APP_TYPE_CLOSED_CLIENT:
        case APP_TYPE_OPEN_CLIENT:
            sbInitialized = TRUE;
            break;
        default:
            ASSERT(FALSE);
            break;
    }
    return sbInitialized;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionManagerClose(void)
{
    if(!sbInitialized)
    {
        return;
    }
    AuthClose();
    for(unsigned int ui = 0; ui <  sChannels.Count(); ui++)
    {
        ASSERT(sChannels[ui]->requestTableStack.Count() == 1);
        ASSERT(sChannels[ui]->responseTableStack.Count() == 1);
        ASSERT(sChannels[ui]->mailslotStack.Count() == 1);
        ConnectionManagerCloseChannel(sChannels[ui]);
    }
    sChannels.Destroy();

    EmbeddedServerRunnerFree();
    EmbeddedServerRunnerFreeNetCom(sNetCom);

	sLock.Free();

    sbInitialized = FALSE;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHANNEL *ConnectionManagerOpenChannel(const ServerInformation& serverInfo,
										struct AuthTicketContext *pContext)
{
    CHANNEL *newChannel = (CHANNEL*)MALLOC_NEW(sPool,CHANNEL);
    char addrStr[MAX_PATH];

    ASSERT_GOTO(sbInitialized,Error);
    ASSERT_GOTO(newChannel,Error);

    ArrayAddItem(sChannels, newChannel);

    newChannel->Init(sPool, pContext);

	//Push every command table on the submitted linked list to the channel.
	int nRepetitions = 0;
	const ServerInformation *pServerTableInfo = &serverInfo;
	do
	{
		ASSERT_BREAK(nRepetitions < MAX_TABLE_LIST);
		ASSERT_GOTO(newChannel->PushRequestTable(pServerTableInfo->request_table),Error);
		ASSERT_GOTO(newChannel->PushResponseTable(pServerTableInfo->response_table),Error);
		ASSERT_GOTO(newChannel->PushMailSlot(pServerTableInfo->mailslot),Error);
		pServerTableInfo = pServerTableInfo->pSecondaryServerInformation;
		nRepetitions++;
	}
	while(pServerTableInfo);

    newChannel->channelState = CHANNEL_STATE_CONNECTING;
    newChannel->serverType = serverInfo.type;


    if(serverInfo.szIP == NULL) //local
    {
        ASSERT_GOTO(sConnectionManagerOpenLocalChannel(newChannel,serverInfo),Error);
		return newChannel;
    }

    PStrPrintf(addrStr,
               MAX_PATH,
               "%d_%s:%d\n",
               serverInfo.type,
               PStrSkipWhitespace(serverInfo.szIP),
               serverInfo.port);

    net_clt_setup clt_setup;

    structclear(clt_setup);

    clt_setup.name = addrStr;
    clt_setup.remote_addr = serverInfo.szIP;
    clt_setup.remote_port = serverInfo.port;
    clt_setup.mailslot = newChannel->current_mailslot;
    clt_setup.cmd_tbl = serverInfo.response_table;
    clt_setup.fp_ConnectCallback = sConnectionManagerConnectCallback;
    clt_setup.user_data   = &newChannel;
	clt_setup.nagle_value = NAGLE_OFF;
	clt_setup.disable_send_buffer = TRUE;
    
	{
		CSAutoLock lock(&sLock);
		newChannel->netClientId = NetCreateClient(sNetCom,clt_setup);
	}

    if(newChannel->netClientId == INVALID_NETCLIENTID)
    {
        TraceError("Could not create net client to server %s\n",addrStr);
        ASSERT_GOTO(FALSE,Error);
    }


    return newChannel;

Error:
    if (newChannel)
    {
        ArrayRemoveByValue(sChannels, newChannel);		// slow!!!

        if(newChannel->netClientId != INVALID_NETCLIENTID)
        {
            NetCloseClient(sNetCom,newChannel->netClientId);
        }
        newChannel->Release();
        newChannel = NULL;
    }
    return newChannel;
    
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConnectionManagerOpenLocalChannel(CHANNEL *newChannel,
                                               const ServerInformation& serverInfo)
{
    BOOL bRet = FALSE;

    char typeStr[128];

    newChannel->bIsLocal = TRUE;

    PStrPrintf(typeStr,sizeof(typeStr),"ST_%02d",serverInfo.type);

    newChannel->netClientId = EmbeddedServerRunnerAttachLocalClient(
                                typeStr,
                                serverInfo.response_table,
                                newChannel->current_mailslot,
                                NULL);
                                    
    if(newChannel->netClientId == INVALID_NETCLIENTID)
    {
        TraceError("failed to create local client\n");
    }
    else
    {
        newChannel->channelState = CHANNEL_STATE_CONNECTED;

        if(AuthStartLogin(newChannel,serverInfo.type,NULL, newChannel->pContext))
        {
            newChannel->channelState = CHANNEL_STATE_AUTHENTICATED;
            bRet = TRUE;
        }
        else
        {
            newChannel->channelState = CHANNEL_STATE_CONNECT_FAILED;
        }
    }

    return bRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionManagerCloseChannel(CHANNEL* pChannel)
{
    ASSERT_RETURN(sbInitialized);
    NetCloseClient(sNetCom,pChannel->netClientId);

    ArrayRemoveByValue(sChannels, pChannel);		// slow!!!

    pChannel->Release();

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerTranslateMessageForRecv(CHANNEL * chan,
                                       BYTE *msg,
                                       unsigned int msgSize,
                                       MSG_STRUCT* outbuf)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(chan);

    return NetTranslateMessageForRecv(chan->current_response_table,
                                      msg,
                                      msgSize,
                                      outbuf);

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerSend(CHANNEL *channel, MSG_STRUCT *msg, NET_CMD command)
{
    BOOL bRet = FALSE;

    BYTE buf_data[MAX_SMALL_MSG_SIZE];

    BYTE_BUF_NET bbuf(buf_data,MAX_SMALL_MSG_SIZE);

    if(!channel)
        return FALSE;

    ASSERT_RETFALSE(sbInitialized);

    if( channel->channelState != CHANNEL_STATE_CONNECTED &&
        channel->channelState != CHANNEL_STATE_AUTHENTICATED &&
		channel->channelState != CHANNEL_STATE_CHARACTER_SELECTED)
    {
        TraceError("Send requested on channel in invalid state. "
                   "Target server type: %d", channel->serverType );
        return FALSE;
    }

    unsigned int size = 0;

    if(!NetTranslateMessageForSend(channel->current_request_table,command,msg,
                                  size,bbuf))
    {
        TraceError("Could not translate message for send. cmd %d\n",command);
    }
    else {
        bRet = (NetSend(sNetCom, channel->netClientId, bbuf.GetBuf(),
                                                             bbuf.GetCursor())> 0);
    }
    return bRet;
}
//----------------------------------------------------------------------------
// For debugging and security testing: bypass the connection manager's
// opinion of what is a valid message, and send whatever garbage you want.
//
// Allow messages to be sent in any state except disconnected and invalid so
// we can test servers during the login process.
//
// This function is debugging purposes only.
//----------------------------------------------------------------------------
#ifdef _BOT
BOOL ConnectionManagerSendBypassTranslation(
	CHANNEL *channel,
	const BYTE *data,
	unsigned int size)
{
	BOOL bRet = FALSE;

	if(!channel)
		return FALSE;

	ASSERT_RETFALSE(sbInitialized);

	if( IsChannelDisconnected(channel->channelState) )
	{
		TraceError("Send bypass requested on channel in disconnected state. "
			"Target server type: %d", channel->serverType );
		return FALSE;
	}

	bRet = (NetSend(sNetCom, channel->netClientId, data, size)> 0);

	return bRet;
}
#endif
//----------------------------------------------------------------------------
// This function is debugging purposes only.  (Violates encapsulation)
// Get the current command table for the channel so we can figure out
// what sort of mischief to send on it.
//
// Note that there are two command tables: client to server,
// and server to client.  This gets client to server.
//----------------------------------------------------------------------------
#ifdef _BOT
NET_COMMAND_TABLE * ConnectionManagerGetChannelCurrentRequestTable(
	CHANNEL *channel)
{
	if( IsChannelDisconnected(channel->channelState) ) return NULL;
	return channel->current_request_table;
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHANNEL_STATE ConnectionManagerGetChannelState(CHANNEL *channel)
{
    if(!sbInitialized)
    {
        TraceGameInfo("Channel state is uninitialized. This is an error, if game is not being restarted\n");
        return CHANNEL_STATE_INVALID;
    }

    if(channel)
    {
        return channel->channelState;
    }
    else
    {
        return CHANNEL_STATE_INVALID;
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAuthInitEncryption( CHANNEL *channel)
{
	BYTE bSendEncryptionKey[MAX_KEY_SIZE];
	DWORD nEncryptionKeySize;

	ASSERT_RETFALSE(AuthTicketGetEncryptionKey(channel->serverType, 
                                               bSendEncryptionKey, 
                                               MAX_KEY_SIZE, 
                                               nEncryptionKeySize,
											   channel->pContext));
	if(nEncryptionKeySize == 0)
    {
        return FALSE;
    }

	ENCRYPTION_TYPE eEncryptionTypeClientToServer = ENCRYPTION_TYPE_NONE, 
		eEncryptionTypeServerToClient = ENCRYPTION_TYPE_NONE;

	AuthTicketGetEncryptionType(channel->serverType, 
		eEncryptionTypeClientToServer, eEncryptionTypeServerToClient,
		channel->pContext);

	sInitSendEncryption(channel, bSendEncryptionKey,
		nEncryptionKeySize/2,
		eEncryptionTypeClientToServer);

	sInitReceiveEncryption(channel,bSendEncryptionKey + nEncryptionKeySize/2,  
		nEncryptionKeySize/2,
		eEncryptionTypeServerToClient);
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAuthDone(CHANNEL *channel,CHANNEL_STATE state)
{
    ASSERT_RETURN(channel);

	// if it's disconnected, don't let the caller try to tell us everything is ok
	if (!IsChannelDisconnected(channel->channelState))
	    channel->channelState = state;
    channel->bAuthPending = FALSE;

    if( (channel->channelState == CHANNEL_STATE_AUTHENTICATED) &&
		(AppGetType() == APP_TYPE_CLOSED_CLIENT))
    {
        sAuthInitEncryption(channel);
    }

	channel->Release();
}
//----------------------------------------------------------------------------
// Note: lovely refcounting isn't seamlessly integrated with characterselect,
// so we're opening the door to all sorts of funky errors here.  On the other
// hand, it's not integrated with the game loop either, and that works fine.
// So hopefully we're good.  This is only called from the main thread.
//----------------------------------------------------------------------------
void CharacterSelectDone(CHANNEL *pChannel, CHANNEL_STATE state)
{
	ASSERT_RETURN(pChannel);
	ASSERT_RETURN(pChannel->channelState == CHANNEL_STATE_AUTHENTICATED);
	pChannel->channelState = state;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConnectionManagerConnectCallback(NETCLIENTID idClient, 
                                       void *data,
                                       DWORD status)
{
	CSAutoLock lock(&sLock);

    CHANNEL *channel = *(static_cast<CHANNEL**>(data));

    ASSERT_RETURN(channel);

    switch(status)
    {
        case CONNECT_STATUS_DISCONNECTED:
        case CONNECT_STATUS_DISCONNECTING:

            channel->channelState =  CHANNEL_STATE_DISCONNECTED;

            if(channel->bAuthPending)
            {
                AuthChannelAbortLoginAttempt(channel);
                channel->bAuthPending = FALSE;
			} else {
				if (channel->fnDisconnect) {
					channel->fnDisconnect(NULL);
				}
			}

            break;
        case CONNECT_STATUS_CONNECTING:
            channel->channelState =  CHANNEL_STATE_CONNECTING;
            break;
        case CONNECT_STATUS_CONNECTED:
            channel->AddRef();
            channel->bAuthPending = TRUE;
            channel->channelState = CHANNEL_STATE_CONNECTED;
            if (!AuthStartLogin(channel, channel->serverType, sAuthDone, channel->pContext))
				channel->channelState = CHANNEL_STATE_CONNECT_FAILED;
            break;
        case CONNECT_STATUS_CONNECTFAILED:
            channel->channelState =  CHANNEL_STATE_CONNECT_FAILED;
            break;
        default:
            ASSERT(FALSE);
            break;
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MAILSLOT *ConnectionManagerGetMailSlotForChannel(CHANNEL *chan)
{
    ASSERT_RETNULL(sbInitialized);
    ASSERT_RETNULL(chan);

    return chan->current_mailslot;

}
//
// functions to change the states of channels
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPushRequestTable(CHANNEL * channel,
                                          NET_COMMAND_TABLE* tbl)
{
    ASSERT_RETFALSE(channel);
    ASSERT_RETFALSE(tbl);

    return channel->PushRequestTable(tbl);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPushResponseTable(CHANNEL * channel,
                                          NET_COMMAND_TABLE* tbl)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(channel);
    ASSERT_RETFALSE(tbl);


    if( channel->PushResponseTable(tbl))
    {
        NetSetClientResponseTable(sNetCom,channel->netClientId, 
                                      channel->current_response_table);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPushMailSlot(CHANNEL * channel,
                                          MAILSLOT* mailslot)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(channel);
    ASSERT_RETFALSE(mailslot);

    if( channel->PushMailSlot(mailslot))
    {
        NetSetClientMailbox(sNetCom,channel->netClientId,
                                    channel->current_mailslot);
    }
    else
    {
        FALSE;
    }
    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPopRequestTable(__in __notnull CHANNEL *channel,
                                          __in __notnull NET_COMMAND_TABLE*& tbl)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(channel);
    return channel->PopRequestTable(tbl);

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPopResponseTable(__in __notnull CHANNEL *channel,
                                          __in __notnull NET_COMMAND_TABLE*& tbl)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(channel);
    if( channel->PopResponseTable(tbl))
    {
        NetSetClientResponseTable(sNetCom,channel->netClientId, 
                                          channel->current_response_table);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConnectionManagerChannelPopMailSlot(__in __notnull CHANNEL *channel,
                                          __in __notnull MAILSLOT*& mailslot)
{
    ASSERT_RETFALSE(sbInitialized);
    ASSERT_RETFALSE(channel);
    if( channel->PopMailSlot(mailslot))
    {
        NetSetClientMailbox(sNetCom,channel->netClientId,
                                    channel->current_mailslot);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInitSendEncryption(__in __notnull CHANNEL *channel,
												__in __notnull BYTE *key,
												size_t nBytes,
												ENCRYPTION_TYPE eEncryptionTypeClientToServer)
{
	ASSERT_RETURN(channel);
	ASSERT_RETURN(key);
	channel->InitSendEncryption(key, nBytes, eEncryptionTypeClientToServer);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInitReceiveEncryption(__in __notnull CHANNEL *channel,
												   __in __notnull BYTE *key,
												   size_t nBytes,
												   ENCRYPTION_TYPE eEncryptionTypeServerToClient)
{
	ASSERT_RETURN(channel);
	ASSERT_RETURN(key);
	channel->InitReceiveEncryption(key, nBytes, eEncryptionTypeServerToClient);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConnectionManagerSetDisconnectCallback(
	CHANNEL* pChannel,
	void* (*fnDisconnect)(void* pArg))
{
	if (pChannel) {
		pChannel->fnDisconnect = fnDisconnect;
	}
}

#endif // !ISVERSION(SERVER_VERSION)

