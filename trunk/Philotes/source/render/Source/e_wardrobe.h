//----------------------------------------------------------------------------
// e_wardrobe.h
//
// - Header for wardrobe functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_WARDROBE__
#define __E_WARDROBE__

#include "boundingbox.h"

#ifndef __LOD__
#include "lod.h"
#endif

#include "e_features.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MAX_WARDROBE_TEXTURE_GROUPS 4	//see sgtWardrobeTextureIndex

#define MAX_WARDROBE_MATERIAL_TEXTURE_PAIRS 10
#define MAX_WARDROBE_MODELS 25
#define WARDROBE_COLORMASK_TOTAL_COLORS 9 // COLOR_SET_COLOR_COUNT + NUM_WARDROBE_BODY_COLORS 

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

typedef enum {
	LAYER_TEXTURE_NONE = -1,
	LAYER_TEXTURE_DIFFUSE = 0,
	LAYER_TEXTURE_NORMAL,
	LAYER_TEXTURE_SPECULAR,
	LAYER_TEXTURE_SELFILLUM,
	LAYER_TEXTURE_COLORMASK,
	LAYER_TEXTURE_BLEND, // this should always be last

	NUM_LAYER_TEXTURES,
	NUM_TEXTURES_PER_SET = LAYER_TEXTURE_BLEND,
} LAYER_TEXTURE_TYPE;

enum WARDROBE_RESOURCE_FUNCTION
{
	WARDROBE_RELEASE,
	WARDROBE_RESTORE,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_ITERATE_WARDROBES)( WARDROBE_RESOURCE_FUNCTION eFunction );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct WARDROBE_MODEL_ENGINE_DATA;
struct granny_skeleton;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// the specific instance of a piece of a model
#define MAX_WARDROBE_MODEL_PARTS 100
struct WARDROBE_MODEL_PART {
	int nPartId;
	int nTriangleStart;
	int nTriangleCount;
	int nMaterial;
	int nTextureIndex;
};

// a collection of parts which can be added to a wardrobe
#define WARDROBE_MODEL_FILE_NAME_LENGTH 128
#define NUM_APPEARANCES_PER_MODEL_GROUP 2
struct WARDROBE_MODEL {
	int nLOD;
	int nModelGroup;
	int pnAppearanceGroup[ NUM_APPEARANCES_PER_MODEL_GROUP ];
	int nRow;
	char pszFileName		[ WARDROBE_MODEL_FILE_NAME_LENGTH  ];
	char pszDefaultMaterial [ DEFAULT_FILE_SIZE  ];
	int nMaterial;

	int nPartGroup;

	BOOL bLoadFailed[ LOD_COUNT ];
	BOOL bAsyncLoadRequested;
	SAFE_PTR(struct WARDROBE_MODEL_ENGINE_DATA*, pEngineData);

	struct BOUNDING_BOX tBoundingBox;
};

// defines what a piece of a model is labeled - so that we know how a model is chopped up
#define WARDROBE_LABEL_LENGTH 10
struct WARDROBE_MODEL_PART_DATA {
	char pszName			[ DEFAULT_INDEX_SIZE ];
	char pszLabel			[ WARDROBE_LABEL_LENGTH ];
	int nPartGroup;
	int nMaterialIndex;
	int nTargetTextureIndex;
	char pszMaterialName	[ DEFAULT_INDEX_SIZE ];
};

struct PART_CHART 
{
	int nPart;
	int nModel;
	int nIndexInModel;
	int nMaterial;
	int nTextureIndex;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#define TEXTURE_WITH_DEFAULT_COLOR -2

PRESULT e_WardrobeInit();

PRESULT e_WardrobeTextureNewLayer( 
	int * pnTextureID,
	const char * pszBaseFolder,
	const char * pszTextureSetFolder,
	const char * pszFilename,
	int nMaxLen,
	LAYER_TEXTURE_TYPE eTextureType,
	int nLOD, 
	int nModelDef,
	class TEXTURE_DEFINITION * pDefOverride,
	DWORD dwReloadFlags = 0,
	DWORD dwTextureFlags = 0 );

DWORD e_WardrobeGetLayerFormat(
	LAYER_TEXTURE_TYPE eLayer );

PRESULT e_WardrobeValidateTexture(
	int nTextureID,
	LAYER_TEXTURE_TYPE eLayer );

void e_WardrobeResourceApplyFunction(
	WARDROBE_RESOURCE_FUNCTION eFunction,
	int * pnTextureCard,
	int * pnTextureCanvas,
	int nCount );

PRESULT e_WardrobeTextureAllocateBaseCanvas(
	int & rnCanvasTextureID,
	LAYER_TEXTURE_TYPE eLayerTextureTypeRGB,
	DWORD dwTextureFlags );

PRESULT e_WardrobeTextureMakeBaseCanvas(
	int nCanvas,
	int nLayer,
	LAYER_TEXTURE_TYPE eTextureType,
	BOOL bOnlyFillInNewTexture,
	int nCanvasWidth = -1,
	int nCanvasHeight = -1 );

PRESULT e_WardrobeAddAlphaTextureToCanvas(
	int nCanvas,
	int nLayer,
	int nBlend );

PRESULT e_WardrobeAddTextureToCanvas(
	int nCanvas,
	int nLayer,
	int nBlend,
	LAYER_TEXTURE_TYPE eType );

PRESULT e_WardrobeCanvasComplete( int nCanvas, BOOL bReleaseSysMem = TRUE );

BOOL e_WardrobeTextureCanvasHasAlpha(
	LAYER_TEXTURE_TYPE eTextureType );

int e_WardrobeModelDefinitionNewFromMemory(
	const char * pszFileName,
	int nLOD,
	int nUseExistingModelDef = INVALID_ID );

PRESULT e_WardrobeAddLayerModelMeshes( 
	int nModelDefinitionId,
	int nLOD,
	int ppnTextureCard[MAX_WARDROBE_TEXTURE_GROUPS][ NUM_TEXTURES_PER_SET ],
	int * pnWardrobeTextureIndex,
	int * pnWardrobeMaterials,
	int nWardrobeMaterialCount,
	int * pnWardrobeModelList,
	int nWardrobeModelCount,
	WARDROBE_MODEL * pWardrobeModelBase,
	PART_CHART * pPartChart,
	int nPartCount,
	BOOL bUsesColorMask );

PRESULT e_WardrobeModelResourceFree(
	WARDROBE_MODEL * pModel );

PRESULT e_WardrobeModelFileUpdate(
	WARDROBE_MODEL * pWardrobeModel,
	int nLOD,
	const char * pszAppearancePath,
	const char * pszSkeletonFilename,
	MATRIX * pmInverseSkinTransform,
#ifdef HAVOK_ENABLED
	hkSkeleton * pHavokSkeleton,
#endif
	granny_skeleton * pGrannySkeleton,
	BOOL bForceUpdate );

PRESULT e_WardrobeModelLoad(
	WARDROBE_MODEL * pWardrobeModel,
	int nLOD,
	BOOL bForceSync = FALSE );

DWORD e_WardrobeGetBlendTextureFormat();

WARDROBE_MODEL_PART* e_WardrobeModelGetParts(
	WARDROBE_MODEL* pWardrobeModel,
	int& nPartCount );

PRESULT e_WardrobeSetTexture( 
	int nModelDefinitionId,
	int nLOD,
	int nTexture,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int nWardrobeTextureIndex );

PRESULT e_WardrobeCombineDiffuseAndColorMask( 
	int nDiffuseTexture,
	int nColorMaskTexture,
	int nDiffusePostColorMaskTexture,
	DWORD pdwColors[ WARDROBE_COLORMASK_TOTAL_COLORS ],
	int nQuality,
	BOOL bDontApplyBody,
	BOOL bNoAsync );

int e_WardrobeGetLayerTextureType( 
	LAYER_TEXTURE_TYPE eLayer );

void e_WardrobePreloadTexture(
	int nTexture );

PRESULT e_WardrobeLoadBlendTexture(
	int * pnTextureID,
	const char * pszFilePath,
	DWORD dwReloadFlags,
	DWORD dwTextureFlags );

#endif // __E_WARDROBE__
