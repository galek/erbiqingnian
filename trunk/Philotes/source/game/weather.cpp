//-------------------------------------------------------------------------------------------------
//
// Prime Weather System v1.0
//
// by Guy Somberg
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
// Weather selection and activation functions
//-------------------------------------------------------------------------------------------------

/*
-------------------------------------------------------------------------------------------------
TODO:

-------------------------------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "weather.h"
#include "game.h"
#include "picker.h"
#include "excel.h"
#include "states.h"
#include "units.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static const WEATHER_DATA * sWeatherSelect(
	GAME * pGame,
	const WEATHER_SET_DATA * pWeatherSet,
	DWORD dwSeed)
{
	ASSERT_RETNULL(pWeatherSet);
	RAND tRand;
	RandInit(tRand, dwSeed, 0, TRUE);
	PickerStart(pGame, picker);
	for(int i=0; i<MAX_WEATHER_SETS; i++)
	{
		const WEATHER_SET * pWeather = &pWeatherSet->pWeatherSets[i];
		if(pWeather->nWeight > 0 && pWeather->nWeather >= 0)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pWeather->nWeight));
		}
	}
	int nPick = PickerChoose(pGame, tRand);
	if(nPick >= 0)
	{
		ASSERT_RETNULL(pWeatherSet->pWeatherSets[nPick].nWeather >= 0);
		return (const WEATHER_DATA *)ExcelGetData(NULL, DATATABLE_WEATHER, pWeatherSet->pWeatherSets[nPick].nWeather);
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const WEATHER_DATA * WeatherSelect(
	GAME * pGame,
	int nWeatherSet,
	DWORD dwSeed)
{
	ASSERT_RETNULL(nWeatherSet >= 0);
	const WEATHER_SET_DATA * pWeatherSet = (const WEATHER_SET_DATA*)ExcelGetData(NULL, DATATABLE_WEATHER_SETS, nWeatherSet);
	return sWeatherSelect(pGame, pWeatherSet, dwSeed);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void WeatherApplyStateToUnit(
	UNIT * pUnit,
	const WEATHER_DATA * pWeatherData)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// server only
	if(!IS_SERVER(pGame))
		return;

	StateClearAllOfType(pUnit, STATE_WEATHER);
	if(pWeatherData && pWeatherData->nState >= 0)
	{
		s_StateSet(pUnit, NULL, pWeatherData->nState, 0);
	}
}
