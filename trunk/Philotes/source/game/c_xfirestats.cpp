#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "c_xfirestats.h"
#include "pstring.h"
#include "units.h"
#include "level.h"
#include "stringtable.h"
#include "..\3rd Party\XFire\xfiresdk\xfireapi.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
struct XFIRE_STAT_LIST
{
	XFIRESTATS_TYPE type;
	const WCHAR *name;
	int statid;
	WCHAR value[128];
};

XFIRE_STAT_LIST gpXfireStatList[] =
{
	{ XFIRESTATS_PLAYER_NAME,		L"Player Name", 	-1,					{ L"" } },
	{ XFIRESTATS_LEVEL_NAME,		L"Level Name",		-1,					{ L"" } },
	{ XFIRESTATS_LEVEL,				L"Level",			STATS_LEVEL,		{ L"" } },
	{ XFIRESTATS_HP_CUR,			L"Current HP",		STATS_HP_CUR,		{ L"" } },
	{ XFIRESTATS_HP_MAX_BONUS,		L"Max HP",			STATS_HP_MAX,		{ L"" } },
	{ XFIRESTATS_POWER_CUR,			L"Current Power",	STATS_POWER_CUR,	{ L"" } },
	{ XFIRESTATS_POWER_MAX_BONUS,	L"Max Power",		STATS_POWER_MAX,	{ L"" } },
	{ XFIRESTATS_EXPERIENCE,		L"Experience",		STATS_EXPERIENCE,	{ L"" } },
	{ XFIRESTATS_EXPERIENCE,		L"Gold",			STATS_GOLD,			{ L"" } }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void XfireStatsSend()
{
	const WCHAR *keys[arrsize(gpXfireStatList)];
	const WCHAR *values[arrsize(gpXfireStatList)];

	int count = 0;
	for (int i = 0; i < arrsize(gpXfireStatList); i++)
	{
		if (gpXfireStatList[i].value[0] != '\0')
		{
			keys[count] = (const WCHAR *)gpXfireStatList[i].name;
			values[count] = (const WCHAR *)gpXfireStatList[i].value;
			count++;
		}
	}

	XfireSetCustomGameDataW(count, keys, values);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void XfireStatsSetUnitStat(int statid, int value)
{
	WCHAR valueStr[128];
	PStrPrintf(valueStr, arrsize(valueStr), L"%d", value);

	for (int i = 0; i < arrsize(gpXfireStatList); i++)
	{
		if (statid == gpXfireStatList[i].statid)
		{
			XfireStatsSetString(gpXfireStatList[i].type, valueStr);
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void XfireStatsSetString(XFIRESTATS_TYPE type, const WCHAR *value)
{
	for (int i = 0; i < arrsize(gpXfireStatList); i++)
	{
		if (type == gpXfireStatList[i].type)
		{
			PStrCopy(gpXfireStatList[i].value, value, arrsize(gpXfireStatList[i].value));
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void XfireStateUpdateUnitState(UNIT *pUnit)
{
	if(!pUnit)
		return;

	// get name
	WCHAR szName[256];
	UnitGetName(pUnit, szName, arrsize(szName), UNF_INCLUDE_TITLE_BIT);
	XfireStatsSetString(XFIRESTATS_PLAYER_NAME, szName);

	// get level
	LEVEL *pLevel = UnitGetLevel(pUnit);
	if(pLevel)
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet(pLevel);
		if(pLevelDef)
		{
			const WCHAR *szLevelName = StringTableGetStringByIndex(pLevelDef->nLevelDisplayNameStringIndex);
			if(szLevelName)
			{
				XfireStatsSetString(XFIRESTATS_LEVEL_NAME, szLevelName);
			}
		}
	}

	for (int i = 0; i < arrsize(gpXfireStatList); i++)
	{
		int wStat = gpXfireStatList[i].statid;
		if (wStat >= 0)
		{
			int nValue = UnitGetStatShift(pUnit, wStat);
			XfireStatsSetUnitStat(wStat, nValue);
		}
	}

	XfireStatsSend();
}

#endif //!SERVER_VERSION
