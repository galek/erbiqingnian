//
// Simple Overlay shader
//

#include "_common.fx"
#include "StateBlocks.fxh"

half4 gvLayerColor;
half2 gvUVScale;

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_OVERLAY_INPUT
{
    float4 Position   : POSITION;
    float2 TexCoord0  : TEXCOORD0;
};

struct VS_XYZCOL_INPUT
{
    float4 Position	: POSITION;
    float4 Color	: COLOR0;
};

struct PS_OVERLAY_WORLDSPACE_INPUT
{
    float4 Position   : POSITION;
    float4 Color	  : COLOR0;
    float3 TexCoord0  : TEXCOORD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// this stuff can and should be done fixed function

PS_OVERLAY_INPUT VS_OVERLAY ( VS_POSITION_TEX In )
{
	PS_OVERLAY_INPUT Out = (PS_OVERLAY_INPUT) 0;
	Out.Position  = float4( In.Position, 1.0f );
	Out.TexCoord0 = In.TexCoord;
    return Out;
}

float4 PS_OVERLAY ( PS_OVERLAY_INPUT In ) : COLOR
{
    return tex2D( DiffuseMapSampler, In.TexCoord0 );
}

float4 PS_OVERLAY_FIXED ( PS_OVERLAY_INPUT In ) : COLOR
{
    return gvLayerColor;
}

//--------------------------------------------------------------------------------------------
// WORLD-SPACE overlay -- take in 3D position, transform, and tex lookup using screen pos as UV
//--------------------------------------------------------------------------------------------

PS_OVERLAY_WORLDSPACE_INPUT VS_OVERLAY_WORLDSPACE ( VS_XYZCOL_INPUT In )
{
	PS_OVERLAY_WORLDSPACE_INPUT Out = (PS_OVERLAY_WORLDSPACE_INPUT) 0;

	Out.Position		= mul( In.Position, WorldViewProjection );
	Out.Color			= In.Color;
	Out.TexCoord0		= float3( Out.Position.x, -Out.Position.y, Out.Position.w );

    return Out;
}

float4 PS_OVERLAY_WORLDSPACE_ALPHA ( PS_OVERLAY_WORLDSPACE_INPUT In ) : COLOR
{
	half2 vCoords = In.TexCoord0.xy;

	vCoords.xy	/= In.TexCoord0.z;
	vCoords.xy	= vCoords.xy * 0.5f + 0.5f;
	vCoords.xy	*= gvUVScale;
	vCoords.xy	+= gvPixelAccurateOffset;

	float4 vColor;
	vColor.rgb = 0;
	vColor.a   = tex2D( DiffuseMapSampler, vCoords ).a;
	return vColor;
}

float4 PS_OVERLAY_WORLDSPACE_COLOR ( PS_OVERLAY_WORLDSPACE_INPUT In ) : COLOR
{
	half2 vCoords = In.TexCoord0.xy;

	vCoords.xy	/= In.TexCoord0.z;
	vCoords.xy	= vCoords.xy * 0.5f + 0.5f;
	vCoords.xy	*= gvUVScale;
	vCoords.xy	+= gvPixelAccurateOffset;

	float4 vColor;
	vColor.rgb = tex2D( DiffuseMapSampler, vCoords ).rgb * In.Color.rgb;
	vColor.a   = In.Color.a;
    return vColor;
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 1.1 Techniques
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_TECH TOverlay < SHADER_VERSION_11 int Index = 0; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_OVERLAY() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_OVERLAY() );

#ifndef ENGINE_TARGET_DX10
		ZWriteEnable = FALSE;
		ZEnable		 = FALSE;
#endif
	}
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DECLARE_TECH TOverlayBlend < SHADER_VERSION_11 int Index = 1; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_OVERLAY() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_OVERLAY_FIXED() );

#ifdef ENGINE_TARGET_DX10
		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
		DXC_DEPTHSTENCIL_OFF;
#else
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		CullMode		 = NONE;
		AlphaTestEnable	 = FALSE;
		BlendOp			 = ADD;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
		FogEnable		 = FALSE;

		ZWriteEnable = FALSE;
		ZEnable		 = FALSE;
#endif
	}
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// CHB 2006.06.14 - Although the PS is compiled 2.0, it's reported
// as 1.1 in the metadata to retain current run-time behavior, as this
// was originally SHADER_GROUP_11.
// CHB 2006.10.13 - Changing this to reflect reality.
#if !defined(FXC_LEGACY_COMPILE)
DECLARE_TECH TOverlayWorldspace < SHADER_VERSION_12 int Index = 2; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_OVERLAY_WORLDSPACE() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_OVERLAY_WORLDSPACE_ALPHA() );

#ifndef ENGINE_TARGET_DX10		
		ColorWriteEnable	= ALPHA;
		//BlendOp				= MAX;
		//SrcBlend			= SRCALPHA;
		//DestBlend			= DESTALPHA;
		AlphaBlendEnable	= FALSE; //TRUE;
#endif
	}
	pass P1
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_OVERLAY_WORLDSPACE() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_OVERLAY_WORLDSPACE_COLOR() );
#ifndef ENGINE_TARGET_DX10
		COLORWRITEENABLE	= BLUE | GREEN | RED;
		BlendOp				= ADD;
		SrcBlend			= SRCALPHA;
		DestBlend			= INVSRCALPHA;
		AlphaBlendEnable	= TRUE;
#endif
	}
}
#endif
