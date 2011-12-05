//
// ACtor Shader - it is the general purpose shader for non-background objects
//
// transformations
#include "_common.fx"
#include "_AnimatedShared.fx"

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
    float2 DiffuseMap			: TEXCOORD0;
    float3 Ambient				: COLOR0;	 // used to be a texcoord to help with overbrightening
    float3 Normal				: TEXCOORD1;
    float4 LightVector[ 3 ]		: TEXCOORD2; // w has brightness
    float3 LightHalfAngle[ 3 ]	: TEXCOORD5;
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
	uniform bool bDebugFlag )
{
//	nPointLights = 1;
//	nPerPixelLights = 0;
//	bSkinned = false;
//	bNormalMap = false;
//	bSpecular = false;
	
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;
	
	float4 Position;
	half3 Normal;
	
	half3x3 objToTangentSpace; 
	if ( bSkinned )
	{
		GetPositionNormalAndObjectMatrix( Position, Normal, objToTangentSpace, In ); 
	} else {
		Normal = In.Normal;
 		Position = In.Position;
		objToTangentSpace[0] = ( 2.0f * In.Tangent  ) - 1.0f;
		objToTangentSpace[1] = ( 2.0f * In.Binormal ) - 1.0f;
		objToTangentSpace[2] = In.Normal;
		if ( ! bNormalMap )
		{
			Out.Normal = mul( objToTangentSpace, Normal );
		}
	}

	half3 ViewVector = normalize( EyeInObject.xyz - Position );

	for ( int i = 0; i < nPerPixelLights && i < nPointLights; i++ )
	{
		float3 LightVector = PointLightInObject[ i ] - Position;
		Out.LightVector[ i ].w = GetBrightness ( LightVector, PointLightFalloff[ i ] );
		
		LightVector				= normalize( LightVector );
		Out.LightVector[ i ].xyz= mul( objToTangentSpace, LightVector );
	    
		if ( bSpecular )
		{
			Out.LightHalfAngle[ i ].xyz = normalize( LightVector + ViewVector );
			if ( bNormalMap )
				Out.LightHalfAngle[ i ].xyz = mul( objToTangentSpace, Out.LightHalfAngle[ i ].xyz );
		} 
	}
  
	Out.Position		= mul( Position, WorldViewProjection ); 
	Out.DiffuseMap		= In.DiffuseMap;                             
	Out.Ambient			= CombinePointLightsPerVertex( Position, Normal, nPointLights, nPerPixelLights );
	Out.Ambient			+= GetAmbientColor();

/*
	// directional light	
	if ( bNormalMap )
		Out.LightVector[ 1 ].xyz = mul( objToTangentSpace, LightDirectional[ 0 ] );
	else
		Out.LightVector[ 1 ].xyz = LightDirectional[ 0 ];
	Out.LightVector[ 1 ].w = 1.0f;
	if ( bSpecular )
	{
		Out.LightHalfAngle[ 1 ].xyz = normalize( LightDirectional[ 0 ] + ViewVector );
		if ( bNormalMap )
			Out.LightHalfAngle[ 1 ].xyz = mul( objToTangentSpace, Out.LightHalfAngle[ 1 ].xyz );
	} 
*/

	FOGMACRO( Position )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	uniform int nPerPixelLights,
	uniform bool bNormalMap,
	uniform bool bSpecular,
	uniform bool bLightMap,
	uniform bool bOpacityMap,
	uniform bool bDebugFlag ) : COLOR
{
//	nPerPixelLights = 1;
//	bNormalMap = true;
//	bShadowMap = false;

	half4 Color		= tex2D( DiffuseMapSampler, In.DiffuseMap );
	
	if ( bOpacityMap )
		Color.w		= tex2D( OpacityMapSampler, In.DiffuseMap ).z;  // maps blue to alpha
	else
		Color.w		= 1.0f;

	half3 vNormal;
	if ( bNormalMap )
	{
		vNormal = 2.0f * tex2D( NormalMapSampler, In.DiffuseMap ) - 1.0f;
	} else {
		vNormal = normalize( In.Normal );
	}

	half lightingAmount = 1.0f;

	half3 FinalLightColor = In.Ambient.xyz;
	half  fGlow = 0;
	
	half4 vSpecularMap;
	half Glossiness;
	if ( bSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap	  = tex2D( SpecularMapSampler, In.DiffuseMap );
		vSpecularMap.xyz  *= SpecularBrightness;
		Glossiness		  = lerp( SpecularGlossiness.x, SpecularGlossiness.y, vSpecularMap.w );
	}
	half3 vBrighten	= 0;
	half3 vOverBrighten = 0;
	half fFinalAlpha = 0;

	for( int i = 0; i < nPerPixelLights; i++ )
	{ 
		half LightBrightness = PSGetDiffuseBrightness( In.LightVector[ i ], vNormal );
		if ( bSpecular )
		{
			// Need to create another solution for specular mapping
			half fPointSpecularAmt	= GetSpecular20PowNoNormalize ( normalize( In.LightHalfAngle[ i ] ), vNormal, Glossiness );
			half3 vPointSpecular	= vSpecularMap.xyz * fPointSpecularAmt * LightBrightness;
			fGlow					+= fPointSpecularAmt; // not attenuating this
			vOverBrighten			+= PointLightColor[ i ] * vPointSpecular * In.LightVector[ i ].w;
		}
		vBrighten += LightBrightness * PointLightColor[ i ];
	}
	if ( bSpecular )
	{
		//vBrighten += Glow; // add the glow amount as a white light contribution
		fFinalAlpha += fGlow * SpecularGlow;
	}

	if ( bLightMap )
	{
		half3 vLightMap = tex2D( LightMapSampler, In.DiffuseMap );
		FinalLightColor += vLightMap;
		fFinalAlpha += dot( vLightMap.xyz, vLightMap.xyz ) * LightmapGlow;
	}

	FinalLightColor += vBrighten;


	//cDiffuse.xyz *= cFinalLight;
	Color.xyz		*= FinalLightColor; 
	Color.xyz		+= vOverBrighten;
	
	// fake glow via a glow-color-add here
	Color.xyz		*= 1.0f + ( saturate( fFinalAlpha ) * 0.5f );
	
	Color.w = 1.0f;
	
	return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//NVTL:
#define TECHNIQUE20(points,specular,normal,skinned,lightmap,opacitymap,debugflag) \
	DECLARE_TECH TVertexAndPixelShader20 \
	< SHADER_VERSION_22\
	int ShaderGroup = SHADER_GROUP_20; \
	int PointLights = points; \
	int Specular = specular; \
	int Skinned = skinned; \
	int NormalMap = normal; \
	int LightMap = lightmap; \
	int OpacityMap = opacitymap; \
	int DebugFlag = debugflag; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( points, min( points, 3 ), normal, specular, skinned, debugflag )); \
		COMPILE_SET_PIXEL_SHADER( PIXEL_SHADER_2_VERSION, PS20_ACTOR( min( points, 3 ), normal, specular, lightmap, opacitymap, debugflag )); \
		}  } 

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
//NVTL:
#define TECHNIQUENAMED(name,points,specular,normal,skinned,lightmap,opacitymap,debugflag) \
	DECLARE_TECH name \
	< SHADER_VERSION_22\
	int ShaderGroup = SHADER_GROUP_20; \
	int PointLights = points; \
	int Specular = specular; \
	int Skinned = skinned; \
	int NormalMap = normal; \
	int LightMap = lightmap; \
	int OpacityMap = opacitymap; \
	int DebugFlag = debugflag; > { \
	pass P0 { \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_ACTOR( points, min( points, 1 ), normal, specular, skinned, debugflag )); \
		COMPILE_SET_PIXEL_SHADER( PIXEL_SHADER_2_VERSION, PS20_ACTOR( min( points, 1 ), normal, specular, lightmap, opacitymap, debugflag )); \
		}  } 

#ifndef PRIME
TECHNIQUENAMED(SimpleOneLight,1,0,0,0,0,0,0)
TECHNIQUENAMED(Everything,1,1,1,0,1,1,0)
#endif

#ifdef PRIME
#include "_ActorInventoryHeader.fx"
#endif