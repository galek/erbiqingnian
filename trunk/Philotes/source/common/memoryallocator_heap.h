//----------------------------------------------------------------------------
// memoryallocator_heap.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator.h"
#include "memoryheapbucket.h"
#include "pool.h"
#include "sortedarray.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memoryallocator_heap.h.tmh"
#endif

#pragma warning(push)
#pragma warning(disable:4189)

namespace FSCommon
{



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorHEAP
//
// Parameters
//	MT_SAFE : Specifies if locks are taken when accessing the internals
//	USE_HEADER_AND_FOOTER : Adds header/footer information to each allocation
//	BUCKET_SIZE : The size of each bucket in the heap
//	BLOCK_COUNT : The number of blocks per bucket
//	INITIAL_BUCKET_COUNT : The number of buckets allocated as member data for 
//		the heap.  The bucket count can increase beyond this value, but it
//		is best to set it accordingly.
//
// Remarks
//	The minimum allocation size is determined by the BUCKET_SIZE / BLOCK_COUNT
// 
/////////////////////////////////////////////////////////////////////////////	
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE = 131072, MEMORY_SIZE BLOCK_COUNT = 32>
class CMemoryAllocatorHEAP : public CMemoryAllocator
{	
	typedef CSortedArray<MEMORY_SIZE, CMemoryHeapBucket<BLOCK_COUNT>*, false> HEAP_FREE_LIST;
	typedef CSortedArray<BYTE*, CMemoryHeapBucket<BLOCK_COUNT>*, false> HEAP_USED_LIST;
	typedef CPool<CMemoryHeapBucket<BLOCK_COUNT>, false> BUCKET_POOL;

	// ALLOCATION_HEADER
	//
	#pragma pack(push, 1)
	
	struct ALLOCATION_SIZE_HEADER
	{
		MEMORY_SIZE								ExternalAllocationSize;
	};
				
	struct ALLOCATION_HEADER : public CMemoryAllocator::ALLOCATION_HEADER, public ALLOCATION_SIZE_HEADER
	{
		DWORD									MagicHeader;
	};
	#pragma pack(pop)		

	public:
	
		CMemoryAllocatorHEAP(const WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);
		virtual ~CMemoryAllocatorHEAP();
		
	public: // IMemoryAllocator
	
		virtual bool							Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, MEMORY_SIZE initialSize, MEMORY_SIZE growSize);
		virtual bool							Term();
		
		// Alloc 
		//
		virtual void*							AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
		// Free
		//
		virtual void							Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator));
		
		// Size
		//
		virtual MEMORY_SIZE						Size(void* ptr, IMemoryAllocator* fallbackAllocator);
		
	public: // IMemoryAllocatorInternals
	
		virtual void*							GetInternalPointer(void* ptr);
		virtual MEMORY_SIZE						GetInternalSize(void* ptr);	
		virtual void							GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics);						

	protected:
	
		virtual void							MemTraceInternalsEx();
	
	private:
	
		bool									Term(bool useLock);
		bool									GrowHeap(MEMORY_SIZE growSize);
		void									RecreateFreeList();
		MEMORY_SIZE								GetMaxBytesRequiredForAlignmentAndHeader(unsigned int alignment); 
		MEMORY_SIZE								GetHeaderSize();
		MEMORY_SIZE								GetFooterSize();
		void									SortFreeListAfterGetBlock(CMemoryHeapBucket<BLOCK_COUNT>* sortBucket, unsigned int sortBackCount, CMemoryHeapBucket<BLOCK_COUNT>* frontBlockChangedBucket, MEMORY_SIZE oldFrontBlockSize);
		void									SortFreeListAfterPutBlock(CMemoryHeapBucket<BLOCK_COUNT>* sortBucket, unsigned int sortBackCount, CMemoryHeapBucket<BLOCK_COUNT>* frontBlockChangedBucket, MEMORY_SIZE oldFrontBlockSize);
		MEMORY_SIZE								SortBucket(CMemoryHeapBucket<BLOCK_COUNT>* bucket, MEMORY_SIZE* nextBucketFrontAllocationSize);
		
		// Debug
		//	
		void									VerifyFreeList();	
		
	private:

		// The smallest external allocation size in bytes that will be allowed
		//
		MEMORY_SIZE								m_MinAllocationSize;

		// The maximum external allocation size in bytes that will be allowed
		//
		MEMORY_SIZE								m_MaxAllocationSize;
		MEMORY_SIZE								m_MaxInternalAllocationSize;

		// The initial size that will be allocated to the heap
		//
		MEMORY_SIZE								m_InitialSize;
		
		// The number of bytes that the heap will grow by when there are no free blocks
		// capable of servicing an allocation request
		//
		MEMORY_SIZE								m_GrowSize;
			
		// An array of buckets sorted by their max allocation size.  Allocating
		// a block of memory involves searching this list for a bucket with
		// a max allocation size just big enough for the requested allocation size.
		// Whenever a bucket's max allocation size changes, it needs to have it's
		// entry in this list updated.
		//
		HEAP_FREE_LIST							m_FreeList;
		BYTE									m_FreeListMemory[INITIAL_BUCKET_COUNT * sizeof(HEAP_FREE_LIST::ARRAY_NODE)];

		// An array of buckets sorted by their memory pointers.  This array contains
		// all buckets and is used when freeing a pointer.  Each used block is stored
		// in the bucket itself, so this list is only used to lookup the bucket pointer
		// given a particular external allocation pointer
		//
		HEAP_USED_LIST							m_UsedList;
		BYTE									m_UsedListMemory[INITIAL_BUCKET_COUNT * sizeof(HEAP_USED_LIST::ARRAY_NODE)];

		BUCKET_POOL								m_BucketPool;
		BYTE									m_BucketPoolMemory[INITIAL_BUCKET_COUNT * sizeof(CMemoryHeapBucket<BLOCK_COUNT>)];

};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorHEAP Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::CMemoryAllocatorHEAP(const WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocator(MEMORY_ALLOCATOR_HEAP, name, defaultAlignment),
	m_BucketPool(IMemoryManager::GetInstance().GetSystemAllocator()),
	m_UsedList(IMemoryManager::GetInstance().GetSystemAllocator()),
	m_FreeList(IMemoryManager::GetInstance().GetSystemAllocator())
{
	m_FreeList.Init(&m_FreeListMemory, INITIAL_BUCKET_COUNT, INITIAL_BUCKET_COUNT);
	m_UsedList.Init(&m_UsedListMemory, INITIAL_BUCKET_COUNT, INITIAL_BUCKET_COUNT);
	ASSERT(m_BucketPool.Init(&m_BucketPoolMemory, sizeof(m_BucketPoolMemory)) == INITIAL_BUCKET_COUNT);

	// Initialize critical section
	//
	if(MT_SAFE)
	{
		m_CS.Init();
	}	
	
	m_MinAllocationSize = BUCKET_SIZE / BLOCK_COUNT;
	
	m_InternalOverhead += sizeof(CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorHEAP Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::~CMemoryAllocatorHEAP()
{
	Term();
	
	if(MT_SAFE)
	{
		m_CS.Free();
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	fallbackAllocator : A pointer to the allocator that will be used if the heap runs out of 
//		space or if a Free is done on a pointer that does not belong to the heap
//	maxAllocationSize : The maximum external allocation size that will be requested.
//		External allocations beyond this size will assert and return NULL.  This 
//		limits traversals of internal lists when sizes of buckets/blocks change.
//		Specify 0 for no max allocation size
//	initialSize : The initial size of the heap
//	growSize : The number of bytes that the heap will grow by when it runs
//		out of space.  Specifying zero means the heap will not grow.
//
// Returns
//	true if the method succeeds, false otherwise
//
// Remarks
//	If growSize is not big enough to satisfy an adjusted maxAllocationSize 
//	due to header/alignment, it could cause the heap to grow out of control
//	since it will always attempt to grow, but then fail to allocate.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
bool CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, MEMORY_SIZE initialSize, MEMORY_SIZE growSize)
{
	ASSERTV(growSize == 0 || growSize > maxAllocationSize, "Possible heap setup that can grow unbounded. - Didier");

	Term();
	
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);
	
	if(m_Initialized == false)
	{
		__super::Init(fallbackAllocator);

		m_MaxAllocationSize = maxAllocationSize;
		m_MaxInternalAllocationSize = m_MaxAllocationSize + GetHeaderSize() + GetFooterSize() + (4 * m_DefaultAlignment);
		
		m_InitialSize = initialSize;
		m_GrowSize = growSize;

		// Allocate all buckets up front
		//
		unsigned int bucketCount = (m_InitialSize + (BUCKET_SIZE - 1)) / BUCKET_SIZE;		
		ASSERTV(bucketCount <= INITIAL_BUCKET_COUNT, "Initial heap bucket count too low. - Didier");
		
		if(GrowHeap(m_InitialSize) == false)
		{
			Term(false);
			return false;
		}

		m_Initialized = true;		
	}
	
#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyFreeList();	
#endif
	
	return m_Initialized;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
bool CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::Term()
{
	return Term(true);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GrowHeap
//
// Parameters
//	growSize : The number of bytes to grow the heap by
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
bool CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GrowHeap(MEMORY_SIZE growSize)
{
	// Determine how many additional buckets are needed
	//
	unsigned int bucketCount = (growSize + (BUCKET_SIZE - 1)) / BUCKET_SIZE;		
		
	// Since there might not be enough contiguous memory to allocate the entire additional size in one virtual address block,
	// attempt to allocate as much as possible contiguously until the entire size has been allocated
	//
	while(bucketCount)
	{
		unsigned int bucketCountAttempt = bucketCount;

		// Try to allocate as many contiguous buckets as possible
		//
		BYTE* memory = NULL;
		while((memory = reinterpret_cast<BYTE*>(InternalAlloc(MEMORY_FUNC_FILELINE(bucketCountAttempt * BUCKET_SIZE)))) == NULL)
		{
			--bucketCountAttempt;

			// If we couldn't allocate a single bucket, we can't initialize
			//
			if(bucketCountAttempt == 0)
			{
				// Already allocated buckets will be cleaned up when the heap terminates
				//
				break;
			}
		}

		// Unable to allocate entire heap size
		//
		if(bucketCountAttempt == 0)
		{
			if(memory)
			{
				InternalFree(MEMORY_FUNC_FILELINE(memory, 0));				
			}
										
			return false;
		}
		else
		{
			m_InternalFragmentation += bucketCountAttempt * BUCKET_SIZE;
		}

		// Gets set to the index of the first bucket to be added to the used list.
		// Since successive attempts to allocate virtual memory may be randomly dispersed in virtual
		// space, spans of buckets in the used list aren't necessarily sorted in the order in which they
		// were allocated.
		//
		unsigned int usedListSpanStart = cHeapBlockInvalid;  

		// Create buckets for the allocated memory
		//			
		for(unsigned int i = 0; i < bucketCountAttempt; ++i)
		{
			// Grab a free heap bucket from the pool
			//
			CMemoryHeapBucket<BLOCK_COUNT>* bucket = m_BucketPool.Get();
			
			// Use placement new to construct the heap bucket since the pool only 
			// allocates memory
			//
			bucket = new(bucket) CMemoryHeapBucket<BLOCK_COUNT>();

			// Initialize the bucket
			//
			if(bucket->Init(memory, BUCKET_SIZE, m_MaxInternalAllocationSize, (i == 0)) == false)
			{
				return false;
			}

			// Add the bucket to the used list
			//
			unsigned int blockIndex = cHeapBlockInvalid;
			m_UsedList.Insert(memory, bucket, &blockIndex);
			
			// Store the first inserted index so we can link the buckets starting from here
			//
			if(i == 0)
			{
				usedListSpanStart = blockIndex;
			}

			memory += BUCKET_SIZE;
		}

		// Link the buckets
		//
		CMemoryHeapBucket<BLOCK_COUNT>* bucket = NULL;
		CMemoryHeapBucket<BLOCK_COUNT>* next = NULL;
		CMemoryHeapBucket<BLOCK_COUNT>* prev = NULL;
		for(unsigned int i = usedListSpanStart; i < (usedListSpanStart + bucketCountAttempt); ++i)
		{
			bucket = *m_UsedList[i];
			next = i < (m_UsedList.GetCount() - 1) ? *m_UsedList[i + 1] : NULL;
			prev = i > 0 ? *m_UsedList[i - 1] : NULL;

			// Don't link buckets that are not contiguous
			//
			if(prev && prev->m_Memory + prev->m_MemorySize != bucket->m_Memory)
			{
				prev = NULL;
			}
			
			if(next && bucket->m_Memory + bucket->m_MemorySize != next->m_Memory)
			{
				next = NULL;
			}

			bucket->SetNextBucket(next);
			bucket->SetPrevBucket(prev);
		}
		
		// If a bucket exists before the first bucket just added and it is contiguous, link it up as well
		//
		if(usedListSpanStart && (*m_UsedList[usedListSpanStart - 1])->m_Memory + (*m_UsedList[usedListSpanStart - 1])->m_MemorySize == (*m_UsedList[usedListSpanStart])->m_Memory)
		{
			(*m_UsedList[usedListSpanStart - 1])->SetNextBucket((*m_UsedList[usedListSpanStart]));
		}
		
		// If a bucket exists after the last bucket just added and it is contiguous, link it up as well
		//
		if(bucket && next && bucket->m_Memory + bucket->m_MemorySize == next->m_Memory)
		{
			next->SetPrevBucket(bucket);
		}
		
		RecreateFreeList();

		// Continue trying to create non-contiguous buckets if the bucket count
		// was not satisfied
		//
		bucketCount -= bucketCountAttempt;
	}
	
	//ASSERTV(m_BucketPool.GetUsedCount() <= INITIAL_BUCKET_COUNT, "Bucket count too small in heap allocator: %S. - Didier", m_Name);							

#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyFreeList();	
#endif
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RecreateFreeList
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::RecreateFreeList()
{
	m_FreeList.RemoveAll();	
	MEMORY_SIZE nextBucketFrontAllocationSize = 0;
	for(unsigned int i = m_UsedList.GetCount() - 1; i != 0xFFFFFFFF; --i) // Iterate from the back to the front in memory addresses
	{
		CMemoryHeapBucket<BLOCK_COUNT>* bucket = *m_UsedList[i];
		
		MEMORY_SIZE maxAllocation = bucket->GetMaxAllocationSize(&nextBucketFrontAllocationSize);
		DBG_ASSERT(maxAllocation == bucket->GetMaxAllocationSize(NULL));		
		
		m_FreeList.Insert(maxAllocation, bucket);

		// The front free block spans the entire bucket and therefore spills into
		// the adjacent bucket.  The size is equal to the max allocation size
		//
		if(bucket->m_FrontFreeBlockIndex != cHeapBlockInvalid && bucket->m_BackFreeBlockIndex != cHeapBlockInvalid &&
			bucket->m_FrontFreeBlockIndex == bucket->m_BackFreeBlockIndex)
		{
			nextBucketFrontAllocationSize = maxAllocation;
		}
		// The front free block ends within the bucket
		//
		else if(bucket->m_FrontFreeBlockIndex != cHeapBlockInvalid)
		{
			nextBucketFrontAllocationSize = bucket->m_FreeBlocks[bucket->m_FrontFreeBlockIndex]->InternalSize;	
		}
		else
		{
			nextBucketFrontAllocationSize = 0;
		}		
		
		// Account for discontinuous buckets due to multiple virtual allocs when creating
		// the heap
		//
		if(bucket->Prev == NULL)
		{
			nextBucketFrontAllocationSize = 0;
		}				
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedAlloc
//
// Parameters
//	size : The size of the external allocation request for which a pointer will be returned.
//	alignment : The size in bytes for which the returned pointer must be aligned
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	A pointer to memory for the allocation request.  NULL if the allocation fails.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void* CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{

	if(size == 0 || m_Initialized == false)
	{
		return NULL;
	}

	// Make sure the allocation request isn't over the max allocation size
	//
	if(m_MaxAllocationSize != 0 && size > m_MaxAllocationSize)
	{
		if(fallbackAllocator)
		{
			return fallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
		}	
		else if(m_FallbackAllocator)
		{
			return m_FallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
		}
		else
		{
			return NULL;
		}
	}	
	
	BYTE* externalAllocationPtr = NULL;
	
	// When searching through the heap free list, we need to determine the max size required for a given alignment
	//
	MEMORY_SIZE maxInternalSize = size + GetMaxBytesRequiredForAlignmentAndHeader(alignment) + GetFooterSize();
	
	// Enforce minimum allocation size
	//
	maxInternalSize = MAX<MEMORY_SIZE>(maxInternalSize, m_MinAllocationSize);
	
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);	
	
	// Search through the heap free list for a bucket with a free block large enough to satisfy the internal request
	//	
	unsigned int bucketIndex = 0xFFFFFFFF;
	CMemoryHeapBucket<BLOCK_COUNT>** ppbucket = m_FreeList.FindOrAfter(maxInternalSize, &bucketIndex);
	
	// Increase the size of the heap if there is no bucket with a block large enough to satisfy the allocation request
	//
	if(ppbucket == NULL && m_GrowSize)
	{				
		GrowHeap(m_GrowSize);
	
		ppbucket = m_FreeList.FindOrAfter(maxInternalSize, &bucketIndex);
	}
	
	if(ppbucket)
	{
		CMemoryHeapBucket<BLOCK_COUNT>* bucket = *ppbucket;

		CMemoryHeapBucket<BLOCK_COUNT>* sortBucket = NULL;
		unsigned int sortBackCount = 0;
		bool frontBlockChanged = false;
		MEMORY_SIZE adjustedSize = maxInternalSize;
		MEMORY_SIZE totalAdjustedSize = maxInternalSize;
		MEMORY_SIZE oldFrontBlockSize = 0;
		BYTE* internalAllocationPtr = bucket->GetBlock(maxInternalSize, adjustedSize, totalAdjustedSize, false, sortBucket, sortBackCount, frontBlockChanged, oldFrontBlockSize);
		DBG_ASSERT(internalAllocationPtr);
		
		// Resort the bucket in the free list due to changes in it's max allocation size
		//
		if(sortBucket || frontBlockChanged)
		{			
			SortFreeListAfterGetBlock(sortBucket, sortBackCount, frontBlockChanged ? bucket : NULL, oldFrontBlockSize);
		}
						
		// Adjust the pointer so that it is aligned and far enough ahead for the header
		//
		externalAllocationPtr = internalAllocationPtr + GetHeaderSize();
		externalAllocationPtr += ((alignment - (PTR_TO_SCALAR(externalAllocationPtr) % alignment)) % alignment);
		DBG_ASSERT(PTR_TO_SCALAR(externalAllocationPtr) % alignment == 0);
		
		// Set the header/footer
		//
		if(USE_HEADER_AND_FOOTER)
		{
			ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));
			allocationHeader->Origin = m_AllocationTracker.OnAlloc(MEMORY_FUNC_PASSFL(externalAllocationPtr, size));			
			allocationHeader->ExternalAllocationSize = size;
			allocationHeader->MagicHeader = cMagicHeader;
			
			ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + size);
			allocationFooter->MagicFooter = cMagicFooter;
			
		}
		else
		{
			ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_SIZE_HEADER));
			allocationHeader->ExternalAllocationSize = size;			
		}
		
		// Update metrics
		//
		m_InternalOverhead += totalAdjustedSize - size;
		m_TotalExternalAllocationSize += size;	
		m_TotalExternalAllocationSizePeak = MAX<MEM_COUNTER>(m_TotalExternalAllocationSize, m_TotalExternalAllocationSizePeak);						
		++m_TotalExternalAllocationCount;	
		m_InternalFragmentation -= totalAdjustedSize;
		
		// Server counters
		//
		if(m_PerfInstance)
		{
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, maxInternalSize);
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalAllocationRate, 1);
		}
		
	}
	// Use the fall back allocator if possible
	//
	else if(fallbackAllocator) 
	{
		return fallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}	
	else if(m_FallbackAllocator) 
	{
		return m_FallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}
	
#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyFreeList();	
#endif
	
	DBG_ASSERT((size_t)externalAllocationPtr % alignment == 0);
	return externalAllocationPtr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
// Parameters
//	ptr : The pointer to free
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator))
{
	if(ptr == NULL || m_Initialized == false)
	{
		return;
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);

	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);	

	unsigned int blockIndex = 0xFFFFFFFF;
	CMemoryHeapBucket<BLOCK_COUNT>** ppBucket = m_UsedList.FindOrBefore(externalAllocationPtr, &blockIndex);

	if(ppBucket && (*ppBucket)->IsInside(externalAllocationPtr))
	{
		MEMORY_SIZE internalSize = (*ppBucket)->GetInternalSize(externalAllocationPtr);
		MEMORY_SIZE externalSize = 0;
		
		// Look for double frees
		//
		ASSERTV_RETURN(internalSize, "Double free detected. - Didier");
	
		if(USE_HEADER_AND_FOOTER)
		{
			ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));

			if(allocationHeader->MagicHeader != cMagicHeader)
			{
				// Throw an exception and let the process terminate
				//
				RaiseException(EXCEPTION_CODE_MEMORY_CORRUPTION, 0, 0, 0);
				return;		
			}
			
			// Set the expiration location in the header
			//
#if DEBUG_MEMORY_ALLOCATIONS			
			allocationHeader->ExpirationFile = file;
			allocationHeader->ExpirationLine = line;
#endif

			ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + allocationHeader->ExternalAllocationSize);

			if(allocationFooter->MagicFooter != cMagicFooter)
			{
				// Throw an exception and let the process terminate
				//
				RaiseException(EXCEPTION_CODE_MEMORY_CORRUPTION, 0, 0, 0);	
				return;	
			}

			m_AllocationTracker.OnFree(MEMORY_FUNC_PASSFL(ptr, allocationHeader->ExternalAllocationSize, allocationHeader->Origin));
					
			externalSize = allocationHeader->ExternalAllocationSize;			
		}
		else
		{
			ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_SIZE_HEADER));
			externalSize = allocationHeader->ExternalAllocationSize;
		}

		CMemoryHeapBucket<BLOCK_COUNT>* sortBucket = NULL;
		unsigned int sortBackCount = 0;
		bool frontBlockChanged = false;
		MEMORY_SIZE oldFrontBlockSize = 0;
		MEMORY_SIZE blockSize = (*ppBucket)->PutBlock(externalAllocationPtr, false, sortBucket, sortBackCount, frontBlockChanged, oldFrontBlockSize);
		
		// Resort the bucket in the free list due to changes in it's max allocation size
		//
		if(sortBucket || frontBlockChanged)
		{
			SortFreeListAfterPutBlock(sortBucket, sortBackCount, frontBlockChanged ? (*ppBucket) : NULL, oldFrontBlockSize);
		}

		// Adjust metrics
		//
		m_InternalOverhead -= internalSize - externalSize;
		m_TotalExternalAllocationSize -= externalSize;							
		--m_TotalExternalAllocationCount;
		m_InternalFragmentation += internalSize;	

		// Server counters
		//
		if(m_PerfInstance)
		{
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, -(signed)blockSize);
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalFreeRate, 1);
		}
		
	}
	else if(fallbackAllocator)
	{
		fallbackAllocator->Free(MEMORY_FUNC_PASSFL(ptr, NULL));			
	}	
	else if(m_FallbackAllocator)
	{
		m_FallbackAllocator->Free(MEMORY_FUNC_PASSFL(ptr, NULL));			
	}
	else
	{
		// The memory is going to be leaked
		//
		DBG_ASSERT(0);
	}
	
#if VERIFY_MEMORY_ALLOCATIONS	
	VerifyFreeList();	
#endif
	
}	
	
/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Size
//
// Parameters
//	ptr : The pointer to return the external allocation size for
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::Size(void* ptr, IMemoryAllocator* fallbackAllocator)
{
	// Size information is only available with the header enabled
	//
	if(ptr == NULL)
	{
		return 0;
	}
	
	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);

	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);	

	// Find a bucket that might hold the pointer
	//
	CMemoryHeapBucket<BLOCK_COUNT>** ppBucket = m_UsedList.FindOrBefore(externalAllocationPtr);

	if(ppBucket && (*ppBucket)->IsInside(externalAllocationPtr))
	{
		if(USE_HEADER_AND_FOOTER)
		{		
			ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));		
			ASSERT(allocationHeader->MagicHeader == cMagicHeader);
			return allocationHeader->ExternalAllocationSize;
		}
		else
		{
			ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_SIZE_HEADER));					
			return allocationHeader->ExternalAllocationSize;
		}

	}
	else if(fallbackAllocator)
	{
		return fallbackAllocator->Size(ptr, NULL);			
	}	
	else if(m_FallbackAllocator)
	{
		return m_FallbackAllocator->Size(ptr, NULL);			
	}
	else
	{
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternalPointer
// 
// Parameters
//	ptr : The external allocation pointer for which the internal pointer will be returned
//
// Returns
//	A pointer to the internal allocation or NULL if the pointer is not
//	held by the allocator
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void* CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetInternalPointer(void* ptr)
{
	if(ptr == NULL)
	{
		return NULL;
	}
	
	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);

	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);	

	CMemoryHeapBucket<BLOCK_COUNT>** ppBucket = m_UsedList.FindOrBefore(externalAllocationPtr);

	if(ppBucket && (*ppBucket)->IsInside(externalAllocationPtr))
	{
		return (*ppBucket)->GetInternalPointer(externalAllocationPtr);
	}
	else if(m_FallbackAllocator)
	{
		return m_FallbackAllocator->GetInternals()->GetInternalPointer(ptr);		
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternalSize
// 
// Parameters
//	ptr : The external allocation pointer for which the internal size will be returned
//
// Returns
//	The size of the internal allocation. Asserts and returns zero if the
//	pointer is not within the allocator or if headers are not enabled
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetInternalSize(void* ptr)
{
	if(ptr == NULL)
	{
		ASSERT(0);
		return 0;		
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);

	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);	

	CMemoryHeapBucket<BLOCK_COUNT>** ppBucket = m_UsedList.FindOrBefore(externalAllocationPtr);

	if(ppBucket && (*ppBucket)->IsInside(externalAllocationPtr))
	{
		return (*ppBucket)->GetInternalSize(externalAllocationPtr);
	}
	else if(m_FallbackAllocator)
	{
		return m_FallbackAllocator->GetInternals()->GetInternalSize(ptr);		
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetMetrics
//
// Parameters
//	metrics : [out] A metrics structure that will be filled in with
//		the allocators information
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics)
{
	__super::GetMetrics(metrics);
	
	if(m_FreeList.GetCount())
	{
		metrics.LargestFreeBlock = *m_FreeList.GetKey(m_FreeList.GetCount() - 1);
	}
	
	float externalFragmentation = 0.0f;
	
	UINT64 regionSize = 0;				
		
	// Traverse the used list which contains the ordered-by-address list of all buckets
	//
	for(unsigned int i = 0; i < m_UsedList.GetCount(); ++i)
	{
		CMemoryHeapBucket<BLOCK_COUNT>* bucket = *m_UsedList[i];
		
		// Determine the size of the next free region which could span multiple buckets
		//
		BYTE* regionStart = bucket->m_Memory;
		
		for(unsigned int j = 0; j < bucket->m_UsedBlocks.GetCount(); ++j)
		{			
			// If the next used block is signifying a free block or if the used block is the front of the bucket and there
			// was a region size carried over from the previous bucket
			//
			if(bucket->m_UsedBlocks[j]->Memory != regionStart || (bucket->m_UsedBlocks[j]->Memory == bucket->m_Memory && regionSize))
			{
				UINT64 freeBlockSize = bucket->m_UsedBlocks[j]->Memory - regionStart;

				regionSize += freeBlockSize;
				
				// Apply fragment algorithm to the region size
				//
				externalFragmentation += CalculateFragmentationForRegion(regionSize, m_MinAllocationSize, m_MaxAllocationSize, m_InternalSize);
				
				// Skip past the free block
				//
				regionStart += freeBlockSize;

				// Reset the region size
				//
				regionSize = 0;
			}
			
			// Skip past the used block
			//
			regionStart += bucket->m_UsedBlocks[j]->InternalSize;			
		}
		
		// Add the size of the back free block to the region size if it exists
		//
		if(bucket->m_BackFreeBlockIndex != cHeapBlockInvalid)
		{
			regionSize += bucket->m_FreeBlocks[bucket->m_BackFreeBlockIndex]->InternalSize;
		}
		
		// If there is a discontinuity in the bucket linked list, it means they are not contiguous,
		// so apply the fragment algorithm if there is a positive regionSize
		//
		if(bucket->Next == NULL && regionSize)
		{
			// Apply fragment algorithm
			//
			externalFragmentation += CalculateFragmentationForRegion(regionSize, m_MinAllocationSize, m_MaxAllocationSize, m_InternalSize);
			
			// Reset the region size
			//
			regionSize = 0;
		}
	}	
	
	externalFragmentation = (1.0f - externalFragmentation) * 100;
	
	metrics.ExternalFragmentation = (MEM_COUNTER)externalFragmentation;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternalsEx
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::MemTraceInternalsEx()
{
	MemPrint("\t\t<internalsEx MinimumAllocationSize='%u' MaximumAllocationSize='%u' MaximumInternalAllocationSize='%u' InitialSize='%u' GrowSize='%u'/>\n", 
		m_MinAllocationSize, m_MaxAllocationSize, m_MaxInternalAllocationSize, m_InitialSize, m_GrowSize);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
// Parameters
//	useLock : Specifies whether the lock is taken while terminating.
//
// Returns
//	true if the method succeeds, false otherwise
//
// Remarks
//	This method is used to terminate internally when when the lock is already
//	held
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
bool CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::Term(bool useLock)
{
	if(m_Initialized)
	{
		__super::Term();
		
		CSLAutoLock lock((useLock && MT_SAFE) ? &m_CS : NULL);
			
		// Terminate and destruct all buckets in the used list
		//
		for(unsigned int i = 0; i < m_UsedList.GetCount(); ++i)
		{
			CMemoryHeapBucket<BLOCK_COUNT>* bucket = *m_UsedList[i];

			bucket->Term();
			
			bucket->~CMemoryHeapBucket();
			
			m_BucketPool.Put(bucket);
		}
		
		// Free the bucket pool
		//
		m_BucketPool.FreeAll();

		// Cleanup the free list
		//
		m_FreeList.FreeAll();

		// Cleanup the used list
		//
		m_UsedList.FreeAll();
	}
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetMaxBytesRequiredForAlignmentAndHeader
//
// Parameters
//	alignment : The alignment for which the max bytes for header and 
//		alignment will be returned
//
// Returns
//	The maximum number of bytes required for the specified alignment and
//	current header size.  This is needed when determining where to look for
//	a potential free block.  The actual amount of bytes used may be less,
//	but that won't be known until the actual internal pointer alignment
//	is known.  When searching the heap's free list, use the max size.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE	CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetMaxBytesRequiredForAlignmentAndHeader(unsigned int alignment)
{
	MEMORY_SIZE headerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_HEADER) : sizeof(ALLOCATION_SIZE_HEADER);

	return (((headerSize / alignment) + 1) * alignment) + ((headerSize % alignment) - 1);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetHeaderSize
//
// Returns
//	The size of the header.  Will be zero if USE_HEADER_AND_FOOTER isn't 
//	true
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE	CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetHeaderSize()
{
	return USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_HEADER) : sizeof(ALLOCATION_SIZE_HEADER);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFooterSize
//
// Returns
//	The size of the header.  Will be zero if USE_HEADER_AND_FOOTER isn't 
//	true
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE	CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::GetFooterSize()
{
	return USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_FOOTER) : 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SortFreeListAfterGetBlock
//
// Parameters
//	sortBucket : A pointer to a bucket in the free list which needs to get
//		resorted
//	sortBackCount : Specifies whether to traverse backward in contiguous buckets
//		when resorting.  The number specified the count of contiguous buckets
//		to traverse that were touched during a GetBlock or PutBlock operation.
//	frontBlockChangedBucket : Indicates whether or not to continue traversing
//		back from the bucket traversal chain.  This is set when the front
//		block of the sort bucket chain has changed.
//	oldFrontBlockSize : The size of the front block before it was changed.
//		Is valid when frontBlockChangedBucket is not NULL.
//
// Remarks
//	It could be that additional buckets need to be resorted beyond the
//	sortBack count up to the max allocation size.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::SortFreeListAfterGetBlock(CMemoryHeapBucket<BLOCK_COUNT>* sortBucket, unsigned int sortBackCount, CMemoryHeapBucket<BLOCK_COUNT>* frontBlockChangedBucket, MEMORY_SIZE oldFrontBlockSize)
{	
		
	MEMORY_SIZE nextBucketFrontAllocationSize = 0;
	bool firstGetMaxAllocationSize = true;
	
	// Re-sort any buckets that had blocks added/removed from them during the GetBlock/PutBlock
	//
	while(sortBucket)
	{
		nextBucketFrontAllocationSize = SortBucket(sortBucket, firstGetMaxAllocationSize ? NULL : &nextBucketFrontAllocationSize);
		firstGetMaxAllocationSize = false;
		
		sortBucket = sortBucket->Prev;	
						
		if(sortBackCount)
		{
			--sortBackCount;
		}
		else
		{
			break;
		}
	} 	
	
	if(frontBlockChangedBucket)
	{
		if(frontBlockChangedBucket->m_FrontFreeBlockIndex == cHeapBlockInvalid)
		{
			nextBucketFrontAllocationSize = 0;
		}
		// This can happen if there is no sortBucket parameter to the method
		//
		else if(firstGetMaxAllocationSize)
		{		
			// Need to figure out what the front block size is 
			//
			if(frontBlockChangedBucket->m_BackFreeBlockIndex == frontBlockChangedBucket->m_FrontFreeBlockIndex)
			{
				nextBucketFrontAllocationSize = frontBlockChangedBucket->GetMaxAllocationSize(NULL);
			}
			else
			{
				nextBucketFrontAllocationSize = frontBlockChangedBucket->m_FreeBlocks[frontBlockChangedBucket->m_FrontFreeBlockIndex]->InternalSize;
			}		
		}
		
		sortBucket = frontBlockChangedBucket->Prev;		
		
		// Only need to continue back traversal up to the max internal allocation size
		//
		MEMORY_SIZE backTraversalSize = nextBucketFrontAllocationSize;
		
		while(sortBucket)
		{
			// The sort bucket's max allocation size wouldn't be affected if it does not have
			// a back free block
			//
			if(sortBucket->m_BackFreeBlockIndex == cHeapBlockInvalid)
			{
				break;
			}	
			
			// If the span block isn't greater than the largest internal block size,
			// no need to update this bucket
			//
			if(sortBucket->m_FreeBlocks[sortBucket->m_BackFreeBlockIndex]->InternalSize + oldFrontBlockSize <= sortBucket->m_FreeBlocks[sortBucket->m_FreeBlocks.GetCount() - 1]->InternalSize)
			{
				break;
			}					
			oldFrontBlockSize += sortBucket->m_FreeBlocks[sortBucket->m_BackFreeBlockIndex]->InternalSize;		
			
			// Keep track of how far back we need to go
			//
			backTraversalSize += sortBucket->m_FreeBlocks[sortBucket->m_BackFreeBlockIndex]->InternalSize;			
			
			if(backTraversalSize >= m_MaxInternalAllocationSize)
			{
				break;
			}
			
			nextBucketFrontAllocationSize = SortBucket(sortBucket, &nextBucketFrontAllocationSize);
			
			// No need to continue traversing if there are more than one free block
			//
			if(sortBucket->m_FreeBlocks.GetCount() > 1)
			{
				break;
			}
			
			// No need to continue traversing back if the previous bucket won't be affected
			//
			if(sortBucket->m_FrontFreeBlockIndex == cHeapBlockInvalid)
			{
				break;
			}
			
			sortBucket = sortBucket->Prev;						
		}
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SortFreeListAfterPutBlock
//
// Parameters
//	sortBucket : A pointer to a bucket in the free list which needs to get
//		resorted
//	sortBackCount : Specifies whether to traverse backward in contiguous buckets
//		when resorting.  The number specified the count of contiguous buckets
//		to traverse that were touched during a GetBlock or PutBlock operation.
//	frontBlockChangedBucket : Indicates whether or not to continue traversing
//		back from the bucket traversal chain.  This is set when the front
//		block of the sort bucket chain has changed.
//	oldFrontBlockSize : The size of the front block before it was changed.
//		Is valid when frontBlockChangedBucket is not NULL.
//
// Remarks
//	It could be that additional buckets need to be resorted beyond the
//	sortBack count up to the max allocation size.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::SortFreeListAfterPutBlock(CMemoryHeapBucket<BLOCK_COUNT>* sortBucket, unsigned int sortBackCount, CMemoryHeapBucket<BLOCK_COUNT>* frontBlockChangedBucket, MEMORY_SIZE oldFrontBlockSize)
{	
		
	MEMORY_SIZE nextBucketFrontAllocationSize = 0;
	bool firstGetMaxAllocationSize = true;
	
	// Re-sort any buckets that had blocks added/removed from them during the GetBlock/PutBlock
	//
	while(sortBucket)
	{
		nextBucketFrontAllocationSize = SortBucket(sortBucket, firstGetMaxAllocationSize ? NULL : &nextBucketFrontAllocationSize);
		firstGetMaxAllocationSize = false;
		
		sortBucket = sortBucket->Prev;	
						
		if(sortBackCount)
		{
			--sortBackCount;
		}
		else
		{
			break;
		}
	} 	
	
	if(frontBlockChangedBucket)
	{
		if(sortBucket == NULL)
		{
			if(frontBlockChangedBucket->m_FrontFreeBlockIndex != cHeapBlockInvalid)
			{
				if(frontBlockChangedBucket->m_BackFreeBlockIndex == frontBlockChangedBucket->m_FrontFreeBlockIndex)
				{
					nextBucketFrontAllocationSize = frontBlockChangedBucket->GetMaxAllocationSize(NULL);
				}
				else
				{
					nextBucketFrontAllocationSize = frontBlockChangedBucket->m_FreeBlocks[frontBlockChangedBucket->m_FrontFreeBlockIndex]->InternalSize;
				}
			}
			else
			{
				nextBucketFrontAllocationSize = 0;
			}
		}
		
		sortBucket = frontBlockChangedBucket->Prev;
		
		
		// Only need to continue back traversal up to the max internal allocation size
		//
		MEMORY_SIZE backTraversalSize = oldFrontBlockSize;
		
		while(sortBucket)
		{
			// The sort bucket's max allocation size wouldn't be affected if it does not have
			// a back free block
			//
			if(sortBucket->m_BackFreeBlockIndex == cHeapBlockInvalid)
			{
				break;
			}	
			
			// If the span block isn't greater than the largest internal block size,
			// no need to update this bucket
			//
			if(sortBucket->m_FreeBlocks[sortBucket->m_BackFreeBlockIndex]->InternalSize + nextBucketFrontAllocationSize <= sortBucket->m_FreeBlocks[sortBucket->m_FreeBlocks.GetCount() - 1]->InternalSize)
			{
				break;
			}
					
			
			// Keep track of how far back we need to go
			//
			backTraversalSize += sortBucket->m_FreeBlocks[sortBucket->m_BackFreeBlockIndex]->InternalSize;			
			
			if(backTraversalSize >= m_MaxInternalAllocationSize)
			{
				break;
			}
			
			nextBucketFrontAllocationSize = SortBucket(sortBucket, &nextBucketFrontAllocationSize);
			
			// No need to continue traversing if there are more than one free block
			//
			if(sortBucket->m_FreeBlocks.GetCount() > 1)
			{
				break;
			}
			
			// No need to continue traversing back if the previous bucket won't be affected
			//
			if(sortBucket->m_FrontFreeBlockIndex == cHeapBlockInvalid)
			{
				break;
			}
			
			sortBucket = sortBucket->Prev;						
		}
	}	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SortBucket
//
// Parameters
//	bucket : The bucket that will be re-sorted in the free list
//	nextBucketFrontAllocationSize : [in/out] Contains the size of the next
//		bucket's front allocation.  On return, contains the size of the
//		supplied bucket's front allocation.
//
// Returns
//	The size of the bucket's front allocation
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
MEMORY_SIZE CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::SortBucket(CMemoryHeapBucket<BLOCK_COUNT>* bucket, MEMORY_SIZE* nextBucketFrontAllocationSize)
{
	// Get the max allocation size for the sort bucket
	//
	MEMORY_SIZE maxAllocationSize = bucket->GetMaxAllocationSize(nextBucketFrontAllocationSize);
	
	// Re-sort the bucket in the free list
	//
	unsigned int bucketIndex = cHeapBlockInvalid;
	MEMORY_SIZE* bucketKey = NULL;
	m_FreeList.Find(bucket, &bucketIndex, &bucketKey);
	DBG_ASSERT(bucketIndex < m_FreeList.GetCount());
	DBG_ASSERT(*bucketKey != maxAllocationSize);			
	m_FreeList.Sort(bucketIndex, maxAllocationSize);
									
	// The front free block spans the entire bucket and therefore spills into
	// the adjacent bucket.  The size is equal to the max allocation size
	//
	if(bucket->m_FrontFreeBlockIndex != cHeapBlockInvalid && bucket->m_BackFreeBlockIndex != cHeapBlockInvalid &&
		bucket->m_FrontFreeBlockIndex == bucket->m_BackFreeBlockIndex)
	{
		return maxAllocationSize;
	}
	// The front free block ends within the bucket
	//
	else if(bucket->m_FrontFreeBlockIndex != cHeapBlockInvalid)
	{
		return bucket->m_FreeBlocks[bucket->m_FrontFreeBlockIndex]->InternalSize;	
	}
	else
	{
		return 0;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	VerifyFreeList
//
// Remarks
//	This method goes through the free list and makes sure that:
//		- the max allocation keys are in order
//		- each bucket's front block index is valid and correct
//		- each bucket's back block index is valid and correct
//		- the bucket's entry in the free list matches the count of it's free
//			block list
//		- if the bucket's max allocation size is greater that that
//			reported by it's internal free block list, there is a next
//			bucket with an appropriate free front block
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE, bool USE_HEADER_AND_FOOTER, unsigned int INITIAL_BUCKET_COUNT, unsigned int BUCKET_SIZE, MEMORY_SIZE BLOCK_COUNT>
void CMemoryAllocatorHEAP<MT_SAFE, USE_HEADER_AND_FOOTER, INITIAL_BUCKET_COUNT, BUCKET_SIZE, BLOCK_COUNT>::VerifyFreeList()
{	

	MEMORY_SIZE lastMaxBucketAllocation = 0;
	for(unsigned int i = 0; i < m_FreeList.GetCount(); ++i)
	{			
		CMemoryHeapBucket<BLOCK_COUNT>* bucket = *m_FreeList[i];
		MEMORY_SIZE maxBucketAllocation = *m_FreeList.GetKey(i);
		DBG_ASSERT(maxBucketAllocation <= m_MaxInternalAllocationSize);
		DBG_ASSERT(maxBucketAllocation == bucket->GetMaxAllocationSize(NULL));
		
		DBG_ASSERT(bucket->m_FreeBlocks.GetCount() + bucket->m_UsedBlocks.GetCount() <= BLOCK_COUNT);
				
		// Verify that the free list keys are in order by increasing max allocation size
		//
		DBG_ASSERT(maxBucketAllocation >= lastMaxBucketAllocation);
		
		lastMaxBucketAllocation = maxBucketAllocation;
		
		// Verify the correctness of the front block index
		//
		if(bucket->m_FrontFreeBlockIndex < bucket->m_FreeBlocks.GetCount())
		{
			DBG_ASSERT(bucket->m_FreeBlocks[bucket->m_FrontFreeBlockIndex]->Memory == bucket->m_Memory);
		}
		
		// Verify the correctness of the back block index
		//
		if(bucket->m_BackFreeBlockIndex < bucket->m_FreeBlocks.GetCount())
		{
			DBG_ASSERT(bucket->m_FreeBlocks[bucket->m_BackFreeBlockIndex]->Memory + bucket->m_FreeBlocks[bucket->m_BackFreeBlockIndex]->InternalSize == bucket->m_Memory + bucket->m_MemorySize);
		}
		
		// If the bucket has a non-zero max allocation size in the free list, it
		// better have a free block
		//
		DBG_ASSERT(maxBucketAllocation == 0 || bucket->m_FreeBlocks.GetCount());
		
		// Make sure that if the bucket's max allocation size in the free list is 
		// greater than the bucket's max sized free block, that there is a next bucket
		// and that the next bucket has a front block index and that the current bucket
		// has a back block index
		//		
		if(maxBucketAllocation && maxBucketAllocation > bucket->m_FreeBlocks[bucket->m_FreeBlocks.GetCount() - 1]->InternalSize)
		{
			// There better be a valid back free block index
			//
			DBG_ASSERT(bucket->m_BackFreeBlockIndex != cHeapBlockInvalid);
			
			// There better be a valid next bucket and it better have a valid front free block index
			//
			DBG_ASSERT(bucket->Next);
			DBG_ASSERT(bucket->Next->m_FrontFreeBlockIndex != cHeapBlockInvalid);
			
			// Validate the cross bucket allocation
			//		
			maxBucketAllocation -= bucket->m_FreeBlocks[bucket->m_BackFreeBlockIndex]->InternalSize;
				
			bucket = bucket->Next;
			while(maxBucketAllocation && bucket)
			{
				DBG_ASSERT(bucket->m_FrontFreeBlockIndex != cHeapBlockInvalid);
				
				if(maxBucketAllocation > bucket->m_MemorySize)
				{
					DBG_ASSERT(bucket->m_BackFreeBlockIndex != cHeapBlockInvalid);
				}
				else
				{
					DBG_ASSERT(maxBucketAllocation <= bucket->m_FreeBlocks[bucket->m_FrontFreeBlockIndex]->InternalSize);
				}
				
				maxBucketAllocation -= MIN<MEMORY_SIZE>(maxBucketAllocation, bucket->m_FreeBlocks[bucket->m_FrontFreeBlockIndex]->InternalSize);
			
				bucket = bucket->Next;
			}
			
			DBG_ASSERT(maxBucketAllocation == 0);									
		}					
		else if(maxBucketAllocation)
		{
			DBG_ASSERT(maxBucketAllocation == bucket->m_FreeBlocks[bucket->m_FreeBlocks.GetCount() - 1]->InternalSize);
		}
		else
		{
			DBG_ASSERT(bucket->m_FreeBlocks.GetCount() == 0);
		}
	}

}

} // end namespace FSCommon

#pragma warning(pop)