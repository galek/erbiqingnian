//----------------------------------------------------------------------------
// unitidmap.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNITIDMAP_H_
#define _UNITIDMAP_H_



//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
struct UNIT_ID_MAP* UnitIdMapInit(
	struct GAME* game);

void UnitIdMapFree(
	UNIT_ID_MAP* idmap);

void UnitIdMapReset(
	UNIT_ID_MAP* idmap,
	BOOL bSortOnInsert = TRUE);

int UnitIdMapGetOrAdd(
	UNIT_ID_MAP* idmap,
	UNITID idUnit);

int UnitIdMapFindByUnitID(
	UNIT_ID_MAP* idmap,
	UNITID idUnit);

UNITID UnitIdMapFindByAlias(
	UNIT_ID_MAP* idmap,
	int nAlias);

void UnitIdMapRemove(
	UNIT_ID_MAP* idmap,
	UNITID idUnit);

void UnitIdMapRemove(
	UNIT_ID_MAP* idmap,
	int nAlias);

int UnitIdMapGetCount(
	UNIT_ID_MAP* idmap);


#endif // _UNITIDMAP_H_
