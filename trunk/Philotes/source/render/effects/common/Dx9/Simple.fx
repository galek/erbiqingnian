//
// Simple Shader - it just transforms and uses one texture
//
// This is a pretty basic one to make sure that things are working.
//
#include "_common.fx"

#ifdef DX10_DEBUG_SHADOWMAPS
#include "Dx9/_Shadow.fxh"
#endif

half4 gcBoxColor = 1.0f;

// transformations
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_OUTPUT
{
    float4 Position  : POSITION;
    float4 Diff : COLOR0;
    float2 Tex  : TEXCOORD0;
    float  Fog  : FOG;
#ifdef DX10_DEBUG_SHADOWMAPS
	float4 shadow : SHADOW;
#endif
#ifdef ENGINE_TARGET_DX10
	float linearw : TEXCOORD1;
#endif
};

VS_OUTPUT VS( VS_BACKGROUND_INPUT_16 Input )
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul( float4( Input.Position, 1.0f ), WorldViewProjection);          // position (projected)
    Out.Diff = 1.0f;									  
    Out.Diff.w = GLOW_MIN;
	Out.Tex  = UNBIAS_TEXCOORD( Input.TexCoord );
#ifdef ENGINE_TARGET_DX10
	Out.linearw = Out.Position.w;
#endif	

	FOGMACRO( Input.Position )
	 
#ifdef DX10_DEBUG_SHADOWMAPS
	ComputeShadowCoords( Out.shadow, Input.Position.xyz );
#endif

    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_NPS_OUTPUT
{
    float4 Position  : POSITION;
    float4 Diff : COLOR0;
    float2 Tex  : TEXCOORD0;
};

VS_NPS_OUTPUT VS_NPS(
    float4 Pos  : POSITION, 
    float2 Tex  : TEXCOORD0)
{
    VS_NPS_OUTPUT Out = (VS_NPS_OUTPUT)0;

    Out.Position  = mul(Pos, WorldViewProjection);          // position (projected)
    Out.Diff = 1.0f;
    Out.Diff.w = GLOW_MIN;
    Out.Tex  = UNBIAS_TEXCOORD( Tex );                                       

    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void PS(
    VS_OUTPUT input,
    out float4 OutColor : COLOR
#ifdef ENGINE_TARGET_DX10
    , out float OutColorDepth : COLOR1 
#endif    
    )
{
    float4 color = tex2D(DiffuseMapSampler, input.Tex) * input.Diff * gcBoxColor;
#ifdef ENGINE_TARGET_DX10
	APPLY_FOG( color.xyz, input.Fog );
#endif

#ifdef DX10_DEBUG_SHADOWMAPS
	color *= ComputeLightingAmount( input.shadow );
#endif

	OutColor = color;
#ifdef ENGINE_TARGET_DX10
	OutColorDepth = input.linearw;
#endif	
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader
  < SHADER_VERSION_11 >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS() );
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
/*
technique TVertexShaderOnly
< int VSVersion = VS_1_1; int PSVersion = PS_NONE; >
{
    pass P0
    {
        // shaders
		VertexShader = compile vs_1_1 VS_NPS();
        PixelShader  = NULL;
        
        // texture stages
        ColorOp[0]   = MODULATE;
        ColorArg1[0] = TEXTURE;
        ColorArg2[0] = DIFFUSE;
		AlphaArg1[0] = DIFFUSE;
		AlphaOp[0]   = SELECTARG1;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
 
		MipFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MagFilter[0] = LINEAR;
    }
}
*/

