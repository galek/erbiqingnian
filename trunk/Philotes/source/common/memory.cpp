//-----------------------------------------------------------------------------
// memory.cpp
//
// Copyright (C) 2003 Flagship Studios, All Rights Reserved.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// INCLUDES
//-----------------------------------------------------------------------------

#include "appcommon.h"
#include "appcommontimer.h"
#include "globalindex.h"
#include "Tlhelp32.h"
#include "memoryallocator_crt.h"
#include "memoryallocator_heap.h"
#include "memoryallocator_pool.h"
#include "memoryallocator_stack.h"
#include "memoryallocator_wrapper.h"
#include "memorymanager_i.h"
#include "CrashHandler.h"

#if ISVERSION(SERVER_VERSION)
#include "memory.cpp.tmh"
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#pragma intrinsic(memcpy,memset)
#endif

using namespace FSCommon;

// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#if !USE_MEMORY_MANAGER
MEMORY_MANAGEMENT_TYPE DefaultMemoryAllocator = MEMORY_BINALLOCATOR_MT;
#endif

#if !defined(_WIN64)
#define MAX_ALLOCATION_SIZE										(64 * MEGA)		// assert if allocation is > 128MB
#else
#define MAX_ALLOCATION_SIZE										(unsigned)0xffffffff
#endif
#define MAX_ALIGNMENT_SUPPORTED									32


// ---------------------------------------------------------------------------
// DEBUG CONSTANTS
// ---------------------------------------------------------------------------
#if !USE_MEMORY_MANAGER
#define MEMORY_TEST_ENABLED										ISVERSION(DEBUG_VERSION)

#if DEBUG_MEMORY_ALLOCATIONS
#define	MEM_FILL_AFTER_VALLOC									0				// set to fill after virtuaalloc (forces system to page allocated memory)
#endif

#if DEBUG_MEMORY_ALLOCATIONS
#define	DEBUG_HEADER_MAGIC										0x4c4c4548
#define	DEBUG_FOOTER_MAGIC										0x6c6c6568
#endif

#if DEBUG_MEMORY_ALLOCATIONS
#define FLIDX_WINDOW_SIZE										5
#define FLIDX_WINDOW_TIME										(10 * MSECS_PER_SEC)	// the window time tracks allocations during the window
#endif
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//#define CHECK_OUT_OF_MEMORY(ptr, size)							if (!ptr) { HandleOutOfMemoryCondition(size); return NULL; }
// All of the code-paths in HandleOutOfMemoryCondition() do an _exit(0), so the "return NULL;" is unreachable code.
#define CHECK_OUT_OF_MEMORY(ptr, size)							if (!ptr && size) { HandleOutOfMemoryCondition(size); }

#if !USE_MEMORY_MANAGER
#if DEBUG_MEMORY_ALLOCATIONS
#define VPM_TRACE_0(fmt, ...)									// trace(fmt, __VA_ARGS__)
#define VPM_TRACE_1(fmt, ...)									// trace(fmt, __VA_ARGS__)
#define VPM_TRACE_2(fmt, ...)									PrintForWpp(fmt, __VA_ARGS__)
#define BIN_TRACE_0(fmt, ...)									// trace(fmt, __VA_ARGS__)
#define BIN_TRACE_1(fmt, ...)									// trace(fmt, __VA_ARGS__)
#define BIN_TRACE_2(fmt, ...)									PrintForWpp(fmt, __VA_ARGS__)
#define MST_TRACE(fmt, ...)										PrintForWpp(fmt, __VA_ARGS__)
#else
#define VPM_TRACE_0(fmt, ...)									
#define VPM_TRACE_1(fmt, ...)									
#define VPM_TRACE_2(fmt, ...)									
#define BIN_TRACE_0(fmt, ...)									
#define BIN_TRACE_1(fmt, ...)									
#define BIN_TRACE_2(fmt, ...)									
#define MST_TRACE(fmt, ...)										
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define VPM_GET_BLOCK(sz)										g_Vpm.GetBlock(sz)
#define VPM_RELEASE_BLOCK(p)									g_Vpm.RecycleBlock(p)

// ---------------------------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------------------------
#define		MEM_RESET_FREE_BLOCKS								1				// set to true to do a virtualalloc reset on unused memory
#define		DEFAULT_PAGE_SIZE									4096
#define		DEFAULT_ALLOCATION_GRANULARITY						65536

unsigned int MEM_PAGE_SIZE = DEFAULT_PAGE_SIZE;
unsigned int MEM_ALLOCATION_GRANULARITY = DEFAULT_ALLOCATION_GRANULARITY;


#endif



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
static void PrintForWpp(
	char * fmt,
	...)
{
	va_list args;
	va_start(args, fmt);

#if !ISVERSION(SERVER_VERSION)
    LogMessageV(PERF_LOG, fmt, args);
#else
    char output[256];
    PStrVprintf(output, ARRAYSIZE(output), fmt, args);
	TraceGameInfo("%s", output);
#endif
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void HandleOutOfMemoryCondition(
	DWORD size)
{
	REF(size);
	
#if !ISVERSION(SERVER_VERSION)

	// Only let one thread throw an out of memory exception.  Other threads
	// should suspend themselves indefinitely.  The first out-of-memory thread
	// will exit the process.
	//
	static long g_IsOutOfMemory = 0;
	if (InterlockedIncrement(&g_IsOutOfMemory) > 1)
	{
		Sleep(5000000);
	}
	
	// Throw an exception and let the process terminate
	//
	RaiseException(EXCEPTION_CODE_OUT_OF_MEMORY, 0, 0, 0);
	
#else // SERVER_VERSION
	UNREFERENCED_PARAMETER(size);
	TraceError("Out of memory. Dumping memory info.");
	MemoryTraceAllFLIDX();
#endif // SERVER_VERSION

}

#if !USE_MEMORY_MANAGER


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct VirtualPoolManager
{
protected:
	static const MEMORY_SIZE		BLOCK_GRANULARITY =			8192;										// difference in sizes between blocks
	static const MEMORY_SIZE		MAX_BLOCK_SIZE =			256 * 1024;									// maximum block size held by the vpm
	static const unsigned int		FREE_BLOCK_HASH_SIZE =		(MAX_BLOCK_SIZE / BLOCK_GRANULARITY);		// size of freehash (by size)
	static const unsigned int		ALLOC_BLOCK_HASH_SIZE =		16384;										// equals 64K on a 32bit machine, 128K on a 64bit machine

	// -----------------------------------------------------------------------
	static unsigned int GET_PAGES_PER_SIZE(
		MEMORY_SIZE size)
	{
		return ROUNDUP(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
	}

	struct BLOCK_NODE
	{
		BLOCK_NODE *				next;										// next in list
		void *						ptr;										// ptr to allocated memory
		MEMORY_SIZE					size;										// size of ptr
	};

	struct BLOCK_NODE_MANAGER
	{
	protected:
		CCritSectLite				cs;
		BLOCK_NODE *				blockbucketlist;							// list of BLOCK_NODE buckets
		BLOCK_NODE *				garbage;									// unused BLOCK_NODEs
		volatile MEM_COUNTER		pagesused;
		unsigned int				blockbucketfirst;							// first unused BLOCK_NODE in head bucket of allocblocklist
		unsigned int				BLOCK_NODE_BUCKET_SIZE;						// number of ALLOCATED_BLOCK containers per bucket

		// -------------------------------------------------------------------
		inline BLOCK_NODE * GetFromGarbage(
			void)
		{
			VPM_TRACE_0("VPM  Get Block Node From Garbage: %p\n", garbage);
			BLOCK_NODE * ptr = garbage;
			garbage = ptr->next;
			return ptr;
		}

		// -------------------------------------------------------------------
		inline BLOCK_NODE * GetFromFirstBucket(
			void)
		{
			ASSERT(blockbucketlist);
			VPM_TRACE_0("VPM  Get Block Node From First Bucket: %p\n", blockbucketlist + blockbucketfirst);
			BLOCK_NODE * ptr = blockbucketlist + blockbucketfirst;
			++blockbucketfirst;
			return ptr;
		}

		// -------------------------------------------------------------------
		inline BLOCK_NODE * AllocateNewBucket(
			void)
		{
			// allocate a new blocklist
			BLOCK_NODE * bucket = (BLOCK_NODE *)VirtualAlloc(NULL, BLOCK_NODE_BUCKET_SIZE * sizeof(BLOCK_NODE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			CHECK_OUT_OF_MEMORY(bucket, BLOCK_NODE_BUCKET_SIZE * sizeof(BLOCK_NODE));

			MEM_COUNTER pages = GET_PAGES_PER_SIZE(BLOCK_NODE_BUCKET_SIZE * sizeof(BLOCK_NODE));
			AtomicAdd(pagesused, pages);

			VPM_TRACE_1("VPM  VirtualAlloc Block Node Bucket: %p\n", bucket);
			bucket->next = blockbucketlist;		// use first node for next ptr (we don't use first node as a container)
			blockbucketlist = bucket;
			bucket = bucket + 1;				// return second node
			blockbucketfirst = 2;				// next usable node is third node
			return bucket;
		}

	public:
		// -------------------------------------------------------------------
		inline void Clear(
			void)
		{
			blockbucketlist = NULL;
			garbage = NULL;
			pagesused = 0;
			BLOCK_NODE_BUCKET_SIZE = (MEM_ALLOCATION_GRANULARITY / sizeof(BLOCK_NODE));
			blockbucketfirst = BLOCK_NODE_BUCKET_SIZE;
			InitCriticalSection(cs);
		}

		// -------------------------------------------------------------------
		inline void Init(
			void)
		{
			Clear();
		}

		// -------------------------------------------------------------------
		// get an unused BLOCK_NODE
		inline BLOCK_NODE * GetNew(
			void)
		{
			CSLAutoLock autolock(&cs);
			// grab node from garbage
			if (garbage)
			{
				return GetFromGarbage();
			}
			// grab container node from blocklist
			if (blockbucketfirst < BLOCK_NODE_BUCKET_SIZE)
			{
				return GetFromFirstBucket();
			}

			return AllocateNewBucket();
		}

		// -------------------------------------------------------------------
		// recycle a BLOCK_NODE
		inline void Recycle(
			BLOCK_NODE * node)
		{
			ASSERT_RETURN(node);
			VPM_TRACE_0("VPM  Recycle Block Node: %p\n", node);

			CSLAutoLock autolock(&cs);
			node->next = garbage;
			garbage = node;
		}

		// -------------------------------------------------------------------
		// free allocated block nodes
		inline void Free(
			void)
		{
			while (blockbucketlist)
			{
				BLOCK_NODE * next = blockbucketlist->next;
				ASSERT(VirtualFree(blockbucketlist, 0, MEM_RELEASE));
				MEM_COUNTER pages = GET_PAGES_PER_SIZE(BLOCK_NODE_BUCKET_SIZE * sizeof(BLOCK_NODE));
				AtomicAdd(pagesused, -pages);
				VPM_TRACE_0("VPM  VirtualAlloc Free Block:  Pointer:%p  Size:%d\n", blockbucketlist, MEM_PAGE_SIZE);
				blockbucketlist = next;
			}
			Clear();
		}
	}								blocknodes;

	struct FREE_HASH
	{
	protected:
		struct FREE_HASH_NODE
		{
			CCritSectLite			cs;
			BLOCK_NODE *			head;
			unsigned int			count;
		};
		FREE_HASH_NODE				hash[FREE_BLOCK_HASH_SIZE];

	public:
		// -------------------------------------------------------------------
		static unsigned int GetHashSize(
			void)
		{
			return FREE_BLOCK_HASH_SIZE;
		}

		// -------------------------------------------------------------------
		// return freehash index from block size
		static unsigned int GetBlockIndexFromSize(
			MEMORY_SIZE size)
		{
			if (size > 4096)
			{
				unsigned int index = (size - 1) / (MEMORY_SIZE)BLOCK_GRANULARITY + 1;
				if (index >= GetHashSize() && size <= MAX_BLOCK_SIZE)
				{
					return GetHashSize() - 1;
				}
				return index;
			}
			return 0;
		}

		// -------------------------------------------------------------------
		// return block size from freehash index
		static MEMORY_SIZE GetBlockSizeFromIndex(
			unsigned int index)
		{
			if (index > 0 && index < GetHashSize() - 1)
			{
				return index * BLOCK_GRANULARITY;
			}
			if (index == 0)
			{
				return 4096;
			}
			return MAX_BLOCK_SIZE;
		}

		// -------------------------------------------------------------------
		void Clear(
			void)
		{
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)
			{
				hash[ii].head = NULL;
				hash[ii].count = 0;
				InitCriticalSection(hash[ii].cs);
			}
		}

		// -------------------------------------------------------------------
		void Init(
			void)
		{
			Clear();
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)
			{
				ASSERT(NULL == hash[ii].head);
				ASSERT(0 == hash[ii].count);
				hash[ii].cs.Free();
			}
		}

		// -------------------------------------------------------------------
		inline void Add(
			unsigned int index,
			BLOCK_NODE * node)
		{
			CSLAutoLock autolock(&hash[index].cs);
			node->next = hash[index].head;
			hash[index].head = node;
			hash[index].count++;
		}

		// -------------------------------------------------------------------
		inline BLOCK_NODE * Pop(
			unsigned int index)
		{
			CSLAutoLock autolock(&hash[index].cs);
			BLOCK_NODE * block = hash[index].head;
			if (!block)
			{
				return NULL;
			}
			hash[index].head = block->next;
			hash[index].count--;
			return block;
		}

		// -------------------------------------------------------------------
		void MemoryTrace(
			void)
		{
#if DEBUG_MEMORY_ALLOCATIONS
			PrintForWpp("    FREE BLOCKS:");
			PrintForWpp("    %3s  %8s  %5s  %10s", "IDX", "SIZE", "COUNT", "TOTAL");
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)	
			{
				if (hash[ii].count <= 0)
				{
					continue;
				}
				MEMORY_SIZE blocksize = GetBlockSizeFromIndex(ii);
				REF(blocksize);
				CSLAutoLock autolock(&hash[ii].cs);
				PrintForWpp("    %3d  %8llu  %5d  %10llu", ii, (UINT64)blocksize, hash[ii].count, (UINT64)hash[ii].count * (UINT64)blocksize);
			}
#endif
		}
	}								freeblocks;

	struct ALLOC_HASH
	{
	protected:
		struct ALLOC_HASH_NODE
		{
			CCritSectLite			cs;
			BLOCK_NODE *			head;				
#if ISVERSION(DEBUG_VERSION)
			MEM_COUNTER				depth;										// depth of hash
#endif
		};
		ALLOC_HASH_NODE				hash[ALLOC_BLOCK_HASH_SIZE];				// hash of allocated blocks

	public:
		// -------------------------------------------------------------------
		static unsigned int GetHashSize(
			void)
		{
			return ALLOC_BLOCK_HASH_SIZE;
		}

		// -------------------------------------------------------------------
		void Clear(
			void)
		{
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)
			{
				hash[ii].head = NULL;
				InitCriticalSection(hash[ii].cs);
#if ISVERSION(DEBUG_VERSION)
				hash[ii].depth = 0;
#endif
			}
		}

		// -------------------------------------------------------------------
		void Init(
			void)
		{
			Clear();
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)
			{
				ASSERT(NULL == hash[ii].head);
				hash[ii].cs.Free();
			}
		}

		// -------------------------------------------------------------------
		inline void Add(
			unsigned int index,
			BLOCK_NODE * node)
		{
			CSLAutoLock autolock(&hash[index].cs);
			node->next = hash[index].head;
			hash[index].head = node;

#if ISVERSION(DEBUG_VERSION)
			++hash[index].depth;
#endif
		}

		// -------------------------------------------------------------------
		inline BLOCK_NODE * Pop(
			unsigned int index)
		{
			CSLAutoLock autolock(&hash[index].cs);
			BLOCK_NODE * block = hash[index].head;
			if (!block)
			{
				return NULL;
			}
			hash[index].head = block->next;
#if ISVERSION(DEBUG_VERSION)
			--hash[index].depth;
#endif
			return block;
		}

		// -------------------------------------------------------------------
		inline BLOCK_NODE * FindRemove(
			unsigned int index,
			void * ptr)
		{
			BLOCK_NODE * prev = NULL;
			CSLAutoLock autolock(&hash[index].cs);
			BLOCK_NODE * next = hash[index].head;
			while (next)
			{
				if (next->ptr == ptr)
				{
					if (prev)
					{
						prev->next = next->next;
					}
					else
					{
						hash[index].head = next->next;
					}
#if ISVERSION(DEBUG_VERSION)
					--hash[index].depth;
#endif
					VPM_TRACE_0("VPM  Remove Allocated Node: Pointer:%p  Allocated Node:%p  Size:%d\n", ptr, next, next->size);
					return next;
				}
				prev = next;
				next = next->next;
			}
			return NULL;
		}

		void MemoryTrace(
			void)
		{
#if DEBUG_MEMORY_ALLOCATIONS
			// first aggregate all the blocks < MAX_BLOCK_SIZE
			unsigned int count[FREE_BLOCK_HASH_SIZE];
			memclear(count, sizeof(count));

			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)	
			{
				CSLAutoLock autolock(&hash[ii].cs);
				BLOCK_NODE * curr = NULL;
				BLOCK_NODE * next = hash[ii].head;
				while (NULL != (curr = next))
				{
					next = curr->next;
					unsigned int index = FREE_HASH::GetBlockIndexFromSize(curr->size);
					if (index < arrsize(count))
					{
						count[index]++;
					}
				}
			}

			// then trace them
			PrintForWpp("    USED BLOCKS:");
			PrintForWpp("    %3s  %8s  %5s  %10s", "IDX", "SIZE", "COUNT", "TOTAL");
			for (unsigned int ii = 0; ii < arrsize(count); ++ii)	
			{
				if (count[ii] <= 0)
				{
					continue;
				}
				MEMORY_SIZE blocksize = FREE_HASH::GetBlockSizeFromIndex(ii);
				REF( blocksize );
				PrintForWpp("    %3d  %8llu  %5d  %10llu", ii, (UINT64)blocksize, count[ii], (UINT64)count[ii] * (UINT64)blocksize);
			}
			
			// trace large blocks
			for (unsigned int ii = 0; ii < arrsize(hash); ++ii)	
			{
				CSLAutoLock autolock(&hash[ii].cs);
				BLOCK_NODE * curr = NULL;
				BLOCK_NODE * next = hash[ii].head;
				while (NULL != (curr = next))
				{
					next = curr->next;
					unsigned int index = FREE_HASH::GetBlockIndexFromSize(curr->size);
					if (index < arrsize(count))
					{
						continue;
					}
					PrintForWpp("    %3s  %8llu  %5d  %10llu", "BIG", (UINT64)curr->size, 1, (UINT64)curr->size);
				}
			}
#endif
		}
	}								allochash;

	volatile MEM_COUNTER			pagesused;									// pages allocated
	volatile MEM_COUNTER			pagesfree;									// pages in free blocks
	volatile MEM_COUNTER			count;										// total VirtualAlloc blocks we've allocated

	// -----------------------------------------------------------------------
	void InitPageSizeAndAllocationGranularityGlobals(
		void) const
	{
		SYSTEM_INFO sysinfo;
		structclear(sysinfo);
		GetSystemInfo(&sysinfo);

		ASSERT(sysinfo.dwPageSize == DEFAULT_PAGE_SIZE);
		if (sysinfo.dwPageSize && IsPowerOf2(sysinfo.dwPageSize))
		{
			MEM_PAGE_SIZE = sysinfo.dwPageSize;	
		}

		ASSERT(sysinfo.dwAllocationGranularity == DEFAULT_ALLOCATION_GRANULARITY);
		if (sysinfo.dwAllocationGranularity && IsPowerOf2(sysinfo.dwAllocationGranularity))
		{
			MEM_ALLOCATION_GRANULARITY = sysinfo.dwAllocationGranularity;	
		}
	}

	// -----------------------------------------------------------------------
	// free a large block
	inline void FreeLargeBlock(
		BLOCK_NODE * block)
	{
		ASSERT_RETURN(block);
		ASSERT_RETURN(block->ptr);
		free(block->ptr);
		AtomicDecrement(count);
		MEM_COUNTER pages = GET_PAGES_PER_SIZE(block->size);
		AtomicAdd(pagesused, -pages);
		VPM_TRACE_1("VPM  Free Large Block:  Pointer:%p  Size:%d  Count:%I64d  Mem Used:%I64d  Total Mem:%I64d\n", block->ptr, block->size, (INT64)count, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		blocknodes.Recycle(block);
	}

	// -----------------------------------------------------------------------
	// add a block to the freehash
	inline void AddFreeBlock(
		BLOCK_NODE * block)
	{
		ASSERT_RETURN(block);
		unsigned int index = freeblocks.GetBlockIndexFromSize(block->size);

		if (index >= freeblocks.GetHashSize())
		{
			FreeLargeBlock(block);
			return;
		}

		void * ptr = block->ptr;
		MEMORY_SIZE size = block->size;

		freeblocks.Add(index, block);

		MEM_COUNTER pages = size / MEM_PAGE_SIZE;
		AtomicAdd(pagesused, -pages);
		AtomicAdd(pagesfree, pages);

#if MEM_RESET_FREE_BLOCKS
		ASSERT(VirtualAlloc(ptr, size, MEM_RESET, PAGE_READWRITE));
#endif

		VPM_TRACE_0("VPM  Add Free Block:  Pointer:%p  Size:%d  Index:%d\n", ptr, size, index);
	}

	// -----------------------------------------------------------------------
	// return index in allochash from ptr to memory block
	inline unsigned int GetAllocatedBlockIndexFromAddress(
		void * ptr) const
	{
		size_t diff = PTR_TO_SCALAR(ptr) / MEM_PAGE_SIZE;
		return RANDOMIZE(diff) % ALLOC_BLOCK_HASH_SIZE;
	}

	// -----------------------------------------------------------------------
	inline void AddAllocatedBlock(
		BLOCK_NODE * node,
		void * ptr,
		MEMORY_SIZE size)
	{
		node->ptr = ptr;
		node->size = size;

		unsigned int index = GetAllocatedBlockIndexFromAddress(ptr);

		allochash.Add(index, node);

		VPM_TRACE_0("VPM  Track Allocated Block: Pointer:%p  Allocated Node:%p  Size:%d\n", ptr, node, size);
	}

	// -----------------------------------------------------------------------
	// find & unlink an allocated block
	inline BLOCK_NODE * FindRemoveAllocatedBlock(
		void * ptr)
	{
		unsigned int index = GetAllocatedBlockIndexFromAddress(ptr);
		BLOCK_NODE * node = allochash.FindRemove(index, ptr);
		ASSERT_RETNULL(node);
		return node;
	}

	// -----------------------------------------------------------------------
	// remove an allocated block
	inline BOOL RemoveAllocatedBlock(
		void * ptr)
	{
		BLOCK_NODE * node = FindRemoveAllocatedBlock(ptr);
		ASSERT_RETFALSE(node);

		AddFreeBlock(node);

		return TRUE;
	}

	// -----------------------------------------------------------------------
	inline void FreeFreeBlock(
		BLOCK_NODE * node,
		MEM_COUNTER & freed)
	{
		ASSERT_RETURN(node);
		MEMORY_SIZE blocksize = node->size;
		ASSERT(node->ptr);
		if (PTR_TO_SCALAR(node->ptr) % MEM_ALLOCATION_GRANULARITY == 0)
		{
			ASSERT(VirtualFree(node->ptr, 0, MEM_RELEASE));
			++freed;
		}
		MEM_COUNTER pages = blocksize / MEM_PAGE_SIZE;
		AtomicAdd(pagesfree, -pages);
		VPM_TRACE_0("VPM  VirtualFree Block:  Pointer:%p  Size:%d  Count:%d  Mem Used:%I64d  Total Mem:%I64d\n", node->ptr, blocksize, freed, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
	}

	// -----------------------------------------------------------------------
	// free all the free blocks
	void FreeFreeBlocks(
		MEM_COUNTER & freed)
	{
		for (unsigned int ii = 0; ii < freeblocks.GetHashSize(); ++ii)
		{
			BLOCK_NODE * node;
			while ((node = freeblocks.Pop(ii)) != NULL)
			{
				FreeFreeBlock(node, freed);
			}
		}
		freeblocks.Free();
	}

	// -----------------------------------------------------------------------
	inline void FreeAllocatedBlock(
		BLOCK_NODE * node,
		MEM_COUNTER & freed)
	{
		ASSERT_RETURN(node);
		ASSERT(node->ptr);

		unsigned int index = freeblocks.GetBlockIndexFromSize(node->size);

		if (index < freeblocks.GetHashSize())
		{
			MEMORY_SIZE blocksize = node->size;
			if (PTR_TO_SCALAR(node->ptr) % MEM_ALLOCATION_GRANULARITY == 0)
			{
				ASSERT(VirtualFree(node->ptr, 0, MEM_RELEASE));
				++freed;
			}
			MEM_COUNTER pages = blocksize / MEM_PAGE_SIZE;
			AtomicAdd(pagesused, -pages);
			VPM_TRACE_0("VPM  VirtualFree Block:  Pointer:%p  Size:%d  Count:%d  Mem Used:%I64d  Total Mem:%I64d\n", node->ptr, blocksize, freed, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		}
		else
		{
			MEMORY_SIZE size = node->size;
			free(node->ptr);
			MEM_COUNTER pages = GET_PAGES_PER_SIZE(size);
			AtomicAdd(pagesused, -pages);
			++freed;
			VPM_TRACE_0("VPM  Free Large Block:  Pointer:%p  Size:%d  Count:%d  Mem Used:%I64d  Total Mem:%I64d\n", node->ptr, size, count, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		}
	}

	// -----------------------------------------------------------------------
	// free all the allocated blocks
	void FreeAllocatedBlocks(
		MEM_COUNTER & freed)
	{
		// free allocated block hash
		for (unsigned int ii = 0; ii < ALLOC_BLOCK_HASH_SIZE; ++ii)
		{
			BLOCK_NODE * node;
			while ((node = allochash.Pop(ii)) != NULL)
			{
				FreeAllocatedBlock(node, freed);
			}
		}
		allochash.Free();
	}

	// -----------------------------------------------------------------------
	void Clear(
		void)
	{
		freeblocks.Clear();
		allochash.Clear();
		blocknodes.Clear();
		count = 0;
		pagesused = 0;
		pagesfree = 0;
	}

	// ---------------------------------------------------------------------------
	inline void * GetBlockFromFreeHash(
		unsigned int index)
	{
		BLOCK_NODE * node = freeblocks.Pop(index);
		if (!node)
		{
			return NULL;
		}

#if MEM_RESET_FREE_BLOCKS
		ASSERT(VirtualAlloc(node->ptr, node->size, MEM_COMMIT, PAGE_READWRITE));
#endif

		MEMORY_SIZE size = node->size;
		MEM_COUNTER pages = size / MEM_PAGE_SIZE;

		AtomicAdd(pagesfree, -pages);
		AtomicAdd(pagesused, pages);

		AddAllocatedBlock(node, node->ptr, size);

		VPM_TRACE_1("VPM  Reuse Block:  Pointer:%p  Size:%d\n", node->ptr, size);
		return node->ptr;
	}

	// -----------------------------------------------------------------------
	inline void * AllocLargeBlock(
		MEMORY_SIZE size)
	{
		BLOCK_NODE * node = blocknodes.GetNew();
		ASSERT_RETNULL(node);

		void * ptr = malloc(size);
		CHECK_OUT_OF_MEMORY(ptr, size);
		if (!ptr)
		{
			VPM_TRACE_1("VPM  Alloc Large Block FAILED!!!:  Size:%d\n", size);
			blocknodes.Recycle(node);
			return NULL;
		}
#if MEM_FILL_AFTER_VALLOC
		memset(ptr, 1, size);
#endif

		MEM_COUNTER pages = GET_PAGES_PER_SIZE(size);
		AtomicAdd(pagesused, pages);
		AtomicIncrement(count);

		AddAllocatedBlock(node, ptr, size);

		VPM_TRACE_1("VPM  Alloc Large Block:  Pointer:%p  Size:%d  Count:%I64d  Mem Used:%I64d  Total Mem:%I64d\n", ptr, size, (INT64)count, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		return ptr;
	}

	// -----------------------------------------------------------------------
	inline void AllocSmallBlockFreeNodeList(
		BLOCK_NODE ** nodes,
		unsigned int top)
	{
		for (unsigned int ii = 0; ii < top; ++ii)
		{
			blocknodes.Recycle(nodes[ii]);
		}
	}

	// -----------------------------------------------------------------------
	inline void * AllocSmallBlock(
		MEMORY_SIZE blocksize)
	{
		BLOCK_NODE * nodes[64];
		unsigned int top = 0;

		MEMORY_SIZE largeblocksize = MEM_ALLOCATION_GRANULARITY / blocksize * blocksize;
		unsigned int nodecount = largeblocksize / blocksize;
		ASSERT(nodecount <= 64);

		for (unsigned int ii = 0; ii < nodecount; ++ii)
		{
			nodes[ii] = blocknodes.GetNew();
			ASSERT(nodes[ii]);
			if (!nodes[ii])
			{
				AllocSmallBlockFreeNodeList(nodes, top);
				return NULL;
			}
			++top;
		}

		void * ptr = VirtualAlloc(NULL, largeblocksize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		CHECK_OUT_OF_MEMORY(ptr, largeblocksize);
		if (!ptr)
		{
			AllocSmallBlockFreeNodeList(nodes, top);
			VPM_TRACE_1("VPM  Alloc Small Block FAILED!!!:  Size:%d\n", largeblocksize);
			return NULL;
		}
#if MEM_FILL_AFTER_VALLOC
		memset(ptr, 0, largeblocksize);
#endif

		MEM_COUNTER pages = largeblocksize / MEM_PAGE_SIZE;
		AtomicAdd(pagesused, pages);
		AtomicIncrement(count);

		AddAllocatedBlock(nodes[0], ptr, blocksize);

		for (unsigned int ii = 1; ii < nodecount; ++ii)
		{
			BLOCK_NODE * node = nodes[ii];
			node->ptr = (BYTE *)ptr + blocksize * ii;
			node->size = blocksize;
			AddFreeBlock(node);
		}

		VPM_TRACE_1("VPM  VirtualAlloc Small Block:  Pointer:%p  Size:%d/%d  Count:%I64d  Mem Used:%I64d  Total Mem:%I64d\n", ptr, blocksize, largeblocksize, (INT64)count, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		return ptr;
	}

	// -----------------------------------------------------------------------
	inline void * AllocNormalBlock(
		MEMORY_SIZE size,
		unsigned int index)
	{
		MEMORY_SIZE blocksize = freeblocks.GetBlockSizeFromIndex(index);
		ASSERT(blocksize >= size);

		if (blocksize < MEM_ALLOCATION_GRANULARITY && MEM_ALLOCATION_GRANULARITY / blocksize > 1)
		{
			return AllocSmallBlock(blocksize);
		}

		BLOCK_NODE * node = blocknodes.GetNew();
		ASSERT_RETNULL(node);

		void * ptr = VirtualAlloc(NULL, blocksize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		CHECK_OUT_OF_MEMORY(ptr, blocksize);
		if (!ptr)
		{
			VPM_TRACE_2("VPM  Alloc New Block FAILED!!!:  Size:%d\n", blocksize);
			blocknodes.Recycle(node);
			return NULL;
		}
#if MEM_FILL_AFTER_VALLOC
		memset(ptr, 0, blocksize);
#endif

		MEM_COUNTER pages = blocksize / MEM_PAGE_SIZE;
		AtomicAdd(pagesused, pages);
		AtomicIncrement(count);

		AddAllocatedBlock(node, ptr, blocksize);

		VPM_TRACE_1("VPM  VirtualAlloc New Block:  Pointer:%p  Size:%d  Count:%I64d  Mem Used:%I64d  Total Mem:%I64d\n", ptr, blocksize, (INT64)count, (INT64)pagesused * MEM_PAGE_SIZE, (INT64)(pagesused + pagesfree) * MEM_PAGE_SIZE);
		return ptr;
	}

public:
	// -----------------------------------------------------------------------
	void Init(
		void)
	{
		VPM_TRACE_1("VPM  Init()\n");

		Clear();
		InitPageSizeAndAllocationGranularityGlobals();

		blocknodes.Init();
	}

	// -----------------------------------------------------------------------
	void Free(
		void)
	{
		VPM_TRACE_1("VPM  Free()\n");
		MEM_COUNTER freed = 0;

		FreeAllocatedBlocks(freed);
		FreeFreeBlocks(freed);
		blocknodes.Free();

		// freed should be equal to count
		VPM_TRACE_2("VPM  Freed %I64d of %I64d Blocks\n", (INT64)freed, (INT64)count);
		ASSERT(freed == count);
		ASSERT(pagesused == 0);
		ASSERT(pagesfree == 0);

		Clear();
	}

	// -----------------------------------------------------------------------
	void * GetBlock(
		MEMORY_SIZE size)
	{
		unsigned int index = freeblocks.GetBlockIndexFromSize(size);
		if (index < freeblocks.GetHashSize())
		{
			void * ptr = GetBlockFromFreeHash(index);
			if (ptr)
			{
				return ptr;
			}
		}
		else
		{
			return AllocLargeBlock(size);
		}
		return AllocNormalBlock(size, index);
	}

	// -----------------------------------------------------------------------
	void RecycleBlock(
		void * ptr)
	{
		RemoveAllocatedBlock(ptr);
	}

	// -----------------------------------------------------------------------
	inline UINT64 GetMemoryTotal(
		void) const
	{
		return (pagesused + pagesfree) * MEM_PAGE_SIZE;
	}

	// -----------------------------------------------------------------------
	void MemoryTrace(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		PrintForWpp("--= VIRTUAL POOL MANAGER =--");
		PrintForWpp("Total Memory: %llu", GetMemoryTotal());
		freeblocks.MemoryTrace();
		allochash.MemoryTrace();
        	PrintForWpp("--= END POOL MANAGER DUMP=--");
#endif
	}
};


// ---------------------------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------------------------
VirtualPoolManager g_Vpm;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
template <unsigned int SIZE>
struct VPMTestStack
{
	unsigned int *index;
	unsigned int count;

	VPMTestStack(void) 
	{ 
		count = 0; 
		index = new unsigned int[SIZE]; 
	}

	~VPMTestStack(void) 
	{ 
		delete index; 
	}

	void Push(
		unsigned int val) 
	{ 
		ASSERT_RETURN(count < SIZE); 
		index[count++] = val; 
	}

	unsigned int Pop(void) 
	{ 
		ASSERT_RETFALSE(count > 0); 
		return index[--count]; 
	}

	void Shuffle(
		RAND & rand)
	{
		unsigned int ii = 0;
		do
		{
			unsigned int remaining = count - ii;
			if (remaining <= 1)
			{
				break;
			}
			unsigned int jj = ii + RandGetNum(rand, remaining);
			unsigned int temp = index[jj];
			index[jj] = index[ii];
			index[ii] = temp;
		} while (++ii);
	}
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
static MEMORY_SIZE sVPMTestGetSize(
	RAND & rand)
{
	static const unsigned int MAX_BLOCK_SIZE_1 = 80000;
	static const unsigned int MAX_BLOCK_SIZE_2 = 256 * 1024;
	if (RandGetNum(rand, 200) == 0)
	{
		return RandGetNum(rand, MAX_BLOCK_SIZE_2, MAX_BLOCK_SIZE_2 * 2);
	}
	return RandGetNum(rand, 1, MAX_BLOCK_SIZE_1);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
struct VPM_TEST
{	
	volatile long *				lGo;
	volatile long *				lDone;
	unsigned int				index;
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
DWORD WINAPI VirtualPoolManagerTestThread(
	void * param)
{
	VPM_TEST * vpmtest = (VPM_TEST *)param;
	
	unsigned int index = vpmtest->index;

	RAND rand;
	RandInit(rand, index + 1, index + 1);

	while (*vpmtest->lGo == 0)
	{
		Sleep(0);
	}

	static const unsigned int ITER_ONE = 4000;
	static const unsigned int ITER_TWO = 100;
	void * blocks[ITER_ONE];

	VPMTestStack<ITER_ONE> freeblocks;
	VPMTestStack<ITER_ONE> unfreed;

	DWORD start = GetTickCount();
	// first allocate ITER_ONE blocks
	for (unsigned int ii = 0; ii < ITER_ONE; ++ii)
	{
		MEMORY_SIZE size = sVPMTestGetSize(rand);
		blocks[ii] = VPM_GET_BLOCK(size);
		ASSERT(blocks[ii]);
		// memset(blocks[ii], 1, size);
		unfreed.Push(ii);
	}
	
	VPM_TRACE_2("[%d]  Allocate %d blocks Total Memory:%I64d  Time:%d\n", index, ITER_ONE, g_Vpm.GetMemoryTotal(), GetTickCount() - start);

	start = GetTickCount();

	DWORD count = 0;
	// now go through a cycle of free & allocate
	for (unsigned int ii = 0; ii < ITER_TWO; ++ii)
	{
		VPM_TRACE_1("\n*** Cycle: %d\n", ii);
		// free some blocks
		if (unfreed.count > 0)
		{
			unsigned int tofree = RandGetNum(rand, 1, unfreed.count);
			VPM_TRACE_1("To Free Count: %d\n", tofree);
			count += tofree;
			unfreed.Shuffle(rand);

			for (unsigned int jj = 0; jj < tofree; ++jj)
			{
				unsigned int index2 = unfreed.Pop();
				ASSERT(blocks[index2] != NULL);
				VPM_RELEASE_BLOCK(blocks[index2]);
				blocks[index2] = NULL;
				freeblocks.Push(index2);
			}
		}
		// allocate some blocks
		if (freeblocks.count > 0)
		{
			unsigned int toalloc = RandGetNum(rand, 1, freeblocks.count);
			VPM_TRACE_1("To Alloc Count: %d\n", toalloc);
			count += toalloc;
			freeblocks.Shuffle(rand);

			for (unsigned int jj = 0; jj < toalloc; ++jj)
			{
				unsigned int index2 = freeblocks.Pop();
				ASSERT(blocks[index2] == NULL);

				MEMORY_SIZE size = sVPMTestGetSize(rand);
				blocks[index2] = VPM_GET_BLOCK(size);
				// memset(blocks[index], 1, size);
				unfreed.Push(index2);
			}
		}
	}
	
	VPM_TRACE_2("[%d]  Allocate & Deallocate %d blocks  Total Memory:%I64d  Time:%d\n", index, count, g_Vpm.GetMemoryTotal(), GetTickCount() - start);

	start = GetTickCount();

	// third deallocate all blocks
	{
		unsigned int ii = 0;
		while (ii < ITER_ONE)
		{
			if (blocks[ii])
			{
				VPM_RELEASE_BLOCK(blocks[ii]);
				blocks[ii] = NULL;
			}
			++ii;
		}
	}
	
	VPM_TRACE_2("[%d]  Deallocate Some Blocks  Total Memory:%I64d  Time:%d\n", index, g_Vpm.GetMemoryTotal(), GetTickCount() - start);
	


	AtomicDecrement(*vpmtest->lDone);
	return 0;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
void VirtualPoolManagerTest(
	void)
{
	static const unsigned int THREAD_COUNT = 8;
	srand(2);

	g_Vpm.Init();

while(true)
{
	volatile long lGo = 0;
	volatile long lDone = THREAD_COUNT;

	VPM_TEST test[THREAD_COUNT];

	for (unsigned int ii = 0; ii < THREAD_COUNT; ++ii)
	{		
		test[ii].lGo = &lGo;
		test[ii].lDone = &lDone;
		test[ii].index = ii;

		DWORD dwThreadId;
		HANDLE handle = CreateThread(NULL, 0, VirtualPoolManagerTestThread, (void *)&test[ii], 0, &dwThreadId);
		CloseHandle(handle);
	}

	lGo = 1;

	while (lDone != 0)
	{
		Sleep(10);
	}
}

	DWORD start = GetTickCount();
	
	g_Vpm.Free();

	VPM_TRACE_2("\nFree all Total Memory:%I64d  Time:%d\n", g_Vpm.GetMemoryTotal(), GetTickCount() - start);

	
}
#endif


// -----------------------------------------------------------------------
struct CS_EmptyLock
{
	void Init(
		void)
	{
	}

#ifdef CS_DEBUG
	void InitDbg(
		const char * _file,
		unsigned int _line)
	{
	}
#endif

	void Free(
		void)
	{
	}

	void ReadLock(
		void)
	{
	}

	void EndRead(
		void)
	{
	}

	void WriteLock(
		void)
	{
	}

	void EndWrite(
		void)
	{
	}
};


// -----------------------------------------------------------------------
struct CS_CCritSectLite : CCritSectLite
{
	void Init(
		void)
	{
		CCritSectLite::Init();
	}

#ifdef CS_DEBUG
	void InitDbg(
		const char * _file,
		unsigned int _line)
	{
		CCritSectLite::InitDbg(_file, _line);
	}
#endif

	void Free(
		void)
	{
		CCritSectLite::Free();
	}

	void ReadLock(
		void)
	{
		CCritSectLite::Enter();
	}

	void EndRead(
		void)
	{
		CCritSectLite::Leave();
	}

	void WriteLock(
		void)
	{
		CCritSectLite::Enter();
	}

	void EndWrite(
		void)
	{
		CCritSectLite::Leave();
	}
};


// -----------------------------------------------------------------------
template <class LOCK_TYPE>
struct CS_SAFE_LOCK_READ
{
	LOCK_TYPE *							lock;

	CS_SAFE_LOCK_READ(
		LOCK_TYPE & in_lock) : lock(&in_lock)
	{
		in_lock.ReadLock();
	}

	~CS_SAFE_LOCK_READ(
		void)
	{
		lock->EndRead();
	}
};


// -----------------------------------------------------------------------
template <class LOCK_TYPE>
struct CS_SAFE_LOCK_WRITE
{
	LOCK_TYPE *							lock;

	CS_SAFE_LOCK_WRITE(
		LOCK_TYPE & in_lock) : lock(&in_lock)
	{
		in_lock.WriteLock();
	}

	~CS_SAFE_LOCK_WRITE(
		void)
	{
		lock->EndWrite();
	}
};


// -----------------------------------------------------------------------
// sorted array that uses VPM to allocate memory
template <class KEY_TYPE, class DATA_TYPE, class LOCK_TYPE>
struct VPM_SORTED_ARRAY
{
public:
	struct VSA_NODE
	{
		KEY_TYPE					key;									// sort key
		DATA_TYPE					data;									// data associated w/ key
	};

	LOCK_TYPE						cs;
	typedef CS_SAFE_LOCK_READ<LOCK_TYPE>	VSA_SAFE_LOCK_READ;
	typedef CS_SAFE_LOCK_WRITE<LOCK_TYPE>	VSA_SAFE_LOCK_WRITE;

	VSA_NODE *						list;									// sorted array of VSA_NODE
	unsigned int					size;									// allocated size of list (number of elements)
	unsigned int					count;									// number of elements in the list
	unsigned int					NODES_PER_PAGE;							// number of nodes per page

	// -------------------------------------------------------------------
	inline unsigned int FindInsertionPoint(
		KEY_TYPE key)
	{
		VSA_NODE * min = list;
		VSA_NODE * max = list + count;
		VSA_NODE * ii;
		while (min < max)
		{
			ii = min + (max - min) / 2;
			if (ii->key > key)
			{
				max = ii;
			}
			else if (ii->key < key)
			{
				min = ii + 1;
			}
			else
			{
				return (unsigned int)(ii - list);
			}
		}
		if (max < list + count)
		{
			return (unsigned int)(max - list);
		}
		return count;
	}

	// -------------------------------------------------------------------
	inline unsigned int FindExact(
		KEY_TYPE key)
	{
		VSA_NODE * min = list;
		VSA_NODE * max = list + count;
		VSA_NODE * ii;
		while (min < max)
		{
			ii = min + (max - min) / 2;
			if (ii->key > key)
			{
				max = ii;
			}
			else if (ii->key < key)
			{
				min = ii + 1;
			}
			else
			{
				return (unsigned int)(ii - list);
			}
		}
		return count;
	}

	// -------------------------------------------------------------------
	inline unsigned int FindEqualOrSmaller(
		KEY_TYPE key)
	{
		VSA_NODE * min = list;
		VSA_NODE * max = list + count;
		VSA_NODE * ii;
		while (min < max)
		{
			ii = min + (max - min) / 2;
			if (ii->key > key)
			{
				max = ii;
			}
			else if (ii->key < key)
			{
				min = ii + 1;
			}
			else
			{
				return (unsigned int)(ii - list);
			}
		}
		if (max > list)
		{
			return (unsigned int)((max - 1) - list);
		}
		return count;
	}

	// -------------------------------------------------------------------
	inline unsigned int FindEqualOrLarger(
		KEY_TYPE key)
	{
		VSA_NODE * min = list;
		VSA_NODE * max = list + count;
		VSA_NODE * ii;
		while (min < max)
		{
			ii = min + (max - min) / 2;
			if (ii->key > key)
			{
				max = ii;
			}
			else if (ii->key < key)
			{
				min = ii + 1;
			}
			else
			{
				return (unsigned int)(ii - list);
			}
		}
		if (min < list + count)
		{
			return (unsigned int)(min - list);
		}
		return count;
	}

	// -------------------------------------------------------------------
	inline BOOL sInsert(
		KEY_TYPE key,
		DATA_TYPE data)
	{
		ASSERT_RETFALSE(data);

		unsigned int ip = FindInsertionPoint(key);

		if (count < size)
		{
			if (ip < count)
			{
				memmove(list + ip + 1, list + ip, sizeof(VSA_NODE) * (count - ip));
			}
			++count;
			list[ip].key = key;
			list[ip].data = data;
			return TRUE;
		}

		unsigned int newsize = size + NODES_PER_PAGE;
		VSA_NODE * newlist = (VSA_NODE *)VPM_GET_BLOCK(sizeof(VSA_NODE) * newsize);
		if (NULL == newlist)
		{
			return FALSE;
		}
		
		if (list)
		{
			if (ip > 0)
			{
				memcpy(newlist, list, sizeof(VSA_NODE) * ip);
			}
			if (ip < count)
			{
				memcpy(newlist + ip + 1, list + ip, sizeof(VSA_NODE) * (count - ip));
			}
			VPM_RELEASE_BLOCK(list);
		}
		list = newlist;
		size = newsize;
		++count;
		list[ip].key = key;
		list[ip].data = data;
		return TRUE;
	}

	// -------------------------------------------------------------------
	inline BOOL sRemove(
		KEY_TYPE key)
	{
		unsigned int idx = FindExact(key);
		ASSERT_RETFALSE(idx < count);

		if (idx >= count)
		{
			return FALSE;
		}
		if (idx < count - 1)
		{
			memmove(list + idx, list + idx + 1, sizeof(VSA_NODE) * ((count - 1) - idx));
		}
		--count;
		return TRUE;
	}

	// -------------------------------------------------------------------
	// returns data for key within a certain range (inclusive)
	inline BOOL sFindInRange(
		KEY_TYPE minkey,
		KEY_TYPE maxkey,
		DATA_TYPE & data)
	{
		ASSERT_RETFALSE(minkey <= maxkey);

		unsigned int idx = FindEqualOrSmaller(minkey);
		if (idx >= count)
		{
			return FALSE;
		}

		if (list[idx].key > maxkey)
		{
			return FALSE;
		}

		return list[idx].data;
	}

public:
	// -------------------------------------------------------------------
	void Init(
		void)
	{
		InitCriticalSection(cs);
		list = NULL;
		size = 0;
		count = 0;
		NODES_PER_PAGE = MEM_PAGE_SIZE / sizeof(VSA_NODE);
		ASSERT(NODES_PER_PAGE > 0);
	}

	// -------------------------------------------------------------------
	void Free(
		void FREE_FUNC(KEY_TYPE key, DATA_TYPE data))
	{
		if (list)
		{
			for (unsigned int ii = 0; ii < count; ++ii)
			{
				FREE_FUNC(key, data);
			}
			VPM_RELEASE_BLOCK(list);
			list = NULL;
		}
		size = 0;
		count = 0;
		cs.Free();
	}

	// -------------------------------------------------------------------
	inline BOOL Insert(
		KEY_TYPE key,
		DATA_TYPE data)
	{
		VSA_SAFE_LOCK_WRITE autolock(cs);
		return sInsert(key, data);
	}

	// -------------------------------------------------------------------
	inline BOOL Remove(
		KEY_TYPE key)
	{
		VSA_SAFE_LOCK_WRITE autolock(cs);
		return sRemove(key);
	}

	// -------------------------------------------------------------------
	inline BOOL FindInRange(
		KEY_TYPE minkey,
		KEY_TYPE maxkey,
		DATA_TYPE & data)
	{
		VSA_SAFE_LOCK_READ autolock(cs);
		return sFindInRange(minkey, maxkey, data);
	}
};


#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
struct MEMORY_OVERRUN_GUARD_NODE
{
	void *								m_Ptr;
	void *								m_TruePtr;
	MEMORY_OVERRUN_GUARD_NODE *			m_Next;
	DWORD								m_Size;
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
struct MEMORY_OVERRUN_GUARD
{
	static const unsigned int			OVERRUN_GUARD_HASH_SIZE = 1024;

	CCritSectLite						m_CS;
	const char *						m_FrontGuardFile;
	unsigned int						m_FrontGuardLine;
	const char *						m_RearGuardFile;
	unsigned int						m_RearGuardLine;

	MEMORY_OVERRUN_GUARD_NODE *			m_BucketList;
	MEMORY_OVERRUN_GUARD_NODE *			m_FreeList;
	MEMORY_OVERRUN_GUARD_NODE *			m_Hash[OVERRUN_GUARD_HASH_SIZE];
};

MEMORY_OVERRUN_GUARD				g_DebugMemoryOverrunGuard;
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardInit(
	void)
{
	structclear(g_DebugMemoryOverrunGuard);
	g_DebugMemoryOverrunGuard.m_CS.Init();
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardFree(
	void)
{
	while (g_DebugMemoryOverrunGuard.m_BucketList)
	{
		MEMORY_OVERRUN_GUARD_NODE *	bucket = g_DebugMemoryOverrunGuard.m_BucketList;
		g_DebugMemoryOverrunGuard.m_BucketList = bucket->m_Next;
		VPM_RELEASE_BLOCK(bucket);
	}
	structclear(g_DebugMemoryOverrunGuard);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardSetFrontGuardFileLine(
	const char * file,
	unsigned int line)
{
	if (g_DebugMemoryOverrunGuard.m_FrontGuardFile == NULL)
	{
		g_DebugMemoryOverrunGuard.m_FrontGuardFile = file;
		g_DebugMemoryOverrunGuard.m_FrontGuardLine = line;
	}
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void DebugMemoryOverrunGuardSetRearGuardFileLine(
	const char * file,
	unsigned int line)
{
	if (g_DebugMemoryOverrunGuard.m_RearGuardFile == NULL)
	{
		g_DebugMemoryOverrunGuard.m_RearGuardFile = file;
		g_DebugMemoryOverrunGuard.m_RearGuardLine = line;
	}
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
MEMORY_OVERRUN_GUARD_NODE * sDebugMemoryOverrunGuardGetFreeNode(
	void)
{
	if (g_DebugMemoryOverrunGuard.m_FreeList == NULL)
	{
		unsigned int bucketcount = MEM_PAGE_SIZE / sizeof(MEMORY_OVERRUN_GUARD_NODE);

		MEMORY_OVERRUN_GUARD_NODE * bucket = (MEMORY_OVERRUN_GUARD_NODE *)VPM_GET_BLOCK(sizeof(MEMORY_OVERRUN_GUARD_NODE) * bucketcount);
		bucket->m_Next = g_DebugMemoryOverrunGuard.m_BucketList;
		g_DebugMemoryOverrunGuard.m_BucketList = bucket;
		for (unsigned int ii = 1; ii < bucketcount; ++ii)
		{
			MEMORY_OVERRUN_GUARD_NODE * node = bucket + ii;
			node->m_Next = g_DebugMemoryOverrunGuard.m_FreeList;
			g_DebugMemoryOverrunGuard.m_FreeList = node;
		}
	}
	MEMORY_OVERRUN_GUARD_NODE * node = g_DebugMemoryOverrunGuard.m_FreeList;
	g_DebugMemoryOverrunGuard.m_FreeList = node->m_Next;
	node->m_Next = NULL;
	return node;
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void sDebugMemoryOverrunGuardRecycleNode(
	MEMORY_OVERRUN_GUARD_NODE * node)
{
	memclear(node, sizeof(MEMORY_OVERRUN_GUARD_NODE));
	node->m_Next = g_DebugMemoryOverrunGuard.m_FreeList; 
	g_DebugMemoryOverrunGuard.m_FreeList = node;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
unsigned int sDebugMemoryOverrunGuardGetHashIndex(
	void * ptr)
{
	size_t diff = PTR_TO_SCALAR(ptr) / MEM_PAGE_SIZE;
	return RANDOMIZE(diff) % g_DebugMemoryOverrunGuard.OVERRUN_GUARD_HASH_SIZE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
static void sDebugMemoryOverrunGuardAddAllocToHash(
	void * ptr,
	void * trueptr,
	MEMORY_SIZE allocsize)
{
	unsigned int hashidx = sDebugMemoryOverrunGuardGetHashIndex(ptr);

	CSLAutoLock autolock(&g_DebugMemoryOverrunGuard.m_CS);

	MEMORY_OVERRUN_GUARD_NODE * node = sDebugMemoryOverrunGuardGetFreeNode();

	node->m_Ptr = ptr;
	node->m_TruePtr = trueptr;
	node->m_Size = allocsize;
	node->m_Next = g_DebugMemoryOverrunGuard.m_Hash[hashidx];
	g_DebugMemoryOverrunGuard.m_Hash[hashidx] = node;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
static BOOL sDebugMemoryOverrunGuardFindInHash(
	void * ptr,
	MEMORY_SIZE & allocsize)
{
	unsigned int hashidx = sDebugMemoryOverrunGuardGetHashIndex(ptr);

	CSLAutoLock autolock(&g_DebugMemoryOverrunGuard.m_CS);

	MEMORY_OVERRUN_GUARD_NODE * node = g_DebugMemoryOverrunGuard.m_Hash[hashidx];
	while (node)
	{
		if (node->m_Ptr == ptr)
		{
			allocsize = node->m_Size;
			return TRUE;
		}
		node = node->m_Next;
	}
	return FALSE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
static void * sDebugMemoryOverrunGuardRemoveFromHash(
	void * ptr,
	MEMORY_SIZE & allocsize)
{
	void * trueptr = NULL;
	unsigned int hashidx = sDebugMemoryOverrunGuardGetHashIndex(ptr);

	CSLAutoLock autolock(&g_DebugMemoryOverrunGuard.m_CS);

	MEMORY_OVERRUN_GUARD_NODE * prev = NULL;
	MEMORY_OVERRUN_GUARD_NODE * node = g_DebugMemoryOverrunGuard.m_Hash[hashidx];
	while (node)
	{
		if (node->m_Ptr == ptr)
		{
			trueptr = node->m_TruePtr;
			if (prev)
			{
				prev->m_Next = node->m_Next;
			}
			else
			{
				g_DebugMemoryOverrunGuard.m_Hash[hashidx] = node->m_Next;
			}
			allocsize = node->m_Size;
			sDebugMemoryOverrunGuardRecycleNode(node);
			return trueptr;
		}
		prev = node;
		node = node->m_Next;
	}
	return NULL;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void * DebugMemoryOverrunGuardAlloc(
	const char * file,
	unsigned int line,
	MEMORY_SIZE size)
{
	if (g_DebugMemoryOverrunGuard.m_FrontGuardFile == file && g_DebugMemoryOverrunGuard.m_FrontGuardLine == line)
	{
		void * trueptr = VirtualAlloc(NULL, size + MEM_PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
		ASSERT(trueptr);
		void * ptr = (void *)((BYTE *)trueptr + MEM_PAGE_SIZE);

		ASSERT(VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) == ptr);

		sDebugMemoryOverrunGuardAddAllocToHash(ptr, trueptr, size);
		return ptr;
	}
	else if (g_DebugMemoryOverrunGuard.m_RearGuardFile == file && g_DebugMemoryOverrunGuard.m_RearGuardLine == line)
	{
		MEMORY_SIZE roundedsize = ROUNDUP(size, MEM_PAGE_SIZE);
		void * trueptr = VirtualAlloc(NULL, roundedsize + MEM_PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
		ASSERT(trueptr);
		void * ptr = (void *)((BYTE *)trueptr + (roundedsize - size));
		void * guardpage = (void *)((BYTE *)trueptr + roundedsize);

		ASSERT(VirtualAlloc(trueptr, roundedsize, MEM_COMMIT, PAGE_READWRITE) == trueptr);

		sDebugMemoryOverrunGuardAddAllocToHash(ptr, trueptr, size);
		return ptr;
	}
	else
	{
		return NULL;
	}
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
BOOL DebugMemoryOverrunGuardFree(
	void * ptr)
{
	MEMORY_SIZE oldsize;
	void * trueptr = sDebugMemoryOverrunGuardRemoveFromHash(ptr, oldsize);
	if (!trueptr)
	{
		return FALSE;
	}
	VirtualFree(trueptr, 0, MEM_RELEASE);
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// this isn't really thread-safe, particularly for incorrect code!!!
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_OVERRUN_GUARD
void * DebugMemoryOverrunGuardRealloc(
	const char * file,
	unsigned int line,
	void * oldptr,
	MEMORY_SIZE size)
{
	if (!((g_DebugMemoryOverrunGuard.m_FrontGuardFile == file && g_DebugMemoryOverrunGuard.m_FrontGuardLine == line) ||
		(g_DebugMemoryOverrunGuard.m_RearGuardFile == file && g_DebugMemoryOverrunGuard.m_RearGuardLine == line)))
	{
		return NULL;
	}

	MEMORY_SIZE oldsize;
	if (!sDebugMemoryOverrunGuardFindInHash(oldptr, oldsize))
	{
		return DebugMemoryOverrunGuardAlloc(file, line, size);
	}

	void * ptr = DebugMemoryOverrunGuardAlloc(file, line, size);
	ASSERT(ptr);
	memcpy(ptr, oldptr, oldsize);
	if (size > oldsize)
	{
		memclear((BYTE *)ptr + oldsize, size - oldsize);
	}

	ASSERT(DebugMemoryOverrunGuardFree(oldptr));

	return ptr;
}
#endif

#if !USE_MEMORY_MANAGER


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline void MEM_SET_BIT(
	DWORD & flags,
	unsigned int bit)
{
	flags |= (1 << bit);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline void MEM_CLEAR_BIT(
	DWORD & flags,
	unsigned int bit)
{
	flags &= ~(1 << bit);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline BOOL MEM_TEST_BIT(
	DWORD flags,
	unsigned int bit)
{
	return ((flags & (1 << bit)) != 0);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline void MEM_SET_BBUF_VALUE(
	BYTE * bbuf,
	unsigned int bytes,
	unsigned int offset,
	unsigned int bits, 
	unsigned int val)
{
	DBG_ASSERT(bits == 32 || val < ((unsigned int)1 << bits));
	DBG_ASSERT(offset + bits <= bytes * BITS_PER_BYTE);
	REF(bytes);
	
	unsigned int bitsleft = bits;
	BYTE * buf = bbuf + offset / BITS_PER_BYTE;
	// write leading bits in first byte
	unsigned int leadingoffs = (offset % BITS_PER_BYTE);
	unsigned int leadingbits = BITS_PER_BYTE - leadingoffs;
	if (leadingoffs != 0)
	{
		unsigned int bitstowrite = MIN(leadingbits, bits);
		// clear bits we're about to write
		BYTE mask = (BYTE)((1 << bitstowrite) - 1);
		*buf &= ~(mask << leadingoffs);
		*buf |= ((val & mask) << leadingoffs);
		++buf;
		bitsleft -= bitstowrite;
		val >>= bitstowrite;
	}
	// write whole byte bits
	switch (bitsleft / BITS_PER_BYTE)
	{
	case 4:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		buf[2] = (BYTE)(val >> 16) & 0xff;
		buf[3] = (BYTE)(val >> 24) & 0xff;
		return;
	case 3:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		buf[2] = (BYTE)(val >> 16) & 0xff;
		bitsleft -= 24;
		val >>= 24;
		buf += 3;
		break;
	case 2:
		buf[0] = (BYTE)(val & 0xff);
		buf[1] = (BYTE)(val >> 8) & 0xff;
		bitsleft -= 16;
		val >>= 16;
		buf += 2;
		break;
	case 1:
		buf[0] = (BYTE)(val & 0xff);
		bitsleft -= 8;
		val >>= 8;
		buf += 1;
		break;
	case 0:
		break;
	default: __assume(0);
	}
	// write trailing bits
	if (bitsleft)
	{
		unsigned int bitstowrite = bitsleft;
		BYTE mask = (BYTE)((1 << bitstowrite) - 1);
		*buf &= ~mask;
		*buf |= val;
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline unsigned int MEM_GET_BBUF_VALUE(
	BYTE * bbuf,
	unsigned int bytes,
	unsigned int offset,
	unsigned int bits)
{
	DBG_ASSERT(offset + bits <= bytes * BITS_PER_BYTE);
	REF(bytes);

	unsigned int val = 0;
	unsigned int bitsleft = bits;
	BYTE * buf = bbuf + offset / BITS_PER_BYTE;
	// read leading bits
	unsigned int leadingoffs = (offset % BITS_PER_BYTE);
	unsigned int leadingbits = (BITS_PER_BYTE - leadingoffs) % BITS_PER_BYTE;
	if (leadingoffs != 0)
	{
		unsigned int bitstoread = MIN(leadingbits, bits);
		// clear bits we're about to write
		BYTE mask = (BYTE)((1 << bitstoread) - 1);
		val = (*buf & (mask << leadingoffs)) >> leadingoffs;
		++buf;
		bitsleft -= bitstoread;
	}
	// read whole byte bits
	unsigned int temp = 0;
	switch (bitsleft / BITS_PER_BYTE)
	{
	case 4:
		temp += buf[0];
		temp += buf[1] << 8;
		temp += buf[2] << 16;
		temp += buf[3] << 24;
		return val + (temp << leadingbits);
	case 3:
		temp += buf[0];
		temp += buf[1] << 8;
		temp += buf[2] << 16;
		val += (temp << leadingbits);
		bitsleft -= 24;
		buf += 3;
		break;
	case 2:
		temp += buf[0];
		temp += buf[1] << 8;
		val += (temp << leadingbits);
		bitsleft -= 16;
		buf += 2;
		break;
	case 1:
		temp += buf[0];
		val += (temp << leadingbits);
		bitsleft -= 8;
		buf += 1;
		break;
	case 0:
		break;
	default: __assume(0);
	}
	// read trailing bits
	if (bitsleft)
	{
		unsigned int bitstoread = bitsleft;
		BYTE mask = (BYTE)((1 << bitstoread) - 1);
		temp = (*buf & mask);
		val += (temp << (bits - bitsleft));
	}
	return val;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if MEMORY_TEST_ENABLED
void MemBitTest(
	void)
{
	static const unsigned int ITER = 10000;
	static const unsigned int ITER_TWO = 10000000;
	BYTE * buf = (BYTE *)malloc(ITER * 4);

	struct MBT_NODE
	{
		unsigned int val;
		unsigned int offset;
		unsigned int bits;
	};
	MBT_NODE * list = (MBT_NODE *)malloc(ITER * sizeof(MBT_NODE));

	RAND rand;
	RandInit(rand, 1, 1);

	// fill in the array
	unsigned int bits = 0;
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		list[ii].offset = bits;
		list[ii].bits = RandGetNum(rand, 32) + 1;
		bits += list[ii].bits;
		list[ii].val = (list[ii].bits == 32) ? RandGetNum(rand) : (RandGetNum(rand) >> (BITS_PER_DWORD - list[ii].bits));
		MEM_SET_BBUF_VALUE(buf, ITER * 4, list[ii].offset, list[ii].bits, list[ii].val);
		unsigned int check = MEM_GET_BBUF_VALUE(buf, ITER * 4, list[ii].offset, list[ii].bits);
		ASSERT(check == list[ii].val);
	}
	// double check the array
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		unsigned int check = MEM_GET_BBUF_VALUE(buf, ITER * 4, list[ii].offset, list[ii].bits);
		ASSERT(check == list[ii].val);
	}
	// randomly write stuff into the array
	for (unsigned int ii = 0; ii < ITER_TWO; ++ii)
	{
		unsigned int idx = RandGetNum(rand, ITER);
		unsigned int check = MEM_GET_BBUF_VALUE(buf, ITER * 4, list[idx].offset, list[idx].bits);
		ASSERT(check == list[idx].val);
		list[idx].val = (list[idx].bits == 32) ? RandGetNum(rand) : (RandGetNum(rand) >> (BITS_PER_DWORD - list[idx].bits));
		MEM_SET_BBUF_VALUE(buf, ITER * 4, list[idx].offset, list[idx].bits, list[idx].val);
		check = MEM_GET_BBUF_VALUE(buf, ITER * 4, list[idx].offset, list[idx].bits);
		ASSERT(check == list[idx].val);
	}
	// double check the array
	for (unsigned int ii = 0; ii < ITER; ++ii)
	{
		unsigned int check = MEM_GET_BBUF_VALUE(buf, ITER * 4, list[ii].offset, list[ii].bits);
		ASSERT(check == list[ii].val);
	}
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
struct FLIDX_NODE;
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_SINGLE_ALLOCATIONS
struct ALLOC_NODE
{
	FLIDX_NODE *					flidx;
	ALLOC_NODE *					prev;
	ALLOC_NODE *					next;
	void *							ptr;										// actual pointer
	MEMORY_SIZE						size;										// size of allocation (doesn't include debug overhead)
	long							alloc_num;									// allocation number (one-up)
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
struct FLIDX_NODE
{
public:
	CCritSectLite					cs;
	FLIDX_NODE *					next;										// next node in hash
#if DEBUG_SINGLE_ALLOCATIONS
	ALLOC_NODE *					alloc_head;
	ALLOC_NODE *					alloc_tail;
#endif
	const char *					file;
	unsigned int					line;
	MEMORY_SIZE						cur_memory;									// current number of bytes
	MEMORY_SIZE						last_memory;								// number of bytes allocated at the last snapshot
	MEMORY_SIZE						max_memory;									// maximum amount of bytes at one time
	MEMORY_SIZE						wdw_memory;									// memory allocated in window
	MEMORY_SIZE						min_single;									// minimum single allocation bytes
	MEMORY_SIZE						max_single;									// maximum single allocation bytes
	unsigned int					cur_allocs;									// current number of allocations
	unsigned int					last_allocs;								// number of allocations at the last snapshot
	unsigned int					max_allocs;									// maximum number of allocations at one time
	unsigned int					tot_allocs;									// total number of allocations at one time
	unsigned int					wdw_allocs;									// number of allocations within window period

	// -----------------------------------------------------------------------
	// assume memclear already
	void Init(
		void)
	{
		min_single = (MEMORY_SIZE)(-1);
	}

#if DEBUG_SINGLE_ALLOCATIONS
	// -----------------------------------------------------------------------
	inline void AddAllocNode(
		ALLOC_NODE * anode)
	{
		anode->flidx = this;
		CSLAutoLock autolock(&cs);
		anode->next = NULL;
		anode->prev = alloc_tail;
		if (alloc_tail)
		{
			alloc_tail->next = anode;
		}
		else
		{
			alloc_head = anode;
		}
		alloc_tail = anode;
	}
#endif

#if DEBUG_SINGLE_ALLOCATIONS
	// -----------------------------------------------------------------------
	inline void RemoveAllocNode(
		ALLOC_NODE * anode)
	{
		ASSERT_RETURN(anode);
		if (anode->prev)
		{
			anode->prev->next = anode->next;
		}
		else
		{
			alloc_head = anode->next;
		}
		if (anode->next)
		{
			anode->next->prev = anode->prev;
		}
		else
		{
			alloc_tail = anode->prev;
		}
		anode->next = NULL;
		anode->prev = NULL;
	}
#endif
};
#endif


// ---------------------------------------------------------------------------
// this needs to be 16 bytes for proper memory alignment
// don't add anything to this structure!!!
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
#pragma pack(push, 8)
struct MEMORY_DEBUG_HEADER
{
#if DEBUG_SINGLE_ALLOCATIONS
	ALLOC_NODE *					alloc_node;
#else
	FLIDX_NODE *					flidx_node;
#endif

#ifndef _WIN64
	const char *					file;
#endif
	unsigned int					line;
	DWORD							dwMagic;
};
#pragma pack(pop)
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
struct MEMORY_DEBUG_FOOTER
{
	DWORD							dwMagic;
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
#define MEMORY_DEBUG_HEADER_SIZE	(sizeof(MEMORY_DEBUG_HEADER))
#define MEMORY_DEBUG_FOOTER_SIZE	(sizeof(MEMORY_DEBUG_FOOTER))
#define MEMORY_DEBUG_SIZE			(MEMORY_DEBUG_HEADER_SIZE + MEMORY_DEBUG_FOOTER_SIZE)
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct MEMORYPOOL
{
	friend struct MEMORY_SYSTEM;


public:
	
	MEMORY_MANAGEMENT_TYPE			m_eType;									// type of allocator
	unsigned int					m_nIndex;									// index in global memory system

	// Tracks the wasted space within all current externally allocated blocks including alignment
	//
	unsigned int					m_InternalFragmentation;					

	// Tracks wasted space due to overhead within the system such as usedflags, 
	// external allocation size tables, and unused blocks
	//
	unsigned int					m_InternalOverhead;	

	// Total number of bytes currently externally allocated from the pool
	//
	unsigned int					m_SmallAllocationSize; 
	unsigned int					m_LargeAllocationSize;	// CRT heap

	// Counts of the number current external allocations from the pool
	//
	unsigned int					m_SmallAllocations; 	// does not account for alignment or debug info
	unsigned int					m_LargeAllocations;

	WCHAR							m_Name[32];									// name of pool
	
#if DEBUG_MEMORY_ALLOCATIONS

	struct FLIDX_TRACKER
	{
	protected:
		static const unsigned int	MEM_FLIDX_SIZE = 4096;						// size of hash
		static const unsigned int	MEM_FLIDX_MULT = 46061;						// prime to distribute hash

		FLIDX_NODE *				m_Hash[MEM_FLIDX_SIZE];						// hash table to track allocations by file & line #
		CCritSectLite				m_lockHash[MEM_FLIDX_SIZE];

		FLIDX_NODE					m_Window[FLIDX_WINDOW_SIZE];				// most allocations during window
		TIME						m_timeWindow;								// next window reset time

		static const unsigned int	MEM_FLIDX_BLOCK = (DEFAULT_PAGE_SIZE / sizeof(FLIDX_NODE));		// size of block of nodes to allocate
		CCritSectLite				m_lockStore;
		FLIDX_NODE *				m_Store;									// unused FLIDX_NODE nodes
		unsigned int				m_StoreFirst;								// first unused FLIDX_NODE node in a free block

		// -------------------------------------------------------------------
		// get a new FLIDX_NODE node
		inline FLIDX_NODE * GetNewNode(
			const char * file,
			unsigned int line)
		{
			FLIDX_NODE * node;
			{
				CSLAutoLock autolock(&m_lockStore);
				if (m_StoreFirst == 0)
				{
					FLIDX_NODE * newblock = (FLIDX_NODE *)VPM_GET_BLOCK(sizeof(FLIDX_NODE) * MEM_FLIDX_BLOCK);
					CHECK_OUT_OF_MEMORY(newblock, sizeof(FLIDX_NODE) * MEM_FLIDX_BLOCK);
					ASSERT_RETNULL(newblock);

					newblock->next = m_Store;
					m_Store = newblock;
					m_StoreFirst = 2;
					node = m_Store + 1;
				}
				else
				{
					node = m_Store + m_StoreFirst;
					++m_StoreFirst;
					if (m_StoreFirst >= MEM_FLIDX_BLOCK)
					{
						m_StoreFirst = 0;
					}
				}
			}
			memclear(node, sizeof(FLIDX_NODE));
			node->Init();
			InitCriticalSection(node->cs);
			node->file = file;
			node->line = line;
			return node;
		}

		// -------------------------------------------------------------------
		inline FLIDX_NODE * GetNode(
			const char * file,
			unsigned int line)
		{
			unsigned int key = (unsigned int)(((DWORD_PTR)file + line) * MEM_FLIDX_MULT) % MEM_FLIDX_SIZE;
			
			CSLAutoLock autolock(&m_lockHash[key]);
			FLIDX_NODE * node = m_Hash[key];
			while (node)
			{
				if (node->file == file && node->line == line)
				{
					break;
				}
				node = node->next;
			}
			if (!node)
			{
				node = GetNewNode(file, line);
				node->next = m_Hash[key];
				m_Hash[key] = node;
			}

			return node;
		}

	public:
		// -------------------------------------------------------------------
		void Init(
			void)
		{
			InitCriticalSection(m_lockStore);
			m_Store = NULL;
			m_StoreFirst = 0;

			for (unsigned int ii = 0; ii < arrsize(m_lockHash); ++ii)
			{
				InitCriticalSection(m_lockHash[ii]);
				m_Hash[ii] = NULL;
			}

			memclear(m_Window, sizeof(m_Window));
			m_timeWindow = 0;
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			FLIDX_NODE * block;
			FLIDX_NODE * next = m_Store;
			while ((block = next) != NULL)
			{
				next = block->next;
				VPM_RELEASE_BLOCK(block);
			}
			m_Store = NULL;
			m_StoreFirst = 0;
			m_lockStore.Free();

			for (unsigned int ii = 0; ii < arrsize(m_lockHash); ++ii)
			{
				m_lockHash[ii].Free();
				m_Hash[ii] = NULL;
			}

			memclear(m_Window, sizeof(m_Window));
			m_timeWindow = 0;
		}

		// -------------------------------------------------------------------
		// track a new allocation
		inline FLIDX_NODE * TrackAlloc(
			void * trueptr,
			MEMORY_SIZE size,
			const char * file,
			unsigned int line)
		{
			REF(trueptr);

			FLIDX_NODE * flidx = GetNode(file, line);
			ASSERT_RETNULL(flidx);

			CSLAutoLock autolock(&flidx->cs);

			flidx->cur_memory += size;
			flidx->wdw_memory += size;
			flidx->max_memory = MAX(flidx->cur_memory, flidx->max_memory);
			flidx->min_single = MIN(size, flidx->min_single);
			flidx->max_single = MAX(size, flidx->max_single);
			flidx->cur_allocs++;
			flidx->max_allocs = MAX(flidx->cur_allocs, flidx->max_allocs);
			flidx->tot_allocs++;
			flidx->wdw_allocs++;

			return flidx;
		}

		// -------------------------------------------------------------------
		// track a free
		inline void TrackFree(
			FLIDX_NODE * flidx,
#if DEBUG_SINGLE_ALLOCATIONS
			ALLOC_NODE * anode,
#endif
			MEMORY_SIZE size)
		{
			CSLAutoLock autolock(&flidx->cs);

#if DEBUG_SINGLE_ALLOCATIONS
			anode->flidx = NULL;
			flidx->RemoveAllocNode(anode);
#endif
			flidx->cur_memory -= size;
			flidx->cur_allocs--;
		}

		// -------------------------------------------------------------------
		// track a realloc
		inline FLIDX_NODE * TrackRealloc(
			void * trueptr,
			FLIDX_NODE * flidx, 
#if DEBUG_SINGLE_ALLOCATIONS
			ALLOC_NODE * anode,
#endif
			MEMORY_SIZE oldsize,
			MEMORY_SIZE size,
			const char * file,
			unsigned int line)
		{
#if DEBUG_SINGLE_ALLOCATIONS
			TrackFree(flidx, anode, oldsize);
#else
			TrackFree(flidx, oldsize);
#endif

			flidx = TrackAlloc(trueptr, size, file, line);
			return flidx;
		}

		// -------------------------------------------------------------------
		void TraceFLIDX(
			void)
		{
#if DEBUG_MEMORY_ALLOCATIONS
			PrintForWpp("%30s  %6s  %9s  %9s %9s  %7s  %7s  %8s  %9s  %9s %9s\n", 
				"FILE", "LINE", "CUR_MEM", "DIFF_MEM", "MAX_MEM", "CUR_NUM", "MAX_NUM", "TOT_NUM", "MIN_ALLOC", "MAX_ALLOC", "DIFF_ALLOC" );
			for (unsigned int ii = 0; ii < arrsize(m_Hash); ++ii)
			{
				// the list iteration is unlocked, we assume that all insertions occur at the head and there are no deletions
				FLIDX_NODE * curr = NULL;
				FLIDX_NODE * next;
				{
					CSLAutoLock autolock(&m_lockHash[ii]);
					next = m_Hash[ii];
				}

				while ((curr = next) != NULL)
				{
					next = curr->next;

					FLIDX_NODE flidx;
					{
						CSLAutoLock autolock(&curr->cs);
						memcpy(&flidx, curr, sizeof(FLIDX_NODE));
						 
						// Save the current stats for the next snapshot.
						curr->last_memory = curr->cur_memory;
						curr->last_allocs = curr->cur_allocs;
					}

					char filename[31];
					unsigned int len = PStrLen(flidx.file);
					PStrCopy(filename, flidx.file + (len <= 30 ? 0 : len - 30), arrsize(filename));
					PrintForWpp("%30s  %6u  %9llu  %9lld %9llu  %7u  %7u  %8u  %9llu  %9llu %9lld", 
						filename, flidx.line, (UINT64)flidx.cur_memory, 
						(INT64)flidx.cur_memory - (INT64)flidx.last_memory,
						(UINT64)flidx.max_memory, flidx.cur_allocs, flidx.max_allocs, 
						flidx.tot_allocs, (UINT64)flidx.min_single, 
						(UINT64)flidx.max_single,
						(INT64)flidx.cur_allocs - (INT64)flidx.last_allocs);
				}
			}
			PrintForWpp("\n");
#endif
		}

		// -------------------------------------------------------------------
		void TraceUnfreed(
			void)
		{
#if DEBUG_MEMORY_ALLOCATIONS
			PrintForWpp("%30s  %6s  %9s  %9s  %7s  %7s  %8s  %9s  %9s\n", "FILE", "LINE", "CUR_MEM", "MAX_MEM", "CUR_NUM", "MAX_NUM", "TOT_NUM", "MIN_ALLOC", "MAX_ALLOC");
			for (unsigned int ii = 0; ii < arrsize(m_Hash); ++ii)
			{
				// the list iteration is unlocked, we assume that all insertions occur at the head and there are no deletions
				FLIDX_NODE * curr = NULL;
				FLIDX_NODE * next;
				{
					CSLAutoLock autolock(&m_lockHash[ii]);
					next = m_Hash[ii];
				}

				while ((curr = next) != NULL)
				{
					next = curr->next;

					FLIDX_NODE flidx;
					{
						CSLAutoLock autolock(&curr->cs);
						memcpy(&flidx, curr, sizeof(FLIDX_NODE));
					}

					if (flidx.cur_allocs <= 0)
					{
						continue;
					}

					char filename[31];
					unsigned int len = PStrLen(flidx.file);
					PStrCopy(filename, flidx.file + (len <= 30 ? 0 : len - 30), arrsize(filename));
					PrintForWpp("%30s  %6u  %9llu  %9llu  %7u  %7u  %8u  %9llu  %9llu", filename, flidx.line, (UINT64)flidx.cur_memory, 
						(UINT64)flidx.max_memory, flidx.cur_allocs, flidx.max_allocs, flidx.tot_allocs, (UINT64)flidx.min_single, (UINT64)flidx.max_single);
				}
			}
			PrintForWpp("\n");
#endif
		}

		// -------------------------------------------------------------------
		void TraceALLOC(
			void)
		{
#if DEBUG_SINGLE_ALLOCATIONS
			PrintForWpp("%9s  %16s  %30s  %6s  %9s\n", "ALLOC#", "ADDRESS", "FILE", "LINE", "MEM");
			for (unsigned int ii = 0; ii < arrsize(m_Hash); ++ii)
			{
				// the list iteration is unlocked, we assume that all insertions occur at the head and there are no deletions
				FLIDX_NODE * curr = NULL;
				FLIDX_NODE * next;
				{
					CSLAutoLock autolock(&m_lockHash[ii]);
					next = m_Hash[ii];
				}

				while ((curr = next) != NULL)
				{
					next = curr->next;

					CSLAutoLock autolock(&curr->cs);

					ALLOC_NODE * alloc = curr->alloc_head;

					char filename[31];
					if (alloc)
					{
						unsigned int len = PStrLen(alloc->flidx->file);
						PStrCopy(filename, alloc->flidx->file + (len <= 30 ? 0 : len - 30), arrsize(filename));
					}

					while (alloc)
					{
						PrintForWpp("%9u  %16p  %30s  %6u  %9llu\n", alloc->alloc_num, alloc->ptr, alloc->flidx->file, alloc->flidx->line, (UINT64)alloc->size);
						alloc = alloc->next;
					}
				}
			}
			PrintForWpp("\n");
#endif
		}

		// -------------------------------------------------------------------
		// collect window data
		void TrackWindow(
			void)
		{
			TIME curtime = AppCommonGetCurTime();
			if (curtime < m_timeWindow)
			{
				return;
			}
			m_timeWindow = curtime + FLIDX_WINDOW_TIME;

			memclear(m_Window, sizeof(m_Window));

			for (unsigned int ii = 0; ii < arrsize(m_Hash); ++ii)
			{
				// the list iteration is unlocked, we assume that all insertions occur at the head and there are no deletions
				// the list iteration is unlocked, we assume that all insertions occur at the head and there are no deletions
				FLIDX_NODE * curr = NULL;
				FLIDX_NODE * next;
				{
					CSLAutoLock autolock(&m_lockHash[ii]);
					next = m_Hash[ii];
				}

				while ((curr = next) != NULL)
				{
					next = curr->next;

					CSLAutoLock autolock(&curr->cs);

					unsigned int wdwmem = curr->wdw_memory;
					if (wdwmem)
					{
						int minj = -1;
						unsigned int minmem = wdwmem;
						for (unsigned int jj = 0; jj < FLIDX_WINDOW_SIZE; jj++)
						{
							if (minmem >= m_Window[jj].wdw_memory)
							{
								minmem = m_Window[jj].wdw_memory;
								minj = jj;
							}
						}
						if (minj >= 0)
						{
							memcpy(&m_Window[minj], curr, sizeof(*curr));
						}
					}
				}
			}
		}

		void GetWindow(
			MEMORY_INFO_LINE * window,
			unsigned int size)
		{
			ASSERT_RETURN(size == FLIDX_WINDOW_SIZE);

			for (unsigned int ii = 0; ii < FLIDX_WINDOW_SIZE; ii++)
			{
				window[ii].m_csFile = m_Window[ii].file;
				window[ii].m_nLine = m_Window[ii].line;
				window[ii].m_nCurBytes = m_Window[ii].cur_memory;
				window[ii].m_nCurAllocs = m_Window[ii].cur_allocs;
				window[ii].m_nRecentBytes = m_Window[ii].wdw_memory;
				window[ii].m_nRecentAllocs = m_Window[ii].wdw_allocs;
			}
		}

	};
	FLIDX_TRACKER					flidx_tracker;
#endif

#if DEBUG_SINGLE_ALLOCATIONS
	struct ALLOC_TRACKER
	{
	protected:
		static const unsigned int	MEM_ALLOC_BLOCK = (DEFAULT_PAGE_SIZE * 4/ sizeof(ALLOC_NODE));		// size of block of nodes to allocate

		volatile long				m_AllocNum;

		CCritSectLite				m_lockStore;
		ALLOC_NODE *				m_Garbage;
		ALLOC_NODE *				m_Store;
		unsigned int				m_StoreFirst;

		// -------------------------------------------------------------------
		inline ALLOC_NODE * GetNewNode(
			void)
		{
			ALLOC_NODE * node;
			{
				CSLAutoLock autolock(&m_lockStore);
				if (m_Garbage)
				{
					node = m_Garbage;
					m_Garbage = node->next;
					return node;
				}

				if (m_StoreFirst == 0)
				{
					ALLOC_NODE * newblock = (ALLOC_NODE *)VPM_GET_BLOCK(sizeof(ALLOC_NODE) * MEM_ALLOC_BLOCK);
					CHECK_OUT_OF_MEMORY(newblock, sizeof(ALLOC_NODE) * MEM_ALLOC_BLOCK);

					newblock->next = m_Store;
					m_Store = newblock;
					m_StoreFirst = 2;
					node = m_Store + 1;
				}
				else
				{
					node = m_Store + m_StoreFirst;
					++m_StoreFirst;
					if (m_StoreFirst >= MEM_ALLOC_BLOCK)
					{
						m_StoreFirst = 0;
					}
				}
			}
			memclear(node, sizeof(ALLOC_NODE));
			return node;
		}

		// -------------------------------------------------------------------
		inline void Recycle(
			ALLOC_NODE * node)
		{
			CSLAutoLock autolock(&m_lockStore);
			node->next = m_Garbage;
			m_Garbage = node;
		}

	public:
		// -------------------------------------------------------------------
		void Init(
			void)
		{
			InitCriticalSection(m_lockStore);
			m_Garbage = NULL;
			m_Store = NULL;
			m_StoreFirst = 0;
			m_AllocNum = 0;
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			m_AllocNum = 0;

			ALLOC_NODE * block;
			ALLOC_NODE * next = m_Store;
			while ((block = next) != NULL)
			{
				next = block->next;
				VPM_RELEASE_BLOCK(block);
			}
			m_Store = NULL;
			m_StoreFirst = 0;
			m_lockStore.Free();
		}

		// -------------------------------------------------------------------
		// track a new allocation
		inline void TrackAlloc(
			void * trueptr,
			FLIDX_NODE * flidx,
			ALLOC_NODE * anode,
			MEMORY_SIZE size)
		{
			anode->ptr = sGetPointerFromTruePointer(trueptr);
			anode->size = size;
			anode->alloc_num = AtomicIncrement(m_AllocNum);

			flidx->AddAllocNode(anode);			
		}

		// -------------------------------------------------------------------
		// track a new allocation
		inline ALLOC_NODE * TrackAlloc(
			void * trueptr,
			FLIDX_NODE * flidx,
			MEMORY_SIZE size)
		{
			ALLOC_NODE * anode = GetNewNode();
			ASSERT_RETNULL(anode);
			TrackAlloc(trueptr, flidx, anode, size);
			return anode;
		}

		// -------------------------------------------------------------------
		// track a free
		inline void TrackFree(
			ALLOC_NODE * anode)
		{
			Recycle(anode);
		}
	};
	ALLOC_TRACKER					alloc_tracker;
#endif

	// -----------------------------------------------------------------------
	static MEMORY_SIZE sGetSizeToAlloc(
		MEMORY_SIZE size)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		return size + MEMORY_DEBUG_SIZE;
#else
		return size;
#endif
	}

	// -----------------------------------------------------------------------
	static MEMORY_SIZE sGetAllocSizeFromSize(
		MEMORY_SIZE size)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		return size + MEMORY_DEBUG_SIZE;
#else
		return size;
#endif
	}

	// -----------------------------------------------------------------------
	static MEMORY_SIZE sGetSizeFromAllocSize(
		MEMORY_SIZE size)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		return size - MEMORY_DEBUG_SIZE;
#else
		return size;
#endif
	}

	// -----------------------------------------------------------------------
	static void * sGetPointerFromTruePointer(
		void * trueptr)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		return (void *)((BYTE *)trueptr + MEMORY_DEBUG_HEADER_SIZE);
#else
		return trueptr;
#endif
	}

	// -----------------------------------------------------------------------
	static void * sGetTruePointerFromPointer(
		void * ptr)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		return (void *)((BYTE *)ptr - MEMORY_DEBUG_HEADER_SIZE);
#else
		return ptr;
#endif
	}

	// -----------------------------------------------------------------------
	static void sSetDebugSentinel(
		MEMORY_FUNCARGS(void * trueptr,
		MEMORY_SIZE size))
	{
		REF(trueptr);
		REF(size);
		MEMORY_DEBUG_REF(file);

#if DEBUG_MEMORY_ALLOCATIONS
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;
		hdr->line = line;
#ifndef _WIN64
		hdr->file = file;
#endif
		hdr->dwMagic = DEBUG_HEADER_MAGIC;

		MEMORY_DEBUG_FOOTER * ftr = (MEMORY_DEBUG_FOOTER *)((BYTE *)trueptr + MEMORY_DEBUG_HEADER_SIZE + size);
		ftr->dwMagic = DEBUG_FOOTER_MAGIC;
#endif
	}

#if DEBUG_MEMORY_ALLOCATIONS
	// -----------------------------------------------------------------------
	static void sCheckDebugSentinel(
		void * trueptr,
		MEMORY_SIZE size)
	{
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;
		ASSERT(DEBUG_HEADER_MAGIC == hdr->dwMagic);

		MEMORY_DEBUG_FOOTER * ftr = (MEMORY_DEBUG_FOOTER *)((BYTE *)trueptr + MEMORY_DEBUG_HEADER_SIZE + size);
		ASSERT(DEBUG_FOOTER_MAGIC == ftr->dwMagic);
	}
#endif

#if DEBUG_MEMORY_ALLOCATIONS
	// -----------------------------------------------------------------------
	// add the data to the flidx and attach node
	inline void TrackAlloc(
		void * trueptr,
		MEMORY_SIZE size,
		const char * file,
		unsigned int line)
	{
		ASSERT_RETURN(trueptr);
		
		FLIDX_NODE * flidx = flidx_tracker.TrackAlloc(trueptr, size, file, line);

#if DEBUG_SINGLE_ALLOCATIONS
		ALLOC_NODE * anode = alloc_tracker.TrackAlloc(trueptr, flidx, size);
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;
		hdr->alloc_node = anode;
#else
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;
		hdr->flidx_node = flidx;
#endif
	}
#endif

#if DEBUG_MEMORY_ALLOCATIONS
	// -----------------------------------------------------------------------
	// update trackers on free
	inline void TrackFree(
		void * trueptr,
		MEMORY_SIZE size)
	{
		ASSERT_RETURN(trueptr);
		
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;

		
#if DEBUG_SINGLE_ALLOCATIONS
		ALLOC_NODE * anode = hdr->alloc_node;
		ASSERT_RETURN(anode);
		hdr->alloc_node = NULL;
		FLIDX_NODE * flidx = anode->flidx;
		ASSERT_RETURN(flidx);
		flidx_tracker.TrackFree(flidx, anode, size);
		alloc_tracker.TrackFree(anode);
#else
		FLIDX_NODE * flidx = hdr->flidx_node;
		ASSERT_RETURN(flidx);
		hdr->flidx_node = NULL;
		flidx_tracker.TrackFree(flidx, size);
#endif
	}
#endif

#if DEBUG_MEMORY_ALLOCATIONS
	// -----------------------------------------------------------------------
	// update trackers on realloc
	inline void TrackRealloc(
		void * trueptr,
		MEMORY_SIZE oldsize,
		MEMORY_SIZE size,
		const char * file,
		unsigned int line)
	{
		ASSERT_RETURN(trueptr);
		
		MEMORY_DEBUG_HEADER * hdr = (MEMORY_DEBUG_HEADER *)trueptr;

#if DEBUG_SINGLE_ALLOCATIONS
		ALLOC_NODE * anode = hdr->alloc_node;
		ASSERT_RETURN(anode);
		FLIDX_NODE * flidx = anode->flidx;
		ASSERT_RETURN(flidx);
		flidx = flidx_tracker.TrackRealloc(trueptr, flidx, anode, oldsize, size, file, line);
		alloc_tracker.TrackAlloc(trueptr, flidx, anode, size);
		hdr->alloc_node = anode;
#else
		FLIDX_NODE * flidx = hdr->flidx_node;
		ASSERT_RETURN(flidx);
		flidx = flidx_tracker.TrackRealloc(trueptr, flidx, oldsize, size, file, line);
		hdr->flidx_node = flidx;
#endif
	}
#endif

public:
	// -----------------------------------------------------------------------
	void InitBase(
		MEMORY_MANAGEMENT_TYPE pooltype)
	{
		m_eType = pooltype;
		m_InternalFragmentation = 0;
		m_InternalOverhead = 0;
		m_SmallAllocationSize = 0;
		m_LargeAllocationSize = 0;
		m_SmallAllocations = 0;
		m_LargeAllocations = 0;
#if DEBUG_MEMORY_ALLOCATIONS
		m_Name[0] = 0;
		flidx_tracker.Init();
#if DEBUG_SINGLE_ALLOCATIONS
		alloc_tracker.Init();
#endif
#endif
	}

	// -----------------------------------------------------------------------
	void FreeBase(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
#if DEBUG_SINGLE_ALLOCATIONS
		alloc_tracker.Free();
#endif

#if defined(_DEBUG)
		flidx_tracker.TraceUnfreed();
#endif
		flidx_tracker.Free();
#endif
	}

	// -----------------------------------------------------------------------
	void TraceFLIDX(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		PrintForWpp("MEMORY SYSTEM - POOL:%S  MemoryTrace file/line allocation tracker:", m_Name);
		flidx_tracker.TraceFLIDX();
#endif
	}

	// -----------------------------------------------------------------------
	void TraceUnfreed(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		PrintForWpp("MEMORY SYSTEM - POOL:%S  MemoryTrace unfreed:", m_Name);
		flidx_tracker.TraceUnfreed();
#endif
	}

	// -----------------------------------------------------------------------
	void TraceALLOC(
		void)
	{
#if DEBUG_SINGLE_ALLOCATIONS
		PrintForWpp("MEMORY SYSTEM - POOL:%S  MemoryTrace individual allocation tracker:", m_Name);
		flidx_tracker.TraceALLOC();
#endif
	}

	// -----------------------------------------------------------------------
	void TrackWindow(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		flidx_tracker.TrackWindow();
#endif
	}

	// -----------------------------------------------------------------------
	void GetWindow(
		MEMORY_INFO_LINE * window,
		unsigned int size)
	{
		MEMORY_NDEBUG_REF(window);
		MEMORY_NDEBUG_REF(size);

#if DEBUG_MEMORY_ALLOCATIONS
		flidx_tracker.GetWindow(window, size);
#endif
	}

	// -----------------------------------------------------------------------
	virtual void Init(void) = 0;
	virtual void Free(void) = 0;
	virtual void * Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size)) = 0;
	virtual void * AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size)) = 0;
	virtual void * AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment)) = 0;
	virtual void * AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment)) = 0;
	virtual void Free(MEMORY_FUNCARGS(void * ptr)) = 0;
	virtual MEMORY_SIZE Size(void * ptr) = 0;
	virtual void * Realloc(MEMORY_FUNCARGS(void * ptr, MEMORY_SIZE size)) = 0;
	virtual void * ReallocZ(MEMORY_FUNCARGS(void * ptr, MEMORY_SIZE size)) = 0;
	virtual void * AlignedRealloc(MEMORY_FUNCARGS(void * ptr, MEMORY_SIZE size, unsigned int alignment)) = 0;
	virtual void * AlignedReallocZ(MEMORY_FUNCARGS(void * ptr, MEMORY_SIZE size, unsigned int alignment)) = 0;
	virtual void MemoryTrace(void) = 0;
};

// ---------------------------------------------------------------------------
// LARGE allocations (> 32768) we store the size in the allocation container (before the allocation, in a 16 byte header)
// BIN allocations, we store the size in a bit array
// ---------------------------------------------------------------------------
#define MT_SAFE_LOCK(cs, ...)			if (MT_SAFE) { MEM_AUTOLOCK autolock(&cs); __VA_ARGS__ } else { __VA_ARGS__ }
#define MT_SAFE_FAIR_LOCK(cs, ...)		if (MT_SAFE) { MEM_AUTOLOCK_FAIR autolock(&cs); __VA_ARGS__ } else { __VA_ARGS__ }
#define MT_CS_SAFE_LOCK(cs, ...)		if (MT_SAFE) { MEM_AUTOLOCK_CS autolock(&cs); __VA_ARGS__ } else { __VA_ARGS__ }
#define MT_SAFE_READ_LOCK(cs, ...)		if (MT_SAFE) { MEM_AUTOLOCK_READ autolock(&cs); __VA_ARGS__ } else { __VA_ARGS__ }
#define MT_SAFE_WRITE_LOCK(cs, ...)		if (MT_SAFE) { MEM_AUTOLOCK_WRITE autolock(&cs); __VA_ARGS__ } else { __VA_ARGS__ }


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MEMORYPOOL_DYNAMIC_STACK_BASE
//
// Remarks
//  Doesn't support free
/////////////////////////////////////////////////////////////////////////////	
/*
template <class LOCK_TYPE>
struct MEMORYPOOL_DYNAMIC_STACK_BASE : public MEMORYPOOL
{
protected:
	// -----------------------------------------------------------------------
	// 
	struct DS_BUCKET
	{
		void *							ptr;									// ptr to actual data
		MEMORY_SIZE						size;									// size of ptr block
		MEMORY_SIZE						last;									// last allocated offset.  free = size - last
		DS_BUCKET *						next;									// next bucket in list
	};
	BOOL								bAllowFree;								// until this is true, we don't allow frees

	// -----------------------------------------------------------------------
	// track buckets so we can look up the bucket a ptr is in
	VPM_SORTED_ARRAY<void *, DS_BUCKET *, LOCK_TYPE>	arrayByPtr;
	VPM_SORTED_ARRAY<void *, DS_BUCKET *, LOCK_TYPE>	arrayBySize;

	// -----------------------------------------------------------------------
	static void DS_BUCKET_FREE(
		void * key,
		DS_BUCKET * data)
	{
		ASSERT_RETURN(key);
		REF(data);
		VPM_RELEASE_BLOCK(key);
	};

	// -----------------------------------------------------------------------
	static void DS_BUCKET_NULLFREE(
		void * key,
		DS_BUCKET * data)
	{
		REF(key);
		REF(data);
	};

public:
	// -----------------------------------------------------------------------
	void Init(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		ASSERT(MEMORY_DEBUG_HEADER_SIZE == 16);		// required for aligned allocs to work
#endif
		bAllowFree = FALSE;
		arrayByPtr.Init();
		arrayBySize.Init();
	}

	// -----------------------------------------------------------------------
	void Free(
		void)
	{
		arrayBySize.Free(DS_BUCKET_NULLFREE);
		arrayByPtr.Free(DS_BUCKET_FREE);
		MEMORYPOOL::FreeBase();
	}

	// -----------------------------------------------------------------------
	// Size is not supported for this allocator type
	inline MEMORY_SIZE Size(
		void * ptr)
	{
		REF(ptr);
		ASSERT(0);
		return 0;
	}

	// -----------------------------------------------------------------------
	inline void * Alloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Account for additional debug overhead in the allocation
		//
		MEMORY_SIZE sizetoalloc = MEMORYPOOL::sGetSizeToAlloc(size);

		// get a block
		{
			VSA_SAFE_LOCK_WRITE
		

		void * trueptr;
		{
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			// Get a pointer to the beginning of the next unused block
			//
			trueptr = GetUnusedBlock(bin, size);
			ASSERT_RETNULL(trueptr);

			// Keep track of the total allocations from the pool
			// 
			m_SmallAllocationSize += size;
			++m_SmallAllocations;

			// Adjust overhead
			//
			m_InternalOverhead -= bin->size;
		}

#if DEBUG_MEMORY_ALLOCATIONS
		sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
		MEMORYPOOL::TrackAlloc(trueptr, size, file, line);
#endif


		// Return the pointer adjusted for any debug information prepended
		// to the pointer
		//
		return sGetPointerFromTruePointer(trueptr);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AllocZ
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	// Remarks
	//	Zeros the memory before returning it.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AllocZ(
		MEMORY_FUNCARGS(MEMORY_SIZE size))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Do a normal allocation
		//
		void * ptr = Alloc(MEMORY_FUNC_PASSFL(size));
		ASSERT(ptr);

		// Zero the memory
		//
		if (!ptr)
		{
			MemoryTraceAllFLIDX();
		}
        else 
        {
            memclear(ptr, size);
        }
		
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AlignedAlloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AlignedAlloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size,
		unsigned int alignment))
	{
		return sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AlignedAllocZ
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	// Remarks
	//	Zeros the memory before returning it.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AlignedAllocZ(
		MEMORY_FUNCARGS(MEMORY_SIZE size,
		unsigned int alignment))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Do a normal aligned allocation
		//
		void * ptr = sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
		ASSERT_RETNULL(ptr);

		// Zero the memory
		//
		memclear(ptr, size);
		
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	Free
	//
	// Parameters
	//	ptr : The pointer to free
	//
	// Remarks
	//	CRT heap allocated memory which is over the max block size is also released 
	//	through this function.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void Free(
		MEMORY_FUNCARGS(void * ptr))
	{
		if (NULL == ptr)
		{
			return;
		}

		void * trueptr = sGetTruePointerFromPointer(ptr);

		// Get a pointer to the bin and bucket for which the pointer was allocated
		//
		MEMORY_BIN * bin;
		MEMORY_BUCKET * bucket = buckettracker.FindBucket(trueptr, bin);

		
		// If the true pointer was not found in a bucket, it means that it was a large
		// allocation and satisfied by the CRT heap.
		//
		if (!bucket)
		{
			sLargeFree(MEMORY_FUNC_PASSFL(ptr));
			return;
		}
		ASSERT_RETURN(bin);

		// Return the block to the bucket's free list
		//
		{						
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			ASSERT_RETURN(bucket->bin == bin);
#if DEBUG_MEMORY_ALLOCATIONS
			MEMORY_SIZE oldsize = sCheckDebugSentinel(trueptr, bucket);
			MEMORYPOOL::TrackFree(trueptr, oldsize);
#endif

			MEMORY_SIZE externalAllocationSize = bucket->Free(trueptr);			

			// Keep track of how many blocks are currently used in the bin
			//
			--bin->usedBlocks;

			// Adjust the internal fragmentation 
			//
			m_InternalFragmentation -= (bin->size - externalAllocationSize);

			// Keep track of outstanding allocations
			//
			m_SmallAllocationSize -= externalAllocationSize;
			--m_SmallAllocations;

			// Adjust overhead
			//
			m_InternalOverhead += bin->size;
		}

		ptr = NULL;
	}

	// -----------------------------------------------------------------------
	inline void * Realloc(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size))
	{
		MEMORY_SIZE oldsize;
		return sRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
	}

	// -----------------------------------------------------------------------
	inline void * ReallocZ(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size))
	{
		MEMORY_SIZE oldsize;
		ptr = sRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
		ASSERT_RETNULL(ptr);
		if (oldsize < size)
		{
			memclear((BYTE *)ptr + oldsize, (size - oldsize));
		}
		return ptr;
	}

	// -----------------------------------------------------------------------
	inline void * AlignedRealloc(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size,
		unsigned int alignment))
	{
		MEMORY_SIZE oldsize;
		return sAlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, oldsize));
	}

	// -----------------------------------------------------------------------
	inline void * AlignedReallocZ(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size,
		unsigned int alignment))
	{
		MEMORY_SIZE oldsize;
		ptr = sAlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, oldsize));
		ASSERT_RETNULL(ptr);
		if (oldsize < size)
		{
			memclear((BYTE *)ptr + oldsize, (size - oldsize));
		}
		return ptr;
	}

	// -----------------------------------------------------------------------
	void MemoryTrace(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		UINT64 total_used = 0;			// total allocated
		UINT64 total = 0;				// total used

		PrintForWpp("BIN ALLOCATOR - POOL:%S", m_Name);
		PrintForWpp("%3s  %6s  %8s  %4s  %7s  %7s  %9s", "BIN", "SIZE", "BKT_SIZE", "NUM", "ALLOCS", "UNUSED", "MEM");
		for (unsigned int ii = 0; ii < bins.GetCount(); ++ii)
		{
			MEMORY_BIN * bin = &bins[ii];
			MEM_AUTOLOCK_CS autolock(bin->GetLock());
			if (bin->bucketcount <= 0)
			{
				continue;
			}
			MEMORY_BUCKET * curr = NULL;
			MEMORY_BUCKET * next = bin->head;
			unsigned int allocs = 0;
			while (NULL != (curr = next))
			{
				next = curr->next;
				allocs += curr->usedcount;
			}
			total_used += (UINT64)allocs * (UINT64)bin->size;
			UINT64 mem = (UINT64)bin->bucketsize * (UINT64)bin->bucketcount;
			total += mem;
			PrintForWpp("%3u  %6llu  %8llu  %4u  %7u  %7u  %9llu", 
				ii, (UINT64)bin->size, (UINT64)bin->bucketsize, bin->bucketcount, allocs, bin->bucketcount * bin->blockcount - allocs, mem);
		}
		PrintForWpp("  TOTAL USED: %llu/%llu", total_used, total);
#endif
	}
};

MEMORYPOOL_DYNAMIC_STACK_BASE<CS_EmptyLock> g_test;
*/

const MEMORY_BIN_DEF g_DefaultBinDef[] =
{ // size,	gran,  bucketsize
		8, 	   8,    4096,
	   32, 	   8,   16384,
	   64, 	  16,   16384,
	  128, 	  32,   65536,
	  256, 	  64,   65536,	// changing this table requires changing sGetSizeToAlloc
	  512, 	 128,  131072,
	 1024, 	 256,  262144,
	 2048, 	1024,  262144,
	 4096, 	2048,  262144,
	 8192, 	4096,  262144,
	16384, 	8192,  262144,
	32768, 16384,  262144,
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MEMORYPOOL_BIN_BASE
//
// Remarks
//	The MEMORYPOOL_BIN_BASE is a list of pools.  The class contains a list of bins.  Each
//	bin corresponds to a particular block size for which allocations up to the block size will
//	be taken from.  Each bin contains a list of buckets.  Each bucket wraps a chunk of memory
//	that is divided into blocks.  All buckets within the bin's bucket list have the same block
//	size.  New buckets are allocated for the bucket list when all free blocks within the bin's 
//	bucket list are currently used.
//
/////////////////////////////////////////////////////////////////////////////	
template <BOOL MT_SAFE>
struct MEMORYPOOL_BIN_BASE : public MEMORYPOOL
{
public:
	static const unsigned int		MAX_ALLOCS_PER_BUCKET = 1024;				// maximum allocations per bucket

	struct MEMORY_BUCKET;

	// -----------------------------------------------------------------------
	template <typename LOCKTYPE, BOOL MT_SAFE>
	struct MEM_AUTOLOCK_BASE
	{
		LOCKTYPE *					lock;

		MEM_AUTOLOCK_BASE(
			LOCKTYPE * in_lock) : lock(NULL)
		{
			if (MT_SAFE)
			{
				ASSERT_RETURN(in_lock);
				lock = in_lock;
				lock->Enter();
			}
		}

		~MEM_AUTOLOCK_BASE(
			void)
		{
			if (MT_SAFE)
			{
				if (lock)
				{
					lock->Leave();
				}
			}
		}
	};
	typedef MEM_AUTOLOCK_BASE<CCritSectLite, MT_SAFE>		MEM_AUTOLOCK;
	typedef MEM_AUTOLOCK_BASE<CCritSectLiteFair, MT_SAFE>	MEM_AUTOLOCK_FAIR;
	typedef MEM_AUTOLOCK_BASE<CCriticalSection, MT_SAFE>	MEM_AUTOLOCK_CS;

	// -----------------------------------------------------------------------
	template <typename LOCKTYPE, BOOL MT_SAFE>
	struct MEM_RW_AUTOLOCK_READ_BASE
	{
		LOCKTYPE *	lock;

		MEM_RW_AUTOLOCK_READ_BASE(
			LOCKTYPE * in_lock) : lock(NULL)
		{
			if (MT_SAFE)
			{
				ASSERT_RETURN(in_lock);
				lock = in_lock;
				lock->ReadLock();
			}
		}

		~MEM_RW_AUTOLOCK_READ_BASE(
			void)
		{
			if (MT_SAFE)
			{
				if (lock)
				{
					lock->EndRead();
				}
			}
		}
	};
	typedef MEM_RW_AUTOLOCK_READ_BASE<CReaderWriterLock_NS_FAIR, MT_SAFE>	MEM_AUTOLOCK_READ;

	// -----------------------------------------------------------------------
	template <typename LOCKTYPE, BOOL MT_SAFE>
	struct MEM_RW_AUTOLOCK_WRITE_BASE
	{
		LOCKTYPE *	lock;

		MEM_RW_AUTOLOCK_WRITE_BASE(
			LOCKTYPE * in_lock) : lock(NULL)
		{
			if (MT_SAFE)
			{
				ASSERT_RETURN(in_lock);
				lock = in_lock;
				lock->WriteLock();
			}
		}

		~MEM_RW_AUTOLOCK_WRITE_BASE(
			void)
		{
			if (MT_SAFE)
			{
				if (lock)
				{
					lock->EndWrite();
				}
			}
		}
	};
	typedef MEM_RW_AUTOLOCK_WRITE_BASE<CReaderWriterLock_NS_FAIR, MT_SAFE>	MEM_AUTOLOCK_WRITE;

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	MEMORY_BIN_BASE
	//
	// Remarks
	//	A MEMORY_BIN stores buckets (MEMORY_BUCKET) which store free and used blocks which
	//	are all the same size.  An array of these bins make up the "bin memory pool".  
	//
	//	When an allocation needs to be made from the "bin memory pool", the requested size 
	//	is used to look up the bin in the bin array with the smallest "bucket block size" capable 
	//	of satisfying the request.
	//
	/////////////////////////////////////////////////////////////////////////////	
	template <BOOL MT_SAFE>
	class MEMORY_BIN_BASE
	{
		friend struct MEMORYPOOL_BIN_BASE<MT_SAFE>;
		template<BOOL MT_SAFE> struct MEMORYPOOL_BIN_ARRAY;
		friend struct MEMORYPOOL_BIN_ARRAY<MT_SAFE>;

		// Only used during debug memory trace
		//
		CCriticalSection			cs;

	public:
			
		// A list of MEMORY_BUCKETs
		//
		MEMORY_BUCKET *				head;
		MEMORY_BUCKET *				tail;
		unsigned int				bucketcount;

		// below here is non volatile data

		// All buckets in the bin contain blocks of this size
		//
		MEMORY_SIZE					size;										// max allocation size for this bin

		// The size in bytes that will be allocated for each bucket in the bin.  This is a 
		// multiple of the allocation granularity which is a multiple of the page size.
		//
		MEMORY_SIZE					bucketsize;									// total size of a bucket

		// All bins are contained in a bin array.  This is the bin's index into that array
		//
		unsigned int				index;										// index of bin

		// The number of blocks that fit in each bucket in the bin
		//
		unsigned int				blockcount;									// max allocations count for this bin

		// In order to be able to return the size of an allocation, each block within the bucket
		// has a corresponding external allocation size stored after the block data itself.  The 
		// size of the size field depends on the size of the block, using the least amount of 
		// bits necessary.  This member stores the number of bits.
		//
		unsigned int				sizebits;									// bits needed to store the size

		// The number of bytes that are wasted in overhead for each bucket including
		// the part of the bucket's block memory that is too small for a block and also the
		// overhead of the external allocation sizes
		//
		unsigned int				overheadPerBucket;

		// The number of used blocks in the bin
		//
		unsigned int				usedBlocks;

	public:
		
		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Init
		//
		/////////////////////////////////////////////////////////////////////////////	
		void Init(
			void)
		{
			if (MT_SAFE)
			{
				InitCriticalSection(cs);
			}
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Free
		//
		/////////////////////////////////////////////////////////////////////////////	
		void Free(
			void)
		{
			if (MT_SAFE)
			{
				cs.Free();
			}
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Add
		//
		// Parameters
		//	bucket : A pointer to the bucket to be added to the bin's bucket list
		//
		// Remarks
		//	buckets only get added to the bin's bucket list when there are no more available blocks
		//	in the bin's currently allocated buckets.  The bucket gets added to the head of the list
		//
		/////////////////////////////////////////////////////////////////////////////	
		inline void Add(
			MEMORY_BUCKET * bucket)
		{
			// Add the bucket to the head of the list
			//
			if (head)
			{
				bucket->next = head;
				bucket->next->prev = bucket;
			}
			head = bucket;
			bucket->prev = NULL;

			// Adjust the tail pointer
			//
			if (NULL == tail)
			{
				tail = bucket;
			}

			// Adjust the count
			//
			++bucketcount;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	MoveToTail
		//
		// Parameters
		//	bucket : A pointer to the bucket to be moved to the back of the bucket list
		//
		// Remarks
		//	Buckets get added to the tail of the bin's bucket list when all blocks have been externally
		//	allocated from the bucket.
		//
		/////////////////////////////////////////////////////////////////////////////	
		inline void MoveToTail(
			MEMORY_BUCKET * bucket)
		{
			DBG_ASSERT(head == bucket);
			if (tail == bucket)
			{
				return;
			}
			ASSERT_RETURN(bucket->next);

			// Move the bucket to the back of the list
			//
			head = bucket->next;
			bucket->next->prev = NULL;
			bucket->prev = tail;
			bucket->prev->next = bucket;
			bucket->next = NULL;
			tail = bucket;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	MoveToHead
		//
		// Parameters
		//	bucket : A pointer to the bucket to be moved to the head of the list
		//
		// Remarks
		//	Buckets get moved to the head of the list when an external allocation is released back
		//	into the bin.  Moving the bucket to the head of the list provides some cache locality.
		//
		/////////////////////////////////////////////////////////////////////////////	
		inline void MoveToHead(
			MEMORY_BUCKET * bucket)
		{
			if (head == bucket)
			{
				return;
			}
			
			if (bucket->next)
			{
				DBG_ASSERT(tail != bucket);
				bucket->next->prev = bucket->prev;
			}
			else
			{
				DBG_ASSERT(tail == bucket);
				tail = bucket->prev;
			}
			ASSERT_RETURN(bucket->prev);
			bucket->prev->next = bucket->next;

			bucket->prev = NULL;
			bucket->next = head;
			if (bucket->next)
			{
				bucket->next->prev = bucket;
			}
			head = bucket;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	CalcInfo
		//
		// Parameters
		//	in_size : [in/out] Contains the bucket block size of the last bin that was created.  Is set
		//		to the size of the next bucket to create.
		//	in_granularity : The number of bytes to increment "size" by when determining successive
		//		bucket block sizes
		//	in_bucketsize : The number of bytes that the next bucket will allocate for it's block memory
		//
		// Returns
		//	TRUE if the method succeeds, FALSE otherwise
		//
		// Remarks
		//	This method calculates the block size and count for any bucket created for the bin.  It 
		//	takes into account overheaded needed to store the external allocation sizes within the
		//	bucket's block memory. 
		//
		//	If the bin measurements are such that the next higher block sized bin would hold the same
		//	amount of blocks in the same amount of memory, the method uses the next higher 
		//	measurements instead.
		//
		/////////////////////////////////////////////////////////////////////////////	
		BOOL CalcInfo(
			MEMORY_SIZE & in_size,
			MEMORY_SIZE in_granularity,
			MEMORY_SIZE in_bucketsize)
		{
			ASSERT_RETFALSE(in_granularity > 0);
			ASSERT_RETFALSE(in_bucketsize > 0);

			// Increment the size of the previous bin to get the current bin's bucket block size
			//
			in_size += in_granularity;

			// Figure out a loose estimate at the count not taking into account any tracking
			// overhead
			//
			unsigned int count = in_bucketsize / in_size;
			ASSERT_RETFALSE(count > 0);
			ASSERT_RETFALSE(count <= MAX_ALLOCS_PER_BUCKET);

			// At the end of the bucket's block memory is stored an array of external allocation
			// sizes (one for each block).  These sizes only need as many bits as necessary to store
			// the maximum size of a block (-1 because we never have size 0 and can use 0 to represent 1).
			// Figure out how many bits are necessary for each size value
			//
			unsigned int in_sizebits = BITS_TO_STORE(in_size - 1);

			// Reduce the count to make room for the external allocation size array
			//
			while (count * in_size + ((in_sizebits * count) + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE > in_bucketsize)
			{
				--count;
			}
			ASSERT_RETFALSE(count > 0);

			MEMORY_SIZE increment = MIN(in_granularity, (MEMORY_SIZE)MAX_ALIGNMENT_SUPPORTED);
			// modify size to fit

			// Figure out if the next potential bin size would fit in the same amount of memory with the same number
			// of blocks.  If so, create the bin based on that next size
			//
			while (1)
			{
				MEMORY_SIZE trysize = in_size + increment;
				unsigned int trysizebits = BITS_TO_STORE(NEXTPOWEROFTWO(in_size - 1));
				if ((count * trysize + ((trysizebits * count) + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE) > in_bucketsize)
				{
					break;
				}
				in_size = trysize;
				in_sizebits = trysizebits;
				ASSERT(in_size <= (MEMORY_SIZE)(1 << in_sizebits));
			}

			size = in_size;
			bucketsize = in_bucketsize;
			blockcount = count;
			sizebits = in_sizebits;
			
			return TRUE;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	GetLock
		//
		// Returns
		//	A pointer to the bin's critical section
		//
		// Remarks
		//	This method is called when allocating/freeing memory from the bin
		//
		/////////////////////////////////////////////////////////////////////////////	
		inline CCriticalSection * GetLock(
			void)
		{
			if (MT_SAFE)
			{
				return &cs;
			}
			else
			{
				return NULL;
			}
		}
		
	};
	typedef MEMORY_BIN_BASE<MT_SAFE>	MEMORY_BIN;

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	MEMORY_BIN_ARRAY
	//
	// Remarks
	//	The MEMORY_BIN_ARRAY is held by the "bin-allocator memory pool" and simply
	//	contains an array of bin pointers sorted by their bucket block size.  The array
	//	is followed by an array of bucket block sizes used to search for a bucket given
	//	a particular external allocation size request.
	//
	/////////////////////////////////////////////////////////////////////////////	
	template <BOOL MT_SAFE>
	struct MEMORY_BIN_ARRAY
	{
		friend struct MEMORYPOOL_BIN_BASE<MT_SAFE>;

	public:

		// An array of bin pointers sorted by their bucket block size
		//
		MEMORY_BIN *				binlist;									// desc and bin are aggregated, list of bins

		// An array of pointers to the bin bucket block sizes used to search for 
		// an appropriate bucket given an external allocation request size
		//
		MEMORY_SIZE *				sizelookup;									// compact size list for cached size lookup

		// The number of bins stored in the binlist.  Also the number of 
		// size pointers stored in the sizelookup array
		//
		unsigned int				numbins;									// number of bin definitions

		// -------------------------------------------------------------------
		void Validate(
			void) const
		{
#if 0
			for (unsigned int ii = 0; ii < numbins; ++ii)
			{
				binlist[ii].Validate();
			}
#endif
		}

		// -------------------------------------------------------------------
		BOOL VerifyBinInfo(
			MEMORY_BIN * bins,
			unsigned int count) const
		{
			for (unsigned int ii = 0; ii < count; ++ii)
			{
				ASSERT_RETFALSE(bins[ii].index == ii);
				if (ii != 0)
				{
					ASSERT_RETFALSE(bins[ii - 1].size < bins[ii].size);
				}
				if (bins[ii].bucketsize >= MEM_ALLOCATION_GRANULARITY)
				{
					ASSERT_RETFALSE(bins[ii].bucketsize % MEM_ALLOCATION_GRANULARITY == 0);
				}
				else
				{
					ASSERT_RETFALSE(MEM_ALLOCATION_GRANULARITY % bins[ii].bucketsize == 0);
				}
				ASSERT_RETFALSE(bins[ii].size <= (MEMORY_SIZE)(1 << bins[ii].sizebits));
				ASSERT_RETFALSE(bins[ii].blockcount <= bins[ii].bucketsize / bins[ii].size);
				ASSERT_RETFALSE(bins[ii].blockcount <= MAX_ALLOCS_PER_BUCKET);
			}
			return TRUE;
		}

	public:
		
		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Init
		//
		// Parameters
		//	bindef : A pointer to the MEMORY_BIN_DEFs that define how the bins should be created
		//	count : The number of items in the bindef list
		//
		// Returns
		//	TRUE if the method succeeds, FALSE otherwise
		//
		// Remarks
		//	This function generates the bins in the array from the bin definitions.
		//
		/////////////////////////////////////////////////////////////////////////////				
		BOOL Init(
			const MEMORY_BIN_DEF * bindef,
			unsigned int count)
		{
			// Allocate some MEMORY_BINs on the stack for assembly purposes.
			// The MEMORY_BIN_DEF data will be assembled into this "temporary"
			// bin space and then copied into the storage space in the MEMORY_BIN_ARRAY
			//
			static const unsigned int MAX_BINS = 512;
			MEMORY_BIN tempBins[MAX_BINS];				

			BIN_TRACE_1("%3s  %5s  %5s  %4s  %7s  %4s\n", "idx", "size", "align", "cnt", "binsize", "waste");

			// Iterate over all the argument MEMORY_BIN_DEFs, adding
			// them to the "temporary" stack-allocated bins.
			//
			unsigned int bin = 0; // This indexes into the temp bin array and gets incremented as we build out the temp bin array
			unsigned int def = 0; // This indexes into the bin definition array and gets incremented as we iterate over that array

			// "size" is the bucket size of the last bin that was created.  It is an in/out parameter to CalcInfo and always 
			// gets incremented
			//
			MEMORY_SIZE size = 0;

			// We continue to create bins until we create a bin of size equal to the that defined in the
			// last definition in the array
			//
			while (size < bindef[count - 1].size)
			{
				// If we've created 512 bins, something has gone wrong
				//
				ASSERT_RETFALSE(bin < MAX_BINS);

				// Stop making bins for the current definition line when the next potential bin's
				// bucket block size has reached the size defined by the next definition line
				//
				while (def < count - 1)
				{
					// Make sure that successive bin definitions have increasing block sizes
					//
					ASSERT_RETFALSE(def == 0 || bindef[def].size > bindef[def-1].size);

					// If the next bin to be created for the current definition line is the same block size
					// as the next definition line...
					//
					if (bindef[def + 1].size <= size)
					{
						// Skip to the next definition
						//
						++def;
						continue;
					}

					// Otherwise continue creating a bin for the current definition
					//
					break;
				}

				// Prepare the bin for initialization
				//
				structclear(tempBins[bin]);
				tempBins[bin].index = bin;

				// Calculate the size of the next bin given the size of the previous bin, the current definition line's granularity and bucketsize (memory available per bucket)
				//
				ASSERT_RETFALSE(tempBins[bin].CalcInfo(size, bindef[def].granularity, bindef[def].bucketsize));

				// Figure out how much memory is being wasted for each bucket that gets allocated in the bin
				//
				MEMORY_SIZE waste = tempBins[bin].bucketsize - (tempBins[bin].size * tempBins[bin].blockcount) - (((tempBins[bin].sizebits * tempBins[bin].blockcount) + 7) / 8);
				REF(waste);

				tempBins[bin].overheadPerBucket = tempBins[bin].bucketsize - (tempBins[bin].size * tempBins[bin].blockcount);				

				// Determine the byte alignment for the bucket block size
				//
				unsigned int align = 0;
				for (unsigned int ii = 31; ii > 0; --ii)
				{
					if (tempBins[bin].size % (1 << ii) == 0)
					{
						align = (1 << ii);
						break;
					}
				}
				REF(align);
				
				BIN_TRACE_1("%3d  %5I64d  %5d  %4d  %7I64d  %4I64d\n", bin, (INT64)tempBins[bin].size, align, tempBins[bin].blockcount, (INT64)tempBins[bin].bucketsize, (INT64)waste);

				// Done building the current bin.  Move on to the next
				//
				++bin;
			}

			VerifyBinInfo(tempBins, bin);

			// Allocate memory to copy from the temp bins into the bin array.
			// Also allocate memory just after the bin list for an array of bin
			// sizes which will be used to get a bin for a given allocation size
			// request.
			//			
			numbins = bin;
			MEMORY_SIZE allocsize = (sizeof(MEMORY_BIN) * numbins);		// bins
			allocsize += (sizeof(MEMORY_SIZE) * numbins);				// sizelookup
			binlist = (MEMORY_BIN *)VPM_GET_BLOCK(allocsize);			
			CHECK_OUT_OF_MEMORY(binlist, allocsize);

			// Copy from the temp bins into the bin array
			//
			memcpy(binlist, tempBins, sizeof(MEMORY_BIN) * numbins);

			// Set sizelookup to point to the memory just past all the bins.  This
			// will be used later to binary search through the bin list when looking
			// for a bin to allocate from given a particular size
			//
			sizelookup = (MEMORY_SIZE *)(binlist + numbins);

			// Initialize all the bins and copy the size of the bin into the
			// bin size array
			//
			for (unsigned int ii = 0; ii < numbins; ++ii)
			{
				binlist[ii].Init();
				sizelookup[ii] = binlist[ii].size;
			}

			return TRUE;
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			if (binlist)
			{
				for (unsigned int ii = 0; ii < numbins; ++ii)
				{
					binlist[ii].Free();
				}
				VPM_RELEASE_BLOCK(binlist);
			}
			binlist = NULL;
			numbins = 0;
			sizelookup = 0;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Add
		//
		// Parameters
		//	bucket : A pointer to the bucket to add to the array
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline void Add(
			MEMORY_BUCKET * bucket)
		{
			unsigned int index = bucket->bin->index;
			
			binlist[index].Add(bucket);
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	GetBySize
		//
		// Parameters
		//	size : The number of bytes requested for which a MEMORY_BIN should be returned.
		//
		// Returns
		//	A pointer to a MEMORY_BIN that contains blocks large enough to satisfy the requested
		//	allocation size.  Returns NULL if no MEMORY_BIN could fulfill the request.
		//
		// Remarks
		//	No need to lock the array for this operation.  Returns the smallest bin that can
		//	satisfy the request.
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline MEMORY_BIN * GetBySize(
			MEMORY_SIZE size) const
		{
			// Do a binary search through the bin array
			//		
			MEMORY_SIZE * min = sizelookup;
			MEMORY_SIZE * max = sizelookup + numbins;
			MEMORY_SIZE * ii;
			while (min < max)
			{
				ii = min + (max - min) / 2;
				if (*ii > size)
				{
					max = ii;
				}
				else if (*ii < size)
				{
					min = ii + 1;
				}
				else
				{
					return &binlist[(ii - sizelookup)];
				}
			}
			if (max < sizelookup + numbins)
			{
				return &binlist[(max - sizelookup)];
			}
			return NULL;
		}

		// -------------------------------------------------------------------
		inline MEMORY_BIN & operator[](
			unsigned int index) const
		{
			DBG_ASSERT(index < numbins);
			return binlist[index];
		}

		// -------------------------------------------------------------------
		inline unsigned int GetCount(
			void)
		{
			return numbins;
		}
	};
	MEMORY_BIN_ARRAY<MT_SAFE>		bins;
	
	// -----------------------------------------------------------------------
	// instance of a memory bucket OR a free allocation
	struct MEMORY_BUCKET_HDR
	{
		void *						ptr;										// pointer to actual data
		MEMORY_BUCKET *				next;										// next, but if this value == 01, then we're not a MEMORY_BUCKET, and the size is next >> 1
	};

	// -----------------------------------------------------------------------
	struct MEMORY_BUCKET : public MEMORY_BUCKET_HDR
	{
		MEMORY_BUCKET *				prev;
		MEMORY_BIN *				bin;
		unsigned int				usedcount;									// number of allocated blocks

		// This is a bitset where each bit corresponds to a block within the bucket memory.
		// If the bit is set, it means that the block is currently available for external allocation
		//
		DWORD						usedflags[MAX_ALLOCS_PER_BUCKET/BITS_PER_DWORD];	// which blocks in the bucket have been allocated (32 bytes)  (1=available)

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	GetSizeArray
		//
		// Parameters
		//	bytes : [out] A reference to an int that will be set to the size in bytes of the external
		//		allocation array.
		//
		// Returns
		//	A pointer to the beginning of the external allocation size array.  This array of sizes
		//	is stored at the end of each bucket's memory and facilitates getting the size from 
		//	a pointer allocated from the bucket.
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline BYTE * GetSizeArray(
			unsigned int & bytes) const
		{
			// Figure out how many bytes are currently reserved in the block's memory for the 
			// external allocation size array
			//
			bytes = ROUNDUP(bin->sizebits * bin->blockcount, (unsigned int)BITS_PER_BYTE) / BITS_PER_BYTE;

			// Return a pointer to the beginning of the external allocation size array
			//
			return (BYTE *)ptr + bin->bucketsize - bytes;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	FreeBlock
		//
		// Parameters
		//	block : The index of the block to mark as unused.
		//
		// Returns
		//	FALSE if it fails, otherwise the external allocation size of the block
		//
		// Remarks
		//	This function sets the usedflag for the block to unused
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline MEMORY_SIZE FreeBlock(
			unsigned int block)
		{
			// Figure out which bit to set in the usedflags to indicate
			// that the block is available
			//
			unsigned int dw = block / BITS_PER_DWORD;
			unsigned int bit = block % BITS_PER_DWORD;
			
			DBG_ASSERT_RETFALSE(dw < arrsize(usedflags));

			// Verify that the bit is not currently set, indicating that
			// it is used
			//
			ASSERT_RETFALSE(!MEM_TEST_BIT(usedflags[dw], bit));

			// Grab the size of the external allocation
			//
			MEMORY_SIZE externalAllocationSize = GetAllocationSize(block);
			
			// Set the bit, indicating that it is available
			//
			MEM_SET_BIT(usedflags[dw], bit);


			return externalAllocationSize;
		}

		// -------------------------------------------------------------------
		inline BOOL BlockIsUsed(
			unsigned int block)
		{
			unsigned int dw = block / BITS_PER_DWORD;
			unsigned int bit = block % BITS_PER_DWORD;
			
			DBG_ASSERT_RETFALSE(dw < arrsize(usedflags));
			
			return (!MEM_TEST_BIT(usedflags[dw], bit));
		}

	public:
		// -------------------------------------------------------------------
		BOOL Init(
			void * in_ptr,
			MEMORY_BIN * in_bin)
		{
			ptr = in_ptr;														// initialize before returning
			ASSERT_RETFALSE(ptr);
			next = prev = NULL;
			usedcount = 0;
			bin = in_bin;

			// set used flags
			unsigned int blockcount = bin->blockcount;
			ASSERT_RETFALSE(blockcount <= arrsize(usedflags) * BITS_PER_DWORD);
			unsigned int completecount = blockcount / BITS_PER_DWORD;
			unsigned int fractionalcount = blockcount % BITS_PER_DWORD;
			for (unsigned int ii = 0; ii < completecount; ++ii)
			{
				usedflags[ii] = 0xffffffff;
			}
			for (unsigned int ii = completecount; ii < arrsize(usedflags); ++ii)
			{
				usedflags[ii] = 0;
			}
			if (0 != fractionalcount)
			{
				usedflags[completecount] = (1 << fractionalcount) - 1;
			}
			return TRUE;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	CalcBlockFromPointer
		//
		// Parameters
		//	in_ptr : A pointer to the block within the bucket
		//
		// Returns
		//	The index of the block within the bucket's memory
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline unsigned int CalcBlockFromPointer(
			void * in_ptr)
		{
			// Calculate the distance in bytes from the beginning of the bucket's memory
			// to the supplied pointer
			//
			MEMORY_SIZE diff = (MEMORY_SIZE)((BYTE *)in_ptr - (BYTE *)ptr);			
			ASSERT_RETINVALID(diff < bin->bucketsize);
			ASSERT_RETINVALID(diff % bin->size == 0);

			// Calculate the index of the block
			//
			unsigned int block = (unsigned int)(diff / bin->size);
			ASSERT_RETINVALID(block < bin->blockcount);
			
			return block;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	Free
		//
		// Parameters
		//	in_ptr : A pointer to the block to mark as unused
		//
		// Returns
		//	The size of the external allocation for which the in_ptr is being free'd.
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline MEMORY_SIZE Free(
			void * in_ptr)
		{
			// Verify that there are externally allocated blocks in the bucket
			//
			ASSERT_RETFALSE(usedcount > 0);

			// Figure out the offset of the block within the bucket's memory
			//
			unsigned int block = CalcBlockFromPointer(in_ptr);

			// Mark the block as unused
			//
			MEMORY_SIZE externalAllocationSize = FreeBlock(block);			
			ASSERT_RETFALSE(externalAllocationSize);			

			// Keep track of how many blocks are used in the bucket
			//
			--usedcount;

			// Move the bucket to the head of the bin's bucket list to increase
			// memory locality
			//
			bin->MoveToHead(this);

			BIN_TRACE_0("Free block:  Ptr:%p\n", ptr);

			return externalAllocationSize;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	GetAndSetFirstUnusedBlock
		//
		// Returns
		//	-1 if there are no free blocks in the bucket, otherwise it returns the index of the next
		//	block available for external allocation.
		//
		// Remarks
		//	The method also marks the block as used in the usedflags array
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline unsigned int GetAndSetFirstUnusedBlock(
			void)
		{
			// blocks is the number of blocks available for external allocation within the bucket
			//
			unsigned int blocks = bin->blockcount;

			// dwords is the number of DWORDs being used in the usedflags array to represent
			// which blocks are currently being used.
			//
			unsigned int dwords = ROUNDUP(blocks, (unsigned int)BITS_PER_DWORD) / BITS_PER_DWORD;

			// Iterate over all the DWORDs in the usedflags array
			//
			DWORD * cur = usedflags;
			DWORD * end = usedflags + dwords;
			while (cur < end)
			{
				// If none of the bits in the DWORD are set, none of those blocks
				// are available, so skip to the next DWORD.
				//
				if (*cur == 0)
				{
					++cur;
					continue;
				}

				// Find the least significant bit in the DWORD that is set
				//
				unsigned int bit = LEAST_SIGNIFICANT_BIT((unsigned int)*cur);
				ASSERT(bit < BITS_PER_DWORD);

				// Compute the bit's offset in bit-count from the start of the usedflags array
				//
				unsigned int retval = (unsigned int)(cur - usedflags) * BITS_PER_DWORD + bit;
				ASSERT(retval < blocks);

				// Clear the bit in the usedflags to indicate that the block is now used in the bucket
				//
				MEM_CLEAR_BIT(*cur, bit);

				// Return the offset of the bit in the usedflags array which is also the index of the
				// block within the bucket's block memory
				//
				return retval;
			}

			// There are no free blocks in the bucket
			//
			return (unsigned int)-1;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	SetAllocationSize
		//
		// Parameters
		//	block : The index of the block within the bucket's memory for which the size will be set
		//	size : The size in bytes for which the block's 
		//
		// Remarks
		//	Decreases the ref count for the ASYNC_FILE, closing the file handle if it reaches zero.
		//
		/////////////////////////////////////////////////////////////////////////////		
		inline void SetAllocationSize(
			unsigned int block,
			MEMORY_SIZE size)
		{
			// Get a pointer to the beginning of the external allocation array along with the
			// size of the array
			//
			unsigned int bytes;
			BYTE * sizearray = GetSizeArray(bytes);

			// Set the value within the external allocation size array to the size of the actual
			// external allocation
			//
			MEM_SET_BBUF_VALUE(sizearray, bytes, block * bin->sizebits, bin->sizebits, (unsigned int)(size - 1));
		}

		// -------------------------------------------------------------------
		inline MEMORY_SIZE GetAllocationSize(
			unsigned int block)
		{
			ASSERT_RETZERO(BlockIsUsed(block));

			unsigned int bytes;
			BYTE * sizearray = GetSizeArray(bytes);

			return MEM_GET_BBUF_VALUE(sizearray, bytes, block * bin->sizebits, bin->sizebits) + 1;
		}
	};

	#define BUCKET_TRACKER_CSLITE	0
	#define BUCKET_TRACKER_CSLTFA	1
	#define BUCKET_TRACKER_RWLOCK	2
	#define BUCKET_TRACKER_CRSECT	3
	#define BUCKET_TRACKER_LOCK		BUCKET_TRACKER_RWLOCK

#if BUCKET_TRACKER_LOCK == BUCKET_TRACKER_CSLITE
	#define MBT_CS					CCritSectLite
	#define MBT_SAFE_READ_LOCK		MT_SAFE_LOCK
	#define MBT_SAFE_WRITE_LOCK		MT_SAFE_LOCK
#elif BUCKET_TRACKER_LOCK == BUCKET_TRACKER_CSLTFA
	#define MBT_CS					CCritSectLiteFair
	#define MBT_SAFE_READ_LOCK		MT_SAFE_FAIR_LOCK
	#define MBT_SAFE_WRITE_LOCK		MT_SAFE_FAIR_LOCK
#elif BUCKET_TRACKER_LOCK == BUCKET_TRACKER_RWLOCK
	#define MBT_CS					CReaderWriterLock_NS_FAIR
	#define MBT_SAFE_READ_LOCK		MT_SAFE_READ_LOCK
	#define MBT_SAFE_WRITE_LOCK		MT_SAFE_WRITE_LOCK
#else 
	#define MBT_CS					CCriticalSection
	#define MBT_SAFE_READ_LOCK		MT_CS_SAFE_LOCK
	#define MBT_SAFE_WRITE_LOCK		MT_CS_SAFE_LOCK
#endif

	// -----------------------------------------------------------------------
	// track memory buckets so we can look up the bucket a ptr is in
	template <BOOL MT_SAFE>
	struct BUCKET_TRACKER
	{
	protected:
		struct MBT_NODE
		{
			void *					ptr;										// pointer
			MEMORY_BUCKET *			bucket;										// bucket holding the pointer
		};

		MBT_CS						cs;
		MBT_NODE *					list;										// sorted array of MBT_NODE
		unsigned int				size;										// allocated size of list (number of elements)
		unsigned int				count;										// number of elements in the list
		unsigned int				NODES_PER_PAGE;								// number of nodes per page

		// -------------------------------------------------------------------
		inline unsigned int FindInsertionPoint(
			void * ptr)
		{
			MBT_NODE * min = list;
			MBT_NODE * max = list + count;
			MBT_NODE * ii;
			while (min < max)
			{
				ii = min + (max - min) / 2;
				if ((BYTE *)ii->ptr > (BYTE *)ptr)
				{
					max = ii;
				}
				else if ((BYTE *)ii->ptr < (BYTE *)ptr)
				{
					min = ii + 1;
				}
				else
				{
					return (unsigned int)(ii - list);
				}
			}
			if (max < list + count)
			{
				return (unsigned int)(max - list);
			}
			return count;
		}

		// -------------------------------------------------------------------
		inline unsigned int FindExact(
			void * ptr)
		{
			MBT_NODE * min = list;
			MBT_NODE * max = list + count;
			MBT_NODE * ii;
			while (min < max)
			{
				ii = min + (max - min) / 2;
				if ((BYTE *)ii->ptr > (BYTE *)ptr)
				{
					max = ii;
				}
				else if ((BYTE *)ii->ptr < (BYTE *)ptr)
				{
					min = ii + 1;
				}
				else
				{
					return (unsigned int)(ii - list);
				}
			}
			return count;
		}

		// -------------------------------------------------------------------
		inline unsigned int FindEqualOrSmaller(
			void * ptr)
		{
			MBT_NODE * min = list;
			MBT_NODE * max = list + count;
			MBT_NODE * ii;
			while (min < max)
			{
				ii = min + (max - min) / 2;
				if ((BYTE *)ii->ptr > (BYTE *)ptr)
				{
					max = ii;
				}
				else if ((BYTE *)ii->ptr < (BYTE *)ptr)
				{
					min = ii + 1;
				}
				else
				{
					return (unsigned int)(ii - list);
				}
			}
			if (max > list)
			{
				return (unsigned int)((max - 1) - list);
			}
			return count;
		}

		// -------------------------------------------------------------------
		inline BOOL sInsert(
			MEMORY_BUCKET * bucket)
		{
			ASSERT_RETFALSE(bucket);

			unsigned int ip = FindInsertionPoint(bucket->ptr);

			if (count < size)
			{
				if (ip < count)
				{
					memmove(list + ip + 1, list + ip, sizeof(MBT_NODE) * (count - ip));
				}
				++count;
				list[ip].ptr = bucket->ptr;
				list[ip].bucket = bucket;
				return TRUE;
			}

			unsigned int newsize = size + NODES_PER_PAGE;
			MBT_NODE * newlist = (MBT_NODE *)VPM_GET_BLOCK(sizeof(MBT_NODE) * newsize);
			if (NULL == newlist)
			{
				return FALSE;
			}
			
			if (list)
			{
				if (ip > 0)
				{
					memcpy(newlist, list, sizeof(MBT_NODE) * ip);
				}
				if (ip < count)
				{
					memcpy(newlist + ip + 1, list + ip, sizeof(MBT_NODE) * (count - ip));
				}
				VPM_RELEASE_BLOCK(list);
			}
			list = newlist;
			size = newsize;
			++count;
			list[ip].ptr = bucket->ptr;
			list[ip].bucket = bucket;
			return TRUE;
		}

		// -------------------------------------------------------------------
		inline BOOL sRemove(
			MEMORY_BUCKET * bucket)
		{
			ASSERT_RETFALSE(bucket);

			unsigned int idx = FindExact(bucket->ptr);
			ASSERT_RETFALSE(idx < count);

			if (idx < count - 1)
			{
				memmove(list + idx, list + idx + 1, sizeof(MBT_NODE) * ((count - 1) - idx));
			}
			--count;
			return TRUE;
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	sFindBucket
		//
		// Parameters
		//	ptr : A pointer for which to return the bin and bucket from which it was taken
		//	bin : [out] A reference to a pointer that will point to the bin from which the pointer was taken
		//
		// Returns
		//	A pointer to the bucket from which the pointer was taken or NULL if not found.
		//
		/////////////////////////////////////////////////////////////////////////////
		inline MEMORY_BUCKET * sFindBucket(
			void * ptr,
			MEMORY_BIN * & bin)
		{
			// Find the index of the bucket which contains the pointer
			//
			unsigned int idx = FindEqualOrSmaller(ptr);
			if (idx >= count)
			{
				return NULL;
			}

			// Get the bucket pointer
			//
			MEMORY_BUCKET * bucket = list[idx].bucket;

			// Make sure the pointer isn't beyond the end of the bucket's memory
			//
			if ((BYTE *)ptr >= (BYTE *)bucket->ptr + bucket->bin->blockcount * bucket->bin->size)
			{
				return NULL;
			}

			// Return the bin and the bucket
			//
			bin = bucket->bin;
			return bucket;
		}

	public:
		// -------------------------------------------------------------------
		void Init(
			void)
		{
			InitCriticalSection(cs);
			list = NULL;
			size = 0;
			count = 0;
			NODES_PER_PAGE = MEM_PAGE_SIZE / sizeof(MBT_NODE);
			ASSERT(NODES_PER_PAGE > 0);
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			if (list)
			{
				for (unsigned int ii = 0; ii < count; ++ii)
				{
					MEMORY_BUCKET * bucket = list[ii].bucket;
					ASSERT_CONTINUE(bucket);
					VPM_RELEASE_BLOCK(bucket->ptr);
				}
				VPM_RELEASE_BLOCK(list);
				list = NULL;
			}
			size = 0;
			count = 0;
			cs.Free();
		}

		// -------------------------------------------------------------------
		inline BOOL Insert(
			MEMORY_BUCKET * bucket)
		{
			ASSERT_RETFALSE(bucket);

			MBT_SAFE_WRITE_LOCK(cs, return sInsert(bucket););
		}

		// -------------------------------------------------------------------
		inline BOOL Remove(
			MEMORY_BUCKET * bucket)
		{
			ASSERT_RETFALSE(bucket);
			
			MBT_SAFE_WRITE_LOCK(cs, return sRemove(bucket););
		}

		/////////////////////////////////////////////////////////////////////////////
		//
		// Name
		//	FindBucket
		//
		// Parameters
		//	ptr : A pointer for which to return the bin and bucket from which it was taken
		//	bin : [out] A reference to a pointer that will point to the bin from which the pointer was taken
		//
		// Returns
		//	A pointer to the bucket from which the pointer was taken or NULL if not found.
		//
		/////////////////////////////////////////////////////////////////////////////
		inline MEMORY_BUCKET * FindBucket(
			void * ptr,
			MEMORY_BIN * & bin)
		{
			ASSERT_RETNULL(ptr);

			MBT_SAFE_READ_LOCK(cs, return sFindBucket(ptr, bin););
		}
	};
	BUCKET_TRACKER<MT_SAFE>			buckettracker;

	// -----------------------------------------------------------------------
	// serve & recycle MEMORY_BUCKET containers
	template <BOOL MT_SAFE>
	struct BUCKET_STORE
	{
	protected:
		struct BUCKET_BLOCK_HDR													// singly linked list of all allocated bucket blocks
		{
			BUCKET_BLOCK_HDR *		next;
		};
		struct BUCKET_BLOCK : public BUCKET_BLOCK_HDR							// each bucket block aggregates buckets in a MEM_PAGE_SIZE block
		{
			MEMORY_BUCKET 			buckets[1];									// easy access to 1st bucket in block
		};

		unsigned int				BUCKETS_PER_BUCKET_BLOCK;					// actual number of bucket in block

		CCritSectLite				cs;											// everything below this is protected
		MEMORY_BUCKET *				garbage;									// singly linked recycled bucket
		BUCKET_BLOCK *				bucketblocklist;							// list of all bucket blocks
		unsigned int				bucketblocklistfirst;						// first unused bucket in head bucket block

		// -------------------------------------------------------------------
		inline MEMORY_BUCKET * GetFromGarbage(
			void)
		{
			if (garbage == NULL)
			{
				return NULL;
			}
			MEMORY_BUCKET * bucket = garbage;
			garbage = (MEMORY_BUCKET *)garbage->next;
			return bucket;
		}

		// -------------------------------------------------------------------
		inline MEMORY_BUCKET * sGetNew(
			void)
		{
			MEMORY_BUCKET * bucket = GetFromGarbage();
			if (bucket)
			{
				return bucket;
			}
			if (bucketblocklistfirst < BUCKETS_PER_BUCKET_BLOCK)
			{
				ASSERT(bucketblocklist);
				return &bucketblocklist->buckets[bucketblocklistfirst++];
			}
			BUCKET_BLOCK * block = (BUCKET_BLOCK *)VPM_GET_BLOCK(MEM_PAGE_SIZE);
			CHECK_OUT_OF_MEMORY(block, MEM_PAGE_SIZE);
			ASSERT_RETNULL(block);
			block->next = bucketblocklist;
			bucketblocklist = block;
			bucketblocklistfirst = 1;
			return &block->buckets[0];
		}

		// -------------------------------------------------------------------
		inline void sRecycle(
			MEMORY_BUCKET * bucket)
		{
			bucket->next = (MEMORY_BUCKET *)garbage;
			garbage = (MEMORY_BUCKET *)bucket;
		}

	public:
		// -------------------------------------------------------------------
		void Init(
			void)
		{
			InitCriticalSection(cs);
			garbage = NULL;
			bucketblocklist = NULL;
			BUCKETS_PER_BUCKET_BLOCK = (MEM_PAGE_SIZE - sizeof(BUCKET_BLOCK_HDR)) / sizeof(MEMORY_BUCKET);
			bucketblocklistfirst = BUCKETS_PER_BUCKET_BLOCK;
		}

		// -------------------------------------------------------------------
		void Free(
			void)
		{
			BUCKET_BLOCK * cur;
			BUCKET_BLOCK * next = bucketblocklist;
			while ((cur = next) != NULL)
			{
				next = (BUCKET_BLOCK *)cur->next;
				VPM_RELEASE_BLOCK(cur);
			}
			bucketblocklist = NULL;
			garbage = NULL;
			cs.Free();
		}

		// -------------------------------------------------------------------
		inline MEMORY_BUCKET * GetNew(
			void)
		{
			MT_SAFE_LOCK(cs, return sGetNew(););
		}

		// -------------------------------------------------------------------
		inline void Recycle(
			MEMORY_BUCKET * bucket)
		{
			ASSERT_RETURN(bucket);

			VPM_RELEASE_BLOCK(bucket->ptr);
			
			bucket->ptr = NULL;

			MT_SAFE_LOCK(cs, return sRecycle(bucket););
		}
	};
	BUCKET_STORE<MT_SAFE>			bucketstore;

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	GetMemoryBucket
	//
	// Parameters
	//	bin : A pointer to the MEMORY_BIN from which to retrieve a bucket.
	//
	// Returns
	//	A MEMORY_BUCKET that has free blocks available.
	//
	// Remarks
	//	If there are no blocks available, this function will allocate and return a new bucket.
	//
	/////////////////////////////////////////////////////////////////////////////	
	inline MEMORY_BUCKET * GetMemoryBucket(
		MEMORY_BIN * bin)
	{
		ASSERT_RETNULL(bin);

		// The index is the index of the bin in the bin array held by the memory pool.
		//
		unsigned int index = bin->index;

		// Grab the first bucket for the bin
		//
		MEMORY_BUCKET * bucket = (MEMORY_BUCKET *)bins.binlist[index].head;

		// If the bucket is full or there are no buckets allocated yet ...
		// 
		if (NULL == bucket || bucket->usedcount >= bin->blockcount)
		{					
			// Grab a new bucket from the pool
			//
			bucket = bucketstore.GetNew();
			ASSERT_RETNULL(bucket);			
			
			if (!bucket->Init(VPM_GET_BLOCK(bin->bucketsize), bin) ||
				!buckettracker.Insert(bucket))
			{
				ASSERT(0);
				if(bucket->ptr)
				{
					VPM_RELEASE_BLOCK(bucket->ptr);
				}
				bucketstore.Recycle(bucket);
				return NULL;
			}
			ASSERT(bucket->ptr);

			// Add the new bucket to bin's bucket list
			//
			bins.Add(bucket);

			// Account for the bucket's overhead
			//
			m_InternalOverhead += bin->overheadPerBucket;

			// Since all the blocks in the new bucket are unused, account for them in the overhead
			//
			m_InternalOverhead += bin->size * bin->blockcount;
			
		}

		return bucket;
	}


	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	GetUnusedBlock
	//
	// Parameters
	//	bin : A pointer to the MEMORY_BIN from which to retrieve an unused block
	//	size : The size of the external allocation request for which an unused block is
	//		being retrieved.
	//
	// Returns
	//	A pointer to the beginning of the unused block or NULL if not available
	//
	// Remarks
	//	This function will allocate new virtual memory if necessary through the creation
	//	of an additional bucket.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * GetUnusedBlock(
		MEMORY_BIN * bin,
		MEMORY_SIZE size)
	{
		void * ptr;

		// Retrieve the next bucket with available blocks, allocating
		// a new bucket if necessary
		//
		MEMORY_BUCKET * bucket = GetMemoryBucket(bin);
		ASSERT_RETNULL(bucket);	

		// Retrieve the index of the next block in the bucket available
		// for external allocation.  This also marks it as used.
		//
		unsigned int block = bucket->GetAndSetFirstUnusedBlock();
		ASSERT(block < bin->blockcount);

		// Keep track of the blocks currently used in each bin
		//
		++bin->usedBlocks;

		// Get a pointer to the beginning of the available block within the bucket
		// memory
		//
		ptr = (BYTE *)bucket->ptr + bin->size * block;

		// Increment the used count
		//
		bucket->usedcount++;

		// If we just allocated the last block from the bucket,
		// move the bucket to the next of the bin's bucket list since
		// we always assume that the first bucket in the list has an 
		// available block
		//
		if (bucket->usedcount >= bin->blockcount)
		{
			bin->MoveToTail(bucket);
		}

		// Record the size of the external allocation in the bucket's array
		// of external allocation sizes
		//
		bucket->SetAllocationSize(block, size);
		
		// Keep track of the internal fragmentation which in this case is the difference 
		// between the block size and the external allocation request size
		//
		m_InternalFragmentation += bin->size - size;		

		BIN_TRACE_0("Allocate block:  Bin:%3d  Count:%d  BinSize:%5d  UsedCount:%3d  BlockId:%3d  Ptr:%p  Size:%d  \n", 
			bin->index, bin->bucketcount, bin->size, bucket->usedcount, block, ptr, size);

		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sGetSizeToAlloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A modified size used to select the bin for which the external allocation will be satisfied.
	//
	// Remarks
	//	If the default bin sizes are ever changed, this method will also need to be changed
	//
	/////////////////////////////////////////////////////////////////////////////	
	static MEMORY_SIZE sGetSizeToAlloc(
		MEMORY_SIZE size,
		unsigned int alignment)
	{
		DBG_ASSERT(IsPowerOf2(alignment) && alignment <= MAX_ALIGNMENT_SUPPORTED);

		// Adjust the size to account for debug info
		//
		size = MEMORYPOOL::sGetSizeToAlloc(size);

		if (size < 128)
		{
			if (size < 64)
			{
				if (alignment == 16)
				{
					size = ROUNDUP(size, (MEMORY_SIZE)16);
				}
				else if (alignment == 8 && size < 32)
				{
					size = ROUNDUP(size, (MEMORY_SIZE)8);
				}
			}
			else if (alignment == 32)
			{
				size = ROUNDUP(size, (MEMORY_SIZE)32);
			}
		}
		return size;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sCheckDebugSentinel
	//
	// Parameters
	//	trueptr : A pointer to the beginning of the actual allocation
	//
	// Returns
	//	0 if the check succeeds.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline MEMORY_SIZE sCheckDebugSentinel(
		void * trueptr,
		MEMORY_BUCKET * bucket)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		ASSERT_RETZERO(bucket);

		unsigned int block = bucket->CalcBlockFromPointer(trueptr);
		ASSERT_RETZERO(block < bucket->bin->blockcount);
		MEMORY_SIZE oldsize = bucket->GetAllocationSize(block);
		MEMORYPOOL::sCheckDebugSentinel(trueptr, oldsize);
		return oldsize;
#else
		return 0;
#endif
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sLargeAlloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	// Remarks
	// 	Used the CRT heap for allocation
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * sLargeAlloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size))
	{
		// Adjust the size accounting for debug info
		//
		MEMORY_SIZE sizetoalloc = MEMORYPOOL::sGetSizeToAlloc(size);

		// Allocate from the CRT heap
		//
		void * trueptr = malloc(sizetoalloc);
		CHECK_OUT_OF_MEMORY(trueptr, sizetoalloc);

		// Track the amount of large allocations (this isn't thread safe but oh well)
		//
		m_LargeAllocationSize += sizetoalloc;
		++m_LargeAllocations;
		
#if DEBUG_MEMORY_ALLOCATIONS

		
		sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
		MEMORYPOOL::TrackAlloc(trueptr, size, file, line);
#endif

		// Return a pointer adjusted for the debug info
		//
		return sGetPointerFromTruePointer(trueptr);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sLargeFree
	//
	// Parameters
	//	ptr : A pointer to the memory to free
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void sLargeFree(
		MEMORY_FUNCARGS(void * ptr))
	{
		// Adjust the pointer accounting for debug info
		//
		void * trueptr = sGetTruePointerFromPointer(ptr);
		MEMORY_DEBUG_REF(file);
		MEMORY_DEBUG_REF(line);

#if DEBUG_MEMORY_ALLOCATIONS
		MEMORY_SIZE oldsize = sGetSizeFromAllocSize((MEMORY_SIZE)_msize(trueptr));

		MEMORYPOOL::sCheckDebugSentinel(trueptr, oldsize);
		MEMORYPOOL::TrackFree(trueptr, oldsize);
#endif

		// Track the amount of large allocations (this isn't thread safe but oh-well)
		//
		m_LargeAllocationSize -= (unsigned int)(_msize(trueptr));
		--m_LargeAllocations;

		// Release the memory to the CRT heap
		//
		free(trueptr);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sLargeRealloc
	//
	// Parameters
	//	ptr : A pointer to the memory to realloc
	//	size : The size in bytes that the pointer should be allocated for
	//	oldsize : [out] Will contain the size of the old allocation
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * sLargeRealloc(
		MEMORY_FUNCARGS(void * ptr,
		MEMORY_SIZE size,
		MEMORY_SIZE & oldsize))
	{
		void * trueptr = sGetTruePointerFromPointer(ptr);

		MEMORY_SIZE rawOldSize = (MEMORY_SIZE)_msize(trueptr);
		oldsize = sGetSizeFromAllocSize(rawOldSize);

#if DEBUG_MEMORY_ALLOCATIONS		
		MEMORYPOOL::sCheckDebugSentinel(trueptr, oldsize);
		MEMORYPOOL::TrackFree(trueptr, oldsize);
#endif

		MEMORY_SIZE sizetoalloc = MEMORYPOOL::sGetSizeToAlloc(size);
#pragma warning(suppress:6308)
		trueptr = realloc(trueptr, sizetoalloc);
		CHECK_OUT_OF_MEMORY(trueptr, sizetoalloc);

		// Track the amount of large allocations
		//
		m_LargeAllocationSize += (int)sizetoalloc - (int)rawOldSize;
		

#if DEBUG_MEMORY_ALLOCATIONS		
		sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
		MEMORYPOOL::TrackAlloc(trueptr, size, file, line);
#endif

		return sGetPointerFromTruePointer(trueptr);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sAlignedAlloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * sAlignedAlloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size,
		unsigned int alignment))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Determine the actual allocation size given the alignment.  This will 
		// potentially bump the allocation into a higher bin that will be aligned.
		//
		unsigned int allocsize = sGetSizeToAlloc(size, alignment);

		// Retrieve a bin for the requested size
		//
		MEMORY_BIN * bin = bins.GetBySize(allocsize);

		// If the request is too big to service with the bin allocator...
		//
		if (!bin)
		{
			// Use the CRT heap
			//
			return sLargeAlloc(MEMORY_FUNC_PASSFL(size));
		}

		void * trueptr;
		{
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			// Get a pointer to the next available block for the bin
			//
			trueptr = GetUnusedBlock(bin, size);
			ASSERT_RETNULL(trueptr);

			// Keep track of the total allocations from the pool
			// 
			m_SmallAllocationSize += size;
			++m_SmallAllocations;

			// Adjust overhead
			//
			m_InternalOverhead -= bin->size;			
		}

#if DEBUG_MEMORY_ALLOCATIONS
		sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
		MEMORYPOOL::TrackAlloc(trueptr, size, file, line);
#endif

		void * ptr = sGetPointerFromTruePointer(trueptr);

		DBG_ASSERT(IS_ALIGNED(ptr, alignment));
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sRealloc
	//
	// Parameters
	//	ptr : A pointer to the memory to realloc
	//	size : The size in bytes that the pointer should be allocated for
	//	oldsize : [out] Will contain the size of the old allocation
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * sRealloc(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size,
		MEMORY_SIZE & oldsize))
	{
		if (NULL == ptr)
		{
			oldsize = 0;
			return Alloc(MEMORY_FUNC_PASSFL(size));
		}

		// Adjust the pointer to account for debug info
		//
		void * trueptr = sGetTruePointerFromPointer(ptr);

		// Find the bin and bucket from which the memory was taken from
		//
		MEMORY_BIN * bin;
		MEMORY_BUCKET * bucket = buckettracker.FindBucket(trueptr, bin);

		// If we didn't find a bucket, it means that the allocation was too large for the bins
		//
		if (!bucket)
		{
			return sLargeRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
		}

		// Lock the bin and put the block back in the bucket
		//
		{
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			ASSERT_RETNULL(bucket->bin == bin);

			// Get the block index within the bucket for the old pointer
			//
			unsigned int block = bucket->CalcBlockFromPointer(trueptr);
			ASSERT_RETNULL(block < bucket->bin->blockcount);
			
			oldsize = bucket->GetAllocationSize(block);
			
#if DEBUG_MEMORY_ALLOCATIONS
			MEMORYPOOL::sCheckDebugSentinel(trueptr, oldsize);
#endif

			unsigned int allocsize = MEMORYPOOL::sGetSizeToAlloc(size);
			
			
			// If the block is large enough to satisfy the new allocation size,
			// just return the same pointer
			//
			if (bucket->bin->size >= allocsize)
			{
				// Update the total count of small allocations
				//
				m_SmallAllocationSize += (int)size - (int)oldsize;
				
				bucket->SetAllocationSize(block, size);
#if DEBUG_MEMORY_ALLOCATIONS
				sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
				MEMORYPOOL::TrackRealloc(trueptr, oldsize, size, file, line);
#endif

				return ptr;
			}
		}

		void * newptr = Alloc(MEMORY_FUNC_PASSFL(size));
		CHECK_OUT_OF_MEMORY(newptr, size);

		MEMORY_SIZE bytestocopy = MIN(size, oldsize);
		memcpy(newptr, ptr, bytestocopy);

		Free(MEMORY_FUNC_PASSFL(ptr));

		ptr = newptr;
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	sRealloc
	//
	// Parameters
	//	ptr : A pointer to the memory to realloc
	//	size : The size in bytes that the pointer should be allocated for
	//	alignment : The alignment in bytes for which the reallocation must occur
	//	oldsize : [out] Will contain the size of the old allocation
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * sAlignedRealloc(
		MEMORY_FUNCARGS(void * ptr,
		MEMORY_SIZE size,
		unsigned int alignment,
		MEMORY_SIZE & oldsize))
	{
		if (NULL == ptr)
		{
			oldsize = 0;
			return sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
		}

		// Adjust the pointer to account for debug info
		//
		void * trueptr = sGetTruePointerFromPointer(ptr);

		// Find the bin and bucket from which the memory was taken from
		//
		MEMORY_BIN * bin;
		MEMORY_BUCKET * bucket = buckettracker.FindBucket(trueptr, bin);

		// If we didn't find a bucket, it means that the allocation was too large for the bins
		//		
		if (!bucket)
		{
			return sLargeRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
		}

		// Lock the bin and put the block back in the bucket
		//
		{
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			ASSERT_RETNULL(bucket->bin == bin);

			// Get the block index within the bucket for the old pointer
			//
			unsigned int block = bucket->CalcBlockFromPointer(trueptr);
			ASSERT_RETNULL(block < bucket->bin->blockcount);
			
			oldsize = bucket->GetAllocationSize(block);
#if DEBUG_MEMORY_ALLOCATIONS
			MEMORYPOOL::sCheckDebugSentinel(trueptr, oldsize);
#endif


			unsigned int allocsize = MEMORYPOOL::sGetSizeToAlloc(size);

			// If the block is large enough to satisfy the new allocation size and it is properly aligned,
			// just return the same pointer
			//
			if (bucket->bin->size >= allocsize && IS_ALIGNED(ptr, alignment))
			{
				// Update the total count of small allocations
				//
				m_SmallAllocationSize += (int)size - (int)oldsize;			
			
				bucket->SetAllocationSize(block, size);
#if DEBUG_MEMORY_ALLOCATIONS
				sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
				MEMORYPOOL::TrackRealloc(trueptr, oldsize, size, file, line);
#endif
				return ptr;
			}
		}

		void * newptr = sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
		CHECK_OUT_OF_MEMORY(newptr, size);

		MEMORY_SIZE bytestocopy = MIN(size, oldsize);
		memcpy(newptr, ptr, bytestocopy);

		Free(MEMORY_FUNC_PASSFL(ptr));

		ptr = newptr;
		return ptr;
	}

public:
	// -----------------------------------------------------------------------
	void Init(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		ASSERT(MEMORY_DEBUG_HEADER_SIZE == 16);		// required for aligned allocs to work
#endif

		InitBase(MT_SAFE ? MEMORY_BINALLOCATOR_MT : MEMORY_BINALLOCATOR_ST);

		const MEMORY_BIN_DEF * bindef = NULL;
		unsigned int count = 0;
		if (!bindef)
		{
			bindef = g_DefaultBinDef;
			count = arrsize(g_DefaultBinDef);
		}

		ASSERT(bins.Init(bindef, count));
		bucketstore.Init();
		buckettracker.Init();
	}

	// -----------------------------------------------------------------------
	void Free(
		void)
	{
		buckettracker.Free();
		bucketstore.Free();
		bins.Free();

		MEMORYPOOL::FreeBase();
	}

	// -----------------------------------------------------------------------
	inline MEMORY_SIZE Size(
		void * ptr)
	{
		if (NULL == ptr)
		{
			return 0;
		}
		
		void * trueptr = sGetTruePointerFromPointer(ptr);

		// get the bucket
		MEMORY_BIN * bin;
		MEMORY_BUCKET * bucket = buckettracker.FindBucket(trueptr, bin);
		if (!bucket)
		{
			return (MEMORY_SIZE)_msize(trueptr);
		}

		MEM_AUTOLOCK_CS autolock(bins[bucket->bin->index].GetLock());

		ASSERT_RETZERO(bucket->bin == bin);

		unsigned int block = bucket->CalcBlockFromPointer(trueptr);
		ASSERT_RETZERO(block < bucket->bin->blockcount);
		return bucket->GetAllocationSize(block);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	Alloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * Alloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Account for additional debug overhead in the allocation
		//
		MEMORY_SIZE sizetoalloc = MEMORYPOOL::sGetSizeToAlloc(size);

		// Grab the bin corresponding to the external allocation request size
		//
		MEMORY_BIN * bin = bins.GetBySize(sizetoalloc);

		// If the request is too large to be services by the bins...
		//
		if (!bin)
		{
			// allocate from heap
			return sLargeAlloc(MEMORY_FUNC_PASSFL(size));
		}

		void * trueptr;
		{
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			// Get a pointer to the beginning of the next unused block
			//
			trueptr = GetUnusedBlock(bin, size);
			ASSERT_RETNULL(trueptr);

			// Keep track of the total allocations from the pool
			// 
			m_SmallAllocationSize += size;
			++m_SmallAllocations;

			// Adjust overhead
			//
			m_InternalOverhead -= bin->size;
		}

#if DEBUG_MEMORY_ALLOCATIONS
		sSetDebugSentinel(MEMORY_FUNC_PASSFL(trueptr, size));
		MEMORYPOOL::TrackAlloc(trueptr, size, file, line);
#endif


		// Return the pointer adjusted for any debug information prepended
		// to the pointer
		//
		return sGetPointerFromTruePointer(trueptr);
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AllocZ
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	// Remarks
	//	Zeros the memory before returning it.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AllocZ(
		MEMORY_FUNCARGS(MEMORY_SIZE size))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Do a normal allocation
		//
		void * ptr = Alloc(MEMORY_FUNC_PASSFL(size));
		ASSERT(ptr);

		// Zero the memory
		//
		if (!ptr)
		{
			MemoryTraceAllFLIDX();
		}
        else 
        {
            memclear(ptr, size);
        }
		
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AlignedAlloc
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AlignedAlloc(
		MEMORY_FUNCARGS(MEMORY_SIZE size,
		unsigned int alignment))
	{
		return sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	AlignedAllocZ
	//
	// Parameters
	//	size : The size of the external allocation request for which a pointer will be returned.
	//	alignment : The size in bytes for which the returned pointer must be aligned
	//
	// Returns
	//	A pointer to memory for the allocation request.  NULL if the allocation fails.
	//
	// Remarks
	//	Zeros the memory before returning it.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void * AlignedAllocZ(
		MEMORY_FUNCARGS(MEMORY_SIZE size,
		unsigned int alignment))
	{
		if (size == 0)
		{
			return NULL;
		}

		// Do a normal aligned allocation
		//
		void * ptr = sAlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
		ASSERT_RETNULL(ptr);

		// Zero the memory
		//
		memclear(ptr, size);
		
		return ptr;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// Name
	//	Free
	//
	// Parameters
	//	ptr : The pointer to free
	//
	// Remarks
	//	CRT heap allocated memory which is over the max block size is also released 
	//	through this function.
	//
	/////////////////////////////////////////////////////////////////////////////
	inline void Free(
		MEMORY_FUNCARGS(void * ptr))
	{
		if (NULL == ptr)
		{
			return;
		}

		void * trueptr = sGetTruePointerFromPointer(ptr);

		// Get a pointer to the bin and bucket for which the pointer was allocated
		//
		MEMORY_BIN * bin;
		MEMORY_BUCKET * bucket = buckettracker.FindBucket(trueptr, bin);

		
		// If the true pointer was not found in a bucket, it means that it was a large
		// allocation and satisfied by the CRT heap.
		//
		if (!bucket)
		{
			sLargeFree(MEMORY_FUNC_PASSFL(ptr));
			return;
		}
		ASSERT_RETURN(bin);

		// Return the block to the bucket's free list
		//
		{						
			MEM_AUTOLOCK_CS autolock(bin->GetLock());

			ASSERT_RETURN(bucket->bin == bin);
#if DEBUG_MEMORY_ALLOCATIONS
			MEMORY_SIZE oldsize = sCheckDebugSentinel(trueptr, bucket);
			MEMORYPOOL::TrackFree(trueptr, oldsize);
#endif

			MEMORY_SIZE externalAllocationSize = bucket->Free(trueptr);			

			// Keep track of how many blocks are currently used in the bin
			//
			--bin->usedBlocks;

			// Adjust the internal fragmentation 
			//
			m_InternalFragmentation -= (bin->size - externalAllocationSize);

			// Keep track of outstanding allocations
			//
			m_SmallAllocationSize -= externalAllocationSize;
			--m_SmallAllocations;

			// Adjust overhead
			//
			m_InternalOverhead += bin->size;
		}

		ptr = NULL;
	}

	// -----------------------------------------------------------------------
	inline void * Realloc(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size))
	{
		MEMORY_SIZE oldsize;
		return sRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
	}

	// -----------------------------------------------------------------------
	inline void * ReallocZ(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size))
	{
		MEMORY_SIZE oldsize;
		ptr = sRealloc(MEMORY_FUNC_PASSFL(ptr, size, oldsize));
		ASSERT_RETNULL(ptr);
		if (oldsize < size)
		{
			memclear((BYTE *)ptr + oldsize, (size - oldsize));
		}
		return ptr;
	}

	// -----------------------------------------------------------------------
	inline void * AlignedRealloc(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size,
		unsigned int alignment))
	{
		MEMORY_SIZE oldsize;
		return sAlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, oldsize));
	}

	// -----------------------------------------------------------------------
	inline void * AlignedReallocZ(
		MEMORY_FUNCARGS(void * ptr, 
		MEMORY_SIZE size,
		unsigned int alignment))
	{
		MEMORY_SIZE oldsize;
		ptr = sAlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, oldsize));
		ASSERT_RETNULL(ptr);
		if (oldsize < size)
		{
			memclear((BYTE *)ptr + oldsize, (size - oldsize));
		}
		return ptr;
	}

	// -----------------------------------------------------------------------
	void MemoryTrace(
		void)
	{
#if DEBUG_MEMORY_ALLOCATIONS
		UINT64 total_used = 0;			// total allocated
		UINT64 total = 0;				// total used

		PrintForWpp("BIN ALLOCATOR - POOL:%S", m_Name);
		PrintForWpp("%3s  %6s  %8s  %4s  %7s  %7s  %9s", "BIN", "SIZE", "BKT_SIZE", "NUM", "ALLOCS", "UNUSED", "MEM");
		for (unsigned int ii = 0; ii < bins.GetCount(); ++ii)
		{
			MEMORY_BIN * bin = &bins[ii];
			MEM_AUTOLOCK_CS autolock(bin->GetLock());
			if (bin->bucketcount <= 0)
			{
				continue;
			}
			MEMORY_BUCKET * curr = NULL;
			MEMORY_BUCKET * next = bin->head;
			unsigned int allocs = 0;
			while (NULL != (curr = next))
			{
				next = curr->next;
				allocs += curr->usedcount;
			}
			total_used += (UINT64)allocs * (UINT64)bin->size;
			UINT64 mem = (UINT64)bin->bucketsize * (UINT64)bin->bucketcount;
			total += mem;
			PrintForWpp("%3u  %6llu  %8llu  %4u  %7u  %7u  %9llu", 
				ii, (UINT64)bin->size, (UINT64)bin->bucketsize, bin->bucketcount, allocs, bin->bucketcount * bin->blockcount - allocs, mem);
		}
		PrintForWpp("  TOTAL USED: %llu/%llu", total_used, total);
#endif
	}
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
typedef MEMORYPOOL_BIN_BASE<FALSE>	MEMORYPOOL_BIN;
typedef MEMORYPOOL_BIN_BASE<TRUE>	MEMORYPOOL_BIN_MT;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <unsigned int SIZE>
struct MPBTestStack
{
	unsigned int *					list;
	unsigned int					count;

	MPBTestStack(
		void) 
	{ 
		count = 0; 
		list = new unsigned int[SIZE]; 
	}

	~MPBTestStack(
		void) 
	{ 
		delete list; 
	}

	void Push(
		unsigned int val)
	{ 
		list[count++] = val; 
	}

	unsigned int Pop(
		void) 
	{ 
		return list[--count]; 
	}

	void Reverse(
		void)
	{
		if (count <= 0)
		{
			return;
		}
		for (unsigned int ii = 0, jj = count - 1; ii < jj; ++ii, --jj)
		{
			unsigned int temp = list[ii];
			list[ii] = list[jj];
			list[jj] = temp;
		}
	}

	void Shuffle(
		RAND & rand)
	{
		unsigned int ii = 0;
		do
		{
			unsigned int remaining = count - ii;
			if (remaining <= 1)
			{
				break;
			}
			unsigned int jj = ii + RandGetNum(rand, remaining);
			unsigned int temp = list[jj];
			list[jj] = list[ii];
			list[ii] = temp;
		} while (++ii);
	}
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct MPB_TEST
{
	MEMORYPOOL *				pool;
	volatile long *				lGo;
	volatile long *				lDone;
	unsigned int				index;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static MEMORY_SIZE sMPBTestGetSize(
	RAND & rand)
{
	/*
	static const unsigned int MAX_BLOCK_SIZE_3 = 26000;
	static const unsigned int MAX_BLOCK_SIZE_3_CHANCE = 1024;
	static const unsigned int MAX_BLOCK_SIZE_2 = 4096;
	static const unsigned int MAX_BLOCK_SIZE_2_CHANCE = 256;
	static const unsigned int MAX_BLOCK_SIZE_1 = 512;
	*/
	
	static const unsigned int MAX_BLOCK_SIZE_3 = 256000;
	static const unsigned int MAX_BLOCK_SIZE_3_CHANCE = 1024;
	static const unsigned int MAX_BLOCK_SIZE_2 = 68000;
	static const unsigned int MAX_BLOCK_SIZE_2_CHANCE = 256;
	static const unsigned int MAX_BLOCK_SIZE_1 = 24000;
	

	if (RandGetNum(rand, MAX_BLOCK_SIZE_3_CHANCE) == 0)
	{
		return RandGetNum(rand, MAX_BLOCK_SIZE_2, MAX_BLOCK_SIZE_3);
	}
	if (RandGetNum(rand, MAX_BLOCK_SIZE_2_CHANCE) == 0)
	{
		return RandGetNum(rand, MAX_BLOCK_SIZE_1 + 1, MAX_BLOCK_SIZE_2);
	}
	//return RandGetNum(rand, 1, MAX_BLOCK_SIZE_1);
	return RandGetNum(rand, 4096, MAX_BLOCK_SIZE_1);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define MPBT_MODE						1

#if (MPBT_MODE == 1)
#define MPBT_ALLOC(pool, size)			MemoryAlloc(MEMORY_FUNC_FILELINE(pool, size))
#define MPBT_FREE(pool, ptr)			MemoryFree(MEMORY_FUNC_FILELINE(pool, ptr))
#define MPBT_REALLOC(pool, ptr, size)	MemoryRealloc(MEMORY_FUNC_FILELINE(pool, ptr, size))
#define MPBT_MEMSET(ptr, val, size)		memset(ptr, val, size)
#define MPBT_SHUFFLE(rand, stack)		stack.Shuffle(rand)
#define MPBT_TRACE_FLIDX(pool)			MemoryTraceFLIDX(pool)
#define MPBT_TRACE_ALLOC(pool)			// MemoryTraceALLOC(pool)
#elif (MPBT_MODE == 2)
#define MPBT_ALLOC(pool, size)			malloc(size)
#define MPBT_FREE(pool, ptr)			free(ptr)
#define MPBT_REALLOC(pool, ptr, size)	realloc(ptr, size)
#define MPBT_MEMSET(ptr, val, size)		// memset(ptr, val, size)
#define MPBT_SHUFFLE(rand, stack)		stack.Shuffle(rand)
#define MPBT_TRACE_FLIDX(pool)
#define MPBT_TRACE_ALLOC(pool)
#else
#define MPBT_ALLOC(pool, size)			(void *)((BYTE *)0 + 1)
#define MPBT_FREE(pool, ptr)			
#define MPBT_REALLOC(pool, ptr, size)	(void *)((BYTE *)0 + 1)
#define MPBT_MEMSET(ptr, val, size)
#define MPBT_SHUFFLE(rand, stack)		stack.Shuffle(rand)
#define MPBT_TRACE_FLIDX(pool)
#define MPBT_TRACE_ALLOC(pool)
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD WINAPI MemPoolBinTestThread(
	void * param)
{
	struct MBTNode
	{
		void *								ptr;
		unsigned int						size;
	};

	MPB_TEST * mpbtest = (MPB_TEST *)param;

	MEMORYPOOL * pool = mpbtest->pool;

	unsigned int index = mpbtest->index;

	RAND rand;
	RandInit(rand, index + 1, index + 1);

	while (*mpbtest->lGo == 0)
	{
		Sleep(0);
	}

	static const unsigned int ITER_ONE = 70000;
	static const unsigned int ITER_TWO = 200;
	
	MBTNode * blocks = (MBTNode *)malloc(ITER_ONE * sizeof(MBTNode));

	MPBTestStack<ITER_ONE> freeblocks;
	MPBTestStack<ITER_ONE> unfreed;
	MPBTestStack<ITER_ONE> reallocblocks;

	BIN_TRACE_2("[%d]  Memory Pool Bin Allocator Test Start\n", index);

	DWORD start = GetTickCount();
	// first allocate ITER_ONE blocks
	for (unsigned int ii = 0; ii < ITER_ONE; ++ii)
	{
		MEMORY_SIZE size = sMPBTestGetSize(rand);
		blocks[ii].size = size;
		blocks[ii].ptr = MPBT_ALLOC(pool, size);
		ASSERT(blocks[ii].ptr);
		MPBT_MEMSET(blocks[ii].ptr, 1, size);
		unfreed.Push(ii);
	}

	BIN_TRACE_2("[%d]  Allocate %d blocks:  Time:%d\n", index, ITER_ONE, GetTickCount() - start);

	start = GetTickCount();

	DWORD count = 0;
	// now go through a cycle of free & allocate
	for (unsigned int ii = 0; ii < ITER_TWO; ++ii)
	{
		BIN_TRACE_0("*** Cycle: %d\n", ii);

		// free some blocks
		if (unfreed.count > 0)
		{
			unsigned int tofree = RandGetNum(rand, 1, unfreed.count);
			BIN_TRACE_0("To Free Count: %d\n", tofree);
			count += tofree;

			MPBT_SHUFFLE(rand, unfreed);
			for (unsigned int jj = 0; jj < tofree; ++jj)
			{
				unsigned int index2 = unfreed.Pop();
				MPBT_FREE(pool, blocks[index2].ptr);
				blocks[index2].ptr = NULL;
				freeblocks.Push(index2);
			}
		}

		// allocate some blocks
		if (freeblocks.count > 0)
		{
			unsigned int toalloc = RandGetNum(rand, 1, freeblocks.count);
			BIN_TRACE_0("To Alloc Count: %d\n", toalloc);
			count += toalloc;

			MPBT_SHUFFLE(rand, freeblocks);
			for (unsigned int jj = 0; jj < toalloc; ++jj)
			{
				unsigned int index2 = freeblocks.Pop();

				blocks[index2].ptr = MPBT_ALLOC(pool, blocks[index2].size);
				ASSERT(blocks[index2].ptr);
				MPBT_MEMSET(blocks[index2].ptr, 1, blocks[index2].size);
				unfreed.Push(index2);
			}
		}

		// realloc some blocks
		if (unfreed.count > 0)
		{
			unsigned int torealloc = RandGetNum(rand, 1, unfreed.count);
			BIN_TRACE_0("To Realloc Count: %d\n", torealloc);
			count += torealloc;

			MPBT_SHUFFLE(rand, unfreed);
			for (unsigned int jj = 0; jj < torealloc; ++jj)
			{
				unsigned int index2 = unfreed.Pop();

				blocks[index2].size = sMPBTestGetSize(rand);
				blocks[index2].ptr = MPBT_REALLOC(pool, blocks[index2].ptr, blocks[index2].size);
				ASSERT(blocks[index2].ptr);
				MPBT_MEMSET(blocks[index2].ptr, 1, blocks[index2].size);
				reallocblocks.Push(index2);
			}
			while (reallocblocks.count)
			{
				unsigned int index2 = reallocblocks.Pop();
				unfreed.Push(index2);
			}
		}
	}

	BIN_TRACE_2("[%d]  Allocate/Deallocate/Reallocate %d blocks  Time:%d\n", index, count, GetTickCount() - start);
	start = GetTickCount();
	
	// Free all blocks
	//
	count = unfreed.count;
	while(unfreed.count)
	{
		unsigned int unfreedIndex = unfreed.Pop();
		MPBT_FREE(pool, blocks[unfreedIndex].ptr);
		blocks[unfreedIndex].ptr = NULL;
	}
	BIN_TRACE_2("[%d]  Free All: %d blocks  Time:%d\n", index, count, GetTickCount() - start);	

	if (blocks)
	{
		free(blocks);
	}

	AtomicDecrement(*mpbtest->lDone);

	return 0;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define MBT_POOL_TYPE			MEMORYPOOL_BIN_MT


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void MemPoolBinTest(
	void)
{
/*
	IMemoryManager::GetInstance().Init();
	
	//g_Vpm.Init();


	static const unsigned int THREAD_COUNT = 1;


//while(true)
//{

	//MEMORYPOOL_HEAP_MT pool;
	//pool.Init(MEMORY_HEAPALLOCATOR_MT, NULL, 256000, 1024 * 1024 * 64, 1024 * 1024 * 64);
	
	//MEMORYPOOL_CRT pool;
	//pool.Init(MEMORY_CRTALLOCATOR, NULL);
	

	volatile long lGo = 0;
	volatile long lDone = THREAD_COUNT;

	MPB_TEST test[THREAD_COUNT];

	for (unsigned int ii = 0; ii < THREAD_COUNT; ++ii)
	{
		test[ii].pool = &pool;
		test[ii].lGo = &lGo;
		test[ii].lDone = &lDone;
		test[ii].index = ii;

		DWORD dwThreadId;
		HANDLE handle = CreateThread(NULL, 0, MemPoolBinTestThread, (void *)&test[ii], 0, &dwThreadId);
		CloseHandle(handle);
	}

	lGo = 1;

	while (lDone != 0)
	{
		Sleep(10);
	}

	DWORD start = GetTickCount();
	//MPBT_TRACE_FLIDX(&pool);
	//MPBT_TRACE_ALLOC(&pool);
	pool.Free();
	BIN_TRACE_2("\nFree pool:  Time:%d\n", GetTickCount() - start);
//}	

	IMemoryManager::GetInstance().Term();

	//g_Vpm.Free();

*/
	
}


//-----------------------------------------------------------------------------
// ADV API
//-----------------------------------------------------------------------------
typedef BOOLEAN (*pfnRtlGenerateRandom)(void * buf, DWORD len);
pfnRtlGenerateRandom RtlGenerateRandom;


//-----------------------------------------------------------------------------
// global memory system, it tracks all of the pools
// note that initialization of the system isn't thread-safe
//-----------------------------------------------------------------------------
struct MEMORY_SYSTEM
{
public:

	struct MEMPOOL_NODE
	{
		MEMORYPOOL *						m_Pool;

		// The m_Next member is only used when a node is not in use.
		// If the node is in use and m_Pool is a valid pointer, m_Next will
		// be -1.
		//
		unsigned int						m_Next;
	};

	// Pointer to an array of memory pool nodes
	//
	MEMPOOL_NODE *							m_MemoryPools;
	
	// This is the size of the array pointed to by m_MemoryPools and never decreases
	//
	unsigned int							m_nMemoryPools;
	
	// The number of currently allocated pools
	//
	unsigned int 							m_PoolCount;

	// The index into m_MemoryPools of the first unused node.  Unused nodes
	// are linked together with their m_Next members.
	//
	unsigned int							m_nGarbage;
	
protected:

	// Used to protect the memory pool node list
	//
	CCritSectLite							m_CS;

	MEMORYPOOL *							m_DefaultPool;
    PERF_INSTANCE(MemoryPool) *				m_pAllGamesPerfInstance;

	BOOL									m_bInitialized;
	volatile long							m_bOutOfMemory;



	DWORD									m_dwTlsIndex;
	struct MEMORY_THREADPOOL
	{
		MEMORYPOOL *						pool;
	};

    HMODULE									m_hAdvApiLib;

	//-------------------------------------------------------------------------
	void InitHeapInfo(
		void)
	{
		intptr_t hCrtHeap = _get_heap_handle();
		ULONG lfh = 2;
		HeapSetInformation((void*)hCrtHeap, HeapCompatibilityInformation, &lfh, sizeof(lfh));
	}

	//-------------------------------------------------------------------------
	void InitAdvApiLib(
		void)
	{
		m_hAdvApiLib = LoadLibrary("ADVAPI32.DLL");
		if (m_hAdvApiLib)
		{
			RtlGenerateRandom = (pfnRtlGenerateRandom)GetProcAddress(m_hAdvApiLib, "SystemFunction036");
			if (!RtlGenerateRandom)
			{
				FreeLibrary(m_hAdvApiLib);
				m_hAdvApiLib = NULL;
			}
		}
	}

	//-------------------------------------------------------------------------
	void InitTLS(
		void)
	{
		m_dwTlsIndex = TlsAlloc();
		ASSERTX(m_dwTlsIndex != TLS_OUT_OF_INDEXES, "Out of Thread Local Storage Indexes.");
	}

	//-------------------------------------------------------------------------
	void FreeTLS(
		void)
	{
		TlsFree(m_dwTlsIndex);
	}

	//-------------------------------------------------------------------------
	unsigned int GetIndexFromPointer(
		MEMPOOL_NODE * node)
	{
		ASSERT_RETINVALID(node >= m_MemoryPools);
		unsigned int index = (unsigned int)(node - m_MemoryPools);
		ASSERT_RETINVALID(index < m_nMemoryPools);
		return index;
	}

	//-------------------------------------------------------------------------
	void RecyclePoolNode(
		MEMPOOL_NODE * node)
	{
		unsigned int index = GetIndexFromPointer(node);
		ASSERT_RETURN(index < m_nMemoryPools);
		node->m_Pool = NULL;
		node->m_Next = m_nGarbage;
		m_nGarbage = index;
	}

	//-------------------------------------------------------------------------
	MEMPOOL_NODE * GetNewPoolNode(
		void)
	{
		if (m_nGarbage < m_nMemoryPools)
		{
			MEMPOOL_NODE * node = &m_MemoryPools[m_nGarbage];
			ASSERT_RETNULL(node->m_Pool == NULL);
			m_nGarbage = node->m_Next;
			node->m_Next = (unsigned int)-1;
			return node;
		}
#pragma warning(suppress:6308)
		m_MemoryPools = (MEMPOOL_NODE *)realloc(m_MemoryPools, sizeof(MEMPOOL_NODE) * (m_nMemoryPools + 1));
		CHECK_OUT_OF_MEMORY(m_MemoryPools, sizeof(MEMPOOL_NODE) * (m_nMemoryPools + 1));
		ASSERT_RETNULL(m_MemoryPools);
		MEMPOOL_NODE * node = &m_MemoryPools[m_nMemoryPools];
		node->m_Pool = NULL;
		node->m_Next = (unsigned int)-1;
		++m_nMemoryPools;
		return node;
	}

	//-------------------------------------------------------------------------
	// pre-allocate pools list to minimize chance of running out of memory on this
	void InitNodeList(
		unsigned int INIT_NODE_COUNT)
	{
		// to do
		REF(INIT_NODE_COUNT);
	}

	//-------------------------------------------------------------------------
	BOOL AddPoolToList(
		MEMORYPOOL * pool)
	{
		CSLAutoLock lock(&m_CS);

		MEMPOOL_NODE * node = GetNewPoolNode();
		ASSERT_RETFALSE(node);

		node->m_Pool = pool;
		pool->m_nIndex = GetIndexFromPointer(node);

		if (m_DefaultPool == NULL)
		{
			m_DefaultPool = pool;
		}
		return TRUE;
	}

	//-------------------------------------------------------------------------
	BOOL RemovePoolFromList(
		MEMORYPOOL * pool)
	{
		CSLAutoLock lock(&m_CS);

		ASSERT_RETFALSE(pool->m_nIndex < m_nMemoryPools);
		MEMPOOL_NODE * node = &m_MemoryPools[pool->m_nIndex];
		ASSERT_RETFALSE(node->m_Pool == pool);
		node->m_Pool = NULL;
		RecyclePoolNode(node);
		return TRUE;
	}

	//-------------------------------------------------------------------------
	// add the pool to the global memory system
	MEMORYPOOL * sNewPool(
		WCHAR * szDesc,
		MEMORY_MANAGEMENT_TYPE pooltype)
	{
		MEMORYPOOL * pool = NULL;

		switch (pooltype)
		{
		case MEMORY_BINALLOCATOR_ST:
		{
			pool = new MEMORYPOOL_BIN;
			CHECK_OUT_OF_MEMORY(pool, sizeof(MEMORYPOOL_BIN));
			pool->Init();
			break;
		}	
		case MEMORY_BINALLOCATOR_MT:
		{
			pool = new MEMORYPOOL_BIN_MT;
			CHECK_OUT_OF_MEMORY(pool, sizeof(MEMORYPOOL_BIN_MT));
			pool->Init();
			break;
		}	
		default:
			ASSERT_RETNULL(0);
		}
		

#if DEBUG_MEMORY_ALLOCATIONS
		if (szDesc)
		{
			PStrCopy(pool->m_Name, szDesc, arrsize(pool->m_Name));
		}
#else
		REF(szDesc);
#endif

		AddPoolToList(pool);

		++m_PoolCount;

		return pool;
	}

public:
	//-------------------------------------------------------------------------
	void Init(
		unsigned int INIT_NODE_COUNT)
	{
		ASSERT_RETURN(!m_bInitialized);

		InitCriticalSection(m_CS);

		g_Vpm.Init();

		InitHeapInfo();
		InitAdvApiLib();
		InitTLS();
		InitNodeList(INIT_NODE_COUNT);
		

		m_MemoryPools = NULL;
		m_nGarbage = (unsigned int)-1;
		m_nMemoryPools = 0;

		// Allocate the default pools
		//
		sNewPool(L"Default Global Pool", DefaultMemoryAllocator);

		m_bOutOfMemory = FALSE;
		m_bInitialized = TRUE;
	}

	//-------------------------------------------------------------------------
	// free the pool
	void FreePool(
		MEMORYPOOL * pool)
	{
		ASSERT_RETURN(pool);

		ASSERT_RETURN(RemovePoolFromList(pool));

		pool->Free();

		--m_PoolCount;

		delete pool;
	}

	//-------------------------------------------------------------------------
	// add the pool to the global memory system
	MEMORYPOOL * NewPool(
		WCHAR * szDesc,
		MEMORY_MANAGEMENT_TYPE pooltype)
	{
		ASSERT_RETNULL(m_bInitialized);

		if (pooltype >= NUM_MEMORY_SYSTEMS)
		{
			pooltype = DefaultMemoryAllocator;
		}

		return sNewPool(szDesc, pooltype);
	}

	//-------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	void TraceAllFLIDX(
		void)
	{
		if (!m_bInitialized)
		{
			return;
		}

		CSLAutoLock lock(&m_CS);

		if (m_DefaultPool)
		{
			m_DefaultPool->MemoryTrace();
			m_DefaultPool->TraceFLIDX();
		}

		for (unsigned int ii = 0; ii < m_nMemoryPools; ++ii)
		{
			MEMORYPOOL * pool = m_MemoryPools[ii].m_Pool;
			if (pool && pool != m_DefaultPool)
			{
				pool->MemoryTrace();
				pool->TraceFLIDX();
			}
		}
	}
#endif

	//-------------------------------------------------------------------------
#if DEBUG_MEMORY_ALLOCATIONS
	void TraceAllALLOC(
		void)
	{
		if (!m_bInitialized)
		{
			return;
		}

		CSLAutoLock lock(&m_CS);

		if (m_DefaultPool)
		{
			m_DefaultPool->MemoryTrace();
			m_DefaultPool->TraceALLOC();
		}

		for (unsigned int ii = 0; ii < m_nMemoryPools; ++ii)
		{
			MEMORYPOOL * pool = m_MemoryPools[ii].m_Pool;
			if (pool && pool != m_DefaultPool)
			{
				pool->MemoryTrace();
				pool->TraceALLOC();
			}
		}
	}
#endif

	//-------------------------------------------------------------------------
	void Free(
		void)
	{
		if (!m_bInitialized)
		{
			return;
		}

		m_bInitialized = FALSE;

		for (unsigned int ii = 0; ii < m_nMemoryPools; ii++)
		{
			if (m_MemoryPools[ii].m_Pool)
			{
				FreePool(m_MemoryPools[ii].m_Pool);
			}
		}

		m_DefaultPool = NULL;
		if (m_MemoryPools)
		{
			free(m_MemoryPools);
			m_MemoryPools = NULL;
		}
		m_nMemoryPools = 0;
		m_nGarbage = (unsigned int)-1;

		g_Vpm.Free();
		
		FreeTLS();
		if (m_hAdvApiLib != NULL)
		{
			FreeLibrary(m_hAdvApiLib);
		}
		if(m_pAllGamesPerfInstance)
		{
			PERF_FREE_INSTANCE(m_pAllGamesPerfInstance);
		}

		m_CS.Free();
	}

	//-------------------------------------------------------------------------
	MEMORYPOOL * GetDefaultPool(
		void)
	{
		return m_DefaultPool;
	}

	//-------------------------------------------------------------------------
	// call once on init for each thread that will use threadpool
	void InitThreadPool(
		void)
	{
		void * ptr = (void *)LocalAlloc(LPTR, sizeof(MEMORY_THREADPOOL));
		ASSERTX(ptr, "LocalAlloc() failed in InitThreadPool()");
		ASSERTX(TlsSetValue(m_dwTlsIndex, ptr), "InitThreadPool() failed to set TLS value.");
	}

	//-------------------------------------------------------------------------
	// call once on exit for each thread that used threadpool
	void FreeThreadPool(
		void)
	{
		void * ptr = TlsGetValue(m_dwTlsIndex);
		if (!ptr)
		{
			LocalFree((HLOCAL)ptr);
		}
	}

	//-------------------------------------------------------------------------
	// call each time to change threadpool
	BOOL SetThreadPool(
		MEMORYPOOL * pool)
	{
		MEMORY_THREADPOOL * threadpool = (MEMORY_THREADPOOL *)TlsGetValue(m_dwTlsIndex);
		if (!threadpool && pool == NULL)
		{
			return TRUE;
		}
		ASSERT_RETFALSE(threadpool);
		threadpool->pool = pool;
		return TRUE;
	}

	//-------------------------------------------------------------------------
	MEMORYPOOL * GetThreadPool(
		void)
	{
		MEMORY_THREADPOOL * threadpool = (MEMORY_THREADPOOL *)TlsGetValue(m_dwTlsIndex);
		if (!threadpool)
		{
			return NULL;
		}
		return threadpool->pool;
	}
};

MEMORY_SYSTEM g_MemorySystem;

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if USE_MEMORY_MANAGER
#define RESOLVE_POOL(pool)				((pool = pool ? pool : (IMemoryManager::GetInstance().GetDefaultAllocator(true) != NULL ? IMemoryManager::GetInstance().GetDefaultAllocator(true) : IMemoryManager::GetInstance().GetDefaultAllocator())) != NULL)

#else
#define RESOLVE_POOL(pool)				((pool = pool ? pool : g_MemorySystem.GetDefaultPool()) != NULL)
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if USE_MEMORY_MANAGER

#if ISVERSION(SERVER_VERSION)
#define INITIAL_POOL_BUCKET_COUNT 4096
#define INITIAL_HEAP_BUCKET_COUNT 16
#else
#define INITIAL_POOL_BUCKET_COUNT 1024
#define INITIAL_HEAP_BUCKET_COUNT 1792
#endif

#if DEBUG_MEMORY_ALLOCATIONS
#define cUseHeaderAndFooter true
#else
#define cUseHeaderAndFooter false
#endif

static CMemoryAllocatorCRT<cUseHeaderAndFooter>* s_DefaultAllocatorCRT = NULL;
static CMemoryAllocatorHEAP<true, cUseHeaderAndFooter, INITIAL_HEAP_BUCKET_COUNT>* s_DefaultAllocatorHEAP = NULL;
static CMemoryAllocatorPOOL<cUseHeaderAndFooter, true, INITIAL_POOL_BUCKET_COUNT>* s_DefaultAllocatorPOOL = NULL;
static CMemoryAllocatorSTACK<cUseHeaderAndFooter, true>* s_StaticAllocatorSTACK = NULL;
static CMemoryAllocatorHEAP<true, cUseHeaderAndFooter, 1024>* s_ScratchAllocatorHEAP = NULL;
static CMemoryAllocatorSTACK<cUseHeaderAndFooter, true>* s_ScratchAllocatorSTACK = NULL;

IMemoryAllocator* g_DefaultAllocatorHEAP = NULL;

#endif // end USE_MEMORY_MANAGER

MEMORYPOOL* g_StaticAllocator = NULL;
MEMORYPOOL* g_ScratchAllocator = NULL;

static bool sServerMemorySetting = false;

#if DEBUG_MEMORY_ALLOCATIONS	
#define	useHeaderAndFooter true
#else
#define useHeaderAndFooter false
#endif	

#if USE_MEMORY_MANAGER
void sMemorySystemInitServer()
{
	IMemoryManager::GetInstance().Init();
		
	// Default CRT Allocator
	//	
	CMemoryAllocatorCRT<useHeaderAndFooter>* crtAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorCRT<useHeaderAndFooter>(L"DefaultCRT");
	ASSERT(crtAllocator->Init(NULL));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(crtAllocator));
	s_DefaultAllocatorCRT = crtAllocator;
	
	// Default POOL allocator
	//	
	CMemoryAllocatorPOOL<useHeaderAndFooter, true, INITIAL_POOL_BUCKET_COUNT>* poolAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorPOOL<useHeaderAndFooter, true, INITIAL_POOL_BUCKET_COUNT>(L"DefaultPOOL");
	ASSERT(poolAllocator->Init(crtAllocator, 32768, false, false));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(poolAllocator));
	s_DefaultAllocatorPOOL = poolAllocator;
	
	IMemoryManager::GetInstance().SetDefaultAllocator(poolAllocator);
}

void sMemorySystemInitClient()
{

	// These are tuned for hellgate
	//
	MEMORY_SIZE defaultHeapInitialSize = 224 * MEGA;
	
	MEMORY_SIZE staticStackInitialSize = 132 * MEGA;
	MEMORY_SIZE staticStackGrowSize = 8 * MEGA;	
	
	MEMORY_SIZE scratchHeapInitialSize = 128 * MEGA;	
	MEMORY_SIZE scratchStackInitialSize = 4 * MEGA;	
	MEMORY_SIZE scratchStackGrowSize = 4 * MEGA;

	if(AppIsTugboat())
	{
		defaultHeapInitialSize = 32 * MEGA;
		staticStackInitialSize = 76 * MEGA;		
		scratchHeapInitialSize = 64 * MEGA;
	}

	IMemoryManager::GetInstance().Init();
			
	// Default CRT Allocator
	//
	CMemoryAllocatorCRT<useHeaderAndFooter>* crtAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorCRT<useHeaderAndFooter>(L"DefaultCRT");
	ASSERT(crtAllocator->Init(NULL));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(crtAllocator));
	s_DefaultAllocatorCRT = crtAllocator;
	
	// Default HEAP allocator
	//
	CMemoryAllocatorHEAP<true, useHeaderAndFooter, INITIAL_HEAP_BUCKET_COUNT>* heapAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorHEAP<true, useHeaderAndFooter, INITIAL_HEAP_BUCKET_COUNT>(L"DefaultHEAP");
	ASSERT(heapAllocator->Init(crtAllocator, 4 * MEGA, defaultHeapInitialSize, 16 * MEGA));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(heapAllocator));
	s_DefaultAllocatorHEAP = heapAllocator;
	g_DefaultAllocatorHEAP = heapAllocator;

	// Default POOL allocator
	//
	CMemoryAllocatorPOOL<useHeaderAndFooter, true, INITIAL_POOL_BUCKET_COUNT>* poolAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorPOOL<useHeaderAndFooter, true, INITIAL_POOL_BUCKET_COUNT>(L"DefaultPOOL");
	ASSERT(poolAllocator->Init(heapAllocator, 4096, true, true));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(poolAllocator));
	s_DefaultAllocatorPOOL = poolAllocator;
	
	IMemoryManager::GetInstance().SetDefaultAllocator(poolAllocator);

	// Static Allocator
	//	
	CMemoryAllocatorSTACK<useHeaderAndFooter, true>* staticAllocator = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorSTACK<useHeaderAndFooter, true>(L"StaticSTACK");
	ASSERT(staticAllocator->Init(poolAllocator, staticStackInitialSize, staticStackGrowSize, staticStackGrowSize));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(staticAllocator));
	s_StaticAllocatorSTACK = staticAllocator;
#if	!ISVERSION(SERVER_VERSION)
	if(c_GetToolMode())

	{  // Hammer doesn't get a static allocator, cause it doesn't need one!
		g_StaticAllocator = poolAllocator;
	}
	else
#endif
	{
		g_StaticAllocator = staticAllocator;
	}
	
	// Scratch Allocator HEAP
	//	
	CMemoryAllocatorHEAP<true, useHeaderAndFooter, 1024>* scratchAllocatorHEAP = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorHEAP<true, useHeaderAndFooter, 1024>(L"ScratchHEAP");
	ASSERT(scratchAllocatorHEAP->Init(poolAllocator, 16 * MEGA, scratchHeapInitialSize, 0));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(scratchAllocatorHEAP));
	s_ScratchAllocatorHEAP = scratchAllocatorHEAP;
	
	// Scratch Allocator STACK
	//
	CMemoryAllocatorSTACK<useHeaderAndFooter, true>* scratchAllocatorSTACK = MEMORYPOOL_NEW(IMemoryManager::GetInstance().GetSystemAllocator()) CMemoryAllocatorSTACK<useHeaderAndFooter, true>(L"ScratchSTACK");
	scratchAllocatorSTACK->Init(scratchAllocatorHEAP, scratchStackInitialSize, scratchStackGrowSize, 4096);
	ASSERT(IMemoryManager::GetInstance().AddAllocator(scratchAllocatorSTACK));	
	s_ScratchAllocatorSTACK = scratchAllocatorSTACK;
		
	g_ScratchAllocator = scratchAllocatorSTACK;	
	
}
#endif

void MemorySystemInit(LPSTR pCmdLine, unsigned int INIT_NODE_COUNT)
{
	REF(INIT_NODE_COUNT);
	REF(pCmdLine);

#if USE_MEMORY_MANAGER
#if ISVERSION(SERVER_VERSION)
	sMemorySystemInitServer();
#else
	// Use server memory settings for any pak building
	//
	if(pCmdLine && PStrStr(pCmdLine, "svrmemory"))	
	{
		sServerMemorySetting = true;
		sMemorySystemInitServer();
	}
	else
	{
		sMemorySystemInitClient();
	}
#endif // end ISVERSION(SERVER_VERSION)
#else	
	g_MemorySystem.Init(INIT_NODE_COUNT);
#endif // end USE_MEMORY_MANAGER
	

#if DEBUG_MEMORY_OVERRUN_GUARD
	DebugMemoryOverrunGuardInit();
#endif
}

#if USE_MEMORY_MANAGER
void sMemorySystemFreeServer()
{
	// Default POOL
	//
	if(s_DefaultAllocatorPOOL)
	{
		s_DefaultAllocatorPOOL->Term();
		IMemoryManager::GetInstance().RemoveAllocator(s_DefaultAllocatorPOOL);
		MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_DefaultAllocatorPOOL);
		s_DefaultAllocatorPOOL = NULL;
	}

	// Default CRT
	//	
	if(s_DefaultAllocatorCRT)
	{
		s_DefaultAllocatorCRT->Term();
		IMemoryManager::GetInstance().RemoveAllocator(s_DefaultAllocatorCRT);
		MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_DefaultAllocatorCRT);
		s_DefaultAllocatorCRT = NULL;
	}

	IMemoryManager::GetInstance().Term();}

void sMemorySystemFreeClient()
{
	// Scratch STACK
	//
	s_ScratchAllocatorSTACK->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_ScratchAllocatorSTACK);	
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_ScratchAllocatorSTACK);
	s_ScratchAllocatorSTACK = NULL;
	g_ScratchAllocator = NULL;

	// Scratch HEAP
	//
	s_ScratchAllocatorHEAP->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_ScratchAllocatorHEAP);
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_ScratchAllocatorHEAP);	
	s_ScratchAllocatorHEAP = NULL;

	// Static STACK
	//
	s_StaticAllocatorSTACK->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_StaticAllocatorSTACK);
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_StaticAllocatorSTACK);	
	s_StaticAllocatorSTACK = NULL;
	g_StaticAllocator = NULL;

	// Default POOL
	//
	s_DefaultAllocatorPOOL->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_DefaultAllocatorPOOL);
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_DefaultAllocatorPOOL);	
	s_DefaultAllocatorPOOL = NULL;

	// Default HEAP
	//
	s_DefaultAllocatorHEAP->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_DefaultAllocatorHEAP);
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_DefaultAllocatorHEAP);	
	s_DefaultAllocatorHEAP = NULL;
	g_DefaultAllocatorHEAP = NULL;

	// Default CRT
	//	
	s_DefaultAllocatorCRT->Term();
	IMemoryManager::GetInstance().RemoveAllocator(s_DefaultAllocatorCRT);
	MEMORYPOOL_DELETE(IMemoryManager::GetInstance().GetSystemAllocator(), s_DefaultAllocatorCRT);	
	s_DefaultAllocatorCRT = NULL;

	IMemoryManager::GetInstance().Term();
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemorySystemFree(void)
{
#if DEBUG_MEMORY_OVERRUN_GUARD
	DebugMemoryOverrunGuardFree();
#endif


#if USE_MEMORY_MANAGER
#if ISVERSION(SERVER_VERSION)
	sMemorySystemFreeServer();
#else
	if(sServerMemorySetting)
	{
		sMemorySystemFreeServer();
	}
	else
	{	
		sMemorySystemFreeClient();
	}
#endif // end ISVERSION(SERVER_VERSION)
#else	
	g_MemorySystem.Free();
#endif // end USE_MEMORY_MANAGER

}


#if !USE_MEMORY_MANAGER
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
MEMORYPOOL * MemoryPoolInit(
	WCHAR * szDesc,
	MEMORY_MANAGEMENT_TYPE pooltype)
{
	return g_MemorySystem.NewPool(szDesc, pooltype);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemoryPoolFree(
	MEMORYPOOL * pool)
{
	g_MemorySystem.FreePool(pool);
}
#endif




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAlloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size))
{
#if DEBUG_MEMORY_OVERRUN_GUARD
	{
		void * ptr = DebugMemoryOverrunGuardAlloc(file, line, size);
		if (ptr)
		{
			return ptr;
		}
	}
#endif

	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* ptr = pool->Alloc(MEMORY_FUNC_PASSFL(size, NULL));
#else
	void* ptr = pool->Alloc(MEMORY_FUNC_PASSFL(size));
#endif	
	
	CHECK_OUT_OF_MEMORY(ptr, size);

	return ptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAllocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size))
{
#if DEBUG_MEMORY_OVERRUN_GUARD
	{
		void * ptr = DebugMemoryOverrunGuardAlloc(file, line, size);
		if (ptr)
		{
			memclear(ptr, size);
			return ptr;
		}
	}
#endif

	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* ptr = pool->AllocZ(MEMORY_FUNC_PASSFL(size, NULL));
#else
	void* ptr = pool->AllocZ(MEMORY_FUNC_PASSFL(size));
#endif	
	
	CHECK_OUT_OF_MEMORY(ptr, size);
	
	
	return ptr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAllocBig(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size))
{
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void * ptr =  pool->Alloc(MEMORY_FUNC_PASSFL(size, NULL));
#else
	void * ptr =  pool->Alloc(MEMORY_FUNC_PASSFL(size));
#endif	
	
	CHECK_OUT_OF_MEMORY(ptr, size);
	

	return ptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAlignedAlloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size,
	unsigned int alignment))
{
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* ptr = pool->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
#else
	void* ptr = pool->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment));
#endif	
	
	CHECK_OUT_OF_MEMORY(ptr, size);
	
	
	return ptr;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAlignedAllocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	MEMORY_SIZE size,
	unsigned int alignment))
{
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* ptr = pool->AlignedAllocZ(MEMORY_FUNC_PASSFL(size, alignment, NULL));
#else
	void* ptr = pool->AlignedAllocZ(MEMORY_FUNC_PASSFL(size, alignment));
#endif	
	
	
	CHECK_OUT_OF_MEMORY(ptr, size);
	return ptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryFree(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr))
{
#if DEBUG_MEMORY_OVERRUN_GUARD
	if (DebugMemoryOverrunGuardFree(ptr))
	{
		return;
	}
#endif
	ASSERT_RETURN(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	pool->Free(MEMORY_FUNC_PASSFL(ptr, NULL));
#else
	pool->Free(MEMORY_FUNC_PASSFL(ptr));
#endif	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MEMORY_SIZE MemorySize(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr))
{
	MEMORY_DEBUG_REF(file);
	MEMORY_DEBUG_REF(line);
	if (!ptr)
	{
		return 0;
	}
	ASSERT_RETZERO(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	return pool->Size(ptr, NULL);
#else
	return pool->Size(ptr);
#endif	
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryRealloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size))
{
#if DEBUG_MEMORY_OVERRUN_GUARD
	{
		void * newptr = DebugMemoryOverrunGuardRealloc(file, line, ptr, size);
		if (newptr)
		{
			return newptr;
		}
	}
#endif
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* newptr = pool->Realloc(MEMORY_FUNC_PASSFL(ptr, size, NULL));
#else
	void* newptr = pool->Realloc(MEMORY_FUNC_PASSFL(ptr, size));
#endif	
	
	CHECK_OUT_OF_MEMORY(newptr, size);
	return newptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryReallocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size))
{
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* newptr = pool->ReallocZ(MEMORY_FUNC_PASSFL(ptr, size, NULL));
#else
	void* newptr = pool->ReallocZ(MEMORY_FUNC_PASSFL(ptr, size));
#endif	
	
	CHECK_OUT_OF_MEMORY(newptr, size);
	return newptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAlignedRealloc(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size,
	unsigned int alignment))
{
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* newptr = pool->AlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, NULL));
#else
	void* newptr = pool->AlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment));
#endif	
	
	CHECK_OUT_OF_MEMORY(newptr, size);
	return newptr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * MemoryAlignedReallocZ(
	MEMORY_FUNCARGS(MEMORYPOOL * pool,
	void * ptr, 
	MEMORY_SIZE size,
	unsigned int alignment))
{
	ASSERT_RETNULL(size < MAX_ALLOCATION_SIZE);
	ASSERT_RETNULL(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	void* newptr = pool->AlignedReallocZ(MEMORY_FUNC_PASSFL(ptr, size, alignment, NULL));
#else
	void* newptr = pool->AlignedReallocZ(MEMORY_FUNC_PASSFL(ptr, size, alignment));
#endif	
	
	CHECK_OUT_OF_MEMORY(newptr, size);
	return newptr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryInitThreadPool(
	void)
{
#if !USE_MEMORY_MANAGER	
	g_MemorySystem.InitThreadPool();
#endif	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryFreeThreadPool(
	void)
{
#if !USE_MEMORY_MANAGER	
	g_MemorySystem.FreeThreadPool();
#endif
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemorySetThreadPool(
	MEMORYPOOL * pool)
{
#if USE_MEMORY_MANAGER
	IMemoryManager::GetInstance().SetPerThreadAllocator(pool);
#else
	g_MemorySystem.SetThreadPool(pool);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MEMORYPOOL * MemoryGetThreadPool(
	void)
{
#if USE_MEMORY_MANAGER
	return IMemoryManager::GetInstance().GetPerThreadAllocator();
#else
	return g_MemorySystem.GetThreadPool();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryTraceFLIDX(
	MEMORYPOOL * pool)
{
	REF(pool);

#if DEBUG_MEMORY_ALLOCATIONS
	ASSERT_RETURN(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	IMemoryManager::GetInstance().MemTrace(NULL, pool);
#else	
	pool->TraceFLIDX();
#endif
	
	
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryTraceAllFLIDX(
	void)
{
#if DEBUG_MEMORY_ALLOCATIONS

#if USE_MEMORY_MANAGER
	IMemoryManager::GetInstance().MemTrace();
#else	
	g_Vpm.MemoryTrace();
	g_MemorySystem.TraceAllFLIDX();
#endif

#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryTraceALLOC(
	MEMORYPOOL * pool)
{
	REF(pool);

#if DEBUG_MEMORY_ALLOCATIONS
	ASSERT_RETURN(RESOLVE_POOL(pool));
	
#if USE_MEMORY_MANAGER
	IMemoryManager::GetInstance().MemTrace(NULL, pool);
#else	
	pool->TraceALLOC();
#endif
	
#endif


}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MemoryTraceAllALLOC(
	void)
{
#if DEBUG_MEMORY_ALLOCATIONS
	
#if USE_MEMORY_MANAGER
	IMemoryManager::GetInstance().MemTrace();
#else	
	g_MemorySystem.TraceAllALLOC();
#endif
	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void memswap(
	void * a,
	void * b,
	int len)
{
	DWORD * dw_a = (DWORD *)a;
	DWORD * dw_b = (DWORD *)b;
	DWORD * dw_end = dw_a + (len & (~3));

	while (dw_a < dw_end)
	{
		DWORD tmp = *dw_a;
		*dw_a = *dw_b;
		*dw_b = tmp;
		++dw_a;
		++dw_b;
	}

	BYTE tmp;
	BYTE * b_a = (BYTE *)dw_a;
	BYTE * b_b = (BYTE *)dw_b;
	switch (len & 3)
	{
	case 3:
		tmp = b_a[0];  b_a[0] = b_b[0];  b_b[0] = tmp;
		tmp = b_a[1];  b_a[1] = b_b[1];  b_b[1] = tmp;
		tmp = b_a[2];  b_a[2] = b_b[2];  b_b[2] = tmp;
		break;
	case 2:
		tmp = b_a[0];  b_a[0] = b_b[0];  b_b[0] = tmp;
		tmp = b_a[1];  b_a[1] = b_b[1];  b_b[1] = tmp;
		break;
	case 1:
		tmp = b_a[0];  b_a[0] = b_b[0];  b_b[0] = tmp;
		break;
	case 0:
		break;
	default: __assume(0);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL MemoryCheckAvail(
	void)
{
#if DEBUG_MEMORY_ALLOCATIONS
	const DWORDLONG ullMinPhys = 10 * 1024 * 1024;  // MB minimum avail phys

	MEMORYSTATUSEX memory_status;
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memory_status);
	DWORDLONG ullAvailPhys = memory_status.ullAvailPhys;

	if (ullAvailPhys < ullMinPhys)
	{
		return -1;
	}
#endif
	return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemoryGetMetrics(
	MEMORYPOOL * pool,
	MEMORY_INFO * info)
{
	memclear(info, sizeof(MEMORY_INFO));
	ASSERT_RETURN(RESOLVE_POOL(pool));

	MEMORYSTATUSEX memory_status;
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memory_status);
	info->ullTotalPhys = memory_status.ullTotalPhys;
	info->ullAvailPhys = memory_status.ullAvailPhys;
	info->ullTotalPageFile = memory_status.ullTotalPageFile;
	info->ullAvailPageFile = memory_status.ullAvailPageFile;
	info->ullTotalVirtual = memory_status.ullTotalVirtual;
	info->ullAvailVirtual = memory_status.ullAvailVirtual;
	info->ullAvailExtendedVirtual = memory_status.ullAvailExtendedVirtual;

	GetProcessHandleCount(GetCurrentProcess(), &info->dwHandleCount);

#if DEBUG_MEMORY_ALLOCATIONS
	info->m_nTotalTotalAllocated = 0;
	info->m_nTotalAllocations = 0;
#if !USE_MEMORY_MANAGER
	pool->TrackWindow();
	pool->GetWindow(info->m_LineWindow, arrsize(info->m_LineWindow));
#endif
#endif
}

int g_MemorySystemInfoCmd = -1;
int g_MemorySystemInfoSetAverageIndex = -1;
int g_MemorySystemInfoMode = 0;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemorySubsystemsTest(
	void)
{
//	FFSTest();
//	BSearchTest();
//	MemBitTest();
//	VirtualPoolManagerTest();
	//MemPoolBinTest();
}


//-----------------------------------------------------------------------------
// threads 1-4 test default (mt) pool, threads 5-8 test st pool
//-----------------------------------------------------------------------------
void MemorySystemTest(
	void)
{
	//MST_TRACE("Start Memory System Test:\n\n");

//#if MEMORY_TEST_ENABLED
	MemorySubsystemsTest();
//#endif

#if MEMORY_TEST_ENABLED
/*
	static const unsigned int DEFAULT_COUNT = 4;
	static const unsigned int BIN_ST_COUNT = 0;
	static const unsigned int BIN_MT_COUNT = 0;
	static const unsigned int CRT_COUNT = 0;
	static const unsigned int LFH_COUNT = 0;
	static const unsigned int THREAD_COUNT = DEFAULT_COUNT + BIN_ST_COUNT + BIN_MT_COUNT + CRT_COUNT + LFH_COUNT;

	volatile long lGo = 0;
	volatile long lDone = THREAD_COUNT;

	MPB_TEST test[THREAD_COUNT];

	for (unsigned int ii = 0; ii < THREAD_COUNT; ++ii)
	{
		if (ii < DEFAULT_COUNT)
		{
			test[ii].pool = NULL;
		}
		else if (ii < DEFAULT_COUNT + BIN_ST_COUNT)
		{
			test[ii].pool = MemoryPoolInit(L"test_st", MEMORY_BINALLOCATOR_ST);
		}
		else if (ii < DEFAULT_COUNT + BIN_ST_COUNT + BIN_MT_COUNT)
		{
			test[ii].pool = MemoryPoolInit(L"test_mt", MEMORY_BINALLOCATOR_MT);
		}
		else if (ii < DEFAULT_COUNT + BIN_ST_COUNT + BIN_MT_COUNT + CRT_COUNT)
		{
			test[ii].pool = MemoryPoolInit(L"test_crt", MEMORY_CRTALLOCATOR);
		}
		else
		{
			test[ii].pool = MemoryPoolInit(L"test_lfh", MEMORY_LFHALLOCATOR);
		}
		test[ii].lGo = &lGo;
		test[ii].lDone = &lDone;
		test[ii].index = ii;

		DWORD dwThreadId;
		HANDLE handle = CreateThread(NULL, 0, MemPoolBinTestThread, (void *)&test[ii], 0, &dwThreadId);
		CloseHandle(handle);
	}

	DWORD start = GetTickCount();

	lGo = 1;

	while (lDone != 0)
	{
		Sleep(10);
	}

	for (unsigned int ii = 0; ii < THREAD_COUNT; ++ii)
	{
		if (test[ii].pool)
		{
			MemoryPoolFree(test[ii].pool);
		}
	}

	MST_TRACE("End Memory System Test:  Time:%d\n\n", GetTickCount() - start);
*/	
	//MST_TRACE("End Memory System Test");
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemoryDumpCRTStats(
	void)
{
#ifdef _DEBUG
	_CrtMemState tState;
	_CrtMemCheckpoint(&tState);
	//_CrtMemDumpAllObjectsSince(NULL);
	_CrtMemDumpStatistics(&tState);
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void MemoryTraceHeapError(
	int heapstatus,
	BOOL bTrace = TRUE)
{
	switch (heapstatus)
	{
	case _HEAPEMPTY:
		if (bTrace)
		{
#if DEBUG_SINGLE_ALLOCATIONS
			PrintForWpp("OK - empty heap\n");
#endif
		}
		break;
	case _HEAPEND:
		if (bTrace)
		{
#if DEBUG_SINGLE_ALLOCATIONS
			PrintForWpp("OK - end of heap\n");
#endif
		}
		break;
	case _HEAPBADPTR:
		ASSERTV_MSG("ERROR - bad pointer to heap\n");
		break;
	case _HEAPBADBEGIN:
		ASSERTV_MSG("ERROR - bad start of heap\n");
		break;
	case _HEAPBADNODE:
		ASSERTV_MSG("ERROR - bad node in heap\n");
		break;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemoryHeapDump(
	void)
{
#if DEBUG_SINGLE_ALLOCATIONS
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = NULL;
	while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{ 
	   PrintForWpp("%6s block at %Fp of size %4.4X\n", (hinfo._useflag == _USEDENTRY ? "USED" : "FREE"), hinfo._pentry, hinfo._size);
	}
	MemoryTraceHeapError(heapstatus);
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void MemoryHeapCheck(
	void)
{
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = NULL;
	while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		;
	}
	MemoryTraceHeapError(heapstatus, FALSE);
}


#if DUMP_HEAP
//-----------------------------------------------------------------------------
// walks a win32 heap
//-----------------------------------------------------------------------------
void DumpHeapInfo(
	HEAPLIST32 & heap)
{
    HEAPENTRY32 he = {0};
    he.dwSize = sizeof(he);

    SIZE_T total = 0;

    ASSERT_RETURN(Heap32First(&he,heap.th32ProcessID,heap.th32HeapID));

    do {
       total = total + he.dwBlockSize;
    } while(Heap32Next(&he));

    PrintForWpp("heap 0x%08x, bytes %d\n", heap.th32HeapID,total);
}


//-----------------------------------------------------------------------------
// walks the win32 heaps. takes a loooooong time, so just don't do it.
//-----------------------------------------------------------------------------
void MemorySystemDumpWin32HeapInformation(
	void)
{
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST,0);
    if (hsnap == INVALID_HANDLE_VALUE)
    {
        PrintForWpp("snap failed : %d\n",GetLastError());
        return;
    }
    HEAPLIST32 heapList = {0};
    heapList.dwSize = sizeof(heapList);

    ASSERT_RETURN(Heap32ListFirst(hsnap,&heapList));
    do{
        PrintForWpp("heap %d\n",heapList.th32HeapID);
      //  DumpHeapInfo(heapList);
    }while(Heap32ListNext(hsnap,&heapList));
     
}
#else //!DUMP_HEAP
#define MemorySystemDumpWin32HeapInformation()
#endif //!DUMP_HEAP

