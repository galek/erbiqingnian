//
// Simple Shader without textures - it just transforms and uses a vertex color
//
// It is currently used for line drawing
//
// IF YOU USE THIS SHADER, be sure to fill WorldMat, ViewMat and ProjMat!
// Also, you must transpose all incoming matrices!
//
#include "_common.fx"

// transformations

float4x4 WorldMat;
float4x4 ViewMat;
float4x4 ProjMat;


float4 VSSimple( VS_SIMPLE_INPUT input ) : POSITION
{
	return mul( mul( mul( float4( input.Position, 1.0f ), WorldMat ), ViewMat ), ProjMat );
}

float4 PSSimple( float4 Position : POSITION ) : COLOR
{
	return FogColor;
}

DECLARE_TECH TVertexAndPixelShader
  < SHADER_VERSION_11 >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VSSimple() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PSSimple() );
    }  
}

/*
technique TNoShader
  < int VSVersion = VS_NONE; int PSVersion = PS_NONE; int Index = 0; > 
{
    pass P0
    {
		// shaders
		VertexShader = NULL;
		PixelShader  = NULL;

		// transforms
		WorldTransform[0]	= (WorldMat);
		ViewTransform		= (ViewMat);
		ProjectionTransform = (ProjMat);

		// texture stages
		//ColorOp[0]   = SELECTARG2;
		//ColorArg2[0] = DIFFUSE;
		//AlphaOp[0]   = DISABLE;

		//Constant[0]	 = 0x017f7f7f;	// non-glowing grey // doesn't work on ATI
		ColorOp[0]   = SELECTARG1;
		ColorArg1[0] = TFACTOR;
		AlphaOp[0]   = SELECTARG1;
		AlphaArg1[0] = TFACTOR;

		ColorOp[1]   = DISABLE;
		AlphaOp[1]   = DISABLE;
    }
}

*/
