//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_irefcount.h"

//-----------------------------------------------------------------------------

#ifdef _DEBUG
#define DEBUG_ASSERT(e) ASSERT(e)
#else
#define DEBUG_ASSERT(e) ((void)0)
#endif

//-----------------------------------------------------------------------------

HRESULT IRefCount::QueryInterface(REFIID /*riid*/, void * * /*ppvObject*/)
{
	DEBUG_ASSERT(false);
	return E_NOTIMPL;
}

ULONG IRefCount::AddRef(void)
{
	return ::InterlockedIncrement(&nRefs);
}

ULONG IRefCount::Release(void)
{
	DEBUG_ASSERT(nRefs > 0);
	LONG nResult = ::InterlockedDecrement(&nRefs);
	if (nResult == 0)
	{
		delete this;
	}
	return nResult;
}

IRefCount::IRefCount(void)
	: nRefs(0)
{
}

IRefCount::~IRefCount(void)
{
	DEBUG_ASSERT(nRefs == 0);
}
