//
// ActorBackground Shader - for background objects that can be skinned/animated
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

PS_COMMON_BACKGROUND_INPUT VS20_ACTOR_BACKGROUND ( 
	out float4 OutPosition : POSITION,
	VS_ANIMATED_INPUT In, 
	uniform bool bSkinned,
	uniform bool bSphericalHarmonics )
{
	PS_COMMON_BACKGROUND_INPUT Out = (PS_COMMON_BACKGROUND_INPUT)0;

	float4 Po;
	half3  No;

	if ( bSkinned )
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
		false					// manual fog
		);

	return Out;
}



//--------------------------------------------------------------------------------------------
float4 PS20_ACTOR_BACKGROUND( 
	PS_COMMON_BACKGROUND_INPUT In,
	uniform bool bSpecular,
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
		bSpecular,				// specular
		true,					// specular via LUT
		false,					// diffuse2
		false,					// manual fog
		nShadowType );

	return Color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUENAMED(name,specular,skinned,shadowtype,bSphericalHarmonics) \
	DECLARE_TECH name < \
		SHADER_VERSION_22 \
		int Specular = specular; \
		int Skinned = skinned; \
		int LightMap = -1; \
		int ShadowType = shadowtype; \
		bool SphericalHarmonics = bSphericalHarmonics; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR_BACKGROUND( skinned, bSphericalHarmonics ) ); \
		COMPILE_SET_PIXEL_SHADER(  ps_2_0, PS20_ACTOR_BACKGROUND( specular, shadowtype ) ); \
	}  } 

#ifndef PRIME
TECHNIQUENAMED(SimpleOneLight,1,0,0,0,0,0,0)
TECHNIQUENAMED(Everything,1,1,1,0,0,1,0)
#endif

#ifdef PRIME

#ifndef TECHNIQUE20_SHADOW0
#define TECHNIQUE20_SHADOW0(specular,skinned,spharm) TECHNIQUENAMED(TVertexAndPixelShader20, specular, skinned, SHADOW_TYPE_NONE, spharm)
#endif

//#ifndef TECHNIQUE20_SHADOW1
//#define TECHNIQUE20_SHADOW1(specular,skinned,spharm) TECHNIQUENAMED(TVertexAndPixelShader20, specular, skinned, SHADOW_TYPE_DEPTH, spharm)
//#endif

//#ifndef TECHNIQUE20_SHADOW2
//#define TECHNIQUE20_SHADOW2(specular,skinned,spharm) TECHNIQUENAMED(TVertexAndPixelShader20, specular, skinned, SHADOW_TYPE_COLOR, spharm)
//#endif


#undef TECHNIQUE20
#define TECHNIQUE20(points,specular,normal,skinned,shadowmap,bSphericalHarmonics,debugflag) TECHNIQUE20_SHADOW0(specular,skinned,bSphericalHarmonics)
#include "dx9/_ActorHeader.fx"

/*
#undef TECHNIQUE20
#define TECHNIQUE20(points,specular,normal,skinned,shadowmap,bSphericalHarmonics,debugflag) TECHNIQUE20_SHADOW1(specular,skinned,bSphericalHarmonics)
#include "dx9/_ActorHeader.fx"

#undef TECHNIQUE20
#define TECHNIQUE20(points,specular,normal,skinned,shadowmap,bSphericalHarmonics,debugflag) TECHNIQUE20_SHADOW2(specular,skinned,bSphericalHarmonics)
#include "dx9/_ActorHeader.fx"
*/
#endif // PRIME