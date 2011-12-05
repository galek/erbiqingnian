//----------------------------------------------------------------------------
// dx9_anim_model.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


#include "dxC_material.h"
#include "dxC_texture.h"
#include "dxC_anim_model.h"
#include "dxC_wardrobe.h"
#include "dxC_VertexDeclarations.h"
#include "dxC_state.h"
#include "pakfile.h"
#include "dxC_caps.h"

#include "e_featureline.h"	// CHB 2007.06.28 - For E_FEATURELINE_FOURCC
#include "e_optionstate.h"	// CHB 2007.07.11


PFN_APPEARANCE_CREATE_SKYBOX_MODEL				gpfn_AppearanceCreateSkyboxModel = NULL;
PFN_APPEARANCE_REMOVE							gpfn_AppearanceRemove = NULL;
PFN_APPEARANCE_DEF_GET_HAVOK_SKELETON			gpfn_AppearanceDefGetHavokSkeleton = NULL;
PFN_APPEARANCE_DEF_GET_INVERSE_SKIN_TRANSFORM	gpfn_AppearanceDefGetInverseSkinTransform = NULL;
PFN_ANIMATED_MODEL_DEF_JUST_LOADED				gpfn_AnimatedModelDefinitionJustLoaded = NULL;

#ifdef FORCE_REMAKE_MODEL_BIN
#define FORCE_REMAKE_ANIM_MODEL_FILES
#endif

struct ANIMATING_MODEL_DEFINITION_HASH
{
	ANIMATING_MODEL_DEFINITION* pDef[ LOD_COUNT ];
	int nId;
	int nAppearanceDef;
	BOOL bShouldLoadTextures;
	ANIMATING_MODEL_DEFINITION_HASH* pNext;
};

CHash<ANIMATING_MODEL_DEFINITION_HASH> gpAnimatingModelDefinitions;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_AppearanceRemove( int nModelID )
{
	ASSERT_RETFAIL( gpfn_AppearanceRemove != NULL );
	gpfn_AppearanceRemove( nModelID );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sClearExtraAnimatedVertexIndex( 
	VS_ANIMATED_INPUT_STREAM_0 & tVertex )
{
	tVertex.Indices[ 3 ] = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_AppearanceCreateSkyboxModel( struct SKYBOX_DEFINITION * pSkyboxDef, struct SKYBOX_MODEL * pSkyboxModel, const char * pszFolder )
{
	ASSERT_RETFAIL( gpfn_AppearanceCreateSkyboxModel != NULL );
	gpfn_AppearanceCreateSkyboxModel( pSkyboxDef, pSkyboxModel, pszFolder );
	return S_OK;
}

#ifdef HAVOK_ENABLED
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
hkSkeleton * e_AppearanceDefinitionGetHavokSkeleton( int nAppearanceDefId )
{
	ASSERT_RETNULL( gpfn_AppearanceDefGetHavokSkeleton != NULL );
	return (hkSkeleton*)gpfn_AppearanceDefGetHavokSkeleton( nAppearanceDefId );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MATRIX * e_AppearanceDefinitionGetInverseSkinTransform( int nAppearanceDefId )
{
	ASSERT_RETNULL( gpfn_AppearanceDefGetInverseSkinTransform != NULL );
	return gpfn_AppearanceDefGetInverseSkinTransform( nAppearanceDefId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_AnimatedModelDefinitionJustLoaded( 
	int nAppearanceDefId, 
	int nModelDef )
{
	ASSERT_RETURN( gpfn_AnimatedModelDefinitionJustLoaded != NULL );
	return gpfn_AnimatedModelDefinitionJustLoaded( nAppearanceDefId, nModelDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_AnimatedModelDefinitionSystemInit( void )
{
	HashInit( gpAnimatingModelDefinitions, NULL, 256 );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static ANIMATING_MODEL_DEFINITION * sGetAnimatedModelDefinition(
	int nId, int nLOD )
{
	ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGet(gpAnimatingModelDefinitions, nId);
	return pHash ? pHash->pDef[ nLOD ] : NULL;
}

static ANIMATING_MODEL_DEFINITION* sGetAnyAnimatedModelDefinition( int nModelDef )
{
	ANIMATING_MODEL_DEFINITION* pAnimatingModelDefinition = NULL;
	for( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		pAnimatingModelDefinition = sGetAnimatedModelDefinition( nModelDef, nLOD );
		if ( pAnimatingModelDefinition )
			break;
	}

	return pAnimatingModelDefinition;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_AnimatedModelDefinitionExists( int nAnimModelDef, int nLOD /*= LOD_NONE*/ )
{
	if ( nLOD == LOD_NONE )
		return ( HashGet( gpAnimatingModelDefinitions, nAnimModelDef ) != NULL );
	else
		return ( sGetAnimatedModelDefinition( nAnimModelDef, nLOD ) != NULL );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static 
void sFreeAnimatedModelDef( ANIMATING_MODEL_DEFINITION* pAnimModelDefinition )
{
	FREE( NULL, pAnimModelDefinition->pnBoneNameOffsets );
	FREE( NULL, pAnimModelDefinition->pszBoneNameBuffer );
	FREE( NULL, pAnimModelDefinition->pmBoneMatrices );
	FREE( NULL, pAnimModelDefinition );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_AnimatedModelDefinitionSystemClose( void )
{
	ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGetFirst(gpAnimatingModelDefinitions);
	while ( pHash )
	{  
		ANIMATING_MODEL_DEFINITION_HASH* pNext = HashGetNext(gpAnimatingModelDefinitions, pHash);

		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			// this will destroy all of the data for both definitions
			if ( pHash->pDef[ nLOD ] && 
				 !dxC_ModelDefinitionGet( pHash->nId, nLOD ) )
			{
				sFreeAnimatedModelDef( pHash->pDef[ nLOD ] );
			} 
			else
			{
				e_ModelDefinitionDestroy( pHash->nId, nLOD, MODEL_DEFINITION_DESTROY_ALL, TRUE );
			}			
			pHash->pDef[ nLOD ] = NULL;
		}			

		pHash = pNext;
	}
	HashFree(gpAnimatingModelDefinitions);

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_AnimatedModelDefinitionRemovePrivate( 
	int nDefinition,
	int nLOD )
{
	// this performs actual freeing
	ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGet( gpAnimatingModelDefinitions, nDefinition );
	if ( !pHash )
		return S_FALSE;

	ANIMATING_MODEL_DEFINITION* pAnimModelDef = pHash->pDef[ nLOD ];
	if ( !pAnimModelDef )
		return S_FALSE;
	sFreeAnimatedModelDef( pAnimModelDef );
	pHash->pDef[ nLOD ] = NULL;

	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		// Definitions still exist, so don't remove the hash yet.
		if ( pHash->pDef[ nLOD ] )
			return S_FALSE;
	}

	HashRemove( gpAnimatingModelDefinitions, nDefinition );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_AnimatedModelDefinitionGetMeshCount( 
	int nModelDefId, int nLOD )
{
	ANIMATING_MODEL_DEFINITION* pAnimModelDef = sGetAnimatedModelDefinition( nModelDefId, nLOD );
	if ( !pAnimModelDef )
		return 0;

	D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDefId, nLOD );
	if ( !pModelDef )
		return 0;

	return pModelDef->nMeshCount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const int * e_AnimatedModelDefinitionGetBoneMapping( 
	int nModelDef,
	int nLOD,
	int nMeshId)
{
	// Bones should be the same across all LODs, so grab any LOD definition.
	ANIMATING_MODEL_DEFINITION* pAnimModelDef = sGetAnimatedModelDefinition( nModelDef, nLOD );
	if ( !pAnimModelDef )
		return NULL;

	D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	if ( !pModelDef )
		return NULL;

	ASSERT_RETNULL( nMeshId >= 0 && nMeshId < pModelDef->nMeshCount );
	return &pAnimModelDef->ppnBoneMappingModelToMesh[ nMeshId ][ 0 ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_AnimatedModelDefinitionClear ( 
	int nModelDefId,
	int nLOD )
{
	ANIMATING_MODEL_DEFINITION* pAnimModelDef = sGetAnimatedModelDefinition( nModelDefId, nLOD );
	if ( pAnimModelDef )
	{
		D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDefId, nLOD );
		if ( pModelDef )
			pModelDef->nMeshCount = 0;

		ZeroMemory( pAnimModelDef->dwBoneIsSkinned, sizeof( DWORD ) * DWORD_FLAG_SIZE(MAX_BONES_PER_MODEL) );
		return S_OK;
	}
	else
		return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_AnimatedModelDefinitionNewFromWardrobe ( 
	int nModelDefinitionId,
	int nLOD,
	WARDROBE_MODEL* pWardrobeModel )
{
	// -- Animation --
	ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGet( gpAnimatingModelDefinitions, nModelDefinitionId );
	if ( pHash && pHash->pDef[ nLOD ]  )
		return S_FALSE;
	else if ( !pHash )
		pHash = HashAdd( gpAnimatingModelDefinitions, nModelDefinitionId );
	ASSERT( pHash );

	pHash->pDef[ nLOD ] = (ANIMATING_MODEL_DEFINITION*)MALLOCZ( NULL, sizeof( ANIMATING_MODEL_DEFINITION ) );
	
	ANIMATING_MODEL_DEFINITION* pAnimatingModelDefinition = pHash->pDef[ nLOD ];

	pAnimatingModelDefinition->dwFlags = 0;
	pAnimatingModelDefinition->nBoneCount = pWardrobeModel->pEngineData->nBoneCount;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template <class T, int s>
static
const T* sGetAnimatedVertex(const D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, int nIndex, size_t nVertexSize)
{
#if ISVERSION(DEBUG_VERSION)
	ASSERT_RETNULL( nIndex < pVertexBufferDef->nVertexCount );
	ASSERT_RETNULL( pVertexBufferDef->pLocalData[ s ] );
#endif // DEBUG
	return (T*) ((BYTE *) pVertexBufferDef->pLocalData[ s ] + nIndex * nVertexSize );
}

template <class T, int s>
static
T* sGetAnimatedVertex(D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, int nIndex, size_t nVertexSize)
{
	return const_cast<T*>(sGetAnimatedVertex<T,s>(static_cast<const D3D_VERTEX_BUFFER_DEFINITION *>(pVertexBufferDef), nIndex, nVertexSize));
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BYTE sGetAnimationBlendWeight ( 
	const VS_ANIMATED_INPUT_STREAM_0* pVertex, 
	int nIndex )
{
	//DWORD dwMask = pVertex->Weights & ( 0xff << ( 8 * (2 - nIndex) ) );
	//return (BYTE) (dwMask >> ( 8 * (2 - nIndex) ));
	return (BYTE)( pVertex->Weights[ nIndex ] * 255.0f );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_GetAnimatedVertexBuffer ( 
	granny_mesh * pMesh, 
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, 
	granny_skeleton * pGrannySkeleton,
	int * pnBoneMappingGrannyToHavok,
	MATRIX * pmTransform,
	BOUNDING_BOX * pBBox,
	BOOL * pbBoxStarted )
{
	ASSERT_RETINVALIDARG( pMesh );
	ASSERT_RETINVALIDARG( pVertexBufferDef );

	pVertexBufferDef->dwFVF = D3DC_FVF_NULL;
	pVertexBufferDef->eVertexType = VERTEX_DECL_ANIMATED;
	pVertexBufferDef->nVertexCount = GrannyGetMeshVertexCount ( pMesh );
	int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );

	for ( int nStream = 0; nStream < nStreamCount; nStream++ )
	{
		pVertexBufferDef->nBufferSize[ nStream ] = pVertexBufferDef->nVertexCount * dxC_GetVertexSize( nStream, pVertexBufferDef );
		ASSERT_RETFAIL( pVertexBufferDef->nBufferSize[ nStream ] > 0 );
		pVertexBufferDef->pLocalData[ nStream ] = (BYTE*)MALLOC( NULL, pVertexBufferDef->nBufferSize[ nStream ] );
	}

	// Convert Granny's vertices to our format 
	GRANNY_ANIMATED_VERTEX * pGrannyVerts = NULL;
	pGrannyVerts = (GRANNY_ANIMATED_VERTEX *) MALLOC( NULL, pVertexBufferDef->nVertexCount * sizeof( GRANNY_ANIMATED_VERTEX ) );
	GrannyCopyMeshVertices( pMesh, g_ptAnimatedVertexDataType, (BYTE *)pGrannyVerts );

	size_t nVertexSize[3] =
	{
		(size_t)dxC_GetVertexSize( 0, pVertexBufferDef ),
		(size_t)dxC_GetVertexSize( 1, pVertexBufferDef ),
		(size_t)dxC_GetVertexSize( 2, pVertexBufferDef ),
	};

	// convert the granny verts to our format
	for ( int i = 0; i < pVertexBufferDef->nVertexCount; i++ )
	{
		VS_ANIMATED_INPUT_STREAM_0* pVertexS0 = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, i, nVertexSize[0] );
		VS_ANIMATED_INPUT_STREAM_1* pVertexS1 = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_1, 1>( pVertexBufferDef, i, nVertexSize[1] );
		VS_ANIMATED_INPUT_STREAM_2* pVertexS2 = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_2, 2>( pVertexBufferDef, i, nVertexSize[2] );

		GRANNY_ANIMATED_VERTEX* pGrannyVertex = & pGrannyVerts[ i ];
		pVertexS1->DiffuseMap[ 0 ] = pGrannyVertex->fTextureCoords0[ 0 ];
		pVertexS1->DiffuseMap[ 1 ] = pGrannyVertex->fTextureCoords0[ 1 ];
		//memcpy ( pVertex->pwIndices, pGrannyVertex->pwIndices, sizeof( WORD ) * MAX_BONES_PER_VERTEX );
		memcpy ( (void*)&pVertexS0->Indices, pGrannyVertex->pwIndices, sizeof( BYTE ) * MAX_BONES_PER_VERTEX );
		memcpy ( (void*)&pVertexS0->Weights, pGrannyVertex->pflWeights, sizeof( float ) * MAX_BONES_PER_VERTEX );
		sClearExtraAnimatedVertexIndex( *pVertexS0 );

		float fTotalWeight = 0.0f;
		for ( int j = 0; j < MAX_BONES_PER_VERTEX; j++ )
		{
			fTotalWeight += pVertexS0->Weights[ j ];
			ASSERTXONCE_ACTION( pVertexS0->Indices[ j ] < MAX_BONES_PER_MODEL, pMesh->Name, pVertexS0->Indices[ j ] = 0 );
		}
		if ( fTotalWeight < 1.0f && fTotalWeight > 0.0f )
		{
			for ( int j = 0; j < MAX_BONES_PER_VERTEX; j++ )
			{
				pVertexS0->Weights[ j ] = pVertexS0->Weights[ j ] / fTotalWeight;
			}
		}
		// convert into a color
		//pVertex->Weights  = 0;
		//BYTE pbWeights[ 3 ];
		//pbWeights[ 0 ] = (BYTE) (pGrannyVertex->pflWeights[ 0 ] * 255);
		//pbWeights[ 1 ] = (BYTE) (pGrannyVertex->pflWeights[ 1 ] * 255);
		//if ( pbWeights[ 0 ] || pbWeights[ 1 ] )
		//	pbWeights[ 2 ] = 255 - pbWeights[ 0 ] - pbWeights[ 1 ];
		//else
		//{
		//	pbWeights[ 2 ] = 0;
		//	pbWeights[ 0 ] = 255;
		//}

		//pVertex->Weights |= (BYTE)(pbWeights[ 0 ]) << 16;
		//pVertex->Weights |= (BYTE)(pbWeights[ 1 ]) <<  8;
		//pVertex->Weights |= (BYTE)(pbWeights[ 2 ]) <<  0;
		//if (! pVertex->Weights)
		//	pVertex->Weights = (BYTE)((BYTE) (1.0f * 255)) << 16;

		if ( pBBox && pbBoxStarted )
		{
			// must calculate bounding box prior to skin transform
			if ( ! *pbBoxStarted && pVertexBufferDef->nVertexCount > 0)
			{
				*pbBoxStarted = TRUE;
				pBBox->vMax = *(VECTOR*)&pGrannyVertex->vPosition;
				pBBox->vMin = *(VECTOR*)&pGrannyVertex->vPosition;
			}
			BoundingBoxExpandByPoint( pBBox, (VECTOR *) & pGrannyVertex->vPosition );
		}

		if ( pmTransform )
		{
			MatrixMultiplyNormal( (VECTOR *) &pGrannyVertex->vTangent,	(VECTOR *) &pGrannyVertex->vTangent,	pmTransform );
			MatrixMultiplyNormal( (VECTOR *) &pGrannyVertex->vBinormal,	(VECTOR *) &pGrannyVertex->vBinormal,	pmTransform );
			MatrixMultiplyNormal( (VECTOR *) &pGrannyVertex->vNormal,	(VECTOR *) &pGrannyVertex->vNormal,		pmTransform );
			MatrixMultiply		( (VECTOR *) &pGrannyVertex->vPosition,	(VECTOR *) &pGrannyVertex->vPosition,	pmTransform );
		}

		D3DXVec3Normalize( &pGrannyVertex->vTangent,  &pGrannyVertex->vTangent );
		D3DXVec3Normalize( &pGrannyVertex->vBinormal, &pGrannyVertex->vBinormal );
		D3DXVec3Normalize( &pGrannyVertex->vNormal,	  &pGrannyVertex->vNormal );

		//pVertex->vTangent  = dxC_VectorToColor ( pGrannyVertex->vTangent  );
		pVertexS2->Tangent  = pGrannyVertex->vTangent;
		pVertexS2->Binormal = dxC_VectorToColor ( pGrannyVertex->vBinormal );
		//pVertex->vNormal   = dxC_VectorToColor ( pGrannyVertex->vNormal   );
		pVertexS2->Normal   = pGrannyVertex->vNormal;
		pVertexS0->Position = pGrannyVertex->vPosition;
	}
	if ( pGrannyVerts )
		FREE ( NULL, pGrannyVerts );

	// Change the bone indices to match Granny's 
	granny_mesh_binding * pMeshBinding = GrannyNewMeshBinding( pMesh, pGrannySkeleton, pGrannySkeleton );
	int *pnFromBoneIndices = GrannyGetMeshBindingFromBoneIndices( pMeshBinding );

	for ( int nVertex = 0; nVertex < pVertexBufferDef->nVertexCount; nVertex ++ )
	{
		VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, nVertex, nVertexSize[0] );
		for ( int nIndex = 0; nIndex < MAX_BONES_PER_VERTEX; nIndex ++ )
		{
			if ( sGetAnimationBlendWeight ( pVertex, nIndex ) != 0 )
				pVertex->Indices[ nIndex ] = pnFromBoneIndices[ pVertex->Indices[ nIndex ] ];
			else
				pVertex->Indices[ nIndex ] = 0;

			if ( pVertex->Indices[ nIndex ] >= MAX_BONES_PER_MODEL ) // this should already assert when the skeleton is loaded and a bone is missing
				pVertex->Indices[ nIndex ] = 0;
		}
		sClearExtraAnimatedVertexIndex( *pVertex );
	}
	GrannyFreeMeshBinding( pMeshBinding );

	if ( pnBoneMappingGrannyToHavok )
	{
		for ( int nVertex = 0; nVertex < pVertexBufferDef->nVertexCount; nVertex ++ )
		{
			VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, nVertex, nVertexSize[0] );
			for ( int nIndex = 0; nIndex < MAX_BONES_PER_VERTEX; nIndex ++ )
			{
				if ( sGetAnimationBlendWeight ( pVertex, nIndex ) != 0 )
					pVertex->Indices[ nIndex ] = pnBoneMappingGrannyToHavok[ pVertex->Indices[ nIndex ] ];
				else
					pVertex->Indices[ nIndex ] = 0;

				if ( pVertex->Indices[ nIndex ] >= MAX_BONES_PER_MODEL ) // this should already assert when the skeleton is loaded and a bone is missing
					pVertex->Indices[ nIndex ] = 0;
			}
			sClearExtraAnimatedVertexIndex( *pVertex );
		}
	} 

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sCheckVertexFormat ( 
	granny_mesh * pMesh, 
	int nMaterialId, 
	granny_data_type_definition * pDesiredDefinition, 
	const char * pszMeshName, 
	const char * pszFilename )
{
	EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialId, SHADER_TYPE_INVALID );
	if ( !pEffectDef || ! TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_CHECK_FORMAT ) )
		return;
	granny_data_type_definition * pDesiredType = pDesiredDefinition;
	while ( pDesiredType->Type != GrannyEndMember )
	{
		granny_data_type_definition * pMeshType = pMesh->PrimaryVertexData->VertexType;
		while ( pMeshType->Type != GrannyEndMember )
		{
			if ( PStrCmp (pMeshType->Name, pDesiredType->Name) == 0 )
				break;
			pMeshType++;
		}
		if ( pMeshType->Type == GrannyEndMember )
		{
			if ( PStrCmp (pDesiredType->Name, GrannyVertexBoneIndicesName ) != 0 &&
				PStrCmp (pDesiredType->Name, GrannyVertexBoneWeightsName ) != 0 )
			{
				//Error ( "Export mistake. File: %s Mesh: %s needs to export %s", pszFilename, pszMeshName, pDesiredType->Name );
			}
		}
		pDesiredType++;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sMeshIsSkinned( 
	D3D_MODEL_DEFINITION * pModelDefinition, 
	D3D_MESH_DEFINITION * pMesh )
{
	int nUsingBoneIndex = -1;

	for ( int i = 0; i < pMesh->wTriangleCount; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			ASSERT_RETFALSE( pMesh->tIndexBufferDef.pLocalData );
			INDEX_BUFFER_TYPE wIndex = ((INDEX_BUFFER_TYPE*)pMesh->tIndexBufferDef.pLocalData)[ 3 * i + j ] + pMesh->nVertexStart;
			D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];
			ASSERT_CONTINUE( pVertexBufferDef->pLocalData[ 0 ] );
			size_t nVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );
			VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, wIndex, nVertexSize );
			for ( int ii = 0; ii < MAX_BONES_PER_VERTEX; ii++ )	
			{
				if ( sGetAnimationBlendWeight( pVertex, ii ) )
				{
					// if more than one bone is ever referenced, return true
					if ( nUsingBoneIndex < 0 )
						nUsingBoneIndex = pVertex->Indices[ ii ];

					if ( nUsingBoneIndex > 0 )
						return TRUE;

					if ( pVertex->Indices[ ii ] != nUsingBoneIndex )
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// There is a limit on how many bones you can have in a vertex shader.
// So, we need to cut up the draw groups so that they only have MAX_BONES_PER_SHADER
// We are going to try just doing a simple algorithm which doesn't reorder the bones or the vertex indices.
// Hopefully, this will create an adequate division of vertices.  
// If not, we can do something more fancy later.
//
// This can either save the resulting meshes to file or register them with the graphics engine
//-------------------------------------------------------------------------------------------------
static void sSplitMeshesByBones ( 
	D3D_MODEL_DEFINITION * pModelDefinition,
	D3D_MESH_DEFINITION * pMeshDef, 
	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition, 
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>* pSerializedFileSections = NULL,
	BYTE_XFER<XFER_SAVE> * pXfer = NULL )
{
	int nMeshTriangleCount = pMeshDef->tIndexBufferDef.nBufferSize / (3 * sizeof (INDEX_BUFFER_TYPE));
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMeshDef->nVertexBufferIndex ];
	size_t nVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );

	// *** Find Used Bones ***
	// ----- Figure out what bones are used
	BOOL pbBoneIsUsed [ MAX_BONES_PER_MODEL ];
	ZeroMemory ( pbBoneIsUsed, sizeof (pbBoneIsUsed) );

	ASSERT_RETURN( pMeshDef->tIndexBufferDef.pLocalData );

	for ( int i = 0; i < 3 * nMeshTriangleCount; i ++ )
	{
		const VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, ((INDEX_BUFFER_TYPE*)pMeshDef->tIndexBufferDef.pLocalData)[ i ], nVertexSize );

		for ( int j = 0; j < MAX_BONES_PER_VERTEX; j++ )
		{
			if ( sGetAnimationBlendWeight ( pVertex, j ) != 0 )
			{
#if ISVERSION(DEVELOPMENT)
				ASSERTONCE( pVertex->Indices[ j ] < MAX_BONES_PER_MODEL );
#endif // DEVELOPMENT
				if ( pVertex->Indices[ j ] < MAX_BONES_PER_MODEL )
					pbBoneIsUsed [ pVertex->Indices[ j ] ] = TRUE;
			}
		}
	}

	// -- Create a mapping to and from used bones -- 
	int nUsedBoneCount = 0;
	int pnBoneMappingFromUsed [ MAX_BONES_PER_MODEL ];
	int pnBoneMappingToUsed   [ MAX_BONES_PER_MODEL ];
	ZeroMemory ( pnBoneMappingFromUsed, sizeof(int) * MAX_BONES_PER_MODEL );
	ZeroMemory ( pnBoneMappingToUsed,   sizeof(int) * MAX_BONES_PER_MODEL );
	for ( int nBone = 0; nBone < pAnimatingModelDefinition->nBoneCount; nBone ++ )
	{
		if ( pbBoneIsUsed[ nBone ] )
		{
			pnBoneMappingToUsed  [ nBone ]			= nUsedBoneCount;
			pnBoneMappingFromUsed[ nUsedBoneCount ] = nBone;
			nUsedBoneCount++;
		}
	}

	// *** Remap and create mesh if we don't have too many bones ***
	if ( nUsedBoneCount <= MAX_BONES_PER_SHADER_ON_DISK )
	{
		// remap the bones in the vertex buffer
		for ( int nVertex = 0; nVertex < pVertexBufferDef->nVertexCount; nVertex ++ )
		{
			VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, nVertex, nVertexSize );

			for ( int j = 0; j < MAX_BONES_PER_VERTEX; j++ )
			{
				if ( sGetAnimationBlendWeight ( pVertex, j ) != 0 )
				{
					// multiply the index by 3 to help make the shader run faster
					pVertex->Indices[ j ] = 3 * pnBoneMappingToUsed [ pVertex->Indices[ j ] ];
				}
			}
			sClearExtraAnimatedVertexIndex( *pVertex );
		}
		pMeshDef->nVertexCount = pVertexBufferDef->nVertexCount;

		memcpy( &pAnimatingModelDefinition->ppnBoneMappingModelToMesh[ pModelDefinition->nMeshCount ][ 0 ], 
			pnBoneMappingFromUsed, 
			sizeof (int) * MAX_BONES_PER_SHADER_ON_DISK );

		if ( pSerializedFileSections && pXfer )
		{
			ASSERT( pModelDefinition->nMeshCount < MAX_MESHES_PER_MODEL );
			V( dxC_MeshWriteToBuffer( *pXfer, *pSerializedFileSections, 
				*pModelDefinition, *pMeshDef ) );
			pModelDefinition->nMeshCount++;
		} 
		else 
		{
			dx9_ModelDefinitionAddMesh ( pModelDefinition, pMeshDef );
		}
		return;
	}

	// *** Create a subset mesh with some of the Used bones ***

	// --- Figure out what bones, vertices and triangles to use in the subset mesh
	int nSubsetBoneFirst = -MAX_BONES_PER_SHADER_ON_DISK / 4; // the first bone in the used list that we are going to use in this mesh

	// These are buffers to keep track of what triangles and vertices we will be using in the subset mesh.
	BOOL * pbIsSubsetTriangle = (BOOL *) MALLOC(NULL, sizeof (BOOL) * nMeshTriangleCount);
	BOOL * pbIsSubsetVertex   = (BOOL *) MALLOC(NULL, sizeof (BOOL) * pVertexBufferDef->nVertexCount);
	// These buffers tell whether the vertex/triangle will remain in the original mesh
	BOOL * pbIsRemainderTriangle = (BOOL *) MALLOC(NULL, sizeof (BOOL) * nMeshTriangleCount);
	BOOL * pbIsRemainderVertex   = (BOOL *) MALLOC(NULL, sizeof (BOOL) * pVertexBufferDef->nVertexCount);

	int nVerticesInRemainder = 0;
	int nVerticesInSubset = 0;
	while ( nVerticesInRemainder == 0 || nVerticesInSubset == 0 )
	{
		nVerticesInRemainder = 0;
		nVerticesInSubset = 0;
		nSubsetBoneFirst += MAX_BONES_PER_SHADER_ON_DISK / 4;
		nSubsetBoneFirst %= nUsedBoneCount;
		ZeroMemory ( pbIsSubsetTriangle, sizeof (BOOL) * nMeshTriangleCount );
		ZeroMemory ( pbIsSubsetVertex,   sizeof (BOOL) * pVertexBufferDef->nVertexCount );
		ZeroMemory ( pbIsRemainderTriangle, sizeof (BOOL) * nMeshTriangleCount );
		ZeroMemory ( pbIsRemainderVertex,   sizeof (BOOL) * pVertexBufferDef->nVertexCount );

		// go through and see which vertices and triangles are in the subset because they use these bones
		for ( int nTriangle = 0; nTriangle < nMeshTriangleCount; nTriangle++ )
		{
			BOOL fIsInSubset = TRUE;
			for ( int i = 0; i < 3; i++ )
			{
				int nVertexIndex = ((INDEX_BUFFER_TYPE*)pMeshDef->tIndexBufferDef.pLocalData)[ 3 * nTriangle + i ];

				const VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, nVertexIndex, nVertexSize );

				for ( int j = 0; j < MAX_BONES_PER_VERTEX; j++ )
				{
					if ( sGetAnimationBlendWeight ( pVertex, j ) != 0 )
					{
						int nUsedBoneIndex = pnBoneMappingToUsed [ pVertex->Indices[ j ] ];
						if ( nUsedBoneIndex < nSubsetBoneFirst ||
							nUsedBoneIndex >= nSubsetBoneFirst + MAX_BONES_PER_SHADER_ON_DISK)
						{// If it has a bone out of the range, then we can't use this vertex or triangle
							fIsInSubset = FALSE;
						}
					}
				}
			}

			if ( fIsInSubset )
			{
				pbIsSubsetTriangle [ nTriangle ] = TRUE;
				for ( int i = 0; i < 3; i++ )
				{
					pbIsSubsetVertex [ ((INDEX_BUFFER_TYPE*)pMeshDef->tIndexBufferDef.pLocalData)[ 3 * nTriangle + i ] ] = TRUE;
					nVerticesInSubset++;
				}
			}
			else
			{
				pbIsRemainderTriangle [ nTriangle ] = TRUE;
				for ( int i = 0; i < 3; i++ )
				{
					pbIsRemainderVertex [ ((INDEX_BUFFER_TYPE*)pMeshDef->tIndexBufferDef.pLocalData)[ 3 * nTriangle + i ] ] = TRUE;
					nVerticesInRemainder++;
				}
			}
		}
	}

	// --- Create the new subset mesh

	// copy the old mesh over - we will realloc to smaller sizes later
	D3D_MESH_DEFINITION tSubsetMeshDef;
	memcpy ( & tSubsetMeshDef, pMeshDef, sizeof (D3D_MESH_DEFINITION) );

	tSubsetMeshDef.tIndexBufferDef.pLocalData = (void *) MALLOC( NULL, tSubsetMeshDef.tIndexBufferDef.nBufferSize );
	memcpy( tSubsetMeshDef.tIndexBufferDef.pLocalData, pMeshDef->tIndexBufferDef.pLocalData, tSubsetMeshDef.tIndexBufferDef.nBufferSize );

	D3D_VERTEX_BUFFER_DEFINITION * pSubsetVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( pModelDefinition, &tSubsetMeshDef.nVertexBufferIndex );

	// CHB 2006.06.25 - Seems either pModelDefinition->pVertexBuffers or
	// pMeshDef->nVertexBufferIndex can change during this function.
	pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMeshDef->nVertexBufferIndex ];

//	memcpy( pSubsetVertexBuffer, pVertexBuffer, sizeof( D3D_VERTEX_BUFFER ) );
	*pSubsetVertexBufferDef = *pVertexBufferDef;

	int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
	for ( int nStream = 0; nStream < nStreamCount; nStream++ )
	{
		pSubsetVertexBufferDef->pLocalData[ nStream ] = (void *) MALLOC( NULL, pSubsetVertexBufferDef->nBufferSize[ nStream ] );
		memcpy( pSubsetVertexBufferDef->pLocalData[ nStream ], pVertexBufferDef->pLocalData[ nStream ], pVertexBufferDef->nBufferSize[ nStream ] );
	}

	// collapse the meshes
	dxC_CollapseMeshByUsage (pSubsetVertexBufferDef, 
		&tSubsetMeshDef.wTriangleCount,
		&tSubsetMeshDef.tIndexBufferDef.nBufferSize,
		(INDEX_BUFFER_TYPE*)tSubsetMeshDef.tIndexBufferDef.pLocalData, 
		nMeshTriangleCount, 
		pbIsSubsetTriangle,    
		pbIsSubsetVertex );

	// CHB 2006.06.25 - Seems either pModelDefinition->pVertexBuffers or
	// pMeshDef->nVertexBufferIndex can change during this function.
	pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMeshDef->nVertexBufferIndex ];

	dxC_CollapseMeshByUsage (pVertexBufferDef, 
		&pMeshDef->wTriangleCount,
		&pMeshDef->tIndexBufferDef.nBufferSize,
		(INDEX_BUFFER_TYPE*)pMeshDef->tIndexBufferDef.pLocalData, 
		nMeshTriangleCount, 
		pbIsRemainderTriangle, 
		pbIsRemainderVertex );

	FREE ( NULL, pbIsRemainderVertex );
	FREE ( NULL, pbIsRemainderTriangle );
	FREE ( NULL, pbIsSubsetVertex );
	FREE ( NULL, pbIsSubsetTriangle );

	// call recursively until we keep every mesh from having too many bones.
	sSplitMeshesByBones( pModelDefinition, &tSubsetMeshDef, pAnimatingModelDefinition, 
		pSerializedFileSections, pXfer );
	sSplitMeshesByBones( pModelDefinition, pMeshDef, pAnimatingModelDefinition, 
		pSerializedFileSections, pXfer );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_AnimatedModelDefinitionUpdateSkinnedBoneMask ( 
	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition,
	const D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef )
{
	size_t nVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );

	for ( int i = 0; i < pVertexBufferDef->nVertexCount; i++ )
	{
		const VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_0, 0>( pVertexBufferDef, i, nVertexSize );

		for ( int m = 0; m < MAX_BONES_PER_VERTEX; m++ )
		{
			if ( pVertex->Weights[ m ] != 0.0f )
			{
				SETBIT( pAnimatingModelDefinition->dwBoneIsSkinned, pVertex->Indices[ m ], TRUE );
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_AnimatedModelDefinitionSplitMeshByBones ( 
	D3D_MODEL_DEFINITION * pModelDefinition,
	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition,
	D3D_MESH_DEFINITION * pMeshDef )
{
	sSplitMeshesByBones( pModelDefinition, pMeshDef, pAnimatingModelDefinition );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.06.26
/*
	Merge meshes/bones together for high-end hardware, where the
	vertex shaders have enough constants registers to handle a
	larger number of bones per mesh.

	This needs to happen before the (non-animated) model definition
	is initialized, which complicates things slightly -- we'll need
	to build up (i.e., convert offsets to pointers) parts of the
	model definition to do our work, then tear it back down when
	we're done.

	Use a greedy algorithm to find meshes to merge.
*/
static
bool dx9_MeshesAreSameMaterial(const D3D_MESH_DEFINITION * pMesh1, const D3D_MESH_DEFINITION * pMesh2)
{
	if (pMesh1->nMaterialId != pMesh2->nMaterialId) {
		return false;
	}
	// CHB 2006.07.14 - Some "materials" can have the same material
	// ID but different textures.
	for (int i = 0; i < NUM_TEXTURE_TYPES; ++i) {
		if (_stricmp(pMesh1->pszTextures[i], pMesh2->pszTextures[i]) != 0) {
			return false;
		}
	}
	return true;
}

#include <vector>

typedef std::vector<bool> BONE_USAGE_VECTOR_TYPE;

static
const BONE_USAGE_VECTOR_TYPE sGetBoneUsage(
	const ANIMATING_MODEL_DEFINITION* pAnimModel, 
	const D3D_MODEL_DEFINITION* pModelDef,
	D3D_MESH_DEFINITION* pMeshes[], 
	int nMeshIndex )
{
	ASSERT(nMeshIndex < pModelDef->nMeshCount);
	const D3D_MESH_DEFINITION * pMesh = pMeshes[ nMeshIndex ];
	ASSERT(pMesh->ePrimitiveType == D3DPT_TRIANGLELIST);

	const D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDef->ptVertexBufferDefs[pMesh->nVertexBufferIndex];

	int nBones = pAnimModel->nBoneCount;
	BONE_USAGE_VECTOR_TYPE vUsedBones;
	vUsedBones.resize(nBones, false);

//#define ITERATE_TRIANGLES
#ifdef ITERATE_TRIANGLES
	// Iterate over all triangles for the mesh, checking
	// the vertices for each triangle, and the bones for
	// each vertex...
	ASSERT(pMesh->wTriangleCount == (pMesh->nIndexBufferSize / (3 * sizeof(INDEX_BUFFER_TYPE))));
	for (int nTri = 0; nTri < pMesh->wTriangleCount; ++nTri) {
		for (int nVert = 0; nVert < 3; ++nVert) {
			int nVertexIndex = pMesh->pwLocalIndexBuffer[3 * nTri + nVert];
			const VS_ANIMATED_INPUT_STREAM_0* pVertex = sGetAnimatedVertex(pVertexBufferDef, nVertexIndex);
#else
	const VS_ANIMATED_INPUT_STREAM_0* pVertices = static_cast<const VS_ANIMATED_INPUT_STREAM_0*>(static_cast<const void *>(pVertexBufferDef->pLocalData[ 0 ]));
	int nVertexSize = dxC_GetVertexSize( 0, pVertexBufferDef );
	ASSERT( nVertexSize == sizeof(*pVertices));
	ASSERT(pVertexBufferDef->nVertexCount * nVertexSize == pVertexBufferDef->nBufferSize[ 0 ]);
	for (int nVert = 0; nVert < pVertexBufferDef->nVertexCount; ++nVert) {
		const VS_ANIMATED_INPUT_STREAM_0* pVertex = &pVertices[nVert];
#endif
			for (int nBone = 0; nBone < MAX_BONES_PER_VERTEX; ++nBone) {
				// Get mesh's shader-local bone index.
				// Note these are stored pre-multiplied by 3 for indexing.
				int nIndex = pVertex->Indices[nBone];
				ASSERT((nIndex % 3) == 0);
				nIndex /= 3;

				// Get model's bone index.
				ASSERT(nIndex < MAX_BONES_PER_SHADER_RUN_TIME);
				nIndex = pAnimModel->ppnBoneMappingModelToMesh[nMeshIndex][nIndex];
				ASSERT(nIndex < nBones);

				// Mark this bone as used.
				vUsedBones[nIndex] = true;
			}
#ifdef ITERATE_TRIANGLES
		}
#endif
	}

	return vUsedBones;
}

static
int sGetMergeableAndTotalBoneCount(
	const ANIMATING_MODEL_DEFINITION * pAnimModel, 
	const D3D_MODEL_DEFINITION* pModelDef, 
	D3D_MESH_DEFINITION* pMeshes[], 
	int nMesh1, 
	int nMesh2, 
	int& nTotal)
{
	const BONE_USAGE_VECTOR_TYPE vUsedBones1 = sGetBoneUsage(pAnimModel, pModelDef, pMeshes, nMesh1);
	const BONE_USAGE_VECTOR_TYPE vUsedBones2 = sGetBoneUsage(pAnimModel, pModelDef, pMeshes, nMesh2);
	BONE_USAGE_VECTOR_TYPE::size_type nSize = min(vUsedBones1.size(), vUsedBones2.size());

	int nMergeable = 0;
	nTotal = 0;
	for (BONE_USAGE_VECTOR_TYPE::size_type i = 0; i < nSize; ++i) {
		nMergeable += vUsedBones1[i] && vUsedBones2[i];
	}
	for (BONE_USAGE_VECTOR_TYPE::const_iterator i = vUsedBones1.begin(); i != vUsedBones1.end(); ++i) {
		nTotal += *i;
	}
	for (BONE_USAGE_VECTOR_TYPE::const_iterator i = vUsedBones2.begin(); i != vUsedBones2.end(); ++i) {
		nTotal += *i;
	}
	return nMergeable;
}

static void sMergeVertexBuffers(D3D_MODEL_DEFINITION * pModel, D3D_MESH_DEFINITION* pMeshes[], int nMergeToIndex, int nMergeIndex)
{
	ASSERT((0 <= nMergeToIndex) && (nMergeToIndex < pModel->nVertexBufferDefCount));
	ASSERT((0 <= nMergeIndex) && (nMergeIndex < pModel->nVertexBufferDefCount));

	D3D_VERTEX_BUFFER_DEFINITION * pMergeToVertexBufferDef = &pModel->ptVertexBufferDefs[nMergeToIndex];
	D3D_VERTEX_BUFFER_DEFINITION * pMergeVertexBufferDef = &pModel->ptVertexBufferDefs[nMergeIndex];

	int nVertices = pMergeToVertexBufferDef->nVertexCount;

	ASSERT( dxC_GetStreamCount( pMergeToVertexBufferDef ) == dxC_GetStreamCount( pMergeVertexBufferDef ) );
	int nStreamCount = dxC_GetStreamCount( pMergeToVertexBufferDef );
	for ( int nStream = 0; nStream < nStreamCount; nStream++ )
	{
		int nMergeToVertexSize = dxC_GetVertexSize( nStream, pMergeToVertexBufferDef );
		int nMergeVertexSize = dxC_GetVertexSize( nStream, pMergeVertexBufferDef );

		ASSERT(pMergeToVertexBufferDef->nVertexCount * nMergeToVertexSize == pMergeToVertexBufferDef->nBufferSize[ nStream ]);
		ASSERT(pMergeVertexBufferDef->nVertexCount * nMergeVertexSize == pMergeVertexBufferDef->nBufferSize[ nStream ]);
		ASSERT(pMergeToVertexBufferDef->eVertexType == pMergeVertexBufferDef->eVertexType );
		ASSERT(nMergeToVertexSize == nMergeVertexSize);

		// Allocate buffer for new vertex buffer.
		BYTE * pNewVertexBuffer = static_cast<BYTE *>(MALLOC(NULL, pMergeToVertexBufferDef->nBufferSize[ nStream ] + pMergeVertexBufferDef->nBufferSize[ nStream ]));

		// Copy vertices.
		memcpy(pNewVertexBuffer, pMergeToVertexBufferDef->pLocalData[ nStream ], pMergeToVertexBufferDef->nBufferSize[ nStream ]);
		memcpy(pNewVertexBuffer + pMergeToVertexBufferDef->nBufferSize[ nStream ], pMergeVertexBufferDef->pLocalData[ nStream ], pMergeVertexBufferDef->nBufferSize[ nStream ]);

		// Free existing vertex buffer data if necessary.
		FREE(NULL, pMergeToVertexBufferDef->pLocalData[ nStream ]);
		
		FREE(NULL, pMergeVertexBufferDef->pLocalData[ nStream ]);
		pMergeVertexBufferDef->pLocalData[ nStream ] = NULL;
		
		// Set new vertex buffer.
		pMergeToVertexBufferDef->pLocalData[ nStream ] = pNewVertexBuffer;
		pMergeToVertexBufferDef->nBufferSize[ nStream ] += pMergeVertexBufferDef->nBufferSize[ nStream ];
	}
	pMergeToVertexBufferDef->nVertexCount += pMergeVertexBufferDef->nVertexCount;

	// Remove merged vertex buffer.
	if (nMergeIndex + 1 < pModel->nVertexBufferDefCount) {
		for (int i = nMergeIndex; i + 1 < pModel->nVertexBufferDefCount; ++i) {
			pModel->ptVertexBufferDefs[i] = pModel->ptVertexBufferDefs[i + 1];
		}
	}
	ZeroMemory(&pModel->ptVertexBufferDefs[pModel->nVertexBufferDefCount - 1], sizeof(D3D_VERTEX_BUFFER_DEFINITION));
	--pModel->nVertexBufferDefCount;

	// Re-map meshes' vertex buffer indices.
	for (int i = 0; i < pModel->nMeshCount; ++i) 
	{
		D3D_MESH_DEFINITION * pMesh = pMeshes[ i ];
		if (pMesh->nVertexBufferIndex > nMergeIndex) {
			--pMesh->nVertexBufferIndex;
		}
		else if (pMesh->nVertexBufferIndex == nMergeIndex) {
			// Belongs to the vertex buffer that disappeared.
			ASSERT(pMesh->wTriangleCount == (pMesh->tIndexBufferDef.nBufferSize / (3 * sizeof(INDEX_BUFFER_TYPE))));
			for (int nTri = 0; nTri < pMesh->wTriangleCount; ++nTri) {
				for (int nVert = 0; nVert < 3; ++nVert) {
					((INDEX_BUFFER_TYPE*)pMesh->tIndexBufferDef.pLocalData)[3 * nTri + nVert] += nVertices;
				}
			}
			pMesh->nVertexBufferIndex = nMergeToIndex;
		}
	}	
}

static void sRemapVertexBoneIndices(
	ANIMATING_MODEL_DEFINITION * pAnimModel, 
	D3D_MODEL_DEFINITION* pModel,
	D3D_MESH_DEFINITION* pMeshes[],
	int nMeshIndex, 
	const int nModelToMeshMap[] )
{
	D3D_MESH_DEFINITION * pMesh = pMeshes[ nMeshIndex ];
	int nVertexBufferIndex = pMesh->nVertexBufferIndex;
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModel->ptVertexBufferDefs[nVertexBufferIndex];
	VS_ANIMATED_INPUT_STREAM_0* pVertices = static_cast<VS_ANIMATED_INPUT_STREAM_0*>(static_cast<void *>(pVertexBufferDef->pLocalData[ 0 ]));
	for (int i = 0; i < pVertexBufferDef->nVertexCount; ++i) {
		VS_ANIMATED_INPUT_STREAM_0* pVertex = &pVertices[i];
		for (int nWeight = 0; nWeight < MAX_BONES_PER_VERTEX; ++nWeight) {
			// Get mesh's shader-local bone index.
			// Note these are stored pre-multiplied by 3 for indexing.
			int nIndex = pVertex->Indices[nWeight];
			ASSERT((nIndex % 3) == 0);
			nIndex /= 3;

			// Get model's bone index.
			ASSERT(nIndex < MAX_BONES_PER_SHADER_RUN_TIME);
			nIndex = pAnimModel->ppnBoneMappingModelToMesh[nMeshIndex][nIndex];
			ASSERT(nIndex < pAnimModel->nBoneCount);

			// Write new entry.
			// Pre-multiply by 3 for shader indexing.
			ASSERT(nModelToMeshMap[nIndex] >= 0);
			pVertex->Indices[nWeight] = nModelToMeshMap[nIndex] * 3;
		}
		sClearExtraAnimatedVertexIndex( *pVertex );
	}
}

static
int sAnimatedModelDefinitionMergeMeshesByBones(
	ANIMATING_MODEL_DEFINITION * pAnimModel, 
	D3D_MODEL_DEFINITION* pModelDef,
	D3D_MESH_DEFINITION* pMeshes[], 
	int nMergeToIndex, 
	int nMergeIndex )
{
	ASSERT_RETZERO(nMergeToIndex != nMergeIndex);
	ASSERT_RETZERO(nMergeToIndex < pModelDef->nMeshCount);
	ASSERT_RETZERO(nMergeIndex < pModelDef->nMeshCount);

	D3D_MESH_DEFINITION * pMergeToMesh = pMeshes[ nMergeToIndex ];
	D3D_MESH_DEFINITION * pMergeMesh = pMeshes[ nMergeIndex ];

	int nMeshToModelMap[MAX_BONES_PER_SHADER_RUN_TIME];
	for (int i = 0; i < MAX_BONES_PER_SHADER_RUN_TIME; ++i)
		nMeshToModelMap[i] = -1;
	int nBonesUsed = 0;

	int nModelToMeshMap[MAX_BONES_PER_MODEL];
	for (int i = 0; i < MAX_BONES_PER_MODEL; ++i)
		nModelToMeshMap[i] = -1;

	// Build maps, for union of two bone usages.
	// A strict monotonically increasing order of model bone index is achieved.
	{
		BONE_USAGE_VECTOR_TYPE vBones1 = sGetBoneUsage(pAnimModel, pModelDef, pMeshes, nMergeToIndex);
		BONE_USAGE_VECTOR_TYPE vBones2 = sGetBoneUsage(pAnimModel, pModelDef, pMeshes, nMergeIndex);
		BONE_USAGE_VECTOR_TYPE::size_type nSize = MAX(vBones1.size(), vBones2.size());
		vBones1.resize(nSize, false);
		vBones2.resize(nSize, false);
		for (BONE_USAGE_VECTOR_TYPE::size_type n = 0; n < nSize; ++n) {
			// Is bone used?
			if (vBones1[n] || vBones2[n]) {
				nModelToMeshMap[n] = nBonesUsed;
				nMeshToModelMap[nBonesUsed] = int(n);
				++nBonesUsed;
			}
		}
	}

	// Now re-map vertex bone indices.
	sRemapVertexBoneIndices(pAnimModel, pModelDef, pMeshes, nMergeToIndex, nModelToMeshMap);
	sRemapVertexBoneIndices(pAnimModel, pModelDef, pMeshes, nMergeIndex, nModelToMeshMap);

	// Merge vertex buffers if necessary.
	if (pMergeToMesh->nVertexBufferIndex != pMergeMesh->nVertexBufferIndex)
		sMergeVertexBuffers(pModelDef, pMeshes, pMergeToMesh->nVertexBufferIndex, pMergeMesh->nVertexBufferIndex);

	// Allocate combined index buffer.
	INDEX_BUFFER_TYPE * pwIndexBuffer = static_cast<INDEX_BUFFER_TYPE *>(MALLOC(NULL, pMergeToMesh->tIndexBufferDef.nBufferSize + pMergeMesh->tIndexBufferDef.nBufferSize));

	// Copy index buffers.
	MemoryCopy(pwIndexBuffer, (pMergeToMesh->tIndexBufferDef.nBufferSize + pMergeMesh->tIndexBufferDef.nBufferSize), pMergeToMesh->tIndexBufferDef.pLocalData, pMergeToMesh->tIndexBufferDef.nBufferSize);
	int nIBOffset = pMergeToMesh->tIndexBufferDef.nBufferSize / sizeof(INDEX_BUFFER_TYPE);
	MemoryCopy(pwIndexBuffer + nIBOffset, (pMergeToMesh->tIndexBufferDef.nBufferSize + pMergeMesh->tIndexBufferDef.nBufferSize - nIBOffset), pMergeMesh->tIndexBufferDef.pLocalData, pMergeMesh->tIndexBufferDef.nBufferSize);

	// Set new index buffer.
	FREE(NULL, pMergeMesh->tIndexBufferDef.pLocalData);
	pMergeMesh->tIndexBufferDef.pLocalData = NULL;

	FREE(NULL, pMergeToMesh->tIndexBufferDef.pLocalData);

	pMergeToMesh->tIndexBufferDef.pLocalData = pwIndexBuffer;
	pMergeToMesh->tIndexBufferDef.nBufferSize += pMergeMesh->tIndexBufferDef.nBufferSize;
	pMergeToMesh->wTriangleCount += pMergeMesh->wTriangleCount;
	pMergeToMesh->nVertexCount = pModelDef->ptVertexBufferDefs[pMergeToMesh->nVertexBufferIndex].nVertexCount;		// CHB 2006.07.05 - Fixes Savage Fiend bug on 2.0 vertex shaders (why?)

	// Set new map.
	for (int i = 0; i < MAX_BONES_PER_SHADER_RUN_TIME; ++i)
		pAnimModel->ppnBoneMappingModelToMesh[nMergeToIndex][i] = MAX(nMeshToModelMap[i], 0);

	// Remove merged mesh.
	if (nMergeIndex + 1 < pModelDef->nMeshCount) 
	{
		for (int i = nMergeIndex; i + 1 < pModelDef->nMeshCount; ++i) 
		{
			pModelDef->pnMeshIds[i] = pModelDef->pnMeshIds[i + 1];
			pMeshes[ i ] = pMeshes[ i + 1 ];
		}

		MemoryMove(pAnimModel->ppnBoneMappingModelToMesh[nMergeIndex], 
					(pModelDef->nMeshCount - nMergeIndex) * sizeof(pAnimModel->ppnBoneMappingModelToMesh[0]),
					pAnimModel->ppnBoneMappingModelToMesh[nMergeIndex + 1], 
					(pModelDef->nMeshCount - nMergeIndex - 1) * sizeof(pAnimModel->ppnBoneMappingModelToMesh[0]));
	}
	pModelDef->nMeshCount--;
	dxC_MeshFree( pMergeMesh );
	pMeshes[ pModelDef->nMeshCount ] = NULL;
	pModelDef->pnMeshIds[ pModelDef->nMeshCount ] = 0;

	return nBonesUsed;
}

static
void sAnimatedModelDefinitionMergeMeshesByBones( 
	ANIMATING_MODEL_DEFINITION* pAnimModelDef,
	D3D_MODEL_DEFINITION* pModelDef,
	D3D_MESH_DEFINITION* pMeshes[] )
{
	if ( e_IsNoRender() )
		return;

	// Check available vertex shader version to see if we should merge.
	ASSERT(e_AnimatedModelGetMaxBonesPerShader() >= MAX_BONES_PER_SHADER_ON_DISK);
	if (e_AnimatedModelGetMaxBonesPerShader() <= MAX_BONES_PER_SHADER_ON_DISK) 
	{
		// Nothing to do on low-end shaders.
		return;
	}

	// Want at least 2 meshes, otherwise uninteresting.
	if (pModelDef->nMeshCount < 2) 
	{
		return;
	}

	// Want at least MAX_BONES_PER_SHADER_ON_DISK + 1 bones, otherwise uninteresting.
	if (pAnimModelDef->nBoneCount <= MAX_BONES_PER_SHADER_ON_DISK) 
	{
		return;
	}
	
	// Work.
	for (int nOuterMesh = 0; nOuterMesh + 1 < pModelDef->nMeshCount; ++nOuterMesh) 
	{
		int nCandidateMesh = 0;
		int nCandidateMergeable = -1;
		int nCandidateFinalBones = 0;

		for (int nInnerMesh = nOuterMesh + 1; nInnerMesh < pModelDef->nMeshCount; ++nInnerMesh) {
			//
			D3D_MESH_DEFINITION* pOuterMeshDef = pMeshes[ nOuterMesh ];
			D3D_MESH_DEFINITION* pInnerMeshDef = pMeshes[ nInnerMesh ];

			// Can only merge meshes of the same material.
			if (!dx9_MeshesAreSameMaterial(pOuterMeshDef, pInnerMeshDef))
				continue;

			// Count how many bones are mergeable between the two.
			int nFinalBones = 0;
			int nMergeable = sGetMergeableAndTotalBoneCount(pAnimModelDef, 
				pModelDef, pMeshes, nOuterMesh, nInnerMesh, nFinalBones);

			// Compute resulting bone count. If it exceeds the maximum
			// bone count, it's no-go.
			ASSERT(nFinalBones > nMergeable);
			nFinalBones -= nMergeable;
			if (nFinalBones > e_AnimatedModelGetMaxBonesPerShader()) {
				continue;
			}

			// Found a possible match.
			if (nMergeable > nCandidateMergeable) {
				nCandidateMergeable = nMergeable;
				nCandidateMesh = nInnerMesh;
				nCandidateFinalBones = nFinalBones;
			}
		}

		// Was a candidate found?
		if (nCandidateMesh < 1) {
			continue;
		}

		// Found; do the merge.
		int nBonesUsed = sAnimatedModelDefinitionMergeMeshesByBones(pAnimModelDef, 
			pModelDef, pMeshes, nOuterMesh, nCandidateMesh);
		ASSERT(nBonesUsed == nCandidateFinalBones);
		--nOuterMesh;	// want to iterate this one again
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
BOOL sAnimatedModelDefinitionUpdate ( 
	int nAppearanceDefId,
	const char * pszSkeletonFilename,
	const char * pszFilename,
	BOOL bForceUpdate,
	bool bSourceOptional )
{
	char pszAnimModelFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszAnimModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFilename, ANIMATED_MODEL_FILE_EXTENSION);

	char pszGrannyMeshFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszGrannyMeshFilename, DEFAULT_FILE_WITH_PATH_SIZE, pszFilename, "gr2" );

	// check and see if we need to update at all
#ifndef FORCE_REMAKE_ANIM_MODEL_FILES
	if ( ! AppCommonAllowAssetUpdate() && ! bForceUpdate )
		return FALSE;

	if (    ! bForceUpdate
		 && ! FileNeedsUpdate( pszAnimModelFileName, pszGrannyMeshFilename, 
				ANIMATED_MODEL_FILE_MAGIC_NUMBER, ANIMATED_MODEL_FILE_VERSION, 
				FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG, 0, FALSE, 
				MIN_REQUIRED_ANIMATED_MODEL_FILE_VERSION )
		 && ! FileNeedsUpdate( pszAnimModelFileName, pszSkeletonFilename, 
				ANIMATED_MODEL_FILE_MAGIC_NUMBER, ANIMATED_MODEL_FILE_VERSION, 
				FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG, 0, FALSE,
				MIN_REQUIRED_ANIMATED_MODEL_FILE_VERSION ) )
	{
		return FALSE;
	}
#endif

	// CHB 2006.10.19 - Update was indicated, check for existence of source file.
	if (bSourceOptional)
	{
		if (::GetFileAttributesA(pszGrannyMeshFilename) == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
	}

	int rootlen;
	const OS_PATH_CHAR * rootpath = AppCommonGetRootDirectory(&rootlen);

	char pszAnimatedModelFileNameForStorage[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrRemovePath( pszAnimatedModelFileNameForStorage, DEFAULT_FILE_WITH_PATH_SIZE, rootpath, pszGrannyMeshFilename );
	PStrLower(pszAnimatedModelFileNameForStorage, DEFAULT_FILE_WITH_PATH_SIZE);


	DebugString( OP_LOW, LOADING_STRING(animated model), pszGrannyMeshFilename );

#ifdef HAVOK_ENABLED
	hkSkeleton * pHavokSkeleton = e_AppearanceDefinitionGetHavokSkeleton( nAppearanceDefId );
#endif
	MATRIX * pmInverseSkinTransform = e_AppearanceDefinitionGetInverseSkinTransform( nAppearanceDefId );

	granny_file * pGrannyFile = e_GrannyReadFile( pszGrannyMeshFilename );

	ASSERTX_RETFALSE( pGrannyFile, pszGrannyMeshFilename );

	granny_file_info * pFileInfo = GrannyGetFileInfo( pGrannyFile );
	ASSERT_RETFALSE( pFileInfo );

	// -- Coordinate System --
	// Tell Granny to transform the file into our coordinate system
	e_GrannyConvertCoordinateSystem ( pFileInfo );

	// -- The Model ---
	// Right now we assumes that there is only one model in the file.
	// If we want to handle more models, then we need to create something like a CMyGrannyModel
	if ( pFileInfo->ModelCount > 1 )
	{
		const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
		if ( pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
		{
			ASSERTV_MSG( "Too many models in %s.  Model count is %d", pszGrannyMeshFilename, pFileInfo->MaterialCount );
		}
	}

	// ---- Start saving to file ---

	TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
	FileGetFullFileName(szFullname, pszAnimModelFileName, DEFAULT_FILE_WITH_PATH_SIZE);

	ANIMATING_MODEL_DEFINITION tAnimatingModelDefinition;
	structclear( tAnimatingModelDefinition );
	D3D_MODEL_DEFINITION tModelDefinition;
	structclear( tModelDefinition );

	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION> tSerializedFileSections;
	ArrayInit(tSerializedFileSections, NULL, DEFAULT_SERIALIZED_FILE_SECTION_COUNT);
	BYTE_BUF tBuf( (MEMORYPOOL*)NULL, 1024 * 1024 ); // 1MB
	BYTE_XFER<XFER_SAVE> tXfer(&tBuf);

	// -- Model Definition --
	PStrCopy( tModelDefinition.tHeader.pszName, pszAnimatedModelFileNameForStorage, MAX_XML_STRING_LENGTH );
	tModelDefinition.dwFlags |= MODELDEF_FLAG_ANIMATED;

	granny_model* pGrannyModel = pFileInfo->Models[0]; 
	granny_skeleton* pGrannySkeleton = pGrannyModel->Skeleton;

	int	pnBoneMappingGrannyToHavok[ MAX_BONES_PER_MODEL ];
#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() )
		e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping( pGrannyModel->Skeleton, pHavokSkeleton, 
			pnBoneMappingGrannyToHavok, pszSkeletonFilename );
	else
#endif
		e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping( pGrannyModel->Skeleton, 
			pnBoneMappingGrannyToHavok, pszSkeletonFilename );

#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() )
		tAnimatingModelDefinition.nBoneCount = pHavokSkeleton ? pHavokSkeleton->m_numBones : pGrannySkeleton->BoneCount;
	else
#endif
		tAnimatingModelDefinition.nBoneCount = pGrannySkeleton->BoneCount;

#ifdef HAVOK_ENABLED
	if ( ! pHavokSkeleton || ! AppIsHellgate() )
#endif
	{ 
		// we need to store the bone names if there is no Havok skeleton which stores them
		int * pnBoneNameOffsets = (int *) MALLOCZ( NULL, sizeof( int ) * tAnimatingModelDefinition.nBoneCount );

		ASSERT_RETINVALID( tAnimatingModelDefinition.nBoneCount == pGrannySkeleton->BoneCount );
		for ( int i = 0; i < tAnimatingModelDefinition.nBoneCount; i++ )
		{
			pnBoneNameOffsets[ i ] = tAnimatingModelDefinition.nBoneNameBufferSize;
			tAnimatingModelDefinition.nBoneNameBufferSize += 1 + PStrLen( pGrannySkeleton->Bones[ i ].Name );
		}
		char * pszBoneNameBuffer = (char *) MALLOCZ( NULL, sizeof( char ) * tAnimatingModelDefinition.nBoneNameBufferSize );

		char * pszBufferCurr = pszBoneNameBuffer;
		for ( int i = 0; i < tAnimatingModelDefinition.nBoneCount; i++ )
		{
			const char * pszBoneName = pGrannySkeleton->Bones[ i ].Name;
			int nLength = PStrLen( pszBoneName );
			memcpy( pszBufferCurr, pszBoneName, nLength );
			pszBufferCurr += nLength;
			*pszBufferCurr = 0;
			pszBufferCurr++;
		}

		tAnimatingModelDefinition.pszBoneNameBuffer = pszBoneNameBuffer;
		tAnimatingModelDefinition.pnBoneNameOffsets = pnBoneNameOffsets;
		
		tAnimatingModelDefinition.nBoneMatrixCount = tAnimatingModelDefinition.nBoneCount;
		MATRIX * pmBoneMatrices = (MATRIX *) MALLOCZ( NULL, sizeof( MATRIX ) * tAnimatingModelDefinition.nBoneMatrixCount );
		for ( int i = 0; i < tAnimatingModelDefinition.nBoneMatrixCount; i++ )
		{
			MatrixInverse( & pmBoneMatrices[ i ], (MATRIX *) pGrannySkeleton->Bones[ i ].InverseWorld4x4 );
		}
		tAnimatingModelDefinition.pmBoneMatrices = pmBoneMatrices;
	} 

	// -- Meshes -- These are eventually saved by sSplitMeshByBones
	int nMeshCount = pGrannyModel->MeshBindingCount;
	ASSERTV( nMeshCount <= MAX_MESHES_PER_MODEL, "Animated granny file %s has too many meshes (%d, max is %d)!", pszGrannyMeshFilename, nMeshCount, MAX_MESHES_PER_MODEL );

	// seed bounding box
	BOOL bBoundingBoxStarted = FALSE;

	BOOL bNormalMaps       = FALSE;
	BOOL bTangentBinormals = FALSE;

	BOOL bModelHasSkin = FALSE;
	int nAnimatingMeshIndex = 0;
	for ( int nGrannyMeshIndex = 0; nGrannyMeshIndex < nMeshCount; ++nGrannyMeshIndex )
	{
		granny_mesh * pMesh = pGrannyModel->MeshBindings[ nGrannyMeshIndex ].Mesh;

		if ( pMesh->MaterialBindings == NULL )
			continue;

		// -- Index buffer - which we will split up later
		int nMeshIndexCount = GrannyGetMeshIndexCount ( pMesh );
		INDEX_BUFFER_TYPE * pwMeshIndexBuffer = NULL;
		if ( nMeshIndexCount )
		{
			pwMeshIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOCZ( NULL, nMeshIndexCount * sizeof ( INDEX_BUFFER_TYPE ) );
			GrannyCopyMeshIndices ( pMesh, sizeof ( INDEX_BUFFER_TYPE ), (BYTE *) pwMeshIndexBuffer );
		}

		BOOL bMeshNormalMap = FALSE;

		// -- Split up Granny Mesh into our meshes by Triangle Group --
		int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount( pMesh );
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pMesh );
		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{
			D3D_MESH_DEFINITION tMeshDef;
			ZeroMemory( &tMeshDef, sizeof( D3D_MESH_DEFINITION ) );

			if ( pMesh->BoneBindingCount == 1 )
			{
				GrannyFindBoneByNameLowercase( pGrannySkeleton, pMesh->BoneBindings[ 0 ].BoneName, &tMeshDef.nBoneToAttachTo );
				tMeshDef.nBoneToAttachTo = pnBoneMappingGrannyToHavok[ tMeshDef.nBoneToAttachTo ];
			}
			else
			{
				tMeshDef.nBoneToAttachTo = INVALID_ID;
			}

			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

			// Find the triangle count.
			tMeshDef.wTriangleCount = pGrannyTriangleGroup->TriCount;

			// -- Index Buffer --
			tMeshDef.tIndexBufferDef.nBufferSize = 3 * sizeof( INDEX_BUFFER_TYPE ) * tMeshDef.wTriangleCount;
			if ( tMeshDef.wTriangleCount )
			{
				tMeshDef.tIndexBufferDef.pLocalData = (void *) MALLOCZ( NULL, tMeshDef.tIndexBufferDef.nBufferSize );
				int nIndexOffset = 3 * pGrannyTriangleGroup->TriFirst;
				memcpy ( tMeshDef.tIndexBufferDef.pLocalData, pwMeshIndexBuffer + nIndexOffset, tMeshDef.tIndexBufferDef.nBufferSize );
			}

			// -- Vertex Buffer --
			D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( &tModelDefinition, &tMeshDef.nVertexBufferIndex );
			tMeshDef.ePrimitiveType = D3DPT_TRIANGLELIST;

			// generate bounding box in pre-transformed space inside this function
			V( dx9_GetAnimatedVertexBuffer ( pMesh, pVertexBufferDef, pGrannySkeleton, 
											pnBoneMappingGrannyToHavok, 
											pmInverseSkinTransform,
											&tModelDefinition.tRenderBoundingBox,
											&bBoundingBoxStarted ) );
			pVertexBufferDef->eVertexType = VERTEX_DECL_ANIMATED;

			// Now cut the vertices up and renumber indices so that we have only the vertices that we are using
			BOOL * pbIsSubsetTriangle = (BOOL *)MALLOCZ( NULL, sizeof (BOOL) * tMeshDef.wTriangleCount );
			BOOL * pbIsSubsetVertex   = (BOOL *)MALLOCZ( NULL, sizeof (BOOL) * pVertexBufferDef->nVertexCount );
			for ( int nTriangle = 0; nTriangle < tMeshDef.wTriangleCount; nTriangle++ )
			{ 
				pbIsSubsetTriangle [ nTriangle ] = TRUE;// we need this for the collapse function
				for ( int i = 0; i < 3; i++ )
				{
					ASSERT( ((INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData)[ 3 * nTriangle + i ] < pVertexBufferDef->nVertexCount );
					pbIsSubsetVertex [ ((INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData)[ 3 * nTriangle + i ] ] = TRUE;
				}
			}
			dxC_CollapseMeshByUsage ( pVertexBufferDef,
				&tMeshDef.wTriangleCount,
				&tMeshDef.tIndexBufferDef.nBufferSize,
				(INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData,
				tMeshDef.wTriangleCount,
				pbIsSubsetTriangle, 
				pbIsSubsetVertex );
			FREE ( NULL, pbIsSubsetTriangle );
			FREE ( NULL, pbIsSubsetVertex   );

			static const granny_material_texture_type peGrannyType[ NUM_TEXTURE_TYPES ] = 
			{
				GrannyDiffuseColorTexture,		//TEXTURE_DIFFUSE = 0,
				GrannyBumpHeightTexture,		//TEXTURE_NORMAL,
				GrannySelfIlluminationTexture,	//TEXTURE_LIGHTMAP,
				GrannyUnknownTextureType,		//TEXTURE_DIFFUSE2,
				GrannyUnknownTextureType,		//TEXTURE_SPECULAR,
				GrannyUnknownTextureType,		//TEXTURE_ENVMAP,
				GrannyUnknownTextureType,		//TEXTURE_HEIGHT,
			};

			granny_material * pGrannyMaterial = pMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;

			for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
			{
				tMeshDef.pnTextures[ i ] = INVALID_ID;
				if ( peGrannyType[ i ] == GrannyUnknownTextureType )
					continue;

				granny_texture * pGrannyTexture = GrannyGetMaterialTextureByType( pGrannyMaterial, peGrannyType[ i ] );
				if ( pGrannyTexture )
				{
					PStrRemoveEntirePath(tMeshDef.pszTextures[ i ], MAX_XML_STRING_LENGTH, pGrannyTexture->FromFileName);
				}
			}

			// specular
			//    this channel name is 3DS MAX-specific
			//    this needs to be specified from MAX now
			granny_texture * pGrannyTexture = e_GrannyGetTextureFromChannelName( pGrannyMaterial, "Specular Level" );
			if ( ! pGrannyTexture )
				pGrannyTexture = e_GrannyGetTextureFromChannelName( pGrannyMaterial, "Specular Color" );
			if ( pGrannyTexture )
			{
				PStrRemoveEntirePath(tMeshDef.pszTextures[TEXTURE_SPECULAR], MAX_XML_STRING_LENGTH, pGrannyTexture->FromFileName);
			}

			if ( tMeshDef.pszTextures[ TEXTURE_NORMAL ][ 0 ] != 0 )
				bNormalMaps = TRUE;

			// find out if it's a two-sided material
			granny_variant tTwoSidedVariant;
			if ( GrannyFindMatchingMember( pGrannyMaterial->ExtendedData.Type, pGrannyMaterial->ExtendedData.Object, "Two-sided", &tTwoSidedVariant ) )
			{
				ASSERT( tTwoSidedVariant.Object );
				if ( *(int*)tTwoSidedVariant.Object != 0 )
					tMeshDef.dwFlags |= MESH_FLAG_TWO_SIDED;
			}

			if ( tMeshDef.pszTextures[ TEXTURE_NORMAL ][ 0 ] != 0 )
			{
				// check for tangent/binormals
				const int nCheckVerts = 10;
				D3DCOLOR vColZero = 0x007f7f7f;  // biased (0,0,0) normal
				D3DXVECTOR3 vVecZero( 0,0,0 );
				size_t nVertexSize = dxC_GetVertexSize( 2, pVertexBufferDef );
				VS_ANIMATED_INPUT_STREAM_2* pVertex = sGetAnimatedVertex<VS_ANIMATED_INPUT_STREAM_2, 2>( pVertexBufferDef, 0, nVertexSize );
				int nMaxVerts = pVertexBufferDef->nVertexCount > RAND_MAX ? RAND_MAX : pVertexBufferDef->nVertexCount;
				int i;
				for ( i = 0; i < nCheckVerts; i++ )
				{
					int nVertex = rand() % nMaxVerts;
					if ( pVertex[ nVertex ].Binormal == vColZero && pVertex[ nVertex ].Tangent == vVecZero )
						break;
				}
				if ( i == nCheckVerts )
				{
					bTangentBinormals = TRUE;
				}
			}

			// figure out if this mesh is skinned or whether it just uses a single bone
			if ( sMeshIsSkinned( &tModelDefinition, &tMeshDef ) )
			{
				tMeshDef.dwFlags |= MESH_FLAG_SKINNED;
				bModelHasSkin = TRUE;
			}
			if ( AppIsTugboat() )
				tMeshDef.dwFlags |= MESH_FLAG_NOCOLLISION;

			dx9_AnimatedModelDefinitionUpdateSkinnedBoneMask ( &tAnimatingModelDefinition, pVertexBufferDef );

			sSplitMeshesByBones( &tModelDefinition, &tMeshDef, &tAnimatingModelDefinition,
				&tSerializedFileSections, &tXfer );
		}

		FREE ( NULL, pwMeshIndexBuffer );
	}

	if ( bNormalMaps && !bTangentBinormals )
	{
		const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
		if ( pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
		{
			ASSERTV_MSG( "Granny file %s has normal maps, but is missing tangent/binormals!", pszGrannyMeshFilename );
		}
	}

	//tAnimatingModelDefinition.nMeshCount = tModelDefinition.nMeshCount;

	tModelDefinition.tBoundingBox = tModelDefinition.tRenderBoundingBox;
	if ( bModelHasSkin )
	{
		tModelDefinition.tBoundingBox.vMin.fX -= BOUNDING_BOX_BUFFER;
		tModelDefinition.tBoundingBox.vMin.fY -= BOUNDING_BOX_BUFFER;
		tModelDefinition.tBoundingBox.vMin.fZ -= BOUNDING_BOX_BUFFER;
		tModelDefinition.tBoundingBox.vMax.fX += BOUNDING_BOX_BUFFER;
		tModelDefinition.tBoundingBox.vMax.fY += BOUNDING_BOX_BUFFER;
		tModelDefinition.tBoundingBox.vMax.fZ += BOUNDING_BOX_BUFFER;
	}

	V( dxC_ModelDefinitionWriteVerticesBuffers( tXfer, tSerializedFileSections, tModelDefinition ) );

	{
		SERIALIZED_FILE_SECTION* pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_ANIMATING_MODEL_DEFINITION;
		pSection->dwOffset = tXfer.GetCursor();
		dxC_AnimatingModelDefinitionXfer( tXfer, tAnimatingModelDefinition );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		pSection = ArrayAdd(tSerializedFileSections);
		pSection->nSectionType = MODEL_FILE_SECTION_MODEL_DEFINITION;
		pSection->dwOffset = tXfer.GetCursor();
		dxC_ModelDefinitionXfer( tXfer, tModelDefinition, ANIMATED_MODEL_FILE_VERSION, TRUE );
		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		HANDLE hFile = CreateFile( szFullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == INVALID_HANDLE_VALUE )
		{
			if ( DataFileCheck( szFullname ) )
				hFile = CreateFile( szFullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		}
		ASSERTX_RETFALSE( hFile != INVALID_HANDLE_VALUE, szFullname );

		BYTE_BUF tHeaderBuf( (MEMORYPOOL*)NULL, sizeof( SERIALIZED_FILE_HEADER ) + 
			sizeof( SERIALIZED_FILE_SECTION ) * DEFAULT_SERIALIZED_FILE_SECTION_COUNT );
		BYTE_XFER<XFER_SAVE> tHeaderXfer(&tHeaderBuf);

		// SERIALIZED_FILE_HEADER inherits from FILE_HEADER
		SERIALIZED_FILE_HEADER tFileHeader;
		tFileHeader.dwMagicNumber	= ANIMATED_MODEL_FILE_MAGIC_NUMBER;
		tFileHeader.nVersion		= ANIMATED_MODEL_FILE_VERSION;
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
		WriteFile( hFile, tHeaderXfer.GetBufferPointer(), nHeaderLen, &dwBytesWritten, NULL );

		// Now, write out the file data
		UINT nDataLen = tXfer.GetLen();
		tXfer.SetCursor( 0 );
		WriteFile( hFile, tXfer.GetBufferPointer(), nDataLen, &dwBytesWritten, NULL );

		CloseHandle( hFile );
	}

	FREE( NULL, tAnimatingModelDefinition.pnBoneNameOffsets );
	FREE( NULL, tAnimatingModelDefinition.pszBoneNameBuffer );
	FREE( NULL, tAnimatingModelDefinition.pmBoneMatrices );

	GrannyFreeFile( pGrannyFile );

	return TRUE;
}

BOOL e_AnimatedModelDefinitionUpdate ( 
	int nAppearanceDefId,
	const char * pszHavokSkeletonFilename,
	const char * pszFilename,
	BOOL bForceUpdate )
{
	bool bParentUpdated = !!sAnimatedModelDefinitionUpdate(nAppearanceDefId, pszHavokSkeletonFilename, pszFilename, bForceUpdate, false);

	char LowFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	e_ModelDefinitionGetLODFileName(LowFileName, ARRAYSIZE(LowFileName), pszFilename);

	bool bLODUpdated = !!sAnimatedModelDefinitionUpdate(nAppearanceDefId, pszHavokSkeletonFilename, LowFileName, bForceUpdate, true);

	return bParentUpdated || bLODUpdated;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct ANIMATED_MODEL_CALLBACK_DATA
{
	int nModelDef;
	int nLOD;
	int nAppearanceDef;
	BOOL bShouldContainSkeletonInfo;
	unsigned nFlags;
	char szFolder[ MAX_PATH ];
};

//----------------------------------------------------------------------------

static
void sAnimatedModelDefinitionLoad(
	int nModelDef,
	int nLOD,
	int nAppearanceDef,
	const char * pszGrannyMeshFilename,
	unsigned nFlags );

static PRESULT sAnimatedModelFileLoadCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC* spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETFAIL(spec);

	ANIMATED_MODEL_CALLBACK_DATA* pCallbackData = (ANIMATED_MODEL_CALLBACK_DATA*)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);
	void *pData = spec->buffer;

	int nLOD = pCallbackData->nLOD;
	unsigned nFlags = 0;
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pCallbackData->nModelDef );
	if ( pHash )
		CLEAR_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING );

	D3D_MODEL_DEFINITION* pModelDef = NULL;
	ANIMATING_MODEL_DEFINITION* pAnimModelDefinition = NULL;
	SERIALIZED_FILE_HEADER* pFileHeader = NULL;

	if ( ( data.m_IOSize == 0 ) || !pData ) 
	{
		if ( pHash )
			SET_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOAD_FAILED );

		if ( nLOD == LOD_LOW )
		{
			nFlags |= MDNFF_FLAG_FORCE_ADD_TO_DEFAULT_PAK;
			goto _loadhigh;
		}
		else
			goto _cleanup;
	}
	else
	{
		BYTE* pbFileStart = (BYTE*)pData;

		ASSERTX_RETFAIL( pbFileStart, spec->filename );
		ASSERTX_RETFAIL( spec->bytesread >= sizeof(FILE_HEADER), spec->filename );

		pFileHeader = MALLOC_TYPE( SERIALIZED_FILE_HEADER, NULL, 1 );
		ASSERTX_GOTO( pFileHeader, spec->filename, _cleanup );
		BYTE_BUF tHeaderBuf( pbFileStart, spec->bytesread );
		BYTE_XFER<XFER_LOAD> tHeaderXfer(&tHeaderBuf);
		dxC_SerializedFileHeaderXfer( tHeaderXfer, *pFileHeader );

		ASSERTX_RETFAIL( pFileHeader->dwMagicNumber == ANIMATED_MODEL_FILE_MAGIC_NUMBER, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nVersion >= MIN_REQUIRED_ANIMATED_MODEL_FILE_VERSION, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nVersion <= ANIMATED_MODEL_FILE_VERSION, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nNumSections >= 2, spec->filename );

		pFileHeader->pSections = MALLOC_TYPE( SERIALIZED_FILE_SECTION, NULL, 
			pFileHeader->nNumSections );
		ASSERTX_GOTO( pFileHeader->pSections, spec->filename, _cleanup );

		for ( int nSection = 0; nSection < pFileHeader->nNumSections; nSection++ )
			dxC_SerializedFileSectionXfer( tHeaderXfer, pFileHeader->pSections[ nSection ], 
				pFileHeader->nSFHVersion );

		BYTE* pbFileDataStart = pbFileStart + tHeaderXfer.GetCursor();
		BYTE_BUF tFileBuf( pbFileDataStart, spec->bytesread - (DWORD)(pbFileDataStart - pbFileStart) );
		BYTE_XFER<XFER_LOAD> tXfer(&tFileBuf);

		// The animating model definition should be stored one prior to the end.
		SERIALIZED_FILE_SECTION* pSection = &pFileHeader->pSections[ pFileHeader->nNumSections - 2 ];
		ASSERTX_RETFAIL( pSection->nSectionType == MODEL_FILE_SECTION_ANIMATING_MODEL_DEFINITION, spec->filename );
		tXfer.SetCursor( pSection->dwOffset );

		pAnimModelDefinition = MALLOCZ_TYPE( ANIMATING_MODEL_DEFINITION, NULL, 1 );
		ASSERTX_GOTO( pAnimModelDefinition, spec->filename, _cleanup );
		V_GOTO( _cleanup, dxC_AnimatingModelDefinitionXfer( tXfer, *pAnimModelDefinition ) );

		if (  ( pCallbackData->bShouldContainSkeletonInfo && !pAnimModelDefinition->pszBoneNameBuffer ) 
		  || ( !pCallbackData->bShouldContainSkeletonInfo &&  pAnimModelDefinition->pszBoneNameBuffer ) )
		{
			// we saved a binary file which should/shouldn't have skeleton information.  Update it again.
			sFreeAnimatedModelDef( pAnimModelDefinition );

			e_AnimatedModelDefinitionUpdate ( pCallbackData->nAppearanceDef, NULL, spec->filename, TRUE );

			// make async load call
			DECLARE_LOAD_SPEC(newspec, spec->filename);
			newspec.pakEnum = spec->pakEnum;
			newspec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FORCE_FROM_DISK 
				| PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
			newspec.fpLoadingThreadCallback = sAnimatedModelFileLoadCallback;
			newspec.callbackData = (ANIMATED_MODEL_CALLBACK_DATA*)MALLOC( NULL, sizeof( ANIMATED_MODEL_CALLBACK_DATA ) );
			ASSERT_RETFAIL( newspec.callbackData );
			CopyMemory( newspec.callbackData, pCallbackData, sizeof( ANIMATED_MODEL_CALLBACK_DATA ) );
			CLEAR_MASK(spec->flags, PAKFILE_LOAD_FREE_CALLBACKDATA);	// don't free the user data
			newspec.priority = ASYNC_PRIORITY_ANIMATED_MODELS;
			PakFileLoad(newspec);
			return S_OK;
		}
	
		D3D_MODEL_DEFINITION* pModelDef = MALLOCZ_TYPE( D3D_MODEL_DEFINITION, NULL, 1 );
		ASSERTX_GOTO( pModelDef, spec->filename, _cleanup );
		pModelDef->tHeader.nId = pCallbackData->nModelDef;

		D3D_MESH_DEFINITION* pMeshDefs[ MAX_MESHES_PER_MODEL ];
		structclear( pMeshDefs );

		V_GOTO( _cleanup, dxC_ModelDefinitionXferFileSections( tXfer, *pModelDef, 
			pMeshDefs, pFileHeader->pSections, pFileHeader->nNumSections, 
			pFileHeader->nVersion, TRUE ) );

		FREE( NULL, pFileHeader->pSections );
		FREE( NULL, pFileHeader );
		pFileHeader = NULL;

		// CHB 2006.06.26
		sAnimatedModelDefinitionMergeMeshesByBones( pAnimModelDefinition, pModelDef, pMeshDefs );

		bool bShouldLoadTextures = (pCallbackData->nFlags & MDNFF_FLAG_MODEL_SHOULD_LOAD_TEXTURES) != 0;

		DWORD dwFlags = 0;
		if (bShouldLoadTextures)
		{
			dwFlags |= MODEL_DEFINITION_CREATE_FLAG_SHOULD_LOAD_TEXTURES;
		}

		V_GOTO( _cleanup, dx9_ModelDefinitionFromFileInMemory( NULL, pCallbackData->nModelDef, 
			nLOD, spec->fixedfilename, pCallbackData->szFolder, pModelDef, 
			pMeshDefs, dwFlags, 0.0f ) );

		ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGet( gpAnimatingModelDefinitions, pCallbackData->nModelDef );
		if ( !pHash )
			pHash = HashAdd( gpAnimatingModelDefinitions, pCallbackData->nModelDef );
		pHash->pDef[ pCallbackData->nLOD ] = pAnimModelDefinition;
		pHash->nAppearanceDef = pCallbackData->nAppearanceDef;
		pHash->bShouldLoadTextures = bShouldLoadTextures;

		e_AnimatedModelDefinitionJustLoaded( pCallbackData->nAppearanceDef, 
			pCallbackData->nModelDef );
	}

	// Fall through and load the high LOD if this LOD was the low LOD.

	if ((pCallbackData->nFlags & MDNFF_FLAG_DONT_LOAD_NEXT_LOD) == 0)
	{
		if ( e_ModelBudgetGetMaxLOD( MODEL_GROUP_UNITS ) > LOD_LOW )
		{
_loadhigh:
			// Is this the first LOD? If so, then load the next one.
			if ( nLOD == LOD_LOW )
			{
				char szPath[DEFAULT_FILE_WITH_PATH_SIZE];
				V_RETURN( e_ModelDefinitionStripLODFileName( szPath, ARRAYSIZE( szPath ), spec->filename ) );
				sAnimatedModelDefinitionLoad( pCallbackData->nModelDef, LOD_HIGH, 
					pCallbackData->nAppearanceDef, szPath, nFlags | (pCallbackData->nFlags & MDNFF_FLAG_MODEL_SHOULD_LOAD_TEXTURES) );
			}
		}
	}

	return S_OK;

_cleanup:
	if ( pFileHeader )
	{
		FREE( NULL, pFileHeader->pSections );
		FREE( NULL, pFileHeader );
	}
	FREE( NULL, pAnimModelDefinition );
	FREE( NULL, pModelDef );

	return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
void sAnimatedModelDefinitionLoad(
	int nModelDef,
	int nLOD,
	int nAppearanceDef,
	const char* pszGrannyMeshFilename,
	unsigned nFlags )
{
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	if ( pHash && TEST_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING ) )
		return;

	char pszAnimModelFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszAnimModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszGrannyMeshFilename, ANIMATED_MODEL_FILE_EXTENSION);

	ANIMATED_MODEL_CALLBACK_DATA* pCallbackData = (ANIMATED_MODEL_CALLBACK_DATA*)MALLOC( NULL, sizeof( ANIMATED_MODEL_CALLBACK_DATA ) );
	pCallbackData->nModelDef = nModelDef;
	pCallbackData->nLOD = nLOD;
	pCallbackData->nAppearanceDef = nAppearanceDef;
	pCallbackData->bShouldContainSkeletonInfo = TRUE;
#ifdef HAVOK_ENABLED
	pCallbackData->bShouldContainSkeletonInfo = e_AppearanceDefinitionGetHavokSkeleton( nAppearanceDef ) == NULL;
#endif
	pCallbackData->nFlags = nFlags;
	PStrGetPath( pCallbackData->szFolder, ARRAYSIZE( pCallbackData->szFolder ), pszAnimModelFileName );

	DECLARE_LOAD_SPEC( spec, pszAnimModelFileName );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER 
		| PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;

	if ( ( (nFlags & MDNFF_FLAG_FORCE_ADD_TO_DEFAULT_PAK) == 0 ) && ( nLOD == LOD_HIGH ) )
		spec.pakEnum = PAK_GRAPHICS_HIGH;
	
	if ( (nFlags & MDNFF_FLAG_SOURCE_FILE_OPTIONAL) != 0 )
		spec.flags |= PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;

	spec.fpLoadingThreadCallback = sAnimatedModelFileLoadCallback;
	spec.callbackData = pCallbackData;
	spec.priority = ASYNC_PRIORITY_ANIMATED_MODELS;

	if ( pHash )
	{
		CLEAR_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOAD_FAILED );
		SET_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING );
	}

	if (c_GetToolMode() || AppCommonIsAnyFillpak())
	{
		PakFileLoadNow(spec);
	}
	else
	{
		PakFileLoad(spec);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
unsigned sGetAnimatedModelDefinitionNewFlags(void)
{
#if !ISVERSION(SERVER_VERSION)
	switch (e_OptionState_GetActive()->get_nAnimatedLoadForceLOD())
	{
		case LOD_LOW:
			return MDNFF_ALLOW_LOW_LOD | MDNFF_IGNORE_HIGH_LOD;
		// No need to force high today.
		//case LOD_HIGH:
		//	return MDNFF_IGNORE_LOW_LOD | MDNFF_ALLOW_HIGH_LOD;
		default:
			break;
	}
#endif
	return MDNFF_DEFAULT;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_AnimatedModelDefinitionLoad(
	int nAppearanceDefId,
	const char * pszGrannyMeshFilename,
	BOOL bModelShouldLoadTextures )
{
	int nModelDef = dx9_ModelDefinitionNew( INVALID_ID );
	unsigned nRequestFlags = 0;
	if (bModelShouldLoadTextures)
	{
		nRequestFlags |= MDNFF_FLAG_MODEL_SHOULD_LOAD_TEXTURES;
	}

	if ( AppIsTugboat() )
		sAnimatedModelDefinitionLoad( nModelDef, LOD_HIGH, nAppearanceDefId, 
			pszGrannyMeshFilename, nRequestFlags );
	else
	{
		// CHB 2007.06.29 - When filling the pak file or using
		// Hammer, always process all LODs.
		unsigned nFlags = AppCommonGetForceLoadAllLODs() ? MDNFF_DEFAULT : sGetAnimatedModelDefinitionNewFlags();

		if (nModelDef != INVALID_ID)
		{
			MODEL_DEFINITION_HASH * pModelDefHash = dxC_ModelDefinitionGetHash( nModelDef );
			if (pModelDefHash != 0)
			{
				if ((nFlags & MDNFF_IGNORE_LOW_LOD) != 0)
				{
					pModelDefHash->dwLODFlags[LOD_LOW] |= MODELDEF_HASH_FLAG_IGNORE;
				}
				if ((nFlags & MDNFF_IGNORE_HIGH_LOD) != 0)
				{
					pModelDefHash->dwLODFlags[LOD_HIGH] |= MODELDEF_HASH_FLAG_IGNORE;
				}
			}
		}

		//
		if (nFlags != 0)
		{
			nRequestFlags |= MDNFF_FLAG_DONT_LOAD_NEXT_LOD;
		}

		// Load the low LOD first, then the high.
		char szLowFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
		e_ModelDefinitionGetLODFileName( szLowFileName, ARRAYSIZE(szLowFileName), pszGrannyMeshFilename );
		sAnimatedModelDefinitionLoad( nModelDef, LOD_LOW, nAppearanceDefId, 
			szLowFileName, nRequestFlags | MDNFF_FLAG_SOURCE_FILE_OPTIONAL );
	}

	return nModelDef;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_AnimatedModelDefinitionReload( int nAnimModelDef, int nLOD )
{
	ASSERT_RETFAIL( nLOD == LOD_HIGH ); // We only support reloading of high LODs.
	
	MODEL_DEFINITION_HASH* pModelDefHash = dxC_ModelDefinitionGetHash( nAnimModelDef );
	if ( !pModelDefHash )
		return S_FALSE;

	if ( TEST_MASK( pModelDefHash->dwLODFlags[ nLOD ], 
		MODELDEF_HASH_FLAG_LOAD_FAILED | MODELDEF_HASH_FLAG_LOADING | MODELDEF_HASH_FLAG_IGNORE ) )
		return S_FALSE;

	// Check if it has already been loaded asynchronously.
	if ( pModelDefHash->pDef[ nLOD ] )
		return S_FALSE;

	D3D_MODEL_DEFINITION* pModelDef = pModelDefHash->pDef[ LOD_LOW ];

	char szPath[DEFAULT_FILE_WITH_PATH_SIZE];
	V_RETURN( e_ModelDefinitionStripLODFileName( szPath, ARRAYSIZE( szPath ), pModelDef->tHeader.pszName ) );
	char szFullFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	FileGetFullFileName( szFullFileName, szPath, DEFAULT_FILE_WITH_PATH_SIZE );
	trace( "REAPER: Reloading '%s'\n", szFullFileName );

	ANIMATING_MODEL_DEFINITION_HASH* pHash = HashGet( gpAnimatingModelDefinitions, nAnimModelDef );
	sAnimatedModelDefinitionLoad( nAnimModelDef, nLOD, pHash->nAppearanceDef, 
		szFullFileName, pHash->bShouldLoadTextures ? MDNFF_FLAG_MODEL_SHOULD_LOAD_TEXTURES : 0 );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping (
	granny_skeleton * pGrannySkeleton,
	int * pnBoneMapping,
	const char * pszFileName)
{
	ASSERT_RETURN( pnBoneMapping );
	{
		for ( int i = 0; i < MAX_BONES_PER_MODEL; i++ )
			pnBoneMapping[ i ] = i;
		return;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping (
	granny_skeleton * pGrannySkeleton,
	class hkSkeleton * pHavokSkeleton,
	int * pnBoneMapping,
	const char * pszFileName)
{
#ifdef HAVOK_ENABLED
	ASSERT_RETURN( pnBoneMapping );
	if ( ! pGrannySkeleton || ! pHavokSkeleton )
	{
		for ( int i = 0; i < MAX_BONES_PER_MODEL; i++ )
			pnBoneMapping[ i ] = i;
		return;
	}
	int nBoneCountGranny = pGrannySkeleton->BoneCount;
	int nBoneCountMayhem = pHavokSkeleton->m_numBones;
	ASSERT( nBoneCountGranny <= nBoneCountMayhem );
	ASSERT_RETURN( nBoneCountGranny <= MAX_BONES_PER_MODEL );

	for ( int i = 0; i < nBoneCountGranny; i++ )
	{
		pnBoneMapping[ i ] = INVALID_ID;
		for ( int j = 0; j < nBoneCountMayhem; j++ )
		{
			if ( _strcmpi ( pHavokSkeleton->m_bones[ j ]->m_name, pGrannySkeleton->Bones[ i ].Name ) == 0 )
			{ 
				pnBoneMapping[ i ] = j;
				break;
			}
		}
#ifdef _DEBUG
		if ( pnBoneMapping[ i ] == INVALID_ID )
		{
			char pszString[ 256 ];
			PStrPrintf( pszString, 256, "Cannot find a bone in Mayhem skeleton called %s - which is in %s.", 
					pGrannySkeleton->Bones[ i ].Name,
					pszFileName );
			WARNX( pnBoneMapping[ i ] != INVALID_ID, pszString );
		}
#endif // _DEBUG
	}
#endif // HAVOK_ENABLED
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DWORD* e_AnimatedModelDefinitionGetBoneIsSkinnedFlags( 
	int nModelDef )
{
	// Bones should be the same across all LODs, so grab any LOD definition.
	ANIMATING_MODEL_DEFINITION* pAnimatingModelDefinition = sGetAnyAnimatedModelDefinition( nModelDef );
	return pAnimatingModelDefinition ? pAnimatingModelDefinition->dwBoneIsSkinned : NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_AnimatedModelDefinitionGetBoneCount(
	int nModelDef )
{
	ANIMATING_MODEL_DEFINITION* pAnimatingModelDefinition = NULL;
	// Bones should be the same across all LODs, so grab any LOD definition.
	pAnimatingModelDefinition = sGetAnyAnimatedModelDefinition( nModelDef );
	return pAnimatingModelDefinition ? pAnimatingModelDefinition->nBoneCount : 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_AnimatedModelDefinitionsSwap( 
	int nFirst, 
	int nSecond )
{
	e_ModelDefinitionsSwap( nFirst, nSecond );

	ANIMATING_MODEL_DEFINITION_HASH * pFirstHash  = HashGet( gpAnimatingModelDefinitions, nFirst );
	ANIMATING_MODEL_DEFINITION_HASH * pSecondHash = HashGet( gpAnimatingModelDefinitions, nSecond );

	if ( !pFirstHash || !pSecondHash )
	{
		return;
	}

	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		ANIMATING_MODEL_DEFINITION* pModelDefSwap = pFirstHash->pDef[ nLOD ];
		pFirstHash ->pDef[ nLOD ] = pSecondHash->pDef[ nLOD ];
		pSecondHash->pDef[ nLOD ] = pModelDefSwap;
	}
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const char * e_AnimatedModelDefinitionGetBoneName( 
	int nModelDef,
	int nBoneId )
{
	// Bones should be the same across all LODs, so grab any LOD definition.
	ANIMATING_MODEL_DEFINITION* pDef = sGetAnyAnimatedModelDefinition( nModelDef );
	if ( ! pDef )
		return NULL;

	if ( nBoneId < 0 || nBoneId >= pDef->nBoneCount )
		return NULL;

	if( !pDef->pnBoneNameOffsets )
		return NULL;

	return pDef->pszBoneNameBuffer + pDef->pnBoneNameOffsets[ nBoneId ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_AnimatedModelDefinitionGetBoneNumber( 
	int nModelDef,
	const char * pszName )
{
	// Bones should be the same across all LODs, so grab any LOD definition.
	ANIMATING_MODEL_DEFINITION* pDef = sGetAnyAnimatedModelDefinition( nModelDef );
	if ( ! pDef )
		return INVALID_ID;

	int nNameLength = PStrLen( pszName );

	if ( ! pDef->pnBoneNameOffsets )
		return INVALID_ID;

	for ( int i = 0; i < pDef->nBoneCount; i++ )
	{
		char * pszBoneName = pDef->pszBoneNameBuffer + pDef->pnBoneNameOffsets[ i ];
		if ( PStrICmp( pszBoneName, pszName, nNameLength ) == 0 )
			return i;
	}
	return INVALID_ID;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.10.16
EFFECT_TARGET dxC_AnimatedModelGetMaximumEffectTargetForModel11Animation(void)
{
	return AppIsHellgate() ? FXTGT_SM_11 : FXTGT_SM_20_LOW;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_AnimatedModelGetMaxBonesPerShader()
{
	EFFECT_TARGET eTgt = dxC_GetCurrentMaxEffectTarget();
	return (eTgt <= dxC_AnimatedModelGetMaximumEffectTargetForModel11Animation()) ? _MAX_BONES_PER_SHADER_11 : _MAX_BONES_PER_SHADER_20;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ANIMATING_MODEL_DEFINITION * dx9_AnimatedModelDefinitionGet(
	int nModelDef, 
	int nLOD )
{
	return sGetAnimatedModelDefinition( nModelDef, nLOD );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const MATRIX * e_AnimatedModelDefinitionGetBoneRestPoses( 
	int nModelDef )
{
	// Bones should be the same across all LODs, so grab any LOD definition.888
	ANIMATING_MODEL_DEFINITION* pDef = sGetAnyAnimatedModelDefinition( nModelDef );
	if ( ! pDef )
		return NULL;
	return pDef->pmBoneMatrices;
}
