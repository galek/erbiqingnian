//
//
#include "_common.fx"

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

sampler NearestSampler = sampler_state
{
	Texture = (tDiffuseMap);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;
#else
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};

sampler LinearSampler = sampler_state
{
	Texture = (tDiffuseMap);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};

sampler LinearSampler2 = sampler_state
{
	Texture = (tDiffuseMap2);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
#else
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	SRGBTexture = FALSE;
#endif
	AddressU = CLAMP;
	AddressV = CLAMP;
};

//----------------------------------------------------------------------------
// DOWNSAMPLE shaders -- 4x downsample by supersampling/averaging
//----------------------------------------------------------------------------

struct PS_DOWNSAMPLE_INPUT
{
	float2 Tap1		: TEXCOORD0;
	float2 Tap2		: TEXCOORD1;
	float2 Tap3		: TEXCOORD2;
	float2 Tap4		: TEXCOORD3;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PS_DOWNSAMPLE_INPUT VS_DOWNSAMPLE(
	in VS_POSITION_TEX In,
	out float4 Position : POSITION )
{
	PS_DOWNSAMPLE_INPUT Out = (PS_DOWNSAMPLE_INPUT) 0;

	Position = float4( In.Position, 1.0f );

	// pixel size of source (4x bigger) texture
	float2 vPs	= gvScreenSize.zw * 0.25f;
	float2 vC = In.TexCoord;
	
#ifndef ENGINE_TARGET_DX10
	// half-pixel size of destination texture
	float2 vHPd = gvScreenSize.zw * 0.5f;

	// kernel center
	vC += vHPd;
#endif

	Out.Tap1	= vC + float2( -vPs.x, -vPs.y );
	Out.Tap2	= vC + float2(  vPs.x, -vPs.y );
	Out.Tap3	= vC + float2( -vPs.x,  vPs.y );
	Out.Tap4	= vC + float2(  vPs.x,  vPs.y );

	return Out;
}

//----------------------------------------------------------------------------

float4 PS_DOWNSAMPLE(
	PS_DOWNSAMPLE_INPUT In ) : COLOR
{
	float4 cAccum, cColor[ 4 ];
	cColor[ 0 ] = tex2D( LinearSampler,		In.Tap1 );
	cColor[ 1 ] = tex2D( LinearSampler,		In.Tap2 );
	cColor[ 2 ] = tex2D( LinearSampler,		In.Tap3 );
	cColor[ 3 ] = tex2D( LinearSampler,		In.Tap4 );
	
	float fCOC = 0.0f;
	
#ifdef DX10_DOF_GATHER
	fCOC += fDOFComputeNearBlur( fGetDepth( In.Tap1 ) ) * 0.25f;
	fCOC += fDOFComputeNearBlur( fGetDepth( In.Tap2 ) ) * 0.25f;
	fCOC += fDOFComputeNearBlur( fGetDepth( In.Tap3 ) ) * 0.25f;
	fCOC += fDOFComputeNearBlur( fGetDepth( In.Tap4 ) ) * 0.25f;
#endif

	cAccum.rgb  = cColor[ 0 ].rgb + cColor[ 1 ].rgb + cColor[ 2 ].rgb + cColor[ 3 ].rgb;
	cAccum.rgb *= 0.25f;
	
	cAccum.w    = max( cColor[ 0 ].w, max( cColor[ 1 ].w, max( cColor[ 2 ].w, cColor[ 3 ].w ) ) );

	float4 cFinal;
	cFinal.rgb = 2.0f * cAccum.rgb * cAccum.aaa;
	cFinal.a = fCOC;
	
	return cFinal;
}

//----------------------------------------------------------------------------
// GAUSSIAN shaders -- gaussian blur
//----------------------------------------------------------------------------

#define NUM_INNER_TAPS	7
#define NUM_OUTER_TAPS	6
#define NUM_WEIGHTS		7

//----------------------------------------------------------------------------

struct PS_GAUSS_INPUT
{
	float2 Tap0			: TEXCOORD0;
	float2 Tap12		: TEXCOORD1;
	float2 TapMinus12   : TEXCOORD2;
	float2 Tap34		: TEXCOORD3;
	float2 TapMinus34   : TEXCOORD4;
	float2 Tap56		: TEXCOORD5;
	float2 TapMinus56   : TEXCOORD6;
};

//----------------------------------------------------------------------------

PS_GAUSS_INPUT VS_GAUSSIAN(
	in VS_POSITION_TEX In,
	out float4 Position : POSITION,
	uniform float2 vDir )
{
	PS_GAUSS_INPUT Out = (PS_GAUSS_INPUT) 0;

	Position = float4( In.Position, 1.0f );

	Out.Tap0		= In.TexCoord + gvPixelAccurateOffset;
	Out.Tap12		= Out.Tap0 + gvScreenSize.zw * 1.5f * vDir;
	Out.TapMinus12	= Out.Tap0 - gvScreenSize.zw * 1.5f * vDir;
	Out.Tap34		= Out.Tap0 + gvScreenSize.zw * 3.5f * vDir;
	Out.TapMinus34	= Out.Tap0 - gvScreenSize.zw * 3.5f * vDir;
	Out.Tap56		= Out.Tap0 + gvScreenSize.zw * 5.5f * vDir;
	Out.TapMinus56	= Out.Tap0 - gvScreenSize.zw * 5.5f * vDir;

	return Out;
}
	
//----------------------------------------------------------------------------

static const float gfTexelWeights13[4] = 
{
	// 7-tap (13-tap)
	0.241020346,  // *1 center
	0.210494742,  // *2
	0.122636328,  // *2
	0.046358756,  // *2  = 1.f
};

static const float gfTexelWeights25[7] = 
{
	// 13-tap (25-tap)
	0.115943867,  // *1 center
	0.113036762,  // *2
	0.102124511,  // *2
	0.085068406,  // *2
	0.065333255,  // *2
	0.046262329,  // *2
	0.030202804,  // *2  = 1.f
};

//----------------------------------------------------------------------------

float4 PS_GAUSSIAN(
	PS_GAUSS_INPUT In,
	uniform float2 vDir,
	uniform int nTaps,
	uniform bool bMaxBlend ) : COLOR
{
	float4 cAccum, cColor[ NUM_INNER_TAPS ];
	cColor[ 0 ] = tex2D( NearestSampler,	In.Tap0 );			// sample  0
	cColor[ 1 ] = tex2D( LinearSampler,		In.Tap12 );			// sample  1,  2
	cColor[ 2 ] = tex2D( LinearSampler,		In.TapMinus12 );	// sample -1, -2
	cColor[ 3 ] = tex2D( LinearSampler,		In.Tap34 );			// sample  3,  4
	cColor[ 4 ] = tex2D( LinearSampler,		In.TapMinus34 );	// sample -3, -4
	cColor[ 5 ] = tex2D( LinearSampler,		In.Tap56 );			// sample  5,  6
	cColor[ 6 ] = tex2D( LinearSampler,		In.TapMinus56 );	// sample -5, -6

	if ( nTaps == 13 )
	{
		cAccum  = cColor[ 0 ] * gfTexelWeights13[ 0 ];
		cAccum += cColor[ 1 ] * gfTexelWeights13[ 1 ];
		cAccum += cColor[ 2 ] * gfTexelWeights13[ 1 ];
		cAccum += cColor[ 3 ] * gfTexelWeights13[ 2 ];
		cAccum += cColor[ 4 ] * gfTexelWeights13[ 2 ];
		cAccum += cColor[ 5 ] * gfTexelWeights13[ 3 ];
		cAccum += cColor[ 6 ] * gfTexelWeights13[ 3 ];
	}
	else if ( nTaps == 25 )
	{
		cAccum  = cColor[ 0 ] * gfTexelWeights25[ 0 ];
		cAccum += cColor[ 1 ] * gfTexelWeights25[ 1 ];
		cAccum += cColor[ 2 ] * gfTexelWeights25[ 1 ];
		cAccum += cColor[ 3 ] * gfTexelWeights25[ 2 ];
		cAccum += cColor[ 4 ] * gfTexelWeights25[ 2 ];
		cAccum += cColor[ 5 ] * gfTexelWeights25[ 3 ];
		cAccum += cColor[ 6 ] * gfTexelWeights25[ 3 ];

		float2 fOuterTaps[ NUM_OUTER_TAPS ];

		fOuterTaps[ 0 ] = In.Tap0 + gvScreenSize.zw * 7.5f  * vDir;		// coord for sample   7,   8
		fOuterTaps[ 1 ] = In.Tap0 - gvScreenSize.zw * 7.5f  * vDir;		// coord for sample  -7,  -8
		fOuterTaps[ 2 ] = In.Tap0 + gvScreenSize.zw * 9.5f  * vDir;		// coord for sample   9,  10
		fOuterTaps[ 3 ] = In.Tap0 - gvScreenSize.zw * 9.5f  * vDir;		// coord for sample  -9, -10
		fOuterTaps[ 4 ] = In.Tap0 + gvScreenSize.zw * 11.5f * vDir;		// coord for sample  11,  12
		fOuterTaps[ 5 ] = In.Tap0 - gvScreenSize.zw * 11.5f * vDir;		// coord for sample -11, -12

		for ( int i = 0; i < NUM_OUTER_TAPS; i++ )
		{
			cColor[ i ] = tex2D( LinearSampler, fOuterTaps[ i ] );
		}

		cAccum += cColor[ 0 ] * gfTexelWeights25[ 4 ];
		cAccum += cColor[ 1 ] * gfTexelWeights25[ 4 ];
		cAccum += cColor[ 2 ] * gfTexelWeights25[ 5 ];
		cAccum += cColor[ 3 ] * gfTexelWeights25[ 5 ];
		cAccum += cColor[ 4 ] * gfTexelWeights25[ 6 ];
		cAccum += cColor[ 5 ] * gfTexelWeights25[ 6 ];
	}

#ifdef ENGINE_TARGET_DX10
	if( bMaxBlend )
	{
		float fOriginalValue = tex2D( LinearSampler2, In.Tap0 ).a;		
		cAccum.a = 2 * max( cAccum.a, fOriginalValue ) - fOriginalValue;
	}
#endif

	return cAccum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

// CHB 2006.06.13 Collapsed separate blur techniques into passes of a single technique.

DECLARE_TECH TGauss < SHADER_VERSION_22 >
{
	// TGaussX_Small
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_GAUSSIAN( float2(1,0) ) );
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(1,0), 13, false ) );
#ifndef ENGINE_TARGET_DX10
		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
		SRGBWriteEnable  = FALSE;
#endif
    }
    
    // TGaussY_Small
    pass P1
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_GAUSSIAN( float2(0,1) ) );
#ifdef DX10_DOF_GATHER
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(0,1), 13, true ) );
#else
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(0,1), 13 , false ) );
#endif

#ifndef ENGINE_TARGET_DX10
		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
		SRGBWriteEnable  = FALSE;
#endif
    }

	// TGaussX_Large
    pass P2
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_GAUSSIAN( float2(1,0) ) );
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(1,0), 25, false ) );
#ifndef ENGINE_TARGET_DX10
		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
		SRGBWriteEnable  = FALSE;
#endif
    }
    
    // TGaussY_Large
    pass P3
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_GAUSSIAN( float2(0,1) ) );
        
#ifdef DX10_DOF_GATHER
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(0,1), 25, true ) );
#else
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_GAUSSIAN( float2(0,1), 25 , false ) );
#endif

#ifndef ENGINE_TARGET_DX10
		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
		SRGBWriteEnable  = FALSE;
#endif
    }
    
    // TDownsample
    pass P4
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS_DOWNSAMPLE() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_DOWNSAMPLE() );
//State now set in dxC_render.cpp
/*
#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable = RED | GREEN | BLUE | ALPHA;
		ZWriteEnable	 = FALSE;
		ZEnable			 = FALSE;
		SRGBWriteEnable  = FALSE;
#endif
*/
    }
}

