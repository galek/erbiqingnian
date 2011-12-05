//----------------------------------------------------------------------------
// memoryallocator_wrapper.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "memoryallocator_i.h"
#include "memoryallocatorinternals_i.h"

namespace FSCommon
{

// Class CMemoryAllocator
//
class CMemoryAllocatorWRAPPER : public IMemoryAllocator
{

	public:
	
		CMemoryAllocatorWRAPPER();
		
	public: // IMemoryAllocator
	
		virtual bool							Init(IMemoryAllocator* fallbackAllocator);
		virtual bool							Init(IMemoryAllocator* redirectAllocator, IMemoryAllocator* fallbackAllocator = NULL);
		virtual bool							Term();
		virtual bool							Flush();
		virtual bool							FreeAll();	
		virtual MEMORY_ALLOCATOR_TYPE			GetType();
		
		// Alloc
		//		
		virtual void*							Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));		
		virtual void*							AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
				
		// Free
		//
		virtual void							Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator));
		
		// Size
		//
		virtual MEMORY_SIZE						Size(void* ptr, IMemoryAllocator* fallbackAllocator);
				
		// Realloc
		//
		virtual void*							Realloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							ReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator));
		virtual void*							AlignedRealloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));		
		virtual void*							AlignedReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator));
		
		// Internals
		//		
		virtual IMemoryAllocatorInternals*		GetInternals();
					
	private:
	
		IMemoryAllocator*						m_RedirectAllocator;
		IMemoryAllocator*						m_FallbackAllocator;
	

};

} // end namespace FSCommon