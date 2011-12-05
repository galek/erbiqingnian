//----------------------------------------------------------------------------
// global_themes.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _GLOBAL_THEMES_H_
#define _GLOBAL_THEMES_H_

#define GLOBAL_THEME_MAX_TREASURECLASS_REPLACEMENTS 8

enum GLOBAL_THEME_MONTH
{
	GTM_INVALID = -1,
	GTM_JANUARY = 1,
	GTM_FEBRUARY,
	GTM_MARCH,
	GTM_APRIL,
	GTM_MAY,
	GTM_JUNE,
	GTM_JULY,
	GTM_AUGUST,
	GTM_SEPTEMBER,
	GTM_OCTOBER,
	GTM_NOVEMBER,
	GTM_DECEMBER,
};

enum GLOBAL_THEME_WEEKDAY
{
	GTW_INVALID = -1,
	GTW_SUNDAY,
	GTW_MONDAY,
	GTW_TUESDAY,
	GTW_WEDNESDAY,
	GTW_THURSDAY,
	GTW_FRIDAY,
	GTW_SATURDAY,
};

struct GLOBAL_THEME_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	int nStartMonth;
	int nStartDay;
	int nStartDayOfWeek;
	int nEndMonth;
	int nEndDay;
	int nEndDayOfWeek;
	int pnTreasureClassPreAndPost[ GLOBAL_THEME_MAX_TREASURECLASS_REPLACEMENTS ][ 2 ];

	BOOL bActivateByTime;
};

void GlobalThemeForce(
	const char * pszTheme,
	BOOL bEnable );

BOOL GlobalThemeIsEnabled(
	GAME * pGame,
	int nGlobalTheme );

void GlobalThemeEnable(
	GAME * pGame,
	int nGlobalTheme );

void GlobalThemeDisable(
	GAME * pGame,
	int nGlobalTheme );

void GlobalThemeMonsterInit(
	GAME * pGame,
	UNIT * pUnit );

void s_GlobalThemesUpdate(
	GAME * pGame );

void s_GlobalThemeSendAllToClient( 
	GAME * pGame,
	GAMECLIENTID idClient );

BOOL ExcelGlobalThemesPostProcess( 
	EXCEL_TABLE * table);

#endif
