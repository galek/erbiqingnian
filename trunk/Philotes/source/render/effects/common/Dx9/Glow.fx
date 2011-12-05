//
// Glow Effect
//

#include "_common.fx"
#include "StateBlocks.fxh"


// glow parameters
float  GlowThickness = 0.05f;
float4 GlowColor     = float4(0.5f, 0.2f, 0.2f, 1.0f);
float4 GlowAmbient   = float4(0.2f, 0.2f, 0.0f, 0.0f);

struct VSGLOW_OUTPUT
{
    float4 Position : POSITION;
    float4 Diffuse  : COLOR;
};

// draws a transparent hull of the unskinned object.
VSGLOW_OUTPUT VSGlow
    (
    float4 Position : POSITION, 
    float3 Normal   : NORMAL
    )
{
	VSGLOW_OUTPUT Out = (VSGLOW_OUTPUT)0;

	float3 N = mul(Normal, (float3x3)WorldView);			// normal (view space)
	N = normalize(N);										// normal (view space)
	float4 P = mul(Position, WorldView);					// position (view space)
	P += GlowThickness * float4(N, 1);						// displaced position (view space)

	float Power;
	Power  = N.z;
	Power *= Power;
	Power -= 1;
	Power *= Power;     // Power = (1 - (N.A)^2)^2 OR ((N.A)^2 - 1)^2
 	Power = 1 - Power;

	Out.Position = mul(P, Projection);   // projected position
//	Out.Diffuse.rgb = 0.8;
//	Out.Diffuse.a = 0.5;
	Out.Diffuse  = GlowColor * Power + GlowAmbient; // modulated glow color + glow ambient

	return Out;    
}

float4 PSGlow(
    float4 Diff : COLOR0) : COLOR
{
    return Diff;
}


DECLARE_TECH TVertexAndPixelShader < SHADER_VERSION_11 >
{
	pass PGlow
	{   
		// glow shader
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VSGlow());
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PSGlow());

		DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;

#ifndef ENGINE_TARGET_DX10
		// enable alpha blending
		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCCOLOR;
		DestBlend        = INVSRCCOLOR;
#endif

		// set up texture stage states to use the diffuse color
//		ColorOp[0]   = SELECTARG2;
//		ColorArg2[0] = DIFFUSE;
//		AlphaOp[0]   = SELECTARG2;
//		AlphaArg2[0] = DIFFUSE;

//		ColorOp[1]   = DISABLE;
//		AlphaOp[1]   = DISABLE;
	}
}

