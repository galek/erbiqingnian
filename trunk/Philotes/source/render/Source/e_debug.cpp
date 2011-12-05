//----------------------------------------------------------------------------
// e_debug.cpp
//
// - Implementation for debug functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "spring.h"
#include "colors.h"

#include "e_primitive.h"
#include "e_settings.h"
#include "e_debug.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

int gnDebugIdentifyModel = INVALID_ID;
CArrayIndexed<RENDER_2D_AABB> gtRender2DAABBs;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void e_DebugIdentifyModel( int nModelID )
{
	gnDebugIdentifyModel = nModelID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ToggleForceGeometryNormals()
{
	e_ToggleRenderFlag( RENDER_FLAG_FORCE_GEOMETRY_NORMALS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_GetForceGeometryNormals()
{
	return e_GetRenderFlag( RENDER_FLAG_FORCE_GEOMETRY_NORMALS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_Clear2DBoxRenders()
{
	gtRender2DAABBs.Clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_Add2DBoxRender( const BOUNDING_BOX * pBBox, DWORD dwColor )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DEBUG_CLIP_DRAW ) )
		return;

	ASSERT_RETURN( pBBox );
	// add this box to a list to be rendered during the normal render loop
	int nID;
	RENDER_2D_AABB * p2DAABB = gtRender2DAABBs.Add( &nID, TRUE );
	ASSERT_RETURN( nID != INVALID_ID && p2DAABB );
	p2DAABB->tBBox = *pBBox;
	p2DAABB->dwColor = dwColor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_SpringDebugDisplay( struct SPRING * pSpring, float fScale )
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	float fW;
	VECTOR2 vMin, vMax, vO;

	vO.x = nWidth  * 0.75f;
	vO.y = nHeight * 0.5f;

	fW = 0.04f * nWidth;
	vMin.x = vO.x	- fW;
	vMin.y = vO.y	+ ( pSpring->cur - pSpring->nul ) * fScale;
	vMax.x = vO.x	+ fW;
	vMax.y = vO.y	+ ( pSpring->cur + pSpring->nul ) * fScale;
	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0x003f3f3f );

	fW = 0.03f * nWidth;
	vMin.x = vO.x	- fW;
	vMin.y = vO.y;
	vMax.x = vO.x	+ fW;
	vMax.y = vO.y	+ pSpring->tgt * fScale;
	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xff0000ff );

	fW = 0.018f * nWidth;
	vMin.x = vO.x	- fW;
	vMin.y = vO.y;
	vMax.x = vO.x	+ fW;
	vMax.y = vO.y	+ pSpring->cur * fScale;
	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xff00ff00 );

	float fDispVel = pSpring->vel * 100.f;

	fW = 0.007f * nWidth;
	vMin.x = vO.x	- fW;
	vMin.y = vO.y	+ pSpring->cur * fScale;
	vMax.x = vO.x	+ fW;
	vMax.y = vO.y	+ ( pSpring->cur + fDispVel ) * fScale;
	e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xffffff00 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static DWORD sGetDebugBarColor( int nCur, int nGood, int nBad )
{
	float fCur = (float)( nCur - nGood );
	if ( nGood < nBad )
		fCur = max( fCur, 0.f );
	//else
	//	fCur = min( fCur, 0.f );
	float fAmt = fCur / (float)( nBad - nGood );
	fAmt = max( fAmt, 0.f );
	fAmt = min( fAmt, 1.f );
	DWORD dwGood  = GFXCOLOR_GREEN;
	DWORD dwBad   = GFXCOLOR_RED;
	UCHAR * pGood = (UCHAR*)&dwGood;
	UCHAR * pBad  = (UCHAR*)&dwBad;
	DWORD dwColor = 0xff000000;
	UCHAR * pCol  = (UCHAR*)&dwColor;
	// lerp RGBs
	pCol[ 0 ] = (UCHAR)LERP( pGood[0], pBad[0], fAmt );
	pCol[ 1 ] = (UCHAR)LERP( pGood[1], pBad[1], fAmt );
	pCol[ 2 ] = (UCHAR)LERP( pGood[2], pBad[2], fAmt );
	return dwColor;
}

void e_DebugSetBar( int nBar, const char * szName, int nValue, int nMax, int nGood, int nBad )
{
	WCHAR szLabel[ 32 ];
	PStrCvt( szLabel, szName, 32 );

	const int cnSize = 256;
	WCHAR szInfo[ cnSize ] = L"";
	PStrPrintf( szInfo, cnSize, L"%s: %d/%d", szLabel, nValue, nMax );
	VECTOR vPos = VECTOR( 0.6f, 0.8f, 0.f );
	vPos.fY += nBar * -0.05f;
	V( e_UIAddLabel( szInfo, &vPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_RIGHT, TRUE ) );
	
	// Convert to custom device coordinates, offsetting appropriately
	VECTOR2 vMin;
	float fHalfWidth = e_GetWindowWidth() / 2.f;
	vMin.fX = fHalfWidth + ( vPos.fX + 0.025f ) * fHalfWidth;
	float fFullHeight = float( e_GetWindowHeight() );
	float fHalfHeight = fFullHeight / 2.f;
	vMin.fY = fFullHeight - ( fHalfHeight + vPos.fY * fHalfHeight );
	VECTOR2 vMax;
	vMax = vMin;
	vMax.fX += 100.f;
	vMax.fY += 15.f;
	float fPercentUsed = float( nValue ) / float( nMax );
	VECTOR2 vUsed;
	VectorInterpolate( vUsed, fPercentUsed, vMin, vMax );
	vUsed.fY = vMax.fY;

	DWORD dwColor = sGetDebugBarColor( nValue, nGood, nBad );
	e_PrimitiveAddBox( 0, &vMin, &vMax, dwColor );
	e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL, &vMin, &vUsed, dwColor );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DebugRenderAxes( const MATRIX & mSpace, float fScaleFactor /*= 1.f*/, VECTOR * pvWorldOrigin /*= NULL*/ )
{
	VECTOR vX( 1,0,0 );
	VECTOR vY( 0,1,0 );
	VECTOR vZ( 0,0,1 );

	VECTOR vOrigin( 0,0,0 );
	if ( pvWorldOrigin )
		vOrigin = *pvWorldOrigin;

	float fScale = fScaleFactor;
	float fConeLen = 0.1f;
	float fConeAngle = PI / 4;

	MatrixMultiplyNormal( &vX, &vX, &mSpace );
	MatrixMultiplyNormal( &vY, &vY, &mSpace );
	MatrixMultiplyNormal( &vZ, &vZ, &mSpace );
	MatrixMultiply( &vOrigin, &vOrigin, &mSpace );

	VECTOR vXEnd = vX * fScale + vOrigin;
	VECTOR vYEnd = vY * fScale + vOrigin;
	VECTOR vZEnd = vZ * fScale + vOrigin;

	V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vOrigin, &vXEnd, 0xffff0000 ) );
	V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vOrigin, &vYEnd, 0xff00ff00 ) );
	V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vOrigin, &vZEnd, 0xff0000ff ) );

	V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, &vXEnd, fScale * -fConeLen, &vX, fConeAngle, 0xffff0000 ) );
	V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, &vYEnd, fScale * -fConeLen, &vY, fConeAngle, 0xff00ff00 ) );
	V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, &vZEnd, fScale * -fConeLen, &vZ, fConeAngle, 0xff0000ff ) );

	V( e_UIAddLabel( L"\n X", &vXEnd, TRUE, 0.5f, 0xffff0000, UIALIGN_TOPLEFT ) );
	V( e_UIAddLabel( L"\n Y", &vYEnd, TRUE, 0.5f, 0xff00ff00, UIALIGN_TOPLEFT ) );
	V( e_UIAddLabel( L"\n Z", &vZEnd, TRUE, 0.5f, 0xff0000ff, UIALIGN_TOPLEFT ) );

	return S_OK;
}