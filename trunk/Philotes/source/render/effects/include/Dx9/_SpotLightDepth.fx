//
// Spot Light Depth - renders geometry to a depth buffer for the spotlight shadows
//
#include "_common.fx"
#include "_BasicShaders.fx"
#include "_AnimatedShared.fx"

float4 Bias = { 0.005f, 0.005f, 0.005f, 0.005f };
// transformations
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_SPOTLIGHTDEPTH_INPUT
{
    float4 Position  : POSITION;
};

struct PS_SPOTLIGHTDEPTH_INPUT
{
    float4 Position  : POSITION;
    float4 Color	 : COLOR0;
};

//--------------------------------------------------------------------------------------------
PS_SPOTLIGHTDEPTH_INPUT VS_SPOTLIGHTDEPTH ( VS_SPOTLIGHTDEPTH_INPUT In ) 
{
    PS_SPOTLIGHTDEPTH_INPUT Out = (PS_SPOTLIGHTDEPTH_INPUT)0;

    Out.Position  = mul( In.Position, WorldViewProjection );          // position (projected)
    Out.Color = ( Out.Position.z / Out.Position.w ) + Bias;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_SPOTLIGHTDEPTH ( PS_SPOTLIGHTDEPTH_INPUT In ) : COLOR
{
	return In.Color;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_SPOTLIGHTDEPTH_INPUT VS_ANIMATED_SPOTLIGHTDEPTH ( VS_ANIMATED_INPUT In ) 
{
    PS_SPOTLIGHTDEPTH_INPUT Out = (PS_SPOTLIGHTDEPTH_INPUT)0;

	float4 Position;
	float3 Normal;
	GetPositionAndNormal( Position, Normal, In );
	Out.Position  = mul( Position, WorldViewProjection );          // position (projected)
	Out.Color = Out.Position.z / Out.Position.w + Bias;

    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH RigidShader < SHADER_VERSION_11 > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SPOTLIGHTDEPTH());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_SPOTLIGHTDEPTH());
        DXC_BLEND_A_NO_BLEND;

#ifndef ENGINE_TARGET_DX10
        COLORWRITEENABLE = ALPHA;
        ALPHATESTENABLE = FALSE;
#endif
    }  
}

//--------------------------------------------------------------------------------------------
DECLARE_TECH AnimatedShader < SHADER_VERSION_11 > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_ANIMATED_SPOTLIGHTDEPTH());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_SPOTLIGHTDEPTH());
        DXC_BLEND_A_NO_BLEND;

#ifndef ENGINE_TARGET_DX10
        COLORWRITEENABLE = ALPHA;
        ALPHATESTENABLE = FALSE;
#endif
    }  
}


