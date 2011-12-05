//----------------------------------------------------------------------------
// memoryallocator_pool.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator.h"
#include "memorypoolbin.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memoryallocator_pool.h.tmh"
#endif

#pragma warning(push)
#pragma warning(disable:4127)


namespace FSCommon
{

// ---------------------------------------------------------------------------
// init takes an array of these with which it generates bin definitions
// ---------------------------------------------------------------------------
struct MEMORY_BIN_DEF
{
	// The size in bytes for buckets blocks created in the bin
	//
	MEMORY_SIZE						size; // up to this size, try stepping using the given granularity

	// When multiple bins are created from a single def, the granularity is used
	// to increment the "size" for each successive bin
	//
	MEMORY_SIZE						granularity;

	// For each bin created in response to the def, the bucketsize is the size of the 
	// bucket's internal memory allocation
	//
	MEMORY_SIZE						bucketsize;	// size of amalgamated allocation
};

// ---------------------------------------------------------------------------
// Bins get created from this table in the following way:  For each line, the bin creation process
// starts by creating a bin from the line-specified values.  This means that the first bin will contain
// blocks of "size" equal to 8 bytes and the bucket memory allocation will be for 4096 bytes.
// The first line will create more than one bucket however.  It will create another bin with 
// block sizes "gran" number of bytes bigger than the last bin.  So the next bin will be created
// with a 16 byte block size, still using 4096 bytes for the bucket memory allocation size.  All
// bins created in response to the first line will use the 4096 bucket memory allocation size.
// The third bin will be created with 24 byte bucket block size (last bucket size + gran = 24).
// Since the next increment would take the bucket block size to 32, the next line in the array,
// further buckets will be created using the next line (32, 8, 16384).  If, during the processing 
// of a potential bin, the number of blocks available in the bucket given the block size is equal
// to the next bin of a slightly greater block size, the current bin will not be generated since
// it would just waste more space in overhead.
// ---------------------------------------------------------------------------
static const MEMORY_BIN_DEF cDefaultBinDef[] =
{ // size,	gran,  bucketsize
		8, 	   8,    8192,
	   32, 	   8,   32768,
	   64, 	  16,   65536,
	  128, 	  32,   65536,
	  256, 	  64,   65536,
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
//	CMemoryAllocatorPOOL
//
// Parameters
//	INITIAL_BUCKET_COUNT : The number of buckets allocated as member data for 
//		the pool.  The bucket count can increase beyond this value, but it
//		is best to set it accordingly.
//
// Remarks
//	The CMemoryAllocatorPOOL is a list of pools.  The class contains a list of bins.  Each
//	bin corresponds to a particular block size for which allocations up to the block size will
//	be taken from.  Each bin contains a list of buckets.  Each bucket wraps a chunk of memory
//	that is divided into blocks.  All buckets within the bin's bucket list have the same block
//	size.  New buckets are allocated for the bucket list when all free blocks within the bin's 
//	bucket list are currently used.
//
/////////////////////////////////////////////////////////////////////////////	
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
class CMemoryAllocatorPOOL : public CMemoryAllocator
{	
	protected:
	
		static const unsigned int cMaxBinCount = 42;


		// TRACKER_NODE
		//
		template<bool MT_SAFE>
		struct TRACKER_NODE
		{
			CMemoryPoolBucket<MT_SAFE>*		Bucket;
			CMemoryPoolBin<MT_SAFE>*		Bin;
		};

		typedef CSortedArray<MEMORY_SIZE, CMemoryPoolBin<MT_SAFE>, MT_SAFE> BIN_COLLECTION;
		typedef CSortedArray<void*, TRACKER_NODE<MT_SAFE>, MT_SAFE> BUCKET_TRACKER;
		typedef CPool<CMemoryPoolBucket<MT_SAFE>, MT_SAFE> BUCKET_POOL;

		// ALLOCATION_HEADER
		//
		#pragma pack(push, 1)		
		struct ALLOCATION_HEADER : public CMemoryAllocator::ALLOCATION_HEADER
		{
			MEMORY_SIZE							InternalAllocationSize;
			DWORD								MagicHeader;
		};
		#pragma pack(pop)	

	public:
	
		CMemoryAllocatorPOOL(const WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);
		virtual ~CMemoryAllocatorPOOL();
		
	public: // IMemoryAllocator
	
		virtual bool						Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, bool allowTrim, bool optimizeForTrim, const MEMORY_BIN_DEF* binDefinitions = NULL, unsigned int binDefinitionCount = 0);
		virtual bool						Term();
		virtual bool						Flush();
		
		// Alloc
		//
		virtual void*						AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
		// Free
		//
		virtual void						Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator));
		
		// Size
		//
		virtual MEMORY_SIZE					Size(void* ptr, IMemoryAllocator* fallbackAllocator);
		
	public: // IMemoryAllocatorInternals
	
		virtual void*						GetInternalPointer(void* ptr);
		virtual MEMORY_SIZE					GetInternalSize(void* ptr);		
		virtual void						GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics);				

	protected:
	
		virtual void						MemTraceInternalsEx();
	
	protected:
	
		MEMORY_SIZE							GetOverheadAdjustedSize(MEMORY_SIZE size, unsigned int alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize);
		virtual bool						InitBins(const MEMORY_BIN_DEF* bindef, unsigned int count, MEMORY_SIZE maxAllocationSize, bool optimizeForTrim);		
		void								RemoveBucket(CMemoryPoolBucket<MT_SAFE>* bucket, CMemoryPoolBin<MT_SAFE>* bin);
		
	protected:
	
		MEMORY_SIZE							m_MaxAllocationSize;
		bool								m_AllowTrim;
		bool								m_OptimizeForTrim;
	
		
		BIN_COLLECTION						m_Bins;
		BYTE								m_BinMemory[cMaxBinCount * sizeof(BIN_COLLECTION::ARRAY_NODE)];
		
		BUCKET_TRACKER						m_BucketTracker;
		BYTE								m_BucketTrackerMemory[INITIAL_BUCKET_COUNT * sizeof(BUCKET_TRACKER::ARRAY_NODE)];
		
		BUCKET_POOL							m_BucketPool;
		BYTE								m_BucketPoolMemory[INITIAL_BUCKET_COUNT * sizeof(CMemoryPoolBucket<MT_SAFE>)];
		
		MEMORY_SIZE							m_LargestFreeBlock;
	
};
	
/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorPOOL Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::CMemoryAllocatorPOOL(const WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocator(MEMORY_ALLOCATOR_POOL, name, defaultAlignment),
	m_MaxAllocationSize(0),
	m_LargestFreeBlock(0),
	m_OptimizeForTrim(false),
	m_AllowTrim(true),
	m_Bins(IMemoryManager::GetInstance().GetSystemAllocator()),
	m_BucketTracker(IMemoryManager::GetInstance().GetSystemAllocator()),
	m_BucketPool(IMemoryManager::GetInstance().GetSystemAllocator())
{
	m_Bins.Init(&m_BinMemory, cMaxBinCount, 0);
	m_BucketTracker.Init(&m_BucketTrackerMemory, INITIAL_BUCKET_COUNT, INITIAL_BUCKET_COUNT);	
	ASSERT(m_BucketPool.Init(&m_BucketPoolMemory, sizeof(m_BucketPoolMemory)) == INITIAL_BUCKET_COUNT);
	
	m_InternalOverhead += sizeof(CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorPOOL Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::~CMemoryAllocatorPOOL()
{
	Term();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Parameters
//	fallbackAllocator : A pointer to the allocator that will be used if 
//		the current allocator cannot fulfill an allocation request.  Can
//		be NULL.
//	maxAllocationSize : Determines the max allocation size that will
//		be serviced by the allocator before it is passed on to the fall
//		back allocator.  Specify 0 for no max allocation size.
//	allowTrim : Specifies whether empty bucket are released to the
//		underlying allocator automatically.  If this is false, it is
//		still possible to call flush manually
//  optimizeForTrim : If true, will sort the free bucket list by increasing number of
//		free blocks and take new allocation requests from the bucket with the
//		least number of free blocks.  This is increase the chance that 
//		buckets will be released to the underlying allocator, however it
//		reduces cache coherency and performance.
//	binDefinitions : [optional] A pointer to the array of bin definitions that will
//		override the default bin definition array
//	binDefintionCount : [optional] If binDefinitions is specified, determines
//		the number of items in the array
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
bool CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, bool allowTrim, bool optimizeForTrim, const MEMORY_BIN_DEF* binDefinitions, unsigned int binDefinitionCount)
{
	bool retValue = true;

	if(m_Initialized == false)
	{
		__super::Init(fallbackAllocator);

		m_MaxAllocationSize = maxAllocationSize;
		m_AllowTrim = allowTrim;
		m_OptimizeForTrim = optimizeForTrim;

		// Use the default bin definitions if not specified
		//
		if(binDefinitions == NULL)
		{
			binDefinitions = cDefaultBinDef;
			binDefinitionCount = _countof(cDefaultBinDef);
		}
		
		if(InitBins(binDefinitions, binDefinitionCount, maxAllocationSize, m_OptimizeForTrim) == false)
		{
			retValue = false;
		}
					
		if(retValue)
		{
			m_Initialized = true;
		}
	}
	
	return retValue;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
bool CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::Term()
{
	if(m_Initialized)
	{	
		__super::Term();
		
		for(unsigned int i = 0; i < m_BucketTracker.GetCount(); ++i)
		{
			CMemoryPoolBucket<MT_SAFE>* bucket = m_BucketTracker[i]->Bucket;
			CMemoryPoolBin<MT_SAFE>* bin = m_BucketTracker[i]->Bin;
			
			InternalFree(MEMORY_FUNC_FILELINE(bucket->m_Memory, bin->m_BucketSize));
		}		
		
		for(unsigned int i = 0; i < m_Bins.GetCount(); ++i)
		{
			m_Bins[i]->Term();
		}
	}
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Flush
//
// Returns
//	true if after the flush operation the allocator is completely empty, false
//	otherwise
//
// Remarks
//	Called when the allocator should cleanup any unused memory
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
bool CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::Flush()
{
	for(unsigned int i = 0; i < m_Bins.GetCount(); ++i)
	{
		CMemoryPoolBin<MT_SAFE>* bin = m_Bins[i];
		
		bin->Lock();
		
		CMemoryPoolBucket<MT_SAFE>* bucket = bin->m_Head;
		while(bucket != NULL)
		{
			CMemoryPoolBucket<MT_SAFE>* nextBucket = bucket->Next;
		
			if(bucket->m_UsedCount == 0)
			{
				RemoveBucket(bucket, bin);					
			}
		
			bucket = nextBucket;
		}		
		
		bin->UnLock();
	}
	
	if(m_TotalExternalAllocationCount == 0)
	{
		return true;
	}
	else
	{
		return false;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void* CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	if(size == 0 || m_Initialized == false)
	{
		return NULL;
	}

	// Account for additional debug overhead in the allocation
	//
	MEMORY_SIZE alignmentOverheadSize = 0;
	MEMORY_SIZE headerSize = 0;
	MEMORY_SIZE footerSize = 0;
	MEMORY_SIZE internalAllocationSize = GetOverheadAdjustedSize(size, alignment, alignmentOverheadSize, headerSize, footerSize);
	DBG_ASSERT(internalAllocationSize >= size);

	// Grab the bin corresponding to the internal allocation request size
	//
	CMemoryPoolBin<MT_SAFE>* bin = m_Bins.FindOrAfter(internalAllocationSize);
	DBG_ASSERT(bin == NULL || bin->m_BlockSize >= internalAllocationSize);

	if(bin)
	{
		bin->Lock();
		
		CMemoryPoolBucket<MT_SAFE>* bucket = bin->m_FirstNonEmptyBucket;
		
		// Create a new bucket if necessary
		//
		if(bucket == NULL)
		{
			// Grab a new bucket from the pool
			//
			bucket = m_BucketPool.Get();
			if(bucket == NULL)
			{
				ASSERT(bucket);
				bin->UnLock();
				return NULL;
			}
			//ASSERTV(m_BucketPool.GetUsedCount() != INITIAL_BUCKET_COUNT + 1, "Bucket count too small in pool allocator: %S. - Didier", m_Name);						
			
			// Create a node to track the bucket
			//
			TRACKER_NODE<MT_SAFE> trackerNode;
			trackerNode.Bin = bin;
			trackerNode.Bucket = bucket;
			
			// Allocate memory for the new bucket
			//
			BYTE* bucketMemory = reinterpret_cast<BYTE*>(InternalAlloc(MEMORY_FUNC_FILELINE(bin->m_BucketSize)));			
			
			// Initialize the bucket and add it to the tracker
			//
			if(bucket->Init(bucketMemory, bin->m_BucketSize, bin->m_BlockSize, bin->m_BlockCountMax, bin->m_SizeBits) == false || m_BucketTracker.Insert(bucketMemory, trackerNode) == false)
			{
				InternalFree(MEMORY_FUNC_FILELINE(bucketMemory, bin->m_BucketSize));
				bucket->Term();
				m_BucketPool.Put(bucket);
				bin->UnLock();
				
				return NULL;
			}
			
			// Add the new bucket to bin's bucket list
			//
			bin->AddBucket(bucket, !m_OptimizeForTrim);	
			
			// Update Metrics
			//
			AtomicAdd(m_InternalFragmentation, bin->m_BucketSize);
			AtomicAdd(m_InternalOverhead, bin->m_BucketSize - (bin->m_BlockSize * bin->m_BlockCountMax)); // Overhead due to per-bucket size array
		}	
		
		BYTE* internalAllocationPtr = NULL;
		
		if(bucket)
		{
			internalAllocationPtr = bin->GetBlock(bucket, size);
		}					
		
		bin->UnLock();
				
		if(internalAllocationPtr)
		{
			// Adjust the pointer so that it is aligned and far enough ahead for the header
			//
			BYTE* externalAllocationPtr = internalAllocationPtr + headerSize;
			externalAllocationPtr += ((alignment - ((size_t)externalAllocationPtr % alignment)) % alignment);
			
			// Set the header/footer
			//
			if(USE_HEADER_AND_FOOTER)
			{
				ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));
				allocationHeader->Origin = m_AllocationTracker.OnAlloc(MEMORY_FUNC_PASSFL(externalAllocationPtr, size));			
				allocationHeader->InternalAllocationSize = internalAllocationSize;
				allocationHeader->MagicHeader = cMagicHeader;
				
				ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + size);
				allocationFooter->MagicFooter = cMagicFooter;

				// Update metrics
				//
				AtomicAdd(m_InternalOverhead, (internalAllocationSize - size));	
				AtomicAdd(m_InternalFragmentation, -(signed)internalAllocationSize);
			}

			// Update metrics
			//
			AtomicAdd(m_TotalExternalAllocationSize, size);
			m_TotalExternalAllocationSizePeak = MAX<MEM_COUNTER>(m_TotalExternalAllocationSize, m_TotalExternalAllocationSizePeak);			
			AtomicIncrement(m_TotalExternalAllocationCount);

			// Server counters
			//
			if(m_PerfInstance)
			{
				PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, bin->m_BlockSize);
				PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalAllocationRate, 1);
			}		
			
			DBG_ASSERT((size_t)externalAllocationPtr % alignment == 0);			
			return externalAllocationPtr;
		}		
	}
	
	if(fallbackAllocator)
	{
		// Attempt to allocate from the large block allocator
		//
		return fallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}	
	else if(m_FallbackAllocator)
	{
		// Attempt to allocate from the large block allocator
		//
		return m_FallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}
	
	return NULL;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator))
{
	if(ptr == NULL || m_Initialized == false)
	{
		return;
	}

	// Attempt to find the pointer in one of the bins/buckets
	//
	TRACKER_NODE<MT_SAFE> trackerNode;
	if(m_BucketTracker.FindOrBefore(ptr, trackerNode) && trackerNode.Bucket->IsInside(ptr))
	{
		CMemoryPoolBin<MT_SAFE>* bin = trackerNode.Bin;
		CMemoryPoolBucket<MT_SAFE>* bucket = trackerNode.Bucket;	
	
		BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
		MEMORY_SIZE externalAllocationSize = bucket->GetSize(ptr);
		
		// Check for double-frees
		//
		ASSERTV_RETURN(externalAllocationSize, "Double free detected. -Didier");
	
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
			
			ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + externalAllocationSize);

			if(allocationFooter->MagicFooter != cMagicFooter)
			{
				// Throw an exception and let the process terminate
				//
				RaiseException(EXCEPTION_CODE_MEMORY_CORRUPTION, 0, 0, 0);	
				return;	
			}

			m_AllocationTracker.OnFree(MEMORY_FUNC_PASSFL(ptr, externalAllocationSize, allocationHeader->Origin));

			// Update metrics
			//
			AtomicAdd(m_InternalOverhead, -(signed)(allocationHeader->InternalAllocationSize - externalAllocationSize));
			AtomicAdd(m_InternalFragmentation, allocationHeader->InternalAllocationSize);
		}

		// Update Metrics
		//
		AtomicAdd(m_TotalExternalAllocationSize, -(signed)externalAllocationSize);
		AtomicDecrement(m_TotalExternalAllocationCount);
		
		// Put the block back in the bucket
		//
		bin->Lock();
		bin->PutBlock(ptr, bucket);		
		
		// If there are no more allocated blocks in the bucket, release the bucket
		// and remove it from the the tracker
		//
		if(m_AllowTrim && bucket->m_UsedCount == 0)
		{
			RemoveBucket(bucket, bin);		
		}
		
		bin->UnLock();
		
		m_LargestFreeBlock = MAX<MEMORY_SIZE>(m_LargestFreeBlock, bin->m_BlockSize);

		// Server counters
		//
		if(m_PerfInstance)
		{
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, -(signed)bin->m_BlockSize);
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalFreeRate, 1);
		}
		
	}
	// If the pointer wasn't allocated in this allocator, fallback if possible
	//
	else if(fallbackAllocator)
	{
		return fallbackAllocator->Free(MEMORY_FUNC_PASSFL(ptr, NULL));
	}	
	else if(m_FallbackAllocator)
	{
		return m_FallbackAllocator->Free(MEMORY_FUNC_PASSFL(ptr, NULL));
	}
	else
	{
		// The memory is going to be leaked
		//
		DBG_ASSERT(0);
	}	
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
MEMORY_SIZE CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::Size(void* ptr, IMemoryAllocator* fallbackAllocator)
{
	if (NULL == ptr)
	{
		return 0;
	}

	TRACKER_NODE<MT_SAFE> trackerNode;	
	if(m_BucketTracker.FindOrBefore(ptr, trackerNode) && trackerNode.Bucket->IsInside(ptr))
	{
		return trackerNode.Bucket->GetSize(ptr);
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void* CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::GetInternalPointer(void* ptr)
{
	if(ptr == NULL)
	{
		return NULL;
	}
	
	// Check to see if the pointer is within the pool's memory
	//
	TRACKER_NODE<MT_SAFE> trackerNode;
	if(m_BucketTracker.FindOrBefore(ptr, trackerNode) == false)
	{
		if(m_FallbackAllocator)
		{
			return m_FallbackAllocator->GetInternals()->GetInternalPointer(ptr);
		}
		else
		{
			return NULL;
		}
	}
	
	size_t externalAllocationPtr = reinterpret_cast<size_t>(ptr);
		
	return reinterpret_cast<void*>(externalAllocationPtr - (externalAllocationPtr % trackerNode.Bin->m_BlockSize));
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
//	pointer is not within the allocator.
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
MEMORY_SIZE CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::GetInternalSize(void* ptr)
{
	if(ptr == NULL)
	{
		ASSERT(0);
		return 0;		
	}

	TRACKER_NODE<MT_SAFE> trackerNode;
	
	// Check to see if the pointer is within the pool's memory
	//
	if(m_BucketTracker.FindOrBefore(ptr, trackerNode) == false)
	{
		if(m_FallbackAllocator)
		{
			return m_FallbackAllocator->GetInternals()->GetInternalSize(ptr);
		}
		else
		{
			return 0;
		}
	}

	return trackerNode.Bin->m_BlockSize;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics)

{
	__super::GetMetrics(metrics);
	
	metrics.LargestFreeBlock = m_LargestFreeBlock;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternalsEx
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::MemTraceInternalsEx()
{
	MemPrint("\t\t<internalsEx MaxAllocationSize='%u'>\n", m_MaxAllocationSize);

	for(unsigned int i = 0; i < m_Bins.GetCount(); ++i)
	{
		CMemoryPoolBin<MT_SAFE>* bin = m_Bins[i];
		
		MEMORY_SIZE blockSize = bin->m_BlockSize;
		unsigned int freeBlockCount = bin->m_FreeBlocks;
		unsigned int usedBlockCount = bin->m_UsedBlocks;
		unsigned int bucketCount = bin->m_BucketCount;
		unsigned int freeBytes = freeBlockCount * blockSize;
		unsigned int bucketCountPeak = bin->m_BucketCountPeak;
		
		MemPrint("\t\t\t<bin BlockSize='%u' BucketCount='%u' BucketCountPeak='%u' FreeBlockCount='%u' FreeBytes='%u' UsedBlockCount='%u'/>\n", blockSize, bucketCount, bucketCountPeak, freeBlockCount, freeBytes, usedBlockCount);
	}

	MemPrint("\t\t</internalsEx>\n");
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetOverheadAdjustedSize
//
// Parameters
//	size : The size in bytes of the external allocation
//	alignment : The alignment for the external allocation request
//	alignmentOverheadSize : [out] A reference to a variable that will be set to the size in
//		bytes required before the header for alignment
//	headerSize : [out] A reference to a variable that will be set to the header
//		size
//	footerSize : [out] A reference to a variable that will be set to the footer
//		size
//
// Returns
//	The size required for an allocation of the requested size, including
//	overhead for header and footer as well as alignment requirements
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
MEMORY_SIZE	CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::GetOverheadAdjustedSize(MEMORY_SIZE size, 
	unsigned int alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize)
{
	headerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_HEADER) : 0;
	footerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_FOOTER) : 0;

	// Figure out the maximum number of bytes extra requires for alignment and header
	//
	MEMORY_SIZE extraBytesNeededForAlignmentAndHeader = (((headerSize / alignment) + 1) * alignment) + ((headerSize % alignment) - 1);				
				
	alignmentOverheadSize = extraBytesNeededForAlignmentAndHeader - headerSize;
		
	return size + extraBytesNeededForAlignmentAndHeader + footerSize;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InitBins
//
// Parameters
//	bindef : A pointer to the MEMORY_BIN_DEFs that define how the bins should be created
//	count : The number of items in the bindef list
//	maxAllocationSize : The max allocation size that will be used when
//		creating bins.  No bins will be created with a size over this amount.
//		Specify 0 to use the entire bindef.
//  optimizeForTrim : If true, will sort the free bucket list by increasing number of
//		free blocks and take new allocation requests from the bucket with the
//		least number of free blocks.  This is increase the chance that 
//		buckets will be released to the underlying allocator, however it
//		reduces cache coherency and performance.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise
//
// Remarks
//	This function generates the bins in the array from the bin definitions.
//
/////////////////////////////////////////////////////////////////////////////				
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
bool CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::InitBins(const MEMORY_BIN_DEF* bindef, unsigned int count, MEMORY_SIZE maxAllocationSize, bool optimizeForTrim)
{	
	ASSERT_RETFALSE(bindef);
	ASSERT_RETFALSE(count);

	unsigned int currentDefinitionIndex = 0;

	MEMORY_SIZE lastBucketBlockSize = 0;

	// Keep creating bins until a bin is created with a block size equal to the last definition
	//
	while(lastBucketBlockSize < bindef[count - 1].size)
	{
		// If there is a max allocation size and the last bucket block size reached the max
		// allocation size, stop creating new bins
		//
		if(maxAllocationSize != 0 && lastBucketBlockSize >= maxAllocationSize)
		{
			break;
		}

		// Stop making bins for the current definition line when the next potential bin's
		// bucket block size has reached the size defined by the next definition line
		//
		while(currentDefinitionIndex < count - 1)
		{
			// Confirm that successive bin definitions have increasing block sizes
			//
			ASSERT_RETFALSE(currentDefinitionIndex == 0 || bindef[currentDefinitionIndex].size > bindef[currentDefinitionIndex-1].size);

			// If the next bin to be created for the current definition line is the same block size
			// as the next definition line...
			//
			if(lastBucketBlockSize + bindef[currentDefinitionIndex].granularity >= bindef[currentDefinitionIndex + 1].size)
			{
				// Skip to the next definition
				//
				++currentDefinitionIndex;				
				lastBucketBlockSize = bindef[currentDefinitionIndex].size - bindef[currentDefinitionIndex].granularity;
				
				continue;
			}

			// Otherwise continue creating a bin for the current definition
			//
			break;
		}

		// Create a new bin
		//
		CMemoryPoolBin<MT_SAFE> bin;
		bin.Init(optimizeForTrim);

		// Calculate the size of the bin given the size of the previous bin, 
		// the current definition line's granularity and bucketsize (memory available per bucket)
		//
		bin.CalcInfo(lastBucketBlockSize, bindef[currentDefinitionIndex].granularity, bindef[currentDefinitionIndex].bucketsize, maxAllocationSize);

		MEMORY_SIZE nextDefinitionBlockSize = 0;
		if(currentDefinitionIndex + 1 < count)
		{
			nextDefinitionBlockSize = bindef[currentDefinitionIndex + 1].size;
		}
		// If the bin's block size has been modified due to the better fit with a larger block size, don't 
		// make the bin if it conflicts with the next definition block size
		//
		if(nextDefinitionBlockSize != 0 && bin.m_BlockSize >= nextDefinitionBlockSize)
		{
			// Skip to the next definition
			//
			++currentDefinitionIndex;				
			lastBucketBlockSize = bindef[currentDefinitionIndex].size - bindef[currentDefinitionIndex].granularity;		
			
			continue;		
		}

		// Add the bin to the sorted array
		//
		ASSERT(m_Bins.Insert(lastBucketBlockSize, bin));
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetMetrics
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int INITIAL_BUCKET_COUNT>
void CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, INITIAL_BUCKET_COUNT>::RemoveBucket(CMemoryPoolBucket<MT_SAFE>* bucket, CMemoryPoolBin<MT_SAFE>* bin)
{
	// Remove the bucket from the pool's bucket tracker
	//
	void* key = bucket->m_Memory;
	m_BucketTracker.Remove(key);
	
	// Remove the bucket from the bin's bucket list
	//
	bin->RemoveBucket(bucket);
	
	// Terminate the bucket and put it back in the pool
	//
	InternalFree(MEMORY_FUNC_FILELINE(bucket->m_Memory, bin->m_BucketSize));
	bucket->Term();
	m_BucketPool.Put(bucket);
	
	// Update metrics
	//
	AtomicAdd(m_InternalFragmentation, -(signed)bin->m_BucketSize);
	AtomicAdd(m_InternalOverhead, -(signed)(bin->m_BucketSize - (bin->m_BlockSize * bin->m_BlockCountMax)));						
}

} // end namespace FSCommon

#pragma warning(pop)