//
// Simple Lit Shader - it just transforms and applies lights
//
// transformations
#include "_common.fx"

sampler DiffuseMap1Sampler : register(SAMPLER(SAMPLER_DIFFUSE)) = sampler_state
{
	Texture = (tDiffuseMap);
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};

sampler DiffuseMap2Sampler : register(SAMPLER(SAMPLER_DIFFUSE2)) = sampler_state
{
	Texture = (tDiffuseMap2);
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else	
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};


//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_COMBINE_LAYERS_INPUT
{
    float4 Position  : POSITION;
    float2 TexCoord0  : TEXCOORD0;
    float2 TexCoord1  : TEXCOORD1;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////
PS_COMBINE_LAYERS_INPUT VS_COMBINE_LAYERS ( VS_POSITION_TEX In )
{
	PS_COMBINE_LAYERS_INPUT Out = (PS_COMBINE_LAYERS_INPUT) 0;
	Out.Position = float4( In.Position, 1.0f );

	//Out.TexCoord0 = In.TexCoord + 1.0f / (gvScreenSize.xy / 4.0f);
	Out.TexCoord0 = In.TexCoord;

	//Out.TexCoord1 = In.TexCoord;

	// this fixes the full-screen blur resulting from the bloom/glow pass
	Out.TexCoord0 += gvPixelAccurateOffset;
	//Out.TexCoord1 += gvPixelAccurateOffset;

    return Out;
}

//--------------------------------------------------------------------------------------------

float4 PS_COMBINE_LAYERS( PS_COMBINE_LAYERS_INPUT In ) : COLOR
{
	half4 Blurr = tex2D( DiffuseMap1Sampler, In.TexCoord0 );
#ifndef DX10_DOF_GATHER
#ifdef DX10_DOF
	Blurr.a = fDOFComputeBlur( fGetDepth( In.TexCoord0 ) ) * 0.5f + 0.5f;
#endif
#endif
    return Blurr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 1.1 Techniques
//--------------------------------------------------------------------------------------------
DECLARE_TECH TVertexAndPixelShader < SHADER_VERSION_11 >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_COMBINE_LAYERS() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_COMBINE_LAYERS() );
//All state is now set in dxC_Render.cpp
/*
		DXC_BLEND_RGB_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		SRGBWriteEnable  = FALSE;
		AlphaBlendEnable = TRUE;
		AlphaTestEnable  = FALSE;
		BlendOp			 = ADD;
		SrcBlend         = ONE;
		DestBlend        = ONE;
		COLORWRITEENABLE = BLUE | GREEN | RED;


		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
#endif
*/
    }  
}

//--------------------------------------------------------------------------------------------
technique TVertexShaderOnly <  int VSVersion = VS_NONE; int PSVersion = PS_NONE; >

{
    pass P0
    {
        VertexShader = NULL;
        PixelShader  = NULL;
    }  
}
