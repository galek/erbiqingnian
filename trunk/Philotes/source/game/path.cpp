//----------------------------------------------------------------------------
// path.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "path.h"
#include "boundingbox.h"


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// add a point to the path - this must be done sequentially!
//----------------------------------------------------------------------------
void CPath::AddPoint(CPATH_FUNCARGS_NODEFAULT(
	const VECTOR & Point,														// point to add to the path
	const VECTOR & Normal,														// the normal of the point to add to the path
	float PathRadiusLeft,														// left radius of the path at this point
	float PathRadiusRight)) 													// rightradius of the path at this point
{
	if (Points() == 0)
	{
		m_tBounds.vMin = Point;
		m_tBounds.vMax = Point;
	}
	else
	{
		BoundingBoxExpandByPoint(&m_tBounds, &Point);
	}

	PathPoint tPoint;
	tPoint.m_vPoint = Point;
	tPoint.m_vNormal = Normal;
	tPoint.m_fPathRadiusLeft = PathRadiusLeft;
	tPoint.m_fPathRadiusRight = PathRadiusRight;

	// no points, so position along the path is 0
	if (m_Count < 1)
	{
		tPoint.m_fPointDistance = 0.0f;
		m_PathLength = 0;
	}
	else
	{
		// calculate the delta between this point and the previous one, and store distance

		VECTOR TempVEC3D = tPoint.m_vPoint - m_Points[m_Count - 1].m_vPoint;

		float Distance = VectorLength(TempVEC3D);

		m_PathLength += Distance;

		Distance += m_Points[m_Count - 1].m_fPointDistance;

		tPoint.m_fPointDistance = Distance;
		
	}

#if DEBUG_CPATH_ALLOCATIONS
	ArrayAddItemFL(m_Points, tPoint, file, line);
#else
	ArrayAddItem(m_Points, tPoint);
#endif
	++m_Count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CPath::Resize(CPATH_FUNCARGS_NODEFAULT(
	unsigned int TargetPoints))				// target point count to resize to
{
	ASSERT_RETURN(TargetPoints > 0);
	if (Points() <= TargetPoints)
	{
		return;
	}

	// copy our existing locations
	SIMPLE_DYNAMIC_ARRAY<PathPoint> PointsCopy;
#if DEBUG_CPATH_ALLOCATIONS
	ArrayCopyFL(PointsCopy, m_Points, file, line);
#else
	ArrayCopy(PointsCopy, m_Points);
#endif

	// calculate a step size based upon the ratio of old points to new
	float StepSize = (float)Points() / (float)TargetPoints;

	// clear out all the original data
	Clear();

	// and add back in only the points we need
	for (unsigned int ii = 0; ii < TargetPoints; ++ii)
	{
		CPathAddPoint(PointsCopy[(int)FLOOR(ii * StepSize + .5f)].m_vPoint, PointsCopy[(int)FLOOR(ii * StepSize + .5f)].m_vNormal);
	}

	// remember to stitch back up the path if it is closed
	if (Closed())
	{
		ClosePath();
	}

	PointsCopy.Destroy();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CPath::Reverse(
	CPATH_FUNCNOARGS_NODEFAULT())
{
	// copy our existing locations
	SIMPLE_DYNAMIC_ARRAY<PathPoint> PointsCopy;
#if DEBUG_CPATH_ALLOCATIONS
	ArrayCopyFL(PointsCopy, m_Points, file, line);
#else
	ArrayCopy(PointsCopy, m_Points );
#endif

	unsigned int Indices = Points();
	// clear out all the original data
	Clear();

	// and add back in only the points we need
	for (unsigned int ii = 0; ii < Indices; ++ii)
	{
		CPathAddPoint(PointsCopy[Indices - 1 - ii].m_vPoint, PointsCopy[Indices - 1 - ii].m_vNormal);
	}

	// remember to stitch back up the path if it is closed
	if (Closed())
	{
		ClosePath();
	}
}


//----------------------------------------------------------------------------
// recalculate the radius of the path based upon a left and right bordering path
// in this way, your path can have variable with along its route
// this is especially useful for pathing and racing games, as you
// will not only be able to tell your location along a path, but whether you
// are within the path's arbitrary bounds
//----------------------------------------------------------------------------
void CPath::CalculatePathWidth(
	CPath & LeftBorder,															// path to use as a left-hand border
	CPath & RightBorder )														// path to use as a right-hand border
{
	VECTOR	Point, Perpendicular, Up;
	int		Segment = 0;
	float		Distance = 0;
	for (unsigned int i = 0; i < m_Count; i++ )
	{

		if ( LeftBorder.FindNearestPoint( m_Points[i].m_vPoint,
										  Point,
										  Perpendicular,
										  Up,
										  Segment,
										  Distance ) )
		{
			VectorNormalize( Perpendicular );
			Point = Point - m_Points[i].m_vPoint;
			Point.y = 0;

			m_Points[i].m_fPathRadiusLeft = VectorDot( Point, Perpendicular );
			if ( m_Points[i].m_fPathRadiusLeft > 0 )
			{
				m_Points[i].m_fPathRadiusLeft = 0;
			}
		}
		if ( RightBorder.FindNearestPoint( m_Points[i].m_vPoint,
										   Point,
										   Perpendicular,
										   Up,
										   Segment,
										   Distance ) )
		{
			VectorNormalize( Perpendicular );
			Perpendicular *= -1;
			Point = Point - m_Points[i].m_vPoint;
			Point.y = 0;
			m_Points[i].m_fPathRadiusRight = VectorDot( Point, Perpendicular );
			if ( m_Points[i].m_fPathRadiusRight < 0 )
			{
				m_Points[i].m_fPathRadiusRight = 0;
			}
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float CPath::GetTweenedRadiusRight(
	float DistanceAlongPath) // given this path position, retrieve tweened radius value
{
	float Radius( 0 );

	while( DistanceAlongPath < 0 )
	{
		DistanceAlongPath += m_PathLength;
	}

	while( DistanceAlongPath > m_PathLength )
	{
		DistanceAlongPath -= m_PathLength;
	}

	for (unsigned int i = 0; i < m_Count ; i++ )
	{
		unsigned int NextPoint = i + 1;
		if(NextPoint >= m_Count )
		{
			NextPoint -= m_Count;
		}

		if( m_Points[NextPoint].m_fPointDistance > DistanceAlongPath ||
			NextPoint == 0 )
		{
			float Length = m_Points[NextPoint].m_fPointDistance - m_Points[i].m_fPointDistance;
			if( NextPoint == 0 && m_PathClosed )
			{
				Length = m_PathLength - m_Points[i].m_fPointDistance;
			}
			float Percentage = DistanceAlongPath - m_Points[i].m_fPointDistance;
			Percentage /= Length;
			Radius = ( m_Points[NextPoint].m_fPathRadiusRight - m_Points[i].m_fPathRadiusRight ) * Percentage;
			Radius += m_Points[i].m_fPathRadiusRight;
			return Radius;
		}
	}
	return Radius;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float CPath::GetTweenedRadiusLeft(
	float DistanceAlongPath ) // given this path position, retrieve tweened radius value
{
	float Radius( 0 );

	while( DistanceAlongPath < 0 )
	{
		DistanceAlongPath += m_PathLength;
	}

	while( DistanceAlongPath > m_PathLength )
	{
		DistanceAlongPath -= m_PathLength;

	}

	for (unsigned int i = 0; i < m_Count ; i++ )
	{
		unsigned int NextPoint = i + 1;
		if( NextPoint >= m_Count )
		{
			NextPoint -= m_Count;
		}

		if( m_Points[NextPoint].m_fPointDistance > DistanceAlongPath ||
			NextPoint == 0 )
		{
			float Length = m_Points[NextPoint].m_fPointDistance - m_Points[i].m_fPointDistance;
			if( NextPoint == 0 && m_PathClosed )
			{
				Length = m_PathLength - m_Points[i].m_fPointDistance;
			}
			float Percentage = DistanceAlongPath - m_Points[i].m_fPointDistance;
			Percentage /= Length;
			Radius = ( m_Points[NextPoint].m_fPathRadiusLeft - m_Points[i].m_fPathRadiusLeft ) * Percentage;
			Radius += m_Points[i].m_fPathRadiusLeft;
			return Radius;
		}
	}
	return Radius;

}


//----------------------------------------------------------------------------
// calculate the position at a given arbitrary distance along the path
//----------------------------------------------------------------------------
VECTOR CPath::GetPositionAtDistance(
	float DistanceAlongPath)	// distance along path to calculate location
{
	if( m_PathLength == 0 )
	{
		return m_Position;
	}
	VECTOR Position;
	while( DistanceAlongPath < 0 )
	{
		DistanceAlongPath += m_PathLength;
	}

	while( DistanceAlongPath > m_PathLength )
	{
		DistanceAlongPath -= m_PathLength;

	}
	if( DistanceAlongPath == m_PathLength )
	{
	    return m_Points[m_Count - 1].m_vPoint + m_Position;
	}
	if( DistanceAlongPath == 0 )
	{
	    return m_Points[0].m_vPoint + m_Position;
	}

	for (unsigned int i = 0; i < m_Count ; i++ )
	{
		unsigned int NextPoint = i + 1;
		if( NextPoint >= m_Count )
		{
			NextPoint -= m_Count;
		}

		if( m_Points[NextPoint].m_fPointDistance > DistanceAlongPath ||
			NextPoint == 0 )
		{
			Position = m_Points[NextPoint].m_vPoint - m_Points[i].m_vPoint;
			VectorNormalize( Position );

			Position *= ( DistanceAlongPath - m_Points[i].m_fPointDistance );
			Position += m_Points[i].m_vPoint;
			return Position + m_Position ;
		}
	}
	return Position + m_Position;
} 


//----------------------------------------------------------------------------
// calculate the catmull-rom position at a given arbitrary distance along the path
//----------------------------------------------------------------------------
VECTOR CPath::GetSplinePositionAtDistance(
	float DistanceAlongPath )	// distance along path to calculate location
{
	if( m_PathLength == 0 )
	{
		return m_Position;
	}
	while( DistanceAlongPath < 0 )
	{
		DistanceAlongPath += m_PathLength;
	}

	while( DistanceAlongPath > m_PathLength )
	{
		DistanceAlongPath -= m_PathLength;

	}
	// 4 points to be used to calculate the catmull rom location
	int A( 0 );
	int B( 0 );
	int C( 0 );
	int D( 0 );
	float Weight( 0 );

	for ( int i = m_Count - 1 ; i >= 0 ; i-- )
	{
		if( m_Points[i].m_fPointDistance < DistanceAlongPath )
		{
			B = i;
			break;
		}
	}

	A = B-1;
	C = B+1;
	D = B+2;

	int Size = m_Count;

	if( DistanceAlongPath == m_PathLength )
	{
	    return m_Points[Size - 1].m_vPoint + m_Position;
	}
	if( DistanceAlongPath == 0 )
	{
	    return m_Points[0].m_vPoint + m_Position;
	}

	if( A < 0 )
	{
		if( m_PathClosed )
		{
			A+= Size;
		}
		else
		{
			A = 0;
		}
	}
	if( C >= Size )
	{
		if( m_PathClosed )
		{
			C-= Size;
		}
		else
		{
			C = Size - 1;
		}
	}

	if( D >= Size )
	{
		if( m_PathClosed )
		{
			D -= Size;
		}
		else
		{
			D = Size - 1;
		}
	}

	Weight = m_Points[C].m_fPointDistance - m_Points[B].m_fPointDistance;
	if( Weight < 0 )
	{
		Weight += m_PathLength;
	}
	float DistanceDelta = DistanceAlongPath - m_Points[B].m_fPointDistance;
	if( DistanceAlongPath < m_Points[B].m_fPointDistance )
	{
		DistanceDelta += m_PathLength;
	}
	Weight = ( DistanceDelta ) / Weight;
	VECTOR vPosition;
	VectorCatMullRom(vPosition, m_Points[A].m_vPoint, m_Points[B].m_vPoint,
								m_Points[C].m_vPoint, m_Points[D].m_vPoint, Weight);

	return m_Position + vPosition;
} // CPath::GetSplinePositionAtDistance()



// get the perpendicular vector to the Y axis at a given point along the path
VECTOR CPath::GetSegmentPerpendicularY(unsigned int Segment )				// Segment to get the perpendicular of
{
	VECTOR Perpendicular( 0, 0, 0 );
	if ( m_Count < 2 || 
		 Segment > m_Count - 1 )
	{
		return Perpendicular;
	}

	unsigned int NextPoint = Segment + 1;
	if (NextPoint == Points() &&
		Closed() )
	{
		NextPoint = 0;
	}

	Perpendicular = m_Points[NextPoint].m_vPoint - m_Points[Segment].m_vPoint;

	Perpendicular.y = Perpendicular.z;
	Perpendicular.z = -Perpendicular.x;
	Perpendicular.x = Perpendicular.y;
	Perpendicular.y = 0;
	VectorNormalize( Perpendicular );
	return Perpendicular;
} // CPath::GetSegmentPerpendicularY()


BOOL CPath::FindNearestPoint( const VECTOR& Position,		// starting position to find nearest point from
							  VECTOR& Point,				// resulting nearest point on path
							  VECTOR& Perpendicular,		// perpendicular in the direction of the starting position
							  VECTOR& Up,					// normal to the ground
							  int& Segment,					// Segment of the path on which the resulting point resides
							  float& Distance,				// how far along the path the resulting point is
							  float MinimumDistance,		// minimum distance along the path to check beyond (default 0)
							  float MaximumDistance )		// maximum distance along the path to check beyond (default 0)
{
	if ( m_Count < 2 )
	{
		Distance = m_PathLength;
		return FALSE;
	}

	if( !m_PathClosed && MinimumDistance > m_PathLength )
	{
		MinimumDistance = m_PathLength;
	}
	float	BestDistance = INVALID_ID;

	float		Length;
	float		CurrentDistance;
	VECTOR	Delta;
	VECTOR	CurrentPosition;
	VECTOR	CurrentPerpendicular;
	// bring the position into the local space of the path
	VECTOR LocalPosition = Position - m_Position;

	int PathLength = m_Count;
	// for closed paths, we don't want to wrap around
	if( !m_PathClosed )
	{
		PathLength --;
	}

	// set our first, best distance to the first point on the spline.
	CurrentPosition = m_Points[0].m_vPoint;
	Up = m_Points[0].m_vNormal;
	CurrentPerpendicular = LocalPosition - CurrentPosition;
	Length = VectorLength( CurrentPerpendicular );
	Distance = 0;
	Point = CurrentPosition;
	Perpendicular = CurrentPerpendicular;
	//BestDistance = Length;
	Segment = 0;

	for ( int i = 0; i < PathLength; i++ )
	{
		if( BestDistance == INVALID_ID &&
			m_Points[i].m_fPointDistance >= MinimumDistance &&
			m_Points[i].m_fPointDistance < MaximumDistance )
		{
			CurrentPosition = m_Points[i].m_vPoint;
			Up = m_Points[i].m_vNormal;
			CurrentPerpendicular = LocalPosition - CurrentPosition;
			Length = VectorLength( CurrentPerpendicular );
			if ( Length < BestDistance )
			{
				Distance = m_Points[i].m_fPointDistance;
				Point = CurrentPosition;
				Perpendicular = CurrentPerpendicular;
				BestDistance = Length;
				Segment = i;
			}
		}

		unsigned int NextPoint = i + 1;
		if( NextPoint >= m_Count )
		{
			NextPoint -= m_Count;
		}
		Delta = m_Points[NextPoint].m_vPoint - m_Points[i].m_vPoint;
		Length = VectorLength( Delta );

		float	U	= ( ( ( LocalPosition.x - m_Points[i].m_vPoint.x ) * 
						  ( m_Points[NextPoint].m_vPoint.x - m_Points[i].m_vPoint.x ) ) +
						( ( LocalPosition.z - m_Points[i].m_vPoint.z ) * 
						  ( m_Points[NextPoint].m_vPoint.z - m_Points[i].m_vPoint.z ) ) +
						  ( ( LocalPosition.y - m_Points[i].m_vPoint.y ) * 
						  ( m_Points[NextPoint].m_vPoint.y - m_Points[i].m_vPoint.y ) ) ) /
					    ( Length * Length );

		if ( !( U < 0 || U > 1 ) )
		{
			CurrentDistance = m_Points[i].m_fPointDistance + Length * U;
			if ( CurrentDistance < MinimumDistance )
			{
				float	Makeup	= MinimumDistance - CurrentDistance + 1.0f;
				Makeup = ( 1.0f / Length ) * Makeup;
				U += Makeup;
				if ( U > 1 )
				{
					U = 1;
				}
				CurrentDistance = m_Points[i].m_fPointDistance  + Length * U;
			}

			if ( CurrentDistance >= MinimumDistance && 
				CurrentDistance < MaximumDistance)
			{
				CurrentPosition = Delta * U + m_Points[i].m_vPoint;
				Up = m_Points[i].m_vNormal;
				CurrentPerpendicular = LocalPosition - CurrentPosition;
				Length = VectorLength( CurrentPerpendicular );
				if ( Length < BestDistance )
				{
					Distance = CurrentDistance;
					Point = CurrentPosition;
					Perpendicular = CurrentPerpendicular;
					BestDistance = Length;
					Segment = i;
				}
			}
		}
		else
		{
			CurrentDistance = m_Points[NextPoint].m_fPointDistance ;
			if( NextPoint == 0 )
			{
				CurrentDistance = m_PathLength;
			}

			if ( CurrentDistance < MinimumDistance )
			{
				float	Makeup	= MinimumDistance - CurrentDistance + 1.0f;
				Makeup = ( 1.0f / Length ) * Makeup;
				U += Makeup;
				if ( U > 1 )
				{
					U = 1;
				}
				CurrentDistance = m_Points[i].m_fPointDistance + Length * U;
			}


			if ( CurrentDistance >= MinimumDistance &&
				CurrentDistance < MaximumDistance)
			{
				CurrentPosition = m_Points[NextPoint].m_vPoint;
				Up = m_Points[NextPoint].m_vNormal;
				CurrentPerpendicular = LocalPosition - CurrentPosition;
				Length = VectorLength( CurrentPerpendicular );
				if ( Length < BestDistance )
				{
					Distance = CurrentDistance;
					Point = CurrentPosition;
					Perpendicular = CurrentPerpendicular;
					BestDistance = Length;
					Segment = i;
				}
			}
		}
	}
	if ( BestDistance != INVALID_ID )
	{
		// move the resulting point into world space
		Point += m_Position;
		return true;
	}
	return false;
} // CPath::FindNearestPoint()

