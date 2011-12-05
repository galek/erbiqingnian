//----------------------------------------------------------------------------
// memorymanager.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "memorymanager_i.h"
#include "memoryallocator_system.h"
#include "memoryallocator_crt.h"

#if DEBUG_MEMORY_ALLOCATIONS		
#define cUseDebugHeadersAndFooters true
#else
#define cUseDebugHeadersAndFooters false
#endif

namespace FSCommon
{

class CMemoryManager : public IMemoryManager
{

	static const unsigned int							cAllocatorGrowSize = 512;
	
	// The system page size
	//
	static const unsigned int 							cDefaultPageSize = 4096;
	
	// All virtual allocations are done in increments of this granularity, even if the block
	// is of a smaller size, meaning that memory will be wasted
	//
	static const unsigned int 							cDefaultAllocationGranularity = 65536;	
	
	static const unsigned int							cMaxSystemPoolBuckets = 1024;

	public:
	
		CMemoryManager();
		~CMemoryManager();
	
	public: // IMemoryManager
	
		virtual bool									Init();
		virtual void									Term();
		virtual void									Flush(IMemoryAllocator* allocator = NULL);		
		virtual bool									AddAllocator(IMemoryAllocator* allocator);
		virtual void									RemoveAllocator(IMemoryAllocator* allocator);
		virtual IMemoryAllocator*						GetSystemAllocator();					
		virtual IMemoryAllocator*						GetDefaultAllocator(bool perThread = false);
		virtual IMemoryAllocator*						SetDefaultAllocator(IMemoryAllocator* allocator, bool perThread = false);
		virtual IMemoryAllocator*						GetPerThreadAllocator();
		virtual IMemoryAllocator*						SetPerThreadAllocator(IMemoryAllocator* allocator);		
		virtual unsigned int							GetPageSize();
		virtual unsigned int							GetAllocationGranularity();
		
		// Tracking
		//
		virtual unsigned int							GetAllocatorCount();
		virtual IMemoryAllocator*						GetAllocator(unsigned int index);
		virtual void									MemTrace(const char* comment = NULL, IMemoryAllocator* allocator = NULL, bool useLocks = true);		
		
		
	private:
	
		void											MemPrint(char* fmt, ...);		

	private:
	
		CCritSectLite									m_CS;		
	
		bool											m_Initialized;
	
		IMemoryAllocator**								m_Allocators;
		unsigned int									m_AllocatorCountMax;
		unsigned int									m_AllocatorCount;
		
		CMemoryAllocatorSYSTEM<cUseDebugHeadersAndFooters, true, cMaxSystemPoolBuckets>* m_SystemAllocator;
		CMemoryAllocatorCRT<cUseDebugHeadersAndFooters>* m_SystemFallbackAllocator;
		
		IMemoryAllocator*								m_DefaultAllocator;			 // Used across all threads
		static __declspec(thread) IMemoryAllocator*		m_DefaultAllocatorPerThread; // Overrides m_DefaultAllocator for an individual thread
		static __declspec(thread) IMemoryAllocator*		m_PerThreadAllocator;		 // Per-thread storage of allocator for user purposes
		bool											m_PerThreadAllocatorSet;	 // Specifies that the TLS storage has been used
		
		// The system memory page size
		//
		unsigned int									m_PageSize;
		unsigned int									m_AllocationGranularity;	
		
		bool											m_FlushInProgress;			

};


} // end namespace FSCommon