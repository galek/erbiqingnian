//----------------------------------------------------------------------------
// dx9_environment.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_definition.h"

#include "dxC_irradiance.h"
#include "dxC_effect.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_query.h"
#include "dxC_drawlist.h"
#include "dxC_main.h"
#include "dxC_profile.h"
#include "filepaths.h"
#include "pakfile.h"
#include "perfhier.h"
#include "ptime.h"
#include "appcommontimer.h"
#include "dxC_render.h"
#include "dxC_environment.h"
#include "dxC_portal.h"
#include "dxC_texture.h"
#include "dxC_meshlist.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int sLoadEnvironmentEnvMap( const char * pszEnvPath, const char * pszFilename, TEXTURE_GROUP eGroup, BOOL bForceSyncLoad = FALSE )
{
	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", FILE_PATH_BACKGROUND, pszEnvPath, pszFilename );

	int nTextureID;
	V( e_CubeTextureNewFromFile( &nTextureID, szFilePath, eGroup, bForceSyncLoad ) );
	return nTextureID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void sReleaseEnvironmentEnvMap( int nEnvMapTextureID )
{
	if ( c_GetToolMode() )
		e_CubeTextureRemove( nEnvMapTextureID );
	else
		e_CubeTextureReleaseRef( nEnvMapTextureID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template< int O > void sComputeEnvironmentEnvMapSHCoefs( ENVIRONMENT_DEFINITION * pEnvDef, int nEnvMapTextureID, SH_COEFS_RGB<O> * ptSHCoefs, BOOL bForceLinear )
{
	D3D_CUBETEXTURE * pCubeTex = dx9_CubeTextureGet( nEnvMapTextureID );
	if ( ! pCubeTex )
		return;

	// turn it into SH coefs
	dx9_SHCoefsInit( ptSHCoefs );
	V( dx9_SHCoefsAddEnvMap( ptSHCoefs, nEnvMapTextureID, bForceLinear ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_EnvironmentApplyCoefs( ENVIRONMENT_DEFINITION * pEnvDef )
{
	if ( ! pEnvDef )
		return;

	// load the lighting env map and generate coefs
	if ( pEnvDef->szBackgroundLightingEnvMapFileName[ 0 ] )
	{
		if ( 0 == ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS ) )
		{
			if ( pEnvDef->nBackgroundLightingEnvMap != INVALID_ID && e_CubeTextureIsLoaded( pEnvDef->nBackgroundLightingEnvMap ) )
			{
				dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefs_O2 );
				dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefs_O3 );
				dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefsLin_O2 );
				dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefsLin_O3 );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nBackgroundLightingEnvMap, &pEnvDef->tBackgroundSHCoefs_O2, FALSE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nBackgroundLightingEnvMap, &pEnvDef->tBackgroundSHCoefs_O3, FALSE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nBackgroundLightingEnvMap, &pEnvDef->tBackgroundSHCoefsLin_O2, TRUE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nBackgroundLightingEnvMap, &pEnvDef->tBackgroundSHCoefsLin_O3, TRUE );
				pEnvDef->dwDefFlags |= ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS;
				sReleaseEnvironmentEnvMap( pEnvDef->nBackgroundLightingEnvMap );
				pEnvDef->nBackgroundLightingEnvMap = INVALID_ID;
			}
		}
	}

	if ( pEnvDef->szAppearanceLightingEnvMapFileName[ 0 ] )
	{
		if ( 0 == ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS ) )
		{
			if ( pEnvDef->nAppearanceLightingEnvMap != INVALID_ID && e_CubeTextureIsLoaded( pEnvDef->nAppearanceLightingEnvMap ) )
			{
				dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefs_O2 );
				dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefs_O3 );
				dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefsLin_O2 );
				dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefsLin_O3 );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nAppearanceLightingEnvMap, &pEnvDef->tAppearanceSHCoefs_O2, FALSE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nAppearanceLightingEnvMap, &pEnvDef->tAppearanceSHCoefs_O3, FALSE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nAppearanceLightingEnvMap, &pEnvDef->tAppearanceSHCoefsLin_O2, TRUE );
				sComputeEnvironmentEnvMapSHCoefs( pEnvDef, pEnvDef->nAppearanceLightingEnvMap, &pEnvDef->tAppearanceSHCoefsLin_O3, TRUE );
				pEnvDef->dwDefFlags |= ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS;
				sReleaseEnvironmentEnvMap( pEnvDef->nAppearanceLightingEnvMap );
				pEnvDef->nAppearanceLightingEnvMap = INVALID_ID;
			}
		}
	}

	if ( pEnvDef->nEnvMapTextureID != INVALID_ID && e_CubeTextureIsLoaded( pEnvDef->nEnvMapTextureID ) && pEnvDef->nEnvMapMIPLevels < 0 )
	{
		D3D_CUBETEXTURE * pTexture = dx9_CubeTextureGet( pEnvDef->nEnvMapTextureID );
		if ( pTexture && pTexture->pD3DTexture )
			pEnvDef->nEnvMapMIPLevels = dxC_GetNumMipLevels( pTexture->pD3DTexture );
	}
}

void e_EnvironmentApplyCoefs( int nEnvDefID )
{
	if ( nEnvDefID != INVALID_ID )
		e_EnvironmentApplyCoefs( (ENVIRONMENT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvDefID ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_RestoreEnvironmentDef( int nEnvDefID, BOOL bRestoreSkybox )
{
	ENVIRONMENT_DEFINITION * pEnvDef = (ENVIRONMENT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvDefID );
	ASSERT( pEnvDef );
	if ( ! pEnvDef )
		return;

	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrGetPath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, pEnvDef->tHeader.pszName );


	// normalize vectors
	for ( int i = 0; i < NUM_ENV_DIR_LIGHTS; i++ )
	{
		VectorNormalize( pEnvDef->tDirLights[ i ].vVec );
	}
	VectorNormalize( pEnvDef->vWindDirection );


	// load the lighting env map and generate coefs
	if ( pEnvDef->szBackgroundLightingEnvMapFileName[ 0 ] )
	{
		if ( 0 == ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS ) )
		{
			dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefs_O2 );
			dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefs_O3 );
			dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefsLin_O2 );
			dx9_SHCoefsInit( &pEnvDef->tBackgroundSHCoefsLin_O3 );

			// force sync the background SH environment maps?
			//BOOL bForceSync = TRUE;
			BOOL bForceSync = FALSE;

			//if ( ! c_GetToolMode() )
			//	e_CubeTextureReleaseRef( pEnvDef->nBackgroundLightingEnvMap );
			if ( pEnvDef->nBackgroundLightingEnvMap == INVALID_ID )
			{
				pEnvDef->nBackgroundLightingEnvMap = sLoadEnvironmentEnvMap( szPath, pEnvDef->szBackgroundLightingEnvMapFileName, TEXTURE_GROUP_BACKGROUND, bForceSync );
				e_CubeTextureAddRef( pEnvDef->nBackgroundLightingEnvMap );
			}
		}
	}
	else
	{
		pEnvDef->dwDefFlags &= ~(ENVIRONMENTDEF_FLAG_HAS_BG_SH_COEFS);
	}

	if ( pEnvDef->szAppearanceLightingEnvMapFileName[ 0 ] )
	{
		if ( 0 == ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS ) )
		{
			dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefs_O2 );
			dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefs_O3 );
			dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefsLin_O2 );
			dx9_SHCoefsInit( &pEnvDef->tAppearanceSHCoefsLin_O3 );

			//if ( ! c_GetToolMode() )
			//	e_CubeTextureReleaseRef( pEnvDef->nAppearanceLightingEnvMap );
			if ( pEnvDef->nAppearanceLightingEnvMap == INVALID_ID )
			{
				pEnvDef->nAppearanceLightingEnvMap = sLoadEnvironmentEnvMap( szPath, pEnvDef->szAppearanceLightingEnvMapFileName, TEXTURE_GROUP_UNITS );
				e_CubeTextureAddRef( pEnvDef->nAppearanceLightingEnvMap );
			}
		}
	}
	else
	{
		pEnvDef->dwDefFlags &= ~(ENVIRONMENTDEF_FLAG_HAS_APP_SH_COEFS);
	}



	// load the reflection env map
	if ( pEnvDef->szEnvMapFileName[ 0 ] && pEnvDef->nEnvMapTextureID == INVALID_ID )
	{
		// remove the old reflection environment map
		//if ( ! c_GetToolMode() )
		//	e_CubeTextureReleaseRef( pEnvDef->nEnvMapTextureID );
		//pEnvDef->nEnvMapTextureID = INVALID_ID;

		// CHB 2006.10.16 - Check to see if envmap will be used.
		if (e_TextureSlotIsUsed(TEXTURE_ENVMAP))
		{
			char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", FILE_PATH_BACKGROUND, szPath, pEnvDef->szEnvMapFileName );

			V( e_CubeTextureNewFromFile( &pEnvDef->nEnvMapTextureID, szFilePath, TEXTURE_GROUP_BACKGROUND ) );
			e_CubeTextureAddRef( pEnvDef->nEnvMapTextureID );
			pEnvDef->nEnvMapMIPLevels = -1;
		}
	}


	if ( bRestoreSkybox && pEnvDef->nSkyboxDefID == INVALID_ID && pEnvDef->szSkyBoxFileName[0] )
	{
		pEnvDef->nSkyboxDefID = e_SkyboxLoad( pEnvDef->szSkyBoxFileName );
	}

	// just in case they were sync loaded or already loaded
	e_EnvironmentApplyCoefs( pEnvDef );

	// load the shaders for this environment
	if ( ! c_GetToolMode() && ! e_IsNoRender() )
	{
		V( e_LoadAllShadersOfType( pEnvDef->nLocation ) );
	}

	int nMaxActive = MAX_SPECULAR_LIGHTS_PER_EFFECT;
	if ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING )
	{
		// limit selection to the 1 best light
		nMaxActive = 1;
	}
	gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ].SetMaxActive( nMaxActive );

	// CHB 2007.09.03
	pEnvDef->dwFlags |= ENVIRONMENT_FLAG_RESTORED;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_EnvironmentReleaseAll()
{
	int nEnvDef = DefinitionGetFirstId( DEFINITION_GROUP_ENVIRONMENT );
	BOUNDED_WHILE ( nEnvDef != INVALID_ID, 100000 )
	{
		e_ReleaseEnvironmentDef( nEnvDef );
		nEnvDef = DefinitionGetNextId( DEFINITION_GROUP_ENVIRONMENT, nEnvDef );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ReleaseEnvironmentDef( int nEnvDefID )
{
	ENVIRONMENT_DEFINITION * pEnvDef = NULL;
	if ( nEnvDefID != INVALID_ID )
		pEnvDef = (ENVIRONMENT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nEnvDefID );
	else
		pEnvDef = e_GetCurrentEnvironmentDef();
	ASSERT_RETURN( pEnvDef );

	// CHB 2007.09.03
	pEnvDef->dwFlags &= ~ENVIRONMENT_FLAG_RESTORED;

	e_CubeTextureReleaseRef( pEnvDef->nBackgroundLightingEnvMap );
	e_CubeTextureReleaseRef( pEnvDef->nAppearanceLightingEnvMap );
	e_CubeTextureReleaseRef( pEnvDef->nEnvMapTextureID );
	pEnvDef->nBackgroundLightingEnvMap = INVALID_ID;
	pEnvDef->nAppearanceLightingEnvMap = INVALID_ID;
	pEnvDef->nEnvMapTextureID = INVALID_ID;

	e_SkyboxRelease( pEnvDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sSetEnvironmentViewParams()
{
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if( ! pEnvDef )
		return;

	// Fog and far clip
	e_SetFarClipPlane( e_GetClipDistance( pEnvDef ) );
	e_SetFogDistances( e_GetFogStartDistance( pEnvDef ), e_GetSilhouetteDistance( pEnvDef ) );
	e_SetFogMaxDensity( 1.0f );
	e_SetFogColor( e_GetFogColorFromEnv( pEnvDef ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetEnvironmentDef( int nRegion, const char * pszEnvFilePath, const char * pszEnvFilePathBackup, BOOL bTransition /*= FALSE*/, BOOL bForceSyncLoad /*= FALSE*/ )
{
	REGION * pRegion = e_RegionGet( nRegion );
	ASSERT_RETURN( pRegion );

	ASSERT_RETURN( pszEnvFilePath );
	if ( !pszEnvFilePath[ 0 ] )
		pszEnvFilePath = DEFAULT_ENVIRONMENT_FILE;

	// NOTE:  currently environment transition is DISABLED
	//bTransition = FALSE;

	int nTempEnvDef = e_EnvironmentLoad( pszEnvFilePath, bForceSyncLoad );
	if(nTempEnvDef == INVALID_ID && pszEnvFilePathBackup && pszEnvFilePathBackup[0])
	{
		nTempEnvDef = e_EnvironmentLoad( pszEnvFilePathBackup, bForceSyncLoad );
	}

	if ( nTempEnvDef != INVALID_ID && bForceSyncLoad )
	{
		// Make sure it loaded.
		ASSERTV( "Environment sync load failed!\nPath: %s\nBackup: %s", pszEnvFilePath, pszEnvFilePathBackup );
	}

	if ( nTempEnvDef != INVALID_ID && nTempEnvDef != pRegion->nEnvironmentDef )
	{
		ENVIRONMENT_DEFINITION * pTempEnvDef = (ENVIRONMENT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, nTempEnvDef );

		if ( pRegion->nEnvironmentDef == INVALID_ID )
			bTransition = FALSE;

		if ( bTransition )
		{
			// ensure that the both the "to" and "from" are fully loaded
			ENVIRONMENT_DEFINITION * pCurEnvDef = (ENVIRONMENT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nEnvironmentDef );
			if ( ! pCurEnvDef )
			{
				bTransition = FALSE;
			}
			else
			{
				if ( ! pTempEnvDef )
				{
					// OK, want to transition to the new env, but it isn't loaded yet...
					// delay the start until it's loaded
					pRegion->nWaitEnvironmentDef = nTempEnvDef;
					return;
				}
				else
				{
					// ok, set up the transition
					pRegion->fEnvTransitionPhase = -1.f;
					pRegion->nPrevEnvironmentDef = pRegion->nEnvironmentDef;
					pRegion->nEnvironmentDef = nTempEnvDef;
					pRegion->nWaitEnvironmentDef = INVALID_ID;
				}
			}
		}


		if ( ! bTransition )
		{
			ASSERT( pTempEnvDef );

			pRegion->nPrevEnvironmentDef = INVALID_ID;

			// if no transition, release
			//if ( pRegion->nEnvironmentDef != INVALID_ID && ! bTransition )
			//	e_ReleaseEnvironmentDef( pRegion->nEnvironmentDef );

			pRegion->nEnvironmentDef = nTempEnvDef;
			pRegion->nWaitEnvironmentDef = INVALID_ID;
		}
	}
	if( dxC_GetDevice() )
	{
		sSetEnvironmentViewParams();

		e_SetupDistanceSpring();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SkyboxResourceRelease( SKYBOX_DEFINITION * pSkyboxDef )
{
	ASSERT_RETURN( pSkyboxDef );

	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		if ( pSkyboxDef->pModels[ i ].nModelID == INVALID_ID )
			continue;

		if ( pSkyboxDef->pModels[ i ].dwFlags & SKYBOX_MODEL_FLAG_APPEARANCE )
		{
			V( e_AppearanceRemove( pSkyboxDef->pModels[ i ].nModelID ) );
		}
		else
		{
			V( e_ModelRemovePrivate( pSkyboxDef->pModels[ i ].nModelID ) );
		}
		pSkyboxDef->pModels[ i ].nModelID = INVALID_ID;
	}

	// mark as not loaded
	pSkyboxDef->bLoaded = FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SkyboxRelease( ENVIRONMENT_DEFINITION * pEnvDef )
{
	ASSERT( pEnvDef );

	if ( pEnvDef->nSkyboxDefID != INVALID_ID )
	{
		SKYBOX_DEFINITION * pSkyboxDef = (SKYBOX_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_SKYBOX, pEnvDef->nSkyboxDefID );
		if ( ! pSkyboxDef )
			return;

		e_SkyboxResourceRelease( pSkyboxDef );

		pEnvDef->nSkyboxDefID = INVALID_ID;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static int sRestoreBackgroundModel( const char * pszModelFile, const char * pszFolder )
{
	int nDef = e_ModelDefinitionNewFromFile( pszModelFile, pszFolder );
	ASSERT_RETINVALID( nDef != INVALID_ID );
	int nModelID;
	V( e_ModelNew( &nModelID, nDef ) );

	//e_ModelSetScale( pSkyboxDef->nModelID, 1.0f );

	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERTX( pModel, "Skybox background model not found!" );
	if (!pModel)
	{
		return INVALID_ID;
	}

	// so that it doesn't draw unless I explicitly allow it
	//V( dx9_ModelSetFlags( pModel, MODEL_FLAG_NODRAW, TRUE ) );
	V( dxC_ModelSetDrawLayer( pModel, DRAW_LAYER_ENV ) );
	V( e_DPVS_ModelSetEnabled( pModel->nId, FALSE ) );

	return nModelID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sRestoreAppearanceModel( SKYBOX_DEFINITION * pSkyboxDef, SKYBOX_MODEL * pSkyboxModel, const char * pszFolder )
{
	V( e_AppearanceCreateSkyboxModel( pSkyboxDef, pSkyboxModel, pszFolder ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_SkyboxModelRestore( SKYBOX_DEFINITION * pSkyboxDef, SKYBOX_MODEL * pSkyboxModel, const char * pszFolder, BOOL bRestoreAppearance )
{
	// randomly selected not to load
	if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_FLAG_DO_NOT_LOAD )
		return S_FALSE;

	if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_FLAG_APPEARANCE && bRestoreAppearance )
	{
		sRestoreAppearanceModel( pSkyboxDef, pSkyboxModel, pszFolder );
	}
	else if ( 0 == ( pSkyboxModel->dwFlags & SKYBOX_MODEL_FLAG_APPEARANCE ) )
	{
		pSkyboxModel->nModelID = sRestoreBackgroundModel( pSkyboxModel->szModelFile, pszFolder );
	}
	if ( pSkyboxModel->nModelID != INVALID_ID )
	{
		V( e_ModelSetFlagbit( pSkyboxModel->nModelID, MODEL_FLAGBIT_NO_LOD_EFFECTS, TRUE ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sSkyboxResourceGetPath( SKYBOX_DEFINITION * pSkyboxDef, char * pszFolder, int nMaxLen )
{
	PStrGetPath( pszFolder, DEFAULT_FILE_WITH_PATH_SIZE, pSkyboxDef->tHeader.pszName );
}

static PRESULT sSkyboxFindRegion( SKYBOX_DEFINITION * pSkyboxDef )
{
	if ( e_RegionGet( pSkyboxDef->nRegionID ) )
		return S_OK;

	// find the environment(s) that loaded this skybox and get the proper region

	REGION * pRegion;
	for ( pRegion = e_RegionGetFirst();
		pRegion;
		pRegion = e_RegionGetNext( pRegion ) )
	{
		if ( e_IsNoRender() || c_GetToolMode() )
		{
			// hammer and fillpak do things weirdly -- let there be regions with missing environments and skyboxes with missing regions
			if ( pRegion->nEnvironmentDef == INVALID_ID )
				continue;
		}
		ASSERTV_CONTINUE( pRegion->nEnvironmentDef != INVALID_ID, "Region missing environment!\nIn skybox: %s", pSkyboxDef->tHeader.pszName );
		ENVIRONMENT_DEFINITION * pEnvDef = (ENVIRONMENT_DEFINITION *)DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pRegion->nEnvironmentDef );
		if ( ! pEnvDef )
			continue;
		if ( pEnvDef->nSkyboxDefID == pSkyboxDef->tHeader.nId )
		{
			pSkyboxDef->nRegionID = pRegion->nId;
			break;
		}
	}

	if ( ! ( c_GetToolMode() || AppCommonGetFillPakFile() ) )
	{
		//ASSERTV( pRegion, "Couldn't find environment for skybox!\n%s", pSkyboxDef->tHeader.pszName );
		return S_FALSE;
	}
	//else if ( pSkyboxDef->nRegionID < 0 )
	//{
	//	pSkyboxDef->nRegionID = 0;
	//}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sSkyboxRestoreCallback( void * pDef )
{
	SKYBOX_DEFINITION * pSkyboxDef = (SKYBOX_DEFINITION *)pDef;
	ASSERT_RETFALSE( pDef );

	ASSERTV_RETFALSE( TESTBIT_DWORD(pSkyboxDef->tHeader.dwFlags, DHF_EXISTS_BIT), "Missing skybox XML!\n%s", pSkyboxDef->tHeader.pszName );

	V( sSkyboxFindRegion( pSkyboxDef ) );

	// select which models will appear based on rands and their chance to show
	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		// clear the flag, in case we're loading the same one over and over
		pSkyboxDef->pModels[ i ].dwFlags &= ~ SKYBOX_MODEL_FLAG_DO_NOT_LOAD;

		float fChance = SATURATE( pSkyboxDef->pModels[ i ].fChance );
		float fRoll = RandGetFloat( e_GetEngineOnlyRand(), 0.f, 1.f );
		if ( fChance < fRoll )
		{
			// aww, you lose
			pSkyboxDef->pModels[ i ].dwFlags |= SKYBOX_MODEL_FLAG_DO_NOT_LOAD;
		}
	}

	e_SkyboxResourceRestore( pSkyboxDef );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SkyboxResourceRestore( SKYBOX_DEFINITION * pSkyboxDef  )
{
	ASSERT_RETURN( pSkyboxDef );
	if ( ! e_IsNoRender() )
	{
		if ( ! e_RegionGet( pSkyboxDef->nRegionID ) )
		{
			V( sSkyboxFindRegion( pSkyboxDef ) );
		}
		if ( ! e_RegionGet( pSkyboxDef->nRegionID ) )
		{
			return;
		}
	}

	char szFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	sSkyboxResourceGetPath( pSkyboxDef, szFolder, DEFAULT_FILE_WITH_PATH_SIZE );

	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		if ( pSkyboxDef->pModels[ i ].nModelID == INVALID_ID && pSkyboxDef->pModels[ i ].szModelFile[ 0 ] != NULL )
		{
			// load background models, but not appearances (yet)
			if ( c_GetToolMode() )
			{
				V( e_SkyboxModelRestore( pSkyboxDef, &pSkyboxDef->pModels[ i ], szFolder, FALSE ) );
			}
			else
			{
				V( e_SkyboxModelRestore( pSkyboxDef, &pSkyboxDef->pModels[ i ], szFolder, TRUE ) );
			}
		}
	}

	pSkyboxDef->bLoaded = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SkyboxSetLocation( SKYBOX_DEFINITION * pSkyboxDef, VECTOR vPos )
{
	ASSERT_RETURN( pSkyboxDef );

	// Don't bother in fillpak mode.
	if ( e_IsNoRender() )
		return;

	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		if ( 0 == ( pSkyboxDef->pModels[ i ].dwFlags & SKYBOX_MODEL_DEF_FLAG_CENTER_ON_CAMERA_XY ) && 
			 e_IsValidModelID( pSkyboxDef->pModels[ i ].nModelID ) )
		{
			VECTOR vNewPos = vPos + pSkyboxDef->pModels[ i ].vOffset;
			MATRIX matWorld;
			MatrixTranslation( &matWorld, &vNewPos );
			V( e_ModelSetLocationPrivate( pSkyboxDef->pModels[ i ].nModelID, &matWorld, vNewPos ) );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSkyboxGetCurrent( SKYBOX_DEFINITION *& pSkyboxDef )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SKYBOX ) )
		return S_FALSE;

	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return S_FALSE;

	if ( pEnvDef->nSkyboxDefID == INVALID_ID )
		return S_FALSE;

	pSkyboxDef = (SKYBOX_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_SKYBOX, pEnvDef->nSkyboxDefID );
	if ( !pSkyboxDef )
	{
		return S_FALSE;
	}
	if ( ! pSkyboxDef->bLoaded )
	{
		e_SkyboxResourceRestore( pSkyboxDef );
		if ( ! pSkyboxDef->bLoaded )
			return S_FALSE;
	}

	REGION * pRegion = e_GetCurrentRegion();
	ASSERT_RETFAIL( pRegion );
	e_SkyboxSetLocation( pSkyboxDef, pRegion->vOffset );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sSkyboxDrawPass(
	RENDER_CONTEXT & tContext,
	int nSkyboxPass )
{
	D3D_PROFILE_REGION( L"Skybox" );

	SKYBOX_DEFINITION * pSkyboxDef = NULL;

	PRESULT pr;
	V_SAVE( pr, sSkyboxGetCurrent( pSkyboxDef ) );
	if ( FAILED(pr) )
		return pr;
	if ( S_FALSE == pr )
		return pr;


	char szFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	sSkyboxResourceGetPath( pSkyboxDef, szFolder, DEFAULT_FILE_WITH_PATH_SIZE );

	//e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );

	// WORKAROUND: make sure samplers are in a good known state here
	V( dx9_ResetSamplerStates() );

	const DRAWLIST_STATE * pOrigState = &tContext.tState;
	ASSERT_RETFAIL( pOrigState );

	int nPass = nSkyboxPass;
	ASSERT( nPass >= 0 && nPass < NUM_SKYBOX_PASSES );

	MATRIX matOrigWorld, matWorld, matOrigView, matView, matProj;
	VECTOR4 vEyeDirection, vEyeLocation;
	//e_InitMatrices( &matOrigWorld, &matOrigView, &matProj, NULL, (VECTOR*)&vEyeDirection, (VECTOR*)&vEyeLocation );

	MatrixIdentity( &matOrigWorld );
	matOrigView = tContext.matView;
	matProj = tContext.matProj;
	vEyeDirection = tContext.vEyeDir_w;
	vEyeLocation = tContext.vEyePos_w;


	// workaround for skybox models clipping out -- this value should be detected
	e_SetFarClipPlane( 3000.f );
	float Q = e_GetFarClipPlane()/(e_GetFarClipPlane() - e_GetNearClipPlane());
	matProj.m[2][2] = Q;
	matProj.m[3][2] = -Q * e_GetNearClipPlane();
	matProj.m[2][3] = 1.f;

	//if ( pOrigState->pmatProj )
	//	matProj = *pOrigState->pmatProj;

	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		SKYBOX_MODEL * pSkyboxModel = &pSkyboxDef->pModels[ i ];

		// only process the models from the current pass
		if ( pSkyboxModel->nPass < 0 || pSkyboxModel->nPass >= NUM_SKYBOX_PASSES )
			continue;
		if ( pSkyboxModel->nPass != nPass )
			continue;

		if ( pSkyboxModel->nModelID == INVALID_ID )
		{
			V( e_SkyboxModelRestore( pSkyboxDef, pSkyboxModel, szFolder, TRUE ) );
		}

		if ( pSkyboxModel->nModelID == INVALID_ID )
			continue;

		matWorld = matOrigWorld;
		matView  = matOrigView;
		DRAWLIST_STATE tCurState = *pOrigState;
		tCurState.bAllowInvisibleModels	= TRUE;
		tCurState.bLights				= FALSE;


		// fog if the model wants it
		V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) && ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_FOG ) ) );

		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_FOG )
		{
			e_SetFogDistances( (float)pSkyboxModel->nFogStart, (float)pSkyboxModel->nFogEnd );
			VECTOR3 vColor = *(VECTOR*)&pSkyboxModel->tFogColor.GetValue( 0.5f );
			DWORD dwColor = ARGB_MAKE_FROM_FLOAT( vColor.x, vColor.y, vColor.z, 0.0f );
			e_SetFogColor( dwColor );
		}

		D3D_MODEL* pModel = dx9_ModelGet( pSkyboxModel->nModelID );
		ASSERT_CONTINUE(pModel);

		BOUNDING_BOX tBBox;
		V( e_GetModelRenderAABBInObject( pSkyboxModel->nModelID, &tBBox ) );
		VECTOR vCenter;
		vCenter.fX = -( tBBox.vMax.fX + tBBox.vMin.fX ) * 0.5f;
		vCenter.fY = -( tBBox.vMax.fY + tBBox.vMin.fY ) * 0.5f;
		vCenter.fZ = 0.0f;
		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_FLAG_APPEARANCE )
			vCenter.fZ = pModel->vPosition.z;
		*(VECTOR*)&matWorld._41 = vCenter;

		//MATRIX matOldWorld = pModel->matScaledWorld;

		VECTOR vZero(0);
		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_CENTER_ON_CAMERA_XY )
		{
			V( dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_WORLD, TRUE ) );
			pModel->matScaledWorld = matWorld;
			tCurState.pvEyeLocation = &vZero;

			// zero out view transform
			*(VECTOR*)&matView._41 = VECTOR( 0, 0, 0 );

			dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
			dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );

			V( dxC_ResetViewport( TRUE ) );
		} else
		{
			// add the region offset
			//*(VECTOR*)&pModel->matScaledWorld._41 += pRegion->vOffset;
			//*(VECTOR*)&matWorld._41 = *(VECTOR*)&pModel->matScaledWorld._41 + pRegion->vOffset;
			//pModel->matScaledWorld = matWorld;
			//dx9_ModelSetFlags( pModel, MODEL_FLAG_WORLD, TRUE );

			//dx9_ModelSetFlags( pModel, MODEL_FLAG_WORLD, FALSE );

			dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
			dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );

			V( dxC_ResetViewport( FALSE ) );
		}

		D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
			e_ModelGetDisplayLOD( pSkyboxModel->nModelID ) );
		if ( !pModelDefinition )
			continue;
		// APE 2007.02.27 - Removed this because model definitions now load in asynchronously.
		//ASSERTONCE_RETVAL( pModelDefinition, E_FAIL );

		// Setup the world, view, and projection matrices

		tCurState.pmatView = &matView;
		tCurState.pmatProj = &matProj;
		tCurState.bAllowInvisibleModels = TRUE;

		V( dx9_RenderModel( &tCurState, pSkyboxModel->nModelID ) );

		// restore old world matrix (this is kludgy)
		//pModel->matScaledWorld = matOldWorld;
	}

	V( e_ResetClipAndFog() );
	V( dxC_ResetViewport( FALSE ) );

	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE  );
	dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SkyboxRenderCallback( void * pUserData )
{
	ASSERT_RETINVALIDARG( pUserData );

	SKYBOX_CALLBACK_DATA * pData = (SKYBOX_CALLBACK_DATA*)pUserData;

	RENDER_CONTEXT & tContext = *pData->pContext;
	int nSkyboxPass = pData->nSkyboxPass;

	return sSkyboxDrawPass( tContext, nSkyboxPass );
}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RenderSkybox( int nDrawList, D3D_DRAWLIST_COMMAND * pCommand )
{
	D3D_PROFILE_REGION( L"Skybox" );

	if ( ! e_GetRenderFlag( RENDER_FLAG_SKYBOX ) )
		return S_FALSE;

	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return S_FALSE;

	if ( pEnvDef->nSkyboxDefID == INVALID_ID )
		return S_FALSE;

	SKYBOX_DEFINITION * pSkyboxDef = (SKYBOX_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_SKYBOX, pEnvDef->nSkyboxDefID );
	if ( !pSkyboxDef )
	{
		return S_FALSE;
	}
	if ( ! pSkyboxDef->bLoaded )
	{
		e_SkyboxResourceRestore( pSkyboxDef );
		if ( ! pSkyboxDef->bLoaded )
			return S_FALSE;
	}

	REGION * pRegion = e_GetCurrentRegion();
	ASSERT_RETFAIL( pRegion );
	e_SkyboxSetLocation( pSkyboxDef, pRegion->vOffset );

	char szFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
	sSkyboxResourceGetPath( pSkyboxDef, szFolder, DEFAULT_FILE_WITH_PATH_SIZE );

	//e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );

	// WORKAROUND: make sure samplers are in a good known state here
	V( dx9_ResetSamplerStates() );

	const DRAWLIST_STATE * pOrigState = dx9_GetDrawListState( nDrawList );
	ASSERT_RETFAIL( pOrigState );

	int nPass = pCommand->nID;
	ASSERT( nPass >= 0 && nPass < NUM_SKYBOX_PASSES );

	MATRIX matOrigWorld, matWorld, matOrigView, matView, matProj;
	VECTOR4 vEyeDirection, vEyeLocation;
	e_InitMatrices( &matOrigWorld, &matOrigView, &matProj, NULL, (VECTOR*)&vEyeDirection, (VECTOR*)&vEyeLocation );

	if ( pOrigState->pmatProj )
		matProj = *pOrigState->pmatProj;

	//float fFarClip  = e_GetFarClipPlane();
	//float fNearClip = e_GetNearClipPlane();
	//float fFarFog, fNearFog;
	//e_GetFogDistances( fNearFog, fFarFog );

	for ( int i = 0; i < pSkyboxDef->nModelCount; i++ )
	{
		SKYBOX_MODEL * pSkyboxModel = &pSkyboxDef->pModels[ i ];

		// only process the models from the current pass
		if ( pSkyboxModel->nPass < 0 || pSkyboxModel->nPass >= NUM_SKYBOX_PASSES )
			continue;
		if ( pSkyboxModel->nPass != nPass )
			continue;

		if ( pSkyboxModel->nModelID == INVALID_ID )
		{
			V( e_SkyboxModelRestore( pSkyboxDef, pSkyboxModel, szFolder, TRUE ) );
		}

		if ( pSkyboxModel->nModelID == INVALID_ID )
			continue;

		matWorld = matOrigWorld;
		matView  = matOrigView;
		DRAWLIST_STATE tCurState = *pOrigState;

		// fog if the model wants it
		V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) && ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_FOG ) ) );

		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_FOG )
		{
			e_SetFogDistances( (float)pSkyboxModel->nFogStart, (float)pSkyboxModel->nFogEnd );
			VECTOR3 vColor = *(VECTOR*)&pSkyboxModel->tFogColor.GetValue( 0.5f );
			DWORD dwColor = ARGB_MAKE_FROM_FLOAT( vColor.x, vColor.y, vColor.z, 0.0f );
			e_SetFogColor( dwColor );
		}

		D3D_MODEL* pModel = dx9_ModelGet( pSkyboxModel->nModelID );
		ASSERT_RETFAIL(pModel);

		BOUNDING_BOX tBBox;
		V( e_GetModelRenderAABBInObject( pSkyboxModel->nModelID, &tBBox ) );
		VECTOR vCenter;
		vCenter.fX = -( tBBox.vMax.fX + tBBox.vMin.fX ) * 0.5f;
		vCenter.fY = -( tBBox.vMax.fY + tBBox.vMin.fY ) * 0.5f;
		vCenter.fZ = 0.0f;
		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_FLAG_APPEARANCE )
			vCenter.fZ = pModel->vPosition.z;
		*(VECTOR*)&matWorld._41 = vCenter;

		//MATRIX matOldWorld = pModel->matScaledWorld;

		VECTOR vZero(0);
		if ( pSkyboxModel->dwFlags & SKYBOX_MODEL_DEF_FLAG_CENTER_ON_CAMERA_XY )
		{
			V( dxC_ModelSetFlagbit( pModel, MODEL_FLAGBIT_WORLD, TRUE ) );
			pModel->matScaledWorld = matWorld;
			tCurState.pvEyeLocation = &vZero;

			// zero out view transform
			*(VECTOR*)&matView._41 = VECTOR( 0, 0, 0 );
		} else
		{
			// add the region offset
			//*(VECTOR*)&pModel->matScaledWorld._41 += pRegion->vOffset;
			//*(VECTOR*)&matWorld._41 = *(VECTOR*)&pModel->matScaledWorld._41 + pRegion->vOffset;
			//pModel->matScaledWorld = matWorld;
			//dx9_ModelSetFlags( pModel, MODEL_FLAG_WORLD, TRUE );

			//dx9_ModelSetFlags( pModel, MODEL_FLAG_WORLD, FALSE );
			
		}

		D3D_MODEL_DEFINITION* pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
													e_ModelGetDisplayLOD( pSkyboxModel->nModelID ) );
		if ( !pModelDefinition )
			continue;
		// APE 2007.02.27 - Removed this because model definitions now load in asynchronously.
		//ASSERTONCE_RETVAL( pModelDefinition, E_FAIL );
		
		// Setup the world, view, and projection matrices

		tCurState.pmatView = &matView;
		tCurState.pmatProj = &matProj;
		tCurState.bAllowInvisibleModels = TRUE;

		V( dx9_RenderModel( &tCurState, pSkyboxModel->nModelID ) );

		// restore old world matrix (this is kludgy)
		//pModel->matScaledWorld = matOldWorld;
	}

	V( e_ResetClipAndFog() );

	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE  );
	dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL EnvDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad )
{
	ASSERTX_RETFALSE( pDef, "Environment XML file load failed!" );

	e_RestoreEnvironmentDef( ( (ENVIRONMENT_DEFINITION*)pDef )->tHeader.nId, TRUE );

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL SkyboxDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad )
{
	ASSERTX_RETFALSE( pDef, "Skybox XML file load failed!" );

	return sSkyboxRestoreCallback( pDef );
}
