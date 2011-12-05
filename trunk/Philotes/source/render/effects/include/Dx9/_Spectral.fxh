//
// Spectral Shader - it uses normal-mapped spherical mapping and a falloff-from-eye effect
//
// transformations
#include "_common.fx"
//#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_ScrollUV.fx"
#include "Dx9/_EnvMap.fx"
#include "Dx9/_AnimatedShared20.fxh"
#include "StateBlocks.fxh"

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
    float4 Ambient				: COLOR0;	 // used to be a texcoord to help with overbrightening
    float2 DiffuseMap			: TEXCOORD0;
    float3 Tangent				: TEXCOORD1;
    float3 Binormal				: TEXCOORD2;
    float3 Normal				: TEXCOORD3;
    float3 WorldPosition		: TEXCOORD6;
    float4 Position				: POSITION;
    float  Fog					: FOG;
};

//--------------------------------------------------------------------------------------------
PS20_ACTOR_INPUT VS20_ACTOR( 
	VS_ANIMATED_INPUT In,
	uniform bool bManualFog,
	uniform bool bSkinned,
	uniform bool bNormalMap )
{
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;

	float4 Position;
	half3 Normal;

	if ( BRANCH( bSkinned ) )
	{
		if ( BRANCH( bNormalMap ) )
			GetPositionNormalAndObjectMatrix( Position, Normal, half3x3( Out.Tangent, Out.Binormal, Out.Normal ), In ); 
		else
			GetPositionAndNormal( Position, Normal, In );
	} else {
 		Position = float4( In.Position, 1.0f );

		Normal = ANIMATED_NORMAL;
		if ( BRANCH( bNormalMap ) )
		{
			Out.Tangent  = ANIMATED_TANGENT;
			Out.Binormal = ANIMATED_BINORMAL;
			Out.Normal   = Normal;
		}
	}

	if ( BRANCH( bNormalMap ) )
	{
		// Xform tangent space matrix to world space
		VectorsTransform( Out.Tangent, Out.Binormal, Out.Normal, World );
	}
	else
	{
		// transform to world-space
		Out.Normal = mul( Normal, World );
	}



	Out.WorldPosition	= mul( Position, World );
	Out.Position		= mul( Position, WorldViewProjection ); 
	Out.DiffuseMap		= In.DiffuseMap;    
	
	Out.Ambient.xyz		= 1;

	if ( bManualFog )
	{
		FOGMACRO_OUTPUT( Out.Ambient.w, Position )
		Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
		Out.Ambient.w = saturate( Out.Ambient.w );
	}
	else
	{
		FOGMACRO( Position );
		Out.Ambient		= 1;
	}

	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	uniform bool bManualFog,
	uniform bool bNormalMap,
	uniform bool bAdditive ) : COLOR
{
	half4 cDiffuse;

	float3 P			= In.WorldPosition;
	half3 N;

	half fAlpha		= tex2D( DiffuseMapSampler, In.DiffuseMap ).w;  // maps blue to alpha
	
	// get the world-space normal
	if ( bNormalMap )
	{
		half3 vNormalTS	= ReadNormalMap( In.DiffuseMap );
		
		half3 vTangentVec  = normalize( In.Tangent );
		half3 vBinormalVec = normalize( In.Binormal );
		half3 vNormalVec   = normalize( In.Normal );

	    // xform normalTS into whatever space tangent, binormal, and normal vecs are in (world, in this case)
	    N = TangentToWorld( vTangentVec, vBinormalVec, vNormalVec, vNormalTS );
	} else {
		// already in world-space
		N = normalize( half3( In.Normal ) );
	}

	half2 vCoords = SphereMapGetCoords( N );
	vCoords = ScrollTextureCoordsHalf( vCoords, SCROLL_CHANNEL_DIFFUSE );
	half3 vEnvMap = tex2D( SphereEnvironmentMapSampler, vCoords );

	//half3 vEnvMap = SphereMapLookup( N );

	// falloff away from eye
	half3 V = normalize( half3( EyeInWorld - P ) );
	half fStr = 1.f - saturate( dot( V, N ) * 0.75f );

	if ( bAdditive )
	{
		//
		cDiffuse.xyz = vEnvMap.xyz * fStr * fAlpha;
		cDiffuse.w = gfSelfIlluminationGlow;

		if ( bManualFog )
			cDiffuse.rgb *= In.Ambient.w;
	}
	else
	{
		// normal lerp blending
		cDiffuse.xyz = vEnvMap.xyz;
		cDiffuse.w = fStr * fAlpha;

		if ( bManualFog )
		{
			APPLY_FOG( cDiffuse.xyz, In.Ambient.w );
		}
	}

	return cDiffuse;
}

//////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENGINE_TARGET_DX10
	#define TECHNIQUE20( skinned, normalmap ) \
		DECLARE_TECH TVertexAndPixelShader20##skinned##normalmap \
		< SHADER_VERSION_22\
		  int Skinned = skinned; \
		  int NormalMap = normalmap; > { \
		pass P0 { \
			COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( 0, skinned, normalmap )); \
			COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS20_ACTOR( 0, normalmap, ADDITIVE_BLEND )); \
			DX10_BLEND_STATES \
			}  } 
#else
	#define TECHNIQUE20( skinned, normalmap ) \
		DECLARE_TECH TVertexAndPixelShader20##skinned##normalmap \
		< SHADER_VERSION_22\
		  int Skinned = skinned; \
		  int NormalMap = normalmap; > { \
		pass P0 { \
			COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( 0, skinned, normalmap )); \
			COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS20_ACTOR( 0, normalmap, ADDITIVE_BLEND )); \
			AlphaBlendEnable = TRUE; \
			COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA; \
			DX9_BLEND_STATES \
			AlphaTestEnable	 = FALSE; \
			}  } 
#endif


#ifndef PRIME
#else
// game techinques
//TECHNIQUE30
TECHNIQUE20( 0, 0 )
TECHNIQUE20( 0, 1 )
TECHNIQUE20( 1, 0 )
TECHNIQUE20( 1, 1 )
#endif
