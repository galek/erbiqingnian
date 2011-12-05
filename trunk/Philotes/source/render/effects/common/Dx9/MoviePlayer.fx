//
// YUV->RGB shader, ps_2_0 version
//

#include "_common.fx"
#include "StateBlocks.fxh"


//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////


struct PS_MOVIE_INPUT
{
    float2 T0		: TEXCOORD0;
};

DECLARE_TEXTURE_STAGE_SAMPLER(0, MIN_MAG_LINEAR_MIP_POINT, LINEAR, LINEAR, CLAMP, CLAMP )
DECLARE_TEXTURE_STAGE_SAMPLER(1, MIN_MAG_LINEAR_MIP_POINT, LINEAR, LINEAR, CLAMP, CLAMP )
DECLARE_TEXTURE_STAGE_SAMPLER(2, MIN_MAG_LINEAR_MIP_POINT, LINEAR, LINEAR, CLAMP, CLAMP )
DECLARE_TEXTURE_STAGE_SAMPLER(3, MIN_MAG_LINEAR_MIP_POINT, LINEAR, LINEAR, CLAMP, CLAMP )



float4x4 gmYUVtoRGB =
{
   1.164123535f,  1.595794678f,  0.0f,         -0.87065506f,
   1.164123535f, -0.813476563f, -0.391448975f,  0.529705048f,
   1.164123535f,  0.0f,          2.017822266f, -1.081668854f,
   1.0f,		  0.0f,			 0.0f,			0.0f
};


//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

float2 ScreenToNDC( float2 screenPos )
{
#ifdef ENGINE_TARGET_DX10  //Remove half texel offset
	screenPos.xy	+= 0.5f;
#endif
	screenPos.xy	*= gvScreenSize.zw;
	screenPos.xy	= ( screenPos.xy - 0.5f ) * float2( 2.f, -2.f );
	return screenPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_MOVIE (
	in VS_POSITION_TEX			In,
	out PS_MOVIE_INPUT			Out,
	out float4					Position	: POSITION )
{
	Position	= float4( In.Position, 1.0f );
	Position.xy	= ScreenToNDC( Position.xy );

	Out.T0.xy	= In.TexCoord;
}

float4 PS_MOVIE_NO_PIXEL_ALPHA( PS_MOVIE_INPUT In ) : COLOR
{
	float4 c;
	float4 p;
	c.x = tex2D( Sampler0, In.T0 ).x;
	c.y = tex2D( Sampler1, In.T0 ).x;
	c.z = tex2D( Sampler2, In.T0 ).x;
	c.w = 1.f;
	p.x = dot( gmYUVtoRGB[0], c );
	p.y = dot( gmYUVtoRGB[1], c );
	p.z = dot( gmYUVtoRGB[2], c );
	p.w = gfAlphaConstant;
	return p;
}

float4 PS_MOVIE_WITH_PIXEL_ALPHA( PS_MOVIE_INPUT In ) : COLOR
{
	float4 c;
	float4 p;
	c.x = tex2D( Sampler0, In.T0 ).x;
	c.y = tex2D( Sampler1, In.T0 ).x;
	c.z = tex2D( Sampler2, In.T0 ).x;
	c.w = 1.f;
	p.w = tex2D( Sampler3, In.T0 ).x;
	p.x = dot( gmYUVtoRGB[0], c );
	p.y = dot( gmYUVtoRGB[1], c );
	p.z = dot( gmYUVtoRGB[2], c );
	p.w *= gfAlphaConstant;
	return p;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////


DECLARE_TECH T_Movie_NoPixelAlpha < SHADER_VERSION_22 int Index = 0; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_MOVIE() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_MOVIE_NO_PIXEL_ALPHA() );

		DXC_BLEND_RGB_NO_BLEND;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable	= RED | GREEN | BLUE;
		AlphaBlendEnable	= FALSE;
		AlphaTestEnable		= FALSE;
#endif
	}
}

DECLARE_TECH T_Movie_WithPixelAlpha < SHADER_VERSION_22 int Index = 1; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_MOVIE() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_MOVIE_WITH_PIXEL_ALPHA() );

		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable	= RED | GREEN | BLUE;
		BlendOp				= ADD;
		SrcBlend			= SRCALPHA;
		DestBlend			= INVSRCALPHA;
		AlphaBlendEnable	= TRUE;
		AlphaTestEnable		= FALSE;
#endif
	}
}

