//----------------------------------------------------------------------------
// c_authentication.cpp
//
// authenticates a client with a server.
//
// Last modified : $Author: pmason $
//            at : $DateTime: 2007/12/07 14:56:19 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "mailslot.h"
#include "servers.h"
#include "globalindex.h"
#include "ServerRunnerAPI.h"
#include "accountbadges.h"	// CHB 2007.03.04
#include "c_connmgr.h"
#include "c_authticket.h"
#include "c_authentication.h"
#include "uix_menus.h"
#include "uix_components.h"
#include <ServerSuite/Login/LoginDef.h>
#include <ServerSuite/Login/LoginClientMsg.h>
#include <ServerSuite/Login/LoginServerMsg.h>

struct HIJACK_INFO {
    CHANNEL *channel;
    SVRTYPE  type;
    MAILSLOT mailslot;
    BOOL     bAborted;
    FP_AUTH_DONE_CALLBACK callback;
    NET_COMMAND_TABLE *prevloginReqTable;
    NET_COMMAND_TABLE *prevloginResponseTable;
    MAILSLOT *prevmailslot;
};

static SIMPLE_DYNAMIC_ARRAY<HIJACK_INFO *> sHijacks;
static BOOL                         sbInitialized = FALSE;
static MEMORYPOOL                  *sPool;
static CCriticalSection             sAuth_cs;

__checkReturn static HIJACK_INFO* sAuthHijackChannel(__in __notnull CHANNEL *channel, SVRTYPE type,
                               FP_AUTH_DONE_CALLBACK callback);

static BOOL sAuthRestoreChannel(HIJACK_INFO *hijack,
                                __in __notnull CHANNEL *channel);

static void sAuthCleanupChannel(unsigned int index,CHANNEL_STATE state);
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AuthInit(MEMORYPOOL *pool)
{
	sPool = pool;

    ArrayInit(sHijacks, pool, 4);

    sAuth_cs.Init();

    sbInitialized = TRUE;

    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthClose(void)
{
    if(!sbInitialized)
    {
        return;
    }

    if(sHijacks.Count() != 0)
    {
        CSAutoLock lock(&sAuth_cs);
        for(unsigned int ui = 0; ui < sHijacks.Count(); ui++)
        {
            sAuthCleanupChannel(ui,CHANNEL_STATE_CONNECT_FAILED);
        }
    }
	sHijacks.Destroy();
    sbInitialized = FALSE;
    sAuth_cs.Free();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AuthStartLogin(CHANNEL *channel,SVRTYPE type, 
					FP_AUTH_DONE_CALLBACK callback,
					struct AuthTicketContext *pContext)
{
    BOOL bRet = FALSE;
    DWORD credSize = MAX_LOGIN_CREDENTIAL_BUFFER_SIZE;

    ASSERT_RETFALSE(sbInitialized);

    MSG_LOGIN_CCMD_LOGIN msg;

    msg.serverType = (BYTE)type;
    if(AppGetType() == APP_TYPE_CLOSED_CLIENT)
    {
        ASSERT_RETFALSE(AuthTicketGetAuthCredentials(type,
                        &msg.credentialBuffer[0],
                        &credSize,
						pContext));
        MSG_SET_BLOB_LEN(msg,credentialBuffer,credSize);
        TraceDebugOnly("Auth ticket params: ticket %d\n", credSize);
    }

    HIJACK_INFO *hijack = sAuthHijackChannel(channel,type,callback);

    if(ConnectionManagerSend(channel,&msg,LOGIN_CCMD_LOGIN))
    {
        TraceInfo(0,"Started login for server type %d, channel %p\n",type,
                    channel);
        bRet = TRUE;
    }

    // If there is no callback, restore the channel tables and mailslot
    if(!callback)
    {
        sAuthRestoreChannel(hijack,channel);
        {
            CSAutoLock lock(&sAuth_cs);
            ArrayRemoveByValue(sHijacks, hijack);	// slow!!!
        }
    }
    return bRet;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HIJACK_INFO * sAuthHijackChannel(CHANNEL *channel, SVRTYPE type,
                        FP_AUTH_DONE_CALLBACK callback)
{
    TraceVerbose(0,"Hijacking channel %p for auth\n",channel);

    ASSERT_RETNULL(channel);

    HIJACK_INFO *hijack = (HIJACK_INFO*)MALLOCZ(sPool,sizeof(*hijack));

    ASSERT_RETNULL(hijack);

    hijack->channel = channel;
    hijack->callback = callback;
    hijack->type = type;

    if(!ConnectionManagerChannelPushRequestTable(channel,
                          hijack->prevloginReqTable = 
                          LoginGetClientRequestTable()))
    {
        TraceError("Could not push request table.\n");
        goto Error;
    }
    if(!ConnectionManagerChannelPushResponseTable(channel,
                          hijack->prevloginResponseTable =
                          LoginGetResponseTable() ))
    {
        TraceError("Could not push response table.\n");
        goto Error;
    }

    MailSlotInit(hijack->mailslot,NULL,MAX_SMALL_MSG_SIZE);

    hijack->prevmailslot = &hijack->mailslot;

    if(!ConnectionManagerChannelPushMailSlot(channel,&hijack->mailslot))
    {
        TraceError("Could not push mailslot.\n");
        goto Error;
    }
	{
		CSAutoLock lock(&sAuth_cs);
		ArrayAddItem(sHijacks, hijack);
	}

    return hijack;
Error:
    FREE(sPool,hijack);
    return NULL;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sAuthRestoreChannel(HIJACK_INFO *hijack, CHANNEL *channel)
{
    NET_COMMAND_TABLE  *pIgnore;
    MAILSLOT *pIgnoreMailSlot;

    ASSERT_RETFALSE(channel);

    TraceVerbose(0,"Restoring channel %p\n",channel);

    ASSERT(ConnectionManagerChannelPopRequestTable(channel,pIgnore));

	if(hijack)
	{
		ASSERT(pIgnore == hijack->prevloginReqTable);
		NetCommandTableFree(hijack->prevloginReqTable);
	}

    ASSERT(ConnectionManagerChannelPopResponseTable(channel,pIgnore));

	if(hijack)
	{
		ASSERT(pIgnore == hijack->prevloginResponseTable);
		NetCommandTableFree(hijack->prevloginResponseTable);
	}

    ASSERT(ConnectionManagerChannelPopMailSlot(channel,pIgnoreMailSlot));


    return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthChannelAbortLoginAttempt(CHANNEL *channel)
{
    ASSERT_RETURN(channel);

    CSAutoLock lock(&sAuth_cs);

    for(unsigned int ui = 0; ui <  sHijacks.Count(); ui++)
    {
        if(sHijacks[ui]->channel == channel)
        {
            TraceVerbose(0,"Aborting login on channel %p\n",sHijacks[ui]);
            sHijacks[ui]->bAborted = TRUE;
        }
    }
}
//----------------------------------------------------------------------------
//
// Call with sAuth_cs held !!
//----------------------------------------------------------------------------
static void sAuthCleanupChannel(unsigned int index,CHANNEL_STATE state)
{
    CSAutoLock lock(&sAuth_cs);

    ASSERT(sAuthRestoreChannel(sHijacks[index],sHijacks[index]->channel));

    HIJACK_INFO *ptr = sHijacks[index];

    if(ptr->callback)
    {
        ptr->callback(ptr->channel,state);
    }

    ArrayRemoveByIndex(sHijacks, index);

    FREE(sPool,ptr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAuthProcessMessage(MSG_STRUCT *msg, BOOL &bDone,
                                CHANNEL_STATE&state )
{
    switch(msg->hdr.cmd)
    {
        case LOGIN_SCMD_LOGIN_RESPONSE:
            //TODO really check the response.
			{
				MSG_LOGIN_SCMD_LOGIN_RESPONSE * responseMsg = 
					(MSG_LOGIN_SCMD_LOGIN_RESPONSE *)msg;
				if(responseMsg->status == LOGIN_STATUS_OK)
				{
					state = CHANNEL_STATE_AUTHENTICATED;
					bDone = TRUE;
					return true;
				}
				else
				{
                    TraceDebugOnly("Login failed\n");
					return false;
				}
				break;
			}
        default:
            ASSERT(FALSE);
			return false;
    }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AuthProcessMessages(void)
{
    if(!sbInitialized)
        return;
    CSAutoLock lock(&sAuth_cs);

    for(unsigned int ui = 0; ui <  sHijacks.Count(); ui++)
    {
        ASSERT_CONTINUE(sHijacks[ui]);

        CHANNEL_STATE state = CHANNEL_STATE_CONNECT_FAILED;

        MSG_BUF *msg = NULL,*head = NULL, *tail = NULL, *next = NULL;

		// if it's been aborted, we will be done after processing remaining messages
        BOOL  bAuthDoneForChannel = sHijacks[ui]->bAborted;

        MailSlotGetMessages(&sHijacks[ui]->mailslot, head, tail);
        next = head;

        while( (msg = next) != NULL)
        {
            next = msg->next;

            BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];

            if(!ConnectionManagerTranslateMessageForRecv(sHijacks[ui]->channel,
                                                  msg->msg,
                                                  msg->size,
                                                 (MSG_STRUCT*)message))
            {
                ASSERT(FALSE);
                bAuthDoneForChannel = TRUE;
                TraceError("Could not translate message\n");
                break;
            }

            if(!sAuthProcessMessage((MSG_STRUCT*)message,bAuthDoneForChannel,state))
                break;
        }

        if(msg != NULL && next)
        {
            MailSlotPushbackMessages(&sHijacks[ui]->mailslot,next,tail);
            tail = msg;
        }
        MailSlotRecycleMessages(&sHijacks[ui]->mailslot,head,tail);

        if(bAuthDoneForChannel) // Array is not the same. do the others in the next frame
        {
            sAuthCleanupChannel(ui,state);
            break;
        }
    }
}

#endif //!SERVER_VERSION
