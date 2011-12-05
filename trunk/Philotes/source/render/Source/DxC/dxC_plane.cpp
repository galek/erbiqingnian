//----------------------------------------------------------------------------
// dx9_plane.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"



#include "dxC_plane.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

float PlaneDotCoord( const PLANE * pP, const VECTOR * pV )
{
	return D3DXPlaneDotCoord( (D3DXPLANE*)pP, (D3DXVECTOR3*)pV );
}

float PlaneDotNormal( const PLANE * pP, const VECTOR * pV )
{
	return D3DXPlaneDotNormal( (D3DXPLANE*)pP, (D3DXVECTOR3*)pV );
}

PLANE * PlaneFromPoints( PLANE *pOut, const VECTOR * pV1, const VECTOR * pV2, const VECTOR * pV3 )
{
	return (PLANE *)D3DXPlaneFromPoints( (D3DXPLANE*)pOut, (D3DXVECTOR3*)pV1, (D3DXVECTOR3*)pV2, (D3DXVECTOR3*)pV3 );
}

PLANE * PlaneFromPointNormal( PLANE * pOut, const VECTOR * pPoint, const VECTOR * pNormal )
{
	return (PLANE *)D3DXPlaneFromPointNormal( (D3DXPLANE*)pOut, (D3DXVECTOR3*)pPoint, (D3DXVECTOR3*)pNormal );
}

void PlaneVectorIntersect( VECTOR * pvIntersect, const PLANE * pPlane, const VECTOR * pvStart, const VECTOR * pvEnd )
{
	D3DXPlaneIntersectLine( (D3DXVECTOR3*)pvIntersect, (D3DXPLANE*)pPlane, (D3DXVECTOR3*)pvStart, (D3DXVECTOR3*)pvEnd );
}

void PlaneNormalize( PLANE * pPlaneOut, const PLANE * pPlaneIn )
{
	D3DXPlaneNormalize( (D3DXPLANE*)pPlaneOut, (const D3DXPLANE*)pPlaneIn );
}

void PlaneTransform( PLANE * pPlaneOut, const PLANE * pPlaneIn, const MATRIX * pmTransform )
{
	D3DXPlaneTransform( (D3DXPLANE*)pPlaneOut, (const D3DXPLANE*)pPlaneIn, (const D3DXMATRIX*)pmTransform );
}