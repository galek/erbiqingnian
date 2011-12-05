//----------------------------------------------------------------------------
// memoryallocationtracker.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "hashtable.h"
#include "pool.h"
#include "memorymacros.h"
#include "memoryallocationorigin.h"
#include "sortedarray.h"

namespace FSCommon
{

// Class CMemoryAllocationOriginTable
//
template<unsigned int TABLE_SIZE>
class CMemoryAllocationOriginTable : public CHashTable<CMemoryAllocationOrigin, TABLE_SIZE, false, unsigned int>
{
	public:
	
		CMemoryAllocationOriginTable() :
			CHashTable<CMemoryAllocationOrigin, TABLE_SIZE, false, unsigned int>()
		{
		
		}
		
	public:
	
		static unsigned int	GetKey(const char* file, unsigned int line)
		{
			return (unsigned int)((DWORD_PTR)file + line);
		}

	protected:

		virtual unsigned int GetKey(CMemoryAllocationOrigin* item)
		{
			return GetKey(item->m_File, item->m_Line);
		}
		
		virtual unsigned int GetHash(unsigned int key)
		{
			return (unsigned int)((key * 46061) % TABLE_SIZE);
		}
};

// Class CMemoryAllocationTracker
//
class CMemoryAllocationTracker
{
	static const unsigned int cAllocationOriginTableSize = 1024;
	static const unsigned int cMaxOriginCount = 1024;


	typedef CPool<CMemoryAllocationOrigin, false> ALLOCATION_ORIGIN_POOL;

	public:
	
		CMemoryAllocationTracker();

	public:

		void							Init();
		void							Term();
		CMemoryAllocationOrigin*		OnAlloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size));
		void							OnFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, CMemoryAllocationOrigin* origin));
		void							MemTrace(bool useLocks);	
		void							GetFastestGrowingOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins);
		void							GetRecentlyChangedOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins);
		DWORD							GetAge();
		unsigned int					GetAliveOriginCount();
		
	private:
	
		void							MemPrint(char* fmt, ...);	
		
	private:

		DWORD							m_InitializationTimestamp;

		ALLOCATION_ORIGIN_POOL			m_AllocationOriginPool;
		BYTE							m_AllocationOriginPoolMemory[cMaxOriginCount * sizeof(CMemoryAllocationOrigin)];

		// A table of allocation origins
		//
		CMemoryAllocationOriginTable<cAllocationOriginTableSize> m_AllocationOriginsAlive; // Those origins that currently have outstanding alloctions
		CMemoryAllocationOriginTable<cAllocationOriginTableSize> m_AllocationOriginsDead;  // Those origins that do not have any outstanding allocations

};

} // end namespace FSCommon
