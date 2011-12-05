//----------------------------------------------------------------------------
// room_path.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ROOM_PATH_H_
#define _ROOM_PATH_H_

#ifndef _DEFINITION_COMMON_H_
#include "definition_common.h"
#endif

#define PATH_NODE_RADIUS .6
#define PATH_NODE_DISTANCE 1

//----------------------------------------------------------------------------
enum
{
	ROOM_PATH_NODE_CONNECTION_JUMP,
};

#define ROOM_PATH_NODE_CONNECTION_JUMP_FLAG	MAKE_MASK(ROOM_PATH_NODE_CONNECTION_JUMP)
#define PATH_MASK_FILE_MAGIC_NUMBER			0xd2d21d25
#define PATH_MASK_FILE_VERSION				(29 + GLOBAL_FILE_VERSION)
#define PATH_MASK_FILE_EXTENSION "_path.rmk"
//-------------------------------------------------------------------------------------------------
struct ROOM_PATH_MASK_DEFINITION
{
	DEFINITION_HEADER		tHeader;
	SAFE_PTR(BYTE*,			pbyFileStart);
	char					pszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	int						nWidth;
	int						nHeight;
	BOOL*					pbPassability;
};


//----------------------------------------------------------------------------
struct ROOM_PATH_NODE_CONNECTION
{
	int							nConnectionIndex;
#if HELLGATE_ONLY
	struct ROOM_PATH_NODE *		pConnection;
	DWORD						dwFlags;
	float						fDistance;
	float						fHeight;
#endif
};

//----------------------------------------------------------------------------
struct ROOM_PATH_NODE_CONNECTION_REF 
{
	int							nConnectionIndex;
	ROOM_PATH_NODE_CONNECTION * pConnection;
};

//----------------------------------------------------------------------------
enum ROOM_PATH_NODE_FLAG
{
	ROOM_PATH_NODE_EDGE,
	ROOM_PATH_NODE_TRAVERSED,
};

#define ROOM_PATH_NODE_EDGE_FLAG		MAKE_MASK(ROOM_PATH_NODE_EDGE)
#define ROOM_PATH_NODE_TRAVERSED_FLAG	MAKE_MASK(ROOM_PATH_NODE_TRAVERSED)

//----------------------------------------------------------------------------
struct ROOM_PATH_NODE
{
	int							nIndex;
	int							nEdgeIndex;

#if HELLGATE_ONLY
	VECTOR						vPosition;
	VECTOR						vNormal;
	float						fHeight;
	float						fRadius;
#else
	int							nOffset;
#endif

	DWORD						dwFlags;

#if HELLGATE_ONLY
	int							nConnectionCount;
#else
	byte						nConnectionCount;
#endif
	ROOM_PATH_NODE_CONNECTION *	pConnections;
#if HELLGATE_ONLY
	int								nLongConnectionCount;
	ROOM_PATH_NODE_CONNECTION_REF * pLongConnections;
	int								nShortConnectionCount;
	ROOM_PATH_NODE_CONNECTION_REF * pShortConnections;
#endif

};

//----------------------------------------------------------------------------
struct ROOM_PATH_NODE_SET
{
	char						pszIndex[MAX_XML_STRING_LENGTH];

	int							nPathNodeCount;
	ROOM_PATH_NODE *			pPathNodes;
	DWORD						dwFlags;

	int							nEdgeNodeCount;
	ROOM_PATH_NODE **			pEdgeNodes;

	ROOM_PATH_NODE_CONNECTION *	pConnections;

	int							nHappyNodeCount;
	int *						pHappyNodes;


	// Hash of room path nodes
	float						fMinX;
	float						fMaxX;
	float						fMinY;
	float						fMaxY;
	int							nArraySize;
#if HELLGATE_ONLY
	ROOM_PATH_NODE ***			pNodeHashArray;
	int *						nHashLengths;
#else
	ROOM_PATH_NODE **			pNodeIndexMap;
#endif
};

//----------------------------------------------------------------------------
enum
{
	ROOM_PATH_NODE_DEF_INDOOR,
	ROOM_PATH_NODE_DEF_NO_PATHNODES,
	ROOM_PATH_NODE_DEF_USE_TUGBOAT,
};

#define ROOM_PATH_NODE_DEF_INDOOR_FLAG			MAKE_MASK(ROOM_PATH_NODE_DEF_INDOOR)
#define ROOM_PATH_NODE_DEF_NO_PATHNODES_FLAG	MAKE_MASK(ROOM_PATH_NODE_DEF_NO_PATHNODES)
#define ROOM_PATH_NODE_DEF_USE_TUGBOAT_FLAG		MAKE_MASK(ROOM_PATH_NODE_DEF_USE_TUGBOAT)

//----------------------------------------------------------------------------
struct ROOM_PATH_NODE_DEFINITION
{
	DEFINITION_HEADER			tHeader;

	int							nPathNodeSetCount;
	ROOM_PATH_NODE_SET *		pPathNodeSets;

	DWORD						dwFlags;
	float						fRadius;
	float						fNodeFrequencyX;
	float						fNodeFrequencyY;
	float						fDiagDistBetweenNodesSq;
	float						fDiagDistBetweenNodes;
	float						fNodeOffsetX;
	float						fNodeOffsetY;
	float						fNodeMinZ;
	float						fNodeMaxZ;
	BOOL						bExists;
	VECTOR						vCorner;
};

#define ROOM_PATH_NODE_SUFFIX "_path.xml"

BOOL RoomPathNodeDefinitionPostXMLLoad(void * pDef, BOOL bForceSyncLoad);

//----------------------------------------------------------------------------
struct FREE_PATH_NODE
{
	ROOM_PATH_NODE *					pNode;									// the path node
	struct ROOM *						pRoom;									// the room it's in
	float flDistSq;																// distance to position of query center
	float flDot;
};

BOOL RoomTransformFreePathNode(
	FREE_PATH_NODE *pFreePathNode,
	VECTOR *pvTransformed);

#if ISVERSION(DEVELOPMENT)
#define RoomGetNearestFreePathNode(game, level, position, destinationroom, ...) RoomGetNearestFreePathNodeDbg(game, level, position, destinationroom, __FILE__, __LINE__, __VA_ARGS__)
ROOM_PATH_NODE * RoomGetNearestFreePathNodeDbg(
	struct GAME * game,
	struct LEVEL* level,
	const VECTOR & vPosition,
	struct ROOM ** pDestinationRoom,
	const char * file,
	int line,
	struct UNIT * pUnit = NULL,
	float fHeight = 0.0f,
	float fRadius = 0.0f,
	struct ROOM *pRoomSource = NULL,
	DWORD dwFreePathNodeFlags = 0);

#define RoomGetNearestFreePosition(roomsource, position, ...) RoomGetNearestFreePositionDbg(roomsource, position, __FILE__, __LINE__, __VA_ARGS__)
VECTOR RoomGetNearestFreePositionDbg(
	ROOM *pRoomSource,
	const VECTOR &vPosition,
	const char * file,
	int line,
	ROOM **pRoomDest = NULL);

#else

ROOM_PATH_NODE * RoomGetNearestFreePathNode(
	struct GAME * game,
	struct LEVEL* level,
	const VECTOR & vPosition,
	struct ROOM ** pDestinationRoom,
	struct UNIT * pUnit = NULL,
	float fHeight = 0.0f,
	float fRadius = 0.0f,
	struct ROOM *pRoomSource = NULL,
	DWORD dwFreePathNodeFlags = 0);

VECTOR RoomGetNearestFreePosition(
	ROOM *pRoomSource,
	const VECTOR &vPosition,
	ROOM **pRoomDest = NULL);

#endif
	
ROOM_PATH_NODE * RoomGetRoomPathNode(
	struct ROOM *pRoom,
	int nNodeIndex);

enum RANDOM_NODE_FLAGS
{
	RNF_MUST_ALLOW_SPAWN,		// path node must allow spawns
};

int s_RoomGetRandomUnoccupiedNode(
	struct GAME *pGame,
	struct ROOM *pRoom,
	DWORD dwRandomNodeFlags,		// see RANDOM_NODE_FLAGS
	float fMinRadius = 0.0f,
	float fMinHeight = 0.0f );


BOOL s_RoomGetRandomUnoccupiedPosition(
	struct GAME *pGame,
	struct ROOM *pRoom,
	VECTOR &vPosition);

int s_LevelGetRandomNodeAround(
	struct GAME *pGame,
	struct LEVEL *pLevel,
	const VECTOR & vPosition,
	float fRadius,
	ROOM ** pOutputRoom);

ROOM *  s_LevelGetRandomPositionAround(
	struct GAME *pGame,
	struct LEVEL *pLevel,
	const VECTOR & vCenter,
	float fRadius,
	VECTOR &vPosition);

enum NODE_RESULT
{
	NODE_FREE,
	NODE_BLOCKED,			// node is blocked by prop
	NODE_BLOCKED_UNIT,		// node is blocked by unit
};

enum BLOCKED_NODE_FLAGS
{
	BNF_NO_SPAWN_IS_BLOCKED,		// no spawn path nodes are treated as blocked
};

NODE_RESULT s_RoomNodeIsBlocked(
	ROOM *pRoom,
	int nNodeIndex,
	DWORD dwBlockedNodeFlags,		// see BLOCKED_NODE_FLAGS
	UNIT **pUnit);

NODE_RESULT s_RoomNodeIsBlockedIgnore(
	ROOM *pRoom,
	int nNodeIndex,
	DWORD dwBlockedNodeFlags,		// see BLOCKED_NODE_FLAGS
	UNIT *pUnitIgnore = NULL);

BOOL RoomPathNodeIsConnected(
	ROOM * pRoomSource,
	ROOM_PATH_NODE * pPathNodeSource,
	ROOM * pRoomDest,
	ROOM_PATH_NODE * pPathNodeDest);

VECTOR RoomPathNodeGetWorldPosition(
	struct GAME *pGame, 
	struct ROOM *pRoom, 
	ROOM_PATH_NODE *pRoomPathNode);

int RoomPathNodeGetHappyPlaceCount(
	struct GAME * pGame,
	struct ROOM * pRoom);

ROOM_PATH_NODE * RoomPathNodeGetHappyPlace(
	struct GAME * pGame,
	struct ROOM * pRoom,
	int nIndex);

ROOM_PATH_NODE * RoomPathNodeGetNearestEdgeNodeToPoint(
	struct GAME * pGame,
	struct ROOM * pRoom,
	const VECTOR & vWorldPosition);

ROOM_PATH_NODE * RoomPathNodeGetNearestNodeToPoint(
	struct GAME * pGame,
	struct ROOM * pRoom,
	const VECTOR & vWorldPosition);

//----------------------------------------------------------------------------
enum
{
	// Pre-search bits
	NPN_ONE_ROOM_ONLY,						// Don't search neighboring rooms
	NPN_USE_GIVEN_ROOM,						// Use the given room (that is, don't use RoomGetFromPosition())
	NPN_POSITION_IS_ALREADY_IN_ROOM_SPACE,	// Don't convert world-space coordinates to room-space, because they're already in room space
											//		(should probably just be used with ONE_ROOM_ONLY and USE_GIVEN_ROOM)

	// Search bits
	NPN_ALLOW_OCCUPIED_NODES,				// Allow path nodes that are occupied by other monsters
	NPN_ALLOW_BLOCKED_NODES,				// Allow path nodes that are blocked by props
	NPN_NO_SPAWN_NODES_ARE_BLOCKING,		// Consider no spawn nodes as blocked nodes
	NPN_DONT_CHECK_HEIGHT,					// Check the height of the path nodes against the given heights
	NPN_DONT_CHECK_RADIUS,					// Check the radius of the path nodes against the given radii
	NPN_IN_FRONT_ONLY,						// Check the angle from the destination position against the given face direction to be in front of it only
	NPN_BEHIND_ONLY,						// Check the angle from the destination position against the given face direction to be behind it only
	NPN_QUARTER_DIRECTIONALITY,				// Modifies IN_FRONT_ONLY and BEHIND_ONLY to require a smaller angle
	NPN_EDGE_NODES_ONLY,					// Only search nodes near room connections
	NPN_BIAS_FAR_NODES,						// Bias nodes away from the facing direction to be artifically further away
	NPN_USE_XY_DISTANCE,					// When checking ranges, only compare XY and not Z
	NPN_USE_EXPENSIVE_SEARCH,				// Do a more thorough and more accurate, but more computationally expensive search.  Only works on the client; ignored on the server.
	NPN_CHECK_DISTANCE,						// Do a real distance check per node in Mythos

	// Post-process bits
	NPN_CHECK_LOS,							// Do a raycast to check the line of sight between the output nodes and the position
	NPN_SORT_OUTPUT,						// Sort the output by distance
	NPN_EMPTY_OUTPUT_IS_OKAY,				// Don't assert if the output is empty
	NPN_CHECK_LOS_AGAINST_OBJECTS,			// When doing a LoS check, also check against objects in the way
};

struct NEAREST_NODE_SPEC
{
	DWORD dwFlags;
	UNIT * pUnit;
	VECTOR vFaceDirection;
	float fMinDistance;
	float fMaxDistance;
	float fMinHeight;
	float fMaxHeight;
	float fMinRadius;
	float fMaxRadius;

	NEAREST_NODE_SPEC() : 
		dwFlags(0),
		pUnit(NULL),
		vFaceDirection(VECTOR(0)),
		fMinDistance(-1.0f), fMaxDistance(-1.0f),
		fMinHeight(-1.0f), fMaxHeight(-1.0f),
		fMinRadius(-1.0f), fMaxRadius(-1.0f)
	{}
};

#if ISVERSION(DEVELOPMENT)
#define RoomGetNearestPathNodes(game, room, worldposition, outputlength, output, spec) RoomGetNearestPathNodesDbg(game, room, worldposition, outputlength, output, spec, __FILE__, __LINE__)
int RoomGetNearestPathNodesDbg(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	const NEAREST_NODE_SPEC * pSpec,
	const char * file,
	int line);

#define RoomGetNearestPathNode(game, room, worldposition, destinationroom, ...) RoomGetNearestPathNodeDbg(game, room, worldposition, destinationroom, __FILE__, __LINE__, __VA_ARGS__)
ROOM_PATH_NODE * RoomGetNearestPathNodeDbg(
	GAME * pGame,
	ROOM * pRoom,
	VECTOR vWorldPosition,
	ROOM ** pDestinationRoom,
	const char * file,
	int line,
	DWORD dwFlags = 0);
#else
int RoomGetNearestPathNodes(
	GAME * pGame,
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const int nOutputLength,
	FREE_PATH_NODE * pOutput,
	const NEAREST_NODE_SPEC * pSpec);

ROOM_PATH_NODE * RoomGetNearestPathNode(
	GAME * pGame,
	ROOM * pRoom,
	VECTOR vWorldPosition,
	ROOM ** pDestinationRoom,
	DWORD dwFlags = 0);
#endif

void RoomPathNodeDoZFixup(
	GAME * game, 
	ROOM * room, 
	PATH_NODE_INSTANCE * node);

ROOM_PATH_NODE * RoomGetNearestPathNodeRoomSpace(
	ROOM * pRoom,
	VECTOR vRoomSpacePosition,
	BOOL bAllowNull = TRUE);

//----------------------------------------------------------------------------
typedef BOOL (*PFN_PATHNODE_ITERATE)(ROOM * pRoomSource, ROOM_PATH_NODE * pPathNodeSource, ROOM * pRoomDest, ROOM_PATH_NODE * pPathNodeDest, float fDistance, void * pUserData);
void RoomPathNodeIterateConnections(
	ROOM * pRoom,
	ROOM_PATH_NODE * pRoomPathNode,
	PFN_PATHNODE_ITERATE pfnIterate,
	void * pUserData);

BOOL RoomPathNodeDoesSimplePathExistFromNodeToPoint(
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	const VECTOR & vPosition);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline ROOM_PATH_NODE_SET * RoomGetPathNodeSet(
	ROOM * room)
{
	if (!room)
	{
		return NULL;
	}
	ROOM_PATH_NODE_DEFINITION * path_node_definition = room->pPathDef;
	if (!path_node_definition)
	{
		return NULL;
	}

	int nPathLayoutSelected = room->nPathLayoutSelected;
	ASSERT_RETNULL((unsigned int)nPathLayoutSelected < (unsigned int)path_node_definition->nPathNodeSetCount);
	return &path_node_definition->pPathNodeSets[nPathLayoutSelected];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PATH_NODE_INSTANCE * RoomGetPathNodeInstance(
	ROOM * room,
	int index)
{
	ASSERT_RETNULL((unsigned int)index < room->nPathNodeCount);
	return &room->pPathNodeInstances[index];
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const VECTOR & RoomGetPathNodeInstanceWorldNormal(
	ROOM * room,
	int index,
	const VECTOR & vError)
{
#if HELLGATE_ONLY
	ASSERT_RETVAL((unsigned int)index < room->nPathNodeCount, vError);
	return room->pPathNodeInstances[index].vWorldNormal;
#else
	return cgvZAxis;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const VECTOR & RoomPathNodeGetWorldNormal(
	GAME * game, 
	ROOM * room, 
	ROOM_PATH_NODE * pathnode)
{
#if HELLGATE_ONLY
	static const VECTOR vError(0);
	REF(game);
	ASSERT_RETVAL(room, vError);
	return RoomGetPathNodeInstanceWorldNormal(room, pathnode->nIndex, vError);
#else
	return cgvZAxis;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const VECTOR & RoomPathNodeGetExactWorldPosition(
	GAME * game, 
	ROOM * room, 
	ROOM_PATH_NODE * pathnode)
{
	static const VECTOR vError(0);
	ASSERT_RETVAL(room, vError);
	ASSERT_RETVAL(pathnode, vError);


	PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(room, pathnode->nIndex);
	if (TESTBIT(node->dwNodeInstanceFlags, NIF_NEEDS_Z_FIXUP))
	{
		RoomPathNodeDoZFixup(game, room, node);
	}
	return node->vWorldPosition;

}

inline float GetPathNodeX( ROOM_PATH_NODE_DEFINITION * pDefinition, 
						  int nOffset )
{
	int y = (int)floor( (float)nOffset / (int)pDefinition->fNodeOffsetX );
	int x = nOffset - ( y * (int)pDefinition->fNodeOffsetX );

	return pDefinition->vCorner.fX + (float)x * -pDefinition->fNodeFrequencyX;
}

inline float GetPathNodeY( ROOM_PATH_NODE_DEFINITION * pDefinition, 
						  int nOffset )
{
	int y = (int)floor( (float)nOffset / (int)pDefinition->fNodeOffsetX );

	return pDefinition->vCorner.fY + (float)y * pDefinition->fNodeFrequencyY;	
}

#endif
