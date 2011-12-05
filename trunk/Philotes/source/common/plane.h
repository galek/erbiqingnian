//----------------------------------------------------------------------------
// plane.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

#ifndef _PLANE_H_
#define _PLANE_H_

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// made to mimic D3DPLANE
struct PLANE
{
	float a, b, c;
	float d;

	inline struct VECTOR& N() { return (struct VECTOR&)a; }
};

float PlaneDotCoord( const PLANE * pP, const struct VECTOR * pV );
float PlaneDotNormal( const PLANE * pP, const struct VECTOR * pV );
PLANE * PlaneFromPoints( PLANE *pOut, const struct VECTOR * pV1, const struct VECTOR * pV2, const struct VECTOR * pV3 );
PLANE * PlaneFromPointNormal( PLANE * pOut, const struct VECTOR * pPoint, const struct VECTOR * pNormal );
void NearestPointOnPlane( const PLANE *pPlane, const VECTOR * pPoint, VECTOR * pOut );
void PlaneVectorIntersect( VECTOR * vIntersect, const PLANE * pPlane, const VECTOR * vStart, const VECTOR * vEnd );
void PlaneNormalize( PLANE * pPlaneOut, const PLANE * pPlaneIn );
void PlaneTransform( PLANE * pPlaneOut, const PLANE * pPlaneIn, const MATRIX * pmTransform );

struct BOUNDING_BOX;


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Box Vs. Plane Dot Product:
//   Tests all points in axis-aligned bounding box against plane, returns:
//     -1 if all points behind bounding box
//      1 if all points in front of bounding box
//      0 if mixed result or all coplanar
int e_BoxVsPlaneDotProduct( struct BOUNDING_BOX * pBBox, const PLANE * pPlane );
int e_BoxPointsVsPlaneDotProduct( struct VECTOR * pPoints, int nPoints, const PLANE * pPlane );

int e_PointVsPlaneDotProduct( struct VECTOR * pPoint, const PLANE * pPlane );


#endif // _PLANE_H_