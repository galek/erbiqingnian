#ifndef _PARTICLES_INPUT_FXH_
#define _PARTICLES_INPUT_FXH_


struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float  Rand			: PSIZE;
    float4 Color		: COLOR0;
    float2 Tex			: TEXCOORD0;
#ifdef DX10_SOFT_PARTICLES
    float  Z    		: TEXCOORD1;
#endif
    float  Fog			: FOG;
};


struct PS_GLOWCONSTANT_ADD_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Color		: COLOR0;
    float4 Glow			: COLOR1;
    float2 Tex			: TEXCOORD0;
#ifdef DX10_SOFT_PARTICLES
    float  Z    		: TEXCOORD1;
#endif
    float  Fog			: FOG;
};


struct PS_GLOWCONSTANT_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Glow			: COLOR0;
    float2 Tex			: TEXCOORD0;
#ifdef DX10_SOFT_PARTICLES
    float  Z    		: TEXCOORD1;
#endif
};


#endif // _PARTICLES_INPUT_FXH_