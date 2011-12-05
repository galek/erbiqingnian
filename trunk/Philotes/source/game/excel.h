//----------------------------------------------------------------------------
// excel.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#define _EXCEL_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _EXCEL_COMMON_H_
#include "excel_common.h"
#endif

#ifndef _DATATABLES_H_
#include "datatables.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL ExcelInit(
	const EXCEL_LOAD_MANIFEST & manifest);

BOOL ExcelLoadData(
	void);

BOOL ExcelLoadDatatable(
	EXCELTABLE eTable);

EXCELTABLE ExcelGetDatatableByGenus(
	GENUS eGenus);

GENUS ExcelGetGenusByDatatable(
	EXCELTABLE eTable);

BOOL ExcelStringTablesLoadAll(
	void);

GENUS GlobalIndexGetUnitGenus(
	enum GLOBAL_INDEX eGlobalIndex);
	

#endif // _EXCEL_H_
