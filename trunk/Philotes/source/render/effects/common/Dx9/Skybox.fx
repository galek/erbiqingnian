// Skybox.fx - Performs rendering of skybox
//

#include "_common.fx"
#include "Dx9/_ScrollUV.fx"
#include "StateBlocks.fxh"

float4x4 InvViewProjection;
float3 EyePosition;
float2 gvScrollDiffuse;
float2 gvScrollOpacity;
float  gfOpacityCoef = 0.0f;
float4 gvVisibilityCos = float4( 0.0f, 0.0f, 0.0f, 0.0f ); // ( vMin, 1.0f / ( vMax - vMin ), 0.0f, 1.0f )


//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////


struct PS_SKYBOX_SCROLL_INPUT
{
    float4 Position   : POSITION;
    float2 TexCoord1  : TEXCOORD_(SAMPLER_DIFFUSE);
    float2 Brightness : TEXCOORD1;
//#ifndef SKYBOX_11
    float2 TexCoord2  : TEXCOORD_(SAMPLER_SELFILLUM);	// CHB 2006.10.02 - for 1.1, register needs to match sampler
//#endif
};


PS_SKYBOX_SCROLL_INPUT VS_SKYBOX_SCROLL ( VS_BACKGROUND_INPUT_16 Input )
{
	PS_SKYBOX_SCROLL_INPUT Out = (PS_SKYBOX_SCROLL_INPUT) 0;

	Out.Position = mul( float4( Input.Position, 1.0f ), WorldViewProjection );	

	float2 fDiffuseMap = UNBIAS_TEXCOORD( Input.TexCoord.xy );

	Out.TexCoord1.xy = ScrollTextureCoords( fDiffuseMap.xy, SAMPLER_DIFFUSE );

//#ifndef SKYBOX_11
	Out.TexCoord2.xy = fDiffuseMap.xy;
//#endif

	return Out;
}

float4 PS_SKYBOX_SCROLL ( PS_SKYBOX_SCROLL_INPUT In ) : COLOR
{
	float4 cColor   = tex2D( DiffuseMapSampler, In.TexCoord1.xy );		// scrolling

//#ifndef SKYBOX_11
	// PROBLEM!  Opacity is in the diffuse alpha now, so this is problematical.
	// We can't sample the same stage twice with different coords in 1.1.
	// We could, however, bind the texture to both slots.

	float  fOpacity = tex2D( SelfIlluminationMapSampler, In.TexCoord2.xy );	// not scrolling

	// floor the opacity
	cColor.a = min( fOpacity, cColor.a );
//#endif

	return cColor;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
DECLARE_TECH TSkyboxScroll <
	SHADER_VERSION_11
	int PointLights = 0;
	int SpecularLights = 0;
	int Specular = 0;
	int NormalMap = 0;
	int OpacityMap = -1;
	int DiffuseMap2 = 0;
	int LightMap = 0; 
	int DebugFlag = 0;
	 > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SKYBOX_SCROLL());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_SKYBOX_SCROLL());
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;

#ifndef ENGINE_TARGET_DX10
		ZWriteEnable	 = FALSE;
		ZEnable			 = TRUE;
		ColorWriteEnable = BLUE | GREEN | RED;
		CullMode		 = NONE;
		FogEnable		 = FALSE;
#endif
    }  
}
