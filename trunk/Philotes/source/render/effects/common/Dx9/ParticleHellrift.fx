//
// Particle Shader 
//

// transformations
#include "_common.fx"
#include "Dx10\\_SoftParticles.fxh"
#include "StateBlocks.fxh"

half4 gvUVScale;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_HELLRIFT_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Color		: COLOR0;
    float2 TexQuad		: TEXCOORD0;
    float3 TexScreen	: TEXCOORD1;
    float  Fog			: FOG;
#ifdef DX10_SOFT_PARTICLES
    float  Z    		: TEXCOORD2;
#endif
};

//--------------------------------------------------------------------------------------------
// Hellrift particle -- take in 3D position, transform, and tex lookup using screen pos as UV
//--------------------------------------------------------------------------------------------

PS_HELLRIFT_PARTICLE_INPUT VS_PARTICLE_HELLRIFT ( VS_PARTICLE_INPUT In )
{
	PS_HELLRIFT_PARTICLE_INPUT Out = (PS_HELLRIFT_PARTICLE_INPUT) 0;

	Out.Position		= mul( float4( In.Position, 1.0f ), WorldViewProjection );
	Out.Color			= In.Color;
	Out.TexQuad			= In.Tex;
	Out.TexScreen		= float3( Out.Position.x, -Out.Position.y, Out.Position.w );
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif
	
	FOGMACRO( In.Position )

    return Out;
}

// for the alpha channel from the hellrift image
#if !defined(FXC_LEGACY_COMPILE)	// CHB 2007.02.16
float4 PS_PARTICLE_HELLRIFT_ALPHA ( PS_HELLRIFT_PARTICLE_INPUT In ) : COLOR
{
#ifdef DX10_SOFT_PARTICLES
	TestDepth( In.Position, In.Z );
#endif
	half2 vCoords = In.TexScreen.xy;

	vCoords.xy	/= In.TexScreen.z;
	vCoords.xy	= vCoords.xy * 0.5f + 0.5f;
	vCoords.xy	*= gvUVScale.xy;
	vCoords.xy	+= gvPixelAccurateOffset;

	float4 vColor;
	vColor.rgb = 0;
	//vColor.a   = tex2D( LightMapSampler, vCoords ).a;
	// trying to see if I can make the alpha only show up in the portal area (not fill the whole quad)
	vColor.a   = tex2D( SelfIlluminationMapSampler, vCoords ).a * tex2D( DiffuseMapSampler, In.TexQuad ).b;	
	return vColor;
}
#endif

// for the color, modulated by the diffuse greyscale mask
float4 PS_PARTICLE_HELLRIFT_COLOR ( PS_HELLRIFT_PARTICLE_INPUT In, uniform bool bSM11, uniform bool bAlphaTest ) : COLOR
{
#ifdef DX10_SOFT_PARTICLES
	TestDepth( In.Position, In.Z );
#endif
	half2 vCoords = In.TexScreen.xy;

	if (!bSM11) {
		vCoords.xy	/= In.TexScreen.z;
		vCoords.xy	= vCoords.xy * 0.5f + 0.5f;
		vCoords.xy	*= gvUVScale.xy;
		vCoords.xy	+= gvPixelAccurateOffset;
	}

	float4 vColor;
	vColor.rgb = In.Color.rgb;
#if !defined(FXC_LEGACY_COMPILE)	// CHB 2007.02.16
	vColor.rgb *= SAMPLE_SELFILLUM( vCoords );
#endif
	vColor.a   = tex2D( DiffuseMapSampler, In.TexQuad ).b;
    AlphaTest(bAlphaTest, vColor )
    return vColor;
}


//--------------------------------------------------------------------------------------------
// CHB 2006.09.06 - SM 1.1 version
// This is rather hackish, but it mostly works.
//--------------------------------------------------------------------------------------------

PS_HELLRIFT_PARTICLE_INPUT VS_PARTICLE_HELLRIFT_11 ( VS_PARTICLE_INPUT In )
{
	PS_HELLRIFT_PARTICLE_INPUT Out = (PS_HELLRIFT_PARTICLE_INPUT)0;

	Out.Position	= mul( float4( In.Position, 1.0f ), WorldViewProjection );
	Out.Color		= In.Color;
	Out.TexQuad		= In.Tex;
	
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif
	Out.Position.xyz /= Out.Position.w;
	Out.Position.w = 1.0f;	// kill the rasterizer's perspective correction
	
	half2 vScreen = half2( Out.Position.x, -Out.Position.y );
	vScreen.xy = vScreen.xy * 0.5f + 0.5f;
	vScreen.xy *= gvUVScale.xy;
	vScreen.xy += gvPixelAccurateOffset;
	Out.TexScreen = float3(vScreen.x, vScreen.y, 0);	// easier than using a separate structure

	FOGMACRO( In.Position )

    return Out;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// CHB 2006.06.16 - This was originally SHADER_GROUP_20, so using VS_2_0
// here to maintain current run-time behavior, although this differs from
// the actual compilation version level 'vs_1_1'.
#if !defined(FXC_LEGACY_COMPILE)	// CHB 2007.02.16
DECLARE_TECH TParticleHellrift_a < SHADER_VERSION_22 int Index = 0; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_PARTICLE_HELLRIFT());
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_PARTICLE_HELLRIFT_ALPHA());
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_A_NO_BLEND;
        DXC_DEPTHSTENCIL_ZREAD;
        
#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= FALSE;

		ColorWriteEnable	= ALPHA;
		BlendOp				= MAX;
		SrcBlend			= SRCALPHA;
		DestBlend			= DESTALPHA;
		AlphaBlendEnable	= FALSE; //TRUE;
		Cullmode			= NONE;
		ZWriteEnable		= FALSE;
#endif
	}
	pass P1
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_PARTICLE_HELLRIFT());
		COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_PARTICLE_HELLRIFT_COLOR(false, true));
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        //TODO: Alpha Test to be done in shader
        
#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

        COLORWRITEENABLE	= BLUE | GREEN | RED;
		BlendOp				= ADD;
		SrcBlend			= SRCALPHA;
		DestBlend			= INVSRCALPHA;
		AlphaBlendEnable	= TRUE;
		Cullmode			= NONE;
		ZWriteEnable		= FALSE;
#endif
	}
}
#endif

#ifndef ENGINE_TARGET_DX10
// CHB 2006.09.05 - SM 1.1 version.
DECLARE_TECH TParticleHellrift_b < SHADER_VERSION_11 >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_PARTICLE_HELLRIFT_11());
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_PARTICLE_HELLRIFT_COLOR(true, true));
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        //TODO: Alpha Test to be done in shader

#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

		COLORWRITEENABLE	= BLUE | GREEN | RED;
		BlendOp				= ADD;
		SrcBlend			= SRCALPHA;
		DestBlend			= INVSRCALPHA;
		AlphaBlendEnable	= TRUE;
		Cullmode			= NONE;
		ZWriteEnable		= FALSE;
#endif
	}
}
#endif