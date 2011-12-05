//----------------------------------------------------------------------------
// collision.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _COLLISION_H_
#define _COLLISION_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _APPCOMMON_H_
#include "appcommon.h"
#endif

#ifndef _BOUNDINGBOX_H_
#include "boundingbox.h"
#endif

#ifndef _QUADTREE_H
#include "quadtree.h"
#endif


#define MAX_UP_DELTA				0.5f		// max height you can walk over without being blocked
#define MAX_ROOMS_CONNECTED			10
#define PROP_MATERIAL				100
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct ROOM_DEFINITION;
struct ROOM_LAYOUT_GROUP;
//----------------------------------------------------------------------------
// ENUM
//----------------------------------------------------------------------------
enum
{
	MOVERESULT_COLLIDE,
	MOVERESULT_SETONGROUND,
	MOVERESULT_TAKEOFFGROUND,
	MOVERESULT_COLLIDE_CEILING,
	MOVERESULT_COLLIDE_UNIT,
	MOVERESULT_OUTOFBOUNDS,
};

//-------------------------------------------------------------------------------------------------
// Room Collide Sphere Flags
enum RCSFLAGS
{
	RCSFLAGS_NONE = 0,
	RCSFLAGS_FLOORS = MAKE_MASK(1),
	RCSFLAGS_WALLS = MAKE_MASK(2),
	RCSFLAGS_CEILINGS = MAKE_MASK(3),
	RCSFLAGS_PROPS = MAKE_MASK(4),
	RCSFLAGS_Z_ADJUST = MAKE_MASK(5),
	RCSFLAGS_CAMERA = MAKE_MASK(6),
	RCSFLAGS_RTS_CAMERA = MAKE_MASK(7),

	RCSFLAGS_ALL = ( RCSFLAGS_FLOORS | RCSFLAGS_WALLS | RCSFLAGS_CEILINGS | RCSFLAGS_PROPS ),
	RCSFLAGS_FLOORS_ONLY = ( RCSFLAGS_FLOORS | RCSFLAGS_PROPS ),
	RCSFLAGS_NO_CEILINGS = ( RCSFLAGS_FLOORS | RCSFLAGS_WALLS | RCSFLAGS_PROPS ),
	RCSFLAGS_WALLS_AND_FLOORS = ( RCSFLAGS_WALLS | RCSFLAGS_FLOORS ),
	RCSFLAGS_FLOORS_ONLY_UNIT = ( RCSFLAGS_FLOORS_ONLY | RCSFLAGS_Z_ADJUST ),
	RCSFLAGS_GROUND_UNIT = ( RCSFLAGS_FLOORS | RCSFLAGS_PROPS | RCSFLAGS_WALLS | RCSFLAGS_Z_ADJUST ),
	RCSFLAGS_ALL_AND_CAMERA = ( RCSFLAGS_ALL | RCSFLAGS_CAMERA ),
	RCSFLAGS_ALL_AND_RTS_CAMERA = ( RCSFLAGS_ALL | RCSFLAGS_RTS_CAMERA ),
	RCSFLAGS_EITHER_CAMERA = ( RCSFLAGS_CAMERA | RCSFLAGS_RTS_CAMERA )
};


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct ROOM_POINT
{
	VECTOR			vPosition;
	int				nIndex;	// used for saving and loading
};

struct ROOM_CPOLY
{
	SAFE_PTR(ROOM_POINT*, pPt1);
	SAFE_PTR(ROOM_POINT*, pPt2);
	SAFE_PTR(ROOM_POINT*, pPt3);
	
	VECTOR			vNormal;
	BOUNDING_BOX	tBoundingBox;
};

struct AABB
{
	VECTOR				vCenter;
	float				fRadius[3];
};

struct COLLIDE_RESULT
{
	DWORD			flags;
	struct UNIT *	unit;
};


//----------------------------------------------------------------------------
struct HAVOK_SHAPE_HOLDER 
{
	SAFE_PTR(class hkShape*, pHavokShape);
	DWORD			dwMemSizeAndFlags;
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

inline float CollisionWalkableZNormal(
	void)
{
	// any Z normal component grater than this angle will be walkable
	if (AppIsTugboat())
	{
		return 0.707f;
	}
	else
	{
		return 0.42262f;
	}
}

void AABBComputeFromPointList(
	AABB* aabb,
	VECTOR* pointlist,
	int count);

void LevelCollisionInit(
	struct GAME* game,
	struct LEVEL * level);

void LevelCollisionFree(
	struct GAME* game,
	struct LEVEL * level);

void LevelCollisionAddRoom(
	struct GAME* game, 
	struct LEVEL * level,
	struct ROOM* room);

bool LevelCollisionRoomPopulated( LEVEL* level,
								 ROOM* room );

void LevelCollisionSetRoomPopulated( LEVEL* level,
								     ROOM* room );
								 
void LevelCollisionAddProp(
	GAME* game, 
	LEVEL * level,
	ROOM_DEFINITION* roomdef,
	ROOM_LAYOUT_GROUP* pLayoutGroup,
	MATRIX* matrix,
	BOOL bUseMatID = FALSE);

enum LEVEL_COLLIDE_FLAGS
{
	LCF_CHECK_OBJECT_CYLINDER_COLLISION_BIT,
};

float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal = NULL,
	DWORD dwFlags = 0,
	int nIgnoreMaterial = -1);

float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	int& FaceMaterial,
	VECTOR* pvNormal = NULL,
	DWORD dwFlags = 0,
	int nIgnoreMaterial = -1);

float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal = NULL,
	int nIgnoreMaterial = -1);

float LevelLineCollideLen( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	int& FaceMaterial,
	VECTOR* pvNormal = NULL,
	int nIgnoreMaterial = -1);

void LevelSortFaces( 
	struct GAME* pGame, 
	struct LEVEL * pLevel,
	SIMPLE_DYNAMIC_ARRAY<unsigned int> &SortedFaces,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen );


float ClientControlUnitLineCollideLen( 
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal = NULL);
	
// This may need to change if more information is required?
typedef void (*CollisionCallback)(struct ROOM_INDEX * pRoomDef, ROOM * pRoom, VECTOR vLocation, int nTriangleNumber, float fHitFraction, void * userdata);
#if HELLGATE_ONLY
void LevelLineCollideAll(
	struct GAME* pGame,
	struct LEVEL * pLevel,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	CollisionCallback callbackFunc,
	void * userdata);
#endif
void ClientControlUnitLineCollideAll(
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	CollisionCallback callbackFunc,
	void * userdata);

// #ifdef HAMMER
float LevelLineCollideGeometry(
	struct ROOM * pRoom,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	int & nModelHitId,
	float fMaxLen,
	VECTOR * pvNormal,
	int nNoCollideCount = 0,
	int *pnNoCollideModelIds = NULL);

int LevelLineCollideModelId(
	ROOM * pRoom,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fMaxLen,
	VECTOR * pvNormal,
	int nNoCollideCount,
	int *pnNoCollideModelIds);
// #endif

void RoomCollide( 
	struct UNIT* pUnit, 
	const VECTOR& vPositionPrev, 
	const VECTOR& vVelocity, 
	VECTOR* pvPositionNew, 
	float fRadius, 
	float fHeight,
	DWORD dwFlags,
	COLLIDE_RESULT * pResult);


void RoomCollide( 
	struct UNIT* pUnit, 
	SIMPLE_DYNAMIC_ARRAY< ROOM* >*	PreSortedRooms,
	const VECTOR& vPositionPrev, 
	const VECTOR& vVelocity, 
	VECTOR* pvPositionNew, 
	float fRadius, 
	float fHeight,
	DWORD dwFlags,
	COLLIDE_RESULT * pResult);


BOOL RoomTestPos(
	struct GAME* game, 
	struct ROOM* room,
	const VECTOR& vPositionPrev,
	VECTOR* pvPositionNew,
	float fRadius,
	float fHeight,
	DWORD dwFlags);

#define SELECTTARGET_FLAG_FATTEN_UNITS					MAKE_MASK( 1 )
#define SELECTTARGET_FLAG_CHECK_LOS_TO_AIM_POS			MAKE_MASK( 2 )
#define SELECTTARGET_FLAG_ALLOW_NOTARGET				MAKE_MASK( 3 )
#define SELECTTARGET_FLAG_RESELECT_PREVIOUS				MAKE_MASK( 4 )
#define SELECTTARGET_FLAG_IGNORE_DIRECTION				MAKE_MASK( 5 )
#define SELECTTARGET_FLAG_ONLY_IN_FRONT_OF_UNIT			MAKE_MASK( 6 )
#define SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT			MAKE_MASK( 7 )
#define SELECTTARGET_FLAG_DONT_RESELECT_PREVIOUS		MAKE_MASK( 8 )
#define SELECTTARGET_FLAG_DONT_RETURN_SELECTION_ERROR	MAKE_MASK( 9 )

UNIT * SelectTarget(
	struct GAME * pGame,
	struct UNIT * pUnit,
	DWORD dwFlags,
	const VECTOR & vPosition,
	const VECTOR & vDirection,
	float fRange,
	BOOL * pbIsInRange = NULL,
	float * pfDistance = NULL,
	VECTOR * pvNormal = NULL,
	int nSkill = INVALID_ID,
	struct UNIT * pWeapon = NULL,
	UNITID idPrevious = INVALID_ID,
	enum UNITTYPE * pUnitType = NULL,
	float* fLevelLineCollideResults = NULL );

UNIT* SelectDebugTarget(
	struct GAME* game,
	struct UNIT* unit,
	const VECTOR& vPosition,
	const VECTOR& vDirection,
	float fRange);

/*UNIT* c_FindSelectionForItemPickup(
	GAME* game,
	UNIT* unit,
	UNIT* oldselection);*/

BOOL RoomCollideNew( 
	struct UNIT * pUnit, 
	const VECTOR & vPositionPrev, 
	const VECTOR & vVelocity, 
	VECTOR * pvPositionNew, 
	DWORD dwFlags );

#define DISTANCE_POINT_FLAGS_NONE				0
#define DISTANCE_POINT_FLAGS_WITH_Z_RANGE		MAKE_MASK( 1 )

BOOL DistancePointLine3D( VECTOR * pOrig, VECTOR * pStart, VECTOR * pEnd, float * pfDist, VECTOR * pIntersection = NULL, DWORD dwFlags = DISTANCE_POINT_FLAGS_WITH_Z_RANGE );
BOOL DistancePointLineXY( VECTOR * pOrig, VECTOR * pStart, VECTOR * pEnd, float * pfDist, DWORD dwFlags = DISTANCE_POINT_FLAGS_NONE );

BOOL ClearLineOfSightBetweenPositions(
	LEVEL *pLevel,
	const VECTOR &vPosFrom,
	const VECTOR &vPosTo);

BOOL ClearLineOfSightBetweenUnits(
	UNIT *pUnitFrom,
	UNIT *pUnitTo);
	
float LevelSphereCollideLen(
	struct GAME * pGame, 
	struct LEVEL * pLevel,
	const VECTOR & vPosition, 
	const VECTOR & vDirection, 
	float fRadius,
	float fMaxLen,
	RCSFLAGS eFlags = RCSFLAGS_ALL );

#endif // _COLLISION_H_