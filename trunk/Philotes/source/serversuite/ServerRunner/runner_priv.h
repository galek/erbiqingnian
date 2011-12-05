//----------------------------------------------------------------------------
// runner_priv.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _RUNNER_PRIV_H_
#define _RUNNER_PRIV_H_

#if USE_MEMORY_MANAGER
#include "memoryallocator_crt.h"
#include "memoryallocator_pool.h"
#endif

//----------------------------------------------------------------------------
// ACTIVE SERVER CONTEXT HOLDER STRUCT
//----------------------------------------------------------------------------
struct ACTIVE_SERVER_ENTRY
{
	struct ACTIVE_SERVER *				Server;
	CLockingRefCountLite				ServerLock;
	SORTED_ARRAY<DWORD,MAX_SVR_THREADS> ServerThreadIds;
};


//----------------------------------------------------------------------------
// MAIN SERVER RUNNER SINGELTON STRUCT
//----------------------------------------------------------------------------
struct SERVER_RUNNER
{
	//	global active local servers array
	ACTIVE_SERVER_ENTRY			m_servers[ TOTAL_SERVER_COUNT ][ MAX_SVR_INSTANCES_PER_MACHINE ];
	struct MEMORYPOOL *			m_entryPool;
	
#if USE_MEMORY_MANAGER
	FSCommon::CMemoryAllocatorCRT<true>* m_netPoolCRT;
	FSCommon::CMemoryAllocatorPOOL<true, true, 1024>* m_netPool;
#else	
    MEMORYPOOL *                m_netPool;
#endif

	//	cluster network map
	NET_MAP *					m_netMap;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
extern SERVER_RUNNER *		  g_runner;


//----------------------------------------------------------------------------
// SERVER ENTRY HELPER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	NOTE: must follow every successfull call to get server with a call to release server
//----------------------------------------------------------------------------
inline ACTIVE_SERVER * RunnerGetServer(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMachineIndex )
{
	ASSERT_RETNULL( g_runner );

	if( svrType >= TOTAL_SERVER_COUNT ||
		svrMachineIndex >= MAX_SVR_INSTANCES_PER_MACHINE )
	{
		return NULL;
	}

	ACTIVE_SERVER_ENTRY * entry = &g_runner->m_servers[svrType][svrMachineIndex];
	if( entry->ServerLock.TryIncrement() )
	{
		if( !entry->Server )
		{
			entry->ServerLock.Decrement();
		}
		return entry->Server;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void RunnerReleaseServer(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMachineIndex )
{
	ASSERT_RETURN( g_runner );
	ASSERT_RETURN( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETURN( svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );

	g_runner->m_servers[svrType][svrMachineIndex].ServerLock.Decrement();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline ACTIVE_SERVER_ENTRY * RunnerGetServerEntry(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMachineIndex )
{
	ASSERT_RETNULL( g_runner );
	ASSERT_RETNULL( svrType < TOTAL_SERVER_COUNT );
	ASSERT_RETNULL( svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );

	if( g_runner->m_servers[svrType][svrMachineIndex].ServerLock.TryIncrement() )
	{
		if( g_runner->m_servers[ svrType ][ svrMachineIndex ].Server )
			return &g_runner->m_servers[svrType][svrMachineIndex];
		g_runner->m_servers[svrType][svrMachineIndex].ServerLock.Decrement();
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void RunnerReleaseServerEntry(
	ACTIVE_SERVER_ENTRY * toRelease )
{
	if( toRelease )
	{
		toRelease->ServerLock.Decrement();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline ACTIVE_SERVER_ENTRY * RunnerLookupServerContext(
	SVRID serverId )
{
	ASSERT_RETNULL( g_runner );
	ASSERT_RETNULL( g_runner->m_netMap );
	SVRMACHINEINDEX svrMachineIndex = NetMapGetServerMachineIndex( g_runner->m_netMap, serverId );
	if( svrMachineIndex == INVALID_SVRMACHINEINDEX )
		return NULL;
	ASSERT_RETNULL( svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE );
	return &g_runner->m_servers[ ServerIdGetType( serverId ) ][ svrMachineIndex ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL RunnerAddPrivilagedThreadId(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMachineIndex,
	DWORD			threadId )
{
	ASSERT_RETFALSE(g_runner);
	ASSERT_RETFALSE(svrType < TOTAL_SERVER_COUNT);
	ASSERT_RETFALSE(svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE);

	ACTIVE_SERVER_ENTRY * entry = &g_runner->m_servers[svrType][svrMachineIndex];
	return entry->ServerThreadIds.Insert( g_runner->m_entryPool, threadId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void RunnerRemovePrivilagedThreadId(
	SVRTYPE			svrType,
	SVRMACHINEINDEX	svrMachineIndex,
	DWORD			threadId )
{
	ASSERT_RETURN(g_runner);
	ASSERT_RETURN(svrType < TOTAL_SERVER_COUNT);
	ASSERT_RETURN(svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE);

	ACTIVE_SERVER_ENTRY * entry = &g_runner->m_servers[svrType][svrMachineIndex];
	entry->ServerThreadIds.Delete(MEMORY_FUNC_FILELINE(g_runner->m_entryPool, threadId));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void RunnerClearPrivilagedThreads(
	SVRTYPE			svrType,
	SVRMACHINEINDEX svrMachineIndex)
{
	ASSERT_RETURN(g_runner);
	ASSERT_RETURN(svrType < TOTAL_SERVER_COUNT);
	ASSERT_RETURN(svrMachineIndex < MAX_SVR_INSTANCES_PER_MACHINE);

	ACTIVE_SERVER_ENTRY * entry = &g_runner->m_servers[svrType][svrMachineIndex];
	entry->ServerThreadIds.Clear( g_runner->m_entryPool );
}


//----------------------------------------------------------------------------
// INTERNAL NET CALLBACKS
//----------------------------------------------------------------------------
LPVOID	RunnerAcceptClientConnection(
			SVRTYPE,
			SVRMACHINEINDEX,
			SERVICEUSERID ,
            __in __bcount(dwAcceptDataLen) BYTE* pAcceptData,
            DWORD dwAcceptDataLen,
			BOOL & acceptSuccess);

BOOL	RunnerProcessRequest(
			SVRTYPE,
			SVRMACHINEINDEX,
			SERVICEUSERID,
			LPVOID,
			NETMSG * );

void	RunnerProcessResponse(
			SVRTYPE,
			SVRMACHINEINDEX,
			SVRID,
			NETMSG * );

void	RunnerProcessClientDisconnect(
			SVRTYPE,
			SVRMACHINEINDEX,
			SERVICEUSERID,
			LPVOID );

struct NET_CONS * RunnerGetServerNetCons(
			SVRTYPE,
			SVRMACHINEINDEX );

void	RunnerReleaseServerNetCons(
			SVRTYPE,
			SVRMACHINEINDEX );

#endif	//	_RUNNER_PRIV_H_
