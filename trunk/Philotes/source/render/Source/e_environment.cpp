//----------------------------------------------------------------------------
// e_environment.cpp
//
// - Implementation for environment functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_definition.h"
#include "e_settings.h"
#include "appcommontimer.h"
#include "ptime.h"
#include "e_region.h"

#include "e_environment.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define CLIP_FOG_DISTANCE_SPRING_BIAS	1.0f

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

const char szLevelLocations[ NUM_LEVEL_LOCATIONS ][16] =
{
	"Indoor",
	"Outdoor",
	"IndoorGrid",
	"Flashlight",
};

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

const char * e_LevelLocationGetName( int nLocation )
{
	ASSERT_RETNULL( IS_VALID_INDEX( nLocation, NUM_LEVEL_LOCATIONS ) );
	return szLevelLocations[ nLocation ];
}

int e_GetCurrentEnvironmentDefID( int nOverrideRegion /*= INVALID_ID*/ )
{
	const REGION * pRegion;
	if ( nOverrideRegion != INVALID_ID )
		pRegion = e_RegionGet( nOverrideRegion );
	else
		pRegion = e_GetCurrentRegion();
	if ( ! pRegion )
		return INVALID_ID;

	// debug CRAP
	//if ( ! pRegion->pEnvironmentDef )
	//	pRegion = e_RegionGet( 0 );

	return pRegion->nEnvironmentDef;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

ENVIRONMENT_DEFINITION * e_GetCurrentEnvironmentDef( int nOverrideRegion /*= INVALID_ID*/ )
{
	int nEnvDefID = e_GetCurrentEnvironmentDefID( nOverrideRegion );
	if ( nEnvDefID == INVALID_ID )
		return NULL;
	ENVIRONMENT_DEFINITION * pEnvDef = (ENVIRONMENT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvDefID );
	ASSERT( pEnvDef );
	return pEnvDef;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_RegionGetLevelLocation( int nRegion )
{
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef( nRegion );
	if ( pEnvDef )
		return pEnvDef->nLocation;
	//ASSERTX(0, "Ah HA!" );
	return LEVEL_LOCATION_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetCurrentLevelLocation( int nOverrideRegion /*= INVALID_ID*/ )
{
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef( nOverrideRegion );
	if ( pEnvDef )
		return pEnvDef->nLocation;
	//ASSERTX(0, "Ah HA!" );
	return LEVEL_LOCATION_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetLevelLocationRenderFlags( int nLocation )
{
	if ( nLocation == LEVEL_LOCATION_UNKNOWN )
		nLocation = e_GetCurrentLevelLocation();

	switch ( nLocation )
	{
	case LEVEL_LOCATION_INDOOR:
	case LEVEL_LOCATION_INDOORGRID:
	case LEVEL_LOCATION_FLASHLIGHT:
		if ( AppIsTugboat() )
		{
			e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				FALSE );
			e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			FALSE );
			//e_SetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED,	FALSE );
			//e_SetRenderFlag( RENDER_FLAG_SILHOUETTE,				FALSE );
		}
		else
		{
			//e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				TRUE );	// for visual portals
			//e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			TRUE );
			//e_SetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED,	FALSE );
			//e_SetRenderFlag( RENDER_FLAG_SILHOUETTE,				FALSE );
		}
		break;
	case LEVEL_LOCATION_OUTDOOR:
		if ( AppIsTugboat() )
		{
			e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				FALSE );
			e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			FALSE );
			//e_SetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED,	FALSE );
			//e_SetRenderFlag( RENDER_FLAG_SILHOUETTE,				FALSE );
		}
		else
		{
			//e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				TRUE );	// for visual portals
			//e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			TRUE ); // for visual portals and the other levels
			//e_SetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED,	TRUE );
			//e_SetRenderFlag( RENDER_FLAG_SILHOUETTE,				TRUE );
		}
		break;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_CurrentLocationUseOutdoorCulling( int nOverrideRegion /*= INVALID_ID*/ )
{
	int nLocation = e_GetCurrentLevelLocation( nOverrideRegion );
	ASSERT_RETFALSE( IS_VALID_INDEX( nLocation, NUM_LEVEL_LOCATIONS ) );

	switch ( nLocation )
	{
	case LEVEL_LOCATION_INDOOR:
	case LEVEL_LOCATION_FLASHLIGHT:
		return FALSE;
	case LEVEL_LOCATION_INDOORGRID:
	case LEVEL_LOCATION_OUTDOOR:
		return TRUE;
	}

	ASSERT_MSG( "Invalid level location encountered!" );
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetSilhouetteDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( (float)pPrevEnvDef->nSilhouetteDistance, 
					(float)pEnvDef->nSilhouetteDistance,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return (float)pEnvDef->nSilhouetteDistance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetClipDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	if ( c_GetToolMode() )
		return TOOL_MODE_CLIP_DISTANCE;
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( (float)pPrevEnvDef->nClipDistance, 
					(float)pEnvDef->nClipDistance,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return (float)pEnvDef->nClipDistance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetFogStartDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( (float)pPrevEnvDef->nFogStartDistance, 
					(float)pEnvDef->nFogStartDistance,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return (float)pEnvDef->nFogStartDistance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetAmbientIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( pPrevEnvDef->fAmbientIntensity, 
					pEnvDef->fAmbientIntensity,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return pEnvDef->fAmbientIntensity;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetShadowIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( pPrevEnvDef->fShadowIntensity, 
					pEnvDef->fShadowIntensity,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return pEnvDef->fShadowIntensity;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetAmbientColor( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR4 * pvAmbient, BOOL bForceCurrent )
{
	VECTOR3 vColor = *(VECTOR*)&pEnvDef->tAmbientColor.GetValue( 0.5f );
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				VECTOR3 vPrevColor = *(VECTOR*)&pPrevEnvDef->tAmbientColor.GetValue( 0.5f );
				vColor = LERP( vPrevColor, vColor, pRegion->fEnvTransitionPhase );
			}
		}
	}
	*pvAmbient = VECTOR4( vColor, 1.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetFogColorVectorFromEnv( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR3 * pvColor, BOOL bForceCurrent )
{
	VECTOR3 vColor = *(VECTOR*)&pEnvDef->tFogColor.GetValue( 0.5f );
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				VECTOR3 vPrevColor = *(VECTOR*)&pPrevEnvDef->tFogColor.GetValue( 0.5f );
				vColor = LERP( vPrevColor, vColor, pRegion->fEnvTransitionPhase );
			}
		}
	}
	*pvColor = vColor;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DWORD e_GetFogColorFromEnv( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	VECTOR3 vColor;
	e_GetFogColorVectorFromEnv( pEnvDef, &vColor, bForceCurrent );
	return ARGB_MAKE_FROM_FLOAT( vColor.x, vColor.y, vColor.z, 0.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetDirLightIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, BOOL bForceCurrent )
{
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				return LERP( pPrevEnvDef->tDirLights[ eDirLight ].fIntensity, 
					pEnvDef->tDirLights[ eDirLight ].fIntensity,
					pRegion->fEnvTransitionPhase );
			}
		}
	}
	return pEnvDef->tDirLights[ eDirLight ].fIntensity;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGetEnvDirLight( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, VECTOR & vDir )
{
	// need to handle the "opposite direction" flag for dirlight # 2 diffuse

	if ( eDirLight == ENV_DIR_LIGHT_SECONDARY_DIFFUSE && pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_DIR1_OPPOSITE_DIR0 )
		vDir = - pEnvDef->tDirLights[ ENV_DIR_LIGHT_PRIMARY_DIFFUSE ].vVec;
	else
		vDir = pEnvDef->tDirLights[ eDirLight ].vVec;

	// should be normalized at restore time
	//VectorNormalize( vDir );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GetDirLightVector( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, VECTOR * pDir, BOOL bForceCurrent )
{
	VECTOR vCurDir;
	sGetEnvDirLight( pEnvDef, eDirLight, vCurDir );

	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				VECTOR vPrevDir;
				sGetEnvDirLight( pPrevEnvDef, eDirLight, vPrevDir );

				vCurDir = LERP( vPrevDir, vCurDir, pRegion->fEnvTransitionPhase );
				VectorNormalize( vCurDir );
			}
		}
	}

	*pDir = vCurDir;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetDirLightColor( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, VECTOR4 * pvColor, BOOL bForceCurrent )
{
	VECTOR3 vColor = *(VECTOR*)&pEnvDef->tDirLights[ eDirLight ].tColor.GetValue( 0.5f );
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				VECTOR3 vPrevColor = *(VECTOR*)&pPrevEnvDef->tDirLights[ eDirLight ].tColor.GetValue( 0.5f );
				vColor = LERP( vPrevColor, vColor, pRegion->fEnvTransitionPhase );
			}
		}
	}
	*pvColor = VECTOR4( vColor, 1.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetHemiLightColors( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR4 * pvColors, BOOL bForceCurrent /*= FALSE*/ )
{
	VECTOR3 vColor[2];
	vColor[0] = *(VECTOR*)&pEnvDef->tHemiLightColors[0].GetValue( 0.5f );
	vColor[1] = *(VECTOR*)&pEnvDef->tHemiLightColors[1].GetValue( 0.5f );
	float fStr = pEnvDef->fHemiLightIntensity;

	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				for ( int i = 0; i < 2; i++ )
				{
					VECTOR3 vPrevColor = *(VECTOR*)&pPrevEnvDef->tHemiLightColors[i].GetValue( 0.5f );
					vColor[i] = LERP( vPrevColor, vColor[i], pRegion->fEnvTransitionPhase );
				}

				fStr = LERP( pPrevEnvDef->fHemiLightIntensity, fStr, pRegion->fEnvTransitionPhase );
			}
		}
	}

	pvColors[0] = VECTOR4( vColor[0], fStr );
	pvColors[1] = VECTOR4( vColor[1], fStr );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_GetEnvMapFactor( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	// this is part of how we transition env maps
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		// lerp down (1..0) to factor .5, then lerp up (0..1) to factor 1.0
		if ( pRegion->fEnvTransitionPhase < 0.5f )
			return LERP( 1.f, 0.f, pRegion->fEnvTransitionPhase * 2.f );
		else
			return LERP( 0.f, 1.f, ( pRegion->fEnvTransitionPhase - 0.5f ) * 2.f );
	}
	return 1.f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_GetEnvMap( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	// this is part of how we transition env maps
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				// lerp down (1..0) to factor .5, then lerp up (0..1) to factor 1.0
				if ( pRegion->fEnvTransitionPhase < 0.5f )
					return pPrevEnvDef->nEnvMapTextureID;
				else
					return pEnvDef->nEnvMapTextureID;
			}
		}
	}
	return pEnvDef->nEnvMapTextureID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_GetEnvMapMIPLevels( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	// this is part of how we transition env maps
	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		if ( pRegion->nPrevEnvironmentDef != INVALID_ID )
		{
			ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
			if ( pPrevEnvDef )
			{
				// lerp down (1..0) to factor .5, then lerp up (0..1) to factor 1.0
				if ( pPrevEnvDef && pRegion->fEnvTransitionPhase < 0.5f )
					return pPrevEnvDef->nEnvMapMIPLevels;
				else
					return pEnvDef->nEnvMapMIPLevels;
			}
		}
	}
	return pEnvDef->nEnvMapMIPLevels;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DWORD e_EnvGetBackgroundColor( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent )
{
	// if no env or bg color specifed use the fog color
	if ( !pEnvDef )
		return e_GetFogColor();

	const CInterpolationPath<CFloatTriple> * ptBackgroundColor = &pEnvDef->tBackgroundColor;

	// if the current env has a skybox, use the skybox bg color
	ENVIRONMENT_DEFINITION * pSkyEnvDef = e_GetCurrentEnvironmentDef();
	if ( pSkyEnvDef && pSkyEnvDef->nSkyboxDefID != INVALID_ID )
	{
		SKYBOX_DEFINITION * pSkyboxDef = (SKYBOX_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_SKYBOX, pSkyEnvDef->nSkyboxDefID );
		if ( pSkyboxDef )
		{
			ptBackgroundColor = &pSkyboxDef->tBackgroundColor;
		}
	}

	VECTOR3 vColor;
	if ( ptBackgroundColor->GetPointCount() <= 0 )
		e_GetFogColorVectorFromEnv( pEnvDef, &vColor, bForceCurrent );
	else
		vColor = *(VECTOR*)&ptBackgroundColor->GetValue( 0.5f );

	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
		if ( pPrevEnvDef )
		{
			VECTOR3 vPrevColor;
			if ( pPrevEnvDef->tBackgroundColor.GetPointCount() <= 0 )
				e_GetFogColorVectorFromEnv( pPrevEnvDef, &vPrevColor, TRUE );
			else
				vPrevColor = *(VECTOR*)&pPrevEnvDef->tBackgroundColor.GetValue( 0.5f );
			vColor = LERP( vPrevColor, vColor, pRegion->fEnvTransitionPhase );
		}
	}
	return ARGB_MAKE_FROM_FLOAT( vColor.x, vColor.y, vColor.z, 0.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetWind(
	const ENVIRONMENT_DEFINITION * pEnvDef,
	float & fMin,
	float & fMax,
	VECTOR & vDir,
	BOOL bForceCurrent )
{
	if ( ! pEnvDef )
		return;

	if ( ! bForceCurrent && e_EnvironmentInTransition() )
	{
		REGION * pRegion = e_GetCurrentRegion();
		ENVIRONMENT_DEFINITION * pPrevEnvDef = (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nPrevEnvironmentDef );
		if ( pPrevEnvDef )
		{
			fMin = LERP( pPrevEnvDef->fWindMin, pEnvDef->fWindMin, pRegion->fEnvTransitionPhase );
			fMax = LERP( pPrevEnvDef->fWindMax, pEnvDef->fWindMax, pRegion->fEnvTransitionPhase );
			vDir = LERP( pPrevEnvDef->vWindDirection, pEnvDef->vWindDirection, pRegion->fEnvTransitionPhase );
			VectorNormalize( vDir );
		}
		else
		{
			bForceCurrent = TRUE;
		}
	}
	else
	{
		bForceCurrent = TRUE;
	}

	if ( bForceCurrent )
	{
		fMin = pEnvDef->fWindMin;
		fMax = pEnvDef->fWindMax;
		vDir = pEnvDef->vWindDirection;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_UpdateEnvironmentTransition()
{
	REGION * pRegion = e_GetCurrentRegion();
	if ( ! pRegion )
		return;

	// if we're waiting to begin a transition
	if ( pRegion->nWaitEnvironmentDef != INVALID_ID )
	{
		// see if it's ready, then begin the transition
		if ( DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nWaitEnvironmentDef ) )
		{
			// go!
			pRegion->fEnvTransitionPhase = -1.f;
			pRegion->nPrevEnvironmentDef = pRegion->nEnvironmentDef;
			pRegion->nEnvironmentDef = pRegion->nWaitEnvironmentDef;
			pRegion->nWaitEnvironmentDef = INVALID_ID;
		}
	}

	if ( pRegion->nPrevEnvironmentDef == INVALID_ID )
		return;

	if ( pRegion->fEnvTransitionPhase < 0.f )
	{
		// start timer
		pRegion->tEnvTransitionBegin = AppCommonGetCurTime();
	}
	float fPrevPhase = pRegion->fEnvTransitionPhase;
	TIME tCurTime = AppCommonGetCurTime();
	DWORD dwDiff = (DWORD)( tCurTime - pRegion->tEnvTransitionBegin );
	if ( dwDiff < ENV_TRANSITION_DELAY_MS )
		dwDiff = 0;
	else
		dwDiff -= ENV_TRANSITION_DELAY_MS;
	pRegion->fEnvTransitionPhase = (float)dwDiff / (float)ENV_TRANSITION_DURATION_MS;
	ASSERT( pRegion->fEnvTransitionPhase >= 0.f );
	if ( pRegion->fEnvTransitionPhase > 1.0f && !pRegion->bSavePrevEnvironmentDef )
	{
		// end transition
		e_ReleaseEnvironmentDef( pRegion->nPrevEnvironmentDef );
		pRegion->nPrevEnvironmentDef = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_EnvironmentInTransition( int nOverrideRegion )
{
	const REGION * pRegion;
	if ( nOverrideRegion != INVALID_ID )
		pRegion = e_RegionGet( nOverrideRegion );
	else
		pRegion = e_GetCurrentRegion();
	ASSERT_RETFALSE( pRegion );
	return INVALID_ID != pRegion->nPrevEnvironmentDef;
}

int e_EnvironmentLoad( const char * pszEnvironment, BOOL bForceSync /*= FALSE*/, BOOL bForceIgnoreWarnIfMissing /*= FALSE*/ )
{
	ASSERT_RETINVALID( pszEnvironment );
	if ( pszEnvironment[0] == NULL )
		return INVALID_ID;
	int nID = DefinitionGetIdByName( DEFINITION_GROUP_ENVIRONMENT, pszEnvironment, -1, bForceSync, bForceIgnoreWarnIfMissing );
	ASSERT_RETINVALID( nID != INVALID_ID );

	// If it's already been loaded (and unloaded), it will be ready and waiting.  However, it won't
	// call the restore callback if it was already loaded, so do it here.
	// CHB 2007.09.03 - Do this only once per unload-load cycle.
	// EnvDefinitionPostXMLLoad() initializes global specular lights, among other things.
	ENVIRONMENT_DEFINITION * pDef = static_cast<ENVIRONMENT_DEFINITION *>(DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nID ));
	if ((pDef != 0) && ((pDef->dwFlags & ENVIRONMENT_FLAG_RESTORED) == 0))
	{
		EnvDefinitionPostXMLLoad( pDef, bForceSync );
	}

	return nID;
}

int e_SkyboxLoad( const char * pszSkybox, BOOL bForceSync /*= FALSE*/, BOOL bForceIgnoreWarnIfMissing /*= FALSE*/ )
{
	ASSERT_RETINVALID( pszSkybox );
	int nID = DefinitionGetIdByName( DEFINITION_GROUP_SKYBOX, pszSkybox, -1, bForceSync, bForceIgnoreWarnIfMissing );
	ASSERT_RETINVALID( nID != INVALID_ID );

	// If it's already been loaded (and unloaded), it will be ready and waiting.  However, it won't
	// call the restore callback if it was already loaded, so do it here.
	void * pDef = DefinitionGetById( DEFINITION_GROUP_SKYBOX, nID );
	if ( pDef )
	{
		SkyboxDefinitionPostXMLLoad( pDef, bForceSync );
	}

	return nID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_EnvUseBlobShadows( ENVIRONMENT_DEFINITION * pEnvDef /*= NULL*/ )
{
	if ( ! pEnvDef )
		pEnvDef = e_GetCurrentEnvironmentDef();
	ASSERT_RETFALSE( pEnvDef );

	return !!( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_USE_BLOB_SHADOWS );
}
