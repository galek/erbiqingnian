//----------------------------------------------------------------------------
// ServerRunnerAPI.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "ActiveServer.h"
#include "servercontext.h"
#include "definition_priv.h"
#include "NetworkConnections.h"

#include "ServerRunnerAPI.cpp.tmh"


//----------------------------------------------------------------------------
// ACTIVE SERVER FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPVOID CurrentSvrGetContext(
	void )
{
	ACTIVE_SERVER *	server = ServerContextGet();
	if(!server )
    {
        TraceInfo(TRACE_FLAG_SRUNNER_NET,"ServerContextGet returned NULL.");
        return NULL;
    }

	LPVOID toRet = server->m_svrData;
	ServerContextRelease();
	
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRTYPE CurrentSvrGetType(
	void )
{
	ACTIVE_SERVER *	server = ServerContextGet();
	ASSERT_RETX( server, INVALID_SVRTYPE );

	SVRTYPE toRet = server->m_specs->svrType;
	ServerContextRelease();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRINSTANCE CurrentSvrGetInstance(
	void )
{
	ACTIVE_SERVER *	server = ServerContextGet();
	ASSERT_RETX( server, INVALID_SVRINSTANCE );

	SVRINSTANCE toRet = server->m_svrInstance;
	ServerContextRelease();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MEMORYPOOL * CurrentSvrGetMemPool(
	void )
{
	ACTIVE_SERVER *	server = ServerContextGet();
	ASSERT_RETX( server, NULL );	//	TODO: figure out a mechanism for specifying an invalid mem pool handle...

	MEMORYPOOL * toRet = server->m_svrPool;
	ServerContextRelease();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CurrentSvrSetThreadContext(
	SVRID serverId )
{
	ServerContextSet( RunnerLookupServerContext( serverId ) );
}


//----------------------------------------------------------------------------
// DATABASE FUNCTIONS
//----------------------------------------------------------------------------
extern DATABASE_MANAGER* g_DatabaseManager[ DS_NUM_SELECTIONS ]; // in runner.cpp

DATABASE_MANAGER* SvrGetGameDatabaseManager(
	DATABASE_SELECTION eDatabase /*= DS_GAME*/)
{ 
	ASSERTX_RETNULL( eDatabase != DS_INVALID && eDatabase < DS_NUM_SELECTIONS, "Invalid database selection" );
	return g_DatabaseManager[ eDatabase ]; 
}

//----------------------------------------------------------------------------
// SERVER RUNNER SYSTEM FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD WINAPI	sSvrRunnerThreadStarter(
	LPVOID lpCreatingSvr )
{
	ACTIVE_SERVER * server = (ACTIVE_SERVER*)lpCreatingSvr;
	ASSERT_RETX( server, THREAD_ERROR );
	ASSERT_RETX( server->m_threadCreateFunc, THREAD_ERROR );

	DWORD toRet = THREAD_ERROR;

	//	retrieve the thread creation parameters
	LPTHREAD_START_ROUTINE  toRun = server->m_threadCreateFunc;
	LPVOID					arg   = server->m_threadCreateArg;

	//	set our context to that of the creating servers
	ASSERT_RETX(
		RunnerAddPrivilagedThreadId(
			server->m_specs->svrType,
			server->m_svrMachineIndex,
			GetCurrentThreadId() ),
		THREAD_ERROR );
	ServerContextSet(
		server->m_specs->svrType,
		server->m_svrMachineIndex );

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Worker thread id %u starting for server %!ServerType! instance %hu.",
		GetCurrentThreadId(),
		server->m_specs->svrType,
		server->m_svrInstance );

	//	release the server thread lock for more ops
	server->m_threadCreateCS.Leave();

	//	now run the thread
	__try
	{
		toRet = toRun( arg );
	}
	__except( RunnerExceptFilter(
				GetExceptionInformation(),
				server->m_specs->svrType,
				server->m_svrInstance,
				"Processing server entry point thread.") )
	{
        TraceError("We should never get here but code-analysis complains if this is empty");
	}

	RunnerRemovePrivilagedThreadId(
		server->m_specs->svrType,
		server->m_svrMachineIndex,
		GetCurrentThreadId() );

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Worker thread id %u exiting for server %!ServerType! instance %hu.",
		GetCurrentThreadId(),
		server->m_specs->svrType,
		server->m_svrInstance );

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HANDLE SvrRunnerCreateThread(
	DWORD					stacksize,
	LPTHREAD_START_ROUTINE	threadFunc,
	LPVOID					threadArg )
{
	HANDLE toRet = NULL;

	//	get the server
	ACTIVE_SERVER * svr = ServerContextGet();
	ASSERT_RETNULL( svr );

	//	only one thread may be created at a time, so lock the cs
	svr->m_threadCreateCS.Enter();
	svr->m_threadsLock.Enter();
	ONCE
	{
		//	make sure they may create more threads
		if( svr->m_threads.Count() >= MAX_SVR_THREADS ) {
			break;
		}

		//	set the creation data
		svr->m_threadCreateArg  = threadArg;
		svr->m_threadCreateFunc = threadFunc;
		
		//	try creating the thread
		toRet = CreateThread(
							NULL,
							stacksize,
							sSvrRunnerThreadStarter,
							svr,
							0,
							NULL );

		//	make sure it was created successfully
		ASSERT_BREAK( toRet );

		//	save the handle
		ASSERT( svr->m_threads.Insert( svr->m_svrPool, toRet ) );
	}
	svr->m_threadsLock.Leave();

	//	unlock the active server and thread cs
	if (toRet == NULL)
		svr->m_threadCreateCS.Leave();
	ServerContextRelease();

	//	return the handle 
	return toRet;
}

//----------------------------------------------------------------------------
//	waits for thread completion for timeout length then returns the exit code of the thread.
//----------------------------------------------------------------------------
DWORD SvrRunnerFreeThread(
	HANDLE thread,
	DWORD  timeout )
{
	//	get the freeing server
	ACTIVE_SERVER * server = ServerContextGet();
	ASSERT_RETX( server, THREAD_ERROR );

	//	return value
	DWORD toRet = THREAD_ERROR;

	server->m_threadCreateCS.Enter();
	server->m_threadsLock.Enter();
	ONCE
	{
		//	make sure they own the thread
		ASSERT_BREAK( server->m_threads.FindExact( thread ) );

		//	try to wait for the thread
		DWORD result = WaitForSingleObject( thread, timeout );

		//	make sure thread is dead...
		ASSERTX( result != WAIT_TIMEOUT, "Server free thread timed out while waiting for thread to exit." );

		//	get return code
		toRet = ( result == WAIT_TIMEOUT ) ? THREAD_ERROR : GetExitCodeThread( thread, &toRet );

		//	free the thread object
		ASSERT( CloseHandle( thread ) );

		//	remove the thread from the servers list
		ASSERT( server->m_threads.Delete( server->m_svrPool, thread ,__FILE__,__LINE__) );
	}
	server->m_threadsLock.Leave();
	server->m_threadCreateCS.Leave();
	ServerContextRelease();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrRunnerKillServer(
	void )
{
	ACTIVE_SERVER * server = ServerContextGet();
	if( !server )
		return;	//	already shut down.

	server->m_wasShutdownRequested = TRUE;

	RunnerStopServer(
		server->m_specs->svrType,
		server->m_svrInstance,
		server->m_svrMachineIndex);

	ServerContextRelease();
}

//----------------------------------------------------------------------------
// SERVER RUNNER NETWORKING FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
IMuxClient *SvrNetGetMuxClientForSvr(SVRID svrId)
{
    IMuxClient *toRet = NULL;
	ACTIVE_SERVER * server = ServerContextGet();
    if(server)
    {
        toRet = RunnerNetGetMuxClient(
					server->m_netCons,
                    ServiceId(ServerIdGetType(svrId),
                              ServerIdGetInstance(svrId),
                              server->m_specs->svrType,
                              server->m_svrInstance));
		ServerContextRelease();
    }
    return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE SvrNetSendRequestToService(
	SVRID		 svrId,
	MSG_STRUCT * msg,
	NET_CMD		 command )
{
	SRNET_RETURN_CODE toRet = SRNET_FAILED;
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		if(!server->m_bIsShuttingDown)
		{
			toRet = RunnerNetSendRequest(
						server->m_netCons,
						ServiceId(
							ServerIdGetType( svrId ),
							ServerIdGetInstance( svrId ),
							server->m_specs->svrType,
							server->m_svrInstance ),
						msg,
						command );
		}
		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE SvrNetSendResponseToClient(
	SERVICEUSERID	userId,
	MSG_STRUCT *	msg,
	NET_CMD			command )
{
	SRNET_RETURN_CODE toRet = SRNET_FAILED;
    ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		if (ServiceUserIdGetConnectionId(userId) == INVALID_SVRCONNECTIONID) {
			// the user server may have multiple connections per instance.
			DBG_ASSERT(ServiceUserIdGetUserType(userId) != USER_SERVER);
			userId = NetConsGetClientServiceUserId(server->m_netCons, ServiceUserIdGetServer(userId));
		}
		if(!server->m_bIsShuttingDown)
		{
			toRet = RunnerNetSendResponse(
						server->m_netCons,
						userId,
						msg,
						command );
		}
		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE SvrNetSendResponseToClients(
	SVRID				serverId,
	SVRCONNECTIONID *	connectedClients,
	UINT				numClients,
	MSG_STRUCT *		message,
	NET_CMD				command )
{
	SRNET_RETURN_CODE toRet = SRNET_FAILED;
	ACTIVE_SERVER * server = ServerContextGet();
	if(server)
	{
		if(!server->m_bIsShuttingDown)
		{
			toRet = RunnerNetSendResponseToClients(server->m_netCons,
												serverId,
												connectedClients,
												numClients,
												message,
												command);
		}
		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRNET_RETURN_CODE SvrNetSendBufferToClient( SERVICEUSERID userId,
                               __notnull BYTE *pBuffer,
                               DWORD bufferLen)
{
	SRNET_RETURN_CODE toRet = SRNET_FAILED;
    ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		if(!server->m_bIsShuttingDown)
		{
			toRet = RunnerNetSendBuffer(
						server->m_netCons,
						userId,
						pBuffer,
						bufferLen );
		}
		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPVOID SvrNetGetClientContext(
	SERVICEUSERID	userId )
{
	void *toRet = NULL;
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		toRet = RunnerNetGetClientContext(
					server->m_netCons,
					userId
					);

		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrNetAttachRequestMailbox(		//	pass NULL for mailslot to un-attach
	SERVICEUSERID	 userId,
	SERVER_MAILBOX * mailbox )
{
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		RunnerNetAttachRequestMailbox(
					server->m_netCons,
					userId,
					mailbox );

		ServerContextRelease();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrNetAttachResponseMailbox(		//	pass NULL for mailslot to un-attach
	SVRID	 userId,
	SERVER_MAILBOX * mailbox )
{
	ACTIVE_SERVER * server = ServerContextGet();

	SERVICEID serviceId = ServiceId(
		ServerIdGetType(userId), ServerIdGetInstance(userId),
		CurrentSvrGetType(), CurrentSvrGetInstance() );
	if( server )
	{
		RunnerNetAttachResponseMailbox(
			server->m_netCons,
			serviceId,
			mailbox );

		ServerContextRelease();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LPVOID SvrNetDisconnectClient(
	SERVICEUSERID userId )
{
	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Attempting to disconnect SERVICEUSERID %#018I64x\n", userId);
	LPVOID toRet = NULL;
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		toRet = RunnerNetDisconnectClient(
					server->m_netCons,
					userId );

		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetPopMailboxMessage(
	SERVER_MAILBOX *	mailbox,
	MAILBOX_MSG *		msgDest )
{
	return RunnerNetPopMailboxMessage( mailbox, msgDest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetPopMailboxListMessage(
	MAILBOX_MSG_LIST *	messageList,
	MAILBOX_MSG *		msgDest )
{
	return RunnerNetPopMailboxMessage( messageList, msgDest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetPostFakeServerResponseToMailbox(
	SERVER_MAILBOX *	mailbox,
	SVRID				sendingService,
	MSG_STRUCT *		msg,
	NET_CMD				command )
{
	ASSERT_RETFALSE( mailbox );
	ASSERT_RETFALSE( msg );

	BOOL toRet = FALSE;
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		RAW_MSG_METADATA msgMetadata;
		msgMetadata.IsRequest = FALSE;
		msgMetadata.IsFake = TRUE;
		msgMetadata.ServiceProviderId = sendingService;
		msgMetadata.ServiceUser = server->m_specs->svrType;
		msgMetadata.ServiceProvider = ServerIdGetType( sendingService );

		toRet = RunnerNetPostMailboxMsg(
						mailbox,
						&msgMetadata,
						msg,
						command );

		ServerContextRelease();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetPostFakeClientRequestToMailbox(
	SERVER_MAILBOX * mailbox,
	SERVICEUSERID sendingClient,
	MSG_STRUCT * msg,
	NET_CMD command )
{
	ASSERT_RETFALSE( mailbox );
	ASSERT_RETFALSE( msg );

	BOOL toRet = FALSE;
	ACTIVE_SERVER * server = ServerContextGet();
	if( server )
	{
		RAW_MSG_METADATA msgMetadata;
		msgMetadata.IsRequest = TRUE;
		msgMetadata.IsFake = TRUE;
		msgMetadata.ServiceUserId = sendingClient;
		msgMetadata.ServiceUser = ServiceUserIdGetUserType( sendingClient );
		msgMetadata.ServiceProvider = server->m_specs->svrType;

		toRet = RunnerNetPostMailboxMsg(
						mailbox,
						&msgMetadata,
						msg,
						command );

		ServerContextRelease();
	}
	return toRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetTranslateMessageForSend(
	SVRTYPE provider,
	SVRTYPE user,
	BOOL bRequest,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	unsigned int & size,
	BYTE_BUF_NET & bbuf)
{
	return RunnerNetTranslateMessageForSend(provider, user, bRequest,
		cmd, msg, size, bbuf);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrNetTranslateMessageForRecv(
	SVRTYPE provider,
	SVRTYPE user,
	BOOL bRequest,
	BYTE * data,
	unsigned int size,
	MSG_STRUCT * msg)
{
	return RunnerNetTranslateMessageForRecv(provider, user, bRequest,
		data, size, msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NETCOM * SvrGetNetconForUserServer(
	void )
{
#if ISVERSION(DEVELOPMENT)
	ACTIVE_SERVER * server = ServerContextGet();
	ASSERT_RETNULL( server );
	SVRTYPE svrType = server->m_specs->svrType;
	ServerContextRelease();
	ASSERT_RETNULL( svrType == USER_SERVER );
#endif
	return RunnerNetHijackNetcom();
}


//----------------------------------------------------------------------------
// SERVER CLUSTER FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT16 SvrClusterGetServerCount(
	SVRTYPE serviceSvrType )
{
	ASSERT_RETZERO( g_runner );
	return (UINT16)NetMapGetServiceInstanceCount(
						g_runner->m_netMap,
						serviceSvrType );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrClusterSendXMLMessagingMessage(
	WCHAR * message )
{
	return SvrManagerSendXMLMessagingMessage(message);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrClusterSendXMLMessageToServerMonitorFromServer(
	WCHAR * bodyMsg )
{
	ASSERT_RETFALSE(bodyMsg && bodyMsg[0]);

	WCHAR msgBuff[2048];
	PStrPrintf(
		msgBuff,
		2048,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<server><type>%S</type><instance>%u</instance></server>"
				L"</source>"
				L"<dest>"
					L"<servermonitor><type>%S</type><instance>%u</instance></servermonitor>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"%s"
			L"</body>"
		L"</cmd>",
		ServerGetName(CurrentSvrGetType()),
		CurrentSvrGetInstance(),
		ServerGetName(CurrentSvrGetType()),
		CurrentSvrGetInstance(),
		bodyMsg);
	return SvrManagerSendXMLMessagingMessage(msgBuff);
}

//----------------------------------------------------------------------------
//	PRIVILAGED FUNCTIONS
//----------------------------------------------------------------------------
void ServerContextSet(
	SVRID serverId )
{
	ServerContextSet( RunnerLookupServerContext( serverId ) );
}
