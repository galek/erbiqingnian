//
// Depth-of-field screen effect
//

#ifdef MOTION_BLUR_AS_PASS
	#include "../hellgate/Dx10/SFX_MotionBlur.fxh"
#endif	

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_DOF_INPUT
{
    float4 Position			: POSITION;
    float2 TexCoord			: TEXCOORD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_GREYSCALE_INPUT VS_DOF( VS_POSITION_TEX In )
{
	PS_DOF_INPUT Out = (PS_DOF_INPUT) 0;
	Out.Position		= float4( In.Position, 1.0f );
	Out.TexCoord		= In.TexCoord;
	Out.TexCoord		+= gvPixelAccurateOffset;

    return Out;
}

#define FILT_SCALE_FAR 4
#define FILT_SCALE_NEAR 2
#define FILT_SCALE_MOTIONZ 64
#define FILT_SCALE_MOTIONXY 8

float4 PS_DOF( PS_DOF_INPUT In ) : COLOR
{
	float4 vColor = TextureRT.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, In.TexCoord );
	float fDepth = fGetDepth( In.TexCoord );
		
#ifdef DX10_DOF_GATHER
	float fFarBlurPix = fDOFComputeFarBlur( fDepth );
	bool bBlurFar = fFarBlurPix > 0.0f;
	float fBlur = 0.0f;
	
	if( bBlurFar )
		fBlur = fFarBlurPix * FILT_SCALE_FAR;
	else
		fBlur = vColor.a * FILT_SCALE_NEAR;
				
	float2 fSampleDist = gvScreenSize.zw * fBlur;
	
#else // ! DX10_DOF_GATHER
	
	float fCOC = ( vColor.a - 0.5 ) * 2;

 //#define DEBUG_OUTPUT_COC	
 #ifdef DEBUG_OUTPUT_COC
	if ( fCOC < 0 )
	{
		vColor.r = abs( fCOC );
		vColor.gb = 0;
	}
	else
	{
		vColor.g = abs( fCOC );
		vColor.rb = 0;
	}
	
	return float4( vColor.rgb, 1 );
 #endif	
	
	float2 fSampleDist = gvScreenSize.zw * abs( fCOC ) * lerp( FILT_SCALE_NEAR, FILT_SCALE_FAR, vColor.a );
	float fTotalWeight = 1.0f;
			
#endif // DX10_DOF_GATHER

	for( uint i = 0; i < POISSON_LARGE_COUNT; ++i )
	{
		float2 vDist = poissonDiskLarge[i] * fSampleDist;
		float2 coords = In.TexCoord + vDist;
		float4 vSample = TextureRT.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, coords );
		float fWeight = 0;

   #ifdef DX10_DOF_GATHER		
		if( bBlurFar && vSample.a > 0.0f )  // this pixel past the focal plane and should reduce the wheight of samples that are in focus
			fWeight = vSample.a;					
   #else
		float fNeighborCOC = ( vSample.a - 0.5 ) * 2;
		float fNeighborDepth = fGetDepth( coords );		
	
  		//fWeight = abs( fCOC );
		fWeight = ( fNeighborDepth > fDepth ) ? 1 : abs( fNeighborCOC );
  					
		vColor.rgb += vSample.rgb * clamp( fWeight, 0, 1 );
		fTotalWeight += fWeight;
   #endif
	}
	vColor.rgb /= fTotalWeight;
			
	return float4( vColor.rgb, 1.0f );
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH DOF <
	SHADER_VERSION_11
	SET_LAYER( LAYER_ON_TOP )
	SET_LABEL( Float0, "Near DOF end" )
	SET_LABEL( Float1, "Far DOF start" )
	SET_LABEL( Float2, "Far DOF end" )
	>
{
#ifdef MOTION_BLUR_AS_PASS
	PASS_SHADERS( SFX_RT_BACKBUFFER, SFX_RT_FULL )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_MotionBlur() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_MotionBlur() );

		DXC_BLEND_RGBA_NO_BLEND;
	}
#else	
	PASS_COPY( SFX_RT_BACKBUFFER, SFX_RT_FULL )
#endif

	PASS_SHADERS( SFX_RT_FULL, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_DOF() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_DOF() );

		DXC_BLEND_RGB_NO_BLEND;
	}
}
