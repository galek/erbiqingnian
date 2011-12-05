//----------------------------------------------------------------------------
// e_primitive.h
//
// - Header for render primitive functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

/*
	DEBUG PRIMITIVES:  types, options, usage

	TYPES:		Line		(2D/3D)
				Triangle	(2D/3D)
				Box			(2D/3D)
				Circle		(2D)
				Sphere		(3D)
				Cylinder	(3D)
				Cone		(3D)

	OPTIONS:	Color:				normal RGB, and alpha blending where 0x00 means "fully additive" and 0xff means "fully opaque"
				On Top:				renders after UI and everything else
				Solid fill:			not just line primitives!
				Wire border:		for use with solid fill, adds a wire overlay
				Specific depth:		on 2D prims, give a specific depth to render at

	USAGE:		Just call any of the e_PrimitiveAdd** functions below.
				You must re-submit the primitive each frame.
*/




#ifndef __E_PRIMITIVE__
#define __E_PRIMITIVE__

#include "boundingbox.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// these two colors are tuned so that the "double surface" of the solid volume default matches the single of the wire/2d solid default
#define PRIM_DEFAULT_COLOR			0x80008080
#define PRIM_DEFAULT_COLOR_SOLID	0x40004040
#define PRIM_DEPTH_MIN				0.01f
#define PRIM_WIRE_BORDER_COLOR		0x007f7f7f

#define PRIM_FLAG_SOLID_FILL			MAKE_MASK(0)
#define PRIM_FLAG_WIRE_BORDER			MAKE_MASK(1)
#define PRIM_FLAG_RENDER_ON_TOP			MAKE_MASK(2)
// "private" primitive flags
#define _PRIM_FLAG_NEED_TRANSFORM		MAKE_MASK(31)


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

// used for final tri and line rendering
struct PRIMITIVE_INFO_RENDER
{
	DWORD					dwFlags;
	VECTOR					v1;
	VECTOR					v2;
	VECTOR					v3;
	DWORD					dwColor;
};


struct PRIMITIVE_INFO_2D_BOX
{
	DWORD					dwFlags;
	VECTOR					vMin;
	VECTOR					vMax;
	DWORD					dwColor;
};

struct PRIMITIVE_INFO_3D_BOX
{
	DWORD					dwFlags;
	ORIENTED_BOUNDING_BOX	tBox;
	DWORD					dwColor;
};

struct PRIMITIVE_INFO_SPHERE
{
	DWORD					dwFlags;
	VECTOR					vCenter;
	float					fRadius;
	DWORD					dwColor;
};

struct PRIMITIVE_INFO_CYLINDER
{
	DWORD					dwFlags;
	VECTOR					vEnd1;
	VECTOR					vEnd2;
	float					fRadius;
	DWORD					dwColor;
};

struct PRIMITIVE_INFO_CONE
{
	DWORD					dwFlags;
	VECTOR					vApex;
	VECTOR					vDirection;
	float					fAngleRad;
	DWORD					dwColor;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void e_PrimitivesMarkNeedRender( BOOL bNeedRender )
{
	extern BOOL gbPrimitivesNeedRender;
	gbPrimitivesNeedRender = bNeedRender;
}

inline BOOL e_PrimitivesNeedRender()
{
	extern BOOL gbPrimitivesNeedRender;
	return gbPrimitivesNeedRender;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddLine(
	DWORD dwFlags,
	const VECTOR * pvEnd1, 
	const VECTOR * pvEnd2,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddLine(
	DWORD dwFlags,
	const VECTOR2 * pvEnd1, 
	const VECTOR2 * pvEnd2,
	DWORD dwColor = PRIM_DEFAULT_COLOR,
	float fDepth = PRIM_DEPTH_MIN );
PRESULT e_PrimitiveAddTri(
	DWORD dwFlags,
	const VECTOR * pv1, 
	const VECTOR * pv2,
	const VECTOR * pv3,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddTri(
	DWORD dwFlags,
	const VECTOR2 * pv1, 
	const VECTOR2 * pv2,
	const VECTOR2 * pv3,
	DWORD dwColor = PRIM_DEFAULT_COLOR,
	float fDepth = PRIM_DEPTH_MIN );

PRESULT e_PrimitiveAddRect(
	DWORD dwFlags,
	const VECTOR * ul, 
	const VECTOR * ur,
	const VECTOR * ll,
	const VECTOR * lr,
	DWORD dwColor = PRIM_DEFAULT_COLOR);

PRESULT e_PrimitiveAddRect(
	DWORD dwFlags,
	const VECTOR * pv1, 
	const VECTOR * pv2,
	float fHeight,
	DWORD dwColor = PRIM_DEFAULT_COLOR);

PRESULT e_PrimitiveAddRect(
	DWORD dwFlags,
	const VECTOR * pv1, 
	const VECTOR * pv2,
	DWORD dwColor = PRIM_DEFAULT_COLOR);

PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const ORIENTED_BOUNDING_BOX pBox,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const BOUNDING_BOX * pBox,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const MATRIX * pmBox,
	DWORD dwColor );
PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const VECTOR2 * pvMin, 
	const VECTOR2 * pvMax,
	DWORD dwColor = PRIM_DEFAULT_COLOR,
	float fDepth = PRIM_DEPTH_MIN );
PRESULT e_PrimitiveAddSphere( 
	DWORD dwFlags,
	const VECTOR * pvCenter, 
	const float fRadius,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddSphere( 
	DWORD dwFlags,
	const BOUNDING_SPHERE * pBS, 
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddCircle(
	DWORD dwFlags,
	const VECTOR2 * pvCenter, 
	const float fRadius,
	DWORD dwColor = PRIM_DEFAULT_COLOR,
	float fDepth = PRIM_DEPTH_MIN );
PRESULT e_PrimitiveAddCylinder( 
	DWORD dwFlags,
	const VECTOR * pvEnd1, 
	const VECTOR * pvEnd2, 
	const float fRadius,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddCone( 
	DWORD dwFlags,
	const VECTOR * pvPos, 
	float fLength, 
	const VECTOR * pvDir,
	float fAngleRad,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveAddCone( 
	DWORD dwFlags,
	const VECTOR * pvPos, 
	const VECTOR * pvFocus, 
	float fAngleRad,
	DWORD dwColor = PRIM_DEFAULT_COLOR );
PRESULT e_PrimitiveInit();
PRESULT e_PrimitiveDestroy();
PRESULT e_PrimitivePlatformInit();
PRESULT e_PrimitivePlatformDestroy();
PRESULT e_UpdateDebugPrimitives();
PRESULT e_RenderDebugPrimitives( BOOL bRenderOnTop );
PRESULT e_PrimitiveInfoClear();

#endif // __E_PRIMITIVE__