//
// 
//

#ifndef SPOTLIGHT_ENABLED
#define SPOTLIGHT_ENABLED	0
#endif

#include "_common.fx"
#include "Dx9/_ScrollUV.fx"

half	 gfFresnelPower = 0.5f;
half	 gfEnvMapBumpPower = 0.5f;

//--------------------------------------------------------------------------------------------
struct PS20_BACKGROUND_INPUT
{
    float4 Position				: POSITION; 
    float4 Ambient				: COLOR0;
    float4 SC					: COLOR1;		// Specular color
    float4 LightMapDiffuse 		: TEXCOORD0; 
    float3 Nw					: TEXCOORD1;	// Normal in world space
    float3 SLt					: TEXCOORD2;	// Specular light vector in tangent space
    float3 Spt					: TEXCOORD3;	// Secondary spotlight vector
    float3 Pt					: TEXCOORD4;	// Position in tangent space
    float3 Et					: TEXCOORD5;	// Eye in tangent space
    float3 Pw					: TEXCOORD6;	// Position in world space
	//float3 EnvMapCoords			: TEXCOORD6;
    float  Fog					: FOG;
};

PS20_BACKGROUND_INPUT VS20_WATER ( 
	VS_BACKGROUND_INPUT_64 In, 
	uniform int nPointLights, 
	uniform bool bSpecular,
	uniform bool bSpotLight )
{
	if ( bSpotLight )
		bSpecular = false;

	PS20_BACKGROUND_INPUT Out = (PS20_BACKGROUND_INPUT)0;

	Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection );	

	half3 No;
	half3x3 o2tMat;
	VS_GetRigidNormalObjToTan( In.Normal.xyz, In.Tangent.xyz, In.Binormal.xyz, o2tMat, No, true );
	half3 Nw = mul( No, World );
	Out.Nw = Nw;

	// environment map
	half3 Pw			 = mul( half4( In.Position.xyz, 1.f ), half4x4( World ) ).xyz;
	//half3 Vw			 = normalize( Pw - half3( EyeInWorld.xyz ) );
	//Out.EnvMapCoords.xyz = normalize( reflect( Nw, Vw ) );
	Out.Pw.xyz			 = Pw;


	if ( bSpecular )
	{
		VS_PickAndPrepareSpecular( In.Position.xyz, No, Out.SC, Out.SLt, false, OBJECT_SPACE );

		Out.SLt			= mul( o2tMat, half3( Out.SLt ) );
		Out.Pt			= mul( o2tMat, half3( In.Position.xyz ) );
		Out.Et			= mul( o2tMat, half3( EyeInObject.xyz ) );
	}

	if ( bSpotLight )
	{
		VS_PrepareSpotlight(
			Out.SLt,
			Out.Spt,
			Out.SC,
			half3( In.Position.xyz ),
			o2tMat,
			true,
			OBJECT_SPACE );

		{
			//Out.ObjPosition			= mul( objToTangentSpace, half3( Out.ObjPosition ) );
			//Out.EyePosition			= mul( objToTangentSpace, half3( Out.EyePosition ) );
		} 
	}

	Out.LightMapDiffuse			= In.LightMapDiffuse;

	Out.Ambient = CombinePointLights( In.Position, No, nPointLights, 0, OBJECT_SPACE );
	Out.Ambient += GetAmbientColor();
	Out.Ambient.xyz = MDR_Pack( saturate( Out.Ambient.xyz ) );
	Out.Ambient.w = 0.0f;

	half3 vInPos = In.Position;
	FOGMACRO( vInPos )
	
	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS2A_WATER( 
	PS20_BACKGROUND_INPUT In,
	uniform bool bSpecular,
	uniform bool bSpotLight ) : COLOR
{
	if ( bSpotLight )
		bSpecular = false;

	half2 vIn_LightMapCoords = In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords = In.LightMapDiffuse.zw;

	// scroll coords
	float2 vDiffuseCoords = ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_DIFFUSE );
	float2 vNormalCoords  = ScrollTextureCoords( vIn_DiffuseMapCoords, SCROLL_CHANNEL_NORMAL );
	float2 vNormalCoords2 = ScrollTextureCoordsCustom( vIn_DiffuseMapCoords, SCROLL_CHANNEL_NORMAL, SCROLL_CHANNEL_DIFFUSE );

    //fetch base color
	half4	cDiffuse		= tex2D( DiffuseMapSampler, vDiffuseCoords );
	half3	cFinalLight		= MDR_Unpack( In.Ambient.rgb );
	half3	Nw				= normalize( half3(In.Nw) );
	half3	Nt;

	cDiffuse.a				= tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords.xy ).a;  // maps blue to alpha

	half fLightingAmount = 1.0f;


			
	half  fFinalAlpha	= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	half4 vSpecularMap  = 0.0f;
	half3 Vt			= 0;


	//fetch bump normal and expand it to [-1,1]
	Nt = ReadNormalMap( vNormalCoords ).xyz;

	// fetch normal again using other coords, and average it with other normal
	Nt = Nt + ReadNormalMap( vNormalCoords2 ).xyz;
	Nt = normalize( Nt );


	if ( bSpecular )
	{
		// calculate specular shininess and level
		vSpecularMap	  = tex2D( SpecularMapSampler, vDiffuseCoords );
		Vt				  = normalize( half3( In.Et ) - half3( In.Pt ) );
	}


	half4 vEnvMap;
	half4 vCoords;

	half3 Pw			= In.Pw.xyz;
	half3 Vw			= normalize( Pw - half3( EyeInWorld.xyz ) );
	vCoords.xyz			= normalize( reflect( Nw, Vw ) );
	//vCoords.xyz = normalize( half3( In.EnvMapCoords ) );


	half fFresnel = half(1) - dot( Nw, vCoords.xyz );
	fFresnel *= gfFresnelPower;
	cDiffuse.w *= 1 - fFresnel;

	vCoords.xyz += Nt * gfEnvMapBumpPower;

	vCoords.w   = gfEnvironmentMapLODBias;
	vEnvMap.xyz = texCUBEbias( CubeEnvironmentMapSampler, vCoords );
	vEnvMap.w   = gfEnvironmentMapPower * fFresnel;

	if ( bSpecular )
	{
		//vEnvMap.w    *= fSpecularPower;

		vSpecularMap.xyz		*= half( SpecularBrightness );
		vSpecularMap.xyz		+= half( SpecularMaskOverride );  // should be 0.0f unless there is no specular map

		half fUnused;		// glow isn't used
		half3 Lt			= normalize( half3( In.SLt.xyz ) );
		PS_EvaluateSpecular( Vt, Lt, Nt, In.SC, vSpecularMap, SpecularGlossiness.x, fUnused, vOverBrighten, false, false );
	}

	if ( bSpotLight )
	{
		vBrighten += PS_EvalSpotlight(
						   half3( In.SLt.xyz ),
						   half3( In.Spt.xyz ),
						   half3( In.SC.rgb ),
						   Nt,
						   OBJECT_SPACE );
	}

	half3 cLightMap = SAMPLE_LIGHTMAP( vIn_LightMapCoords );
	cFinalLight += cLightMap + vBrighten;

	cDiffuse.xyz *= cFinalLight.xyz;
	cDiffuse.xyz = LDR_Clamp( cDiffuse.xyz );

	cDiffuse.xyz  = lerp( cDiffuse.xyz, vEnvMap.xyz, saturate( vEnvMap.w ) );

	cDiffuse.xyz += vOverBrighten;

	//cDiffuse.xyz = Vt;

	return cDiffuse;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE30(suffix, points,specular,shadowmap,debugflag) \
    DECLARE_TECH TVertexAndPixelShader2b_##suffix < \
	SHADER_VERSION_22 \
	int PointLights = points; \
	int Specular = specular; \
	int Skinned = 0; \
	int LightMap = 0; \
	int NormalMap = 1; \
	int DiffuseMap2 = 0; \
	int ShadowMap = shadowmap; \
	int DebugFlag = debugflag; \
	int SpotLight = SPOTLIGHT_ENABLED; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_WATER( points, specular, SPOTLIGHT_ENABLED )); \
		COMPILE_SET_PIXEL_SHADER ( ps_2_b, PS2A_WATER( specular, SPOTLIGHT_ENABLED )); \
		} } 

#ifdef PRIME
//#include "_BackgroundHeader.fx"

TECHNIQUE30(a, 0, 0, 0, 0 )
TECHNIQUE30(b, 0, 1, 0, 0 )

#include "Dx9/_NoPixelShader.fx"
#endif