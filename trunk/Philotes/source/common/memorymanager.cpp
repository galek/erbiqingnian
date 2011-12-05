//----------------------------------------------------------------------------
// memorymanager.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "memorymanager.h"
#include "memoryallocator.h"
#include "memoryallocator_crt.h"
#include "memoryallocator_system.h"

#if ISVERSION(SERVER_VERSION)
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "memorymanager.cpp.tmh"
#endif

namespace FSCommon
{

CMemoryManager g_MemoryManager;
__declspec(thread) IMemoryAllocator* CMemoryManager::m_DefaultAllocatorPerThread;
__declspec(thread) IMemoryAllocator* CMemoryManager::m_PerThreadAllocator;

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInstance
//
/////////////////////////////////////////////////////////////////////////////
IMemoryManager& IMemoryManager::GetInstance()
{
	return g_MemoryManager;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryManager Constructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryManager::CMemoryManager() :
	m_AllocatorCount(0),
	m_AllocatorCountMax(0),
	m_Allocators(NULL),
	m_Initialized(false),
	m_DefaultAllocator(NULL),
	m_SystemAllocator(NULL),
	m_SystemFallbackAllocator(NULL),
	m_PageSize(cDefaultPageSize),
	m_AllocationGranularity(cDefaultAllocationGranularity),
	m_PerThreadAllocatorSet(false),
	m_FlushInProgress(false)
{
	m_CS.Init();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryManager Destructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryManager::~CMemoryManager()
{
	m_CS.Free();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Returns
//	True if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryManager::Init()
{
	if(m_Initialized == false)
	{
		// Get the default page size and allocation granularity
		//
		SYSTEM_INFO sysinfo;
		structclear(sysinfo);
		GetSystemInfo(&sysinfo);

		if (sysinfo.dwPageSize && (sysinfo.dwPageSize & sysinfo.dwPageSize - 1) == 0)
		{
			m_PageSize = sysinfo.dwPageSize;	
		}

		if (sysinfo.dwAllocationGranularity && (sysinfo.dwAllocationGranularity & sysinfo.dwAllocationGranularity - 1) == 0)
		{
			m_AllocationGranularity = sysinfo.dwAllocationGranularity;	
		}

		// Create the perf instance for all games
		//
		CMemoryAllocator::g_PerfInstance = PERF_GET_INSTANCE(CMemoryAllocator, L"Games - All");		
	
		// Create a CRT allocator that will be the fall back for the system memory allocator
		//
		m_SystemFallbackAllocator = new CMemoryAllocatorCRT<cUseDebugHeadersAndFooters>(L"SystemCRT");
		if(m_SystemFallbackAllocator->Init(NULL) == false)
		{
			delete m_SystemFallbackAllocator;
			m_SystemFallbackAllocator = NULL;
						
			return false;
		}
		
		// Create a system memory allocator
		//
		m_SystemAllocator = new CMemoryAllocatorSYSTEM<cUseDebugHeadersAndFooters, true, cMaxSystemPoolBuckets>(L"System");
		if(m_SystemAllocator->Init(m_SystemFallbackAllocator, 260 * KILO, false, true) == false)
		{
			delete m_SystemAllocator;
			delete m_SystemFallbackAllocator;
			
			Term();
			return false;			
		}
		
		// Set the system memory allocator as the default for now
		//
		m_DefaultAllocator = m_SystemAllocator;

		if(AddAllocator(m_SystemFallbackAllocator) == false)
		{
			delete m_SystemAllocator;
			delete m_SystemFallbackAllocator;
				
			Term();		
			return false;
		}
		
		if(AddAllocator(m_SystemAllocator) == false)
		{
			delete m_SystemAllocator;
			delete m_SystemFallbackAllocator;
			
			Term();		
			return false;
		}
		
		
		m_Initialized = true;
		
		return true;
	}
	
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryManager::Term()
{
	ASSERT_RETURN(m_SystemAllocator);
	m_SystemAllocator->Free(MEMORY_FUNC_FILELINE(m_Allocators, NULL));
	m_Allocators = NULL;
	m_AllocatorCountMax = 0;
	m_AllocatorCount = 0;

	m_SystemAllocator->Term();
	delete m_SystemAllocator;
	m_SystemAllocator = NULL;
	
	m_SystemFallbackAllocator->Term();
	delete m_SystemFallbackAllocator;	
	m_SystemFallbackAllocator = NULL;
	
	m_DefaultAllocator = NULL;
		
	// Release the perf instance for all games
	//
	PERF_FREE_INSTANCE(CMemoryAllocator::g_PerfInstance);
	CMemoryAllocator::g_PerfInstance = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Flush
//
// Parameters
//	allocator : [optional] If specified will only flush that allocator,
//		otherwise will flush all allocators.
//
// Remarks
//	The flush operation may cause allocators to be removed from the list,
//	locks cannot be taken here.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryManager::Flush(IMemoryAllocator* allocator)
{	
	CSLAutoLock lock(&m_CS);

	m_FlushInProgress = true;
	
	if(allocator)
	{
		allocator->Flush();	
	}
	else
	{
		// Flush all allocators
		//
		for(unsigned int i = 0; i < m_AllocatorCount; ++i)
		{
			m_Allocators[i]->Flush();
		}
	}
	
	m_FlushInProgress = false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AddAllocator
//
// Parameters
//	allocator : The pointer to the allocator to add
//
// Returns
//	True if successful, false otherwise.  Can return false if the 
//	max allocator count has been reached.
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryManager::AddAllocator(IMemoryAllocator* allocator)
{
	CSLAutoLock lock(&m_CS);

	// Expand allocator array if necessary
	//
	if(m_AllocatorCount >= m_AllocatorCountMax)
	{
		m_AllocatorCountMax += cAllocatorGrowSize;
		m_Allocators = reinterpret_cast<IMemoryAllocator**>(m_SystemAllocator->Realloc(MEMORY_FUNC_FILELINE(m_Allocators, m_AllocatorCountMax * sizeof(IMemoryAllocator*), NULL)));
	}

	if(m_Allocators)
	{
		m_Allocators[m_AllocatorCount++] = allocator;
		return true;
	}
	else
	{
		return false;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RemoveAllocator
//
// Parameters 
//	allocator : The pointer to the allocator to remove
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryManager::RemoveAllocator(IMemoryAllocator* allocator)
{
	CSLAutoLock lock(m_FlushInProgress ? NULL : &m_CS); // Don't lock if a flush is in progress since the lock is already taken

	for(unsigned int i = 0; i < m_AllocatorCount; ++i)
	{
		if(m_Allocators[i] == allocator)
		{
			if(i + 1 < m_AllocatorCount)
			{
				memcpy(&m_Allocators[i], &m_Allocators[i + 1], (m_AllocatorCount - i - 1) * sizeof(IMemoryAllocator*));				
			}
			
			--m_AllocatorCount;
			
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetSystemAllocator
//
// Returns
//	A pointer to the system memory allocator
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::GetSystemAllocator()
{
	return m_SystemAllocator;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetDefaultAllocator
//
// Parameters
//	perThread : [optional] If true, the per thread default allocator 
//		will be returned instead of the process default allocator
//
// Returns
//	The current default allocator
//
// Remarks
//	When no allocator is specified in an allocation or free call, the 
//	per-thread allocator is first checked.  If valid, it is used, otherwise
//	the non-per-thread default allocator is used.
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::GetDefaultAllocator(bool perThread)
{
	if(perThread == false)
	{
		return m_DefaultAllocator;
	}
	else if(m_PerThreadAllocatorSet)
	{
		return m_DefaultAllocatorPerThread;
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetDefaultAllocator
//
// Parameters
//	allocator : The allocator that will become the default allocator
//	perThread : [optional] If true, will set the per thread default allocator
//		instead of the process default allocator.
//
// Returns
//	The previous default allocator
//
// Remarks
//	When no allocator is specified in an allocation or free call, the 
//	per-thread allocator is first checked.  If valid, it is used, otherwise
//	the non-per-thread default allocator is used.
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::SetDefaultAllocator(IMemoryAllocator* allocator, bool perThread)
{
	CSLAutoLock lock(&m_CS);

	IMemoryAllocator* previousAllocator = NULL;
	
	if(perThread == false)
	{
		previousAllocator = m_DefaultAllocator;
		m_DefaultAllocator = allocator;		
	}
	else
	{
		previousAllocator = m_DefaultAllocatorPerThread;
		m_DefaultAllocatorPerThread = allocator;
		m_PerThreadAllocatorSet = true;
	}	
	
	return previousAllocator;
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetPerThreadAllocator
//
// Returns
//	The allocator stored in the per thread storage if it exists
//
// Remarks
//	This is a convenience method to store an allocator pointer in TLS.
//	System can call this method to retrieve that pointer, however it is
//	not used internally.  The caller can call SetDefaultAllocator on the
//	returned pointer or use it directly.
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::GetPerThreadAllocator()
{
	return m_PerThreadAllocator;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetPerThreadAllocator
//
// Parameters
//	allocator : The allocator that will stored in the per thread storage
//
// Returns
//	The previous per thread allocator.
//
// Remarks
//	This is a convenience method to store an allocator pointer in TLS.
//	System can call this method to retrieve that pointer, however it is
//	not used internally.  The caller can call SetDefaultAllocator on the
//	returned pointer or use it directly.
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::SetPerThreadAllocator(IMemoryAllocator* allocator)
{
	CSLAutoLock lock(&m_CS);

	IMemoryAllocator* previousAllocator = NULL;
	
	previousAllocator = m_PerThreadAllocator;
	m_PerThreadAllocator = allocator;	
	
	m_PerThreadAllocatorSet = true;	
	
	return previousAllocator;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetPageSize
//
// Returns
//	The default system page size (probably 4K)
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryManager::GetPageSize()
{
	return m_PageSize;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAllocationGranularity
//
// Returns
//	The default system allocation granularity (probably 65K)
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryManager::GetAllocationGranularity()
{
	return m_AllocationGranularity;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAllocatorCount
//
// Returns
//	The number of allocators currently registered through AddAllocator
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryManager::GetAllocatorCount()
{
	return m_AllocatorCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAllocator
//
// Parameters
//	index : The index of the allocator to return
//
// Returns
//	The allocator for the index
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocator* CMemoryManager::GetAllocator(unsigned int index)
{
	CSLAutoLock lock(&m_CS);

	if(index < m_AllocatorCount)
	{
		return m_Allocators[index];
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemTrace
//
// Parameters
//	comment : [optional] A pointer to an optional comment that will
//		be added to the memtrace
//	allocator : [optional] If specified will only trace that allocator,
//		otherwise will trace all allocators.
//	useLocks : [optional] If false, no locking will be used when doing the
//		memory trace.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryManager::MemTrace(const char* comment, IMemoryAllocator* allocator, bool useLocks)
{
	REF(comment);

	CSLAutoLock lock(useLocks && m_FlushInProgress == false ? &m_CS : NULL);

	MemPrint("<memtrace comment='%s'>\n", comment);

	if(allocator)
	{
		allocator->GetInternals()->MemTrace(useLocks);	
	}
	else
	{
		// Trace all allocators
		//
		for(unsigned int i = 0; i < m_AllocatorCount; ++i)
		{
			m_Allocators[i]->GetInternals()->MemTrace(useLocks);
		}
	}	
	
	MemPrint("</memtrace>\n");	
	LogFlush(MEM_TRACE_LOG);	
	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	MemPrint
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryManager::MemPrint(char* fmt, ...)
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
