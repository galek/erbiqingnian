//----------------------------------------------------------------------------
// dx9_state.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "e_pch.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

D3DMATERIAL9				gStubMtrl;
D3DLIGHT9					gStubLight;

DWORD dx9_GetUsage( D3DC_USAGE usage )
{
	switch( usage )
	{
	case D3DC_USAGE_2DTEX_SCRATCH:
	case D3DC_USAGE_2DTEX_STAGING:
	case D3DC_USAGE_2DTEX:
	case D3DC_USAGE_CUBETEX_DYNAMIC:
		return 0x00000000; break;
	case D3DC_USAGE_2DTEX_DEFAULT:
		return D3DUSAGE_DYNAMIC;

#ifdef DX9_MOST_BUFFERS_MANAGED
	case D3DC_USAGE_BUFFER_MUTABLE:
	case D3DC_USAGE_BUFFER_REGULAR:
		return D3DUSAGE_WRITEONLY; break;
#else
	case D3DC_USAGE_BUFFER_REGULAR:
		return D3DUSAGE_WRITEONLY; break;
	case D3DC_USAGE_BUFFER_MUTABLE:
		return D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY; break;
#endif
	case D3DC_USAGE_BUFFER_CONSTANT:
		return D3DUSAGE_WRITEONLY; break;
	case D3DC_USAGE_2DTEX_RT:
		return D3DUSAGE_RENDERTARGET; break;
	case D3DC_USAGE_2DTEX_DS:
	case D3DC_USAGE_2DTEX_SM:
		return D3DUSAGE_DEPTHSTENCIL; break;
	};
	ASSERTX(0, "Unhandled D3DC_USAGE!");
	return 0x00000000;
}
D3DPOOL dx9_GetPool( D3DC_USAGE usage )
{
	switch( usage )
	{
	case D3DC_USAGE_2DTEX_SCRATCH:
		return D3DPOOL_SCRATCH; break;
	case D3DC_USAGE_2DTEX_STAGING:
		return D3DPOOL_SYSTEMMEM; break;
	case D3DC_USAGE_2DTEX:
	case D3DC_USAGE_CUBETEX_DYNAMIC:

#ifdef DX9_MOST_BUFFERS_MANAGED
	case D3DC_USAGE_BUFFER_MUTABLE:
	case D3DC_USAGE_BUFFER_REGULAR:
		return D3DPOOL_MANAGED; break;
#else
	case D3DC_USAGE_BUFFER_REGULAR:
		return D3DPOOL_MANAGED; break;
	case D3DC_USAGE_BUFFER_MUTABLE:
#endif
	case D3DC_USAGE_2DTEX_RT:
	case D3DC_USAGE_2DTEX_DS:
	case D3DC_USAGE_2DTEX_SM:
	case D3DC_USAGE_2DTEX_DEFAULT:	
	case D3DC_USAGE_BUFFER_CONSTANT:
		return D3DPOOL_DEFAULT; break;
	};
	ASSERTX(0, "Unhandled D3DC_USAGE!");
	return (D3DPOOL)0x00000000;
}