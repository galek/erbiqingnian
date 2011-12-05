//----------------------------------------------------------------------------
// dxC_anim_model.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_ANIM_MODEL__
#define __DXC_ANIM_MODEL__

#include "e_anim_model.h"
#include "dxC_model.h"
#include "dxc_FVF.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_BONES_PER_VERTEX 3

// this is dependent on the file version of the model files since it wraps one
// CHB 2006.06.27 - +1 for bones per shader reduction.
#define ANIMATED_MODEL_FILE_VERSION					(7 + MODEL_FILE_VERSION + 1)
#define MIN_REQUIRED_ANIMATED_MODEL_FILE_VERSION	(7 + MIN_REQUIRED_MODEL_FILE_VERSION + 1 )
#define ANIMATED_MODEL_FILE_EXTENSION "am"
#define ANIMATED_MODEL_FILE_MAGIC_NUMBER 0x1b0061e1

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

//-------------------------------------------------------------------------------------------------
// Animating model definition - one per model file
//-------------------------------------------------------------------------------------------------
struct ANIMATING_MODEL_DEFINITION 
{
	DWORD					dwFlags;
	int						nBoneCount;
	DWORD					dwBoneIsSkinned[ DWORD_FLAG_SIZE(MAX_BONES_PER_MODEL) ];

	// CHB 2006.06.26 - Despite the name, this actually maps
	// a mesh's shader-local bone index to its absolute model
	// bone index.
	//
	// This will be stored on disk using at most
	// MAX_BONES_PER_SHADER_ON_DISK elements, although due to
	// post-load merging, could actually use up to
	// MAX_BONES_PER_SHADER_RUN_TIME.
	int						ppnBoneMappingModelToMesh[MAX_MESHES_PER_MODEL][MAX_BONES_PER_SHADER_RUN_TIME];

	int*					pnBoneNameOffsets;
	int						nBoneNameBufferSize;
	char*					pszBoneNameBuffer;
	int						nBoneMatrixCount;
	MATRIX*					pmBoneMatrices;
};

//----------------------------------------------------------------------------

// this is what is an intermediate format for grabbing values from granny
struct GRANNY_ANIMATED_VERTEX
{
	D3DXVECTOR3 vPosition;      // Vertex Position
	D3DXVECTOR3 vNormal;		// Vertex Normal
	FLOAT fTextureCoords0[2];	// The texture coordinates
	BYTE pwIndices[4]; // Bone Indices
	FLOAT pflWeights[3]; // Bone Weights
	D3DXVECTOR3 vTangent;		// Used for Pixel Lighting
	D3DXVECTOR3 vBinormal;		// Used for Pixel Lighting
};

extern granny_data_type_definition g_ptAnimatedVertexDataType[];

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void dx9_AnimatedModelDefinitionSplitMeshByBones ( 
	D3D_MODEL_DEFINITION * pModelDefinition,
	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition,
	D3D_MESH_DEFINITION * pMeshDef );

PRESULT dx9_GetAnimatedVertexBuffer ( 
	granny_mesh * pMesh, 
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef, 
	granny_skeleton * pSkeletonGranny,
	int * pnBoneMappingGrannyToHavok,
	MATRIX * pmTransform,
	BOUNDING_BOX * pBBox = NULL,
	BOOL * pbBoxStarted = NULL );

void dx9_AnimatedModelDefinitionUpdateSkinnedBoneMask ( 
	ANIMATING_MODEL_DEFINITION * pAnimatingModelDefinition,
	const D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef );

ANIMATING_MODEL_DEFINITION * dx9_AnimatedModelDefinitionGet(
	int nModelDef,
	int nLOD );

PRESULT dxC_AnimatedModelDefinitionRemovePrivate( 
	int nDefinition,
	int nLOD );

template <XFER_MODE mode>
PRESULT dxC_AnimatingModelDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	ANIMATING_MODEL_DEFINITION & tAnimModelDef);

template PRESULT dxC_AnimatingModelDefinitionXfer<XFER_SAVE>(BYTE_XFER<XFER_SAVE> &, ANIMATING_MODEL_DEFINITION &);
template PRESULT dxC_AnimatingModelDefinitionXfer<XFER_LOAD>(BYTE_XFER<XFER_LOAD> &, ANIMATING_MODEL_DEFINITION &);

EFFECT_TARGET dxC_AnimatedModelGetMaximumEffectTargetForModel11Animation(void);	// CHB 2007.10.16


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DXC_ANIM_MODEL__