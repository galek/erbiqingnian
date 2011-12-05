//----------------------------------------------------------------------------
// e_hdrange.h
//
// - Header for generic HDR
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_HDRANGE__
#define __E_HDRANGE__

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum HDR_MODE
{
	HDR_MODE_INVALID = -1,
	HDR_MODE_NONE = 0,
	HDR_MODE_SRGB_LIGHTS,			// gamma-correct lighting values passed down to sRGB-space shader
	HDR_MODE_LINEAR_SHADERS,		// sRGB read textures into linear shader and sRGB write pixels
	HDR_MODE_LINEAR_FRAMEBUFFER,	// sRGB read textures into linear shader and write into linear buffer
	// count
	NUM_HDR_MODES
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

namespace HDRRendering
{
	PRESULT CreateResources( const D3DC_TEXTURE2D_DESC* pDisplayDesc );
	PRESULT DestroyResources( );
	//HRESULT RenderScene( );
	//HRESULT UpdateScene(float fTime, CModelViewerCamera *pCamera );
	HRESULT GetOutputTexture( SPD3DCSHADERRESOURCEVIEW *pTexture );
	//HRESULT DrawToScreen( IDirect3DDevice9 *pDevice, ID3DXFont *pFont, ID3DXSprite *pTextSprite, IDirect3DTexture9 *pArrowTex );
	DWORD   CalculateResourceUsage( );
};

namespace HDRLuminance
{

};

namespace HDRPostProcess
{

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

BOOL		e_SetHDRMode				( HDR_MODE eMode );
HDR_MODE	e_GetValidatedHDRMode();
BOOL		e_IsHDRModeSupported		( HDR_MODE eMode );
BOOL		e_TexturesReadSRGB();
BOOL		e_ColorsTranslateSRGB();
PRESULT		e_SRGBToLinear				( VECTOR * pvVector );

#endif // __E_SETTINGS__
