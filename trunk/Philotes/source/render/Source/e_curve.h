//----------------------------------------------------------------------------
// e_curve.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_CURVE__
#define __E_CURVE__

#include "e_quaternion.h"
#include "e_lmhspline.h"

using namespace LMSPLINE;

namespace FSSE
{;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------


#define CURVE_DEFAULT_ORDER				3

#define MIN_CURVE_KNOTS					2

#define CURVE_DEFAULT_LENGTH_DELTA_PCT	0.0001f
#define CURVE_LENGTH_SEGMENTS_PER_KNOT	1

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//template<class T>
//struct HERMITE_NODE
//{
//	T pos;
//	T tan;
//};


struct CURVE_KNOT
{
	VECTOR3 pos;
};

struct CURVE
{
	int nKnots;
	CURVE_KNOT * pKnots;
	int * pnKnotTbl;
	float * pfKnotLengthTbl;
	float fCurveLength;

	int nOrder;
	MEMORYPOOL * pPool;

	PRESULT SetKnots( const VECTOR3 * pvKnotPositions, int nKnots );
	PRESULT GetPosition( VECTOR3 * vOut, float fDelta );
	PRESULT GetPositionNoCurve( VECTOR3 * pvOutPosition, float fDelta, VECTOR * pvOutDirection = NULL );
	PRESULT GetCatmullRomPosition( int nCurrentKnot, float time_delta, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection = NULL );
	PRESULT GetCatmullRomPosition( float T, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection = NULL );
	PRESULT GetSlideshowPosition( float T, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection = NULL );
	PRESULT GetSpeedMult( float T, float & fMult );
	void Init( MEMORYPOOL * _pPool, int _nOrder = CURVE_DEFAULT_ORDER )
	{
		pPool = _pPool;
		nOrder = _nOrder;
		pKnots = NULL;
		pnKnotTbl = NULL;
		pfKnotLengthTbl = NULL;
		nKnots = 0;
		fCurveLength = 0.f;
	}
	void Free()
	{
		if ( pKnots )
		{
			FREE( pPool, pKnots );
			pKnots = NULL;
		}
		if ( pnKnotTbl )
		{
			FREE( pPool, pnKnotTbl );
			pnKnotTbl = NULL;
		}
		if ( pfKnotLengthTbl )
		{
			FREE( pPool, pfKnotLengthTbl );
			pfKnotLengthTbl = NULL;
		}
		nKnots = 0;
	}

private:
	PRESULT SetKnotTable();
	void CalculateSplineDirection( int current_knot, float time_delta, VECTOR * vOutDirection );
	PRESULT GetCutmullRomSegmentLength( int nStartKnot, float & fLength, float fMaxDelta = CURVE_DEFAULT_LENGTH_DELTA_PCT );
	PRESULT GetSlideshowKnotPos( float T, float & fLength, float & fTimeDelta, int * pnCurrentKnot = NULL, int * pnCurrentSegment = NULL );
};






/* Portions Copyright (C) Alex Vlachos and John Isidoro, 2001 */

typedef char char8;

typedef char    int8;
typedef short   int16;
typedef int     int32;
typedef __int64 int64;

typedef float       float32;
typedef double      float64;
typedef long double float80;

typedef int fixed32;

typedef char bool8;

// UNSIGNED TYPEDEFS ==========================================================
typedef unsigned char    uint8;
typedef unsigned short   uint16;
typedef unsigned int     uint32;
typedef unsigned __int64 uint64;

typedef unsigned int ufixed32;

//=============================================================================
typedef union
{
	//Point in 3D space
	struct { float32 x, y, z; };
	struct { float32 v[3]; };
} FPVertex3, FPVector3;

//============================================================================
typedef union
{
	struct { uint8 b, g, r, a; };
	struct { uint32 color; };
} FPColorDWordRGBA;

//=============================================================================
typedef struct
{
	FPVertex3 vertex;
	FPColorDWordRGBA color1;
} FPVertexColor1;

//=============================================================================
typedef union
{
	//Point in 3D space
	struct { float32 x, y, z, w; };
	struct { float32 v[4]; };
} FPVertex4, FPVector4;

//=============================================================================
typedef struct
{
	FPVertex4 lo;
	FPVertex4 hi;
	FPVertex4 lo2;
	FPVertex4 hi2;
} FPSplineInterval;

//=============================================================================
class FlyPath
{
public:
	/******************************/
	/* Constructor and Destructor */
	/******************************/
	FlyPath();
	~FlyPath();

	/***************************/
	/* Start/Stop Fly Throughs */
	/***************************/
	void InitPath(void);
	bool8 StartFlight(float64 aTime);
	void PauseFlight(float64 aTime);
	bool8 ResumeFlight(float64 aTime);
	void EndFlight(void);
	void SetSpeedFactor(float32 a_fSpeedMul);
	void SetWrapMode( bool8 bWrap );

	float64 GetStartTime(void);
	float64 GetTotalPathTime(void);
	float64 GetTimeRelativeToStartFlight(float64 aTime);

	/*******************/
	/* Sample The Path */
	/*******************/
	bool8 GetPathPosition(FPVertex3 *aPosition, float64 aTime);
	bool8 GetPathOrientation(Quaternion *aOrientation, float64 aTime);
	bool8 GetPathInterval(int32 & aInterval, float64 aTime);

	/*******************/
	/* Compile Splines */
	/*******************/
	bool8 CompilePositions(void);
	bool8 CompileOrientations(void);
	bool8 CompilePositionsAndOrientations(void);
	bool8 CompileConstantSpeedSpline(void);

	/********************/
	/* Editing commands */
	/********************/
	bool8 AddFlyPointAfterCurrent(FPVertex3 position, Quaternion orientation);
	void ChangeCurrentFlyPoint(FPVertex3 position, Quaternion orientation);
	void DeleteCurrentFlyPoint(bool8 aSimulateBackspaceKey);
	void ToggleCutForCurrentFlyPoint(void);
	bool8 DrawFlyPath( const VECTOR2 & vWCenter, float fScale, const VECTOR2 & vSCenter );
	void SelectNearestFlyPoint(FPVertex3 aPos);
	bool8 GetCurrentFlyPoint(FPVertex3 *position, Quaternion *orientation);
	void ClearFlyPointSelection(void);
	bool8 GetCurrentFlyPointInfo(int32 *aPointNumber, float32 *aPointTime);

	/***************************/
	/* Next and Previous Paths */
	/***************************/
	void SetNextPath(FlyPath *aPath);
	void SetPreviousPath(FlyPath *aPath);
	FlyPath* GetNextPath(void);
	FlyPath* GetPreviousPath(void);

private:
	/*********************/
	/* Private Variables */
	/*********************/
	char8 mFileName[32];

	bool8 mInFlight;
	float64 mStartTime;
	float64 mPausedTime;
	bool8 mPaused;

	int32 mNumFlyPoints;
	int32 mCurrentEditPoint;
	float64 mCurrentEditPointTime;
	float64 mSampleInterval;
	float32 m_fSpeedMul;
	bool8 m_bWrap;

	float64 mPathTime;
	int32 mNumSplineIntervals;

	FPVertex4 *mLocation; //w is non-zero if this is a cut point
	FPSplineInterval *mLocationIntervals;
	//NOTE: Always a one-to-one correspondance between mLocation and mOrientation!!!

	Quaternion *mOrientation;
	FPSplineInterval *mOrientationIntervals;

	//If there is no previous and next, point to self
	FlyPath *mPreviousPath;
	FlyPath *mNextPath;

	// To handle constant speed playback
	HSpline<vector3d> * mptHSpline;
};



//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


}; // FSSE

#endif // __E_CURVE__