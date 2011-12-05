//----------------------------------------------------------------------------
// memoryallocator.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator_i.h"
#include "memoryallocatorinternals_i.h"
#include "memoryallocationtracker.h"
#include "CrashHandler.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#endif

namespace FSCommon
{

// Class CMemoryAllocator
//
class CMemoryAllocator : public IMemoryAllocator, public IMemoryAllocatorInternals
{

	protected:
	
		// 16-byte default alignment
		//
		static const unsigned int cDefaultAlignment = 16;

	protected:

		// ALLOCATION_HEADER
		//
#pragma pack(push, 1)			
		struct ALLOCATION_HEADER
		{
			CMemoryAllocationOrigin*			Origin;
			const char* 						ExpirationFile;	// The file where the memory was last freed
			unsigned int						ExpirationLine;	// The line where the memory was last freed
		};

		// ALLOCATION_FOOTER
		//
		struct ALLOCATION_FOOTER
		{
			unsigned int						MagicFooter; // Magic number used to determine if the allocation was corrupted at time of free
		};
#pragma pack(pop)			

	protected:
	
		CMemoryAllocator(MEMORY_ALLOCATOR_TYPE allocatorType, 
			const WCHAR* name = NULL, 
			unsigned int defaultAlignment = cDefaultAlignment);

	public:
	
		virtual ~CMemoryAllocator();
		
	public: // IMemoryAllocator
	
		virtual bool							Init(IMemoryAllocator* fallbackAllocator);
		virtual bool							Term();
		virtual bool							Flush();
		virtual bool							FreeAll();	
		virtual MEMORY_ALLOCATOR_TYPE			GetType();
		
		// Alloc
		//		
		virtual void*							Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
				
		// Realloc
		//
		virtual void*							Realloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							ReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AlignedRealloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));		
		virtual void*							AlignedReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
		virtual IMemoryAllocatorInternals*		GetInternals();
		
	public: // IMemoryAllocatorInternals
		
		virtual void							GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics);
		virtual void							MemTrace(bool useLocks = true);
		virtual void							GetFastestGrowingOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins);		
		virtual void							GetRecentlyChangedOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins);		
		
	protected:
	
		virtual void*							InternalAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size));
		virtual void							InternalFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size));
		void									MemTraceHeader();
		void									MemTraceInternals();
		virtual void							MemTraceInternalsEx();
		virtual void  							MemTraceBody(bool useLocks);
		void									MemTraceFooter();
		float									CalculateFragmentationForRegion(UINT64 regionSize, UINT64 minAllocationSize, UINT64 maxAllocationSize, UINT64 allocatorSize);
		void									MemPrint(char* fmt, ...);
		
	public:
	
		static PERF_INSTANCE(CMemoryAllocator)* g_PerfInstance;		
		
	protected:
	
		// Used for the server build's counters
		//
		PERF_INSTANCE(CMemoryAllocator)*		m_PerfInstance;	
		
		CCritSectLite							m_CS;		
	
		bool									m_Initialized;
	
		// Allocator type
		//
		MEMORY_ALLOCATOR_TYPE   				m_AllocatorType;
		
		// The default alignment for the stack (16-byte aligned default).  This
		// can be set in the constructor
		//
		unsigned int							m_DefaultAlignment;		

		// Tracks the total internal size of the allocator
		//
		volatile MEM_COUNTER					m_InternalSize;

		// Tracks the wasted space within all current externally allocated blocks including: unused space
		//
		volatile MEM_COUNTER					m_InternalFragmentation;					

		// Tracks wasted space due to overhead within the system including: headers/footers, alignment
		//
		volatile MEM_COUNTER					m_InternalOverhead;	

		// Total number of bytes currently externally allocated.  This does not include
		// allocations for the fall back allocator
		//
		volatile MEM_COUNTER					m_TotalExternalAllocationSize; 
		volatile MEM_COUNTER					m_TotalExternalAllocationSizePeak;

		// Count of the number of current external allocations.  This does not include allocations
		// for the fall back allocator
		//
		volatile MEM_COUNTER					m_TotalExternalAllocationCount;

		// A friendly name for the allocator
		//
		WCHAR									m_Name[32]; // name of allocator
		
		// Allocations that fail or frees that do not belong to the allocator
		// fall back to this allocator
		//
		IMemoryAllocator*						m_FallbackAllocator;	

		CMemoryAllocationTracker				m_AllocationTracker;
};

} // end namespace FSCommon
