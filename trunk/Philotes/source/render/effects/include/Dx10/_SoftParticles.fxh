#undef DX10_SOFT_PARTICLES
#undef SOFT_PARTICLES
#undef FeatherAlpha
#undef TestDepth
#undef FN
#undef TECHNAME

#ifdef ENGINE_TARGET_DX10

#ifdef DX10_EMULATE_DX9_BEHAVIOR
	#define SOFT_PARTICLES	0
#else
	#define DX10_SOFT_PARTICLES
	#define SOFT_PARTICLES	1
#endif

// Optional 'contrast' function to smooth the curve from 0 to 1
float Contrast(float Input, float ContrastPower)
{
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? 1-Input : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? 1-Output : Output;
     return Output;
}

void TestDepth( in float4 Pos, in float depth )
{
	float zBuf = Texture_sceneDepth.Load( int4(Pos.x, Pos.y,0,0)).r;
    if( depth > zBuf )
    {
        discard;
    }
}

void FeatherAlpha( inout float Alpha, in float4 Pos, in float depth )
{
	float zBuf = Texture_sceneDepth.Load( int4(Pos.x, Pos.y,0,0)).r;
    if( depth > zBuf )
    {
        discard;
    }
    float d = (zBuf - depth) * gSoftParticleScale;
#if 0
    Alpha *= saturate(d);
#else
    Alpha *= Contrast(d, gSoftParticleContrast);
#endif
}

#	define FN(fn)			fn##_SOFT
#	define TECHNAME(tch)	tch##_SOFT

#else
#	define FeatherAlpha( Alpha, Pos, depth )
#	define TestDepth( Pos, depth )
#	define SOFT_PARTICLES	0
#	define FN(fn)			fn
#	define TECHNAME(tch)	tch
#endif