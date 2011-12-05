//----------------------------------------------------------------------------
// boundingbox.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "boundingbox.h"


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const ORIENTED_BOUNDING_BOX gtUnitOBB =
{
	VECTOR( -1.f, -1.f, -1.f ),
	VECTOR(  1.f, -1.f, -1.f ),
	VECTOR( -1.f,  1.f, -1.f ),
	VECTOR(  1.f,  1.f, -1.f ),
	VECTOR( -1.f, -1.f,  1.f ),
	VECTOR(  1.f, -1.f,  1.f ),
	VECTOR( -1.f,  1.f,  1.f ),
	VECTOR(  1.f,  1.f,  1.f ),
};


//----------------------------------------------------------------------------
// algorithm borrowed from http://www.cs.caltech.edu/~arvo/code/TransformingBoxes.c
// well, it didn't work with some rotations... maybe someone else can figure out why - Tyler
// fixed it for you! - PHu
//----------------------------------------------------------------------------
void BoundingBoxTranslate(
	const MATRIX * Matrix,				// transform matrix
	BOUNDING_BOX * BoxTarget,			// the transformed bounding box
	BOUNDING_BOX * BoxSource)			// the original bounding box
{
	ASSERT_RETURN(Matrix);
	ASSERT_RETURN(BoxTarget);
	ASSERT_RETURN(BoxSource);
	
	if (BoxSource->vMin.x > BoxSource->vMax.x)
	{
		SWAP(BoxSource->vMin.x, BoxSource->vMax.x);
	}
	if (BoxSource->vMin.y > BoxSource->vMax.y)
	{
		SWAP(BoxSource->vMin.y, BoxSource->vMax.y);
	}
	if (BoxSource->vMin.z > BoxSource->vMax.z)
	{
		SWAP(BoxSource->vMin.z, BoxSource->vMax.z);
	}
	VECTOR SourceMin(BoxSource->vMin);
	VECTOR SourceMax(BoxSource->vMax);
	VECTOR TargetMin(Matrix->_41, Matrix->_42, Matrix->_43);
	VECTOR TargetMax(Matrix->_41, Matrix->_42, Matrix->_43);

	// now find the extreme points by considering the product of the min and max with each component of the matrix
	for (unsigned int ii = 0; ii < 3; ++ii)
	{
		for (unsigned int jj = 0; jj < 3; ++jj)
		{
			float m = Matrix->m[jj][ii];
			float a = m * SourceMin[jj];
			float b = m * SourceMax[jj];
			if (a > b) 
			{
				TargetMin[ii] += b; 
				TargetMax[ii] += a;
			}
			else
			{
				TargetMin[ii] += a;
				TargetMax[ii] += b;
			}
		}
	}

	BoxTarget->vMin = TargetMin;
	BoxTarget->vMax = TargetMax;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL BoundingBoxIntersectRay(
	const BOUNDING_BOX & tBoundingBox,
	const CRayIntersector &vRay,
	float fDistanceAlongRay ) 
{
	ASSERT( OFFSET(BOUNDING_BOX, vMin) == 0 );
	ASSERT( OFFSET(BOUNDING_BOX, vMin) + sizeof( VECTOR ) == OFFSET(BOUNDING_BOX, vMax) );
	ASSERT_RETFALSE(vRay.m_nSign[0] == 0 || vRay.m_nSign[0] == 1);
	ASSERT_RETFALSE(vRay.m_nSign[1] == 0 || vRay.m_nSign[1] == 1);
	ASSERT_RETFALSE(vRay.m_nSign[2] == 0 || vRay.m_nSign[2] == 1);

	const VECTOR * pvBounds = (const VECTOR *)&tBoundingBox;

	float tmin  = (pvBounds[	vRay.m_nSign[0]	].x - vRay.m_vOrigin.x) * vRay.m_InverseDirection.x;
	float tmax  = (pvBounds[1 -	vRay.m_nSign[0]	].x - vRay.m_vOrigin.x) * vRay.m_InverseDirection.x;
	float tymin = (pvBounds[	vRay.m_nSign[1]	].y - vRay.m_vOrigin.y) * vRay.m_InverseDirection.y;
	float tymax = (pvBounds[1 -	vRay.m_nSign[1]	].y - vRay.m_vOrigin.y) * vRay.m_InverseDirection.y;
	if ( (tmin > tymax) || (tymin > tmax) )
		return FALSE;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	float tzmin = (pvBounds[	vRay.m_nSign[2]	].z - vRay.m_vOrigin.z) * vRay.m_InverseDirection.z;
	float tzmax = (pvBounds[1 - vRay.m_nSign[2] ].z - vRay.m_vOrigin.z) * vRay.m_InverseDirection.z;
	if ( (tmin > tzmax) || (tzmin > tmax) )
		return FALSE;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	return ( (tmin < fDistanceAlongRay) && (tmax > 0.0f) );
}

