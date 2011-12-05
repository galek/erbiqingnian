//----------------------------------------------------------------------------
// dxC_resource.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_resource.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CHash<DXC_RESOURCE_TEXTURE2D>		gtResTexture2D;
CHash<DXC_RESOURCE_TEXTURECUBE>		gtResTextureCube;
CHash<DXC_RESOURCE_TEXTURE3D>		gtResTexture3D;
CHash<DXC_RESOURCE_VERTEXBUFFER>	gtResVertexBuffer;
CHash<DXC_RESOURCE_INDEXBUFFER>		gtResIndexBuffer;
int gnResourceHashIDNext[ NUM_DXC_RESOURCE_TYPES ] = { 0 };

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

PRESULT dxC_ResourceSystemInit()
{
	HashInit( gtResTextureCube,		NULL, 8 );
	HashInit( gtResTexture3D,		NULL, 1 );
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	HashInit( gtResTexture2D,		g_StaticAllocator, 512 );
	HashInit( gtResVertexBuffer,	g_StaticAllocator, 1024 );
	HashInit( gtResIndexBuffer,		g_StaticAllocator, 1024 );
#else
	HashInit( gtResTexture2D,		NULL, 512 );
	HashInit( gtResVertexBuffer,	NULL, 1024 );
	HashInit( gtResIndexBuffer,		NULL, 1024 );
#endif		

	return S_OK;
}

PRESULT dxC_ResourceSystemDestroy()
{
	HashFree( gtResTexture2D );
	HashFree( gtResTextureCube );
	HashFree( gtResTexture3D );
	HashFree( gtResVertexBuffer );
	HashFree( gtResIndexBuffer );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

template < class T >
static void sDestroyResource( CHash<T> & tHash, DXCRHANDLE hDestroy )
{
	int nIndex = dxC_ResHandleGetIndex( hDestroy );
	T * pRes = HashGet( tHash, nIndex );
	if ( !pRes )
		return;
	pRes->Release();
	HashRemove( tHash, nIndex );
}

template < class T >
static DXCRHANDLE sCreateResource( CHash<T> & tHash, DXCRESOURCE_TYPE eType )
{
	int nIndex = gnResourceHashIDNext[ eType ];
	++gnResourceHashIDNext[ eType ];
	T* pRes = HashAdd( tHash, nIndex );
	ASSERT_RETVAL( pRes, INVALID_DXCRHANDLE );
	pRes->Init( eType );
	DXCRHANDLE hRes = MAKE_DXCRHANDLE( eType, nIndex );
	pRes->SetHandle( hRes );
	pRes->SetState( DXCRSTATE_NOT_LOADED );
	return hRes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_ResourceDestroy( DXCRHANDLE hDestroy )
{
	DXCRESOURCE_TYPE eType = dxC_ResHandleGetType( hDestroy );
	switch ( eType )
	{
	case DXCRTYPE_TEXTURE2D:	sDestroyResource( gtResTexture2D,		hDestroy );				break;
	case DXCRTYPE_TEXTURECUBE:	sDestroyResource( gtResTextureCube,		hDestroy );				break;
	case DXCRTYPE_TEXTURE3D:	sDestroyResource( gtResTexture3D,		hDestroy );				break;
	case DXCRTYPE_VERTEXBUFFER:	sDestroyResource( gtResVertexBuffer,	hDestroy );				break;
	case DXCRTYPE_INDEXBUFFER:	sDestroyResource( gtResIndexBuffer,		hDestroy );				break;
	default:	ASSERTX_RETURN( 0, "Invalid resource type in dxC_ResourceDestroy!" );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DXCRHANDLE dxC_ResourceCreate( DXCRESOURCE_TYPE eType )
{
	switch ( eType )
	{
	case DXCRTYPE_TEXTURE2D:	return sCreateResource( gtResTexture2D,		eType );
	case DXCRTYPE_TEXTURECUBE:	return sCreateResource( gtResTextureCube,	eType );
	case DXCRTYPE_TEXTURE3D:	return sCreateResource( gtResTexture3D,		eType );
	case DXCRTYPE_VERTEXBUFFER:	return sCreateResource( gtResVertexBuffer,	eType );
	case DXCRTYPE_INDEXBUFFER:	return sCreateResource( gtResIndexBuffer,	eType );
	default:	ASSERTX_RETVAL( 0, INVALID_DXCRHANDLE, "Invalid resource type in dxC_ResourceCreate!" );
	}
}