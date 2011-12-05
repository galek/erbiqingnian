//----------------------------------------------------------------------------
// memoryallocator_lfh.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once

#include "memoryallocator.h"

#pragma warning(push)
#pragma warning(disable:4127)

namespace FSCommon
{

// Class CMemoryAllocatorLFH
//
template<bool USE_HEADER_AND_FOOTER>
class CMemoryAllocatorLFH : public CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>
{

	public:
		
		CMemoryAllocatorLFH(const WCHAR* name = NULL, unsigned int defaultAlignment = cDefaultAlignment);
		
	public: // IMemoryAllocator
			
		virtual bool			Init(IMemoryAllocator* fallbackAllocator);			
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorLFH Constructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
CMemoryAllocatorLFH<USE_HEADER_AND_FOOTER>::CMemoryAllocatorLFH(const WCHAR* name, unsigned int defaultAlignment) :
	CMemoryAllocatorCRT<USE_HEADER_AND_FOOTER>(name, defaultAlignment)
{
	m_AllocatorType = MEMORY_ALLOCATOR_LFH;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
/////////////////////////////////////////////////////////////////////////////
template<bool USE_HEADER_AND_FOOTER>
bool CMemoryAllocatorLFH<USE_HEADER_AND_FOOTER>::Init(IMemoryAllocator* fallbackAllocator)
{
	if(m_Initialized == false)
	{
		if(__super::Init(fallbackAllocator))
		{		
			ULONG lowfrag = 2;
			HeapSetInformation(m_Heap, HeapCompatibilityInformation, &lowfrag, sizeof(lowfrag));	
		}
		else
		{
			return false;
		}
	}
	
	return true;
}

} // end namespace FSCommon

#pragma warning(pop)
