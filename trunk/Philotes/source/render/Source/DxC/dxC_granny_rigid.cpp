//----------------------------------------------------------------------------
// dx9_granny_rigid.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "config.h"
#include "filepaths.h"

#include "dxC_model.h"
#include "dxC_texture.h"
#include "dxC_state.h"
#include "dxC_VertexDeclarations.h"
#include "e_adclient.h"
#include "dxC_obscurance.h"

#include "pakfile.h"
#include "performance.h"
#include "appcommon.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// find out what kinds of vertex buffers we want for each mesh
struct MESH_SAVER
{
	D3D_MESH_DEFINITION tMeshDef;
	int nGrannyMeshIndex;
	int nGrannyTriangleGroup;
	VERTEX_DECL_TYPE eDesiredVertexType;
	int pnTexChannels[ TEXCOORDS_MAX_CHANNELS ];
	char pszLightmapNameWithPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	int nTexChannelCount;
	BOOL bAdjustVerticesForLightmap;
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

static granny_data_type_definition g_ptSimpleVertexDataType[] =
{
	{GrannyReal32Member, GrannyVertexPositionName, 0, 3},
	{GrannyReal32Member, GrannyVertexNormalName, 0, 3},
	{GrannyEndMember},
};

//-------------------------------------------------------------------------------------------------
// Rigid vertex definitions
//-------------------------------------------------------------------------------------------------

// this is what is an intermediate format for grabbing values from granny
struct GRANNY_RIGID_VERTEX
{
	D3DXVECTOR3 vPosition;									// Vertex Position
	D3DXVECTOR3 vNormal;									// Vertex Normal
	FLOAT		fTextureCoords[TEXCOORDS_MAX_CHANNELS][2];	// The texture coordinates
	D3DXVECTOR3 vBinormal;									// Used for Pixel Lighting
	D3DXVECTOR3 vTangent;									// Used for Pixel Lighting
};

static granny_data_type_definition g_ptGrannyRigidVertexDataType[] =
{
	{GrannyReal32Member, GrannyVertexPositionName,				 0, 3},
	{GrannyReal32Member, GrannyVertexNormalName,				 0, 3},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName "0", 0, 2},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName "1", 0, 2},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName "2", 0, 2},
	{GrannyReal32Member, GrannyVertexBinormalName,				 0, 3},
	{GrannyReal32Member, GrannyVertexTangentName,				 0, 3},
	// gap
	{GrannyEndMember},
};
 
//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDecompressColorToVector ( D3DXVECTOR3 & vOutVector, const D3DCOLOR ColorNormal )
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

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define RIGID_VERTEX_32_TEX_COORDS 2

static float* sGetRigidVertexTextureCoords ( 
	D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef, 
	int nVertex,
	TEXCOORDS_CHANNEL eChannel,
	BYTE* pbVertices = NULL )
{
	ASSERT( nVertex < pVertexBufferDef->nVertexCount );
	ASSERT( nVertex >= 0 );

	switch ( pVertexBufferDef->eVertexType )
	{
	case VERTEX_DECL_RIGID_64:
		{
			ASSERT( dxC_GetStreamCount( pVertexBufferDef ) == VS_BACKGROUND_INPUT_64_STREAM_COUNT );

			if ( !pbVertices )
				pbVertices = (BYTE*)pVertexBufferDef->pLocalData[ 1 ];

			int nStreamOneVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );
			BYTE* pbStreamOneVertex =  pbVertices + nStreamOneVertexSize * nVertex;
			ASSERT( pbStreamOneVertex < pbVertices + pVertexBufferDef->nBufferSize[ 1 ] );

			VS_BACKGROUND_INPUT_64_STREAM_1* pVertex = (VS_BACKGROUND_INPUT_64_STREAM_1*)pbStreamOneVertex;
			return &pVertex->LightMapDiffuse[ 0 ];
		}
	case VERTEX_DECL_RIGID_32:
		{
			if ( eChannel >= RIGID_VERTEX_32_TEX_COORDS )
				return NULL;
			
			ASSERT( dxC_GetStreamCount( pVertexBufferDef ) == VS_BACKGROUND_INPUT_32_STREAM_COUNT );

			int nStreamOneVertexSize = dxC_GetVertexSize( 1, pVertexBufferDef );
			BYTE* pbStreamOneVertex = (BYTE*)pVertexBufferDef->pLocalData[ 1 ] + nStreamOneVertexSize * nVertex;
			ASSERT( pbStreamOneVertex < (BYTE*)pVertexBufferDef->pLocalData[ 1 ] + pVertexBufferDef->nBufferSize[ 1 ] );

			VS_BACKGROUND_INPUT_32_STREAM_1* pStreamOneVertex = (VS_BACKGROUND_INPUT_32_STREAM_1*)pbStreamOneVertex;
			return &pStreamOneVertex->LightMapDiffuse[ 0 ];
		}
	case VERTEX_DECL_RIGID_16: // the 16 byte version only support short texture coords... no floats
	default:
		return NULL; 
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static VECTOR * sGetRigidVertexPosition ( 
	D3D_VERTEX_BUFFER_DEFINITION* pBufferDef, 
	void * pbVertices,
	int nIndex )
{
	ASSERT_RETNULL( nIndex < pBufferDef->nVertexCount );
	ASSERT_RETNULL( nIndex >= 0 );
	ASSERT_RETNULL( pbVertices );
	ASSERT_RETNULL( OFFSET(VS_BACKGROUND_INPUT_64_STREAM_0, Position) == 0 );
	ASSERT_RETNULL( OFFSET(VS_BACKGROUND_INPUT_32_STREAM_0, Position) == 0 );
	ASSERT_RETNULL( OFFSET(VS_BACKGROUND_INPUT_16_STREAM_0, Position) == 0 );
	return (VECTOR*)( (BYTE*)pbVertices + nIndex * dxC_GetVertexSize( 0, pBufferDef ) );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sRearrangeTextureCoords ( 
	GRANNY_RIGID_VERTEX * pVertex, 
	int * nTexChannel )
{
	int nCoordPairSize = sizeof(float) * 2;
	float fTexCoords[ TEXCOORDS_MAX_CHANNELS ][ 2 ];

	if ( nTexChannel[ TEXCOORDS_CHANNEL_DIFFUSE ] < 0 )
		memset( &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE ], 0, nCoordPairSize	);
	else
		memcpy( &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE  ], pVertex->fTextureCoords[ nTexChannel[ TEXCOORDS_CHANNEL_DIFFUSE  ] ], nCoordPairSize );

	if ( nTexChannel[ TEXCOORDS_CHANNEL_DIFFUSE2 ] < 0 )
		memset( &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE2 ], 0, nCoordPairSize	);
	else
		memcpy( &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE2 ], pVertex->fTextureCoords[ nTexChannel[ TEXCOORDS_CHANNEL_DIFFUSE2 ] ], nCoordPairSize );

	if ( nTexChannel[ TEXCOORDS_CHANNEL_LIGHTMAP ] < 0 )
		memset( &fTexCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], 0, nCoordPairSize	);
	else
		memcpy( &fTexCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], pVertex->fTextureCoords[ nTexChannel[ TEXCOORDS_CHANNEL_LIGHTMAP ] ], nCoordPairSize );

	memcpy( pVertex->fTextureCoords[ TEXCOORDS_CHANNEL_DIFFUSE  ], &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE  ], nCoordPairSize );
	memcpy( pVertex->fTextureCoords[ TEXCOORDS_CHANNEL_DIFFUSE2 ], &fTexCoords[ TEXCOORDS_CHANNEL_DIFFUSE2 ], nCoordPairSize );
	memcpy( pVertex->fTextureCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], &fTexCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], nCoordPairSize );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3DCOLOR dx9_CompressVectorToColor ( const D3DXVECTOR3 & vInVector )
{
	D3DXVECTOR3 vVector = vInVector;
	// transform to [0,1]
	vVector.x = (vVector.x + 1.0f) / 2;
	vVector.y = (vVector.y + 1.0f) / 2;
	vVector.z = (vVector.z + 1.0f) / 2;

	// convert into a color
	D3DCOLOR ColorNormal = 0;
	ColorNormal |= (BYTE)(vVector.x * 255) << 16;
	ColorNormal |= (BYTE)(vVector.y * 255) <<  8;
	ColorNormal |= (BYTE)(vVector.z * 255) <<  0;

	return ColorNormal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3DCOLOR dx9_CompressVectorToColor ( const D3DXVECTOR4 & vInVector )
{
	D3DXVECTOR4 vVector = vInVector;
	// transform to [0,1]
	vVector.x = (vVector.x + 1.0f) / 2;
	vVector.y = (vVector.y + 1.0f) / 2;
	vVector.z = (vVector.z + 1.0f) / 2;

	// convert into a color
	D3DCOLOR ColorNormal = 0;
	ColorNormal |= (BYTE)(vVector.x * 255) << 16;
	ColorNormal |= (BYTE)(vVector.y * 255) <<  8;
	ColorNormal |= (BYTE)(vVector.z * 255) <<  0;
	ColorNormal |= (BYTE)(vVector.w * 255) << 24;

	return ColorNormal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_GrannyTransformAndCompressNormal ( VECTOR * pvGrannyNormal, MATRIX * pmMatrix )
{
	ASSERT_RETZERO( pvGrannyNormal );
	VECTOR vTranformedNormal;
	VectorNormalize( *pvGrannyNormal );
	MatrixMultiplyNormal( &vTranformedNormal, pvGrannyNormal, pmMatrix );
	VectorNormalize( vTranformedNormal );

	*pvGrannyNormal = vTranformedNormal; // the caller needs it tranformed too

	DWORD dwNormal = dx9_CompressVectorToColor ( *(D3DXVECTOR3*)&vTranformedNormal );

	// set alpha to 1.0 for obscurance default
	dwNormal |= 0xff000000;

	return dwNormal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR e_GrannyTransformNormal ( VECTOR * pvGrannyNormal, MATRIX * pmMatrix )
{
	VECTOR vTranformedNormal;
	VectorNormalize( *pvGrannyNormal );
	MatrixMultiplyNormal( &vTranformedNormal, pvGrannyNormal, pmMatrix );
	VectorNormalize( vTranformedNormal );

	*pvGrannyNormal = vTranformedNormal; // the caller needs it tranformed too

	return vTranformedNormal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGrannyGetAdObjectFromMesh ( AD_OBJECT_DEFINITION * pAdObjDef, granny_mesh * pGrannyMesh, granny_tri_material_group * pTriMaterialGroup, MATRIX & mOriginTransformMatrix )
{
	PRESULT pr = E_FAIL;

	pAdObjDef->eResType = AORTYPE_INVALID;

	// get the vertices
	SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOCZ( NULL, GrannyGetMeshVertexCount( pGrannyMesh ) * sizeof( SIMPLE_VERTEX ) );
	GrannyCopyMeshVertices( pGrannyMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );

	int nTriCount = pTriMaterialGroup->TriCount;

	// get the indices
	INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOCZ( NULL, 3 * GrannyGetMeshTriangleCount( pGrannyMesh ) * sizeof( INDEX_BUFFER_TYPE ) );
	GrannyCopyMeshIndices( pGrannyMesh, sizeof ( INDEX_BUFFER_TYPE ), pwIndexBuffer );

	// find the middle of the mesh by average
	// find the overall mesh normal by area-weighted average
	VECTOR vCenter(0, 0, 0);
	VECTOR vNormal(0, 0, 0);
	VECTOR vDiff(0, 0, 0);
	BOOL bBoundingBoxInitialized = FALSE;
	for (int nTriangle = 0; nTriangle < nTriCount; nTriangle ++ )
	{
		SIMPLE_VERTEX * pVerts[] =
		{
			&pVertices[ pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + 0 ] ],
			&pVertices[ pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + 1 ] ],
			&pVertices[ pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + 2 ] ]
		};

		if ( ! bBoundingBoxInitialized )
		{
			pAdObjDef->tAABB_Obj.vMin = pVerts[0]->vPosition;
			pAdObjDef->tAABB_Obj.vMax = pVerts[0]->vPosition;
			bBoundingBoxInitialized = TRUE;
		}

		VECTOR vTriNormal(0,0,0);
		for (int i = 0; i < 3; i++)
		{
			vCenter += pVerts[i]->vPosition;
			vTriNormal += pVerts[i]->vNormal;
			BoundingBoxExpandByPoint( &pAdObjDef->tAABB_Obj, &pVerts[i]->vPosition );
		}

		float fLen = VectorNormalize( vTriNormal );
		ASSERT( fLen > 0.f );

		// get triangle area
		VECTOR vEdge1 = pVerts[ 0 ]->vPosition - pVerts[ 1 ]->vPosition;
		VECTOR vEdge2 = pVerts[ 1 ]->vPosition - pVerts[ 2 ]->vPosition;
		VECTOR vCross;
		VectorCross( vCross, vEdge1, vEdge2 );
		float fTriArea = VectorLength( vCross );

		vNormal += vTriNormal * fTriArea;
	}

	vCenter /= 3.0f * nTriCount;
	float fLen = VectorNormalize( vNormal );
	ASSERTV_GOTO( fLen > 0.f, cleanup, "Failed to calculate overall ad object mesh normal -- the area-weighted average normal length was 0!\n\nThis needs to be fixed in data or this ad object won't work.\n\n%s", pGrannyMesh->Name );

	pAdObjDef->eResType = AORTYPE_2D_TEXTURE;
	MatrixMultiplyNormal( &pAdObjDef->vNormal_Obj, &vNormal, &mOriginTransformMatrix );
	MatrixMultiply(		  &pAdObjDef->vCenter_Obj, &vCenter, &mOriginTransformMatrix );
	BoundingBoxTranslate( &mOriginTransformMatrix, &pAdObjDef->tAABB_Obj, &pAdObjDef->tAABB_Obj );

	vDiff = pAdObjDef->tAABB_Obj.vMax - pAdObjDef->tAABB_Obj.vMin;
	float fVolume = vDiff.x * vDiff.y * vDiff.z;
	ASSERTV( fVolume < 1000.f, "Ad object volume is very large -- erroneously joined ad meshes?\n\nAd def: %s", pAdObjDef->szName );

	pr = S_OK;
cleanup:
	FREE ( NULL, pVertices );
	FREE ( NULL, pwIndexBuffer );

	return pr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int sAdObjectDefinitionsCompare( const void * p1, const void * p2 )
{
	const AD_OBJECT_DEFINITION * pE1 = static_cast<const AD_OBJECT_DEFINITION*>(p1);
	const AD_OBJECT_DEFINITION * pE2 = static_cast<const AD_OBJECT_DEFINITION*>(p2);

	return PStrCmp( pE1->szName, pE2->szName, MAX_ADOBJECT_NAME_LEN );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyGetTransformationFromOriginMesh ( MATRIX * pOutMatrix, granny_mesh * pGrannyMesh, granny_tri_material_group * pTriMaterialGroup, const char * pszFileName )
{
	// get the vertices
	SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOCZ( NULL, GrannyGetMeshVertexCount( pGrannyMesh ) * sizeof( SIMPLE_VERTEX ) );
	GrannyCopyMeshVertices( pGrannyMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );
	REF( pszFileName );
	ASSERTV( pTriMaterialGroup->TriCount == TRIANGLES_PER_ORIGIN, "%s", pszFileName );// they should have made a rectangle for this

	// get the indices
	INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOCZ( NULL, 3 * GrannyGetMeshTriangleCount( pGrannyMesh ) * sizeof( INDEX_BUFFER_TYPE ) );
	GrannyCopyMeshIndices( pGrannyMesh, sizeof ( INDEX_BUFFER_TYPE ), pwIndexBuffer );

	// find the middle of the mesh
	VECTOR vCenter(0, 0, 0);
	for (int nTriangle = 0; nTriangle < TRIANGLES_PER_ORIGIN; nTriangle ++ )
	{
		for (int i = 0; i < 3; i++)
			vCenter += pVertices[ pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + i ] ].vPosition;
	}
	vCenter /= 3.0f * TRIANGLES_PER_ORIGIN;

	int nNormalVertexIndex = pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst ) ];
	VECTOR vNormal = pVertices[ nNormalVertexIndex ].vNormal;
	ASSERT( fabsf( vNormal.fZ ) < 0.001f );
	VECTOR vForward( 0, 1, 0 );
	float fRotation = acos ( VectorDot( vNormal, vForward ) );
	if ( vNormal.fX > 0 )
		fRotation = - fRotation;
	MatrixRotationZ( *pOutMatrix, fRotation );
	MATRIX tTranslationMatrix;
	MatrixTranslation( & tTranslationMatrix, & vCenter );
	MatrixMultiply( pOutMatrix, pOutMatrix, & tTranslationMatrix );

	MatrixInverse( pOutMatrix, pOutMatrix );

	FREE ( NULL, pVertices );
	FREE ( NULL, pwIndexBuffer );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyFindLowestPointInVertexList ( void * pVertices, int nVertexCount, int nVertexStructSize, VECTOR * pvPosition, int * pnDirection )
{
	ASSERT ( pVertices );
	ASSERT ( pvPosition );
	ASSERT ( pnDirection );
	ASSERT ( nVertexStructSize );
	// since every vertex format starts with a position vector,
	// we will grab it and then jump ahead the size of the vertex format
	// this way, we don't have to know the format for the vertices
	D3DXVECTOR3 * pPositionVector = (D3DXVECTOR3 *)pVertices;
	D3DXVECTOR3 * pvLowestPosition  = pPositionVector;
	D3DXVECTOR3 * pvHighestPosition = pPositionVector;
	for ( int nVertex = 0; nVertex < nVertexCount; nVertex ++ )
	{
		if ( pPositionVector->z < pvLowestPosition->z )
			pvLowestPosition  = pPositionVector;
		if ( pPositionVector->z > pvHighestPosition->z )
			pvHighestPosition = pPositionVector;
		pPositionVector = (D3DXVECTOR3 *)( (BYTE *)pPositionVector + nVertexStructSize );
	}
	*pvPosition = *pvLowestPosition;
	float fRadians = VectorToAngle( (VECTOR &) *pvLowestPosition, (VECTOR &) *pvHighestPosition );
	*pnDirection = NORMALIZE( (int)(fRadians * 360.0f / (2.0f * PI)), 360 ); 
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sFindLowestPointInVertexBuffer ( D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, VECTOR * pvPosition, int * pnDirection )
{
	ASSERT ( pVertexBufferDef );
	ASSERT ( pVertexBufferDef->pLocalData[ 0 ] );
	ASSERT ( pvPosition );
	ASSERT ( pnDirection );
	int nVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );
	ASSERT ( nVertexSize );

	e_GrannyFindLowestPointInVertexList(
		pVertexBufferDef->pLocalData[ 0 ],
		pVertexBufferDef->nVertexCount,
		nVertexSize,
		pvPosition,
		pnDirection );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static float sFloatIsEqualWithError( float fFirst, float fSecond, float fError )
{
	return fabsf( fFirst - fSecond ) < fError;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGetPositionAndNormalFromMesh ( granny_mesh * pGrannyMesh, VECTOR * pvPosition, VECTOR * pvNormal, MATRIX * pmOriginTransformMatrix )
{
	int nVertexCount = GrannyGetMeshVertexCount( pGrannyMesh );
	ASSERT( nVertexCount );
	SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOC( NULL, nVertexCount * sizeof( SIMPLE_VERTEX ) );
	GrannyCopyMeshVertices( pGrannyMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );
	ASSERT ( pvPosition );
	// since every vertex format starts with a position vector,
	// we will grab it and then jump ahead the size of the vertex format
	// this way, we don't have to know the format for the vertices
	SIMPLE_VERTEX * pSimpleVertex = &pVertices[ 0 ];
	*pvNormal = pSimpleVertex->vNormal;
	D3DXVec3TransformNormal ( (D3DXVECTOR3*)pvNormal, (D3DXVECTOR3*)pvNormal, (D3DXMATRIXA16*)pmOriginTransformMatrix );

	VECTOR vTotalPosition(0.0f, 0.0f, 0.0f);
	for ( int i = 0; i < nVertexCount; i ++ )
	{
		vTotalPosition += pSimpleVertex->vPosition;
		pSimpleVertex++;
	}
	vTotalPosition /= (float) nVertexCount;

	VECTOR4 vVector4;
	D3DXVec3Transform ( (D3DXVECTOR4*)& vVector4, (D3DXVECTOR3*)& vTotalPosition, (D3DXMATRIXA16*)pmOriginTransformMatrix );

	pvPosition->fX = vVector4.fX;
	pvPosition->fY = vVector4.fY;
	pvPosition->fZ = vVector4.fZ;

	FREE( NULL, pVertices );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sApplyBoundingBoxMinScale( BOUNDING_BOX &tBBox, float fMinScale )
{
	float * pfMin = (float *)&tBBox.vMin;
	float * pfMax = (float *)&tBBox.vMax;
	const int nMaxDims = 3;
	float fHalfMinScale = fMinScale * 0.5f;

	for ( int nDim = 0; nDim < nMaxDims; nDim++ )
	{
		if ( fabs( pfMax[ nDim ] - pfMin[ nDim ] ) < fMinScale )
		{
			float fMid = ( pfMax[ nDim ] + pfMin[ nDim ] ) * 0.5f;
			pfMin[ nDim ] = fMid - fHalfMinScale;
			pfMax[ nDim ] = fMid + fHalfMinScale;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sVerifyBoundingBoxIsValid( BOUNDING_BOX &tBBox )
{
	BOUNDING_BOX tValidBox;

	tValidBox.vMin.fX = min( tBBox.vMin.fX, tBBox.vMax.fX );
	tValidBox.vMin.fY = min( tBBox.vMin.fY, tBBox.vMax.fY );
	tValidBox.vMin.fZ = min( tBBox.vMin.fZ, tBBox.vMax.fZ );
	tValidBox.vMax.fX = max( tBBox.vMin.fX, tBBox.vMax.fX );
	tValidBox.vMax.fY = max( tBBox.vMin.fY, tBBox.vMax.fY );
	tValidBox.vMax.fZ = max( tBBox.vMin.fZ, tBBox.vMax.fZ );

	sApplyBoundingBoxMinScale( tValidBox, MIN_BOUNDING_BOX_SCALE );

	tBBox = tValidBox;

	ASSERT( tBBox.vMin.fX < tBBox.vMax.fX );
	ASSERT( tBBox.vMin.fY < tBBox.vMax.fY );
	ASSERT( tBBox.vMin.fZ < tBBox.vMax.fZ );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float dx9_GetMeshScale(
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef,
	INDEX_BUFFER_TYPE * pIndices,
	int nTriCount )
{
	// this function needs a rewrite/bugfix if it's ever to be used again
	UNDEFINED_CODE();

	ASSERT_RETZERO( pVertexBufferDef );
	ASSERT_RETZERO( dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) );
	ASSERT_RETZERO( pIndices );
	int nIndexCount = nTriCount * 3;

	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( pVertexBufferDef->nD3DBufferID[ 0 ] );
	ASSERT_RETZERO( pVB && pVB->pD3DVertices );

	BYTE * pVerts = NULL;
#if defined(ENGINE_TARGET_DX9)
	V( pVB->pD3DVertices->Lock( 0, 0, (void **) &pVerts, D3DLOCK_READONLY ) );
#elif defined(ENGINE_TARGET_DX10)
	V( pVB->pD3DVertices->Map( D3D10_MAP_READ, 0, (void **) &pVerts));
#endif
		VECTOR * pvPosition = sGetRigidVertexPosition( pVertexBufferDef, pVerts, pIndices[ 0 ] );
		BOUNDING_BOX tBBox;
		if ( pvPosition )
		{
			tBBox.vMin = *pvPosition;
			tBBox.vMax = *pvPosition;
		} else {
			tBBox.vMin = VECTOR( 0 );
			tBBox.vMax = VECTOR( 0 );
		}
		for ( int i = 0; i < nIndexCount; i++ )
		{
			pvPosition = sGetRigidVertexPosition( pVertexBufferDef, pVerts, pIndices[ i ] );
			if ( pvPosition )
				BoundingBoxExpandByPoint( &tBBox, pvPosition );
		}
#if defined(ENGINE_TARGET_DX9)
	V( pVB->pD3DVertices->Unlock() );
#elif defined(ENGINE_TARGET_DX10)
	pVB->pD3DVertices->Unmap();
#endif

	VECTOR vScale;
	vScale.fX = tBBox.vMax.fX - tBBox.vMin.fX;
	vScale.fY = tBBox.vMax.fY - tBBox.vMin.fY;
	vScale.fZ = tBBox.vMax.fZ - tBBox.vMin.fZ;
	float fScale = max( vScale.fX, max( vScale.fY, vScale.fZ ) );
	ASSERT_RETZERO( fScale > 0.0f );
	return fScale;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int sgnVCacheMisses = 0;
int sgnVCacheHits = 0;
int sgnVCacheUnique = 0;
int sgnVCacheTotal = 0;
float sgfVCacheUsage = 0.0f;

static void sMeshCheckVertexCacheUsage (
	D3D_MODEL_DEFINITION & tModelDef,
	INDEX_BUFFER_TYPE * pwIndexBuffer,
	int nIndexBuffer )
{
	// should get this out of CAPS
	const int cnVertexCacheSize = 24;
	INDEX_BUFFER_TYPE pnCache[ cnVertexCacheSize ];
	int nUsed;
	int nNext;

	int nHits = 0, nMisses = 0;
	unsigned int nMinVertex = 0xffffffff, nMaxVertex = 0;

	// brute-force analysis of optimal vertex usage
	int nUniqueVerts = 0;
	INDEX_BUFFER_TYPE * pnUniqueCache;
	pnUniqueCache = (INDEX_BUFFER_TYPE *)MALLOC( 0, sizeof( INDEX_BUFFER_TYPE ) * nIndexBuffer );
	for ( int i = 0; i < nIndexBuffer; i++ )
	{
		int j;
		for ( j = 0; j < nUniqueVerts; j++ )
			if ( pnUniqueCache[ j ] == pwIndexBuffer[ i ] )
				break;
		if ( j == nUniqueVerts )
			// unique found
			pnUniqueCache[ nUniqueVerts++ ] = pwIndexBuffer[ i ];
	}
	FREE( 0, pnUniqueCache );

	sgnVCacheTotal  += nIndexBuffer;
	sgnVCacheUnique += nUniqueVerts;

	// my assumption of cache behavior is that it's simple:
	// upon a cache miss, index is added to end of cache; it never "touches" cache on cache hits

	nUsed = nNext = 0;
	for ( int nIndex = 0; nIndex < nIndexBuffer; nIndex++ )
	{
		INDEX_BUFFER_TYPE nVertex = pwIndexBuffer[ nIndex ];
		if ( nVertex < nMinVertex )
			nMinVertex = nVertex;
		if ( nVertex > nMaxVertex )
			nMaxVertex = nVertex;

		// iterate cache
		int nCache;
		for ( nCache = 0; nCache < nUsed; nCache++ )
		{
			if ( pnCache[ nCache ] == pwIndexBuffer[ nIndex ] )
			{
				// cache hit - do nothing
				nHits++;
				sgnVCacheHits++;
				break;
			}
		}

		if ( nCache == nUsed )
		{
			// cache miss - add to cache
			pnCache[ nNext ] = pwIndexBuffer[ nIndex ];
			nNext++;
			nMisses++;
			sgnVCacheMisses++;
		}

		if ( nNext >= cnVertexCacheSize )
			nNext -= cnVertexCacheSize;
		if ( nUsed < cnVertexCacheSize )
			nUsed++;
	}

	// cache usage: 100% = (Misses=UniqueVerts); 0% = (Misses=IndexCount);
	float fModelCacheUsage = 1.0f - ( (float)( nMisses - nUniqueVerts ) / (float)( nIndexBuffer - nUniqueVerts ) );

	sgfVCacheUsage = 1.0f - ( (float)( sgnVCacheMisses - sgnVCacheUnique ) / (float)( sgnVCacheTotal - sgnVCacheUnique ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sFindLowestUVUnitsPerMeter(
	float * pfUVsPerMeter,
	D3D_VERTEX_BUFFER_DEFINITION * pBufferDef,
	INDEX_BUFFER_TYPE * pIndices,
	int nIndexCount,
	int nVertexStart )
{
	ASSERT( pfUVsPerMeter );
	ASSERT( pBufferDef );
	ASSERT( pIndices );
	
	//BYTE * pbVerts = (BYTE *)pBufferDef->pLocalData[ 0 ] + dxC_GetVertexSize( 0, pBufferDef ) * nVertexStart;
	const float cfHuge = 1000000.0f;
	float pfTempUVsPerMeter[ TEXCOORDS_MAX_CHANNELS ];

	for ( int t = 0; t < TEXCOORDS_MAX_CHANNELS; t++ )
	{
		pfTempUVsPerMeter[ t ] = cfHuge;
	}

	for ( int i = 0; i < nIndexCount; i += 3 )
	{
		// get triangle area
		VECTOR * pTriPositions[ 3 ] = {
			sGetRigidVertexPosition( pBufferDef, pBufferDef->pLocalData[ 0 ], pIndices[ i + 0 ] ),
			sGetRigidVertexPosition( pBufferDef, pBufferDef->pLocalData[ 0 ], pIndices[ i + 1 ] ),
			sGetRigidVertexPosition( pBufferDef, pBufferDef->pLocalData[ 0 ], pIndices[ i + 2 ] ) };
		ASSERT_BREAK( pTriPositions[ 0 ] && pTriPositions[ 1 ] && pTriPositions[ 2 ] );
		VECTOR vEdge1 = *(pTriPositions[ 0 ]) - *(pTriPositions[ 1 ]);
		VECTOR vEdge2 = *(pTriPositions[ 1 ]) - *(pTriPositions[ 2 ]);
		VECTOR vCross;
		VectorCross( vCross, vEdge1, vEdge2 );
		float fTriArea = VectorLength( vCross );

		for ( int t = 0; t < TEXCOORDS_MAX_CHANNELS; t++ )
		{
			TEXCOORDS_CHANNEL eChannel = (TEXCOORDS_CHANNEL) t;
			float * pfTriTextureCoords[ 3 ] = {
				sGetRigidVertexTextureCoords( pBufferDef, pIndices[ i + 0 ], eChannel ),
				sGetRigidVertexTextureCoords( pBufferDef, pIndices[ i + 1 ], eChannel ),
				sGetRigidVertexTextureCoords( pBufferDef, pIndices[ i + 2 ], eChannel ) };
			if ( !pfTriTextureCoords[ 0 ] || ! pfTriTextureCoords[ 1 ] || ! pfTriTextureCoords[ 2 ] )
				continue;

			float fMaxU = -cfHuge, fMinU = cfHuge, fMaxV = -cfHuge, fMinV = cfHuge;
			for ( int v = 0; v < 3; v++ )
			{
				if ( pfTriTextureCoords[ v ][ 0 ] > fMaxU )
					fMaxU = pfTriTextureCoords[ v ][ 0 ];
				if ( pfTriTextureCoords[ v ][ 0 ] < fMinU )
					fMinU = pfTriTextureCoords[ v ][ 0 ];
				if ( pfTriTextureCoords[ v ][ 1 ] > fMaxV )
					fMaxV = pfTriTextureCoords[ v ][ 1 ];
				if ( pfTriTextureCoords[ v ][ 1 ] < fMinV )
					fMinV = pfTriTextureCoords[ v ][ 1 ];
			}
			float fUVsPerMeter = ( fMaxU - fMinU ) * ( fMaxV - fMinV );
			// ignore triangle UV sets with 0 area -- probably not exported
			if ( fUVsPerMeter > 0.0f )
			{
				fUVsPerMeter /= fTriArea;
				if ( pfTempUVsPerMeter[ t ] > fUVsPerMeter )
				pfTempUVsPerMeter[ t ] = fUVsPerMeter;
			}
		}
	}

	for ( int t = 0; t < TEXCOORDS_MAX_CHANNELS; t++ )
	{
		if ( pfTempUVsPerMeter[ t ] >= cfHuge )
			pfUVsPerMeter[ t ] = 0.0f;
		else
			pfUVsPerMeter[ t ] = sqrtf( pfTempUVsPerMeter[ t ] );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_AddToModelBoundingBox( int nRootModel, int nAddModel )
{
	if ( nRootModel == INVALID_ID )
		return;
	if ( nAddModel == INVALID_ID )
		return;
	// this is adding to the global model def, but it'll do for now

	int nRootModelDef = e_ModelGetDefinition( nRootModel );
	int nAddModelDef  = e_ModelGetDefinition( nAddModel );
	D3D_MODEL_DEFINITION* pRootModelDef = dxC_ModelDefinitionGet( nRootModelDef, LOD_ANY );
	D3D_MODEL_DEFINITION* pAddModelDef = dxC_ModelDefinitionGet( nAddModelDef, LOD_ANY );

	ASSERT_RETURN( pRootModelDef );
	ASSERT_RETURN( pAddModelDef );

	const MATRIX * pMatrix = e_ModelGetWorldScaled( nAddModel );
	MATRIX tMatWorld = *pMatrix;
	VECTOR tBBoxPoints[ 8 ];

	VECTOR * pvMin = (VECTOR*)&pAddModelDef->tRenderBoundingBox.vMin;
	VECTOR * pvMax = (VECTOR*)&pAddModelDef->tRenderBoundingBox.vMax;
	tBBoxPoints[ 0 ] = VECTOR( pvMin->fX, pvMin->fY, pvMin->fZ ); // xyz
	tBBoxPoints[ 1 ] = VECTOR( pvMax->fX, pvMin->fY, pvMin->fZ ); // Xyz
	tBBoxPoints[ 2 ] = VECTOR( pvMin->fX, pvMax->fY, pvMin->fZ ); // xYz
	tBBoxPoints[ 3 ] = VECTOR( pvMax->fX, pvMax->fY, pvMin->fZ ); // XYz
	tBBoxPoints[ 4 ] = VECTOR( pvMin->fX, pvMin->fY, pvMax->fZ ); // xyZ
	tBBoxPoints[ 5 ] = VECTOR( pvMax->fX, pvMin->fY, pvMax->fZ ); // XyZ
	tBBoxPoints[ 6 ] = VECTOR( pvMin->fX, pvMax->fY, pvMax->fZ ); // xYZ
	tBBoxPoints[ 7 ] = VECTOR( pvMax->fX, pvMax->fY, pvMax->fZ ); // XYZ

	for( int i = 0; i < 8; i++ )
	{
		BoundingBoxExpandByPoint( &pRootModelDef->tRenderBoundingBox, &tBBoxPoints[ i ] );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGetVectorAndRotationFromOriginMesh(
	granny_mesh * pGrannyMesh,
	granny_tri_material_group * pTriMaterialGroup,
	VECTOR *pvPos,
	float *pfRotation )
{
	// get the vertices
	SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOC( NULL, GrannyGetMeshVertexCount( pGrannyMesh ) * sizeof( SIMPLE_VERTEX ) );
	GrannyCopyMeshVertices( pGrannyMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );
	ASSERT( pTriMaterialGroup->TriCount == TRIANGLES_PER_ORIGIN );// they should have made a rectangle for this

	// get the indices
	INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOC( NULL, 3 * GrannyGetMeshTriangleCount( pGrannyMesh ) * sizeof( INDEX_BUFFER_TYPE ) );
	GrannyCopyMeshIndices( pGrannyMesh, sizeof ( INDEX_BUFFER_TYPE ), pwIndexBuffer );

	VECTOR vCenter(0, 0, 0);
	for (int nTriangle = 0; nTriangle < TRIANGLES_PER_ORIGIN; nTriangle ++ )
	{
		for (int i = 0; i < 3; i++)
		{
			int nIndex = pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + i ];
			vCenter.fX += pVertices[ nIndex ].vPosition.fX;
			vCenter.fY += pVertices[ nIndex ].vPosition.fY;
			vCenter.fZ += pVertices[ nIndex ].vPosition.fZ;
		}
	}
	vCenter /= 3.0f * TRIANGLES_PER_ORIGIN;
	*pvPos = vCenter;

	int nNormalVertexIndex = pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst ) ];
	VECTOR vNormal = pVertices[ nNormalVertexIndex ].vNormal;
	ASSERT( fabsf( vNormal.fZ ) < 0.001f );
	VECTOR vForward( 0, 1, 0 );
	*pfRotation = acosf ( VectorDot( vNormal, vForward ) );
	if ( vNormal.fX > 0 )
		*pfRotation = - *pfRotation;
	
	FREE ( NULL, pVertices );
	FREE ( NULL, pwIndexBuffer );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMeshGetTokenAndName ( const char * pszInString, char * pszOutName, int * pnOutIndex )
{
	int nInStringLength = PStrLen ( pszInString );
	if (! nInStringLength )
		return FALSE;

	char pszString [ MAX_XML_STRING_LENGTH ];
	PStrCopy ( pszString, pszInString, MAX_XML_STRING_LENGTH );

	char * pTokenContext;
	// cut out the spaces - in case there was an artist typo
	strtok_s ( pszString, " ", &pTokenContext );

	char * pszIndex = pszString + nInStringLength - 1;
	*pnOutIndex = 0;
	while ( pszIndex != pszString )
	{
		int nIndex = PStrToInt ( pszIndex );
		if ( nIndex )
			*pnOutIndex = nIndex;
		if ( *pnOutIndex != 0 && nIndex == 0 )
		{// we are past it... back up and break
			pszIndex ++;
			break;
		}
		pszIndex --;
	}
	if ( pszIndex != pszString )
	{
		*pszIndex = 0;
	} 
	PStrCopy ( pszOutName, pszString, MAX_XML_STRING_LENGTH );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyCollapseMeshByUsage ( 
	WORD * pwTriangleCount,
	int * pnIndexBufferSize,
	INDEX_BUFFER_TYPE * pwIndexBuffer,
	int nMeshTriangleCount, 
	BOOL * pbIsSubsetTriangle )
{
	dxC_CollapseMeshByUsage(
		NULL,
		pwTriangleCount,
		pnIndexBufferSize,
		pwIndexBuffer,
		nMeshTriangleCount,
		pbIsSubsetTriangle,
		NULL );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_CollapseMeshByUsage ( 
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, 
	WORD * pwTriangleCount,
	int * pnIndexBufferSize,
	INDEX_BUFFER_TYPE * pwIndexBuffer,
	int nMeshTriangleCount, 
	BOOL * pbIsSubsetTriangle, 
	BOOL * pbIsSubsetVertex )
{
	// Collapse the vertices and create a map to their new indices
	int * pnVertexIndexToSubset = NULL;
	if ( pVertexBufferDef )
	{
		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );

		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
			ASSERT_RETURN( pVertexBufferDef->pLocalData[ nStream ] );

		pnVertexIndexToSubset = (int*)MALLOCZ( NULL, sizeof( int ) * pVertexBufferDef->nVertexCount );
		ZeroMemory( pnVertexIndexToSubset, sizeof( int ) * pVertexBufferDef->nVertexCount );

		int nSubsetVertexIndex = 0;
		for ( int nVertex = 0; nVertex < pVertexBufferDef->nVertexCount; nVertex ++ )
		{
			if ( !pbIsSubsetVertex[ nVertex ] )
				continue;

			for ( int nStream = 0; nStream < nStreamCount; nStream++ )
			{
				int nVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
				memcpy( (BYTE*)(pVertexBufferDef->pLocalData[ nStream ]) + nSubsetVertexIndex * nVertexSize,
						(BYTE*)(pVertexBufferDef->pLocalData[ nStream ]) + nVertex			  *	nVertexSize,
						nVertexSize );
			}

			pnVertexIndexToSubset[ nVertex ] = nSubsetVertexIndex;
			nSubsetVertexIndex ++;
		}

		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
		{
			// reallocate the vertex buffer
			int nVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
			pVertexBufferDef->nBufferSize[ nStream ] = nSubsetVertexIndex * nVertexSize;
			pVertexBufferDef->pLocalData[ nStream ] = (void*)REALLOCZ( NULL, pVertexBufferDef->pLocalData[ nStream ], pVertexBufferDef->nBufferSize[ nStream ] );
		}

		pVertexBufferDef->nVertexCount = nSubsetVertexIndex;
	}

	// Collapse the Index Buffer using the mapping and IsUsed array - and create a triangle mapping to the subset
	int nSubsetTriangleIndex = 0;
	for ( int nTriangle = 0; nTriangle < nMeshTriangleCount; nTriangle++ )
	{
		if ( ! pbIsSubsetTriangle[ nTriangle ] )
			continue;

		for ( int i = 0; i < 3; i++ )
		{
			if ( pnVertexIndexToSubset )
			{
				pwIndexBuffer[ 3 * nSubsetTriangleIndex + i ] =
					pnVertexIndexToSubset[ pwIndexBuffer[ 3 * nTriangle + i ] ];
			} else {
				pwIndexBuffer[ 3 * nSubsetTriangleIndex + i ] =	pwIndexBuffer[ 3 * nTriangle + i ];
			}
		}
		nSubsetTriangleIndex ++;
	}
	*pwTriangleCount = nSubsetTriangleIndex;
	// reallocate the index buffer
	*pnIndexBufferSize = *pwTriangleCount * sizeof (INDEX_BUFFER_TYPE) * 3;

	if ( pnVertexIndexToSubset )
		FREE ( NULL, pnVertexIndexToSubset );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyGetVertices( 
	GRANNY_VERTEX_64*& pVertices, 
	int & nVertexCount, 
	int & nVertexBufferSize, 
	granny_mesh * pGrannyMesh, 
	MATRIX & mOriginTransformMatrix )
{
	nVertexCount = GrannyGetMeshVertexCount( pGrannyMesh );
	nVertexBufferSize = nVertexCount * sizeof( GRANNY_VERTEX_64 );

	GRANNY_RIGID_VERTEX* pGrannyVerts = NULL;
	if ( nVertexBufferSize )
	{
		pVertices = (GRANNY_VERTEX_64*) MALLOCZ( NULL, nVertexBufferSize );
		ZeroMemory( pVertices, nVertexBufferSize );
#if USE_MEMORY_MANAGER
		pGrannyVerts = (GRANNY_RIGID_VERTEX *) MALLOCZ( g_ScratchAllocator, nVertexCount * sizeof( GRANNY_RIGID_VERTEX ) );
#else
		pGrannyVerts = (GRANNY_RIGID_VERTEX *) MALLOCZ( NULL, nVertexCount * sizeof( GRANNY_RIGID_VERTEX ) );
#endif
		GrannyCopyMeshVertices( pGrannyMesh, g_ptGrannyRigidVertexDataType, (BYTE *)pGrannyVerts );
	}

	// Transform the vertices to match our origin mesh and copy them into our vertex buffer
	for ( int nVertex = 0; nVertex < nVertexCount; nVertex++ )
	{
		GRANNY_VERTEX_64* pVertex = & ((GRANNY_VERTEX_64 *)pVertices)[ nVertex ];
		GRANNY_RIGID_VERTEX* pGrannyVertex = & pGrannyVerts[ nVertex ];

		memcpy( pVertex->fTextureCoords, pGrannyVertex->fTextureCoords, sizeof( pVertex->fTextureCoords ) );

		MatrixMultiply( &pVertex->vPosition, (VECTOR*)&pGrannyVertex->vPosition, &mOriginTransformMatrix );
		pVertex->vNormal   = e_GrannyTransformAndCompressNormal	( (VECTOR*)& pGrannyVertex->vNormal,   & mOriginTransformMatrix );
		pVertex->vTangent  = e_GrannyTransformNormal		    ( (VECTOR*)& pGrannyVertex->vTangent,  & mOriginTransformMatrix );
		pVertex->vBinormal = e_GrannyTransformNormal			( (VECTOR*)& pGrannyVertex->vBinormal, & mOriginTransformMatrix );
		pVertex++;
	}
	if ( pGrannyVerts )
#if USE_MEMORY_MANAGER
		FREE ( g_ScratchAllocator, pGrannyVerts );
#else
		FREE ( NULL, pGrannyVerts );
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sAdjustVerticesForLightmap(
	D3D_MESH_DEFINITION & tMeshDef,
	int nLOD,
	D3D_VERTEX_BUFFER_DEFINITION & tVertexBufferDef,
	const char * pszLightMapNameWithPath )
{	
	const char * pszLightMap = pszLightMapNameWithPath;
	
	// check if the local lightmap exists, then check the override path
	OVERRIDE_PATH tLightMapOverride;	
	if ( tMeshDef.dwTexturePathOverride && ! FileExists( pszLightMap ) )
	{
		PStrCopy( tLightMapOverride.szPath, pszLightMapNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
		OverridePathByCode( tMeshDef.dwTexturePathOverride, &tLightMapOverride, nLOD );
		pszLightMap = tLightMapOverride.szPath;
	}

	// get lightmap dimensions
	D3DXIMAGE_INFO tImageInfo;
	TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
	FileGetFullFileName(szFullname, pszLightMap, DEFAULT_FILE_WITH_PATH_SIZE);
	V( dxC_GetImageInfoFromFile( szFullname, &tImageInfo) );


	float fFuzzyPixelSize = 2.0f;
	float fPixelU = fFuzzyPixelSize / (float)tImageInfo.Width;
	float fPixelV = fFuzzyPixelSize / (float)tImageInfo.Height;
	float fRealPixelU = 1.0f / (float)tImageInfo.Width;
	float fRealPixelV = 1.0f / (float)tImageInfo.Height;

	WORD * pwIndexBuffer = (WORD*)tMeshDef.tIndexBufferDef.pLocalData;
	ASSERT_RETURN( pwIndexBuffer );

	// find triangles that are too small in u or v dimension
	for ( int nIndex = 0; nIndex < tMeshDef.wTriangleCount; nIndex += 3 )
	{
		// check tri lightmap coord scale
		// grow if less than 1 pixel in either dimension

		float * pfTextureCoords[ 3 ] = {
			sGetRigidVertexTextureCoords ( &tVertexBufferDef, pwIndexBuffer[ nIndex   ] + tMeshDef.nVertexStart, TEXCOORDS_CHANNEL_LIGHTMAP ),
			sGetRigidVertexTextureCoords ( &tVertexBufferDef, pwIndexBuffer[ nIndex+1 ] + tMeshDef.nVertexStart, TEXCOORDS_CHANNEL_LIGHTMAP ),
			sGetRigidVertexTextureCoords ( &tVertexBufferDef, pwIndexBuffer[ nIndex+2 ] + tMeshDef.nVertexStart, TEXCOORDS_CHANNEL_LIGHTMAP ),
		};
		if ( ! pfTextureCoords[ 0 ] || !pfTextureCoords[ 1 ] || !pfTextureCoords[ 2 ] )
			continue;

		float fMinU = 100.0f, fMaxU = -100.0f, fMinV = 100.0f, fMaxV = -100.0f;
		for ( int nVert = 0; nVert < 3; nVert++ )
		{
			float fU = pfTextureCoords[ nVert ][ 0 ];
			float fV = pfTextureCoords[ nVert ][ 1 ];
			if ( fU < fMinU ) fMinU = fU;
			if ( fU > fMaxU ) fMaxU = fU;
			if ( fV < fMinV ) fMinV = fV;
			if ( fV > fMaxV ) fMaxV = fV;
		}

		float fScaleU = 1.0f;
		float fScaleV = 1.0f;
		float fRangeU = fMaxU - fMinU;
		float fRangeV = fMaxV - fMinV;
		float fScaleBoost = 0.00005f;
		if ( fRangeU < fPixelU && fRangeU > 0.0f )
		{
			fScaleU = fPixelU / fRangeU + fScaleBoost;
		}
		if ( fRangeV < fPixelV && fRangeV > 0.0f )
		{
			fScaleV = fPixelV / fRangeV + fScaleBoost;
		}

		float fCenterU = ( fMinU + fMaxU ) * 0.5f;
		float fCenterV = ( fMinV + fMaxV ) * 0.5f;
		float fHalfPixelU = fPixelU * 0.5f;
		float fHalfPixelV = fPixelV * 0.5f;
		if ( fRangeU == 0.0f )
		{
			pfTextureCoords[ 0 ][ 0 ] = -fHalfPixelU + fCenterU;
			pfTextureCoords[ 1 ][ 0 ] = -fHalfPixelU + fCenterU;
			pfTextureCoords[ 2 ][ 0 ] =  fHalfPixelU + fCenterU;
		}
		if ( fRangeV == 0.0f )
		{
			pfTextureCoords[ 0 ][ 1 ] =  fHalfPixelV + fCenterV;
			pfTextureCoords[ 1 ][ 1 ] = -fHalfPixelV + fCenterV;
			pfTextureCoords[ 2 ][ 1 ] = -fHalfPixelV + fCenterV;
		}

		for ( int nVert = 0; nVert < 3; nVert++ )
		{
			float & rfU = pfTextureCoords[ nVert ][ 0 ];
			rfU = fScaleU * ( rfU - fCenterU ) + fCenterU;
			float & rfV = pfTextureCoords[ nVert ][ 1 ];
			rfV = fScaleV * ( rfV - fCenterV ) + fCenterV;
		}

		// testing how the lightmap precision compensation code worked
		fMinU =  100.0f;
		fMaxU = -100.0f;
		fMinV =  100.0f;
		fMaxV = -100.0f;
		for ( int nVert = 0; nVert < 3; nVert++ )
		{
			float fU = pfTextureCoords[ nVert ][ 0 ];
			float fV = pfTextureCoords[ nVert ][ 1 ];
			if ( fU < fMinU ) fMinU = fU;
			if ( fU > fMaxU ) fMaxU = fU;
			if ( fV < fMinV ) fMinV = fV;
			if ( fV > fMaxV ) fMaxV = fV;
		}
		fRangeU = fMaxU - fMinU;
		fRangeV = fMaxV - fMinV;
		//ASSERT( fRangeU >= fRealPixelU );
		//ASSERT( fRangeV >= fRealPixelV );
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sMeshCalculateVertexType(
	MESH_SAVER & tMeshSaver,
	int nLOD,
	D3D_MESH_DEFINITION & tMeshDef,
	granny_mesh * pGrannyMesh,
	const char * pszFolderIn )
{
#if !ISVERSION(SERVER_VERSION)
	tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_64;
	EFFECT_DEFINITION * pEffectDef = NULL;

	if ( tMeshDef.pszTextures[ TEXTURE_DIFFUSE ][ 0 ] != 0 )
	{
		char szFolder  [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";

		OVERRIDE_PATH tOverride;
		dxC_MeshGetFolders( &tMeshDef, nLOD, szFolder, &tOverride, pszFolderIn, 
			DEFAULT_FILE_WITH_PATH_SIZE, TRUE ); 

		char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrPrintf(szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", tOverride.szPath[0] ? tOverride.szPath : szFolder, tMeshDef.pszTextures[ TEXTURE_DIFFUSE ] );

		char szFileWithFullPath[ MAX_PATH ];
		FileGetFullFileName( szFileWithFullPath, szFile, MAX_PATH );
		//int nTextureId;
		//V( e_TextureNewFromFile( &nTextureId, szFileWithFullPath, TEXTURE_GROUP_BACKGROUND, TEXTURE_DIFFUSE, 
		//	0, 0, NULL, NULL, NULL, NULL, TEXTURE_LOAD_NO_ASYNC_DEFINITION, 0.0f ) );
		//e_TextureAddRef( nTextureId );
		//ASSERT_RETURN( nTextureId != INVALID_ID );

		//D3D_TEXTURE * pTexture = dxC_TextureGet( nTextureId );
		//ASSERT_RETURN( pTexture );
		//ASSERT_RETURN( pTexture->nDefinitionId != INVALID_ID);

		TEXTURE_DEFINITION * pTextureDef = (TEXTURE_DEFINITION *) DefinitionGetByFilename( DEFINITION_GROUP_TEXTURE, szFileWithFullPath, -1, TRUE );
		//TEXTURE_DEFINITION * pTextureDef = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
		ASSERT_RETURN( pTextureDef );

		int nMaterialId;
		V( e_MaterialNew( &nMaterialId, pTextureDef->pszMaterialName, TRUE ) );
		ASSERT_RETURN( nMaterialId != INVALID_ID );

		pEffectDef = e_MaterialGetEffectDef( nMaterialId, SHADER_TYPE_INVALID );
		ASSERT_RETURN( pEffectDef );
		//e_TextureReleaseRef( nTextureId );

	} else {
		tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_16;
	}

	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_COMPRESS_TEX_COORD ) )
		tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_16;
	else if ( pEffectDef && pEffectDef->eVertexFormat != VERTEX_DECL_RIGID_64 && 
			  tMeshDef.pszTextures[ TEXTURE_NORMAL ][ 0 ] == 0 &&
			  tMeshDef.pszTextures[ TEXTURE_DIFFUSE2 ][ 0 ] == 0 )
	{
		if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_NEEDS_NORMAL ) )
			tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_32;
		else if ( tMeshDef.pszTextures[ TEXTURE_LIGHTMAP ][ 0 ] == 0 )
			tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_16;
		else {
			int nGrannyMeshVertexCount = GrannyGetMeshVertexCount( pGrannyMesh );
			GRANNY_RIGID_VERTEX * pGrannyVerts = (GRANNY_RIGID_VERTEX *) MALLOC( NULL, nGrannyMeshVertexCount * sizeof( GRANNY_RIGID_VERTEX ) );
			GrannyCopyMeshVertices( pGrannyMesh, g_ptGrannyRigidVertexDataType, (BYTE *)pGrannyVerts );

			BOOL bHasNonZeroLightCoord = FALSE;
			for ( int i = 0; i < MIN( 15, nGrannyMeshVertexCount ); i++ )
			{
				sRearrangeTextureCoords( &pGrannyVerts[ i ], tMeshSaver.pnTexChannels );
				if ( pGrannyVerts[ i ].fTextureCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ][ 0 ] != 0.0f ||
					pGrannyVerts[ i ].fTextureCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ][ 1 ] != 0.0f )
				{
					bHasNonZeroLightCoord = TRUE;
					break;
				}
			}
			FREE( NULL, pGrannyVerts );

			if ( bHasNonZeroLightCoord )
				tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_32;
			else
				tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_16;
		}
	} else {
		tMeshSaver.eDesiredVertexType = VERTEX_DECL_RIGID_64;
	}
#endif	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMaterialNameIsAdObject( const char * pszName )
{
	DWORD dwTextoreOverridePathCode = GetTextureOverridePathCode( pszName );
	return (AD_OBJECT_PATH_CODE == dwTextoreOverridePathCode) || (NONREPORTING_AD_OBJECT_PATH_CODE == dwTextoreOverridePathCode);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMaterialNameIsNonreportingAd( const char * pszName )
{
	DWORD dwTextoreOverridePathCode = GetTextureOverridePathCode( pszName );
	return (NONREPORTING_AD_OBJECT_PATH_CODE == dwTextoreOverridePathCode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sMeshCheckForSpecialMaterials (
	D3D_MESH_DEFINITION & tMeshDef, 
	granny_mesh * pGrannyMesh,
	granny_material * pMaterial,
	MATRIX & mOriginTransformMatrix,
	ATTACHMENT_DEFINITION ** ppAttachmentDefs,
	int & nAttachmentDefCount,
	const BOOL bIsMax9Model)
{
	// -- Special Materials --
	// -- Debug materials --
	char pszDebugMaterialPrefix[] = "Debug";
	if ( PStrICmp( pMaterial->Name, pszDebugMaterialPrefix, PStrLen( pszDebugMaterialPrefix ) ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Collision --
	if ( PStrICmp( pMaterial->Name, "debug_occluder" ) == 0 )
	{
		SET_MASK( tMeshDef.dwFlags, MESH_FLAG_OCCLUDER );
		CLEAR_MASK( tMeshDef.dwFlags, MESH_FLAG_DEBUG );
	}

	// -- Collision --
	if(e_IsCollisionMaterial(pMaterial->Name, bIsMax9Model))
	{
		tMeshDef.dwFlags |= MESH_FLAG_SILHOUETTE | MESH_FLAG_COLLISION;
	}

	// -- Convex hull --
	if ( PStrICmp( pMaterial->Name, "debug_hull" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_HULL;
		tMeshDef.dwFlags &= ~MESH_FLAG_DEBUG;
	}


	// -- Starting Location -- just use the lowest vertex in the mesh
	if ( PStrICmp( pMaterial->Name, "startinglocation" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Origin -- Already grabbed it, but we should label it here as a layout mesh
	if ( PStrICmp( pMaterial->Name, "origin" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Monster Spawn Point -- 
	if ( PStrICmp( pMaterial->Name, "monsterspawn" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	if ( PStrICmp( pMaterial->Name, sgszObjectPrefix, sgnObjectPrefixLength ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	if ( PStrICmp( pMaterial->Name, sgszMonsterPrefix, sgnMonsterPrefixLength ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Monster Spawn Area -- 
	if ( PStrICmp( pMaterial->Name, "monsterspawnarea" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Particle Systems -- 
	if ( PStrICmp( pMaterial->Name, sgszParticlePrefix, sgnParticleSystemPrefixLength ) == 0 )
	{
		nAttachmentDefCount++;
		*ppAttachmentDefs = (ATTACHMENT_DEFINITION * ) REALLOC( NULL, *ppAttachmentDefs, nAttachmentDefCount * sizeof( ATTACHMENT_DEFINITION ) );
		ATTACHMENT_DEFINITION * pAttachmentDef = &(*ppAttachmentDefs)[ nAttachmentDefCount - 1 ];
		e_AttachmentDefInit( *pAttachmentDef );
		PStrCopy( pAttachmentDef->pszAttached, pMaterial->Name + sgnParticleSystemPrefixLength, MAX_XML_STRING_LENGTH );
		pAttachmentDef->eType = ATTACHMENT_PARTICLE;

		sGetPositionAndNormalFromMesh ( pGrannyMesh, &pAttachmentDef->vPosition, &pAttachmentDef->vNormal, &mOriginTransformMatrix );
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}

	// -- Clip -- add a connection to the room
	if ( PStrICmp( pMaterial->Name, "clip" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_CLIP;
	}

	// -- Path nodes -- nodes for helping monsters move along the walls
	if ( PStrICmp( pMaterial->Name, "path_A" ) == 0 )
	{
		tMeshDef.dwFlags |= MESH_FLAG_DEBUG;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
static void sLightsXfer( 
	BYTE_XFER<mode> & tXfer,
	STATIC_LIGHT_DEFINITION pLightDefinitions[],
	int nLightDefinitionCount )
{
	for ( int nLightDef = 0; nLightDef < nLightDefinitionCount; nLightDef++ )
	{
		dxC_LightDefinitionXfer( tXfer, pLightDefinitions[ nLightDef ] );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
static void sFindLightsAndWriteToBuffer (
	MLI_DEFINITION & tSharedModelDefinition,
	D3D_MODEL_DEFINITION & tModelDefinition,
	granny_file_info * pGrannyFileInfo,
	float fGrannyToGameScale,
	MATRIX & mOriginTransformMatrix,
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>& tSerializedFileSections,
	BYTE_XFER<mode>& tXfer )
{
	// Lights - these are stored in the skeleton
	STATIC_LIGHT_DEFINITION pSpecularLightDefinitions[ MAX_SPECULAR_LIGHTS_PER_MODEL ];
	memclear(pSpecularLightDefinitions, MAX_SPECULAR_LIGHTS_PER_MODEL * sizeof(STATIC_LIGHT_DEFINITION));
	STATIC_LIGHT_DEFINITION pStaticLightDefinitions	 [ MAX_STATIC_LIGHTS_PER_MODEL ];
	memclear(pStaticLightDefinitions, MAX_STATIC_LIGHTS_PER_MODEL * sizeof(STATIC_LIGHT_DEFINITION));
	for ( int nSkeleton = 0; nSkeleton < pGrannyFileInfo->SkeletonCount; nSkeleton++ )
	{
		granny_skeleton * pSkeleton = pGrannyFileInfo->Skeletons[ nSkeleton ];
		for ( int nBone = 0; nBone < pSkeleton->BoneCount; nBone++ )
		{
			granny_bone * pBone = & pSkeleton->Bones[ nBone ];

			struct LIGHT_FIELDS_TO_LOAD
			{
				granny_real32 bUse;
				granny_real32 fType;
				granny_real32 fIntensity;
				granny_real32 fFalloffStart;
				granny_real32 fFalloffEnd;
				granny_real32 fAmbient;
				granny_real32 bAffectSpecular;
				granny_real32 bAffectDiffuse;
				granny_real32 pfColor[3];
			};
			granny_data_type_definition LightFieldsType[] =
			{
				{GrannyReal32Member, "Use"},
				{GrannyReal32Member, "Type"},
				{GrannyReal32Member, "Intensity"},
				{GrannyReal32Member, "Far Attenuation Start"},
				{GrannyReal32Member, "Far Attenuation End"},
				{GrannyReal32Member, "Ambient Only"},
				{GrannyReal32Member, "Affect Specular"},
				{GrannyReal32Member, "Affect Diffuse"},
				{GrannyReal32Member, "Color", 0, 3},
				{GrannyEndMember},
			};

			granny_variant &ExtendedData = pBone->ExtendedData;
			granny_variant LightInfo;

			if (! GrannyFindMatchingMember(	ExtendedData.Type, ExtendedData.Object,	"LightInfo", &LightInfo))
				continue;

			// LightInfo should be a reference to the data
			ASSERT_CONTINUE(LightInfo.Type[0].Type == GrannyReferenceMember);

			granny_data_type_definition *LightType = &LightInfo.Type[0].ReferenceType[0];
			void const *LightObject = *((void const **)(LightInfo.Object));

			LIGHT_FIELDS_TO_LOAD tLightFields;
			structclear(tLightFields);

			GrannyConvertSingleObject(LightType, LightObject, LightFieldsType, &tLightFields);
			//			if ( tLightFields.bUse == 0 )
			//				continue;

			// scale extended world-space data
			tLightFields.fFalloffStart *= fGrannyToGameScale;
			tLightFields.fFalloffEnd   *= fGrannyToGameScale;

			if ( tLightFields.fAmbient != 0 )
			{
				//tRoom.vAmbientLightColor.fX = tLightFields.fIntensity * tLightFields.pfColor[ 0 ];
				//tRoom.vAmbientLightColor.fY = tLightFields.fIntensity * tLightFields.pfColor[ 1 ];
				//tRoom.vAmbientLightColor.fZ = tLightFields.fIntensity * tLightFields.pfColor[ 2 ];
			} 
			else if ( tLightFields.fType == 2.0f ) 
			{
				//tRoom.vDirectionalLightColor.fX = tLightFields.fIntensity * tLightFields.pfColor[ 0 ];
				//tRoom.vDirectionalLightColor.fY = tLightFields.fIntensity * tLightFields.pfColor[ 1 ];
				//tRoom.vDirectionalLightColor.fZ = tLightFields.fIntensity * tLightFields.pfColor[ 2 ];
				//tRoom.vLightDirection.fX = 0.0f; // don't know how to get this out of granny yet.
				//tRoom.vLightDirection.fY = 0.0f;
				//tRoom.vLightDirection.fZ = -1.0f;
			} else {
				STATIC_LIGHT_DEFINITION * pLightDef = NULL;
				BOOL bValidLight = FALSE;
				if ( tLightFields.bUse && tLightFields.bAffectSpecular )
				{
					if ( tSharedModelDefinition.nSpecularLightCount < MAX_SPECULAR_LIGHTS_PER_MODEL )
					{
						pLightDef = & pSpecularLightDefinitions[ tSharedModelDefinition.nSpecularLightCount ];
						tSharedModelDefinition.nSpecularLightCount++;
						bValidLight = TRUE;
					} else
					{
						ShowDataWarning( 0, "Too many specular lights in this model!\n%s", tModelDefinition.tHeader.pszName );
					}
				} else if ( ! tLightFields.bUse ) {
					if ( tSharedModelDefinition.nStaticLightCount < MAX_STATIC_LIGHTS_PER_MODEL )
					{
						pLightDef = & pStaticLightDefinitions[ tSharedModelDefinition.nStaticLightCount ];
						tSharedModelDefinition.nStaticLightCount++;
						bValidLight = TRUE;
					} else
					{
						ShowDataWarning( 0, "Too many static lights in this model!\n%s", tModelDefinition.tHeader.pszName );
					}
				}
				if ( bValidLight )
				{
					ZeroMemory( pLightDef, sizeof( STATIC_LIGHT_DEFINITION ) );
					pLightDef->fIntensity = tLightFields.fIntensity;
					pLightDef->tFalloff = CFloatPair( tLightFields.fFalloffStart, tLightFields.fFalloffEnd );
					pLightDef->tColor = CFloatTriple( tLightFields.pfColor[ 0 ], tLightFields.pfColor[ 1 ], tLightFields.pfColor[ 2 ] );
					pLightDef->dwFlags |= ( tLightFields.bAffectDiffuse  != 0 ) ? LIGHTDEF_FLAG_DIFFUSE : 0;
					pLightDef->dwFlags |= ( tLightFields.bAffectSpecular != 0 ) ? LIGHTDEF_FLAG_SPECULAR : 0;
					// grab the position after being transformed
					MATRIX mWorldTransform;
					MatrixInverse( &mWorldTransform, (MATRIX *) pBone->InverseWorld4x4 );
					MatrixMultiply( &mWorldTransform, & mWorldTransform, & mOriginTransformMatrix );
					VECTOR4 vLightPosition;
					vLightPosition.fX = vLightPosition.fY = vLightPosition.fZ = 0.0f;
					vLightPosition.fW = 1.0f;
					MatrixMultiply( & vLightPosition, & vLightPosition, & mWorldTransform );
					pLightDef->vPosition = (VECTOR &) vLightPosition;
					pLightDef->fPriority = e_LightPriorityTypeGetValue( LIGHT_PRIORITY_TYPE_BACKGROUND );
				}
			}
		}
	}
	// write lights to file
	DWORD dwBytesWritten = 0;
	if ( tSharedModelDefinition.nStaticLightCount > 0 )
	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_STATIC_LIGHTS;
		pSection->dwOffset = tXfer.GetCursor();
		sLightsXfer( tXfer, pStaticLightDefinitions, tSharedModelDefinition.nStaticLightCount );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
		ASSERT( pSection->dwSize == tSharedModelDefinition.nStaticLightCount * sizeof( STATIC_LIGHT_DEFINITION ) );
	}
	if ( tSharedModelDefinition.nSpecularLightCount > 0 )
	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_SPECULAR_LIGHTS;
		pSection->dwOffset = tXfer.GetCursor();
		sLightsXfer( tXfer, pSpecularLightDefinitions, tSharedModelDefinition.nSpecularLightCount );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
		ASSERT( pSection->dwSize == tSharedModelDefinition.nSpecularLightCount * sizeof( STATIC_LIGHT_DEFINITION ) );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static short sConvertTexcoordFloatToShort( 
	float fCoord,
	const char * pszGrannyFileName )
{
	float fZeroToOne = fCoord / 8.0f;

	if ( fZeroToOne < -1.0f || fZeroToOne > 1.0f )
	{
		//ShowDataWarning( WARNING_TYPE_BACKGROUND, "Texture Coord out of bounds for compression -8,8 %s", pszGrannyFileName );
		fZeroToOne = max( fZeroToOne, -1.0f );
		fZeroToOne = min( fZeroToOne, 1.0f );
	}

	short val = (short) (fZeroToOne * 32767.0f );

	return val;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sConvertGrannyVertexToMeshVertex( 
	int nVertex, 
	GRANNY_RIGID_VERTEX* pGrannyVertex, 
	D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef,
	MATRIX& mOriginTransformMatrix,
	char* pszGrannyFilename )
{	
	switch ( pVertexBufferDef->eVertexType )
	{
	case VERTEX_DECL_RIGID_64:
		{
			ASSERT( dxC_GetStreamCount( pVertexBufferDef ) == VS_BACKGROUND_INPUT_64_STREAM_COUNT );

			for ( UINT nStream = 0; nStream < VS_BACKGROUND_INPUT_64_STREAM_COUNT; nStream++ )
			{
				int nStreamVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
				BYTE* pbStreamVertex = (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + nStreamVertexSize * nVertex;
				ASSERT( pbStreamVertex < (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + pVertexBufferDef->nBufferSize[ nStream ] );

				switch ( nStream )
				{
				case 0:					 
					{
						VS_BACKGROUND_INPUT_64_STREAM_0* pStreamVertex = (VS_BACKGROUND_INPUT_64_STREAM_0*)pbStreamVertex;
						MatrixMultiply( (VECTOR*)&pStreamVertex->Position, (VECTOR*)&pGrannyVertex->vPosition, &mOriginTransformMatrix );
					}
					break;
				case 1:
					{
						VS_BACKGROUND_INPUT_64_STREAM_1* pStreamVertex = (VS_BACKGROUND_INPUT_64_STREAM_1*)pbStreamVertex;
						MemoryCopy( (void*)&pStreamVertex->LightMapDiffuse, sizeof( pStreamVertex->LightMapDiffuse ), 
							pGrannyVertex->fTextureCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], 
							sizeof( pStreamVertex->LightMapDiffuse ) );
					}
					break;
				case 2:
					{
						VS_BACKGROUND_INPUT_64_STREAM_2* pStreamVertex = (VS_BACKGROUND_INPUT_64_STREAM_2*)pbStreamVertex;
						MemoryCopy( (void*)&pStreamVertex->DiffuseMap2, sizeof( pStreamVertex->DiffuseMap2 ), 
							pGrannyVertex->fTextureCoords[ TEXCOORDS_CHANNEL_DIFFUSE2 ], 
							sizeof( pStreamVertex->DiffuseMap2 ) );

						pStreamVertex->Normal   = e_GrannyTransformAndCompressNormal( (VECTOR*)& pGrannyVertex->vNormal,   & mOriginTransformMatrix );
						pStreamVertex->Tangent  = *(D3DXVECTOR3*)&e_GrannyTransformNormal( (VECTOR*)& pGrannyVertex->vTangent,  & mOriginTransformMatrix );
						pStreamVertex->Binormal = *(D3DXVECTOR3*)&e_GrannyTransformNormal( (VECTOR*)& pGrannyVertex->vBinormal, & mOriginTransformMatrix );
					}
					break;
				default:
					ASSERTV( 0, "INVALID STREAM" );
				}
			}
		}
		break;
	case VERTEX_DECL_RIGID_32:
		{
			ASSERT( dxC_GetStreamCount( pVertexBufferDef ) == VS_BACKGROUND_INPUT_32_STREAM_COUNT );

			for ( UINT nStream = 0; nStream < VS_BACKGROUND_INPUT_32_STREAM_COUNT; nStream++ )
			{
				int nStreamVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
				BYTE* pbStreamVertex = (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + nStreamVertexSize * nVertex;
				ASSERT( pbStreamVertex < (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + pVertexBufferDef->nBufferSize[ nStream ] );

				switch ( nStream )
				{
				case 0:					 
					{
						VS_BACKGROUND_INPUT_32_STREAM_0* pStreamVertex = (VS_BACKGROUND_INPUT_32_STREAM_0*)pbStreamVertex;
						MatrixMultiply( (VECTOR*)&pStreamVertex->Position, (VECTOR*)&pGrannyVertex->vPosition, &mOriginTransformMatrix );
					}
					break;
				case 1:
					{
						VS_BACKGROUND_INPUT_32_STREAM_1* pStreamVertex = (VS_BACKGROUND_INPUT_32_STREAM_1*)pbStreamVertex;
						MemoryCopy( (void*)&pStreamVertex->LightMapDiffuse, sizeof( pStreamVertex->LightMapDiffuse ), 
							pGrannyVertex->fTextureCoords[ TEXCOORDS_CHANNEL_LIGHTMAP ], 
							sizeof( pStreamVertex->LightMapDiffuse ) );
					}
					break;
				case 2:
					{
						VS_BACKGROUND_INPUT_32_STREAM_2* pStreamVertex = (VS_BACKGROUND_INPUT_32_STREAM_2*)pbStreamVertex;
						pStreamVertex->Normal = e_GrannyTransformAndCompressNormal( (VECTOR*)& pGrannyVertex->vNormal, &mOriginTransformMatrix );
					}
					break;
				default:
					ASSERTV( 0, "INVALID STREAM" );
				}
			}			
		}
		break;
	case VERTEX_DECL_RIGID_16:
		{
			ASSERT( dxC_GetStreamCount( pVertexBufferDef ) == VS_BACKGROUND_INPUT_16_STREAM_COUNT );

			for ( UINT nStream = 0; nStream < VS_BACKGROUND_INPUT_16_STREAM_COUNT; nStream++ )
			{
				int nStreamVertexSize = dxC_GetVertexSize( nStream, pVertexBufferDef );
				BYTE* pbStreamVertex = (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + nStreamVertexSize * nVertex;
				ASSERT( pbStreamVertex < (BYTE*)pVertexBufferDef->pLocalData[ nStream ] + pVertexBufferDef->nBufferSize[ nStream ] );

				switch ( nStream )
				{
				case 0:					 
					{
						VS_BACKGROUND_INPUT_16_STREAM_0* pStreamVertex = (VS_BACKGROUND_INPUT_16_STREAM_0*)pbStreamVertex;
						MatrixMultiply( (VECTOR*)&pStreamVertex->Position, (VECTOR*)&pGrannyVertex->vPosition, &mOriginTransformMatrix );
					}
					break;
				case 1:
					{
						VS_BACKGROUND_INPUT_16_STREAM_1* pStreamVertex = (VS_BACKGROUND_INPUT_16_STREAM_1*)pbStreamVertex;
						pStreamVertex->TexCoord[ 0 ] = sConvertTexcoordFloatToShort( pGrannyVertex->fTextureCoords[ TEXCOORDS_CHANNEL_DIFFUSE ][ 0 ], pszGrannyFilename );
						pStreamVertex->TexCoord[ 1 ] = sConvertTexcoordFloatToShort( pGrannyVertex->fTextureCoords[ TEXCOORDS_CHANNEL_DIFFUSE ][ 1 ], pszGrannyFilename );
					}
					break;
				default:
					ASSERTV( 0, "INVALID STREAM" );
				}
			}
		}
		break;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sCheckExport( 
	D3D_MESH_DEFINITION & tMeshDef, 
	D3D_VERTEX_BUFFER_DEFINITION & tVertexBufferDef,
	BOOL bHasRoomData,
	const char * pszGrannyFileName )
{
	if ( tVertexBufferDef.eVertexType != VERTEX_DECL_RIGID_64 )
		return;

	if ( tMeshDef.pszTextures[ TEXTURE_NORMAL ][ 0 ] != 0 )
	{
		// check for tangent/binormals
		D3DXVECTOR3 vVecZero(0,0,0);
		VS_BACKGROUND_INPUT_64_STREAM_2* pVertex = (VS_BACKGROUND_INPUT_64_STREAM_2*) ((BYTE*)tVertexBufferDef.pLocalData[ 2 ] + tMeshDef.nVertexStart * sizeof( VS_BACKGROUND_INPUT_64_STREAM_2 ));
		int nMaxVerts = min( tMeshDef.nVertexCount, 10 );
		int i;
		for ( i = 0; i < nMaxVerts; i++ )
		{
			if ( pVertex[ i ].Binormal != vVecZero && pVertex[ i ].Tangent != vVecZero )
				break;
		}
		if ( i == nMaxVerts )
		{
			ShowDataWarning( bHasRoomData ? WARNING_TYPE_BACKGROUND : 0, "Granny file %s has normal maps, but is missing tangent/binormals!", pszGrannyFileName );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_GrannyUpdateModelFileEx (
	const char * pszFileName, 
	BOOL bHasRoomData, 
	bool bSourceFileOptional,
	int nLOD )
{
	granny_file * pGrannyFile = NULL;
	HANDLE hModelFile = INVALID_HANDLE_VALUE;
	SIMPLE_DYNAMIC_ARRAY<AD_OBJECT_DEFINITION> tAdObjectDefinitions;
	ArrayInit(tAdObjectDefinitions, NULL, 1);

	BOOL bResult = FALSE;

	char pszGrannyFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension(pszGrannyFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName, "gr2");

	char pszModelFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension(pszModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName, D3D_MODEL_FILE_EXTENSION);

	// check and see if we need to update at all
	// FORCE_REMAKE_MODEL_BIN defined in e_granny_rigid.h
#ifndef FORCE_REMAKE_MODEL_BIN
	if ( ! AppCommonAllowAssetUpdate() ||
		 ! FileNeedsUpdate( pszModelFileName, pszGrannyFileName, MODEL_FILE_MAGIC_NUMBER, 
			MODEL_FILE_VERSION, FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG, 0, FALSE, 
			MIN_REQUIRED_MODEL_FILE_VERSION ) )
		return bResult;
#endif

	// CHB 2006.10.20 - Update is indicated. Check for optional source file.
	if (bSourceFileOptional)
	{
		if (::GetFileAttributesA(pszGrannyFileName) == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
	}

	int rootlen;
	const OS_PATH_CHAR * rootpath = AppCommonGetRootDirectory(&rootlen);

	char pszModelFileNameForStorage[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrRemovePath( pszModelFileNameForStorage, DEFAULT_FILE_WITH_PATH_SIZE, rootpath, pszGrannyFileName );
	PStrLower(pszModelFileNameForStorage, DEFAULT_FILE_WITH_PATH_SIZE);

	D3D_MODEL_DEFINITION tModelDefinition;
	structclear( tModelDefinition );

	// -- Timer --
	char pszTimerString[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( pszTimerString, MAX_XML_STRING_LENGTH, "Converting %s to %s", pszGrannyFileName, pszModelFileName );
	TIMER_START( pszTimerString );
	DebugString( OP_MID, STATUS_STRING(Updating model file), pszModelFileName );

	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION> tSerializedFileSections;
	ArrayInit(tSerializedFileSections, NULL, DEFAULT_SERIALIZED_FILE_SECTION_COUNT);
	BYTE_BUF tBuf( (MEMORYPOOL*)NULL, 1024 * 1024 ); // 1MB
	BYTE_XFER<XFER_SAVE> tXfer(&tBuf);

	SIMPLE_DYNAMIC_ARRAY<INDEX_BUFFER_TYPE> tOccluderIndices;
	ArrayInit(tOccluderIndices, NULL, 1);
	SIMPLE_DYNAMIC_ARRAY<VECTOR> tOccluderVertices;
	ArrayInit(tOccluderVertices, NULL, 1);

	trace("GrannyUpdateModelFile(): %s\n", pszModelFileName);

	// -- Load Granny file --
	ASSERT( pszGrannyFileName );
	pGrannyFile = e_GrannyReadFile( pszGrannyFileName );
	if (!pGrannyFile) 
	{
		ErrorDialog("Could Not Find File %s", pszGrannyFileName);
		goto exit;
	}
	trace("GrannyUpdateModelFile(), GrannyReadEntireFile(): %s\n", pszGrannyFileName);

	granny_file_info * pGrannyFileInfo = GrannyGetFileInfo( pGrannyFile );
	ASSERTX( pGrannyFileInfo, pszGrannyFileName );

	BOOL bIsMax9Model = e_IsMax9Exporter(pGrannyFileInfo->ExporterInfo);

	// -- Coordinate System --
	// Tell Granny to transform the file into our coordinate system
	e_GrannyConvertCoordinateSystem ( pGrannyFileInfo );

	// -- Scale --
	float fGrannyToGameScale = 1.0f / pGrannyFileInfo->ArtToolInfo->UnitsPerMeter;

	// -- The Model ---
	// Right now we assumes that there is only one model in the file.
	// If we want to handle more models, then we need to create something like a CMyGrannyModel
	if ( pGrannyFileInfo->ModelCount > 1 )
	{
		ShowDataWarning( bHasRoomData ? WARNING_TYPE_BACKGROUND : 0, "Too many models in %s.  Model count is %d", pszGrannyFileName, pGrannyFileInfo->ModelCount);
	}
	if ( pGrannyFileInfo->ModelCount == 0 )
	{
		ShowDataWarning( bHasRoomData ? WARNING_TYPE_BACKGROUND : 0, "No models in %s.", pszGrannyFileName, pGrannyFileInfo->ModelCount);
		goto exit;
	}
	granny_model * pGrannyModel = pGrannyFileInfo->Models[0]; 

	PStrCopy ( tModelDefinition.tHeader.pszName, pszModelFileNameForStorage, MAX_XML_STRING_LENGTH );

	ATTACHMENT_DEFINITION * pAttachmentDefs = NULL;
	int nAttachmentDefCount = 0;

	// -- Origin Transformation --
	// see if they saved a mesh called origin which is used to label the intended origin
	MATRIX mOriginTransformMatrix;
	MatrixIdentity( &mOriginTransformMatrix );
	for ( int nGrannyMeshIndex = 0; nGrannyMeshIndex < pGrannyModel->MeshBindingCount; ++nGrannyMeshIndex )
	{
		granny_mesh * pGrannyMesh = pGrannyModel->MeshBindings[ nGrannyMeshIndex ].Mesh;
		ASSERTX_CONTINUE( pGrannyMesh, pszGrannyFileName );

		int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount( pGrannyMesh );
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pGrannyMesh );
		ASSERTX_CONTINUE( pGrannyTriangleGroupArray, pszGrannyFileName );
		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{
			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

			ASSERTX_CONTINUE( pGrannyTriangleGroup->MaterialIndex >= 0, pszGrannyFileName );
			ASSERTX_CONTINUE( pGrannyMesh->MaterialBindings, pszGrannyFileName );

			granny_material * pMaterial = pGrannyMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;

			if ( ! pMaterial )
			{
				ShowDataWarning( bHasRoomData ? WARNING_TYPE_BACKGROUND : 0, "Mesh %s is missing a material!\n\nMaterial ID may be missing from Multi/Sub-Object material.", pGrannyMesh->Name );
			}
			ASSERTX_CONTINUE( pMaterial, pszGrannyFileName );

			if ( PStrICmp ( pMaterial->Name, "origin" ) == 0 )
			{
				e_GrannyGetTransformationFromOriginMesh ( &mOriginTransformMatrix, pGrannyMesh, pGrannyTriangleGroup, pszGrannyFileName );
				break;
			}
		}
	}

	BOOL bNormalMaps       = FALSE;
	MESH_SAVER pMeshSavers[ MAX_MESHES_PER_MODEL ];
	char pszLightMapName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char pszLightMapNameWithPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char pszFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	char pszTextureName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	
	// Meshes
	int nSavedMeshIndex = 0;
	for ( int nGrannyMeshIndex = 0; nGrannyMeshIndex < pGrannyModel->MeshBindingCount; ++nGrannyMeshIndex )
	{
		granny_mesh * pGrannyMesh = pGrannyModel->MeshBindings[ nGrannyMeshIndex ].Mesh;
		ASSERTX_CONTINUE( pGrannyMesh, pszGrannyFileName );

		// Grab mesh vertex and index data for use later
		int nGrannyMeshIndexCount = GrannyGetMeshIndexCount ( pGrannyMesh );
		int nGrannyMeshVertexCount = GrannyGetMeshVertexCount( pGrannyMesh );

		ASSERTX_CONTINUE( nGrannyMeshIndexCount, pszGrannyFileName );
		ASSERTX_CONTINUE( nGrannyMeshIndexCount % 3 == 0, pszGrannyFileName );
		INDEX_BUFFER_TYPE* pwGrannyMeshIndexBuffer = MALLOCZ_TYPE( INDEX_BUFFER_TYPE, NULL, nGrannyMeshIndexCount );
		GrannyCopyMeshIndices( pGrannyMesh, sizeof( INDEX_BUFFER_TYPE ), pwGrannyMeshIndexBuffer );

		// Grab Granny Mesh Vertices
		ASSERTX_CONTINUE ( nGrannyMeshVertexCount, pszGrannyFileName );
#if USE_MEMORY_MANAGER
		GRANNY_RIGID_VERTEX* pGrannyVerts = MALLOCZ_TYPE( GRANNY_RIGID_VERTEX, g_ScratchAllocator, nGrannyMeshVertexCount );
#else
		GRANNY_RIGID_VERTEX* pGrannyVerts = MALLOCZ_TYPE( GRANNY_RIGID_VERTEX, NULL, nGrannyMeshVertexCount );
#endif
		GrannyCopyMeshVertices( pGrannyMesh, g_ptGrannyRigidVertexDataType, pGrannyVerts );

		INDEX_BUFFER_TYPE* pwGrannyIndexToMeshIndexMapping = MALLOC_TYPE( INDEX_BUFFER_TYPE, NULL, nGrannyMeshVertexCount );
		memset( pwGrannyIndexToMeshIndexMapping, INVALID_ID, sizeof( INDEX_BUFFER_TYPE ) * nGrannyMeshVertexCount );

		// -- Light map --
		PStrPrintf( pszLightMapName, DEFAULT_FILE_WITH_PATH_SIZE, "%sLightingMap.tga", pGrannyMesh->Name );

		// lightmap
		PStrGetPath(pszFolder, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName);
		PStrPrintf( pszLightMapNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszFolder, pszLightMapName );

		// -- Split up Granny Mesh into our meshes by Triangle Group --
		int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount( pGrannyMesh );
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pGrannyMesh );

		enum
		{
			TEXCOORDS_MASK_DIFFUSE  = MAKE_MASK(0),
			TEXCOORDS_MASK_DIFFUSE2 = MAKE_MASK(1),
			TEXCOORDS_MASK_LIGHTMAP = MAKE_MASK(2),
		};
		BOOL bMeshNormalMap = FALSE;

		// iterate materials first, so we know how to arrange texture coordinate channels
		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{
			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

			// don't mess with triangle groups that don't have vertices
			if (pGrannyTriangleGroup->TriCount == 0 )
				continue;

			// we save a mesh for each triangle group
			MESH_SAVER& tMeshSaver = pMeshSavers[ tModelDefinition.nMeshCount ];
			D3D_MESH_DEFINITION& tMeshDef = tMeshSaver.tMeshDef;
			structclear( tMeshSaver );
			// may use this later
			dxC_IndexBufferDefinitionInit( tMeshSaver.tMeshDef.tIndexBufferDef );

			tMeshSaver.nGrannyMeshIndex = nGrannyMeshIndex;
			tMeshSaver.nGrannyTriangleGroup = nTriangleGroup;
			tMeshDef.ePrimitiveType = DXC_9_10( D3DPT_TRIANGLELIST, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			tMeshDef.nBoneToAttachTo = INVALID_ID;
			tMeshDef.wTriangleCount = pGrannyTriangleGroup->TriCount;
			PStrCopy( tMeshSaver.pszLightmapNameWithPath, pszLightMapNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );

			// -- Material --
			ASSERTX_CONTINUE( pGrannyTriangleGroup->MaterialIndex >= 0, pszGrannyFileName );
			ASSERTV_CONTINUE( pGrannyMesh->MaterialBindings, "Missing material bindings in mesh %s\n\nFile: %s", pGrannyMesh->Name, pszFileName );
			granny_material * pMaterial = pGrannyMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;
			ASSERTX_CONTINUE( pMaterial, pszGrannyFileName );

			// detect texture path override
			if ( pMaterial->Name )
			{
				tMeshDef.dwTexturePathOverride = GetTextureOverridePathCode( pMaterial->Name );
			}

			// -- Textures --
			DWORD dwTexCoordsMask = 0;

			// diffuse map 1
			granny_texture * pGrannyTexture = GrannyGetMaterialTextureByType( pMaterial, GrannyDiffuseColorTexture );
			if ( pGrannyTexture )
			{
				PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
				PStrCopy ( tMeshDef.pszTextures[ TEXTURE_DIFFUSE ], pszTextureName, MAX_XML_STRING_LENGTH );
				dwTexCoordsMask	|= TEXCOORDS_MASK_DIFFUSE;
			}

			// diffuse color 2
			//   this channel name is probably 3DS MAX-specific
			granny_material * pGrannyMaterial = e_GrannyGetMaterialFromChannelName( pMaterial, "Diffuse Color", 1 );
			if ( pGrannyMaterial && pGrannyMaterial->Texture )
			{
				pGrannyTexture = pGrannyMaterial->Texture;
				PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
				PStrCopy ( tMeshDef.pszTextures[ TEXTURE_DIFFUSE2 ], pszTextureName, MAX_XML_STRING_LENGTH );
				dwTexCoordsMask	|= TEXCOORDS_MASK_DIFFUSE2;

				// get the diffuse 2 mapping addressing method (wrap/clamp)
				tMeshDef.nDiffuse2AddressMode[ 0 ] = MESH_TEXTURE_CLAMP;
				tMeshDef.nDiffuse2AddressMode[ 1 ] = MESH_TEXTURE_CLAMP;

				granny_variant tTilingVariant;
				BOOL bFoundUVWTiling = GrannyFindMatchingMember( pGrannyMaterial->ExtendedData.Type, pGrannyMaterial->ExtendedData.Object, "UVWTiling", &tTilingVariant );
				// CHB 2007.06.25 - warning C4002: too many actual parameters for macro 'ASSERT'
				//ASSERT( bFoundUVWTiling, pszGrannyFileName );
				ASSERT( bFoundUVWTiling );
				if ( bFoundUVWTiling )
				{
					if ( *(DWORD*)tTilingVariant.Object & ART_TOOL_TILE_FLAG_U )
						tMeshDef.nDiffuse2AddressMode[ 0 ] = MESH_TEXTURE_WRAP;
					if ( *(DWORD*)tTilingVariant.Object & ART_TOOL_TILE_FLAG_V )
						tMeshDef.nDiffuse2AddressMode[ 1 ] = MESH_TEXTURE_WRAP;
				}
			}

			// lightmap
			OVERRIDE_PATH tLightMapOverride;
			if ( tMeshDef.dwTexturePathOverride )
			{
				PStrCopy( tLightMapOverride.szPath, pszLightMapNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
				OverridePathByCode( tMeshDef.dwTexturePathOverride, &tLightMapOverride, nLOD );
			}
			// check if local dir lightmap exists, then check overridden dir
			if ( FileExists( pszLightMapNameWithPath ) || ( tLightMapOverride.szPath[ 0 ] && FileExists( tLightMapOverride.szPath ) ) )
			{
				PStrCopy( tMeshDef.pszTextures[ TEXTURE_LIGHTMAP ], pszLightMapName, MAX_XML_STRING_LENGTH );
				dwTexCoordsMask	|= TEXCOORDS_MASK_LIGHTMAP;
				tMeshSaver.bAdjustVerticesForLightmap = TRUE;
			} 

			// self-illumination
			pGrannyTexture = GrannyGetMaterialTextureByType( pMaterial, GrannySelfIlluminationTexture );
			if ( pGrannyTexture )
			{
				PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
				PStrCopy( tMeshDef.pszTextures[ TEXTURE_SELFILLUM ], pszTextureName, MAX_XML_STRING_LENGTH );
				dwTexCoordsMask	|= TEXCOORDS_MASK_DIFFUSE;
			}

			// normal
			pGrannyTexture = GrannyGetMaterialTextureByType( pMaterial, GrannyBumpHeightTexture );
			if ( pGrannyTexture )
			{
				PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
				PStrCopy( tMeshDef.pszTextures[ TEXTURE_NORMAL ], pszTextureName, MAX_XML_STRING_LENGTH );
				bNormalMaps    = TRUE;
				bMeshNormalMap = TRUE;
			}

			// displacement
			//   The channel name comes from 3DS MAX
			//pGrannyTexture = e_GrannyGetTextureFromChannelName( pMaterial, "Displacement" );
			//if ( pGrannyTexture )
			//{
			//	PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
			//	PStrCopy( tMeshDef.pszTextures[ TEXTURE_HEIGHT ], pszTextureName, MAX_XML_STRING_LENGTH );
			//}

			// specular
			//   this channel name is probably 3DS MAX specific
			//   if it's not in the obvious first one, try the other possible one
			pGrannyTexture = e_GrannyGetTextureFromChannelName( pMaterial, "Specular Level" );
			if ( ! pGrannyTexture )
				pGrannyTexture = e_GrannyGetTextureFromChannelName( pMaterial, "Specular Color" );
			if ( pGrannyTexture )
			{
				PStrRemoveEntirePath(pszTextureName, MAX_FILE_NAME_LENGTH, pGrannyTexture->FromFileName);
				PStrCopy( tMeshDef.pszTextures[ TEXTURE_SPECULAR ], pszTextureName, MAX_XML_STRING_LENGTH );
			}

			// find out if it's a two-sided material
			granny_variant tTwoSidedVariant;
			BOOL bFoundTwoSided = GrannyFindMatchingMember( pMaterial->ExtendedData.Type, pMaterial->ExtendedData.Object, "Two-sided", &tTwoSidedVariant );
			//ASSERTV( bFoundTwoSided, "Unexpected Material type found!\n\n%s\n\n%s", pMaterial->Name, pszGrannyFileName );
			if ( bFoundTwoSided && *(int*)tTwoSidedVariant.Object != 0 )
				tMeshDef.dwFlags |= MESH_FLAG_TWO_SIDED;

			// move texture coordinates around in vertices based on which textures we're using
			// goal: lightmap diffuse diffuse2
			int nCoordPairSize = sizeof(float) * 2;
			memset( tMeshSaver.pnTexChannels, -1, sizeof(int) * TEXCOORDS_MAX_CHANNELS ); // hard set -1
			// this defines the order they come out of MAX: diffuse, diffuse2, lightmap
			if ( dwTexCoordsMask & TEXCOORDS_MASK_DIFFUSE )
				tMeshSaver.pnTexChannels[ TEXCOORDS_CHANNEL_DIFFUSE ]  = tMeshSaver.nTexChannelCount++;
			if ( dwTexCoordsMask & TEXCOORDS_MASK_DIFFUSE2 )
				tMeshSaver.pnTexChannels[ TEXCOORDS_CHANNEL_DIFFUSE2 ] = tMeshSaver.nTexChannelCount++;
			if ( dwTexCoordsMask & TEXCOORDS_MASK_LIGHTMAP )
				tMeshSaver.pnTexChannels[ TEXCOORDS_CHANNEL_LIGHTMAP ] = tMeshSaver.nTexChannelCount++;

			int nTexCoordSets = 0;
			// trying to pull data about what exactly was exported
			granny_data_type_definition *VertexType = GrannyGetMeshVertexType(pGrannyMesh);
			if ( GrannyGetVertexComponentIndex ( GrannyVertexTextureCoordinatesName "0", VertexType ) >= 0 )
				nTexCoordSets++;
			if ( GrannyGetVertexComponentIndex ( GrannyVertexTextureCoordinatesName "1", VertexType ) >= 0 )
				nTexCoordSets++;
			if ( GrannyGetVertexComponentIndex ( GrannyVertexTextureCoordinatesName "2", VertexType ) >= 0 )
				nTexCoordSets++;

			if ( nTexCoordSets < tMeshSaver.nTexChannelCount )
			{
				ShowDataWarning( bHasRoomData ? WARNING_TYPE_BACKGROUND : 0, "Granny file %s, mesh \"%s\" references more textures than it has tex coord mappings!", pszModelFileName, pGrannyMesh->Name );
			}


			if ( sMaterialNameIsAdObject( pMaterial->Name ) )
			{
				int nCount = tAdObjectDefinitions.Count();
				AD_OBJECT_DEFINITION * pAdObjectDef = ArrayAdd(tAdObjectDefinitions);
				ASSERTX( pAdObjectDef, pszGrannyFileName );

				pAdObjectDef->nMeshIndex = tModelDefinition.nMeshCount;
				pAdObjectDef->dwFlags = 0;
				SETBIT( pAdObjectDef->dwFlags, AODF_NONREPORTING_BIT, sMaterialNameIsNonreportingAd( pMaterial->Name ) );
				V( OverridePathRemoveCode( pAdObjectDef->szName, MAX_ADOBJECT_NAME_LEN, pMaterial->Name ) );
				//PStrCopy( pAdObjectDef->szName, pMaterial->Name, MAX_ADOBJECT_NAME_LEN );

				V_DO_FAILED( sGrannyGetAdObjectFromMesh( pAdObjectDef, pGrannyMesh, pGrannyTriangleGroup, mOriginTransformMatrix ) )
				{
					ShowDataWarning( WARNING_TYPE_BACKGROUND, "Ad object creation failed during Granny file update!\n\nModel: %s\n\nMaterial: %s", tModelDefinition.tHeader.pszName, pMaterial->Name );
				}
			}


			sMeshCheckForSpecialMaterials( tMeshDef, pGrannyMesh, pMaterial, mOriginTransformMatrix, &pAttachmentDefs, nAttachmentDefCount, bIsMax9Model );

			sMeshCalculateVertexType( tMeshSaver, nLOD, tMeshDef, pGrannyMesh, pszFolder );

			// uncomment to enable vertex cache profiling
			//sMeshCheckVertexCacheUsage( tModelDefinition, (INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData, tMeshDef.nVertexCount * 3 );

			// If this is an occluder mesh, then don't add it to the mesh definition.
			// We generate a separate file for occluder meshes.
			if ( TEST_MASK( tMeshDef.dwFlags, MESH_FLAG_OCCLUDER ) )
			{
				if ( tMeshDef.wTriangleCount )
				{
					int nIndexCount = 3 * tMeshDef.wTriangleCount;
					int nIndexOffset = 3 * pGrannyTriangleGroup->TriFirst;
					for ( int nIndex = 0; nIndex < nIndexCount; nIndex++ )
					{
						INDEX_BUFFER_TYPE wGrannyIndex = pwGrannyMeshIndexBuffer[ nIndexOffset + nIndex ];

						if ( pwGrannyIndexToMeshIndexMapping[ wGrannyIndex ] == (WORD)INVALID_ID )
						{
							VECTOR* pvPosition = (VECTOR*)&pGrannyVerts[ wGrannyIndex ].vPosition;
							ASSERT_CONTINUE( pvPosition );
							VECTOR vTransformedPosition;
							MatrixMultiply( &vTransformedPosition, pvPosition, &mOriginTransformMatrix );
							pwGrannyIndexToMeshIndexMapping[ wGrannyIndex ] = tOccluderVertices.Count();
							ArrayAddItem( tOccluderVertices, vTransformedPosition );
						}

						ArrayAddItem( tOccluderIndices, pwGrannyIndexToMeshIndexMapping[ wGrannyIndex ] );
					}
				}
			}
			else
				tModelDefinition.nMeshCount++;

			ASSERTV( tModelDefinition.nMeshCount <= MAX_MESHES_PER_MODEL, "Granny file %s has too many meshes!\n\nMesh count is %d, max is %d!", pszGrannyFileName, tModelDefinition.nMeshCount, MAX_MESHES_PER_MODEL );
		}

#if USE_MEMORY_MANAGER
		FREE( g_ScratchAllocator, pGrannyVerts );
#else
		FREE( NULL, pGrannyVerts );
#endif
		FREE( NULL, pwGrannyMeshIndexBuffer );
		FREE( NULL, pwGrannyIndexToMeshIndexMapping );
	}

	// mark render bounding box for initialization
	BOOL bRenderBBoxInitialized = FALSE;

	// figure out what vertex buffers we need and what meshes will use them
	for ( int i = 0; i < tModelDefinition.nMeshCount; i++ )
	{
		MESH_SAVER & tMeshSaver = pMeshSavers[ i ];
		D3D_MESH_DEFINITION & tMeshDef = tMeshSaver.tMeshDef;

		granny_mesh * pGrannyMesh = pGrannyModel->MeshBindings[ tMeshSaver.nGrannyMeshIndex ].Mesh;
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pGrannyMesh );
		granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ tMeshSaver.nGrannyTriangleGroup ];

		int nGrannyMeshIndexCount = GrannyGetMeshIndexCount ( pGrannyMesh );
		int nGrannyMeshVertexCount = GrannyGetMeshVertexCount( pGrannyMesh );
		ASSERTX_CONTINUE( nGrannyMeshIndexCount, pszGrannyFileName );
		ASSERTX( nGrannyMeshIndexCount % 3 == 0, pszGrannyFileName );
		INDEX_BUFFER_TYPE * pwGrannyMeshIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOCZ( NULL, nGrannyMeshIndexCount * sizeof ( INDEX_BUFFER_TYPE ) );
		GrannyCopyMeshIndices ( pGrannyMesh, sizeof ( INDEX_BUFFER_TYPE ), (BYTE *) pwGrannyMeshIndexBuffer );

		// create the index buffer for this mesh
		int nIndexCount = 3 * tMeshDef.wTriangleCount;
		tMeshDef.tIndexBufferDef.nBufferSize = sizeof( INDEX_BUFFER_TYPE ) * nIndexCount;
		if ( tMeshDef.wTriangleCount )
		{
			tMeshDef.tIndexBufferDef.pLocalData = (void *) MALLOCZ( NULL, tMeshDef.tIndexBufferDef.nBufferSize );
			int nIndexOffset = 3 * pGrannyTriangleGroup->TriFirst;
			memcpy ( tMeshDef.tIndexBufferDef.pLocalData, pwGrannyMeshIndexBuffer + nIndexOffset, tMeshDef.tIndexBufferDef.nBufferSize );
		}

		// find the vertex buffer that we need
		BOOL bMeshIsDebug = (tMeshDef.dwFlags & (MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL)) != 0; 
		BOOL bMeshIsSilhouette = (tMeshDef.dwFlags & MESH_FLAG_SILHOUETTE) != 0; 
		if ( AppIsTugboat() )
		{
		    if( ( tMeshDef.dwFlags & MESH_FLAG_COLLISION ) != 0 )
		    {
			    tMeshDef.dwFlags |= MESH_FLAG_NOCOLLISION;
		    }
		}
		BOOL bNeedNewVB = TRUE;
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = NULL;
		for ( int j = 0; j < tModelDefinition.nVertexBufferDefCount; j++ )
		{
			D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDefCurr = &tModelDefinition.ptVertexBufferDefs[ j ];
			if ( pVertexBufferDefCurr->eVertexType != tMeshSaver.eDesiredVertexType )
				continue;
			if ( ((pVertexBufferDefCurr->dwFlags & VERTEX_BUFFER_FLAG_DEBUG) != 0) != bMeshIsDebug )
				continue;
			if ( ((pVertexBufferDefCurr->dwFlags & VERTEX_BUFFER_FLAG_SILHOUETTE) != 0) != bMeshIsSilhouette )
				continue;

			bNeedNewVB = FALSE;
			tMeshDef.nVertexBufferIndex = j;
			pVertexBufferDef = pVertexBufferDefCurr;
			break;
		}

		if ( bNeedNewVB )
		{
			pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef ( &tModelDefinition, &tMeshDef.nVertexBufferIndex );
			ASSERTX_CONTINUE( pVertexBufferDef, pszGrannyFileName );
			if ( bMeshIsDebug )
				pVertexBufferDef->dwFlags |= VERTEX_BUFFER_FLAG_DEBUG;
			if ( bMeshIsSilhouette )
				pVertexBufferDef->dwFlags |= VERTEX_BUFFER_FLAG_SILHOUETTE;
			pVertexBufferDef->dwFVF = D3DC_FVF_NULL;
			pVertexBufferDef->eVertexType = tMeshSaver.eDesiredVertexType;
			pVertexBufferDef->nVertexCount = 0;
		}
		ASSERTX( pVertexBufferDef, pszGrannyFileName );

		// count the vertices used by this mesh
		BOOL * pbVertexIsUsed = (BOOL *) MALLOCZ( NULL, sizeof(BOOL) * nGrannyMeshVertexCount );
		tMeshDef.nVertexCount = 0;
		ASSERTX_CONTINUE( tMeshSaver.tMeshDef.wTriangleCount, pszGrannyFileName );
		for ( int j = 0; j < tMeshDef.wTriangleCount * 3; j++ )
		{
			int nIndex = pwGrannyMeshIndexBuffer[ pGrannyTriangleGroup->TriFirst * 3 + j ];
			ASSERT_CONTINUE( nIndex < nGrannyMeshVertexCount );
			if ( !pbVertexIsUsed[ nIndex ] )
				tMeshDef.nVertexCount++;
			pbVertexIsUsed[ nIndex ] = TRUE;
		}

		tMeshDef.nVertexStart = pVertexBufferDef->nVertexCount;
		pVertexBufferDef->nVertexCount += tMeshDef.nVertexCount;
		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
		{
			pVertexBufferDef->nBufferSize[ nStream ] = pVertexBufferDef->nVertexCount * dxC_GetVertexSize( nStream, pVertexBufferDef );
			pVertexBufferDef->pLocalData[ nStream ] = (void *)REALLOCZ( NULL, pVertexBufferDef->pLocalData[ nStream ], pVertexBufferDef->nBufferSize[ nStream ] );
		}
		
		ASSERTX_CONTINUE ( nGrannyMeshVertexCount, pszGrannyFileName );

#if USE_MEMORY_MANAGER
		GRANNY_RIGID_VERTEX * pGrannyVerts = (GRANNY_RIGID_VERTEX *) MALLOCZ( g_ScratchAllocator, nGrannyMeshVertexCount * sizeof( GRANNY_RIGID_VERTEX ) );
#else
		GRANNY_RIGID_VERTEX * pGrannyVerts = (GRANNY_RIGID_VERTEX *) MALLOCZ( NULL, nGrannyMeshVertexCount * sizeof( GRANNY_RIGID_VERTEX ) );
#endif
		GrannyCopyMeshVertices( pGrannyMesh, g_ptGrannyRigidVertexDataType, (BYTE *)pGrannyVerts );

		// add the vertices to the vertex buffer and keep track of a mapping of indices
		int nVertexCurr = 0;
		int * pnGrannyToMeshIndexMapping = (int *) MALLOCZ( NULL, sizeof(int) * nGrannyMeshVertexCount );
		for ( int j = 0; j < nGrannyMeshVertexCount; j++ )
		{
			if ( ! pbVertexIsUsed[ j ] )
			{
				pnGrannyToMeshIndexMapping[ j ] = INVALID_ID;
				continue;
			}

			pnGrannyToMeshIndexMapping[ j ] = nVertexCurr;

			GRANNY_RIGID_VERTEX * pGrannyVertex = &pGrannyVerts[ j ];
			sRearrangeTextureCoords( pGrannyVertex, tMeshSaver.pnTexChannels );

			sConvertGrannyVertexToMeshVertex( (tMeshDef.nVertexStart + nVertexCurr),
				pGrannyVertex, pVertexBufferDef, mOriginTransformMatrix, 
				pszGrannyFileName );

			nVertexCurr++;
		}

		// remap indices to match the new vertex buffer
		for ( int j = 0; j < 3 * tMeshDef.wTriangleCount; j++ )
		{
			INDEX_BUFFER_TYPE * pIndex = &((INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData)[ j ];
			*pIndex = pnGrannyToMeshIndexMapping[ *pIndex ];
		}

		//if ( tMeshSaver.pnTexChannels[ TEXCOORDS_CHANNEL_LIGHTMAP ] > 0 && tMeshSaver.bAdjustVerticesForLightmap )
		//{
		//	sAdjustVerticesForLightmap( tMeshDef, *pVertexBuffer, tMeshSaver.pszLightmapNameWithPath );
		//}

		V( dxC_MeshMakeBoundingBox( &tMeshDef, pVertexBufferDef, tMeshDef.wTriangleCount * 3, (INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData ) );


		// assemble "render" bounding box, without debug meshes and z bonus
		DWORD dwIgnoreMeshMask = MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL | MESH_FLAG_SILHOUETTE;
		tMeshDef.pszTextures[ TEXTURE_DIFFUSE ];
		DWORD dwTest = ( MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL | MESH_FLAG_SILHOUETTE ) & tMeshDef.dwFlags;
		if ( ( tMeshDef.dwFlags & dwIgnoreMeshMask ) == 0 && tMeshDef.tIndexBufferDef.pLocalData )
		{
			if ( bRenderBBoxInitialized == FALSE )
			{
				VECTOR * pvPosition = sGetRigidVertexPosition( pVertexBufferDef, pVertexBufferDef->pLocalData[ 0 ], ((INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData)[ 0 ] + tMeshDef.nVertexStart );
				ASSERTX_CONTINUE( pvPosition, pszGrannyFileName );
				tModelDefinition.tRenderBoundingBox.vMin = *pvPosition;
				tModelDefinition.tRenderBoundingBox.vMax = *pvPosition;
				bRenderBBoxInitialized = TRUE;
			}
			for ( int nIndex = 0; nIndex < tMeshDef.wTriangleCount * 3; nIndex++ )
			{
				VECTOR * pvPosition = sGetRigidVertexPosition( pVertexBufferDef, pVertexBufferDef->pLocalData[ 0 ], ((INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData)[ nIndex ] + tMeshDef.nVertexStart );
				ASSERTX_CONTINUE( pvPosition, pszGrannyFileName );
				BoundingBoxExpandByPoint( &tModelDefinition.tRenderBoundingBox, pvPosition );
			}

			ASSERTX( tMeshDef.tIndexBufferDef.nBufferSize == tMeshDef.wTriangleCount * 3 * sizeof( INDEX_BUFFER_TYPE ), pszGrannyFileName );
		}


		sFindLowestUVUnitsPerMeter( tMeshDef.pfUVsPerMeter, pVertexBufferDef, (INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData, tMeshDef.wTriangleCount * 3, tMeshDef.nVertexStart );

		sCheckExport( tMeshDef, *pVertexBufferDef, bHasRoomData, pszGrannyFileName );

		FREE( NULL, pwGrannyMeshIndexBuffer );
		FREE( NULL, pnGrannyToMeshIndexMapping );
		FREE( NULL, pbVertexIsUsed );
#if USE_MEMORY_MANAGER
		FREE( g_ScratchAllocator, pGrannyVerts );
#else
		FREE( NULL, pGrannyVerts );
#endif
	}

	// make sure that renderboundingbox isn't too small in each dimension
	sVerifyBoundingBoxIsValid( tModelDefinition.tRenderBoundingBox );


	// compute ambient occlusion for all meshes
	{
		// make a list of mesh defs first -- we need these to render
		D3D_MESH_DEFINITION ** ppMeshDefs = (D3D_MESH_DEFINITION**)MALLOCZ( NULL, sizeof(D3D_MESH_DEFINITION**) * tModelDefinition.nMeshCount );
		for ( int i = 0; i < tModelDefinition.nMeshCount; i++ )
			ppMeshDefs[i] = &pMeshSavers[ i ].tMeshDef;

		V( dxC_ObscuranceComputeForModelDefinition( ppMeshDefs, &tModelDefinition, FALSE ) );

		FREE( NULL, ppMeshDefs );
	}


	// save all of the meshes
	for ( int i = 0; i < tModelDefinition.nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION & tMeshDef = pMeshSavers[ i ].tMeshDef;
		V( dxC_MeshWriteToBuffer( tXfer, tSerializedFileSections, 
			tModelDefinition, tMeshDef ) );
	}

	// Save Attachments
	tModelDefinition.nAttachmentCount = nAttachmentDefCount;
	for ( int nAttachmentDef = 0; nAttachmentDef < nAttachmentDefCount; nAttachmentDef++ )
	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_ATTACHMENT_DEFINITION;
		pSection->dwOffset = tXfer.GetCursor();
		dxC_AttachmentDefinitionXfer( tXfer, pAttachmentDefs[ nAttachmentDef ] );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
	}

	if ( pAttachmentDefs )
		FREE ( NULL, pAttachmentDefs );

	// save ad objects
	tModelDefinition.nAdObjectDefCount = tAdObjectDefinitions.Count();
	if ( tModelDefinition.nAdObjectDefCount > 0 )
	{
		// have to sort first, so that LOD model definitions match
		tAdObjectDefinitions.QSort( sAdObjectDefinitionsCompare );
		for ( int i = 0; i < tModelDefinition.nAdObjectDefCount; ++i )
			tAdObjectDefinitions[i].nIndex = i;

		for ( int nAdObjectDef = 0; nAdObjectDef < tModelDefinition.nAdObjectDefCount; nAdObjectDef++ )
		{
			SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
			pSection->nSectionType = MODEL_FILE_SECTION_AD_OBJECT_DEFINITION;			
			pSection->dwOffset = tXfer.GetCursor();
			dxC_AdObjectDefinitionXfer( tXfer, tAdObjectDefinitions[ nAdObjectDef ], MODEL_FILE_VERSION, FALSE );
			pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
		}
	}

	V( dxC_ModelDefinitionWriteVerticesBuffers( tXfer, tSerializedFileSections, tModelDefinition ) );

	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_MODEL_DEFINITION;
		pSection->dwOffset = tXfer.GetCursor();
		dxC_ModelDefinitionXfer( tXfer, tModelDefinition, MODEL_FILE_VERSION, FALSE );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		// -- Create Model File --
		hModelFile = CreateFile( pszModelFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hModelFile == INVALID_HANDLE_VALUE )
		{
			if ( DataFileCheck( pszModelFileName ) )
				hModelFile = CreateFile( pszModelFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		}		
		ASSERTX_GOTO( hModelFile != INVALID_HANDLE_VALUE, pszModelFileName, exit );

		BYTE_BUF tHeaderBuf( (MEMORYPOOL*)NULL, sizeof( SERIALIZED_FILE_HEADER ) + 
			sizeof( SERIALIZED_FILE_SECTION ) * DEFAULT_SERIALIZED_FILE_SECTION_COUNT );
		BYTE_XFER<XFER_SAVE> tHeaderXfer(&tHeaderBuf);

		// SERIALIZED_FILE_HEADER inherits from FILE_HEADER
		SERIALIZED_FILE_HEADER tFileHeader;
		tFileHeader.dwMagicNumber	= MODEL_FILE_MAGIC_NUMBER;
		tFileHeader.nVersion		= MODEL_FILE_VERSION;
		tFileHeader.nSFHVersion		= SERIALIZED_FILE_HEADER_VERSION;
		tFileHeader.nNumSections	= tSerializedFileSections.Count();
		tFileHeader.pSections		= tSerializedFileSections.GetPointer( 0 );

		dxC_SerializedFileHeaderXfer( tHeaderXfer, tFileHeader );

		for ( int nSection = 0; nSection < tFileHeader.nNumSections; nSection++ )
		{
			dxC_SerializedFileSectionXfer( tHeaderXfer, 
				tFileHeader.pSections[ nSection ], tFileHeader.nSFHVersion );
		}

		DWORD dwBytesWritten;
		// Write out the header
		UINT nHeaderLen = tHeaderXfer.GetLen();
		tHeaderXfer.SetCursor( 0 );
		WriteFile( hModelFile, tHeaderXfer.GetBufferPointer(), nHeaderLen, &dwBytesWritten, NULL );

		// Now, write out the file data
		UINT nDataLen = tXfer.GetLen();
		tXfer.SetCursor( 0 );
		WriteFile( hModelFile, tXfer.GetBufferPointer(), nDataLen, &dwBytesWritten, NULL );

		CloseHandle( hModelFile );
		hModelFile = INVALID_HANDLE_VALUE;
	}

	if ( nLOD != LOD_LOW )
	{
		// Write out model lod-independent (MLI) data
		// -- Create Model File --
		PStrReplaceExtension( pszModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName, MODEL_LOD_INDEPENDENT_FILE_EXTENSION );
		hModelFile = CreateFile( pszModelFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hModelFile == INVALID_HANDLE_VALUE )
		{
			if ( DataFileCheck( pszModelFileName ) )
				hModelFile = CreateFile( pszModelFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		}		
		ASSERTX_GOTO( hModelFile != INVALID_HANDLE_VALUE, pszModelFileName, exit );
		
		// Re-use tXfer, tSerialzedFileSections since we have already written out the model definition
		ArrayClear(tSerializedFileSections);

		if ( tOccluderIndices.Count() > 0 && tOccluderVertices.Count() > 0 )
		{
			OCCLUDER_DEFINITION tOccluderDefinition;
			structclear( tOccluderDefinition );
			tOccluderDefinition.nIndexCount = tOccluderIndices.Count();
			tOccluderDefinition.pIndices = tOccluderIndices.GetPointer( 0 );
			tOccluderDefinition.nVertexCount = tOccluderVertices.Count();
			tOccluderDefinition.pVertices = tOccluderVertices.GetPointer( 0 );

			SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
			pSection->nSectionType = MODEL_FILE_SECTION_OCCLUDER_DEFINITION;
			pSection->dwOffset = tXfer.GetCursor();
			dxC_OccluderDefinitionXfer( tXfer, tOccluderDefinition );
			pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;
		}

		MLI_DEFINITION tSharedModelDef;
		structclear( tSharedModelDef );
		sFindLightsAndWriteToBuffer( tSharedModelDef, tModelDefinition, pGrannyFileInfo, 
			fGrannyToGameScale, mOriginTransformMatrix, tSerializedFileSections, tXfer );

		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_MLI_DEFINITION;
		pSection->dwOffset = tXfer.GetCursor();
		dxC_SharedModelDefinitionXfer( tXfer, tSharedModelDef );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		BYTE_BUF tHeaderBuf( (MEMORYPOOL*)NULL, sizeof( SERIALIZED_FILE_HEADER ) + 
			sizeof( SERIALIZED_FILE_SECTION ) * DEFAULT_SERIALIZED_FILE_SECTION_COUNT );
		BYTE_XFER<XFER_SAVE> tHeaderXfer(&tHeaderBuf);

		// SERIALIZED_FILE_HEADER inherits from FILE_HEADER
		SERIALIZED_FILE_HEADER tFileHeader;
		tFileHeader.dwMagicNumber	= MLI_FILE_MAGIC_NUMBER;
		tFileHeader.nVersion		= MLI_FILE_VERSION;
		tFileHeader.nSFHVersion		= SERIALIZED_FILE_HEADER_VERSION;
		tFileHeader.nNumSections	= tSerializedFileSections.Count();
		tFileHeader.pSections		= tSerializedFileSections.GetPointer( 0 );

		dxC_SerializedFileHeaderXfer( tHeaderXfer, tFileHeader );

		for ( int nSection = 0; nSection < tFileHeader.nNumSections; nSection++ )
		{
			dxC_SerializedFileSectionXfer( tHeaderXfer, 
				tFileHeader.pSections[ nSection ], tFileHeader.nSFHVersion );
		}

		DWORD dwBytesWritten;
		// Write out the header
		UINT nHeaderLen = tHeaderXfer.GetLen();
		tHeaderXfer.SetCursor( 0 );
		WriteFile( hModelFile, tHeaderXfer.GetBufferPointer(), nHeaderLen, &dwBytesWritten, NULL );

		// Now, write out the file data
		UINT nDataLen = tXfer.GetLen();
		tXfer.SetCursor( 0 );
		WriteFile( hModelFile, tXfer.GetBufferPointer(), nDataLen, &dwBytesWritten, NULL );
	}

	bResult = TRUE;

exit:
	tOccluderVertices.Destroy();
	tOccluderIndices.Destroy();
	tAdObjectDefinitions.Destroy();
	tSerializedFileSections.Destroy();

	if(pGrannyFile)
	{
		GrannyFreeFile ( pGrannyFile );
	}

	if(hModelFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle( hModelFile );
	}

	return bResult;
}

BOOL e_GrannyUpdateModelFile (const char * pszFileName, BOOL bHasRoomData, int nLOD )
{
	return e_GrannyUpdateModelFileEx(pszFileName, bHasRoomData, false, nLOD );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GrannyGetTextCoordsAtPos( 
	D3D_MODEL_DEFINITION * pModelDef,
	D3D_MESH_DEFINITION * pMesh, 
	int nTriangleId, 
	BOOL bLightmap, 
	float & fU, 
	float & fV )
{
	ASSERT_RETINVALIDARG( pMesh && pModelDef );

	ASSERT_RETINVALIDARG( pMesh->nVertexBufferIndex < pModelDef->nVertexBufferDefCount );
	ASSERT_RETINVALIDARG( pMesh->nVertexBufferIndex >= 0 );
	D3D_VERTEX_BUFFER_DEFINITION * pVBufferDef = &pModelDef->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

	if ( pVBufferDef->eVertexType != VERTEX_DECL_RIGID_64 )
		return S_FALSE; // we only do one type right now

	ASSERT_RETFAIL( dxC_VertexBufferD3DExists( pVBufferDef->nD3DBufferID[ 1 ] ) );
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( pVBufferDef->nD3DBufferID[ 1 ] );
	ASSERT_RETFAIL( pVB && pVB->pD3DVertices );

	BYTE * pbVertices = NULL;

#if defined(ENGINE_TARGET_DX9)
	V_RETURN( pVB->pD3DVertices->Lock( 0, 0, (void **)&pbVertices, D3DLOCK_READONLY ) );
#elif defined(ENGINE_TARGET_DX10)
	V_RETURN( pVB->pD3DVertices->Map( D3D10_MAP_READ, 0, (void **)&pbVertices) );
#endif

	if ( ! pbVertices )
	{
#if defined(ENGINE_TARGET_DX9)
		V_RETURN( pVB->pD3DVertices->Unlock() );
#elif defined(ENGINE_TARGET_DX10)
		pVB->pD3DVertices->Unmap();
#endif
		return E_FAIL;
	}

	ASSERT_RETFAIL( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) );
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( pMesh->tIndexBufferDef.nD3DBufferID );
	ASSERT_RETFAIL( pIB && pIB->pD3DIndices );

	INDEX_BUFFER_TYPE * pwIndexBuffer = NULL;
	PRESULT pr;
	V_SAVE( pr, dxC_MapEntireBuffer( pIB->pD3DIndices, D3DLOCK_READONLY, D3D10_MAP_READ, (void **)&pwIndexBuffer ) );

	// This is redundant given we check for the lock below
	//if ( FAILED(hResult) )
	//{
	//	dxC_Unmap( pVB->pD3DVertices );
	//	return FALSE;
	//}

	if ( FAILED(pr) || ! pwIndexBuffer )
	{
		V( dxC_Unmap( pVB->pD3DVertices ) );
		V( dxC_Unmap( pIB->pD3DIndices ) );		
		return E_FAIL;
	}

	ASSERT_RETFAIL( pwIndexBuffer[ 0 ] <= pVBufferDef->nVertexCount );
	ASSERT_RETFAIL( pwIndexBuffer[ 1 ] <= pVBufferDef->nVertexCount );
	ASSERT_RETFAIL( pwIndexBuffer[ 2 ] <= pVBufferDef->nVertexCount );

	TEXCOORDS_CHANNEL eTextCoordChannel = bLightmap ? TEXCOORDS_CHANNEL_LIGHTMAP : TEXCOORDS_CHANNEL_DIFFUSE;

	float * pTexCoords1 = sGetRigidVertexTextureCoords( pVBufferDef, pwIndexBuffer[ 0 ], eTextCoordChannel, pbVertices );
	float * pTexCoords2 = sGetRigidVertexTextureCoords( pVBufferDef, pwIndexBuffer[ 1 ], eTextCoordChannel, pbVertices );
	float * pTexCoords3 = sGetRigidVertexTextureCoords( pVBufferDef, pwIndexBuffer[ 2 ], eTextCoordChannel, pbVertices );

	V_RETURN( dxC_Unmap( pIB->pD3DIndices ) );
	
	float fTextU;
	float fTextV;
	if ( pTexCoords1 && pTexCoords2 && pTexCoords3 )
	{
		fTextU = fU * pTexCoords1[ 0 ] + ( 1.0f - fU ) * pTexCoords2[ 0 ];
		fTextU = fV * fTextU + ( 1.0f - fV ) * pTexCoords3[ 0 ];

		float fTextV = fU * pTexCoords1[ 1 ] + ( 1.0f - fU ) * pTexCoords2[ 1 ];
		fTextV = fV * fTextV + ( 1.0f - fV ) * pTexCoords3[ 1 ];
	} else {
		fTextU = fTextV = 0.0f;
	}

	V_RETURN( dxC_Unmap( pVB->pD3DVertices ) );

	fU = fTextU;
	fV = fTextV;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const BOUNDING_BOX * e_MeshGetBoundingBox( int nModelDef, int nLOD, int nMeshIndex )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	if ( ! pModelDef )
		return NULL;
	return dx9_MeshGetBoundingBox( pModelDef, nMeshIndex );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const BOUNDING_BOX * dx9_MeshGetBoundingBox( D3D_MODEL_DEFINITION * pModelDef, int nMeshIndex )
{
	D3D_MESH_DEFINITION * pMesh = dx9_ModelDefinitionGetMesh( pModelDef, nMeshIndex );
	ASSERT_RETNULL( pMesh );
	return &pMesh->tRenderBoundingBox;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_MeshMakeBoundingBox( D3D_MESH_DEFINITION * pMesh, D3D_VERTEX_BUFFER_DEFINITION * pVBufferDef, int nIndexCount, INDEX_BUFFER_TYPE * pwIBuffer )
{
	ASSERT_RETINVALIDARG( pMesh );
	ASSERT_RETINVALIDARG( nIndexCount );
	ASSERT_RETINVALIDARG( pwIBuffer );

	BOUNDING_BOX * pBBox = &pMesh->tRenderBoundingBox;
	pBBox->vMin = VECTOR( 0,0,0 );
	pBBox->vMax = VECTOR( 0,0,0 );

	ASSERT_RETINVALIDARG( pVBufferDef );

	ASSERT_RETINVALIDARG( pVBufferDef->pLocalData[ 0 ] );
	pBBox->vMin = * sGetRigidVertexPosition( pVBufferDef, pVBufferDef->pLocalData[ 0 ], pwIBuffer[ 0 ] + pMesh->nVertexStart );
	pBBox->vMax = pBBox->vMin;
	for ( int i = 1; i < nIndexCount; i++ )
	{
		BoundingBoxExpandByPoint( pBBox, sGetRigidVertexPosition( pVBufferDef, pVBufferDef->pLocalData[ 0 ], pwIBuffer[ i ] + pMesh->nVertexStart ) );
	}

	// set mesh def flag -- has a bounding box
	pMesh->dwFlags |= MESH_FLAG_BOUNDINGBOX;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyGetVectorAndRotationFromOriginMesh( granny_mesh * pGrannyMesh, granny_tri_material_group * pTriMaterialGroup, VECTOR *pvPos, float *pfRotation )
{
	// get the vertices
	SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOCZ( NULL, GrannyGetMeshVertexCount( pGrannyMesh ) * sizeof( SIMPLE_VERTEX ) );
	GrannyCopyMeshVertices( pGrannyMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );
	ASSERT( pTriMaterialGroup->TriCount == TRIANGLES_PER_ORIGIN );// they should have made a rectangle for this

	// get the indices
	INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOCZ( NULL, 3 * GrannyGetMeshTriangleCount( pGrannyMesh ) * sizeof( INDEX_BUFFER_TYPE ) );
	GrannyCopyMeshIndices( pGrannyMesh, sizeof ( INDEX_BUFFER_TYPE ), pwIndexBuffer );

	VECTOR vCenter(0, 0, 0);
	for (int nTriangle = 0; nTriangle < TRIANGLES_PER_ORIGIN; nTriangle ++ )
	{
		for (int i = 0; i < 3; i++)
		{
			int nIndex = pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst + nTriangle ) + i ];
			vCenter.fX += pVertices[ nIndex ].vPosition.x;
			vCenter.fY += pVertices[ nIndex ].vPosition.y;
			vCenter.fZ += pVertices[ nIndex ].vPosition.z;
		}
	}
	vCenter /= 3.0f * TRIANGLES_PER_ORIGIN;
	*pvPos = vCenter;

	int nNormalVertexIndex = pwIndexBuffer[ 3 * ( pTriMaterialGroup->TriFirst ) ];
	VECTOR vNormal = pVertices[ nNormalVertexIndex ].vNormal;
	ASSERT( fabsf( vNormal.z ) < 0.001f );
	VECTOR vForward( 0, 1, 0 );
	*pfRotation = acosf ( VectorDot( vNormal, vForward ) );
	if ( vNormal.x > 0 )
		*pfRotation = - *pfRotation;

	FREE ( NULL, pVertices );
	FREE ( NULL, pwIndexBuffer );
}
