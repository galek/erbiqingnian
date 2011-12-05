//----------------------------------------------------------------------------
// movement.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _MOVEMENT_H_
#define _MOVEMENT_H_

// drb debug
#define DEBUG_PICKUP		0

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path debugging

//#define DRB_SUPER_TRACKER

//----------------------------------------------------------------------------

#ifdef DRB_SUPER_TRACKER

BOOL drbToggleSuperTracker();
void drbSTDump( int gid, int id );

void drbST( UNIT * unit, const char * szFunction, int line );
void drbSTI( UNIT * unit, const char * szFunction, int line, int d0 = 0, int d1 = 0, int d2 = 0 );
void drbSTF( UNIT * unit, const char * szFunction, int line, float d0 = 0.0f, float d1 = 0.0f, float d2 = 0.0f );
void drbSTV( UNIT * unit, const char * szFunction, int line, VECTOR v );

#define drbSuperTracker( unit )									drbST( unit, __FUNCSIG__, __LINE__ )
#define drbSuperTrackerI( unit, data0 )							drbSTI( unit, __FUNCSIG__, __LINE__, data0 )
#define drbSuperTrackerI2( unit, data0, data1 )					drbSTI( unit, __FUNCSIG__, __LINE__, data0, data1 )
#define drbSuperTrackerI3( unit, data0, data1, data2 )			drbSTI( unit, __FUNCSIG__, __LINE__, data0, data1, data2 )
#define drbSuperTrackerF( unit, data0 )							drbSTF( unit, __FUNCSIG__, __LINE__, data0 )
#define drbSuperTrackerV( unit, data0 )							drbSTV( unit, __FUNCSIG__, __LINE__, data0 )
#define drbSTASSERT( exp, unit )								{ if (!(exp)) { drbSTDump( GameGetId(UnitGetGame(unit)),(int)UnitGetId(unit) ); ASSERT(exp); } }
#define drbSTASSERT_RETURN( exp, unit )							{ if (!(exp)) { drbSTDump( GameGetId(UnitGetGame(unit)),(int)UnitGetId(unit) ); ASSERT_RETURN(exp); } }
#define drbSTASSERT_RETFALSE( exp, unit )						{ if (!(exp)) { drbSTDump( GameGetId(UnitGetGame(unit)),(int)UnitGetId(unit) ); ASSERT_RETFALSE(exp); } }

#else

#define drbSuperTracker( unit )									//drbST
#define drbSuperTrackerI( unit, data0 )							//drbSTI
#define drbSuperTrackerI2( unit, data0, data1 )					//drbSTI
#define drbSuperTrackerF( unit, data0 )							//drbSTF
#define drbSuperTrackerV( unit, data0 )							//drbSTV
#define drbSTASSERT( exp, unit )								ASSERT( exp )
#define drbSTASSERT_RETURN( exp, unit )							ASSERT_RETURN( exp )
#define drbSTASSERT_RETFALSE( exp, unit )						ASSERT_RETFALSE( exp )

#endif

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define UNIT_DISTANCE_CLOSE (0.5f)
extern float GRAVITY_ACCELERATION;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path - debugging

#define DRB_SPLINE_PATHING

#define DRB_DIST		( 2.5f * 2.5f )

#define REPATH_FUDGE_FACTOR 0.2f



#define DRB_HAPPY_PATHING

// drb debug
#ifdef DEBUG_PICKUP
extern BOOL gDrawNearby;
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
enum UNITMODE;

#include "astar.h"
//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
enum PATHING_STATES
{
	PATHING_STATE_INVALID = -1,
	PATHING_STATE_INACTIVE = PATHING_STATE_INVALID,
	PATHING_STATE_ABORT = 0,
	PATHING_STATE_ACTIVE = 1,
	PATHING_STATE_ACTIVE_CLIENT = 2,
};

#define MAX_CLIENT_FAST_PATH_SIZE		16
#define MAX_PATH_NODE_FUDGE .3f		// maximum radius around pathnode that player can precisely be at.

#define MAX_PATH_OCCUPIED_NODES 16
#define MAX_HAPPY_PLACE_HISTORY 4

struct PATH_STATE_LOCATION
{
	ROOMID				m_nRoomId;
	int					m_nRoomPathNodeIndex;
};

struct HAPPY_PLACE
{
	PATH_STATE_LOCATION	m_HappyPlace;
	GAME_TICK			m_tiCreateTick;
};

struct PATH_STATE
{
	// current path state, inactive, invalid, active, abort, etc
	PATHING_STATES		m_eState;

	VECTOR				m_vDirection;
	GAME_TICK			m_tiInitTick;

	// where I am
	//ROOMID				m_nRoomId;
	//int					m_nRoomPathNodeIndex;
	PATH_STATE_LOCATION	m_tOccupiedLocation;

	PATH_STATE_LOCATION	m_pOccupiedNodes[MAX_PATH_OCCUPIED_NODES];

	// client-side optimized path
#if HELLGATE_ONLY
	int					cFastPathIndex;
	ASTAR_OUTPUT		cFastPathNodes[ MAX_CLIENT_FAST_PATH_SIZE ];
	int					cFastPathSize;

	int					m_nHappyPlaceCount;
	HAPPY_PLACE			m_HappyPlaces[MAX_HAPPY_PLACE_HISTORY];
#else
	SIMPLE_DYNAMIC_ARRAY<unsigned int> m_SortedFaces;
	VECTOR				m_LastSortPosition;
#endif
	// path finding testing hash
	CHash<PATH_NODE_SHOULD_TEST> m_tShouldTestHash;

	// current path with all nodes, etc
	int					m_nCurrentPath;
	int					m_nPathNodeCount;
	ASTAR_OUTPUT		m_pPathNodes[DEFAULT_ASTAR_LIST_SIZE];

	// new hellgate spline code
#ifdef DRB_SPLINE_PATHING
	VECTOR				vStart;
	TIME				m_Time;
#endif

	// TRAVIS: one more than astar_list_size - this is for the special non-node based point at the end.
	float				m_fPathNodeDistance[DEFAULT_ASTAR_LIST_SIZE+1];


#ifdef _DEBUG
	int	*				m_pParticleIds;
	int					m_nParticleIdCount;
#endif

	PATH_STATE() {}
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
// add a unit to the step list (units that move every frame)
void UnitStepListAdd(
	struct GAME* game,
	struct UNIT* unit);

enum
{
	SLRF_FORCE								= MAKE_MASK(0),
	SLRF_DONT_SEND_PATH_POSITION_UPDATE		= MAKE_MASK(1),
};

void UnitStepListRemove(
	struct GAME* game,
	struct UNIT* unit,
	DWORD dwFlags = 0);

void UnitDoPathTermination(
	GAME* game,
	UNIT* unit );

BOOL UnitOnStepList(
	struct GAME* game,
	struct UNIT* unit);

// call once per tick to move all units on the step list
void DoUnitStepping(
	struct GAME* game);

// call every draw frame on the client to update interpolated positions between steps
void DoUnitStepInterpolating(
	struct GAME* game,
	float fTimeDelta,
	float fRealTimeDelta);

void UnitForceToGround(
	struct UNIT* unit);

struct PATH_STATE* PathStructInit(
	struct GAME* game);

void PathStructClear(
	struct UNIT* pUnit);

void PathStructFree(
	struct GAME* game,
	struct PATH_STATE* path);

//----------------------------------------------------------------------------
// drb.path

BOOL UnitClearPathOccupied( UNIT * unit );
BOOL UnitClearPathNextNode( UNIT * unit );
BOOL UnitClearBothPathNodes( UNIT * unit );

BOOL UnitReservePathOccupied( UNIT * unit );
BOOL UnitReservePathNextNode( UNIT * unit );
BOOL UnitReservePathNodeArea( UNIT * pUnit, ROOM * pRoom, BOUNDING_BOX * pBox );

BOOL UnitTestPathNextNodeIsClear( UNIT * unit );

BOOL UnitClearBothSetOccupied( UNIT * unit );

void UnitInitPathNodeOccupied( UNIT * unit, ROOM * room, int index );
void UnitErasePathNodeOccupied( UNIT * unit );
void UnitGetPathNodeOccupied( UNIT * unit, ROOM ** room, int * index );

BOOL UnitPathIsActive( UNIT * unit );
BOOL UnitPathIsActiveClient( UNIT * unit );

int UnitGetPathNodeCount( UNIT * unit );

void UnitPathAbort( UNIT * unit );
void UnitEndPathReserve( UNIT * unit );

// client side
void c_UnitSetPathServerLoc( UNIT * unit, ROOM * room, unsigned int index );
void c_UpdatePathing( GAME * game, UNIT * unit, VECTOR & vPositionPrev, VECTOR & vNewPosition, COLLIDE_RESULT * result );

// server side
void s_UpdatePathing( GAME * game, UNIT * unit, VECTOR & vPositionPrev, VECTOR & vNewPosition, BOOL bIsPlayer = FALSE );
BOOL s_UnitTestPathSync(GAME *game, UNIT *unit);

void UnitStopPathing( UNIT * unit );

//----------------------------------------------------------------------------

enum
{
	PSS_CLIENT_STRAIGHT_BIT,
	PSS_APPEND_DESTINATION_BIT,
};

void PathStructSerialize(
	GAME * pGame,
	struct PATH_STATE* path,
	BIT_BUF & tBuffer);

void PathStructDeserialize(
	struct UNIT* pUnit,
	BYTE * tBuffer,
	BYTE bFlags = 0);

struct ROOM_PATH_NODE * PathStructGetEstimatedLocation(
	struct UNIT* unit,
	struct ROOM *& pEstimatedRoom);

class CHash<struct PATH_NODE_SHOULD_TEST> * PathStructGetShouldTest(
	struct UNIT* unit);

class CHash<struct PATH_NODE_SHOULD_TEST> * PathStructGetShouldTest(
	struct PATH_STATE * path);

void s_UnitAutoPickupObjectsAround(
	struct UNIT* unit);

VECTOR UnitCalculateMoveDirection(
	GAME* game, 								  
	UNIT* unit,
	BOOL bRealStep);

BOOL UnitCalculateTargetReached(
	struct GAME* game,
	struct UNIT* unit,
	const struct VECTOR& vPositionPrev,
	struct VECTOR& vNewPosition,
	BOOL bCollided,
	BOOL bOldOnGround,
	BOOL bOnGround);

VECTOR CalculatePathTargetFly( 
	GAME * game, 
	float fAltitude, 
	float fCollisionHeight, 
	ROOM *room, 
	ROOM_PATH_NODE *roomPathNode );

VECTOR UnitCalculatePathTargetFly( 
	GAME* game, 
	UNIT* unit, 
	ROOM* room, 
	struct ROOM_PATH_NODE* roomPathNode);

VECTOR UnitCalculatePathTargetFly( 
	GAME* game, 
	UNIT* unit, 
	struct ASTAR_OUTPUT* outputNode);

enum PATH_CALCULATION_TYPE
{
	PATH_SIMPLEST,

	PATH_STRAIGHT = PATH_SIMPLEST,
	PATH_ASTAR,

	PATH_FULL = PATH_ASTAR,

	// Custom path types.  These don't fall through
	PATH_WANDER,
};

enum PATH_CALCULATION_FLAGS
{
	PCF_ACCEPT_PARTIAL_SUCCESS_BIT,
	PCF_DISALLOW_EXPENSIVE_PATHNODE_LOOKUPS_BIT,
	PCF_CALCULATE_SOURCE_POSITION_BIT,
	PCF_USE_HAPPY_PLACES_BIT,
	PCF_CHECK_MELEE_ATTACK_AT_DESTINATION_BIT,
	PCF_TEST_ONLY_BIT,
	PCF_ALREADY_FOUND_PATHNODE_BIT,
};

BOOL UnitCalculatePath(
	struct GAME* game,
	struct UNIT * unit,
	PATH_CALCULATION_TYPE eMaxPathCalculation = PATH_FULL,
	DWORD dwFlags = 0);

BOOL ClientCalculatePath(
	GAME * pGame,
	UNIT * pUnit,
	UNITMODE eMode,
	PATH_CALCULATION_TYPE eMaxPathCalculation = PATH_FULL,
	DWORD dwFlags = 0);

BOOL UnitFindGoodHappyPlace(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vHappyPlace);

void UnitActivatePath(
	struct UNIT * unit,
	BOOL bSendPathPositionUpdate = TRUE);

void MovementCheckDebugUnit(
	UNIT *pUnit,
	const char *pszLabel,
	const VECTOR *pVector);

void s_SendUnitPathPositionUpdate(
	UNIT *pUnit );

void c_SendUnitPathPositionUpdate(
	UNIT *pUnit );

void s_SendUnitWarp(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags);

void s_SendUnitWarpToOthers(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags);

void c_SendUnitWarp(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags);

void s_SendUnitMoveXYZ(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vTargetPosition,
	const VECTOR &vMoveDirection,
	UNIT * pOptionalFaceTarget,
	BOOL bSendPath,
	GAMECLIENTID idClientExclude = INVALID_ID );

static const float MoveThresholdSq = 1.0 * 1.0; // movement distance trigger for level prox map update
static const float DetectionRangeNormal = 20.0; // detection range outside of which unit move messages may be deferred
static const float DetectionRangeShadowLands = 50.0f; // we detect further in shadowlands, go pvp!
void s_SendDeferredMoveMsgsToProximity(UNIT* pUnit);

// sends player's path to server
void c_SendUnitMoveXYZ(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vTargetPosition,
	const VECTOR &vMoveDirection,
	BOOL bSendPath);

// sends player's direction to server - tugboat
void c_SendUnitSetFaceDirection(
	UNIT *pUnit,
	const VECTOR &vMoveDirection );

enum UNITMOVEID_FLAGS
{
	UMID_NO_PATH,
	UMID_USE_EXISTING_PATH,
	UMID_CLIENT_STRAIGHT,
};

void s_SendUnitMoveID(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	UNITID idTarget,
	const VECTOR &vDirection);

void s_SendUnitMoveDirection(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vDirection);

void s_SendUnitPathChange(
	UNIT *pUnit );

void CopyPathToSpline( 
	GAME * pGame,
	UNIT* pUnit,
	const VECTOR* vDestination,
	BOOL AppendDestination );


void PathSlideToDestination( GAME * pGame,
							UNIT * pUnit,
							float fTimeDelta );

BOOL PathPrematureEnd( GAME * pGame,
					  UNIT * pUnit,
					  const VECTOR& vPosition );
void UnitPathingInit(
	UNIT *unit);

void UnitPositionTrace(
	UNIT *pUnit,
	const char *pszLabel);

void c_UIUpdatePathingSplineDebug();

#endif // _MOVEMENT_H_
