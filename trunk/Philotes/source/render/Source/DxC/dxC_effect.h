//----------------------------------------------------------------------------
// dxC_effect.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_EFFECT__
#define __DXC_EFFECT__

#include "e_definition.h"
#include "e_effect_priv.h"

#include "dxC_shaderlimits.h"

// CML 2007.04.19 - For VERTEX_SHADER_VERSION and PIXEL_SHADER_VERSION.
#include "../Dx9/dx9_shaderversionconstants.h"

//#pragma warning(disable: 4995)
//#include "dxC_state.h"
//#pragma warning(default: 4995)

//--------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------

#define D3D_BACKGROUND_SHADER_NAME		"Background"

#define MAX_SPECULAR_LIGHTS_PER_EFFECT	2
#define MAX_POINT_LIGHTS_PER_EFFECT		5
#define MAX_SH_LIGHTS_PER_EFFECT		16
#define MAX_LIGHTS_PER_EFFECT			256

#define MAX_DIFFUSE_MAPS_PER_EFFECT		2
#define MAX_NUM_GROUPED_LIGHTS			1
#define MAX_SUPPORTED_SH_ORDER			3

#define NUM_CACHED_EFFECT_TECHNIQUES	3

#define SHADER_VERSION_VERTEX	0xfffe
#define SHADER_VERSION_PIXEL	0xffff

#define EFFECT_FLAG_COMPILED			MAKE_MASK(0)
#define EFFECT_FLAG_CASTSHADOW			MAKE_MASK(1)
#define EFFECT_FLAG_ANIMATED			MAKE_MASK(2)
#define EFFECT_FLAG_RENDERTOZ			MAKE_MASK(3)
#define EFFECT_FLAG_LOADED				MAKE_MASK(4)
#define EFFECT_FLAG_STATICBRANCH		MAKE_MASK(5)
#define EFFECT_FLAG_VALIDATED			MAKE_MASK(6)
#define EFFECT_FLAG_FORCE_LIGHTMAP		MAKE_MASK(7)
#define EFFECT_FLAG_RECEIVESHADOW		MAKE_MASK(8)
#define EFFECT_FLAG_EMISSIVEDIFFUSE		MAKE_MASK(9)
#define EFFECT_FLAG_KEEP_INVALID_TECHS	MAKE_MASK(31)	// CHB 2007.07.13

// the order of these flags is important
#define TECHNIQUE_FLAG_MODEL_NORMALMAP				MAKE_MASK(0)
#define TECHNIQUE_FLAG_MODEL_SPECULAR				MAKE_MASK(1)
#define TECHNIQUE_FLAG_MODEL_SKINNED				MAKE_MASK(2)
#define TECHNIQUE_FLAG_MODEL_LIGHTMAP				MAKE_MASK(3)
#define TECHNIQUE_FLAG_MODEL_DIFFUSEMAP2			MAKE_MASK(4)
#define TECHNIQUE_FLAG_MODEL_SPECULAR_LUT			MAKE_MASK(5)
#define TECHNIQUE_FLAG_MODEL_SHADOW					MAKE_MASK(6)
#define TECHNIQUE_FLAG_MODEL_SPHERICAL_HARMONICS	MAKE_MASK(7)
#define TECHNIQUE_FLAG_MODEL_WORLDSPACELIGHTS		MAKE_MASK(8)
#define TECHNIQUE_FLAG_MODEL_CUBE_ENVMAP			MAKE_MASK(9)
#define TECHNIQUE_FLAG_MODEL_SPHERE_ENVMAP			MAKE_MASK(10)
#define TECHNIQUE_FLAG_MODEL_SELFILLUM				MAKE_MASK(11)
#define TECHNIQUE_FLAG_MODEL_32BYTE_VERTEX			MAKE_MASK(12)
#define TECHNIQUE_FLAG_MODEL_SCROLLUV				MAKE_MASK(13)
#define TECHNIQUE_FLAG_MODEL_SCATTERING				MAKE_MASK(14)
#define TECHNIQUE_FLAG_MODEL_SPOTLIGHT				MAKE_MASK(15)	// CHB 2007.07.16
// technique particle group
#define TECHNIQUE_FLAG_PARTICLE_ADDITIVE			MAKE_MASK(16)
#define TECHNIQUE_FLAG_PARTICLE_GLOW				MAKE_MASK(17)
#define TECHNIQUE_FLAG_PARTICLE_GLOW_CONSTANT		MAKE_MASK(18)
#define TECHNIQUE_FLAG_PARTICLE_SOFT_PARTICLES		MAKE_MASK(19)
#define TECHNIQUE_FLAG_PARTICLE_MULTIPLY			MAKE_MASK(20)

#define EFFECT_BLUR_PASS_X			0
#define EFFECT_BLUR_PASS_Y			1
#define EFFECT_BLUR_PASS_FILTER		2
#define EFFECT_BLUR_PASS_MAX		1	// CHB 2006.06.14 - This is really the max number of techniques

#define EFFECT_OVERLAY_TEXTURE		0
#define EFFECT_OVERLAY_COLORONLY	1
#define EFFECT_OVERLAY_WORLDSPACE	2

#define EFFECT_SET_DIR_DONT_MULTIPLY_INTENSITY		MAKE_MASK(0)

//--------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------

#define IS_PARAM_USED(Effect, Tech, Param) (Param && Effect->pD3DEffect->IsParameterUsed( Param, Tech ))

#define EFFECT_MAKE_BRANCH_MASK( eParam )	(DWORD( 1 << ( eParam - EFFECT_FIRST_STATIC_BRANCH_PARAM ) ))

//--------------------------------------------------------------------------------------
// ENUMS
//--------------------------------------------------------------------------------------

//#ifdef ENGINE_TARGET_DX9
//#elif defined(ENGINE_TARGET_DX10)  //KMNV HACK: Override since all techniques use same shader for now
//	#define TECHNIQUE_GROUP_MODEL 0
//	#define TECHNIQUE_GROUP_PARTICLE 0
//	#define TECHNIQUE_GROUP_BLUR 0
//	#define TECHNIQUE_GROUP_GENERAL 0
//	#define TECHNIQUE_GROUP_MAX 0
//#endif

#define INCLUDE_EFFECTPARAM_ENUM
typedef enum 
{
	EFFECT_PARAM_INVALID = -1,
	#include "dxC_effect_def.h"
	NUM_EFFECT_PARAMS,
	EFFECT_FIRST_STATIC_BRANCH_PARAM = EFFECT_PARAM_BRANCH_SPECULAR,
	EFFECT_LAST_STATIC_BRANCH_PARAM = EFFECT_PARAM_BRANCH_SCROLLLIGHTMAP,
	MAX_MUST_HAVE_EFFECT_PARAM = EFFECT_LAST_STATIC_BRANCH_PARAM,
} EFFECT_PARAM;
#undef INCLUDE_EFFECTPARAM_ENUM


#define INCLUDE_TECHFEAT_INT_ENUM
typedef enum
{
	TECHNIQUE_INT_INVALID = -1,
#if !ISVERSION(SERVER_VERSION)
	#include "dxC_effect_feature_def.h"
#else
	FAKE_TECHNIQUE,
#endif
	NUM_TECHNIQUE_INT_CHARTS
} TECHNIQUE_INT_TYPE;
#undef INCLUDE_TECHFEAT_INT_ENUM

// this enum should include both DX9 and DX10 effects in both targets
enum
{
	NONMATERIAL_EFFECT_ZBUFFER,
	NONMATERIAL_EFFECT_COMBINE,
	NONMATERIAL_EFFECT_BLUR,
	NONMATERIAL_EFFECT_SHADOW,
	NONMATERIAL_EFFECT_DEBUG,
	NONMATERIAL_EFFECT_SIMPLE,
	NONMATERIAL_EFFECT_OVERLAY,
	//NONMATERIAL_EFFECT_LIGHTMAP,
    NONMATERIAL_EFFECT_SILHOUETTE,
	NONMATERIAL_EFFECT_OCCLUDED,
	//NONMATERIAL_EFFECT_SHADOW_MIPGENERATOR,
	//NONMATERIAL_EFFECT_FIXED_FUNC_EMULATOR,
	NONMATERIAL_EFFECT_GAUSSIAN,
	NONMATERIAL_EFFECT_SCREEN_EFFECT,
	NONMATERIAL_EFFECT_HDR,
	NONMATERIAL_EFFECT_UI,
	NONMATERIAL_EFFECT_OBSCURANCE,
	NONMATERIAL_EFFECT_SHADOW_BLOB,
	NONMATERIAL_EFFECT_MOVIE,
	NONMATERIAL_EFFECT_AUTOMAP_GENERATE,
	NONMATERIAL_EFFECT_AUTOMAP_RENDER,
	NONMATERIAL_EFFECT_GAMMA,	// CHB 2007.10.02
	NUM_NONMATERIAL_EFFECTS,
};

enum DEBUG_VERTEX_SHADER
{
	DEBUG_VS_PROFILE,
	// count
	NUM_DEBUG_VERTEX_SHADERS
};

enum DEBUG_PIXEL_SHADER
{
	DEBUG_PS_WIREFRAME,
	// count
	NUM_DEBUG_PIXEL_SHADERS
};

//--------------------------------------------------------------------------------------
// STRUCTS
//--------------------------------------------------------------------------------------

typedef struct 
{
	char pszName[ 64 ];
} EFFECT_PARAM_CHART;

typedef struct 
{
	char	pszName[ 32 ];
	int		nCostBase;
	int		nCostPer;
	BOOL	bMatchMin;			// try to match at least this many
} TECHNIQUE_INT_CHART;

typedef struct 
{
	char	pszName[ 32 ];
	int		nCost;
	DWORD	dwFlag;
	DWORD	dwEffectDefFlagBit;
} TECHNIQUE_FLAG_CHART;

typedef struct
{
	D3DC_EFFECT_PASS_HANDLE		hHandle;
	SPD3DCIALAYOUT				pptIAObjects[ NUM_VERTEX_DECL_TYPES ];
} EFFECT_PASS;


typedef struct _TECHNIQUE_FEATURES
{
	int							nInts[ NUM_TECHNIQUE_INT_CHARTS ];
	DWORD						dwFlags;

	void Reset()				{ ZeroMemory( this, sizeof(_TECHNIQUE_FEATURES) ); }

	bool operator ==( const struct _TECHNIQUE_FEATURES & rhs ) const 
	{
		return ( 0 == memcmp( (const void*)this, (const void*)&rhs, sizeof(_TECHNIQUE_FEATURES) ) );
	}
} TECHNIQUE_FEATURES;

#if ISVERSION(DEVELOPMENT)
typedef struct _TECHNIQUE_USAGE
{
	DWORD	dwSelected;
} TECHNIQUE_USAGE;
#endif // DEVELOPMENT

typedef struct _EFFECT_TECHNIQUE
{
	D3DC_TECHNIQUE_HANDLE		hHandle;
	DWORD						dwParamFlags[ DWORD_FLAG_SIZE(NUM_EFFECT_PARAMS) ];

	TECHNIQUE_FEATURES			tFeatures;
	int							nCost;

#if ISVERSION(DEVELOPMENT)
	TECHNIQUE_USAGE				tStats;
#endif // DEVELOPMENT

#ifdef ENGINE_TARGET_DX10
	int							nPasses;
	EFFECT_PASS*				ptPasses;
#endif
} EFFECT_TECHNIQUE;


struct EFFECT_TECHNIQUE_REF
{
	int							nEffectID;
	TECHNIQUE_FEATURES			tFeatures;
	int							nIndex;

	void Reset()
	{
		nEffectID = INVALID_ID;
		tFeatures.Reset();
		nIndex = INVALID_ID;
	}
};



typedef struct _EFFECT_TECHNIQUE_CACHE
{
	EFFECT_TECHNIQUE_REF		tRefs[ NUM_CACHED_EFFECT_TECHNIQUES ];
	int							nSentinel;

	void Reset()
	{
		for (int i=0; i<NUM_CACHED_EFFECT_TECHNIQUES; i++)
			tRefs[i].Reset();
	}
} EFFECT_TECHNIQUE_CACHE;


struct D3D_EFFECT
{
	// CHash members
	int							nId;
	D3D_EFFECT*					pNext;

	char						pszFXFileName[ MAX_XML_STRING_LENGTH ];
	int							nEffectDefId;

	float						fRangeToLOD;
	int							nLODEffect;

	DWORD						dwFlags;
	SPD3DCEFFECT				pD3DEffect;
	EFFECT_TECHNIQUE*			ptTechniques;
	int							nTechniques;
	int*						pnTechniquesIndex;			// sorted by cost
	int							nTechniqueGroup;
	EFFECT_TECHNIQUE*			ptCurrentTechnique;

#ifdef ENGINE_TARGET_DX10		//Don't really need this in Dx9
	EFFECT_PASS*				ptCurrentPass;
#endif

	// these handles are used to communicate with the effect - they are NULL if the effect doesn't need them
	D3DC_EFFECT_VARIABLE_HANDLE	phParams[ NUM_EFFECT_PARAMS ];
};

struct EFFECT_FILE_CALLBACKDATA
{
	int							nEffectID;
	int							nEffectDef;
};

struct TECHNIQUE_STATS
{
	UINT						nSearch;
	UINT						nSearchFailed;
	UINT						nCacheHits;
	UINT						nGets;
};

// structures for better effect constant packing
struct FXConst_MiscLighting
{
	float						vUnused_x;
	float						fShadowIntensity;
	float						fAlphaScale;
	float						fAlphaForLighting;	// CHB 2007.09.11 - PS 1.1 compiler generates better code when this is w (!)
};

struct FXConst_MiscMaterial
{
	float						SpecularMaskOverride;
	float						SelfIlluminationGlow;
	float						SelfIlluminationPower;
	float						SubsurfaceScatterSharpness;
};

struct FXConst_SpecularMaterial
{
	VECTOR2						SpecularGlossiness;
	float						SpecularBrightness;
	float						SpecularGlow;
};

struct FXConst_EnvironmentMap
{
	float						fEnvMapPower;
	float						fEnvMapGlossThreshold;
	float						fEnvMapInvertGlossThreshold;
	float						fEnvMapLODBias;
};

struct FXConst_ScatterMaterial
{
	VECTOR3						vScatterColor;
	float						fScatterIntensity;
};

//--------------------------------------------------------------------------------------
// EXTERNS
//--------------------------------------------------------------------------------------

extern const EFFECT_PARAM_CHART gptEffectParamChart[];
extern DWORD gdwEffectBeginFlags;

//--------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------

struct D3D_MODEL;
struct D3D_MESH_DEFINITION;
struct DRAWLIST_STATE;
struct MODEL_RENDER_LIGHT_DATA;

//#if defined(ENGINE_TARGET_DX9)
//--------------------------------------------------------------------------------------
// HEADERS
//--------------------------------------------------------------------------------------

PRESULT dxC_GetFullShaderPath( TCHAR * fullname, const TCHAR * filename, EFFECT_FOLDER eFolder, EFFECT_SUBFOLDER eSubFolder );

PRESULT dx9_LoadShaderType( int nShaderLineID, int nShaderType );
PRESULT dx9_LoadShaderType( const char * pszShaderName, int nShaderType );
PRESULT dx9_NotifyEffectsDeviceLost();
PRESULT dx9_NotifyEffectsDeviceReset();
PRESULT dx9_EffectTechniqueSetParamFlags( const D3D_EFFECT * pEffect, EFFECT_TECHNIQUE & tTechnique );
PRESULT dx9_EffectLoad( int nEffect, EFFECT_FOLDER eFolder, int nEffectDefID, BOOL bForceSync = FALSE, BOOL bForceDirect = FALSE );
D3D_EFFECT * dxC_EffectFindNext( const char * pszFileName, D3D_EFFECT * pLastEffect = NULL );
PRESULT dx9_EffectRemove( int nEffectID );
PRESULT dx9_EffectsInit();
PRESULT dx9_EffectsDestroy();
PRESULT dx9_EffectsCleanup( BOOL bSkipGeneral = FALSE );
PRESULT dx9_EffectsLoadAll( BOOL bForceSync = FALSE, BOOL bForceDirect = FALSE );
PRESULT dx9_EffectsCloseActive();
PRESULT dx9_ResetEffectCache();
PRESULT dx9_EffectCreateNonMaterialEffects();
PRESULT dx9_EffectCreateDebugShaders();
PRESULT dx9_EffectSetDebugVertexShader( DEBUG_VERTEX_SHADER eShader );
PRESULT dx9_EffectSetDebugPixelShader( DEBUG_PIXEL_SHADER eShader );
BOOL dxC_EffectIsLoaded( const D3D_EFFECT * pEffect );

PRESULT dxC_EffectGetPassDesc( const D3D_EFFECT * pEffect, D3DC_EFFECT_PASS_HANDLE hPass, D3DC_PASS_DESC* pDesc);
PRESULT dxC_EffectGetTechniqueDesc( const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, D3DC_TECHNIQUE_DESC* pDesc);
int dx9_EffectGetAnnotation ( const D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, const char * pszName ); //Get annotation by name

D3D_EFFECT * dxC_EffectGet( int nEffectId );
int dx9_EffectGetNonMaterialID( int nType );
EFFECT_TARGET dxC_GetCurrentMaxEffectTarget();
void dxC_GetCurrentMaxShaderVersions( VERTEX_SHADER_VERSION & eVS, PIXEL_SHADER_VERSION & ePS );
EFFECT_TARGET dxC_GetEffectTargetFromShaderVersions( VERTEX_SHADER_VERSION tVS, PIXEL_SHADER_VERSION tPS );
void dxC_GetShaderVersionsFromEffectTarget( EFFECT_TARGET eTarget, VERTEX_SHADER_VERSION & tVS, PIXEL_SHADER_VERSION & tPS );

int dx9_GetEffectFromIndex( int nEffectIndexLine, EFFECT_TARGET eEffectTarget );
int dx9_GetBestEffectFromIndex( int nEffectIndexLine );
PRESULT dx9_EffectNew( int* pnEffectID, int nEffectFileLine, int nTechniqueGroup = INVALID_ID, BOOL bForceSync = FALSE );
PRESULT dx9_EffectNew ( int* pnEffectID, const char * pszFileName, EFFECT_FOLDER eFolder, int nEffectLine = INVALID_ID, int nTechniqueGroup = INVALID_ID, BOOL bForceSync = FALSE );
PRESULT dxC_EffectBeginPass( D3D_EFFECT * pEffect, int nPass, EFFECT_DEFINITION* pEffectDef = NULL );
PRESULT dxC_SetTechnique( D3D_EFFECT * pEffect, EFFECT_TECHNIQUE * ptTechnique, UINT * pnNumPasses );

PRESULT dx9_GetEffectCacheStatsString( WCHAR * szStats, int nBufLen );
PRESULT dx9_GetTechniqueStatsString( WCHAR * szStats, int nBufLen );
PRESULT dx9_GetTechniqueFeaturesString( WCHAR * szStr, int nBufLen, const TECHNIQUE_FEATURES & tFeats );
PRESULT dx9_GetTechniqueRefFeaturesString( WCHAR * szStr, int nBufLen, const EFFECT_TECHNIQUE_REF * ptRef );

PRESULT dx9_EffectSetScrollingTextureParams(
	struct D3D_MODEL * pModel,
	int nLOD,
	int nMesh,
	int nMeshCount,
	MATERIAL * pMaterial,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique );
PRESULT dxC_EffectGetScaledMeshLights(
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	struct MODEL_RENDER_LIGHT_DATA & tLights,
	struct MESH_RENDER_LIGHT_DATA & tMeshLights );
PRESULT dx9_EffectSetSHLightingParams(
	const struct D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const struct MODEL_RENDER_LIGHT_DATA * ptLights,
	const struct MESH_RENDER_LIGHT_DATA * tMeshLights,
	BOOL bForceCurrentEnv = FALSE );
PRESULT dx9_EffectSetSpotlightParams(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const EFFECT_DEFINITION * pEffectDef,
	const MATRIX * pmLightDirectionMultiplier,
	const D3D_MODEL * pModel,
	const D3D_LIGHT * pLight );
PRESULT dx9_EffectSetHemiLightingParams(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef );
PRESULT dx9_EffectSetDirectionalLightingParams(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const MATRIX * pmLightDirectionMultiplier,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	DWORD dwFlags = 0,
	float fTransitionStr = 1.f,
	BOOL bForceCurrentEnv = FALSE );
PRESULT dx9_EffectSetFogParameters( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	float fScale = 1.f );
PRESULT dxC_EffectSetScreenSize( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	RENDER_TARGET_INDEX nOverrideRT = RENDER_TARGET_INVALID );
PRESULT dxC_EffectSetViewport( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	RENDER_TARGET_INDEX nOverrideRT = RENDER_TARGET_INVALID );
PRESULT dxC_EffectSetShadowSize( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	int nWidth, 
	int nHeight );
PRESULT dxC_EffectSetPixelAccurateOffset( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique );
PRESULT dx9_EffectSetMatrix ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXMATRIXA16 * pMatrix );
PRESULT dx9_EffectSetMatrix ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const struct MATRIX * pMatrix );
PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pVector );
PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const VECTOR4 * pVector );
PRESULT dx9_EffectSetVector ( 
	const D3D_EFFECT * pEffect,
	D3DC_EFFECT_VARIABLE_HANDLE eParam, 
	const D3DXVECTOR4 * pVector );
PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pColor,
	BOOL bNoSRGBConvert = FALSE );
PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	DWORD dwColor,
	BOOL bNoSRGBConvert = FALSE );
PRESULT dx9_EffectSetColor ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	const char * pszParam, 
	DWORD dwColor,
	BOOL bNoSRGBConvert = FALSE );
PRESULT dx9_EffectSetVectorArray ( 
	const D3D_EFFECT * pEffect, 
	D3DC_EFFECT_VARIABLE_HANDLE hParam, 
	const D3DXVECTOR4 * pVector,
	int nCount );
PRESULT dx9_EffectSetVectorArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const D3DXVECTOR4 * pVector,
	int nCount);
PRESULT dx9_EffectSetTexture ( 
   const D3D_EFFECT * pEffect,
   D3DC_EFFECT_VARIABLE_HANDLE hTexture,
   const LPD3DCBASESHADERRESOURCEVIEW pTexture );
PRESULT dx9_EffectSetTexture ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const LPD3DCBASESHADERRESOURCEVIEW pTexture );
//PRESULT dx9_EffectSetTexture ( 
//	const D3D_EFFECT * pEffect, 
//	const EFFECT_TECHNIQUE & tTechnique, 
//	EFFECT_PARAM eParam, 
//	const SPD3DCBASESHADERRESOURCEVIEW pTexture );
PRESULT dx9_EffectSetBool ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	BOOL bValue );
PRESULT dx9_EffectSetInt ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	int nValue );
PRESULT dx9_EffectSetIntArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const int * pnValue,
	int nCount );
PRESULT dx9_EffectSetFloat ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	float fValue );
PRESULT dx9_EffectSetFloatArray ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nCount );
PRESULT dx9_EffectSetRawValue ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nOffset,
	int nSize);
PRESULT dx9_EffectSetValue ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const float * pfValue,
	int nSize);
PRESULT dx9_SetStaticBranchParamsFlags(
	DWORD & dwBranches,
	const MATERIAL * pMaterial, 
	struct D3D_MODEL * pModel,
	const struct D3D_MODEL_DEFINITION * pModelDef,
	const struct D3D_MESH_DEFINITION * pMesh, 
	int nMesh,
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique,
	int nShaderType,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	int nLOD );	// CHB 2006.11.28
PRESULT dx9_SetStaticBranchParamsFromFlags(
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique,
	DWORD dwBranches );
PRESULT dx9_GetEffectAndTechnique( 
	const MATERIAL * pMaterial, 
	struct D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,		// CHB 2007.09.25
	const struct D3D_MESH_DEFINITION * pMesh, 
	int nMesh,
	DWORD dwMeshFlagsDoDraw,
	int nPointLights, 
	int nSpecularLights, 
	float fDistanceToEyeSquared,
	EFFECT_DEFINITION * pEffectDef,
	D3D_EFFECT ** ppEffect, 
	EFFECT_TECHNIQUE ** pptTechnique,
	float & fTransitionStr,
	int nShaderType,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	DWORD * pdwFinalFlags,
	int nLOD );	// CHB 2006.11.28
int dx9_GetCurrentShaderType(
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const DRAWLIST_STATE * pState = NULL,
	const struct D3D_MODEL * pModel = NULL );

PRESULT dxC_EffectGetTechniqueByFeatures(
	D3D_EFFECT * pEffect,
	const TECHNIQUE_FEATURES & tFeatures,
	int & nIndex );

PRESULT dxC_EffectGetTechniqueByFeatures(
	D3D_EFFECT * pEffect,
	const TECHNIQUE_FEATURES & tFeatures,
	EFFECT_TECHNIQUE ** ppOutTechnique );

PRESULT dxC_GetEffectAndTechniqueByRef(
	const EFFECT_TECHNIQUE_REF * pRef,
	D3D_EFFECT ** ppOutEffect,
	EFFECT_TECHNIQUE ** ppOutTechnique );

PRESULT dxC_EffectGetTechnique(
	const int nEffectID,
	const TECHNIQUE_FEATURES & tFeatures,
	EFFECT_TECHNIQUE ** ppOutTechnique,
	EFFECT_TECHNIQUE_CACHE * ptCache = NULL );

PRESULT dxC_EffectGetTechniqueByIndex(	// CHB 2007.10.02
	D3D_EFFECT * pEffect,
	int nIndex,
	EFFECT_TECHNIQUE * & pTechniqueOut );

PRESULT dxC_ToolLoadShaderNow(
	D3D_EFFECT *& pEffect,
	const MATERIAL * pMaterial,
	int nShaderType );

PRESULT dxC_CreateEffectFromFile(
  LPCTSTR pSrcFile,
  LPD3DCEFFECT * ppEffect,
  LPD3DCBLOB * ppCompilationErrors = NULL);

D3DC_EFFECT_PASS_HANDLE dxC_EffectGetPassByIndex( LPD3DCEFFECT pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, UINT iIndex);

//--------------------------------------------------------------------------------------
// INLINE FUNCTIONS
//--------------------------------------------------------------------------------------

inline D3D_EFFECT* dxC_EffectGet( int nEffectId )
{
	extern CHash<D3D_EFFECT> gtEffects;
	return HashGet( gtEffects, nEffectId );
}

inline D3D_EFFECT* dxC_EffectGetFirst()
{
	extern CHash<D3D_EFFECT> gtEffects;
	return HashGetFirst( gtEffects );
}

inline D3D_EFFECT* dxC_EffectGetNext( D3D_EFFECT* pCurrentEffect )
{
	extern CHash<D3D_EFFECT> gtEffects;
	return HashGetNext( gtEffects, pCurrentEffect );
}

inline SPD3DCEFFECTPOOL dxC_GetEffectPool()
{
	extern SPD3DCEFFECTPOOL gpEffectPool;
	return gpEffectPool;
}

#ifdef ENGINE_TARGET_DX10
inline SPD3DCEFFECT dx10_GetGlobalEffect()
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETNULL( dxC_GetEffectPool() );
#endif
	return dxC_GetEffectPool()->AsEffect();
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL dx9_NeedShaderCache()
{
	extern BOOL gbNeedShaderCache;
	return gbNeedShaderCache;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.08.15 - Added macro version. As an inline function, the compiler
// does not optimize ideally when a constant bit index is used.
#if 1
#define dx9_EffectIsParamUsed(pEffect, tTechnique, eParam) ( EFFECT_INTERFACE_ISVALID( (pEffect)->phParams[(eParam)] ) && TESTBIT_MACRO_ARRAY((tTechnique).dwParamFlags, (eParam)))
#else
inline BOOL dx9_EffectIsParamUsed( 
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam )
{
	if ( pEffect->phParams[ eParam ] == NULL )
		return FALSE;
	return TESTBIT_MACRO_ARRAY( tTechnique.dwParamFlags, eParam );
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline int dx9_EffectGetNonMaterialID( int nType )
{
	extern int gpnNonMaterialEffects[];
	return gpnNonMaterialEffects[ nType ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline D3DC_EFFECT_VARIABLE_HANDLE dx9_GetEffectParam ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam )
{
	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, eParam ) )
	{
		D3DC_EFFECT_VARIABLE_HANDLE hParam = pEffect->phParams[ eParam ];
#if 0
		if( eParam == EFFECT_PARAM_POINTLIGHTPOSITION )
		{
			D3D10_EFFECT_VARIABLE_DESC varDesc;
			D3D10_EFFECT_VARIABLE_DESC CBDesc;
			hParam->GetDesc( &varDesc );
			hParam->GetParentConstantBuffer()->GetDesc( &CBDesc );
			LogMessage( "Parameter name is %s it is in %s", varDesc.Name, CBDesc.Name );
		}
#endif
		return hParam;
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline D3DC_EFFECT_VARIABLE_HANDLE dxC_EffectGetParameterByName ( 
	const D3D_EFFECT * pEffect, 
	const char * pszName )
{
	ASSERT( pszName );
	if( !(pEffect->pD3DEffect) )
		return NULL;

	DX9_BLOCK( return pEffect->pD3DEffect->GetParameterByName( NULL, pszName ); )
	
#ifdef ENGINE_TARGET_DX10
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx10_GetGlobalEffect()->GetVariableByName( pszName );
	if( EFFECT_INTERFACE_ISVALID( hParam ) )
		return hParam;

	return pEffect->pD3DEffect->GetVariableByName( pszName );
#endif	

	//DX10_BLOCK( return pEffect->pD3DEffect->GetVariableByName( pszName ); )
	//DX10_BLOCK( return dx10_GetGlobalEffect()->GetVariableByName( pszName ); )  //KMNV TODO HACK: We're assuming all variables you'd want to set are in the pool.  This may not be true.  No it's not, I'm an idiot.
}
//-------------------------------------------------------------------------------------------------
// Particle technique group

//inline EFFECT_TECHNIQUE * dx9_EffectGetTechniquePtrParticle( 
//	const D3D_EFFECT * pEffect,
//	DWORD dwTechniqueMask )
//{
//	if ( ! pEffect->ptTechniques )
//		return NULL;
//
//	ASSERT ( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_PARTICLE );
//	ASSERT( (int) dwTechniqueMask < pEffect->nTechniques );
//	EFFECT_TECHNIQUE * ptTechnique = &pEffect->ptTechniques[ dwTechniqueMask ];
//	ASSERT ( ptTechnique );
//	return ptTechnique;
//}
//
////-------------------------------------------------------------------------------------------------
//// Blur technique group
//
//inline EFFECT_TECHNIQUE * dx9_EffectGetTechniquePtrBlur( 
//	const D3D_EFFECT * pEffect )
//{
//	// CHB 2006.06.13 - Blur effect now uses a single technique with multiple passes.
//	if ( ! pEffect->ptTechniques )
//		return NULL;
//	ASSERT_RETNULL(pEffect->nTechniqueGroup == TECHNIQUE_GROUP_BLUR);
//	ASSERT_RETNULL(pEffect->nTechniques == 1);
//	EFFECT_TECHNIQUE * ptTechnique = &pEffect->ptTechniques[ 0 ];
//	ASSERT ( ptTechnique );
//	return ptTechnique;
//}

//inline EFFECT_TECHNIQUE dx9_EffectGetTechniqueBlur( 
//	const D3D_EFFECT * pEffect )
//{
//	// CHB 2006.06.13 - Blur effect now uses a single technique with multiple passes.
//	return * dx9_EffectGetTechniquePtrBlur( pEffect );
//}

//-------------------------------------------------------------------------------------------------
// Model technique group

//inline void dx9_EffectGetFromModelTechniqueIndex(
//	const D3D_EFFECT * pEffect,
//	int nTechniqueIndex,
//	int & nPointLights,
//	DWORD & dwTechniqueMask )
//{
//	ASSERT_RETURN( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_MODEL );
//	ASSERT_RETURN( nTechniqueIndex < pEffect->nTechniques );
//
//	if ( pEffect->dwFlags & EFFECT_FLAG_STATICBRANCH )
//	{
//		nPointLights = nTechniqueIndex / TECHNIQUE_FLAG_MODEL_STATIC_MAX;
//		//DWORD dwStaticFlags = nTechinqueIndex / TECHNIQUE_FLAG_MODEL_STATIC_MAX;
//	}
//	else
//	{
//		nPointLights = nTechniqueIndex / TECHNIQUE_FLAG_MODEL_MATRIX_MAX;
//		dwTechniqueMask = ( nTechniqueIndex % TECHNIQUE_FLAG_MODEL_MATRIX_MAX ) & TECHNIQUE_FLAG_MODEL_MATRIX_MASK;
//	}
//}
//
//
//inline int dx9_EffectGetTechniqueIndexModel(
//	const D3D_EFFECT * pEffect,
//	int nPointLights,
//	DWORD dwTechniqueMask )
//{
//	ASSERT_RETZERO( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_MODEL );
//
//	nPointLights = min(nPointLights, MAX_POINT_LIGHTS_PER_EFFECT);
//
//	int nTechnique;
//	if ( pEffect->dwFlags & EFFECT_FLAG_STATICBRANCH )
//	{
//		DWORD dwStaticFlags = (dwTechniqueMask & TECHNIQUE_FLAG_MODEL_STATIC_MASK) >> TECHNIQUE_FLAG_MODEL_STATIC_SHIFT;
//		nTechnique = (nPointLights * TECHNIQUE_FLAG_MODEL_STATIC_MAX) + dwStaticFlags;
//	}
//	else
//	{
//		nTechnique = nPointLights * TECHNIQUE_FLAG_MODEL_MATRIX_MAX + (dwTechniqueMask & TECHNIQUE_FLAG_MODEL_MATRIX_MASK);
//	}
//	ASSERT_RETZERO(nTechnique < pEffect->nTechniques);
//	return nTechnique;
//}
//
//inline EFFECT_TECHNIQUE * dx9_EffectGetTechniquePtrModel( 
//	const D3D_EFFECT * pEffect,
//	int nPointLights,
//	DWORD dwTechniqueMask )
//{
//	if (pEffect->ptTechniques == 0) {
//		return NULL;
//	}
//	int nTechnique = dx9_EffectGetTechniqueIndexModel(pEffect, nPointLights, dwTechniqueMask);
//	EFFECT_TECHNIQUE * pTechnique = &pEffect->ptTechniques[nTechnique];
//	ASSERT( pTechnique );
//	return pTechnique;
//}
//
////-------------------------------------------------------------------------------------------------
//// General technique group
//
//inline EFFECT_TECHNIQUE * dx9_EffectGetTechniquePtrGeneral( 
//	const D3D_EFFECT * pEffect,
//	int nIndex )
//{
//	ASSERT_RETNULL( nIndex >= 0 && nIndex < TECHNIQUE_FLAG_GENERAL_MAX );
//	ASSERT_RETNULL( nIndex < pEffect->nTechniques );
//	ASSERT_RETNULL( (pEffect->nTechniqueGroup == TECHNIQUE_GROUP_GENERAL) || (pEffect->nTechniqueGroup == TECHNIQUE_GROUP_SHADOW) );
//	EFFECT_TECHNIQUE * pTechnique = &pEffect->ptTechniques[ nIndex ];
//	ASSERT ( pTechnique );
//	return pTechnique;
//}
//
////-------------------------------------------------------------------------------------------------
//// List technique group
//
//inline EFFECT_TECHNIQUE * dx9_EffectGetTechniquePtrList( 
//	const D3D_EFFECT * pEffect,
//	int nIndex )
//{
//	ASSERT_RETNULL( nIndex >= 0 && nIndex < TECHNIQUE_LIST_COUNT_MAX );
//	ASSERT_RETNULL( nIndex < pEffect->nTechniques );
//	ASSERT_RETNULL( pEffect->nTechniqueGroup == TECHNIQUE_GROUP_LIST );
//	EFFECT_TECHNIQUE * pTechnique = &pEffect->ptTechniques[ nIndex ];
//	ASSERT( pTechnique );
//	return pTechnique;
//}

//inline EFFECT_TECHNIQUE dx9_EffectGetTechniqueGeneral( 
//	const D3D_EFFECT * pEffect,
//	int nIndex )
//{
//	return * dx9_EffectGetTechniquePtrGeneral( pEffect, nIndex );
//}
//#endif

inline D3DC_EFFECT_VARIABLE_HANDLE dxC_EffectGetPassAnnotationByIndex( D3D_EFFECT * pEffect, D3DC_EFFECT_PASS_HANDLE hPass, UINT nIndex )
{
#ifdef ENGINE_TARGET_DX9
	return pEffect->pD3DEffect->GetAnnotation( hPass, nIndex );
#else if defined( ENGINE_TARGET_DX10 )
	return hPass->GetAnnotationByIndex( nIndex );
#endif
}

inline D3DC_EFFECT_VARIABLE_HANDLE dxC_EffectGetTechniqueAnnotationByIndex( D3D_EFFECT * pEffect, D3DC_TECHNIQUE_HANDLE hTechnique, UINT nIndex )
{
#ifdef ENGINE_TARGET_DX9
	return pEffect->pD3DEffect->GetAnnotation( hTechnique, nIndex );
#else if defined( ENGINE_TARGET_DX10 )
	return hTechnique->GetAnnotationByIndex( nIndex );
#endif
}

inline PRESULT dxC_EffectGetParamDesc( D3D_EFFECT * pEffect, D3DC_EFFECT_VARIABLE_HANDLE hParam, D3DXPARAMETER_DESC* pDesc )
{
#ifdef ENGINE_TARGET_DX9
	return pEffect->pD3DEffect->GetParameterDesc( hParam, pDesc );
#else if defined( ENGINE_TARGET_DX10)
	return hParam->GetDesc( pDesc );
#endif
}

inline int dxC_EffectGetAnnotationAsIntByIndex( D3D_EFFECT * pEffect, D3DC_EFFECT_PASS_HANDLE hPass, UINT nIndex )
{
	D3DC_EFFECT_VARIABLE_HANDLE varHandle = dxC_EffectGetPassAnnotationByIndex( pEffect, hPass, nIndex );
	int nArg = 0;
	ASSERT_RETVAL( EFFECT_INTERFACE_ISVALID( varHandle ), nArg );

#ifdef ENGINE_TARGET_DX9
	V( pEffect->pD3DEffect->GetInt( varHandle, &nArg ) );
#else if defined( ENGINE_TARGET_DX10 )
	V( varHandle->AsScalar()->GetInt( &nArg ) );
#endif

	return nArg;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template< int O > PRESULT dx9_ComputeSHPointLights(
	const D3D_MODEL * pModel,
	const MODEL_RENDER_LIGHT_DATA * ptLights,
	SH_COEFS_RGB<O> & tBaseCoefs,
	SH_COEFS_RGB<O> * ptPointCoefs )
{
	VECTOR vCenterWorld;
	V_DO_FAILED( e_GetModelRenderAABBCenterInWorld( pModel->nId, vCenterWorld ) )
		return S_FALSE;

	int nStartLights = MAX_POINT_LIGHTS_PER_EFFECT;

	for ( int i = nStartLights; i < ptLights->nSHLights; i++ )
	{
		V( dx9_SHCoefsAddPointLight( &tBaseCoefs, *(VECTOR4*)&ptLights->pvSHLightColors[ i ], vCenterWorld, *(VECTOR3*)&ptLights->pvSHLightPos[ i ] ) );
	}
	for ( int i = 0; i < nStartLights && i < ptLights->nSHLights; i++ )
	{
		V( dx9_SHCoefsAddPointLight( &ptPointCoefs[ i ], *(VECTOR4*)&ptLights->pvSHLightColors[ i ], vCenterWorld, *(VECTOR3*)&ptLights->pvSHLightPos[ i ] ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL dx9_EffectUsesSH_O3(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique )
{
	ASSERT_RETFALSE( pEffect );
	return (   0 != dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SH_CAR )
			&& 0 != dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SH_CC  ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline BOOL dx9_EffectUsesSH_O2(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique )
{
	ASSERT_RETFALSE( pEffect );
	return (   0 != dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SH_CAR )
			&& 0 == dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SH_CC  ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline PRESULT dx9_EffectSetFloat ( 
	const D3D_EFFECT * pEffect, 
	D3DC_EFFECT_VARIABLE_HANDLE hParam, 
	float fValue )
{
	REF( pEffect );
	DX9_BLOCK( V_RETURN( pEffect->pD3DEffect->SetFloat( hParam, fValue ) ); )
	DX10_BLOCK( V_RETURN( hParam->AsScalar()->SetFloat( fValue ) ); )
	return S_OK;
}

// CHB 2006.08.21 - Moved from dx9_effect.cpp for optimization.
inline PRESULT dx9_EffectSetFloat ( 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	float fValue )
{
	D3DC_EFFECT_VARIABLE_HANDLE hParam = dx9_GetEffectParam( pEffect, tTechnique, eParam );
	if ( EFFECT_INTERFACE_ISVALID( hParam ) )
	{
		return dx9_EffectSetFloat( pEffect, hParam, fValue );
	}
	return S_FALSE;
}


#endif // __DXC_EFFECT__
