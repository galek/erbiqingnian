#include "stdafx.h"
#include "c_LevelAreasClient.h"
#include "LevelAreas.h"
#include "level.h"
#include "LevelZones.h"
#include "LevelAreaNameGen.h"
#include "stringtable.h"

void MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( int nLevelArea,
												 int nLevelAreaDepth,
												  WCHAR *puszBuffer,
												  int nBufferSiz,
												  BOOL bForceGeneralNameOfArea )
{
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelArea );
	ASSERTX_RETURN( pLevelArea, "Invalid Level Area" );
	return MYTHOS_LEVELAREAS::GetRandomLibName( NULL, nLevelArea, nLevelAreaDepth, puszBuffer, nBufferSiz, bForceGeneralNameOfArea );
}

void MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( int nLevelAreaID, int nZoneID, float &x, float &y )
{
	if( nLevelAreaID == INVALID_ID )
		return;	//this is valid
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETURN( pLevelArea, "Invalid Level Area" );
	if( pLevelArea->nX > 0 )
	{
		x = (float)pLevelArea->nX;
		y = (float)pLevelArea->nY;
	}
	else
	{
		if( nZoneID == INVALID_ID )
		{
			nZoneID = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelAreaID, pLevelArea );
		}
		RAND tLvlRand;
		LevelAreaInitRandByLevelArea( &tLvlRand, nLevelAreaID );
		ASSERTX( MYTHOS_LEVELZONES::GetPixelLocation( tLvlRand, nLevelAreaID, nZoneID, x, y ), "Unable to determin dungeon pixel location" );
	}


}


void MYTHOS_LEVELAREAS::c_GetLevelAreaWorldMapPixels( int nLevelAreaID, int nZoneID, float &x, float &y )
{
	if( nLevelAreaID == INVALID_ID )
		return;	//this is valid
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETURN( pLevelArea, "Invalid Level Area" );
	if( pLevelArea->nWorldX > 0 )
	{
		x = (float)pLevelArea->nWorldX;
		y = (float)pLevelArea->nWorldY;
	}
	else
	{
		x = 0;
		y = 0;
	}

}

const enum FONTCOLOR sDifficultyColors[ MYTHOS_LEVELAREAS::LEVELAREA_DIFFICULTY_COUNT ] = {FONTCOLOR_WHITE,		//LEVELAREA_DIFFICULTY_TO_EASY
																								FONTCOLOR_LIGHT_YELLOW,	//LEVELAREA_DIFFICULTY_JUST_RIGHT	
																								FONTCOLOR_FSORANGE,		//LEVELAREA_DIFFICULTY_ALMOST_TO_HIGH
																								FONTCOLOR_LIGHT_RED };	//LEVELAREA_DIFFICULTY_TO_HIGH

enum FONTCOLOR MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( enum MYTHOS_LEVELAREAS::ELEVELAREA_DIFFICULTY nDifficulty )
{
	return sDifficultyColors[ nDifficulty ];
}

enum FONTCOLOR MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( int nLevelArea, UNIT *pPlayer )
{

	ASSERT_RETVAL( pPlayer, sDifficultyColors[1]  );
	ASSERT_RETVAL( nLevelArea >= 0, sDifficultyColors[1] );
	ELEVELAREA_DIFFICULTY nDifficulty = MYTHOS_LEVELAREAS::LevelAreaGetDifficultyComparedToUnit( nLevelArea, pPlayer );
	return sDifficultyColors[ nDifficulty ]; 
}
