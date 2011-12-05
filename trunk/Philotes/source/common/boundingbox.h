//----------------------------------------------------------------------------
// boundingbox.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// ORIENTED_BOUNDING_BOX point alignment
//
//       4-----5
//      /|    /|
// +Z  6-----7 |    -y
//  |  | |   | |    /
// -z  | 0---|-1  +Y
//     |/    |/
//     2-----3
//
//     -x -- +X
//----------------------------------------------------------------------------
#pragma	once

#ifndef _BOUNDINGBOX_H_
#define _BOUNDINGBOX_H_

#ifndef _PLANE_H_
#include "plane.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define OBB_POINTS						8
typedef VECTOR ORIENTED_BOUNDING_BOX[OBB_POINTS];


//----------------------------------------------------------------------------
// EXTERN
//----------------------------------------------------------------------------
extern const ORIENTED_BOUNDING_BOX		gtUnitOBB;


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct BOUNDING_BOX
{
	VECTOR								vMin;
	VECTOR								vMax;

	float		MaxDimension()				{ return MAX<float>( MAX<float>( vMax.x - vMin.x, vMax.y - vMin.y ), vMax.z - vMin.z ); }
	float		MinDimension()				{ return MIN<float>( MIN<float>( vMax.x - vMin.x, vMax.y - vMin.y ), vMax.z - vMin.z ); }
	VECTOR&		Center( VECTOR& vResult ) const	{ vResult = ( vMax + vMin ) * 0.5f; return vResult; }
	VECTOR&		Scale( VECTOR& vResult ) const { vResult = vMax - vMin; return vResult; }
};

//----------------------------------------------------------------------------
struct BOUNDING_BOX2
{
	VECTOR2								vMin;
	VECTOR2								vMax;

	float		MaxDimension() const		{ return MAX<float>( vMax.x - vMin.x, vMax.y - vMin.y ); }
	float		MinDimension() const		{ return MIN<float>( vMax.x - vMin.x, vMax.y - vMin.y ); }
	VECTOR2&	Center( VECTOR2& vResult ) const { vResult = ( vMax + vMin ) * 0.5f; return vResult; }
	VECTOR2&	Scale( VECTOR2& vResult ) const	{ vResult = vMax - vMin; return vResult; }
};

//----------------------------------------------------------------------------
struct BOUNDING_SPHERE
{
	VECTOR								C;		// center
	float								r;		// radius

	BOUNDING_SPHERE(
		void) : 
			C(0, 0, 0), 
			r(0)
	{
	}
	
	BOUNDING_SPHERE(
		const VECTOR & vCenter, 
		float fRadius) :
			C(vCenter),
			r(fRadius) 
	{
	}

	BOUNDING_SPHERE(
		const BOUNDING_BOX & tBBox)
	{
		C = ( tBBox.vMin + tBBox.vMax ) * 0.5f;
		r = sqrtf(VectorDistanceSquared(tBBox.vMin, C));
	}

	inline float Distance(
		const VECTOR & tV) const
	{
		float d = sqrtf(VectorDistanceSquared(C, tV)) - r;
		return MAX(d, 0.f);
	}

	float Distance(
		const BOUNDING_SPHERE & tBS)		
	{	
		float d = Distance(tBS.C) - tBS.r; 
		return MAX(d, 0.f);							
	}

	BOOL  Intersect(
		const BOUNDING_SPHERE & tBS)		
	{	
		return 0.f > (VectorDistanceSquared(C, tBS.C) - (r + tBS.r) * (r + tBS.r));	
	}

	BOOL  Intersect(
		const VECTOR & tV)				
	{	
		return r * r > VectorDistanceSquared(C, tV);									
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxExpandByPoint(
	BOUNDING_BOX * box,
	const VECTOR * point)
{
	__assume(box != NULL);
	__assume(point != NULL);
	if (box->vMin.x > point->x)
	{
		box->vMin.x = point->x;
	}
	else if (box->vMax.x < point->x)
	{
		box->vMax.x = point->x;
	}

	if (box->vMin.y > point->y)
	{
		box->vMin.y = point->y;
	}
	else if (box->vMax.y < point->y)
	{
		box->vMax.y = point->y;
	}

	if (box->vMin.z > point->z)
	{
		box->vMin.z = point->z;
	}
	else if (box->vMax.z < point->z)
	{
		box->vMax.z = point->z;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxExpandByBox(
	BOUNDING_BOX * box,
	const BOUNDING_BOX * box2)
{
	if (box->vMin.x > box2->vMin.x)
	{
		box->vMin.x = box2->vMin.x;
	}
	if (box->vMax.x < box2->vMax.x)
	{
		box->vMax.x = box2->vMax.x;
	}

	if (box->vMin.y > box2->vMin.y)
	{
		box->vMin.y = box2->vMin.y;
	}
	if (box->vMax.y < box2->vMax.y)
	{
		box->vMax.y = box2->vMax.y;
	}

	if (box->vMin.z > box2->vMin.z)
	{
		box->vMin.z = box2->vMin.z;
	}
	if (box->vMax.z < box2->vMax.z)
	{
		box->vMax.z = box2->vMax.z;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxExpandBySize(
	BOUNDING_BOX * box,
	const float size)
{
	if (box->vMin.x > box->vMax.x)
	{
		box->vMin.x += size;
		box->vMax.x -= size;
	}
	else
	{
		box->vMin.x -= size;
		box->vMax.x += size;
	}

	if (box->vMin.y > box->vMax.y)
	{
		box->vMin.y += size;
		box->vMax.y -= size;
	}
	else
	{
		box->vMin.y -= size;
		box->vMax.y += size;
	}

	if (box->vMin.z > box->vMax.z)
	{
		box->vMin.z += size;
		box->vMax.z -= size;
	}
	else
	{
		box->vMin.z -= size;
		box->vMax.z += size;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ExpandBounds(
	VECTOR & Min,						// minimum bounds to expand
	VECTOR & Max,						// maximum bounds to expand
	VECTOR Point)						// point to expand the bounds by
{
	if (Point.x < Min.x)
	{
		Min.x = Point.x;
	}
	if (Point.x > Max.x)
	{
		Max.x = Point.x;
	}
	if (Point.y < Min.y)
	{
		Min.y = Point.y;
	}
	if (Point.y > Max.y)
	{
		Max.y = Point.y;
	}
	if (Point.z < Min.z)
	{
		Min.z = Point.z;
	}
	if (Point.z > Max.z)
	{
		Max.z = Point.z;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxTestPoint(
	const BOUNDING_BOX * box,
	const VECTOR * point)
{
	return (box->vMin.x <= point->x && box->vMin.y <= point->y &&
		box->vMin.z <= point->z && box->vMax.x > point->x &&
		box->vMax.y > point->y && box->vMax.z > point->z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxTestPoint(
	const BOUNDING_BOX * box,
	const VECTOR * point,
	float error)
{
	return (box->vMin.x - error <= point->x && box->vMin.y - error <= point->y &&
		box->vMin.z - error <= point->z && box->vMax.x + error >  point->x &&
		box->vMax.y + error >  point->y && box->vMax.z + error >  point->z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundsIntersect(
	const VECTOR & MinA, 
	const VECTOR & MaxA, 
	const VECTOR & MinB,
	const VECTOR & MaxB) 
{
	return (MinA.x < MaxB.x && MinA.y < MaxB.y && MinA.z < MaxB.z &&
		MaxA.x > MinB.x && MaxA.y > MinB.y && MaxA.z > MinB.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollide(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x < boxB->vMax.x && boxA->vMin.y < boxB->vMax.y &&
		boxA->vMin.z < boxB->vMax.z &&	boxA->vMax.x > boxB->vMin.x &&
		boxA->vMax.y > boxB->vMin.y &&	boxA->vMax.z > boxB->vMin.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollide(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB, 
	float error) 
{
	return (boxA->vMin.x + error < boxB->vMax.x && boxA->vMin.y + error < boxB->vMax.y &&
		boxA->vMin.z + error < boxB->vMax.z &&	boxA->vMax.x - error > boxB->vMin.x &&
		boxA->vMax.y - error > boxB->vMin.y &&	boxA->vMax.z - error > boxB->vMin.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x < boxB->vMax.x && boxA->vMin.y < boxB->vMax.y &&
		boxA->vMax.x > boxB->vMin.x && boxA->vMax.y > boxB->vMin.y);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX2 * boxA,
	const BOUNDING_BOX2 * boxB) 
{
	return (boxA->vMin.x < boxB->vMax.x && boxA->vMin.y < boxB->vMax.y &&
		boxA->vMax.x > boxB->vMin.x && boxA->vMax.y > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB,
	float error) 
{
	return (boxA->vMin.x + error < boxB->vMax.x && boxA->vMin.y + error < boxB->vMax.y &&
		boxA->vMax.x - error > boxB->vMin.x && boxA->vMax.y - error > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX2 * boxB) 
{
	return (boxA->vMin.x < boxB->vMax.x && boxA->vMin.y < boxB->vMax.y &&
		boxA->vMax.x > boxB->vMin.x && boxA->vMax.y > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX2 * boxB,
	float error) 
{
	return (boxA->vMin.x + error < boxB->vMax.x && boxA->vMin.y + error < boxB->vMax.y &&
		boxA->vMax.x - error > boxB->vMin.x && boxA->vMax.y - error > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX2 * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x < boxB->vMax.x && boxA->vMin.y < boxB->vMax.y &&
		boxA->vMax.x > boxB->vMin.x && boxA->vMax.y > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxCollideXY(
	const BOUNDING_BOX2 * boxA,
	const BOUNDING_BOX * boxB,
	float error) 
{
	return (boxA->vMin.x + error < boxB->vMax.x && boxA->vMin.y + error < boxB->vMax.y &&
		boxA->vMax.x - error > boxB->vMin.x && boxA->vMax.y - error > boxB->vMin.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxContains(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x <= boxB->vMin.x && boxA->vMin.y <= boxB->vMin.y && boxA->vMin.z <= boxB->vMin.z && 
		boxA->vMax.x >= boxB->vMax.x && boxA->vMax.y >= boxB->vMax.y && boxA->vMax.z >= boxB->vMax.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxContainsXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x <= boxB->vMin.x && boxA->vMin.y <= boxB->vMin.y &&
		boxA->vMax.x >= boxB->vMax.x && boxA->vMax.y >= boxB->vMax.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxContainsXY(
	const BOUNDING_BOX2 * boxA,
	const BOUNDING_BOX2 * boxB) 
{
	return (boxA->vMin.x <= boxB->vMin.x && boxA->vMin.y <= boxB->vMin.y &&
		boxA->vMax.x >= boxB->vMax.x && boxA->vMax.y >= boxB->vMax.y);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxContainsXY(
	const BOUNDING_BOX2 * boxA,
	const BOUNDING_BOX * boxB) 
{
	return (boxA->vMin.x <= boxB->vMin.x && boxA->vMin.y <= boxB->vMin.y &&
		boxA->vMax.x >= boxB->vMax.x && boxA->vMax.y >= boxB->vMax.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxContainsXY(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX2 * boxB) 
{
	return (boxA->vMin.x <= boxB->vMin.x && boxA->vMin.y <= boxB->vMin.y &&
		boxA->vMax.x >= boxB->vMax.x && boxA->vMax.y >= boxB->vMax.y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float BoundingBoxManhattanDistance(
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB) 
{
	float distance = 0.0f;

	if (boxA->vMin.x > boxB->vMax.x)
	{
		distance += boxA->vMin.x - boxB->vMax.x;
	}
	else if (boxB->vMin.x > boxA->vMax.x)
	{
		distance += boxB->vMin.x - boxA->vMax.x;
	}

	if (boxA->vMin.y > boxB->vMax.y)
	{
		distance += boxA->vMin.y - boxB->vMax.y;
	}
	else if (boxB->vMin.y > boxA->vMax.y)
	{
		distance += boxB->vMin.y - boxA->vMax.y;
	}

	if (boxA->vMin.z > boxB->vMax.z)
	{
		distance += boxA->vMin.z - boxB->vMax.z;
	}
	else if (boxB->vMin.z > boxA->vMax.z)
	{
		distance += boxB->vMin.z - boxA->vMax.z;
	}

	return distance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float BoundingBoxManhattanDistance(
	const BOUNDING_BOX * box,
	const VECTOR * point) 
{
	float distance = 0.0f;

	if (box->vMin.x > point->x)
	{
		distance += box->vMin.x - point->x;
	}
	else if (point->x > box->vMax.x)
	{
		distance += point->x - box->vMax.x;
	}

	if (box->vMin.y > point->y)
	{
		distance += box->vMin.y - point->y;
	}
	else if (point->y > box->vMax.y)
	{
		distance += point->y - box->vMax.y;
	}

	if (box->vMin.z > point->z)
	{
		distance += box->vMin.z - point->z;
	}
	else if (point->z > box->vMax.z)
	{
		distance += point->z - box->vMax.z;
	}

	return distance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float BoundingBoxDistanceSquared(
	const BOUNDING_BOX * box,
	const VECTOR * point) 
{
	VECTOR v;

	if (box->vMin.x > point->x)
	{
		v.x = box->vMin.x - point->x;
	}
	else if( point->x > box->vMax.x )
	{
		v.x = point->x - box->vMax.x;
	}
	else
	{
		v.x = 0;
	}

	if (box->vMin.y > point->y)
	{
		v.y = box->vMin.y - point->y;
	}
	else if( point->y > box->vMax.y )
	{
		v.y = point->y - box->vMax.y;
	}
	else
	{
		v.y = 0;
	}

	if (box->vMin.z > point->z)
	{
		v.z = box->vMin.z - point->z;
	}
	else if( point->z > box->vMax.z )
	{
		v.z = point->z - box->vMax.z;
	}
	else
	{
		v.z = 0;
	}

	return VectorLengthSquared(v);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float BoundingBoxDistanceSquaredXY(
	const BOUNDING_BOX * box,
	const VECTOR * point) 
{
	VECTOR v;

	if (box->vMin.x > point->x)
	{
		v.x = box->vMin.x - point->x;
	}
	else if( point->x > box->vMax.x )
	{
		v.x = point->x - box->vMax.x;
	}
	else
	{
		v.x = 0;
	}

	if (box->vMin.y > point->y)
	{
		v.y = box->vMin.y - point->y;
	}
	else if( point->y > box->vMax.y )
	{
		v.y = point->y - box->vMax.y;
	}
	else
	{
		v.y = 0;
	}


	return VectorLengthSquaredXY(v);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxScaleDelta(
	float min,
	float max,
	float scale,
	float * out_min,
	float * out_max)
{
	float delta = max - min;
	float mid = min + delta / 2.0f;
	delta *= scale;
	*out_min = mid - delta / 2.0f;
	*out_max = mid + delta / 2.0f;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxScale(
	BOUNDING_BOX * boxTarget,
	const BOUNDING_BOX * boxSource, 
	float scale)
{
	if (scale == 1.0f)
	{
		*boxTarget = *boxSource;
	}
	else
	{
		BoundingBoxScaleDelta(boxSource->vMin.fX, boxSource->vMax.fX, scale, &boxTarget->vMin.fX, &boxTarget->vMax.fX);
		BoundingBoxScaleDelta(boxSource->vMin.fY, boxSource->vMax.fY, scale, &boxTarget->vMin.fY, &boxTarget->vMax.fY);
		BoundingBoxScaleDelta(boxSource->vMin.fZ, boxSource->vMax.fZ, scale, &boxTarget->vMin.fZ, &boxTarget->vMax.fZ);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxScale(
							 BOUNDING_BOX * boxTarget,
							 const BOUNDING_BOX * boxSource, 
							 const VECTOR vScale)
{
	if (vScale.fX == 1.0f && vScale.fY == 1.0f && vScale.fZ == 1.0f)
	{
		*boxTarget = *boxSource;
	}
	else
	{
		BoundingBoxScaleDelta(boxSource->vMin.fX, boxSource->vMax.fX, vScale.fX, &boxTarget->vMin.fX, &boxTarget->vMax.fX);
		BoundingBoxScaleDelta(boxSource->vMin.fY, boxSource->vMax.fY, vScale.fY, &boxTarget->vMin.fY, &boxTarget->vMax.fY);
		BoundingBoxScaleDelta(boxSource->vMin.fZ, boxSource->vMax.fZ, vScale.fZ, &boxTarget->vMin.fZ, &boxTarget->vMax.fZ);
	}
}

//----------------------------------------------------------------------------
// if the boxes don't intersect, then some min will be greater than some max
//----------------------------------------------------------------------------
inline void BoundingBoxIntersect(
	BOUNDING_BOX * intersection,
	const BOUNDING_BOX * boxA,
	const BOUNDING_BOX * boxB)
{
	intersection->vMin.x = MAX(boxA->vMin.x, boxB->vMin.x);
	intersection->vMin.y = MAX(boxA->vMin.y, boxB->vMin.y);
	intersection->vMin.z = MAX(boxA->vMin.z, boxB->vMin.z);

	intersection->vMax.x = MIN(boxA->vMax.x, boxB->vMax.x);
	intersection->vMax.y = MIN(boxA->vMax.y, boxB->vMax.y);
	intersection->vMax.z = MIN(boxA->vMax.z, boxB->vMax.z);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void BoundingBoxZRotation(
	BOUNDING_BOX * box,
	const float zrot)
{
	VECTOR vUL = box->vMin;
	VECTOR vLR = box->vMax;
	VECTOR vUR(vLR.x, vUL.y, vUL.z), vLL(vUL.x, vLR.y, vLR.z);
	VectorZRotation(vUL, zrot);
	VectorZRotation(vUR, zrot);
	VectorZRotation(vLL, zrot);
	VectorZRotation(vLR, zrot);
	box->vMin = vUL;
	box->vMax = vUL;
	BoundingBoxExpandByPoint(box, &vUR);
	BoundingBoxExpandByPoint(box, &vLL);
	BoundingBoxExpandByPoint(box, &vLR);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline VECTOR BoundingBoxGetCenter(
	const BOUNDING_BOX * box)
{
	VECTOR v = box->vMax + box->vMin;
	v /= 2.0f;
	return v;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline VECTOR BoundingBoxGetSize(
	const BOUNDING_BOX * box)
{
	return box->vMax - box->vMin;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL BoundingBoxIsZero(
	const BOUNDING_BOX & box)
{
	return (VectorIsZero(box.vMax) && VectorIsZero(box.vMin));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BoundingBoxTranslate( const struct MATRIX * pMatrix, BOUNDING_BOX *pBoxTarget, BOUNDING_BOX *pBoxSource);
void TransformOBB( ORIENTED_BOUNDING_BOX pOBBDest, const MATRIX * pmTrans, const ORIENTED_BOUNDING_BOX * pOBBSrc );

class CRayIntersector {
public:
	CRayIntersector( const VECTOR & vOrigin, const VECTOR & vDirection ) {
		m_vOrigin = vOrigin;
		m_vDirection = vDirection;
		m_InverseDirection = VECTOR(1.0f/vDirection.x, 1.0f/vDirection.y, 1.0f/vDirection.z);
		m_nSign[0] = (m_InverseDirection.x < 0) ? 1 : 0;
		m_nSign[1] = (m_InverseDirection.y < 0) ? 1 : 0;
		m_nSign[2] = (m_InverseDirection.z < 0) ? 1 : 0;
	}
	VECTOR m_vOrigin;
	VECTOR m_vDirection;
	VECTOR m_InverseDirection;
	int m_nSign[3];
};

BOOL BoundingBoxIntersectRay(
	const BOUNDING_BOX & tBoundingBox,
	const CRayIntersector &vRay,
	float fDistanceAlongRay );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void MakeOBBFromAABB(
	ORIENTED_BOUNDING_BOX pOBB, 
	const BOUNDING_BOX * pAABB)
{
	pOBB[0] = VECTOR(pAABB->vMin.fX, pAABB->vMin.fY, pAABB->vMin.fZ); // xyz
	pOBB[1] = VECTOR(pAABB->vMax.fX, pAABB->vMin.fY, pAABB->vMin.fZ); // Xyz
	pOBB[2] = VECTOR(pAABB->vMin.fX, pAABB->vMax.fY, pAABB->vMin.fZ); // xYz
	pOBB[3] = VECTOR(pAABB->vMax.fX, pAABB->vMax.fY, pAABB->vMin.fZ); // XYz
	pOBB[4] = VECTOR(pAABB->vMin.fX, pAABB->vMin.fY, pAABB->vMax.fZ); // xyZ
	pOBB[5] = VECTOR(pAABB->vMax.fX, pAABB->vMin.fY, pAABB->vMax.fZ); // XyZ
	pOBB[6] = VECTOR(pAABB->vMin.fX, pAABB->vMax.fY, pAABB->vMax.fZ); // xYZ
	pOBB[7] = VECTOR(pAABB->vMax.fX, pAABB->vMax.fY, pAABB->vMax.fZ); // XYZ
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void MakeOBBFromMatrix(
	ORIENTED_BOUNDING_BOX pOBB, 
	const MATRIX * pmBox)
{
	for (unsigned int ii = 0; ii < OBB_POINTS; ++ii)
	{
		MatrixMultiplyCoord(&pOBB[ii], &gtUnitOBB[ii], pmBox);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void TransformAABBToOBB(
	ORIENTED_BOUNDING_BOX OBB,
	const MATRIX * m,
	const BOUNDING_BOX * AABB)
{
	ASSERT_RETURN(OBB);
	ASSERT_RETURN(AABB);
	ASSERT_RETURN(m);

	MakeOBBFromAABB(OBB, AABB);

	for (unsigned int ii = 0; ii < OBB_POINTS; ++ii)
	{
		MatrixMultiplyCoord(&OBB[ii], &OBB[ii], m);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void TransformOBB(
	ORIENTED_BOUNDING_BOX destOBB,
	const MATRIX * m,
	const ORIENTED_BOUNDING_BOX srcOBB)
{
	ASSERT_RETURN(destOBB);
	ASSERT_RETURN(srcOBB);
	ASSERT_RETURN(m);

	for (unsigned int ii = 0; ii < OBB_POINTS; ++ii)
	{
		MatrixMultiplyCoord(&destOBB[ii], &srcOBB[ii], m);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void MakeOBBPlanes(
	PLANE Planes[6], 
	const ORIENTED_BOUNDING_BOX OBB)
{
	ASSERT_RETURN(Planes);
	ASSERT_RETURN(OBB);

	PlaneFromPoints(&Planes[0], &OBB[0], &OBB[1], &OBB[2]);
	PlaneFromPoints(&Planes[1], &OBB[6], &OBB[7], &OBB[5]);
	PlaneFromPoints(&Planes[2], &OBB[2], &OBB[6], &OBB[4]);
	PlaneFromPoints(&Planes[3], &OBB[7], &OBB[3], &OBB[5]);
	PlaneFromPoints(&Planes[4], &OBB[2], &OBB[3], &OBB[6]);
	PlaneFromPoints(&Planes[5], &OBB[1], &OBB[0], &OBB[4]);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PointInOBBPlanes(
	const PLANE Planes[6],
	VECTOR & point)
{
	ASSERT_RETFALSE(Planes);

	// dot against each plane, exiting on failure
	for (unsigned int ii = 0; ii < 6; ++ii)
	{
		if (e_PointVsPlaneDotProduct(&point, &Planes[ii]) < 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}


#endif // _BOUNDINGBOX_H_
