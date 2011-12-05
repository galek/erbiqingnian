//----------------------------------------------------------------------------
// memoryallocationtracker.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "memoryallocationtracker.h"
#include "memorymanager_i.h"
#include "sortedarray.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memoryallocationtracker.cpp.tmh"
#endif

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocationTracker Constructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocationTracker::CMemoryAllocationTracker() :	
	m_InitializationTimestamp(0)
{	
	ASSERT(m_AllocationOriginPool.Init(&m_AllocationOriginPoolMemory, sizeof(m_AllocationOriginPoolMemory)) == cMaxOriginCount);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::Init()
{
	m_InitializationTimestamp = GetTickCount();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::Term()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	OnAlloc
//
// Parameters
//	ptr : The external allocation pointer that needs to be tracked
//	size : The size of the external allocation
//
// Returns
//	A pointer to the allocation origin node that should be placed in the allocation
//	header.  Returns NULL if there was an error or out of memory
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocationOrigin* CMemoryAllocationTracker::OnAlloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size))
{
 	MEMORY_NDEBUG_REF(ptr);
	MEMORY_NDEBUG_REF(size);
	REF(ptr);

	CMemoryAllocationOrigin* origin = NULL;
	
#if DEBUG_MEMORY_ALLOCATIONS

	m_AllocationOriginsAlive.Lock();
	m_AllocationOriginsDead.Lock();
	
	// See if there is an alive origin already
	//
 	unsigned int key = m_AllocationOriginsAlive.GetKey(file, line);
	origin = m_AllocationOriginsAlive.Find(key);

	// If there isn't, see if there is a dead one we can resurrect
	//
	if(origin == NULL)
	{
		key = m_AllocationOriginsDead.GetKey(file, line);
		origin = m_AllocationOriginsDead.Get(key);
		
		if(origin)
		{
			m_AllocationOriginsAlive.Put(true, origin);
			
			origin->m_ResurrectionTimestamp = GetTickCount();
		}		
	}

	// Create a new node if necessary
	//
	if(origin == NULL)
	{
		origin = m_AllocationOriginPool.Get();
		ASSERTV(origin, "allocation tracker out of origins. - Didier")

		if(origin == NULL)
		{
			m_AllocationOriginsAlive.Unlock();
			m_AllocationOriginsDead.Unlock();
			return NULL;
		}

		// Use placement new since the pool only allocates memory
		//
		origin = new(origin) CMemoryAllocationOrigin();
		origin->Init();
		
		origin->m_File = file;
		origin->m_Line = line;
		
		origin->m_FirstAllocationTimestamp = GetTickCount();
		origin->m_ResurrectionTimestamp = GetTickCount();
		
		m_AllocationOriginsAlive.Put(true, origin);
	}

	origin->OnAlloc(size);	
	
	m_AllocationOriginsAlive.Unlock();
	m_AllocationOriginsDead.Unlock();

#endif

	return origin;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	OnFree
//
// Parameters
//	ptr : A pointer to the external allocation that is being freed.
// 	size : The external allocation size of ptr
//	origin : A pointer to the allocation origin node for the allocation that is being freed
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::OnFree(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, CMemoryAllocationOrigin* origin))
{
	MEMORY_DEBUG_REF(file);
	MEMORY_DEBUG_REF(line);
 	REF(ptr);
 	MEMORY_NDEBUG_REF(size);
	MEMORY_NDEBUG_REF(origin);

#if DEBUG_MEMORY_ALLOCATIONS

	if(origin)
	{
		// Validate the origin pointer
		//
		if((BYTE*)origin < m_AllocationOriginPoolMemory || (BYTE*)origin > m_AllocationOriginPoolMemory + sizeof(m_AllocationOriginPoolMemory))
		{
			return;
		}
	
		m_AllocationOriginsAlive.Lock();
		m_AllocationOriginsDead.Lock();
	
		origin->OnFree(size);

		// Remove the origin if it was the last one
		//
		if(origin->m_TotalExternalAllocationCount <= 0)
		{
			DBG_ASSERT(origin->m_TotalExternalAllocationSize == 0);
		
			unsigned int key = m_AllocationOriginsAlive.GetKey(origin->m_File, origin->m_Line);
			m_AllocationOriginsAlive.Get(key); // This removes the item from the hash table

			// Put the origin on the dead list
			//
			m_AllocationOriginsDead.Put(true, origin);	
			
			origin->m_MaxLifetime = MAX<DWORD>(origin->m_MaxLifetime, GetTickCount() - origin->m_ResurrectionTimestamp);		
		}
		
		m_AllocationOriginsAlive.Unlock();
		m_AllocationOriginsDead.Unlock();
	}

#endif


}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTrace
//
// Remarks
//	Dumps out all the allocation origin information
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::MemTrace(bool useLocks)
{
	char file[64];
	unsigned int line = 0;
	char totalExternalAllocationSize[16];
	MEMORY_SIZE totalExternalAllocationSizeValue = 0;
	char totalExternalAllocationSizePeak[16];
	MEMORY_SIZE totalExternalAllocationSizePeakValue = 0;
	char totalExternalAllocationCount[16];
	MEM_COUNTER totalExternalAllocationCountValue = 0;
	char totalExternalAllocationCountPeak[16];
	MEM_COUNTER totalExternalAllocationCountPeakValue = 0;
	char totalExternalAllocations[16];
	MEM_COUNTER totalExternalAllocationsValue = 0;
	char smallestAllocation[16];
	MEMORY_SIZE smallestAllocationValue = 0;
	char largestAllocation[16];
	MEMORY_SIZE largestAllocationValue = 0;
	DWORD allocationBirth = 0;
	char birth[16];
	DWORD allocationAge = 0;
	char age[16];
	DWORD lastSizeChange = 0;
	char lastChange[16];
	DWORD instantaneousAverage = 0;
	char averageAllocationSizeOverTime[16];
	char growRate[16];
	unsigned int growRateValue = 0;
	DWORD maxLifetimeValue = 0;
	char maxLifetime[16];
	
	DWORD now = GetTickCount();
	
	// Alive origins
	//
	unsigned int originCount = m_AllocationOriginsAlive.GetCount();
	
	MemPrint("\t\t\t<aliveOrigins>\n");
	
	for(unsigned int i = 0; i < originCount; ++i)
	{
		CMemoryAllocationOrigin* origin = m_AllocationOriginsAlive[i];
		if(origin)
		{
			if(useLocks)
			{
				m_AllocationOriginsAlive.Lock();
			}
			
			PStrCopy(file, arrsize(file), origin->GetFileName(), PStrLen(origin->GetFileName()));
			line = origin->m_Line;
			totalExternalAllocationSizeValue = origin->m_TotalExternalAllocationSize;
			totalExternalAllocationSizePeakValue = origin->m_TotalExternalAllocationSizePeak;
			totalExternalAllocationCountValue = origin->m_TotalExternalAllocationCount;
			totalExternalAllocationCountPeakValue = origin->m_TotalExternalAllocationCountPeak;
			totalExternalAllocationsValue = origin->m_TotalExternalAllocationCounter;
			smallestAllocationValue = origin->m_SmallestAllocationSize;
			largestAllocationValue = origin->m_LargestAllocationSize;
			allocationBirth = (origin->m_FirstAllocationTimestamp - m_InitializationTimestamp) / 1000;
			allocationAge = (now - origin->m_FirstAllocationTimestamp) / 1000;
			lastSizeChange = (now - origin->m_CurrentSizeChangeTimestamp) / 1000;
			instantaneousAverage = origin->GetAverage();
			growRateValue = origin->GetGrowRate();
			maxLifetimeValue = origin->m_MaxLifetime / 1000;

			if(useLocks)
			{
				m_AllocationOriginsAlive.Unlock();
			}
			
			sprintf_s(totalExternalAllocationSize, "%u", totalExternalAllocationSizeValue);
			PStrGroupThousands(totalExternalAllocationSize, arrsize(totalExternalAllocationSize));
			
			sprintf_s(totalExternalAllocationSizePeak, "%u", totalExternalAllocationSizePeakValue);
			PStrGroupThousands(totalExternalAllocationSizePeak, arrsize(totalExternalAllocationSizePeak));
			
			sprintf_s(totalExternalAllocationCount, "%u", (unsigned int)totalExternalAllocationCountValue);
			PStrGroupThousands(totalExternalAllocationCount, arrsize(totalExternalAllocationCount));

			sprintf_s(totalExternalAllocationCountPeak, "%u", (unsigned int)totalExternalAllocationCountPeakValue);
			PStrGroupThousands(totalExternalAllocationCountPeak, arrsize(totalExternalAllocationCountPeak));

			sprintf_s(totalExternalAllocations, "%u", (unsigned int)totalExternalAllocationsValue);
			PStrGroupThousands(totalExternalAllocations, arrsize(totalExternalAllocations));

			sprintf_s(smallestAllocation, "%u", smallestAllocationValue);
			PStrGroupThousands(smallestAllocation, arrsize(smallestAllocation));

			sprintf_s(largestAllocation, "%u", largestAllocationValue);
			PStrGroupThousands(largestAllocation, arrsize(largestAllocation));
			
			sprintf_s(birth, "%u", allocationBirth);
			PStrGroupThousands(birth, arrsize(birth));
			
			sprintf_s(age, "%u", allocationAge);
			PStrGroupThousands(age, arrsize(age));			
			
			sprintf_s(lastChange, "%u", lastSizeChange);
			PStrGroupThousands(lastChange, arrsize(lastChange));			
			
			sprintf_s(averageAllocationSizeOverTime, "%u", instantaneousAverage);
			PStrGroupThousands(averageAllocationSizeOverTime, arrsize(averageAllocationSizeOverTime));			

			sprintf_s(growRate, "%u", growRateValue);
			
			sprintf_s(maxLifetime, "%u", maxLifetimeValue);
			
			MemPrint("\t\t\t\t<origin File='%s' Line='%u' ExternalSize='%u' ExternalSizePeak='%u' Count='%u' CountPeak='%u' TotalAllocations='%u' MinAlloc='%u' MaxAlloc='%u' Birth='%02d:%02d:%02d' Age='%02d:%02d:%02d' LastChange='%02d:%02d:%02d' AverageSize='%u' GrowRate='%u' MaxLifetime='%02d:%02d:%02d'/>\n",
				file, line, 
				totalExternalAllocationSizeValue, totalExternalAllocationSizePeakValue, 
				(unsigned int)totalExternalAllocationCountValue, (unsigned int)totalExternalAllocationCountPeakValue,
				(unsigned int)totalExternalAllocationsValue, 
				smallestAllocationValue, 
				largestAllocationValue, 
				allocationBirth / 3600, (allocationBirth / 60) % 60, allocationBirth % 60, 
				allocationAge / 3600, (allocationAge / 60) % 60, allocationAge % 60, 
				lastSizeChange / 3600, (lastSizeChange / 60) % 60, lastSizeChange % 60, 
				instantaneousAverage,
				growRateValue,
				maxLifetimeValue / 3600, (maxLifetimeValue / 60) % 60, maxLifetimeValue % 60);
		}
	}
	
	MemPrint("\t\t\t</aliveOrigins>\n");
	
	// Dead origins
	//
	originCount = m_AllocationOriginsDead.GetCount();
	
	MemPrint("\t\t\t<deadOrigins>\n");
	
	for(unsigned int i = 0; i < originCount; ++i)
	{
		CMemoryAllocationOrigin* origin = m_AllocationOriginsDead[i];
		if(origin)
		{
			m_AllocationOriginsDead.Lock();
			
			PStrCopy(file, arrsize(file), origin->GetFileName(), PStrLen(origin->GetFileName()));
			line = origin->m_Line;
			totalExternalAllocationSizeValue = origin->m_TotalExternalAllocationSize;
			totalExternalAllocationSizePeakValue = origin->m_TotalExternalAllocationSizePeak;
			totalExternalAllocationCountValue = origin->m_TotalExternalAllocationCount;
			totalExternalAllocationCountPeakValue = origin->m_TotalExternalAllocationCountPeak;
			totalExternalAllocationsValue = origin->m_TotalExternalAllocationCounter;
			smallestAllocationValue = origin->m_SmallestAllocationSize;
			largestAllocationValue = origin->m_LargestAllocationSize;
			allocationBirth = (origin->m_FirstAllocationTimestamp - m_InitializationTimestamp) / 1000;
			allocationAge = (now - origin->m_FirstAllocationTimestamp) / 1000;
			lastSizeChange = (now - origin->m_CurrentSizeChangeTimestamp) / 1000;
			instantaneousAverage = origin->GetAverage();
			growRateValue = origin->GetGrowRate();
			maxLifetimeValue = origin->m_MaxLifetime / 1000;			

			m_AllocationOriginsDead.Unlock();
			
			sprintf_s(totalExternalAllocationSize, "%u", totalExternalAllocationSizeValue);
			PStrGroupThousands(totalExternalAllocationSize, arrsize(totalExternalAllocationSize));
			
			sprintf_s(totalExternalAllocationSizePeak, "%u", totalExternalAllocationSizePeakValue);
			PStrGroupThousands(totalExternalAllocationSizePeak, arrsize(totalExternalAllocationSizePeak));
			
			sprintf_s(totalExternalAllocationCount, "%u", (unsigned int)totalExternalAllocationCountValue);
			PStrGroupThousands(totalExternalAllocationCount, arrsize(totalExternalAllocationCount));

			sprintf_s(totalExternalAllocationCountPeak, "%u", (unsigned int)totalExternalAllocationCountPeakValue);
			PStrGroupThousands(totalExternalAllocationCountPeak, arrsize(totalExternalAllocationCountPeak));

			sprintf_s(totalExternalAllocations, "%u", (unsigned int)totalExternalAllocationsValue);
			PStrGroupThousands(totalExternalAllocations, arrsize(totalExternalAllocations));

			sprintf_s(smallestAllocation, "%u", smallestAllocationValue);
			PStrGroupThousands(smallestAllocation, arrsize(smallestAllocation));

			sprintf_s(largestAllocation, "%u", largestAllocationValue);
			PStrGroupThousands(largestAllocation, arrsize(largestAllocation));
			
			sprintf_s(birth, "%u", allocationBirth);
			PStrGroupThousands(birth, arrsize(birth));
			
			sprintf_s(age, "%u", allocationAge);
			PStrGroupThousands(age, arrsize(age));			
			
			sprintf_s(lastChange, "%u", lastSizeChange);
			PStrGroupThousands(lastChange, arrsize(lastChange));			
			
			sprintf_s(averageAllocationSizeOverTime, "%u", instantaneousAverage);
			PStrGroupThousands(averageAllocationSizeOverTime, arrsize(averageAllocationSizeOverTime));			

			sprintf_s(growRate, "%u", growRateValue);

			sprintf_s(maxLifetime, "%u", maxLifetimeValue);			
			
			MemPrint("\t\t\t\t<origin File='%s' Line='%u' ExternalSize='%u' ExternalSizePeak='%u' Count='%u' CountPeak='%u' TotalAllocations='%u' MinAlloc='%u' MaxAlloc='%u' Birth='%02d:%02d:%02d' Age='%02d:%02d:%02d' LastChange='%02d:%02d:%02d' AverageSize='%u' GrowRate='%u' MaxLifetime='%02d:%02d:%02d'/>\n",
				file, line, 
				totalExternalAllocationSizeValue, totalExternalAllocationSizePeakValue, 
				(unsigned int)totalExternalAllocationCountValue, (unsigned int)totalExternalAllocationCountPeakValue,
				(unsigned int)totalExternalAllocationsValue, 
				smallestAllocationValue, 
				largestAllocationValue, 
				allocationBirth / 3600, (allocationBirth / 60) % 60, allocationBirth % 60, 
				allocationAge / 3600, (allocationAge / 60) % 60, allocationAge % 60, 
				lastSizeChange / 3600, (lastSizeChange / 60) % 60, lastSizeChange % 60, 
				instantaneousAverage,
				growRateValue,
				maxLifetimeValue / 3600, (maxLifetimeValue / 60) % 60, maxLifetimeValue % 60);
		}
	}
	
	MemPrint("\t\t\t</deadOrigins>\n");	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFastestGrowingOrigins
//
// Parameters
//	origins : [out] Will have the fastest growing origins added to it.
//
// Remarks
//	Set the origins array to be not grow-able to limit the number of origins
//	returned.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::GetFastestGrowingOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins)
{	
	unsigned int originCount = m_AllocationOriginsAlive.GetCount();
	for(unsigned int i = 0; i < originCount; ++i)
	{
		CMemoryAllocationOrigin* origin = m_AllocationOriginsAlive[i];
		unsigned int growRate = origin->GetGrowRate();
		unsigned int age = origin->GetAge();
		
		if(growRate && age)
		{
			origins.Insert(growRate, origin);
		}
	}
		
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetRecentlyChangedOrigins
//
// Parameters
//	origins : [out] Will have the recently changed origins added to it.
//
// Remarks
//	Set the origins array to be not grow-able to limit the number of origins
//	returned.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::GetRecentlyChangedOrigins(CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>& origins)
{	
	DWORD now = GetTickCount();

	unsigned int originCount = m_AllocationOriginsAlive.GetCount();
	for(unsigned int i = 0; i < originCount; ++i)
	{
		CMemoryAllocationOrigin* origin = m_AllocationOriginsAlive[i];
		
		unsigned int lastChange = now - origin->m_CurrentSizeChangeTimestamp;		
		unsigned int age = origin->GetAge();
		
		if(age)
		{
			origins.Insert(lastChange, origin);
		}
	}
		
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAge
//
// Returns
//	The age of the allocation tracker and allocator in seconds.
//
/////////////////////////////////////////////////////////////////////////////
DWORD CMemoryAllocationTracker::GetAge()
{
	return (GetTickCount() - m_InitializationTimestamp)	/ 1000;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAliveOriginCount
//
// Returns
//	The number of current alive origins
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryAllocationTracker::GetAliveOriginCount()
{
	return m_AllocationOriginsAlive.GetCount();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemPrint
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationTracker::MemPrint(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

#if ISVERSION(SERVER_VERSION)
	char output[512];
	PStrVprintf(output, ARRAYSIZE(output), fmt, args);
	TraceGameInfo("%s", output);
#else
	LogMessageV(MEM_TRACE_LOG, fmt, args);
#endif
}


} // end namespace FSCommon

