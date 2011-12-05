// Common functions for all mythos background techniques


#ifdef PRIME


//--------------------------------------------------------------------------------------------
// INPUT STRUCTS
//--------------------------------------------------------------------------------------------

// PIXEL SHADER INPUT
// -------------------

struct PS_COMMON_BACKGROUND_INPUT
{
    float4 Ambient				: COLOR0;
    float4 LightMapDiffuse 		: TEXCOORD0; 
    float2 DiffuseMap2 			: TEXCOORD1; 
    float3 Nw					: TEXCOORD2;
    float3 Pw					: TEXCOORD3;
	float4 ShadowMapCoords		: TEXCOORD4;
	float3 EnvMapCoords			: TEXCOORD5;
    float  Fog					: FOG;
};


//--------------------------------------------------------------------------------------------
// SHADERS
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------

void VS_COMMON_BACKGROUND ( 
	in		float4						Po,
	in		half3						No,
	in		half4						LightMapDiffuse,
	in		half2						DiffuseMap2,
	inout	PS_COMMON_BACKGROUND_INPUT	Out,
	out		float4						Prhw,
	//
	uniform bool						bDiffuseMap2,
	uniform bool						bSphericalHarmonics,
	uniform bool						bManualFog )
{
	// this is done regardless of shadowtype, because it's used as well for the projected lighting map
	ComputeShadowCoords( Out.ShadowMapCoords, Po );


	Prhw				= mul( Po, WorldViewProjection ); 
	Out.Pw				= mul( Po, World );
	Out.Nw				= normalize( mul( No, half4x4( World ) ) );
	Out.LightMapDiffuse	= LightMapDiffuse;
	Out.DiffuseMap2		= DiffuseMap2;

	Out.Ambient			= GetAmbientColor();
	if (bSphericalHarmonics)
	{
		Out.Ambient.xyz += SHEval_9_RGB( Out.Nw );
	}
	Out.Ambient.xyz		= saturate( Out.Ambient.xyz );
	Out.Ambient.w		= 0.0f;

	if ( bManualFog )
	{
		FOGMACRO_OUTPUT( Out.Ambient.w, Po )
		Out.Ambient.w += gfFogFactor; // either 1 or 0 -- this forces the factor to > 1 before the sat == unfogged
		Out.Ambient.w = saturate( Out.Ambient.w );
	}
	else
	{
		FOGMACRO( Po )
	}
}


//--------------------------------------------------------------------------------------------
void PS_COMMON_BACKGROUND ( 
	in		PS_COMMON_BACKGROUND_INPUT	In,
	out		half4						Color,
	out		half2						fSpecularPowerGloss,
	out		half3						Nw,
	//
	uniform bool						bSpecular,
	uniform bool						bSpecularViaLUT,
	uniform bool						bDiffuseMap2,
	uniform bool						bManualFog,
	uniform int							nShadowType )
{
	half2 vIn_LightMapCoords	= In.LightMapDiffuse.xy;
	half2 vIn_DiffuseMapCoords	= In.LightMapDiffuse.zw;	

    //fetch base color
	Color					= tex2D( DiffuseMapSampler, vIn_DiffuseMapCoords );
	half3	cFinalLight		= In.Ambient;

	if ( bDiffuseMap2 || gbDiffuseMap2 )
	{
		half2 vDiffMap2Coords = In.DiffuseMap2;
		half4 cDiffMap2		  = tex2D( DiffuseMapSampler2, half2( vDiffMap2Coords ) );
		Color.xyz			  = lerp( Color.xyz, cDiffMap2.xyz, cDiffMap2.w );
	}


	half fLightingAmount = 1.0f;
	if (nShadowType == SHADOW_TYPE_DEPTH)
	{
		fLightingAmount = tex2D( ShadowMapDepthSampler, In.ShadowMapCoords.xy ).r;
	}
	else if (nShadowType == SHADOW_TYPE_COLOR)
	{
		fLightingAmount = ComputeLightingAmount_Color( In.ShadowMapCoords );
	}


	half  fFinalAlpha	= 0;
	half  fGlow			= 0;
	half3 vBrighten		= 0;
	half3 vOverBrighten = 0;
	fSpecularPowerGloss	= 0;

	Nw = normalize( half3(In.Nw) );

	if ( bSpecular || gbSpecular )
	{
		half4 vSpecularMap		= tex2D( SpecularMapSampler, vIn_DiffuseMapCoords );
		fSpecularPowerGloss.x	= length( vSpecularMap.xyz );
		fSpecularPowerGloss.y	= vSpecularMap.a;

		vSpecularMap.xyz		*= half( SpecularBrightness );
		vSpecularMap.xyz		+= half( SpecularMaskOverride );  // should be 0.0f unless there is no specular map

		half3 Vw				= normalize( EyeInWorld - half3( In.Pw ) );

		half3 Lw				= DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_SPECULAR ];
		half4 C					= DirLightsColor[ DIR_LIGHT_0_SPECULAR ];

		half fFade				= 0.05f;
		//fFade = 1.0f;

		if ( bSpecularViaLUT )
		{
			PS_EvaluateSpecularBackgroundLUT( Vw, Lw, Nw, C, vSpecularMap, fGlow, vOverBrighten, true, fFade );
		}
		else
		{
			half fGloss		= PS_GetSpecularGloss( vSpecularMap );
			PS_EvaluateSpecularBackground( Vw, Lw, Nw, C, vSpecularMap, fGloss, fGlow, vOverBrighten, true, fFade );
		}

		fGlow		  *= SpecularGlow * fSpecularPowerGloss.x * half( SpecularBrightness );
		fFinalAlpha	  += fGlow;
	}

	half3 cLightingMap = tex2D( ParticleLightMapSampler, In.ShadowMapCoords );
	cLightingMap -= .5;
	cFinalLight += cLightingMap;

	if ( nShadowType != SHADOW_TYPE_NONE )
	{
		cFinalLight = lerp( cFinalLight, 0.0f, saturate( gfShadowIntensity - fLightingAmount ) );
	}
//	if ( gbLightMap )
//	{
		half3 cLightMap = tex2D( LightMapSampler, vIn_LightMapCoords );
		cFinalLight		*= cLightMap;
//	}
//	else
//	{
//		cFinalLight *= .5f;
//	}
	Color.xyz		*= cFinalLight;
	Color.xyz		*= 2;
	Color.xyz		+= vOverBrighten;
	Color.w			*= ( half( 1.0f ) - gfAlphaForLighting ) + ( saturate( fFinalAlpha + GLOW_MIN ) * gfAlphaForLighting );
	Color.w	 *= gfAlphaConstant;
	if ( bManualFog )
		APPLY_FOG( Color.xyz, In.Ambient.w );
}



	#endif // PRIME