//----------------------------------------------------------------------------
// c_xfirestats.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_XFIRESTATS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _XFIRESTATS_H_

enum XFIRESTATS_TYPE
{
	XFIRESTATS_PLAYER_NAME,
	XFIRESTATS_LEVEL_NAME,
	XFIRESTATS_LEVEL,
	XFIRESTATS_HP_CUR,
	XFIRESTATS_HP_MAX_BONUS,
	XFIRESTATS_POWER_CUR,
	XFIRESTATS_POWER_MAX_BONUS,
	XFIRESTATS_EXPERIENCE
};

void XfireStatsSetUnitStat(int statid, int value);
void XfireStatsSetString(XFIRESTATS_TYPE type, const WCHAR *value);
void XfireStateUpdateUnitState(UNIT *unit);

#endif
