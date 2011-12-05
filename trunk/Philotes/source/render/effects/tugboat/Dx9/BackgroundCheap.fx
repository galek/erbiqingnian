//
// 
//

#ifndef INDOOR_VERSION
#define INDOOR_VERSION		0
#endif

#include "_common.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_Shadow.fxh"

//--------------------------------------------------------------------------------------------
struct PS20_BACKGROUND_INPUT
{
    float4 OutPosition			: POSITION; 
    float4 LightMapDiffuse 		: TEXCOORD0; 
    float4 Ambient				: COLOR0; 
    float4 Overbright			: COLOR1;
    float2 DiffuseMap2 			: TEXCOORD1;
    float3 Normal				: TEXCOORD2;
    float4 SpotVector			: TEXCOORD3;
    float4 SpotColor			: TEXCOORD4;
    float4 Position				: TEXCOORD5; 
	float4 ShadowMapCoords		: TEXCOORD6; 
    float  Fog					: FOG;
};

PS20_BACKGROUND_INPUT VS20_BACKGROUND ( 
	VS_BACKGROUND_INPUT_64 In, 
	uniform int nPointLights,
	uniform bool bDiffuseMap2,
	uniform bool bLightMap,
	uniform bool bIndoor )
{
	PS20_BACKGROUND_INPUT Out = (PS20_BACKGROUND_INPUT)0;

	Out.OutPosition = mul( float4( In.Position, 1.0f ), WorldViewProjection );	
	Out.Position = Out.OutPosition;

	if ( bDiffuseMap2 )
		Out.DiffuseMap2 = In.DiffuseMap2;
	Out.LightMapDiffuse = In.LightMapDiffuse;
	
	half3 vInNormal = RIGID_NORMAL;
	
	//if ( gbShadowMap )
	{
		if ( !bIndoor && dot( ShadowLightDir, vInNormal ) >= 0.0f )
			Out.SpotVector.w = 0.0f;
		else
		{
			Out.SpotVector.w = 1.0f;
			ComputeShadowCoords( Out.ShadowMapCoords, In.Position );
		}
	}

	//Out.Ambient.xyz =  CombinePointLightsPerVertex( In.Position , vInNormal, nPointLights, 0 );
	Out.Ambient.xyz = GetAmbientColor();
	if ( !bIndoor )
	{
		half3 N_ws = normalize( mul( vInNormal, World ) );
		Out.Ambient.xyz += SHEval_9_RGB( N_ws );
		Out.Ambient.xyz += GetDirectionalColor( vInNormal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );
	}
	Out.Ambient.xyz = saturate( Out.Ambient.xyz );
	Out.Ambient.w   =  0.0f;
	Out.Overbright  = 0;
	Out.Normal = vInNormal;
	
	half3 vInPos = In.Position;
	FOGMACRO( vInPos )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_BACKGROUND( 
	PS20_BACKGROUND_INPUT In,
	uniform int nPointLights,
	uniform bool bDiffuseMap2,
	uniform bool bLightMap,
	uniform bool bSpecular,
	uniform bool bIndoor ) : COLOR
{
	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

	//fetch base color
	half4 color = tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords );
	
	if ( bDiffuseMap2 )
	{
		half4 diffMap2 = tex2D( DiffuseMapSampler2, In.DiffuseMap2.xy );
		color.xyz = lerp( color.xyz, diffMap2.xyz, diffMap2.w );
	}

	half3 N = normalize( half3( In.Normal ) );
	half4 finalLightColor = In.Ambient;
	
	if ( !bIndoor )
	{
		finalLightColor.xyz += GetDirectionalColor( N, DIR_LIGHT_0_DIFFUSE, OBJECT_SPACE );
		finalLightColor.xyz += GetDirectionalColor( N, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );
	}

	if ( bLightMap ) 
	{
		//fetch light map
		half4 lightMap = SAMPLE_LIGHTMAP( vIn_LightMapCoords);
		finalLightColor += lightMap;
	}
	
	//if ( gbShadowMap )
	{
		half fLightingAmount = In.SpotVector.w;
		if ( fLightingAmount > 0.0f ) // pre-computed to be in darkness if < 0.0f
			fLightingAmount = ComputeLightingAmount( In.ShadowMapCoords );		

		fLightingAmount *= gfShadowIntensity;
		half fShadow = 1.0f - gfShadowIntensity + fLightingAmount;
		finalLightColor.xyz = lerp( 0.0f, finalLightColor, fShadow );
		//finalLightColor.xyz = float3(1,1,1);
	}
	//return float4( finalLightColor.xyz, GLOW_MIN );

	half4 vOverbright = In.Overbright;

	//compute final color
	//if ( nPointLights != 0 )
	//	finalLightColor += In.Ambient;

	color.xyz *= finalLightColor; 
	color.xyz += half3( vOverbright.xyz );
	color.w   *= half( In.Ambient.w ) + half( In.Overbright.w ) + GLOW_MIN;

	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE20(points, diffusemap2, lightmap, specular, debugflag) DECLARE_TECH TVertexAndPixelShader20##points##diffusemap2##lightmap##specular##debugflag < \
	SHADER_VERSION_22 \
	int PointLights = points; \
	int SpecularLights = 0; \
	int Specular = specular; \
	int NormalMap = 0; \
	int DiffuseMap2 = diffusemap2; \
	int LightMap = lightmap; \
	int DebugFlag = debugflag; \
	int Indoor = INDOOR_VERSION; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_BACKGROUND( points, diffusemap2, lightmap, INDOOR_VERSION ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS20_BACKGROUND( points, diffusemap2, lightmap, specular, INDOOR_VERSION ) ); \
 		} } 

#include "Dx9/_BackgroundHeaderCheap.fx"

#include "Dx9/_NoPixelShader.fx"
