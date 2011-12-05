//----------------------------------------------------------------------------
// Prime v2.0
//
// s_netclient.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "clients.h"
#include "netclient.h"
#include "svrstd.h"
#include "GameServer.h"
#include "performance.h"
#include "s_message.h"
#include "c_message.h"

#if ISVERSION(SERVER_VERSION)
#include "svrdebugtrace.h"
#include "s_netclient.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// EXTERNS (s_network.cpp)
//----------------------------------------------------------------------------
extern BOOL SrvValidateMessageTable(void);
extern BOOL CltValidateMessageTable(void);


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
// net client functions

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientSetAppClientId(
	NETCLIENTID64 idNetClient,
	APPCLIENTID idAppClient)
{
	NETCLT_USER * pContext = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);

	//possible lock
	if (!pContext)
	{
		return FALSE;
	}

	pContext->m_cReadWriteLock.WriteLock();
	pContext->m_idAppClient = idAppClient;
	pContext->m_cReadWriteLock.EndWrite();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APPCLIENTID NetClientGetAppClientId(
	void * data)
{
	if (!data) 
	{
		return (APPCLIENTID)INVALID_CLIENTID;
	}

	NETCLT_USER * user = (NETCLT_USER *)data;

	user->m_cReadWriteLock.ReadLock();
	APPCLIENTID idAppClient = user->m_idAppClient;
	user->m_cReadWriteLock.EndRead();

	return idAppClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APPCLIENTID NetClientGetAppClientId(
	NETCLIENTID64 idClient)
{
	return NetClientGetAppClientId(SvrNetGetClientContext(idClient));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID NetClientGetGameId(
	void * data)
{
	ASSERT_RETVAL(data, INVALID_ID);
	NETCLT_USER * user = (NETCLT_USER *)data;

	user->m_cReadWriteLock.ReadLock();
	GAMEID idGame = user->m_idGame;
	user->m_cReadWriteLock.EndRead();

	return idGame;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID NetClientGetGameId(
	NETCLIENTID64 idClient)
{
    return NetClientGetGameId(SvrNetGetClientContext(idClient));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID NetClientGetGameClientId(
	void * data)
{
	if (!data)
	{
		return INVALID_GAMECLIENTID; //happens naturally upon client side disconnect
	}

	NETCLT_USER * user = (NETCLT_USER *)data;

	user->m_cReadWriteLock.ReadLock();
	GAMECLIENTID idGameClient = user->m_idGameClient;
	user->m_cReadWriteLock.EndRead();
	return idGameClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID NetClientGetGameClientId(
	NETCLIENTID64 idClient)
{
	return NetClientGetGameClientId(SvrNetGetClientContext(idClient));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIQUE_ACCOUNT_ID NetClientGetUniqueAccountId(
	void * data)
{
	if (!data)
	{
		return INVALID_UNIQUE_ACCOUNT_ID;
	}

	NETCLT_USER * user = (NETCLT_USER *)data;

	user->m_cReadWriteLock.ReadLock();
	UNIQUE_ACCOUNT_ID idAccount = user->m_idAccount;
	user->m_cReadWriteLock.EndRead();
	return idAccount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIQUE_ACCOUNT_ID NetClientGetUniqueAccountId(
	NETCLIENTID64 idClient)
{
	return NetClientGetUniqueAccountId(SvrNetGetClientContext(idClient));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRINSTANCE NetClientGetBillingInstance(
	NETCLIENTID64 idClient )
{
	NETCLT_USER* pNetcltUser = static_cast<NETCLT_USER*>(SvrNetGetClientContext(idClient));
	return pNetcltUser ? pNetcltUser->m_billingInstance : INVALID_SVRINSTANCE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientSetGameClientId(
	NETCLIENTID64 idNetClient, 
    GAMECLIENTID idGameClient)
{
	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
	//possible lock
	if (!user)
	{
		return FALSE;
	}

	user->m_cReadWriteLock.WriteLock();
	user->m_idGameClient = idGameClient;
	user->m_cReadWriteLock.EndWrite();
	
	TraceNetDebug(
		"NetClientSetGameClientId() -- netclient: %lli  gameclientid: %lli",
        idNetClient,
		idGameClient);

	return TRUE;
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientClearGame(
	NETCLIENTID64 idNetClient)
{
	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
	if (!user)
	{
		return FALSE;
	}

	TraceNetDebug("NetClientClearGame() -- netclient: %lli", idNetClient);
	user->m_cReadWriteLock.WriteLock();
	user->m_idGame = INVALID_ID;
	user->m_idGameClient = INVALID_GAMECLIENTID;

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE(serverContext);

	SvrNetAttachRequestMailbox(
		idNetClient,
		NULL );
	user->m_cReadWriteLock.EndWrite();	

	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientQueryDetached(void * data)
{
	if(!data)
	{
		return TRUE;
	}
	NETCLT_USER * user = (NETCLT_USER *) data;

	user->m_cReadWriteLock.ReadLock();
	BOOL toRet = user->m_bDetachCalled;
	user->m_cReadWriteLock.EndRead();
	return toRet;
}
BOOL NetClientQueryDetached(
	NETCLIENTID64 idClient)
{
	return NetClientQueryDetached(SvrNetGetClientContext(idClient));
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientQueryRemoved(void * data)
{
	ASSERT_RETTRUE(data);
	NETCLT_USER * user = (NETCLT_USER *) data;

	user->m_cReadWriteLock.ReadLock();
	BOOL toRet = user->m_bRemoveCalled;
	user->m_cReadWriteLock.EndRead();
	return toRet;
}
BOOL NetClientQueryRemoved(
	NETCLIENTID64 idClient)
{
	return NetClientQueryRemoved(SvrNetGetClientContext(idClient));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientSetGameAndName(
	NETCLIENTID64 idNetClient,
	GAMEID idGame,
	const WCHAR * wszPlayerName,
	SERVER_MAILBOX * mailbox)
{
	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
	
	if (!user)
	{
		return FALSE;
	}
	user->m_cReadWriteLock.WriteLock();
	if (user->m_idGame != INVALID_ID)
	{
		return FALSE;
	}
	user->m_idGame = idGame;

	if (wszPlayerName)
	{
		PStrCopy(user->szCharacter, wszPlayerName, MAX_CHARACTER_NAME);
	}
	user->m_cReadWriteLock.EndWrite();

    SvrNetAttachRequestMailbox(
		idNetClient,
		mailbox);

	return TRUE;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientGetName(
	NETCLIENTID64 idNetClient,
	WCHAR * wszPlayerName,
	unsigned int nMaxLength)
{
	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);

	if (!user)
	{
		return FALSE;
	}
	user->m_cReadWriteLock.ReadLock();

	if (wszPlayerName)
	{
		PStrCopy(wszPlayerName, user->szCharacter, nMaxLength);
	}
	user->m_cReadWriteLock.EndRead();

	return TRUE;	
}

//----------------------------------------------------------------------------
// Ensure we don't have terrible mismap between us, gameclient, and appclient
// Note: we cannot read the gameclient since we don't have the game lock,
// but we can verify that the gameclientid is consistent between app and net.
//----------------------------------------------------------------------------
#define ABR(x) if( !(x) ) {bRet = FALSE;}

BOOL NetClientValidateIdConsistency(
	NETCLIENTID64 idNetClient)
{
	if(idNetClient == INVALID_NETCLIENTID64) return TRUE;
	if(ServiceUserIdGetConnectionId(idNetClient) == INVALID_SVRCONNECTIONID) return TRUE;

	BOOL bRet = TRUE;

	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);

	if (!user)
	{
		return TRUE; //Nonexistence is consistency!
	}

	user->m_cReadWriteLock.ReadLock();
	GAMECLIENTID idGameClient = user->m_idGameClient;
	APPCLIENTID idAppClient = user->m_idAppClient;
	user->m_cReadWriteLock.EndRead();

	NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
	ABR(idNetClient == idNetClientFromAppClient);

	GAMECLIENTID idGameClientFromAppClient = ClientSystemGetClientGameId(AppGetClientSystem(), idAppClient);
	ABR(idGameClient == idGameClientFromAppClient);

	return bRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetClientInitUserData(
	void * data)
{
	ASSERT_RETURN(data);

	NETCLT_USER * user = (NETCLT_USER *)data;
	user->m_idAppClient = (APPCLIENTID)INVALID_CLIENTID;
	user->m_idGameClient = INVALID_GAMECLIENTID;
	user->m_idGame = INVALID_ID;
	user->m_billingInstance = INVALID_SVRINSTANCE;
	user->szCharacter[0] = 0;
	user->m_bDetachCalled = FALSE;
    user->m_bRemoveCalled = FALSE;
	user->m_idNetClient = INVALID_SERVICEUSERID;
	user->m_cReadWriteLock.Init();
	user->m_idAccount = INVALID_UNIQUE_ACCOUNT_ID;
}

void NetClientFreeUserData(
	void * data)
{
	ASSERT_RETURN(data);

	NETCLT_USER * user = (NETCLT_USER *)data;

	user->m_cReadWriteLock.WriteLock(); //overcautious, but better safe than sorry.
	user->m_bRemoveCalled = TRUE;
	user->m_cReadWriteLock.EndWrite();

	user->m_cReadWriteLock.Free();//If anything wants the lock, it'll die here anyway.
								  //So the lock really doesn't protect much.  But I'll use it anyway -rjd
}

