//----------------------------------------------------------------------------
// dxC_hdrange.h
//
// - Header for DX-specific HDR
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_HDRANGE__
#define __DXC_HDRANGE__

#include "e_hdrange.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum HDR_TECHNIQUE_INDEX
{
	HDR_TECHNIQUE_LUMINANCE = 0,
	HDR_TECHNIQUE_FINALPASS,
	// count
	NUM_HDR_TECHNIQUES
};

enum HDR_LUMINANCE_PASS_INDEX
{
	HDR_LUMINANCE_PASS_GREYSCALE = 0,
	HDR_LUMINANCE_PASS_DOWNSAMPLE,
	HDR_LUMINANCE_PASS_DEBUGDISPLAY
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

namespace HDRScene
{
	//HRESULT						CreateResources( const D3DSURFACE_DESC* pDisplayDesc );
	//HRESULT						DestroyResources();
	//SPD3DCSHADERRESOURCEVIEW	GetOutputTexture();
	SPD3DCRENDERTARGETVIEW		GetRenderTarget();
	DWORD						CalculateResourceUsage();
};

namespace Luminance
{
	//HRESULT						CreateResources();
	//HRESULT						DestroyResources();
	HRESULT						MeasureLuminance();
	//SPD3DCSHADERRESOURCEVIEW	GetLuminanceTexture();
	DWORD						ComputeResourceUsage();
};

namespace HDRPipeline
{
	HRESULT						CreateResources();
	HRESULT						DestroyResources();
	HRESULT						ApplyLuminance();
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_RestoreHDRMode();

#endif // __DXC_HDRANGE__
