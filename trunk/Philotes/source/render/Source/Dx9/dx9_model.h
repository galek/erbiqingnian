//----------------------------------------------------------------------------
// dx9_model.h
//
// - Header for DX9-specific model functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_MODEL__
#define __DX9_MODEL__

#include "dxC_occlusion.h"
//#include "dxC_texture.h"
#include "boundingbox.h"
#include "e_region.h"
#include "e_definition.h"
//#include "dxC_effect.h"

#include "e_attachment.h"
#include "e_settings.h"

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void dxC_ModelDrawLayerToDrawFlags( DRAW_LAYER eDrawLayer, DWORD & dwFlags )
{
	BYTE byDrawLayer = static_cast<BYTE>(eDrawLayer);
	ASSERT_RETURN( byDrawLayer < (1 << MODEL_BITS_DRAW_LAYER) );
	DWORD dwMask = byDrawLayer << MODEL_SHIFT_DRAW_LAYER;
	dwFlags &= ~MODEL_MASK_DRAW_LAYER;
	dwFlags |= dwMask;
}

inline DRAW_LAYER dxC_ModelDrawLayerFromDrawFlags( DWORD dwFlags )
{
	DWORD dwMask = (dwFlags & MODEL_MASK_DRAW_LAYER);
	return (DRAW_LAYER)(BYTE)(dwMask >> MODEL_SHIFT_DRAW_LAYER);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL dx9_ModelIsVisible( const D3D_MODEL * pModel ) 
{
	if (!pModel)
	{
		return FALSE;
	}
	//if ( ( pModel->dwFlags & MODEL_FLAG_VISIBLE )			== 0 &&
	//	( pModel->dwFlags & MODEL_FLAG_FIRST_PERSON_PROJ ) == 0 )
	//	return FALSE;
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NODRAW ) )
		return FALSE;
	DRAW_LAYER eDrawLayer = dxC_ModelDrawLayerFromDrawFlags( pModel->dwDrawFlags );
	if ( eDrawLayer != DRAW_LAYER_UI
		&& pModel->nRegion != e_GetCurrentRegionID() && ! c_GetToolMode() )
		return FALSE;
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NODRAW_TEMPORARY ) )
		return FALSE;
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL dx9_ModelShouldRender( D3D_MODEL * pModel ) 
{
	if ( ! dx9_ModelIsVisible( pModel ) )
		return FALSE;
	if ( pModel->nModelDefinitionId == INVALID_ID )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

int  dx9_ModelDefinitionNewFromMemory ( D3D_MODEL_DEFINITION* pModelDefinition, int nLOD, BOOL bRestore, int nId = INVALID_ID );
int  dx9_ModelDefinitionNew( int nPlaceholderModelDef );
int  dx9_ModelDefinitionAddMesh		  ( D3D_MODEL_DEFINITION * pModelDefinition, D3D_MESH_DEFINITION * pSavedMesh );
//D3D_VERTEX_BUFFER *    dx9_ModelDefinitionAddVertexBuffer    ( D3D_MODEL_DEFINITION * pModelDefinition, int * pnIndex );
const D3D_VERTEX_BUFFER_DEFINITION * dx9_ModelDefinitionGetVertexBufferDef ( int nModelDefinitionId, int nIndex );
D3D_MESH_DEFINITION *  dx9_ModelDefinitionGetMesh ( D3D_MODEL_DEFINITION * pModelDefinition, int nMeshIndex );
D3D_MESH_DEFINITION *  dx9_ModelDefinitionGetMesh ( int nModelDefinitionId, int nMeshIndex );
PRESULT dx9_ModelDefinitionRestore (
	D3D_MODEL_DEFINITION * pModelDef,
	BOOL bZeroOutLocalPointer,
	BOOL bCreateCullObjects = TRUE );

PRESULT dx9_MeshRestore ( D3D_MESH_DEFINITION * pMesh );
PRESULT dx9_ModelSetOccluded( D3D_MODEL * pModel, BOOL bOccluded );
BOOL dx9_ModelGetOccluded( D3D_MODEL * pModel );
BOOL dx9_ModelGetInFrustum( D3D_MODEL * pModel );
VECTOR * dx9_GetMeshRenderOBBInWorld( const D3D_MODEL * pModel, int nLOD, int nMeshIndex );	// CHB 2006.12.08
BOOL dx9_ModelGetInFrustumPrev( D3D_MODEL * pModel );
int dx9_ModelGetTexture(
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh,			// If pMesh is NULL, it looks it up
	int nMesh,
	TEXTURE_TYPE eType,
	int nLOD,
	BOOL bNoOverride = FALSE,
	const MATERIAL * pMaterial = NULL );
bool dx9_ModelHasSpecularMap(D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, const D3D_MESH_DEFINITION * pMesh, int nMesh, const MATERIAL * pMaterial, int nLOD);	// CHB 2007.09.25

int dx9_ModelGetTextureOverride( D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, int nMesh, TEXTURE_TYPE eType, int nLOD );	// CHB 2006.11.28
DWORD dx9_ModelGetMeshTechniqueMaskOverride( const D3D_MODEL * pModel, const D3D_MODEL_DEFINITION * pModelDef, int nMesh, int nLOD );	// CHB 2006.11.28
BOOL dx9_MeshIsVisible( D3D_MODEL * pModel, const D3D_MESH_DEFINITION * pMesh ) ;
int dx9_MeshGetLODPriority( D3D_MESH_DEFINITION * pMesh );
PRESULT dxC_ModelSetFlagbit( D3D_MODEL * pModel, MODEL_FLAGBIT eFlagbit, BOOL bSet );
//PRESULT dx9_ModelSetVisible( D3D_MODEL * pModel, BOOL bSet );
PRESULT dx9_ModelSetAlpha( D3D_MODEL * pModel, float fAlpha );
int dx9_MeshGetMaterial( D3D_MODEL * pModel, const D3D_MESH_DEFINITION * pMesh, MATERIAL_OVERRIDE_TYPE eMatOverrideType = MATERIAL_OVERRIDE_NORMAL );
int dx9_ModelGetMaterialOverride( const D3D_MODEL * pModel, MATERIAL_OVERRIDE_TYPE eMatOverrideType = MATERIAL_OVERRIDE_NORMAL );
int dx9_ModelGetMaterialOverrideAttachment( const D3D_MODEL * pModel, MATERIAL_OVERRIDE_TYPE eMatOverrideType = MATERIAL_OVERRIDE_NORMAL );
PRESULT dx9_ModelTexturesPreload( D3D_MODEL * pModel );
PRESULT dx9_MeshUpdateMaterialVersion( D3D_MESH_DEFINITION * pMeshDef );
PRESULT dx9_MeshRefreshMaterial( D3D_MESH_DEFINITION * pMeshDef, int nDiffuseTexture );

PRESULT dx9_ModelClearPerMeshData( D3D_MODEL * pModel );
PRESULT dx9_ModelClearPerMeshDataForDefinition( int nModelDefID );

PRESULT dx9_ModelAddToProxMap( D3D_MODEL * pModel, BOOL bStaticModel = FALSE );
PRESULT dx9_ModelMoveInProxmap( D3D_MODEL * pModel );
PRESULT dx9_ModelRemoveFromProxmap( D3D_MODEL * pModel );

#define MODEL_DEFINITION_CREATE_FLAG_BACKGROUND				MAKE_MASK(0)
#define MODEL_DEFINITION_CREATE_FLAG_SHOULD_LOAD_TEXTURES	MAKE_MASK(1)

PRESULT dx9_ModelDefinitionFromFileInMemory(
	int* pnModelDefID,
	int nModelDefIdOverride,
	int nLOD, 
	const char * pszFilename,
	const char * pszFolderIn,
	D3D_MODEL_DEFINITION* model_definition,
	D3D_MESH_DEFINITION* pMeshDefs[],
	DWORD dwFlags,
	float fDistanceToCamera );

BOOL dx9_ModelDefinitionShouldRender(
	const D3D_MODEL_DEFINITION * pModelDefinition,
	const D3D_MODEL * pModel,
	DWORD dwModelDefFlagsDoDraw = 0,
	DWORD dwModelDefFlagsDontDraw = 0 );

//void dx9_MeshGetFolders( 
//	D3D_MESH_DEFINITION * pMeshDefinition, 
//	char * szFolder, 
//	char * szOverride, 
//	const char * pszFolderIn, 
//	int nStrSize, 
//	BOOL bBackground );

PRESULT dx9_MeshApplyMaterial( 
	D3D_MESH_DEFINITION * pMeshDefinition, 
	D3D_MODEL_DEFINITION * pModelDef,
	D3D_MODEL * pModel,
	int nLOD,	// CHB 2006.12.08
	int nMesh,
	BOOL bForceSyncLoadMaterial = FALSE );

D3DCOLOR dx9_CompressVectorToColor ( const D3DXVECTOR3 & vInVector );
D3DCOLOR dx9_CompressVectorToColor ( const D3DXVECTOR4 & vInVector );


PRESULT dx9_GrannyGetTextCoordsAtPos( 
	struct D3D_MODEL_DEFINITION * pModelDef,
	struct D3D_MESH_DEFINITION * pMesh, 
	int nTriangleId, 
	BOOL bLightmap, 
	float & fU, 
	float & fV );

const BOUNDING_BOX * dx9_MeshGetBoundingBox(
	D3D_MODEL_DEFINITION * pModelDef,
	int nMeshIndex );

//int dx9_MeshMakeBoundingBox(
//	D3D_MESH_DEFINITION * pMesh,
//	D3D_VERTEX_BUFFER * pVBuffer,
//	int nIndexCount,
//	INDEX_BUFFER_TYPE * pwIBuffer );

//void dx9_CollapseMeshByUsage ( 
//	D3D_VERTEX_BUFFER * pVertexBuffer, 
//	WORD * pwTriangleCount,
//	int * pnIndexBufferSize,
//	INDEX_BUFFER_TYPE * pwIndexBuffer,
//	int nMeshTriangleCount, 
//	BOOL * pbIsSubsetTriangle, 
//	BOOL * pbIsSubsetVertex );

//-------------------------------------------------------------------------------------------------


#endif // __DX9_MODEL__