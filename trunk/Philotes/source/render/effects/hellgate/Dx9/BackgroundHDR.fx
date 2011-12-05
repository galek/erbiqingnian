//
// 
//

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#ifndef INDOOR_VERSION
#define INDOOR_VERSION		0
#endif

#include "_common.fx"
#include "Dx9/_EnvMap.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_Shadow.fxh"

#define VERTEX_FORMAT_BACKGROUND_64BYTES 0
#define VERTEX_FORMAT_BACKGROUND_32BYTES 1

//--------------------------------------------------------------------------------------------
struct PS30_BACKGROUND_INPUT
{
	float4 Ambient				: TEXCOORD1;
	float4 SpecularColor		: COLOR1;
	float4 LightMapDiffuse 		: TEXCOORD0; 
	float3 Normal				: COLOR0;	// might be unnecessary; could be rolled into LightDir?
	float4 SpecularLightVector	: TEXCOORD2;
	float4 DiffuseLightVector	: TEXCOORD3;
	float4 ObjPosition			: TEXCOORD4;	// sticking diffmap2 u in the .w
	float4 EyePosition			: TEXCOORD5;	// sticking diffmap2 v in the .w
	float3 EnvMapCoords			: TEXCOORD6;
	float4 ShadowMapCoords		: TEXCOORD7; 
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS30_BACKGROUND_INPUT VS30_BACKGROUND_64BYTES ( 
	out float4 OutPosition : POSITION,
	VS_BACKGROUND_INPUT_64 In, 
	uniform int nPointLights,
	uniform bool bSpotlight,
	uniform bool bIndoor )
{
	PS30_BACKGROUND_INPUT Out = (PS30_BACKGROUND_INPUT)0;

	OutPosition.xyzw = mul( float4( In.Position, 1.0f ), WorldViewProjection );	

	// compute the 3x3 transform from tangent space to object space
	half3x3 objToTangentSpace;
	half3 vNormal;
	VS_GetRigidNormal( In.Normal, In.Tangent, In.Binormal, objToTangentSpace, vNormal, false );

	if ( gbShadowMap )
	{
		if ( !bIndoor && dot( ShadowLightDir, vNormal ) >= 0.0f )
			Out.DiffuseLightVector.w = 0.0f;
		else
		{
			Out.DiffuseLightVector.w = 1.0f;
			ComputeShadowCoords( Out.ShadowMapCoords, In.Position );
		}
	}
	
	half3 OutNormal = vNormal;

	if ( gbCubeEnvironmentMap )
	{
		Out.EnvMapCoords.xyz	= mul( CubeMapGetCoords( half3( In.Position.xyz ),
														 half3( EyeInObject.xyz ),
														 vNormal ),		half4x4( World ) );
	}

	if ( gbSpecular && ! bSpotlight )
	{
		if ( ! bIndoor )
		{
			// directional diffuse added per-pixel
			// SpecularLightVector gets dir specular
			// DiffuseLightVector  gets dir diffuse

			Out.SpecularLightVector.xyz		= gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_SPECULAR ].L.xyz;
			Out.DiffuseLightVector.xyz		= gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_DIFFUSE ].L.xyz;
		}
		else
		{
			// no directional lighting
			// SpecularLightVector gets point light specular
			// DiffuseLightVector  gets nothing (unused)
			VS_PickAndPrepareSpecular( In.Position.xyz, vNormal, Out.SpecularColor, Out.SpecularLightVector, false, OBJECT_SPACE );
		}
		
		Out.ObjPosition.xyz		= In.Position.xyz;
		Out.EyePosition.xyz		= EyeInObject;

		// assume a normal map if you have specular
		Out.SpecularLightVector.xyz	= mul( objToTangentSpace, half3( Out.SpecularLightVector.xyz ) );
		Out.DiffuseLightVector.xyz		= mul( objToTangentSpace, half3( Out.DiffuseLightVector.xyz ) );
		Out.ObjPosition.xyz			= mul( objToTangentSpace, half3( Out.ObjPosition.xyz ) );
		Out.EyePosition.xyz			= mul( objToTangentSpace, half3( Out.EyePosition.xyz ) );
		OutNormal					= mul( objToTangentSpace, half3( OutNormal ) );
	}

	if ( bSpotlight )
	{
		VS_PrepareSpotlight(
			Out.SpecularLightVector.xyz,
			Out.DiffuseLightVector.xyz,
			Out.SpecularColor,
			half3( In.Position.xyz ),
			objToTangentSpace,
			false,
			OBJECT_SPACE );

		if ( gbNormalMap )
		{
			//Out.ObjPosition.xyz		= mul( objToTangentSpace, half3( Out.ObjPosition.xyz ) );
			//Out.EyePosition.xyz		= mul( objToTangentSpace, half3( Out.EyePosition.xyz ) );
			OutNormal				= mul( objToTangentSpace, half3( OutNormal ) );
		} 
	}

	if ( gbDiffuseMap2 )
	{
		// Packing diffuse 2 into spare .w coords
		Out.ObjPosition.w = In.DiffuseMap2.x;
		Out.EyePosition.w = In.DiffuseMap2.y;
	}
	
	Out.LightMapDiffuse = In.LightMapDiffuse;

	Out.Ambient.xyz = CombinePointLights( In.Position, vNormal, nPointLights, 0, OBJECT_SPACE );
	Out.Ambient.xyz += GetAmbientColor();
	if ( !bIndoor )
	{
		half3 N_ws = normalize( mul( vNormal, World ) );
		Out.Ambient.xyz += SHEval_9_RGB( N_ws );
		Out.Ambient.xyz += GetDirectionalColor( vNormal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );
	}
	//Out.Ambient.xyz = saturate( Out.Ambient.xyz );

	// needs to check fog enabled
	FOGMACRO_OUTPUT( Out.Ambient.w, In.Position )
	Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
	Out.Ambient.w = saturate( Out.Ambient.w );

	Out.Normal = PACK_NORMAL(OutNormal);

	return Out;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS30_BACKGROUND_INPUT VS30_BACKGROUND_32BYTES ( 
	out float4 OutPosition : POSITION,
	VS_BACKGROUND_INPUT_32 In, 
	uniform int nPointLights,
	uniform bool bSpotlight,
	uniform bool bIndoor )
{
	PS30_BACKGROUND_INPUT Out = (PS30_BACKGROUND_INPUT)0;

	OutPosition.xyzw = mul( float4( In.Position, 1.0f ), WorldViewProjection );	

    // compute the 3x3 transform from tangent space to object space
	half3 vNormal = RIGID_NORMAL;

	if ( gbShadowMap )
	{	
		if ( !bIndoor && dot( ShadowLightDir, vNormal ) >= 0.0f )
			Out.DiffuseLightVector.w = 0.0f;
		else
		{
			Out.DiffuseLightVector.w = 1.0f;
			ComputeShadowCoords( Out.ShadowMapCoords, In.Position );
		}
	}
	
    Out.Normal.xyz = vNormal;

	if ( gbCubeEnvironmentMap )
	{
		Out.EnvMapCoords.xyz	= mul( CubeMapGetCoords( half3( In.Position.xyz ),
														 half3( EyeInObject.xyz ),
														 vNormal ),		half4x4( World ) );
	}

	if ( gbSpecular )
	{
		if ( ! bIndoor )
		{
			// directional diffuse added per-pixel
			// SpecularLightVector gets dir specular
			// DiffuseLightVector  gets dir diffuse

			Out.SpecularLightVector.xyz		= gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_SPECULAR ].L.xyz;
			Out.DiffuseLightVector.xyz		= gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_DIFFUSE ].L.xyz;
		}
		else
		{
			// no directional lighting
			// SpecularLightVector gets point light specular
			// DiffuseLightVector  gets nothing (unused)
			VS_PickAndPrepareSpecular( In.Position.xyz, vNormal, Out.SpecularColor, Out.SpecularLightVector, false, OBJECT_SPACE );
		}

		Out.ObjPosition.xyz	= In.Position.xyz;
		Out.EyePosition.xyz	= EyeInObject;
	}

	if ( bSpotlight )
	{
		VS_PrepareSpotlight(
			Out.SpecularLightVector.xyz,
			Out.DiffuseLightVector.xyz,
			Out.SpecularColor,
			half3( In.Position.xyz ),
			0,
			false,
			OBJECT_SPACE );
	}
	
	Out.LightMapDiffuse = In.LightMapDiffuse;

	Out.Ambient.xyz = CombinePointLights( In.Position, vNormal, nPointLights, 0, OBJECT_SPACE );
	Out.Ambient.xyz += GetAmbientColor();
	if ( !bIndoor )
	{
		half3 N_ws = normalize( mul( vNormal, World ) );
		Out.Ambient.xyz += SHEval_9_RGB( N_ws );
		Out.Ambient.xyz += GetDirectionalColor( vNormal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );
	}
	Out.Ambient.xyz = saturate( Out.Ambient.xyz );

	// needs to check fog enabled
	FOGMACRO_OUTPUT( Out.Ambient.w, In.Position )
	Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
	Out.Ambient.w = saturate( Out.Ambient.w );
			
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS30_BACKGROUND( 
	PS30_BACKGROUND_INPUT In,
	uniform bool bSpotlight,
	uniform bool bIndoor ) : COLOR
{
	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

    //fetch base color
	half3	cAlbedo			= tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords );

	//return float4( cAlbedo, 1.f );

	//return float4( cDiffuse.xyz, GLOW_MIN );
	half3	cFinalLight		= In.Ambient.xyz;
	//return float4( cFinalLight.xyz, GLOW_MIN );
	half	fOpacity		= tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords ).w;

	if ( gbDiffuseMap2 )
	{
		half2 vDiffMap2Coords = half2( In.ObjPosition.w, In.EyePosition.w );
		half4 cDiffMap2		  = tex2D( DiffuseMapSampler2, half2( vDiffMap2Coords) );
		cAlbedo				  = lerp( cAlbedo, cDiffMap2.xyz, cDiffMap2.w );
	}

	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;

	half3 N;
	if ( gbNormalMap )
	{
		//fetch bump normal and expand it to [-1,1]
		N = ReadNormalMap( vIn_DiffuseMapCoords ).xyz;
	}
	else
	{
		N = normalize( UNPACK_NORMAL(half3(In.Normal)) );
	}

	half4 vSpecularMap = (half4)0.0f;
	half fSpecularPower = 0.0f;

	cFinalLight.rgb = float3(0,1,0);

    if ( gbSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap			= tex2D( SpecularMapSampler, vIn_DiffuseMapCoords );
		fSpecularPower			= length( vSpecularMap.xyz );
		
		if ( !bSpotlight )
		{
			half3 L;	


			half3 V = normalize( half3( In.EyePosition.xyz ) - half3( In.ObjPosition.xyz ) );
			half fGloss	= PS_GetSpecularGloss( vSpecularMap );

			//if ( ! bIndoor )
			if ( 1 )
			{
				//if ( gbDirectionalLight )
				if ( 1 )
				{
					half3 vDirDiff = normalize( half3( In.DiffuseLightVector.xyz ) );
					half3 vDirSpec = normalize( half3( In.SpecularLightVector.xyz ) );

					// directional light
					PS_EvaluateSpecular( V, vDirSpec, N, gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_SPECULAR ].C, vSpecularMap, fGloss, fGlow, vOverBrighten, true );
					cFinalLight.rgb += GetDirectionalColorEX( N, vDirDiff, gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_DIFFUSE ].C );
					cFinalLight.rgb = float3(1,0,0);
				}
			}
			else
			{
			
					// specular point light
					L = normalize( half3( In.SpecularLightVector.xyz ) );
					half4 vColor = In.SpecularColor;
					PS_SpecularCoverBoundary( In.SpecularLightVector.w, vColor );
					//vOverBrighten = vColor.rgb;
					//vOverBrighten = In.SpecularLightVector.www;
					PS_EvaluateSpecular( V, L, N, vColor, vSpecularMap, fGloss, fGlow, vOverBrighten, true );

			}
		}
	}

	return float4( cFinalLight.rgb, 1.f );

    if ( bSpotlight )
	{
		cFinalLight.xyz += PS_EvalSpotlight(
							   In.SpecularLightVector.xyz,
							   In.DiffuseLightVector.xyz,
							   In.SpecularColor.rgb,
							   N,
							   OBJECT_SPACE );
	}

	if ( gbLightMap )
	{
		half3 cLightMap = SAMPLE_LIGHTMAP( vIn_LightMapCoords );
		//cFinalLight.xyz += cLightMap;
	}

	if ( gbShadowMap )
	{
		half fLightingAmount = In.DiffuseLightVector.w;
		if ( fLightingAmount > 0.0f ) // pre-computed to be in darkness if < 0.0f
			fLightingAmount = ComputeLightingAmount( In.ShadowMapCoords );

		vOverBrighten	= lerp( 0.0f, vOverBrighten, fLightingAmount ); // specular
		fLightingAmount *= gfShadowIntensity;
		half fShadow = 1.0f - gfShadowIntensity + fLightingAmount;
		cFinalLight.xyz = lerp( 0.0f, cFinalLight, fShadow );
	}

	//cDiffuse.xyz *= saturate( cFinalLight.xyz );
	cAlbedo *= cFinalLight.xyz;

	half4 vEnvMap = EnvMapLookupCoords( N, normalize( half3( In.EnvMapCoords ) ), fSpecularPower, vSpecularMap.a, false );
	if ( gbCubeEnvironmentMap )
		cAlbedo  = lerp( cAlbedo, vEnvMap.xyz, saturate( vEnvMap.w ) );

	cAlbedo += vOverBrighten;
	//cDiffuse.w   *= fFinalAlpha + GLOW_MIN;
	fOpacity   *= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( fFinalAlpha + GLOW_MIN ) * gfAlphaForLighting );

	// manual fog
	// needs to check fog enabled
	//APPLY_FOG( cAlbedo, In.Ambient.w );
	
	AppControlledAlphaTest( cAlbedo );

	return float4( cAlbedo, fOpacity );
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#ifdef PRIME

#ifdef ENGINE_TARGET_DX10
// Warning : still considered as 3_0 in excel spreadsheet. 
// TODO: update the excel file, make a ps_4_0 version...
PixelShader gPixelShader = COMPILE_PIXEL_SHADER( ps_4_0, PS30_BACKGROUND( SPOTLIGHT_ENABLED, INDOOR_VERSION ) );
#else
PixelShader gPixelShader = COMPILE_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SPOTLIGHT_ENABLED, INDOOR_VERSION ) );
#endif

// NOTE: limiting number of point lights passed in to VS to 2 until FXC fix is in!


#define TECHNIQUE30_64BYTES(points) \
    DECLARE_TECH TBackgroundStatic33_64_##points < \
	SHADER_VERSION_33 \
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_64BYTES; \
	int PointLights		= points; \
	int SpotLight		= SPOTLIGHT_ENABLED; \
	int Indoor			= INDOOR_VERSION; > { \
	pass Main	{ \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_64BYTES( min(points,2), SPOTLIGHT_ENABLED, INDOOR_VERSION ) ); \
		SET_PIXEL_SHADER( gPixelShader ); \
		DXC_RASTERIZER_SOLID_BACK; \
		DXC_BLEND_RGBA_NO_BLEND; \
		DXC_DEPTHSTENCIL_ZREAD_ZWRITE; \
		} } 

TECHNIQUE30_64BYTES(0)
TECHNIQUE30_64BYTES(1)
TECHNIQUE30_64BYTES(2)
TECHNIQUE30_64BYTES(3)
TECHNIQUE30_64BYTES(4)
TECHNIQUE30_64BYTES(5)

#define TECHNIQUE30_32BYTES(points) \
    DECLARE_TECH TBackgroundStatic33_32_##points < \
	SHADER_VERSION_33 \
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_32BYTES; \
	int PointLights		= points; \
	int SpotLight		= SPOTLIGHT_ENABLED; \
	int Indoor			= INDOOR_VERSION; > { \
	pass Main	{ \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_32BYTES( min(points,2), SPOTLIGHT_ENABLED, INDOOR_VERSION ) ); \
		SET_PIXEL_SHADER( gPixelShader ); \
		DXC_RASTERIZER_SOLID_BACK; \
		DXC_BLEND_RGBA_NO_BLEND; \
		DXC_DEPTHSTENCIL_ZREAD_ZWRITE; \
		} } 

TECHNIQUE30_32BYTES(0)
TECHNIQUE30_32BYTES(1)
TECHNIQUE30_32BYTES(2)
TECHNIQUE30_32BYTES(3)
TECHNIQUE30_32BYTES(4)
TECHNIQUE30_32BYTES(5)

#else

DECLARE_TECH TBackgroundStatic33_64_NonPrime <
	SHADER_VERSION_33;
	int VertexFormat	= VERTEX_FORMAT_BACKGROUND_64BYTES;
	int PointLights		= 0;
	int SpotLight		= SPOTLIGHT_ENABLED;
	int Indoor			= INDOOR_VERSION; >
{
	pass Main
	{
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS30_BACKGROUND_64BYTES( 0, SPOTLIGHT_ENABLED, INDOOR_VERSION ) );
		COMPILE_SET_PIXEL_SHADER( ps_3_0, PS30_BACKGROUND( SPOTLIGHT_ENABLED, INDOOR_VERSION ) );
		DX10_GLOBAL_STATE
	}
} 

#endif
	
