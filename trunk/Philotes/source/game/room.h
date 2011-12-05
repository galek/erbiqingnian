//----------------------------------------------------------------------------
// room.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ROOM_H_
#define _ROOM_H_

#pragma warning(disable:4127)

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _DEFINITION_HEADER_INCLUDED_
#include "definition.h"
#endif

#ifndef _GAME_H_
#include "game.h"
#endif

#ifndef _TARGET_H_
#include "target.h"
#endif

#ifndef _COLLISION_H_
#include "collision.h"
#endif

#include "c_attachment.h"

#ifndef _CONVEXHULL_H_
#include "convexhull.h"
#endif

#ifndef _QUADTREE_H
#include "quadtree.h"
#endif

#ifndef _APPCOMMONTIMER_H_
#include "appcommontimer.h"
#endif

#include "e_portal.h"
#include "difficulty.h"

//----------------------------------------------------------------------------
// DEBUG
//----------------------------------------------------------------------------
#define DEBUG_TRACE_ACTIVE				(1 && ISVERSION(DEVELOPMENT))

#if DEBUG_TRACE_ACTIVE
#define TRACE_ACTIVE(...)				trace(__VA_ARGS__)
#else
#define TRACE_ACTIVE(...)
#endif


//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
enum TARGET_TYPE;
struct SPAWN_ENTRY;
struct DISTRICT;
struct GAME_EVENT;
struct DRLG_DEFINITION;
struct SUBLEVEL;


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define ROOM_FILE_EXTENSION					"rom"
#define ROOM_FILE_VERSION					(72 + GLOBAL_FILE_VERSION)
#define ROOM_FILE_MAGIC_NUMBER				0xea7a7abe

//#define FORCE_UPDATE_ROOM_FILES

#define MAX_VISUAL_PORTALS_PER_ROOM			32
#define MAX_CONNECTIONS_PER_ROOM			32		// visibility flags are saved in a dword, so max is 32 connections
#define MAX_OCCLUDER_PLANES_PER_ROOM		64		// arbitrary limit


#define MAX_DYNAMIC_LIGHTS_PER_ROOM			40
#define MAX_MODELS_PER_ROOM					40
#define MAX_STATIC_MODELS_PER_ROOM			15

#define ROOM_STATIC_MODEL_START_COUNT		2
#define ROOM_DYNAMIC_MODEL_START_COUNT		8

// if you change these structures, then update the room file version number
#define ROOM_CONNECTION_CORNER_COUNT		4

#define POINTS_PER_MONSTER_SPAWN_AREA		4
#define OUTDOOR_ROOM_BOUNDING_BOX_Z_BONUS	20
#define ROOM_INDEX_SIZE						64

// a room is considered "nearby" if below this distance from any point/plane in my hull to another room's point/plane
#define TUGBOAT_NEARBY_ROOM_DISTANCE		0.5f
#define ADJACENT_ROOM_DISTANCE				0.5f
#define NEARBY_ROOM_DISTANCE				25.0f
#define VISIBLE_ROOM_DISTANCE				30.0f

#define MAX_ROOM_NAME						(128)	// arbitrary

// spawn classes for rooms
#define MAX_SPAWN_CLASS_PER_ROOM			5

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define TRACE_ROOM(nRoomID, pszEvent)				c_TraceRoom( nRoomID, pszEvent, __LINE__ )
#define TRACE_ROOM_INT(nRoomID, pszEvent, nInt)		c_TraceRoomInt( nRoomID, pszEvent, nInt, __LINE__ )


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum ROOM_FLAG
{
	ROOM_POPULATED_BIT,				// room has been populated with monsters/objects
	ROOM_CLIENT_POPULATED_BIT,		// client has populated room with client side things
	ROOM_ACTIVE_BIT,				// room is "active" (its contents exist and are running ais and such)
	ROOM_CANNOT_RESET_BIT,			// room cannot be reset
	ROOM_NO_SUBLEVEL_ENTRANCE_BIT,	// this room cannot be selected for a sublevel entrance portal
	ROOM_POPULATED_ONCE_BIT,		// room has been populated at least once
	ROOM_NO_SPAWN_BIT,				// just like the "no spawn" flag in a room definition, but dynamic
	ROOM_HAS_NO_SPAWN_NODES_BIT,	// room has at least one "no spawn" path node
	ROOM_UNITS_BEING_ITERATED_BIT,	// units in the room are currently being iterated by the helper functions
	ROOM_NO_ADVENTURES_BIT,			// room cannot have adventures
	ROOM_KNOWN_BIT,					// client only ... this room is officially known to us from the server, and therefore all the units etc
	ROOM_KNOWN_PREVIOUS,			// server only, room was known previously
	ROOM_KNOWN_CURRENT,				// server only, room is known now
	ROOM_EDGENODE_CONNECTIONS,		// have we created edgenode connections for this room
	ROOM_IGNORE_ALL_THEMES,			// some levels need to create all content on the level and then the client will hide what it needs to
	NUM_ROOM_FLAGS,
};

//----------------------------------------------------------------------------
// visible room list commands
enum
{
	VRCOM_ADD_ROOM = 0,
	VRCOM_BEGIN_SECTION,
	VRCOM_END_SECTION
};

//----------------------------------------------------------------------------
enum ROOM_POPULATE_PASS
{
	RPP_INVALID = -1,
	
	RPP_SUBLEVEL_POPULATE,		// sublevel populated (units with level wide effects, like warps and no spawners)
	RPP_SETUP,					// room setup (like blockers, cause they can effect where you can put the next pass of monster content)
	RPP_CONTENT,				// this is where content (monsters, chests) are spawned
	
	RPP_NUM_PASSES					// keep last please
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct SPAWN_LOCATION
{
	VECTOR vSpawnPosition;
	VECTOR vSpawnDirection;
	VECTOR vSpawnUp;
	float fRadius;
};

struct ROOM_CONNECTION 
{
	DWORD			dwVisibilityFlags;
	VECTOR			pvBorderCorners[ ROOM_CONNECTION_CORNER_COUNT ];
	BOUNDING_BOX	tConnectionBounds;
	VECTOR			vBorderNormal;
	VECTOR			vBorderCenter;
	float			fPortalRadiusSqr;
	VECTOR			vPortalCentroid;
	SAFE_PTR(ROOM*, pRoom);
	BYTE			bOtherConnectionIndex;
	int				nCullingPortalID;
};

//----------------------------------------------------------------------------
struct ROOM_INDEX
{
	char			pszFile[ ROOM_INDEX_SIZE ];
	BOOL			bOutdoor;
	BOOL			bOutdoorVisibility;
	BOOL			bComputeObscurance;
	BOOL			bNoMonsterSpawn;
	BOOL			bNoCollision;
	BOOL			bNoAdventures;
	BOOL			bNoSubLevelEntrance;
	BOOL			bOccupiesNodes;
	BOOL			bRaisesNodes;
	BOOL			bFullCollision;
	BOOL			bUseMatID;
	BOOL			bDontObstructSound;
	BOOL			bDontOccludeVisibility;
	BOOL			b3rdPersonCameraIgnore;
	BOOL			bRTSCameraIgnore;
	BOOL			bCanopyProp;
	BOOL			bClutter;
	int				nMessageString;
	int				nHavokSliceType;
	int				nExcelVersion;
	int				nSpawnClass[MAX_SPAWN_CLASS_PER_ROOM];
	int				nSpawnClassRunXTimes[MAX_SPAWN_CLASS_PER_ROOM];
	float			fNodeBuffer;
	SAFE_PTR(struct ROOM_DEFINITION*,	pRoomDefinition);
	char			pszReverbFile[ MAX_XML_STRING_LENGTH ];
	char			pszOverrideNodes[ MAX_XML_STRING_LENGTH ];
	PCODE			codeMonsterLevel[ NUM_DIFFICULTIES ];
	SAFE_PTR(struct SOUND_REVERB_DEFINITION*, pReverbDefinition);
	int				nBackgroundSound;
	int				nNoGoreProp;
	int				nNoHumansProp;

	int				nId;
	SAFE_PTR(ROOM_INDEX*, pNext);
};

//----------------------------------------------------------------------------
#define PATHING_MESH_SIZE	2.0f

enum
{
	COLLISION_WALL,
	COLLISION_FLOOR,
	COLLISION_CEILING,
	NUM_COLLISION_CATEGORIES
};


//----------------------------------------------------------------------------
struct COLLISION_GRID_NODE
{
	unsigned int * POINTER_64	pnPolyList[NUM_COLLISION_CATEGORIES];
	unsigned int				nCount[NUM_COLLISION_CATEGORIES];
};

//----------------------------------------------------------------------------
struct OCCLUDER_PLANE
{
	// same general idea as clip plane
	VECTOR	pvCorners[ ROOM_CONNECTION_CORNER_COUNT ];
	float	fRadiusSqr;
	VECTOR	vCentroid;
};


//----------------------------------------------------------------------------
struct VISIBLE_ROOM_COMMAND
{
	int				nCommand;
	int				nPass;

	DWORD_PTR		dwData;				// room ptr, portal ID
	BOUNDING_BOX	ClipBox;
};

//----------------------------------------------------------------------------
struct ROOM_COLLISION_MATERIAL_DATA
{
	char			pszName[ DEFAULT_INDEX_SIZE ];
	int				nMaterialNumber;
	BOOL			bFloor;
	int				nMapsTo;
	float			fDirectOcclusion;
	float			fReverbOcclusion;
	int				nDebugColor;
	char			pszMax9Name[ DEFAULT_INDEX_SIZE ];
};

//----------------------------------------------------------------------------
struct ROOM_COLLISION_MATERIAL_INDEX
{
	int				nMaterialIndex;
	int				nFirstTriangle;
	int				nTriangleCount;
};

//------------------------------------------------------------------------
// This allows for rooms when spawning monsters to give monsters a secondary
//treasure to spawn. For instance, a key to a locked door.
//------------------------------------------------------------------------
struct ROOM_SPAWNS_TREASURE
{
	TARGET_TYPE		nTargetType;
	int				nTreasure;
	int				nObjectClassOverRide;
};

//----------------------------------------------------------------------------
struct ROOM_DEFINITION
{
	DEFINITION_HEADER tHeader;
	int				nRoomExcelIndex; // filled on load

	// collision data
	int				nRoomPointCount;
	SAFE_PTR(ROOM_POINT*, pRoomPoints);
	int				nRoomCPolyCount;
	SAFE_PTR(ROOM_CPOLY*, pRoomCPolys);
	float			fRoomCRadius;

	int				nConnectionCount;
	SAFE_PTR(ROOM_CONNECTION*, pConnections);
	int				nOccluderPlaneCount;
	SAFE_PTR(OCCLUDER_PLANE*, pOccluderPlanes);
	BOUNDING_BOX	tBoundingBox;
	int					nCollisionMaterialIndexCount;
	SAFE_PTR(ROOM_COLLISION_MATERIAL_INDEX*, pCollisionMaterialIndexes);
	
	int nVertexBufferSize;
	int nVertexCount;
	SAFE_PTR(VECTOR*, pVertices);
	int nIndexBufferSize;
	int nTriangleCount;
	SAFE_PTR(WORD*, pwIndexBuffer);
	HAVOK_SHAPE_HOLDER tHavokShapeHolder;
	//SAFE_PTR(class hkMoppBvTreeShape*, pHavokShape);
	//SAFE_PTR(class hkMoppBvTreeShape**, ppHavokMultiShape);
	int nHavokShapeCount;
	VECTOR vHavokShapeTranslation[16];
	int nExcelVersion;

	SAFE_PTR(VECTOR*, pHullVertices);
	int nHullVertexCount;
	SAFE_PTR(int*, pHullTriangles);
	int nHullTriangleCount;

	// collision info
	SAFE_PTR(COLLISION_GRID_NODE*, pCollisionGrid);
	float					fCollisionGridX;
	float					fCollisionGridY;
	int						nCollisionGridWidth;
	int						nCollisionGridHeight;
	SAFE_PTR(unsigned int*,	pnCollisionGridIndexes);
	unsigned int			nCollisionGridIndexes;

	// server
	VECTOR			vStart;
	int				nStartDirection;

	//client
	float			fInitDistanceToCamera;
	int				nModelDefinitionId;

	SAFE_PTR(BYTE*, pbFileStart);
	int				nFileSize;
};


//----------------------------------------------------------------------------
#define AUTOMAP_VISITED			MAKE_MASK(0)
#define AUTOMAP_WARPIN			MAKE_MASK(1)
#define AUTOMAP_WARPOUT			MAKE_MASK(2)
#define AUTOMAP_WALL			MAKE_MASK(2)

//----------------------------------------------------------------------------
struct AUTOMAP_NODE
{
	AUTOMAP_NODE *	m_pNext;
	VECTOR			v1;
	VECTOR			v2;
	VECTOR			v3;
	DWORD			m_dwFlags;
	int				time;
};

//----------------------------------------------------------------------------
struct PROP_NODE
{
	ROOM_DEFINITION *	m_pDefinition;
	ROOM_POINT *		pCPoints;
	ROOM_CPOLY *		pCPolys;
	PROP_NODE *			m_pNext;
	MATRIX				m_matrixWorldToProp;
	MATRIX				m_matrixPropToRoom;
};

//----------------------------------------------------------------------------
struct PATH_NODE_EDGE_CONNECTION
{
	struct ROOM *						pRoom;
	float								fDistance;
	int									nEdgeIndex;
};

//----------------------------------------------------------------------------
#define MAX_INSTANCE_CONNECTION_COUNT 16

struct EDGE_NODE_INSTANCE
{
	WORD								nEdgeConnectionStart;	// Index into ROOM::pEdgeConnections
	WORD								nEdgeConnectionCount;
};

//----------------------------------------------------------------------------
struct UNIT_NODE_MAP
{
	UNIT_NODE_MAP *		pNext;
	UNIT *				pUnit;
	int					nId;
	BOOL				bBlocked;
};

//----------------------------------------------------------------------------
struct FIELD_MISSILE
{
	int nMissileClass;
	GAME_TICK tiFuseStartTime;
	GAME_TICK tiFuseEndTime;
};

struct FIELD_MISSILE_MAP
{
	FIELD_MISSILE_MAP *	pNext;
	UNIT *				pUnit;
	FIELD_MISSILE *		pFieldMissiles;
	int					nId;
	int					nFieldMissileCount;
};

//----------------------------------------------------------------------------
#define VP_FLAG_HAS_PARTICLE		MAKE_MASK(0)

struct ROOM_VISUAL_PORTAL
{
	DWORD	dwFlags;
	int		nEnginePortalID;					// engine portal id, for culling traversal
	UNITID	idVisualPortal;
};

//----------------------------------------------------------------------------
enum VISUAL_PORTAL_DIRECTION
{
	VPD_INVALID = -1,
	
	VPD_FROM,
	VPD_TO,
};

//----------------------------------------------------------------------------
enum NODE_INSTANCE_FLAGS
{
	NIF_MARKED,			// mark tag (used during traversal iterations)
	NIF_NO_SPAWN,		// node is no spawn node
	NIF_NEEDS_Z_FIXUP,	// node needs prop Z fixup
	NIF_DID_Z_FIXUP,	// did z fixup
	NIF_BLOCKED,		// blocked by pathnode mask
};

//----------------------------------------------------------------------------
struct PATH_NODE_INSTANCE
{
	VECTOR								vWorldPosition;
#if HELLGATE_ONLY
	VECTOR								vWorldNormal;
#endif
	DWORD								dwNodeInstanceFlags;
};

//----------------------------------------------------------------------------
enum ROOM_MONSTER_COUNT_TYPE
{
	RMCT_INVALID = -1,
	
	RMCT_ABSOLUTE,					// 1 monster = 1 count
	RMCT_DENSITY_VALUE_OVERRIDE,	// 1 monster = pUnitData->nDensityValueOverride count
	
	RMCT_NUM_TYPES					// keep this last
};

//----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable:4324)			// warning C4324: 'ROOM' : structure was padded due to __declspec(align())
struct ROOM
{
#ifdef _DEBUG	
	char								szName[MAX_ROOM_NAME];
#endif
	char 								szLayoutOverride[DEFAULT_FILE_WITH_PATH_SIZE];

	GAME *								m_pGame;
	ROOM_DEFINITION *					pDefinition;
	ROOM *								m_pNextRoomInHash;
	ROOM *								m_pPrevRoomInHash;

	ROOM *								pNextRoomInLevel;
	ROOM *								pPrevRoomInLevel;
	
	ROOM **								ppConnectedRooms;						// unique list of rooms with actual connections
	ROOM **								ppVisibleRooms;							// unique list of rooms deemed "visible" via raycast
	ROOM **								ppAdjacentRooms;						// unique list of rooms within .5 meters
	ROOM **								ppNearbyRooms;							// unique list of rooms within 10 meters

	DISTRICT *							pDistrict;								// "neighborhood" in level
	ROOM *								pNextRoomInDistrict;
	struct LEVEL *						pLevel;

	ROOM_CONNECTION *					pConnections;
	CONVEXHULL *						pHull;

	PROP_NODE *							pProps;
	WORD *								pwCollisionIter;
	struct ROOM_PATH_NODE_DEFINITION *	pPathDef;
	struct ROOM_PATH_MASK_DEFINITION*	pMaskDef;

	AUTOMAP_NODE *						pAutomap;
	struct ROOM_LAYOUT_GROUP_DEFINITION * pLayout;
	struct ROOM_LAYOUT_SELECTIONS *		pLayoutSelections;

	BUCKET_ARRAY<PATH_NODE_EDGE_CONNECTION, 75>		pEdgeNodeConnections;
	BUCKET_ARRAY<PATH_NODE_INSTANCE, 45>			pPathNodeInstances;
	BUCKET_ARRAY<EDGE_NODE_INSTANCE, 64> 			pEdgeInstances;
	CHash<UNIT_NODE_MAP>				tUnitNodeMap;
	CHash<FIELD_MISSILE_MAP>			tFieldMissileMap;
	SIMPLE_DYNAMIC_ARRAY<class hkRigidBody *>		pRoomModelRigidBodies;				// client only
	SIMPLE_DYNAMIC_ARRAY<class hkRigidBody *>		pRoomModelInvisibleRigidBodies;		// client only
	SIMPLE_DYNAMIC_ARRAY<int>			pnModels;
	SIMPLE_DYNAMIC_ARRAY<int>			pnRoomModels;

	TARGET_LIST							tUnitList;
	ROOM_VISUAL_PORTAL					pVisualPortals[MAX_VISUAL_PORTALS_PER_ROOM];	// visual portals in this room
	ROOM_SPAWNS_TREASURE				nSpawnTreasureFromUnit;

	MATRIX								tWorldMatrix;
	MATRIX								tWorldMatrixInverse;
	BOUNDING_BOX						tBoundingBoxWorldSpace;
	BOUNDING_SPHERE						tBoundingSphereWorldSpace;
	VECTOR								vBoundingBoxCenterWorldSpace;
	VECTOR								vAmbientColor;							// client only
	VECTOR								vDirectionalLight;						// client only
	VECTOR								vDirectionalColor;						// client only
	VECTOR								vPossibleSubLevelEntrance;

	RAND								tLayoutRand;
	ROOMID								idRoom;
	SUBLEVELID							idSubLevel;
	GAME_TICK							tResetTick;

	DWORD								pdwFlags[DWORD_FLAG_SIZE(NUM_ROOM_FLAGS)];
	DWORD								dwIterator;
	DWORD								dwRoomSeed;
	DWORD								dwResetDelay;
	DWORD								dwRoomListTimestamp;
	BOOL								bPopulated[RPP_NUM_PASSES];
	int									nMonsterCountOnPopulate[RMCT_NUM_TYPES];
	int									nIndexInLevel;
	int									nNumVisualPortals;						// count of idVisualPortal
	int									nConnectionCount;						// number of actual connections
	int									nPathLayoutSelected;
	int									nEdgeNodeConnectionCount;
	unsigned int						nVisibleRoomCount;
	unsigned int						nAdjacentRoomCount;
	unsigned int						nNearbyRoomCount;
	unsigned int						nConnectedRoomCount;
	unsigned int						nPathNodeCount;
	unsigned int						nKnownByClientRefCount;					// number of clients that know about this room

	WORD								wCollisionIter;
};
#pragma warning(pop)


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void RoomFree(
	GAME * game,
	ROOM * room,
	BOOL bFreeEntireLevel);

void RoomFreeCommon(
	GAME * pGame,
	ROOM * pRoom,
	LEVEL * level,
	BOOL bFreeEntireLevel);

void RoomInit(
	GAME* game,
	ROOM* room,
	int nRoomIndex);

void RoomSetDistrict(
	ROOM* pRoom,
	DISTRICT* pDistrict);

DISTRICT* RoomGetDistrict(
	ROOM* pRoom);

void RoomSetRegion(
	GAME* pGame,
	ROOM* pRoom,
	int nRegion);

void RoomSetSubLevel(
	GAME *pGame,
	ROOM *pRoom,
	SUBLEVELID idSubLevel);

SUBLEVELID RoomGetSubLevelID(
	const ROOM *pRoom);

SUBLEVEL *RoomGetSubLevel(
	const ROOM *pRoom);

int RoomGetDRLGDefIndex(
	const ROOM *pRoom);

const DRLG_DEFINITION *RoomGetDRLGDef(
	const ROOM *pRoom);
	
int RoomGetSubLevelEngineRegion(
	ROOM *pRoom);
		
float RoomGetArea( 
	ROOM* pRoom);

const ROOM_INDEX *RoomGetRoomIndex(
	GAME *pGame,
	const ROOM *pRoom);

ROOM* RoomAdd(
	GAME* game,
	struct LEVEL* level,
	int nRoomIndex,
	MATRIX* pMatrix,
	BOOL bRunPostProcess,
	DWORD dwRoomSeed,
	ROOMID idRoom,
	SUBLEVELID idSubLevel,	
	const char * pszLayoutOverride = NULL);

void RoomAddPostProcess(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	BOOL bLevelCreate);

void RoomDoSetup(
	GAME * game,
	LEVEL * level,
	ROOM * room);

void RoomCreatePathEdgeNodeConnections(
	GAME * game,
	ROOM * pRoom,
	BOOL bTwoWay = TRUE);

void RoomRemoveEdgeNodeConnections(
	GAME * game,
	ROOM * room);

void RoomFreeEdgeNodeConnections(
	GAME * game,
	ROOM * room);

void c_RoomDrawUpdate(
	GAME * game,
	UNIT * unit,
	float fTimeDelta );

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
BOOL c_RoomToggleDrawAllRooms();
BOOL c_RoomToggleDrawOnlyConnected();
BOOL c_RoomToggleDrawOnlyNearby();
#endif

int c_RoomGetDrawCount();

void c_RoomAddModel		   ( ROOM * pRoom, int nModelId, MODEL_SORT_TYPE SortType );
void c_RoomRemoveModel	   ( ROOM * pRoom, int nModelId );
#ifdef HAVOK_ENABLED
void RoomAddRigidBody	   ( ROOM * pRoom, class hkRigidBody * pRigidBody );
void c_RoomAddInvisibleRigidBody ( ROOM * pRoom, class hkRigidBody * pRigidBody );
#endif
void c_RoomLightMoved      ( ROOM * pRoom );
BOOL c_RoomIsVisible	   ( ROOM * pRoom );

//#ifdef HAMMER
void RoomRerollDRLG( GAME *pGame, const char * pszFileName, int nSeedDelta  );
//#endif

ROOM* RoomGetFromPosition(
	GAME* game,
	ROOM* room,
	const VECTOR* loc,
	float fEpsilon = HULL_EPSILON );

ROOM* RoomGetFromPosition(
	GAME* game,
	LEVEL* level,
	const VECTOR * loc);

struct ROOM_DEFINITION* RoomDefinitionGet(
	struct GAME* game,
	int nRoomIndex,
	BOOL fLoadModel,
	float fDistanceToCamera = 0.0f);

ROOM_PATH_MASK_DEFINITION* RoomPathMaskDefinitionGet(
		const char* pszFileNameWithPath);

struct ROOM_DEFINITION* PropDefinitionGet(
	struct GAME* game,
	const char * pszPropFile,
	BOOL bLoadModel,
	float fInitDistanceToCamera);

BOOL RoomIsPlayerInConnectedRoom(
	GAME* game, 
	ROOM * pRoom,
	const VECTOR & vTargetingLocation,
	float fRange);

BOOL RoomIsHellrift(
	ROOM *pRoom);

int s_RoomGetPlayerCount(
	GAME *pGame,
	const ROOM *pRoom);

#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_RoomReset(
	GAME *pGame,
	ROOM *pRoom);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

void s_RoomStartResetTimer(
	ROOM *pRoom,
	float flDelayMultiplier,
	DWORD dwDelayOverride = 0);

void s_RoomMakePlayersSafe(
	ROOM *pRoom);
		
void s_RoomSendToClient(
	ROOM *pRoom,
	struct GAMECLIENT *pClient,
	BOOL bMarkAsKnown);

void s_RoomRemoveFromClient( 
	ROOM *pRoom, 
	GAMECLIENT *pClient);

typedef void (* PFN_MONSTER_SPAWNED_CALLBACK)( UNIT *pSpawned, void *pCallbackData );

int s_RoomSpawnMonsterByMonsterClass( 
	GAME * pGame, 
	SPAWN_ENTRY *pSpawn,
	int nMonsterQuality,
	ROOM * pRoom, 
	SPAWN_LOCATION *pSpawnLocation,
	PFN_MONSTER_SPAWNED_CALLBACK pfnSpawnCallback,
	void *pCallbackData);

int s_RoomSpawnMinions( 
	GAME * pGame, 
	UNIT * pMonster,
	ROOM * pRoom, 
	VECTOR *vPosition );

void s_RoomPopulateSpawnPoint(
	GAME *pGame,
	ROOM *pRoom,
	struct ROOM_LAYOUT_GROUP *pSpawnPoint,
	struct ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	ROOM_POPULATE_PASS ePass);

//----------------------------------------------------------------------------
enum ROOM_ITERATE_UNITS
{	
	RIU_CONTINUE,
	RIU_STOP
};

typedef ROOM_ITERATE_UNITS (* PFN_ROOM_ITERATE_UNIT_CALLBACK)( UNIT *pUnit, void *pCallbackData );
typedef BOOL (* PFN_UNIT_FILTER)( UNIT *pUnit );

ROOM_ITERATE_UNITS RoomIterateUnits(
	ROOM *room,
	const TARGET_TYPE *eTargetTypes,
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData);

UNIT *RoomFindFirstUnitOf(
	ROOM *pRoom,
	TARGET_TYPE *pTargetTypes,
	GENUS eGenus,
	int nClass,
	PFN_UNIT_FILTER pfnFilter = NULL);

int RoomGetMonsterCount(
	ROOM *pRoom,
	ROOM_MONSTER_COUNT_TYPE eType = RMCT_ABSOLUTE,
	const TARGET_TYPE *peTargetTypes = NULL);

int c_RoomGetCritterCount(
	ROOM *pRoom);

int s_RoomGetEstimatedMonsterCount(
	ROOM * pRoom);

int RoomCountUnitsOfType(
	ROOM *pRoom,
	int nUnitType);
	
void SpawnLocationInit(
	SPAWN_LOCATION *pSpawnLocation,
	ROOM *pRoom,
	const struct ROOM_LAYOUT_GROUP *pLayoutGroup,
	const struct ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation);

void SpawnLocationInit(
	SPAWN_LOCATION *pSpawnLocation,
	ROOM *pRoom,
	int nNodeIndex,
	float * pfOrientationRadians = NULL);

void s_RoomSubLevelPopulated(
	ROOM *pRoom,
	UNIT *pActivator);
	
void s_RoomActivate(
	ROOM *pRoom,
	UNIT *pActivator);
	
void s_RoomDeactivate(
	ROOM * room,
	BOOL bDeactivateLevel);

ROOM *RoomGetByID(
	GAME *pGame,
	ROOMID idRoom);

void s_RoomUpdate( 
	ROOM *pRoom);

void RoomUpdateClient( 
	UNIT *pPlayer);

const ROOM_COLLISION_MATERIAL_INDEX * GetMaterialFromIndex(
	ROOM_DEFINITION * pRoomDef,
	int nIndex);

const ROOM_COLLISION_MATERIAL_DATA * FindMaterialWithIndex(
	int nMaterialIndex);

const ROOM_COLLISION_MATERIAL_DATA * GetMaterialByIndex(
	int nIndex);

int GetIndexOfMaterial(
	const ROOM_COLLISION_MATERIAL_DATA * pCollisionData );

int FindMaterialIndexWithMax9Name(
	const char * pszMax9Name);

int RoomGetMonsterExperienceLevel(
	ROOM* room);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline struct LEVEL * RoomGetLevel(
	const ROOM * room)
{
	ASSERT_RETNULL(room);
	ASSERT(room->pLevel);
	return room->pLevel;
}

//----------------------------------------------------------------------------
inline int RoomGetNearbyRoomCount(
	ROOM * room)
{
	ASSERT_RETZERO(room);
	return room->nNearbyRoomCount;
}


//----------------------------------------------------------------------------
inline ROOM * RoomGetNearbyRoom(
	ROOM * room,
	unsigned int index)
{
	ASSERT_RETNULL(room);
	ASSERT_RETNULL(index < (unsigned int)room->nNearbyRoomCount);
	return room->ppNearbyRooms[index];
}


//----------------------------------------------------------------------------
inline int RoomGetAdjacentRoomCount(
	ROOM * room)
{
	ASSERT_RETZERO(room);
	return room->nAdjacentRoomCount;
}


//----------------------------------------------------------------------------
inline ROOM * RoomGetAdjacentRoom(
	ROOM * room,
	unsigned int index)
{
	ASSERT_RETNULL(room);
	ASSERT_RETNULL(index < (unsigned int)room->nAdjacentRoomCount);
	return room->ppAdjacentRooms[index];
}


//----------------------------------------------------------------------------
inline int RoomGetVisibleRoomCount(
	ROOM * room)
{
	ASSERT_RETZERO(room);
	return room->nVisibleRoomCount;
}	


//----------------------------------------------------------------------------
inline ROOM * RoomGetVisibleRoom(
	ROOM * room,
	unsigned int index)
{
	ASSERT_RETNULL(room);
	ASSERT_RETNULL(index < (unsigned int)room->nVisibleRoomCount);
	return room->ppVisibleRooms[index];
}

//----------------------------------------------------------------------------
inline int RoomGetConnectedRoomCount(
	ROOM * room)
{
	ASSERT_RETZERO(room);
	return room->nConnectedRoomCount;
}	

//----------------------------------------------------------------------------
inline ROOM * RoomGetConnectedRoom(
	ROOM * room,
	unsigned int index)
{
	ASSERT_RETNULL(room);
	ASSERT_RETNULL(index < (unsigned int)room->nConnectedRoomCount);
	ASSERT_RETNULL(room->ppConnectedRooms);
	return room->ppConnectedRooms[index];
}

//----------------------------------------------------------------------------
enum NEIGHBORING_ROOM
{
	NR_CONNECTED,						// use room "connected" data structures - this is actually connected rooms via ROOM_CONNECTION (albeit unique)
	NR_ADJACENT,						// use room "nearby room" data structures - this is rooms whose manhattan distance is within .5 meters
	NR_NEARBY,							// use room "nearby room" data structures - this is rooms whose manhattan distance is within 10 meters
	NR_VISIBLE,							// use room "visible room" data structures
	NR_FOR_KNOWN,						// use algorithm for "known" rooms (rooms that should be sent to clients)
	
	NR_NUM_TYPES
};


//----------------------------------------------------------------------------
struct ROOM_LIST
{
	ROOM *								pRooms[MAX_ROOMS_PER_LEVEL];
	int									nNumRooms;
	
	ROOM_LIST(void) 
		: nNumRooms(0) 
	{ }
};


//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetConnectedRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
inline void RoomGetConnectedRoomList(
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	if (bIncludeCenter)
	{
		TemplatedRoomGetConnectedRoomList<TRUE>(room, roomlist);
	}
	else
	{
		TemplatedRoomGetConnectedRoomList<FALSE>(room, roomlist);
	}
}
	

//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetAdjacentRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
inline void RoomGetAdjacentRoomList(
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	if (bIncludeCenter)
	{
		TemplatedRoomGetAdjacentRoomList<TRUE>(room, roomlist);
	}
	else
	{
		TemplatedRoomGetAdjacentRoomList<FALSE>(room, roomlist);
	}
}


//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetNearbyRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
inline void RoomGetNearbyRoomList(
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	if (bIncludeCenter)
	{
		TemplatedRoomGetNearbyRoomList<TRUE>(room, roomlist);
	}
	else
	{
		TemplatedRoomGetNearbyRoomList<FALSE>(room, roomlist);
	}
}


//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetVisibleRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
inline void RoomGetVisibleRoomList(
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	if (bIncludeCenter)
	{
		TemplatedRoomGetVisibleRoomList<TRUE>(room, roomlist);
	}
	else
	{
		TemplatedRoomGetVisibleRoomList<FALSE>(room, roomlist);
	}
}


//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetKnownRoomList( 
	UNIT * player,							
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
inline void RoomGetKnownRoomList(
	UNIT * player,							
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	if (bIncludeCenter)
	{
		TemplatedRoomGetKnownRoomList<TRUE>(player, room, roomlist);
	}
	else
	{
		TemplatedRoomGetKnownRoomList<FALSE>(player, room, roomlist);
	}
}


//----------------------------------------------------------------------------
inline void RoomGetNeighboringRoomList(
	NEIGHBORING_ROOM eSearchType,
	UNIT * player,							
	ROOM * room, 
	ROOM_LIST * roomlist,
	BOOL bIncludeCenter = FALSE)
{
	switch (eSearchType)
	{
	case NR_CONNECTED:
		RoomGetConnectedRoomList(room, roomlist, bIncludeCenter);
		break;
	case NR_ADJACENT:
		RoomGetAdjacentRoomList(room, roomlist, bIncludeCenter);
		break;
	case NR_NEARBY:
		RoomGetNearbyRoomList(room, roomlist, bIncludeCenter);
		break;
	case NR_VISIBLE:
		RoomGetVisibleRoomList(room, roomlist, bIncludeCenter);
		break;
	case NR_FOR_KNOWN:
		RoomGetKnownRoomList(player, room, roomlist, bIncludeCenter);
		break;
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
inline int RoomGetNeighboringRoomCount(
	NEIGHBORING_ROOM eType,
	ROOM * room)
{
	switch (eType)
	{
		case NR_CONNECTED:	return RoomGetConnectedRoomCount(room);
		case NR_ADJACENT:	return RoomGetAdjacentRoomCount(room);
		case NR_NEARBY:		return RoomGetNearbyRoomCount(room);
		case NR_VISIBLE:	return RoomGetVisibleRoomCount(room);
		case NR_FOR_KNOWN:	ASSERTX_RETZERO(0, "Getting known neighboring rooms count is not supported");
		default:			ASSERTX_RETZERO(0, "Unknown neighboring room type");
	}
}	


//----------------------------------------------------------------------------
inline ROOM * RoomGetNeighboringRoom(
	NEIGHBORING_ROOM eType,
	ROOM * room,
	unsigned int index)
{
	switch (eType)
	{
		case NR_CONNECTED:	return RoomGetConnectedRoom(room, index);
		case NR_ADJACENT:	return RoomGetAdjacentRoom(room, index );
		case NR_NEARBY:		return RoomGetNearbyRoom(room, index );
		case NR_VISIBLE:	return RoomGetVisibleRoom(room, index );
		case NR_FOR_KNOWN:	ASSERTX_RETZERO(0, "Unknown known neighboring room by index is not supported");
		default:			ASSERTX_RETZERO(0, "Unknown neighboring room type");
	}
}	

ROOM * RoomGetFirstConnectedRoomOrSelf(
	ROOM * room);

ROOM * RoomGetRandomNearbyRoom(
	GAME * game,
	ROOM * room);

BOOL RoomOutsideOfDeactivationRange(
	ROOM * room,
	UNIT * player);

const char *RoomGetDevName(
	ROOM *pRoom);


//----------------------------------------------------------------------------
// SERVER FUNCTIONS
//----------------------------------------------------------------------------
void s_DrawRoom(
	GAME* game,
	UNIT* unit,
	int nWorldX,
	int nWorldY);

//spawns an object in a room
UNIT * s_RoomSpawnObject( GAME *pGame, 
						  ROOM *pRoom, 
						  SPAWN_LOCATION *pSpawnLocation,
						  DWORD dwObjectCode );

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
inline struct GAME * RoomGetGame(
	const ROOM * room)
{
	ASSERT_RETNULL(room);
	return room->m_pGame;
}


inline ROOMID RoomGetId(
	const ROOM* room)
{
	ASSERT_RETINVALID(room);

	return room->idRoom;
}
	
inline unsigned int RoomGetIndexInLevel(
	const ROOM* room)
{
	ASSERT_RETINVALID(room);

	return room->nIndexInLevel;
}

inline void RoomSetFlag(
	ROOM* room,
	ROOM_FLAG eFlag,
	BOOL fValue = TRUE)
{
	ASSERT_RETURN(room && eFlag >= 0 && eFlag < NUM_ROOM_FLAGS);
	SETBIT(room->pdwFlags, eFlag, fValue);
}

inline int RoomTestFlag(
	const ROOM * room,
	ROOM_FLAG eFlag)
{
	ASSERT_RETFALSE(room && eFlag >= 0 && eFlag < NUM_ROOM_FLAGS);
	return TESTBIT(room->pdwFlags, eFlag);
}

inline BOOL RoomIsActive(
	ROOM * room)
{
	return RoomTestFlag(room, ROOM_ACTIVE_BIT);
}


inline int& RoomGetRootModel(
	ROOM* room)
{
	static int nINVALID_ID = (int)INVALID_ID;
	ASSERT_RETVAL( room, nINVALID_ID );
	if ( ! room->pnRoomModels.Count() )
		return nINVALID_ID;
	return room->pnRoomModels[ 0 ];
}

inline int RoomGetRootModel(
	const ROOM* room)
{
	static int nINVALID_ID = (int)INVALID_ID;
	ASSERT_RETVAL( room, nINVALID_ID );
	if ( ! room->pnRoomModels.Count() )
		return INVALID_ID;
	return room->pnRoomModels[ 0 ];
}

inline class hkRigidBody*& RoomGetRootRigidBody(
	ROOM* room)
{
	static class hkRigidBody * pNULL = NULL;
	ASSERT_RETVAL( room && room->pRoomModelRigidBodies, pNULL );
	if ( ! room->pRoomModelRigidBodies.Count() )
		return pNULL;
	return room->pRoomModelRigidBodies[ 0 ];
}

inline BOOL c_IsTraceRoom(
	int nRoomID)
{
#if ISVERSION(DEVELOPMENT)
	extern int gnTraceRoom;
	//if ( ! e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
	//	return FALSE;
	if ( gnTraceRoom == INVALID_ID || gnTraceRoom != nRoomID )
		return FALSE;
	return TRUE;
#else
	REF(nRoomID);
	return FALSE;
#endif
}

inline void c_TraceRoom(
	int nRoomID, 
	const char * pszEvent, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( c_IsTraceRoom( nRoomID ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE ROOM  %4d:   %s", nLine, nRoomID, pszEvent );
#else
	REF(nRoomID);
	REF(pszEvent);
	REF(nLine);
#endif
}

inline void c_TraceRoomInt( 
	int nRoomID, 
	const char * pszEvent, 
	const int nInt, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	if ( c_IsTraceRoom( nRoomID ) )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE ROOM  %4d:   %s [ %d ]", nLine, nRoomID, pszEvent, nInt );
#else
	REF(nRoomID);
	REF(pszEvent);
	REF(nInt);
	REF(nLine);
#endif
}

inline int c_GetFirstVisibleRoomCommandID()
{
	extern CArrayIndexed<VISIBLE_ROOM_COMMAND> gtVisibleRooms;
	return gtVisibleRooms.GetFirst();
}

inline int c_GetNextVisibleRoomCommandID( int nRoomID )
{
	extern CArrayIndexed<VISIBLE_ROOM_COMMAND> gtVisibleRooms;
	return gtVisibleRooms.GetNextId( nRoomID );
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

BOOL RoomAllConnectionsReady( 
	ROOM * pRoom );
	
int CurrentLevelGetFirstRoom(
	void);
	
int CurrentLevelGetNextRoom( 
	int nCurRoomID );
	
int RoomGetControlUnitRoom(
	void);
	
BOOL RoomIsOutdoorRoom( 
	GAME * pGame,
	ROOM * pRoom );
	
BOOL RoomUsesOutdoorVisibility( 
	int nRoomID );

BOOL RoomFileWantsObscurance( 
	const char * pszFilePath );

BOOL RoomFileOccludesVisibility( 
	const char * pszFilePath );

BOOL RoomFileInExcel( 
	const char * pszFilePath );

int RoomFileGetRoomIndexLine(
	const char * pszFilePath );

ROOM_INDEX * RoomFileGetRoomIndex(
	const char * pszFilePath );

BOOL RoomAllowsMonsterSpawn( 
	ROOM *pRoom);
	
BOOL RoomAllowsAdventures( 
	GAME *pGame,
	ROOM *pRoom);
	
void c_RoomSystemInit();
void c_RoomSystemDestroy();
void c_SetTraceRoom( int nRoomID );

void HammerShortcutRoomPopulate( GAME * pGame, ROOM * pRoom );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void RoomAddVisualPortal(
	GAME *pGame,
	ROOM *pRoom,
	UNIT *pPortal);

void RoomRemoveVisualPortal(
	GAME *pGame,
	ROOM *pRoom,
	UNIT *pPortal);

const ROOM_VISUAL_PORTAL *RoomVisualPortalFind(
	UNIT *pUnit);
	
void c_SetRoomModelsVisible( const ROOM * pRoom, BOOL bFrustumCull, const struct PLANE * pPlanes = NULL );
void c_RoomCullModels	   ( ROOM * pRoom );
void c_ClearVisibleRooms();
void c_AddVisibleRoom( ROOM * pRoom, BOUNDING_BOX & ClipBox );
void c_ProcessVisibleRoomsList();
void c_ModelSetLocation(
	int nModelID,
	const MATRIX * pmatWorld,
	const VECTOR & vPosition,
	int nRoomID,
	MODEL_SORT_TYPE SortType = MODEL_DYNAMIC,
	BOOL bForce = FALSE );

void c_ModelSetScale( int nModelID,
					  VECTOR vScale );
void c_ModelRemove(
   int nModelID );
ROOMID c_ModelGetRoomID( int nModelID );
void c_UpdateLights();

void RoomGetMaterialInfoByTriangleNumber(
	const ROOM_INDEX * pRoom,
	const int nTriangleNumber,
	int * nMaterialIndex,
	int * nMaterialIndexMapped,
	float * fDirectOcclusion,
	float * fReverbOcclusion,
	DWORD * pdwDebugColor,
	BOOL bOcclusion);

ROOM * ClientControlUnitGetRoom();
BOOL ClientControlUnitGetPosition(VECTOR & vPosition);

BOOL s_RoomSelectPossibleSubLevelEntranceLocation(
	GAME *pGame,
	ROOM *pRoom,
	struct SUBLEVEL *pSubLevel);

BOOL s_RoomAllowedForStart(
	ROOM *pRoom,
	DWORD dwWarpFlags);

BOOL RoomsAreInSameSubLevel(
	const ROOM *pRoomA,
	const ROOM *pRoomB);

BOOL RoomIsInDefaultSubLevel(
	ROOM *pRoom);

BOOL RoomGetClearFlatArea(
	ROOM *pRoom,
	float flMinNodeHeight,
	float flRadius,
	VECTOR *pvPositionWorld,
	VECTOR *pvNormal,
	BOOL bAllowEdgeNodeAnchor,
	float flFlatZTolerance);

BOOL RoomIsClearFlatArea( 
	ROOM *pRoom, 
	struct ROOM_PATH_NODE *pNode,
	float flRadius,
	float flMinNodeHeight,
	float flZTolerance);

void RoomDisplayDebugOverview();

DWORD RoomGetIterateSequenceNumber( 
	ROOM *pRoom);

inline const BOUNDING_BOX * RoomGetBoundingBoxWorldSpace(
	ROOM * room,
	VECTOR & center)
{
	center = room->vBoundingBoxCenterWorldSpace;

	return &room->tBoundingBoxWorldSpace;
}


inline const BOUNDING_BOX * RoomGetBoundingBoxWorldSpace(
	ROOM * room)
{
	return &room->tBoundingBoxWorldSpace;
}


void RoomGetBoundingSphereWorldSpace(
	ROOM *pRoom,
	float &flRadius,
	VECTOR &vCenter);

float RoomGetDistanceSquaredToPoint(
	ROOM * pRoom,
	const VECTOR & vPosition);

BOOL UseMultiMopp();

BOOL PathNodeEdgeConnectionTestTraversed(
	const PATH_NODE_EDGE_CONNECTION * pConnection);

void PathNodeEdgeConnectionSetTraversed(
	PATH_NODE_EDGE_CONNECTION * pConnection,
	BOOL bTraversed);

int PathNodeEdgeConnectionGetEdgeIndex(
	const PATH_NODE_EDGE_CONNECTION * pConnection);

void PathNodeEdgeConnectionSetEdgeIndex(
	PATH_NODE_EDGE_CONNECTION * pConnection,
	int nEdgeIndex);

BOOL RoomAllowsResurrect(
	ROOM *pRoom);

BOOL s_RoomSetupTreasureSpawn( ROOM *pRoom, 
							   int nTreasure,
							   TARGET_TYPE nType,
							   int nObjectSpawnClassFallback );

//just creates whatever the spawn class tell it to in a random position in the level
int s_RoomCreateSpawnClass( ROOM *pRoom,
							 int nSpawnClass,
							 int nCount );

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern const float DEFAULT_FLAT_Z_TOLERANCE;


#endif // _ROOM_H_
