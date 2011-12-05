//----------------------------------------------------------------------------
// dx9_render.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "plane.h"
#include "dxC_model.h"
#include "dxC_effect.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_drawlist.h"
#include "dxC_primitive.h"
#include "dxC_target.h"
#include "dxC_environment.h"
#include "dxC_shadow.h"
#include "dxC_caps.h"
#include "dxC_material.h"
#include "dxC_query.h"
#include "dxC_irradiance.h"
#include "dxC_profile.h"
#include "e_frustum.h"
#include "e_attachment.h"
#include "dxC_2d.h"
#include "camera_priv.h"
#include "perfhier.h"
#include "appcommontimer.h"
#include "dxC_profile.h"
#include "dxC_statemanager.h"
#include "dxC_texture.h"
#include "e_particle.h"
#include "e_texture_priv.h"
#include "e_primitive.h"
#include "e_main.h"
#include "p_windows.h"
#include "game.h"
#include "level.h"
#include "e_visibilitymesh.h"
#include "dxC_main.h"
#include "dxC_render.h"
#include "dx9_shadowtypeconstants.h"		// CHB 2006.11.03
#include "dxC_meshlist.h"
#include "dxC_VertexDeclarations.h"
#include "dxC_pass.h"
#include "dxC_pixo.h"

#include "dxC_matrix.h"

#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
#include "dx10_ParticleSimulateGPU.h"
#include "e_particles_priv.h"
#endif

#include "c_appearance.h"	// CHB 2006.10.19 - For c_AppearanceSetLODModelDefinitionId().
#include "e_optionstate.h"	// CHB 2003.03.16

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct RenderTextureUsage
{
	int nMaxWidth, nMaxHeight;  // render target dimensions
	int nCurX, nCurY;			// current upper-left corner of viewport
	int nUsedHeight;			// current height of row (max of all the render heights on this row)
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
D3D_INDEX_BUFFER_DEFINITION  gt2DBoundingBoxIndexBuffer;
D3D_VERTEX_BUFFER_DEFINITION gt2DBoundingBoxVertexBuffer;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static void sTraceFloatArray( const char * pszText, float * pfValues, int nCount )
{
	if ( IsKeyDown( KEYTRACE_CLASS::GetKey() ) )
	{
		char szNum[ 8 ];
		char szLine[ 256 ];
		PStrPrintf( szLine, 256, "### %s", pszText );
		for ( int i = 0; i < nCount; i++ )
		{
			PStrPrintf( szNum, 8, " %2.2f", pfValues[ i ] );
			PStrCat( szLine, szNum, 256 );
		}
		keytrace( "%s\n", szLine );
	}
}


static PRESULT sComputeLightsSH(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	if ( dx9_EffectUsesSH_O2( pEffect, tTechnique ) )
	{
		V_RETURN( dxC_ComputeLightsSH_O2( pModel, tLights ) );
	}

	if ( dx9_EffectUsesSH_O3( pEffect, tTechnique ) )
	{
		V_RETURN( dxC_ComputeLightsSH_O3( pModel, tLights ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetSHEffectParameters(
	const D3D_MODEL * pModel,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights,
	MESH_RENDER_LIGHT_DATA & tMeshLights,
	BOOL bForceCurrentEnv )
{
	V_RETURN( sComputeLightsSH( pModel, pEffect, tTechnique, tLights ) );
	V_RETURN( dx9_EffectSetSHLightingParams( pModel, pEffect, pEffectDef, tTechnique, pEnvDef, &tLights, &tMeshLights, bForceCurrentEnv ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//#define POINT_LIGHT_STATS 1
#if POINT_LIGHT_STATS
static struct
{
	unsigned nTotalPointLights;		// total point lights processed
	unsigned nUselessPointLights;	// total of all useless point lights processed
	unsigned nEffectLightWaste;		// total of filler dummy point lights had to be inserted
	unsigned nEffectLightCount;		// total of all effect point light slots processed
	unsigned nPointLightUsage[MAX_POINT_LIGHTS_PER_EFFECT + 1];		// count of how many times each number of point lights was encountered
	unsigned nEffectLightUsage[MAX_POINT_LIGHTS_PER_EFFECT + 1];	// count of how many times each number of effect point light slots was encountered
	unsigned nUselessLightUsage[MAX_POINT_LIGHTS_PER_EFFECT + 1];	// count of how many times each number of useless point lights was encountered
} s_PointLightStats = { 0 };
#endif

bool dxC_IsLightUseful(const D3DXVECTOR4 & vDiffuse)
{
	const float fCutoff = 1.0f / 255.0f;
	return (vDiffuse.x >= fCutoff) && (vDiffuse.y >= fCutoff) && (vDiffuse.z >= fCutoff);
}

static PRESULT sSetPointLightEffectParameters(
	const D3D_MODEL * pModel,
	const DRAWLIST_STATE * pState,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	float fEffectTransitionStr,
	MODEL_RENDER_LIGHT_DATA & tLights,
	MESH_RENDER_LIGHT_DATA & tMeshLights )
{
	if (    dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTPOS_OBJECT )
		 || dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTPOS_WORLD ) )
	{
		// this is a real array since we need to modify a copy
		D3DXVECTOR4 vPointLightColors[ MAX_POINT_LIGHTS_PER_EFFECT ];
		C_ASSERT( sizeof(vPointLightColors) == sizeof(tLights.ModelPoint.pvColors) );

		const bool bGlobal = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS );
		MODEL_RENDER_LIGHT_DATA::POINTS & tPoints = bGlobal ? tLights.GlobalPoint : tLights.ModelPoint;	// not const because calls below change the underlying tLights struct
		const D3DXVECTOR4 * pvColors = tPoints.pvColors;

		if ( bGlobal )
		{
			// global slot diffuse point lights
			V_RETURN( dxC_AssembleLightsGlobalPoint( pModel, tLights ) );
		}
		else
		{
			// per-model selected diffuse point lights
			V_RETURN( dxC_AssembleLightsPoint( pModel, tLights ) );

			//ASSERT( tLights.nModelPointLights == tMeshLights.nRealLights + tMeshLights.nSplitLights );

			if ( tPoints.nLights > 0 )
			{
				// always setting the split lights here because we want the specular selection to fade

				if ( tMeshLights.nSplitLights > 0 )
				{
					MemoryCopy( vPointLightColors, sizeof(vPointLightColors), tPoints.pvColors, sizeof(D3DXVECTOR4) * tPoints.nLights );
					pvColors = vPointLightColors;
					// change the split light's real-part intensity by the calculated factor
					vPointLightColors[ tMeshLights.nRealLights ] *= tMeshLights.fSplitRealStr;
				}
			}
		}

		const int nPointLights = tPoints.nLights;
		if ( nPointLights > 0 )
		{
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTPOS_WORLD,			tPoints.pvPosWorld,			nPointLights ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTPOS_OBJECT,		tPoints.pvPosObject,		nPointLights ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTFALLOFF_WORLD,		tPoints.pvFalloffsWorld,	nPointLights ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTFALLOFF_OBJECT,	tPoints.pvFalloffsObject,	nPointLights ) );

			// if the effect specifies a point light count, use that in order to clear the extra constants
			int nEffectLights = tTechnique.tFeatures.nInts[ TF_INT_MODEL_POINTLIGHTS ];

			// CHB 2007.09.04 - Count usage
			#if POINT_LIGHT_STATS
			s_PointLightStats.nPointLightUsage[nPointLights]++;
			s_PointLightStats.nEffectLightUsage[nEffectLights]++;
			s_PointLightStats.nEffectLightCount++;
			s_PointLightStats.nTotalPointLights += nPointLights;
			unsigned nUseless = 0;
			for (int i = 0; i < nPointLights; ++i)
			{
				nUseless += !dxC_IsLightUseful(pvColors[i]);
			}
			s_PointLightStats.nUselessLightUsage[nUseless]++;
			s_PointLightStats.nUselessPointLights += nUseless;
			#endif

			// to clear the extra constants, set all the point light colors to defaults first
			if (nEffectLights > nPointLights)
			{
				// CHB 2007.09.04 - Slightly more optimal.
				D3DXVECTOR4 vColors[ MAX_POINT_LIGHTS_PER_EFFECT ];
				memcpy(vColors, pvColors, nPointLights * sizeof(D3DXVECTOR4));
				memset(vColors + nPointLights, 0, (nEffectLights - nPointLights) * sizeof(D3DXVECTOR4));
				V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTCOLOR, vColors, nEffectLights ) );
				#if POINT_LIGHT_STATS
				s_PointLightStats.nEffectLightWaste += nEffectLights - nPointLights;
				#endif
			}
			else
			{
				V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTCOLOR, pvColors, min(nPointLights, nEffectLights) ) );
			}
		}
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetSpecularEffectParameters(
	const D3D_MODEL * pModel,
	const DRAWLIST_STATE * pState,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	float fEffectTransitionStr,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	if (    dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTPOS_OBJECT )
		 || dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTPOS_WORLD ) )
	{
		const bool bGlobal = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS );
		if ( bGlobal )
		{
			// global slot specular point lights
			V_RETURN( dxC_AssembleLightsGlobalSpec( pModel, pEnvDef, tLights ) );
		}
		else
		{
			// specular only lights - add specular to the lightmap
			V_RETURN( dxC_SelectLightsSpecularOnly( pModel, pState, pEnvDef, tLights ) );
		}

		const MODEL_RENDER_LIGHT_DATA::SPECULARS & tPoints = bGlobal ? tLights.GlobalSpec : tLights.ModelSpec;
		//const int nSpecularLights = tPoints.nLights;
		//if ( nSpecularLights > 0 )
		{
			//D3DXVECTOR4 pvModifiedColors[ MAX_SPECULAR_LIGHTS_PER_EFFECT ];
			//if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_LOD_TRANS_SPECULAR ) && fEffectTransitionStr < 1.f )
			//{
			//	// transitioning to a backup effect without specular, so fade the colors
			//	for ( int i = 0; i < nSpecularLights; i++ )
			//	{
			//		pvModifiedColors[ i ] = pvColors[ i ] * fEffectTransitionStr;
			//	}
			//	pvColors = pvModifiedColors;
			//}

			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTPOS_WORLD,		tPoints.pvPosWorld,			MAX_SPECULAR_LIGHTS_PER_EFFECT ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTPOS_OBJECT,		tPoints.pvPosObject,		MAX_SPECULAR_LIGHTS_PER_EFFECT ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTFALLOFF_WORLD,	tPoints.pvFalloffsWorld,	MAX_SPECULAR_LIGHTS_PER_EFFECT ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTFALLOFF_OBJECT,	tPoints.pvFalloffsObject,	MAX_SPECULAR_LIGHTS_PER_EFFECT ) );
			V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_SPECULARLIGHTCOLOR,			tPoints.pvColors,			MAX_SPECULAR_LIGHTS_PER_EFFECT ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetLightingEffectParameters(
	const D3D_MODEL * pModel,
	const DRAWLIST_STATE * pState,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	float fEffectTransitionStr,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights,
	BOOL bForceCurrentEnv = FALSE )
{
	PERF( DRAW_SETLIGHTPRMS );

	V_RETURN( sSetSpecularEffectParameters(
		pModel,
		pState,
		pEffect,
		pEffectDef,
		tTechnique,
		pEnvDef,
		fEffectTransitionStr,
		tLights ) );

	V( dxC_AssembleLightsPoint( pModel, tLights ) );

	MESH_RENDER_LIGHT_DATA tMeshLights;
	V_RETURN( dxC_EffectGetScaledMeshLights(
		pEffect,
		pEffectDef,
		tTechnique,
		tLights,
		tMeshLights ) );

	V_RETURN( sSetPointLightEffectParameters(
		pModel,
		pState,
		pEffect,
		pEffectDef,
		tTechnique,
		fEffectTransitionStr,
		tLights,
		tMeshLights ) );

	V_RETURN( sSetSHEffectParameters(
		pModel,
		pEffect,
		pEffectDef,
		tTechnique,
		pEnvDef,
		tLights,
		tMeshLights,
		bForceCurrentEnv ) );

	const MATRIX * pmWorldToObject = &pModel->matScaledWorldInverse;

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SPOTLIGHTCOLOR ) )	// CHB 2007.07.16
	{
		VECTOR vCenter = ( pModel->tRenderAABBWorld.vMin + pModel->tRenderAABBWorld.vMax ) * 0.5f;

		int nLightID = e_LightGetClosestMatching( LIGHT_TYPE_SPOT, vCenter, LIGHT_FLAG_DEFINITION_APPLIED, LIGHT_FLAG_NODRAW );
		D3D_LIGHT * pLight = dx9_LightGet( nLightID );
		if ( pLight )
		{
			V_RETURN( dx9_EffectSetSpotlightParams( pEffect, tTechnique, pEffectDef, pmWorldToObject, pModel, pLight ) );
		}
	}

	// directional
	V_RETURN( dx9_EffectSetDirectionalLightingParams( pModel, pEffect, pEffectDef, pmWorldToObject, tTechnique, pEnvDef, 0, fEffectTransitionStr, bForceCurrentEnv ) );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static float sCalculateModelSelfIllum(
	const D3D_MODEL * pModel,
	int nLOD,	// CHB 2006.12.08
	int nMesh,
	const MATERIAL * pMaterial )
{
	float fSelfIllumPower = 1.0f;

	if ( pMaterial->dwFlags & MATERIAL_FLAG_VARY_SELFILLUMINATION )
	{
		// init if not already done, but don't reset if not
		V( e_ModelInitSelfIlluminationPhases( pModel->nId, FALSE ) );
		LightmapWaveData * ptLightmapData = pModel->tModelDefData[nLOD].ptLightmapData;	// CHB 2006.12.08
		if ( ptLightmapData )	// CHB 2006.12.08
		{
			ptLightmapData += nMesh;	// CHB 2006.12.08

			const float & fMax		 = pMaterial->fSelfIlluminationMax;
			const float & fMin		 = pMaterial->fSelfIlluminationMin;
			const float & fWaveAmp   = pMaterial->fSelfIlluminationWaveAmp;
			const float & fWaveHertz = pMaterial->fSelfIlluminationWaveHertz;
			const float & fNoiseAmp  = pMaterial->fSelfIlluminationNoiseAmp;
			const float & fNoiseFreq = pMaterial->fSelfIlluminationNoiseFreq;
			const float fElapsed	 = e_GetElapsedTime();
			const float fHalf = (fMax - fMin) * 0.5f;
			float & fWavePhase  = ptLightmapData->fWavePhase;	// CHB 2006.12.08

			fSelfIllumPower = 0.0f;

			BOOL bWave = ( fWaveAmp > 0.0f && fWaveHertz > 0.0f );
			if ( bWave )
			{
				// Perform a sine wave operation
				fWavePhase += fElapsed * fWaveHertz * TWOxPI;
				fWavePhase = FMODF( fWavePhase, TWOxPI );

				fSelfIllumPower += sinf( fWavePhase ) * fWaveAmp * fHalf + fMin + fHalf;
			}

			if ( fNoiseAmp > 0.0f )
			{
				// Perform a sine wave operation
				float fNoise = 0.0f;
				for ( int i = 0; i < SELF_ILLUM_NOISE_WAVES; i++ )
				{
					float & fNoisePhase = ptLightmapData->fNoisePhase[ i ];								// CHB 2006.12.08
					fNoisePhase += fElapsed * fNoiseFreq * ptLightmapData->fNoiseFreq[ i ] * TWOxPI;	// CHB 2006.12.08
					fNoisePhase = FMODF( fNoisePhase, TWOxPI );


					//float fCurNoise = sinf( fNoisePhase ) * fNoiseAmp;
					float fSine = gfSin360Tbl[ int( fNoisePhase * 180.0f / PI ) ];
					float fCurNoise = fSine * fNoiseAmp;
					fNoise += fCurNoise;
				}

				//fNoise = fNoise * fHalf + fMin + fHalf;
				if ( ! bWave )
					fNoise = fNoise * fHalf + fMin + fHalf;
				fSelfIllumPower += fNoise;
			}

			fSelfIllumPower = min( fSelfIllumPower, fMax );
			fSelfIllumPower = max( fSelfIllumPower, fMin );
		}
	}

	return fSelfIllumPower;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetMaterialParameters(
	D3D_MODEL * NOALIAS pModel,
	const D3D_MODEL_DEFINITION * const NOALIAS pModelDef,
	int nLOD,	// CHB 2006.12.08
	const D3D_MESH_DEFINITION * const NOALIAS pMesh,
	int nMesh,
	const D3D_EFFECT * const NOALIAS pEffect,
	const EFFECT_DEFINITION * const NOALIAS pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	MATERIAL * const NOALIAS pMaterial,
	const ENVIRONMENT_DEFINITION * const NOALIAS pEnvDef,
	float fEffectTransitionStr,
	BOOL bForceCurrentEnv )
{
	PERF( DRAW_SETMTRLPRMS );

	FXConst_MiscMaterial	 tConstMisc		= {0};
	FXConst_SpecularMaterial tConstSpec		= {0};
	FXConst_ScatterMaterial	 tConstScatter	= {0};

	// for the case of no specular map,  set an override value
	float fSpecOverride = 0.0f;
	//if (!dx9_ModelHasSpecularMap(pModel, pModelDef, pMesh, nMesh, pMaterial, nLOD))
	//	fSpecOverride = pMaterial->fSpecularMaskOverride * 0.5f;
	tConstMisc.SpecularMaskOverride = fSpecOverride;

	//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_SPECULARGLOW, (float) pMaterial->fSpecularGlow ) );
	tConstSpec.SpecularGlow = pMaterial->fSpecularGlow;
	//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_LIGHTMAPGLOW, (float) pMaterial->fLightmapGlow ) );
	tConstMisc.SelfIlluminationGlow = pMaterial->fLightmapGlow;

	float fSelfIllumPower = sCalculateModelSelfIllum( pModel, nLOD, nMesh, pMaterial );	// CHB 2006.12.08

	// lerp to model self-illum override
	fSelfIllumPower = LERP( fSelfIllumPower, pModel->fSelfIlluminationValue, pModel->fSelfIlluminationFactor );
	//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_LIGHTMAPPOWER, fSelfIllumPower ) );
	tConstMisc.SelfIlluminationPower = fSelfIllumPower;

	const float cfSpecularPowScale = SPECULAR_MAX_POWER;
	VECTOR2 vGlossiness ( pMaterial->fGlossiness[ 0 ],
						  pMaterial->fGlossiness[ 1 ] );

	float fGlossMin = e_GetMinGlossiness();
	if ( vGlossiness.x < fGlossMin )
		vGlossiness.x = fGlossMin;
	if ( vGlossiness.y < fGlossMin )
		vGlossiness.y = fGlossMin;
	vGlossiness *= cfSpecularPowScale;

	//V( dx9_EffectSetVector(	pEffect, tTechnique, EFFECT_PARAM_SPECULARGLOSSINESS, &vGlossiness ) );
	tConstSpec.SpecularGlossiness = vGlossiness;

	float fSpecularLevel = pMaterial->fSpecularLevel[ 0 ];
	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_LOD_TRANS_SPECULAR ) )
		fSpecularLevel *= fEffectTransitionStr;
	//V( dx9_EffectSetFloat(		pEffect, tTechnique, EFFECT_PARAM_SPECULARLEVEL, fSpecularLevel ) );
	tConstSpec.SpecularBrightness = fSpecularLevel;


	// subsurface scattering
	if ( pMaterial->dwFlags & MATERIAL_FLAG_SUBSURFACE_SCATTERING )
	{
		tConstMisc.SubsurfaceScatterSharpness	= pMaterial->fScatterSharpness;
		tConstScatter.fScatterIntensity			= pMaterial->fScatterIntensity * fEffectTransitionStr;
		tConstScatter.vScatterColor				= *(VECTOR3*)&pMaterial->tScatterColor.GetValue( 0.5f );
	}


	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_SPECULAR_LUT ) )
	{
		V( dxC_SetTexture(
			pMaterial->nSpecularLUTId,
			tTechnique,
			pEffect,
			EFFECT_PARAM_SPECULARLOOKUP,
			INVALID_ID ) );
	}

	// CHB 2006.08.21 - Only bother with all these environment mapping
	// settings if the effect accepts an environment texture parameter.
	BOOL bCubeMap   = dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_CUBE_ENVMAP );
	BOOL bSphereMap = dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SPHERE_ENVMAP );
	if ( bCubeMap || bSphereMap )
	{
		FXConst_EnvironmentMap tConstEnvMap = {0};

		float fEnvMapPower = pMaterial->fEnvMapPower * e_GetEnvMapFactor( pEnvDef, bForceCurrentEnv );
		int nEnvMapMIPLevels = -1;

		if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_LOD_TRANS_SPECULAR ) )
			fEnvMapPower *= fEffectTransitionStr;

		//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_ENVMAPPOWER, fEnvMapPower ) );
		tConstEnvMap.fEnvMapPower = fEnvMapPower;

		float fGlossThreshold		= pMaterial->fEnvMapGlossThreshold;
		float fInvGlossThresholdMul = 1.f;
		if ( pMaterial->dwFlags & MATERIAL_FLAG_ENVMAP_INVTHRESHOLD )
		{
			fInvGlossThresholdMul = -1.f;
			fGlossThreshold *= -1.f;
		}

		//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_ENVMAPGLOSSTHRESHOLD,		 fGlossThreshold ) );
		tConstEnvMap.fEnvMapGlossThreshold = fGlossThreshold;
		//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_ENVMAPINVERTGLOSSTHRESHOLD, fInvGlossThresholdMul ) );
		tConstEnvMap.fEnvMapInvertGlossThreshold = fInvGlossThresholdMul;

		LPD3DCBASESHADERRESOURCEVIEW pEnvMapTexture = NULL;
		if ( pMaterial->nSphereMapTextureID != INVALID_ID && e_TextureIsLoaded( pMaterial->nSphereMapTextureID, TRUE ) )
		{
			// set up the sphere map texture
			D3D_TEXTURE * pSMTexture = dxC_TextureGet( pMaterial->nSphereMapTextureID );
			ASSERT( pSMTexture );
			if ( pSMTexture )
				pEnvMapTexture = pSMTexture->GetShaderResourceView();
			if ( pEnvMapTexture )
			{
				V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SPHERE_ENVMAP, pEnvMapTexture ) );
				if ( pSMTexture->tEngineRes.IsTracked() )
					gtReaper.tTextures.Touch( &pSMTexture->tEngineRes );
			}
		}
		if ( ! pEnvMapTexture )
		{
			int nCubeMapID;
			V( dxC_MaterialGetCubeMap( pMaterial, nCubeMapID, nEnvMapMIPLevels ) );
			if ( nCubeMapID == INVALID_ID )
			{
				nCubeMapID = e_GetEnvMap( pEnvDef, bForceCurrentEnv );
				nEnvMapMIPLevels = e_GetEnvMapMIPLevels( pEnvDef, bForceCurrentEnv );
			}
			D3D_CUBETEXTURE * pCubeTexture = dx9_CubeTextureGet( nCubeMapID );
			if( pCubeTexture )
				pEnvMapTexture = pCubeTexture->GetShaderResourceView();//pCubeTexture ? pCubeTexture->pD3DShaderResourceView : NULL;
			if ( pEnvMapTexture )
			{
				V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_CUBE_ENVMAP, pEnvMapTexture ) );
			}
		}

		if ( nEnvMapMIPLevels < 0 )
			nEnvMapMIPLevels = 0;
		//V( dx9_EffectSetFloat(		pEffect, tTechnique, EFFECT_PARAM_ENVMAPLODBIAS,		pMaterial->fEnvMapBlurriness * nEnvMapMIPLevels ) );
		tConstEnvMap.fEnvMapLODBias = pMaterial->fEnvMapBlurriness * nEnvMapMIPLevels;

		V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_ENVMAP_DATA,		  (VECTOR4*)&tConstEnvMap ) );
	}

	V( dx9_EffectSetVector(	pEffect, tTechnique, EFFECT_PARAM_MISC_MATERIAL_DATA,     (VECTOR4*)&tConstMisc ) );
	V( dx9_EffectSetVector(	pEffect, tTechnique, EFFECT_PARAM_SPECULAR_MATERIAL_DATA, (VECTOR4*)&tConstSpec ) );
	V( dx9_EffectSetVector(	pEffect, tTechnique, EFFECT_PARAM_SCATTER_COLOR_POWER,	  (VECTOR4*)&tConstScatter ) );

	V_RETURN( dx9_EffectSetScrollingTextureParams( pModel, nLOD, nMesh, pModelDef->nMeshCount, pMaterial, pEffect, tTechnique ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetMeshTextures(
	D3D_MODEL * pModel,
	int nMesh,
	const D3D_MESH_DEFINITION * pMesh,
	const D3D_MODEL_DEFINITION * pModelDefinition,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const MATERIAL * pMaterial,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	int nLOD,
	BOOL bDepthOnly = FALSE )	// CHB 2006.11.28
{
	PERF( DRAW_SETTEXTURES );

	int nStage = 0;

	if ( bDepthOnly )
	{
		V( dxC_SetTextureConditional( 
			pModel, TEXTURE_DIFFUSE,	nLOD, tTechnique, pEffect, EFFECT_PARAM_DIFFUSEMAP0,	nStage++,	pModelDefinition, pMesh, nMesh, TRUE,  pMaterial ) );
		return S_OK;
	}

	BOOL bSetLightmap  = TRUE;
	BOOL bSetDiffuse   = TRUE;
	BOOL bSetSelfIllum = TRUE;

	// CML 2007.09.23 - If this is a flashlight level and this isn't an animated model, use flashlight level texture overrides.
	if ( pEnvDef->nLocation == LEVEL_LOCATION_FLASHLIGHT && ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
	{
		V( dxC_SetTexture(
			e_GetUtilityTexture( TEXTURE_FLASHLIGHTMAP ),
			tTechnique, pEffect, EFFECT_PARAM_LIGHTMAP, nStage++,
			pModelDefinition, pMaterial, TEXTURE_LIGHTMAP, FALSE ) );
		bSetLightmap = FALSE;

		if ( 0 == ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_FLASHLIGHT_EMISSIVE ) )
		{
			if ( pEffect->dwFlags & EFFECT_FLAG_EMISSIVEDIFFUSE )
			{
				V( dxC_SetTexture(
					e_GetUtilityTexture( TEXTURE_RGB_00_A_FF ),
					tTechnique, pEffect, EFFECT_PARAM_DIFFUSEMAP0, nStage++,
					pModelDefinition, pMaterial, TEXTURE_DIFFUSE, TRUE ) );
				bSetDiffuse = FALSE;
			}

			V( dxC_SetTexture(
				e_GetUtilityTexture( TEXTURE_RGB_00_A_FF ),
				tTechnique, pEffect, EFFECT_PARAM_SELFILLUMMAP, INVALID_ID,
				pModelDefinition, pMaterial, TEXTURE_SELFILLUM, FALSE ) );
			bSetSelfIllum = FALSE;
		}
	}

	if ( bSetLightmap )
	{
		V( dxC_SetTextureConditional(
			pModel,	TEXTURE_LIGHTMAP,	nLOD, tTechnique, pEffect, EFFECT_PARAM_LIGHTMAP,		nStage++,	pModelDefinition, pMesh, nMesh, FALSE, pMaterial ) );
	}

	if ( bSetDiffuse )
	{
		V( dxC_SetTextureConditional( 
			pModel, TEXTURE_DIFFUSE,	nLOD, tTechnique, pEffect, EFFECT_PARAM_DIFFUSEMAP0,	nStage++,	pModelDefinition, pMesh, nMesh, TRUE,  pMaterial ) );
	}

	V( dxC_SetTextureConditional(
		pModel, TEXTURE_DIFFUSE2,	nLOD, tTechnique, pEffect, EFFECT_PARAM_DIFFUSEMAP1,	nStage++,	pModelDefinition, pMesh, nMesh, TRUE,  pMaterial ) );
	V( dxC_SetTextureConditional(
		pModel, TEXTURE_NORMAL,		nLOD, tTechnique, pEffect, EFFECT_PARAM_NORMALMAP,		INVALID_ID, pModelDefinition, pMesh, nMesh, FALSE, pMaterial ) );
	V( dxC_SetTextureConditional(
		pModel, TEXTURE_SPECULAR,	nLOD, tTechnique, pEffect, EFFECT_PARAM_SPECULARMAP,	INVALID_ID,	pModelDefinition, pMesh, nMesh, FALSE, pMaterial ) );

	if ( bSetSelfIllum )
	{
		V( dxC_SetTextureConditional(
			pModel, TEXTURE_SELFILLUM,	nLOD, tTechnique, pEffect, EFFECT_PARAM_SELFILLUMMAP,	INVALID_ID,	pModelDefinition, pMesh, nMesh, FALSE, pMaterial ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetGeneralMeshParameters(
	const D3D_MODEL* pModel,
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	int nLOD,	// CHB 2006.11.30
	const DRAWLIST_STATE * pState,
	const MATRIX * pmatWorldCurrent,
	const MATRIX * pmatViewCurrent,
	const MATRIX * pmatProjCurrent,
	const MATRIX * pmatWorldViewProj,
	const VECTOR4 * pvEyeLocationInWorld,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const MATERIAL * pMaterial,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	BOOL bForceCurrentEnv,
	BOOL bAlphaAsLighting,
	float fModelAlpha,
	BOOL bDepthOnly = FALSE )
{
	PERF( DRAW_SETMESHPRMS );

	if ( dxC_IsPixomaticActive() )
	{
		dxC_PixoSetWorldMatrix( pmatWorldCurrent );
		dxC_PixoSetViewMatrix( pmatViewCurrent );
		dxC_PixoSetProjectionMatrix( pmatProjCurrent );

		return S_OK;
	}

	V( dx9_EffectSetMatrix ( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, pmatWorldViewProj ) );
#ifdef ENGINE_TARGET_DX10
	V( dx9_EffectSetMatrix ( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX_PREV, &pModel->matWorldViewProjPrev ) );
#endif
	V( dx9_EffectSetMatrix ( pEffect, tTechnique, EFFECT_PARAM_WORLDMATRIX, pmatWorldCurrent ) );
	V( dx9_EffectSetMatrix ( pEffect, tTechnique, EFFECT_PARAM_VIEWMATRIX, pmatViewCurrent ) );

	V( dxC_SetBones ( pModel, nMesh, nLOD, pEffect, tTechnique ) );

	FXConst_MiscLighting tConstLighting = {0};

	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA ) )
	{
		tConstLighting.fAlphaScale = fModelAlpha;
		bAlphaAsLighting = FALSE;
	} else
	{
		tConstLighting.fAlphaScale = 1.0f;
	}

	if ( bAlphaAsLighting )
	{
		// we're using alpha as glow (result of spec and self-illum lighting)
		//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_ALPHAFORLIGHTING, 1.0f ) );
		tConstLighting.fAlphaForLighting = 1.f;
	} else
	{
		// we're using alpha as transparency
		//V( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_ALPHAFORLIGHTING, 0.0f ) );
		tConstLighting.fAlphaForLighting = 0.f;
	}


	// In one-bit alpha mode, force the alpha to write opacity
	if ( e_GetRenderFlag( RENDER_FLAG_ONE_BIT_ALPHA ) )
		tConstLighting.fAlphaForLighting = 0.f;


	if ( pEnvDef )
	{
		float fShadowIntensity = e_GetShadowIntensity( pEnvDef, bForceCurrentEnv );
		fShadowIntensity = min( fShadowIntensity, 1.f );
		fShadowIntensity = max( fShadowIntensity, 0.f );
		//V_RETURN( dx9_EffectSetFloat( pEffect, tTechnique, EFFECT_PARAM_SHADOW_INTENSITY, fShadowIntensity ) );
		tConstLighting.fShadowIntensity = fShadowIntensity;
	}

	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_MISC_LIGHTING_DATA, (VECTOR4*)&tConstLighting ) );

	// If this is a depth-only pass, we're done here.
	if ( bDepthOnly )
		return S_OK;

	
	
	float fFogValue = ( pModel->vScale.fX + pModel->vScale.fY )/2.0f;	
	V( dx9_EffectSetFogParameters( pEffect, tTechnique, fFogValue ) );
	//V( dx9_EffectSetFogParameters( pEffect, tTechnique, ( pModel->vScale.fX + pModel->vScale.fY + pModel->vScale.fZ ) / 3.0f) );
	// CHB 2007.02.17
	V(dx9_SetParticleLightingMapParameters(pEffect, tTechnique, reinterpret_cast<const MATRIX *>(pmatWorldCurrent)));


	int nLocation = e_GetCurrentLevelLocation();
	if ( e_UseShadowMaps() && e_ShadowsActive() && ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NOSHADOW ) )
	{
		int nUseShadowMap = -1;

		if ( AppIsTugboat() 
		  || nLocation == LEVEL_LOCATION_INDOOR 
		  || nLocation == LEVEL_LOCATION_INDOORGRID )
			nUseShadowMap = 0;
		else
		{
			// Default to the lowest detail SM because it covers the full region.
			nUseShadowMap = dx9_ShadowBufferGetFirstID();

			for ( int nShadowID = dx9_ShadowBufferGetNextID( nUseShadowMap );
				nShadowID != INVALID_ID;
				nShadowID = dx9_ShadowBufferGetNextID( nShadowID ) )
			{
				D3D_SHADOW_BUFFER* pShadow = dx9_ShadowBufferGet( nShadowID );

				if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_EXTRA_OUTDOOR_PASS ) )
					continue;

				float fRad = pShadow->fScale * 0.5f;
				float fDist = VectorDistanceXY( pShadow->vWorldFocus, pModel->vPosition );

				if ( fDist <= fRad )
				{
					VECTOR* pPoints = dx9_GetMeshRenderOBBInWorld( pModel, nLOD, nMesh );

					if ( !pPoints )
						pPoints = e_GetModelRenderOBBInWorld( pModel->nId );							

					// we exclude the top and bottom planes because we only need to test xy containment.
					if ( e_BoxPointsVsPlaneDotProduct( pPoints, OBB_POINTS, &pShadow->tOBBPlanes[ 2 ] ) < 0
					  && e_BoxPointsVsPlaneDotProduct( pPoints, OBB_POINTS, &pShadow->tOBBPlanes[ 3 ] ) < 0
					  && e_BoxPointsVsPlaneDotProduct( pPoints, OBB_POINTS, &pShadow->tOBBPlanes[ 4 ] ) < 0
					  && e_BoxPointsVsPlaneDotProduct( pPoints, OBB_POINTS, &pShadow->tOBBPlanes[ 5 ] ) < 0 )
					{
						nUseShadowMap = nShadowID;

						if ( c_GetToolMode() && e_GetRenderFlag( RENDER_FLAG_SHADOWS_SHOWAREA ) )
							e_PrimitiveAddBox( 0, pPoints, pShadow->dwDebugColor );
					}
				}
			}
		}		

		int nForceShadowMap = e_GetRenderFlag( RENDER_FLAG_SHADOWS_FORCE_MAP );
		if ( nForceShadowMap != INVALID_ID )
			nUseShadowMap = nForceShadowMap;
		
		if ( nUseShadowMap >= 0 )
		{
			V( dx9_SetShadowMapParameters( pEffect, tTechnique, 
				nUseShadowMap, pEnvDef, (MATRIX*)pmatWorldCurrent, 
				(MATRIX*)pmatViewCurrent, (MATRIX*)pmatProjCurrent ) );
		}
	}

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_EYEINOBJECT ) )
	{
		D3DXVECTOR4 vEyeLocationInObject(0.f, 0.f, 0.f, 0.f);
		MatrixMultiply( (VECTOR *)&vEyeLocationInObject, (VECTOR*)pState->pvEyeLocation, (MATRIX *)&pModel->matScaledWorldInverse );
		V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyeLocationInObject) );
	}
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_EYEINWORLD, pvEyeLocationInWorld ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetGeneralMeshStates(
	D3D_MODEL* pModel,
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	int nLOD,
	const DRAWLIST_STATE * pState,
	const D3D_EFFECT * pEffect,
	const EFFECT_DEFINITION * pEffectDef,
	const EFFECT_TECHNIQUE & tTechnique,
	const MATERIAL * pMaterial,
	BOOL & bAlphaAsLighting,
	BOOL bDepthOnly = FALSE )
{
	PERF( DRAW_SETMESHSTATES );

	// change cull mode if necessary
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMesh->dwFlags & MESH_FLAG_TWO_SIDED || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
		 dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
	{
		V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
	} else {
		V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
	}

#if ISVERSION(DEVELOPMENT)
	// change wireframe mode if necessary
	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) && ! bDepthOnly )
	{
		V( dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) );
	} else {
		V( dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
	}
#endif

	bAlphaAsLighting = TRUE;
	BOOL bForceNoZWrite = FALSE;

	DWORD dwFullColorWrite = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;

	// Don't ever write alpha during zpass.
	if ( bDepthOnly )
		dwFullColorWrite |= ~(D3DCOLORWRITEENABLE_ALPHA);

	// change alpha test, if necessary
	if ( dxC_MeshDiffuseHasAlpha( pModel, pModelDef, pMesh, nMesh, nLOD )
	  || pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) 
	  || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA ) )
	{
		// set alpha test, if specified for both effect and mesh
		// if alphablend is allowed for this effect and the material requests blending
		if ( ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_BLEND ) 
			   && ( MATERIAL_FLAG_ALPHABLEND & pMaterial->dwFlags ) ) 
			|| dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA )
			|| TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS )
			&& ! bDepthOnly )
		{
			V( dxC_SetAlphaBlendEnable( TRUE ) );
			V( dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA) );
			V( dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA) );
			V( dxC_SetRenderState(D3DRS_BLENDOP,			D3DBLENDOP_ADD) );
			// don't write alpha -- interferes with glow

			if ( e_GetRenderFlag( RENDER_FLAG_ONE_BIT_ALPHA ) )
			{
				V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, dwFullColorWrite ) );
			}
			else
			{
				V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
			}

			V( dxC_SetAlphaTestEnable( TRUE ) );
			bAlphaAsLighting = FALSE;
			if ( MATERIAL_FLAG_ALPHA_NO_WRITE_DEPTH & pMaterial->dwFlags )
			{
				// don't write depth, either
				bForceNoZWrite = TRUE;
			}
		} else {
			V( dxC_SetAlphaBlendEnable( FALSE ) );
			V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, dwFullColorWrite ) );

			// if alphatest is allowed for this effect
			if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_TEST ) )
			{
				V( dxC_SetAlphaTestEnable( TRUE ) );
			} else {
				V( dxC_SetAlphaTestEnable( FALSE ) );
			}
		}
	} else
	{
		V( dxC_SetAlphaBlendEnable( FALSE ) );
		V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, dwFullColorWrite ) );
		V( dxC_SetAlphaTestEnable( FALSE ) );
	}


	if ( pMaterial->dwFlags & MATERIAL_FLAG_ALPHAADD )
	{
		V( dxC_SetAlphaBlendEnable( TRUE ) );
		V( dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_ONE) );
		V( dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_ONE) );
		V( dxC_SetRenderState(D3DRS_BLENDOP,			D3DBLENDOP_ADD) );
	}

	// restore depth settings (can be changed by effects)
	if( !e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) )
		{ V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, ( ! bForceNoZWrite && e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) && e_GetRenderFlag( RENDER_FLAG_USE_ZWRITE ) ) ? TRUE : FALSE ) ); }
	V( dxC_SetRenderState( D3DRS_ZENABLE, ( ! pState->bNoDepthTest && e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ) ? D3DZB_TRUE : D3DZB_FALSE ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGetEffectAndTechnique(
	D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef,		// CHB 2007.09.25
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	const DRAWLIST_STATE * pState,
	const MATERIAL * pMaterial,
	DWORD dwMeshFlagsDoDraw,
	MODEL_RENDER_LIGHT_DATA & tLights,
	float fDistanceToEyeSquared,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	float & fTransitionStr,
	int & nShaderType,
	D3D_EFFECT ** ppEffect,
	EFFECT_TECHNIQUE ** pptTechnique,
	int nLOD )	// CHB 2006.11.28
{
	ASSERT_RETINVALIDARG( ppEffect );
	nShaderType = dx9_GetCurrentShaderType( pEnvDef, pState, pModel );
	*ppEffect = NULL;

	// figure out how many point lights to ask for
	EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( pMaterial->tHeader.nId, nShaderType );
	ASSERT_RETFAIL( pEffectDef );
	int nPointLights;
	if ( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS ) )
	{
		V( dxC_AssembleLightsGlobalPoint( pModel, tLights ) );	
		nPointLights	= tLights.GlobalPoint.nLights;		// global slot lights
		// global slot specular point lights
		V( dxC_AssembleLightsGlobalSpec( pModel, pEnvDef, tLights ) );
	}
	else
	{
		V( dxC_AssembleLightsPoint( pModel, tLights ) );
		nPointLights	= tLights.ModelPoint.nLights;	// per-model selected lights
		//keytrace( "nPointLights = %d\n", tLights.nModelPointLights );
	}

	//keytrace( "### MODEL %3d, effect point lights %d\n", pModel->nId, nPointLights );

	DWORD dwFinalFlags = 0;
	V_RETURN( dx9_GetEffectAndTechnique( pMaterial, pModel, pModelDef, pMesh, nMesh, dwMeshFlagsDoDraw, nPointLights, tLights.ModelSpec.nLights, fDistanceToEyeSquared, pEffectDef, ppEffect, pptTechnique, fTransitionStr, nShaderType, pEnvDef, &dwFinalFlags, nLOD ) );	// CHB 2006.11.28
	if ( ! *ppEffect )
		return S_FALSE;

	if ( dxC_IsPixomaticActive() )
		return S_OK;

	ASSERTX_RETFAIL( *pptTechnique && (*pptTechnique)->hHandle, pEffectDef->pszFXFileName );

	// Apply the LOD transition value
	fTransitionStr *= pModel->fLODTransitionLERP;

	EFFECT_TECHNIQUE_REF tRef;
	if ( pMesh->ptTechniqueCache )
		tRef = pMesh->ptTechniqueCache->tRefs[0];
	else
	{
		tRef.Reset();
		tRef.nEffectID = pEffectDef->nD3DEffectId;
	}

	dx9_DebugTrackMeshRenderInfo(
		pModel->nId,
		nMesh,
		pMaterial->tHeader.nId,
		tRef,
		fDistanceToEyeSquared );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetMeshVertexParameters(
	const D3D_MODEL_DEFINITION* pModelDefinition,
	const D3D_MESH_DEFINITION* pMesh,
	const D3D_EFFECT* pEffect,
	const EFFECT_DEFINITION* pEffectDef,
	D3D_VERTEX_BUFFER*& pVertexBuffer )
{
	// set the vertex buffer
	ASSERT_RETINVALIDARG( pMesh->nVertexBufferIndex < pModelDefinition->nVertexBufferDefCount );
	ASSERT_RETINVALIDARG( pMesh->nVertexBufferIndex >= 0  );

	if ( ! dxC_IsPixomaticActive() )
	{
		D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];
		int nStreamCount = dxC_GetStreamCount( pVertexBufferDef );
		for ( int nStream = 0; nStream < nStreamCount; nStream++ )
		{
			pVertexBuffer = dxC_VertexBufferGet( pVertexBufferDef->nD3DBufferID[ nStream ] );
			ASSERT_RETFAIL( pVertexBuffer );
			if ( !pVertexBuffer->pD3DVertices )
				return S_FALSE;
		}
	}

	V_RETURN( dxC_SetStreamSource( pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ], 0 ) );	
	V_RETURN( dxC_SetIndices( pMesh->tIndexBufferDef ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMeshFrustumCull(
	const D3D_MODEL * pModel,
	int nLOD,
	int nMesh,
	const PLANE * tPlanes )
{
	if ( e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MESHES ) )
	{
		if ( nLOD == LOD_NONE || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
			return FALSE;

		// frustum cull mesh
		
		VECTOR * ptMeshRenderOBB = dx9_GetMeshRenderOBBInWorld( pModel, nLOD, nMesh );
		if ( ptMeshRenderOBB && ! e_OBBInFrustum( ptMeshRenderOBB, tPlanes ) )
			return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMeshCullRender(
	D3D_MODEL * pModel,
	const D3D_MESH_DEFINITION * pMesh,
	int nMesh,
	DWORD dwMeshFlags,
	const DRAWLIST_STATE * pState,
	const PLANE * tPlanes,
	DWORD dwMeshFlagsDoDraw,
	DWORD dwMeshFlagsDontDraw,
	int nLOD = LOD_NONE )
{
	//if ( ( bTestVisibility || AppIsTugboat() ) && !dx9_MeshIsVisible( pModel, pMesh ) )
	if ( ! dx9_MeshIsVisible( pModel, pMesh ) )
		return TRUE;

	if ( pModel && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA ) )
		dwMeshFlags |= MESH_FLAG_ALPHABLEND;

	// TRAVIS - nope, Tug still counts on partial matches.
	if( AppIsTugboat() )
	{
		if ( dwMeshFlagsDoDraw   && ( dwMeshFlags & dwMeshFlagsDoDraw   ) == 0)
			return TRUE;		
	}

	// CML 2006.10.3: This code change makes it so all requested mesh flags must be present, instead of at least one.  I think Tugboat wants this fix, too.
	if( AppIsHellgate() )
	{
		if ( dwMeshFlagsDoDraw   && ( dwMeshFlags & dwMeshFlagsDoDraw   ) != dwMeshFlagsDoDraw )
			return TRUE;		
	}
	if ( dwMeshFlagsDontDraw && ( dwMeshFlags & dwMeshFlagsDontDraw ) != 0 )
		return TRUE;
			
	if ( tPlanes )
	{
		//PERF( FRUSTUM_CULL )
		if ( sMeshFrustumCull( pModel, nLOD, nMesh, tPlanes ) )
			return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sUpdateMeshDrawOptions(
	DWORD & dwMeshFlagsDoDraw,
	DWORD & dwMeshFlagsDontDraw,
	BOOL & bLights,
	float fDistanceToEyeSquared,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	BOOL bForceCurrentEnv )
{
	float fSilhouetteDistanceSquared = e_GetSilhouetteDistance( pEnvDef, bForceCurrentEnv );
	fSilhouetteDistanceSquared *= fSilhouetteDistanceSquared;
	int nDrawCollisionMode = e_GetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW );
	if ( e_GetRenderFlag( RENDER_FLAG_SILHOUETTE )				&&
		( dwMeshFlagsDoDraw & MESH_FLAG_SILHOUETTE ) != 0		&& 
		fDistanceToEyeSquared > fSilhouetteDistanceSquared		&&
		! e_GetRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW )		&&
		! e_GetRenderFlag( RENDER_FLAG_HULL_MESH_DRAW )			&&
		nDrawCollisionMode != RF_COLLISION_DRAW_ONLY )
	{
		dwMeshFlagsDoDraw = MESH_FLAG_SILHOUETTE | MESH_FLAG_COLLISION; // these two share the same mesh
		bLights = FALSE;
	} else if ( nDrawCollisionMode != RF_COLLISION_DRAW_NONE ) {
		dwMeshFlagsDoDraw &= ~MESH_FLAG_SILHOUETTE;
		dwMeshFlagsDontDraw &= ~MESH_FLAG_SILHOUETTE; // collision and silhouette share flags so don't exclude silhouette in this case
	} else {
		dwMeshFlagsDoDraw &= ~MESH_FLAG_SILHOUETTE;
		dwMeshFlagsDontDraw |= MESH_FLAG_SILHOUETTE;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGetEnvironment(
	const DRAWLIST_STATE * pState,
	const D3D_MODEL* pModel,
	ENVIRONMENT_DEFINITION *& pEnvDef,
	BOOL & bForceCurrentEnv )
{
	if ( pState->nOverrideEnvDefID != INVALID_ID )
	{
		pEnvDef = (ENVIRONMENT_DEFINITION *)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pState->nOverrideEnvDefID );
		ASSERT_RETFAIL( pEnvDef );
		bForceCurrentEnv = TRUE;	// don't allow transitions for overridden env renders
	} else {
		if ( pModel && pModel->nEnvironmentDefOverride != INVALID_ID )
		{
			pEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pModel->nEnvironmentDefOverride );
			ASSERT_RETFAIL( pEnvDef );
		}
		else
		{
			pEnvDef = e_GetCurrentEnvironmentDef();
			ASSERT_RETFAIL( pEnvDef );
		}
	}

	e_EnvironmentApplyCoefs( pEnvDef );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sGetMatrices(
	D3D_MODEL * pModel,
	const DRAWLIST_STATE * pState,
	MATRIX *& pmatWorldCurrent,
	MATRIX *& pmatViewCurrent,
	MATRIX *& pmatProjCurrent,
	MATRIX * pmatWorldView,
	MATRIX * pmatWorldViewProjection,
	float * pfDistanceToEyeSquared,
	VECTOR4 & vEyeLocationInWorld )
{
	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	// Setup the world, view, and projection matrices
	pmatWorldCurrent = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_WORLD ) ? &pModel->matScaledWorld : &mIdentity;
	pmatViewCurrent  = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_VIEW )  ? &pModel->matView		: pState->pmatView;
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ ) )
		pmatProjCurrent = &pModel->matProj;
	else if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
	{
		pmatProjCurrent = pState->pmatProj2;
		//pModel->dwFlags &= ~MODEL_FLAG_FIRST_PERSON_PROJ; // this gets reset every frame
	} else
		pmatProjCurrent = pState->pmatProj;

	ASSERT_RETINVALIDARG( pmatWorldView );
	ASSERT_RETINVALIDARG( pmatWorldViewProjection );
	ASSERT_RETINVALIDARG( pmatWorldCurrent );
	ASSERT_RETINVALIDARG( pmatViewCurrent );
	ASSERT_RETINVALIDARG( pmatProjCurrent );

	MatrixMultiply ( pmatWorldView,				pmatWorldCurrent,	pmatViewCurrent );
	MatrixMultiply ( pmatWorldViewProjection,	pmatWorldView,		pmatProjCurrent );

#ifdef ENGINE_TARGET_DX10
	if (  pModel->dwWorldViewProjPrevToken != e_GetVisibilityToken() )
	{
		// Save off this frame's matrix for use in the next frame.
		pModel->matWorldViewProjPrev = pModel->matWorldViewProjCurr;
		pModel->matWorldViewProjCurr = *pmatWorldViewProjection;
		pModel->dwWorldViewProjPrevToken = e_GetVisibilityToken();
	}
#endif

	if ( pfDistanceToEyeSquared )
	{
		BOUNDING_BOX tBBox;
		V( dxC_GetModelRenderAABBInWorld( pModel, &tBBox ) );
		*pfDistanceToEyeSquared = BoundingBoxDistanceSquared( &tBBox, (VECTOR *)pState->pvEyeLocation );
		//fDistanceToEyeSquared *= pModel->fScale * pModel->fScale;	// not correct to scale, box and pos already in world
	}

	vEyeLocationInWorld = D3DXVECTOR4( pState->pvEyeLocation->fX, pState->pvEyeLocation->fY, pState->pvEyeLocation->fZ, 0.0f );

	return S_OK;
}




//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RenderBoundingBox(
	BOUNDING_BOX * pBBox,
	MATRIX * pmatWorld,
	MATRIX * pmatView,
	MATRIX * pmatProj,
	BOOL bWriteColorAndDepth,
	BOOL bTestDepth,
	DWORD dwColor )
{
	D3D_PROFILE_REGION( L"Bounding Box" );

	ASSERT_RETINVALIDARG( pBBox );
	ASSERT_RETINVALIDARG( pmatWorld && pmatView && pmatProj );

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SIMPLE ) );

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );

	MATRIX matTrans;
	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	// update offset and scale matrix to match bounding box
	VECTOR vScale( pBBox->vMax.fX - pBBox->vMin.fX, pBBox->vMax.fY - pBBox->vMin.fY, pBBox->vMax.fZ - pBBox->vMin.fZ );
	MatrixTranslationScale( &matTrans, &pBBox->vMin, &vScale );
	//MATRIX matScale, matTranslate;
	//MatrixScale( &matScale, pBBox->vMax.fX - pBBox->vMin.fX, pBBox->vMax.fY - pBBox->vMin.fY, pBBox->vMax.fZ - pBBox->vMin.fZ );
	//MatrixTranslation( &matTranslate, pBBox->vMin.fX, pBBox->vMin.fY, pBBox->vMin.fZ );
	//MatrixMultiply(  (D3DXMATRIXA16*)&matTrans, &matScale, &matTranslate );
	MatrixMultiply( &matTrans, &matTrans, pmatWorld );

	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_WORLDMAT, &matTrans ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_VIEWMAT,  pmatView ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_PROJMAT,  pmatProj ) );

	ASSERT_RETFAIL( nPassCount == 1 ); // only one pass

	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	// set the global constant color
	//DX9_BLOCK( V( dxC_SetRenderState( D3DRS_TEXTUREFACTOR, dwColor ) ); );
	//DX10_BLOCK( ASSERTX( 0, "KMNV TODO: Get rid of fixed function" ) );
	VECTOR4 vColor = VECTOR_MAKE_FROM_ARGB( dwColor );
	V( dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_FOGCOLOR, (D3DXVECTOR4*)&vColor ) );
	V( e_SetFogEnabled( FALSE ) );

	if ( ! bWriteColorAndDepth )
	{
		V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, 0 ) );
		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE ) );
	} else {

		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );
		V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
		if ( e_GetRenderFlag( RENDER_FLAG_RENDER_BOUNDING_BOX ) )
		{
			V( dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) );
		}
	}
	if ( ! bTestDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE ) );
	} else
	{
		V( dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE ) );
	}
	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );

	V_RETURN( dx9_SetFVF( BOUNDING_BOX_FVF ) ); 
	V_RETURN( dxC_SetIndices( gtBoundingBoxIndexBuffer ) );
	V_RETURN( dxC_SetStreamSource( gtBoundingBoxVertexBuffer, 0 ) );

	V_RETURN( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, NUM_BOUNDING_BOX_LIST_VERTICES, 0, DP_GETPRIMCOUNT_TRILIST( NUM_BOUNDING_BOX_LIST_INDICES ), pEffect, METRICS_GROUP_DEBUG ) );

	V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
	if ( ! bWriteColorAndDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );
	}
	if ( ! bTestDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE ) );
	}
	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );

	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) == FALSE )
	{
		V( dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
	}

	V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RenderDebugBoxes()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DEBUG_CLIP_DRAW ) )
		return S_FALSE;

	for ( int nID = gtRender2DAABBs.GetFirst();
		nID != INVALID_ID;
		nID = gtRender2DAABBs.GetNextId( nID ) )
	{
		RENDER_2D_AABB * pAABB = gtRender2DAABBs.Get( nID );
		V_RETURN( dx9_Render2DAABB(
			&pAABB->tBBox,
			TRUE,
			FALSE,
			pAABB->dwColor ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_Render2DAABB(
	const BOUNDING_BOX * pBBox,
	BOOL bWriteColorAndDepth,
	BOOL bTestDepth,
	DWORD dwColor )
{
	D3D_PROFILE_REGION( L"2D AABB" );

	ASSERT_RETINVALIDARG( pBBox );

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SIMPLE ) );
	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );

	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_WORLDMAT, &mIdentity ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_VIEWMAT,  &mIdentity ) );
	V( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_DEBUG_PROJMAT,  &mIdentity ) );

	ASSERT_RETFAIL( nPassCount == 1 ); // only one pass

	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	VECTOR4 vColor = VECTOR_MAKE_FROM_ARGB( dwColor );
	float fStrength = max( pBBox->vMin.z, pBBox->vMax.z );
	fStrength = fStrength / e_GetFarClipPlane();
	fStrength = max( 0.0f, fStrength );
	fStrength = min( 1.0f, fStrength );
	fStrength = 1.0f - fStrength;

	vColor.x *= fStrength;
	vColor.y *= fStrength;
	vColor.z *= fStrength;
	//dwColor = ARGB_MAKE_FROM_FLOAT(vColor.x, vColor.y, vColor.z, vColor.w);

	V( dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_FOGCOLOR, (D3DXVECTOR4*)&vColor ) );
	//DX9_BLOCK( V( dxC_SetRenderState( D3DRS_TEXTUREFACTOR, dwColor ) ); )
	//DX10_BLOCK( ASSERTX( 0, "KMNV TODO: Get rid of fixed function" ) );
	V( e_SetFogEnabled( FALSE ) );

	if ( ! bWriteColorAndDepth )
	{
		V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, 0 ) );
		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE ) );
	} else {

		V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
	}
	if ( ! bTestDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE ) );
	}
	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );

	dxC_SetVertexDeclaration( VERTEX_DECL_SIMPLE, pEffect );

	//V_RETURN( dx9_SetFVF( BOUNDING_BOX_FVF ) ); 
	//V_RETURN( dxC_SetIndices( NULL ) );
	//V_RETURN( dxC_SetStreamSource( 0, gpBoundingBoxVertexBuffer, 0, sizeof(BOUNDING_BOX_VERTEX) ) );

	//WORD pInds[ 8 ] =
	//{
	//	0,1,1,2,2,3,3,0
	//};
	BOUNDING_BOX_VERTEX pVerts[ 4 ];

	V_RETURN( dxC_SetIndices( gt2DBoundingBoxIndexBuffer ) );

	float fDepth = e_GetNearClipPlane() + 0.001f;
	pVerts[ 0 ].vPosition = VECTOR( pBBox->vMin.x, pBBox->vMin.y, fDepth );
	pVerts[ 1 ].vPosition = VECTOR( pBBox->vMax.x, pBBox->vMin.y, fDepth );
	pVerts[ 2 ].vPosition = VECTOR( pBBox->vMax.x, pBBox->vMax.y, fDepth );
	pVerts[ 3 ].vPosition = VECTOR( pBBox->vMin.x, pBBox->vMax.y, fDepth );

	dxC_UpdateBuffer( 0, gt2DBoundingBoxVertexBuffer, pVerts, 0, gtBoundingBoxVertexBuffer.nBufferSize[ 0 ] );
	V_RETURN( dxC_SetStreamSource( gt2DBoundingBoxVertexBuffer , 0 ) );
	V_RETURN( dxC_DrawIndexedPrimitive( D3DPT_LINELIST, 0, 0, 4, 0, 4, pEffect, METRICS_GROUP_DEBUG ) );

	//V_RETURN( dxC_DrawIndexedPrimitiveUP( D3DPT_LINELIST, 0, 4, 4, pInds, D3DFMT_INDEX16, pVerts, sizeof( BOUNDING_BOX_VERTEX ), pEffect->pD3DEffect, METRICS_GROUP_DEBUG ) );

	V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
	if ( ! bWriteColorAndDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );
	}
	if ( ! bTestDepth )
	{
		V( dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE ) );
	}
	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );

	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) == FALSE )
	{
		V( dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
	}

	V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_PrepareUIModelRenders()
{
	DX9_BLOCK( if ( dx9_DeviceLost() ) return S_FALSE; )
	// clear zbuffer
	V_RETURN( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_ZBUFFER ) );
	// set model sampler states back
	V_RETURN( dxC_SetSamplerDefaults( SAMPLER_DIFFUSE ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_UIModelRender(
	int nModelID,
	UI_RECT * ptScreenCoords,
	UI_RECT * ptOrigCoords,
	float fRotationOffset,
	const VECTOR & vInventoryCamFocus,
	float fInventoryCamRotation,
	float fInventoryCamPitch,
	float fInventoryCamFOV,
	float fInventoryCamDistance,
	const char * pszInventoryEnvName )
{
	DX9_BLOCK( if ( dx9_DeviceLost() ) return S_FALSE; )

	HITCOUNT( DRAW_UIMODEL )

	ASSERT_RETFAIL( gpfn_AttachmentGetAttachedOfType );

	ASSERT_RETINVALIDARG( ptScreenCoords );
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG( pModel );
	ASSERT_RETFAIL( DRAW_LAYER_UI == dxC_ModelGetDrawLayer( pModel ) );

	dxC_ModelSelectLOD( pModel, -1.f, -1.f );

	fRotationOffset = FMODF( fRotationOffset, TWOxPI );

	D3DXVECTOR3 vUpVec( 0.0f, 0.0f, 1.0f );
	D3DXVECTOR3 vLookatPt( (const float *)&vInventoryCamFocus );
	D3DXVECTOR3 vEyePt;
	CameraMakeFlyEye( 
		(VECTOR*)&vEyePt,
		(VECTOR*)&vLookatPt, 
		fInventoryCamRotation, 
		fInventoryCamPitch, 
		fInventoryCamDistance );


	VECTOR * pvOBB = dxC_GetModelRenderOBBInWorld( pModel, NULL );
	VECTOR vCenter(0,0,0);
	for ( int i = 0; i < OBB_POINTS; i++ )
		vCenter.fX += pvOBB[ i ].fX;
	vCenter.fX *= 1.0f / (float)OBB_POINTS;
	for ( int i = 0; i < OBB_POINTS; i++ )
		vCenter.fY += pvOBB[ i ].fY;
	vCenter.fY *= 1.0f / (float)OBB_POINTS;

	// for now, only adjust in the greatest dimension
	//   -- some weapons are slightly off-balance (think "rifle grip"), making the rotation wobbly
	if ( fabsf( vCenter.fX ) > fabsf( vCenter.fY ) )
		vCenter.fY = 0.0f;
	else
		vCenter.fX = 0.0f;

	MATRIX matXlate;
	MATRIX matRotateObj;
	MATRIX matTemp;
	vCenter *= -1.0f;
	MatrixTranslation( &matXlate,     &vCenter   );
	MatrixTransform  ( &matRotateObj, NULL,      fRotationOffset );
	MatrixMultiply   ( &matTemp,      &matXlate, &matRotateObj );
	vCenter *= -1.0f;
	MatrixTranslation( &matXlate,     &vCenter   );
	MatrixMultiply   ( &matRotateObj, &matTemp,  &matXlate );

	VECTOR vPos( 0, 0, 0 );
	V( e_ModelSetLocationPrivate( nModelID, &matRotateObj, vPos, MODEL_DYNAMIC ) );


	D3DXMATRIXA16 matView, matProj;
	// set up view matrix as requested in app def
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );

	float fNear = 0.1f;
	float fFar = fInventoryCamDistance * 5.0f;


	RECT tOrig, tScreen;
	SetRect( &tOrig,   (int)ptOrigCoords  ->m_fX1, (int)ptOrigCoords  ->m_fY1, (int)ptOrigCoords  ->m_fX2, (int)ptOrigCoords  ->m_fY2 );
	SetRect( &tScreen, (int)ptScreenCoords->m_fX1, (int)ptScreenCoords->m_fY1, (int)ptScreenCoords->m_fX2, (int)ptScreenCoords->m_fY2 );
	MatrixMakeOffCenterPerspectiveProj( *(MATRIX*)&matProj, &tOrig, &tScreen, fInventoryCamFOV, fNear, fFar );


	MatrixMakeWFriendlyProjection( (MATRIX*)&matProj );	

	//e_ModelSetView( nModelID, &matView );
	//e_ModelSetProj( nModelID, &matProj );

	ENVIRONMENT_DEFINITION * pAppEnv = e_GetCurrentEnvironmentDef();
	if ( ! pAppEnv )
		return S_FALSE;
	ASSERT_RETFAIL( pAppEnv );
	if ( pszInventoryEnvName[ 0 ] )
	{
		int nEnvID = e_EnvironmentLoad( pszInventoryEnvName );
		if ( nEnvID != INVALID_ID )
			pAppEnv = (ENVIRONMENT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvID );
	}
	if ( ! pAppEnv )
		return S_FALSE;

	// set up viewport
	BOUNDING_BOX tViewport;
	tViewport.vMin = VECTOR( ptScreenCoords->m_fX1, ptScreenCoords->m_fY1, 0.0f );
	tViewport.vMax = VECTOR( ptScreenCoords->m_fX2, ptScreenCoords->m_fY2, 1.0f );

	int nCurWidth, nCurHeight;
	dxC_GetRenderTargetDimensions( nCurWidth, nCurHeight );
	ASSERT_RETFAIL( tViewport.vMin.fX >= 0 );
	ASSERT_RETFAIL( tViewport.vMin.fY >= 0 );
	ASSERT_RETFAIL( tViewport.vMax.fX <= nCurWidth );
	ASSERT_RETFAIL( tViewport.vMax.fY <= nCurHeight );

	BOOL bSavedCheapMode = e_InCheapMode();
	if ( bSavedCheapMode )
		e_EndCheapMode();
	V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE ) );
	V( dxC_SetRenderState( D3DRS_ALPHATESTENABLE,  FALSE ) );

	DL_CLEAR_LIST( DRAWLIST_INTERFACE );
	DL_SET_STATE_PTR(     DRAWLIST_INTERFACE, DLSTATE_VIEW_MATRIX_PTR, &matView );
	DL_SET_STATE_PTR(     DRAWLIST_INTERFACE, DLSTATE_PROJ_MATRIX_PTR, &matProj );
	DL_SET_VIEWPORT(	  DRAWLIST_INTERFACE, &tViewport );
	DL_SET_STATE_PTR(     DRAWLIST_INTERFACE, DLSTATE_EYE_LOCATION_PTR, &vEyePt );
	DL_SET_STATE_DWORD(	  DRAWLIST_INTERFACE, DLSTATE_USE_LIGHTS, FALSE );
	//DL_SET_STATE_DWORD(	  DRAWLIST_INTERFACE, DLSTATE_ALLOW_INVISIBLE_MODELS, TRUE ); // inventory models are typically marked nodraw
	DL_SET_STATE_DWORD(	  DRAWLIST_INTERFACE, DLSTATE_OVERRIDE_ENVDEF, pAppEnv->tHeader.nId );
	DL_SET_STATE_DWORD(	  DRAWLIST_INTERFACE, DLSTATE_NO_BACKUP_SHADERS, TRUE );
	DL_DRAW(			  DRAWLIST_INTERFACE, nModelID, 0 );

	//for ( int i = 0; i < pModel->tAttachmentHolder.nCount; i++ )
	//	if ( pModel->tAttachmentHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL )
	//		DL_DRAW(	  DRAWLIST_INTERFACE, pModel->tAttachmentHolder.pAttachments[ i ].nAttachedId, 0 );

	int nIndex = 0;
	int nAttachedID;
	while ( ( nAttachedID = gpfn_AttachmentGetAttachedOfType( nModelID, ATTACHMENT_MODEL, nIndex ) ) != INVALID_ID )
	{
		DL_DRAW( DRAWLIST_INTERFACE, nAttachedID, 0 );
	}

	DL_RESET_FULLVIEWPORT(	  DRAWLIST_INTERFACE );
	BOOL bFogEnabled = e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED );
	e_SetRenderFlag( RENDER_FLAG_FOG_ENABLED, FALSE );
	V( dx9_RenderDrawList( DRAWLIST_INTERFACE ) );
	e_SetRenderFlag( RENDER_FLAG_FOG_ENABLED, bFogEnabled );

	MatrixIdentity( &matRotateObj );
	V( e_ModelSetLocationPrivate( nModelID, &matRotateObj, vPos, MODEL_DYNAMIC ) );

	if ( bSavedCheapMode )
		e_StartCheapMode();
	V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE ) );
	V( dxC_SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE ) );

	// restore some UI-critical states
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE) );
	V( dx9_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE) );
	V( dx9_SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE) );
	V( dx9_SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE) );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dx9_RenderInterfaceCommand( int nCurID, RenderTextureUsage & tStatus )
{
	D3D_DRAWLIST_COMMAND * pCommand = dx9_DrawListCommandGet( DRAWLIST_INTERFACE, nCurID );
	ASSERT( pCommand );
	int nNextID = dx9_DrawListCommandGetNextID( DRAWLIST_INTERFACE, nCurID );
	int nOrigID = nCurID;

	D3D_DRAWLIST_COMMAND tUserCommand;
	BOOL bUserCommand = pCommand->nCommand == DLCOM_SET_STATE;
	if ( bUserCommand )
	{
		// create user command; SetState(override env def), typically
		//dx9_DrawListAddCommand( DRAWLIST_SCRATCH, DLCOM_SET_STATE, pCommand->nID, pCommand->nData, pCommand->pData, NULL, pCommand->dwFlags );
		tUserCommand.Clear();
		tUserCommand = *pCommand;
		tUserCommand.nCommand = DLCOM_SET_STATE;

		ASSERT( nNextID != INVALID_ID );
		nCurID = nNextID;
		pCommand = dx9_DrawListCommandGet( DRAWLIST_INTERFACE, nCurID );
		ASSERT( pCommand );
		nNextID = dx9_DrawListCommandGetNextID( DRAWLIST_INTERFACE, nCurID );
	}

	ASSERT( pCommand->nCommand == DLCOM_DRAW );

	int nVPWidth  = int(pCommand->bBox.vMax.fX - pCommand->bBox.vMin.fX + 1.0f); // this render's viewport width
	int nVPHeight = int(pCommand->bBox.vMax.fY - pCommand->bBox.vMin.fY + 1.0f); // this render's viewport height
	// weird hack here: there's the spacing size and the render viewport size, and they aren't equal
	// the bigger one ( spacing size ) is stored in the z components of the bbox
	int nSpacingWidth  = int(pCommand->bBox.vMin.fZ - pCommand->bBox.vMin.fX + 1.0f); // footprint in the dest texture
	int nSpacingHeight = int(pCommand->bBox.vMax.fZ - pCommand->bBox.vMin.fY + 1.0f); // footprint in the dest texture
	ASSERT( nVPWidth && nVPHeight && nSpacingWidth && nSpacingHeight );
	if ( tStatus.nCurX + nSpacingWidth >= tStatus.nMaxWidth )
	{
		// next row
		tStatus.nCurY       += tStatus.nUsedHeight;
		tStatus.nUsedHeight =  0;
		tStatus.nCurX       =  0;
	}
	if ( tStatus.nUsedHeight < nSpacingHeight )
	{
		// track tallest render in row
		tStatus.nUsedHeight = nSpacingHeight;

		if ( tStatus.nCurY + tStatus.nUsedHeight >= tStatus.nMaxHeight )
		{
			// can't fit any more: finalize this render and start with a fresh render target
			return nOrigID;
		}
	}

	BOUNDING_BOX tBBoxSpace, tBBoxVP;
	tBBoxSpace.vMin = VECTOR( float(tStatus.nCurX                ), float(tStatus.nCurY                 ), 0.0f );
	tBBoxSpace.vMax = VECTOR( float(tStatus.nCurX + nSpacingWidth), float(tStatus.nCurY + nSpacingHeight), 1.0f );
	tBBoxVP.vMin = VECTOR( float(tStatus.nCurX           ), float(tStatus.nCurY            ), 0.0f );
	tBBoxVP.vMax = VECTOR( float(tStatus.nCurX + nVPWidth), float(tStatus.nCurY + nVPHeight), 1.0f );

	// clear entire footprint, including buffer region
	//DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &tBBoxSpace );
	//DL_CLEAR_COLORDEPTH( DRAWLIST_SCRATCH, 0x00000000 );

	// apply user command
	if ( bUserCommand )
		dx9_DrawListAddCommand( DRAWLIST_SCRATCH, &tUserCommand );
	// set viewport to visible region
	DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &tBBoxVP );
	DL_DRAW( DRAWLIST_SCRATCH, pCommand->nID, 0 );

	tStatus.nCurX += nSpacingWidth;

	if ( nNextID != INVALID_ID )
	{
		nNextID = dx9_RenderInterfaceCommand( nNextID, tStatus );
	}

	// re-adding bbox as viewport...
	DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &tBBoxSpace );

	// ... copy_rendertarget uses current d3d viewport as the source rect
	D3D_DRAWLIST_COMMAND * pCopyCommand = dx9_DrawListAddCommand( DRAWLIST_SCRATCH );
	ASSERT( pCopyCommand );
	pCopyCommand->nCommand = DLCOM_COPY_RENDERTARGET;
	pCopyCommand->nID      = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FULL );
	pCopyCommand->bBox     = pCommand->bBox;
	pCopyCommand->pData    = pCommand->pData;

	return nNextID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderToInterfaceTexture(
	MATRIX * pmatView,
	MATRIX * pmatProj,
	VECTOR * pvEyeLocation,
	BOOL bClear )
{
	D3D_PROFILE_REGION( L"Interface Texture" );

	const SWAP_CHAIN_RENDER_TARGET nSwapRT = SWAP_RT_FULL;

	DL_CLEAR_LIST( DRAWLIST_SCRATCH );

	RenderTextureUsage tStatus;
	D3DC_TEXTURE2D_DESC tDesc;

	RENDER_TARGET_INDEX nRenderTarget = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), nSwapRT );
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );

	V_RETURN( dxC_Get2DTextureDesc( dxC_RTResourceGet( nRenderTarget ), 0, &tDesc ) );

	// skip the first command if it's a RESET_STATE command
	int nIndex = dx9_DrawListCommandGetFirstID( DRAWLIST_INTERFACE );
	if ( nIndex != INVALID_ID && dx9_DrawListCommandGet( DRAWLIST_INTERFACE, nIndex )->nCommand == DLCOM_RESET_STATE )
		nIndex = dx9_DrawListCommandGetNextID( DRAWLIST_INTERFACE, nIndex );

	// may have to do multiple renderDrawList calls on this texture
	while ( nIndex != INVALID_ID )
	{
		DL_SET_RENDER_DEPTH_TARGETS(	DRAWLIST_SCRATCH, nRenderTarget, eDT );
		DL_SET_STATE_PTR(   			DRAWLIST_SCRATCH, DLSTATE_VIEW_MATRIX_PTR, pmatView );
		DL_SET_STATE_PTR(   			DRAWLIST_SCRATCH, DLSTATE_PROJ_MATRIX_PTR, pmatProj );
		DL_SET_STATE_PTR(   			DRAWLIST_SCRATCH, DLSTATE_EYE_LOCATION_PTR, pvEyeLocation );
		DL_SET_STATE_DWORD(				DRAWLIST_SCRATCH, DLSTATE_USE_LIGHTS, TRUE );
		DL_SET_STATE_DWORD(				DRAWLIST_SCRATCH, DLSTATE_ALLOW_INVISIBLE_MODELS, TRUE ); // inventory models are typically marked nodraw
		//DL_SET_STATE_DWORD(				DRAWLIST_SCRATCH, DLSTATE_OVERRIDE_SHADERTYPE, SHADER_TYPE_INVENTORY );

		if ( bClear )
		{
			DL_RESET_FULLVIEWPORT( DRAWLIST_SCRATCH );
			DL_CLEAR_COLORDEPTH( DRAWLIST_SCRATCH, 0 );
		}

		ZeroMemory( &tStatus, sizeof(tStatus) );
		tStatus.nMaxWidth  = tDesc.Width;
		tStatus.nMaxHeight = tDesc.Height;

		// recurse through interface commands
		nIndex = dx9_RenderInterfaceCommand( nIndex, tStatus );
		V( dx9_RenderDrawList( DRAWLIST_SCRATCH ) );
		DL_CLEAR_LIST( DRAWLIST_SCRATCH );
	}

	DL_CLEAR_LIST( DRAWLIST_INTERFACE );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderToLightingMap(
	D3D_SHADOW_BUFFER * pShadow,
	LEVEL* pLevel,
	UNIT* pPlayer
	)
{
	if ( dxC_IsPixomaticActive() )
		return E_NOTIMPL;

	DL_CLEAR_LIST( DRAWLIST_SCRATCH );
	DEPTH_TARGET_INDEX eDepth = DEPTH_TARGET_NONE;
	if ( pShadow->eShadowMap != -1 )
		eDepth = dxC_GetGlobalDepthTargetIndex( dxC_GetGlobalDepthTargetFromShadowMap( pShadow->eShadowMap ) );
	if ( eDepth < 0 )
		eDepth = DEPTH_TARGET_NONE;
	DL_SET_RENDER_DEPTH_TARGETS( DRAWLIST_SCRATCH, pShadow->eRenderTarget, eDepth );
	DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &pShadow->tVP );

	// travis - This is for tugboat. It figures into the modulate2x lighting that happens with line-of-sight
	// and particle lights. so THIS is why everything was turning black ;)
	if( pLevel && e_VisibilityMeshGetCurrent() )
	{
		CVisibilityMesh * pVisMesh = e_VisibilityMeshGetCurrent();
		if( pVisMesh->BaseOpacity() == 1 )
		{
			DL_CLEAR_COLOR( DRAWLIST_SCRATCH, 0xff404040 );
		}
		else
		{
			DL_CLEAR_COLOR( DRAWLIST_SCRATCH, 0xffbbbbbb ); // 0xff808080 );//			
		}
	}
	else
	{
		DL_CLEAR_COLOR( DRAWLIST_SCRATCH, 0xffbbbbbb ); // 0xff808080 );//
	}

	BOUNDING_BOX tVP = pShadow->tVP;
	tVP.vMin += VECTOR( 0, 0, 0.0f );
	tVP.vMax += VECTOR( 0.0f, -0.0f, 0.0f );
	DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &tVP );
	DL_SET_STATE_PTR( DRAWLIST_SCRATCH, DLSTATE_VIEW_MATRIX_PTR, &pShadow->mShadowView );
	DL_SET_STATE_PTR( DRAWLIST_SCRATCH, DLSTATE_PROJ_MATRIX_PTR, &pShadow->mShadowProj );

	V_RETURN( dx9_RenderDrawList( DRAWLIST_SCRATCH ) );


    //if ( ! e_GetUIOnlyRender() )
    if( e_GetCurrentLevelLocation() != LEVEL_LOCATION_OUTDOOR &&
		e_VisibilityMeshGetCurrent() )
    {
	    //PERF(DRAW_PART);
	    //D3D_PROFILE_REGION( L"Particles" );
	    V( e_ParticlesDrawPreLightmap( pShadow->mShadowView, pShadow->mShadowProj, pShadow->mShadowProj, pShadow->vLightPos, pShadow->vLightDir ) );
    }


    // visibility overlay
#ifdef ENGINE_TARGET_DX9
    V( dx9_SetVertexShader( NULL ) );
    V( dx9_SetPixelShader( NULL ) );
#endif
    //dxC_SetIndices(NULL);
	// CHB 2006.11.13 - Removed alpha writing.
    V( dxC_SetRenderState(D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE /*| D3DCOLORWRITEENABLE_ALPHA*/ ) );
	V( dxC_SetRenderState(D3DRS_ALPHABLENDENABLE,	TRUE) );
	V( dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA) );
	V( dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA) );
    V( dxC_SetRenderState(D3DRS_BLENDOP,			D3DBLENDOP_ADD) );
    V( dxC_SetRenderState(D3DRS_ALPHATESTENABLE,	TRUE) );
    V( dxC_SetRenderState(D3DRS_ALPHAREF,			1L) );
    V( dxC_SetRenderState(D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL) );
    V( dxC_SetRenderState(D3DRS_ZENABLE,			D3DZB_FALSE) );
    V( dxC_SetRenderState(D3DRS_ZWRITEENABLE,		D3DZB_FALSE) );

#ifdef ENGINE_TARGET_DX9
    V( dx9_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE) );
    V( dx9_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE) );
    V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE) );
    V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE) );
    V( dxC_SetTexture(1, NULL) );
    V( dx9_SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE) );
    V( dxC_SetTexture(2, NULL) );
    V( dx9_SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE) );
#endif

    if ( !c_GetToolMode() &&
	    AppGetCltGame() )
    {
	    MATRIX matWorld;
	    MatrixIdentity( &matWorld );

	    
	    if( pPlayer )
	    {

		    if( e_GetCurrentLevelLocation() != LEVEL_LOCATION_OUTDOOR &&
				e_GetRenderFlag( RENDER_FLAG_LINEOFSIGHT_ENABLED ) )
		    {
			    D3DXMATRIXA16 View = (D3DXMATRIXA16)pShadow->mShadowView;
			    //dxC_GetDevice()->SetTransform( D3DTS_WORLD,	  (D3DXMATRIXA16*)( &matWorld ) );
			    //dxC_GetDevice()->SetTransform( D3DTS_VIEW,		  &View );
			    //dxC_GetDevice()->SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)&pShadow->mShadowProj );
				dxC_SetTransform( D3DTS_WORLD, (D3DXMATRIXA16*)( &matWorld ) );
				dxC_SetTransform( D3DTS_VIEW, &View );
				dxC_SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)&pShadow->mShadowProj );

				CVisibilityMesh * pVisMesh = e_VisibilityMeshGetCurrent();
				if ( pVisMesh )
				{
					e_ParticleDrawEnterSection( PARTICLE_DRAW_SECTION_LINEOFSIGHT );
					e_VisibilityMeshGetCurrent()->RenderVisibility( pShadow->mShadowView );
					//V( pLevel->m_pVisibility->RenderVisibility( pShadow->mShadowView ) ); 
					e_ParticleDrawLeaveSection();
				}
			}
	    }
    }
	e_SetFogEnabled( FALSE );
    DX9_BLOCK( V( dx9_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE) ); )
    DX9_BLOCK( dx9_DirtyStateCache(); ) //KMNV TODO
    //if ( ! e_GetUIOnlyRender() )

    {
	    //PERF(DRAW_PART);
	    //D3D_PROFILE_REGION( L"Particles" );
	    V( e_ParticlesDrawLightmap( pShadow->mShadowView, pShadow->mShadowProj, pShadow->mShadowProj, pShadow->vLightPos, pShadow->vLightDir ) );
    }
    dxC_SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE);
    dxC_SetRenderState(D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
    dxC_SetRenderState(D3DRS_ALPHABLENDENABLE,	TRUE);
    dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_ONE);
    dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_ZERO);
    dxC_SetRenderState(D3DRS_BLENDOP,				D3DBLENDOP_ADD);
    dxC_SetRenderState(D3DRS_ALPHATESTENABLE,		FALSE);
    dxC_SetRenderState(D3DRS_ALPHAREF,			1L);
	dxC_SetRenderState(D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL);
    DX9_BLOCK( V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE) ); )
    dxC_SetRenderState(D3DRS_ZWRITEENABLE,		D3DZB_TRUE);
    // layer on particles

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
PRESULT dx9_RenderToParticleLightingMap(void)
{
	if ( AppIsTugboat() )	// CHB 2007.02.15
	{
		GAME* pGame = AppGetCltGame();
		if (!pGame)
			return S_FALSE;

		UNIT* pPlayer = GameGetControlUnit( pGame );
		LEVEL* pLevel = NULL;
		if( pPlayer )
		{
			pLevel = UnitGetLevel( pPlayer );
		}

		// CHB 2007.04.06 - Moved from dx9_RenderToShadowMaps(),
		// because whaddayaknow, it has nothing to do with shadow
		// maps.
		int nLevelLocation = e_GetCurrentLevelLocation();
	    if( pLevel &&
		    pLevel->m_pVisibility &&
			e_GetRenderFlag( RENDER_FLAG_LINEOFSIGHT_ENABLED ))
	    {
		    LevelUpdateVisibility( pGame, pLevel, pPlayer->vDrawnPosition );
		    pLevel->m_pVisibility->Sort( AppCommonGetElapsedTime() );
			pLevel->m_pVisibility->SortVisible( );
	    }


		D3D_SHADOW_BUFFER * pShadow = dxC_GetParticleLightingMapShadowBuffer();
		V( sRenderToLightingMap( pShadow, pLevel, pPlayer ) );

	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RenderToShadowMaps()
{
#if !ISVERSION(SERVER_VERSION)
	// iterate through shadow map objects
	// for each shadow map object, render its shadow map

	if ( ! e_ShadowsEnabled() || ! e_ShadowsActive() )
		return S_FALSE;
	ASSERT_RETFAIL( e_UseShadowMaps() );

#ifdef ENGINE_TARGET_DX9
	V( dxC_SetTexture( SAMPLER_SHADOW, NULL ) );
	V( dxC_SetTexture( SAMPLER_SHADOWDEPTH, NULL ) );
#endif

	if ( AppIsTugboat() )
	{
		V( dxC_SetTexture( SAMPLER_PARTICLE_LIGHTMAP, NULL ) );
	}

	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	if ( !pCameraInfo )
		return S_FALSE;

	e_SetFogEnabled( FALSE );

	int nShadowType = e_GetActiveShadowType();
	ASSERT_RETFAIL(nShadowType != SHADOW_TYPE_NONE);

	int nLevelLocation = e_GetCurrentLevelLocation();
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	ASSERT_RETFAIL( pEnvDef );

	for ( int nShadowID = dx9_ShadowBufferGetFirstID();
		nShadowID != INVALID_ID;
		nShadowID = dx9_ShadowBufferGetNextID( nShadowID ) )
	{
		if ( nShadowID == e_GetRenderFlag( RENDER_FLAG_SHADOWS_SKIP_MAP ) )
			continue;

		D3D_SHADOW_BUFFER* pShadow = dx9_ShadowBufferGet( nShadowID );

		if ( e_GetRenderFlag( RENDER_FLAG_SHADOWS_ALWAYS_DIRTY ) )
			SET_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY );

		if ( ! TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY )
	    || ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_STATIC | SHADOW_BUFFER_FLAG_FULL_REGION ) &&
		     nLevelLocation != LEVEL_LOCATION_OUTDOOR ) )
			continue;

		D3D_PROFILE_REGION_INT( L"Shadow Map", nShadowID );

		if ( pShadow->eRenderTargetPrimer != RENDER_TARGET_INVALID 
		  && nLevelLocation == LEVEL_LOCATION_OUTDOOR )
		{
			D3D_EFFECT* pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SHADOW ) );
			ASSERT( pEffect );
			if ( dxC_EffectIsLoaded( pEffect ) )
			{
				EFFECT_TECHNIQUE * pTech = NULL;
				TECHNIQUE_FEATURES tFeat;
				tFeat.Reset();
				tFeat.nInts[ TF_INT_INDEX ] = 3;
				tFeat.nInts[ TF_INT_MODEL_SHADOWTYPE ] = nShadowType;
				V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTech ) );
				ASSERT( pTech->hHandle );

				UINT nPassCount;
				V_CONTINUE( dxC_SetTechnique( pEffect, pTech, &nPassCount ) );
				V_CONTINUE( dxC_EffectBeginPass( pEffect, 0 ) );

#ifdef ENGINE_TARGET_DX10
				LPD3DCBASESHADERRESOURCEVIEW pShadowTexture = dxC_DSShaderResourceView( pShadow->eRenderTargetPrimer );
#else
				LPD3DCBASESHADERRESOURCEVIEW pShadowTexture = dxC_RTShaderResourceGet( pShadow->eRenderTargetPrimer );
#endif
				V( dxC_SetSamplerDefaults( SAMPLER_SHADOW ) );
				V( dx9_EffectSetTexture( pEffect, *pTech, EFFECT_PARAM_SHADOWMAP, pShadowTexture ) );

				DEPTH_TARGET_INDEX eDepth = dxC_GetGlobalDepthTargetIndex( dxC_GetGlobalDepthTargetFromShadowMap( pShadow->eShadowMap ) );
				V( dxC_SetRenderTargetWithDepthStencil( pShadow->eRenderTarget, eDepth ) );
				
				if ( nShadowType == SHADOW_TYPE_COLOR )
				{
					V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER ) );
					V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED ) );
				}
				else
				{
					V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_ZBUFFER ) );
				}

				V( dxC_SetRenderState( D3DRS_ZENABLE, TRUE ) ); 
				V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );

				V( dxC_DrawFullscreenQuad( FULLSCREEN_QUAD_UV, pEffect, *pTech ) );
			}
		}

		// we already have the parameters for this shadow map
		// render the scene into the shadow map buffer
		// build the draw list
		// just do the models in this room

		DL_CLEAR_LIST( DRAWLIST_SCRATCH );
		DEPTH_TARGET_INDEX eDepth = dxC_GetGlobalDepthTargetIndex( dxC_GetGlobalDepthTargetFromShadowMap( pShadow->eShadowMap ) );
		DL_SET_RENDER_DEPTH_TARGETS( DRAWLIST_SCRATCH, pShadow->eRenderTarget, eDepth );
		DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &pShadow->tVP );
		dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		if( AppIsHellgate() )
		{
			// CHB 2006.11.03 - We keep this as white here for
			// generic shadow maps, as we need the color render
			// target color component initialized to 1.0.
			if (nShadowType == SHADOW_TYPE_COLOR)
			{
				if ( pShadow->eRenderTargetPrimer == RENDER_TARGET_INVALID
				  || nLevelLocation != LEVEL_LOCATION_OUTDOOR )
					DL_CLEAR_COLORDEPTH( DRAWLIST_SCRATCH, 0xffffffff );
			}
			else
			{
				if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER ) )
					DL_CLEAR_COLORDEPTH( DRAWLIST_SCRATCH, 0xffffffff );
				else if ( pShadow->eRenderTargetPrimer == RENDER_TARGET_INVALID
					   || nLevelLocation != LEVEL_LOCATION_OUTDOOR )
					DL_CLEAR_DEPTH( DRAWLIST_SCRATCH );
			}
		}
		else
		{
			// Tugboat used to use the shadow color buffer for a lighting map
			// now it's separate

			// CHB 2006.11.03 - We keep this as white here for
			// generic shadow maps, as we need the color render
			// target color component initialized to 1.0.
			if (nShadowType == SHADOW_TYPE_COLOR)
			{
				DL_CLEAR_COLORDEPTH( DRAWLIST_SCRATCH, 0xffffffff );
			} 
			else
			{
				DL_CLEAR_DEPTH( DRAWLIST_SCRATCH );
			}
		}

		// Add a one-texel border to prevent "shadow streaks"
		BOUNDING_BOX tVP = pShadow->tVP;
		tVP.vMin += VECTOR( 1, 1, 0.0f );
		tVP.vMax += VECTOR( -1.0f, -1.0f, 0.0f );
		DL_SET_VIEWPORT( DRAWLIST_SCRATCH, &tVP );
		DL_SET_STATE_PTR( DRAWLIST_SCRATCH, DLSTATE_VIEW_MATRIX_PTR, &pShadow->mShadowView );
		DL_SET_STATE_PTR( DRAWLIST_SCRATCH, DLSTATE_PROJ_MATRIX_PTR, &pShadow->mShadowProj );
		//DL_SET_STATE_DWORD( DRAWLIST_SCRATCH, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND );

		PROXNODE nCurrent;
		float fRad = pShadow->fScale * (AppIsHellgate() ? SHADOW_RANGE_FUDGE : 1 );
				
		PLANE pPlanes[ NUM_FRUSTUM_PLANES ];
		V( e_ExtractFrustumPlanes( pPlanes, &pShadow->mShadowView, &pShadow->mShadowProj ) );
		REGION* pRegion = e_GetCurrentRegion();
		BOOL bIndoor = ( e_GetCurrentLevelLocation() == LEVEL_LOCATION_INDOOR );
		
		int nModelID, nNextModelID = e_ModelProxMapGetCloseFirst( nCurrent, pShadow->vWorldFocus, fRad );
		BOUNDED_WHILE( nNextModelID != INVALID_ID, 10000 )
		{
			nModelID = nNextModelID;
			nNextModelID = e_ModelProxMapGetCloseNext( nCurrent );

			D3D_MODEL* pModel = dx9_ModelGet( nModelID );
			if (!pModel)
			{
				continue;
			}

			bool bBackGroundModel = ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED );
			int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL_SHADOW );
			if ( nIsolateModel != INVALID_ID && nIsolateModel != pModel->nId )
			{
				continue;
			}

			bool bPropModel = bBackGroundModel && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROP );

			// If this is an environment that wants to use blob shadows for actors/props, check this model for the "force articulated shadow" flag and skip it if missing
			if (    ( bPropModel || ! bBackGroundModel ) 
				 && e_GetActiveShadowTypeForEnv( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ARTICULATED_SHADOW ), pEnvDef ) == SHADOW_TYPE_NONE )
				continue;

			if ( ! dx9_ModelShouldRender( pModel ) && 
				 ! TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_STATIC ) )
				continue;

			if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NOSHADOW ) )
				continue;

			DRAW_LAYER eDrawLayer = dxC_ModelDrawLayerFromDrawFlags( pModel->dwDrawFlags );
			if ( eDrawLayer != DRAW_LAYER_GENERAL )
				continue;

			if ( pModel->fDistanceAlpha < 0.5f )
				continue;

			if ( AppIsTugboat() )
			{
				if ( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
					continue;
				// to prevent crazy shadow ranges with our pov view
				float fDistToCamera = VectorDistanceXY( pModel->vPosition, pCameraInfo->vLookAt );
				if( fDistToCamera > 30 )
				{
					continue;
				}
			}
			else if ( AppIsHellgate() )
			{
				if ( pRegion->nId != pModel->nRegion )
					continue;

				if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
					continue;

				if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_STATIC )
					 && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
					continue;

				if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC ) )
				{
					if ( bBackGroundModel )
					{
						// compare environment and model flag
						if ( ! bIndoor || ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_CAST_DYNAMIC_SHADOW ) )
							continue;
					}
				}
			}

			if ( ( ! AppIsTugboat() && nLevelLocation != LEVEL_LOCATION_OUTDOOR )
			  || TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_FULL_REGION )
			  || dxC_ModelProjectedInFrustum( pModel, LOD_NONE, INVALID_ID, pPlanes, pShadow->vLightDir ) )
			{
					DL_DRAW_SHADOW( DRAWLIST_SCRATCH, nModelID, nShadowID );
			}
		}

		V( dx9_RenderDrawList( DRAWLIST_SCRATCH ) );
		if ( ! TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_ALWAYS_DIRTY ) )
			CLEAR_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY );
	}

	DL_CLEAR_LIST( DRAWLIST_SCRATCH );

	RENDER_TARGET_INDEX eRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER );
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );

	DL_SET_RENDER_DEPTH_TARGETS( DRAWLIST_SCRATCH, eRT, eDT );
	DL_RESET_VIEWPORT( DRAWLIST_SCRATCH );
	V_RETURN( dx9_RenderDrawList( DRAWLIST_SCRATCH ) );

	// restore depth bias state
	dxC_SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, (DWORD)0 );
	dxC_SetRenderState( D3DRS_DEPTHBIAS, (DWORD)0 );

	e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );

	// restore two-sided rendering to off
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	// restore alpha test to off
	V( dxC_SetAlphaTestEnable( FALSE ) );

	// restore alpha blend to off
	V( dxC_SetAlphaBlendEnable( FALSE ) );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );		
#endif // ! SERVER_VERSION

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RenderBlobShadows( void * pUserData )
{
#if !ISVERSION(SERVER_VERSION)
	RENDER_CONTEXT* pContext = (RENDER_CONTEXT*)pUserData;

	int nShadowType = e_GetActiveShadowTypeForEnv( FALSE, e_GetCurrentEnvironmentDef() );
	if ( nShadowType != SHADOW_TYPE_NONE )
		return S_FALSE;

	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	if ( !pCameraInfo )
		return S_FALSE;

	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return S_FALSE;

	UNIT * pPlayer = GameGetControlUnit( pGame );
	if( ! pPlayer )
		return S_FALSE;
	
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	if ( ! pLevel )
		return S_FALSE;

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SHADOW_BLOB ) );
	if ( ! pEffect )
		return S_FALSE;

	int nMaterial = e_GetGlobalMaterial( ExcelGetLineByStrKey( DATATABLE_MATERIALS_GLOBAL, "ShadowBlob" ) );
	MATERIAL * pMaterial = (MATERIAL *)DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterial );
	if ( ! pMaterial )
		return S_FALSE;

	if ( ! e_TextureIsValidID( pMaterial->nOverrideTextureID[ TEXTURE_DIFFUSE ] ) )
	{
		e_MaterialRestore( nMaterial );
		return S_FALSE;
	}

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );

	ASSERT_RETFAIL( nPassCount == 1 ); // only one pass

	static const float scfShadowRad = 25.f;
	BOOL bIndoor = ( e_GetCurrentLevelLocation() == LEVEL_LOCATION_INDOOR );

	BOOL bNoArticulated = SHADOW_TYPE_NONE != e_GetActiveShadowTypeForEnv( TRUE, pContext->pEnvDef );

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

	PROXNODE nCurrent;
	int nModelID, nNextModelID = e_ModelProxMapGetCloseFirst( nCurrent, pCameraInfo->vLookAt, scfShadowRad );
	BOUNDED_WHILE( nNextModelID != INVALID_ID, 10000 )
	{
		nModelID = nNextModelID;
		nNextModelID = e_ModelProxMapGetCloseNext( nCurrent );

		int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
		if ( nIsolateModel != INVALID_ID && nIsolateModel != nModelID )
			continue;

		D3D_MODEL * pModel = dx9_ModelGet( nModelID );
		if ( ! pModel )
			continue;

		if ( ! dx9_ModelShouldRender( pModel ) )
			continue;

		if (    dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NOSHADOW ) 
			 || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ATTACHMENT )
			 || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
			continue;

		if (    bNoArticulated
			 && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ARTICULATED_SHADOW ) )
			continue;

		DRAW_LAYER eDrawLayer = dxC_ModelDrawLayerFromDrawFlags( pModel->dwDrawFlags );
		if ( eDrawLayer != DRAW_LAYER_GENERAL )
			continue;

		if ( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
		{
			//if ( AppIsTugboat() || ! bIndoor || ! TEST_MASK( pModel->dwFlags, MODEL_FLAG_CAST_DYNAMIC_SHADOW ) )
				continue;
		}

		VECTOR vGroundNormal = VECTOR( 0, 0, 1.f );
		static const VECTOR vCollideDir = VECTOR( 0, 0, -1.0f);
		static const float scfMaxLen = 50.f;
		float fDist = LevelLineCollideLen( pGame, pLevel, pModel->vPosition, 
			vCollideDir, scfMaxLen, &vGroundNormal );
		if ( vGroundNormal.fZ < 0 )
			vGroundNormal.fZ *= -1;
		
		if ( FEQ( fDist, scfMaxLen, 0.1f ) )
			fDist = 0.f; // we are probably poking through the floor a little bit

		VECTOR vModelPosition;
		if ( ! e_ModelGetRagdollPosition( nModelID, vModelPosition ) )
			vModelPosition = pModel->vPosition;
		VECTOR vGround = vModelPosition + ( vCollideDir * fDist );

		static const float scfMaxShadowHeight = 5.f;
		float fHeightScale = scfMaxShadowHeight - MIN( fDist, scfMaxShadowHeight );
		fHeightScale /= scfMaxShadowHeight;
		float fWorldSize = 1.f;
		VECTOR vWeights( 1.f, 1.f, 0.f );
		V( dxC_ModelGetWorldSizeAvgWeighted( pModel, fWorldSize, vWeights ) );
		float fScale = fHeightScale * fWorldSize * 0.5f;
		VECTOR vScale = VECTOR( fScale, fScale, 1 );

		// Setup the world, view, and projection matrices
		MATRIX mWorld;
		MatrixTranslationScale( &mWorld, &vGround, &vScale );

		static const VECTOR scvUp = VECTOR( 0, 0, 1 );
		float fAngle = VectorAngleBetween( vGroundNormal, scvUp );

		if ( ! FEQ( fAngle, 0.f, 0.1f ) )
		{
			VECTOR vAxis;
			VectorCross( vAxis, scvUp, vGroundNormal );
			VectorNormalize( vAxis );

			// Clamp rotation at 30 degrees to prevent vertical shadows.
			static const float scfMaxAngle = DEG_TO_RAD( 30.f );
			if ( fAngle > scfMaxAngle )
				fAngle = scfMaxAngle;
			else if ( fAngle < -scfMaxAngle )
				fAngle = -scfMaxAngle;

			MATRIX mRotate;
			MatrixRotationAxis( mRotate, vAxis, fAngle );
			MatrixMultiply( &mWorld, &mRotate, &mWorld );
		}

		MATRIX* pmatWorldCurrent = &mWorld;
		MATRIX* pmatViewCurrent = &pContext->matView;
		MATRIX* pmatProjCurrent = &pContext->matProj;

		V_RETURN( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_WORLDMATRIX, pmatWorldCurrent ) );
		V_RETURN( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_VIEWMATRIX,	pmatViewCurrent ) );
		V_RETURN( dx9_EffectSetMatrix( pEffect, *ptTechnique, EFFECT_PARAM_PROJECTIONMATRIX,  pmatProjCurrent ) );

		ENVIRONMENT_DEFINITION * pEnvDef = NULL;
		if ( pModel && pModel->nEnvironmentDefOverride != INVALID_ID )
		{
			pEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pModel->nEnvironmentDefOverride );
			ASSERT_CONTINUE( pEnvDef );
		}
		else
		{
			pEnvDef = e_GetCurrentEnvironmentDef();
			ASSERT_CONTINUE( pEnvDef );
		}

		FXConst_MiscLighting tConstLighting = {0};
		float fShadowIntensity = e_GetShadowIntensity( pEnvDef, FALSE );
		//float fDistToCamera = VectorDistanceXY( vGround, pCameraInfo->vLookAt );
		//fShadowIntensity = LERP( 0.f, fShadowIntensity, ( scfShadowRad - fDistToCamera ) / scfShadowRad );
		fShadowIntensity = min( fShadowIntensity, 1.f );
		fShadowIntensity = max( fShadowIntensity, 0.f );
		tConstLighting.fShadowIntensity = fShadowIntensity * fHeightScale;
		V_RETURN( dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_MISC_LIGHTING_DATA, (VECTOR4*)&tConstLighting ) );

		V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

		//V( dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE ) );
		//V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE ) );
		//V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
		//V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ) );
		//V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE ) );
		//V( dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT ) );
		//V( dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ) );
		//V( dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE ) );
		//V( dxC_SetRenderState( D3DRS_ALPHATESTENABLE, TRUE ) );
		//V( dxC_SetRenderState( D3DRS_FOGENABLE, FALSE ) );

		V_RETURN( dxC_SetTexture( pMaterial->nOverrideTextureID[ TEXTURE_DIFFUSE ], 
			*ptTechnique, pEffect, EFFECT_PARAM_DIFFUSEMAP0, INVALID_ID ) );
 
		V_RETURN( dxC_DrawQuad( BASIC_QUAD_XY_PLANE, pEffect, *ptTechnique ) );
	}

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
#endif // ! SERVER_VERSION

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderModelBoundingBox (
	int nDrawList,
	int nModelID,
	BOOL bWriteColorAndDepth,
	DWORD dwColor )
{
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG(pModel);

	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	ASSERT_RETFAIL( pState );

	BOOL bVisible = dx9_ModelShouldRender( pModel );
	if ( !bVisible && !pState->bAllowInvisibleModels )
	{
		return S_FALSE;
	}

	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return S_FALSE;

	// Setup the world, view, and projection matrices
	MATRIX * pmatWorldCurrent= dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_WORLD ) ? &pModel->matScaledWorld	: &mIdentity;
	MATRIX * pmatViewCurrent = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_VIEW )  ? &pModel->matView		: pState->pmatView;
	MATRIX * pmatProjCurrent = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ )  ? &pModel->matProj		: pState->pmatProj;

	BOUNDING_BOX tBBox;
	V( e_GetModelRenderAABBInObject( nModelID, &tBBox ) );
	V_RETURN( dx9_RenderBoundingBox( &tBBox, pmatWorldCurrent, pmatViewCurrent, pmatProjCurrent, bWriteColorAndDepth, ! pState->bNoDepthTest, dwColor ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RenderModelDepth (
	int nDrawList,
	int nModelID )
{
	D3D_PROFILE_REGION_INT( L"Model Depth", nModelID );

	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG(pModel);
	ASSERT_RETFAIL( dx9_ModelShouldRender( pModel ) );

	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) )
		return S_FALSE;

	int nLOD = LOD_HIGH;	// CHB 2006.12.08

	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	ASSERT_RETFAIL( pState );

	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_ZBUFFER ) );
	ASSERT_RETFAIL( pEffect );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	EFFECT_TECHNIQUE * pTechniqueRigid		= NULL;
	EFFECT_TECHNIQUE * pTechniqueAnimated	= NULL;
	TECHNIQUE_FEATURES tFeat;

	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueRigid ) );
	tFeat.nInts[ TF_INT_INDEX ] = 1;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueAnimated ) );

	ASSERT_RETFAIL( pTechniqueRigid   ->hHandle );
	ASSERT_RETFAIL( pTechniqueAnimated->hHandle );

	// Setup the world, view, and projection matrices
	MATRIX * pmatWorldCurrent= dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_WORLD ) ? &pModel->matScaledWorld	: &mIdentity;
	MATRIX * pmatViewCurrent = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_VIEW )  ? &pModel->matView		: pState->pmatView;
	MATRIX * pmatProjCurrent = dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ )  ? &pModel->matProj		: pState->pmatProj;

	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ ) )
		pmatProjCurrent = &pModel->matProj;
	else if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
	{
		pmatProjCurrent = pState->pmatProj2;
		//pModel->dwFlags &= ~MODEL_FLAG_FIRST_PERSON_PROJ; // this gets reset every frame
	} else
		pmatProjCurrent = pState->pmatProj;

	// Many shaders use these.  We could keep track of whether this model needs them... 
	D3DXMATRIXA16 matWorldView;
	D3DXMatrixMultiply ( &matWorldView, (D3DXMATRIXA16*)pmatWorldCurrent, (D3DXMATRIXA16*)pmatViewCurrent );
	D3DXMATRIXA16 matWorldViewProjection;
	D3DXMatrixMultiply ( &matWorldViewProjection, &matWorldView, (D3DXMATRIXA16*)pmatProjCurrent );

	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return S_FALSE;

	int nVertexBufferIndexPrevious = INVALID_ID;

	// don't draw the silhouette mesh or any alpha-blended meshes into the shadow buffer
	DWORD dwMeshFlagsDoDraw   = 0;
	DWORD dwMeshFlagsDontDraw = MESH_FLAG_SILHOUETTE | MESH_FLAG_ALPHABLEND;

	// Iterate through the meshes and draw them
	for ( int nMesh = 0; nMesh < pModelDefinition->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDefinition->pnMeshIds [ nMesh ] );
		ASSERT_CONTINUE( pMesh );

		dx9_MeshApplyMaterial( pMesh, pModelDefinition, pModel, nLOD, nMesh );	// CHB 2006.12.08

		DWORD dwMeshFlags = pMesh->dwFlags;
		int nShaderType = dx9_GetCurrentShaderType( NULL, pState );

		int nMaterialID = dx9_MeshGetMaterial( pModel, pMesh );
		//if ( nMaterialID != pMesh->nMaterialId )
		//{
		//	EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialID, nShaderType );
		//	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
		//		dwMeshFlags |= MESH_FLAG_ALPHABLEND;
		//}

		{
			if ( sMeshCullRender(
				pModel,
				pMesh,
				nMesh,
				dwMeshFlags,
				pState,
				NULL,
				dwMeshFlagsDoDraw,
				dwMeshFlagsDontDraw ) )
				continue;
		}

		MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID );
		if ( ! pMaterial )
			continue;

		D3D_EFFECT * pMeshEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
		V_CONTINUE( dxC_ToolLoadShaderNow( pMeshEffect, pMaterial, nShaderType ) );
		if ( (pMeshEffect->dwFlags & EFFECT_FLAG_RENDERTOZ) == 0 )
			continue;

		// set the vertex buffer
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex < pModelDefinition->nVertexBufferDefCount );
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];
		
		if ( ! dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) )
			continue;

		if ( pMesh->nVertexBufferIndex != nVertexBufferIndexPrevious )
		{
			//if( !dxC_SetVertexDeclaration( pVertexBuffer->eVertexType, pEffect ) )
			//{
			//	ASSERT( pVertexBuffer->dwFVF );
			//	dx9_SetFVF( pVertexBuffer->dwFVF );
			//}

#if 0			
			// AE 2006.12.13 - Only support initial z-pass on background and animated vertex types.
			D3D_VERTEX_BUFFER_DEFINITION tVBDef;

			switch ( pVertexBufferDef->eVertexType )
			{
			case VERTEX_DECL_RIGID_16:
			case VERTEX_DECL_RIGID_32:
			case VERTEX_DECL_RIGID_64:
				tVBDef.eVertexType = VERTEX_DECL_RIGID_ZBUFFER;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				break;

			case VERTEX_DECL_ANIMATED:
#if 1
				continue;
#else
				tVBDef.eVertexType = VERTEX_DECL_ANIMATED_ZBUFFER;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				break;
#endif

			default:
				UNDEFINED_CODE_X( "This vertex format is not supported for zpass!" );
				return E_FAIL;
			}	

			V_CONTINUE( dxC_SetStreamSource( tVBDef, 0, pEffect ) );
#else
			V_CONTINUE( dxC_SetStreamSource( *pVertexBufferDef, 0, pEffect ) );
#endif
			nVertexBufferIndexPrevious = pMesh->nVertexBufferIndex;
		}

		EFFECT_TECHNIQUE * pTechnique = (pMeshEffect->dwFlags & EFFECT_FLAG_ANIMATED) ? pTechniqueAnimated : pTechniqueRigid;
		UINT nPassCount;
		V_CONTINUE( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );
		ASSERT_CONTINUE( nPassCount == 1 );
		V_CONTINUE( dxC_EffectBeginPass( pEffect, 0 ) );

		// change alpha test, if necessary
		BOOL bMeshAlphaTest = FALSE;
		if ( dxC_MeshDiffuseHasAlpha( pModel, pModelDefinition, pMesh, nMesh, nLOD ) 
			/*|| TEST_MASK( pModel->dwFlags, MODEL_FLAG_FORCE_ALPHA )*/ )
		{
			// set alpha test, if specified for both effect and mesh
			EFFECT_DEFINITION * pEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pMeshEffect->nEffectDefId );
			ASSERT_CONTINUE( pEffectDef );
			bMeshAlphaTest = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_TEST );
			V( dxC_SetTexture(
				pMesh->pnTextures[ TEXTURE_DIFFUSE ],
				*pTechnique,
				pEffect,
				EFFECT_PARAM_DIFFUSEMAP0,
				INVALID_ID,
				NULL,
				NULL,
				TEXTURE_DIFFUSE,
				FALSE ) );
		}
		else
		{
			V( dxC_SetTexture(
				e_GetUtilityTexture( TEXTURE_RGBA_FF ),
				*pTechnique,
				pEffect,
				EFFECT_PARAM_DIFFUSEMAP0,
				INVALID_ID ) );
		}
		V( dxC_SetAlphaTestEnable( bMeshAlphaTest ) );

		V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &matWorldViewProjection ) );

		V( dxC_SetBones( pModel, nMesh, nLOD, pEffect, *pTechnique ) );	// CHB 2006.12.08, 2006.11.30

		// set the index buffer
		V_CONTINUE( dxC_SetIndices( pMesh->tIndexBufferDef ) );

		TRACE_MODEL_INT( pModel->nId, "Draw Primitive", nMesh );
		
		// loop through all of the passes for the effect
		if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		} else {
			V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderModelShadow (
	int nDrawList,
	int nData,
	int nModelID )
{	
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG(pModel);
	ASSERT_RETINVALIDARG( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NOSHADOW ) )
	ASSERT_RETFAIL( AppIsHellgate() || dx9_ModelShouldRender( pModel ) );

	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	ASSERT_RETFAIL( pState );

	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nData );

	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) )
		return S_FALSE;

	if ( ! e_GetRenderFlag( RENDER_FLAG_SHADOWS_ENABLED ) )
		return S_FALSE;

	//if ( pModel->dwFlags & MODEL_FLAG_NOSHADOW )
	//	return S_FALSE; // shouldn't get here, as it shouldn't have made it into the draw list

	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_SHADOW ) );
	ASSERT_RETFAIL( pEffect );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	EFFECT_TECHNIQUE * pTechniqueRigid16	= NULL;
	EFFECT_TECHNIQUE * pTechniqueRigid3264	= NULL;
	EFFECT_TECHNIQUE * pTechniqueAnimated	= NULL;
	TECHNIQUE_FEATURES tFeat;

	tFeat.Reset();
	tFeat.nInts[ TF_INT_MODEL_SHADOWTYPE ] = e_GetActiveShadowType();
	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueRigid16 ) );
	tFeat.nInts[ TF_INT_INDEX ] = 1;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueRigid3264 ) );
	tFeat.nInts[ TF_INT_INDEX ] = 2;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueAnimated ) );

	ASSERT_RETFAIL( pTechniqueRigid16->hHandle );
	ASSERT_RETFAIL( pTechniqueRigid3264->hHandle );
	ASSERT_RETFAIL( pTechniqueAnimated->hHandle );

	// Setup the world, view, and projection matrices
	MATRIX mIdentity;
	MATRIX * pmatWorldCurrent;

	MatrixIdentity( &mIdentity );
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_WORLD ) )
		pmatWorldCurrent = &pModel->matScaledWorld;
	else
		pmatWorldCurrent = &mIdentity;

	// don't try to shadow from objects that have their own projection
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_PROJ ) )
		return S_FALSE;
	// don't try to shadow from objects that have their own view
	if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_VIEW ) )
		return S_FALSE;
	//if ( pModel->dwFlags & MODEL_FLAG_VIEW )
	//	pmatViewCurrent = &pModel->matView; // this needs to be changed to take into account the shadow view... but nothing seems to use it
	//else
	//	pmatViewCurrent = pShadow->GetView();

	// CHB 2006.11.30
	int nModelDefId = pModel->nModelDefinitionId;
	int nLOD = dxC_ModelGetDisplayLODWithDefault( pModel );
	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( nModelDefId, nLOD );
	if ( ! pModelDefinition )
		return S_FALSE;

	D3D_PROFILE_REGION_INT_STR( L"Model Shadow", nModelID, pModelDefinition->tHeader.pszName );

	int nVertexBufferIndexPrevious = INVALID_ID;

	DWORD dwMeshFlagsDoDraw   = pState->dwMeshFlagsDoDraw;
	DWORD dwMeshFlagsDontDraw = pState->dwMeshFlagsDontDraw;

	V( dxC_SetAlphaTestEnable( FALSE ) );

	// don't draw the silhouette mesh into the shadow buffer
	dwMeshFlagsDoDraw &= ~MESH_FLAG_SILHOUETTE;
	dwMeshFlagsDontDraw |= MESH_FLAG_SILHOUETTE;

	int nDebugRenders = 0;

	// Iterate through the meshes and draw them
	for ( int nMesh = 0; nMesh < pModelDefinition->nMeshCount; nMesh++ )
	{
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDefinition->pnMeshIds [ nMesh ] );
		ASSERT_CONTINUE( pMesh );

		V_CONTINUE( dx9_MeshApplyMaterial( pMesh, pModelDefinition, pModel, nLOD, nMesh, pState->bForceLoadMaterial ) );

		//if ( ! dx9_MeshIsVisible( pModel, pMesh ) )
		//	continue;

		//if ( dwMeshFlagsDoDraw   && ( pMesh->dwFlags & dwMeshFlagsDoDraw   ) != dwMeshFlagsDoDraw )
		//	continue;
		//if ( dwMeshFlagsDontDraw && ( pMesh->dwFlags & dwMeshFlagsDontDraw ) != 0 )
		//	continue;

		DWORD dwMeshFlags = pMesh->dwFlags;
		int nShaderType = dx9_GetCurrentShaderType( NULL, pState );

		int nMaterialID = dx9_MeshGetMaterial( pModel, pMesh );
		if ( nMaterialID != pMesh->nMaterialId )
		{
			EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialID, nShaderType );
			if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
				dwMeshFlags |= MESH_FLAG_ALPHABLEND;
		}

		{
			if ( sMeshCullRender(
				pModel,
				pMesh,
				nMesh,
				dwMeshFlags,
				pState,
				NULL,
				dwMeshFlagsDoDraw,
				dwMeshFlagsDontDraw ) )
				continue;
		}

		MATERIAL * pMaterial = nMaterialID != INVALID_ID ? (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID ) : NULL;
		if ( ! pMaterial )
			continue;

		if ( pMaterial->dwFlags & MATERIAL_FLAG_NO_CAST_SHADOW )
			continue;

		D3D_EFFECT * pMeshEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
		V_CONTINUE( dxC_ToolLoadShaderNow( pEffect, pMaterial, nShaderType ) );
		if ( !pMeshEffect )
			continue;

		// don't cast shadows from this mesh
		if ( 0 == ( pMeshEffect->dwFlags & EFFECT_FLAG_CASTSHADOW ) )
			continue;

		// set the vertex buffer
		ASSERT_CONTINUE( pMesh->nVertexBufferIndex < pModelDefinition->nVertexBufferDefCount );
		ASSERT_CONTINUE( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];
		if ( ! dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) )
			continue;

		D3D_PROFILE_REGION_INT( L"Mesh", nMesh );

		// does the model have bones?  if not, use the rigid version of the simple shadow shader
		float * pfBones = e_ModelGetBones ( pModel->nId, nMesh, nLOD );

		EFFECT_TECHNIQUE * pTechnique;
		if ( pfBones && pMeshEffect->dwFlags & EFFECT_FLAG_ANIMATED )
		{
			pTechnique = pTechniqueAnimated;
		} 
		else 
		{
			switch ( pVertexBufferDef->eVertexType )
			{
			case VERTEX_DECL_RIGID_16:
				pTechnique = pTechniqueRigid16;
				break;

			case VERTEX_DECL_RIGID_32:
			case VERTEX_DECL_RIGID_64:
				pTechnique = pTechniqueRigid3264;
				break;

			case VERTEX_DECL_ANIMATED:
				pTechnique = pTechniqueAnimated;
				break;

			default: 
				UNDEFINED_CODE_X( "This vertex format is not supported for shadow map rendering!" );
				return E_FAIL;
			}
		}

		UINT nPassCount;
		V_CONTINUE( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );

		if ( pMesh->nVertexBufferIndex != nVertexBufferIndexPrevious )
		{
#if 1
			D3D_VERTEX_BUFFER_DEFINITION tVBDef = ZERO_VB_DEF;
			switch ( pVertexBufferDef->eVertexType )
			{
			case VERTEX_DECL_RIGID_16:
				tVBDef.eVertexType = VERTEX_DECL_R16_POS_TEX;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				tVBDef.nD3DBufferID[ 1 ] = pVertexBufferDef->nD3DBufferID[ 1 ];
				break;

			case VERTEX_DECL_RIGID_32:
			case VERTEX_DECL_RIGID_64:
				tVBDef.eVertexType = VERTEX_DECL_R3264_POS_TEX;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				tVBDef.nD3DBufferID[ 1 ] = pVertexBufferDef->nD3DBufferID[ 1 ];
				break;

			case VERTEX_DECL_ANIMATED:
				tVBDef.eVertexType = VERTEX_DECL_ANIMATED_POS_TEX;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				tVBDef.nD3DBufferID[ 1 ] = pVertexBufferDef->nD3DBufferID[ 1 ];
				break;

			default:
				UNDEFINED_CODE_X( "This vertex format is not supported for shadow map rendering!" );
				return E_FAIL;
			}	
			V_CONTINUE( dxC_SetStreamSource( tVBDef, 0, pEffect ) );
#else
			V_CONTINUE( dxC_SetStreamSource( *pVertexBufferDef, 0 ) );
#endif
			nVertexBufferIndexPrevious = pMesh->nVertexBufferIndex;
		}

		ASSERT_CONTINUE( nPassCount == 1 );
		V_CONTINUE( dxC_EffectBeginPass( pEffect, 0 ) );

		//Set our matrices, the preshader will take care of concatenating the transforms
		V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDMATRIX, (D3DXMATRIXA16*)pmatWorldCurrent ) );
		V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_VIEWMATRIX, (D3DXMATRIXA16*)&pShadow->mShadowView ) );

		int nShadowType = e_GetActiveShadowType();
		static BOOL bSupportsDepthBias = dx9_CapGetValue( DX9CAP_SLOPESCALEDEPTHBIAS ) && dx9_CapGetValue( DX9CAP_DEPTHBIAS );
		if ( AppIsHellgate() && nShadowType == SHADOW_TYPE_DEPTH && bSupportsDepthBias )
		{
			if (   TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC )
				&& dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
			{
				// Don't set depth bias for animated models that render to the 
				// dynamic shadow buffer
				dxC_SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, (DWORD)0 );
				dxC_SetRenderState( D3DRS_DEPTHBIAS, (DWORD)0 );
			}
			else
			{
				// bias = ( m * D3DRS_SLOPESCALEDEPTHBIAS ) + D3DRS_DEPTHBIAS,
				// where m is the maximum depth slope of the triangle being rendered.
				// m = max( abs( delta z / delta x ), abs( delta z / delta y) )
				static float fSlopeScaleDepthBias = 1.7f;
				static float fDepthBias = DXC_9_10( 0.001f, 100000.f );
				DWORD dwSlopeScaleDepthBias = *(DWORD*)&fSlopeScaleDepthBias;
				DX9_BLOCK( DWORD dwDepthBias = *(DWORD*)&fDepthBias );
				DX10_BLOCK( DWORD dwDepthBias = fDepthBias ); // DX10 uses int for depth bias instead of F2DW.
				V( dxC_SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, dwSlopeScaleDepthBias ) );
				V( dxC_SetRenderState( D3DRS_DEPTHBIAS, dwDepthBias ) );
			}

			V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_PROJECTIONMATRIX, (D3DXMATRIXA16*)&pShadow->mShadowProj ) );
		}
		else
		{
			if ( AppIsTugboat() )
			{
				V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_PROJECTIONMATRIX, (D3DXMATRIXA16*)&pShadow->mShadowProj ) );
			}
			else
			{
				if (   TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC )
					&& dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
				{
					V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_PROJECTIONMATRIX, (D3DXMATRIXA16*)&pShadow->mShadowProj ) );
				}
				else
				{
					V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_PROJECTIONMATRIX, (D3DXMATRIXA16*)&pShadow->mShadowProjBiased ) );
				}
			}
		}

		// change alpha test, if necessary
		BOOL bMeshAlphaTest = FALSE;
		if ( dxC_MeshDiffuseHasAlpha( pModel, pModelDefinition, pMesh, nMesh, nLOD ) )
		{
			// set alpha test, if specified for both effect and mesh
			EFFECT_DEFINITION * pEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pMeshEffect->nEffectDefId );
			ASSERT_CONTINUE( pEffectDef );
			bMeshAlphaTest = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_TEST );
			V( dxC_SetTexture(
				pMesh->pnTextures[ TEXTURE_DIFFUSE ],
				*pTechnique,
				pEffect,
				EFFECT_PARAM_DIFFUSEMAP0,
				INVALID_ID,
				NULL,
				NULL,
				TEXTURE_DIFFUSE,
				FALSE ) );
		}
		else
		{
			V( dxC_SetTexture(
				e_GetUtilityTexture( TEXTURE_RGBA_FF ),
				*pTechnique,
				pEffect,
				EFFECT_PARAM_DIFFUSEMAP0,
				INVALID_ID ) );
		}
		V( dxC_SetAlphaTestEnable( bMeshAlphaTest ) );

		if (TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER ))	// CHB 2007.02.17
		{
			dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED );
		}
		
		V( dxC_SetBonesEx( pModel, nMesh, pEffect, *pTechnique, pfBones ) );

		// set the index buffer
		//if ( 0 == pMesh->tIndexBufferDef.nIndexCount )
		//	continue;
		V_CONTINUE( dxC_SetIndices( pMesh->tIndexBufferDef ) );

		TRACE_MODEL_INT( pModel->nId, "Draw Primitive", nMesh );

		nDebugRenders++;

		//trace("Shadow dp: model %4d, mesh %4d, modeldef %4d, %s\n", nModelID, nMesh, pModelDefinition->tHeader.nId, pModelDefinition->tHeader.pszName );
		dxC_SetRenderState( D3DRS_ZENABLE, true );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, true );

		dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		// loop through all of the passes for the effect
		if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, pMesh->wTriangleCount, pEffect, METRICS_GROUP_SHADOW ) );
		} else {
			V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, pMesh->wTriangleCount, pEffect, METRICS_GROUP_SHADOW ) );
		}
	}

	//if ( !TEST_MASK( pModel->dwFlags, MODEL_FLAG_ANIMATED ) )
	//	dxC_SaveTextureToFile( "test.dds", D3DX10_IFF_DDS, dxC_RTResourceGet( dxC_GetCurrentRenderTarget() ) );

	//if( false )
	//	dxC_SaveTextureToFile( "test.dds", D3DX10_IFF_DDS, dxC_RTResourceGet( dxC_GetCurrentRenderTarget() ) );

	//if ( nDebugRenders == 0 )
	//{
	//	int a=0;
	//} else
	//{
	//	trace("ShadowModel: model %4d, dps %4d, modeldef %4d, %s\n", nModelID, nDebugRenders, pModelDefinition->tHeader.nId, pModelDefinition->tHeader.pszName );
	//}

	// restore alpha test to off
	V( dxC_SetAlphaTestEnable( FALSE ) );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );


	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderModel (
	int nDrawList,
	int nModelID )
{
	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	return dx9_RenderModel( pState, nModelID );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2006.08.09 - Define USE_SIMULATED_LOD to draw a fraction of the
// triangles, which can be useful for simulating reduced geometry.
//#define USE_SIMULATED_LOD 1
#ifdef USE_SIMULATED_LOD
float g_fGeometryReductionFactor = 0.1f;
#endif

static PRESULT sRenderModel (
	const DRAWLIST_STATE * const NOALIAS pState,
	int nModelID,
	MATERIAL_OVERRIDE_TYPE eMatOverrideType /*= MATERIAL_OVERRIDE_NORMAL*/ )
{
	PERF( DRAW_MODEL )

	//KEYTRACE_SETLOCALKEY( VK_INSERT );

	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG(pModel);

	int nModelDefinitionId = pModel->nModelDefinitionId;
	int nLOD = dxC_ModelGetDisplayLODWithDefault( pModel );
	if ( nModelDefinitionId == INVALID_ID )
		return S_FALSE;

	if ( !pState->bAllowInvisibleModels )
		if ( ! dx9_ModelShouldRender( pModel ) )
			return S_FALSE;

	// temp workaround for cube map gen
	if ( e_GeneratingCubeMap() && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
		return S_FALSE;

	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, nLOD );
	if ( ! pModelDefinition )
		return S_FALSE;

	D3D_PROFILE_REGION_INT_STR( L"Model", nModelID, pModelDefinition->tHeader.pszName );

	if ( e_GetRenderFlag( RENDER_FLAG_RENDER_BOUNDING_SPHERE ) )
	{
		BOUNDING_SPHERE tBS( pModel->tRenderAABBWorld );
		V( e_PrimitiveAddSphere( 0, &tBS, 0xff00ff3f ) );
	}

	//int nRoomID = e_ModelGetRoomID( pModel->nId ); // used for ambient and directional lights

	MATRIX * pmatWorldCurrent;
	MATRIX * pmatViewCurrent;
	MATRIX * pmatProjCurrent;
	MATRIX matWorldView;
	MATRIX matWorldViewProjection;
	VECTOR4 vEyeLocationInWorld;
	float fDistanceToEyeSquared;

	V_RETURN( sGetMatrices(
		pModel,
		pState,
		pmatWorldCurrent,
		pmatViewCurrent,
		pmatProjCurrent,
		&matWorldView,
		&matWorldViewProjection,
		&fDistanceToEyeSquared,
		vEyeLocationInWorld ) );


	// CHB 2006.11.30
#ifdef USE_SIMULATED_LOD
	float fLODSimulationFactor = (nLOD == LOD_LOW) ? g_fGeometryReductionFactor : 1.0f;
#endif

	// to make fog work correctly
	//MatrixMakeWFriendlyProjection( pmatProjCurrent );
	V( dxC_SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)pmatProjCurrent ) );
	//e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );

	BOOL bForceCurrentEnv = FALSE;
	ENVIRONMENT_DEFINITION * pEnvDef;

	V_RETURN( sGetEnvironment(
		pState,
		pModel,
		pEnvDef,
		bForceCurrentEnv ) );
	if ( ! pEnvDef )
		return S_FALSE;


	DWORD dwMeshFlagsDoDraw   = pState->dwMeshFlagsDoDraw;
	DWORD dwMeshFlagsDontDraw = pState->dwMeshFlagsDontDraw;
	BOOL bLights = pState->bLights;

	V( sUpdateMeshDrawOptions(
		dwMeshFlagsDoDraw,
		dwMeshFlagsDontDraw,
		bLights,
		fDistanceToEyeSquared,
		pEnvDef,
		bForceCurrentEnv ) );

	float fModelAlpha = pModel->fAlpha * pModel->fDistanceAlpha;

	MODEL_RENDER_LIGHT_DATA & tLights = pModel->tLights;
	tLights.SetUpdateRate( 20.f );
	tLights.CheckUpdate();
	//tLights.Clear();

	if ( bLights )
	{
		// have to select these early to get a count for effect technique selection

		//// point lights selected inside effect/technique selection routine
		//V( sSelectLightsSpecularOnly( pModel, pState, tLights ) );
	}
	else
	{
		//tLights.dwStateFlags |= MODEL_RENDER_LIGHT_DATA::STATE_NO_DYNAMIC_LIGHTS;
	}

	PLANE tPlanes[ NUM_FRUSTUM_PLANES ];
	if ( AppIsHellgate() )
	{
		if ( pModel->tModelDefData[nLOD].ptMeshRenderOBBWorld )	// CHB 2006.12.08
			e_MakeFrustumPlanes( tPlanes, (MATRIX*)pmatViewCurrent, (MATRIX*)pmatProjCurrent );

		if ( pState->bNoDepthTest )
			dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	}

	UINT nPassCount = 0;

	if ( AppIsTugboat() )
	{
		dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) && e_GetRenderFlag( RENDER_FLAG_USE_ZWRITE ) );
		dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	}
	//keytrace( "### MODEL %3d, 0x%8x:0x%8x\n", pModel->nId, dwMeshFlagsDoDraw, dwMeshFlagsDontDraw );

	//CC@HAVOK
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
	bool gpuEmittedParticlesNeedAdvancement = false;
	int nParticleSystemId = INVALID_ID;
	//D3D_EFFECT* pTempEffect; //temporary until shader is split into 3 phases
#endif

	int nIsolateMesh = INVALID_ID;
	if ( e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO ) == nModelID && e_GetRenderFlag( RENDER_FLAG_ISOLATE_MESH ) )
	{
		nIsolateMesh = dxC_DebugTrackMesh( pModel, pModelDefinition );
	}

	int nLocation = e_GetCurrentLevelLocation();

	// Iterate through the meshes and draw them
	for ( int nMesh = 0; nMesh < pModelDefinition->nMeshCount; nMesh++ )
	{
#ifdef FXC_REPRO
		if ( nMesh != 7 )
			continue;
#endif

		if ( nIsolateMesh != INVALID_ID && nIsolateMesh != nMesh )
			continue;

		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDefinition->pnMeshIds [ nMesh ] );
		ASSERT_CONTINUE( pMesh );

		V_CONTINUE( dx9_MeshApplyMaterial( pMesh, pModelDefinition, pModel, nLOD, nMesh ) );	// CHB 2006.12.08

		DWORD dwMeshFlags = pMesh->dwFlags;
		int nShaderType = dx9_GetCurrentShaderType( pEnvDef, pState, pModel );

		int nMaterialID = dx9_MeshGetMaterial( pModel, pMesh, eMatOverrideType );
		if ( nMaterialID != pMesh->nMaterialId )
		{
			EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialID, nShaderType );
			if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
				dwMeshFlags |= MESH_FLAG_ALPHABLEND;
		}

		{
			if ( sMeshCullRender(
				pModel,
				pMesh,
				nMesh,
				dwMeshFlags,
				pState,
				tPlanes,
				dwMeshFlagsDoDraw,
				dwMeshFlagsDontDraw ) )
				continue;
		}

		PERF( DRAW_MESH )
			D3D_PROFILE_REGION_INT( L"Mesh", nMesh );

		D3D_EFFECT * pEffect;
		EFFECT_DEFINITION * pEffectDef = NULL;
		EFFECT_TECHNIQUE* ptTechnique = NULL;
		float fEffectTransitionStr;
		D3D_VERTEX_BUFFER * pVertexBuffer;
		MATERIAL * pMaterial;

		pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID );
		if (!pMaterial)
		{
			continue;
		}

		// CHB 2006.12.21 - Make sure we have a diffuse texture.
		// Other code makes the assumption that existence of the
		// material means the diffuse texture has been loaded.
		// For example, see dx9_MeshApplyMaterial().
		// The material is created by
		//	dxC_DiffuseTextureDefinitionLoadedCallback(),
		// which is called when either the high or low LOD diffuse
		// texture has been loaded. If both textures use the same
		// material, the material will be created when either is
		// loaded. Thus other code will take this to mean the
		// diffuse texture is ready, when in fact it is ready for
		// one LOD and not the other.
		{
			int nTextureId = dx9_ModelGetTexture(pModel, pModelDefinition, pMesh, nMesh, TEXTURE_DIFFUSE, nLOD, false, pMaterial);
			if (nTextureId == INVALID_ID)
			{
				continue;
			}
		}

		{
			PERF( DRAW_SETTINGS )

			{
				PERF( DRAW_FX_PICK )

					//keytrace( "GetEffect: %3d:%-2d %d\n", pModel->nId, nMesh, nShaderType );

				V_CONTINUE( sGetEffectAndTechnique(
					pModel,
					pModelDefinition,	// CHB 2007.09.25
					pMesh,
					nMesh,
					pState,
					pMaterial,
					dwMeshFlagsDoDraw,
					tLights,
					fDistanceToEyeSquared,
					pEnvDef,
					fEffectTransitionStr,
					nShaderType,
					&pEffect,
					&ptTechnique,
					nLOD) );	// CHB 2006.11.28

				if ( !pEffect )
					continue;

				V_CONTINUE( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );

				if ( pEffect->nEffectDefId != INVALID_ID )
					pEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pEffect->nEffectDefId );

#ifdef DX10_MODEL_HAS_NO_SHADER_DEF_HACK
				if( !pEffectDef )
				{
					//LogMessage( ERROR_LOG, "Mesh is using %s but doesn't have a valid effect definition!", pEffect->pszFXFileName );
					continue;
				}
#else
				ASSERT_CONTINUE( pEffectDef );
#endif
			}


			BOOL bStateFromEffect = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_STATE_FROM_EFFECT );
#ifdef ENGINE_TARGET_DX10
			bStateFromEffect = TRUE;
#endif

			// loop through all of the passes for the effect
			for( UINT nPass = 0; nPass < nPassCount; nPass ++ ) 
			{
				if ( ! bStateFromEffect )
				{
					V_CONTINUE( dxC_EffectBeginPass( pEffect, nPass, pEffectDef ) );
				}

				{
					PERF( DRAW_VBSETUP )
						if ( S_OK != sSetMeshVertexParameters( pModelDefinition, pMesh, pEffect, pEffectDef, pVertexBuffer ) )
							continue;
				}

				ASSERT_CONTINUE( nPassCount > 0 );

				{

					BOOL bAlphaAsLighting;

					V_CONTINUE( sSetGeneralMeshStates(
						pModel,
						pModelDefinition,
						pMesh,
						nMesh,
						nLOD,
						pState,
						pEffect,
						pEffectDef,
						*ptTechnique,
						pMaterial,
						bAlphaAsLighting ) );

					V_CONTINUE( sSetGeneralMeshParameters(
						pModel,
						pMesh,
						nMesh,
						nLOD,
						pState,
						pmatWorldCurrent,
						pmatViewCurrent,
						pmatProjCurrent,
						&matWorldViewProjection,
						&vEyeLocationInWorld,
						pEffect,
						*ptTechnique,
						pMaterial,
						pEnvDef,
						bForceCurrentEnv,
						bAlphaAsLighting, 
						fModelAlpha ) );

					V_CONTINUE( sSetMaterialParameters(
						pModel,
						pModelDefinition,
						nLOD,	// CHB 2006.12.08
						pMesh,
						nMesh,
						pEffect,
						pEffectDef,
						*ptTechnique,
						pMaterial,
						pEnvDef,
						fEffectTransitionStr,
						bForceCurrentEnv ) );

					V_CONTINUE( sSetLightingEffectParameters(
						pModel,
						pState,
						pEffect,
						pEffectDef,
						*ptTechnique,
						fEffectTransitionStr,
						pEnvDef,
						tLights ) );

					V_CONTINUE( sSetMeshTextures(
						pModel,
						nMesh,
						pMesh,
						pModelDefinition,
						pEffect,
						*ptTechnique,
						pMaterial,
						pEnvDef,
						nLOD) );	// CHB 2006.11.28
				}

				if ( bStateFromEffect )
				{
					V_CONTINUE( dxC_EffectBeginPass( pEffect, nPass, pEffectDef ) );
				}

				if ( pEffect->dwFlags & EFFECT_FLAG_STATICBRANCH )
				{
					DWORD dwBranches;
					V( dx9_SetStaticBranchParamsFlags( dwBranches, pMaterial, pModel, pModelDefinition, pMesh, nMesh, pEffect, *ptTechnique, nShaderType, pEnvDef, nLOD ) );	// CHB 2006.11.28
					V( dx9_SetStaticBranchParamsFromFlags( pEffect, *ptTechnique, dwBranches ) );
				}

				// do here the render/texture state changes that override effect defaults
				if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
					 dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
					dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

				TRACE_MODEL_INT( pModel->nId, "Draw Primitive", nMesh );

				BOOL bAnimated = pModel->nAppearanceDefId != INVALID_ID || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED );
				METRICS_GROUP eGroup = bAnimated ? METRICS_GROUP_ANIMATED : METRICS_GROUP_BACKGROUND;

				UINT nTriangles = pMesh->wTriangleCount;
#ifdef USE_SIMULATED_LOD
				nTriangles = static_cast<UINT>(nTriangles * fLODSimulationFactor);
				nTriangles = max(nTriangles, 1);
				nTriangles = min(nTriangles, pMesh->wTriangleCount);
#endif

				/////
				//CC@HAVOK
				//
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
				//GET ATTACHMENET
				int nAttachmentId = dx9_ModelGetMaterialOverrideAttachment( pModel, eMatOverrideType );
				if ( nAttachmentId != INVALID_ID )
				{
					//SEE IF IT HAS A PARTICLE SYSTEM
					int nParticleSystemIdTemp = gpfn_AttachmentGetAttachedById( pModel->nId, nAttachmentId );
					if ( nParticleSystemIdTemp != INVALID_ID )
					{

						PARTICLE_SYSTEM_DEFINITION* def;
						def = (PARTICLE_SYSTEM_DEFINITION*)DefinitionGetById(DEFINITION_GROUP_PARTICLE,nParticleSystemIdTemp);

						//SEE IF MATERIAL SHADER WANTS TO EMIT PARTICLES
						if( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_EMITS_GPU_PARTICLES ) )
						{

							V( dx10_ParticleSystemPrepareEmitter(pEffect, nParticleSystemIdTemp) );
							nPassCount = 1;

							gpuEmittedParticlesNeedAdvancement = true;
							nParticleSystemId = nParticleSystemIdTemp;
							//pTempEffect = pEffect;
						}
					}
				}
#endif
				///////
				// CC@HAVOK
				// end

				if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
				{
					V_CONTINUE( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, nTriangles, pEffect, eGroup ) );
				} else {
					V_CONTINUE( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, nTriangles, pEffect, eGroup ) );
				}

				MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pModelDefinition->tHeader.nId );
				if ( pHash->tEngineRes.IsTracked() )
				{
					float fDistance = sqrtf( fDistanceToEyeSquared );
					if ( fDistance < pHash->tEngineRes.fDistance || pHash->tEngineRes.fDistance < 0.f )
						pHash->tEngineRes.fDistance = fDistance;
					gtReaper.tModels.Touch( &pHash->tEngineRes );
				}
			}
		}

		pMesh->dwFlags |= MESH_FLAG_WARNED; // we have given all of the warnings on this mesh
	}

	// CC@HAVOK
	//
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
	if(gpuEmittedParticlesNeedAdvancement)
	{
		V( dx10_ParticleSystemGPUAdvance(
			nParticleSystemId ) );
	}
#endif
	// CC@HAVOK
	// end
	//




	// restore two-sided rendering to off
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	// restore alpha test to off
	dxC_SetAlphaTestEnable( FALSE );

	// restore alpha blend to off
	dxC_SetAlphaBlendEnable( FALSE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );

	// reset WRAP0 (only the poison shader uses this currently)
	DX9_BLOCK( dxC_SetRenderState( D3DRS_WRAP0, (DWORD)0 ); ) //NUTTAPONG TODO: Get wrap modes working



		// CC@HAVOK
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
		dxC_GetDevice()->GSSetShader(NULL);
	//dxC_GetDevice()->SOSetTargets(0, NULL, 0);
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelDrawPrepare(
	RENDER_CONTEXT & tContext,
	ModelDraw & tModelDraw )
{
	//PERF( DRAW_MODEL_PREPARE );
	//HITCOUNT( DRAW_MODEL_PREPARE );
	//KEYTRACE_SETLOCALKEY( VK_INSERT );

	if ( TESTBIT_DWORD( tModelDraw.dwFlagBits, ModelDraw::FLAGBIT_DATA_COMPLETE ) )
		return S_OK;

	ASSERT_RETFAIL( TESTBIT_DWORD( tModelDraw.dwFlagBits, ModelDraw::FLAGBIT_DATA_INITIALIZED ) );

	tModelDraw.pModel = dx9_ModelGet( tModelDraw.nModelID );
	ASSERT_RETINVALIDARG( tModelDraw.pModel );

	tModelDraw.nModelDefinitionId = tModelDraw.pModel->nModelDefinitionId;
	//tModelDraw.nLOD = dxC_ModelGetDisplayLODWithDefault( tModelDraw.pModel );
	ASSERT_RETFAIL( tModelDraw.nModelDefinitionId != INVALID_ID );
	tModelDraw.pModelDefinition = dxC_ModelDefinitionGet( tModelDraw.nModelDefinitionId, tModelDraw.nLOD );
	ASSERT_RETFAIL( tModelDraw.pModelDefinition );

	MATRIX* pmatWorldCurrent;
	MATRIX* pmatViewCurrent;
	MATRIX* pmatProjCurrent;
	//MATRIX matWorldView;
	//MATRIX matWorldViewProjection;
	VECTOR4 vEyeLocationInWorld;


	V_RETURN( sGetMatrices(
		tModelDraw.pModel,
		&tContext.tState,
		pmatWorldCurrent,
		pmatViewCurrent,
		pmatProjCurrent,
		&tModelDraw.mWorldView,
		&tModelDraw.mWorldViewProjection,
		NULL,		// dist to eye sq already calculated
		tModelDraw.vEye_w ) );

	tModelDraw.mWorld = *pmatWorldCurrent;
	tModelDraw.mView  = *pmatViewCurrent;
	tModelDraw.mProj  = *pmatProjCurrent;

	tModelDraw.dwMeshFlagsDoDraw   = tContext.tState.dwMeshFlagsDoDraw;
	tModelDraw.dwMeshFlagsDontDraw = tContext.tState.dwMeshFlagsDontDraw;
	BOOL bLights = tContext.tState.bLights;

	V( sUpdateMeshDrawOptions(
		tModelDraw.dwMeshFlagsDoDraw,
		tModelDraw.dwMeshFlagsDontDraw,
		bLights,
		tModelDraw.fDistanceToEyeSquared,
		tContext.pEnvDef,
		tContext.bForceCurrentEnv ) );

	tModelDraw.fModelAlpha = tModelDraw.pModel->fAlpha * tModelDraw.pModel->fDistanceAlpha;


	if ( bLights )
	{
		// have to select these early to get a count for effect technique selection
		// point lights selected inside effect/technique selection routine
		//V( sSelectLightsSpecularOnly( pModel, pState, tLights ) );

		tModelDraw.pModel->tLights.SetUpdateRateByScreensize( tModelDraw.fScreensize );
		tModelDraw.pModel->tLights.CheckUpdate();
	}
	else
	{
		//tModelDraw.tLights.dwFlags |= MODEL_RENDER_LIGHT_DATA::STATE_NO_DYNAMIC_LIGHTS;
	}

	//if ( AppIsHellgate() )
	{
		if ( tModelDraw.pModel->tModelDefData[tModelDraw.nLOD].ptMeshRenderOBBWorld )	// CHB 2006.12.08
			e_MakeFrustumPlanes( tModelDraw.tPlanes, &tModelDraw.mView, &tModelDraw.mProj );
	}

	//CC@HAVOK
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
	bool gpuEmittedParticlesNeedAdvancement = false;
	int nParticleSystemId = INVALID_ID;
	//D3D_EFFECT* pTempEffect; //temporary until shader is split into 3 phases
#endif

#if ISVERSION(DEVELOPMENT)
	tModelDraw.nIsolateMesh = INVALID_ID;
	if ( e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO ) == tModelDraw.nModelID && e_GetRenderFlag( RENDER_FLAG_ISOLATE_MESH ) )
	{
		tModelDraw.nIsolateMesh = dxC_DebugTrackMesh( tModelDraw.pModel, tModelDraw.pModelDefinition );
	}
#endif

	MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( tModelDraw.pModelDefinition->tHeader.nId );
	if ( pHash->tEngineRes.IsTracked() )
	{
		float fDistance = sqrtf( tModelDraw.fDistanceToEyeSquared );
		if ( fDistance < pHash->tEngineRes.fDistance || pHash->tEngineRes.fDistance < 0.f )
			pHash->tEngineRes.fDistance = fDistance;
		gtReaper.tModels.Touch( &pHash->tEngineRes );
	}

	SETBIT( tModelDraw.dwFlagBits, ModelDraw::FLAGBIT_DATA_COMPLETE, TRUE );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelDrawAddMesh(
	MeshList & tMeshList,
	RENDER_CONTEXT & tContext,
	ModelDraw & tModelDraw,
	int nMesh,
	MATERIAL_OVERRIDE_TYPE eMatOverrideType,
	DWORD dwPassFlags
	)
{
	ASSERT_RETINVALIDARG( TESTBIT_DWORD( tModelDraw.dwFlagBits, ModelDraw::FLAGBIT_DATA_COMPLETE ) );

#if ISVERSION(DEVELOPMENT)
	if ( tModelDraw.nIsolateMesh != INVALID_ID && tModelDraw.nIsolateMesh != nMesh )
		return S_FALSE;
#endif

	D3D_MESH_DEFINITION* pMesh = dx9_MeshDefinitionGet( tModelDraw.pModelDefinition->pnMeshIds [ nMesh ] );
	ASSERT_RETFAIL( pMesh );

	V_RETURN( dx9_MeshApplyMaterial( pMesh, tModelDraw.pModelDefinition, tModelDraw.pModel, tModelDraw.nLOD, nMesh ) );	// CHB 2006.12.08

	DWORD dwMeshFlags = pMesh->dwFlags;
	int nShaderType = dx9_GetCurrentShaderType( tContext.pEnvDef, &tContext.tState, tModelDraw.pModel );
	int nMaterialID = dx9_MeshGetMaterial( tModelDraw.pModel, pMesh, eMatOverrideType );

	{
		if ( sMeshCullRender(
				tModelDraw.pModel,
				pMesh,
				nMesh,
				dwMeshFlags,
				&tContext.tState,
				tModelDraw.tPlanes,
				tModelDraw.dwMeshFlagsDoDraw,
				tModelDraw.dwMeshFlagsDontDraw,
				tModelDraw.nLOD ) )
			return S_FALSE;
	}

	PERF( DRAW_MESH );

	D3D_EFFECT* pEffect;
	EFFECT_DEFINITION* pEffectDef = NULL;
	EFFECT_TECHNIQUE* ptTechnique = NULL;
	float fEffectTransitionStr = 0;
	MATERIAL* pMaterial;

	pMaterial = (MATERIAL*)DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID );
	if ( !pMaterial )
	{
		return S_FALSE;
	}

	// CHB 2006.12.21 - Make sure we have a diffuse texture.
	// Other code makes the assumption that existence of the
	// material means the diffuse texture has been loaded.
	// For example, see dx9_MeshApplyMaterial().
	// The material is created by
	//	dxC_DiffuseTextureDefinitionLoadedCallback(),
	// which is called when either the high or low LOD diffuse
	// texture has been loaded. If both textures use the same
	// material, the material will be created when either is
	// loaded. Thus other code will take this to mean the
	// diffuse texture is ready, when in fact it is ready for
	// one LOD and not the other.
	{
		int nTextureId = dx9_ModelGetTexture(
							tModelDraw.pModel,
							tModelDraw.pModelDefinition,
							pMesh,
							nMesh,
							TEXTURE_DIFFUSE,
							tModelDraw.nLOD,
							false,
							pMaterial);
		if (nTextureId == INVALID_ID)
		{
			return S_FALSE;
		}
	}


	if ( dwPassFlags & PASS_FLAG_DEPTH )
	{
		// depth-only pass
		BOOL bAnimated = tModelDraw.pModel->nAppearanceDefId != INVALID_ID || dxC_ModelGetFlagbit( tModelDraw.pModel, MODEL_FLAGBIT_ANIMATED );

		pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_ZBUFFER ) );

		ASSERT_RETFAIL( pEffect );
		if ( ! dxC_EffectIsLoaded( pEffect ) )
			return S_FALSE;

		EFFECT_TECHNIQUE * pTechniqueDepth = NULL;
		TECHNIQUE_FEATURES tFeat;


		//This should be revisited it's hacktastic
		{
			int nIndex = 0;

			if( tModelDraw.pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ].eVertexType == VERTEX_DECL_RIGID_16 )
				nIndex = 1;
	
			else if ( bAnimated )
				nIndex = 2 + !!( pMesh->dwFlags & MESH_FLAG_SKINNED );


			tFeat.Reset();
			tFeat.nInts[ TF_INT_INDEX ] = nIndex;
			V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechniqueDepth ) );
			ASSERT_RETFAIL( pTechniqueDepth->hHandle );
		}

		ptTechnique = pTechniqueDepth;
	}
	else if ( dwPassFlags & PASS_FLAG_BEHIND )
	{
		// Mythos render-behind pass
		BOOL bAnimated = tModelDraw.pModel->nAppearanceDefId != INVALID_ID || dxC_ModelGetFlagbit( tModelDraw.pModel, MODEL_FLAGBIT_ANIMATED );
		if( tModelDraw.pModelDefinition->nMeshCount > 0 )
		{
			D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( tModelDraw.pModelDefinition->pnMeshIds [ 0 ] );
			ASSERT_RETFAIL( pMesh );
			if( !( pMesh->dwFlags & MESH_FLAG_SKINNED ) )
			{
				bAnimated = FALSE;
			}
		}
	
		pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_OCCLUDED ) );

		ASSERT_RETFAIL( pEffect );
		if ( ! dxC_EffectIsLoaded( pEffect ) )
			return S_FALSE;

		TECHNIQUE_FEATURES tFeat;
		tFeat.Reset();

		tFeat.nInts[ TF_INT_INDEX ] = bAnimated ? 0 : 1;
		V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
		ASSERT_RETFAIL( ptTechnique );
		ASSERT_RETFAIL( ptTechnique->hHandle );
	}
	else
	{
		// normal pass
		{
			PERF( DRAW_FX_PICK );

			//keytrace( "GetEffect: %3d:%-2d %d\n", pModel->nId, nMesh, nShaderType );
			V_RETURN( sGetEffectAndTechnique(
				tModelDraw.pModel,
				tModelDraw.pModelDefinition,	// CHB 2007.09.25
				pMesh,
				nMesh,
				&tContext.tState,
				pMaterial,
				tModelDraw.dwMeshFlagsDoDraw,
				tModelDraw.pModel->tLights,
				tModelDraw.fDistanceToEyeSquared,
				tContext.pEnvDef,
				fEffectTransitionStr,
				nShaderType,
				&pEffect,
				&ptTechnique,
				tModelDraw.nLOD ) );	// CHB 2006.11.28

			if ( !pEffect )
				return S_FALSE;

			if ( pEffect->nEffectDefId != INVALID_ID )
				pEffectDef = (EFFECT_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pEffect->nEffectDefId );

#ifdef DX10_MODEL_HAS_NO_SHADER_DEF_HACK
			if( !pEffectDef )
			{
				//LogMessage( ERROR_LOG, "Mesh is using %s but doesn't have a valid effect definition!", pEffect->pszFXFileName );
				return S_FALSE;
			}
#else
			ASSERT_RETFAIL( pEffectDef );
#endif
		}

		sComputeLightsSH( tModelDraw.pModel, pEffect, *ptTechnique, tModelDraw.pModel->tLights );
	}

	if ( ( e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL ) != INVALID_ID )
		&& ( e_GetRenderFlag( RENDER_FLAG_MODEL_RENDER_INFO ) != INVALID_ID )
		&& ( gtDebugMeshRenderInfo.nMeshIndex != nMesh ) )
		return S_FALSE;

	FSSE::MeshDraw tMeshDraw;
	tMeshDraw.Clear();
	tMeshDraw.pMesh					= pMesh;
	tMeshDraw.pModel				= tModelDraw.pModel;
	tMeshDraw.pModelDef				= tModelDraw.pModelDefinition;
	tMeshDraw.nMeshIndex			= nMesh;
	tMeshDraw.nLOD					= tModelDraw.nLOD;
	tMeshDraw.pEnvDef				= tContext.pEnvDef;		
	tMeshDraw.pEffect				= pEffect;
	tMeshDraw.pEffectDef			= pEffectDef;
	tMeshDraw.pTechnique			= ptTechnique;
	tMeshDraw.tState				= tContext.tState;
	tMeshDraw.pMaterial				= pMaterial;
	tMeshDraw.matWorld				= tModelDraw.mWorld;
	tMeshDraw.matView				= tModelDraw.mView;
	tMeshDraw.matProj				= tModelDraw.mProj;
	tMeshDraw.matWorldViewProj		= tModelDraw.mWorldViewProjection;
	tMeshDraw.vEyeInWorld			= tModelDraw.vEye_w;
	tMeshDraw.fAlpha				= tModelDraw.fModelAlpha;
	//tMeshDraw.tLights				= tModelDraw.tLights;
	tMeshDraw.nShaderType			= nShaderType;
	//tMeshDraw.nParticleSystemId		= INVALID_ID;
	tMeshDraw.fEffectTransitionStr	= fEffectTransitionStr;
	// This could be more accurate by calculating the actual distance to the mesh instead of the model.
	tMeshDraw.fDist					= tModelDraw.fDistanceToEyeSquared;
	tMeshDraw.tScissor				= tModelDraw.tScissor;

	SETBIT( tMeshDraw.dwFlagBits, FSSE::MeshDraw::FLAGBIT_FORCE_CURRENT_ENV, tContext.bForceCurrentEnv );
	SETBIT( tMeshDraw.dwFlagBits, FSSE::MeshDraw::FLAGBIT_FORCE_NO_SCISSOR, TESTBIT_DWORD( tModelDraw.dwFlagBits, FSSE::ModelDraw::FLAGBIT_FORCE_NO_SCISSOR ) );
	SETBIT( tMeshDraw.dwFlagBits, FSSE::MeshDraw::FLAGBIT_DEPTH_ONLY, !!( dwPassFlags & PASS_FLAG_DEPTH ) );
	SETBIT( tMeshDraw.dwFlagBits, FSSE::MeshDraw::FLAGBIT_BEHIND, !!( dwPassFlags & PASS_FLAG_BEHIND ) );

	// This currently involves a copy of the tMeshDraw object.  It could be made more lightweight by a global vector of 
	// MODEL_RENDER_LIGHT_DATA objects.
	V_RETURN( dxC_MeshListAddMesh( tMeshList, tMeshDraw ) );
 
	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT dxC_ModelDrawAddMeshes(
	MeshList & tMeshList,
	RENDER_CONTEXT & tContext,
	ModelDraw & tModelDraw )
{
	ASSERT_RETINVALIDARG( TESTBIT_DWORD( tModelDraw.dwFlagBits, ModelDraw::FLAGBIT_DATA_COMPLETE ) );

	// Iterate through the meshes and draw them
	for ( int nMesh = 0; nMesh < tModelDraw.pModelDefinition->nMeshCount; nMesh++ )
	{
		V( dxC_ModelDrawAddMesh( tMeshList, tContext, tModelDraw, nMesh, MATERIAL_OVERRIDE_NORMAL, 0 ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderMesh( MeshDraw & tMeshDraw )
{
	PERF( DRAW_MESH );

	D3D_MESH_DEFINITION* pMesh = tMeshDraw.pMesh;
	D3D_MODEL* pModel = tMeshDraw.pModel;
	D3D_MODEL_DEFINITION* pModelDefinition = tMeshDraw.pModelDef;
	int nMesh = tMeshDraw.nMeshIndex;
	int nLOD = tMeshDraw.nLOD;
	ENVIRONMENT_DEFINITION* pEnvDef = tMeshDraw.pEnvDef;
	DRAWLIST_STATE* pDLState = &tMeshDraw.tState;
	D3D_EFFECT* pEffect = tMeshDraw.pEffect;
	EFFECT_DEFINITION* pEffectDef = tMeshDraw.pEffectDef;
	EFFECT_TECHNIQUE* pTechnique = tMeshDraw.pTechnique;
	MATERIAL* pMaterial = tMeshDraw.pMaterial;	
	D3D_VERTEX_BUFFER* pVertexBuffer;
	BOOL bForceCurrentEnv = TESTBIT_DWORD( tMeshDraw.dwFlagBits, MeshDraw::FLAGBIT_FORCE_CURRENT_ENV );
	BOOL bForceNoScissor = TESTBIT_DWORD( tMeshDraw.dwFlagBits, MeshDraw::FLAGBIT_FORCE_NO_SCISSOR );

	// CHB 2006.11.30
#ifdef USE_SIMULATED_LOD
	float fLODSimulationFactor = (nLOD == LOD_LOW) ? g_fGeometryReductionFactor : 1.0f;
#endif

	ASSERT_RETFAIL( pEnvDef );

	// this should be done once per viewparams
	dxC_SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)&tMeshDraw.matProj );

	V( dxC_SetScissorRect( (const RECT*)&tMeshDraw.tScissor, 0 ) );

	if ( pDLState->bNoDepthTest )
		dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );

	if ( AppIsTugboat() )
	{
		dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) && e_GetRenderFlag( RENDER_FLAG_USE_ZWRITE ) );
		dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	}

	D3D_PROFILE_REGION_INT_STR( L"Model", pModel->nId, pModelDefinition->tHeader.pszName );
	{
		PERF( DRAW_SETTINGS );
		D3D_PROFILE_REGION_INT( L"Mesh", nMesh );

		BOOL bStateFromEffect = DXC_9_10( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_STATE_FROM_EFFECT ), TRUE );

		int nLocation = e_GetCurrentLevelLocation();
		
		UINT nPassCount;
		V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );

		// loop through all of the passes for the effect
		for( UINT nPass = 0; nPass < nPassCount; nPass ++ ) 
		{
			if ( ! bStateFromEffect )
			{
				V_CONTINUE( dxC_EffectBeginPass( pEffect, nPass, pEffectDef ) );
			}

			{
				PERF( DRAW_VBSETUP );
				if ( S_OK != sSetMeshVertexParameters( pModelDefinition, pMesh, pEffect, pEffectDef, pVertexBuffer ) )
					continue;
			}

			ASSERT_CONTINUE( nPassCount > 0 );

			{
				BOOL bAlphaAsLighting;

				V_CONTINUE( sSetGeneralMeshStates(
					pModel,
					pModelDefinition,
					pMesh,
					nMesh,
					nLOD,
					pDLState,
					pEffect,
					pEffectDef,
					*pTechnique,
					pMaterial,
					bAlphaAsLighting ) );

				V_CONTINUE( sSetGeneralMeshParameters(
					pModel,
					pMesh,
					nMesh,
					nLOD,
					pDLState,
					&tMeshDraw.matWorld,
					&tMeshDraw.matView,
					&tMeshDraw.matProj,
					&tMeshDraw.matWorldViewProj,
					&tMeshDraw.vEyeInWorld,
					pEffect,
					*pTechnique,
					pMaterial,
					pEnvDef,
					bForceCurrentEnv,
					bAlphaAsLighting, 
					tMeshDraw.fAlpha ) );

				V_CONTINUE( sSetMeshTextures(
					pModel,	
					nMesh,
					pMesh,
					pModelDefinition,
					pEffect,
					*pTechnique,
					pMaterial,
					pEnvDef,
					nLOD) );	// CHB 2006.11.28

				V_CONTINUE( sSetMaterialParameters(
					pModel,
					pModelDefinition,
					nLOD,	// CHB 2006.12.08
					pMesh,
					nMesh,
					pEffect,
					pEffectDef,
					*pTechnique,
					pMaterial,
					pEnvDef,
					tMeshDraw.fEffectTransitionStr,
					bForceCurrentEnv ) );

				V_CONTINUE( sSetLightingEffectParameters(
					pModel,
					pDLState,
					pEffect,
					pEffectDef,
					*pTechnique,
					tMeshDraw.fEffectTransitionStr,
					pEnvDef,
					pModel->tLights ) );
			}

			if ( bStateFromEffect )
			{
				V_CONTINUE( dxC_EffectBeginPass( pEffect, nPass, pEffectDef ) );
			}

			if ( pEffect->dwFlags & EFFECT_FLAG_STATICBRANCH )
			{
				DWORD dwBranches;
				V( dx9_SetStaticBranchParamsFlags( dwBranches, pMaterial, pModel, pModelDefinition, pMesh, nMesh, pEffect, *pTechnique, tMeshDraw.nShaderType, pEnvDef, nLOD ) );	// CHB 2006.11.28
				V( dx9_SetStaticBranchParamsFromFlags( pEffect, *pTechnique, dwBranches ) );
			}

			// do here the render/texture state changes that override effect defaults
			if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
				dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
				dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

			if ( bForceNoScissor )
				dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

			TRACE_MODEL_INT( pModel->nId, "Draw Primitive", nMesh );

			BOOL bAnimated = pModel->nAppearanceDefId != INVALID_ID || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED );
			METRICS_GROUP eGroup = bAnimated ? METRICS_GROUP_ANIMATED : METRICS_GROUP_BACKGROUND;

			UINT nTriangles = pMesh->wTriangleCount;
#ifdef USE_SIMULATED_LOD
			nTriangles = static_cast<UINT>(nTriangles * fLODSimulationFactor);
			nTriangles = max(nTriangles, 1);
			nTriangles = min(nTriangles, pMesh->wTriangleCount);
#endif

			/////
			//CC@HAVOK
			//
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
			//GET ATTACHMENET
			int nAttachmentId = dx9_ModelGetMaterialOverrideAttachment( pModel, tMeshDraw.eMatOverrideType );
			if ( nAttachmentId != INVALID_ID )
			{
				//SEE IF IT HAS A PARTICLE SYSTEM
				int nParticleSystemIdTemp = gpfn_AttachmentGetAttachedById( pModel->nId, nAttachmentId );
				if ( nParticleSystemIdTemp != INVALID_ID )
				{

					PARTICLE_SYSTEM_DEFINITION* def;
					def = (PARTICLE_SYSTEM_DEFINITION*)DefinitionGetById(DEFINITION_GROUP_PARTICLE,nParticleSystemIdTemp);

					//SEE IF MATERIAL SHADER WANTS TO EMIT PARTICLES
					if( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_EMITS_GPU_PARTICLES ) )
					{

						V( dx10_ParticleSystemPrepareEmitter(pEffect, nParticleSystemIdTemp) );
						nPassCount = 1;

						gpuEmittedParticlesNeedAdvancement = true;
						nParticleSystemId = nParticleSystemIdTemp;
						//pTempEffect = pEffect;
					}
				}
			}
#endif
			///////
			// CC@HAVOK
			// end

			if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
			{
				V_CONTINUE( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, nTriangles, pEffect, eGroup ) );
			} else {
				V_CONTINUE( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, nTriangles, pEffect, eGroup ) );
			}

			pMesh->dwFlags |= MESH_FLAG_WARNED; // we have given all of the warnings on this mesh
		}

		// CC@HAVOK
		//
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
		if(gpuEmittedParticlesNeedAdvancement)
		{
			V( dx10_ParticleSystemGPUAdvance(
				nParticleSystemId ) );
		}
#endif
		// CC@HAVOK
		// end
		//
	}

#if 0
	// restore two-sided rendering to off
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	// restore alpha test to off
	dxC_SetAlphaTestEnable( FALSE );

	// restore alpha blend to off
	dxC_SetAlphaBlendEnable( FALSE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );
#endif

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) );

	// reset WRAP0 (only the poison shader uses this currently)
	DX9_BLOCK( dxC_SetRenderState( D3DRS_WRAP0, (DWORD)0 ); ) //NUTTAPONG TODO: Get wrap modes working

	// CC@HAVOK
#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_GPU_PARTICLE_EMITTER )
	dxC_GetDevice()->GSSetShader(NULL);
	//dxC_GetDevice()->SOSetTargets(0, NULL, 0);
#endif

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_RenderModel (
	const DRAWLIST_STATE * pState,
	int nModelID )
{
	V_RETURN( sRenderModel( pState, nModelID, MATERIAL_OVERRIDE_NORMAL ) );

	// if this model requests a clone, render it now
	if ( e_ModelGetFlagbit( nModelID, MODEL_FLAGBIT_CLONE ) )
	{
		V_RETURN( sRenderModel( pState, nModelID, MATERIAL_OVERRIDE_CLONE ) );
	}

	return S_OK;
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RenderMesh(
	MeshDraw & tMeshDraw )
{
	return sRenderMesh( tMeshDraw );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderMeshBehind( MeshDraw & tMeshDraw )
{
	PERF( DRAW_MESH_BEHIND );

	D3D_MESH_DEFINITION* pMesh = tMeshDraw.pMesh;
	D3D_MODEL* pModel = tMeshDraw.pModel;
	D3D_MODEL_DEFINITION* pModelDefinition = tMeshDraw.pModelDef;
	int nMesh = tMeshDraw.nMeshIndex;
	int nLOD = tMeshDraw.nLOD;
	ENVIRONMENT_DEFINITION* pEnvDef = NULL;
	DRAWLIST_STATE* pDLState = &tMeshDraw.tState;
	D3D_EFFECT* pEffect = tMeshDraw.pEffect;
	EFFECT_DEFINITION* pEffectDef = tMeshDraw.pEffectDef;
	EFFECT_TECHNIQUE* pTechnique = tMeshDraw.pTechnique;
	MATERIAL* pMaterial = tMeshDraw.pMaterial;	
	//D3D_VERTEX_BUFFER* pVertexBuffer;
	BOOL bForceCurrentEnv = FALSE;
	BOOL bForceNoScissor = TESTBIT_DWORD( tMeshDraw.dwFlagBits, MeshDraw::FLAGBIT_FORCE_NO_SCISSOR );

	// CHB 2006.11.30
#ifdef USE_SIMULATED_LOD
	float fLODSimulationFactor = (nLOD == LOD_LOW) ? g_fGeometryReductionFactor : 1.0f;
#endif


	V( dxC_SetSamplerDefaults( SAMPLER_DIFFUSE ) );

	ASSERT_RETFAIL( pMaterial );
	ASSERT_RETFAIL( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_DRAWBEHIND ) );

	//D3D_EFFECT * pMeshEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, tMeshDraw.nShaderType ) );
	//V_RETURN( dxC_ToolLoadShaderNow( pMeshEffect, pMaterial, tMeshDraw.nShaderType ) );


	D3D_PROFILE_REGION_INT_STR( L"Model", pModel->nId, pModelDefinition->tHeader.pszName );
	{
		PERF( DRAW_SETTINGS );
		D3D_PROFILE_REGION_INT( L"Mesh", nMesh );

		// set the vertex buffer
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex < pModelDefinition->nVertexBufferDefCount );
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

		if ( ! dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) )
			return S_FALSE;

		V_RETURN( dxC_SetStreamSource( *pVertexBufferDef, 0, pEffect ) );

		UINT nPassCount;
		V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );
		ASSERT_RETFAIL( nPassCount == 1 );
		V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

		//Set proper culling mode
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMesh->dwFlags & MESH_FLAG_TWO_SIDED || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
			dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
		{
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
		} else {
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
		}

		{
			D3DXVECTOR4 vLightAmbient(0,0,0,0);
			vLightAmbient.x = pModel->AmbientLight;
			vLightAmbient.x *= vLightAmbient.x;
			vLightAmbient.y = pModel->fBehindColor;

			V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_LIGHTAMBIENT, &vLightAmbient) );
		}

		// change alpha test, if necessary
		BOOL bMeshAlphaTest = dxC_MeshDiffuseHasAlpha( pModel, pModelDefinition, pMesh, nMesh, nLOD );
		if ( bMeshAlphaTest ) 
		{
			sSetMeshTextures(
				pModel,	
				nMesh,
				pMesh,
				pModelDefinition,
				pEffect,
				*pTechnique,
				pMaterial,
				pEnvDef,
				nLOD,
				true );	// CHB 2006.11.28

		}

		{
			V( sSetGeneralMeshParameters(
				pModel,
				pMesh,
				nMesh,
				nLOD,
				pDLState,
				&tMeshDraw.matWorld,
				&tMeshDraw.matView,
				&tMeshDraw.matProj,
				&tMeshDraw.matWorldViewProj,
				&tMeshDraw.vEyeInWorld,
				pEffect,
				*pTechnique,
				pMaterial,
				pEnvDef,
				bForceCurrentEnv,
				TRUE, 
				tMeshDraw.fAlpha,
				TRUE) );
		}

		V( dxC_SetAlphaTestEnable( bMeshAlphaTest ) );

		V( dxC_SetAlphaBlendEnable( false ) );
		//dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		//dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
		dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

		dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_GREATER );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, D3DZB_FALSE );

		//V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &tMeshDraw.matWorldViewProj ) );

		V( dxC_SetBones( pModel, nMesh, nLOD, pEffect, *pTechnique ) );	// CHB 2006.12.08, 2006.11.30

		// set the index buffer
		V_RETURN( dxC_SetIndices( pMesh->tIndexBufferDef ) );

		TRACE_MODEL_INT( pModel->nId, "Draw Primitive Behind", nMesh );

		// loop through all of the passes for the effect
		if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		} else {
			V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		}
	}

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) );

	// CML: Optimization would be to perform this reset in a end-of-pass callback command
	dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	return S_OK;
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RenderMeshBehind(
	MeshDraw & tMeshDraw )
{
	return sRenderMeshBehind( tMeshDraw );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderMeshDepth( MeshDraw & tMeshDraw )
{
	PERF( DRAW_MESH_DEPTH );

	D3D_MESH_DEFINITION* pMesh = tMeshDraw.pMesh;
	D3D_MODEL* pModel = tMeshDraw.pModel;
	D3D_MODEL_DEFINITION* pModelDefinition = tMeshDraw.pModelDef;
	int nMesh = tMeshDraw.nMeshIndex;
	int nLOD = tMeshDraw.nLOD;
	ENVIRONMENT_DEFINITION* pEnvDef = NULL;
	DRAWLIST_STATE* pDLState = &tMeshDraw.tState;
	D3D_EFFECT* pEffect = tMeshDraw.pEffect;
	EFFECT_DEFINITION* pEffectDef = tMeshDraw.pEffectDef;
	EFFECT_TECHNIQUE* pTechnique = tMeshDraw.pTechnique;
	MATERIAL* pMaterial = tMeshDraw.pMaterial;	
	//D3D_VERTEX_BUFFER* pVertexBuffer;
	BOOL bForceCurrentEnv = FALSE;
	BOOL bForceNoScissor = TESTBIT_DWORD( tMeshDraw.dwFlagBits, MeshDraw::FLAGBIT_FORCE_NO_SCISSOR );

	// CHB 2006.11.30
#ifdef USE_SIMULATED_LOD
	float fLODSimulationFactor = (nLOD == LOD_LOW) ? g_fGeometryReductionFactor : 1.0f;
#endif

	BOOL bZWriteEnable = TRUE;
	if (    dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FORCE_ALPHA ) 
	     || ( pMaterial->dwFlags & MATERIAL_FLAG_ALPHABLEND ) )			
	{
		bZWriteEnable = FALSE;
	}

	if ( AppIsHellgate() )
	{
		if ( pDLState->bNoDepthTest )
			dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		else
			dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );
	}
	else
	{
		dxC_SetRenderState( D3DRS_ZENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER ) ? D3DZB_TRUE : D3DZB_FALSE );
		dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	}
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, e_GetRenderFlag( RENDER_FLAG_USE_ZBUFFER )
		&& e_GetRenderFlag( RENDER_FLAG_USE_ZWRITE ) && bZWriteEnable );


	V( dxC_SetSamplerDefaults( SAMPLER_DIFFUSE ) );

	ASSERT_RETFAIL( pMaterial )

	D3D_EFFECT * pMeshEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, tMeshDraw.nShaderType ) );
	V_RETURN( dxC_ToolLoadShaderNow( pMeshEffect, pMaterial, tMeshDraw.nShaderType ) );
	if ( (pMeshEffect->dwFlags & EFFECT_FLAG_RENDERTOZ) == 0 )
		return S_FALSE;


	D3D_PROFILE_REGION_INT_STR( L"Model", pModel->nId, pModelDefinition->tHeader.pszName );
	{
		PERF( DRAW_SETTINGS );
		D3D_PROFILE_REGION_INT( L"Mesh", nMesh );

		// set the vertex buffer
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex < pModelDefinition->nVertexBufferDefCount );
		ASSERT_RETFAIL( pMesh->nVertexBufferIndex >= 0  );
		D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = &pModelDefinition->ptVertexBufferDefs[ pMesh->nVertexBufferIndex ];

		if ( ! dxC_VertexBufferD3DExists( pVertexBufferDef->nD3DBufferID[ 0 ] ) )
			return S_FALSE;

		{

#if 0			
			// AE 2006.12.13 - Only support initial z-pass on background and animated vertex types.
			D3D_VERTEX_BUFFER_DEFINITION tVBDef;

			switch ( pVertexBufferDef->eVertexType )
			{
			case VERTEX_DECL_RIGID_16:
			case VERTEX_DECL_RIGID_32:
			case VERTEX_DECL_RIGID_64:
				tVBDef.eVertexType = VERTEX_DECL_RIGID_ZBUFFER;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				break;

			case VERTEX_DECL_ANIMATED:
#if 1
				continue;
#else
				tVBDef.eVertexType = VERTEX_DECL_ANIMATED_ZBUFFER;
				tVBDef.nD3DBufferID[ 0 ] = pVertexBufferDef->nD3DBufferID[ 0 ];
				break;
#endif

			default:
				UNDEFINED_CODE_X( "This vertex format is not supported for zpass!" );
				return E_FAIL;
			}	

			V_CONTINUE( dxC_SetStreamSource( tVBDef, 0, pEffect ) );
#else
			V_RETURN( dxC_SetStreamSource( *pVertexBufferDef, 0, pEffect ) );
#endif
		}

		UINT nPassCount;
		V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );
		ASSERT_RETFAIL( nPassCount == 1 );
		V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

		//Set proper culling mode
		if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMesh->dwFlags & MESH_FLAG_TWO_SIDED || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
			dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
		{
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
		} else {
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
		}

		// change alpha test, if necessary
		BOOL bMeshAlphaTest = dxC_MeshDiffuseHasAlpha( pModel, pModelDefinition, pMesh, nMesh, nLOD );
		if ( bMeshAlphaTest ) 
		{
			sSetMeshTextures(
				pModel,	
				nMesh,
				pMesh,
				pModelDefinition,
				pEffect,
				*pTechnique,
				pMaterial,
				pEnvDef,
				nLOD,
				true );	// CHB 2006.11.28

		}

		{
			V( sSetGeneralMeshParameters(
				pModel,
				pMesh,
				nMesh,
				nLOD,
				pDLState,
				&tMeshDraw.matWorld,
				&tMeshDraw.matView,
				&tMeshDraw.matProj,
				&tMeshDraw.matWorldViewProj,
				&tMeshDraw.vEyeInWorld,
				pEffect,
				*pTechnique,
				pMaterial,
				pEnvDef,
				bForceCurrentEnv,
				TRUE, 
				tMeshDraw.fAlpha,
				TRUE) );
		}

		V( dxC_SetAlphaTestEnable( bMeshAlphaTest ) );
		V( dxC_SetAlphaBlendEnable( false ) );

		//V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &tMeshDraw.matWorldViewProj ) );

		V( dxC_SetBones( pModel, nMesh, nLOD, pEffect, *pTechnique ) );	// CHB 2006.12.08, 2006.11.30

		// set the index buffer
		V_RETURN( dxC_SetIndices( pMesh->tIndexBufferDef ) );

		TRACE_MODEL_INT( pModel->nId, "Draw Primitive Depth", nMesh );

		// loop through all of the passes for the effect
		if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
		{
			V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		} else {
			V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, pMesh->wTriangleCount, pEffect, METRICS_GROUP_ZPASS ) );
		}
	}

	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) );

	// reset WRAP0 (only the poison shader uses this currently)
	//DX9_BLOCK( dxC_SetRenderState( D3DRS_WRAP0, (DWORD)0 ); ) //NUTTAPONG TODO: Get wrap modes working

	return S_OK;
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RenderMeshDepth(
	MeshDraw & tMeshDraw )
{
	return sRenderMeshDepth( tMeshDraw );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderModelBehind (
	int nDrawList,
	int nModelID )
{
	if ( ! AppIsTugboat() )
		return S_FALSE;
	const DRAWLIST_STATE * pState = dx9_GetDrawListState( nDrawList );
	return dx9_RenderModelBehind( pState, nModelID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderModelBehind (
	const DRAWLIST_STATE * pState,
	int nModelID )
{
	if ( ! AppIsTugboat() )
		return S_FALSE;

	PERF( DRAW_MODEL )

	D3D_PROFILE_REGION_INT( L"Model", nModelID );
	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETINVALIDARG(pModel);

	int nModelDefinitionId = pModel->nModelDefinitionId;
	if ( nModelDefinitionId == INVALID_ID )
		return S_FALSE;

	if ( !pState->bAllowInvisibleModels )
		if ( ! dx9_ModelShouldRender( pModel ) )
			return S_FALSE;

	// temp workaround for cube map gen
	if ( e_GeneratingCubeMap() && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_FIRST_PERSON_PROJ ) )
		return S_FALSE;

	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( nModelDefinitionId, 
												e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return S_FALSE;

	//int nRoomID = e_ModelGetRoomID( pModel->nId ); // used for ambient and directional lights


	MATRIX * pmatWorldCurrent;
	MATRIX * pmatViewCurrent;
	MATRIX * pmatProjCurrent;
	MATRIX matWorldView;
	MATRIX matWorldViewProjection;
	VECTOR4 vEyeLocationInWorld;
	float fDistanceToEyeSquared;

	V_RETURN( sGetMatrices(
		pModel,
		pState,
		pmatWorldCurrent,
		pmatViewCurrent,
		pmatProjCurrent,
		&matWorldView,
		&matWorldViewProjection,
		&fDistanceToEyeSquared,
		vEyeLocationInWorld ) );

	int nLOD = LOD_HIGH;	// CHB 2006.11.28
//	// CHB 2006.08.09 - Check for LOD transition.
//#ifdef USE_SIMULATED_LOD
//	float fLODSimulationFactor = 1.0f;
//#else
//	D3D_MODEL_DEFINITION* pMD = dxC_ModelDefinitionGet( nModelDefinitionId, LOD_LOW );
//	if ( pMD )
//#endif
//	{
//		float fEffectiveDistance = e_OptionState_GetActive()->get_fLODScale() * pModel->fLODDistance + e_OptionState_GetActive()->get_fLODBias();
//		fEffectiveDistance = max(fEffectiveDistance, 0.0f);
//		if (fDistanceToEyeSquared >= fEffectiveDistance * fEffectiveDistance) 
//		{
//#ifdef USE_SIMULATED_LOD
//			fLODSimulationFactor = g_fGeometryReductionFactor;
//#else
//			pModelDefinition = pMD;
//			nLOD = LOD_LOW;	// CHB 2006.11.28
//#endif
//		}
//	}

	// to make fog work correctly
	//MatrixMakeWFriendlyProjection( pmatProjCurrent );
	V( dxC_SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)pmatProjCurrent ) );
	BOOL bForceCurrentEnv = FALSE;
	ENVIRONMENT_DEFINITION * pEnvDef;

	V_RETURN( sGetEnvironment(
		pState,
		pModel,
		pEnvDef,
		bForceCurrentEnv ) );



	DWORD dwMeshFlagsDoDraw   = pState->dwMeshFlagsDoDraw;
	DWORD dwMeshFlagsDontDraw = pState->dwMeshFlagsDontDraw;
	BOOL bLights = FALSE;//pState->bLights;

	V_RETURN( sUpdateMeshDrawOptions(
		dwMeshFlagsDoDraw,
		dwMeshFlagsDontDraw,
		bLights,
		fDistanceToEyeSquared,
		pEnvDef,
		bForceCurrentEnv ) );


	//MODEL_RENDER_LIGHT_DATA tLights;
	//sInitModelLights( tLights );

	//if ( bLights )
	//{
	//	// have to select these early to get a count for effect technique selection

	//	// point lights selected inside effect/technique selection routine
	//	//sSelectLightsSpecularOnly( pModel, pState, tLights );
	//}
	//else
	//{
	//	tLights.dwFlags |= MODEL_RENDER_LIGHT_DATA::STATE_NO_DYNAMIC_LIGHTS;
	//}


	PLANE tPlanes[ NUM_FRUSTUM_PLANES ];
	//if ( pModel->ptMeshRenderOBBWorld )
	//	e_MakeFrustumPlanes( tPlanes, (MATRIX*)pmatViewCurrent, (MATRIX*)pmatProjCurrent );

	//HRVERIFY( dx9_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE  ) );
	//dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_ONE);
	//dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_ZERO);

	UINT nPassCount = 0;
	D3D_EFFECT * pEffect;
	EFFECT_TECHNIQUE * tTechnique = NULL;
	EFFECT_TECHNIQUE * tStaticTechnique = NULL;
	//int nTextureId = e_GetUtilityTexture( TEXTURE_OCCLUDED );
 	pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_OCCLUDED ) );
	ASSERT_RETFAIL( pEffect );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();

	tFeat.nInts[ TF_INT_INDEX ] = 0;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &tTechnique ) );
	ASSERT_RETFAIL( tTechnique );
	ASSERT_RETFAIL( tTechnique->hHandle );

	tFeat.nInts[ TF_INT_INDEX ] = 1;
	V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &tStaticTechnique ) );
	ASSERT_RETFAIL( tStaticTechnique );
	ASSERT_RETFAIL( tStaticTechnique->hHandle );

	// Iterate through the meshes and draw them
	D3DXVECTOR4 vLightAmbient;

	dxC_SetAlphaTestEnable( FALSE );
	//dx9_SetAlphaBlendEnable( TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	dxC_SetRenderState( D3DRS_ZFUNC, D3DCMP_GREATER );
 	dxC_SetRenderState( D3DRS_ZWRITEENABLE, D3DZB_FALSE );

	for ( int nMesh = 0; nMesh < pModelDefinition->nMeshCount; nMesh++ )
	{
		EFFECT_TECHNIQUE * pTechnique = NULL;
		D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGet( pModelDefinition->pnMeshIds [ nMesh ] );
		ASSERT_CONTINUE( pMesh );
									  
		V_CONTINUE( dx9_MeshApplyMaterial( pMesh, pModelDefinition, pModel, nLOD, nMesh ) );	// CHB 2006.12.08

		DWORD dwMeshFlags = pMesh->dwFlags;
		int nShaderType = dx9_GetCurrentShaderType( pEnvDef, pState, pModel );

		int nMaterialID = dx9_MeshGetMaterial( pModel, pMesh );
		if ( nMaterialID != pMesh->nMaterialId )
		{
			EFFECT_DEFINITION * pEffectDef = e_MaterialGetEffectDef( nMaterialID, nShaderType );
			if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS ) )
				dwMeshFlags |= MESH_FLAG_ALPHABLEND;
		}
		dwMeshFlags |= MESH_FLAG_ALPHABLEND;
		{
			if ( sMeshCullRender(
					pModel,
					pMesh,
					nMesh,
					dwMeshFlags,
					pState,
					tPlanes,
					dwMeshFlagsDoDraw,
					dwMeshFlagsDontDraw ) )
				continue;
		}
		PERF( DRAW_MESH )


		D3D_VERTEX_BUFFER * pVertexBuffer;
		MATERIAL * pMaterial;

		if( pMesh->nMaterialId == INVALID_ID )
		{
			continue;
		}
		pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, pMesh->nMaterialId );
		if( !pMaterial )
		{
			continue;
		}
		{	  

			int nShaderType = dx9_GetCurrentShaderType( pEnvDef, pState, pModel );
			D3D_EFFECT * pMeshEffect = dxC_EffectGet( dxC_GetShaderTypeEffectID( pMaterial->nShaderLineId, nShaderType ) );
			V_CONTINUE( dxC_ToolLoadShaderNow( pMeshEffect, pMaterial, nShaderType ) );
			BOOL skinned = pMesh->dwFlags & MESH_FLAG_SKINNED;
			//D3DXHANDLE hTechnique = ( pMeshEffect->dwFlags & EFFECT_FLAG_ANIMATED ) ? hTechniqueAnimated : hTechniqueRigid;
			pTechnique = tTechnique;
			if( !skinned )
			{
				pTechnique = tStaticTechnique;
			}
			


			PERF( DRAW_SETTINGS )

			{
				PERF( DRAW_FX_PICK )

				if ( !pMeshEffect )
					continue;
				V_CONTINUE( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );
			}
					  

			{
				PERF( DRAW_VBSETUP )
				if ( S_OK != sSetMeshVertexParameters( pModelDefinition, pMesh, pEffect, NULL, pVertexBuffer ) )
					continue;
			}

			{

				V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &matWorldViewProjection ) );
				V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDMATRIX, pmatWorldCurrent ) );
				V( dxC_SetBones ( pModel, nMesh, nLOD, pEffect, *pTechnique ) );
				{

					vLightAmbient.x = pModel->AmbientLight;
					vLightAmbient.x *= vLightAmbient.x;
					vLightAmbient.y = pModel->fBehindColor;
					V( dx9_EffectSetVector( pEffect, *tTechnique, EFFECT_PARAM_LIGHTAMBIENT, &vLightAmbient) );
				}
			

				// change alpha test, if necessary
				BOOL bMeshAlphaTest = FALSE;




	
				int nDiffTextureId = dx9_ModelGetTexture(pModel, pModelDefinition, pMesh, nMesh, TEXTURE_DIFFUSE, nLOD, false, pMaterial);
				D3D_TEXTURE * pTexture = dxC_TextureGet( nDiffTextureId );
				if( pTexture )
				{
					V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_DIFFUSEMAP0, (LPD3DCBASESHADERRESOURCEVIEW)pTexture->GetShaderResourceView()) );
				}

				if ( dxC_MeshDiffuseHasAlpha( pModel, pModelDefinition, pMesh, nMesh, nLOD ) )
				{
					// set alpha test, if specified for both effect and mesh
					const EFFECT_DEFINITION * pEffectDef = (const EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pMeshEffect->nEffectDefId );
					ASSERT( pEffectDef );
					bMeshAlphaTest = TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_ALPHA_TEST );

				}
				dxC_SetAlphaTestEnable( bMeshAlphaTest );

			
			}
		}


		ASSERT_CONTINUE( nPassCount > 0 );

		// loop through all of the passes for the effect
		for( UINT nPass = 0; nPass < nPassCount; nPass ++ ) 
		{
			V_CONTINUE( dxC_EffectBeginPass( pEffect, nPass ) );

			if ( pEffect->dwFlags & EFFECT_FLAG_STATICBRANCH )
			{
				DWORD dwBranches;
				V( dx9_SetStaticBranchParamsFlags( dwBranches, pMaterial, pModel, pModelDefinition, pMesh, nMesh, pEffect, *tTechnique, nShaderType, pEnvDef, nLOD ) );	// CHB 2006.11.28
				V( dx9_SetStaticBranchParamsFromFlags( pEffect, *tTechnique, dwBranches ) );
			}
			// do here the render/texture state changes that override effect defaults
			if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_TWO_SIDED ) || pMaterial->dwFlags & MATERIAL_FLAG_TWO_SIDED ||
				dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MIRRORED ) )
				dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );	

			TRACE_MODEL_INT( pModel->nId, "Draw Primitive", nMesh );

			BOOL bAnimated = pModel->nAppearanceDefId != INVALID_ID || dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED );
			METRICS_GROUP eGroup = bAnimated ? METRICS_GROUP_ANIMATED : METRICS_GROUP_BACKGROUND;

			UINT nTriangles = pMesh->wTriangleCount;
#ifdef USE_SIMULATED_LOD
			nTriangles = static_cast<UINT>(nTriangles * fLODSimulationFactor);
			nTriangles = max(nTriangles, 1);
			nTriangles = min(nTriangles, pMesh->wTriangleCount);
#endif

			if ( dxC_IndexBufferD3DExists( pMesh->tIndexBufferDef.nD3DBufferID ) )
			{
				V( dxC_DrawIndexedPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, 0, pMesh->nVertexCount, 0, nTriangles, pEffect, eGroup ) );
			}
			else
			{
				V( dxC_DrawPrimitive( pMesh->ePrimitiveType, pMesh->nVertexStart, nTriangles, pEffect, eGroup ) );
			}
		}

		pMesh->dwFlags |= MESH_FLAG_WARNED; // we have given all of the warnings on this mesh
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderLayer( int nLayerID, LPD3DCSHADERRESOURCEVIEW pTexture1, LPD3DCSHADERRESOURCEVIEW pTexture2, DWORD dwColor )
{

	ASSERT_RETINVALIDARG( nLayerID >= 0 && nLayerID < LAYER_MAX );

	int nEffectID = dx9_EffectGetNonMaterialID( gtScreenLayers[ nLayerID ].nEffect );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	UINT nUsePass = 0;
	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();

	switch ( pEffect->nTechniqueGroup )
	{
	case TECHNIQUE_GROUP_BLUR:
		// CHB 2006.06.13 - Blur effect now uses a single technique with multiple passes.
		nUsePass = gtScreenLayers[ nLayerID ].nOption;
		dxC_SetRenderState( D3DRS_ZENABLE, false );
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, false );
		break;
	case TECHNIQUE_GROUP_GENERAL:
	case TECHNIQUE_GROUP_SHADOW:	// CHB 2006.11.03
		tFeat.nInts[ TF_INT_INDEX ] = gtScreenLayers[ nLayerID ].nOption;
		break;
	default:
		return E_FAIL;
	}

	V_RETURN( dxC_EffectGetTechnique( nEffectID, tFeat, &ptTechnique, &gtScreenLayers[ nLayerID ].tTechniqueCache ) );
	ASSERT_RETFAIL( ptTechnique );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );
	ASSERT_RETFAIL(nUsePass < nPassCount);	// ensure valid pass number
	ASSERT_RETFAIL((pEffect->nTechniqueGroup == TECHNIQUE_GROUP_BLUR) || (nPassCount == 1));	// currently non-blur layers only use one pass

	V( dxC_EffectSetScreenSize( pEffect, *ptTechnique ) );
	V( dxC_EffectSetPixelAccurateOffset( pEffect, *ptTechnique ) );
	V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, (LPD3DCBASESHADERRESOURCEVIEW)pTexture1) );
	if ( pTexture2 )
	{
		V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP1, (LPD3DCBASESHADERRESOURCEVIEW)pTexture2) );
	}
	//SPD3DCTEXTURE2D pTex = NULL;
	switch( nLayerID )
	{
	case LAYER_FADE:
		dx9_EffectSetColor( pEffect, *ptTechnique, EFFECT_PARAM_LAYER_COLOR, dwColor );
		break;
	case LAYER_COMBINE:
		dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, false );
		dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, true );
		dxC_SetRenderState( D3DRS_ALPHATESTENABLE, false );
		dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
		dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

#ifdef DX10_DOF
		dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
		dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );
		dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO );
#ifndef DX10_DOF_GATHER
		dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, dxC_RTShaderResourceGet(dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_DEPTH )) );
		dx9_EffectSetFloat( pEffect, *ptTechnique, EFFECT_PARAM_SMOKE_ZFAR,  e_GetFarClipPlane() );
#endif
#else
		dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
#endif
		break;
	case LAYER_DOWNSAMPLE:
		dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, false );
		dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
		dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, false );
		dxC_SetRenderState( D3DRS_ALPHATESTENABLE, false );
		break;
#ifdef DX10_DOF_GATHER
	case LAYER_GAUSS_S_X:
	case LAYER_GAUSS_S_Y:
		dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, dxC_RTShaderResourceGet(RENDER_TARGET_DEPTH) );
		dx9_EffectSetFloat( pEffect, *ptTechnique, EFFECT_PARAM_SMOKE_ZFAR,  e_GetFarClipPlane() );
		break;
#endif
	default:
		break;
	}

	V_RETURN( dxC_SetStreamSource( gtScreenCoverVertexBufferDef, 0 , pEffect) );

	V_RETURN( dxC_EffectBeginPass( pEffect, nUsePass ) );

	
	V_RETURN( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, 0, DP_GETPRIMCOUNT_TRILIST( NUM_SCREEN_COVER_VERTICES ), pEffect, METRICS_GROUP_EFFECTS ) );

#ifdef DX10_DOF
	if( nLayerID == LAYER_COMBINE )
		dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO );
#endif

	return S_OK;
}

