//----------------------------------------------------------------------------
// memorytypes.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once 

// Typedefs
//
typedef unsigned int MEMORY_SIZE;

#ifdef _WIN64
typedef INT64 MEM_COUNTER;
#else
typedef long MEM_COUNTER;
#endif


namespace FSCommon
{

	// Enumerations
	//
	enum MEMORY_ALLOCATOR_TYPE
	{
		MEMORY_ALLOCATOR_SYSTEM,	// System memory allocator
		MEMORY_ALLOCATOR_CRT,		// CRT allocator
		MEMORY_ALLOCATOR_LFH,		// Windows low fragmentation heap
		MEMORY_ALLOCATOR_POOL,		// pool allocator
		MEMORY_ALLOCATOR_STACK,		// stack allocator
		MEMORY_ALLOCATOR_HEAP		// heap allocator
	};
	
	struct MEMORY_ALLOCATOR_METRICS
	{
		WCHAR			Name[32];
		MEM_COUNTER		InternalSize;
		MEM_COUNTER		InternalFragmentation;
		MEM_COUNTER		ExternalFragmentation;
		MEM_COUNTER		LargestFreeBlock;
		MEM_COUNTER		InternalOverhead;
		MEM_COUNTER		TotalExternalAllocationSize;
		MEM_COUNTER		TotalExternalAllocationSizePeak;
		MEM_COUNTER		TotalExternalAllocationCount;
	};

} // end namespace FSCommon

