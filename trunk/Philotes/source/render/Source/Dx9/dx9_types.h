//----------------------------------------------------------------------------
// dx9_types.h
//
// This file provides all DirectX interface smart pointer types
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_TYPES__
#define __DX9_TYPES__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#include "dx9_compileflags.h"

#include <d3d9.h>
#include <d3dx9.h>
//#include <dinput.h>  KMNV: Moved up to dxC_input.h
#include <atlcomcli.h>

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
// Direct3D
typedef CComPtr<IDirect3D9>						SPDIRECT3D9;
typedef CComPtr<IDirect3DBaseTexture9>			SPDIRECT3DBASETEXTURE9;
typedef CComPtr<IDirect3DCubeTexture9>			SPDIRECT3DCUBETEXTURE9;
typedef CComPtr<IDirect3DDevice9>				SPDIRECT3DDEVICE9;
//typedef CComPtr<IDirect3DIndexBuffer9>		SPDIRECT3DINDEXBUFFER9;
typedef CComPtr<IDirect3DPixelShader9>			SPDIRECT3DPIXELSHADER9;
//typedef CComPtr<IDirect3DQuery9>				SPDIRECT3DQUERY9;
typedef CComPtr<IDirect3DResource9>				SPDIRECT3DRESOURCE9;
typedef CComPtr<IDirect3DStateBlock9>			SPDIRECT3DSTATEBLOCK9;
typedef CComPtr<IDirect3DSurface9>				SPDIRECT3DSURFACE9;
//typedef CComPtr<IDirect3DSwapChain9>			SPDIRECT3DSWAPCHAIN9;
//typedef CComPtr<IDirect3DTexture9>			SPDIRECT3DTEXTURE9;
//typedef CComPtr<IDirect3DVertexBuffer9>		SPDIRECT3DVERTEXBUFFER9;
//typedef CComPtr<IDirect3DVertexDeclaration9>	SPDIRECT3DVERTEXDECLARATION9;
typedef CComPtr<IDirect3DVertexShader9>			SPDIRECT3DVERTEXSHADER9;
typedef CComPtr<IDirect3DVolume9>				SPDIRECT3DVOLUME9;
typedef CComPtr<IDirect3DVolumeTexture9>		SPDIRECT3DVOLUMETEXTURE9;
// D3DX
//typedef CComPtr<ID3DXBuffer>					SPD3DXBUFFER;
//typedef CComPtr<ID3DXEffectPool>				SPD3DXEFFECTPOOL;
//typedef CComPtr<ID3DXSprite>					SPD3DXSPRITE;
//typedef CComPtr<ID3DXEffect>					SPD3DXEFFECT;
typedef CComPtr<ID3DXEffectCompiler>			SPD3DXEFFECTCOMPILER;
typedef CComPtr<ID3DXEffectStateManager>		SPD3DXEFFECTSTATEMANAGER;
//typedef CComPtr<ID3DXFragmentLinker>			SPD3DXFRAGMENTLINKER;

/*
KMNV: Moved up to dxC_input.h
// DirectInput
typedef CComPtr<IDirectInput8>					SPDIRECTINPUT8;
typedef CComPtr<IDirectInputDevice8>			SPDIRECTINPUTDEVICE8;
*/

/*
We're using FMOD now -- no need for these
// DirectSound
typedef CComPtr<IDirectSound8>					SPDIRECTSOUND8;
typedef CComPtr<IDirectSoundBuffer>				SPDIRECTSOUNDBUFFER;
typedef CComPtr<IDirectSoundBuffer8>			SPDIRECTSOUNDBUFFER8;
typedef CComPtr<IDirectSound3DBuffer8>			SPDIRECTSOUND3DBUFFER8;
typedef CComQIPtr<IDirectSound3DListener8,&IID_IDirectSound3DListener8>	SPDIRECTSOUND3DLISTENER8;
// */

const unsigned DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED = 0;
const unsigned DXGI_MODE_SCALING_UNSPECIFIED = 0;


#endif // __DX9_TYPES__