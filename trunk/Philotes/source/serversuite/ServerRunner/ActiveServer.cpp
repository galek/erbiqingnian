//----------------------------------------------------------------------------
// ActiveServer.cpp
// (C)Copyright 2007, Ping0 Interactive Limited. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "ActiveServer.h"
#include "servercontext.h"
#include "definition_priv.h"
#include <Ping0ServerMessages.h>

#include "ActiveServer.cpp.tmh"

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
using namespace FSCommon;
#endif

//----------------------------------------------------------------------------
// ACTIVE SERVER INIT AND SHUTDOWN
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActiveServerCallPreNetInit(
	ACTIVE_SERVER * svr )
{
	ASSERT_RETFALSE( svr );
	ASSERT_RETFALSE( svr->m_specs );
	ASSERT_RETFALSE( svr->m_specs->svrPreNetSetupInit );
	BOOL toRet = FALSE;

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Calling Pre-Network initialization function for server %!ServerType! instance %hu.",
		svr->m_specs->svrType,
		svr->m_svrInstance );

	ASSERT_GOTO(
		ServerContextSet(
			&g_runner->m_servers
				[svr->m_specs->svrType]
				[svr->m_svrMachineIndex]),
		_PRE_NET_INIT_ERROR );
	__try
	{
		//	do pre-net svr init
		svr->m_svrData = svr->m_specs->svrPreNetSetupInit(
										svr->m_svrPool,
										ServerId(
											svr->m_specs->svrType,
											svr->m_svrInstance ),
										svr->m_startupConfig );
		ASSERT_GOTO( svr->m_svrData, _PRE_NET_INIT_ERROR );
		toRet = TRUE;
	}
	__except( RunnerExceptFilter(
				GetExceptionInformation(),
				svr->m_specs->svrType,
				svr->m_svrInstance,
				"Processing server Pre-Net Init handler.") )
	{
        TraceError("We should never get here but code-analysis complains if this is empty");
	}

_PRE_NET_INIT_ERROR:
	ServerContextSet( (ACTIVE_SERVER_ENTRY*)NULL );

	//	free the config struct
	FREE( svr->m_svrPool, svr->m_startupConfig );
	svr->m_startupConfig = NULL;

	if(toRet)
	{
		TraceInfo(
			TRACE_FLAG_SRUNNER,
			"Successfully finished calling pre-network initialization function for server %!ServerType! instance %hu.",
			svr->m_specs->svrType,
			svr->m_svrInstance );
	}
	else
	{
		TraceError(
			"Error while calling pre-network initialization function for server %!ServerType! instance %hu.",
			svr->m_specs->svrType,
			svr->m_svrInstance );
	}

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActiveServerInitNetConnections(
	ACTIVE_SERVER * svr )
{
	ASSERT_RETFALSE(svr);

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Initializing network connections for server %!ServerType! instance %hu.",
		svr->m_specs->svrType,
		svr->m_svrInstance );

	//	init the network connections
	svr->m_netCons = RunnerNetEstablishConnections(
						svr->m_svrPool,
						&G_SERVER_CONNECTIONS[ svr->m_specs->svrType ],
						svr->m_svrInstance,
						svr->m_svrMachineIndex );

	if(svr->m_netCons)
	{
		TraceInfo(
			TRACE_FLAG_SRUNNER,
			"Successfully finished initializing network connections for server %!ServerType! instance %hu.",
			svr->m_specs->svrType,
			svr->m_svrInstance );
	}
	else
	{
		TraceError(
			"Error initializing network connections for server %!ServerType! instance %hu.",
			svr->m_specs->svrType,
			svr->m_svrInstance );
	}

	ASSERT_RETFALSE( svr->m_netCons );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD WINAPI sActiveServerMainRunner(
	LPVOID pserver )
{
	ACTIVE_SERVER_ENTRY * svrEntry = (ACTIVE_SERVER_ENTRY*)pserver;

	ASSERT_RETNULL( svrEntry );
	ASSERT_RETNULL( svrEntry->Server );
	ASSERT_RETNULL( svrEntry->Server->m_specs );
	ASSERT_RETNULL( svrEntry->Server->m_specs->svrEntryPoint );

	//	do init on this thread
	ASSERT_GOTO( RunnerAddPrivilagedThreadId(
					svrEntry->Server->m_specs->svrType,
					svrEntry->Server->m_svrMachineIndex,
					GetCurrentThreadId() ), _error );
	ASSERT_GOTO( sActiveServerCallPreNetInit( svrEntry->Server ), _error );
	ASSERT_GOTO( sActiveServerInitNetConnections( svrEntry->Server ), _error );

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Successfully started server %!ServerType! instance %hu.",
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance );

	DesktopServerTrace(
		"Successfully started server %s",
		ServerGetName(svrEntry->Server->m_specs->svrType));

	SvrManagerSendServerStatusUpdate(
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance,
		L"running");

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Main thread id %u starting for server %!ServerType! instance %hu.",
		GetCurrentThreadId(),
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance );
	
	//	stall on thread create lock until ActiveServerStartup is done with our servers creation
	svrEntry->Server->m_threadCreateCS.Enter();
	svrEntry->Server->m_threadCreateCS.Leave();

	//	set our context
	ASSERT_GOTO( ServerContextSet( svrEntry ), _error );
	__try
	{
		//	run the server
		svrEntry->Server->m_specs->svrEntryPoint( svrEntry->Server->m_svrData );
	}
	__except( RunnerExceptFilter(
				GetExceptionInformation(),
				svrEntry->Server->m_specs->svrType,
				svrEntry->Server->m_svrInstance,
				"Processing server entry point method.") )
	{
        TraceError("We should never get here but code-analysis complains if this is empty");
	}
	goto _done;

_error:
	SvrManagerSendServerOpResult(
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance,
		L"startserver",
		L"ERROR",
		L"Internal error.");
		
	TraceError(
		"Error starting server type %!ServerType! instance %hu.",
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance );

	DesktopServerTrace(
		"ERROR starting server %s",
		ServerGetName(svrEntry->Server->m_specs->svrType));

	WCHAR svrName[256];
	PStrCvt(svrName, ServerGetName(svrEntry->Server->m_specs->svrType), 256);
	LogErrorInEventLog(EventCategoryServerRunner,MSG_ID_SERVER_FAILED_TO_START,svrName,NULL,0);

	RunnerStopServer(
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance,
		svrEntry->Server->m_svrMachineIndex );

_done:
	SvrRunnerKillServer();

	RunnerRemovePrivilagedThreadId(
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrMachineIndex,
		GetCurrentThreadId() );

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Main thread id %u exiting for server %!ServerType! instance %hu.",
		GetCurrentThreadId(),
		svrEntry->Server->m_specs->svrType,
		svrEntry->Server->m_svrInstance );

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL	ActiveServerStartup(
	SERVER_SPECIFICATION *	spec,
#if USE_MEMORY_MANAGER
	CMemoryAllocatorCRT<true>* allocatorCRT,
	CMemoryAllocatorPOOL<true, true, 1024>* pool,
#else		
	struct MEMORYPOOL *		pool,				// the memory pool for this server to use
#endif				
	SVRINSTANCE				instance,
	SVRMACHINEINDEX			machineIndex,
	const SVRCONFIG *		config )
{
	ASSERT_RETFALSE( spec );
	ASSERT_RETFALSE( spec->Validate() );
	ASSERT_RETFALSE( instance != INVALID_SVRINSTANCE );
	ASSERTV_RETFALSE( instance < SvrClusterGetServerCount( spec->svrType ),
		"Server runner asked to start an invalid server index. Type %h, Instance %h", spec->svrType, instance );
	ASSERT_RETFALSE( machineIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	ACTIVE_SERVER *	 svr = NULL;
	WCHAR * errorString = L"";

	//	lock the entry and make sure they aren't already running
	errorString = L"Server already running.";
	ASSERT_GOTO( !g_runner->m_servers[spec->svrType][machineIndex].ServerLock.Lock(), _error );
	if (g_runner->m_servers[spec->svrType][machineIndex].Server)
	{
		ASSERT_GOTO(g_runner->m_servers[spec->svrType][machineIndex].Server->m_svrInstance == instance, _error);
		g_runner->m_servers[spec->svrType][machineIndex].ServerLock.Unlock();
		TraceWarn(
			TRACE_FLAG_SRUNNER,
			"Recieved start server command for a server that was already running. Type: %!ServerType!, Instance: %hu.",
			spec->svrType, instance);
		SvrManagerSendServerOpResult(
			spec->svrType,
			instance,
			L"startserver",
			L"ERROR",
			errorString);
		return TRUE;
	}

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Beginning server startup for server %!ServerType! instance %hu.",
		spec->svrType,
		instance );

	DesktopServerTrace(
		"Starting server %s",
		ServerGetName(spec->svrType));

	//	create the struct
	svr = (ACTIVE_SERVER*)MALLOCZ( pool, sizeof( ACTIVE_SERVER ) );
	errorString = L"Out of memory.";
	ASSERT_GOTO( svr, _error );

	//	init the members
	svr->m_specs					= spec;
	svr->m_svrInstance				= instance;
	svr->m_svrMachineIndex			= machineIndex;
#if USE_MEMORY_MANAGER
	svr->m_svrAllocatorCRT			= allocatorCRT;
#endif	
	svr->m_svrPool					= pool;
	svr->m_svrData					= NULL;
	svr->m_threadCreateCS.Init();
	svr->m_threads.Init();
	svr->m_threadsLock.Init();
	svr->m_startupConfig			= config;
	svr->m_wasShutdownRequested		= FALSE;
	svr->m_bIsShuttingDown			= FALSE;

	//	set the entry
	g_runner->m_servers[spec->svrType][machineIndex].Server = svr;
	RunnerClearPrivilagedThreads(
		spec->svrType,
		svr->m_svrMachineIndex);

	//	call pre net init
	errorString = L"Internal error.";
	HANDLE svrThread = NULL;
	if( svr->m_specs->svrEntryPoint )
	{
		svr->m_threadCreateCS.Enter();
		svr->m_threadsLock.Enter();

		svrThread = CreateThread(
						NULL,
						0,
						sActiveServerMainRunner,
						&g_runner->m_servers
							[svr->m_specs->svrType]
							[svr->m_svrMachineIndex],
						0,
						NULL );
		ASSERT_DO( svrThread != NULL )
		{
			svr->m_threadsLock.Leave();
			svr->m_threadCreateCS.Leave();
			goto _error;
		}

		ASSERT( svr->m_threads.Insert( svr->m_svrPool, svrThread ) );
		g_runner->m_servers[spec->svrType][machineIndex].ServerLock.Unlock();

		svr->m_threadsLock.Leave();
		svr->m_threadCreateCS.Leave();
	}
	else
	{
		ASSERT_GOTO( RunnerAddPrivilagedThreadId(
			spec->svrType,
			svr->m_svrMachineIndex,
			GetCurrentThreadId() ), _error );

		ASSERT_GOTO( sActiveServerCallPreNetInit( svr ), _error );
		ASSERT_GOTO( sActiveServerInitNetConnections( svr ), _error );
		g_runner->m_servers[spec->svrType][machineIndex].ServerLock.Unlock();

		TraceInfo(
			TRACE_FLAG_SRUNNER,
			"Successfully started server %!ServerType! instance %hu.",
			spec->svrType,
			instance );

		DesktopServerTrace(
			"Successfully started server %s",
			ServerGetName(spec->svrType));

		SvrManagerSendServerStatusUpdate(
			spec->svrType,
			instance,
			L"running");

		RunnerRemovePrivilagedThreadId(
			spec->svrType,
			svr->m_svrMachineIndex,
			GetCurrentThreadId() );
	}

	return TRUE;

_error:
	TraceError(
		"Error starting server type %!ServerType! instance %hu.",
		spec->svrType,
		instance );

	DesktopServerTrace(
		"ERROR starting server %s",
		ServerGetName(spec->svrType));

	SvrManagerSendServerOpResult(
		spec->svrType,
		instance,
		L"startserver",
		L"ERROR",
		errorString);

	WCHAR svrName[256];
	PStrCvt(svrName, ServerGetName(spec->svrType), 256);
    LogErrorInEventLog(EventCategoryServerRunner,MSG_ID_SERVER_FAILED_TO_START,svrName,NULL,0);

	//	undo our damage, unlock, and return
	ActiveServerShutdown( &g_runner->m_servers[spec->svrType][machineIndex] );
	g_runner->m_servers[spec->svrType][machineIndex].Server = NULL;
	g_runner->m_servers[spec->svrType][machineIndex].ServerLock.Unlock();
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ActiveServerShutdown(
	ACTIVE_SERVER_ENTRY * entry )
{
	ACTIVE_SERVER * svr = entry->Server;
	if( !svr ){
		return;
	}

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Beginning server shutdown for server %!ServerType! instance %hu.",
		svr->m_specs->svrType,
		svr->m_svrInstance );

	svr->m_bIsShuttingDown = TRUE;

	//	tell the server to shutdown
	ASSERT( ServerContextSet( entry ) );
	__try
	{
		if( !entry->Server->m_wasShutdownRequested &&
			 G_SERVERS[svr->m_specs->svrType]->svrCommandCallback &&
			 svr->m_svrData )
		{
			TraceInfo(
				TRACE_FLAG_SRUNNER,
				"Issuing shutdown command for server %!ServerType! instance %hu.",
				svr->m_specs->svrType,
				svr->m_svrInstance );
			G_SERVERS[ svr->m_specs->svrType ]->svrCommandCallback(
													svr->m_svrData,
													SR_COMMAND_SHUTDOWN );
		}
	}
	__except( RunnerExceptFilter(
				GetExceptionInformation(),
				svr->m_specs->svrType,
				svr->m_svrInstance,
				"Issuing shutdown command to server.") )
	{
        TraceError("We should never get here but code-analysis complains if this is empty");
	}
	ServerContextSet( (ACTIVE_SERVER_ENTRY*)NULL );

    if(svr->m_threads.Count() > 0)
    {
		TraceInfo(
			TRACE_FLAG_SRUNNER,
			"Waiting for all threads to exit for server %!ServerType! instance %hu.",
			svr->m_specs->svrType,
			svr->m_svrInstance );
        WaitForMultipleObjects(svr->m_threads.Count(),&svr->m_threads[0],TRUE,INFINITE);
    }
	//	release the created threads
	for( unsigned int ii = 0; ii < svr->m_threads.Count(); ++ii )
	{
		CloseHandle( svr->m_threads[ ii ] );
	}
	svr->m_threads.Destroy( svr->m_svrPool );

	//	shutdown the networking
	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Shutting down network connections for server %!ServerType! instance %hu.",
		svr->m_specs->svrType,
		svr->m_svrInstance );
	RunnerNetFreeConnections( svr->m_netCons );
	svr->m_netCons = NULL;

	//	free the active server struct
	SVRTYPE svrType = svr->m_specs->svrType;
	SVRINSTANCE svrInstance = svr->m_svrInstance;
	MEMORYPOOL * svrPool = svr->m_svrPool;
	svr->m_threadCreateCS.Free();
	svr->m_threadsLock.Free();
	FREE( svrPool, svr );

	//	free the servers memory pool
#if USE_MEMORY_MANAGER
	svr->m_svrPool->Term();
	IMemoryManager::GetInstance().RemoveAllocator(svr->m_svrPool);
	delete svr->m_svrPool;
	svr->m_svrPool = NULL;
	
	svr->m_svrAllocatorCRT->Term();
	IMemoryManager::GetInstance().RemoveAllocator(svr->m_svrAllocatorCRT);
	delete svr->m_svrAllocatorCRT;
	svr->m_svrAllocatorCRT = NULL;
#else	
	MemoryPoolFree( svrPool );
#endif

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Shutdown complete for server %!ServerType! instance %hu.",
		svrType,
		svrInstance );
}
