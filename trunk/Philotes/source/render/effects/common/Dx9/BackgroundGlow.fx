//
// Background Glow - it just transforms and uses one texture - with full glow
//		Uses Texture Channel 1 instead of 0, to match background object mapping
//
#include "_common.fx"
// transformations
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_OUTPUT
{
    float4 Position  : POSITION;
    float4 Diff : COLOR0;
    float2 Tex  : TEXCOORD0;
    float  Fog  : FOG;
};

VS_OUTPUT VS( VS_BACKGROUND_INPUT_16 Input )
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul( float4( Input.Position, 1.0f ), WorldViewProjection);          // position (projected)
	Out.Tex  = UNBIAS_TEXCOORD( Input.TexCoord );   

	FOGMACRO( Input.Position )
	 
    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////

VS_OUTPUT VS_NPS( VS_BACKGROUND_INPUT_16 Input )
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul( float4( Input.Position, 1.0f ), WorldViewProjection);          // position (projected)
    Out.Diff = 1.0f;
    Out.Diff.w = 0.0f;
    Out.Tex  = UNBIAS_TEXCOORD( Input.TexCoord );                                       

    return Out;
}

#include "Dx9/_GlowShared.fx"
