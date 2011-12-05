//----------------------------------------------------------------------------
// memoryallocatorinternals_i.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memorymacros.h"
#include "memorytypes.h"
#include "memoryallocationorigin.h"
#include "sortedarray.h"

namespace FSCommon
{


// Interface IMemoryAllocatorInternals
//
interface IMemoryAllocatorInternals
{

	public:
			
		virtual void*					GetInternalPointer(void* ptr) = 0;
		virtual MEMORY_SIZE				GetInternalSize(void* ptr) = 0;
		virtual void					GetMetrics(MEMORY_ALLOCATOR_METRICS& metrics) = 0;
		virtual void					MemTrace(bool useLocks = true) = 0;
		virtual void					GetFastestGrowingOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins) = 0;
		virtual void					GetRecentlyChangedOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins) = 0;
};

} // end namespace FSCommon