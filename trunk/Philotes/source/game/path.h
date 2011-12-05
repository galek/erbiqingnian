//----------------------------------------------------------------------------
// path.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PATH_H
#define _PATH_H


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _BOUNDINGBOX_H_
#include "boundingbox.h"
#endif


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define CPATH_STARTING_SIZE 10


// ---------------------------------------------------------------------------
// DEBUG CONSTANTS & MACROS
// ---------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define DEBUG_CPATH_ALLOCATIONS									1
#else
#define DEBUG_CPATH_ALLOCATIONS									0
#endif

#if DEBUG_CPATH_ALLOCATIONS
#define CPATH_FUNCNOARGS()										const char * file = NULL, unsigned int line = 0
#define CPATH_FUNCARGS(...)										__VA_ARGS__, const char * file = NULL, unsigned int line = 0
#define CPATH_FUNCNOARGS_NODEFAULT()							const char * file, unsigned int line
#define CPATH_FUNCARGS_NODEFAULT(...)							__VA_ARGS__, const char * file, unsigned int line
#define CPATH_FUNC_FILELINE(...)								__VA_ARGS__, (const char *)__FILE__, (unsigned int)__LINE__
#define CPATH_FUNCNOARGS_PASSFL()								(const char *)(file), (unsigned int)(line)
#define CPATH_FUNC_PASSFL(...)									__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define CPATH_FUNC_FLPARAM(file, line, ...)						__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define CPATH_DEBUG_REF(x)										REF(x)
#define CPATH_NDEBUG_REF(x)									
#else
#define CPATH_FUNCNOARGS()										
#define CPATH_FUNCARGS(...)										__VA_ARGS__
#define CPATH_FUNCNOARGS_NODEFAULT()							
#define CPATH_FUNCARGS_NODEFAULT(...)							__VA_ARGS__
#define CPATH_FUNC_FILELINE(...)								__VA_ARGS__
#define CPATH_FUNCNOARGS_PASSFL()							
#define CPATH_FUNC_PASSFL(...)									__VA_ARGS__
#define CPATH_FUNC_FLPARAM(file, line, ...)						__VA_ARGS__
#define CPATH_DEBUG_REF(x)										
#define CPATH_NDEBUG_REF(x)										REF(x)
#endif


//----------------------------------------------------------------------------
// WRAPPER MACROS for CPath
//----------------------------------------------------------------------------
#define CPathInit(closed, pool)									Init(CPATH_FUNC_FILELINE(closed, pool))
#define CPathAddPoint(...)										AddPoint(CPATH_FUNC_FILELINE(__VA_ARGS__))


//----------------------------------------------------------------------------
// a path is a spline object, which may be open or closed
// used for object guide paths.
//----------------------------------------------------------------------------
class CPath
{
public:
	//------------------------------------------------------------------------
	// default constructor
	CPath(
		void)	
	{	
		Init(CPATH_FUNC_FILELINE(FALSE, NULL));
	};

	//------------------------------------------------------------------------
	// constructor
	CPath(CPATH_FUNCARGS(
		BOOL PathClosed,														// is this a closed spline?
		const VECTOR & Position,												// world-space location
		MEMORYPOOL * Pool))	:													// memory pool
			m_Position(Position),
			m_PathClosed(PathClosed),
			m_PathLength(0),
			m_Count(0)
	{
		Init(CPATH_FUNC_PASSFL(PathClosed, Pool));
	}

	//------------------------------------------------------------------------
	// copy constructor
	CPath(CPATH_FUNCARGS(
		CPath & Path,															// path to copy from
		MEMORYPOOL * Pool = NULL))												// mem pool
	{
		Init(CPATH_FUNC_PASSFL(FALSE, Pool));
		m_Position = Path.Position();
		m_PathClosed = Path.Closed();
		m_PathLength = Path.Length();

		m_tBounds = Path.Bounds();
		m_Count = Path.Points();

#if DEBUG_CPATH_ALLOCATIONS && DEBUG_MEMORY_ALLOCATIONS
		ArrayInitFL(m_Points, Pool, CPATH_STARTING_SIZE, file, line);
#else
		ArrayInit(m_Points, Pool, CPATH_STARTING_SIZE);
#endif
		for (unsigned int ii = 0; ii < m_Count; ++ii)
		{
			m_Points[ii] = Path.GetPathPoint(ii);
			m_Points[ii].m_vPoint -= m_Position;
		}
	}

	//------------------------------------------------------------------------
	// destructor
	~CPath(
		void)
	{
		m_Points.Destroy();
	}
	
	//------------------------------------------------------------------------
	// initialize the m_Points structure
	void Init(CPATH_FUNCARGS(
		BOOL PathClosed,
		MEMORYPOOL * Pool))
	{
		m_Position.x = 0;
		m_Position.y = 0;
		m_Position.z = 0;
		m_PathClosed = PathClosed;
		m_PathLength = 0;
#if DEBUG_CPATH_ALLOCATIONS && DEBUG_MEMORY_ALLOCATIONS
		ArrayInitFL(m_Points, Pool, CPATH_STARTING_SIZE, file, line);
#else
		ArrayInit(m_Points, Pool, CPATH_STARTING_SIZE);
#endif
	}

	//------------------------------------------------------------------------
	// calculates the full path length for a closed path - 
	// seams up the first and last points 
	inline void ClosePath(
		void)
	{
		VECTOR Start = GetPoint(0);
		Start -= GetPoint(Points() - 1);
		m_PathLength += VectorLength(Start);
	} 

	//------------------------------------------------------------------------
	// remove all points
	inline void Clear(
		void)
	{
		m_PathLength = 0;
		ArrayClear(m_Points);
		m_Count = 0;
	}

	//------------------------------------------------------------------------
	// ACCESSORS
	//------------------------------------------------------------------------
	inline float Length(
		void) const
	{	
		return m_PathLength;		
	};	

	inline float LengthOverride(
		void) const
	{	
		return m_PathLengthOverride;		
	};	

	inline unsigned int Points(
		void) const
	{	
		return m_Count;			
	};

	inline unsigned int	Segments(
		void) const	
	{	
		return  m_Count > 0 ? m_Count - 1 : 0;	
	};

	inline BOOL	Closed(
		void) const			
	{	
		return m_PathClosed;		
	};

	inline VECTOR GetPoint(
		unsigned int Index) const
	{
		if (Index < m_Count)
		{
			return m_Points[Index].m_vPoint + m_Position;
		}
		else
		{
			return m_Position;
		}
	}

	inline float GetRadiusLeft(
		unsigned int Index) const
	{
		if (m_PathClosed && Index >= m_Count)
		{
			Index -= m_Count;
		}
		if (Index < m_Count)
		{
			return m_Points[Index].m_fPathRadiusLeft;
		}
		return 0;
	}

	inline float GetRadiusRight(
		unsigned int Index) const
	{
		if (m_PathClosed && Index >= m_Count)
		{
			Index -= m_Count;
		}
		if (Index < m_Count)
		{
			return m_Points[Index].m_fPathRadiusRight;
		}
		return 0;
	}

	const BOUNDING_BOX & Bounds(
		void) const
	{	
		return m_tBounds;	
	};

	const VECTOR & Position(
		void) const
	{	
		return m_Position;	
	};

	// retrieve the distance of a point down th epath
	inline float GetPointDistance(
		unsigned int Index) const
	{
		if (Index < m_Count)
		{
			return m_Points[Index].m_fPointDistance;
		}
		return 0;
	}

	inline VECTOR GetPathSegment(
		unsigned int Segment) const
	{
		VECTOR PathSegment(0, 0, 0);
		if (m_Count < 2 || Segment > m_Count - 2)
		{
			return PathSegment;
		}

		PathSegment = m_Points[Segment + 1].m_vPoint - m_Points[Segment].m_vPoint;
		return PathSegment;
	}

	//------------------------------------------------------------------------
	// MUTATORS
	//------------------------------------------------------------------------

	inline void SetLengthOverride( 
		float Length )
	{
		m_PathLengthOverride = Length;
	}

	inline void SetRadiusLeft(
		unsigned int Index,
		float Radius)															// radius value ( should be negative )
	{
		if (Index < m_Count)
		{
			m_Points[Index].m_fPathRadiusLeft = Radius;
		}
	}

	inline void SetRadiusRight(
		unsigned int Index,
		float Radius)															// radius value ( should be positive )
	{
		if (Index < m_Count)
		{
			m_Points[Index].m_fPathRadiusRight = Radius;
		}
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	void AddPoint(CPATH_FUNCARGS(
		const VECTOR & Point,													// point to add to the path
		const VECTOR & Normal,													// the normal of the point to add to the path
		float PathRadiusLeft,													// left radius of the path at this point
		float PathRadiusRight)); 												// rightradius of the path at this point

	void AddPoint(CPATH_FUNCARGS(
		const VECTOR & Point,													// point to add to the path
		const VECTOR & Normal))
	{
		AddPoint(CPATH_FUNC_PASSFL(Point, Normal, -60.0f, 60.0f));
	}

	void CalculatePathWidth(
		CPath & LeftBorder,														// path to use as a left-hand border
		CPath & RightBorder);													// path to use as a right-hand border

	void Resize(CPATH_FUNCARGS(
		unsigned int  TargetPoints));											// target point count to resize to

	void Reverse(CPATH_FUNCNOARGS());

	float GetTweenedRadiusLeft(
		float DistanceAlongPath);												// given this path position, retrieve tweened radius value

	float GetTweenedRadiusRight(
		float DistanceAlongPath);												// given this path position, retrieve tweened radius value

	VECTOR GetPositionAtDistance(
		float DistanceAlongPath);												// distance along path to calculate location

	VECTOR GetSplinePositionAtDistance(
		float DistanceAlongPath);												// distance along path to calculate location

	VECTOR GetSegmentPerpendicularY(
		unsigned int Segment);													// segment to get the perpendicular of

	BOOL FindNearestPoint(
		const VECTOR& Position,													// starting position to find nearest point from
		VECTOR& Point,															// resulting nearest point on path
		VECTOR& Perpendicular,													// perpendicular in the direction of the starting position
		VECTOR& Up,																// normal to the ground
		int& Segment,															// Segment of the path on which the resulting point resides
		float& Distance,														// how far along the path the resulting point is
		float MinimumDistance = 0 ,												// minimum distance along the path to check beyond (default 0)
		float MaximumDistance = INFINITY );										// maximum distance along the path to check beyond (default 0)

private:
	struct PathPoint
	{
		VECTOR					m_vPoint;
		VECTOR					m_vNormal;
		float					m_fPathRadiusLeft;
		float					m_fPathRadiusRight;
		float					m_fPointDistance;

		PathPoint(
			void) : 
				m_vPoint(0, 0, 0), m_vNormal(0, 0, 0),
				m_fPathRadiusLeft(0), m_fPathRadiusRight(0), 
				m_fPointDistance(0)
		{
		}
	};

	SIMPLE_DYNAMIC_ARRAY<PathPoint>	m_Points;
	VECTOR						m_Position;										// worldspace location of path
	BOUNDING_BOX				m_tBounds;
	float						m_PathLength;									// total length of the path
	float						m_PathLengthOverride;							// overriden length of path
	unsigned int				m_Count;
	BOOL						m_PathClosed;									// is this path closed?

	// retrieve a PathPoint structure
	inline const PathPoint & GetPathPoint(	
		unsigned int Index) const
	{
		if (Index < m_Count)
		{
			return m_Points[Index];
		}
		static const PathPoint ZeroPoint;
		return ZeroPoint;
	}
};


#endif