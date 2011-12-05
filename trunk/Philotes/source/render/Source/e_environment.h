//----------------------------------------------------------------------------
// e_environment.h
//
// - Header for environment functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_ENVIRONMENT__
#define __E_ENVIRONMENT__

#include "e_definition.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define DEFAULT_ENVIRONMENT_FILE "Tools\\default_env.xml"

#define TOOL_MODE_CLIP_DISTANCE			3000.f

#define ENV_TRANSITION_DURATION_MS		3000
#define ENV_TRANSITION_DELAY_MS			500

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum
{
	SKYBOX_PASS_BACKDROP=0,
	SKYBOX_PASS_AFTER_MODELS,
	//SKYBOX_PASS_NEW_Z,
	NUM_SKYBOX_PASSES
};

enum
{
	LEVEL_LOCATION_UNKNOWN		= -1,
	LEVEL_LOCATION_INDOOR		= 0,
	LEVEL_LOCATION_OUTDOOR		= 1,
	LEVEL_LOCATION_INDOORGRID	= 2,
	LEVEL_LOCATION_FLASHLIGHT	= 3,	// for file consistency
	NUM_LEVEL_LOCATIONS
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

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

const char * e_LevelLocationGetName( int nLocation );
void e_SetSkyboxFile( char * pszSkyboxFilePath );
int e_GetCurrentEnvironmentDefID( int nOverrideRegion = INVALID_ID );
struct ENVIRONMENT_DEFINITION * e_GetCurrentEnvironmentDef( int nOverrideRegion = INVALID_ID );
void e_RestoreEnvironmentDef( int nEnvDefID, BOOL bRestoreSkybox = FALSE );
void e_ReleaseEnvironmentDef( int nEnvDefID = INVALID_ID );
void e_SetCurrentEnvironmentDef( int nEnvDefID );
void e_SetEnvironmentDef( int nRegion, const char * pszEnvFilePath, const char * pszEnvFilePathBackup, BOOL bTransition = FALSE, BOOL bForceSyncLoad = FALSE );
void e_UpdateEnvironmentTransition();
BOOL e_EnvironmentInTransition( int nOverrideRegion = INVALID_ID );
int e_RegionGetLevelLocation( int nRegion );
int e_GetCurrentLevelLocation( int nOverrideRegion = INVALID_ID );
BOOL e_CurrentLocationUseOutdoorCulling( int nOverrideRegion = INVALID_ID );
void e_SetLevelLocationRenderFlags( int nLocation = LEVEL_LOCATION_UNKNOWN );
float e_GetSilhouetteDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
float e_GetClipDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
float e_GetFogStartDistance( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
float e_GetShadowIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
float e_GetAmbientIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
void e_GetAmbientColor( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR4 * pvAmbient, BOOL bForceCurrent = FALSE );
void e_GetFogColorVectorFromEnv( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR3 * pvColor, BOOL bForceCurrent = FALSE );
DWORD e_GetFogColorFromEnv( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
float e_GetDirLightIntensity( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, BOOL bForceCurrent = FALSE );
void e_GetDirLightVector( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, VECTOR * pDir, BOOL bForceCurrent = FALSE );
void e_GetDirLightColor( const ENVIRONMENT_DEFINITION * pEnvDef, ENV_DIR_LIGHT_TYPE eDirLight, VECTOR4 * pvColor, BOOL bForceCurrent = FALSE );
void e_GetHemiLightColors( const ENVIRONMENT_DEFINITION * pEnvDef, VECTOR4 * pvColors, BOOL bForceCurrent = FALSE );
float e_GetEnvMapFactor( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
int e_GetEnvMap( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
int e_GetEnvMapMIPLevels( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
DWORD e_EnvGetBackgroundColor( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bForceCurrent = FALSE );
void e_GetWind(
	const ENVIRONMENT_DEFINITION * pEnvDef,
	float & fMin,
	float & fMax,
	VECTOR & vDir,
	BOOL bForceCurrent = FALSE );
void e_EnvironmentApplyCoefs( ENVIRONMENT_DEFINITION * pEnvDef );
void e_EnvironmentApplyCoefs( int nEnvDefID );

PRESULT e_SkyboxModelRestore( SKYBOX_DEFINITION * pSkyboxDef, SKYBOX_MODEL * pSkyboxModel, const char * pszFolder, BOOL bRestoreAppearance );
void e_SkyboxResourceRestore( SKYBOX_DEFINITION * pSkyboxDef );
void e_SkyboxResourceRelease( SKYBOX_DEFINITION * pSkyboxDef );
void e_SkyboxRelease( ENVIRONMENT_DEFINITION * pEnvDef );
PRESULT e_EnvironmentReleaseAll();
BOOL EnvDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad );
BOOL SkyboxDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad );

int e_EnvironmentLoad( const char * pszEnvironment, BOOL bForceSync = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE );
int e_SkyboxLoad( const char * pszSkybox, BOOL bForceSync = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE );

BOOL e_EnvUseBlobShadows( ENVIRONMENT_DEFINITION * pEnvDef = NULL );

// seems awfully platform specific, but it's actually not so much
PRESULT e_LoadAllShadersOfType( int nShaderType );

#endif // __E_ENVIRONMENT__
