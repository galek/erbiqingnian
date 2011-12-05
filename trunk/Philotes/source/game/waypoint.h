//----------------------------------------------------------------------------
// waypoint.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_WAYPOINT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _WAYPOINT_H_

#include "statssave.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_WAYPOINTS			1024
#define MAX_WAYPOINT_BLOCKS		(DWORD_FLAG_SIZE(MAX_WAYPOINTS))


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL WaypointSystemInit(
	struct EXCEL_TABLE * table);
	
void WaypointSystemFree(
	void);

BOOL WaypointIsMarked( 
	GAME * game, 
	UNIT * player, 
	int level_definition,
	int nDifficulty);

void WaypointGive(
	GAME * game,
	UNIT * player,
	const struct LEVEL_DEFINITION *pLevelDef);
	
void WaypointCreateListForPlayer( 
	GAME * game, 
	UNIT * player, 
	const WCHAR ** level_names, 
	int * table_index, 
	unsigned int * list_length);

//----------------------------------------------------------------------------
// SERVER-ONLY EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL s_PlayerInitWaypointFlags(
	GAME * game,
	UNIT * player);

BOOL s_LevelSetWaypoint(
	GAME * game,
	UNIT * player,
	UNIT * marker);

BOOL s_LevelWaypointWarp(
	GAME * game,
	struct GAMECLIENT * client,
	UNIT * unit,	
	int leveldepth,
	int levelarea );			//Tugboat only

unsigned int WaypointSystemGetCount(
	void);
	
BOOL s_WaypointsClearAll(
	UNIT* player,
	int nDifficulty);


#if ISVERSION(DEVELOPMENT)
BOOL s_LevelSetAllWaypoints(
	GAME * game,
	UNIT * player);
#endif
	

//----------------------------------------------------------------------------
// CLIENT-ONLY EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void c_LevelOpenWaypointUI(
	GAME * game);


void c_WaypointTryOperate(
	GAME * game,
	UNIT * player,
	unsigned int clicked);

enum STAT_VERSION_RESULT s_VersionStatWaypointFlag(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit);

enum STAT_VERSION_RESULT s_VersionStatStartLevel(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit);

#endif // _WAYPOINT_H_