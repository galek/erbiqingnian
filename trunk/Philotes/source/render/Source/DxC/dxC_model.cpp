//----------------------------------------------------------------------------
// dxC_model.cpp
//
// - Implementation for model functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_query.h"
#include "dxC_occlusion.h"
#include "dxC_model.h"
#include "dxC_caps.h"
#include "dxC_material.h"
#include "dxC_texture.h"
#include "dxC_macros.h"
#include "dxC_buffer.h"
#include "dxC_state.h"
#include "dxC_irradiance.h"
#include "dxC_shadow.h"
#include "dxC_anim_model.h"
#include "dxC_dpvs.h"
#include "dxC_profile.h"
#include "datatables.h"
#include "filepaths.h"
#include "pakfile.h"
#include "event_data.h"
#include "perfhier.h"
#include "e_frustum.h"
#include "performance.h"
#include "e_lod.h"			// CHB 2007.03.19
#include "asyncfile.h"
#include "e_optionstate.h"
#include "e_dpvs.h"
#include "dxC_obscurance.h"
#include "e_region.h"
#include "prime.h"
#include "dxC_pixo.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DEBUG_MESH_DISTANCE_TO_CAMERA 300.0f

#define MODEL_LOD_TRANSITION_MUL		1.5f

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CHash<D3D_MODEL>				gtModels;
int								gnModelIdNext = 0;
CHash<MESH_DEFINITION_HASH>		gtMeshes;
int								gnMeshDefinitionNextId = 0;
CHash<MODEL_DEFINITION_HASH>	gtModelDefinitions;
int								gnModelDefinitionNextId = 0;
int								gnIsolateModelID = INVALID_ID;

#ifdef MODEL_BOXMAP
extern BoxMap					gtModelProxMap;
#else
extern PointMap					gtModelProxMap;
#endif // MODEL_BOXMAP

extern RANGE2					gtModelCullDistances[NUM_MODEL_DISTANCE_CULL_TYPES];
extern RANGE2					gtModelCullDistancesTugboat[NUM_MODEL_DISTANCE_CULL_TYPES];
extern RANGE2					gtModelCanopyCullDistances[NUM_MODEL_DISTANCE_CULL_TYPES];
extern BOOL						gbModelCullDistanceOverride;
extern RANGE2					gtModelCullDistanceOverride;
extern RANGE2					gtModelCullScreensizes[NUM_MODEL_DISTANCE_CULL_TYPES];
extern RANGE2					gtModelCullScreensizesTugboat[NUM_MODEL_DISTANCE_CULL_TYPES];


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

static PRESULT sModelDefinitionRelease( D3D_MODEL_DEFINITION * pModelDef );
static PRESULT sMeshComputeTechniqueMaskOverride( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef, int nMeshIndex, int nLOD );	// CHB 2006.11.28

static PRESULT sModelSetPendingTextureOverride( int nLOD, D3D_MODEL * pModel, TEXTURE_TYPE eType, int nTexture );
static PRESULT sModelResolvePendingTextureOverrides( D3D_MODEL * pModel );
static PRESULT sModelSetTextureOverride( int nLOD, int nModelDef, int nModelId, int nMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride);

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void dxC_MeshGetFolders( 
	D3D_MESH_DEFINITION * pMeshDefinition, 
	int nLOD,
	char * szFolder, 
	OVERRIDE_PATH *pOverridePath,
	const char * pszFolderIn, 
	int nStrSize, 
	BOOL bBackground )
{ 
	if (pMeshDefinition->dwFlags & (MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL))
	{
		PStrCopy( szFolder, FILE_PATH_BACKGROUND_DEBUG, nStrSize );
	} 
	else 
	{
		PStrCopy( szFolder, pszFolderIn, nStrSize );
	}
	if ( bBackground )
	{
		OverridePathByCode( pMeshDefinition->dwTexturePathOverride, pOverridePath, nLOD );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sIndexBufferXfer(
	BYTE_XFER<mode> & tXfer,
	D3D_INDEX_BUFFER_DEFINITION & tIndexBufferDef)
{
	BYTE_XFER_ELEMENT( tXfer, tIndexBufferDef.nBufferSize );
	BYTE_XFER_ELEMENT( tXfer, tIndexBufferDef.nIndexCount );
	BYTE_XFER_ELEMENT( tXfer, tIndexBufferDef.tUsage );
	BYTE_XFER_ELEMENT( tXfer, tIndexBufferDef.tFormat );

	if ( tXfer.IsLoad() )
	{
		tIndexBufferDef.pLocalData = MALLOC( NULL, tIndexBufferDef.nBufferSize );
		tIndexBufferDef.nD3DBufferID = INVALID_ID;
	}

	BYTE_XFER_POINTER( tXfer, tIndexBufferDef.pLocalData, tIndexBufferDef.nBufferSize );
	
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_BoundingBoxXfer(
	BYTE_XFER<mode> & tXfer,
	BOUNDING_BOX& tBoundingBox)
{
	BYTE_XFER_ELEMENT( tXfer, tBoundingBox.vMin );
	BYTE_XFER_ELEMENT( tXfer, tBoundingBox.vMax );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_MeshDefinitionXfer(
	BYTE_XFER<mode> & tXfer,
	D3D_MESH_DEFINITION & tMeshDef )
{
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.dwFlags );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.dwTechniqueFlags );

	BYTE_XFER_ELEMENT( tXfer, tMeshDef.ePrimitiveType );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.eVertType );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nMaterialId );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nMaterialVersion );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nBoneToAttachTo );

	sIndexBufferXfer( tXfer, tMeshDef.tIndexBufferDef );
	
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nVertexBufferIndex );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nVertexStart );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.wTriangleCount );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nVertexCount );

	BYTE_XFER_ELEMENT( tXfer, tMeshDef.dwTexturePathOverride );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nWardrobeTextureIndex );

	for ( int nTextureType = 0; nTextureType < NUM_TEXTURE_TYPES; nTextureType++ )
	{
		BYTE_XFER_STRING( tXfer, tMeshDef.pszTextures[ nTextureType ], MAX_XML_STRING_LENGTH );
	}
	
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nDiffuse2AddressMode[ 0 ] );
	BYTE_XFER_ELEMENT( tXfer, tMeshDef.nDiffuse2AddressMode[ 1 ] );

	for ( int nTexCoordChannel = 0; nTexCoordChannel < TEXCOORDS_MAX_CHANNELS; nTexCoordChannel++ )
		BYTE_XFER_ELEMENT( tXfer, tMeshDef.pfUVsPerMeter[ nTexCoordChannel ] );

	dxC_BoundingBoxXfer( tXfer, tMeshDef.tRenderBoundingBox );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_MeshWriteToBuffer ( 
	BYTE_XFER<mode> & tXfer,
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>& tSerializedFileSections,
	D3D_MODEL_DEFINITION & tModelDef,
	D3D_MESH_DEFINITION & tMeshDef )
{
	ASSERT_RETFAIL( tModelDef.ptVertexBufferDefs );
	ASSERT_RETFAIL( tMeshDef.nVertexBufferIndex < tModelDef.nVertexBufferDefCount );
	ASSERT_RETFAIL( tMeshDef.nVertexBufferIndex >= 0 );
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &tModelDef.ptVertexBufferDefs[ tMeshDef.nVertexBufferIndex ];
	tMeshDef.eVertType = pVertexBufferDef->eVertexType;

	SERIALIZED_FILE_SECTION* pSection = ArrayAdd( tSerializedFileSections );
	pSection->nSectionType = MODEL_FILE_SECTION_MESH_DEFINITION;
	pSection->dwOffset = tXfer.GetCursor();
	dxC_MeshDefinitionXfer( tXfer, tMeshDef );
	pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

	FREE( NULL, tMeshDef.tIndexBufferDef.pLocalData );
	tMeshDef.tIndexBufferDef.pLocalData = NULL;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_SerializedFileSectionXfer( 
	BYTE_XFER<mode> & tXfer,
	SERIALIZED_FILE_SECTION& tSection,
	int nSFHVersion )
{
	// Check for current version
	switch ( nSFHVersion )
	{
	// Current version
	case SERIALIZED_FILE_HEADER_VERSION:
		BYTE_XFER_ELEMENT( tXfer, tSection );
		break;

	// we can handle prior versions of the SFH
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_SerializedFileHeaderXfer( 	
	BYTE_XFER<mode> & tXfer,
	SERIALIZED_FILE_HEADER & tFileHeader)
{
	BYTE_XFER_ELEMENT( tXfer, tFileHeader.dwMagicNumber );
	BYTE_XFER_ELEMENT( tXfer, tFileHeader.nVersion );
	BYTE_XFER_ELEMENT( tXfer, tFileHeader.nSFHVersion );
	BYTE_XFER_ELEMENT( tXfer, tFileHeader.nNumSections );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_ModelDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	D3D_MODEL_DEFINITION & tModelDef,
	int nVersion,
	BOOL bIsAnimatedModel )
{
	BYTE_XFER_ELEMENT( tXfer, tModelDef.tHeader.dwFlags );
	BYTE_XFER_STRING( tXfer, tModelDef.tHeader.pszName, MAX_XML_STRING_LENGTH );

	BYTE_XFER_ELEMENT( tXfer, tModelDef.dwFlags );

	BYTE_XFER_ELEMENT( tXfer, tModelDef.nMeshCount );
	BYTE_XFER_ELEMENT( tXfer, tModelDef.nMeshesPerBatchMax );
	dxC_BoundingBoxXfer( tXfer, tModelDef.tBoundingBox );
	dxC_BoundingBoxXfer( tXfer, tModelDef.tRenderBoundingBox );

	BYTE_XFER_ELEMENT( tXfer, tModelDef.nAttachmentCount );
	BYTE_XFER_ELEMENT( tXfer, tModelDef.nAdObjectDefCount );

	BYTE_XFER_ELEMENT( tXfer, tModelDef.nVertexBufferDefCount );	

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_SharedModelDefinitionXfer(
	BYTE_XFER<mode> & tXfer,
	MLI_DEFINITION & tSharedModelDef )
{
	BYTE_XFER_ELEMENT( tXfer, tSharedModelDef.nStaticLightCount );
	BYTE_XFER_ELEMENT( tXfer, tSharedModelDef.nSpecularLightCount );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_OccluderDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	OCCLUDER_DEFINITION & tOccluderDef)
{
	BYTE_XFER_ELEMENT( tXfer, tOccluderDef.nIndexCount );

	if ( tXfer.IsLoad() )
		tOccluderDef.pIndices = MALLOC_TYPE( INDEX_BUFFER_TYPE, NULL, tOccluderDef.nIndexCount );

	BYTE_XFER_POINTER( tXfer, tOccluderDef.pIndices, 
		sizeof( INDEX_BUFFER_TYPE ) * tOccluderDef.nIndexCount );

	BYTE_XFER_ELEMENT( tXfer, tOccluderDef.nVertexCount );

	if ( tXfer.IsLoad() )
		tOccluderDef.pVertices = MALLOC_TYPE( VECTOR, NULL, tOccluderDef.nVertexCount );

	BYTE_XFER_POINTER( tXfer, tOccluderDef.pVertices, 
		sizeof( VECTOR ) * tOccluderDef.nVertexCount );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_AttachmentDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	ATTACHMENT_DEFINITION & tAttachmentDef)
{
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.eType );
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.dwFlags );

	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.nVolume );

	BYTE_XFER_STRING( tXfer, tAttachmentDef.pszAttached, MAX_XML_STRING_LENGTH );
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.nAttachedDefId );

	BYTE_XFER_STRING( tXfer, tAttachmentDef.pszBone, MAX_XML_STRING_LENGTH );
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.nBoneId );

	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.vPosition );
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.vNormal );
	BYTE_XFER_ELEMENT( tXfer, tAttachmentDef.fRotation );
	
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_LightDefinitionXfer( 
	BYTE_XFER<mode> & tXfer, 
	STATIC_LIGHT_DEFINITION & tLightDef)
{
	BYTE_XFER_ELEMENT( tXfer, tLightDef.dwFlags );
	BYTE_XFER_ELEMENT( tXfer, tLightDef.tColor );
	BYTE_XFER_ELEMENT( tXfer, tLightDef.fIntensity );
	BYTE_XFER_ELEMENT( tXfer, tLightDef.tFalloff );
	BYTE_XFER_ELEMENT( tXfer, tLightDef.fPriority );
	BYTE_XFER_ELEMENT( tXfer, tLightDef.vPosition );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_AdObjectDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	AD_OBJECT_DEFINITION& tAdObjectDef,
	int nVersion,
	BOOL bIsAnimatedModel )
{
	BYTE_XFER_STRING( tXfer, tAdObjectDef.szName, MAX_ADOBJECT_NAME_LEN );
	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.nIndex );
	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.nMeshIndex );
	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.eResType );

	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.dwFlags );

	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.vCenter_Obj );
	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.vNormal_Obj );
	BYTE_XFER_ELEMENT( tXfer, tAdObjectDef.tAABB_Obj );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_SharedModelDefinitionXferFileSections( 
	BYTE_XFER<mode> & tXfer,
	MLI_DEFINITION & tSharedModelDef,
	SERIALIZED_FILE_SECTION* pSections, 
	int nNumSections,
	int nVersion )
{
	// AE 2007.06.07: Version 3 introduced shared light definitions and an MLI
	//   definition section.
	if ( nVersion >= 3 )
	{
		// The model definition should be stored at the end.
		SERIALIZED_FILE_SECTION* pSection = &pSections[ nNumSections - 1 ];
		ASSERT_RETFAIL( pSection->nSectionType == MODEL_FILE_SECTION_MLI_DEFINITION );
		tXfer.SetCursor( pSection->dwOffset );

		dxC_SharedModelDefinitionXfer( tXfer, tSharedModelDef );

		if ( tSharedModelDef.nStaticLightCount > 0 )
		{
			if ( tSharedModelDef.pStaticLightDefinitions )
				FREE( NULL, tSharedModelDef.pStaticLightDefinitions );

			tSharedModelDef.pStaticLightDefinitions = MALLOCZ_TYPE( STATIC_LIGHT_DEFINITION,
				NULL, tSharedModelDef.nStaticLightCount );
		}

		if ( tSharedModelDef.nSpecularLightCount > 0 )
		{
			if ( tSharedModelDef.pSpecularLightDefinitions )
				FREE( NULL, tSharedModelDef.pSpecularLightDefinitions );

			tSharedModelDef.pSpecularLightDefinitions = MALLOCZ_TYPE( STATIC_LIGHT_DEFINITION, 
				NULL, tSharedModelDef.nSpecularLightCount );
		}
	}

	for ( int nSection = 0; nSection < nNumSections; nSection++ )
	{
		SERIALIZED_FILE_SECTION* pSection = &pSections[ nSection ];
		tXfer.SetCursor( pSection->dwOffset );
		switch ( pSection->nSectionType )
		{
		case MODEL_FILE_SECTION_STATIC_LIGHTS:
			for ( int nStaticLightDefsXferred = 0; 
					  nStaticLightDefsXferred < tSharedModelDef.nStaticLightCount; 
					  nStaticLightDefsXferred++ )
			{
				dxC_LightDefinitionXfer( tXfer, 
					tSharedModelDef.pStaticLightDefinitions[ nStaticLightDefsXferred ] );
			}
			break;

		case MODEL_FILE_SECTION_SPECULAR_LIGHTS:
			for ( int nSpecularLightDefsXferred = 0; 
					  nSpecularLightDefsXferred < tSharedModelDef.nSpecularLightCount; 
					  nSpecularLightDefsXferred++ )
			{
				dxC_LightDefinitionXfer( tXfer, 
					tSharedModelDef.pSpecularLightDefinitions[ nSpecularLightDefsXferred ] );
			}
			break;

		case MODEL_FILE_SECTION_OCCLUDER_DEFINITION:
			{
				if ( tSharedModelDef.pOccluderDef )
					FREE( NULL, tSharedModelDef.pOccluderDef );

				tSharedModelDef.pOccluderDef = MALLOCZ_TYPE( OCCLUDER_DEFINITION, NULL, 1 );
				ASSERT_RETFAIL( tSharedModelDef.pOccluderDef );
				V_RETURN( dxC_OccluderDefinitionXfer( tXfer, *tSharedModelDef.pOccluderDef ) );
			}
			break;
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_ModelDefinitionXferFileSections( 
	BYTE_XFER<mode> & tXfer,
	D3D_MODEL_DEFINITION & tModelDef,
	D3D_MESH_DEFINITION* pMeshDefs[],
	SERIALIZED_FILE_SECTION* pSections, 
	int nNumSections,
	int nVersion,
	BOOL bIsAnimatedModel )
{
	// The model definition should be stored at the end.
	SERIALIZED_FILE_SECTION* pSection = &pSections[ nNumSections - 1 ];
	ASSERT_RETFAIL( pSection->nSectionType == MODEL_FILE_SECTION_MODEL_DEFINITION );
	tXfer.SetCursor( pSection->dwOffset );

	dxC_ModelDefinitionXfer( tXfer, tModelDef, nVersion, bIsAnimatedModel );

	MLI_DEFINITION * pSharedModelDef = dxC_SharedModelDefinitionGet( tModelDef.tHeader.nId );

	int nVertexBufferDefsXferred = 0;
	if ( tModelDef.nVertexBufferDefCount > 0 )
	{
		tModelDef.ptVertexBufferDefs = (D3D_VERTEX_BUFFER_DEFINITION *)MALLOC(NULL, sizeof(D3D_VERTEX_BUFFER_DEFINITION) * tModelDef.nVertexBufferDefCount);
	}

	int nMeshDefsXferred = 0;
	if ( tModelDef.nMeshCount > 0 )
		for ( int nMesh = 0; nMesh < tModelDef.nMeshCount; nMesh++ )
			pMeshDefs[ nMesh ] = MALLOCZ_TYPE( D3D_MESH_DEFINITION, NULL, 1 );
	
	BOOL bProcessLights = pSharedModelDef 
					   && !pSharedModelDef->pStaticLightDefinitions
					   && !pSharedModelDef->pSpecularLightDefinitions;
	if ( bProcessLights )
	{
		if ( pSharedModelDef->nStaticLightCount > 0 )
			pSharedModelDef->pStaticLightDefinitions = MALLOCZ_TYPE( STATIC_LIGHT_DEFINITION,
				NULL, pSharedModelDef->nStaticLightCount );

		if ( pSharedModelDef->nSpecularLightCount > 0 )
			pSharedModelDef->pSpecularLightDefinitions = MALLOCZ_TYPE( STATIC_LIGHT_DEFINITION, 
				NULL, pSharedModelDef->nSpecularLightCount );
	}

	int nAttachmentDefsXferred = 0;
	if ( tModelDef.nAttachmentCount > 0 )
		tModelDef.pAttachments = MALLOCZ_TYPE( ATTACHMENT_DEFINITION, NULL,
			tModelDef.nAttachmentCount );

	int nAdObjectDefsXferred = 0;
	if ( tModelDef.nAdObjectDefCount > 0 )
		tModelDef.pAdObjectDefs = MALLOCZ_TYPE( AD_OBJECT_DEFINITION, NULL,
			tModelDef.nAdObjectDefCount );

	for ( int nSection = 0; nSection < nNumSections; nSection++ )
	{
		SERIALIZED_FILE_SECTION* pSection = &pSections[ nSection ];
		tXfer.SetCursor( pSection->dwOffset );
		switch ( pSection->nSectionType )
		{
		case MODEL_FILE_SECTION_STATIC_LIGHTS:
			if ( bProcessLights )
			{
				for ( int nStaticLightDefsXferred = 0; 
					nStaticLightDefsXferred < pSharedModelDef->nStaticLightCount; 
					nStaticLightDefsXferred++ )
				{
					dxC_LightDefinitionXfer( tXfer, 
						pSharedModelDef->pStaticLightDefinitions[ nStaticLightDefsXferred ] );
				}
			}
			break;

		case MODEL_FILE_SECTION_SPECULAR_LIGHTS:
			if ( bProcessLights )
			{
				for ( int nSpecularLightDefsXferred = 0; 
						  nSpecularLightDefsXferred < pSharedModelDef->nSpecularLightCount; 
						  nSpecularLightDefsXferred++ )
				{
					dxC_LightDefinitionXfer( tXfer, 
						pSharedModelDef->pSpecularLightDefinitions[ nSpecularLightDefsXferred ] );
				}
			}
			break;

		case MODEL_FILE_SECTION_MESH_DEFINITION:
			{
				dxC_MeshDefinitionXfer( tXfer, *pMeshDefs[ nMeshDefsXferred ] );
				nMeshDefsXferred++;
			}
			break;

		case MODEL_FILE_SECTION_ATTACHMENT_DEFINITION:
			{
				dxC_AttachmentDefinitionXfer( tXfer, 
					tModelDef.pAttachments[ nAttachmentDefsXferred ] );
				nAttachmentDefsXferred++;
			}
			break;

		case MODEL_FILE_SECTION_AD_OBJECT_DEFINITION:
			{
				dxC_AdObjectDefinitionXfer( tXfer, 
					tModelDef.pAdObjectDefs[ nAdObjectDefsXferred ], nVersion, bIsAnimatedModel );
				nAdObjectDefsXferred++;
			}
			break;

		case MODEL_FILE_SECTION_VERTEX_BUFFER_DEFINITION:
			{
				int nStreamCount = 0;
				dxC_VertexBufferDefinitionXfer( tXfer, 
					tModelDef.ptVertexBufferDefs[ nVertexBufferDefsXferred ],
					nStreamCount );
				nVertexBufferDefsXferred++;
			}
			break;			
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3D_VERTEX_BUFFER_DEFINITION * dxC_ModelDefinitionAddVertexBufferDef( D3D_MODEL_DEFINITION *pModelDefinition, int * pnIndex )
{
	int nVertexBufferIndex = pModelDefinition->nVertexBufferDefCount;
	pModelDefinition->nVertexBufferDefCount++;
	pModelDefinition->ptVertexBufferDefs = (D3D_VERTEX_BUFFER_DEFINITION *)REALLOC(NULL, pModelDefinition->ptVertexBufferDefs, sizeof( D3D_VERTEX_BUFFER_DEFINITION ) * pModelDefinition->nVertexBufferDefCount);
	if ( ! pModelDefinition->ptVertexBufferDefs )
		pModelDefinition->nVertexBufferDefCount = 0;
	ASSERT_RETNULL( pModelDefinition->ptVertexBufferDefs );
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ nVertexBufferIndex ];
	dxC_VertexBufferDefinitionInit( *pVertexBufferDef );

	if ( pnIndex )
		*pnIndex = nVertexBufferIndex;
	return pVertexBufferDef;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3DCOLOR dxC_VectorToColor ( 
	const D3DXVECTOR3 & vInVector )
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
template <XFER_MODE mode>
PRESULT dxC_VertexBufferDefinitionXfer(
	BYTE_XFER<mode> & tXfer,
	D3D_VERTEX_BUFFER_DEFINITION& tVertexBufferDef,
	int& nStreamCount )
{
	if ( tXfer.IsLoad() )
		dxC_VertexBufferDefinitionInit( tVertexBufferDef );

	BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.dwFlags );
	BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.nVertexCount );
	BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.eVertexType );
	BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.tUsage );
	BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.dwFVF );

	BYTE_XFER_ELEMENT( tXfer, nStreamCount );

	for ( int nVertexStream = 0; nVertexStream < nStreamCount; nVertexStream++ )
	{
		BYTE_XFER_ELEMENT( tXfer, tVertexBufferDef.nBufferSize[ nVertexStream ] );

		if ( tXfer.IsLoad() )
			tVertexBufferDef.pLocalData[ nVertexStream ] = MALLOC( NULL, 
				tVertexBufferDef.nBufferSize[ nVertexStream ] );

		BYTE_XFER_POINTER( tXfer, tVertexBufferDef.pLocalData[ nVertexStream ], tVertexBufferDef.nBufferSize[ nVertexStream ] );
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_ModelDefinitionWriteVerticesBuffers( 
	class BYTE_XFER<mode> & tXfer,
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>& tSerializedFileSections,
	D3D_MODEL_DEFINITION & tModelDefinition )
{
	D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDefs = tModelDefinition.ptVertexBufferDefs;

	// save the VBs themselves
	for ( int i = 0; i < tModelDefinition.nVertexBufferDefCount; i++ )
	{
		D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef = &pVertexBufferDefs[ i ];

		SERIALIZED_FILE_SECTION* pSection = ArrayAdd( tSerializedFileSections );
		pSection->nSectionType = MODEL_FILE_SECTION_VERTEX_BUFFER_DEFINITION;			
		pSection->dwOffset = tXfer.GetCursor();

		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		dxC_VertexBufferDefinitionXfer( tXfer, *pVertexBufferDef, nStreamCount );

		pSection->dwSize = tXfer.GetCursor() - pSection->dwOffset;

		for ( int i = 0; i < nStreamCount; i++ )
			FREE ( NULL, pVertexBufferDef->pLocalData[ i ] );
	}

	FREE ( NULL, pVertexBufferDefs );
	tModelDefinition.ptVertexBufferDefs = NULL;

	return S_OK;
}

PRESULT e_ModelsInit()
{
	HashInit( gtModelDefinitions, NULL, 64 );
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	HashInit( gtMeshes, g_StaticAllocator, 1024 );
	HashInit( gtModels, g_StaticAllocator, 256 );
#else
	HashInit( gtMeshes, NULL, 1024 );
	HashInit( gtModels, NULL, 256 );
#endif	
	

	e_ModelProxmapInit();

	return S_OK;
}

PRESULT e_ModelsDestroy()
{
	// free all models
	D3D_MODEL * pModel = HashGetFirst( gtModels );
	BOUNDED_WHILE ( pModel, 1000000 )
	{
		V( e_ModelRemovePrivate( pModel->nId ) );
		D3D_MODEL * pNextModel = HashGetFirst( gtModels );
		ASSERT_BREAK( pNextModel != pModel );
		pModel = pNextModel;
	}
	HashFree(gtModels);

	// free all meshes
	MESH_DEFINITION_HASH * pMeshHash = HashGetFirst( gtMeshes ); 
	BOUNDED_WHILE( pMeshHash, 1000000 )
	{
		D3D_MESH_DEFINITION * pMesh = pMeshHash->pDef;
		V( dxC_MeshFree( pMesh ) );
		HashRemove( gtMeshes, pMeshHash->nId );
		MESH_DEFINITION_HASH * pNextHash = HashGetFirst( gtMeshes ); 
		ASSERT_BREAK( pNextHash != pMeshHash );
		pMeshHash = pNextHash;
	}
	HashFree( gtMeshes );

	// free all model definitions
	MODEL_DEFINITION_HASH * pModelDefHash = HashGetFirst( gtModelDefinitions );
	BOUNDED_WHILE( pModelDefHash, 1000000 )
	{
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			e_ModelDefinitionDestroy( pModelDefHash->nId, nLOD, MODEL_DEFINITION_DESTROY_DEFINITION | MODEL_DEFINITION_DESTROY_VBUFFERS, TRUE );
		}
		MODEL_DEFINITION_HASH * pNextHash = HashGetFirst( gtModelDefinitions );
		ASSERT_BREAK( pNextHash != pModelDefHash );
		pModelDefHash = pNextHash;
	}
	HashFree( gtModelDefinitions );

	e_ModelProxmapDestroy();

	return S_OK;
}




//-------------------------------------------------------------------------------------------------
// D3D_MODEL
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL dxC_ModelInFrustum( D3D_MODEL * pModel, const PLANE * pPlanes )
{
	ASSERT_RETFALSE(pPlanes);
	ASSERT_RETFALSE(pModel);

	if ( e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) == FALSE )
		return TRUE;

	V_DO_FAILED( dxC_ModelLazyUpdate( pModel ) )
	{
		return FALSE;
	}

	VECTOR * pPoints = dxC_GetModelRenderOBBInWorld( pModel, NULL );
	if ( ! pPoints )
		return FALSE;

	return e_OBBInFrustum( pPoints, pPlanes );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_ModelInFrustum( int nModelID, const PLANE * pPlanes )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	return dxC_ModelInFrustum( pModel, pPlanes );
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_MeshInFrustum( int nModelID, int nMesh, const PLANE * pPlanes )
{
	ASSERT_RETFALSE(pPlanes);

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	ASSERT_RETFALSE(pModel);

	if ( e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) == FALSE )
		return TRUE;

	VECTOR* pPoints = e_GetMeshRenderOBBInWorld( nModelID, LOD_HIGH, nMesh );;
	if ( !pPoints )
		return FALSE;

	return e_OBBInFrustum( pPoints, pPlanes );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelProjectedInFrustum( int nModelID, int nLOD, int nMeshIndex, const PLANE * pPlanes, const VECTOR& vDir, BOOL bIntersectionsAllowed /*= TRUE*/ )
{
#if ISVERSION( SERVER_VERSION )
	return FALSE;
#else
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	return dxC_ModelProjectedInFrustum( pModel, nLOD, nMeshIndex, pPlanes, vDir, bIntersectionsAllowed );
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL dxC_ModelProjectedInFrustum( D3D_MODEL * pModel, int nLOD, int nMeshIndex, const PLANE * pPlanes, const VECTOR& vDir, BOOL bIntersectionsAllowed /*= TRUE*/ )
{
#if ISVERSION( SERVER_VERSION )
	return FALSE;
#else
	ASSERT_RETFALSE(pPlanes);
	ASSERT_RETFALSE(pModel);

	if ( e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) == FALSE )
		return TRUE;

	VECTOR* pPoints = NULL;
	if ( nMeshIndex != INVALID_ID )
		pPoints = dx9_GetMeshRenderOBBInWorld( pModel, nLOD, nMeshIndex );
	
	if ( !pPoints )
		pPoints = dxC_GetModelRenderOBBInWorld( pModel, NULL );

	if ( !pPoints )
		return FALSE;

	VECTOR vTopCenter = ( pPoints[ 4 ] +
						  pPoints[ 5 ] +
						  pPoints[ 6 ] +
						  pPoints[ 7 ] )
						  * 0.25f;		
	
	VECTOR vDirNorm;
	VectorNormalize( vDirNorm, vDir );
	float fProjectedDist = fabs( ( vTopCenter.fZ - pPoints[ 0 ].fZ ) / VectorDot( vDirNorm, VECTOR( 0, 0, -1 ) ) );
	VECTOR vProjected = vTopCenter + vDirNorm * fProjectedDist;

#if ISVERSION( DEBUG_VERSION )
	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	if ( nIsolateModel != INVALID_ID && nIsolateModel == pModel->nId )
	{
		V( e_PrimitiveAddRect( 0, &pPoints[ 4 ], &pPoints[ 5 ], &pPoints[ 6 ], &pPoints[ 7 ] ) );
		V( e_PrimitiveAddLine( 0, &vTopCenter, &vProjected ) );
	}
#endif

	if ( bIntersectionsAllowed // Only allow cheap test if intersections are allowed.
	  && e_PointInFrustum( &vProjected, pPlanes ) )
		return TRUE;

	BOOL bInFrustum = FALSE;
#if ISVERSION( DEBUG_VERSION )
	VECTOR vProjectedPoints[4];
#endif
	for ( int i = 0; i < 4; i++ )
	{
		VECTOR vPoint = vProjected - vTopCenter + pPoints[ 4 + i ];
#if ISVERSION( DEBUG_VERSION )
		vProjectedPoints[ i ] = vPoint;
#endif

		if( e_PointInFrustum( &vPoint, pPlanes ) )
		{
			bInFrustum = TRUE;

			if ( bIntersectionsAllowed )
				break;
		}
		else
		{
			bInFrustum = FALSE;

			if ( !bIntersectionsAllowed )
				break;
		}
	}

#if ISVERSION( DEBUG_VERSION )
	if ( nIsolateModel != INVALID_ID && nIsolateModel == pModel->nId )
	{
		V( e_PrimitiveAddRect( 0, &vProjectedPoints[ 0 ], &vProjectedPoints[ 1 ], &vProjectedPoints[ 2 ], &vProjectedPoints[ 3 ] ) );
	}
#endif

	return bInFrustum;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ModelSetOccluded( D3D_MODEL * pModel, BOOL bOccluded )
{
	TRACE_MODEL_INT( pModel->nId, "Model Set Occluded", bOccluded );

	return dx9_SetOccluded( &pModel->tOcclusion, bOccluded, pModel->nId );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL dx9_ModelGetOccluded( D3D_MODEL * pModel )
{
	return dx9_GetOccluded( &pModel->tOcclusion );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelGetOccluded( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETFALSE(pModel);

	return dx9_ModelGetOccluded( pModel );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL dx9_ModelGetInFrustum( D3D_MODEL * pModel )
{
	return pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelGetInFrustum( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETFALSE(pModel);

	return dx9_ModelGetInFrustum( pModel );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sUpdateBoundingBoxPosition( D3D_MODEL * pModel, BOUNDING_BOX * pDestBBox, VECTOR * pvNewPos )
{
	ASSERT_RETINVALIDARG( pModel && pDestBBox && pvNewPos );

	VECTOR vNewPos;
	// transform vPos by invScaledWorld -- get the new world pos and model world pos into object space
	D3DXVec3TransformCoord( (D3DXVECTOR3*)&vNewPos, (D3DXVECTOR3*)pvNewPos, (D3DXMATRIXA16*)&pModel->matScaledWorldInverse );
	VECTOR vModPos = *(VECTOR*)&pModel->vPosition;
	D3DXVec3TransformCoord( (D3DXVECTOR3*)&vModPos, (D3DXVECTOR3*)&vModPos, (D3DXMATRIXA16*)&pModel->matScaledWorldInverse );

	// subtract out the current world translation
	VECTOR vDiff = vNewPos - vModPos;

	// something to replace the current position; use it as the new center of the bounding box
	VECTOR vScale = pDestBBox->vMax - pDestBBox->vMin;
	vDiff = vDiff * pModel->vScale;
	vDiff -= vScale * 0.5f;
	pDestBBox->vMin = vDiff;
	pDestBBox->vMax = vDiff + vScale;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_GetModelRenderAABBCenterInWorld( int nModelID, VECTOR & vCenter )
{
	VECTOR vObjectCenter;
	PRESULT pr;
	V_SAVE( pr, e_GetModelRenderAABBCenterInObject( nModelID, vObjectCenter ) );
	if ( FAILED( pr ) )
		return pr;
	if ( S_OK != pr )
		return S_FALSE;
	MatrixMultiply( &vCenter, &vObjectCenter, e_ModelGetWorldScaled( nModelID ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_GetModelRenderAABBCenterInObject( int nModelID, VECTOR & vCenter )
{
	BOUNDING_BOX tAABB;
	PRESULT pr;
	V_SAVE( pr, e_GetModelRenderAABBInObject( nModelID, &tAABB ) );
	if ( FAILED( pr ) )
		return pr;
	if ( S_OK != pr )
		return S_FALSE;
	vCenter = ( tAABB.vMin + tAABB.vMax ) * 0.5f;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_GetModelRenderAABBInObject( D3D_MODEL * pModel, BOUNDING_BOX * pBBox, BOOL bAnyLOD /*= TRUE*/ )
{
	ASSERT_RETINVALIDARG( pBBox );
	ASSERT_RETINVALIDARG( pModel );

	if ( pModel->nModelDefinitionId == INVALID_ID )
		return S_FALSE;

	int nLOD;
	if ( bAnyLOD )
		nLOD = dxC_ModelGetFirstLOD( pModel );
	else
		nLOD = dxC_ModelGetDisplayLOD( pModel );
	D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return S_FALSE;

	*pBBox = pModelDef->tRenderBoundingBox;

	if ( AppIsHellgate() )
	{
		VECTOR vRagPos;
		BOOL bRagPos = e_ModelGetRagdollPosition( pModel->nId, vRagPos );
		if ( bRagPos )
		{
			sUpdateBoundingBoxPosition( pModel, pBBox, &vRagPos );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_GetModelRenderAABBInObject( int nModelID, BOUNDING_BOX * pBBox, BOOL bAnyLOD /*= TRUE*/ )
{
	ASSERT_RETINVALIDARG( nModelID != INVALID_ID );
	D3D_MODEL* pModel = HashGet( gtModels, nModelID );
	return dxC_GetModelRenderAABBInObject( pModel, pBBox, bAnyLOD );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_GetModelRenderAABBInWorld( const D3D_MODEL * pModel, BOUNDING_BOX * pBBox )
{
	ASSERT_RETINVALIDARG(pModel);
	ASSERT_RETINVALIDARG(pBBox);
	*pBBox = pModel->tRenderAABBWorld;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_GetModelRenderAABBInWorld( int nModelID, BOUNDING_BOX * pBBox )
{
    const D3D_MODEL* pModel = HashGet( gtModels, nModelID );
    return dxC_GetModelRenderAABBInWorld( pModel, pBBox );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR * dxC_GetModelRenderOBBInWorld( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef )
{
	ASSERT_RETNULL(pModel);

	if ( AppIsTugboat() )
    {
		return (VECTOR*)pModel->tRenderOBBWorld;
    }

	VECTOR vRagPos;
	BOOL bRagPos = e_ModelGetRagdollPosition( pModel->nId, vRagPos );
	if ( bRagPos )
	{
		if ( ! pModelDef )
			pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, dxC_ModelGetDisplayLOD( pModel ) );
		if ( ! pModelDef )
			return NULL;
		BOUNDING_BOX tBBox = pModelDef->tRenderBoundingBox;
		V( sUpdateBoundingBoxPosition( pModel, &tBBox, &vRagPos ) );
		TransformAABBToOBB( pModel->tRenderOBBWorld, (MATRIX*)&pModel->matScaledWorld, &tBBox );
	}

	return (VECTOR*)pModel->tRenderOBBWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR * e_GetModelRenderOBBInWorld( int nModelID )
{
	D3D_MODEL* pModel = HashGet( gtModels, nModelID );
	if ( ! pModel )
		return NULL;
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, dxC_ModelGetDisplayLOD( pModel ) );
	return dxC_GetModelRenderOBBInWorld( pModel, pModelDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetModelRenderOBBCenterInWorld( int nModelID, VECTOR & vCenter )
{
	D3D_MODEL* pModel = HashGet( gtModels, nModelID );
	ASSERT_RETINVALIDARG(pModel);

	VECTOR vRagPos;
	BOOL bRagPos = FALSE;

	if ( AppIsHellgate() )
	{
		bRagPos = e_ModelGetRagdollPosition( nModelID, vRagPos );
	}

	if ( bRagPos )
	{
		D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
		if ( ! pModelDef )
		{
			vCenter = VECTOR(0,0,0);
			return S_FALSE;
		}
		BOUNDING_BOX tBBox = pModelDef->tRenderBoundingBox;
		V( sUpdateBoundingBoxPosition( pModel, &tBBox, &vRagPos ) );
		//TransformAABBToOBB( pModel->tRenderOBBWorld, (MATRIX*)&pModel->matScaledWorld, &tBBox );
		vCenter = VECTOR( tBBox.vMin.fX, tBBox.vMin.fY, tBBox.vMin.fZ ) +
				  VECTOR( tBBox.vMax.fX, tBBox.vMax.fY, tBBox.vMax.fZ );
		vCenter *= 0.5f;
		MatrixMultiplyCoord( &vCenter, &vCenter, (MATRIX*)&pModel->matScaledWorld );
	} else
	{
		// if the OBB positions get too huge, this could lose too much precision -- would be better to pre-weight, then add, in that case
		vCenter =
			( pModel->tRenderOBBWorld[ 0 ] +
			pModel->tRenderOBBWorld[ 1 ] +
			pModel->tRenderOBBWorld[ 2 ] +
			pModel->tRenderOBBWorld[ 3 ] +
			pModel->tRenderOBBWorld[ 4 ] +
			pModel->tRenderOBBWorld[ 5 ] +
			pModel->tRenderOBBWorld[ 6 ] +
			pModel->tRenderOBBWorld[ 7 ] )
			* 0.125f;
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_ModelGetTextureOverride( int nModelID, int nMesh, TEXTURE_TYPE eType, int nLOD )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALID( pModel );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return INVALID_ID;
	return dx9_ModelGetTextureOverride( pModel, pModelDef, nMesh, eType, nLOD );	// CHB 2006.11.28
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_ModelGetTextureOverride( D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, int nMesh, TEXTURE_TYPE eType, int nLOD )
{
	ASSERT_RETINVALID( pModel );
	ASSERT_RETINVALID( (0 <= nLOD) && (nLOD < LOD_COUNT) );
	if ( !pModel->tModelDefData[nLOD].pnTextureOverrides )
		return INVALID_ID;
	if ( pModel->tModelDefData[nLOD].dwFlags & MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE )
	{
		sModelResolvePendingTextureOverrides( pModel );
		if ( pModel->tModelDefData[nLOD].dwFlags & MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE )
			return INVALID_ID;
	}
	ASSERT_RETINVALID( nMesh < pModelDef->nMeshCount );
	ASSERT_RETINVALID( eType < NUM_TEXTURE_TYPES );

	int nIndex = nMesh * NUM_TEXTURE_TYPES + eType;
	ASSERT( nIndex < pModel->tModelDefData[nLOD].nTextureOverridesAllocated );
	if ( nIndex >= pModel->tModelDefData[nLOD].nTextureOverridesAllocated )
		nIndex = eType;
	ASSERT_RETINVALID ( nIndex < pModel->tModelDefData[nLOD].nTextureOverridesAllocated );
	ASSERT_RETINVALID ( nIndex >= 0 );

	return pModel->tModelDefData[nLOD].pnTextureOverrides[ nIndex ];	// CHB 2006.12.08, 2006.11.28
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DWORD dx9_ModelGetMeshTechniqueMaskOverride( const D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, int nMesh, int nLOD )
{
	ASSERT_RETZERO( pModel );
	if ( !pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks )
		return INVALID_ID;
	ASSERT_RETZERO( nMesh < pModelDef->nMeshCount );
	return pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMesh ];	// CHB 2006.12.08, 2006.11.28
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_ModelGetTexture( int nModelID, int nMesh, TEXTURE_TYPE eType, int nLOD, BOOL bNoOverride )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALID( pModel );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return INVALID_ID;
	return dx9_ModelGetTexture( pModel, pModelDef, NULL, nMesh, eType, nLOD );	// CHB 2006.11.28
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_ModelGetTexture(
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	TEXTURE_TYPE eType,
	int nLOD,
	BOOL bNoOverride,
	const MATERIAL * pMaterial )
{
	ASSERT_RETINVALID( eType < NUM_TEXTURE_TYPES );
	if ( !pMesh )
		pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
	if ( !pMesh )
		return INVALID_ID;
	int nTextureId = INVALID_ID;
	// try the material override first
	if( pMaterial )
		nTextureId = pMaterial->nOverrideTextureID[ eType ];
	if ( nTextureId == INVALID_ID && ! bNoOverride )
		nTextureId = dx9_ModelGetTextureOverride( pModel, pModelDef, nMesh, eType, nLOD );	// CHB 2006.11.28
	if ( nTextureId == INVALID_ID )
		nTextureId = pMesh->pnTextures[ eType ];
	return nTextureId;	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
bool dx9_ModelHasSpecularMap(D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, const D3D_MESH_DEFINITION * pMesh, int nMesh, const MATERIAL * pMaterial, int nLOD)
{
	return INVALID_ID != dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_SPECULAR, nLOD, FALSE, pMaterial );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_ModelIsVisible( int nModelId ) 
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	return dx9_ModelIsVisible( pModel );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetDefinition( 
	int nModelId, 
	int nModelDefinitionId,
	int nLOD )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_FAIL;
	}

	BOOL bNewModelDef = FALSE;
	if ( pModel->nModelDefinitionId != nModelDefinitionId )
	{
		if ( pModel->nModelDefinitionId != INVALID_ID )
		{
			e_ModelDefinitionReleaseRef( pModel->nModelDefinitionId, TRUE );

			if ( pModel->pnAdObjectInstances )
			{
				for ( int i = 0; i < pModel->nAdObjectInstances; ++i )
				{
					V( e_AdInstanceFreeAd( pModel->pnAdObjectInstances[i] ) );
				}
			}
		}
		pModel->nModelDefinitionId = nModelDefinitionId;
		if ( pModel->nModelDefinitionId != INVALID_ID )
			e_ModelDefinitionAddRef( pModel->nModelDefinitionId );
		DPVS_SAFE_RELEASE( pModel->pCullObject );

		bNewModelDef = TRUE;
	}

	ASSERT_RETFAIL( nModelDefinitionId == INVALID_ID || HashGet( gtModelDefinitions, nModelDefinitionId ) );

	// create animating model if necessary
	pModel->nModelDefinitionId = nModelDefinitionId;
	if ( nModelDefinitionId == INVALID_ID )
		return S_FALSE;

	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDefinition )
	{
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED, TRUE );
		return S_FALSE;
	} else {
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED, FALSE );
	}

	V( sModelResolvePendingTextureOverrides( pModel ) );

	if ( pModelDefinition->dwFlags & MODELDEF_FLAG_ANIMATED )
	{
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED, TRUE );
	} 

	if ( pModelDefinition->dwFlags & MODELDEF_FLAG_CAST_DYNAMIC_SHADOW )
	{
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW, TRUE );
		if( pModel->nProxmapId == INVALID_ID )
		{
			V( dx9_ModelAddToProxMap( pModel ) );
		}
	}

	if ( pModelDefinition->dwFlags & MODELDEF_FLAG_IS_PROP )
	{
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_PROP, TRUE );
	}

	dxC_DPVS_ModelCreateCullObject( pModel, pModelDefinition );


#if !ISVERSION( SERVER_VERSION )
	// Mark static shadow maps as dirty if this is a new background model.
	if ( bNewModelDef && ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
		dxC_DirtyStaticShadowBuffers();
#endif

	//VECTOR vFrom;
	//V_DO_SUCCEEDED( e_GetModelRenderOBBCenterInWorld( nModelId, vFrom ) )
	//{
	//	V( e_LightSpecularSelectNearest(
	//		vFrom,

	//		) );
	//}

	if ( AppIsHellgate() && !AppIsDemo() )
	{
	    // find and add any ad objects
	    for ( int i = 0; i < pModelDefinition->nAdObjectDefCount; ++i )
	    {
		    e_AdClientAddInstance( pModelDefinition->pAdObjectDefs[ i ].szName, nModelId, i );
	    }
	}


	// add particles
	ASSERT_RETFAIL( gpfn_AttachmentNew );
	for ( int i = 0; i < pModelDefinition->nAttachmentCount; i++ )
	{
		gpfn_AttachmentNew( nModelId, &pModelDefinition->pAttachments[ i ] );
	}

	// initialize per-mesh model data
	V_RETURN( e_ModelInitSelfIlluminationPhases( nModelId ) );
	V_RETURN( e_ModelInitScrollingMaterialPhases( nModelId ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelNew( int* pnModelId, int nModelDefinitionId )
{
	ASSERT_RETINVALIDARG( pnModelId );
	*pnModelId = INVALID_ID;

	int nModelId = gnModelIdNext;
	gnModelIdNext++;
	// Create a new D3D Geometry
	D3D_MODEL* pModel = HashAdd( gtModels, nModelId );
	pModel->nId							= nModelId;
	pModel->vScale						= VECTOR( 1.0f, 1.0f, 1.0f );
	ZeroMemory( pModel->dwFlagBits, sizeof(pModel->dwFlagBits) );
	SETBIT( pModel->dwFlagBits, MODEL_FLAGBIT_FORCE_SETLOCATION, TRUE );
	pModel->nAppearanceDefId			= INVALID_ID;
	pModel->dwLightGroup				= LIGHT_GROUPS_DEFAULT;
	pModel->nRoomID						= INVALID_ID;
	pModel->nCellID						= INVALID_ID;
	pModel->nTextureMaterialOverride	= INVALID_ID;
	pModel->nEnvironmentDefOverride		= INVALID_ID;
	pModel->nRegion						= 0;
	pModel->nProxmapId					= INVALID_ID;
	pModel->fAlpha						= 1.0f;
	pModel->fDistanceAlpha				= 1.0f;
	pModel->nModelDefinitionId			= INVALID_ID;
	pModel->nDisplayLOD					= LOD_NONE;
	pModel->nDisplayLODOverride			= LOD_NONE;
	pModel->tLights.Init();

	for ( int i = 0; i < NUM_MATERIAL_OVERRIDE_TYPES; i++ )
	{
		for ( int j = 0; j < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH; j++ )
		{
			pModel->pStateMaterialOverride[ i ][ j ].nMaterialId	= INVALID_ID;
			pModel->pStateMaterialOverride[ i ][ j ].nAttachmentId	= INVALID_ID;
		}
		pModel->nStateMaterialOverrideIndex[ i ] = -1;
	}

	ASSERT_RETFAIL( gpfn_AttachmentModelNew );
	gpfn_AttachmentModelNew( nModelId );

	// initialize occlusion
	V( dx9_RestoreOcclusionQuery( &pModel->tOcclusion, pModel->nId ) ); // is this the right place to do this?

	pModel->AmbientLight = 1;
	pModel->HitColor = 0;
	pModel->bFullbright = FALSE;
	pModel->fBehindColor = 0;
	MatrixIdentity( (MATRIX*)&pModel->matScaledWorld );
	MatrixIdentity( (MATRIX*)&pModel->matWorld );
	MatrixIdentity( (MATRIX*)&pModel->matScaledWorldInverse );
	MatrixIdentity( (MATRIX*)&pModel->matView );

	// CHB 2006.12.08, 2006.11.28
	memset(pModel->tModelDefData, 0, sizeof(pModel->tModelDefData));

	pModel->fLODScreensize = e_LODGetDefaultModelLODScreensize();
	CLT_VERSION_ONLY( pModel->RequestFeatures.SetAll(); )
	CLT_VERSION_ONLY( pModel->ForceFeatures.SetNone(); )

	// adds reference inside
	V( e_ModelSetDefinition( nModelId, nModelDefinitionId, e_ModelGetDisplayLOD( nModelId ) ) );

	//pModel->nAttachToBone = INVALID_ID;
	//pModel->nAttachToModel= INVALID_ID;

	if( AppIsHellgate() || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW ) )
	{
		V( dx9_ModelAddToProxMap( pModel ) );
	}

	TRACE_MODEL( nModelId, "Model New" );

	*pnModelId = nModelId;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelRemovePrivate( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return S_FALSE;
	}

	V( dx9_ModelRemoveFromProxmap( pModel ) );

	e_ModelDefinitionReleaseRef( pModel->nModelDefinitionId, TRUE );

	V( dx9_ReleaseOcclusionQuery( &pModel->tOcclusion, pModel->nId ) );

	ASSERT_RETFAIL( gpfn_AttachmentModelRemove );
	gpfn_AttachmentModelRemove( nModelId );

	for ( int nLight = 0; nLight < pModel->nSpecularLightCount; nLight++ )
	{
		e_LightRemove(pModel->pnSpecularLightIds[ nLight ]);
	}

	DPVS_SAFE_RELEASE( pModel->pCullObject );

	V( dx9_ModelClearPerMeshData( pModel ) );

	// clean up anything we need to...
	HashRemove(gtModels, nModelId);

	TRACE_MODEL( nModelId, "Model Remove" );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ModelClearPerMeshDataForDefinition( int nModelDefID )
{
	// iterate all models and clear per-mesh data on the ones using this model def
	D3D_MODEL * pModel = dx9_ModelGetFirst();

	BOUNDED_WHILE ( pModel, 1000000 )
	{
		if ( pModel->nModelDefinitionId == nModelDefID )
		{
			V( dx9_ModelClearPerMeshData( pModel ) );
		}
		pModel = dx9_ModelGetNext( pModel );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ModelClearPerMeshData( D3D_MODEL * pModel )
{
	// CHB 2006.11.28
	int nModelDef = pModel->nModelDefinitionId;
	for (int j = 0; j < LOD_COUNT; ++j)
	{
		D3D_MODEL_DEFDATA * pDefData = &pModel->tModelDefData[j];	// CHB 2006.12.08
		if ( pDefData->pnTextureOverrides != 0 )
		{
			ASSERT(nModelDef != INVALID_ID);
			if (nModelDef != INVALID_ID)
			{
				for ( int i = 0; i < pDefData->nTextureOverridesAllocated; i++ )
				{
					if ( pDefData->pnTextureOverrides[i] != INVALID_ID )
						e_TextureReleaseRef( pDefData->pnTextureOverrides[i] );
				}
			}

			FREE( NULL, pDefData->pnTextureOverrides );
			pDefData->nTextureOverridesAllocated = 0;
			pDefData->pnTextureOverrides = NULL;
		}

		if ( pDefData->pdwMeshTechniqueMasks != 0 )
		{
			FREE( NULL, pDefData->pdwMeshTechniqueMasks );
			pDefData->pdwMeshTechniqueMasks = NULL;
		}

		if ( pDefData->ptMeshRenderOBBWorld )
		{
			FREE( NULL, pDefData->ptMeshRenderOBBWorld );
			pDefData->ptMeshRenderOBBWorld = NULL;
		}

		if ( pDefData->ptLightmapData )
		{
			FREE( NULL, pDefData->ptLightmapData );
			pDefData->ptLightmapData = NULL;
		}

		if ( pDefData->pvScrollingUVPhases )
		{
			FREE( NULL, pDefData->pvScrollingUVPhases );
			pDefData->pvScrollingUVPhases = NULL;
		}

		if ( pDefData->pfScrollingUVOffsets )
		{
			FREE( NULL, pDefData->pfScrollingUVOffsets );
			pDefData->pfScrollingUVOffsets = NULL;
		}

		if ( pDefData->pnMaterialVersion )
		{
			// Until Chris or Chuck work on making sure that this doesn't crash in the memory code, I'm creating this memory leak - Tyler
			// CHB 2006.12.08 - Hopefully this will do the trick.
			FREE( NULL, pDefData->pnMaterialVersion );
			pDefData->pnMaterialVersion = NULL;
		}

		pDefData->dwFlags = 0;
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

MODEL_SORT_TYPE e_ModelGetSortType( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return MODEL_DYNAMIC;
	}
	return pModel->m_SortType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ModelSetScale( int nModelId, const VECTOR vScale )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	pModel->vScale = vScale;
	return e_ModelSetLocationPrivate( nModelId, (MATRIX *)&pModel->matWorld, (const VECTOR &)pModel->vPosition, pModel->m_SortType, TRUE );

}
PRESULT e_ModelSetScale( int nModelId, float fScale )
{
	return e_ModelSetScale( nModelId, VECTOR( fScale, fScale, fScale ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetAppearanceDefinition( int nModelId, int nAppearanceDefId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	pModel->nAppearanceDefId = nAppearanceDefId;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetAppearanceDefinition( 
	int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return INVALID_ID;
	}

	return pModel->nAppearanceDefId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetAppearance( int nModelId )
{
	return nModelId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//PRESULT dxC_ModelSetCullScreensize( D3D_MODEL * pModel, float fScreensize )
//{
//	ASSERT_RETINVALIDARG( pModel );
//	pModel->fCullScreensize = fScreensize;
//	return S_OK;
//}
//
//PRESULT e_ModelSetCullScreensize(int nModelId, float fScreensize )
//{
//	D3D_MODEL * pModel = dx9_ModelGet(nModelId);
//	if ( ! pModel )
//		return;
//	return dxC_ModelSetCullScreensize( pModel, fScreensize );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelComputeUnbiasedLODScreensize( D3D_MODEL * pModel, float & fScreensize, float fDist, D3D_MODEL_DEFINITION * pModelDef /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( pModel );

	if (    DRAW_LAYER_GENERAL != dxC_ModelGetDrawLayer( pModel ) 
		|| fDist <= 0.f
		|| fDist == Math_GetInfinity() )
	{
		fScreensize = 0.f;
		return S_FALSE;
	}

	float fWorldSize = 0.f;
	V( dxC_ModelGetWorldSizeAvg( pModel, fWorldSize, pModelDef ) );
	// Prevent small objects from fading in far too close to the camera.
	fWorldSize = MAX( fWorldSize, 1.0f );

	fScreensize = e_GetWorldDistanceUnbiasedScreenSizeByVertFOV( fDist, fWorldSize );
	return S_OK;
}


PRESULT e_ModelComputeUnbiasedLODScreensize( int nModelID, float fDist, float & fScreensize )
{
	D3D_MODEL * pModel = dx9_ModelGet(nModelID);
	if ( ! pModel )
		return S_FALSE;

	D3D_MODEL_DEFINITION * pModelDef = NULL;
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		if ( pModelDef ) break;
		pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, LOD_LOW );
	}

	return dxC_ModelComputeUnbiasedLODScreensize( pModel, fScreensize, fDist, pModelDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_ModelSetLODScreensize( D3D_MODEL * pModel, float fLODScreensize )
{
	ASSERT_RETURN( pModel );
	if ( fLODScreensize < 0.f )
		fLODScreensize = e_LODGetDefaultModelLODScreensize();
	pModel->fLODScreensize = fLODScreensize;
}

void e_ModelSetLODScreensize( int nModelId, float fLODScreensize )
{
	D3D_MODEL * pModel = dx9_ModelGet(nModelId);
	if ( ! pModel )
		return;

	dxC_ModelSetLODScreensize( pModel, fLODScreensize );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_ModelSetFarCullMethod( D3D_MODEL * pModel, MODEL_FAR_CULL_METHOD eMethod )
{
	ASSERT_RETURN( pModel );
	pModel->eFarCullMethod = eMethod;
}

void e_ModelSetFarCullMethod( int nModelId, MODEL_FAR_CULL_METHOD eMethod )
{
	D3D_MODEL * pModel = dx9_ModelGet(nModelId);
	if ( ! pModel )
		return;

	dxC_ModelSetFarCullMethod( pModel, eMethod );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_ModelSetDistanceCullType( D3D_MODEL * pModel, MODEL_DISTANCE_CULL_TYPE eType )
{
	ASSERT_RETURN( pModel );
	pModel->eDistanceCullType = eType;
}

void e_ModelSetDistanceCullType( int nModelId, MODEL_DISTANCE_CULL_TYPE eType )
{
	D3D_MODEL * pModel = dx9_ModelGet(nModelId);
	if ( ! pModel )
		return;

	dxC_ModelSetDistanceCullType( pModel, eType );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//void dxC_ModelSetLODDistance( D3D_MODEL * pModel, float fLODDistance )
//{
//	ASSERT_RETURN( pModel );
//
//	if ( fLODDistance <= 0.f )
//	{
//		pModel->fLODDistance = Math_GetInfinity();
//		pModel->fLODScreensize = 0.f;
//		return;
//	}
//
//	pModel->fLODDistance = fLODDistance;
//	sModelSetLODScreensize( pModel, NULL );
//}
//
//void e_ModelSetLODDistance( int nModelId, float fLODDistance)
//{
//	D3D_MODEL * pModel = dx9_ModelGet(nModelId);
//	if ( ! pModel )
//		return;
//
//	dxC_ModelSetLODDistance( pModel, fLODDistance );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sUpdateModelCell( D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel );

#if !ISVERSION(SERVER_VERSION)
	LP_INTERNAL_CELL pInternalCell = NULL;

	CULLING_CELL * pCell = e_CellGet( pModel->nRoomID );
	if ( pCell )
	{
		pInternalCell = pCell->pInternalCell;
		pModel->nCellID = pModel->nRoomID;
	}

	if ( ! pInternalCell /*&& ( bUsesOutdoorVis )*/ )
	{
		int nCullCell;
		ASSERT_RETFAIL( pModel->nRegion != INVALID_ID );
		V_RETURN( e_RegionGetCell( pModel->nRegion, nCullCell ) );
		if ( nCullCell != INVALID_ID )
		{
			pCell = e_CellGet( nCullCell );
			ASSERT_RETFAIL( pCell );
			pInternalCell = pCell->pInternalCell;
		}
		pModel->nCellID = nCullCell;
	}

	//if ( pInternalCell )
	{
		dxC_DPVS_ModelSetCell( pModel, pInternalCell );
	}

	//else if ( bUsesOutdoorVis )
	//{
	//	//dxC_DPVS_ModelSetCell( pModel, NULL );
	//	return;
	//}

	//if ( ! pInternalCell )
	//	return;
	//ASSERT_RETURN( pInternalCell );
	dxC_DPVS_ModelUpdate( pModel );
	//dxC_DPVS_ModelSetCell( pModel, pInternalCell );
#endif // !ISVERSION(SERVER_VERSION)

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetRoomID( int nModelID, int nRoomID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
	{
		return;
	}

	// debug
	//if ( nRoomID == 0 )
	//{
	//	// grab info about the model
	//	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
	//											e_ModelGetDisplayLOD( nModelID ) );
	//	//ASSERTX( dwUserData != 0, pModelDef ? pModelDef->tHeader.pszName : "unknown" );
	//}

	pModel->nRoomID = nRoomID;

	//if ( nRoomID == INVALID_ID )
	//	return;

	V( sUpdateModelCell( pModel ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetCellID( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
	{
		return INVALID_ID;
	}

	return pModel->nCellID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_ModelGetRoomID( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
	{
		return INVALID_ID;
	}

	return pModel->nRoomID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR e_ModelGetScale( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return VECTOR( 1.0f, 1.0f, 1.0f );
	}

	return pModel->vScale;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2006.12.08
static
PRESULT sCreateMeshBoundingBoxes( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef, int nLOD )
{
	if ( 0 == pModelDef->nMeshCount )
		return S_FALSE;

	VECTOR * ptMeshRenderOBBWorld = pModel->tModelDefData[nLOD].ptMeshRenderOBBWorld;
	if (ptMeshRenderOBBWorld == 0)
	{
		ptMeshRenderOBBWorld = (VECTOR*) MALLOCZ( NULL, sizeof(VECTOR) * OBB_POINTS * pModelDef->nMeshCount );
		pModel->tModelDefData[nLOD].ptMeshRenderOBBWorld = ptMeshRenderOBBWorld;
	}
	ASSERT_RETFAIL( ptMeshRenderOBBWorld );
	for ( int nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	{
		const BOUNDING_BOX * pSrcBBox;
		pSrcBBox = e_MeshGetBoundingBox( pModelDef->tHeader.nId, nLOD, nMesh );
		TransformAABBToOBB(
			&ptMeshRenderOBBWorld[ nMesh * OBB_POINTS ],
			(MATRIX*)&pModel->matScaledWorld,
			pSrcBBox );
	}
	return S_OK;
}

// CHB 2006.12.08
static
PRESULT sCreateMeshBoundingBoxes( D3D_MODEL * pModel )
{
	int nModelDef = pModel->nModelDefinitionId;
	for ( int i = LOD_LOW; i < LOD_COUNT; ++i )
	{
		D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( nModelDef, i );
		if ( pModelDefinition )
		{
			V( sCreateMeshBoundingBoxes(pModel, pModelDefinition, i) );
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUpdateModelLightsFromDefinition( D3D_MODEL* pModel, MLI_DEFINITION * pSharedModelDef )
{
	if ( !pSharedModelDef )
		return S_FALSE;

	// add lights if they haven't already been added.
	if ( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MLI_DEF_APPLIED ) )
	{
		ASSERT_RETFAIL( gpfn_AttachmentDefinitionNewStaticLight );
		for ( int nLight = 0; nLight < pSharedModelDef->nStaticLightCount; nLight++ )
		{
			gpfn_AttachmentDefinitionNewStaticLight( pModel->nId, 
				pSharedModelDef->pStaticLightDefinitions[ nLight ], FALSE );
		} 

		ASSERT_RETFAIL( MAX_SPECULAR_LIGHTS_PER_EFFECT >= MAX_SPECULAR_LIGHTS_PER_MODEL );
		for ( int nLight = 0; nLight < pSharedModelDef->nSpecularLightCount; nLight++ )
		{
			pModel->pnSpecularLightIds[ nLight ] = 
				e_LightNewStatic( &pSharedModelDef->pSpecularLightDefinitions[ nLight ], TRUE );
		}
		pModel->nSpecularLightCount = pSharedModelDef->nSpecularLightCount;

		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_MLI_DEF_APPLIED, TRUE );
	}

	for ( int nLight = 0; nLight < pSharedModelDef->nSpecularLightCount; nLight ++ )
	{
		D3DXVECTOR4 vPositionNew;
		D3DXVec3Transform( &vPositionNew, (D3DXVECTOR3 *) &pSharedModelDef->pSpecularLightDefinitions[ nLight ].vPosition, (D3DXMATRIXA16*)&pModel->matWorld );
		e_LightSetPosition( pModel->pnSpecularLightIds[ nLight ], (VECTOR &)vPositionNew );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUpdateModelAdObjectsFromDefinition( D3D_MODEL* pModel, D3D_MODEL_DEFINITION* pModelDefinition )
{
#ifdef AD_CLIENT_ENABLED
	if ( c_GetToolMode() || ! AppIsHellgate() )
		return S_FALSE;

	if ( !pModelDefinition || ! pModelDefinition->pAdObjectDefs )
		return S_FALSE;

	if( pModel->nAdObjectInstances != pModelDefinition->nAdObjectDefCount )
		return S_FALSE;

	for ( int nAdObject = 0; nAdObject < pModelDefinition->nAdObjectDefCount; nAdObject ++ )
	{
		AD_OBJECT_DEFINITION * pAdObjDef = &pModelDefinition->pAdObjectDefs[ nAdObject ];
		V( e_AdInstanceUpdateTransform(
			pModel->pnAdObjectInstances[ pAdObjDef->nIndex ],
			pAdObjDef,
			&pModel->matScaledWorld ) );
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUpdateModelBBoxFromDefinition( D3D_MODEL* pModel, D3D_MODEL_DEFINITION* pModelDefinition )
{
	if ( !pModelDefinition )
		return S_FALSE;

	BOUNDING_BOX tSrcBBox  = pModelDefinition->tRenderBoundingBox;

	// if the model has a ragdoll position, update the bounding boxen
#ifdef HAVOK_ENABLED
	VECTOR vRagPos;
	BOOL bRagPos = e_ModelGetRagdollPosition( pModel->nId, vRagPos );
	if ( bRagPos )
	{
		V( sUpdateBoundingBoxPosition( pModel, &tSrcBBox, &vRagPos ) );
	}
#endif

	BOUNDING_BOX * pDestBBox = &pModel->tRenderAABBWorld;
	BoundingBoxScale( pDestBBox, &tSrcBBox, pModel->vScale );
	BoundingBoxTranslate( (MATRIX*)&pModel->matWorld, pDestBBox, pDestBBox );

	TransformAABBToOBB( pModel->tRenderOBBWorld, (MATRIX*)&pModel->matScaledWorld, &tSrcBBox );

	if ( pModel->m_SortType == MODEL_STATIC)
	{
		// update mesh bounding boxes
		V( sCreateMeshBoundingBoxes( pModel ) );	// CHB 2006.12.08
	}

#if !ISVERSION(SERVER_VERSION)
	V( dxC_DPVS_ModelUpdate( pModel ) );
#endif // !ISVERSION(SERVER_VERSION)

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_ModelSetLocationPrivate(
	int nModelId,
	const MATRIX* pmatWorld,
	const VECTOR& vPosition,
	MODEL_SORT_TYPE SortType,
	BOOL bForce,
	BOOL bUpdateAttachments)
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return S_FALSE;
	}

	//BOOL beqRom = ( pModel->m_idRoom  == idRoom		);
	BOOL beqPos = ( pModel->vPosition == vPosition  );
	BOOL beqMat = ( pModel->matWorld  == *pmatWorld );

	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_SETLOCATION ) )
		bForce = TRUE;
	if ( pModel->nModelDefinitionId == INVALID_ID )
		bForce = TRUE;

	if( SortType == MODEL_ICONIC )
	{
		dxC_ModelSetDrawBehind( pModel, TRUE );
	}
	else
	{
		dxC_ModelSetDrawBehind( pModel, FALSE );
	}



	if ( !bForce && beqPos && beqMat && SortType == pModel->m_SortType )
	{
		HITCOUNT(MODEL_SETLOC_SKIP);
		return S_FALSE;
	}

	HITCOUNT(MODEL_SETLOC);

	pModel->matWorld = *pmatWorld;
	//D3DXMatrixInverse( & pModel->matWorldInverse, NULL, &pModel->matWorld );

	D3DXMATRIX mScale;
	if( AppIsTugboat() )
	{
		D3DXMatrixScaling( &mScale, pModel->vScale.fX, pModel->vScale.fY, pModel->vScale.fZ );
	}
	else
	{
		//this is a temporary fix for hellgate.  need to figure out why the above code isn't working.
		//D3DXMatrixScaling( &mScale, pModel->vScale.fX, pModel->vScale.fX, pModel->vScale.fX );

		D3DXMatrixScaling( &mScale, pModel->vScale.fX, pModel->vScale.fY, pModel->vScale.fZ );
	}

	D3DXMatrixMultiply( (D3DXMATRIXA16*)&pModel->matScaledWorld, &mScale, (D3DXMATRIXA16*)&pModel->matWorld );

	D3DXMatrixInverse( (D3DXMATRIXA16*)& pModel->matScaledWorldInverse, NULL, (D3DXMATRIXA16*)&pModel->matScaledWorld );

	D3DXMatrixInverse( (D3DXMATRIXA16*)& pModel->matWorldInverse, NULL, (D3DXMATRIXA16*)&pModel->matWorld );

	dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_WORLD, TRUE );

	pModel->m_SortType = SortType;
	pModel->vPosition = vPosition;

	if( pModel->nProxmapId != INVALID_ID )
	{
		V( dx9_ModelMoveInProxmap( pModel ) );
	}

	BOOL bForceNext = TRUE;

	if ( pModel->nModelDefinitionId != INVALID_ID )
	{
		BOOL bSetDefinition = FALSE;
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED ) )
			bSetDefinition = TRUE;

		MLI_DEFINITION * pSharedModelDef = dxC_SharedModelDefinitionGet( pModel->nModelDefinitionId );
		V( sUpdateModelLightsFromDefinition( pModel, pSharedModelDef ) );

		for( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
			if ( pModelDefinition ) 
			{
				bForceNext = FALSE;

				if ( bSetDefinition )
					e_ModelSetDefinition( pModel->nId, pModel->nModelDefinitionId, nLOD );

				V( sUpdateModelAdObjectsFromDefinition( pModel, pModelDefinition ) );

				// update bounding box
				if ( BoundingBoxIsZero( pModelDefinition->tRenderBoundingBox ) )
				{
					// render bounding box isn't set yet, so force the next setlocation to go through
					bForceNext = TRUE;
				}
				else
				{
					V( sUpdateModelBBoxFromDefinition( pModel, pModelDefinition ) );
				}
			}
		}
	} 
	else 
	{
		pModel->tRenderAABBWorld.vMin = vPosition;
		pModel->tRenderAABBWorld.vMax = vPosition;
		for ( int i = 0; i < OBB_POINTS; i++ )
		{
			pModel->tRenderOBBWorld[ i ] = vPosition;
		}
	}

	dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_FORCE_SETLOCATION, bForceNext );

	ASSERT_RETFAIL( gpfn_ModelAttachmentsUpdate );

	//gpfn_ModelAttachmentsUpdate( pModel->nId, MODEL_DYNAMIC ); // I added MODEL_DYNAMIC until this file is merged.   I was working on attachment code - Tyler
	if( bUpdateAttachments )
	{
		gpfn_ModelAttachmentsUpdate( pModel->nId, SortType );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_ModelGetLightmapColor ( 
	int nModelId, 
	const VECTOR & vWorldPosition,
	const VECTOR & vNormal )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	DWORD dwDefaultColor = ARGB_MAKE_FROM_INT( 125, 125, 125, 255 );
	if (!pModel)
	{
		return dwDefaultColor;
	}
	return dwDefaultColor; /// this function is causing all sorts of buffer locking issues

#if 0 // this section wasn't compiling
//#if defined(ENGINE_TARGET_DX9) //KMNV TODO
	VECTOR vPositionInModel;
	MatrixMultiply( &vPositionInModel, &vWorldPosition, (MATRIX*)&pModel->matScaledWorldInverse );

	VECTOR vTestStart = vPositionInModel;
	VECTOR vNormalOffset = vNormal * -0.001f;
	vTestStart += vNormalOffset;

	int nMeshIndex;
	int nTriangleId;
	VECTOR vPositionOnMesh;
	float fU, fV;

	if ( ! e_ModelDefinitionCollide( pModel->nModelDefinitionId, vTestStart, -vNormal, 
									 &nMeshIndex, &nTriangleId, &vPositionOnMesh, &fU, &fV ) )
		return dwDefaultColor;

	D3D_MODEL_DEFINITION * pModelDef = dx9_ModelDefinitionGet( pModel->nModelDefinitionId );
	if ( ! pModelDef )
		return dwDefaultColor;

	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMeshIndex ] );
	if ( ! pMesh )
		return dwDefaultColor;

	int nLOD = LOD_HIGH;	// CHB 2006.12.08

	int nTexture = INVALID_ID;
	if ( pModel->tModelDefData[nLOD].pnTextureOverrides )	// CHB 2006.12.08, 2006.11.28
	{
		int nIndex = nMeshIndex * NUM_TEXTURE_TYPES + TEXTURE_LIGHTMAP;
		if ( nIndex >= pModel->tModelDefData[nLOD].nTextureOverridesAllocated )
			nIndex = TEXTURE_LIGHTMAP;
		ASSERT_RETVAL( nIndex < pModel->tModelDefData[nLOD].nTextureOverridesAllocated, 0 );
		nTexture = pModel->tModelDefData[nLOD].pnTextureOverrides[ nMeshIndex * NUM_TEXTURE_TYPES + TEXTURE_LIGHTMAP ];	// CHB 2006.12.08, 2006.11.28
	} else {
		nTexture = pMesh->pnTextures[ TEXTURE_LIGHTMAP ];
	}
	if ( nTexture == INVALID_ID )
		return dwDefaultColor;

	D3D_TEXTURE* pTexture = dxC_TextureGet( nTexture );
	if (!pTexture || ! pTexture->pD3DTexture)
		return dwDefaultColor;

	if ( FAILED( dx9_GrannyGetTextCoordsAtPos( pModelDef, pMesh, nTriangleId, TRUE, fU, fV ) ) )
		return dwDefaultColor;

	SPDIRECT3DSURFACE9 pSurfaceDest;
	V( dxC_GetDevice()->CreateOffscreenPlainSurface( 2, 2, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurfaceDest, NULL ) );

	SPDIRECT3DSURFACE9 pSurfaceSrc;
	V( pTexture->pD3DTexture->GetSurfaceLevel( 0, &pSurfaceSrc ) );

	D3DC_TEXTURE2D_DESC tDescSrc;
	V( pTexture->pD3DTexture->GetLevelDesc( 0, &tDescSrc ) );

	RECT rSrcRect;
	rSrcRect.top	= (int) (fV * tDescSrc.Height);
	rSrcRect.bottom = rSrcRect.top + 2;
	if ( (UINT)rSrcRect.bottom >= tDescSrc.Height )
	{
		rSrcRect.top -= 2;
		rSrcRect.bottom -= 2;
	}
	rSrcRect.left  = (int) (fU * tDescSrc.Width);
	rSrcRect.right = rSrcRect.left + 2;
	if ( (UINT)rSrcRect.right >= tDescSrc.Width )
	{
		rSrcRect.left -= 2;
		rSrcRect.right -= 2;
	}

	ASSERT_RETVAL( rSrcRect.top  >= 0 && (UINT)rSrcRect.bottom < tDescSrc.Height &&
		           rSrcRect.left >= 0 && (UINT)rSrcRect.right  < tDescSrc.Width, 0xffffffff );
	V( D3DXLoadSurfaceFromSurface( pSurfaceDest, NULL, NULL, pSurfaceSrc, NULL, &rSrcRect, D3DX_FILTER_POINT, 0 ) );

	D3DLOCKED_RECT tLockedRect; 
	V( pSurfaceDest->LockRect( &tLockedRect, NULL, D3DLOCK_READONLY ) );
	DWORD dwColor = *(DWORD *)tLockedRect.pBits;
	V( pSurfaceDest->UnlockRect() );

	return dwColor;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelSetDrawLayer(
	D3D_MODEL * pModel,
	DRAW_LAYER eDrawLayer )
{
	dxC_ModelDrawLayerToDrawFlags( eDrawLayer, pModel->dwDrawFlags );

	//for ( int i = 0; i < pModel->tAttachmentHolder.nCount; i++ )
	//{
	//	c_AttachmentSetDrawLayer( pModel->tAttachmentHolder.pAttachments[ i ], bDrawLayer );
	//}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetDrawLayer( 
	int nModelId, 
	DRAW_LAYER eDrawLayer )
{
    D3D_MODEL* pModel = dx9_ModelGet( nModelId );
    if (!pModel)
    {
	    return S_FALSE;
    }

	return dxC_ModelSetDrawLayer( pModel, eDrawLayer );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DRAW_LAYER dxC_ModelGetDrawLayer( const D3D_MODEL * pModel )
{
	ASSERT_RETVAL( pModel, DRAW_LAYER_GENERAL );

	return dxC_ModelDrawLayerFromDrawFlags( pModel->dwDrawFlags );
}

//-------------------------------------------------------------------------------------------------

DRAW_LAYER e_ModelGetDrawLayer( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
		return DRAW_LAYER_GENERAL;

	return dxC_ModelGetDrawLayer( pModel );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetRegionPrivate( int nModelId, int nRegion )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
		return;

	pModel->nRegion = nRegion;

	V( sUpdateModelCell( pModel ) );

#ifdef ENABLE_DPVS
	//if ( pModel->pCullObject )
	//{
	//	REGION * pRegion = e_RegionGet( nRegion );
	//	int nCullCell = pRegion ? pRegion->nCullCell : NULL;
	//	CULLING_CELL * pCell = e_CellGet( nCullCell );
	//	if ( ! pCell && pModel->nRoomID != INVALID_ID )
	//		pCell = e_CellGet( pModel->nRoomID );
	//	if ( pCell )
	//	{
	//		dxC_DPVS_SetCell( pModel->pCullObject, pCell->pInternalCell );
	//	}
	//}
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetRegion( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
		return -1;

	return pModel->nRegion;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetView( int nModelId, const MATRIX *pmatView )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return;
	}

	pModel->matView = *pmatView;
	dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_VIEW, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetProj( int nModelId, const MATRIX *pmatProj )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return;
	}

	pModel->matProj = *pmatProj;
	dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_PROJ, TRUE );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelExists( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	return pModel != NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ModelSetInFrustum( int nModelId, BOOL bSet )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return;
	}

	pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ] = bSet;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL dx9_ModelGetInFrustumPrev( D3D_MODEL * pModel )
{
	ASSERT_RETFALSE(pModel);
	return pModel->bInFrustum[ MODEL_IN_FRUSTUM_PREV ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ModelSwapInFrustum( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return;
	}

	//if ( e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) && pModel->bInFrustum[ MODEL_IN_FRUSTUM_PREV ] != pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ] )
	//{
	//	DebugString( OP_DEBUG, L"In frustum: curr %d, prev %d", pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ], pModel->bInFrustum[ MODEL_IN_FRUSTUM_PREV ] );
	//}

	pModel->bInFrustum[ MODEL_IN_FRUSTUM_PREV ] = pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ];
	pModel->bInFrustum[ MODEL_IN_FRUSTUM_CURR ] = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelSetFlagbit( D3D_MODEL * pModel, MODEL_FLAGBIT eFlagBit, BOOL bSet )
{
#if ISVERSION(DEVELOPMENT)
	//if ( dwFlags & MODEL_FLAG_VISIBLE )
	//{
	//	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	//	if ( nIsolateModel != INVALID_ID && nIsolateModel != pModel->nId )
	//		bSet = FALSE;
	//	TRACE_MODEL_INT( pModel->nId, "Set Visible", bSet );
	//}
#endif

	SETBIT( pModel->dwFlagBits, eFlagBit, bSet );

	if ( eFlagBit == MODEL_FLAGBIT_FIRST_PERSON_PROJ )
	{
		if ( pModel->nModelDefinitionId != INVALID_ID )
		{
			for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
			{
				D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
				if ( pModelDef )
				{
					// marking "has first person proj" so that it can be rendered first

					// CML 2006.10.4: TUGBOAT merge: the following commented if was in (not commented) tugboat -- any reason in particular?

					//if( bSet )
					//{
					pModelDef->dwFlags |= MODELDEF_FLAG_HAS_FIRST_PERSON_PROJ;
					//}
					//else
					//{
					//	pModelDef->dwFlags &= ~MODELDEF_FLAG_HAS_FIRST_PERSON_PROJ;
					//}
				}
			}
		}
	}

	if ( eFlagBit == MODEL_FLAGBIT_NODRAW )
	{
		V( e_DPVS_ModelSetEnabled( pModel->nId, ! TESTBIT_MACRO_ARRAY( pModel->dwFlagBits, MODEL_FLAGBIT_NODRAW ) ) );
	}

	ASSERT_RETFAIL( gpfn_AttachmentSetModelFlagbit );
	gpfn_AttachmentSetModelFlagbit( pModel->nId, eFlagBit, bSet );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetFlagbit( int nModelId, MODEL_FLAGBIT eFlagbit, BOOL bSet )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	return dxC_ModelSetFlagbit( pModel, eFlagbit, bSet );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_ModelSetNoDrawGroup( D3D_MODEL * pModel, DWORD dwDrawGroup, BOOL bSet )
{
	SETBIT( pModel->dwDrawFlags, MODEL_DRAWFLAG_NODRAW_GROUP0 << dwDrawGroup, bSet );
}

PRESULT e_ModelSetNoDrawGroup( int nModelId, DWORD dwDrawGroup, BOOL bSet )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	dxC_ModelSetNoDrawGroup( pModel, dwDrawGroup, bSet );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL dxC_ModelGetNoDrawGroup( D3D_MODEL * pModel, DWORD dwDrawGroup )
{
	return TESTBIT_DWORD( pModel->dwDrawFlags, MODEL_DRAWFLAG_NODRAW_GROUP0 << dwDrawGroup );
}

PRESULT e_ModelGetNoDrawGroup( int nModelId, DWORD dwDrawGroup, BOOL & bIsSet )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	bIsSet = dxC_ModelGetNoDrawGroup( pModel, dwDrawGroup );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelGetFlagbit( int nModelId, MODEL_FLAGBIT eFlagbit )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return 0;
	}

	return TESTBIT_MACRO_ARRAY( pModel->dwFlagBits, eFlagbit );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelUpdateVisibilityToken( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
		return E_INVALIDARG;
	return dxC_ModelUpdateVisibilityToken( pModel );
}

PRESULT dxC_ModelUpdateVisibilityToken( D3D_MODEL * pModel )
{
	pModel->dwVisibilityToken = e_GetVisibilityToken();	

	ASSERT_RETFAIL( gpfn_AttachmentSetVisible );
	gpfn_AttachmentSetVisible( pModel->nId, TRUE );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelCurrentVisibilityToken( int nModelID )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if (!pModel)
		return FALSE;
	return dxC_ModelCurrentVisibilityToken( pModel );
}

BOOL dxC_ModelCurrentVisibilityToken( const D3D_MODEL * pModel )
{
	if ( pModel && pModel->dwVisibilityToken == e_GetVisibilityToken() )
		return TRUE;
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ModelSetAlpha( D3D_MODEL * pModel, float fAlpha )
{
	ASSERT_RETINVALIDARG( pModel );

	// saturate alpha
	fAlpha = min( fAlpha, 1.f );
	fAlpha = max( fAlpha, 0.f );

	pModel->fAlpha = fAlpha;
	return dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA, ( fAlpha < 1.f ) || pModel->fDistanceAlpha < 1.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetAlpha( int nModelID, float fAlpha )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;
	return dx9_ModelSetAlpha( pModel, fAlpha );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_ModelGetAlpha( 
	int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return 1.0f;
	return pModel->fAlpha;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelSetDistanceAlpha( D3D_MODEL * pModel, float fAlpha )
{
	ASSERT_RETINVALIDARG( pModel );

	// saturate alpha
	pModel->fDistanceAlpha = SATURATE( fAlpha );
	return dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA, ( fAlpha < 1.f ) || pModel->fAlpha < 1.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetDistanceAlpha( int nModelID, float fAlpha )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	return dxC_ModelSetDistanceAlpha( pModel, fAlpha );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetLightGroup( int nModelId, DWORD dwLightGroups )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	pModel->dwLightGroup = dwLightGroups;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_ModelGetLightGroup( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return 0;
	}

	return pModel->dwLightGroup;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetDefinition ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return INVALID_ID;
	}

	return pModel->nModelDefinitionId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelCalculateDistanceAlpha( float & fDistanceAlpha, const D3D_MODEL * pModel, float fScreensize, float fDistToEyeSq, float fPointDistToEyeSq )
{
#if ! ISVERSION(SERVER_VERSION)
	ASSERT_RETINVALIDARG( pModel );
	fDistanceAlpha = 1.f;

	if ( AppIsHellgate() && LEVEL_LOCATION_INDOOR == e_GetCurrentLevelLocation( pModel->nRegion ) )
		return S_FALSE;

	if (    dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_DISTANCE_CULLABLE )
		 && dxC_ModelGetDrawLayer( pModel ) == DRAW_LAYER_GENERAL )
	{
		if ( pModel->eFarCullMethod == MODEL_FAR_CULL_BY_SCREENSIZE ||
			 pModel->eFarCullMethod == MODEL_CANOPY_CULL_BY_SCREENSIZE )
		{
			float fScale = e_OptionState_GetActive()->get_fCullSizeScale();
			fScale = SATURATE( fScale );
			fScale = 1.f / LERP( MIN_DISTANCE_CULL_SIZE_SCALE, MAX_DISTANCE_CULL_SIZE_SCALE, fScale );
			float fMin = AppIsHellgate() ? ( gtModelCullScreensizes[ pModel->eDistanceCullType ].f * fScale ) :
										   ( gtModelCullScreensizesTugboat[ pModel->eDistanceCullType ].f * fScale );
			float fMax = AppIsHellgate() ? ( gtModelCullScreensizes[ pModel->eDistanceCullType ].n * fScale ) :
											( gtModelCullScreensizesTugboat[ pModel->eDistanceCullType ].n * fScale );
			fDistanceAlpha = ( fScreensize - fMin )	 /	( fMax - fMin );
			fDistanceAlpha = SATURATE( fDistanceAlpha );
			if( fDistanceAlpha == 1 &&
				pModel->eFarCullMethod == MODEL_CANOPY_CULL_BY_SCREENSIZE )
			{
				float fMin = gtModelCanopyCullDistances[ pModel->eDistanceCullType ].n;
				float fMax = gtModelCanopyCullDistances[ pModel->eDistanceCullType ].f;

				float fDist = SQRT_SAFE(fPointDistToEyeSq);
				fDistanceAlpha = ( fDist - fMin )	/	( fMax - fMin );
				fDistanceAlpha = SATURATE( fDistanceAlpha );	
			}
		}
		else if ( pModel->eFarCullMethod == MODEL_FAR_CULL_BY_DISTANCE )
		{
			float fMin = AppIsHellgate() ? gtModelCullDistances[ pModel->eDistanceCullType ].n : gtModelCullDistancesTugboat[ pModel->eDistanceCullType ].n;
			float fMax = AppIsHellgate() ? gtModelCullDistances[ pModel->eDistanceCullType ].f : gtModelCullDistancesTugboat[ pModel->eDistanceCullType ].f;

			// If there is a distance cull override (sniper mode), and this model should use it, use that instead.
			if ( gbModelCullDistanceOverride && pModel->eDistanceCullType == MODEL_DISTANCE_CULL_ANIMATED_ACTOR )
			{
				fMin = gtModelCullDistanceOverride.n;
				fMax = gtModelCullDistanceOverride.f;
			}

			float fDist = AppIsHellgate() ? SQRT_SAFE(fDistToEyeSq) : SQRT_SAFE(fPointDistToEyeSq);
			fDistanceAlpha = ( fDist - fMin )	/	( fMax - fMin );
			fDistanceAlpha = 1.f - SATURATE( fDistanceAlpha );
		}

	}
#endif

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDistanceCullSetOverride( float fDist )
{
	gbModelCullDistanceOverride = TRUE;
	gtModelCullDistanceOverride.n = fDist;
	gtModelCullDistanceOverride.f = fDist + MODEL_ALPHA_FADE_OVER_DISTANCE;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDistanceCullClearOverride()
{
	gbModelCullDistanceOverride = FALSE;
	gtModelCullDistanceOverride.n = 0.f;
	gtModelCullDistanceOverride.f = 0.f;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetDisplayLODOverride( int nModel, int nLOD )
{
#if ISVERSION( SERVER_VERSION )
	return;
#else

	D3D_MODEL* pModel = dx9_ModelGet( nModel );
	if ( !pModel )
		return;

	pModel->nDisplayLODOverride = nLOD;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dxC_ModelGetFirstLOD( const D3D_MODEL * pModel )
{
#if ISVERSION( SERVER_VERSION )
	return -1;
#else
	ASSERT_RETINVALID( pModel );

	for ( int i = 0; i < LOD_COUNT; ++i )
	{
		if ( dxC_ModelDefinitionGet( pModel->nModelDefinitionId, i ) )
			return i;
	}
	return -1;
#endif	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetFirstLOD( int nModel )
{
#if ISVERSION( SERVER_VERSION )
	return -1;
#else
	D3D_MODEL* pModel = dx9_ModelGet( nModel );
	if ( !pModel )
	{
		return LOD_LOW;
	}

	return dxC_ModelGetFirstLOD( pModel );
#endif	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dxC_ModelGetDisplayLOD( D3D_MODEL * pModel )
{
#if ISVERSION( SERVER_VERSION )
	return -1;
#else
	ASSERT_RETINVALID( pModel );

	if ( pModel->nDisplayLOD == LOD_NONE )
		dxC_ModelSelectLOD( pModel, -1.f, -1.f );

	if ( pModel->nDisplayLODOverride != LOD_NONE )
		return pModel->nDisplayLODOverride;
	else
		return pModel->nDisplayLOD;
#endif	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelGetDisplayLOD( int nModel )
{
#if ISVERSION( SERVER_VERSION )
	return -1;
#else
	D3D_MODEL* pModel = dx9_ModelGet( nModel );
	if ( !pModel )
	{
		return LOD_LOW;
	}

	return dxC_ModelGetDisplayLOD( pModel );
#endif	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dxC_ModelGetDisplayLODWithDefault( D3D_MODEL* pModel )
{
#if ISVERSION( SERVER_VERSION )
	return -1;
#else
	int nModelDef = pModel->nModelDefinitionId;
	int nLOD = LOD_LOW;

	if (nModelDef != INVALID_ID)
	{
		D3D_MODEL_DEFINITION* pModelDefinition = NULL;
		for ( nLOD = dxC_ModelGetDisplayLOD( pModel ); nLOD >= 0; nLOD-- )
		{
			pModelDefinition = dxC_ModelDefinitionGet( nModelDef, nLOD );
			if ( pModelDefinition )
				break;
		}

		if ( !pModelDefinition )
			nLOD = LOD_LOW;
	}

	return nLOD;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelGetWorldSizeAvgWeighted( D3D_MODEL * pModel, float & fSize, const VECTOR & vWeights, D3D_MODEL_DEFINITION * pModelDefinition /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( pModel );

	VECTOR vSize = BoundingBoxGetSize( &pModel->tRenderAABBWorld );
	if ( VectorIsZero( vSize ) )
	{
		if ( pModelDefinition )
		{
			vSize = BoundingBoxGetSize( &pModelDefinition->tRenderBoundingBox );
			if ( VectorIsZero( vSize ) )
				vSize = BoundingBoxGetSize( &pModelDefinition->tBoundingBox );
			// We only need to scale the size vector by model scale from the modeldef bounding box.
			vSize = vSize * pModel->vScale;
			//vSize *= pModel->fScale;
		}
	}
	//fSize = MAX( vSize.x, MAX( vSize.y, vSize.z ) );
	fSize = ( vSize.x * vWeights.x + vSize.y * vWeights.y + vSize.z * vWeights.z ) / ( vWeights.x + vWeights.y + vWeights.z );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelGetWorldSizeAvg( D3D_MODEL * pModel, float & fSize, D3D_MODEL_DEFINITION * pModelDefinition /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( pModel );

	VECTOR vSize = BoundingBoxGetSize( &pModel->tRenderAABBWorld );
	if ( VectorIsZero( vSize ) )
	{
		if ( pModelDefinition )
		{
			vSize = BoundingBoxGetSize( &pModelDefinition->tRenderBoundingBox );
			if ( VectorIsZero( vSize ) )
				vSize = BoundingBoxGetSize( &pModelDefinition->tBoundingBox );
			// We only need to scale the size vector by model scale from the modeldef bounding box.

			vSize = vSize * pModel->vScale;
			//vSize *= pModel->fScale;
		}
	}
	if( AppIsTugboat() )
		fSize = MAX( vSize.x, MAX( vSize.y, vSize.z ) );
	else
		fSize = ( vSize.x + vSize.y + vSize.z ) / 3.f;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.11.30 - Select appropriate LOD for model.  This is done
// only once per frame so that all renders of the model (normal,
// shadow, etc.) use a consistent LOD and bone array.
PRESULT dxC_ModelSelectLOD( D3D_MODEL * pModel, float fScreensize, float fDistanceFromEyeSquared )
{
#if ISVERSION( SERVER_VERSION )
	ASSERT(0);
	return 0;
#else
	ASSERT_RETINVALIDARG( pModel );
	int nModelDef = pModel->nModelDefinitionId;
	if (nModelDef == INVALID_ID)
	{
		return S_FALSE;
	}

	// if we are forcing the LOD, this should be set to 1 anyway
	pModel->fLODTransitionLERP = 1.f;

	// For skybox and UI, just use the highest available LOD.
	if ( dxC_ModelGetDrawLayer( pModel ) != DRAW_LAYER_GENERAL )
	{
		D3D_MODEL_DEFINITION * pModelDef = NULL;
		int nLOD;
		for ( nLOD = LOD_COUNT-1; nLOD >= 0; nLOD-- )
		{
			pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
			if ( pModelDef )
				break;
		}
		if ( ! pModelDef )
			return S_FALSE;
		pModel->nDisplayLOD = nLOD;
		return S_OK;
	}

	// CHB 2007.07.13 - Do not force display LOD in Hammer.
	// (Really the basic problem for some of these issues
	// may be that Hammer shouldn't load the game's settings
	// file.)
	int nForceLOD = LOD_NONE;
	if (!c_GetToolMode())
	{
		nForceLOD = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED )
					?	e_OptionState_GetActive()->get_nAnimatedShowForceLOD()	:	e_OptionState_GetActive()->get_nBackgroundShowForceLOD();
	}

#if ISVERSION(DEVELOPMENT)
	int nDebugForceLOD = e_GetRenderFlag( RENDER_FLAG_FORCE_MODEL_LOD );
	nForceLOD = (nDebugForceLOD != LOD_NONE) ? nDebugForceLOD : nForceLOD;

	if ( e_GetRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW ) )
		nForceLOD = LOD_HIGH;
#endif

	if ((0 <= nForceLOD) && (nForceLOD < LOD_COUNT))
	{
		D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( nModelDef, nForceLOD );
		if ( pModelDefinition )
		{
			pModel->nDisplayLOD = nForceLOD;
			return S_OK;
		}		
	}

	D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( nModelDef, LOD_HIGH );
	if ( pModelDefinition )
	{
		pModel->nDisplayLOD = LOD_HIGH;	

		pModelDefinition = dxC_ModelDefinitionGet( nModelDef, LOD_LOW );
		if ( pModelDefinition )
		{
			if ( fScreensize < 0.f )
			{
				if ( fDistanceFromEyeSquared < 0.f )
				{
					BOUNDING_BOX tBBox;
					V( dxC_GetModelRenderAABBInWorld( pModel, &tBBox ) );
					VECTOR vEye;
					e_GetViewPosition( &vEye );
					fDistanceFromEyeSquared = BoundingBoxDistanceSquared( &tBBox, &vEye );
				}
				float fSize = 0.f;
				V( dxC_ModelGetWorldSizeAvg( pModel, fSize, pModelDefinition ) );
				fSize = MAX( fSize, 1.0f );

				fScreensize = e_GetWorldDistanceBiasedScreenSizeByVertFOV( SQRT_SAFE( fDistanceFromEyeSquared ), fSize );
			}

			if ( fScreensize < pModel->fLODScreensize )
			{
				pModel->nDisplayLOD = LOD_LOW;
			}
			else
			{
				float fStartScrSize = MODEL_LOD_TRANSITION_MUL * pModel->fLODScreensize;
				pModel->fLODTransitionLERP = MAP_VALUE_TO_RANGE( fScreensize, pModel->fLODScreensize, fStartScrSize, 0.f, 1.f );
				pModel->fLODTransitionLERP = SATURATE( pModel->fLODTransitionLERP );
			}
		}
	}
	else
	{
		pModelDefinition = dxC_ModelDefinitionGet( nModelDef, LOD_LOW );
		if ( pModelDefinition )
			pModel->nDisplayLOD = LOD_LOW;
		else
			return S_FALSE;
	}	
	return S_OK;
#endif // SERVER_VERSION else
}


//-------------------------------------------------------------------------------------------------
VECTOR e_ModelGetPosition ( 
	int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return VECTOR(0.0f);
	}
	// CML 2006.10.4: Should we be doing the matrix mult each time we get position?  Obviously it doesn't make sense, but why were we doing it before?
	//VECTOR vPosition(0.0f);
	//MatrixMultiply( &vPosition, &vPosition, (MATRIX*)&pModel->matWorld );
	//return vPosition;
	return pModel->vPosition;
}

//-------------------------------------------------------------------------------------------------
VECTOR* e_ModelGetPositionPointer ( 
	int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	ASSERT( pModel );
	//VECTOR vPosition(0.0f);
	//MatrixMultiply( &vPosition, &vPosition, (MATRIX*)&pModel->matWorld );
	//return vPosition;
	return &pModel->vPosition;
}

//-------------------------------------------------------------------------------------------------
VECTOR e_ModelGetPosition ( 
	D3D_MODEL* pModel )
{
	if (!pModel)
	{
		return VECTOR(0.0f);
	}
	
	return pModel->vPosition;
}


//-------------------------------------------------------------------------------------------------
float e_ModelGetRadius ( 
	int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return 0;
	}
	return pModel->fRadius;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetWorld ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return (const MATRIX*)&pModel->matWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetWorldInverse( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return (const MATRIX*)&pModel->matWorldInverse;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetWorldScaled ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return (const MATRIX*)&pModel->matScaledWorld;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetView ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_VIEW ) ? (const MATRIX*)&pModel->matView : NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetProj ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ ) ? (const MATRIX*)&pModel->matProj : NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const MATRIX * e_ModelGetWorldScaledInverse( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return NULL;
	}

	return (const MATRIX*)&pModel->matScaledWorldInverse;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelAnimates ( int nModelId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return FALSE;
	}

	return dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
PRESULT sModelSetPendingTextureOverride( int nLOD, D3D_MODEL * pModel, TEXTURE_TYPE eType, int nTexture )
{
	ASSERT_RETINVALIDARG( pModel );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( nLOD, LOD_COUNT ) );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eType, NUM_TEXTURE_TYPES ) );

	if ( pModel->tModelDefData[nLOD].dwFlags & MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE )
	{
		ASSERT_RETFAIL( pModel->tModelDefData[nLOD].pnTextureOverrides );
		ASSERT_RETFAIL( eType < pModel->tModelDefData[nLOD].nTextureOverridesAllocated );
	}
	else
	{
		ASSERT_RETFAIL( NULL == pModel->tModelDefData[nLOD].pnTextureOverrides );
		pModel->tModelDefData[nLOD].nTextureOverridesAllocated = NUM_TEXTURE_TYPES;
		pModel->tModelDefData[nLOD].pnTextureOverrides = (int*)MALLOC( NULL, sizeof(int) * pModel->tModelDefData[nLOD].nTextureOverridesAllocated );
		ASSERT_RETVAL( pModel->tModelDefData[nLOD].pnTextureOverrides, E_OUTOFMEMORY );
		for ( int i = 0; i < pModel->tModelDefData[nLOD].nTextureOverridesAllocated; i++ )
			pModel->tModelDefData[nLOD].pnTextureOverrides[ i ] = -1;
	}

	e_TextureAddRef( nTexture );

	pModel->tModelDefData[nLOD].pnTextureOverrides[ eType ] = nTexture;
	pModel->tModelDefData[nLOD].dwFlags |= MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
PRESULT sModelResolvePendingTextureOverrides(
	D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel );

	int nTextureIDs[ NUM_TEXTURE_TYPES ];

	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		if ( 0 == ( pModel->tModelDefData[nLOD].dwFlags & MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE ) )
			continue;
		if ( 0 == e_ModelDefinitionGetMeshCount( pModel->nModelDefinitionId, nLOD ) )
			continue;

		ASSERT_CONTINUE( pModel->tModelDefData[nLOD].nTextureOverridesAllocated > 0 );
		ASSERT_CONTINUE( pModel->tModelDefData[nLOD].pnTextureOverrides );

		// We have to copy out the texture IDs here because the set override function below
		// expects a NULL pnTextureOverrides.
		ASSERT_CONTINUE(pModel->tModelDefData[nLOD].nTextureOverridesAllocated >= NUM_TEXTURE_TYPES);
		const int nCopySize = NUM_TEXTURE_TYPES * sizeof(int);
		MemoryCopy( nTextureIDs, nCopySize, pModel->tModelDefData[nLOD].pnTextureOverrides, nCopySize );
		FREE( NULL, pModel->tModelDefData[nLOD].pnTextureOverrides );
		pModel->tModelDefData[nLOD].pnTextureOverrides = NULL;
		pModel->tModelDefData[nLOD].nTextureOverridesAllocated = 0;

		pModel->tModelDefData[nLOD].dwFlags &= ~MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE;

		for ( int t = 0; t < NUM_TEXTURE_TYPES; t++ )
		{
			V_BREAK( sModelSetTextureOverride(
				nLOD,
				pModel->nModelDefinitionId,
				pModel->nId,
				INVALID_ID,
				(TEXTURE_TYPE)t,
				nTextureIDs[t],
				FALSE ) );

			e_TextureReleaseRef( nTextureIDs[t] );
		}

	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.12.08, 2006.11.28
static
PRESULT sModelSetTextureOverride( int nLOD, int nModelDef, int nModelId, int nMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride)
{
	ASSERT_RETINVALIDARG((0 <= nLOD) && (nLOD < LOD_COUNT));

	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}

	int nMeshCount = 0;
	BOOL bPending = FALSE;
	if ( nModelDef == INVALID_ID )
		bPending = TRUE;
	else
	{
		nMeshCount = e_ModelDefinitionGetMeshCount( nModelDef, nLOD );
		if ( nMeshCount == 0 )
			bPending = TRUE;
	}
	if ( bPending )
	{
		V_RETURN( sModelSetPendingTextureOverride( nLOD, pModel, eType, nTexture ) );
		return S_FALSE;
	}

	V_RETURN( sModelResolvePendingTextureOverrides( pModel ) );

	if ( !pModel->tModelDefData[nLOD].pnTextureOverrides )
	{
		ASSERT( ! pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks );

		// allocate texture override grid: [ meshcount ][ texturetype ]
		int nSize = sizeof(int) * nMeshCount * NUM_TEXTURE_TYPES;
		pModel->tModelDefData[nLOD].pnTextureOverrides = (int*)MALLOC( NULL, nSize );
		ASSERT_RETVAL( pModel->tModelDefData[nLOD].pnTextureOverrides, E_OUTOFMEMORY );
		pModel->tModelDefData[nLOD].nTextureOverridesAllocated = nMeshCount * NUM_TEXTURE_TYPES;
		//memset( pModel->pnTextureOverrides, -1, nSize );
		for ( int i = 0; i < pModel->tModelDefData[nLOD].nTextureOverridesAllocated; i++ )
		{
			pModel->tModelDefData[nLOD].pnTextureOverrides[i] = INVALID_ID;
		}

		nSize = sizeof(DWORD) * nMeshCount;
		pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks = (DWORD*)MALLOCZ( NULL, nSize );
		ASSERT_RETVAL( pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks, E_OUTOFMEMORY );
	}

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	ASSERT_RETFAIL( pModelDef );

	ASSERT_RETFAIL( pModel->tModelDefData[nLOD].nTextureOverridesAllocated >= nMeshCount * NUM_TEXTURE_TYPES );

	if ( nMeshIndex == INVALID_ID )
	{
		// set this texture for each mesh's eType slot
		for ( int i = 0; i < nMeshCount; i++ )
		{
			pModel->tModelDefData[nLOD].pnTextureOverrides[ i * NUM_TEXTURE_TYPES + eType ] = nTexture;
			e_TextureAddRef( nTexture );
			V( sMeshComputeTechniqueMaskOverride( pModel, pModelDef, i, nLOD ) );
		}
	} else {
		ASSERT_RETFAIL( nMeshIndex < nMeshCount );
		pModel->tModelDefData[nLOD].pnTextureOverrides[ nMeshIndex * NUM_TEXTURE_TYPES + eType ] = nTexture;
		e_TextureAddRef( nTexture );
		V( sMeshComputeTechniqueMaskOverride( pModel, pModelDef, nMeshIndex, nLOD ) );
	}

	// if a diffuse texture, update the mesh's material version as well to force a model material update
	if ( eType == TEXTURE_DIFFUSE && ! bNoMaterialOverride )
	{
		//pModel->dwFlags &= ~MODEL_FLAG_MATERIAL_APPLIED;
		if ( nMeshIndex >= 0 )
		{
			V( e_MeshUpdateMaterialVersion( nModelDef, nLOD, nMeshIndex ) );
		}
		else
		{
			V( e_ModelDefinitionUpdateMaterialVersions( nModelDef, nLOD ) );
		}
	}

	return S_OK;
}

PRESULT e_ModelSetTextureOverride( int nModelId, int nMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride /*= FALSE*/ )
{

	return sModelSetTextureOverride(LOD_HIGH, e_ModelGetDefinition(nModelId), nModelId, nMeshIndex, eType, nTexture, bNoMaterialOverride);
}


PRESULT e_ModelSetTextureOverride( int nModelId, const char * pszTextureName, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride /*= FALSE*/ )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelId );
	if (!pModel)
	{
		return E_INVALIDARG;
	}
	int nModelDef = e_ModelGetDefinition(nModelId);

	int nMeshCount = 0;
	BOOL bPending = FALSE;
	if ( nModelDef != INVALID_ID )
	{
		nMeshCount = e_ModelDefinitionGetMeshCount( nModelDef, LOD_HIGH );
	}
	if( nMeshCount == 0 )
	{
		return S_FALSE;
	}
	// set this texture for each mesh's eType slot
	for ( int i = 0; i < nMeshCount; i++ )
	{
		if( PStrICmp( pszTextureName, e_MeshGetTextureFilename( nModelId, LOD_HIGH, i, eType ) ) == 0 )
		{
			sModelSetTextureOverride(LOD_HIGH, nModelDef, nModelId, i, eType, nTexture, bNoMaterialOverride);
		}
	}
	return S_OK;
}

// CHB 2006.11.28
PRESULT e_ModelSetTextureOverrideLOD ( int nModelId, TEXTURE_TYPE eType, const int pnTexture[], BOOL bNoMaterialOverride)
{
	// CHB 2006.12.21 - Individual meshes not supported by this function.
	// (Because the mesh indices aren't the same for different model definitions.)
//	trace("CHB**e_ModelSetTextureOverrideLOD() model=%d, type=%d, high_tex=%d, low_tex=%d\n", nModelId, eType, pnTexture[0], pnTexture[1]);
	int nModelDef = e_ModelGetDefinition(nModelId);
//	trace("CHB**e_ModelSetTextureOverrideLOD() high_def=%d, low_def=%d\n", nModelDef, e_ModelDefinitionGetLODNext(nModelDef));
	for (int j = 0; j < LOD_COUNT; ++j)
	{
		//D3D_MODEL_DEFINITION* pModelDef = dxC_ModelDefinitionGet( nModelDef, j );
		//if ( !pModelDef )
		//	continue;

		if (pnTexture[j] != INVALID_ID)
		{
			V( sModelSetTextureOverride(j, nModelDef, nModelId, INVALID_ID, eType, pnTexture[j], bNoMaterialOverride) );
		}
	}

	return S_OK;
}

// CML 2007.02.23
PRESULT e_ModelSetTextureOverrideLODMesh( int nModelId, int nLOD, int nLODMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride)
{
	//	trace("CHB**e_ModelSetTextureOverrideLODMesh() model=%d, type=%d, high_tex=%d, low_tex=%d\n", nModelId, eType, pnTexture[0], pnTexture[1]);
	int nModelDef = e_ModelGetDefinition(nModelId);
	//	trace("CHB**e_ModelSetTextureOverrideLODMesh() high_def=%d, low_def=%d\n", nModelDef, e_ModelDefinitionGetLODNext(nModelDef));

	ASSERTX_RETINVALIDARG( nModelDef != INVALID_ID, "Invalid model passed in!" )
	ASSERTX_RETINVALIDARG( dxC_ModelDefinitionGet( nModelDef, nLOD ), "Invalid LOD level passed in!" )

	V_RETURN( sModelSetTextureOverride(
				nLOD,
				nModelDef,
				nModelId,
				nLODMeshIndex,
				eType,
				nTexture,
				bNoMaterialOverride) );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_IsValidModelID( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	return ( pModel != NULL );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelUpdateIllumination( int nModelID, 
								float fTimeDelta, 
								BOOL bFullbright, 
								BOOL bDrawBehind, 
								BOOL bHit,
								float fAmbientLight,
								float fBehindColor )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return;
	}
	e_ModelSetFullbright( pModel, bFullbright );
	e_ModelSetAmbientLight( pModel, fAmbientLight, fTimeDelta );
	e_ModelSetHit( pModel, bHit, fTimeDelta );
	e_ModelSetBehindColor( pModel, fBehindColor );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetIllumination( int nModelID, float fIllumination, float fFactor )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return;
	}

	fIllumination = max( fIllumination, 0.0f );
	pModel->fSelfIlluminationValue  = fIllumination;
	fFactor = min( fFactor, 1.0f );
	fFactor = max( fFactor, 0.0f );
	pModel->fSelfIlluminationFactor = fFactor;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetFullbright( D3D_MODEL * pModel, BOOL bFullbright )
{
	if ( !pModel )
	{
		return;
	}

	pModel->bFullbright = bFullbright;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetDrawBehind( int nModelID, BOOL bDrawBehind )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return;
	}

	dxC_ModelSetDrawBehind( pModel, bDrawBehind );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_ModelSetDrawBehind( D3D_MODEL * pModel, BOOL bDrawBehind )
{

	if ( !pModel )
	{
		return;
	}

	dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_DRAWBEHIND, bDrawBehind );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_ModelGetMaterialOverride( const D3D_MODEL * pModel, MATERIAL_OVERRIDE_TYPE eMatOverrideType /*= MATERIAL_OVERRIDE_NORMAL*/ )
{
	ASSERT_RETINVALID( pModel );
	
	// look for whole-model material override first
	MATERIAL_OVERRIDE_TYPE eOverrideType = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CLONE ) ? eMatOverrideType : MATERIAL_OVERRIDE_NORMAL;
	int nOverrideIndex = pModel->nStateMaterialOverrideIndex[ eOverrideType ];
	if ( nOverrideIndex >= 0 )
	{
		ASSERTX_RETINVALID( nOverrideIndex < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "state material override stack index out of bounds");
		if ( pModel->pStateMaterialOverride[ eOverrideType ][ nOverrideIndex ].nMaterialId != INVALID_ID )
			return pModel->pStateMaterialOverride[ eOverrideType ][ nOverrideIndex ].nMaterialId;
	}

	// look for diffuse texture override second
	if ( pModel->nTextureMaterialOverride != INVALID_ID )
		return pModel->nTextureMaterialOverride;

	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_ModelGetMaterialOverrideAttachment( const D3D_MODEL * pModel, MATERIAL_OVERRIDE_TYPE eMatOverrideType /*= MATERIAL_OVERRIDE_NORMAL*/ )
{
	ASSERT_RETINVALID( pModel );

	// look for whole-model material override first
	MATERIAL_OVERRIDE_TYPE eOverrideType = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CLONE ) ? eMatOverrideType : MATERIAL_OVERRIDE_NORMAL;
	int nOverrideIndex = pModel->nStateMaterialOverrideIndex[ eOverrideType ];
	if ( nOverrideIndex >= 0 )
	{
		ASSERTX_RETINVALID( nOverrideIndex < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "state material override stack index out of bounds");
		if ( pModel->pStateMaterialOverride[ eOverrideType ][ nOverrideIndex ].nMaterialId != INVALID_ID )
			return pModel->pStateMaterialOverride[ eOverrideType ][ nOverrideIndex ].nAttachmentId;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sMaterialOverrideCreateAttachment( 
	 D3D_MODEL * pModel,
	 MATERIAL_OVERRIDE_TYPE eType,
	 MATERIAL * pMaterial )
{
	ASSERT_RETINVALIDARG( pModel );

	int nOverrideIndex = pModel->nStateMaterialOverrideIndex[ eType ];
	ASSERTX_RETFAIL( nOverrideIndex < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "state material override stack index out of bounds");
	if ( nOverrideIndex < 0 || pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId != INVALID_ID )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pMaterial );

	if ( pMaterial->nParticleSystemDefId != INVALID_ID &&
		pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId == INVALID_ID )
	{
		ATTACHMENT_DEFINITION tAttachmentDef;
		tAttachmentDef.nAttachedDefId = pMaterial->nParticleSystemDefId;
		tAttachmentDef.eType = ATTACHMENT_PARTICLE;

		ASSERT_RETFAIL( gpfn_AttachmentNew );
		pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId = gpfn_AttachmentNew( pModel->nId, &tAttachmentDef );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRemoveMaterialOverride( 
	D3D_MODEL * pModel,
	MATERIAL_OVERRIDE_TYPE eType,
	int nMaterial )
{
	ASSERT_RETINVALIDARG( nMaterial >= 0 );
	ASSERT_RETINVALIDARG( pModel );

	int nStackTopIndex = pModel->nStateMaterialOverrideIndex[ eType ];
	ASSERTX_RETFAIL( nStackTopIndex < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "state material override stack index out of bounds");
	// remove the material matching the argument that is highest on the stack (in theory this could cause issues with multiple mat overrides of the same mat in the stack)
	for ( int i = nStackTopIndex; i >= 0; --i )
	{
		if ( pModel->pStateMaterialOverride[ eType ][ i ].nMaterialId == nMaterial )
		{
			ASSERTX_RETFAIL(pModel->nStateMaterialOverrideIndex[ eType ] >= 0, "material override stack underflow");
			if ( pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId != INVALID_ID )
			{
				gpfn_AttachmentRemove( pModel->nId, pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId, 0 );
			}

			if ( i == nStackTopIndex )
			{
				// removing from top of stack
				pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId = INVALID_ID;
				pModel->pStateMaterialOverride[ eType ][ i ].nMaterialId = INVALID_ID;
			}
			else
			{
				// removing item from stack that isn't on top, move everything else down
				for ( int j = i; j + 1 <= nStackTopIndex; ++j )
				{
					pModel->pStateMaterialOverride[ eType ][ j ].nAttachmentId = pModel->pStateMaterialOverride[ eType ][ j + 1 ].nAttachmentId;
					pModel->pStateMaterialOverride[ eType ][ j ].nMaterialId = pModel->pStateMaterialOverride[ eType ][ j + 1 ].nMaterialId;
				}
			}
			--pModel->nStateMaterialOverrideIndex[ eType ];

			// if all clone material overrides are removed, reset the clone flag
			if ( eType == MATERIAL_OVERRIDE_CLONE && 
				 pModel->nStateMaterialOverrideIndex[ eType ] < 0 )
				dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CLONE, FALSE );

			return S_OK;
		}
	}

	return S_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddMaterialOverride( 
	 D3D_MODEL * pModel,
	 MATERIAL_OVERRIDE_TYPE eType,
	 int nMaterial )
{
	ASSERT_RETINVALIDARG( nMaterial >= 0 );
	ASSERT_RETINVALIDARG( pModel );

	// since the same state won't stack on itself, need to remove any existing
	// material overrides for that state, and then add the material override
	// again so it's on top of the stack
	sRemoveMaterialOverride( pModel, eType, nMaterial );

	int nOverrideIndex = pModel->nStateMaterialOverrideIndex[ eType ];

	ASSERTX_RETFAIL( nOverrideIndex + 1 < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "too many state material overrides, increase MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH");
	if ( nOverrideIndex >= 0 && pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId != INVALID_ID )
	{
		gpfn_AttachmentRemove( pModel->nId, pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId, 0 );
		pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId = INVALID_ID;
	}

	// push new material override on to the top of the stack
	nOverrideIndex = ++pModel->nStateMaterialOverrideIndex[ eType ];
	ASSERTX_RETFAIL( nOverrideIndex >= 0, "state material override stack underflow");
	pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nMaterialId = nMaterial;
	pModel->pStateMaterialOverride[ eType ][ nOverrideIndex ].nAttachmentId = INVALID_ID;

	if ( eType == MATERIAL_OVERRIDE_CLONE )
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CLONE, TRUE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRemoveAllMaterialOverrides( 
	D3D_MODEL * pModel,
	MATERIAL_OVERRIDE_TYPE eType )
{
	int nStackTopIndex = pModel->nStateMaterialOverrideIndex[ eType ];
	ASSERTX_RETFAIL( nStackTopIndex < MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH, "state material override stack index out of bounds");
	for ( int i = nStackTopIndex; i >= 0; --i )
	{
		if ( pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId != INVALID_ID )
		{
			gpfn_AttachmentRemove( pModel->nId, pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId, 0 );
			pModel->pStateMaterialOverride[ eType ][ i ].nAttachmentId = INVALID_ID;
		}
		pModel->pStateMaterialOverride[ eType ][ i ].nMaterialId = INVALID_ID;
	}
	pModel->nStateMaterialOverrideIndex[ eType ] = -1;
	if ( eType == MATERIAL_OVERRIDE_CLONE )
		dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CLONE, FALSE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelAddMaterialOverride( int nModelID, int nMaterial )
{
	ASSERT_RETINVALIDARG( nMaterial >= 0 );
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	return sAddMaterialOverride( pModel, MATERIAL_OVERRIDE_NORMAL, nMaterial );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelRemoveMaterialOverride( int nModelID, int nMaterial )
{
	ASSERT_RETINVALIDARG( nMaterial >= 0 );
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	return sRemoveMaterialOverride( pModel, MATERIAL_OVERRIDE_NORMAL, nMaterial );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelRemoveAllMaterialOverrides( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	return sRemoveAllMaterialOverrides( pModel, MATERIAL_OVERRIDE_NORMAL );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelSetEnvironmentOverride( int nModelID, int nEnvDefId )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
		return E_INVALIDARG;

	if ( nEnvDefId != INVALID_ID )
	{
		ENVIRONMENT_DEFINITION* pEnvDef = (ENVIRONMENT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvDefId );
		ASSERT( pEnvDef );
	}

	pModel->nEnvironmentDefOverride = nEnvDefId;

	return S_OK;
}	

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ModelActivateClone( int nModelId, int nMaterialOverride )
{
	ASSERT_RETINVALIDARG( nMaterialOverride >= 0 );
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	if ( ! pModel )
		return E_INVALIDARG;

	V_RETURN( sAddMaterialOverride( pModel, MATERIAL_OVERRIDE_CLONE, nMaterialOverride ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelDeactivateClone( int nModelId, int nMaterialOverride )
{
	ASSERT_RETINVALIDARG( nMaterialOverride >= 0 );
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	if ( ! pModel )
		return E_INVALIDARG;

	V_RETURN( sRemoveMaterialOverride( pModel, MATERIAL_OVERRIDE_CLONE, nMaterialOverride ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelDeactivateAllClones( int nModelId )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	if ( ! pModel )
		return E_INVALIDARG;

	V_RETURN( sRemoveAllMaterialOverrides( pModel, MATERIAL_OVERRIDE_CLONE ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ModelSetAmbientLight( D3D_MODEL * pModel, float AmbientLight, float TimeDelta )
{
	if ( !pModel )
	{
		return;
	}

	if( AmbientLight > pModel->AmbientLight  )
	{
		pModel->AmbientLight += TimeDelta * 4.0f;
		if( pModel->AmbientLight > AmbientLight )
		{
			pModel->AmbientLight = AmbientLight;
		}
	}
	else if( AmbientLight < pModel->AmbientLight )
	{
		pModel->AmbientLight -= TimeDelta * 6.0f;
		if( pModel->AmbientLight < AmbientLight )
		{
			pModel->AmbientLight = AmbientLight;
		}
	}
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_ModelGetAmbientLight( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return 1;
	}
	return pModel->AmbientLight;
}

float e_ModelGetHit( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return 1;
	}
	return pModel->HitColor;
}


float e_ModelGetBehindColor( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel )
	{
		return 0;
	}
	return pModel->fBehindColor;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetHit( D3D_MODEL * pModel, BOOL Hit, float TimeDelta )
{
	if ( !pModel )
	{
		return;
	}
	
	pModel->HitColor -= TimeDelta * 3.0f;
	if( pModel->HitColor < 0 )
	{
		pModel->HitColor = 0;
	}
	if( Hit )
	{
		pModel->HitColor = 1;
	}
	
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelSetBehindColor( D3D_MODEL * pModel, float fBehindColor )
{
	if ( !pModel )
	{
		return;
	}

	pModel->fBehindColor = fBehindColor;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelLazyUpdate( D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel );

	MODEL_DEFINITION_HASH * pModelDefHash = HashGet( gtModelDefinitions, pModel->nModelDefinitionId );

	if ( pModelDefHash )
	{
		if ( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MLI_DEF_APPLIED ) )
		{
			V( sUpdateModelLightsFromDefinition( pModel, pModelDefHash->pSharedDef ) );
		}

		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED ) )
		{
			V( sUpdateModelLightsFromDefinition( pModel, pModelDefHash->pSharedDef ) );

			for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
			{
				D3D_MODEL_DEFINITION* pModelDef = pModelDefHash->pDef[ nLOD ];
				if ( pModelDef )
				{
					V( e_ModelSetDefinition( pModel->nId, pModel->nModelDefinitionId, nLOD ) );
					V( sUpdateModelAdObjectsFromDefinition( pModel, pModelDef ) );
					V( sUpdateModelBBoxFromDefinition( pModel, pModelDef ) );
				}
			}
		}

		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_SETLOCATION ) )
		{
			e_ModelSetLocationPrivate( pModel->nId, (MATRIX *)&pModel->matWorld, (const VECTOR &)pModel->vPosition, pModel->m_SortType, TRUE, FALSE );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ModelsUpdate( )
{
	D3D_MODEL * pModel = HashGetFirst( gtModels );
	while ( pModel )
	{

		MODEL_DEFINITION_HASH * pModelDefHash = HashGet( gtModelDefinitions, pModel->nModelDefinitionId );
		if ( pModelDefHash )
		{
			if ( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MLI_DEF_APPLIED ) )
			{
				V( sUpdateModelLightsFromDefinition( pModel, pModelDefHash->pSharedDef ) );
			}

			if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED ) )
			{
					V( sUpdateModelLightsFromDefinition( pModel, pModelDefHash->pSharedDef ) );

					for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
					{
						D3D_MODEL_DEFINITION* pModelDef = pModelDefHash->pDef[ nLOD ];
						if ( pModelDef )
						{
							V( e_ModelSetDefinition( pModel->nId, pModel->nModelDefinitionId, nLOD ) );
							V( sUpdateModelAdObjectsFromDefinition( pModel, pModelDef ) );
							V( sUpdateModelBBoxFromDefinition( pModel, pModelDef ) );
						}
					}
			}
		}

		pModel = HashGetNext( gtModels, pModel );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ModelAddToProxMap(
	D3D_MODEL * pModel,
	BOOL bStaticModel /*=FALSE*/ )
{
	ASSERT_RETINVALIDARG( pModel && pModel->nProxmapId == INVALID_ID );
	BOUNDING_BOX tBox;
	V( dxC_GetModelRenderAABBInWorld( pModel, &tBox ) );
	float xr = fabsf( tBox.vMax.x - tBox.vMin.x );
	float yr = fabsf( tBox.vMax.y - tBox.vMin.y );
	float zr = fabsf( tBox.vMax.z - tBox.vMin.z );
	DWORD dwFlags = bStaticModel ? PROX_FLAG_STATIC : 0;
	//pModel->nProxmapId = gtModelProxMap.Insert( pModel->nId, dwFlags, pModel->vPosition.x, pModel->vPosition.y, pModel->vPosition.z, xr, yr, zr );
	pModel->nProxmapId = gtModelProxMap.Insert( pModel->nId, dwFlags, pModel->vPosition.x, pModel->vPosition.y, pModel->vPosition.z );
#ifdef TRACE_MODEL_PROXMAP_ACTIONS
	trace( "---- Proxmap insert [ %4d ] [ %4d ] [ 0x%08x ]\n", pModel->nProxmapId, pModel->nId, pModel );
#endif // TRACE_MODEL_PROXMAP_ACTIONS
	ASSERT_RETFAIL( pModel->nProxmapId != INVALID_ID );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ModelMoveInProxmap(
	D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel && pModel->nProxmapId != INVALID_ID );
	gtModelProxMap.Move( pModel->nProxmapId, pModel->vPosition.x, pModel->vPosition.y, pModel->vPosition.z );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ModelRemoveFromProxmap(
	D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel );
	if ( pModel->nProxmapId == INVALID_ID )
		return S_FALSE;
#ifdef TRACE_MODEL_PROXMAP_ACTIONS
	trace( "---- Proxmap delete [ %4d ] [ %4d ] [ 0x%08x ]\n", pModel->nProxmapId, pModel->nId, pModel );
#endif // TRACE_MODEL_PROXMAP_ACTIONS
	gtModelProxMap.Delete( pModel->nProxmapId );
	pModel->nProxmapId = INVALID_ID;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_ModelTexturesPreload( D3D_MODEL * pModel )
{
	ASSERT_RETINVALIDARG( pModel );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId,
											e_ModelGetDisplayLOD( pModel->nId ) );
	if ( ! pModelDef )
		return S_FALSE;

	for ( int nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	{
		// could end up preload()ing some textures more than once, but that shouldn't hurt anything
		for ( int nType = 0; nType < NUM_TEXTURE_TYPES; nType++ )
		{
			int nTextureID = dx9_ModelGetTexture( pModel, pModelDef, NULL, nMesh, (TEXTURE_TYPE)nType, FALSE, NULL );
			V( e_TexturePreload( nTextureID ) );
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelTexturesPreload( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( pModel )
		return dx9_ModelTexturesPreload( pModel );
	return E_INVALIDARG;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelSetBaseSHCoefs( int nModelID, float fIntensity, SH_COEFS_RGB<2> & tCoefsO2, SH_COEFS_RGB<3> & tCoefsO3, SH_COEFS_RGB<2> * pCoefsLinO2, SH_COEFS_RGB<3> * pCoefsLinO3 )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return E_INVALIDARG;

	V( dx9_SHCoefsScale( &pModel->tBaseSHCoefs_O2, &tCoefsO2, fIntensity ) );
	V( dx9_SHCoefsScale( &pModel->tBaseSHCoefs_O3, &tCoefsO3, fIntensity ) );
	if ( pCoefsLinO2 )
	{
		V( dx9_SHCoefsScale( &pModel->tBaseSHCoefsLin_O2, pCoefsLinO2, fIntensity ) );
	}
	else
		pModel->tBaseSHCoefsLin_O2 = pModel->tBaseSHCoefs_O2;
	if ( pCoefsLinO3 )
	{
		V( dx9_SHCoefsScale( &pModel->tBaseSHCoefsLin_O3, pCoefsLinO3, fIntensity ) );
	}
	else
		pModel->tBaseSHCoefsLin_O3 = pModel->tBaseSHCoefs_O3;

	return dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_HAS_BASE_SH, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelClearBaseSHCoefs( int nModelID )
{
	return e_ModelSetFlagbit( nModelID, MODEL_FLAGBIT_HAS_BASE_SH, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDumpList()
{
#if ISVERSION(DEVELOPMENT)
	trace( "\n--- MODEL LIST ---\n" );

	trace( "-- Model Definitions:\n" );
	trace( " ID    | REFS | PTR | MESH | VBs | DPT | DPW | LOD | MLI | FILE\n" );

	for ( MODEL_DEFINITION_HASH * pModelDefHash = dx9_ModelDefinitionGetFirst();
		pModelDefHash;
		pModelDefHash = dx9_ModelDefinitionGetNext( pModelDefHash ) )
	{
		int nRef = pModelDefHash->tRefCount.GetCount();
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			D3D_MODEL_DEFINITION* pDef = pModelDefHash->pDef[ nLOD ];

			if ( nLOD == 0 )
				trace( " %-5d | %4d | ", pModelDefHash->nId, nRef );
			else
				trace( " %5s | %4s | ", "", "" );

			trace( "%3s | %4d | %3d | %3s | %3s | %3d | %3s | [ %s ]",
				pDef ? "yes" : "no",
				pDef ? pDef->nMeshCount : -1,
				pDef ? pDef->nVertexBufferDefCount : -1,
				( pDef && pDef->pCullTestModel ) ? "yes" : "no",
				( pDef && pDef->pCullWriteModel ) ? "yes" : "no",
				nLOD,
				( pModelDefHash->pSharedDef ) ? "yes" : "no",
				pDef ? pDef->tHeader.pszName : "" );
			trace( "\n" );
		}
	}


	trace( "\n-- Models:\n" );
	trace( " ID    | DEF  | LOD | FLAGS 0  | FLAGS 1  | RGN | DPO | DEF NAME\n" );

	for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
		pModel;
		pModel = dx9_ModelGetNext( pModel ) )
	{
		int nDisplayLOD = e_ModelGetDisplayLOD( pModel->nId );
		trace( " %-5d | %-4d | %3d | %08X | %08X | %-3d | %3s |",
			pModel->nId,
			pModel->nModelDefinitionId,
			nDisplayLOD,
			pModel->dwFlagBits[0],
			0, //pModel->dwFlagBits[1],
			pModel->nRegion,
			pModel->pCullObject ? "yes" : "no" );

		D3D_MODEL_DEFINITION* pDef = dxC_ModelDefinitionGet( 
										pModel->nModelDefinitionId, 
										nDisplayLOD );
		if ( pDef )
		{
			trace( " [ %s ]", pDef->tHeader.pszName );
		}
		trace( "\n" );
	}

	trace( "---\n" );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelSetFeatureRequestFlag( int nModelID, FEATURES_FLAG eFlag, BOOL bValue )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	pModel->RequestFeatures.SetFlag(eFlag, !!bValue);
	return S_OK;
}

BOOL e_ModelGetFeatureRequestFlag( int nModelID, FEATURES_FLAG eFlag )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return FALSE;

	return pModel->RequestFeatures.GetFlag(eFlag);
}

PRESULT e_ModelSetFeatureForceFlag( int nModelID, FEATURES_FLAG eFlag, BOOL bValue )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	pModel->ForceFeatures.SetFlag(eFlag, !!bValue);
	return S_OK;
}

BOOL e_ModelGetFeatureForceFlag( int nModelID, FEATURES_FLAG eFlag )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return FALSE;

	return pModel->ForceFeatures.GetFlag(eFlag);
}

PRESULT e_ModelFeaturesGetEffective( int nModelID, class Features & tFeatures )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	extern Features e_gFeaturesGlobalSelected;
	extern Features e_gFeaturesGlobalAllowed;

	tFeatures = ((pModel->RequestFeatures & e_gFeaturesGlobalSelected) | pModel->ForceFeatures) & e_gFeaturesGlobalAllowed;

	return S_OK;
}

// ************************************************************************************************
//
//  --- Mesh ---
//
// ************************************************************************************************

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR * dx9_GetMeshRenderOBBInWorld( const D3D_MODEL * pModel, int nLOD, int nMeshIndex )	// CHB 2006.12.08
{
	ASSERT_RETNULL( pModel );

	if ( nLOD == LOD_ANY )
		nLOD = dxC_ModelGetFirstLOD( pModel );

	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );	// CHB 2006.12.08
	if ( ! pModelDef )
		return NULL;
	ASSERT_RETNULL( nMeshIndex >= 0 && nMeshIndex < pModelDef->nMeshCount );

	if ( pModel->tModelDefData[nLOD].ptMeshRenderOBBWorld )
	{
		return &pModel->tModelDefData[nLOD].ptMeshRenderOBBWorld[ nMeshIndex * OBB_POINTS ];
	}

	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

VECTOR * e_GetMeshRenderOBBInWorld( int nModelID, int nLOD, int nMeshIndex )
{
	D3D_MODEL* pModel = HashGet( gtModels, nModelID );
	ASSERT_RETNULL( pModel );
	return dx9_GetMeshRenderOBBInWorld( pModel, nLOD, nMeshIndex );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_MeshFree( D3D_MESH_DEFINITION * pMesh )
{
	ASSERT_RETINVALIDARG( pMesh );

	// free all point elements
	FREE( NULL, pMesh->tIndexBufferDef.pLocalData );
	pMesh->tIndexBufferDef.pLocalData = NULL;

	V( dxC_IndexBufferDestroy( pMesh->tIndexBufferDef.nD3DBufferID ) );

	if ( pMesh->ptTechniqueCache )
	{
		FREE ( NULL, pMesh->ptTechniqueCache );
		pMesh->ptTechniqueCache = NULL;
	}

	FREE( NULL, pMesh );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_MeshSetRender( int nMeshId, WORD nVertexStart, WORD wTriangleCount )
{
	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( nMeshId );
	ASSERT_RETINVALIDARG( pMesh );
	pMesh->nVertexStart  = nVertexStart;
	pMesh->wTriangleCount = wTriangleCount;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dx9_MeshGetMaterial ( D3D_MODEL * pModel, const D3D_MESH_DEFINITION * pMesh, MATERIAL_OVERRIDE_TYPE eMatOverrideType /*= MATERIAL_OVERRIDE_NORMAL*/ )
{
	ASSERT_RETNULL( pModel );
	ASSERT_RETNULL( pMesh );

	int nMaterial = dx9_ModelGetMaterialOverride( pModel, eMatOverrideType );
	if ( nMaterial != INVALID_ID )
	{
		MATERIAL * pMat = (MATERIAL*) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterial );
		if ( pMat && TESTBIT_DWORD(pMat->tHeader.dwFlags, DHF_LOADED_BIT) )
		{
			sMaterialOverrideCreateAttachment( pModel, eMatOverrideType, pMat );
			return nMaterial;
		}
	}

	// fall back to mesh base material
	return pMesh->nMaterialId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_MeshGetMaterial ( int nModelId, int nMeshId )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	ASSERT_RETINVALID( pModel );
	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( nMeshId );
	ASSERT_RETINVALID( pMesh );
	return dx9_MeshGetMaterial( pModel, pMesh );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_MeshRefreshMaterial( D3D_MESH_DEFINITION * pMeshDef, int nDiffuseTexture )
{
	ASSERT_RETINVALIDARG( pMeshDef );
	if ( nDiffuseTexture == INVALID_ID )
		return S_FALSE;

	D3D_TEXTURE * pTex = dxC_TextureGet( nDiffuseTexture );
	if ( !pTex )
		return S_FALSE;
	if ( pTex->nDefinitionId == INVALID_ID )
		return S_FALSE;
	TEXTURE_DEFINITION * pTexDef = (TEXTURE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTex->nDefinitionId );
	//ASSERT_RETURN( pTexDef );

	if ( pMeshDef->dwFlags & MESH_FLAG_SKINNED && PStrCmp( pTexDef->pszMaterialName, "Default" ) == 0 )
	{
		V_RETURN( e_MaterialNew( &pMeshDef->nMaterialId, "Animated" ) );
	} else {
		V_RETURN( e_MaterialNew( &pMeshDef->nMaterialId, pTexDef->pszMaterialName ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_MeshesRefreshMaterial()
{
	// iterate all meshes
	//     for the ones using the old material specified
	//         refresh material

	for ( MESH_DEFINITION_HASH * pHash = HashGetFirst( gtMeshes );
		pHash;
		pHash = HashGetNext( gtMeshes, pHash ) )
	{
		D3D_MESH_DEFINITION * pMeshDef = pHash->pDef;
		if ( ! pMeshDef )
			continue;

		V_CONTINUE( dx9_MeshRefreshMaterial( pMeshDef, pMeshDef->pnTextures[ TEXTURE_DIFFUSE ] ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const char* e_MeshGetTextureFilename ( int nModelID, int nLOD, int nMesh, TEXTURE_TYPE eType )
{
	ASSERT_RETNULL( nMesh >= 0 );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( e_ModelGetDefinition( nModelID ), nLOD );
	ASSERT_RETNULL( pModelDef );
	ASSERT_RETNULL( nMesh < pModelDef->nMeshCount );
	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
	ASSERT_RETNULL( pMesh );
	return pMesh->pszTextures[ eType ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_MeshSetTexture ( int nMeshId, TEXTURE_TYPE eType, int nTextureId )
{
	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( nMeshId );
	ASSERT_RETINVALIDARG( pMesh );
	e_TextureReleaseRef( pMesh->pnTextures[ eType ] );
	pMesh->pnTextures[ eType ] = nTextureId;
	e_TextureAddRef( pMesh->pnTextures[ eType ] );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


static void sMeshComputeTextureTechniqueMask( TEXTURE_TYPE eType, int nTexture, DWORD * pdwTechniqueFlags )
{
	if ( nTexture == INVALID_ID )
		return;

	switch ( eType )
	{
	case TEXTURE_NORMAL:
		*pdwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_NORMALMAP;
		break;
	case TEXTURE_SELFILLUM:
		*pdwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_SELFILLUM;
		break;
	case TEXTURE_DIFFUSE2:
		*pdwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_DIFFUSEMAP2;
		break;
	case TEXTURE_LIGHTMAP:
		*pdwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_LIGHTMAP;
		break;
	}
}


static void sMeshComputeTechniqueMask( D3D_MESH_DEFINITION * pMesh, DWORD * pdwTechniqueFlags = NULL )
{
	pMesh->dwTechniqueFlags = 0;

	sMeshComputeTextureTechniqueMask( TEXTURE_NORMAL,	pMesh->pnTextures[ TEXTURE_NORMAL ],	&pMesh->dwTechniqueFlags );
	sMeshComputeTextureTechniqueMask( TEXTURE_SELFILLUM,pMesh->pnTextures[ TEXTURE_SELFILLUM ],	&pMesh->dwTechniqueFlags );
	sMeshComputeTextureTechniqueMask( TEXTURE_DIFFUSE2,	pMesh->pnTextures[ TEXTURE_DIFFUSE2 ],	&pMesh->dwTechniqueFlags );
	sMeshComputeTextureTechniqueMask( TEXTURE_LIGHTMAP, pMesh->pnTextures[ TEXTURE_LIGHTMAP ],	&pMesh->dwTechniqueFlags );

	if ( pMesh->dwFlags & MESH_FLAG_SKINNED )
		pMesh->dwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_SKINNED;
	if ( pMesh->eVertType == VERTEX_DECL_RIGID_32 )
		pMesh->dwTechniqueFlags |= TECHNIQUE_FLAG_MODEL_32BYTE_VERTEX;
}

// CHB 2006.11.28
static PRESULT sMeshComputeTechniqueMaskOverride( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef, int nMeshIndex, int nLOD )
{
	ASSERT_RETINVALIDARG( pModel );
	ASSERT_RETINVALIDARG( pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks );	// CHB 2006.12.08, 2006.11.29
	ASSERT_RETINVALIDARG( pModelDef );
	ASSERT_RETINVALIDARG( nMeshIndex >= 0 );
	ASSERT_RETINVALIDARG( nMeshIndex < pModelDef->nMeshCount );

	// Hey Chuck, pModel->pdwMeshTechniqueMasks[nLOD] is NULL when you do /monster minion melee in soundfield.  This causes a crash
	// so, I'm disabling this code until you can fix it.  - Tyler
	// CHB 2006.11.29 - Sorry about that!
	//return;

	pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMeshIndex ] = 0;	// CHB 2006.12.08, 2006.11.29 - DOH!

	sMeshComputeTextureTechniqueMask(	TEXTURE_NORMAL,
										dx9_ModelGetTextureOverride( pModel, pModelDef, nMeshIndex, TEXTURE_NORMAL, nLOD ),
										&pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMeshIndex ] );
	sMeshComputeTextureTechniqueMask(	TEXTURE_SELFILLUM,
										dx9_ModelGetTextureOverride( pModel, pModelDef, nMeshIndex, TEXTURE_SELFILLUM, nLOD ),
										&pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMeshIndex ] );
	sMeshComputeTextureTechniqueMask(	TEXTURE_DIFFUSE2,
										dx9_ModelGetTextureOverride( pModel, pModelDef, nMeshIndex, TEXTURE_DIFFUSE2, nLOD ),
										&pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMeshIndex ] );
	sMeshComputeTextureTechniqueMask(	TEXTURE_LIGHTMAP,
										dx9_ModelGetTextureOverride( pModel, pModelDef, nMeshIndex, TEXTURE_LIGHTMAP, nLOD ),
										&pModel->tModelDefData[nLOD].pdwMeshTechniqueMasks[ nMeshIndex ] );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelDefinitionUpdateMeshTechniqueMasks( D3D_MODEL_DEFINITION * pModelDef )
{
	if ( ! pModelDef )
		return S_FALSE;

	for ( int nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMeshDef = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
		ASSERT_CONTINUE( pMeshDef );
		sMeshComputeTechniqueMask( pMeshDef );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelDefinitionUpdateMeshTechniqueMasks( int nModelDef, int nLOD )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );

	return dxC_ModelDefinitionUpdateMeshTechniqueMasks( pModelDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_MeshRestore ( D3D_MESH_DEFINITION * pMesh )
{
	ASSERT_RETFAIL( pMesh );

	if ( pMesh->tIndexBufferDef.pLocalData && pMesh->tIndexBufferDef.nBufferSize > 0 )
	{
		pMesh->tIndexBufferDef.tFormat = D3DFMT_INDEX16;
//#ifdef DX9_MOST_BUFFERS_MANAGED
		pMesh->tIndexBufferDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;
//#else
//		pMesh->tIndexBufferDef.tUsage = D3DC_USAGE_BUFFER_MUTABLE;
//#endif
		V_DO_SUCCEEDED( dxC_CreateIndexBuffer( pMesh->tIndexBufferDef ) )
		{
			V( dxC_UpdateBuffer( pMesh->tIndexBufferDef, pMesh->tIndexBufferDef.pLocalData, 0, pMesh->tIndexBufferDef.nBufferSize ) );
			V( dxC_IndexBufferPreload( pMesh->tIndexBufferDef.nD3DBufferID ) );
		}
	}
	sMeshComputeTechniqueMask( pMesh );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL dx9_MeshIsVisible( D3D_MODEL * NOALIAS pModel, const D3D_MESH_DEFINITION * const NOALIAS pMesh )
{
	// CHB 2006.08.21 - This is the #1 reason for rejection.
	// don't draw collision and other layout stuff - toggle this for debugging
	if ( (pMesh->dwFlags & MESH_FLAG_DEBUG) != 0 && ! e_GetRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW ) )
		return FALSE;

	// CHB 2006.08.21 - This is the #2 reason for rejection.
	if ( (pMesh->dwFlags & MESH_FLAG_CLIP) != 0 && ! e_GetRenderFlag( RENDER_FLAG_CLIP_MESH_DRAW ) )
		return FALSE;

	// CHB 2006.08.21 - This is the #3 reason for rejection.
	if ( (pMesh->dwFlags & MESH_FLAG_HULL) != 0 && ! e_GetRenderFlag( RENDER_FLAG_HULL_MESH_DRAW ) )
		return FALSE;

	int nMatID = dx9_MeshGetMaterial( pModel, pMesh );
	// diffuse texture isn't loaded yet... so no material... so no drawing..
	if ( nMatID == INVALID_ID )
		return FALSE;

	// CHB 2006.08.21 - This is the #4 reason for rejection.
	if ( pMesh->nMaterialId == nMatID )
	{
		if ( 0 == ( pMesh->dwFlags & MESH_FLAG_MATERIAL_APPLIED ) )
			return FALSE;
	}

	if ( pMesh->dwFlags & MESH_FLAG_NODRAW )
		return FALSE;

	//if ( (pMesh->dwFlags & MESH_FLAG_COLLISION) != 0 && ! e_GetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW ) )
	//	return FALSE;

	if ( (pMesh->dwFlags & (MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION | MESH_FLAG_HULL) ) == 0 &&
		 ( e_GetRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW ) ||
		   e_GetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW ) == RF_COLLISION_DRAW_ONLY ||
		   e_GetRenderFlag( RENDER_FLAG_HULL_MESH_DRAW ) ) ) 
		return FALSE;

	if (   dxC_ModelGetNoDrawGroup( pModel, 0 )
		&& (pMesh->dwFlags  & MESH_FLAG_DRAWGROUP0) != 0 )
		return FALSE;
	if (   dxC_ModelGetNoDrawGroup( pModel, 1 )
		&& (pMesh->dwFlags  & MESH_FLAG_DRAWGROUP1) != 0 )
		return FALSE;

	if ( ! pMesh->nVertexCount )
		return FALSE;
	// only draw if flag not set and verts to draw
	//	if ( ! pMesh->tVertexBuffer.pVertexBuffer )
	//		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_MeshIsVisible( int nModelId, void * pMesh ) 
{
	ASSERT_RETFALSE( pMesh );
	D3D_MODEL * pModel = dx9_ModelGet( nModelId );
	D3D_MESH_DEFINITION * pMeshDef = (D3D_MESH_DEFINITION*)pMesh;
	return dx9_MeshIsVisible( pModel, pMeshDef );
}

// ************************************************************************************************
//
//  --- Model Definition---
//
// ************************************************************************************************

static PRESULT sModelDefinitionReferenceMesh ( D3D_MODEL_DEFINITION *pModelDefinition, int nMeshId )
{
	// reference that mesh through the model
	pModelDefinition->pnMeshIds [ pModelDefinition->nMeshCount ] = nMeshId;
	pModelDefinition->nMeshCount++;
	ASSERT_RETFAIL( pModelDefinition->nMeshCount <= MAX_MESHES_PER_MODEL );
	return S_OK;
}

static MODEL_DEFINITION_HASH* sModelDefinitionHashAdd( float fDistanceFromCamera = -1.f )
{
	MODEL_DEFINITION_HASH* pHash = HashAdd( gtModelDefinitions, gnModelDefinitionNextId++ );
	pHash->tEngineRes.nId = pHash->nId;
	pHash->tEngineRes.fQuality = gtModelBudget.fQualityBias;
	pHash->tEngineRes.fDistance = fDistanceFromCamera;
	gtReaper.tModels.Add( &pHash->tEngineRes, 1 );

	return pHash;
}

static void sModelDefinitionHashRemove( int nModelDef )
{
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	if ( pHash )
	{
		MLI_DEFINITION * pSharedModelDef = pHash->pSharedDef;
		if ( pSharedModelDef )
		{
			if ( pSharedModelDef->pOccluderDef )
			{
				OCCLUDER_DEFINITION * pOccluderDef = pSharedModelDef->pOccluderDef;
				FREE( NULL, pOccluderDef->pIndices );
				FREE( NULL, pOccluderDef->pVertices );
				FREE( NULL, pOccluderDef );
			}

			FREE( NULL, pSharedModelDef->pStaticLightDefinitions );
			FREE( NULL, pSharedModelDef->pSpecularLightDefinitions );

			FREE( NULL, pSharedModelDef );
		}

		if ( pHash->tEngineRes.IsTracked() )
			gtReaper.tModels.Remove( &pHash->tEngineRes );

		HashRemove( gtModelDefinitions, nModelDef );
	}
}

int dx9_ModelDefinitionNewFromMemory ( 
	D3D_MODEL_DEFINITION* pModelDefinition, 
	int nLOD, 
	BOOL bRestore, 
	int nId /*= INVALID_ID*/ )
{
#if ISVERSION( SERVER_VERSION )
	return INVALID_ID;
#else
	MODEL_DEFINITION_HASH * pHash;
	
	if ( nId == INVALID_ID )
	{
		pHash = sModelDefinitionHashAdd();
	}
	else
	{
		pHash = HashGet( gtModelDefinitions, nId );
		if ( ! pHash )
			return INVALID_ID;
	}
	pHash->pDef[ nLOD ] = pModelDefinition;
	pModelDefinition->tHeader.nId = pHash->nId;
	if ( bRestore )
	{
		V( dx9_ModelDefinitionRestore( pModelDefinition, FALSE ) );
	}

	return pHash->nId;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL dxC_MeshDiffuseHasAlpha(
	D3D_MODEL* pModel,
	const D3D_MODEL_DEFINITION* pModelDef,
	const D3D_MESH_DEFINITION* pMesh,
	int nMesh,
	int nLOD )
{
	int nDiffuseTex = dx9_ModelGetTexture( pModel, pModelDef, pMesh, nMesh, TEXTURE_DIFFUSE, nLOD );
	D3D_TEXTURE* pDiffuseTex = dxC_TextureGet( nDiffuseTex );
	if ( !pDiffuseTex )
		return FALSE;

	return TEST_MASK( pDiffuseTex->dwFlags, TEXTURE_FLAG_HAS_ALPHA );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_MeshUpdateMaterialVersion( D3D_MESH_DEFINITION * pMeshDef )
{
	ASSERT_RETINVALIDARG( pMeshDef );
	if ( pMeshDef->nMaterialVersion < 0 )
		pMeshDef->nMaterialVersion = 0;
	else
		pMeshDef->nMaterialVersion++;
	return S_OK;
}

PRESULT e_MeshUpdateMaterialVersion( int nModelDef, int nLOD, int nMesh )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	if ( ! pModelDef )
		return S_FALSE;
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( nMesh, pModelDef->nMeshCount ) );
	D3D_MESH_DEFINITION * pMeshDef = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
	ASSERT_RETINVALIDARG( pMeshDef );
	return dx9_MeshUpdateMaterialVersion( pMeshDef );
}

PRESULT e_ModelDefinitionUpdateMaterialVersions( int nModelDef, int nLOD )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	if ( ! pModelDef )
		return S_FALSE;

	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMeshDef = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
		ASSERT_CONTINUE( pMeshDef );
		V( dx9_MeshUpdateMaterialVersion( pMeshDef ) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.12.08
static void sModelMaterialVersionsInit( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef, int nLOD )
{
	if ( pModelDef->nMeshCount <= 0 )
		return;
	ASSERT( ! pModel->tModelDefData[nLOD].pnMaterialVersion );
	pModel->tModelDefData[nLOD].pnMaterialVersion = (int*) MALLOC( NULL, sizeof(int) * pModelDef->nMeshCount );
	memset( pModel->tModelDefData[nLOD].pnMaterialVersion, 0xff, sizeof(int) * pModelDef->nMeshCount );
}

// CHB 2006.12.08
PRESULT dx9_MeshApplyMaterial( 
	D3D_MESH_DEFINITION * pMeshDefinition, 
	D3D_MODEL_DEFINITION * pModelDef,
	D3D_MODEL * pModel,
	int nLOD,	// CHB 2006.12.08
	int nMesh,
	BOOL bForceSyncLoadMaterial /*= FALSE*/ )
{
	if ( ! pMeshDefinition || ! pModel )
		return S_FALSE;

	if ( ! pMeshDefinition->ptTechniqueCache )
	{
		pMeshDefinition->ptTechniqueCache = (EFFECT_TECHNIQUE_CACHE *) MALLOC( NULL, sizeof(EFFECT_TECHNIQUE_CACHE) );
		ASSERT( pMeshDefinition->ptTechniqueCache );
		if ( pMeshDefinition->ptTechniqueCache )
			pMeshDefinition->ptTechniqueCache->Reset();
	}

	if ( pModel && pModelDef && ! pModel->tModelDefData[nLOD].pnMaterialVersion )
	{
		sModelMaterialVersionsInit( pModel, pModelDef, nLOD );
	}

	if ( pModel && pModelDef )
	{
		// propagate flags
		if ( pModelDef->dwFlags & MODELDEF_FLAG_CAST_DYNAMIC_SHADOW && ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW ) )
		{
			dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW, TRUE );
			if( pModel->nProxmapId == INVALID_ID )
			{
				V( dx9_ModelAddToProxMap( pModel ) );
			}
		}
	}

	// This function could be called multiple times, and could complete different phases at these times (first just mesh + model def, later model)

	BOOL bModelMatApplied = FALSE;
	if ( pModel && pModel->tModelDefData[nLOD].pnMaterialVersion )
	{
		if ( pModel->tModelDefData[nLOD].pnMaterialVersion[ nMesh ] == pMeshDefinition->nMaterialVersion && pMeshDefinition->nMaterialVersion != INVALID_ID )
			bModelMatApplied = TRUE;
		// debug
		//else if ( pMeshDefinition->nMaterialVersion != INVALID_ID )
		//	trace( "Detected material version change!\nModel: %d ModelDef: %d Mesh: %d Version: %d\n", pModel->nId, pModelDef->tHeader.nId, nMesh, pMeshDefinition->nMaterialVersion );
	}

	// if this model needs to re-apply the mesh material
	if ( ! bModelMatApplied )
	{
		// first check the diffuse override texture for a material
		int nDiffTex = dx9_ModelGetTextureOverride( pModel, pModelDef, nMesh, TEXTURE_DIFFUSE, nLOD );	// CHB 2006.12.08, 2006.11.28
		if ( nDiffTex != INVALID_ID )
		{
			int nTexDef = e_TextureGetDefinition( nDiffTex );
			if ( nTexDef != INVALID_ID )
			{
				TEXTURE_DEFINITION * pTexDef = (TEXTURE_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_TEXTURE, nTexDef );
				if ( pTexDef )
				{
					if ( 0 == PStrICmp( pTexDef->pszMaterialName, "Default" ) )
					{
						if( AppIsHellgate() )
						{
							ShowDataWarning( "Texture override doesn't specify a material:\n\n%s", pTexDef->tHeader.pszName );
						}
						pModel->nTextureMaterialOverride = DefinitionGetIdByName( DEFINITION_GROUP_MATERIAL, "Animated", -1, bForceSyncLoadMaterial );  // don't override
					}
					else
					{
						pModel->nTextureMaterialOverride = DefinitionGetIdByName( DEFINITION_GROUP_MATERIAL, pTexDef->pszMaterialName, -1, bForceSyncLoadMaterial );
					}
					ASSERTX( pModel->nTextureMaterialOverride != INVALID_ID, "Model should have an override material at this point!" )
				}
				else
				{
					return S_FALSE;
				}
			}
		}

		// if that didn't work...
		//if ( pModel->nTextureMaterialOverride == INVALID_ID )
		//{
		//	// need to try to load the material from the diffuse texture

		//	int nDiffTex = dx9_ModelGetTexture( pModel, pModelDef, nMesh, TEXTURE_DIFFUSE, TRUE, NULL );
		//	BOOL bLoaded = AUTO_VERSION( e_TextureIsLoaded( nDiffTex, TRUE ), FALSE );
		//	if ( bLoaded )
		//	{
		//		D3D_TEXTURE * pTexture = dxC_TextureGet( nDiffTex );
		//		ASSERT(pTexture);
		//		if ( pTexture && pTexture->nDefinitionId != INVALID_ID )
		//		{
		//			TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId);
		//			if ( pTextureDefinition )
		//			{
		//				pMeshDefinition->nMaterialId = DefinitionGetIdByName( DEFINITION_GROUP_MATERIAL, pTextureDefinition->pszMaterialName, -1, bForceSyncLoadMaterial );
		//			}
		//		}
		//	}
		//}

		//nDiffTex = dx9_ModelGetTexture( pModel, pModelDef, nMesh, TEXTURE_DIFFUSE );
		//dx9_MeshRefreshMaterial( pMeshDefinition, nDiffTex );
	}

	int nMaterialId = dx9_MeshGetMaterial( pModel, pMeshDefinition );
	if ( nMaterialId == INVALID_ID )
		return S_FALSE;

	MATERIAL * pMaterial = NULL;

	// apply to model if necessary
	if ( pModel && ! bModelMatApplied )
	{
		pMaterial = (MATERIAL *) DefinitionGetById(DEFINITION_GROUP_MATERIAL, nMaterialId );
		if ( pMaterial )
		{
			if ( pModel->tModelDefData[nLOD].pfScrollingUVOffsets && pMaterial->dwFlags & MATERIAL_FLAG_SCROLL_RANDOM_OFFSET )
			{
				pModel->tModelDefData[nLOD].pfScrollingUVOffsets[ nMesh ] = RandGetFloat( e_GetEngineOnlyRand(), 1.0f );
			}
			ASSERT_RETFAIL( pModel->tModelDefData[nLOD].pnMaterialVersion );
			ASSERT_RETFAIL( nMesh < pModelDef->nMeshCount );
			pModel->tModelDefData[nLOD].pnMaterialVersion[ nMesh ] = pMeshDefinition->nMaterialVersion;

			pModel->tModelDefData[nLOD].tScrollLastUpdateTime = e_GetCurrentTimeRespectingPauses();
		}
	}

	// is this the mesh's material, or one from a model diffuse texture or state override?
	BOOL bFromMesh = nMaterialId == pMeshDefinition->nMaterialId;
	if ( ! bFromMesh )
		return S_FALSE;

	// the rest of this function applies only to the mesh's own material (pMesh->nMaterialId)

	// it's the lazy update for mesh-definition check
	if ( pMeshDefinition->dwFlags & MESH_FLAG_MATERIAL_APPLIED )
		return S_FALSE;

	if ( !pMaterial )
	{
		pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, pMeshDefinition->nMaterialId );
		if (!pMaterial)
			return S_FALSE;
	}

	// if the environment isn't yet loaded...
	if ( e_GetCurrentEnvironmentDefID() == INVALID_ID )
		ASSERTX_RETFAIL( 0, "No environment defined! Check the levels spreadsheet." );
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return S_FALSE;

	int nLocation = e_GetCurrentLevelLocation();
	ASSERTV_RETFAIL( nLocation != LEVEL_LOCATION_UNKNOWN, "Invalid level location! Is the correct level location specified in the current level environment?\n%s", pEnvDef->tHeader.pszName );

	const EFFECT_DEFINITION * pEffectDef = NULL;
	int nEffectID = dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nLocation );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );

#if ! ISVERSION(SERVER_VERSION)
	V_RETURN( dxC_ToolLoadShaderNow( pEffect, pMaterial, nLocation ) );
	ASSERTV_RETFAIL( pEffect, "Effect missing: effectid[%d] mtrl[%s] lineid[%d] loc[%d]", nEffectID, pMaterial->tHeader.pszName, pMaterial->nShaderLineId, nLocation );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;
#endif // SERVER_VERSION


	if ( pEffect && pEffect->nEffectDefId != INVALID_ID )
	{
		pEffectDef = (const EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pEffect->nEffectDefId );
	}
	if ( ! pEffectDef )
	{
		const char * pszFXFileName = dxC_GetShaderTypeFXFile( pMaterial->nShaderLineId, nLocation );
		ASSERT_RETFAIL(pszFXFileName);
		if ( pszFXFileName )
			pEffectDef = (const EFFECT_DEFINITION *)ExcelGetDataByStringIndex(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pszFXFileName );
	}

	if (pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_BLEND ) )
	{
		if ( TEST_MASK( pMaterial->dwFlags, MATERIAL_FLAG_ALPHABLEND ) 
		  && dxC_MeshDiffuseHasAlpha( pModel, pModelDef, pMeshDefinition, nMesh, nLOD ) )
		{
			pMeshDefinition->dwFlags |= MESH_FLAG_ALPHABLEND;
			if ( pModelDef )
				pModelDef->dwFlags |= MODELDEF_FLAG_HAS_ALPHA;
		}
	}
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
	{
		pMeshDefinition->dwFlags |= MESH_FLAG_ALPHABLEND;
		if ( pModelDef )
			pModelDef->dwFlags |= MODELDEF_FLAG_HAS_ALPHA;
	}
	// Determine correct shadow type
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_CAST_SHADOW ) )
	{
		if ( pModelDef )
			pModelDef->dwFlags |= MODELDEF_FLAG_CAST_DYNAMIC_SHADOW;
		if ( pModel )
		{
			dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW, TRUE );
			if( pModel->nProxmapId == INVALID_ID )
			{
				V( dx9_ModelAddToProxMap( pModel ) );
			}
		}
	}

	sMeshComputeTechniqueMask( pMeshDefinition );

	pMeshDefinition->dwFlags |= MESH_FLAG_MATERIAL_APPLIED;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.06.23
static
void dx9_ModelMassageVertices(D3D_VERTEX_BUFFER_DEFINITION * pBufferDef)
{
#if !ISVERSION(SERVER_VERSION)
	EFFECT_TARGET eTarget = dxC_GetCurrentMaxEffectTarget();

	if (eTarget > dxC_AnimatedModelGetMaximumEffectTargetForModel11Animation())	// CHB 2007.10.16
	{
		return;
	}

	switch (pBufferDef->eVertexType)
	{
		case VERTEX_DECL_ANIMATED:
			// Currently just maps.
			pBufferDef->eVertexType = VERTEX_DECL_ANIMATED_11;
			break;
		case VERTEX_DECL_PARTICLE_MESH:
			// Currently just maps.
			pBufferDef->eVertexType = VERTEX_DECL_PARTICLE_MESH_11;
			break;
		default:
			break;
	}
#else
	REF(pBufferDef);
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelUpdateByDefinition( int nDefinitionID )
{
	ASSERT_RETINVALIDARG( nDefinitionID != INVALID_ID );

	for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
		pModel;
		pModel = dx9_ModelGetNext( pModel ) )
	{
		if ( pModel->nModelDefinitionId != nDefinitionID )
			continue;

		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			V( e_ModelSetDefinition( pModel->nId, pModel->nModelDefinitionId, nLOD ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelUpdateByDefinition( int nModelDefinition )
{
	return dxC_ModelUpdateByDefinition( nModelDefinition );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sModelDefCreateDPVSModels( D3D_MODEL_DEFINITION* pModelDef )
{
	ASSERT_RETINVALIDARG( pModelDef );

	DPVS_SAFE_RELEASE( pModelDef->pCullTestModel );
	DPVS_SAFE_RELEASE( pModelDef->pCullWriteModel );

	dxC_DPVS_ModelDefCreateTestModel( pModelDef );
	dxC_DPVS_ModelDefCreateWriteModel( pModelDef );

	V( dxC_ModelUpdateByDefinition( pModelDef->tHeader.nId ) );
	
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_ModelDefinitionFromFileInMemory(
	int* pnModelDefID,
	int nModelDefIdOverride,
	int nLOD, 
	const char * pszFilename,
	const char * pszFolderIn,
	D3D_MODEL_DEFINITION* model_definition,
	D3D_MESH_DEFINITION* pMeshDefs[],
	DWORD dwFlags,
	float fDistanceToCamera )
{
	if ( pnModelDefID )
		*pnModelDefID = INVALID_ID;

#if !ISVERSION( SERVER_VERSION )
	MODEL_DEFINITION_HASH* hash = NULL;
	if ( nModelDefIdOverride != INVALID_ID )
	{
		hash = HashGet(gtModelDefinitions, nModelDefIdOverride);
		ASSERT_RETFAIL( !hash || !hash->pDef[ nLOD ] );
	} 
	
	if ( !hash )
		hash = sModelDefinitionHashAdd( fDistanceToCamera > 0 ? fDistanceToCamera : -1.f );

	int nModelDefinitionId = hash->nId;
	if ( pszFilename )
		PStrCopy(model_definition->tHeader.pszName, pszFilename, MAX_XML_STRING_LENGTH);
	model_definition->tHeader.nId = nModelDefinitionId;

	for (int ii = 0; ii < model_definition->nAttachmentCount; ii++)
	{
		ASSERT_RETFAIL( gpfn_AttachmentDefinitionLoad );
		gpfn_AttachmentDefinitionLoad( model_definition->pAttachments[ii], INVALID_ID, pszFolderIn );
	}

	if ( hash && hash->dwFlags & MODELDEF_HASH_FLAG_IS_PROP )
	{
		model_definition->dwFlags |= MODELDEF_FLAG_IS_PROP;
	}

	// -- Vertex Buffers --
	if ( ! model_definition->nVertexBufferDefCount )
	{
		ASSERTX_RETFAIL( model_definition->nVertexBufferDefCount, model_definition->tHeader.pszName );
		model_definition->ptVertexBufferDefs = NULL;
	}

	// don't load some textures in CheapMode
	BOOL bCheapMode = FALSE;
#ifdef _DEBUG
	//GLOBAL_DEFINITION * pGlobalDef = DefinitionGetGlobal();
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( pOverrides && ( pOverrides->dwFlags & GFX_FLAG_LOD_EFFECTS_ONLY ) != 0 )
		bCheapMode = TRUE;
#endif

	ASSERTV( model_definition->nMeshCount <= MAX_MESHES_PER_MODEL, "Too many meshes in model %s\nCurrent: %d, max: %d", model_definition->tHeader.pszName, model_definition->nMeshCount, MAX_MESHES_PER_MODEL );

	BOOL bIncludeDebugTextures = FALSE;

	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( pGlobals->dwFlags & GLOBAL_FLAG_LOAD_DEBUG_BACKGROUND_TEXTURES )
		bIncludeDebugTextures = TRUE;

	BOOL bZeroLocalPointer = ! c_GetToolMode();

	// -- Meshes --
	for (int nMeshIndex = 0; nMeshIndex < model_definition->nMeshCount; nMeshIndex++)
	{
		D3D_MESH_DEFINITION* pMeshDefinition = pMeshDefs[ nMeshIndex ];

		// clear the runtime flags
		pMeshDefinition->dwFlags &= ~(MESH_FLAG_CHECKED_TEXTURE_SIZES);
		pMeshDefinition->dwFlags &= ~(MESH_FLAG_MATERIAL_APPLIED);
		memset( pMeshDefinition->pnTextures, -1, sizeof(pMeshDefinition->pnTextures) );

		// Add this mesh to the big list of meshes
		MESH_DEFINITION_HASH * pMeshHash = HashAdd(gtMeshes, gnMeshDefinitionNextId++);
		pMeshHash->pDef = pMeshDefinition;
		int nMeshId = pMeshHash->nId;

		model_definition->pnMeshIds[nMeshIndex] = nMeshId;

		pMeshDefinition->tIndexBufferDef.nD3DBufferID = INVALID_ID;

		char szFolder  [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		OVERRIDE_PATH tOverride;

		BOOL bBackground = (dwFlags & MODEL_DEFINITION_CREATE_FLAG_BACKGROUND) != 0;


		// Set the hash pointer now that pointers have been offset correctly.
		hash->pDef[ nLOD ] = model_definition;

		pMeshDefinition->nMaterialId = INVALID_ID;
		if ( !bIncludeDebugTextures && (pMeshDefinition->dwFlags & MESH_MASK_ALL_DEBUG_MESHES) != 0 )
			continue;

		dxC_MeshGetFolders( pMeshDefinition, nLOD, szFolder, &tOverride, pszFolderIn, DEFAULT_FILE_WITH_PATH_SIZE, bBackground );
		for (int ii = 0; ii < NUM_TEXTURE_TYPES; ii++)
		{
			if ( bCheapMode )
			{ // don't load some textures in cheap mode
				switch ( ii )
				{
				case TEXTURE_NORMAL:
				case TEXTURE_SPECULAR:
					pMeshDefinition->pnTextures[ii] = INVALID_ID;
					continue;
				}
			}

			// CHB 2006.10.09 - Don't load unused textures.
			if (!e_TextureSlotIsUsed(static_cast<TEXTURE_TYPE>(ii)))
			{
				pMeshDefinition->pnTextures[ii] = INVALID_ID;
				continue;
			}

			if (pMeshDefinition->pszTextures[ii][0] != 0 && (dwFlags & MODEL_DEFINITION_CREATE_FLAG_SHOULD_LOAD_TEXTURES) )
			{
				char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
				BOOL bOverrideUsed = FALSE;
				
				if (ii == TEXTURE_LIGHTMAP && tOverride.szPath[0])
				{
					// if this is a lightmap file in an overridden-path material
					// check if the local lightmap exists, then check the override path
					PStrPrintf(szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szFolder, pMeshDefinition->pszTextures[ii]);

					BOOL bAllowDirectFileCheck = AppCommonGetUpdatePakFile() || c_GetToolMode();

					char szFixedFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
					FixFilename( szFixedFolder, szFolder, ARRAYSIZE( szFolder ) );
					PStrForceBackslash( szFixedFolder, ARRAYSIZE( szFixedFolder ) );
					PStrT pstrFolder( szFixedFolder, PStrLen( szFixedFolder, ARRAYSIZE( szFixedFolder ) ) );
					char szTextureDDS[ DEFAULT_FILE_WITH_PATH_SIZE ];
					FixFilename( szTextureDDS, pMeshDefinition->pszTextures[ii], ARRAYSIZE( szTextureDDS ) );
					PStrReplaceExtension( szTextureDDS, DEFAULT_FILE_WITH_PATH_SIZE, 
						szTextureDDS, D3D_TEXTURE_FILE_EXTENSION );
					PStrT pstrTexture( szTextureDDS, PStrLen( szTextureDDS, ARRAYSIZE( szTextureDDS ) ) );

					BOOL bUseOverride = TRUE;
					if ( PakFileLoadGetIndex( pstrFolder, pstrTexture ) )
						bUseOverride = FALSE;
					if ( bUseOverride && bAllowDirectFileCheck && FileExists( szFile ) )
						bUseOverride = FALSE;

					if ( bUseOverride )
					{
						// use the override path instead
						PStrPrintf(szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", tOverride.szPath, pMeshDefinition->pszTextures[ii]);
						bOverrideUsed = TRUE;
					}
				} 
				else
				{
					// normal texture or no override
					const char *szPath = szFolder;
					if (tOverride.szPath[0])
					{
						szPath = tOverride.szPath;
						bOverrideUsed = TRUE;
					}
					PStrPrintf(szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, pMeshDefinition->pszTextures[ii]);						
				}
				DWORD dwReloadFlags = 0;
				int nLocation = e_GetCurrentLevelLocation();

				if ( bBackground && ii == TEXTURE_DIFFUSE )
				{ // currently required to get the material
					dwReloadFlags |= TEXTURE_LOAD_NO_ASYNC_DEFINITION;
				}

				MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDefinitionId );
				if (   ( nLOD == LOD_HIGH ) 
					&& ( !pHash 
					  || !TEST_MASK( pHash->dwLODFlags[ LOD_LOW ], MODELDEF_HASH_FLAG_LOAD_FAILED ) ) )
					dwReloadFlags |= TEXTURE_LOAD_ADD_TO_HIGH_PAK;

				float fMeshDistanceToCamera = fDistanceToCamera;
				if ( pMeshDefinition->dwFlags & (MESH_FLAG_DEBUG | MESH_FLAG_HULL | MESH_FLAG_CLIP | MESH_FLAG_COLLISION) )
				{
					fMeshDistanceToCamera = DEBUG_MESH_DISTANCE_TO_CAMERA;
				}
				char szFileWithFullPath[ MAX_PATH ];
				FileGetFullFileName( szFileWithFullPath, szFile, MAX_PATH );

				// what pak file is this texture in
				PAK_ENUM ePakfile = PAK_DEFAULT;
				if (bOverrideUsed == TRUE)
				{
					ePakfile = tOverride.ePakEnum;
				}
				
				e_TextureReleaseRef( pMeshDefinition->pnTextures[ii] );
				V( e_TextureNewFromFile( 
						&pMeshDefinition->pnTextures[ii], 
						szFileWithFullPath, 
						bBackground ? TEXTURE_GROUP_BACKGROUND : TEXTURE_GROUP_UNITS, 
						ii, 
						0, 
						0, 
						NULL, 
						NULL, 
						NULL, 
						NULL, 
						dwReloadFlags, 
						fMeshDistanceToCamera,
						0,
						ePakfile ) );
				e_TextureAddRef( pMeshDefinition->pnTextures[ ii ] );

				// CHB 2007.09.04 - Temporarily, warn when a texture
				// is not found for low-LOD backgrounds. This is so
				// that the asset files can be rearranged.
#if ISVERSION(DEVELOPMENT)
				if (    bBackground 
					 && (nLOD == LOD_LOW)
					 && ii != TEXTURE_NORMAL
					 && ii != TEXTURE_SPECULAR )
				{
					D3D_TEXTURE * pTexture = dxC_TextureGet( pMeshDefinition->pnTextures[ ii ] );
					ASSERT(pTexture != 0);
					if ((pTexture->dwFlags & _TEXTURE_FLAG_LOAD_FAILED) != 0)
					{
						ShowDataWarning("LOW_LOD_BG_TEXTURE_MISSING: %s", szFileWithFullPath);
					}
				}
#endif

				ASSERT(pMeshDefinition->pnTextures[ii] != INVALID_ID);
			} 
			else 
			{
				pMeshDefinition->pnTextures[ii] = INVALID_ID;
			}
		}		
		
		// set the material from the diffuse texture's material
		if ( pMeshDefinition->pszTextures[TEXTURE_DIFFUSE][0] != 0 )
		{
			char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
			PAK_ENUM ePakEnum = PAK_DEFAULT;
			char *szPath = szFolder;
			if (tOverride.szPath[0])
			{
				szPath = tOverride.szPath;
				ePakEnum = tOverride.ePakEnum;
			}
			PStrPrintf(szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, pMeshDefinition->pszTextures[ TEXTURE_DIFFUSE ]);

			int nTexDef = DefinitionGetIdByFilename( DEFINITION_GROUP_TEXTURE, szFile, -1, FALSE, TRUE, ePakEnum );
			ASSERT( nTexDef != INVALID_ID );
			TEXTURE_DEFINITION * pTextureDefinition = (TEXTURE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_TEXTURE, nTexDef );
			EVENT_DATA tEventData( bBackground ? 1 : 0, nMeshId, nModelDefinitionId, nLOD );
			if ( pTextureDefinition )
			{
				dxC_DiffuseTextureDefinitionLoadedCallback( pTextureDefinition, &tEventData );
			} else {
				pMeshDefinition->nMaterialId = INVALID_ID;
				DefinitionAddProcessCallback( DEFINITION_GROUP_TEXTURE, nTexDef, 
					dxC_DiffuseTextureDefinitionLoadedCallback, &tEventData );
			}
		} 
		else if ( !bBackground )
		{
			V( e_MaterialNew( &pMeshDefinition->nMaterialId, "Animated" ) );
			//pMeshDefinition->nMaterialId = INVALID_ID;
		}
		else 
		{
			V( e_MaterialNew( &pMeshDefinition->nMaterialId, "Simple" ) );
			//pMeshDefinition->nMaterialId = INVALID_ID;
		}

		if ( pMeshDefinition->nMaterialId != INVALID_ID )
		{
			V( dx9_MeshApplyMaterial( pMeshDefinition, model_definition, NULL, -1, -1 ) );	// CHB 2006.12.08
		}

		// this is where we actually create the device-specific stuff
		V( dx9_MeshRestore(pMeshDefinition) );

		if ( bZeroLocalPointer )
		{
			FREE( NULL, pMeshDefinition->tIndexBufferDef.pLocalData );
			pMeshDefinition->tIndexBufferDef.pLocalData = NULL;
		}
	}

	V( dx9_ModelDefinitionRestore(model_definition, bZeroLocalPointer) );

	if ( pnModelDefID ) 
		*pnModelDefID = nModelDefinitionId;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct MODEL_DEFINITION_CALLBACK_DATA
{
	int nModelDef;
	int nLOD;
	float fDistanceToCamera;
	unsigned nFlags;
	char szFolder[ MAX_PATH ];
};

static PRESULT sSharedModelDefinitionFileLoadCallback( ASYNC_DATA& data)
{
#if ISVERSION( SERVER_VERSION )
	return S_FALSE;
#else
	PAKFILE_LOAD_SPEC* spec = (PAKFILE_LOAD_SPEC*)data.m_UserData;
	ASSERT_RETFAIL(spec);

	MODEL_DEFINITION_CALLBACK_DATA* pCallbackData = (MODEL_DEFINITION_CALLBACK_DATA*)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);

	void *pData = spec->buffer;

	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pCallbackData->nModelDef );
	if ( pHash )
		CLEAR_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOADING );
	else
		return S_FALSE;

	SERIALIZED_FILE_HEADER* pFileHeader = NULL;

	if ( ( data.m_IOSize ) == 0 || !pData ) 
	{
		SET_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOAD_FAILED );
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

		ASSERTX_RETFAIL( pFileHeader->dwMagicNumber == MLI_FILE_MAGIC_NUMBER, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nVersion <= MLI_FILE_VERSION, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nNumSections > 0, spec->filename );

		pFileHeader->pSections = MALLOC_TYPE( SERIALIZED_FILE_SECTION, NULL, 
			pFileHeader->nNumSections );

		for ( int nSection = 0; nSection < pFileHeader->nNumSections; nSection++ )
			dxC_SerializedFileSectionXfer( tHeaderXfer, pFileHeader->pSections[ nSection ], 
				pFileHeader->nSFHVersion );

		BYTE* pbFileDataStart = pbFileStart + tHeaderXfer.GetCursor();
		BYTE_BUF tFileBuf( pbFileDataStart, spec->bytesread - (DWORD)(pbFileDataStart - pbFileStart) );
		BYTE_XFER<XFER_LOAD> tXfer(&tFileBuf);

		if ( !pHash->pSharedDef )
		{
			pHash->pSharedDef = MALLOCZ_TYPE( MLI_DEFINITION, NULL, 1 );
			ASSERTX_GOTO( pHash->pSharedDef, spec->filename, _cleanup );
		}

		V_GOTO( _cleanup, dxC_SharedModelDefinitionXferFileSections( tXfer, 
			*pHash->pSharedDef, pFileHeader->pSections, pFileHeader->nNumSections,
			pFileHeader->nVersion ) );
				
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			D3D_MODEL_DEFINITION* pModelDef = pHash->pDef[ nLOD ];
			if ( pModelDef )
				sModelDefCreateDPVSModels( pModelDef );
		}

		FREE( NULL, pFileHeader->pSections );
		FREE( NULL, pFileHeader );
		pFileHeader = NULL;
	}

	return S_OK;

_cleanup:
	if ( pHash->pSharedDef )
	{
		MLI_DEFINITION* pSharedModelDef = pHash->pSharedDef;
		FREE( NULL, pSharedModelDef->pStaticLightDefinitions );
		FREE( NULL, pSharedModelDef->pSpecularLightDefinitions);
		FREE( NULL, pSharedModelDef->pOccluderDef );
		FREE( NULL, pSharedModelDef );
		pHash->pSharedDef = NULL;
	}

	if ( pFileHeader )
	{
		FREE( NULL, pFileHeader->pSections );
		FREE( NULL, pFileHeader );
	}

	return S_FALSE;
#endif //SERVER_VERSION
}

static PRESULT sSharedModelDefinitionNewFromFile( 
	int nModelDef, 	
	const char* filename,
	BOOL bForceSyncLoad /*= FALSE*/ )
{
	TIMER_STARTEX(filename, 50);

	char model_filename[MAX_XML_STRING_LENGTH];
	PStrCopy( model_filename, filename, MAX_XML_STRING_LENGTH );

	DebugString(OP_LOW, LOADING_STRING(model), model_filename);
	PStrReplaceExtension(model_filename, MAX_XML_STRING_LENGTH, model_filename, MODEL_LOD_INDEPENDENT_FILE_EXTENSION );

	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	ASSERT_RETFAIL( pHash );

	if ( TEST_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOADING ) )
		return S_FALSE;
	
	MODEL_DEFINITION_CALLBACK_DATA* pCallbackData = MALLOCZ_TYPE( MODEL_DEFINITION_CALLBACK_DATA, NULL, 1 );
	pCallbackData->nModelDef = nModelDef;

	DECLARE_LOAD_SPEC( spec, model_filename );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER
		| PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND
		| PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;
	spec.fpLoadingThreadCallback = sSharedModelDefinitionFileLoadCallback;
	spec.callbackData = pCallbackData;
	spec.priority = ASYNC_PRIORITY_MODELS;

	CLEAR_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOAD_FAILED );
	SET_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOADING );

	if( c_GetToolMode() || bForceSyncLoad )
		PakFileLoadNow(spec);
	else
		PakFileLoad(spec);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sModelDefinitionFileLoadCallback( ASYNC_DATA& data)
{
#if ISVERSION( SERVER_VERSION )
	return S_FALSE;
#else
	PAKFILE_LOAD_SPEC* spec = (PAKFILE_LOAD_SPEC*)data.m_UserData;
	ASSERT_RETFAIL(spec);

	MODEL_DEFINITION_CALLBACK_DATA* pCallbackData = (MODEL_DEFINITION_CALLBACK_DATA*)spec->callbackData;
	void *pData = spec->buffer;
	
	int nLOD = pCallbackData->nLOD;
	unsigned nFlags = 0;
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pCallbackData->nModelDef );
	if ( pHash )
		CLEAR_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING );

	SERIALIZED_FILE_HEADER* pFileHeader = NULL;
	D3D_MODEL_DEFINITION* pModelDef = NULL;

	if ( ( data.m_IOSize ) == 0 || !pData ) 
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
		V_RETURN( dxC_SerializedFileHeaderXfer( tHeaderXfer, *pFileHeader ) );

		ASSERTX_RETFAIL( pFileHeader->dwMagicNumber == MODEL_FILE_MAGIC_NUMBER, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nVersion >= MIN_REQUIRED_MODEL_FILE_VERSION, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nVersion <= MODEL_FILE_VERSION, spec->filename );
		ASSERTX_RETFAIL( pFileHeader->nNumSections > 0, spec->filename );

		pFileHeader->pSections = MALLOC_TYPE( SERIALIZED_FILE_SECTION, NULL, 
			pFileHeader->nNumSections );
		ASSERTX_GOTO( pFileHeader->pSections, spec->filename, _cleanup );

		for ( int nSection = 0; nSection < pFileHeader->nNumSections; nSection++ )
			dxC_SerializedFileSectionXfer( tHeaderXfer, pFileHeader->pSections[ nSection ], 
				pFileHeader->nSFHVersion );
		
		BYTE* pbFileDataStart = pbFileStart + tHeaderXfer.GetCursor();
		BYTE_BUF tFileBuf( pbFileDataStart, spec->bytesread - (DWORD)(pbFileDataStart - pbFileStart) );
		BYTE_XFER<XFER_LOAD> tXfer(&tFileBuf);

		D3D_MODEL_DEFINITION* pModelDef = MALLOCZ_TYPE( D3D_MODEL_DEFINITION, NULL, 1 );
		ASSERTX_GOTO( pModelDef, spec->filename, _cleanup );
		pModelDef->tHeader.nId = pCallbackData->nModelDef;

		D3D_MESH_DEFINITION* pMeshDefs[ MAX_MESHES_PER_MODEL ];
		structclear( pMeshDefs );

		V_GOTO( _cleanup, dxC_ModelDefinitionXferFileSections( tXfer, *pModelDef, 
			pMeshDefs, pFileHeader->pSections, pFileHeader->nNumSections, 
			pFileHeader->nVersion, FALSE ) );

		FREE( NULL, pFileHeader->pSections );
		FREE( NULL, pFileHeader );
		pFileHeader = NULL;

		V_GOTO( _cleanup, dx9_ModelDefinitionFromFileInMemory( NULL, pCallbackData->nModelDef, nLOD, 
			spec->filename, pCallbackData->szFolder, pModelDef, pMeshDefs,
			MODEL_DEFINITION_CREATE_FLAG_BACKGROUND | MODEL_DEFINITION_CREATE_FLAG_SHOULD_LOAD_TEXTURES, 
			pCallbackData->fDistanceToCamera ) );
	}

	// Fall through and load the high LOD if this LOD was the low LOD.

	if ((pCallbackData->nFlags & MDNFF_FLAG_DONT_LOAD_NEXT_LOD) == 0)
	{
		if ( e_ModelBudgetGetMaxLOD( MODEL_GROUP_BACKGROUND ) > LOD_LOW )
		{
_loadhigh:
			// Is this the first LOD? If so, then load the next one.
			if ( nLOD == LOD_LOW )
			{
				char szPath[DEFAULT_FILE_WITH_PATH_SIZE];
				char szFolder[DEFAULT_FILE_WITH_PATH_SIZE];
				V_RETURN( e_ModelDefinitionStripLODFileName( szPath, ARRAYSIZE( szPath ), spec->filename ) );
				PStrGetPath( szFolder, ARRAYSIZE( szFolder ), szPath );

				int nModelDef;
				dxC_ModelDefinitionNewFromFileEx( &nModelDef, LOD_HIGH, pCallbackData->nModelDef, 
					szPath, szFolder, pCallbackData->fDistanceToCamera, nFlags );
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
	FREE( NULL, pModelDef );

	return S_FALSE;
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ModelDefinitionNewFromFileEx( 
	int* pnModelDef,
	int nLOD,
	int nModelDefOverride,
	const char* filename, 
	const char* filefolder,
	float fDistanceToCamera,
	unsigned nFlags )
{
	ASSERT_RETFAIL( pnModelDef );
	*pnModelDef = INVALID_ID;

	TIMER_STARTEX(filename, 50);

	char model_filename[MAX_XML_STRING_LENGTH];
	PStrCopy( model_filename, filename, MAX_XML_STRING_LENGTH );

	DebugString(OP_LOW, LOADING_STRING(model), model_filename);

	// CHB 2006.10.20
	bool bSourceFileOptional = (nFlags & MDNFF_FLAG_SOURCE_FILE_OPTIONAL) != 0;
	BOOL bModelUpdated = e_GrannyUpdateModelFileEx(model_filename, FALSE, bSourceFileOptional, nLOD );

	PStrReplaceExtension(model_filename, MAX_XML_STRING_LENGTH, model_filename, D3D_MODEL_FILE_EXTENSION);

	if ( nModelDefOverride != INVALID_ID )
	{
		MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDefOverride );
		ASSERT_RETFAIL( pHash );
		*pnModelDef = nModelDefOverride;

		if ( pHash && TEST_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING ) )
			return S_FALSE;
	}
	else
	{
		int nModelDefinitionId = e_ModelDefinitionFind(model_filename, MAX_XML_STRING_LENGTH);
		if (nModelDefinitionId != INVALID_ID)
		{
			*pnModelDef = nModelDefinitionId;
			if ( dxC_ModelDefinitionGet( nModelDefinitionId, nLOD ) )			
				return S_OK;
		}
		else
			*pnModelDef = dx9_ModelDefinitionNew( INVALID_ID );
	}

	ASSERT_RETFAIL( *pnModelDef != INVALID_ID );

	MODEL_DEFINITION_CALLBACK_DATA* pCallbackData = 
		(MODEL_DEFINITION_CALLBACK_DATA*)MALLOC( NULL, sizeof( MODEL_DEFINITION_CALLBACK_DATA ) );
	pCallbackData->nModelDef = *pnModelDef;
	pCallbackData->fDistanceToCamera = fDistanceToCamera;
	pCallbackData->nLOD = nLOD;
	pCallbackData->nFlags = nFlags;
	PStrCopy( pCallbackData->szFolder, filefolder, MAX_PATH );

	DECLARE_LOAD_SPEC( spec, model_filename );
	spec.pool = g_ScratchAllocator;
	
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.fpLoadingThreadCallback = sModelDefinitionFileLoadCallback;
	spec.callbackData = pCallbackData;
	spec.priority = ASYNC_PRIORITY_MODELS;

	if ( nLOD == LOD_LOW )
		spec.flags |= PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;
	else if ( ( (nFlags & MDNFF_FLAG_FORCE_ADD_TO_DEFAULT_PAK) == 0 ) && ( nLOD == LOD_HIGH ) )
		spec.pakEnum = PAK_GRAPHICS_HIGH_BACKGROUNDS;

	if ( bSourceFileOptional )
		spec.flags |= PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;

	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pCallbackData->nModelDef );
	ASSERT_RETFAIL( pHash );

	CLEAR_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOAD_FAILED );
	SET_MASK( pHash->dwLODFlags[ nLOD ], MODELDEF_HASH_FLAG_LOADING );

	bool bForceSyncLoad = (nFlags & MDNFF_FLAG_FORCE_SYNC_LOAD) != 0;

	if (     bModelUpdated
		|| ( !pHash->pSharedDef 
		  && !TEST_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOAD_FAILED ) ) )
	{
		// If the model has been updated, then force a reload of the MLI definition.
		if ( bModelUpdated )
			CLEAR_MASK( pHash->dwFlags, MODELDEF_HASH_FLAG_LOAD_FAILED | MODELDEF_HASH_FLAG_LOADING );

		char szPath[DEFAULT_FILE_WITH_PATH_SIZE];
		V_RETURN( e_ModelDefinitionStripLODFileName( szPath, ARRAYSIZE( szPath ), spec.filename ) );
		sSharedModelDefinitionNewFromFile( pCallbackData->nModelDef, szPath, bForceSyncLoad );
	}

	if( c_GetToolMode() || bForceSyncLoad )
	{
		PakFileLoadNow(spec);
	}
	else
	{
		PakFileLoad(spec);
	}

	return S_OK;
}

PRESULT e_ModelDefinitionGetLODFileName(char * pFileNameOut, unsigned nFileNameOutBufferChars, const char * pFileNameIn)
{
	if ( PStrStrI( pFileNameIn, MODEL_LOW_SUBDIR ) == NULL )
	{
		// Build the path and filename for the low-poly model.
		char just_filename[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrRemoveEntirePath(just_filename, ARRAYSIZE(just_filename), pFileNameIn);
		PStrGetPath(pFileNameOut, nFileNameOutBufferChars, pFileNameIn);
		PStrForceBackslash(pFileNameOut, nFileNameOutBufferChars);
		PStrCat(pFileNameOut, MODEL_LOW_SUBDIR, nFileNameOutBufferChars);
		PStrCat(pFileNameOut, just_filename, nFileNameOutBufferChars);
		return S_OK;
	}
	else
	{
		PStrCopy( pFileNameOut, pFileNameIn, nFileNameOutBufferChars );
		return S_FALSE;
	}
}

PRESULT e_ModelDefinitionStripLODFileName( char* pFileNameOut, unsigned nFileNameOutBufferChars, const char* pFileNameIn )
{
	char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrRemoveEntirePath( szFile, ARRAYSIZE( szFile ), pFileNameIn );
	PStrCopy( pFileNameOut, pFileNameIn, nFileNameOutBufferChars );
	char* pszChopLow = PStrStrI( pFileNameOut, MODEL_LOW_SUBDIR );
	if ( ! pszChopLow )
		return S_FALSE;
	//ASSERT_RETFAIL( pszChopLow );
	*pszChopLow = '\0';
	PStrCat( pFileNameOut, szFile, ARRAYSIZE( szFile ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_ModelDefinitionNewFromFile( 
	const char * filename, 
	const char * filefolder,
	unsigned nFlags,
	float fDistanceToCamera )
{
#if ISVERSION( SERVER_VERSION )
	return INVALID_ID;
#else
	// CHB 2007.06.29 - When filling the pak file or using
	// Hammer, always process all LODs.
	if (AppCommonGetForceLoadAllLODs())
	{
		nFlags = MDNFF_DEFAULT;
	}

	char model_filename[DEFAULT_FILE_WITH_PATH_SIZE];
	if (filefolder)
	{
		char szFolder[DEFAULT_FILE_WITH_PATH_SIZE];
		char szFileBeforeFull[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrCopy( szFolder, filefolder, ARRAYSIZE(szFolder) );
		PStrForceBackslash( szFolder, ARRAYSIZE(szFolder) );
		PStrPrintf( szFileBeforeFull, ARRAYSIZE(szFileBeforeFull), "%s%s", szFolder, filename );
		FileGetFullFileName( model_filename, szFileBeforeFull, ARRAYSIZE(model_filename) );
	}
	else
	{
		FileGetFullFileName( model_filename, filename, ARRAYSIZE(model_filename) );
	}

	int nLOD = LOD_HIGH;
	if ( AppIsHellgate() )
	{
		// Build the path and filename for the low-poly model.
		if ((nFlags & MDNFF_IGNORE_LOW_LOD) == 0)
		{
			nLOD = LOD_LOW;
			e_ModelDefinitionGetLODFileName( model_filename, ARRAYSIZE(model_filename), model_filename );
		}
	}

	char model_folder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrGetPath( model_folder, MAX_PATH, model_filename );

	// Don't request loading of the next LOD if we don't want it.
	unsigned nRequestFlags = 0;
	if (nFlags != 0)
	{
		nRequestFlags |= MDNFF_FLAG_DONT_LOAD_NEXT_LOD;
	}

	// Source file is optional for low LOD (low LOD may not exist).
	if (nLOD == LOD_LOW)
	{
		nRequestFlags |= MDNFF_FLAG_SOURCE_FILE_OPTIONAL;
	}

	// For Hellgate, start with the low LOD first.
	int nModelDef;
	V( dxC_ModelDefinitionNewFromFileEx( &nModelDef, nLOD, INVALID_ID, model_filename, 
		model_folder, fDistanceToCamera, nRequestFlags ) );
	
	// Tell the reaper not to load the ones we're intentionally ignoring.
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

	return nModelDef;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelDefinitionReload( int nModelDef, int nLOD )
{
	ASSERT_RETFAIL( nLOD == LOD_HIGH ); // We only support reloading of high LODs.

	MODEL_DEFINITION_HASH* pModelDefHash = dxC_ModelDefinitionGetHash( nModelDef );
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
	PStrGetPath( szPath, ARRAYSIZE( szPath ), szFullFileName );
	trace( "REAPER: Reloading '%s'\n", szFullFileName );

	int nModelDefSave;
	dxC_ModelDefinitionNewFromFileEx( &nModelDefSave, LOD_HIGH, nModelDef, szFullFileName,
		szPath, 0, 0 );
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dx9_ModelDefinitionAddMesh( D3D_MODEL_DEFINITION * pModelDefinition, D3D_MESH_DEFINITION * pInMesh )
{
	if ( !pInMesh || !pModelDefinition )
		return INVALID_ID;

	ASSERT( pModelDefinition );

	ASSERT( pInMesh->nVertexBufferIndex >= 0 );
	ASSERT( pInMesh->nVertexBufferIndex <  pModelDefinition->nVertexBufferDefCount );

	// get a mesh from the big list of meshes
	D3D_MESH_DEFINITION * pNewMesh = (D3D_MESH_DEFINITION *) MALLOC( NULL, sizeof( D3D_MESH_DEFINITION ) );
	ASSERT( pNewMesh );
	memcpy ( pNewMesh, pInMesh, sizeof( D3D_MESH_DEFINITION ) );

	MESH_DEFINITION_HASH * pHash = HashAdd( gtMeshes, gnMeshDefinitionNextId++ );
	pHash->pDef = pNewMesh;
	int nMeshId = pHash->nId;

	// reference that mesh through the model
	sModelDefinitionReferenceMesh( pModelDefinition, nMeshId );

	// addref the textures when adding a mesh to a modeldef
	for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
	{
		e_TextureAddRef( pNewMesh->pnTextures[ i ] );
	}

	// if they don't tell us how many triangles to render, then compute it for them
	if ( pNewMesh->wTriangleCount == 0 )
	{
		pNewMesh->nVertexStart = 0;
		// We are computing the triangle count here - it depends on how the triangles are indexed
		if ( dxC_IndexBufferD3DExists( pNewMesh->tIndexBufferDef.nD3DBufferID ) || pNewMesh->tIndexBufferDef.pLocalData )
			pNewMesh->wTriangleCount = pNewMesh->tIndexBufferDef.nBufferSize / (3 * int(sizeof( WORD )) );
		else 
			pNewMesh->wTriangleCount = pModelDefinition->ptVertexBufferDefs[ pNewMesh->nVertexBufferIndex ].nVertexCount / 3;
	}

	// initialize the material version
	pNewMesh->nMaterialVersion = 0;

	// this is where we actually create the device-specific stuff
	V( dx9_MeshRestore ( pNewMesh ) );

	return nMeshId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelDefinitionDestroy( 
	int nModelDefinition,
	int nLOD,
	DWORD dwFlags,
	BOOL bCleanupNow /*= FALSE*/ )
{

#if ISVERSION( SERVER_VERSION )
	return;
#else

	D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( nModelDefinition, nLOD );

	if ( !pModelDefinition )
	{
		if ( dwFlags & MODEL_DEFINITION_DESTROY_DEFINITION )
		{
			// free the animated model definition hash, in case there is one
			dxC_AnimatedModelDefinitionRemovePrivate( nModelDefinition, nLOD );

			for ( int i = 0; i < LOD_COUNT; i++ )
			{
				// Definitions still exist for this hash, so don't remove it yet.
				if ( dxC_ModelDefinitionGet( nModelDefinition, i ) )
					return;
			}

			sModelDefinitionHashRemove( nModelDefinition );
		}
		return;
	}

	if ( dwFlags & MODEL_DEFINITION_DESTROY_TEXTURES )
	{
		dxC_ModelDefinitionTexturesReleaseRef( pModelDefinition, bCleanupNow );
	} 

	if ( dwFlags & MODEL_DEFINITION_DESTROY_MESHES )
	{
		for ( int i = 0; i < pModelDefinition->nMeshCount; i++ )
		{
			D3D_MESH_DEFINITION* pMesh = dx9_MeshDefinitionGet( pModelDefinition->pnMeshIds[ i ] );
			ASSERT_CONTINUE( pMesh );

			V( dxC_MeshFree( pMesh ) );
			HashRemove( gtMeshes, pModelDefinition->pnMeshIds[ i ] );
		}
		pModelDefinition->nMeshCount = 0;
	} 

	DPVS_SAFE_RELEASE( pModelDefinition->pCullTestModel );
	DPVS_SAFE_RELEASE( pModelDefinition->pCullWriteModel );

	if ( dwFlags & MODEL_DEFINITION_DESTROY_VBUFFERS )
	{
		V( sModelDefinitionRelease( pModelDefinition ) );

		for ( int i = 0; i < pModelDefinition->nVertexBufferDefCount; i++ )
		{
			D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[i];

			int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
			for ( int nStream = 0; nStream < nStreamCount; nStream++ )
			{
				FREE( NULL, pVertexBufferDef->pLocalData[ nStream ] );
				pVertexBufferDef->pLocalData[ nStream ] = NULL;
			}
		}

		FREE( NULL, pModelDefinition->ptVertexBufferDefs );
		pModelDefinition->ptVertexBufferDefs = NULL;
		pModelDefinition->nVertexBufferDefCount = 0;
	}

	if ( dwFlags & MODEL_DEFINITION_DESTROY_DEFINITION )
	{
		if ( pModelDefinition->dwFlags & MODELDEF_FLAG_ANIMATED )
		{
			// free the animated model definition hash
			dxC_AnimatedModelDefinitionRemovePrivate( nModelDefinition, nLOD );
		}

		FREE( NULL, pModelDefinition->pAttachments );
		FREE( NULL, pModelDefinition->pAdObjectDefs );
		FREE( NULL, pModelDefinition );

		MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDefinition );
		ASSERT( pHash );
		pHash->pDef[ nLOD ] = NULL;

		for ( int i = 0; i < LOD_COUNT; i++ )
		{
			// Definitions still exist for this hash, so don't remove it yet.
			if ( dxC_ModelDefinitionGet( nModelDefinition, i ) )
				return;
		}

		sModelDefinitionHashRemove( nModelDefinition );
	}
	
#endif  // end ISVERSION( SERVER_VERSION )

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
D3D_MESH_DEFINITION * dx9_ModelDefinitionGetMesh ( D3D_MODEL_DEFINITION * pModelDefinition, int nMeshIndex )
{
	ASSERT_RETNULL( pModelDefinition );

	if ( nMeshIndex < 0 || nMeshIndex >= pModelDefinition->nMeshCount )
		return NULL;

	int nMeshId = pModelDefinition->pnMeshIds[ nMeshIndex ];

	return dx9_MeshDefinitionGet( nMeshId );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

D3D_MESH_DEFINITION * dx9_ModelDefinitionGetMesh ( int nModelDefinitionId, int nLOD, int nMeshIndex )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	return dx9_ModelDefinitionGetMesh( pModelDefinition, nMeshIndex );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ModelDefinitionGetMeshCount ( int nModelDefinitionId, int nLOD )
{
	ASSERT_RETZERO(nModelDefinitionId != INVALID_ID);
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	return pModelDefinition ? pModelDefinition->nMeshCount : 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD e_ModelDefinitionGetFlags	( int nModelDefinitionId, int nLOD )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( !pModelDef )
		return 0;
	return pModelDef->dwFlags;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelDefinitionSetHashFlags ( int nModelDefinitionId, DWORD dwFlags, BOOL bSet )
{
	MODEL_DEFINITION_HASH * pModelDefHash = dxC_ModelDefinitionGetHash( nModelDefinitionId );
	if ( !pModelDefHash )
		return;

	if ( bSet )
	{
		SET_MASK( pModelDefHash->dwFlags, dwFlags );
	}
	else
	{
		CLEAR_MASK( pModelDefHash->dwFlags, dwFlags );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelDefinitionSetBoundingBox ( int nModelDefinitionId, int nLOD, const BOUNDING_BOX * pNewBox )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( pModelDefinition )
		pModelDefinition->tBoundingBox		 = *pNewBox;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ModelDefinitionSetRenderBoundingBox ( int nModelDefinitionId, int nLOD, const BOUNDING_BOX * pNewBox )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( pModelDefinition )
		pModelDefinition->tRenderBoundingBox = *pNewBox;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const BOUNDING_BOX * e_ModelDefinitionGetBoundingBox( int nModelDefinitionId, int nLOD )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	return pModelDefinition ? &pModelDefinition->tBoundingBox : NULL;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ModelDefinitionSetTextures ( 
	int nModelDefinitionId, 
	int nLOD,
	TEXTURE_TYPE eTextureType, 
	int nTextureId )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDefinition )
		return;

	for ( int i = 0; i < pModelDefinition->nMeshCount; i++ )
	{
		int nMeshId = pModelDefinition->pnMeshIds[ i ];
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( nMeshId );
		if ( ! pMesh )
			continue;
		e_TextureReleaseRef( pMesh->pnTextures[ eTextureType ] );
		pMesh->pnTextures[ eTextureType ] = nTextureId;
		e_TextureAddRef( pMesh->pnTextures[ eTextureType ] );
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sModelDefinitionRelease( D3D_MODEL_DEFINITION* pModelDef )
{
	for ( int i = 0; i < pModelDef->nVertexBufferDefCount; i++ )
	{
		D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef = &pModelDef->ptVertexBufferDefs[ i ];
		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );

		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
		{
			V( dxC_VertexBufferReleaseResource( pVertexBufferDef->nD3DBufferID[ nStream ] ) );
			pVertexBufferDef->nD3DBufferID[ nStream ] = INVALID_ID;
		}
	}

	return S_OK;
}

static	// CHB 2006.12.08
PRESULT sModelInitScrollingMaterialPhases( D3D_MODEL * pModel, int nModelDefinitionId, int nLOD, BOOL bReset )
{
	// TODO: this function should only malloc and set if any of the meshes use a scrolling UV effect

	int nMeshCount = e_ModelDefinitionGetMeshCount( nModelDefinitionId, nLOD );
	if (nMeshCount <= 0)
	{
		return S_FALSE;
	}
	if ( ! pModel->tModelDefData[nLOD].pvScrollingUVPhases )
	{
		pModel->tModelDefData[nLOD].pvScrollingUVPhases = (VECTOR2*) MALLOCZ( NULL, sizeof(VECTOR2) * NUM_MATERIAL_SCROLL_CHANNELS * nMeshCount );
		ASSERT_RETVAL( pModel->tModelDefData[nLOD].pvScrollingUVPhases, E_OUTOFMEMORY );
	}
	if ( ! pModel->tModelDefData[nLOD].pfScrollingUVOffsets )
	{
		pModel->tModelDefData[nLOD].pfScrollingUVOffsets = (float*) MALLOCZ( NULL, sizeof(float) * nMeshCount );
		ASSERT_RETVAL( pModel->tModelDefData[nLOD].pfScrollingUVOffsets, E_OUTOFMEMORY );
	}
	else if ( ! bReset )
	{
		// the data was already MALLOCed, and we don't want to reset the offsets right now
		return S_FALSE;
	}

	// this is now handled during a lazy init for async load reasons
	//for ( int i = 0; i < nMeshCount; i++ )
	//{
	//	D3D_MESH_DEFINITION * pMeshDef = dx9_ModelDefinitionGetMesh( pModel->nModelDefinitionId, i );
	//	if ( pMeshDef->nMaterialId == INVALID_ID )
	//		continue;
	//	MATERIAL * pMaterial = (MATERIAL*)DefinitionGetById( DEFINITION_GROUP_MATERIAL, pMeshDef->nMaterialId );
	//	if ( pMaterial )
	//	{
	//		//EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffect( pMaterial->tHeader.nId, dx9_GetCurrentShaderType( NULL ) );
	//		//if ( pEffectDef->bScrollUVs )
	//		if ( pMaterial->dwFlags & MATERIAL_FLAG_SCROLL_RANDOM_OFFSET )
	//			pModel->pfScrollingUVOffsets[ i ] = RandGetFloat( e_GetEngineOnlyRand(), 1.0f );
	//	}
	//}

	return S_OK;
}

// CHB 2006.12.08
PRESULT e_ModelInitScrollingMaterialPhases( int nModelID, BOOL bReset )
{
	D3D_MODEL * pModel = HashGet( gtModels, nModelID );
	ASSERT_RETFAIL( pModel );
	int nModelDefId = pModel->nModelDefinitionId;
	for (int i = 0; i < LOD_COUNT; ++i)
	{
		V( sModelInitScrollingMaterialPhases(pModel, nModelDefId, i, bReset) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static	// CHB 2006.12.08
PRESULT sModelInitSelfIlluminationPhases( D3D_MODEL * pModel, int nModelDefinitionId, int nLOD, BOOL bReset )
{
	// TODO: this function should only malloc and set if any of the meshes use a self illum variance material

	if (pModel->nModelDefinitionId < 0)
	{
		return S_FALSE;
	}
	int nMeshCount = e_ModelDefinitionGetMeshCount( nModelDefinitionId, nLOD );
	if (nMeshCount <= 0)
	{
		return S_FALSE;
	}
	if ( ! pModel->tModelDefData[nLOD].ptLightmapData )
	{
		pModel->tModelDefData[nLOD].ptLightmapData = (LightmapWaveData*) MALLOC( NULL, sizeof(LightmapWaveData) * nMeshCount );
	}
	else if ( ! bReset )
	{
		// the data was already MALLOCed, and we don't want to reset the illum phases right now
		return S_FALSE;
	}
	ASSERT_RETFAIL( pModel->tModelDefData[nLOD].ptLightmapData );
	float fRandWavePhase = RandGetFloat( e_GetEngineOnlyRand(), TWOxPI );
	float fRandNoisePhase = RandGetFloat( e_GetEngineOnlyRand(), TWOxPI );
	float fRandNoiseFreq = RandGetFloat( e_GetEngineOnlyRand(), PI * 0.5f, TWOxPI );
	for ( int i = 0; i < nMeshCount; i++ )
	{
		pModel->tModelDefData[nLOD].ptLightmapData[ i ].fWavePhase = fRandWavePhase;
		for ( int n = 0; n < SELF_ILLUM_NOISE_WAVES; n++ )
		{
			pModel->tModelDefData[nLOD].ptLightmapData[ i ].fNoisePhase[ n ] = fRandNoisePhase;
			pModel->tModelDefData[nLOD].ptLightmapData[ i ].fNoiseFreq [ n ] = fRandNoiseFreq;
		}
	}

	return S_OK;
}

// CHB 2006.12.08
PRESULT e_ModelInitSelfIlluminationPhases( int nModelID, BOOL bReset )
{
	D3D_MODEL * pModel = HashGet( gtModels, nModelID );
	ASSERT_RETINVALIDARG( pModel );
	int nModelDefId = pModel->nModelDefinitionId;
	for (int i = 0; i < LOD_COUNT; ++i)
	{
		V( sModelInitSelfIlluminationPhases(pModel, nModelDefId, i, bReset) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ModelDefinitionRestore ( 
	D3D_MODEL_DEFINITION * pModelDef, 
	BOOL bZeroOutLocalPointer,
	BOOL bCreateCullObjects /*= TRUE*/ )
{
	ASSERT_RETFAIL( pModelDef );

	if ( e_IsNoRender() )
		return S_OK;

	if ( ! dxC_IsPixomaticActive() )
	{
		for ( int i = 0; i < pModelDef->nVertexBufferDefCount; i++ )
		{
			ASSERT_CONTINUE( pModelDef->ptVertexBufferDefs );
			D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef = &pModelDef->ptVertexBufferDefs[ i ];

			int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
			for ( int nStream = 0; nStream < nStreamCount; nStream++ )
			{
				ASSERT_CONTINUE( !dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ nStream ] ) );
				ASSERT_CONTINUE( pVertexBufferDef->dwFVF != 0 || IS_VALID_INDEX( pVertexBufferDef->eVertexType, NUM_VERTEX_DECL_TYPES ) )

					BOOL bUpdatedVertexBufferDef = FALSE;

				if ( pVertexBufferDef->pLocalData[ nStream ] && pVertexBufferDef->nBufferSize[ nStream ] )
				{
					if ( !bUpdatedVertexBufferDef )
					{
						// CHB 2006.06.23 - Remap vertices for low-end if necessary.
						dx9_ModelMassageVertices(pVertexBufferDef);

						// Create and fill the vertex buffer
						//#ifdef DX9_MOST_BUFFERS_MANAGED
						pVertexBufferDef->tUsage = D3DC_USAGE_BUFFER_REGULAR;
						//#else
						//					pVertexBufferDef->tUsage = D3DC_USAGE_BUFFER_MUTABLE;
						//#endif

						bUpdatedVertexBufferDef = TRUE;
					}

					V_CONTINUE( dxC_CreateVertexBuffer( nStream, *pVertexBufferDef ) );
					V_CONTINUE( dxC_UpdateBuffer( nStream, *pVertexBufferDef, pVertexBufferDef->pLocalData[ nStream ], 0, pVertexBufferDef->nBufferSize[ nStream ] ) );

					if ( bZeroOutLocalPointer ) 
					{
						FREE(NULL, pVertexBufferDef->pLocalData[ nStream ]);
						pVertexBufferDef->pLocalData[ nStream ] = NULL;
					}

					V_CONTINUE( dxC_VertexBufferPreload( pVertexBufferDef->nD3DBufferID[ nStream ] ) );
				}
			}
		}
	}

	if ( bCreateCullObjects )
		sModelDefCreateDPVSModels( pModelDef );

#if !ISVERSION( SERVER_VERSION )
	e_ReaperUpdate();
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sReleaseModelDefinitionVBuffers()
{
	for ( MODEL_DEFINITION_HASH * pHash = HashGetFirst( gtModelDefinitions ); 
		pHash; 
		pHash = HashGetNext( gtModelDefinitions, pHash ) )
	{
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			D3D_MODEL_DEFINITION* pModelDef = pHash->pDef[ nLOD ];
			if ( pModelDef )
			{
				V( sModelDefinitionRelease( pModelDef ) );
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ModelDefinitionRestore ( int nModelDefinitionId, int nLOD )
{
	D3D_MODEL_DEFINITION *pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( pModelDefinition )
		return dx9_ModelDefinitionRestore ( pModelDefinition, FALSE );
	return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
// Not sure how else to do it right now.
//-------------------------------------------------------------------------------------------------
int e_ModelDefinitionFind( 
	const char* pszName,
	int nBufLen)
{
	for ( MODEL_DEFINITION_HASH * pHash = HashGetFirst( gtModelDefinitions ); 
		pHash; 
		pHash = HashGetNext( gtModelDefinitions, pHash ) )
	{
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			D3D_MODEL_DEFINITION * pModelDef = pHash->pDef[ nLOD ];
			if ( pModelDef && PStrICmp( pModelDef->tHeader.pszName, pszName, min( nBufLen, arrsize( pModelDef->tHeader.pszName ) ) ) == 0 )
				return pHash->nId;
		}
	}
	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const D3D_VERTEX_BUFFER_DEFINITION * dx9_ModelDefinitionGetVertexBufferDef ( 
	int nModelDefinitionId,
	int nLOD,
	int nIndex )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return NULL;
	if ( nIndex >= pModelDef->nVertexBufferDefCount || nIndex < 0 )
		return NULL;
	return &pModelDef->ptVertexBufferDefs[ nIndex ];
}

//-------------------------------------------------------------------------------------------------
static bool anyVBLockedForRead = false;
static bool anyIBLockedForRead = false;

#if defined(ENGINE_TARGET_DX10)
static SPD3DCVB currentVBStaging = NULL;
static D3D10_BUFFER_DESC currentVBDesc = {0, D3D10_USAGE_STAGING, 0, D3D10_CPU_ACCESS_READ, 0};
static SPD3DCVB currentIBStaging = NULL;
static D3D10_BUFFER_DESC currentIBDesc = {0, D3D10_USAGE_STAGING, 0, D3D10_CPU_ACCESS_READ, 0};
#endif

PRESULT dxC_LockVBReadOnly(LPD3DCVB vb, void ** ppbData) {
	
#if defined(ENGINE_TARGET_DX9) 
	anyVBLockedForRead = true;
	return vb->Lock(0, 0, ppbData, D3DLOCK_READONLY);
#endif
#if defined(ENGINE_TARGET_DX10)
	if (anyVBLockedForRead == true) ASSERTX(0, "NUTTAPONG: Can lock only 1 vertex buffer for read at a time");
	anyVBLockedForRead = true;
	D3D10_BUFFER_DESC desc;
	vb->GetDesc(&desc);	
	if (desc.ByteWidth > currentVBDesc.ByteWidth) {
		// Recreate the temporary staging buffer
		if (currentVBStaging) delete currentVBStaging;
		currentVBDesc.ByteWidth = desc.ByteWidth;
		dxC_GetDevice()->CreateBuffer(&currentVBDesc, NULL, &currentVBStaging);
	}
	dxC_GetDevice()->CopySubresourceRegion(currentVBStaging, 0, 0, 0, 0, vb, 0, NULL);
	dxC_GetDevice()->Flush(); // NUTTAPONG COMMENT: May not need this
	return currentVBStaging->Map(D3D10_MAP_READ, 0, ppbData);
#endif
}
PRESULT dxC_UnLockVBReadOnly(LPD3DCVB vb) {
	anyVBLockedForRead = false;
#if defined(ENGINE_TARGET_DX9) 
	return vb->Unlock();
#endif
#if defined(ENGINE_TARGET_DX10)
	currentVBStaging->Unmap();
	return S_OK;
#endif
}
PRESULT dxC_LockIBReadOnly(const LPD3DCIB ib, void ** ppbData) {
	anyIBLockedForRead = true;
#if defined(ENGINE_TARGET_DX9) 
	return ib->Lock(0, 0, ppbData, D3DLOCK_READONLY);
#endif
#if defined(ENGINE_TARGET_DX10)
	if (anyIBLockedForRead == true) ASSERTX(0, "NUTTAPONG: Can lock only 1 Indrx buffer for read at a time");
	anyIBLockedForRead = true;
	D3D10_BUFFER_DESC desc;
	ib->GetDesc(&desc);	
	if (desc.ByteWidth > currentIBDesc.ByteWidth) {
		// Recreate the temporary staging buffer
		if (currentIBStaging) delete currentIBStaging;
		currentIBDesc.ByteWidth = desc.ByteWidth;
		dxC_GetDevice()->CreateBuffer(&currentIBDesc, NULL, &currentIBStaging);
	}
	dxC_GetDevice()->CopySubresourceRegion(currentIBStaging, 0, 0, 0, 0, ib, 0, NULL);
	dxC_GetDevice()->Flush(); // NUTTAPONG COMMENT: May not need this
	return currentIBStaging->Map(D3D10_MAP_READ, 0, ppbData);
#endif
}
PRESULT dxC_UnLockIBReadOnly(const LPD3DCIB ib) {
	anyIBLockedForRead = false;
#if defined(ENGINE_TARGET_DX9) 
	return ib->Unlock();
#endif
#if defined(ENGINE_TARGET_DX10)
	currentIBStaging->Unmap();

	return 0;

#endif
}
//-------------------------------------------------------------------------------------------------
BOOL e_ModelDefinitionCollide( 
	int nModelDefinitionId, 
	const VECTOR & vTestStart, 
	const VECTOR & vDirection,
	int * pnMeshId,
	int * pnTriangleId,
	VECTOR * pvPositionOnMesh,
	float * pfU_Out,
	float * pfV_Out,
	VECTOR * pvNormal,
	float * pfMaxDistance,
	BOOL bReturnNegative)
{
#if defined(ENGINE_TARGET_DX9)  //NUTTAPONG TODO: Use DX10 Staging buffers to accomplish same thing
	D3D_MODEL_DEFINITION* pDef = dxC_ModelDefinitionGet ( nModelDefinitionId, LOD_ANY );
	if ( ! pDef )
		return FALSE;

	//ASSERT_RETFALSE(pDef->pVertexBuffers->pVertexDeclaration);

	float fDistMin;
	if (pfMaxDistance)
	{
		fDistMin = *pfMaxDistance;
	}
	else
	{
		fDistMin = 1.0f;
	}
	float fBestU = 0.0f;
	float fBestV = 0.0f;
	BOOL bFound = FALSE;
	VECTOR vNormal;
	VECTOR * pvNorm1 = NULL;
	VECTOR * pvNorm2 = NULL;
	VECTOR * pvNorm3 = NULL;

	for ( int i = 0; i < pDef->nMeshCount; i++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pDef->pnMeshIds[ i ] );
		if ( ! pMesh )
			continue;

		// don't look at the debug meshes
		if ( pMesh->dwFlags & ( MESH_FLAG_DEBUG | MESH_FLAG_COLLISION | MESH_FLAG_CLIP | MESH_FLAG_SILHOUETTE | MESH_FLAG_HULL ) )
			continue;

		MATERIAL * pMaterial = (MATERIAL*) DefinitionGetById( DEFINITION_GROUP_MATERIAL, pMesh->nMaterialId );
		if ( ! pMaterial)
			continue;

		const SHADER_TYPE_DEFINITION * pShaderDef = (const SHADER_TYPE_DEFINITION*)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_SHADERS, pMaterial->nShaderLineId );
		if ( ! pShaderDef)
			continue;

		if (pShaderDef->bNoCollide)
			continue;

		ASSERT_RETFALSE( pMesh->nVertexBufferIndex < pDef->nVertexBufferDefCount );
		ASSERT_RETFALSE( pMesh->nVertexBufferIndex >= 0 );

		D3D_VERTEX_BUFFER_DEFINITION* pVBufferDef = &pDef->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

		
		BYTE * pVertices = NULL;
		// not sure how horrible this lock is... we don't call it often though...
		D3D_VERTEX_BUFFER* pD3DVB = dxC_VertexBufferGet( pVBufferDef->nD3DBufferID[ 0 ] );
		ASSERT_CONTINUE( pD3DVB && pD3DVB->pD3DVertices );
		V_CONTINUE( dxC_LockVBReadOnly(pD3DVB->pD3DVertices,(void **)&pVertices) );

		if ( ! pVertices )
		{
			V( dxC_UnLockVBReadOnly(pD3DVB->pD3DVertices) );
			continue;
		}

		INDEX_BUFFER_TYPE * pwIndices = NULL;
		// not sure how horrible this lock is... we don't call it often though...
		D3D_INDEX_BUFFER * pD3DIB = dxC_IndexBufferGet( pMesh->tIndexBufferDef.nD3DBufferID );
		ASSERT_CONTINUE( pD3DIB && pD3DIB->pD3DIndices );
		V_DO_FAILED( dxC_LockIBReadOnly(pD3DIB->pD3DIndices, (void **)&pwIndices) )
		{
			V( dxC_UnLockVBReadOnly(pD3DVB->pD3DVertices) );
			continue;
		}

		if ( ! pwIndices )
		{
			V( dxC_UnLockVBReadOnly(pD3DVB->pD3DVertices) );
			V( dxC_UnLockIBReadOnly(pD3DIB->pD3DIndices) );
			continue;
		}

		int nVertexSize = dxC_GetVertexSize( 0, pVBufferDef );
		BYTE * pbVertexFirst = pVertices + pMesh->nVertexStart * nVertexSize;
		for ( int j = 0; j < pMesh->wTriangleCount; j++ )
		{
			int nTriangleStart = j * 3;
			VECTOR * pvPos1 = (VECTOR *) (pbVertexFirst + pwIndices[ nTriangleStart + 0 ] * nVertexSize);
			VECTOR * pvPos2 = (VECTOR *) (pbVertexFirst + pwIndices[ nTriangleStart + 1 ] * nVertexSize);
			VECTOR * pvPos3 = (VECTOR *) (pbVertexFirst + pwIndices[ nTriangleStart + 2 ] * nVertexSize);

			float fDist = fDistMin;
			float fU = 0.0f;
			float fV = 0.0f;
			if ( ! RayIntersectTriangle( &vTestStart, &vDirection, 
										*pvPos1, *pvPos2, *pvPos3, 
										&fDist, &fU, &fV ) )
				continue;

			if ( fDist >= fDistMin )
				continue;

			if ( !bReturnNegative && fDist < 0.0f )
				continue;

			pvNorm1 = pvPos1;
			pvNorm2 = pvPos2;
			pvNorm3 = pvPos3;

			fDistMin = fDist;
			if (pfU_Out)
				*pfU_Out = fU;
			if (pfV_Out)
				*pfV_Out = fV;
			if (pnMeshId)
				*pnMeshId = i;
			if (pnTriangleId)
				*pnTriangleId = j;
			bFound = TRUE;
		}
		V( dxC_UnLockVBReadOnly(pD3DVB->pD3DVertices) );
		V( dxC_UnLockIBReadOnly(pD3DIB->pD3DIndices) );
		
	}

	if ( bFound )
	{
		ASSERT_RETFALSE( pvNorm1 && pvNorm2 && pvNorm3 );

		VECTOR vDelta = vDirection;
		vDelta *= fDistMin;
		if (pvPositionOnMesh)
			*pvPositionOnMesh = vTestStart + vDelta;

		VECTOR vNorm1, vNorm2;
		vNormal = VECTOR(0.f);
		VectorAdd(vNorm1, *pvNorm2, -*pvNorm3);
		VectorAdd(vNorm2, *pvNorm2, -*pvNorm1);
		VectorCross(vNormal, vNorm1, vNorm2);
		VectorNormalize(vNormal);
		if (pvNormal)
			*pvNormal = vNormal;

		if (pfMaxDistance)
			*pfMaxDistance = fDistMin;
	}

	return bFound;
#endif
	DX10_BLOCK( return true; )
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ModelDefinitionsSwap( 
	int nFirstId, 
	int nSecondId )
{
	MODEL_DEFINITION_HASH * pFirstHash  = HashGet( gtModelDefinitions, nFirstId );
	MODEL_DEFINITION_HASH * pSecondHash = HashGet( gtModelDefinitions, nSecondId );

	if ( !pFirstHash || !pSecondHash )
	{
		return FALSE;
	}

	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		ASSERT( !pFirstHash->pDef[ nLOD ] || pFirstHash->pDef[ nLOD ]->tHeader.nId == nFirstId );
		ASSERT( !pSecondHash->pDef[ nLOD ] || pSecondHash->pDef[ nLOD ]->tHeader.nId == nSecondId );

		// not sure if this is necessary, but just in case...
		if ( pFirstHash->pDef[ nLOD ] )
			pFirstHash->pDef[ nLOD ]->tHeader.nId = nSecondId;

		if ( pSecondHash->pDef[ nLOD ])
			pSecondHash->pDef[ nLOD ]->tHeader.nId = nFirstId;

		D3D_MODEL_DEFINITION* pModelDefSwap = pFirstHash->pDef[ nLOD ];
		pFirstHash ->pDef[ nLOD ] = pSecondHash->pDef[ nLOD ];
		pSecondHash->pDef[ nLOD ] = pModelDefSwap;
	}

	V( dx9_ModelClearPerMeshDataForDefinition( nFirstId ) );
	V( dx9_ModelClearPerMeshDataForDefinition( nSecondId ) );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dx9_ModelDefinitionNew( int nPlaceholderModelDef )
{
#if ISVERSION( SERVER_VERSION )
	return INVALID_ID;
#else
	MODEL_DEFINITION_HASH * pHash = sModelDefinitionHashAdd();

	//MODEL_DEFINITION_HASH * pHashPlaceholder = HashGet( gtModelDefinitions, nPlaceholderModelDef );
	//if ( pHashPlaceholder )
	//	pHash->pDef = pHashPlaceholder->pDef;
	//else

	return pHash->nId;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL dx9_ModelDefinitionShouldRender(
	const D3D_MODEL_DEFINITION * pModelDefinition,
	const D3D_MODEL * pModel,
	DWORD dwModelDefFlagsDoDraw /*= 0*/,
	DWORD dwModelDefFlagsDontDraw /*= 0*/ )
{
	DWORD dwFlags = pModelDefinition->dwFlags;
	BOOL bRender = FALSE;

	for ( int nMatType = 0; nMatType < NUM_MATERIAL_OVERRIDE_TYPES; nMatType++ )
	{
		// if the model has a material override that forces alpha rendering, need to filter here appropriately
		int nMaterialOverride = dx9_ModelGetMaterialOverride( pModel, (MATERIAL_OVERRIDE_TYPE)nMatType );
		if ( nMaterialOverride != INVALID_ID )
		{
			// this isn't perfect, because maybe the final render will use a different shader type
			EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialOverride, e_GetCurrentLevelLocation() );
			if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
			{
				dwFlags |= MODELDEF_FLAG_HAS_ALPHA;
			}
		}

		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA ) )
			dwFlags |= MODELDEF_FLAG_HAS_ALPHA;

		if ( dwModelDefFlagsDoDraw   && ( dwFlags & dwModelDefFlagsDoDraw   ) != dwModelDefFlagsDoDraw )
			continue;
		if ( dwModelDefFlagsDontDraw && ( dwFlags & dwModelDefFlagsDontDraw ) != 0 )
			continue;
		bRender = TRUE;
	}

	return bRender;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDefinitionsCleanup()
{
	MODEL_DEFINITION_HASH * pModelDefHash = dx9_ModelDefinitionGetFirst();
	while ( pModelDefHash )
	{
		MODEL_DEFINITION_HASH* pModelDefHashNext = dx9_ModelDefinitionGetNext( pModelDefHash );
		if ( pModelDefHash->tRefCount.GetCount() == 0 )
		{
			for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
			{
				D3D_MODEL_DEFINITION* pDef = pModelDefHash->pDef[ nLOD ];
				if ( pDef )
				{
					// clean it!
					trace( "Model definition cleanup: %d (lod %d) [ %s ]\n", 
						pDef->tHeader.nId, nLOD, pDef->tHeader.pszName );
					e_ModelDefinitionDestroy( pDef->tHeader.nId, nLOD, 
						MODEL_DEFINITION_DESTROY_ALL, TRUE );
				}
			}			
		}
		pModelDefHash = pModelDefHashNext;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ModelDefinitionAddRef( int nModelDefinitionId )
{
	dxC_ModelDefinitionAddRef( HashGet( gtModelDefinitions, nModelDefinitionId ) );
}

void e_ModelDefinitionReleaseRef(
	int nModelDefinitionId,
	BOOL bCleanupNow /*= FALSE*/ )
{
	MODEL_DEFINITION_HASH * pHash = HashGet( gtModelDefinitions, nModelDefinitionId );
	dxC_ModelDefinitionReleaseRef( pHash );

	if ( bCleanupNow && pHash && pHash->tRefCount.GetCount() <= 0 )
	{
		for ( int nLOD = 0; nLOD < LOD_COUNT; ++nLOD )
			e_ModelDefinitionDestroy( pHash->nId, nLOD, MODEL_DEFINITION_DESTROY_ALL, bCleanupNow );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_ModelDefinitionAddRef( MODEL_DEFINITION_HASH* pModelDefHash )
{
	if ( ! pModelDefHash )
		return;

	pModelDefHash->tRefCount.AddRef();
}

void dxC_ModelDefinitionReleaseRef( MODEL_DEFINITION_HASH * pModelDefHash )
{	
	if ( ! pModelDefHash )
		return;

	pModelDefHash->tRefCount.Release();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_ModelDefinitionTexturesAddRef( D3D_MODEL_DEFINITION * pModelDef )
{
	if ( ! pModelDef )
		return;

	// never used!

	//for ( int nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	//{
	//	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
	//	ASSERT_CONTINUE(pMesh);
	//	for ( int nType = 0; nType < NUM_TEXTURE_TYPES; nType++ )
	//	{
	//		if ( e_TextureIsValidID( pMesh->pnTextures[ nType ] ) )
	//		{
	//			e_TextureAddRef( pMesh->pnTextures[ nType ] );
	//		}
	//		else
	//		{
	//			pMesh->pnTextures[ nType ] = INVALID_ID;
	//		}
	//	}
	//}
}

void dxC_ModelDefinitionTexturesReleaseRef( D3D_MODEL_DEFINITION * pModelDef, BOOL bCleanupNow /*= FALSE*/ )
{	
	if ( ! pModelDef )
		return;

	for ( int nMesh = 0; nMesh < pModelDef->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ nMesh ] );
		ASSERT_CONTINUE(pMesh);
		for ( int nType = 0; nType < NUM_TEXTURE_TYPES; nType++ )
		{
			e_TextureReleaseRef( pMesh->pnTextures[ nType ], bCleanupNow );
			pMesh->pnTextures[ nType ] = INVALID_ID;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ModelDefinitionExists( int nModelDefinitionId, int nLOD /*= LOD_NONE*/ )
{
	if ( nLOD == LOD_NONE )
		return ( HashGet( gtModelDefinitions, nModelDefinitionId ) != NULL );
	else
		return ( dxC_ModelDefinitionGet( nModelDefinitionId, nLOD ) != NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDefinitionGetSizeInMemory( int nModelDefinitionId, ENGINE_MEMORY * pMem, int nLOD /*= LOD_NONE*/, BOOL bSkipTextures /*= FALSE*/ )
{
	ASSERT_RETINVALIDARG( pMem );

	D3D_MODEL_DEFINITION* pModelDef = NULL;
	for ( int nLODIndex = 0; nLODIndex < LOD_COUNT; nLODIndex++ )
	{
		if ( nLOD != LOD_NONE && nLOD != nLODIndex )
			continue;

		pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLODIndex );
		if ( pModelDef )
		{
			for ( int i = 0; i < pModelDef->nVertexBufferDefCount; i++ )
			{
				DWORD dwBytes = 0;
				DWORD dwBuffers = 0;
				V_CONTINUE( dxC_VertexBufferDefGetSizeInMemory( pModelDef->ptVertexBufferDefs[ i ], &dwBytes, &dwBuffers ) );
				pMem->dwVBufferBytes += dwBytes;
				pMem->dwVBuffers += dwBuffers;
			}
			for ( int i = 0; i < pModelDef->nMeshCount; i++ )
			{
				D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );
				ASSERT_CONTINUE( pMesh );

				if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
				{
					DWORD dwBytes = 0;
					V_CONTINUE( dxC_IndexBufferDefGetSizeInMemory( pMesh->tIndexBufferDef, &dwBytes ) );
					pMem->dwIBufferBytes += dwBytes;
					pMem->dwIBuffers++;
				}

				if ( !bSkipTextures )
				{
					for ( int t = 0; t < NUM_TEXTURE_TYPES; t++ )
					{
						if ( ! e_TextureIsLoaded( pMesh->pnTextures[ t ] ) )
							continue;
						DWORD dwBytes = 0;
						V( e_TextureGetSizeInMemory( pMesh->pnTextures[ t ], &dwBytes ) );
						pMem->dwTextureBytes += dwBytes;
						pMem->dwTextures++;
					}
				}
			}
		}
	}

	ASSERT_RETFAIL( pModelDef );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDefinitionsDumpMemUsage()
{
	// probably has lots of overlap, so it will error high :/

	//ENGINE_MEMORY tMemory;
	//tMemory.Zero();

	//MODEL_DEFINITION_HASH * pModelDefHash = dx9_ModelDefinitionGetFirst();
	//while ( pModelDefHash )
	//{
	//	MODEL_DEFINITION_HASH * pModelDefHashNext = dx9_ModelDefinitionGetNext( pModelDefHash );
	//	if ( ! pModelDefHash->pDef )
	//	{
	//		pModelDefHash = pModelDefHashNext;
	//		continue;
	//	}
	//	D3D_MODEL_DEFINITION * pDef = pModelDefHash->pDef;

	//	e_ModelDefintionGetSizeInMemory( pDef->tHeader.nId, &tMemory );

	//	pModelDefHash = pModelDefHashNext;
	//}

	//tMemory.DebugTrace( "Model definitions memory:\n" );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDefinitionGetAdObjectDefCount(
	int nModelDefinitionId,
	int nLOD,
	int & nCount )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return S_FALSE;

	nCount = pModelDef->nAdObjectDefCount;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ModelDefinitionGetAdObjectDef(
	int nModelDefinitionId,
	int nLOD, 
	int nIndex,
	struct AD_OBJECT_DEFINITION & tAdObjDef )
{
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDef )
		return S_FALSE;

	ASSERT_RETINVALIDARG( IS_VALID_INDEX( nIndex, pModelDef->nAdObjectDefCount ) );

	tAdObjDef = pModelDef->pAdObjectDefs[ nIndex ];

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void sUpdateBackgroundModelCallback( const FILE_ITERATE_RESULT *pResult, void *pUserData )
{
	BGMU_DATA * pData = (BGMU_DATA*)pUserData;
	//ASSERT_RETURN( pData->pfn_AddStr );

	// Check that this GR2 file is in the levels or props spreadsheet
	if ( ! e_BackgroundModelFileInExcel( pResult->szFilenameRelativepath ) )
		return;

	const int cnBufLen = 256;
	//char szMsg[ cnBufLen ];

	OS_PATH_CHAR szFilenameRelPathModel[ DEFAULT_FILE_WITH_PATH_SIZE ];
	OS_PATH_CHAR szFilenameRelPathAO[ DEFAULT_FILE_WITH_PATH_SIZE ];

	OS_PATH_CHAR szTemp[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCvt( szTemp, pResult->szFilenameRelativepath, DEFAULT_FILE_WITH_PATH_SIZE );
	PStrReplaceExtension( szFilenameRelPathModel, DEFAULT_FILE_WITH_PATH_SIZE, szTemp, L"m" );
	PStrReplaceExtension( szFilenameRelPathAO, DEFAULT_FILE_WITH_PATH_SIZE, szTemp, L"mao" );

	if ( pData->dwFlags & BGMU_FLAG_CHECK_OUT_FILES )
	{
		// only checking out the MAO file
		TCHAR szCheckout[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrCvt( szCheckout, szFilenameRelPathAO, DEFAULT_FILE_WITH_PATH_SIZE );
		FileCheckOut( szCheckout );

		//PStrPrintf( szMsg, cnBufLen, "%s",  );
		if ( pData->pfn_AddStr )
			pData->pfn_AddStr( "Opened for add/edit: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathAO), pData->pContext, cnBufLen );
	}

	if ( pData->dwFlags & BGMU_FLAG_FORCE_UPDATE )
	{
		BOOL bDeleted = FileDelete( szFilenameRelPathModel );

		if ( pData->pfn_AddStr )
		{
			if ( bDeleted )
				pData->pfn_AddStr( "Deleted: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathModel), pData->pContext, cnBufLen );
			else
				pData->pfn_AddStr( "Failed to delete: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathModel), pData->pContext, cnBufLen );
		}

		if ( pData->dwFlags & BGMU_FLAG_FORCE_UPDATE_OBSCURANCE )
		{
			BOOL bDeleted = FileDelete( szFilenameRelPathAO );

			if ( pData->pfn_AddStr )
			{
				if ( bDeleted )
					pData->pfn_AddStr( "Deleted: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathAO), pData->pContext, cnBufLen );
				else
					pData->pfn_AddStr( "Failed to delete: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathAO), pData->pContext, cnBufLen );
			}
		}
	}

	if ( pData->pfn_AddStr )
		pData->pfn_AddStr( "Converting: ", pResult->szFilenameRelativepath, pData->pContext, cnBufLen );

	char szStripped[MAX_PATH];
	PRESULT pr = e_ModelDefinitionStripLODFileName( szStripped, MAX_PATH, pResult->szFilenameRelativepath );
	ASSERT_RETURN( SUCCEEDED(pr) );
	int nLOD;
	if ( pr == S_FALSE )
		nLOD = LOD_HIGH;
	else
		nLOD = LOD_LOW;

	// Convert it first to make sure the model is up to date.
	int nModelDef = INVALID_ID;
	V( dxC_ModelDefinitionNewFromFileEx( &nModelDef, nLOD, INVALID_ID, pResult->szFilenameRelativepath, 
		"", 0.f, 0 ) );
	// Destroy it and reload it.
	e_ModelDefinitionDestroy( nModelDef, nLOD, MODEL_DEFINITION_DESTROY_ALL & ~(MODEL_DEFINITION_DESTROY_TEXTURES) );
	V( dxC_ModelDefinitionNewFromFileEx( &nModelDef, nLOD, INVALID_ID, pResult->szFilenameRelativepath, 
		"", 0.f, 0 ) );


	// Free this LOD's vbuffers.
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( nModelDef, nLOD );
	ASSERT_RETURN( pModelDef )

	V( sModelDefinitionRelease( pModelDef ) );

	// make a list of mesh defs first -- we need these to render
	D3D_MESH_DEFINITION ** ppMeshDefs = (D3D_MESH_DEFINITION**)MALLOCZ( NULL, sizeof(D3D_MESH_DEFINITION**) * pModelDef->nMeshCount );
	for ( int i = 0; i < pModelDef->nMeshCount; i++ )
		ppMeshDefs[i] = dx9_MeshDefinitionGet( pModelDef->pnMeshIds[ i ] );

	V( dxC_ObscuranceComputeForModelDefinition( ppMeshDefs, pModelDef, TRUE, FALSE ) );

	FREE( NULL, ppMeshDefs );


	// Free this LOD to reload later.
	e_ModelDefinitionDestroy( nModelDef, nLOD, MODEL_DEFINITION_DESTROY_ALL & ~(MODEL_DEFINITION_DESTROY_TEXTURES) );

	// Delete the model file again, to force integration of the new MAO.
	BOOL bDeleted = FileDelete( szFilenameRelPathModel );

	// Load the modeldef again to incorporate the new MAO file(s).
	int nNewModelDef;
	V( dxC_ModelDefinitionNewFromFileEx( &nNewModelDef, nLOD, INVALID_ID, pResult->szFilenameRelativepath, 
		"", 0.f, 0 ) );

	if ( pData->dwFlags & BGMU_FLAG_CLEANUP )
	{
		V( e_ModelDefinitionsCleanup() );
		V( e_TexturesCleanup() );
	}

	if ( pData->dwFlags & BGMU_FLAG_CHECK_OUT_FILES )
	{
		// only checking out the MAO file
		TCHAR szCheckout[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrCvt( szCheckout, szFilenameRelPathAO, DEFAULT_FILE_WITH_PATH_SIZE );
		FileCheckOut( szCheckout );

		//PStrPrintf( szMsg, cnBufLen, "%s",  );
		if ( pData->pfn_AddStr )
			pData->pfn_AddStr( "Opened for add/edit: ", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilenameRelPathAO), pData->pContext, cnBufLen );
	}

	// let async complete the texture file loads/frees
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	Sleep(0);
}


PRESULT e_ModelsUpdateAllBackgroundModels( const char * pszPath, BGMU_DATA * pData )
{
	// iterate and update all background model files in this path
	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	FileGetFullFileName( szPath, pszPath, DEFAULT_FILE_WITH_PATH_SIZE );

	FilesIterateInDirectory( szPath, "*.gr2", sUpdateBackgroundModelCallback, pData, !!(pData->dwFlags & BGMU_FLAG_INCLUDE_SUBDIRS) );

	return S_OK;
}


PRESULT e_ModelUpdateBackgroundModel( const char * pszFile, BGMU_DATA * pData )
{
	// iterate and update all background model files in this path
	FILE_ITERATE_RESULT tFIR;
	tFIR.szFilename[0] = NULL;

	FileGetFullFileName( tFIR.szFilenameRelativepath, pszFile, MAX_PATH );
	sUpdateBackgroundModelCallback( &tFIR, (void*)pData );

	return S_OK;
}

PRESULT e_ModelUpdateBackgroundModel( int nModelID, BGMU_DATA * pData )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return S_FALSE;

	D3D_MODEL_DEFINITION * pModelDef = NULL;
	int nLOD = LOD_NONE;
	for ( nLOD = LOD_COUNT - 1; nLOD >= 0; nLOD-- )
	{
		pModelDef = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, nLOD );
		if ( pModelDef )
			break;
	}

	ASSERT_RETFAIL( pModelDef );

	char szFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCopy( szFilename, DEFAULT_FILE_WITH_PATH_SIZE, pModelDef->tHeader.pszName, MAX_XML_STRING_LENGTH );
	e_ModelDefinitionDestroy( pModelDef->tHeader.nId, nLOD, MODEL_DEFINITION_DESTROY_ALL );

	V( e_ModelUpdateBackgroundModel( szFilename, pData ) );

	// Get the new modeldef and set the background model to use it.
	int nModelDef = INVALID_ID;
	V_RETURN( dxC_ModelDefinitionNewFromFileEx( &nModelDef, nLOD, INVALID_ID, szFilename, 
		"", 0.f, MDNFF_FLAG_FORCE_SYNC_LOAD ) );
	V( e_ModelSetDefinition( nModelID, nModelDef, nLOD ) );

	return S_FALSE;
}
