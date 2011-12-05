
// dxC_model.h
//
// - Header for model functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_MODEL__
#define __DXC_MODEL__

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

#include "e_granny_rigid.h"
#include "e_model.h"
#ifndef __LOD__
#include "lod.h"
#endif
#include "e_features.h"
#include "e_attachment.h"
#include "e_adclient.h"
#include "boundingbox.h"
#include "refcount.h"
#include "e_budget.h"

#include "dxC_occlusion.h"
#include "dxC_light.h"
#include "dxC_effect.h"
#include "dxC_FVF.h"
#include "dxC_primitive.h"
#include "dxC_buffer.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MODEL_FILE_VERSION 							(154 + GLOBAL_FILE_VERSION)
#define MIN_REQUIRED_MODEL_FILE_VERSION 			(154 + GLOBAL_FILE_VERSION)
#define D3D_MODEL_FILE_EXTENSION					"m"
#define MODEL_FILE_MAGIC_NUMBER						0xcafe1515

#define MLI_FILE_VERSION 							(2 + GLOBAL_FILE_VERSION)
#define MODEL_LOD_INDEPENDENT_FILE_EXTENSION		"mli"
#define MLI_FILE_MAGIC_NUMBER						0x1515cafe

#define MODEL_LOW_SUBDIR				"low\\"

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MAX_SPECULAR_LIGHTS_PER_MODEL 2
#define MAX_STATIC_LIGHTS_PER_MODEL 20
#define MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH 8

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum
{
	MESH_TEXTURE_CLAMP  = DXC_9_10(	D3DTADDRESS_CLAMP,		D3D10_TEXTURE_ADDRESS_CLAMP),
	MESH_TEXTURE_WRAP   = DXC_9_10( D3DTADDRESS_WRAP,		D3D10_TEXTURE_ADDRESS_WRAP),
	MESH_TEXTURE_BORDER = DXC_9_10( D3DTADDRESS_BORDER,		D3D10_TEXTURE_ADDRESS_BORDER),
	MESH_TEXTURE_MIRROR = DXC_9_10( D3DTADDRESS_MIRROR,		D3D10_TEXTURE_ADDRESS_MIRROR),
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

namespace DPVS
{
	class Model;
	class Object;
}

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct D3D_MESH_DEFINITION 
{
	DWORD								dwFlags;			// Drawing flags
	DWORD								dwTechniqueFlags;   // Flags to help select what shader technique to use

	D3DC_PRIMITIVE_TOPOLOGY				ePrimitiveType;
	VERTEX_DECL_TYPE					eVertType;

	int									nMaterialId;		// The method that we use to draw this mesh
	int									nMaterialVersion;	// have the material or the diffuse texture changed?
	EFFECT_TECHNIQUE_CACHE*				ptTechniqueCache;	// Cache of recently used effect techniques (to avoid frequent searching)

	int									nBoneToAttachTo;	// index of the bone that this mesh is attached to - if there is one

	D3D_INDEX_BUFFER_DEFINITION			tIndexBufferDef;

	int									nVertexBufferIndex; // which vertex buffer in the model to use
	int									nVertexStart;		// Which vertex to start drawing with - used for vertex animation
	WORD								wTriangleCount;		// Number of triangles to render ( default is all )
	int									nVertexCount;

	DWORD								dwTexturePathOverride;
	int									nWardrobeTextureIndex;
	int									pnTextures  [ NUM_TEXTURE_TYPES ];
	char								pszTextures [ NUM_TEXTURE_TYPES ][ MAX_XML_STRING_LENGTH ];
	char								nDiffuse2AddressMode[ 2 ]; // Specific texture address mode (wrap,clamp...) for diffuse 2
	float								pfUVsPerMeter[ TEXCOORDS_MAX_CHANNELS ];
	BOUNDING_BOX						tRenderBoundingBox;
};

//-------------------------------------------------------------------------------------------------

// Do not re-order these file sections! 
// These enumerated values are written out to disk.
enum MODEL_FILE_SECTION
{
	MODEL_FILE_SECTION_STATIC_LIGHTS,
	MODEL_FILE_SECTION_SPECULAR_LIGHTS,
	MODEL_FILE_SECTION_MESH_DEFINITION,
	MODEL_FILE_SECTION_ATTACHMENT_DEFINITION,
	MODEL_FILE_SECTION_AD_OBJECT_DEFINITION,
	MODEL_FILE_SECTION_VERTEX_BUFFER_DEFINITION,
	MODEL_FILE_SECTION_MODEL_DEFINITION,
	MODEL_FILE_SECTION_ANIMATING_MODEL_DEFINITION,
	MODEL_FILE_SECTION_WARDROBE_MODEL_ENGINE_DATA,
	MODEL_FILE_SECTION_WARDROBE_MODEL_PART,
	MODEL_FILE_SECTION_OCCLUDER_DEFINITION,
	MODEL_FILE_SECTION_MLI_DEFINITION,
	NUM_MODEL_FILE_SECTIONS
};

struct OCCLUDER_DEFINITION
{
	int						nVertexCount;
	VECTOR*					pVertices;

	int						nIndexCount;
	INDEX_BUFFER_TYPE*		pIndices;
};

struct D3D_MODEL_DEFINITION
{
	DEFINITION_HEADER		tHeader;

	DWORD					dwFlags;
	int						pnMeshIds[MAX_MESHES_PER_MODEL];// The Id of each mesh being used
	int						nMeshCount;
	int						nMeshesPerBatchMax;
	BOUNDING_BOX			tBoundingBox;
	BOUNDING_BOX			tRenderBoundingBox;

	DPVS::Model*			pCullTestModel;
	DPVS::Model*			pCullWriteModel;

	int						nPostRestoreFileSize; // after loading the file and creating the VBs and IBs, what size should we realloc to?

	int						nAttachmentCount;
	ATTACHMENT_DEFINITION*	pAttachments;

	int						nAdObjectDefCount;
	AD_OBJECT_DEFINITION*	pAdObjectDefs;
	
	int						nVertexBufferDefCount;
	D3D_VERTEX_BUFFER_DEFINITION* ptVertexBufferDefs;
};

struct MLI_DEFINITION
{
	OCCLUDER_DEFINITION*	pOccluderDef;

	int						nStaticLightCount;
	STATIC_LIGHT_DEFINITION* pStaticLightDefinitions;

	int						nSpecularLightCount;
	STATIC_LIGHT_DEFINITION* pSpecularLightDefinitions;
};

struct MATERIAL_OVERRIDE
{
	int nMaterialId;
	int nAttachmentId;
};


//-------------------------------------------------------------------------------------------------
// CHB 2006.12.08 - With LOD, a D3D_MODEL may use more than one
// model definition. The model definitions will have different
// mesh counts and layouts. However, the D3D_MODEL keeps some
// mesh-related data, which must be kept separate for each model
// definition. All "per mesh" data should go in here.
struct D3D_MODEL_DEFDATA
{
	DWORD				dwFlags;				// per-lod, per-model flags
	VECTOR *			ptMeshRenderOBBWorld;	// for each mesh, world-transformed render oriented bounding box (not optimal storage)
	LightmapWaveData *	ptLightmapData;			// for each mesh, phase and freq data for self-illum scrolling
	TIME				tScrollLastUpdateTime;	// model-global last update time for scrolling texture phases
	VECTOR2	*			pvScrollingUVPhases;	// for each mesh, phase for U,V for each scrolling texture channel	
	float *				pfScrollingUVOffsets;	// for each mesh, a phase offset for scrolling uvs
	int	*				pnMaterialVersion;		// for each mesh, the version token for its material
	DWORD *				pdwMeshTechniqueMasks;	// mesh technique masks to be OR'd with the actual mesh one
	int					nTextureOverridesAllocated;
	int *				pnTextureOverrides;		// CHB 2006.11.28
};

//-------------------------------------------------------------------------------------------------

struct D3D_MODEL
{
	//DWORD						dwFlags;			// Drawing flags
	DWORD						dwFlagBits[ DWORD_FLAG_SIZE(NUM_MODEL_FLAGBITS) ];
	DWORD						dwDrawFlags;

	int							nId;
	D3D_MODEL					*pNext;				// CHB 2006.08.09 - Used by hashing.

	PROXNODE					nProxmapId;
	int							nModelDefinitionId;
	int							nAppearanceDefId;
	int							nRegion;					// render region (used for sublevels)

	MODEL_SORT_TYPE				m_SortType;

	// structure must be 16-byte aligned at this point
	MATRIX						matProj;					// proj matrix for geometry (field of view, etc.)
	MATRIX						matView;					// view matrix for geometry
	MATRIX						matWorld;					// world matrix for geometry
	MATRIX						matWorldInverse;			// inverse world matrix for geometry
	MATRIX						matScaledWorld;				// world matrix for geometry - adjusted by scale
	MATRIX						matScaledWorldInverse;		// inverse world matrix (adjusted by scale) - used to move lights into model space
#ifdef ENGINE_TARGET_DX10
	MATRIX						matWorldViewProjCurr;
	MATRIX						matWorldViewProjPrev;		// Previous matrix used last render call.
	DWORD						dwWorldViewProjPrevToken;	// The visibility token that this was last updated.
#endif

	DPVS::Object*				pCullObject;
	DWORD						dwVisibilityToken;
	int							nCellID;

	D3D_OCCLUSION				tOcclusion;					// tracks occlusion query results - used for visibility determination
	BOUNDING_BOX				tRenderAABBWorld;			// world-transformed render axis-aligned bounding box
	ORIENTED_BOUNDING_BOX		tRenderOBBWorld;			// world-transformed render oriented bounding box (not optimal storage)
	BOOL						bInFrustum[2];				// double-buffered flags to tell whether the room was in the frustum LAST frame
	float						fSelfIlluminationValue;		// amount to self-illuminate
	float						fSelfIlluminationFactor;	// lerp factor to self-illuminate: 1.0f means above value
	MATERIAL_OVERRIDE			pStateMaterialOverride[ NUM_MATERIAL_OVERRIDE_TYPES ][ MAX_STATE_MATERIAL_OVERRIDE_STACK_DEPTH ];		// stacks of materials. render with material on top of stack instead of current mesh materials (temporary, takes precedence)
	int							nStateMaterialOverrideIndex[ NUM_MATERIAL_OVERRIDE_TYPES ]; // stack top indexes for material override stacks
	int							nTextureMaterialOverride;	// material to render with instead of current mesh materials (persistent, base priority)
	int							nEnvironmentDefOverride;	// environment definition override
	float						fAlpha;						// whole-model opacity value
	float						fDistanceAlpha;				// whole-model distance opacity value
	BOOL						bFullbright;
	float							fBehindColor;
	float						AmbientLight;
	float						HitColor;
	float						fRadius;
	DWORD						nRoomID;					// room, if applicable
	SH_COEFS_RGB<2>				tBaseSHCoefs_O2;			// optional base SH lighting coefs (order 2) sRGB
	SH_COEFS_RGB<3>				tBaseSHCoefs_O3;			// optional base SH lighting coefs (order 3) sRGB
	SH_COEFS_RGB<2>				tBaseSHCoefsLin_O2;			// optional base SH lighting coefs (order 2) linear
	SH_COEFS_RGB<3>				tBaseSHCoefsLin_O3;			// optional base SH lighting coefs (order 3) linear
	MODEL_RENDER_LIGHT_DATA		tLights;					// cached rendering light data

	VECTOR						vPosition;	
	VECTOR						vScale;
	DWORD						dwLightGroup;				// only tries to apply lights in the same group(s) -- currently ignored

	MODEL_FAR_CULL_METHOD		eFarCullMethod;				// Which method to use for far-away culling
	MODEL_DISTANCE_CULL_TYPE	eDistanceCullType;			// Which distance cull value to use
	float						fLODScreensize;				// Screensize factor at which LOD changes (derived from fLODDistance).
	float						fLODTransitionLERP;			// How far is it to LOD (where 0 == lower LOD)
	int							nDisplayLOD;				// CHB 2006.11.30 - The LOD level selected for display.
	int							nDisplayLODOverride;		// APE 2007.03.15 - Added an override LOD level for display.
	Features					RequestFeatures;
	Features					ForceFeatures;

	D3D_MODEL_DEFDATA			tModelDefData[LOD_COUNT];	// CHB 2006.12.08

	int *						pnAdObjectInstances;		// for each ad object instance, the ID of the engine data object
	int							nAdObjectInstances;

	// these are the lights affecting the model's specular only
	// they are mostly used for background models
	int							nSpecularLightCount;
	int							pnSpecularLightIds[ MAX_SPECULAR_LIGHTS_PER_EFFECT ]; 
};

struct MESH_DEFINITION_HASH
{
	D3D_MESH_DEFINITION* pDef;
	int nId;
	MESH_DEFINITION_HASH* pNext;
};

struct MODEL_DEFINITION_HASH
{
	int nId;
	MODEL_DEFINITION_HASH* pNext;
	REFCOUNT tRefCount;

	D3D_MODEL_DEFINITION* pDef[ LOD_COUNT ];
	DWORD dwLODFlags[ LOD_COUNT ];
	MLI_DEFINITION*	pSharedDef;
	DWORD dwFlags;
	ENGINE_RESOURCE tEngineRes;
};

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline D3D_MESH_DEFINITION * dx9_MeshDefinitionGetFromHash( MESH_DEFINITION_HASH * pHash )
{
	return pHash ? pHash->pDef : NULL;
}

inline D3D_MESH_DEFINITION * dx9_MeshDefinitionGet( int nMeshID )
{
	if ( nMeshID == INVALID_ID )
		return NULL;
	extern CHash<MESH_DEFINITION_HASH> gtMeshes;
	return dx9_MeshDefinitionGetFromHash( HashGet( gtMeshes, nMeshID ) );
}

inline MESH_DEFINITION_HASH * dx9_MeshDefinitionGetHash( int nMeshID )
{
	if ( nMeshID == INVALID_ID )
		return NULL;
	extern CHash<MESH_DEFINITION_HASH> gtMeshes;
	return HashGet( gtMeshes, nMeshID );
}

inline MESH_DEFINITION_HASH * dx9_MeshDefinitionGetFirst()
{
	extern CHash<MESH_DEFINITION_HASH> gtMeshes;
	return HashGetFirst( gtMeshes );
}

inline MESH_DEFINITION_HASH * dx9_MeshDefinitionGetNext( MESH_DEFINITION_HASH * pHashCur )
{
	extern CHash<MESH_DEFINITION_HASH> gtMeshes;
	return HashGetNext( gtMeshes, pHashCur );
}

inline D3D_MODEL * dx9_ModelGet( int nModelID )
{
	if ( nModelID == INVALID_ID )
		return NULL;
	extern CHash<D3D_MODEL> gtModels;
	return HashGet( gtModels, nModelID );
}

inline D3D_MODEL * dx9_ModelGetFirst()
{
	extern CHash<D3D_MODEL> gtModels;
	return HashGetFirst( gtModels );
}

inline D3D_MODEL * dx9_ModelGetNext( D3D_MODEL * pModelCur )
{
	extern CHash<D3D_MODEL> gtModels;
	return HashGetNext( gtModels, pModelCur );
}

inline D3D_MODEL_DEFINITION * dxC_ModelDefinitionGetFromHash( MODEL_DEFINITION_HASH * pHash, int nLOD )
{
	if ( nLOD == LOD_NONE )
		return NULL;

	ASSERT_RETNULL( ( nLOD >= 0 && nLOD < LOD_COUNT ) || nLOD == LOD_ANY );
	if ( pHash && ( nLOD == LOD_ANY ) )
	{
		for (int i = 0; i < LOD_COUNT; i++ )
		{
			if( pHash->pDef[ i ] )
			{
				nLOD = i;
				break;
			}
		}

		// We didn't find a model definition 
		if ( nLOD == LOD_ANY )
			return NULL;
	}

	return pHash ? pHash->pDef[ nLOD ] : NULL;
}

inline D3D_MODEL_DEFINITION* dxC_ModelDefinitionGet( int nModelDef, int nLOD )
{
	if ( nModelDef == INVALID_ID )
		return NULL;
	extern CHash<MODEL_DEFINITION_HASH> gtModelDefinitions;
	return dxC_ModelDefinitionGetFromHash( HashGet( gtModelDefinitions, nModelDef ), nLOD );
}

inline MODEL_DEFINITION_HASH * dxC_ModelDefinitionGetHash( int nModelDef )
{
	if ( nModelDef == INVALID_ID )
		return NULL;
	extern CHash<MODEL_DEFINITION_HASH> gtModelDefinitions;
	return HashGet( gtModelDefinitions, nModelDef );
}

inline MODEL_DEFINITION_HASH * dx9_ModelDefinitionGetFirst()
{
	extern CHash<MODEL_DEFINITION_HASH> gtModelDefinitions;
	return HashGetFirst( gtModelDefinitions );
}

inline MODEL_DEFINITION_HASH * dx9_ModelDefinitionGetNext( MODEL_DEFINITION_HASH* pHashCur )
{
	extern CHash<MODEL_DEFINITION_HASH> gtModelDefinitions;
	return HashGetNext( gtModelDefinitions, pHashCur );
}

inline MLI_DEFINITION* dxC_SharedModelDefinitionGet( int nModelDef )
{
	if ( nModelDef == INVALID_ID )
		return NULL;
	extern CHash<MODEL_DEFINITION_HASH> gtModelDefinitions;
	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( nModelDef );
	return pHash ? pHash->pSharedDef : NULL;
}


//inline void dxC_ModelSetFlagbit( D3D_MODEL * pModel, MODEL_FLAGBIT eFlagbit, BOOL bSet )
//{
//#if ISVERSION(DEVELOPMENT)
//	ASSERT_RETURN( pModel );
//#endif
//	SETBIT( pModel->dwFlagBits, eFlagbit, bSet );
//}

inline BOOL dxC_ModelGetFlagbit( const D3D_MODEL * pModel, MODEL_FLAGBIT eFlagbit )
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETFALSE( pModel );
#endif
	return TESTBIT_MACRO_ARRAY( pModel->dwFlagBits, eFlagbit );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O > const SH_COEFS_RGB<O> * dxC_ModelGetBaseSHCoefs( const D3D_MODEL * pModel, BOOL bLinearSpace );

template<> inline const SH_COEFS_RGB<2> * dxC_ModelGetBaseSHCoefs< 2 > ( const D3D_MODEL * pModel, BOOL bLinearSpace )
{
	return bLinearSpace ? &pModel->tBaseSHCoefsLin_O2 : &pModel->tBaseSHCoefs_O2;
}

template<> inline const SH_COEFS_RGB<3> * dxC_ModelGetBaseSHCoefs< 3 > ( const D3D_MODEL * pModel, BOOL bLinearSpace )
{
	return bLinearSpace ? &pModel->tBaseSHCoefsLin_O3 : &pModel->tBaseSHCoefs_O3;
}

//----------------------------------------------------------------------------

PRESULT dxC_MeshFree( D3D_MESH_DEFINITION * pMesh );

void dxC_CollapseMeshByUsage ( 
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, 
	WORD * pwTriangleCount,
	int * pnIndexBufferSize,
	INDEX_BUFFER_TYPE * pwIndexBuffer,
	int nMeshTriangleCount, 
	BOOL * pbIsSubsetTriangle, 
	BOOL * pbIsSubsetVertex );

void dxC_MeshGetFolders( 
	D3D_MESH_DEFINITION * pMeshDefinition, 
	int nLOD,
	char * szFolder, 
	struct OVERRIDE_PATH *pOverridePath,
	const char * pszFolderIn, 
	int nStrSize, 
	BOOL bBackground );

enum
{
	MDNFF_FLAG_FORCE_SYNC_LOAD				= (1 << 0),
	MDNFF_FLAG_SOURCE_FILE_OPTIONAL			= (1 << 1),
	MDNFF_FLAG_FORCE_ADD_TO_DEFAULT_PAK		= (1 << 2),
	MDNFF_FLAG_MODEL_SHOULD_LOAD_TEXTURES	= (1 << 3),
	MDNFF_FLAG_DONT_LOAD_NEXT_LOD			= (1 << 4),
};

PRESULT dxC_ModelDefinitionNewFromFileEx( 
	int* pnModelDef,
	int nLOD,
	int nModelDefOverride,
	const char* filename, 
	const char* filefolder,
	float fDistanceToCamera,
	unsigned nFlags );

XFER_FUNC_PROTO( dxC_MeshWriteToBuffer, 
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>& tSerializedFileSections,
	D3D_MODEL_DEFINITION & tModelDef,
	D3D_MESH_DEFINITION & tMeshDef );

PRESULT dxC_MeshMakeBoundingBox(
	D3D_MESH_DEFINITION * pMesh,
	D3D_VERTEX_BUFFER_DEFINITION * pVBufferDef, 
	int nIndexCount,
	INDEX_BUFFER_TYPE * pwIBuffer );

XFER_FUNC_PROTO( dxC_ModelDefinitionWriteVerticesBuffers, 
	SIMPLE_DYNAMIC_ARRAY<SERIALIZED_FILE_SECTION>& tSerializedFileSections,
	D3D_MODEL_DEFINITION & tModelDefinition );

D3D_VERTEX_BUFFER_DEFINITION * dxC_ModelDefinitionAddVertexBufferDef( D3D_MODEL_DEFINITION * pModelDefinition, int * pnIndex );

void dxC_ModelUpdateMeshTechniqueMasks( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef );
BOOL dxC_ModelInFrustum( D3D_MODEL * pModel, const PLANE * pPlanes );

PRESULT dxC_ModelUpdateVisibilityToken( D3D_MODEL * pModel );
BOOL dxC_ModelCurrentVisibilityToken( const D3D_MODEL * pModel );

PRESULT dxC_ModelCalculateDistanceAlpha( float & fDistanceAlpha, const D3D_MODEL * pModel, float fScreensize, float fDistToEyeSq, float fPointDistToEyeSq );
PRESULT dxC_ModelComputeUnbiasedLODScreensize( D3D_MODEL * pModel, float & fScreensize, float fDist, D3D_MODEL_DEFINITION * pModelDef = NULL );
void dxC_ModelSetLODScreensize( D3D_MODEL * pModel, float fLODScreensize );
void dxC_ModelSetDistanceCullType( D3D_MODEL * pModel, MODEL_DISTANCE_CULL_TYPE eType );
void dxC_ModelSetFarCullMethod( D3D_MODEL * pModel, MODEL_FAR_CULL_METHOD eMethod );
PRESULT dxC_ModelSelectLOD( D3D_MODEL * pModel, float fScreensize, float fDistanceFromEyeSquared );
int dxC_ModelGetFirstLOD( const D3D_MODEL * pModel );
int dxC_ModelGetDisplayLOD( D3D_MODEL * pModel );
int dxC_ModelGetDisplayLODWithDefault( D3D_MODEL* pModel );

void dxC_ModelSetDrawBehind	( D3D_MODEL * pModel, BOOL bDraw );
DRAW_LAYER dxC_ModelGetDrawLayer( const D3D_MODEL * pModel );
PRESULT dxC_ModelSetDrawLayer(
	D3D_MODEL * pModel,
	DRAW_LAYER eDrawLayer );
BOOL dxC_ModelGetNoDrawGroup( D3D_MODEL * pModel, DWORD dwDrawGroup );
void dxC_ModelSetNoDrawGroup( D3D_MODEL * pModel, DWORD dwDrawGroup, BOOL bSet );
PRESULT dxC_ModelLazyUpdate( D3D_MODEL * pModel );
PRESULT dxC_GetModelRenderAABBInWorld( const D3D_MODEL * pModel, BOUNDING_BOX * pBBox );
VECTOR * dxC_GetModelRenderOBBInWorld( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef );
PRESULT dxC_GetModelRenderAABBInObject( D3D_MODEL * pModel, BOUNDING_BOX * pBBox, BOOL bAnyLOD = TRUE );
BOOL dxC_ModelProjectedInFrustum( D3D_MODEL * pModel, int nLOD, int nMeshIndex, const PLANE * pPlanes, const VECTOR& vDir, BOOL bIntersectionsAllowed = TRUE );
PRESULT dxC_ModelSetDistanceAlpha( D3D_MODEL * pModel, float fAlpha );
PRESULT dxC_ModelGetWorldSizeAvg( D3D_MODEL * pModel, float & fSize, D3D_MODEL_DEFINITION * pModelDefinition = NULL );
PRESULT dxC_ModelGetWorldSizeAvgWeighted( D3D_MODEL * pModel, float & fSize, const VECTOR & vWeights, D3D_MODEL_DEFINITION * pModelDefinition = NULL );

D3DCOLOR dxC_VectorToColor ( 
	const D3DXVECTOR3 & vInVector );

void dxC_ModelDefinitionAddRef( MODEL_DEFINITION_HASH * pModelDefHash );
void dxC_ModelDefinitionReleaseRef( MODEL_DEFINITION_HASH * pModelDefHash );
void dxC_ModelDefinitionSyncLODRef( MODEL_DEFINITION_HASH * pModelDefHash );
void dxC_ModelDefinitionTexturesAddRef( D3D_MODEL_DEFINITION * pModelDef );
void dxC_ModelDefinitionTexturesReleaseRef( D3D_MODEL_DEFINITION * pModelDef, BOOL bCleanupNow = FALSE );

BOOL dxC_MeshDiffuseHasAlpha(
	D3D_MODEL* pModel,
	const D3D_MODEL_DEFINITION* pModelDef,
	const D3D_MESH_DEFINITION* pMesh,
	int nMesh,
	int nLOD );

XFER_FUNC_PROTO( dxC_SerializedFileSectionXfer,
   SERIALIZED_FILE_SECTION & tSection,
   int nSFHVersion);

XFER_FUNC_PROTO( dxC_SerializedFileHeaderXfer, SERIALIZED_FILE_HEADER & tFileHeader);

XFER_FUNC_PROTO( dxC_ModelDefinitionXfer, 
	D3D_MODEL_DEFINITION & tModelDef,
	int nVersion,
	BOOL bIsAnimatedModel );

XFER_FUNC_PROTO( dxC_SharedModelDefinitionXfer, MLI_DEFINITION & tSharedModelDef );

XFER_FUNC_PROTO( dxC_MeshDefinitionXfer, D3D_MESH_DEFINITION & tMeshDef );

XFER_FUNC_PROTO( dxC_VertexBufferDefinitionXfer, 
	D3D_VERTEX_BUFFER_DEFINITION & tVertexBufferDef,
	int& nStreamCount );

XFER_FUNC_PROTO( dxC_AttachmentDefinitionXfer, ATTACHMENT_DEFINITION& tAttachmentDef );

XFER_FUNC_PROTO( dxC_AdObjectDefinitionXfer, 
	AD_OBJECT_DEFINITION& tAdObjectDef,
	int nVersion,
	BOOL bIsAnimatedModel);

XFER_FUNC_PROTO( dxC_BoundingBoxXfer, BOUNDING_BOX& tBoundingBox );

XFER_FUNC_PROTO( dxC_LightDefinitionXfer, STATIC_LIGHT_DEFINITION& tLightDef );

XFER_FUNC_PROTO( dxC_ModelDefinitionXferFileSections,
	D3D_MODEL_DEFINITION & tModelDef,
	D3D_MESH_DEFINITION* pMeshDefs[],
	SERIALIZED_FILE_SECTION* pSections, 
	int nNumSections,
	int nVersion,
	BOOL bIsAnimatedModel );

XFER_FUNC_PROTO( dxC_OccluderDefinitionXfer, OCCLUDER_DEFINITION & tOccluderDef);


//#ifdef ENGINE_TARGET_DX9
	#include "..\\dx9\\dx9_model.h"
//#endif


#endif //__DXC_MODEL__
