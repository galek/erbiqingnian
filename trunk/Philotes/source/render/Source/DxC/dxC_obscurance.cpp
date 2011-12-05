//----------------------------------------------------------------------------
// dx9_obscurance.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


#if ISVERSION(DEVELOPMENT)


#include "list.h"
#include "performance.h"
#include "dxC_primitive.h"
#include "dxC_occlusion.h"
#include "dxC_state.h"
#include "vector.h"
#include "matrix.h"
#include "dxC_target.h"
#include "dxC_render.h"
#include "dxC_model.h"
#include "dxC_effect.h"
#include "e_main.h"
#include "dxC_VertexDeclarations.h"

#include "dxC_obscurance.h"




//#define DEBUG_OBSCURANCE_DISPLAY
//#define DEBUG_OBSCURANCE_PRESENT
//#define TRACE_OBSCURANCE_TIMERS

#ifdef TRACE_OBSCURANCE_TIMERS
#	define _AO_TIMER_START(str)		TIMER_START2(str);
#	define _AO_TIMER_END()			TIMER_END2(str);
#else
#	define _AO_TIMER_START(str)		
#	define _AO_TIMER_END()			(UINT64)0L
#endif


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define OBSCURANCE_FILE_VERSION_CURRENT		1
#define OBSCURANCE_FILE_MAGIC_NUMBER		0x6ef985db
#define OBSCURANCE_FILE_EXTENSION			"mao"

#define OBSCURANCE_PER_FACE				TRUE
#define OBSCURANCE_QUALITY				4
#define OBSCURANCE_ADD_OFFSET_NORMALS	1
#define NUM_OBSCURANCE_QUERIES			1024

#define OBSCURANCE_RENDER_FOV			(PI / 2.0f)
#define OBSCURANCE_RENDER_RESOLUTION	32
#define OBSCURANCE_RENDER_BBOX_ENLARGE	0.2f
#define OBSCURANCE_RENDER_INFINITY		2000.f
#define OBSCURANCE_BOUNDING_BOX_EPSILON	0.01f

//#define NUM_OBSCURANCE_RAYS				(OBSCURANCE_QUALITY + 1)

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct IndexResult
{
	float fAct;
	float fMax;
};

struct OBSCURANCE_CONTEXT
{
	D3D_MODEL_DEFINITION * pModelDef;
	D3D_MESH_DEFINITION ** ppMeshes;
	D3D_MODEL_DEFINITION * pBoundingEdgeModelDef;
	D3D_MESH_DEFINITION ** ppBoundingEdgeMeshes;
	IndexResult			** ppVBResults;
	BOUNDING_BOX		   tAOBox;
	float fNear;
	float fFar;
};

struct OBSCURANCE_RESULT
{
	int		nQuery;
	float	fAO;
	float	fWeight;
	int		nVFIndex;

	void Reset()
	{
		nQuery		= INVALID_ID;
		fAO			= 0.f;
		fWeight		= 0.f;
		nVFIndex	= -1;
	}
};

struct OBSCURANCE_RAY
{
	VECTOR	v;
	float	weight;
};

struct OBSCURANCE_FALLOFF
{
	float	fNear;
	float	fFar;
	float	fWeight;
};

// the obscurance file header is just a copy of the basic file header, at least for now
typedef FILE_HEADER		OBSCURANCE_FILE_HEADER;

//----------------------------------------------------------------------------
struct OBSCURANCE_VERTEX_BUFFER
{
	DWORD				dwNumVertices;
	DWORD				dwNumStreams;
	SAFE_PTR( DWORD*,	pdwCRC );					// array of CRCs (per-stream)
	SAFE_PTR( BYTE*,	byVertexObscurance );
};

//----------------------------------------------------------------------------
struct OBSCURANCE_DATA
{
	// store data per-vertex buffer, per-vertex
	DWORD									dwNumVertexBuffers;
	SAFE_PTR( OBSCURANCE_VERTEX_BUFFER*,	ptVertexBuffers );
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static OBSCURANCE_RAY				sgvRayDirs[] =
{ //  dir								weight
	{ VECTOR( 1.f, 0.f, 1.f       ),	1.f },		// 45 deg

	//{ VECTOR( 1.f, 0.f, 0.414214f ),	1.f / OBSCURANCE_QUALITY },		// 22.5 deg
	//{ VECTOR( 1.f, 0.f, 2.414214f ),	1.f / OBSCURANCE_QUALITY },		// 67.5 deg
	//{ VECTOR( 1.f, 0.f, 0.577350f ),	1.f / OBSCURANCE_QUALITY },		// 30 deg
	//{ VECTOR( 1.f, 0.f, 1.732051f ),	1.f / OBSCURANCE_QUALITY },		// 60 deg
	//{ VECTOR( 1.f, 0.f, 0.577350f ),	0.5f },		// 30 deg
	//{ VECTOR( 1.f, 0.f, 1.732051f ),	0.5f },		// 60 deg
};
static const int					sgnRayDirs	= arrsize( sgvRayDirs );


static OBSCURANCE_FALLOFF			sgtDists[] =
{ //  near		far			weight
	{ 0.01f,	1000.f,		1.f		},
	//{ 0.01f,	100.f,		2.f		},
	//{ 0.01f,	10.0f,		1.f		},
	//{ 0.01f,	2.0f,		1.f		},
	//{ 0.01f,	0.3f,		1.f		},
};
static const int					sgnDists	= arrsize( sgtDists );


static const int					NUM_OBSCURANCE_RAYS = ( sgnRayDirs * OBSCURANCE_QUALITY + 1 ) * ( OBSCURANCE_ADD_OFFSET_NORMALS * 3 /*+ 1*/ );
static OBSCURANCE_RAY				sgvRays[ NUM_OBSCURANCE_RAYS ];


static BOOL							sgbObscuranceInProgress = FALSE;
static SPD3DCQUERY					sgpD3DQuery[ NUM_OBSCURANCE_QUERIES ];

static PList<int>					sgtFreeQueries;
static PList<OBSCURANCE_RESULT>		sgtQuerying;
static PList<OBSCURANCE_RESULT>		sgtReturned;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static void sGetRotatedRays( OBSCURANCE_RAY * pvRays, const VECTOR & vAxis )
{
	// pvRays is actually the 2nd ray in the global list -- the first being the normal

	int nCount = OBSCURANCE_QUALITY;
	ASSERT_RETURN( nCount != 0 );

	for ( int nSrc = 0; nSrc < sgnRayDirs; nSrc++ )
	{
		// rotate vSrc around vAxis 1/QUALITYth of a circle

		//VECTOR vSrc( 1.f, 0.f, 1.f );
		VECTOR vSrc = sgvRayDirs[nSrc].v;
		VectorNormalize( vSrc );
		float fRadStep = 1.f / nCount * TWOxPI;

		MATRIX mRot;

		int nStart = nSrc * nCount;
		for ( int i = 0; i < nCount; i++ )
		{
			float fRads = i * fRadStep;
			MatrixRotationAxis( mRot, vAxis, fRads );
			MatrixMultiplyNormal( &pvRays[i + nStart].v, &vSrc, &mRot );
			VectorNormalize( pvRays[i + nStart].v );
			pvRays[i + nStart].weight = sgvRayDirs[nSrc].weight;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sIsObscuranceVertFormat( VERTEX_DECL_TYPE eVertDecl )
{
	switch( eVertDecl )
	{
	case VERTEX_DECL_RIGID_64:
	case VERTEX_DECL_RIGID_32:
		return TRUE;
	default:
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceQueriesInit()
{
	for ( int i = 0; i < NUM_OBSCURANCE_QUERIES; i++ )
	{
		V( dxC_OcclusionQueryCreate( &sgpD3DQuery[i] ) );
		sgtFreeQueries.PListPushHead( i );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceRenderInit( OBSCURANCE_CONTEXT & tContext )
{
	ASSERT_RETFAIL( ! sgbObscuranceInProgress );


	// load the high-res version for clip planes
	char szHighResFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PRESULT pr = e_ModelDefinitionStripLODFileName( szHighResFile, DEFAULT_FILE_WITH_PATH_SIZE, tContext.pModelDef->tHeader.pszName );
	V_RETURN( pr );
	if ( pr == S_FALSE )
	{
		// this is the high LOD, so we'll just copy the pointers
		tContext.pBoundingEdgeModelDef = tContext.pModelDef;
		tContext.ppBoundingEdgeMeshes = tContext.ppMeshes;
	}
	else 
	{
		// this is the low LOD, so request the high LOD
		char model_folder[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrGetPath( model_folder, MAX_PATH, szHighResFile );
		int nModelDef = tContext.pModelDef->tHeader.nId;
		V_RETURN( dxC_ModelDefinitionNewFromFileEx( &nModelDef, LOD_HIGH, INVALID_ID, szHighResFile, 
			model_folder, 0.f, MDNFF_FLAG_FORCE_SYNC_LOAD ) );
		ASSERT_RETFAIL( nModelDef != INVALID_ID );
		tContext.pBoundingEdgeModelDef = dxC_ModelDefinitionGet( nModelDef, LOD_HIGH );
		ASSERT_RETFAIL( tContext.pBoundingEdgeModelDef );
		tContext.ppBoundingEdgeMeshes = (D3D_MESH_DEFINITION**)MALLOCZ( NULL, sizeof(D3D_MESH_DEFINITION**) * tContext.pBoundingEdgeModelDef->nMeshCount );
		for ( int i = 0; i < tContext.pBoundingEdgeModelDef->nMeshCount; i++ )
			tContext.ppBoundingEdgeMeshes[i] = dx9_ModelDefinitionGetMesh( tContext.pBoundingEdgeModelDef, i );

		//for ( int i = 0; i < tContext.pBoundingEdgeModelDef->nMeshCount; i++ )
		//{
		//	D3D_MESH_DEFINITION * pMesh = tContext.ppBoundingEdgeMeshes[i];
		//	ASSERT_CONTINUE( pMesh );
		//	V( dx9_MeshRestore( pMesh ) );
		//	pMesh->dwTechniqueFlags = 0;
		//}
	}


	// set up rays
	VECTOR vNormal( 0.f, 0.f, 1.f );
	sgvRays[ 0 ].v		= vNormal;
	sgvRays[ 0 ].weight = 1.f;
	int nRays = 1;
	if ( OBSCURANCE_QUALITY > 0 )
	{
		// add other rays
		sGetRotatedRays( &sgvRays[1], sgvRays[0].v );
		nRays += OBSCURANCE_QUALITY;
	}
#if OBSCURANCE_ADD_OFFSET_NORMALS
	for ( int i = nRays; i < NUM_OBSCURANCE_RAYS; ++i )
		sgvRays[i] = sgvRays[i % nRays];
#endif // OBSCURANCE_ADD_OFFSET_NORMALS

#ifndef DEBUG_OBSCURANCE_DISPLAY

	sgtQuerying.Init();
	sgtReturned.Init();
	sgtFreeQueries.Init();

	// initialize queries
	V( sObscuranceQueriesInit() );

	// create the vertex and index buffers
	ASSERT_RETFAIL( tContext.pModelDef );

	V( dx9_ModelDefinitionRestore( tContext.pModelDef, FALSE, FALSE ) );
	for ( int i = 0; i < tContext.pModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMesh = tContext.ppMeshes[i];
		ASSERT_CONTINUE( pMesh );
		if ( ! dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dx9_MeshRestore( pMesh ) );
		}
		pMesh->dwTechniqueFlags = 0;
	}

	// This has to be an array of pointers to individually-allocated arrays
	tContext.ppVBResults = (IndexResult**) MALLOCZ( NULL, sizeof(IndexResult*) * tContext.pModelDef->nVertexBufferDefCount );
	for ( int nVB = 0; nVB < tContext.pModelDef->nVertexBufferDefCount; nVB++ )
	{
		tContext.ppVBResults[ nVB ] = (IndexResult*) MALLOCZ( NULL, sizeof(IndexResult) * tContext.pModelDef->ptVertexBufferDefs[ nVB ].nVertexCount );
	}

	VECTOR vCenter = 0.5f * ( tContext.pModelDef->tRenderBoundingBox.vMin + tContext.pModelDef->tRenderBoundingBox.vMax );
	tContext.tAOBox.vMin = vCenter;
	tContext.tAOBox.vMax = vCenter;

	V_RETURN( dxC_BeginScene() );


#endif  // DEBUG_OBSCURANCE_DISPLAY

	sgbObscuranceInProgress = TRUE;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceQueriesFree()
{
	for ( int i = 0; i < NUM_OBSCURANCE_QUERIES; i++ )
	{
		sgpD3DQuery[i] = NULL;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCleanupModelDef( D3D_MODEL_DEFINITION * pModelDef, D3D_MESH_DEFINITION ** ppMeshes )
{
	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMesh = ppMeshes[i];
		ASSERT_CONTINUE( pMesh );
		V( dxC_IndexBufferDestroy( pMesh->tIndexBufferDef.nD3DBufferID ) );
		pMesh->tIndexBufferDef.nD3DBufferID = INVALID_ID;
	}

	for ( int i = 0; i < pModelDef->nVertexBufferDefCount; i++ )
	{
		for ( int j = 0; j < DXC_MAX_VERTEX_STREAMS; j++ )
		{
			if ( pModelDef->ptVertexBufferDefs[i].nD3DBufferID[j] != INVALID_ID )
			{
				V( dxC_VertexBufferDestroy( pModelDef->ptVertexBufferDefs[i].nD3DBufferID[j] ) );
				pModelDef->ptVertexBufferDefs[i].nD3DBufferID[j] = INVALID_ID;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceRenderDone( OBSCURANCE_CONTEXT & tContext, BOOL bCleanup )
{
	ASSERT( sgbObscuranceInProgress );
	sgbObscuranceInProgress = FALSE;

#ifndef DEBUG_OBSCURANCE_DISPLAY

	DX9_BLOCK( V( dx9_RestoreD3DStates() ) );

	V( sObscuranceQueriesFree() );

	V( dxC_EndScene() );

	for ( int nVB = 0; nVB < tContext.pModelDef->nVertexBufferDefCount; nVB++ )
	{
		if ( tContext.ppVBResults[ nVB ] )
			FREE( NULL, tContext.ppVBResults[ nVB ] );
	}
	if ( tContext.ppVBResults )
		FREE( NULL, tContext.ppVBResults );


	BOOL bFreeBounding = ( tContext.pModelDef != tContext.pBoundingEdgeModelDef );
	if ( ! bFreeBounding )
	{
		tContext.pBoundingEdgeModelDef = NULL;
		tContext.ppBoundingEdgeMeshes = NULL;
	}

	// destroy the vertex and index buffers
	ASSERT_RETFAIL( tContext.pModelDef );
	if ( bCleanup )
		sCleanupModelDef( tContext.pModelDef, tContext.ppMeshes );
	if ( bFreeBounding )
	{
		ASSERT_RETFAIL( tContext.pBoundingEdgeModelDef );
		if ( bCleanup )
			sCleanupModelDef( tContext.pBoundingEdgeModelDef, tContext.ppBoundingEdgeMeshes );
		FREE( NULL, tContext.ppBoundingEdgeMeshes );
		tContext.ppBoundingEdgeMeshes = NULL;
		tContext.pBoundingEdgeModelDef = NULL;
	}

	sgtFreeQueries.Destroy( NULL );

	// iterate the list and free any remaining?  nothing to free right now
	sgtQuerying.Destroy( NULL );
	sgtReturned.Destroy( NULL );

#endif  // DEBUG_OBSCURANCE_DISPLAY


	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT sObscuranceGetEffect( D3D_EFFECT ** ppEffect, EFFECT_TECHNIQUE ** pptTechnique )
{
	*ppEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_OBSCURANCE ) );
	ASSERT_RETFAIL( *ppEffect );
	if ( ! dxC_EffectIsLoaded( *ppEffect ) )
		return E_FAIL;

	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( *ppEffect, tFeat, pptTechnique ) );
	ASSERT_RETFAIL( *pptTechnique && (*pptTechnique)->hHandle );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDecompressColorToVector ( VECTOR & vOutVector, const DWORD ColorNormal )
{
	// convert from a color
	float fInv255 = 1.0f / 255.0f;
	vOutVector.x = (FLOAT)( ( ColorNormal >> 16 ) & 0x000000ff ) * fInv255;
	vOutVector.y = (FLOAT)( ( ColorNormal >>  8 ) & 0x000000ff ) * fInv255;
	vOutVector.z = (FLOAT)( ( ColorNormal >>  0 ) & 0x000000ff ) * fInv255;

	// transform to [-1,1]
	vOutVector.x = vOutVector.x * 2.0f - 1.0f;
	vOutVector.y = vOutVector.y * 2.0f - 1.0f;
	vOutVector.z = vOutVector.z * 2.0f - 1.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetObscuranceDest(
	DWORD *& pdwDest,
	BYTE * pbNorms,
	int nNormStride,
	VERTEX_DECL_TYPE eVertType,
	int nIndex )
{
	BYTE * vN = (BYTE*)( pbNorms +  nIndex * nNormStride );

	if ( eVertType == VERTEX_DECL_RIGID_64 )
		pdwDest = &( (VS_BACKGROUND_INPUT_64_STREAM_2*)vN )->Normal;
	else if ( eVertType == VERTEX_DECL_RIGID_32 )
		pdwDest = &( (VS_BACKGROUND_INPUT_32_STREAM_2*)vN )->Normal;
	else
		return E_INVALIDARG;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetCenterAndNormal(
	VECTOR & C,
	VECTOR & N,
	VECTOR * P[ 3 ],
	DWORD ** pdwDest,
	BYTE * pbPos,
	int nPosStride,
	BYTE * pbNorms,
	int nNormStride,
	VERTEX_DECL_TYPE eVertType,
	OBSCURANCE_CONTEXT & tContext,
	int nIndex = -1,
	int * nFaceInds = NULL )
{
	//VECTOR * P[ 3 ] = { NULL };
	P[0] = NULL;
	P[1] = NULL;
	P[2] = NULL;

	if ( nFaceInds )
	{
		P[0] = (VECTOR*)( pbPos + nFaceInds[0] * nPosStride );
		P[1] = (VECTOR*)( pbPos + nFaceInds[1] * nPosStride );
		P[2] = (VECTOR*)( pbPos + nFaceInds[2] * nPosStride );

		ASSERT_RETFAIL( P[ 0 ] && P[ 1 ] && P[ 2 ] );

		BoundingBoxExpandByPoint( &tContext.tAOBox, P[0] );
		BoundingBoxExpandByPoint( &tContext.tAOBox, P[1] );
		BoundingBoxExpandByPoint( &tContext.tAOBox, P[2] );
	}
	else
	{
		ASSERT_RETFAIL( nIndex >= 0 );
		P[0] = (VECTOR*)( pbPos + nIndex * nPosStride );
		ASSERT_RETFAIL( P[0] );

		BoundingBoxExpandByPoint( &tContext.tAOBox, P[0] );
	}

	PRESULT pr = E_FAIL;

	// get normal
	if ( ! pbNorms )
	{
		// no normals?  nowhere to stick the obscurance value!
		pr = E_FAIL;
	}
	else
	{
		N = VECTOR(0,0,0);

		// average 3 vertex normals
		BYTE * vNs[ 3 ] = { NULL };

		if ( nFaceInds )
		{
			vNs[0] = (BYTE*)( pbNorms +  ( nFaceInds[0] * nNormStride ) );
			vNs[1] = (BYTE*)( pbNorms +  ( nFaceInds[1] * nNormStride ) );
			vNs[2] = (BYTE*)( pbNorms +  ( nFaceInds[2] * nNormStride ) );
		}
		else
		{
			vNs[0] = (BYTE*)( pbNorms +  nIndex * nNormStride );
		}

		int nCount = nFaceInds ? 3 : 1;
		for ( int i = 0; i < nCount; i++ )
		{
			if ( eVertType == VERTEX_DECL_RIGID_64 )
				pdwDest[i] = &( (VS_BACKGROUND_INPUT_64_STREAM_2*)vNs[i] )->Normal;
			else if ( eVertType == VERTEX_DECL_RIGID_32 )
				pdwDest[i] = &( (VS_BACKGROUND_INPUT_32_STREAM_2*)vNs[i] )->Normal;

			VECTOR vN;
			sDecompressColorToVector( vN, *pdwDest[i] );
			N += vN;
		}
		VectorNormalize( N );

		// good to go!
		pr = S_OK;
	}

	if ( nFaceInds )
	{
		// find face center
		C = ( *P[0] + *P[1] + *P[2] ) / 3.f;
	}
	else
	{
		C = *P[0];
	}

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTransformRays( OBSCURANCE_RAY * pvRays, const VECTOR & N )
{
	MATRIX mRot;
	VECTOR vAxis;
	VectorCross( vAxis, N, sgvRays[0].v );
	VectorNormalize( vAxis );
	float fCos = VectorDot( N, sgvRays[0].v );
	float fAngle = acosf( fCos );
	MatrixRotationAxis( mRot, vAxis, -fAngle );

	for ( int nRay = 0; nRay < NUM_OBSCURANCE_RAYS; nRay++ )
	{
		MatrixMultiplyNormal( &pvRays[ nRay ].v, &sgvRays[ nRay ].v, &mRot );
		pvRays[nRay].weight = sgvRays[nRay].weight;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRenderGroundPlane(
	const BOUNDING_BOX * pBBox,
	D3D_EFFECT * pEffect,
	EFFECT_TECHNIQUE & tTechnique,
	MATRIX * pmView,
	MATRIX * pmProj )
{
	// update offset and scale matrix to match bounding box
	MATRIX matScale, matTranslate;
	MATRIX matXform;

	// enlarge bounding box to avoid intersection issues
	VECTOR vEnlarge( OBSCURANCE_RENDER_BBOX_ENLARGE, OBSCURANCE_RENDER_BBOX_ENLARGE, OBSCURANCE_RENDER_BBOX_ENLARGE );
	VECTOR vScale( pBBox->vMax - pBBox->vMin );
	vScale += vEnlarge;
	// shrink this bbox vertically to include just the floor
	vScale.z = 0.f;

	VECTOR vTrans = pBBox->vMin;
	vTrans -= vEnlarge * 0.5f;

	// drop the bottom only a little bit
	vTrans.z = pBBox->vMin.z - vEnlarge.z * 0.25f;

	MatrixScale( &matScale, &vScale );
	MatrixTranslation( &matTranslate, &vTrans );

	MatrixMultiply( &matXform, &matScale, &matTranslate );

	MATRIX mTemp, mWVP;
	MatrixMultiply( &mTemp, &matXform, pmView );
	MatrixMultiply( &mWVP,  &mTemp,    pmProj );

	V( dx9_EffectSetMatrix( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &mWVP ) );

	V_RETURN( dxC_SetIndices( gtBoundingBoxIndexBuffer, TRUE ) );
	V_RETURN( dxC_SetStreamSource( gtBoundingBoxVertexBuffer, 0 ) );

	V_RETURN( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, NUM_BOUNDING_BOX_LIST_VERTICES, 0, DP_GETPRIMCOUNT_TRILIST( NUM_BOUNDING_BOX_LIST_INDICES ), pEffect, METRICS_GROUP_DEBUG ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRenderBoundingBox(
	const BOUNDING_BOX * pBBox,
	MATRIX * pmView,
	MATRIX * pmProj,
	MATRIX * pmWorld = NULL )
{
	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SIMPLE ) );

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );

	MATRIX matTrans;
	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	if ( ! pmWorld )
		pmWorld = &mIdentity;

	// update offset and scale matrix to match bounding box
	MATRIX matScale, matTranslate;

	// enlarge bounding box to avoid intersection issues
	VECTOR vEnlarge( OBSCURANCE_RENDER_BBOX_ENLARGE, OBSCURANCE_RENDER_BBOX_ENLARGE, OBSCURANCE_RENDER_BBOX_ENLARGE );
	VECTOR vScale( pBBox->vMax - pBBox->vMin );
	vScale += vEnlarge;

	VECTOR vTrans = pBBox->vMin;
	vTrans -= vEnlarge * 0.5f;

	MatrixScale( &matScale, &vScale );
	MatrixTranslation( &matTranslate, &vTrans );
	MatrixMultiply(  &matTrans, &matScale, &matTranslate );
	MatrixMultiply(  &matTrans, &matTrans, pmWorld );

	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_WORLDMAT, &matTrans ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_VIEWMAT,  pmView ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_PROJMAT,  pmProj ) );

	ASSERT_RETFAIL( nPassCount == 1 ); // only one pass

	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	V( dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE ) );
	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );

	V_RETURN( dx9_SetFVF( BOUNDING_BOX_FVF ) ); 
	V_RETURN( dxC_SetIndices( gtBoundingBoxIndexBuffer, TRUE ) );
	V_RETURN( dxC_SetStreamSource( gtBoundingBoxVertexBuffer, 0 ) );

	V_RETURN( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, NUM_BOUNDING_BOX_LIST_VERTICES, 0, DP_GETPRIMCOUNT_TRILIST( NUM_BOUNDING_BOX_LIST_INDICES ), pEffect, METRICS_GROUP_DEBUG ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGatherResult()
{
	// grab oldest request
	PList<OBSCURANCE_RESULT>::USER_NODE * pResult = NULL;
	pResult = sgtQuerying.GetPrev( NULL );
	if ( ! pResult )
		return S_FALSE;

	D3DC_QUERY_DATA dwPixels = 0;
	int nWaiting = 0;

	if ( sgpD3DQuery[ pResult->Value.nQuery ] )
	{
		BOUNDED_WHILE( S_OK != dxC_OcclusionQueryGetData( sgpD3DQuery[pResult->Value.nQuery], dwPixels, TRUE ), 100000 )
		{
			//if ( nWaiting == 0 )
			//	trace( "STALL: query %d spinning on GetData()... ", pResult->Value.nQuery );
			nWaiting++;

			Sleep(0);
		}
	}

	//if ( nWaiting > 0 )
	//	trace( "%8d loops\n", nWaiting );

	DWORD nMax = OBSCURANCE_RENDER_RESOLUTION * OBSCURANCE_RENDER_RESOLUTION;
	float fAO = (float)dwPixels / (float)( nMax );


	OBSCURANCE_RESULT tResult;
	BOOL bPopped = sgtQuerying.PopTail( tResult );
	ASSERT_RETFAIL( bPopped );

	ASSERT_RETFAIL( tResult.nQuery != INVALID_ID );
	sgtFreeQueries.PListPushHead( tResult.nQuery );
	tResult.nQuery	= INVALID_ID;
	tResult.fAO		= SATURATE( fAO );

	sgtReturned.PListPushHead( tResult );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sGetFreeQuery( BOOL bWait = TRUE )
{
	_AO_TIMER_START( "Get free query" );

	int nQuery = INVALID_ID;
	sgtFreeQueries.PopTail( nQuery );

	BOUNDED_WHILE( nQuery == INVALID_ID && bWait, 100000000 )
	{
		// if no free queries, spin until there is one
		sGatherResult();
		sgtFreeQueries.PopTail( nQuery );
	}

	_AO_TIMER_END();

	return nQuery;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRenderAlongRay(
	D3D_EFFECT * pEffect,
	EFFECT_TECHNIQUE & tTechnique,
	const OBSCURANCE_RAY & vRay,
	const OBSCURANCE_FALLOFF & vFalloff,
	const VECTOR & vFrom,
	const OBSCURANCE_RESULT & tTemplate,
	const OBSCURANCE_CONTEXT & tContext )
{
	// 1. render all of the meshes in the model def
	// 2. start query
	// 3. render modeldef bounding box
	// 4. end query

	//int nLOD = LOD_HIGH;	// CHB 2006.12.08

	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );


	// set up view and proj matrices
	MATRIX mView, mProj;
	VECTOR vAt = vFrom + vRay.v;
	VECTOR vUp;
	VECTOR vRight(0,1,0);
	// try to find a valid "up"
	VectorCross( vUp, vRight, vRay.v );
	if ( VectorIsZero( vUp ) )
	{
		// stopgap for bad input vectors
		vUp = VECTOR( 0, 0, 1 );
	}
	VectorNormalize( vUp );
	MatrixMakeView( mView, vFrom, vAt, vUp );
	//MatrixMakePerspectiveProj( mProj, OBSCURANCE_RENDER_FOV, 1.0f, tContext.fNear, tContext.fFar );
	//MatrixMakePerspectiveProj( mProj, OBSCURANCE_RENDER_FOV, 1.0f, 0.01f, 1000.f );
	MatrixMakePerspectiveProj( mProj, OBSCURANCE_RENDER_FOV, 1.0f, vFalloff.fNear, vFalloff.fFar );
	MATRIX mWVP;
	MatrixMultiply( &mWVP, &mView, &mProj );


	// set states
	DX9_BLOCK( V( dx9_RestoreD3DStates() ) );
	DX9_BLOCK( V( dxC_SetRenderState( D3DRS_SHADEMODE,				D3DSHADE_FLAT ) ) );
	DX9_BLOCK( V( dxC_SetRenderState( D3DRS_LIGHTING,				FALSE ) ) );
	V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,		FALSE ) );
	V( dxC_SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE ) );
	V( dxC_SetRenderState( D3DRS_ALPHAREF,				1L ) );
	V( dxC_SetRenderState( D3DRS_ALPHAFUNC,				D3DCMP_GREATEREQUAL ) );
	V( dxC_SetRenderState( D3DRS_CULLMODE,				D3DCULL_NONE ) );
#ifndef DEBUG_OBSCURANCE_PRESENT
	V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE,		0 ) );
#endif


	// setup targets
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
	V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eDT ) );
	D3DC_VIEWPORT tVP;
	tVP.MinDepth = 0.f;
	tVP.MaxDepth = 1.f;
	tVP.TopLeftX = 0;
	tVP.TopLeftY = 0;
	tVP.Width    = OBSCURANCE_RENDER_RESOLUTION;
	tVP.Height   = OBSCURANCE_RENDER_RESOLUTION;
	V( dxC_SetViewport( tVP ) );


	// clear the scene depth
	V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL ) );

	// set vertex decl
	V( dxC_SetVertexDeclaration( VERTEX_DECL_RIGID_ZBUFFER ) );


	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, &tTechnique, &nPassCount ) );
	ASSERT_RETFAIL( nPassCount == 1 );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	V( sRenderGroundPlane(
		&tContext.pModelDef->tRenderBoundingBox,
		pEffect,
		tTechnique,
		&mView,
		&mProj ) );

	V( dx9_EffectSetMatrix ( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &mWVP ) );

	for ( int nMesh = 0; nMesh < tContext.pModelDef->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMesh = tContext.ppMeshes[ nMesh ];
		if ( ! pMesh )
			continue;
		if ( pMesh->dwFlags & (MESH_FLAG_NODRAW) )
			continue;
		if ( pMesh->dwFlags & (MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL) )
			continue;
		if ( pMesh->dwFlags & (MESH_FLAG_SILHOUETTE | MESH_FLAG_ALPHABLEND) )
			continue;
		if ( 0 == pMesh->nVertexCount )
			continue;

		ASSERT_RETFAIL( pMesh->nVertexBufferIndex < tContext.pModelDef->nVertexBufferDefCount );
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &tContext.pModelDef->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

		if ( ! dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) )
			continue;

		D3D_VERTEX_BUFFER_DEFINITION tVBDef;

		switch ( pVertexBufferDef->eVertexType )
		{
		//case VERTEX_DECL_RIGID_16:
		case VERTEX_DECL_RIGID_32:
		case VERTEX_DECL_RIGID_64:
			tVBDef.eVertexType = VERTEX_DECL_RIGID_ZBUFFER;
			tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
			break;
		default:
			continue;
		}

		V_CONTINUE( dxC_SetStreamSource( tVBDef, 0, pEffect ) );
		V_CONTINUE( dxC_SetIndices( pMesh->tIndexBufferDef, TRUE ) );

		if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, pMesh->wTriangleCount, pEffect, METRICS_GROUP_UNKNOWN ) );
		} else {
			V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, pMesh->wTriangleCount, pEffect, METRICS_GROUP_UNKNOWN ) );
		}
	}


	// select the query
	int nQuery = sGetFreeQuery();
	ASSERT_RETFAIL( IS_VALID_INDEX( nQuery, NUM_OBSCURANCE_QUERIES ) );


	// create the result struct
	OBSCURANCE_RESULT tResult = tTemplate;
	tResult.nQuery = nQuery;
	// store the weight in fAO for later application
	tResult.fWeight = vRay.weight * vFalloff.fWeight;
	sgtQuerying.PListPushHead( tResult );


	// begin the query
	SPD3DCQUERY pQuery = sgpD3DQuery[ nQuery ];
	if ( pQuery )
	{
		V_RETURN( dxC_OcclusionQueryBegin( pQuery ) );
	}

	//MatrixMakePerspectiveProj( mProj, OBSCURANCE_RENDER_FOV, 1.0f, 0.01f, 1000.f );
	MatrixMakePerspectiveProj( mProj, OBSCURANCE_RENDER_FOV, 1.0f, vFalloff.fNear, OBSCURANCE_RENDER_INFINITY );

	// render bounding box
	V( sRenderBoundingBox( &tContext.pModelDef->tRenderBoundingBox, &mView, &mProj ) );


	// end the query
	if ( pQuery )
	{
		V_RETURN( dxC_OcclusionQueryEnd( pQuery ) );
	}


#ifdef DEBUG_OBSCURANCE_PRESENT
	{
		V( dxC_EndScene() );

		PRESULT pr;
		DX9_BLOCK
			(
			V_SAVE( pr, dxC_GetD3DSwapChain()->Present( NULL, NULL, NULL, NULL, 0 ) );
		);
		DX10_BLOCK
			(
			V_SAVE( pr, dxC_GetD3DSwapChain()->Present(0, 0) );
		);

		V( dxC_BeginScene() );
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sGetObscuranceAsUBYTE( BYTE * pbyDest, float fAO )
{
	*pbyDest = (BYTE)( (DWORD)( fAO * 255.f ) & 0xff );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sStoreObscuranceInDWORDAlpha( DWORD * pdwDest, float fAO )
{
	*pdwDest = ( 0x00ffffff & *pdwDest )  |  ( ( (DWORD)( fAO * 255.f ) & 0xff ) << 24 );
}

static void sStoreObscuranceInDWORDAlpha( DWORD * pdwDest, BYTE byAO )
{
	*pdwDest = ( 0x00ffffff & *pdwDest )  |  ( (DWORD)byAO << 24 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sComputeMeshObscurance( int nMeshIndex, OBSCURANCE_CONTEXT & tContext )
{
	D3D_MESH_DEFINITION * pMesh = tContext.ppMeshes[ nMeshIndex ];
	if ( ! IS_VALID_INDEX( pMesh->nVertexBufferIndex, tContext.pModelDef->nVertexBufferDefCount ) )
		return E_FAIL;

#ifndef DEBUG_OBSCURANCE_DISPLAY
	ASSERT_RETFAIL( tContext.ppVBResults );
	ASSERT_RETFAIL( tContext.ppVBResults[ pMesh->nVertexBufferIndex ] );
#endif

	D3D_VERTEX_BUFFER_DEFINITION * pVBDef = &tContext.pModelDef->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

	if ( pVBDef->eVertexType == VERTEX_DECL_RIGID_16 )
	{
		// Currently, we're sticking the obscurance value in the unused high 8-bits of the DWORD normal in
		// rigid 32 and 64. Since rigid 16 doesn't have normals, we can forgo obscurance on the grounds that
		// there will be no meaningful lighting anyway.
		return S_FALSE;
	}
	ASSERT_RETVAL( sIsObscuranceVertFormat( pVBDef->eVertexType ), S_FALSE );

	// for each vertex or face
	BOOL bPerFace = OBSCURANCE_PER_FACE;


	int nIndexCount = pMesh->wTriangleCount * 3;

	INDEX_BUFFER_TYPE * pIndices = (INDEX_BUFFER_TYPE*)pMesh->tIndexBufferDef.pLocalData;
	ASSERT( pIndices || nIndexCount <= 0 );

	if ( ! pIndices || nIndexCount <= 0 )
		bPerFace = FALSE;

#ifdef DEBUG_OBSCURANCE_DISPLAY
	if ( pMesh->tIndexBufferDef.nD3DBufferID == INVALID_ID )
		return S_FALSE;
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( pMesh->tIndexBufferDef.nD3DBufferID );
	V( dxC_MapEntireBuffer( pIB->pD3DIndices, 0, 0, (void**)&pIndices ) );
#endif


	int nNormalStream;

	switch ( pVBDef->eVertexType )
	{
	case VERTEX_DECL_RIGID_64:
		nNormalStream = 2;
		break;
	case VERTEX_DECL_RIGID_32:
		nNormalStream = 2;
		break;
	default:
		return E_FAIL;
	}
	ASSERT_RETFAIL( nNormalStream >= 0 )


	BYTE * pbPos = (BYTE*)pVBDef->pLocalData[ 0 ];
	UINT nPosStride = dxC_GetVertexSize( 0, pVBDef );

	BYTE * pbNorms = (BYTE*)pVBDef->pLocalData[ nNormalStream ];
	UINT nNormStride = dxC_GetVertexSize( nNormalStream, pVBDef );


#ifdef DEBUG_OBSCURANCE_DISPLAY

	D3D_VERTEX_BUFFER * pVB0 = dxC_VertexBufferGet( pVBDef->nD3DBufferID[0] );
	V( dxC_MapEntireBuffer( pVB0->pD3DVertices, 0, 0, (void**)&pbPos ) );
	D3D_VERTEX_BUFFER * pVB1 = dxC_VertexBufferGet( pVBDef->nD3DBufferID[nNormalStream] );
	V( dxC_MapEntireBuffer( pVB1->pD3DVertices, 0, 0, (void**)&pbNorms ) );

#else

	D3D_EFFECT * pEffect;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( sObscuranceGetEffect( &pEffect, &ptTechnique ) );

	sgtReturned.Clear();
	sgtQuerying.Clear();
	ASSERT( sgtFreeQueries.Count() == NUM_OBSCURANCE_QUERIES );

#endif

	int nInc;
	int nMax;
	if ( bPerFace )
	{
		nInc = 3;
		nMax = nIndexCount;
	}
	else
	{
		nInc = 1;
		nMax = pVBDef->nVertexCount;
	}


	{
		//TIMER_START2( "Render loop" );

		for ( int nIndex = 0; nIndex < nMax; nIndex += nInc )
		{
			VECTOR C, N;
			VECTOR * P[3];
			DWORD * pdwDest[3] = { NULL, NULL, NULL };

			int * pnFaces = NULL;
			int nFace[3];
			if ( bPerFace )
			{
				nFace[0] = pIndices[ nIndex + 0 ] + pMesh->nVertexStart;
				nFace[1] = pIndices[ nIndex + 1 ] + pMesh->nVertexStart;
				nFace[2] = pIndices[ nIndex + 2 ] + pMesh->nVertexStart;
				pnFaces = nFace;
			}

			V_RETURN( sGetCenterAndNormal( C, N, P, pdwDest, pbPos, nPosStride, pbNorms, nNormStride, pVBDef->eVertexType, tContext, nIndex, pnFaces ) );


			OBSCURANCE_RAY vRays[ NUM_OBSCURANCE_RAYS ];
			sTransformRays( vRays, N );

#ifdef DEBUG_OBSCURANCE_DISPLAY

			for ( int nRay = 0; nRay < NUM_OBSCURANCE_RAYS; nRay++ )
			{
				VECTOR vEnd = C + ( vRays[nRay].v * 0.15f * vRays[nRay].weight );
				V( e_PrimitiveAddLine( 0, &C, &vEnd ) );
			}

#else

			VECTOR vFrom[ NUM_OBSCURANCE_RAYS ];
			int nCenterRays = 0; // OBSCURANCE_QUALITY + 1;

			for ( int i = 0; i < nCenterRays; i++ )
			{
				vFrom[i] = C;
			}
			if ( P[1] && P[2] )
			{
				for ( int i = nCenterRays; i < NUM_OBSCURANCE_RAYS; i++ )
				{
					// offset the "from" for these rays towards the appropriate vertex
					//int nVert = (i-nCenterRays) / (OBSCURANCE_QUALITY+1);
					int nVert = i % 3;
					vFrom[i] = ( C + *P[nVert] ) * 0.5f;
				}
			}

			OBSCURANCE_RESULT tTemplateResult;
			tTemplateResult.Reset();
			tTemplateResult.nVFIndex = nIndex / nInc;

			//trace( "  Inner loop: %8d/%8d [%d renders]\n", nIndex / nInc, nMax / nInc, sgnDists * NUM_OBSCURANCE_RAYS );
			for ( int f = 0; f < sgnDists; f++ )
			{
				for ( int i = 0; i < NUM_OBSCURANCE_RAYS; i++ )
				{
					V( sRenderAlongRay( pEffect, *ptTechnique, vRays[i], sgtDists[f], vFrom[i], tTemplateResult, tContext ) );
				}
			}

			//if ( 0 == ( nIndex % ( nInc * 10 ) ) )
			//{
			//	AppCommonRunMessageLoop();
			//}
#endif
		}

		//UINT64 nMS = TIMER_END2();
	}


#ifndef DEBUG_OBSCURANCE_DISPLAY

	{
		_AO_TIMER_START( "Gather" );

		// gather all results
		BOUNDED_WHILE( S_OK == sGatherResult(), 1000000000 )
			;
		ASSERT( sgtReturned.Count() == ( nMax / nInc * NUM_OBSCURANCE_RAYS * sgnDists ) );

		_AO_TIMER_END();
	}


	// allocate a list of summed returned values
	int nResults = nMax / nInc;

	IndexResult * pResults = (IndexResult*)MALLOCZ( NULL, sizeof(IndexResult) * nResults );
	ASSERT_RETFAIL(pResults);


	{
		_AO_TIMER_START( "Build list" );

		PList<OBSCURANCE_RESULT>::USER_NODE * pOR = sgtReturned.GetNext( NULL );
		while ( pOR )
		{
			OBSCURANCE_RESULT & tOR = pOR->Value;
			pOR = sgtReturned.GetNext( pOR );

			ASSERT_CONTINUE( IS_VALID_INDEX( tOR.nVFIndex, nResults ) );
			pResults[ tOR.nVFIndex ].fAct += tOR.fAO * tOR.fWeight;
			pResults[ tOR.nVFIndex ].fMax += 1.f	 * tOR.fWeight;
		}

		_AO_TIMER_END();
	}


	IndexResult * pPerVertex = NULL;

	if ( bPerFace )
	{
		_AO_TIMER_START( "Per face" );

		// If this is per-face, we need to make a per-vertex list and average the accessibility
		// of each face to which a vertex belongs.

		// Build into this staging list, then sum it with the existing list for the vertex buffer (inter-mesh).
		pPerVertex = (IndexResult*) MALLOCZ( NULL, sizeof(IndexResult) * pVBDef->nVertexCount );

		for ( int nIndex = 0; nIndex < nIndexCount; nIndex += nInc )
		{
			int nFace[3];
			nFace[0] = pIndices[ nIndex + 0 ] + pMesh->nVertexStart;
			nFace[1] = pIndices[ nIndex + 1 ] + pMesh->nVertexStart;
			nFace[2] = pIndices[ nIndex + 2 ] + pMesh->nVertexStart;


			// Add the contribution weighted by the area of the triangle
			VECTOR * pVerts = (VECTOR*)pbPos;
			VECTOR vEdge1 = pVerts[ nFace[0] ] - pVerts[ nFace[1] ];
			VECTOR vEdge2 = pVerts[ nFace[1] ] - pVerts[ nFace[2] ];
			VECTOR vCross;
			VectorCross( vCross, vEdge1, vEdge2 );
			float fTriArea = VectorLength( vCross );

			float fAct = pResults[ nIndex/3 ].fAct * fTriArea;
			float fMax = pResults[ nIndex/3 ].fMax * fTriArea;

			pPerVertex[ nFace[0] ].fAct += fAct;
			pPerVertex[ nFace[1] ].fAct += fAct;
			pPerVertex[ nFace[2] ].fAct += fAct;

			pPerVertex[ nFace[0] ].fMax += fMax;
			pPerVertex[ nFace[1] ].fMax += fMax;
			pPerVertex[ nFace[2] ].fMax += fMax;
		}

		_AO_TIMER_END();
	}
	else
	{
		// If this is per-vertex, we have our list.

		pPerVertex = pResults;
		pResults = NULL;	// so we don't double-free
	}


	// Add these into the vertex buffer list.
	for ( int nVert = 0; nVert < pVBDef->nVertexCount; nVert++ )
	{
		tContext.ppVBResults[ pMesh->nVertexBufferIndex ][ nVert ].fAct += pPerVertex[ nVert ].fAct;
		tContext.ppVBResults[ pMesh->nVertexBufferIndex ][ nVert ].fMax += pPerVertex[ nVert ].fMax;
	}
	


	if ( pPerVertex )
		FREE( NULL, pPerVertex );
	if ( pResults )
		FREE( NULL, pResults );

#else
	if ( pVB1 )
		dxC_Unmap( pVB1->pD3DVertices );
	dxC_Unmap( pVB0->pD3DVertices );
	dxC_Unmap( pIB->pD3DIndices );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sExtractBoundingPlanes(
	PLANE *& ptPlanes,
	int & nPlanes,
	OBSCURANCE_CONTEXT & tContext )
{
	//ORIENTED_BOUNDING_BOX OBB;
	//MakeOBBFromAABB( OBB, &tBBox );
	//MakeOBBPlanes( tPlanes, OBB );

	// find clip planes, turn into planes
	ASSERT_RETINVALIDARG( ! ptPlanes );

	nPlanes = 0;

	for ( int i = 0; i < tContext.pBoundingEdgeModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMesh = tContext.ppBoundingEdgeMeshes[i];
		ASSERT_CONTINUE( pMesh );
		if ( 0 == ( pMesh->dwFlags & MESH_FLAG_CLIP ) )
			continue;
		if ( 0 == pMesh->nVertexCount )
			continue;

		ASSERT_CONTINUE( pMesh->nVertexBufferIndex < tContext.pBoundingEdgeModelDef->nVertexBufferDefCount );
		ASSERT_CONTINUE( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &tContext.pBoundingEdgeModelDef->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];
		if ( ! pVertexBufferDef->pLocalData )
			continue;

		int nIndexCount = pMesh->wTriangleCount * 3;
		INDEX_BUFFER_TYPE * pIndices = (INDEX_BUFFER_TYPE*)pMesh->tIndexBufferDef.pLocalData;
		ASSERT_CONTINUE( nIndexCount && pIndices );

		BYTE * pbPos = (BYTE*)pVertexBufferDef->pLocalData[ 0 ];
		UINT nPosStride = dxC_GetVertexSize( 0, pVertexBufferDef );

		int nFace[3];

		nFace[0] = pIndices[ 0 ] + pMesh->nVertexStart;
		nFace[1] = pIndices[ 1 ] + pMesh->nVertexStart;
		nFace[2] = pIndices[ 2 ] + pMesh->nVertexStart;

		VECTOR * P[3];
		P[0] = (VECTOR*)( pbPos + nFace[0] * nPosStride );
		P[1] = (VECTOR*)( pbPos + nFace[1] * nPosStride );
		P[2] = (VECTOR*)( pbPos + nFace[2] * nPosStride );

		ASSERT_CONTINUE( P[ 0 ] && P[ 1 ] && P[ 2 ] );

		// need to allocate more
		nPlanes++;

		ptPlanes = (PLANE*)REALLOC( NULL, ptPlanes, sizeof(PLANE) * nPlanes );
		ASSERT_RETFAIL( ptPlanes );
		PlaneFromPoints( &(ptPlanes[nPlanes-1]), P[0], P[1], P[2] );
	}

	return S_OK;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetBoundingPlaneEdgeOverride(
	float & fAO,
	const BYTE * pbPos,
	int nPosStride,
	const PLANE * tPlanes,
	int nPlanes,
	int nVert )
{
	VECTOR * pvPos = (VECTOR*)( pbPos + nVert * nPosStride );

	for ( int i = 0; i < nPlanes; i++ )
	{
		float fDot = PlaneDotCoord( &tPlanes[i], pvPos );
		if ( fabs( fDot ) < OBSCURANCE_BOUNDING_BOX_EPSILON )
		{
			// point at the edge -- make it fully accessible
			fAO = 1.f;
			break;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFinalizeVertexBuffers(
	const OBSCURANCE_DATA * pData,
	OBSCURANCE_CONTEXT & tContext )
{
	_AO_TIMER_START( "Finalize vbuffers" );

	ASSERT_RETINVALIDARG( pData );

	PLANE * tPlanes = NULL;
	int nPlanes = 0;
	V( sExtractBoundingPlanes(
		tPlanes,
		nPlanes,
		tContext ) );

	// For each vertex buffer, finalize the average and commit it
	for ( int nVB = 0; nVB < tContext.pModelDef->nVertexBufferDefCount; nVB++ )
	{
		if ( ! pData->ptVertexBuffers[ nVB ].byVertexObscurance )
			continue;
		D3D_VERTEX_BUFFER_DEFINITION * pVBDef = &tContext.pModelDef->ptVertexBufferDefs[ nVB ];

		if ( pVBDef->eVertexType == VERTEX_DECL_RIGID_16 )
		{
			// Currently, we're sticking the obscurance value in the unused high 8-bits of the DWORD normal in
			// rigid 32 and 64. Since rigid 16 doesn't have normals, we can forgo obscurance on the grounds that
			// there will be no meaningful lighting anyway.
			continue;
		}

		ASSERT_CONTINUE( sIsObscuranceVertFormat( pVBDef->eVertexType ) );

		int nPosStream = 0;
		int nNormalStream;
		switch ( pVBDef->eVertexType )
		{
		case VERTEX_DECL_RIGID_64:
			nNormalStream = 2;
			break;
		case VERTEX_DECL_RIGID_32:
			nNormalStream = 2;
			break;
		default:
			return E_FAIL;
		}
		ASSERT_CONTINUE( nNormalStream >= 0 )

		BYTE * pbPos = (BYTE*)pVBDef->pLocalData[ nPosStream ];
		UINT nPosStride = dxC_GetVertexSize( nPosStream, pVBDef );

		BYTE * pbNorms = (BYTE*)pVBDef->pLocalData[ nNormalStream ];
		UINT nNormStride = dxC_GetVertexSize( nNormalStream, pVBDef );


		// Now we can iterate per-vertex, finalize the average, and store off the accessibility value.
		for ( int nVert = 0; nVert < pVBDef->nVertexCount; nVert++ )
		{
			DWORD * pdwDest = NULL;
			V( sGetObscuranceDest(
				pdwDest,
				pbNorms,
				nNormStride,
				pVBDef->eVertexType,
				nVert ) );
			ASSERT_CONTINUE( pdwDest );

			// Store the result
			sStoreObscuranceInDWORDAlpha( pdwDest, pData->ptVertexBuffers[ nVB ].byVertexObscurance[ nVert ] );
		}
	}

	if ( tPlanes )
		FREE( NULL, tPlanes );

	UINT64 nMS = _AO_TIMER_END();
	trace( "  Finalize vbuffers:  %1.2f seconds\n",
		nMS / 1000.f );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAssembleVertexBuffers(
	OBSCURANCE_CONTEXT & tContext,
	OBSCURANCE_DATA & tData )
{
	_AO_TIMER_START( "Assemble vbuffers" );

	ASSERT_RETFAIL( tContext.ppVBResults );

	ZeroMemory( &tData, sizeof(OBSCURANCE_DATA) );

	PLANE * tPlanes = NULL;
	int nPlanes = 0;
	V( sExtractBoundingPlanes(
		tPlanes,
		nPlanes,
		tContext ) );

	tData.dwNumVertexBuffers = tContext.pModelDef->nVertexBufferDefCount;
	if ( ! tData.dwNumVertexBuffers )
		return S_FALSE;
	tData.ptVertexBuffers = (OBSCURANCE_VERTEX_BUFFER*)MALLOCZ( NULL, sizeof(OBSCURANCE_VERTEX_BUFFER) * tData.dwNumVertexBuffers );
	ASSERT_RETVAL( tData.ptVertexBuffers, E_OUTOFMEMORY );


	// For each vertex buffer, finalize the average and commit it
	for ( int nVB = 0; nVB < tContext.pModelDef->nVertexBufferDefCount; nVB++ )
	{
		ASSERT_CONTINUE( tContext.ppVBResults[ nVB ] );
		D3D_VERTEX_BUFFER_DEFINITION * pVBDef = &tContext.pModelDef->ptVertexBufferDefs[ nVB ];
		OBSCURANCE_VERTEX_BUFFER * pOVB = &tData.ptVertexBuffers[ nVB ];

		if ( pVBDef->eVertexType == VERTEX_DECL_RIGID_16 )
		{
			// Currently, we're sticking the obscurance value in the unused high 8-bits of the DWORD normal in
			// rigid 32 and 64. Since rigid 16 doesn't have normals, we can forgo obscurance on the grounds that
			// there will be no meaningful lighting anyway.
			continue;
		}

		ASSERT_CONTINUE( sIsObscuranceVertFormat( pVBDef->eVertexType ) );

		int nPosStream = 0;
		int nNormalStream;
		switch ( pVBDef->eVertexType )
		{
		case VERTEX_DECL_RIGID_64:
			nNormalStream = 1;
			break;
		case VERTEX_DECL_RIGID_32:
			nNormalStream = 1;
			break;
		default:
			return E_FAIL;
		}
		ASSERT_CONTINUE( nNormalStream >= 0 )

		BYTE * pbPos = (BYTE*)pVBDef->pLocalData[ nPosStream ];
		UINT nPosStride = dxC_GetVertexSize( nPosStream, pVBDef );

		BYTE * pbNorms = (BYTE*)pVBDef->pLocalData[ nNormalStream ];
		UINT nNormStride = dxC_GetVertexSize( nNormalStream, pVBDef );


		pOVB->dwNumVertices = pVBDef->nVertexCount;
		pOVB->byVertexObscurance = (BYTE*)MALLOC( NULL, sizeof(BYTE) * pOVB->dwNumVertices );
		ASSERT_RETVAL( pOVB->byVertexObscurance, E_OUTOFMEMORY );


		// Now we can iterate per-vertex, finalize the average, and store off the accessibility value.
		for ( int nVert = 0; nVert < pVBDef->nVertexCount; nVert++ )
		{
			// Finalize average.
			float fAO = 0.f;
			if ( tContext.ppVBResults[ nVB ][ nVert ].fMax != 0.0f )
			{
				fAO = tContext.ppVBResults[ nVB ][ nVert ].fAct / tContext.ppVBResults[ nVB ][ nVert ].fMax;
				fAO = SATURATE( fAO );
			}

			// If the position is touching a clip plane, make it completely accessible.
			V( sGetBoundingPlaneEdgeOverride(
				fAO,
				pbPos,
				nPosStride,
				tPlanes,
				nPlanes,
				nVert ) );

			BYTE * pbyDest = &pOVB->byVertexObscurance[ nVert ];

			// Store the result
			sGetObscuranceAsUBYTE( pbyDest, fAO );
		}
	}

	if ( tPlanes )
		FREE( NULL, tPlanes );


	UINT64 nMS = _AO_TIMER_END();
	trace( "  Assemble vbuffers:  %1.2f seconds\n",
		nMS / 1000.f );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sComputeCRCs(
	OBSCURANCE_DATA & tData,
	OBSCURANCE_CONTEXT & tContext )
{
	_AO_TIMER_START( "Compute CRCs" );

	ASSERT_RETFAIL( tContext.pModelDef->ptVertexBufferDefs );

	for ( int nVB = 0; nVB < tContext.pModelDef->nVertexBufferDefCount; nVB++ )
	{
		if ( tData.ptVertexBuffers[ nVB ].dwNumVertices == 0 )
			continue;

		D3D_VERTEX_BUFFER_DEFINITION * pVBDef = &tContext.pModelDef->ptVertexBufferDefs[ nVB ];
		// compute per-VB CRCs
		tData.ptVertexBuffers[ nVB ].dwNumStreams = dxC_GetStreamCount( pVBDef );
		tData.ptVertexBuffers[ nVB ].pdwCRC = (DWORD*)MALLOC( NULL, sizeof(DWORD) * tData.ptVertexBuffers[ nVB ].dwNumStreams );
		ASSERT_RETVAL( tData.ptVertexBuffers[nVB].pdwCRC, E_OUTOFMEMORY );

		for ( DWORD s = 0; s < tData.ptVertexBuffers[nVB].dwNumStreams; s++ )
		{
			tData.ptVertexBuffers[nVB].pdwCRC[s] = CRC( 0, (const BYTE *)pVBDef->pLocalData[s], pVBDef->nBufferSize[s] );
		}
	}

	UINT64 nMS = _AO_TIMER_END();
	trace( "  Compute CRCs:  %1.2f seconds\n",
		nMS / 1000.f );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sSaveDataToFile(
	OBSCURANCE_DATA & tData,
	OBSCURANCE_CONTEXT & tContext )
{
	_AO_TIMER_START( "Save to file" );

	PRESULT pr = E_FAIL;

	ASSERT_RETFAIL( tContext.pModelDef->ptVertexBufferDefs );

	HANDLE hFile = INVALID_HANDLE_VALUE;


	// save file and add to pak

	// -- Get file name --

	char szAOFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension( szAOFilename, DEFAULT_FILE_WITH_PATH_SIZE, tContext.pModelDef->tHeader.pszName, OBSCURANCE_FILE_EXTENSION );
	ASSERT_GOTO( szAOFilename[0], exit );

	// -- Create file --

	hFile = CreateFile( szAOFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE ) 
	{
		ErrorDialog( "Couldn't open model ambient occlusion file for writing!  Check it out from source control and try again.\n\n%s", szAOFilename );
		goto exit;
	}


	// -- Header --
	OBSCURANCE_FILE_HEADER tHeader;
	tHeader.nVersion = OBSCURANCE_FILE_VERSION_CURRENT;
	// Like with the granny file, add the magic number at the end in case we stop while debugging
	// tHeader.dwMagicNumber = OBSCURANCE_FILE_MAGIC_NUMBER;
	tHeader.dwMagicNumber = 0;

	DWORD dwBytesWritten = 0;
	BOOL bWritten = WriteFile( hFile, &tHeader, sizeof( OBSCURANCE_FILE_HEADER ), &dwBytesWritten, NULL );
	ASSERT( dwBytesWritten == sizeof( OBSCURANCE_FILE_HEADER ) );
	int nFileOffset = dwBytesWritten;

	// -- Main data container --

	bWritten = WriteFile( hFile, &tData, sizeof( OBSCURANCE_DATA ), &dwBytesWritten, NULL );
	ASSERT( dwBytesWritten == sizeof( OBSCURANCE_DATA ) );
	nFileOffset += dwBytesWritten;




	// -- Obscurance data --

	int nVertexBufferArrayFileOffset = nFileOffset;
	OBSCURANCE_VERTEX_BUFFER * pVertexBuffers = tData.ptVertexBuffers;
	tData.ptVertexBuffers = (OBSCURANCE_VERTEX_BUFFER *) ((size_t)nFileOffset);
	bWritten = WriteFile( hFile, pVertexBuffers, sizeof( OBSCURANCE_VERTEX_BUFFER ) * tData.dwNumVertexBuffers, &dwBytesWritten, NULL );
	nFileOffset += dwBytesWritten;

	// mark this as the place in the file where we will start freeing data after it is loaded - through a realloc
	//tModelDefinition.nPostRestoreFileSize = nFileOffset;

	// save the OVBs themselves
	for ( DWORD i = 0; i < tData.dwNumVertexBuffers; i++ )
	{
		OBSCURANCE_VERTEX_BUFFER * pOVB = &pVertexBuffers[ i ];

		if ( ! pOVB->dwNumVertices )
			continue;

		// per-vertex obscurances
		bWritten = WriteFile( hFile, pOVB->byVertexObscurance, sizeof(BYTE) * pOVB->dwNumVertices, &dwBytesWritten, NULL );
		FREE ( NULL, pOVB->byVertexObscurance );
		pOVB->__p64_byVertexObscurance = (BYTE*)IntToPtr( nFileOffset );
		nFileOffset += dwBytesWritten;

		// per-stream CRCs
		bWritten = WriteFile( hFile, pOVB->pdwCRC, sizeof(DWORD) * pOVB->dwNumStreams, &dwBytesWritten, NULL );
		FREE ( NULL, pOVB->pdwCRC );
		pOVB->__p64_pdwCRC = (DWORD*)IntToPtr( nFileOffset );
		nFileOffset += dwBytesWritten;
	}

	// resave the array of vertex buffer structs to get the proper offsets written
	SetFilePointer( hFile, nVertexBufferArrayFileOffset, 0, FILE_BEGIN );
	bWritten = WriteFile( hFile, pVertexBuffers, sizeof( OBSCURANCE_VERTEX_BUFFER ) * tData.dwNumVertexBuffers, &dwBytesWritten, NULL );
	FREE( NULL, pVertexBuffers );





	// Save the header again - with the magic number so that we can mark that we are finished
	SetFilePointer( hFile, 0, 0, FILE_BEGIN );
	tHeader.dwMagicNumber = OBSCURANCE_FILE_MAGIC_NUMBER;
	bWritten = WriteFile( hFile, &tHeader, sizeof( OBSCURANCE_FILE_HEADER ), &dwBytesWritten, NULL );
	ASSERT( dwBytesWritten == sizeof( OBSCURANCE_FILE_HEADER ) );
	nFileOffset = dwBytesWritten;

	// Save the data struct again - we have updated some OFFSET values
	bWritten = WriteFile( hFile, &tData, sizeof( OBSCURANCE_DATA ), &dwBytesWritten, NULL );
	ASSERT( dwBytesWritten == sizeof( OBSCURANCE_DATA ) );
	nFileOffset += dwBytesWritten;

	pr = S_OK;

	//// Mark the model definition as having an ambient occlusion file.
	//tContext.pModelDef->dwFlags |= MODELDEF_FLAG_HAS_OBSCURANCE_FILE;
exit:

	if( hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hFile );
	}

	UINT64 nMS = _AO_TIMER_END();
	trace( "  Save to file:  %1.2f seconds\n",
		nMS / 1000.f );

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceLoadFromData(
	const OBSCURANCE_DATA * pData,
	OBSCURANCE_CONTEXT & tContext )
{
	//OBSCURANCE_CONTEXT tContext;
	//tContext.ppMeshes = ppMeshes;
	//tContext.pModelDef = pModelDef;
	V_RETURN( sFinalizeVertexBuffers( pData, tContext ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sObscuranceComputeForModelDefinition(
	D3D_MESH_DEFINITION ** ppMeshes,
	D3D_MODEL_DEFINITION * pModelDef,
	BOOL bCleanup )
{
	_AO_TIMER_START( "Obscurance render" );

	ASSERT_RETINVALIDARG( ppMeshes );
	ASSERT_RETINVALIDARG( pModelDef );

	trace( "NOTE: Computing ambient occlusion for %s\n", pModelDef->tHeader.pszName );

	OBSCURANCE_CONTEXT tContext;
	tContext.pModelDef = pModelDef;
	tContext.ppMeshes = ppMeshes;
	tContext.fNear = 0.01f;
	tContext.fFar = VectorDistanceSquared( pModelDef->tRenderBoundingBox.vMin, pModelDef->tRenderBoundingBox.vMax );
	ASSERT( tContext.fFar > 0.f );
	tContext.fFar = sqrtf( tContext.fFar );

	V( sObscuranceRenderInit( tContext ) );

#ifndef DEBUG_OBSCURANCE_DISPLAY

	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
	{
		char szName[32];
		PStrPrintf( szName, 32, "Obscurance mesh %3d/%d", i, pModelDef->nMeshCount );
		_AO_TIMER_START( szName );

		V( sComputeMeshObscurance( i, tContext ) );

		UINT64 nMS = _AO_TIMER_END();
		trace( "  Mesh %3d/%d:  %1.2f seconds\n",
			i,
			pModelDef->nMeshCount,
			nMS / 1000.f );

		AppCommonRunMessageLoop();
	}

	OBSCURANCE_DATA tData;
	V( sAssembleVertexBuffers( tContext, tData ) );

	AppCommonRunMessageLoop();

	V( sComputeCRCs( tData, tContext ) );

	AppCommonRunMessageLoop();

	// set the obscurance data on the model
	V( sObscuranceLoadFromData( &tData, tContext ) );

	AppCommonRunMessageLoop();

	// save obscurance data to a file for later use if model file changes
	V( sSaveDataToFile( tData, tContext ) );

	AppCommonRunMessageLoop();

#endif  // DEBUG_OBSCURANCE_DISPLAY

	V( sObscuranceRenderDone( tContext, bCleanup ) );

	UINT64 nMS = _AO_TIMER_END();
	trace( "Obscurance render time (quality %d, res %d):  %1.2f seconds     [%s]\n",
		OBSCURANCE_QUALITY,
		OBSCURANCE_RENDER_RESOLUTION,
		nMS / 1000.f,
		pModelDef->tHeader.pszName );

	AppCommonRunMessageLoop();

	return S_OK;
}

#endif  // DEVELOPMENT







//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ObscuranceDeviceLost()
{
#if ISVERSION(DEVELOPMENT)
	V_RETURN( sObscuranceQueriesFree() );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ObscuranceDeviceReset()
{
#if ISVERSION(DEVELOPMENT)
	V_RETURN( sObscuranceQueriesInit() );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ObscuranceComputeForModelDefinition(
	D3D_MESH_DEFINITION ** ppMeshes,
	D3D_MODEL_DEFINITION * pModelDef,
	BOOL bGenerate /*= FALSE*/,
	BOOL bCleanup /*= TRUE*/ )
{
#if ISVERSION(DEVELOPMENT)	

#ifdef ENGINE_TARGET_DX10
	// CML 2007.06.12 - Allow integration of already-generated obscurance data in DX10.
	if ( bGenerate )
		return S_FALSE;
#endif // DX10

	ASSERT_RETINVALIDARG( pModelDef );

	// Callback into game code to find out if we should compute obscurance for this file
	if ( ! e_ObscuranceShouldComputeForFile( pModelDef->tHeader.pszName ) )
		return S_FALSE;
	if ( ! pModelDef->ptVertexBufferDefs )
		return S_FALSE;


	PRESULT pr = E_FAIL;

	ASSERT_RETINVALIDARG( ppMeshes );

	BOOL bNeedGenerate = FALSE;

	// derive obscurance file name

	char szAOFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension( szAOFilename, DEFAULT_FILE_WITH_PATH_SIZE, pModelDef->tHeader.pszName, OBSCURANCE_FILE_EXTENSION );
	ASSERT_RETFAIL( szAOFilename[0] );

	// load obscurance file

	DWORD dwBufferSize;
	BYTE * pFile = NULL;
	if ( ! bGenerate )
	{
		pFile = (BYTE*)FileLoad(MEMORY_FUNC_FILELINE( NULL, szAOFilename, &dwBufferSize ));
		if ( ! pFile )
			bNeedGenerate = TRUE;
	}

	int nFileOffset = 0;
	if ( ! bGenerate && ! bNeedGenerate )
	{
		ASSERT_GOTO( pFile, exit );
		// validate file
		OBSCURANCE_FILE_HEADER * pHeader = (OBSCURANCE_FILE_HEADER*) ( &pFile[ nFileOffset ] );
		nFileOffset += sizeof(OBSCURANCE_FILE_HEADER);
		if (    pHeader->dwMagicNumber != OBSCURANCE_FILE_MAGIC_NUMBER
			 || pHeader->nVersion      != OBSCURANCE_FILE_VERSION_CURRENT )
		{
			bNeedGenerate = TRUE;
		}
	}

	OBSCURANCE_DATA * pData = NULL;
	if ( ! bGenerate && ! bNeedGenerate )
	{
		ASSERT_GOTO( pFile, exit );
		pData = (OBSCURANCE_DATA*) ( &pFile[ nFileOffset ] );
		nFileOffset += sizeof(OBSCURANCE_DATA);

		// compare stats
		if ( pData->dwNumVertexBuffers != pModelDef->nVertexBufferDefCount )
		{
			bNeedGenerate = TRUE;
		}
		else
		{
			pData->__p64_ptVertexBuffers = (OBSCURANCE_VERTEX_BUFFER*) ( pFile + PtrToInt( pData->__p64_ptVertexBuffers ) );
			for ( DWORD i = 0; i < pData->dwNumVertexBuffers; i++ )
			{
				if ( ! sIsObscuranceVertFormat( pModelDef->ptVertexBufferDefs[i].eVertexType ) 
					 && pData->ptVertexBuffers[i].dwNumVertices == 0 )
					continue;

				if ( pData->ptVertexBuffers[i].dwNumVertices != (DWORD)pModelDef->ptVertexBufferDefs[i].nVertexCount )
				{
					bNeedGenerate = TRUE;
					break;
				}
				else if ( pData->ptVertexBuffers[i].dwNumStreams != (DWORD) dxC_GetStreamCount( &pModelDef->ptVertexBufferDefs[i] ) )
				{
					bNeedGenerate = TRUE;
					break;
				}
				else
				{
					// compare CRCs
					//pData->ptVertexBuffers[i].__p64_pdwCRC = (DWORD*) ( pFile + PtrToInt( pData->ptVertexBuffers[i].__p64_pdwCRC ) );
					//for ( DWORD s = 0; s < pData->ptVertexBuffers[i].dwNumStreams; s++ )
					//{
					//	DWORD dwModelDefCRC = CRC( 0, (BYTE*)pModelDef->ptVertexBufferDefs[i].pLocalData[s], pModelDef->ptVertexBufferDefs[i].nBufferSize[s] );
					//	if ( pData->ptVertexBuffers[i].pdwCRC[s] != dwModelDefCRC )
					//	{
					//		bGenerate = TRUE;
					//		break;
					//	}
					//}
				}
			}
		}
	}

	if ( bGenerate )
	{
		// generate and save
		V_SAVE_GOTO( pr, exit, sObscuranceComputeForModelDefinition( ppMeshes, pModelDef, bCleanup ) );
	}
	else
	{
		if ( bNeedGenerate )
		{
			// Data is out of date!  Post a data warning (or assert, depending on fillpak status?)
			ShowDataWarning( WARNING_TYPE_BACKGROUND, "Background file MAO out of date, needs ambient occlusion generation!\n\n%s", pModelDef->tHeader.pszName );
			pr = S_FALSE;
			goto exit;
		}

		// finish restoring pointers
		for ( DWORD i = 0; i < pData->dwNumVertexBuffers; i++ )
		{
			if ( ! pData->ptVertexBuffers[i].dwNumVertices )
				continue;
			pData->ptVertexBuffers[i].__p64_byVertexObscurance = (BYTE*) ( pFile + PtrToInt( pData->ptVertexBuffers[i].__p64_byVertexObscurance ) );
		}

		// load and integrate data 
		OBSCURANCE_CONTEXT tContext;
		structclear( tContext );
		tContext.ppMeshes = ppMeshes;
		tContext.pModelDef = pModelDef;
		tContext.ppBoundingEdgeMeshes = ppMeshes;
		tContext.pBoundingEdgeModelDef = pModelDef;
		V_SAVE_GOTO( pr, exit, sObscuranceLoadFromData( pData, tContext ) );
	}

	pr = S_OK;
exit:
	if ( pFile )
		FREE( NULL, pFile );
	return pr;

#endif  // DEVELOPMENT

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_DebugRenderNormals( int nModelID )
{
#if ISVERSION(DEVELOPMENT)


#ifdef DEBUG_OBSCURANCE_DISPLAY

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	int nMeshes = e_ModelDefinitionGetMeshCount( pModel->nModelDefinitionId, LOD_HIGH );

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, LOD_HIGH );
	ASSERT_RETFAIL( pModelDef );

	D3D_MESH_DEFINITION * ppMeshes[ MAX_MESHES_PER_MODEL ] = {NULL};
	for ( int i = 0; i < nMeshes; i++ )
	{
		ppMeshes[i] = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
		ASSERT_CONTINUE( ppMeshes[i] );
	}

	OBSCURANCE_CONTEXT tContext;
	tContext.pModelDef = pModelDef;
	tContext.ppMeshes = ppMeshes;
	tContext.fNear = 0.01f;
	tContext.fFar = VectorDistanceSquared( pModelDef->tRenderBoundingBox.vMin, pModelDef->tRenderBoundingBox.vMax );
	ASSERT( tContext.fFar > 0.f );
	tContext.fFar = sqrtf( tContext.fFar );

	for ( int i = 0; i < nMeshes; i++ )
	{
		V( sComputeMeshObscurance( i, tContext ) );
	}

#endif


#if 0
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	//V( sObscuranceQueriesInit() );
	V( dxC_BeginScene() );

	int nMeshes = e_ModelDefinitionGetMeshCount( pModel->nModelDefinitionId, LOD_HIGH );

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, LOD_HIGH );
	ASSERT_RETFAIL( pModelDef );

	D3D_MESH_DEFINITION * ppMeshes[ MAX_MESHES_PER_MODEL ] = {NULL};
	for ( int i = 0; i < nMeshes; i++ )
	{
		ppMeshes[i] = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
		ASSERT_CONTINUE( ppMeshes[i] );
	}

	D3D_EFFECT * pEffect;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( sObscuranceGetEffect( &pEffect, &ptTechnique ) );

	OBSCURANCE_CONTEXT tContext;
	tContext.pModelDef = pModelDef;
	tContext.ppMeshes = ppMeshes;
	tContext.fNear = 0.01f;
	tContext.fFar = VectorDistanceSquared( pModelDef->tRenderBoundingBox.vMin, pModelDef->tRenderBoundingBox.vMax );
	ASSERT( tContext.fFar > 0.f );
	tContext.fFar = sqrtf( tContext.fFar );

	VECTOR vViewPos, vViewDir;
	V( e_GetViewPosition( &vViewPos ) );
	V( e_GetViewDirection( &vViewDir ) );

	V( sRenderAlongRay( pEffect, *ptTechnique, vViewDir, vViewPos, 0, tContext ) );

	V( dxC_ResetViewport() );

	V( dxC_EndScene() );
	//V( sObscuranceQueriesFree() );
#endif  // 0

#endif  // DEVELOPMENT

	return S_OK;
}

