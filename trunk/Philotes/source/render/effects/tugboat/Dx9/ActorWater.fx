//
// ActorIndoor Shader - it is the general purpose shader for non-background objects (indoors)
//
// transformations
#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"
#include "dx9/_SphericalHarmonics.fx"
#include "dx9/_AnimatedShared20.fxh"
#include "dx9/_ScrollUV.fx"
half	 gfFresnelPower = 0.5f;
half	 gfEnvMapBumpPower = 0.5f;
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
    float2 DiffuseMap			: TEXCOORD0;
    float3 Normal				: TEXCOORD1;
    //float3 ViewPosition			: TEXCOORD2; 
    float4 CubeEnvMap 			: TEXCOORD2; 
    float4 LightVector[ 1 ]		: TEXCOORD3; // w has brightness // array for code compatibility
    float4 DirLightVector		: TEXCOORD4; // w has brightness
    float3 ObjPosition			: TEXCOORD5;
    float3 EyePosition			: TEXCOORD6;
    float3 EnvMapCoords			: TEXCOORD7;
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
	uniform bool bCubeEnvironmentMap,
	uniform bool bDebugFlag ,
	uniform bool bSphericalHarmonics )
{
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;
	
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
		Out.Normal = mul( objToTangentSpace, Normal );
	else
		Out.Normal = Normal;
	//if ( bCubeEnvironmentMap )
	{
		// what about sending vViewToPos down as COLOR1, packed?
		half3 vViewToPos = normalize( Position - EyeInObject );
		Out.EnvMapCoords = normalize( reflect( vViewToPos, Normal ) );
	}
	/*
	for ( int i = 0; i < nPerPixelLights && i < nPointLights; i++ )
	{
		half3 LightVector = half3( PointLightPosition[ i ].xyz ) - half3( Position.xyz );
		Out.LightVector[ i ].w = GetFalloffBrightness ( LightVector, PointLightFalloff[ i ] );
		
		if ( bNormalMap )
			Out.LightVector[ i ].xyz = mul( objToTangentSpace, half3( PointLightPosition[ i ].xyz ) );
		else
			Out.LightVector[ i ].xyz = PointLightPosition[ i ];
	}*/

	if ( bSpecular )
	{
		Out.ObjPosition = Position;
		Out.EyePosition = EyeInObject;
		if ( bNormalMap )
		{
			Out.ObjPosition = mul( objToTangentSpace, half3( Out.ObjPosition ) );
			Out.EyePosition = mul( objToTangentSpace, half3( Out.EyePosition ) );
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
	//if ( ! bSpotLight )
	{
		Out.Ambient		+= GetDirectionalColor( Normal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );

		half4 L			= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ];

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
float4 PS20_ACTOR ( 
	PS20_ACTOR_INPUT In,
	uniform int nPerPixelLights,
	uniform bool bNormalMap,
	uniform bool bSpecular,
	uniform bool bCubeEnvironmentMap,
	uniform bool bDebugFlag ) : COLOR
{
	half2 vIn_LightMapCoords = In.DiffuseMap.xy;
	half2 vIn_DiffuseMapCoords =In.DiffuseMap.xy;

	// scroll coords
	float2 vDiffuseCoords = ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_DIFFUSE );
	float2 vNormalCoords  = ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_NORMAL );
	float2 vNormalCoords2 = ScrollTextureCoordsCustom( vIn_DiffuseMapCoords, SCROLL_CHANNEL_NORMAL, SCROLL_CHANNEL_DIFFUSE );

    //fetch base color
	half4	cDiffuse		= tex2D( DiffuseMapSampler, vDiffuseCoords );
	half3	cFinalLight		= In.Ambient;
	half3	vNormal;

	//cDiffuse.w				= tex2D( OpacityMapSampler, vIn_DiffuseMapCoords ).z;  // maps blue to alpha
	cDiffuse.a				= tex2D( DiffuseMapSampler, vDiffuseCoords.xy ).a;

	half fLightingAmount = 1.0f;
			
	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	half4 vSpecularMap  = 0.0f;
	half  fSpecularPower;
	half3 vViewVector;

	half3 vLightVector;
	half3 vHalfVector;
	half  fDot;
	half  fLightBrightness;
	half  fPointSpecularAmt;
	half3 vPointSpecular;


	//fetch bump normal and expand it to [-1,1]
	//vNormal = normalize( UNBIAS( tex2D( NormalMapSampler, vNormalCoords ).xyz ) );
	vNormal	= normalize( ReadNormalMap( vNormalCoords ) );

	vLightVector = normalize( In.DirLightVector.xyz );


	// fetch normal again using other coords, and average it with other normal
	//vNormal = vNormal + normalize( UNBIAS( tex2D( NormalMapSampler, vNormalCoords2 ).xyz ) );
	vNormal	= vNormal + normalize( ReadNormalMap( vNormalCoords2 ) );
	vNormal = normalize( vNormal );

	if ( bSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap	  = tex2D( SpecularMapSampler, vNormalCoords );
		fSpecularPower.x  = length( vSpecularMap.xyz );
		vViewVector		  =  normalize( half3( In.EyePosition ) - half3( In.ObjPosition ) );
	}


/*
	half4 vEnvMap;
*/
	half4 vCoords;
	vCoords.xyz = normalize( half3( In.EnvMapCoords ) );

	half fFresnel = half(1) - dot( vNormal, vCoords.xyz );
	fFresnel *= gfFresnelPower;
	cDiffuse.w *= 1 - fFresnel;

/*
	vCoords.xyz += vNormal * gfEnvMapBumpPower;

	vCoords.w   = gfEnvironmentMapLODBias;
	vEnvMap.xyz = texCUBEbias( CubeEnvironmentMapSampler, vCoords );
	vEnvMap.w   = gfEnvironmentMapPower * fFresnel;
*/

	if ( bSpecular )
	{
/*
		vEnvMap.w    *= fSpecularPower;
*/

		vSpecularMap.xyz		*= half( SpecularBrightness );
		vSpecularMap.xyz		+= half( SpecularMaskOverride );  // should be 0.0f unless there is no specular map
		//fSpecularLevel		= lerp( SpecularBrightness.x, SpecularBrightness.y, vSpecularMap );

		// directional light
		half4 C				= DirLightsColor[ DIR_LIGHT_0_SPECULAR ];
	
		fLightBrightness	= PSGetSpecularBrightness( vLightVector, vNormal );
		vHalfVector			= normalize( vViewVector + vLightVector );
		fDot				= saturate( dot( vHalfVector, vNormal ) );
		fPointSpecularAmt	= tex2D( SpecularLookupSampler, float2( vSpecularMap.w, fDot ) ).x;
		vPointSpecular		= fPointSpecularAmt * vSpecularMap.xyz * fLightBrightness;
		fGlow				+= fPointSpecularAmt * C.w; // attenuating this
		vOverBrighten		+= C.xyz * vPointSpecular.xyz * C.w;


		fGlow		*= SpecularGlow * fSpecularPower;
		fFinalAlpha += fGlow;  // currently ignored and compiled out
	}


	cDiffuse.xyz *= cFinalLight.xyz;

/*
	cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );
*/

	cDiffuse.xyz += vOverBrighten;

	// ignore alpha for glow -- use for opacity instead
	//cDiffuse.w   *= fFinalAlpha + GLOW_MIN;

	return cDiffuse;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/*#define TECHNIQUE20(points,specular,normal,skinned,cubeenvmap,debugflag) \
	technique TVertexAndPixelShader20 \
	< int VSVersion = VS_2_0; int PSVersion = PS_2_0; \
	int PointLights = points; \
	int Specular = specular; \
	int Skinned = skinned; \
	int NormalMap = normal; \
	int LightMap = -1; \
	int CubeEnvironmentMap = cubeenvmap; \
	int DebugFlag = debugflag; > { \
	pass P0 { \
		VertexShader = compile vs_2_0 VS20_ACTOR( points, min( points, 1 ), normal, specular, skinned, cubeenvmap, debugflag ); \
		PixelShader  = compile ps_2_0 PS20_ACTOR( min( points, 1 ), normal, specular, cubeenvmap, debugflag ); \
		}  } 
*/
//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques for tools like 3dsMax or FXComposer
//////////////////////////////////////////////////////////////////////////////////////////////
#define TECHNIQUENAMED(name,points,specular,normal,skinned,cubeenvmap,bSphericalHarmonics,debugflag) \
	technique name \
	< int VSVersion = VS_2_0; int PSVersion = PS_2_0; \
	  int PointLights = points; \
	  int Specular = specular; \
	  int Skinned = skinned; \
	  int NormalMap = normal; \
	  int LightMap = -1; \
	  int CubeEnvironmentMap = cubeenvmap; \
	  int DebugFlag = debugflag; > { \
	pass P0 { \
		VertexShader = compile vs_2_0 VS20_ACTOR( points, min( points, 1 ), normal, specular, skinned, cubeenvmap, debugflag, bSphericalHarmonics ); \
		PixelShader  = compile ps_2_0 PS20_ACTOR( min( points, 1 ), normal, specular, cubeenvmap, debugflag ); \
	}  } 

#define TECHNIQUE20(points,specular,normal,skinned,cubeenvma,bSphericalHarmonics, debugflag) TECHNIQUENAMED(TVertexAndPixelShader20, points, specular, normal, skinned, cubeenvma, bSphericalHarmonics, debugflag)
#ifndef PRIME
TECHNIQUENAMED(SimpleOneLight,1,0,0,0,0,0,0)
TECHNIQUENAMED(Everything,1,1,1,0,0,1,0)
#endif

#ifdef PRIME
#include "dx9/_ActorHeader.fx"
#endif