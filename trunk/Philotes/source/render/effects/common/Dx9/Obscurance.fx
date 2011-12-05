//
// Obscurance -- renders geometry similarly to a z or shadow pass for ambient occlusion calculations
//
#include "_common.fx"
#include "StateBlocks.fxh"

// transformations
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_OBSCURANCE_INPUT
{
    float4 Position  : POSITION;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_OBSCURANCE_INPUT VS_RIGID_OBSCURANCE ( VS_BACKGROUND_ZBUFFER_INPUT In ) 
{
    PS_OBSCURANCE_INPUT Out = (PS_OBSCURANCE_INPUT)0;

	float4 Position = float4( In.Position, 1 );
    Out.Position  = mul( Position, WorldViewProjection );          // position (projected)
    
    return Out;
}

float4 PS_DEBUG() : COLOR
{
	return float4( 1, 0, 0, 1 );
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH RigidShader < SHADER_VERSION_1X  int Index = 0; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_RIGID_OBSCURANCE() );
        //COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_DEBUG() );
        PixelShader = NULL;

        DXC_BLEND_NO_COLOR_NO_BLEND;
        DXC_DEPTHSTENCIL_ZREAD_ZWRITE;
    }  
}
