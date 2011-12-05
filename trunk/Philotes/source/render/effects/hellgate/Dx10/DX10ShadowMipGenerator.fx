#include "_Common.fx"

float4 VS_FSQuadXY(uint iv : SV_VertexID) : SV_Position
{
  return float4((iv << 1) & 2, iv & 2, 0.5, 1) * 2 - 1;
}

float2 CreateMipPS(float4 vPos : SV_Position) : SV_Target0
{
  uint4 iPos = uint4((int)vPos.x << 1, (int)vPos.y << 1, 0, 0);  //Array index 0, miplevel 0
  float2 vDepth = tShadowMap.Load(iPos), vDepth1;
  ++iPos.x;
  vDepth1 = tShadowMap.Load(iPos);
  vDepth = float2(min(vDepth.x, vDepth1.x), max(vDepth.y, vDepth1.y));
  ++iPos.y;
  vDepth1 = tShadowMap.Load(iPos);
  vDepth = float2(min(vDepth.x, vDepth1.x), max(vDepth.y, vDepth1.y));
  --iPos.x;
  vDepth1 = tShadowMap.Load(iPos);
  vDepth = float2(min(vDepth.x, vDepth1.x), max(vDepth.y, vDepth1.y));
  return vDepth;
}

technique10 generate < SHADER_VERSION_44 >
{
	pass GenerateMips
	{
		SetVertexShader( CompileShader( vs_4_0, VS_FSQuadXY() ) );
		SetPixelShader( CompileShader( ps_4_0, CreateMipPS() ) );
		
		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGBA_NO_BLEND;
		DXC_DEPTHSTENCIL_OFF;
	}
}
