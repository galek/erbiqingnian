//----------------------------------------------------------------------------
// demolevel.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "definition.h"
#include "room.h"
#include "level.h"
#include "warp.h"
#include "demolevel.h"
#include "e_curve.h"
#include "movement.h"
#include "astar.h"
#include "room_path.h"
#include "e_primitive.h"
#include "e_math.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_menus.h"
#include "chb_timing.h"
#include "filepaths.h"
#include "prime.h"
#include "e_featureline.h"
#include "version.h"
#include "e_optionstate.h"
#include "e_caps.h"
#include "dungeon.h"
#include "gamelist.h"
#include "e_main.h"
#include "c_input.h"
#include "camera_priv.h"
#include "c_network.h"

using namespace FSSE;



#if ! ISVERSION(SERVER_VERSION) && ! ISVERSION(CLIENT_ONLY_VERSION)



#define MAX_DIST_BETWEEN_KNOTS			10.f
#define MIN_DIST_BETWEEN_KNOTS			2.0f
#define MIN_DIST_BETWEEN_KNOTS_SQR		(MIN_DIST_BETWEEN_KNOTS * MIN_DIST_BETWEEN_KNOTS)
#define MIN_PATH_NODE_RADIUS			1.7f


#define MAX_CONNECTED_ROOMS				6

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef const struct ROOM * LPCROOM;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct MY_ROOM_PATH_NODE_CONNECTION
{
	float				fRadius;
	float				fDistance;
	ROOM_PATH_NODE *	pNode;
	ROOM *				pRoom;
};

struct ROOM_CONNECTION_STATUS
{
	// for CHash
	int nId;
	ROOM_CONNECTION_STATUS * pNext;

	BOOL bTraversed;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static int sgnDemoLevel = INVALID_ID;
static ROOMID * sgpnRoomPath = NULL;
static int sgnRoomsInPath = 0;

static CURVE sgtDemoCameraPath;

static LPCROOM * sgppIterRooms = NULL;
static int sgnIterRoomCount = 0;
static int sgnIterRoomsTop = -1;

static TIME sgPathMoveStartTime;
static TIME sgPathDeltaTime;

static resizable_array<MY_ROOM_PATH_NODE_CONNECTION> sgtPathNodeConnectionList;
static int sgnPathNodeConnectionList = 0;

static FlyPath * sgpFlyPath = NULL;
static float64 sgfFlyPathLastTime = float64(0);

extern DEMO_LEVEL_STATUS gtDemoLevelStatus;

static CHash<ROOM_CONNECTION_STATUS>	sgtRoomConnectionStatusHash;

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

DEMO_LEVEL_DEFINITION * DemoLevelGetDefinition()
{
	if ( ! AppIsHellgate() )
		return NULL;

	if ( sgnDemoLevel == INVALID_ID )
	{
		const TCHAR * pszName = AppCommonGetDemoLevelName();
		if ( ! pszName || ! pszName[0] )
			return NULL;

		// Force allow asset updates while loading this definition
		BOOL bFAAU = AppTestDebugFlag( ADF_FORCE_ALLOW_ASSET_UPDATE );
		AppSetDebugFlag( ADF_FORCE_ALLOW_ASSET_UPDATE, TRUE );

		sgnDemoLevel = DefinitionGetIdByName( DEFINITION_GROUP_DEMO_LEVEL, pszName, -1, TRUE );

		// Restore the force allow asset update flag
		AppSetDebugFlag( ADF_FORCE_ALLOW_ASSET_UPDATE, bFAAU );
	}

	if ( sgnDemoLevel == INVALID_ID )
		return NULL;

	return (DEMO_LEVEL_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_DEMO_LEVEL, sgnDemoLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAddKnot( SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots, const VECTOR & vPos, VECTOR & vLastKnot, float fMinDistBetweenKnotsSqr = MIN_DIST_BETWEEN_KNOTS_SQR )
{
	// don't add new knots too close to the old ones
	if ( VectorDistanceSquared( vLastKnot, vPos ) < fMinDistBetweenKnotsSqr )
		return;
	vLastKnot = vPos;
	ArrayAddItem( pKnots, vPos );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAddFlyPoint( const VECTOR & vPos, VECTOR & vLastKnot )
{
	// don't add new knots too close to the old ones
	if ( VectorDistanceSquared( vLastKnot, vPos ) < MIN_DIST_BETWEEN_KNOTS_SQR )
		return;

	Quaternion tOrientation;
	tOrientation.v = vPos - vLastKnot;
	//VECTOR vDir = vPos - vLastKnot;
	//VectorNormalize( vDir );
	//VectorNormalize( tOrientation.v );
	tOrientation.w = 0.f;
	QuatNormalize( &tOrientation );
	vLastKnot = vPos;

	//QuatFromDirectionVector( tOrientation, vDir );

	sgpFlyPath->AddFlyPointAfterCurrent( *(FPVertex3*)&vPos, tOrientation );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFindWorkingPathNode( ROOM_PATH_NODE *& pPathNode )
{
#if HELLGATE_ONLY
	ASSERT_RETINVALIDARG( pPathNode );

	const ROOM_PATH_NODE * pStartNode = pPathNode;

	if ( pStartNode->fRadius >= MIN_PATH_NODE_RADIUS )
		return S_OK;

	int nClosest = -1;
	float fClosestDist = -1.f;

	int nBackupClosest = -1;
	float fBackupClosestRad = 0.f;

	for ( int i = 0; i < pStartNode->nConnectionCount; ++i )
	{
		ROOM_PATH_NODE_CONNECTION * pNodeConnection = &pStartNode->pConnections[i];
		ROOM_PATH_NODE * pConnectedNode = pNodeConnection->pConnection;

		if ( pConnectedNode->fRadius >= MIN_PATH_NODE_RADIUS )
		{
			if ( nClosest == -1 || pNodeConnection->fDistance < fClosestDist )
			{
				fClosestDist = pNodeConnection->fDistance;
				nClosest = i;
			}
		}

		if ( nBackupClosest == -1 || pConnectedNode->fRadius > fBackupClosestRad )
		{
			fBackupClosestRad = pConnectedNode->fRadius;
			nBackupClosest = i;
		}
	}

	if ( nClosest >= 0 )
	{
		ASSERT_RETFAIL( nClosest < pStartNode->nConnectionCount );
		pPathNode = pStartNode->pConnections[ nClosest ].pConnection;
		return S_OK;
	}

	if ( nBackupClosest >= 0 )
	{
		ASSERT_RETFAIL( nBackupClosest < pStartNode->nConnectionCount );
		pPathNode = pStartNode->pConnections[ nBackupClosest ].pConnection;
		return S_OK;
	}
#endif
	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sComparePathNodeConnections( const void * p1, const void * p2 )
{
	const MY_ROOM_PATH_NODE_CONNECTION * e1 = (const MY_ROOM_PATH_NODE_CONNECTION *)p1;
	const MY_ROOM_PATH_NODE_CONNECTION * e2 = (const MY_ROOM_PATH_NODE_CONNECTION *)p2;

	BOOL bBig1 = ( e1->fRadius >= MIN_PATH_NODE_RADIUS );
	BOOL bBig2 = ( e2->fRadius >= MIN_PATH_NODE_RADIUS );

	// if one is big enough, but not the other, sort by that
	if ( bBig1 && ! bBig2 )
		return -1;
	if ( bBig2 && ! bBig1 )
		return 1;

	if ( bBig1 )
	{
		// both connections are big enough, sort by distance
		if ( e1->fDistance < e2->fDistance )
			return -1;
		if ( e1->fDistance > e2->fDistance )
			return 1;
	}
	else
	{
		// both connections are less than min radius, sort by radius
		if ( e1->fRadius > e2->fRadius )
			return -1;
		if ( e1->fRadius < e2->fRadius )
			return 1;
	}

	return 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sPathNodeIterateConnections(
	ROOM * pStartRoom,
	ROOM_PATH_NODE * pStartNode )
{
#if HELLGATE_ONLY
	ASSERT_RETINVALIDARG(pStartRoom);
	ASSERT_RETINVALIDARG(pStartNode);

	for(int i=0; i<pStartNode->nConnectionCount; i++)
	{
		ROOM_PATH_NODE_CONNECTION * pConnection = &pStartNode->pConnections[i];
		if(pConnection->pConnection)
		{
			sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].fRadius		= pConnection->pConnection->fRadius;
			sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].fDistance	= pConnection->fDistance;
			sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].pNode		= pConnection->pConnection;
			sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].pRoom		= pStartRoom;
			sgnPathNodeConnectionList++;
		}
	}

	if((pStartNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pStartRoom, ROOM_EDGENODE_CONNECTIONS))
	{
		int nIndex = pStartNode->nEdgeIndex;
		EDGE_NODE_INSTANCE * pInstance = &pStartRoom->pEdgeInstances[nIndex];
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pStartRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
			int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
			ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
			if(pOtherRoomNode)
			{
				sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].fRadius		= pOtherRoomNode->fRadius;
				sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].fDistance	= pConnection->fDistance;
				sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].pNode		= pOtherRoomNode;
				sgtPathNodeConnectionList[ sgnPathNodeConnectionList ].pRoom		= pConnection->pRoom;
				sgnPathNodeConnectionList++;
			}
		}
	}
#endif
	return S_OK;
}


static PRESULT sFindWorkingPathNodeList( ROOM_PATH_NODE * pStartNode, ROOM * pStartRoom )
{
#if HELLGATE_ONLY
	ASSERT_RETINVALIDARG( pStartRoom );

	if ( pStartNode->fRadius >= MIN_PATH_NODE_RADIUS )
	{
		// Just use this node
		//V_RETURN( sgtPathNodeConnectionList.Resize( 1 ) );
		sgnPathNodeConnectionList = 0;
		//sgtPathNodeConnectionList[ 0 ].fRadius		= pOtherRoomNode->fRadius;
		//sgtPathNodeConnectionList[ 0 ].fDistance	= pConnection->fDistance;
		//sgtPathNodeConnectionList[ 0 ].pNode		= pOtherRoomNode;
		//sgtPathNodeConnectionList[ 0 ].pRoom		= pStartRoom;
		//sgnPathNodeConnectionList ;
		return S_OK;
	}

	int nAlloc = pStartNode->nConnectionCount;

	if( (pStartNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pStartRoom, ROOM_EDGENODE_CONNECTIONS) )
	{
		int nIndex = pStartNode->nEdgeIndex;
		EDGE_NODE_INSTANCE * pInstance = &pStartRoom->pEdgeInstances[nIndex];
		nAlloc += pInstance->nEdgeConnectionCount;
	}


	V_RETURN( sgtPathNodeConnectionList.Resize( nAlloc ) );
	sgnPathNodeConnectionList = 0;

	V_RETURN( sPathNodeIterateConnections( pStartRoom, pStartNode ) );

	MY_ROOM_PATH_NODE_CONNECTION * pBuf = &(sgtPathNodeConnectionList[0]);
	qsort( (void*)pBuf, sgnPathNodeConnectionList, sizeof(MY_ROOM_PATH_NODE_CONNECTION), sComparePathNodeConnections );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetNextWorkingPathNodeFromList( ROOM_PATH_NODE *& pPathNode, ROOM *& pRoom, int & nIndex )
{
	if ( nIndex == -1 && ( sgnPathNodeConnectionList == 0 || sgtPathNodeConnectionList.GetAllocElements() == 0 ) )
	{
		// Just use this path node
		nIndex = 0;
		return S_OK;
	}

	pPathNode = NULL;

	if ( nIndex >= sgnPathNodeConnectionList )
		return S_FALSE;
	if ( nIndex < 0 )
		nIndex = 0;

	MY_ROOM_PATH_NODE_CONNECTION * pRPNC = &(sgtPathNodeConnectionList[nIndex]);
	nIndex++;

	ASSERT_RETFAIL( pRPNC );
	pPathNode = pRPNC->pNode;
	pRoom =		pRPNC->pRoom;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static bool sRoomInIterList( LPCROOM pRoom )
{
	ASSERT_RETFALSE( pRoom );

	for ( int i = 0; i < sgnIterRoomsTop; ++i )
	{
		if ( sgppIterRooms[i] == pRoom )
			return true;
	}

	return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sRoomIterListPush( LPCROOM pRoom )
{
	ASSERT_RETURN( ! sRoomInIterList( pRoom ) );
	ASSERT_RETURN( sgnIterRoomsTop < sgnIterRoomCount );


	sgppIterRooms[ sgnIterRoomsTop ] = pRoom;
	sgnIterRoomsTop++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static LPCROOM sRoomIterListPop()
{
	ASSERT_RETNULL( sgnIterRoomsTop > 0 );

	sgnIterRoomsTop--;
	LPCROOM pRoom = sgppIterRooms[ sgnIterRoomsTop ];
	sgppIterRooms[ sgnIterRoomsTop ] = NULL;

	return pRoom;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static bool sFindEndRoom( LPCROOM pCur, LPCROOM pEnd, LPCROOM * ppRoomPath, size_t tRoomPathBytes )
{
	bool bResult = false;

	ASSERT_RETVAL( pCur != pEnd, false ); 

	ASSERT_RETVAL( pCur->ppConnectedRooms, false );

	ASSERT_RETVAL( ! sRoomInIterList( pCur ), false );

	sRoomIterListPush( pCur );

	for ( unsigned int c = 0; c < pCur->nConnectedRoomCount; ++c )
	{
		LPCROOM pNext = pCur->ppConnectedRooms[c];

		if ( sRoomInIterList( pNext ) )
			continue;

		if ( pNext == pEnd )
		{
			bResult = true;
			goto done;
		}

		if ( sFindEndRoom( pNext, pEnd, ppRoomPath, tRoomPathBytes ) )
		{
			// Found the end room, and this room is part of the path.  Prepend this room to the current path.
			size_t tMoveSize = tRoomPathBytes - sizeof(ROOM*);
			ASSERT( MemoryMove( &ppRoomPath[1], tMoveSize, &ppRoomPath[0], tMoveSize ) );
			ppRoomPath[0] = pCur;

			bResult = true;
			goto done;
		}

	}

done:
	ASSERT( pCur == sRoomIterListPop() );
	return bResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetConnectionPathNodePosition( const ROOM * pRoomFrom, const ROOM * pRoomTo, const VECTOR & vFromPos, VECTOR & vPos )
{
	ASSERT_RETINVALIDARG( pRoomFrom );
	ASSERT_RETINVALIDARG( pRoomTo );

	int nClosest = -1;
	float fClosestSqr = 0.f;

	for ( int c = 0; c < pRoomFrom->nConnectionCount; ++c )
	{
		if ( pRoomFrom->pConnections[ c ].pRoom == pRoomTo )
		{
			float fDistSqr = VectorDistanceSquared( vFromPos, pRoomFrom->pConnections[c].vBorderCenter );
			if ( fDistSqr < fClosestSqr || nClosest == -1 )
			{
				nClosest = c;
				fClosestSqr = fDistSqr;
			}
		}
	}

	if ( nClosest > -1 )
	{
		ASSERT_RETFAIL( pRoomTo->pConnections );
		ASSERT_RETFAIL( pRoomFrom->pConnections[nClosest].bOtherConnectionIndex < pRoomTo->nConnectionCount );
		vPos = pRoomTo->pConnections[ pRoomFrom->pConnections[nClosest].bOtherConnectionIndex ].vBorderCenter;
		return S_OK;
	}

	ASSERT_MSG( "Couldn't find other room in connection list!" )

	return E_FAIL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFindPath(
	GAME * pGame,
	struct PATH_STATE * pPathState,
	ASTAR_OUTPUT * tOutput,
	int & nPathNodeCount,
	ROOM * pSrcRoom,
	ROOM * pDestRoom,
	const VECTOR & vSrcPt,
	const VECTOR & vDestPt
	)
{
	pSrcRoom = RoomGetFromPosition( pGame, pSrcRoom, &vSrcPt );
	pDestRoom = RoomGetFromPosition( pGame, pDestRoom, &vDestPt );
	// Path between the two nodes
	ASSERT( BoundingBoxTestPoint( &pSrcRoom->tBoundingBoxWorldSpace,  &vSrcPt  ) );
	ASSERT( BoundingBoxTestPoint( &pDestRoom->tBoundingBoxWorldSpace, &vDestPt ) );
	ROOM_PATH_NODE * pSrcNode = RoomPathNodeGetNearestNodeToPoint( pGame, pSrcRoom, vSrcPt );
	ROOM_PATH_NODE * pDestNode = RoomPathNodeGetNearestNodeToPoint( pGame, pDestRoom, vDestPt );

	VECTOR vEndPt = vDestPt;

	nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	AStarCalculatePath( pGame, &nPathNodeCount, tOutput, pPathState, pSrcRoom, pSrcNode, pDestRoom, pDestNode, 0 );
	BOUNDED_WHILE( nPathNodeCount == 0, 100 )
	{
		// No path through the room, move the end 20% closer
		vEndPt = ( vEndPt + vSrcPt ) * 0.8f;
		pDestRoom = RoomGetFromPosition( pGame, pDestRoom, &vEndPt );
		pDestNode = RoomPathNodeGetNearestNodeToPoint( pGame, pDestRoom, vEndPt );
		ASSERT_CONTINUE( pDestNode );

		nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
		AStarCalculatePath( pGame, &nPathNodeCount, tOutput, pPathState, pSrcRoom, pSrcNode, pDestRoom, pDestNode, 0 );
	}


	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddKnotsFromPath(
	GAME * pGame,
	LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	const ASTAR_OUTPUT * tOutput,
	const int nPathNodeCount,
	const VECTOR & vPathNodeOffset,
	const float cfDistBetweenKnotsSqr,
	VECTOR & vLastKnot
	)
{
	// Turn the path into knots!
	for( int i = 0; i < nPathNodeCount; ++i )
	{
		ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[i].nRoomId );
		if( pNodeRoom )
		{
			ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[i].nPathNodeIndex );
			VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
			vPos += vPathNodeOffset;
			if ( cfDistBetweenKnotsSqr < VectorDistanceSquared( vPos, vLastKnot ) )
			{
				vLastKnot = vPos;
				ArrayAddItem( pKnots, vPos );
			}
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFilterKnotList(
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	float fMinFilterDist = 2.5f,
	float fDotLimit = -0.85f )
{
	//float fMinFilterDistSq = fMinFilterDist * fMinFilterDist;

	for ( unsigned int i = 1; i < pKnots.Count()-1; ++i )
	{
		VECTOR vP = pKnots[i - 1] - pKnots[i];
		float fDist = VectorNormalize( vP );
		if ( fDist > fMinFilterDist )
			continue;

		VECTOR vN = pKnots[i + 1] - pKnots[i];
		VectorNormalize( vN );

		float fDot = VectorDot( vP, vN );
		if ( fDot < fDotLimit )
		{
			// straight line -- remove it
			ArrayRemoveByIndex( pKnots, i );
			--i;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFilterKnotListWithMax(
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	float fMinFilterDist = 2.5f,
	float fMaxFilterDist = 5.0f,
	float fDotLimit = -0.85f )
{
	//float fMinFilterDistSq = fMinFilterDist * fMinFilterDist;

	for ( unsigned int i = 1; i < pKnots.Count()-1; ++i )
	{
		VECTOR vP = pKnots[i - 1] - pKnots[i];
		VectorNormalize( vP );
		VECTOR vN = pKnots[i + 1] - pKnots[i];
		float fDist = VectorNormalize( vN );
		if ( fDist > fMinFilterDist && fDist < fMaxFilterDist )
			continue;

		float fDot = VectorDot( vP, vN );
		if ( fDot < fDotLimit )
		{
			// straight line -- remove it
			ArrayRemoveByIndex( pKnots, i );
			--i;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sMassageKnotList(
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots )
{
	const float cfMinMassageDist = 7.f;
	const float cfMinMassageDistSq = cfMinMassageDist * cfMinMassageDist;

	for ( unsigned int i = 1; i < pKnots.Count()-1; ++i )
	{
		VECTOR vP = pKnots[i - 1] - pKnots[i];
		VectorNormalize( vP );
		VECTOR vN = pKnots[i + 1] - pKnots[i];
		float fDistSq = VectorLengthSquared( vN );
		if ( fDistSq < cfMinMassageDistSq )
		{
			//float fDist = SQRT_SAFE( fDistSq );
			pKnots[i - 1] -= vP * cfMinMassageDist;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sRoomGetLargestPathNode(
	GAME * pGame,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	ROOM * pRoom,
//	const VECTOR & vNear,
	VECTOR & vPathNodePos )
{
#if HELLGATE_ONLY
	ROOM_PATH_NODE * pPathNode = NULL;
	for ( unsigned int i = 0; i < pRoom->nPathNodeCount; ++i )
	{
		ROOM_PATH_NODE * pTestNode = RoomGetRoomPathNode( pRoom, i );
		if ( ! pPathNode || pTestNode->fRadius > pPathNode->fRadius )
			pPathNode = pTestNode;
	}

	ASSERT_RETFAIL( pPathNode );

	vPathNodePos = RoomPathNodeGetExactWorldPosition( pGame, pRoom, pPathNode );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddKnotsByLargestPathNodes(
	GAME * pGame,
	struct PATH_STATE * pPathState,
	ASTAR_OUTPUT * tOutput,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	ROOM * pSrcRoom,
	ROOM * pDestRoom,
	VECTOR v1,
	VECTOR v2,
	VECTOR & vLastKnot,
	float fAddKnotPerMeters = 3.f,
	float fHeightBonusFactor = 0.5f,
	float fMaxHeightBonus = 2.f,
	float fMinDistBetweenKnots = MIN_DIST_BETWEEN_KNOTS )
{

	PRESULT pr = S_FALSE;
#if HELLGATE_ONLY
	float fAddKnotPerMetersSq = fAddKnotPerMeters * fAddKnotPerMeters;
	float fMinDistBetweenKnotsSq = fMinDistBetweenKnots * fMinDistBetweenKnots;

	int nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pSrcRoom, pDestRoom, v1, v2 ) );


	for ( int i = 0; i < nPathNodeCount; ++i )
	{
		ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[i].nRoomId );

		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[i].nPathNodeIndex );

		V_CONTINUE( sFindWorkingPathNodeList( pPathNode, pNodeRoom ) );

		int nIndex = -1;
		sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
		while ( nIndex >= 0 && pPathNode )
		{
			if ( pPathNode->fRadius < MIN_PATH_NODE_RADIUS )
				break;

			VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
			float fHeightBonus = pPathNode->fHeight * fHeightBonusFactor;
			fHeightBonus = MIN( fHeightBonus, fMaxHeightBonus );
			vPos.z += fHeightBonus;
			//vPos += vPathNodeOffset;

			// Get the distance between the last and new nodes
			float fDistSqr = VectorDistanceSquared( vLastKnot, vPos );
			if ( fDistSqr > fAddKnotPerMetersSq )
			{
				sAddKnot( pKnots, vPos, vLastKnot, fMinDistBetweenKnotsSq );
				pr = S_OK;
				break;
			}

			sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
		}
	}
#endif
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddKnotForPathNode(
	GAME * pGame,
	ROOM_PATH_NODE * pPathNode,
	ROOM * pNodeRoom,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	VECTOR & vLastKnot,
	//float fAddKnotPerMeters,
	float fHeightBonusFactor,
	float fMaxHeightBonus )
{
#if HELLGATE_ONLY
	VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
	float fHeightBonus = pPathNode->fHeight * fHeightBonusFactor;
	fHeightBonus = MIN( fHeightBonus, fMaxHeightBonus );
	vPos.z += fHeightBonus;
	//vPos += vPathNodeOffset;

	// Get the distance between the last and new nodes
	//float fDistSqr = VectorDistanceSquared( vLastKnot, vPos );
	//if ( fDistSqr > fAddKnotPerMetersSq )
	{
		sAddKnot( pKnots, vPos, vLastKnot );
	}
#endif
	return S_OK;
}

static PRESULT sOutdoorNewAddKnotsThroughEdgeNodes(
	GAME * pGame,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	ROOM * pSrcRoom,
	ROOM * pDestRoom,
	VECTOR & vLastKnot,
	float fAddKnotPerMeters = 3.f,
	float fHeightBonusFactor = 0.5f,
	float fMaxHeightBonus = 2.f )
{
	//float fAddKnotPerMetersSq = fAddKnotPerMeters * fAddKnotPerMeters;
	REF( fAddKnotPerMeters );


	//int nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	//V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pSrcRoom, pDestRoom, v1, v2 ) );

	// Add a knot at the largest path node on the edge
	// Add a knot at the largest path node in the room

	ROOM * pNodeRoom = NULL;
	ROOM_PATH_NODE * pEdgeNode = RoomGetNearestPathNode( pGame, pDestRoom, vLastKnot, &pNodeRoom, NPN_EDGE_NODES_ONLY );
	if ( pEdgeNode )
	{
		V( sAddKnotForPathNode( pGame, pEdgeNode, pNodeRoom, pKnots, vLastKnot, fHeightBonusFactor, fMaxHeightBonus ) );
	}

	/*ROOM * */pNodeRoom = NULL;
	ROOM_PATH_NODE * pCenterNode = RoomGetNearestPathNode( pGame, pDestRoom, pDestRoom->vBoundingBoxCenterWorldSpace, &pNodeRoom );
	if ( pCenterNode )
	{
		V( sAddKnotForPathNode( pGame, pCenterNode, pNodeRoom, pKnots, vLastKnot, fHeightBonusFactor, fMaxHeightBonus ) );
	}


	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddKnotsThroughPath(
	GAME * pGame,
	struct PATH_STATE * pPathState,
	ASTAR_OUTPUT * tOutput,
	LEVEL * pLevel,
	const VECTOR & vPathNodeOffset,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	ROOM * pSrcRoom,
	ROOM * pDestRoom,
	VECTOR v1,
	VECTOR v2,
	VECTOR & vLastKnot )
{
#if HELLGATE_ONLY
	const float cfMinDistBetweenPathNodes = 5.f;
	const float cfMinDistBetweenPathNodesSqr = cfMinDistBetweenPathNodes * cfMinDistBetweenPathNodes;

	// Path between the two nodes

	int nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pSrcRoom, pDestRoom, v1, v2 ) );

	ASSERT( nPathNodeCount > 0 );

	//VECTOR vPrevPathNodePos = vLastKnot;

	for ( int i = 0; i < nPathNodeCount; ++i )
	{
		ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[i].nRoomId );
		//ASSERT_CONTINUE( pNodeRoom == pSrcRoom );

		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[i].nPathNodeIndex );

		// find a path node of at least a minimum radius (if possible)
		//V( sFindWorkingPathNode( pPathNode ) );

		V_CONTINUE( sFindWorkingPathNodeList( pPathNode, pNodeRoom ) );

		int nIndex = -1;
		sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
		while ( nIndex >= 0 && pPathNode )
		{
			if ( pPathNode->fRadius < MIN_PATH_NODE_RADIUS )
				break;

			VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
			float fHeightBonus = pPathNode->fHeight * 0.5f;
			fHeightBonus = MIN( fHeightBonus, 2.f );
			vPos.z += fHeightBonus;
			//vPos += vPathNodeOffset;

			// Get the distance between the last and new nodes
			float fDistSqr = VectorDistanceSquared( vLastKnot, vPos );
			if ( fDistSqr > cfMinDistBetweenPathNodesSqr )
			{
				sAddKnot( pKnots, vPos, vLastKnot );
				break;
			}

			sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
		}


		//if ( ! ClearLineOfSightBetweenPositions( pLevel, vLastKnot, vPos ) )
		//{
		//	// can no longer see the start of this segment!
		//	sAddKnot( pKnots, vPrevPathNodePos, vLastKnot );
		//	v1 = vLastKnot;
		//}

		//if ( ClearLineOfSightBetweenPositions( pLevel, vPos, v2 ) )
		//{
		//	// can see the end!
		//	sAddKnot( pKnots, vPrevPathNodePos, vLastKnot );
		//	sAddKnot( pKnots, v2, vLastKnot );
		//	return S_OK;
		//}

		//vPrevPathNodePos = vPos;
	}

	//V( sAddKnotsFromPath( pGame, pLevel, pKnots, tOutput, nPathNodeCount, vPathNodeOffset, cfDistBetweenKnotsSqr, vLastKnot ) );

	//ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[nPathNodeCount-1].nRoomId );
	////ASSERT_CONTINUE( pNodeRoom == pSrcRoom );
	//ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[nPathNodeCount-1].nPathNodeIndex );
	//VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
	//vPos += vPathNodeOffset;
	//v1 = vPos;
	//	} while ( ! ClearLineOfSightBetweenPositions( pLevel, v1, v2 ) );
#endif
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddKnotsByPath(
	GAME * pGame,
	struct PATH_STATE * pPathState,
	ASTAR_OUTPUT * tOutput,
	LEVEL * pLevel,
	const VECTOR & vPathNodeOffset,
	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
	ROOM * pSrcRoom,
	ROOM * pDestRoom,
	VECTOR v1,
	VECTOR v2,
	VECTOR & vLastKnot )
{
	int nSanity = 0;
//	do
//	{
		ASSERTX_RETFAIL( (nSanity++) < 1000, "Max loop iterations exceeded!" );

		// Path between the two nodes

		int nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
		V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pSrcRoom, pDestRoom, v1, v2 ) );

		ASSERT( nPathNodeCount > 0 );

		VECTOR vPrevPathNodePos = vLastKnot;

		for ( int i = 0; i < nPathNodeCount; ++i )
		{
			ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[i].nRoomId );
			//ASSERT_CONTINUE( pNodeRoom == pSrcRoom );

			ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[i].nPathNodeIndex );

			// find a path node of at least a minimum radius (if possible)
			V( sFindWorkingPathNode( pPathNode ) );

			VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
			vPos += vPathNodeOffset;

sAddKnot( pKnots, vPos, vLastKnot );

			//if ( ! ClearLineOfSightBetweenPositions( pLevel, vLastKnot, vPos ) )
			//{
			//	// can no longer see the start of this segment!
			//	sAddKnot( pKnots, vPrevPathNodePos, vLastKnot );
			//	v1 = vLastKnot;
			//}

			//if ( ClearLineOfSightBetweenPositions( pLevel, vPos, v2 ) )
			//{
			//	// can see the end!
			//	sAddKnot( pKnots, vPrevPathNodePos, vLastKnot );
			//	sAddKnot( pKnots, v2, vLastKnot );
			//	return S_OK;
			//}

			vPrevPathNodePos = vPos;
		}

		//V( sAddKnotsFromPath( pGame, pLevel, pKnots, tOutput, nPathNodeCount, vPathNodeOffset, cfDistBetweenKnotsSqr, vLastKnot ) );

		//ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[nPathNodeCount-1].nRoomId );
		////ASSERT_CONTINUE( pNodeRoom == pSrcRoom );
		//ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[nPathNodeCount-1].nPathNodeIndex );
		//VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
		//vPos += vPathNodeOffset;
		//v1 = vPos;
//	} while ( ! ClearLineOfSightBetweenPositions( pLevel, v1, v2 ) );

	return S_OK;
}



////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//
//static PRESULT sAddKnotsByRoom( GAME * pGame, struct PATH_STATE * pPathState, ASTAR_OUTPUT * tOutput, LEVEL * pLevel, const VECTOR & vPathNodeOffset, SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots, ROOM * pSrcRoom, VECTOR v1, VECTOR v2 )
//{
//	int nSanity = 0;
//	do
//	{
//		ASSERTX_RETFAIL( (nSanity++) < 1000, "Max loop iterations exceeded!" );
//
//
//		{
//			//if ( ! ClearLineOfSightBetweenPositions( pLevel, v1, vPos ) )
//			//{
//			//	// can no longer see the start of this segment!
//			//	if ( pPrevPathNode )
//			//	{
//			//		ArrayAddItem( pKnots, vPrevPathNodePos );
//			//		v1 = vPrevPathNodePos;
//			//		pPrevPathNode = NULL;
//			//		continue;
//			//	}
//			//}
//
//
//// USE HAPPY PLACES
//
//
//			if ( ClearLineOfSightBetweenPositions( pLevel, vPos, v2 ) )
//			{
//				// can see the end!
//				ArrayAddItem( pKnots, v2 );
//				return S_OK;
//			}
//
//			//pPrevPathNode = pPathNode;
//			//vPrevPathNodePos = vPos;
//		}
//
//		//V( sAddKnotsFromPath( pGame, pLevel, pKnots, tOutput, nPathNodeCount, vPathNodeOffset, cfDistBetweenKnotsSqr, vLastKnot ) );
//
//		ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[nPathNodeCount-1].nRoomId );
//		//ASSERT_CONTINUE( pNodeRoom == pSrcRoom );
//		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[nPathNodeCount-1].nPathNodeIndex );
//		VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
//		vPos += vPathNodeOffset;
//		v1 = vPos;
//
//		ArrayAddItem( pKnots, vPos );
//
//	} while ( ! ClearLineOfSightBetweenPositions( pLevel, v1, v2 ) );
//
//	return S_OK;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFindNextRoom(
	GAME * pGame,
	LEVEL * pLevel,
	ROOM* pSrcRoom,
	const VECTOR& vStartLoc,
	VECTOR& vDestLoc,
	const VECTOR& vPathNodeOffset,
	int& nDestRoom,
	const int nRoomsInPath,
	const float cfMaxDistBetweenKnots )
{
	float fDistSqr = -1.f;

	nDestRoom++;

	int nPrevDestRoom = -1;
	VECTOR vPrevDestLoc;

	// nDestRoom should be the first index into the room path array to look for a connection to

	BOUNDED_WHILE( nDestRoom < nRoomsInPath, 1000 )
	{
		vPrevDestLoc = vDestLoc;

		ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nDestRoom ] );
		if ( (nDestRoom + 1) < nRoomsInPath && nDestRoom > 1 )
		{
			ROOM * pPrevRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nDestRoom-1 ] );

			// find the connection
			V_CONTINUE( sGetConnectionPathNodePosition( pPrevRoom, pDestRoom, vStartLoc, vDestLoc ) );
			vDestLoc += vPathNodeOffset;
		}
		else
		{
			// last room -- use the center
			VECTOR vCenter = pDestRoom->tBoundingSphereWorldSpace.C;
			vDestLoc = vCenter;
			break;
		}

		fDistSqr = VectorDistanceSquared( vStartLoc, vDestLoc );

		//if ( nPrevDestRoom >= 0 && ! ClearLineOfSightBetweenPositions( pLevel, vStartLoc, vDestLoc ) )
		if ( nPrevDestRoom >= 0 && fDistSqr > ( cfMaxDistBetweenKnots * cfMaxDistBetweenKnots ) )
		{
			vDestLoc = vPrevDestLoc;
			nDestRoom = nPrevDestRoom;
			break;
		}


		nPrevDestRoom = nDestRoom;
		nDestRoom++;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFindBiggestPathNodeBetween( ROOM * pRoom, int nPathNode1, int nPathNode2 )
{
	ASSERT_RETFAIL( pRoom );

	for ( int i = 0; i < (int)pRoom->nPathNodeCount; ++i )
	{
		if ( i == nPathNode1 || i == nPathNode2 )
			continue;

		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pRoom, i );
		ASSERT_CONTINUE( pPathNode );

		//if ( pPathNode->fRadius )
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetRoomDestinationPos(
	GAME * pGame,
	const VECTOR & vStartLoc,
	int nDestRoom,
	const int nRoomsInPath,
	VECTOR & vNewKnotPos )
{
	ASSERT_RETINVALIDARG( nDestRoom < nRoomsInPath );

	int nConnectedRoom = MIN( nDestRoom+1, nRoomsInPath-1 );

	ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nDestRoom ] );
	ASSERT_RETFAIL( pDestRoom );

	if ( nDestRoom != nConnectedRoom )
	{
		ROOM * pConnectedRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nConnectedRoom ] );
		ASSERT_RETFAIL( pConnectedRoom );

		// find the connection
		V_RETURN( sGetConnectionPathNodePosition( pDestRoom, pConnectedRoom, vStartLoc, vNewKnotPos ) );


// TEMP
		vNewKnotPos.z += 1.f;

	}
	else
	{
		// last room -- use the center
		VECTOR vCenter = pDestRoom->tBoundingSphereWorldSpace.C;
		vNewKnotPos = vCenter;
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sFindNextRoomAndPath(
	GAME * pGame,
	struct PATH_STATE * pPathState,
	ASTAR_OUTPUT * tOutput,
	LEVEL * pLevel,
	ROOM* pSrcRoom,
	const VECTOR& vStartLoc,
	VECTOR& vDestLoc,
	const VECTOR& vPathNodeOffset,
	int& nStartRoom,
	int& nEndRoom,
	const int nRoomsInPath,
	const float cfMaxDistBetweenKnots )
{
	float fDistSqr = -1.f;


	// add a room if it isn't too far (and we aren't at the end)
	if ( nEndRoom >= (nRoomsInPath - 1) )
		return S_FALSE;		

	int nTestEndRoom = nEndRoom + 1;
	VECTOR vTestDestPos;
	V_RETURN( sGetRoomDestinationPos( pGame, vStartLoc, nTestEndRoom, nRoomsInPath, vTestDestPos ) );
	fDistSqr = VectorDistanceSquared( vStartLoc, vTestDestPos );
	if ( fDistSqr > ( cfMaxDistBetweenKnots * cfMaxDistBetweenKnots ) )
	{
		return S_FALSE;
	}


	// recursive call
	PRESULT pr = sFindNextRoomAndPath( pGame, pPathState, tOutput, pLevel, pSrcRoom, vStartLoc, vTestDestPos, vPathNodeOffset, nStartRoom, nTestEndRoom, nRoomsInPath, cfMaxDistBetweenKnots );
	if ( FAILED(pr) )
		return pr;


	// if result == S_OK then all done, just return S_OK
	if ( pr == S_OK )
	{
		nEndRoom = nTestEndRoom;
		vDestLoc = vTestDestPos;
		return pr;
	}


	// if result == S_FALSE then:
	if ( pr == S_FALSE )
	{
		//		try pathing
		//		if path succeeds return S_OK
		//		else return S_FALSE

		int nPathNodeCount = 0;
		ROOM * pStartRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nStartRoom ] );
		ROOM * pEndRoom   = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[ nTestEndRoom ] );

		V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pStartRoom, pEndRoom, vStartLoc, vTestDestPos ) );

		if ( nPathNodeCount > 0 )
		{
			nEndRoom = nTestEndRoom;
			vDestLoc = vTestDestPos;
			return S_OK;
		}
		else
			return S_FALSE;
	}

	ASSERT_MSG( "Shouldn't have gotten here!" );
	return E_FAIL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelGeneratePath_OUTDOOR()
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	LEVELID nLevel = AppGetCurrentLevel();
	LEVEL * pLevel = LevelGetByID( pGame, nLevel );
	ASSERT_RETFAIL( pLevel );

	VECTOR vStartPosition;
	VECTOR vStartDirection;
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	ROOM * pStartRoom = LevelGetStartPosition( pGame, GameGetControlUnit(pGame), nLevel, vStartPosition, vStartDirection, dwWarpFlags );
	ASSERT_RETFAIL( pStartRoom );

	ROOM * pEndRoom = NULL;
	float fMinDist = -1;

	ROOM ** ppRoomPath = (ROOM**)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETVAL( ppRoomPath, E_OUTOFMEMORY );

	sgppIterRooms = (LPCROOM*)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(LPCROOM) );
	sgnIterRoomCount = pLevel->m_nRoomCount;
	sgnIterRoomsTop = 0;

	// Find another room with a portal
	for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
		pIterRoom;
		pIterRoom = LevelGetNextRoom( pIterRoom ) )
	{
		if ( pStartRoom->idRoom == pIterRoom->idRoom )
			continue;

		int nWarps = RoomCountUnitsOfType( pIterRoom, UNITTYPE_WARP_LEVEL );
		if ( nWarps > 0 )
		{
			pEndRoom = pIterRoom;
			break;
		}
	}

	if ( ! pEndRoom )
	{
		// Find the farthest possible room to be the end room
		for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
			pIterRoom;
			pIterRoom = LevelGetNextRoom( pIterRoom ) )
		{
			if ( pStartRoom->idRoom == pIterRoom->idRoom )
				continue;
			float fDist = BoundingBoxManhattanDistance( &pStartRoom->tBoundingBoxWorldSpace, &pIterRoom->tBoundingBoxWorldSpace );
			if ( fMinDist >= 0.f && fDist < fMinDist )
				continue;
			if ( pIterRoom->nConnectedRoomCount == 0 )
				continue;

			fMinDist = fDist;
			pEndRoom = pIterRoom;
		}
	}
	ASSERT_RETFAIL( pEndRoom );

	bool bRet = sFindEndRoom( pStartRoom, pEndRoom, (LPCROOM*)ppRoomPath, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETFAIL( bRet );
	ASSERT_RETFAIL( pEndRoom );

	if ( sgpnRoomPath )
	{
		FREE( NULL, sgpnRoomPath );
	}

	sgpnRoomPath = (ROOMID*)MALLOC( NULL, pLevel->m_nRoomCount * sizeof(ROOMID) );
	ASSERT_RETVAL( sgpnRoomPath, E_OUTOFMEMORY );
	memset( sgpnRoomPath, -1, pLevel->m_nRoomCount * sizeof(ROOMID) );


	int n = 0;
	int p = 0;
	BOUNDED_WHILE( ppRoomPath[p], 10000 )
	{
		sgpnRoomPath[n] = ppRoomPath[p]->idRoom;
		trace( " %3d: [ %4d ]\n", n, sgpnRoomPath[n] );
		++n;
		++p;
	}
	sgnRoomsInPath = n;

	// generate CURVE nodes
	// use pathfinding code and follow it inside a room from one connection to another, traveling through the center of each connection if the next/prev "pathable" point is too far from it

	sgtDemoCameraPath.Init( NULL );

	SIMPLE_DYNAMIC_ARRAY<VECTOR3> pKnots;
	// pKnots is automatically destroyed at end of scope
	ArrayInit( pKnots, NULL, sgnRoomsInPath );


	//const int cnRoomsToPath = 1;
	//const float cfDistBetweenKnots = 3.5f;
	//const float cfDistBetweenKnotsSqr = cfDistBetweenKnots * cfDistBetweenKnots;
	VECTOR vLastKnot = VECTOR( 0, 0, 0 );
	const VECTOR vPathNodeOffset( 0.f, 0.f, 1.5f );
	const float cfMaxDistBetweenKnots = MAX_DIST_BETWEEN_KNOTS;


	struct PATH_STATE * pPathState = PathStructInit( pGame );
	ASSERT_GOTO( pPathState, cleanup );

	ASTAR_OUTPUT tOutput[DEFAULT_ASTAR_LIST_SIZE];

	ROOM * pSrcRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[0] );
	ASSERT_GOTO( pSrcRoom, cleanup );
	//ROOM_PATH_NODE * pCurrentNode = RoomPathNodeGetNearestNodeToPoint( pGame, pSrcRoom, vStartPosition );
	//ASSERT_GOTO( pCurrentNode, cleanup );
	// Ensure that the first knot goes through

	vLastKnot = vStartPosition;
	ArrayAddItem( pKnots, vStartPosition );

	for ( int nCurrentRoom = 0; nCurrentRoom < sgnRoomsInPath-1; /*++nCurrentRoom*/ )
	{
		VECTOR vDestLoc;

		//int nDestRoom = nCurrentRoom + cnRoomsToPath;
		//nDestRoom = MIN( nRoomsInPath - 1, nDestRoom );

		ROOM * pCurRoom  = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nCurrentRoom] );
		ASSERT_CONTINUE( pCurRoom );


		//int nNextRoom = nCurrentRoom + 1;
		//sFindNextRoom( pGame, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nNextRoom, sgnRoomsInPath, cfMaxDistBetweenKnots );

		int nEndRoom = nCurrentRoom + 1;
		// Get the first room's destination location as a fallback
		V( sGetRoomDestinationPos( pGame, vLastKnot, nEndRoom, sgnRoomsInPath, vDestLoc ) );


		// See if we can skip ahead in rooms
		V( sFindNextRoomAndPath( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nCurrentRoom, nEndRoom, sgnRoomsInPath, cfMaxDistBetweenKnots ) );

		ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nEndRoom] );

		VECTOR v1 = vLastKnot;
		VECTOR v2 = vDestLoc;

		//sAddKnotsByPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		//sAddKnotsThroughPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		sAddKnotsByLargestPathNodes( pGame, pPathState, tOutput, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot, 7.5f );



		// Prime this for the next loop iteration
		ASSERT( nCurrentRoom != nEndRoom );
		nCurrentRoom = nEndRoom;



		//REF( tOutput );
	}


	V( sFilterKnotList( pKnots, 2.5f, -0.7f ) );

	//V( sMassageKnotList( pKnots ) );

	//V( sFilterKnotList( pKnots ) );

	V( sgtDemoCameraPath.SetKnots( pKnots.GetPointer(0), pKnots.Count() ) );


cleanup:
	PathStructFree( pGame, pPathState );
	FREE( NULL, sgppIterRooms );
	sgppIterRooms = NULL;
	FREE( NULL, ppRoomPath );
#endif
	return S_OK;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct OUTDOOR_TARGET
{
	VECTOR vPos;
	ROOM * pRoom;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sOutdoorNewFindLastRoom(
	const OUTDOOR_TARGET & tStart,
	OUTDOOR_TARGET & tEnd,
	LEVEL * pLevel )
{
	float fMinDist = -1;

	// Find another room with a portal
	for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
		pIterRoom;
		pIterRoom = LevelGetNextRoom( pIterRoom ) )
	{
		if ( tStart.pRoom->idRoom == pIterRoom->idRoom )
			continue;

		if ( pIterRoom->idSubLevel != tStart.pRoom->idSubLevel )
			continue;

		int nWarps = RoomCountUnitsOfType( pIterRoom, UNITTYPE_WARP_LEVEL );
		if ( nWarps > 0 )
		{
			tEnd.pRoom = pIterRoom;
			break;
		}
	}

	if ( ! tEnd.pRoom )
	{
		// Find the farthest possible room to be the end room
		for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
			pIterRoom;
			pIterRoom = LevelGetNextRoom( pIterRoom ) )
		{
			if ( tStart.pRoom->idRoom == pIterRoom->idRoom )
				continue;
			if ( pIterRoom->idSubLevel != tStart.pRoom->idSubLevel )
				continue;
			float fDist = BoundingBoxManhattanDistance( &tStart.pRoom->tBoundingBoxWorldSpace, &pIterRoom->tBoundingBoxWorldSpace );
			if ( fMinDist >= 0.f && fDist < fMinDist )
				continue;
			if ( pIterRoom->nConnectedRoomCount == 0 )
				continue;

			fMinDist = fDist;
			tEnd.pRoom = pIterRoom;
		}
	}
	ASSERT_RETFAIL( tEnd.pRoom );
	ASSERT_RETFAIL( tStart.pRoom->idSubLevel == tEnd.pRoom->idSubLevel );

	tEnd.vPos = tEnd.pRoom->vBoundingBoxCenterWorldSpace;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sGetHashKey( int nRoomIDFrom, int nRoomIDTo )
{
	ASSERT_RETINVALID( 0 == ( nRoomIDFrom & 0xffff0000 ) );
	ASSERT_RETINVALID( 0 == ( nRoomIDTo   & 0xffff0000 ) );

	return nRoomIDTo & ( nRoomIDFrom << 16 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static ROOM_CONNECTION_STATUS * sGetHashEntry( int nRoomIDFrom, int nRoomIDTo )
{
	int nKey = sGetHashKey( nRoomIDFrom, nRoomIDTo );

	ASSERT_RETNULL( nKey != INVALID_ID );

	ROOM_CONNECTION_STATUS * pEntry = HashGet( sgtRoomConnectionStatusHash, nKey );
	if ( ! pEntry )
	{
		pEntry = HashAdd( sgtRoomConnectionStatusHash, nKey );
		pEntry->bTraversed = FALSE;
	}
	return pEntry;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct PROOM_SORT
{
	float fCombinedDot;
	ROOM* pRoom;
};
static int sComparePROOM_SORT( const void * p1, const void * p2 )
{
	const PROOM_SORT * pe1 = static_cast<const PROOM_SORT *>(p1);
	const PROOM_SORT * pe2 = static_cast<const PROOM_SORT *>(p2);

	if ( pe1->fCombinedDot > pe2->fCombinedDot )
		return -1;
	if ( pe2->fCombinedDot > pe1->fCombinedDot )
		return 1;
	return 0;
}


static bool sOutdoorNewPathToEndRoom(
	OUTDOOR_TARGET & tStart,
	OUTDOOR_TARGET & tEnd,
	LPCROOM * ppRoomPath,
	size_t tRoomPathBytes,
	ROOM * pCur,
	VECTOR & vCurDirection )
{


	bool bResult = false;

	ASSERT_RETVAL( pCur != tEnd.pRoom, false ); 

	ASSERT_RETVAL( pCur->ppConnectedRooms, false );

	// Prevent loopbacks
	ASSERT_RETVAL( ! sRoomInIterList( pCur ), false );
	sRoomIterListPush( pCur );


	VECTOR vPrevCenter = BoundingBoxGetCenter( &pCur->tBoundingBoxWorldSpace );
	VECTOR vEndDirection = tEnd.vPos - vPrevCenter;
	VectorNormalize( vEndDirection );


	// Sort all connected rooms by dot product to end room
	PROOM_SORT pConnRooms[ MAX_CONNECTED_ROOMS ];
	ZeroMemory( pConnRooms, sizeof(pConnRooms) );
	//PROOM_SORT * pConnRooms = (PROOM_SORT*)MALLOCZ( NULL, sizeof(PROOM_SORT) * pCur->nConnectedRoomCount );
	//ASSERT( pConnRooms );
	//if ( ! pConnRooms )
	//	goto done;
	int nConnRooms = 0;

	for ( unsigned int c = 0; c < pCur->nConnectedRoomCount; ++c )
	{
		ROOM* pNext = pCur->ppConnectedRooms[c];

		// Prevent loopbacks
		if ( sRoomInIterList( pNext ) )
			continue;

		VECTOR vNextCenter = BoundingBoxGetCenter( &pNext->tBoundingBoxWorldSpace );
		VECTOR vNextDirection = vNextCenter - vPrevCenter;
		VectorNormalize( vNextDirection );

		float fMvmtDot = VectorDot( vCurDirection, vNextDirection );
		float fEndDot = VectorDot( vEndDirection, vNextDirection );
		//if ( fEndDot > 0.f )
		//	fEndDot = 1.f;
		if ( fEndDot < 0.f )
			fMvmtDot = 0.f;
		pConnRooms[nConnRooms].fCombinedDot = fMvmtDot + fEndDot;
		// Include total number of connections as sorting criteria factor
		if ( fEndDot > 0.f )
		{
			pConnRooms[nConnRooms].fCombinedDot += (float)pNext->nConnectedRoomCount;
			pConnRooms[nConnRooms].fCombinedDot += (float)pNext->nEdgeNodeConnectionCount;
		}
		pConnRooms[nConnRooms].pRoom = pNext;
		++nConnRooms;
	}
	if ( nConnRooms <= 0 )
		goto done;

	qsort( pConnRooms, nConnRooms, sizeof(PROOM_SORT), sComparePROOM_SORT );

	for ( int i = 0; i < nConnRooms; ++i )
	{
		PROOM_SORT & tNext = pConnRooms[i];

		if ( tNext.pRoom == tEnd.pRoom )
		{
			bResult = true;
			goto done;
		}

		ROOM_CONNECTION_STATUS * pStatus = sGetHashEntry( pCur->idRoom, tNext.pRoom->idRoom );
		ASSERT_CONTINUE( pStatus );
		if ( pStatus->bTraversed )
			continue;

		VECTOR vNextCenter = BoundingBoxGetCenter( &tNext.pRoom->tBoundingBoxWorldSpace );

		VECTOR vNextDirection = vNextCenter - vPrevCenter;
		VectorNormalize( vNextDirection );

		if ( sOutdoorNewPathToEndRoom( tStart, tEnd, ppRoomPath, tRoomPathBytes, tNext.pRoom, vNextDirection ) )
		{
			// Found the end room, and this room is part of the path.  Prepend this room to the current path.
			size_t tMoveSize = tRoomPathBytes - sizeof(ROOM*);
			ASSERT( MemoryMove( &ppRoomPath[1], tMoveSize, &ppRoomPath[0], tMoveSize ) );
			ppRoomPath[0] = pCur;

			bResult = true;
			goto done;
		}

		pStatus->bTraversed = TRUE;
	}

done:
	//if ( pConnRooms )
	//	FREE( NULL, pConnRooms );
	ASSERT( pCur == sRoomIterListPop() );
	return bResult;

	

//	bool bResult = false;
//
//	ASSERT_RETVAL( pCur != pEnd, false ); 
//
//	ASSERT_RETVAL( pCur->ppConnectedRooms, false );
//
//	ASSERT_RETVAL( ! sRoomInIterList( pCur ), false );
//
//	sRoomIterListPush( pCur );
//
//	for ( unsigned int c = 0; c < pCur->nConnectedRoomCount; ++c )
//	{
//		LPCROOM pNext = pCur->ppConnectedRooms[c];
//
//		if ( sRoomInIterList( pNext ) )
//			continue;
//
//		if ( pNext == pEnd )
//		{
//			bResult = true;
//			goto done;
//		}
//
//		if ( sFindEndRoom( pNext, pEnd, ppRoomPath, tRoomPathBytes ) )
//		{
//			// Found the end room, and this room is part of the path.  Prepend this room to the current path.
//			size_t tMoveSize = tRoomPathBytes - sizeof(ROOM*);
//			ASSERT( MemoryMove( &ppRoomPath[1], tMoveSize, &ppRoomPath[0], tMoveSize ) );
//			ppRoomPath[0] = pCur;
//
//			bResult = true;
//			goto done;
//		}
//
//	}
//
//done:
//	ASSERT( pCur == sRoomIterListPop() );
//	return bResult;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//static PRESULT sOutdoorNewPathRooms(
//	OUTDOOR_TARGET & tStart,
//	OUTDOOR_TARGET & tEnd,
//	struct PATH_STATE * pPathState,
//	ASTAR_OUTPUT * tOutput,
//	SIMPLE_DYNAMIC_ARRAY<VECTOR3> & pKnots,
//	VECTOR & vLastKnot,
//	LEVEL * pLevel,
//	VECTOR & vStartDirection
//	)
//{
//	V_RETURN( sOutdoorNewFindLastRoom( tStart, tEnd, pLevel ) );
//	ASSERT_RETFAIL( tEnd.pRoom );
//
//	V_RETURN( sOutdoorNewPathToEndRoom( tStart, tEnd, pPathState, tOutput, pKnots, vLastKnot, tStart.pRoom, vStartDirection ) );
//
//	return S_OK;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelGeneratePath_OUTDOOR_NEW()
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	LEVELID nLevel = AppGetCurrentLevel();
	LEVEL * pLevel = LevelGetByID( pGame, nLevel );
	ASSERT_RETFAIL( pLevel );

	VECTOR vStartPosition;
	VECTOR vStartDirection;
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	ROOM * pStartRoom = LevelGetStartPosition( pGame, GameGetControlUnit(pGame), nLevel, vStartPosition, vStartDirection, dwWarpFlags );
	ASSERT_RETFAIL( pStartRoom );

	//ROOM * pEndRoom = NULL;
	//float fMinDist = -1;

	ROOM ** ppRoomPath = (ROOM**)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETVAL( ppRoomPath, E_OUTOFMEMORY );

	sgppIterRooms = (LPCROOM*)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(LPCROOM) );
	sgnIterRoomCount = pLevel->m_nRoomCount;
	sgnIterRoomsTop = 0;


	OUTDOOR_TARGET tStart;
	tStart.pRoom = pStartRoom;
	tStart.vPos = vStartPosition;
	OUTDOOR_TARGET tEnd;
	ZeroMemory( &tEnd, sizeof(OUTDOOR_TARGET) );
	
	V_RETURN( sOutdoorNewFindLastRoom( tStart, tEnd, pLevel ) );

	HashInit( sgtRoomConnectionStatusHash, NULL, 16 );

	V_RETURN( sOutdoorNewPathToEndRoom( tStart, tEnd, (LPCROOM*)ppRoomPath, pLevel->m_nRoomCount * sizeof(ROOM*), tStart.pRoom, vStartDirection ) );

	//bool bRet = sFindEndRoom( pStartRoom, pEndRoom, (LPCROOM*)ppRoomPath, pLevel->m_nRoomCount * sizeof(ROOM*) );
	//ASSERT_RETFAIL( bRet );
	//ASSERT_RETFAIL( pEndRoom );

	if ( sgpnRoomPath )
	{
		FREE( NULL, sgpnRoomPath );
	}

	sgpnRoomPath = (ROOMID*)MALLOC( NULL, pLevel->m_nRoomCount * sizeof(ROOMID) );
	ASSERT_RETVAL( sgpnRoomPath, E_OUTOFMEMORY );
	memset( sgpnRoomPath, -1, pLevel->m_nRoomCount * sizeof(ROOMID) );


	int n = 0;
	int p = 0;
	BOUNDED_WHILE( ppRoomPath[p], 10000 )
	{
		sgpnRoomPath[n] = ppRoomPath[p]->idRoom;
		trace( " %3d: [ %4d ]\n", n, sgpnRoomPath[n] );
		++n;
		++p;
	}
	sgnRoomsInPath = n;

	// generate CURVE nodes
	// use pathfinding code and follow it inside a room from one connection to another, traveling through the center of each connection if the next/prev "pathable" point is too far from it

	sgtDemoCameraPath.Init( NULL );

	SIMPLE_DYNAMIC_ARRAY<VECTOR3> pKnots;
	// pKnots is automatically destroyed at end of scope
	ArrayInit( pKnots, NULL, sgnRoomsInPath );


	//const int cnRoomsToPath = 1;
	//const float cfDistBetweenKnots = 3.5f;
	//const float cfDistBetweenKnotsSqr = cfDistBetweenKnots * cfDistBetweenKnots;
	VECTOR vLastKnot = VECTOR( 0, 0, 0 );
	const VECTOR vPathNodeOffset( 0.f, 0.f, 1.5f );
	//const float cfMaxDistBetweenKnots = MAX_DIST_BETWEEN_KNOTS;


	struct PATH_STATE * pPathState = PathStructInit( pGame );
	ASSERT_GOTO( pPathState, cleanup );

	ASTAR_OUTPUT tOutput[DEFAULT_ASTAR_LIST_SIZE];

	ROOM * pSrcRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[0] );
	ASSERT_GOTO( pSrcRoom, cleanup );
	//ROOM_PATH_NODE * pCurrentNode = RoomPathNodeGetNearestNodeToPoint( pGame, pSrcRoom, vStartPosition );
	//ASSERT_GOTO( pCurrentNode, cleanup );
	// Ensure that the first knot goes through

	vLastKnot = vStartPosition;
	ArrayAddItem( pKnots, vStartPosition );

	for ( int nCurrentRoom = 0; nCurrentRoom < sgnRoomsInPath-1; /*++nCurrentRoom*/ )
	{
		VECTOR vDestLoc;

		//int nDestRoom = nCurrentRoom + cnRoomsToPath;
		//nDestRoom = MIN( nRoomsInPath - 1, nDestRoom );

		ROOM * pCurRoom  = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nCurrentRoom] );
		ASSERT_CONTINUE( pCurRoom );


		//int nNextRoom = nCurrentRoom + 1;
		//sFindNextRoom( pGame, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nNextRoom, sgnRoomsInPath, cfMaxDistBetweenKnots );

		int nEndRoom = nCurrentRoom + 1;
		// Get the first room's destination location as a fallback
		//V( sGetRoomDestinationPos( pGame, vLastKnot, nEndRoom, sgnRoomsInPath, vDestLoc ) );
		V( sGetRoomDestinationPos( pGame, vLastKnot, nCurrentRoom, sgnRoomsInPath, vDestLoc ) );

		// See if we can skip ahead in rooms
//		V( sFindNextRoomAndPath( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nCurrentRoom, nEndRoom, sgnRoomsInPath, cfMaxDistBetweenKnots ) );

		ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nEndRoom] );


		//V( sOutdoorNewAddKnotsThroughEdgeNodes( pGame, pKnots, pCurRoom, pDestRoom, vLastKnot, 2.f, 0.5f, 3.5f ) );



		VECTOR v1 = vLastKnot;
		VECTOR v2 = vDestLoc;

		float fMinDistBetweenKnots = 5.f;
		float fMinDistBetweenKnotsSq = fMinDistBetweenKnots * fMinDistBetweenKnots;

		//sAddKnotsByPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		//sAddKnotsThroughPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		PRESULT pr = sAddKnotsByLargestPathNodes( pGame, pPathState, tOutput, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot, 3.0f, 0.5f, 3.5f, fMinDistBetweenKnots );
		//ASSERT( pr != S_FALSE );
		if ( pr == S_FALSE )
		{
			// Didn't add any knots, so add one now using the fallback position
			sAddKnot( pKnots, vDestLoc, vLastKnot, fMinDistBetweenKnotsSq );
		}



		// Prime this for the next loop iteration
		ASSERT( nCurrentRoom != nEndRoom );
		nCurrentRoom = nEndRoom;



		//REF( tOutput );
	}


	//V( sFilterKnotListWithMax( pKnots, 2.5f, 5.f, -0.7f ) );
	V( sFilterKnotList( pKnots, 6.f, -0.7f ) );

	//V( sMassageKnotList( pKnots ) );

	//V( sFilterKnotList( pKnots ) );

	V( sgtDemoCameraPath.SetKnots( pKnots.GetPointer(0), pKnots.Count() ) );


cleanup:
	PathStructFree( pGame, pPathState );
	HashFree( sgtRoomConnectionStatusHash );
	FREE( NULL, sgppIterRooms );
	sgppIterRooms = NULL;
	FREE( NULL, ppRoomPath );
#endif
	return S_OK;
}







//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelGeneratePath_INDOOR()
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	LEVELID nLevel = AppGetCurrentLevel();
	LEVEL * pLevel = LevelGetByID( pGame, nLevel );
	ASSERT_RETFAIL( pLevel );

	VECTOR vStartPosition;
	VECTOR vStartDirection;
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	ROOM * pStartRoom = LevelGetStartPosition( pGame, GameGetControlUnit(pGame), nLevel, vStartPosition, vStartDirection, dwWarpFlags );
	ASSERT_RETFAIL( pStartRoom );

	ROOM * pEndRoom = NULL;
	float fMinDist = -1;

	ROOM ** ppRoomPath = (ROOM**)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETVAL( ppRoomPath, E_OUTOFMEMORY );

	sgppIterRooms = (LPCROOM*)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(LPCROOM) );
	sgnIterRoomCount = pLevel->m_nRoomCount;
	sgnIterRoomsTop = 0;

	// Find another room with a portal
	for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
		pIterRoom;
		pIterRoom = LevelGetNextRoom( pIterRoom ) )
	{
		if ( pStartRoom->idRoom == pIterRoom->idRoom )
			continue;

		int nWarps = RoomCountUnitsOfType( pIterRoom, UNITTYPE_WARP_LEVEL );
		if ( nWarps > 0 )
		{
			pEndRoom = pIterRoom;
			break;
		}
	}

	if ( ! pEndRoom )
	{
		// Find the farthest possible room to be the end room
		for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
			pIterRoom;
			pIterRoom = LevelGetNextRoom( pIterRoom ) )
		{
			if ( pStartRoom->idRoom == pIterRoom->idRoom )
				continue;
			float fDist = BoundingBoxManhattanDistance( &pStartRoom->tBoundingBoxWorldSpace, &pIterRoom->tBoundingBoxWorldSpace );
			if ( fMinDist >= 0.f && fDist < fMinDist )
				continue;
			if ( pIterRoom->nConnectedRoomCount == 0 )
				continue;

			fMinDist = fDist;
			pEndRoom = pIterRoom;
		}
	}
	ASSERT_RETFAIL( pEndRoom );

	bool bRet = sFindEndRoom( pStartRoom, pEndRoom, (LPCROOM*)ppRoomPath, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETFAIL( bRet );
	ASSERT_RETFAIL( pEndRoom );

	if ( sgpnRoomPath )
	{
		FREE( NULL, sgpnRoomPath );
	}

	sgpnRoomPath = (ROOMID*)MALLOC( NULL, pLevel->m_nRoomCount * sizeof(ROOMID) );
	ASSERT_RETVAL( sgpnRoomPath, E_OUTOFMEMORY );
	memset( sgpnRoomPath, -1, pLevel->m_nRoomCount * sizeof(ROOMID) );


	int n = 0;
	int p = 0;
	BOUNDED_WHILE( ppRoomPath[p], 10000 )
	{
		sgpnRoomPath[n] = ppRoomPath[p]->idRoom;
		trace( " %3d: [ %4d ]\n", n, sgpnRoomPath[n] );
		++n;
		++p;
	}
	sgnRoomsInPath = n;

	// generate CURVE nodes
	// use pathfinding code and follow it inside a room from one connection to another, traveling through the center of each connection if the next/prev "pathable" point is too far from it

	sgtDemoCameraPath.Init( NULL );

	SIMPLE_DYNAMIC_ARRAY<VECTOR3> pKnots;
	// pKnots is automatically destroyed at end of scope
	ArrayInit( pKnots, NULL, sgnRoomsInPath );


	//const int cnRoomsToPath = 1;
	//const float cfDistBetweenKnots = 3.5f;
	//const float cfDistBetweenKnotsSqr = cfDistBetweenKnots * cfDistBetweenKnots;
	VECTOR vLastKnot = VECTOR( 0, 0, 0 );
	const VECTOR vPathNodeOffset( 0.f, 0.f, 1.5f );
	const float cfMaxDistBetweenKnots = MAX_DIST_BETWEEN_KNOTS;


	struct PATH_STATE * pPathState = PathStructInit( pGame );
	ASSERT_GOTO( pPathState, cleanup );

	ASTAR_OUTPUT tOutput[DEFAULT_ASTAR_LIST_SIZE];

	ROOM * pSrcRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[0] );
	ASSERT_GOTO( pSrcRoom, cleanup );
	//ROOM_PATH_NODE * pCurrentNode = RoomPathNodeGetNearestNodeToPoint( pGame, pSrcRoom, vStartPosition );
	//ASSERT_GOTO( pCurrentNode, cleanup );
	// Ensure that the first knot goes through

	vLastKnot = vStartPosition;
	ArrayAddItem( pKnots, vStartPosition );

	for ( int nCurrentRoom = 0; nCurrentRoom < sgnRoomsInPath-1; /*++nCurrentRoom*/ )
	{
		VECTOR vDestLoc;

		//int nDestRoom = nCurrentRoom + cnRoomsToPath;
		//nDestRoom = MIN( nRoomsInPath - 1, nDestRoom );

		ROOM * pCurRoom  = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nCurrentRoom] );
		ASSERT_CONTINUE( pCurRoom );


		//int nNextRoom = nCurrentRoom + 1;
		//sFindNextRoom( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nNextRoom, sgnRoomsInPath, cfMaxDistBetweenKnots );

		int nEndRoom = nCurrentRoom + 1;
		// Get the first room's destination location as a fallback
		V( sGetRoomDestinationPos( pGame, vLastKnot, nEndRoom, sgnRoomsInPath, vDestLoc ) );


		// See if we can skip ahead in rooms
		V( sFindNextRoomAndPath( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nCurrentRoom, nEndRoom, sgnRoomsInPath, cfMaxDistBetweenKnots ) );

		ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nEndRoom] );

		VECTOR v1 = vLastKnot;
		VECTOR v2 = vDestLoc;

		//sAddKnotsByPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		//sAddKnotsThroughPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		sAddKnotsByLargestPathNodes( pGame, pPathState, tOutput, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );



		// Prime this for the next loop iteration
		ASSERT( nCurrentRoom != nEndRoom );
		nCurrentRoom = nEndRoom;



		//REF( tOutput );
	}


	V( sFilterKnotList( pKnots ) );

	//V( sMassageKnotList( pKnots ) );

	//V( sFilterKnotList( pKnots ) );

	V( sgtDemoCameraPath.SetKnots( pKnots.GetPointer(0), pKnots.Count() ) );


cleanup:
	PathStructFree( pGame, pPathState );
	FREE( NULL, sgppIterRooms );
	sgppIterRooms = NULL;
	FREE( NULL, ppRoomPath );
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelGeneratePath_INDOOR_NEW()
{
#if HELLGATE_ONLY
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	LEVELID nLevel = AppGetCurrentLevel();
	LEVEL * pLevel = LevelGetByID( pGame, nLevel );
	ASSERT_RETFAIL( pLevel );



	DEMO_LEVEL_DEFINITION * pDef = DemoLevelGetDefinition();
	ASSERT_RETFAIL( pDef );
	if ( ! sgpFlyPath )
	{
		sgpFlyPath = MALLOC_NEW( NULL, FlyPath );
	}
	ASSERT_RETFAIL( sgpFlyPath );
	sgpFlyPath->InitPath();
	// THIS IS WRONG -- passing mps instead of multiplier!
	sgpFlyPath->SetSpeedFactor( pDef->fMetersPerSecond );



	VECTOR vStartPosition;
	VECTOR vStartDirection;
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	ROOM * pStartRoom = LevelGetStartPosition( pGame, GameGetControlUnit(pGame), nLevel, vStartPosition, vStartDirection, dwWarpFlags );
	ASSERT_RETFAIL( pStartRoom );

	ROOM * pEndRoom = NULL;
	float fMinDist = -1;

	ROOM ** ppRoomPath = (ROOM**)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETVAL( ppRoomPath, E_OUTOFMEMORY );

	sgppIterRooms = (LPCROOM*)MALLOCZ( NULL, pLevel->m_nRoomCount * sizeof(LPCROOM) );
	sgnIterRoomCount = pLevel->m_nRoomCount;
	sgnIterRoomsTop = 0;

	// Find another room with a portal
	for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
		pIterRoom;
		pIterRoom = LevelGetNextRoom( pIterRoom ) )
	{
		if ( pStartRoom->idRoom == pIterRoom->idRoom )
			continue;

		int nWarps = RoomCountUnitsOfType( pIterRoom, UNITTYPE_WARP_LEVEL );
		if ( nWarps > 0 )
		{
			pEndRoom = pIterRoom;
			break;
		}
	}

	if ( ! pEndRoom )
	{
		// Find the farthest possible room to be the end room
		for ( ROOM * pIterRoom = LevelGetFirstRoom( pLevel );
			pIterRoom;
			pIterRoom = LevelGetNextRoom( pIterRoom ) )
		{
			if ( pStartRoom->idRoom == pIterRoom->idRoom )
				continue;
			float fDist = BoundingBoxManhattanDistance( &pStartRoom->tBoundingBoxWorldSpace, &pIterRoom->tBoundingBoxWorldSpace );
			if ( fMinDist >= 0.f && fDist < fMinDist )
				continue;
			if ( pIterRoom->nConnectedRoomCount == 0 )
				continue;

			fMinDist = fDist;
			pEndRoom = pIterRoom;
		}
	}
	ASSERT_RETFAIL( pEndRoom );

	bool bRet = sFindEndRoom( pStartRoom, pEndRoom, (LPCROOM*)ppRoomPath, pLevel->m_nRoomCount * sizeof(ROOM*) );
	ASSERT_RETFAIL( bRet );
	ASSERT_RETFAIL( pEndRoom );

	if ( sgpnRoomPath )
	{
		FREE( NULL, sgpnRoomPath );
	}

	sgpnRoomPath = (ROOMID*)MALLOC( NULL, pLevel->m_nRoomCount * sizeof(ROOMID) );
	ASSERT_RETVAL( sgpnRoomPath, E_OUTOFMEMORY );
	memset( sgpnRoomPath, -1, pLevel->m_nRoomCount * sizeof(ROOMID) );


	int n = 0;
	int p = 0;
	BOUNDED_WHILE( ppRoomPath[p], 10000 )
	{
		sgpnRoomPath[n] = ppRoomPath[p]->idRoom;
		trace( " %3d: [ %4d ]\n", n, sgpnRoomPath[n] );
		++n;
		++p;
	}
	sgnRoomsInPath = n;

	// generate CURVE nodes
	// use pathfinding code and follow it inside a room from one connection to another, traveling through the center of each connection if the next/prev "pathable" point is too far from it



	//const int cnRoomsToPath = 1;
	//const float cfDistBetweenKnots = 3.5f;
	//const float cfDistBetweenKnotsSqr = cfDistBetweenKnots * cfDistBetweenKnots;
	VECTOR vLastKnot = VECTOR( 0, 0, 0 );
	const VECTOR vPathNodeOffset( 0.f, 0.f, 1.5f );
	const float cfMaxDistBetweenKnots = MAX_DIST_BETWEEN_KNOTS;
	Quaternion tOrientation;


	struct PATH_STATE * pPathState = PathStructInit( pGame );
	ASSERT_GOTO( pPathState, cleanup );

	ASTAR_OUTPUT tOutput[DEFAULT_ASTAR_LIST_SIZE];

	ROOM * pSrcRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[0] );
	ASSERT_GOTO( pSrcRoom, cleanup );
	//ROOM_PATH_NODE * pCurrentNode = RoomPathNodeGetNearestNodeToPoint( pGame, pSrcRoom, vStartPosition );
	//ASSERT_GOTO( pCurrentNode, cleanup );
	// Ensure that the first knot goes through

	vLastKnot = vStartPosition;
	//ArrayAddItem( pKnots, vStartPosition );


	QuatFromDirectionVector( tOrientation, vStartDirection );
	sgpFlyPath->AddFlyPointAfterCurrent( *(FPVertex3*)&vStartPosition, tOrientation );



	for ( int nCurrentRoom = 0; nCurrentRoom < sgnRoomsInPath-1; /*++nCurrentRoom*/ )
	{
		VECTOR vDestLoc;

		//int nDestRoom = nCurrentRoom + cnRoomsToPath;
		//nDestRoom = MIN( nRoomsInPath - 1, nDestRoom );

		ROOM * pCurRoom  = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nCurrentRoom] );
		ASSERT_CONTINUE( pCurRoom );


		//int nNextRoom = nCurrentRoom + 1;
		//sFindNextRoom( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nNextRoom, sgnRoomsInPath, cfMaxDistBetweenKnots );

		int nEndRoom = nCurrentRoom + 1;
		// Get the first room's destination location as a fallback
		V( sGetRoomDestinationPos( pGame, vLastKnot, nEndRoom, sgnRoomsInPath, vDestLoc ) );


		// See if we can skip ahead in rooms
		V( sFindNextRoomAndPath( pGame, pPathState, tOutput, pLevel, pCurRoom, vLastKnot, vDestLoc, vPathNodeOffset, nCurrentRoom, nEndRoom, sgnRoomsInPath, cfMaxDistBetweenKnots ) );

		ROOM * pDestRoom = RoomGetByID( pGame, (ROOMID)sgpnRoomPath[nEndRoom] );

		VECTOR v1 = vLastKnot;
		VECTOR v2 = vDestLoc;

		//sAddKnotsByPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		//sAddKnotsThroughPath( pGame, pPathState, tOutput, pLevel, vPathNodeOffset, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );
		//sAddKnotsByLargestPathNodes( pGame, pPathState, tOutput, pKnots, pCurRoom, pDestRoom, v1, v2, vLastKnot );


		float fAddKnotPerMeters = 2.f;
		float fAddKnotPerMetersSq = fAddKnotPerMeters * fAddKnotPerMeters;

		int nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
		V( sFindPath( pGame, pPathState, tOutput, nPathNodeCount, pCurRoom, pDestRoom, v1, v2 ) );

		for ( int i = 0; i < nPathNodeCount; ++i )
		{
			ROOM * pNodeRoom = RoomGetByID( pGame, tOutput[i].nRoomId );

			ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode( pNodeRoom, tOutput[i].nPathNodeIndex );

			V_CONTINUE( sFindWorkingPathNodeList( pPathNode, pNodeRoom ) );

			int nIndex = -1;
			sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
			while ( nIndex >= 0 && pPathNode )
			{
				if ( pPathNode->fRadius < MIN_PATH_NODE_RADIUS )
					break;

				VECTOR vPos = RoomPathNodeGetExactWorldPosition( pGame, pNodeRoom, pPathNode );
				float fHeightBonus = pPathNode->fHeight * 0.5f;
				fHeightBonus = MIN( fHeightBonus, 2.f );
				vPos.z += fHeightBonus;
				//vPos += vPathNodeOffset;

				// Get the distance between the last and new nodes
				float fDistSqr = VectorDistanceSquared( vLastKnot, vPos );
				if ( fDistSqr > fAddKnotPerMetersSq )
				{
					//sAddKnot( pKnots, vPos, vLastKnot );
					sAddFlyPoint( vPos, vLastKnot );
					break;
				}

				sGetNextWorkingPathNodeFromList( pPathNode, pNodeRoom, nIndex );
			}
		}



		// Prime this for the next loop iteration
		ASSERT( nCurrentRoom != nEndRoom );
		nCurrentRoom = nEndRoom;



		//REF( tOutput );
	}


	//V( sFilterKnotList( pKnots ) );

	//V( sMassageKnotList( pKnots ) );

	//V( sFilterKnotList( pKnots ) );

	//V( sgtDemoCameraPath.SetKnots( pKnots.GetPointer(0), pKnots.Count() ) );




cleanup:
	PathStructFree( pGame, pPathState );
	FREE( NULL, sgppIterRooms );
	sgppIterRooms = NULL;
	FREE( NULL, ppRoomPath );
#endif
#endif
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDemoLevelStartTimer()
{
	ASSERT_RETURN( gtDemoLevelStatus.RUNNING );

	DEMO_LEVEL_DEFINITION * pDef = DemoLevelGetDefinition();
	ASSERT_RETURN( pDef );

	sgPathMoveStartTime = AppCommonGetCurTime();
	sgPathDeltaTime = (TIME)(float)FLOOR((sgtDemoCameraPath.fCurveLength / pDef->fMetersPerSecond * (float)MSECS_PER_SEC) + 0.5f);

	// This zeroes the accumulation field in preparation for the first frame.
	CHB_TimingEndFrame();
	CHB_TimingBegin( CHB_TIMING_FRAME );

	if ( pDef->nCameraType == DLCT_INDOOR_NEW && sgpFlyPath )
		sgpFlyPath->StartFlight( (float64)e_GetCurrentTimeRespectingPauses() );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelGeneratePath()
{
	gtDemoLevelStatus.Init();

	DEMO_LEVEL_DEFINITION * pDef = DemoLevelGetDefinition();
	ASSERT_RETFAIL( pDef );
	ASSERT_RETFAIL( IS_VALID_INDEX( pDef->nCameraType, DLCT_MAX_TYPES ) );

	PRESULT pr = S_FALSE;

#if ! ISVERSION( CLIENT_ONLY_VERSION )

	switch ( pDef->nCameraType )
	{
	case DLCT_OUTDOOR:
	case DLCT_OUTDOOR_SLIDESHOW:
		pr = DemoLevelGeneratePath_OUTDOOR();
		break;
	case DLCT_INDOOR_NEW:
		pr = DemoLevelGeneratePath_INDOOR_NEW();
		break;
	case DLCT_OUTDOOR_NEW:
		pr = DemoLevelGeneratePath_OUTDOOR_NEW();
		break;
	case DLCT_INDOOR:
	case DLCT_INDOOR_SLIDESHOW:
	default:
		pr = DemoLevelGeneratePath_INDOOR();
	}

	sgtPathNodeConnectionList.Init( NULL );

	DemoLevelSetState( DEMO_LEVEL_STATUS::PRE_COUNTDOWN );

	GAME * pSrvGame = AppGetSrvGame();
	GAME * pCltGame = AppGetCltGame();
	if ( pSrvGame && pCltGame )
	{
		UNIT * pClientCU = GameGetControlUnit( pCltGame );
		if ( pClientCU )
		{
			//e_ModelSetDrawAndShadow(c_UnitGetModelId(pClientCU), TRUE);

			UNITID pCUID = UnitGetId( pClientCU );
			UNIT * pServerCU = UnitGetById( pSrvGame, pCUID );
			if ( pServerCU )
				UnitSetDontTarget( pServerCU, TRUE );
		}
	}

#if ISVERSION(DEVELOPMENT) && defined(DEMO_LEVEL_DISPLAY_DEBUG_INFO)
	e_SetRenderFlag( RENDER_FLAG_PATH_OVERVIEW, TRUE );
	e_SetRenderFlag( RENDER_FLAG_AXES, TRUE );
#endif

#endif // ! CLT_ONLY

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelFollowPath( VECTOR * pvOutPosition, VECTOR * pvOutDirection )
{
	ASSERT_RETURN( pvOutPosition );
	ASSERT_RETURN( pvOutDirection );

	// default these in case of early-out
	*pvOutPosition = VECTOR( 0,0,0 );
	*pvOutDirection = VECTOR( 0,1,0 );


	DEMO_LEVEL_DEFINITION * pDef = DemoLevelGetDefinition();
	if ( ! pDef )
		return;

	// for debug countdown output
	static int snLastSeconds = -1;

	switch (gtDemoLevelStatus.eState)
	{
	case DEMO_LEVEL_STATUS::PRE_COUNTDOWN:
		// Wait until the app is in game state
		//if ( AppGetState() == APP_STATE_IN_GAME )
		//	DemoLevelSetState( DEMO_LEVEL_STATUS::COUNTDOWN );
		break;
	case DEMO_LEVEL_STATUS::COUNTDOWN:
		{
			TIMEDELTA nSeconds = gtDemoLevelStatus.tTimeToStart - e_GetCurrentTimeRespectingPauses();

			if ( (int)nSeconds / MSECS_PER_SEC != snLastSeconds )
			{
				trace( "\n%d\n\n", nSeconds / MSECS_PER_SEC);
				snLastSeconds = (int)nSeconds / MSECS_PER_SEC;
			}
		}

		// check if we're done counting down
		if ( e_GetCurrentTimeRespectingPauses() < gtDemoLevelStatus.tTimeToStart )
			break;
		// Countdown is complete!
		DemoLevelSetState( DEMO_LEVEL_STATUS::READY_TO_RUN );
		// fall through
	case DEMO_LEVEL_STATUS::READY_TO_RUN:
		if ( e_GetUIOnlyRender() )
			return;
		DemoLevelSetState( DEMO_LEVEL_STATUS::RUNNING );
		// fall through
	case DEMO_LEVEL_STATUS::RUNNING:
		break;
	default:
		return;
	}

	if ( ! sgpFlyPath && sgtDemoCameraPath.nKnots <= 0 )
		return;

	UNIT * pControlUnit = GameGetControlUnit( AppGetCltGame() );
	ASSERT_RETURN( pControlUnit );

	float fTimePercent = 0.f;

	if ( sgPathDeltaTime > 0 )
	{
		fTimePercent = (float)( ( (double)AppCommonGetCurTime() - (double)sgPathMoveStartTime ) / (double)sgPathDeltaTime );
		fTimePercent = MIN( fTimePercent, 1.f );
	}

	ASSERT_RETURN( IS_VALID_INDEX( pDef->nCameraType, DLCT_MAX_TYPES ) );

	switch ( pDef->nCameraType )
	{
	case DLCT_OUTDOOR:
	case DLCT_OUTDOOR_NEW:
		{
			float fLookAheadMeters;
			fLookAheadMeters = 20.f;

			V( sgtDemoCameraPath.GetPosition( pvOutPosition, fTimePercent ) );
			float fForwardTime = MIN( fTimePercent + ( fLookAheadMeters / sgtDemoCameraPath.fCurveLength ), 1.f );
			V( sgtDemoCameraPath.GetPosition( pvOutDirection, fForwardTime ) );
			*pvOutDirection = *pvOutDirection - *pvOutPosition;

			//V( sgtDemoCameraPath.GetCatmullRomPosition( fTimePercent, pvOutPosition, pvOutDirection ) );
			//V( sgtDemoCameraPath.GetPositionNoCurve( pvOutPosition, fTimePercent, pvOutDirection ) );

			VectorNormalize( *pvOutDirection );

			//UnitSetPosition( pControlUnit, *pvOutPosition );
		}
		break;
	case DLCT_OUTDOOR_SLIDESHOW:
	case DLCT_INDOOR_SLIDESHOW:
		{
			V( sgtDemoCameraPath.GetSlideshowPosition( fTimePercent, pvOutPosition, pvOutDirection ) );

			VectorNormalize( *pvOutDirection );

			//UnitSetPosition( pControlUnit, *pvOutPosition );

		}
		break;
	case DLCT_INDOOR_NEW:
		{
			ASSERT_BREAK( sgpFlyPath );

			float64 fCurTime = (float64)e_GetCurrentTimeRespectingPauses();
			float64 fTotalPathTime = sgpFlyPath->GetTotalPathTime();
			float64 fStartTime = sgpFlyPath->GetStartTime();

			float64 fElapsed = fCurTime - fStartTime;
			fElapsed *= pDef->fMetersPerSecond;
			fCurTime = fElapsed + fStartTime;

			//float64 fFwdTime = fCurTime + 300 * pDef->fMetersPerSecond;

			fTimePercent = (float)( fElapsed / fTotalPathTime );
			fTimePercent = MIN( fTimePercent, 1.f );


			sgpFlyPath->GetPathPosition( (FPVertex3*)pvOutPosition, fCurTime );

			//sgpFlyPath->GetPathPosition( (FPVertex3*)pvOutDirection, fFwdTime );
			//*pvOutDirection = *pvOutDirection - *pvOutPosition;
			//VectorNormalize( *pvOutDirection );

			//pvOutDirection->Set( 1, 0, 0 );

			//sgfFlyPathLastTime = fCurTime;

			Quaternion tOrientation;
			//Quaternion tInvOrientation;
			MATRIX mOrientation;
			//MATRIX mAdjOrientation;
			MATRIX mTranslation;
			MATRIX mWorldToView;
			MATRIX mViewToWorld;

			//VECTOR vInvPos( -*pvOutPosition );

			//tOrientation.Set( *pvOutDirection, 0.f );
			//tOrientation.Set( 1.f, 0.f, 0.f, 0.f );
			//EulerToQuat( tOrientation, *pvOutDirection );

			// Point back to the origin
			//*pvOutDirection = -*pvOutPosition;
			//VectorNormalize( *pvOutDirection );

			//QuatFromDirectionVector( tOrientation, *pvOutDirection );

			sgpFlyPath->GetPathOrientation( &tOrientation, fCurTime );
			QuatNormalize( &tOrientation );


			// Rotate forward and up by orientation
			//VECTOR vForward( 1, 0, 0 );
			////VECTOR vUp( 0, 0, 1 );
			//Quaternion tVec;
			//Quaternion tRotated;

			//tVec.Set( vForward, 0 );
			//QuatMul( &tOrientation, &tVec, &tRotated );
			////vForward = tRotated.v;

			//tOrientation = tRotated;
			//QuatNormalize( &tOrientation );

			//tVec.Set( vUp, 0 );
			//QuatMul( &tVec, &tOrientation, &tRotated );
			//vUp = tRotated.v;


			//QuatToMatrix( &tOrientation, mOrientation );

			//tInvOrientation.Set( -tOrientation.v, tOrientation.w );
			MatrixFromQuaternion( &mOrientation, &tOrientation );

			//VECTOR vPos(0,0,0);
			//VECTOR vUp(0,0,1);
			//MatrixFromVectors( mOrientation, vPos, vUp, *pvOutDirection );
			//MatrixTranspose( &mOrientation, &mOrientation );


			//MATRIX mBasis;
			//CameraGetBasisOrientation( &mBasis );

			//mAdjOrientation.Identity();
			//mAdjOrientation.SetRight( *(VECTOR*)mOrientation.m[0] );
			//mAdjOrientation.SetForward( *(VECTOR*)mOrientation.m[2] );
			//mAdjOrientation.SetUp( *(VECTOR*)mOrientation.m[1] );
			//MatrixInverse( &mAdjOrientation, &mOrientation );


			// Try combining our orientation and translation matrices
			MatrixTranslation( &mTranslation, pvOutPosition );
			//MatrixMultiply( &mOrientation, &mBasis, &mOrientation );
			MatrixMultiply( &mViewToWorld, &mOrientation, &mTranslation );
			MatrixInverse( &mWorldToView, &mViewToWorld );



			// Make it straight from the vectors
			//MatrixFromVectors( mOrientation, *pvOutPosition, vUp, *pvOutDirection );
			//MatrixMultiply( &mOrientation, &mOrientation, &mBasis );
			//MatrixInverse( &mWorldToView, &mOrientation );
			//MatrixInverse( &mViewToWorld, &mWorldToView );



			// Try just making a view matrix
			//VECTOR vAt = *pvOutPosition + vForward;
			//MatrixMakeView( mWorldToView, *pvOutPosition, vAt, vUp );
			//MatrixInverse( &mViewToWorld, &mWorldToView );



			VECTOR vLook( 0, 0, 1 );
			VECTOR vLookAt;
			VECTOR vDirection;
			//MatrixMultiply( pvOutDirection, &vForward, &mOrientation );
			MatrixMultiply( &vLookAt, &vLook, &mViewToWorld );
			vDirection = vLookAt - *pvOutPosition;
			VectorNormalize( vDirection );

			e_SetCurrentViewOverrides( &mWorldToView, pvOutPosition, &vDirection, &vLookAt );
		}
		break;
	case DLCT_INDOOR:
	default:
		{
			float fLookAheadMeters;
			fLookAheadMeters = 12.f;

			V( sgtDemoCameraPath.GetPosition( pvOutPosition, fTimePercent ) );
			float fForwardTime = MIN( fTimePercent + ( fLookAheadMeters / sgtDemoCameraPath.fCurveLength ), 1.f );
			V( sgtDemoCameraPath.GetPosition( pvOutDirection, fForwardTime ) );
			*pvOutDirection = *pvOutDirection - *pvOutPosition;

			//V( sgtDemoCameraPath.GetCatmullRomPosition( fTimePercent, pvOutPosition, pvOutDirection ) );
			//V( sgtDemoCameraPath.GetPositionNoCurve( pvOutPosition, fTimePercent, pvOutDirection ) );

			VectorNormalize( *pvOutDirection );

			//UnitSetPosition( pControlUnit, *pvOutPosition );
			//UnitSetFaceDirection( pControlUnit, *pvOutDirection );

			//float fSpeedMult = 1.f;
			//V( sgtDemoCameraPath.GetSpeedMult( fTimePercent, fSpeedMult ) );



			//float fTime = 

			//sgfDemoCameraTime += 0.0001f * fSpeedMult;
		}
		break;
	}

	if ( pControlUnit->pRoom )
	{
		VECTOR vUp( 0, 0, 1 );
		DWORD dwUnitWarpFlags = 0;
		SETBIT( dwUnitWarpFlags, UW_PRESERVE_CAMERA_BIT );

		// Fails because pControlUnit doesn't have a level. :(
		UnitWarp( pControlUnit, pControlUnit->pRoom, *pvOutPosition, *pvOutDirection, vUp, dwUnitWarpFlags );
	} else
	{
		// just set position
		UnitSetPosition( pControlUnit, *pvOutPosition );
	}

	if ( fTimePercent >= 1.f )
	{
		DemoLevelSetState( DEMO_LEVEL_STATUS::SHOW_STATS );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sAddStatsLine(
	UI_TEXTBOX * pTextbox,
	char *& pszFile,
	int & nFileCurLen,
	int & nFileMaxLen,
	WCHAR * pwszLine )
{
	UITextBoxAddLine(pTextbox, pwszLine);

	int nLineLen = PStrLen( pwszLine );

	if ( ( (nLineLen * 2) + nFileCurLen + 1 ) >= nFileMaxLen )
	{
		// realloc the file
		int nMinBigger = (nLineLen * 2) + 1;
		int nBuckets = nMinBigger / 512;
		int nNewLen = nFileMaxLen + (nBuckets + 1) * 512;
		pszFile = (char*)REALLOC( NULL, pszFile, nNewLen );
		ASSERT_RETFAIL( pszFile );
		nFileMaxLen = nNewLen;
	}

	PStrCvt( pszFile + nFileCurLen, pwszLine, nFileMaxLen - nFileCurLen - 1 );
	memset( pszFile + nFileCurLen + nLineLen, 0, (nLineLen * 2) + 1 );
	PStrCvtToCRLF( pszFile + nFileCurLen, (nLineLen * 2) + 1 );
	//PStrCat( pszFile, "\r\n", nFileMaxLen - nFileCurLen - 1 );

	// After CRLF, new line length
	nLineLen = PStrLen( pszFile + nFileCurLen );

	nFileCurLen += nLineLen;

	return S_OK;
}

static PRESULT sDemoLevelCreateStatsUI()
{
	DEMO_LEVEL_DEFINITION * pDef = DemoLevelGetDefinition();
	ASSERT_RETFAIL( pDef );

	PRESULT pr = E_FAIL;

	e_SetRenderFlag( RENDER_FLAG_PATH_OVERVIEW, FALSE );

	// Populate the final stats screen.
	e_SetUIOnlyRender( TRUE );
	UIComponentActivate( UICOMP_DEMO_RUN_STATS_PANEL, TRUE, 0, NULL, FALSE, FALSE, TRUE, TRUE );
	UIComponentSetVisibleByEnum( UICOMP_DEMO_RUN_STATS_PANEL, TRUE );


	UI_COMPONENT * pTextboxComp = UIComponentGetByEnum( UICOMP_DEMO_RUN_STATS_DATA );
	ASSERT_RETFAIL( pTextboxComp );
	UI_TEXTBOX * pTextbox = UICastToTextBox( pTextboxComp );
	ASSERT_RETFAIL( pTextbox );

	const int cnStatsLen = 2048;
	WCHAR wszTxt[ cnStatsLen ];


	int nFileMaxLen = cnStatsLen;
	int nFileCurLen = 0;
	char * pszFile = (char*)MALLOCZ( NULL, sizeof(char) * nFileMaxLen );


	PStrPrintf( wszTxt, cnStatsLen, L"%-20s: %S\n",
		L"Demo Name",			pDef->tHeader.pszName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen, L"%-20s: %s\n\n",
		L"Result",				gtDemoLevelStatus.bInterrupted ? L"Interrupted" : L"Completed" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	DWORD dwSeed = DungeonGetSeed( AppGetCltGame() );

	const DRLG_DEFINITION * pDRLGDef = NULL;
	if ( pDef->nDRLGDefinition != INVALID_LINK )
		pDRLGDef = DRLGDefinitionGet( pDef->nDRLGDefinition );

	//const LEVEL_DEFINITION * pLevelDef = NULL;
	//if ( pDef->nLevelDefinition != INVALID_LINK )
	//	pLevelDef = LevelDefinitionGet( pDef->nLevelDefinition );

	PStrPrintf( wszTxt, cnStatsLen, L"%-20s: %d.%d.%d.%d\n%-20s: %d\n%-20s: %d\n%-20s: %s\n%-20s: %S\n%-20s: %u\n\n\n",
		L"Build Version",		TITLE_VERSION,PRODUCTION_BRANCH_VERSION,RC_BRANCH_VERSION,MAIN_LINE_VERSION,
		L"Demo Version",		DEMO_LEVEL_VERSION,
		L"Camera Type",			pDef->nCameraType,
		L"Level",				LevelDefinitionGetDisplayName( pDef->nLevelDefinition ),
		L"Dungeon Override",	pDRLGDef ? pDRLGDef->pszName : "none",
		L"Layout Seed",			dwSeed );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );


	WCHAR wszDash[] = L"-----------------------------------------------------------------------------------------------------------";

	PStrPrintf( wszTxt, cnStatsLen, L"%-14s %-7s %-7s %-7s %7s\n",
		L"Timer",	L"Min ms",		L"Max ms",	 L"Avg ms",		L"Avg FPS");
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen, L"%-15.12ls%-8.6ls%-8.6ls%-8.6ls%7.7ls\n",
		wszDash,	wszDash,	wszDash,	wszDash,	wszDash		);
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );


	PStrPrintf( wszTxt, cnStatsLen, L"%-15s%-8d%-8d%-8d%7.1f\n",
		L"Render (CPU)",
		gtDemoLevelStatus.tStats.dwMinFrameCPUMS,
		gtDemoLevelStatus.tStats.dwMaxFrameCPUMS,
		(DWORD)( (double) gtDemoLevelStatus.tStats.dwTotalCPUMS / (double) gtDemoLevelStatus.tStats.dwTotalFrames ),
		(float)((double) gtDemoLevelStatus.tStats.dwTotalFrames / ( (double) gtDemoLevelStatus.tStats.dwTotalCPUMS / 1000.0 ) ) );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	float fGPUFPS = ( gtDemoLevelStatus.tStats.dwTotalGPUMS > 0 )
						?	(float)((double) gtDemoLevelStatus.tStats.dwTotalFrames / ( (double) gtDemoLevelStatus.tStats.dwTotalGPUMS / 1000.0 ) )
						:	0.f;
	PStrPrintf( wszTxt, cnStatsLen, L"%-15s%-8d%-8d%-8d%7.1f\n",
		L"Render (GPU)",
		gtDemoLevelStatus.tStats.dwMinFrameGPUMS,
		gtDemoLevelStatus.tStats.dwMaxFrameGPUMS,
		(DWORD)( (double) gtDemoLevelStatus.tStats.dwTotalGPUMS / (double) gtDemoLevelStatus.tStats.dwTotalFrames ),
		fGPUFPS );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen, L"%-15s%-8d%-8d%-8d%7.1f\n",
		L"Frame",
		gtDemoLevelStatus.tStats.dwMinFrameMS,
		gtDemoLevelStatus.tStats.dwMaxFrameMS,
		(DWORD)( (double) gtDemoLevelStatus.tStats.dwTotalMS / (double) gtDemoLevelStatus.tStats.dwTotalFrames ),
		(float)((double) gtDemoLevelStatus.tStats.dwTotalFrames / ( (double) gtDemoLevelStatus.tStats.dwTotalMS / 1000.0 ) ) );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );


	const OptionState * pState = e_OptionState_GetActive();

	OptionState * pCurrentState = 0;
	FeatureLineMap * pCurrentFeatureMap = 0;
	V(e_OptionState_CloneActive(&pCurrentState))
	V(e_FeatureLine_OpenMap(pCurrentState, &pCurrentFeatureMap))


	PStrPrintf( wszTxt, cnStatsLen,		L"\n\n%-20s: %S %S\n",
		L"Engine", APP_PLATFORM_NAME, e_GetTargetName() );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );
		
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Display Mode", e_GetDisplaySettingsString() );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"V-Sync", pState->get_bFlipWaitVerticalRetrace() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Triple-Buffer", pState->get_bTripleBuffer() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Dynamic Lights", pState->get_bDynamicLights() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Anisotropic", pState->get_bAnisotropic() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Trilinear", pState->get_bTrilinear() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"Enhanced Weather", pState->get_bAsyncEffects() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",
		L"DX10 Smoke", pState->get_bFluidEffects() ? "yes" : "no" );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );



	char szStopName[16];
	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("SHDR"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Shader Quality", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("TLOD"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Texture Detail", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("SHDW"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Shadow Quality", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("MSAA"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Antialiasing", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("MLOD"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Model Detail", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("DIST"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"View Distance", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );

	V_GOTO( cleanup, e_FeatureLine_GetSelectedStopInternalName( MAKEFOURCCSTR("WLIM"), pCurrentFeatureMap, szStopName, 16 ) );
	PStrPrintf( wszTxt, cnStatsLen,		L"%-20s: %S\n",	L"Outfit Limit", szStopName );
	V_GOTO( cleanup, sAddStatsLine( pTextbox, pszFile, nFileCurLen, nFileMaxLen, wszTxt ) );


	OS_PATH_CHAR szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];


	SYSTEMTIME tTime;
	GetLocalTime(&tTime);

	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, OS_PATH_TEXT("%s%s_%04d%02d%02d_%02d%02d%02d.%s"), FilePath_GetSystemPath( FILE_PATH_LOG ), OS_PATH_TEXT("demostats"), tTime.wYear, tTime.wMonth, tTime.wDay, tTime.wHour, tTime.wMinute, tTime.wSecond, OS_PATH_TEXT("txt") );

	if ( FileSave( szFilePath, pszFile, nFileCurLen ) )
	{
		PStrPrintf( wszTxt, cnStatsLen, L"\n\nStats saved to:  %s", szFilePath );
		UITextBoxAddLine(pTextbox, wszTxt);
	}


	pr = S_OK;
cleanup:
	if ( pszFile )
		FREE( NULL, pszFile );

	return pr;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelCleanup()
{
	sgtPathNodeConnectionList.Destroy();

	sgtDemoCameraPath.Free();

	if ( sgpnRoomPath )
	{
		FREE( NULL, sgpnRoomPath );
		sgpnRoomPath = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelSetState( DEMO_LEVEL_STATUS::STATE eState )
{
	if ( gtDemoLevelStatus.eState != eState && eState == DEMO_LEVEL_STATUS::SHOW_STATS )
	{
		// switching to SHOW_STATS
		if ( sgpFlyPath )
			sgpFlyPath->EndFlight();

		sDemoLevelCreateStatsUI();
		DemoLevelCleanup();
	}

	if ( gtDemoLevelStatus.eState != eState && eState == DEMO_LEVEL_STATUS::PRE_COUNTDOWN )
	{
		// switching to PRE-COUNTDOWN
	}

	if ( gtDemoLevelStatus.eState != eState && eState == DEMO_LEVEL_STATUS::COUNTDOWN )
	{
		// switching to COUNTDOWN
		gtDemoLevelStatus.tTimeToStart = e_GetCurrentTimeRespectingPauses();
		TIMEDELTA tCountdown = (TIMEDELTA)( MSECS_PER_SEC * DEMO_LEVEL_COUNTDOWN_SECONDS );
		gtDemoLevelStatus.tTimeToStart += tCountdown;
	}
	
	if ( gtDemoLevelStatus.eState != eState && eState == DEMO_LEVEL_STATUS::RUNNING )
	{
		// switching to RUNNING
		e_SetSolidColorRenderEnabled( FALSE );
		sDemoLevelStartTimer();
		gtDemoLevelStatus.Reset();

		// Set the player model back to nodraw and noshadow
		UNIT * pControlUnit = GameGetControlUnit( AppGetCltGame() );
		if ( pControlUnit )
		{
			int nModelID = c_UnitGetModelIdThirdPerson( pControlUnit );
			e_ModelSetFlagbit( nModelID, MODEL_FLAGBIT_NODRAW, TRUE );
			e_ModelSetFlagbit( nModelID, MODEL_FLAGBIT_NOSHADOW, TRUE );
		}
	}

	e_DemoLevelSetState( eState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelDebugGetRoomPath( ROOMID *& pRoomPath, int & nRoomCount )
{
	pRoomPath = sgpnRoomPath;
	nRoomCount = sgnRoomsInPath;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT DemoLevelDebugRenderPath( const VECTOR2 & vWCenter, float fScale, const VECTOR2 & vSCenter )
{
#if ! ISVERSION(SERVER_VERSION) && defined(DEMO_LEVEL_DISPLAY_DEBUG_INFO)
	DEMO_LEVEL_DEFINITION * pDemoLevel = DemoLevelGetDefinition();
	if ( ! pDemoLevel )
		return S_FALSE;

	if ( ! e_GetRenderFlag( RENDER_FLAG_PATH_OVERVIEW ) )
		return S_FALSE;

	float fDepth = 0.1f;
	DWORD dwLineFlags = PRIM_FLAG_RENDER_ON_TOP;
	float fRadius = fScale * 1.5f;
	DWORD dwKnotColor  = 0xffffff00;
	DWORD dwLineColor  = 0xff00ffff;
	DWORD dwCurveColor = 0xff00ff00;

	if ( gtDemoLevelStatus.eState == DEMO_LEVEL_STATUS::COUNTDOWN )
	{
		const int nBufLen = 2048;
		WCHAR wszStats[nBufLen];
		TIMEDELTA nSeconds = gtDemoLevelStatus.tTimeToStart - e_GetCurrentTimeRespectingPauses();
		PStrPrintf( wszStats, nBufLen, 
			L"%d",
			nSeconds / MSECS_PER_SEC );
		VECTOR vPos( 0.0f, 0.0f, 0.001f );
		e_UIAddLabel( wszStats, &vPos, FALSE, 3.f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT );

		return S_OK;
	}

	if ( sgpFlyPath )
	{
		sgpFlyPath->DrawFlyPath( vWCenter, fScale, vSCenter );

		const int nBufLen = 2048;
		WCHAR wszStats[nBufLen];
		int nInterval;
		sgpFlyPath->GetPathInterval( nInterval, sgfFlyPathLastTime );
		PStrPrintf( wszStats, nBufLen, 
			L"Segments: %d\n"
			L"Current:  %d\n",
			(DWORD)sgpFlyPath->GetTotalPathTime(),
			nInterval );
		VECTOR vPos( 0.97f, 0.97f, 0.001f );
		e_UIAddLabel( wszStats, &vPos, FALSE, 5.f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT );

		return S_OK;
	}

	if ( ! sgtDemoCameraPath.pKnots )
		return S_FALSE;

	for ( int i = 0; i < sgtDemoCameraPath.nKnots; ++i )
	{
		VECTOR2 vC = *(VECTOR2*)&sgtDemoCameraPath.pKnots[ i ].pos;
		vC -= vWCenter;
		vC *= fScale;
		vC += vSCenter;
		e_PrimitiveAddCircle( dwLineFlags, &vC, fRadius, dwKnotColor, fDepth );

		if ( i > 0 )
		{
			VECTOR2 vP = *(VECTOR2*)&sgtDemoCameraPath.pKnots[ i-1 ].pos;
			vP -= vWCenter;
			vP *= fScale;
			vP += vSCenter;
			e_PrimitiveAddLine( dwLineFlags, &vP, &vC, dwLineColor, fDepth );
		}
	}

	float fPath = 0.f;
	float fStep = 0.001f;
	VECTOR2 vP = *(VECTOR2*)&sgtDemoCameraPath.pKnots[ 0 ].pos;
	vP -= vWCenter;
	vP *= fScale;
	vP += vSCenter;

	while ( fPath <= 1.f )
	{
		VECTOR vPos;
		V( sgtDemoCameraPath.GetPosition( &vPos, fPath ) );
		VECTOR2 vN = *(VECTOR2*)&vPos;
		vN -= vWCenter;
		vN *= fScale;
		vN += vSCenter;
		e_PrimitiveAddLine( dwLineFlags, &vP, &vN, dwCurveColor, fDepth );

		vP = vN;
		fPath += fStep;
	}

#endif // ! SERVER && DEMO_LEVEL_DISPLAY_DEBUG_INFO

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelInterrupt()
{
	if ( gtDemoLevelStatus.eState == DEMO_LEVEL_STATUS::SHOW_STATS )
	{
		// Just exit
		AppSwitchState(APP_STATE_EXIT);
		//PostQuitMessage( 0 );
	}

	gtDemoLevelStatus.bInterrupted = TRUE;
	DemoLevelSetState( DEMO_LEVEL_STATUS::SHOW_STATS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelLoadScreenFadeCallback()
{
	//V( e_BeginFade( FADE_IN, UI_POSTLOAD_FADEIN_MS, 0x00000000 ) );
	e_SetSolidColorRenderEnabled( TRUE );
	e_SetSolidColorRenderColor( 0x00000000 );
	DemoLevelSetState( DEMO_LEVEL_STATUS::COUNTDOWN );
	g_UI.m_eLoadScreenState = LOAD_SCREEN_CAN_HIDE;
	UIHideLoadingScreen( FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNIT * DemoLevelInitializePlayer()
{
	GAME * pGame = AppGetSrvGame();

	WCHAR strName[] = L"BenchmarkPlayer";
	unsigned int playerClass = 0;		// Male Guardian
	unsigned int numClasses = ExcelGetNumRows(pGame, DATATABLE_PLAYERS);
	playerClass = PIN(playerClass, (unsigned int)0, (unsigned int)(numClasses - 1));

	GAMECLIENT * pClient = ClientGetById( pGame, AppGetClientId() );
	ASSERT_RETNULL( s_MakeNewPlayer( pGame, pClient, playerClass, strName) );

	return ClientGetControlUnit( pClient, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DemoLevelLoadDefinition(LPCSTR filename)
{
	DefinitionGetIdByFilename( DEFINITION_GROUP_DEMO_LEVEL, filename, -1, TRUE );
}





#else	// SERVER_VERSION || CLIENT_ONLY_VERSION


DEMO_LEVEL_DEFINITION * DemoLevelGetDefinition()									{ return NULL; }
void DemoLevelFollowPath( VECTOR * , VECTOR * )										{}
void DemoLevelCleanup()																{}
PRESULT DemoLevelGeneratePath()														{ return S_FALSE; }
PRESULT DemoLevelDebugGetRoomPath( ROOMID *&, int & )								{ return S_FALSE; }
PRESULT DemoLevelDebugRenderPath( const VECTOR2 &, float, const VECTOR2 & )			{ return S_FALSE; }
void DemoLevelSetState( FSSE::DEMO_LEVEL_STATUS::STATE )							{}
void DemoLevelInterrupt()															{}
void DemoLevelLoadScreenFadeCallback()												{}
UNIT * DemoLevelInitializePlayer()													{ return NULL; }
void DemoLevelLoadDefinition(LPCSTR)												{}


#endif	// SERVER_VERSION || CLIENT_ONLY_VERSION