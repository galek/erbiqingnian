//----------------------------------------------------------------------------
// memoryallocator_wrapper.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------



#include "memoryallocator_wrapper.h"

namespace FSCommon
{

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CMemoryAllocatorWRAPPER Constructor
//
/////////////////////////////////////////////////////////////////////////////
CMemoryAllocatorWRAPPER::CMemoryAllocatorWRAPPER() :
	m_RedirectAllocator(NULL),
	m_FallbackAllocator(NULL)
{

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
bool CMemoryAllocatorWRAPPER::Init(IMemoryAllocator* fallbackAllocator)
{
	REF(fallbackAllocator);
	
	// This version of init isn't supported
	//
	DBG_ASSERT(0);
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
// 
// Parameters
//	redirectAllocator : The allocator that all IMemoryAllocator calls will
//		be redirected to.
//	fallbackAllocator : [optional] Used to override the redirectAllocator's fallback
//		allocator
//
// Returns
//	True if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocatorWRAPPER::Init(IMemoryAllocator* redirectAllocator, IMemoryAllocator* fallbackAllocator)
{
	m_RedirectAllocator = redirectAllocator;
	m_FallbackAllocator = fallbackAllocator;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Term
//
// Returns
//	True if the method succeeds, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocatorWRAPPER::Term()
{
	m_RedirectAllocator = NULL;
	m_FallbackAllocator = NULL;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Flush
//
// Returns
//	true if after the flush operation the allocator is completely empty, false
//	otherwise
//
// Remarks
//	Called when the allocator should cleanup any unused memory
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocatorWRAPPER::Flush()
{
	return m_RedirectAllocator->Flush();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	FreeAll
//
// Returns
//	True if the method succeeds, false otherwise
//
// Remarks
//	Does not free internal allocations, just external.
//
/////////////////////////////////////////////////////////////////////////////
bool CMemoryAllocatorWRAPPER::FreeAll()
{
	return m_RedirectAllocator->FreeAll();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetType
//
// Returns
//	The enum indicating the type of the allocator (CRT, HEAP, etc)
//
/////////////////////////////////////////////////////////////////////////////
MEMORY_ALLOCATOR_TYPE CMemoryAllocatorWRAPPER::GetType()
{
	return m_RedirectAllocator->GetType();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Alloc
//
// Parameters
//	size : The number of bytes to allocate
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The pointer to the allocated memory or NULL if the stack ran
//	out of memory or there was an error
//
// Remarks
//	The allocation header comes immediately before the returned
//	pointer
//
//	All allocations are aligned with the default alignment size
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::Alloc(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->Alloc(MEMORY_FUNC_PASSFL(size, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AllocZ
//
// Parameters
//	size : The number of bytes to allocate
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	The pointer to the allocated memory or NULL if the stack ran
//	out of memory or there was an error
//
// Remarks
//	Zeros the memory returned.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::AllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->AllocZ(MEMORY_FUNC_PASSFL(size, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedAlloc
//
// Parameters
//	size : The size of the external allocation request for which a pointer will be returned.
//	alignment : The size in bytes for which the returned pointer must be aligned
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Returns
//	A pointer to memory for the allocation request.  NULL if the allocation fails.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::AlignedAlloc(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->AlignedAlloc(MEMORY_FUNC_PASSFL(size, alignment, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedAllocZ
//
// Parameters:
//	size : The number of bytes requested for allocation
//	alignment : The alignment for the allocation in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zeros the memory returned.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::AlignedAllocZ(MEMORY_FUNCARGS(MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->AlignedAllocZ(MEMORY_FUNC_PASSFL(size, alignment, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
// Parameters
//	ptr : The pointer to free
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
void CMemoryAllocatorWRAPPER::Free(MEMORY_FUNCARGS(void* ptr, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->Free(MEMORY_FUNC_PASSFL(ptr, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Size
//
// Parameters
//	ptr : The pointer to return the external allocation size for
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
MEMORY_SIZE CMemoryAllocatorWRAPPER::Size(void* ptr, IMemoryAllocator* fallbackAllocator)
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->Size(ptr, m_FallbackAllocator);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Realloc
//
// Parameters:
//	ptr : The pointer to realloc
//	size : The new size for the allocation
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::Realloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->Realloc(MEMORY_FUNC_PASSFL(ptr, size, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	ReallocZ
//
// Parameters:
//	ptr : The pointer to realloc
//	size : The new size for the allocation
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zero's any additional memory returned if possible.  It is not possible
//	if the header is not stored, since the size is required.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::ReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->ReallocZ(MEMORY_FUNC_PASSFL(ptr, size, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedRealloc
//
// Parameters
//	ptr : A pointer to old memory or NULL
//	size : The required size of the new allocation
//	alignment : The requested alignment in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::AlignedRealloc(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->AlignedRealloc(MEMORY_FUNC_PASSFL(ptr, size, alignment, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AlignedReallocZ
//
// Parameters
//	ptr : A pointer to old memory or NULL
//	size : The required size of the new allocation
//	alignment : The requested alignment in bytes
//  fallbackAllocator : [optional] If specified, overrides the fallback
//		allocator specified in the Init call
//
// Remarks
//	Zero's any additional memory returned if possible.  It is not possible
//	if the header is not stored, since the size is required.
//
/////////////////////////////////////////////////////////////////////////////
void* CMemoryAllocatorWRAPPER::AlignedReallocZ(MEMORY_FUNCARGS(void* ptr, MEMORY_SIZE size, unsigned int alignment, IMemoryAllocator* fallbackAllocator))
{
	REF(fallbackAllocator);
	return m_RedirectAllocator->AlignedReallocZ(MEMORY_FUNC_PASSFL(ptr, size, alignment, m_FallbackAllocator));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetInternals
//
// Returns
//	A pointer to the internals interface
//
/////////////////////////////////////////////////////////////////////////////
IMemoryAllocatorInternals* CMemoryAllocatorWRAPPER::GetInternals()
{
	return m_RedirectAllocator->GetInternals();
}




} // end namespace FSCommon
