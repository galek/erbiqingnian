//
// BackgroundCommon - header file for background shader functions
//

#include "_common.fx"

#include "Dx9/_EnvMap.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_Shadow.fxh"
#include "Dx9/_ScrollUV.fx"
//#include "Dx9/_SamplersCommon.fxh"

#pragma warning(disable : 1519) // macro redefinition

//--------------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// STRUCTS
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// FUNCTIONS
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// LIGHTING
//--------------------------------------------------------------------------------------------
















// Common functions for all background techniques


//--------------------------------------------------------------------------------------------
// INPUT STRUCTS
//--------------------------------------------------------------------------------------------

// PIXEL SHADER INPUT
// -------------------

struct PS_COMMON_BG_BASE_INPUT
{
    float4 Diffuse				: COLOR0;
    float4 LightMapDiffuse 		: TEXCOORD0;
    float4 P					: TEXCOORD1;	// if normal map, in tangent, else in world
    float4 Nw					: TEXCOORD2; // using w for linear-w
	float4 Shadow1				: TEXCOORD3;	// first shadow map coords set
	float4 EnvMap_Shadow2		: TEXCOORD4;	// environment map or 2nd shadow map coords set
};

struct PS_COMMON_BG_ADDL_INPUT
{
    float4 T5					: TEXCOORD5;
    float4 T6					: TEXCOORD6;
    float4 T7					: TEXCOORD7;
	float4 C1					: COLOR1;
};


struct PS_COMMON_BG_TBNMAT_INPUT
{
    float4 Tw;
    float4 Bw;
    float4 DiffuseMap2;
    float4 Unused;
};

struct PS_COMMON_BG_LIGHTS_INPUT
{
    float3 Dt;					// diffuse vector
    float  DM2u;					// diffusemap2 coords.x
    float4 St;					// specular vector
    float3 Et;					// eye position
    float  DM2v;					// diffusemap2 coords.y
    float4 C0;					// specular color
};


//--------------------------------------------------------------------------------------------
// SHADERS
//--------------------------------------------------------------------------------------------


void COMMON_BG_TBNMAT_SetDiffuseMap2(
	inout PS_COMMON_BG_TBNMAT_INPUT		OutTBN,
	in	  half2							DiffuseMap2 )
{
	OutTBN.DiffuseMap2	= half4( DiffuseMap2, 0, 0 );
}

void COMMON_BG_TBNMAT_GetDiffuseMap2(
	in	  PS_COMMON_BG_TBNMAT_INPUT		InTBN,
	out	  half2							DiffuseMap2 )
{
	DiffuseMap2 = InTBN.DiffuseMap2.xy;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void COMMON_BG_LIGHTS_SetDiffuseMap2(
	inout PS_COMMON_BG_LIGHTS_INPUT		OutLights,
	in	  half2							DiffuseMap2 )
{
	OutLights.DM2u	= DiffuseMap2.x;
	OutLights.DM2v	= DiffuseMap2.y;
}

void COMMON_BG_LIGHTS_GetDiffuseMap2(
	in	  PS_COMMON_BG_LIGHTS_INPUT		InLights,
	out	  half2							DiffuseMap2 )
{
	DiffuseMap2.x = InLights.DM2u;
	DiffuseMap2.y = InLights.DM2v;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_Common( 
	in		float4						Po,
	in		half3						Nw,
	in		half4						LightMapDiffuse,
	in		half						ao,
	inout	PS_COMMON_BG_BASE_INPUT		Out,
	out		float4						Prhw,
	//
	uniform int							nPointLights,
	uniform bool						bNormalMap,
	uniform bool						bSH4,
	uniform bool						bSH9,
	uniform bool						bIndoor,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bHemisphereLight,
	uniform bool						bPSWorldNMap,
	uniform bool						bCubeEnvironmentMap,
	uniform int							nShadowType )
{
	Prhw				= mul( Po, WorldViewProjection ); 
	half4 Pw			= mul( half4(Po), World );
	Out.LightMapDiffuse	= LightMapDiffuse;

	if ( ! bNormalMap )
	{
		Out.P.xyz		= Pw.xyz;
	}

	if ( nShadowType != SHADOW_TYPE_NONE )
	{
		ComputeShadowCoords( Out.Shadow1, Po );
		
		if ( ! bIndoor )
			ComputeShadowCoords( Out.EnvMap_Shadow2, Po, true );
	}

	Out.Diffuse			= GetAmbientColor();  // initializes w to 0.f

	Out.Diffuse.xyz		+= CombinePointLights( Pw, Nw, nPointLights, 0, WORLD_SPACE );


	if ( bSH9 )
		Out.Diffuse.xyz += SHEval_9_RGB( Nw );
	else if ( bSH4 )
		Out.Diffuse.xyz += SHEval_4_RGB( Nw );

	if ( bDir0 )
		Out.Diffuse.xyz += GetDirectionalColor( Nw, DIR_LIGHT_0_DIFFUSE, WORLD_SPACE );		

	if ( bDir1 )
		Out.Diffuse.xyz += GetDirectionalColor( Nw, DIR_LIGHT_1_DIFFUSE, WORLD_SPACE );

	if ( bHemisphereLight )
		Out.Diffuse.xyz += HemisphereLight( Nw, WORLD_SPACE );

	if ( bCubeEnvironmentMap )
		Out.EnvMap_Shadow2.xyz	= CubeMapGetCoords( Pw.xyz, half3( EyeInWorld.xyz ), Nw );

	// ambient occlusion
	Out.P.w = lerp( 1.f, ao, gfShadowIntensity );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_WorldNormalMap( 
	in		half3						InN,
	in		half3						InT,
	in		half3						InB,
	in		half2						DiffuseMap2,
	inout	PS_COMMON_BG_BASE_INPUT		Out,
	inout	PS_COMMON_BG_TBNMAT_INPUT	OutTBN,
	out		half3						Nw,
	//
	uniform bool						bDiffuseMap2 )
{
	half3 No;
	VS_GetRigidNormal( InN, No );
	half3 tNw = mul( No, World );
	Out.Nw = half4(tNw,1);
	Nw = tNw.xyz;

	OutTBN.Tw.xyz = mul( RIGID_TANGENT_V (InT), World );
	OutTBN.Bw.xyz = mul( RIGID_BINORMAL_V(InB), World );

	if ( bDiffuseMap2 )
		COMMON_BG_TBNMAT_SetDiffuseMap2( OutTBN, DiffuseMap2 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_LightingToPS( 
	in		half3						InN,
	in		half3						InT,
	in		half3						InB,
	in		half3						Po,
	in		half2						DiffuseMap2,
	inout	PS_COMMON_BG_BASE_INPUT		Out,
	inout	PS_COMMON_BG_LIGHTS_INPUT	OutLights,
	out		half3						Nw,
	//
	uniform bool						bIndoor,
	uniform bool						bNormalMap,
	uniform bool						bDiffuseMap2,
	uniform bool						bDir0_PS,
	uniform bool						bHemisphereLight_PS,
	uniform bool						bSpecularDir,
	uniform bool						bSpecularPts )
{
	half3 No;
	half3x3 o2tMat;
	VS_GetRigidNormalObjToTan( InN.xyz, InT, InB, o2tMat, No, bNormalMap );
	Nw = mul( No, World );


	if ( bNormalMap )
	{
		if ( bDir0_PS || bHemisphereLight_PS )
		{
			// directional diffuse or hemispherical light added per-pixel
			// Dt gets dir diffuse

			OutLights.Dt.xyz		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ].xyz;
			OutLights.Dt.xyz		= mul( o2tMat, half3( OutLights.Dt.xyz	) );
		}

		if ( bSpecularDir )
		{
			// St gets dir specular
			OutLights.St.xyz		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_SPECULAR ].xyz;
			OutLights.St.xyz		= mul( o2tMat, half3( OutLights.St.xyz	) );
		}
	}


	if ( bSpecularPts )
	{
		if ( bNormalMap )
		{
			VS_PickAndPrepareSpecular( Po.xyz, No, OutLights.C0, OutLights.St, false, OBJECT_SPACE );
			OutLights.St.xyz		= mul( o2tMat, half3( OutLights.St.xyz	) );
		}
		else
		{
			half3 Pw = mul( half4(Po,1), World );
			VS_PickAndPrepareSpecular( Pw, Nw, OutLights.C0, OutLights.St, false, WORLD_SPACE );
		}
	}

	// normal is always in world (normal map is used for tangent space one anyway)
	Out.Nw.xyz			= Nw;


	if ( bNormalMap )
	{
		Out.P.xyz			= Po;
		OutLights.Et.xyz	= EyeInObject;
		// move into tangent space
		Out.P.xyz			= mul( o2tMat, half3( Out.P.xyz			) );
		OutLights.Et.xyz	= mul( o2tMat, half3( OutLights.Et.xyz	) );
	}
	else
	{
		// Pass in world space
		// Out.P set elsewhere
		OutLights.Et.xyz	= EyeInWorld;
	}

	if ( bDiffuseMap2 )
		COMMON_BG_LIGHTS_SetDiffuseMap2( OutLights, DiffuseMap2 );
}






//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void VS_COMMON_BACKGROUND ( 
	in float4						InPosition,
	in half4						InLightMapDiffuse,
	in half4						InNormal,
	in half3						InTangent,
	in half3						InBinormal,
	in half2						InDiffuseMap2,
	//
	out PS_COMMON_BG_BASE_INPUT		OutBase,
	out PS_COMMON_BG_ADDL_INPUT		OutAddl,
	out float4						OutPosition : POSITION,
	//
	uniform int		nPointLights,
	SHADER_BRANCH_TYPE,
	uniform branch	bSpotlight,
	uniform branch	bIndoor,
	uniform branch	bDiffuseMap2,
	uniform branch	bNormalMap,
	uniform branch  bDir0,
	uniform branch  bDir1,
	uniform branch  bSH9,
	uniform branch  bSH4,
	uniform branch	bHemisphereLight,
	uniform branch	bSpecularDir,
	uniform branch	bSpecularPts,
	uniform branch	bPSWorldNMap,
	uniform branch	bCubeEnvironmentMap,
	uniform int		nShadowType )
{
	// For outdoor shadows, we use 2 separate sets of coords and lookups.  NVIDIA hw shadows require full a float4 each,
	// so we co-opt the envmap coords interpolant and instead calculate any envmap reflection vectors on the PS.
	// As a result, we don't support PS-tangent-space normal mapping and environment mapping in the outdoor shadow case.
	if ( ! bIndoor && nShadowType != SHADOW_TYPE_NONE && bCubeEnvironmentMap )
	{
		bCubeEnvironmentMap = _ON_PS;
		if ( ! B( bPSWorldNMap ) )
			bNormalMap = _OFF;
	}

	OutBase	 = (PS_COMMON_BG_BASE_INPUT)0;

	half3 Nw;

	// no specular pts allowed if doing world space normal map (insuff interpolators)

	half3x3 o2tMat;

	if ( B( bSpotlight ) )
	{
		half3 No;
		VS_GetRigidNormalObjToTan( InNormal, InTangent, InBinormal, o2tMat, No, B( bNormalMap ) );
		Nw = mul( No, World );
		
		OutBase.Nw.xyz = Nw;
	}
	else if ( B( bPSWorldNMap ) && B( bNormalMap ) )
	{
		PS_COMMON_BG_TBNMAT_INPUT OutTBN = (PS_COMMON_BG_TBNMAT_INPUT)0;

		VS_WorldNormalMap(
			InNormal.xyz,
			InTangent,
			InBinormal,
			InDiffuseMap2,
			OutBase,
			OutTBN,
			Nw,
			B( bDiffuseMap2 ) );


		OutAddl = (PS_COMMON_BG_ADDL_INPUT)( OutTBN );
	}
	else
	{
		PS_COMMON_BG_LIGHTS_INPUT OutLights = (PS_COMMON_BG_LIGHTS_INPUT)0;

		VS_LightingToPS(
			InNormal.xyz,
			InTangent,
			InBinormal,
			InPosition.xyz,
			InDiffuseMap2,
			OutBase,
			OutLights,
			Nw,
			B( bIndoor ),
			B( bNormalMap ),
			B( bDiffuseMap2 ),
			bDir0 == _ON_PS,
			bHemisphereLight == _ON_PS,
			B( bSpecularDir ),
			B( bSpecularPts )
			 );

		OutAddl = (PS_COMMON_BG_ADDL_INPUT)( OutLights );
	}

	VS_Common(
		InPosition,
		Nw,
		InLightMapDiffuse,
		InNormal.w,
		OutBase,
		OutPosition,
		//
		nPointLights,
		B( bNormalMap ),					// normal map
		B( bSH4 ),							// SH4
		B( bSH9 ),							// SH9
		B( bIndoor),						// indoor
		B( bDir0 ),							// dirlight1
		B( bDir1 ),							// dirlight2
		B( bHemisphereLight ),				// hemisphere light
		B( bPSWorldNMap ),					// PS world-space normal mapping
		B( bCubeEnvironmentMap ),			// cube environment map
		nShadowType );

	if ( B( bSpotlight ) )
	{
		PS_COMMON_SPOTLIGHT_INPUT OutSpot = (PS_COMMON_SPOTLIGHT_INPUT)0;

		VS_Spotlight(
			WORLD_SPACE,
			B( bNormalMap ),
			B( bNormalMap ) ? InPosition.xyz : OutBase.P.xyz,
			o2tMat,
			OutSpot
		);

		OutAddl = (PS_COMMON_BG_ADDL_INPUT)( OutSpot );
	}
	
/*
	if ( bSpecular && ! bSpotlight )
	{
		VS_PickAndPrepareSpecular( InPosition.xyz, No, Out.SpecularColor, Out.SpecularLightVector, false );
	}
*/

	OutBase.Diffuse.xyz = MDR_Pack( OutBase.Diffuse.xyz );
}





















//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



void PS_SpecularLightingInWorld(
	in		half3			Pw,
	in		half3			Nw,
	in		half2			vCoords,
	inout	Light			L,
	inout	half4			vSpecularMap,
	//
	uniform bool			bAttenuateGlow,
	uniform bool			bSpecularMap,
	uniform bool			bSpecularViaLUT
	)
{
	PS_GetSpecularMap(
		vCoords,
		vSpecularMap,
		L.SpecularPower,
		L.SpecularAlpha,
		L.SpecularGloss,
		bSpecularMap );

	half3 Vw				= normalize( half3( EyeInWorld.xyz ) - Pw );
	half3 Lw				= DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_SPECULAR ].xyz;
	half4 C					= DirLightsColor[ DIR_LIGHT_0_SPECULAR ];

	PS_EvaluateSpecular( Vw, Lw, Nw, C, vSpecularMap, L.SpecularGloss, L.Glow, L.Specular, bAttenuateGlow, bSpecularViaLUT );

	L.Glow					*= SpecularGlow * L.SpecularPower * half( SpecularBrightness );
}









//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void PS_LightingFromVS(
	in		PS_COMMON_BG_LIGHTS_INPUT	In,
	in		half3						Pt,
	in		half3						Nt,
	in		half2						vCoords,
	inout	Light						L,
	inout	half4						vSpecularMap,
	//
	uniform bool						bAttenuateGlow,
	uniform bool						bIndoor,
	uniform bool						bDir0,
	uniform bool						bHemisphereLight,
	uniform bool						bNormalMap,
	uniform bool						bSpecularDir,
	uniform bool						bSpecularPts,
	uniform bool						bSpecularMap,
	uniform bool						bSpecularViaLUT
	)
{
	PS_GetSpecularMap(
		vCoords,
		vSpecularMap,
		L.SpecularPower,
		L.SpecularAlpha,
		L.SpecularGloss,
		bSpecularMap );

	half3 Vt					= normalize( half3( In.Et.xyz ) - Pt.xyz );

	half3 DiffuseLightVector	= In.Dt.xyz;
	half4 SpecularLightVector	= In.St;


	if ( bSpecularDir )
	{
		// specular dir light in object/tangent space

		half3 Lt;
		if ( bNormalMap )
			Lt = normalize( SpecularLightVector.xyz );
		else
			Lt = DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_SPECULAR ];

		PS_EvaluateSpecular( Vt, Lt, Nt, DirLightsColor[ DIR_LIGHT_0_SPECULAR ], vSpecularMap, L.SpecularGloss, L.Glow, L.Specular, bAttenuateGlow, bSpecularViaLUT );
	}

	if ( ( bDir0 || bHemisphereLight ) && bNormalMap )
	{
		half3 Lt;
		Lt = normalize( DiffuseLightVector.xyz );

		if ( bDir0 )
			L.Diffuse += GetDirectionalColorEX( Nt, Lt, DirLightsColor[ DIR_LIGHT_0_DIFFUSE ] );

		if ( bHemisphereLight )
			L.Diffuse += HemisphereLightEx( Nt, Lt );
	}

	if ( bSpecularPts )
	{
		// specular point light in object/tangent space
		half3 Lt = normalize( SpecularLightVector.xyz );
		half4 vColor = half4(In.C0) * SpecularLightVector.w;

		PS_EvaluateSpecular( Vt, Lt, Nt, vColor, vSpecularMap, L.SpecularGloss, L.Glow, L.Specular, bAttenuateGlow, bSpecularViaLUT );
	}

	L.Glow		  *= SpecularGlow * L.SpecularPower * half( SpecularBrightness );
}











void PS_Common(
	out		half4						Color,
	//
	in		PS_COMMON_BG_BASE_INPUT		In,
	in		PS_COMMON_BG_ADDL_INPUT		InAddl,
	in		half3						Nw,
	in		half2						vLightmapUV,
	in		half2						vDiffuseUV,
	in		half2						vDiffuseScrollUV,
	inout	Light						L,
	//
	uniform bool						bIndoor,
	uniform bool						bNormalMap,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bSH9,
	uniform bool						bSH4,
	uniform bool						bSpecularMap,
	uniform bool						bHemisphereLight,
	uniform bool						bSelfIllum,
	uniform bool						bDiffuseMap2,
	uniform bool						bLightMap,
	uniform branch						brCubeEnvironmentMap,
	uniform branch						brSphereEnvironmentMap,
	uniform bool						bScroll,
	uniform bool						bManualFog,
	uniform int							nShadowType,
	uniform bool						bSelfShadow
	)
{

	L.LightingAmount	= 1.f; // unshadowed		
	L.Diffuse			+= MDR_Unpack( In.Diffuse.rgb );



	if ( bSelfIllum )
	{
		half fMask = 1;
		if ( bScroll )
			fMask = SAMPLE_SELFILLUM( vDiffuseUV ).a;

		half3 SelfIllum = SAMPLE_SELFILLUM( AdjustUV( SAMPLER_SELFILLUM, vDiffuseUV, vDiffuseScrollUV, bScroll ) ).rgb;
		SelfIllum		*= fMask * gfSelfIlluminationPower;
		L.Emissive		+= SelfIllum;
		L.Glow			+= dot( SelfIllum, SelfIllum ) * gfSelfIlluminationGlow;
	}


	if ( bDir0 )
		L.Diffuse += GetDirectionalColorEX( Nw, DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_DIFFUSE], DirLightsColor[ DIR_LIGHT_0_DIFFUSE ].xyz );

	// these should be done after shadowing, but that creates a major inconsistency with VS application
	if ( bDir1 )
		L.Diffuse += GetDirectionalColorEX( Nw, DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_1_DIFFUSE], DirLightsColor[ DIR_LIGHT_1_DIFFUSE ].xyz );

	if ( bSH9 )
		L.Diffuse += SHEval_9_RGB( Nw );
	else if ( bSH4 )
		L.Diffuse += SHEval_4_RGB( Nw );

	if ( bHemisphereLight )
		L.Diffuse += HemisphereLight( Nw, WORLD_SPACE );

	if ( nShadowType != SHADOW_TYPE_NONE )
	{
		// If this fragment is facing away from the light direction, then it 
		// is already in shadow.
		if ( ( ! bIndoor || bSelfShadow ) && dot( ShadowLightDir, In.Nw ) >= 0.f )
		{
			if ( bIndoor )
				L.LightingAmount = 0.f;
			else
				L.LightingAmount = 0.5f;
		}
		else
		{
			if (nShadowType == SHADOW_TYPE_DEPTH)
			{					
				if ( bIndoor )
					L.LightingAmount = ComputeLightingAmount( In.Shadow1, false, false );
				else
				{
					float fLightingAmount = ComputeLightingAmount( In.Shadow1, false, true );
					float fLighting2Amount = ComputeLightingAmount( In.EnvMap_Shadow2, true, false );

					// Lighten the background shadows a bit, so character shadows appear darker.
					fLightingAmount = ( fLightingAmount + 1.f ) / 2.f; 
					
					L.LightingAmount = min( fLightingAmount, fLighting2Amount );					
				}			
			}
			else
			{
				if ( bIndoor )
					L.LightingAmount = ComputeLightingAmount_Color( In.Shadow1, false, false );
				else
				{
					float fLightingAmount = ComputeLightingAmount_Color( In.Shadow1, false, true);
					float fLighting2Amount = ComputeLightingAmount_Color( In.EnvMap_Shadow2, true, false );
					
					// Lighten the background shadows a bit, so character shadows appear darker.
					fLightingAmount = ( fLightingAmount + 1.f ) / 2.f; 
					
					L.LightingAmount = min( fLightingAmount, fLighting2Amount );
				}
			}		
		}

		if ( ! bIndoor )
		{
			half fMin		= half( gfShadowIntensity ); // * 0.5f;
			L.Specular		*= lerp( fMin, 1.0f, L.LightingAmount ); // specular
		}

		L.LightingAmount	*= half( gfShadowIntensity );
		L.LightingAmount	+= 1.0f - half( gfShadowIntensity );
		
		
	}

	// Apply ambient occlusion
	L.Diffuse			*= In.P.w;

	// Ambient occlusion does not affect the lightmap
	if ( bLightMap )
		L.Diffuse		+= SAMPLE_LIGHTMAP( vLightmapUV );

	// Apply shadows
	L.Diffuse			*= L.LightingAmount;


	if ( brCubeEnvironmentMap || brSphereEnvironmentMap )
	{
		half3 vCoords;
		if ( ( ( brCubeEnvironmentMap == _ON_PS ) || ( brSphereEnvironmentMap == _ON_PS ) ) && ! bNormalMap )
			vCoords	= CubeMapGetCoords( half3( In.P.xyz ), EyeInWorld.xyz, Nw );
		else
			vCoords = normalize( half3( In.EnvMap_Shadow2.xyz ) );		// Does this need to be normalized?  Hm...

		half4 vEnvMap = EnvMapLookupCoords( Nw, vCoords, L.SpecularPower, L.SpecularAlpha, brCubeEnvironmentMap, brSphereEnvironmentMap, bSpecularMap );

		// alpha contains env map power/lerp factor
		L.Reflect.rgba = vEnvMap.rgba;
	}




	// LDR clamp is handled in CombineLight
	Color.rgb = CombineLight( L, true, brCubeEnvironmentMap || brSphereEnvironmentMap );

	if ( bManualFog )
	{
		APPLY_FOG( Color.rgb, In.Diffuse.a );
		L.Glow *= In.Diffuse.a;
	}


	//Color.a = L.Opacity	* ( ( ( half( 1.0f ) - gfAlphaForLighting ) * gfAlphaConstant ) + ( max( L.Glow, GLOW_MIN ) * gfAlphaForLighting ) );
	Color.a = ComputeScaledAlpha( L.Opacity, L.Glow );
}








//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void PS_COMMON_BACKGROUND( 
	in		PS_COMMON_BG_BASE_INPUT		In,
	in		PS_COMMON_BG_ADDL_INPUT		InAddl,
	out		half4						Color,
	//
	SHADER_BRANCH_TYPE,
	uniform bool						bSpotlight,
	uniform bool						bIndoor,
	uniform bool						bNormalMap,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bSH9,
	uniform bool						bSH4,
	uniform bool						bHemisphereLight,
	uniform bool						bPSWorldNMap,
	uniform bool						bSpecularDir,
	uniform bool						bSpecularPts,
	uniform bool						bSpecularMap,
	uniform bool						bSpecularViaLUT,
	uniform bool						bDiffuseMap2,
	uniform bool						bLightMap,
	uniform bool						bSelfIllum,
	uniform branch						brCubeEnvironmentMap,
	uniform bool						bScroll,
	uniform bool						bManualFog,
	uniform int							nShadowType,
	uniform bool						bSelfShadow )
{
	// For outdoor shadows, we use 2 separate sets of coords and lookups.  NVIDIA hw shadows require full a float4 each,
	// so we co-opt the envmap coords interpolant and instead calculate any envmap reflection vectors on the PS.
	// As a result, we don't support PS-tangent-space normal mapping and environment mapping in the outdoor shadow case.
	if ( ! bIndoor && nShadowType != SHADOW_TYPE_NONE && brCubeEnvironmentMap )
	{
		brCubeEnvironmentMap = _ON_PS;
		if ( ! bPSWorldNMap )
			bNormalMap = false;
	}


	Light L					= (Light)0;	
	half3 Nw				= normalize( half3( In.Nw.xyz ) );

	half2 vLightmapUV			= In.LightMapDiffuse.xy;
	half2 vDiffuseUV			= In.LightMapDiffuse.zw;
	half2 vDiffuseScrollUV;
	half4 vSpecularMap;

	if ( bScroll )
		vDiffuseScrollUV	= ScrollTextureCoordsHalf( vDiffuseUV, SCROLL_CHANNEL_DIFFUSE );

	half4 DiffuseMap		= tex2D( DiffuseMapSampler, vDiffuseUV );
	L.Albedo				= DiffuseMap.rgb;
	L.Opacity				= DiffuseMap.a;

	bool bGetSpecularMap = false;


	if ( bSpotlight )
	{
		PS_COMMON_SPOTLIGHT_INPUT InSpot = (PS_COMMON_SPOTLIGHT_INPUT)InAddl;

		half3 N = Nw;
		if ( bNormalMap )
		{
			N = ReadNormalMap( vDiffuseUV );
		}

		PS_Spotlight(bNormalMap, N, InSpot, L);

		// CHB 2007.07.17 - Spot lights do not currently work with worldspace PS, specular.
		ASSERT_NOT_BRANCH_COMBINATION( B(brCubeEnvironmentMap) )
		//ASSERT_NOT_BRANCH_COMBINATION(bSphereEnvironmentMap)
		ASSERT_NOT_BRANCH_COMBINATION(bPSWorldNMap)
		ASSERT_NOT_BRANCH_COMBINATION(bSpecularDir || bSpecularPts || bSpecularMap || bSpecularViaLUT)
	}
	else if ( bPSWorldNMap && bNormalMap )
	{
		// normal in world space

		PS_COMMON_BG_TBNMAT_INPUT	InTBN = (PS_COMMON_BG_TBNMAT_INPUT)InAddl;

		if ( bNormalMap )
		{
			PS_GetWorldNormalFromMap(
				vDiffuseUV,
				half3( InTBN.Tw.xyz ),
				half3( InTBN.Bw.xyz ),
				half3( In.Nw.xyz ),
				Nw );
		}

		if ( bSpecularDir )
		{
			PS_SpecularLightingInWorld(
				half3( In.P.xyz ),
				Nw,
				vDiffuseUV,
				L,
				vSpecularMap,
				//
				true,					// attenuate glow
				bSpecularMap,
				bSpecularViaLUT
				);
		}
		else if ( brCubeEnvironmentMap )
		{
			bGetSpecularMap = true;
		}

		if ( bDiffuseMap2 )
		{
			half2 vDiffMap2;
			COMMON_BG_TBNMAT_GetDiffuseMap2( InTBN, vDiffMap2 );
			half4 cDiffMap2		= tex2D( DiffuseMapSampler2, vDiffMap2 );
			L.Albedo			= lerp( L.Albedo, cDiffMap2.xyz, cDiffMap2.w );
		}
	} else 
	{
		// normal in tangent or world space

		PS_COMMON_BG_LIGHTS_INPUT	InLights = (PS_COMMON_BG_LIGHTS_INPUT)InAddl;

		half3 N = Nw;
		if ( bNormalMap )
		{
			N = ReadNormalMap( vDiffuseUV );
		}

		if (    bSpecularDir 
			 || bSpecularPts
			 ||  ( ( bHemisphereLight || bDir0 )  && bNormalMap ) )  // and therefore not psworld
		{
			PS_LightingFromVS(
				InLights,
				half3( In.P.xyz ),
				N,
				vDiffuseUV,
				L,
				vSpecularMap,
				//
				true,					// bAttenuateGlow,
				bIndoor,
				bDir0,
				bHemisphereLight,
				bNormalMap,
				bSpecularDir,
				bSpecularPts,
				bSpecularMap,
				bSpecularViaLUT
				 );
		}
		else if ( brCubeEnvironmentMap )
		{
			bGetSpecularMap = true;
		}

		if ( bDiffuseMap2 )
		{
			half2 vDiffMap2;
			COMMON_BG_LIGHTS_GetDiffuseMap2( InLights, vDiffMap2 );
			half4 cDiffMap2		= tex2D( DiffuseMapSampler2, vDiffMap2 );
			L.Albedo			= lerp( L.Albedo, cDiffMap2.xyz, cDiffMap2.w );
		}
	}


	if ( bGetSpecularMap )
	{
		// this is just to get the specular parameters needed for cube maps

		PS_GetSpecularMap(
			vDiffuseUV,
			vSpecularMap,
			L.SpecularPower,
			L.SpecularAlpha,
			L.SpecularGloss,
			bSpecularMap );
	}


	if ( bDir0 )
		bDir0 = bPSWorldNMap || ! bNormalMap;
	if ( bHemisphereLight )
		bHemisphereLight = bPSWorldNMap || ! bNormalMap;

	PS_Common(
		Color,
		//
		In,
		InAddl,
		Nw,
		vLightmapUV,
		vDiffuseUV,
		vDiffuseScrollUV,
		L,
		//
		bIndoor,
		bNormalMap,
		bDir0,
		bDir1,
		bSH9,
		bSH4,
		bSpecularMap,
		bHemisphereLight,
		bSelfIllum,
		bDiffuseMap2,
		bLightMap,
		brCubeEnvironmentMap,
		false,					// brSphereEnvironmentMap
		bScroll,
		bManualFog,
		nShadowType,
		bSelfShadow );
}
