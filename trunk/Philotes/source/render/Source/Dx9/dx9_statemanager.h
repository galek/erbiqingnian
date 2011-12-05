//----------------------------------------------------------------------------
// dx9_statemanager.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_STATEMANAGER__
#define __DX9_STATEMANAGER__

#include "hash.h"
#include "matrix.h"
#include "vector.h"
#include "boundingbox.h"
#include "dxC_effect.h"
#include "dxC_caps.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// set to 1+ the last renderstate in the _D3DRENDERSTATETYPE enum
#define D3D_RENDERSTATE_COUNT			( D3DRS_BLENDOPALPHA + 1 )
// set to 1+ the last texturestate in the _D3DTEXTURESTAGESTATETYPE enum
#define D3D_TEXTURESTATE_COUNT			( D3DTSS_CONSTANT + 1 )
// set to 1+ the last samplerstate in the _D3DSAMPLERSTATETYPE enum
#define D3D_SAMPLERSTATE_COUNT			( D3DSAMP_DMAPOFFSET + 1 )
// set to max number of textures slots used
#define D3D_TEXTURE_COUNT				16
// set to max number of vertex streams used
#define D3D_VERTEXSTREAM_COUNT			4

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

template< class T, int count > struct STATE_ARRAY
{
	static const int snCount = count; // for iteration outside a template function context
	DWORD	dwPresent[ DWORD_FLAG_SIZE(count) ];
	T		pStates[ count ];
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

#define RENDERSTATE_TEMPLATE_PARAMS										DWORD,D3D_RENDERSTATE_COUNT
#define TEXTURESTATE_TEMPLATE_PARAMS									DWORD,D3D_TEXTURESTATE_COUNT
#define SAMPLERSTATE_TEMPLATE_PARAMS									DWORD,D3D_SAMPLERSTATE_COUNT
#define TEXTURE_TEMPLATE_PARAMS											LPD3DCBASETEXTURE,D3D_TEXTURE_COUNT
#define VERTEXSTREAM_TEMPLATE_PARAMS									LPD3DCVB,D3D_VERTEXSTREAM_COUNT

#define RENDERSTATE_ARRAY												STATE_ARRAY<RENDERSTATE_TEMPLATE_PARAMS>
#define TEXTURESTATE_ARRAY												STATE_ARRAY<TEXTURESTATE_TEMPLATE_PARAMS>						
#define SAMPLERSTATE_ARRAY												STATE_ARRAY<SAMPLERSTATE_TEMPLATE_PARAMS>						
#define TEXTURE_ARRAY													STATE_ARRAY<TEXTURE_TEMPLATE_PARAMS>							
#define VERTEXSTREAM_ARRAY												STATE_ARRAY<VERTEXSTREAM_TEMPLATE_PARAMS>						

//----------------------------------------------------------------------------
// TEMPLATE FUNCTION INLINES
//----------------------------------------------------------------------------

// Called to update the cache
// ----
// The return value indicates whether or not the update was a redundant change.
// A value of 'true' indicates the new state was unique, and must be submitted
// to the D3D Runtime.
template< class T, int count > BOOL dx9_StateCacheSet( STATE_ARRAY<T,count> & tArray, DWORD dwIndex, T & tValue )
{
	if ( dx9_StateArrayHas( tArray, dwIndex ) )
	{
		T tCurrent = dx9_StateArrayGet( tArray, dwIndex );
		if ( tValue == tCurrent )
			return FALSE;
	}

	dx9_StateArraySet( tArray, dwIndex, tValue );
	return TRUE;
}

template BOOL dx9_StateCacheSet<DWORD,D3D_RENDERSTATE_COUNT> (
	STATE_ARRAY<DWORD,D3D_RENDERSTATE_COUNT> & tArray,
	DWORD dwIndex,
	DWORD & tValue );
template BOOL dx9_StateCacheSet<DWORD,D3D_TEXTURESTATE_COUNT> (
	STATE_ARRAY<DWORD,D3D_TEXTURESTATE_COUNT> & tArray,
	DWORD dwIndex,
	DWORD & tValue );
template BOOL dx9_StateCacheSet<DWORD,D3D_SAMPLERSTATE_COUNT> (
	STATE_ARRAY<DWORD,D3D_SAMPLERSTATE_COUNT> & tArray,
	DWORD dwIndex,
	DWORD & tValue );
template BOOL dx9_StateCacheSet<LPD3DCBASETEXTURE,D3D_TEXTURE_COUNT> (
	STATE_ARRAY<LPD3DCBASETEXTURE,D3D_TEXTURE_COUNT> & tArray,
	DWORD dwIndex,
	LPD3DCBASETEXTURE & tValue );
template BOOL dx9_StateCacheSet<LPD3DCVB,D3D_VERTEXSTREAM_COUNT> (
	STATE_ARRAY<LPD3DCVB,D3D_VERTEXSTREAM_COUNT> & tArray,
	DWORD dwIndex,
	LPD3DCVB & tValue );


// Copy all array items to the other array, blowing away contents of tToArray
template< class T, int count > void dx9_BatchCacheCopy( STATE_ARRAY<T,count> & tToArray, STATE_ARRAY<T,count> & tFromArray )
{
	memcpy( &tToArray, &tFromArray, sizeof(STATE_ARRAY<T,count>) );
}

template void dx9_BatchCacheCopy<RENDERSTATE_TEMPLATE_PARAMS>(
	RENDERSTATE_ARRAY & tToArray,
	RENDERSTATE_ARRAY & tFromArray );
template void dx9_BatchCacheCopy<TEXTURESTATE_TEMPLATE_PARAMS>(
	TEXTURESTATE_ARRAY & tToArray,
	TEXTURESTATE_ARRAY & tFromArray );
template void dx9_BatchCacheCopy<SAMPLERSTATE_TEMPLATE_PARAMS>(
	SAMPLERSTATE_ARRAY & tToArray,
	SAMPLERSTATE_ARRAY & tFromArray );
template void dx9_BatchCacheCopy<TEXTURE_TEMPLATE_PARAMS>(
	TEXTURE_ARRAY & tToArray,
	TEXTURE_ARRAY & tFromArray );
template void dx9_BatchCacheCopy<VERTEXSTREAM_TEMPLATE_PARAMS>(
	VERTEXSTREAM_ARRAY & tToArray,
	VERTEXSTREAM_ARRAY & tFromArray );

// Copy all array items that are different than the default array to the other array
template< class T, int count > void dx9_BatchCacheCopyDiff( STATE_ARRAY<T,count> & tToArray, STATE_ARRAY<T,count> & tFromArray, STATE_ARRAY<T,count> & tDefArray )
{
	for ( int i = 0; i < count; i++ )
	{
		// if (from != def) to = from
		if( dx9_StateArrayHas( tFromArray, i ) )
		{
			BOOL bSet = FALSE;
			T tFromValue = dx9_StateArrayGet( tFromArray, i );

			if ( dx9_StateArrayHas( tDefArray, i ) )
			{
				T tDefValue = dx9_StateArrayGet( tDefArray, i );
				if ( tDefValue != tFromValue )
					dx9_StateArraySet( tToArray, i, tFromValue );
			} else
			{
				dx9_StateArraySet( tToArray, i, tFromValue );
			}
		}
	}
}

template< class T, int count > void dx9_StateArrayClear( STATE_ARRAY<T,count> & tArray )
{
	memset( tArray.dwPresent, 0, sizeof(tArray.dwPresent) );
}

template< class T, int count > void dx9_StateArraySet( STATE_ARRAY<T,count> & tArray, int nIndex, T & tValue )
{
	SETBIT( tArray.dwPresent, nIndex, TRUE );
	tArray.pStates[ nIndex ] = tValue;
}

template< class T, int count > void dx9_StateArrayRemove( STATE_ARRAY<T,count> & tArray, int nIndex )
{
	SETBIT( tArray.dwPresent, nIndex, FALSE );
}

template< class T, int count > BOOL dx9_StateArrayHas( const STATE_ARRAY<T,count> & tArray, int nIndex )
{
	return TESTBIT_MACRO_ARRAY( tArray.dwPresent, nIndex );
}

// this function doesn't test for the presense of a state in the array
template< class T, int count > T dx9_StateArrayGet( const STATE_ARRAY<T,count> & tArray, int nIndex )
{
	return tArray.pStates[ nIndex ];
}


//--------------------------------------------------------------------------------------
// CLASSES
//--------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

#endif // __DX9_STATEMANAGER__