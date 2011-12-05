//----------------------------------------------------------------------------
// e_frustum.cpp
//
// - Implementation for frustum structures and functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


#include "plane.h"
#include "boundingbox.h"
#include "e_settings.h"
#include "e_model.h"
#include "performance.h"
#include "perfhier.h"
#include "camera_priv.h"

#include "e_frustum.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

VIEW_FRUSTUM		gtViewFrustum;			// global current main frustum

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void VIEW_FRUSTUM::Update( const MATRIX * pmatView, const MATRIX * pmatProj )
{
	bCurrent = FALSE;
	ASSERT_RETURN( pmatView && pmatProj );
	if ( ! CameraGetUpdated() )
		return;
	matView = *pmatView;
	matProj = *pmatProj;
	e_MakeFrustumPlanes( tPlanes, pmatView, pmatProj );
	bCurrent = TRUE;
}

const PLANE * VIEW_FRUSTUM::GetPlanes( BOOL bAllowDirty /*= FALSE*/ )
{
	if ( ! bAllowDirty )
	{
		if ( ! IsCurrent() )
			return NULL;
#if ISVERSION(DEVELOPMENT)
		if ( ! CameraGetUpdated() )
			return NULL;
#endif
	}
	return tPlanes;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Returns the points of the frustum in this order:
//   X     Y     Z
// small small small
//  big  small small
// small  big  small
//  big   big  small
// small small  big
//  big  small  big
// small  big   big
//  big   big   big
PRESULT e_ExtractFrustumPoints( VECTOR * pvPoints, const BOUNDING_BOX * pScreenBox, const MATRIX * pmatTrans )
{
	ASSERT_RETINVALIDARG( pvPoints );
	ASSERT_RETINVALIDARG( pScreenBox );
	ASSERT_RETINVALIDARG( pmatTrans );

	const float & sx = pScreenBox->vMin.x;
	const float & sy = pScreenBox->vMin.y;
	const float & sz = 0.f;
	const float & bx = pScreenBox->vMax.x;
	const float & by = pScreenBox->vMax.y;
	const float & bz = 1.f;

	pvPoints[ 0 ] = VECTOR( sx, sy, sz ); // xyz
	pvPoints[ 1 ] = VECTOR( bx, sy, sz ); // Xyz
	pvPoints[ 2 ] = VECTOR( sx, by, sz ); // xYz
	pvPoints[ 3 ] = VECTOR( bx, by, sz ); // XYz
	pvPoints[ 4 ] = VECTOR( sx, sy, bz ); // xyZ
	pvPoints[ 5 ] = VECTOR( bx, sy, bz ); // XyZ
	pvPoints[ 6 ] = VECTOR( sx, by, bz ); // xYZ
	pvPoints[ 7 ] = VECTOR( bx, by, bz ); // XYZ

	for( int i = 0; i < OBB_POINTS; i++ )
		MatrixMultiplyCoord( &pvPoints[ i ], &pvPoints[ i ], pmatTrans );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_ExtractFrustumPoints( VECTOR * pvPoints, const MATRIX * pmatView, const MATRIX * pmatProj )
{
	ASSERT_RETINVALIDARG( pvPoints );
	ASSERT_RETINVALIDARG( pmatView );
	ASSERT_RETINVALIDARG( pmatProj );

	MATRIX mat;

	MatrixMultiply( &mat, pmatView, pmatProj );
	MatrixInverse( &mat, &mat );

	pvPoints[ 0 ] = VECTOR( -1.0f, -1.0f, 0.0f ); // xyz
	pvPoints[ 1 ] = VECTOR(  1.0f, -1.0f, 0.0f ); // Xyz
	pvPoints[ 2 ] = VECTOR( -1.0f,  1.0f, 0.0f ); // xYz
	pvPoints[ 3 ] = VECTOR(  1.0f,  1.0f, 0.0f ); // XYz
	pvPoints[ 4 ] = VECTOR( -1.0f, -1.0f, 1.0f ); // xyZ
	pvPoints[ 5 ] = VECTOR(  1.0f, -1.0f, 1.0f ); // XyZ
	pvPoints[ 6 ] = VECTOR( -1.0f,  1.0f, 1.0f ); // xYZ
	pvPoints[ 7 ] = VECTOR(  1.0f,  1.0f, 1.0f ); // XYZ


	for( int i = 0; i < OBB_POINTS; i++ )
		MatrixMultiplyCoord( &pvPoints[ i ], &pvPoints[ i ], &mat );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// pmatXform: optional transform of extracted world-space points prior to plane creation
PRESULT e_ExtractFrustumPlanes( PLANE * pPlanes, const BOUNDING_BOX * pScreenBox, const MATRIX * pmatView, const MATRIX * pmatProj )
{
	ASSERT_RETINVALIDARG( pPlanes );
	ASSERT_RETINVALIDARG( pScreenBox );
	ASSERT_RETINVALIDARG( pmatView );
	ASSERT_RETINVALIDARG( pmatProj );

	VECTOR vFrustum[OBB_POINTS];

	MATRIX mat;

	MatrixMultiply( &mat, pmatView, pmatProj );
	MatrixInverse( &mat, &mat );

	V_RETURN( e_ExtractFrustumPoints( vFrustum, pScreenBox, &mat ) );

	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_NEAR   ], &vFrustum[0], &vFrustum[1], &vFrustum[2] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_FAR    ], &vFrustum[6], &vFrustum[7], &vFrustum[5] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_LEFT   ], &vFrustum[2], &vFrustum[6], &vFrustum[4] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_RIGHT  ], &vFrustum[7], &vFrustum[3], &vFrustum[5] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_TOP    ], &vFrustum[2], &vFrustum[3], &vFrustum[6] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_BOTTOM ], &vFrustum[1], &vFrustum[0], &vFrustum[4] );

	return S_OK;
}


void e_ExtractAABBBounds( const MATRIX * pmatView, const MATRIX * pmatProj, BOUNDING_BOX * BBox )
{
	// we also maintain an Axial-aligned bounding box for very quick and rough frustum culls

	MATRIX mat;

	MatrixMultiply( &mat, pmatView, pmatProj );
	MatrixInverse( &mat, &mat );

	VECTOR pvPoints[OBB_POINTS];
	pvPoints[ 0 ] = VECTOR( -1.0f, -1.0f, 0.0f ); // xyz
	pvPoints[ 1 ] = VECTOR(  1.0f, -1.0f, 0.0f ); // Xyz
	pvPoints[ 2 ] = VECTOR( -1.0f,  1.0f, 0.0f ); // xYz
	pvPoints[ 3 ] = VECTOR(  1.0f,  1.0f, 0.0f ); // XYz
	pvPoints[ 4 ] = VECTOR( -1.0f, -1.0f, 1.0f ); // xyZ
	pvPoints[ 5 ] = VECTOR(  1.0f, -1.0f, 1.0f ); // XyZ
	pvPoints[ 6 ] = VECTOR( -1.0f,  1.0f, 1.0f ); // xYZ
	pvPoints[ 7 ] = VECTOR(  1.0f,  1.0f, 1.0f ); // XYZ

	MatrixMultiplyCoord( &pvPoints[ 0 ], &pvPoints[ 0 ], &mat );
	BBox->vMin= pvPoints[0];
	BBox->vMax = pvPoints[0];
	for( int i = 1; i < OBB_POINTS; i++ )
	{
		MatrixMultiplyCoord( &pvPoints[ i ], &pvPoints[ i ], &mat );
		BoundingBoxExpandByPoint( BBox, &pvPoints[i] );
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ExtractFrustumPlanes( PLANE * pPlanes, const MATRIX * pmatView, const MATRIX * pmatProj )
{
	ASSERT_RETINVALIDARG( pPlanes );
	ASSERT_RETINVALIDARG( pmatView );
	ASSERT_RETINVALIDARG( pmatProj );

	static VECTOR vFrustum[OBB_POINTS];

	V_RETURN( e_ExtractFrustumPoints( vFrustum, pmatView, pmatProj ) );

	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_NEAR   ], &vFrustum[0], &vFrustum[1], &vFrustum[2] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_FAR    ], &vFrustum[6], &vFrustum[7], &vFrustum[5] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_LEFT   ], &vFrustum[2], &vFrustum[6], &vFrustum[4] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_RIGHT  ], &vFrustum[7], &vFrustum[3], &vFrustum[5] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_TOP    ], &vFrustum[2], &vFrustum[3], &vFrustum[6] );
	PlaneFromPoints( &pPlanes[ FRUSTUM_PLANE_BOTTOM ], &vFrustum[1], &vFrustum[0], &vFrustum[4] );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_MakeFrustumPlanes( PLANE * pPlanes, const MATRIX * pmatView, const MATRIX * pmatProj, int nModelID )
{
	ASSERT( pmatView && pmatProj && pPlanes );

	// Setup the view and projection matrices
	const MATRIX * pmatViewCurrent;
	const MATRIX * pmatProjCurrent;

	pmatViewCurrent = e_ModelGetView( nModelID );
	pmatProjCurrent = e_ModelGetProj( nModelID );

	if ( ! pmatViewCurrent )
		pmatViewCurrent = pmatView;
	if ( ! pmatProjCurrent )
		pmatProjCurrent = pmatProj;

	V( e_ExtractFrustumPlanes( pPlanes, pmatViewCurrent, pmatProjCurrent ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_OBBInFrustum( ORIENTED_BOUNDING_BOX pPoints, const PLANE * pPlanes )
{
	//PERF( FRUSTUM_CULL )
	HITCOUNT( FRUSTUM_CULL )

	ASSERT( pPoints );
	ASSERT( pPlanes );
	const int nPoints = OBB_POINTS;

	// CHB 2005.08.22 - This could be optimized heavily if
	// restricted to view space, e.g., near and far perpendicular
	// to Z axis, left and right perpendicular to XZ plane, and top
	// and bottom perpendicular to YZ plane.

	// CHB 2005.08.22 - Empirical analysis shows that the planes are
	// responsible for rejection in roughly this order:
	// left, right, near, top and bottom (about even), far.

	// next, dot against left plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_LEFT ] )	< 0 )
		return FALSE;
	// next, dot against right plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_RIGHT ] )	< 0 )
		return FALSE;
	// first, dot against near plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_NEAR ] )	< 0 )
		return FALSE;
	// next, dot against top plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_TOP ] )	< 0 )
		return FALSE;
	// next, dot against bottom plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_BOTTOM ] )	< 0 )
		return FALSE;
	// next, dot against far plane
	if ( e_BoxPointsVsPlaneDotProduct( pPoints, nPoints, &pPlanes[ FRUSTUM_PLANE_FAR ] )	< 0 )
		return FALSE;

	return TRUE;
}

int	e_IntersectsAABB( BOUNDING_BOX * FrustumBox, BOUNDING_BOX * BBox )	// max AABB bounds of the object to intersect
						
{

	// see if the AABBs intersect
	if( BoundingBoxCollideXY( FrustumBox, BBox ) )
		{
		// see if they contain one another
		if( BoundingBoxContainsXY( FrustumBox, BBox ) )
		{
			return 1;
		}

		return 0;
	}
	return -1;
} // IntersectsAABB()	

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_PointInFrustum( VECTOR *pPoint, const PLANE * pPlanes, float fPad /*= 0.f*/ )
{
	HITCOUNT( FRUSTUM_CULL )
	ASSERT( pPoint );

	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_LEFT   ], pPoint ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_RIGHT  ], pPoint ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_NEAR   ], pPoint ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_TOP    ], pPoint ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_BOTTOM ], pPoint ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_FAR    ], pPoint ) + fPad < 0.0f )	return FALSE;
	return TRUE;	
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_PointInFrustumCheap( VECTOR *pPoint, const PLANE * pPlanes, float fPad /*= 0.f*/ )
{
	HITCOUNT( FRUSTUM_CULL )
	ASSERT( pPoint );

	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_LEFT   ],	pPoint  ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_RIGHT  ],	pPoint  ) + fPad < 0.0f )	return FALSE;
	if ( PlaneDotCoord( &pPlanes[ FRUSTUM_PLANE_NEAR   ],	pPoint  ) + fPad < 0.0f )	return FALSE;
	return TRUE;	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_PrimitiveInFrustum(
	PRIMITIVE_INFO * ptInfo,
	PLANE * pPlanes )
{
	UNDEFINED_CODE();
	return TRUE;
}

