//
// Vignette screen effect
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_VIGNETTE_INPUT
{
    float4 Position			: POSITION;
    float2 PixelPosition	: TEXCOORD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_VIGNETTE_INPUT VS_Vignette( VS_POSITION_TEX In )
{
	PS_VIGNETTE_INPUT Out = (PS_VIGNETTE_INPUT) 0;
	Out.Position		= float4( In.Position, 1.0f );
	Out.PixelPosition	= In.Position.xy;
    return Out;
}


float4 PS_Vignette( PS_VIGNETTE_INPUT In ) : COLOR
{
	// convert window coord to [-1, 1] range
	//half2 vPos = half(2.f) * ( In.PixelPosition - half2( 0.5, 0.5 ) ); 
	half2 vPos = In.PixelPosition;

	// calculate distance from origin
	half r = length( vPos.xy );

	// hermite blend -- not cheap
	r = smoothstep( gfFloats.y, gfFloats.z, r);
	r *= gfTransition.a;

	float4 color = float4( gvColor0.rgb, gfFloats.x * r );
	AppControlledAlphaTest( color );
	return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH Vignette <
	SHADER_VERSION_12
	SET_LAYER( LAYER_ON_TOP )
	SET_LABEL( Color0, "Vignette color" )
	SET_LABEL( Float0, "Max opacity" )
	SET_LABEL( Float1, "Inner radius" )
	SET_LABEL( Float2, "Outer radius" )
	>
{
	PASS_SHADERS( SFX_RT_NONE, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_Vignette() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_Vignette() );

		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable	= RED | GREEN | BLUE;
		
		AlphaBlendEnable	= TRUE;
		BlendOp				= Add;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		AlphaTestEnable		= TRUE;
#endif
	}
}
