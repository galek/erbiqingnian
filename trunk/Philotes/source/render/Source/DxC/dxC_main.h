//----------------------------------------------------------------------------
// dxC_main.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_MAIN__
#define __DX9_MAIN__

#include "e_main.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern BOOL gbBlurInitialized;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_AddRenderToTexture( int nModelID, SPD3DCTEXTURE2D pDestTexture, RECT * pDestVPRect, int nEnvDefID = INVALID_ID, RECT * pDestSpacingRect = NULL );

PRESULT e_ParticlesDrawLightmap( MATRIX& matView, MATRIX& matProj, MATRIX& matProj2, VECTOR& vEyeLocation, VECTOR& vEyeVector );
PRESULT e_ParticlesDrawPreLightmap( MATRIX& matView, MATRIX& matProj, MATRIX& matProj2, VECTOR& vEyeLocation, VECTOR& vEyeVector );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void dxC_SetWantFullscreen( BOOL bWantFullscreen )
{
	extern BOOL gbWantFullscreen;
	gbWantFullscreen = bWantFullscreen;
}

inline BOOL dxC_GetWantFullscreen()
{
	extern BOOL gbWantFullscreen;
	return gbWantFullscreen;
}

#endif // __DX9_MAIN__