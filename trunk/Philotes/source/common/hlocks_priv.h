//----------------------------------------------------------------------------
//	hlocks_priv.h
//  Copyright 2006, Flagship Studios
//
//	Hierarchical thread lock library.
//	Designed to enforce an ordering and hierarchy to a programs locks.
//----------------------------------------------------------------------------

#pragma once

#if DBG_LOCKING


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _REFERENCELIST_H_
#include "reflist.h"
#endif
#ifndef _LIST_H_
#include "list.h"
#endif
#ifndef _STRINGHASHKEY_H_
#include "stringhashkey.h"
#endif


//----------------------------------------------------------------------------
// PERFORMANCE COUNTER DEFINES
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#include "watchdogclient_counter_definitions.h"
#include <PerfHelper.h>
#else
struct PERFCOUNTER_INSTANCE_HLOCK_LIB;
#ifndef _COMMON_H_
#include "common.h"
#endif // _COMMON_H_
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
extern struct MEMORYPOOL * g_HlockGlobalPool;


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef CStringHashKey<char,MAX_HLOCK_NAME>		LockNameType;
typedef PList<LockNameType>						LockNameListType;
typedef REFERENCE_LIST<LockNameType>			LockNameLookupType;


//----------------------------------------------------------------------------
// PER-THREAD TYPE
//----------------------------------------------------------------------------
struct HLOCK_THREAD_STORAGE
{
private:
	DWORD						m_noLocksAllowed;
	LockNameLookupType			m_bannedLockNames;
	LockNameLookupType			m_heldLockNames;
	PERF_INSTANCE(HLOCK_LIB) *	m_perfCtrs;

public:
	void Init(
		void )
	{
		m_noLocksAllowed = 0;
		m_bannedLockNames.Init();
		m_heldLockNames.Init();
		m_perfCtrs = NULL;

#if ISVERSION(SERVER_VERSION)
		//	init the chat performance counters
		WCHAR instanceName[MAX_PERF_INSTANCE_NAME];
		PStrPrintf(
			instanceName,
			MAX_PERF_INSTANCE_NAME,
			L"Thread ID: %u",
			GetCurrentThreadId() );
		m_perfCtrs = PERF_GET_INSTANCE(HLOCK_LIB,instanceName);
		ASSERT(m_perfCtrs);
#endif
	}
	void Free(
		void )
	{
		m_noLocksAllowed = 0;
		m_bannedLockNames.Destroy(g_HlockGlobalPool);
		m_heldLockNames.Destroy(g_HlockGlobalPool);
		SVR_VERSION_ONLY(PERF_FREE_INSTANCE(m_perfCtrs));
	}

	//------------------------------------------------------------------------
	// BANNED LOCK TYPE TRACKING
	//------------------------------------------------------------------------

	//------------------------------------------------------------------------
	BOOL GetNoLocksAllowed(
		void )
	{
		return (m_noLocksAllowed > 0);
	}
	void IncNoLocksCounter(
		void )
	{
		++m_noLocksAllowed;
	}
	void DecNoLocksCounter(
		void )
	{
		ASSERT(m_noLocksAllowed > 0);
		--m_noLocksAllowed;
	}

	//------------------------------------------------------------------------
	void AddBannedLocks(
		LockNameListType * list )
	{
		ASSERT_RETURN(list);
		LockNameListType::USER_NODE * itr = list->GetNext(NULL);
		while (itr)
		{
			m_bannedLockNames.Add(g_HlockGlobalPool,itr->Value);
			itr = list->GetNext(itr);
		}
	}
	void RemoveBannedLocks(
		LockNameListType * list )
	{
		ASSERT_RETURN(list);
		LockNameListType::USER_NODE * itr = list->GetNext(NULL);
		while (itr)
		{
			m_bannedLockNames.Remove(g_HlockGlobalPool,itr->Value);
			itr = list->GetNext(itr);
		}
	}
	BOOL ContainsBannedLock(
		LockNameType & lockName )
	{
		return (m_bannedLockNames.KeyReferences(lockName) > 0);
	}

	//------------------------------------------------------------------------
	void AddHeldLock(
		LockNameType & lockName )
	{
		m_heldLockNames.Add(g_HlockGlobalPool,lockName);
	}
	void RemoveHeldLock(
		LockNameType & lockName )
	{
		m_heldLockNames.Remove(g_HlockGlobalPool,lockName);
	}
	BOOL HasLockHeld(
		LockNameType & lockName )
	{
		return (m_heldLockNames.KeyReferences(lockName) > 0);
	}

	//------------------------------------------------------------------------
	// PERF COUNTER METHODS
	//------------------------------------------------------------------------
	void IncHeldLockCount(
		void )
	{
		PERF_ADD64(m_perfCtrs,HLOCK_LIB,LocksHeld,1);
		PERF_ADD64(m_perfCtrs,HLOCK_LIB,LockOps,1);
	}
	void DecHeldLockCount(
		void )
	{
		PERF_ADD64(m_perfCtrs,HLOCK_LIB,LocksHeld,-1);
		PERF_ADD64(m_perfCtrs,HLOCK_LIB,LockOps,1);
	}
	void AddContention(
		void )
	{
		PERF_ADD64(m_perfCtrs,HLOCK_LIB,LockContention,1);
	}

	//------------------------------------------------------------------------
	// ERROR CONDITION PRINTING
	//------------------------------------------------------------------------
	void PrintErrorContext(
		const char * attemptingTypeName,
		const char * missingPreReqName );
};


//----------------------------------------------------------------------------
// TLS METHODS
//----------------------------------------------------------------------------
HLOCK_THREAD_STORAGE * HlockGetTLS(
	void );


//----------------------------------------------------------------------------
// BASE TYPES
//----------------------------------------------------------------------------

struct HLOCK_BASE
{
protected:
	LockNameListType	m_requiredLockNames;
	LockNameListType	m_bannedLockNames;
	BOOL				m_bansAllLocks;
	LockNameType		m_thisLockType;

public:
	//------------------------------------------------------------------------
	BOOL HLBaseInit(
		const char * thisLockTypeName )
	{
		ASSERT_RETFALSE(thisLockTypeName);
		m_bannedLockNames.Init();
		m_requiredLockNames.Init();
		m_bansAllLocks = FALSE;
		m_thisLockType = thisLockTypeName;

		if(thisLockTypeName[0])
			return HLAddLockRestriction(thisLockTypeName);
		else
			return TRUE;
	}
	void HLBaseFree(
		void )
	{
		m_bannedLockNames.Destroy(g_HlockGlobalPool);
		m_requiredLockNames.Destroy(g_HlockGlobalPool);
	}

	//------------------------------------------------------------------------
	BOOL HLAddLockRestriction(
		const char * forbiddenLockName )
	{
		ASSERT_RETFALSE(forbiddenLockName);

		if(PStrCmp(forbiddenLockName,"ALL_LOCKS_HLOCK_NAME") == 0)
		{
			m_bansAllLocks = TRUE;
			return TRUE;
		}

		LockNameType toBan = forbiddenLockName;
		return m_bannedLockNames.PListPushHeadPool(g_HlockGlobalPool,toBan);
	}
	BOOL HLAddRequiredLock(
		const char * requiredLockName )
	{
		ASSERT_RETFALSE(requiredLockName);

		LockNameType toReq = requiredLockName;
		return m_requiredLockNames.PListPushHeadPool(g_HlockGlobalPool,toReq);
	}
	BOOL HLAllowReentrance(
		void )
	{
		LockNameType self = m_thisLockType;
		return m_bannedLockNames.FindAndDelete(self);
	}

	//------------------------------------------------------------------------
	BOOL HLTryEnterLock(
		void )
	{
		HLOCK_THREAD_STORAGE * threadData = HlockGetTLS();
		ASSERT_RETFALSE(threadData);

		if(threadData->GetNoLocksAllowed())
		{
			threadData->PrintErrorContext(&m_thisLockType[0],NULL);
			UNIGNORABLE_ASSERTX(!threadData->GetNoLocksAllowed(),
								"No locks are permitted under the current lock context." );
		}

		if(threadData->ContainsBannedLock(m_thisLockType))
		{
			threadData->PrintErrorContext(&m_thisLockType[0],NULL);
			UNIGNORABLE_ASSERTX(!threadData->ContainsBannedLock(m_thisLockType),
								"Lock type banned under the current lock context." );
		}

		LockNameListType::USER_NODE * itr = m_requiredLockNames.GetNext(NULL);
		while(itr)
		{
			if(!threadData->HasLockHeld(itr->Value))
			{
				threadData->PrintErrorContext(&m_thisLockType[0],&itr->Value[0]);
				UNIGNORABLE_ASSERTX(threadData->HasLockHeld(itr->Value),
									"Lock pre-requisite type not held." );
			}
			itr = m_requiredLockNames.GetNext(itr);
		}

		if(m_bansAllLocks)
			threadData->IncNoLocksCounter();
		threadData->AddBannedLocks(&m_bannedLockNames);
		threadData->IncHeldLockCount();
		threadData->AddHeldLock(m_thisLockType);

		return TRUE;
	}
	void HLLeaveLock(
		void )
	{
		HLOCK_THREAD_STORAGE * threadData = HlockGetTLS();
		ASSERT_RETURN(threadData);

		if(m_bansAllLocks)
			threadData->DecNoLocksCounter();
		threadData->RemoveBannedLocks(&m_bannedLockNames);
		threadData->DecHeldLockCount();
		threadData->RemoveHeldLock(m_thisLockType);
	}

	//------------------------------------------------------------------------
	void HLMarkLockContention(
		void )
	{
		HLOCK_THREAD_STORAGE * threadData = HlockGetTLS();
		ASSERT_RETURN(threadData);
		threadData->AddContention();
	}
};

struct HLOCK_CP_BASE : HLOCK_BASE
{
	//------------------------------------------------------------------------
	void HLCPBaseInit(
		void )
	{
		HLBaseInit("");
	}
	~HLOCK_CP_BASE()
	{
		HLOCK_THREAD_STORAGE * threadData = HlockGetTLS();
		ASSERT_RETURN(threadData);

		if(m_bansAllLocks)
			threadData->DecNoLocksCounter();
		threadData->RemoveBannedLocks(&m_bannedLockNames);
	}

	//------------------------------------------------------------------------
	BOOL HLCPAddLockRestriction(
		const char * forbiddenLockName )
	{
		return HLAddLockRestriction(forbiddenLockName);
	}

	//------------------------------------------------------------------------
	void HLCPSetBannedTypesContext(
		void )
	{
		HLOCK_THREAD_STORAGE * threadData = HlockGetTLS();
		ASSERT_RETURN(threadData);

		if(m_bansAllLocks)
			threadData->IncNoLocksCounter();
		threadData->AddBannedLocks(&m_bannedLockNames);
	}
};

//----------------------------------------------------------------------------
// LOCK SPECIFIC DEBUGGERS
//----------------------------------------------------------------------------

//	critical section debug lock
struct HLCriticalSectionDBG : HLOCK_BASE
{
private:
	HLCriticalSection m_critSect;

public:
	//------------------------------------------------------------------------
	BOOL DbgBaseInit(
		const char * thisLockName )
	{
		ASSERT_RETFALSE(HLBaseInit(thisLockName));
		return m_critSect.Init();
	}

	//------------------------------------------------------------------------
	void Free(
		void )
	{
		HLBaseFree();
		m_critSect.Free();
	}
	void Enter(
		void )
	{
		if(HLTryEnterLock())
		{
			if(!m_critSect.TryEnter())
			{
				HLMarkLockContention();
				m_critSect.Enter();
			}
		}
	}
	void Leave(
		void )
	{
		HLLeaveLock();
		m_critSect.Leave();
	}
	BOOL TryEnter(
		void )
	{
		if(HLTryEnterLock())
		{
			BOOL locked = m_critSect.TryEnter();
			if(!locked)
				HLLeaveLock();
			return locked;
		}
		return FALSE;
	}
};

// reader writer debug lock
struct HLReaderWriterDBG : HLOCK_BASE
{
private:
	HLReaderWriter m_lock;

public:
	//------------------------------------------------------------------------
	BOOL DbgBaseInit(
		const char * thisLockName )
	{
		ASSERT_RETFALSE(HLBaseInit(thisLockName));
		return m_lock.Init();
	}

	//------------------------------------------------------------------------
	void Free(
		void )
	{
		HLBaseFree();
		m_lock.Free();
	}
	void ReadLock(
		void )
	{
		BOOL wasContended = FALSE;

		if(HLTryEnterLock())
			m_lock.ReadLock(&wasContended);

		if(wasContended)
			HLMarkLockContention();
	}
	void EndRead(
		void )
	{
		HLLeaveLock();
		m_lock.EndRead();
	}
	void WriteLock(
		void )
	{
		BOOL wasContended = FALSE;

		if(HLTryEnterLock())
			m_lock.WriteLock(&wasContended);

		if(wasContended)
			HLMarkLockContention();
	}
	void EndWrite(
		void )
	{
		HLLeaveLock();
		m_lock.EndWrite();
	}
};


#endif	//	DBG_LOCKING
