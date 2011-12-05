//----------------------------------------------------------------------------
// room_path.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "game.h"
#include "level.h"
#include "prime.h"
#include "room_path.h"
#include "room.h"
#include "picker.h"
#include "units.h"
#if ISVERSION(SERVER_VERSION)
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * RoomGetRoomPathNode(
									 ROOM *pRoom,
									 int nNodeIndex)
{
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );
	ASSERT_RETNULL( pRoomPathNodes );
	ASSERT_RETNULL( nNodeIndex >= 0 && nNodeIndex < pRoomPathNodes->nPathNodeCount );
	return &pRoomPathNodes->pPathNodes[ nNodeIndex ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline int sRoomPathNodeGetMultiplier(
	float fMinX,
	float fMaxX,
	int nGridSize)
{
	return int(CEIL((fMaxX - fMinX) / nGridSize));
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline int sRoomPathNodeGetHashValue(
	float fTestX,
	float fTestY,
	float fMinX,
	float fMinY,
	float fMaxX,
	float fMaxY,
	int nGridSize)
{

	int nXPart = int((fTestX - fMinX) / nGridSize);
	int nYPart = int((fTestY - fMinY) / nGridSize);

	int nMultiplierX = sRoomPathNodeGetMultiplier(fMinX, fMaxX, nGridSize);
	nXPart = PIN(nXPart, 0, nMultiplierX-1);
	int nMultiplierY = sRoomPathNodeGetMultiplier(fMinY, fMaxY, nGridSize);
	nYPart = PIN(nYPart, 0, nMultiplierY-1);
	return nXPart + nYPart * nMultiplierX;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define ROOM_PATH_NODE_HASH_ARRAY_GRID_SIZE 4
#define ROOM_PATH_NODE_HASH_ARRAY_GRID_SIZE_TUGBOAT 4
static inline const int sRoomPathNodeGetHashArrayGridSize()
{
	if(AppIsTugboat())
	{
		return ROOM_PATH_NODE_HASH_ARRAY_GRID_SIZE_TUGBOAT;
	}
	else
	{
		return ROOM_PATH_NODE_HASH_ARRAY_GRID_SIZE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomPathNodeDefinitionPostXMLLoad(
									   void * pDef,
									   BOOL bForceSyncLoad)
{
	if (!AppIsHammer())
	{
		ROOM_PATH_NODE_DEFINITION * pDefinition = (ROOM_PATH_NODE_DEFINITION*)pDef;
		typedef ROOM_PATH_NODE * lpROOM_PATH_NODE;
		SIMPLE_DYNAMIC_ARRAY<lpROOM_PATH_NODE> tEdgeNodes;
		ArrayInit(tEdgeNodes, NULL, 256);

		if( AppIsTugboat() )
		{
			if( pDefinition->fNodeFrequencyX == 0 ||
				pDefinition->fNodeFrequencyX == 1 )
			{
				pDefinition->fNodeFrequencyX = .45f;
				pDefinition->fNodeFrequencyY = .45f;
			}
		}

		// move everything to the static allocator, out of scratch

		if ( pDefinition->pPathNodeSets )
		{
			ROOM_PATH_NODE_SET* pPathNodeSets;
			pPathNodeSets = (ROOM_PATH_NODE_SET *)MALLOCZ(g_StaticAllocator, pDefinition->nPathNodeSetCount * sizeof(ROOM_PATH_NODE_SET ));
			MemoryCopy(pPathNodeSets, pDefinition->nPathNodeSetCount * sizeof(ROOM_PATH_NODE_SET), pDefinition->pPathNodeSets, pDefinition->nPathNodeSetCount * sizeof(ROOM_PATH_NODE_SET));
			FREE( g_ScratchAllocator, pDefinition->pPathNodeSets );
			pDefinition->pPathNodeSets = pPathNodeSets;
			for(int i=0; i< pDefinition->nPathNodeSetCount; i++)
			{
				if (pDefinition->pPathNodeSets[i].pPathNodes)
				{

					for(int j=0; j<pDefinition->pPathNodeSets[i].nPathNodeCount; j++)
					{

#if HELLGATE_ONLY
						if (pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections)
						{
							ROOM_PATH_NODE_CONNECTION_REF* pLongConnections;
							pLongConnections = (ROOM_PATH_NODE_CONNECTION_REF *)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF ));
							MemoryCopy(pLongConnections, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF), pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF));
							FREE( g_ScratchAllocator, pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections );
							pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections = pLongConnections;

						}
						if (pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections)
						{
							ROOM_PATH_NODE_CONNECTION_REF* pShortConnections;
							pShortConnections = (ROOM_PATH_NODE_CONNECTION_REF *)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF ));
							MemoryCopy(pShortConnections, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF), pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION_REF));
							FREE( g_ScratchAllocator, pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections );
							pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections = pShortConnections;

						}
#endif
					}

					ROOM_PATH_NODE* pNewPathNodes;
					pNewPathNodes = (ROOM_PATH_NODE *)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].nPathNodeCount * sizeof(ROOM_PATH_NODE ));
					MemoryCopy(pNewPathNodes, pDefinition->pPathNodeSets[i].nPathNodeCount * sizeof(ROOM_PATH_NODE), pDefinition->pPathNodeSets[i].pPathNodes, pDefinition->pPathNodeSets[i].nPathNodeCount * sizeof(ROOM_PATH_NODE));
					FREE( g_ScratchAllocator, pDefinition->pPathNodeSets[i].pPathNodes );
					pDefinition->pPathNodeSets[i].pPathNodes = pNewPathNodes;						
				}


			}

		}

		for(int i=0; i<pDefinition->nPathNodeSetCount; i++)
		{
			int nTotalConnections = 0;


			pDefinition->pPathNodeSets[i].fMinX = INFINITY;
			pDefinition->pPathNodeSets[i].fMaxX = -INFINITY;
			pDefinition->pPathNodeSets[i].fMinY = INFINITY;
			pDefinition->pPathNodeSets[i].fMaxY = -INFINITY;
			pDefinition->pPathNodeSets[i].nArraySize = -1;
			for(int j=0; j<pDefinition->pPathNodeSets[i].nPathNodeCount; j++)
			{
#if HELLGATE_ONLY
				if(pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fX < pDefinition->pPathNodeSets[i].fMinX)
				{
					pDefinition->pPathNodeSets[i].fMinX = pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fX;
				}
				if(pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fX > pDefinition->pPathNodeSets[i].fMaxX)
				{
					pDefinition->pPathNodeSets[i].fMaxX = pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fX;
				}
				if(pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fY < pDefinition->pPathNodeSets[i].fMinY)
				{
					pDefinition->pPathNodeSets[i].fMinY = pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fY;
				}
				if(pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fY > pDefinition->pPathNodeSets[i].fMaxY)
				{
					pDefinition->pPathNodeSets[i].fMaxY = pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fY;
				}

#else

				float X = GetPathNodeX( pDefinition, pDefinition->pPathNodeSets[i].pPathNodes[j].nOffset );
				float Y = GetPathNodeY( pDefinition, pDefinition->pPathNodeSets[i].pPathNodes[j].nOffset );

				if(X < pDefinition->pPathNodeSets[i].fMinX)
				{
					pDefinition->pPathNodeSets[i].fMinX = X;
				}
				if(X > pDefinition->pPathNodeSets[i].fMaxX)
				{
					pDefinition->pPathNodeSets[i].fMaxX = X;
				}
				if(Y < pDefinition->pPathNodeSets[i].fMinY)
				{
					pDefinition->pPathNodeSets[i].fMinY = Y;
				}
				if(Y > pDefinition->pPathNodeSets[i].fMaxY)
				{
					pDefinition->pPathNodeSets[i].fMaxY = Y;
				}
#endif
				if (pDefinition->pPathNodeSets[i].pPathNodes[j].dwFlags & ROOM_PATH_NODE_EDGE_FLAG)
				{
					pDefinition->pPathNodeSets[i].pPathNodes[j].nEdgeIndex = tEdgeNodes.Count();
					ArrayAddItem(tEdgeNodes, &pDefinition->pPathNodeSets[i].pPathNodes[j]);
				}

				nTotalConnections += pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount;
#if HELLGATE_ONLY

				for(int k=0; k<pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount; k++)
				{
					pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections[k].pConnection = &pDefinition->pPathNodeSets[i].pPathNodes[pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections[k].nConnectionIndex];
				}				

				if( AppIsTugboat() )
				{
					pDefinition->pPathNodeSets[i].pPathNodes[j].vNormal = VECTOR( 0, 0, 1 );
				}
				for(int k=0; k<pDefinition->pPathNodeSets[i].pPathNodes[j].nLongConnectionCount; k++)
				{
					pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections[k].pConnection = &pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections[pDefinition->pPathNodeSets[i].pPathNodes[j].pLongConnections[k].nConnectionIndex];
				}
				for(int k=0; k<pDefinition->pPathNodeSets[i].pPathNodes[j].nShortConnectionCount; k++)
				{
					pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections[k].pConnection = &pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections[pDefinition->pPathNodeSets[i].pPathNodes[j].pShortConnections[k].nConnectionIndex];
				}
#endif

			}

			// need to build edge lists and hashes AFTER the reallocation so that the pointers are valid
			pDefinition->pPathNodeSets[i].nEdgeNodeCount = tEdgeNodes.Count();
			pDefinition->pPathNodeSets[i].pEdgeNodes = (ROOM_PATH_NODE **)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].nEdgeNodeCount * sizeof(ROOM_PATH_NODE *));
			MemoryCopy(pDefinition->pPathNodeSets[i].pEdgeNodes, pDefinition->pPathNodeSets[i].nEdgeNodeCount * sizeof(ROOM_PATH_NODE *), (const lpROOM_PATH_NODE*)tEdgeNodes, pDefinition->pPathNodeSets[i].nEdgeNodeCount * sizeof(ROOM_PATH_NODE *));
			ArrayClear(tEdgeNodes);


#if HELLGATE_ONLY 
			// Build the array
			int nHashArrayGridSize = sRoomPathNodeGetHashArrayGridSize();


			int nArraySizePathNodeCount = (pDefinition->pPathNodeSets[i].nPathNodeCount / (nHashArrayGridSize * nHashArrayGridSize)) + 1;
			int nArraySizeGrid = pDefinition->pPathNodeSets[i].nPathNodeCount > 0 ? sRoomPathNodeGetHashValue(
				pDefinition->pPathNodeSets[i].fMaxX, pDefinition->pPathNodeSets[i].fMaxY,
				pDefinition->pPathNodeSets[i].fMinX, pDefinition->pPathNodeSets[i].fMinY,
				pDefinition->pPathNodeSets[i].fMaxX, pDefinition->pPathNodeSets[i].fMaxY,
				nHashArrayGridSize) + 1 :
			0;
			pDefinition->pPathNodeSets[i].nArraySize = MAX(nArraySizePathNodeCount, nArraySizeGrid);
			pDefinition->pPathNodeSets[i].pNodeHashArray = (ROOM_PATH_NODE***)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].nArraySize * sizeof(ROOM_PATH_NODE**));
			pDefinition->pPathNodeSets[i].nHashLengths = (int*)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].nArraySize * sizeof(int));
			for(int j=0; j<pDefinition->pPathNodeSets[i].nPathNodeCount; j++)
			{
				int nIndex = 
					sRoomPathNodeGetHashValue(
					pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fX, pDefinition->pPathNodeSets[i].pPathNodes[j].vPosition.fY,
					pDefinition->pPathNodeSets[i].fMinX, pDefinition->pPathNodeSets[i].fMinY,
					pDefinition->pPathNodeSets[i].fMaxX, pDefinition->pPathNodeSets[i].fMaxY,
					nHashArrayGridSize);
				pDefinition->pPathNodeSets[i].nHashLengths[nIndex]++;
				pDefinition->pPathNodeSets[i].pNodeHashArray[nIndex] = (ROOM_PATH_NODE**)REALLOC(NULL, pDefinition->pPathNodeSets[i].pNodeHashArray[nIndex], pDefinition->pPathNodeSets[i].nHashLengths[nIndex] * sizeof(ROOM_PATH_NODE*));
				pDefinition->pPathNodeSets[i].pNodeHashArray[nIndex][pDefinition->pPathNodeSets[i].nHashLengths[nIndex]-1] = &pDefinition->pPathNodeSets[i].pPathNodes[j];
			}
			for( int k = 0; k < pDefinition->pPathNodeSets[i].nArraySize; k++ )
			{
				ROOM_PATH_NODE** pHashNodes;
				pHashNodes = (ROOM_PATH_NODE **)MALLOCZ(g_StaticAllocator, pDefinition->pPathNodeSets[i].nHashLengths[k] * sizeof(ROOM_PATH_NODE* ));
				MemoryCopy(pHashNodes, pDefinition->pPathNodeSets[i].nHashLengths[k] * sizeof(ROOM_PATH_NODE*), pDefinition->pPathNodeSets[i].pNodeHashArray[k], pDefinition->pPathNodeSets[i].nHashLengths[k] * sizeof(ROOM_PATH_NODE*));
				FREE( NULL, pDefinition->pPathNodeSets[i].pNodeHashArray[k] );
				pDefinition->pPathNodeSets[i].pNodeHashArray[k] = pHashNodes;
			}
#else
			int nLength = (int)( pDefinition->fNodeOffsetX * pDefinition->fNodeOffsetY );
			pDefinition->pPathNodeSets[i].pNodeIndexMap = (ROOM_PATH_NODE **)MALLOCZ(g_StaticAllocator, nLength * sizeof(ROOM_PATH_NODE* ));
			memclear( pDefinition->pPathNodeSets[i].pNodeIndexMap, nLength * sizeof(ROOM_PATH_NODE* ) );
			if( nLength > 0 )
			{
				for(int j=0; j<pDefinition->pPathNodeSets[i].nPathNodeCount; j++)
				{
					ASSERT_CONTINUE( pDefinition->pPathNodeSets[i].pPathNodes[j].nOffset < nLength);
					pDefinition->pPathNodeSets[i].pNodeIndexMap[pDefinition->pPathNodeSets[i].pPathNodes[j].nOffset] = &pDefinition->pPathNodeSets[i].pPathNodes[j];
				}
			}
#endif
			if( nTotalConnections > 0 )
			{
				pDefinition->pPathNodeSets[i].pConnections = (ROOM_PATH_NODE_CONNECTION *)MALLOCZ(g_StaticAllocator, nTotalConnections * sizeof(ROOM_PATH_NODE_CONNECTION ));

				ROOM_PATH_NODE_CONNECTION* pConnection = pDefinition->pPathNodeSets[i].pConnections;
				for(int j=0; j<pDefinition->pPathNodeSets[i].nPathNodeCount; j++)
				{
					MemoryCopy(pConnection, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION), pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections, pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount * sizeof(ROOM_PATH_NODE_CONNECTION));
					FREE( g_ScratchAllocator, pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections );				
					pDefinition->pPathNodeSets[i].pPathNodes[j].pConnections = pConnection;
					pConnection += pDefinition->pPathNodeSets[i].pPathNodes[j].nConnectionCount;
				}
			}

		}
		tEdgeNodes.Destroy();


		// save the distance between nodes
		// this is because we have a default frequency value of .45 that isn't saved....
		// legacy crap.
		if( AppIsTugboat() && pDefinition->fNodeFrequencyX == 1 )
		{
			pDefinition->fDiagDistBetweenNodesSq = .45f * .45f * .45f * .45f;
		}
		else
		{
			pDefinition->fDiagDistBetweenNodesSq = 
				pDefinition->fNodeFrequencyX * pDefinition->fNodeFrequencyX +
				pDefinition->fNodeFrequencyY * pDefinition->fNodeFrequencyY;
		}

		pDefinition->fDiagDistBetweenNodes = sqrt( pDefinition->fDiagDistBetweenNodesSq );
	}


	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int s_RoomGetRandomUnoccupiedNode(
								  GAME *pGame,
								  ROOM *pRoom,
								  DWORD dwRandomNodeFlags,
								  float fMinRadius, /* = 0.0f */
								  float fMinHeight /* = 0.0f */ )
{
	ASSERT_RETINVALID(pGame);
	ASSERT_RETINVALID(pRoom);

	DWORD dwBlockedNodeFlags = 0;

	// start picker
	PickerStart( pGame, picker );

	// check rooms that do not allow monster spawns at all
	if (TESTBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN ))
	{
		// check for whole room
		if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
		{
			return INVALID_INDEX;
		}

		// don't pick any no spawn nodes
		SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );
	}

	// put all nodes in a picker
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );	
	int nValidNodeCount = 0;
	for (int i = 0; i < pRoomPathNodes->nPathNodeCount; ++i)
	{
		ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pRoom, i);
		ASSERTX_CONTINUE(pPathNode, "NULL path node for index");

#if HELLGATE_ONLY
		// don't pick blocked nodes, or nodes that do not have enough free space
		if (s_RoomNodeIsBlocked( pRoom, i, dwBlockedNodeFlags, NULL ) != NODE_FREE ||
			pPathNode->fRadius < fMinRadius ||
			(AppIsHellgate() && pPathNode->fHeight < fMinHeight))
		{
			continue;
		}
#else
		// don't pick blocked nodes, or nodes that do not have enough free space
		if (s_RoomNodeIsBlocked( pRoom, i, dwBlockedNodeFlags, NULL ) != NODE_FREE ||
			PATH_NODE_RADIUS < fMinRadius )
		{
			continue;
		}
#endif

		// add to picker
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
		nValidNodeCount++;
	}

	if (nValidNodeCount > 0)
	{
		return PickerChoose( pGame );
	}

	return INVALID_INDEX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RoomGetRandomUnoccupiedPosition(
									   GAME *pGame,
									   ROOM *pRoom,
									   VECTOR &vPosition)
{

	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );
	if(nNodeIndex < 0)
	{
		return FALSE;
	}

	ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pRoom, nNodeIndex );
	ASSERT_RETFALSE( pNode );

	// transform position from room to world space
	vPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pNode);

	return TRUE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int s_LevelGetRandomNodeAround(
							   GAME *pGame,
							   LEVEL *pLevel,
							   const VECTOR & vCenter,
							   float fRadius,
							   ROOM ** pOutputRoom)
{
	ASSERT_RETINVALID(pGame);
	ASSERT_RETINVALID(pLevel);
	ASSERT_RETINVALID(pOutputRoom);

	// start picker
	PickerStart( pGame, picker );

	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
	SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
	tSpec.fMaxDistance = fRadius;

	const int MAX_RANDOM_NODES = 64;
	FREE_PATH_NODE pFreePathNodes[MAX_RANDOM_NODES];
	int nNodeCount = RoomGetNearestPathNodes(pGame, RoomGetFromPosition(pGame, pLevel, &vCenter), vCenter, MAX_RANDOM_NODES, pFreePathNodes, &tSpec);

	if(nNodeCount <= 0)
	{
		return INVALID_INDEX;
	}

	// put all nodes in a picker
	for (int i = 0; i < nNodeCount; ++i)
	{
		// add to picker
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
	}

	int nOutputIndex = PickerChoose( pGame );
	if (nOutputIndex < 0 || nOutputIndex >= nNodeCount)
	{
		return INVALID_INDEX;
	}

	*pOutputRoom = pFreePathNodes[nOutputIndex].pRoom;
	return pFreePathNodes[nOutputIndex].pNode->nIndex;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ROOM * s_LevelGetRandomPositionAround(
									  GAME *pGame,
									  LEVEL *pLevel,
									  const VECTOR & vCenter,
									  float fRadius,
									  VECTOR &vPosition )
{
	ROOM * pOutputRoom = NULL;
	int nNodeIndex = s_LevelGetRandomNodeAround( pGame, pLevel, vCenter, fRadius, &pOutputRoom );
	if(nNodeIndex < 0 || !pOutputRoom)
	{
		return NULL;
	}

	ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pOutputRoom, nNodeIndex );
	ASSERT_RETFALSE( pNode );

	// transform position from room to world space
	vPosition = RoomPathNodeGetExactWorldPosition(pGame, pOutputRoom, pNode);

	return pOutputRoom;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
ROOM_PATH_NODE * RoomGetNearestFreePathNodeDbg(
	GAME * game,
	LEVEL* level,
	const VECTOR & vPosition,
	ROOM** pDestinationRoom,
	const char * file,
	int line,
	UNIT * pUnit /*= NULL*/,
	float fHeight /*= 0.0f*/,
	float fRadius /*= 0.0f*/,
	ROOM *pRoomSource /*= NULL*/,
	DWORD dwFreePathNodeFlags /*= 0*/)
#else
ROOM_PATH_NODE * RoomGetNearestFreePathNode(
	GAME * game,
	LEVEL* level,
	const VECTOR & vPosition,
	ROOM** pDestinationRoom,
	UNIT * pUnit /*= NULL*/,
	float fHeight /*= 0.0f*/,
	float fRadius /*= 0.0f*/,
	ROOM *pRoomSource /*= NULL*/,
	DWORD dwFreePathNodeFlags /*= 0*/)
#endif
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,RoomGetNearestPathNodesInArray,1);
#endif
	// find exactly 1 node
	FREE_PATH_NODE tFreePathNode;

	NEAREST_NODE_SPEC tSpec;
	tSpec.dwFlags = dwFreePathNodeFlags;
	tSpec.pUnit = pUnit;
	tSpec.fMinHeight = fHeight;
	tSpec.fMinRadius = fRadius;

	ROOM * pRoomToUse = pRoomSource;
	if(pRoomToUse)
	{
		SETBIT(tSpec.dwFlags, NPN_ONE_ROOM_ONLY);
		SETBIT(tSpec.dwFlags, NPN_USE_GIVEN_ROOM);
	}
	else
	{
		pRoomToUse = RoomGetFromPosition(game, level, &vPosition);
	}
	if( AppIsTugboat() )
	{
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
		tSpec.fMaxDistance = 5;
	}

	if(!pRoomToUse)
		return NULL;	

#if ISVERSION(DEVELOPMENT)
	int nNumNodes = RoomGetNearestPathNodesDbg(game, pRoomToUse, vPosition, 1, &tFreePathNode, &tSpec, file, line);
#else
	int nNumNodes = RoomGetNearestPathNodes(game, pRoomToUse, vPosition, 1, &tFreePathNode, &tSpec);
#endif

	// return pointer to node (if any)
	if (nNumNodes > 0)
	{
		// save room
		if (pDestinationRoom)
		{
			*pDestinationRoom = tFreePathNode.pRoom;
		}

		// return node
		return tFreePathNode.pNode;
	}
	else
	{
		return NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
VECTOR RoomGetNearestFreePositionDbg(
									 ROOM *pRoomSource,
									 const VECTOR &vPosition,
									 const char * file,
									 int line,
									 ROOM **pRoomDest /*= NULL*/)
#else
VECTOR RoomGetNearestFreePosition(
								  ROOM *pRoomSource,
								  const VECTOR &vPosition,
								  ROOM **pRoomDest /*= NULL*/)
#endif
{
	LEVEL *pLevel = RoomGetLevel( pRoomSource );
	GAME *pGame = LevelGetGame( pLevel );

	ROOM *pRoom = NULL;
#if ISVERSION(DEVELOPMENT)
	ROOM_PATH_NODE *pPathNode = RoomGetNearestFreePathNodeDbg( 
		pGame, 
		pLevel, 
		vPosition, 
		&pRoom,
		file,
		line,
		NULL,
		0.0f,
		0.0f,
		pRoomSource);
	FL_ASSERTX_RETVAL( pPathNode, vPosition, "Unable to find nearest free path node", file, line );
#else
	ROOM_PATH_NODE *pPathNode = RoomGetNearestFreePathNode( 
		pGame, 
		pLevel, 
		vPosition, 
		&pRoom,
		NULL,
		0.0f,
		0.0f,
		pRoomSource);
	ASSERTX_RETVAL( pPathNode, vPosition, "Unable to find nearest free path node" );
#endif

	// save room if desired
	if (pRoomDest)
	{
		*pRoomDest = pRoom;
	}

	// get position
	return RoomPathNodeGetWorldPosition( pGame, pRoom, pPathNode );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
NODE_RESULT s_RoomNodeIsBlocked(
								ROOM *pRoom,
								int nNodeIndex,
								DWORD dwBlockedNodeFlags,
								UNIT ** pUnit)
{
	ASSERTX_RETVAL(pRoom, NODE_FREE, "Expected Room");
	if(pUnit)
	{
		*pUnit = NULL;
	}
	if( AppIsTugboat() )
	{
		PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, nNodeIndex );
		if (TESTBIT( pNodeInstance->dwNodeInstanceFlags, NIF_BLOCKED ))
		{
			return NODE_BLOCKED;
		}
	}

	UNIT_NODE_MAP *pNodeMap = HashGet( pRoom->tUnitNodeMap, nNodeIndex );
	if (pNodeMap)
	{
		if (pNodeMap->pUnit != NULL)
		{
			if(pUnit)
			{
				*pUnit = pNodeMap->pUnit;
			}
			return NODE_BLOCKED_UNIT;
		}	
		if (pNodeMap->bBlocked == TRUE)
		{
			return NODE_BLOCKED;		
		}
	}

	// check for no spawn nodes if requested
	if (TESTBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED ))
	{

		PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, nNodeIndex );
		if (TESTBIT( pNodeInstance->dwNodeInstanceFlags, NIF_NO_SPAWN ))
		{
			return NODE_BLOCKED;
		}
	}


	return NODE_FREE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NODE_RESULT s_RoomNodeIsBlockedIgnore(
									  ROOM *pRoom,
									  int nNodeIndex,
									  DWORD dwBlockedNodeFlags,
									  UNIT *pUnitIgnore /*= NULL*/)
{
	UNIT *pBlockedBy = NULL;
	NODE_RESULT eResult = s_RoomNodeIsBlocked( pRoom, nNodeIndex, dwBlockedNodeFlags, &pBlockedBy );

	switch (eResult)
	{

		//----------------------------------------------------------------------------
	case NODE_BLOCKED_UNIT:
		{
			if (pUnitIgnore != NULL && pUnitIgnore == pBlockedBy)
			{
				return NODE_FREE;	// blocked by unit, but it's our ignore unit
			}
			return NODE_BLOCKED_UNIT;	// node is blocked
		}

		//----------------------------------------------------------------------------
	case NODE_BLOCKED:
		{
			return NODE_BLOCKED;	// node is blocked
		}

	}

	return NODE_FREE;  // node is not blocked

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomTransformFreePathNode(
							   FREE_PATH_NODE *pFreePathNode,
							   VECTOR *pvTransformed)
{
	ASSERTX_RETFALSE( pFreePathNode, "Expected free path node" );
	ASSERTX_RETFALSE( pvTransformed, "Expected output vector" );
	ASSERTX_RETFALSE( pFreePathNode->pRoom, "Expected room in free path node" );

	LEVEL * pLevel = RoomGetLevel(pFreePathNode->pRoom);
	ASSERTX_RETFALSE( pLevel, "Room in free path node has no level" );

	*pvTransformed = RoomPathNodeGetExactWorldPosition(LevelGetGame(pLevel), pFreePathNode->pRoom, pFreePathNode->pNode);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomHasNoPathNodes(
								ROOM * pRoom)
{
	if(!pRoom)
		return TRUE;

	if(!pRoom->pPathDef)
		return TRUE;

	if(pRoom->pPathDef->dwFlags & ROOM_PATH_NODE_DEF_NO_PATHNODES_FLAG)
		return TRUE;

	if(!pRoom->pPathDef->nPathNodeSetCount)
		return TRUE;

	if(!pRoom->pPathDef->pPathNodeSets)
		return TRUE;

	if(!pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].nPathNodeCount)
		return TRUE;

	if(!pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes)
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR RoomPathNodeGetWorldPosition(
									GAME *pGame, 
									ROOM * pRoom, 
									ROOM_PATH_NODE * pRoomPathNode)
{
	ASSERT_RETVAL(pRoom && pRoomPathNode, cgvNull);

	// get position
	VECTOR vOutput = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);

	// offset it in cell
	ROOM_PATH_NODE_DEFINITION *pPathNodeDef = pRoom->pPathDef;

	// we want to get a position that is valid between this node and any of
	// its neighboring nodes (or walls), so we have to use the smaller of the 
	// following values that determine the spacing or node size

	float flRadius = MIN( pPathNodeDef->fNodeFrequencyX, pPathNodeDef->fNodeFrequencyY ) / 2.0f;
#if HELLGATE_ONLY
	if(AppIsHellgate())
	{
		flRadius = MIN(flRadius, pRoomPathNode->fRadius);
	}
#endif
	// this is because we have a default frequency value of .45 that isn't saved....
	// legacy crap.
	if( AppIsTugboat() && flRadius == .5f )
	{
		flRadius = .225f;
	}
#if HELLGATE_ONLY
	const float flSlop = 0.01f;
	flRadius = MIN( flRadius, pRoomPathNode->fRadius ) - flSlop;
#endif

	// Use the path node's normal to find a location along the floor.
	VECTOR vNormal = RoomPathNodeGetWorldNormal( pGame, pRoom, pRoomPathNode );
	VECTOR vProposedOutput = vOutput;

	// you get exactly ten tries to find a spot in this room
	for(int i=0; i<10; i++)
	{
		VECTOR vDelta(0.0f, 1.0f, 0.0f);
		VECTOR vSide;
		VectorCross(vSide, vDelta, vNormal);
		VectorCross(vDelta, vSide, vNormal);
		vDelta *= RandGetFloat( pGame, flRadius );
		MATRIX mRotation;
		MatrixRotationAxis(mRotation, vNormal, RandGetFloat(pGame, 0.0f, TWOxPI));
		MatrixMultiply(&vDelta, &vDelta, &mRotation);

		vProposedOutput += vDelta;
		ROOM * pProposedRoom = RoomGetFromPosition(pGame, RoomGetLevel(pRoom), &vProposedOutput);
		if(pProposedRoom != pRoom)
		{
			vProposedOutput = vOutput;
		}
		else
		{
			break;
		}
	}

	vOutput = vProposedOutput;

	return vOutput;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomPathNodeDoZFixup(
						  GAME * game, 
						  ROOM * room, 
						  PATH_NODE_INSTANCE * node)
{
#if HELLGATE_ONLY
	const float MAX_OFFSET = 1.7f;			// we cast a ray from the MAX_OFFSET above the node position
	static const VECTOR vDown(0, 0, -1);	// the ray goes down
	ASSERT_RETURN(node);

	VECTOR vTestOrigin = node->vWorldPosition;
	vTestOrigin.fZ += MAX_OFFSET;
	VECTOR vNormal(0);

	float fDistance = LevelLineCollideLen(game, RoomGetLevel(room), vTestOrigin, vDown, MAX_OFFSET, &vNormal);
	if (fDistance < MAX_OFFSET)
	{
		node->vWorldPosition.fZ = vTestOrigin.fZ - fDistance;
		node->vWorldNormal = vNormal;
		SETBIT(node->dwNodeInstanceFlags, NIF_DID_Z_FIXUP);
	}

	CLEARBIT(node->dwNodeInstanceFlags, NIF_NEEDS_Z_FIXUP);
#endif
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sRoomGetNearestPathNodesAddToOutput(
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	int nCurrentLength,
	ROOM_PATH_NODE * pRoomPathNode,
	ROOM * pRoom,
	float fDistanceSquared,
	float fDot)
{
	ASSERT_RETVAL(pOutput, nCurrentLength);
	ASSERT_RETVAL(nOutputLength > 0, nCurrentLength);

	BOOL bReplacing = FALSE;
	int nIndexToUse = nCurrentLength;
	if(nCurrentLength > nOutputLength - 1)
	{
		nIndexToUse = INVALID_ID;
		for(int i=0; i<nCurrentLength; i++)
		{
			if(pOutput[i].flDistSq > fDistanceSquared)
			{
				nIndexToUse = i;
				bReplacing = TRUE;
				break;
			}
		}
	}

	//#define TEST_DUPLICATE_NODES
#ifdef TEST_DUPLICATE_NODES
	if(nIndexToUse >= 0)
	{
		for(int i=0; i<nCurrentLength; i++)
		{
			if(bReplacing && nIndexToUse == i)
			{
				continue;
			}
			FREE_PATH_NODE * pTest = &pOutput[i];
			if(pRoom == pTest->pRoom && pRoomPathNode == pTest->pNode)
			{
				ASSERTV_MSG("Adding duplicate node room '%s' index %d", pRoom ? RoomGetDevName(pRoom) : "unknown room", pRoomPathNode ? pRoomPathNode->nIndex : INVALID_ID);
			}
		}
	}
#endif

	if(nIndexToUse >= 0)
	{
		FREE_PATH_NODE * pTarget = &pOutput[nIndexToUse];
		pTarget->pNode = pRoomPathNode;
		pTarget->pRoom = pRoom;
		pTarget->flDot = fDot;
		pTarget->flDistSq = fDistanceSquared;
		return bReplacing ? nCurrentLength : nCurrentLength+1;
	}
	return nCurrentLength;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sRoomGetNearestPathNodesInArray(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_PATH_NODE ** pPathNodeArray,
	const int nPathNodeArrayLength,
	const VECTOR & vWorldPosition,
	const VECTOR & vRoomPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	int nCurrentLength,
	const NEAREST_NODE_SPEC * pSpec)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,RoomGetNearestPathNodesInArray,1);
#endif
	ASSERT_RETVAL(pRoom, nCurrentLength);
	ASSERT_RETVAL(pOutput, nCurrentLength);
	ASSERT_RETVAL(nOutputLength > 0, nCurrentLength);
	ASSERT_RETVAL(pSpec, nCurrentLength);

	DWORD dwBlockedNodeFlags = 0;
	if (TESTBIT( pSpec->dwFlags, NPN_NO_SPAWN_NODES_ARE_BLOCKING ))
	{
		SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );
	}

	if(!pPathNodeArray)
		return nCurrentLength;

	for(int i=0; i<nPathNodeArrayLength; i++)
	{
		ROOM_PATH_NODE * pRoomPathNode = pPathNodeArray[i];
		ASSERT_CONTINUE(pRoomPathNode);

		if(TESTBIT(pSpec->dwFlags, NPN_EDGE_NODES_ONLY))
		{
			if(!(pRoomPathNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG))
			{
				continue;
			}
		}

		if(!TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES) || !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
		{
			NODE_RESULT eResult;
			if(pSpec->pUnit)
			{
				eResult = s_RoomNodeIsBlockedIgnore( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, pSpec->pUnit );
			}
			else
			{
				eResult = s_RoomNodeIsBlocked( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, NULL );
			}
			if(eResult == NODE_BLOCKED && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
			{
				continue;
			}
			if(eResult == NODE_BLOCKED_UNIT && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES))
			{
				continue;
			}
		}

		if(!TESTBIT(pSpec->dwFlags, NPN_DONT_CHECK_HEIGHT) || !TESTBIT(pSpec->dwFlags, NPN_DONT_CHECK_RADIUS))
		{
#if HELLGATE_ONLY
			if(!TESTBIT(pSpec->dwFlags, NPN_DONT_CHECK_HEIGHT))
			{
				if(pSpec->fMinHeight >= 0.0f || pSpec->fMaxHeight >= 0.0f)
				{
					if(pSpec->fMinHeight > 0.0f && pRoomPathNode->fHeight < pSpec->fMinHeight)
					{
						continue;
					}
					if(pSpec->fMaxHeight > 0.0f && pRoomPathNode->fHeight > pSpec->fMaxHeight)
					{
						continue;
					}
				}
				else if(pSpec->pUnit && pRoomPathNode->fHeight < UnitGetCollisionHeight(pSpec->pUnit))
				{
					continue;
				}
			}
			if(!TESTBIT(pSpec->dwFlags, NPN_DONT_CHECK_RADIUS))
			{
				if(pSpec->fMinRadius >= 0.0f || pSpec->fMaxRadius >= 0.0f)
				{
					if(pSpec->fMinRadius > 0.0f && pRoomPathNode->fRadius < pSpec->fMinRadius)
					{
						continue;
					}
					if(pSpec->fMaxRadius > 0.0f && pRoomPathNode->fRadius > pSpec->fMaxRadius)
					{
						continue;
					}
				}
				else if(pSpec->pUnit && pRoomPathNode->fRadius < UnitGetPathingCollisionRadius(pSpec->pUnit))
				{
					continue;
				}
			}
#else
			if(!TESTBIT(pSpec->dwFlags, NPN_DONT_CHECK_RADIUS))
			{
				if(pSpec->fMinRadius >= 0.0f || pSpec->fMaxRadius >= 0.0f)
				{
					if(pSpec->fMinRadius > 0.0f && PATH_NODE_RADIUS < pSpec->fMinRadius)
					{
						continue;
					}
					if(pSpec->fMaxRadius > 0.0f && PATH_NODE_RADIUS > pSpec->fMaxRadius)
					{
						continue;
					}
				}
				else if(pSpec->pUnit && PATH_NODE_RADIUS < UnitGetPathingCollisionRadius(pSpec->pUnit))
				{
					continue;
				}
			}
#endif
		}

		float fDot = 0.0f;
		if(TESTBIT(pSpec->dwFlags, NPN_IN_FRONT_ONLY) || TESTBIT(pSpec->dwFlags, NPN_BEHIND_ONLY))
		{
			VECTOR vDirectionalityFaceDirection = pSpec->vFaceDirection;
			if(pSpec->pUnit && VectorIsZero(vDirectionalityFaceDirection))
			{
				vDirectionalityFaceDirection = UnitGetFaceDirection(pSpec->pUnit, FALSE);
			}
			if(!VectorIsZero(vDirectionalityFaceDirection))
			{
				VectorNormalize(vDirectionalityFaceDirection);
				VECTOR vNodePositionWorld = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);
				VECTOR vToNodePosWorld;
				VectorSubtract( vToNodePosWorld, vNodePositionWorld, vWorldPosition );
				VectorNormalize( vToNodePosWorld );

				// check for front or behind
				// get cos of the angle between direction and vector to node
				fDot = VectorDot( vDirectionalityFaceDirection, vToNodePosWorld );
				if(fDot <= 0.5f && TESTBIT( pSpec->dwFlags, NPN_IN_FRONT_ONLY))
				{
					if(TESTBIT( pSpec->dwFlags, NPN_QUARTER_DIRECTIONALITY))
					{
						continue;
					}
					else if(fDot <= 0)
					{
						continue;
					}
				}
				if(fDot > 0.5f && TESTBIT( pSpec->dwFlags, NPN_BEHIND_ONLY))
				{
					if(TESTBIT( pSpec->dwFlags, NPN_QUARTER_DIRECTIONALITY))
					{
						continue;
					}
					else if(fDot > 0)
					{
						continue;
					}
				}
			}
		}
		float fCheckDistance = INFINITY;
		if( AppIsTugboat() || TESTBIT( pSpec->dwFlags, NPN_USE_XY_DISTANCE) )
		{
#if HELLGATE_ONLY
			fCheckDistance = VectorDistanceSquaredXY(pRoomPathNode->vPosition, vRoomPosition);
#else
			float fX = GetPathNodeX( pRoom->pPathDef, pRoomPathNode->nOffset );
			float fY = GetPathNodeY( pRoom->pPathDef, pRoomPathNode->nOffset );
			fCheckDistance = (vRoomPosition.fX - fX) * (vRoomPosition.fX - fX) + (vRoomPosition.fY - fY) * (vRoomPosition.fY - fY);
#endif
		}
#if HELLGATE_ONLY
		else if( AppIsHellgate() )
		{
			fCheckDistance = VectorDistanceSquared(pRoomPathNode->vPosition, vRoomPosition);
		}
#endif

		int nLengthToUse = nOutputLength;
		if(pSpec->fMinDistance >= 0.0f || pSpec->fMaxDistance >= 0.0f)
		{
			if(pSpec->fMinDistance >= 0.0f && fCheckDistance < pSpec->fMinDistance*pSpec->fMinDistance)
			{
				continue;
			}
			if(pSpec->fMaxDistance >= 0.0f && fCheckDistance > pSpec->fMaxDistance*pSpec->fMaxDistance)
			{
				continue;
			}
		}
		else
		{
			nLengthToUse = 1;
		}

		if(TESTBIT( pSpec->dwFlags, NPN_BIAS_FAR_NODES))
		{
			VECTOR vPathNodeWorldPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);
			VECTOR vNodeDelta = vWorldPosition - vPathNodeWorldPosition;
			vNodeDelta.fZ = 0;
			VectorNormalize(vNodeDelta);
			float fBiasDot = VectorDot(vNodeDelta, pSpec->vFaceDirection);
			if(fBiasDot > -0.5f)
			{
				fBiasDot += 1.0f;
				fCheckDistance += fBiasDot*fBiasDot;
			}
		}
		nCurrentLength = sRoomGetNearestPathNodesAddToOutput(nLengthToUse, pOutput, nCurrentLength, pRoomPathNode, pRoom, fCheckDistance, fDot);
	}
	return nCurrentLength;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline int sRoomGetNearestPathNodesHashIndex(
	const ROOM_PATH_NODE_SET * pPathNodeSet,
	const VECTOR & vRoomPosition)
{
	int nHashIndex = sRoomPathNodeGetHashValue(
		vRoomPosition.fX, vRoomPosition.fY,
		pPathNodeSet->fMinX, pPathNodeSet->fMinY,
		pPathNodeSet->fMaxX, pPathNodeSet->fMaxY,
		sRoomPathNodeGetHashArrayGridSize());
	return nHashIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sRoomGetNearestPathNodesShouldUseHashIndex(
	BOOL bRecurse,
	ROOM_PATH_NODE_SET * pPathNodeSet,
	const VECTOR & vRoomPosition,
	float fXOffset,
	float fYOffset,
	int nHashOffset)
{
	if(!bRecurse)
	{
		return 1;
	}

	VECTOR vOffsetPosition = vRoomPosition;
	vOffsetPosition.fX += fXOffset;
	vOffsetPosition.fY += fXOffset;
	int nOffsetHashIndex = sRoomGetNearestPathNodesHashIndex(pPathNodeSet, vOffsetPosition);
	if(nOffsetHashIndex != nHashOffset)
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomDoGetNearestPathNodes(
									  GAME * pGame,
									  ROOM * pRoom,
									  const VECTOR & vWorldPosition,
									  const VECTOR & vRoomPosition,
									  const int nOutputLength,
									  FREE_PATH_NODE * pOutput,
									  int nCurrentLength,
									  const NEAREST_NODE_SPEC * pSpec,
									  BOOL bRecurse)
{
	ASSERT_RETVAL(pGame && pRoom, nCurrentLength);
	ASSERT_RETVAL(pOutput, nCurrentLength);
	ASSERT_RETVAL(nOutputLength > 0, nCurrentLength);
	ASSERT_RETVAL(pSpec, nCurrentLength);

	ROOM_PATH_NODE_DEFINITION * pDefinition = pRoom->pPathDef;
	ASSERTV_RETVAL(pDefinition, nCurrentLength, "Room '%s' - Expected Definition", RoomGetDevName(pRoom));
	ROOM_PATH_NODE_SET * pPathNodeSet = &pDefinition->pPathNodeSets[0];
	ASSERTV_RETVAL(pPathNodeSet, nCurrentLength, "Room '%s' - Expected Path Node Set", RoomGetDevName(pRoom));

#if HELLGATE_ONLY
	const int nGridSize = sRoomPathNodeGetHashArrayGridSize();

	int nMinDistanceAdjustment = 1;
	if(pSpec->fMinDistance >= 0.0f)
	{
		nMinDistanceAdjustment = int((2*pSpec->fMinDistance) / (3*nGridSize)) + 1;
	}

	float fXOffset = pDefinition->fNodeFrequencyX / 2.0f;
	float fYOffset = pDefinition->fNodeFrequencyY / 2.0f;
	// this is because we have a default frequency value of .45 that isn't saved....
	// legacy crap.
	if( AppIsTugboat() && fXOffset == .5f )
	{
		fXOffset = .225f;
		fYOffset = .225f;
	}

	int nHashIndex = sRoomGetNearestPathNodesHashIndex(pPathNodeSet, vRoomPosition);

	const int nXWidth = sRoomPathNodeGetMultiplier( pPathNodeSet->fMinX, pPathNodeSet->fMaxX, nGridSize );
	const int nYWidth = sRoomPathNodeGetMultiplier( pPathNodeSet->fMinY, pPathNodeSet->fMaxY, nGridSize );
	int nXCenter = (nHashIndex % nXWidth);
	int nXStart = nXCenter - sRoomGetNearestPathNodesShouldUseHashIndex(bRecurse, pPathNodeSet, vRoomPosition, -fXOffset, 0.0f, nHashIndex) * nMinDistanceAdjustment;
	nXStart = PIN(nXStart, 0, nXWidth-1);

	int nXEnd = nXCenter + sRoomGetNearestPathNodesShouldUseHashIndex(bRecurse, pPathNodeSet, vRoomPosition, fXOffset, 0.0f, nHashIndex) * nMinDistanceAdjustment;
	nXEnd = PIN(nXEnd, 0, nXWidth-1);

	int nYCenter = (nHashIndex / nXWidth);
	int nYStart = nYCenter - sRoomGetNearestPathNodesShouldUseHashIndex(bRecurse, pPathNodeSet, vRoomPosition, 0.0f, -fYOffset, nHashIndex) * nMinDistanceAdjustment;
	nYStart = PIN(nYStart, 0, nYWidth-1);

	int nYEnd = nYCenter + sRoomGetNearestPathNodesShouldUseHashIndex(bRecurse, pPathNodeSet, vRoomPosition, 0.0f, fYOffset, nHashIndex) * nMinDistanceAdjustment;
	nYEnd = PIN(nYEnd, 0, nYWidth-1);

	for(int i=nYStart; i<=nYEnd; i += nMinDistanceAdjustment)
	{
		int nYPart = i * nXWidth;
		for(int j=nXStart; j<=nXEnd; j += nMinDistanceAdjustment)
		{
			int nHashIndexToTest = nYPart + j;
			ASSERT_CONTINUE(nHashIndexToTest >= 0);
			ASSERT_CONTINUE(nHashIndexToTest < pPathNodeSet->nArraySize);
			nCurrentLength = sRoomGetNearestPathNodesInArray(
				pGame, 
				pRoom, 
				pPathNodeSet->pNodeHashArray[nHashIndexToTest], 
				pPathNodeSet->nHashLengths[nHashIndexToTest], 
				vWorldPosition, 
				vRoomPosition, 
				nOutputLength, 
				pOutput, 
				nCurrentLength, 
				pSpec);
		}
	}

	if(bRecurse && nCurrentLength <= 0)
	{
		return sRoomDoGetNearestPathNodes(pGame, pRoom, vWorldPosition, vRoomPosition, nOutputLength, pOutput, nCurrentLength, pSpec, FALSE);
	}
#else
	if(pDefinition->fNodeOffsetX == 0 ||
		pDefinition->fNodeOffsetY == 0 )
	{
		return nCurrentLength;
	}

	DWORD dwBlockedNodeFlags = 0;
	if (TESTBIT( pSpec->dwFlags, NPN_NO_SPAWN_NODES_ARE_BLOCKING ))
	{
		SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );
	}


	int nWidth = (int)pDefinition->fNodeOffsetX;
	int X = (int)(( vRoomPosition.fX - pDefinition->vCorner.fX ) / -pDefinition->fNodeFrequencyX + .5f);
	int Y = (int)(( vRoomPosition.fY - pDefinition->vCorner.fY ) / pDefinition->fNodeFrequencyY + .5f );
	X = MAX(0, X );
	Y = MAX(0, Y );
	X = MIN( (int)pDefinition->fNodeOffsetX - 1, X );
	Y = MIN( (int)pDefinition->fNodeOffsetY - 1, Y );

	if( nOutputLength == 1 &&  !TESTBIT(pSpec->dwFlags, NPN_CHECK_DISTANCE) && pSpec->fMinDistance <= 0)
	{
		if( nCurrentLength == 0 )
		{
			 if( pDefinition->pPathNodeSets[0].pNodeIndexMap[Y*nWidth + X] )
			 {
				 ROOM_PATH_NODE * pRoomPathNode = pDefinition->pPathNodeSets[0].pNodeIndexMap[Y*nWidth + X];
				 BOOL bSuccess = TRUE;
				 if(!TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES) || !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
				 {
					 NODE_RESULT eResult;
					 if(pSpec->pUnit)
					 {
						 eResult = s_RoomNodeIsBlockedIgnore( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, pSpec->pUnit );
					 }
					 else
					 {
						 eResult = s_RoomNodeIsBlocked( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, NULL );
					 }
					 if(eResult == NODE_BLOCKED && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
					 {
						 bSuccess = FALSE;
					 }
					 if(eResult == NODE_BLOCKED_UNIT && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES))
					 {
						 bSuccess = FALSE;
					 }
				 }

				 if( bSuccess )
				 {
					 FREE_PATH_NODE * pTarget = &pOutput[nCurrentLength];
					 pTarget->pNode = pDefinition->pPathNodeSets[0].pNodeIndexMap[Y*nWidth + X];
					 pTarget->pRoom = pRoom;
					 pTarget->flDot = 0;
					 pTarget->flDistSq = 0;
					 return 1;
				 }			
			 }
		}
	}
	int X1 = X - MAX( 1, (int)ceil( pSpec->fMaxDistance / pDefinition->fNodeFrequencyX ) );
	int Y1 = Y - MAX( 1, (int)ceil( pSpec->fMaxDistance / pDefinition->fNodeFrequencyY ) );
	X1 = MAX(0, X1 );
	Y1 = MAX(0, Y1 );

	int X2 = X + MAX( 1, (int)ceil( pSpec->fMaxDistance / pDefinition->fNodeFrequencyX ) );
	int Y2 = Y + MAX( 1, (int)ceil( pSpec->fMaxDistance / pDefinition->fNodeFrequencyY ) );
	X2 = MIN( (int)pDefinition->fNodeOffsetX, X2 );
	Y2 = MIN( (int)pDefinition->fNodeOffsetY, Y2 );



	for( int i = X1; i < X2; i++ )
	{
		for( int j = Y1; j < Y2; j++ )
		{
			if( nCurrentLength >= nOutputLength && !TESTBIT(pSpec->dwFlags, NPN_CHECK_DISTANCE))
				break;

			ROOM_PATH_NODE * pRoomPathNode = pDefinition->pPathNodeSets[0].pNodeIndexMap[j*nWidth + i];
			if( pRoomPathNode )
			{
				if(TESTBIT(pSpec->dwFlags, NPN_EDGE_NODES_ONLY))
				{
					if(!(pRoomPathNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG))
					{
						continue;
					}
				}
				if(!TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES) || !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
				{
					NODE_RESULT eResult;
					if(pSpec->pUnit)
					{
						eResult = s_RoomNodeIsBlockedIgnore( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, pSpec->pUnit );
					}
					else
					{
						eResult = s_RoomNodeIsBlocked( pRoom, pRoomPathNode->nIndex, dwBlockedNodeFlags, NULL );
					}
					if(eResult == NODE_BLOCKED && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_BLOCKED_NODES))
					{
						continue;
					}
					if(eResult == NODE_BLOCKED_UNIT && !TESTBIT(pSpec->dwFlags, NPN_ALLOW_OCCUPIED_NODES))
					{
						continue;
					}
				}
				if( TESTBIT(pSpec->dwFlags, NPN_CHECK_DISTANCE) )
				{
					float fX = GetPathNodeX( pRoom->pPathDef, pRoomPathNode->nOffset );
					float fY = GetPathNodeY( pRoom->pPathDef, pRoomPathNode->nOffset );
					float fCheckDistance = (vRoomPosition.fX - fX) * (vRoomPosition.fX - fX) + (vRoomPosition.fY - fY) * (vRoomPosition.fY - fY);

					if(pSpec->fMinDistance >= 0.0f )
					{
						if(pSpec->fMinDistance >= 0.0f && fCheckDistance < pSpec->fMinDistance*pSpec->fMinDistance)
						{
							continue;
						}
					}
					if(TESTBIT( pSpec->dwFlags, NPN_BIAS_FAR_NODES))
					{
						VECTOR vPathNodeWorldPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);
						VECTOR vNodeDelta = vWorldPosition - vPathNodeWorldPosition;
						vNodeDelta.fZ = 0;
						VectorNormalize(vNodeDelta);
						float fBiasDot = VectorDot(vNodeDelta, pSpec->vFaceDirection);
						if(fBiasDot > -0.5f)
						{
							fBiasDot += 1.0f;
							fCheckDistance += fBiasDot*fBiasDot;
						}
					}

						if( nCurrentLength > 0 &&
							fCheckDistance < pOutput[0].flDistSq )
						{
							
							FREE_PATH_NODE * pTarget = &pOutput[0];
							pTarget->pNode = pRoomPathNode;
							pTarget->pRoom = pRoom;
							pTarget->flDot = 0;
							pTarget->flDistSq = fCheckDistance;
							nCurrentLength = MAX( 1, nCurrentLength );
						}
						else if( nCurrentLength < nOutputLength )
						{
							FREE_PATH_NODE * pTarget = &pOutput[nCurrentLength];
							pTarget->pNode = pRoomPathNode;
							pTarget->pRoom = pRoom;
							pTarget->flDot = 0;
							pTarget->flDistSq = fCheckDistance;
							nCurrentLength++;
						}

						
				
				}
				else
				{

					FREE_PATH_NODE * pTarget = &pOutput[nCurrentLength];
					pTarget->pNode = pRoomPathNode;
					pTarget->pRoom = pRoom;
					pTarget->flDot = 0;
					pTarget->flDistSq = (float)abs( i - X ) + abs( j - Y );
					nCurrentLength++;
				}

			}
		}
	}
#endif
	return nCurrentLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomDoGetNearestPathNodes(
									  GAME * pGame,
									  ROOM * pRoom,
									  const VECTOR & vWorldPosition,
									  const VECTOR & vRoomPosition,
									  const int nOutputLength,
									  FREE_PATH_NODE * pOutput,
									  int nCurrentLength,
									  const NEAREST_NODE_SPEC * pSpec)
{
	if(pSpec->fMinDistance < 0.0f && pSpec->fMaxDistance < 0.0f)
	{
		return sRoomDoGetNearestPathNodes(pGame, pRoom, vWorldPosition, vRoomPosition, nOutputLength, pOutput, nCurrentLength, pSpec, TRUE);
	}
	return sRoomDoGetNearestPathNodes(pGame, pRoom, vWorldPosition, vRoomPosition, nOutputLength, pOutput, nCurrentLength, pSpec, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomDoGetNearestPathNodesExpensive(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const VECTOR & vRoomPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	int nCurrentLength,
	const NEAREST_NODE_SPEC * pSpec)
{
#if HELLGATE_ONLY
	ASSERT_RETVAL(pGame && pRoom, nCurrentLength);
	ASSERT_RETVAL(pOutput, nCurrentLength);
	ASSERT_RETVAL(nOutputLength > 0, nCurrentLength);
	ASSERT_RETVAL(pSpec, nCurrentLength);

	ROOM_PATH_NODE_DEFINITION * pDefinition = pRoom->pPathDef;
	ASSERTV_RETVAL(pDefinition, nCurrentLength, "Room '%s' - Expected Definition", RoomGetDevName(pRoom));
	ROOM_PATH_NODE_SET * pPathNodeSet = &pDefinition->pPathNodeSets[0];
	ASSERTV_RETVAL(pPathNodeSet, nCurrentLength, "Room '%s' - Expected Path Node Set", RoomGetDevName(pRoom));

	const int nGridSize = sRoomPathNodeGetHashArrayGridSize();
	const int nXWidth = sRoomPathNodeGetMultiplier( pPathNodeSet->fMinX, pPathNodeSet->fMaxX, nGridSize );
	const int nYWidth = sRoomPathNodeGetMultiplier( pPathNodeSet->fMinY, pPathNodeSet->fMaxY, nGridSize );

	for(int i=0; i<nYWidth; i++)
	{
		int nYPart = i * nXWidth;
		for(int j=0; j<nXWidth; j++)
		{
			int nHashIndexToTest = nYPart + j;
			ASSERT_CONTINUE(nHashIndexToTest >= 0);
			ASSERT_CONTINUE(nHashIndexToTest < pPathNodeSet->nArraySize);
			nCurrentLength = sRoomGetNearestPathNodesInArray(
				pGame, 
				pRoom, 
				pPathNodeSet->pNodeHashArray[nHashIndexToTest], 
				pPathNodeSet->nHashLengths[nHashIndexToTest], 
				vWorldPosition, 
				vRoomPosition, 
				nOutputLength, 
				pOutput, 
				nCurrentLength, 
				pSpec);
		}
	}
#endif
	return nCurrentLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomGetNearestPathNodesSortCompare(
	const void * pLeft,
	const void * pRight)
{
	const FREE_PATH_NODE * pNodeLeft = (FREE_PATH_NODE *)pLeft;
	const FREE_PATH_NODE * pNodeRight = (FREE_PATH_NODE *)pRight;
	if(pNodeLeft->flDistSq < pNodeRight->flDistSq)
	{
		return -1;
	}
	else if(pNodeLeft->flDistSq > pNodeRight->flDistSq)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static int sRoomGetNearestPathNodesPostProcessDbg(
	GAME * pGame,
	const VECTOR & vWorldPositionCenter,
	FREE_PATH_NODE * pOutput,
	int nOutputLength,
	const NEAREST_NODE_SPEC * pSpec,
	const char * file,
	int line)
#else
static int sRoomGetNearestPathNodesPostProcess(
	GAME * pGame,
	const VECTOR & vWorldPositionCenter,
	FREE_PATH_NODE * pOutput,
	int nOutputLength,
	const NEAREST_NODE_SPEC * pSpec)
#endif
{
#if ISVERSION(DEVELOPMENT)
	FL_ASSERT_RETVAL(pGame, nOutputLength, file, line);
	if( nOutputLength == 0 )
	{
		return 0;
	}
	FL_ASSERT_RETVAL(pOutput, nOutputLength, file, line);
#else
	ASSERT_RETVAL(pGame, nOutputLength);
	if( nOutputLength == 0 )
	{
		return 0;
	}
	ASSERT_RETVAL(pOutput, nOutputLength);
#endif

	// LoS here
	if(TESTBIT( pSpec->dwFlags, NPN_CHECK_LOS))
	{
		LEVEL * pLevel = RoomGetLevel(pOutput[0].pRoom);
		ASSERT_RETVAL(pLevel, nOutputLength);

		// elevate the start pos pos off the ground a little bit for LOS checks
		VECTOR vStartPos = vWorldPositionCenter;
		vStartPos.fZ += 1.0f;  // elevate off the ground a little bit

		DWORD dwCollideFlags = 0;
		if(TESTBIT( pSpec->dwFlags, NPN_CHECK_LOS_AGAINST_OBJECTS))
		{
			SETBIT(dwCollideFlags, LCF_CHECK_OBJECT_CYLINDER_COLLISION_BIT);
		}

		for(int i=0; i<nOutputLength; i++)
		{
			const float flSlop = 0.5f;

			VECTOR vNodeWorldPosition;
			if(!RoomTransformFreePathNode(&pOutput[i], &vNodeWorldPosition))
			{
				continue;
			}
			// A little bit more than the slop
			vNodeWorldPosition.fZ += flSlop + 0.1f;

			VECTOR vToNodePosWorld = vNodeWorldPosition - vStartPos;
			float flDistance = VectorNormalize(vToNodePosWorld);

			// do a line of sight check			
			float flDistToCollide = LevelLineCollideLen( 
				pGame,
				pLevel,
				vStartPos,
				vToNodePosWorld,
				flDistance,
				NULL,
				dwCollideFlags);
			if (flDistToCollide < (flDistance - flSlop))
			{
				if(i < nOutputLength-1)
				{
					MemoryMove(
						&pOutput[i],
						sizeof(FREE_PATH_NODE) * (nOutputLength-i),
						&pOutput[i + 1],
						sizeof(FREE_PATH_NODE) * (nOutputLength-i-1));
				}
				nOutputLength--;
				i--;
			}
		}
	}

	// Very short lists don't need to be sorted
	if(nOutputLength <= 1)
	{
		return nOutputLength;
	}

	if(TESTBIT( pSpec->dwFlags, NPN_SORT_OUTPUT) || TESTBIT( pSpec->dwFlags, NPN_CHECK_LOS))
	{
		qsort(pOutput, nOutputLength, sizeof(FREE_PATH_NODE), sRoomGetNearestPathNodesSortCompare);
	}

	return nOutputLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomGetNearestPathNodesCheap(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	const NEAREST_NODE_SPEC * pSpec)
{
	VECTOR vRoomPosition(0);
	int nCurrentOutputLength = 0;

	int nRoomCount = (TESTBIT(pSpec->dwFlags, NPN_ONE_ROOM_ONLY)) ? 0 : RoomGetAdjacentRoomCount(pRoom);
	for(int i=-1; i<nRoomCount; i++)
	{
		ROOM * pTestRoom = pRoom;
		if(i >= 0)
		{
			pTestRoom = RoomGetAdjacentRoom(pRoom, i);
		}
		if( !pTestRoom )
		{
			continue;
		}
		if(TESTBIT( pSpec->dwFlags, NPN_POSITION_IS_ALREADY_IN_ROOM_SPACE))
		{
			vRoomPosition = vWorldPosition;
		}
		else
		{
			MatrixMultiply(&vRoomPosition, &vWorldPosition, &pTestRoom->tWorldMatrixInverse);
		}
		nCurrentOutputLength = sRoomDoGetNearestPathNodes(pGame, pTestRoom, vWorldPosition, vRoomPosition, nOutputLength, pOutput, nCurrentOutputLength, pSpec);
	}

	return nCurrentOutputLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomGetNearestPathNodesExpensive(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	const NEAREST_NODE_SPEC * pSpec)
{
	if(IS_SERVER(pGame))
	{
		return sRoomGetNearestPathNodesCheap(pGame, pRoom, vWorldPosition, nOutputLength, pOutput, pSpec);
	}

	VECTOR vRoomPosition(0);
	int nCurrentOutputLength = 0;

	int nRoomCount = (TESTBIT(pSpec->dwFlags, NPN_ONE_ROOM_ONLY)) ? 0 : RoomGetNearbyRoomCount(pRoom);
	for(int i=-1; i<nRoomCount; i++)
	{
		ROOM * pTestRoom = pRoom;
		if(i >= 0)
		{
			pTestRoom = RoomGetNearbyRoom(pRoom, i);
		}
		if( !pTestRoom )
		{
			continue;
		}
		if(TESTBIT( pSpec->dwFlags, NPN_POSITION_IS_ALREADY_IN_ROOM_SPACE))
		{
			vRoomPosition = vWorldPosition;
		}
		else
		{
			MatrixMultiply(&vRoomPosition, &vWorldPosition, &pTestRoom->tWorldMatrixInverse);
		}

		nCurrentOutputLength = sRoomDoGetNearestPathNodesExpensive(pGame, pTestRoom, vWorldPosition, vRoomPosition, nOutputLength, pOutput, nCurrentOutputLength, pSpec);
	}

	return nCurrentOutputLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
int RoomGetNearestPathNodesDbg(
							   GAME * pGame,
							   ROOM * pRoom,
							   const VECTOR & vWorldPosition,
							   const int nOutputLength,
							   FREE_PATH_NODE * pOutput,
							   const NEAREST_NODE_SPEC * pSpec,
							   const char * file,
							   int line)
#else
int RoomGetNearestPathNodes(
							GAME * pGame,
							ROOM * pRoom,
							const VECTOR & vWorldPosition,
							const int nOutputLength,
							FREE_PATH_NODE * pOutput,
							const NEAREST_NODE_SPEC * pSpec)
#endif
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,RoomGetNearestPathNodes,1);
#endif
#if ISVERSION(DEVELOPMENT)
	FL_ASSERT_RETZERO(pGame, file, line);

	if( AppIsTugboat() && !pRoom )
	{
		return 0;
	}
	FL_ASSERT_RETZERO(pRoom, file, line);

	FL_ASSERT_RETZERO(nOutputLength > 0, file, line);
	FL_ASSERT_RETZERO(pOutput, file, line);
#else
	ASSERT_RETZERO(pGame);

	if( AppIsTugboat() && !pRoom )
	{
		return 0;
	}
	ASSERT_RETZERO(pRoom);

	ASSERT_RETZERO(nOutputLength > 0);
	ASSERT_RETZERO(pOutput);
#endif

	NEAREST_NODE_SPEC tNearestNodeSpec;
	if(!pSpec)
	{
		pSpec = &tNearestNodeSpec;
	}

	// We'll go to one room first and get all the nodes from there, then check out its neighbors
	if(!TESTBIT(pSpec->dwFlags, NPN_USE_GIVEN_ROOM))
	{
		ROOM * pOriginalRoom = pRoom;
		pRoom = RoomGetFromPosition(pGame, pRoom, &vWorldPosition);
		if(!pRoom || sRoomHasNoPathNodes(pRoom))
		{
			pRoom = pOriginalRoom;
		}
	}

	int nCurrentOutputLength = 0;
	if(TESTBIT( pSpec->dwFlags, NPN_USE_EXPENSIVE_SEARCH))
	{
		nCurrentOutputLength = sRoomGetNearestPathNodesExpensive(pGame, pRoom, vWorldPosition, nOutputLength, pOutput, pSpec);
	}
	else
	{
		nCurrentOutputLength = sRoomGetNearestPathNodesCheap(pGame, pRoom, vWorldPosition, nOutputLength, pOutput, pSpec);
	}

#if ISVERSION(DEVELOPMENT)
	nCurrentOutputLength = sRoomGetNearestPathNodesPostProcessDbg(pGame, vWorldPosition, pOutput, nCurrentOutputLength, pSpec, file, line);
#else
	nCurrentOutputLength = sRoomGetNearestPathNodesPostProcess(pGame, vWorldPosition, pOutput, nCurrentOutputLength, pSpec);
#endif
	return nCurrentOutputLength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
ROOM_PATH_NODE * RoomGetNearestPathNodeDbg(
	GAME * pGame,
	ROOM * pRoom,
	VECTOR vWorldPosition,
	ROOM ** pDestinationRoom,
	const char * file,
	int line,
	DWORD dwFlags)
#else
ROOM_PATH_NODE * RoomGetNearestPathNode(
										GAME * pGame,
										ROOM * pRoom,
										VECTOR vWorldPosition,
										ROOM ** pDestinationRoom,
										DWORD dwFlags)
#endif
{
#if ISVERSION(DEVELOPMENT)
	FL_ASSERT_RETNULL(pDestinationRoom, file, line);
#else
	ASSERT_RETNULL(pDestinationRoom);
#endif
	*pDestinationRoom = NULL;
#if ISVERSION(DEVELOPMENT)
	FL_ASSERT_RETNULL(pGame, file, line);
	FL_ASSERT_RETNULL(pRoom, file, line);
#else
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(pRoom);
#endif

	FREE_PATH_NODE tNearestPathNode;
	NEAREST_NODE_SPEC tSpec;
	tSpec.dwFlags = dwFlags;
	SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
	SETBIT(tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
	if( AppIsTugboat() )
	{
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
	}
#if ISVERSION(DEVELOPMENT)
	int nCount = RoomGetNearestPathNodesDbg(pGame, pRoom, vWorldPosition, 1, &tNearestPathNode, &tSpec, file, line);
#else
	int nCount = RoomGetNearestPathNodes(pGame, pRoom, vWorldPosition, 1, &tNearestPathNode, &tSpec);
#endif
	if( AppIsHellgate() )
	{
		if(!TESTBIT(dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY))
		{
#if ISVERSION(DEVELOPMENT)
			FL_ASSERTX_RETNULL(nCount == 1, "Unable to find nearest path node at all", file, line);
			FL_ASSERTX_RETNULL(tNearestPathNode.pNode, "Nearest path node is null", file, line);
			FL_ASSERTX_RETNULL(tNearestPathNode.pRoom, "Nearest path node room is null", file, line);
#else
			ASSERTX_RETNULL(nCount == 1, "Unable to find nearest path node at all");
			ASSERTX_RETNULL(tNearestPathNode.pNode, "Nearest path node is null");
			ASSERTX_RETNULL(tNearestPathNode.pRoom, "Nearest path node room is null");
#endif
		}
	}
	if(nCount != 1 || !tNearestPathNode.pNode || !tNearestPathNode.pRoom)
	{
		return NULL;
	}
	*pDestinationRoom = tNearestPathNode.pRoom;

	return tNearestPathNode.pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * RoomGetNearestPathNodeRoomSpace(
	ROOM * pRoom,
	VECTOR vRoomSpacePosition,
	BOOL bAllowNull)
{
	ASSERT_RETNULL(pRoom);
	LEVEL * pLevel = RoomGetLevel(pRoom);
	ASSERT_RETNULL(pLevel);
	GAME * pGame = LevelGetGame(pLevel);
	ASSERT_RETNULL(pGame);

	FREE_PATH_NODE tNearestPathNode;
	structclear(tNearestPathNode);
	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
	SETBIT(tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
	SETBIT(tSpec.dwFlags, NPN_ONE_ROOM_ONLY);
	SETBIT(tSpec.dwFlags, NPN_USE_GIVEN_ROOM);

	// Clear these because we have no world position
	CLEARBIT(tSpec.dwFlags, NPN_IN_FRONT_ONLY);
	CLEARBIT(tSpec.dwFlags, NPN_BEHIND_ONLY);
	CLEARBIT(tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY);
	CLEARBIT(tSpec.dwFlags, NPN_BIAS_FAR_NODES);
	int nCount = sRoomDoGetNearestPathNodes(pGame, pRoom, vRoomSpacePosition, vRoomSpacePosition, 1, &tNearestPathNode, 0, &tSpec);
	if(!bAllowNull)
	{
		ASSERTX_RETNULL(nCount == 1, "Unable to find nearest path node at all");
		ASSERTX_RETNULL(tNearestPathNode.pNode, "Nearest path node is null");
		ASSERTX_RETNULL(tNearestPathNode.pRoom, "Nearest path node room is null");
		ASSERTX_RETNULL(tNearestPathNode.pRoom == pRoom, "Nearest path node room does not match input room");
	}
	return tNearestPathNode.pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomPathNodeIsConnected(
							 ROOM * pRoomSource,
							 ROOM_PATH_NODE * pPathNodeSource,
							 ROOM * pRoomDest,
							 ROOM_PATH_NODE * pPathNodeDest)
{
	ASSERT_RETFALSE(pRoomSource);
	ASSERT_RETFALSE(pPathNodeSource);
	ASSERT_RETFALSE(pRoomDest);
	ASSERT_RETFALSE(pPathNodeDest);

	// Different levels means that they can't possibly be connected
	if(RoomGetLevel(pRoomSource) != RoomGetLevel(pRoomDest))
	{
		return FALSE;
	}

	// If the two path nodes are in the same room, then it's an easier check
	if(pRoomSource->idRoom == pRoomDest->idRoom)
	{
		// Just iterate through all of the connections of the source and see if any of them matches the destination
		for(int i=0; i<pPathNodeSource->nConnectionCount; i++)
		{
			if(pPathNodeSource->pConnections[i].nConnectionIndex == pPathNodeDest->nIndex)
			{
				// If the indexes match, we've found a connection and we're done.
				return TRUE;
			}
		}
	}
	else
	{
		// The two nodes are in different rooms - this is a more complex check

		// First, if the source node isn't an edge node, then there's no way it's going to be connected
		int nIndex = pPathNodeSource->nEdgeIndex;
		if(nIndex < 0)
		{
			return FALSE;
		}

		// If the source room has no edge instances, then we must assume that they are not connected
		if(!RoomTestFlag(pRoomSource, ROOM_EDGENODE_CONNECTIONS))
		{
			return FALSE;
		}

		// Get the edge instance from the source room
		EDGE_NODE_INSTANCE * pInstance = &pRoomSource->pEdgeInstances[nIndex];
		if(!pInstance)
		{
			return FALSE;
		}

		// Each edge instance contains a list of edge connections
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pRoomSource->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];

			// If this connection's room doesn't match the destination room, then it can't be what we're looking for
			if(pConnection->pRoom->idRoom != pRoomDest->idRoom)
			{
				continue;
			}

			// We've got the right room, see if it's the right node.
			int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
			ROOM_PATH_NODE * pPathNodeTest = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
			if(!pPathNodeTest)
			{
				continue;
			}
			if(pPathNodeTest->nIndex != pPathNodeDest->nIndex)
			{
				continue;
			}

			// If it was the wrong node, we would have exited early.  At this point, it's the right room, the right node.  We're done.
			return TRUE;
		}
	}

	// No luck.  Insert quarter and try again, thank you for playing.
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomPathNodeIterateConnections(
									ROOM * pRoom,
									ROOM_PATH_NODE * pRoomPathNode,
									PFN_PATHNODE_ITERATE pfnIterate,
									void * pUserData)
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pRoomPathNode);
	ASSERT_RETURN(pfnIterate);

	for(int i=0; i<pRoomPathNode->nConnectionCount; i++)
	{
		ROOM_PATH_NODE_CONNECTION * pConnection = &pRoomPathNode->pConnections[i];
#if HELLGATE_ONLY
		if(pConnection->pConnection)
		{
			if(!pfnIterate(pRoom, pRoomPathNode, pRoom, pConnection->pConnection, pConnection->fDistance, pUserData))
			{
				return;
			}
		}
#else
		ROOM_PATH_NODE * pConnectionNode = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[pConnection->nConnectionIndex];

		if(pConnectionNode)
		{

			if(!pfnIterate(pRoom, pRoomPathNode, pRoom, pConnectionNode, PATH_NODE_DISTANCE, pUserData))
			{
				return;
			}
		}
#endif
	}

	if((pRoomPathNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pRoom, ROOM_EDGENODE_CONNECTIONS))
	{
		int nIndex = pRoomPathNode->nEdgeIndex;
		EDGE_NODE_INSTANCE * pInstance = &pRoom->pEdgeInstances[nIndex];
		for(int i=0; i<pInstance->nEdgeConnectionCount; i++)
		{
			PATH_NODE_EDGE_CONNECTION * pConnection = &pRoom->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + i];
			int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
			ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];
			if(pOtherRoomNode)
			{
				if(!pfnIterate(pRoom, pRoomPathNode, pConnection->pRoom, pOtherRoomNode, pConnection->fDistance, pUserData))
				{
					return;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RoomPathNodeGetHappyPlaceCount(
								   GAME * pGame,
								   ROOM * pRoom)
{
	REF(pGame);
	ASSERT_RETZERO(pRoom);
	ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoom);
	if(!pPathNodeSet)
		return 0;
	return pPathNodeSet->nHappyNodeCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * RoomPathNodeGetHappyPlace(
	GAME * pGame,
	ROOM * pRoom,
	int nIndex)
{
	REF(pGame);
	ASSERT_RETZERO(pRoom);
	ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoom);
	if(!pPathNodeSet)
		return NULL;

	ASSERT_RETZERO(nIndex >= 0 && nIndex < pPathNodeSet->nHappyNodeCount);

	return &pPathNodeSet->pPathNodes[pPathNodeSet->pHappyNodes[nIndex]];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * RoomPathNodeGetNearestEdgeNodeToPoint(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition)
{
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(pRoom);

	ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoom);
	ASSERT_RETNULL(pPathNodeSet);
	ASSERT_RETNULL(pPathNodeSet->pEdgeNodes);

	float fNearestDistanceSquared = INFINITY;
	ROOM_PATH_NODE * pNearestNode = NULL;

	for(int i=0; i<pPathNodeSet->nEdgeNodeCount; i++)
	{
		ROOM_PATH_NODE * pPathNode = pPathNodeSet->pEdgeNodes[i];
		ASSERT_CONTINUE(pPathNode);

		VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
		float fDistanceSquared = VectorDistanceSquared(vWorldPosition, vNodePosition);

		if(fDistanceSquared < fNearestDistanceSquared || pNearestNode == NULL)
		{
			pNearestNode = pPathNode;
			fNearestDistanceSquared = fDistanceSquared;
		}
	}

	return pNearestNode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * RoomPathNodeGetNearestNodeToPoint(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition)
{
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(pRoom);

	//ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoom);
	//ASSERT_RETNULL(pPathNodeSet);

	float fNearestDistanceSquared = INFINITY;
	ROOM_PATH_NODE * pNearestNode = NULL;

	for(unsigned int i=0; i<pRoom->nPathNodeCount; i++)
	{
		ROOM_PATH_NODE * pRoomPathNode = RoomGetRoomPathNode(pRoom, i);
		VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pRoomPathNode);


		//ROOM_PATH_NODE * pPathNode = &pPathNodeSet->pPathNodes[i];
		//ASSERT_CONTINUE(pPathNode);

		//VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
		float fDistanceSquared = VectorDistanceSquared(vWorldPosition, vNodePosition);

		if(fDistanceSquared < fNearestDistanceSquared || pNearestNode == NULL)
		{
			pNearestNode = pRoomPathNode;
			fNearestDistanceSquared = fDistanceSquared;
		}
	}

	return pNearestNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct SIMPLE_PATH_EXISTS_CONTEXT
{
	ROOM * pClosestRoom;
	ROOM_PATH_NODE * pClosestPathNode;
	float fClosestDistanceSquared;
	VECTOR vTestPosition;
};

static BOOL sSimplePathExistsIterate(
									 ROOM * pRoomSource,
									 ROOM_PATH_NODE * pPathNodeSource,
									 ROOM * pRoomDest,
									 ROOM_PATH_NODE * pPathNodeDest,
									 float fDistance,
									 void * pUserData)
{
	ASSERT_RETFALSE(pUserData);
	SIMPLE_PATH_EXISTS_CONTEXT *pContext = (SIMPLE_PATH_EXISTS_CONTEXT*)pUserData;

	VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoomDest), pRoomDest, pPathNodeDest);
	float fDistanceSquared = VectorDistanceSquared(vNodePosition, pContext->vTestPosition);
	if(fDistanceSquared <= pContext->fClosestDistanceSquared)
	{
		pContext->fClosestDistanceSquared = fDistanceSquared;
		pContext->pClosestRoom = pRoomDest;
		pContext->pClosestPathNode = pPathNodeDest;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomPathNodeDoesSimplePathExistFromNodeToPoint(
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	const VECTOR & vPosition)
{
	ASSERT_RETFALSE(pRoom);
	ASSERT_RETFALSE(pPathNode);
	GAME * pGame = RoomGetGame(pRoom);
	ASSERT_RETFALSE(pGame);

	int nPerformanceLimiter = 100;

	ROOM * pRoomTest = pRoom;
	ROOM_PATH_NODE * pPathNodeTest = pPathNode;
	BOOL bAnswerHasBeenDivined = FALSE;
	do 
	{
		VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(pGame, pRoomTest, pPathNodeTest);
		float fDistanceSquared = VectorDistanceSquared(vNodePosition, vPosition);
#if HELLGATE_ONLY
		if(fDistanceSquared < pPathNodeTest->fRadius * pPathNodeTest->fRadius)
		{
			return TRUE;
		}
#else
		if(fDistanceSquared < PATH_NODE_RADIUS * PATH_NODE_RADIUS)
		{
			return TRUE;
		}
#endif
		SIMPLE_PATH_EXISTS_CONTEXT tContext;
		tContext.pClosestRoom = pRoomTest;
		tContext.pClosestPathNode = pPathNodeTest;
		tContext.fClosestDistanceSquared = fDistanceSquared;
		tContext.vTestPosition = vPosition;
		RoomPathNodeIterateConnections(pRoomTest, pPathNodeTest, sSimplePathExistsIterate, &tContext);

		if(tContext.pClosestRoom != pRoomTest || tContext.pClosestPathNode != pPathNodeTest)
		{
			pRoomTest = tContext.pClosestRoom;
			pPathNodeTest = tContext.pClosestPathNode;
		}
		else
		{
			bAnswerHasBeenDivined = TRUE;
		}
		nPerformanceLimiter--;
	}
	while(!bAnswerHasBeenDivined && nPerformanceLimiter > 0);

	return FALSE;
}
