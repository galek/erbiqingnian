//----------------------------------------------------------------------------
// dx9_profile.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_buffer.h"
#include "dxC_texture.h"
#include "dxC_state.h"
#include "debug.h"

#include "dxC_profile.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9
static const int cnMaxPool = D3DPOOL_SCRATCH;
static const int nVerifyMaxPool = max( D3DPOOL_DEFAULT, max( D3DPOOL_MANAGED, max( D3DPOOL_SYSTEMMEM, D3DPOOL_SCRATCH ) ) );
C_ASSERT( cnMaxPool == nVerifyMaxPool );
#elif defined(ENGINE_TARGET_DX10)
static const int cnMaxPool = 0;		// all DX10 goes into the first pool
#endif

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

PRESULT dxC_EngineMemoryGetPool( int nMemoryPool, ENGINE_MEMORY & tMemory, DWORD* pdwBytesTotal /*= NULL*/ )
{
	// iterate textures, vbuffers and ibuffers

	tMemory.Zero();

	// TEXTURES		-- 2D Textures
	for ( D3D_TEXTURE * pTexture = dxC_TextureGetFirst();
		pTexture;
		pTexture = dxC_TextureGetNext( pTexture ) )
	{
		if ( ! e_TextureIsLoaded( pTexture->nId ) )
			continue;

		int nPool = 0; // all DX10 goes into the first pool
		DX9_BLOCK ( nPool = dx9_GetPool( pTexture->tUsage ); );

		if ( nMemoryPool == nPool )
		{
			DWORD dwBytes = 0;
			V_CONTINUE( e_TextureGetSizeInMemory( pTexture->nId, &dwBytes ) );
			tMemory.dwTextureBytes += dwBytes;
			tMemory.dwTextures++;

			if ( pdwBytesTotal )
				*pdwBytesTotal += dwBytes;
		}
	}

	// TEXTURES		-- Cube Textures
	//for ( D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGetFirst();
	//	pCubeTexture;
	//	pCubeTexture = dx9_CubeTextureGetNext( pCubeTexture ) )
	//{
	//	if ( ! e_CubeTextureIsLoaded( pCubeTexture->nId ) )
	//		continue;

	//	int nPool = 0; // all DX10 goes into the first pool
	//	DX9_BLOCK ( nPool = pCubeTexture->nD3DPool; );

	//	DWORD dwBytes = 0;
	//	V_CONTINUE( e_CubeTextureGetSizeInMemory( pCubeTexture->nId, &dwBytes ) );
	//	tMemory[ nPool ].dwTextureBytes += dwBytes;
	//	tMemory[ nPool ].dwTextures++;
	//}

	// VBUFFERS
	for ( D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGetFirst();
		pVB;
		pVB = dxC_VertexBufferGetNext( pVB ) )
	{
		if ( ! dxC_VertexBufferD3DExists( pVB->nId ) )
			continue;

		int nPool = 0; // all DX10 goes into the first pool
		DX9_BLOCK ( nPool = dx9_GetPool( pVB->tUsage ); );

		if ( nMemoryPool == nPool )
		{
			DWORD dwBytes = 0;
			V_CONTINUE( dxC_VertexBufferGetSizeInMemory( pVB->nId, &dwBytes ) );
			tMemory.dwVBufferBytes += dwBytes;
			tMemory.dwVBuffers++;

			if ( pdwBytesTotal )
				*pdwBytesTotal += dwBytes;
		}
	}

	// IBUFFERS
	for ( D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGetFirst();
		pIB;
		pIB = dxC_IndexBufferGetNext( pIB ) )
	{
		if ( ! dxC_IndexBufferD3DExists( pIB->nId ) )
			continue;

		int nPool = 0; // all DX10 goes into the first pool
		DX9_BLOCK ( nPool = dx9_GetPool( pIB->tUsage ); );

		if ( nMemoryPool == nPool )
		{
			DWORD dwBytes = 0;
			V_CONTINUE( dxC_IndexBufferGetSizeInMemory( pIB->nId, &dwBytes ) );
			tMemory.dwIBufferBytes += dwBytes;
			tMemory.dwIBuffers++;

			if ( pdwBytesTotal )
				*pdwBytesTotal += dwBytes;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_EngineMemoryDump( BOOL bOutputTrace /*= TRUE*/, DWORD* pdwBytesTotal /*= NULL*/ )
{
	ENGINE_MEMORY tMemory[ cnMaxPool + 1 ];

	if ( pdwBytesTotal )
		*pdwBytesTotal = 0;

	for ( int i = 0; i < cnMaxPool; i++ )
	{
		V( dxC_EngineMemoryGetPool( i, tMemory[ i ], pdwBytesTotal ) );
	}

#ifdef ENGINE_TARGET_DX9
	if ( pdwBytesTotal )
		*pdwBytesTotal += MIN(tMemory[D3DPOOL_MANAGED].Total(), (DWORD)(256 * 1024 * 1024));
#endif
	

	if ( bOutputTrace )
	{
#ifdef ENGINE_TARGET_DX10
		tMemory[ 0 ].DebugTrace( "Total engine memory:\n" );
#elif defined(ENGINE_TARGET_DX9)
		tMemory[ D3DPOOL_DEFAULT   ].DebugTrace( "Total engine POOL_DEFAULT   memory:\n" );
		tMemory[ D3DPOOL_MANAGED   ].DebugTrace( "Total engine POOL_MANAGED   memory:\n" );
		tMemory[ D3DPOOL_SYSTEMMEM ].DebugTrace( "Total engine POOL_SYSTEMMEM memory:\n" );
		tMemory[ D3DPOOL_SCRATCH   ].DebugTrace( "Total engine POOL_SCRATCH   memory:\n" );
#endif
	}

	return S_OK;
}

