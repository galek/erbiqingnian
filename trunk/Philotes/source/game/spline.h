//----------------------------------------------------------------------------
// spline.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _SPLINE_H
#define _SPLINE_H

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define SPLINE_T				3

#define MIN_SPLINE_KNOTS		2
#define MAX_SPLINE_KNOTS		10

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SplineSetKnotTable( int * nKnotTbl, int num_knots );
void SplineGetPosition( VECTOR * vKnots, int num_knots, int * nKnotTbl, VECTOR * vOut, float delta );
void SplineGetCatmullRomPosition( VECTOR * vKnots, int num_knots, int current_knot, float time_delta, VECTOR * vOutPosition, VECTOR * vOutDirection = NULL );

#endif
