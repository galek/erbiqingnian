//----------------------------------------------------------------------------
// vector.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const VECTOR cgvNone(0.0f, 0.0f, 0.0f);
const VECTOR cgvNull(0.0f, 0.0f, 0.0f);
const VECTOR cgvXAxis(1.0f, 0.0f, 0.0f);
const VECTOR cgvYAxis(0.0f, 1.0f, 0.0f);
const VECTOR cgvZAxis(0.0f, 0.0f, 1.0f);


//-------------------------------------------------------------------------------------------------
// todo: dir is allowed to be either 0 or 1 vector, should split into 2
//-------------------------------------------------------------------------------------------------
BOOL RayIntersectTriangle(  const VECTOR *pOrig, const VECTOR *pDir,
							const VECTOR &vVert0, const VECTOR &vVert1, const VECTOR &vVert2,
							float *pfDist, float *pfUi, float *pfVi )
{
#ifdef _DEBUG
	float lensq = VectorLengthSquared(*pDir);
	if (lensq < -EPSILON || lensq > EPSILON)
	{
		ASSERT(lensq >= 1.0f - 0.01f && lensq <= 1.0f + 0.01f);
	}
#endif

	VECTOR edge1, edge2, tvec, pvec, qvec;

	// find vectors for two edges sharing vert0
	VectorSubtract( edge1, vVert1, vVert0 );
	VectorSubtract( edge2, vVert2, vVert0 );

	// begin calculating determinant - also used to calculate U parameter
	VectorCross( pvec, *pDir, edge2 );

	// if determinant is near zero, ray lies in plane of triangle
	float fDet = VectorDot( edge1, pvec );

	if ( fDet < EPSILON )
		return FALSE;

	// calculate distance from vert0 to ray origin
	VectorSubtract( tvec, *pOrig, vVert0 );

	// calculate U parameter and test bounds
	*pfUi = VectorDot( tvec, pvec );
	if ( *pfUi < 0.0f || *pfUi > fDet )
		return FALSE;

	// prepare to test V parameter
	VectorCross( qvec, tvec, edge1 );

	// calculate V parameter and test bounds
	*pfVi = VectorDot( *pDir, qvec );
	if ( *pfVi < 0.0f || *pfUi + *pfVi > fDet )
		return FALSE;

	// calculate t, ray intersects triangle
	float fInvDet = 1.0f / fDet;
	*pfDist = VectorDot( edge2, qvec ) * fInvDet;
	*pfUi *= fInvDet;
	*pfVi *= fInvDet;
	return TRUE;
}

//----------------------------------------------------------------------------
// Is the position vPos 'in front of' the position vFrontOfPos given
// the direction vFrontOfDir at vFrontOfPos
//----------------------------------------------------------------------------
BOOL VectorPosIsInFrontOf(
	const VECTOR &vPos,
	const VECTOR &vFrontOfPos,
	const VECTOR &vFrontOfDir)
{
	
	// get vector from front of position to position
	VECTOR vToPos = vPos - vFrontOfPos;

	// normalize each
	VectorNormalize( vToPos );
	VECTOR vFrontOfDirNormalized = vFrontOfDir;	
	VectorNormalize( vFrontOfDirNormalized );
	
	// check dot product	
	float flCosBetween = VectorDot( vFrontOfDirNormalized, vToPos );
	return flCosBetween >= 0.0f;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL VectorPosIsInBehindOf(
	const VECTOR &vPos,
	const VECTOR &vBehindOfPos,
	const VECTOR &vBehindOfDir)
{
	return !VectorPosIsInFrontOf( vPos, vBehindOfPos, vBehindOfDir );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClosestPointOnLine( 
	const VECTOR& PointA,		// point a of line segment
	const VECTOR& PointB,		// point b of line segment
	const VECTOR& Point,		// point to check from
	VECTOR& ClosestPoint )		// resultant point to be filled
{
	float		LineMagnitude;
	float		U;
	VECTOR	Intersection;

	Intersection = PointB - PointA;
	LineMagnitude = VectorLengthSquared( Intersection );

	U = ( ( ( Point.fX - PointA.fX ) * ( PointB.fX - PointA.fX ) ) +
		  ( ( Point.fY - PointA.fY ) * ( PointB.fY - PointA.fY ) ) +
		  ( ( Point.fZ - PointA.fZ ) * ( PointB.fZ - PointA.fZ ) ) ) /
		( LineMagnitude );

	if ( U < 0 )
	{
		ClosestPoint = PointA;
		return;
	}
	if ( U > 1 )
	{
		ClosestPoint = PointB;
		return;
	}

	Intersection.fX = PointA.fX + U * ( PointB.fX - PointA.fX );
	Intersection.fY = PointA.fY + U * ( PointB.fY - PointA.fY );
	Intersection.fZ = PointA.fZ + U * ( PointB.fZ - PointA.fZ );

	ClosestPoint = Intersection;
} // ClosestPointOnLine()


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClosestPointOnTriangle(
	const VECTOR& CornerA,	// corner a of the triangle
	const VECTOR& CornerB,	// corner b of the triangle 
	const VECTOR& CornerC,	// corner c of the triangle 
	const VECTOR& Point, 	// point to start from
	VECTOR& ClosestPoint )	// closest point to be filled
{
	VECTOR	TempVEC3D;
	VECTOR	DistanceVEC3D;
	float		BestDistance, Distance;
	ClosestPointOnLine( CornerA, CornerB, Point, TempVEC3D );
	DistanceVEC3D = TempVEC3D - Point;
	BestDistance = VectorLengthSquared( DistanceVEC3D );
	ClosestPoint = TempVEC3D;

	ClosestPointOnLine( CornerB, CornerC, Point, TempVEC3D );
	DistanceVEC3D = TempVEC3D - Point;
	Distance = VectorLengthSquared( DistanceVEC3D );

	if ( Distance < BestDistance )
	{
		ClosestPoint = TempVEC3D;
		BestDistance = Distance;
	}

	ClosestPointOnLine( CornerC, CornerA, Point, TempVEC3D );
	DistanceVEC3D = TempVEC3D - Point;
	Distance = VectorLengthSquared( DistanceVEC3D );

	if ( Distance < BestDistance )
	{
		ClosestPoint = TempVEC3D;
		BestDistance = Distance;
	}
} // ClosestPointOnTriangle()


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PLANE_CLASSIFICATION ClassifyPoint(
	const VECTOR& Point,			// point to classify
	const VECTOR& VertexOnPlane,	// a vertex on the plane
	const VECTOR& Normal )			// the normal of the plane
{
	float	Result	= ( VertexOnPlane.x - Point.x ) * Normal.x +
					  ( VertexOnPlane.y - Point.y ) * Normal.y +
					  ( VertexOnPlane.z - Point.z ) * Normal.z;
	if ( Result < -EPSILON )
	{
		return KPlaneFront;
	}
	if ( Result > EPSILON )
	{
		return KPlaneBack;
	}
	return KPlaneOn;
} // ClassifyPoint()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PLANE_CLASSIFICATION ClassifyPointForSphere(
	const VECTOR& Point, 				// point to classify
	const VECTOR& VertexOnPlane, 		// a vertex on the plane
	const VECTOR& Normal, 				// the normal of the plane
	float Radius )						// the radius of the sphere
{
	float	Result	= ( VertexOnPlane.x - Point.x ) * Normal.x +
					  ( VertexOnPlane.y - Point.y ) * Normal.y +
					  ( VertexOnPlane.z - Point.z ) * Normal.z;
	Result += Radius;
	if ( Result < -EPSILON )
	{
		return KPlaneFront;
	}
	if ( Result > EPSILON )
	{
		return KPlaneBack;
	}
	return KPlaneOn;
} // ClassifyPointForSphere()	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GetLinePlaneIntersection(
	const VECTOR& LineStart,		// line segment start 
	const VECTOR& LineEnd, 			// line segment end
	const VECTOR& VertexOnPlane,	// point on plane
	const VECTOR& Normal,			// normal of plane
	VECTOR& Intersection )			// intersection point to fill
{
	float DeltaX = LineEnd.x - LineStart.x;
	float DeltaY = LineEnd.y - LineStart.y;
	float DeltaZ = LineEnd.z - LineStart.z;
	float LineLength = DeltaX * Normal.x + DeltaY * Normal.y + DeltaZ * Normal.z;
	if ( fabs( LineLength ) < EPSILON )
	{
		return FALSE;
	}
	float ToPlaneX = VertexOnPlane.x - LineStart.x;
	float ToPlaneY = VertexOnPlane.y - LineStart.y;
	float ToPlaneZ = VertexOnPlane.z - LineStart.z;
	float DistanceFromPlane = ToPlaneX * Normal.x + ToPlaneY * Normal.y + ToPlaneZ * Normal.z;

	float Percentage = DistanceFromPlane / LineLength;
	if ( Percentage <0.0f || Percentage> 1.0f )
	{
		return FALSE;
	}

	Intersection.x = LineStart.x + DeltaX * Percentage;
	Intersection.y = LineStart.y + DeltaY * Percentage;
	Intersection.z = LineStart.z + DeltaZ * Percentage;
	return TRUE;
} // GetLinePlaneIntersection()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GetSpherePlaneIntersection( 
	const VECTOR& LineStart,		// line segment start
	const VECTOR& LineEnd,			// line segment end
	const VECTOR& VertexOnPlane,	// point on plane
	const VECTOR& Normal,			// normal of plane
	VECTOR& Intersection )			// intersection point to fill
{
	float DeltaX = LineEnd.x - LineStart.x;
	float DeltaY = LineEnd.y - LineStart.y;
	float DeltaZ = LineEnd.z - LineStart.z;
	float LineLength = DeltaX * Normal.x + DeltaY * Normal.y + DeltaZ * Normal.z;
	if ( fabs( LineLength ) < EPSILON )
	{
		return FALSE;
	}
	float ToPlaneX = VertexOnPlane.x - LineStart.x;
	float ToPlaneY = VertexOnPlane.y - LineStart.y;
	float ToPlaneZ = VertexOnPlane.z - LineStart.z;
	float DistanceFromPlane = ToPlaneX * Normal.x + ToPlaneY * Normal.y + ToPlaneZ * Normal.z;

	float Percentage = DistanceFromPlane / LineLength;

	Intersection.x = LineStart.x + DeltaX * Percentage;
	Intersection.y = LineStart.y + DeltaY * Percentage;
	Intersection.z = LineStart.z + DeltaZ * Percentage;
	return TRUE;
} // GetSpherePlaneIntersection()
