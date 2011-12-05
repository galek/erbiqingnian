//----------------------------------------------------------------------------
// memoryallocator_crt.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator.h"

#pragma warning(push)
#pragma warning(disable:4127)

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memoryallocator_crt.h.tmh"
#endif

namespace FSCommon
{

// Class CMemoryAllocatorCRT
//
template<bool USE_HEADER_AND_FOOTER>
class CMemoryAllocatorCRT : public CMemoryAllocator
{
	protected:
	
		// ALLOCATION_HEADER
		//
#pragma pack(push, 1)	

		struct ALLOCATION_SIZE_HEADER
		{
			MEMORY_SIZE				AlignmentBytes; // The number of bytes before the header needed for alignment
			MEMORY_SIZE				ExternalAllocationSize;
		};

		struct ALLOCATION_HEADER : public CMemoryAllocator::ALLOCATION_HEADER, public ALLOCATION_SIZE_HEADER
		{		
			unsigned int			MagicHeader; // Magic number used to determine if the allocation was corrupted at time of free
		};
#pragma pack(pop)	

#ifdef _WIN64	
		static const MEMORY_SIZE	cDefaultAlignmentCRT = 16;
#else
		static const MEMORY_SIZE	cDefaultAlignmentCRT = 8;
#endif

	public:
		
		CMemoryAllocatorCRT(const WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);
		
	public: // IMemoryAllocator
			
		virtual bool			Init(IMemoryAllocator* fallbackAllocator);
			
		// Alloc
		//		
		virtual void*			AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
		// Free
		//
		virtual void			Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator));
		
		// Size
		//
		virtual MEMORY_SIZE		Size(void* ptr, IMemoryAllocator* fallbackAllocator);

	public: // IMemoryAllocatorInternals
	
		virtual void*			GetInternalPointer(void* ptr);
		virtual MEMORY_SIZE		GetInternalSize(void* ptr);	
		virtual void			GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics);	

	protected:
	
		virtual void			MemTraceInternalsEx();
	
	protected:

		MEMORY_SIZE 			GetExtraBytesForOverhead(MEMORY_SIZE alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize);
	
	protected:
	
		HANDLE					m_Heap; // handle to windows heap			
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorCRT Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::CMemoryAllocatorCRT(const WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocator(MEMORY_ALLOCATOR_CRT, name, defaultAlignment),	
	m_Heap(NULL)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
bool CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::Init(IMemoryAllocator* fallbackAllocator)
{
	if(m_Initialized == false)
	{
		__super::Init(fallbackAllocator);
		
		m_Heap = GetProcessHeap();
		
		if(m_Heap == NULL)
		{
			Term();
			return false;
		}		
	}
	
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
// Remarks
//	CRT heap guarantees 16-byte alignment
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
void* CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	if(size == 0 || m_Initialized == false)
	{
		return NULL;
	}

	REF(fallbackAllocator);
	MEMORY_DEBUG_REF(line);
	MEMORY_DEBUG_REF(file);
			
	if (size == 0)
	{
		return NULL;
	}

	// For alignment and header purposes, we need to internally allocate more
	// bytes than requested
	//
	MEMORY_SIZE alignmentOverheadSize = 0;
	MEMORY_SIZE headerSize = 0;
	MEMORY_SIZE footerSize = 0;
	MEMORY_SIZE totalOverheadSize = GetExtraBytesForOverhead(alignment, alignmentOverheadSize, headerSize, footerSize);
	
	MEMORY_SIZE internalAllocationSize = size + totalOverheadSize;	
	
	BYTE* internalAllocationPtr = reinterpret_cast<BYTE*>(HeapAlloc(m_Heap, 0, internalAllocationSize));
	BYTE* externalAllocationPtr = NULL;

	if(internalAllocationPtr)
	{
		externalAllocationPtr = internalAllocationPtr + headerSize;
		externalAllocationPtr += (alignment - ((size_t)externalAllocationPtr % alignment)) % alignment;
		DBG_ASSERT(externalAllocationPtr >= internalAllocationPtr);
		DBG_ASSERT((size_t)(externalAllocationPtr - internalAllocationPtr) <= (alignmentOverheadSize + headerSize));
		alignmentOverheadSize = (MEMORY_SIZE)((size_t)(externalAllocationPtr - internalAllocationPtr) - headerSize);
		DBG_ASSERT(internalAllocationPtr + alignmentOverheadSize + headerSize == externalAllocationPtr);

		// Fill the allocation header and footer
		//
		if(USE_HEADER_AND_FOOTER)
		{
			ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - headerSize);
			allocationHeader->Origin = m_AllocationTracker.OnAlloc(MEMORY_FUNC_PASSFL(externalAllocationPtr, size));
			allocationHeader->AlignmentBytes = alignmentOverheadSize;
			allocationHeader->ExternalAllocationSize = size;
			allocationHeader->MagicHeader = cMagicHeader;
			
			ALLOCATION_FOOTER* allocationFooter = reinterpret_cast<ALLOCATION_FOOTER*>(externalAllocationPtr + size);
			allocationFooter->MagicFooter = cMagicFooter;
		}
		else
		{
			ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_SIZE_HEADER));
			allocationHeader->ExternalAllocationSize = size;			
			allocationHeader->AlignmentBytes = alignmentOverheadSize;			
		}
		
		// Update metrics
		//
		++m_TotalExternalAllocationCount;
		m_TotalExternalAllocationSize += size;
		m_TotalExternalAllocationSizePeak = MAX<MEM_COUNTER>(m_TotalExternalAllocationSize, m_TotalExternalAllocationSizePeak);
		m_InternalOverhead += totalOverheadSize;
		m_InternalSize += internalAllocationSize;
		
		// Server counters
		//
		if(m_PerfInstance)
		{
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, internalAllocationSize);
			PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalAllocationRate, 1);
		}
	}

	DBG_ASSERT((size_t)externalAllocationPtr % alignment == 0);
	return externalAllocationPtr;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
// Remarks
//	Since there is no way to determine if the pointer was allocated using
//	the CRT, there can be no fall back allocator
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
void CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator))
{
	MEMORY_DEBUG_REF(line);
	MEMORY_DEBUG_REF(file);

	if(ptr == NULL || m_Initialized == false)
	{
		return;
	}
	
	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);	
	BYTE* internalAllocationPtr = reinterpret_cast<BYTE*>(GetInternalPointer(externalAllocationPtr));

	MEMORY_SIZE externalAllocationSize = Size(ptr, fallbackAllocator);
	MEMORY_SIZE internalAllocationSize = GetInternalSize(ptr);
	
	// Check sentinel
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

		m_AllocationTracker.OnFree(MEMORY_FUNC_PASSFL(ptr, externalAllocationSize, allocationHeader->Origin));
	}

	// Update metrics
	//	
	m_TotalExternalAllocationSize -= externalAllocationSize;
	--m_TotalExternalAllocationCount;
	m_InternalOverhead -= internalAllocationSize - externalAllocationSize;
	m_InternalSize -= internalAllocationSize;

	// Server counters
	//
	if(m_PerfInstance)
	{
		PERF_ADD64(m_PerfInstance, CMemoryAllocator, TotalExternalAllocationSize, -(signed)internalAllocationSize);
		PERF_ADD64(m_PerfInstance, CMemoryAllocator, ExternalFreeRate, 1);
	}


	HeapFree(m_Heap, 0, internalAllocationPtr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Size
// 
// Parameters
//	ptr : The pointer for which the size of the external allocation will be returned
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The size of the external allocation or 0 if the pointer is NULL.
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
MEMORY_SIZE	CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::Size(void* ptr, IMemoryAllocator* fallbackAllocator)
{
	REF(fallbackAllocator);

	if(ptr == NULL)
	{
		return 0;
	}

	MEMORY_SIZE externalAllocationSize = 0;
	
	if(USE_HEADER_AND_FOOTER)
	{
		ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>((BYTE*)ptr - sizeof(ALLOCATION_HEADER));
		externalAllocationSize = allocationHeader->ExternalAllocationSize;
	}
	else
	{
		ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>((BYTE*)ptr - sizeof(ALLOCATION_SIZE_HEADER));
		externalAllocationSize = allocationHeader->ExternalAllocationSize;
	}
	
	return externalAllocationSize;
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
//	A pointer to the internal allocation
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
void* CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::GetInternalPointer(void* ptr)
{
	if(ptr == NULL)
	{
		return NULL;
	}

	BYTE* externalAllocationPtr = reinterpret_cast<BYTE*>(ptr);
	BYTE* internalAllocationPtr = externalAllocationPtr;

	if(USE_HEADER_AND_FOOTER)
	{
		ALLOCATION_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_HEADER));
		internalAllocationPtr -= allocationHeader->AlignmentBytes + sizeof(ALLOCATION_HEADER);		
	}
	else
	{
		ALLOCATION_SIZE_HEADER* allocationHeader = reinterpret_cast<ALLOCATION_SIZE_HEADER*>(externalAllocationPtr - sizeof(ALLOCATION_SIZE_HEADER));
		internalAllocationPtr -= allocationHeader->AlignmentBytes + sizeof(ALLOCATION_SIZE_HEADER);
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
//	The size of the internal allocation
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
MEMORY_SIZE CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::GetInternalSize(void* ptr)
{
	void* internalAllocationPtr = GetInternalPointer(ptr);
	
	if(internalAllocationPtr == NULL)
	{
		return 0;
	}
	
	size_t size = HeapSize(m_Heap, 0, internalAllocationPtr);
	if(size == -1)
	{
		DBG_ASSERT(0);
	}
	
	return (MEMORY_SIZE)size;

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
template<bool USE_HEADER_AND_FOOTER>
void CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics)
{
	__super::GetMetrics(metrics);
	
	float externalFragmentation = 0.0f;
	
#ifndef _WIN64	
	
	MEMORY_SIZE alignmentOverheadSize = 0;
	MEMORY_SIZE headerSize = 0;
	MEMORY_SIZE footerSize = 0;
	UINT64 minAllocationSize = 1 + GetExtraBytesForOverhead(m_DefaultAlignment, alignmentOverheadSize, headerSize, footerSize);
	UINT64 maxAllocationSize = (UINT64)((unsigned)2 * GIGA);
	
	// Search through the virtual address space for the largest free block
	//
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	
	BYTE* virtualStart = reinterpret_cast<BYTE*>(systemInfo.lpMinimumApplicationAddress);
	BYTE* virtualEnd = reinterpret_cast<BYTE*>(systemInfo.lpMaximumApplicationAddress);
	virtualEnd = MIN<BYTE*>(virtualEnd, virtualStart + ((unsigned int)2 * GIGA));
	
	while(virtualStart < systemInfo.lpMaximumApplicationAddress)
	{
		MEMORY_BASIC_INFORMATION memoryInfo;
		ZeroMemory(&memoryInfo, sizeof(MEMORY_BASIC_INFORMATION));
		
		if(VirtualQuery(virtualStart, &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
		{
			break;
		}
		DBG_ASSERT(memoryInfo.AllocationBase <= memoryInfo.BaseAddress);

		
		if(memoryInfo.State == MEM_FREE)
		{
			metrics.LargestFreeBlock = MAX<MEM_COUNTER>(metrics.LargestFreeBlock, (MEM_COUNTER)memoryInfo.RegionSize);
			
			// Add the block to the external fragmentation counter
			//
			externalFragmentation += CalculateFragmentationForRegion((UINT64)memoryInfo.RegionSize, minAllocationSize, maxAllocationSize, maxAllocationSize);
		}
		
		virtualStart = reinterpret_cast<BYTE*>(memoryInfo.BaseAddress) + memoryInfo.RegionSize;				
	}	
	
	externalFragmentation = (1.0f - externalFragmentation) * 100;
	
#endif // _WIN64
	
	metrics.ExternalFragmentation = (MEM_COUNTER)externalFragmentation;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTraceInternalsEx
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
void CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::MemTraceInternalsEx()
{
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memStatus);	

	MemPrint("\t\t<internalsEx MemoryLoad='%u' TotalPhysical='%I64u' AvailablePhysical='%I64u' TotalPageFile='%I64u' AvailablePageFile='%I64u' TotalVirtual='%I64u' AvailableVirtual='%I64u' AvailableExtendedVirtual='%I64u'/>\n",
		memStatus.dwMemoryLoad, memStatus.ullTotalPhys, memStatus.ullAvailPhys, memStatus.ullTotalPageFile, memStatus.ullAvailPageFile, memStatus.ullTotalVirtual, memStatus.ullAvailVirtual, memStatus.ullAvailExtendedVirtual);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetExtraBytesForOverhead
//
// Parameters
//	alignment : The requested alignment for the allocation
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
//	No locks are needed here because it is assumed that it will be called
//	while the stack's CS is already taken.
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
MEMORY_SIZE CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>::GetExtraBytesForOverhead(MEMORY_SIZE alignment, MEMORY_SIZE& alignmentOverheadSize, MEMORY_SIZE& headerSize, MEMORY_SIZE& footerSize)
{
	
	headerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_HEADER) : sizeof(ALLOCATION_SIZE_HEADER);
	footerSize = USE_HEADER_AND_FOOTER ? sizeof(ALLOCATION_FOOTER) : 0;
	
	MEMORY_SIZE extraBytesNeededForAlignmentAndHeader = 0;
	
	if(headerSize > alignment)
	{
		extraBytesNeededForAlignmentAndHeader = static_cast<MEMORY_SIZE>(headerSize + ((alignment - (headerSize % alignment)) % alignment));
	}
	else
	{
		extraBytesNeededForAlignmentAndHeader = alignment + headerSize - 1;
	}
	
	alignmentOverheadSize = extraBytesNeededForAlignmentAndHeader - headerSize;
		
	return extraBytesNeededForAlignmentAndHeader + footerSize;
}


} // end namespace FSCommon

#pragma warning(pop)