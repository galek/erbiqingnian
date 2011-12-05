#ifndef INCLUDE_PARTICLE
#define INCLUDE_PARTICLE
// c_particles.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef INTEL_CHANGES
#define PARTICLE_SYSTEM_ASYNC_MAX 3
#define VERTICES_PER_ASYNC_PARTICLE 6
#define PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX (35000 * VERTICES_PER_ASYNC_PARTICLE)
#endif

typedef enum PARTICLE_SYSTEM_PARAM
{
	PARTICLE_SYSTEM_PARAM_HOSE_PRESSURE,
	PARTICLE_SYSTEM_PARAM_CIRCLE_RADIUS,
	PARTICLE_SYSTEM_PARAM_NOVA_START,
	PARTICLE_SYSTEM_PARAM_NOVA_ANGLE,
	PARTICLE_SYSTEM_PARAM_RANGE_PERCENT,
	PARTICLE_SYSTEM_PARAM_DURATION,
	PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDXY,
	PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDZ,
	PARTICLE_SYSTEM_PARAM_PARTICLE_SPAWN_THROTTLE,
};

#ifdef INTEL_CHANGES
//-------------------------------------------------------------------------------------------------
struct PARTICLE_QUAD_VERTEX
{
	VECTOR		vPosition;
	float		fRand; // per-particle random pct, labeled as psize
	DWORD		dwDiffuse;
	DWORD		dwGlow; // labeled as specular
	float		pfTextureCoord0[2];
}; // 32 bytes... should be 32 for best cache performance!

#define VERTICES_PER_PARTICLE 6
#define TRIANGLES_PER_PARTICLE 2
extern PARTICLE_QUAD_VERTEX * sgpParticleSystemAsyncVerticesBuffer[ PARTICLE_SYSTEM_ASYNC_MAX ];
extern PARTICLE_QUAD_VERTEX * sgpParticleSystemAsyncMainThreadVerticesBuffer[ PARTICLE_SYSTEM_ASYNC_MAX ];
extern BOOL * sgbParticleSystemAsyncIndexAvailable;

#endif
//-------------------------------------------------------------------------------------------------
int c_ParticleSystemNew( 
	int nParticleSystemDefId, 
	const struct VECTOR * pvPosition = NULL, 
	const struct VECTOR * pvNormal = NULL, 
	ROOMID idRoom = INVALID_ID,
	int nParticleSystemPrev = INVALID_ID,
	const VECTOR * pvVelocity = NULL,
	BOOL bForceNonHavokFX = FALSE,
	BOOL bForceNonAsync = FALSE,
#ifdef INTEL_CHANGES
	BOOL bIsAsync = FALSE,
#endif
	const MATRIX * pmWorld = NULL );

int c_ParticleSystemAddToEnd( 
	int nParticleSystemFirstId,
	int nParticleSystemDefId,
	BOOL bForceNonHavokFX = FALSE,
	BOOL bForceNonAsync = FALSE );

void c_ParticleSystemDestroy ( 
	int nId,
#ifdef INTEL_CHANGES
	BOOL bDestroyNext = TRUE,
	BOOL bIsAsync = FALSE );
#else
	BOOL bDestroyNext = TRUE );
#endif

void c_ParticleSystemKill ( 
	int nId,
	BOOL bDestroyNext = TRUE,
#ifdef INTEL_CHANGES
	BOOL bForce = FALSE,
	BOOL bIsAsync = FALSE );
#else
	BOOL bForce = FALSE );
#endif

void c_ParticleSystemUpdateAll(
	struct GAME* pGame,
	float fTimeDelta );

void c_ParticleSystemsDestroyAll();

float c_ParticleSystemsGetLifePercent( 
	struct GAME * pGame, 
	int nId );

BOOL c_ParticleSystemHasRope( 
	int nId);

void c_ParticleSystemSetDrawLayer( 
	int nParticleSystemId, 
	BYTE bDrawLayer,
	BOOL bLock );

void c_ParticleSystemSetRegion( 
	int nParticleSystemId, 
	int nRegionID );

void c_ParticleSystemMove( 
	int nParticleSystemId, 
	const struct VECTOR& vPosition, 
	struct VECTOR* pvNormal = NULL, 
	ROOMID idRoom = INVALID_ID,
	const MATRIX * pmWorld = NULL );

void c_ParticleSystemsRestoreVBuffer();
void c_ParticleSystemsRestore();
void c_ParticleSystemsInit();
void c_ParticleSystemsDestroy();
PRESULT c_ParticlesDraw(	
	BYTE bGroup,
	const struct MATRIX * pmatView, 
	const struct MATRIX * pmatProj,
	const struct MATRIX * pmatProj2,
	const struct VECTOR & vEyeLocation,
	const struct VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly );

PRESULT c_ParticlesDrawLightmap(	
	BYTE bGroup,
	const struct MATRIX * pmatView, 
	const struct MATRIX * pmatProj,
	const struct MATRIX * pmatProj2,
	const struct VECTOR & vEyeLocation,
	const struct VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly );

PRESULT c_ParticlesDrawPreLightmap(	
	BYTE bGroup,
	const struct MATRIX * pmatView, 
	const struct MATRIX * pmatProj,
	const struct MATRIX * pmatProj2,
	const struct VECTOR & vEyeLocation,
	const struct VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly );

int c_ParticlesGetUpdateTotal();

void c_ParticleSystemSetVisible( 
	int nParticleSystemId, 
	BOOL bSet );

void c_ParticleSystemWarpToCurrentPosition(
	int nParticleSystemId );

void c_ParticleSystemSetDraw( 
	int nParticleSystemId, 
	BOOL bDraw );

void c_ParticleSystemSetFirstPerson( 
	int nParticleSystemId, 
	BOOL bSet );

void c_ParticleSystemSetNoLights( 
	int nParticleSystemId, 
	BOOL bSet );

void c_ParticleSystemSetNoSound( 
	int nParticleSystemId, 
	BOOL bSet );

int c_ParticleSystemGetDefinition( 
	int nParticleSystemId );

BOOL c_ParticleSystemExists( 
	int nParticleSystemId );

BOOL c_ParticleSystemCanDestroy( 
	int nParticleSystemId );

void c_ParticleSystemSetRopeEnd( 
	int nParticleSystemId,
	const struct VECTOR & vPosition,
	const struct VECTOR & vNormal,
	BOOL bLockEnd = TRUE,
	BOOL bSetIdealPosition = TRUE,
	BOOL bIsFromParent = FALSE );

void c_ParticleSystemReleaseRopeEnd( 
	int nParticleSystemId);

int c_ParticleSystemGetRopeEndSystem( 
	int nParticleSystemId);

int c_ParticleSystemSetRopeEndSystem( 
	int nParticleSystemId,
	int nRopeEndSystemDefId );

void c_ParticleSystemSetParam( 
	int nParticleSystemId,
	PARTICLE_SYSTEM_PARAM eParam,
	float fValue);

void c_ParticleSystemSetVelocity( 
	int nParticleSystemId,
	const VECTOR & vVelocity);

void c_ParticleSystemSetRopeMaxBend( 
	int nParticleSystemId,
	float fMaxBendXY,
	float fMaxBendZ);

void c_ParticleSystemLoad( 
	int nParticleSystemDefId );

void c_ParticleSystemFlagSoundsForLoad( 
	int nParticleSystemDefId,
	BOOL bLoadNow);

void c_ParticleSetWind(
	float fMin,
	float fMax,
	const struct VECTOR & vWindDirection );

void c_ParticleGetWindFromEngine();

void c_ParticleSystemForceMoveParticles( 
	int nParticleSystemId,
	BOOL bForceMove );

void c_ParticleSetFogAdditiveLum(
	BOOL bEnabled );

typedef int (*FP_HOLODECK_GETMODELID)(void);

void c_SetHolodeckGetModelIdFunction(
	FP_HOLODECK_GETMODELID fpHolodeckGetModelId);

void c_ParticleToggleDebugMode();

void c_ParticleSetCullPriority( 
	int nCullPriority );
int c_ParticleGetCullPriority();

void c_ParticleSystemAddRef(
	int nParticleSystemDefId );

void c_ParticleSystemReleaseRef(
	int nParticleSystemDefId );

enum
{
	PARTICLE_GLOBAL_BIT_CENSORSHIP_ENABLED = 0,
	PARTICLE_GLOBAL_BIT_DX10_ENABLED,
	PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED,
};

BOOL c_ParticleTestGlobalBit(
	int nBit );

void c_ParticleSetGlobalBit(
	int nBit,
	BOOL bValue);

void c_ParticleEffectSetLoad(
	struct PARTICLE_EFFECT_SET * particle_effect_set,
	const char * pszFolder );


void c_ParticleSetFPS( float fps );
#if ISVERSION(DEVELOPMENT)
float c_ParticleGetDelta();
#endif
#endif