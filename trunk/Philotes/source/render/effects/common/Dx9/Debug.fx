//
// Debug rendering technique:
//   Used for rendering into the debug texture for on-screen display of graphical stats, in-progress textures, etc.
//

#include "_common.fx"
#include "StateBlocks.fxh"
//#include "..\\_ShaderVersionConstants.fx"
//#include "..\\VertexInputStructs.fxh"

//float4x4 WorldViewProjection : WorldViewProjection < string UIWidget="None"; >;

TEXTURE tDiffuseDebugTexture;
sampler DiffuseDebugSampler : register(s0) = sampler_state
{
	Texture = (tDiffuseDebugTexture);
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
#endif
};


//////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Shader for basic per-vertex, diffuse-colored  (No pixel shader)
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_OUTPUT
{
    float4 Pos  : POSITION;
    float4 Diff : COLOR0;
    float2 Tex0 : TEXCOORD0;
};

VS_OUTPUT VS_DEBUG( VS_POSITION_TEX In )
{
	VS_OUTPUT Out = (VS_OUTPUT)0;

	Out.Pos  = mul( In.Position, WorldViewProjection );			// position (Projected space)
	//Out.Diff = In.Diff;
	//Out.Tex0 = In.Tex;

	return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////

float4 PS_DEBUG( VS_OUTPUT In ) : COLOR
{
    //return tex2D(DiffuseDebugSampler, In.Tex0) * In.Diff;
    return float4( 0.5f, 0.5f, 1.0f, 1.0f );
}

//////////////////////////////////////////////////////////////////////////////////////////////
/*
float4 OverdrawColor;
float4 PS_DEBUG_OVERDRAW() : COLOR
{
	return OverdrawColor;
    //return float4( 0.1f, 0.1f, 0.1f, 1.0f );
}
*/
//pixelshader DebugOverdrawPS = compile _PS_1_1 PS_DEBUG_OVERDRAW();

//////////////////////////////////////////////////////////////////////////////////////////////
// Technique Vertex Shader Only - uses above Vertex Shader, no Pixel Shader
//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader < SHADER_VERSION_11 int Index = 0; > 
{
    pass P0
    {
        // shaders
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_DEBUG() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_DEBUG() );

		DXC_BLEND_RGB_NO_BLEND;
		DXC_DEPTHSTENCIL_OFF;

#ifndef ENGINE_TARGET_DX10
		AlphaTestEnable		= FALSE;
		AlphaBlendEnable	= FALSE;
		ColorWriteEnable	= RED | GREEN | BLUE;
		ZEnable				= FALSE;
		ZWriteEnable		= FALSE;
#endif
    }
}
