//----------------------------------------------------------------------------
// ActiveServer.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ACTIVESERVER_H_
#define _ACTIVESERVER_H_

#if USE_MEMORY_MANAGER
#include "memoryallocator_crt.h"
#include "memoryallocator_pool.h"
#endif

//----------------------------------------------------------------------------
// lock types
//----------------------------------------------------------------------------
// note: this has to be a non-reentrant lock!!!
typedef CCritSectLite ActiveSvrThreadCreateLock;


//----------------------------------------------------------------------------
// DESC: the active server struct tracks all information and context data for
//			an active server running within the server runner process.
//----------------------------------------------------------------------------
struct ACTIVE_SERVER
{
	//	server information
	SERVER_SPECIFICATION *		m_specs;					// specs
	SVRINSTANCE					m_svrInstance;				// the instance id of this server
	SVRMACHINEINDEX				m_svrMachineIndex;			// the machine index of this server
	BOOL						m_wasShutdownRequested;		// for self terminating servers
	BOOL						m_bIsShuttingDown;			// disallows sends during shutdown

	//	memory pool
#if USE_MEMORY_MANAGER
	FSCommon::CMemoryAllocatorCRT<true>* m_svrAllocatorCRT;
	FSCommon::CMemoryAllocatorPOOL<true, true, 1024>* m_svrPool;
#else	
	struct MEMORYPOOL *			m_svrPool;					// memory pool for this server
#endif

	//	server data
	LPVOID						m_svrData;					// server specific global data
	const SVRCONFIG *			m_startupConfig;			// server config 

	//	thread data
	LPTHREAD_START_ROUTINE		m_threadCreateFunc;			// thread creation entry point
	LPVOID						m_threadCreateArg;			// thread creation arg
	ActiveSvrThreadCreateLock	m_threadCreateCS;			// thread creation lock

	SORTED_ARRAY<HANDLE,MAX_SVR_THREADS>
								m_threads;					// server thread handles
	CCritSectLite				m_threadsLock;				// server thread handles lock

	//	connections info
	NET_CONS *					m_netCons;					// network layer connections struct
};


//----------------------------------------------------------------------------
// ACTIVE SERVER INIT AND SHUTDOWN
//----------------------------------------------------------------------------
BOOL
	ActiveServerStartup(
		SERVER_SPECIFICATION *	spec,				// the server specification of the server to start		
#if USE_MEMORY_MANAGER
		FSCommon::CMemoryAllocatorCRT<true>* allocatorCRT,
		FSCommon::CMemoryAllocatorPOOL<true, true, 1024>* pool,
#else		
		struct MEMORYPOOL *		pool,				// the memory pool for this server to use
#endif				
		SVRINSTANCE				instance,			// the server type instance of this server
		SVRMACHINEINDEX			machineIndex,		// the machine index of this server
		const SVRCONFIG *		config );			// filled server config object

void
	ActiveServerShutdown(
		ACTIVE_SERVER_ENTRY *	toShutdown );


#endif	//	_ACTIVESERVER_H_

