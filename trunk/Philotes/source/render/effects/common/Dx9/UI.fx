//
// Simple Overlay shader
//

#include "_common.fx"


//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////


struct PS_UI_TEXTURE_INPUT
{
	float4 Position   : POSITION;
    float2 TexCoord0  : TEXCOORD0;
    float4 Diffuse	  : COLOR0;
};

struct PS_UI_BASIC_INPUT
{
	float4 Position   : POSITION;
    float4 Diffuse	  : COLOR0;
};


sampler UIDiffuseSampler : register(SAMPLER(SAMPLER_DIFFUSE)) = sampler_state
{
	Texture = (tDiffuseMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
#endif
	AddressU = WRAP;
	AddressV = WRAP;
};


//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

float2 UIScreenToNDC( float2 screenPos )
{
#ifdef ENGINE_TARGET_DX10  //Remove half texel offset
	screenPos.xy	+= 0.5f;
#endif
	screenPos.xy	*= gvScreenSize.zw;
	screenPos.xy	= ( screenPos.xy - 0.5f ) * float2( 2.f, -2.f );
	return screenPos;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void VS_UI_BASIC (
	in VS_XYZ_COL In,
	out PS_UI_BASIC_INPUT Out )
{
	Out.Position		= float4( In.Position, 1.0f );
	Out.Position.xy		= UIScreenToNDC( Out.Position.xy );
	Out.Diffuse			= DX10_SWIZ_COLOR_INPUT( In.Color );
}

float4 PS_UI_BASIC (
	in PS_UI_BASIC_INPUT In,
	uniform bool bDisabled ) : COLOR
{
	float4 color = In.Diffuse;

	if ( bDisabled )
		color.rgb = dot( color.rgb, float3( 0.3f, 0.59f, 0.1f ) );

	AppControlledAlphaTest( color );
    return color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_UI_TEXTURE (
	in VS_XYZ_COL_UV In,
	out PS_UI_TEXTURE_INPUT Out )
{
	Out.Position	= float4( In.Position, 1.0f );
	Out.Position.xy	= UIScreenToNDC( Out.Position.xy );

	Out.TexCoord0	= In.TexCoord;
	//Out.TexCoord0	+= gvPixelAccurateOffset;
	Out.Diffuse		= DX10_SWIZ_COLOR_INPUT( In.Color );
}

float4 PS_UI_TEXTURE (
	PS_UI_TEXTURE_INPUT In,
	uniform bool bDisabled ) : COLOR
{
	float4 color = tex2D( UIDiffuseSampler, In.TexCoord0 ) * In.Diffuse;

	if ( bDisabled )
		color.rgb = dot( color.rgb, float3( 0.3f, 0.59f, 0.1f ) );

	AppControlledAlphaTest( color );
    return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////


DECLARE_TECH T_UI_Texture < SHADER_VERSION_11 int Index = 0; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_UI_TEXTURE() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_UI_TEXTURE( false ) );
	}
}

DECLARE_TECH T_UI_Basic < SHADER_VERSION_11 int Index = 1; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_UI_BASIC() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_UI_BASIC( false ) );
	}
}

DECLARE_TECH T_UI_Texture_Disabled < SHADER_VERSION_11 int Index = 2; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_UI_TEXTURE() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_UI_TEXTURE( true ) );
	}
}

DECLARE_TECH T_UI_Basic_Disabled < SHADER_VERSION_11 int Index = 3; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_UI_BASIC() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_UI_BASIC( true ) );
	}
}
