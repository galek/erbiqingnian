//
// Spectral Shader - it uses normal-mapped spherical mapping and a falloff-from-eye effect
//

#define COMMON11_USE_SELF_ILLUMINATION 0	// no self illum for this effect
#include "_common.fx"
#include "dx9/_Common11.fxh"
#include "Dx9/_ScrollUV.fx"
#include "Dx9/_EnvMap.fx"

PS11_COMMON_INPUT VS20_ACTOR(VS_ANIMATED_INPUT In, uniform bool bSkinned)
{
	PS11_COMMON_INPUT Out = (PS11_COMMON_INPUT)0;

	float4 Position;
	half3 Normal;

	if ( bSkinned )
	{
		// A D3DCOLOR was used instead of UBYTE4 because some
		// 1.1 cards don't support the UBYTE4 type.
		// This adds one instruction to scale the color
		// components (0 <= x <= 1) by approximately 255
		// to arrive at the original values (0 <= x <= 255).
		int4 Indices = D3DCOLORtoUBYTE4(In.Indices);
		GetPositionAndNormalEx(Position, Normal, In, Indices);
	}
	else
	{
 		Position = float4( In.Position, 1.0f );
		Normal = ANIMATED_NORMAL;
	}

	Out.Position = mul( Position, WorldViewProjection );

	Normal = normalize( mul( Normal, World ) );
	half2 vCoords = SphereMapGetCoords( Normal );
	Out.DiffuseMapUV = ScrollTextureCoordsHalf( vCoords, SCROLL_CHANNEL_DIFFUSE );

	float3 WorldPosition = mul( Position, World );
	half3 V = normalize( half3( EyeInWorld - WorldPosition ) );
	half fStr = 1.f - saturate( dot( V, Normal ) * 0.75f );
	Out.Ambient.a = fStr;
	Out.Ambient.rgb = 0;

	FOGMACRO(Position);
	
	return Out;
}

float4 PS20_ACTOR (
	PS11_COMMON_INPUT In,
	uniform bool bAdditive ) : COLOR
{
	// In legacy FXC mode, the SphereEnvMapSampler connects to register s0.
	half4 cDiffuse = tex2D( SphereEnvironmentMapSampler, In.DiffuseMapUV );
	if ( bAdditive )
		cDiffuse.rgb *= In.Ambient.a;
	else
		cDiffuse.a *= In.Ambient.a;
	return cDiffuse;
}

#define TECHNIQUE1x( skinned ) \
	DECLARE_TECH TVertexAndPixelShader1x##skinned \
	< SHADER_VERSION_11\
	  int Skinned = skinned; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS20_ACTOR( skinned )); \
		COMPILE_SET_PIXEL_SHADER ( ps_1_1, PS20_ACTOR( ADDITIVE_BLEND )); \
		AlphaBlendEnable = TRUE; \
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA; \
		DX9_BLEND_STATES \
		AlphaTestEnable	 = FALSE; \
		}  } 

TECHNIQUE1x( 0 )
TECHNIQUE1x( 1 )
