//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

// This is shared between C and FX code.

// Samplers used for 1.1 shaders MUST be < 4.


// NOTE DATA DEPENDENCY: Data depends on these orderings!
#define SAMPLER_DIFFUSE				0
#define SAMPLER_LIGHTMAP			1
#define SAMPLER_SELFILLUM			2
#define SAMPLER_PARTICLE_LIGHTMAP	3	// CHB 2007.02.15
// End 1.1
#define SAMPLER_DIFFUSE2			4
#define SAMPLER_SPECULAR			5
#define SAMPLER_NORMAL				6

// Max scrolling index
#define NUM_SCROLLING_SAMPLER_TYPES	7
// END DATA DEPENDENCY

#define SAMPLER_CUBEENVMAP			7
#define SAMPLER_SPECLOOKUP			8
#define SAMPLER_SPHEREENVMAP		9

#define NUM_SAMPLER_DX10_ENGINE_TOUCHES 10
//Every sampler after this can not be adjusted by the engine

#define SAMPLER_SHADOW				10
#define SAMPLER_SHADOWDEPTH 		11

// hellgate
#define SAMPLER_DIFFUSE_RAIN		12
#define SAMPLER_BUMP_RAIN			13

#define SAMPLER_DX10_FULLSCREEN		14

// Count.
// CHB 2007.08.30 - If you add a sampler, be sure to add
// an initializer for it in dx9_DefaultStatesInit() of
// dxC_state_defaults.cpp.
#define NUM_SAMPLER_TYPES			15
