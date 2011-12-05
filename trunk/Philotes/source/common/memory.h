//-----------------------------------------------------------------------------
// memory.h
//
// Copyright (C) 2003 Flagship Studios, All Rights Reserved.
//-----------------------------------------------------------------------------
#ifdef _MEMORY_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _MEMORY_H_


#include "memorymanager_i.h"

#if USE_MEMORY_MANAGER
#define MEMORYPOOL FSCommon::IMemoryAllocator 
#else
struct MEMORYPOOL;
#endif

extern MEMORYPOOL* g_StaticAllocator;
extern MEMORYPOOL* g_ScratchAllocator;

#if USE_MEMORY_MANAGER

extern FSCommon::IMemoryAllocator* g_DefaultAllocatorHEAP;

#else

// ---------------------------------------------------------------------------
// ENUMS
// ---------------------------------------------------------------------------
enum MEMORY_MANAGEMENT_TYPE
{
	MEMORY_CRTALLOCATOR,										// uses crt (malloc, free)
    MEMORY_LFHALLOCATOR,              							// win32 lfh (HeapAlloc)
	MEMORY_BINALLOCATOR_ST,										// bin allocator (not mt-safe)
	MEMORY_BINALLOCATOR_MT,										// bin allocator (mt-safe)
	MEMORY_STKALLOCATOR_ST,										// stack allocator (not mt-safe)
	MEMORY_STKALLOCATOR_MT,										// stack allocator (mt-safe)
	MEMORY_HEAPALLOCATOR_ST,
	MEMORY_HEAPALLOCATOR_MT,
	MEMORY_DSTALLOCATOR_ST,										// dynamic stack allocator (not mt-safe)
	MEMORY_DSTALLOCATOR_MT,										// dynamic stack allocator (mt-safe)

	NUM_MEMORY_SYSTEMS
};


//-----------------------------------------------------------------------------
// STRUCTS
//-----------------------------------------------------------------------------
struct MEMORYPOOL;

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
extern MEMORY_MANAGEMENT_TYPE DefaultMemoryAllocator;

#endif

#define FLIDX_WINDOW_SIZE				5


//-----------------------------------------------------------------------------
// STRUCTS
//-----------------------------------------------------------------------------
struct MEMORY_INFO_LINE
{
	const char *						m_csFile;
	unsigned int						m_nLine;
	unsigned int						m_nCurBytes;
	unsigned int						m_nCurAllocs;
	unsigned int						m_nRecentBytes;
	unsigned int						m_nRecentAllocs;
};

struct MEMORY_INFO
{
	// system
	DWORD								dwMemoryLoad;
	UINT64								ullTotalPhys;
	UINT64								ullAvailPhys;
	UINT64								ullTotalPageFile;
	UINT64								ullAvailPageFile;
	UINT64								ullTotalVirtual;
	UINT64								ullAvailVirtual;
	UINT64								ullAvailExtendedVirtual;

	// process
	DWORD								dwHandleCount;
	DWORD								dwProcessHeap;

	// prime
	unsigned int						m_nTotalTotalAllocated;
	unsigned int						m_nTotalAllocations;
	MEMORY_INFO_LINE					m_LineWindow[FLIDX_WINDOW_SIZE];
};

struct MEMORY_BIN_INFO
{
	unsigned int						BlockSize;
	unsigned int						UsedBlocks;
};

struct MEMORY_POOL_INFO
{
	MEMORYPOOL*							Pool;
	WCHAR								Name[32];
	unsigned int						SmallAllocations;
	unsigned int						SmallAllocationSize;
	unsigned int						LargeAllocations;
	unsigned int						LargeAllocationSize;
	unsigned int						InternalOverhead;
	unsigned int						InternalFragmentation;
	MEMORY_BIN_INFO						Bins[36];	
};

struct MEMORY_SYSTEM_INFO
{
	MEMORY_POOL_INFO					PoolInfo[10];
	unsigned int						PoolInfoCount;
};



// ---------------------------------------------------------------------------
// EXPORTED UTILITY FUNCTIONS
// ---------------------------------------------------------------------------
void memswap(
	void* a,
	void* b,
	int len);


// ---------------------------------------------------------------------------
// EXPORTED MEMORY SYSTEM FUNCTIONS
// ---------------------------------------------------------------------------
void MemorySystemInit(LPSTR pCmdLine,
	unsigned int INIT_NODE_COUNT = 0);

void MemorySystemFree(
	void);

#if !USE_MEMORY_MANAGER
MEMORYPOOL * MemoryPoolInit(
	WCHAR * szDesc = NULL,
	MEMORY_MANAGEMENT_TYPE pooltype = NUM_MEMORY_SYSTEMS);

void MemoryPoolFree(
	MEMORYPOOL * pool);
#endif

void * MemoryAlloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size));

void * MemoryAllocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size));

void * MemoryAllocBig(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size));

void * MemoryAlignedAlloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size,
	unsigned int alignment));

void * MemoryAlignedAllocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size,
	unsigned int alignment));

void MemoryFree(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr));

void * MemoryRealloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size));

void * MemoryReallocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size));

void * MemoryAlignedRealloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size,
	unsigned int alignment));

void * MemoryAlignedReallocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size,
	unsigned int alignment));

MEMORY_SIZE MemorySize(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr));

void MemoryInitThreadPool(
	void);

void MemoryFreeThreadPool(
	void);

void MemorySetThreadPool(
	MEMORYPOOL * pool);

MEMORYPOOL * MemoryGetThreadPool(
	void);

void MemoryTraceFLIDX(
	MEMORYPOOL * pool);

void MemoryTraceAllFLIDX(
	void);

void MemoryTraceALLOC(
	MEMORYPOOL * pool);

void MemoryTraceAllALLOC(
	void);

BOOL MemoryCheckAvail(
	void);

void MemoryGetMetrics(
	MEMORYPOOL * pool,
	struct MEMORY_INFO * info);
	
void MemoryDumpCRTStats(
	void);

void MemoryHeapDump(
	void);

void MemoryHeapCheck(
	void);

#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardSetFrontGuardFileLine(
	const char * file,
	unsigned int line);
#endif

#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardSetRearGuardFileLine(
	const char * file,
	unsigned int line);
#endif


// ---------------------------------------------------------------------------
// UNIT TESTING
// ---------------------------------------------------------------------------
void MemorySystemTest(
	void);


// ---------------------------------------------------------------------------
// NEW/DELETE FUNCTIONS
// ---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void * operator new(
	MEMORY_FUNCARGS(size_t size,
	MEMORYPOOL * pool))
{
	return MALLOCFL(pool, static_cast<unsigned int>(size), file, line);
}


//----------------------------------------------------------------------------
inline void operator delete(
	MEMORY_FUNCARGS(void * ptr, 
	MEMORYPOOL * pool))
{ 
	return FREEFL(pool, ptr, file, line); 
}


//----------------------------------------------------------------------------
template <typename T>
inline T * MallocNew(
	MEMORY_FUNCARGS(MEMORYPOOL * pool))
{
	return MEMORYPOOL_NEWFL(pool, file, line) T;
}


//----------------------------------------------------------------------------
struct NEW_ARRAY_HDR
{
	union 
	{
		unsigned int		count;
		BYTE				buf[16];
	};
};


//----------------------------------------------------------------------------
template <typename T>
inline T * MallocNewArray(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	unsigned int count))
{
	if (count == 0)
	{
		return NULL;
	}
	MEMORY_SIZE size = sizeof(T) * count + sizeof(NEW_ARRAY_HDR);
	void * ptr = MALLOCFL(pool, size, file, line);

	NEW_ARRAY_HDR * hdr = (NEW_ARRAY_HDR *)ptr;
	hdr->count = count;
	T * start = (T *)((BYTE *)ptr + sizeof(NEW_ARRAY_HDR));
	T * end = start + count;
	for (T * cur = start; cur < end; ++cur)
	{
		cur = new(cur) T;
	}
	return start;
}


//----------------------------------------------------------------------------
template<typename T>
inline void FreeDelete(
	MEMORY_FUNCARGS(MEMORYPOOL * pool, 
	const T * ptr))
{ 
	if (!ptr)
	{
		return; 
	}
	ptr->~T();
	FREEFL(pool, const_cast<T *>(ptr), file, line); 
}


//----------------------------------------------------------------------------
template<typename T>
inline void FreeDeleteArray(
	MEMORY_FUNCARGS(MEMORYPOOL * pool, 
	const T * ptr))
{ 
	if (!ptr)
	{
		return; 
	}
	void * trueptr = (void *)(((BYTE *)ptr) - sizeof(NEW_ARRAY_HDR));
	NEW_ARRAY_HDR * hdr = (NEW_ARRAY_HDR *)trueptr;
	unsigned int count = hdr->count;
	T * start = (T *)ptr;
	T * end = start + count;
	for (T * cur = end - 1; cur >= start; --cur)
	{
		cur->~T();
	}
	FREEFL(pool, trueptr, file, line); 
}


// ---------------------------------------------------------------------------
// INLINE FUNCTIONS
// ---------------------------------------------------------------------------
__checkReturn inline BOOL MemoryCopy(
	void * pDest,
	size_t nSizeInBytes,
	const void * pSource,
	size_t nBytesToCopy)
{
	return (memcpy_s(pDest, nSizeInBytes, pSource, nBytesToCopy) == 0);
}

__checkReturn inline BOOL MemoryMove(
	void * pDest,
	size_t nSizeInBytes,
	const void * pSource,
	size_t nBytesToCopy)
{
	return (memmove_s(pDest, nSizeInBytes, pSource, nBytesToCopy) == 0);
}


//----------------------------------------------------------------------------
// simple AUTOFREE class (doesn't handle nested destroy)
//----------------------------------------------------------------------------
struct AUTOFREE
{
	MEMORYPOOL *		m_pool;
	void *				m_ptr;
	AUTOFREE(MEMORYPOOL * pool, void * ptr) : m_pool(pool), m_ptr(ptr)
	{
	}
	~AUTOFREE()
	{
		if (m_ptr)
		{
			FREE(m_pool, m_ptr);
		}
	}
	void Clear(
		void)
	{
		m_pool = NULL;
		m_ptr = NULL;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> 
class AUTODESTRUCT
{
private:
	T *						ptr;
	void (* func)(T *);

public:
	AUTODESTRUCT(T * in_ptr, void(* in_func)(T *)) : ptr(in_ptr), func(in_func) 
	{

	}
	~AUTODESTRUCT(void)
	{
		if (ptr)
		{
			func(ptr);
		}
		ptr = NULL;
	}
	void Clear(
		void)
	{
		ptr = NULL;
	}
};


//----------------------------------------------------------------------------
// creates an array on the stack of set size, but if the desired count
// is greater than that size, allocates the array on the heap.
// auto-frees allocated memory
//----------------------------------------------------------------------------
template <typename T, unsigned int LOCAL_COUNT, BOOL ZERO>
struct LOCAL_OR_ALLOC_BUFFER
{
	MEMORYPOOL *	m_Pool;
	T				m_localBuffer[LOCAL_COUNT];
	T *				m_useBuffer;

	LOCAL_OR_ALLOC_BUFFER(
		MEMORYPOOL * pool, 
		unsigned int count) : m_Pool(pool)
	{
		if (count > LOCAL_COUNT)
		{
			if (ZERO)
			{
				m_useBuffer = (T *)MALLOCZ(m_Pool, count * sizeof(T));
			}
			else
			{
				m_useBuffer = (T *)MALLOC(m_Pool, count * sizeof(T));
			}
		}
		else
		{
			if (ZERO)
			{
				structclear(m_localBuffer);
			}
			m_useBuffer = m_localBuffer;
		}
	}

	~LOCAL_OR_ALLOC_BUFFER(
		void)
	{
		if (m_useBuffer != m_localBuffer)
		{
			FREE(m_Pool, m_useBuffer);
			m_useBuffer = NULL;
		}
	}

	T * GetBuf(
		void)
	{
		return m_useBuffer;
	}

	const T & operator[](
		unsigned int index) const
	{
		return m_useBuffer[index];
	}

	T & operator[](
		unsigned int index)
	{
		return m_useBuffer[index];
	}

	T & operator[](
		int index)
	{
		return m_useBuffer[index];
	}

	const T & operator[](
		int index) const
	{
		return m_useBuffer[index];
	}
};


#endif // _MEMORY_H_
