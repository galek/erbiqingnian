//----------------------------------------------------------------------------
// debugbars_priv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DEBUGBARS_PRIV_H__
#define __DEBUGBARS_PRIV_H__

#include "debugbars.h"

#define INCLUDE_DEBUGBARS_ENUM
enum DEBUG_BARS_SET
{
	DEBUG_BARS_NONE = -1,
#include "debugbars_def.h"
	// count
	NUM_DEBUG_BARS_SETS
};
#undef INCLUDE_DEBUGBARS_ENUM


struct DEBUG_BAR_SET_DEFINITION
{
	char szName[ UI_BAR_SET_NAME_MAX_LEN ];
	PFN_UPDATE_UI_BAR_SET pfnUpdate;
};


BOOL ExcelDebugBarsPostProcess(
	EXCEL_TABLE * table);

void c_SetUIBarsSet( int nBarsSet );
void c_UpdateUIBars();

void SetUIBarCallback(
	int nBarIndex,
	const char * szName,
	int nValue,
	int nMax,
	int nGood,
	int nBad );


#endif // __DEBUGBARS_PRIV_H__