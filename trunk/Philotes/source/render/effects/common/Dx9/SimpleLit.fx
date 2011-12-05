//
// 
//

#include "_common.fx"

#pragma warning( disable : 3550 )		// array ref cannot be l-value


//--------------------------------------------------------------------------------------------
struct VS_SIMPLELIT_INPUT
{
    float4 Position		: POSITION; 
    float3 Normal		: NORMAL; 
    float3 Tangent  	: TANGENT;		
    float3 Binormal  	: BINORMAL0;		
    float3 TBCross		: BINORMAL1;		
    float2 DiffuseMap 	: TEXCOORD1; 
};

//--------------------------------------------------------------------------------------------
struct PS20_SIMPLELIT_INPUT
{
    float4 Position				: POSITION; 
    float2 DiffuseMap 			: TEXCOORD0; 
    float3 HalfVector[ 2 ]		: TEXCOORD1;
    float4 LightVector[ 2 ]		: TEXCOORD3; // brightness is in the w
    float3 Ambient				: TEXCOORD5; 
    float3 Normal				: TEXCOORD6;
    float Fog					: FOG;
};

PS20_SIMPLELIT_INPUT VS20_SIMPLELIT ( 
	VS_SIMPLELIT_INPUT In, 
	uniform int nPointLights, 
	uniform bool bSpecular,
	uniform bool bNormalMap )
{
//	nPointLights = 1;
//	nSpecularLights = 1;
	
	PS20_SIMPLELIT_INPUT Out = (PS20_SIMPLELIT_INPUT)0;

	Out.Position = mul( In.Position, WorldViewProjection );	

	if ( bSpecular && nPointLights > 0 )
	{
		// normalize these vectors
		half3 viewVector  = normalize( EyeInObject.xyz - In.Position.xyz );
		
		// compute the 3x3 tranform from tangent space to object space
		half3x3 objToTangentSpace;
		objToTangentSpace[0] = ( 2.0f * In.Tangent  ) - 1.0f;
		objToTangentSpace[1] = ( 2.0f * In.Binormal ) - 1.0f;
		objToTangentSpace[2] = In.Normal;
		
/*
		for ( int i = 0; i < nPointLights && i < 2; i++ ) {
			// compute the first light's brightness and position
			half3 LightVector = PointLightPosition[ i ].xyz - In.Position.xyz;
			Out.LightVector[ i ].w = GetFalloffBrightness( LightVector, PointLightFalloff[ i ] );
			LightVector = normalize( LightVector );
			
			// compute half-angle vectors
			half3 HalfVector = normalize(LightVector + viewVector);

			// transform vectors from object space to tangent space
			Out.HalfVector[ i ]			= mul(objToTangentSpace, HalfVector);
			Out.LightVector[ i ].xyz    = mul(objToTangentSpace, LightVector);
		}		
*/		

		if ( !bNormalMap )
		{
			Out.Normal = mul(objToTangentSpace, In.Normal);
		}
	}
	
	// the bump map, light map and the diffuse texture use the same texture coordinates
	Out.DiffuseMap	= In.DiffuseMap;

	Out.Ambient = CombinePointLights( In.Position, In.Normal, nPointLights, 2, OBJECT_SPACE );
	
	FOGMACRO( In.Position )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_SIMPLELIT( 
	PS20_SIMPLELIT_INPUT In,
	uniform int nSpecularLights,
	uniform bool bNormalMap ) : COLOR
{
//	bSpecular = true;
//	bNormalMap = true;
	//fetch light map
	half4 lightMap = SAMPLE_LIGHTMAP( In.DiffuseMap );

    //fetch base color
	half4 color = tex2D(DiffuseMapSampler, In.DiffuseMap);
	
	half3 finalLightColor = lightMap + In.Ambient;
	half  finalAlpha = 0;
	if ( nSpecularLights > 0 )
	{
		half3 Normal;
		//compute specular brightness
		if ( bNormalMap )
		{
			//fetch bump normal and expand it to [-1,1]
			Normal = ReadNormalMap( In.DiffuseMap );
		} else {
			Normal = normalize( In.Normal );
		}
		half SpecularMask = color.w;
		for ( int i = 0; i < nSpecularLights && i < 2; i++ )
		{
			//half Specular = GetSpecular20( normalize(In.HalfVector[ i ]), Normal ) * SpecularMask;
			half Specular = 0;
			half Diffuse = saturate( dot( Normal, normalize(In.LightVector[ i ].xyz) ) );

			finalLightColor += (Specular + Diffuse ) * PointLightColor[ i ] * In.LightVector[ i ].w;
			
			finalAlpha += Specular;
		}
	}
	
	color.xyz *= finalLightColor;
	color.w = finalAlpha + GLOW_MIN;
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE20(points,specular,normal) DECLARE_TECH TVertexAndPixelShader20##points##specular##normal < \
	SHADER_VERSION_22\
	int PointLights = points; \
	int SpecularLights = 0; \
	int Specular = specular; \
	int NormalMap = normal; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_SIMPLELIT( points, specular, normal )); \
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS20_SIMPLELIT( specular ? points : 0, normal )); \
		} } 

TECHNIQUE20( 0, 0, 0 )
TECHNIQUE20( 1, 0, 0 )
TECHNIQUE20( 2, 0, 0 )
TECHNIQUE20( 3, 0, 0 )

TECHNIQUE20( 0, 0, 1 )
TECHNIQUE20( 1, 0, 1 )
TECHNIQUE20( 2, 0, 1 )
TECHNIQUE20( 3, 0, 1 )

TECHNIQUE20( 0, 1, 0 )
TECHNIQUE20( 1, 1, 0 )
TECHNIQUE20( 2, 1, 0 )
TECHNIQUE20( 3, 1, 0 )

TECHNIQUE20( 0, 1, 1 )
TECHNIQUE20( 1, 1, 1 )
TECHNIQUE20( 2, 1, 1 )
TECHNIQUE20( 3, 1, 1 )

#include "Dx9/_NoPixelShader.fx"
