//----------------------------------------------------------------------------
// e_model.h
//
// - Header for model functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_MODEL__
#define __E_MODEL__


#include "plane.h"
#include "e_texture.h"
#include "e_features.h"
#include "e_common.h"

#ifndef __LOD__
#include "lod.h"
#endif

//-------------------------------------------------------------------------------------------------
// There is a hierarchy of structures used in this drawing code.
// The graph looks like model->mesh->material->effect with IDs used instead of pointers.
//-------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MIN_DISTANCE_CULL_SIZE_SCALE			0.35f
#define MAX_DISTANCE_CULL_SIZE_SCALE			1.0f

#define MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE		0.025f
#define MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT		0.04f
#define MODEL_CLUTTER_NEAR_ALPHA_SCREENSIZE		0.2f
#define MODEL_ALPHA_FADE_OVER_DISTANCE			3.5f
#define MODEL_ALPHA_FADE_OVER_SCREENSIZE		0.005f
#define MODEL_NEAR_ALPHA_FADE_OVER_DISTANCE		4.5f

//#define MODEL_BOXMAP

#define BGMU_FLAG_INCLUDE_SUBDIRS				MAKE_MASK(0)
#define BGMU_FLAG_FORCE_UPDATE					MAKE_MASK(1)
#define BGMU_FLAG_FORCE_UPDATE_OBSCURANCE		MAKE_MASK(2)
#define BGMU_FLAG_CHECK_OUT_FILES				MAKE_MASK(3)
#define BGMU_FLAG_CLEANUP						MAKE_MASK(4)

#define MAX_MESHES_PER_MODEL		256

//-------------------------------------------------------------------------------------------------
enum MODEL_FLAGBIT
{
	MODEL_FLAGBIT_NODRAW = 0,							// Do not draw this
	MODEL_FLAGBIT_PROJ,									// Has own view matrix
	MODEL_FLAGBIT_VIEW,									// Has own view matrix
	MODEL_FLAGBIT_WORLD,								// Has own world matrix
	MODEL_FLAGBIT_NOSHADOW,								// does not cast a shadow
	MODEL_FLAGBIT_ANIMATED,								// animates
	MODEL_FLAGBIT_FORCE_SETLOCATION,					// force the next ModelSetLocation to go through
	MODEL_FLAGBIT_FIRST_PERSON_PROJ,					// does it use the first person projection
	MODEL_FLAGBIT_TWO_SIDED,							// does it draw both sides of the triangles
	MODEL_FLAGBIT_MIRRORED,								// is it mirrored along one axis?
	MODEL_FLAGBIT_NOLIGHTS,								// should we not draw attached lights?
	MODEL_FLAGBIT_MUTESOUND,							// should we mute attached sounds?
	MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED,				// was the model definition not loaded when needed?
	MODEL_FLAGBIT_FORCE_ALPHA,							// should always render entirely in alpha pass as if all meshes have alpha
	MODEL_FLAGBIT_CLONE,								// model should render twice, and the clone can use a different material override
	MODEL_FLAGBIT_HAS_BASE_SH,							// model has base SH lighting coefficients
	MODEL_FLAGBIT_NO_LOD_EFFECTS,						// don't use LOD effects on this model
	MODEL_FLAGBIT_NODRAW_TEMPORARY,						// Do not draw this - a more temporary setting. NODRAW TRUE overrides this.
	MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW,					// model should cast an indoor shadow (regardless of whether ANIMATED is true)
	MODEL_FLAGBIT_MLI_DEF_APPLIED,						// was the shared model definition not loaded when needed?
	MODEL_FLAGBIT_ATTACHMENT,							// this model is an attachment
	MODEL_FLAGBIT_DISTANCE_CULLABLE,					// this model can be culled by distance
	MODEL_FLAGBIT_NOSOUND,								// should we not even start playing attached sounds?
	MODEL_FLAGBIT_DRAWBEHIND,							// should we not even start playing attached sounds?
	MODEL_FLAGBIT_ARTICULATED_SHADOW,					// this model should always use a shadow-buffer shadow (where possible)
	MODEL_FLAGBIT_PROP,
	//
	NUM_MODEL_FLAGBITS,
};

// DRAW FLAGS
#define MODEL_DRAWFLAG_NODRAW_GROUP0		MAKE_MASK(0)			// does the mesh draw group 0 draw?
#define MODEL_DRAWFLAG_NODRAW_GROUP1		MAKE_MASK(1)			// does the mesh draw group 1 draw?
#define MODEL_DRAWFLAG_NODRAW_GROUP2		MAKE_MASK(2)			// does the mesh draw group 2 draw?
#define MODEL_DRAWFLAG_NODRAW_GROUP3		MAKE_MASK(3)			// does the mesh draw group 3 draw?
#define MODEL_DRAWFLAG_NODRAW_GROUP4		MAKE_MASK(4)			// does the mesh draw group 4 draw?
#define MODEL_DRAWFLAG_NODRAW_GROUP5		MAKE_MASK(5)			// does the mesh draw group 5 draw?
#define MODEL_MASK_DRAW_LAYER		(MAKE_MASK(6) | MAKE_MASK(7) | MAKE_MASK(8))		// what layer does this model and attachments draw in?
#define MODEL_SHIFT_DRAW_LAYER		6
#define MODEL_BITS_DRAW_LAYER		3
// END DRAW FLAGS

#define MAX_MESH_DRAW_GROUPS	6

//-------------------------------------------------------------------------------------------------
#define MODEL_DEFDATA_FLAG_PENDING_TEXTURE_OVERRIDE		MAKE_MASK(0)

//-------------------------------------------------------------------------------------------------
#define MODEL_DEFINITION_DESTROY_DEFINITION 	MAKE_MASK(0)
#define MODEL_DEFINITION_DESTROY_MESHES			MAKE_MASK(1)
#define MODEL_DEFINITION_DESTROY_VBUFFERS		MAKE_MASK(2)
#define MODEL_DEFINITION_DESTROY_TEXTURES		MAKE_MASK(3)
//#define MODEL_DEFINITION_DESTROY_ALL_HUMANS	MAKE_MASK(32)
#define MODEL_DEFINITION_DESTROY_ALL			((DWORD)-1)

#define MODELDEF_FLAG_FREE_BUFFER_ARRAY_DEPRECATED MAKE_MASK(0)
#define MODELDEF_FLAG_ANIMATED					MAKE_MASK(1)
#define MODELDEF_FLAG_HAS_ALPHA					MAKE_MASK(2) // some mesh in the model definition is drawn in the alpha blend pass
#define MODELDEF_FLAG_HAS_FIRST_PERSON_PROJ		MAKE_MASK(3) // some mesh in the model definition is drawn in first-person proj
#define MODELDEF_FLAG_CHECKED_TEXTURE_SIZES		MAKE_MASK(4) // runtime flag - when the artist has background warnings turned on... this has checked for textures that are too large for how they are used
#define MODELDEF_FLAG_CAST_DYNAMIC_SHADOW		MAKE_MASK(5) // modeldef should cast an indoor shadow (regardless of whether ANIMATED is true)
#define MODELDEF_FLAG_IS_PROP					MAKE_MASK(6) // modeldef was created as a layout object


// Flags for the modeldef hash
#define MODELDEF_HASH_FLAG_LOADING						MAKE_MASK(0)
#define MODELDEF_HASH_FLAG_LOAD_FAILED					MAKE_MASK(1)
#define MODELDEF_HASH_FLAG_IGNORE						MAKE_MASK(2)	// CHB 2007.06.27
#define MODELDEF_HASH_FLAG_IS_PROP						MAKE_MASK(3)


//-------------------------------------------------------------------------------------------------
#define MESH_FLAG_NONE						MAKE_MASK(0)
#define MESH_FLAG_NODRAW					MAKE_MASK(1)			// Do not draw this
#define MESH_FLAG_DEBUG						MAKE_MASK(2)			// The mesh is used for layout information and usually not drawn
#define MESH_FLAG_FREEBUFFERS_DEPRECATED	MAKE_MASK(3)			// the buffers aren't from a big block or file - the mesh should free them
#define MESH_FLAG_FROMFILE_DEPRECATED		MAKE_MASK(4)			// the buffers aren't from a big block or file - the mesh should free them
#define MESH_FLAG_WARNED					MAKE_MASK(5)			// we have already displayed a warning for this texture
#define MESH_FLAG_ALPHABLEND				MAKE_MASK(6)			// for meshes that blend alpha - they get drawn in a different pass
#define MESH_FLAG_SKINNED					MAKE_MASK(7)			// Is the mesh deformed by bones
#define MESH_FLAG_CLIP						MAKE_MASK(8)			// the mesh is a clip plane, and can be drawn separately from _debug
#define MESH_FLAG_COLLISION					MAKE_MASK(9)			// the mesh is a collision volume, and can be drawn separately from _debug
#define MESH_FLAG_SILHOUETTE				MAKE_MASK(10)			// the mesh is a silhouette volume, and can be drawn separately from _debug
#define MESH_FLAG_HULL						MAKE_MASK(11)			// The mesh is a convex hull volume, and can be drawn separately from _debug
#define MESH_FLAG_TWO_SIDED					MAKE_MASK(12)			// the mesh faces are to be drawn 2-sided (no culling)
#define MESH_FLAG_BOUNDINGBOX				MAKE_MASK(13)			// The mesh has a bounding box defined
#define MESH_DRAWGROUP_SHIFT				14				// this must be updated to match MESH_FLAG_DRAWGROUP0
#define MESH_FLAG_DRAWGROUP0				MAKE_MASK(14)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_DRAWGROUP1				MAKE_MASK(15)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_DRAWGROUP2				MAKE_MASK(16)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_DRAWGROUP3				MAKE_MASK(17)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_DRAWGROUP4				MAKE_MASK(18)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_DRAWGROUP5				MAKE_MASK(19)			// some draw groups within a model might be enabled and others disabled
#define MESH_FLAG_CHECKED_TEXTURE_SIZES		MAKE_MASK(20)			// runtime flag - when the artist has background warnings turned on... this has checked for textures that are too large for how they are used
#define MESH_FLAG_MATERIAL_APPLIED			MAKE_MASK(21)			// runtime flag - the material has been applied to the mesh
#define MESH_FLAG_NOCOLLISION				MAKE_MASK(22)			// mesh does not have collision
#define MESH_FLAG_OCCLUDER					MAKE_MASK(23)			// the mesh is an occluder, which will be used by dpvs

#define MESH_MASK_ALL_DEBUG_MESHES		(MESH_FLAG_DEBUG | MESH_FLAG_CLIP | MESH_FLAG_COLLISION)

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum
{
	MODEL_IN_FRUSTUM_CURR=0,
	MODEL_IN_FRUSTUM_PREV,
};

enum MODEL_FAR_CULL_METHOD
{
	MODEL_FAR_CULL_BY_SCREENSIZE = 0,
	MODEL_FAR_CULL_BY_DISTANCE,
	MODEL_CANOPY_CULL_BY_SCREENSIZE,
	//
	NUM_MODEL_FAR_CULL_METHODS
};

enum MODEL_DISTANCE_CULL_TYPE
{
	MODEL_DISTANCE_DONT_CULL = -1,
	MODEL_DISTANCE_CULL_BACKGROUND = 0,
	MODEL_DISTANCE_CULL_ANIMATED_ACTOR,
	MODEL_DISTANCE_CULL_ANIMATED_PROP,
	MODEL_DISTANCE_CULL_CLUTTER,
	// count
	NUM_MODEL_DISTANCE_CULL_TYPES
};

//-------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef float * (*PFN_GET_BONES)( int nID, int nMeshIndex, int nLOD, BOOL bGetPrev );	// CHB 2006.11.30
typedef BOOL (*PFN_RAGDOLL_GET_POS)( int nID, VECTOR & vPosition );
typedef void (*PFN_ADD_STATUS_STRING)( const char * pszStr1, const char * pszStr2, void * pContext, int nBufLen );
typedef BOOL (*PFN_QUERY_BY_FILE)( const char * pszFile );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct BOUNDING_BOX;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct BGMU_DATA
{
	DWORD dwFlags;
	void * pContext;
	PFN_ADD_STATUS_STRING pfn_AddStr;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline float * e_ModelGetBones( int nModelID, int nMeshIndex, int nLOD, BOOL bGetPrev = FALSE )
{
	extern PFN_GET_BONES gpfn_GetBones;
	ASSERT_RETNULL( gpfn_GetBones );
	return gpfn_GetBones( nModelID, nMeshIndex, nLOD, bGetPrev );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ModelGetRagdollPosition( int nModelID, VECTOR & vRagPos );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_BackgroundModelFileInExcel( const char * pszFilePath )
{
	extern PFN_QUERY_BY_FILE gpfn_BGFileInExcel;
	if ( gpfn_BGFileInExcel )
		return gpfn_BGFileInExcel( pszFilePath );
	return FALSE;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//    Model Definition
//----------------------------------------------------------------------------

enum
{
	MDNFF_ALLOW_LOW_LOD		= 0,
	MDNFF_ALLOW_HIGH_LOD	= 0,
	MDNFF_IGNORE_LOW_LOD	= (1 << 0),
	MDNFF_IGNORE_HIGH_LOD	= (1 << 1),

	MDNFF_DEFAULT = MDNFF_ALLOW_LOW_LOD | MDNFF_ALLOW_HIGH_LOD,
};

int e_ModelDefinitionNewFromFile ( 
	const char * pszFileName, 
	const char * pszFileFolder,
	unsigned nFlags = MDNFF_DEFAULT,
	float fDistanceToCamera = 0.0f );

PRESULT e_ModelDefinitionReload( int nModelDef, int nLOD );
int e_ModelDefinitionGetMeshCount ( int nModelDefinitionId, int nLOD );
void e_ModelDefinitionSetBoundingBox( int nModelDefinitionId, int nLOD, const BOUNDING_BOX * pNewBox );
void e_ModelDefinitionSetRenderBoundingBox( int nModelDefinitionId, int nLOD, const BOUNDING_BOX * pNewBox );
const BOUNDING_BOX * e_ModelDefinitionGetBoundingBox( int nModelDefinitionId, int nLOD );
DWORD e_ModelDefinitionGetFlags		( int nModelDefinitionId, int nLOD );
void e_ModelDefinitionSetHashFlags ( int nModelDefinitionId, DWORD dwFlags, BOOL bSet );
PRESULT e_ModelDefinitionRestore		( int nModelDefinitionId, int nLOD );
BOOL e_ModelDefinitionsSwap			( int nFirstId, int nSecondId );
PRESULT e_ModelUpdateByDefinition( int nModelDefinition );
void e_ModelDefinitionSetTextures	( int nModelDefinitionId, int nLOD, TEXTURE_TYPE eTextureType, int nTextureId );
void e_ModelDefinitionDestroy( int nModelDefinition, int nLOD, DWORD dwFlags, BOOL bCleanupNow = FALSE );
int e_ModelDefinitionFind( const char* pszName, int nBufLen );
PRESULT e_ModelDefinitionsCleanup();
PRESULT e_ModelDefinitionUpdateMeshTechniqueMasks( int nModelDef, int nLOD );
PRESULT e_MeshUpdateMaterialVersion( int nModelDef, int nLOD, int nMesh );
PRESULT e_ModelDefinitionUpdateMaterialVersions( int nModelDef, int nLOD );
BOOL e_ModelDefinitionCollide( 
	int nModelDefinitionId, 
	const VECTOR & vTestStart, 
	const VECTOR & vDirection,
	int * pnMeshId = NULL,
	int * pnTriangleId = NULL,
	VECTOR * pvPositionOnMesh = NULL,
	float * pfU_Out = NULL,
	float * pfV_Out = NULL,
	VECTOR * pvNormal = NULL,
	float * pfMaxDistance = NULL,
	BOOL bReturnNegative = TRUE);
PRESULT e_ModelDefinitionGetLODFileName(char * pFileNameOut, unsigned nFileNameOutBufferChars, const char * pFileNameIn);
PRESULT e_ModelDefinitionStripLODFileName( char* pFileNameOut, unsigned nFileNameOutBufferChars, const char* pFileNameIn );
void e_ModelDefinitionAddRef		( int nModelDefinitionId );
void e_ModelDefinitionReleaseRef	( int nModelDefinitionId, BOOL bCleanupNow = FALSE );
BOOL e_ModelDefinitionExists		( int nModelDefinitionId, int nLOD = LOD_NONE );
PRESULT e_ModelDefinitionGetSizeInMemory( int nModelDefinitionId, struct ENGINE_MEMORY * pMem, int nLOD = LOD_NONE, BOOL bSkipTextures = FALSE );
PRESULT e_ModelDefinitionGetAdObjectDefCount(
	int nModelDefinitionId,
	int nLOD,
	int & nCount );
PRESULT e_ModelDefinitionGetAdObjectDef(
	int nModelDefinitionId,
	int nLOD,
	int nIndex,
	struct AD_OBJECT_DEFINITION & tAdObjDef );

//----------------------------------------------------------------------------
//    Model
//----------------------------------------------------------------------------

PRESULT e_ModelNew			( int* pnModelId, int nModelDefinitionId );
PRESULT e_ModelRemovePrivate	( int nModelId );
PRESULT e_ModelSetDefinition	( int nModelId, int nModelDefinitionId, int nLOD );
PRESULT e_ModelSetScale		( int nModelId, float fScale );
PRESULT e_ModelSetScale		( int nModelId, const VECTOR vScale );
VECTOR e_ModelGetScale		( int nModelId );
PRESULT e_ModelSetAppearanceDefinition( int nModelId, int nAppearanceId );
int e_ModelGetAppearanceDefinition ( int nModelId );
int e_ModelGetAppearance	( int nModelId );
MODEL_SORT_TYPE e_ModelGetSortType( int nModelId );
void e_ModelSetRegionPrivate( int nModelId, int nRegion );
int  e_ModelGetRegion		( int nModelId );
void e_ModelSetView			( int nModelId, const MATRIX *pmatView );
void e_ModelSetProj			( int nModelId, const MATRIX *pmatProj );
PRESULT e_ModelSetFlagbit	( int nModelId, MODEL_FLAGBIT eFlagbit, BOOL bSet );    
BOOL e_ModelGetFlagbit		( int nModelId, MODEL_FLAGBIT eFlagbit ); // a model must be set to draw and visible to be rendered
PRESULT e_ModelSetLightGroup   ( int nModelId, DWORD dwLightGroups );
DWORD e_ModelGetLightGroup  ( int nModelId );
PRESULT e_ModelSetNoDrawGroup( int nModelId, DWORD dwDrawGroup, BOOL bSet );
PRESULT e_ModelGetNoDrawGroup( int nModelId, DWORD dwDrawGroup, BOOL & bIsSet );
int  e_ModelGetDefinition	( int nModelId );
void e_ModelSetDisplayLODOverride( int nModel, int nLOD );
int  e_ModelGetDisplayLOD	( int nModelId );
void e_ModelResetTime( int nModelId );
void e_ModelAddTime( int nModelId, float fTime );
void e_ModelSetMaxTime( int nModelId, float fTime );
VECTOR e_ModelGetPosition ( int nModelId );
VECTOR* e_ModelGetPositionPointer ( int nModelId );
float e_ModelGetRadius ( int nModelId );
const MATRIX * e_ModelGetWorld ( int nModelId );
const MATRIX * e_ModelGetWorldInverse ( int nModelId );
const MATRIX * e_ModelGetWorldScaled  ( int nModelId );
const MATRIX * e_ModelGetWorldScaledInverse ( int nModelId );
const MATRIX * e_ModelGetView ( int nModelId );
const MATRIX * e_ModelGetProj ( int nModelId );
BOOL e_ModelAnimates		( int nModelId );
PRESULT e_ModelSetTextureOverride ( int nModelId, int nMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride = FALSE );
PRESULT e_ModelSetTextureOverride( int nModelId, const char * pszTextureName, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride = FALSE );
PRESULT e_ModelSetTextureOverrideLOD ( int nModelId, TEXTURE_TYPE eType, const int pnTexture[], BOOL bNoMaterialOverride = FALSE );	// CHB 2006.11.28
PRESULT e_ModelSetTextureOverrideLODMesh( int nModelId, int nLOD, int nLODMeshIndex, TEXTURE_TYPE eType, int nTexture, BOOL bNoMaterialOverride = FALSE);
BOOL e_ModelExists			( int nModelId );
void e_ModelSetInFrustum	( int nModelId, BOOL bSet );
void e_ModelSwapInFrustum	( int nModelId );
void e_ModelSetFullbright	( D3D_MODEL * pModel, BOOL bFullbright );
void e_ModelSetDrawBehind	( D3D_MODEL * pModel, BOOL bDraw );
void e_ModelSetAmbientLight	( D3D_MODEL * pModel, float AmbientLight, float TimeDelta );
float e_ModelGetAmbientLight	( int nModelId );
void e_ModelSetHit	( D3D_MODEL * pModel, BOOL Hit, float TimeDelta );
float e_ModelGetHit	( int nModelId );
void e_ModelSetBehindColor	( D3D_MODEL * pModel, float nBehindColor );
float e_ModelGetBehindColor	( int nModelId );
void e_ModelSetIllumination ( int nModelID, float fIllum, float fFactor );
void e_ModelUpdateIllumination( int nModelID, float fTimeDelta, BOOL bFullbright, BOOL bDrawBehind, BOOL bHit, float fAmbientLight, float fBehindColor );
PRESULT e_ModelInitScrollingMaterialPhases( int nModelID, BOOL bReset = TRUE );
PRESULT e_ModelInitSelfIlluminationPhases( int nModelID, BOOL bReset = TRUE );
DWORD e_ModelGetLightmapColor ( 
	int nModelId, 
	const VECTOR & vWorldPosition,
	const VECTOR & vNormal );
PRESULT e_ModelsUpdate( );
PRESULT e_ModelSetDrawLayer( 
	int nModelId, 
	DRAW_LAYER eDrawLayer );
DRAW_LAYER e_ModelGetDrawLayer( int nModelId );

PRESULT e_ModelSetLocationPrivate(
	int nModelId,
	const MATRIX* pmatWorld,
	const VECTOR& vPosition,
	MODEL_SORT_TYPE SortType = MODEL_DYNAMIC,
	BOOL bForce = FALSE,
	BOOL bUpdateAttachments = TRUE);
inline void e_ModelSetDrawTemporary( int nModelId, BOOL bSet )
{
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY, !bSet );
}
inline void e_ModelSetDrawAndShadow( int nModelId, BOOL bSet )
{
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, !bSet );
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NOSHADOW, !bSet );
}


BOOL e_ModelIsVisible( int nModelId );
VECTOR * e_GetModelRenderOBBInWorld( int nModelID );
PRESULT e_GetModelRenderOBBCenterInWorld( int nModelID, VECTOR & vCenter );
VECTOR * e_GetMeshRenderOBBInWorld( int nModelID, int nLOD, int nMeshIndex );	// CHB 2006.12.08
int e_ModelGetTextureOverride( int nModelID, int nMesh, TEXTURE_TYPE eType, int nLOD );	// CHB 2006.11.28
int e_ModelGetTexture( int nModelID, int nMesh, TEXTURE_TYPE eType, int nLOD, BOOL bNoOverride = FALSE );	// CHB 2006.11.28
PRESULT e_GetModelRenderAABBInWorld( int nModelID, BOUNDING_BOX * pBBox );
PRESULT e_GetModelRenderAABBInObject( int nModelID, BOUNDING_BOX * pBBox, BOOL bAnyLOD = TRUE );
PRESULT e_GetModelRenderAABBCenterInObject( int nModelID, VECTOR & vCenter );
PRESULT e_GetModelRenderAABBCenterInWorld( int nModelID, VECTOR & vCenter );
BOOL e_IsValidModelID( int nModelID );
BOOL e_ModelGetOccluded( int nModelID );
BOOL e_ModelGetInFrustum( int nModelID );
BOOL e_ModelInFrustum( int nModelID, const PLANE * pPlanes );
BOOL e_MeshInFrustum( int nModelID, int nMesh, const PLANE * pPlanes );
BOOL e_ModelProjectedInFrustum( int nModelID, int nLOD, int nMeshIndex, const PLANE * pPlanes, const VECTOR& vDir, BOOL bIntersectionsAllowed = TRUE );
PRESULT e_ModelsInit();
PRESULT e_ModelsDestroy();
PRESULT e_ModelAddMaterialOverride( int nModelID, int nMaterial );
PRESULT e_ModelRemoveMaterialOverride( int nModelID, int nMaterial );
PRESULT e_ModelRemoveAllMaterialOverrides( int nModelID );
PRESULT e_ModelSetEnvironmentOverride( int nModelID, int nEnvDefId );
PRESULT e_ModelSetAlpha( int nModelID, float fAlpha );
float e_ModelGetAlpha( int nModelID );
PRESULT e_ModelSetDistanceAlpha( int nModelID, float fAlpha );
void e_ModelSetRoomID( int nModelID, int nRoomID );
DWORD e_ModelGetRoomID( int nModelID );
int e_ModelGetCellID( int nModelID );
PRESULT e_ModelActivateClone( int nModelId, int nMaterialOverride );
PRESULT e_ModelDeactivateClone( int nModelId, int nMaterialOverride );
PRESULT e_ModelDeactivateAllClones( int nModelId );
PRESULT e_ModelTexturesPreload( int nModelID );
PRESULT e_ModelSetBaseSHCoefs(
	int nModelID,
	float fIntensity,
	SH_COEFS_RGB<2> & tCoefsO2,
	SH_COEFS_RGB<3> & tCoefsO3,
	SH_COEFS_RGB<2> * pCoefsLinO2,
	SH_COEFS_RGB<3> * pCoefsLinO3 );
PRESULT e_ModelClearBaseSHCoefs( int nModelID );
PRESULT e_ModelUpdateVisibilityToken( int nModelID );
BOOL e_ModelCurrentVisibilityToken( int nModelID );

void e_ModelProxmapInit();
void e_ModelProxmapDestroy();
int e_ModelProxMapGetClosest( const VECTOR & vPos );
int e_ModelProxMapGetCloseFirstFromNode( PROXNODE & nCurrent, PROXNODE nFrom, float fRange );
int e_ModelProxMapGetCloseFirst( PROXNODE & nCurrent, const VECTOR & vPos, float fRange );
int e_ModelProxMapGetCloseNext( PROXNODE & nCurrent );
int e_ModelProxMapInsert( int nModelID, BOOL bStaticModel = FALSE );
void e_ModelProxMapRemoveByProxNode( PROXNODE nProxMapID );

PRESULT e_ModelComputeUnbiasedLODScreensize( int nModelID, float fDist, float & fScreensize );
void e_ModelSetLODScreensize(int nModelId, float fLODScreensize);	// fLODScreensize < 0 means use default
void e_ModelSetDistanceCullType( int nModelId, MODEL_DISTANCE_CULL_TYPE eType );
void e_ModelSetFarCullMethod( int nModelId, MODEL_FAR_CULL_METHOD eMethod );
PRESULT e_ModelDumpList();
PRESULT e_ModelSelectLOD(int nModelId, float fDistaceFromEyeSquared);		// CHB 2006.11.30 - Select appropriate LOD for model
int e_ModelGetFirstLOD( int nModel );

PRESULT e_ModelSetFeatureRequestFlag( int nModelID, FEATURES_FLAG eFlag, BOOL bValue );
BOOL e_ModelGetFeatureRequestFlag( int nModelID, FEATURES_FLAG eFlag );
PRESULT e_ModelSetFeatureForceFlag( int nModelID, FEATURES_FLAG eFlag, BOOL bValue );
BOOL e_ModelGetFeatureForceFlag( int nModelID, FEATURES_FLAG eFlag );
PRESULT e_ModelFeaturesGetEffective( int nModelID, class Features & tFeatures );

PRESULT e_ModelsUpdateAllBackgroundModels( const char * pszPath, BGMU_DATA * pData );
PRESULT e_ModelUpdateBackgroundModel( const char * pszFile, BGMU_DATA * pData );
PRESULT e_ModelUpdateBackgroundModel( int nModelID, BGMU_DATA * pData );

PRESULT e_ModelDistanceCullSetOverride( float fDist );
PRESULT e_ModelDistanceCullClearOverride();

//----------------------------------------------------------------------------
//    Mesh Definition
//----------------------------------------------------------------------------

int e_MeshGetMaterial					( int nModelId, int nMeshId );
BOOL e_MeshIsVisible					( int nModelId, void * pMesh );
PRESULT e_MeshSetRender					( int nMeshId, WORD nVertexStart, WORD wTriangleCount );
const char * e_MeshGetTextureFilename ( int nModelID, int nLOD,  int nMesh, TEXTURE_TYPE eType );
PRESULT e_MeshSetTexture				( int nMeshId, TEXTURE_TYPE eType, int nTextureId );
DWORD e_MeshGetLightmapColor( 
	int nModelDefId, 
	int nMeshId, 
	int nTriangleId, 
	float fU,
	float fV );
void e_MeshesCleanup();
const BOUNDING_BOX * e_MeshGetBoundingBox(
	int nModelDef,
	int nLOD,
	int nMeshIndex );
int e_MeshGetLODPriority	( int nModelDefinitionId, int nMeshIndex );
PRESULT e_MeshesRefreshMaterial();

#endif // __E_MODEL__
