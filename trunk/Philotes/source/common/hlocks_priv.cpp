//----------------------------------------------------------------------------
//	hlocks_priv.cpp
//  Copyright 2006, Flagship Studios
//
//	Hierarchical thread lock library.
//	Designed to enforce an ordering and hierarchy to a programs locks.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "hlocks.h"

#if DBG_LOCKING

#if ISVERSION(SERVER_VERSION)
#include "hlocks_priv.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
CCriticalSection				g_HlockGlobalLock;
DWORD							g_HlockTlsIndex = TLS_OUT_OF_INDEXES;
struct MEMORYPOOL *				g_HlockGlobalPool = NULL;
PList<HLOCK_THREAD_STORAGE*>	g_HlockThreadStorageStructs;


//----------------------------------------------------------------------------
// SYSTEM METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitHlockSystem(
	struct MEMORYPOOL * pool )
{
	g_HlockGlobalLock.Init();
	g_HlockTlsIndex = TlsAlloc();
	ASSERT_RETFALSE(g_HlockTlsIndex != TLS_OUT_OF_INDEXES);
	g_HlockGlobalPool = pool;
	g_HlockThreadStorageStructs.Init();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void FP_HLOCK_THREAD_STORAGE_DESTRUCTOR(
	struct MEMORYPOOL * pool,
	HLOCK_THREAD_STORAGE *& toFree )
{
	ASSERT_RETURN(toFree);
	toFree->Free();
	FREE(pool,toFree);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FreeHlockSystem(
	void )
{
	TlsFree(g_HlockTlsIndex);
	g_HlockTlsIndex = TLS_OUT_OF_INDEXES;

	g_HlockGlobalLock.Free();

	g_HlockThreadStorageStructs.Destroy(g_HlockGlobalPool,FP_HLOCK_THREAD_STORAGE_DESTRUCTOR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HLOCK_THREAD_STORAGE * HlockGetTLS(
	void )
{
	ASSERT_RETNULL(g_HlockTlsIndex != TLS_OUT_OF_INDEXES);
	HLOCK_THREAD_STORAGE * toRet = (HLOCK_THREAD_STORAGE*)TlsGetValue(g_HlockTlsIndex);
	if(toRet == NULL)
	{
		toRet = (HLOCK_THREAD_STORAGE*)MALLOCZ(g_HlockGlobalPool,sizeof(HLOCK_THREAD_STORAGE));
		ASSERT_RETNULL(toRet);
		toRet->Init();

		TlsSetValue(g_HlockTlsIndex,toRet);

		{
			CSAutoLock lock = &g_HlockGlobalLock;
			g_HlockThreadStorageStructs.PListPushTailPool(g_HlockGlobalPool,toRet);
		}
	}

	return toRet;
}


//------------------------------------------------------------------------
// THREAD STORAGE ERROR CONDITION PRINTING
//------------------------------------------------------------------------
void HLOCK_THREAD_STORAGE::PrintErrorContext(
	const char * attemptingTypeName,
	const char * missingPreReqName )
{
	TraceError( "HLOCK logic error!!" );
	TraceError( "Trying to lock type \"%s\".", attemptingTypeName );

	TraceError(
		"Disallowing all lock types: %s",
		(m_noLocksAllowed > 0) ? "TRUE" : "FALSE" );

	if(missingPreReqName && missingPreReqName[0])
	{
		TraceError(
			"Missing pre-requisite lock type: \"%s\"",
			missingPreReqName );
	}

	TraceError( "Currently held lock types:" );
	for( UINT ii = 0; ii < m_heldLockNames.KeyCount(); ++ii )
	{
		TraceError( "    %s", &m_heldLockNames[ii][0] );
	}

	TraceError( "Currently banned lock types:" );
	for( UINT ii = 0; ii < m_bannedLockNames.KeyCount(); ++ii )
	{
		TraceError( "    %s", &m_bannedLockNames[ii][0] );
	}
}


#endif	//	DBG_LOCKING
