//
// 
//

#include "_common.fx"
#include "dx9/_EnvMap.fx"
#include "dx9/_SphericalHarmonics.fx"
#include "dx9/_Shadow.fxh"
#include "../tugboat/dx9/MythosBackgroundCommon.fxh"

#define VERTEX_FORMAT_BACKGROUND_64BYTES 0
#define VERTEX_FORMAT_BACKGROUND_32BYTES 1




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_COMMON_BACKGROUND_INPUT VS30_BACKGROUND_64BYTES ( 
	out float4 OutPosition : POSITION,
	VS_BACKGROUND_INPUT_64 In, 
	uniform int nShadowType )
{
	PS_COMMON_BACKGROUND_INPUT Out = (PS_COMMON_BACKGROUND_INPUT)0;

	half3 vNormal = RIGID_NORMAL_V( In.Normal );

	VS_COMMON_BACKGROUND(
		float4( In.Position, 1.f ),
		vNormal,
		In.LightMapDiffuse,
		In.DiffuseMap2,
		Out,
		OutPosition,
		true,				// diffuse2
		false,				// SH
		true				// manual fog
		);

	if ( gbCubeEnvironmentMap )
	{
		Out.EnvMapCoords.xyz	= CubeMapGetCoords( half3( Out.Pw ), half3( EyeInWorld.xyz ), half3( Out.Nw ) );
	}

	return Out;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_COMMON_BACKGROUND_INPUT VS30_BACKGROUND_32BYTES ( 
	out float4 OutPosition : POSITION,
	VS_BACKGROUND_INPUT_32 In, 
	uniform int nShadowType )
{
	PS_COMMON_BACKGROUND_INPUT Out = (PS_COMMON_BACKGROUND_INPUT)0;

	half3 vNormal = RIGID_NORMAL_V( In.Normal );

	half2 DiffuseMap2 = 0;  // unused

	VS_COMMON_BACKGROUND(
		float4( In.Position, 1.f ),
		vNormal,
		In.LightMapDiffuse,
		DiffuseMap2,
		Out,
		OutPosition,
		false,				// diffuse2
		false,				// SH
		true				// manual fog
		);

	if ( gbCubeEnvironmentMap )
	{
		Out.EnvMapCoords.xyz	= CubeMapGetCoords( half3( Out.Pw ), half3( EyeInWorld.xyz ), half3( Out.Nw ) );
	}

	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS30_BACKGROUND( 
	PS_COMMON_BACKGROUND_INPUT In,
	uniform int nShadowType ) : COLOR
{
	half4 Color;
	half2 fSpecularPowerGloss;
	half3 Nw;

	PS_COMMON_BACKGROUND(
		In,
		Color,
		fSpecularPowerGloss,
		Nw,
		false,				// specular
		false,				// specular via LUT
		false,				// diffuse2
		true,				// manual fog
		nShadowType );


	half3 vEMCoords = normalize( half3( In.EnvMapCoords ) );
	half fPower		= fSpecularPowerGloss.x;
	half fGloss		= fSpecularPowerGloss.y;
	half4 vEnvMap	= EnvMapLookupCoords( Nw, vEMCoords, fPower, fGloss, false, false, false );
	if ( gbCubeEnvironmentMap )
	{
		Color.xyz		= lerp( Color.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );
	}

	return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#ifdef PRIME

PixelShader gPixelShader[SHADOW_TYPE_COUNT] = 
{
	COMPILE_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SHADOW_TYPE_NONE ) ),
	COMPILE_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SHADOW_TYPE_DEPTH ) ),
	COMPILE_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SHADOW_TYPE_COLOR ) ),
};


#define TECHNIQUE30_64BYTES(shadowtype) \
    DECLARE_TECH TVertexAndPixelShader30_64_##shadowtype < \
	SHADER_VERSION_33 \
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_64BYTES; \
	int ShadowType		= shadowtype; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_64BYTES( shadowtype ) ); \
		SET_PIXEL_SHADER( gPixelShader[shadowtype] ); \
		} } 

TECHNIQUE30_64BYTES(SHADOW_TYPE_NONE)
TECHNIQUE30_64BYTES(SHADOW_TYPE_DEPTH)
TECHNIQUE30_64BYTES(SHADOW_TYPE_COLOR)

#define TECHNIQUE30_32BYTES(shadowtype) \
    DECLARE_TECH TVertexAndPixelShader30_32_##points < \
	SHADER_VERSION_33 \
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_32BYTES; \
	int ShadowType		= shadowtype; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_32BYTES( shadowtype ) ); \
		SET_PIXEL_SHADER( gPixelShader[shadowtype] ); \
		} } 

TECHNIQUE30_32BYTES(SHADOW_TYPE_NONE)
TECHNIQUE30_32BYTES(SHADOW_TYPE_DEPTH)
TECHNIQUE30_32BYTES(SHADOW_TYPE_COLOR)

#else

DECLARE_TECH TVertexAndPixelShader30 <
	SHADER_VERSION_33;
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_64BYTES;
	int ShadowType		= SHADOW_TYPE_COLOR; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_64BYTES( SHADOW_TYPE_COLOR ) );
		COMPILE_SET_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SHADOW_TYPE_COLOR ) );
	}
} 

#endif
