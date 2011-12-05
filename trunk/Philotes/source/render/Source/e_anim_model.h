//----------------------------------------------------------------------------
// e_anim_model.h
//
// - Header for animated model functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_ANIM_MODEL__
#define __E_ANIM_MODEL__


#include "e_granny_rigid.h"
#include "e_model.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define BOUNDING_BOX_BUFFER 0.2f

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef void (*PFN_APPEARANCE_CREATE_SKYBOX_MODEL)( struct SKYBOX_DEFINITION * pSkyboxDef, struct SKYBOX_MODEL * pSkyboxModel, const char * pszFolder );
typedef void (*PFN_APPEARANCE_REMOVE)( int nModelID );
typedef void * (*PFN_APPEARANCE_DEF_GET_HAVOK_SKELETON)( int nAppearanceDefID );
typedef struct MATRIX * (*PFN_APPEARANCE_DEF_GET_INVERSE_SKIN_TRANSFORM)( int nAppearanceDefID );
typedef void (*PFN_ANIMATED_MODEL_DEF_JUST_LOADED)( int nAppearanceDefID, int nModelDefID );
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_AppearanceCreateSkyboxModel(
	struct SKYBOX_DEFINITION * pSkyboxDef, 
	struct SKYBOX_MODEL * pSkyboxModel,
	const char * pszFolder );

PRESULT e_AppearanceRemove(
	int nModelID );

void e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping (
	granny_skeleton * pSkeletonGranny,
	int * pnBoneMapping,
	const char * pszFileName);

void e_AnimatedModelDefinitionCalcGrannyToHavokBoneMapping (
	granny_skeleton * pSkeletonGranny,
	class hkSkeleton * pSkeletonMayhem,
	int * pnBoneMapping,
	const char * pszFileName);

PRESULT e_AnimatedModelDefinitionClear ( 
	int nModelDefId,
	int nLOD );

DWORD * e_AnimatedModelDefinitionGetBoneIsSkinnedFlags( 
	int nModelDef );

const int * e_AnimatedModelDefinitionGetBoneMapping( 
	int nModelDef,
	int nLOD,
	int nMeshId);

int e_AnimatedModelDefinitionGetMeshCount( 
	int nModelDefId, 
	int nLOD );

BOOL e_AnimatedModelDefinitionUpdate ( 
	int nAppearanceDefId,
	const char * pszHavokSkeletonFilename,
	const char * pszGrannyMeshFilename,
	BOOL bForceUpdate );

int e_AnimatedModelDefinitionLoad(
	int nAppearanceDefId,
	const char * pszGrannyMeshFilename,
	BOOL bModelShouldLoadTextures );

PRESULT e_AnimatedModelDefinitionReload(
	int nAnimModelDef,
	int nLOD );

PRESULT e_AnimatedModelDefinitionNewFromWardrobe ( 
	int nModelDefinitionId,
	int nLOD,
	struct WARDROBE_MODEL * pWardrobeModel );

PRESULT e_AnimatedModelDefinitionSystemClose( void );

PRESULT e_AnimatedModelDefinitionSystemInit( void );

int e_AnimatedModelGetMaxBonesPerShader();

int e_AnimatedModelDefinitionGetBoneCount(
	int nModelDef );

const char * e_AnimatedModelDefinitionGetBoneName( 
	int nModelDef,
	int nBoneId );

int e_AnimatedModelDefinitionGetBoneNumber( 
	int nModelDef,
	const char * pszName );

const MATRIX * e_AnimatedModelDefinitionGetBoneRestPoses( 
	int nModelDefId );

void e_AnimatedModelDefinitionsSwap( 
	int nFirst, 
	int nSecond );

BOOL e_AnimatedModelDefinitionExists( 
	int nAnimModelDef, 
	int nLOD = LOD_NONE );

#endif // __E_ANIM_MODEL__
