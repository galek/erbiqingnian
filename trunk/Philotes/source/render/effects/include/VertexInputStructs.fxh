#ifndef __VERTEX_INPUT_STRUCTS__
#define __VERTEX_INPUT_STRUCTS__

#define VERTEX_DECL_FX
#include "../../source/dxC/dxC_VertexDeclarations.h"

//--------------------------------------------------------------------------------------------
struct VS_BACKGROUND_INPUT_DIFF2_32
{
    float4 Position			: POSITION;
    float3 Normal			: NORMAL;
    float4 LightMapDiffuse 	: TEXCOORD0;
    float2 DiffuseMap2 		: TEXCOORD1;  //KMNV Placing here to include VS_BACKGROUND_INPUT
};

//--------------------------------------------------------------------------------------------
// VS11_NPS_ACTOR - vertex lighting for a when you don't have a pixel shader
//--------------------------------------------------------------------------------------------
struct VS11_ANIMATED_INPUT
{
	float4 Position		: POSITION; 
	float2 DiffuseMap	: TEXCOORD0;
	int4   Indices		: BLENDINDICES; // the indices are premultiplied by 3 to get the proper bone register
	float4 Weights		: BLENDWEIGHT;
};

#endif //__VERTEX_INPUT_STRUCTS__