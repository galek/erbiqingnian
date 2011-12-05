//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_GAMEMAPS_H_
#define __S_GAMEMAPS_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UNITTYPES_H_
#include "unittypes.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif

#ifndef _STATS_H_
#include "stats.h"
#endif

#ifndef _LEVEL_H_
#include "level.h"
#endif

#ifndef __LEVELAREAS_H_
#include "LevelAreas.h"
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct ITEM_SPEC;
struct MSG_SCMD_AVAILABLE_QUESTS;
struct MSG_SCMD_QUEST_TASK_STATUS;
namespace MYTHOS_MAPS
{
//STATIC FUNCTIONS and Defines

//----------------------------------------------------------------------------
// enums
//----------------------------------------------------------------------------
const enum EMAP_MESSSAGES
{
	KMAP_MESSAGE_MAP_LEARNED,
	KMAP_MESSAGE_COUNT
};

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

//returns if it is a TB Map
inline BOOL IsMAP( const UNIT *unit )
{
	return UnitIsA( unit, UNITTYPE_MAP );
}
//returns if it is a Map Spawner
inline BOOL IsMapKnown( const UNIT *unit )
{
	return UnitIsA( unit, UNITTYPE_MAP_KNOWN );
}
//returns if it's a Map Conditional
inline BOOL IsMapSpawner( const UNIT *unit )
{
	return UnitIsA( unit, UNITTYPE_MAP_SPAWNER );
}

float GetMapPrice( UNIT *unit, float defaultValue );

//returns the idea of the level area
inline int GetLevelAreaID( UNIT *unit )
{
	int nLevelArea = UnitGetStat( unit, STATS_MAP_AREA_REF );
	int nLevelAreaRandom = UnitGetStat( unit, STATS_MAP_AREA_REF_OFFSET );
	ASSERTX_RETINVALID( nLevelArea != INVALID_ID && nLevelAreaRandom != INVALID_ID, "Map doesn't specify level area" );
	return MYTHOS_LEVELAREAS::LevelAreaBuildLevelAreaID( nLevelArea, nLevelAreaRandom );
}

//Returns true if the player has a map with the area....
BOOL PlayerHasMapForAreaByAreaID( UNIT *pPlayer,
							      int nLevelAreaID );

UNIT * GetMapForAreaByAreaID( UNIT *pPlayer,
									 int nLevelAreaID );

//This returns the Maps UNIT ID of a given area
int GetUnitIDOfMapWithLevelAreaID( UNIT *pPlayer,
								   int nLevelAreaID );

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------


} //end namespace
#endif
