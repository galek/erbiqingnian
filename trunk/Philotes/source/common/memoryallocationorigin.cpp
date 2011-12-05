//----------------------------------------------------------------------------
// memoryallocationtracker.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "memoryallocationorigin.h"

namespace FSCommon
{


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocationOrigin Constructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocationOrigin::CMemoryAllocationOrigin() :
	Next(NULL),
	m_File(NULL),
	m_Line(0),
	m_TotalExternalAllocationSize(0),
	m_TotalExternalAllocationCount(0),
	m_TotalExternalAllocationSizePeak(0),
	m_TotalExternalAllocationCountPeak(0),
	m_TotalExternalAllocationCounter(0),
	m_SmallestAllocationSize(0xFFFFFFFF),
	m_LargestAllocationSize(0),
	m_FirstAllocationTimestamp(0),
	m_CurrentSizeChangeTimestamp(0),
	m_AverageAllocationSizeOverTime(0),
	m_ResurrectionTimestamp(0),
	m_MaxLifetime(0)
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocationOrigin Destructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocationOrigin::~CMemoryAllocationOrigin()
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::Init()
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::Term()
{

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	OnAlloc
//
// Parameters
//	size : The size in bytes of the new allocation
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::OnAlloc(MEMORY_SIZE size)
{	
	
	// Update Metrics
	//
	++m_TotalExternalAllocationCounter; // never decreases
	
	m_SmallestAllocationSize = MIN<MEMORY_SIZE>(m_SmallestAllocationSize, size);
	m_LargestAllocationSize = MAX<MEMORY_SIZE>(m_LargestAllocationSize, size);	
				
	m_TotalExternalAllocationSize += size;
	m_TotalExternalAllocationSizePeak = MAX<MEMORY_SIZE>(m_TotalExternalAllocationSizePeak, m_TotalExternalAllocationSize);
	
	++m_TotalExternalAllocationCount;
	m_TotalExternalAllocationCountPeak = MAX<MEM_COUNTER>(m_TotalExternalAllocationCountPeak, m_TotalExternalAllocationCount);	
	
	UpdateAverage(size);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	OnFree
//
// Parameters
//	size : The size in bytes of the allocation to free
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::OnFree(MEMORY_SIZE size)
{
	if(m_TotalExternalAllocationCount > 1)
	{
		UpdateAverage(size);
	}
	
	DBG_ASSERT(m_TotalExternalAllocationSize >= size);				
	m_TotalExternalAllocationSize -= size;
	
	DBG_ASSERT(m_TotalExternalAllocationCount);
	m_TotalExternalAllocationCount--;
		
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFileName
//
// Returns
//	A pointer to the file name without the path info
//
/////////////////////////////////////////////////////////////////////////////
const char* CMemoryAllocationOrigin::GetFileName()
{
	const char* file = strrchr(m_File, '\\');
	if(file)
	{
		file += 1;
	}
	else
	{
		file = m_File;
	}
	
	return file;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAverage
//
// Returns
//	The instantaneous average allocation size over time 
//
/////////////////////////////////////////////////////////////////////////////
MEMORY_SIZE CMemoryAllocationOrigin::GetAverage()
{
	float instantaneousAverageSize = 0;
	
	if(m_CurrentSizeChangeTimestamp)
	{
		DWORD now = GetTickCount();		
		float allocationDuration = (float)now - (float)m_FirstAllocationTimestamp; // duration of entire allocation origin
		
		if(allocationDuration)
		{
			float currentAllocationSize = (float)m_TotalExternalAllocationSize;
			float currentAllocationSizeDuration = (float)now - (float)m_CurrentSizeChangeTimestamp;
			
			instantaneousAverageSize = m_AverageAllocationSizeOverTime + ((currentAllocationSizeDuration / allocationDuration) * (currentAllocationSize - m_AverageAllocationSizeOverTime));
		}
		else
		{
			instantaneousAverageSize = (float)m_TotalExternalAllocationSize;
		}
	}
	
	return CEIL(instantaneousAverageSize);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	SetAverage
//
// Remarks
//	Set's the average to the current external allocation size.
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::SetAverage()
{
	m_AverageAllocationSizeOverTime = m_TotalExternalAllocationSize;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetGrowRate
//
// Returns
//	The grow rate percent for the origin between 0 and 100
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryAllocationOrigin::GetGrowRate()
{
	float growRate = ((float)m_TotalExternalAllocationSize - GetAverage()) / m_TotalExternalAllocationSize;
	
	growRate = MAX<float>(0.0f, growRate);
	
	return (unsigned int)(growRate * 100);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAge
//
// Returns
//	The number of seconds that the origin has had an outstanding allocation
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CMemoryAllocationOrigin::GetAge()
{
	return (GetTickCount() - m_FirstAllocationTimestamp) / 1000;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	UpdateAverage
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocationOrigin::UpdateAverage(MEMORY_SIZE size)
{
	DWORD now = GetTickCount();
	
	DWORD allocationDuration = now - m_FirstAllocationTimestamp;
	
	if(allocationDuration == 0 || m_TotalExternalAllocationCounter < 2)
	{
		if(m_TotalExternalAllocationSize)
		{
			m_AverageAllocationSizeOverTime = m_TotalExternalAllocationSize;
		}
		else
		{
			m_AverageAllocationSizeOverTime = size;
		}
	}
	else
	{		
		m_AverageAllocationSizeOverTime = GetAverage();
		DBG_ASSERT(m_AverageAllocationSizeOverTime);
	}	
	
	m_CurrentSizeChangeTimestamp = now;
}

} // end namespace FSCommon