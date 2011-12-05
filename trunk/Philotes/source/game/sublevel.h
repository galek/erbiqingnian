//----------------------------------------------------------------------------
// FILE: 
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef _SUBLEVEL_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _SUBLEVEL_H_


//----------------------------------------------------------------------------
// INCLDUE
//----------------------------------------------------------------------------
#ifndef _ROOM_H_
#include "room.h"
#endif


//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------
#define DEFAULT_SUBLEVEL_ID (0)
#define SUBLEVEL_ENTRANCE_NAME_LEN (32)
#define MAX_RANDOM_ROOMS (3200)  // arbitrary, increase as needed

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct LEVEL;
struct SUBLEVEL_DESC;
enum ROOM_ITERATE_UNITS;
enum TARGET_TYPE;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum SUBLEVEL_CONSTANTS
{
	MAX_SUBLEVEL_NAME = 64,		// arbitrary
};

//----------------------------------------------------------------------------
enum SUBLEVEL_TYPE
{
	ST_INVALID,
	
	ST_HELLRIFT,
	ST_TRUTH_A_OLD,
	ST_TRUTH_A_NEW,
	ST_TRUTH_B_OLD,
	ST_TRUTH_B_NEW,
	ST_TRUTH_C_OLD,
	ST_TRUTH_C_NEW,
	ST_TRUTH_D_OLD,
	ST_TRUTH_D_NEW,
	ST_TRUTH_E_OLD,
	ST_TRUTH_E_NEW,
	
	ST_NUM_TYPES
	
};

//----------------------------------------------------------------------------
struct SUBLEVEL_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];
	int nDRLGDef;					// DRLG for sub level
	BOOL bAllowTownPortals;			// allows town portals
	BOOL bAllowMonsterDistribution;	// allows monster spawns by distribution
	BOOL bHeadstoneAtEntranceObject;// if players die, headstone will be created at entrance object of sublevel
	BOOL bRespawnAtEntrance;		// if it ever comes up that a player can respawn at an object inside a this sublevel, instead we will put them at the sublevel entrance
	BOOL bPartyPortalsAtEntrance;	// if a party member opens a portal to a player that is in a sublevel with this set, the party portal will be opened at the sublevel entrance instead
	SUBLEVEL_TYPE eType;			// enum for types
	VECTOR vDefaultPosition;		// position in world
	float flEntranceFlatZTolerance; // for auto created entrances, they can appear on terrain that is this "flat" 
	float flEntranceFlatRadius;		// for auto created entrances, they can appear on flat terrain areas of this size
	float flEntranceFlatHeightMin;	// auto created entrances must have this Z clearance at the path nodes
	BOOL bAutoCreateEntrance;		// auto create entrance when creating level
	int nObjectEntrance;			// object that leads to this sub level
	int nObjectExit;				// object used to exit sub level
	int nAlternativeEntranceUnitType;		// alternative allowable unit type to link up sublevels (for fake visual portals)
	int nAlternativeExitUnitType;	// alternative allowable unit type to link up sublevels (for fake visual portals)
	BOOL bAllowLayoutMarkersForEntrance;	// when auto creating entrances, select from specific layout locations	
	char szLayoutMarkerEntrance[ SUBLEVEL_ENTRANCE_NAME_LEN ];	// name of layout markers to look for
	BOOL bAllowPathNodesForEntrance;	// when auto creating entrances, select locations from random path nodes
	int nSubLevelDefNext;			// next sub level (for quests at present)
	BOOL bOverrideLevelSpawns;		// ignore level spawn class for spawning here
	int nSpawnClass;				// what monsters to spawn here instead (if override is set)
	int nWeatherSet;				// what weather is allowed on this level	
	int nStateWhenInside;			// players have this state when they are inside this sublevel
};

//----------------------------------------------------------------------------
enum SUB_LEVEL_STATUS
{
	SLS_POPULATED_BIT,				// sub level has been populated
	SLS_RESURRECT_RESTRICTED_BIT,	// when set, nobody can resurrect when in this sublevel
	
	SLS_NUM_SUBLEVEL_STATUS_BITS	// keep this last
};

//----------------------------------------------------------------------------
struct SUBLEVEL_DOORWAY
{
	UNITID idDoorway;		// entrance/exit unit (if any)
	VECTOR vPosition;
	VECTOR vDirection;
	ROOMID idRoom;
};

//----------------------------------------------------------------------------
struct SPAWN_TRACKER
{
	int	nSpawnedCount;		// distribution # things spawned in sublevel
	int	nDesiredCount;		// distribution # of things wanted in sublevel
};

//----------------------------------------------------------------------------
enum DRLG_SPAWN_TYPE
{
	DRLG_SPAWN_TYPE_INVALID = -1,

	DRLG_SPAWN_TYPE_CRITTER,
	DRLG_SPAWN_TYPE_ENVIRONMENT,
	
	NUM_DRLG_SPAWN_TYPE				// keep this last
};

//----------------------------------------------------------------------------
enum SUBLEVEL_DOORWAY_TYPE
{
	SD_INVALID = -1,
	
	SD_ENTRANCE,					// entrance to sublevel (warp unit is *not* in the sublevel itself)
	SD_EXIT,						// exit from sublevel (warp unit *is* inside the sublevel itself)
	
	SD_NUM_DOORWAYS					// keep this last
	
};

//----------------------------------------------------------------------------
struct SUBLEVEL
{
	LEVEL *								pLevel;									// pointer back to level sublevel is in
	const struct WEATHER_DATA *			pWeather;								// The weather override in this sublevel.  It's perfectly legitimate for this to be NULL.
	char								szName[MAX_SUBLEVEL_NAME];				// name for debugging
	VECTOR								vPosition;								// position of sub-level (or offset) in level
	SUBLEVELID							idSubLevel;								// id for this sublevel (unique among all sub levels *in* a level)
	UNITID								idEntranceAux;							// entrance aux object
	SUBLEVEL_DOORWAY					tDoorway[SD_NUM_DOORWAYS];				// sublevel entrances and exits
	SPAWN_TRACKER						tSpawnTracker[NUM_DRLG_SPAWN_TYPE];		// to keep track of how many of each drlg spawn type is spawned/needed
	DWORD								dwSubLevelStatus;						// see SUB_LEVEL_STATUS
	int									nSubLevelDef;							// sublevel definition (row in .xls)
	int									nEngineRegion;							// each sublevel has an index for the engine code	
	int									nRoomCount;								// number of rooms in the sublevel
	int									nMonsterCount;							// distribution # of monsters spawned in level
	int									nDesiredMonsters;						// distribution # of monsters wanted in level
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

const SUBLEVEL_DEFINITION *SubLevelGetDefinition(
	int nSubLevelDef);

const SUBLEVEL_DEFINITION *SubLevelGetDefinition(
	const SUBLEVEL *pSubLevel);

const char *SubLevelGetDevName( 
	const SUBLEVEL *pSubLevel);

const char *SubLevelGetDevName( 
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel);

void SubLevelInit(
	SUBLEVEL *pSubLevel,
	SUBLEVELID idSubLevel,
	int nSubLevelDef,
	LEVEL *pLevel);

GAME *SubLevelGetGame( 
	const SUBLEVEL *pSubLevel);
	
void SubLevelSetPosition(
	SUBLEVEL *pSubLevel,
	const VECTOR &vPosition);
		
LEVEL *SubLevelGetLevel(
	const SUBLEVEL *pSubLevel);

int SubLevelGetDRLG(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel);

VECTOR SubLevelGetPosition(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel);
		
SUBLEVELID SubLevelAdd(
	LEVEL *pLevel,
	int nSubLevelDef);

SUBLEVELID SubLevelAdd(
	LEVEL *pLevel, 
	const SUBLEVEL_DESC *pSubLevelDesc);

SUBLEVEL *SubLevelGetById(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel);

SUBLEVEL *SubLevelGetByIndex(
	LEVEL *pLevel,
	int nIndex);

SUBLEVELID SubLevelGetId(
	const SUBLEVEL *pSubLevel);
	
typedef	void (* PFN_SUBLEVEL_ITERATE)( LEVEL *pLevel, SUBLEVEL *pSubLevel, void *pCallbackData );

void SubLevelsIterate(
	LEVEL *pLevel,
	PFN_SUBLEVEL_ITERATE pfnCallback,
	void *pCallbackData);

void SubLevelClearAll( 
	LEVEL *pLevel);

SUBLEVELID SubLevelFindFirstOfType( 
	LEVEL *pLevel, 
	int nSubLevelDefDest);

SUBLEVEL_TYPE SubLevelGetType(
	const SUBLEVEL *pSubLevel);

BOOL SubLevelAllowsTownPortals( 
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel);

int s_SubLevelGetSpawnClass(
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel);

BOOL s_SubLevelOverrideLevelSpawns(
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel);

void s_SubLevelPopulate(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel,
	UNIT *pActivator);
	
BOOL s_SubLevelAllowsMonsterDistribution(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel);

void SubLevelSetStatus(
	SUBLEVEL *pSubLevel,
	SUB_LEVEL_STATUS eStatus,
	BOOL bEnable = TRUE);

BOOL SubLevelTestStatus(
	SUBLEVEL *pSubLevel,
	SUB_LEVEL_STATUS eStatus);

void s_SubLevelSendStatus( 
	SUBLEVEL *pSubLevel, 
	UNIT *pUnit);

typedef	BOOL (* PFN_RANDOM_ROOM_FILTER)( ROOM *room, void *pFilterCallbackData );
enum RANDOM_ROOM_FLAGS
{
	RRF_PLAYERLESS_BIT,						// only rooms without players
	RRF_HAS_MONSTERS_BIT,					// only rooms with monsters
	RRF_SPAWN_POINTS_BIT,					// only rooms with spawn points
	RRF_PATH_NODES_BIT,						// only rooms with path nodes
	RRF_MUST_ALLOW_MONSTER_SPAWN_BIT,		// only rooms that allow monster spawns
	RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT,	// do not allow rooms that have any no spawn path nodes in them
	RRF_ACTIVE_BIT,							// only rooms with their active bit set
};

ROOM *s_SubLevelGetRandomRoom(
	SUBLEVEL *pSubLevel,
	DWORD dwRandomRoomFlags,	// see RANDOM_ROOM_FLAGS
	PFN_RANDOM_ROOM_FILTER pfnFilter,
	void *pFilterCallbackData);
	
int s_SubLevelGetRandomRooms(
	SUBLEVEL *pSubLevel,
	ROOM **pResultRooms,
	int nNumResultRooms,
	DWORD dwRandomRoomFlags,	// see RANDOM_ROOM_FLAGS
	PFN_RANDOM_ROOM_FILTER pfnFilter,
	void *pFilterCallbackData);

UNIT *s_SubLevelFindFirstUnitOf( 
	const SUBLEVEL *pSubLevel, 
	TARGET_TYPE *pTargetTypes, 
	GENUS eGenus,
	int nClass);

void s_SubLevelSetDoorway( 
	SUBLEVEL *pSubLevel, 
	UNIT *pDoorway,
	SUBLEVEL_DOORWAY_TYPE eDoorway,
	BOOL bInitializing = FALSE);

const SUBLEVEL_DOORWAY *s_SubLevelGetDoorway(
	SUBLEVEL *pSubLevel,
	SUBLEVEL_DOORWAY_TYPE eDoorway);

void s_SubLevelTransition(
	UNIT *pUnit,
	SUBLEVELID idSubLevelOld,
	ROOM *pRoomOld,
	SUBLEVELID idSubLevelNew,
	ROOM *pRoomNew);

void s_SubLevelSetupDoorways(
	SUBLEVEL *pSubLevel);

void s_SubLevelWarpCreated(
	UNIT *pWarp);
	
BOOL s_SubLevelHasALiveMonster( 
	const SUBLEVEL *pSubLevel);
#endif