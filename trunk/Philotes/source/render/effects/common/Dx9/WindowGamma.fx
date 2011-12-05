//
// Windowed-mode gamma adjustment shader
//

#include "_common.fx"
#include "StateBlocks.fxh"

half gfGammaPow;

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

sampler NearestSampler = sampler_state
{
	Texture = (tDiffuseMap);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;
#else
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};


float4 PS_GammaMath( PS_FULLSCREEN_QUAD In ) : COLOR
{
	float4 C = tex2D( NearestSampler, In.TexCoord.xy );
	C.rgb = pow( C.rgb, gfGammaPow );
	return C;
}

float4 PS_PassThru( PS_FULLSCREEN_QUAD In ) : COLOR
{
	return tex2D( NearestSampler, In.TexCoord.xy );
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 1.1 Techniques
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

DECLARE_TECH TGammaMath < SHADER_VERSION_12 int Index = 0; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_GammaMath() );

		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGBA_NO_BLEND;
		DXC_DEPTHSTENCIL_OFF;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable = RED | GREEN | BLUE | ALPHA;
		CullMode		 = NONE;
		AlphaBlendEnable = FALSE;
		AlphaTestEnable	 = FALSE;
		ZEnable			 = FALSE;
		FogEnable		 = FALSE;
		BlendOp			 = ADD;
		SrcBlend         = ONE;
		DestBlend        = ZERO;
#endif
	}
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

DECLARE_TECH TPassThru < SHADER_VERSION_12 int Index = 1; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_PassThru() );

		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGBA_NO_BLEND;
		DXC_DEPTHSTENCIL_OFF;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable = RED | GREEN | BLUE | ALPHA;
		CullMode		 = NONE;
		AlphaBlendEnable = FALSE;
		AlphaTestEnable	 = FALSE;
		ZEnable			 = FALSE;
		FogEnable		 = FALSE;
		BlendOp			 = ADD;
		SrcBlend         = ONE;
		DestBlend        = ZERO;
#endif
	}
}
