//----------------------------------------------------------------------------
// dxC_enums.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_ENUMS__
#define __DXC_ENUMS__

#include "dxC_macros.h"

#if defined(ENGINE_TARGET_DX9)
	#include <d3d9types.h>
#elif defined(ENGINE_TARGET_DX10)
	typedef DWORD D3DCOLOR;
	//#define D3DCOLOR_DEFINED
	//#include <d3d9types.h>  //Including for now, perhaps we can get rid of it later
	#include <DXGIType.h>
#endif



#ifdef ENGINE_TARGET_DX10
	#undef D3DCLEAR_ZBUFFER
	#undef D3DCLEAR_STENCIL
	#undef D3DCLEAR_TARGET
	enum
	{
		D3DCLEAR_ZBUFFER = D3D10_CLEAR_DEPTH,
		D3DCLEAR_STENCIL = D3D10_CLEAR_STENCIL,
		D3DCLEAR_TARGET  = 4, //KMNV Hack
	};
#endif

#endif //__DXC_ENUMS__