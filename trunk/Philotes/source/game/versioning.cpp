//----------------------------------------------------------------------------
// versioning.cpp
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "versioning.h"
#include "units.h"
#include "script.h"
#include "excel_private.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sUnitVersionProps(
	GAME * pGame,
	UNIT * pUnit,
	int nUnitVersion)
{
	GENUS eUnitGenus = UnitGetGenus(pUnit);

	int nCount = ExcelGetNumRows(pGame, DATATABLE_VERSIONINGUNITS);
	for(int i=0; i<nCount; i++)
	{
		VERSIONING_UNITS * pVersioning = (VERSIONING_UNITS *)ExcelGetData(pGame, DATATABLE_VERSIONINGUNITS, i);
		if(!pVersioning)
			continue;

		ASSERT_CONTINUE(pVersioning->nUnitVersion > 0);

		if(pVersioning->nUnitVersion < nUnitVersion)
			continue;

		GENUS eVersioningGenus = ExcelGetGenusByDatatable(pVersioning->eTable);
		if(eVersioningGenus == GENUS_NONE || eVersioningGenus != eUnitGenus)
			continue;

		ASSERT_CONTINUE(pVersioning->nUnitTypeInclude != INVALID_ID || pVersioning->nUnitClass != INVALID_ID);

		if(pVersioning->nUnitTypeInclude != INVALID_ID &&
			!UnitIsA(pUnit, pVersioning->nUnitTypeInclude))
			continue;

		if(pVersioning->nUnitTypeExclude != INVALID_ID &&
			UnitIsA(pUnit, pVersioning->nUnitTypeExclude))
			continue;

		if(pVersioning->nUnitClass != INVALID_ID &&
			UnitGetClass(pUnit) != pVersioning->nUnitClass)
			continue;

		int nCodeLength = 0;
		BYTE * pCode = ExcelGetScriptCode(pGame, DATATABLE_VERSIONINGUNITS, pVersioning->codeScript, &nCodeLength);
		if(pCode && nCodeLength)
		{
			VMExecI(pGame, pUnit, pCode, nCodeLength);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sUnitVersionAffixes(
	GAME * pGame,
	UNIT * pUnit,
	int nUnitVersion)
{
	int nCount = ExcelGetNumRows(pGame, DATATABLE_VERSIONINGAFFIXES);
	for(int i=0; i<nCount; i++)
	{
		VERSIONING_AFFIXES * pVersioning = (VERSIONING_AFFIXES *)ExcelGetData(pGame, DATATABLE_VERSIONINGAFFIXES, i);
		if(!pVersioning)
			continue;

		ASSERT_CONTINUE(pVersioning->nUnitVersion > 0);
		ASSERT_CONTINUE(pVersioning->nAffix >= 0);

		if(pVersioning->nUnitVersion < nUnitVersion)
			continue;

		int nCodeLength = 0;
		BYTE * pCode = ExcelGetScriptCode(pGame, DATATABLE_VERSIONINGAFFIXES, pVersioning->codeScript, &nCodeLength);
		if(pCode && nCodeLength)
		{
			STATS * pAffixStats = StatsGetModList(pUnit, SELECTOR_AFFIX);
			while(pAffixStats)
			{
				int nStatsAffixId = StatsGetStat(pAffixStats, STATS_AFFIX_ID);
				if(nStatsAffixId == pVersioning->nAffix)
				{
					VMExecI(pGame, pUnit, pAffixStats, pCode, nCodeLength, NULL);
				}
				pAffixStats = StatsGetModNext(pAffixStats, SELECTOR_AFFIX);
			}
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitVersionProps(
	UNIT * pUnit)
{
	if(!AppIsHellgate())
		return TRUE;

	ASSERT_RETFALSE(pUnit);

	GAME * pGame = UnitGetGame(pUnit);
	ASSERT_RETFALSE(pGame);

	int nUnitVersion = UnitGetVersion(pUnit);

	if(!s_sUnitVersionProps(pGame, pUnit, nUnitVersion))
		return FALSE;

	if(!s_sUnitVersionAffixes(pGame, pUnit, nUnitVersion))
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL VersioningUnitsExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	int nCount = ExcelGetCountPrivate(table);
	for(int i=0; i<nCount; i++)
	{
		VERSIONING_UNITS * pVersioning = (VERSIONING_UNITS *)ExcelGetDataPrivate(table, i);
		if(!pVersioning)
			continue;

		if(!pVersioning->pszUnitClass)
			continue;

		ASSERT_CONTINUE(pVersioning->eTable != DATATABLE_NONE);

		pVersioning->nUnitClass = ExcelGetLineByStringIndex(NULL, pVersioning->eTable, pVersioning->pszUnitClass);
	}
	return TRUE;
}
