//----------------------------------------------------------------------------
// memoryallocator_i.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memorymacros.h"
#include "memorytypes.h"

namespace FSCommon
{

// Forward declare
//
interface IMemoryAllocatorInternals; // include memoryallocatorinternals_i.h if you need access

// Interface IMemoryAllocator
//
interface IMemoryAllocator
{

	protected:
	
		static const unsigned int cMagicHeader = 0x4c4c4548;
		static const unsigned int cMagicFooter = 0x6c6c6568;

	public:
	
		virtual bool							Init(IMemoryAllocator* fallbackAllocator = NULL) = 0;
		virtual bool							Term() = 0;
		virtual bool							Flush() = 0;
		virtual bool							FreeAll() = 0;	
		virtual MEMORY_ALLOCATOR_TYPE			GetType() = 0;
		
		// Alloc
		//		
		virtual void*							Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator)) = 0;
		
		// Free
		//
		virtual void							Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator)) = 0;
		
		// Size
		//
		virtual MEMORY_SIZE						Size(void* ptr, IMemoryAllocator* fallbackAllocator) = 0;
		
		// Realloc
		//
		virtual void*							Realloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							ReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							AlignedRealloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator)) = 0;
		virtual void*							AlignedReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator)) = 0;
		
		// Internals
		//
		virtual IMemoryAllocatorInternals*		GetInternals() = 0;
};

} // end namespace FSCommon