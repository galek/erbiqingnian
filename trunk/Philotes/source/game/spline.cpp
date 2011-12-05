//----------------------------------------------------------------------------
// spine.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "spline.h"
#include "astar.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void SplineSetKnotTable( int * nKnotTbl, int num_knots )
{
	ASSERT_RETURN( num_knots >= 2 && num_knots <= MAX_SPLINE_KNOTS );
	ASSERT_RETURN( nKnotTbl );

	int n = num_knots - 1;

	for ( int j = 0; j <= n + SPLINE_T; j++ )
	{
		if ( j < SPLINE_T )
		{
			nKnotTbl[j] = 0;
		}
		else if ( j <= n )
		{
			nKnotTbl[j] = j - SPLINE_T + 1;
		}
		else if ( j > n )
		{
			nKnotTbl[j] = n - SPLINE_T + 2;
		}
	}
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

void SplineGetPosition( VECTOR * vKnots, int num_knots, int * nKnotTbl, VECTOR * vOut, float delta )
{
	ASSERT_RETURN( num_knots >= 2 && num_knots <= MAX_SPLINE_KNOTS );
	ASSERT_RETURN( vKnots );
	ASSERT_RETURN( vOut );
	ASSERT_RETURN( nKnotTbl );
	ASSERT_RETURN( delta >= 0.0f && delta <= 1.0f );

	if ( delta == 0.0f )
	{
		*vOut = vKnots[0];
		return;
	}

	if ( delta == 1.0f )
	{
		*vOut = vKnots[num_knots-1];
		return;
	}

	float v = (float)( num_knots - SPLINE_T + 1 ) * delta;

	*vOut = VECTOR( 0.0f, 0.0f, 0.0f );

	for ( int k = 0; k < num_knots; k++ )
	{
		float blend = sSplineBlend( k, SPLINE_T, nKnotTbl, v );
		*vOut += vKnots[k] * blend;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sCalculateSplineDirection( VECTOR * vKnots, int num_knots, int current_knot, float time_delta, VECTOR * vOutDirection )
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
		if ( ( nk + 1 ) >= num_knots )
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
	SplineGetCatmullRomPosition( vKnots, num_knots, pk, pt, &vOldPos );
	SplineGetCatmullRomPosition( vKnots, num_knots, nk, nt, &vNextPos );
	VECTOR vDirection = vNextPos - vOldPos;
	*vOutDirection = vDirection;
}

//-------------------------------------------------------------------------------------------------
// NOTE: vOutDirection will NOT be normalized

void SplineGetCatmullRomPosition( VECTOR * vKnots, int num_knots, int current_knot, float time_delta, VECTOR * vOutPosition, VECTOR * vOutDirection )
{
	ASSERT_RETURN( num_knots >= 2 );
	ASSERT_RETURN( vKnots );
	ASSERT_RETURN( vOutPosition );
	ASSERT_RETURN( current_knot >= 0 && current_knot < num_knots );
	ASSERT_RETURN( time_delta >= 0.0f && time_delta <= 1.0f );

	if ( time_delta == 0.0f )
	{
		*vOutPosition = vKnots[current_knot];
		if ( vOutDirection )
		{
			sCalculateSplineDirection( vKnots, num_knots, current_knot, time_delta, vOutDirection );
		}
		return;
	}

	int previous_knot = current_knot - 1;
	if ( previous_knot < 0 ) previous_knot = 0;
	int next_knot = current_knot + 1;
	if ( next_knot >= num_knots ) next_knot = num_knots - 1;
	int last_knot = next_knot + 1;
	if ( last_knot >= num_knots ) last_knot = num_knots - 1;

	if ( time_delta == 1.0f )
	{
		*vOutPosition = vKnots[next_knot];
		if ( vOutDirection )
		{
			sCalculateSplineDirection( vKnots, num_knots, current_knot, time_delta, vOutDirection );
		}
		return;
	}

	VECTOR a = -1.0f * vKnots[previous_knot];
	a += 3.0f * vKnots[current_knot];
	a += -3.0f * vKnots[next_knot];
	a += vKnots[last_knot];
	a = a * time_delta * time_delta * time_delta;
	VECTOR b = 2.0f * vKnots[previous_knot ];
	b += -5.0f * vKnots[current_knot];
	b += 4.0f * vKnots[next_knot];
	b -= vKnots[last_knot];
	b = b * time_delta * time_delta;
	VECTOR c = vKnots[next_knot] - vKnots[previous_knot];
	c = c * time_delta;
	VECTOR d = 2.0f * vKnots[current_knot];
	VECTOR vPos = a + b + c + d;
	*vOutPosition = 0.5f * vPos;
	if ( vOutDirection )
	{
		sCalculateSplineDirection( vKnots, num_knots, current_knot, time_delta, vOutDirection );
	}
}

//----------------------------------------------------------------------------
// spline debugging
//----------------------------------------------------------------------------
#if (0)

#include "debugwindow.h"
#include "s_gdi.h"

static VECTOR sgvPath[MAX_SPLINE_KNOTS];
static int sgnNumPathKnots = 0;

#define TESTCURVE_SCALE		20.0f
#define Y_DELTA				-100

static void sDrawTestCurve()
{
	for ( int i = 0; i < sgnNumPathKnots; i++ )
	{
		int x1 = (int)FLOOR( ( sgvPath[i].x - sgvPath[0].x ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
		int y1 = (int)FLOOR( ( sgvPath[i].y - sgvPath[0].y ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
		gdi_drawbox( x1-1,y1-1,x1+1,y1+1, gdi_color_green );
	}

	VECTOR old = VECTOR( 0.0f, 0.0f, 0.0f );
	for ( int k = 0; k < sgnNumPathKnots; k++ )
	{
		for ( float t = 0.0f; t < 1.0f; t += 0.1f )
		{
			VECTOR newpos, newdir;
			SplineGetCatmullRomPosition( &sgvPath[0], sgnNumPathKnots, k, t, &newpos, &newdir );
			newpos -= sgvPath[0];

			int nX1 = (int)FLOOR( old.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
			int nY1 = (int)FLOOR( old.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;
			REF(nX1);
			REF(nY1);

			int nX2 = (int)FLOOR( newpos.fX * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
			int nY2 = (int)FLOOR( newpos.fY * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;

			//gdi_drawline( nX1, nY1, nX2, nY2, gdi_color_red );
			gdi_drawpixel( nX2, nY2, gdi_color_red );

			int nX3 = (int)FLOOR( ( newpos.fX + newdir.x ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_X_CENTER;
			int nY3 = (int)FLOOR( ( newpos.fY + newdir.y ) * TESTCURVE_SCALE ) + DEBUG_WINDOW_Y_CENTER + Y_DELTA;

			gdi_drawline( nX2, nY2, nX3, nY3, gdi_color_yellow );

			old = newpos;
		}
	}
}

void drbSpline()
{
	sgvPath[0] = VECTOR( 10.0f,  0.0f, 0.0f );
	sgvPath[1] = VECTOR( 10.0f, 10.0f, 0.0f );
	sgvPath[2] = VECTOR(  0.0f, 10.0f, 0.0f );
	sgvPath[3] = VECTOR(  0.0f, 20.0f, 0.0f );
	sgvPath[4] = VECTOR( 10.0f, 20.0f, 0.0f );
	sgvPath[5] = VECTOR( 15.0f, 15.0f, 0.0f );
	sgnNumPathKnots = 6;

	DaveDebugWindowSetDrawFn( sDrawTestCurve );
	DaveDebugWindowShow();
}

#endif
