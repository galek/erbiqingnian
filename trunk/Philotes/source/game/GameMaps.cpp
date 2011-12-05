#include "stdafx.h"
#include "GameMaps.h"
#include "Inventory.h"
#include "LevelAreas.h"



UNIT * MYTHOS_MAPS::GetMapForAreaByAreaID( UNIT *pPlayer,
										   int nLevelAreaID )
{
	ASSERTX_RETNULL( pPlayer, "No pPlayer given" );
	ASSERTX_RETNULL( nLevelAreaID != INVALID_ID, " level area ID not given" );
	UNIT* pItem = NULL;	
	int location, x, y;
	while ( ( pItem = UnitInventoryIterate( pPlayer, pItem ) ) != NULL)
	{		
		UnitGetInventoryLocation( pItem, &location, &x, &y );
		if( MYTHOS_MAPS::IsMapKnown( pItem ) )
		{
			int mapLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( pItem ); 
			if( nLevelAreaID == mapLevelAreaID )
				return pItem;
		}
	}
	
	return NULL;	

}

BOOL MYTHOS_MAPS::PlayerHasMapForAreaByAreaID( UNIT *pPlayer,
											   int nLevelAreaID )
{
	return (MYTHOS_MAPS::GetMapForAreaByAreaID( pPlayer, nLevelAreaID ) != NULL );
}


int MYTHOS_MAPS::GetUnitIDOfMapWithLevelAreaID( UNIT *pPlayer,
													 int nLevelAreaID )
{
	ASSERTX_RETINVALID( pPlayer, "No pPlayer given" );
	ASSERTX_RETINVALID( ( nLevelAreaID != INVALID_ID ), "A level area ID and or pLevel Def not given" );
	UNIT* pItem = NULL;	
	int location, x, y;
	while ( ( pItem = UnitInventoryIterate( pPlayer, pItem ) ) != NULL)
	{		
		UnitGetInventoryLocation( pItem, &location, &x, &y );
		if( MYTHOS_MAPS::IsMapKnown( pItem ) )
		{
			if( nLevelAreaID != INVALID_ID )
			{
				if( nLevelAreaID == MYTHOS_MAPS::GetLevelAreaID( pItem ) )
					return UnitGetId( pItem );
			}
		}
	}
	
	return INVALID_ID;	
}

float MYTHOS_MAPS::GetMapPrice( UNIT *unit, float defaultValue )
{
	//this needs to be put inside of an excel table
	int nMin = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( MYTHOS_MAPS::GetLevelAreaID( unit ) );
	int nMax = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( MYTHOS_MAPS::GetLevelAreaID( unit ) );
	int nSpread = nMax - nMin;
	defaultValue += nMin * (25 + (nMin + nMin));
	if( nSpread > 0 )
	{
		defaultValue += (nSpread - 1) * ( 25 + (nMax + nMax) );
	}
	return defaultValue;
}