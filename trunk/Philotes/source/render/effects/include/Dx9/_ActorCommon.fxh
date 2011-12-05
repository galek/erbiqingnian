//
// ActorCommon - header file for actor shader functions
//

#include "_common.fx"

#include "Dx9/_EnvMap.fx"
#include "Dx9/_SphericalHarmonics.fx"
#include "Dx9/_AnimatedShared20.fxh"
#include "Dx9/_Shadow.fxh"
#include "Dx9/_ScrollUV.fx"

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
















// Common functions for all actor techniques


//--------------------------------------------------------------------------------------------
// INPUT STRUCTS
//--------------------------------------------------------------------------------------------

// PIXEL SHADER INPUT
// -------------------

struct PS_COMMON_ACTOR_BASE_INPUT
{
    float4 Diffuse				: COLOR0;
    //float4 SpotColor			: COLOR1;
    float4 DiffuseMap 			: TEXCOORD0;	// using zw for sss strength and strengthSq when bScatter = _ON_VS
    float4 P					: TEXCOORD1;
    float4 Nw					: TEXCOORD2;	// using w for linear-w
	float4 ShadowMapCoords		: TEXCOORD3;
	float4 EnvMapCoords			: TEXCOORD4;	// using all 4 components
};

struct PS_COMMON_ACTOR_ADDL_INPUT
{
    float4 T5					: TEXCOORD5;
    float4 T6					: TEXCOORD6;
    float4 T7					: TEXCOORD7;
	float4 C1					: COLOR1;
};


struct PS_COMMON_ACTOR_TBNMAT_INPUT
{
    float4 Tw;
    float4 Bw;
    float4 Unused0;
    float4 Unused1;
};

struct PS_COMMON_ACTOR_LIGHTS_INPUT
{
    float4 Dt;					// diffuse vector
    float4 Pw;					// position in world
    float4 Et;					// eye position - in tangent space if normal mapping, world space otherwise
    float4 St;					// specular vector
};


//--------------------------------------------------------------------------------------------
// SHADERS
//--------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void VS_Common( 
	in		VS_ANIMATED_INPUT			In,
	in		float4						Po,	
	in		half3						Nw,
	in		half3						Pw,
	inout	PS_COMMON_ACTOR_BASE_INPUT	Out,
	out		float4						Prhw,
	//
	uniform int							nTotalPointLights,
	uniform int							nPSPointLights,
	uniform bool						bSpotlight,
	uniform bool						bNormalMap,
	uniform bool						bSH4,
	uniform bool						bSH9,
	uniform bool						bIndoor,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bDiffusePts,
	uniform bool						bPSWorld,
	uniform bool						bCubeEnvironmentMap,
	uniform bool						bSphereEnvironmentMap,
	uniform bool						bScatter,
	uniform int							nShadowType )
{
	Prhw				= mul( Po, WorldViewProjection ); 	
	
	Out.DiffuseMap.xy	= In.DiffuseMap;

	if ( bPSWorld )
	{
		Out.P			= half4( Pw, 1.f );
	}

	if ( nShadowType != SHADOW_TYPE_NONE )
	{
		if ( dot( ShadowLightDir, Nw ) >= 0.0f )
		{
			if ( ! bIndoor )
				Out.EnvMapCoords.w = 0.0f;
			else
				Out.EnvMapCoords.w = 1.0f;
		}
		else
		{
			Out.EnvMapCoords.w = 1.0f;
			ComputeShadowCoords( Out.ShadowMapCoords, Po );
		}
	}

	Out.Diffuse			= GetAmbientColor();  // initializes w to 0.f

	if ( bSH9 )
		Out.Diffuse.xyz += SHEval_9_RGB( Nw );
	else if ( bSH4 )
		Out.Diffuse.xyz += SHEval_4_RGB( Nw );

	if ( bDir0 )
		Out.Diffuse.xyz += GetDirectionalColor( Nw, DIR_LIGHT_0_DIFFUSE, WORLD_SPACE );		

	if ( bDir1 )
		Out.Diffuse.xyz += GetDirectionalColor( Nw, DIR_LIGHT_1_DIFFUSE, WORLD_SPACE );

	half3 V = normalize( EyeInWorld - Pw );
	if ( nTotalPointLights > 0 && bDiffusePts )
	{
		for ( int nP = 0; nP < nTotalPointLights; nP++ )
		{
			half4 PlC;
			half3 I;
			LightPointCommon( nP, Pw, PlC, I, WORLD_SPACE );
			LightPointDiffuse( Nw, I, PlC, Out.Diffuse.xyz );
		}
	}

	if ( bCubeEnvironmentMap || bSphereEnvironmentMap )
	{
		Out.EnvMapCoords.xyz	= CubeMapGetCoords( Pw.xyz, half3( EyeInWorld.xyz ), Nw );
	}

	if ( bScatter )
	{
		GetSubsurfaceScatterFactor( V, Nw, Out.DiffuseMap.z /*, Out.DiffuseMap.w*/ );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_WorldNormalMap( 
	in		half3x3							mTan,
	in		half3							T,
	in		half3							B,
	inout	PS_COMMON_ACTOR_BASE_INPUT		Out,
	inout	PS_COMMON_ACTOR_TBNMAT_INPUT	OutTBN,
	in		half3							Nw_ro,
	//
	uniform bool							bNormalMap
	)
{
	Out.Nw = half4( Nw_ro, 0.f );

	if ( bNormalMap )
	{
		OutTBN.Tw = mul( mTan[0], World );
		OutTBN.Bw = mul( mTan[1], World );
	}
	else
	{
		OutTBN.Tw = mul( ANIMATED_TANGENT_V (T), World );
		OutTBN.Bw = mul( ANIMATED_BINORMAL_V(B), World );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_LightIntoInterpolants(
	int			nP,
	in	half3	Po,
	in	half3x3	mTan,
	out half4	Lo )
{
	Lo.xyz	= PointLightsPos(OBJECT_SPACE)[ nP ] - Po;
	Lo.w	= GetFalloffBrightness( Lo.xyz, PointLightsFalloff(OBJECT_SPACE)[ nP ] );
	Lo.xyz	= mul( mTan, Lo.xyz );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_LightingToPS( 
	in		half3x3							mTan,
	in		half3							Po,
	inout	PS_COMMON_ACTOR_BASE_INPUT		Out,
	inout	PS_COMMON_ACTOR_LIGHTS_INPUT	OutLights,
	in		half3							Nw_ro,
	in		half3							Pw_ro,
	//
	uniform int								nPointLights,
	uniform bool							bIndoor,
	uniform bool							bNormalMap,
	uniform bool							bSpecularDir,
	uniform bool							bDir0_PS,
	uniform bool							bDiffusePts_PS,
	uniform bool							bSpecularPts_PS )
{
	if ( bNormalMap )
	{
		if ( bDir0_PS )
		{
			// directional diffuse or hemispherical light added per-pixel
			// Dt gets dir diffuse

			OutLights.Dt.xyz		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ].xyz;
			OutLights.Dt.xyz		= mul( mTan, half3( OutLights.Dt.xyz	) );
		}

		if ( bSpecularDir )
		{
			// St gets dir specular
			OutLights.St.xyz		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_SPECULAR ].xyz;
			OutLights.St.xyz		= mul( mTan, half3( OutLights.St.xyz	) );
		}

		if ( bDiffusePts_PS || bSpecularPts_PS )
		{
			// first point light goes into Dt, 2nd into St, saving falloffs in .w

			if ( nPointLights > 0 )
				VS_LightIntoInterpolants( 0, Po, mTan, OutLights.Dt );
			if ( nPointLights > 1 )
				VS_LightIntoInterpolants( 1, Po, mTan, OutLights.St );
		}
	}

	// normal is always in world (normal map is used for targent space one anyway)
	Out.Nw.xyz			= Nw_ro;
	OutLights.Pw.xyz	= Pw_ro;

	if ( bNormalMap )
	{
		// move into tangent space
		Out.P.xyz			= mul( mTan, half3( Po.xyz			) );
		OutLights.Et.xyz	= mul( mTan, half3( EyeInObject.xyz	) );
	}
	else
	{
		Out.P.xyz			= Po;
		OutLights.Et.xyz	= EyeInWorld;
	}
 }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void VS_GetInputs(
	in		VS_ANIMATED_INPUT			In,
	inout	PS_COMMON_ACTOR_BASE_INPUT	OutBase,
	out		float4						Po,
	out		half3						No,
	out		half3x3						mTan,
	//
	uniform bool						bSkinned,
	uniform bool						bNormalMap )
{
	mTan = 0;

	if ( bSkinned )
	{
		if ( bNormalMap )
			GetPositionNormalAndObjectMatrix( Po, No, mTan, In ); 
		else
			GetPositionAndNormal( Po, No, In );

	} else {
	
 		Po = float4( In.Position, 1.0f );

		No = ANIMATED_NORMAL;
		if ( bNormalMap )
		{
			mTan[0] = ANIMATED_TANGENT;
			mTan[1] = ANIMATED_BINORMAL;
			mTan[2] = No;
		}
	}
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void VS_COMMON_ACTOR ( 
	VS_ANIMATED_INPUT				In,
	//
	out PS_COMMON_ACTOR_BASE_INPUT	OutBase,
	out PS_COMMON_ACTOR_ADDL_INPUT	OutAddl,
	out float4						OutPosition : POSITION,
	//
	uniform int		nTotalPointLights,
	uniform int		nPSPointLights,
	SHADER_BRANCH_TYPE,
	uniform branch	bSkinned,
	uniform branch	bSpotlight,
	uniform branch	bIndoor,
	uniform branch	bNormalMap,
	uniform branch  bDir0,
	uniform branch  bDir1,
	uniform branch  bSH9,
	uniform branch  bSH4,
	uniform branch	bDiffusePts,
	uniform branch	bSpecularDir,
	uniform branch	bSpecularPts,
	uniform branch	bPSWorld,
	uniform branch	bCubeEnvironmentMap,
	uniform branch	bSphereEnvironmentMap,
	uniform branch	bScatter,
	uniform int		nShadowType )
{
	OutBase	 = (PS_COMMON_ACTOR_BASE_INPUT)0;

	float4 Po;
	half3 No;
	half3x3 mTan;

	VS_GetInputs(
		In,
		OutBase,
		Po,			
		No,
		mTan,
		B( bSkinned ),
		B( bNormalMap ) );

	half3 Nw = mul( No, World );
	//Pw = mul( half4(Po.xyz, 1.f), World ).xyz;
	half3 Pw = mul( half4(Po), World ).xyz;
	
	if ( B( bSpotlight ) )
	{
		PS_COMMON_SPOTLIGHT_INPUT OutSpot = (PS_COMMON_SPOTLIGHT_INPUT)0;

		VS_Spotlight(
			WORLD_SPACE,
			false,	// bNormalMap
			Pw,
			mTan,
			OutSpot
		);

		OutAddl = (PS_COMMON_ACTOR_ADDL_INPUT)( OutSpot );
		
		OutBase.Nw.xyz = Nw;
		OutBase.P = Po;

	}
	else if ( B( bPSWorld ) && B( bNormalMap ) )
	{
		PS_COMMON_ACTOR_TBNMAT_INPUT OutTBN = (PS_COMMON_ACTOR_TBNMAT_INPUT)0;

		VS_WorldNormalMap(
			mTan,
			half3(In.Tangent.xyz),
			half3(In.Binormal.xyz),
			OutBase,
			OutTBN,
			Nw,
			B( bNormalMap ) );

		OutAddl = (PS_COMMON_ACTOR_ADDL_INPUT)( OutTBN );
	}
	else
	{
		PS_COMMON_ACTOR_LIGHTS_INPUT OutLights = (PS_COMMON_ACTOR_LIGHTS_INPUT)0;

		VS_LightingToPS(
			mTan,
			Po,
			OutBase,
			OutLights,
			Nw,
			Pw,
			//
			nPSPointLights,
			B( bIndoor ),
			B( bNormalMap ),
			bSpecularDir	!= _OFF,
			bDir0			== _ON_PS,
			bDiffusePts		== _ON_PS,
			bSpecularPts	== _ON_PS
			 );

		OutAddl = (PS_COMMON_ACTOR_ADDL_INPUT)( OutLights );
	}

	VS_Common(
		In,
		Po,	
		Nw,
		Pw,
		OutBase,
		OutPosition,
		//
		nTotalPointLights,
		nPSPointLights,
		B( bSpotlight ),					// spot light
		B( bNormalMap ),					// normal map
		B( bSH4 ),							// SH4
		B( bSH9 ),							// SH9
		B( bIndoor),						// indoor
		B( bDir0 ),							// dirlight1
		B( bDir1 ),							// dirlight2
		B( bDiffusePts ),
		B( bPSWorld ),						// PS world-space normal mapping
		B( bCubeEnvironmentMap ),			// cube environment map
		B( bSphereEnvironmentMap ),			// sphere environment map
		B( bScatter ),
		nShadowType );

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



void PS_SpecularDirInWorld(
	in		half3			Pw,
	in		half3			Nw,
	in		half2			vCoords,
	inout	Light			L,
	out		half4			vSpecularMap,
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
}









//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



void PS_LightingFromVS(
	in		PS_COMMON_ACTOR_LIGHTS_INPUT	In,
	in		half3							Pt,
	in		half3							Nt,
	in		half2							vCoords,
	inout	Light							L,
	out		half4							vSpecularMap,
	//
	uniform int								nPointLights,
	uniform bool							bAttenuateGlow,
	uniform bool							bIndoor,
	uniform bool							bDir0,
	uniform bool							bNormalMap,
	uniform bool							bSpecularDir,
	uniform bool							bDiffusePts,
	uniform bool							bSpecularPts,
	uniform bool							bSpecularMap,
	uniform bool							bSpecularViaLUT
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


	if ( bSpecularDir )
	{
		// specular dir light in object/tangent space
		half4 SpecularLightVector	= In.St;

		half3 Lt;
		if ( bNormalMap )
			Lt = normalize( SpecularLightVector.xyz );
		else
			Lt = DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_SPECULAR ];

		PS_EvaluateSpecular( Vt, Lt, Nt, DirLightsColor[ DIR_LIGHT_0_SPECULAR ], vSpecularMap, L.SpecularGloss, L.Glow, L.Specular, bAttenuateGlow, bSpecularViaLUT );
	}

	if ( bDir0 && bNormalMap )
	{
		half3 Lt					= normalize( In.Dt.xyz );

		L.Diffuse += GetDirectionalColorEX( Nt, Lt, DirLightsColor[ DIR_LIGHT_0_DIFFUSE ] );
	}


	if ( bDiffusePts || bSpecularPts )
	{
		if ( nPointLights > 0 )
		{
			half4 C = PointLightsColor[0] * half( In.Dt.w );
			LightPointEx( C, normalize( In.Dt.xyz ), Nt, Pt, Vt, vSpecularMap, L, bDiffusePts, bSpecularPts, bSpecularViaLUT );
		}
		if ( nPointLights > 1 )
		{
			half4 C = PointLightsColor[1] * half( In.St.w );
			LightPointEx( C, normalize( In.St.xyz ), Nt, Pt, Vt, vSpecularMap, L, bDiffusePts, bSpecularPts, bSpecularViaLUT );
		}
	}
}











void PS_Common(
	out		half4						Color,
	//
	in		PS_COMMON_ACTOR_BASE_INPUT	In,
	in		half3						Nw,
	in		half3						Pw,
	in		half2						vDiffuseUV,
	in		half2						vDiffuseScrollUV,
	in		half4						vSpecularMap,
	inout	Light						L,
	//
	uniform int							nPointLights,
	uniform bool						bSpotlight,
	uniform bool						bIndoor,
	uniform bool						bNormalMap,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bSH9,
	uniform bool						bSH4,
	uniform bool						bDiffusePts,
	uniform bool						bSpecularPts,
	uniform bool						bSpecularMap,
	uniform bool						bSpecularViaLUT,
	uniform bool						bSelfIllum,
	uniform bool						bCubeEnvironmentMap,
	uniform bool						bSphereEnvironmentMap,
	uniform bool						bScatter_PS,
	uniform bool						bScatter,
	uniform bool						bScroll,
	uniform bool						bManualFog,
	uniform int							nShadowType
	)
{
	L.Diffuse			+= MDR_Unpack( In.Diffuse.rgb );



	if ( bDir0 )
		L.Diffuse += GetDirectionalColorEX( Nw, DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_DIFFUSE], DirLightsColor[ DIR_LIGHT_0_DIFFUSE ].xyz );


	// these should be done after shadowing, but that creates a major inconsistency with VS application
	if ( bDir1 )
		L.Diffuse += GetDirectionalColorEX( Nw, DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_1_DIFFUSE], DirLightsColor[ DIR_LIGHT_1_DIFFUSE ].xyz );

	if ( bSH9 )
		L.Diffuse += SHEval_9_RGB( Nw );
	else if ( bSH4 )
		L.Diffuse += SHEval_4_RGB( Nw );

	if ( nShadowType != SHADOW_TYPE_NONE )
	{
		L.LightingAmount = In.EnvMapCoords.w;
		if ( L.LightingAmount > 0.f ) // pre-computed to be in darkness if <= 0.0f
		{
			if (nShadowType == SHADOW_TYPE_DEPTH)
				L.LightingAmount = ComputeLightingAmount( In.ShadowMapCoords );
			else
				L.LightingAmount = ComputeLightingAmount_Color( In.ShadowMapCoords );
		}

		if ( ! bIndoor )
			L.Specular		= lerp( 0.0f, L.Specular, L.LightingAmount ); // specular

		L.LightingAmount	*= half( gfShadowIntensity );
		L.LightingAmount	+= 1.0f - half( gfShadowIntensity );
		L.Diffuse			= lerp( 0.0f, L.Diffuse, L.LightingAmount );
	}


	half3 V = normalize( EyeInWorld - Pw );
	if ( nPointLights > 0 && ( bDiffusePts || bSpecularPts ) )
	{
		for ( int nP = 0; nP < nPointLights; nP++ )
		{
			LightPoint( nP, Nw, Pw, V, vSpecularMap, L, WORLD_SPACE, bDiffusePts, bSpecularPts, bSpecularViaLUT );
		}
	}


	if ( bCubeEnvironmentMap || bSphereEnvironmentMap )
	{
		half4 vEnvMap = EnvMapLookupCoords( Nw, normalize( half3( In.EnvMapCoords.xyz ) ), L.SpecularPower, L.SpecularAlpha, bCubeEnvironmentMap, bSphereEnvironmentMap, bSpecularMap );

		// alpha contains env map power/lerp factor
		L.Reflect.rgba = vEnvMap.rgba;
	}


	// at this point, the only thing in the glow channel is specular
	L.Glow		  *= SpecularGlow * L.SpecularPower * half( SpecularBrightness );


	half ScatterAmt = 1;
	if ( bSelfIllum )
	{
		half fMask = 1;
		if ( bScroll )
			fMask = SAMPLE_SELFILLUM( vDiffuseUV ).a;

		half4 SelfIllum	= SAMPLE_SELFILLUM( AdjustUV( SAMPLER_SELFILLUM, vDiffuseUV, vDiffuseScrollUV, bScroll ) ).rgba;
		SelfIllum.rgb	*= L.Albedo;
		SelfIllum.rgb	*= fMask * gfSelfIlluminationPower;
		L.Emissive		+= SelfIllum.rgb;
		L.Glow			+= dot( SelfIllum.rgb, SelfIllum.rgb ) * gfSelfIlluminationGlow;

		// in case we're doing scattering
		ScatterAmt		= SelfIllum.a;
		if ( bScroll )
			ScatterAmt	= fMask;
	}


	if ( bScatter )
	{
		half fScatter   = In.DiffuseMap.z;
		//half fScatterSq = In.DiffuseMap.w;

		if ( bScatter_PS )
			GetSubsurfaceScatterFactor( V, Nw, fScatter /*, fScatterSq*/ );

		if ( ! bSelfIllum )
			ScatterAmt	= SAMPLE_SELFILLUM( vDiffuseUV ).a;


		ScatterAmt *= gvSubsurfaceScatterColorPower.a;
		ScatterAmt *= fScatter;

		L.Emissive.xyz	+= gvSubsurfaceScatterColorPower.rgb * ScatterAmt;
	}


	// LDR clamp is handled in CombineLight
	Color.rgb = CombineLight( L, true, bCubeEnvironmentMap || bSphereEnvironmentMap );




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

void PS_COMMON_ACTOR( 
	in		PS_COMMON_ACTOR_BASE_INPUT	In,
	in		PS_COMMON_ACTOR_ADDL_INPUT	InAddl,
	out		half4						Color,
	//
	uniform int							nPointLights,
	uniform bool						bSpotlight,
	uniform bool						bIndoor,
	uniform bool						bNormalMap,
	uniform bool						bDir0,
	uniform bool						bDir1,
	uniform bool						bSH9,
	uniform bool						bSH4,
	uniform bool						bDiffusePts,
	uniform bool						bPSWorld,
	uniform bool						bSpecularDir,
	uniform bool						bSpecularPts,
	uniform bool						bSpecularMap,
	uniform bool						bSpecularViaLUT,
	uniform bool						bSelfIllum,
	uniform bool						bCubeEnvironmentMap,
	uniform bool						bSphereEnvironmentMap,
	uniform bool						bScatter_PS,
	uniform bool						bScatter,
	uniform bool						bScroll,
	uniform bool						bManualFog,
	uniform int							nShadowType )
{
	Light L					= (Light)0;

	half3 Nw				= normalize( half3( In.Nw.xyz ) );
	half3 Pw;

	half2 vDiffuseUV		= In.DiffuseMap.xy;	
	half2 vDiffuseScrollUV;
	half4 vSpecularMap;

	if ( bScroll )
	{
		vDiffuseScrollUV	= ScrollTextureCoordsHalf( vDiffuseUV, SCROLL_CHANNEL_DIFFUSE );
	}

	half4 DiffuseMap		= tex2D( DiffuseMapSampler, vDiffuseUV );
	L.Albedo				= DiffuseMap.rgb;
	L.Opacity				= DiffuseMap.a;

	bool bGetSpecularMap = false;

	if ( bSpotlight )
	{
		PS_COMMON_SPOTLIGHT_INPUT InSpot = (PS_COMMON_SPOTLIGHT_INPUT)InAddl;
		PS_Spotlight(
			false,	// bNormalMap
			Nw,
			InSpot,
			L
		);

		// CHB 2007.07.17 - Spot lights do not currently work with worldspace PS, specular, normal map.
		ASSERT_NOT_BRANCH_COMBINATION(bNormalMap)
		ASSERT_NOT_BRANCH_COMBINATION(bCubeEnvironmentMap)
		ASSERT_NOT_BRANCH_COMBINATION(bSphereEnvironmentMap)
		ASSERT_NOT_BRANCH_COMBINATION(bPSWorld)
		ASSERT_NOT_BRANCH_COMBINATION(bSpecularDir || bSpecularPts || bSpecularMap || bSpecularViaLUT)
	}
	else if ( bPSWorld )
	{
		// normal in world space
	
		PS_COMMON_ACTOR_TBNMAT_INPUT	InTBN = (PS_COMMON_ACTOR_TBNMAT_INPUT)InAddl;

		if ( bNormalMap )
		{
			PS_GetWorldNormalFromMap(
				vDiffuseUV,
				half3( InTBN.Tw.xyz ),
				half3( InTBN.Bw.xyz ),
				half3( In.Nw.xyz ),
				Nw );
		}

		Pw = half3( In.P.xyz );

		if ( bSpecularDir )
		{
			PS_SpecularDirInWorld(
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
		else if ( ( nPointLights && bSpecularPts ) || bCubeEnvironmentMap )
		{
			bGetSpecularMap = true;
		}
	} else 
	{
		// normal in tangent or world space

		PS_COMMON_ACTOR_LIGHTS_INPUT	InLights = (PS_COMMON_ACTOR_LIGHTS_INPUT)InAddl;

		Pw = half3( InLights.Pw.xyz );
		half3 N = Nw;
		half3 P = Pw;
		if ( bNormalMap )
		{
			N = ReadNormalMap( vDiffuseUV );
			P = half3( In.P.xyz );
		}

		if (    bSpecularDir
			|| ( bNormalMap
			    && ( ( nPointLights && ( bDiffusePts || bSpecularPts ) ) 
			        || bDir0 ) ) )  // and therefore not psworld
		{
			PS_LightingFromVS(
				InLights,
				P,
				N,
				vDiffuseUV,
				L,
				vSpecularMap,
				//
				nPointLights,
				true,					// bAttenuateGlow,
				bIndoor,
				bDir0,
				bNormalMap,
				bSpecularDir,
				bDiffusePts,
				bSpecularPts,
				bSpecularMap,
				bSpecularViaLUT
				 );
		}
		else if ( ( nPointLights && bSpecularPts ) || bCubeEnvironmentMap )
		{
			bGetSpecularMap = true;
		}
	}


	if ( bGetSpecularMap )
	{
		PS_GetSpecularMap(
			vDiffuseUV,
			vSpecularMap,
			L.SpecularPower,
			L.SpecularAlpha,
			L.SpecularGloss,
			bSpecularMap );
	}


//	Color.rgb = L.Albedo.xyz;
//	Color.a = GLOW_MIN;
//	return;



	if ( bDir0 )
		bDir0			= bPSWorld || ! bNormalMap;
	if ( bDiffusePts )
		bDiffusePts		= bPSWorld || ! bNormalMap;
	if ( bSpecularPts )
		bSpecularPts	= bPSWorld || ! bNormalMap;


	PS_Common(
		Color,
		//
		In,
		Nw,
		Pw,
		vDiffuseUV,
		vDiffuseScrollUV,
		vSpecularMap,
		L,
		//
		nPointLights,
		bSpotlight,
		bIndoor,
		bNormalMap,
		bDir0,
		bDir1,
		bSH9,
		bSH4,
		bDiffusePts,
		bSpecularPts,
		bSpecularMap,
		bSpecularViaLUT,
		bSelfIllum,
		bCubeEnvironmentMap,
		bSphereEnvironmentMap,
		bScatter_PS,
		bScatter,
		bScroll,
		bManualFog,
		nShadowType );
}
