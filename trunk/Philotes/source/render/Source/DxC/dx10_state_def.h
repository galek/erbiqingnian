//----------------------------------------------------------------------------
// dx10_state_def.h
// States that transfer from dx9
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __DX10_STATE_DEF__
#define __DX10_STATE_DEF__

#define D3DXPARAMETER_DESC D3D10_EFFECT_VARIABLE_DESC

#define D3DMULTISAMPLE_NONE			1
#define D3DMULTISAMPLE_2_SAMPLES	2
#define D3DMULTISAMPLE_4_SAMPLES	4
#define D3DMULTISAMPLE_8_SAMPLES	8
#define D3DMULTISAMPLE_16_SAMPLES   16

#define	D3DX_FROM_FILE				D3DX10_DEFAULT

#define D3DDEVTYPE					D3D10_DRIVER_TYPE

#define D3DTADDRESS_CLAMP			D3D10_TEXTURE_ADDRESS_CLAMP
#define D3DTADDRESS_WRAP			D3D10_TEXTURE_ADDRESS_WRAP
#define D3DTADDRESS_MIRROR			D3D10_TEXTURE_ADDRESS_MIRROR

//D3DRESOURCETYPE
    //D3DRTYPE_SURFACE                =  1,
    //D3DRTYPE_VOLUME                 =  2,
    //D3DRTYPE_CUBETEXTURE            =  5,
#define D3DRESOURCETYPE				D3D10_RESOURCE_DIMENSION

#define D3DRTYPE_TEXTURE			D3D10_RESOURCE_DIMENSION_TEXTURE2D
#define D3DRTYPE_CUBETEXTURE		D3D10_RESOURCE_DIMENSION_TEXTURE2D
#define D3DRTYPE_VOLUMETEXTURE		D3D10_RESOURCE_DIMENSION_TEXTURE3D
#define D3DRTYPE_VERTEXBUFFER		D3D10_RESOURCE_DIMENSION_BUFFER
#define D3DRTYPE_INDEXBUFFER		D3D10_RESOURCE_DIMENSION_BUFFER


// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)

#define D3DCOLOR_XYUV(y,u,v)   D3DCOLOR_ARGB(0xff,y,u,v)
#define D3DCOLOR_AYUV(a,y,u,v) D3DCOLOR_ARGB(a,y,u,v)

//Swap effect
//----------------------------------------------------------------------------	
#define D3DSWAPEFFECT_DISCARD		DXGI_SWAP_EFFECT_DISCARD

//Blending
//----------------------------------------------------------------------------	
#define D3DBLENDOP_ADD				D3D10_BLEND_OP_ADD
#define D3DBLENDOP_MAX				D3D10_BLEND_OP_MAX
#define D3DBLENDOP_REVSUBTRACT      D3D10_BLEND_OP_REV_SUBTRACT
#define D3DBLEND_ONE				D3D10_BLEND_ONE
#define D3DBLEND_ZERO				D3D10_BLEND_ZERO
#define D3DBLEND_SRCALPHA           D3D10_BLEND_SRC_ALPHA
#define D3DBLEND_INVSRCALPHA        D3D10_BLEND_INV_SRC_ALPHA
#define D3DBLEND_SRCCOLOR           D3D10_BLEND_SRC_COLOR


//Zbuffer
typedef enum _D3DZBUFFERTYPE {
    D3DZB_FALSE                 = 0,
    D3DZB_TRUE                  = 1, // Z buffering
    D3DZB_USEW                  = 2, // W buffering
    D3DZB_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
} D3DZBUFFERTYPE;

typedef enum _D3DCUBEMAP_FACES
{
    D3DCUBEMAP_FACE_POSITIVE_X     = 0,
    D3DCUBEMAP_FACE_NEGATIVE_X     = 1,
    D3DCUBEMAP_FACE_POSITIVE_Y     = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y     = 3,
    D3DCUBEMAP_FACE_POSITIVE_Z     = 4,
    D3DCUBEMAP_FACE_NEGATIVE_Z     = 5,

    D3DCUBEMAP_FACE_FORCE_DWORD    = 0x7fffffff
} D3DCUBEMAP_FACES;

typedef enum _D3DTEXTURESTAGESTATETYPE
{
	D3DTSS_COLOROP,
	D3DTSS_ALPHAOP
} D3DTEXTURESTAGESTATETYPE;

typedef enum D3DTEXTUREOP
{
    D3DTOP_DISABLE,
    D3DTOP_SELECTARG1,
    D3DTOP_SELECTARG2,
    D3DTOP_MODULATE,
} D3DTEXTUREOP;

//Primitive types
//----------------------------------------------------------------------------	
#define D3DPT_POINTLIST             D3D10_PRIMITIVE_TOPOLOGY_POINTLIST
#define D3DPT_LINELIST				D3D10_PRIMITIVE_TOPOLOGY_LINELIST
#define D3DPT_LINESTRIP             D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP
#define D3DPT_TRIANGLELIST          D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST
#define D3DPT_TRIANGLESTRIP			D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP

//Matrices
//----------------------------------------------------------------------------
typedef enum
{
	D3DTS_WORLD,
	D3DTS_VIEW,
	D3DTS_PROJECTION,
} D3DTRANSFORMSTATETYPE;


//D3D Filter Types
//----------------------------------------------------------------------------
#define D3DTEXF_POINT					D3D10_FILTER_TYPE_POINT
#define D3DTEXF_LINEAR					D3D10_FILTER_TYPE_LINEAR
#define D3DTEXF_ANISOTROPIC				D3D10_FILTER_ANISOTROPIC

//D3DX Filter Types
//----------------------------------------------------------------------------
//#define D3DX_DEFAULT					(D3DX10_FILTER_TRIANGLE | D3DX10_FILTER_DITHER))
#define D3DX_FILTER_NONE				D3DX10_FILTER_NONE
#define D3DX_FILTER_POINT				D3DX10_FILTER_POINT
#define D3DX_FILTER_LINEAR				D3DX10_FILTER_LINEAR
#define D3DX_FILTER_TRIANGLE			D3DX10_FILTER_TRIANGLE
#define D3DX_FILTER_BOX					D3DX10_FILTER_BOX
#define D3DX_FILTER_MIRROR_U			D3DX10_FILTER_MIRROR_U
#define D3DX_FILTER_MIRROR_V			D3DX10_FILTER_MIRROR_V
#define D3DX_FILTER_MIRROR_W			D3DX10_FILTER_MIRROR_W
#define D3DX_FILTER_MIRROR				D3DX10_FILTER_MIRROR
#define D3DX_FILTER_DITHER_DIFFUSION	D3DX10_FILTER_DITHER_DIFFUSION
#define D3DX_FILTER_DITHER				D3DX10_FILTER_DITHER
#define D3DX_FILTER_SRGB_IN				0	//D3DX10_FILTER_SRGB_IN			// apparently, DX10 sRGB filter flags have been removed and are instead reflected in formats
#define D3DX_FILTER_SRGB_OUT			0	//D3DX10_FILTER_SRGB_OUT
#define D3DX_FILTER_SRGB				0	//D3DX10_FILTER_SRGB

//Formats
//----------------------------------------------------------------------------
#define D3DFMT_FROM_FILE			DXGI_FORMAT_FROM_FILE
#define D3DFORMAT					DXGI_FORMAT
#define D3DFMT_UNKNOWN              DXGI_FORMAT_UNKNOWN

#define D3DFMT_A8R8G8B8             DXGI_FORMAT_R8G8B8A8_UNORM// sbustituting for DXGI_FORMAT_B8G8R8A8_UNORM  Even though this is in DXGI it's not supported in DX10...huh
#define D3DFMT_A8B8G8R8             DXGI_FORMAT_R8G8B8A8_UNORM

#define D3DFMT_X8R8G8B8             DXGI_FORMAT_B8G8R8X8_UNORM
#define D3DFMT_R8G8B8				DXGI_FORMAT_B8G8R8X8_UNORM  //HACK Shouldn't cause any problems for our use
#define D3DFMT_R5G6B5               DXGI_FORMAT_B5G6R5_UNORM
#define D3DFMT_A1R5G5B5             DXGI_FORMAT_B5G5R5A1_UNORM
#define D3DFMT_A4R4G4B4             DXGI_FORMAT_R8G8B8A8_UNORM// ARGB4 isn't supported in DX10, so emulating with 8888
#define D3DFMT_A8                   DXGI_FORMAT_A8_UNORM
#define D3DFMT_A2B10G10R10          DXGI_FORMAT_R10G10B10A2_UNORM


#define D3DFMT_G16R16               DXGI_FORMAT_R16G16_UNORM

#define D3DFMT_A16B16G16R16         DXGI_FORMAT_R16G16B16A16_UNORM

#define D3DFMT_L8                   DXGI_FORMAT_R8_UNORM //Note: Use .r swizzle in shader to duplicate red to other components to get D3D9 behavior

#define D3DFMT_V8U8                 DXGI_FORMAT_R8G8_SNORM

#define D3DFMT_Q8W8V8U8             DXGI_FORMAT_R8G8B8A8_SNORM
#define D3DFMT_V16U16               DXGI_FORMAT_R16G16_SNORM

#define D3DFMT_R8G8_B8G8            DXGI_FORMAT_G8R8_G8B8_UNORM // (in DX9 the data was scaled up by 255.0f but this can be handled in shader code).

#define D3DFMT_G8R8_G8B8            DXGI_FORMAT_R8G8_B8G8_UNORM //(in DX9 the data was scaled up by 255.0f but this can be handled in shader code).
#define D3DFMT_DXT1                 DXGI_FORMAT_BC1_UNORM
#define D3DFMT_DXT2                 DXGI_FORMAT_BC2_UNORM
#define D3DFMT_DXT3                 DXGI_FORMAT_BC2_UNORM
#define D3DFMT_DXT4                 DXGI_FORMAT_BC3_UNORM
#define D3DFMT_DXT5                 DXGI_FORMAT_BC3_UNORM

#define D3DFMT_D16_LOCKABLE         DXGI_FORMAT_D16_UNORM

#define D3DFMT_D24S8                DXGI_FORMAT_R24G8_TYPELESS
#define D3DFMT_D24X8				DXGI_FORMAT_R24G8_TYPELESS  //Not exactly correct but ok

#define D3DFMT_D16                  DXGI_FORMAT_R16_TYPELESS

#define D3DFMT_D32F_LOCKABLE        DXGI_FORMAT_R32_TYPELESS

#define D3DFMT_L16                  DXGI_FORMAT_R16_UNORM  //Note: Use .r swizzle in shader to duplicate red to other components to get D3D9 behavior


#define D3DFMT_INDEX16              DXGI_FORMAT_R16_UINT
#define D3DFMT_INDEX32              DXGI_FORMAT_R32_UINT

#define D3DFMT_Q16W16V16U16         DXGI_FORMAT_R16G16B16A16_SNORM
// Floating point surface formats

// s10e5 formats (16-bits per channel)
#define D3DFMT_R16F                 DXGI_FORMAT_R16_FLOAT
#define D3DFMT_G16R16F              DXGI_FORMAT_R16G16_TYPELESS
#define D3DFMT_A16B16G16R16F        DXGI_FORMAT_R16G16B16A16_FLOAT

// IEEE s23e8 formats (32-bits per channel)
#define D3DFMT_R32F                 DXGI_FORMAT_R32_TYPELESS
#define D3DFMT_G32R32F              DXGI_FORMAT_R32G32_FLOAT
#define D3DFMT_A32B32G32R32F        DXGI_FORMAT_R32G32B32A32_FLOAT

#define D3DXIFF_BMP D3DX10_IFF_BMP
#define D3DXIFF_JPG D3DX10_IFF_JPG
#define D3DXIFF_PNG D3DX10_IFF_PNG
#define D3DXIFF_DDS D3DX10_IFF_DDS
#define D3DXIFF_TGA 420 //KMNV HACK HACK

	//D3DX10IFF_TIFF = 10,
	///D3DX10IFF_GIF = 11,

//enum
//{
//	D3DFMT_R8G8B8,
//	D3DFMT_X1R5G5B5,
//	D3DFMT_A4R4G4B4,
//	D3DFMT_R3G3B2,
//	D3DFMT_A8R3G3B2,
//	D3DFMT_X4R4G4B4,
//	D3DFMT_X8B8G8R8,
//	D3DFMT_A2R10G10B10,
//	D3DFMT_A8P8,
//	D3DFMT_P8,
//	D3DFMT_L6V5U5,
//	D3DFMT_X8L8V8U8,
//	D3DFMT_A2W10V10U10,
//	D3DFMT_UYVY,
//	D3DFMT_CxV8U8,
//	D3DFMT_MULTI2_ARGB8,
//	D3DFMT_VERTEXDATA,
//	D3DFMT_D24FS8,
//	D3DFMT_D24X8,
//	D3DFMT_D24X4S4,
//	D3DFMT_D32,
//	D3DFMT_D15S1,
//	D3DFMT_A8L8,
//	D3DFMT_A4L4,
//	D3DFMT_YUY2,
//} D3D10_UNSUPPORTED_FORMATS;


//Device type
#define D3DC_DRIVER_TYPE_HAL			D3D10_DRIVER_TYPE_HARDWARE
#define D3DC_DRIVER_TYPE_REF			D3D10_DRIVER_TYPE_REFERENCE

//Image info
#define D3DXIMAGE_INFO					D3DX10_IMAGE_INFO

//Sampler state type
typedef enum
{
    D3DSAMP_MAGFILTER      = 5,  /* D3DTEXTUREFILTER filter to use for magnification */
    D3DSAMP_MINFILTER      = 6,  /* D3DTEXTUREFILTER filter to use for minification */
    D3DSAMP_MIPFILTER      = 7,  /* D3DTEXTUREFILTER filter to use between mipmaps during minification */
} D3DCSAMPLERSTATETYPE;

//Query flush
#define D3DGETDATA_FLUSH 0
#define D3DGETDATA_DONOTFLUSH D3D10_GETDATA_DONOTFLUSH

//Cullmode
#define D3DCULL_CCW		D3D10_CULL_BACK
#define D3DCULL_CW		D3D10_CULL_FRONT
#define D3DCULL_NONE	D3D10_CULL_NONE

//Fillmode
#define D3DFILL_WIREFRAME	D3D10_FILL_WIREFRAME
#define D3DFILL_SOLID		D3D10_FILL_SOLID

//Compare functions
#define D3DCMP_NEVER		D3D10_COMPARISON_NEVER
#define D3DCMP_LESS			D3D10_COMPARISON_LESS
#define D3DCMP_EQUAL		D3D10_COMPARISON_EQUAL
#define D3DCMP_LESSEQUAL	D3D10_COMPARISON_LESS_EQUAL 
#define D3DCMP_GREATER		D3D10_COMPARISON_GREATER 
#define D3DCMP_NOTEQUAL		D3D10_COMPARISON_NOT_EQUAL 
#define D3DCMP_GREATEREQUAL D3D10_COMPARISON_GREATER_EQUAL 
#define D3DCMP_ALWAYS		D3D10_COMPARISON_ALWAYS

//Colorwrite
#define D3DCOLORWRITEENABLE_RED		D3D10_COLOR_WRITE_ENABLE_RED
#define D3DCOLORWRITEENABLE_GREEN	D3D10_COLOR_WRITE_ENABLE_GREEN
#define D3DCOLORWRITEENABLE_BLUE	D3D10_COLOR_WRITE_ENABLE_BLUE
#define D3DCOLORWRITEENABLE_ALPHA	D3D10_COLOR_WRITE_ENABLE_ALPHA

typedef enum
{
	D3DRS_FILLMODE, //FillMode
	D3DRS_CULLMODE, //CullMode
	//FrontCounterClockwise == FALSE, hardcoding to false so CCW/CW works
	D3DRS_DEPTHBIAS, //DepthBias
	D3DRS_SLOPESCALEDEPTHBIAS, //SlopeScaledDepthBias
	//DepthClipEnable
	D3DRS_SCISSORTESTENABLE, //ScissorEnable
	D3DRS_MULTISAMPLEENABLE,
	//AntialiasedLineEnable
} D3DRS_RASTER;

typedef enum 
{
	//Alpha to coverage
	D3DRS_ALPHABLENDENABLE, //D3DRS_ALPHABLENDENABLE[0]
	D3DRS_SRCBLEND, //SrcBlend
	D3DRS_DESTBLEND, //DestBlend
	D3DRS_BLENDOP, //BlendOp

	D3DRS_SRCBLENDALPHA, //SrcBlendAlpha
	D3DRS_DESTBLENDALPHA, //DestBlendAlpha
	D3DRS_BLENDOPALPHA, //BlendOpAlpha

	D3DRS_COLORWRITEENABLE, //UINT8 RenderTargetWriteMask[0]
	D3DRS_COLORWRITEENABLE1, //UINT8 RenderTargetWriteMask[1]
	D3DRS_COLORWRITEENABLE2, //UINT8 RenderTargetWriteMask[2]
	D3DRS_COLORWRITEENABLE3, //UINT8 RenderTargetWriteMask[3] 

	//States handled in shader
	//========================
	D3DRS_ALPHATESTENABLE,
	D3DRS_SRGBWRITEENABLE,
	D3DRS_ALPHAREF,
	D3DRS_ALPHAFUNC,
	//========================

	D3DRS_BLENDFACTOR,
	D3DRS_MULTISAMPLEMASK 

} D3DRS_BLEND;

typedef enum
{
	D3DRS_ZENABLE,  //DepthEnable
	D3DRS_ZWRITEENABLE,  //DepthWriteMask
	D3DRS_ZFUNC,  //DepthFunc
	
	D3DRS_STENCILENABLE, //StencilEnable
	D3DRS_STENCILMASK, //StencilReadMask
	D3DRS_STENCILWRITEMASK, //StencilWriteMask

	D3DRS_STENCILFAIL, //D3D10_DEPTH_STENCILOP_DESC FrontFace
	D3DRS_STENCILZFAIL, //D3D10_DEPTH_STENCILOP_DESC FrontFace
	D3DRS_STENCILPASS, //D3D10_DEPTH_STENCILOP_DESC FrontFace
	D3DRS_STENCILFUNC, //D3D10_DEPTH_STENCILOP_DESC FrontFace

    D3DRS_CCW_STENCILFAIL, //D3D10_DEPTH_STENCILOP_DESC BackFace
    D3DRS_CCW_STENCILZFAIL, //D3D10_DEPTH_STENCILOP_DESC BackFace
    D3DRS_CCW_STENCILPASS, //D3D10_DEPTH_STENCILOP_DESC BackFace
    D3DRS_CCW_STENCILFUNC, //D3D10_DEPTH_STENCILOP_DESC BackFace

	D3DRS_STENCILREF
} DRDRS_DS;


//Finally pull in the DX9 caps def, we'll need to fill this in later
#include <d3d9caps.h>
#endif //__DX10_STATE_DEF__