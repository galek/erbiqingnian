#include "stdafx.h"
#include "s_GameMapsServer.h"
#include "units.h"
#include "items.h"
#include "player.h"
#include "c_ui.h"
#include "game.h"
#include "LevelAreas.h"
#include "picker.h"
#include "s_LevelAreasServer.h"
#include "s_message.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSendMapMessage( UNIT *pPlayer, 
							 UNIT *pMapUnit,
							 MYTHOS_MAPS::EMAP_MESSSAGES nMessage )
{
	ASSERT_RETFALSE( pPlayer );	
	
	
	//send message to client that a map has a message for the player
	MSG_SCMD_LEVELAREA_MAP_MESSAGE message;
	if( pMapUnit != NULL )
	{
		message.nMapID = UnitGetId( pMapUnit );
	}
	else
	{
		message.nMapID = INVALID_ID;
	}
	message.nMessage = (int)nMessage;
	// send it
	s_SendMessage( UnitGetGame(pPlayer), UnitGetClientId(pPlayer), SCMD_LEVELAREA_MAP_MESSAGE, &message );
	return TRUE;
}

UNIT * MYTHOS_MAPS::s_CreateMap( UNIT *pPlayer, 
								int nItemClassID,
								int nLevelAreaID,	
								int nItemExperienceLevel,
								BOOL bGivePlayer,
								BOOL bPlaceInWorld )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERTX_RETNULL( nItemClassID != INVALID_ID, "Trying to spawn an item but the class was invalide.(CreateMap)" );		
	ASSERTX_RETNULL( pPlayer, "Trying to spawn an item to player but the player was NULL.(CreateMap)" );
	const UNIT_DATA *unitData = ItemGetData( nItemClassID );
	ASSERTX_RETNULL( unitData, "Unit data for item class was NULL.(CreateMap)" );
	ITEM_SPEC tSpec;
	if( bGivePlayer )
	{
		if( MYTHOS_MAPS::PlayerHasMapForAreaByAreaID( pPlayer, nLevelAreaID ) )
		{
			s_PlayerShowWorldMap( pPlayer, MAP_ATLAS_MSG_ALREADYKNOWN, nLevelAreaID );
			return NULL;
		}
		SETBIT(tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT);
		tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
		SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		SETBIT( tSpec.dwFlags, ISF_NO_FLIPPY_BIT );
	}
	if( bPlaceInWorld )
	{
		SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
	}	
	tSpec.nItemClass = nItemClassID;	
	tSpec.pSpawner = pPlayer;
	tSpec.nLevel = nItemExperienceLevel;
	UNIT * pMapItem = s_ItemSpawn( UnitGetGame( pPlayer ), tSpec, NULL);			
	//if( nItemExperienceLevel > 0 )
	//	UnitSetExperienceLevel(pMapItem, nItemExperienceLevel);
	if( pMapItem &&
		( bPlaceInWorld || bGivePlayer ) )
	{
		
		SetMapLevelAreaID( pMapItem, nLevelAreaID );
		UnitWarp( 
			pMapItem,
			UnitGetRoom( pPlayer ),
			UnitGetPosition( pPlayer ),
			UnitGetFaceDirection( pPlayer, FALSE ),
			UnitGetVUpDirection( pPlayer ),
			0,
			FALSE);

		// pick up item
		if( bGivePlayer )
		{
			UNIT* pPickedUpItem = s_ItemPickup( pPlayer, pMapItem );
			if( pPickedUpItem && !UnitIsContainedBy(pPickedUpItem, pPlayer) )
			{
				UnitFree( pPickedUpItem, UFF_SEND_TO_CLIENTS );	//free the item
				s_PlayerShowWorldMap( pPlayer, MAP_ATLAS_MSG_NOROOM, nLevelAreaID );
				return NULL;
			}
			//send a message the player picked the map up
			s_MapWasLearnedByPlayer( pPlayer, pPickedUpItem );
		}
	}
	return pMapItem;
#else
	REF( pPlayer );
	REF( nItemClassID );
	REF( nLevelAreaID );
	REF( nItemExperienceLevel );
	REF( bGivePlayer );
	REF( bPlaceInWorld );
	return NULL;
#endif
}

//this basically just creates an array of maps for the player
int MYTHOS_MAPS::s_CreateGenericMaps_FromAreaIDArray( UNIT *pPlayer,
													 const int *nAreaIDArray,
													 int nArraySize,													 
													 BOOL bGivePlayer,
													 BOOL bPlaceInWorld,
													 UNIT **pMapArrayBack )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int count( 0 );
	for( int t = 0; t < nArraySize; t++ )
	{
		if( nAreaIDArray[ t ] != INVALID_ID  ) //Make sure we don't already ahve it
		{
			UNIT *pMap = MYTHOS_MAPS::s_CreateMap_Generic( pPlayer, nAreaIDArray[ t ], bGivePlayer, bPlaceInWorld );
			if( pMapArrayBack != NULL )
				pMapArrayBack[ count ] = pMap;
			count++;
		}
	}
	return count;
#else
	REF( pPlayer );
	REF( nAreaIDArray );
	REF( nArraySize );
	REF( bGivePlayer );
	REF( bPlaceInWorld );
	REF( pMapArrayBack );
	return 0;
#endif
}


//this basically just creates an array of maps for the player
int MYTHOS_MAPS::s_CreateMapEntry_FromAreaIDArray( UNIT *pPlayer,
													 const int *nAreaIDArray,
													 int nArraySize,
													 UNIT **pMapArrayBack  )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int count( 0 );
	for( int t = 0; t < nArraySize; t++ )
	{
		if( nAreaIDArray[ t ] != INVALID_ID  ) //Make sure we don't already ahve it
		{
			UNIT *pMap = MYTHOS_MAPS::s_CreateMap_Generic( pPlayer, nAreaIDArray[ t ], TRUE, TRUE );
			if( pMapArrayBack != NULL )
				pMapArrayBack[ count ] = pMap;
			count++;
		}
	}
	return count;
#else
	REF( pPlayer );
	REF( nAreaIDArray );
	REF( nArraySize );
	REF( pMapArrayBack );
	return 0;
#endif
}

//Creates a location bypassing the map items....
int MYTHOS_MAPS::s_CreateMapLocations_FromAreaIDArray( UNIT *pPlayer,
										 const int *nAreaIDArray,
										 int nArraySize,
										 BOOL bPermanent,
										 BOOL bShowUIMap,
										 UNIT **pMapArrayBack )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)	
	int count(0);
	for( int t = 0; t < nArraySize; t++ )
	{
		if( nAreaIDArray[ t ] != INVALID_ID  ) //Make sure we don't already ahve it
		{
			UNIT *mapLocation = MYTHOS_MAPS::s_CreateMapLocation( pPlayer, nAreaIDArray[ t ], bPermanent, bShowUIMap );
			if( mapLocation )
			{
				pMapArrayBack[ count ] = mapLocation;
				count++;
			}
		}
	}
	return count;
#else
	REF( pPlayer );
	REF( nAreaIDArray );
	REF( nArraySize );
	REF( bPermanent );
	REF( bShowUIMap );
	REF( pMapArrayBack );
	return 0;
#endif
	
}

UNIT *  MYTHOS_MAPS::s_CreateMapLocation( UNIT *pPlayer,
										  int nLevelAreaID,
										  BOOL bPermanent,
										  BOOL bShowUIMap)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETFALSE( pPlayer, "No player" );	
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "pPlayer is not a Player" );	
	ASSERTX_RETFALSE( nLevelAreaID != INVALID_ID, "INVALID level area" );
	int nIndex = GlobalIndexGet( GI_ITEM_MAP_TUGBOAT );
	if( bPermanent )
	{
		nIndex = GlobalIndexGet( GI_ITEM_MAP_TUGBOAT_PERMANENT );
	}


	UNIT *pMap = s_CreateMap( pPlayer, 
							 nIndex, 
							 nLevelAreaID,
							 1,
							 TRUE,
							 FALSE );
	if( !pMap )
	{
		// either they already knew it, or they didn't have space
		return NULL;
	}
	
	// show the map atlas as soon as we do this.
	if( bShowUIMap )
	{
		s_PlayerShowWorldMap( pPlayer,  MAP_ATLAS_MSG_SUCCESS, nLevelAreaID );
	}
	return pMap;
#else
	REF( pPlayer );
	REF( nLevelAreaID );
	REF( bPermanent );
	REF( bShowUIMap );
	return NULL;
#endif
	
}


BOOL MYTHOS_MAPS::s_ConvertMapSpawnerToMap( UNIT *pPlayer,
										    UNIT *pMapSpawner )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETFALSE( pPlayer, "No player( s_ConvertMapSpawnerToMap )" );
	ASSERTX_RETFALSE( pMapSpawner, "No pMapSpawner( s_ConvertMapSpawnerToMap )" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "pPlayer is not a Player( s_ConvertMapSpawnerToMap )" );	
	ASSERTX_RETFALSE( MYTHOS_MAPS::IsMapSpawner( pMapSpawner ), "pMapSpawner is not a Spawner Map( s_ConvertMapSpawnerToMap )" );
	int nIndex = GlobalIndexGet( GI_ITEM_MAP_TUGBOAT );
	if( UnitIsA( pMapSpawner, UNITTYPE_QUEST_MAP_ITEM_SPAWNER ) )
	{
		nIndex = GlobalIndexGet( GI_ITEM_MAP_TUGBOAT_PERMANENT );
	}


	UNIT *pMap = s_CreateMap( pPlayer, 
							 nIndex, 
							 MYTHOS_MAPS::GetLevelAreaID( pMapSpawner ),
							 UnitGetExperienceLevel( pMapSpawner ),
							 TRUE,
							 FALSE );
	if( !pMap )
	{
		// either they already knew it, or they didn't have space
		return FALSE;
	}


	MYTHOS_MAPS::s_SetSeedOnMapKnown( pMap ); //set random seed for the map.
	// show the map atlas as soon as we do this.
	s_PlayerShowWorldMap( pPlayer,  MAP_ATLAS_MSG_SUCCESS, MYTHOS_MAPS::GetLevelAreaID( pMapSpawner ) );
	UnitFree( pMapSpawner, UFF_SEND_TO_CLIENTS);
	return TRUE;
#else
	REF( pPlayer );
	REF( pMapSpawner );
	return FALSE;
#endif
}

//this sets the seed on the map
void MYTHOS_MAPS::s_SetSeedOnMapKnown( UNIT *pMap )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pMap, "No Map passed in." );
	GAME *pGame = UnitGetGame( pMap );	
	ASSERTX_RETURN( pGame, "No game found in map." );
	UnitSetStat( pMap, STATS_LEVELAREA_SEED, RandGetNum( pGame->rand ) );
#else
	REF( pMap );
#endif
}

//this actually sets up the map when a generic map is created
void MYTHOS_MAPS::s_PopulateMapSpawner( GAME *pGame,									    
										UNIT *pMap,
										UNIT *pMapOwner,
										int nMinGroupSize,
										int nOverRideWithAreaID )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERTX_RETURN( pMap, "No pMap" );	
	ASSERTX_RETURN( pGame, "No Game" );
	ASSERTX_RETURN( MYTHOS_MAPS::IsMapSpawner( pMap ), "No Game" );
	int levelAreaID = INVALID_ID;
	int nLevelMonsterExperience( 0 );
	int nLevelRangeMin( 0 );
	int nLevelRangeMax( 0 );
	const static int MINRANGE_MERCHANT( -10 );
	const static int MAXRANGE_MERCHANT( 25 );
	const static int MINRANGE_MAP( -1 );
	const static int MAXRANGE_MAP( 5 );

	//for merchants
	if( pMapOwner != NULL )
	{
		if( UnitGetLevel( pMapOwner ) != NULL )
		{
			levelAreaID = LevelGetLevelAreaID( UnitGetLevel( pMapOwner ) );
			nLevelMonsterExperience = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( levelAreaID );
			nLevelRangeMin = nLevelMonsterExperience + MINRANGE_MERCHANT;			
			nLevelRangeMax = nLevelMonsterExperience + MAXRANGE_MERCHANT;
		}	
	}	
	//for when maps drop randomly
	if( levelAreaID == -1 &&		
		UnitGetLevel( pMap ) != NULL )
	{
		levelAreaID = LevelGetLevelAreaID( UnitGetLevel( pMap ) );
		nLevelMonsterExperience = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( levelAreaID );
		nLevelRangeMin = nLevelMonsterExperience + MINRANGE_MAP;			
		nLevelRangeMax = nLevelMonsterExperience + MAXRANGE_MAP;
	}
	//cap the min and max range respectivily
	nLevelRangeMin = max( 1, nLevelRangeMin );
	nLevelRangeMax = min( 50, nLevelRangeMax );  //I can't believe we don't have a const for this someplace.

	int nLevelZone( INVALID_ID );
	if( levelAreaID != INVALID_ID )
	{
		nLevelZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( levelAreaID, NULL );
	}
	if( nOverRideWithAreaID == INVALID_ID )
	{	
		
		nOverRideWithAreaID = MYTHOS_LEVELAREAS::s_LevelAreaGetRandomLevelID( pGame, 
																			  ((( nMinGroupSize > 0 )?TRUE:FALSE) || UnitIsA( pMap, UNITTYPE_MAP_ITEM_SPAWNER_EPIC)),
																			  nLevelRangeMin,
																			  nLevelRangeMax,
																			  nLevelZone );		
	}
	SetMapLevelAreaID( pMap, nOverRideWithAreaID );
	
#else
	REF( pGame );
	REF( pMap );
	REF( nOverRideWithAreaID );
#endif
}

//anytime a map is learned this needs to be called
void MYTHOS_MAPS::s_MapWasLearnedByPlayer( UNIT *pPlayer, 
										   UNIT *pMap )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pMap );
	sSendMapMessage( pPlayer,
					 pMap,
					 KMAP_MESSAGE_MAP_LEARNED );	
	
}


void MYTHOS_MAPS::s_MapHasBeenRemoved( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	
	sSendMapMessage( pPlayer,
					NULL,
					KMAP_MESSAGE_MAP_LEARNED );	
}