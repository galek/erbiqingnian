//
// ActorBackground - for background objects that can be skinned/animated
//

#include "_common.fx"
#include "dx9/_SphericalHarmonics.fx"
#include "dx9/_AnimatedShared20.fxh"
#include "dx9/_Shadow.fxh"
#include "../tugboat/dx9/MythosBackgroundCommon.fxh"


//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------

PS_COMMON_BACKGROUND_INPUT VS30_ACTOR_BACKGROUND ( 
	out float4 OutPosition : POSITION,
	VS_ANIMATED_INPUT In, 
	uniform int nShadowType,
	uniform bool bSphericalHarmonics )
{
	PS_COMMON_BACKGROUND_INPUT Out = (PS_COMMON_BACKGROUND_INPUT)0;

	float4 Po;
	half3  No;

	if ( gbSkinned )
	{
		GetPositionAndNormal( Po, No, In );
	} else {
 		Po = float4( In.Position, 1.0f );
		No = ANIMATED_NORMAL;
	}

	half4 LightMapDiffuse = half4( 0,0, In.DiffuseMap.xy );
	half2 unused = 0;

	VS_COMMON_BACKGROUND(
		Po,
		No,
		LightMapDiffuse,
		unused,
		Out,
		OutPosition,
		false,					// diffuse2
		bSphericalHarmonics,	// SH
		true					// manual fog
		);

	return Out;
}



//--------------------------------------------------------------------------------------------
float4 PS30_ACTOR_BACKGROUND( 
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
		false,					// specular
		false,					// specular via LUT
		false,					// diffuse2
		true,					// manual fog
		nShadowType );

	return Color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUE30(spharm, shadowtype) \
	DECLARE_TECH TVertexAndPixelShader30 < \
		SHADER_VERSION_33 \
		bool SphericalHarmonics = spharm; \
		int ShadowType = shadowtype; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_ACTOR_BACKGROUND( shadowtype, spharm ) ); \
		COMPILE_SET_PIXEL_SHADER(  ps_3_0, PS30_ACTOR_BACKGROUND( shadowtype ) ); \
		}  } 

#ifdef PRIME

TECHNIQUE30(0, SHADOW_TYPE_NONE)
TECHNIQUE30(1, SHADOW_TYPE_NONE)
//TECHNIQUE30(0, SHADOW_TYPE_DEPTH)
//TECHNIQUE30(1, SHADOW_TYPE_DEPTH)
//TECHNIQUE30(0, SHADOW_TYPE_COLOR)
//TECHNIQUE30(1, SHADOW_TYPE_COLOR)

#endif
