//
// ActorStatic Shader - it is the general purpose shader for non-background objects (static branch)
//

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_AnimatedShared20.fxh"
#include "Dx9/_EnvMap.fx"

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////
//============================================================================================
//============================================================================================
struct PS20_ACTOR_INPUT
{
    float4 Ambient				: COLOR0;	 // used to be a texcoord to help with overbrightening
    float4 SpotColor			: COLOR1;
    float2 DiffuseMap			: TEXCOORD0;
    float4 Normal				: TEXCOORD1; // w has linear-w distance
    float4 LightVector[ 1 ]		: TEXCOORD2; // w has brightness // array for code compatibility
    float4 DirLightVector[ 2 ]	: TEXCOORD3; // w has brightness
    float3 ObjPosition			: TEXCOORD5;
    float3 EyePosition			: TEXCOORD6;
    float3 EnvMapCoords			: TEXCOORD7;
    //float  Fog					: FOG;
    float4 Position				: POSITION;
};

//--------------------------------------------------------------------------------------------
PS20_ACTOR_INPUT VS20_ACTOR( 
	VS_ANIMATED_INPUT In, 
	uniform int nPointLights, 
	uniform int nPerPixelLights,
	uniform bool bSpotLight )
{
//	nPointLights = 1;
//	nPerPixelLights = 0;

	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;
	
	float4 Position;
	half3 Normal;
	
	half3x3 objToTangentSpace = 0;

	if ( gbSkinned )
	{
		if ( gbNormalMap )
			GetPositionNormalAndObjectMatrix( Position, Normal, objToTangentSpace, In ); 
		else
			GetPositionAndNormal( Position, Normal, In );
	} else {
	
 		Position = float4( In.Position, 1.0f );

		Normal = ANIMATED_NORMAL;
		if ( gbNormalMap )
		{
			objToTangentSpace[0] = ANIMATED_TANGENT;
			objToTangentSpace[1] = ANIMATED_BINORMAL;
			objToTangentSpace[2] = Normal;
		}
	}
	
	if ( gbNormalMap )
		Out.Normal.xyz = mul( objToTangentSpace, Normal );
	else
		Out.Normal.xyz = Normal;

	if ( gbCubeEnvironmentMap )
	{
		// what about sending vViewToPos down as COLOR1, packed?
		half3 vViewToPos = normalize( half3( Position.xyz ) - half3( EyeInObject.xyz ) );
		Out.EnvMapCoords = normalize( reflect( vViewToPos, Normal ) );
	}

	for ( int i = 0; i < nPerPixelLights && i < nPointLights; i++ )
	{
		float3 LightVector = PointLightsPos(OBJECT_SPACE)[ i ] - Position;
		Out.LightVector[ i ].w = GetFalloffBrightness ( LightVector, PointLightsFalloff( OBJECT_SPACE )[ i ] );
		
		if ( gbNormalMap )
			Out.LightVector[ i ].xyz = mul( objToTangentSpace, PointLightsPos(OBJECT_SPACE)[ i ] );
		else
			Out.LightVector[ i ].xyz = PointLightsPos(OBJECT_SPACE)[ i ];
	}

	if ( gbSpecular )
	{
		Out.ObjPosition = Position;
		Out.EyePosition = EyeInObject;
		if ( gbNormalMap )
		{
			Out.ObjPosition = mul( objToTangentSpace, half3( Out.ObjPosition ) );
			Out.EyePosition = mul( objToTangentSpace, half3( Out.EyePosition ) );
		}
	} 

	if ( bSpotLight )
	{
		VS_PrepareSpotlight(
			Out.DirLightVector[0].xyz,
			Out.DirLightVector[1].xyz,
			Out.SpotColor,
			half3( Position.xyz ),
			objToTangentSpace,
			false,
			OBJECT_SPACE );

		if ( gbNormalMap )
		{
			Out.ObjPosition			= mul( objToTangentSpace, half3( Out.ObjPosition ) );
			Out.EyePosition			= mul( objToTangentSpace, half3( Out.EyePosition ) );
		} 
	}
  
	Out.Position		= mul( Position, WorldViewProjection ); 
	Out.DiffuseMap.xy	= In.DiffuseMap;                             
	//Out.Ambient		= CombinePointLightsPerVertex( Position, Normal, nPointLights, nPerPixelLights );
	half3 NormalWS		= normalize( mul( Normal, half4x4( World ) ) );
	Out.Ambient.xyz		= SHEval_9_RGB( NormalWS );
	Out.Ambient			+= GetAmbientColor();
	Out.Ambient.xyz		= saturate( Out.Ambient.xyz );

	// directional light	
	if ( gbDirectionalLight && ! bSpotLight )
	{
		half4 L0		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ];

		// INTENTIONALLY using the diffuse vector for specular as well
		if ( gbNormalMap )
			Out.DirLightVector[ 0 ].xyz = mul( objToTangentSpace, half3( L0.xyz ) );
		else
			Out.DirLightVector[ 0 ].xyz = L0.xyz;
		Out.DirLightVector[ 0 ].w = 1.0f;
	}

	if ( gbDirectionalLight2 && ! bSpotLight )
	{
		half4 L1		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_1_DIFFUSE ];

		if ( gbNormalMap )
			Out.DirLightVector[ 1 ].xyz = mul( objToTangentSpace, half3( L1.xyz ) );
		else
			Out.DirLightVector[ 1 ].xyz = L1.xyz;
		Out.DirLightVector[ 1 ].w = 1.0f;
	}

	FOGMACRO_OUTPUT( Out.Ambient.w, Position )
	Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
	Out.Ambient.w = saturate( Out.Ambient.w );

	return Out;
}

//#define WARDROBED_INVENTORY_NO_BODY
//--------------------------------------------------------------------------------------------
void PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	out	float4						OutColor		: COLOR,
	//
	uniform int nPerPixelLights,
	uniform bool bSpotLight )
{
	half2 vDiffuseMap	= In.DiffuseMap;

	half4 cDiffuse		= tex2D( DiffuseMapSampler, vDiffuseMap );
	half3 cFinalLight	= In.Ambient.xyz;
	half3 vNormal;
		
#ifdef WARDROBED_INVENTORY_NO_BODY
	if ( cDiffuse.w <= 0.01f )
		cDiffuse = 0.0f;
	else
	{
#endif
		cDiffuse.w			*= gfAlphaConstant;
#ifdef WARDROBED_INVENTORY_NO_BODY
	}

	return float4(cDiffuse.xyz,cDiffuse.w);
#endif

	if ( gbNormalMap )
	{
//		vNormal = tex2D( NormalMapSampler,vDiffuseMap ).agb;
//		return float4( vNormal, 0.0f );
		vNormal			= ReadNormalMap( vDiffuseMap );
	} else {
		vNormal			= normalize( half3( In.Normal.xyz ) );
//		return float4( vNormal, 1.0f );
	}

	half fLightingAmount = 1.0f;
	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	half4 vSpecularMap  = 0.0f;
	half  fSpecularPower = 0;
	half3 vViewVector = 0;

	if ( gbSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap		= tex2D( SpecularMapSampler, vDiffuseMap );
		fSpecularPower		= length( vSpecularMap.xyz );
		//fSpecularLevel		= lerp( SpecularBrightness.x, SpecularBrightness.y, vSpecularMap.w );
		vViewVector			= normalize( half3( In.EyePosition ) - half3( In.ObjPosition ) );
	}
	
	half4 vEnvMap = 0;
	if ( gbCubeEnvironmentMap )
	{
		half3 vCoords;
		vCoords		= normalize( half3( In.EnvMapCoords ) );
		vEnvMap		= EnvMapLookupCoords( vNormal, vCoords, fSpecularPower, vSpecularMap.w, false, false, false );

		//cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap, fEnvMapPower );
	}

	if ( gbSpecular )
	{
		vSpecularMap.xyz	*= SpecularBrightness;
	}

	half3 vLightVector;
	half3 vHalfVector;
	half  fSpecular;
	half  fDot;
	half  fLightBrightness;
	half  fPointSpecularAmt;
	half3 vPointSpecular;

	for( int i = 0; i < nPerPixelLights; i++ )
	{
		vLightVector		= normalize( half3( In.LightVector[ i ].xyz ) - half3( In.ObjPosition ) );
		fLightBrightness	= PSGetDiffuseBrightnessNoBias( vLightVector, vNormal );
		//fLightBrightness	*= In.LightVector[ i ].w; // attenuation
		vBrighten			+= PointLightsColor[ i ].rgb * ( fLightBrightness * half( In.LightVector[ i ].w ) );

		if ( gbSpecular && ! bSpotLight )
		{
			vHalfVector			= normalize( vViewVector + vLightVector );
			fDot				= saturate( dot( vHalfVector, vNormal ) );
			fPointSpecularAmt	= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, fDot ) );
			// attenuating point-light specular
			vPointSpecular		= vSpecularMap.xyz * ( fPointSpecularAmt * PSGetSpecularBrightness( vLightVector, vNormal ) * half( In.LightVector[ i ].w ) );
			fGlow				+= fPointSpecularAmt;
			vOverBrighten		+= PointLightsColor[ i ].rgb * vPointSpecular;
		}
	}

	// directional light
	if ( gbDirectionalLight && ! bSpotLight )
	{
		if ( gbNormalMap )
			vLightVector = normalize( half3( In.DirLightVector[ 0 ].xyz ) );
		else
			vLightVector = In.DirLightVector[ 0 ].xyz;
		fLightBrightness	= PSGetDiffuseBrightnessNoBias( vLightVector, vNormal );
		//fLightBrightness	*= In.LightVector[ i ].w; // attenuation
		vBrighten			+= fLightBrightness * DirLightsColor[ DIR_LIGHT_0_DIFFUSE ].xyz;
		if ( gbSpecular )
		{
			vHalfVector			= normalize( vViewVector + vLightVector );
			fDot				= saturate( dot( vHalfVector, vNormal ) );
			fPointSpecularAmt	= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, fDot ) );
			vPointSpecular		= fPointSpecularAmt * vSpecularMap.xyz * PSGetSpecularBrightness( vLightVector, vNormal );
			fGlow				+= fPointSpecularAmt;
			vOverBrighten		+= DirLightsColor[ DIR_LIGHT_0_SPECULAR ].xyz * vPointSpecular;
		}
	}
	
	if ( gbDirectionalLight2 && ! bSpotLight )
	{
		if ( gbNormalMap )
			vLightVector = normalize( half3( In.DirLightVector[ 1 ].xyz ) );
		else
			vLightVector = In.DirLightVector[ 1 ].xyz;
		fLightBrightness	= PSGetDiffuseBrightnessNoBias( vLightVector, vNormal );
		//fLightBrightness	*= In.LightVector[ i ].w; // attenuation
		vBrighten			+= fLightBrightness * DirLightsColor[ DIR_LIGHT_1_DIFFUSE ].xyz;
		if ( gbSpecular )
		{
			vHalfVector			= normalize( vViewVector + vLightVector );
			fDot				= saturate( dot( vHalfVector, vNormal ) );
			fPointSpecularAmt	= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, fDot ) );
			vPointSpecular		= fPointSpecularAmt * vSpecularMap.xyz * PSGetSpecularBrightness( vLightVector, vNormal );
			fGlow				+= fPointSpecularAmt;
			vOverBrighten		+= DirLightsColor[ DIR_LIGHT_1_DIFFUSE ].xyz * vPointSpecular;
		}
	}

	if ( bSpotLight )
	{
		vBrighten.rgb += PS_EvalSpotlight(
							   half3( In.DirLightVector[0].xyz ),
							   half3( In.DirLightVector[1].xyz ),
							   half3( In.SpotColor.rgb ),
							   vNormal,
							   OBJECT_SPACE );
	}

//	if ( gbSpecular )
//	{
		//vBrighten   += fGlow; // add the glow amount as a white light contribution
		fFinalAlpha += fGlow * SpecularGlow * fSpecularPower * SpecularBrightness;
//	}

	if ( gbLightMap )
	{
		half3 cLightMap = SAMPLE_SELFILLUM( vDiffuseMap ) * gfSelfIlluminationPower;
		cFinalLight += cLightMap;
		fFinalAlpha += dot( cLightMap.xyz, cLightMap.xyz ) * gfSelfIlluminationGlow;
	}

	cFinalLight += vBrighten;
	
	//cDiffuse.xyz *= saturate( cFinalLight );
	cDiffuse.xyz *= cFinalLight;

	if ( gbCubeEnvironmentMap )
	{
		cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );
	}

	cDiffuse.xyz += vOverBrighten;

#ifdef ENGINE_TARGET_DX10
	cDiffuse.w   *= ( half( 1.0f ) - gfAlphaForLighting );
#else
	cDiffuse.w   *= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( fFinalAlpha + GLOW_MIN ) * gfAlphaForLighting );
#endif

	// manual fog
	APPLY_FOG( cDiffuse.xyz, half( In.Ambient.w ) );

	OutColor = cDiffuse;
}

//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUE20(points) \
	DECLARE_TECH TVertexAndPixelShader20##points \
	<  \
	SHADER_VERSION_33\
	int PointLights = points; \
	int SpotLight = SPOTLIGHT_ENABLED; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS20_ACTOR( points, min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_3_0, PS20_ACTOR( min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
		}  } 

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUENAMED(name,points) \
	DECLARE_TECH name \
	<  \
	SHADER_VERSION_33\
	int PointLights = points; \
	int SpotLight = SPOTLIGHT_ENABLED; > { \
	pass P0 { \
	COMPILE_SET_VERTEX_SHADER( vs_3_0, VS20_ACTOR( points, min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
	COMPILE_SET_PIXEL_SHADER( ps_3_0, PS20_ACTOR( min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
		}  } 

#ifndef PRIME
TECHNIQUENAMED(SimpleOneLight,1)
TECHNIQUENAMED(Everything,1)
#else
// game techinques
TECHNIQUE20(0)
TECHNIQUE20(1)
#endif
