//----------------------------------------------------------------------------
// memoryallocator_stack.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memoryallocator_stack.h.tmh"
#endif


namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Class CMemoryAllocatorSTACK
//
// Remarks
//	Internal Fragmentation shouldn't show up at all unless there are frees
//	to the stack allocator.  Any freed memory will show up as internal fragmentation.
//
//	The multi-thread safe version does not protect against concurrent free/size/realloc
//	on the same pointer, but only protects the internals of the stack itself from multiple
//	alloc/frees.
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
class CMemoryAllocatorSTACK : public CMemoryAllocator
{

	// ALLOCATION_HEADER
	//
	#pragma pack(push, 1)	
	struct ALLOCATION_HEADER : public CMemoryAllocator::ALLOCATION_HEADER
	{
		MEMORY_SIZE				ExternalAllocationSize;
		MEMORY_SIZE				InternalAllocationSize;
		unsigned int			MagicHeader; // Magic number used to determine if the allocation was corrupted at time of free
	};	
	#pragma pack(pop)	
	
	public:
				
		CMemoryAllocatorSTACK(WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);		
		virtual ~CMemoryAllocatorSTACK();
		
	public: // IMemoryAllocator
			
		virtual bool											Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE initialSize, MEMORY_SIZE growSize, MEMORY_SIZE maxAllocationSize = 0);		
		virtual bool											Term();	
		virtual bool											Flush();	
		virtual bool											FreeAll();

		// Alloc
		//
		virtual void*											AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
	
		// Free
		//
		virtual void											Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator));
		
		// Size
		//
		virtual MEMORY_SIZE										Size(void* ptr, IMemoryAllocator* fallbackAllocator);
		
	public: // IMemoryAllocatorInternals
	
		virtual void*											GetInternalPointer(void* ptr);
		virtual MEMORY_SIZE										GetInternalSize(void* ptr);
		virtual void											GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics);			
			
	protected:
	
		virtual void											MemTraceInternalsEx();	
				
	private:
	
		MEMORY_SIZE												GetExtraBytesForOverhead(unsigned int alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize);
	
	private:
	
		// The number of bytes that the stack will grow by when there are no free blocks
		// capable of servicing an allocation request
		//
		MEMORY_SIZE												m_GrowSize;
		CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>*	m_GrowAllocator;
		IMemoryAllocator*										m_OriginalFallbackAllocator;
		
		MEMORY_SIZE												m_MaxAllocationSize;
		
		// A pointer to the beginning of the stack memory block
		//
		BYTE*													m_Memory;		
		
		// A pointer to the stack memory block of the next free byte of memory
		//
		BYTE*													m_NextFreeBlock;
		
	
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorSTACK Constructor
//
// Parameters
//	name : [optional] The friendly name of the allocator
//	defaultAlignment : [optional] A byte alignment for all default aligned
//		allocations from the stack.  Defaults to 16-byte alignment
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::CMemoryAllocatorSTACK(WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocator(MEMORY_ALLOCATOR_STACK, name, defaultAlignment),
	m_Memory(NULL),
	m_NextFreeBlock(0),
	m_GrowSize(0),
	m_GrowAllocator(NULL),
	m_MaxAllocationSize(0),
	m_OriginalFallbackAllocator(NULL)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocator Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::~CMemoryAllocatorSTACK()
{
	// Make sure any memory gets released
	//
	Term();		
}
	
/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Parameters
//	fallbackAllocator : A pointer to the allocator that will be used if the stack runs out of 
//		space or if a Free is done on a pointer that does not belong to the stack
//	initialSize : The initial size of the stack
//	growSize : The number of bytes that the stack will grow by when it runs
//		out of space.  Specifying zero means the stack will not grow.
//	maxAllocationSize : [optional] If specified, allocation requests over the 
//		specified amount will be passed to the fallback allocator
//
// Remarks
//	This method will allocate memory using the virtual page manager
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
bool CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::Init(IMemoryAllocator* fallbackAllocator, MEMORY_SIZE initialSize, MEMORY_SIZE growSize, MEMORY_SIZE maxAllocationSize)
{
	if(m_Initialized == false)
	{	
		__super::Init(fallbackAllocator);
		
		CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);
		
		m_GrowSize = growSize;
		m_MaxAllocationSize = maxAllocationSize;
		m_OriginalFallbackAllocator = fallbackAllocator;
		
		DBG_ASSERT(maxAllocationSize <= growSize);
					
		// Allocate memory from the virtual page manager
		//
		m_Memory = reinterpret_cast<BYTE*>(InternalAlloc(MEMORY_FUNC_FILELINE(initialSize)));
		
		// Start allocating from the beginning of the stack
		//
		m_NextFreeBlock = m_Memory;

		// Handle out of memory
		//
		if(m_Memory == NULL)
		{
			return false;
		}
	}
	
	m_Initialized = true;
	
	return true;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
// Remarks
//	This method will release memory the virtual memory through the virtual
//	page manager
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
bool CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::Term()
{
	if(m_Initialized)
	{
		__super::Term();
		
		CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

		// Release the memory back to the virtual page manager
		//
		if(m_Memory)
		{
			InternalFree(MEMORY_FUNC_FILELINE(m_Memory, (unsigned)m_InternalSize));
			m_Memory = NULL;			
		}

		// Terminate the grow allocator
		//
		if(m_GrowAllocator)
		{
			IMemoryManager::GetInstance().RemoveAllocator(m_GrowAllocator);
			
			m_GrowAllocator->Term();
			delete m_GrowAllocator;
			m_GrowAllocator = NULL;
		}
		
		m_OriginalFallbackAllocator = NULL;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
bool CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::Flush()
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

	// If the grow allocator has no more external allocations, it can
	// be removed
	//
	if(m_GrowAllocator && m_GrowAllocator->Flush())
	{
		m_GrowAllocator->Term();
		IMemoryManager::GetInstance().RemoveAllocator(m_GrowAllocator);
		delete m_GrowAllocator;						
		m_GrowAllocator = NULL;	
		
		m_FallbackAllocator = m_OriginalFallbackAllocator;
	}
	
	if(m_GrowAllocator == NULL && m_TotalExternalAllocationCount == 0)
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
//	FreeAll
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
bool CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::FreeAll()
{
	CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

	// Reset the stack pointer
	//
	m_NextFreeBlock = m_Memory;	
	
	return true;	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedAlloc
//
// Parameters:
//	size : The number of bytes requested for allocation
//	alignment : The alignment for the allocation in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
void* CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
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
	
	{
		CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

		// For alignment and header purposes, we need to internally allocate more
		// bytes than requested
		//
		MEMORY_SIZE alignmentOverheadSize = 0;
		MEMORY_SIZE headerSize = 0;
		MEMORY_SIZE footerSize = 0;
		MEMORY_SIZE totalOverheadSize = GetExtraBytesForOverhead(alignment, alignmentOverheadSize, headerSize, footerSize);
		
		MEMORY_SIZE internalAllocationSize = size + totalOverheadSize;
		
		// Make sure there is enough memory remaining in the stack
		//
		DBG_ASSERT(m_Memory);
		DBG_ASSERT(m_InternalSize);
		DBG_ASSERT(m_NextFreeBlock);
			
		size_t remainingBytes =  m_InternalSize - (m_NextFreeBlock - m_Memory);
		
		// Grab the externalAllocationPointer
		//
		BYTE* externalAllocationPtr = NULL;
		
		// If there is enough room in the stack for the allocation
		//
		if(remainingBytes >= internalAllocationSize)
		{
			// Grab some memory from the stack
			//
			externalAllocationPtr = m_NextFreeBlock + alignmentOverheadSize + headerSize;	

			// Increment the free block pointer
			//
			m_NextFreeBlock += internalAllocationSize;	
			
			// Fill the allocation header and footer
			//
			if(USE_HEADER_AND_FOOTER)
			{
				ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - headerSize);
				allocationHeader->Origin = m_AllocationTracker.OnAlloc(MEMORY_FUNC_PASSFL(externalAllocationPtr, size));						
				allocationHeader->ExternalAllocationSize = size;
				allocationHeader->InternalAllocationSize = internalAllocationSize;
				allocationHeader->MagicHeader = cMagicHeader;
				
				ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + size);
				allocationFooter->MagicFooter = cMagicFooter;
				
				// Update Metrics
				//
				m_InternalOverhead += internalAllocationSize - size;
				m_TotalExternalAllocationSize += size;
				m_TotalExternalAllocationSizePeak = MAX<MEM_COUNTER>(m_TotalExternalAllocationSize, m_TotalExternalAllocationSizePeak);		

				// Server counters
				//
				if(m_PerfInstance)
				{
					PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, internalAllocationSize);
				}
				
			}

			// Update Metrics
			//
			++m_TotalExternalAllocationCount;							

			// Server counters
			//
			if(m_PerfInstance)
			{
				PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalAllocationRate, 1);
			}

			DBG_ASSERT((size_t)externalAllocationPtr % alignment == 0);
			return externalAllocationPtr;				
		}
		// If the stack is out of room and the stack should grow
		//
		else if(m_GrowSize && m_GrowAllocator == NULL)
		{
			// Create a new stack allocator that will be the new fall back allocator
			//
			m_GrowAllocator = new CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>(m_Name);
			if(m_GrowAllocator->Init(m_FallbackAllocator, m_GrowSize, m_GrowSize, m_MaxAllocationSize))
			{
				IMemoryManager::GetInstance().AddAllocator(m_GrowAllocator);
			}
			else
			{
				delete m_GrowAllocator;
				m_GrowAllocator = NULL;
			}

			m_FallbackAllocator = m_GrowAllocator;
			
		}
	}
	
	if(fallbackAllocator)
	{
		
		// Attempt to allocate memory from a different allocator
		//
		return fallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}	
	else if(m_FallbackAllocator)
	{
		
		// Attempt to allocate memory from a different allocator
		//
		return m_FallbackAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, NULL));
	}
	else
	{
		return NULL;
	}

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
// Parameters
//	ptr : A pointer to the memory to free
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
void CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator))
{
	MEMORY_DEBUG_REF(file);
	MEMORY_DEBUG_REF(line);
	
	if(ptr == NULL || m_Initialized == false)
	{
		return;
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	
	// Check to see if the pointer is within the stack's memory
	//
	if(externalAllocationPtr >= m_Memory && externalAllocationPtr < m_Memory + m_InternalSize)
	{
		CSLAutoLock lock(MT_SAFE ? &m_CS : NULL);

		// Check for corrupted memory
		//
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
			
			// Adjust metrics
			//
			m_InternalFragmentation += allocationHeader->InternalAllocationSize;
			m_InternalOverhead -= allocationHeader->InternalAllocationSize - allocationHeader->ExternalAllocationSize;
			m_TotalExternalAllocationSize -= allocationHeader->ExternalAllocationSize;

			// Server counters
			//
			if(m_PerfInstance)
			{
				PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, -(signed)allocationHeader->InternalAllocationSize);
			}
			
		}

		// Adjust metrics
		//
		--m_TotalExternalAllocationCount;
		
		// Reset the stack pointer if there are no outstanding allocations
		//
		if(m_TotalExternalAllocationCount == 0)
		{
			m_NextFreeBlock = m_Memory;
			m_InternalFragmentation = 0;
		}	

		// Server counters
		//
		if(m_PerfInstance)
		{
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalFreeRate, 1);
		}
		
	}
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
//	ptr : The pointer for which to return the size
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The size of the allocation pointed to by ptr
//
// Remarks
//	This is only supported if headers/footers are allocated, otherwise
//	it just asserts
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
MEMORY_SIZE	CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::Size(void* ptr, IMemoryAllocator* fallbackAllocator)
{
	if(ptr == NULL)
	{
		return 0;
	}
	
	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	
	// Check to see if the pointer is within the stack's memory
	//
	if(externalAllocationPtr < m_Memory || externalAllocationPtr > m_Memory + m_InternalSize)
	{
		if(fallbackAllocator)
		{
			return fallbackAllocator->Size(ptr, NULL);
		}	
		if(m_FallbackAllocator)
		{
			return m_FallbackAllocator->Size(ptr, NULL);
		}
		else
		{
			ASSERT(0);
			return 0;
		}
	}

	if(USE_HEADER_AND_FOOTER)
	{
		ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));

		MEMORY_SIZE externalAllocationSize = allocationHeader->ExternalAllocationSize;
				
		return externalAllocationSize;
	}
	else
	{
		ASSERT(0);
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
void* CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::GetInternalPointer(void* ptr)
{
	if(ptr == NULL)
	{
		return NULL;
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	
	// Check to see if the pointer is within the stack's memory
	//
	if(externalAllocationPtr < m_Memory || externalAllocationPtr > m_Memory + m_InternalSize)
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
		
	BYTE* internalAllocationPtr = externalAllocationPtr;

	if(USE_HEADER_AND_FOOTER)
	{
		ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));
		internalAllocationPtr -= allocationHeader->InternalAllocationSize - allocationHeader->ExternalAllocationSize - sizeof(ALLOCATION_FOOTER);
	}
	
	return internalAllocationPtr;
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
MEMORY_SIZE CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::GetInternalSize(void* ptr)
{
	if(ptr == NULL)
	{
		ASSERT(0);
		return 0;		
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	
	// Check to see if the pointer is within the stack's memory
	//
	if(externalAllocationPtr < m_Memory || externalAllocationPtr > m_Memory + m_InternalSize)
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
	
	if(USE_HEADER_AND_FOOTER == false)
	{
		ASSERT(0);
		return 0;
	}

	ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));
	return allocationHeader->InternalAllocationSize;	
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
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
void CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics)
{
	__super::GetMetrics(metrics);
	
	metrics.LargestFreeBlock = (MEM_COUNTER)((m_Memory + m_InternalSize) - m_NextFreeBlock);
	metrics.InternalSize = m_InternalSize;
	
	float externalFragmentation = 0.0f;
	
	MEMORY_SIZE alignmentOverheadSize = 0;
	MEMORY_SIZE headerSize = 0;
	MEMORY_SIZE footerSize = 0;
	UINT64 minAllocationSize = 1 + GetExtraBytesForOverhead(m_DefaultAlignment, alignmentOverheadSize, headerSize, footerSize);
	UINT64 maxAllocationSize = m_InternalSize;
	
	// Calculate the external fragmentation
	//
	UINT64 regionSize = (UINT64)m_InternalSize - (m_NextFreeBlock - m_Memory);
	
	externalFragmentation += CalculateFragmentationForRegion(regionSize, minAllocationSize, maxAllocationSize, m_InternalSize);		
	
	externalFragmentation = (1.0f - externalFragmentation) * 100;
	
	metrics.ExternalFragmentation = (MEM_COUNTER)externalFragmentation;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternalsEx
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
void CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::MemTraceInternalsEx()
{
	MemPrint("\t\t<internalsEx GrowSize='%u'/>\n", m_GrowSize);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetExtraBytesForOverhead
//
// Parameters
//	alignment : The alignment for the external allocation request
//	alignmentOverheadSize : [out] A reference to a variable that will be set to the size in
//		bytes required before the header for alignment
//	headerSize : [out] A reference to a variable that will be set to the header
//		size
//	footerSize : [out] A reference to a variable that will be set to the footer
//		size
//
// Remarks
//	Returns the number of extra bytes that need to be allocated
//	to satisfy the allocation request, including bytes for header/footer and
//	for alignment
//
//  No locks are needed here because it is assumed that it will be called
//	while the stack's CS is already taken.
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER, bool MT_SAFE>
MEMORY_SIZE	CMemoryAllocatorSTACK<USE_HEADER_AND_FOOTER, MT_SAFE>::GetExtraBytesForOverhead(unsigned int alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize)
{
	headerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_HEADER) : 0;
	footerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_FOOTER) : 0;

	// Since any allocation header must come before the external pointer, adjust the next block
	// pointer for the purpose of alignment
	//
	BYTE* headerAdjustedNextBlockPtr = m_NextFreeBlock + headerSize;
	
	// Figure out how many extra bytes are needed for the header and alignment
	//
	MEMORY_SIZE extraBytesNeededForAlignmentAndHeader = static_cast<MEMORY_SIZE>(headerSize + ((alignment - ((size_t)headerAdjustedNextBlockPtr % alignment)) % alignment));
	
	alignmentOverheadSize = extraBytesNeededForAlignmentAndHeader - headerSize;
		
	return extraBytesNeededForAlignmentAndHeader + footerSize;
}

} // end namespace FSCommon

