#define DX10_BLEND_STATES		DXC_BLEND_RGBA_ONE_ONE_ADD;			DXC_DEPTHSTENCIL_ZREAD;
#define DX9_BLEND_STATES		SrcBlend = ONE;		DestBlend = ONE;	ZWRITEENABLE = FALSE;	BlendOp = ADD;
#define ADDITIVE_BLEND			1

#include "Dx9/_Spectral.fxh"

