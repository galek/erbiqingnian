//
// Add color screen effect
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_ADDCOLOR_INPUT
{
    float4 Position			: POSITION;
    float4 Color			: COLOR0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_ADDCOLOR_INPUT VS_AddColor( VS_POSITION_TEX In )
{
	PS_ADDCOLOR_INPUT Out	= (PS_ADDCOLOR_INPUT) 0;
	Out.Position			= float4( In.Position, 1.0f );
	Out.Color				= float4( gvColor0.xyz, gfFloats.w );
	Out.Color				*= gfFloats.xxxx * gfTransition.xxxx;

    return Out;
}


float4 PS_AddColor( PS_ADDCOLOR_INPUT In ) : COLOR
{
	float4 color = In.Color;
	AppControlledAlphaTest( color );
	return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH AddColor <
	SHADER_VERSION_11
	SET_LAYER( LAYER_PRE_BLOOM )
	SET_LABEL( Float0, "Intensity" )
	SET_LABEL( Float3, "Add glow value" )
	SET_LABEL( Color0, "Add color" )
	>
{
	PASS_SHADERS( SFX_RT_NONE, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_AddColor() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_AddColor() );

		DXC_BLEND_RGBA_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable	= RED | GREEN | BLUE | ALPHA;

		AlphaBlendEnable	= TRUE;
		BlendOp				= Add;
		SrcBlend			= One;
		DestBlend			= One;
		AlphaTestEnable		= FALSE;
#endif
	}
}
