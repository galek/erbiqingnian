//----------------------------------------------------------------------------
// weather.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _WEATHER_H_
#define _WEATHER_H_

#define MAX_THEMES_FROM_WEATHER 4
struct WEATHER_DATA
{
	char	pszName[DEFAULT_INDEX_SIZE];
	int		nState;
	int		nThemes[MAX_THEMES_FROM_WEATHER];
};

struct WEATHER_SET
{
	int		nWeather;
	int		nWeight;
};

#define MAX_WEATHER_SETS 8
struct WEATHER_SET_DATA
{
	char			pszName[DEFAULT_INDEX_SIZE];
	WEATHER_SET		pWeatherSets[MAX_WEATHER_SETS];
};

const WEATHER_DATA * WeatherSelect(
	struct GAME * pGame,
	int nWeatherSet,
	DWORD dwSeed);

void WeatherApplyStateToUnit(
	struct UNIT * pUnit,
	const WEATHER_DATA * pWeatherData);

#endif
