//----------------------------------------------------------------------------
// dxC_movie.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include <map>
#include "memoryallocator_stl.h"
#include "dxC_buffer.h"
#include "dxC_texture.h"
#include "dxC_state.h"
#include "dxC_caps.h"

#ifdef BINK_ENABLED
#include "Bink.h"
#endif

#include "e_movie.h"

using namespace std;
using namespace FSCommon;

namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


#ifdef BINK_ENABLED

typedef struct BINKFRAMETEXTURES
{
	DWORD			dwSizeY;
	DWORD			dwSizeCr;
	DWORD			dwSizeCb;
	DWORD			dwSizeA;

	SPD3DCTEXTURE2D pYTexture;
	SPD3DCTEXTURE2D pCrTexture;
	SPD3DCTEXTURE2D pCbTexture;
	SPD3DCTEXTURE2D pATexture;

	SPD3DCSHADERRESOURCEVIEW pYTextureSRV;
	SPD3DCSHADERRESOURCEVIEW pCrTextureSRV;
	SPD3DCSHADERRESOURCEVIEW pCbTextureSRV;
	SPD3DCSHADERRESOURCEVIEW pATextureSRV;

	void ReleaseAll()
	{
		pYTexture = NULL;
		pYTextureSRV = NULL;

		pCrTexture = NULL;
		pCrTextureSRV = NULL;

		pCbTexture = NULL;
		pCbTextureSRV = NULL;

		pATexture = NULL;
		pATextureSRV = NULL;
	}
} BINKFRAMETEXTURES;


typedef struct BINKTEXTURESET
{
	BINKFRAMETEXTURES tSysTextures[ BINKMAXFRAMEBUFFERS ];
	BINKFRAMEBUFFERS tBinkBuffers;
	BINKFRAMETEXTURES tDrawTextures;
} BINKTEXTURESET;

#endif // BINK_ENABLED



struct MovieResources
{
	MOVIE_HANDLE hMovie;

#ifdef BINK_ENABLED
	BINKTEXTURESET tTextureSet;
	HBINK hBink;
#endif // BINK_ENABLED

	D3D_VERTEX_BUFFER_DEFINITION tVBDef;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

typedef map<DWORD, MovieResources, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, MovieResources> > > MovieResourceMap;
static MovieResourceMap * g_Movies = NULL;
static int gnNextMovieID = 1;

static MOVIE_HANDLE sghMovieFullscreen	= NULL;

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

static PRESULT sFreeDrawTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED
	pMovieRes->tTextureSet.tDrawTextures.ReleaseAll();
#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sFreeTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED
	for ( int i = 0; i < BINKMAXFRAMEBUFFERS; ++i )
	{
		pMovieRes->tTextureSet.tSysTextures[ i ].ReleaseAll();
	}
	V_RETURN( sFreeDrawTextures( pMovieRes ) );
#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sMakeTexture(
	DWORD w,
	DWORD h,
	D3DC_USAGE tUsage,
	DXC_FORMAT tFormat,
	SPD3DCTEXTURE2D & pTexture,
	LPD3DCSHADERRESOURCEVIEW* ppSRV,
	void ** ppOutBits,
	unsigned int * pOutPitch,
	DWORD * pOutSize )
{
	ASSERT_RETINVALIDARG( pOutSize );
	ASSERT_RETINVALIDARG( w > 0 );
	ASSERT_RETINVALIDARG( h > 0 );
	REF( tUsage );
	REF( tFormat );
	REF( pOutPitch );

#ifdef BINK_ENABLED

	SPD3DCTEXTURE2D pTex;

	// release any existing texture
	pTexture = NULL;

	PRESULT pr = dxC_Create2DTexture(
		w, h, 1,
		tUsage, tFormat,
		&pTex );


	if ( FAILED(pr) )
	{
		if ( pTex )
			pTex = NULL;
		return pr;
	}

	if( ppSRV )
	{
#ifdef ENGINE_TARGET_DX10
		dxC_GetDevice()->CreateShaderResourceView( pTex, NULL, ppSRV );
#else
		*ppSRV = pTex;
		(*ppSRV)->AddRef();
#endif
	}

	// success!
	pTexture = pTex;
	int nPixelBitDepth = dx9_GetTextureFormatBitDepth( tFormat );
	*pOutSize = w * h * nPixelBitDepth / 8;

	if ( ppOutBits && pOutPitch )
	{
		D3DLOCKED_RECT tLR;
		V_RETURN( dxC_MapTexture( pTex, 0, &tLR ) );
		*pOutPitch = dxC_nMappedRectPitch(tLR);
		*ppOutBits = dxC_pMappedRectData(tLR);
		dxC_UnmapTexture( pTex, 0 );
	}

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sCreateDrawTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED
	PRESULT pr = E_FAIL;

	BINKFRAMEBUFFERS * bb = &pMovieRes->tTextureSet.tBinkBuffers;
	BINKFRAMETEXTURES * bt = &pMovieRes->tTextureSet.tDrawTextures;
	bt->ReleaseAll();

	// Create the video memory draw texture set

	// Create Y plane
	if ( bb->Frames[0].YPlane.Allocate )
	{
		V_SAVE_GOTO( pr, fail, sMakeTexture(
			bb->YABufferWidth, bb->YABufferHeight,
			D3DC_USAGE_2DTEX_DEFAULT, D3DFMT_L8,
			bt->pYTexture,
			&bt->pYTextureSRV,
			NULL, NULL,
			&bt->dwSizeY ) );
	}

	// Create cR plane
	if ( bb->Frames[0].cRPlane.Allocate )
	{
		V_SAVE_GOTO( pr, fail, sMakeTexture(
			bb->cRcBBufferWidth, bb->cRcBBufferHeight,
			D3DC_USAGE_2DTEX_DEFAULT, D3DFMT_L8,
			bt->pCrTexture,
			&bt->pCrTextureSRV,
			NULL, NULL,
			&bt->dwSizeCr ) );
	}

	// Create cB plane
	if ( bb->Frames[0].cBPlane.Allocate )
	{
		V_SAVE_GOTO( pr, fail, sMakeTexture(
			bb->cRcBBufferWidth, bb->cRcBBufferHeight,
			D3DC_USAGE_2DTEX_DEFAULT, D3DFMT_L8,
			bt->pCbTexture,
			&bt->pCbTextureSRV,
			NULL, NULL,
			&bt->dwSizeCb ) );
	}

	// Create A plane
	if ( bb->Frames[0].APlane.Allocate )
	{
		V_SAVE_GOTO( pr, fail, sMakeTexture(
			bb->YABufferWidth, bb->YABufferHeight,
			D3DC_USAGE_2DTEX_DEFAULT, D3DFMT_L8,
			bt->pATexture,
			&bt->pATextureSRV,
			NULL, NULL,
			&bt->dwSizeA ) );
	}

	return S_OK;

fail:
	V_RETURN( sFreeDrawTextures( pMovieRes ) );
	return pr;

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sCreateTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED
	PRESULT pr = E_FAIL;

	BINKFRAMEBUFFERS * bb = &pMovieRes->tTextureSet.tBinkBuffers;

	// Create the system decompress texture sets

	for ( int i = 0; i < bb->TotalFrames; ++i )
	{
		BINKFRAMETEXTURES * bt = &pMovieRes->tTextureSet.tSysTextures[i];

		bt->ReleaseAll();

		// Create Y plane
		if ( bb->Frames[i].YPlane.Allocate )
		{
			V_SAVE_GOTO( pr, fail, sMakeTexture(
				bb->YABufferWidth, bb->YABufferHeight,
				D3DC_USAGE_2DTEX_STAGING, D3DFMT_L8,
				bt->pYTexture,
				NULL,
				&bb->Frames[i].YPlane.Buffer,
				&bb->Frames[i].YPlane.BufferPitch,
				&bt->dwSizeY ) );
		}

		// Create cR plane
		if ( bb->Frames[i].cRPlane.Allocate )
		{
			V_SAVE_GOTO( pr, fail, sMakeTexture(
				bb->cRcBBufferWidth, bb->cRcBBufferHeight,
				D3DC_USAGE_2DTEX_STAGING, D3DFMT_L8,
				bt->pCrTexture,
				NULL,
				&bb->Frames[i].cRPlane.Buffer,
				&bb->Frames[i].cRPlane.BufferPitch,
				&bt->dwSizeCr ) );
		}

		// Create cB plane
		if ( bb->Frames[i].cBPlane.Allocate )
		{
			V_SAVE_GOTO( pr, fail, sMakeTexture(
				bb->cRcBBufferWidth, bb->cRcBBufferHeight,
				D3DC_USAGE_2DTEX_STAGING, D3DFMT_L8,
				bt->pCbTexture,
				NULL,
				&bb->Frames[i].cBPlane.Buffer,
				&bb->Frames[i].cBPlane.BufferPitch,
				&bt->dwSizeCb ) );
		}

		// Create A plane
		if ( bb->Frames[i].APlane.Allocate )
		{
			V_SAVE_GOTO( pr, fail, sMakeTexture(
				bb->YABufferWidth, bb->YABufferHeight,
				D3DC_USAGE_2DTEX_STAGING, D3DFMT_L8,
				bt->pATexture,
				NULL,
				&bb->Frames[i].APlane.Buffer,
				&bb->Frames[i].APlane.BufferPitch,
				&bt->dwSizeA ) );
		}
	}

	V_SAVE_GOTO( pr, fail, sCreateDrawTextures( pMovieRes ) );

	return S_OK;

fail:
	V_RETURN( sFreeTextures( pMovieRes ) );
	return pr;

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sLockTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED

	BINKFRAMETEXTURES * bt = pMovieRes->tTextureSet.tSysTextures;
	BINKFRAMEPLANESET * bp = pMovieRes->tTextureSet.tBinkBuffers.Frames;

	int frame_cur = pMovieRes->tTextureSet.tBinkBuffers.FrameNum;

	for ( int i = 0; i < pMovieRes->tTextureSet.tBinkBuffers.TotalFrames; ++i, ++bt, ++bp )
	{
		bool bNoDirtyUpdate = ( i == frame_cur );
		bool bReadOnly      = ! bNoDirtyUpdate;
		D3DLOCKED_RECT tLR;

		ASSERT_CONTINUE( bt->pYTexture );
		ASSERT_CONTINUE( bt->pCrTexture );
		ASSERT_CONTINUE( bt->pCbTexture );

		V_DO_SUCCEEDED( dxC_MapTexture( bt->pYTexture, 0, &tLR, bReadOnly, bNoDirtyUpdate ) )
		{
			bp->YPlane.Buffer      = dxC_pMappedRectData( tLR );
			bp->YPlane.BufferPitch = dxC_nMappedRectPitch( tLR );
		}

		V_DO_SUCCEEDED( dxC_MapTexture( bt->pCrTexture, 0, &tLR, bReadOnly, bNoDirtyUpdate ) )
		{
			bp->cRPlane.Buffer      = dxC_pMappedRectData( tLR );
			bp->cRPlane.BufferPitch = dxC_nMappedRectPitch( tLR );
		}

		V_DO_SUCCEEDED( dxC_MapTexture( bt->pCbTexture, 0, &tLR, bReadOnly, bNoDirtyUpdate ) )
		{
			bp->cBPlane.Buffer      = dxC_pMappedRectData( tLR );
			bp->cBPlane.BufferPitch = dxC_nMappedRectPitch( tLR );
		}

		if ( bt->pATexture )
		{
			V_DO_SUCCEEDED( dxC_MapTexture( bt->pATexture, 0, &tLR, bReadOnly, bNoDirtyUpdate ) )
			{
				bp->APlane.Buffer      = dxC_pMappedRectData( tLR );
				bp->APlane.BufferPitch = dxC_nMappedRectPitch( tLR );
			}
		}
	}

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX10
	#define DX10_NO_DIRTY_RECT //For some reason we get a green line at the bottom of the video if we try to emulate dirty rects
#endif

static PRESULT sUpdateTextures( MovieResources * pMovieRes, bool bForceUpdateAll )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef DX10_NO_DIRTY_RECT
	bForceUpdateAll = true;
#endif

#ifdef BINK_ENABLED
	BINKFRAMETEXTURES * bt_src = &pMovieRes->tTextureSet.tSysTextures[ pMovieRes->tTextureSet.tBinkBuffers.FrameNum ];
	BINKFRAMETEXTURES * bt_dst = &pMovieRes->tTextureSet.tDrawTextures;

	ASSERT_RETFAIL( bt_src->pYTexture );
	ASSERT_RETFAIL( bt_src->pCrTexture );
	ASSERT_RETFAIL( bt_src->pCbTexture );



	// In DX9, we need to mark a dirty rectangle and call UpdateTexture to propagate the changes.

	ASSERT_RETFAIL( bt_dst->pYTexture );
	ASSERT_RETFAIL( bt_dst->pCrTexture );
	ASSERT_RETFAIL( bt_dst->pCbTexture );

	// CHB 2007.07.30 - If the device was lost (and the draw textures
	// discarded and recreated), need to do a full update.

	int nRects = bForceUpdateAll ? 0 : BinkGetRects( pMovieRes->hBink, BINKSURFACEFAST );

#ifdef ENGINE_TARGET_DX10  //Need to build our own dirty rect
	D3D10_BOX ayBox;
	D3D10_BOX brBox;
	
	brBox.front = ayBox.front = 0;
	brBox.back = ayBox.back = 1;
	brBox.top = brBox.left = ayBox.top = ayBox.left = 65536;
	brBox.bottom = brBox.right = ayBox.bottom = ayBox.right = 0;
#endif

	if ( ( nRects > 0 ) || bForceUpdateAll )
	{
		BINKRECT * brc;
		RECT rc;

		for ( int i = 0; i < nRects; ++i )
		{
			brc = &pMovieRes->hBink->FrameRects[i];
			rc.left = brc->Left;
			rc.top = brc->Top;
			rc.right = rc.left + brc->Width;
			rc.bottom = rc.top + brc->Height;

#ifdef ENGINE_TARGET_DX9
			V( ((LPDIRECT3DTEXTURE9)bt_src->pYTexture)->AddDirtyRect( &rc ) );
			if ( bt_src->pATexture )
			{
				V( ((LPDIRECT3DTEXTURE9)bt_src->pATexture)->AddDirtyRect( &rc ) );
			}
#else
			//Stretch the rectangle to take into acount this rect
			ayBox.bottom = rc.bottom > ayBox.bottom ? rc.bottom : ayBox.bottom;
			ayBox.right = rc.right > ayBox.right ? rc.right : ayBox.right;
			ayBox.top = rc.top < ayBox.top ? rc.top : ayBox.top;
			ayBox.left = rc.left < ayBox.left ? rc.left : ayBox.left;
#endif

			rc.left >>= 1;
			rc.top >>= 1;
			rc.right >>= 1;
			rc.bottom >>= 1;

#ifdef ENGINE_TARGET_DX9
			((LPDIRECT3DTEXTURE9)bt_src->pCrTexture)->AddDirtyRect( &rc );
			((LPDIRECT3DTEXTURE9)bt_src->pCbTexture)->AddDirtyRect( &rc );
#else
			//Stretch the rectangle to take into acount this rect
			brBox.bottom = rc.bottom > brBox.bottom ? rc.bottom : brBox.bottom;
			brBox.right = rc.right > brBox.right ? rc.right : brBox.right;
			brBox.top = rc.top < brBox.top ? rc.top : brBox.top;
			brBox.left = rc.left < brBox.left ? rc.left : brBox.left;
#endif
		}

#ifdef ENGINE_TARGET_DX9
		if (bForceUpdateAll)
		{
			V( ((LPDIRECT3DTEXTURE9)bt_src->pYTexture)->AddDirtyRect( 0 ) );
			((LPDIRECT3DTEXTURE9)bt_src->pCrTexture)->AddDirtyRect( 0 );
			((LPDIRECT3DTEXTURE9)bt_src->pCbTexture)->AddDirtyRect( 0 );
			if ( bt_src->pATexture )
			{
				V( ((LPDIRECT3DTEXTURE9)bt_src->pATexture)->AddDirtyRect( 0 ) );
			}
		}

		V( dxC_GetDevice()->UpdateTexture( bt_src->pYTexture, bt_dst->pYTexture ) );
		V( dxC_GetDevice()->UpdateTexture( bt_src->pCrTexture, bt_dst->pCrTexture ) );
		V( dxC_GetDevice()->UpdateTexture( bt_src->pCbTexture, bt_dst->pCbTexture ) );

		if ( bt_src->pATexture )
		{
			ASSERT_RETFAIL( bt_dst->pATexture );
			V( dxC_GetDevice()->UpdateTexture( bt_src->pATexture, bt_dst->pATexture ) );
		}
#else //DX10

	if ( bt_src->pATexture )
		dxC_GetDevice()->CopySubresourceRegion( bt_dst->pATexture, 0, 0, 0, 0, bt_src->pATexture, 0, bForceUpdateAll ? 0 : &ayBox );
	
	dxC_GetDevice()->CopySubresourceRegion( bt_dst->pYTexture, 0, 0, 0, 0, bt_src->pYTexture, 0, bForceUpdateAll ? 0 : &ayBox );

	dxC_GetDevice()->CopySubresourceRegion( bt_dst->pCrTexture, 0, 0, 0, 0, bt_src->pCrTexture, 0, bForceUpdateAll ? 0 : &brBox );
	dxC_GetDevice()->CopySubresourceRegion( bt_dst->pCbTexture, 0, 0, 0, 0, bt_src->pCbTexture, 0, bForceUpdateAll ? 0 : &brBox );
#endif
	}


#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sUnlockTextures( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED
	BINKFRAMETEXTURES * bt = pMovieRes->tTextureSet.tSysTextures;
	BINKFRAMEPLANESET * bp = pMovieRes->tTextureSet.tBinkBuffers.Frames;

	for ( int i = 0; i < pMovieRes->tTextureSet.tBinkBuffers.TotalFrames; ++i, ++bt, ++bp )
	{
		ASSERT_CONTINUE( bt->pYTexture );
		ASSERT_CONTINUE( bt->pCrTexture );
		ASSERT_CONTINUE( bt->pCbTexture );

		dxC_UnmapTexture( bt->pYTexture, 0 );
		bp->YPlane.Buffer = NULL;

		dxC_UnmapTexture( bt->pCrTexture, 0 );
		bp->cRPlane.Buffer = NULL;

		dxC_UnmapTexture( bt->pCbTexture, 0 );
		bp->cBPlane.Buffer = NULL;

		if ( bt->pATexture )
		{
			dxC_UnmapTexture( bt->pATexture, 0 );
			bp->APlane.Buffer = NULL;
		}
	}

	V_RETURN( sUpdateTextures( pMovieRes, false ) );
#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sCreateVB( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED

	// Gather size info from Bink.

	ASSERTX_RETFAIL( pMovieRes->hBink, "Need Bink handle to allocate engine movie resources!" );

	UINT width = pMovieRes->hBink->Width;
	UINT height = pMovieRes->hBink->Height;
	float x_offset = 0.f;
	float y_offset = 0.f;

	int nScreenWidth, nScreenHeight;
	e_GetWindowSize( nScreenWidth, nScreenHeight );
	ASSERT_RETFAIL( nScreenWidth >= 1 );
	ASSERT_RETFAIL( nScreenHeight >= 1 );
	float x_scale = (float)nScreenWidth / width;
	float y_scale = (float)nScreenHeight / height;

	float scale = MIN( x_scale, y_scale );
	if ( x_scale < y_scale )
		y_offset = fabsf( height * scale - nScreenHeight ) * 0.5f;
	else if ( y_scale < x_scale )
		x_offset = fabsf( width * scale - nScreenWidth ) * 0.5f;

	// Create vertex buffer of the appropriate size.

	dxC_VertexBufferDefinitionInit( pMovieRes->tVBDef );

	ASSERT_RETFAIL( ! dxC_VertexBufferD3DExists( pMovieRes->tVBDef.nD3DBufferID[ 0 ] ) );

	const int cnVerts = 4;
	struct QuadVertex
	{
		float x, y, z;
		float u, v;
	};

	pMovieRes->tVBDef.dwFVF = DX9_BLOCK( SCREEN_COVER_FVF ) DX10_BLOCK( D3DC_FVF_NULL );
	pMovieRes->tVBDef.nVertexCount = cnVerts;
	int nBufferSize = cnVerts * sizeof( QuadVertex );
	pMovieRes->tVBDef.nBufferSize[ 0 ] = nBufferSize;
	pMovieRes->tVBDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	pMovieRes->tVBDef.eVertexType = VERTEX_DECL_XYZ_UV;
	V_DO_SUCCEEDED( dxC_CreateVertexBuffer( 0, pMovieRes->tVBDef ) )
	{
		QuadVertex pVerts[ cnVerts ];

		pVerts[0].x   = x_offset;
		pVerts[0].y   = y_offset;
		pVerts[0].z   = 0.f;
		pVerts[0].u   = 0.f;
		pVerts[0].v   = 0.f;
		pVerts[1]     = pVerts[0];
		pVerts[1].x   = x_offset + ( width * scale );
		pVerts[1].u   = 1.f;
		pVerts[2]     = pVerts[0];
		pVerts[2].y   = y_offset + ( height * scale );
		pVerts[2].v   = 1.f;
		pVerts[3]     = pVerts[1];
		pVerts[3].y   = pVerts[2].y;
		pVerts[3].v   = 1.f;

		V_RETURN( dxC_UpdateBuffer( 0, pMovieRes->tVBDef, pVerts, 0, nBufferSize ) );
	}

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sFreeVB( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

#ifdef BINK_ENABLED

	for ( int i = 0; i < DXC_MAX_VERTEX_STREAMS; i++ )
	{
		V( dxC_VertexBufferDestroy( pMovieRes->tVBDef.nD3DBufferID[i] ) );
		pMovieRes->tVBDef.nD3DBufferID[i] = INVALID_ID;
	}

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sMovieGetIterator( MOVIE_HANDLE hMovie, MovieResourceMap::iterator & i )
{
	ASSERT_RETFAIL( g_Movies );
	i = g_Movies->end();
	if ( hMovie <= 0 )
		return S_FALSE;

	DWORD dwMovie = static_cast<DWORD>(hMovie);
	i = g_Movies->find( dwMovie );

	return S_OK;
}

//----------------------------------------------------------------------------

static MovieResources * sMovieGet( MOVIE_HANDLE hMovie )
{
	if ( ! g_Movies )
		return NULL;
	if ( hMovie <= 0 )
		return NULL;

	MovieResourceMap::iterator iMovie;
	V_DO_FAILED( sMovieGetIterator( hMovie, iMovie ) )
	{
		return NULL;
	}

	if ( iMovie == g_Movies->end() )
	{
		// didn't find it in the map!
		return NULL;
	}

	return &(iMovie->second);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sMovieRemove( MovieResourceMap::iterator i, BOOL bRemoveHash )
{
	ASSERT_RETINVALIDARG( i != g_Movies->end() );

	// free movie resources
	V( sFreeVB( &(i->second) ) );
	V( sFreeTextures( &(i->second) ) );

	if ( bRemoveHash )
		g_Movies->erase( i );

	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sMovieRemove( MOVIE_HANDLE hMovie, BOOL bRemoveHash )
{
	MovieResourceMap::iterator i;
	V_RETURN( sMovieGetIterator( hMovie, i ) );

	if ( i == g_Movies->end() )
		return S_FALSE;

	return sMovieRemove( i, bRemoveHash );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MoviesInit()
{
	ASSERT_RETFAIL( NULL == g_Movies );
	g_Movies	= MEMORYPOOL_NEW(g_StaticAllocator) MovieResourceMap(MovieResourceMap::key_compare(), MovieResourceMap::allocator_type(g_StaticAllocator));
	ASSERT_RETVAL( g_Movies, E_OUTOFMEMORY );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MoviesDestroy()
{
	if ( g_Movies )
	{
		for ( MovieResourceMap::iterator i = g_Movies->begin();
			i != g_Movies->end();
			++i)
		{
			V( sMovieRemove( i, FALSE ) );
		}
		MEMORYPOOL_PRIVATE_DELETE( g_StaticAllocator, g_Movies, MovieResourceMap );
		g_Movies = NULL;
	}
	return S_OK;
}

//----------------------------------------------------------------------------

static PRESULT sMovieRelease( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

	V( sFreeVB( pMovieRes ) );
	V( sFreeTextures( pMovieRes ) );

	return S_OK;
}

static PRESULT sMovieRestore( MovieResources * pMovieRes )
{
	ASSERT_RETINVALIDARG( pMovieRes );

	V( sCreateVB( pMovieRes ) );
	V( sCreateTextures( pMovieRes ) );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MoviesDeviceLost()
{
	if ( ! g_Movies )
		return S_FALSE;

	for ( MovieResourceMap::iterator i = g_Movies->begin();
		i != g_Movies->end();
		++i)
	{
		V( sFreeVB( &(i->second) ) );

		// CHB 2007.07.30 - Keep system memory textures intact
		// through device reset.
		V( sFreeDrawTextures( &(i->second) ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MoviesDeviceReset()
{
	if ( ! g_Movies )
		return S_FALSE;

	for ( MovieResourceMap::iterator i = g_Movies->begin();
		i != g_Movies->end();
		++i)
	{
		V( sCreateVB( &(i->second) ) );
		V( sCreateDrawTextures( &(i->second) ) );

		// CHB 2007.07.30 - If the device was lost (and the draw textures
		// discarded and recreated), need to do a full update.
		sUpdateTextures( &(i->second), true );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

#ifdef BINK_ENABLED
PRESULT e_MovieCreate( MOVIE_HANDLE & hMovie, HBINK hBink )
{
	ASSERT_RETINVALIDARG( hBink != 0 );	// CHB 2007.09.17

	hMovie = NULL;
	if ( ! g_Movies )
		return S_FALSE;

	DWORD dwID = (DWORD)gnNextMovieID;
	MovieResources * pMovieRes = &(*g_Movies)[ dwID ];
	++gnNextMovieID;
	ASSERT_RETFAIL( pMovieRes );
	hMovie = (MOVIE_HANDLE)dwID;

	pMovieRes->hMovie = hMovie;
	pMovieRes->hBink = hBink;

	// CHB 2007.09.17 - Call this only once, at movie initialization,
	// otherwise it will overwrite the buffer structure mid-playback
	// (which is not a good idea for the FrameNum member).
	BinkGetFrameBuffersInfo( pMovieRes->hBink, &pMovieRes->tTextureSet.tBinkBuffers );

	V_RETURN( sMovieRestore( pMovieRes ) );

	return S_OK;
}
#endif // BINK_ENABLE

//----------------------------------------------------------------------------

PRESULT e_MovieSetFullscreen( MOVIE_HANDLE hMovie )
{
	if ( ! g_Movies )
		return S_FALSE;

	sghMovieFullscreen = hMovie;
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_MovieRemove( MOVIE_HANDLE hMovie )
{
	if ( ! g_Movies )
		return S_FALSE;

	if ( hMovie == sghMovieFullscreen )
		sghMovieFullscreen = NULL;

	return sMovieRemove( hMovie, TRUE );
}

//----------------------------------------------------------------------------

PRESULT dxC_MovieUpdate()
{
	if ( ! g_Movies )
		return S_FALSE;

	if ( ! sghMovieFullscreen )
		return S_FALSE;

	MovieResources * pMovieRes = sMovieGet( sghMovieFullscreen );
	if ( ! pMovieRes )
	{
		V( sMovieRemove( sghMovieFullscreen, TRUE ) );
		return S_FALSE;
	}

#ifdef BINK_ENABLED

	if ( BinkWait( pMovieRes->hBink ) )
		return S_FALSE;

	V_RETURN( sLockTextures( pMovieRes ) );
	BinkRegisterFrameBuffers( pMovieRes->hBink, &pMovieRes->tTextureSet.tBinkBuffers );

	// Decompress a frame.
	BinkDoFrame( pMovieRes->hBink );

	// If we are falling behind, decompress an extra frame to catch up.
	while ( BinkShouldSkip( pMovieRes->hBink ) )
	{
		BinkNextFrame( pMovieRes->hBink );
		BinkDoFrame( pMovieRes->hBink );
	}

	V_RETURN( sUnlockTextures( pMovieRes ) );

	BinkNextFrame( pMovieRes->hBink );

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MovieRender()
{
	if ( ! g_Movies )
		return S_FALSE;

	if ( ! sghMovieFullscreen )
		return S_FALSE;

	MovieResources * pMovieRes = sMovieGet( sghMovieFullscreen );
	if ( ! pMovieRes )
	{
		V( sMovieRemove( sghMovieFullscreen, TRUE ) );
		return S_FALSE;
	}

#ifdef BINK_ENABLED

	//if ( BinkWait( pMovieRes->hBink ) )
	//	return S_FALSE;

	BINKFRAMEPLANESET * bp = &pMovieRes->tTextureSet.tBinkBuffers.Frames[ pMovieRes->tTextureSet.tBinkBuffers.FrameNum ];
	BINKFRAMETEXTURES * bt_src = &pMovieRes->tTextureSet.tSysTextures[ pMovieRes->tTextureSet.tBinkBuffers.FrameNum ];
	BINKFRAMETEXTURES * bt_dst = &pMovieRes->tTextureSet.tDrawTextures;


	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_MOVIE );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );

	// right now, this is assumed
	BOOL bMovieHasAlpha = FALSE;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = (int)bMovieHasAlpha;
	EFFECT_TECHNIQUE * pTechnique = NULL;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechnique ) );

	ASSERT_RETFAIL( pTechnique );
	UINT uPasses = 0;
	V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &uPasses ) );
	ASSERT_RETFAIL( uPasses > 0 );

	V_RETURN( dxC_SetStreamSource( pMovieRes->tVBDef, 0, pEffect ) );

	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	V( dxC_SetTexture( 0, bt_dst->pYTextureSRV ) );
	V( dxC_SetTexture( 1, bt_dst->pCrTextureSRV ) );
	V( dxC_SetTexture( 2, bt_dst->pCbTextureSRV ) );

	FXConst_MiscLighting tConstLighting = {0};
	tConstLighting.fAlphaScale = 1.0f;
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_MISC_LIGHTING_DATA, (VECTOR4*)&tConstLighting ) );

	V( dxC_EffectSetScreenSize( pEffect, *pTechnique ) );

	if ( bMovieHasAlpha )
	{
		ONCE
		{
			ASSERT_BREAK( bt_src->pATexture );
			V( dxC_SetTexture( 3, bt_dst->pATextureSRV ) );
		}
	}

	for ( UINT uPass = 0; uPass < uPasses; ++uPass )
	{
		V( dxC_EffectBeginPass( pEffect, uPass ) );

		V_RETURN( dxC_DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2, pEffect, METRICS_GROUP_UI, uPass ) );
	}

	V( dxC_SetTexture( 0, NULL ) );
	V( dxC_SetTexture( 1, NULL ) );
	V( dxC_SetTexture( 2, NULL ) );
	V( dxC_SetTexture( 3, NULL ) );

#endif // BINK_ENABLED

	return S_OK;
}

//----------------------------------------------------------------------------

BOOL e_MovieUseLowResolution()
{
	// Anything lower than these values should use the low-res version of the movies!
	const int cnMinCPUMHz = 1700;
	const int cnMinBusMHz = 400;

	BOOL bLowRes = FALSE;
	if ( e_CapGetValue(CAP_CPU_SPEED_MHZ) < cnMinCPUMHz )
		bLowRes = TRUE;
	if ( e_CapGetValue(CAP_SYSTEM_CLOCK_SPEED_MHZ) < cnMinBusMHz )
		bLowRes = TRUE;

	return bLowRes;
}

}; // FSSE