//----------------------------------------------------------------------------
// e_particle.h
//
// - Header for particle system render functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_PARTICLE__
#define __E_PARTICLE__

#include "e_common.h"
#include "interpolationpath.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define PARTICLE_MODEL_FILE_EXTENSION		"mp"
#define PARTICLE_MODEL_FILE_VERSION			(6 + GLOBAL_FILE_VERSION)
#define PARTICLE_MODEL_FILE_MAGIC_NUMBER	0xface5432

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum PARTICLE_DRAW_SECTION
{
	PARTICLE_DRAW_SECTION_INVALID = -1,
	PARTICLE_DRAW_SECTION_NORMAL = 0,
	// one for each draw layer in here...
	// mythos-only
	PARTICLE_DRAW_SECTION_PRELIGHTMAP = PARTICLE_DRAW_SECTION_NORMAL + NUM_DRAW_LAYERS,
	PARTICLE_DRAW_SECTION_LINEOFSIGHT,
	PARTICLE_DRAW_SECTION_LIGHTMAP,
	// count
	NUM_PARTICLE_DRAW_SECTIONS
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct PARTICLE;
struct PARTICLE_SYSTEM;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
#define HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS 3

struct PARTICLE_DRAW_INFO
{
	MATRIX matViewProjection;
	MATRIX matViewProjection2;
	MATRIX matView;
	MATRIX matProjection;

	int nVerticesInBuffer;
	int nFirstVertexOfBatch;

	int nMeshesInBatchCurr;

	int nTrianglesInBatchCurr;
	//void * pIndexBufferCurr;
	int nIndexBufferCurr;

	VECTOR vEyePosition;
	VECTOR vEyeDirection;
	VECTOR vUpCurrent;
	VECTOR vNormalCurrent;

	void* ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ];
	void* pParticlePositions;
	void* pParticleVelocities;

	DWORD dwIndexFormat;

	struct PARTICLE_SYSTEM_FOCUS_UNIT_INFO
	{
		VECTOR vPosition;
		float fRadius;
		float fHeight;
	};

	PARTICLE_SYSTEM_FOCUS_UNIT_INFO tFocusUnit;

	VECTOR vWindDirection;
	float fWindMagnitude;

	float fTotalScreenPredicted;
	float fScreensMax;

	struct PARTICLE_SYSTEM * pParticleSystemCurr;
	struct PARTICLE_SYSTEM * pParticleSystemCurr2;//?
	BOOL bPixelShader;

	//for fluid gpu sim
	void * pTechnique; //EFFECT_TECHNIQUE
	void * pEffect; //D3D_EFFECT

#ifdef INTEL_CHANGES
	// which aysnchronous vertex buffer are we targeting? 0 for the original, n for async buffer n-1
	int nParticleSystemAsyncIndex;
	// when we finally render these particles, which section of the aysnc buffer do we read?
	int nVerticesMultiplier;
#endif


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

PRESULT e_ParticlesDrawInit();
void e_ParticleDrawUpdateStats();
PRESULT e_ParticleSystemsRestore();

void e_ParticleDrawInitFrame(
	PARTICLE_DRAW_SECTION eSection,
	PARTICLE_DRAW_INFO & tDrawInfo );
void e_ParticleDrawEndFrame();

void e_ParticleDrawInitFrameLightmap(
	PARTICLE_DRAW_SECTION eSection,
	PARTICLE_DRAW_INFO & tDrawInfo );
void e_ParticleDrawEndFrameLightmap();

BOOL e_ParticleDefinitionsCanDrawTogether(
	const struct PARTICLE_SYSTEM_DEFINITION * pFirst,
	const struct PARTICLE_SYSTEM_DEFINITION * pSecond );

PRESULT e_ParticleSystemsReleaseGlobalResources();
PRESULT e_ParticleSystemsReleaseDefinitionResources();

int e_ParticlesGetDrawnTotal( PARTICLE_DRAW_SECTION eSection = PARTICLE_DRAW_SECTION_INVALID );
float e_ParticlesGetTotalWorldArea( PARTICLE_DRAW_SECTION eSection = PARTICLE_DRAW_SECTION_INVALID );
float e_ParticlesGetTotalScreenArea( PARTICLE_DRAW_SECTION eSection = PARTICLE_DRAW_SECTION_INVALID, BOOL bPredicted = FALSE );

void e_ParticleSetFogAdditiveLum(
	BOOL bEnabled );

void e_ParticleSystemLoadModel(
	struct PARTICLE_SYSTEM_DEFINITION * pDefinition );

PRESULT e_ParticlesDraw(
	BYTE bDrawLayer,
	const MATRIX * pmatView,
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2,
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly );

PRESULT e_ParticlesDrawBasic(	
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bClearBuffer,
	BOOL bDepthOnly );

PRESULT e_ParticlesDrawMeshes( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bDepthOnly );

void e_ParticleDrawAddMesh(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE * pParticle,
	PARTICLE_DRAW_INFO & tDrawInfo,
	VECTOR & vOffset,
	VECTOR & vPerpOffset,
	VECTOR & vPositionShift,
	BOOL bDepthOnly );

void e_ParticleDrawAddQuad(
	const PARTICLE_SYSTEM * const NOALIAS pParticleSystem,
	const PARTICLE * const NOALIAS pParticle,
	PARTICLE_DRAW_INFO & tDrawInfo,
	VECTOR & vOffset,
	VECTOR & vPerpOffset,
	VECTOR & vPositionShift,
	BOOL bDepthOnly );

void e_ParticleDrawAddRope(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bDepthOnly );

PRESULT e_ParticlesSystemDrawWithHavokFX( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	PARTICLE_SYSTEM * pParticleSystem );

PRESULT e_ParticlesDrawGPUSim( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	PARTICLE_SYSTEM * pParticleSystem, float SmokeRenderThickness, float SmokeAmbientLight, DWORD diffuseColor, float windInfluence, float densityModifier, float velocityModifier, VECTOR vOffset, float glowMinDensity);

PRESULT e_ParticleSystemGPUSimInit( 
	PARTICLE_SYSTEM * pParticleSystem );

PRESULT e_ParticleSystemGPUSimDestroy( 
	PARTICLE_SYSTEM * pParticleSystem );

BOOL e_ParticlesIsGPUSimEnabled();

void e_ParticleDrawEnterSection(
	PARTICLE_DRAW_SECTION eSection );
void e_ParticleDrawLeaveSection();

void e_ParticleDrawStatsGetString(
	WCHAR * pwszStr,
	int nBufLen );

DWORD e_GetColorFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatTriple> * pPath );

#endif // __E_PARTICLE__
