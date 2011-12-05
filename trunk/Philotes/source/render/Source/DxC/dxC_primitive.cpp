//----------------------------------------------------------------------------
// dx9_primitive.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_state.h"
#include "dxC_main.h"
#include "dxC_target.h"

#include "dxC_primitive.h"
#include "dxC_buffer.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define PRIM_MAX_LINES					32
#define PRIM_MAX_TRIS					32
#define	PRIM_CIRCLE_VERTS				32

#define SPHERE_LOD_DIST_RAD_RATIO_2		13.f
#define SPHERE_LOD_DIST_RAD_RATIO_3		4.f

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DEBUG_PRIMITIVE_SCREEN_VERTEX
{
	D3DXVECTOR3		vPos;				// screenspace position
	float			fRHW;
	D3DCOLOR		dwColor;			// diffuse color
};

struct DEBUG_PRIMITIVE_WORLD_VERTEX
{
	D3DXVECTOR3		vPos;				// world position
	D3DCOLOR		dwColor;			// diffuse color
};

struct UNIT_SPHERE
{
	// subs is number of subdivisions
	// tris is 8 * 4^subs
	int nSubs;
	int nTris;
	int nVerts;
	VECTOR * vSource;
	VECTOR * vTemp;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

D3D_INDEX_BUFFER_DEFINITION  gtBoundingBoxIndexBuffer;
D3D_VERTEX_BUFFER_DEFINITION gtBoundingBoxVertexBuffer;
PRIMITIVE_RENDER_LIST< DEBUG_PRIMITIVE_WORLD_VERTEX  >	gtPrimRenderList_XYZ[ NUM_PRIM_PASSES ][ NUM_PRIM_RENDER_TYPES ];
PRIMITIVE_RENDER_LIST< DEBUG_PRIMITIVE_SCREEN_VERTEX >	gtPrimRenderList_RHW[ NUM_PRIM_PASSES ][ NUM_PRIM_RENDER_TYPES ];

static UNIT_SPHERE sgtUnitSphere1 = {0};
static UNIT_SPHERE sgtUnitSphere2 = {0};
static UNIT_SPHERE sgtUnitSphere3 = {0};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
extern D3D_INDEX_BUFFER_DEFINITION  gt2DBoundingBoxIndexBuffer;
extern D3D_VERTEX_BUFFER_DEFINITION gt2DBoundingBoxVertexBuffer;

extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER>		gtPrimInfoLine;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER>		gtPrimInfoTri;

extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_2D_BOX>		gtPrimInfo2DBox;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_3D_BOX>		gtPrimInfo3DBox;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_SPHERE>		gtPrimInfoCircle;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_SPHERE>		gtPrimInfoSphere;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_CYLINDER>	gtPrimInfoCylinder;
extern SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_CONE>		gtPrimInfoCone;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static void sSetBoxVertices( BOUNDING_BOX_VERTEX * ptVerts )
{
	//	 0	 1
	//	4	5
	//	 2	 3
	//	6	7
	ptVerts[ 0 ].vPosition = VECTOR( 0, 0, 1 );
	ptVerts[ 1 ].vPosition = VECTOR( 1, 0, 1 );
	ptVerts[ 2 ].vPosition = VECTOR( 0, 0, 0 );
	ptVerts[ 3 ].vPosition = VECTOR( 1, 0, 0 );
	ptVerts[ 4 ].vPosition = VECTOR( 0, 1, 1 );
	ptVerts[ 5 ].vPosition = VECTOR( 1, 1, 1 );
	ptVerts[ 6 ].vPosition = VECTOR( 0, 1, 0 );
	ptVerts[ 7 ].vPosition = VECTOR( 1, 1, 0 );
}

inline static void sSetTriangleIndices( int ul, int ur, int ll, int lr, WORD * pwIndices )
{
	pwIndices[ 0 ] = ul;
	pwIndices[ 1 ] = ur;
	pwIndices[ 2 ] = ll;
	pwIndices[ 3 ] = ll;
	pwIndices[ 4 ] = ur;
	pwIndices[ 5 ] = lr;
}

static void sSetBoxIndices( WORD * pwIndices )
{
	//	 0	 1
	//	4	5
	//	 2	 3
	//	6	7
	int nIndex = 0;
	sSetTriangleIndices( 1, 0, 3, 2, &pwIndices[ nIndex ] ); nIndex += 6;
	sSetTriangleIndices( 5, 1, 7, 3, &pwIndices[ nIndex ] ); nIndex += 6; 
	sSetTriangleIndices( 4, 5, 6, 7, &pwIndices[ nIndex ] ); nIndex += 6;
	sSetTriangleIndices( 0, 4, 2, 6, &pwIndices[ nIndex ] ); nIndex += 6;
	sSetTriangleIndices( 0, 1, 4, 5, &pwIndices[ nIndex ] ); nIndex += 6;
	sSetTriangleIndices( 6, 7, 2, 3, &pwIndices[ nIndex ] );
}

PRESULT dx9_PrimitiveDrawRestore()
{
	// bounding box verts
	gtBoundingBoxVertexBuffer.eVertexType = VERTEX_DECL_SIMPLE;
	gtBoundingBoxVertexBuffer.nVertexCount = NUM_BOUNDING_BOX_LIST_VERTICES;
	gtBoundingBoxVertexBuffer.nBufferSize[ 0 ] = NUM_BOUNDING_BOX_LIST_VERTICES * sizeof( BOUNDING_BOX_VERTEX );
	gtBoundingBoxVertexBuffer.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	V_RETURN( dxC_CreateVertexBuffer( 0, gtBoundingBoxVertexBuffer ) );
	BOUNDING_BOX_VERTEX tBBoxVerts[ NUM_BOUNDING_BOX_LIST_VERTICES ];
	sSetBoxVertices( tBBoxVerts );
	V_RETURN( dxC_UpdateBuffer( 0, gtBoundingBoxVertexBuffer, tBBoxVerts, 0, gtBoundingBoxVertexBuffer.nBufferSize[ 0 ] ) );
	V_RETURN( dxC_VertexBufferPreload( gtBoundingBoxVertexBuffer.nD3DBufferID[ 0 ] ) );

	//2D bounding box verts
	gt2DBoundingBoxVertexBuffer.eVertexType = VERTEX_DECL_SIMPLE;
	gt2DBoundingBoxVertexBuffer.nVertexCount = NUM_BOUNDING_BOX_2D_LIST_VERTICES;
	gt2DBoundingBoxVertexBuffer.nBufferSize[ 0 ] = NUM_BOUNDING_BOX_2D_LIST_VERTICES * sizeof( BOUNDING_BOX_VERTEX );
	gt2DBoundingBoxVertexBuffer.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	V_RETURN( dxC_CreateVertexBuffer( 0, gt2DBoundingBoxVertexBuffer ) );

	// bounding box indices
	gtBoundingBoxIndexBuffer.tFormat = D3DFMT_INDEX16;
	gtBoundingBoxIndexBuffer.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	gtBoundingBoxIndexBuffer.nBufferSize = NUM_BOUNDING_BOX_LIST_INDICES * sizeof( WORD );
	V_RETURN( dxC_CreateIndexBuffer( gtBoundingBoxIndexBuffer ) );
	WORD wIndices[ NUM_BOUNDING_BOX_LIST_INDICES ];
	sSetBoxIndices( wIndices );
	V_RETURN( dxC_UpdateBuffer( gtBoundingBoxIndexBuffer, wIndices, 0, gtBoundingBoxIndexBuffer.nBufferSize ) );
	V_RETURN( dxC_IndexBufferPreload( gtBoundingBoxIndexBuffer.nD3DBufferID ) );

	//2D bounding box indices
	gt2DBoundingBoxIndexBuffer.tFormat = D3DFMT_INDEX16;
	gt2DBoundingBoxIndexBuffer.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	gt2DBoundingBoxIndexBuffer.nBufferSize = NUM_BOUNDING_BOX_2D_LIST_INDICES * sizeof( WORD );
	V_RETURN( dxC_CreateIndexBuffer( gt2DBoundingBoxIndexBuffer ) );
	WORD w2DIndices[ NUM_BOUNDING_BOX_LIST_INDICES ] = { 0,1,1,2,2,3,3,0 };
	V_RETURN( dxC_UpdateBuffer( gt2DBoundingBoxIndexBuffer, w2DIndices, 0, gt2DBoundingBoxIndexBuffer.nBufferSize ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_PrimitiveDrawDestroy()
{
	V( dxC_VertexBufferDestroy( gtBoundingBoxVertexBuffer.nD3DBufferID[ 0 ] ) );
	gtBoundingBoxVertexBuffer.nD3DBufferID[ 0 ] = INVALID_ID;
	V( dxC_IndexBufferDestroy( gtBoundingBoxIndexBuffer.nD3DBufferID ) );
	gtBoundingBoxIndexBuffer.nD3DBufferID = INVALID_ID;

	V( dxC_VertexBufferDestroy( gt2DBoundingBoxVertexBuffer.nD3DBufferID[ 0 ] ) );
	gt2DBoundingBoxVertexBuffer.nD3DBufferID[ 0 ] = INVALID_ID;
	V( dxC_IndexBufferDestroy( gt2DBoundingBoxIndexBuffer.nD3DBufferID ) );
	gt2DBoundingBoxIndexBuffer.nD3DBufferID = INVALID_ID;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//static void sTransformDebugPrimitiveVertex( DEBUG_PRIMITIVE_SCREEN_VERTEX & pVert, D3DXMATRIXA16 & matWVP, DWORD dwFlags )
//{
//	D3DXVECTOR4 vTransformed;
//	D3DXVec3Transform( &vTransformed, &pVert.vPos, &matWVP );
//	pVert.vPos.x = vTransformed.x / vTransformed.w;
//	pVert.vPos.y = vTransformed.y / vTransformed.w;
//	if ( vTransformed.w < 0.0f )
//	{
//		pVert.vPos.x = - pVert.vPos.x;
//		pVert.vPos.y = - pVert.vPos.y;
//	}
//	if ( dwFlags & PRIM_FLAG_RENDER_ON_TOP )
//		pVert.vPos.z = PRIM_DEPTH_MIN;
//	else
//		pVert.vPos.z = vTransformed.z / vTransformed.w;
//
//	int nWidth, nHeight;
//	e_GetWindowSize( nWidth, nHeight );
//	pVert.vPos.x = ( pVert.vPos.x * 0.5f + 0.5f ) * (float)nWidth;
//	pVert.vPos.y = ( 1.0f - ( pVert.vPos.y * 0.5f + 0.5f ) ) * (float)nHeight;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static inline void sBuildDebugPrimitiveVertex( DEBUG_PRIMITIVE_SCREEN_VERTEX * pVerts, int nVertsPerPrim, PRIMITIVE_INFO_RENDER & tInfo, D3DXMATRIXA16 * pmatWVP )
{
	pVerts[ 0 ].fRHW		= pVerts[ 1 ].fRHW		= 1.0f;
	pVerts[ 0 ].dwColor		= pVerts[ 1 ].dwColor	= tInfo.dwColor;
	pVerts[ 0 ].vPos		= *(D3DXVECTOR3*)&tInfo.v1;
	pVerts[ 1 ].vPos		= *(D3DXVECTOR3*)&tInfo.v2;
	if ( nVertsPerPrim == 3 )
	{
		pVerts[ 2 ].fRHW		= 1.0f;
		pVerts[ 2 ].dwColor		= tInfo.dwColor;
		pVerts[ 2 ].vPos		= *(D3DXVECTOR3*)&tInfo.v3;
	}

	// screen space offset
	pVerts[ 0 ].vPos		+= D3DXVECTOR3( 0.5f, 0.5f, 0.f );
	pVerts[ 1 ].vPos		+= D3DXVECTOR3( 0.5f, 0.5f, 0.f );
	if ( nVertsPerPrim == 3 )
	{
		pVerts[ 2 ].vPos		+= D3DXVECTOR3( 0.5f, 0.5f, 0.f );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static inline void sBuildDebugPrimitiveVertex( DEBUG_PRIMITIVE_WORLD_VERTEX * pVerts, int nVertsPerPrim, PRIMITIVE_INFO_RENDER & tInfo, D3DXMATRIXA16 * pmatWVP )
{
	pVerts[ 0 ].dwColor		= pVerts[ 1 ].dwColor	= tInfo.dwColor;
	pVerts[ 0 ].vPos		= *(D3DXVECTOR3*)&tInfo.v1;
	pVerts[ 1 ].vPos		= *(D3DXVECTOR3*)&tInfo.v2;
	if ( nVertsPerPrim == 3 )
	{
		pVerts[ 2 ].dwColor		= tInfo.dwColor;
		pVerts[ 2 ].vPos		= *(D3DXVECTOR3*)&tInfo.v3;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sBuildUnitSphere( UNIT_SPHERE * pSphere, int nSubs )
{
	// subdivide an octahedron -- 8 * 4^N tris for N subdivisions
	// sub-optimal -- not reusing verts, but duplicating them
	//const int cnSubs = 2;
	//const int cnTris = 8 * 16; // this needs to be 8 * 4^N, be it 4^1 or 4^5
	//const int cnVerts = 3 * cnTris;
	//VECTOR tVerts[ cnTris ][ 3 ];

	pSphere->nSubs = nSubs;
	pSphere->nTris = 8 * POWI( 4, nSubs );
	pSphere->nVerts = 3 * pSphere->nTris;
	ASSERT_RETINVALIDARG( NULL == pSphere->vSource );
	ASSERT_RETINVALIDARG( NULL == pSphere->vTemp );
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	pSphere->vSource = (VECTOR*) MALLOCZ( g_StaticAllocator, sizeof(VECTOR) * pSphere->nVerts );
	ASSERT_RETVAL( NULL != pSphere->vSource, E_OUTOFMEMORY );
	pSphere->vTemp   = (VECTOR*) MALLOCZ( g_StaticAllocator, sizeof(VECTOR) * pSphere->nVerts );
	ASSERT_RETVAL( NULL != pSphere->vTemp, E_OUTOFMEMORY );
#else
	pSphere->vSource = (VECTOR*) MALLOCZ( NULL, sizeof(VECTOR) * pSphere->nVerts );
	ASSERT_RETVAL( NULL != pSphere->vSource, E_OUTOFMEMORY );
	pSphere->vTemp   = (VECTOR*) MALLOCZ( NULL, sizeof(VECTOR) * pSphere->nVerts );
	ASSERT_RETVAL( NULL != pSphere->vTemp, E_OUTOFMEMORY );
#endif		

	VECTOR * tVerts = pSphere->vSource;

	// build octahedron in unit-length segments
	VECTOR vX(1,0,0), vY(0,1,0), vZ(0,0,1);
	int nGran = max( POWI( 4, pSphere->nSubs ), 1 );
	tVerts[ 0 * nGran * 3 + 0 ] = VECTOR(  0, 0, 1 );
	tVerts[ 0 * nGran * 3 + 1 ] = VECTOR(  1, 1, 0 );
	tVerts[ 0 * nGran * 3 + 2 ] = VECTOR( -1, 1, 0 );
	tVerts[ 1 * nGran * 3 + 0 ] = VECTOR(  0, 0, 1 );
	tVerts[ 1 * nGran * 3 + 1 ] = VECTOR(  1,-1, 0 );
	tVerts[ 1 * nGran * 3 + 2 ] = VECTOR( -1,-1, 0 );
	tVerts[ 2 * nGran * 3 + 0 ] = VECTOR(  0, 0, 1 );
	tVerts[ 2 * nGran * 3 + 1 ] = VECTOR( -1, 1, 0 );
	tVerts[ 2 * nGran * 3 + 2 ] = VECTOR( -1,-1, 0 );
	tVerts[ 3 * nGran * 3 + 0 ] = VECTOR(  0, 0, 1 );
	tVerts[ 3 * nGran * 3 + 1 ] = VECTOR(  1, 1, 0 );
	tVerts[ 3 * nGran * 3 + 2 ] = VECTOR(  1,-1, 0 );
	tVerts[ 4 * nGran * 3 + 0 ] = VECTOR(  0, 0,-1 );
	tVerts[ 4 * nGran * 3 + 1 ] = VECTOR(  1, 1, 0 );
	tVerts[ 4 * nGran * 3 + 2 ] = VECTOR( -1, 1, 0 );
	tVerts[ 5 * nGran * 3 + 0 ] = VECTOR(  0, 0,-1 );
	tVerts[ 5 * nGran * 3 + 1 ] = VECTOR(  1,-1, 0 );
	tVerts[ 5 * nGran * 3 + 2 ] = VECTOR( -1,-1, 0 );
	tVerts[ 6 * nGran * 3 + 0 ] = VECTOR(  0, 0,-1 );
	tVerts[ 6 * nGran * 3 + 1 ] = VECTOR( -1, 1, 0 );
	tVerts[ 6 * nGran * 3 + 2 ] = VECTOR( -1,-1, 0 );
	tVerts[ 7 * nGran * 3 + 0 ] = VECTOR(  0, 0,-1 );
	tVerts[ 7 * nGran * 3 + 1 ] = VECTOR(  1, 1, 0 );
	tVerts[ 7 * nGran * 3 + 2 ] = VECTOR(  1,-1, 0 );

	// normalize sphere verts at each subdivision stage
	for ( int i = 0; i < pSphere->nTris; i+=nGran )
	{
		VectorNormalize( tVerts[ i * 3 + 0 ] );
		VectorNormalize( tVerts[ i * 3 + 1 ] );
		VectorNormalize( tVerts[ i * 3 + 2 ] );
	}

	VECTOR vMidpoints[3], * pvCorners;
	int nUpGran = nGran;
	for ( int s = 0; s < pSphere->nSubs; s++ )
	{
		nGran = max( POWI( 4, pSphere->nSubs - (s+1) ), 1 );
		// for each tri previously created, subdivide it 4 ways spread over the new granularity
		for ( int t = 0; t < pSphere->nTris; t += nUpGran )
		{
			pvCorners = &tVerts[ ( t + 0 * nGran ) * 3 ];
			vMidpoints[ 0 ] = ( pvCorners[ 0 ] + pvCorners[ 1 ] ) * 0.5f;
			vMidpoints[ 1 ] = ( pvCorners[ 1 ] + pvCorners[ 2 ] ) * 0.5f;
			vMidpoints[ 2 ] = ( pvCorners[ 2 ] + pvCorners[ 0 ] ) * 0.5f;
			VectorNormalize( vMidpoints[ 0 ] );
			VectorNormalize( vMidpoints[ 1 ] );
			VectorNormalize( vMidpoints[ 2 ] );

			tVerts[ ( t + 1 * nGran ) * 3 + 0 ] = vMidpoints[ 0 ];
			tVerts[ ( t + 1 * nGran ) * 3 + 1 ] = pvCorners[ 1 ];
			tVerts[ ( t + 1 * nGran ) * 3 + 2 ] = vMidpoints[ 1 ];
			tVerts[ ( t + 2 * nGran ) * 3 + 0 ] = vMidpoints[ 1 ];
			tVerts[ ( t + 2 * nGran ) * 3 + 1 ] = vMidpoints[ 2 ];
			tVerts[ ( t + 2 * nGran ) * 3 + 2 ] = vMidpoints[ 0 ];
			tVerts[ ( t + 3 * nGran ) * 3 + 0 ] = vMidpoints[ 2 ];
			tVerts[ ( t + 3 * nGran ) * 3 + 1 ] = vMidpoints[ 1 ];
			tVerts[ ( t + 3 * nGran ) * 3 + 2 ] = pvCorners[ 2 ];
			// reset the top triangle last
			tVerts[ ( t + 0 * nGran ) * 3 + 0 ] = pvCorners[ 0 ];
			tVerts[ ( t + 0 * nGran ) * 3 + 1 ] = vMidpoints[ 0 ];
			tVerts[ ( t + 0 * nGran ) * 3 + 2 ] = vMidpoints[ 2 ];
		}
		nUpGran = nGran;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitivePlatformInit()
{
	dxC_VertexBufferDefinitionInit( gtBoundingBoxVertexBuffer );
	dxC_IndexBufferDefinitionInit( gtBoundingBoxIndexBuffer );

	dxC_VertexBufferDefinitionInit( gt2DBoundingBoxVertexBuffer );
	dxC_IndexBufferDefinitionInit( gt2DBoundingBoxIndexBuffer );

	gtPrimRenderList_XYZ[ PRIM_PASS_IN_WORLD ][ PRIM_RENDER_TRI  ].Init( PRIM_MAX_TRIS,  3, FALSE, TRUE,  DEBUG_PRIMITIVE_WORLD_FVF,  &gtPrimInfoTri );
	gtPrimRenderList_XYZ[ PRIM_PASS_IN_WORLD ][ PRIM_RENDER_LINE ].Init( PRIM_MAX_LINES, 2, FALSE, TRUE,  DEBUG_PRIMITIVE_WORLD_FVF,  &gtPrimInfoLine );
	gtPrimRenderList_XYZ[ PRIM_PASS_ON_TOP   ][ PRIM_RENDER_TRI  ].Init( PRIM_MAX_TRIS,  3, TRUE,  TRUE,  DEBUG_PRIMITIVE_WORLD_FVF,  &gtPrimInfoTri );
	gtPrimRenderList_XYZ[ PRIM_PASS_ON_TOP   ][ PRIM_RENDER_LINE ].Init( PRIM_MAX_LINES, 2, TRUE,  TRUE,  DEBUG_PRIMITIVE_WORLD_FVF,  &gtPrimInfoLine );

	gtPrimRenderList_RHW[ PRIM_PASS_IN_WORLD ][ PRIM_RENDER_TRI  ].Init( PRIM_MAX_TRIS,  3, FALSE, FALSE, DEBUG_PRIMITIVE_SCREEN_FVF, &gtPrimInfoTri );
	gtPrimRenderList_RHW[ PRIM_PASS_IN_WORLD ][ PRIM_RENDER_LINE ].Init( PRIM_MAX_LINES, 2, FALSE, FALSE, DEBUG_PRIMITIVE_SCREEN_FVF, &gtPrimInfoLine );
	gtPrimRenderList_RHW[ PRIM_PASS_ON_TOP   ][ PRIM_RENDER_TRI  ].Init( PRIM_MAX_TRIS,  3, TRUE,  FALSE, DEBUG_PRIMITIVE_SCREEN_FVF, &gtPrimInfoTri );
	gtPrimRenderList_RHW[ PRIM_PASS_ON_TOP   ][ PRIM_RENDER_LINE ].Init( PRIM_MAX_LINES, 2, TRUE,  FALSE, DEBUG_PRIMITIVE_SCREEN_FVF, &gtPrimInfoLine );

	// pre-create two LOD spheres
	V( sBuildUnitSphere( &sgtUnitSphere1, 1 ) );
	V( sBuildUnitSphere( &sgtUnitSphere2, 2 ) );
	V( sBuildUnitSphere( &sgtUnitSphere3, 3 ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCleanupUnitSphere( UNIT_SPHERE & tSphere )
{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	if ( tSphere.vSource )
		FREE( g_StaticAllocator, tSphere.vSource );
	if ( tSphere.vTemp )
		FREE( g_StaticAllocator, tSphere.vTemp );
#else
	if ( tSphere.vSource )
		FREE( NULL, tSphere.vSource );
	if ( tSphere.vTemp )
		FREE( NULL, tSphere.vTemp );
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitivePlatformDestroy()
{
	for ( int p = 0; p < NUM_PRIM_PASSES; p++ )
	{
		for ( int t = 0; t < NUM_PRIM_RENDER_TYPES; t++ )
		{
			V( gtPrimRenderList_XYZ[ p ][ t ].Destroy() );
			V( gtPrimRenderList_RHW[ p ][ t ].Destroy() );
		}
	}

	sCleanupUnitSphere( sgtUnitSphere1 );
	sCleanupUnitSphere( sgtUnitSphere2 );
	sCleanupUnitSphere( sgtUnitSphere3 );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_PrimitiveDrawDeviceLost()
{
	for ( int p = 0; p < NUM_PRIM_PASSES; p++ )
	{
		for ( int t = 0; t < NUM_PRIM_RENDER_TYPES; t++ )
		{
			gtPrimRenderList_XYZ[ p ][ t ].DeviceLost();
			gtPrimRenderList_RHW[ p ][ t ].DeviceLost();
		}
	}
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template < class T >
void sBuildUnitCircle( T * pVerts, int nVerts )
{
	int nDegStep = 360 / nVerts;
	int nQuadVerts = nVerts / 4;

	// reset per quadrant to shrink int truncation error
	int nQuadDeg = 0;
	int nVert = 0;
	for ( int q = 0; q < 4; q++ )
	{
		int nDeg = nQuadDeg;
		for ( int i = 0; i < nQuadVerts; i++ )
		{
			ASSERT_BREAK( nVert < nVerts );
			pVerts[ nVert ].x = gfSin360Tbl[ nDeg ];
			pVerts[ nVert ].y = gfCos360Tbl[ nDeg ];
			nVert++;
			nDeg += nDegStep;
		}
		nQuadDeg += 90;
	}
}
template void sBuildUnitCircle< VECTOR2 > ( VECTOR2 * pVerts, int nVerts );
template void sBuildUnitCircle< VECTOR  > ( VECTOR  * pVerts, int nVerts );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UNIT_SPHERE * sPickSphereLOD( PRIMITIVE_INFO_SPHERE & tInfo, VECTOR & vViewPos )
{
	// select between the available sphere LODs

	// base it on range and size
	// when range is x * radius, switch
	float fDist = VectorDistanceSquared( tInfo.vCenter, vViewPos );
	fDist /= tInfo.fRadius * tInfo.fRadius;
	if ( fDist < SPHERE_LOD_DIST_RAD_RATIO_3 * SPHERE_LOD_DIST_RAD_RATIO_3 )
		return & sgtUnitSphere3;
	if ( fDist < SPHERE_LOD_DIST_RAD_RATIO_2 * SPHERE_LOD_DIST_RAD_RATIO_2 )
		return & sgtUnitSphere2;
	return & sgtUnitSphere1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UpdateDebugPrimitives()
{
	if ( ! e_PrimitivesNeedRender() )
		return S_FALSE;

	VECTOR vViewPos;
	e_GetViewPosition( &vViewPos );

	// iterate each of the high-level prim types and add them to the line/tri render lists
	int nCount;

	// -----------------------------------------------------------
	// 2D BOX
	// -----------------------------------------------------------
	nCount = gtPrimInfo2DBox.Count();
	for ( int b = 0; b < nCount; b++ )
	{
		PRIMITIVE_INFO_2D_BOX & tInfo = gtPrimInfo2DBox[ b ];
		// build point list
		// 0 xy  1 Xy
		// 2 xY  3 XY
		VECTOR2 tVerts[ 4 ];

		tVerts[ 0 ].x = tInfo.vMin.x;	tVerts[ 0 ].y = tInfo.vMin.y;
		tVerts[ 1 ].x = tInfo.vMax.x;	tVerts[ 1 ].y = tInfo.vMin.y;
		tVerts[ 2 ].x = tInfo.vMin.x;	tVerts[ 2 ].y = tInfo.vMax.y;
		tVerts[ 3 ].x = tInfo.vMax.x;	tVerts[ 3 ].y = tInfo.vMax.y;

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 0 ], &tVerts[ 1 ], &tVerts[ 3 ], tInfo.dwColor, tInfo.vMin.z ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 3 ], &tVerts[ 2 ], &tVerts[ 0 ], tInfo.dwColor, tInfo.vMin.z ) );
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 0 ], &tVerts[ 1 ], tInfo.dwColor, tInfo.vMin.z ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 1 ], &tVerts[ 3 ], tInfo.dwColor, tInfo.vMin.z ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 3 ], &tVerts[ 2 ], tInfo.dwColor, tInfo.vMin.z ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 2 ], &tVerts[ 0 ], tInfo.dwColor, tInfo.vMin.z ) );
		}
	}

	// -----------------------------------------------------------
	// 3D BOX
	// -----------------------------------------------------------
	nCount = gtPrimInfo3DBox.Count();
	for ( int b = 0; b < nCount; b++ )
	{
		PRIMITIVE_INFO_3D_BOX & tInfo = gtPrimInfo3DBox[ b ];
		// point list already in tInfo
		ASSERT_BREAK( OBB_POINTS == 8 );
		const VECTOR * tVerts = tInfo.tBox;

		// ORIENTED_BOUNDING_BOX point alignment
		//
		//       4-----5
		//      /|    /|
		// +Z  6-----7 |    -y
		//     | |   | |   
		// -z  | 0---|-1  +Y
		//     |/    |/
		//     2-----3
		//
		//      -x  +X

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 4 ], &tVerts[ 5 ], &tVerts[ 7 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 7 ], &tVerts[ 6 ], &tVerts[ 4 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 5 ], &tVerts[ 4 ], &tVerts[ 0 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 0 ], &tVerts[ 1 ], &tVerts[ 5 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 0 ], &tVerts[ 1 ], &tVerts[ 3 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 3 ], &tVerts[ 2 ], &tVerts[ 0 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 6 ], &tVerts[ 7 ], &tVerts[ 3 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 3 ], &tVerts[ 2 ], &tVerts[ 6 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 7 ], &tVerts[ 5 ], &tVerts[ 1 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 1 ], &tVerts[ 3 ], &tVerts[ 7 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 4 ], &tVerts[ 6 ], &tVerts[ 2 ], tInfo.dwColor ) );
			V( e_PrimitiveAddTri( tInfo.dwFlags, &tVerts[ 2 ], &tVerts[ 0 ], &tVerts[ 4 ], tInfo.dwColor ) );
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 4 ], &tVerts[ 5 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 5 ], &tVerts[ 7 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 7 ], &tVerts[ 6 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 6 ], &tVerts[ 4 ], tInfo.dwColor ) );

			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 0 ], &tVerts[ 1 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 1 ], &tVerts[ 3 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 3 ], &tVerts[ 2 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 2 ], &tVerts[ 0 ], tInfo.dwColor ) );

			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 4 ], &tVerts[ 0 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 5 ], &tVerts[ 1 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 7 ], &tVerts[ 3 ], tInfo.dwColor ) );
			V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ 6 ], &tVerts[ 2 ], tInfo.dwColor ) );
		}
	}

	// -----------------------------------------------------------
	// CIRCLE
	// -----------------------------------------------------------
	nCount = gtPrimInfoCircle.Count();
	for ( int c = 0; c < nCount; c++ )
	{
		PRIMITIVE_INFO_SPHERE & tInfo = gtPrimInfoCircle[ c ];

		// build point list
		const int cnVerts = PRIM_CIRCLE_VERTS;
		VECTOR2 tVerts[ cnVerts+1 ];
		sBuildUnitCircle( tVerts, cnVerts );
		VECTOR2 vCenter( tInfo.vCenter.x, tInfo.vCenter.y );
		for ( int i = 0; i < cnVerts; i++ )
		{
			tVerts[ i ] *= tInfo.fRadius;
			tVerts[ i ] += vCenter;
		}
		tVerts[ cnVerts ] = tVerts[ 0 ];  // allow for wrap-around generation

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddTri( tInfo.dwFlags, &vCenter, &tVerts[ i ], &tVerts[ i+1 ], tInfo.dwColor, tInfo.vCenter.z ) );
			}
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddLine( tInfo.dwFlags, &tVerts[ i ], &tVerts[ i+1 ], tInfo.dwColor, tInfo.vCenter.z ) );
			}
		}
	}

	// -----------------------------------------------------------
	// SPHERE
	// -----------------------------------------------------------
	nCount = gtPrimInfoSphere.Count();
	for ( int s = 0; s < nCount; s++ )
	{
		PRIMITIVE_INFO_SPHERE & tInfo = gtPrimInfoSphere[ s ];

		// select high or low LOD
		UNIT_SPHERE * pSphere = sPickSphereLOD( tInfo, vViewPos );
		memcpy( pSphere->vTemp, pSphere->vSource, sizeof(VECTOR) * pSphere->nVerts );

		// offset sphere verts
		for ( int i = 0; i < pSphere->nTris; i++ )
		{
			pSphere->vTemp[ i * 3 + 0 ] *= tInfo.fRadius;
			pSphere->vTemp[ i * 3 + 1 ] *= tInfo.fRadius;
			pSphere->vTemp[ i * 3 + 2 ] *= tInfo.fRadius;
			pSphere->vTemp[ i * 3 + 0 ] += tInfo.vCenter;
			pSphere->vTemp[ i * 3 + 1 ] += tInfo.vCenter;
			pSphere->vTemp[ i * 3 + 2 ] += tInfo.vCenter;
		}

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			for ( int i = 0; i < pSphere->nTris; i++ )
			{
				V( e_PrimitiveAddTri( tInfo.dwFlags, &pSphere->vTemp[ i * 3 + 0 ], &pSphere->vTemp[ i * 3 + 1 ], &pSphere->vTemp[ i * 3 + 2 ], tInfo.dwColor ) );
			}
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			for ( int i = 0; i < pSphere->nTris; i++ )
			{
				V( e_PrimitiveAddLine( tInfo.dwFlags, &pSphere->vTemp[ i * 3 + 0 ], &pSphere->vTemp[ i * 3 + 1 ], tInfo.dwColor ) );
				V( e_PrimitiveAddLine( tInfo.dwFlags, &pSphere->vTemp[ i * 3 + 1 ], &pSphere->vTemp[ i * 3 + 2 ], tInfo.dwColor ) );
				V( e_PrimitiveAddLine( tInfo.dwFlags, &pSphere->vTemp[ i * 3 + 2 ], &pSphere->vTemp[ i * 3 + 0 ], tInfo.dwColor ) );
			}
		}
	}

	// -----------------------------------------------------------
	// CYLINDER
	// -----------------------------------------------------------
	nCount = gtPrimInfoCylinder.Count();
	for ( int c = 0; c < nCount; c++ )
	{
		PRIMITIVE_INFO_CYLINDER & tInfo = gtPrimInfoCylinder[ c ];

		// build point list
		const int cnVerts = PRIM_CIRCLE_VERTS;
		VECTOR tVerts[ cnVerts+1 ];
		sBuildUnitCircle( tVerts, cnVerts );
		VECTOR vCenter[2] = { tInfo.vEnd1, tInfo.vEnd2 };
		tVerts[ cnVerts ] = tVerts[ 0 ];  // allow for wrap-around generation

		VECTOR vDir = tInfo.vEnd1 - tInfo.vEnd2;
		VectorNormalize( vDir );
		MATRIX matRot;

		//VECTOR vUp( 0, 0, 1 );
		VECTOR xaxis, yaxis, zaxis = vDir;
		xaxis.fX = -zaxis.fZ;
		xaxis.fY = zaxis.fX;
		xaxis.fZ = zaxis.fY;
		VectorNormalize( xaxis );
		VectorCross( yaxis, zaxis, xaxis );
		VectorNormalize( yaxis );

		MatrixIdentity( &matRot );
		*(VECTOR*)&matRot._11 = xaxis;
		*(VECTOR*)&matRot._21 = yaxis;
		*(VECTOR*)&matRot._31 = zaxis;

		VECTOR tVertsRot[ 2 ][ cnVerts+1 ];
		for ( int i = 0; i < cnVerts+1; i++ )
		{
			tVerts[ i ].z = 0.0f;
			VectorNormalize( tVerts[ i ] );
			MatrixMultiply( &tVertsRot[ 0 ][ i ], &tVerts[ i ], &matRot );
			tVertsRot[ 0 ][ i ] *= tInfo.fRadius;
		}
		memcpy( &tVertsRot[ 1 ], &tVertsRot[ 0 ], sizeof(VECTOR)*(cnVerts+1) );
		for ( int i = 0; i < cnVerts+1; i++ )
		{
			tVertsRot[ 0 ][ i ] += tInfo.vEnd1;
			tVertsRot[ 1 ][ i ] += tInfo.vEnd2;
		}

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			for ( int n = 0; n < 2; n++ )
			{
				for ( int i = 0; i < cnVerts; i++ )
				{
					V( e_PrimitiveAddTri( tInfo.dwFlags, &vCenter[ n ], &tVertsRot[ n ][ i ], &tVertsRot[ n ][ i+1 ], tInfo.dwColor ) );
				}
			}
			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddTri( tInfo.dwFlags, &tVertsRot[ 0 ][ i   ], &tVertsRot[ 0 ][ i+1 ], &tVertsRot[ 1 ][ i+1 ], tInfo.dwColor ) );
				V( e_PrimitiveAddTri( tInfo.dwFlags, &tVertsRot[ 1 ][ i+1 ], &tVertsRot[ 1 ][ i   ], &tVertsRot[ 0 ][ i   ], tInfo.dwColor ) );
			}
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			for ( int n = 0; n < 2; n++ )
			{
				for ( int i = 0; i < cnVerts; i++ )
				{
					V( e_PrimitiveAddLine( tInfo.dwFlags, &tVertsRot[ n ][ i ], &tVertsRot[ n ][ i+1 ], tInfo.dwColor ) );
				}
			}
			for ( int i = 0; i < cnVerts; i+= cnVerts/6 )
			{
				V( e_PrimitiveAddLine( tInfo.dwFlags, &tVertsRot[ 0 ][ i ], &tVertsRot[ 1 ][ i ], tInfo.dwColor ) );
			}
		}
	}

	// -----------------------------------------------------------
	// CONE
	// -----------------------------------------------------------
	nCount = gtPrimInfoCone.Count();
	for ( int c = 0; c < nCount; c++ )
	{
		PRIMITIVE_INFO_CONE & tInfo = gtPrimInfoCone[ c ];

		// build point list
		const int cnVerts = PRIM_CIRCLE_VERTS;
		VECTOR tVerts[ cnVerts+1 ];
		sBuildUnitCircle( tVerts, cnVerts );
		VECTOR vCenter = tInfo.vApex + tInfo.vDirection;
		tVerts[ cnVerts ] = tVerts[ 0 ];  // allow for wrap-around generation

		float fRadius = atanf( tInfo.fAngleRad ) * VectorLength( tInfo.vDirection );

		VECTOR vDir = tInfo.vDirection;
		VectorNormalize( vDir );
		MATRIX matRot;

		//VECTOR vUp( 0, 0, 1 );
		VECTOR xaxis, yaxis, zaxis = vDir;
		xaxis.fX = -zaxis.fZ;
		xaxis.fY = zaxis.fX;
		xaxis.fZ = zaxis.fY;
		VectorNormalize( xaxis );
		VectorCross( yaxis, zaxis, xaxis );
		VectorNormalize( yaxis );

		MatrixIdentity( &matRot );
		*(VECTOR*)&matRot._11 = xaxis;
		*(VECTOR*)&matRot._21 = yaxis;
		*(VECTOR*)&matRot._31 = zaxis;

		VECTOR tVertsRot[ cnVerts+1 ];
		for ( int i = 0; i < cnVerts+1; i++ )
		{
			tVerts[ i ].z = 0.0f;
			VectorNormalize( tVerts[ i ] );
			MatrixMultiply( &tVertsRot[ i ], &tVerts[ i ], &matRot );
			tVertsRot[ i ] *= fRadius;
			tVertsRot[ i ] += vCenter;
		}

		if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL )
		{
			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddTri( tInfo.dwFlags, &vCenter,		&tVertsRot[ i ], &tVertsRot[ i+1 ], tInfo.dwColor ) );
			}
			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddTri( tInfo.dwFlags, &tInfo.vApex, &tVertsRot[ i ], &tVertsRot[ i+1 ], tInfo.dwColor ) );
			}
		}
		if ( 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) || tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
		{
			if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER )
				tInfo.dwColor = PRIM_WIRE_BORDER_COLOR;

			for ( int i = 0; i < cnVerts; i++ )
			{
				V( e_PrimitiveAddLine( tInfo.dwFlags, &tVertsRot[ i ], &tVertsRot[ i+1 ], tInfo.dwColor ) );
			}
			for ( int i = 0; i < cnVerts; i+= cnVerts/6 )
			{
				V( e_PrimitiveAddLine( tInfo.dwFlags, &tInfo.vApex,	&tVertsRot[ i ], tInfo.dwColor ) );
			}
		}

	}




	// -----------------------------------------------------------
	// BUILD FINAL RENDER LISTS
	// -----------------------------------------------------------

	for ( int p = 0; p < NUM_PRIM_PASSES; p++ )
	{
		for ( int t = 0; t < NUM_PRIM_RENDER_TYPES; t++ )
		{
			gtPrimRenderList_XYZ[ p ][ t ].Add();
			gtPrimRenderList_RHW[ p ][ t ].Add();
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_RenderDebugPrimitives( BOOL bRenderOnTop )
{
	if ( ! e_PrimitivesNeedRender() )
		return S_FALSE;
#ifdef ENGINE_TARGET_DX9  //NUTTAPONG TODO: Need to do in DX10 without fixed function
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
	dxC_SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA );
	dxC_SetRenderState( D3DRS_BLENDOP,			D3DBLENDOP_ADD);
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE);
	dxC_SetRenderState( D3DRS_CULLMODE,			D3DCULL_NONE );
	e_SetFogEnabled( FALSE );

	V( dx9_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE) );
	V( dx9_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1) );
	V( dx9_SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE) );
	V( dx9_SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE) );

	D3DXMATRIXA16 matWorld, matView, matProj;
	V( e_GetWorldViewProjMatricies( (MATRIX*)&matWorld, (MATRIX*)&matView, (MATRIX*)&matProj ) );
	V( dxC_SetTransform( D3DTS_WORLD,		&matWorld ) );
	V( dxC_SetTransform( D3DTS_VIEW,		&matView ) );
	V( dxC_SetTransform( D3DTS_PROJECTION,	&matProj ) );

	//dxC_SetIndices( NULL );
	V( dx9_SetPixelShader( NULL ) );
	V( dx9_SetVertexShader( NULL ) );

	int nPass;

	if ( ! bRenderOnTop )
	{
		dxC_SetRenderState( D3DRS_ZENABLE,			D3DZB_TRUE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE,		D3DZB_FALSE );

		// "betwixt models" pass
		nPass = PRIM_PASS_IN_WORLD;
	}
	else
	{
		dxC_SetRenderState( D3DRS_ZENABLE,			D3DZB_FALSE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE,		D3DZB_FALSE );

		// "after everything" pass
		nPass = PRIM_PASS_ON_TOP;
	}

	for ( int t = 0; t < NUM_PRIM_RENDER_TYPES; t++ )
	{
		V( gtPrimRenderList_XYZ[ nPass ][ t ].Render() );
		V( gtPrimRenderList_XYZ[ nPass ][ t ].Clear() );
		V( gtPrimRenderList_RHW[ nPass ][ t ].Render() );
		V( gtPrimRenderList_RHW[ nPass ][ t ].Clear() );
	}


	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE  );
	dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
#endif

	return S_OK;
}
