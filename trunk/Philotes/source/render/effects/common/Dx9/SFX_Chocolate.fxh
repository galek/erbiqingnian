//
// Greyscale screen effect
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_CHOCOLATE_INPUT
{
    float4 Position			: POSITION;
    float2 TexCoord			: TEXCOORD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_GREYSCALE_INPUT VS_Chocolate( VS_POSITION_TEX In )
{
	PS_GREYSCALE_INPUT Out = (PS_GREYSCALE_INPUT) 0;
	Out.Position		= float4( In.Position, 1.0f );
	Out.TexCoord		= In.TexCoord;
	Out.TexCoord		+= gvPixelAccurateOffset;

    return Out;
}


float4 PS_Chocolate( PS_GREYSCALE_INPUT In ) : COLOR
{
	half4 vColor = tex2D( RTSampler, In.TexCoord );
	half3 vTinted = dot( vColor.rgb, gfFloats.rgb ) * gvColor0;
	float4 color = float4( vTinted.xyz, 1.f * gfTransition.a );
	AppControlledAlphaTest( color );
	return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH Chocolate <
	SHADER_VERSION_11
	SET_LAYER( LAYER_ON_TOP )
	SET_LABEL( Color0, "Tint Color" )
	SET_LABEL( Float0, "R weight" )
	SET_LABEL( Float1, "G weight" )
	SET_LABEL( Float2, "B weight" )
	>
{
	PASS_COPY( SFX_RT_BACKBUFFER, SFX_RT_FULL )

	PASS_SHADERS( SFX_RT_FULL, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_Chocolate() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_Chocolate() );

		//DXC_BLEND_RGB_NO_BLEND;
		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

		//DX10_GLOBAL_STATE

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
