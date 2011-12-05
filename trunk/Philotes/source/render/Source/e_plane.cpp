//----------------------------------------------------------------------------
// e_plane.cpp
//
// - Implementation for plane functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "boundingbox.h"

#include "e_plane.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_BoxPointsVsPlaneDotProduct( VECTOR * pPoints, int nPoints, const PLANE * pPlane )
{
	ASSERT( pPoints );
	ASSERT( pPlane );

	int nSide = 0;
	float fDot;
	for ( int nVec = 0; nVec < nPoints; nVec++ )
	{
		fDot = PlaneDotCoord( pPlane, &pPoints[ nVec ] );
		if ( fDot > 0.0f )
			nSide++;
		else if ( fDot < 0.0f )
			nSide--;
	}

	// reduces to -1, 0, 1
	return nSide / nPoints;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Box Vs. Plane Dot Product:
//   Tests all points in axis-aligned bounding box against plane, returns:
//     -1 if all points behind bounding box
//      1 if all points in front of bounding box
//      0 if mixed result or all coplanar
int e_BoxVsPlaneDotProduct( BOUNDING_BOX * pBBox, const PLANE * pPlane )
{
	ASSERT( pBBox );
	ASSERT( pPlane );

	const int nPoints = OBB_POINTS;

	VECTOR vBBox[ OBB_POINTS ] = {
		VECTOR( pBBox->vMin.fX, pBBox->vMin.fY, pBBox->vMin.fZ ),
		VECTOR( pBBox->vMax.fX, pBBox->vMin.fY, pBBox->vMin.fZ ),
		VECTOR( pBBox->vMin.fX, pBBox->vMin.fY, pBBox->vMax.fZ ),
		VECTOR( pBBox->vMax.fX, pBBox->vMin.fY, pBBox->vMax.fZ ),
		VECTOR( pBBox->vMin.fX, pBBox->vMax.fY, pBBox->vMin.fZ ),
		VECTOR( pBBox->vMax.fX, pBBox->vMax.fY, pBBox->vMin.fZ ),
		VECTOR( pBBox->vMin.fX, pBBox->vMax.fY, pBBox->vMax.fZ ),
		VECTOR( pBBox->vMax.fX, pBBox->vMax.fY, pBBox->vMax.fZ )
	};

	return e_BoxPointsVsPlaneDotProduct( vBBox, nPoints, pPlane );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void NearestPointOnPlane( const PLANE *pPlane, const VECTOR * pPoint, VECTOR * pOut )
{
	ASSERT_RETURN( pPlane && pPoint && pOut );

	// nearest point on plane = P - ( ( plane.d + Dot(plane.n, P) ) / Dot(plane.n, plane.n) ) * plane.n

	VECTOR & vPlaneNormal = *(VECTOR*)&pPlane->a;
	VECTOR vT;
	float fNDotP = VectorDot( vPlaneNormal, *pPoint ) + pPlane->d;
	float fNDotN = VectorDot( vPlaneNormal, vPlaneNormal );
	vT = vPlaneNormal * ( fNDotP / fNDotN );
	*pOut = *pPoint - vT;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_PointVsPlaneDotProduct( VECTOR * pPoint, const PLANE * pPlane )
{
	ASSERT_RETZERO( pPoint );
	ASSERT_RETZERO( pPlane );

	int nSide = 0;
	float fDot;
	fDot = PlaneDotCoord( pPlane, pPoint );
	if ( fDot > 0.0f )
		nSide++;
	else if ( fDot < 0.0f )
		nSide--;

	// reduces to -1, 0, 1
	return nSide;
}
