//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_GAMEMAPSSERVER_H_
#define __S_GAMEMAPSSERVER_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
enum EMAP_ATLAS_MESSAGE
{
	MAP_ATLAS_MSG_INVALID,
	MAP_ATLAS_MSG_SUCCESS,
	MAP_ATLAS_MSG_NOROOM,
	MAP_ATLAS_MSG_ALREADYKNOWN,
	MAP_ATLAS_MSG_MAPREMOVED,
	MAP_ATLAS_MSG_COUNT
};

#ifndef __S_GAMEMAPS_H_
#include "GameMaps.h"
#endif

#ifndef __GLOBALINDEX_H_
#include "globalindex.h"
#endif


namespace MYTHOS_MAPS
{
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------




//STATIC FUNCTIONS and Defines

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
					
inline void SetMapLevelAreaID( UNIT *pMapItem,
							   int nLevelAreaID )
{
	if( MYTHOS_MAPS::IsMAP( pMapItem ) )
	{
		int nRandomOffset = MYTHOS_LEVELAREAS::LevelAreaGetRandomOffsetNumber( nLevelAreaID );
		nLevelAreaID = MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaIDWithNoRandom( nLevelAreaID );
		UnitSetStat( pMapItem, STATS_MAP_AREA_REF, nLevelAreaID );
		UnitSetStat( pMapItem, STATS_MAP_AREA_REF_OFFSET, nRandomOffset );
	}
}



UNIT * s_CreateMap( UNIT *pPlayer, 
			        int nItemClassID,
				    int nLevelAreaID,	
					int nItemExperienceLevel,
					BOOL bGivePlayer,
					BOOL bPlaceInWorld );


inline UNIT * s_CreateMap_Generic( UNIT *pPlayer,
								   int nLevelAreaID,								  
								   BOOL bGivePlayer,
								   BOOL bPlaceInWorld )
{
	return s_CreateMap( pPlayer,
					    GlobalIndexGet( GI_ITEM_QUEST_MAP_TUGBOAT ),
					    nLevelAreaID,
						-1,
					    bGivePlayer,
						bPlaceInWorld ); 
		       
}

//Creates a location bypassing the map items....
int s_CreateMapLocations_FromAreaIDArray( UNIT *pPlayer,
										 const int *nAreaIDArray,
										 int nArraySize,
										 BOOL bPermanent,
										 BOOL bShowUIMap,
										 UNIT **pMapArrayBack = NULL );

//Creates a bunch of maps from an array and passes back the array if not NULL
int s_CreateGenericMaps_FromAreaIDArray( UNIT *pPlayer,
										 const int *nAreaIDArray,
										 int nArraySize,										 
										 BOOL bGivePlayer,
										 BOOL bPlaceInWorld,
										 UNIT **pMapArrayBack = NULL );

int s_CreateMapEntry_FromAreaIDArray( UNIT *pPlayer,
									 const int *nAreaIDArray,
									 int nArraySize,
									 UNIT **pMapArrayBack = NULL  );

UNIT * s_CreateMapLocation( UNIT *pPlayer,
						  int nLevelAreaID,
						  BOOL bPermanent,
						  BOOL bShowUIMap);

BOOL s_ConvertMapSpawnerToMap( UNIT *pPlayer,
							   UNIT *pMapSpawner );

void s_SetSeedOnMapKnown( UNIT *pMap );
						  
//this actually sets up the map when a generic map is created
void s_PopulateMapSpawner( GAME *pGame,						   
						   UNIT *pMap,
						   UNIT *pMapOwner,
						   int nMinGroupSize,
						   int nOverRideWithAreaID = INVALID_ID);

//anytime a map is learned this needs to be called
void s_MapWasLearnedByPlayer( UNIT *pPlayer, 
							  UNIT *pMap );

//whenever a map has been removed from a player
void s_MapHasBeenRemoved( UNIT *pPlayer );


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

} //end namespace
#endif
