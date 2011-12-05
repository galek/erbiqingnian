// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

#ifndef __MACROS_FX__
#define __MACROS_FX__

//
// Functions commonly used across fx files
//

#include "../../source/dxC/dxC_effectssharedconstants.h"


#if defined(FXC_LEGACY_COMPILE)		// CHB 2007.01.02
#define _PS_1_1 ps_1_1
#define vs_1_2 vs_1_1
#define vs_1_3 vs_1_1
#define vs_1_4 vs_1_1
#define vs_2_0 vs_1_1
#define vs_2_a vs_1_1
#define vs_2_b vs_1_1
#define vs_2_x vs_1_1
#define vs_3_0 vs_1_1
#define vs_4_0 vs_1_1
#define ps_1_2 ps_1_1
#define ps_1_3 ps_1_1
#define ps_1_4 ps_1_1
#define ps_2_0 ps_1_1
#define ps_2_a ps_1_1
#define ps_2_b ps_1_1
#define ps_2_x ps_1_1
#define ps_3_0 ps_1_1
#define ps_4_0 ps_1_1
#else
#define _PS_1_1 ps_2_0
#endif

// --------------------------------------------------------------------------------------
// CROSS API MACROS
// --------------------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX10
	#define AppControlledAlphaTest( color ) \
		if( g_bAlphaTestEnable && ( round( color.a * 255.0f ) < g_iAlphaTestRef ) ) \
			discard;
    #define AlphaTest(doit, color ) //Moved from Dx10/dx10_Common.fxh \
	    if( doit && ( round( color.a * 255.0f ) < g_iAlphaTestRef ) ) \
			discard;
	#define DECLARE_TECH technique10
	#define COMPILE_VERTEX_SHADER( sm, method) CompileShader( vs_4_0, method )
	#define SET_VERTEX_SHADER( shader ) SetVertexShader( shader )
	#define COMPILE_GEOMETRY_SHADER( sm, method) CompileShader( gs_4_0, method )
	#define SET_GEOMETRY_SHADER( shader ) SetGeometryShader( shader )	
	#define COMPILE_PIXEL_SHADER( sm, method ) CompileShader( ps_4_0, method )
	#define SET_PIXEL_SHADER( shader ) SetPixelShader( shader )
	#define DX10_GLOBAL_STATE					\
		SetGeometryShader( NULL ) ;				\
		SetRasterizerState( GlobalRasterizer ); \
		SetBlendState( GlobalBlend, float4(1.0f, 1.0f, 1.0f, 1.0f ), 0xffffffff ); \
		SetDepthStencilState( GlobalDS, 1 );
	#define DX10_SWIZ_COLOR_INPUT( color ) color.bgra;
	#define DX10_NO_SHADOW_TYPE_COLOR

	#define SHADER_VERSION_1X int VSVersion = VS_4_0; int PSVersion = PS_4_0;
	#define SHADER_VERSION_11 int VSVersion = VS_4_0; int PSVersion = PS_4_0;
	#define SHADER_VERSION_12 int VSVersion = VS_4_0; int PSVersion = PS_4_0;
	#define SHADER_VERSION_22 int VSVersion = VS_4_0; int PSVersion = PS_4_0;
	#define SHADER_VERSION_33 int VSVersion = VS_4_0; int PSVersion = PS_4_0;
#else
	#define AppControlledAlphaTest( color )
    #define AlphaTest(doit, color ) //this empty define must be there for DX9
	#define DECLARE_TECH technique
	#define COMPILE_VERTEX_SHADER( sm, method) compile sm method
	#define SET_VERTEX_SHADER( shader ) VertexShader = < shader >
	#define COMPILE_PIXEL_SHADER( sm, method ) compile sm method
	#define SET_PIXEL_SHADER( shader ) PixelShader = < shader >
	#define DX10_GLOBAL_STATE
	#define DX10_SWIZ_COLOR_INPUT( color ) color
	
	#define SHADER_VERSION_1X int VSVersion = VS_1_1; int PSVersion = PS_NONE;
	#define SHADER_VERSION_11 int VSVersion = VS_1_1; int PSVersion = PS_1_1;
	#define SHADER_VERSION_12 int VSVersion = VS_1_1; int PSVersion = PS_2_0;
	#define SHADER_VERSION_22 int VSVersion = VS_2_0; int PSVersion = PS_2_0;
	#define SHADER_VERSION_33 int VSVersion = VS_3_0; int PSVersion = PS_3_0;
#endif

#define COMPILE_SET_VERTEX_SHADER( sm, method ) SET_VERTEX_SHADER( COMPILE_VERTEX_SHADER( sm, method ) )
#define COMPILE_SET_GEOMETRY_SHADER( sm, method ) SET_GEOMETRY_SHADER( COMPILE_GEOMETRY_SHADER( sm, method ) )
#define COMPILE_SET_PIXEL_SHADER( sm, method ) SET_PIXEL_SHADER( COMPILE_PIXEL_SHADER( sm, method ) )

#define SHADER_VERSION_44 int VSVersion = VS_4_0; int PSVersion = PS_4_0;

// --------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------

// uncomment to enable /DBG_RENDER debug flag for testing shaders
//#define EFFECT_DEBUG

#ifdef ENGINE_TARGET_DX10
#	define SHARED
#else
#	define SHARED		shared
#endif

#define PI							3.1415926535f

// set the minimum opaque glow value to 1/255 to allow a 1-bit transparency
//#define GLOW_MIN					half(0.004f)
// Moved to common.fx to fit into a global cbuffer

// allows using both uniform bool and static branching in the same code
#define BRANCH( flag )				( flag || g##flag )

#define UNBIAS_TEXCOORD(var)		( (var / 32767.0f) * 8.0f )
float3 UNBIAS( float3 vecIn )  //Unbias packed normal vector, in DX10 we need to swizzle
{
	return 
#ifdef ENGINE_TARGET_DX10
	vecIn.zyx 
#else
	vecIn.xyz
#endif
	* half(2.0f) - half(1.0f);
}
#define UNPACK_NORMAL(var)					( var * half(2.0f) - half(1.0f) )
#define PACK_NORMAL(var)					( var * half(0.5f) + half(0.5f) )

#define UNPACK_XY_NORMAL( N )				N.z = sqrt( half(1) - N.x*N.x - N.y*N.y )


#define VERTEX_FORMAT_BACKGROUND_64BYTES 0
#define VERTEX_FORMAT_BACKGROUND_32BYTES 1



// vertex format compression compensators (for temporal aperture coherence shifting)
#define RIGID_NORMAL_V(vec)			( UNBIAS( vec ) )
#define RIGID_TANGENT_V(vec)		( vec )
#define RIGID_BINORMAL_V(vec)		( vec )
#define RIGID_NORMAL				( RIGID_NORMAL_V  ( In.Normal.rgb   ) )

#define RIGID_TANGENT				( RIGID_TANGENT_V ( In.Tangent  ) )
#define RIGID_BINORMAL				( RIGID_BINORMAL_V( In.Binormal ) )

#define ANIMATED_NORMAL_V(vec)		( vec )
#define ANIMATED_TANGENT_V(vec)		( vec )
#define ANIMATED_BINORMAL_V(vec)	( UNBIAS( vec ) )

#define ANIMATED_NORMAL				( ANIMATED_NORMAL_V  ( In.Normal   ) )
#define ANIMATED_TANGENT			( ANIMATED_TANGENT_V ( In.Tangent  ) )
#define ANIMATED_BINORMAL			( ANIMATED_BINORMAL_V( In.Binormal ) )

//Don't need to swizzle for Dx10 anymore
#define SAMPLE_LIGHTMAP( texcoord )		tex2D( LightMapSampler, texcoord )
#define SAMPLE_SELFILLUM( texcoord )	tex2D( SelfIlluminationMapSampler, texcoord )

//
// D3D DEFINES - must match actual D3D constant values
//
// sampler state
#define ADDRESS_WRAP	1
#define ADDRESS_MIRROR	2
#define ADDRESS_CLAMP	3
#define ADDRESS_BORDER	4
// filter state
#define FILTER_NONE				0
#define FILTER_POINT			1
#define FILTER_LINEAR			2
#define FILTER_ANISOTROPIC		3
#define FILTER_PYRAMIDALQUAD	6
#define FILTER_GAUSSIANQUAD		7



#define FOGMACRO_OUTPUT(Output,Position) Output  = saturate( ( FogMaxDistance - (half)length( (half3)EyeInObject - (half3)Position ) ) / ( FogMaxDistance - FogMinDistance ) );
#define FOGMACRO_WORLD_OUTPUT(Output,Position) Output  = saturate( ( FogMaxDistance - (half)length( (half3)EyeInWorld - (half3)Position ) ) / ( FogMaxDistance - FogMinDistance ) );
#define FOGMACRO(Position)				 FOGMACRO_OUTPUT(Out.Fog,Position)
#define APPLY_FOG( color, fogval )		 color = lerp( FogColor, color, saturate( fogval ) )

//////////////////////////////////////////////////////////////////////////////////
////////////// Macros and Defines ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


#define COMBINE_PASSES 		ZFunc = EQUAL; \
							AlphaBlendEnable = TRUE; \
							BlendOp = Add; \
							SrcBlend = ONE; \
							DestBlend = ONE; 

#define COMBINE_LIGHTMAP_DIFFUSE 		ColorOp[0]   = ADD; \
										ColorArg1[0] = TEXTURE; \
										ColorArg2[0] = DIFFUSE; \
										AlphaArg1[0] = DIFFUSE; \
										AlphaOp[0]   = SELECTARG1; \
										ColorOp[1]   = MODULATE; \
										ColorArg1[1] = TEXTURE; \
										ColorArg2[1] = CURRENT; \
										AlphaArg1[1] = CURRENT; \
										AlphaOp[1]   = SELECTARG1; \
										ColorOp[2]   = DISABLE; \
										AlphaOp[2]   = DISABLE; \
										MipFilter[0] = LINEAR; \
										MinFilter[0] = LINEAR; \
										MagFilter[0] = LINEAR; \
										MipFilter[1] = LINEAR; \
										MinFilter[1] = LINEAR; \
										MagFilter[1] = LINEAR; \


#endif // __MACROS_FX__
