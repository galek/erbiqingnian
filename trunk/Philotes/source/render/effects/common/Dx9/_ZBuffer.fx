//
// Z Buffer - renders geometry so that the Z Buffer can be prepped for the actual render
//
#include "_common.fx"
#include "StateBlocks.fxh"
#include "Dx9/_AnimatedShared20.fxh"
#include "Dx10\\_SoftParticles.fxh"
#include "Dx9\\_ParticleMesh.fxh"

// transformations
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_ZBUFFER_INPUT
{
    float4 Position  : POSITION;
    float2 TexCoords : TEXCOORD0;
#ifdef ENGINE_TARGET_DX10
	float4 PositionPrev : TEXCOORD1;
	float4 PositionCur	: TEXCOORD2;
#endif
};

void VS_BackgroundShared( inout PS_ZBUFFER_INPUT Out, float3 Position )
{
    Out.Position  = mul( float4( Position, 1.0f ), WorldViewProjection );          // position (projected)
#ifdef ENGINE_TARGET_DX10
	Out.PositionCur = Out.Position;
	Out.PositionPrev  = mul( float4( Position, 1.0f ), WorldViewProjectionPrev );
#endif
} 

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_ZBUFFER_INPUT VS_BACKGROUND_ZBUFFER ( VS_BACKGROUND_INPUT_32 In ) 
{
    PS_ZBUFFER_INPUT Out = (PS_ZBUFFER_INPUT)0;

	VS_BackgroundShared( Out, In.Position );

    Out.TexCoords.xy = In.LightMapDiffuse.zw;
    
    return Out;
}

PS_ZBUFFER_INPUT VS_BACKGROUND_ZBUFFER_16 ( VS_BACKGROUND_INPUT_16 In ) 
{
    PS_ZBUFFER_INPUT Out = (PS_ZBUFFER_INPUT)0;

	VS_BackgroundShared( Out, In.Position );

    Out.TexCoords.xy = In.TexCoord.xy;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_ZBUFFER_INPUT VS_ANIMATED_ZBUFFER ( VS_ANIMATED_INPUT In, uniform bool bSkinned ) 
{
    PS_ZBUFFER_INPUT Out = (PS_ZBUFFER_INPUT)0;

	float4 Position;
	float4 PoPrev;

	if ( bSkinned )
	{
		GetPosition( Position, In );
#ifdef ENGINE_TARGET_DX10
		GetPositionPrev( PoPrev, In );
#endif
	}
	else
	{
		Position = float4( In.Position.xyz, 1 );
#ifdef ENGINE_TARGET_DX10
 		PoPrev = Position;
#endif
	}

	Out.Position  = mul( Position, WorldViewProjection );          // position (projected)
	
#ifdef ENGINE_TARGET_DX10
	Out.PositionCur = Out.Position;
	Out.PositionPrev  = mul( PoPrev, WorldViewProjectionPrev );
#endif

	Out.TexCoords.xy = In.DiffuseMap.xy;
	
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

PS_ZBUFFER_INPUT VS_BASIC_PARTICLE_ZBUFFER ( VS_PARTICLE_INPUT In )
{
    PS_ZBUFFER_INPUT Out = (PS_ZBUFFER_INPUT)0;

    Out.Position = mul( float4( In.Position.xyz, 1.0f ), WorldViewProjection);          // position (projected)
    
#ifdef ENGINE_TARGET_DX10
	Out.PositionCur = Out.Position;
#endif

    Out.TexCoords = In.Tex;
        
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

PS_ZBUFFER_INPUT VS_BASIC_PARTICLE_MESH_ZBUFFER ( VS_PARTICLE_MESH_INPUT In )
{
    PS_ZBUFFER_INPUT Out = (PS_ZBUFFER_INPUT)0;

	Out.Position = sGetPosition(In);
    Out.Position = mul( Out.Position, ViewProjection );

#ifdef ENGINE_TARGET_DX10
    Out.PositionCur = Out.Position;
#endif 

    Out.TexCoords = sGetDiffuseMapUV(In);
 
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void PS_PARTICLE_ZBUFFER (
	in	PS_ZBUFFER_INPUT				In,
#ifndef ENGINE_TARGET_DX10
	out float4							OutColorDepth		: COLOR
#else
	out float							OutColorDepth		: COLOR0
#endif
	)
{
	OutColorDepth = ComputeScaledAlpha( tex2D( DiffuseMapSampler, In.TexCoords.xy ).a, 0.f );
	
#ifdef ENGINE_TARGET_DX10

#ifdef DX10_SOFT_PARTICLES
	FeatherAlpha( OutColorDepth, In.Position, In.PositionCur.w );
#endif

    AppControlledAlphaTest( float4( 0, 0, 0, OutColorDepth ) );    
	OutColorDepth = In.PositionCur.w;
#endif	

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void PS_ZBUFFER (
	in	PS_ZBUFFER_INPUT				In,
#ifndef ENGINE_TARGET_DX10
	out float4							OutColorDepth		: COLOR
#else
	out float							OutColorDepth	: COLOR0,
	out float2							OutColorMotion	: SV_Target1
#endif
	)
{
	OutColorDepth = ComputeScaledAlpha( tex2D( DiffuseMapSampler, In.TexCoords.xy ).a, 0.f );
	
#ifdef ENGINE_TARGET_DX10
    AppControlledAlphaTest( float4( 0, 0, 0, OutColorDepth ) );
	OutColorDepth = In.PositionCur.w;
	OutColorMotion = GetPixelVelocity( In.PositionCur, In.PositionPrev );
#endif	

}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH RigidShader < SHADER_VERSION_1X  int Index = 0; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BACKGROUND_ZBUFFER() );        
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_ZBUFFER() );
    }  
}

DECLARE_TECH RigidShader16 < SHADER_VERSION_1X  int Index = 1; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BACKGROUND_ZBUFFER_16() );        
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_ZBUFFER() );
    }  
}

//--------------------------------------------------------------------------------------------
// CHB 2006.10.13 - Although this is compiled for VS 1.1, it fails
// validation on 1.1 hardware since the high number of bones
// overflows the constants registers. We would have to either (1)
// implement a separate version for low-bone animation on 1.1,
// restricting this one to 2.0 or better; or (2) allow this one on
// 1.1, where some cards with beefy registers might be able to use
// it, but allow silent failure for others.  The latter is chosen
// today until it's clear we need a low-bone animation version.
DECLARE_TECH AnimatedShader_NoSkin < SHADER_VERSION_1X  int Index = 2; int AllowInvalidVS = VS_1_1; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_ANIMATED_ZBUFFER( false ) );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_ZBUFFER() );
    }  
}

//--------------------------------------------------------------------------------------------
// CHB 2006.10.13 - Although this is compiled for VS 1.1, it fails
// validation on 1.1 hardware since the high number of bones
// overflows the constants registers. We would have to either (1)
// implement a separate version for low-bone animation on 1.1,
// restricting this one to 2.0 or better; or (2) allow this one on
// 1.1, where some cards with beefy registers might be able to use
// it, but allow silent failure for others.  The latter is chosen
// today until it's clear we need a low-bone animation version.
DECLARE_TECH AnimatedShader_Skin < SHADER_VERSION_1X  int Index = 3; int AllowInvalidVS = VS_1_1; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_ANIMATED_ZBUFFER( true ) );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_ZBUFFER() );
    }  
}

DECLARE_TECH ParticleShader < SHADER_VERSION_1X  int Index = 4; int AllowInvalidVS = VS_1_1; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_PARTICLE_ZBUFFER() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_PARTICLE_ZBUFFER() );
    }  
}

DECLARE_TECH ParticleMeshShader < SHADER_VERSION_1X  int Index = 5; int AllowInvalidVS = VS_1_1; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_PARTICLE_MESH_ZBUFFER() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_PARTICLE_ZBUFFER() );
    }  
}