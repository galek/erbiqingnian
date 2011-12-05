//
//
#include "_common.fx"

#pragma warning( disable : 3550 )		// array ref cannot be l-value

static const float downsampleScale = 4.0f;

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////
#define NUM_SAMPLES 7
struct PS_BLURR_INPUT
{
    float4 Position   : POSITION;
    float2 TexCoord[NUM_SAMPLES]: TEXCOORD0;
};

struct PS_BLURR_FILTER_INPUT
{
    float4 Position  : POSITION;
    float2 TexCoord  : TEXCOORD0;
};

sampler FramebufferMapSampler : register(SAMPLER(SAMPLER_DIFFUSE)) = sampler_state
{
	Texture = (tDiffuseMap);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// BLURR FILTER
//////////////////////////////////////////////////////////////////////////////////////////////
PS_BLURR_FILTER_INPUT VS_BLURR_FILTER ( VS_POSITION_TEX In )
{
	PS_BLURR_FILTER_INPUT Out = (PS_BLURR_FILTER_INPUT) 0;
	Out.Position = float4( In.Position, 1.0f );
	Out.TexCoord = In.TexCoord;
	Out.TexCoord += gvPixelAccurateOffset;

	return Out;
}

float4 PS_BLURR_FILTER( PS_BLURR_FILTER_INPUT In ) : COLOR
{
	return float4( 0.0f, 1.0f, 0.0f, 1.0f );
	float4 Color = tex2D( FramebufferMapSampler, In.TexCoord );
//	return Color * dot( Color.xyz, Color.xyz ) * dot( Color.xyz, Color.xyz ) * Color.w;
	//return 2.0f * Color * Color.w;

	return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// BLURR X and BLURR Y
//////////////////////////////////////////////////////////////////////////////////////////////
float2 ConstantOffsets[ NUM_SAMPLES ] = {
	{ -3, 0.05f },
	{ -2, 0.1f },
	{ -1, 0.2f },
	{  0, 0.3f },
	{  1, 0.2f },
	{  2, 0.1f },
	{  3, 0.05f },
};

PS_BLURR_INPUT VS_BLURR ( VS_POSITION_TEX In, uniform float2 Direction )
{
	PS_BLURR_INPUT Out = (PS_BLURR_INPUT) 0;
	Out.Position = float4( In.Position, 1.0f );
	for ( int i = 0; i < NUM_SAMPLES; i++ )
	{
		Out.TexCoord[ i ] = In.TexCoord + ConstantOffsets[ i ].xx * Direction * ( 2.0f / ( gvScreenSize.xy / downsampleScale ) );
	}
	return Out;
}

//--------------------------------------------------------------------------------------------

float4 PS_BLURR( PS_BLURR_INPUT In ) : COLOR
{
	return float4( 1.0f, 0.0f, 0.0f, 1.0f );
	float4 Color = 0;
	for ( int i = 0; i < NUM_SAMPLES; i++ )
	{
		Color += tex2D( FramebufferMapSampler, In.TexCoord[ i ] ) * ConstantOffsets[ i ].y;
	}
    return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 1.1 Techniques - using the normal map flag to differ
//--------------------------------------------------------------------------------------------

// CHB 2006.06.13 Collapsed separate blur techniques into passes of a single technique.

DECLARE_TECH TBlurr < SHADER_VERSION_22 >
{
	// TBlurrX
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_BLURR( float2( 1, 0 ) ) );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_BLURR() );
    }
    
    // TBlurrY
    pass P1
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_BLURR( float2( 0, 1 ) ) );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_BLURR() );
    }
    
    // TBlurrFilter
    pass P2
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_BLURR_FILTER() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_BLURR_FILTER() );
    }
}

//--------------------------------------------------------------------------------------------
technique TVertexShaderOnly < int VSVersion = VS_NONE; int PSVersion = PS_NONE; > 
 
{
    pass P0
    {
        VertexShader = NULL;
        PixelShader  = NULL;
    }  
}
