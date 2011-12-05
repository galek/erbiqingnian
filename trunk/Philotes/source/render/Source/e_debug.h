//----------------------------------------------------------------------------
// e_debug.h
//
// - Header for debug functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_DEBUG__
#define __E_DEBUG__


#include "boundingbox.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct RENDER_2D_AABB
{
	BOUNDING_BOX tBBox;
	DWORD dwColor;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern int gnDebugIdentifyModel;
extern CArrayIndexed<RENDER_2D_AABB> gtRender2DAABBs;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void e_DebugIdentifyModel( int nModelID );
void e_SetTraceQuery( unsigned int nAddress );
void e_SetTraceModel( int nModelID );
void e_DebugDumpDrawLists();
void e_DebugDumpOcclusion();
void e_ToggleForceGeometryNormals();
BOOL e_GetForceGeometryNormals();
void e_Clear2DBoxRenders();
void e_Add2DBoxRender( const BOUNDING_BOX * pBBox, DWORD dwColor );
PRESULT e_CreateDebugMipTextures();
PRESULT e_ReleaseDebugMipTextures();
void e_DebugShowInfo();
void e_SwitchToRef();

void e_SpringDebugDisplay( struct SPRING * pSpring, float fScale = 1.f );
void e_DebugSetBar( int nBar, const char * szName, int nValue, int nMax, int nGood, int nBad );
PRESULT e_DebugRenderAxes(
	const MATRIX & mSpace,
	float fScaleFactor = 1.f,
	VECTOR * pvWorldOrigin = NULL );

#endif // __E_DEBUG__
