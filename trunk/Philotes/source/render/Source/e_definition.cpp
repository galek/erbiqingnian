//----------------------------------------------------------------------------
// e_definition.cpp
//
// This file defines all engine definitions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "interpolationpath.h"
#include "e_material.h"
#include "e_settings.h"
#include "e_caps.h"
#include "e_environment.h"
#include "filepaths.h"
#include "e_definition.h"
#include "definition_priv.h"
#include "e_2d.h"


int gnEngineOverridesDefinitionID;

//-------------------------------------------------------------------------------------------------
DEFINITION_START (SCREEN_EFFECT_DEFINITION, sgtDefinitionLoaderScreenFX, 'SCFX', TRUE, FALSE, NULL, 16)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_STRING,				szTechniqueName,						)
	DEFINITION_MEMBER_FLAG ( SCREEN_EFFECT_DEFINITION, dwDefFlags,				SCREEN_EFFECT_DEF_FLAG_EXCLUSIVE,	0	)
	DEFINITION_MEMBER_FLAG ( SCREEN_EFFECT_DEFINITION, dwDefFlags,				SCREEN_EFFECT_DEF_FLAG_DX10_ONLY,	0	)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_INT_NOSAVE,			nTechniqueInEffect,			-1			)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_INT_NOSAVE,			nLayer,						0			)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fFloats[0],					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fFloats[1],					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fFloats[2],					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fFloats[3],					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE,	tColor0,					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE,	tColor1,					0.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_STRING,				szTextureFilenames[0],					)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_STRING,				szTextureFilenames[1],					)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_INT_NOSAVE,			nTextureIDs[0],				-1			)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_INT_NOSAVE,			nTextureIDs[1],				-1			)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_INT,				nPriority,					-1			)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fTransitionIn,				1.0f		)
	DEFINITION_MEMBER ( SCREEN_EFFECT_DEFINITION, DATA_TYPE_FLOAT,				fTransitionOut,				1.0f		)
DEFINITION_END

class CScreenEffectsContainer : public CDefinitionContainer
{
public:
	CScreenEffectsContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderScreenFX;
		m_pszFilePath = FILE_PATH_SCREENFX;
		m_bAsyncLoad = TRUE;
		m_bZeroOutData = FALSE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_SCREENFX;
		InitLoader();
	}
	void Free  ( void * pData, int nCount )
	{
		m_pLoader->FreeElements( pData, nCount, FALSE );
		FREE( NULL, pData );
	}
	void * Alloc ( int nCount ) { return MALLOC( NULL, sizeof( SCREEN_EFFECT_DEFINITION ) * nCount ); }
	// AE 2007.08.21: These are restored by the screen effects system upon use, which also takes care
	//		of the effect shader being loaded asynchronously.
	//void Restore ( void * pData ) { V( e_ScreenFXRestore( pData ) ); }
};

//-------------------------------------------------------------------------------------------------
DEFINITION_START (MATERIAL, sgtDefinitionLoaderMaterial, 'MATD', TRUE, FALSE, MaterialDefinitionPostXMLLoad, 256)
//	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT,		nShininess,						16			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSpecularGlow,					0.1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fLightmapGlow,					1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		pszShaderName,					Appearance	)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT,		dwFlags,						0			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fGlossiness[0],					0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fGlossiness[1],					1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSpecularLevel[0],				1.0f		)
//	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSpecularLevel[1],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE, nShaderLineId,					-1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE, nSpecularLUTId,					-1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSpecularMaskOverride,			0.5f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fEnvMapPower,					0.1f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fEnvMapGlossThreshold,			1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fEnvMapBlurriness,				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szEnvMapFileName,							)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nCubeMapTextureID,				-1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nCubeMapMIPLevels,				0			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nSphereMapTextureID,			-1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_DIFFUSE ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_NORMAL ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_SELFILLUM ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_DIFFUSE2 ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_SPECULAR ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_ENVMAP ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nOverrideTextureID[ TEXTURE_LIGHTMAP ]	,	-1)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szDiffuseOverrideFileName,					)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szNormalOverrideFileName,					)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szSelfIllumOverrideFileName,				)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szDiffuse2OverrideFileName,					)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szSpecularOverrideFileName,					)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationMax,			1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationMin,			0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationNoiseFreq,		1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationNoiseAmp,		0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationWaveHertz,		1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fSelfIlluminationWaveAmp,		0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollRateU[0],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollRateV[0],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollTileU[0],				1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollTileV[0],				1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT_NOSAVE,fScrollPhaseU[0],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT_NOSAVE,fScrollPhaseV[0],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollRateU[1],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollRateV[1],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollTileU[1],				1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollTileV[1],				1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT_NOSAVE,fScrollPhaseU[1],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT_NOSAVE,fScrollPhaseV[1],				0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollAmt[SAMPLER_DIFFUSE],	0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollAmt[SAMPLER_NORMAL],		0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollAmt[SAMPLER_SPECULAR],	0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScrollAmt[SAMPLER_SELFILLUM],	0.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT_NOSAVE,fScrollAmtLCM,					1.0f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_STRING,		szParticleSystemDef,						)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_INT_NOSAVE,	nParticleSystemDefId,			-1			)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScatterIntensity,				0.3f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_FLOAT,		fScatterSharpness,				0.5f		)
	DEFINITION_MEMBER ( MATERIAL, DATA_TYPE_PATH_FLOAT_TRIPLE,	tScatterColor, 			0.7f 		)
DEFINITION_END

class CMaterialContainer : public CDefinitionContainer
{
public:
	CMaterialContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderMaterial;
		m_pszFilePath = FILE_PATH_MATERIAL;
		m_bAsyncLoad = TRUE;
		m_bZeroOutData = FALSE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_MATERIAL;
		m_bAlwaysWarnWhenMissing = TRUE;
		InitLoader();
	}
	void Free  ( void * pData, int nCount )
	{
		m_pLoader->FreeElements( pData, nCount, FALSE );
		FREE( NULL, pData );
	}
	void * Alloc ( int nCount ) { return MALLOC ( NULL, sizeof( MATERIAL ) * nCount ); }
	void Restore ( void * pData ) { e_MaterialRestore ( pData ); }
};

//-------------------------------------------------------------------------------------------------
DEFINITION_START (BLEND_RUN, sgtDefinitionLoaderBlendRun, 'BRUN', FALSE, FALSE, NULL, 0 )
	DEFINITION_MEMBER(			BLEND_RUN, DATA_TYPE_INT,	nTotalAlpha,	0 )
	DEFINITION_MEMBER(			BLEND_RUN, DATA_TYPE_INT,	nBlockStart,	0 )
	DEFINITION_MEMBER(			BLEND_RUN, DATA_TYPE_INT,	nBlockRun,		0 )
	DEFINITION_MEMBER_VARRAY(	BLEND_RUN, DATA_TYPE_BYTE,	pbAlpha,		0, nAlphaCount )
DEFINITION_END

DEFINITION_START (BLEND_RLE, sgtDefinitionLoaderBlendRLE, 'BRLE', FALSE, FALSE, NULL, 0 )
	DEFINITION_REFERENCE_VARRAY( BLEND_RLE, sgtDefinitionLoaderBlendRun,	pRuns,	nCount )
DEFINITION_END

DEFINITION_START (TEXTURE_DEFINITION, sgtDefinitionLoaderTexture, 'TEXD', TRUE, FALSE, NULL, 512 )
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nFormat,					0 		) // if it is 0, then we will fill in the format
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT_NOSAVE,			tBinding,					0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT_NOSAVE,			nFileSize,					0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nMipMapLevels,				0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nArraySize,					1		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nMipMapUsed,				0 		)
	DEFINITION_MEMBER			( TEXTURE_DEFINITION, DATA_TYPE_STRING,				pszMaterialName,			Default	)
	DEFINITION_MEMBER			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nMeshLODPriority,			0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nBlendWidth,				0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nBlendHeight,				0 		)
	DEFINITION_REFERENCE_VARRAY	( TEXTURE_DEFINITION, sgtDefinitionLoaderBlendRLE,	pBlendRLE,					nBlendRLELevels )
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nFramesU,					1 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nFramesV,					1 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT_NOSAVE,			nWidth,						0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT_NOSAVE,			nHeight,					0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				dwConvertFlags,				0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_FLOAT,				fBlurFactor,				1.0f	)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nSharpenFilter,				0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nSharpenPasses,				0 		)
	DEFINITION_MEMBER 			( TEXTURE_DEFINITION, DATA_TYPE_INT,				nMIPFilter,					6 		)
DEFINITION_END

class CTextureContainer : public CDefinitionContainer
{
public:
	CTextureContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderTexture;
		m_pszFilePath = FILE_PATH_DATA;
		m_bIgnoreFilePathOnSave = TRUE;
		m_bRemoveExtension = FALSE;
		m_bNodirectIgnoreMissing = TRUE;
		m_bAsyncLoad = TRUE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_TEXTURE;
		InitLoader();
	}

	void Free(
		void * data,
		int nCount) 
	{ 
		m_pLoader->FreeElements(data, nCount, FALSE);
		FREE_DELETE_ARRAY(NULL, data, TEXTURE_DEFINITION); 
	}
	void * Alloc (
		int nCount) 
	{ 
		return MALLOC_NEWARRAY(NULL, TEXTURE_DEFINITION, nCount);  
	}
};

//-------------------------------------------------------------------------------------------------
DEFINITION_START (LIGHT_DEFINITION, sgtDefinitionLoaderLight, 'LIGD', TRUE, FALSE, LightDefinitionPostXMLLoad, 64)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_INT,				eType,					0 )
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_INT,				nDurationType,			1		)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_FLOAT, 				fStartTime, 			0.5f	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_FLOAT, 				fLoopTime, 				0.5f	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_FLOAT, 				fEndTime, 				0.5f	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_FLOAT, 				fSpotAngleDeg,			45		)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE,	tColor, 				0.5f 	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_PATH_FLOAT_PAIR, 	tFalloff, 				0.0f 	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_PATH_FLOAT_PAIR, 	tIntensity,				1.0f 	)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_INT_NOSAVE,			dwFlags,				0		)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_STRING,				szSpotUmbraTexture,				)
	DEFINITION_MEMBER ( LIGHT_DEFINITION, DATA_TYPE_INT_NOSAVE,			nSpotUmbraTextureID,	-1		)
DEFINITION_END

class CLightContainer : public CDefinitionContainer
{
public:
	CLightContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderLight;
		m_pszFilePath = FILE_PATH_LIGHT;
		m_bZeroOutData = FALSE;
		m_bAsyncLoad = TRUE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_LIGHT;
		InitLoader();
	}
	void Free  ( void * pData, int nCount )
	{
		m_pLoader->FreeElements( pData, nCount, FALSE );
		FREE( NULL, pData );
	}
	void * Alloc ( int nCount ) { return MALLOC ( NULL, sizeof( LIGHT_DEFINITION ) * nCount ); }
};


//-------------------------------------------------------------------------------------------------
DEFINITION_START ( ENV_LIGHT_DEFINITION, sgtDefinitionLoaderEnvLight, 'ENLD', FALSE, FALSE, NULL, 0 )
	DEFINITION_MEMBER ( ENV_LIGHT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE,	tColor,		1.0f	)
	DEFINITION_MEMBER ( ENV_LIGHT_DEFINITION, DATA_TYPE_FLOAT,				fIntensity,	0.0f	)
	DEFINITION_MEMBER ( ENV_LIGHT_DEFINITION, DATA_TYPE_FLOAT,				vVec.fX,	-1.0f	)
	DEFINITION_MEMBER ( ENV_LIGHT_DEFINITION, DATA_TYPE_FLOAT,				vVec.fY,	-1.0f	)
	DEFINITION_MEMBER ( ENV_LIGHT_DEFINITION, DATA_TYPE_FLOAT,				vVec.fZ,	1.0f	)
DEFINITION_END

//-------------------------------------------------------------------------------------------------
DEFINITION_START  ( ENVIRONMENT_DEFINITION, sgtDefinitionLoaderEnvironment, 'ENVD', TRUE, FALSE, EnvDefinitionPostXMLLoad, 32)
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT,				 dwDefFlags,								0	  )	// CHB 2007.07.04 - Got rid of _NOSAVE
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_DIR1_OPPOSITE_DIR0,	0	  )
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS,		0	  )
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS,		0	  )
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING,	0	  )
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_FLASHLIGHT_EMISSIVE,	0	  )
	DEFINITION_MEMBER_FLAG ( ENVIRONMENT_DEFINITION,	dwDefFlags,			 ENVIRONMENTDEF_FLAG_USE_BLOB_SHADOWS,		0	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 dwFlags,									0	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_STRING,			 szSkyBoxFileName,								  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 nSkyboxDefID,								-1	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_STRING,			 szEnvMapFileName,								  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_STRING,			 szBackgroundLightingEnvMapFileName,			  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_STRING,			 szAppearanceLightingEnvMapFileName,			  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 nBackgroundLightingEnvMap,					-1	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 nAppearanceLightingEnvMap,					-1	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 nEnvMapTextureID,							-1	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT_NOSAVE,		 nEnvMapMIPLevels,							0	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fWindMin,									0     )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fWindMax,									0     )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 vWindDirection.fX,							0     )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 vWindDirection.fY,							1     )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 vWindDirection.fZ,							0     )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT,				 nClipDistance,								100   )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT,				 nSilhouetteDistance,						60    )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT,				 nFogStartDistance,							30    )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE, tFogColor,									0.5f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE, tAmbientColor,								1.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fAmbientIntensity,							0.25f )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE, tBackgroundColor,							1.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE, tHemiLightColors[0],						0.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE, tHemiLightColors[1],						0.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fHemiLightIntensity,						0.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fShadowIntensity,							1.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_INT,				 nLocation,									1	  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fBackgroundSHIntensity,					1.0f  )
	DEFINITION_MEMBER ( ENVIRONMENT_DEFINITION, DATA_TYPE_FLOAT,			 fAppearanceSHIntensity,					1.0f  )
	DEF_MEMBER_SH_O2  ( ENVIRONMENT_DEFINITION,								 tBackgroundSHCoefs_O2							  )
	DEF_MEMBER_SH_O2  ( ENVIRONMENT_DEFINITION,								 tAppearanceSHCoefs_O2							  )
	DEF_MEMBER_SH_O3  ( ENVIRONMENT_DEFINITION,								 tBackgroundSHCoefs_O3							  )
	DEF_MEMBER_SH_O3  ( ENVIRONMENT_DEFINITION,								 tAppearanceSHCoefs_O3							  )
	DEF_MEMBER_SH_O2  ( ENVIRONMENT_DEFINITION,								 tBackgroundSHCoefsLin_O2						  )
	DEF_MEMBER_SH_O2  ( ENVIRONMENT_DEFINITION,								 tAppearanceSHCoefsLin_O2						  )
	DEF_MEMBER_SH_O3  ( ENVIRONMENT_DEFINITION,								 tBackgroundSHCoefsLin_O3						  )
	DEF_MEMBER_SH_O3  ( ENVIRONMENT_DEFINITION,								 tAppearanceSHCoefsLin_O3						  )
	DEFINITION_REFERENCE_ARRAY ( ENVIRONMENT_DEFINITION, sgtDefinitionLoaderEnvLight, tDirLights, NUM_ENV_DIR_LIGHTS )
DEFINITION_END

class CEnvironmentContainer : public CDefinitionContainer
{
public:
	CEnvironmentContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderEnvironment;
		m_pszFilePath = FILE_PATH_BACKGROUND;
		m_bZeroOutData = FALSE;
		m_bAlwaysWarnWhenMissing = TRUE;
		m_bAsyncLoad = TRUE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_ENVIRONMENT;
		InitLoader();
	}
	void Free  ( void * pData, int nCount )
	{
		m_pLoader->FreeElements( pData, nCount, FALSE );
		FREE( NULL, pData );
	}
	void * Alloc ( int nCount ) { return MALLOC ( NULL, sizeof( ENVIRONMENT_DEFINITION ) * nCount ); }
};

//-------------------------------------------------------------------------------------------------
DEFINITION_START ( SKYBOX_MODEL, sgtDefinitionLoaderSkyboxModel, 'SBMD', FALSE, FALSE, NULL, 0 )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_INT,				dwFlags,			0	 )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_STRING,				szModelFile,			 )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_INT_DEFAULT,		nModelID,			-1	 )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_INT,				nPass,				-1	 )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_INT,				nFogStart,			150  )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_INT,				nFogEnd,			300  )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_PATH_FLOAT_TRIPLE,	tFogColor,			0.5f )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_FLOAT,				fAltitude,			0.f  )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_FLOAT,				fScatterRad,		0.f  )
	DEFINITION_MEMBER ( SKYBOX_MODEL, DATA_TYPE_FLOAT,				fChance,			1.f  )
DEFINITION_END

DEFINITION_START  ( SKYBOX_DEFINITION, sgtDefinitionLoaderSkybox, 'SKYD', TRUE, FALSE, SkyboxDefinitionPostXMLLoad, 16)
	DEFINITION_MEMBER ( SKYBOX_DEFINITION, DATA_TYPE_INT_NOSAVE,		bLoaded,					0	)
	DEFINITION_MEMBER ( SKYBOX_DEFINITION, DATA_TYPE_PATH_FLOAT_TRIPLE,	tBackgroundColor,			0.0f	)
	DEFINITION_MEMBER ( SKYBOX_DEFINITION, DATA_TYPE_FLOAT,				fWorldScale,				0.0f	)
	DEFINITION_MEMBER ( SKYBOX_DEFINITION, DATA_TYPE_INT_NOSAVE,		nRegionID,					-1	)
	DEFINITION_REFERENCE_VARRAY ( SKYBOX_DEFINITION, sgtDefinitionLoaderSkyboxModel, pModels, nModelCount )
DEFINITION_END

class CSkyboxContainer : public CDefinitionContainer
{
public:
	CSkyboxContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderSkybox;
		m_pszFilePath = FILE_PATH_BACKGROUND;
		m_bZeroOutData = FALSE;
		m_bAlwaysWarnWhenMissing = TRUE;
		m_bAsyncLoad = TRUE;
		m_nLoadPriority = ENGINE_FILE_PRIORITY_SKYBOX;
		InitLoader();
	}
	void Free  ( void * pData, int nCount )
	{
		m_pLoader->FreeElements( pData, nCount, FALSE );
		FREE( NULL, pData );
	}
	void * Alloc ( int nCount ) { return MALLOC ( NULL, sizeof( SKYBOX_DEFINITION ) * nCount ); }
};




//-------------------------------------------------------------------------------------------------
DEFINITION_START  ( ENGINE_OVERRIDES_DEFINITION, sgtDefinitionLoaderEngineSettings, 'ENGD', TRUE, TRUE, NULL, 4)
	//DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_FULLSCREEN,						0						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwWindowedResWidth,							-1						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwWindowedResHeight,						-1						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			nWindowedPositionX,							-1						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			nWindowedPositionY,							-1						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwWindowedColorFormat,						0						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwFullscreenResWidth,						0						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwFullscreenResHeight,						0						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwFullscreenRefreshRate,					60						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwFullscreenColorFormat,					0						)
	//DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_FULLSCREEN_VSYNC,					1						)
	//DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_WINDOWED_VSYNC,					1						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			nMSAA,										0						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_FLOAT,		fGammaAdjust,								1.f						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			nHDRMode,									0						)
	DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			dwAdaptiveBatchTarget,						300						)
	//DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,			nTextureDetail,								10						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_GLOW,						0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_NORMAL_MAPS,				0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_LOD_EFFECTS_ONLY,					0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_DYNAMIC_LIGHTS,			0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_ADAPTIVE_BATCHES,			0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_SHADOWS,					0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_DISABLE_PARTICLES,					0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_RANDOM_WINDOWED_SIZE,				0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_RANDOM_FULLSCREEN_SIZE,			0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_SOFTWARE_SYNC,						0						)
	DEFINITION_MEMBER_FLAG ( ENGINE_OVERRIDES_DEFINITION, dwFlags,			GFX_FLAG_ENABLE_MIP_LOD_OVERRIDES,			0						)
	#define SETTINGS_TEX_LOD_BIAS(group)	\
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_DIFFUSE],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_NORMAL],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_SELFILLUM],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_DIFFUSE2],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_SPECULAR],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_ENVMAP],	0	) \
		DEFINITION_MEMBER ( ENGINE_OVERRIDES_DEFINITION, DATA_TYPE_INT,		pnTextureLODDown[group][TEXTURE_LIGHTMAP],	0	)
	SETTINGS_TEX_LOD_BIAS(TEXTURE_GROUP_BACKGROUND)
	SETTINGS_TEX_LOD_BIAS(TEXTURE_GROUP_PARTICLE)
	SETTINGS_TEX_LOD_BIAS(TEXTURE_GROUP_UI)
	SETTINGS_TEX_LOD_BIAS(TEXTURE_GROUP_UNITS)
	SETTINGS_TEX_LOD_BIAS(TEXTURE_GROUP_WARDROBE)
DEFINITION_END

class CEngineSettingsContainer : public CDefinitionContainer
{
public:
	CEngineSettingsContainer() 
	{
		m_pLoader = & sgtDefinitionLoaderEngineSettings;
		m_pszFilePath = FILE_PATH_GLOBAL;
		m_bZeroOutData = FALSE;
		m_bAlwaysLoadDirect = TRUE;
		InitLoader();
	}
	void Free(
		void * data,
		int nCount)
	{ 
		REF(nCount);
		FREE_DELETE_ARRAY(NULL, data, ENGINE_OVERRIDES_DEFINITION); 
	}

	void * Alloc(
		int nCount)
	{ 
		return MALLOC_NEWARRAY(NULL, ENGINE_OVERRIDES_DEFINITION, nCount); 
	}
};


//-------------------------------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------------------------------

void DefinitionInitEngine ( void )
{
	DefinitionContainerGetRef( DEFINITION_GROUP_MATERIAL 					) = new CMaterialContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_TEXTURE  					) = new CTextureContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_LIGHT    					) = new CLightContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_ENVIRONMENT					) = new CEnvironmentContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_SKYBOX						) = new CSkyboxContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_ENGINEOVERRIDES				) = new CEngineSettingsContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_SCREENEFFECT				) = new CScreenEffectsContainer;

	gnEngineOverridesDefinitionID = INVALID_ID;
}

