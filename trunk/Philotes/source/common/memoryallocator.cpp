//----------------------------------------------------------------------------
// memoryallocator.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "memoryallocator.h"
#include "memorymanager_i.h"

#if ISVERSION(SERVER_VERSION)
#include "memoryallocator.cpp.tmh"
#endif

#pragma intrinsic(memcpy,memset) // Specifies that these methods should be "inlined"

namespace FSCommon
{

PERF_INSTANCE(CMemoryAllocator)* CMemoryAllocator::g_PerfInstance = NULL;

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocator Constructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocator::CMemoryAllocator(MEMORY_ALLOCATOR_TYPE allocatorType, 
	const WCHAR* name, 
	unsigned int defaultAlignment) :
	m_AllocatorType(allocatorType),
	m_FallbackAllocator(NULL),
	m_DefaultAlignment(defaultAlignment),
	m_InternalSize(0),
	m_InternalFragmentation(0),
	m_InternalOverhead(0),
	m_TotalExternalAllocationSize(0),
	m_TotalExternalAllocationSizePeak(0),
	m_TotalExternalAllocationCount(0),
	m_Initialized(false)
{
	if(name)
	{
		wcsncpy_s(m_Name, arrsize(m_Name), name, _TRUNCATE);
	}
	else
	{
		m_Name[0] = NULL;
	}
	
	m_CS.Init();	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocator Destructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocator::~CMemoryAllocator()
{
	Term();
	
	m_CS.Free();	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	fallbackAllocator : The allocator that will be used when the allocator cannot 
//		satisfy an allocation request or when a request to free some memory that does
//		not belong to the allocator is found.
//
// Returns
//	True if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocator::Init(IMemoryAllocator* fallbackAllocator)
{
	if(m_Initialized == false)
	{
		m_FallbackAllocator = fallbackAllocator;
		
		m_AllocationTracker.Init();

		// Assign a perf instance to the allocator
		//
		if(PStrStr(m_Name, L"GamePool"))
		{
			m_PerfInstance = g_PerfInstance;
		}
		else
		{
			m_PerfInstance = PERF_GET_INSTANCE(CMemoryAllocator, m_Name);
		}
	
		m_Initialized = true;
		
		return true;
	}
	
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
// Returns
//	True if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocator::Term()
{
	if(m_Initialized)
	{
#if DEBUG_MEMORY_ALLOCATIONS

		if(m_AllocationTracker.GetAliveOriginCount())
		{
			// Do a memory trace for the purpose of showing leaks
			//	
			IMemoryManager::GetInstance().MemTrace("OnTerm : Leak Detected", this);	
		}
#endif

		if(m_PerfInstance)
		{
			if(m_PerfInstance != g_PerfInstance)
			{
				PERF_FREE_INSTANCE(m_PerfInstance);		
			}
			m_PerfInstance = NULL;
		}
		
		m_AllocationTracker.Term();
		
		m_Initialized = false;
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
bool CMemoryAllocator::Flush()
{
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FreeAll
//
// Returns
//	True if the method succeeds, false otherwise
//
// Remarks
//	Does not free internal allocations, just external.
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocator::FreeAll()
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetType
//
// Returns
//	The enum indicating the type of the allocator (CRT, HEAP, etc)
//
/////////////////////////////////////////////////////////////////////////////
MEMORY_ALLOCATOR_TYPE CMemoryAllocator::GetType()
{
	return m_AllocatorType;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Alloc
//
// Parameters
//	size : The number of bytes to allocate
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The pointer to the allocated memory or NULL if the stack ran
//	out of memory or there was an error
//
// Remarks
//	The allocation header comes immediately before the returned
//	pointer
//
//	All allocations are aligned with the default alignment size
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	return AlignedAlloc(MEMORY_FUNC_PASSFL(size, m_DefaultAlignment, fallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AllocZ
//
// Parameters
//	size : The number of bytes to allocate
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The pointer to the allocated memory or NULL if the stack ran
//	out of memory or there was an error
//
// Remarks
//	Zeros the memory returned.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	return AlignedAllocZ(MEMORY_FUNC_PASSFL(size, m_DefaultAlignment, fallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedAllocZ
//
// Parameters:
//	size : The number of bytes requested for allocation
//	alignment : The alignment for the allocation in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zeros the memory returned.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	void* ptr = AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, fallbackAllocator));

	if(ptr)
	{
		ZeroMemory(ptr, size);
	}

	return ptr;	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Realloc
//
// Parameters:
//	ptr : The pointer to realloc
//	size : The new size for the allocation
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::Realloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	return AlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, m_DefaultAlignment, fallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	ReallocZ
//
// Parameters:
//	ptr : The pointer to realloc
//	size : The new size for the allocation
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zero's any additional memory returned if possible.  It is not possible
//	if the header is not stored, since the size is required.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::ReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	return AlignedReallocZ(MEMORY_FUNC_PASSFL(ptr, size, m_DefaultAlignment, fallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedRealloc
//
// Parameters
//	ptr : A pointer to old memory or NULL
//	size : The required size of the new allocation
//	alignment : The requested alignment in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::AlignedRealloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	// Allocate a new block of memory for the request
	//
	void* newPtr = AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, fallbackAllocator));
	
	if(ptr)
	{
		MEMORY_SIZE oldExternalAllocationSize = Size(ptr, fallbackAllocator);

		// Copy the data from the old memory to the new
		//
		if(newPtr && oldExternalAllocationSize)
		{
			// The new allocation could be for a smaller size
			//
			CopyMemory(newPtr, ptr, MIN(oldExternalAllocationSize, size));		
		}
	
		// Free the old memory
		//
		Free(MEMORY_FUNC_PASSFL(ptr, fallbackAllocator));
	}

	return newPtr;				
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedReallocZ
//
// Parameters
//	ptr : A pointer to old memory or NULL
//	size : The required size of the new allocation
//	alignment : The requested alignment in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zero's any additional memory returned if possible.  It is not possible
//	if the header is not stored, since the size is required.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::AlignedReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{	
	MEMORY_SIZE oldExternalAllocationSize = Size(ptr, fallbackAllocator);
	
	void* newPtr = AlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, fallbackAllocator));
	
	if(newPtr)
	{
		// Only zero additional memory if the old size is saved
		//
		if(ptr)
		{
			if(oldExternalAllocationSize)
			{
				BYTE* newExternalAllocation = reinterpret_cast<BYTE*>(newPtr);
			
				// Only zero memory if new memory has been allocated
				//
				if(size > oldExternalAllocationSize)
				{
					ZeroMemory(newExternalAllocation + oldExternalAllocationSize, size - oldExternalAllocationSize);
				}
			}
		}
		// Zero the entire new buffer
		//
		else
		{
			ZeroMemory(newPtr, size);
		}
	}
	
	return newPtr;
}	

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternals
//
// Returns
//	A pointer to the internals interface
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocatorInternals* CMemoryAllocator::GetInternals()
{
	return this;
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
void CMemoryAllocator::GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics)
{
	wcsncpy_s(metrics.Name, arrsize(metrics.Name), m_Name, _TRUNCATE);
	metrics.InternalSize = m_InternalSize;
	metrics.InternalFragmentation = m_InternalFragmentation;
	metrics.ExternalFragmentation = 0;
	metrics.LargestFreeBlock = 0;
	metrics.InternalOverhead = m_InternalOverhead;
	metrics.TotalExternalAllocationSize = m_TotalExternalAllocationSize;
	metrics.TotalExternalAllocationCount = m_TotalExternalAllocationCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTrace
//
// Parameters
//	useLocks : [optional] If false, no locks will be taken
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTrace(bool useLocks)
{
	MemTraceHeader();
	MemTraceInternals();
	MemTraceInternalsEx();
	MemTraceBody(useLocks);
	MemTraceFooter();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFastestGrowingOrigins
//
// Parameters
//	origins : [out] Will have the fastest growing origins added to it.
//
// Remarks
//	Set the origins array to be not grow-able to limit the number of origins
//	returned.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::GetFastestGrowingOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins)
{
	return m_AllocationTracker.GetFastestGrowingOrigins(origins);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetRecentlyChangedOrigins
//
// Parameters
//	origins : [out] Will have the recently changed origins added to it.
//
// Remarks
//	Set the origins array to be not grow-able to limit the number of origins
//	returned.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::GetRecentlyChangedOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins)
{
	return m_AllocationTracker.GetRecentlyChangedOrigins(origins);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InternalAlloc
//
// Parameters
//	size : The number of bytes of memory to allocate
//
// Returns
//	A pointer to the allocated memory or NULL if an error
//
// Remarks
//	Individual allocators can override this method if they want to get
//	their underlying memory from another source
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocator::InternalAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size))
{
	
	void* memory = IMemoryManager::GetInstance().GetSystemAllocator()->Alloc(MEMORY_FUNC_PASSFL(size, NULL));
	
	if(memory)
	{
		AtomicAdd(m_InternalSize, size);
	}
	
	return memory;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	InternalFree
//
// Parameters
//	ptr : A pointer to the memory to free
//	size : The size of the allocation
//
// Remarks
//	Individual allocators can override this method if they want to get
//	their underlying memory from another source
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::InternalFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size))
{
	IMemoryManager::GetInstance().GetSystemAllocator()->Free(MEMORY_FUNC_PASSFL(ptr, NULL));
	
	AtomicAdd(m_InternalSize, -(signed)size);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceHeader
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTraceHeader()
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	
	DWORD age = m_AllocationTracker.GetAge();
		
	MemPrint("\t<allocator Name='%S' Age='%02d:%02d:%02d' Timestamp='%02d:%02d:%02d'>\n", m_Name, age / 3600, (age / 60) % 60, age % 60, time.wHour, time.wMinute, time.wSecond);		
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternals
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTraceInternals()
{
	MEMORY_ALLOCATOR_METRICS metrics;
	GetMetrics(metrics);

	char internalSize[16];
	sprintf_s(internalSize, "%u", (unsigned int)m_InternalSize);
	PStrGroupThousands(internalSize, arrsize(internalSize));
	
	char internalFragmentation[16];
	sprintf_s(internalFragmentation, "%u", (unsigned int)m_InternalFragmentation);
	PStrGroupThousands(internalFragmentation, arrsize(internalFragmentation));
	
	char externalFragmentation[16];
	sprintf_s(externalFragmentation, "%u", metrics.ExternalFragmentation);
	PStrGroupThousands(externalFragmentation, arrsize(externalFragmentation));
	
	char largestFreeBlock[16];
	sprintf_s(largestFreeBlock, "%u", metrics.LargestFreeBlock);
	PStrGroupThousands(largestFreeBlock, arrsize(largestFreeBlock));
	
	char internalOverhead[16];
	sprintf_s(internalOverhead, "%u", (unsigned int)m_InternalOverhead);
	PStrGroupThousands(internalOverhead, arrsize(internalOverhead));
	
	char totalExternalAllocationSize[16];
	sprintf_s(totalExternalAllocationSize, "%u", (unsigned int)m_TotalExternalAllocationSize);
	PStrGroupThousands(totalExternalAllocationSize, arrsize(totalExternalAllocationSize));

	char totalExternalAllocationSizePeak[16];
	sprintf_s(totalExternalAllocationSizePeak, "%u", (unsigned int)m_TotalExternalAllocationSizePeak);
	PStrGroupThousands(totalExternalAllocationSizePeak, arrsize(totalExternalAllocationSizePeak));

	char totalExternalAllocationCount[16];
	sprintf_s(totalExternalAllocationCount, "%u", (unsigned int)m_TotalExternalAllocationCount);
	PStrGroupThousands(totalExternalAllocationCount, arrsize(totalExternalAllocationCount));

	MemPrint("\t\t<internals InternalSize='%u' InternalFragmentation='%u' ExternalFragmentation='%u' LargestFreeBlock='%u' InternalOverhead='%u' TotalExternalAllocationSize='%u' TotalExternalAllocationSizePeak='%u' TotalExternalAllocationCount='%u'/>\n", 
		(unsigned int)m_InternalSize, (unsigned int)m_InternalFragmentation, metrics.ExternalFragmentation, metrics.LargestFreeBlock, (unsigned int)m_InternalOverhead, (unsigned int)m_TotalExternalAllocationSize, (unsigned int)m_TotalExternalAllocationSizePeak, (unsigned int)m_TotalExternalAllocationCount);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternalsEx
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTraceInternalsEx()
{
	MemPrint("\t\t<internalsEx/>\n");
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceBody
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTraceBody(bool useLocks)
{
	MemPrint("\t\t<origins>\n");

	{
		CSLAutoLock lock(useLocks ? &m_CS : NULL);
		m_AllocationTracker.MemTrace(useLocks);	
	}
	
	MemPrint("\t\t</origins>\n");
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceFooter
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemTraceFooter()
{
	MemPrint("\t</allocator>\n");			
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CalculateFragmentationForRegion
//
// Parameters
//	regionSize : The size of the free region in the allocator for which
//		the external fragmentation contribution will be returned
//	minAllocationSize : The minimum allocation size supported by the allocator
//	maxAllocationSize : The maximum allocation size supported by the allocator
//  allocatorSize : The size of the allocator
//
// Returns
//	The contribution to the external fragmentation from the region.
//
// Remarks
//	This method should be called on all free blocks within an allocator and
//	the results should be summed in order to determine external fragmentation.
//	The sum will be some number between 0.0 and 1.0f.  0.0f means completely
//	fragmented.
//
/////////////////////////////////////////////////////////////////////////////
float CMemoryAllocator::CalculateFragmentationForRegion(UINT64 regionSize, UINT64 minAllocationSize, UINT64 maxAllocationSize, UINT64 allocatorSize)
{
	float externalFragmentation = 0.0f;

	// Add the block to the external fragmentation counter
	//
	UINT64 fragmentSize = maxAllocationSize;
	while(fragmentSize >= minAllocationSize && regionSize >= minAllocationSize)
	{		
		if(regionSize >= fragmentSize)
		{
			UINT64 fragmentCount = regionSize / fragmentSize;
			DBG_ASSERT(fragmentCount);
			DBG_ASSERT(fragmentCount <= (allocatorSize / fragmentSize));
			externalFragmentation += ((float)fragmentCount / ((float)allocatorSize / fragmentSize));
			
			regionSize -= fragmentCount * fragmentSize;					
		}
	
		if((fragmentSize >> 1) < minAllocationSize)
		{
			fragmentSize = minAllocationSize;
		}
		else
		{
			fragmentSize >>= 1;
		}
	}
	
	return externalFragmentation;			
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemPrint
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocator::MemPrint(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

#if ISVERSION(SERVER_VERSION)
	char output[512];
	PStrVprintf(output, ARRAYSIZE(output), fmt, args);
	TraceGameInfo("%s", output);
#else
	LogMessageV(MEM_TRACE_LOG, fmt, args);
#endif
}



} // end namespace FSCommon



	


