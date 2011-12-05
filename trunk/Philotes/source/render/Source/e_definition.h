//----------------------------------------------------------------------------
// e_definition.h
//
// This file defines all engine definitions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_DEFINITION__
#define __E_DEFINITION__

#include "interpolationpath.h"
#include "definition_common.h"
#include "config.h"
#include "e_light.h"
#include "e_material.h"
#include "e_irradiance.h"
#include "e_2d.h"
#include "e_texture_priv.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define GFXOVERRIDES_DEF_NAME					"GFXOverrides"

//#define GFX_FLAG_FULLSCREEN						MAKE_MASK(0)
//#define GFX_FLAG_FULLSCREEN_VSYNC				MAKE_MASK(1)
#define GFX_FLAG_DISABLE_GLOW					MAKE_MASK(2)
#define GFX_FLAG_DISABLE_NORMAL_MAPS			MAKE_MASK(3)
#define GFX_FLAG_LOD_EFFECTS_ONLY				MAKE_MASK(4)
#define GFX_FLAG_DISABLE_DYNAMIC_LIGHTS			MAKE_MASK(5)
#define GFX_FLAG_DISABLE_ADAPTIVE_BATCHES		MAKE_MASK(6)
#define GFX_FLAG_DISABLE_SHADOWS				MAKE_MASK(7)
#define GFX_FLAG_DISABLE_PARTICLES				MAKE_MASK(8)
#define GFX_FLAG_RANDOM_WINDOWED_SIZE			MAKE_MASK(9)
#define GFX_FLAG_RANDOM_FULLSCREEN_SIZE			MAKE_MASK(10)
#define GFX_FLAG_SOFTWARE_SYNC					MAKE_MASK(11)
//#define GFX_FLAG_WINDOWED_VSYNC					MAKE_MASK(12)
#define GFX_FLAG_ENABLE_MIP_LOD_OVERRIDES		MAKE_MASK(13)


#define ENVIRONMENTDEF_FLAG_DIR1_OPPOSITE_DIR0		MAKE_MASK(0)
#define ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS		MAKE_MASK(1)
#define ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS			MAKE_MASK(2)
#define ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING	MAKE_MASK(3)
#define ENVIRONMENTDEF_FLAG_FLASHLIGHT_EMISSIVE		MAKE_MASK(4)
#define ENVIRONMENTDEF_FLAG_USE_BLOB_SHADOWS		MAKE_MASK(5)

#define ENVIRONMENT_FLAG_RESTORED				MAKE_MASK(0)	// CHB 2007.09.03
#define ENVIRONMENT_FLAG_NO_LOD_EFFECTS			MAKE_MASK(1)
#define ENVIRONMENT_FLAG_SUB_ENVIRONMENT		MAKE_MASK(2)

#define ENVIRONMENT_SUFFIX	"_env.xml"


#define SKYBOX_MODEL_DEF_FLAG_CENTER_ON_CAMERA_XY	MAKE_MASK(0)
#define SKYBOX_MODEL_DEF_FLAG_FOG					MAKE_MASK(1)
#define SKYBOX_MODEL_FLAG_APPEARANCE				MAKE_MASK(2)
#define SKYBOX_MODEL_FLAG_DO_NOT_LOAD				MAKE_MASK(3)		// this particular model has been randomly selected not to show up (based on its chance weight)

#define SKYBOX_SUFFIX		"_skybox.xml"


#define MATERIAL_FLAG_SPECULAR									MAKE_MASK(0)
#define MATERIAL_FLAG_USE_OPACITY_2__DEPRECATED					MAKE_MASK(1)
#define MATERIAL_FLAG_ENVMAP_ENABLED							MAKE_MASK(2)
#define MATERIAL_FLAG_ENVMAP_INVTHRESHOLD						MAKE_MASK(3)
#define MATERIAL_FLAG_VARY_SELFILLUMINATION						MAKE_MASK(4)
#define MATERIAL_FLAG_ALPHABLEND								MAKE_MASK(5)
#define MATERIAL_FLAG_SCROLL_RANDOM_OFFSET						MAKE_MASK(6)
#define MATERIAL_FLAG_CUBE_ENVMAP								MAKE_MASK(7)
#define MATERIAL_FLAG_SPHERE_ENVMAP								MAKE_MASK(8)
#define MATERIAL_FLAG_OVERRIDE_DIFFUSE_TEXTURE					MAKE_MASK(9)
#define MATERIAL_FLAG_OVERRIDE_NORMAL_TEXTURE					MAKE_MASK(10)
#define MATERIAL_FLAG_OVERRIDE_OPACITY_TEXTURE__DEPRECATED		MAKE_MASK(11)
#define MATERIAL_FLAG_OVERRIDE_SELFILLUM_TEXTURE				MAKE_MASK(12)
#define MATERIAL_FLAG_OVERRIDE_SPECULAR_TEXTURE					MAKE_MASK(13)
#define MATERIAL_FLAG_ALPHA_NO_WRITE_DEPTH						MAKE_MASK(14)
#define MATERIAL_FLAG_SCROLLING_ENABLED							MAKE_MASK(15)
#define MATERIAL_FLAG_SUBSURFACE_SCATTERING						MAKE_MASK(16)
#define MATERIAL_FLAG_ALPHAADD									MAKE_MASK(17)
#define MATERIAL_FLAG_TWO_SIDED									MAKE_MASK(18)
#define MATERIAL_FLAG_NO_CAST_SHADOW							MAKE_MASK(19)
#define MATERIAL_FLAG_NO_RECEIVE_SHADOW							MAKE_MASK(20)
#define MATERIAL_FLAG_SCROLL_GLOBAL_PHASE						MAKE_MASK(21)

#define SCREEN_EFFECT_FLOATS		4
#define SCREEN_EFFECT_COLORS		2
#define SCREEN_EFFECT_TEXTURES		2

#define SCREEN_EFFECT_DEF_FLAG_EXCLUSIVE						MAKE_MASK(0)
#define SCREEN_EFFECT_DEF_FLAG_DX10_ONLY						MAKE_MASK(1)

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define DEF_MEMBER_SH_COEF( strukt, coefs, nCoef ) \
	DEFINITION_MEMBER ( strukt, DATA_TYPE_FLOAT, coefs.pfRed[nCoef],   0.f	  ) \
	DEFINITION_MEMBER ( strukt, DATA_TYPE_FLOAT, coefs.pfGreen[nCoef], 0.f	  ) \
	DEFINITION_MEMBER ( strukt, DATA_TYPE_FLOAT, coefs.pfBlue[nCoef],  0.f	  )

#define DEF_MEMBER_SH_O3( strukt, coefs ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 0 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 1 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 2 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 3 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 4 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 5 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 6 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 7 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 8 )

#define DEF_MEMBER_SH_O2( strukt, coefs ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 0 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 1 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 2 ) \
	DEF_MEMBER_SH_COEF( strukt, coefs, 3 )

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum {
	DEFINITION_GROUP_MATERIAL = DEFINITION_GROUP_START_ENGINE,
	DEFINITION_GROUP_TEXTURE,
	DEFINITION_GROUP_LIGHT,
	DEFINITION_GROUP_ENVIRONMENT,
	DEFINITION_GROUP_SKYBOX,
	DEFINITION_GROUP_ENGINEOVERRIDES,
	DEFINITION_GROUP_SCREENEFFECT,
};

enum MODEL_GROUP
{
	MODEL_GROUP_NONE = -1,
	MODEL_GROUP_BACKGROUND = 0,
	MODEL_GROUP_UNITS,
	MODEL_GROUP_PARTICLE,
	MODEL_GROUP_UI,
	MODEL_GROUP_WARDROBE,
	NUM_MODEL_GROUPS,
};
//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct ENV_LIGHT_DEFINITION
{
	VECTOR							 vVec;
	CInterpolationPath<CFloatTriple> tColor;
	float							 fIntensity;
};

struct ENVIRONMENT_DEFINITION
{
	DEFINITION_HEADER				 tHeader;
	DWORD							 dwDefFlags;	// saved definition flags
	DWORD							 dwFlags;
	char							 szSkyBoxFileName[ MAX_XML_STRING_LENGTH ];
	int								 nSkyboxDefID;
	char							 szEnvMapFileName[ MAX_XML_STRING_LENGTH ];
	char							 szBackgroundLightingEnvMapFileName[ MAX_XML_STRING_LENGTH ];
	char							 szAppearanceLightingEnvMapFileName[ MAX_XML_STRING_LENGTH ];
	int								 nBackgroundLightingEnvMap;
	int								 nAppearanceLightingEnvMap;
	SH_COEFS_RGB<2>					 tBackgroundSHCoefs_O2;			// sRGB space
	SH_COEFS_RGB<2>					 tAppearanceSHCoefs_O2;
	SH_COEFS_RGB<3>					 tBackgroundSHCoefs_O3;
	SH_COEFS_RGB<3>					 tAppearanceSHCoefs_O3;
	SH_COEFS_RGB<2>					 tBackgroundSHCoefsLin_O2;		// Linear space
	SH_COEFS_RGB<2>					 tAppearanceSHCoefsLin_O2;
	SH_COEFS_RGB<3>					 tBackgroundSHCoefsLin_O3;
	SH_COEFS_RGB<3>					 tAppearanceSHCoefsLin_O3;
	float							 fBackgroundSHIntensity;
	float							 fAppearanceSHIntensity;
	int								 nEnvMapTextureID;
	int								 nEnvMapMIPLevels;
	float							 fWindMin;
	float							 fWindMax;
	VECTOR							 vWindDirection;
	int								 nClipDistance;
	int								 nSilhouetteDistance;
	int								 nFogStartDistance;
	CInterpolationPath<CFloatTriple> tFogColor;
	ENV_LIGHT_DEFINITION			 tDirLights[ NUM_ENV_DIR_LIGHTS ];
	CInterpolationPath<CFloatTriple> tAmbientColor;
	float							 fAmbientIntensity;
	CInterpolationPath<CFloatTriple> tBackgroundColor;
	CInterpolationPath<CFloatTriple> tHemiLightColors[ 2 ];
	float							 fHemiLightIntensity;
	float							 fShadowIntensity;
	int								 nLocation;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct SKYBOX_MODEL
{
	DWORD								dwFlags;
	char								szModelFile[ MAX_XML_STRING_LENGTH ];
	int									nModelID;
	int									nPass;
	int									nFogStart;
	int									nFogEnd;
	CInterpolationPath<CFloatTriple>	tFogColor;
	float								fAltitude;
	float								fScatterRad;
	float								fChance;
	VECTOR								vOffset;
};

struct SKYBOX_DEFINITION
{
	DEFINITION_HEADER tHeader;

	BOOL								bLoaded;
	CInterpolationPath<CFloatTriple>	tBackgroundColor;
	float								fWorldScale;
	int									nRegionID;

	SKYBOX_MODEL*						pModels;
	int									nModelCount;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct SCREEN_EFFECT_DEFINITION
{
	DEFINITION_HEADER					tHeader;
	DWORD								dwDefFlags;
	char								szTechniqueName[ MAX_XML_STRING_LENGTH ];
	int									nTechniqueInEffect;
	int									nLayer;
	float								fFloats[ SCREEN_EFFECT_FLOATS ];
	CInterpolationPath<CFloatTriple>	tColor0;
	CInterpolationPath<CFloatTriple>	tColor1;
	char								szTextureFilenames[ SCREEN_EFFECT_TEXTURES ][ MAX_XML_STRING_LENGTH ];
	int									nTextureIDs[ SCREEN_EFFECT_TEXTURES ];
	int									nPriority;
	float								fTransitionIn;
	float								fTransitionOut;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define SELF_ILLUM_NOISE_WAVES	5
struct LightmapWaveData
{
	float						fWavePhase;
	float						fNoisePhase[ SELF_ILLUM_NOISE_WAVES ];
	float						fNoiseFreq [ SELF_ILLUM_NOISE_WAVES ];
};

struct MATERIAL
{
	DEFINITION_HEADER			tHeader;
	// This name is also used for the XML file
	char						pszShaderName[ MAX_XML_STRING_LENGTH ];
	int							nShaderLineId;
	//int							nShininess;
	float						fSpecularGlow;
	float						fLightmapGlow;
	float						fGlossiness[2];
	float						fSpecularLevel[2];
	DWORD						dwFlags;
	int							nSpecularLUTId;
	float						fSpecularMaskOverride;
	float						fEnvMapPower;
	float						fEnvMapGlossThreshold;
	float						fEnvMapBlurriness;
	char						szEnvMapFileName[ MAX_XML_STRING_LENGTH ];
	int							nCubeMapTextureID;
	int							nCubeMapMIPLevels;
	int							nSphereMapTextureID;
	int							nOverrideTextureID[ NUM_TEXTURE_TYPES ];
	char						szDiffuseOverrideFileName[ MAX_XML_STRING_LENGTH ];
	char						szNormalOverrideFileName[ MAX_XML_STRING_LENGTH ];
	char						szSelfIllumOverrideFileName[ MAX_XML_STRING_LENGTH ];
	char						szDiffuse2OverrideFileName[ MAX_XML_STRING_LENGTH ];
	char						szSpecularOverrideFileName[ MAX_XML_STRING_LENGTH ];
	float						fSelfIlluminationMax;
	float						fSelfIlluminationMin;
	float						fSelfIlluminationNoiseFreq;
	float						fSelfIlluminationNoiseAmp;
	float						fSelfIlluminationWaveHertz;
	float						fSelfIlluminationWaveAmp;
	float						fScrollRateU[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollRateV[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollTileU[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollTileV[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollPhaseU[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollPhaseV[ NUM_MATERIAL_SCROLL_CHANNELS ];
	float						fScrollAmt[ NUM_SCROLLING_SAMPLER_TYPES ];
	float						fScrollAmtLCM;
	TIME						tScrollLastUpdateTime;			// not initialized by definition system
	char						szParticleSystemDef[ MAX_XML_STRING_LENGTH ];
	int							nParticleSystemDefId;
	float						fScatterIntensity;
	float						fScatterSharpness;
	CInterpolationPath<CFloatTriple>	tScatterColor;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// An ID is needed to distinguish blend texture definitions that are loading
// from ones that aren't there.
#define BLEND_TEXTURE_DEFINITION_LOADING_ID ((DWORD)-2)

struct BLEND_RUN
{
	int nTotalAlpha;
	int nBlockStart;
	int nBlockRun;
	int nAlphaCount;
	BYTE* pbAlpha;
};

struct BLEND_RLE
{
	int nCount;
	BLEND_RUN* pRuns;
};

#define MAX_TEXTURE_DEFINITIONS 50000
class TEXTURE_DEFINITION
{
public:
	TEXTURE_DEFINITION() : nFileSize(0) {}

	DEFINITION_HEADER			tHeader;
	int							nFormat; // one of the TEXTURE_FORMAT types
	DWORD						tBinding; // How is this texture bound?  Is it a render target?
	int							nFileSize;
	int							nMipMapLevels;	
	int							nArraySize;
	int							nMipMapUsed;			
	char						pszMaterialName[ MAX_XML_STRING_LENGTH ];
	int							nMeshLODPriority;
	int							nBlendWidth;
	int							nBlendHeight;
	BLEND_RLE*					pBlendRLE;
	int							nBlendRLELevels;
	int							nFramesU; // this is for animated textures
	int							nFramesV;
	int							nWidth;
	int							nHeight;
	DWORD						dwConvertFlags;
	float						fBlurFactor;
	int							nSharpenFilter;
	int							nSharpenPasses;
	int							nMIPFilter;
};

struct STATIC_LIGHT_DEFINITION		
{
	DWORD					dwFlags;
	CFloatTriple			tColor;
	float					fIntensity;
	CFloatPair				tFalloff;
	float					fPriority;
	VECTOR					vPosition;
};

struct LIGHT_DEFINITION		
{
	DEFINITION_HEADER					tHeader;
	LIGHT_TYPE							eType;
	DWORD								dwFlags;
	int									nDurationType;
	float								fStartTime;
	float								fLoopTime;
	float								fEndTime;
	float								fSpotAngleDeg;
	CInterpolationPath<CFloatTriple>	tColor;
	CInterpolationPath<CFloatPair>		tIntensity;
	CInterpolationPath<CFloatPair>		tFalloff;
	char								szSpotUmbraTexture[ MAX_XML_STRING_LENGTH ];
	int									nSpotUmbraTextureID;
};

struct ENGINE_OVERRIDES_DEFINITION
{
	DEFINITION_HEADER tHeader;

	DWORD		dwFlags;
	// windowed
	int			dwWindowedResWidth;
	int			dwWindowedResHeight;
	int			nWindowedPositionX;
	int			nWindowedPositionY;
	DWORD		dwWindowedColorFormat;
	// fullscreen
	//DWORD		dwFullscreenResWidth;
	//DWORD		dwFullscreenResHeight;
	//DWORD		dwFullscreenRefreshRate;
	DWORD		dwFullscreenColorFormat;
	//int			nMSAA;
	// general
	//float		fGammaAdjust;
	int			nHDRMode;
	// performance
	DWORD		dwAdaptiveBatchTarget;
	int			pnTextureLODDown[ NUM_TEXTURE_GROUPS ][ NUM_TEXTURE_TYPES ];
	//int			nTextureDetail;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void DefinitionInitEngine ( void );

//----------------------------------------------------------------------------
// INLINESs
//----------------------------------------------------------------------------


inline struct ENGINE_OVERRIDES_DEFINITION * e_GetEngineOverridesDefinition()
{
#if !ISVERSION(DEVELOPMENT) || ISVERSION(SERVER_VERSION)
	return NULL;
#else
	extern int gnEngineOverridesDefinitionID;
	if ( gnEngineOverridesDefinitionID == INVALID_ID )
		gnEngineOverridesDefinitionID = DefinitionGetIdByName( DEFINITION_GROUP_ENGINEOVERRIDES, GFXOVERRIDES_DEF_NAME, -1, TRUE, TRUE );
	return (struct ENGINE_OVERRIDES_DEFINITION *)DefinitionGetById( DEFINITION_GROUP_ENGINEOVERRIDES, gnEngineOverridesDefinitionID );
#endif // RETAIL
}


#endif // __E_DEFINITION__