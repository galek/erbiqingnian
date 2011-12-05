// ---------------------------------------------------------------------------
// FILE:	cpu_usage.cpp
// ---------------------------------------------------------------------------

#include "cpu_usage.h"
#include <pdh.h>
#include <pdhmsg.h>


#pragma comment(lib, "PDH.lib")


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define MAX_RAW_VALUES          20

const char szCounterName[] = "\\Processor(_Total)\\% Processor Time";


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct PDH_COUNTER
{
    HCOUNTER						hCounter;						// handle to the counter - given to use by PDH Library
    int								nNextIndex;						// element to get the next raw value
	int								nOldestIndex;					// element containing the oldest raw value
    int								nRawCount;						// number of elements containing raw values
    PDH_RAW_COUNTER					RawValues[MAX_RAW_VALUES];		// ring buffer to contain raw values
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct CPU_USAGE
{
	struct MEMORYPOOL *				mempool;
	PDH_COUNTER	*					m_pCounter;
	HQUERY							m_hQuery;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
CPU_USAGE * CpuUsageInit(
	struct MEMORYPOOL * mempool)
{
	CPU_USAGE * cpu = (CPU_USAGE *)MALLOCZ(mempool, sizeof(CPU_USAGE));
	ASSERT_RETNULL(cpu);

	cpu->mempool = mempool;
	cpu->m_pCounter = (PDH_COUNTER *)MALLOCZ(mempool, sizeof(PDH_COUNTER));
	ASSERT_RETNULL(cpu->m_pCounter);

	if (PdhOpenQuery(NULL, 1, &cpu->m_hQuery) != ERROR_SUCCESS)
	{
		return NULL;
	}

	PDH_STATUS pdh_status = PdhAddCounter(cpu->m_hQuery, szCounterName, (DWORD_PTR)cpu->m_pCounter, &(cpu->m_pCounter->hCounter));
	// CHB 2006.11.13 - Continue silently on failure.
	// cpu->m_pCounter->hCounter will be NULL, which is tolerable.
	// Returning NULL here, however, leads to a cascade of subsequent failures.
#if 1
	(void)pdh_status;
#else
    if (pdh_status != ERROR_SUCCESS)
	{
		return NULL;
	}
#endif

	return cpu;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CpuUsageFree(
	CPU_USAGE * cpu)
{
	if (!cpu)
	{
		return;
	}
	if (cpu->m_hQuery)
	{
		PdhCloseQuery(cpu->m_hQuery);
	}
	if (cpu->m_pCounter)
	{
		FREE(cpu->mempool, cpu->m_pCounter);
	}
	FREE(cpu->mempool, cpu);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
int CpuUsageGetValue(
	CPU_USAGE * cpu)
{
	if (!(cpu && cpu->m_hQuery))
	{
		return 0;
	}

    PDH_FMT_COUNTERVALUE pdhFormattedValue;
	PdhCollectQueryData(cpu->m_hQuery);

	if (PdhGetFormattedCounterValue(cpu->m_pCounter->hCounter, PDH_FMT_LONG,NULL, &pdhFormattedValue) != ERROR_SUCCESS) 
	{
		return 0;
	}

	return pdhFormattedValue.longValue;
}
