#include "StdAfx.h"
#include "LevelAreaLinker.h"
#include "excel.h"
#include "level.h"
#include "units.h"
#include "warp.h"
#include "player.h"

//Returns the level ID that the player needs to travel in to get from point A to point B
int MYTHOS_LEVELAREAS::GetLevelAreaID_First_BetweenLevelAreas( int nLevelAreaAt, int nLevelAreaGoingTo )
{
	int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );
	//first we must find the first level area that links to area's together.
	if( nLevelAreaGoingTo != INVALID_ID )
	{
			for( int t = 0; t < nCount; t++ )
			{
				const LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
				if( linker &&			
					( linker->m_LevelAreaIDLinkA == nLevelAreaAt ||
					  linker->m_LevelAreaIDLinkA == nLevelAreaGoingTo ) &&
					( linker->m_LevelAreaIDLinkB == nLevelAreaAt ||
					  linker->m_LevelAreaIDLinkB == nLevelAreaGoingTo ) )
				{
					//ok we have found at least one level area that is a link in the road to the two destination points
					//Now we need to look for if any of the next or previous level ID's are equal to the zone I'm at.
					if( linker->m_LevelAreaIDPrevious == nLevelAreaAt ||
						linker->m_LevelAreaIDNext == nLevelAreaAt )
					{
						return ( linker->m_LevelAreaID != INVALID_ID )?linker->m_LevelAreaID:nLevelAreaAt; //this is the level area I want to go to.
					}
				}
			}
	}
	return nLevelAreaAt;
}

//nLevelAreaGoingTo is the first zone of the level area between two hubs
BOOL MYTHOS_LEVELAREAS::ShouldStartAtLinkA( int nLevelAreaAt, int nLevelAreaGoingTo )
{
int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );
	//first we must find the first level area that links to area's together.
	if( nLevelAreaGoingTo != INVALID_ID )
	{
			for( int t = 0; t < nCount; t++ )
			{
				const LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
				if( linker &&	
					linker->m_LevelAreaID == nLevelAreaGoingTo )
				{
					//basicaly finding the level area we are going to. Then seeing if it's link A or link B.
					return ( linker->m_LevelAreaIDLinkA == nLevelAreaAt )?TRUE:FALSE;
				}
			}
	}
	return TRUE;
}

int MYTHOS_LEVELAREAS::GetLevelAreaID_NextAndPrev_BetweenLevelAreas( int nLevelAreaAt, ELevelAreaLinker nLinkType, int nDepth, BOOL bCheckLinks )
{
	int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );
	for( int t = 0; t < nCount; t++ )
	{
		const LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
		if( linker &&
			linker->m_LevelAreaID == nLevelAreaAt )
		{
			switch( nLinkType )
			{
			case KLEVELAREA_LINK_NEXT:
				if( bCheckLinks && linker->m_LevelAreaIDLinkB != INVALID_ID )
				{
					return linker->m_LevelAreaIDLinkB;
				}
				return ( linker->m_LevelAreaIDNext != INVALID_ID )?linker->m_LevelAreaIDNext:nLevelAreaAt;
			case KLEVELAREA_LINK_PREVIOUS:
				if( bCheckLinks && linker->m_LevelAreaIDLinkA != INVALID_ID )
				{
					return linker->m_LevelAreaIDLinkA;
				}
				return ( linker->m_LevelAreaIDPrevious != INVALID_ID)?linker->m_LevelAreaIDPrevious:nLevelAreaAt;
			}
		}
	}

	if( nLinkType == KLEVELAREA_LINK_PREVIOUS &&		
		nDepth <= 0 )
	{
		int nNewLevelAreaID = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaAt, NULL );
		if( nNewLevelAreaID != INVALID_ID )
			return nNewLevelAreaID;
	}

	if( nLinkType == KLEVELAREA_LINK_NEXT &&
		nDepth == MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelAreaAt ) - 1 )
	{
		int nNewLevelAreaID =  MYTHOS_LEVELAREAS::LevelAreaGetLevelLink( nLevelAreaAt );
		if( nNewLevelAreaID != INVALID_ID )
			return nNewLevelAreaID;
	}

	return nLevelAreaAt;	
}

BOOL MYTHOS_LEVELAREAS::FillOutLevelTypeForWarpObject( const LEVEL *pLevel, UNIT *pPlayer, LEVEL_TYPE *levelType, ELevelAreaLinker nLinkType  )
{
	ASSERTX_RETFALSE( levelType && pLevel && pPlayer, "one or more of the params for linking warp object were NULL" );	
	int nLevelArea = LevelGetLevelAreaID( pLevel );
	int nNewDepth = LevelGetDepth( pLevel ) + ( ( nLinkType == KLEVELAREA_LINK_NEXT)?1:-1);
	int nMaxDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelArea );
	//oh the easy part
	if(  nNewDepth >= 0 && nNewDepth <= nMaxDepth - 1 )
	{
		levelType->nLevelArea = nLevelArea;
		levelType->nLevelDepth = nNewDepth;
		levelType->bPVPWorld = PlayerIsInPVPWorld( pPlayer );
		levelType->nLevelDef = -1;
		return TRUE;		
	}
	int nNewLevelAreaID = GetLevelAreaID_NextAndPrev_BetweenLevelAreas( nLevelArea, nLinkType, LevelGetDepth( pLevel ) );
	ASSERTX_RETFALSE( nNewLevelAreaID != INVALID_ID, "Unable to find next level specified in level area linker" );
	levelType->nLevelArea = nNewLevelAreaID;
	if( nNewLevelAreaID == nLevelArea )
	{
		levelType->nLevelDepth = nNewDepth;
	}
	else
	{
		levelType->nLevelDepth = 0; //going someplace new so we need to start on the first level.
	}
	
	levelType->bPVPWorld = PlayerIsInPVPWorld( pPlayer );
	levelType->nLevelDef = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, nNewLevelAreaID, levelType->nLevelDepth );
	return TRUE;	
}

int MYTHOS_LEVELAREAS::GetLevelAreaIDByStairs( UNIT *pStairs )
{
	ASSERT_RETINVALID( pStairs );
	LEVEL *pLevel = UnitGetLevel( pStairs );
	ASSERT_RETINVALID( pLevel );
	int nDepth = LevelGetDepth( pLevel );
	int nLevelAreaID = LevelGetLevelAreaID( pLevel );
	int nMaxDepth = LevelGetLevelAreaOverrides( pLevel )->nDungeonDepth - 1;
	if( nDepth == 0 &&
		( UnitIsA( pStairs, UNITTYPE_STAIRS_UP ) ||
		  UnitIsA( pStairs, UNITTYPE_WARP_PREV_LEVEL ) ) )
	{
		int nLvlAreaID = GetLevelAreaID_NextAndPrev_BetweenLevelAreas( nLevelAreaID, KLEVELAREA_LINK_PREVIOUS, nDepth );
		if( nLvlAreaID != INVALID_ID )
			nLevelAreaID = nLvlAreaID;
	}
	else if( nDepth == nMaxDepth &&
		( UnitIsA( pStairs, UNITTYPE_STAIRS_DOWN ) ||
		UnitIsA( pStairs, UNITTYPE_WARP_NEXT_LEVEL ) ) )
	{
		int nLvlAreaID = GetLevelAreaID_NextAndPrev_BetweenLevelAreas( nLevelAreaID, KLEVELAREA_LINK_NEXT, nDepth );
		if( nLvlAreaID != INVALID_ID )
			nLevelAreaID = nLvlAreaID;

	}
	return nLevelAreaID;
}

BOOL MYTHOS_LEVELAREAS::IsLevelAreaAHub( int nLevelAreaID )
{
	
	//first we must find the first level area that links to area's together.
	if( nLevelAreaID != INVALID_ID )
	{
		int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );
		for( int t = 0; t < nCount; t++ )
		{
			const LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
			if( linker &&
				linker->m_LevelAreaID == nLevelAreaID )
			{
				return linker->m_CanTravelFrom;
			}
		}
	}
	return FALSE;
}




int MYTHOS_LEVELAREAS::GetLevelAreaLinkerRadius( int nLevelArea )
{
	const LEVELAREA_LINK *linker = GetLevelAreaLinkByAreaID( nLevelArea );
	ASSERT_RETVAL( linker, 200 );
	return linker->m_HubRadius;
}

//////////////////////////////////////////////////////////////////////////////////////
//This function takes all the logic to say if the player can travel to a level area
//////////////////////////////////////////////////////////////////////////////////////
BOOL MYTHOS_LEVELAREAS::PlayerCanTravelToLevelArea( UNIT *pPlayer, int nLevelAreaGoingTo )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETFALSE( nLevelAreaGoingTo != INVALID_ID );

	int nLevelAreaHubConnect = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaGoingTo, NULL );
	int nLevelAreaZoneAt = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelAreaHubConnect, NULL );
	BOOL bTravelingToHub = ( nLevelAreaHubConnect == nLevelAreaZoneAt );


	int nPlayerAtLevelArea = LevelGetLevelAreaID( UnitGetLevel( pPlayer ) );
	int nPlayerZoneAt = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nPlayerAtLevelArea, NULL );
	int nPlayerHubAt = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nPlayerAtLevelArea, NULL );
	BOOL bPlayerAtHub = ( nPlayerHubAt == nPlayerAtLevelArea );
	REF( bPlayerAtHub );

	if( nPlayerAtLevelArea == nLevelAreaGoingTo )
		return TRUE;

	//only certain types of levels allow for overland travel.
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if( pLevel &&
		pLevel->m_pLevelDefinition->bAllowOverworldTravel == FALSE )
	{
		return FALSE;
	}
	//Taking out for now. But we might want to have this...
	//if( bPlayerAtHub == FALSE )
	//	return FALSE; 

	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nPlayerAtLevelArea );
	for( int i = 0; i < pLevelArea->nNumTransporterAreas; i++ )
	{
		if( pLevelArea->nTransporterAreas[i] == nLevelAreaGoingTo )
		{
			const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelWarpArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( pLevelArea->nTransporterAreas[i] );
			if( UnitGetExperienceLevel( pPlayer ) >= pLevelWarpArea->nAreaWarpLevelRequirement )
			{
				return TRUE;
			}
		}
	}

	//if we aren't at the same zone we can't travel
	if( nPlayerZoneAt != nLevelAreaZoneAt )
		return FALSE;

	//if we are at the same hub I can go there...
	if( nLevelAreaHubConnect == nPlayerAtLevelArea )
		return TRUE;

	BOOL bPlayerHasBeenAtHub = (UnitGetStat( pPlayer, STATS_HUBS_VISITED, nLevelAreaHubConnect ) == 1 )? TRUE: FALSE;
	BOOL bLevelAreaGoingToIsHub = (nLevelAreaHubConnect == nLevelAreaGoingTo )?TRUE:FALSE;

	//if we are at a level area we have to go back to a hub to go to another hub
	if( bLevelAreaGoingToIsHub == FALSE &&
		bPlayerAtHub == FALSE )
		return FALSE;

	//I have to at least have gone to the hub to instant travel
	if( bPlayerHasBeenAtHub == TRUE &&
		( bPlayerAtHub ||
		 (nLevelAreaHubConnect == nPlayerHubAt )))
		return TRUE;

	//if it's a hub you can always travel to it as long as your at a hub
	if( bLevelAreaGoingToIsHub &&
		( bPlayerAtHub ||
		  (nLevelAreaGoingTo == nPlayerHubAt) ))
		return TRUE;

	//not sure if this is the best method,but for now if your at a safe zone you can travel to other hubs( safe zones )...
	if( pLevel->m_pLevelDefinition->bSafe &&
		bTravelingToHub )
		return TRUE;

	return FALSE;

}

//////////////////////////////////////////////////////////////////////////////////////
//This function determines if the area has a warpable section in it
//////////////////////////////////////////////////////////////////////////////////////
int MYTHOS_LEVELAREAS::CanWarpToSection( UNIT *pPlayer, int nLevelAreaGoingTo )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	ASSERT_RETFALSE( nLevelAreaGoingTo != INVALID_ID );


	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaGoingTo );

	if( pLevelArea )
	{
		return pLevelArea->nCanWarpToSection;
	}
	return 0;

}
void MYTHOS_LEVELAREAS::SetPlayerVisitedHub( UNIT *pPlayer, int nLevelAreaHub )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( nLevelAreaHub != INVALID_ID );
	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = GetLevelAreaLinkByAreaID( nLevelAreaHub );
	if( linker == NULL )
		return;
	//is a hub
	if( linker->m_CanTravelFrom )
	{
		UnitSetStat( pPlayer, STATS_HUBS_VISITED, nLevelAreaHub, 1 );
	}
}


BOOL MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UNIT *pPlayer, int nLevelAreaHub )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nLevelAreaHub != INVALID_ID );
	return UnitGetStat( pPlayer, STATS_HUBS_VISITED, nLevelAreaHub );
}


BOOL MYTHOS_LEVELAREAS::HasPlayerVisitedAConnectedHub( UNIT *pPlayer, int nLevelArea )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nLevelArea != INVALID_ID );
	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *link = GetLevelAreaLinkByAreaID( nLevelArea );
	if( link == NULL )
	{
		int LevelAreaHubID = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelArea, NULL );
		if( LevelAreaHubID == INVALID_ID )
			return FALSE;
		return HasPlayerVisitedHub( pPlayer, LevelAreaHubID );
	}
	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *visitA = GetLevelAreaLinkByAreaID( link->m_LevelAreaIDLinkA );
	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *visitB = GetLevelAreaLinkByAreaID( link->m_LevelAreaIDLinkB );
	BOOL visitedA = FALSE;
	BOOL visitedB = FALSE;
	if( visitA &&
		visitA->m_CanTravelFrom )
	{
		visitedA = HasPlayerVisitedHub( pPlayer,link->m_LevelAreaIDLinkA );
	}
	if( visitB &&
		visitB->m_CanTravelFrom )
	{
		visitedB = HasPlayerVisitedHub( pPlayer,link->m_LevelAreaIDLinkB );
	}	
	return visitedA | visitedB;
}


void MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( WARP_SPEC &tSpec, UNIT *pPlayer, BOOL allowRoadSkip )
{
	if( pPlayer )
	{
		
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		if( pLevel )
		{
			tSpec.nPVPWorld = ( LevelGetPVPWorld( pLevel ) ? 1 : 0 );
			int nTmpLevelArea( tSpec.nLevelAreaDest );
			int nLevelAreaAt = LevelGetLevelAreaID( pLevel );			
			if( allowRoadSkip )
			{
				allowRoadSkip = MYTHOS_LEVELAREAS::IsLevelAreaAHub( tSpec.nLevelAreaDest ) & ((MYTHOS_LEVELAREAS::HasPlayerVisitedHub( pPlayer, tSpec.nLevelAreaDest))?TRUE:FALSE);
				
			}
			//when a player has visited a hub they can skip the road if they want
			if( allowRoadSkip == FALSE )
			{
				tSpec.nLevelAreaDest = MYTHOS_LEVELAREAS::GetLevelAreaID_First_BetweenLevelAreas( nLevelAreaAt, tSpec.nLevelAreaDest );
				if( tSpec.nLevelAreaDest == nLevelAreaAt )
				{
					tSpec.nLevelAreaDest = nTmpLevelArea;
				}
				else if( MYTHOS_LEVELAREAS::ShouldStartAtLinkA( nLevelAreaAt, tSpec.nLevelAreaDest ) == FALSE )
				{
					tSpec.nLevelDepthDest = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( tSpec.nLevelAreaDest ) - 1; //set the last level....
					SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_FROM_LEVEL_PORTAL ); //start at the end of the dungeon.....
				}
			}

			//if the level area is an area between hubs( on the road ) we might need to start at the end of the level....
			const LEVELAREA_LINK *linkerLevel = GetLevelAreaLinkByAreaID( tSpec.nLevelAreaDest );
			if( linkerLevel )
			{
				if( ShouldStartAtLinkA( nLevelAreaAt, tSpec.nLevelAreaDest ) == FALSE )
				{
					tSpec.nLevelDepthDest = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( tSpec.nLevelAreaDest ) - 1;
					SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_FROM_LEVEL_PORTAL ); //start at the end of the dungeon.....
				}
			}
		}
	}
	
}