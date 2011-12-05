//
// Actor Shader - it is the general purpose shader for non-background objects
//
// transformations

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_AnimatedShared20.fxh"

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Functions
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////
//============================================================================================
//============================================================================================
struct PS20_ACTOR_INPUT
{
    float4 Position				 : POSITION;
    float2 DiffuseMap			 : TEXCOORD0;
    float3 Normal_WS			 : TEXCOORD1;
    float3 SpotVector			 : TEXCOORD2;
    float4 SpotColor			 : TEXCOORD3;
    float  Fog					: FOG;
};

//--------------------------------------------------------------------------------------------
PS20_ACTOR_INPUT VS20_ACTOR( 
	VS_ANIMATED_INPUT In, 
//	uniform int nPointLights, 
	uniform bool bSkinned,
	uniform bool bSpotLight )
{
//	nPointLights = 1;
//	nPerPixelLights = 0;
//	bSkinned = false;
//	bNormalMap = false;
//	bSpecular = false;
	
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;

	float4 Position;
	half3  Normal;
	
	if ( bSkinned )
	{
		GetPositionAndNormal( Position, Normal, In ); 
	} else {
		Normal = ANIMATED_NORMAL;
 		Position = float4( In.Position, 1.0f );
	}
	float3 NormalWS  = normalize( mul( Normal, World ) );
	
	Out.DiffuseMap.xy  =  In.DiffuseMap;
	
	Out.Position	=  mul( Position, WorldViewProjection );               

	if ( bSpotLight )
	{
		half3 Pw = mul( Position, World );
		half3 nop;
		VS_PrepareSpotlight(
			Out.SpotVector,
			nop,
			Out.SpotColor,
			Pw,
			0,
			false,
			WORLD_SPACE );

		//Out.Ambient	= GetAmbientColor();
	}
	Out.Normal_WS = NormalWS;
	//else
	//{
	//	Out.Ambient = GetDirectionalColor( Normal, 0 ) + GetDirectionalColor( Normal, 1 ) + GetAmbientColor();
	//}
	// do closest light the higher-quality "old" way
	//Out.Ambient.xyz		+= CombinePointLightsPerVertex( In.Position, Normal, nPointLights, 0  );
	//Out.Ambient.xyz		+= SHEval_9_RGB( NormalWS );
	//Out.Ambient.xyz	+= SHEval_4_RGB( NormalWS );
	//Out.Ambient.xyz		= saturate( Out.Ambient.xyz );


	FOGMACRO( Position )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	uniform bool bLightMap,
	uniform bool bSpotLight,
	uniform bool bSphericalHarmonics ) : COLOR
{
	half2 diffMapCoor = In.DiffuseMap.xy;
	
	half3 Nw		= In.Normal_WS;
	
	half4 Color     = tex2D( DiffuseMapSampler,		diffMapCoor );
	Color.w			*= gfAlphaConstant;
	
	
	half3 FinalLightColor = GetAmbientColor();//In.Ambient;
	
	if (bSphericalHarmonics)
	{
		FinalLightColor += SHEval_9_RGB( Nw );
	}

	half FinalAlpha = 0;
	if ( bLightMap )
	{
		half3 LightMap = SAMPLE_SELFILLUM( diffMapCoor ) * gfSelfIlluminationPower;
		FinalLightColor += LightMap;
		FinalAlpha = dot( LightMap, LightMap );
	}

	if ( bSpotLight )
	{
		FinalLightColor.rgb += PS_EvalSpotlight(
								   half3( In.SpotVector.xyz ),
								   SpotLightsNormal(WORLD_SPACE)[ 0 ].xyz,
								   half3( In.SpotColor.rgb ),
								   Nw,
								   WORLD_SPACE );
	}
	else
		FinalLightColor.rgb += GetDirectionalColor( Nw, DIR_LIGHT_0_DIFFUSE, WORLD_SPACE ) + GetDirectionalColor( Nw, DIR_LIGHT_1_DIFFUSE, WORLD_SPACE );

	Color.xyz *= FinalLightColor; 
	Color.w   *= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( FinalAlpha + GLOW_MIN ) * gfAlphaForLighting );

	return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
// VS_2_0 and PS_2_0
//--------------------------------------------------------------------------------------------

#define TECHNIQUENAMED(name,skinned,lightmap,bSphericalHarmonics) \
	DECLARE_TECH name##lightmap##skinned##bSphericalHarmonics \
	< SHADER_VERSION_22 \
	int PointLights = 0; \
	int Skinned = skinned; \
	int SelfIllum = lightmap; \
	int SpotLight = SPOTLIGHT_ENABLED; \
	int SphericalHarmonics = bSphericalHarmonics; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( skinned, SPOTLIGHT_ENABLED ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS20_ACTOR( lightmap, SPOTLIGHT_ENABLED, bSphericalHarmonics ) ); \
		}  } 

#define TECHNIQUE20(skinned,bSphericalHarmonics,lightmap) TECHNIQUENAMED(TVertexAndPixelShader20, skinned, lightmap, bSphericalHarmonics)

#ifndef PRIME
TECHNIQUENAMED(Simple,0,0,0)
TECHNIQUENAMED(Everything,0,1,1)
#endif // ! PRIME



TECHNIQUE20(0, 0, 0)
TECHNIQUE20(0, 0, 1)
TECHNIQUE20(0, 1, 0)
TECHNIQUE20(0, 1, 1)
TECHNIQUE20(1, 0, 0)
TECHNIQUE20(1, 0, 1)
TECHNIQUE20(1, 1, 0)
TECHNIQUE20(1, 1, 1)


/*
//------------------------------------
TECHNIQUESHADERONLY_LIGHTMAP(0, 0)
TECHNIQUESHADERONLY_LIGHTMAP(1, 0)

TECHNIQUESHADERONLY_LIGHTMAP(0, 1)
TECHNIQUESHADERONLY_LIGHTMAP(1, 1)

//------------------------------------
TECHNIQUESHADERONLY_NOLIGHTMAP(0, 0)
TECHNIQUESHADERONLY_NOLIGHTMAP(1, 0)

TECHNIQUESHADERONLY_NOLIGHTMAP(0, 1)
TECHNIQUESHADERONLY_NOLIGHTMAP(1, 1)
*/
