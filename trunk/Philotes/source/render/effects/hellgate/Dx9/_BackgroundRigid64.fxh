//
// _backgroundrigid64.fxh -- shaders for background effects
//



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void VS30_BACKGROUND_64BYTES ( 
	out PS_COMMON_BG_BASE_INPUT		OutBase,
	out PS_COMMON_BG_ADDL_INPUT		OutAddl,
	out float4						OutPosition : POSITION,
    out float						Fog			: FOG,
	in	VS_BACKGROUND_INPUT_64		In, 
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
	uniform branch	bManualFog,
	uniform int		nShadowType )
{
	VS_COMMON_BACKGROUND(
		float4( In.Position, 1.f ),
		In.LightMapDiffuse,
		In.Normal,
		In.Tangent,
		In.Binormal,
		In.DiffuseMap2,
		//
		OutBase,
		OutAddl,
		OutPosition,
		//
		nPointLights,
		_BRANCH_PARAM_NAME,
		bSpotlight,
		bIndoor,
		bDiffuseMap2,
		bNormalMap,
		bDir0,
		bDir1,
		bSH9,
		bSH4,
		bHemisphereLight,
		bSpecularDir,
		bSpecularPts,
		bPSWorldNMap,
		bCubeEnvironmentMap,
		nShadowType );


	if ( bManualFog )
	{
		FOGMACRO_OUTPUT( OutBase.Diffuse.w, half3(In.Position.xyz) )
		OutBase.Diffuse.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
		OutBase.Diffuse.w = saturate( OutBase.Diffuse.w );
	}
	else
	{
		FOGMACRO_OUTPUT( Fog, In.Position.xyz )
	}
}





//--------------------------------------------------------------------------------------------
void PS30_BACKGROUND( 
	in PS_COMMON_BG_BASE_INPUT		In,
	in PS_COMMON_BG_ADDL_INPUT		InAddl,
	out	float4						OutColor		: COLOR,
	//
	SHADER_BRANCH_TYPE,
	uniform branch	bSpotlight,
	uniform branch	bIndoor,
	uniform branch	bNormalMap,
	uniform branch	bDir0,
	uniform branch	bDir1,
	uniform branch	bSH9,
	uniform branch	bSH4,
	uniform branch	bHemisphereLight,
	uniform branch	bSpecularDir,
	uniform branch	bSpecularPts,
	uniform branch	bSpecularMap,
	uniform branch	bSpecularViaLUT,
	uniform branch	bDiffuseMap2,
	uniform branch	bLightMap,
	uniform branch	bSelfIllum,
	uniform branch	bCubeEnvironmentMap,
	uniform branch	bScroll,
	uniform branch	bPSWorldNMap,
	uniform branch	bManualFog,
	uniform int		nShadowType,
	uniform bool	bSelfShadow )
{
	half4 Color;

	PS_COMMON_BACKGROUND(
		In,
		InAddl,
		Color,
		//
		_BRANCH_PARAM_NAME,
		B( bSpotlight ),
		B( bIndoor ),
		B( bNormalMap ),
		B( bDir0 ),
		B( bDir1 ),
		B( bSH9 ),
		B( bSH4 ),
		B( bHemisphereLight ),
		bPSWorldNMap,
		B( bSpecularDir ),
		B( bSpecularPts ),
		B( bSpecularMap ),
		B( bSpecularViaLUT ),
		B( bDiffuseMap2 ),
		B( bLightMap ),
		B( bSelfIllum ),
		bCubeEnvironmentMap,
		B( bScroll ),
		B( bManualFog ),
		nShadowType,
		bSelfShadow );


/*
		if ( !bSpotlight )
		{
			half3 L;

			vSpecularMap.xyz		*= half( SpecularBrightness );
			vSpecularMap.xyz		+= half( SpecularMaskOverride );  // should be 0.0f unless there is no specular map

			half3 V = normalize( half3( In.EyePosition.xyz ) - half3( In.ObjPosition.xyz ) );
			half fGloss	= PS_GetSpecularGloss( vSpecularMap );

			if ( ! bIndoor )
			{
				if ( gbDirectionalLight )
				{
					half3 vDirDiff = normalize( half3( In.DiffuseLightVector.xyz ) );
					half3 vDirSpec = normalize( half3( In.SpecularLightVector.xyz ) );

					// directional light
					PS_EvaluateSpecular( V, vDirSpec, N, gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_SPECULAR ].C, vSpecularMap, fGloss, fGlow, vOverBrighten, true );
					cFinalLight.rgb += GetDirectionalColorEX( N, vDirDiff, gtDirLights[ OBJECT_SPACE ][ DIR_LIGHT_0_DIFFUSE ].C );
				}
			}
			else
			{
				// specular point light
				L = normalize( half3( In.SpecularLightVector.xyz ) );
				half4 vColor = In.SpecularColor;
				PS_SpecularCoverBoundary( In.SpecularLightVector.w, vColor );
				//vOverBrighten = vColor.rgb;
				//vOverBrighten = In.SpecularLightVector.www;
				PS_EvaluateSpecular( V, L, N, vColor, vSpecularMap, fGloss, fGlow, vOverBrighten, true );
			}

			// finalize specular
			fGlow.x		  *= SpecularGlow * fSpecularPower * half( SpecularBrightness );
			//vBrighten   += fGlow; // add the glow amount as a white light contribution
			fFinalAlpha.x += fGlow;
		}
*/

	AppControlledAlphaTest( Color );

	OutColor = Color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////


#ifdef PRIME


#ifdef ENGINE_TARGET_DX10
#	define BG_VS		vs_4_0
#	define BG_PS		ps_4_0
#	define BG_SV		SHADER_VERSION_33
#	define MANUAL_FOG	_ON
#else
#	if (SHADER_VERSION == PS_3_0)
#		define BG_VS		vs_3_0
#		define BG_PS		ps_3_0
#		define BG_SV		SHADER_VERSION_33
#		define MANUAL_FOG	_ON
#	elif (SHADER_VERSION == PS_2_0)
#		define BG_VS		vs_2_0
#		define BG_PS		ps_2_b
#		define BG_SV		SHADER_VERSION_22
#		define MANUAL_FOG	_OFF
#	else
#		error Missing SHADER_VERSION define!
#	endif
#endif



#ifndef SELF_SHADOW
#define SELF_SHADOW 0
#endif

#define TECHNIQUE64(points,shadowtype,spotlight,indoor,diffuse2,normalmap,lightmap,selfillum,dir0,dir1,sh9,sh4,hemilight,speculardir,specularmap,specularlut,specularpts,psworldnmap,cubemap,scroll) \
    DECLARE_TECH TBackground_64_##points##_##shadowtype##_##spotlight##indoor##diffuse2##normalmap##lightmap##selfillum##dir0##dir1##sh9##sh4##hemilight##speculardir##specularmap##specularlut##specularpts##psworldnmap##cubemap##scroll < \
	BG_SV \
	int VertexFormat		= VERTEX_FORMAT_BACKGROUND_64BYTES; \
	int PointLights			= points;						\
	int ShadowType			= shadowtype;					\
	int SpotLight			= spotlight;					\
	int Indoor				= indoor;						\
	int DiffuseMap2			= diffuse2;						\
	int NormalMap			= normalmap;					\
	int LightMap			= lightmap;						\
	int SelfIllum			= selfillum;					\
	int Specular			= speculardir || specularpts;	\
	int SpecularLUT			= specularlut;					\
	int SphericalHarmonics	= sh9 || sh4;					\
	int WorldSpaceLight		= psworldnmap;					\
	int CubeEnvMap			= cubemap;						\
	int ScrollUV			= scroll;						\
	> { pass Main	{ \
		COMPILE_SET_VERTEX_SHADER( BG_VS, VS30_BACKGROUND_64BYTES( points,  \
																	_ON_VS, \
																	spotlight, \
																	indoor, \
																	diffuse2, \
																	normalmap, \
																	dir0, \
																	dir1, \
																	sh9, \
																	sh4, \
																	hemilight, \
																	speculardir, \
																	specularpts, \
																	psworldnmap, \
																	cubemap, \
																	MANUAL_FOG, \
																	shadowtype ) ); \
		COMPILE_SET_PIXEL_SHADER( BG_PS, PS30_BACKGROUND(	_ON_PS, \
															spotlight, \
															indoor, \
															normalmap, \
															dir0, \
															dir1, \
															sh9, \
															sh4, \
															hemilight, \
															speculardir, \
															specularpts, \
															specularmap, \
															specularlut, \
															diffuse2, \
															lightmap, \
															selfillum, \
															cubemap, \
															scroll, \
															psworldnmap, \
															MANUAL_FOG, \
															shadowtype, \
															SELF_SHADOW ) ); \
		} \
	  } 



#else

#error Need #define PRIME!

#endif