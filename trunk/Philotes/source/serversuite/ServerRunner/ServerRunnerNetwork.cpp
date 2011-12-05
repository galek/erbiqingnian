
// ServerRunnerNetwork.cpp
// (C)Copyright 2007, Ping0 Interactive Limited. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "NetworkConnections.h"
#include "fasttraversehash.h"
#include "ServerRunnerNetwork.cpp.tmh"


//----------------------------------------------------------------------------
// NET-CONS HASH CONTAINER STRUCT
//----------------------------------------------------------------------------
struct NET_CON_CONTAINER
{
	SVRID						id;
	struct NET_CON_CONTAINER *	next;
	struct NET_CON_CONTAINER *	traverse[2];

	NET_CONS *					NetCons;
};

typedef CFastTraverseHash<
			NET_CON_CONTAINER,
			SVRID,
			TOTAL_SERVER_COUNT * MAX_SVR_INSTANCES_PER_MACHINE>	CONS_HASH;


//----------------------------------------------------------------------------
// PER-CONNECTION DATA STRUCT
// DESC: stored with every network connection.
//----------------------------------------------------------------------------
struct CLIENT_DATA
{
	//	arbitrary data pointer for parent hosted server
	void *				ServerData;

	//	runner net data
	BOOL				RemoteMachineIsServiceProvider;	//	if TRUE, is service provider, if FALSE, is service user...

	SVRTYPE				RemoteServerType;
	SVRINSTANCE			RemoteServerInstance;

	SVRTYPE				LocalServerType;
	SVRMACHINEINDEX		LocalServerMachineIndex;

	SERVER_MAILBOX_PTR	MailboxPtr;

	IMuxClient         *pMuxClient;
    bool                bReleaseRefOnDisconnect;
    ~CLIENT_DATA() 
    {
        MailboxPtr = NULL;
    }
};


//----------------------------------------------------------------------------
// lock types
//----------------------------------------------------------------------------
DEF_HLOCK(RunnerNetAllConsLock, HLCriticalSection)
END_HLOCK


//----------------------------------------------------------------------------
// SERVER RUNNER NETWORK
//----------------------------------------------------------------------------
struct RUNNER_NET
{
	//	server info
	char							m_szLocalIP[MAX_NET_ADDRESS_LEN];	// our IP address

	//	net structure
	NETCOM *						m_Net;								// network structure

	//	command tables
	NET_COMMAND_TABLE *				m_cmdTables
										[ TOTAL_SERVER_COUNT ]
										[ TOTAL_SERVER_COUNT ]
										[ 2 ];							// in/out command tables for all server types & possible channels
	#define REQUEST_CMD_TBL_I		0									// server request command table index
	#define RESPONSE_CMD_TBL_I		1									// server response command table index

	//	network map
	NET_MAP *						m_netMap;							// address and port info for other servers

	//	all created net cons
	CONS_HASH						m_netCons;
	RunnerNetAllConsLock			m_netConsLock;

	//	mem pool
	struct MEMORYPOOL *				m_pool;
};


//----------------------------------------------------------------------------
// RUNNER NETWORK SINGELTON
//----------------------------------------------------------------------------
static RUNNER_NET * g_runnerNet = NULL;


//----------------------------------------------------------------------------
// NET FRAMEWORK CALLBACK DECLARATIONS
//----------------------------------------------------------------------------
static BOOL sNetAccept(DWORD,MUXCLIENTID,SOCKADDR_STORAGE&,SOCKADDR_STORAGE&,void *, void **,BYTE *,DWORD);
static BOOL sNetRemove( NETCLIENTID, void * );
static void sNetInitUserData( void * );
static void sNetConnectCallback( DWORD, PVOID );
static void sNetProcessImmediateCallback(DWORD,void*,MUXCLIENTID,BYTE*,DWORD);


//----------------------------------------------------------------------------
// INTERNAL FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static NET_COMMAND_TABLE * sRunnerNetGetCommandTable( 
	SVRTYPE providerType,
	SVRTYPE userType,
	BOOL	getRequestTable )
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERT_RETNULL( providerType < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( userType < TOTAL_SERVER_COUNT );

	//	get a reference to the tbl pointer
	NET_COMMAND_TABLE * & toRet = g_runnerNet->m_cmdTables
										[ providerType ]
										[ userType ]
										[ ( getRequestTable == 0 ) ];

	//	if we don't already have it, try to get it
	if( !toRet )
	{
		CONNECTION & connection = G_SERVER_CONNECTIONS[ providerType ].OfferedServices[ userType ];

		ASSERT_GOTO( connection.ConnectionSet, _no_table );
		ASSERT_GOTO( connection.GetInboundCmdTbl, _no_table );
		ASSERT_GOTO( connection.GetOutboundCmdTbl, _no_table );
		ASSERT_GOTO( connection.ConnectionType == CONNECTION_TYPE_SERVICE_PROVIDER, _no_table );

		//	get the table builder fp
		FP_GET_CMD_TBL fpTblBuilder = ( getRequestTable == 0 ) ? 
										connection.GetOutboundCmdTbl : 
										connection.GetInboundCmdTbl;
		ASSERT_GOTO( fpTblBuilder, _no_table );
		toRet = fpTblBuilder();
		ASSERT( toRet );
	}

_no_table:
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static IMuxClient* sRunnerNetTryClientConnect(
	SVRTYPE				userType,
	SVRMACHINEINDEX		userMachineIndex,
	SVRTYPE				providerType,
	SVRINSTANCE			providerInstance,
	SERVER_MAILBOX *	mailbox )
{
	ASSERT_RETNULL( g_runnerNet );

	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Attempting client connection from server %!ServerType! at machine index %hu to server %!ServerType! instance %hu",
		userType, userMachineIndex, providerType, providerInstance );

	//	get provider info
	SOCKADDR_STORAGE providerAddr;
	if( !NetMapGetServerAddrForUser(
			g_runnerNet->m_netMap,
			providerType, 
			providerInstance,
			userType,
			userMachineIndex,
			providerAddr ) )
	{
		//	they aren't registered with the net map yet
		TraceWarn(
			TRACE_FLAG_SRUNNER,
			"Server %!ServerType! inst. %hu not found for client connection from %!ServerType!",
			providerType, providerInstance, userType );
		return NULL;
	}

    //	The connection data
    CLIENT_DATA * connectionData = MALLOC_NEW( g_runnerNet->m_pool,CLIENT_DATA);
    if( connectionData )
    {
        connectionData->RemoteMachineIsServiceProvider	= TRUE;
        connectionData->LocalServerType					= userType;
        connectionData->LocalServerMachineIndex			= userMachineIndex;
        connectionData->RemoteServerType				= providerType;
        connectionData->RemoteServerInstance			= providerInstance;
        connectionData->MailboxPtr						= mailbox;
    }
    else
    {
        TraceError("Could not allocate client data.");
        return NULL;
    }
	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Created server runner connection context %#018I64x for client connection to %!ServerType! inst. %hu",
		(UINT64)connectionData, providerType, providerInstance );

    MuxClientConfig setup;
    WCHAR perfInstanceName[MAX_PERF_INSTANCE_NAME];
	structclear( setup );

    PStrPrintf(perfInstanceName,ARRAYSIZE(perfInstanceName),L"MuxClient:%02d-%02d",providerType,userType);
	//	setup standard members
	//	TODO: maybe change this to use a direct sockaddr_storage struct... or at least try to avoid the itoa conversion...
	char remoteAddr[ MAX_NET_ADDRESS_LEN ];
    RunnerNetSockAddrToString( remoteAddr, MAX_NET_ADDRESS_LEN, &providerAddr );
	setup.connect_callback	= sNetConnectCallback;
    setup.read_callback		= sNetProcessImmediateCallback;
	setup.server_addr		= remoteAddr;
	setup.server_port		= ntohs(SS_PORT(&providerAddr));
    setup.callback_context	= connectionData;
    setup.pNet				= g_runnerNet->m_Net;
    setup.perfInstanceName  = perfInstanceName;

	//	try to connect to the server
	IMuxClient* pClient =  IMuxClient::Create(g_runnerNet->m_pool);
	if( !pClient)
    {
        TraceError("Could not create MuxClient for connection to server %!ServerType!, inst %hu, at ip %!IPADDR!",
			providerType,
			providerInstance,
			((sockaddr_in*)&providerAddr)->sin_addr.s_addr );
		goto _error;
    }
	else
	{
		connectionData->pMuxClient = pClient;
        connectionData->bReleaseRefOnDisconnect = true;
		pClient->AddRef();
		if(!pClient->Connect(setup))
		{
			TraceError("Connect() failed\n");
			pClient->Release();
			goto _error;
		}
	}
	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Established connection to %!ServerType! inst. %hu, IMuxClient ptr: %#018I64x",
		providerType, providerInstance, (UINT64)pClient );
	return pClient;
_error:
    FREE_DELETE(g_runnerNet->m_pool,connectionData,CLIENT_DATA);
    if(pClient)
    {
        pClient->Release();
    }
    pClient = NULL;
	return pClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static MuxServer * sRunnerNetCreateNetServer(
	SOCKADDR_STORAGE & addr,
	void * context,
    WCHAR *perfInstanceName)
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERTX_RETNULL( g_runnerNet->m_szLocalIP[0],
		"Unable to create server as local IP address has not been set with the server runner network." );

    MuxServerConfig setup = {0};

	//	fill standard members
	setup.server_addr		= g_runnerNet->m_szLocalIP;
	setup.server_port		= ntohs(SS_PORT(&addr));
    setup.pNet				= g_runnerNet->m_Net;
    setup.pool				= g_runnerNet->m_pool;
    setup.accept_callback	= sNetAccept;
    setup.read_callback		= sNetProcessImmediateCallback;
    setup.accept_context	= context;
    setup.perfInstanceName  = perfInstanceName;

	//	create and return the server
	MuxServer * toRet = MuxServer::Create(setup);
    if(!toRet)
	{
		CHANNEL_SERVERS connInfo = { 0 };
		connInfo = NetMapGetPortUsers(SS_PORT(&addr));
		TraceError("Could not create MuxServer for %!ServerType! server offering a service to %!ServerType! on ip %s:%hu.",
			connInfo.OfferingType,
			connInfo.UserType,
			g_runnerNet->m_szLocalIP,
			ntohs(SS_PORT(&addr)) );
		ASSERT_MSG("Error creating mux server, see trace.");
    }
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static NETCOM *	sRunnerNetInitNetFramework(
	struct MEMORYPOOL * pool,
	const char *		baseName )
{
	//	init the struct
	net_setup setup;
	structclear( setup );

	//	fill our data
	setup.mempool			= pool;
	setup.name				= baseName;
	setup.max_clients		= RUNNER_NET_MAX_CLIENTS;
	setup.user_data_size	= sizeof( CLIENT_DATA )*10;

	//	create and return
	return NetInit( setup );
}


//----------------------------------------------------------------------------
// NETWORK FRAMEWORK CALLBACKS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sNetConnectCallback(
	DWORD		cltStatus, 
	PVOID		cData)
{
	ASSERT_RETURN( g_runnerNet );
	ASSERT_RETURN( g_runnerNet->m_netMap );

	//	get the client data
	CLIENT_DATA * cltData = (CLIENT_DATA*)cData;
	ASSERT_RETURN( cltData );

	TraceInfo(
		TRACE_FLAG_SRUNNER_NET,
		"CONNECT STATUS: %!ConnectStatus!, for attempted connection to server %!ServerType!, instance %hu, from local type %!ServerType!.",
		cltStatus,
		cltData->RemoteServerType,
		cltData->RemoteServerInstance,
		cltData->LocalServerType );


	//	get server net cons
	NET_CONS * cons	= RunnerGetServerNetCons(
							cltData->LocalServerType,
							cltData->LocalServerMachineIndex );
	if( !cons )
	{
		TraceError(
			"Unable to find NET_CONS for server %!ServerType! at machine index %hu, freeing client connection context. Ptr: %#018I64x",
			cltData->LocalServerType, cltData->LocalServerMachineIndex, (UINT64)cltData );
		FREE_DELETE(g_runnerNet->m_pool,cltData,CLIENT_DATA);
		return;
	}

	//	update the client status
	switch( cltStatus )
	{
		case CONNECT_STATUS_DISCONNECTED:
		case CONNECT_STATUS_CONNECTING:
		case CONNECT_STATUS_CONNECTFAILED:
		case CONNECT_STATUS_DISCONNECTING:
		default:
			//	connect failed
			cltData->MailboxPtr = NULL;
			NetConsSetProviderStatusAndNetClient(
				cons,
				cltData->RemoteServerType,
				cltData->RemoteServerInstance,
				CONNECT_STATUS_DISCONNECTED,
				NULL );
            if(cltData->pMuxClient)
            {
                cltData->pMuxClient->Release();  //once for connect callback
                if(cltData->bReleaseRefOnDisconnect)
                {
                    TraceVerbose(
                            TRACE_FLAG_SRUNNER,
                            "Release ref on disconnect specified. Freeing Ptr: %#018I64x",(UINT64)cltData);
                    cltData->pMuxClient->Release();  //once to release the object itself.
                    cltData->bReleaseRefOnDisconnect = false;
                    FREE_DELETE(g_runnerNet->m_pool,cltData,CLIENT_DATA);
                }
            }
            else 
            {
                ASSERT(cltData->pMuxClient); //need to figure out why this happens
            }

			break;
		case CONNECT_STATUS_CONNECTED:
			//	connect success
			NetConsSetProviderStatusAndNetClient(
				cons,
				cltData->RemoteServerType,
				cltData->RemoteServerInstance,
				CONNECT_STATUS_CONNECTED,
				cltData->pMuxClient );
            cltData->bReleaseRefOnDisconnect = false;
			cltData->pMuxClient->Release(); // one ref added when we initiated connect
			break;
	};

	//	release data
	RunnerReleaseServerNetCons(
		NetConsGetSvrType( cons ),
		NetConsGetSvrMachineIndex( cons ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sNetAccept(
	DWORD				dwError,
	MUXCLIENTID			netId,
	SOCKADDR_STORAGE &	localAddr,
	SOCKADDR_STORAGE &	remoteAddr,
	void *				pServerContext,
	void **				ppNewContext,
	BYTE *				pAcceptData,
	DWORD				dwAcceptDataLen)
{
	UNREFERENCED_PARAMETER(dwError); //TODO handle error
	ASSERT_RETFALSE( g_runnerNet );
	ASSERT_RETFALSE( g_runnerNet->m_netMap );
	ASSERT_RETFALSE( pServerContext );
	ASSERT_RETFALSE( ppNewContext );

	ppNewContext[0] = NULL;
	NET_CONS * mycons = (NET_CONS*)pServerContext;
	CLIENT_DATA * cltData = MALLOC_NEW(g_runnerNet->m_pool,CLIENT_DATA);
	if(!cltData)
	{
		TraceError("Could not allocate CLIENT_DATA for client %d",netId);
		return FALSE;
	}
	sNetInitUserData(cltData);

	//	get the service id for this connection
	SERVICEID serviceId = INVALID_SERVICEID;
	{
		NetConsAutolock nlock(mycons);
		//	get the address of this user
		serviceId = NetMapGetServiceIdFromUserAddress(
						g_runnerNet->m_netMap,
						remoteAddr ,
						SS_PORT(&localAddr));
		ASSERT_GOTO( serviceId != INVALID_SERVICEID,Error )
	}

	//	set the client data for this connection
	//	by definition, we can only accept connections from service users.
	cltData->RemoteMachineIsServiceProvider = FALSE;
	cltData->RemoteServerType				= ServiceIdGetUserType( serviceId );
	cltData->RemoteServerInstance			= ServiceIdGetUserInstance( serviceId );
	cltData->LocalServerType				= ServiceIdGetProviderType( serviceId );
	cltData->LocalServerMachineIndex		= ServiceIdGetProviderInstance( serviceId );//	HACK: machine index is set in place of providing svr instance...
	cltData->MailboxPtr						= NULL;

	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Created server connection context for client connection to %!ServerType! from remote server %!ServerType! inst. %hu, Ptr: %#018I64x",
		cltData->LocalServerType, cltData->RemoteServerType, cltData->RemoteServerInstance, (UINT64)cltData );

	ppNewContext[0] = cltData;	// accept client connection tries to get a pointer to this.

    //	notify the server and set the data
	BOOL acceptSuccess = FALSE;
    cltData->ServerData = RunnerAcceptClientConnection(
                                cltData->LocalServerType,
                                cltData->LocalServerMachineIndex,
                                ServiceUserId(
									cltData->RemoteServerType,
									cltData->RemoteServerInstance,
									netId ),
                                pAcceptData,
                                dwAcceptDataLen,
								acceptSuccess);
	if (!acceptSuccess)
		goto Error;

	TraceVerbose(
		TRACE_FLAG_SRUNNER_NET,
		"Accepted client connection for server %!ServerType! from server %!ServerType! instance %hu",
		cltData->LocalServerType,
		cltData->RemoteServerType,
		cltData->RemoteServerInstance );

    return TRUE;
Error:
    FREE_DELETE(g_runnerNet->m_pool,cltData,CLIENT_DATA);
    return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sNetRemove(
	NETCLIENTID	netId,
	void *		data )
{
	ASSERT_RETTRUE( g_runnerNet );
	ASSERT_RETTRUE( data );

	CLIENT_DATA * cltData = (CLIENT_DATA*)data;
	SVRTYPE localSvrType = cltData->LocalServerType;
	SVRMACHINEINDEX localSvrMIndex = cltData->LocalServerMachineIndex;

	TraceVerbose(
		TRACE_FLAG_SRUNNER_NET,
		"Removing connection from a %s of local server %!ServerType! from server %!ServerType! instance %hu.",
		cltData->RemoteMachineIsServiceProvider ? "SERVER" : "CLIENT",
		cltData->LocalServerType,
		cltData->RemoteServerType,
		cltData->RemoteServerInstance );

	//	clear out any mailbox
	cltData->MailboxPtr = NULL;

	NET_CONS * cons = RunnerGetServerNetCons(localSvrType,localSvrMIndex);
	if( !cons )
	{
		TraceWarn(
			TRACE_FLAG_SRUNNER,
			"Unable to retrieve a NET_CONS for a connection disconnect callback, NOT freeing context data. Ptr: %#018I64x, NETCLIENTID: %#018I64x",
			(UINT64)cltData, (UINT64)netId );
		//Freeing it here crashes anyone requesting the net client context.
		//FREE(g_runnerNet->m_pool,cltData);	//	special mux connection...
		return TRUE;
	}

	if( cltData->RemoteMachineIsServiceProvider )
	{
		//	we are loosing a required service connection
		NetConsSetProviderStatusAndNetClient(
			cons,
			cltData->RemoteServerType,
			cltData->RemoteServerInstance,
			CONNECT_STATUS_DISCONNECTED,
			NULL );

		TraceVerbose(
			TRACE_FLAG_SRUNNER,
			"Freeing client connection context. Ptr: %#018I64x",
			(UINT64)cltData );
		FREE_DELETE(g_runnerNet->m_pool,cltData,CLIENT_DATA);
	}
	else
	{
        // if remote = user server && netid = special, don't process
        if ( (IsSpecialMuxClientId(netId) != TRUE) || (cltData->RemoteServerType != USER_SERVER) ) 
        {
            //	a server is losing a client
            RunnerProcessClientDisconnect(
                cltData->LocalServerType,
                cltData->LocalServerMachineIndex,
                ServiceUserId(
                cltData->RemoteServerType,
                cltData->RemoteServerInstance,
                netId ),
                cltData->ServerData );
        }
	}

	//	release the associated net cons and return
	RunnerReleaseServerNetCons(localSvrType,localSvrMIndex);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sNetProcessImmediateCallback(
	DWORD       dwError,
	void *	    context,
	MUXCLIENTID	netId,
	BYTE *		msg,
	DWORD       msgSize )
{
	ASSERT_RETURN( g_runnerNet );
	ASSERT_RETURN( context );

	CLIENT_DATA * cltData = (CLIENT_DATA*)context;
    if(dwError)
    {
        TraceError("Error on runner network process immediate, %!WINERROR!",dwError);
        ASSERT(msgSize == 0);
    }
    if(msgSize == 0)
    {
        TraceVerbose(
			TRACE_FLAG_SRUNNER_NET,
			"Client type %!ServerType!, instance %hu, client id %d, removed.",
			cltData->RemoteServerType,
			cltData->RemoteServerInstance,
			netId );
        sNetRemove(netId,cltData);
        return;
    }

	//	build the message struct
	ASSERT_RETURN(msg);
	NETMSG msgData;
	msgData.Data		= msg;
	msgData.DataLength	= msgSize;
	msgData.Mailbox		= cltData->MailboxPtr.Ptr();

	if( cltData->RemoteMachineIsServiceProvider )
	{
		//	it's a response, we are a service user
		msgData.CmdTbl = sRunnerNetGetCommandTable(
							cltData->RemoteServerType,
							cltData->LocalServerType,
							FALSE );

		//	notify the controller of the response
		RunnerProcessResponse(
			cltData->LocalServerType,
			cltData->LocalServerMachineIndex,
			ServerId(
				cltData->RemoteServerType,
				cltData->RemoteServerInstance ),
			&msgData );
	}
	else
	{
		//	it's a request, we are a service provider
		msgData.CmdTbl = sRunnerNetGetCommandTable(
							cltData->LocalServerType,
							cltData->RemoteServerType,
							TRUE );

		//	notify the controller of the request
		if(!RunnerProcessRequest(
				cltData->LocalServerType,
				cltData->LocalServerMachineIndex,
				ServiceUserId(
					cltData->RemoteServerType,
					cltData->RemoteServerInstance,
					netId ),
				cltData->ServerData,
				&msgData ))
        {
            TraceError(
				"Runner Process Request failed. Disconnecting server %!ServerType!, instance %hu, client id %u, from local %!ServerType! server.",
				cltData->RemoteServerType,
				cltData->RemoteServerInstance,
				netId,
				cltData->LocalServerType );
            SvrNetDisconnectClient(
				ServiceUserId(
					cltData->RemoteServerType,
                    cltData->RemoteServerInstance,
                    netId ) );
        }
	}
	return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sNetInitUserData(
		void * data )
{
	ASSERT_RETURN( data );
	CLIENT_DATA * toSet = (CLIENT_DATA*)data;

	toSet->ServerData				= NULL;
	toSet->RemoteServerType			= INVALID_SVRTYPE;
	toSet->RemoteServerInstance		= INVALID_SVRINSTANCE;
	toSet->LocalServerType			= INVALID_SVRTYPE;
	toSet->LocalServerMachineIndex	= INVALID_SVRMACHINEINDEX;
	toSet->MailboxPtr.Init();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRunnerNetCheckPortMappings(
	void )
{
	TraceVerbose(TRACE_FLAG_SRUNNER_NET,"==== Server Runner Port Map ====");
	TraceVerbose(TRACE_FLAG_SRUNNER_NET,"Starting Port: %hu",INTERNAL_SERVERS_START_PORT);
	TraceVerbose(TRACE_FLAG_SRUNNER_NET,"Port: <port #>, Server: <offering type> - <offering machine index>, Client: <using type> - <using machine index>");

	UINT maxPort = INTERNAL_SERVERS_START_PORT+TOTAL_SERVER_COUNT*TOTAL_SERVER_COUNT*MAX_SVR_INSTANCES_PER_MACHINE*MAX_SVR_INSTANCES_PER_MACHINE;
	ASSERTX_RETFALSE( maxPort <= USHORT(-1),"Out of port addressing space!!");

	for(USHORT port = INTERNAL_SERVERS_START_PORT; port < maxPort; ++port)
	{
		CHANNEL_SERVERS info;
		info = NetMapGetPortUsers(htons(port));
		TraceVerbose(
			TRACE_FLAG_SRUNNER_NET,
			"Port: %hu, Server: %!ServerType! - %hu, Client: %!ServerType! - %hu",
			port,
			info.OfferingType,
			info.OfferingMachineIndex,
			info.UserType,
			info.UserMachineIndex );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// PUBLIC NETWORK METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetInit(
	struct MEMORYPOOL * pool,
	NET_MAP *           netMap,
	const char *		baseName )
{
	ASSERT_RETTRUE( !g_runnerNet );
	ASSERT_GOTO( netMap, _initError );

	ASSERT_GOTO( sRunnerNetCheckPortMappings(), _initError );

	//	create the net singleton
	g_runnerNet = ( RUNNER_NET * )MALLOC( pool, sizeof( RUNNER_NET ) );
	ASSERT_GOTO( g_runnerNet, _initError );

	//	initialize members
	g_runnerNet->m_pool		= pool;
	g_runnerNet->m_netMap	= netMap;
	g_runnerNet->m_szLocalIP[0] = 0;
	g_runnerNet->m_netCons.Init();
	g_runnerNet->m_netConsLock.Init();
	memclear(
		g_runnerNet->m_cmdTables,
		sizeof(NET_COMMAND_TABLE*) * TOTAL_SERVER_COUNT * TOTAL_SERVER_COUNT * 2 );

	//	init the net framework
	g_runnerNet->m_Net = sRunnerNetInitNetFramework( pool, baseName );	
	ASSERT_GOTO( g_runnerNet->m_Net, _initError );

	TraceInfo(TRACE_FLAG_SRUNNER_NET,"Server Runner Network initialized successfully.");

	return TRUE;

_initError:
	TraceError("Error initializing Server Runner Network.");
	RunnerNetClose();
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetSetLocalIp(
	const char *		localIp )
{
	ASSERT_RETFALSE(localIp && localIp[0]);

	if(g_runnerNet->m_szLocalIP[0])
		return FALSE;

	TraceWarn(
		TRACE_FLAG_SRUNNER,
		"Setting local server runner IP address: %s",
		localIp);

	PStrCopy( g_runnerNet->m_szLocalIP, localIp, MAX_NET_ADDRESS_LEN );
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRunnerNetItrDeadCons(
	struct MEMORYPOOL * pool,
	NET_CON_CONTAINER * toDestroy )
{
	UNREFERENCED_PARAMETER(pool);
	if( toDestroy->NetCons )
	{
		NetConsFree( toDestroy->NetCons );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerNetClose(
	void )
{
	if( !g_runnerNet )
	{
		return;
	}

	TraceInfo(TRACE_FLAG_SRUNNER_NET,"Closing Server Runner Network.");
	
	//	clear the command tables
	for( int ii = 0; ii < TOTAL_SERVER_COUNT; ++ii )
	{
		for( int jj = 0; jj < TOTAL_SERVER_COUNT; ++jj )
		{
			for( int kk = 0; kk < 2; ++kk )
			{
				if( g_runnerNet->m_cmdTables[ii][jj][kk] )
				{
					NetCommandTableFree( g_runnerNet->m_cmdTables[ii][jj][kk] );
					g_runnerNet->m_cmdTables[ii][jj][kk] = NULL;
				}
			}
		}
	}

	//	free our list of net cons
	g_runnerNet->m_netCons.Destroy(g_runnerNet->m_pool, sRunnerNetItrDeadCons );

	//	free the net framework
	if( g_runnerNet->m_Net )
	{
		NetClose( g_runnerNet->m_Net );
		g_runnerNet->m_Net = NULL;
	}

    g_runnerNet->m_netConsLock.Free();

	//	free the singleton
	FREE( g_runnerNet->m_pool, g_runnerNet );
	g_runnerNet = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * RunnerNetGetLocalIp(
	void )
{
	ASSERT_RETNULL(g_runnerNet);
	return g_runnerNet->m_szLocalIP;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetSockAddrToString(
	char * strDest,
	DWORD strLen,
	SOCKADDR_STORAGE * addr )
{
	ASSERT_RETFALSE(strDest);
	ASSERT_RETFALSE(addr);
	DWORD len = 0;

	if(addr->ss_family == AF_INET) 
	{
		len = sizeof(struct sockaddr_in);
	}
	else if(addr->ss_family == AF_INET6) 
	{
		len = sizeof(struct sockaddr_in6);
	}
	if(WSAAddressToString(
		(struct sockaddr*)addr, 
		len,
		NULL,
		strDest,
		&strLen) != 0)
	{
		TraceError("WSAAddressToString failed %!WINERROR!",WSAGetLastError());
		return FALSE;
	}
	else
	{
		char *ptr = strchr(strDest,':');
		if(ptr)
		{
			*ptr = 0;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetStringToSockAddr(
	char * strSource,
	SOCKADDR_STORAGE * addrDest )
{
	ASSERT_RETFALSE(strSource && strSource[0]);
	ASSERT_RETFALSE(addrDest);

	struct addrinfo *aiList = NULL;

	if(getaddrinfo(strSource,"0",NULL,&aiList) == 0)
	{
		struct addrinfo *curr = aiList;
		while(curr)
		{
			if(aiList->ai_family == AF_INET || aiList->ai_family == AF_INET6)
			{
				memcpy(addrDest,curr->ai_addr,curr->ai_addrlen);
				break;
			}
			curr = curr->ai_next;
		}
		freeaddrinfo(aiList);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static IMuxClient * sRunnerNetDisconnectedConnectionsItr(
	NET_CONS *		 cons,
	SVRTYPE			 providerType,
	SVRINSTANCE		 providerInstance,
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETFALSE( g_runnerNet );
	ASSERT_RETFALSE( cons );

	//	try to get this connection
	return sRunnerNetTryClientConnect(
				NetConsGetSvrType( cons ),
				NetConsGetSvrMachineIndex( cons ),
				providerType,
				providerInstance,
				mailbox );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerNetAttemptOutstandingConnections(
	void )
{
	ASSERT_RETURN( g_runnerNet );

	HLCSAutoLock lock(&g_runnerNet->m_netConsLock);

	//	iterate all open net cons
	NET_CON_CONTAINER * itr = g_runnerNet->m_netCons.GetFirst();
	while( itr )
	{
		//	if they have outstanding net connections, try them by iterating over all outstanding connections
		if( NetConsGetDisconnectedProviderCount( itr->NetCons ) > 0 )
		{
			TraceExtraVerbose(
				TRACE_FLAG_SRUNNER_NET,
				"Server Runner Network attempting outstanding client connections for server %!ServerType! instance %hu.",
				NetConsGetSvrType(itr->NetCons),
				NetConsGetSvrInstance(itr->NetCons) );
			NetConsIterateDisconnectedProviders( itr->NetCons, sRunnerNetDisconnectedConnectionsItr );
		}
		itr = g_runnerNet->m_netCons.GetNext( itr );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NET_CONS * RunnerNetEstablishConnections(
	struct MEMORYPOOL * pool,
	CONNECTION_TABLE *  tbl,
	SVRINSTANCE			serverInstance,
	SVRMACHINEINDEX		serverMachineIndex )
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERT_RETNULL( tbl );
	ASSERT_RETNULL( serverMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );

	SVRTYPE serverType = tbl->TableServerType;

	//	get a fresh net cons
	NET_CONS * toRet = NetConsInit(
						pool,
						tbl,
						serverType,
						serverInstance,
						serverMachineIndex );
	ASSERT_RETNULL( toRet );

	//	create offered internal services
	for( SVRTYPE userType = 0; userType < TOTAL_SERVER_COUNT; ++userType )
	{
		if( tbl->OfferedServices[userType].ConnectionSet )
		{
			for( SVRMACHINEINDEX userMIndex = 0;
                 userMIndex < MAX_SVR_INSTANCES_PER_MACHINE; 
                 ++userMIndex )
			{
                NetConsAutolock nlock(toRet);
				SOCKADDR_STORAGE addr = {0};
                WCHAR instanceName[MAX_PERF_INSTANCE_NAME];
				SS_PORT(&addr) = NetMapGetCommunicationPort(
										serverType,
										serverMachineIndex,
										userType,
										userMIndex );
				ASSERT_CONTINUE(SS_PORT(&addr) != INVALID_SVRPORT);

                PStrPrintf(instanceName,ARRAYSIZE(instanceName),L"MuxServer:%02d-%02d",serverType,userType);
				MuxServer * svr = sRunnerNetCreateNetServer( addr, toRet,instanceName);
				ASSERT_CONTINUE(svr);
				NetConsSetProvidedServicePtr( toRet, userType, userMIndex, svr );
			}
		}
	}

	//	loop over all possible providing types
	for( SVRTYPE providerType = 0; providerType < TOTAL_SERVER_COUNT; ++providerType )
	{
		if( tbl->RequiredServices[providerType].ConnectionSet )
		{
			//	they require this type, register it
			DWORD instanceCount = NetMapGetServiceInstanceCount( g_runnerNet->m_netMap, providerType );
			NetConsRegisterProviderType( toRet, providerType, instanceCount );
		}
	}

	//	add this net cons to our list
	HLCSAutoLock lock = &g_runnerNet->m_netConsLock;
	NET_CON_CONTAINER * added = g_runnerNet->m_netCons.Add(
												g_runnerNet->m_pool,
												ServerId(
													serverType,
													serverInstance ) );
	if( !added )
	{
		NetConsFree( toRet );
		ASSERT_RETNULL(added);
	}
	added->NetCons = toRet;
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerNetFreeConnections(
	NET_CONS * cons )
{
	ASSERT_RETURN( g_runnerNet );
	if( !cons )
		return;

	//	free all connections
	NetConsCloseActiveConnections(
		cons,
		g_runnerNet->m_Net );

	//	remove the cons from our table
	HLCSAutoLock lock = &g_runnerNet->m_netConsLock;
	g_runnerNet->m_netCons.Remove(
							ServerId(
								NetConsGetSvrType( cons ),
								NetConsGetSvrInstance( cons ) ) );

	//	free the cons object
	NetConsFree( cons );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE RunnerNetSendRequestReal(
	NET_CONS *		cons,
	SERVICEID		serviceId,
	IMuxClient *    pClient,
	MSG_STRUCT *	msg,
	NET_CMD			command)
{
    BYTE			buffer[ MAX_LARGE_MSG_SIZE ];
    SRNET_RETURN_CODE            toRet = SRNET_FAILED;

	ASSERT_RETVAL( g_runnerNet ,SRNET_FAILED);
	ASSERT_RETVAL( cons ,SRNET_FAILED);
	ASSERT_RETVAL( serviceId != INVALID_SERVICEID ,SRNET_FAILED);
	ASSERT_RETVAL( msg ,SRNET_FAILED);

	TraceExtraVerbose(TRACE_FLAG_SRUNNER_NET, 
		"Sending message %d to service id %#018I64x", int(command), serviceId);

    if(!pClient)
    {
        //	get the net id from the net cons, increments the client ref count
        pClient = NetConsGetProviderNetClient(
						cons,
						ServiceIdGetProviderType( serviceId ),
						ServiceIdGetProviderInstance( serviceId ) );
    }
	ASSERT_RETVAL(pClient,SRNET_FAILED);

	//	get the command table for message translation
	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
									ServiceIdGetProviderType( serviceId ),
									ServiceIdGetUserType( serviceId ),
									TRUE );
    ASSERT_GOTO(tbl,_error);

	//	translate the message for sending

    {
        BYTE_BUF_NET	bbuf( buffer, sizeof(buffer) );
        unsigned int	data_size = 0;

        if( NetTranslateMessageForSend(
            tbl,
            command,
            msg,
            data_size,
            bbuf )) 
        {
            //	now send the message and dec the ref count
            int ir  = pClient->Send(buffer, data_size );
			switch(ir)
			{
				case 0:
					toRet = SRNET_TEMPORARY_FAILURE;
					break;
				case -1:
					toRet = SRNET_FAILED;
					break;
				default:
					toRet = SRNET_SUCCESS;
					break;
			}
        }
        else
        {
            ASSERT(toRet);
        }
    }

_error:
	pClient->Release();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE RunnerNetSendRequest(
	NET_CONS *		cons,
	SERVICEID		serviceId,
	MSG_STRUCT *	msg,
	NET_CMD			command)
{
    return RunnerNetSendRequestReal(cons,serviceId,NULL,msg,command);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerNetAttachResponseMailbox(
	NET_CONS *			cons,
	SERVICEID			serviceId,
	SERVER_MAILBOX *	mailbox )
{
	ASSERT_RETURN( cons );
	ASSERT_RETURN( serviceId != INVALID_SERVICEID );

	//	register it with the cons object
	NetConsSetProviderMailbox(
		cons,
		ServiceIdGetProviderType( serviceId ),
		ServiceIdGetProviderInstance( serviceId ),
		mailbox );

	//	now set the connection box if we are connected
	IMuxClient *pClient = NetConsGetProviderNetClient(
							cons,
							ServiceIdGetProviderType( serviceId ),
							ServiceIdGetProviderInstance( serviceId ) );
	if( pClient )
	{
		CLIENT_DATA * cltData = (CLIENT_DATA*)(pClient->GetContext());
		if( cltData ) {
			cltData->MailboxPtr = mailbox;
		}
        pClient->Release();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONNECT_STATUS	RunnerNetGetServiceStatus(
	NET_CONS *	cons,
	SERVICEID	serviceId )
{
    CONNECT_STATUS status;
	ASSERT_RETX( g_runnerNet,	CONNECT_STATUS_DISCONNECTED );
	ASSERT_RETX( cons,			CONNECT_STATUS_DISCONNECTED );
	ASSERT_RETX( serviceId != INVALID_SERVICEID, CONNECT_STATUS_DISCONNECTED );

	//	try to get the net id from the cons struct
	IMuxClient* pClient = NetConsGetProviderNetClient(
							cons,
							ServiceIdGetProviderType( serviceId ),
							ServiceIdGetProviderInstance( serviceId ) );

	//	see if we even have a connection
	if( pClient == NULL )
	{
		return CONNECT_STATUS_DISCONNECTED;
	}

	//	we have a connection, get its status
	status =  pClient->GetConnectionStatus();

    pClient->Release();

    return status;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE RunnerNetSendResponse(
	NET_CONS *		cons,
	SERVICEUSERID	userId,
	MSG_STRUCT *	msg,
	NET_CMD			command )
{
    SRNET_RETURN_CODE            toRet = SRNET_FAILED;
	BYTE			buf_small[ MAX_SMALL_MSG_SIZE ];
    BYTE           *buf_data = &buf_small[0];

	ASSERT_RETVAL( g_runnerNet ,SRNET_FAILED);
	ASSERT_RETVAL( cons ,SRNET_FAILED);
	ASSERT_RETVAL( userId != INVALID_SERVICEUSERID ,SRNET_FAILED);
	ASSERT_RETVAL( msg ,SRNET_FAILED);

	TraceExtraVerbose(TRACE_FLAG_SRUNNER_NET, 
		"Sending message %d to service user id %#018I64x", int(command), userId);

	//	get the command table for message translation
	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
									NetConsGetSvrType( cons ),
									ServiceUserIdGetUserType( userId ),
									FALSE );
	ASSERT_RETVAL( tbl,SRNET_FAILED );
	DWORD maxsize = NetCommandGetMaxSize(tbl, command);


	//	translate the message for sending
    if(maxsize > MAX_SMALL_MSG_SIZE)
    {
        buf_data = (BYTE*)MALLOCZ(g_runnerNet->m_pool,maxsize);
        ASSERT_RETVAL(buf_data,SRNET_FAILED);
    }
	BYTE_BUF_NET	bbuf( buf_data, max((MSG_SIZE)MAX_SMALL_MSG_SIZE,maxsize) );
	unsigned int	data_size = 0;
	ASSERT_GOTO( NetTranslateMessageForSend(
							tbl,
							command,
							msg,
							data_size,
							bbuf ),_done );

	//	now send the message
	int ir =  NetConsSendToClient(
			cons,
            NetMapGetServerMachineIndex(g_runnerNet->m_netMap,ServiceUserIdGetServer(userId)),
			userId ,
			buf_data,
			data_size );
	switch(ir)
	{
		case 0:
			toRet = SRNET_TEMPORARY_FAILURE;
			break;
		case -1:
			toRet = SRNET_FAILED;
			break;
		default:
			toRet = SRNET_SUCCESS;
			break;
	}

_done:
    if(buf_data && (buf_data != buf_small))
    {
        FREE(g_runnerNet->m_pool,buf_data);
    }
    return toRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE RunnerNetSendBuffer(
	NET_CONS *		cons,
	SERVICEUSERID	userId,
	BYTE *	        buffer,
	DWORD			bufferLen )
{
    SRNET_RETURN_CODE     toRet = SRNET_FAILED;

	ASSERT_RETVAL( g_runnerNet ,SRNET_FAILED);
	ASSERT_RETVAL( cons ,SRNET_FAILED);
	ASSERT_RETVAL( userId != INVALID_SERVICEUSERID ,SRNET_FAILED);
	ASSERT_RETVAL( buffer ,SRNET_FAILED);
	ASSERT_RETVAL( bufferLen ,SRNET_FAILED);

	TraceExtraVerbose(TRACE_FLAG_SRUNNER_NET, 
		"Sending buffer to service user id %#018I64x",  userId);


	//	now send the message
	int ir =  NetConsSendToClient(
                			cons,
                            NetMapGetServerMachineIndex(g_runnerNet->m_netMap,ServiceUserIdGetServer(userId)),
                			userId ,
                			buffer,
                			bufferLen );

	switch(ir)
	{
		case 0:
			toRet = SRNET_TEMPORARY_FAILURE;
			break;
		case -1:
			toRet = SRNET_FAILED;
			break;
		default:
			toRet = SRNET_SUCCESS;
			break;
	}
    return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE RunnerNetSendResponseToClients(
	NET_CONS *			cons,
	SVRID				server,
	SVRCONNECTIONID *	connectedClients,
	UINT				numClients,
	MSG_STRUCT *		message,
	NET_CMD				command)
{
    SRNET_RETURN_CODE            toRet = SRNET_FAILED;
	BYTE			buf_small[ MAX_SMALL_MSG_SIZE ];
	BYTE			*buf_data = buf_small;
	unsigned int	data_size = 0;

    if(message->hdr.size > MAX_SMALL_MSG_SIZE)
    {
        buf_data = (BYTE*)MALLOCZ(g_runnerNet->m_pool,message->hdr.size);
        ASSERT_RETVAL(buf_data,SRNET_FAILED);
    }
	BYTE_BUF_NET	bbuf( buf_data, MAX((MSG_SIZE)MAX_SMALL_MSG_SIZE,message->hdr.size) );

	//	get the command table for message translation
	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
									NetConsGetSvrType( cons ),
									ServerIdGetType( server ),
									FALSE );
	ASSERT_GOTO( tbl,_error );
	ASSERT_GOTO( NetTranslateMessageForSend(
							tbl,
							command,
							message,
							data_size,
							bbuf ),_error );

	int ir =  NetConsSendToMultipleClients(
			cons,
			ServerIdGetType( server ),
			NetMapGetServerMachineIndex(g_runnerNet->m_netMap,server),
			connectedClients ,
			numClients,
			buf_data,
			data_size );
	switch(ir)
	{
		case 0:
			toRet = SRNET_TEMPORARY_FAILURE;
			break;
		case -1:
			toRet = SRNET_FAILED;
			break;
		default:
			toRet = SRNET_SUCCESS;
			break;
	}
_error:
    if(buf_data && (buf_data != buf_small))
    {
        FREE(g_runnerNet->m_pool,buf_data);
    }
    return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerNetAttachRequestMailbox(
	NET_CONS *			cons,
	SERVICEUSERID		userId,
	SERVER_MAILBOX *	mailbox )
{
	ASSERT_RETURN( g_runnerNet );
	ASSERT_RETURN( cons );
	ASSERT_RETURN( userId != INVALID_SERVICEUSERID );

	//	just attach it to the raw client data, no way/need to track client state in framework
	CLIENT_DATA * cltData = (CLIENT_DATA*)NetConsGetClientContext(
											cons,
											NetMapGetServerMachineIndex(
												g_runnerNet->m_netMap,
												ServiceUserIdGetServer(userId)),
											userId);
	if( cltData ) {
		cltData->MailboxPtr = mailbox;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * RunnerNetGetClientContext(
	NET_CONS *		cons,
	SERVICEUSERID	userId )
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERT_RETNULL( cons );
	ASSERT_RETNULL( userId != INVALID_SERVICEUSERID );

	CLIENT_DATA *cltData = (CLIENT_DATA*)NetConsGetClientContext(
											cons,
											NetMapGetServerMachineIndex(
												g_runnerNet->m_netMap,
												ServiceUserIdGetServer(userId)),
											userId );
    if(cltData)
    {
        return cltData->ServerData;
    }
    else
    {
        return NULL;
    }
}
static CLIENT_DATA * sRunnerNetGetClientData(
	NET_CONS *		cons,
	SERVICEUSERID	userId )
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERT_RETNULL( cons );
	ASSERT_RETNULL( userId != INVALID_SERVICEUSERID );

	CLIENT_DATA *cltData = (CLIENT_DATA*)NetConsGetClientContext(
											cons,
											NetMapGetServerMachineIndex(
												g_runnerNet->m_netMap,
												ServiceUserIdGetServer(userId)),
											userId );
    return cltData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPVOID RunnerNetDisconnectClient(
	NET_CONS *		cons,
	SERVICEUSERID	userId )
{
	ASSERT_RETNULL( g_runnerNet );
	ASSERT_RETNULL( cons );
	ASSERT_RETNULL( userId != INVALID_SERVICEUSERID );

	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"RunnerNetDisconnectClient called by %!ServerType! inst. %hu for service user %#018I64x",
		NetConsGetSvrType(cons), NetConsGetSvrInstance(cons), userId );

	CLIENT_DATA * cltData = sRunnerNetGetClientData(cons,userId);
	ASSERT_RETNULL( cltData );
	LPVOID serverData = cltData->ServerData;

	//	disconnect the user
	NetConsDisconnectClient(
		cons,
		NetMapGetServerMachineIndex(g_runnerNet->m_netMap,
		ServiceUserIdGetServer(userId)),
		userId );

	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Freeing disconnected client context. Ptr: %#018I64x", (UINT64)cltData );

	FREE_DELETE(g_runnerNet->m_pool,cltData,CLIENT_DATA);
	return serverData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sTranslateMailboxMessage(
		RAW_MSG_ENTRY * rawMsg,
		MAILBOX_MSG   * msgDest )
{
	ASSERT_RETFALSE( rawMsg );
	ASSERT_RETFALSE( msgDest );

	//	set the type and info
	msgDest->IsFakeMessage = rawMsg->MsgMetadata.IsFake;
	if( rawMsg->MsgMetadata.IsRequest )
	{
		//	set as client request
		msgDest->MessageType		= MSG_TYPE_REQUEST;
		msgDest->RequestingClient	= rawMsg->MsgMetadata.ServiceUserId;
	}
	else
	{
		//	set as server response
		msgDest->MessageType		= MSG_TYPE_RESPONSE;
		msgDest->RespondingServer	= rawMsg->MsgMetadata.ServiceProviderId;
	}

	//	get the command table for message translation
	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
		rawMsg->MsgMetadata.ServiceProvider,
		rawMsg->MsgMetadata.ServiceUser,
		rawMsg->MsgMetadata.IsRequest );
	ASSERTX_RETFALSE( tbl, "Could not find command table for mailbox message." );

	//	translate the message for receive
	ASSERT_RETFALSE( NetTranslateMessageForRecv(
								tbl,
								rawMsg->MsgBuff,
								rawMsg->MsgSize,
								msgDest->GetMsg() ) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetPopMailboxMessage(
	SERVER_MAILBOX *	mailbox,
	MAILBOX_MSG *		msgDest )
{
	ASSERT_RETFALSE( g_runnerNet );
	ASSERT_RETFALSE( mailbox );
	ASSERT_RETFALSE( msgDest );
	RAW_MSG_ENTRY *  rawMsg;

	//	pop a message
	rawMsg = SvrMailboxPopRawMessage( mailbox );
	if( !rawMsg )
	{
		return FALSE;
	}

	BOOL toRet = sTranslateMailboxMessage( rawMsg, msgDest );

	//	release the raw mailbox message
	SvrMailboxRecycleRawMessage( mailbox, rawMsg );

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetPopMailboxMessage(
	MAILBOX_MSG_LIST *	msgList,
	MAILBOX_MSG *		msgDest )
{
	ASSERT_RETFALSE( msgList );
	ASSERT_RETFALSE( msgDest );

	RAW_MSG_ENTRY * rawMsg;

	//	get a message
	rawMsg = SvrMailboxPopRawListMessage( msgList );
	if( !rawMsg )
		return FALSE;

	BOOL toRet = sTranslateMailboxMessage( rawMsg, msgDest );

	//	release the message
	SvrMailboxRecycleRawListMessage( msgList, rawMsg );

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetPostMailboxMsg(
	SERVER_MAILBOX *	mailbox,
	RAW_MSG_METADATA *  metadata,
	MSG_STRUCT *		msg,
	NET_CMD				command )
{
	ASSERT_RETFALSE( g_runnerNet );
	ASSERT_RETFALSE( mailbox );
	ASSERT_RETFALSE( msg );
	ASSERT_RETFALSE( metadata );
	
	//	get the command table for message translation
	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
									metadata->ServiceProvider,
									metadata->ServiceUser,
									metadata->IsRequest );
	ASSERT_RETFALSE( tbl );
	
	//	translate the message for sending
	BYTE			buf_data[ SMALL_MSG_BUF_SIZE ];
	BYTE_BUF_NET	bbuf( buf_data, SMALL_MSG_BUF_SIZE );
	unsigned int	data_size = 0;
	ASSERT_RETFALSE(NetTranslateMessageForSend(
						tbl,
						command,
						msg,
						data_size,
						bbuf ) );

	//	place msg in mailbox
	ASSERT_RETFALSE(
		SvrMailboxAddRawMessage(
			mailbox,
			*metadata,
			buf_data,
			data_size ) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetAreAddressesEqual(
	SOCKADDR_STORAGE& lhs,
	SOCKADDR_STORAGE&rhs)
{
    if(lhs.ss_family != rhs.ss_family)
    {
        return FALSE;
    }
    if(lhs.ss_family == AF_INET)
    {
        struct sockaddr_in *lh4 = (struct sockaddr_in*)&lhs,
                           *rh4 = (struct sockaddr_in*)&rhs;

        return (lh4->sin_addr.s_addr == rh4->sin_addr.s_addr );
    }
    else if(lhs.ss_family == AF_INET6)
    {
        return IN6_ADDR_EQUAL((in6_addr*)&lhs,(in6_addr*)&rhs);
    }
    else
    {
        ASSERT(lhs.ss_family == AF_INET);
        return FALSE;
    }
}

//----------------------------------------------------------------------------
// Since we only have the message command tables at a runner level, giving
// a translation service in case someone needs them.
//----------------------------------------------------------------------------
BOOL RunnerNetTranslateMessageForSend(
	SVRTYPE provider,
	SVRTYPE user,
	BOOL bRequest,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	unsigned int & size,
	BYTE_BUF_NET & bbuf)
{
	ASSERT_RETFALSE( msg );

	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
		provider,
		user,
		bRequest );
	ASSERT_RETFALSE( tbl );

	//	translate the message for sending
	return NetTranslateMessageForSend(
					tbl,
					cmd,
					msg,
					size,
					bbuf );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerNetTranslateMessageForRecv(
	SVRTYPE provider,
	SVRTYPE user,
	BOOL bRequest,
	BYTE * data,
	unsigned int size,
	MSG_STRUCT * msg)
{
	ASSERT_RETFALSE( msg );

	NET_COMMAND_TABLE * tbl = sRunnerNetGetCommandTable(
		provider,
		user,
		bRequest );
	ASSERT_RETFALSE( tbl );

	return NetTranslateMessageForRecv(
		tbl,
		data,
		size,
		msg);
}


//----------------------------------------------------------------------------
// PRIVATE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NET_COMMAND_TABLE * SvrGetRequestCommandTableForUserServer(
	SVRTYPE provider)
{
	return sRunnerNetGetCommandTable(provider,USER_SERVER,TRUE);
}
IMuxClient *RunnerNetGetMuxClient(NET_CONS *cons,SERVICEID serviceId)
{
    return NetConsGetProviderNetClient(cons,
                                       ServiceIdGetProviderType(serviceId),
                                       ServiceIdGetProviderInstance(serviceId));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NETCOM * RunnerNetHijackNetcom(
	void )
{
	ASSERT_RETNULL(g_runnerNet);
	return g_runnerNet->m_Net;
}

