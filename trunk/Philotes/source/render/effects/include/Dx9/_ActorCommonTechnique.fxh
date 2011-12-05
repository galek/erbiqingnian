//
// _ActorCommonTechnique - header file for actor shader technique
//

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void VS_ACTOR ( 
	out PS_COMMON_ACTOR_BASE_INPUT		OutBase,
	out PS_COMMON_ACTOR_ADDL_INPUT		OutAddl,
	out float4							OutPosition : POSITION,
    out float							Fog			: FOG,
	in	VS_ANIMATED_INPUT				In, 
	//
	SHADER_BRANCH_TYPE,
	uniform int		nTotalPointLights,
	uniform int		nPSPointLights,
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
	uniform branch	bManualFog,
	uniform int		nShadowType )
{
	VS_COMMON_ACTOR(
		In,
		//
		OutBase,
		OutAddl,
		OutPosition,
		//
		nTotalPointLights,
		nPSPointLights,
		_BRANCH_PARAM_NAME,
		bSkinned,
		bSpotlight,
		bIndoor,
		bNormalMap,
		bDir0,
		bDir1,
		bSH9,
		bSH4,
		bDiffusePts,
		bSpecularDir,
		bSpecularPts,
		bPSWorld,
		bCubeEnvironmentMap,
		bSphereEnvironmentMap,
		bScatter,
		nShadowType );


	if ( bManualFog )
	{
		FOGMACRO_OUTPUT( OutBase.Diffuse.w, half3(In.Position.xyz) )
		OutBase.Diffuse.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
		OutBase.Diffuse.w = saturate( OutBase.Diffuse.w );
	}
	else
	{
		FOGMACRO_OUTPUT( Fog, half3(In.Position.xyz) )
	}
}



//--------------------------------------------------------------------------------------------
void PS_ACTOR( 
	in PS_COMMON_ACTOR_BASE_INPUT		In,
	in PS_COMMON_ACTOR_ADDL_INPUT		InAddl,
	out	float4							OutColor		: COLOR,	
	//
	SHADER_BRANCH_TYPE,
	uniform int		nPSPointLights,
	uniform branch	bSpotlight,
	uniform branch	bIndoor,
	uniform branch	bNormalMap,
	uniform branch	bDir0,
	uniform branch	bDir1,
	uniform branch	bSH9,
	uniform branch	bSH4,
	uniform branch	bDiffusePts,
	uniform branch	bSpecularDir,
	uniform branch	bSpecularPts,
	uniform branch	bSpecularMap,
	uniform branch	bSpecularViaLUT,
	uniform branch	bSelfIllum,
	uniform branch	bCubeEnvironmentMap,
	uniform branch	bSphereEnvironmentMap,
	uniform branch	bScatter,
	uniform branch	bScroll,
	uniform branch	bPSWorld,
	uniform branch	bManualFog,
	uniform int		nShadowType )
{
	half4 Color;

	// validate certain branch combinations
	ASSERT_NOT_BRANCH_COMBINATION	( B( bDiffusePts )  &&  ! B( bNormalMap )  &&  ( bSH4 || bSH9 ) )
	ASSERT_NOT_BRANCH_COMBINATION	( ( B( bDiffusePts ) || B( bSpecularPts ) )  &&  B( bNormalMap )  &&  ! B( bPSWorld )  &&  nPSPointLights > 2 )
	if ( B( bPSWorld ) )
	{
		ASSERT_NOT_BRANCH_COMBINATION	( B( bSpecularDir ) && B( bSpecularPts ) )
		ASSERT_NOT_BRANCH_COMBINATION	( B( bSpecularDir ) && B( bDiffusePts ) )
	}
	ASSERT_NOT_BRANCH_COMBINATION	( B( bCubeEnvironmentMap ) && B( bSphereEnvironmentMap ) )

	PS_COMMON_ACTOR(
		In,
		InAddl,
		Color,
		//
		nPSPointLights,
		B( bSpotlight ),
		B( bIndoor ),
		B( bNormalMap ),
		B( bDir0 ),
		B( bDir1 ),
		B( bSH9 ),
		B( bSH4 ),
		B( bDiffusePts ),
		B( bPSWorld ),
		B( bSpecularDir ),
		B( bSpecularPts ),
		B( bSpecularMap ),
		B( bSpecularViaLUT ),
		B( bSelfIllum ),
		B( bCubeEnvironmentMap ),
		B( bSphereEnvironmentMap ),
		B( bScatter ),
		 ( bScatter != _OFF ),
		B( bScroll ),
		B( bManualFog ),
		nShadowType );


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
#	define ACTOR_VS		vs_4_0
#	define ACTOR_PS		ps_4_0
#	define ACTOR_SV		SHADER_VERSION_33
#	define MANUAL_FOG	_ON
#else
#	if (SHADER_VERSION == PS_3_0)
#		define ACTOR_VS		vs_3_0
#		define ACTOR_PS		ps_3_0
#		define ACTOR_SV		SHADER_VERSION_33
#		define MANUAL_FOG	_ON
#	elif (SHADER_VERSION == PS_2_0)
#		define ACTOR_VS		vs_2_0
#		define ACTOR_PS		ps_2_b
#		define ACTOR_SV		SHADER_VERSION_22
#		define MANUAL_FOG	_OFF
#	else
#		error Missing SHADER_VERSION define!
#	endif
#endif






#define TECHNIQUE_ACTOR(totalpoints,pspoints,shadowtype,skinned,spotlight,indoor,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,specularmap,specularlut,specularpts,diffusepts,psworldnmap,cubemap,spheremap,scatter,scroll) \
    DECLARE_TECH TActor_##totalpoints##pspoints##_##shadowtype##_##skinned##spotlight##indoor##normalmap##selfillum##dir0##dir1##sh9##sh4##speculardir##specularmap##specularlut##specularpts##diffusepts##psworldnmap##cubemap##spheremap##scatter##scroll < \
	ACTOR_SV \
	int PointLights			= totalpoints;					\
	int ShadowType			= shadowtype;					\
	int Skinned				= skinned;						\
	int SpotLight			= spotlight;					\
	int Indoor				= indoor;						\
	int NormalMap			= normalmap;					\
	int SelfIllum			= selfillum;					\
	int Specular			= speculardir || specularpts;	\
	int SpecularLUT			= specularlut;					\
	int SphericalHarmonics	= sh9 || sh4;					\
	int WorldSpaceLight		= psworldnmap;					\
	int CubeEnvMap			= cubemap;						\
	int SphereEnvMap		= spheremap;					\
	int Scatter				= scatter;						\
	int ScrollUV			= scroll;						\
	> { pass Main	{ \
		COMPILE_SET_VERTEX_SHADER( ACTOR_VS, VS_ACTOR(		_ON_VS, \
															totalpoints,  \
															pspoints, \
															skinned, \
															spotlight, \
															indoor, \
															normalmap, \
															dir0, \
															dir1, \
															sh9, \
															sh4, \
															diffusepts, \
															speculardir, \
															specularpts, \
															psworldnmap, \
															cubemap, \
															spheremap, \
															scatter, \
															MANUAL_FOG, \
															shadowtype ) ); \
		COMPILE_SET_PIXEL_SHADER( ACTOR_PS, PS_ACTOR(		_ON_PS, \
															pspoints, \
															spotlight, \
															indoor, \
															normalmap, \
															dir0, \
															dir1, \
															sh9, \
															sh4, \
															diffusepts, \
															speculardir, \
															specularpts, \
															specularmap, \
															specularlut, \
															selfillum, \
															cubemap, \
															spheremap, \
															scatter, \
															scroll, \
															psworldnmap, \
															MANUAL_FOG, \
															shadowtype ) ); \
		} } 



#else

#error Need #define PRIME!

#endif