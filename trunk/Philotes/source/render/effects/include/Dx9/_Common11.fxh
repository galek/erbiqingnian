//*****************************************************************************
//
// 1.1 shaders common
//
//*****************************************************************************

#ifndef __COMMON11_FXH__
#define __COMMON11_FXH__

#include "Dx9/_AnimatedShared11.fxh"
#include "dx9/_Shadow.fxh"
#include "dx9/_SphericalHarmonics.fx"

//-----------------------------------------------------------------------------

#ifndef COMMON11_USE_LIGHTMAP
#define COMMON11_USE_LIGHTMAP 0
#endif

#ifndef COMMON11_USE_PARTICLE_LIGHTMAP
#define COMMON11_USE_PARTICLE_LIGHTMAP 0
#endif

#ifndef COMMON11_USE_SELF_ILLUMINATION
#define COMMON11_USE_SELF_ILLUMINATION 1	// I'd prefer this default to 0, with explicit enabling, but setting 1 maintains current behavior
#endif

#ifndef COMMON11_USE_SPOTLIGHT
#define COMMON11_USE_SPOTLIGHT 0
#endif

#define COMMON11_DEFAULT_DIR_LIGHTS 2

//-----------------------------------------------------------------------------

#if COMMON11_USE_SPOTLIGHT_PS
#if COMMON11_USE_SELF_ILLUMINATION
#error Spotlight cannot be used with self illumination
#endif
#if COMMON11_USE_PARTICLE_LIGHTMAP
#error Particle lightmap cannot be used with self illumination
#endif
#endif

//-----------------------------------------------------------------------------

// Input to common pixel shader.
struct PS11_COMMON_INPUT
{
	float4 Position				: POSITION;
	float4 Ambient				: COLOR0;
	float2 DiffuseMapUV			: TEXCOORD_(SAMPLER_DIFFUSE);
#if COMMON11_USE_LIGHTMAP
	float2 LightMapUV			: TEXCOORD_(SAMPLER_LIGHTMAP);
#endif

#if COMMON11_USE_SPOTLIGHT_PS

	float3 Spotlight_DeltaW		: TEXCOORD_(SAMPLER_PARTICLE_LIGHTMAP);	// spot light to vertex
	float3 Spotlight_NormalW	: TEXCOORD_(SAMPLER_SELFILLUM);			// pixel normal
	float3 Spotlight_Color		: COLOR1;								// spot light color

#else

#if COMMON11_USE_SELF_ILLUMINATION
	float2 SelfIllumUV			: TEXCOORD_(SAMPLER_SELFILLUM);
#endif
#if COMMON11_USE_PARTICLE_LIGHTMAP
	float2 ParticleLightMapUV	: TEXCOORD_(SAMPLER_PARTICLE_LIGHTMAP);
#endif

#endif

	float Fog					: FOG;
};

#define PACK(v) (((v) + 1) * 0.5)
#define UNPACK(v) (((v) - 0.5) * 2)

//-----------------------------------------------------------------------------

half3 _EvalSpotlight1(in half3 vSpotFacing, in half3 vSpotToPoint, in half3 vPointNormal, in half3 cSpotColor)
{
	// Angle of incidence between light and pixel.
	half fDot = saturate( dot( vSpotFacing, vSpotToPoint ) );
	
	// "Shape" of light (linear).
	half fPower = fDot;
	
	// Dot against pixel normal.
	fPower *= saturate(dot(vSpotFacing, vPointNormal));
	
	// Combine.
	return cSpotColor * fPower;
}

half3 _EvalSpotlight2(in half3 vSpotFacing, in half3 vSpotToPoint, in half3 vPointNormal, in half3 cSpotColor)
{
	// Angle of incidence between light and pixel.
	half fDot = saturate( dot( vSpotFacing, vSpotToPoint ) );
	
	// "Shape" of light.
	half fPower = fDot;
	fPower *= fPower;
	fPower *= fPower;
	
	// Combine.
	return cSpotColor * fPower;
}

half3 _EvalSpotlight3(in half3 vSpotFacing, in half3 vSpotToPoint, in half3 vPointNormal, in half3 cSpotColor)
{
	// Angle of incidence between light and pixel.
	half fDot = saturate( dot( vSpotFacing, vSpotToPoint ) );
	
	// Compute umbra falloff.
	half2 vFocus = SpotLightsFalloff(WORLD_SPACE)[0].zw;
	fDot = saturate( ( fDot + vFocus.x ) * vFocus.y );
	
	// "Shape" of light (linear).
	half fPower = fDot;
	
	// Combine.
	return cSpotColor * fPower;
}

half3 _EvalSpotlight(in half3 vSpotFacing, in half3 vSpotToPoint, in half3 vPointNormal, in half3 cSpotColor)
{
	return _EvalSpotlight2(vSpotFacing, vSpotToPoint, vPointNormal, cSpotColor);
}

//-----------------------------------------------------------------------------

void VS11Common(
	inout PS11_COMMON_INPUT Out,
	float4 Position,
	half3 No,
	half3 Nw,
	uniform int nPointLights,
	uniform int nDirectionalLights,
	uniform int nSHLevel,
	uniform bool bParticleLightMap,
	uniform branch bSpotlight )
{
	Out.Position = mul(Position, WorldViewProjection);

	Out.Ambient = CombinePointLights(Position, No, nPointLights, 0 /*nLightStart*/, OBJECT_SPACE);
	if (nSHLevel == 9)
	{
		Out.Ambient.rgb += SHEval_9_RGB(Nw);
	}
	if (nSHLevel == 4)
	{
		Out.Ambient.rgb += SHEval_4_RGB(Nw);
	}
	Out.Ambient += GetAmbientColor();
	if (nDirectionalLights > 0)
	{
		Out.Ambient.rgb += GetDirectionalColor(No, DIR_LIGHT_0_DIFFUSE, OBJECT_SPACE);
	}
	if (nDirectionalLights > 1)
	{
		Out.Ambient.rgb += GetDirectionalColor(No, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE);
	}
	Out.Ambient.rgb = saturate( MDR_Pack( Out.Ambient.rgb ) );

	FOGMACRO(Position)

#if COMMON11_USE_PARTICLE_LIGHTMAP
	half4 Temp;
	ComputeShadowCoords(Temp, Position);
	Out.ParticleLightMapUV = Temp.xy;
#endif

	if ((bSpotlight == _ON_VS) || (bSpotlight == _ON_PS))
	{
		PS_COMMON_SPOTLIGHT_INPUT OutSpot = (PS_COMMON_SPOTLIGHT_INPUT)0;
		half3x3 o2tMat = (half3x3)0;	// it's not used

		VS_Spotlight(
			OBJECT_SPACE,
			false,
			Position.xyz,
			o2tMat,
			OutSpot
		);
		
#if COMMON11_USE_SPOTLIGHT_PS
		if (bSpotlight == _ON_PS)
		{
			OutSpot.St.xyz = -normalize(OutSpot.St.xyz);
			Out.Spotlight_DeltaW = PACK(OutSpot.St.xyz);
			Out.Spotlight_NormalW = PACK(-No);
			Out.Spotlight_Color = OutSpot.Cs;
		}
		else
#endif
		{
			// fudge factor to help make VS and PS spotlight
			// brightness similar, since PS normals lose magnitude
			// during interpolation.
			half InterpolatedNormalFactor = 0.85;
			
			half3 vSpotFacing = SpotLightsNormal(OBJECT_SPACE)[0].xyz;
			half3 vSpotToPoint = -normalize(OutSpot.St.xyz) * InterpolatedNormalFactor;
			half3 vPointNormal = -No * InterpolatedNormalFactor;
			half3 cSpotColor = OutSpot.Cs;
			Out.Ambient.rgb += _EvalSpotlight(vSpotFacing, vSpotToPoint, vPointNormal, cSpotColor);
		}
	}
}

//-----------------------------------------------------------------------------

PS11_COMMON_INPUT VS11_COMMON_RIGID(
	VS_BACKGROUND_INPUT_64 In,
	uniform int nPointLights,
	uniform int nDirectionalLights,
	uniform int nSHLevel,
	uniform bool bLightMap,
	uniform bool bParticleLightMap,
	uniform bool bSelfIllum,
	uniform bool bAmbientOcclusion,
	uniform branch bSpotlight )
{
	PS11_COMMON_INPUT Out = (PS11_COMMON_INPUT)0;	// stupid compiler

	half3 No = RIGID_NORMAL;
	half3 Nw = mul( No, World );

	VS11Common(Out, float4( In.Position, 1.0f ), No, Nw, nPointLights, nDirectionalLights, nSHLevel, bParticleLightMap, bSpotlight);
	Out.DiffuseMapUV = In.LightMapDiffuse.zw;
#if COMMON11_USE_SELF_ILLUMINATION
	if (bSelfIllum)
	{
		Out.SelfIllumUV = In.LightMapDiffuse.zw;
	}
#endif

	if ( bAmbientOcclusion )
	{
		Out.Ambient.a = In.Normal.w;
	}

#if COMMON11_USE_LIGHTMAP
	if (bLightMap)
	{
		Out.LightMapUV = In.LightMapDiffuse.xy;
	}
#endif

	return Out;
}

//-----------------------------------------------------------------------------

PS11_COMMON_INPUT VS11_COMMON_ANIMATED(
	VS_ANIMATED_INPUT In,
	uniform int  nPointLights,
	uniform int  nDirectionalLights,
	uniform int  nSHLevel,
	uniform bool bSelfIllum,
	uniform bool bSkinned,
	uniform branch bSpotlight )
{
	PS11_COMMON_INPUT Out = (PS11_COMMON_INPUT)0;	// stupid compiler

	float4 Position;
	half3 Normal;

	if (bSkinned)
	{
#ifdef DXC_SHADERLIMITS_COMPILE_11_AS_20
		GetPositionAndNormal(Position, Normal, In);
#else
		// A D3DCOLOR was used instead of UBYTE4 because some
		// 1.1 cards don't support the UBYTE4 type.
		// This adds one instruction to scale the color
		// components (0 <= x <= 1) by approximately 255
		// to arrive at the original values (0 <= x <= 255).
		int4 Indices = D3DCOLORtoUBYTE4(In.Indices);
		GetPositionAndNormalEx(Position, Normal, In, Indices);
#endif
	}
	else
	{
 		Position = float4( In.Position, 1.0f );
		Normal = ANIMATED_NORMAL;
	}

	half3 Nw = mul( Normal, World );

	VS11Common(Out, Position, Normal, Nw, nPointLights, nDirectionalLights, nSHLevel, false, bSpotlight);
	Out.DiffuseMapUV = In.DiffuseMap;
#if COMMON11_USE_SELF_ILLUMINATION
	if (bSelfIllum)
	{
		Out.SelfIllumUV = In.DiffuseMap;
	}
#endif

	return Out;
}

//-----------------------------------------------------------------------------

// Common pixel shader.
float4 PS11_COMMON(
	PS11_COMMON_INPUT In,
	uniform bool bLightMap,
	uniform bool bParticleLightMap,
	uniform bool bSelfIllum,
	uniform bool bAmbientOcclusion,
	uniform branch bSpotlight ) : COLOR
{
#if 0
	float4 Diffuse = tex2D(DiffuseMapSampler, In.DiffuseMapUV);
	
	Diffuse.rgb *= In.Ambient.rgb;
	
	Diffuse.a *= fAlpha * GLOW_MIN;
	
	return Diffuse;
#endif



	half4 DiffuseMap = tex2D(DiffuseMapSampler, In.DiffuseMapUV);
	half3 Albedo = DiffuseMap.rgb;
	half Opacity = DiffuseMap.a;

	half3 Diffuse = MDR_Unpack( In.Ambient.rgb );

	if ( bAmbientOcclusion )
	{
		Diffuse *= In.Ambient.a;
	}

#if COMMON11_USE_LIGHTMAP
	if (bLightMap)
	{
		Diffuse += SAMPLE_LIGHTMAP(In.LightMapUV);
	}
#endif

#if COMMON11_USE_SPOTLIGHT_PS
	if (bSpotlight == _ON_PS)
	{
		half3 vSpotFacing = SpotLightsNormal(OBJECT_SPACE)[0].xyz;
		half3 vSpotToPoint = UNPACK(In.Spotlight_DeltaW);
		half3 vPointNormal = UNPACK(In.Spotlight_NormalW);
		half3 cSpotColor = In.Spotlight_Color.rgb;
		Diffuse.rgb += _EvalSpotlight(vSpotFacing, vSpotToPoint, vPointNormal, cSpotColor);
	}
#endif

	half4 Color;
	Color.rgb = Albedo * Diffuse;

#if COMMON11_USE_SELF_ILLUMINATION
	if (bSelfIllum)
	{
		//Color.rgb += SAMPLE_SELFILLUM( In.SelfIllumUV );
		half4 SI = SAMPLE_SELFILLUM( In.SelfIllumUV );
		Color.rgb += SI.rgb * SI.a;
	}
#endif

	half fGlow = 0;		// CHB 2007.09.11 - For future expansion.

	// CHB 2007.09.11 - Argh, this adds two useless dp instructions to the shader!	:-(
	Color.a = Opacity * ( ( ( half( 1.0f ) - gfAlphaForLighting ) * gfAlphaConstant ) + ( GLOW_MIN * gfAlphaForLighting ) );
	//Color.a = Opacity * ( ( ( half( 1.0f ) - gfAlphaForLighting ) * gfAlphaConstant ) + ( max( fGlow, GLOW_MIN ) * gfAlphaForLighting ) );
//	Color.a = Opacity * fAlphaScale * GLOW_MIN;
	return Color;
}

//-----------------------------------------------------------------------------

#endif
