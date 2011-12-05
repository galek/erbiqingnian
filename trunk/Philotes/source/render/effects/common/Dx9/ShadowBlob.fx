//
// Blob Shadow using texture
//
// IF YOU USE THIS SHADER, be sure to fill WorldMat, ViewMat and ProjMat!
// Also, you must transpose all incoming matrices!
//

#include "_common.fx"
#include "StateBlocks.fxh"

#ifndef USE_DEPTH
#define USE_DEPTH 1
#endif

struct PS_BLOBSHADOW_INPUT
{
    float4 Position  : POSITION;
	float2 TexCoord : TEXCOORD0;
#if USE_DEPTH
	float Depth : TEXCOORD1;
#endif
};

PS_BLOBSHADOW_INPUT VSBlobShadow( VS_POSITION_TEX In )
{
	PS_BLOBSHADOW_INPUT Out = (PS_BLOBSHADOW_INPUT)0;
	
	Out.Position = mul( mul( float4( In.Position, 1.0f ), World ), View );
	float4 DepthPosition = Out.Position;
	DepthPosition.z -= 0.75f; 	
	float2 Depth = mul( DepthPosition, Projection ).zw;
#if USE_DEPTH
	Out.Depth = Depth.x / Depth.y;
#endif
	Out.Position = mul( Out.Position, Projection );
#if ! USE_DEPTH
	Out.Position.z = Depth.x / Depth.y * Out.Position.w;
#endif
	Out.TexCoord = In.TexCoord;
	
	return Out;
}

struct PS_BLOBSHADOW_OUTPUT
{
	float4 Color : COLOR;
#if USE_DEPTH
	float Depth : DEPTH;
#endif
};

PS_BLOBSHADOW_OUTPUT PSBlobShadow( PS_BLOBSHADOW_INPUT In )
{
	PS_BLOBSHADOW_OUTPUT Out = (PS_BLOBSHADOW_OUTPUT)0;
	Out.Color = tex2D( DiffuseMapSampler, In.TexCoord.xy );
	Out.Color.a *= half( gfShadowIntensity );

#if USE_DEPTH
	Out.Depth = In.Depth;
#endif

	return Out;
}

DECLARE_TECH TVertexAndPixelShader
  < SHADER_VERSION_11 >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VSBlobShadow() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PSBlobShadow() );

#ifdef ENGINE_TARGET_DX10
		DXC_DEPTHSTENCIL_ZREAD;
		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
#else
		AlphaBlendEnable = true;
/*
        BlendOp = RevSubtract;
        SrcBlend = SrcAlpha;
        DestBlend = One;
*/

        BlendOp = Add;
        SrcBlend = SrcAlpha;
        DestBlend = InvSrcAlpha;

        ZEnable = true;
        ZWriteEnable = false;
        CullMode = None;
        AlphaTestEnable = true;
        ColorWriteEnable = Red | Green | Blue;
        FogEnable = false;
#endif        
    }  
}