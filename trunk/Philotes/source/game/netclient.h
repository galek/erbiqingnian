//----------------------------------------------------------------------------
// netclient.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _NETCLIENT_H_
#define _NETCLIENT_H_

#ifndef _LIST_H_
#include "list.h"
#endif

//----------------------------------------------------------------------------
// NETCLIENTID64 STRUCTURES (extensions to client defined in net.cpp)
//----------------------------------------------------------------------------
struct NETCLT_USER : LIST_SL_NODE
{
    UNIQUE_ACCOUNT_ID           m_idAccount;
	NETCLIENTID64				m_idNetClient;
	APPCLIENTID					m_idAppClient;
	GAMECLIENTID				m_idGameClient;
	GAMEID						m_idGame;
	UINT16						m_billingInstance;
	WCHAR						szCharacter[MAX_CHARACTER_NAME];
	CReaderWriterLock_NS_FAIR	m_cReadWriteLock;

	BOOL                        m_bRemoveCalled;
	BOOL						m_bDetachCalled;

	//gameserver switching required:
	NETCLIENTID					m_netId;
	SOCKADDR_STORAGE			m_ipAddress;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID NetClientGetGameClientId(
		void * data );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetClientSetGameClientId(
		NETCLIENTID64 idNetClient, 
        GAMECLIENTID idGameClient );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetClientInitUserData(
		void * data );

void NetClientFreeUserData(
		void * data);

BOOL NetClientQueryDetatched(
		NETCLIENTID64 idClient);


//----------------------------------------------------------------------------
#endif // _NETCLIENT_H_