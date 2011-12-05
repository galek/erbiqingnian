//----------------------------------------------------------------------------
// e_portal.cpp
//
// - Implementation for portal functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_primitive.h"
#include "e_frustum.h"
#include "e_main.h"
#include "e_settings.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "e_region.h"

#include "e_portal.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DEBUG_PORTAL_RENDER
{
	VECTOR2 vMin;
	VECTOR2 vMax;
	DWORD dwColor;
	int nDepth;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CHash<PORTAL>	gtPortals;
static int		gnPortalNextId = 0;

// override view parameters
//const MATRIX *	gpmOverrideViewMatrix	= NULL;
//const VECTOR *	gpvOverrideEyeLocation	= NULL;
//const VECTOR *	gpvOverrideEyeDirection = NULL;
//const VECTOR *	gpvOverrideEyeLookat	= NULL;

SIMPLE_DYNAMIC_ARRAY<DEBUG_PORTAL_RENDER> sgtDebugPortalRenders;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

PRESULT e_PortalsInit()
{
	HashInit(gtPortals, NULL, 4);
	ArrayInit(sgtDebugPortalRenders, NULL, 4);

	//gpmOverrideViewMatrix	= NULL;
	//gpvOverrideEyeLocation	= NULL;
	//gpvOverrideEyeDirection = NULL;
	//gpvOverrideEyeLookat	= NULL;

	V( e_RegionsInit() );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalsDestroy()
{
	V( e_PortalsReleaseAll() );

	sgtDebugPortalRenders.Destroy();
	HashFree( gtPortals );

	V( e_RegionsDestroy() );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalsReset()
{
	// clear out any existing connections *and* portals we have
	ArrayClear(sgtDebugPortalRenders);
	HashClear(gtPortals);

	//gpmOverrideViewMatrix	= NULL;
	//gpvOverrideEyeLocation	= NULL;
	//gpvOverrideEyeDirection = NULL;
	//gpvOverrideEyeLookat	= NULL;

	V( e_RegionsReset() );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPortalCalculateVectors(
	PORTAL * pPortal )
{
	ASSERT_RETURN( pPortal->nCorners > 0 );

	// find center
	pPortal->vCenter = 0.5f * ( pPortal->tBBox.vMin + pPortal->tBBox.vMax );

	VectorNormalize( pPortal->vNormal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPortalMakeBoundingBox(
	PORTAL * pPortal,
	const VECTOR * vCorners,
	int nCorners )
{
	ASSERT_RETURN( nCorners > 0 );

	// build bbox
	pPortal->tBBox.vMin = vCorners[ 0 ];
	pPortal->tBBox.vMax = vCorners[ 0 ];
	for ( int i = 1; i < nCorners; i++ )
		BoundingBoxExpandByPoint( &pPortal->tBBox, &vCorners[ i ] );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPortalSortCornersByAxis( int nAxis, const VECTOR * pvCorners, int nCorners, int * pnOrder )
{
	// insertion sort by axis
	float fValues[ PORTAL_NUM_CORNERS ];
	for ( int i = 0; i < nCorners; i++ )
		fValues[ i ] = ( (const float*)&pvCorners[ i ] )[ nAxis ];

	memset( pnOrder, -1, sizeof(int) * nCorners );

	for ( int i = 0; i < nCorners; i++ )
	{
		// insert sort each corner
		int n;
		for ( n = 0; n < nCorners; n++ )
			if ( pnOrder[ n ] == -1 ||   fValues[ pnOrder[ n ] ] > fValues[ i ]   )
				break;
		ASSERT( n < nCorners );

		size_t nSize = sizeof(int) * ( nCorners - (n+1) );
		// in this case, capacity and amount to move are identical
		MemoryMove( &pnOrder[ n+1 ], nSize, &pnOrder[ n ], nSize );
		pnOrder[ n ] = i;
	}

	// validate
	for ( int i = 0; i < nCorners-1; i++ )
	{
		ASSERT( fValues[ pnOrder[ i ] ] <= fValues[ pnOrder[ i+1 ] ] );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPortalCopyCorners(
	PORTAL * pPortal,
	const VECTOR * vCorners,
	int nCorners )
{
	return; // not necessary when rendering via particle

	ASSERT_RETURN( nCorners > 0 );

	// currently forcing a squarish shape
	ASSERT_RETURN( nCorners == PORTAL_NUM_CORNERS );

	ASSERT( pPortal->tBBox.vMax.x >= pPortal->tBBox.vMin.x );
	ASSERT( pPortal->tBBox.vMax.y >= pPortal->tBBox.vMin.y );
	ASSERT( pPortal->tBBox.vMax.z >= pPortal->tBBox.vMin.z );

	MemoryCopy( pPortal->vCorners, sizeof(pPortal->vCorners), vCorners, sizeof(VECTOR) * nCorners );
	pPortal->nCorners = nCorners;

	// first build index lists sorting corners by x, y and z
	int nOrderX[ PORTAL_NUM_CORNERS ];
	int nOrderY[ PORTAL_NUM_CORNERS ];
	int nOrderZ[ PORTAL_NUM_CORNERS ];
	sPortalSortCornersByAxis( 0, vCorners, nCorners, nOrderX );
	sPortalSortCornersByAxis( 1, vCorners, nCorners, nOrderY );
	sPortalSortCornersByAxis( 2, vCorners, nCorners, nOrderZ );

	// using the axis of greatest variance, assume the two min and two max are edges
	VECTOR vDelta = pPortal->tBBox.vMax - pPortal->tBBox.vMin;

	int * pnOrderMajor;
	if ( vDelta.fX > vDelta.fY )
		// min x form one edge, max x the opposite
		pnOrderMajor = nOrderX;
	else
		// min y form one edge, max y the opposite
		pnOrderMajor = nOrderY;

	pPortal->nEdges[ 0 ][ 0 ] = pnOrderMajor[ 0 ];
	pPortal->nEdges[ 0 ][ 1 ] = pnOrderMajor[ 1 ];
	pPortal->nEdges[ 1 ][ 0 ] = pnOrderMajor[ 2 ];
	pPortal->nEdges[ 1 ][ 1 ] = pnOrderMajor[ 3 ];

	// min z forms the third edge, max z the fourth
	pPortal->nEdges[ 2 ][ 0 ] = nOrderZ[ 0 ];
	pPortal->nEdges[ 2 ][ 1 ] = nOrderZ[ 1 ];
	pPortal->nEdges[ 3 ][ 0 ] = nOrderZ[ 2 ];
	pPortal->nEdges[ 3 ][ 1 ] = nOrderZ[ 3 ];

	// find proper tristrip ordering (zigzag)
	pPortal->nStrip[ 0 ] = pPortal->nEdges[ 0 ][ 0 ];
	pPortal->nStrip[ 1 ] = pPortal->nEdges[ 0 ][ 1 ];
	pPortal->nStrip[ 2 ] = pPortal->nEdges[ 1 ][ 0 ];
	pPortal->nStrip[ 3 ] = pPortal->nEdges[ 1 ][ 1 ];
	// need to flip the 2nd pair?
	if ( ! ( ( pPortal->nStrip[ 0 ] == nOrderZ[ 0 ] &&
			   pPortal->nStrip[ 2 ] == nOrderZ[ 1 ] ) ||
			 ( pPortal->nStrip[ 1 ] == nOrderZ[ 0 ] &&
			   pPortal->nStrip[ 3 ] == nOrderZ[ 1 ] ) ||
			 ( pPortal->nStrip[ 2 ] == nOrderZ[ 0 ] &&
			   pPortal->nStrip[ 0 ] == nOrderZ[ 1 ] ) ||
			 ( pPortal->nStrip[ 3 ] == nOrderZ[ 0 ] &&
			   pPortal->nStrip[ 1 ] == nOrderZ[ 1 ] ) ) )
	{
		pPortal->nStrip[ 2 ] = pPortal->nEdges[ 1 ][ 1 ];	// swapping
		pPortal->nStrip[ 3 ] = pPortal->nEdges[ 1 ][ 0 ];
	}





	// dirty hack to make it look cooler:  make the two top ones same height as portal width
	float fWidth = max( vDelta.x, vDelta.y );
	float fBump = fWidth - vDelta.z;
	pPortal->vCorners[ nOrderZ[ 2 ] ].z += max( 0, fBump );
	pPortal->vCorners[ nOrderZ[ 3 ] ].z += max( 0, fBump );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// assumes the corners form a convex shape
int e_PortalCreate(
	int nRegion )
{
	int nPortalID = gnPortalNextId;
	PORTAL * pPortal = HashAdd( gtPortals, nPortalID );
	gnPortalNextId++;
	ASSERT_RETINVALID( pPortal );

	pPortal->pConnection	= NULL;
	pPortal->nRegion		= nRegion;

	e_PortalRestore( pPortal );

	pPortal->nCullingPortalID = INVALID_ID;

	return nPortalID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalSetDestination(
	PORTAL * pPortalFrom,
	PORTAL * pPortalDest )
{
	ASSERT_RETURN( pPortalFrom );
	ASSERT_RETURN( pPortalDest );

	pPortalFrom->pConnection = pPortalDest;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalSetCellID(
	PORTAL * pPortal,
	int nUserData )
{
	ASSERT_RETURN( pPortal );

	pPortal->nCellID = nUserData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_PortalIsActive( int nPortal )
{
	PORTAL * pPortal = e_PortalGet( nPortal );
	return e_PortalIsActive( pPortal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_PortalIsActive( PORTAL * pPortal )
{
	if ( ! e_PortalHasConnection( pPortal ) )
		return FALSE;
	if ( ! e_PortalHasFlags( pPortal,			   PORTAL_FLAG_ACTIVE ) ||
		 ! e_PortalHasFlags( pPortal->pConnection, PORTAL_FLAG_ACTIVE ) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_PortalHasConnection( PORTAL * pPortal )
{
	return ( pPortal && pPortal->pConnection );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_PortalHasFlags( PORTAL * pPortal, DWORD dwFlags )
{
	return ( dwFlags == ( pPortal->dwFlags & dwFlags ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalConnect(
	PORTAL * pPortalFrom )
{
	ASSERT_RETINVALIDARG( pPortalFrom );
	if ( 0 == ( pPortalFrom->dwFlags & PORTAL_FLAG_SHAPESET ) )
		return S_FALSE;
	PORTAL * pPortalDest = pPortalFrom->pConnection;
	if ( ! pPortalDest || 0 == ( pPortalDest->dwFlags & PORTAL_FLAG_SHAPESET ) )
		return S_FALSE;

	if ( pPortalFrom->nCullingPortalID == INVALID_ID || pPortalDest->nCullingPortalID == INVALID_ID )
		return S_FALSE;

	CULLING_VIRTUAL_PORTAL * pCVPFrom = e_VirtualPortalGet( pPortalFrom->nCullingPortalID );
	ASSERT_RETFAIL( pCVPFrom );
	CULLING_VIRTUAL_PORTAL * pCVPTo = e_VirtualPortalGet( pPortalDest->nCullingPortalID );
	ASSERT_RETFAIL( pCVPTo );
	V( e_DPVS_VirtualPortalSetTarget( *pCVPFrom, *pCVPTo ) );
	pPortalFrom->dwFlags |= PORTAL_FLAG_TARGETSET;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_PortalSetShape(
	PORTAL * pPortal,
	const VECTOR * pvCorners,
	int nCorners,
	const VECTOR * pvNormal )
{
	ASSERT_RETINVALIDARG( pPortal );

	if ( pPortal->nCullingPortalID != INVALID_ID )
	{
		if ( e_VirtualPortalGet( pPortal->nCullingPortalID ) )
		{
			return S_FALSE;
		}
	}

	if ( pvNormal )
		pPortal->vNormal = *pvNormal;

	if ( pvCorners )
	{
		V_RETURN( e_CellCreateVirtualPortal( pPortal->nCullingPortalID, pPortal->nCellID, pvCorners, nCorners, pPortal->vNormal ) );
		pPortal->dwFlags |= PORTAL_FLAG_SHAPESET;
	}

	return S_OK;

	//if ( pvCorners )
	//{
	//	// currently forcing a squarish shape
	//	ASSERT_RETINVALIDARG( nCorners == PORTAL_NUM_CORNERS );

	//	if ( pPortal->dwFlags & PORTAL_FLAG_SHAPESET )
	//	{
	//		// update positions, but assume same vertex ordering (pleasepleaseplease)
	//		ASSERT_RETFAIL( pPortal->nCorners == nCorners );
	//		MemoryCopy( pPortal->vCorners, sizeof(pPortal->vCorners), pvCorners, sizeof(VECTOR) * nCorners );
	//		sPortalMakeBoundingBox( pPortal, pvCorners, nCorners );
	//		sPortalCalculateVectors( pPortal );
	//	}
	//	else
	//	{
	//		// this is now the same as above, as long as particles are providing the final quad geometry

	//		sPortalMakeBoundingBox( pPortal, pvCorners, nCorners );
	//		ASSERT_RETINVALIDARG( nCorners );
	//		pPortal->nCorners = nCorners;
	//		MemoryCopy( pPortal->vCorners, sizeof(pPortal->vCorners), pvCorners, sizeof(VECTOR) * nCorners );
	//		//sPortalCopyCorners( pPortal, pvCorners, nCorners );
	//		sPortalCalculateVectors( pPortal );
	//		pPortal->dwFlags |= PORTAL_FLAG_SHAPESET;
	//	}
	//}
	//return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// removes the portal and breaks the connection with it's connection portal, if any
void e_PortalRemove(
	int nPortalID )
{
	PORTAL * pPortal = e_PortalGet( nPortalID );
	if ( ! pPortal )
		return;

	// if any portals are connected to this portal, turn off the connection
	for ( PORTAL * pFrom = e_PortalGetFirst();
		pFrom;
		pFrom = e_PortalGetNext( pFrom ) )
	{
		if ( pFrom->pConnection == pPortal )
		{
			pFrom->pConnection = NULL;
		}
	}

	HashRemove( gtPortals, nPortalID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalUpdateMatrices( PORTAL * pPortal, const VECTOR & vEye, const VECTOR & vLookat )
{
#if 0
	if ( ! pPortal->pConnection )
		return;

	VECTOR vUp( 0,0,1 );
	MATRIX mLocalPortal, mRemotePortal;

	MatrixFromVectors( mLocalPortal, pPortal->vCenter, vUp, pPortal->vNormal );
	MatrixInverse( &mLocalPortal, &mLocalPortal );
	VECTOR vNormal = -pPortal->pConnection->vNormal;
	MatrixFromVectors( mRemotePortal, pPortal->pConnection->vCenter, vUp, vNormal );

	MATRIX mLocalToRemote;
	MatrixMultiply( &mLocalToRemote, &mLocalPortal, &mRemotePortal );


	// build new view matrix and eye parameters
	MatrixMultiply( &pPortal->vEyeLocation, &vEye, &mLocalToRemote );
	VECTOR vEyeDir = vLookat - vEye;
	VectorNormalize( vEyeDir );
	MatrixMultiply( &pPortal->vEyeDirection, &vEyeDir, &mLocalToRemote );
	MatrixMultiply( &pPortal->vEyeLookat, &vLookat, &mLocalToRemote );
	MatrixMakeView( pPortal->mView, pPortal->vEyeLocation, pPortal->vEyeLookat, vUp );



	MATRIX matWorld;
	MATRIX matView;
	MATRIX matProj;
	e_GetWorldViewProjMatricies( &matWorld, &matView, &matProj );
	MATRIX matWorldView;
	MATRIX matWVP;
	MatrixMultiply( &matWorldView, &matWorld, &matView );
	MatrixMultiply( &matWVP, &matWorldView, &matProj );

	VECTOR * vVecs = pPortal->vScreenCorners;

	for ( int i = 0; i < PORTAL_NUM_CORNERS; i++ )
	{
		VECTOR4 vTransformed;
		MatrixMultiply( &vTransformed, &pPortal->vCorners[ i ], &matWVP );
		vVecs[ i ].x = vTransformed.x / vTransformed.w;
		vVecs[ i ].y = vTransformed.y / vTransformed.w;
		vVecs[ i ].z = vTransformed.z / vTransformed.w;
		if ( vTransformed.w < 0.0f )
		{
			vVecs[ i ].x = - vVecs[ i ].x;
			vVecs[ i ].y = - vVecs[ i ].y;
		}
	}

	pPortal->tScreenBBox.vMin = vVecs[ 0 ];
	pPortal->tScreenBBox.vMax = vVecs[ 0 ];
	for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
	{
		BoundingBoxExpandByPoint( &pPortal->tScreenBBox, &vVecs[ i ] );
	}

	VECTOR vCenter = vVecs[ 0 ];
	for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
	{
		vCenter += vVecs[ i ];
	}
	vCenter /= PORTAL_NUM_CORNERS;

	// push edges outwards in screenspace
	VECTOR2 vT;
	int nWidth, nHeight;
	dxC_GetRenderTargetDimensions( nWidth, nHeight );
	float fWidthAdj  = 0.5f / nWidth  * 10.f;
	float fHeightAdj = 0.5f / nHeight * 10.f;
	for ( int i = 0; i < PORTAL_NUM_CORNERS; i++ )
	{
		vT.x = vVecs[ i ].x - vCenter.x;
		vT.y = vVecs[ i ].y - vCenter.y;
		VectorNormalize( vT );
		if ( vT.x < 0 )
			vT.x = -fWidthAdj;
		else
			vT.x = fWidthAdj;
		if ( vT.y < 0 )
			vT.y = -fHeightAdj;
		else
			vT.y = fHeightAdj;
		vVecs[ i ].x += vT.x;
		vVecs[ i ].y += vT.y;
	}

	pPortal->tScreenBBox.vMin = vVecs[ 0 ];
	pPortal->tScreenBBox.vMax = vVecs[ 0 ];
	for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
	{
		BoundingBoxExpandByPoint( &pPortal->tScreenBBox, &vVecs[ i ] );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalsUpdateVisible( const VECTOR & vEye, const VECTOR & vLookat )
{
/*
	const PLANE * pPlanes = e_GetCurrentViewFrustum()->GetPlanes();
	ORIENTED_BOUNDING_BOX tOBB;

	if ( ! e_GetRenderFlag( RENDER_FLAG_PORTAL_RENDER_ENABLED ) )
	{
		for ( PORTAL * pPortal = e_PortalGetFirst();
			pPortal;
			pPortal = e_PortalGetNext( pPortal ) )
		{
			pPortal->dwFlags &= ~PORTAL_FLAG_VISIBLE;
		}
		return;
	}

	if ( !pPlanes )
		return;

	for ( PORTAL * pPortal = e_PortalGetFirst();
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		if ( ! pPortal->pConnection )
			continue;
		if ( 0 == ( pPortal->dwFlags			  & PORTAL_FLAG_ACTIVE ) ||
			 0 == ( pPortal->pConnection->dwFlags & PORTAL_FLAG_ACTIVE ) )
		{
			continue;
		}

		// if this side of the portal is marked nodraw, skip update
		if ( pPortal->dwFlags & PORTAL_FLAG_NODRAW )
		{
			continue;
		}

		// don't render the portal if the eye and lookat are on opposite sides of the plane
		VECTOR vP2E = vEye    - pPortal->vCenter;
		VectorNormalize( vP2E );
		VECTOR vP2L = vLookat - pPortal->vCenter;
		VectorNormalize( vP2L );
		float fP2E = VectorDot( vP2E, pPortal->vNormal );
		float fP2L = VectorDot( vP2L, pPortal->vNormal );
		if ( ( fP2E <= 0.f && fP2L >= 0.f ) || ( fP2E >= 0.f && fP2L <= 0.f ) )
		{
			e_PortalSetFlags( pPortal, PORTAL_FLAG_VISIBLE, FALSE );
			continue;
		}

		// just doing a frustum cull right now :/
		MakeOBBFromAABB( tOBB, &pPortal->tBBox );
		BOOL bInFrustum = e_OBBInFrustum( tOBB, pPlanes );
		e_PortalSetFlags( pPortal, PORTAL_FLAG_VISIBLE, bInFrustum );

		if ( bInFrustum )
		{
			// this portal, and its connection, need updating
			pPortal->dwFlags |= PORTAL_FLAG_NEEDUPDATE;
			pPortal->pConnection->dwFlags |= PORTAL_FLAG_NEEDUPDATE;
		}
	}

	for ( PORTAL * pPortal = e_PortalGetFirst();
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		if ( ! pPortal->pConnection )
			continue;

		if ( 0 == ( pPortal->dwFlags			  & PORTAL_FLAG_NEEDUPDATE ) ||
			 0 == ( pPortal->dwFlags			  & PORTAL_FLAG_ACTIVE ) ||
			 0 == ( pPortal->pConnection->dwFlags & PORTAL_FLAG_ACTIVE ) )
		{
			continue;
		}

		// set the view parameters for stuff through the portal

		if ( pPortal->dwFlags & PORTAL_FLAG_VISIBLE )
		{
			// this portal is primarily visible

			e_PortalUpdateMatrices( pPortal, vEye, vLookat );
		}
		else
		{
			// this is the connection

		}

		pPortal->dwFlags &= ~PORTAL_FLAG_NEEDUPDATE;
	}
*/
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PORTAL * e_PortalGetFirstVisible()
{
	for ( PORTAL * pPortal = e_PortalGetFirst();
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		if ( /*pPortal->dwFlags			   & PORTAL_FLAG_VISIBLE &&*/
			 pPortal->pConnection								 &&
			 pPortal->dwFlags			   & PORTAL_FLAG_ACTIVE  &&
			 pPortal->pConnection->dwFlags & PORTAL_FLAG_ACTIVE )
			return pPortal;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PORTAL * e_PortalGetNextVisible( PORTAL * pPortal )
{
	ASSERT_RETNULL( pPortal );
	
	for ( pPortal = e_PortalGetNext( pPortal );
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		if ( /*pPortal->dwFlags			   & PORTAL_FLAG_VISIBLE &&*/
			 pPortal->pConnection								 &&
			 pPortal->dwFlags			   & PORTAL_FLAG_ACTIVE  &&
			 pPortal->pConnection->dwFlags & PORTAL_FLAG_ACTIVE )
			return pPortal;
	}

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_SetupPortal( int nPortal )
{
/*
	PORTAL * pPortal = e_PortalGet( nPortal );
	if ( pPortal )
	{
		// setup region and clip planes
		V( e_SetRegion( pPortal->pConnection->nRegion ) );


		if ( 0 == ( pPortal->dwFlags & PORTAL_FLAG_SHAPESET ) || 
			 0 == ( pPortal->pConnection->dwFlags & PORTAL_FLAG_SHAPESET ) )
			 return S_FALSE;


		//MATRIX matWorld;
		//MATRIX matView;
		//MATRIX matProj;
		//e_GetWorldViewProjMatricies( &matWorld, &matView, &matProj );
		//MATRIX matWorldView;
		//MATRIX matWVP;
		//MatrixMultiply( &matWorldView, &matWorld, &matView );
		//MatrixMultiply( &matWVP, &matWorldView, &matProj );

		//VECTOR vVecs[PORTAL_NUM_CORNERS];

		//for ( int i = 0; i < PORTAL_NUM_CORNERS; i++ )
		//{
		//	VECTOR4 vTransformed;
		//	MatrixMultiply( &vTransformed, &pPortal->vCorners[ i ], &matWVP );
		//	vVecs[ i ].x = vTransformed.x / vTransformed.w;
		//	vVecs[ i ].y = vTransformed.y / vTransformed.w;
		//	vVecs[ i ].z = vTransformed.z / vTransformed.w;
		//	if ( vTransformed.w < 0.0f )
		//	{
		//		vVecs[ i ].x = - vVecs[ i ].x;
		//		vVecs[ i ].y = - vVecs[ i ].y;
		//	}
		//}

		//pPortal->tScreenBBox.vMin = vVecs[ 0 ];
		//pPortal->tScreenBBox.vMax = vVecs[ 0 ];
		//for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
		//{
		//	BoundingBoxExpandByPoint( &pPortal->tScreenBBox, &vVecs[ i ] );
		//}

		//VECTOR vCenter = vVecs[ 0 ];
		//for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
		//{
		//	vCenter += vVecs[ i ];
		//}
		//vCenter /= PORTAL_NUM_CORNERS;

		//// push edges outwards in screenspace
		//VECTOR2 vT;
		//int nWidth, nHeight;
		//dxC_GetRenderTargetDimensions( nWidth, nHeight );
		//float fWidthAdj  = 0.5f / nWidth  * 10.f;
		//float fHeightAdj = 0.5f / nHeight * 10.f;
		//for ( int i = 0; i < PORTAL_NUM_CORNERS; i++ )
		//{
		//	vT.x = vVecs[ i ].x - vCenter.x;
		//	vT.y = vVecs[ i ].y - vCenter.y;
		//	VectorNormalize( vT );
		//	if ( vT.x < 0 )
		//		vT.x = -fWidthAdj;
		//	else
		//		vT.x = fWidthAdj;
		//	if ( vT.y < 0 )
		//		vT.y = -fHeightAdj;
		//	else
		//		vT.y = fHeightAdj;
		//	vVecs[ i ].x += vT.x;
		//	vVecs[ i ].y += vT.y;
		//}

		//pPortal->tScreenBBox.vMin = vVecs[ 0 ];
		//pPortal->tScreenBBox.vMax = vVecs[ 0 ];
		//for ( int i = 1; i < PORTAL_NUM_CORNERS; i++ )
		//{
		//	BoundingBoxExpandByPoint( &pPortal->tScreenBBox, &vVecs[ i ] );
		//}

		VECTOR * vVecs = pPortal->vScreenCorners;

		int nWidth, nHeight;
		V( dxC_GetRenderTargetDimensions( nWidth, nHeight ) );

		PLANE tPlane;
		PlaneFromPoints( &tPlane, &pPortal->pConnection->vCorners[ 0 ], &pPortal->pConnection->vCorners[ 1 ], &pPortal->pConnection->vCorners[ 2 ] );
		float fDot = PlaneDotCoord( &tPlane, &pPortal->vEyeLocation );

		VECTOR2 vMin( pPortal->tScreenBBox.vMin.x, -pPortal->tScreenBBox.vMax.y );
		VECTOR2 vMax( pPortal->tScreenBBox.vMax.x, -pPortal->tScreenBBox.vMin.y );

		RECT tRect;
		tRect.top    = (LONG)( ( vMin.fY * 0.5f + 0.5f ) * nHeight );
		tRect.bottom = (LONG)( ( vMax.fY * 0.5f + 0.5f ) * nHeight );
		tRect.left   = (LONG)( ( vMin.fX * 0.5f + 0.5f ) * nWidth  );
		tRect.right  = (LONG)( ( vMax.fX * 0.5f + 0.5f ) * nWidth  );
		tRect.top    = max( tRect.top,    0 );
		tRect.bottom = min( tRect.bottom, nHeight );
		tRect.left   = max( tRect.left,	  0 );
		tRect.right  = min( tRect.right,  nWidth );
		//tRect.right  = nWidth  - tRect.right;
		//tRect.bottom = nHeight - tRect.bottom;
		keytrace( "rect: l:%4d t:%4d r:%4d b:%4d  l:%2.2f t:%2.2f r:%2.2f b:%2.2f\n",
			tRect.left,	tRect.top, tRect.right, tRect.bottom,
			pPortal->tScreenBBox.vMin.x, 
			pPortal->tScreenBBox.vMin.y, 
			pPortal->tScreenBBox.vMax.x, 
			pPortal->tScreenBBox.vMax.y );
		//dxC_SetViewport(
		//	tRect.left,
		//	tRect.top,
		//	tRect.right,
		//	tRect.bottom,
		//	0.f,
		//	1.f );
		V( dxC_SetScissorRect( &tRect ) );

		//dxC_SetClipPlaneRect( (D3DXVECTOR3*)vVecs, pPortal->nCorners, (D3DXMATRIXA16*)&matProj, FALSE );
		if ( fDot > 0 )
		{
			V( dxC_PushClipPlane( (D3DXVECTOR3*)&vVecs[ 0 ], (D3DXVECTOR3*)&vVecs[ 1 ], (D3DXVECTOR3*)&vVecs[ 2 ] ) );
		}
		else
		{
			V( dxC_PushClipPlane( (D3DXVECTOR3*)&vVecs[ 0 ], (D3DXVECTOR3*)&vVecs[ 2 ], (D3DXVECTOR3*)&vVecs[ 1 ] ) );
		}
		V( dxC_CommitClipPlanes() );

		e_SetCurrentViewOverrides( pPortal );
	}
	else
	{
		if ( c_GetToolMode() )
		{
			V( e_SetRegion( REGION_DEFAULT ) );
		}

		V( dxC_ResetViewport() ); 
		V( dxC_ResetClipPlanes() );

		// debug
		//e_PortalDebugRenderAll();
	}
*/
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DebugDrawPortal( BOUNDING_BOX & tClipBox, DWORD dwColor, int nDepth )
{
	// add this portal to a list to be rendered later
	if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_PORTAL_INFO ) )
		return;

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );
	DEBUG_PORTAL_RENDER tPortal;
	tPortal.vMin = VECTOR2( ( tClipBox.vMin.x * 0.5f + 0.5f ) * nWidth, ( 1.0f - ( tClipBox.vMin.y * 0.5f + 0.5f ) ) * nHeight );
	tPortal.vMax = VECTOR2( ( tClipBox.vMax.x * 0.5f + 0.5f ) * nWidth, ( 1.0f - ( tClipBox.vMax.y * 0.5f + 0.5f ) ) * nHeight );
	tPortal.dwColor = dwColor;
	tPortal.nDepth = nDepth;

	ArrayAddItem(sgtDebugPortalRenders, tPortal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DebugRenderPortals()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_PORTAL_INFO ) )
	{
		ArrayClear(sgtDebugPortalRenders);
		return;
	}

	// process and create primitives for debug portal list
	int nMinDepth = INT_MAX;
	int nMaxDepth = 0;
	int nCount = sgtDebugPortalRenders.Count();

	for ( int i = 0; i < nCount; i++ )
	{
		const DEBUG_PORTAL_RENDER & tPortal = sgtDebugPortalRenders[ i ];
		if ( nMinDepth > tPortal.nDepth )
			nMinDepth = tPortal.nDepth;
		if ( nMaxDepth < tPortal.nDepth )
			nMaxDepth = tPortal.nDepth;
	}

	nMaxDepth -= nMinDepth;
	// avoid div by 0
	if ( nMaxDepth < 0 )
		nMaxDepth = 0;

	DWORD dwBorderColor = 0xffffffff;
	DWORD dwPrimFlags = 0;
	DWORD dwPrimWireFlags = PRIM_FLAG_RENDER_ON_TOP;
	const BYTE  byAlpha = 0x00;
	const float fMaxPower = 0.3f;

	// alpha step
	float fPowerStep = fMaxPower / ( nMaxDepth + 1 );
	for ( int i = 0; i < nCount; i++ )
	{
		const DEBUG_PORTAL_RENDER & tPortal = sgtDebugPortalRenders[ i ];
		DWORD dwColor = tPortal.dwColor;
		BYTE * pCol = (BYTE*)&dwColor;
		float fPower = fPowerStep * ( tPortal.nDepth - nMinDepth + 1 );
		pCol[ 0 ] = (BYTE)( pCol[ 0 ] * fPower );
		pCol[ 1 ] = (BYTE)( pCol[ 1 ] * fPower );
		pCol[ 2 ] = (BYTE)( pCol[ 2 ] * fPower );
		pCol[ 3 ] = byAlpha;

		VECTOR2 vUL( tPortal.vMin.x, tPortal.vMax.y );
		VECTOR2 vUR( tPortal.vMax.x, tPortal.vMax.y );
		VECTOR2 vLL( tPortal.vMin.x, tPortal.vMin.y );
		VECTOR2 vLR( tPortal.vMax.x, tPortal.vMin.y );

		V( e_PrimitiveAddLine( dwPrimWireFlags, &vUL, &vUR, dwBorderColor ) );
		V( e_PrimitiveAddLine( dwPrimWireFlags, &vUR, &vLR, dwBorderColor ) );
		V( e_PrimitiveAddLine( dwPrimWireFlags, &vLR, &vLL, dwBorderColor ) );
		V( e_PrimitiveAddLine( dwPrimWireFlags, &vLL, &vUL, dwBorderColor ) );

		V( e_PrimitiveAddTri( dwPrimFlags, &vUL, &vUR, &vLL, dwColor ) );
		V( e_PrimitiveAddTri( dwPrimFlags, &vLL, &vUR, &vLR, dwColor ) );
	}

	ArrayClear(sgtDebugPortalRenders);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalDebugRender(
	int nPortalID )
{
	return;
	//if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_PORTAL_INFO ) )
	//	return;

	PORTAL * pPortal = e_PortalGet( nPortalID );
	if ( ! pPortal )
		return;

	DWORD dwColor = 0x00004f4f;
	DWORD dwBorderColor = 0xffffffff;
	//D3DCOLOR dwColor = ARGB_MAKE_FROM_INT(0, 0, 79, 79);
	//D3DCOLOR dwBorderColor = ARGB_MAKE_FROM_INT(255, 255, 255, 255);

	DWORD dwPrimFlags = 0;
	DWORD dwPrimWireFlags = PRIM_FLAG_RENDER_ON_TOP;

	switch ( pPortal->nCorners )
	{
	case 3:
		{
			V( e_PrimitiveAddTri( dwPrimFlags, &pPortal->vCorners[ 0 ], &pPortal->vCorners[ 1 ], &pPortal->vCorners[ 2 ], dwColor ) );
		}
		break;
	case 4:
		{
			// clobber for coverage, since I don't know the corner ordering
			// halve the color to keep equivalent coloring
			BYTE * pCols;
			pCols = (BYTE*)&dwColor;
			dwColor = ARGB_MAKE_FROM_INT( pCols[ 2 ] / 2, pCols[ 1 ] / 2, pCols[ 0 ] / 2, pCols[ 3 ] );
			V( e_PrimitiveAddLine( dwPrimWireFlags, &pPortal->vCorners[ pPortal->nEdges[ 0 ][ 0 ] ], &pPortal->vCorners[ pPortal->nEdges[ 0 ][ 1 ] ], dwBorderColor ) );
			V( e_PrimitiveAddLine( dwPrimWireFlags, &pPortal->vCorners[ pPortal->nEdges[ 1 ][ 0 ] ], &pPortal->vCorners[ pPortal->nEdges[ 1 ][ 1 ] ], dwBorderColor ) );
			V( e_PrimitiveAddLine( dwPrimWireFlags, &pPortal->vCorners[ pPortal->nEdges[ 2 ][ 0 ] ], &pPortal->vCorners[ pPortal->nEdges[ 2 ][ 1 ] ], dwBorderColor ) );
			V( e_PrimitiveAddLine( dwPrimWireFlags, &pPortal->vCorners[ pPortal->nEdges[ 3 ][ 0 ] ], &pPortal->vCorners[ pPortal->nEdges[ 3 ][ 1 ] ], dwBorderColor ) );

			V( e_PrimitiveAddTri( dwPrimFlags, &pPortal->vCorners[ 0 ], &pPortal->vCorners[ 1 ], &pPortal->vCorners[ 2 ], dwColor ) );
			V( e_PrimitiveAddTri( dwPrimFlags, &pPortal->vCorners[ 0 ], &pPortal->vCorners[ 1 ], &pPortal->vCorners[ 3 ], dwColor ) );
			V( e_PrimitiveAddTri( dwPrimFlags, &pPortal->vCorners[ 0 ], &pPortal->vCorners[ 2 ], &pPortal->vCorners[ 3 ], dwColor ) );
			V( e_PrimitiveAddTri( dwPrimFlags, &pPortal->vCorners[ 1 ], &pPortal->vCorners[ 2 ], &pPortal->vCorners[ 3 ], dwColor ) );
		}
		break;
	default:
		// cheesy line thing instead of bothering to do coverage right now
		for ( int i = 0; i < pPortal->nCorners; i++ )
		{
			for ( int j = 0; j < pPortal->nCorners; j++ )
			{
				if ( i == j )
					continue;
				V( e_PrimitiveAddLine( 0, &pPortal->vCorners[ i ], &pPortal->vCorners[ j ], dwColor ) );
			}
		}
	}

	VECTOR vNormalEnd = pPortal->vCenter + ( pPortal->vNormal * 0.7f );
	V( e_PrimitiveAddLine( 0, &pPortal->vCenter, &vNormalEnd, 0x7fff0000 ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalDebugRenderAll()
{
	return;

	if ( ! e_GetRenderFlag( RENDER_FLAG_SHOW_PORTAL_INFO ) )
		return;

	PORTAL * pPortal = e_PortalGetFirstVisible();
	while ( pPortal )
	{
		e_PortalDebugRender( pPortal->nId );
		pPortal = e_PortalGetNextVisible( pPortal );
	}
}

