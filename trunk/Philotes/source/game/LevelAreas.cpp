#include "stdafx.h"
#include "LevelAreas.h"
#include "excel.h"
#include "GameMaps.h"
#include "ChatServer.h"
#include "player.h"
#include "inventory.h"
#include "level.h"
#include "stringtable.h"
#include "drlg.h"
#include "excel_private.h"
#include "picker.h"
#include "skills.h"
#include "script.h"
#include "dungeon.h"
#include "c_LevelAreasClient.h"
#include "LevelZones.h"
#include "LevelAreasKnownIterator.h"
#include "LevelAreaLinker.h"
#include "Quest.h"
#include "QuestProperties.h"

static int		sLevelAreasAlwaysVisible[ MYTHOS_LEVELAREAS::LVL_AREA_MAX_ALWAYS_VISIBLE ];
static int		sLevelAreasAlwaysVisibleCount( 0 );

inline int LevelAreaGetSectionByDepthInline( int nLevelAreaID, int nDepth )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETINVALID(pLevelArea, "Invalid area" );
	RAND Rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelAreaID );
	int nMaxDepth(0);
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_SECTIONS; t++ )
	{
		if( pLevelArea->m_Sections[t].nLevels[0] != INVALID_ID )
		{
			if( pLevelArea->m_Sections[t].nDepthRange[1] > 0 )
			{
				nMaxDepth += RandGetNum( Rand, pLevelArea->m_Sections[t].nDepthRange[0], pLevelArea->m_Sections[t].nDepthRange[1] );
			}
			else
			{
				nMaxDepth += MAX( 1, pLevelArea->m_Sections[t].nDepthRange[0] );
			}
			if( nDepth < nMaxDepth )
			{
				return t;
			}
		}
	}
	return 0;
}



inline int LevelAreaGetRandomMinMax( RAND &tLvlRand,
									  int nMin,
									  int nMax,
									  int nDefault )
{
	if( nMin != INVALID_ID &&
		nMax == INVALID_ID )
	{
		return nMin;
	}else if( nMin != INVALID_ID &&
			  nMax != INVALID_ID )
	{
		 return RandGetNum( tLvlRand, nMin, nMax );
	}
	return nDefault;
}

inline float LevelAreaGetRandomMinMaxFloat( RAND &tLvlRand,
											  float nFloatMin,
											  float nFloatMax,
											  float nDefault )
{
	if( nFloatMin > nFloatMax &&
		nFloatMin > 0 )
	{
		return nFloatMin;
	}else if( nFloatMin > 0 &&
			  nFloatMax > 0 )
	{
		 return (float)RandGetFloat( tLvlRand, nFloatMin, nFloatMax );//pLevelArea->nLevelSizeMultMinMax[0], pLevelArea->nLevelSizeMultMinMax[1] )/(float)100.0f;	
	}
	return nDefault;
}

inline float LevelAreaGetChampionMult( MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides )
{
	if( pOverRides->bIsLastLevel )
	{
		//we are on the last level
		if( pOverRides->pLevelArea->nChampionNotAllowedOnLastLevel )
		{
			return 0.0f;	//no champions allowed
		}
	}		
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_ARRAY; t++ )
	{
		if( pOverRides->pLevelArea->nChampionMultLevel[ t ] == pOverRides->nCurrentDungeonDepth ||
			(pOverRides->bIsLastLevel && pOverRides->pLevelArea->nChampionMultLevel[ t ] == -1 ) )
		{
			if( pOverRides->pLevelArea->nChampionMult[ t ] == -1 )
				continue;
			return (float)pOverRides->pLevelArea->nChampionMult[ t ]/100.0f;
		}
	}
	if( pOverRides->pLevelArea->nChampionMultGlobal > 0 )
	{
		return (float)pOverRides->pLevelArea->nChampionMultGlobal/100.0f;
	}
	return 1.0f;
}


inline int LevelAreaGetSpawnClass( RAND &tLvlRand,
								   MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides,
								   const LEVEL *pLevel )
{		
	//first we check the hard coded levels
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_ARRAY; t++ )
	{
		if( pOverRides->pLevelArea->nSpawnClassFloorsToSpawnIn[ t ] == pOverRides->nCurrentDungeonDepth )
		{
			ASSERTX_RETINVALID( pOverRides->pLevelArea->nSpanwClassFloorsSpawnClasses[ t ] != INVALID_ID, "Trying to select spawn class for a floor but the spawn class is not valid" );
			return pOverRides->pLevelArea->nSpanwClassFloorsSpawnClasses[ t ];
		}
	}
	
	int nSection = LevelAreaGetSectionByDepthInline( LevelGetLevelAreaID( pLevel ), LevelGetDepth( pLevel ) );
	int index = RandGetNum( tLvlRand, 0, pOverRides->pLevelArea->m_Sections[ nSection ].nSpawnClassCount - 1 );
	return  pOverRides->pLevelArea->m_Sections[ nSection ].nSpawnClassOverride[ index ];
}

inline int LevelAreaGetXPModForNormalMonsters( MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides )
{		
	//first we check the hard coded levels
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_ARRAY; t++ )
	{
		if( pOverRides->pLevelArea->nXPModLevel[ t ] == pOverRides->nCurrentDungeonDepth ||
			( pOverRides->pLevelArea->nXPModLevel[ t ] == -1 && pOverRides->bIsLastLevel ) ) //last level
		{
			ASSERTX_RETINVALID( pOverRides->pLevelArea->nXPModPerNormalMonsters[ t ] != INVALID_ID, "Trying to select spawn class for a floor but the spawn class is not valid" );
			if( pOverRides->pLevelArea->nXPModPerNormalMonsters[ t ] != 100 )
			{
				return pOverRides->pLevelArea->nXPModPerNormalMonsters[ t ];
			}
		}
	}


	return -1;
}



inline float LevelAreaGetMonsterDensity( RAND &tLvlRand,
										 MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides )
{
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_ARRAY; t++ )
	{
		if( pOverRides->pLevelArea->nMonsterDensityMin[t] < 0 ||
			pOverRides->pLevelArea->nMonsterDensityMax[t] < 0 )
			break;
		if( pOverRides->pLevelArea->nMonsterDensityLevels[ t ] == pOverRides->nCurrentDungeonDepth ||
			(pOverRides->bIsLastLevel && pOverRides->pLevelArea->nMonsterDensityLevels[t] < 0 ) )			 
		{
			return (float)LevelAreaGetRandomMinMaxFloat( tLvlRand, (float)pOverRides->pLevelArea->nMonsterDensityMin[t], (float)pOverRides->pLevelArea->nMonsterDensityMax[t],100 )/(float)100.0f;	
		}
	}
	return (float)LevelAreaGetRandomMinMaxFloat( tLvlRand, (float)pOverRides->pLevelArea->nMonsterDensityGlobal[0], (float)pOverRides->pLevelArea->nMonsterDensityGlobal[1],100 )/(float)100.0f;	
}

inline BOOL LevelAreaHasObjectCreatedIn( RAND &tLvlRand,
								  MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides )
{
	if( pOverRides->pLevelArea->nObjectsToSpawnCount <= 0 ) return FALSE;	
	if( pOverRides->pLevelArea->nChanceForObjectSpawn != INVALID_ID &&
		RandGetNum( tLvlRand, 0, pOverRides->pLevelArea->nObjectsToSpawnCount ) > (DWORD)pOverRides->pLevelArea->nChanceForObjectSpawn )
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MYTHOS_LEVELAREAS::LevelAreaGetStartNewGameLevelDefinition(
	void)
{
	int nLevelAreaDef = INVALID_LINK;
	int nNumLevelAreaDefs = ExcelGetNumRows( NULL, DATATABLE_LEVEL_AREAS );	

	// go through all level defs and find the first level
	for (int i = 0 ; i < nNumLevelAreaDefs; ++i)
	{
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelAreaDef = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( i );
		if (! pLevelAreaDef)
			continue;
		if (pLevelAreaDef->bStartArea )
		{
			nLevelAreaDef = i;
			break;
		} 			
	}

	ASSERTX( nLevelAreaDef != INVALID_LINK, "Unable to find first level area of game" );		
	return nLevelAreaDef;

}

int LevelAreaGetObjectToCreate( RAND &tLvlRand,
								MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides )
{
	
	if( pOverRides->pLevelArea->nObjectsToSpawnCount <= 0 ) return INVALID_ID;	
	if( !pOverRides->bIsLastLevel )
	{
		return INVALID_ID; //must be last level
	}
	
	if( !LevelAreaHasObjectCreatedIn( tLvlRand, pOverRides ) )
	{
		return INVALID_ID;
	}
	return pOverRides->pLevelArea->nObjectsToSpawn[RandGetNum( tLvlRand, 0, pOverRides->pLevelArea->nObjectsToSpawnCount - 1 )];
}

inline int LevelAreaGetLevelAreaDifficulty( int levelAreaID,
										    BOOL bHighRange )
{
	//this function is special because it's needed for the UI as well.
	//NOTE- this only occurs once on the server. MULTIPLE times on the client
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( levelAreaID );
	ASSERT_RETZERO( pLevelArea );
	if( pLevelArea->nDifficultyLevelAdditional[ 0 ] < 0 )
		return 0;	
	RAND tLvlRand;		
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &tLvlRand, levelAreaID );
	return LevelAreaGetRandomMinMax( tLvlRand, 0, ((!bHighRange)?pLevelArea->nDifficultyLevelAdditional[0]:pLevelArea->nDifficultyLevelAdditional[1]), 1 );
}


int MYTHOS_LEVELAREAS::LevelAreaGetMinPartySizeRecommended( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETVAL( pLevelArea, 1 );
	RAND tLvlRand;		
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &tLvlRand, nLevelAreaID );	
	return LevelAreaGetRandomMinMax( tLvlRand, pLevelArea->nPartySizeMinMax[0], pLevelArea->nPartySizeMinMax[1], 1 );
}

//----------------------------------------------------------------------------
//This initializes the all the stats.
//----------------------------------------------------------------------------
void MYTHOS_LEVELAREAS::LevelAreaInitializeLevelAreaOverrides( LEVEL *pLevel )
{
	ASSERTX_RETURN( pLevel, "pLevel was NULL" ); 
	//zero it out
	ZeroMemory( &pLevel->m_LevelAreaOverrides, sizeof( MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES ) );
	pLevel->m_LevelAreaOverrides.fDRLGScaleSize = 1.0f;			//default value for scaling the DRLG
	pLevel->m_LevelAreaOverrides.fChampionSpawnMult = 1.0f;		//default value for scaling how offten champions appear
	pLevel->m_LevelAreaOverrides.fMonsterDensityMult = 1.0f;	//default value for monster density
	pLevel->m_LevelAreaOverrides.nLevelDMGScale = -1.0f;
	//pLevel->m_LevelAreaOverrides.nDifficulty = 0;
	pLevel->m_LevelAreaOverrides.fDifficultyScaleByLevel = .5f;
	if( AppIsHellgate() )
		return;	//no hellgate here
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelAreaDefinitionGet( LevelGetLevelAreaID( pLevel ) );
	ASSERTX_RETURN( pLevelArea, "Level does not have level area" ); 
	
	RAND tRand;
	RAND tLvlRand;	
	//always random

	RandInit( tRand, pLevel->m_dwSeed );					//we have a seed for the level based off the object spawning the dungeon
	//never random for the level area
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &tLvlRand, LevelGetLevelAreaID( pLevel ) );
	
	pLevel->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes = FALSE;
	pLevel->m_LevelAreaOverrides.bOverridesEnabled = TRUE; //overiding
	pLevel->m_LevelAreaOverrides.pLevelArea = pLevelArea; //Parent level area
	pLevel->m_LevelAreaOverrides.nDungeonDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( LevelGetLevelAreaID( pLevel ) );
	pLevel->m_LevelAreaOverrides.nCurrentDungeonDepth = LevelGetDepth( pLevel );
	pLevel->m_LevelAreaOverrides.bIsLastLevel = FALSE;
	if( pLevel->m_LevelAreaOverrides.nCurrentDungeonDepth == pLevel->m_LevelAreaOverrides.nDungeonDepth - 1 ) //we on the last level or not
	{
		//we need to see if we are linking to a different level area
		int nNextLevelArea = MYTHOS_LEVELAREAS::GetLevelAreaID_NextAndPrev_BetweenLevelAreas(  LevelGetLevelAreaID( pLevel ), MYTHOS_LEVELAREAS::KLEVELAREA_LINK_NEXT, pLevel->m_LevelAreaOverrides.nCurrentDungeonDepth );
		if( nNextLevelArea == LevelGetLevelAreaID( pLevel ) ||
			IsLevelAreaAHub( LevelGetLevelAreaID( pLevel ) ))
		{
			pLevel->m_LevelAreaOverrides.bIsLastLevel = TRUE;
		}
	}
	//Lets get the section we are on
	int nSection = LevelAreaGetSectionByDepthInline(  LevelGetLevelAreaID( pLevel ), LevelGetDepth( pLevel ) );

	pLevel->m_LevelAreaOverrides.nLevelDMGScale = pLevelArea->nLevelDMGScale;	//level damage scaler. At some point this needs to be removed.		
	//resize the dungeon now...Total Random
	pLevel->m_LevelAreaOverrides.fDRLGScaleSize = LevelAreaGetRandomMinMaxFloat( tRand, (float)pLevelArea->nLevelSizeMultMinMax[0], (float)pLevelArea->nLevelSizeMultMinMax[1], 100 )/100.f;	
	if( pLevelArea->m_Sections[nSection].nLevelScale[0] > 0 )
	{
		pLevel->m_LevelAreaOverrides.fDRLGScaleSize *= LevelAreaGetRandomMinMaxFloat( tRand, (float)pLevelArea->m_Sections[nSection].nLevelScale[0], (float)pLevelArea->m_Sections[nSection].nLevelScale[1], 100 )/100.f;
	}
	//Get chance for champtions to appear...
	pLevel->m_LevelAreaOverrides.fChampionSpawnMult = LevelAreaGetChampionMult( &pLevel->m_LevelAreaOverrides );
	//Get the spawn class for this level and floor...
	pLevel->m_LevelAreaOverrides.nSpawnClass = LevelAreaGetSpawnClass( tLvlRand, &pLevel->m_LevelAreaOverrides, pLevel );	 //this use to be based off hte level area ID
	//Get the XP mod per monster for this floor
	pLevel->m_LevelAreaOverrides.nLevelXPModForNormalMonsters = LevelAreaGetXPModForNormalMonsters( &pLevel->m_LevelAreaOverrides );	 //this use to be based off the level area ID
	//Get the monster density mult...Total random
	pLevel->m_LevelAreaOverrides.fMonsterDensityMult = LevelAreaGetMonsterDensity( tRand, &pLevel->m_LevelAreaOverrides );
	//Object to spawn - make total random
	pLevel->m_LevelAreaOverrides.bHasObjectCreatedSomeWhereInDungeon = LevelAreaHasObjectCreatedIn( tRand, &pLevel->m_LevelAreaOverrides );
	pLevel->m_LevelAreaOverrides.nObjectSpawn = LevelAreaGetObjectToCreate( tRand, &pLevel->m_LevelAreaOverrides );
	//this is the recommended party size of the dungeon
	pLevel->m_LevelAreaOverrides.nPartySizeRecommended = LevelAreaGetMinPartySizeRecommended( LevelGetLevelAreaID( pLevel ) );
	//zone for the level to appear in
	pLevel->m_LevelAreaOverrides.nZoneToAppearAt = LevelAreaGetZoneToAppearIn( LevelGetLevelAreaID( pLevel ), pLevelArea ); 


	//level area difficulty
	pLevel->m_LevelAreaOverrides.fDifficultyScaleByLevel = LevelAreaGetRandomMinMaxFloat( tLvlRand, (float)pLevelArea->fDifficultyScaleByLevelMin, (float)pLevelArea->fDifficultyScaleByLevelMax, .5f );
	//this is the diffculty of the dungeon	
	//pLevel->m_LevelAreaOverrides.nDifficulty = LevelAreaGetMinDifficulty( LevelGetLevelAreaID( pLevel ) );// LevelAreaGetRandomMinMax( tLvlRand, pLevelArea->nDifficultyLevelAdditional[0], pLevelArea->nDifficultyLevelAdditional[1] );


	//Lets check to see if the client and server can have different themes	
	pLevel->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes = pLevelArea->bAllowsForDiffThemesForClientVsServer | pLevelArea->m_Sections[ nSection ].bAllowsForDiffThemesForClientVsServer;

	//This is not really implemented yet.
	/*
	pLevel->m_LevelAreaOverrides.eConditional = (LEVEL_AREA_CONDITIONALS)RandGetNum( tLvlRand, 0, LVL_AREA_CON_COUNT );	
	switch( pLevel->m_LevelAreaOverrides.eConditional )
	{
	case LVL_AREA_CON_X_TIMES:
			pLevel->m_LevelAreaOverrides.nConditionalParam = RandGetNum( tRand, pLevelArea->nCon_VisitXTimesMin, pLevelArea->nCon_VisitXTimesMax );
		break;
	case LVL_AREA_CON_X_HOURS:
			pLevel->m_LevelAreaOverrides.nConditionalParam = RandGetNum( tRand, pLevelArea->nCon_AfterXNumberOfHoursMin, pLevelArea->nCon_AfterXNumberOfHoursMax );
		break;
	default:
			pLevel->m_LevelAreaOverrides.nConditionalParam = 0;
		break;
	};
	*/


}

//The post process for the Level Areas
BOOL MYTHOS_LEVELAREAS::LevelAreaExcelPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ZeroMemory( sLevelAreasAlwaysVisible, sizeof( int ) * MYTHOS_LEVELAREAS::LVL_AREA_MAX_ALWAYS_VISIBLE );
	sLevelAreasAlwaysVisibleCount = 0;
	MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea( NULL );	

	//first things first. Link all level areas
	for( int t = 0; t < (int)ExcelGetCountPrivate(table); t++ )
	{
		pLevelArea = (MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION*)ExcelGetDataPrivate(table, t);		
		ASSERT_RETFALSE(pLevelArea);
		
		if( pLevelArea->bAlwaysShowsOnMap )
		{
			sLevelAreasAlwaysVisible[ sLevelAreasAlwaysVisibleCount ] = t;
		}
		BOOL bCanBreak( FALSE );		
		for( int y = 0; y < LVL_AREA_MAX_SECTIONS; y++ )
		{
			bCanBreak = FALSE;
			for( int i = 0; i < LVL_AREA_MAX_DRLGS_CHOOSE; i++ )
			{
				if( pLevelArea->m_Sections[ y ].nLevels[i] == INVALID_ID )
				{
					bCanBreak = TRUE;
					continue;
				}
				ASSERTX_CONTINUE( !bCanBreak, "Invalid DRLG choice." );
				pLevelArea->m_Sections[ y ].nLevelCount++;
			}	
		}
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_ARRAY; i++ )
		{
			if( pLevelArea->nSpanwClassFloorsSpawnClasses[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid spawn class typed in for level area on spawn on floor." );			
		}
		
		for( int y = 0; y < LVL_AREA_MAX_SECTIONS; y++ )
		{
			bCanBreak = FALSE;
			for( int i = 0; i < LVL_AREA_MAX_SPAWNCLASSES; i++ )
			{
				if( pLevelArea->m_Sections[ y ].nSpawnClassOverride[ i ] == INVALID_ID )
				{
					bCanBreak = TRUE;
					continue;
				}
				ASSERTX_CONTINUE( !bCanBreak, "Invalid spawn class typed in for level area on spawn." );			
				pLevelArea->m_Sections[ y ].nSpawnClassCount++;
			}
		}
		for( int y = 0; y < LVL_AREA_MAX_SECTIONS; y++ )
		{
			bCanBreak = FALSE;
			for( int i = 0; i < LVL_AREA_MAX_SPAWNCLASSES; i++ )
			{
				if( pLevelArea->m_Sections[ y ].m_Themes.nThemes[ i ] == INVALID_ID )
				{
					bCanBreak = TRUE;
					continue;
				}
				ASSERTX_CONTINUE( !bCanBreak, "Invalid theme level area on spawn." );							
			}
		}
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_ARRAY; i++ )
		{
			if( pLevelArea->nObjectsToSpawn[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid object specified in level area to create." );			
			pLevelArea->nObjectsToSpawnCount++;
		}
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_ARRAY; i++ )
		{
			if( pLevelArea->nBossQuality[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid quality specified." );			
			pLevelArea->nBossQualityCount++;
		}		
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_ARRAY; i++ )
		{
			if( pLevelArea->nBossEliteQuality[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid quality for elite specified." );			
			pLevelArea->nBossEliteQualityCount++;
		}
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_HUBS; i++ )
		{
			if( pLevelArea->nAreaHubs[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid zone specified." );			
			pLevelArea->nNumAreaHubs++;
		}
		bCanBreak = FALSE;
		for( int i = 0; i < LVL_AREA_MAX_HUBS; i++ )
		{
			if( pLevelArea->nTransporterAreas[ i ] == INVALID_ID )
			{
				bCanBreak = TRUE;
				continue;
			}
			ASSERTX_CONTINUE( !bCanBreak, "Invalid zone specified." );			
			pLevelArea->nNumTransporterAreas++;
		}

		//Lets see if the level is setup for having different themes for clients versus server
		pLevelArea->bAllowsForDiffThemesForClientVsServer = FALSE;
		for( int i = 0; i < LVL_AREA_GENERIC_THEMES; i++ )
		{
			if( pLevelArea->nThemePropertyGeneric[ i ] != INVALID_ID &&
				pLevelArea->nThemePropertyGeneric[ i ] != KLEVELAREA_THEME_JUST_SHOW )
			{
				//It has a theme that is dependent on a quest so the themes can be different
				pLevelArea->bAllowsForDiffThemesForClientVsServer = TRUE;
				break;
			}
		}
		for( int s = 0; s < LVL_AREA_MAX_SECTIONS; s++ )
		{
			pLevelArea->m_Sections[ s ].bAllowsForDiffThemesForClientVsServer = FALSE;
			for( int i = 0; i < LVL_AREA_MAX_THEMES; i++ )
			{
				if( pLevelArea->m_Sections[ s ].m_Themes.nThemeProperty[ i ] != INVALID_ID &&
					pLevelArea->m_Sections[ s ].m_Themes.nThemeProperty[ i ] != KLEVELAREA_THEME_JUST_SHOW )
				{
					//It has a theme that is dependent on a quest so the themes can be different
					pLevelArea->m_Sections[ s ].bAllowsForDiffThemesForClientVsServer = TRUE;
					break;
				}
			}
		}
		
	}
	return TRUE;
}

//this returns the actual Level Area Index into the tables
int MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaIDWithNoRandom(int nLevelArea)
{
	if( nLevelArea > MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE )
	{
		nLevelArea = nLevelArea/MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE;
	}
	return nLevelArea;
}

//returns true if the level area is a random level area
BOOL MYTHOS_LEVELAREAS::LevelAreaIsRandom( int nLevelArea )
{
	return ( nLevelArea > MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE );	
}

//this returns the random offset of the level area. So if you pass in say 339857 it'll return 9857
int MYTHOS_LEVELAREAS::LevelAreaGetRandomOffsetNumber(int nLevelArea)
{
	if( nLevelArea > MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE )
	{
		int nLevelAreaTmp = nLevelArea/MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE;
		return ( nLevelArea - ( nLevelAreaTmp * MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE ) );
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION * MYTHOS_LEVELAREAS::LevelAreaDefinitionGet(
	int nLevelArea)
{
	nLevelArea = LevelAreaGetLevelAreaIDWithNoRandom( nLevelArea );
	return (const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS, nLevelArea );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL MYTHOS_LEVELAREAS::LevelAreaShowsOnMap( UNIT *pUnit, int nLevelArea, int nFlags )
{
	LEVEL* pLevel = UnitGetLevel( pUnit );
	if( !pLevel )
		return FALSE;	
	int nZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( LevelGetLevelAreaID( pLevel ), NULL );
	return MYTHOS_LEVELAREAS::LevelAreaShowsOnMapWithZone( pUnit, nZone, nLevelArea, nFlags );
}

BOOL MYTHOS_LEVELAREAS::LevelAreaShowsOnMapWithZone( UNIT *pUnit, int nLevelZoneIn, int nLevelArea, int nFlags )
{
	if( AppIsHellgate() )
		return TRUE;	//hellgate

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//first check to see if the level area has any reason not to show( such as zone's not matching )
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	if( nLevelArea == INVALID_ID )
		return FALSE;	//invalid level ID

	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	if( pLevelArea == NULL )
		return FALSE;

	if( nLevelZoneIn != INVALID_ID )
	{
		int nZoneOther = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelArea, pLevelArea );
		if( nLevelZoneIn != nZoneOther )	//must be in the same zone
			return false;
	}	

	if( pLevelArea->bShowsOnMap ||
		pLevelArea->bAlwaysShowsOnMap ||
		( nFlags & MYTHOS_LEVELAREAS::KLEVELAREA_MAP_FLAGS_ALLOWINVISIBLE ) != 0 )
	{
		return TRUE;	//it just always shows
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//End level area checking - lets check player next
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	if( !pUnit )
		return FALSE; //must have unit
	
	LEVEL* pLevel = UnitGetLevel( pUnit );
	if( !pLevel )
		return FALSE;	//no level

	if( UnitGetLevelAreaID( pUnit ) == nLevelArea )
		return TRUE; //if your there show it!

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//End check player - lets check multi-player next
	//////////////////////////////////////////////////////////////////////////////////////////////////////


#if !ISVERSION(SERVER_VERSION)
	int nMember = 0;
	int nPartyMemberAreas[MAX_CHAT_PARTY_MEMBERS];

	 while (nMember < MAX_CHAT_PARTY_MEMBERS)
	 {
		nPartyMemberAreas[nMember] = INVALID_ID;
	    if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
	    {
		    int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
		    if( nArea >= 0 )
		    {
				if( nArea == nLevelArea )
				{
					int nZoneIn = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nArea, pLevelArea );
					return ( nLevelZoneIn == INVALID_ID || nLevelZoneIn == nZoneIn ); //they have to be in the same zone					
				}
			}
		}
	    nMember++;

    }
#endif
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//End party checks
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	return FALSE; //No reason to show it
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID MYTHOS_LEVELAREAS::LevelAreaGetPartyMemberInArea( UNIT *pUnit, int nLevelArea, GAMEID *pIdGameOfPartyMember)
//MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea )
{
	ASSERTX_RETVAL(AppIsTugboat(), INVALID_GUID, "Level areas are only applicable in tugboat!");	

#if !ISVERSION(SERVER_VERSION)
	int nMember = 0;

	while (nMember < MAX_CHAT_PARTY_MEMBERS)
	{
		if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
		{
			int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
			if(nArea == nLevelArea)
			{
				if(pIdGameOfPartyMember) *pIdGameOfPartyMember = c_PlayerGetPartyMemberGameId(nMember); 
				return  c_PlayerGetPartyMemberGUID(nMember);
			}
		}
		nMember++;
	}
#else
	ASSERT_MSG("Currently client-only"); //We could make a server version using s_partycache functions.
#endif
	return INVALID_GUID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Iterates known level areas for the player.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MYTHOS_LEVELAREAS::LevelAreaIsKnowByPlayer( UNIT *pPlayer, int nLevelArea )
{
	MYTHOS_LEVELAREAS::CLevelAreasKnownIterator iter( pPlayer );
	while( iter.GetNextLevelAreaID( MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_INVISIBLE_AREAS ) != INVALID_ID )
	{
		if( iter.GetLevelAreaIDCurrent() == nLevelArea )
			return TRUE;
	}
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( UnitGetLevelAreaID( pPlayer ) );
	for( int i = 0; i < pLevelArea->nNumTransporterAreas; i++ )
	{
		if( pLevelArea->nTransporterAreas[i] == nLevelArea )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int MYTHOS_LEVELAREAS::LevelAreaGetIconType( int nLevelArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	return pLevelArea->nIconType;
}

BOOL MYTHOS_LEVELAREAS::LevelAreaIsHostile( int nLevelArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	return pLevelArea->bIsHostile;
}

int MYTHOS_LEVELAREAS::LevelAreaGetDifficulty( const LEVEL* pLevel )
{
	
	const MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverrides = LevelGetLevelAreaOverrides( pLevel );
	int nDif = (int)( //pOverrides->nDifficulty +
		   FLOOR((float)pOverrides->nCurrentDungeonDepth * pOverrides->fDifficultyScaleByLevel) +
		   LevelGetDifficultyOffset( pLevel ) );//+
		   //1 );
	return nDif;
	//return 1 + (int)FLOOR( (float)( LevelGetDepth( pLevel ) - 1 ) * pLevelArea->fDifficultyScaleByLevel +  (float)( LevelGetDifficultyOffset( pLevel ) ) );
}

inline int LevelAreaGetDifficultyByComparison( int nLevelArea,
								   BOOL bMaxValue )
{
	int nMinValue = LevelAreaGetLevelAreaDifficulty( nLevelArea, FALSE );
	int nMaxValue = LevelAreaGetLevelAreaDifficulty( nLevelArea, TRUE );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	ASSERT_RETVAL( pLevelArea, nMinValue );
	if( bMaxValue )
		return MAX( nMinValue + pLevelArea->nMinDifficultyLevel, nMaxValue + pLevelArea->nMaxDifficultyLevel );
	else
		return MIN( nMinValue + pLevelArea->nMinDifficultyLevel, nMaxValue + pLevelArea->nMaxDifficultyLevel );
}

int MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( int nLevelArea,
												  UNIT *pPlayer)
{
	if( pPlayer )
	{
		int nRandomMax = MYTHOS_QUESTS::QuestGetLevelAreaDifficultyOverride( pPlayer, nLevelArea, TRUE );
		if( nRandomMax != -1 )
			return nRandomMax;		
	}
	return LevelAreaGetDifficultyByComparison( nLevelArea, TRUE );
}
int MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( int nLevelArea, 
												  UNIT *pPlayer )
{
	if( pPlayer )
	{
		int nRandomMin = MYTHOS_QUESTS::QuestGetLevelAreaDifficultyOverride( pPlayer, nLevelArea, FALSE );
		if( nRandomMin != -1 )
			return nRandomMin;
	}
	return LevelAreaGetDifficultyByComparison( nLevelArea, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * MYTHOS_LEVELAREAS::LevelAreaGetDevName( int nLevelArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *ptLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	return ptLevelArea->pszName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * MYTHOS_LEVELAREAS::LevelAreaGetDisplayName( int nLevelArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	return StringTableGetStringByIndex( pLevelArea->nLevelAreaDisplayNameStringIndex );
}


BOOL MYTHOS_LEVELAREAS::LevelAreaDontDisplayDepth( int nLevelArea, int nDepth )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	ASSERT_RETFALSE( pLevelArea );
	int nSection = LevelAreaGetSectionByDepthInline( nLevelArea, nDepth );
	return pLevelArea->m_Sections[ nSection ].bDontShowDepth;	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MYTHOS_LEVELAREAS::LevelAreaGetModifiedDRLGSize( const LEVEL *pLevel,
													 const DRLG_PASS *pDRLG,
													 BOOL bMin )
{
	if( !pLevel ||
		pLevel->m_LevelAreaOverrides.bOverridesEnabled == FALSE )
	{
		return (bMin)?pDRLG->nMinSubs[0]:pDRLG->nMaxSubs[0];
	}
	if( pDRLG->bMustPlace &&
		pDRLG->nMinSubs[0] == 1 &&
		pDRLG->nMaxSubs[0] == 1 )
		return 1; //stairs!

	if( bMin )
	{
		if( pLevel &&
			pLevel->m_LevelAreaOverrides.bOverridesEnabled )
		{
			int nNewMinSize = (int)FLOOR( (float)pDRLG->nMinSubs[0] * pLevel->m_LevelAreaOverrides.fDRLGScaleSize );
			if( nNewMinSize < 0 ) 
			{
				nNewMinSize = 0;
			}
			if( nNewMinSize == 0 && pDRLG->nMinSubs[0] > 0 ) 
			{
				nNewMinSize = 1;
			}
			return nNewMinSize;
		}
		return pDRLG->nMinSubs[0];
	}
	else
	{
		if( pLevel &&
			pLevel->m_LevelAreaOverrides.bOverridesEnabled )
		{
			static BOOL nSetToZero( TRUE );
			int nNewMaxSize = (int)FLOOR( (float)pDRLG->nMaxSubs[0] * pLevel->m_LevelAreaOverrides.fDRLGScaleSize );
			if( nNewMaxSize < 0 ) nNewMaxSize = 0;
			if( nNewMaxSize == 0 && pDRLG->bMustPlace ) nNewMaxSize = 1;
			if( !nSetToZero &&
				!pDRLG->bMustPlace )
			{			
				if( nNewMaxSize == 0 && pDRLG->nMaxSubs[0] != 0 ) nNewMaxSize = 1;
			}
			nSetToZero = !nSetToZero;
			return nNewMaxSize;
		}
		return pDRLG->nMaxSubs[0];
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( const LEVEL *pLevel,
											  int nLevelAreaID,
											  int nDepth )
{
	ASSERT_RETINVALID( pLevel || nLevelAreaID != INVALID_ID );
	if( nLevelAreaID == INVALID_ID )
	{
		nLevelAreaID = LevelGetLevelAreaID( pLevel );
	}
	ASSERT_RETINVALID( nLevelAreaID != INVALID_ID );
	//ASSERTX_RETINVALID((unsigned int)nArea < ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_LEVEL_AREAS), "Invalid area" );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETINVALID(pLevelArea, "Invalid area" );
	ASSERTX_RETINVALID(pLevelArea->m_Sections[0].nLevels[0] != INVALID_ID, "Section 0 of the level area is invalid" );
	//always allow for a last level of a specific type
	if( nDepth < 0 )
	{
		nDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelAreaID ) - 1;
	}
	if( pLevel )
	{
		int nMaxDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelAreaID );	
		
		if( nDepth >= nMaxDepth )
		{
			nDepth = nMaxDepth;
			if( pLevelArea->nLastLevel != INVALID_ID )
			{
				return pLevelArea->nLastLevel;
			}
		}
	}
	RAND Rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelAreaID );	
	int nSection = LevelAreaGetSectionByDepthInline( nLevelAreaID, nDepth );	
	int nLevelDefinition = pLevelArea->m_Sections[nSection].nLevels[RandGetNum( Rand, 0, pLevelArea->m_Sections[nSection].nLevelCount - 1  )];
	ASSERTX_RETINVALID((unsigned int)nLevelDefinition < ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_LEVEL), "Invalid LevelDefinitionGet() params" );
	return nLevelDefinition;

}





//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sCanBeLevelAreaKillTarget(
	GAME* pGame, 
	int nClass, 
	const UNIT_DATA* pUnitData)
{
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GOOD))
	{
		return FALSE;
	}
	if (!UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CAN_BE_CHAMPION))
	{
		return FALSE;
	}
	return TRUE;
}	

//returns the boss at should be at the end level
int MYTHOS_LEVELAREAS::LevelAreaGetEndBossClassID( LEVEL *pLevel,
												   BOOL &bIsElite )
{
	ASSERTX_RETINVALID( pLevel, "Level is NULL" );
	int nClassID( INVALID_ID );
	bIsElite = FALSE;
	GAME *pGame = LevelGetGame( pLevel );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES *pOverRides = LevelGetLevelAreaOverrides( pLevel );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelGetLevelAreaOverrides( pLevel )->pLevelArea;
	if( !pOverRides->bIsLastLevel )
		return -1;	//bosses only appear on the last level for now.

	//Ok this is bad but who wants to rewrite the entire spawn class for it to allow a seed to be passed
	//in for the picker so that we can always get the same monster for a saved dungeon....
	RAND tGameRand = pGame->rand;	
	//we are using the seed from the item that spawn the dungeon
	//so we always will get the same value.	
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &pGame->rand,
								  LevelGetLevelAreaID( pLevel ) );


	if( pLevelArea->nChanceForEliteBoss > 0 &&
		(int)RandGetNum( pGame->rand, 0, 100 ) <= pLevelArea->nChanceForEliteBoss )
	{
		
		//first try to spawn the boss from a spawn class
		if( pLevelArea->nBossEliteSpawnClass != INVALID_ID )
		{
			int nQuestSpawnClass = pLevel->m_nQuestSpawnClass;
			pLevel->m_nQuestSpawnClass = pLevelArea->nBossEliteSpawnClass;
			nClassID = s_LevelSelectRandomSpawnableQuestMonster( LevelGetGame( pLevel ), 
																pLevel, 
																LevelGetDepth( pLevel ),  // <-- this seems wrong to me, why not use the pLevel? -Colin
																LevelGetDepth( pLevel ), 
																sCanBeLevelAreaKillTarget );
			pLevel->m_nQuestSpawnClass = nQuestSpawnClass;
		}
		//if no classID was found it's not an elite
		bIsElite = ( nClassID != INVALID_ID );
	}
	//no elite so lets just try it with the normal monsters now.
	if( !bIsElite )
	{
		//first try to spawn the boss from a spawn class
		if( pLevelArea->nBossSpawnClass != INVALID_ID )
		{
			int nQuestSpawnClass = pLevel->m_nQuestSpawnClass;
			pLevel->m_nQuestSpawnClass = pLevelArea->nBossSpawnClass;
			nClassID = s_LevelSelectRandomSpawnableQuestMonster( LevelGetGame( pLevel ), 
																pLevel, 
																LevelGetDepth( pLevel ),  // <-- this seems wrong to me, why not use the pLevel? -Colin
																LevelGetDepth( pLevel ), 
																sCanBeLevelAreaKillTarget );
			pLevel->m_nQuestSpawnClass = nQuestSpawnClass;
		}
	}

	//Ok now reset the game's randomness...
	pGame->rand = tGameRand;

	return nClassID;
}
int MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( int nLevelArea )
{

	//ASSERTX_RETINVALID((unsigned int)nArea < ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_LEVEL_AREAS), "Invalid area" );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelArea );
	ASSERT_RETINVALID(pLevelArea);

	RAND Rand;
	//RandInit( Rand, ( nArea + 1 ) * dwSeed );
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelArea );
	int nMaxDepth(0);

	if( pLevelArea->m_Sections[0].nLevels[0] == INVALID_ID )
	{
		//all levels should use the sections now instead of just specifying depth 
		ASSERT( pLevelArea->m_Sections[0].nLevels[0] != INVALID_ID );
		return 1;
	}

	for( int t = 0; t < LVL_AREA_MAX_SECTIONS; t++ )
	{
		if( pLevelArea->m_Sections[t].nLevels[0] != INVALID_ID )
		{
			if( pLevelArea->m_Sections[t].nDepthRange[1] > 0 )
			{
				nMaxDepth += RandGetNum( Rand, pLevelArea->m_Sections[t].nDepthRange[0], pLevelArea->m_Sections[t].nDepthRange[1] );
			}
			else
			{
				nMaxDepth += MAX( 1, pLevelArea->m_Sections[t].nDepthRange[0] );
			}
		}
	}		
	return nMaxDepth;
}







int MYTHOS_LEVELAREAS::LevelAreaGetBossQuality( LEVEL *pLevel,
							 const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea,
							 BOOL bElite )
{
	ASSERTX_RETINVALID( pLevelArea, "no level area" ); 
	RAND tLvlRand;
	RandInit( tLvlRand, pLevel->m_dwSeed );		//we also have the seed the dungeon is using
	if( bElite )
	{
		if( pLevelArea->nBossEliteQualityCount <= 0 )
			return INVALID_ID;

		return pLevelArea->nBossEliteQuality[ RandGetNum( tLvlRand, 0, pLevelArea->nBossEliteQualityCount - 1 ) ];
	}
	else
	{
		if( pLevelArea->nBossQualityCount <= 0 )
			return INVALID_ID;

		return pLevelArea->nBossQuality[ RandGetNum( tLvlRand, 0, pLevelArea->nBossEliteQualityCount - 1 ) ];
	}
	
}


void MYTHOS_LEVELAREAS::LevelAreaFillLevelWithRandomTheme( LEVEL *pLevel,
						   const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea )
{
	ASSERTX_RETURN( pLevelArea, "no level area" ); 
	ASSERTX_RETURN( pLevel, "no level" ); 
	GAME *pGame = LevelGetGame( pLevel );
	PickerStart(pGame, picker);
	// Add a "no theme" item
	PickerAdd(MEMORY_FUNC_FILELINE(pGame, INVALID_ID, 1));
	for( int t = 0; t < LVL_AREA_MAX_THEMES; t++ )
	{
		if( pLevelArea->nAreaThemes[ t ] != INVALID_ID )
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, pLevelArea->nAreaThemes[ t ], 1));
		}
	}

	int nRandomTheme = PickerChoose(pGame);
	if(nRandomTheme >= 0)
	{
		pLevel->m_pnThemes[pLevel->m_nThemeIndexCount] = nRandomTheme;
		pLevel->m_nThemeIndexCount++;
	}


}

void MYTHOS_LEVELAREAS::LevelAreaExecuteLevelAreaScriptOnMonster( UNIT *pMonster )
{
	if( pMonster == NULL ||
		UnitGetGenus( pMonster ) != GENUS_MONSTER ||
		UnitIsA( pMonster, UNITTYPE_HIRELING ) ||
		UnitIsA( pMonster, UNITTYPE_MINOR_PET ) ||
		UnitIsA( pMonster, UNITTYPE_MAJOR_PET ) )
		return;
	ASSERTX_RETURN( UnitGetLevel( pMonster ), "Monster doesn't have a level" );
	LEVEL *pLevel = UnitGetLevel( pMonster );
	if( pLevel )
	{		
		MYTHOS_LEVELAREAS::ExecuteLevelAreaScript( pLevel, 
												   KSCRIPT_LEVELAREA_MONSTERINIT,
												   pMonster );
	}
	
}

void MYTHOS_LEVELAREAS::ExecuteLevelAreaScript( LEVEL *pLevel,
							 KLevelAreaScripts scriptType,
							 UNIT *Unit,
							 UNIT *pTarget,
							 STATS *pStats,// = NULL,
							 int nParam1,// = INVALID_ID,
							 int nParam2,// = INVALID_ID,
							 int nSkillID,// = INVALID_ID,
							 int nSkillLevel,// = INVALID_ID,
							 int nState ) //= INVALID_ID )
{
	GAME *pGame = UnitGetGame( Unit );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelGetLevelAreaOverrides( pLevel )->pLevelArea;
	ASSERT_RETURN( scriptType >= 0 && scriptType < KSCRIPT_LEVELAREA_COUNT );
	if( pLevelArea )
	{
		PCODE codeExecute( NULL_CODE );		
		codeExecute = pLevelArea->codeScripts[scriptType];
		if( codeExecute  )
		{
			int code_len = 0;
			BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_LEVEL_AREAS, codeExecute, &code_len);		
			if( IS_CLIENT( pGame ) )
			{
				VMExecI( pGame, GameGetControlUnit( pGame ), (pTarget)?pTarget:Unit, pStats, nParam1, nParam2, nSkillID, nSkillLevel, nState, code_ptr, code_len);
			}
			else
			{
				VMExecI( pGame, Unit, (pTarget)?pTarget:Unit, pStats, nParam1, nParam2, nSkillID, nSkillLevel, nState, code_ptr, code_len);
			}
		}
	}

}

int MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( int nLevelAreaID, const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea )
{
	if( pLevelArea == NULL )
	{
		pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );		
	}
	ASSERT_RETINVALID( pLevelArea );
	if( pLevelArea->nNumAreaHubs <= 0 )
		return nLevelAreaID;  //no hub to attach to ( could be a hub )
	RAND tLvlRand;		
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &tLvlRand, nLevelAreaID );
	return pLevelArea->nAreaHubs[ RandGetNum( tLvlRand, 0, pLevelArea->nNumAreaHubs - 1 ) ];
}

int MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( int nLevelAreaID, const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea )
{
	if( pLevelArea == NULL )
	{
		pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );		
	}
	ASSERTX_RETINVALID( pLevelArea, "Level area is null" );
	int levelAreaHubID = LevelAreaGetHubAttachedTo( nLevelAreaID, pLevelArea );
	if( levelAreaHubID == nLevelAreaID ) //hub in which it is around
	{
		ASSERT_RETINVALID( pLevelArea->nZone != INVALID_ID );
		return pLevelArea->nZone; //this is the zone the hub/area is located
	}
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelHubAttachedTo = LevelAreaDefinitionGet( levelAreaHubID );		
	ASSERTX_RETINVALID( pLevelHubAttachedTo, "Level HUB is null" );
	return LevelAreaGetZoneToAppearIn( levelAreaHubID, pLevelHubAttachedTo ); //yes we now loop till we get back to the main hub

}

int MYTHOS_LEVELAREAS::LevelAreaHasHardCodedPosition( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETINVALID(pLevelArea, "Invalid area" );
	return ( pLevelArea->nX > 0 && pLevelArea->nY > 0 );
}

int MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaName( int nLevelAreaID, int nDepth, BOOL bForceGeneralNameOfArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETINVALID(pLevelArea, "Invalid area" );
	if( nDepth < 0 )
		nDepth = 0;
	int nSection = LevelAreaGetSectionByDepthInline(  nLevelAreaID, nDepth );
	int nNameOveride = pLevelArea->m_Sections[ nSection ].nLevelAreaNameOverride;
	if( nNameOveride == INVALID_ID || bForceGeneralNameOfArea )
		return pLevelArea->nLevelAreaDisplayNameStringIndex;
	return nNameOveride;
}

int MYTHOS_LEVELAREAS::LevelAreaGetWeather( LEVEL *pLevel )
{
	ASSERT_RETINVALID( pLevel );
	int nLevelAreaID = LevelGetLevelAreaID( pLevel );
	ASSERT_RETINVALID( nLevelAreaID != INVALID_ID );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETINVALID( pLevelArea );
	int nDepth = LevelGetDepth( pLevel );
	if( nDepth < 0 )
		nDepth = 0;
	int nSection = LevelAreaGetSectionByDepthInline(  nLevelAreaID, nDepth );
	ASSERT_RETINVALID( nSection != INVALID_ID );
	int nCount( 0 );
	for( int t = 0; t < LVL_AREA_MAX_WEATHERS; t++ )
	{
		if( pLevelArea->m_Sections[ nSection ].nWeathers[t] == INVALID_ID )
			break;
		nCount++;
	}
	if( nCount <= 1 )
		return pLevelArea->m_Sections[ nSection ].nWeathers[ 0 ];

	RAND tLvlRand;		
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &tLvlRand, nLevelAreaID );
	return pLevelArea->m_Sections[ nSection ].nWeathers[ RandGetNum( tLvlRand, 0, nCount ) - 1 ];
}



int MYTHOS_LEVELAREAS::LevelAreaGetRandomLevelID( int nLevelAreaID, RAND &rand )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETINVALID( pLevelArea );
	int nRandomOffset = RandGetNum( rand, pLevelArea->nDungeonsToCreateFromTemplate );
	return LevelAreaBuildLevelAreaID( nLevelAreaID, nRandomOffset );	
}


int MYTHOS_LEVELAREAS::LevelAreaGetLevelLink( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETINVALID(pLevelArea, "Invalid area" );
	return pLevelArea->nLinkToLevelArea;	
}

inline void UpdateThemesForLevelArea( LEVEL *pLevel, const int *nThemes, const int *nQuestTask, const int *nThemeProperty, int count )
{
	if( pLevel->m_nThemeIndexCount >= MAX_THEMES_PER_LEVEL )
		return;
	for( int t = 0; t < count; t++ )
	{
		if( nThemes[t] == INVALID_ID )
			return;
		BOOL Add( TRUE );
		//Check Quests on Client only...
		if( Add )
		{
			if( IS_CLIENT( LevelGetGame( pLevel ) ) )
			{
				const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData( LevelGetGame( pLevel ), DATATABLE_LEVEL_THEMES, nThemes[t]);
				REF( pLevelTheme );
				BOOL bRemoveTheme( FALSE );
				UNIT *pPlayer = GameGetControlUnit( LevelGetGame( pLevel ) );
				if( pPlayer == NULL )
					pPlayer = UnitGetById( LevelGetGame( pLevel ), 0 );
				if( pPlayer &&
					nQuestTask[ t ] != INVALID_ID &&
					nThemeProperty[ t ] != INVALID_ID &&
					nThemeProperty[ t ] != MYTHOS_LEVELAREAS::KLEVELAREA_THEME_JUST_SHOW )
				{
					Add = FALSE;

					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( nQuestTask[ t ] );
					BOOL questTaskComplete = QuestIsQuestTaskComplete( pPlayer, pQuestTask );					
					BOOL questTaskIsActive = QuestPlayerHasTask( pPlayer, pQuestTask );
					switch( nThemeProperty[ t ] )
					{
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_SHOW_IFNOTCOMPLETE:
						Add = !questTaskComplete;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_HIDE_IFCOMPLETE:						
						bRemoveTheme = questTaskComplete;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_SHOW_IFCOMPLETE:
						Add = questTaskComplete;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_HIDE_IFNOTCOMPLETE:
						bRemoveTheme = !questTaskComplete;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_SHOW_IFTASKISACTIVE:
						Add = questTaskIsActive;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_HIDE_IFTASKISNOTACTIVE:
						bRemoveTheme = !questTaskIsActive;						
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_SHOW_IFTASKISNOTACTIVE:
						Add = !questTaskIsActive;
						break;
					case MYTHOS_LEVELAREAS::KLEVELAREA_THEME_HIDE_IFTASKISACTIVE:
						bRemoveTheme = questTaskIsActive;						
						break;
					}
				}
				//if we aren't remove or adding a theme on the client no reason
				//to add it to the list.
				if( !bRemoveTheme &&
					!Add )
				{
					continue;
				}
					
			}
		}
		//lets check that the theme isn't already set.
		if( Add )
		{
			for( int i = 0; i < pLevel->m_nThemeIndexCount; i++ )
			{
				if( pLevel->m_pnThemes[i] == nThemes[t] )
				{				
					Add = FALSE;
					break;
				}
			}
			if( !Add )
			{
				continue;
			}

		}
		if( Add )
		{
			pLevel->m_pnThemes[pLevel->m_nThemeIndexCount] = nThemes[t];
			pLevel->m_nThemeIndexCount++;
			if( pLevel->m_nThemeIndexCount == MAX_THEMES_PER_LEVEL )
				return;
		}
		else
		{
			//if it's not meant to be inside the level lets remove it...
			for( int i = 0; i < pLevel->m_nThemeIndexCount; i++ )
			{
				if( pLevel->m_pnThemes[i] == nThemes[t] )
				{				
					pLevel->m_pnThemes[i] = pLevel->m_pnThemes[--pLevel->m_nThemeIndexCount];
					i--;
				}
			}
		}
	}
}
void MYTHOS_LEVELAREAS::LevelAreaPopulateThemes( LEVEL *pLevel )
{
	ASSERT(pLevel);
	if( pLevel->m_nThemeIndexCount == MAX_THEMES_PER_LEVEL )
		return;

	int nLevelAreaID = LevelGetLevelAreaID( pLevel ) ;
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETURN(pLevelArea, "Invalid area" );
	int nSection = LevelAreaGetSectionByDepthInline(  nLevelAreaID, LevelGetDepth( pLevel ) );
	UpdateThemesForLevelArea( pLevel,
								pLevelArea->m_Sections[ nSection].m_Themes.nThemes, 
								pLevelArea->m_Sections[ nSection].m_Themes.nQuestTask, 
								pLevelArea->m_Sections[ nSection].m_Themes.nThemeProperty, 
								LVL_AREA_MAX_THEMES );
	UpdateThemesForLevelArea( pLevel,
								&pLevelArea->nThemesGeneric[0], 
								&pLevelArea->nThemesQuestTaskGeneric[0], 
								&pLevelArea->nThemePropertyGeneric[0], 
								LVL_AREA_GENERIC_THEMES );


}


int MYTHOS_LEVELAREAS::LevelAreaGetLevelDepthDisplay( int nLevelAreaID,
													   int nLevelDepth )
{
	
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETVAL( pLevelArea, nLevelDepth, "Invalid area" );
	int nSection = LevelAreaGetSectionByDepthInline(  nLevelAreaID, nLevelDepth );
	int nName = pLevelArea->m_Sections[nSection].nLevelAreaNameOverride;
	RAND Rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelAreaID );
	int nDepthDisplay(0);
	int nDepthsAddedUp( 0 );
	for( int t = 0; t < MYTHOS_LEVELAREAS::LVL_AREA_MAX_SECTIONS; t++ )
	{
		if( pLevelArea->m_Sections[t].nLevels[0] != INVALID_ID )
		{
			int addDepth( 0 );
			if( pLevelArea->m_Sections[t].nDepthRange[1] > 0 )
			{
				addDepth = RandGetNum( Rand, pLevelArea->m_Sections[t].nDepthRange[0], pLevelArea->m_Sections[t].nDepthRange[1] );
			}
			else
			{
				addDepth = MAX( 1, pLevelArea->m_Sections[t].nDepthRange[0] );
			}
			nDepthsAddedUp += addDepth;
			if( pLevelArea->m_Sections[t].nLevelAreaNameOverride != nName ||
				pLevelArea->m_Sections[t].bDontShowDepth )
			{
				if( pLevelArea->m_Sections[t].bDontShowDepth )  //anything not showing depths clears all depths.
				{
					nDepthDisplay = nDepthsAddedUp;
				}
				else
				{
					nDepthDisplay += addDepth;
				}
			}
			if( nLevelDepth <= nDepthsAddedUp )
			{
				return (nLevelDepth - nDepthDisplay) + 1;
			}
		}
	}
	return nDepthDisplay;
}

//this builds a level area ID based off a random offset.
int MYTHOS_LEVELAREAS::LevelAreaBuildLevelAreaID(int nLevelArea, int nRandomOffset )
{	
	if( nRandomOffset <= 0 )
		return nLevelArea;
	ASSERT_RETVAL( nRandomOffset < MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE, nLevelArea );
	return ( MYTHOS_LEVELAREAS::LVL_AREA_MAX_LEVELS_PER_TEMPLATE * nLevelArea ) + nRandomOffset;
}



inline BOOL sLevelAreaCanSpawnInZone( const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea,
									  int nZone )
{
	ASSERTX_RETFALSE( pLevelArea, "Level area is null" );
	ASSERTX_RETFALSE( nZone >= 0, "Bad zone" );	
	if( pLevelArea->nNumAreaHubs <= 0 )
		return FALSE;  //no hub to attach to ( could be a hub )
	for( int t = 0; t < pLevelArea->nNumAreaHubs; t++ )
	{
		int nZoneHubIsIn = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( pLevelArea->nAreaHubs[t], NULL );
		if( nZoneHubIsIn == nZone )
			return TRUE;
	}

	return FALSE;
}


int MYTHOS_LEVELAREAS::s_LevelAreaGetRandomLevelID( GAME *pGame, 
												    BOOL epic, 
													int nMin, 
													int nMax, 
													int nZoneToSpawnFrom, 
													int nStartingLevelId, /*= INVALID_ID */
													BOOL bRuneStoneOnly /*= FALSE */ )
{
	PickerStart(pGame, picker);
	int nAvailablePicks = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_LEVEL_AREAS);
	
	for (int i = 0; i < nAvailablePicks; ++i)
	{		
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( i );		
		if( pLevelArea->nDungeonsToCreateFromTemplate > 0 )
		{
			BOOL canSpawnInZone = sLevelAreaCanSpawnInZone( pLevelArea, nZoneToSpawnFrom );
			if( !canSpawnInZone )
				continue;   //must be in the same zone
			if(	epic &&
				pLevelArea->nPartySizeMinMax[0] <= 0 )
			{
				continue;//epics have more then one group size
			}
			else if( !epic &&
					 pLevelArea->nPartySizeMinMax[0] > 0 )
			{
				continue;//can't have epic dungeons with normal dung
			}

			if( bRuneStoneOnly &&
				!pLevelArea->bRuneStone )
			{
				continue; //can only use maps made for rune stones
			}
					
			//lets check and make sure the startingLevelId has the same type as the level area
			if( nStartingLevelId != INVALID_ID &&
				pLevelArea->m_Sections[ 0 ].nLevels[ 0 ] != nStartingLevelId )
			{
				continue;//has to match the starting level DRLG type
			}

			if( (nMin != 0 && nMax >= nMin ) )  //make sure we get a range of random maps
			{				
				int nMinLevelMin = pLevelArea->nMinDifficultyLevel;// + min( pLevelArea->nDifficultyLevelAdditional[0],pLevelArea->nDifficultyLevelAdditional[1] );
				int nMaxLevelMax = pLevelArea->nMaxDifficultyLevel + max( pLevelArea->nDifficultyLevelAdditional[0],pLevelArea->nDifficultyLevelAdditional[1] );
				if( nMin >= nMinLevelMin && nMin <= nMaxLevelMax )
				{
					PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pLevelArea->nPickWeight + 500)); //this will force it to pick maps in the local level area.
				}
				else if( !bRuneStoneOnly )
				{
					PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pLevelArea->nPickWeight + 1));
				}
			}
			else if( !bRuneStoneOnly )
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pLevelArea->nPickWeight + 1));
			}
		}
	}
	int nReturnValue = PickerChooseRemove( pGame );	
	return MYTHOS_LEVELAREAS::LevelAreaGetRandomLevelID( nReturnValue, pGame->rand );
}

BOOL MYTHOS_LEVELAREAS::DoesLevelAreaAlwaysShow( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETFALSE( pLevelArea, "Invalid area" );
	return pLevelArea->bAlwaysShowsOnMap;
}


int MYTHOS_LEVELAREAS::LevelAreaGetAngleToHub( int nLevelAreaID )
{

	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = LevelAreaDefinitionGet( nLevelAreaID );
	if( pLevelArea &&
		pLevelArea->nX > 0 && pLevelArea->nY > 0 )
	{
		int nHubAreaID = LevelAreaGetHubAttachedTo( nLevelAreaID, pLevelArea );
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pHubArea = LevelAreaDefinitionGet( nHubAreaID );
		if( pHubArea &&
			pHubArea->nX > 0 && pHubArea->nY > 0 )
		{
			float DeltaX = (float)(pLevelArea->nX - pHubArea->nX);
			float DeltaY = (float)(pLevelArea->nY - pHubArea->nY);
			float fRadians = atan2( DeltaX, -DeltaY );
			return NORMALIZE( (int)(fRadians * 360.0f / (2.0f * PI)), 360 ); 
		}
	}

	RAND Rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelAreaID );
	return LevelAreaGetAngleToHub( nLevelAreaID, Rand );
	
}

int MYTHOS_LEVELAREAS::LevelAreaGetAngleToHub( int nLevelAreaID, RAND &rand )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETFALSE( pLevelArea, "Invalid area" );
	return RandGetNum( rand, 0, 360 );
}

int MYTHOS_LEVELAREAS::LevelAreaGetSectionByDepth( int nLevelAreaID, int nDepth )
{
	return LevelAreaGetSectionByDepthInline( nLevelAreaID, nDepth );
}

MYTHOS_LEVELAREAS::ELEVELAREA_DIFFICULTY MYTHOS_LEVELAREAS::LevelAreaGetDifficultyComparedToUnit( int nLevelArea, UNIT *pPlayer )
{
	ASSERT_RETVAL( pPlayer, LEVELAREA_DIFFICULTY_JUST_RIGHT );
	int nPlayerLevel = UnitGetStat( pPlayer, STATS_LEVEL );
	int nMinLevel = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelArea, pPlayer );	
	int nMaxLevel = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelArea, pPlayer );	
	int nLevelRange = nMaxLevel - nMinLevel;
	if( nMinLevel > nPlayerLevel )
		return LEVELAREA_DIFFICULTY_TO_HIGH;
	if( nMaxLevel < nPlayerLevel )
		return LEVELAREA_DIFFICULTY_TO_EASY;
	if( nLevelRange <= 0 )	//level range is 1 so the player has to be at the right level
		return LEVELAREA_DIFFICULTY_JUST_RIGHT;

	int nHalf = (int)( nLevelRange/2.0f + .5f );
	if( nPlayerLevel <= nMinLevel + nHalf )
		return LEVELAREA_DIFFICULTY_ALMOST_TO_HIGH;

	return LEVELAREA_DIFFICULTY_JUST_RIGHT;
}

//returns the type of level
MYTHOS_LEVELAREAS::KLEVELAREA_TYPE MYTHOS_LEVELAREAS::LevelAreaGetLevelType( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETVAL( pLevelArea, MYTHOS_LEVELAREAS::KLEVELTYPE_NORMAL );
	return (MYTHOS_LEVELAREAS::KLEVELAREA_TYPE)pLevelArea->nLevelType;	
}

//returns how many visible level area there are
int MYTHOS_LEVELAREAS::LevelAreaGetNumOfLevelAreasAlwaysVisible()
{
	return sLevelAreasAlwaysVisibleCount;
}

//returns the level area ID by index of visibility 
int MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaIDThatIsAlwaysVisibleByIndex( int nIndex )
{
	ASSERT_RETINVALID( nIndex >=0 && nIndex < sLevelAreasAlwaysVisibleCount );
	return sLevelAreasAlwaysVisible[ nIndex ];
}


//returns the warp index that this level area will return to ( in the world there could be 5 to say 500 dungeons that all share the same random
int MYTHOS_LEVELAREAS::LevelAreaGetWarpIndexByWarpCount( int nLevelAreaID,
									  				     int nNumOfWarpsInWorld )
{
	if( nNumOfWarpsInWorld <= 0 )
		return 0;
	RAND Rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &Rand, nLevelAreaID );
	return RandGetNum( Rand, 0, nNumOfWarpsInWorld - 1 );
}

//returns if the level is a static world
BOOL MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETFALSE( pLevelArea );
	return pLevelArea->bIsStaticWorld;	

}

//returns TRUE if the level area allows primary saves
BOOL MYTHOS_LEVELAREAS::LevelAreaAllowPrimarySave( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETFALSE( pLevelArea );
	return pLevelArea->bPrimarySave;		
}

//returns TRUE if the level area is a rune stone Level Area ID
BOOL MYTHOS_LEVELAREAS::LevelAreaIsRuneStone( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETFALSE( pLevelArea );
	return pLevelArea->bRuneStone;		
}



//returns the number of levels that can be generated off the area
int MYTHOS_LEVELAREAS::LevelAreaGetNumOfGeneratedMapsAllowed( int nLevelAreaID )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERT_RETVAL( pLevelArea, 0 );
	return ( pLevelArea->nDungeonsToCreateFromTemplate <= 1 )?1:pLevelArea->nDungeonsToCreateFromTemplate;	
}


//returns a random level ID generated by position
int MYTHOS_LEVELAREAS::LevelAreaGenerateLevelIDByPos( int nLevelAreaIDBase, int nX, int nY )
{
	if( MYTHOS_LEVELAREAS::LevelAreaGetNumOfGeneratedMapsAllowed( nLevelAreaIDBase ) <= 1 )
		return nLevelAreaIDBase;	
	int nSeed = ( nX * 10 ) + ( nY * 100 ) + ( (nX + nY ) * 1000 );	
	RAND rand;
	RandInit( rand, nSeed );	//That's right the level area ID is the random seed for the level. Got to love it!
	return MYTHOS_LEVELAREAS::LevelAreaGetRandomLevelID(  nLevelAreaIDBase, rand );	
}
