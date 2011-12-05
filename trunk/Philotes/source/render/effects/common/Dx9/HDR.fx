//
// HDR pipeline shaders
//

#include "_common.fx"
#include "StateBlocks.fxh"



//------------------------------------------------------------------
//  GLOBAL VARIABLES
//------------------------------------------------------------------
float4      tcLumOffsets[4];                // The offsets used by GreyScaleDownSample()
float4      tcDSOffsets[9];                 // The offsets used by DownSample()

texture		tLuminanceSrc;
sampler     s0  :   register( s0 ) = sampler_state         // The first texture
{
	Texture		= (tLuminanceSrc);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
#endif
};

sampler     s1  :   register( s0 ) = sampler_state         // The first texture
{
	Texture		= (tLuminanceSrc);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter	= POINT;
	MagFilter	= POINT;
#endif
};


//------------------------------------------------------------------
//  DEBUG LUMINANCE DISPLAY
//------------------------------------------------------------------
float4 PS_LuminanceDisplay( in float2 t : TEXCOORD0 ) : COLOR0
{

    // Acquire the luminance from the texture
        float4 l = tex2D( s0, t );
        
    // Compute a simple scalar, due to the values being > 1.0f
    // the output is often all white, so just to make it a little
    // more informative we can apply a scalar to bring it closer
    // to the 0..1 range
        float scalar = 1.0f;
        
    // Only the RED and GREEN channels have anything stored in them, but
    // we're not interested in the maximum value, so we just use the red
    // channel:
        return float4( l.r * scalar, l.r * scalar, l.r * scalar, 1.0f );
        
}
    

//------------------------------------------------------------------
//  This entry point performs the basic first-pass when measuring
//  luminance of the HDR render target. It samples the HDR target
//  multiple times so as to compensate for the down-sampling and
//  subsequent loss of data.
//------------------------------------------------------------------
float4 PS_GreyScaleDownSample( in float2 t : TEXCOORD0 ) : COLOR0
{

    // Compute the average of the 4 necessary samples
        float average = 0.0f;
        float maximum = -1e20;
        float4 color = 0.0f;
        float3 WEIGHT = float3( 0.299f, 0.587f, 0.114f );
        
        for( int i = 0; i < 4; i++ )
        {
            color = tex2D( s0, t + float2( tcLumOffsets[i].x, tcLumOffsets[i].y ) );
            
            // There are a number of ways we can try and convert the RGB value into
            // a single luminance value:
            
            // 1. Do a very simple mathematical average:
            //float GreyValue = dot( color.rgb, float3( 0.33f, 0.33f, 0.33f ) );
            
            // 2. Perform a more accurately weighted average:
            //float GreyValue = dot( color.rgb, WEIGHT );
            
            // 3. Take the maximum value of the incoming, same as computing the
            //    brightness/value for an HSV/HSB conversion:
            float GreyValue = max( color.r, max( color.g, color.b ) );
            
            // 4. Compute the luminance component as per the HSL colour space:
            //float GreyValue = 0.5f * ( max( color.r, max( color.g, color.b ) ) + min( color.r, min( color.g, color.b ) ) );
            
            // 5. Use the magnitude of the colour
            //float GreyValue = length( color.rgb );
                        
            maximum = max( maximum, GreyValue );
            average += (0.25f * log( 1e-5 + GreyValue )); //1e-5 necessary to stop the singularity at GreyValue=0
        }
        
        average = exp( average );
        
    // Output the luminance to the render target
        return float4( average, maximum, 0.0f, 1.0f );
        
}
    
    
    
//------------------------------------------------------------------
//  This entry point will, using a 3x3 set of reads will downsample
//  from one luminance map to another.
//------------------------------------------------------------------
float4 PS_DownSample( in float2 t : TEXCOORD0 ) : COLOR0
{
    
    // Compute the average of the 10 necessary samples
        float4 color = 0.0f;
        float maximum = -1e20;
        float average = 0.0f;
        
        for( int i = 0; i < 9; i++ )
        {
            color = tex2D( s1, t + float2( tcDSOffsets[i].x, tcDSOffsets[i].y ) );
            average += color.r;
            maximum = max( maximum, color.g );
        }
        
    // We've just taken 9 samples from the
    // high resolution texture, so compute the
    // actual average that is written to the
    // lower resolution texture (render target).
        average /= 9.0f;
        
    // Return the new average luminance
        return float4( average, maximum, 0.0f, 1.0f );
}




//------------------------------------------------------------------
//  GLOBAL VARIABLES
//------------------------------------------------------------------
texture tOriginalScene;
texture tLuminance;
texture tBloom;

sampler     original_scene  : register( s0 ) = sampler_state		// The HDR data
{
	Texture = (tOriginalScene);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
#endif
};
sampler     luminance       : register( s1 ) = sampler_state		// The 1x1 luminance map
{
	Texture = (tLuminance);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
#endif
};
sampler     bloom           : register( s2 ) = sampler_state		// The post processing results
{
	Texture = (tBloom);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter	= LINEAR;
	MagFilter	= LINEAR;
#endif
};


float       fExposure;                          // A user configurable bias to under/over expose the image
float       fGaussianScalar;                    // Used in the post-processing, but also useful here
float       g_rcp_bloom_tex_w;                  // The reciprocal WIDTH of the texture in 'bloom'
float       g_rcp_bloom_tex_h;                  // The reciprocal HEIGHT of the texture in 'bloom'



//------------------------------------------------------------------
//  SHADER ENTRY POINT
//------------------------------------------------------------------
float4 PS_FinalPass( in float2 t : TEXCOORD0 ) : COLOR0
{

    // Read the HDR value that was computed as part of the original scene
        float4 c = tex2D( original_scene, t );
    
    // Read the luminance value, target the centre of the texture
    // which will map to the only pixel in it!
        float4 l = tex2D( luminance, float2( 0.5f, 0.5f ) );

/*        
    // Compute the blur value using a bilinear filter
    // It is worth noting that if the hardware supports linear filtering of a
    // floating point render target that this step can probably be skipped.
        float xWeight = frac( t.x / g_rcp_bloom_tex_w ) - 0.5;
        float xDir = xWeight;
        xWeight = abs( xWeight );
        xDir /= xWeight;
        xDir *= g_rcp_bloom_tex_w;

        float yWeight = frac( t.y / g_rcp_bloom_tex_h ) - 0.5;
        float yDir = yWeight;
        yWeight = abs( yWeight );
        yDir /= yWeight;
        yDir *= g_rcp_bloom_tex_h;

    // sample the blur texture for the 4 relevant pixels, weighted accordingly
        float4 b = ((1.0f - xWeight) * (1.0f - yWeight))    * tex2D( bloom, t );        
        b +=       (xWeight * (1.0f - yWeight))             * tex2D( bloom, t + float2( xDir, 0.0f ) );
        b +=       (yWeight * (1.0f - xWeight))             * tex2D( bloom, t + float2( 0.0f, yDir ) );
        b +=       (xWeight * yWeight)                      * tex2D( bloom, t + float2( xDir, yDir ) );
//*/

	float b = 0.f;  
            
    // Compute the actual colour:
        float4 final = c + 0.25f * b;
            
    // Reinhard's tone mapping equation (See Eqn#3 from 
    // "Photographic Tone Reproduction for Digital Images" for more details) is:
    //
    //      (      (   Lp    ))
    // Lp * (1.0f +(---------))
    //      (      ((Lm * Lm)))
    // -------------------------
    //         1.0f + Lp
    //
    // Lp is the luminance at the given point, this is computed using Eqn#2 from the above paper:
    //
    //        exposure
    //   Lp = -------- * HDRPixelIntensity
    //          l.r
    //
    // The exposure ("key" in the above paper) can be used to adjust the overall "balance" of 
    // the image. "l.r" is the average luminance across the scene, computed via the luminance
    // downsampling process. 'HDRPixelIntensity' is the measured brightness of the current pixel
    // being processed.
    
    fExposure = 0.18f;
    //l.r = 30.0f;
    //l.g = 10000.f;

        float Lp = (fExposure / l.r) * max( final.r, max( final.g, final.b ) );
    
    // A slight difference is that we have a bloom component in the final image - this is *added* to the 
    // final result, therefore potentially increasing the maximum luminance across the whole image. 
    // For a bright area of the display, this factor should be the integral of the bloom distribution 
    // multipled by the maximum value. The integral of the gaussian distribution between [-1,+1] should 
    // be AT MOST 1.0; but the sample code adds a scalar to the front of this, making it a good enough
    // approximation to the *real* integral.
    
        float LmSqr = (l.g + fGaussianScalar * l.g) * (l.g + fGaussianScalar * l.g);

    // Compute Eqn#3:
        float toneScalar = ( Lp * ( 1.0f + ( Lp / ( LmSqr ) ) ) ) / ( 1.0f + Lp );
    
    // Tonemap the final outputted pixel:
        c = final * toneScalar;
    
		c.rgb = pow(c.rgb, 1.f / 2.2f);
    
    // Return the fully composed colour
        c.a = 1.0f;
        return c;

}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
DECLARE_TECH TLuminance < SHADER_VERSION_12 int Index = 0; >
{
	pass GreyscaleDownsample
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_GreyScaleDownSample() );

		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGBA_NO_BLEND;
		DXC_DEPTHSTENCIL_OFF;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable = RED | GREEN | BLUE | ALPHA;
		CullMode = NONE;
		AlphaBlendEnable = FALSE;
		AlphaTestEnable = FALSE;
		ZEnable = FALSE;
		FogEnable = FALSE;
		BlendOp			 = ADD;
		SrcBlend         = ONE;
		DestBlend        = ZERO;
#endif
	}

	pass Downsample
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_DownSample() );
	}

	pass DebugDisplay
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_LuminanceDisplay() );
	}
}

//--------------------------------------------------------------------------------------------
DECLARE_TECH TFinalPass < SHADER_VERSION_12 int Index = 1; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FSQuadUV() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_FinalPass() );
	}
}