//----------------------------------------------------------------------------
// astar.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//
// Originally written by Dave Brevik, adapted by Guy Somberg
// Data structures modified by Robert Donald
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "astar.h"

#include "array.h"
#include "room.h"
#include "room_path.h"
#include "game.h"
#include "gamerand.h"
#include "unit_priv.h"
#include "movement.h"
#include "performance.h"
#include "appcommon.h"
#include "heap.h"
#include "skills.h"
#include "states.h"

#if ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "astar.cpp.tmh"
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PATHING_ALGORITHM_CONTEXT
{
	GAME * game;
	UNIT * unit;
	int * nPathNodeCount;
	ASTAR_OUTPUT * pPathNodes;
	ROOM * pSourceRoom;
	ROOM_PATH_NODE * pSourcePathNode;
	ROOM * pDestinationRoom;
	ROOM_PATH_NODE * pDestinationNode;
	BOOL bAcceptPartialSuccess;
	CHash<PATH_NODE_SHOULD_TEST> * tHash;
	int nMaxPathNodeCount;
	int nPotentiallyCollidableCount;
	UNIT * pPotentiallyCollidableSet[MAX_TARGETS_PER_QUERY];
	DWORD dwFlags;
};

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAKE_DUAL_POINTER_HASHKEY(p1, p2) ( UINT64(p1)*UINT64(p1)+UINT64(p2) )

//---------------------------------------------------------------------------------
// a* types

struct ASTAR_NODE
{
	ROOM *				pRoom;
	ROOM_PATH_NODE *	pRoomPathNode;
	VECTOR				vWorldPosition;
	
	float				f;
	float				g;
	float				h;
	ASTAR_NODE *		pParent;

	BOOL				bClosed; //RJD added
	UINT64				id; //RJD added for hash
	ASTAR_NODE *		next; //RJD added for hash
};

BOOL CompareNodeF(ASTAR_NODE *a, ASTAR_NODE *b)
{
	return a->f < b->f;
}

struct ASTAR_STATE
{
	int nOpenListSize;
	int nClosedListSize;
	ASTAR_NODE tDestination;
	GAME * pGame;

	Heap<ASTAR_NODE, 256, CompareNodeF> OpenListHeap; //rjd added
	//TODO: move from hardcoded 256 to macro for next highest power of 2 of DEFAULT_ASTAR_LIST_SIZE
	HASH_NOALLOC_NOREMOVE
		<ASTAR_NODE, UINT64, DEFAULT_ASTAR_LIST_SIZE*4 + 1, DEFAULT_ASTAR_LIST_SIZE*2> 
		NodeListHash; //rjd added

	//int nBestSolutionIndex;
	ASTAR_NODE * pBestSolution;

	ASTAR_STATE()
		:nOpenListSize(0), nClosedListSize(0), 
		pGame(NULL),
		pBestSolution(NULL)
	{
		structclear(tDestination);
		NodeListHash.Init();
		OpenListHeap.Init();
	}
};

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------
static void sGetWorldPosition(
	ROOM * pRoom,
	const VECTOR * pVectorIn,
	VECTOR * pVectorOut)
{
	MatrixMultiply(pVectorOut, pVectorIn, &pRoom->tWorldMatrix);
}

static void sGetWorldPosition(
	ROOM * pRoom,
	ROOM_PATH_NODE * pNodeIn,
	VECTOR * pVectorOut)
{
	*pVectorOut = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoom), pRoom, pNodeIn);
}

static float sManhattenDistance(
	const VECTOR & v1,
	const VECTOR & v2)
{
	return fabs(v1.fX - v2.fX) + fabs(v1.fY - v2.fY) + fabs(v1.fZ - v2.fZ);
}

static float sHeuristic(
	const VECTOR & v1,
	const VECTOR & v2)
{
	if(AppIsTugboat())
	{
		return VectorDistanceSquaredXY(v1, v2);
	}
	else
	{
		return sManhattenDistance(v1, v2);
	}
}

static BOOL sShouldTest(
	ROOM * pRoom,
	ROOM_PATH_NODE * pNode,
	ROOM_PATH_NODE_CONNECTION * pConnection,
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	if(!pRoom || !pNode)
	{
		return FALSE;
	}

	//Height is never used in Tugboat, and getting it is expensive.  Only get it for hellgate.
	//Note: we should optimize the UnitGetCollisionHeight function so it doesn't repeatedly hit excel.
	float fUnitHeight = 0.0f;
	if(!AppIsTugboat() && tContext.unit) fUnitHeight = UnitGetCollisionHeight(tContext.unit);

	int nKey = RoomGetId(pRoom) << 16 | pNode->nIndex;
	PATH_NODE_SHOULD_TEST * pShouldtest = HashGet(*tContext.tHash, nKey);
	if(pShouldtest)
	{
		//trace("Cache hit!\n");
		ASSERT(pShouldtest->idRoom == RoomGetId(pRoom));
		ASSERT(pShouldtest->nPathNodeIndex == pNode->nIndex);
		if(!pShouldtest->bResult)
		{
			return FALSE;
		}
	}
	else
	{
		//trace("Cache miss!\n");
		BOOL bResult = TRUE;
#if HELLGATE_ONLY
		if(!AppIsTugboat())
		{
			if (tContext.unit && UnitGetChecksHeightWhenPathing(tContext.unit) && (pNode->fHeight < fUnitHeight))
			{
				bResult = FALSE;
			}

			/*
			// GS - 4-23-2007
			// Removing this check because it's causing more problems than it solves, even with large monsters.
			// The problem manifests with the radius calculation at the bottom of ramps
			if (bResult && UnitGetChecksRadiusWhenPathing(tContext.unit) && pNode->fRadius < UnitGetPathingCollisionRadius(tContext.unit))
			{
				bResult = FALSE;
			}
			// */
		}
#endif
		//*
		if(bResult)
		{
			// check for blocked path nodes
			DWORD dwBlockedNodeFlags = 0;
			if (s_RoomNodeIsBlockedIgnore( pRoom, pNode->nIndex, dwBlockedNodeFlags, tContext.unit ) != NODE_FREE)
			{
				bResult = FALSE;
			}
		}
		// */

		/*
		for(int i=0; i<tContext.nPotentiallyCollidableCount; i++)
		{
			if(tContext.pPotentiallyCollidableSet[i])
			{
				BOOL bWouldCollide = UnitIsInRange(tContext.pPotentiallyCollidableSet[i], RoomPathNodeGetExactWorldPosition(tContext.game, pRoom, pNode), UnitGetPathingCollisionRadius(tContext.unit), NULL);
				if(bWouldCollide)
				{
					bResult = FALSE;
					break;
				}
			}
		}
		// */

		//*
		pShouldtest = HashAdd(*tContext.tHash, nKey);
		ASSERT(pShouldtest);
		pShouldtest->bResult = bResult;
		pShouldtest->idRoom = RoomGetId(pRoom);
		pShouldtest->nPathNodeIndex = pNode->nIndex;
		// */

		if(!bResult)
		{
			return FALSE;
		}
	}
#if HELLGATE_ONLY
	if(!AppIsTugboat())
	{
		if(pConnection && pConnection->fHeight > 0.0f && tContext.unit && UnitGetChecksHeightWhenPathing(tContext.unit) && pConnection->fHeight < fUnitHeight)
		{
			return FALSE;
		}
	}
#endif
	return TRUE;
}

static BOOL sShouldTestEdge(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_PATH_NODE * pNodeThis,
	EDGE_NODE_INSTANCE* pInstance,
	int & nEdgeConnectionIndex,
	PATH_NODE_EDGE_CONNECTION * pConnection)
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pRoom);
	ASSERT_RETFALSE(pNodeThis);
	ASSERT_RETFALSE(pInstance);
	ASSERT_RETFALSE(nEdgeConnectionIndex >= 0 && nEdgeConnectionIndex < pInstance->nEdgeConnectionCount);
	ASSERT_RETFALSE(pConnection);
	ASSERT_RETFALSE(pConnection->pRoom);

	if(PathNodeEdgeConnectionTestTraversed(pConnection))
	{
		return TRUE;
	}

	PathNodeEdgeConnectionSetTraversed(pConnection, TRUE);

	VECTOR vWorldNodePosThis = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pNodeThis);
	int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
	VECTOR vWorldNodePosOther = RoomPathNodeGetExactWorldPosition(pGame, pConnection->pRoom, pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex]);
	
	// Do a ray test
	VECTOR vRayTestPos = vWorldNodePosThis;
	vRayTestPos.fZ += 0.25f;
	VECTOR vRayTestDir = vWorldNodePosOther - vRayTestPos;
	VectorNormalize(vRayTestDir);
	float fDistance = VectorLength(vRayTestPos - vWorldNodePosOther);
	if ( AppIsHellgate() )
	{
		float fCollideLength = LevelLineCollideLen(pGame, RoomGetLevel(pRoom), vRayTestPos, vRayTestDir, fDistance);

		if (fCollideLength + 0.05 < fDistance)
		{
			if(nEdgeConnectionIndex < pInstance->nEdgeConnectionCount - 1)
			{
				MemoryCopy(&pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + nEdgeConnectionIndex], sizeof(PATH_NODE_EDGE_CONNECTION), 
							&pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + pInstance->nEdgeConnectionCount - 1], sizeof(PATH_NODE_EDGE_CONNECTION));
				nEdgeConnectionIndex--;
			}
			pInstance->nEdgeConnectionCount--;
			return FALSE;
		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------------

static BOOL add_open_list(
	ASTAR_STATE & tState,
	ROOM * pRoom,
	ROOM_PATH_NODE * pRoomPathNode,
	ASTAR_NODE * pParent,
	float g )
{
	if (tState.nOpenListSize >= DEFAULT_ASTAR_LIST_SIZE - 1)
	{
		return FALSE;
	}
	UINT64 nKey = MAKE_DUAL_POINTER_HASHKEY(pRoom, pRoomPathNode);

	//Add the node to the visited list.
	ASTAR_NODE *pNode = tState.NodeListHash.Add(nKey);

	if(!pNode) return FALSE;

	pNode->pRoom = pRoom;
	pNode->pRoomPathNode = pRoomPathNode;
	sGetWorldPosition(pRoom, pRoomPathNode, &pNode->vWorldPosition);
	pNode->pParent = pParent;
	pNode->g = g;
	pNode->h = sHeuristic(pNode->vWorldPosition, tState.tDestination.vWorldPosition);
	pNode->f = pNode->g + pNode->h;
	pNode->bClosed = FALSE;
	//Add the node to the open list.
	ASSERT_RETFALSE( HeapAdd(tState.OpenListHeap, pNode) );
	tState.nOpenListSize++;

	return TRUE;
}

//---------------------------------------------------------------------------------

static ASTAR_NODE * find_lowest_open(
	ASTAR_STATE & tState)
{
	ASTAR_NODE * pNode = tState.OpenListHeap.PopMinNode();

	ASSERT( pNode );
	return pNode;
}

//---------------------------------------------------------------------------------

static ASTAR_NODE * move_to_closed(
	ASTAR_STATE & tState,
	ASTAR_NODE * pOpenNode )
{
	//Node was already removed from heap when we got it,
	//so let's just set its state to closed and check if we're done.
	if (tState.nClosedListSize >= DEFAULT_ASTAR_LIST_SIZE - 1)
	{
		return NULL;
	}

	//set node to closed
	pOpenNode->bClosed = TRUE;

	tState.nOpenListSize--;

	tState.nClosedListSize++;

	ASTAR_NODE * pBestSolution = tState.pBestSolution;
	if( pBestSolution == NULL ||
		pBestSolution->h > pOpenNode->h )
	{
		tState.pBestSolution = pOpenNode;
	}

	return pOpenNode;
}

//---------------------------------------------------------------------------------

static BOOL check_lower_g_closed(
	ASTAR_STATE & tState,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	ASTAR_NODE * pParent,
	float g)
{
	UINT64 nKey = MAKE_DUAL_POINTER_HASHKEY(pRoom, pPathNode);
	ASTAR_NODE *pNode = tState.NodeListHash.Get(nKey);
	if(pNode && pNode->bClosed)
		return TRUE;
	else return FALSE;
}

//---------------------------------------------------------------------------------

static BOOL check_lower_g(
	ASTAR_STATE & tState,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	ASTAR_NODE * pParent,
	float g )
{
	UINT64 nKey = MAKE_DUAL_POINTER_HASHKEY(pRoom, pPathNode);
	ASTAR_NODE *pNode = tState.NodeListHash.Get(nKey);
	if(pNode && (pNode->bClosed == FALSE))
	{
		if ( g < pNode->g )
		{
			pNode->g = g;
			pNode->f = pNode->g + pNode->h;
			pNode->pParent = pParent;
		}
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------------
// Just combining the above two functions, but leaving them there just in case.
//---------------------------------------------------------------------------------
static BOOL check_lower_g_both(
	ASTAR_STATE & tState,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	ASTAR_NODE * pParent,
	float g )
{
	UINT64 nKey = MAKE_DUAL_POINTER_HASHKEY(pRoom, pPathNode);
	ASTAR_NODE *pNode = tState.NodeListHash.Get(nKey);
	if(pNode)
	{
		if ( (pNode->bClosed == FALSE) && (g < pNode->g) )
		{
			pNode->g = g;
			pNode->f = pNode->g + pNode->h;
			pNode->pParent = pParent;
		}
		return TRUE;
	}
	return FALSE;
}
//---------------------------------------------------------------------------------
static BOOL process_current(
	ASTAR_STATE & tState,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	float g,
	ASTAR_NODE * pParent )
{
	if ( tState.tDestination.pRoomPathNode == pPathNode &&
			tState.tDestination.pRoom == pRoom )
		return TRUE;

	if ( check_lower_g_both( tState, pRoom, pPathNode, pParent, g ) )
		return FALSE;

	add_open_list( tState, pRoom, pPathNode, pParent, g );
	return FALSE;
}

//---------------------------------------------------------------------------------

static BOOL sUnitCalculatePathAStar(
	PATHING_ALGORITHM_CONTEXT & tContext)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerAStarPathRate,1);
#endif
	ASTAR_STATE tState;

	VECTOR vWorldPositionDestination;
	sGetWorldPosition(tContext.pDestinationRoom, tContext.pDestinationNode, &vWorldPositionDestination);
	tState.tDestination.pRoomPathNode = tContext.pDestinationNode;
	tState.tDestination.pRoom = tContext.pDestinationRoom;
	tState.tDestination.vWorldPosition = vWorldPositionDestination;
	tState.pGame = tContext.game;

	if ( !add_open_list( tState, tContext.pSourceRoom, tContext.pSourcePathNode, NULL, 0 ) )
	{
		return FALSE;
	}

	if( AppIsHellgate() && IS_SERVER( tContext.game ) )
	{
		if ( tContext.unit )
			s_StateClear( tContext.unit, UnitGetId(tContext.unit), STATE_PATH_BLOCKED, 0 );
	}

	ASTAR_NODE * list_end = NULL;

	bool bSuccess = FALSE;
	BOOL bDone = FALSE;
	int nSteps = 0;
	BOOL bPartialSuccess = FALSE;
	while ( !bDone )
	{
		if ( tState.nOpenListSize == 0 )
			break;

		ASTAR_NODE * lowest = find_lowest_open(tState);
		ASTAR_NODE * current = move_to_closed( tState, lowest );

		if ( !current || nSteps >= tContext.nMaxPathNodeCount )
		{
			if ( tState.pBestSolution != NULL )
			{
				bPartialSuccess = TRUE;
				if ( tContext.bAcceptPartialSuccess)
				{
					bSuccess = TRUE;
					if( AppIsHellgate() && IS_SERVER( tContext.game ) )
					{
						if ( tContext.unit )
						s_StateSet( tContext.unit, tContext.unit, STATE_PATH_BLOCKED, 100 );
					}
				}
				list_end = tState.pBestSolution;
				break;
			}
			bSuccess = FALSE;
			list_end = current;
			break;
		}
		nSteps++;

		// Iterate through all of the node's connections
		// Connections are sorted in order of distance, so iterate backwards so that
		// we favor longer connections over shorter ones.
		for ( int i=current->pRoomPathNode->nConnectionCount-1; i >= 0 && !bDone; i-- )
		{
			ROOM_PATH_NODE_CONNECTION * pConnection = &current->pRoomPathNode->pConnections[i];
#if HELLGATE_ONLY
			if (!sShouldTest(current->pRoom, pConnection->pConnection, pConnection, tContext) /*||
				pConnection->dwFlags & ROOM_PATH_NODE_CONNECTION_JUMP_FLAG */)
#else
			ROOM_PATH_NODE * pConnectionNode = &current->pRoom->pPathDef->pPathNodeSets[current->pRoom->nPathLayoutSelected].pPathNodes[pConnection->nConnectionIndex];
			if (!sShouldTest(current->pRoom, pConnectionNode, pConnection, tContext))
#endif
			{
				continue;
			}
#if HELLGATE_ONLY
			bDone = process_current(tState, current->pRoom, pConnection->pConnection, pConnection->fDistance, current);
#else
			bDone = process_current(tState, current->pRoom, pConnectionNode, PATH_NODE_DISTANCE, current);
#endif
		}

		// If the node is an edge node, then iterate through all of the edge connections
		if ( !bDone && ( current->pRoomPathNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG ) && RoomTestFlag(current->pRoom, ROOM_EDGENODE_CONNECTIONS) )
		{
			int nIndex = current->pRoomPathNode->nEdgeIndex;
			ASSERT_RETFALSE(current->pRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
			EDGE_NODE_INSTANCE * pInstance = &current->pRoom->pEdgeInstances[nIndex];
			for(int i=0; i<pInstance->nEdgeConnectionCount && !bDone; i++)
			{
				PATH_NODE_EDGE_CONNECTION * pConnection = &current->pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
				if(!sShouldTestEdge(tContext.game, current->pRoom, current->pRoomPathNode, pInstance, i, pConnection))
					continue;
				int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
				ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
				if (!sShouldTest(pConnection->pRoom, pOtherRoomNode, NULL, tContext))
				{
					continue;
				}
				bDone = process_current(tState, pConnection->pRoom, pOtherRoomNode, pConnection->fDistance, current);
			}
		}

		if ( bDone )
		{
			bSuccess = TRUE;
			list_end = current;
		}
	}

	if (tContext.nPathNodeCount && tContext.pPathNodes)
	{
		if (bSuccess)
		{
			ASTAR_OUTPUT FinalPath[DEFAULT_ASTAR_LIST_SIZE];
			int nElementCount = 0;

			if(!bPartialSuccess)
			{
				// Add the destination
				ASTAR_OUTPUT * pDestination = &FinalPath[nElementCount++];
				pDestination->nPathNodeIndex = tContext.pDestinationNode->nIndex;
				pDestination->nRoomId = RoomGetId(tContext.pDestinationRoom);
			}

			ASTAR_NODE * temp = list_end;
			temp = list_end;
			while(temp)
			{
				if(temp->pRoomPathNode != tContext.pSourcePathNode || temp->pRoom != tContext.pSourceRoom)
				{
					ASTAR_OUTPUT * pOutput = &FinalPath[nElementCount++];
					pOutput->nPathNodeIndex = temp->pRoomPathNode->nIndex;
					pOutput->nRoomId = RoomGetId(temp->pRoom);
				}

				ASTAR_NODE * pParent = temp->pParent;
				temp = pParent;
			}

			*tContext.nPathNodeCount = nElementCount;
			if(AppIsHellgate())
			{
				for(int i=0; i<*tContext.nPathNodeCount; i++)
				{
					tContext.pPathNodes[i] = FinalPath[*tContext.nPathNodeCount - i - 1];
				}
			}
			else
			{
				tContext.pPathNodes[0].nPathNodeIndex = tContext.pSourcePathNode->nIndex;
				tContext.pPathNodes[0].nRoomId = RoomGetId(tContext.pSourceRoom);

				for(int i=0; i<*tContext.nPathNodeCount; i++)
				{
					tContext.pPathNodes[i+1] = FinalPath[*tContext.nPathNodeCount - i - 1];
				}
				(*tContext.nPathNodeCount)++;
			}
		}
		else
		{
			(*tContext.nPathNodeCount) = 0;
		}
	}

	//trace("Open List Size: %d\nClosed List Size: %d\n", tState.nOpenListSize, tState.nClosedListSize);
	tState.OpenListHeap.Free();

	return bSuccess;
}

//---------------------------------------------------------------------------------

static BOOL sCheckCurrentDistance(
	const VECTOR & vDestWorldPosition,
	const VECTOR & vTestPosition,
	ROOM *& pSelectedRoom,
	ROOM_PATH_NODE *& pSelected,
	ROOM * pCurrentRoom,
	ROOM_PATH_NODE * pCurrent,
	float fDistanceCurrent,
	float & fMaxClosest,
	ROOM_PATH_NODE * pConnection,
	ROOM_PATH_NODE * pDestinationNode,
	ROOM * pDestinationRoom)
{
	BOOL bDone = FALSE;
	VECTOR vTestWorldPosition;
	sGetWorldPosition(pCurrentRoom, &vTestPosition, &vTestWorldPosition);
	float fDistanceTest = VectorDistanceSquared(vDestWorldPosition, vTestWorldPosition);
	float fDifference = fDistanceCurrent - fDistanceTest;
	if (fDifference > fMaxClosest)
	{
		pSelectedRoom = pCurrentRoom;
		pSelected = pConnection;
		fMaxClosest = fDifference;

		if (pSelected == pDestinationNode && pSelectedRoom == pDestinationRoom)
		{
			bDone = TRUE;
		}
	}
	return bDone;
}

//---------------------------------------------------------------------------------

static BOOL sUnitCalculatePathLinear(
	PATHING_ALGORITHM_CONTEXT & tContext)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerLinearPathRate,1);
#endif
	ASTAR_OUTPUT FinalPath[DEFAULT_ASTAR_LIST_SIZE];
	int nElementCount = 0;

	BOOL bSuccess = FALSE;

	BOOL bDone = FALSE;
	ROOM * pCurrentRoom = tContext.pSourceRoom;
	ROOM_PATH_NODE * pCurrent = tContext.pSourcePathNode;
	VECTOR vDestWorldPosition;
	sGetWorldPosition(tContext.pDestinationRoom, tContext.pDestinationNode, &vDestWorldPosition);
	while(!bDone)
	{
		ROOM * pSelectedRoom = pCurrentRoom;
		ROOM_PATH_NODE * pSelected = pCurrent;
		VECTOR vCurrWorldPosition;
		sGetWorldPosition(pCurrentRoom, pCurrent, &vCurrWorldPosition);
		float fDistanceCurrent = VectorDistanceSquared(vDestWorldPosition, vCurrWorldPosition);
		float fMaxClosest = 0.0f;
		for(int i=0; i<pCurrent->nConnectionCount && !bDone; i++)
		{
			ROOM_PATH_NODE_CONNECTION * pConnection = &pCurrent->pConnections[i];
#if HELLGATE_ONLY
			if (!sShouldTest(pCurrentRoom, pConnection->pConnection, pConnection, tContext))
			{
				continue;
			}
#else
			ROOM_PATH_NODE * pConnectionNode = &pCurrentRoom->pPathDef->pPathNodeSets[pCurrentRoom->nPathLayoutSelected].pPathNodes[pConnection->nConnectionIndex];
			if (!sShouldTest(pCurrentRoom, pConnectionNode, pConnection, tContext))
			{
				continue;
			}
#endif
#if HELLGATE_ONLY
			bDone = sCheckCurrentDistance(vDestWorldPosition, pConnection->pConnection->vPosition,
											pSelectedRoom, pSelected,
											pCurrentRoom, pCurrent,
											fDistanceCurrent, fMaxClosest,
											pConnection->pConnection,
											tContext.pDestinationNode,
											tContext.pDestinationRoom);
#else

			VECTOR vPosition( GetPathNodeX( pSelectedRoom->pPathDef, pConnectionNode->nOffset), GetPathNodeY( pSelectedRoom->pPathDef, pConnectionNode->nOffset), 0 );
			bDone = sCheckCurrentDistance(vDestWorldPosition, vPosition,
				pSelectedRoom, pSelected,
				pCurrentRoom, pCurrent,
				fDistanceCurrent, fMaxClosest,
				pConnectionNode,
				tContext.pDestinationNode,
				tContext.pDestinationRoom);
#endif
		}

		// If the node is an edge node, then iterate through all of the edge connections
		if ( !bDone && ( pCurrent->dwFlags & ROOM_PATH_NODE_EDGE_FLAG ) && RoomTestFlag(pCurrentRoom, ROOM_EDGENODE_CONNECTIONS) )
		{
			int nIndex = pCurrent->nEdgeIndex;
			ASSERT_RETFALSE(pCurrentRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
			EDGE_NODE_INSTANCE * pInstance = &pCurrentRoom->pEdgeInstances[nIndex];
			for(int i=0; i<pInstance->nEdgeConnectionCount && !bDone; i++)
			{
				PATH_NODE_EDGE_CONNECTION * pConnection = &pCurrentRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
				if(!sShouldTestEdge(tContext.game, pCurrentRoom, pCurrent, pInstance, i, pConnection))
					continue;
				int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
				ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
				if (!sShouldTest(pConnection->pRoom, pOtherRoomNode, NULL, tContext))
				{
					continue;
				}

#if HELLGATE_ONLY
				bDone = sCheckCurrentDistance(vDestWorldPosition, pOtherRoomNode->vPosition,
						pSelectedRoom, pSelected,
						pConnection->pRoom, pCurrent,
						fDistanceCurrent, fMaxClosest,
						pOtherRoomNode,
						tContext.pDestinationNode,
						tContext.pDestinationRoom);
#else
				VECTOR vPosition( GetPathNodeX( pSelectedRoom->pPathDef, pOtherRoomNode->nOffset), GetPathNodeY( pSelectedRoom->pPathDef, pOtherRoomNode->nOffset), 0 );
				bDone = sCheckCurrentDistance(vDestWorldPosition, vPosition,
						pSelectedRoom, pSelected,
						pConnection->pRoom, pCurrent,
						fDistanceCurrent, fMaxClosest,
						pOtherRoomNode,
						tContext.pDestinationNode,
						tContext.pDestinationRoom);
#endif
			}
		}

		if (bDone)
		{
			bSuccess = TRUE;
		}

		if (fMaxClosest <= 0.0f)
		{
			// TRAVIS: Linear paths should NEVER accept partial success!
			/*if (tContext.bAcceptPartialSuccess)
			{
				bSuccess = TRUE;
			}*/
			bDone = TRUE;
		}

		pCurrent = pSelected;
		pCurrentRoom = pSelectedRoom;
		//ASSERTX_RETFALSE( nElementCount < DEFAULT_ASTAR_LIST_SIZE, "DEFAULT_ASTAR_LIST_SIZE too small" );
		if( nElementCount >= tContext.nMaxPathNodeCount )
			return FALSE;
		ASTAR_OUTPUT * pOutput = &FinalPath[nElementCount++];
		pOutput->nPathNodeIndex = pCurrent->nIndex;
		pOutput->nRoomId = RoomGetId(pCurrentRoom);
	}

	if (bSuccess && tContext.nPathNodeCount && tContext.pPathNodes)
	{
		*tContext.nPathNodeCount = nElementCount;
		if(AppIsHellgate())
		{
			MemoryCopy(tContext.pPathNodes, tContext.nMaxPathNodeCount * sizeof(ASTAR_OUTPUT), FinalPath, (*tContext.nPathNodeCount)*sizeof(ASTAR_OUTPUT));
		}
		else
		{
			tContext.pPathNodes[0].nPathNodeIndex = tContext.pSourcePathNode->nIndex;
			tContext.pPathNodes[0].nRoomId = RoomGetId(tContext.pSourceRoom);
			for(int i=0; i<*tContext.nPathNodeCount; i++)
			{
				tContext.pPathNodes[i+1] = FinalPath[ i ];
			}
			(*tContext.nPathNodeCount)++;
		}
	}

	return bSuccess;
}

//---------------------------------------------------------------------------------
static void sCheckWanderDistance(
	GAME * game,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	const VECTOR & vSourcePosition,
	const VECTOR & vMoveDirection,
	float * fDistanceMovedSquared,
	float & fPathLengthDiffSquared,
	float fConnectionDistance,
	float & fClosestToLine,
	float & fFurthestDistanceMoved,
	ROOM *& pSelectedRoom,
	ROOM_PATH_NODE *& pSelected)
{
	VECTOR vPoint = AStarGetWorldPosition(game, RoomGetId(pRoom), pPathNode->nIndex);
	float fDistanceToLineSquared = VectorGetDistanceToLineSquared(vSourcePosition, vMoveDirection, vPoint, fDistanceMovedSquared);
	if(fDistanceToLineSquared < fClosestToLine && (*fDistanceMovedSquared) > fFurthestDistanceMoved)
	{
		fPathLengthDiffSquared = fConnectionDistance * fConnectionDistance;
		fFurthestDistanceMoved = *fDistanceMovedSquared;
		fClosestToLine = fDistanceToLineSquared;
		pSelectedRoom = pRoom;
		pSelected = pPathNode;
	}
}

//---------------------------------------------------------------------------------
static BOOL sUnitCalculatePathWander(
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	ASTAR_OUTPUT FinalPath[DEFAULT_ASTAR_LIST_SIZE];
	int nElementCount = 0;

	BOOL bSuccess = FALSE;
	BOOL bDone = FALSE;
	ROOM * pCurrentRoom = tContext.pSourceRoom;
	ROOM_PATH_NODE * pCurrent = tContext.pSourcePathNode;
	VECTOR vSourcePosition = AStarGetWorldPosition(tContext.game, RoomGetId(pCurrentRoom), tContext.pSourcePathNode->nIndex);
	const VECTOR * vMoveTarget = UnitGetMoveTarget(tContext.unit);
	ASSERT_RETFALSE(vMoveTarget);
	VECTOR vMoveDirection = UnitGetMoveDirection(tContext.unit);
	if(vMoveDirection == VECTOR(0))
	{
		vMoveDirection = *vMoveTarget - UnitGetPosition(tContext.unit);
	}
	VectorNormalize(vMoveDirection);
	float fDistanceToMoveSquared = VectorLengthSquared(*vMoveTarget - UnitGetPosition(tContext.unit));
	float fTotalPathLength = 0.0f;
	float fFurthestDistanceMoved = 0.0f;
	while(!bDone)
	{
		ROOM * pSelectedRoom = pCurrentRoom;
		ROOM_PATH_NODE * pSelected = pCurrent;
		VECTOR vCurrWorldPosition;
		sGetWorldPosition(pCurrentRoom, pCurrent, &vCurrWorldPosition);
		float fClosestToLine = INFINITY;
		float fDistanceMovedSquared = 0.0f;
		float fPathLengthDiffSquared = 0.0f;
		for(int i=0; i<pCurrent->nConnectionCount && !bDone; i++)
		{
			ROOM_PATH_NODE_CONNECTION * pConnection = &pCurrent->pConnections[i];
#if HELLGATE_ONLY
			if (!sShouldTest(pCurrentRoom, pConnection->pConnection, pConnection, tContext))
			{
				continue;
			}
#else
			ROOM_PATH_NODE * pConnectionNode = &pCurrentRoom->pPathDef->pPathNodeSets[pCurrentRoom->nPathLayoutSelected].pPathNodes[pConnection->nConnectionIndex];
			if (!sShouldTest(pCurrentRoom, pConnectionNode, pConnection, tContext))
			{
				continue;
			}
#endif

#if HELLGATE_ONLY
			sCheckWanderDistance(tContext.game, pCurrentRoom,
									pConnection->pConnection,
									vSourcePosition, vMoveDirection,
									&fDistanceMovedSquared, fPathLengthDiffSquared, 
									pConnection->fDistance,
									fClosestToLine, fFurthestDistanceMoved,
									pSelectedRoom, pSelected);

#else
			sCheckWanderDistance(tContext.game, pCurrentRoom,
				pConnectionNode,
				vSourcePosition, vMoveDirection,
				&fDistanceMovedSquared, fPathLengthDiffSquared, 
				PATH_NODE_DISTANCE,
				fClosestToLine, fFurthestDistanceMoved,
				pSelectedRoom, pSelected);
#endif
		}

		// If the node is an edge node, then iterate through all of the edge connections
		if ( (pCurrent->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pCurrentRoom, ROOM_EDGENODE_CONNECTIONS) )
		{
			int nIndex = pCurrent->nEdgeIndex;
			ASSERT_RETFALSE(pCurrentRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
			EDGE_NODE_INSTANCE * pInstance = &pCurrentRoom->pEdgeInstances[nIndex];
			for(int i=0; i<pInstance->nEdgeConnectionCount && !bDone; i++)
			{
				PATH_NODE_EDGE_CONNECTION * pConnection = &pCurrentRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
				if(!sShouldTestEdge(tContext.game, pCurrentRoom, pCurrent, pInstance, i, pConnection))
					continue;
				int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
				ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
				if (!sShouldTest(pConnection->pRoom, pOtherRoomNode, NULL, tContext))
				{
					continue;
				}
				sCheckWanderDistance(tContext.game, pConnection->pRoom,
					pOtherRoomNode,
					vSourcePosition, vMoveDirection,
					&fDistanceMovedSquared, fPathLengthDiffSquared, pConnection->fDistance,
					fClosestToLine, fFurthestDistanceMoved,
					pSelectedRoom, pSelected);
			}
		}

		if ((pSelectedRoom == pCurrentRoom && pSelected == pCurrent) ||
			fFurthestDistanceMoved >= fDistanceToMoveSquared ||
			fTotalPathLength >= fDistanceToMoveSquared * (1.5*1.5))
		{
			if(TESTBIT(tContext.dwFlags, PCF_CHECK_MELEE_ATTACK_AT_DESTINATION_BIT))
			{
				UNITID idTarget = UnitGetMoveTargetId(tContext.unit);
				UNIT * pTarget = UnitGetById(tContext.game, idTarget);
				if(!pTarget)
				{
					bSuccess = FALSE;
				}
				else
				{
					VECTOR vMoveTarget = UnitGetPosition(tContext.unit);
					if(nElementCount > 0)
					{
						ASTAR_OUTPUT * pOutput = &FinalPath[nElementCount-1];
						vMoveTarget = AStarGetWorldPosition(tContext.game, pOutput);
					}
					const UNIT_DATA * pUnitData = UnitGetData(tContext.unit);
					ASSERT_RETFALSE(pUnitData);
					if(!UnitCanMeleeAttackUnit(tContext.unit, pTarget, NULL, 0, pUnitData->fMeleeRangeMax, FALSE, NULL, FALSE, 0.0f, &vMoveTarget))
					{
						bSuccess = FALSE;
					}
					else
					{
						bSuccess = TRUE;
					}
				}
			}
			else
			{
				bSuccess = TRUE;
			}
			bDone = TRUE;
		}

		fTotalPathLength += fPathLengthDiffSquared;
		pCurrent = pSelected;
		pCurrentRoom = pSelectedRoom;
		if( nElementCount >= tContext.nMaxPathNodeCount )
			return FALSE;
		ASTAR_OUTPUT * pOutput = &FinalPath[nElementCount++];
		pOutput->nPathNodeIndex = pCurrent->nIndex;
		pOutput->nRoomId = RoomGetId(pCurrentRoom);
	}

	if (bSuccess && tContext.nPathNodeCount && tContext.pPathNodes)
	{
		*tContext.nPathNodeCount = nElementCount;
		MemoryCopy(tContext.pPathNodes, tContext.nMaxPathNodeCount * sizeof(ASTAR_OUTPUT), FinalPath, (*tContext.nPathNodeCount)*sizeof(ASTAR_OUTPUT));
	}

	return bSuccess;
}

//---------------------------------------------------------------------------------

static void sUnitPrunePath(
	GAME* game,
	UNIT * unit,
	int	* nPathNodeCount,
	ASTAR_OUTPUT * pPathNodes )
{
	ASSERTX_RETURN(game && unit, "Expected Game and Unit");
	ASSERTX_RETURN(nPathNodeCount && pPathNodes, "Expected Path Nodes and Path Node Count");
	ASSERTX_RETURN(*nPathNodeCount < DEFAULT_ASTAR_LIST_SIZE, "Path Node Count is too large!");
	ASTAR_OUTPUT PrunedPath[DEFAULT_ASTAR_LIST_SIZE];
	int nElementCount = 0;

	VECTOR LastDirection( 0 );
	for(int i=0; i<*nPathNodeCount; i++)
	{
		if( i == 0 || i == (*nPathNodeCount) - 1 )
		{
			PrunedPath[nElementCount++] = pPathNodes[i];
			if( i + 1 < (*nPathNodeCount) )
			{
				ASTAR_OUTPUT pNode = pPathNodes[i];
				ASTAR_OUTPUT pNextNode = pPathNodes[i + 1];
				VECTOR Delta = AStarGetWorldPosition( game, &pNextNode ) - AStarGetWorldPosition( game, &pNode );
				Delta.fZ = 0;
				VectorNormalize( Delta );
				LastDirection = Delta;
			}

		}
		else
		{
			ASTAR_OUTPUT pNode = pPathNodes[i];
			ASTAR_OUTPUT pNextNode = pPathNodes[i + 1];
			VECTOR Delta = AStarGetWorldPosition( game, &pNextNode ) - AStarGetWorldPosition( game, &pNode );
			Delta.fZ = 0;
			VectorNormalize( Delta );
			float fDot = VectorDot( Delta, LastDirection );
			if( fDot > .9f )
			{
				//LastDirection = Delta;
			}
			else
			{
				PrunedPath[nElementCount++] = pPathNodes[i];
				LastDirection = Delta;
			}

		}
	}
	*nPathNodeCount = nElementCount;
	for(int i=0; i<*nPathNodeCount; i++)
	{
		pPathNodes[i] = PrunedPath[i];
	}
#if HELLGATE_ONLY	
	if(!AppIsTugboat())
	{
		nElementCount = 0;

		float fUnitPathCollisionRadius = UnitGetPathingCollisionRadius(unit);

		int nStartIndex = 0;
		int nEndIndex = 2;
		for(int i=0; i<*nPathNodeCount; i++)
		{
			if( i == 0 || i == (*nPathNodeCount) - 1 )
			{
				PrunedPath[nElementCount++] = pPathNodes[i];
			}
			else
			{
				ASTAR_OUTPUT tStart	= pPathNodes[nStartIndex];
				ASTAR_OUTPUT tEnd	= pPathNodes[nEndIndex];
				ASTAR_OUTPUT tOrig	= pPathNodes[i];

				VECTOR vStart	= AStarGetWorldPosition(game, &tStart);
				VECTOR vEnd		= AStarGetWorldPosition(game, &tEnd);
				VECTOR vOrig	= AStarGetWorldPosition(game, &tOrig);

				float fDistance = 0.0f;
				BOOL bCanPrune = FALSE;
				if(DistancePointLine3D(&vOrig, &vStart, &vEnd, &fDistance))
				{
					ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(RoomGetByID(game, tOrig.nRoomId), tOrig.nPathNodeIndex);
					if(fDistance + fUnitPathCollisionRadius < pPathNode->fRadius)
					{
						bCanPrune = TRUE;
					}
				}
				if(!bCanPrune)
				{
					PrunedPath[nElementCount++] = pPathNodes[i];
					nStartIndex = i;
				}
				nEndIndex++;
			}
		}
		*nPathNodeCount = nElementCount;
		for(int i=0; i<*nPathNodeCount; i++)
		{
			pPathNodes[i] = PrunedPath[i];
		}
	}
#endif
}

//---------------------------------------------------------------------------------

static ROOM_PATH_NODE * sFindFreeNodeSomewhereAroundNodeTest(
	ROOM * pRoom,
	ROOM_PATH_NODE * pNode,
	ROOM ** pDestinationRoom,
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	if( sShouldTest(pRoom, pNode, NULL, tContext))
	{
		if(pDestinationRoom)
		{
			*pDestinationRoom = pRoom;
		}
		return pNode;
	}
	return NULL;
}

//---------------------------------------------------------------------------------

static ROOM_PATH_NODE * sFindFreeNodeSomewhereAroundRingTest(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_PATH_NODE * pNode,
	ROOM ** pDestinationRoom,
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	for(int i=0; i<pNode->nConnectionCount; i++)
	{
#if HELLGATE_ONLY
		ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundNodeTest(pRoom, pNode->pConnections[i].pConnection, pDestinationRoom, tContext);
#else
		ROOM_PATH_NODE * pConnectionNode = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[pNode->pConnections[i].nConnectionIndex];
		ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundNodeTest(pRoom, pConnectionNode, pDestinationRoom, tContext);
#endif
		if(pResult)
		{
			return pResult;
		}
	}

	if((pNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pRoom, ROOM_EDGENODE_CONNECTIONS))
	{
		int nIndex = pNode->nEdgeIndex;
		ASSERT_RETNULL(pRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
		EDGE_NODE_INSTANCE * pInstance = &pRoom->pEdgeInstances[nIndex];
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
			if(!sShouldTestEdge(pGame, pRoom, pNode, pInstance, i, pConnection))
				continue;
			int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
			ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];

			ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundNodeTest(pConnection->pRoom, pOtherRoomNode, pDestinationRoom, tContext);
			if(pResult)
			{
				return pResult;
			}
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------------

static ROOM_PATH_NODE * sFindFreeNodeSomewhereAroundRecurse(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_PATH_NODE * pNode,
	ROOM ** pDestinationRoom,
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	for(int i=0; i<pNode->nConnectionCount; i++)
	{
#if HELLGATE_ONLY
		ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundRingTest(pGame, pRoom, pNode->pConnections[i].pConnection, pDestinationRoom, tContext);
#else
		ROOM_PATH_NODE * pConnectionNode = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[pNode->pConnections[i].nConnectionIndex];
		ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundRingTest(pGame, pRoom, pConnectionNode, pDestinationRoom, tContext);
#endif
		if(pResult)
		{
			return pResult;
		}
	}

	if((pNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pRoom, ROOM_EDGENODE_CONNECTIONS))
	{
		int nIndex = pNode->nEdgeIndex;
		ASSERT_RETNULL(pRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
		EDGE_NODE_INSTANCE * pInstance = &pRoom->pEdgeInstances[nIndex];
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
			if(!sShouldTestEdge(pGame, pRoom, pNode, pInstance, i, pConnection))
				continue;
			int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
			ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];

			ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundRingTest(pGame, pConnection->pRoom, pOtherRoomNode, pDestinationRoom, tContext);
			if(pResult)
			{
				return pResult;
			}
		}
	}

	/*
	// Turn this code on to extend the ring test one layer further
	for(int i=0; i<pNode->nConnectionCount; i++)
	{
		ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundRecurse(pGame, pRoom, pNode->pConnections[i].pConnection, pDestinationRoom, tContext);
		if(pResult)
		{
			return pResult;
		}
	}

	if((pNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pRoom, ROOM_EDGENODE_CONNECTIONS))
	{
		int nIndex = pNode->nEdgeIndex;
		ASSERT_RETNULL(pRoom->pEdgeInstances.GetSize() > (unsigned)nIndex);
		EDGE_NODE_INSTANCE * pInstance = &pRoom->pEdgeInstances[nIndex];
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pInstance->pEdgeConnections[i];
			ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[pConnection->nEdgeIndex];

			ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundRecurse(pGame, pConnection->pRoom, pOtherRoomNode, pDestinationRoom, tContext);
			if(pResult)
			{
				return pResult;
			}
		}
	}
	// */
	return NULL;
}

//---------------------------------------------------------------------------------

static ROOM_PATH_NODE * sFindFreeNodeSomewhereAround(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_PATH_NODE * pNode,
	ROOM ** pDestinationRoom,
	PATHING_ALGORITHM_CONTEXT & tContext)
{
	ROOM_PATH_NODE * pResult = sFindFreeNodeSomewhereAroundNodeTest(pRoom, pNode, pDestinationRoom, tContext);
	if(pResult)
	{
		return pResult;
	}

	pResult = sFindFreeNodeSomewhereAroundRingTest(pGame, pRoom, pNode, pDestinationRoom, tContext);
	if(pResult)
	{
		return pResult;
	}

	pResult = sFindFreeNodeSomewhereAroundRecurse(pGame, pRoom, pNode, pDestinationRoom, tContext);
	if(pResult)
	{
		return pResult;
	}
	return NULL;
}

//---------------------------------------------------------------------------------

VECTOR AStarGetWorldPosition(
	GAME * pGame,
	ASTAR_OUTPUT * pOutputNode )
{
	return AStarGetWorldPosition(pGame, pOutputNode->nRoomId, pOutputNode->nPathNodeIndex);
}

//---------------------------------------------------------------------------------

VECTOR AStarGetWorldPosition(
	GAME * pGame,
	ROOMID idRoom,
	int nNodeIndex )
{
	VECTOR vOutput(0);
	ROOM * pRoom = RoomGetByID( pGame, idRoom );
	if(pRoom)
	{
		ROOM_PATH_NODE * pRoomPathNode = RoomGetRoomPathNode(pRoom, nNodeIndex);
		vOutput = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);
	}
	return vOutput;
}

#if HELLGATE_ONLY
//---------------------------------------------------------------------------------
VECTOR AStarGetWorldNormal(
	GAME * pGame,
	ASTAR_OUTPUT * pOutputNode )
{
	VECTOR vOutput(0);
	ROOM * room = RoomGetByID(pGame, pOutputNode->nRoomId);
	if (!room)
	{
		return vOutput;
	}
	return RoomGetPathNodeInstanceWorldNormal(room, pOutputNode->nPathNodeIndex, vOutput);
}


//---------------------------------------------------------------------------------
VECTOR AStarGetWorldNormal(
	GAME * game,
	ROOMID idRoom,
	int nNodeIndex)
{
	VECTOR vOutput(0);
	ROOM * room = RoomGetByID(game, idRoom);
	if (!room)
	{
		return vOutput;
	}
	return RoomGetPathNodeInstanceWorldNormal(room, nNodeIndex, vOutput);
}

#endif
//---------------------------------------------------------------------------------
static BOOL sRequireDestinationNode(
	int nMaxPathCalculationMethod,
	DWORD dwFlags)
{
	if(AppIsTugboat())
		return TRUE;

	switch(nMaxPathCalculationMethod)
	{
	case PATH_WANDER:
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------------
// drb.path

static BOOL drbDISTANCE_ASSERT = FALSE;

BOOL AStarCalculatePath(
	GAME * pGame,
	int	* nPathNodeCount,
	ASTAR_OUTPUT * pPathNodes,
	struct PATH_STATE * pPathState,
	ROOM * pSourceRoom,
	ROOM_PATH_NODE * pSourceNode,
	ROOM * pDestinationRoom,
	ROOM_PATH_NODE * pDestinationNode,
	DWORD dwFlags)
{
	ASSERTX_RETFALSE(pGame, "Expected Game");
	ASSERTX_RETFALSE((nPathNodeCount && pPathNodes) || (!nPathNodeCount && !pPathNodes), "Bad combo of Path Nodes and Path Node Count Pointers");
	ASSERTX_RETFALSE(pPathState, "Expected pathing structure");

	int nPinOffset = AppIsTugboat() ? 1 : 0;
	int nMaxPathNodes = DEFAULT_ASTAR_LIST_SIZE - nPinOffset;
	nMaxPathNodes = PIN(nMaxPathNodes, 1, DEFAULT_ASTAR_LIST_SIZE - nPinOffset);
	CHash<PATH_NODE_SHOULD_TEST> * pShouldTestHash = PathStructGetShouldTest(pPathState);
	ASSERT_RETFALSE(pShouldTestHash);
	HashClear(*pShouldTestHash);
	BOOL bAcceptPartialSuccess = TESTBIT(dwFlags, PCF_ACCEPT_PARTIAL_SUCCESS_BIT);
	BOOL bSuccess = FALSE;

	PATHING_ALGORITHM_CONTEXT tContext;
	structclear(tContext);
	tContext.game = pGame;
	tContext.unit = NULL;
	tContext.nPathNodeCount = nPathNodeCount;
	tContext.pPathNodes = pPathNodes;
	tContext.pSourceRoom = pSourceRoom;
	tContext.pSourcePathNode = pSourceNode;
	tContext.bAcceptPartialSuccess = bAcceptPartialSuccess;
	tContext.tHash = pShouldTestHash;
	tContext.nMaxPathNodeCount = nMaxPathNodes;
	tContext.dwFlags = dwFlags;
	tContext.pSourceRoom = pSourceRoom;
	tContext.pSourcePathNode = pSourceNode;
	tContext.pDestinationRoom = pDestinationRoom;
	tContext.pDestinationNode = pDestinationNode;

	if (!bSuccess)
	{
		TIMER_START("Doing Linear");
		bSuccess = sUnitCalculatePathLinear(tContext);
	}

	if (!bSuccess)
	{
		TIMER_START("Doing A*");
		bSuccess = sUnitCalculatePathAStar(tContext);
	}

	return bSuccess;
}

//---------------------------------------------------------------------------------
// drb.path

BOOL UnitCalculatePath(
	GAME* game,
	UNIT * unit,
	UNIT * targetunit,
	int	* nPathNodeCount,
	ASTAR_OUTPUT * pPathNodes,
	const VECTOR & vDestination,
	int nMaxPathCalculationMethod,
	DWORD dwFlags,
	int nMaxPathNodeCount)
{
	ASSERTX_RETFALSE(game && unit, "Expected Game and Unit");
	ASSERTX_RETFALSE((nPathNodeCount && pPathNodes) || (!nPathNodeCount && !pPathNodes), "Bad combo of Path Nodes and Path Node Count Pointers");
	ASSERTX_RETFALSE(unit->m_pPathing, "Unit has no pathing structure");

	CHash<PATH_NODE_SHOULD_TEST> * pShouldTestHash = PathStructGetShouldTest(unit);
	ASSERT_RETFALSE(pShouldTestHash);
	HashClear(*pShouldTestHash);

	if (RoomIsActive(UnitGetRoom(unit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(unit, "Attempt to path Inactive Room"))
		{
			return FALSE;
		}
	}

	ROOM * pSourceRoom = NULL;
	ROOM_PATH_NODE * pSourceNode = NULL;
	int nSourceIndex = INVALID_ID;
	if(!TESTBIT(dwFlags, PCF_CALCULATE_SOURCE_POSITION_BIT))
	{
		UnitGetPathNodeOccupied( unit, &pSourceRoom, &nSourceIndex );
	}
	if(pSourceRoom && nSourceIndex != INVALID_ID)
	{
		pSourceNode = RoomGetRoomPathNode( pSourceRoom, nSourceIndex );
	}
	else
	{
		pSourceNode = RoomGetNearestPathNode(game, UnitGetRoom(unit), UnitGetPosition(unit), &pSourceRoom);
	}
	ASSERTX_RETFALSE(pSourceRoom, "Unit does not have a room he's occupying");
	ASSERTX_RETFALSE(pSourceNode, "Unit does not have a path node he's occupying");
	nSourceIndex = pSourceNode->nIndex;

	int nPinOffset = AppIsTugboat() ? 1 : 0;
	int nMaxPathNodes = (nMaxPathNodeCount > 0) ? nMaxPathNodeCount - nPinOffset : DEFAULT_ASTAR_LIST_SIZE - nPinOffset;
	nMaxPathNodes = PIN(nMaxPathNodes, 1, DEFAULT_ASTAR_LIST_SIZE - nPinOffset);

	int nCurrentCalculationMethod = 0;

	BOOL bAcceptPartialSuccess = TESTBIT(dwFlags, PCF_ACCEPT_PARTIAL_SUCCESS_BIT);

	VECTOR vSourceNodePosition = RoomPathNodeGetExactWorldPosition(game, pSourceRoom, pSourceNode);
	VECTOR vMidPoint = vSourceNodePosition + (vDestination - vSourceNodePosition) / 2.0f;

	/*
	// Let's get a potentially collidable set of units
	// These are not used yet, so don't spend the time do find these yet
	UNITID idPotentiallyCollidableSet[MAX_TARGETS_PER_QUERY];
	SKILL_TARGETING_QUERY tTargetQuery;
	tTargetQuery.pSeekerUnit = unit;
	tTargetQuery.pvLocation = &vMidPoint;
	tTargetQuery.fMaxRange = VectorLength(vDestination - vSourceNodePosition) / 2.0f + UnitGetPathingCollisionRadius(unit);
	tTargetQuery.pnUnitIds = idPotentiallyCollidableSet;
	tTargetQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
	tTargetQuery.tFilter.dwFlags = SKILL_TARGETING_BIT_TARGET_FRIEND | SKILL_TARGETING_BIT_TARGET_ENEMY | SKILL_TARGETING_BIT_USE_LOCATION | SKILL_TARGETING_BIT_NO_FLYERS | SKILL_TARGETING_BIT_NO_DESTRUCTABLES | SKILL_TARGETING_BIT_FIRSTFOUND;
	SkillTargetQuery(game, tTargetQuery);
	// */

	BOOL bSuccess = FALSE;
	PATHING_ALGORITHM_CONTEXT tContext;
	structclear(tContext);
	tContext.game = game;
	tContext.unit = unit;
	tContext.nPathNodeCount = nPathNodeCount;
	tContext.pPathNodes = pPathNodes;
	tContext.pSourceRoom = pSourceRoom;
	tContext.pSourcePathNode = pSourceNode;
	tContext.bAcceptPartialSuccess = bAcceptPartialSuccess;
	tContext.tHash = pShouldTestHash;
	tContext.nMaxPathNodeCount = nMaxPathNodes;
	tContext.dwFlags = dwFlags;
	/*
	// Ditto above
	tContext.nPotentiallyCollidableCount = tTargetQuery.nUnitIdCount;
	for(int i=0; i<tTargetQuery.nUnitIdCount; i++)
	{
		tContext.pPotentiallyCollidableSet[i] = UnitGetById(game, idPotentiallyCollidableSet[i]);
	}
	// */

	ROOM * pDestinationRoom = NULL;
	ROOM_PATH_NODE * pDestinationNode = NULL;
	if(sRequireDestinationNode(nMaxPathCalculationMethod, dwFlags))
	{
		TIMER_START("Find Destination");
		ROOM * pEstimatedRoom = pSourceRoom;
		ROOM_PATH_NODE * pEstimatedNode = pSourceNode;
		int nDestIndex;
		if(targetunit && targetunit->m_pPathing)
		{
			UnitGetPathNodeOccupied( targetunit, &pEstimatedRoom, &nDestIndex );
			pEstimatedNode = RoomGetRoomPathNode( pEstimatedRoom, nDestIndex );

			if (!pEstimatedNode || !pEstimatedRoom)
			{
				pEstimatedRoom = pSourceRoom;
				pEstimatedNode = pSourceNode;
			}
		}
		else
		{
			DWORD dwNearestPathNodeFlags = 0;
			SETBIT(dwNearestPathNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
			pDestinationNode = RoomGetNearestPathNode(game, pEstimatedRoom, vDestination, &pDestinationRoom, dwNearestPathNodeFlags);
			if(!pDestinationNode || !pDestinationRoom)
			{
				return FALSE;
			}
		}

		if( !pDestinationRoom )
		{
			pDestinationRoom = pEstimatedRoom;
			pDestinationNode = pEstimatedNode;
		}
		// Now find a free spot near that node
		pDestinationNode = sFindFreeNodeSomewhereAround(game, pDestinationRoom, pDestinationNode, &pDestinationRoom, tContext);
		if (!pDestinationNode || !pDestinationRoom)
		{
			return FALSE;
		}

		if ( pSourceNode == pDestinationNode && pSourceRoom == pDestinationRoom )
		{
			return FALSE;
		}
	}
	tContext.pDestinationRoom = pDestinationRoom;
	tContext.pDestinationNode = pDestinationNode;

	if(nMaxPathCalculationMethod <= PATH_FULL)
	{
		// If other algorithms present themselves, place them in here
		if (!bSuccess && nCurrentCalculationMethod <= nMaxPathCalculationMethod)
		{
			TIMER_START("Doing Linear");
			bSuccess = sUnitCalculatePathLinear(tContext);
			nCurrentCalculationMethod++;
		}

		if (!bSuccess && nCurrentCalculationMethod <= nMaxPathCalculationMethod)
		{
			TIMER_START("Doing A*");
			bSuccess = sUnitCalculatePathAStar(tContext);
			nCurrentCalculationMethod++;
		}
	}
	else
	{
		switch(nMaxPathCalculationMethod)
		{
		case PATH_WANDER:
			{
				TIMER_START("Doing Wander");
				bSuccess = sUnitCalculatePathWander(tContext);
			}
			break;
		}
	}

	if(!bSuccess && TESTBIT(dwFlags, PCF_USE_HAPPY_PLACES_BIT))
	{
		VECTOR vHappyPlace;
		BOOL bHasHappyPlace = UnitFindGoodHappyPlace(game, unit, vHappyPlace);
		if(bHasHappyPlace)
		{
			DWORD dwHappyFlags = dwFlags;
			CLEARBIT(dwHappyFlags, PCF_USE_HAPPY_PLACES_BIT);
			bSuccess = UnitCalculatePath(game, unit, targetunit, nPathNodeCount, pPathNodes, vHappyPlace, nMaxPathCalculationMethod, dwHappyFlags, nMaxPathNodeCount);
		}
	}

	if(bSuccess && drbDISTANCE_ASSERT)
	{
		// debug check!
		VECTOR first = AStarGetWorldPosition( game, &pPathNodes[0] );
		VECTOR start = AStarGetWorldPosition( game, RoomGetId( pSourceRoom ), nSourceIndex );
		float drbdist = VectorDistanceSquaredXY( start, first );
		drbSuperTrackerI( unit, UnitGetClass( unit ) );
		drbSuperTrackerI2( unit, (int)RoomGetId(pSourceRoom), nSourceIndex );
		drbSuperTrackerI2( unit, pPathNodes[0].nRoomId, pPathNodes[0].nPathNodeIndex );
		drbSuperTrackerV( unit, start );
		drbSuperTrackerV( unit, first );
		// GS - 2-02-2007
		// Changing this from 1.6 to 1.75
		// 1.6 is more than we need for 2D movement, but we can also have movement in 3D, and that gets us from
		// sqrt(2) to sqrt(3) as our max range (plus fudge)
		drbSTASSERT( drbdist < ( 1.75f * 1.75f ), unit );
		drbDISTANCE_ASSERT = FALSE;
	}

#if ISVERSION(SERVER_VERSION)
	if(!bSuccess)
	{
		GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
		if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerPathFailureRate,1);
	}
#endif

	return bSuccess;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void AStarPathSpline( GAME * pGame, ASTAR_OUTPUT * alist, int list_size, int index, VECTOR * vPrevious, VECTOR * vNew, VECTOR * vFace, float delta )
{
	ASSERT_RETURN( list_size >= 2 );
	ASSERT_RETURN( alist );
	ASSERT_RETURN( vPrevious );
	ASSERT_RETURN( vNew );
	ASSERT_RETURN( vFace );
	ASSERT_RETURN( index >= 0 && index < list_size );
	ASSERT_RETURN( delta >= 0.0f && delta <= 1.0f );

	if ( delta == 0.0f )
	{
		*vNew = AStarGetWorldPosition( pGame, &alist[index] );
		VECTOR dir = *vNew - *vPrevious;
		VectorNormalize( dir );
		*vFace = dir;
		return;
	}

	int previous_knot = index - 1;
	if ( previous_knot < 0 ) previous_knot = 0;
	int next_knot = index + 1;
	if ( next_knot >= list_size ) next_knot = list_size - 1;
	int last_knot = next_knot + 1;
	if ( last_knot >= list_size ) last_knot = list_size - 1;

	if ( delta == 1.0f )
	{
		*vNew = AStarGetWorldPosition( pGame, &alist[next_knot] );
		VECTOR dir = *vNew - *vPrevious;
		VectorNormalize( dir );
		*vFace = dir;
		return;
	}

	VECTOR vPreviousKnot = AStarGetWorldPosition( pGame, &alist[previous_knot] );
	VECTOR vCurrentKnot = AStarGetWorldPosition( pGame, &alist[index] );
	VECTOR vNextKnot = AStarGetWorldPosition( pGame, &alist[next_knot] );
	VECTOR vLastKnot = AStarGetWorldPosition( pGame, &alist[last_knot] );

	VECTOR a = -1.0f * vPreviousKnot;
	a += 3.0f * vCurrentKnot;
	a += -3.0f * vNextKnot;
	a += vLastKnot;
	a = a * delta * delta * delta;
	VECTOR b = 2.0f * vPreviousKnot;
	b += -5.0f * vCurrentKnot;
	b += 4.0f * vNextKnot;
	b -= vLastKnot;
	b = b * delta * delta;
	VECTOR c = vNextKnot - vPreviousKnot;
	c = c * delta;
	VECTOR d = 2.0f * vCurrentKnot;
	VECTOR vPos = a + b + c + d;
	*vNew = 0.5f * vPos;

	VECTOR dir = *vNew - *vPrevious;
	VectorNormalize( dir );
	*vFace = dir;
}
