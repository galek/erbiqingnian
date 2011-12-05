// c_particles.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef INCLUDE_PARTICLE_PRIV
#define INCLUDE_PARTICLE_PRIV

#include "interpolationpath.h"
#include "definition_common.h"
#include "boundingbox.h"
#include "refcount.h"

enum
{
	PARTICLE_SYSTEM_CULL_PRIORITY_UNDEFINED			= -1,
	PARTICLE_SYSTEM_CULL_PRIORITY_ALWAYS_DRAW		= 1,
	PARTICLE_SYSTEM_CULL_PRIORITY_SOMETIMES_DRAW	= 5,
	PARTICLE_SYSTEM_CULL_PRIORITY_RARELY			= 9,

	MAX_PARTICLE_SYSTEM_CULL_PRIORITY = PARTICLE_SYSTEM_CULL_PRIORITY_RARELY,
};

//-------------------------------------------------------------------------------------------------
// dwFlags
#define PARTICLE_SYSTEM_DEFINITION_FLAG_LOOPS							MAKE_MASK(0)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ATTACH_PARTICLES				MAKE_MASK(1)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLE_GRAVITY				MAKE_MASK(2)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_SYSTEM		MAKE_MASK(3)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_AFFECTS_BLUR					MAKE_MASK(4)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ANIMATES				MAKE_MASK(5)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND			MAKE_MASK(6)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_RANDOM_ANIM_FRAME				MAKE_MASK(7)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_KILL_PARTICLES_WITH_SYSTEM		MAKE_MASK(8)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_VIEW_CIRCLE						MAKE_MASK(9)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_VIEW_SHOOT						MAKE_MASK(10)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_U_WITH_VELOCITY			MAKE_MASK(11)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_V_WITH_VELOCITY			MAKE_MASK(12)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_NORMAL_WITH_VELOCITY		MAKE_MASK(13)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_VIEW_FIXED_FIRE					MAKE_MASK(14)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ALIGN_SYSTEM_NORMAL_WITH_CAMERA	MAKE_MASK(15)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_DONT_DIE				MAKE_MASK(16)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_RANDOMLY_MIRROR_TEXTURE			MAKE_MASK(17)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_HAS_ROPE						MAKE_MASK(18)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE						MAKE_MASK(19)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_STRETCH_TEXTURE_DOWN_ROPE		MAKE_MASK(20)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_SPRING_PHYSICS		MAKE_MASK(21)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_ROPE_PHYSICS			MAKE_MASK(22)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_WAVE_PHYSICS			MAKE_MASK(23)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_WAVES_RANDOM_CYCLE			MAKE_MASK(24)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FOLLOW_ROPE			MAKE_MASK(25)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW							MAKE_MASK(26)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_USE_FOLLOW_SYSTEM_ROPE			MAKE_MASK(27)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_DECAL							MAKE_MASK(28)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_ROPE_USES_HOSE_PHYSICS			MAKE_MASK(29)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_USE_ROPE_SCALE		MAKE_MASK(30)
#define PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_FACE_UP				MAKE_MASK(31)

// dwFlags2
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_STRAIGHT_TO_END				MAKE_MASK(0)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_SYSTEM_FACE_UP						MAKE_MASK(1)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_RANGE_AFFECTS_SPEED				MAKE_MASK(2)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_RANGE_AFFECTS_DURATION				MAKE_MASK(3)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_SPAWN_ON_FOLLOW_SYSTEM	MAKE_MASK(4)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT						MAKE_MASK(6)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_DESTROY_SYSTEM				MAKE_MASK(7)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_CHANGE_FOV					MAKE_MASK(8)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_HOLYRADIUS				MAKE_MASK(9)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_USE_LIGHTMAP						MAKE_MASK(10)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLE_NORMAL_ON_SPHERE			MAKE_MASK(11)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_MOVE_WITH_SYSTEM			MAKE_MASK(12)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ALLOW_MORE_PARTICLES				MAKE_MASK(13)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_PARTICLES_LOOP_ON_ROPE				MAKE_MASK(14)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_NOVA_PHYSICS				MAKE_MASK(15)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_4					MAKE_MASK(16)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_6					MAKE_MASK(17)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_8					MAKE_MASK(18)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_ONE_STRIP				MAKE_MASK(19)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_DRAW_FRONT_FACE_ONLY				MAKE_MASK(20)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_FACE_CAMERA					MAKE_MASK(21)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_WRITE_Z							MAKE_MASK(22)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_CIRCLE_PHYSICS			MAKE_MASK(23)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_USES_PARTICLE_SPAWN_THROTTLE		MAKE_MASK(24)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_VIEW_NONE							MAKE_MASK(25)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DOESNT_TURN					MAKE_MASK(26)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_WRAP_TEXTURE_AROUND_ROPE			MAKE_MASK(27)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_TRAIL_PHYSICS			MAKE_MASK(28)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_END_IS_INDEPENDENT			MAKE_MASK(29)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_U_WITH_SYSTEM_NORMAL			MAKE_MASK(30)
#define PARTICLE_SYSTEM_DEFINITION_FLAG2_ALIGN_V_WITH_SYSTEM_NORMAL			MAKE_MASK(31)

// dwFlags3
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_ROPE_DRAW_NORMAL_FACING			MAKE_MASK(0)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_VIEW_SWORD_TRAIL					MAKE_MASK(1)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_CAN_USE_HAVOKFX					MAKE_MASK(2)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_IGNORE_MODEL_VISIBILITY			MAKE_MASK(3)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_NOT_WITH_HAVOKFX					MAKE_MASK(4)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_HAVOKFX_CRAWL						MAKE_MASK(5)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_FLIP_MESH_ALONG_Y					MAKE_MASK(6)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_ATTRACTOR_AT_ROPE_END				MAKE_MASK(7)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES				MAKE_MASK(8)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_REPLAY_SOUND_ON_LOOP				MAKE_MASK(9)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_IGNORE_MODEL_NODRAW				MAKE_MASK(10)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_START_AT_RANDOM_PERCENT			MAKE_MASK(11)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_CENSOR								MAKE_MASK(12)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_USES_GPU_SIM                       MAKE_MASK(13)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_TURN_TO_FACE_CAMERA                MAKE_MASK(14)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10								MAKE_MASK(15)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_REPLACEMENT					MAKE_MASK(16)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_ROPE_USES_CONTOUR_PHYSICS			MAKE_MASK(17)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DISABLE_READ_Z						MAKE_MASK(18)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_PARTICLE_IS_LIGHT					MAKE_MASK(19)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_LIGHT_PARTICLE_RENDER_FIRST		MAKE_MASK(20)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_LOCK_DURATION						MAKE_MASK(21)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_FORCE_PLAY_SOUND_FIRST_TIME		MAKE_MASK(22)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION					MAKE_MASK(23)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION_REPLACEMENT		MAKE_MASK(24)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_CENSOR_REPLACEMENT					MAKE_MASK(25)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID							MAKE_MASK(26)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID_USE_BFECC 	    		MAKE_MASK(27)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_DONT_USE_SOFT_PARTICLES  		MAKE_MASK(28)
#define PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND				MAKE_MASK(29)

#define PARTICLE_SYSTEM_DEFINITION_MASK_PARTICLES_LOOP					(PARTICLE_SYSTEM_DEFINITION_FLAG_KILL_PARTICLES_WITH_SYSTEM | PARTICLE_SYSTEM_DEFINITION_FLAG_PARTICLES_DONT_DIE)

// dwUpdateFlags
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_GLOW					MAKE_MASK( 0 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_SCALE				MAKE_MASK( 1 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_X				MAKE_MASK( 2 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_CENTER_Y				MAKE_MASK( 3 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_BOX			MAKE_MASK( 4 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_STRETCH_DIAMOND		MAKE_MASK( 5 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_COLOR				MAKE_MASK( 6 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_ALPHA				MAKE_MASK( 7 )
#define PARTICLE_SYSTEM_DEFINITION_UPDATE_FLAG_DISTORTION			MAKE_MASK( 8 )

// dwUpdateFlags
#define PARTICLE_SYSTEM_DEFINITION_RUNTIME_FLAG_LOADED				MAKE_MASK( 0 )

struct PARTICLE_SYSTEM_DEFINITION
{
	DEFINITION_HEADER			tHeader;
	DWORD						dwFlags;
	DWORD						dwFlags2;
	DWORD						dwFlags3;
	DWORD						dwUpdateFlags;
	DWORD						dwRuntimeFlags;
	int							nLighting;
	int							nLaunchParticleCount;
	float						fMinParticlesPercentDropRate;
	char						pszTextureName	[ MAX_XML_STRING_LENGTH ];
	char						pszLightName	[ MAX_XML_STRING_LENGTH ];
	char						pszModelDefName	[ MAX_XML_STRING_LENGTH ];
	char						pszRopePath		[ MAX_XML_STRING_LENGTH ];
    char						pszTextureDensityName		[ MAX_XML_STRING_LENGTH ];
    char						pszTextureVelocityName		[ MAX_XML_STRING_LENGTH ];
    char						pszTextureObstructorName	[ MAX_XML_STRING_LENGTH ];
	int							nDensityTextureId;
	int							nVelocityTextureId;
	int							nObstructorTextureId;
	int							nRopePathId;
	int							nVolume;
	float						fSoundPlayChance;
	int							nTextureId;
	int							nModelDefId;
	int							nSoundGroup;
	int							nFootstep;
	int							nLightDefId;
	VECTOR						vLightOffset;
	VECTOR                      vGlowCompensationColor;
	REFCOUNT					tRefCount;
	CInterpolationPath<CFloatPair>	tParticleDurationPath;
	CInterpolationPath<CFloatPair>	tParticleScale;
	CInterpolationPath<CFloatPair>	tParticleRotation;
	CInterpolationPath<CFloatPair>	tParticlesPerSecondPath;
	CInterpolationPath<CFloatPair>	tParticlesPerMeterPerSecond;
	CInterpolationPath<CFloatPair>	tParticlesPerMeter;
	CInterpolationPath<CFloatPair>	tParticleBurst;
	CInterpolationPath<CFloatPair>	tParticleAcceleration;
	CInterpolationPath<CFloatPair>	tParticleBounce;
	CInterpolationPath<CFloatPair>	tParticleSpeedBounds;
	CInterpolationPath<CFloatPair>	tParticleTurnSpeed; // havok fx only right now
	CInterpolationPath<CFloatPair>	tParticleWorldAccelerationZ;
	CInterpolationPath<CFloatPair>	tParticleCenterX;
	CInterpolationPath<CFloatPair>	tParticleCenterY;
	CInterpolationPath<CFloatPair>	tParticleCenterRotation;
	CInterpolationPath<CFloatPair>	tParticleWindInfluence;
	CInterpolationPath<CFloatPair>	tLaunchScale;
	CInterpolationPath<CFloatPair>	tLaunchRotation;
	CInterpolationPath<CFloatPair>	tLaunchDirRotation;
	CInterpolationPath<CFloatPair>	tLaunchDirPitch;
	CInterpolationPath<CFloatPair>	tLaunchSpeed;
	CInterpolationPath<CFloatPair>	tLaunchVelocityFromSystem;
	CInterpolationPath<CFloatPair>	tLaunchSpeedFromSystemForward;
	CInterpolationPath<CFloatPair>	tLaunchOffsetX;
	CInterpolationPath<CFloatPair>	tLaunchOffsetY;
	CInterpolationPath<CFloatPair>	tLaunchOffsetZ;
	CInterpolationPath<CFloatPair>	tLaunchSphereRadius;
	CInterpolationPath<CFloatPair>	tLaunchCylinderRadius;
	CInterpolationPath<CFloatPair>	tLaunchCylinderHeight;
	CInterpolationPath<CFloatPair>	tParticleAlpha;
	CInterpolationPath<CFloatPair>	tParticleGlow;
	CInterpolationPath<CFloatTriple>tParticleColor;
	CInterpolationPath<CFloatPair>	tParticleStretchBox;
	CInterpolationPath<CFloatPair>	tParticleStretchDiamond;
	CInterpolationPath<CFloatPair>	tAnimationRate;
	CInterpolationPath<CFloatPair>	tAnimationSlidingRateX;
	CInterpolationPath<CFloatPair>	tAnimationSlidingRateY;
	CInterpolationPath<CFloatPair>	tLaunchRopeScale;
	CInterpolationPath<CFloatTriple>tLaunchRopeColor;
	CInterpolationPath<CFloatPair>	tLaunchRopeGlow;
	CInterpolationPath<CFloatPair>	tLaunchRopeAlpha;
	CInterpolationPath<CFloatPair>	tRopeAlpha;
	CInterpolationPath<CFloatPair>	tRopeGlow;
	CInterpolationPath<CFloatPair>	tRopeWorldAccelerationZ;
	CInterpolationPath<CFloatPair>	tLaunchRopeSpringiness;
	CInterpolationPath<CFloatPair>	tLaunchRopeStiffness;
	CInterpolationPath<CFloatPair>	tRopeWaveAmplitudeUp;
	CInterpolationPath<CFloatPair>	tRopeWaveAmplitudeSide;
	CInterpolationPath<CFloatPair>	tRopeWaveFrequency;
	CInterpolationPath<CFloatPair>	tRopeWaveSpeed;
	CInterpolationPath<CFloatPair>	tRopeDampening;
	CInterpolationPath<CFloatPair>	tRopeMetersPerTexture;
	CInterpolationPath<CFloatPair>	tRopePathScale;
	CInterpolationPath<CFloatPair>	tRopeZOffsetOverTime;
	CInterpolationPath<CFloatPair>	tAttractorOffsetNormal;
	CInterpolationPath<CFloatPair>	tAttractorOffsetSideX;
	CInterpolationPath<CFloatPair>	tAttractorOffsetSideY;
	CInterpolationPath<CFloatPair>	tAttractorWorldOffsetZ;
	CInterpolationPath<CFloatPair>	tParticleAttractorAcceleration;
	CInterpolationPath<CFloatPair>	tAttractorDestructionRadius;
	CInterpolationPath<CFloatPair>	tAttractorForceRadius;
	CInterpolationPath<CFloatPair>	tAttractorForceOverRadius;
	CInterpolationPath<CFloatPair>	tAlphaRef;
	CInterpolationPath<CFloatPair>	tAlphaMin;
	CInterpolationPath<CFloatPair>	tFluidSmokeThickness;
	CInterpolationPath<CFloatPair>	tFluidSmokeAmbientLight;
	CInterpolationPath<CFloatPair>	tGlowMinDensity;
    CInterpolationPath<CFloatPair>	tFluidSmokeDensityModifier;
	CInterpolationPath<CFloatPair>	tFluidSmokeVelocityModifier;
	CInterpolationPath<CFloatPair>	tParticleDistortionStrength;
	int							nShaderType;
	int							nGPUShader;
	int							nDrawOrder;
	float						fDuration;
	float						fStartDelay;
	float						fCullDistance;
	int							nCullPriority;
	char						pszNextParticleSystem[ MAX_XML_STRING_LENGTH ];
	int							nNextParticleSystem;
	char						pszFollowParticleSystem[ MAX_XML_STRING_LENGTH ];
	int							nFollowParticleSystem;
	char						pszRopeEndParticleSystem[ MAX_XML_STRING_LENGTH ];
	int							nRopeEndParticleSystem;
	char						pszDyingParticleSystem[ MAX_XML_STRING_LENGTH ];
	int							nDyingParticleSystem;
	float						pfTextureFrameSize [ 2 ];
	int							pnTextureFrameCount[ 2 ];
	float						fCircleRadius;
	float						fRopeEndSpeed;
	float						fTrailKnotDuration;
	float						fViewSpeed; // just used by previewer
	float						fViewHosePressure;// just used by previewer
	float						fViewRangePercent;// just used by previewer
	float						fViewCircleRadius;// just used by previewer
	float						fViewNovaAngle;// just used by previewer
	int							nViewParticleSpawnThrottle;// just used by previewer

	int							nKnotCount; // this is the min count
	int							nKnotCountMax;
	int							nMeshesPerBatchMax;
	FLOAT						fSegmentSize;

	int							nRuntimeParticleUpdatedCount;
	int							nRuntimeRopeUpdatedCount;

	// NVTL: for fluid simulation
	int                         nGridWidth;
	int                         nGridHeight;
	int                         nGridDepth;
	int                         nGridBorder;
	float                       fRenderScale;
	float                       fVelocityMultiplier;
	float                       fVelocityClamp;
	float                       fSizeMultiplier;
	int                         nGridDensityTextureIndex;
	int                         nGridVelocityTextureIndex;
	int                         nGridObstructorTextureIndex;
	float                       fVorticityConfinementScale;

	// NVTL: for soft particles
	float						fSoftParticleContrast;
	float						fSoftParticleScale;

	BOUNDING_BOX				tMultiMeshCompressionBox;
};


//-------------------------------------------------------------------------------------------------
#define PARTICLE_FLAG_MIRROR_TEXTURE		MAKE_MASK( 0 )
#define PARTICLE_FLAG_DYING					MAKE_MASK( 1 )
#define PARTICLE_FLAG_JUST_CREATED			MAKE_MASK( 2 )
struct PARTICLE
{
	DWORD		dwFlags;
	DWORD		dwDiffuse;
	DWORD		dwGlowColor;
	VECTOR		vPosition;
	VECTOR		vVelocity;
	float		fScaleInit;
	float		fScale;
	float		fCenterX;
	float		fCenterY;
	float		fElapsedTime;
	float		fDuration;
	float		fRotation;
	float		fCenterRotation;
	float		fRandPercent;
	float		fFrame; // for texture animations
	float		fRopeDistance;  // for particles that follow along a rope
	union
	{
		int		nFollowKnot;
	};
	float		fStretchBox;  
	float		fStretchDiamond;  
};

//-------------------------------------------------------------------------------------------------
#define KNOT_FLAG_REMOVED					MAKE_MASK(0)

struct KNOT
{
	DWORD		dwFlags;
	DWORD		dwDiffuse;
	DWORD		dwGlow;
	VECTOR		vPosition;
	VECTOR		vVelocity;		
	VECTOR		vNormal;		
	VECTOR		vPositionIdeal; // used by wave physics
	float		fAlphaInit;
	float		fGlowInit;
	float		fScale;
	float		fRemainingTime; // used by trail physics
	float		fSegmentLength;
	float		fDistanceFromStart; // used by follow particles
	float		fDampening;
	float		fRandPercent;
	float		fFrame; // for texture animations
};

//-------------------------------------------------------------------------------------------------
#define OLD_PARTICLE_DRAW_LIMIT 300
#define MAX_WAVE_OFFSETS 100 
#define PARTICLE_SYSTEM_FLAG_DEAD					MAKE_MASK(0)
#define PARTICLE_SYSTEM_FLAG_VISIBLE				MAKE_MASK(1)
#define PARTICLE_SYSTEM_FLAG_DYING					MAKE_MASK(2)
#define PARTICLE_SYSTEM_FLAG_ROPE_END_ATTACHED		MAKE_MASK(3)
#define PARTICLE_SYSTEM_FLAG_ROPE_END_MOVED			MAKE_MASK(4)
#define PARTICLE_SYSTEM_FLAG_NODRAW					MAKE_MASK(5)
#define PARTICLE_SYSTEM_FLAG_DELAYING				MAKE_MASK(6)
#define PARTICLE_SYSTEM_FLAG_FADE_IN				MAKE_MASK(7)
#define PARTICLE_SYSTEM_FLAG_FADE_OUT				MAKE_MASK(8)
#define PARTICLE_SYSTEM_FLAG_CULLED_BY_DISTANCE		MAKE_MASK(9)
#define PARTICLE_SYSTEM_FLAG_CULLED_FOR_SPEED		MAKE_MASK(10)
#define PARTICLE_SYSTEM_MASK_DRAW_LAYER				(MAKE_MASK(11) | MAKE_MASK(12) | MAKE_MASK(13))
#define PARTICLE_SYSTEM_SHIFT_DRAW_LAYER			11 
#define PARTICLE_SYSTEM_BITS_DRAW_LAYER				3 
#define PARTICLE_SYSTEM_FLAG_LOCK_DRAW_LAYER		MAKE_MASK(14)
#define PARTICLE_SYSTEM_FLAG_NOLIGHTS				MAKE_MASK(15)
#define PARTICLE_SYSTEM_FLAG_NOSOUND				MAKE_MASK(16)
#define PARTICLE_SYSTEM_FLAG_WAS_NODRAW				MAKE_MASK(17) // was nodraw last frame
#define PARTICLE_SYSTEM_FLAG_FIRST_PERSON			MAKE_MASK(18)
#define PARTICLE_SYSTEM_FLAG_FORCE_MOVE_PARTICLES	MAKE_MASK(19)
#define PARTICLE_SYSTEM_FLAG_MOVE_WHEN_DEAD			MAKE_MASK(20)
#define PARTICLE_SYSTEM_FLAG_USING_HAVOK_FX			MAKE_MASK(21)
#define PARTICLE_SYSTEM_FLAG_FORCE_NON_HAVOK_FX		MAKE_MASK(22)
#define PARTICLE_SYSTEM_FLAG_HAS_UPDATED_ONCE		MAKE_MASK(23)
#define PARTICLE_SYSTEM_FLAG_WORLD_MATRIX_SET		MAKE_MASK(24)
#define PARTICLE_SYSTEM_FLAG_FORCE_NON_ASYNC		MAKE_MASK(25)
#define PARTICLE_SYSTEM_FLAG_ASYNC					MAKE_MASK(26)
#define PARTICLE_SYSTEM_FLAG_ASYNC_SYSTEM_IS_NEW	MAKE_MASK(27)
#define PARTICLE_SYSTEM_FLAG_ASYNC_OPTIMIZE			MAKE_MASK(28)
#define PARTICLE_SYSTEM_FLAG_ASYNC_ROLLBACK_TIMES	MAKE_MASK(29)

struct PARTICLE_SYSTEM_DEFINITION;

// 4324: structure was padded due to __declspec(align())
#pragma warning(disable : 4324)  

struct PARTICLE_SYSTEM
{
	int			 nId;
	PARTICLE_SYSTEM* pNext;
	DWORD		 dwFlags;
	int			 nDefinitionId;
	PARTICLE_SYSTEM_DEFINITION * pDefinition;
	float		 fMinParticlesAllowedToDrop;
	float		 fElapsedTime;
	float		 fDuration;
	float		 fTimeLastParticleCreated;
	float		 fLastParticleBurstTime;// this is stored in Path Time
	float		 fPathTime;
	float		 fGlobalAlpha; // used for fading in culled systems
	float		 fPredictedScreenFill;

	VECTOR		 vPositionLastParticleSpawned;
	VECTOR		 vPositionPrev;
	float		 fPositionPrevTime;
	VECTOR		 vPosition;
	VECTOR		 vNormalIn;
	VECTOR		 vNormalToUse;
	MATRIX		 mWorld; // this is mostly used by our DX10 effects.

	VECTOR		 vVelocity;
	int			 nSoundId;
	int			 nFootstepId;
	int			 nLightId;
	ROOMID		 idRoom;

	DWORD		 dwSystemColor;

	VECTOR4		 vHavokFXSystemColor;
	float		 fHavokFXSystemGlow;
	float		 fHavokFXSystemScale;

	int			 nHavokFXParticleCount;

	int			 nAyncVertexCount;

	CArrayIndexed<PARTICLE> tParticles;

	CArrayIndexed<KNOT> tKnots;
	VECTOR		 vRopeEnd;
	VECTOR		 vRopeEndNormal;
	VECTOR		 vRopeEndIdeal;
	float		 fTextureSlideX;
	float		 fTextureSlideY;
	float		 fRopeMetersPerTexture;

	VECTOR		 vTrailPosLast;

	CArrayIndexed<VECTOR> tWaveOffsets;	
	float		 fWaveStart;

	float		 fHosePressure;
	float		 fCircleRadius;
	float		 fNovaAngle;
	float		 fNovaStart;
	float		 fParticleSpawnThrottle;

	int			 nPortalID;
	int			 nRegionID;

	float		 fRopePathScale;

	float		 fRangePercent;
	float		 fRopeBendMaxXY;
	float		 fRopeBendMaxZ;

	float		 fAlphaRef;
	float		 fAlphaMin;

	VECTOR		 vAttractor;

	int			 nParticleSystemNext;
	int			 nParticleSystemPrev;
	int			 nParticleSystemFollow;
	int			 nParticleSystemRopeEnd;
#ifdef INTEL_CHANGES
	// data for async particle systems
	// this is put together here during update, then copied to async system before job dispatch in draw
	float		 fAsyncUpdateTime;
	VECTOR		 vAsyncPosDelta; 
#endif
};

#pragma warning(default : 4324)  


BOOL ParticleSystemPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad );

void e_ParticlesSetPredictedScreenArea( float fArea );

//---------------------------------------------------------------------------------------
typedef int (* PFN_PARTICLES_GET_NEARBY_UNITS)(
	int nMaxUnits, 
	const VECTOR & vCenter, 
	float fRadius, 
	VECTOR * pvPositions, 
	VECTOR * pvHeightsRadiiSpeeds, 
	VECTOR * pvDirections );

extern PFN_PARTICLES_GET_NEARBY_UNITS gpfn_ParticlesGetNearbyUnits;

typedef PARTICLE_SYSTEM * (*PFN_PARTICLESYSTEM_GET)( int nId );
extern PFN_PARTICLESYSTEM_GET gpfn_ParticleSystemGet;

void e_ParticleSystemsInitCallbacks(
	PFN_PARTICLESYSTEM_GET pfn_ParticleSystemGet,
	PFN_PARTICLES_GET_NEARBY_UNITS pfn_ParticlesGetNearbyUnits);

inline PARTICLE_SYSTEM * e_ParticleSystemGet(
	int nId )
{
	return gpfn_ParticleSystemGet( nId );
}

struct EFFECT_DEFINITION * e_ParticleSystemGetEffectDef( 
	const PARTICLE_SYSTEM_DEFINITION * pDefinition );

inline void e_ParticlesSetNeedScreenTexture( BOOL bNeeds )
{
#if ! ISVERSION(SERVER_VERSION)
	extern BOOL gbParticlesNeedScreenTexture;
	gbParticlesNeedScreenTexture = bNeeds;
#else
	REF( bNeeds );
#endif
}

inline BOOL e_ParticlesGetNeedScreenTexture()
{
#if ! ISVERSION(SERVER_VERSION)
	extern BOOL gbParticlesNeedScreenTexture;
	return gbParticlesNeedScreenTexture;
#else
	return FALSE;
#endif
}

BOOL e_ParticleSystemUsesProj2(
	PARTICLE_SYSTEM * pParticleSystem );

PRESULT e_ParticleSystemLoadGPUSimTextures(
	PARTICLE_SYSTEM_DEFINITION * pDefinition );

#endif