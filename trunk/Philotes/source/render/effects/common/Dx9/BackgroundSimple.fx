//
// 
//

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"

//--------------------------------------------------------------------------------------------
struct PS20_BACKGROUND_INPUT
{
	float4 Position				: POSITION; 
	float4 LightMapDiffuse 		: TEXCOORD0; 
	float3 Normal				: TEXCOORD1;
	float3 SpotVector			: TEXCOORD2;
	float4 SpotColor			: TEXCOORD3;
};


PS20_BACKGROUND_INPUT 
	VS20_BACKGROUND ( 
	VS_BACKGROUND_INPUT_64 In, 
	uniform bool bLightMap,
	uniform bool bSpotLight )
{
	PS20_BACKGROUND_INPUT Out = (PS20_BACKGROUND_INPUT)0;
	Out.Position = mul( In.Position, WorldViewProjection );	
	Out.LightMapDiffuse = In.LightMapDiffuse;
		 
	half3 vInNormal = RIGID_NORMAL;	

	Out.Normal = vInNormal;
	
	if ( bSpotLight )
	{
		half3 nop;
		VS_PrepareSpotlight(
			Out.SpotVector,
			nop,
			Out.SpotColor,
			half3( In.Position.xyz ),
			0,
			false,
			OBJECT_SPACE );

		Out.Normal = vInNormal;
	}
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_BACKGROUND( 
	PS20_BACKGROUND_INPUT In,
	uniform bool bLightMap,
	uniform bool bSpotLight ) 
	: COLOR
{
	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

	//fetch base color
	half4 color = tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords );
	
	half4 finalLightColor = 0;
	finalLightColor.xyz = saturate( GetAmbientColor() * 2.0f );

	if ( bLightMap ) 
	{
		//fetch light map
		half4 lightMap = SAMPLE_LIGHTMAP( vIn_LightMapCoords);
		finalLightColor += lightMap;
	}
	
	half3 N = normalize( half3( In.Normal ) );
	
	if ( bSpotLight )
	{
		finalLightColor.rgb += PS_EvalSpotlight(
								   half3( In.SpotVector.xyz ),
								   SpotLightsNormal(OBJECT_SPACE)[ 0 ].xyz,
								   half3( In.SpotColor.rgb ),
								   N,
								   OBJECT_SPACE );
	}

	finalLightColor.xyz += GetDirectionalColor( N, DIR_LIGHT_0_DIFFUSE, OBJECT_SPACE );
	finalLightColor.xyz += GetDirectionalColor( N, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );
	
	color.xyz *= finalLightColor; 
	color.w   *= GLOW_MIN;

	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE20(lightmap, debugflag) DECLARE_TECH TVertexAndPixelShader20##lightmap##debugflag < \
	SHADER_VERSION_22 \
	int PointLights = 0; \
	int SpecularLights = 0; \
	int Specular = 0; \
	int NormalMap = 0; \
	int DiffuseMap2 = 0; \
	int LightMap = lightmap; \
	int SpotLight = SPOTLIGHT_ENABLED; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_BACKGROUND( lightmap, SPOTLIGHT_ENABLED ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS20_BACKGROUND( lightmap, SPOTLIGHT_ENABLED ) ); \
		} } 

#include "Dx9/_BackgroundSimpleHeader.fx"
#include "Dx9/_NoPixelShader.fx"