//----------------------------------------------------------------------------
// e_primitive.cpp
//
// - Implementation for render primitive structures and functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "boundingbox.h"
#include "e_primitive.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER>		gtPrimInfoLine;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER>		gtPrimInfoTri;

SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_2D_BOX>		gtPrimInfo2DBox;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_3D_BOX>		gtPrimInfo3DBox;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_SPHERE>		gtPrimInfoCircle;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_SPHERE>		gtPrimInfoSphere;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_CYLINDER>	gtPrimInfoCylinder;
SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_CONE>		gtPrimInfoCone;

BOOL gbPrimitivesNeedRender = FALSE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddLine(
	DWORD dwFlags,
	const VECTOR * pvEnd1, 
	const VECTOR * pvEnd2,
	DWORD dwColor )
{
	PRIMITIVE_INFO_RENDER tInfo;
	tInfo.dwFlags = dwFlags | _PRIM_FLAG_NEED_TRANSFORM;
	tInfo.v1 = *pvEnd1;
	tInfo.v2 = *pvEnd2;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoLine, tInfo);
	e_PrimitivesMarkNeedRender( TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddLine(
	DWORD dwFlags,
	const VECTOR2 * pvEnd1, 
	const VECTOR2 * pvEnd2,
	DWORD dwColor,
	float fDepth )
{
	PRIMITIVE_INFO_RENDER tInfo;
	tInfo.dwFlags = dwFlags;
	if ( dwFlags & PRIM_FLAG_RENDER_ON_TOP )
		fDepth = PRIM_DEPTH_MIN;
	tInfo.v1 = VECTOR( pvEnd1->x, pvEnd1->y, fDepth );
	tInfo.v2 = VECTOR( pvEnd2->x, pvEnd2->y, fDepth );
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoLine, tInfo);
	e_PrimitivesMarkNeedRender( TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddTri(
	DWORD dwFlags,
	const VECTOR * pv1, 
	const VECTOR * pv2,
	const VECTOR * pv3, 
	DWORD dwColor )
{
	PRIMITIVE_INFO_RENDER tInfo;
	tInfo.dwFlags = dwFlags | _PRIM_FLAG_NEED_TRANSFORM;
	tInfo.v1 = *pv1;
	tInfo.v2 = *pv2;
	tInfo.v3 = *pv3;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoTri, tInfo);
	e_PrimitivesMarkNeedRender( TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddTri(
	DWORD dwFlags,
	const VECTOR2 * pv1, 
	const VECTOR2 * pv2,
	const VECTOR2 * pv3,
	DWORD dwColor,
	float fDepth )
{
	PRIMITIVE_INFO_RENDER tInfo;
	tInfo.dwFlags = dwFlags;
	if ( dwFlags & PRIM_FLAG_RENDER_ON_TOP )
		fDepth = PRIM_DEPTH_MIN;
	tInfo.v1 = VECTOR( pv1->x, pv1->y, fDepth );
	tInfo.v2 = VECTOR( pv2->x, pv2->y, fDepth );
	tInfo.v3 = VECTOR( pv3->x, pv3->y, fDepth );
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoTri, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddRect(
	DWORD dwFlags,
	const VECTOR * ul, 
	const VECTOR * ur,
	const VECTOR * ll,
	const VECTOR * lr,
	DWORD dwColor )
{
	V_RETURN( e_PrimitiveAddLine(dwFlags, ul, ur, dwColor) );
	V_RETURN( e_PrimitiveAddLine(dwFlags, ul, ll, dwColor) );
	V_RETURN( e_PrimitiveAddLine(dwFlags, ur, lr, dwColor) );
	V_RETURN( e_PrimitiveAddLine(dwFlags, ll, lr, dwColor) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddRect(
	DWORD dwFlags,
	const VECTOR * pv1, 
	const VECTOR * pv2,
	float fHeight,
	DWORD dwColor )
{
	VECTOR pv3 = *pv1, pv4 = *pv2;
	pv3.fZ += fHeight;
	pv4.fZ += fHeight;
	return e_PrimitiveAddRect(dwFlags, pv1, pv2, &pv3, &pv4, dwColor);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const ORIENTED_BOUNDING_BOX pBox,
	DWORD dwColor )
{
	PRIMITIVE_INFO_3D_BOX tInfo;
	tInfo.dwFlags = dwFlags;
	memcpy( tInfo.tBox, pBox, sizeof(ORIENTED_BOUNDING_BOX) );
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL && tInfo.dwColor == PRIM_DEFAULT_COLOR )
		tInfo.dwColor = PRIM_DEFAULT_COLOR_SOLID;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfo3DBox, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const BOUNDING_BOX * pBox,
	DWORD dwColor )
{
	ORIENTED_BOUNDING_BOX pOBB;
	MakeOBBFromAABB( pOBB, pBox );
	return e_PrimitiveAddBox( dwFlags, pOBB, dwColor );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const MATRIX * pmBox,
	DWORD dwColor )
{
	ORIENTED_BOUNDING_BOX pOBB;
	MakeOBBFromMatrix( pOBB, pmBox );
	return e_PrimitiveAddBox( dwFlags, pOBB, dwColor );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddBox(
	DWORD dwFlags,
	const VECTOR2 * pvMin, 
	const VECTOR2 * pvMax,
	DWORD dwColor,
	float fDepth )
{
	PRIMITIVE_INFO_2D_BOX tInfo;
	tInfo.dwFlags = dwFlags;
	if ( dwFlags & PRIM_FLAG_RENDER_ON_TOP )
		fDepth = PRIM_DEPTH_MIN;
	tInfo.vMin = VECTOR( pvMin->x, pvMin->y, fDepth );
	tInfo.vMax = VECTOR( pvMax->x, pvMax->y, fDepth );
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfo2DBox, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddSphere( 
	DWORD dwFlags,
	const VECTOR * pvCenter, 
	const float fRadius,
	DWORD dwColor )
{
	PRIMITIVE_INFO_SPHERE tInfo;
	tInfo.dwFlags = dwFlags;
	tInfo.vCenter = *pvCenter;
	tInfo.fRadius = fRadius;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL && tInfo.dwColor == PRIM_DEFAULT_COLOR )
		tInfo.dwColor = PRIM_DEFAULT_COLOR_SOLID;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoSphere, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddSphere( 
	DWORD dwFlags,
	const BOUNDING_SPHERE * pBS, 
	DWORD dwColor )
{
	return e_PrimitiveAddSphere( dwFlags, &pBS->C, pBS->r, dwColor );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddCircle( 
	DWORD dwFlags,
	const VECTOR2 * pvCenter, 
	const float fRadius,
	DWORD dwColor,
	float fDepth )
{
	PRIMITIVE_INFO_SPHERE tInfo;
	tInfo.dwFlags = dwFlags;
	if ( dwFlags & PRIM_FLAG_RENDER_ON_TOP )
		fDepth = PRIM_DEPTH_MIN;
	tInfo.vCenter = VECTOR( pvCenter->x, pvCenter->y, fDepth );
	tInfo.fRadius = fRadius;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoCircle, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddCylinder( 
	DWORD dwFlags,
	const VECTOR * pvEnd1, 
	const VECTOR * pvEnd2,
	const float fRadius,
	DWORD dwColor )
{
	PRIMITIVE_INFO_CYLINDER tInfo;
	tInfo.dwFlags = dwFlags;
	tInfo.vEnd1 = *pvEnd1;
	tInfo.vEnd2 = *pvEnd2;
	tInfo.fRadius = fRadius;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL && tInfo.dwColor == PRIM_DEFAULT_COLOR )
		tInfo.dwColor = PRIM_DEFAULT_COLOR_SOLID;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoCylinder, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddCone( 
	DWORD dwFlags,
	const VECTOR * pvPos, 
	float fLength, 
	const VECTOR * pvDir,
	float fAngleRad,
	DWORD dwColor )
{
	PRIMITIVE_INFO_CONE tInfo;
	tInfo.dwFlags = dwFlags;
	tInfo.vApex = *pvPos;
	tInfo.vDirection = *pvDir * fLength;
	tInfo.fAngleRad = fAngleRad;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL && tInfo.dwColor == PRIM_DEFAULT_COLOR )
		tInfo.dwColor = PRIM_DEFAULT_COLOR_SOLID;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoCone, tInfo);
	e_PrimitivesMarkNeedRender( TRUE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveAddCone( 
	DWORD dwFlags,
	const VECTOR * pvPos, 
	const VECTOR * pvFocus, 
	float fAngleRad,
	DWORD dwColor )
{
	PRIMITIVE_INFO_CONE tInfo;
	tInfo.dwFlags = dwFlags;
	tInfo.vApex = *pvPos;
	tInfo.vDirection = ( *pvFocus - *pvPos );
	tInfo.fAngleRad = fAngleRad;
	tInfo.dwColor = dwColor;
	if ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL && tInfo.dwColor == PRIM_DEFAULT_COLOR )
		tInfo.dwColor = PRIM_DEFAULT_COLOR_SOLID;
	if ( tInfo.dwFlags & PRIM_FLAG_WIRE_BORDER && 0 == ( tInfo.dwFlags & PRIM_FLAG_SOLID_FILL ) )
		tInfo.dwFlags &= ~(PRIM_FLAG_WIRE_BORDER);
	ArrayAddItem(gtPrimInfoCone, tInfo);
	e_PrimitivesMarkNeedRender( TRUE);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveInit()
{
	ArrayInit(gtPrimInfoLine, NULL, 8);
	ArrayInit(gtPrimInfoTri, NULL, 8);
	ArrayInit(gtPrimInfo2DBox, NULL, 8);
	ArrayInit(gtPrimInfo3DBox, NULL, 8);
	ArrayInit(gtPrimInfoCircle, NULL, 1);
	ArrayInit(gtPrimInfoSphere, NULL, 1);
	ArrayInit(gtPrimInfoCylinder, NULL, 1);
	ArrayInit(gtPrimInfoCone, NULL, 1);

	e_PrimitivesMarkNeedRender( FALSE );
	return e_PrimitivePlatformInit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveDestroy()
{
	gtPrimInfoLine.Destroy();
	gtPrimInfoTri.Destroy();
	gtPrimInfo2DBox.Destroy();
	gtPrimInfo3DBox.Destroy();
	gtPrimInfoCircle.Destroy();
	gtPrimInfoSphere.Destroy();
	gtPrimInfoCylinder.Destroy();
	gtPrimInfoCone.Destroy();

	e_PrimitivesMarkNeedRender( FALSE );
	return e_PrimitivePlatformDestroy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PrimitiveInfoClear()
{
	ArrayClear(gtPrimInfoLine);
	ArrayClear(gtPrimInfoTri);
	ArrayClear(gtPrimInfo2DBox);
	ArrayClear(gtPrimInfo3DBox);
	ArrayClear(gtPrimInfoCircle);
	ArrayClear(gtPrimInfoSphere);
	ArrayClear(gtPrimInfoCylinder);
	ArrayClear(gtPrimInfoCone);

	e_PrimitivesMarkNeedRender( FALSE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PrimitiveDrawTest()
{
	// debug test
	VECTOR2 vEnd1( 100.f, 200.f );
	VECTOR2 vEnd2( 500.f, 200.f );
	V( e_PrimitiveAddLine( 0, &vEnd1, &vEnd2 ) );
	VECTOR vEnd5( -2.f, -2.f, 1.f );
	VECTOR vEnd6( -3.f, -3.f, -1.f );
	V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vEnd5, &vEnd6, 0x000000af ) );
	VECTOR vEnd3( 10.f, 10.f, 10.f );
	VECTOR vEnd4( -10.f, -10.f, -10.f );
	V( e_PrimitiveAddLine( 0, &vEnd3, &vEnd4, 0x0000af00 ) );

	VECTOR2 v1( 500.f, 200.f );
	VECTOR2 v2( 500.f, 400.f );
	VECTOR2 v3( 600.f, 300.f );
	V( e_PrimitiveAddTri( 0, &v1, &v2, &v3 ) );
	VECTOR v4( 1.f, 1.f, 1.f );
	VECTOR v5( -1.f, -1.f, -1.f );
	VECTOR v6( 1.f, 0.f, -1.f );
	//V( e_PrimitiveAddTri( 0, &v4, &v5, &v6, 0x0000af00 ) );
	VECTOR v7( 4.f, 4.f, 1.f );
	VECTOR v8( 2.f, 2.f, -1.f );
	VECTOR v9( 4.f, 3.f, -1.f );
	V( e_PrimitiveAddTri( PRIM_FLAG_RENDER_ON_TOP, &v7, &v8, &v9, 0x000000af ) );

	VECTOR2 vb1( 200.f, 100.f );
	VECTOR2 vb2( 300.f, 200.f );
	V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vb1, &vb2, 0xff7f007f ) );

	BOUNDING_BOX tBox1 = 
	{
		VECTOR( 2.f, 1.f, 2.f ),
		VECTOR( 3.f, 4.f, 3.f ),
	};
	V( e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &tBox1, 0x003f3f00 ) );
	BOUNDING_BOX tBox2 = 
	{
		VECTOR( 1.f, 2.f, 2.f ),
		VECTOR( 4.f, 3.f, 3.f ),
	};
	V( e_PrimitiveAddBox( 0, &tBox2, 0xff007f7f ) );

	VECTOR2 vc1( 600.f, 150.f );
	V( e_PrimitiveAddCircle( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vc1, 100.0f, 0xff003f3f ) );

	VECTOR vc2( -3.f, -1.f, 1.f );
	VECTOR vc3(  0.f, -2.f, 3.f );
	V( e_PrimitiveAddCylinder( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vc2, &vc3, 0.5f ) );

	VECTOR vc4( 3.f, -3.f, 1.f );
	VECTOR vc5(  2.f, -2.f, 3.f );
	V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vc4, &vc5, PI/6 ) );

	VECTOR vs1( -5.f, 5.f, -3.f );
	V( e_PrimitiveAddSphere( PRIM_FLAG_SOLID_FILL, &vs1, 1.0f ) );
}