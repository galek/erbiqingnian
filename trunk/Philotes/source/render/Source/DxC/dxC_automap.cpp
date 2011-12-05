//----------------------------------------------------------------------------
// dxC_automap.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_buffer.h"
#include "dxC_effect.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "e_main.h"
#include "datatables.h"

#include "dxC_automap.h"


namespace FSSE
{;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define AUTOMAP_REVEAL_DIST_HG			35.0f
#define AUTOMAP_REVEAL_DIST_TB			12.2f

#define AUTOMAP_LEVEL_MAGIC				0x55171B35
#define AUTOMAP_REGION_MAGIC			0x591929E9

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

typedef struct
{
	VECTOR3 vPosition;
	VECTOR2 uv;
} AUTOMAP_DISPLAY_VERTEX;

struct FOGOFWAR_VERTEX
{
	VECTOR3 pos;
	DWORD	col;
	VECTOR2 uv;
};

typedef unsigned short FOGOFWAR_INDEX;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static CHash<AUTOMAP>	gtAutomaps;
//static int				gnNextAutomapID = 1;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

PRESULT e_AutomapsInit()
{
#ifdef ENABLE_NEW_AUTOMAP

	HashInit( gtAutomaps, NULL, 2 );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_AutomapsDestroy()
{
#ifdef ENABLE_NEW_AUTOMAP

	for ( AUTOMAP * pMap = HashGetFirst(gtAutomaps);
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		V( e_AutomapDestroy( pMap->nId ) );
	}

	HashFree( gtAutomaps );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::Init( int nRegion )
{
#ifdef ENABLE_NEW_AUTOMAP


	for ( int i = 0; i < NUM_BUFFERS; ++i )
	{
		dxC_VertexBufferDefinitionInit( m_tVBDef[i] );
		m_tVBDef[i].eVertexType	= AUTOMAP_GENERATE_VERTEX_DECL;
		m_tVBDef[i].tUsage		= D3DC_USAGE_BUFFER_REGULAR;	// not constant, in case we start adding collision later
	}

	dxC_VertexBufferDefinitionInit( m_tRenderVBDef );
	m_tRenderVBDef.eVertexType = AUTOMAP_DISPLAY_VERTEX_DECL;
	m_tRenderVBDef.tUsage		 = D3DC_USAGE_BUFFER_REGULAR;

	m_tFogOfWar.Init( nRegion );

	SETBIT( m_dwFlagBits, FLAGBIT_BOUNDING_BOX_INIT, TRUE );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

void FOG_OF_WAR::Init( int nRegion )
{
	ASSERT( nRegion != INVALID_ID );

	m_nRegionID = nRegion;
	m_pIndex.Init( NULL );
	m_pNodes.Init( NULL );
	m_nNodesMapped = 0;
	m_dwFlagBits = 0;
	m_nRevealTexture = INVALID_ID;
	m_nMaterial = INVALID_ID;

	dxC_VertexBufferDefinitionInit( m_tVBDef );
	dxC_IndexBufferDefinitionInit( m_tIBDef );

	// For the fog of war "reveal" texture
	RestoreMaterial();
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::Free()
{
#ifdef ENABLE_NEW_AUTOMAP


	m_tFogOfWar.Destroy();

	for ( int i = 0; i < NUM_BUFFERS; ++i )
	{
		for ( int s = 0; s < DXC_MAX_VERTEX_STREAMS; ++s )
		{
			V( dxC_VertexBufferDestroy( m_tVBDef[i].nD3DBufferID[s] ) );
			if ( m_tVBDef[i].pLocalData[s] )
			{
				FREE( NULL, m_tVBDef[i].pLocalData[s] );
				m_tVBDef[i].pLocalData[s] = NULL;
			}
		}
	}

	for ( int s = 0; s < DXC_MAX_VERTEX_STREAMS; ++s )
	{
		V( dxC_VertexBufferDestroy( m_tRenderVBDef.nD3DBufferID[s] ) );
		if ( m_tRenderVBDef.pLocalData[s] )
		{
			FREE( NULL, m_tRenderVBDef.pLocalData[s] );
			m_tRenderVBDef.pLocalData[s] = NULL;
		}
	}

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}


//----------------------------------------------------------------------------

PRESULT AUTOMAP::AddTri( const VECTOR * pv1, const VECTOR * pv2, const VECTOR * pv3, BOOL bWall )
{
#ifdef ENABLE_NEW_AUTOMAP


	ASSERT_RETINVALIDARG( pv1 );
	ASSERT_RETINVALIDARG( pv2 );
	ASSERT_RETINVALIDARG( pv3 );


	// debug: don't support walls yet
	//if ( bWall )
	//	return S_FALSE;

	BUFFER_TYPE eType = bWall ? WALL : GROUND;
	int nIndex = m_tVBDef[eType].nVertexCount;
	int nVertsAlloced = m_tVBDef[eType].nBufferSize[0] / sizeof(AUTOMAP_VERTEX);

	if ( (nIndex + 2) >= nVertsAlloced )
	{
		// need to resize
		int nNewAlloc = nVertsAlloced / 2;		// add half of existing again
		nNewAlloc /= 3;		// convert from verts to tris
		nNewAlloc = MAX( nNewAlloc, 3 );
		V_RETURN( Prealloc( eType, nNewAlloc ) );
	}

	AUTOMAP_VERTEX * pDest = &( ((AUTOMAP_VERTEX*)(m_tVBDef[eType].pLocalData[0]))[ nIndex ] );
	pDest[0] = *pv1;
	pDest[1] = *pv2;
	pDest[2] = *pv3;

	// Track map extents for later transformation.
	if ( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_BOUNDING_BOX_INIT ) )
	{
		m_tMapBBox.vMin = pDest[0];
		m_tMapBBox.vMax = pDest[0];
		SETBIT( m_dwFlagBits, FLAGBIT_BOUNDING_BOX_INIT, FALSE );
	}
	BoundingBoxExpandByPoint( &m_tMapBBox, &pDest[0] );
	BoundingBoxExpandByPoint( &m_tMapBBox, &pDest[1] );
	BoundingBoxExpandByPoint( &m_tMapBBox, &pDest[2] );

	m_tVBDef[eType].nVertexCount += 3;

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::Prealloc( BUFFER_TYPE eType, int nAddlTris )
{
#ifdef ENABLE_NEW_AUTOMAP


	if ( nAddlTris > 0 )
	{
		int nBufSize = m_tVBDef[eType].nBufferSize[0] + ( nAddlTris * 3 * sizeof(AUTOMAP_VERTEX) );
		m_tVBDef[eType].pLocalData[0] = REALLOC( NULL, m_tVBDef[eType].pLocalData[0], nBufSize );
		ASSERT_RETVAL( m_tVBDef[eType].pLocalData[0], E_OUTOFMEMORY );
		m_tVBDef[eType].nBufferSize[0] = nBufSize;
	}

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::UpdateBand()
{
	ASSERT_RETFAIL( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_FINALIZED ) );

	VECTOR vFoc;
	vFoc = m_vCenterPos_W;
	float z = vFoc.z;
	float fBandHeight = AppIsHellgate() ? AUTOMAP_BAND_HEIGHT : AUTOMAP_BAND_HEIGHT_MYTHOS;
	m_nTotalBands = AppIsHellgate() ? (int)CEIL( ( m_tMapBBox.vMax.z - m_tMapBBox.vMin.z ) / fBandHeight ) : 1;
	int nDetectedBand = (int)( ( z - m_tMapBBox.vMin.z ) / fBandHeight );
	nDetectedBand = MAX( nDetectedBand, NUM_BANDS / 2 );
	nDetectedBand = MIN( nDetectedBand, m_nTotalBands - 1 - (NUM_BANDS/2) );

	int nMinBand = AppIsHellgate() ? ( nDetectedBand - (NUM_BANDS/2) ) : 0;

	for ( int i = 0; i < NUM_BANDS; ++i )
		m_fBandMin[i] = (nMinBand + i) * fBandHeight + m_tMapBBox.vMin.z;

	float min = m_fBandMin[BAND_LOW];
	float max = m_fBandMin[BAND_HIGH] + fBandHeight;
	float f = AppIsHellgate() ? ( ( z - min ) / ( max - min ) ) : 0;
	//ASSERT( f >= 0.f && f <= 1.f );
	f = SATURATE(f);
	f *= (float)NUM_BANDS;

	float fMaxDist = 0.75f;
	float fSumRads = fMaxDist + 0.5f;		// max radius of influence plus half the scaled band height
	for ( int i = 0; i < NUM_BANDS; ++i )
	{
		float bavg = float(i) + 0.5f;
		float fDist = fabs( f - bavg );
		m_fBandWeights[i] = MAX( fSumRads - fDist, 0.f );
	}

	if ( nMinBand == m_nCurBandLow )
		return S_FALSE;

	// the center band has changed, so re-generate the texture
	m_nCurBandLow = nMinBand;
	SETBIT( m_dwFlagBits, FLAGBIT_RENDER_DIRTY, TRUE );

	return S_OK;
}

//----------------------------------------------------------------------------

static int sCompareAutomapVerts( const void * l, const void * r )
{
	const AUTOMAP_VERTEX * vl = static_cast<const AUTOMAP_VERTEX*>(l);
	const AUTOMAP_VERTEX * vr = static_cast<const AUTOMAP_VERTEX*>(r);

	if ( vl->z < vr->z )
		return -1;
	if ( vl->z > vr->z )
		return 1;
	return 0;
}

PRESULT AUTOMAP::Finalize()
{
#ifdef ENABLE_NEW_AUTOMAP


	// All triangles must have been added by now
	ASSERTX_RETFAIL( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_FINALIZED ), "Can't call finalize twice on the same automap!" );


	for ( int i = 0; i < NUM_BUFFERS; i++ )
	{
		ASSERTX_RETFAIL( ! dxC_VertexBufferD3DExists( m_tVBDef[i].nD3DBufferID[0] ), "Can't call finalize twice on the same automap!" );
		//ASSERTX_RETFAIL( ! dxC_IndexBufferD3DExists(  tIBDef[i].nD3DBufferID ), "Can't call finalize twice on the same automap!" );

		// resize this buffer down to its actual size
		if ( (unsigned)m_tVBDef[i].nBufferSize[0] > ( m_tVBDef[i].nVertexCount * sizeof(AUTOMAP_VERTEX) ) )
		{
			m_tVBDef[i].nBufferSize[0] = m_tVBDef[i].nVertexCount * sizeof(AUTOMAP_VERTEX);

			if ( m_tVBDef[i].nBufferSize[0] == 0 && m_tVBDef[i].pLocalData[0] )
			{
				FREE( NULL, m_tVBDef[i].pLocalData[0] );
				m_tVBDef[i].pLocalData[0] = NULL;
			}

			if ( m_tVBDef[i].pLocalData[0] )
			{
				m_tVBDef[i].pLocalData[0] = REALLOC( NULL, m_tVBDef[i].pLocalData[0], m_tVBDef[i].nBufferSize[0] );
			}
		}
		//if ( tIBDef[i].nBufferSize > ( tIBDef[i].nIndexCount * sizeof(AUTOMAP_INDEX_TYPE) ) )
		//{
		//	tIBDef[i].nBufferSize = tIBDef[i].nIndexCount * sizeof(AUTOMAP_INDEX_TYPE);
		//	tIBDef[i].pLocalData = REALLOC( NULL, tIBDef[i].pLocalData, tIBDef[i].nBufferSize );
		//}

		if ( ! m_tVBDef[i].pLocalData[0] )
			continue;


		// Sort the data to be depth-first (for proper rendering)
		// Sort by triangles, and not just vertices!
		int nTriCount = m_tVBDef[i].nVertexCount / 3;
		size_t nSize = sizeof(AUTOMAP_VERTEX) * 3;
		qsort( m_tVBDef[i].pLocalData[0], nTriCount, nSize, sCompareAutomapVerts );


		V( dxC_CreateVertexBuffer( 0, m_tVBDef[i], m_tVBDef[i].pLocalData[0] ) );
		//V( dxC_CreateIndexBuffer( tIBDef[i] ) );
	}

	RENDER_TARGET_INDEX nRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_AUTOMAP );

	int nRTW, nRTH;
	V( dxC_GetRenderTargetDimensions( nRTW, nRTH, nRT ) );


	ONCE
	{
		ASSERT_BREAK( ! dxC_VertexBufferD3DExists( m_tRenderVBDef.nD3DBufferID[0] ) );

		float min = -1;
		float max = 1;
		// Create the render display quad
		AUTOMAP_DISPLAY_VERTEX tVerts[4] =
		{
			{ VECTOR3( min, max, 0.f ), VECTOR2( 0.f, 0.f ) },
			{ VECTOR3( min, min, 0.f ), VECTOR2( 0.f, 1.f ) },
			{ VECTOR3( max, max, 0.f ), VECTOR2( 1.f, 0.f ) },
			{ VECTOR3( max, min, 0.f ), VECTOR2( 1.f, 1.f ) },
		};
		m_tRenderVBDef.nVertexCount = 4;
		m_tRenderVBDef.nBufferSize[0] = m_tRenderVBDef.nVertexCount * sizeof(AUTOMAP_DISPLAY_VERTEX);
		V( dxC_CreateVertexBuffer( 0, m_tRenderVBDef, tVerts ) );
	}	


	// build matrix transformation from world space to viewport space
	MATRIX mView;
	VECTOR vCenter = ( m_tMapBBox.vMin + m_tMapBBox.vMax ) * 0.5f;
	VECTOR vFrom( vCenter.x, vCenter.y, m_tMapBBox.vMax.z + 1.f );		// + fudge
	VECTOR vTo  ( vCenter.x, vCenter.y, m_tMapBBox.vMin.z );
	VECTOR vUp  ( 0.f, -1.f, 0.f );
	MatrixMakeView( mView, vFrom, vTo, vUp );
	MATRIX mProj;
	float dx = m_tMapBBox.vMax.x - m_tMapBBox.vMin.x;
	float dy = m_tMapBBox.vMax.y - m_tMapBBox.vMin.y;
	float dz = m_tMapBBox.vMax.z - m_tMapBBox.vMin.z;
	float fScale = MAX( dx, dy );
	float fDepth = dz + 1.f; // + fudge


	//// bias and scale to get from 0..1 to actual world
	//vGeneratedBias = tMapBBox.vMin;
	//vGeneratedScale = VECTOR( dx, dy, dz );


	m_fMetersPerPixel = fScale / MAX(nRTW, nRTH);
	m_fMetersSpan = fScale;
	fScale += AUTOMAP_EDGE_BUFFER * 2 * m_fMetersPerPixel;	// convert pixel buffer to a meter buffer
	MatrixMakeOrthoProj( mProj, fScale, fScale, 0.f, fDepth );
	MatrixMultiply( &m_mWorldToVP, &mView, &mProj );


	V( m_tFogOfWar.SetDimensions( m_tMapBBox ) );


	m_nCurBandLow = -1;
	SETBIT( m_dwFlagBits, FLAGBIT_FINALIZED, TRUE );
	SETBIT( m_dwFlagBits, FLAGBIT_RENDER_DIRTY, TRUE );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::SetShowAll( BOOL bShowAll )
{
#ifdef ENABLE_NEW_AUTOMAP


	return m_tFogOfWar.SetShowAll( bShowAll );

#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::RenderGenerate()
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( dxC_IsPixomaticActive() )
		return E_NOTIMPL;


	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_FINALIZED ) )
	{
		V_RETURN( Finalize() );
		//return S_FALSE;
	}

	V_RETURN( UpdateBand() );

	V_RETURN( m_tFogOfWar.Update() );
	V_RETURN( m_tFogOfWar.RenderGenerate() );

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_RENDER_DIRTY ) )
		return S_FALSE;


	VECTOR4 vBandMin( 0,0,0,0 );
	VECTOR4 vBandMax( 0,0,0,0 );
	ASSERT_RETFAIL( NUM_BANDS <= 4);
	for ( int i = 0; i < NUM_BANDS; ++i )
	{
		vBandMin[i] = m_fBandMin[i];
		vBandMax[i] = m_fBandMin[i] + ( AppIsHellgate() ? AUTOMAP_BAND_HEIGHT : AUTOMAP_BAND_HEIGHT_MYTHOS );
	}


	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_AUTOMAP_GENERATE );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
	ASSERT_RETFAIL( ptTechnique );
	UINT uPasses;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &uPasses ) );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	int nRTW, nRTH;
	RENDER_TARGET_INDEX nRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_AUTOMAP );
	V( dxC_GetRenderTargetDimensions( nRTW, nRTH, nRT ) );

	// On low resolutions, like 512x512, the highest zoom looks too blurry with MSAA turned on.
	BOOL bMSAA = !!( m_fScaleFactor < AUTOMAP_MAX_SCALE_FOR_MSAA );
	if ( MIN( nRTW, nRTH ) > AUTOMAP_MIN_RT_RES_FOR_ALWAYS_MSAA )
		bMSAA = TRUE;

	if ( bMSAA )
	{
		V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, DEPTH_TARGET_NONE ) );
	}
	else
	{
		//RENDER_TARGET_INDEX nRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_AUTOMAP );
		V_RETURN( dxC_SetRenderTargetWithDepthStencil( GLOBAL_RT_AUTOMAP, DEPTH_TARGET_NONE ) );
	}
	V( dxC_SetViewport( 0, 0, nRTW, nRTH, 0.f, 1.f ) );
	V( dxC_SetScissorRect( NULL ) );
	V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET, 0 ) );

	DWORD dwColors[ NUM_BUFFERS ] =
	{
		ARGB_MAKE_FROM_INT( 0,0,0, AUTOMAP_FLOOR_ALPHA ),
		ARGB_MAKE_FROM_INT( 0,0,0, AppIsHellgate() ? AUTOMAP_WALL_ALPHA : AUTOMAP_WALL_ALPHA_MYTHOS  ),
	};

	for ( int i = 0; i < NUM_BUFFERS; ++i )
	{
		if ( ! dxC_VertexBufferD3DExists( m_tVBDef[i].nD3DBufferID[0] ) )
			continue;

		V_CONTINUE( dxC_SetStreamSource( m_tVBDef[i], 0, pEffect ) );

		dxC_SetRenderState( D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
		dxC_SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE );
		dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
		dxC_SetRenderState( D3DRS_BLENDOP,			D3DBLENDOP_MAX );
		dxC_SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_DESTBLEND,		D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_ZENABLE,			FALSE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE,		FALSE );
		dxC_SetRenderState( D3DRS_CULLMODE,			D3DCULL_NONE );

		dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_FOGCOLOR, dwColors[i] );

		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_BIAS,  &vBandMin );
		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_SCALE, &vBandMax );

		dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX,	&m_mWorldToVP );

		V( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, 0, m_tVBDef[i].nVertexCount / 3, pEffect, METRICS_GROUP_UI, 0 ) );
	}

	if ( bMSAA )
	{
		RENDER_TARGET_INDEX nRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_AUTOMAP );
		SPD3DCTEXTURE2D & tDest = dxC_RTResourceGet( nRT );
		E_RECT tRect;
		tRect.Set( 0, 0, nRTW, nRTH );
		V( dxC_CopyBackbufferToTexture( tDest, (const RECT*)&tRect, (const RECT*)&tRect ) );
	}

	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
	V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eDT ) );

	SETBIT( m_dwFlagBits, FLAGBIT_RENDER_DIRTY, FALSE );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::SetDisplay( const VECTOR * pvPos, const MATRIX * pmRotate, float * pfScale )
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( pvPos )
	{
		m_vCenterPos_W	= *pvPos;
		m_vCenterPos_W.z += AUTOMAP_CENTER_POSITION_Z_OFFSET;
		V_RETURN( m_tFogOfWar.SetDisplay( m_vCenterPos_W, FOW_REVEAL_XY_RADIUS ) );
	}

	if ( pmRotate )
		m_mRotate		= *pmRotate;

	if ( pfScale )
		m_fScaleFactor	= *pfScale;

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::GetItemPos( const VECTOR & vWorldPos, VECTOR & vNewPos, const E_RECT & tRect )
{
#ifdef ENABLE_NEW_AUTOMAP

	VECTOR vRel;
	vRel.x = vWorldPos.x - m_vCenterPos_W.x;
	vRel.y = vWorldPos.y - m_vCenterPos_W.y;
	vRel.z = vWorldPos.z - m_vCenterPos_W.z;
	MatrixMultiply( &vNewPos, &vRel, &m_mRotate );

	int nRTW, nRTH;
	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER );
	V( dxC_GetRenderTargetDimensions( nRTW, nRTH, nRT ) );
	int nRW = tRect.right - tRect.left;
	int nRH = tRect.bottom - tRect.top;
	VECTOR vImplicitScale( AppIsHellgate() ? ( (float)nRW / nRTW ) : 1, 
						   AppIsHellgate() ? ( (float)nRH / nRTH ) : 1, 1.f );

	vNewPos.x *= m_fScaleFactor * vImplicitScale.x;
	vNewPos.y *= m_fScaleFactor * vImplicitScale.y;

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT AUTOMAP::RenderDisplay(
	const struct E_RECT & tRect,
	DWORD dwColor,
	float fZFinal,
	float fScale )
{
#ifdef ENABLE_NEW_AUTOMAP


	// Unused.
	REF(fScale);

	if ( ! e_GetRenderFlag( RENDER_FLAG_AUTOMAP ) )
		return S_FALSE;

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_FINALIZED ) )
	{
		return S_FALSE;
	}

	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_AUTOMAP_RENDER );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
	ASSERT_RETFAIL( ptTechnique );
	UINT uPasses;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &uPasses ) );


	VECTOR4 vWeights( 0,0,0,0 );
	ASSERT_RETFAIL( NUM_BANDS <= 4 );
	for ( int i = 0; i < NUM_BANDS; ++i )
	{
		vWeights[i] = m_fBandWeights[i];
	}


	// Note: Bigger scale means more zoomed in.

	// Bias first, then scale.
	VECTOR vFoc;
	MatrixMultiply( &vFoc, &m_vCenterPos_W, &m_mWorldToVP );
	VECTOR4 vBias(0,0,0,0);
	vBias.x = -vFoc.x;
	vBias.y = -vFoc.y;
	vBias.z = fZFinal;
	vBias.w = fZFinal;

	int nRW = tRect.right - tRect.left;
	int nRH = tRect.bottom - tRect.top;
	VECTOR vImplicitScale( m_fMetersSpan / nRW, m_fMetersSpan / nRH, 1.f );
	VECTOR4 vScale(
		vImplicitScale.x * m_fScaleFactor,
		vImplicitScale.y * m_fScaleFactor,
		m_tFogOfWar.GetMappedCurrentHeight(), 1 );

	// Map the automap RT's texture coverage to the fogofwar RT's texture coverage
	VECTOR4 vUVBiasScale;
	VECTOR vMinProj, vMaxProj;
	MatrixMultiply( &vMinProj, &m_tMapBBox.vMin, &m_mWorldToVP );
	MatrixMultiply( &vMaxProj, &m_tMapBBox.vMax, &m_mWorldToVP );
	vMinProj.x = vMinProj.x *  0.5f + 0.5f;
	vMinProj.y = vMinProj.y * -0.5f + 0.5f;
	vMaxProj.x = vMaxProj.x *  0.5f + 0.5f;
	vMaxProj.y = vMaxProj.y * -0.5f + 0.5f;
	vUVBiasScale.x = -vMinProj.x;
	vUVBiasScale.y = -vMinProj.y;
	vUVBiasScale.z = 1.f / fabs(vMaxProj.x - vMinProj.x);
	vUVBiasScale.w = 1.f / fabs(vMaxProj.y - vMinProj.y);

	//V( dxC_SetScissorRect( (const RECT*)&tRect ) );
	V( dxC_SetViewport( tRect.left, tRect.top, tRect.right - tRect.left, tRect.bottom - tRect.top, 0.f, 1.f ) );

	for ( UINT uPass = 0; uPass < uPasses; ++uPass )
	{
		if ( ! dxC_VertexBufferD3DExists( m_tRenderVBDef.nD3DBufferID[0] ) )
			continue;

		V_RETURN( dxC_EffectBeginPass( pEffect, uPass ) );

		V_CONTINUE( dxC_SetStreamSource( m_tRenderVBDef, 0, pEffect ) );

		dxC_SetRenderState( D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
		dxC_SetRenderState( D3DRS_ALPHATESTENABLE,	TRUE );
		dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
		dxC_SetRenderState( D3DRS_ZENABLE,			D3DZB_TRUE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE,		FALSE );
		dxC_SetRenderState( D3DRS_BLENDOP,			D3DBLENDOP_ADD );
		if ( uPass == 0 )
		{
			dxC_SetRenderState( D3DRS_SRCBLEND,		D3DBLEND_SRCALPHA );
			dxC_SetRenderState( D3DRS_DESTBLEND,	D3DBLEND_INVSRCALPHA );
		}
		else
		{
			dxC_SetRenderState( D3DRS_SRCBLEND,		D3DBLEND_ONE );
			dxC_SetRenderState( D3DRS_DESTBLEND,	D3DBLEND_ONE );
		}
		dxC_SetRenderState( D3DRS_CULLMODE,			D3DCULL_NONE );

		dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_WORLDMATRIX, &m_mRotate );

		RENDER_TARGET_INDEX nAutomapRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_AUTOMAP );
		dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, dxC_RTShaderResourceGet( nAutomapRT ) );
		RENDER_TARGET_INDEX nFoWRT = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FOGOFWAR );
		dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_LIGHTMAP,    dxC_RTShaderResourceGet( nFoWRT ) );

		dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_AUTOMAP_COLOR_SAME,   SAME );
		dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_AUTOMAP_COLOR_SHADOW, SHADOW );
		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_AUTOMAP_BAND_WEIGHTS, &vWeights );

		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_BIAS,  &vBias );
		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_SCALE, &vScale );

		dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_UV_SCALE, &vUVBiasScale );

		dxC_EffectSetScreenSize( pEffect, *ptTechnique );
		dxC_EffectSetPixelAccurateOffset( pEffect, *ptTechnique );

		V( dxC_DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2, pEffect, METRICS_GROUP_UI, 0 ) );
	}

	//V( dxC_SetScissorRect( NULL ) );
	V( dxC_ResetFullViewport() );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_AutomapCreate( int nRegion )
{
#ifdef ENABLE_NEW_AUTOMAP

	ASSERT_RETINVALIDARG( nRegion != INVALID_ID );

	//nID = gnNextAutomapID;
	//gnNextAutomapID++;
	HashAdd( gtAutomaps, nRegion );
	AUTOMAP * pMap = HashGet( gtAutomaps, nRegion );
	ASSERT_RETFAIL( pMap );
	pMap->Init( nRegion );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_AutomapDestroy( int nID )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;
	pMap->Free();
	HashRemove( gtAutomaps, nID );

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_AutomapAddTri(
	int nID,
	const VECTOR * pv1,
	const VECTOR * pv2,
	const VECTOR * pv3,
	BOOL bWall )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	if ( bWall )
	{
		ASSERT_RETFAIL( pv1 && pv2 );
		// build a fat line out of pv1 and pv2
		VECTOR L = *pv2 - *pv1;
		VectorNormalize( L );
		VECTOR S( L.y, -L.x, L.z );

		float fWidth_m = AUTOMAP_WALL_WIDTH_METERS * 0.5f;

		S *= fWidth_m;
		VECTOR v1a = *pv1 + S;
		VECTOR v1b = *pv1 - S;
		VECTOR v2a = *pv2 + S;
		VECTOR v2b = *pv2 - S;

		V_RETURN( pMap->AddTri( &v1a, &v1b, &v2a, TRUE ) );
		V_RETURN( pMap->AddTri( &v2a, &v2b, &v1b, TRUE ) );
		return S_OK;
	}
	else
	{
		return pMap->AddTri( pv1, pv2, pv3, FALSE );
	}
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapPrealloc(
	int nID,
	int nAddlTris )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	return pMap->Prealloc( AUTOMAP::GROUND, nAddlTris );
	return pMap->Prealloc( AUTOMAP::WALL,   nAddlTris );
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapRenderGenerate(
	int nID )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	return pMap->RenderGenerate();
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapRenderDisplay(
	int nID,
	const struct E_RECT & tRect,
	DWORD dwColor,
	float fZFinal,
	float fScale )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	return pMap->RenderDisplay( tRect, dwColor, fZFinal, fScale );
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapsMarkDirty()
{
#ifdef ENABLE_NEW_AUTOMAP


	for ( AUTOMAP * pMap = HashGetFirst(gtAutomaps);
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		pMap->SetDirty();
	}

#endif // ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_AutomapSetDisplay(
	const VECTOR * pvPos,
	const MATRIX * pmRotate,
	float * pfScale )
{
#ifdef ENABLE_NEW_AUTOMAP


	// Set the unit position for all automaps

	for ( AUTOMAP * pMap = HashGetFirst( gtAutomaps );
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		pMap->SetDisplay( pvPos, pmRotate, pfScale );
	}
	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapGetItemPos(
	int nID,
	const VECTOR & vWorldPos,
	VECTOR & vNewPos,
	const E_RECT & tRect )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	return pMap->GetItemPos( vWorldPos, vNewPos, tRect );
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT e_AutomapSetShowAll(
	int nID,
	BOOL bShowAll )
{
#ifdef ENABLE_NEW_AUTOMAP


	AUTOMAP * pMap = HashGet( gtAutomaps, nID );
	if ( ! pMap )
		return S_FALSE;

	return pMap->SetShowAll( bShowAll );
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT dxC_AutomapDeviceLost()
{
#ifdef ENABLE_NEW_AUTOMAP

	PRESULT pr = S_OK;

	for ( AUTOMAP * pMap = HashGetFirst( gtAutomaps );
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		V_SAVE_ERROR( pr, pMap->m_tFogOfWar.DeviceLost() );
	}

	return pr;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT dxC_AutomapDeviceReset()
{
#ifdef ENABLE_NEW_AUTOMAP


	PRESULT pr = S_OK;

	for ( AUTOMAP * pMap = HashGetFirst( gtAutomaps );
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		V_SAVE_ERROR( pr, pMap->m_tFogOfWar.DeviceReset() );
	}

	return pr;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void FOG_OF_WAR::HideAll()
{
#ifdef ENABLE_NEW_AUTOMAP


	DWORD dwElements = m_pIndex.GetAllocElements();
	if ( dwElements > 0 )
		memset( (NODE_INDEX*)m_pIndex, INVALID_NODE, sizeof(NODE_INDEX) * dwElements );
	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );
	SETBIT( m_dwFlagBits, FLAGBIT_DISPLAY_SET, FALSE );
	m_nNodesMapped = 0;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

void FOG_OF_WAR::ShowAll()
{
#ifdef ENABLE_NEW_AUTOMAP


	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) )
		return;

	VECTOR vCenter = (m_tBBox.vMin + m_tBBox.vMax) * 0.5f;
	VECTOR vDiff = m_tBBox.vMax - m_tBBox.vMin;
	float fDiag = VectorLength( vDiff );
	V( MarkVisibleInRange( vCenter, fDiag ) );

	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );
	SETBIT( m_dwFlagBits, FLAGBIT_ALWAYS_SHOW_ALL, TRUE );
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::SetShowAll( BOOL bShowAll )
{
#ifdef ENABLE_NEW_AUTOMAP


	if ( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_ALWAYS_SHOW_ALL ) == bShowAll )
		return S_OK;

	if ( bShowAll )
	{
		ShowAll();
	}

	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::SetDimensions( const BOUNDING_BOX & tBBox )
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) )
		return S_FALSE;

	m_tBBox = tBBox;
	VECTOR vDiff = tBBox.vMax - tBBox.vMin;
	m_fHeightBias  = -tBBox.vMin.z;
	m_fHeightScale = 1.f / vDiff.z * 255.f;
	m_nHeightRevealRad = static_cast<HEIGHT>(CEIL(m_fHeightScale * FOW_REVEAL_HEIGHT_RADIUS));
	m_nXRes = ROUND( vDiff.x / FOW_NODE_SIZE );
	m_nYRes = ROUND( vDiff.y / FOW_NODE_SIZE );
	m_nZRes = 256;
	m_vBias.x = -tBBox.vMin.x;
	m_vBias.y = -tBBox.vMin.y;
	if( m_nXRes != 0 && m_nYRes != 0 )
	{
		m_vScale.x = 1.f / vDiff.x * m_nXRes;
		m_vScale.y = 1.f / vDiff.y * m_nYRes;
	}
	else
	{
		m_vScale.x = 0;
		m_vScale.y = 0;
	}


	V( m_pIndex.Resize( m_nXRes * m_nYRes ) );
	V( m_pNodes.Resize( m_nXRes * m_nYRes ) );

	SETBIT( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET, TRUE );
	HideAll();

	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

void FOG_OF_WAR::MarkNode( NODE * pNode, HEIGHT nZmax, HEIGHT nZmin )
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( pNode->nMax >= nZmax && pNode->nMin <= nZmin )
	{
		// Up-to-date, just exit.
		return;
	}

	pNode->nMax = MAX( pNode->nMax, nZmax );
	pNode->nMin = MIN( pNode->nMin, nZmin );

	// mark the node for upload

	// ???

	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );

#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::MarkVisibleInRange( const VECTOR& vPos, float fRadius )
{
#ifdef ENABLE_NEW_AUTOMAP


	int Xrad = fRadius * m_vScale.x;
	int Yrad = fRadius * m_vScale.y;
	Xrad = MAX( 1, Xrad );
	Yrad = MAX( 1, Yrad );
	int Xc, Yc;
	GetNodeXY( vPos, Xc, Yc );
	int Xmin = MAX( 0, Xc - Xrad );
	int Ymin = MAX( 0, Yc - Yrad );
	int Xmax = MIN( m_nXRes, Xc + Xrad );
	int Ymax = MIN( m_nYRes, Yc + Yrad );
	int rsq = Xrad * Xrad + Yrad * Yrad;

	HEIGHT nZ = GetMappedHeight( vPos.z );
	HEIGHT nZmin = nZ - MIN( m_nHeightRevealRad, nZ );
	HEIGHT ZHIGH = (m_nZRes-1 - nZ);
	HEIGHT nZmax = nZ + MIN( m_nHeightRevealRad, ZHIGH );

	for ( int y = Ymin; y < Ymax; ++y )
	{
		int Yd = y - Yc;
		Yd *= Yd;

		for ( int x = Xmin; x < Xmax; ++x )
		{
			int Xd = x - Xc;
			Xd *= Xd;
			// 2D distance check
			if ( (Yd + Xd) > rsq )
				continue;

			// make sure this node is visible at the current height
			NODE_INDEX * pnIndex = GetNodeIndex( x, y );
			ASSERT_CONTINUE( pnIndex );
			NODE * pNode = GetNode( *pnIndex );
			if ( ! pNode )
				pNode = AddNode( x, y );
			ASSERT_CONTINUE( pNode );
			MarkNode( pNode, nZmax, nZmin );
		}
	}

	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::SetDisplay( const VECTOR & vPos, float fRadius )
{
	m_vPosition = vPos;
	m_fXYRevealRadius = fRadius;
	SETBIT( m_dwFlagBits, FLAGBIT_DISPLAY_SET, TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::Update()
{
#ifdef ENABLE_NEW_AUTOMAP


	RestoreMaterial();

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DISPLAY_SET ) )
		return S_OK;

	V_RETURN( MarkVisibleInRange( m_vPosition, m_fXYRevealRadius ) );
	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::DeviceLost()
{
	for ( int i = 0; i < DXC_MAX_VERTEX_STREAMS; ++i )
	{
		if ( dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[i] ) )
		{
			V( dxC_VertexBufferDestroy( m_tVBDef.nD3DBufferID[i] ) );
		}
	}

	if ( dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
	{
		V( dxC_IndexBufferDestroy( m_tIBDef.nD3DBufferID ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::DeviceReset()
{
	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );
	SETBIT( m_dwFlagBits, FLAGBIT_DISPLAY_SET, FALSE );
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::FillNodeVerts( int nX, int nY, const NODE * pNode, FOGOFWAR_VERTEX * pVerts )
{
#ifdef ENABLE_NEW_AUTOMAP

	// Pos
	// Find bordering node indexes

	//int nL = max( nX - 1, 0 );
	//int nU = max( nY - 1, 0 );
	//int nR = min( nX + 1, m_nXRes - 1 );
	//int nB = min( nY + 1, m_nYRes - 1 );

	int nL = nX - FOW_NODE_COVERAGE_RADIUS;
	int nU = nY - FOW_NODE_COVERAGE_RADIUS;
	int nR = nX + FOW_NODE_COVERAGE_RADIUS;
	int nB = nY + FOW_NODE_COVERAGE_RADIUS;

	GetNodeScreen( nL, nU, pVerts[0].pos );		pVerts[0].pos.z = 0.f;
	GetNodeScreen( nR, nU, pVerts[1].pos );		pVerts[1].pos.z = 0.f;
	GetNodeScreen( nL, nB, pVerts[2].pos );		pVerts[2].pos.z = 0.f;
	GetNodeScreen( nR, nB, pVerts[3].pos );		pVerts[3].pos.z = 0.f;

	// Cols
	DWORD dwCol = ARGB_MAKE_FROM_INT( pNode->nMax, (pNode->nMax - pNode->nMin), 0, 0 );
	pVerts[0].col = dwCol;
	pVerts[1].col = dwCol;
	pVerts[2].col = dwCol;
	pVerts[3].col = dwCol;

	// UVs
	pVerts[0].uv.x = 0.f;		pVerts[0].uv.y = 0.f;
	pVerts[1].uv.x = 1.f;		pVerts[1].uv.y = 0.f;
	pVerts[2].uv.x = 0.f;		pVerts[2].uv.y = 1.f;
	pVerts[3].uv.x = 1.f;		pVerts[3].uv.y = 1.f;

#endif	// ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::FillNodeVerts( int nIter, const NODE * pNode, FOGOFWAR_VERTEX * pVerts )
{
	int nX = nIter % m_nXRes;
	int nY = nIter / m_nXRes;

	return FillNodeVerts( nX, nY, pNode, pVerts );
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::PrepareBuffers()
{
#ifdef ENABLE_NEW_AUTOMAP
	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) )
		return S_OK;
	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DISPLAY_SET ) )
		return S_OK;
	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE ) )
		return S_OK;



	int nVertCount = m_nNodesMapped * 4;
	int nVBSize = nVertCount * sizeof(FOGOFWAR_VERTEX);
	if ( nVBSize == 0 )
		return S_FALSE;
	int nIndexCount = nVertCount / 4 * 6;
	int nIBSize = nIndexCount * sizeof(FOGOFWAR_INDEX);
	if ( nIBSize == 0 )
		return S_FALSE;


	// create vertex list
	BOOL bRealloc = FALSE;
	if ( ! m_tVBDef.pLocalData[0] )
		bRealloc = TRUE;
	if ( m_tVBDef.nBufferSize[0] < nVBSize )
		bRealloc = TRUE;
	if ( bRealloc )
	{
		m_tVBDef.pLocalData[0] = REALLOC( NULL, m_tVBDef.pLocalData[0], nVBSize );
		ASSERT_RETVAL( m_tVBDef.pLocalData[0], E_OUTOFMEMORY );
		m_tVBDef.nBufferSize[0] = nVBSize;
	}
	if ( bRealloc || ! dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[0] ) )
	{
		if ( dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[0] ) )
		{
			V( dxC_VertexBufferDestroy( m_tVBDef.nD3DBufferID[0] ) );
		}
		if ( m_tVBDef.nBufferSize[0] > 0 )
		{
			ASSERT_RETFAIL( ! dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[0] ) );
			m_tVBDef.tUsage = D3DC_USAGE_BUFFER_MUTABLE;
			m_tVBDef.eVertexType = VERTEX_DECL_XYZ_COL_UV;
			V_RETURN( dxC_CreateVertexBuffer( 0, m_tVBDef ) );
		}
	}
	int nVert = 0;
	ASSERT_RETFAIL( (unsigned)m_nNodesMapped <= m_pNodes.GetAllocElements() );
	for ( int y = 0; y < m_nYRes; ++y )
	{
		for ( int x = 0; x < m_nXRes; ++x )
		{
			NODE_INDEX * pnIndex = GetNodeIndex( x, y );
			ASSERT_CONTINUE( pnIndex );
			if ( *pnIndex == INVALID_NODE )
				continue;
			NODE* pNode = GetNode( *pnIndex );
			ASSERT_CONTINUE(pNode);

			ASSERT_BREAK( (nVert + 4) <= nVertCount );
			FOGOFWAR_VERTEX * pVerts = ((FOGOFWAR_VERTEX*)m_tVBDef.pLocalData[0]) + nVert;
			V_CONTINUE( FillNodeVerts( x, y, pNode, pVerts ) );
			nVert += 4;
		}
	}

	{
		if ( ! dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[0] ) )
			return E_FAIL;

		m_tVBDef.nVertexCount = nVert;
		V_RETURN( dxC_UpdateBuffer( 0, m_tVBDef, m_tVBDef.pLocalData[0], 0, m_tVBDef.nBufferSize[0], TRUE ) );
	}



	// create index list
	bRealloc = FALSE;
	if ( ! m_tIBDef.pLocalData )
		bRealloc = TRUE;
	if ( m_tIBDef.nBufferSize < nIBSize )
		bRealloc = TRUE;
	if ( bRealloc )
	{
		m_tIBDef.pLocalData = REALLOC( NULL, m_tIBDef.pLocalData, nIBSize );
		ASSERT_RETVAL( m_tIBDef.pLocalData, E_OUTOFMEMORY );
		m_tIBDef.nBufferSize = nIBSize;
	}
	if ( bRealloc || ! dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
	{
		if ( dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
		{
			V( dxC_IndexBufferDestroy( m_tIBDef.nD3DBufferID ) );
		}
		if ( m_tIBDef.nBufferSize > 0 )
		{
			ASSERT_RETFAIL( ! dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) );
			m_tIBDef.tFormat = D3DFMT_INDEX16;
			m_tIBDef.tUsage = D3DC_USAGE_BUFFER_MUTABLE;
			V_RETURN( dxC_CreateIndexBuffer( m_tIBDef ) );
		}
	}
	int nQuadCount = nIndexCount / 6;
	int nIndex = 0;
	for ( int nQuad = 0; nQuad < nQuadCount; ++nQuad )
	{
		FOGOFWAR_INDEX * pInds = &((FOGOFWAR_INDEX*)m_tIBDef.pLocalData)[nIndex];
		unsigned short nBase = nQuad * 4;
		pInds[0] = nBase + 0;
		pInds[1] = nBase + 1;
		pInds[2] = nBase + 2;
		pInds[3] = nBase + 2;
		pInds[4] = nBase + 1;
		pInds[5] = nBase + 3;
		nIndex += 6;
	}
	{
		if ( ! dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
			return E_FAIL;

		m_tIBDef.nIndexCount = nIndex;
		V_RETURN( dxC_UpdateBuffer( m_tIBDef, m_tIBDef.pLocalData, 0, m_tIBDef.nBufferSize, TRUE ) );
	}


#endif // ENABLE_NEW_AUTOMAP
	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::RenderGenerate()
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) )
		return S_OK;

	V_RETURN( PrepareBuffers() );

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE ) )
		return S_FALSE;

	if ( ! dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[0] ) )
		return S_FALSE;
	if ( ! dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
		return S_FALSE;

	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_AUTOMAP_GENERATE );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 1;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
	ASSERT_RETFAIL( ptTechnique );
	UINT uPasses;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &uPasses ) );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	D3D_TEXTURE * pRevealTex = dxC_TextureGet( m_nRevealTexture );
	ASSERT_RETFAIL( pRevealTex );

	V_RETURN( dxC_SetRenderTargetWithDepthStencil( GLOBAL_RT_FOGOFWAR, DEPTH_TARGET_NONE ) );

	V( dxC_ResetFullViewport() );
	V( dxC_SetScissorRect( NULL ) );
	V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET, 0 ) );

	V_RETURN( dxC_SetStreamSource( m_tVBDef, 0, pEffect ) );
	V_RETURN( dxC_SetIndices( m_tIBDef, TRUE ) );

	dxC_SetRenderState( D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE );
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
	dxC_SetRenderState( D3DRS_BLENDOP,			D3DBLENDOP_MAX );
	dxC_SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND,		D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_ZENABLE,			FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE,		FALSE );
	dxC_SetRenderState( D3DRS_CULLMODE,			D3DCULL_NONE );

	//dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_FOGCOLOR, dwColors[i] );

	//dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_BIAS,  &vBandMin );
	//dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_SCALE, &vBandMax );

	//dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX,	&m_mWorldToVP );

	V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, pRevealTex->GetShaderResourceView() ) );

	V( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_tVBDef.nVertexCount, 0, m_tIBDef.nIndexCount / 3, pEffect, METRICS_GROUP_UI, 0 ) );


	dxC_SetRenderState( D3DRS_ZENABLE,			TRUE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE,		TRUE );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE );
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
	V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eDT ) );
	V( dxC_SetScissorRect( NULL ) );


	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, FALSE );

	return S_OK;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

void FOG_OF_WAR::RestoreMaterial()
{
#ifdef ENABLE_NEW_AUTOMAP
	if ( m_nMaterial == INVALID_ID )
		m_nMaterial = e_GetGlobalMaterial( ExcelGetLineByStrKey( DATATABLE_MATERIALS_GLOBAL, "FogOfWar" ) );

	// This could be before the global materials have been loaded.
	if ( m_nMaterial != INVALID_ID && m_nRevealTexture == INVALID_ID )
	{
		MATERIAL * pMaterial = (MATERIAL *)DefinitionGetById( DEFINITION_GROUP_MATERIAL, m_nMaterial );
		ASSERT_RETURN( pMaterial );
		if ( ! e_TextureIsValidID( pMaterial->nOverrideTextureID[ TEXTURE_DIFFUSE ] ) )
		{
			V( e_MaterialRestore( m_nMaterial ) );
		}
		m_nRevealTexture = pMaterial->nOverrideTextureID[ TEXTURE_DIFFUSE ];
	}
#endif
}

//----------------------------------------------------------------------------

float FOG_OF_WAR::GetMappedCurrentHeight()
{
#ifdef ENABLE_NEW_AUTOMAP
	HEIGHT nZ = GetMappedHeight( m_vPosition.z );
	return nZ / 255.f;
#else
	return 0.f;
#endif
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::Save( WRITE_BUF_DYNAMIC & fbuf )
{
#ifdef ENABLE_NEW_AUTOMAP

	if ( ! TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) )
	{
		ASSERT_RETFAIL(fbuf.PushInt(0));
		return S_FALSE;
	}
	ASSERT_RETFAIL(fbuf.PushInt(1));

	ASSERT_RETFAIL(fbuf.PushBuf(&m_tBBox, sizeof(BOUNDING_BOX)));

	ASSERT_RETFAIL(fbuf.PushInt((DWORD)m_nNodesMapped));

	ASSERT_RETFAIL( (DWORD)m_nNodesMapped <= m_pNodes.GetAllocElements() );

	int nInds = m_pIndex.GetAllocElements();
	for ( int i = 0; i < nInds; ++i )
	{
		NODE_INDEX nIndex = m_pIndex[ i ];
		if ( nIndex == INVALID_NODE )
			continue;

		ASSERT_CONTINUE( (DWORD)nIndex < m_pNodes.GetAllocElements() );
		NODE * pNode = &m_pNodes[ nIndex ];
		int nX = i % m_nXRes;
		int nY = i / m_nXRes;
		ASSERT_RETFAIL(fbuf.PushInt((DWORD)nX));
		ASSERT_RETFAIL(fbuf.PushInt((DWORD)nY));
		ASSERT_RETFAIL(fbuf.PushInt((DWORD)pNode->nMax));
		ASSERT_RETFAIL(fbuf.PushInt((DWORD)pNode->nMin));
	}

	return S_OK;
#else
	return S_FALSE;
#endif
}

//----------------------------------------------------------------------------

PRESULT e_AutomapSave(
	WRITE_BUF_DYNAMIC & fbuf )
{
#ifdef ENABLE_NEW_AUTOMAP

	PRESULT pr = S_OK;

	ASSERT_RETFAIL(fbuf.PushInt((DWORD)AUTOMAP_LEVEL_MAGIC));
	ASSERT_RETFAIL(fbuf.PushInt((DWORD)HashGetCount(gtAutomaps)));

	for ( AUTOMAP * pMap = HashGetFirst( gtAutomaps );
		pMap;
		pMap = HashGetNext( gtAutomaps, pMap ) )
	{
		ASSERT( pMap->nId == pMap->m_tFogOfWar.m_nRegionID );
		ASSERT( pMap->nId != INVALID_ID );
		ASSERT_RETFAIL(fbuf.PushInt((DWORD)pMap->m_tFogOfWar.m_nRegionID));

		ASSERT_RETFAIL(fbuf.PushInt((DWORD)AUTOMAP_REGION_MAGIC));

		V_SAVE_ERROR( pr, pMap->m_tFogOfWar.Save( fbuf ) );
	}

	return pr;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

//----------------------------------------------------------------------------

PRESULT FOG_OF_WAR::Load( BYTE_BUF & buf )
{
#ifdef ENABLE_NEW_AUTOMAP

	DWORD dw;
	ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)));
	if ( dw == 0 )
		return S_OK;
	ASSERT_RETFAIL( dw == 1 );

	BOUNDING_BOX tBBox;
	ASSERT_RETFAIL(buf.PopBuf(&tBBox, sizeof(BOUNDING_BOX)));

	SETBIT( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET, FALSE );
	V( SetDimensions( tBBox ) );

	int nNodes;
	ASSERT_RETFAIL(buf.PopInt(&nNodes, sizeof(nNodes)));
	for ( int i = 0; i < nNodes; ++i )
	{
		int nX, nY;
		ASSERT_RETFAIL(buf.PopInt(&nX, sizeof(nX)));
		ASSERT_RETFAIL(buf.PopInt(&nY, sizeof(nY)));

		NODE * pNode = AddNode( nX, nY );
		ASSERT_RETFAIL( pNode );

		ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)));
		pNode->nMax = (HEIGHT)dw;
		ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)));
		pNode->nMin = (HEIGHT)dw;
	}

	SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );

	return S_OK;
#else
	return S_FALSE;
#endif
}

//----------------------------------------------------------------------------

PRESULT e_AutomapLoad(
	BYTE_BUF & buf )
{
#ifdef ENABLE_NEW_AUTOMAP

	PRESULT pr = S_OK;

	DWORD dw;
	ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)) && dw == AUTOMAP_LEVEL_MAGIC);
	int nMaps;
	ASSERT_RETFAIL(buf.PopInt(&nMaps, sizeof(nMaps)));

	ASSERT_RETFAIL( nMaps == HashGetCount(gtAutomaps) );

	for ( int i = 0; i < nMaps; ++i )
	{
		int nMap;
		ASSERT_RETFAIL(buf.PopInt(&nMap, sizeof(nMap)));
		ASSERTV_RETFAIL( IS_VALID_INDEX( nMap, e_RegionGetCount() ), "Unknown region %d found in automap load (max %d)!", nMap, e_RegionGetCount() );

		AUTOMAP * pMap = HashGet( gtAutomaps, nMap );
		ASSERTV_CONTINUE( pMap, "No automap hash found for region %d!", nMap );

		ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)) && dw == AUTOMAP_REGION_MAGIC);

		V_SAVE_ERROR( pr, pMap->m_tFogOfWar.Load( buf ) );
	}

	//for ( AUTOMAP * pMap = HashGetFirst( gtAutomaps );
	//	pMap;
	//	pMap = HashGetNext( gtAutomaps, pMap ) )
	//{
	//	ASSERT_RETFAIL(buf.PopUInt(&dw, sizeof(dw)) && dw == AUTOMAP_REGION_MAGIC);

	//	V_SAVE_ERROR( pr, pMap->m_tFogOfWar.Load( buf ) );
	//}

	return pr;
#else
	return S_FALSE;
#endif	// ENABLE_NEW_AUTOMAP
}

};	// FSSE