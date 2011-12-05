//-----------------------------------------------------------------------------
// Prime v2.0 - critsect.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _CRITSECT_H_
#define _CRITSECT_H_


//-----------------------------------------------------------------------------
// DEFINES
//-----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
#define CRITSECT_DEBUG_REFCOUNT			1
#endif

#define CRITSECT_READERWRITER_DEBUG		0			// turns on debugging info for reader writer locks
#define CRITSECT_READERWRITER_ALERT		100			// threshold for triggering long lock warning


#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)


// ---------------------------------------------------------------------------
// INTERLOCKED WRAPPERS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// returns original value!
// ---------------------------------------------------------------------------
inline long AtomicAdd(
	volatile long & addend,
	long value)
{
	return InterlockedExchangeAdd(&addend, value);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline long AtomicIncrement(
	volatile long & addend)
{
	return InterlockedIncrement(&addend);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline long AtomicDecrement(
	volatile long & addend)
{
	return InterlockedDecrement(&addend);
}


// ---------------------------------------------------------------------------
// returns original value!
// ---------------------------------------------------------------------------
#ifdef _WIN64
inline INT64 AtomicAdd(
	volatile INT64 & addend,
	INT64 value)
{
	return InterlockedExchangeAdd64(&addend, value);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#ifdef _WIN64
inline INT64 AtomicIncrement(
	volatile INT64 & addend)
{
	return InterlockedIncrement64(&addend);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#ifdef _WIN64
inline INT64 AtomicDecrement(
	volatile INT64 & addend)
{
	return InterlockedDecrement64(&addend);
}
#endif


//----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef CS_DEBUG
#define InitCriticalSection(cs)					cs.InitDbg(__FILE__, __LINE__)
#else
#define InitCriticalSection(cs)					cs.Init();
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
extern unsigned int SPIN_WAIT_INTERVAL;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef CS_DEBUG
#define SpinWait(ctr, cnt, ms)								sSpinWait(ctr, cnt, ms)
#else
#define SpinWait(ctr, cnt, ms)								sSpinWait(ctr, cnt)
#endif

#ifdef CS_DEBUG
static void sSpinWait(
	ULONG & counter,
	ULONG count,
	ULONG & maxspin)
#else
static void sSpinWait(
	ULONG & counter,
	ULONG count)
#endif
{
	++counter;
	if ((counter % count) == 0)
	{
#ifdef CS_DEBUG
		if (counter > maxspin)
		{
			maxspin = counter;
		}
#endif
		Sleep(0);
	}
	return;
}


//----------------------------------------------------------------------------
// AUTO LOCK BASE CLASS
//----------------------------------------------------------------------------
template<class AutoLocker, class LockType>
class CAutoLock
{
private:
	LockType * m_lock;
public:
	CAutoLock<AutoLocker,LockType> & operator = (
		LockType * lock )
	{
		if( m_lock )
		{
			AutoLocker::Leave(m_lock);
		}
		m_lock = lock;
		if( m_lock )
		{
			AutoLocker::Enter(m_lock);
		}
		return *this;
	}
	CAutoLock(
		void )
	{
		m_lock = NULL;
	}
	CAutoLock(
		LockType * lock )
	{
		m_lock = NULL;
		*this = lock;
	}
	~CAutoLock(
		void )
	{
		*this = NULL;
	}
};

template<class AutoLocker, class LockType>
class CAutoLeave
{
private:
	LockType * m_lock;
public:
	CAutoLeave<AutoLocker,LockType> & operator = (
		LockType * lock )
	{
		if( m_lock )
		{
			AutoLocker::Leave(m_lock);
		}
		m_lock = lock;
		return *this;
	}
	CAutoLeave(
		void )
	{
		m_lock = NULL;
	}
	CAutoLeave(
		LockType * lock )
	{
		m_lock = NULL;
		*this = lock;
	}
	~CAutoLeave(
		void )
	{
		*this = NULL;
	}
};


//-----------------------------------------------------------------------------
// CLASSES
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// simple critical section wrapper
// todo: investigate if a local variable would optimize instead of always
//		 doing a EnterCriticalSection
//-----------------------------------------------------------------------------
class CCriticalSection
{
private:
    static const DWORD CS_SPIN_WAIT_COUNT = 8000;
	CRITICAL_SECTION	cs;
	BOOL bAutoFree;

public:
	void Init(void)				{ ASSERT(InitializeCriticalSectionAndSpinCount(&cs,CS_SPIN_WAIT_COUNT)); }
	void Free(void)				{ DeleteCriticalSection(&cs); }
	void Enter(void)			{ EnterCriticalSection(&cs); }
	void Leave(void)			{ LeaveCriticalSection(&cs); }
	BOOL TryEnter(void)			{ return TryEnterCriticalSection(&cs); }

	// this function is useful in debug builds to verify that a critical section was acquired by the caller of a fxn.
	// DBG_ASSERT(CS.IsOwnedByCurrentThread()), is a nice alternative to wantonly re-grabbing critical sections.
	BOOL IsOwnedByCurrentThread(void)
	{ 
		if (!TryEnter())
			return false;
		bool bResult = cs.RecursionCount > 1;
		Leave();
		return bResult;
	}

	CCriticalSection() {bAutoFree = FALSE;}
	CCriticalSection(BOOL bInit)
	{
		bAutoFree = bInit;
		if (bInit) {
			Init();
		}
	}
	~CCriticalSection()
	{
		if (bAutoFree) {
			Free();
		}
	}
};

class CCriticalSectionAutoLocker
{
public:
	static void Enter( __notnull CCriticalSection * lock )
	{
		lock->Enter();
	}
	static void Leave( __notnull CCriticalSection * lock )
	{
		lock->Leave();
	}
};

typedef CAutoLock<CCriticalSectionAutoLocker,CCriticalSection> CSAutoLock;
typedef CAutoLeave<CCriticalSectionAutoLocker,CCriticalSection> CSAutoLeave;

//-----------------------------------------------------------------------------
// simple, non-scalable, critical section
//-----------------------------------------------------------------------------
class CCritSectLite
{
private:
	static const LONG LOCK_FLAG =	0x1;
    static const DWORD CS_LITE_WARN_MS = 2000; // warn if CSLite held more than 2 seconds.

	volatile LONG					lock;

#ifdef CS_DEBUG
	DWORD							owningThread;
	ULONG							maxspin;
	const char *					file;
	unsigned int					line;
#endif

public:
	CCritSectLite(
		void) : lock(0)
	{
	}

	void Init(
		void)
	{
		lock = 0;
#ifdef CS_DEBUG
		file = NULL;
		line = 0;
		maxspin = 0;
#endif
	}

#ifdef CS_DEBUG
	void InitDbg(
		const char * _file,
		unsigned int _line)
	{
		Init();
		file = _file;
		line = _line;
	}
#endif

	void Free(
		void)
	{
		lock = 0;
#ifdef CS_DEBUG
		if (maxspin > SPIN_WAIT_INTERVAL)
		{
			trace("CCritSectLite [FILE:%s  LINE:%d]  maxspin: %8d\n", file, line, maxspin);
		}
#endif
	}

	void Enter(
		void)
	{
		DBG_ASSERT(!(lock & (~0x1)));
		ULONG spincounter = 0;
		while (InterlockedCompareExchange(&lock, LOCK_FLAG, 0) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
#ifdef CS_DEBUG
		owningThread = GetCurrentThreadId();
#endif
	}

	void Leave(
		void)
	{
		_WriteBarrier();
		lock = 0;
		_ReadWriteBarrier(); // The CS is released.
	}

	BOOL Try(
		void)
	{
		DBG_ASSERT(!(lock & (~0x1)));
		return (InterlockedCompareExchange(&lock, LOCK_FLAG, 0) == 0);
	}
};


class CCritSectLiteAutoLocker
{
public:
	static void Enter( __notnull CCritSectLite * lock )
	{
		lock->Enter();
	}
	static void Leave( __notnull CCritSectLite * lock )
	{
		lock->Leave();
	}
};
typedef CAutoLock<CCritSectLiteAutoLocker,CCritSectLite>	CSLAutoLock;


//-----------------------------------------------------------------------------
// simple, non-scalable, critical section (fair)
//-----------------------------------------------------------------------------
class CCritSectLiteFair
{
private:
	volatile LONG					ticket;
	volatile LONG					lock;

#ifdef CS_DEBUG
	DWORD							owningThread;
	ULONG							maxspin;
	const char *					file;
	unsigned int					line;
#endif

public:
	void Init(
		void)
	{
		ticket = 0;
		lock = 0;
#ifdef CS_DEBUG
		maxspin = 0;
#endif
	}

#ifdef CS_DEBUG
	void InitDbg(
		const char * _file,
		unsigned int _line)
	{
		Init();
		file = _file;
		line = _line;
	}
#endif

	void Free(
		void)
	{
		ticket = 0;
		lock = 0;
#ifdef CS_DEBUG
		if (maxspin > SPIN_WAIT_INTERVAL)
		{
			trace("CCritSectLite [FILE:%s  LINE:%d]  maxspin: %8d\n", file, line, maxspin);
		}
#endif
	}

	void Enter(
		void)
	{
		ULONG spincounter = 0;
		
		LONG me = InterlockedIncrement(&ticket) - 1;
		while (lock != me)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
#ifdef CS_DEBUG
		owningThread = GetCurrentThreadId();
#endif
	}

	void Leave(
		void)
	{
		InterlockedIncrement(&lock);
	}
};


class CCritSectLiteFairAutoLocker
{
public:
	static void Enter( __notnull CCritSectLiteFair * lock )
	{
		lock->Enter();
	}
	static void Leave( __notnull CCritSectLiteFair * lock )
	{
		lock->Leave();
	}
};
typedef CAutoLock<CCritSectLiteFairAutoLocker, CCritSectLiteFair>	CSLFAutoLock;


//-----------------------------------------------------------------------------
// simple, non-scalable, thread-safe refcount
//-----------------------------------------------------------------------------
#if CRITSECT_DEBUG_REFCOUNT
#define CRITSEC_FUNCNOARGS()									const char * file, unsigned int line
#define CRITSEC_FUNCARGS(...)									__VA_ARGS__, const char * file, unsigned int line
#define CRITSEC_FUNCNOARGS_FILELINE()							(const char *)__FILE__, (unsigned int)__LINE__
#define CRITSEC_FUNC_FILELINE(...)								__VA_ARGS__, (const char *)__FILE__, (unsigned int)__LINE__
#define CRITSEC_FUNCNOARGS_PASSFL()								(const char *)(file), (unsigned int)(line)
#define CRITSEC_FUNC_PASSFL(...)								__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define CRITSEC_FUNC_FLPARAM(file, line, ...)					__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#else
#define CRITSEC_FUNCNOARGS()										
#define CRITSEC_FUNCARGS(...)									__VA_ARGS__
#define CRITSEC_FUNCNOARGS_FILELINE()							
#define CRITSEC_FUNC_FILELINE(...)								__VA_ARGS__
#define CRITSEC_FUNCNOARGS_PASSFL()							
#define CRITSEC_FUNC_PASSFL(...)								__VA_ARGS__
#define CRITSEC_FUNC_FLPARAM(file, line, ...)					__VA_ARGS__
#endif

class CRefCount
{
static const unsigned int			cTrackerNodeCountMax = 32; // This will probably need to be increased

private:
	volatile LONG					refcount;

#if CRITSECT_DEBUG_REFCOUNT
	struct FileLineInfo
	{
		const char *				file;
		unsigned int				line;
		unsigned int				count;
	};
	CCritSectLite					cs;
	FileLineInfo					tracker[cTrackerNodeCountMax];
	unsigned int					trackersize;
#endif
#ifdef CS_DEBUG
	ULONG							maxspin;
    DWORD                           owningThread;
#endif

#if CRITSECT_DEBUG_REFCOUNT
	unsigned int Track(
		const char * file,
		unsigned int line)
	{
		for (unsigned int ii = 0; ii < trackersize; ++ii)
		{
			if (tracker[ii].file == file && tracker[ii].line == line)
			{
				return ++tracker[ii].count;
			}
		}
		
		ASSERT_RETVAL(trackersize < cTrackerNodeCountMax, 1); // If this assert get's hit, we need to increase cTrackerNodeCountMax
						
		tracker[trackersize].file = file;
		tracker[trackersize].line = line;
		tracker[trackersize].count = 1;
		trackersize++;
		return 1;
	}
#endif

public:
	void Init(
		void)
	{
		refcount = 1;
#if CRITSECT_DEBUG_REFCOUNT
		cs.Init();		
		trackersize = 0;
#endif
#ifdef CS_DEBUG
		maxspin = 0;
#endif
	}

	void Free(
		void)
	{
#if CRITSECT_DEBUG_REFCOUNT
		if (refcount != 0)
		{
			CLT_VERSION_ONLY(TraceDebugOnly("refcount debug: count = %u", refcount));
			for (unsigned int ii = 0; ii < trackersize; ii++)
			{
				char file[33];
				int len = PStrLen(tracker[ii].file);
				PStrCopy(file, tracker[ii].file + (len < 32 ? 0 : len - 31), 33);
				CLT_VERSION_ONLY(TraceDebugOnly("file: %32s   line: %5d   count: %d", file, tracker[ii].line, tracker[ii].count));
			}
		}
		trackersize = 0;
		cs.Free();
#endif
		refcount = 0;
	}

	LONG RefAdd(CRITSEC_FUNCNOARGS())
	{
#if CRITSECT_DEBUG_REFCOUNT
		{
			CSLAutoLock autolock(&cs);
			Track(CRITSEC_FUNCNOARGS_PASSFL());
			return (refcount += (refcount != 0));
		}
#else
		for (;;)
		{
			LONG prevCopy = refcount;
			if (prevCopy <= 0)
			{
				return 0;
			}
			if (InterlockedCompareExchange(&refcount, prevCopy + 1, prevCopy) == prevCopy)
			{
				return prevCopy + 1;
			}
		}
#endif			
	}

	LONG RefDec(CRITSEC_FUNCNOARGS())
	{
#if CRITSECT_DEBUG_REFCOUNT
		{
			CSLAutoLock autolock(&cs);
			Track(CRITSEC_FUNCNOARGS_PASSFL());
			return (refcount -= (refcount > 0));
		}
#else
		for (;;)
		{
			LONG prevCopy = refcount;
			if (prevCopy <= 0)
			{
				return 0;
			}
			if (InterlockedCompareExchange(&refcount, prevCopy - 1, prevCopy) == prevCopy)
			{
				return prevCopy - 1;
			}
		}
#endif			
	}

	// this is iffy, but is useful for debug
	LONG GetRef(
		void)
	{
		return refcount;
	}
};


//-----------------------------------------------------------------------------
// DESC:	a simple, non-scalable, lockable, thread-safe ref count
//-----------------------------------------------------------------------------
class CLockingRefCountLite
{
private:
	static const LONG					LOCK_FLAG = 0x1;
	static const LONG					LOCKED = 0;
	static const LONG					UNLOCKED = 1;

	volatile unsigned short				m_lock;
	volatile unsigned short				m_refCount;
	volatile LONG						m_syncLock;

#ifdef CS_DEBUG
	ULONG					maxspin;
#endif

	void LOCKSYNC(
		void)
	{
		ULONG spincounter = 0;
		while (InterlockedCompareExchange(&m_syncLock, LOCK_FLAG, 0) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}

	void UNLOCKSYNC(
		void)
	{
		_WriteBarrier();
		m_syncLock = 0;
		_ReadWriteBarrier(); // The CS is released.
	}

public:

	//-------------------------------------------------------------------------
	void Init(
		void )
	{
		m_lock = UNLOCKED;
		m_refCount = 0;
		m_syncLock = 0;
#ifdef CS_DEBUG
		maxspin = 0;
#endif
	}

	void Free(
		void )
	{
		m_lock = UNLOCKED;
		m_refCount = 0;
		m_syncLock = 0;
	}

	CLockingRefCountLite(
		void )
	{
		Init();
	}

	~CLockingRefCountLite(
		void )
	{
		Free();
	}

	//-------------------------------------------------------------------------
	// returns TRUE if the count was incremented, or FALSE if the 
	// ref count is locked and therefore could not increment.
	//-------------------------------------------------------------------------
	BOOL TryIncrement(
		void)
	{
		unsigned short locked;

		LOCKSYNC();
		locked = m_lock;
		m_refCount = m_refCount + locked;
		UNLOCKSYNC();

		return locked;
	}

	//-------------------------------------------------------------------------
	// will stall thread while ref is locked.
	//-------------------------------------------------------------------------
	void Increment(
		void )
	{
		ULONG spincounter = 0;
		while (!TryIncrement())
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}

	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	void Decrement(
		void )
	{
		LOCKSYNC();
		--m_refCount;
		UNLOCKSYNC();
	}

	//-------------------------------------------------------------------------
	// prevents increments. returns TRUE if the ref was already locked.
	//-------------------------------------------------------------------------
	BOOL Lock(
		void )
	{
		LONG locked;

		LOCKSYNC();
		locked = m_lock;
		m_lock = LOCKED;
		UNLOCKSYNC();

		return (locked == LOCKED);
	}


	//-------------------------------------------------------------------------
	// allows increments.
	//-------------------------------------------------------------------------
	BOOL Unlock(
		void)
	{
		LONG locked;

		LOCKSYNC();
		locked = m_lock;
		m_lock = UNLOCKED;
		UNLOCKSYNC();

		return (locked == LOCKED);
	}

	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	LONG RefCount(
		void )
	{
		LONG count;

		LOCKSYNC();
		count = m_refCount;
		UNLOCKSYNC();

		return count;
	}

	//-------------------------------------------------------------------------
	// stalls thread until ref count is 0.
	//-------------------------------------------------------------------------
	void WaitZeroRef(
		void)
	{
		ULONG spincounter = 0;
		while(RefCount() != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}
};


//-----------------------------------------------------------------------------
// simple, non-scalable, reader-writer lock (reader preference)
// doesn't support promotion
//-----------------------------------------------------------------------------
class CReaderWriterLock_NS_RP
{
private:
	static const LONG WRITER_ACTIVE_FLAG =		0x1;
	static const LONG READER_COUNT_INC =		0x2;

	volatile LONG			lock;
#ifdef CS_DEBUG
	ULONG					maxspin;
	DWORD					owningThread;
#endif

public:
	void Init(
		void)
	{
		lock = 0;
#ifdef CS_DEBUG
		maxspin = 0;
#endif
	}

	void Free(
		void)
	{
		lock = 0;
	}

	void ReadLock(
		void)
	{
		ULONG spincounter = 0;

		InterlockedExchangeAdd(&lock, READER_COUNT_INC);
		while ((lock & WRITER_ACTIVE_FLAG) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}

	void WriteLock(
		void)
	{
		ULONG spincounter = 0;

		while (InterlockedCompareExchange(&lock, WRITER_ACTIVE_FLAG, 0) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
#ifdef CS_DEBUG
		owningThread = GetCurrentThreadId();
#endif
	}

	void EndRead(
		void)
	{
		InterlockedExchangeAdd(&lock, -READER_COUNT_INC);
	}

	void EndWrite(
		void)
	{
		InterlockedExchangeAdd(&lock, -WRITER_ACTIVE_FLAG);
	}
};

class CReaderWriterLock_NS_RP_AutoReadLocker
{
public:
	static void Enter(
		__notnull CReaderWriterLock_NS_RP * lock)
	{
		lock->ReadLock();
	}
	static void Leave(
		__notnull CReaderWriterLock_NS_RP * lock)
	{
		lock->EndRead();
	}
};

class CReaderWriterLock_NS_RP_AutoWriteLocker
{
public:
	static void Enter(
		__notnull CReaderWriterLock_NS_RP * lock)
	{
		lock->WriteLock();
	}
	static void Leave(
		__notnull CReaderWriterLock_NS_RP * lock)
	{
		lock->EndWrite();
	}
};

typedef CAutoLock<CReaderWriterLock_NS_RP_AutoReadLocker, CReaderWriterLock_NS_RP>	CRW_RP_AutoReadLock;
typedef CAutoLock<CReaderWriterLock_NS_RP_AutoWriteLocker, CReaderWriterLock_NS_RP>	CRW_RP_AutoWriteLock;


//-----------------------------------------------------------------------------
// simple, non-scalable, reader-writer lock (writer preference)
//-----------------------------------------------------------------------------
class CReaderWriterLock_NS_WP
{
private:
	static const LONG WRITER_ACTIVE_FLAG =		0x1;
	static const LONG READER_COUNT_INC =		0x2;

	volatile LONG			lock;
	volatile LONG			write_requests;
	volatile LONG			write_completions;
#ifdef CS_DEBUG
	ULONG					maxspin;
	DWORD					owningThread;
#endif

public:
	void Init(
		void)
	{
		lock = 0;
		write_requests = 0;
		write_completions = 0;
	}

	void Free(
		void)
	{
		lock = 0;
		write_requests = 0;
		write_completions = 0;
	}

	void ReadLock(
		void)
	{
		ULONG spincounter = 0;

		while (write_requests != write_completions)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
		spincounter = 0;
		InterlockedExchangeAdd(&lock, READER_COUNT_INC);
		while ((lock & WRITER_ACTIVE_FLAG) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}

	void WriteLock(
		void)
	{
		ULONG spincounter = 0;

		LONG previous_writers = InterlockedIncrement(&write_requests) - 1;
		while (write_completions != previous_writers)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
		spincounter = 0;
		while (InterlockedCompareExchange(&lock, WRITER_ACTIVE_FLAG, 0) != 0)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
#ifdef CS_DEBUG
		owningThread = GetCurrentThreadId();
#endif
	}

	void EndRead(
		void)
	{
		InterlockedExchangeAdd(&lock, -READER_COUNT_INC);
	}

	void EndWrite(
		void)
	{
		InterlockedExchangeAdd(&lock, -WRITER_ACTIVE_FLAG);
		InterlockedIncrement(&write_completions);
	}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class LockType>
class CAutoLockReaderWriterReadLock
{
private:
	LockType *			m_lock;

public:
	CAutoLockReaderWriterReadLock<LockType> & operator = (
		LockType * lock)
	{
		if (m_lock)
		{
			m_lock->EndRead();
		}
		m_lock = lock;
		if (m_lock)
		{
			m_lock->ReadLock();
		}
		return *this;
	}

	CAutoLockReaderWriterReadLock(
		void) : m_lock(NULL) {}

	CAutoLockReaderWriterReadLock(
		LockType * lock)
	{
		m_lock = NULL;
		*this = lock;
	}

	~CAutoLockReaderWriterReadLock(
		void)
	{
		*this = NULL;
	}
};
typedef CAutoLockReaderWriterReadLock<CReaderWriterLock_NS_WP>	CWRAutoLockReader;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class LockType>
class CAutoLockReaderWriterWriteLock
{
private:
	LockType *			m_lock;

public:
	CAutoLockReaderWriterWriteLock<LockType> & operator = (
		LockType * lock)
	{
		if (m_lock)
		{
			m_lock->EndWrite();
		}
		m_lock = lock;
		if (m_lock)
		{
			m_lock->WriteLock();
		}
		return *this;
	}

	CAutoLockReaderWriterWriteLock(
		void) : m_lock(NULL) {}

	CAutoLockReaderWriterWriteLock(
		LockType * lock)
	{
		m_lock = NULL;
		*this = lock;
	}

	~CAutoLockReaderWriterWriteLock(
		void)
	{
		*this = NULL;
	}
};
typedef CAutoLockReaderWriterWriteLock<CReaderWriterLock_NS_WP>	CWRAutoLockWriter;


//-----------------------------------------------------------------------------
// simple, non-scalable, reader-writer lock (fair)
// everything is served in FIFO order except readers are allowed to acquire
// a read lock if there are no writers in queue
//-----------------------------------------------------------------------------
class CReaderWriterLock_NS_FAIR
{
private:
	static const LONG READER_COUNT_INC =		0x10000;
	static const LONG WRITER_COUNT_INC =		0x1;
	static const LONG WRITER_MASK =				0xffff;
	static const LONG WRITER_TOP_MASK =			0x8000;
	static const LONG READER_TOP_MASK =			0x80000000;

	CCriticalSection		cs;
	volatile LONG			requests;
	volatile LONG			completions;
	BOOL					bAutoFree;

#ifdef CS_DEBUG
	ULONG					maxspin;
	DWORD					owningThread;
#endif

	// ideally the following interlocked functions could get replaced by intrinsics?
	void InterlockedClearAdd(
		volatile LONG * ptr,
		LONG mask,
		LONG inc)
	{
		cs.Enter();
		*ptr &= ~mask;
		*ptr += inc;
		cs.Leave();
	}

	LONG InterlockedFetchClearAdd(
		volatile LONG * ptr,
		LONG mask,
		LONG inc)
	{
		LONG retval;
		cs.Enter();
		retval = *ptr;
		*ptr &= ~mask;
		*ptr += inc;
		cs.Leave();
		return retval;
	}

public:
	void Init(
		void)
	{
		cs.Init();
		requests = 0;
		completions = 0;
#ifdef CS_DEBUG
		maxspin = 0;
#endif
	}

	void Free(
		void)
	{
		// note if there are in-queue tasks, they will crash!
		cs.Free();
		requests = 0;
		completions = 0;
	}

	void ReadLock(
		void)
	{
		ULONG spincounter = 0;
		LONG prev_writers = InterlockedFetchClearAdd(&requests, READER_TOP_MASK, READER_COUNT_INC) & WRITER_MASK;
		while ((completions & WRITER_MASK) != prev_writers)
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
	}

	void WriteLock(
		void)
	{
		ULONG spincounter = 0;
		LONG prev_processes = InterlockedFetchClearAdd(&requests, WRITER_TOP_MASK, WRITER_COUNT_INC);
		while ((completions != prev_processes))
		{
			SpinWait(spincounter, SPIN_WAIT_INTERVAL, maxspin);
		}
#ifdef CS_DEBUG
		owningThread = GetCurrentThreadId();
#endif
	}

	void EndRead(
		void)
	{
		// clearing READER_TOP_MASK prevents overflow
		InterlockedClearAdd(&completions, READER_TOP_MASK, READER_COUNT_INC);
	}

	void EndWrite(
		void)
	{
		// clearing WRITER_TOP_MASK prevents overflow
		InterlockedClearAdd(&completions, WRITER_TOP_MASK, WRITER_COUNT_INC);
	}

	CReaderWriterLock_NS_FAIR() {bAutoFree = FALSE;}
	CReaderWriterLock_NS_FAIR(BOOL bInit)
	{
		bAutoFree = bInit;
		if (bInit) {
			Init();
		}
	}
	~CReaderWriterLock_NS_FAIR()
	{
		if (bAutoFree) {
			Free();
		}
	}

};


//----------------------------------------------------------------------------
//	read auto lock
class CReaderWriterLock_NS_FAIR_ReadAutoLocker
{
public:
	static void Enter( __notnull CReaderWriterLock_NS_FAIR * lock )
	{
		lock->ReadLock();
	}
	static void Leave( __notnull CReaderWriterLock_NS_FAIR * lock )
	{
		lock->EndRead();
	}
};
typedef CAutoLock<
			CReaderWriterLock_NS_FAIR_ReadAutoLocker,
			CReaderWriterLock_NS_FAIR>					RWFairReadAutoLock;

//----------------------------------------------------------------------------
//	write auto lock
class CReaderWriterLock_NS_FAIR_WriteAutoLocker
{
public:
	static void Enter( __notnull CReaderWriterLock_NS_FAIR * lock )
	{
		lock->WriteLock();
	}
	static void Leave( __notnull CReaderWriterLock_NS_FAIR * lock )
	{
		lock->EndWrite();
	}
};
typedef CAutoLock<
			CReaderWriterLock_NS_FAIR_WriteAutoLocker,
			CReaderWriterLock_NS_FAIR>					RWFairWriteAutoLock;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitCritSectSpinWaitInterval(
	void);


#endif // _CRITSECT_H_
