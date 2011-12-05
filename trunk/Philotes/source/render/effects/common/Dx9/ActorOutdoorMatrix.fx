//
// ActorIndoor Shader - it is the general purpose shader for non-background objects (indoors)
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
    float4 Position				: POSITION;
    float3 Ambient				: COLOR0;	 // used to be a texcoord to help with overbrightening
    float3 SpotColor			: COLOR1;
    float2 DiffuseMap			: TEXCOORD0;
    float4 Normal				: TEXCOORD1; // w has linear-w distance
    //float3 ViewPosition			: TEXCOORD2; 
    //float4 ShadowMap1 			: TEXCOORD3; 
    float3 SpotVector			: TEXCOORD2; 
    float4 LightVector[ 1 ]		: TEXCOORD4; // w has brightness // array for code compatibility
    float4 DirLightVector		: TEXCOORD5; // w has brightness
    float3 ObjPosition			: TEXCOORD6;
    float3 EyePosition			: TEXCOORD7;
    float  Fog					: FOG;
};

//--------------------------------------------------------------------------------------------
PS20_ACTOR_INPUT VS20_ACTOR( 
	VS_ANIMATED_INPUT In, 
	uniform int nPointLights, 
	uniform int nPerPixelLights, 
	uniform bool bNormalMap,
	uniform bool bSpecular,
	uniform bool bSkinned,
	uniform bool bShadowMap,
	uniform bool bDebugFlag,
	uniform bool bSpotLight,
	uniform bool bSphericalHarmonics )
{
//	nPointLights = 1;
//	nPerPixelLights = 0;
//	bSkinned = false;
//	bNormalMap = false;
//	bSpecular = false;
//	bShadowMap = false;

	if ( bSpotLight )
	{
		bSpecular = false;
	}
	
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;

/*
	if ( bShadowMap && 0 )
	{
		// is vViewPos OK as half precision?
		float4 vViewPos  = mul( In.Position,	  WorldView );				// view position (in worldview)
		Out.ShadowMap1	 = mul( vViewPos,		  ViewToLightProjection );	// position (in light view)
		Out.ViewPosition = vViewPos;
		//Out.ViewPosition = vViewPos * 0.5f + 0.5f; // convert to a color
	}
*/
	
	float4 Position;
	half3 Normal;
	
	half3x3 objToTangentSpace = 0;
	if ( bSkinned )
	{
		if ( bNormalMap )
			GetPositionNormalAndObjectMatrix( Position, Normal, objToTangentSpace, In ); 
		else
			GetPositionAndNormal( Position, Normal, In );
	} else {
 		Position = float4( In.Position, 1.0f );

		half3 vNormal = ANIMATED_NORMAL;
		if ( bNormalMap )
		{
			objToTangentSpace[0] = ANIMATED_TANGENT;
			objToTangentSpace[1] = ANIMATED_BINORMAL;
			objToTangentSpace[2] = vNormal;
		}
		Normal = vNormal;
	}
	if ( bNormalMap )
		Out.Normal.xyz = mul( objToTangentSpace, Normal );
	else
		Out.Normal.xyz = Normal;

	for ( int i = 0; i < nPerPixelLights && i < nPointLights; i++ )
	{
		half3 LightVector = half3( PointLightsPos(OBJECT_SPACE)[ i ].xyz ) - half3( Position.xyz );
		Out.LightVector[ i ].w = GetFalloffBrightness ( LightVector, PointLightsFalloff( OBJECT_SPACE )[ i ] );
		
		if ( bNormalMap )
			Out.LightVector[ i ].xyz = mul( objToTangentSpace, half3( PointLightsPos(OBJECT_SPACE)[ i ].xyz ) );
		else
			Out.LightVector[ i ].xyz = PointLightsPos(OBJECT_SPACE)[ i ].xyz;
	}

	if ( bSpecular )
	{
		Out.ObjPosition	= Position;
		Out.EyePosition	= EyeInObject;
		if ( bNormalMap )
		{
			Out.ObjPosition = mul( objToTangentSpace, half3( Out.ObjPosition ) );
			Out.EyePosition = mul( objToTangentSpace, half3( Out.EyePosition ) );
		}
	} 

	if ( bSpotLight )
	{
		VS_PrepareSpotlight(
			Out.SpotVector.xyz,
			Out.DirLightVector.xyz,
			Out.SpotColor,
			half3( Position.xyz ),
			objToTangentSpace,
			bNormalMap,
			OBJECT_SPACE );

		if ( bNormalMap )
		{
			Out.ObjPosition			= mul( objToTangentSpace, half3( Out.ObjPosition ) );
			Out.EyePosition			= mul( objToTangentSpace, half3( Out.EyePosition ) );
		} 
	}
  
	Out.Position		= mul( Position, WorldViewProjection ); 
	Out.DiffuseMap		= In.DiffuseMap;                             
	
	//Out.Ambient			= CombinePointLightsPerVertex( Position, Normal, nPointLights, nPerPixelLights );
	Out.Ambient			= GetAmbientColor();
	if (bSphericalHarmonics) {
		half3 NormalWS = normalize( mul( Normal, half4x4( World ) ) );
		Out.Ambient.xyz += SHEval_9_RGB( NormalWS );
	}
	//Out.Ambient.xyz		= SHEval_4_RGB( Normal ) * 3;

	// directional light
	if ( ! bSpotLight )
	{
		Out.Ambient			+= GetDirectionalColor( Normal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );

		half4 L				= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ];

		// INTENTIONALLY using the diffuse vector for specular as well
		if ( bNormalMap )
			Out.DirLightVector.xyz = mul( objToTangentSpace, half3( L.xyz ) );
		else
			Out.DirLightVector.xyz = L.xyz;
		Out.DirLightVector.w = 1.0f;
	}

	Out.Ambient.xyz		= saturate( Out.Ambient.xyz );

	FOGMACRO( Position )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
void PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	out float4 OutColor : COLOR,	
	uniform int nPerPixelLights,
	uniform bool bNormalMap,
	uniform bool bSpecular,
	uniform bool bShadowMap,
	uniform bool bDebugFlag,
	uniform bool bSpotLight )
{
//	nPerPixelLights = 1;
//	bNormalMap = true;
//	bShadowMap = false;

	if ( bSpotLight )
	{
		bSpecular = false;
	}

	half2 vDiffuseMap	= In.DiffuseMap;

	half4 cDiffuse		= tex2D( DiffuseMapSampler, vDiffuseMap );
	half3 cFinalLight	= In.Ambient.xyz;
	half3 vNormal;
	
	cDiffuse.w			*= gfAlphaConstant;

	if ( bNormalMap )
	{
		//vNormal			= normalize( half(2.0f) * tex2D( NormalMapSampler, vDiffuseMap ).xyz - half(1.0f) );
		vNormal			= normalize( ReadNormalMap( vDiffuseMap ) );
	} else {
		vNormal			= normalize( half3( In.Normal.xyz ) );
	}

	half fLightingAmount = 1.0f;

	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	half4 vSpecularMap  = 0.0f;
	half  fSpecularPower;
	half3 vViewVector;

	if ( bSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap		= tex2D( SpecularMapSampler, vDiffuseMap );
		vSpecularMap.xyz	*= SpecularBrightness;
		fSpecularPower		= length( vSpecularMap.xyz );
		//fSpecularLevel		= lerp( SpecularBrightness.x, SpecularBrightness.y, vSpecularMap.w );
		vViewVector			= normalize( half3( In.EyePosition ) - half3( In.ObjPosition ) );
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

/*
		if ( bSpecular )
		{
			vHalfVector			= normalize( vViewVector + vLightVector );
			fDot				= saturate( dot( vHalfVector, vNormal ) );
			fPointSpecularAmt	= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, fDot ) );
			// attenuating point-light specular
			vPointSpecular		= vSpecularMap.xyz * ( fPointSpecularAmt * PSGetSpecularBrightness( vLightVector, vNormal ) * half( In.LightVector[ i ].w ) );
			fGlow				+= fPointSpecularAmt; // not attenuating this
			vOverBrighten		+= PointLightColor[ i ] * vPointSpecular;
		}
*/
	}


	// directional light
	if ( ! bSpotLight )
	{
		if ( bNormalMap )
			vLightVector = normalize( half3( In.DirLightVector.xyz ) );
		else
			vLightVector = In.DirLightVector.xyz;
		fLightBrightness	= PSGetDiffuseBrightnessNoBias( vLightVector, vNormal );
		vBrighten			+= fLightBrightness * DirLightsColor[ DIR_LIGHT_0_DIFFUSE ].rgb;
		if ( bSpecular )
		{
			half4 C				= DirLightsColor[ DIR_LIGHT_0_SPECULAR ];
		
			vHalfVector			= normalize( vViewVector + vLightVector );
			fDot				= saturate( dot( vHalfVector, vNormal ) );
			fPointSpecularAmt	= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, fDot ) );
			vPointSpecular		= fPointSpecularAmt * vSpecularMap.xyz * PSGetSpecularBrightness( vLightVector, vNormal );
			fGlow				+= fPointSpecularAmt; // * C.a; // attenuating this
			vOverBrighten		+= C.rgb * vPointSpecular;
		}
	}
	
	if ( bSpecular )
	{
		//vBrighten   += fGlow; // add the glow amount as a white light contribution
		fFinalAlpha += fGlow * SpecularGlow * fSpecularPower;
	}

	if ( bSpotLight )
	{
		vBrighten += PS_EvalSpotlight(
						   In.SpotVector.xyz,
						   In.DirLightVector.xyz,
						   In.SpotColor.rgb,
						   vNormal,
						   OBJECT_SPACE );
	}

	half3 cLightMap = SAMPLE_SELFILLUM( vDiffuseMap ) * gfSelfIlluminationPower;
	cFinalLight += cLightMap;
	fFinalAlpha += dot( cLightMap.xyz, cLightMap.xyz ) * gfSelfIlluminationGlow;

	if ( bShadowMap )
	{
		float fDebugShadowMin = 0.30f;
		cFinalLight += vBrighten * lerp( fDebugShadowMin, 1.0f, fLightingAmount );
	} else {
		cFinalLight += vBrighten;
	}

	cDiffuse.xyz *= cFinalLight;
	cDiffuse.xyz += vOverBrighten;
	cDiffuse.w   *= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( fFinalAlpha + GLOW_MIN ) * gfAlphaForLighting );

	OutColor = cDiffuse;	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUENAMED(name,points,specular,normal,skinned,shadowmap,bSphericalHarmonics, debugflag) \
	DECLARE_TECH name##points##specular##normal##skinned##shadowmap##bSphericalHarmonics##debugflag \
	< SHADER_VERSION_22 \
	  int PointLights = points; \
	  int Specular = specular; \
	  int Skinned = skinned; \
	  int NormalMap = normal; \
	  /*int SelfIllum = -1;*/ /* CHB 2007.03.20 - Commenting out, otherwise makes "cost" too expensive and only lame techniques are chosen. */ \
	  int ShadowMap = shadowmap; \
	  int DebugFlag = debugflag; \
	  int SpotLight = SPOTLIGHT_ENABLED; \
	  bool SphericalHarmonics = bSphericalHarmonics; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( points, min( points, 1 ), normal, specular, skinned, shadowmap, debugflag, SPOTLIGHT_ENABLED, bSphericalHarmonics ) ); \
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS20_ACTOR( min( points, 1 ), normal, specular, shadowmap, debugflag, SPOTLIGHT_ENABLED ) ); \
	}  } 

#define TECHNIQUE20(points,specular,normal,skinned,shadowmap,bSphericalHarmonics, debugflag) TECHNIQUENAMED(TVertexAndPixelShader20, points, specular, normal, skinned, shadowmap, bSphericalHarmonics, debugflag)

#ifndef PRIME
TECHNIQUENAMED(SimpleOneLight,1,0,0,0,0,0,0)
TECHNIQUENAMED(Everything,1,1,1,0,0,1,0)
#endif

#ifdef PRIME
#include "Dx9/_ActorHeader.fx"
#endif
