// TOC for all BackgroundOutdoorMatrix techniques

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#ifndef DYNAMICDIR_ENABLED
#define DYNAMICDIR_ENABLED	0
#endif

#include "_common.fx"
#include "dx9/_SphericalHarmonics.fx"
#include "dx9/_Shadow.fxh"
#include "../tugboat/dx9/MythosBackgroundCommon.fxh"

#ifdef PRIME

#pragma warning(disable : 1519) // macro redefinition

#define COMBINE(name,type)				name##__##type
#define VS_TARGET(type)					COMBINE(VS_COMPILE_TARGET,type)
#define VSCT_DECLARE_NAME(type)			static const string VS_TARGET(type) = #type;
#define PS_TARGET(type)					COMBINE(PS_COMPILE_TARGET,type)
#define PSCT_DECLARE_NAME(type)			static const string PS_TARGET(type) = #type; 
#define TECHNIQUENAME(name,vsv,psv)		TVertexAndPixelShader20_##vsv##_##psv

//--------------------------------------------------------------------------------------------
// REFERENCE: SHADER CODE DEFINES
/*
	VS_2_V		Vertex Shader version
	PS_2_V		Pixel Shader version
	VSI_TYPE	VS input structure type
*/
//--------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------
// SHADERS
//--------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_COMMON_BACKGROUND_INPUT VS20_BACKGROUND ( 
	out float4 OutPosition : POSITION,
	VS_BACKGROUND_INPUT_64 In, 
	uniform bool bDiffuseMap2,
	uniform int nShadowType )
{
	PS_COMMON_BACKGROUND_INPUT Out = (PS_COMMON_BACKGROUND_INPUT)0;

	half3 vNormal = RIGID_NORMAL_V( In.Normal );
	half2 unused = 0;

	VS_COMMON_BACKGROUND(
		float4( In.Position.xyz, 1 ),
		vNormal,
		In.LightMapDiffuse,
		unused,
		Out,
		OutPosition,
		bDiffuseMap2,		// diffuse2
		false,				// SH
		false				// manual fog
		);

	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_BACKGROUND( 
	PS_COMMON_BACKGROUND_INPUT In,
	uniform bool bSpecular,
	uniform bool bDiffuseMap2,
	uniform int nShadowType ) : COLOR
{
	// CML 2007.02.17 - Temporarily disabling specular until slider-based selection system can be put into place
	// TWB - uh, why not keep it on? Looks so much better.
	//bSpecular = false;

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
		bDiffuseMap2,			// diffuse2
		false,					// manual fog
		nShadowType );

	return Color;
}


//--------------------------------------------------------------------------------------------
// TECHNIQUE VARIATION ORDER
//--------------------------------------------------------------------------------------------

#define VS_1				vs_2_a
#define VS_1_NAME			VS_2_a
#define VS_2				vs_2_0
#define VS_2_NAME			VS_2_0
#define PS_1				ps_2_a
#define PS_1_NAME			PS_2_a
#define PS_2				ps_2_b
#define PS_2_NAME			PS_2_b
//#define PS_3				ps_2_b
//#define PS_3_NAME			PS_2_b



// VS/PS compile target type names for reflection

VSCT_DECLARE_NAME( VS_1	)
VSCT_DECLARE_NAME( VS_2	)
PSCT_DECLARE_NAME( PS_1	)
PSCT_DECLARE_NAME( PS_2	)
PSCT_DECLARE_NAME( PS_3	)

//--------------------------------------------------------------------------------------------
// TECHNIQUES
//--------------------------------------------------------------------------------------------

// CHB 2006.11.03 - Disable normal mapping for SHADOW_TYPE_COLOR
// because it overflows arithmetic instruction slots.
#ifndef ENGINE_TARGET_DX10
#define TECHNIQUE20_SHADOW1(points,specular,normal,diffusemap2,debugflag) \
	TECHNIQUE20(points,specular,normal,diffusemap2,SHADOW_TYPE_DEPTH,debugflag) \
	TECHNIQUE20_NAME(tech_##normal##_,points,specular,0,diffusemap2,SHADOW_TYPE_COLOR,debugflag)
#endif

#define VS_2_V					VS_1
#define VS_2_N					VS_1_NAME
		#define PS_2_V					PS_1
		#define PS_2_N					PS_1_NAME
//			#include "BackgroundOutdoorMatrix_Techniques.fx"
		#define PS_2_V					PS_2
		#define PS_2_N					PS_2_NAME
//			#include "BackgroundOutdoorMatrix_Techniques.fx"
		#define PS_2_V					PS_3
		#define PS_2_N					PS_3_NAME
//			#include "BackgroundOutdoorMatrix_Techniques.fx"

#define VS_2_V					VS_2
#define VS_2_N					VS_2_NAME
		#define PS_2_V					PS_1
		#define PS_2_N					PS_1_NAME
//			#include "BackgroundOutdoorMatrix_Techniques.fx"
		#define PS_2_V					PS_2
		#define PS_2_N					PS_2_NAME
			#include "BackgroundOutdoorMatrix_Techniques.fx"
		#define PS_2_V					PS_3
		#define PS_2_N					PS_3_NAME
//			#include "BackgroundOutdoorMatrix_Techniques.fx"


#include "dx9/_NoPixelShader.fx"

#endif // PRIME