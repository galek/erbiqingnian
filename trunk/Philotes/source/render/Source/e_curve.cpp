//----------------------------------------------------------------------------
// e_curve.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_curve.h"
#include "e_primitive.h"


namespace FSSE
{;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------


// NOTE: first pass on curve uses code copied/modified from spline.cpp

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::SetKnotTable()
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETINVALIDARG( pnKnotTbl );

	int n = nKnots - 1;

	for ( int j = 0; j <= n + nOrder; j++ )
	{
		if ( j < nOrder )
		{
			pnKnotTbl[j] = 0;
		}
		else if ( j <= n )
		{
			pnKnotTbl[j] = j - nOrder + 1;
		}
		else if ( j > n )
		{
			pnKnotTbl[j] = n - nOrder + 2;
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::SetKnots( const VECTOR3 * pvKnotPositions, int nKnotCount )
{
	ASSERT_RETINVALIDARG( nKnotCount >= MIN_CURVE_KNOTS );
	ASSERT_RETINVALIDARG( pvKnotPositions );

	Free();

	nKnots = nKnotCount;

	pKnots = (CURVE_KNOT*) MALLOC( pPool, sizeof(CURVE_KNOT) * nKnots );
	ASSERT_RETVAL( pKnots, E_OUTOFMEMORY );

	pnKnotTbl = (int*) MALLOC( pPool, sizeof(int) * (nKnots + nOrder) );
	ASSERT_RETVAL( pnKnotTbl, E_OUTOFMEMORY );

	pfKnotLengthTbl = (float*) MALLOC( pPool, sizeof(float) * (nKnots - 1) * CURVE_LENGTH_SEGMENTS_PER_KNOT );
	ASSERT_RETVAL( pnKnotTbl, E_OUTOFMEMORY );

	for ( int i = 0; i < nKnots; ++i )
	{
		pKnots[i].pos = pvKnotPositions[i];
	}

	V_RETURN( SetKnotTable() );

	fCurveLength = 0.f;
	for ( int i = 0; i < nKnots - 1; ++i )
	{
		for ( int s = 0; s < CURVE_LENGTH_SEGMENTS_PER_KNOT; ++s )
		{
			V( GetCutmullRomSegmentLength( i, pfKnotLengthTbl[i*CURVE_LENGTH_SEGMENTS_PER_KNOT+s] ) );
			fCurveLength += pfKnotLengthTbl[i*CURVE_LENGTH_SEGMENTS_PER_KNOT+s];
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::GetSpeedMult( float T, float & fMult )
{
	if ( nKnots <= 0 )
		return S_FALSE;
	if ( ! pfKnotLengthTbl )
		return S_FALSE;

	float fCurrentKnot = (nKnots-1) * T;
	int nCurrentKnot = fCurrentKnot;
	float time_delta = fCurrentKnot - (float)nCurrentKnot;

	float fAvgLen = fCurveLength / (nKnots-1);
	float fCurLen = pfKnotLengthTbl[ nCurrentKnot ];

	fMult = fAvgLen / fCurLen;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static float sSplineBlend( int k, int t, int * u, float v )
{
	if ( t == 1 )
	{
		if ( ( u[k] <= (int)FLOOR(v) ) && ( (int)FLOOR(v) < u[k+1] ) )
			return 1.0f;
		else
			return 0.0f;
	}
	else
	{
		if ( ( u[k+t-1] == u[k] ) && ( u[k+t] == u[k+1] ) )
		{
			return 0.0f;
		}
		else if ( u[k+t-1] == u[k] )
		{

			return ( (float)u[k+t] - v ) / (float)( ( u[k+t] - u[k+1] ) ) * sSplineBlend( k+1, t-1, u, v );
		}
		else if ( u[k+t] == u[k+1] )
		{
			return ( v - (float)u[k] ) / (float)( ( u[k+t-1] - u[k] ) ) * sSplineBlend( k, t-1, u, v );
		}
		else
		{
			return	( v - (float)u[k] ) / (float)( ( u[k+t-1] - u[k] ) ) * sSplineBlend( k, t-1, u, v ) +
				( (float)u[k+t] - v ) / (float)( ( u[k+t] - u[k+1] ) ) * sSplineBlend( k+1, t-1, u, v );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::GetPosition( VECTOR * vOut, float delta )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	ASSERT_RETINVALIDARG( vOut );
	ASSERT_RETFAIL( pnKnotTbl );

	delta = SATURATE(delta);

	if ( delta == 0.0f )
	{
		*vOut = pKnots[0].pos;
		return S_OK;
	}

	if ( delta == 1.0f )
	{
		*vOut = pKnots[nKnots-1].pos;
		return S_OK;
	}

	float v = (float)( nKnots - nOrder + 1 ) * delta;

	*vOut = VECTOR( 0.0f, 0.0f, 0.0f );

	for ( int k = 0; k < nKnots; k++ )
	{
		float blend = sSplineBlend( k, nOrder, pnKnotTbl, v );
		*vOut += pKnots[k].pos * blend;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::GetPositionNoCurve( VECTOR * pvOutPosition, float delta, VECTOR * pvOutDirection /*= NULL*/ )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	ASSERT_RETINVALIDARG( pvOutPosition );

	delta = SATURATE(delta);

	if ( delta == 0.0f )
	{
		*pvOutPosition = pKnots[0].pos;
		if ( pvOutDirection && nKnots >= 2 )
			*pvOutDirection = pKnots[1].pos - pKnots[0].pos;
		return S_OK;
	}

	if ( delta == 1.0f )
	{
		*pvOutPosition = pKnots[nKnots-1].pos;
		if ( pvOutDirection && nKnots >= 2 )
			*pvOutDirection = pKnots[nKnots-1].pos - pKnots[nKnots-2].pos;
		return S_OK;
	}

	int k1 = FLOAT_TO_INT_FLOOR((nKnots-1) * delta);
	int k2 = k1 + 1;
	ASSERT_RETFAIL( k2 < nKnots );
	float f = (nKnots-1)*delta - (float)k1;

	*pvOutPosition = LERP( pKnots[k1].pos, pKnots[k2].pos, f );

	if ( pvOutDirection )
		*pvOutDirection = pKnots[k2].pos - pKnots[k1].pos;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CURVE::CalculateSplineDirection( int current_knot, float time_delta, VECTOR * vOutDirection )
{
	int pk = current_knot;
	float pt = time_delta - 0.1f;
	if ( pt < 0.0f )
	{
		if ( pk == 0 )
		{
			pt = 0.0f;
		}
		else
		{
			pk--;
			pt += 1.0f;
		}
	}
	int nk = current_knot;
	float nt = time_delta + 0.1f;
	if ( nt > 1.0f )
	{
		if ( ( nk + 1 ) >= nKnots )
		{
			nt = 1.0f;
		}
		else
		{
			nk++;
			nt -= 1.0f;
		}
	}
	VECTOR vOldPos, vNextPos;
	GetCatmullRomPosition( pk, pt, &vOldPos );
	GetCatmullRomPosition( nk, nt, &vNextPos );
	VECTOR vDirection = vNextPos - vOldPos;
	*vOutDirection = vDirection;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT CURVE::GetCatmullRomPosition( int nCurrentKnot, float time_delta, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection /*= NULL*/ )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	ASSERT_RETINVALIDARG( vOutPosition );
	ASSERT_RETINVALIDARG( nCurrentKnot >= 0 && nCurrentKnot < nKnots );
	time_delta = SATURATE( time_delta );

	if ( time_delta == 0.0f )
	{
		*vOutPosition = pKnots[nCurrentKnot].pos;
		if ( vOutDirection )
		{
			CalculateSplineDirection( nCurrentKnot, time_delta, vOutDirection );
		}
		return S_OK;
	}

	int previous_knot = nCurrentKnot - 1;
	if ( previous_knot < 0 ) previous_knot = 0;
	int next_knot = nCurrentKnot + 1;
	if ( next_knot >= nKnots ) next_knot = nKnots - 1;
	int last_knot = next_knot + 1;
	if ( last_knot >= nKnots ) last_knot = nKnots - 1;

	if ( time_delta == 1.0f )
	{
		*vOutPosition = pKnots[next_knot].pos;
		if ( vOutDirection )
		{
			CalculateSplineDirection( nCurrentKnot, time_delta, vOutDirection );
		}
		return S_OK;
	}

	VECTOR a = -1.0f * pKnots[previous_knot].pos;
	a += 3.0f * pKnots[nCurrentKnot].pos;
	a += -3.0f * pKnots[next_knot].pos;
	a += pKnots[last_knot].pos;
	a = a * time_delta * time_delta * time_delta;
	VECTOR b = 2.0f * pKnots[previous_knot].pos;
	b += -5.0f * pKnots[nCurrentKnot].pos;
	b += 4.0f * pKnots[next_knot].pos;
	b -= pKnots[last_knot].pos;
	b = b * time_delta * time_delta;
	VECTOR c = pKnots[next_knot].pos - pKnots[previous_knot].pos;
	c = c * time_delta;
	VECTOR d = 2.0f * pKnots[nCurrentKnot].pos;
	VECTOR vPos = a + b + c + d;
	*vOutPosition = 0.5f * vPos;
	if ( vOutDirection )
	{
		CalculateSplineDirection( nCurrentKnot, time_delta, vOutDirection );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------

PRESULT CURVE::GetSlideshowKnotPos( float T, float & fLength, float & fTimeDelta, int * pnCurrentKnot /*= NULL*/, int * pnCurrentSegment /*= NULL*/ )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	if ( ! pfKnotLengthTbl )
	{
		fLength = T * fCurveLength;
		return S_FALSE;
	}
	if ( fCurveLength <= 0.f )
	{
		fLength = 0.f;
		return S_FALSE;
	}
	if ( T >= 1.f )
	{
		if ( pnCurrentKnot )
			*pnCurrentKnot = nKnots-1;
		if ( pnCurrentSegment )
			*pnCurrentSegment = CURVE_LENGTH_SEGMENTS_PER_KNOT-1;
		fTimeDelta = 1.f;
		return S_OK;
	}

	float fCurLength = 0.f;

	for ( int k = 0; k < nKnots; ++k )
	{
		for ( int s = 0; s < CURVE_LENGTH_SEGMENTS_PER_KNOT; ++s )
		{
			if ( ( fCurLength / fCurveLength ) > T )
			{
				if ( pnCurrentKnot )
					*pnCurrentKnot = k;
				if ( pnCurrentSegment )
					*pnCurrentSegment = s;


				// THIS LINE CAUSES "SLIDESHOW" MODE
				fTimeDelta = T - ( fLength / fCurveLength );

				return S_OK;
			}

			fLength = fCurLength;
			fCurLength += pfKnotLengthTbl[ k * CURVE_LENGTH_SEGMENTS_PER_KNOT + s ];
		}
	}

	return E_FAIL;
}


//-------------------------------------------------------------------------------------------------
// NOTE: vOutDirection will NOT be normalized!
// T is the {0..1] value covering the entire curve length.
PRESULT CURVE::GetSlideshowPosition( float T, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection /*= NULL*/ )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	ASSERT_RETINVALIDARG( vOutPosition );

	int nCurrentKnot;
	int nCurrentSegment;
	float fPrevKnotLength;
	float fTimeDelta;
	V( GetSlideshowKnotPos( T, fPrevKnotLength, fTimeDelta, &nCurrentKnot, &nCurrentSegment ) );

	ASSERT_RETFAIL( nCurrentKnot < nKnots );
	ASSERT( fTimeDelta >= -0.01f );
	ASSERT( fTimeDelta <= 1.01f );
	fTimeDelta = SATURATE(fTimeDelta);

	return GetCatmullRomPosition( nCurrentKnot, fTimeDelta, vOutPosition, vOutDirection );
}



//-------------------------------------------------------------------------------------------------
// NOTE: vOutDirection will NOT be normalized!
// T is the {0..1] value covering the entire curve length.
PRESULT CURVE::GetCatmullRomPosition( float T, VECTOR3 * vOutPosition, VECTOR3 * vOutDirection /*= NULL*/ )
{
	ASSERT_RETFAIL( nKnots >= MIN_CURVE_KNOTS );
	ASSERT_RETFAIL( pKnots );
	ASSERT_RETINVALIDARG( vOutPosition );

	// Skew T by the lengths of the full curve
	//float fDist = fCurveLength * T;
	//int nCurrentKnot;
	//float time_delta;
	//for ( nCurrentKnot = 0; nCurrentKnot < nKnots-1; ++nCurrentKnot )
	//{
	//	float fNewLen = pfKnotLengthTbl[nCurrentKnot];
	//	if ( fDist < fNewLen )
	//	{
	//		time_delta = fDist / fNewLen;
	//		break;
	//	}
	//	fDist -= fNewLen;
	//}
	//ASSERT_RETFAIL( nCurrentKnot < nKnots );
	//time_delta = SATURATE(time_delta);


	//int nCurrentKnot;
	//int nCurrentSegment;
	//float fPrevKnotLength;
	//float fTimeDelta;
	//V( GetCurveLengthUntil( T, fPrevKnotLength, fTimeDelta, &nCurrentKnot, &nCurrentSegment ) );

	//float fCurrentKnotLen = 0.f;
	//for ( int i = 0; i < CURVE_LENGTH_SEGMENTS_PER_KNOT; ++i )
	//{
	//	fCurrentKnotLen += pfKnotLengthTbl[ nCurrentKnot * CURVE_LENGTH_SEGMENTS_PER_KNOT + i ];
	//}

	//float fCurrentKnotLen = pfKnotLengthTbl[ nCurrentKnot * CURVE_LENGTH_SEGMENTS_PER_KNOT + nCurrentSegment ];
	//float fCurKnotDist = (fCurveLength * T) - fPrevKnotLength;
	//float time_delta = fCurKnotDist / fCurrentKnotLen;
	//time_delta = SATURATE(time_delta);
	//ASSERT_RETFAIL( nCurrentKnot < nKnots );
	//ASSERT( fTimeDelta >= -0.01f );
	//ASSERT( fTimeDelta <= 1.01f );
	//fTimeDelta = SATURATE(fTimeDelta);


	float fCurrentKnot = (nKnots-1) * T;
	int nCurrentKnot = fCurrentKnot;
	float time_delta = fCurrentKnot - (float)nCurrentKnot;

	return GetCatmullRomPosition( nCurrentKnot, time_delta, vOutPosition, vOutDirection );
}


// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

PRESULT CURVE::GetCutmullRomSegmentLength( int nStartKnot, float & fLength, float fMaxDelta /*= CURVE_DEFAULT_LENGTH_DELTA_PCT*/ )
{
	ASSERT_RETINVALIDARG( nStartKnot >= 0 );

	if ( nStartKnot > (nKnots - 2) )
	{
		fLength = 0.f;
		return S_FALSE;
	}

	// Continually subdivide the segment until the difference between the sub of the sub-segment distances and the next subdivision is less than some threshold (fMaxDelta)

	int nSegments = 2;
	float fCurDelta;
	float fArcSegLengths;
	float fLastLength;

	VECTOR vStart, vEnd;
	V( GetCatmullRomPosition( nStartKnot, 0.f, &vStart ) );
	V( GetCatmullRomPosition( nStartKnot, 1.f, &vEnd   ) );

	fArcSegLengths = VectorDistanceSquared( vStart, vEnd );
	fArcSegLengths = SQRT_SAFE( fArcSegLengths );

	do
	{
		fLastLength = fArcSegLengths;
		fArcSegLengths = 0.f;

		float fStep = 1.f / nSegments;

		for ( int s = 0; s < nSegments; s++ )
		{
			V( GetCatmullRomPosition( nStartKnot, s     * fStep, &vStart ) );
			V( GetCatmullRomPosition( nStartKnot, (s+1) * fStep, &vEnd   ) );

			float fDistSqr = VectorDistanceSquared( vStart, vEnd );
			float fDist = SQRT_SAFE( fDistSqr );

			fArcSegLengths += fDist;
		}

		fCurDelta = fabs( fArcSegLengths - fLastLength );
		fCurDelta = fCurDelta / max( fArcSegLengths, fLastLength );

		nSegments *= 2;
	}
	while ( fCurDelta > fMaxDelta );

	fLength = fLastLength;

	return S_OK;
}

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------



/* Portions Copyright (C) Alex Vlachos and John Isidoro, 2001 */

// INCLUDES ===================================================================
//#include "Ati.h"

// DEFINES ====================================================================
#define FLY_CUBE_RADIUS 2.5f
#define FLY_SPLINE_TENSION (-0.5f)
#define FLY_S ((1.0f-FLY_SPLINE_TENSION)*0.5f)
#define STRING_LENGTH 64

// MACROS =====================================================================
#define FPFreeArray(array, type)					\
	if (array != NULL)								\
		FREE_DELETE_ARRAY( NULL, array, type );     \
	array = NULL;

#define FPMallocArray(array,num,type)							\
	array = NULL;												\
	if (num > 0)												\
	{															\
		array = MALLOC_NEWARRAY( NULL, type, num );				\
		if (array == NULL)										\
		{														\
			return FALSE;										\
		}														\
		memset (array, 0, sizeof(type) * num);					\
	}

// GLOBALS ====================================================================
MATRIX sgCardinalMatrix = { -FLY_S,			2.0f*FLY_S,			-FLY_S,			0.0f,
							2.0f-FLY_S,		FLY_S-3.0f,			0.0f,			1.0f,
							FLY_S-2.0f,		3.0f-2.0f*FLY_S,	FLY_S,			0.0f,
							FLY_S,			-FLY_S,				0.0f,			0.0f };

static char8 sgFileBase[STRING_LENGTH] = "";

//=============================================================================
FlyPath::FlyPath()
{
	mFileName[0] = '\0';

	mInFlight = FALSE;
	mStartTime = 0.0;
	mPausedTime = 0.0;
	mPaused = FALSE;

	mNumFlyPoints = 0;
	mCurrentEditPoint = -1;
	mCurrentEditPointTime = 0.0;
	mSampleInterval = 1.0;
	mPathTime = 0.0;
	mNumSplineIntervals = 0;
	m_fSpeedMul = 1.0;
	m_bWrap = false;

	mLocation = NULL;
	mLocationIntervals = NULL;

	mOrientation = NULL;
	mOrientationIntervals = NULL;

	mptHSpline = NULL;

	mPreviousPath = this;
	mNextPath = this;
}

//=============================================================================
FlyPath::~FlyPath()
{
	InitPath();
}

////=============================================================================
//void FlyPath::SetBasePath (char8 *aPath)
//{
//	strcpy(sgFileBase, aPath);
//}

//=============================================================================
void FlyPath::InitPath (void)
{
	mInFlight = FALSE;
	mStartTime = 0.0;
	mPausedTime = 0.0;
	mPaused = FALSE;

	mNumFlyPoints = 0;
	mCurrentEditPoint = -1;
	mCurrentEditPointTime = 0.0;
	mSampleInterval = 1.0;
	mPathTime = 0.0;
	mNumSplineIntervals = 0;

	FREE_DELETE( NULL, mptHSpline, HSpline<vector3d> );

	FPFreeArray(mLocation, FPVertex4);
	FPFreeArray(mLocationIntervals, FPSplineInterval);

	FPFreeArray(mOrientation, Quaternion);
	FPFreeArray(mOrientationIntervals, FPSplineInterval);
}

////=============================================================================
//bool8 FlyPath::ReadFile (char8 *aFileName)
//{
//	int32 i;
//	FILE *fp;
//	float32 tmpf;
//	char8 buff[STRING_LENGTH*2];
//
//	/*************************/
//	/* Init Member Variables */
//	/*************************/
//	InitPath();
//
//	/*****************************/
//	/* Copy File Name To Current */
//	/*****************************/
//	strncpy(mFileName, aFileName, 31);
//	mFileName[31] = '\0';
//
//	/*************/
//	/* Open File */
//	/*************/
//	sprintf(buff, "%s%s", sgFileBase, aFileName);
//	fp = fopen(buff, "r");
//	if (fp == NULL)
//		return FALSE;
//
//	/***********************/
//	/* Get Sample Interval */
//	/***********************/
//	fscanf(fp, "%f", &tmpf);
//	mSampleInterval = tmpf;
//
//	/************************/
//	/* Get number of points */
//	/************************/
//	fscanf(fp, "%d", &mNumFlyPoints);
//	if (mNumFlyPoints <= 0)
//	{
//		mNumFlyPoints = 0;
//		mCurrentEditPoint = -1;
//		return TRUE;
//	}
//	mCurrentEditPoint = 0;
//
//	/*******************/
//	/* Allocate Memory */
//	/*******************/
//	FPMallocArray(mLocation, mNumFlyPoints+1, FPVertex4);
//	FPMallocArray(mOrientation, mNumFlyPoints+1, Quaternion);
//
//	/************/
//	/* Get data */
//	/************/
//	for (i=0; i<mNumFlyPoints; i++)
//	{
//		fscanf(fp, "%f %f %f %f", &(mLocation[i].x), &(mLocation[i].y), &(mLocation[i].z), &(mLocation[i].w));
//		fscanf(fp, "%f %f %f %f", &(mOrientation[i].x), &(mOrientation[i].y), &(mOrientation[i].z), &(mOrientation[i].w));
//
//		//Read in the hex versions of the floats
//		//fscanf(fp, "%x %x %x %x", &(mLocation[i].x), &(mLocation[i].y), &(mLocation[i].z), &(mLocation[i].w));
//		//fscanf(fp, "%x %x %x %x", &(mOrientation[i].x), &(mOrientation[i].y), &(mOrientation[i].z), &(mOrientation[i].w));
//	}
//
//	/*********************************************/
//	/* Don't allow first point to be a cut point */
//	/*********************************************/
//	mLocation[0].w = 0.0f;
//
//	/**************/
//	/* Close File */
//	/**************/
//	fclose(fp);
//
//	/********************/
//	/* Compile Fly Path */
//	/********************/
//	if (CompilePositionsAndOrientations() == FALSE)
//		return FALSE;
//
//	/***************************/
//	/* Finished With No Errors */
//	/***************************/
//	return TRUE;
//}

////=============================================================================
//bool8 FlyPath::ReadCurrentFile (void)
//{
//	return ReadFile(mFileName);
//}

////=============================================================================
//bool8 FlyPath::WriteFile (char8 *aFileName)
//{
//	int32 i;
//	FILE *fp;
//	char8 buff[STRING_LENGTH*2];
//
//	/***********************************/
//	/* Make sure we have enough points */
//	/***********************************/
//	if (mNumFlyPoints < 5)
//		return FALSE;
//
//	/*************/
//	/* Open File */
//	/*************/
//	sprintf(buff, "%s%s", sgFileBase, aFileName);
//	fp = fopen(buff, "w");
//	if (fp == NULL)
//		return FALSE;
//
//	/*************************/
//	/* Write Sample Interval */
//	/*************************/
//	fprintf(fp, "%f\n", mSampleInterval);
//
//	/**************************/
//	/* Write number of points */
//	/**************************/
//	fprintf(fp, "%d\n", mNumFlyPoints);
//
//	/************************************/
//	/* Write positions and orientations */
//	/************************************/
//	for (i=0; i<mNumFlyPoints; i++)
//	{
//		fprintf(fp, "%f %f %f %f\n", mLocation[i].x, mLocation[i].y, mLocation[i].z, mLocation[i].w);
//		fprintf(fp, "%f %f %f %f\n", mOrientation[i].x, mOrientation[i].y, mOrientation[i].z, mOrientation[i].w);
//
//		//Write out the hex versions of the floats
//		//fprintf(fp, "%x %x %x %x\n", *(int*)&(mLocation[i].x), *(int*)&(mLocation[i].y), *(int*)&(mLocation[i].z), *(int*)&(mLocation[i].w));
//		//fprintf(fp, "%x %x %x %x\n", *(int*)&(mOrientation[i].x), *(int*)&(mOrientation[i].y), *(int*)&(mOrientation[i].z), *(int*)&(mOrientation[i].w));
//	}
//
//	/**************/
//	/* Close File */
//	/**************/
//	fclose(fp);
//
//	/***************************/
//	/* Finished With No Errors */
//	/***************************/
//	return TRUE;
//}
//
////=============================================================================
//bool8 FlyPath::WriteCurrentFile (void)
//{
//	return WriteFile(mFileName);
//}

//=============================================================================
bool8 FlyPath::StartFlight (float64 aTime)
{
	/***********************************/
	/* Make sure we have enough points */
	/***********************************/
	if (mNumFlyPoints < 5)
		return FALSE;

	/*****************/
	/* Set variables */
	/*****************/
	mInFlight = TRUE;
	mPaused = FALSE;
	mStartTime = aTime;

	/***********************************************/
	/* Engine specific: This will start you at the */
	/* selected position when in editing mode      */
	/***********************************************/
	//if (gEditingPath == TRUE)
	//   mStartTime -= mCurrentEditPointTime;

	return TRUE;
}

//=============================================================================
void FlyPath::PauseFlight (float64 aTime)
{
	mInFlight = FALSE;
	mPaused = TRUE;
	mPausedTime = aTime;
}

//=============================================================================
bool8 FlyPath::ResumeFlight (float64 aTime)
{
	if ((mPaused == FALSE) && (mInFlight == FALSE))
	{
		return StartFlight(aTime);
	}

	if (mPaused == FALSE)
		return FALSE;

	mStartTime += aTime - mPausedTime;
	mPaused = FALSE;
	mInFlight = TRUE;

	return TRUE;
}

//=============================================================================
void FlyPath::EndFlight (void)
{
	mInFlight = FALSE;
	mPaused = FALSE;
}

//=============================================================================
void FlyPath::SetSpeedFactor (float32 a_fSpeedMul)
{
	m_fSpeedMul = float64( 1.f ) / a_fSpeedMul;
}

void FlyPath::SetWrapMode( bool8 bWrap )
{
	m_bWrap = bWrap;
}

//=============================================================================
float64 FlyPath::GetStartTime (void)
{
	return (mStartTime);
}

//=============================================================================
float64 FlyPath::GetTotalPathTime (void)
{
	return m_bWrap ? (mPathTime) : (mPathTime - 1);
}

//=============================================================================
float64 FlyPath::GetTimeRelativeToStartFlight (float64 aTime)
{
	if (mPaused == TRUE)
		return (mPausedTime - mStartTime);
	else
		return (aTime - mStartTime);
}

//=============================================================================
bool8 FlyPath::AddFlyPointAfterCurrent (FPVertex3 aPosition, Quaternion aOrientation)
{
	FPVertex4 *tmpLocation = NULL;
	Quaternion *tmpOrientation = NULL;

	/******************************/
	/* Make sure we're not flying */
	/******************************/
	if (mInFlight == TRUE)
		return FALSE;

	/*********************************/
	/* Allocate memory for new point */
	/*********************************/
	tmpLocation = mLocation;
	FPMallocArray(mLocation, mNumFlyPoints+1, FPVertex4);

	tmpOrientation = mOrientation;
	FPMallocArray(mOrientation, mNumFlyPoints+1, Quaternion);

	/*********************************************/
	/* Coyp all points before current edit point */
	/*********************************************/
	if ((mNumFlyPoints > 0) && (mCurrentEditPoint >= 0))
	{
		memcpy(mLocation, tmpLocation, sizeof(FPVertex4)*(mCurrentEditPoint+1));
		memcpy(mOrientation, tmpOrientation, sizeof(Quaternion)*(mCurrentEditPoint+1));
	}

	/********************************************/
	/* Copy all points after current edit point */
	/********************************************/
	if ((mCurrentEditPoint+1) < mNumFlyPoints) //If last point is not current
	{
		memcpy(&(mLocation[mCurrentEditPoint+2]), &(tmpLocation[mCurrentEditPoint+1]), sizeof(FPVertex4)*(mNumFlyPoints-mCurrentEditPoint-1));
		memcpy(&(mOrientation[mCurrentEditPoint+2]), &(tmpOrientation[mCurrentEditPoint+1]), sizeof(Quaternion)*(mNumFlyPoints-mCurrentEditPoint-1));
	}

	/*********************/
	/* Now add new point */
	/*********************/
	mLocation[mCurrentEditPoint+1].x = aPosition.x;
	mLocation[mCurrentEditPoint+1].y = aPosition.y;
	mLocation[mCurrentEditPoint+1].z = aPosition.z;
	mLocation[mCurrentEditPoint+1].w = 0.0f;

	mOrientation[mCurrentEditPoint+1].x = aOrientation.x;
	mOrientation[mCurrentEditPoint+1].y = aOrientation.y;
	mOrientation[mCurrentEditPoint+1].z = aOrientation.z;
	mOrientation[mCurrentEditPoint+1].w = aOrientation.w;

	/*******************/
	/* Update counters */
	/*******************/
	mNumFlyPoints++;
	mCurrentEditPoint++;

	/*******************/
	/* Free tmp memory */
	/*******************/
	FPFreeArray(tmpLocation, FPVertex4);
	FPFreeArray(tmpOrientation, Quaternion);

	/********************/
	/* Compile Fly Path */
	/********************/
	if (CompilePositionsAndOrientations() == FALSE)
		return FALSE;

	return TRUE;
}

//=============================================================================
void FlyPath::ChangeCurrentFlyPoint (FPVertex3 aPosition, Quaternion aOrientation)
{
	/******************************/
	/* Make sure we're not flying */
	/******************************/
	if (mInFlight == TRUE)
		return;

	/*************************************/
	/* Make sure we have a point to move */
	/*************************************/
	if (mCurrentEditPoint < 0)
		return;

	/**********/
	/* Update */
	/**********/
	mLocation[mCurrentEditPoint].x = aPosition.x;
	mLocation[mCurrentEditPoint].y = aPosition.y;
	mLocation[mCurrentEditPoint].z = aPosition.z;

	mOrientation[mCurrentEditPoint].x = aOrientation.x;
	mOrientation[mCurrentEditPoint].y = aOrientation.y;
	mOrientation[mCurrentEditPoint].z = aOrientation.z;
	mOrientation[mCurrentEditPoint].w = aOrientation.w;

	/********************/
	/* Compile Fly Path */
	/********************/
	if (CompilePositionsAndOrientations() == FALSE)
		return;
}

//=============================================================================
void FlyPath::DeleteCurrentFlyPoint (bool8 aSimulateBackspaceKey)
{
	/******************************/
	/* Make sure we're not flying */
	/******************************/
	if (mInFlight == TRUE)
		return;

	/***************************************/
	/* Make sure we have a point to delete */
	/***************************************/
	if ((mCurrentEditPoint < 0) || (mNumFlyPoints == 0))
		return;

	/*****************************************************/
	/* Copy all points after current edit point back one */
	/*****************************************************/
	if ((mCurrentEditPoint+1) < mNumFlyPoints) //If last point is not current
	{
		memcpy(&(mLocation[mCurrentEditPoint]), &(mLocation[mCurrentEditPoint+1]), sizeof(FPVertex4)*(mNumFlyPoints-mCurrentEditPoint-1));
		memcpy(&(mOrientation[mCurrentEditPoint]), &(mOrientation[mCurrentEditPoint+1]), sizeof(Quaternion)*(mNumFlyPoints-mCurrentEditPoint-1));
	}

	/*******************/
	/* Update counters */
	/*******************/
	mNumFlyPoints--;
	if (aSimulateBackspaceKey == TRUE)
	{
		if ((mCurrentEditPoint != 0) || (mNumFlyPoints == 0))
			mCurrentEditPoint--;
	}
	else //Like delete key
	{
		if (mCurrentEditPoint == mNumFlyPoints)
			mCurrentEditPoint--;
	}

	/********************/
	/* Compile Fly Path */
	/********************/
	if (CompilePositionsAndOrientations() == FALSE)
		return;
}

//=============================================================================
void FlyPath::ToggleCutForCurrentFlyPoint (void)
{
	/******************************/
	/* Make sure we're not flying */
	/******************************/
	if (mInFlight == TRUE)
		return;

	/**********/
	/* Toggle */
	/**********/
	if (mCurrentEditPoint > 0) //Any point except first point
	{
		if (mLocation[mCurrentEditPoint].w == 0.0)
			mLocation[mCurrentEditPoint].w = 1.0;
		else
			mLocation[mCurrentEditPoint].w = 0.0;
	}

	/********************/
	/* Compile Fly Path */
	/********************/
	if (CompilePositionsAndOrientations() == FALSE)
		return;
}

//=============================================================================
// NOTE: This isn't meant to be very optimal since end-users won't ever see this
//=============================================================================
bool8 FlyPath::DrawFlyPath ( const VECTOR2 & vWCenter, float fScale, const VECTOR2 & vSCenter )
{
	int32 i;
	int32 j;
	int32 numVertices = 0;
	FPVertexColor1 *vertexArray;
	int32 numIndices = 0;
	uint16 *indexArray;

	bool8 inFlight = mInFlight;
	float64 startTime = mStartTime;
	float64 pausedTime = mPausedTime;
	bool8 paused = mPaused;


	float fDepth = 0.1f;
	DWORD dwLineFlags = PRIM_FLAG_RENDER_ON_TOP;
	float fRadius = fScale * 1.5f;
	DWORD dwKnotColor  = 0xffffff00;
	DWORD dwLineColor  = 0xff00ffff;
	DWORD dwCurveColor = 0xff00ff00;


	if (mNumFlyPoints <= 0)
		return TRUE;

	/*******************/
	/* Allocate memory */
	/*******************/
	FPMallocArray(vertexArray, mNumFlyPoints*46, FPVertexColor1);
	FPMallocArray(indexArray, mNumFlyPoints*46, uint16);

	/******************/
	/* Reset counters */
	/******************/
	numVertices = 0;
	numIndices = 0;

	if (mNumFlyPoints >= 5)
	{
		/**********************************/
		/* Build draw array for cut lines */
		/**********************************/
		//for (i=0; i<mNumFlyPoints-1; i++)
		//{
		//	if (mLocation[i].w != 0.0f)
		//	{
		//		//This point
		//		vertexArray[numVertices].vertex.x = mLocation[i].x;
		//		vertexArray[numVertices].vertex.y = mLocation[i].y;
		//		vertexArray[numVertices].vertex.z = mLocation[i].z;
		//		vertexArray[numVertices].color1.r = 0;
		//		vertexArray[numVertices].color1.g = 0;
		//		vertexArray[numVertices].color1.b = 255;
		//		numVertices++;

		//		//Next point
		//		vertexArray[numVertices].vertex.x = mLocation[i+1].x;
		//		vertexArray[numVertices].vertex.y = mLocation[i+1].y;
		//		vertexArray[numVertices].vertex.z = mLocation[i+1].z;
		//		vertexArray[numVertices].color1.r = 0;
		//		vertexArray[numVertices].color1.g = 0;
		//		vertexArray[numVertices].color1.b = 255;
		//		numVertices++;
		//	}
		//}

		//if (mNumFlyPoints > 0)
		//{
		//	if ((mLocation[i].w != 0.0f) && (mNextPath->mNumFlyPoints > 0))
		//	{
		//		//This point
		//		vertexArray[numVertices].vertex.x = mLocation[i].x;
		//		vertexArray[numVertices].vertex.y = mLocation[i].y;
		//		vertexArray[numVertices].vertex.z = mLocation[i].z;
		//		vertexArray[numVertices].color1.r = 0;
		//		vertexArray[numVertices].color1.g = 0;
		//		vertexArray[numVertices].color1.b = 255;
		//		numVertices++;

		//		//Next point
		//		vertexArray[numVertices].vertex.x = mNextPath->mLocation[0].x;
		//		vertexArray[numVertices].vertex.y = mNextPath->mLocation[0].y;
		//		vertexArray[numVertices].vertex.z = mNextPath->mLocation[0].z;
		//		vertexArray[numVertices].color1.r = 0;
		//		vertexArray[numVertices].color1.g = 0;
		//		vertexArray[numVertices].color1.b = 255;
		//		numVertices++;
		//	}
		//}



		for (i=0; i<mNumFlyPoints; i++)
		{
			VECTOR2 vC = *(VECTOR2*)&mLocation[i].x;
			vC -= vWCenter;
			vC *= fScale;
			vC += vSCenter;
			e_PrimitiveAddCircle( dwLineFlags, &vC, fRadius, dwKnotColor, fDepth );

			if ( i > 0 )
			{
				VECTOR2 vP = *(VECTOR2*)&mLocation[i-1].x;
				vP -= vWCenter;
				vP *= fScale;
				vP += vSCenter;
				e_PrimitiveAddLine( dwLineFlags, &vP, &vC, dwLineColor, fDepth );
			}
		}




		/*******************************/
		/* Build draw array for spline */
		/*******************************/
		StartFlight(0.0);
		for (i=0; i<mNumSplineIntervals; i++)
		{
			for (j=0; j<7; j++)
			{
				float64 x1 = 0.0;
				float64 x2 = 0.0;

				//MATRIX matrix;
				Quaternion quat;
				FPVertex3 pos;

				x1 = (i*7 + (j+0)) * (mSampleInterval/7.0);
				x2 = (i*7 + (j+1)) * (mSampleInterval/7.0);
				if (j == 6)
					x2 -= 0.001f;

				if (GetPathPosition(&pos, x1) == TRUE)
				{
					vertexArray[numVertices].vertex.x = pos.x;
					vertexArray[numVertices].vertex.y = pos.y;
					vertexArray[numVertices].vertex.z = pos.z;
					vertexArray[numVertices].color1.r = 255;
					vertexArray[numVertices].color1.g = 0;
					vertexArray[numVertices].color1.b = 255;
					numVertices++;
				}

				if (GetPathPosition(&pos, x2) == TRUE)
				{
					vertexArray[numVertices].vertex.x = pos.x;
					vertexArray[numVertices].vertex.y = pos.y;
					vertexArray[numVertices].vertex.z = pos.z;
					vertexArray[numVertices].color1.r = 255;
					vertexArray[numVertices].color1.g = 0;
					vertexArray[numVertices].color1.b = 255;
					numVertices++;
				}

				//if (GetPathOrientation(&quat, x2) == TRUE)
				//{
				//	QuatToMatrix(&quat, matrix);

				//	//This point
				//	vertexArray[numVertices].vertex.x = pos.x;
				//	vertexArray[numVertices].vertex.y = pos.y;
				//	vertexArray[numVertices].vertex.z = pos.z;
				//	vertexArray[numVertices].color1.r = 255;
				//	vertexArray[numVertices].color1.g = 255;
				//	vertexArray[numVertices].color1.b = 255;
				//	numVertices++;

				//	//Up Vector
				//	//vertexArray[numVertices].vertex.x = pos.x + matrix[1]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].vertex.y = pos.y + matrix[5]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].vertex.z = pos.z + matrix[9]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 255;
				//	//numVertices++;

				//	//This point
				//	//vertexArray[numVertices].vertex.x = pos.x;
				//	//vertexArray[numVertices].vertex.y = pos.y;
				//	//vertexArray[numVertices].vertex.z = pos.z;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 0;
				//	//numVertices++;

				//	//View Vector
				//	//vertexArray[numVertices].vertex.x = pos.x - matrix[2]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].vertex.y = pos.y - matrix[6]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].vertex.z = pos.z - matrix[10]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 0;
				//	//numVertices++;
				//}

				//if ((j == 0) && (GetPathOrientation(&quat, x1) == TRUE))
				//{
				//	QuatToMatrix(&quat, matrix);
				//	GetPathPosition(&pos, x1);

				//	//This point
				//	vertexArray[numVertices].vertex.x = pos.x;
				//	vertexArray[numVertices].vertex.y = pos.y;
				//	vertexArray[numVertices].vertex.z = pos.z;
				//	vertexArray[numVertices].color1.r = 255;
				//	vertexArray[numVertices].color1.g = 255;
				//	vertexArray[numVertices].color1.b = 255;
				//	numVertices++;

				//	//Up Vector
				//	//vertexArray[numVertices].vertex.x = pos.x + matrix[1]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].vertex.y = pos.y + matrix[5]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].vertex.z = pos.z + matrix[9]*FLY_CUBE_RADIUS*2.0f;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 255;
				//	//numVertices++;

				//	//This point
				//	//vertexArray[numVertices].vertex.x = pos.x;
				//	//vertexArray[numVertices].vertex.y = pos.y;
				//	//vertexArray[numVertices].vertex.z = pos.z;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 0;
				//	//numVertices++;

				//	//View Vector
				//	//vertexArray[numVertices].vertex.x = pos.x - matrix[2]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].vertex.y = pos.y - matrix[6]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].vertex.z = pos.z - matrix[10]*FLY_CUBE_RADIUS*4.0f;
				//	//vertexArray[numVertices].color1.r = 255;
				//	//vertexArray[numVertices].color1.g = 255;
				//	//vertexArray[numVertices].color1.b = 0;
				//	//numVertices++;
				//}
			}
		}
		EndFlight();

		for (i=0; i<numVertices; i++)
		{
			indexArray[numIndices] = i;
			numIndices++;
		}

		/**********************************************/
		/* Draw: An example draw call from our engine */
		/**********************************************/
		//AwDraw (AWPT_LINE_LIST, sizeof(FPVertexColor1),
		//        0, (uint8 *)vertexArray, 0, numVertices,
		//        0, indexArray,  0, numIndices);
		for ( i = 1; i < numIndices; i++ )
		{
			VECTOR2 vC = *(VECTOR2*)&vertexArray[indexArray[i]].vertex;
			vC -= vWCenter;
			vC *= fScale;
			vC += vSCenter;

			VECTOR2 vP = *(VECTOR2*)&vertexArray[indexArray[i-1]].vertex;
			vP -= vWCenter;
			vP *= fScale;
			vP += vSCenter;
			e_PrimitiveAddLine( dwLineFlags, &vP, &vC, dwLineColor, fDepth );
		}
	}

	/******************/
	/* Reset counters */
	/******************/
	numIndices = 0;
	numVertices = 0;

	/**********************************/
	/* Build draw array for traingles */
	/**********************************/
	if (mNumFlyPoints > 0)
	{
		for (i=0; i<mNumFlyPoints; i++)
		{
			MATRIX matrix;
			QuatToMatrix(&(mOrientation[i]), matrix);
			FPColorDWordRGBA color;

			if (i == mCurrentEditPoint)
			{
				color.r = 255;
				color.g = 255;
				color.b = 0;
				color.a = 0;
			}
			else if (i == 0)
			{
				color.r = 0;
				color.g = 255;
				color.b = 0;
				color.a = 0;
			}
			else if (i == mNumFlyPoints-1)
			{
				color.r = 255;
				color.g = 0;
				color.b = 0;
				color.a = 0;
			}
			else
			{
				color.r = 0;
				color.g = 0;
				color.b = 255;
				color.a = 0;
			}


			vertexArray[numVertices].vertex.x = mLocation[i].x + ((matrix[0] + matrix[1] + matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((matrix[4] + matrix[5] + matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((matrix[8] + matrix[9] + matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((matrix[0] - matrix[1] + matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((matrix[4] - matrix[5] + matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((matrix[8] - matrix[9] + matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((-matrix[0] - matrix[1] + matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((-matrix[4] - matrix[5] + matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((-matrix[8] - matrix[9] + matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((-matrix[0] + matrix[1] + matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((-matrix[4] + matrix[5] + matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((-matrix[8] + matrix[9] + matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((matrix[0] + matrix[1] - matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((matrix[4] + matrix[5] - matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((matrix[8] + matrix[9] - matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((matrix[0] - matrix[1] - matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((matrix[4] - matrix[5] - matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((matrix[8] - matrix[9] - matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((-matrix[0] - matrix[1] - matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((-matrix[4] - matrix[5] - matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((-matrix[8] - matrix[9] - matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			vertexArray[numVertices].vertex.x = mLocation[i].x + ((-matrix[0] + matrix[1] - matrix[2] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.y = mLocation[i].y + ((-matrix[4] + matrix[5] - matrix[6] )*FLY_CUBE_RADIUS);
			vertexArray[numVertices].vertex.z = mLocation[i].z + ((-matrix[8] + matrix[9] - matrix[10])*FLY_CUBE_RADIUS);
			vertexArray[numVertices].color1.color = color.color;
			numVertices++;

			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-6;
			indexArray[numIndices++] = numVertices-7;
			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-5;
			indexArray[numIndices++] = numVertices-6;

			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-3;
			indexArray[numIndices++] = numVertices-4;
			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-7;
			indexArray[numIndices++] = numVertices-3;

			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-4;
			indexArray[numIndices++] = numVertices-1;
			indexArray[numIndices++] = numVertices-8;
			indexArray[numIndices++] = numVertices-1;
			indexArray[numIndices++] = numVertices-5;

			indexArray[numIndices++] = numVertices-7;
			indexArray[numIndices++] = numVertices-2;
			indexArray[numIndices++] = numVertices-3;
			indexArray[numIndices++] = numVertices-7;
			indexArray[numIndices++] = numVertices-6;
			indexArray[numIndices++] = numVertices-2;

			indexArray[numIndices++] = numVertices-5;
			indexArray[numIndices++] = numVertices-2;
			indexArray[numIndices++] = numVertices-6;
			indexArray[numIndices++] = numVertices-5;
			indexArray[numIndices++] = numVertices-1;
			indexArray[numIndices++] = numVertices-2;

			indexArray[numIndices++] = numVertices-4;
			indexArray[numIndices++] = numVertices-2;
			indexArray[numIndices++] = numVertices-1;
			indexArray[numIndices++] = numVertices-4;
			indexArray[numIndices++] = numVertices-3;
			indexArray[numIndices++] = numVertices-2;
		}

		/**********************************************/
		/* Draw: An example draw call from our engine */
		/**********************************************/
		//AwDraw (AWPT_TRIANGLE_LIST, sizeof(FPVertexColor1),
		//        0, (uint8 *)vertexArray, 0, numVertices,
		//        0, indexArray,  0, numIndices);
	}

	/***************/
	/* Free memory */
	/***************/
	FPFreeArray(vertexArray, FPVertexColor1);
	FPFreeArray(indexArray, uint16);

	mInFlight    = inFlight;
	mStartTime   = startTime;
	mPausedTime  = pausedTime;
	mPaused      = paused;

	return TRUE;
}

//=============================================================================
void FlyPath::SelectNearestFlyPoint (FPVertex3 aPos)
{
	int32 i;
	float32 tmpf;
	float32 dist = 999999999999.0f;

	/**********************/
	/* Find closest point */
	/**********************/
	mCurrentEditPoint = -1;
	for (i=0; i<mNumFlyPoints; i++)
	{
		tmpf = ((mLocation[i].x - aPos.x) * (mLocation[i].x - aPos.x)) +
			((mLocation[i].y - aPos.y) * (mLocation[i].y - aPos.y)) +
			((mLocation[i].z - aPos.z) * (mLocation[i].z - aPos.z));

		if ((tmpf < dist) || (i == 0))
		{
			dist = tmpf;
			mCurrentEditPoint = i;
		}
	}

	if (mPaused == TRUE)
	{
		int32 point;
		float32 time;

		GetCurrentFlyPointInfo(&point, &time);
		mPausedTime = mStartTime + time;
	}
}

//=============================================================================
bool8 FlyPath::GetCurrentFlyPoint (FPVertex3 *aPosition, Quaternion *aOrientation)
{
	if (mCurrentEditPoint < 0)
		return FALSE;

	aPosition->x = mLocation[mCurrentEditPoint].x;
	aPosition->y = mLocation[mCurrentEditPoint].y;
	aPosition->z = mLocation[mCurrentEditPoint].z;

	aOrientation->x = mOrientation[mCurrentEditPoint].x;
	aOrientation->y = mOrientation[mCurrentEditPoint].y;
	aOrientation->z = mOrientation[mCurrentEditPoint].z;
	aOrientation->w = mOrientation[mCurrentEditPoint].w;

	return TRUE;
}

//=============================================================================
bool8 FlyPath::GetCurrentFlyPointInfo(int32 *aPointNumber, float32 *aPointTime)
{
	int32 i;

	if (mCurrentEditPoint < 0)
		return FALSE;

	*aPointNumber = mCurrentEditPoint;
	*aPointTime = 0.0f;
	for (i=0; i<mCurrentEditPoint; i++)
	{
		if (mLocation[i].w == 0.0)
			*aPointTime += (float32)mSampleInterval;
	}

	return TRUE;
}

//=============================================================================
void FlyPath::ClearFlyPointSelection (void)
{
	if (mNumFlyPoints == 0)
		mCurrentEditPoint = -1;
	else
		mCurrentEditPoint = 0;
}

//=============================================================================
void FlyPath::SetNextPath (FlyPath *aPath)
{
	if (aPath != NULL)
		mNextPath = aPath;
	else
		mNextPath = this;
}

//=============================================================================
void FlyPath::SetPreviousPath (FlyPath *aPath)
{
	if (aPath != NULL)
		mPreviousPath = aPath;
	else
		mPreviousPath = this;
}

//=============================================================================
FlyPath* FlyPath::GetNextPath (void)
{
	return mNextPath;
}

//=============================================================================
FlyPath* FlyPath::GetPreviousPath (void)
{
	return mPreviousPath;
}

//=============================================================================
bool8 FlyPath::CompilePositionsAndOrientations (void)
{
	if (CompilePositions() == FALSE)
		return FALSE;

	if (CompileOrientations() == FALSE)
		return FALSE;

	return TRUE;
}

//=============================================================================
float32 FPDistance (FPVertex4 *v1, FPVertex4 *v2)
{
	FPVertex4 tmpv;

	tmpv.x = v1->x - v2->x;
	tmpv.y = v1->y - v2->y;
	tmpv.z = v1->z - v2->z;

	return (float32)(sqrt((tmpv.x*tmpv.x) + (tmpv.y*tmpv.y) + (tmpv.z*tmpv.z)));
}

//=============================================================================
void FPVertex4Lerp (FPVertex4 *ret, FPVertex4 *v1, FPVertex4 *v2, float32 lerpValue)
{
	ret->x = v1->x + lerpValue*(v2->x - v1->x);
	ret->y = v1->y + lerpValue*(v2->y - v1->y);
	ret->z = v1->z + lerpValue*(v2->z - v1->z);
}

//=============================================================================
void FPVertex4Copy (FPVertex4 *v1, FPVertex4 *v2)
{
	v1->x = v2->x;
	v1->y = v2->y;
	v1->z = v2->z;
}

//=============================================================================
bool8 FlyPath::CompilePositions (void)
{
	int32 i;
	int32 j;
	int32 k;
	float32 tmpf;
	float32 tmpf2;
	int32 numPoints = 0;
	FPVertex4 *pointArray = NULL;
	FPVertex4 *tmpD2 = NULL;
	FPVertex4 *tmpIntD2 = NULL;

	/***********************************/
	/* Make sure we have enough points */
	/***********************************/
	if (mNumFlyPoints < 5)
		return FALSE;

	/********************************************/
	/* Allocate new memory for spline intervals */
	/********************************************/
	FPFreeArray(mLocationIntervals, FPSplineInterval);
	FPMallocArray(mLocationIntervals, mNumFlyPoints, FPSplineInterval);

	/*****************************************/
	/* Allocate memory for temporary storage */
	/*****************************************/
	FPMallocArray(pointArray, mNumFlyPoints+9, FPVertex4);
	FPMallocArray(tmpD2,      mNumFlyPoints+9, FPVertex4);
	FPMallocArray(tmpIntD2,   mNumFlyPoints+9, FPVertex4);

	/***************************************************************************/
	/* Build each array of vertices padding by 1 point on each end and process */
	/***************************************************************************/
	mPathTime = 0.0;
	mNumSplineIntervals = 0;
	for (i=0; i<mNumFlyPoints; i++)
	{
		/*******************/
		/* Skip cut points */
		/*******************/
		if (mLocation[i].w != 0.0)
			continue;

		/****************/
		/* Init counter */
		/****************/
		numPoints = 0;

		/**************************/
		/* Pad spline by 4 points */
		/**************************/
		if ((i == 0) && (mPreviousPath->mNumFlyPoints >= 5) && (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-1].w == 0.0))
		{
			FPVertex4Copy (&(pointArray[numPoints+3]), &(mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-1]));
			if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-2].w == 0.0)
			{
				FPVertex4Copy (&(pointArray[numPoints+2]), &(mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-2]));
				if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-3].w == 0.0)
				{
					FPVertex4Copy (&(pointArray[numPoints+1]), &(mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-3]));
					if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-4].w == 0.0)
					{
						FPVertex4Copy (&(pointArray[numPoints+0]), &(mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-4]));
					}
					else
					{
						FPVertex4Lerp (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
					}
				}
				else
				{
					FPVertex4Lerp (&(pointArray[numPoints+1]), &(pointArray[numPoints+2]), &(pointArray[numPoints+3]), -1.0f);
					FPVertex4Lerp (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
				}
			}
			else
			{
				FPVertex4Lerp (&(pointArray[numPoints+2]), &(pointArray[numPoints+3]), &(mLocation[i]), -1.0f);
				FPVertex4Lerp (&(pointArray[numPoints+1]), &(pointArray[numPoints+2]), &(pointArray[numPoints+3]), -1.0f);
				FPVertex4Lerp (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
			}
		}
		else //Not first point on spline, just create phantom points
		{
			if (i < mNumFlyPoints-1) //If not last point on spline
			{
				FPVertex4Lerp (&(pointArray[numPoints+3]), &(mLocation[i]), &(mLocation[i+1]), -1.0f);
				FPVertex4Lerp (&(pointArray[numPoints+2]), &(mLocation[i]), &(mLocation[i+1]), -2.0f);
				FPVertex4Lerp (&(pointArray[numPoints+1]), &(mLocation[i]), &(mLocation[i+1]), -3.0f);
				FPVertex4Lerp (&(pointArray[numPoints+0]), &(mLocation[i]), &(mLocation[i+1]), -4.0f);
			}
			else
			{
				if (mNextPath->mNumFlyPoints >= 5)
				{
					FPVertex4Lerp (&(pointArray[numPoints+3]), &(mLocation[i]), &(mNextPath->mLocation[0]), -1.0f);
					FPVertex4Lerp (&(pointArray[numPoints+2]), &(mLocation[i]), &(mNextPath->mLocation[0]), -2.0f);
					FPVertex4Lerp (&(pointArray[numPoints+1]), &(mLocation[i]), &(mNextPath->mLocation[0]), -3.0f);
					FPVertex4Lerp (&(pointArray[numPoints+0]), &(mLocation[i]), &(mNextPath->mLocation[0]), -4.0f);
				}
				else //No next point...something's wrong
				{
					FPVertex4Copy (&(pointArray[numPoints+3]), &(mLocation[i]));
					FPVertex4Copy (&(pointArray[numPoints+2]), &(mLocation[i]));
					FPVertex4Copy (&(pointArray[numPoints+1]), &(mLocation[i]));
					FPVertex4Copy (&(pointArray[numPoints+0]), &(mLocation[i]));
				}
			}
		}
		numPoints += 4;

		/*****************************************/
		/* Add all points that aren't cut points */
		/*****************************************/
		for (; i<mNumFlyPoints; i++)
		{
			/******************/
			/* Add this point */
			/******************/
			FPVertex4Copy (&(pointArray[numPoints]), &(mLocation[i]));
			numPoints++;

			/**********************/
			/* Break if cut point */
			/**********************/
			if (mLocation[i].w != 0.0)
				break;
		}

		/*************************************************/
		/* Add last 4 pad points by either getting them  */
		/* from the next path or creating phantom points */
		/*************************************************/
		if (((i < mNumFlyPoints) && (mLocation[i].w != 0.0)) || ((i >= mNumFlyPoints) && (mNextPath->mNumFlyPoints < 5))) //If cut point OR next path isn't valid
		{
			FPVertex4Lerp (&(pointArray[numPoints+0]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 2.0f);
			FPVertex4Lerp (&(pointArray[numPoints+1]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 3.0f);
			FPVertex4Lerp (&(pointArray[numPoints+2]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 4.0f);
			FPVertex4Lerp (&(pointArray[numPoints+3]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 5.0f);
			numPoints += 4;
		}
		else //Look at next path
		{
			/********************************************************************/
			/* This is the last point and smoothly feeding into the next spline */
			/********************************************************************/
			//Add first point to close the spline
			FPVertex4Copy (&(pointArray[numPoints]), &(mNextPath->mLocation[0]));
			numPoints++;

			//Pad with next point
			FPVertex4Copy (&(pointArray[numPoints+0]), &(mNextPath->mLocation[1]));
			if (mNextPath->mLocation[1].w == 0.0f)
			{
				FPVertex4Copy (&(pointArray[numPoints+1]), &(mNextPath->mLocation[2]));
				if (mNextPath->mLocation[2].w == 0.0f)
				{
					FPVertex4Copy (&(pointArray[numPoints+2]), &(mNextPath->mLocation[3]));
					if (mNextPath->mLocation[3].w == 0.0f)
					{
						FPVertex4Copy (&(pointArray[numPoints+3]), &(mNextPath->mLocation[4]));
					}
					else
					{
						FPVertex4Lerp (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
					}
				}
				else
				{
					FPVertex4Lerp (&(pointArray[numPoints+2]), &(pointArray[numPoints+0]), &(pointArray[numPoints+1]), 2.0f);
					FPVertex4Lerp (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
				}
			}
			else
			{
				FPVertex4Lerp (&(pointArray[numPoints+1]), &(pointArray[numPoints-1]), &(pointArray[numPoints+0]), 2.0f);
				FPVertex4Lerp (&(pointArray[numPoints+2]), &(pointArray[numPoints+0]), &(pointArray[numPoints+1]), 2.0f);
				FPVertex4Lerp (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
			}
			numPoints += 4;
		}

		/********************************************************/
		/* Create spline interval variables based on pointArray */
		/********************************************************/
		tmpD2[0].x = 0.0f;
		tmpD2[0].y = 0.0f;
		tmpD2[0].z = 0.0f;
		tmpD2[0].w = 0.0f;
		tmpIntD2[0].x = 0.0f;
		tmpIntD2[0].y = 0.0f;
		tmpIntD2[0].z = 0.0f;
		tmpIntD2[0].w = 0.0f;

		//Calculate second derivative for all points except first and last
		for (j=1; j<numPoints-1; j++) //For each point
		{
			for (k=0; k<4; k++) //For x, y, and z
			{
				float32 time  = 0.5f;
				float32 time2 = 1.0f;

				tmpf = (time * tmpD2[j-1].v[k]) + 2.0f;
				tmpD2[j].v[k] = (time - 1.0f) / tmpf;

				tmpf2 = ((pointArray[j+1].v[k] - pointArray[j].v[k]) / (time2)) - 
					((pointArray[j].v[k] - pointArray[j-1].v[k]) / (time2));

				tmpIntD2[j].v[k] = ((6.0f * tmpf2) / (time2+time2)) - (time * tmpIntD2[j-1].v[k]);
				tmpIntD2[j].v[k] /= tmpf;
			}
		}

		tmpD2[numPoints-1].x = 0.0f;
		tmpD2[numPoints-1].y = 0.0f;
		tmpD2[numPoints-1].z = 0.0f;
		tmpD2[numPoints-1].w = 0.0f;
		tmpIntD2[numPoints-1].x = 0.0f;
		tmpIntD2[numPoints-1].y = 0.0f;
		tmpIntD2[numPoints-1].z = 0.0f;
		tmpIntD2[numPoints-1].w = 0.0f;

		for (j=numPoints-2; j>=1; j--)
		{
			tmpD2[j].x = (tmpD2[j].x * tmpD2[j+1].x) + tmpIntD2[j].x;
			tmpD2[j].y = (tmpD2[j].y * tmpD2[j+1].y) + tmpIntD2[j].y;
			tmpD2[j].z = (tmpD2[j].z * tmpD2[j+1].z) + tmpIntD2[j].z;
			tmpD2[j].w = (tmpD2[j].w * tmpD2[j+1].w) + tmpIntD2[j].w;
		}

		for (j=4; j<numPoints-5; j++)
		{
			mLocationIntervals[mNumSplineIntervals].lo.x = pointArray[j].x;
			mLocationIntervals[mNumSplineIntervals].lo.y = pointArray[j].y;
			mLocationIntervals[mNumSplineIntervals].lo.z = pointArray[j].z;
			mLocationIntervals[mNumSplineIntervals].lo.w = pointArray[j].w;

			mLocationIntervals[mNumSplineIntervals].hi.x = pointArray[j+1].x;
			mLocationIntervals[mNumSplineIntervals].hi.y = pointArray[j+1].y;
			mLocationIntervals[mNumSplineIntervals].hi.z = pointArray[j+1].z;
			mLocationIntervals[mNumSplineIntervals].hi.w = pointArray[j+1].w;

			mLocationIntervals[mNumSplineIntervals].lo2.x = tmpD2[j].x;
			mLocationIntervals[mNumSplineIntervals].lo2.y = tmpD2[j].y;
			mLocationIntervals[mNumSplineIntervals].lo2.z = tmpD2[j].z;
			mLocationIntervals[mNumSplineIntervals].lo2.w = tmpD2[j].w;

			mLocationIntervals[mNumSplineIntervals].hi2.x = tmpD2[j+1].x;
			mLocationIntervals[mNumSplineIntervals].hi2.y = tmpD2[j+1].y;
			mLocationIntervals[mNumSplineIntervals].hi2.z = tmpD2[j+1].z;
			mLocationIntervals[mNumSplineIntervals].hi2.w = tmpD2[j+1].w;

			/********************/
			/* Update variables */
			/********************/
			mNumSplineIntervals++;
			mPathTime += mSampleInterval;
		}
	}

	/*********************************/
	/* Free temporary storage memory */
	/*********************************/
	FPFreeArray(pointArray, FPVertex4);
	FPFreeArray(tmpD2, FPVertex4);
	FPFreeArray(tmpIntD2, FPVertex4);

	return TRUE;
}

//=============================================================================
bool8 FlyPath::GetPathPosition (FPVertex3 *aPosition, float64 aTime)
{
	int32 tmpi;
	int32 interval;
	float32 intervalOffset;
	float64 tmpf64;
	float32 a;
	float32 b;
	float64 pathTime;

	if (mInFlight == FALSE)
		return FALSE;

	/******************************/
	/* Take care of loop wrapping */
	/******************************/
	pathTime = GetTotalPathTime();
	tmpf64 = aTime - mStartTime;
	if (tmpf64 >= pathTime)
	{
		if ( m_bWrap )
		{
			tmpi = (int32)(tmpf64/pathTime);
			tmpf64 -= pathTime * (float64)(tmpi);
		}
		else
		{
			tmpf64 = pathTime;
		}
	}

	/**********************************************************************************/
	/* Figure out which interval this time falls into and calculate normalized offset */
	/**********************************************************************************/
	interval = (int32)(tmpf64/mSampleInterval);
	intervalOffset = (float32)((tmpf64 - (mSampleInterval*(float64)(interval))) / mSampleInterval);

	a = 1.0f - intervalOffset;
	b = intervalOffset;

	aPosition->x = a*mLocationIntervals[interval].lo.x + b*mLocationIntervals[interval].hi.x + ((a*a*a - a)*mLocationIntervals[interval].lo2.x + (b*b*b - b)*mLocationIntervals[interval].hi2.x) / 6.0f;// * (float32)(mSampleInterval*mSampleInterval) / 6.0f;
	aPosition->y = a*mLocationIntervals[interval].lo.y + b*mLocationIntervals[interval].hi.y + ((a*a*a - a)*mLocationIntervals[interval].lo2.y + (b*b*b - b)*mLocationIntervals[interval].hi2.y) / 6.0f;// * (float32)(mSampleInterval*mSampleInterval) / 6.0f;
	aPosition->z = a*mLocationIntervals[interval].lo.z + b*mLocationIntervals[interval].hi.z + ((a*a*a - a)*mLocationIntervals[interval].lo2.z + (b*b*b - b)*mLocationIntervals[interval].hi2.z) / 6.0f;// * (float32)(mSampleInterval*mSampleInterval) / 6.0f;

	return TRUE;
}

//=============================================================================
bool8 FlyPath::GetPathInterval (int32 & aInterval, float64 aTime)
{
	int32 tmpi;
	int32 interval;
	float32 intervalOffset;
	float64 tmpf64;
	//float32 a;
	//float32 b;
	float64 pathTime;

	if (mInFlight == FALSE)
		return FALSE;

	/******************************/
	/* Take care of loop wrapping */
	/******************************/
	pathTime = GetTotalPathTime();
	tmpf64 = aTime - mStartTime;
	if (tmpf64 >= pathTime)
	{
		if ( m_bWrap )
		{
			tmpi = (int32)(tmpf64/pathTime);
			tmpf64 -= pathTime * (float64)(tmpi);
		}
		else
		{
			tmpf64 = pathTime;
		}
	}

	/**********************************************************************************/
	/* Figure out which interval this time falls into and calculate normalized offset */
	/**********************************************************************************/
	interval = (int32)(tmpf64/mSampleInterval);
	intervalOffset = (float32)((tmpf64 - (mSampleInterval*(float64)(interval))) / mSampleInterval);

	aInterval = interval;

	return TRUE;
}

//=============================================================================
bool8 FlyPath::CompileOrientations (void)
{
	int32 i;
	int32 j;
	int32 k;
	float32 tmpf;
	float32 tmpf2;
	int32 numPoints = 0;
	Quaternion *pointArray = NULL;
	Quaternion *tmpD2 = NULL;
	Quaternion *tmpIntD2 = NULL;

	/***********************************/
	/* Make sure we have enough points */
	/***********************************/
	if (mNumFlyPoints < 5)
		return FALSE;

	/********************************************/
	/* Allocate new memory for spline intervals */
	/********************************************/
	FPFreeArray(mOrientationIntervals, FPSplineInterval);
	FPMallocArray(mOrientationIntervals, mNumFlyPoints, FPSplineInterval);

	/*****************************************/
	/* Allocate memory for temporary storage */
	/*****************************************/
	FPMallocArray(pointArray, mNumFlyPoints+9, Quaternion);
	FPMallocArray(tmpD2,      mNumFlyPoints+9, Quaternion);
	FPMallocArray(tmpIntD2,   mNumFlyPoints+9, Quaternion);

	/***************************************************************************/
	/* Build each array of vertices padding by 1 point on each end and process */
	/***************************************************************************/
	mPathTime = 0.0;
	mNumSplineIntervals = 0;
	for (i=0; i<mNumFlyPoints; i++)
	{
		/*******************/
		/* Skip cut points */
		/*******************/
		if (mLocation[i].w != 0.0)
			continue;

		/****************/
		/* Init counter */
		/****************/
		numPoints = 0;

		/**************************/
		/* Pad spline by 4 points */
		/**************************/
		if ((i == 0) && (mPreviousPath->mNumFlyPoints >= 5) && (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-1].w == 0.0))
		{
			QuatCopy (&(pointArray[numPoints+3]), &(mPreviousPath->mOrientation[mPreviousPath->mNumFlyPoints-1]));
			if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-2].w == 0.0)
			{
				QuatCopy (&(pointArray[numPoints+2]), &(mPreviousPath->mOrientation[mPreviousPath->mNumFlyPoints-2]));
				if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-3].w == 0.0)
				{
					QuatCopy (&(pointArray[numPoints+1]), &(mPreviousPath->mOrientation[mPreviousPath->mNumFlyPoints-3]));
					if (mPreviousPath->mLocation[mPreviousPath->mNumFlyPoints-4].w == 0.0)
					{
						QuatCopy (&(pointArray[numPoints+0]), &(mPreviousPath->mOrientation[mPreviousPath->mNumFlyPoints-4]));
					}
					else
					{
						QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
					}
				}
				else
				{
					QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(pointArray[numPoints+2]), &(pointArray[numPoints+3]), -1.0f);
					QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
				}
			}
			else
			{
				QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(pointArray[numPoints+3]), &(mOrientation[i]), -1.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(pointArray[numPoints+2]), &(pointArray[numPoints+3]), -1.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), -1.0f);
			}
		}
		else //Not first point on spline, just create phantom points
		{
			if (i < mNumFlyPoints-1) //If not last point on spline
			{
				QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(mOrientation[i]), &(mOrientation[i+1]), -1.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(mOrientation[i]), &(mOrientation[i+1]), -2.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(mOrientation[i]), &(mOrientation[i+1]), -3.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(mOrientation[i]), &(mOrientation[i+1]), -4.0f);
			}
			else
			{
				if (mNextPath->mNumFlyPoints >= 5)
				{
					QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(mOrientation[i]), &(mNextPath->mOrientation[0]), -1.0f);
					QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(mOrientation[i]), &(mNextPath->mOrientation[0]), -2.0f);
					QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(mOrientation[i]), &(mNextPath->mOrientation[0]), -3.0f);
					QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(mOrientation[i]), &(mNextPath->mOrientation[0]), -4.0f);
				}
				else //No next point...something's wrong
				{
					QuatCopy (&(pointArray[numPoints+3]), &(mOrientation[i]));
					QuatCopy (&(pointArray[numPoints+2]), &(mOrientation[i]));
					QuatCopy (&(pointArray[numPoints+1]), &(mOrientation[i]));
					QuatCopy (&(pointArray[numPoints+0]), &(mOrientation[i]));
				}
			}
		}
		numPoints += 4;

		/*****************************************/
		/* Add all points that aren't cut points */
		/*****************************************/
		for (; i<mNumFlyPoints; i++)
		{
			/******************/
			/* Add this point */
			/******************/
			QuatCopy (&(pointArray[numPoints]), &(mOrientation[i]));
			numPoints++;

			/**********************/
			/* Break if cut point */
			/**********************/
			if (mLocation[i].w != 0.0)
				break;
		}

		/*************************************************/
		/* Add last 4 pad points by either getting them  */
		/* from the next path or creating phantom points */
		/*************************************************/
		if (((i < mNumFlyPoints) && (mLocation[i].w != 0.0)) || ((i >= mNumFlyPoints) && (mNextPath->mNumFlyPoints < 5))) //If cut point OR next path isn't valid
		{
			QuatSlerpShortestPath (&(pointArray[numPoints+0]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 2.0f);
			QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 3.0f);
			QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 4.0f);
			QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(pointArray[numPoints-2]), &(pointArray[numPoints-1]), 5.0f);
			numPoints += 4;
		}
		else //Look at next path
		{
			/********************************************************************/
			/* This is the last point and smoothly feeding into the next spline */
			/********************************************************************/
			//Add first point to close the spline
			QuatCopy (&(pointArray[numPoints]), &(mNextPath->mOrientation[0]));
			numPoints++;

			//Pad with next point
			QuatCopy (&(pointArray[numPoints+0]), &(mNextPath->mOrientation[1]));
			if (mNextPath->mLocation[1].w == 0.0f)
			{
				QuatCopy (&(pointArray[numPoints+1]), &(mNextPath->mOrientation[2]));
				if (mNextPath->mLocation[2].w == 0.0f)
				{
					QuatCopy (&(pointArray[numPoints+2]), &(mNextPath->mOrientation[3]));
					if (mNextPath->mLocation[3].w == 0.0f)
					{
						QuatCopy (&(pointArray[numPoints+3]), &(mNextPath->mOrientation[4]));
					}
					else
					{
						QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
					}
				}
				else
				{
					QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(pointArray[numPoints+0]), &(pointArray[numPoints+1]), 2.0f);
					QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
				}
			}
			else
			{
				QuatSlerpShortestPath (&(pointArray[numPoints+1]), &(pointArray[numPoints-1]), &(pointArray[numPoints+0]), 2.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+2]), &(pointArray[numPoints+0]), &(pointArray[numPoints+1]), 2.0f);
				QuatSlerpShortestPath (&(pointArray[numPoints+3]), &(pointArray[numPoints+1]), &(pointArray[numPoints+2]), 2.0f);
			}
			numPoints += 4;
		}

		/*************************************************/
		/* Order quarternions for shortest path rotation */
		/*************************************************/
		for (j=0; j<(numPoints-1); j++)
		{
			if (QuatDot(&(pointArray[j]), &(pointArray[j+1])) < 0.0f)
			{
				pointArray[j+1].x = -pointArray[j+1].x;
				pointArray[j+1].y = -pointArray[j+1].y;
				pointArray[j+1].z = -pointArray[j+1].z;
				pointArray[j+1].w = -pointArray[j+1].w;
			}
		}

		/************************************************/
		/* Move quaternions from S3 space into R4 space */
		/************************************************/
		for (j=0; j<numPoints; j++)
		{
			tmpf = 1.0f / (float32)sqrt(2.0*(1.0 - pointArray[j].w));
			pointArray[j].x *= tmpf;
			pointArray[j].y *= tmpf;
			pointArray[j].z *= tmpf;
			pointArray[j].w = (1.0f - pointArray[j].w) * tmpf;
		}

		/********************************************************/
		/* Create spline interval variables based on pointArray */
		/********************************************************/
		tmpD2[0].x = 0.0f;
		tmpD2[0].y = 0.0f;
		tmpD2[0].z = 0.0f;
		tmpD2[0].w = 0.0f;
		tmpIntD2[0].x = 0.0f;
		tmpIntD2[0].y = 0.0f;
		tmpIntD2[0].z = 0.0f;
		tmpIntD2[0].w = 0.0f;

		//Calculate second derivative for all points except first and last
		for (j=1; j<numPoints-1; j++) //For each point
		{
			for (k=0; k<4; k++) //For x, y, and z
			{
				float32 time  = 0.5f;
				float32 time2 = 1.0f;

				tmpf = (time * tmpD2[j-1].v[k]) + 2.0f;
				tmpD2[j].v[k] = (time - 1.0f) / tmpf;

				tmpf2 = ((pointArray[j+1].v[k] - pointArray[j].v[k]) / (time2)) - 
					((pointArray[j].v[k] - pointArray[j-1].v[k]) / (time2));

				tmpIntD2[j].v[k] = ((6.0f * tmpf2) / (time2+time2)) - (time * tmpIntD2[j-1].v[k]);
				tmpIntD2[j].v[k] /= tmpf;
			}
		}

		tmpD2[numPoints-1].x = 0.0f;
		tmpD2[numPoints-1].y = 0.0f;
		tmpD2[numPoints-1].z = 0.0f;
		tmpD2[numPoints-1].w = 0.0f;
		tmpIntD2[numPoints-1].x = 0.0f;
		tmpIntD2[numPoints-1].y = 0.0f;
		tmpIntD2[numPoints-1].z = 0.0f;
		tmpIntD2[numPoints-1].w = 0.0f;

		for (j=numPoints-2; j>=1; j--)
		{
			tmpD2[j].x = (tmpD2[j].x * tmpD2[j+1].x) + tmpIntD2[j].x;
			tmpD2[j].y = (tmpD2[j].y * tmpD2[j+1].y) + tmpIntD2[j].y;
			tmpD2[j].z = (tmpD2[j].z * tmpD2[j+1].z) + tmpIntD2[j].z;
			tmpD2[j].w = (tmpD2[j].w * tmpD2[j+1].w) + tmpIntD2[j].w;
		}

		for (j=4; j<numPoints-5; j++)
		{
			mOrientationIntervals[mNumSplineIntervals].lo.x = pointArray[j].x;
			mOrientationIntervals[mNumSplineIntervals].lo.y = pointArray[j].y;
			mOrientationIntervals[mNumSplineIntervals].lo.z = pointArray[j].z;
			mOrientationIntervals[mNumSplineIntervals].lo.w = pointArray[j].w;

			mOrientationIntervals[mNumSplineIntervals].hi.x = pointArray[j+1].x;
			mOrientationIntervals[mNumSplineIntervals].hi.y = pointArray[j+1].y;
			mOrientationIntervals[mNumSplineIntervals].hi.z = pointArray[j+1].z;
			mOrientationIntervals[mNumSplineIntervals].hi.w = pointArray[j+1].w;

			mOrientationIntervals[mNumSplineIntervals].lo2.x = tmpD2[j].x;
			mOrientationIntervals[mNumSplineIntervals].lo2.y = tmpD2[j].y;
			mOrientationIntervals[mNumSplineIntervals].lo2.z = tmpD2[j].z;
			mOrientationIntervals[mNumSplineIntervals].lo2.w = tmpD2[j].w;

			mOrientationIntervals[mNumSplineIntervals].hi2.x = tmpD2[j+1].x;
			mOrientationIntervals[mNumSplineIntervals].hi2.y = tmpD2[j+1].y;
			mOrientationIntervals[mNumSplineIntervals].hi2.z = tmpD2[j+1].z;
			mOrientationIntervals[mNumSplineIntervals].hi2.w = tmpD2[j+1].w;

			/********************/
			/* Update variables */
			/********************/
			mNumSplineIntervals++;
			mPathTime += mSampleInterval;
		}
	}

	/*********************************/
	/* Free temporary storage memory */
	/*********************************/
	FPFreeArray(pointArray, Quaternion);
	FPFreeArray(tmpD2, Quaternion);
	FPFreeArray(tmpIntD2, Quaternion);

	return TRUE;
}

//=============================================================================
bool8 FlyPath::GetPathOrientation (Quaternion *aOrientation, float64 aTime)
{
	int32 tmpi;
	int32 interval;
	float32 intervalOffset;
	float64 tmpf64;
	float32 a;
	float32 b;
	float32 tmpf;
	Quaternion tmpQuat;
	float64 pathTime;

	if (mInFlight == FALSE)
		return FALSE;

	/******************************/
	/* Take care of loop wrapping */
	/******************************/
	pathTime = GetTotalPathTime();
	tmpf64 = aTime - mStartTime;
	if (tmpf64 >= pathTime)
	{
		if ( m_bWrap )
		{
			tmpi = (int32)(tmpf64/pathTime);
			tmpf64 -= pathTime * (float64)(tmpi);
		}
		else
		{
			tmpf64 = pathTime;
		}
	}

	/**********************************************************************************/
	/* Figure out which interval this time falls into and calculate normalized offset */
	/**********************************************************************************/
	interval = (int32)(tmpf64/mSampleInterval);
	intervalOffset = (float32)((tmpf64 - (mSampleInterval*(float64)(interval))) / mSampleInterval);

	a = 1.0f - intervalOffset;
	b = intervalOffset;

	tmpQuat.x = a*mOrientationIntervals[interval].lo.x + b*mOrientationIntervals[interval].hi.x + ((a*a*a - a)*mOrientationIntervals[interval].lo2.x + (b*b*b - b)*mOrientationIntervals[interval].hi2.x) / 6.0f;
	tmpQuat.y = a*mOrientationIntervals[interval].lo.y + b*mOrientationIntervals[interval].hi.y + ((a*a*a - a)*mOrientationIntervals[interval].lo2.y + (b*b*b - b)*mOrientationIntervals[interval].hi2.y) / 6.0f;
	tmpQuat.z = a*mOrientationIntervals[interval].lo.z + b*mOrientationIntervals[interval].hi.z + ((a*a*a - a)*mOrientationIntervals[interval].lo2.z + (b*b*b - b)*mOrientationIntervals[interval].hi2.z) / 6.0f;
	tmpQuat.w = a*mOrientationIntervals[interval].lo.w + b*mOrientationIntervals[interval].hi.w + ((a*a*a - a)*mOrientationIntervals[interval].lo2.w + (b*b*b - b)*mOrientationIntervals[interval].hi2.w) / 6.0f;

	//R4 space -> S3 space
	tmpf = 1.0f / ((tmpQuat.x*tmpQuat.x) + (tmpQuat.y*tmpQuat.y) + (tmpQuat.z*tmpQuat.z) + (tmpQuat.w*tmpQuat.w));
	aOrientation->x = (2.0f*tmpQuat.x*tmpQuat.w) * tmpf;
	aOrientation->y = (2.0f*tmpQuat.y*tmpQuat.w) * tmpf;
	aOrientation->z = (2.0f*tmpQuat.z*tmpQuat.w) * tmpf;
	aOrientation->w = ((tmpQuat.x*tmpQuat.x) + (tmpQuat.y*tmpQuat.y) + (tmpQuat.z*tmpQuat.z) - (tmpQuat.w*tmpQuat.w)) * tmpf;

	QuatNormalize(aOrientation);

	return TRUE;
}

bool8 FlyPath::CompileConstantSpeedSpline(void)
{
	/***********************************/
	/* Make sure we have enough points */
	/***********************************/
	if (mNumFlyPoints < 5)
		return FALSE;

	/**********************************************/
	/* Allocate new memory for const speed spline */
	/**********************************************/
	FREE_DELETE( NULL, mptHSpline, HSpline<vector3d> );
	mptHSpline = MALLOC_NEW( NULL, HSpline<vector3d> );
	ASSERT_RETFALSE( mptHSpline );

	for ( int i = 0; i < mNumFlyPoints; ++i )
	{
		vector3d tPos = *(vector3d*)&mLocation[i];
		vector3d tTangent;
		mptHSpline->addPointAtEnd( tPos, tTangent );
	}

	mptHSpline->setupTimeConstant( 1000 );

	return TRUE;
}




}; // FSSE
