#include "StdAfx.h"
#include "LevelAreasAndMapScriptsTB.h"
#include "script_priv.h"
#include "GameMaps.h"
#include "s_GameMapsServer.h"
#include "LevelAreas.h"
#include "units.h"
#include "uix.h"
#include "skills.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCreateMapFromMapSpawner(
	SCRIPT_CONTEXT * pContext )
{
	ASSERTX_RETFALSE( pContext, "No pContext" );
	UNIT *pPlayer = pContext->unit;
	UNIT *pMap = SkillGetAnyTarget( pPlayer, pContext->nSkill, NULL, FALSE );
	ASSERTX_RETFALSE( pMap, "No pMap" );
	ASSERTX_RETFALSE( pPlayer, "No pPlayer" );
	ASSERTX_RETFALSE( MYTHOS_MAPS::IsMAP( pMap ), "pMap is not of UNITYPE MAP" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "pPlayer is a Player" );
#if !ISVERSION(SERVER_VERSION)
	if( IS_CLIENT( pContext->game ) )
	{
		UI_TB_HideModalDialogs();
		//TODO: God I hate the UI system. So how can I just show the map?????
		//UIShowWorldMapScreen( componet, param, param )
		return 1;
	}	
#endif
	return MYTHOS_MAPS::s_ConvertMapSpawnerToMap( pPlayer, pMap );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRandomizeMapSpawnerEpic(
	SCRIPT_CONTEXT * pContext )
{
	ASSERTX_RETFALSE( pContext, "No pContext" );
	UNIT *pMap = pContext->unit;	
	ASSERTX_RETFALSE( pMap, "No pMap" );	
	ASSERTX_RETFALSE( MYTHOS_MAPS::IsMapSpawner( pMap ), "pMap is not of UNITYPE MAP SPAWNER" );	
	MYTHOS_MAPS::s_PopulateMapSpawner( pContext->game, pMap, pContext->object, 3 );
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMRandomizeMapSpawner(
	SCRIPT_CONTEXT * pContext )
{
	ASSERTX_RETFALSE( pContext, "No pContext" );
	UNIT *pMap = pContext->unit;	
	ASSERTX_RETFALSE( pMap, "No pMap" );	
	ASSERTX_RETFALSE( MYTHOS_MAPS::IsMapSpawner( pMap ), "pMap is not of UNITYPE MAP SPAWNER" );	
	MYTHOS_MAPS::s_PopulateMapSpawner( pContext->game, pMap, pContext->object, 0 );
	return 1;
}


int VMSetMapSpawnerByLevelAreaID( 
	 SCRIPT_CONTEXT * pContext,
	 int nLevelAreaID )
{
	ASSERTX_RETFALSE( pContext, "No pContext" );
	UNIT *pMap = pContext->unit;	
	ASSERTX_RETFALSE( pMap, "No pMap" );	
	ASSERTX_RETFALSE( MYTHOS_MAPS::IsMapSpawner( pMap ), "pMap is not of UNITYPE MAP SPAWNER" );
	MYTHOS_MAPS::s_PopulateMapSpawner( pContext->game, pMap, NULL, 0, nLevelAreaID );
	return 1;
	
}

int VMCreateRandomDungeonSeed( SCRIPT_CONTEXT * pContext,
							   int nZone,
							   int nLevelType,
							   int nMinLevel,
							   int nMaxLevel,
							   int nEpic )
{

	ASSERTX_RETFALSE( pContext, "No pContext" );
	if( IS_CLIENT( pContext->game ) )
	{
		return FALSE;
	}
	UNIT *pItemToAssignSeed = pContext->unit;	
	ASSERTX_RETFALSE( pItemToAssignSeed, "No Item" );	
	BOOL bEpic = ( nEpic > 0 )?TRUE:FALSE;
	int nLevelAreaID = MYTHOS_LEVELAREAS::s_LevelAreaGetRandomLevelID( pContext->game, 
																	   bEpic, 
																	   nMinLevel, 
																	   nMaxLevel, 
																	   nZone, 
																	   nLevelType,
																	   TRUE );
	ASSERTX_RETFALSE( nLevelAreaID != INVALID_ID, "Unable to find level area with params." );
	MYTHOS_MAPS::SetMapLevelAreaID( pContext->unit, nLevelAreaID );	
	return 1;

}