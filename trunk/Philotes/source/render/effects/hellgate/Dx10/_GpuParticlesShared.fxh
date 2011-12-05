#define GPUPS_DECL_FX
#ifdef FXC10_PATH
#include "../../../source/dx10/dx10_ParticleSimulateGPU.h"
#else
#include "../../source/dx10/dx10_ParticleSimulateGPU.h"
#endif

cbuffer cbGPUParticles
{
	GPUPS_PARAMETERS gpupsParams;
};

//////////////// UTILITY DATA/FUNCTIONS ///////////////
SamplerState gpups_sRandom
{
	AddressU = Wrap;
	Filter = MIN_MAG_MIP_POINT;
};

Texture1D gpups_tRandom;

float internalSeeder = 0.0f;

float gpups_GetRandomValue(float min, float max, float seedish)
{
	internalSeeder += 0.01337;
	float normr = gpups_tRandom.SampleLevel(gpups_sRandom, seedish + gpupsParams.fRunningTime + internalSeeder, 0);
	return lerp(min,max,normr);
}

float gpups_GetRandomValueNorm(float seedish)
{
	internalSeeder += 0.01337;
	return gpups_tRandom.SampleLevel(gpups_sRandom, seedish + gpupsParams.fRunningTime + internalSeeder, 0);
}

float4 gpups_GetRandomValueNorm4(float seedish)
{
	internalSeeder += 0.01337;
	return gpups_tRandom.SampleLevel(gpups_sRandom, seedish + gpupsParams.fRunningTime + internalSeeder, 0);
}
