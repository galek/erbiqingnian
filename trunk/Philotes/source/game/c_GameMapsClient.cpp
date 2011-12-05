#include "stdafx.h"
#include "c_GameMapsClient.h"
#include "c_LevelAreasClient.h"
#include "stringtable.h"
#include "Quest.h"
#include "globalindex.h"
#include "stringreplacement.h"
#include "LevelAreaLinker.h"

BOOL MYTHOS_MAPS::c_MapHasPixelLocation( UNIT *pMap )
{
	if( UnitGetStatFloat( pMap, STATS_MAP_AREA_LOCATION_X ) > 0 && UnitGetStatFloat( pMap, STATS_MAP_AREA_LOCATION_Y ) > 0 )
		return TRUE;
	return MYTHOS_LEVELAREAS::LevelAreaHasHardCodedPosition( MYTHOS_MAPS::GetLevelAreaID( pMap ) );
	
}

void MYTHOS_MAPS::c_GetMapPixels( UNIT *pMap, float &x, float &y )
{
	if( !pMap ||
		!MYTHOS_MAPS::IsMAP( pMap ) )
	{
		return;		
	}
	x = UnitGetStatFloat( pMap, STATS_MAP_AREA_LOCATION_X );
	y = UnitGetStatFloat( pMap, STATS_MAP_AREA_LOCATION_Y );
	if( x <= 0 ||
		y <= 0 )
	{
		int nLevelArea = MYTHOS_MAPS::GetLevelAreaID( pMap );
		int nMapZone = MYTHOS_MAPS::c_GetMapZone( pMap );
		if( nLevelArea == INVALID_ID )
			return;
		MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( nLevelArea, nMapZone, x, y );
		UnitSetStatFloat( pMap, STATS_MAP_AREA_LOCATION_X, 0, x );
		UnitSetStatFloat( pMap, STATS_MAP_AREA_LOCATION_Y, 0, y );
	}
}

int MYTHOS_MAPS::c_GetMapZone( UNIT *pMap )
{
	if( !pMap ||
		!MYTHOS_MAPS::IsMAP( pMap ) )
	{
		return INVALID_ID;		
	}
	int nIndex = UnitGetStat( pMap, STATS_MAP_ZONE_LOCATION );
	if( nIndex == INVALID_ID )
	{
		int nLevelArea = MYTHOS_MAPS::GetLevelAreaID( pMap );
		if( nLevelArea == INVALID_ID )
			return INVALID_ID;
		nIndex = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelArea, NULL );
		UnitSetStat( pMap, STATS_MAP_ZONE_LOCATION, nIndex );		
	}
	return nIndex;
}



int MYTHOS_MAPS::c_GetFlavoredText( UNIT *pMapUnit )
{
	ASSERTX_RETINVALID( pMapUnit, "pMapUnit invalid in getting Map Flavored Text(c_GetFlavoredText)" );	
	ASSERTX_RETINVALID( MYTHOS_MAPS::IsMAP(pMapUnit), "Unit passed in not map(c_GetFlavoredText)" );		
	UNIT *pPlayer = UnitGetOwnerUnit( pMapUnit );
	ASSERTX_RETINVALID( pPlayer, "Unit passed in is not owned by a player(c_GetFlavoredText)" );
	if( MYTHOS_MAPS::IsMapSpawner( pMapUnit ) )
	{
		return GlobalStringGetStringIndex( ( sQuestIsLevelAreaNeededByQuestSystem( pPlayer, MYTHOS_MAPS::GetLevelAreaID( pMapUnit )) )?GS_QUEST_MAP_TEXT_CANNOT_DROP:GS_QUEST_MAP_TEXT_CAN_DROP );	
	}
	else
	{
		return GlobalStringGetStringIndex( ( sQuestIsLevelAreaNeededByQuestSystem( pPlayer, MYTHOS_MAPS::GetLevelAreaID( pMapUnit )) )?GS_QUEST_MAPENTRY_TEXT_PERMANENT:GS_QUEST_MAPENTRY_TEXT );	
	}
}


void MYTHOS_MAPS::c_FillFlavoredText( UNIT* unit,
									  WCHAR* szFlavor,
									  int len)
{	
	if( unit &&
		MYTHOS_MAPS::IsMAP( unit ) )
	{
		int nLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( unit ) ;		
		if( nLevelAreaID != INVALID_ID )
		{
			WCHAR insertString[ 1024 ];
			MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, -1, &insertString[0], 1024 );
			//const WCHAR *insertString = StringTableGetStringByIndex( MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID ) );
			PStrReplaceToken( szFlavor, len, StringReplacementTokensGet( SR_AREA_NAME ), insertString );
			ZeroMemory( insertString, sizeof( WCHAR ) * 1024 );
			if( MYTHOS_LEVELAREAS::IsLevelAreaAHub( nLevelAreaID ) == FALSE )
			{
				int nHubLevelAreaID = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaID, NULL );								
				if( nHubLevelAreaID != INVALID_ID )
				{
					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nHubLevelAreaID, 0, &insertString[0], 1024 );
				}
			}
			PStrReplaceToken( szFlavor, len, StringReplacementTokensGet( SR_HUB_NAME ), insertString );

			if( MYTHOS_LEVELAREAS::LevelAreaIsHostile( nLevelAreaID ) )
			{
				WCHAR uszText[ 32 ];
				int nMin = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID );
				int nMax = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID );
				PStrPrintf( uszText, 32, L"%d-%d", nMin, nMax );

				PStrReplaceToken( szFlavor, len, StringReplacementTokensGet( SR_AREA_LEVEL ), uszText );
			}
			else
			{
				PStrReplaceToken( szFlavor, len, StringReplacementTokensGet( SR_AREA_LEVEL ), L"" );
			}

			/*WCHAR uszText[ 32 ];
			//TODO: THis needs to use the area index to get the value of the party size
			int minPartySize = MYTHOS_LEVELAREAS::LevelAreaGetMinPartySizeRecommended( nLevelAreaID );
			PStrPrintf( uszText, 32, L"%d", ( minPartySize < 1 )?1:minPartySize );

			PStrReplaceToken( szFlavor, len, StringReplacementTokensGet( SR_GROUP_SIZE ), uszText );*/

		}
		
	}
}


void MYTHOS_MAPS::c_MapMessage( GAME *pGame, 				 
							    MSG_SCMD_LEVELAREA_MAP_MESSAGE *message )
{
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Attempting to handle map message by server" );
	UNIT *pPlayer = GameGetControlUnit( pGame );	
	ASSERTX_RETURN( pPlayer, "Got Quest Message for client before player is out of limbo(c_QuestHandleQuestTaskMessage)" );
	ASSERT_RETURN( message );

	UNIT *pMapUnit( NULL );
	if( message->nMapID != INVALID_ID )
	{
		pMapUnit = UnitGetById( pGame, message->nMapID );
	}

	switch( message->nMessage )
	{
	case KMAP_MESSAGE_MAP_LEARNED:
		c_LevelResetDungeonEntranceVisibility( UnitGetLevel( GameGetControlUnit( pGame ) ),
												GameGetControlUnit( pGame ) );
											  
		break;
	}
}