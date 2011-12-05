//
// ActorHair Shader - it uses normal-mapped spherical mapping
//

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_EnvMap.fx"
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
    float4 Ambient				: COLOR0;	 // used to be a texcoord to help with overbrightening
    float4 SpotColor			: COLOR1;	 // used to be a texcoord to help with overbrightening
    float2 DiffuseMap			: TEXCOORD0;
    float3 Tangent				: TEXCOORD1;
    float3 Binormal				: TEXCOORD2;
    float3 Normal				: TEXCOORD3;
	float3 SpotVector			: TEXCOORD4;
    float3 WorldPosition		: TEXCOORD6; // w has brightness
    float4 Position				: POSITION;   
};

//--------------------------------------------------------------------------------------------
PS20_ACTOR_INPUT VS20_ACTOR( 
	VS_ANIMATED_INPUT In, 
	uniform int nPointLights, 
	uniform int nPerPixelLights,
	uniform bool bSpotLight )
{
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;

/*
	if ( gbShadowMap && 0 )
	{
		// is vViewPos OK as half precision?
		half4 vViewPos   = mul( In.Position,	  WorldView );				// view position (in worldview)
		Out.ShadowMap1	 = mul( vViewPos,		  ViewToLightProjection );	// position (in light view)
		Out.ViewPosition = vViewPos;
		//Out.ViewPosition = vViewPos * 0.5f + 0.5f; // convert to a color
	}
*/

	float4 Position;
	half3 Normal;

	if ( gbSkinned )
	{
		if ( gbNormalMap )
			GetPositionNormalAndObjectMatrix( Position, Normal, half3x3( Out.Tangent, Out.Binormal, Out.Normal ), In ); 
		else
			GetPositionAndNormal( Position, Normal, In );
			
	} else {
 		Position = float4( In.Position, 1.0f );

		Normal = ANIMATED_NORMAL;
		if ( gbNormalMap )
		{
			Out.Tangent  = ANIMATED_TANGENT;
			Out.Binormal = ANIMATED_BINORMAL;
			Out.Normal   = Normal;
		}
	}

	if ( gbNormalMap )
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


	if ( bSpotLight )
	{
		half3 nop;
		VS_PrepareSpotlight(
			Out.SpotVector,
			nop,
			Out.SpotColor,
			half3( Out.WorldPosition.xyz ),
			0,
			false,
			WORLD_SPACE );
	}

	
	// this could be (should be) rolled into the SH coefs                         
	Out.Ambient			= GetAmbientColor();
	Out.Ambient.xyz		= saturate( Out.Ambient.xyz );


	FOGMACRO_OUTPUT( Out.Ambient.w, Position )
	Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
	Out.Ambient.w = saturate( Out.Ambient.w );

	return Out;
}

//--------------------------------------------------------------------------------------------
void PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	out float4 OutColor : COLOR,
	uniform int nPerPixelLights,
	uniform bool bSpotLight )
{
	half4 cDiffuse		= tex2D( DiffuseMapSampler, In.DiffuseMap );
	half3 cFinalLight	= In.Ambient.xyz;

	float3 P			= In.WorldPosition;
	half3 N;

	cDiffuse.w			*= gfAlphaConstant;
	//return float4( cDiffuse.w, 0, 0, 1 );
	
	// get the world-space normal
	if ( gbNormalMap )
	{
		half3 vNormalTS	= ReadNormalMap( In.DiffuseMap ).xyz;
		
		half3 vTangentVec  = normalize( In.Tangent );
		half3 vBinormalVec = normalize( In.Binormal );
		half3 vNormalVec   = normalize( In.Normal );

	    // xform normalTS into whatever space tangent, binormal, and normal vecs are in (world, in this case)
	    N = TangentToWorld( vTangentVec, vBinormalVec, vNormalVec, vNormalTS );
	} else {
		// already in world-space
		N = normalize( half3( In.Normal ) );
	}

	half fLightingAmount = 1.0f;

	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	half4 vSpecularMap  = 0.0f;
	half  fSpecularPower = 0;
	half3 V = 0;


	if ( gbSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap		= tex2D( SpecularMapSampler, In.DiffuseMap );
		fSpecularPower		= length( vSpecularMap.xyz );
		V					= normalize( half3( EyeInWorld - P ) );
	}

	half4 vEnvMap = EnvMapLookup( N, V, fSpecularPower, vSpecularMap.w, false, true, false );

	if ( gbSpecular )
	{
		vSpecularMap.xyz	*= SpecularBrightness;
	}

	half  fSpecular;
	half3 vPointSpecular;

	for( int i = 0; i < nPerPixelLights; i++ )
	{
		half3 L					= PointLightsPos(WORLD_SPACE)[ i ] - P;
		half fAttenuate			= GetFalloffBrightness ( L, PointLightsFalloff( WORLD_SPACE )[ i ] );
		L						= normalize( L );

		// diffuse
		half Kd					= PSGetDiffuseBrightnessNoBias( L, N );
		vBrighten				+= PointLightsColor[ i ] * ( fAttenuate * Kd );

		if ( gbSpecular )
		{
			half3 H				= normalize( V + L );
			half Ks				= saturate( dot( H, N ) );
			Ks					= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, Ks ) ).b;

			// attenuating point-light specular
			vPointSpecular		= vSpecularMap.xyz * ( Ks * PSGetSpecularBrightness( L, N ) * fAttenuate );
			fGlow				+= Ks;
			vOverBrighten		+= PointLightsColor[ i ] * vPointSpecular;
		}
	}

	// add spherical harmonics contribution
	cFinalLight += SHEval_9_RGB( N );
	
	if ( gbDirectionalLight && ! bSpotLight )
	{
		PS_DirectionalInWorldSB( 0, N, V, vSpecularMap, vBrighten, fGlow, vOverBrighten, gbSpecular );
	}

	if ( gbDirectionalLight2 && ! bSpotLight )
	{
		PS_DirectionalInWorldSB( 1, N, V, vSpecularMap, vBrighten, fGlow, vOverBrighten, gbSpecular );
	}

//	if ( gbSpecular )
//	{
		//vBrighten   += fGlow; // add the glow amount as a white light contribution
		fFinalAlpha += fGlow * SpecularGlow * fSpecularPower * SpecularBrightness;
//	}

	if ( bSpotLight )
	{
		vBrighten.rgb += PS_EvalSpotlight(
							   half3( In.SpotVector.xyz ),
							   SpotLightsNormal(WORLD_SPACE)[ 0 ].xyz,
							   half3( In.SpotColor.rgb ),
							   N,
							   WORLD_SPACE );
	}

	if ( gbLightMap )
	{
		half3 cLightMap = SAMPLE_SELFILLUM( In.DiffuseMap ) * gfSelfIlluminationPower;
		cFinalLight += cLightMap;
		fFinalAlpha += dot( cLightMap.xyz, cLightMap.xyz ) * gfSelfIlluminationGlow;
	}


/*
	if ( gbShadowMap && 0 )
	{
		float fDebugShadowMin = 0.30f;
		cFinalLight += vBrighten * lerp( fDebugShadowMin, 1.0f, fLightingAmount );
	} else {
		cFinalLight += vBrighten;
	}
*/
	cFinalLight += vBrighten;
	
	//cDiffuse.xyz *= saturate( cFinalLight );
	cDiffuse.xyz *= cFinalLight;

	if ( gbCubeEnvironmentMap || gbSphereEnvironmentMap )
	{
		cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );
	}

	cDiffuse.xyz += vOverBrighten;
	cDiffuse.w   *= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( fFinalAlpha + GLOW_MIN ) * gfAlphaForLighting );
	
	// manual fog
	APPLY_FOG( cDiffuse.xyz, In.Ambient.w );

	AppControlledAlphaTest( cDiffuse );
	
	OutColor = cDiffuse;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUENAMED(name,points) \
	DECLARE_TECH name##points \
	< SHADER_VERSION_33 \
	int PointLights = points; \
	int SpotLight = SPOTLIGHT_ENABLED; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS20_ACTOR( points, min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_3_0, PS20_ACTOR( min( points, 1 ), SPOTLIGHT_ENABLED ) ); \
		}  } 
TECHNIQUENAMED(SimpleOneLight,0)
//TECHNIQUENAMED(Everything,1)