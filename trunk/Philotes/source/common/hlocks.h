//----------------------------------------------------------------------------
//	hlocks.h
//  Copyright 2006, Flagship Studios
//
//	Hierarchical thread lock library.
//	Designed to enforce an ordering and hierarchy to a programs locks.
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define DBG_CODE(x)		x
#define DBG_LOCKING		1
#else
#define DBG_CODE(x)
#define DBG_LOCKING		0
#endif
#define HLOCK_CP_NAME(typeName)		HLOCK_CP_##typeName
#define MAX_HLOCK_NAME				128
#define ALL_LOCKS_HLOCK_NAME


//----------------------------------------------------------------------------
// HIERARCHAL LOCK DEFINITION MACROS
//----------------------------------------------------------------------------
#if DBG_LOCKING

//	HLOCK system startup and shutdown functions
BOOL InitHlockSystem( struct MEMORYPOOL * );
void FreeHlockSystem( void );

//	debug lock definitions
#define DEF_HLOCK(name,baseType)					struct name : baseType##DBG { BOOL Init( void ) { ASSERT_RETFALSE( DbgBaseInit(#name) );
#define HLOCK_LOCK_RESTRICTION(forbiddenType)		ASSERT_RETFALSE( HLAddLockRestriction(#forbiddenType) );
#define HLOCK_LOCK_REQUIREMENT(requiredType)		ASSERT_RETFALSE( HLAddRequiredLock(#requiredType) );
#define HLOCK_ALLOW_REENTRANCE						ASSERT_RETFALSE( HLAllowReentrance() );
#define END_HLOCK									return TRUE; } };

//	debug code path marker
#define DEF_HLOCK_CP(name)							struct name : HLOCK_CP_BASE { name() { HLCPBaseInit();
#define HLOCK_CP_LOCK_RESTRICTION(forbiddenType)	ASSERT( HLCPAddLockRestriction(#forbiddenType) );
#define END_HLOCK_CP								HLCPSetBannedTypesContext(); } };

//	in-code code path marker declaration
#define MARK_HLOCK_CP(type)							type HLOCK_CP_NAME(type); HLOCK_CP_NAME(type) = HLOCK_CP_NAME(type);	//	assign to self to eliminate unref-var warning.

#else

//	dummy HLOCK system startup and shutdown functions
inline BOOL InitHlockSystem(struct MEMORYPOOL *)	{ return TRUE; }
inline void FreeHlockSystem(void)					{ }

//	retail lock definitions
#define DEF_HLOCK(name,baseType)					typedef baseType name;
#define HLOCK_LOCK_RESTRICTION(forbiddenType)
#define HLOCK_LOCK_REQUIREMENT(requiredType)
#define HLOCK_ALLOW_REENTRANCE
#define END_HLOCK

//	retail code path marker
#define DEF_HLOCK_CP(name)
#define HLOCK_CP_LOCK_RESTRICTION(forbiddenType)
#define END_HLOCK_CP

//	in-code code path marker declaration
#define MARK_HLOCK_CP(type)

#endif


//----------------------------------------------------------------------------
// LOCK TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct HLCriticalSection
{
private:
	static const DWORD SPIN_COUNT = 5000;

	CRITICAL_SECTION	cs;
	BOOL bAutoFree;

public:
	BOOL Init(void)
	{
		return InitializeCriticalSectionAndSpinCount(&cs,SPIN_COUNT);
	}
	void Free(void)				{ DeleteCriticalSection(&cs); }
	void Enter(void)			{ EnterCriticalSection(&cs); }
	void Leave(void)			{ LeaveCriticalSection(&cs); }
	BOOL TryEnter(void)			{ return TryEnterCriticalSection(&cs); }
};

//----------------------------------------------------------------------------
struct HLReaderWriter
{
private:
	HLCriticalSection	cs;
	HANDLE				dataLock;
	volatile LONG		readers;
	volatile DWORD		writerThreadId;
	DBG_CODE(volatile LONG readingReaders;)

	BOOL AquireDataLock(
		DBG_CODE(BOOL * wasContended) )
	{
		DBG_CODE(UNREFERENCED_PARAMETER(wasContended));

		if(!dataLock)
			return FALSE;

		DWORD result = WaitForSingleObject(dataLock,INFINITE);

		return (result == WAIT_OBJECT_0);
	}

public:
	BOOL Init(
		void)
	{
		readers = 0;
		DBG_CODE(readingReaders = 0;)
		writerThreadId = 0;

		ASSERT_RETFALSE(cs.Init());
		dataLock = CreateEvent(NULL,FALSE,TRUE,NULL);
		ASSERT_RETFALSE(dataLock);

		return TRUE;
	}
	void Free(
		void)
	{
		cs.Free();
		readers = 0;
		writerThreadId = 0;
		DBG_CODE(readingReaders = 0;)
		if(dataLock)
			CloseHandle(dataLock);
	}
	void ReadLock(
		DBG_CODE(BOOL * wasContended = NULL) )
	{
		DBG_CODE(UNREFERENCED_PARAMETER(wasContended));

		cs.Enter();

		DBG_ASSERT(readers >= 0);
		if(InterlockedIncrement(&readers) == 1)
		{
			ASSERT(AquireDataLock(DBG_CODE(wasContended)));
		}

		DBG_ASSERT(readingReaders >= 0);
		DBG_CODE(++readingReaders;)
		DBG_ASSERT(writerThreadId == 0);
		cs.Leave();
	}
	void EndRead(
		void)
	{
		cs.Enter();

		DBG_ASSERT(writerThreadId == 0);
		DBG_ASSERT(readingReaders > 0);
		DBG_CODE(--readingReaders);
		DBG_ASSERT(readers > 0);

		if(InterlockedDecrement(&readers) == 0)
		{
			DBG_ASSERT(dataLock);
			BOOL set = SetEvent(dataLock);
			DBG_ASSERT(set);
			UNREFERENCED_PARAMETER(set);
		}

		cs.Leave();
	}
	void WriteLock(
		DBG_CODE(BOOL * wasContended = NULL) )
	{
		ASSERT_RETURN(AquireDataLock(DBG_CODE(wasContended)));

		DBG_ASSERT(readingReaders == 0);
		DBG_ASSERT(writerThreadId == 0);
		writerThreadId = GetCurrentThreadId();
	}
	void EndWrite(
		void)
	{
		DBG_ASSERT(writerThreadId == GetCurrentThreadId());
		DBG_ASSERT(readingReaders == 0);

		writerThreadId = 0;

		ASSERT_RETURN(dataLock);
		SetEvent(dataLock);
	}
};


//----------------------------------------------------------------------------
// DEBUG INCLUDES
//----------------------------------------------------------------------------
#include "hlocks_priv.h"


//----------------------------------------------------------------------------
// AUTO LOCKS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct HLCSAutoLock
{
private:
#if DBG_LOCKING
	typedef HLCriticalSectionDBG	HLCSType;
#else
	typedef HLCriticalSection		HLCSType;
#endif

	HLCSType * lock;
public:
	const HLCSAutoLock & operator = (
		HLCSType * toLock)
	{
		if(lock)
		{
			lock->Leave();
		}
		lock = toLock;
		if(lock)
		{
			lock->Enter();
		}
		return this[0];
	}
	HLCSAutoLock()
	{
		lock = NULL;
	}
	HLCSAutoLock(HLCSType * toLock)
	{
		lock = NULL;
		this[0] = toLock;
	}
	~HLCSAutoLock()
	{
		ReleaseLock();
	}
	void ReleaseLock()
	{
		this[0] = NULL;
	}
};

//----------------------------------------------------------------------------
struct HLRWReadLock
{
private:
#if DBG_LOCKING
	typedef HLReaderWriterDBG	HLRWRType;
#else
	typedef HLReaderWriter		HLRWRType;
#endif

	HLRWRType * lock;
public:
	const HLRWReadLock & operator = (
		HLRWRType * toLock)
	{
		if(lock)
		{
			lock->EndRead();
		}
		lock = toLock;
		if(lock)
		{
			lock->ReadLock();
		}
		return this[0];
	}
	HLRWReadLock(HLRWRType * toLock)
	{
		lock = NULL;
		this[0] = toLock;
	}
	~HLRWReadLock()
	{
		this[0] = NULL;
	}
};

//----------------------------------------------------------------------------
struct HLRWWriteLock
{
private:
#if DBG_LOCKING
	typedef HLReaderWriterDBG	HLRWWType;
#else
	typedef HLReaderWriter		HLRWWType;
#endif

	HLRWWType * lock;
public:
	const HLRWWriteLock & operator = (
		HLRWWType * toLock)
	{
		if(lock)
		{
			lock->EndWrite();
		}
		lock = toLock;
		if(lock)
		{
			lock->WriteLock();
		}
		return this[0];
	}
	HLRWWriteLock(HLRWWType * toLock)
	{
		lock = NULL;
		this[0] = toLock;
	}
	~HLRWWriteLock()
	{
		this[0] = NULL;
	}
};
