//----------------------------------------------------------------------------
// dxC_2d.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_2D__
#define __DX9_2D__

#include "e_2d.h"
#include "dxC_FVF.h"
#include "dxC_effect.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define NUM_SCREEN_COVER_VERTICES 6

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum {
	LAYER_BLUR_FILTER=0,
	LAYER_BLUR_X,
	LAYER_BLUR_Y,
	LAYER_COMBINE,
	LAYER_OVERLAY,
	LAYER_FADE,
	LAYER_DOWNSAMPLE,
	LAYER_GAUSS_S_X,
	LAYER_GAUSS_S_Y,
	LAYER_GAUSS_L_X,
	LAYER_GAUSS_L_Y,
	LAYER_MAX
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

typedef struct {
	VECTOR vPosition;
	//	float fRHW;
	float pfTexCoords[ 2 ];
} SCREEN_COVER_VERTEX;

struct LINEVERTEX
{
	D3DXVECTOR3	vPosition;
	D3DCOLOR	dwColor;
};

struct D3D_SCREEN_LAYER
{
	int	nEffect;
	int nOption;

	EFFECT_TECHNIQUE_CACHE tTechniqueCache;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern D3D_SCREEN_LAYER gtScreenLayers[];
extern struct D3D_VERTEX_BUFFER_DEFINITION gtScreenCoverVertexBufferDef;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void dx9_AddScreenLayer( int nID, int nEffect, int nOption );
PRESULT dx9_ScreenBuffersInit();
PRESULT dx9_ScreenBuffersDestroy();
PRESULT dx9_ScreenDrawRestore();
PRESULT dx9_ScreenDrawRelease();
PRESULT dx9_ScreenDrawCover(D3D_EFFECT * pEffect);	// CHB 2007.10.02

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DX9_2D__