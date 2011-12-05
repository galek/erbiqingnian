//----------------------------------------------------------------------------
// memoryallocationorigin.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "hashtable.h"
#include "memorymacros.h"

namespace FSCommon
{


// Class CMemoryAllocationOrigin
//
class CMemoryAllocationOrigin
{		

	struct ALLOCATION_INSTANCE
	{
			// For use in a hash table
			//
			ALLOCATION_INSTANCE*		Next;

			// Pointer to the external allocation memory
			//
			void*						m_Memory;

			// Size of external allocation
			//
			MEMORY_SIZE 				m_MemorySize;
	};


	typedef CHashTable<ALLOCATION_INSTANCE, 512, false> ALLOCATION_INSTANCE_TABLE;

	public:

		CMemoryAllocationOrigin();
		~CMemoryAllocationOrigin();

	public:

		void							Init();
		void							Term();
		void							OnAlloc(MEMORY_SIZE size);
		void							OnFree(MEMORY_SIZE size);
		const char*						GetFileName();
		MEMORY_SIZE						GetAverage();
		void							SetAverage();
		unsigned int					GetGrowRate();
		unsigned int					GetAge();
		void							MemTrace() {};

	private:
	
		void							UpdateAverage(MEMORY_SIZE size);
		
	public:
	
		// Note: These members are packed for 16-byte alignment
	
		// For use in a hash table
		//
		CMemoryAllocationOrigin*		Next;

		const char *					m_File;
		
		MEM_COUNTER						m_TotalExternalAllocationCount;				// current number of outstanding allocations
		MEM_COUNTER						m_TotalExternalAllocationCountPeak;
		MEM_COUNTER						m_TotalExternalAllocationCounter;			// similar to m_TotalExternalAllocationCount, but always goes up
		
		unsigned int					m_Line;
		
		DWORD							m_FirstAllocationTimestamp;					// The timestamp of the first allocation
		DWORD							m_ResurrectionTimestamp;					// The timestamp of when the origin moves from dead to alive
		DWORD							m_MaxLifetime;								// The largest amount of time that the allocation has been alive
		MEMORY_SIZE						m_TotalExternalAllocationSize; 				// current number of bytes
		MEMORY_SIZE						m_TotalExternalAllocationSizePeak;			
		MEMORY_SIZE						m_SmallestAllocationSize;
		MEMORY_SIZE						m_LargestAllocationSize;

		// Grow rate related
		//
		MEMORY_SIZE						m_AverageAllocationSizeOverTime;			// Takes into consideration the duration for which the allocation was at a particular time relative to the entire allocation duration
		DWORD							m_CurrentSizeChangeTimestamp;

#ifndef _WIN64
		BYTE							m_Pad[4];
#endif		

		// A table of allocation instances
		//
		//ALLOCATION_INSTANCE_TABLE		m_Allocations;		
};

} // end namespace FSCommon