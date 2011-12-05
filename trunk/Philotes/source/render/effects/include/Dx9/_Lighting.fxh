// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

#ifndef __LIGHTING_FX__
#define __LIGHTING_FX__

//
// Lighting structures and functions commonly used across fx files
//

// --------------------------------------------------------------------------------------
// MACROS
// --------------------------------------------------------------------------------------

// set the wrap-around lighting dp3 bias
#define DOT_LIGHTING_BIAS( fDot )	( ( fDot + half(0.2f) ) * half(0.833333f) )

// --------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// FUNCTIONS
//--------------------------------------------------------------------------------------------

half3 MDR_Pack(
	in half3 Color )
{
	return Color * half( 0.5f );
}

half3 MDR_Unpack(
	in half3 Color )
{
	return Color * half( 2.f );
}

half MDR_Pack_Scalar(
	in half Value )
{
	return Value * half( 0.5f );
}

half MDR_Unpack_Scalar(
	in half Value )
{
	return Value * half( 2.f );
}

half LDR_Clamp(
	inout half3 Color )
{
	// this performs a scale by the max value, as opposed to a simple saturate

	half fMax = max( Color.r, max( Color.g, Color.b ) );
	fMax = max( fMax, 1.0f );
	Color /= fMax;

	// return an additional glow amount as "bloom"
	fMax = ( fMax - 1 ) * 0.5f;
	return ( fMax * fMax );
}

//--------------------------------------------------------------------------------------------

half3 CombineLight(
	inout Light	L,
	uniform bool bLDRClamp,
	uniform bool bReflect )
{
	half3 D = L.Albedo * L.Diffuse;

	if ( bReflect )
		D = lerp( D, L.Reflect.rgb, L.Reflect.a );

	if ( bLDRClamp )
		L.Glow += LDR_Clamp( D.rgb );

	return  D + L.Specular + L.Emissive;
}

//--------------------------------------------------------------------------------------------

void GetSubsurfaceScatterFactor(
	in half3 V,
	in half3 N,
	out half fScatter
	/*
	,
		out half fScatterSq*/
	 )
{
	fScatter = saturate( half(1.f) - dot( N, V ) );
	half fScatterSq = fScatter * fScatter;
	fScatter = lerp( fScatter, fScatterSq, gfSubsurfaceScatterSharpness );
}

//--------------------------------------------------------------------------------------------

void PS_GetWorldNormalFromMap(
	in	half2 Tex,
	in	half3 T,
	in	half3 B,
	in	half3 N,
	out half3 Nw )
{
	half3 Nt = ReadNormalMap( Tex );

/*
	half3x3 mTan = { normalize( T ), normalize( B ), normalize( N ) };

    Nw = mul( Nt, mTan );

	// It seems like this shouldn't be necessary, but it is.
	Nw = normalize(Nw);
//*/

	Nw = Nt.x * normalize(T)  +  Nt.y * normalize(B)  +  Nt.z * normalize(N);
	Nw = normalize(Nw);
}

//--------------------------------------------------------------------------------------------

// L = distance vector
// F.x = base light brightness
// F.y = linear falloff coefficient
half GetFalloffBrightness( half3 L, half2 F )
{
	half ld = length( L );
	half lightBrightness = F.x - ld * F.y;
	return saturate ( lightBrightness );
}

//--------------------------------------------------------------------------------------------

half4 GetColorBrightness( half3 L, half2 F, half4 C )
{
	return C * GetFalloffBrightness( L, F );
}

//--------------------------------------------------------------------------------------------
half PSGetDiffuseBrightnessNoBias( half3 L, half3 N )
{
	return saturate( dot( N, L ) );
}

//--------------------------------------------------------------------------------------------
half PSGetDiffuseBrightness( half3 L, half3 N )
{
	return PSGetDiffuseBrightnessNoBias( L, N );

	//return saturate( DOT_LIGHTING_BIAS( dot( N, L ) ) );
}

//--------------------------------------------------------------------------------------------
half PSGetSpecularBrightness( half3 V, half3 N )
{
	//return PSGetDiffuseBrightnessNoBias( L, N );
	//return 1.f;
	return half( dot( N, V ) > 0 );
}

//--------------------------------------------------------------------------------------------
half4 GetPointLightColor( 	half3 Pv,		// vertex position
							half3 Pl,		// light position
							half3 N,		// normal
							half4 C,		// color
							half2 F )		// falloff
{
	half3 L = Pl - Pv;
	half4 lightColorBrightness = GetColorBrightness( L, F, C );
	L = normalize( L );
	return lightColorBrightness * PSGetDiffuseBrightness( L, N );
}

//--------------------------------------------------------------------------------------------

half4 GetAmbientColor()
{
	return LightAmbient;
}

//--------------------------------------------------------------------------------------------

half4 GetDirectionalColor( half3 N, int nDir, uniform int nSpace )
{
	half4 C = DirLightsColor[ nDir ];

	half3 L;
	if ( nSpace == OBJECT_SPACE )
		L = DirLightsDir(OBJECT_SPACE)[ nDir ].xyz;
	else
		L = DirLightsDir(WORLD_SPACE)[ nDir ].xyz;
		
	return C * saturate( dot( N, L ) );

	//return tDL.C * saturate( DOT_LIGHTING_BIAS( dot( N, tDL.L ) ) );
}

//--------------------------------------------------------------------------------------------

half3 GetDirectionalColorEX( half3 N, half3 L, half3 Color )
{
	return Color * PSGetDiffuseBrightness( L, N );
}

//--------------------------------------------------------------------------------------------
half GetSpecular20PowNoNormalize( half3 H, half3 N, half Pow )
{
	half specular = saturate( dot( H, N ) );
	return pow( specular, Pow );
}

//--------------------------------------------------------------------------------------------
half4 CombinePointLights(
	in half3 P,
	in half3 N,
	uniform int nLightCount,
	uniform int nLightStart,
	uniform int nSpace )
{
	half4 C = 0; 
	for ( int i = nLightStart; i < nLightCount; i++ )
	{
		if ( nSpace == OBJECT_SPACE )
			C += GetPointLightColor( P, PointLightsPos(OBJECT_SPACE)[ i ], N, PointLightsColor[ i ], PointLightsFalloff(OBJECT_SPACE)[ i ] );
		else
			C += GetPointLightColor( P, PointLightsPos(WORLD_SPACE)[ i ], N, PointLightsColor[ i ], PointLightsFalloff(WORLD_SPACE)[ i ] );
	}
	return C;
}

//--------------------------------------------------------------------------------------------

void VectorsTransform( inout half3 v1, inout half3 v2, inout half3 v3, in half4x4 mtx )
{
    v1 = mul(v1,mtx);
    v2 = mul(v2,mtx);
    v3 = mul(v3,mtx);
}

half3 TangentToWorld(
	in half3 T,
	in half3 B,
	in half3 N,
	in half3 Nt )
{
	half3x3 mTan = { normalize( T ), normalize( B ), normalize( N ) };
    return mul( Nt, mTan );
}


//--------------------------------------------------------------------------------------------

void PS_DirectionalInWorldSB(
	int nDirLight,
	half3 N,
	half3 V,
	half4 vSpecularMap,
	inout half3 vBrighten,
	inout half fGlow,
	inout half3 vOverBrighten,
	bool bSpecular )
{
	half3 L					= DirLightsDir(WORLD_SPACE)[ nDirLight ];
	half4 C					= DirLightsColor[ nDirLight ];

	// diffuse
	half Kd					= PSGetDiffuseBrightness( L, N );
	vBrighten				+= Kd * C.xyz;

	// specular
	if ( bSpecular )
	{
		half3 H					= normalize( V + L );
		half Ks					= saturate( dot( H, N ) );
		Ks						= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, Ks ) ).x;

		// not attenuating dir-light specular
		half3 vPointSpecular	= vSpecularMap.xyz * ( Ks * PSGetSpecularBrightness( V, N ) );
		fGlow					+= Ks; // * C.w; // modulating glow factor by intensity
		vOverBrighten			+= C.xyz * vPointSpecular;
	}
}

//--------------------------------------------------------------------------------------------

void VS_PrepareSpotlight(
	out half3 Vs,	// spotlight to pos
	out half3 Ns,	// spotlight normal
	out half4 Cs,	// spotlight color
	in  half3 Pv,	// vertex pos... possibly too much precision loss?
	in  half3x3  objToTangentSpace,
	uniform bool bNormalMap,
	uniform int nSpace )
{
	half3 Ps;
	half2 Fs;

	if ( nSpace == OBJECT_SPACE )
	{
		Ps = SpotLightsPos(OBJECT_SPACE)[0];
		Fs = SpotLightsFalloff(OBJECT_SPACE)[0];
		Ns = SpotLightsNormal(OBJECT_SPACE)[0];
	}
	else
	{
		Ps = SpotLightsPos(WORLD_SPACE)[0];
		Fs = SpotLightsFalloff(WORLD_SPACE)[0];
		Ns = SpotLightsNormal(WORLD_SPACE)[0];
	}

	half4 SLC	= SpotLightsColor[0];

	// Vector between light position and vertex.
	Vs			= Pv - Ps;

	// Get distance attenuation value.
	half Kd		= GetFalloffBrightness( Vs, Fs );

	// Attenuate by light color's alpha component (?).
	// Commented out for now.
//	Kd			*= SLC.w;

	// Attenuate light color based on distance attenuation.
//	Cs.xyz		= SLC.xyz * Kd;
	Cs			= SLC * Kd;

	// Currently unused.
//	Cs.w		= Kd;

	if ( bNormalMap )
	{
		Vs	= mul( objToTangentSpace, Vs );
		Ns	= mul( objToTangentSpace, Ns );
	}

	// Negate here to save a negation in the
	// pixel shader at the pixel normal dot.
	Vs = -Vs;
	Ns = -Ns;
}

//--------------------------------------------------------------------------------------------

half3 PS_EvalSpotlight(
	in half3 Vs,			// spotlight to pos
	in half3 Ns,			// spotlight normal
	in half3 Cs,			// spotlight color
	in half3 Np,			// pixel normal
	uniform int nSpace )
{
	half2 vFocus;
	if ( nSpace == OBJECT_SPACE )
		vFocus			= SpotLightsFalloff(OBJECT_SPACE)[0].zw;
	else
		vFocus			= SpotLightsFalloff(WORLD_SPACE)[0].zw;

	half3 SL_N			= normalize( Ns );
	half3 SL_V			= normalize( Vs );
	
	// Angle of incidence between light and pixel.
	half fDot			= saturate( dot( SL_N, SL_V ) );

	// Compute umbra falloff.
	fDot				= saturate( ( fDot + vFocus.x ) * vFocus.y );

	// "Shape" of light comes from 1D texture.
	half fPower			= tex1D( SpecularLookupSampler1D, fDot ).b;

	// Dot against pixel normal.
	fPower				*= saturate( dot( Np, SL_V ) );

	return Cs * fPower;
}

//--------------------------------------------------------------------------------------------

void VS_GetRigidNormal(
	in half3 iN,
	out half3 N )
{
	N = RIGID_NORMAL_V( iN );
}

//--------------------------------------------------------------------------------------------

void VS_GetRigidNormalObjToTan(
	in half3 iN,
	in half3 iT,
	in half3 iB,
	out half3x3 mObjToTan,
	out half3 N,
	uniform bool bNormalMap )
{
	N = RIGID_NORMAL_V( iN );
	mObjToTan = 0;
	if ( bNormalMap || gbNormalMap )
	{
		mObjToTan[0].xyz = RIGID_TANGENT_V ( iT );
		mObjToTan[1].xyz = RIGID_BINORMAL_V( iB );
		mObjToTan[2].xyz = N;
	}
}

//--------------------------------------------------------------------------------------------

void VS_GetAnimatedNormal(
	in half3 iN,
	in half3 iT,
	in half3 iB,
	out half3x3 mObjToTan,
	out half3 N,
	uniform bool bNormalMap )
{
	N = ANIMATED_NORMAL_V( iN );
	if ( bNormalMap || gbNormalMap )
	{
		mObjToTan[0].xyz = ANIMATED_TANGENT_V ( iT );
		mObjToTan[1].xyz = ANIMATED_BINORMAL_V( iB );
		mObjToTan[2].xyz = N;
	}
}

//--------------------------------------------------------------------------------------------

void GetSpecularKs(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half  fGloss,
	out half3 H,
	out half Ks )
{
	H						= normalize( V + L );
	half fDot				= max( 0, dot( H, N ) );
	Ks						= pow( fDot, fGloss );
//	Ks *= max(0,dot(H,V));
}

//--------------------------------------------------------------------------------------------

void GetFresnel(
	in half3 V,	// view or light vector
	in half3 H, // half vector
	out half f )
{
	f = 1.0 - dot( V, H );
	f *= f;		// squared
	f *= f;		// ^4
}

//--------------------------------------------------------------------------------------------
void PickLightEx(	out half4 LightVector, 
					out half4 LightColor, 
					in half3 P, 
					in half3 N,
					in half3 LightInObject0,
					in half3 LightInObject1,
					in half2 LightFalloff0,
					in half2 LightFalloff1,
					in half4 LightColor0,
					in half4 LightColor1,
					uniform bool bPositionOnly,
					uniform bool bEvaluateKs )
{
	// compute the first light's brightness and position
	half3 L0 = LightInObject0 - P;
	half  kL0 = GetFalloffBrightness( L0, LightFalloff0 );  // falloff to pos

	// compute the second light's brightness and position
	half3 L1 = LightInObject1 - P;
	half  kL1 = GetFalloffBrightness( L1, LightFalloff1 );  // falloff to pos

	half fL0 = kL0;
	half fL1 = kL1;

    // pick the brightest light
    if ( fL0 > fL1 )
    {
		if ( bPositionOnly )
			LightVector.xyz = LightInObject0.xyz;
		else
			LightVector.xyz = L0;

		LightVector.w = 1.f;
		LightColor  = half4( LightColor0.xyz * kL0, kL0 );

    }
    else if ( fL1 > fL0 )
    {
		if ( bPositionOnly )
			LightVector.xyz = LightInObject1.xyz;
		else
			LightVector.xyz = L1;

		LightVector.w = 1.f;
		LightColor  = half4( LightColor1.xyz * kL1, kL1 );

    }
	else
    {
		LightVector = half4( 0, 0, -1, 0 );
		LightColor = 0;
    }
}

//--------------------------------------------------------------------------------------------
void PickLight( out half4 LightVector, 
			    out half4 LightColor, 
			    half3 Position, 
			    half3 LightInObject0,
			    half3 LightInObject1,
			    half2 LightFalloff0,
			    half2 LightFalloff1,
			    half4 LightColor0,
			    half4 LightColor1,
			    uniform bool bPositionOnly )
{
	PickLightEx(	LightVector,
					LightColor,
					Position,
					half3(0,0,1),
					LightInObject0,
					LightInObject1,
					LightFalloff0,
					LightFalloff1,
					LightColor0,
					LightColor1,
					bPositionOnly,
					false );
}

//--------------------------------------------------------------------------------------------
void PickLightPos( out half4 LightPosition, 
			       out half4 LightColor, 
			       half3 Position, 
			       half3 LightInObject0,
			       half3 LightInObject1,
			       half2 LightFalloff0,
			       half2 LightFalloff1,
			       half4 LightColor0,
			       half4 LightColor1)
{
	PickLight(	LightPosition,
				LightColor,
				Position,
				LightInObject0,
				LightInObject1,
				LightFalloff0,
				LightFalloff1,
				LightColor0,
				LightColor1,
				true );
}

//--------------------------------------------------------------------------------------------

void VS_PickAndPrepareSpecular(
	in half3 P,
	in half3 N,
	out half4 Color,
	out half4 Vector,
	uniform bool bForceFirstLight,
	uniform int nSpace )
{
	half3 SLP_0 = nSpace == OBJECT_SPACE ? SpecularLightsPos(OBJECT_SPACE)[ 0 ] : SpecularLightsPos(WORLD_SPACE)[ 0 ];
	half3 SLP_1 = nSpace == OBJECT_SPACE ? SpecularLightsPos(OBJECT_SPACE)[ 1 ] : SpecularLightsPos(WORLD_SPACE)[ 1 ];
	half3 SLF_0 = nSpace == OBJECT_SPACE ? SpecularLightsFalloff(OBJECT_SPACE)[ 0 ] : SpecularLightsFalloff(WORLD_SPACE)[ 0 ];
	half3 SLF_1 = nSpace == OBJECT_SPACE ? SpecularLightsFalloff(OBJECT_SPACE)[ 1 ] : SpecularLightsFalloff(WORLD_SPACE)[ 1 ];

	if ( bForceFirstLight )
	{
		half3 L				= P - SLP_0;
		half fBrightness	= GetFalloffBrightness( L, SLF_0 );
		Color.xyz			= SpecularLightsColor[ 0 ].xyz * fBrightness;
		Color.w				= fBrightness;		// for glow attenuation

		Vector				= half4( L, 1.f ); // .w used to identify which light and avoid interpolation artifacts at the selection border
	}
	else
	{
		PickLightEx(	Vector, 
						Color, 
						P,
						N, 
						SLP_0,
						SLP_1,
						SLF_0,
						SLF_1,
						SpecularLightsColor[ 0 ],
						SpecularLightsColor[ 1 ],
						false,
						true );
	}
}

//--------------------------------------------------------------------------------------------

void PS_ApplySpecular(
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	in half  Ks,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow )
{
	// not attenuating dir-light specular
	half3 vPointSpecular	= vSpecularMap.xyz * Ks;

	// modulating glow factor by intensity
	if ( bAttenuateGlow )
		fGlow				+= Ks * Color.a;
	else
		fGlow				+= Ks;
	vOverBrighten			+= Color.rgb * vPointSpecular;
}

//--------------------------------------------------------------------------------------------
// Tugboat Specific
void PS_ApplySpecularBackground(
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	in half  Ks,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow,
	in half fFade )
{

	// not attenuating dir-light specular
	half3 vPointSpecular	= vSpecularMap.xyz * ( Ks * PSGetSpecularBrightness( L, N ) ) * fFade;

	// modulating glow factor by intensity
	if ( bAttenuateGlow )
		fGlow				+= Ks * Color.a;
	else
		fGlow				+= Ks;
	vOverBrighten			+= Color.rgb * vPointSpecular;
}

//--------------------------------------------------------------------------------------------
half PS_GetSpecularGloss(
	in half fSpecularAlpha )
{
	return lerp( SpecularGlossiness.x, SpecularGlossiness.y, fSpecularAlpha );
}

//--------------------------------------------------------------------------------------------

void PS_GetSpecularLUTKs(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half4 vSpecularMap,
	out half3 H,
	out half Ks )
{
	H						= normalize( V + L );
	Ks						= saturate( dot( H, N ) );
	// gloss is already pre-lerped in the spec LUT -- just want a 0..1 here
	Ks						= tex2D( SpecularLookupSampler, half2( vSpecularMap.w, Ks ) ).x;
}

//--------------------------------------------------------------------------------------------

void PS_EvaluateSpecularLUT(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow )
{
	half Ks;
	half3 H;
	PS_GetSpecularLUTKs( V, L, N, vSpecularMap, H, Ks );
	PS_ApplySpecular( L, N, Color, vSpecularMap, Ks, fGlow, vOverBrighten, bAttenuateGlow );
}

//--------------------------------------------------------------------------------------------
// Tugboat Specific
void PS_EvaluateSpecularBackgroundLUT(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow,
	half fFade )
{
	half Ks;
	half3 H;
	PS_GetSpecularLUTKs( V, L, N, vSpecularMap, H, Ks );
	PS_ApplySpecularBackground( L, N, Color, vSpecularMap, Ks, fGlow, vOverBrighten, bAttenuateGlow, fFade );
}

//--------------------------------------------------------------------------------------------
void PS_EvaluateSpecular(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	in half fGloss,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow,
	uniform bool bSpecularViaLUT )
{
	if ( bSpecularViaLUT )
	{
		PS_EvaluateSpecularLUT(
			V,
			L,
			N,
			Color,
			vSpecularMap,
			fGlow,
			vOverBrighten,
			bAttenuateGlow );
		return;
	}

	half Ks;
	half3 H;
	GetSpecularKs( V, L, N, fGloss, H, Ks );
	PS_ApplySpecular( L, N, Color, vSpecularMap, Ks, fGlow, vOverBrighten, bAttenuateGlow );
}

//--------------------------------------------------------------------------------------------
// Tugboat Specific
void PS_EvaluateSpecularBackground(
	in half3 V,	// view vector
	in half3 L, // light vector
	in half3 N,
	in half4 Color,
	in half4 vSpecularMap,
	in half fGloss,
	inout half fGlow,
	inout half3 vOverBrighten,
	uniform bool bAttenuateGlow,
	in half fFade )
{
	half Ks;
	half3 H;
	GetSpecularKs( V, L, N, fGloss, H, Ks );
	PS_ApplySpecularBackground( L, N, Color, vSpecularMap, Ks, fGlow, vOverBrighten, bAttenuateGlow, fFade );
}

//--------------------------------------------------------------------------------------------
void PS_SpecularCoverBoundary(
	in half fLightID,
	inout half4 Color )
{
	Color.rgba *= fLightID;
	return;
}

//----------------------------------------------------------------------------

void PS_GetSpecularMap(
	in		half2			vCoords,
	out		half4			vSpecularMap,
	out		half			SpecularPower,
	out		half			SpecularAlpha,
	out		half			SpecularGloss,
	uniform bool			bSpecularMap
	)
{
	half fOverride = SpecularMaskOverride;
	vSpecularMap = half4( fOverride.xxxx );
	if ( bSpecularMap )
	{
		vSpecularMap += tex2D( SpecularMapSampler, vCoords );
	}
	SpecularPower = length( vSpecularMap.rgb );
	vSpecularMap.rgb *= half( SpecularBrightness );
	SpecularAlpha = vSpecularMap.a;
	SpecularGloss = PS_GetSpecularGloss( SpecularAlpha );
}

//--------------------------------------------------------------------------------------------

half3 BRDF_Phong_Specular(
	in half3 N,				// normal
	in half3 H,				// half unit vector
	in half3 S,				// specular light color (pre-attenuated)
	in half  e )			// material specular exponent
{
	return pow( dot( N, H ), e ) * S;
}


//--------------------------------------------------------------------------------------------

half3 BRDF_Phong_Diffuse(
	in half3 N,				// normal
	in half3 I,				// incident light unit vector
	in half3 D )			// diffuse light color (pre-attenuated)
{
	return saturate( dot( N, I ) ) * D;
}

//--------------------------------------------------------------------------------------------

void LightPointCommon(
	int nP,
	in  half3 P,
	out half4 PlC,
	out half3 I,
	uniform int nSpace )
{
	half3 PlP;
	half2 PlF;
	PlC = PointLightsColor[ nP ];

	if ( nSpace == OBJECT_SPACE )
	{
		PlP = PointLightsPos(OBJECT_SPACE)[ nP ];
		PlF = PointLightsFalloff(OBJECT_SPACE)[ nP ];
	}
	else
	{
		PlP = PointLightsPos(WORLD_SPACE)[ nP ];
		PlF = PointLightsFalloff(WORLD_SPACE)[ nP ];
	}

	I			= PlP - P;
	half	id	= length( I );
	half	kd	= saturate( PlF.x - id * PlF.y );
	PlC			= kd * PlC;
	I			= normalize( I );
}

//--------------------------------------------------------------------------------------------

void LightPointDiffuse(
	in half3 N,				// normal
	in half3 I,				// incident light vector
	in half3 C,				// light color
	inout half3 Dc )		// diffuse color contribution
{
	Dc += BRDF_Phong_Diffuse( N, I, C );
}

//--------------------------------------------------------------------------------------------

void LightPointEx(
	in half4 PlC,
	in half3 I,
	in half3 N,				// normal
	in half3 P,				// position
	in half3 V,				// view unit vector
	in half4 Cmap,			// specular map
	inout Light L,
	uniform bool bDiffuse,
	uniform bool bSpecular,
	uniform bool bSpecularViaLUT )
{
	if ( bDiffuse )
		LightPointDiffuse( N, I, PlC, L.Diffuse );

	if ( bSpecular )
		PS_EvaluateSpecular( V, I, N, PlC, Cmap, L.SpecularGloss, L.Glow, L.Specular, true, bSpecularViaLUT );
}

//--------------------------------------------------------------------------------------------

void LightPoint(
	int nP,
	in half3 N,				// normal
	in half3 P,				// position
	in half3 V,				// view unit vector
	in half4 Cmap,			// specular map
	inout Light L,
	uniform int nSpace,
	uniform bool bDiffuse,
	uniform bool bSpecular,
	uniform bool bSpecularViaLUT )
{
	half4 PlC;
	half3 I;

	LightPointCommon( nP, P, PlC, I, nSpace );

	LightPointEx( PlC, I, N, P, V, Cmap, L, bDiffuse, bSpecular, bSpecularViaLUT );
}

//----------------------------------------------------------------------------

void PS_Spotlight(
	uniform bool bNormalMap,
	in half3 Nw,
	in PS_COMMON_SPOTLIGHT_INPUT InSpot,
	inout Light L
	)
{
	L.Diffuse += PS_EvalSpotlight(
		InSpot.St,
		InSpot.Nt,
		InSpot.Cs,
		Nw,
		bNormalMap ? OBJECT_SPACE : WORLD_SPACE );
}

//--------------------------------------------------------------------------------------------

void VS_Spotlight(
	uniform int nSpace,
	uniform bool bNormalMap,
	in half3 Pw,
	in half3x3 mTan,
	inout PS_COMMON_SPOTLIGHT_INPUT OutSpot
	)
{
	VS_PrepareSpotlight(
		OutSpot.St.xyz,
		OutSpot.Nt.xyz,
		OutSpot.Cs,
		Pw,
		mTan,
		bNormalMap,
		bNormalMap ? OBJECT_SPACE : nSpace
	);
}

//----------------------------------------------------------------------------\

float ComputeScaledAlpha( float fOpacity, float fGlow )
{
	return fOpacity	* ( ( ( half( 1.0f ) - gfAlphaForLighting ) * gfAlphaConstant ) + ( max( fGlow, GLOW_MIN ) * gfAlphaForLighting ) );
}

#endif // __LIGHTING_FX__
