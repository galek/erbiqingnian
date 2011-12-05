//----------------------------------------------------------------------------
// memorymanager_i.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "memoryallocator_i.h"

namespace FSCommon
{

// Interface IMemoryManager
//
interface IMemoryManager
{
	public:
	
		static IMemoryManager&			GetInstance();
		virtual bool					Init() = 0;
		virtual void					Term() = 0;	
		virtual void					Flush(IMemoryAllocator* allocator = NULL) = 0;
		virtual bool					AddAllocator(IMemoryAllocator* allocator) = 0;
		virtual void					RemoveAllocator(IMemoryAllocator* allocator) = 0;					
		virtual IMemoryAllocator*		GetSystemAllocator() = 0;
		virtual IMemoryAllocator*		GetDefaultAllocator(bool perThread = false) = 0;
		virtual IMemoryAllocator*		SetDefaultAllocator(IMemoryAllocator* allocator, bool perThread = false) = 0;
		virtual IMemoryAllocator*		GetPerThreadAllocator() = 0;
		virtual IMemoryAllocator*		SetPerThreadAllocator(IMemoryAllocator* allocator) = 0;
		virtual unsigned int			GetPageSize() = 0;
		virtual unsigned int			GetAllocationGranularity() = 0;
		
		// Tracking
		//
		virtual unsigned int			GetAllocatorCount() = 0;
		virtual IMemoryAllocator*		GetAllocator(unsigned int index) = 0;
		virtual void					MemTrace(const char* comment = NULL, IMemoryAllocator* allocator = NULL, bool useLocks = true) = 0;
};



} // end namespace FSCommon