//
// Solid color screen effect
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_SOLIDCOLOR_INPUT
{
    float4 Position			: POSITION;
    float4 Color			: COLOR0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_SOLIDCOLOR_INPUT VS_SolidColor( VS_POSITION_TEX In )
{
	PS_SOLIDCOLOR_INPUT Out = (PS_SOLIDCOLOR_INPUT) 0;
	Out.Position			= float4( In.Position, 1.0f );
	Out.Color				= saturate( float4( gvColor0.xyz, gfFloats.w * gfTransition.w ) );
    return Out;
}


float4 PS_SolidColor( PS_SOLIDCOLOR_INPUT In ) : COLOR
{
	float4 color = In.Color;
	AppControlledAlphaTest( color );
	return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH SolidColor <
	SHADER_VERSION_11
	SET_LAYER( LAYER_ON_TOP )
	SET_LABEL( Float3, "Blend factor" )
	SET_LABEL( Color0, "Blend color" )
	>
{
	//PASS_COPY( SFX_RT_BACKBUFFER, SFX_RT_FULL )

	PASS_SHADERS( SFX_RT_NONE, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_SolidColor() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_SolidColor() );

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
