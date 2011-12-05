//----------------------------------------------------------------------------
// dxC_types.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_TYPES__
#define __DXC_TYPES__

#include "dxC_macros.h"

#if defined(ENGINE_TARGET_DX9)
	#include <d3d9.h>
	#include <d3dx9tex.h>
	#include <d3d9types.h>
	#include "dx9_types.h"
#elif defined(ENGINE_TARGET_DX10)
	#include <d3d10.h>
	#include <d3dx10.h>
	#include <d3dx10tex.h>
	#include "dx10_types.h"
#endif

#include <dxdiag.h>

#include "dxC_remapper_def.h"

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
// Interfaces
//Effect
//DXC_TYPEDEF_LP( ID3DEffect,				ID3D10Effect)				LPD3DCEFFECT;
//DXC_TYPEDEF_SP( ID3DEffect,				ID3D10Effect)				SPD3DCEFFECT;

//Base texture
DXC_TYPEDEF_LP( IDirect3DBaseTexture9,	ID3D10Resource)				LPD3DCBASETEXTURE;
DXC_TYPEDEF_SP( IDirect3DBaseTexture9,	ID3D10Resource)				SPD3DCBASETEXTURE;

//Base Resource
DXC_TYPEDEF_LP( IDirect3DResource9,		ID3D10Resource)				LPD3DCBASERESOURCE;
DXC_TYPEDEF_SP( IDirect3DResource9,		ID3D10Resource)				SPD3DCBASERESOURCE;

//Base buffer resource
DXC_TYPEDEF_LP( IDirect3DResource9,		ID3D10Buffer)				LPD3DCBASEBUFFER;
DXC_TYPEDEF_SP( IDirect3DResource9,		ID3D10Buffer)				SPD3DCBASEBUFFER;

//Base Shader resource view
DXC_TYPEDEF_LP( IDirect3DBaseTexture9,	ID3D10ShaderResourceView)	LPD3DCBASESHADERRESOURCEVIEW;
DXC_TYPEDEF_SP( IDirect3DBaseTexture9,	ID3D10ShaderResourceView)	SPD3DCBASESHADERRESOURCEVIEW;

//Texture (base resource, in DX10 we don't interact with this directly)
DXC_TYPEDEF_LP( IDirect3DTexture9,		ID3D10Texture2D)			LPD3DCTEXTURE2D;
DXC_TYPEDEF_SP(	IDirect3DTexture9,		ID3D10Texture2D)			SPD3DCTEXTURE2D;

DXC_TYPEDEF_LP( IDirect3DTexture9,		ID3D10Texture1D)			LPD3DCTEXTURE1D;
DXC_TYPEDEF_SP(	IDirect3DTexture9,		ID3D10Texture1D)			SPD3DCTEXTURE1D;

//Shader resource view
DXC_TYPEDEF_LP(	IDirect3DTexture9,		ID3D10ShaderResourceView)	LPD3DCSHADERRESOURCEVIEW;
DXC_TYPEDEF_SP(	IDirect3DTexture9,		ID3D10ShaderResourceView)	SPD3DCSHADERRESOURCEVIEW;

//Render target view
DXC_TYPEDEF_LP(	IDirect3DSurface9,		ID3D10RenderTargetView)		LPD3DCRENDERTARGETVIEW;
DXC_TYPEDEF_SP(	IDirect3DSurface9,		ID3D10RenderTargetView)		SPD3DCRENDERTARGETVIEW;

//Depth stencil view
DXC_TYPEDEF_LP(	IDirect3DSurface9,		ID3D10DepthStencilView)		LPD3DCDEPTHSTENCILVIEW;
DXC_TYPEDEF_SP(	IDirect3DSurface9,		ID3D10DepthStencilView)		SPD3DCDEPTHSTENCILVIEW;

//Cube Texture
DXC_TYPEDEF_LP(	IDirect3DCubeTexture9,	ID3D10Texture2D)			LPD3DCTEXTURECUBE;
DXC_TYPEDEF_SP(	IDirect3DCubeTexture9,	ID3D10Texture2D)			SPD3DCTEXTURECUBE;

//D3D Device 
DXC_TYPEDEF_LP(	IDirect3DDevice9,		ID3D10Device)				LPD3DCDEVICE;
DXC_TYPEDEF_SP(	IDirect3DDevice9,		ID3D10Device)				SPD3DCDEVICE;

//Query
DXC_TYPEDEF_LP(	IDirect3DQuery9,		ID3D10Query)				LPD3DCQUERY;
DXC_TYPEDEF_SP(	IDirect3DQuery9,		ID3D10Query)				SPD3DCQUERY;

////Index buffer
DXC_TYPEDEF_LP(	IDirect3DIndexBuffer9,	ID3D10Buffer)				LPD3DCIB;
DXC_TYPEDEF_SP(	IDirect3DIndexBuffer9,	ID3D10Buffer)				SPD3DCIB;

//Vertex buffer
DXC_TYPEDEF_LP(	IDirect3DVertexBuffer9,	ID3D10Buffer)				LPD3DCVB;
DXC_TYPEDEF_SP(	IDirect3DVertexBuffer9,	ID3D10Buffer)				SPD3DCVB;

//Input assembler layout
DXC_TYPEDEF_LP( IDirect3DVertexDeclaration9, ID3D10InputLayout)		LPD3DCIALAYOUT;
DXC_TYPEDEF_SP( IDirect3DVertexDeclaration9, ID3D10InputLayout)		SPD3DCIALAYOUT;


//Effect interface
DXC_TYPEDEF_LP(	ID3DXEffect,			ID3D10Effect)				LPD3DCEFFECT;
DXC_TYPEDEF_SP(	ID3DXEffect,			ID3D10Effect)				SPD3DCEFFECT;

//Effect pool
DXC_TYPEDEF_SP( ID3DXEffectPool,		ID3D10EffectPool)			SPD3DCEFFECTPOOL;
DXC_TYPEDEF_LP( ID3DXEffectPool,		ID3D10EffectPool)			LPD3DCEFFECTPOOL;

//D3DX Sprite
DXC_TYPEDEF_LP( ID3DXSprite,			ID3DX10Sprite)				LPD3DCSPRITE;
DXC_TYPEDEF_SP( ID3DXSprite,			ID3DX10Sprite)				SPD3DCSPRITE;

//D3DCX Include
DXC_TYPEDEF_LP( ID3DXInclude,			ID3D10Include)				LPD3DCXINCLUDE;
DXC_TYPEDEF_SP( ID3DXInclude,			ID3D10Include)				SPD3DCXINCLUDE;

//D3D Adapter
DXC_TYPEDEF(	DWORD,					IDXGIAdapter*)				DXCADAPTER;

//Swap chain
DXC_TYPEDEF_LP( IDirect3DSwapChain9,	IDXGISwapChain)				LPD3DCSWAPCHAIN;
DXC_TYPEDEF_SP( IDirect3DSwapChain9,	IDXGISwapChain)				SPD3DCSWAPCHAIN;

//Buffer
DXC_TYPEDEF_LP( ID3DXBuffer,			ID3D10Blob)					LPD3DCBLOB;
DXC_TYPEDEF_SP( ID3DXBuffer,			ID3D10Blob)					SPD3DCBLOB;


// DXDiag
typedef CComPtr<IDxDiagProvider>				SPDXDIAGPROVIDER;
typedef CComPtr<IDxDiagContainer>				SPDXDIAGCONTAINER;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Descriptions

//Vertex
DXC_TYPEDEF(		D3DVERTEXELEMENT9,		D3D10_INPUT_ELEMENT_DESC)	D3DC_INPUT_ELEMENT_DESC;

DXC_TYPEDEF(		D3DDEVTYPE,				D3D10_DRIVER_TYPE)			D3DC_DRIVER_TYPE;
//

DXC_TYPEDEF(		D3DGAMMARAMP,			DXGI_GAMMA_CONTROL)			D3DC_GAMMA_CONTROL;

DXC_TYPEDEF(		DWORD,					UINT64)						D3DC_QUERY_DATA;

//MSAA
#if defined(ENGINE_TARGET_DX9)
	typedef struct
	{
		D3DMULTISAMPLE_TYPE Count;
		DWORD Quality;
	} DXC_SAMPLE_DESC;
#elif defined(ENGINE_TARGET_DX10)
	typedef DXGI_SAMPLE_DESC DXC_SAMPLE_DESC;
#endif


//Filter
DXC_TYPEDEF(		D3DTEXTUREFILTERTYPE,	D3D10_FILTER_TYPE)					D3DC_FILTER;

//D3DX Filter type
DXC_TYPEDEF(		DWORD,					D3DX10_FILTER_FLAG)					D3DCX_FILTER;

//Texture address mode
DXC_TYPEDEF(		D3DTEXTUREADDRESS,		D3D10_TEXTURE_ADDRESS_MODE)			D3DC_TEXTURE_ADDRESS_MODE;

DXC_TYPEDEF(		D3DSURFACE_DESC,		D3D10_TEXTURE2D_DESC)				D3DC_TEXTURE2D_DESC;

DXC_TYPEDEF(		D3DSURFACE_DESC,		D3D10_RENDER_TARGET_VIEW_DESC)		D3DC_RENDER_TARGET_VIEW_DESC;
DXC_TYPEDEF(		D3DSURFACE_DESC,		D3D10_DEPTH_STENCIL_VIEW_DESC)		D3DC_DEPTH_STENCIL_VIEW_DESC;
DXC_TYPEDEF(		D3DSURFACE_DESC,		D3D10_SHADER_RESOURCE_VIEW_DESC)	D3DC_SHADER_RESOURCE_VIEW_DESC;



DXC_TYPEDEF(		D3DXHANDLE,	ID3D10EffectTechnique*)					D3DC_TECHNIQUE_HANDLE; 

DXC_TYPEDEF(		D3DXHANDLE,	ID3D10EffectVariable*)					D3DC_EFFECT_VARIABLE_HANDLE;

DXC_TYPEDEF(		D3DXHANDLE,	ID3D10EffectPass*)						D3DC_EFFECT_PASS_HANDLE;

DXC_TYPEDEF(		D3DXHANDLE,	ID3D10EffectShaderResourceVariable*)	D3DC_EFFECT_SRVAR_HANDLE;

DXC_TYPEDEF(D3DXTECHNIQUE_DESC, D3D10_TECHNIQUE_DESC)					D3DC_TECHNIQUE_DESC;

DXC_TYPEDEF( D3DXEFFECT_DESC, D3D10_EFFECT_DESC )						D3DC_EFFECT_DESC;

DXC_TYPEDEF( D3DXPASS_DESC, D3D10_PASS_DESC )							D3DC_PASS_DESC;	

DXC_TYPEDEF( D3DXMACRO, D3D10_SHADER_MACRO )							D3DC_SHADER_MACRO;

#ifdef ENGINE_TARGET_DX10
	#define D3DC_VIEWPORT D3D10_VIEWPORT
#elif defined(ENGINE_TARGET_DX9)
	struct D3DC_VIEWPORT : D3DVIEWPORT9
	{
	public:
		remapperVar<FLOAT> MinDepth;
		remapperVar<FLOAT> MaxDepth;
		remapperVar<DWORD> TopLeftX;
		remapperVar<DWORD> TopLeftY;

		REMAPPER_CONSTRUCTOR( D3DC_VIEWPORT )

		void setRemappers()
		{
			TopLeftX.set( &X );
			TopLeftY.set( &Y );
			MinDepth.set( &MinZ );
			MaxDepth.set( &MaxZ );
		}
	};
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Types

//D3D Formats
DXC_TYPEDEF(		D3DFORMAT,				DXGI_FORMAT)				DXC_FORMAT;

//File format
DXC_TYPEDEF(		D3DXIMAGE_FILEFORMAT,	D3DX10_IMAGE_FILE_FORMAT)	D3DC_IMAGE_FILE_FORMAT;

//Primitive type
DXC_TYPEDEF(		D3DPRIMITIVETYPE,		D3D10_PRIMITIVE_TOPOLOGY)	D3DC_PRIMITIVE_TOPOLOGY;

// CHB 2007.03.05 - multisample type
DXC_TYPEDEF(		D3DMULTISAMPLE_TYPE,	int)						DXC_MULTISAMPLE_TYPE;

typedef enum 
{
	D3DC_USAGE_INVALID,

	D3DC_USAGE_1DTEX,

	D3DC_USAGE_2DTEX_SCRATCH,
	D3DC_USAGE_2DTEX_STAGING, 
	D3DC_USAGE_2DTEX,
	D3DC_USAGE_2DTEX_RT,
	D3DC_USAGE_2DTEX_DS,
	D3DC_USAGE_2DTEX_SM,  //SHADOW MAP: In our engine a shadowmap is a depth stencil surface that will be used as a texture (have a shader resource view)
	D3DC_USAGE_2DTEX_DEFAULT,

	D3DC_USAGE_CUBETEX_DYNAMIC,

	D3DC_USAGE_BUFFER_MUTABLE,			// used for frequent locking and updating
	D3DC_USAGE_BUFFER_REGULAR,			// common case: infrequently changes
	D3DC_USAGE_BUFFER_CONSTANT,			// really, truly permanent -- may lock only on create
	D3D10_USAGE_BUFFER_STREAMOUT,

} D3DC_USAGE;

struct E_SCREEN_DISPLAY_MODE;	// CHB 2007.09.07

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif //__DXC_TYPES__
