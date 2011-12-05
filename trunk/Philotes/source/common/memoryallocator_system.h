//----------------------------------------------------------------------------
// memoryallocator_system.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator_pool.h"

#pragma warning(push)
#pragma warning(disable:4100)

namespace FSCommon
{

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
static const MEMORY_BIN_DEF cSystemBinDef[] =
{ // size,		gran,		bucketsize
	4*KILO,		4*KILO,		64*KILO,
	8*KILO, 	8*KILO,		64*KILO,
	16*KILO,	8*KILO,		128*KILO,
	32*KILO,	8*KILO,		256*KILO,
	64*KILO,	8*KILO,		512*KILO,
	128*KILO,	8*KILO,		1024*KILO,
	264*KILO,	8*KILO,		1088*KILO	
};

// Class CMemoryAllocatorSYSTEM
//
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
class CMemoryAllocatorSYSTEM : public CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>
{
	protected:
	

	public:
				
		CMemoryAllocatorSYSTEM(const WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);		
		virtual ~CMemoryAllocatorSYSTEM();

	public: // IMemoryAllocator
	
		// Alloc
		//
		virtual bool						Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, bool allowTrim, bool optimizeForTrim);		
		virtual void*						AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
	protected:
	
		virtual bool						InitBins(const MEMORY_BIN_DEF* bindef, unsigned int count, MEMORY_SIZE maxAllocationSize, bool optimizeForTrim);		

	protected:
	
		virtual void*						InternalAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size));
		virtual void						InternalFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size));

	private:

		MEMORY_SIZE							m_PageSize;
		MEMORY_SIZE							m_AllocationGranularity;

};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorSYSTEM Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::CMemoryAllocatorSYSTEM(const WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocatorPOOL<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>(name, defaultAlignment)
{
	m_AllocatorType = MEMORY_ALLOCATOR_SYSTEM;
	m_PageSize = IMemoryManager::GetInstance().GetPageSize();
	m_AllocationGranularity = IMemoryManager::GetInstance().GetAllocationGranularity();
		
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorPOOL Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::~CMemoryAllocatorSYSTEM()
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
//
// Returns
//	true if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
bool CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE maxAllocationSize, bool allowTrim, bool optimizeForTrim)
{
	m_Bins.SetAllocator(fallbackAllocator);
	m_BucketTracker.SetAllocator(fallbackAllocator);
	m_BucketPool.SetAllocator(fallbackAllocator);
	
	return __super::Init(fallbackAllocator, maxAllocationSize, allowTrim, optimizeForTrim);
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
// Remarks
//	The system allocator overrides any alignment since all allocations must
//	be page aligned
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
void* CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	DBG_ASSERT(alignment <= m_PageSize);
	return __super::AlignedAlloc(MEMORY_FUNC_PASSFL(size, m_PageSize, fallbackAllocator));
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
bool CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::InitBins(const MEMORY_BIN_DEF* bindef, unsigned int count, MEMORY_SIZE maxAllocationSize, bool optimizeForTrim)
{	
	return __super::InitBins(cSystemBinDef, arrsize(cSystemBinDef), maxAllocationSize, optimizeForTrim);
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
void* CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::InternalAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size))
{
	ASSERTV(size % m_AllocationGranularity == 0, "System allocation not granuality-sized. -Didier");
	
	void* memory = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);		
	
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
//	size : The size of the memory
//
// Remarks
//	Individual allocators can override this method if they want to get
//	their underlying memory from another source
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE, unsigned int MAX_BUCKET_COUNT>
void CMemoryAllocatorSYSTEM<USE_HEADER_AND_FOOTER, MT_SAFE, MAX_BUCKET_COUNT>::InternalFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size))
{
	ASSERTV(size % m_AllocationGranularity == 0, "System free not granuality-sized. -Didier");
	
	VirtualFree(ptr, 0, MEM_RELEASE);
}

} // end namespace FSCommon

#pragma warning(pop)