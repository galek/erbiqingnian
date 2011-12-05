//
// Silhouette Shader - draw a colored silhouette as fast as possible
//
// This is a pretty basic one to make sure that things are working.
//
#include "_common.fx"
// transformations
//////////////////////////////////////////////////////////////////////////////////////////////
float4 SillhouetteColor = { 0.25f, 0.25f, 0.25f, 1.f };
struct VS_OUTPUT
{
    float4 Position  : POSITION;
    float4 Diff : COLOR0;
    float  Fog  : FOG;
#ifdef ENGINE_TARGET_DX10
	float linearw : TEXCOORD1;
#endif
};

VS_OUTPUT VS(
    float4 Pos  : POSITION)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul(Pos, WorldViewProjection);          // position (projected)
    Out.Diff = float4( SillhouetteColor.xyz, GLOW_MIN );
#ifdef ENGINE_TARGET_DX10
	Out.linearw = Out.Position.w;
#endif	

	FOGMACRO( Pos )

    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////
technique TVertexShaderOnly
  < int VSVersion = VS_1_1; int PSVersion = PS_NONE; >
{
    pass P0
    {
        // shaders
        VertexShader = compile vs_1_1 VS();
        PixelShader  = NULL;
        
        // texture stages
        ColorArg1[0] = DIFFUSE;
        ColorOp[0]   = SELECTARG1;
		AlphaArg1[0] = DIFFUSE;
		AlphaOp[0]   = SELECTARG1;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
	}
}

#ifdef ENGINE_TARGET_DX10

void PS(
    VS_OUTPUT input,
    out float4 OutColor : COLOR,
    out float OutColorDepth : COLOR1   
    )
{
	float4 color = input.Diff;
	APPLY_FOG( color.xyz, input.Fog );

	OutColor = color;
	
#ifdef ENGINE_TARGET_DX10
	OutColorDepth = input.linearw;
#endif	
}

DECLARE_TECH TVertexShaderOnly  < SHADER_VERSION_11 > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS() );
    }
}
#endif


