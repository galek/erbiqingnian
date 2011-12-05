//----------------------------------------------------------------------------
// dxC_FVF.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_FVF__
#define __DXC_FVF__

typedef enum _D3DC_FVF
{
	BOUNDING_BOX_FVF				=	DXC_9_10( ( D3DFVF_XYZ ), NULL ),
	VERTEX_ONLY_FORMAT				=	DXC_9_10( ( D3DFVF_XYZ ), NULL ),
	D3DFVF_LINEVERT					=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_DIFFUSE ), NULL ),
	SCREEN_COVER_FVF				=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_TEX1 ), NULL ),
	D3DFVF_PARTICLE_QUAD_VERTEX		=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_PSIZE ), NULL ),
	D3DC_FVF_STD					=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1), NULL ),
	DEBUG_PRIMITIVE_WORLD_FVF		=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_DIFFUSE ), NULL ),
	DEBUG_PRIMITIVE_SCREEN_FVF		=	DXC_9_10( ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE ), NULL ),
	VISIBILITY_FVF					=	DXC_9_10( ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 ), NULL ),
	D3DC_FVF_NULL					= NULL
} D3DC_FVF;

#endif //__DXC_FVF__