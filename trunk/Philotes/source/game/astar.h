//----------------------------------------------------------------------------
// astar.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ASTAR_H_
#define _ASTAR_H_

struct ASTAR_OUTPUT
{
	ROOMID nRoomId;
	unsigned int nPathNodeIndex;
};

struct PATH_NODE_SHOULD_TEST 
{
	int						nId;
	PATH_NODE_SHOULD_TEST *	pNext;

	ROOMID					idRoom;
	int						nPathNodeIndex;
	BOOL					bResult;
};


#define DEFAULT_ASTAR_LIST_SIZE 192

BOOL UnitCalculatePath(
	struct GAME* game,
	struct UNIT * unit,
	struct UNIT * targetunit,
	int	* nPathNodeCount,
	ASTAR_OUTPUT * pPathNodes,
	const VECTOR & vDestination,
	int nMaxPathCalculationMethod,
	DWORD dwFlags,
	int nMaxPathNodeCount = -1);

BOOL AStarCalculatePath(
	struct GAME * pGame,
	int	* nPathNodeCount,
	ASTAR_OUTPUT * pPathNodes,
	struct PATH_STATE * pPathState,
	struct ROOM * pSourceRoom,
	struct ROOM_PATH_NODE * pSourceNode,
	struct ROOM * pDestinationRoom,
	struct ROOM_PATH_NODE * pDestinationNode,
	DWORD dwFlags);

VECTOR AStarGetWorldPosition(
	struct GAME * pGame, 
	ASTAR_OUTPUT * pOutputNode );

VECTOR AStarGetWorldPosition(
	GAME * pGame,
	ROOMID idRoom,
	int nNodeIndex );

#if HELLGATE_ONLY
VECTOR AStarGetWorldNormal(
	struct GAME * pGame, 
	ASTAR_OUTPUT * pOutputNode );

VECTOR AStarGetWorldNormal(
	GAME * pGame,
	ROOMID idRoom,
	int nNodeIndex );
#endif
void AStarPathSpline(
	GAME * pGame,
	ASTAR_OUTPUT * alist,
	int list_size,
	int index,
	VECTOR * vPrevious,
	VECTOR * vNew,
	VECTOR * vFace,
	float delta );

#endif
