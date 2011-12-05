//-------------------------------------------------------------------------------------------------
//
// Hellgate Colorset v1.0
//
// by Guy Somberg
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
// Colorset Management Functions
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "definition.h"
#include "colorset.h"
#include "stringhashkey.h"
#include "prime.h"
#include "unittypes.h"
#include "excel_private.h"
#include "units.h"


//-------------------------------------------------------------------------------------------------
// GLBOALS
//-------------------------------------------------------------------------------------------------
static COLOR_DEFINITION tColorDefinitionDefault = { -1, UNITTYPE_ANY, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, NULL };


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL ColorSetsExcelPostProcess(
	EXCEL_TABLE * table)
{
	if (AppIsTugboat())
	{
		return FALSE;
	}

	ASSERT_RETFALSE(table);

	ColorSetClose();

	COLOR_SET_DEFINITION * pColorSetDef = (COLOR_SET_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_COLOR_SET, "colorsets");
	ASSERT_RETFALSE(pColorSetDef);

	for (int i = 0; i < pColorSetDef->nColorDefinitionCount; i++)
	{
		COLOR_DEFINITION * pColorDef = &pColorSetDef->pColorDefinitions[i];
		if ( pColorDef->nId == INVALID_ID )
			continue;

		COLORSET_DATA * colorset_data = (COLORSET_DATA *)ExcelGetDataPrivate(table, pColorDef->nId);
		ASSERT_CONTINUE(colorset_data);

		pColorDef->pNext = colorset_data->pFirstDef;
		colorset_data->pFirstDef = pColorDef;
	}

	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ColorSetClose(
	void)
{
	EXCEL_TABLE * table = ExcelGetTableNotThreadSafe(DATATABLE_COLORSETS);
	ASSERT_RETURN(table);

	COLOR_SET_DEFINITION * pColorSetDef = (COLOR_SET_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_COLOR_SET, "colorsets");
	ASSERT_RETURN(pColorSetDef);

	for(int i=0; i<pColorSetDef->nColorDefinitionCount; i++)
	{
		COLOR_DEFINITION * pColorDef = &pColorSetDef->pColorDefinitions[i];
		pColorDef->pNext = NULL;
	}

	unsigned int nNumRows = ExcelGetCountPrivate(table);
	ASSERTX_RETURN(nNumRows > 0, "Color set data is missing.");

	for (unsigned int ii = 0; ii < nNumRows; ++ii)
	{
		COLORSET_DATA * colorset_data = (COLORSET_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETURN(colorset_data);
		colorset_data->pFirstDef = NULL;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL ColorSetDefinitionPostXMLLoad(void * pDef, BOOL bForceSyncLoad)
{
	if (AppIsHammer())
	{
		ColorSetsExcelPostProcess(ExcelGetTableNotThreadSafe(DATATABLE_COLORSETS));
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int ColorSetPickRandom(
	UNIT * pUnit )
{
	int nNumColorsets = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_COLORSETS );
	if( nNumColorsets == 0 )
	{
		return INVALID_ID;
	}
	if ( pUnit )
	{
		int nPickByUnitId = pUnit->unitid % nNumColorsets;
		const COLORSET_DATA * pColorset = (const COLORSET_DATA *) ExcelGetData( EXCEL_CONTEXT(), DATATABLE_COLORSETS, nPickByUnitId );
		if ( pColorset && pColorset->bCanBeRandomPick )
			return nPickByUnitId;
	}

	RAND tRand;
	RandInit( tRand );

	for ( int i = 0; i < 100; i++ )
	{
		int nPickByRoll = RandGetNum( tRand, nNumColorsets );
		const COLORSET_DATA * pColorset = (const COLORSET_DATA *) ExcelGetData( EXCEL_CONTEXT(), DATATABLE_COLORSETS, nPickByRoll );
		if ( pColorset && pColorset->bCanBeRandomPick )
			return nPickByRoll;
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const COLOR_DEFINITION * ColorSetGetColorDefinition(
	int nIndex, 
	int nUnittype)
{
	if ( nIndex == INVALID_ID )
		return &tColorDefinitionDefault;

	if ( nUnittype == INVALID_ID )
		nUnittype = UNITTYPE_ANY;

	COLORSET_DATA * pData = (COLORSET_DATA *) ExcelGetData( NULL, DATATABLE_COLORSETS, nIndex );
	if ( ! pData )
		return &tColorDefinitionDefault;

	COLOR_DEFINITION * pColorDefBest = NULL;
	COLOR_DEFINITION * pColorDef = pData->pFirstDef;
	while ( pColorDef )
	{
		if ( pColorDef->nUnittype == INVALID_ID || UnitIsA( nUnittype, pColorDef->nUnittype ) )
		{
			if ( !pColorDefBest || pColorDefBest->nUnittype == INVALID_ID || !UnitIsA( pColorDefBest->nUnittype, pColorDef->nUnittype ) )
				pColorDefBest = pColorDef;
		}
		pColorDef = pColorDef->pNext;
	}

	//if ( AppIsHammer() )
	//{
	//	WARNX( pColorDefBest, pData->szColor );
	//}

	return pColorDefBest ? pColorDefBest : &tColorDefinitionDefault;
}

