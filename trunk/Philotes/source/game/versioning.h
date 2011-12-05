//----------------------------------------------------------------------------
// versioning.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma once
#endif
#ifdef	_VERSIONING_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit (slows down compile, so fix this if you get a chance!!!")
#else
#define _VERSIONING_H_

struct VERSIONING_UNITS
{
	int			nUnitVersion;
	enum EXCELTABLE	eTable;
	int			nUnitTypeInclude;
	int			nUnitTypeExclude;
	EXCEL_STR	(pszUnitClass);
	int			nUnitClass;
	PCODE		codeScript;
};

struct VERSIONING_AFFIXES
{
	int			nUnitVersion;
	int			nAffix;
	PCODE		codeScript;
};

BOOL s_UnitVersionProps(
	struct UNIT * pUnit);

BOOL VersioningUnitsExcelPostProcess(
	EXCEL_TABLE * table);

#endif