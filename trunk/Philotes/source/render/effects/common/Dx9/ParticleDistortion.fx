//
// Particle Shader - Fade in for the Hell Gates
//

// transformations
#include "_common.fx"
#include "Dx10\\_SoftParticles.fxh"
#include "StateBlocks.fxh"

texture TextureRT;

sampler RTSampler : register(SAMPLER(SAMPLER_DIFFUSE2)) = sampler_state
{
	Texture = (TextureRT);
	AddressU = CLAMP;
	AddressV = CLAMP;

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_LINEAR_MIP_POINT;
#else
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = POINT;
#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float2 DiffuseTex0	: TEXCOORD0;
    float3 ScreenCoords	: TEXCOORD1;
    float4 Color		: COLOR0;
#ifdef DX10_SOFT_PARTICLES
    float  Z    		: TEXCOORD2;
#endif
};

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	half fPowerScale	= 1.0f;					// what is the max displacement (pct of screen)?

    Out.Position		= mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
    Out.DiffuseTex0		= In.Tex;
    float4 inColor		= DX10_SWIZ_COLOR_INPUT( In.Color );
    Out.Color.xy		= inColor.xw;			// selecting red and alpha

	// calculate screen area falloff factor
	half fDistance		= length( half3( In.Position.xyz ) - EyeInWorld );
	half fTheta			= atan( half(1.f) / fDistance );
	fPowerScale			*= saturate( fTheta / half( PI / 4.f ) );

	Out.Color.x			*= fPowerScale;

	Out.ScreenCoords	= float3( Out.Position.x, -Out.Position.y, Out.Position.w );
	
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif

    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_BASIC ( PS_BASIC_PARTICLE_INPUT In, uniform bool bUseAlphaFromTexture ) : COLOR
{
	half fAlphaScale	= 10.f;					// how fast is the alpha falloff with displacement length?  (fade out bottom (1/fAlphaScale) pct)

	half2 vCoords		= In.ScreenCoords.xy;
	half fPower			= In.Color.x;
	half fAlpha			= In.Color.y;

	half2 dUdV;	
	if ( bUseAlphaFromTexture )
	{
		half3 dUdVwA = ReadDuDvMapWithAlpha( DiffuseMapSampler, In.DiffuseTex0 );
		dUdV = dUdVwA.xy;
		fAlpha *= dUdVwA.z;
	}
	else
	{
		dUdV		= ReadDuDvMap( DiffuseMapSampler, In.DiffuseTex0 );
		fAlpha		*= saturate( length( dUdV ) * fAlphaScale );
	}

	half XScale			= ( gvViewport.z - gvViewport.x ) / ( gvScreenSize.x);
	half YScale			= ( gvViewport.w - gvViewport.y ) / ( gvScreenSize.y);

	half XOffset		= gvViewport.x / gvScreenSize.x;
	half YOffset		= gvViewport.y / gvScreenSize.y;

	vCoords.xy			/= In.ScreenCoords.z;
	vCoords.xy			= vCoords.xy * 0.5f + 0.5f;
	vCoords.x			*= XScale;
	vCoords.y			*= YScale;
	vCoords.x			+= XOffset;
	vCoords.y			+= YOffset;
	//vCoords.xy		+= gvPixelAccurateOffset;

	vCoords				+= dUdV.xy * fPower; 

	float4 color = float4( tex2D( RTSampler, vCoords ).rgb, fAlpha );

#ifdef DX10_SOFT_PARTICLES
	FeatherAlpha( color.a, In.Position, In.Z );
#endif

	return color;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// CHB 2006.06.16 - This was originally SHADER_GROUP_20, so using VS_2_0
// here to maintain current run-time behavior, although this differs from
// the actual compilation version level 'vs_1_1'.
DECLARE_TECH Distortion  < 
	SHADER_VERSION_11
	int Index = 0;
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_BASIC( false ));
        
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        
        //TODO: Alpha test to be done in the pixel shader...

#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

		CullMode			= NONE;
		AlphaBlendEnable 	= TRUE;
		BlendOp				= ADD;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		COLORWRITEENABLE 	= BLUE | GREEN | RED;
		ZWriteEnable	 	= FALSE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
// AE 2008.01.07 - Created a second technique to perform distortion using diffuse
// textures rather than normal map textures. We can support alpha testing from 
// the original texture with this method.
DECLARE_TECH DistortionWithAlpha  < 
	SHADER_VERSION_11
	int Index = 1;
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_BASIC( true ));
        
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        
        //TODO: Alpha test to be done in the pixel shader...

#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

		CullMode			= NONE;
		AlphaBlendEnable 	= TRUE;
		BlendOp				= ADD;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		COLORWRITEENABLE 	= BLUE | GREEN | RED;
		ZWriteEnable	 	= FALSE;
#endif
    }  
}

