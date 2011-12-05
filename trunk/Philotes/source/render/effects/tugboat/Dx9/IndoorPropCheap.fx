//
// 
//

#include "_common.fx"
#include "Dx9/_SphericalHarmonics.fx"

//--------------------------------------------------------------------------------------------
struct PS20_BACKGROUND_INPUT
{
    float4 Position				: POSITION; 
    float4 LightMapDiffuse 		: TEXCOORD0; 
    float4 Ambient				: COLOR0; 
    float4 Overbright			: COLOR1;
    float2 DiffuseMap2 			: TEXCOORD1; 
    float3 Normal				: TEXCOORD2;
    float3 SpotVector			: TEXCOORD3;
    float4 SpotColor			: TEXCOORD4;
    float  Fog					: FOG;
};

PS20_BACKGROUND_INPUT VS20_BACKGROUND ( 
	VS_BACKGROUND_INPUT_DIFF2_32 In, 
	uniform int nPointLights,
	uniform bool bDiffuseMap2,
	uniform bool bLightMap,
	uniform bool bSpecular )
{

	PS20_BACKGROUND_INPUT Out = (PS20_BACKGROUND_INPUT)0;

	Out.Position = mul( In.Position, WorldViewProjection );	

	if ( bDiffuseMap2 )
		Out.DiffuseMap2 = In.DiffuseMap2;
	Out.LightMapDiffuse = In.LightMapDiffuse;
	
	half3 vInNormal = RIGID_NORMAL;

	Out.Ambient.xyz		=  CombinePointLights( half4( In.Position ), vInNormal, nPointLights, 0, OBJECT_SPACE );
	Out.Ambient.xyz		+= SHEval_9_RGB( vInNormal );
	
	Out.Ambient.xyz += GetDirectionalColor( vInNormal, DIR_LIGHT_0_DIFFUSE, OBJECT_SPACE );
	Out.Ambient.xyz	+= GetDirectionalColor( vInNormal, DIR_LIGHT_1_DIFFUSE, OBJECT_SPACE );

	Out.Ambient.xyz		+= GetAmbientColor();	// should probably be combined into SH contribution
	Out.Ambient.xyz		= saturate( Out.Ambient.xyz );
	Out.Ambient.w		=  0.0f;
	Out.Overbright		= 0;

	if ( bSpecular )
	{
		half fSpecularLevel		= SpecularBrightness * SpecularMaskOverride;  // this isn't the best way, but it's probably the best right now
		half fGlossiness		= lerp( SpecularGlossiness.x, SpecularGlossiness.y, SpecularMaskOverride );

		half3 vViewVector		= normalize( half3( EyeInObject.xyz ) - half3( In.Position.xyz ) );
		half3 vLightVector		= half3( DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_SPECULAR ].xyz );
		half3 vHalfVector		= normalize( vViewVector + vLightVector );
		half fPointSpecular		= GetSpecular20PowNoNormalize( vHalfVector, vInNormal, fGlossiness );
		half fLightBrightness	= PSGetSpecularBrightness( vLightVector, vInNormal );
		fPointSpecular			*= fLightBrightness * fSpecularLevel;
		half fGlow				= fPointSpecular * DirLightsColor[ 0 ].w; // attenuating this
		half3 vBrighten			= DirLightsColor[ DIR_LIGHT_0_SPECULAR ].xyz * fPointSpecular * DirLightsColor[ DIR_LIGHT_0_SPECULAR ].w;
		Out.Overbright			+= half4( vBrighten, fGlow * SpecularGlow );
	}

	half3 vInPos = In.Position;
	FOGMACRO( vInPos )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS20_BACKGROUND( 
	PS20_BACKGROUND_INPUT In,
	uniform int nPointLights,
	uniform bool bDiffuseMap2,
	uniform bool bLightMap ) : COLOR
{
	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

	//fetch base color
	half4 color = tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords );

	if ( bDiffuseMap2 )
	{
		half4 diffMap2 = tex2D( DiffuseMapSampler2, half2( In.DiffuseMap2.xy ) );
		color.xyz = lerp( color.xyz, diffMap2.xyz, diffMap2.w );
	}

	half4 finalLightColor = In.Ambient;

	if ( bLightMap ) 
	{
		//fetch light map
		half4 lightMap = SAMPLE_LIGHTMAP( vIn_LightMapCoords);
		finalLightColor += lightMap;
	}
	
	//compute final color
	//if ( nPointLights != 0 )
	//	finalLightColor += In.Ambient;

	color.xyz *= finalLightColor; 
	color.xyz += half3( In.Overbright.xyz );
	color.w   *= half( In.Ambient.w ) + half( In.Overbright.w ) + GLOW_MIN;

	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE20(points, diffusemap2, lightmap, specular, debugflag ) \
DECLARE_TECH TVertexAndPixelShader20##points##diffusemap2##lightmap##specular < \
	SHADER_VERSION_22 \
	int PointLights = points; \
	int SpecularLights = 0; \
	int Specular = specular; \
	int NormalMap = 0; \
	int DiffuseMap2 = diffusemap2; \
	int LightMap = lightmap; \
	int DebugFlag = debugflag; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_BACKGROUND( points, diffusemap2, lightmap, specular )); \
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS20_BACKGROUND( points, diffusemap2, lightmap )); \
 		} } 

#include "Dx9/_BackgroundHeaderCheap.fx"

#include "Dx9/_NoPixelShader.fx"
