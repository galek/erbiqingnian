//
//

#include "_common.fx"
#include "StateBlocks.fxh"

half4 gvBias;
half4 gvScale;


half AutomapInRange( half z, half fmin, half fmax )
{
	half f = 1;
	if ( z < fmin )
		f = 0;
	if ( z > fmax )
		f = 0;
	return f;
}

void VSAutomap(
	in	VS_SIMPLE_INPUT In,
	out	float4			Color		: COLOR0,
	out float4			Position	: POSITION
	 )
{
	Color.x = AutomapInRange( In.Position.z, gvBias.x, gvScale.x );
	Color.y = AutomapInRange( In.Position.z, gvBias.y, gvScale.y );
	Color.z = AutomapInRange( In.Position.z, gvBias.z, gvScale.z );
	Color.w = FogColor.w;

	Position = mul( float4( In.Position, 1.0f ), WorldViewProjection );
}

float4 PSAutomap(
	in float4			Color	: COLOR0 ) : COLOR
{
	return Color;
}

//----------------------------------------------------------------------------

struct PS_FOG_INPUT
{
	float4	Position	: POSITION;
	float4	Color		: COLOR0;
	float2	TexCoord	: TEXCOORD0;
};


float2 ScreenToNDC( float2 screenPos )
{
#ifdef ENGINE_TARGET_DX10  //Remove half texel offset
	//screenPos.xy	+= 0.5f;
#endif
	//screenPos.xy	*= gvScreenSize.zw;
	screenPos.xy	= ( screenPos.xy - 0.5f ) * float2( 2.f, -2.f );
	return screenPos;
}

void VSFog(
	in	VS_XYZ_COL_UV	In,
	out PS_FOG_INPUT	Out )
{
	Out.Position = float4( ScreenToNDC(In.Position.xy), 0.f, 1.f );
	Out.TexCoord = In.TexCoord;
	Out.Color    = In.Color;
}


float4 PSFog(
	in PS_FOG_INPUT		In ) : COLOR
{
	half4 vCol;
	vCol.rg = In.Color.rg;
	vCol.b = tex2D( DiffuseMapSampler, In.TexCoord ).b;
	vCol.a = 1.f;

	//vCol.rb = 0.f;


	return vCol;
}


//----------------------------------------------------------------------------

DECLARE_TECH TAutomapGenerate
  < SHADER_VERSION_11
	int Index = 0; >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VSAutomap() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PSAutomap() );

		DXC_BLEND_RGBA_ONE_ONE_MAX;
    }
}

DECLARE_TECH TFogOfWarUpdate
  < SHADER_VERSION_11
	int Index = 1; >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VSFog() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PSFog() );
    }
}