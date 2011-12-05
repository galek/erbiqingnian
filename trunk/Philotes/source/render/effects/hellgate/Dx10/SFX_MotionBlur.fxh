//
// Motion blur screen effect
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_MOTION_BLUR_INPUT
{
    float4 Position			: POSITION;
    float2 TexCoord			: TEXCOORD0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////


PS_GREYSCALE_INPUT VS_MotionBlur( VS_POSITION_TEX In )
{
	PS_MOTION_BLUR_INPUT Out = (PS_MOTION_BLUR_INPUT) 0;
	Out.Position		= float4( In.Position, 1.0f );
	Out.TexCoord		= In.TexCoord;
	Out.TexCoord		+= gvPixelAccurateOffset;

    return Out;
}

#define FILT_SCALE_MOTIONZ 64
#define FILT_SCALE_MOTIONXY 8

static const float NumberOfPostProcessSamples = 12.0f;

float4 PS_MotionBlur( PS_MOTION_BLUR_INPUT In ) : COLOR
{
	float4 vColor = TextureRT.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, In.TexCoord );
	//return float4( vColor.r, 0, 0, 0.5 );
    float2 pixelVelocity = Texture_motion.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, In.TexCoord );

	float2 fSampleDist = gvScreenSize.zw;
   	for( uint i = 0; i < POISSON_MED_COUNT; ++i )
	{
		float2 vDist = poissonDiskMed[i] * fSampleDist;
		float2 coords = In.TexCoord + vDist;

		// Check neighboring pixel velocities in lieu of having last frame's velocity
		float4 neighborVelocity = Texture_motion.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, coords );

		if ( ( pow( neighborVelocity.x, 2 ) + pow( neighborVelocity.y, 2 ) ) >  
				( pow( pixelVelocity.x, 2 ) + pow( pixelVelocity.y, 2 ) ) )
		{
			pixelVelocity =  neighborVelocity;
		}
	}
	
	const float PixelBlurConst = 0.005f;
	pixelVelocity.y *= -1;
    pixelVelocity *= PixelBlurConst;
    //pixelVelocity *= gfPCSSLightSize.x;
	
	float2 fRadialCoord = In.TexCoord - float2( 0.5f, 0.5f );
	float2 fBlurRadial = pow( fRadialCoord, 2.f ) * 4.f;
	fBlurRadial *= gvScreenSize.zw * FILT_SCALE_MOTIONZ * gvCamVelocity.z;	
	fBlurRadial *= sign( fRadialCoord );
	
	if ( ( pow( fBlurRadial.x, 2 ) + pow( fBlurRadial.y, 2 ) ) >  
			( pow( pixelVelocity.x, 2 ) + pow( pixelVelocity.y, 2 ) ) )
	{
		pixelVelocity = fBlurRadial;
	}

    pixelVelocity = sign( pixelVelocity ) * min( abs( pixelVelocity ), 0.01f );
    
    // For each sample, sum up each sample's color in "Blurred" and then divide
    // to average the color after all the samples are added.
    float3 Blurred = 0;    
    for(float i = 0; i < NumberOfPostProcessSamples; i++)
    {   
        // Sample texture in a new spot based on pixelVelocity vector 
        // and average it with the other samples        
        float2 lookup = pixelVelocity * i / NumberOfPostProcessSamples + In.TexCoord;
        
        // Lookup the color at this new spot
        float4 Current = TextureRT.Sample( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, lookup);
        
        // Add it with the other samples
        Blurred += Current.rgb;
    }
    
    // Return the average color of all the samples
    return float4(Blurred / NumberOfPostProcessSamples, vColor.a );	
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MOTION_BLUR_AS_PASS
DECLARE_TECH MotionBlur <
	SHADER_VERSION_11
	SET_LAYER( LAYER_ON_TOP )
	>
{
	//PASS_COPY( SFX_RT_BACKBUFFER, SFX_RT_FULL )

	PASS_SHADERS( SFX_RT_FULL, SFX_RT_BACKBUFFER )
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_MotionBlur() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_MotionBlur() );

		DXC_BLEND_RGBA_NO_BLEND;
	}
}
#endif
