//----------------------------------------------------------------------------
// NetworkConnections.cpp
// (C)Copyright 2007, Ping0 Interactive Limited. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "NetworkConnections.h"
#include "NetworkConnections.cpp.tmh"

using namespace std;


//----------------------------------------------------------------------------
// connection entry for a required service
//----------------------------------------------------------------------------
struct REQUIRED_SERVICE
{
	int							nId;		//	server instance, for hash
	struct REQUIRED_SERVICE *	pNext;		//	for hash

	CONNECT_STATUS				Status;		//	connection state status
	SVRTYPE						SvrType;	//	server type when found through missing list
	IMuxClient *				muxClient;	//	service connection
	SERVER_MAILBOX_PTR			Mailbox;	//	the mailbox to associate with the connection
};

//----------------------------------------------------------------------------
// lock types
//----------------------------------------------------------------------------
DEF_HLOCK(NetConsProvidersLock, HLCriticalSection)
END_HLOCK

//----------------------------------------------------------------------------
// SERVER CONNECTIONS
// THREAD SAFETY: all methods fully thread safe
// DESC: responsible for maintaining state for all required services of	a single
//			server being hosted by the server runner framework.
//		 also holds the NETSRV object ptrs for all offered services as a 
//			convinience to the net layer runner wrapper.
//----------------------------------------------------------------------------
struct NET_CONS
{
	NET_CONS(struct MEMORYPOOL * pool,
		CONNECTION_TABLE *  connectionTable,
		SVRTYPE				serverType,
		SVRINSTANCE			serverInstance,
		SVRMACHINEINDEX		serverMachineIndex );
	~NET_CONS(void);

	//	running parent server info, set at creation
	CONNECTION_TABLE *			m_connectionTable;
	SVRTYPE						m_serverType;
	SVRINSTANCE					m_serverInstance;
	SVRMACHINEINDEX				m_serverMachineIndex;

	//	required services data
	NetConsProvidersLock		m_providersLock;
	CHash<REQUIRED_SERVICE> *	m_allProviders[ TOTAL_SERVER_COUNT ];
	PList<REQUIRED_SERVICE*>	m_disconnectedProviders;

	// map of client SERVICEUSERIDs, which allow sending response to clients with only a SVRID
	CCriticalSection m_ClientServiceUserIdMapLock; 
	typedef hash_map_mp<SVRID, SERVICEUSERID>::type TSvrIdServiceUserIdMap;
	TSvrIdServiceUserIdMap m_ClientServiceUserIdMap;

	//	offered services data
	MuxServer *					m_offeredServices[ TOTAL_SERVER_COUNT ][ MAX_SVR_INSTANCES_PER_MACHINE ];

	//	mem pool
	struct MEMORYPOOL *			m_pool;
	
private:
	NET_CONS(const NET_CONS&); // noncopyable
	NET_CONS& operator=(const NET_CONS&); // noncopyable
};


NET_CONS::NET_CONS(
		struct MEMORYPOOL * pool,
		CONNECTION_TABLE *  connectionTable,
		SVRTYPE				serverType,
		SVRINSTANCE			serverInstance,
		SVRMACHINEINDEX		serverMachineIndex ) :
	m_ClientServiceUserIdMapLock(true),
	m_ClientServiceUserIdMap(TSvrIdServiceUserIdMap::key_compare(), pool)
{
	ASSERT( connectionTable );
	ASSERT( serverType			!= INVALID_SVRTYPE );
	ASSERT( serverInstance		!= INVALID_SVRINSTANCE );
	ASSERT( serverMachineIndex	!= INVALID_SVRMACHINEINDEX );

	//	init all members
	m_pool				= pool;
	m_connectionTable	= connectionTable;
	m_serverType			= serverType;
	m_serverInstance		= serverInstance;
	m_serverMachineIndex	= serverMachineIndex;

	//  zero pointer arrays
	memclear(m_allProviders, sizeof(m_allProviders));
	memclear(m_offeredServices, sizeof(m_offeredServices));

	//	init data structures
	m_providersLock.Init();
	m_disconnectedProviders.Init();
}


NET_CONS::~NET_CONS(void)
{
	//	free the required services
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		//	free the table
		CHash<REQUIRED_SERVICE> * & entry = m_allProviders[ ii ];
		if( entry )
		{
			HashFree( (*entry) );
			FREE( m_pool, entry );
			entry = NULL;
		}
	}

	//	free all members
	m_providersLock.Free();
	m_disconnectedProviders.Destroy( m_pool );
}


NetConsAutolock::NetConsAutolock(NET_CONS *pCons)
{
	m_pCons = pCons;
	if(m_pCons)
		m_pCons->m_providersLock.Enter();
}


NetConsAutolock::~NetConsAutolock()
{
	if(m_pCons)
		m_pCons->m_providersLock.Leave();
}
//----------------------------------------------------------------------------
// PRIVATE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sNetConsHashSize(
	int expectedProviders )
{
	return (unsigned int)( 1.15f * expectedProviders );
}


//----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NET_CONS * NetConsInit(
	struct MEMORYPOOL * pool,
	CONNECTION_TABLE *  connectionTable,
	SVRTYPE				serverType,
	SVRINSTANCE			serverInstance,
	SVRMACHINEINDEX		serverMachineIndex )
{
	ASSERT_RETNULL( connectionTable );
	ASSERT_RETNULL( serverType			!= INVALID_SVRTYPE );
	ASSERT_RETNULL( serverInstance		!= INVALID_SVRINSTANCE );
	ASSERT_RETNULL( serverMachineIndex	!= INVALID_SVRMACHINEINDEX );
	return MEMORYPOOL_NEW(pool) NET_CONS(pool, connectionTable, serverType, serverInstance, serverMachineIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsFree(
	NET_CONS * toFree )
{
	if (toFree)
		MEMORYPOOL_DELETE(toFree->m_pool, toFree);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsCloseActiveConnections(
	NET_CONS *	toClose,
	NETCOM *	net )
{
	ASSERT_RETURN( toClose );
	ASSERT_RETURN( net );

	NetConsAutolock lock = toClose;

	//	close net servers
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		for( int jj = 0; jj < MAX_SVR_INSTANCES_PER_MACHINE; ++jj )
		{
			if( toClose->m_offeredServices[ii][jj] )
			{
				toClose->m_offeredServices[ii][jj]->Destroy();
				toClose->m_offeredServices[ii][jj] = NULL;
			}
		}
	}

	//	close required services connections

	//	loop over all possible
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		if( toClose->m_allProviders[ii] )
		{
			//	loop over all type instance connections
			REQUIRED_SERVICE * itr = toClose->m_allProviders[ii]->GetFirstItem();
			while( itr )
			{
				//	close the connection
				if( itr->muxClient != NULL ) {
					itr->muxClient->Release();
				}

				//	release the mailbox
				itr->Mailbox = NULL;

				//	get the next entry
				itr = toClose->m_allProviders[ii]->GetNextItem( itr );
			}

			//	clear all connections
			toClose->m_allProviders[ii]->Clear();
		}
	}

	//	clear the outstanding required services
	toClose->m_disconnectedProviders.Clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONNECTION_TABLE * NetConsGetConnectionTable(
	NET_CONS * cons )
{
	ASSERT_RETNULL( cons );
	return cons->m_connectionTable;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRTYPE NetConsGetSvrType(
	NET_CONS * cons )
{
	ASSERT_RETX( cons, INVALID_SVRTYPE );
	return cons->m_serverType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRINSTANCE	NetConsGetSvrInstance(
	NET_CONS * cons )
{
	ASSERT_RETX( cons, INVALID_SVRINSTANCE );
	return cons->m_serverInstance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRMACHINEINDEX	NetConsGetSvrMachineIndex(
	NET_CONS * cons )
{
	ASSERT_RETX( cons, INVALID_SVRMACHINEINDEX );
	return cons->m_serverMachineIndex;
}

//----------------------------------------------------------------------------
// NOTE: adds all instances to disconnected list
//----------------------------------------------------------------------------
BOOL NetConsRegisterProviderType(
	NET_CONS *	cons,
	SVRTYPE		toRegister,
	DWORD		providerInstanceCount )
{
	ASSERT_RETFALSE( cons );
	ASSERT_RETFALSE( toRegister < TOTAL_SERVER_COUNT );
	if(providerInstanceCount == 0)
	{
		TraceError(
			"Server %!ServerType! requires %!ServerType! but no instances of %!ServerType! have been registered registered.",
			cons->m_serverType,
			toRegister,
			toRegister);
		return FALSE;
	}

	NetConsAutolock lock = cons;

	//	allocate the table
	ASSERTX_RETFALSE( !cons->m_allProviders[ toRegister ], "Server type already registered." );
	cons->m_allProviders[ toRegister ] = (CHash<REQUIRED_SERVICE>*)MALLOCZ( cons->m_pool, sizeof( CHash<REQUIRED_SERVICE> ) );
	ASSERT_RETFALSE( cons->m_allProviders[ toRegister ] );

	//	init the table
	HashInit( (*cons->m_allProviders[ toRegister ]), cons->m_pool, sNetConsHashSize( providerInstanceCount ) );
	HashPrealloc( (*cons->m_allProviders[ toRegister ]), providerInstanceCount);

	//	populate the hash
	for( SVRINSTANCE ii = 0; ii < providerInstanceCount; ++ii )
	{
		//	add an entry for this instance
		REQUIRED_SERVICE * added = HashAdd( (*cons->m_allProviders[ toRegister ]), ii );
		ASSERT_GOTO(added,Error);
		added->Status  = CONNECT_STATUS_DISCONNECTED;
		added->SvrType = toRegister;
		added->muxClient   = NULL;
		added->Mailbox.Init();
	}

	//	add all entries to the disconnected list
	REQUIRED_SERVICE * itr = cons->m_allProviders[toRegister]->GetFirstItem();
	while( itr )
	{
		cons->m_disconnectedProviders.PListPushTailPool(cons->m_pool, itr);
		//	get the next entry
		itr = cons->m_allProviders[toRegister]->GetNextItem( itr );
	}

	return TRUE;

Error:
	if( cons->m_allProviders[toRegister] )
	{
		//	clear all connections
		HashFree((*cons->m_allProviders[toRegister]));
		FREE(cons->m_pool, cons->m_allProviders[toRegister]);
		cons->m_allProviders[toRegister] = NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
// NOTE: adds to disconnected providers list of status == disconnected
//----------------------------------------------------------------------------
void NetConsSetProviderStatusAndNetClient(
	NET_CONS *		cons,
	SVRTYPE			svrType,
	SVRINSTANCE		svrInstance,
	CONNECT_STATUS	status,
	IMuxClient*		pClient )
{
	ASSERT_RETURN( cons );
	ASSERT_RETURN( svrType != INVALID_SVRTYPE );
	ASSERT_RETURN( svrInstance != INVALID_SVRINSTANCE );

	NetConsAutolock lock = cons;

	ASSERTX_RETURN(cons->m_allProviders[ svrType ], "Attempting to set provider status when provider not registered.");

	//	get the entry
	REQUIRED_SERVICE * entry = HashGet( (*cons->m_allProviders[ svrType ]), svrInstance );
	ASSERT_RETURN( entry );

	//	set info
	entry->Status = status;
	entry->muxClient  = pClient;

	//	see if we need to add it to disconnected list
	if( status == CONNECT_STATUS_DISCONNECTED )
	{
		cons->m_disconnectedProviders.PListPushTailPool(cons->m_pool, entry);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
IMuxClient * NetConsGetProviderNetClient(
	NET_CONS *	cons,
	SVRTYPE		type,
	SVRINSTANCE instance )
{
	ASSERT_RETX( cons,								NULL );
	ASSERT_RETX( type < TOTAL_SERVER_COUNT,			NULL );
	ASSERT_RETX( instance != INVALID_SVRINSTANCE,	NULL );

	IMuxClient* toRet = NULL;

	NetConsAutolock lock = cons;

	ASSERTX_RETNULL( cons->m_allProviders[ type ], "Attempting to get provider connection when provider not registered." );

	REQUIRED_SERVICE * entry = HashGet( (*cons->m_allProviders[ type ]), instance );
	ASSERTX_RETNULL( entry, "Provider instance not available." );

	toRet = entry->muxClient;
	ASSERTX_RETNULL( toRet, "Mising connection to server." );

	toRet->AddRef();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONNECT_STATUS NetConsGetProviderStatus(
	NET_CONS *	cons,
	SVRTYPE		type,
	SVRINSTANCE	instance )
{
	ASSERT_RETX( cons,								CONNECT_STATUS_DISCONNECTED );
	ASSERT_RETX( type < TOTAL_SERVER_COUNT,			CONNECT_STATUS_DISCONNECTED );
	ASSERT_RETX( instance != INVALID_SVRINSTANCE,	CONNECT_STATUS_DISCONNECTED );

	NetConsAutolock lock = cons;

	if( !cons->m_allProviders[type] )
		return CONNECT_STATUS_DISCONNECTED;

	REQUIRED_SERVICE * entry = HashGet( (*cons->m_allProviders[ type ]), instance );
	if( !entry )
		return CONNECT_STATUS_DISCONNECTED;

	return entry->Status;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsSetProviderMailbox(
	NET_CONS *			cons,
	SVRTYPE				type,
	SVRINSTANCE			instance,
	SERVER_MAILBOX *	mailbox )
{
	ASSERT_RETURN( cons );
	ASSERT_RETURN( type < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( instance != INVALID_SVRINSTANCE );

	NetConsAutolock lock = cons;

	ASSERT_RETURN( cons->m_allProviders[ type ] );

	//	set the entry
	REQUIRED_SERVICE * toSet = HashGet( (*cons->m_allProviders[ type ]), instance );
	ASSERT_RETURN( toSet );
	toSet->Mailbox = mailbox;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SERVER_MAILBOX * NetConsGetProviderMailbox(
	NET_CONS *		cons,
	SVRTYPE			type,
	SVRINSTANCE		instance )
{
	ASSERT_RETNULL( cons );
	ASSERT_RETNULL( type < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( instance != INVALID_SVRINSTANCE );

	NetConsAutolock lock = cons;

	ASSERT_RETNULL(cons->m_allProviders[type]);

	//	get the entry
	REQUIRED_SERVICE * entry = HashGet( (*cons->m_allProviders[ type ]), instance );
	ASSERT_RETNULL( entry );
	return entry->Mailbox.Ptr();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD NetConsGetDisconnectedProviderCount(
	NET_CONS * cons )
{
	ASSERT_RETZERO( cons );
	return cons->m_disconnectedProviders.Count();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsIterateDisconnectedProviders(
	NET_CONS * cons,
	IMuxClient* (*fpProviderItr)( NET_CONS *, SVRTYPE, SVRINSTANCE, SERVER_MAILBOX * ) )
{
	ASSERT_RETURN( cons );
	ASSERT_RETURN( fpProviderItr );

	NetConsAutolock lock = cons;

	PList<REQUIRED_SERVICE*>::USER_NODE * itr = cons->m_disconnectedProviders.GetNext( NULL );
	while( itr )
	{
		if( itr->Value->Status == CONNECT_STATUS_DISCONNECTED )
		{
			//	need a connection
			itr->Value->muxClient = fpProviderItr( cons, itr->Value->SvrType, (SVRINSTANCE)itr->Value->nId, itr->Value->Mailbox.Ptr() );

			//	handle connect
			if( itr->Value->muxClient == NULL )
			{
				//	connect failed, leave it in the list
				itr->Value->Status = CONNECT_STATUS_DISCONNECTED;
				itr = cons->m_disconnectedProviders.GetNext( itr );
			}
			else
			{
				//	connect success or pending
				itr->Value->Status = CONNECT_STATUS_CONNECTING;
				itr = cons->m_disconnectedProviders.RemoveCurrent( itr );
			}
		}
		else
		{
			//	duplicate entry
			itr = cons->m_disconnectedProviders.RemoveCurrent( itr );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsSetProvidedServicePtr(
	NET_CONS *		cons,
	SVRTYPE			type,
	SVRMACHINEINDEX index,
	MuxServer *		netSvr )
{
	ASSERT_RETURN( cons );
	ASSERT_RETURN( type < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( index < MAX_SVR_INSTANCES_PER_MACHINE );

	cons->m_offeredServices[ type ][ index ] = netSvr;

	TraceVerbose(
		TRACE_FLAG_SRUNNER_NET,
		"Assigned server ptr %p to type %d, index %d\n",
		netSvr,
		type,
		index);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MuxServer *	NetConsGetProvidedServicePtr(
				NET_CONS *		cons,
				SVRTYPE			type,
				SVRMACHINEINDEX index )
{
	ASSERT_RETNULL( cons );
	ASSERT_RETNULL( type < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( index < MAX_SVR_INSTANCES_PER_MACHINE );

	return cons->m_offeredServices[ type ][ index ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int NetConsSendToClient(
	NET_CONS *		pCons,
	SVRMACHINEINDEX machineIndex,
	SERVICEUSERID   userId,
	BYTE *			pBuffer,
	DWORD			dwBufSize)
{
	SVRTYPE type = ServiceUserIdGetUserType(userId);
    MuxServer *pServer = NetConsGetProvidedServicePtr(pCons, type, machineIndex);

	ASSERT_RETFALSE(pServer);
    return pServer->Send(ServiceUserIdGetConnectionId(userId),
                         pBuffer,
                         dwBufSize);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int NetConsSendToMultipleClients(
	NET_CONS *			pCons,
	SVRTYPE				serverType,
	SVRMACHINEINDEX     machineIndex,
	SVRCONNECTIONID *	connectedClients,
	UINT				numClients,
	BYTE *				pBuffer,
	DWORD				dwBufSize)
{
	MuxServer *pServer = NetConsGetProvidedServicePtr(pCons, serverType, machineIndex);

	ASSERT_RETFALSE(pServer);
    return pServer->SendMultipoint((MUXCLIENTID*)connectedClients,numClients,pBuffer, dwBufSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NetConsDisconnectClient(
	NET_CONS *		pCons,
	SVRMACHINEINDEX machineIndex,
	SERVICEUSERID   userId)
{
	SVRTYPE type = ServiceUserIdGetUserType(userId);
	MuxServer *pServer = NetConsGetProvidedServicePtr(pCons, type, machineIndex);

	ASSERT_RETFALSE(pServer);
    return pServer->Disconnect(ServiceUserIdGetConnectionId(userId));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * NetConsGetClientContext(
	NET_CONS* pCons,
	SVRMACHINEINDEX machineIndex,
	SERVICEUSERID   userId)
{
	SVRTYPE type = ServiceUserIdGetUserType(userId);
	MuxServer *pServer = NetConsGetProvidedServicePtr(pCons, type, machineIndex);

	ASSERT_RETNULL(pServer);
    return pServer->GetReadContext(ServiceUserIdGetConnectionId(userId));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SERVICEUSERID NetConsGetClientServiceUserId(NET_CONS* pNetCons, SVRID SvrId)
{
	ASSERT_RETZERO(pNetCons);
	CSAutoLock Lock(&pNetCons->m_ClientServiceUserIdMapLock);
	NET_CONS::TSvrIdServiceUserIdMap::iterator it = pNetCons->m_ClientServiceUserIdMap.find(SvrId);
	return it != pNetCons->m_ClientServiceUserIdMap.end() ? it->second : 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsClientAttached(NET_CONS* pNetCons, SERVICEUSERID ServiceUserId)
{
	ASSERT_RETURN(pNetCons);
	CSAutoLock Lock(&pNetCons->m_ClientServiceUserIdMapLock);
	SVRID SvrId = ServiceUserIdGetServer(ServiceUserId);
	pNetCons->m_ClientServiceUserIdMap.insert(make_pair(SvrId, ServiceUserId));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NetConsClientDetached(NET_CONS* pNetCons, SERVICEUSERID ServiceUserId)
{
	ASSERT_RETURN(pNetCons);
	CSAutoLock Lock(&pNetCons->m_ClientServiceUserIdMapLock);
	pNetCons->m_ClientServiceUserIdMap.erase(ServiceUserIdGetServer(ServiceUserId));
}
