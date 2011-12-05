//----------------------------------------------------------------------------
// dx10_ParticleSimulateGPU.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _DX10_PARTICLESIMULATEGPU_H_
#define _DX10_PARTICLESIMULATEGPU_H_

#ifndef GPUPS_DECL_FX
	#include "e_pch.h"
	#include "e_particle.h"


	struct PARTICLE_SYSTEM;

	//OVERALL INITIALIZATION AND SHUTDOWN
	PRESULT dx10_ParticleSystemsGPUInit();
	PRESULT dx10_ParticleSystemsFree();





	//CREATES AND DESTROYS AN INSTANCE OF GPU PARTICLE SYSTEM INFO & BUFFERS
	PRESULT dx10_ParticleSystemGPUDestroy(int nId);
	PRESULT dx10_ParticleSystemGPUInitialize( PARTICLE_SYSTEM* pParticleSystem );




	//CC@HAVOK
	PRESULT dx10_ParticleSystemPrepareEmitter(D3D_EFFECT* pEffectX, int nParticleInstanceId);
	PRESULT dx10_ParticleSystemGPUAdvance(
		int nParticleInstanceId );
	PRESULT dx10_ParticleSystemGPURender(int nParticleInstanceId, PARTICLE_DRAW_INFO & drawInfo, float color[], float windInfluence, VECTOR vOffsetPosition, float velocityMultiplier, float velocityClamp, float sizeMultiplier, float SmokeRenderThickness);
#endif // GPUPS_DECL_FX

struct GPUPS_MINMAX
{
	float fMin;
	float fMax;
	float fUnusedOne;
	float fUnusedTwo;
};

#define MAX_GPUPS_INTERP_VALUES	4
struct GPUPS_FLOAT4
{
	float fP0;
	float fP1;
	float fP2;
	float fP3;
};

struct GPUPS_PARAMETERS
{
	//////// TIME DATA ///////////////
	float fDeltaTime;
	float fRunningTime;
	float fUnusedOne;
	float fUnusedTwo;

	/////////// PARTICLE SYSTEM DATA (CURRENT FRAME ONLY / SYSTEM TIME) ////////////

	GPUPS_MINMAX duration;
	GPUPS_MINMAX launchScale;
	GPUPS_MINMAX launchSpeed;
	GPUPS_MINMAX ratePPMS;
	GPUPS_MINMAX launchOffsetZ;
	GPUPS_MINMAX launchSphereRadius;
	GPUPS_FLOAT4 scaleMin;
	GPUPS_FLOAT4 scaleMax;
	GPUPS_FLOAT4 scaleTime;
	GPUPS_FLOAT4 alpha;
	GPUPS_FLOAT4 alphaTime;
	GPUPS_FLOAT4 color0;
	GPUPS_FLOAT4 color1;
	GPUPS_FLOAT4 color2;
	GPUPS_FLOAT4 color3;

	//////////// PARTICLE DATA (PARTICLE TIME) //////////////

	// temporarily associated with system time, not particle time...
	// this should be later replaced with constant buffers or textures that
	// can be looked up using the particles local time value.
	GPUPS_MINMAX zAcceleration;
};

#endif //_DX10_PARTICLESIMULATEGPU_H_
