//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#ifndef __E_INITDB__
#define __E_INITDB__

struct INITDB_TABLE
{
	typedef int CRITERIA_TYPE;

	static const CRITERIA_TYPE INVALID_RANGE = -1;
	static const DWORD INVALID_FOURCC = 0;

	static const int nNumKnobNameMaxLen = 64;

	BOOL bSkip;
	DWORD nCriteria;
	CRITERIA_TYPE nRangeLow;
	CRITERIA_TYPE nRangeHigh;
	float fNumMin;
	float fNumMax;
	float fNumInit;
	DWORD nFeatKnob;
	DWORD nFeatMin;
	DWORD nFeatMax;
	DWORD nFeatInit;
	char szNumKnob[nNumKnobNameMaxLen];
};

PRESULT e_InitDB_Init(void);

#if ISVERSION(DEVELOPMENT)
#define E_INITDB_DEVELOPMENT 1
#endif

#if E_INITDB_DEVELOPMENT
void e_InitDB_SetDumpToFile(bool bFlag);
void e_InitDB_SetIgnoreDB(bool bFlag);
#endif

#endif
