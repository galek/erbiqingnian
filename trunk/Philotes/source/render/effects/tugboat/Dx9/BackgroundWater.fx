//
// 
//

#include "_common.fx"
#include "Dx9/_ScrollUV.fx"

half	 gfFresnelPower = 0.5f;
half	 gfEnvMapBumpPower = 0.5f;

//--------------------------------------------------------------------------------------------
struct PS20_BACKGROUND_INPUT
{
    float4 Position				: POSITION; 
    float4 Ambient				: COLOR0;
    float4 SpecularColor		: COLOR1;		// could be a color (lose overbrighting...)
    float4 LightMapDiffuse 		: TEXCOORD0; 
    float3 Normal				: TEXCOORD1;	// might be unnecessary; could be rolled into LightDir?
    //float2 DiffuseMap2 			: TEXCOORD1; 
    float3 SpecularLightVector	: TEXCOORD2;
    float3 DirLightVector		: TEXCOORD3;
    float3 ObjPosition			: TEXCOORD4;
    float3 EyePosition			: TEXCOORD5;
	float3 EnvMapCoords			: TEXCOORD6;
//	float4 ShadowMapCoords		: TEXCOORD7; 
    float  Fog					: FOG;
};

PS20_BACKGROUND_INPUT VS20_WATER ( 
	VS_BACKGROUND_INPUT_64 In, 
	uniform int nPointLights, 
	uniform bool bSpecular,
	uniform bool bShadowMap )
{


	PS20_BACKGROUND_INPUT Out = (PS20_BACKGROUND_INPUT)0;

	Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection );	

	// compute the 3x3 transform from tangent space to object space
	half3x3 objToTangentSpace;
	half3 vNormal =  In.Normal;


	Out.Normal = vNormal;


	// environment map
	half3 WorldPos		 = mul( half3( In.Position.xyz ), half4x4( World ) );
	half3 vViewToPos	 = normalize( WorldPos - half3( EyeInWorld.xyz ) );
	Out.EnvMapCoords.xyz = normalize( reflect( vViewToPos, vNormal ) );


	if ( bSpecular )
	{
		half3 vLightPosition;
		half3 vLightVector;

		VS_PickAndPrepareSpecular(
			In.Position,
			vNormal,
			Out.SpecularColor,
			Out.SpecularLightVector,
			false,
			OBJECT_SPACE );

		//Out.SpecularLightVector = vLightPosition;
		Out.DirLightVector		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ].xyz;
		Out.ObjPosition			= In.Position.xyz;
		Out.EyePosition			= EyeInObject;

	}


	Out.LightMapDiffuse			= In.LightMapDiffuse;
	Out.Ambient = GetAmbientColor();

	Out.Ambient.xyz = saturate( Out.Ambient.xyz );
	Out.Ambient.w = 0.0f;

	half3 vInPos = In.Position;
	Out.Ambient.w		= 0.0f;

	
	{
		FOGMACRO_OUTPUT( Out.Ambient.w, vInPos )
		Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
		Out.Ambient.w = saturate( Out.Ambient.w );
	}

	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS2A_WATER( 
	PS20_BACKGROUND_INPUT In,
	uniform bool bSpecular,
	uniform bool bShadowMap ) : COLOR
{


	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

	// scroll coords
	float2 vDiffuseCoords = vIn_DiffuseMapCoords;//ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_DIFFUSE );
	float2 vNormalCoords  = vIn_DiffuseMapCoords;//ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_NORMAL );
	int frame = floor( gfScrollTextures[ SCROLL_CHANNEL_NORMAL ].fPhase * gfScrollTextures[ SCROLL_CHANNEL_NORMAL ].fTile * 20.0f );
	int yTile = floor( (float)frame / 8.0f );
	int xTile = frame - ( yTile * 8 );
	vNormalCoords.x += (float)xTile * .125f;
	vNormalCoords.y += (float)yTile * .125f;
	vDiffuseCoords.x += (float)xTile * .125f;
	vDiffuseCoords.y += (float)yTile * .125f;
	//vDiffuseCoords = vNormalCoords;

    //fetch base color
	half4	cDiffuse		= tex2D( DiffuseMapSampler, vDiffuseCoords );
	half3	cFinalLight		= In.Ambient + .5 * In.Ambient.w;
	half3	vNormal			= In.Normal;

	//cDiffuse.w				= tex2D( OpacityMapSampler, vIn_DiffuseMapCoords ).z;  // maps blue to alpha
	cDiffuse.a				= tex2D( DiffuseMapSampler, vDiffuseCoords.xy ).a + 1.0f - In.Ambient.w;
	cDiffuse.a				= saturate( cDiffuse.a );
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
	vLightVector = normalize( half3( In.DirLightVector.xyz ) );

	/*//fetch bump normal and expand it to [-1,1]
	//vNormal = normalize( UNBIAS( tex2D( NormalMapSampler, vNormalCoords ).xyz ) );
	vNormal = normalize( ReadNormalMap( vNormalCoords ).xyz );


	// fetch normal again using other coords, and average it with other normal
	//vNormal = vNormal + normalize( UNBIAS( tex2D( NormalMapSampler, vNormalCoords2 ).xyz ) );
	vNormal = vNormal + normalize( ReadNormalMap( vNormalCoords2 ).xyz );

	vNormal = normalize( vNormal );*/
	if ( bSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap	  = tex2D( SpecularMapSampler, vNormalCoords );
		fSpecularPower.x  = length( vSpecularMap.xyz );
		vViewVector		  = normalize( half3( In.EyePosition ) - half3( In.ObjPosition ) );
	}


/*	half4 vEnvMap;
	half4 vCoords;
	vCoords.xyz = normalize( half3( In.EnvMapCoords ) );

	half fFresnel = half(1) - dot( vNormal, vCoords.xyz );
	fFresnel *= gfFresnelPower;
	cDiffuse.w *= 1 - fFresnel;

	vCoords.xyz += vNormal * gfEnvMapBumpPower;

	vCoords.w   = gfEnvironmentMapLODBias;
	vEnvMap.xyz = texCUBEbias( CubeEnvironmentMapSampler, vCoords );
	vEnvMap.w   = gfEnvironmentMapPower * fFresnel;
*/
	if ( bSpecular )
	{
		//vEnvMap.w    *= fSpecularPower;
/*
		if ( gbEnvironmentMapInvertGlossThreshold )
		{
			if ( vSpecularMap.w < gfEnvironmentMapGlossThreshold )
			{
				vEnvMap.w = 0.0f;
			}
		}
		else
		{
			if ( vSpecularMap.w > gfEnvironmentMapGlossThreshold )
			{
				vEnvMap.w = 0.0f;
			}
		}
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
		fGlow				+= fPointSpecularAmt * C.a; // attenuating this
		vOverBrighten		+= C.rgb * vPointSpecular.rgb * C.a;

		// specular light
		
		vLightVector		= normalize( half3( In.SpecularLightVector.xyz ) - half3( In.ObjPosition ) );
		fLightBrightness	= PSGetSpecularBrightness( vLightVector, vNormal );
		//vBrighten			+= fLightBrightness * In.SpecularColor;

		vHalfVector			= normalize( vViewVector + vLightVector );
		fDot				= saturate( dot( vHalfVector, vNormal ) );
		fPointSpecularAmt	= tex2D( SpecularLookupSampler, float2( vSpecularMap.w, fDot ) ).x;
		vPointSpecular		= fPointSpecularAmt * vSpecularMap.xyz * fLightBrightness;
		fGlow				+= fPointSpecularAmt; // not attenuating this
		vOverBrighten		+= In.SpecularColor.xyz * vPointSpecular.xyz; // * In.LightVector[ i ].w;

		fGlow		*= SpecularGlow * fSpecularPower;
		//vBrighten   += fGlow; // add the glow amount as a white light contribution
		fFinalAlpha += fGlow;  // currently ignored and compiled out
	}

	half3 cLightMap = tex2D( LightMapSampler, vIn_LightMapCoords );
	cFinalLight		*= cLightMap;
	
	cDiffuse.xyz *= cFinalLight.xyz;

	//cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );

	cDiffuse.xyz += vOverBrighten;

	// ignore alpha for glow -- use for opacity instead
	//cDiffuse.w   *= fFinalAlpha + GLOW_MIN;
	APPLY_FOG( cDiffuse.xyz, In.Ambient.w );
	return cDiffuse;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE30(suffix, points,specular,shadowmap,debugflag) \
    DECLARE_TECH TVertexAndPixelShader20_##suffix < \
	SHADER_VERSION_33 \
	int PointLights = points; \
	int Specular = specular; \
	int Skinned = 0; \
	int LightMap = 0; \
	int NormalMap = 1; \
	int DiffuseMap2 = 0; \
	int ShadowMap = shadowmap; \
	int DebugFlag = debugflag; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_3_0, VS20_WATER( points, specular, shadowmap )); \
		COMPILE_SET_PIXEL_SHADER ( ps_3_0, PS2A_WATER( specular, shadowmap )); \
		} } 

#ifdef PRIME
//#include "_BackgroundHeader.fx"
TECHNIQUE30(a, 0, 0, 0, 0 )

TECHNIQUE30(b, 0, 0, 1, 0 )

TECHNIQUE30(c, 0, 1, 0, 0 )

TECHNIQUE30(d, 0, 1, 1, 0 )

#include "dx9/_NoPixelShader.fx"
#endif