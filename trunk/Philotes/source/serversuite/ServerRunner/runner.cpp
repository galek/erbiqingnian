//----------------------------------------------------------------------------
// runner.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "dbgtrace.h"
#include "playertrace.h"

#include "cpu_usage.h"
#include "fileio.h"
#include "definition_priv.h"
#include "DatabaseManager.h"
#include "Ping0ServerMessages.h"
#include "NetworkConnections.h"
#include <cryptoapi.h>

#include "runner.cpp.tmh"

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
using namespace FSCommon;
#endif
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
struct LOGGING_DATA
{
	const char *	m_loggerName;
	ULONG			m_initialFlags;
	ULONG			m_initialLevel;
};


//----------------------------------------------------------------------------
// RUNNER GLOBALS
//----------------------------------------------------------------------------
SERVER_RUNNER *		g_runner			= NULL;
DWORD				g_contextTLSIndex	= TLS_OUT_OF_INDEXES;
static LOGGING_DATA g_logging			= {0};
static LOGGING_DATA g_playerActionLogging = {0};

#define DB_REQUEST_QUEUE_MAX_LENGTH 20000
DATABASE_MANAGER*	g_DatabaseManager[ DS_NUM_SELECTIONS ] = { 0 };

//----------------------------------------------------------------------------
// DATABASE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOnFatalDBEvent(
	DATABASE_MANAGER*,
	DB_FATAL_EVENT eventType,
	LPVOID )
{
	if (eventType == DB_FATAL_EVENT_ALL_CONNECTIONS_LOST)
	{
		TraceError("All database connections lost, shutting down.");

		LogErrorInEventLog(
			EventCategoryServerRunner,
			MSG_ID_LOST_ALL_DATABASE_CONNECTIONS,
			NULL,
			NULL,
			0 );
	}
	else if (eventType == DB_FATAL_EVENT_QUEUE_LENGTH_EXCEEDED)
	{
		TraceError("Database request queue excedeed max length, shutting down.");

		LogErrorInEventLog(
			EventCategoryServerRunner,
			MSG_ID_DB_REQUEST_QUEUE_TOO_LONG,
			NULL,
			NULL,
			0 );
	}

	RunnerStop("Fatal database connection pool error.");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sRunnerInitDatabaseManager(
	DATABASE_SELECTION eSelection,
	ServerRunnerDatabase * databaseConfig )
{
	ASSERT_RETFALSE( databaseConfig );
	
	database_manager_setup dbSetup;
	dbSetup.memPool					= NULL;
	dbSetup.databaseAddress			= databaseConfig->DatabaseAddress;
	dbSetup.databaseServer			= databaseConfig->DatabaseServer;
	dbSetup.databaseUserId			= databaseConfig->DatabaseUserId;
	dbSetup.databaseUserPassword	= databaseConfig->DatabaseUserPassword;
	dbSetup.databaseName			= databaseConfig->DatabaseName;
	dbSetup.bIsTrustedDatabase		= strstr(databaseConfig->IsTrustedDatabase, "true") != 0;
	dbSetup.databaseNetwork			= databaseConfig->DatabaseNetwork;
	dbSetup.nDatabaseConnections	= databaseConfig->DatabaseConnectionCount;
	dbSetup.nMaxRequestQueueLength  = DB_REQUEST_QUEUE_MAX_LENGTH;
	dbSetup.nRequestsPerConnectionBeforeReestablishingConnection = databaseConfig->RequestsPerConnectionBeforeReconnect;

	g_DatabaseManager[ eSelection ] = DatabaseManagerInit( dbSetup, sOnFatalDBEvent, 0 );
	
	return g_DatabaseManager[ eSelection ] != NULL;
	
}

//----------------------------------------------------------------------------
// HELPER FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRunnerInitLogging(
	const char * appName,
	ULONG initialFlags,
	ULONG initialLevel )
{
	ASSERT_RETFALSE( appName );
	g_logging.m_loggerName   = appName;
	g_logging.m_initialFlags = initialFlags;
	g_logging.m_initialLevel = initialLevel;

	WCHAR szLogFile[MAX_PATH];
	SYSTEMTIME sysTime;

	GetLocalTime( &sysTime );
	DWORD processId = GetCurrentProcessId();

	PStrPrintf(
		szLogFile,
		MAX_PATH,
		L"logs\\%S_%4hu.%02hu.%02hu_(%02hu.%02hu)_PID%u_%%d.etl",
		g_logging.m_loggerName,
		sysTime.wYear,
		sysTime.wMonth,
		sysTime.wDay,
		sysTime.wHour,
		sysTime.wMinute,
		processId );

	WCHAR szLoggerName[MAX_PATH];
	PStrCvt( szLoggerName, g_logging.m_loggerName, MAX_PATH );

#if !ISVERSION(DEBUG_VERSION)
	g_logging.m_initialFlags &= ~WPP_REAL_FLAG(TRACE_FLAG_DEBUG_BUILD_ONLY);
#endif

	ASSERT_RETFALSE(
		StartETWLogging(
		szLogFile,
		szLoggerName,
		&ServerRunner_WPPControlGuid,
		NULL,
		g_logging.m_initialFlags,
		g_logging.m_initialLevel ) );

	ASSERT_RETFALSE(StartPETS());

	
	// Player Transaction Logging
	g_playerActionLogging.m_loggerName = sgszPlayerActionLogName;

	WCHAR szPlayerActionLoggerName[MAX_PATH];
	PStrCvt( szPlayerActionLoggerName, g_playerActionLogging.m_loggerName, MAX_PATH );

	g_playerActionLogging.m_initialFlags = WPP_REAL_FLAG(PLAYER_ACTION_FLAG);
	g_playerActionLogging.m_initialLevel = TRACE_LEVEL_EXTRA_VERBOSE;

	WCHAR szPlayerActionLogFile[MAX_PATH];
	PStrPrintf(
		szPlayerActionLogFile,
		MAX_PATH,
		L"logs\\player\\%S_%4hu.%02hu.%02hu_(%02hu.%02hu)_PID%u_%%d.etl",
		g_playerActionLogging.m_loggerName,
		sysTime.wYear,
		sysTime.wMonth,
		sysTime.wDay,
		sysTime.wHour,
		sysTime.wMinute,
		processId );

	TRACEHANDLE hPlayerActionTrace = NULL;
	
	ASSERT_RETFALSE(
		StartETWLogging(
		szPlayerActionLogFile,
		szPlayerActionLoggerName,
		&PlayerAction_WPPControlGuid,
		&hPlayerActionTrace,
		g_playerActionLogging.m_initialFlags,
		g_playerActionLogging.m_initialLevel,
		5 ) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRunnerFreeLogging(
	void )
{
	StopPETS();
	if( g_logging.m_loggerName )
	{
		WCHAR szLoggerName[MAX_PATH];
		PStrCvt( szLoggerName, g_logging.m_loggerName, MAX_PATH );
		StopETWLogging( szLoggerName );
	}
	if(g_playerActionLogging.m_loggerName)
	{
		WCHAR szLoggerName[MAX_PATH];
		PStrCvt( szLoggerName, g_playerActionLogging.m_loggerName, MAX_PATH );
		StopETWLogging( szLoggerName );
	}
	g_logging.m_loggerName = NULL;
	g_playerActionLogging.m_loggerName = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRunnerProcessCommandLine(
	int		argc,
	char*	argv[] )
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRunnerFree(
	struct MEMORYPOOL *	pool,
	SERVER_RUNNER *		toFree )
{
	if( !toFree )
		return;

	//	halt active servers
	TraceInfo(TRACE_FLAG_SRUNNER,"Shutting down servers.");
	for( SVRTYPE svrType = 0; svrType < TOTAL_SERVER_COUNT; ++svrType )
	{
		for( SVRMACHINEINDEX svrMIndex = 0; svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE; ++svrMIndex )
		{
			ACTIVE_SERVER_ENTRY & entry = toFree->m_servers[ svrType ][ svrMIndex ];

			if( entry.Server )
			{
				entry.ServerLock.Lock();
				entry.ServerLock.WaitZeroRef();

				//	free the server
				ActiveServerShutdown( &entry );
			}
			entry.Server = NULL;
			entry.ServerLock.Free();
			entry.ServerThreadIds.Destroy( toFree->m_entryPool );
		}
	}

	//	free the network wrapper
	TraceInfo(TRACE_FLAG_SRUNNER,"Shutting down network layer.");
	RunnerNetClose();

	//	free the net map
	if( toFree->m_netMap )
	{
		NetMapFree( toFree->m_netMap );
	}

#if USE_MEMORY_MANAGER
	toFree->m_netPool->Term();
	IMemoryManager::GetInstance().RemoveAllocator(toFree->m_netPool);
	delete toFree->m_netPool;
	toFree->m_netPool = NULL;

	toFree->m_netPoolCRT->Term();
	IMemoryManager::GetInstance().RemoveAllocator(toFree->m_netPoolCRT);
	delete toFree->m_netPoolCRT;
	toFree->m_netPoolCRT = NULL;
#else
    MemoryPoolFree(toFree->m_netPool);
#endif
    
	//	free the runner memory
	FREE( pool, toFree );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SERVER_RUNNER * sRunnerNew(
	struct MEMORYPOOL * pool,
	int					argc,
	char *				argv[],
	const char *		appName )
{
	//	get the memory
	SERVER_RUNNER * toRet = (SERVER_RUNNER*)MALLOCZ( pool, sizeof( SERVER_RUNNER ) );
	ASSERT_RETNULL( toRet );

	//	init the active server holders
	toRet->m_entryPool = pool;
	for( SVRTYPE svrType = 0; svrType < TOTAL_SERVER_COUNT; ++svrType )
	{
		for( SVRMACHINEINDEX svrMIndex = 0; svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE; ++svrMIndex )
		{
			toRet->m_servers[ svrType ][ svrMIndex ].ServerLock.Init();
			toRet->m_servers[ svrType ][ svrMIndex ].Server = NULL;
			toRet->m_servers[ svrType ][ svrMIndex ].ServerThreadIds.Init();
		}
	}

	//	init the server type entries
	for( SVRTYPE svrType = 0; svrType < TOTAL_SERVER_COUNT; ++svrType )
	{
		BOOL svrOK = TRUE;
		svrOK = svrOK && G_SERVERS[ svrType ]->Validate();
		svrOK = svrOK && G_SERVER_CONNECTIONS[ svrType ].Init( G_SERVERS[ svrType ]->svrConnectionTableInit );
		ASSERT_GOTO(svrOK,_INIT_ERROR);
	}

	//	init system
	sRunnerProcessCommandLine( argc, argv );

	//	get a net map
	toRet->m_netMap = NetMapNew( NULL );
	ASSERT_GOTO( toRet->m_netMap, _INIT_ERROR );

	//	init the network wrapper
	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing network layer.");
	
#if USE_MEMORY_MANAGER
	CMemoryAllocatorCRT<true>* netPoolCRT = new CMemoryAllocatorCRT<true>(L"CommonNetPoolCRT");
	ASSERT_RETFALSE(netPoolCRT);
	netPoolCRT->Init(NULL);
	IMemoryManager::GetInstance().AddAllocator(netPoolCRT);
	
	CMemoryAllocatorPOOL<true, true, 1024>* netPool = new CMemoryAllocatorPOOL<true, true, 1024>(L"CommonNetPool");
	ASSERT_RETFALSE(netPool);
	netPool->Init(netPoolCRT, 32 * KILO, false, false);
	IMemoryManager::GetInstance().AddAllocator(netPool);
	
	toRet->m_netPoolCRT = netPoolCRT;
	toRet->m_netPool = netPool;
#else
    MEMORYPOOL *netPool = MemoryPoolInit(L"CommonNetPool",DefaultMemoryAllocator);
    ASSERT_RETFALSE(netPool);
    toRet->m_netPool = netPool;    
#endif
    
	ASSERT_GOTO(
		RunnerNetInit(
			netPool,
			toRet->m_netMap,
			appName ),
		_INIT_ERROR );

	//	done, return runner
	return toRet;

_INIT_ERROR:
	TraceError("Error initializing server runner.");
	sRunnerFree(pool,toRet);
	return NULL;
}


//----------------------------------------------------------------------------
// PROGRAM CONTROL FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerIsRunning(
	const char * appMutexName )
{
	static HANDLE sghMutex = CreateMutex(NULL, TRUE, appMutexName);

	if ( !sghMutex )
	{
		trace( "Failed to create \"%s\" mutex, exiting", appMutexName );
		return TRUE;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		trace( "Mutex \"%s\" already exists, exiting", appMutexName );
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerInitSystems(
	BOOL runningAsService,
	const char * appName )
{
	ASSERT_RETFALSE(RunnerSetWorkingDirectory(runningAsService));

	ASSERT_RETFALSE(InitEventLog());

	ASSERT_RETFALSE(RunnerInitDefaultLogging(appName));

	// now we can take a dump,if needed
	(void)SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)RunnerTakeDump);
	
	ASSERT_RETFALSE(CryptoInit());

	ASSERT_RETFALSE(PerfHelper::Init());

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerSetWorkingDirectory(
	BOOL bRunningAsService )
{
	trace("running as service %d\n",bRunningAsService);
	OS_PATH_CHAR path[256];
    if (!bRunningAsService)
    {
        ASSERTX_RETFALSE(PGetModuleDirectory(path, _countof(path)),"Couldn't get root directory.");
        ASSERTX_RETFALSE(PSetCurrentDirectory(path),"Couldn't set root directory.");
    }
    else
    { // Services start in system32 by default. We don't want that.
        // We also don't want to do the silly walk up the tree like above. Just find where the exe is.
       OS_PATH_CHAR *p = NULL;
       ASSERTX_RETFALSE(OS_PATH_FUNC(GetModuleFileName)(NULL,path,ARRAYSIZE(path)) <= ARRAYSIZE(path),"Could not get module filename");
       p = const_cast<OS_PATH_CHAR *>(PStrRChr(path,OS_PATH_TEXT('\\'))) + 1;
       *p = OS_PATH_TEXT('\0');
       OS_PATH_FUNC(SetCurrentDirectory)(path);

    }
	trace("Set current directory: %s\n", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(path));
	PStrCopy(gtAppCommon.m_szRootDirectory, path, _countof(gtAppCommon.m_szRootDirectory));
	gtAppCommon.m_nRootDirectoryStrLen = PStrLen(gtAppCommon.m_szRootDirectory, _countof(gtAppCommon.m_szRootDirectory));
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerInit(
		int				argc,
		char*			argv[],
		const char *	appName,
		ServerRunnerConfig * appConfig,
		ServerRunnerDatabase * gameDatabaseConfig,
		ServerRunnerLogDatabase * logDatabaseConfig )
{
	ASSERT_RETFALSE(appConfig);

	//	set silent assert option
	gtAppCommon.m_bSilentAssert = appConfig->AssertSilently;
#if ISVERSION(RETAIL_VERSION)
	gtAppCommon.m_bSilentAssert = TRUE;
#endif

	//	init logging
	SetETWLoggingControls(
		appConfig->WppTraceFlags,
		appConfig->WppTraceLevel,
		&ServerRunner_WPPControlGuid,
		NULL );
	TraceInfo(TRACE_FLAG_SRUNNER,"Logging initialized.");

	//  player action logging config setting
	PlayerActionLoggingEnable(appConfig->EnablePlayerTrace);
	TraceInfo(TRACE_FLAG_SRUNNER,"Player action logging is %s.", appConfig->EnablePlayerTrace ? "ENABLED" : "DISABLED");

	////	init server runner ////

	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing server context system.");
	ASSERT_RETFALSE( ServerContextInit() );

	// clear all database manager pointers
	for (int i = 0; i < DS_NUM_SELECTIONS; ++i)
	{
		g_DatabaseManager[ i ] = NULL;
	}

	// init game database manager	
	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing game database manager.");
	ASSERT_RETFALSE(sRunnerInitDatabaseManager(DS_GAME, gameDatabaseConfig));

	// init log database manager (optional for now until all configs and
	// databases are in place)
	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing log database manager.");
	sRunnerInitDatabaseManager(DS_LOG_WAREHOUSE, logDatabaseConfig);
	
	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing server manager.");
	ASSERT_RETFALSE(
		SvrManagerInit(
			NULL,
			appConfig->CommandPipeName,
			appConfig->XmlMessageDefinitions ) );

	TraceInfo(TRACE_FLAG_SRUNNER,"Initializing server runner.");

	g_runner = sRunnerNew(
					NULL,
					argc,
					argv,
					appName );


	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerInitDefaultLogging(const char *appName)
{
    ASSERT_RETFALSE(sRunnerInitLogging(
                    appName,
                    WPP_TRACE_FLAGS_ALL,
                    TRACE_LEVEL_EXTRA_VERBOSE ) );

	SetETWLoggingControls(
		WPP_TRACE_FLAGS_ALL,
		TRACE_LEVEL_EXTRA_VERBOSE,
		&ServerRunner_WPPControlGuid,
		NULL );

    return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerRun(
		void )
{
	//	have the main thread enter the svr manager message loop
	TraceInfo(TRACE_FLAG_SRUNNER,"Beginning server manager main loop.");
	SvrManagerMainMsgLoop();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerClose(
		void )
{
	TraceInfo(TRACE_FLAG_SRUNNER,"Shutting down systems.");
	
	////	free server runner systems	////
	TraceInfo(TRACE_FLAG_SRUNNER,"Closing server runner.");
	sRunnerFree( NULL, g_runner );
	g_runner = NULL;
	TraceInfo(TRACE_FLAG_SRUNNER,"Closing server manager.");
	SvrManagerFree();
	TraceInfo(TRACE_FLAG_SRUNNER,"Closing server context system.");
	ServerContextFree();
	TraceInfo(TRACE_FLAG_SRUNNER,"Closing database manager.");
	
	for (int i = 0; i < DS_NUM_SELECTIONS; ++i)
	{
		if (g_DatabaseManager[ i ])
		{
			DatabaseManagerFree( g_DatabaseManager[ i ] );
			g_DatabaseManager[ i ] = NULL;
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerCloseLogging(
	void )
{
	////	free common app framework	////
	TraceInfo(TRACE_FLAG_SRUNNER,"Closing logging.");
	sRunnerFreeLogging();
}


//----------------------------------------------------------------------------
//	RUNNER SERVER NET CALLBACKS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPVOID	RunnerAcceptClientConnection(
	SVRTYPE			svrType,
	SVRMACHINEINDEX svrMIndex,
	SERVICEUSERID	userId,
	BYTE            *pAcceptData,
	DWORD            acceptDataLen,
	BOOL & acceptSuccess)
{
	acceptSuccess = FALSE;
	ASSERT_RETNULL( g_runner );
	ASSERT_RETNULL( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	ASSERT_RETNULL( userId != INVALID_SERVICEUSERID );

	LPVOID toRet = NULL;

	ACTIVE_SERVER_ENTRY * entry = RunnerGetServerEntry( svrType, svrMIndex );
	if( entry )
	{
		//	get this connection spec
		SVRTYPE userType = ServiceUserIdGetUserType( userId );
		CONNECTION & connection = G_SERVER_CONNECTIONS[ svrType ].OfferedServices[ userType ];
		ASSERTX_GOTO(
			connection.ConnectionSet,
			"Recieved an accept callback for an un-set connection!",
			_ACCEPT_ERROR );
		if (!entry->Server->m_netCons)
		{
			TraceWarn(TRACE_FLAG_SRUNNER,
				"Server %!ServerType! received an accept before it was ready for connections from a %!ServerType!",
				svrType, userType);
			goto _ACCEPT_ERROR;
		}

		// MuxClientId for the first connection is "special". Runner will ignore this and not
		// notify the service about this connection.
		if(!((ServiceUserIdGetUserType(userId) == USER_SERVER)
            && (IsSpecialMuxClientId(ServiceUserIdGetConnectionId(userId)) == TRUE) ) )
		{
			NetConsClientAttached(entry->Server->m_netCons, userId);
			if( connection.OnClientAttached )
			{
				//	set our context
				ASSERT_GOTO( ServerContextSet( entry ), _ACCEPT_ERROR );
				//	call it
				__try
				{
					toRet = connection.OnClientAttached(
											entry->Server->m_svrData, userId ,
											pAcceptData,
											acceptDataLen );
				}
				__except( RunnerExceptFilter(
							GetExceptionInformation(),
							entry->Server->m_specs->svrType,
							entry->Server->m_svrInstance,
							"Processing server Client Attached callback.") )
				{
                    TraceError("We should never get here but code-analysis complains if this is empty");
				}
				ServerContextSet((ACTIVE_SERVER_ENTRY*)NULL);
			}
		}//!(USER_SERVER && SPECIAL_MUXCLIENTID)
	}
	acceptSuccess = TRUE;
_ACCEPT_ERROR:
	RunnerReleaseServerEntry( entry );
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RunnerProcessRequest(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMIndex,
	SERVICEUSERID	userId,
	LPVOID			userContext,
	NETMSG *		msgData )
{
    BOOL bRet = FALSE;
	ACTIVE_SERVER_ENTRY *svr = NULL;
	CBRA( g_runner );
	CBRA( svrType < TOTAL_SERVER_COUNT );
	CBRA( svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	CBRA( userId != INVALID_SERVICEUSERID );
	CBRA( msgData );

    svr = RunnerGetServerEntry( svrType, svrMIndex );
	if( svr )
	{
		if( msgData->Mailbox )
		{
			//	mailbox is set, add message

			//	fill out a metadata struct
			RAW_MSG_METADATA msgMetaData;
			msgMetaData.IsRequest		= TRUE;
			msgMetaData.IsFake			= FALSE;
			msgMetaData.ServiceUserId	= userId;
			msgMetaData.ServiceProvider	= svrType;
			msgMetaData.ServiceUser		= ServiceUserIdGetUserType( userId );

			//	add the mailbox
			if( !SvrMailboxAddRawMessage(
					msgData->Mailbox,
					msgMetaData,
					msgData->Data,
					msgData->DataLength ) )
			{
				TraceError(
					"Error adding mailbox message for server %!ServerType! instance %hu from server %!ServerType! instance %hu.",
					svrType,
					svr->Server->m_svrInstance,
					ServiceUserIdGetUserType(userId),
					ServiceUserIdGetUserInstance(userId) );
				ASSERT_MSG("Error adding mailbox request.");
				goto Error;
			}
			bRet = TRUE;
		}
		else
		{
			//	no mailbox set, try immediate callback

			//	translate the message
			BYTE			buf_data[ MAX_SMALL_MSG_STRUCT_SIZE ];
			MSG_STRUCT *	msg = (MSG_STRUCT*)buf_data;

            CBRA(msgData->DataLength <= MAX_SMALL_MSG_STRUCT_SIZE);
			CBRA(NetTranslateMessageForRecv(
					msgData->CmdTbl,
					msgData->Data,
					msgData->DataLength,
					msg ));

			//	get this connection spec
			SVRTYPE		 userType   = ServiceUserIdGetUserType( userId );
			CONNECTION & connection = G_SERVER_CONNECTIONS[ svrType ].OfferedServices[ userType ];
			ASSERTX_GOTO(
				connection.ConnectionSet,
				"Request received for connection that is not set!",
				Error );

			//	handle the message
			if( connection.RequestCommandHandlers && connection.RequestCommandHandlers[ msg->hdr.cmd ] )
			{
				//	set our context
				ASSERT_GOTO( ServerContextSet( svr ), Error );

				//	run the handler
				__try
				{
					connection.RequestCommandHandlers[ msg->hdr.cmd ](
						svr->Server->m_svrData,
						userId,
						msg,
						userContext );
					bRet = TRUE;
				}
				__except( RunnerExceptFilter(
							GetExceptionInformation(),
							svrType,
							svr->Server->m_svrInstance,
							"Processing client request handler callback.") )
				{
                    TraceError("We should never get here but code-analysis complains if this is empty");
				}
			}
			else
			{
				//	no mailbox and no handler, lost message...
				WARNX( connection.RequestCommandHandlers && connection.RequestCommandHandlers[ msg->hdr.cmd ], "no mailbox and no handler, lost message..." );
			}
		}
	}
	else
	{
		TraceWarn(
			TRACE_FLAG_SRUNNER,
			"Recieved request for inactive server of type %!ServerType! from server %!ServerType! instance %hu.",
			svrType,
			ServiceUserIdGetUserType(userId),
			ServiceUserIdGetUserInstance(userId) );
	}
Error:
    if(svr)
    {
        RunnerReleaseServerEntry( svr );
    }
    return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerProcessResponse(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMIndex,
	SVRID			serviceSvrId,
	NETMSG *		msgData )
{
	ASSERT_RETURN( g_runner );
	ASSERT_RETURN( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	ASSERT_RETURN( serviceSvrId != INVALID_SVRID );
	ASSERT_RETURN( msgData );

	ACTIVE_SERVER_ENTRY * svr = RunnerGetServerEntry( svrType, svrMIndex );
	if( svr )
	{
		if( msgData->Mailbox )
		{
			//	mailbox is set, add message

			//	fill out a metadata struct
			RAW_MSG_METADATA msgMetaData;
			msgMetaData.IsRequest		  = FALSE;
			msgMetaData.IsFake			  = FALSE;
			msgMetaData.ServiceProviderId = serviceSvrId;
			msgMetaData.ServiceProvider	  = ServerIdGetType( serviceSvrId );
			msgMetaData.ServiceUser		  = svrType;

			//	add the mailbox
			if( !SvrMailboxAddRawMessage(
					msgData->Mailbox,
					msgMetaData,
					msgData->Data,
					msgData->DataLength ) )
			{
				TraceError(
					"Error adding mailbox message for server %!ServerType! instance %hu from server %!ServerType! instance %hu.",
					svrType,
					svr->Server->m_svrInstance,
					ServerIdGetType(serviceSvrId),
					ServerIdGetInstance(serviceSvrId) );
				ASSERT_MSG("Error adding mailbox response.");
			}
		}
		else
		{
			BYTE			buf_data[ MAX_LARGE_MSG_STRUCT_SIZE ];
			MSG_STRUCT *	msg = (MSG_STRUCT*)buf_data;

			//	no mailbox set, try immediate callback
			//	get this connection spec
			bool bHandled = false;
			CONNECTION & connection = G_SERVER_CONNECTIONS[ svrType ].RequiredServices[ ServerIdGetType( serviceSvrId ) ];
			ASSERTX_GOTO(
				connection.ConnectionSet,
				"Response recieved for connection that is not set.",
				_error );

			if(connection.RawReadHandler != NULL)
			{
				ASSERT_GOTO( ServerContextSet(svrType,svrMIndex), _error );
				bHandled = true;
				//	run the handler
				__try
				{
					connection.RawReadHandler(svr->Server->m_svrData,serviceSvrId,msgData->Data,msgData->DataLength);
				}
				__except( RunnerExceptFilter(
							GetExceptionInformation(),
							svrType,
							svr->Server->m_svrInstance,
							"Processing server raw read callback.") )
				{
                    TraceError("We should never get here but code-analysis complains if this is empty");
				}
			}
			if(!bHandled)
			{
				//	translate the message
				ASSERT_GOTO(NetTranslateMessageForRecv(
					msgData->CmdTbl,
					msgData->Data,
					msgData->DataLength,
					msg ),_error);
			}

			//	handle the message
			if(!bHandled && connection.ResponseCommandHandlers[ msg->hdr.cmd ] )
			{
				//	set our context
				ASSERT_GOTO( ServerContextSet( svr ), _error );

				//	run the handler
				__try
				{
					connection.ResponseCommandHandlers[ msg->hdr.cmd ](
						svr->Server->m_svrData,
						serviceSvrId,
						msg );
				}
				__except( RunnerExceptFilter(
							GetExceptionInformation(),
							svrType,
							svr->Server->m_svrInstance,
							"Processing server response callback.") )
				{
                    TraceError("We should never get here but code-analysis complains if this is empty");
				}
				bHandled = true;
			}
_error:
			if(!bHandled)
			{
				//	no mailbox and no handler, lost message...
				WARNX( connection.ResponseCommandHandlers[ msg->hdr.cmd ], "no mailbox and no handler, lost message..." );
			}
		}
	}
	RunnerReleaseServerEntry( svr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void	RunnerProcessClientDisconnect(
	SVRTYPE				svrType,
	SVRMACHINEINDEX		svrMIndex,
	SERVICEUSERID		userId,
	LPVOID				userContext )
{
	ASSERT_RETURN( g_runner );
	ASSERT_RETURN( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( svrMIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	ASSERT_RETURN( userId != INVALID_SERVICEUSERID );

	ACTIVE_SERVER_ENTRY * svr = RunnerGetServerEntry( svrType, svrMIndex );
	if( svr )
	{
		//	get this connection spec
		SVRTYPE userType = ServiceUserIdGetUserType( userId );
		CONNECTION & connection = G_SERVER_CONNECTIONS[ svrType ].OfferedServices[ userType ];
		ASSERTX_GOTO(
			connection.ConnectionSet,
			"Disconnect recieved for connection that is not set!",
			_error );

		NetConsClientDetached(svr->Server->m_netCons, userId);
		if( connection.OnClientDetached )
		{
			//	set our context
			ASSERT_GOTO( ServerContextSet( svr ), _error );
			//	call it
			__try
			{
				connection.OnClientDetached( svr->Server->m_svrData, userId, userContext );
			}
			__except( RunnerExceptFilter(
						GetExceptionInformation(),
						svrType,
						svr->Server->m_svrInstance,
						"Processing server Client Detached callback.") )
			{
                    TraceError("We should never get here but code-analysis complains if this is empty");
			}
		}
	}
_error:
	RunnerReleaseServerEntry( svr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NET_CONS * RunnerGetServerNetCons(
	SVRTYPE			svrType,
	SVRMACHINEINDEX svrMachineIndex )
{
	ASSERT_RETNULL( g_runner );
	ASSERT_RETNULL( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );

	ACTIVE_SERVER * svr = RunnerGetServer( svrType, svrMachineIndex );
	if( svr )
	{
		return svr->m_netCons;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerReleaseServerNetCons(
	SVRTYPE			svrType,
	SVRMACHINEINDEX svrMachineIndex )
{
	ASSERT_RETURN( g_runner );
	ASSERT_RETURN( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );

	RunnerReleaseServer( svrType, svrMachineIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerStop(
	const char * reason )
{
	WCHAR buff[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrPrintf(
		buff,
		CMD_PIPE_TEXT_BUFFER_LENGTH,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</source>"
				L"<dest>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"<shutdown><reason>%S</reason></shutdown>"
			L"</body>"
		L"</cmd>",
		RunnerNetGetLocalIp(),
		RunnerNetGetLocalIp(),
		reason );
	SvrManagerEnqueueMessage(buff);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RunnerStopServer(
	SVRTYPE svrType,
	SVRINSTANCE svrInstance,
	SVRMACHINEINDEX svrMIndex )
{
	WCHAR buff[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrPrintf(
		buff,
		CMD_PIPE_TEXT_BUFFER_LENGTH,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</source>"
				L"<dest>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"<stopserver>"
					L"<type>%S</type>"
					L"<instance>%u</instance>"
					L"<machineindex>%u</machineindex>"
				L"</stopserver>"
			L"</body>"
		L"</cmd>",
		RunnerNetGetLocalIp(),
		RunnerNetGetLocalIp(),
		ServerGetName(svrType),
		svrInstance,
		svrMIndex);
	SvrManagerEnqueueMessage(buff);
}
