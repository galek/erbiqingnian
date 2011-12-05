//
// Actor Glow - it just transforms and uses one texture - with full glow
//		Uses Texture Channel 0 instead of 1, to match actor object mapping
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

VS_OUTPUT VS(
    float4 Pos  : POSITION, 
    float2 Tex  : TEXCOORD0)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul(Pos, WorldViewProjection);          // position (projected)
	Out.Tex  = Tex;                                       
	Out.Diff = float4( 1,1,1,1 );

	FOGMACRO( Pos )
	 
    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////

VS_OUTPUT VS_NPS(
    float4 Pos  : POSITION, 
    float2 Tex  : TEXCOORD0)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    Out.Position  = mul(Pos, WorldViewProjection);          // position (projected)
    Out.Diff = 1.0f;
    Out.Diff.w = 1.0f;
    Out.Tex  = Tex;                                       

    return Out;
}

#include "Dx9/_GlowShared.fx"
