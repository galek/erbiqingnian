//----------------------------------------------------------------------------
// FILE: globalindex.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "globalindex.h"
#include "stringtable.h"
#include "appcommon.h"
#include "excel.h"
#include "excel_private.h"


//----------------------------------------------------------------------------
// globalindex.txt, globalstring.txt
//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(GLOBAL_INDEX_ENTRY)
	EF_STRING(		"name",									pszName																		)
	EF_STRING(		"string",								pszString																	)
	EF_STRING(		"string_tugboat",						pszTugboatString															)
	EF_LINK_TABLE(	"datatable",							eDataTable																	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_POSTPROCESS_ALL(GlobalIndexExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_GLOBAL_INDEX, GLOBAL_INDEX_ENTRY, APP_GAME_ALL, EXCEL_TABLE_SHARED, "globalindex") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_GLOBAL_STRING, GLOBAL_INDEX_ENTRY,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "globalstring") EXCEL_TABLE_END


//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Local Data
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const GLOBAL_INDEX_ENTRY *sGlobalIndexEntryFind(
	GLOBAL_INDEX eGlobalIndex )
{
	ASSERT_RETNULL((unsigned int)eGlobalIndex < ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_GLOBAL_INDEX));
	return (const GLOBAL_INDEX_ENTRY *)ExcelGetData(NULL, DATATABLE_GLOBAL_INDEX, eGlobalIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sGlobalIndexHasString(
	const GLOBAL_INDEX_ENTRY *pGlobalIndexEntry)
{
	if(!pGlobalIndexEntry)
		return FALSE;

	if(AppIsHellgate())
	{
		if(pGlobalIndexEntry->pszString[0])
		{
			return TRUE;
		}
		return FALSE;
	}
	
	if(AppIsTugboat())
	{
		if(pGlobalIndexEntry->pszTugboatString[0])
		{
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GlobalIndexGet(
	GLOBAL_INDEX eGlobalIndex)
{
	int nIndex = INVALID_LINK;

	if (eGlobalIndex != GI_INVALID)
	{
		const GLOBAL_INDEX_ENTRY *pGlobalIndexEntry = sGlobalIndexEntryFind(eGlobalIndex);
		if(pGlobalIndexEntry)
		{
			nIndex = pGlobalIndexEntry->nIndex;
		}

		// validate we found something if the string is not blank
		if (nIndex == INVALID_LINK && sGlobalIndexHasString(pGlobalIndexEntry))
		{
			//const int MAX_MESSAGE = 1024;
			//char szMessage[ MAX_MESSAGE ];
			//PStrPrintf( 
			//	szMessage, 
			//	MAX_MESSAGE, 
			//	"Unable to find global index entry for #%d (%s)\n",
			//	eGlobalIndex,
			//	pGlobalIndexEntry ? pGlobalIndexEntry->pszName : "UNKNOWN");

			//ASSERTX( nIndex != INVALID_LINK, szMessage );
		}
	}

	return nIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *GlobalIndexGetName(
	GLOBAL_INDEX eGlobalIndex)
{
	if (eGlobalIndex != GI_INVALID)
	{
		const GLOBAL_INDEX_ENTRY *pGlobalIndexEntry = sGlobalIndexEntryFind(eGlobalIndex);
		if(pGlobalIndexEntry)
		{
			return pGlobalIndexEntry->pszName;
		}
	}

	return "INVALID INDEX";
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCELTABLE GlobalIndexGetDatatable(
	GLOBAL_INDEX eGlobalIndex)
{
	EXCELTABLE eTable = DATATABLE_NONE;
	
	if (eGlobalIndex != GI_INVALID)
	{
		const GLOBAL_INDEX_ENTRY *pGlobalIndexEntry = sGlobalIndexEntryFind( eGlobalIndex );
		if (pGlobalIndexEntry)
		{
			eTable = pGlobalIndexEntry->eDataTable;
		}
			
		if (eTable == DATATABLE_NONE)
		{
			const int MAX_MESSAGE = 1024;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Unable to find global index entry for #%d (%s)\n",
				eGlobalIndex,
				pGlobalIndexEntry ? pGlobalIndexEntry->pszName : "UNKNOWN");
				
			ASSERTX( eTable != DATATABLE_NONE, szMessage );		
		}
	}
	
	return eTable;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GlobalIndexValidate(
	void)
{
	for (unsigned int ii = 0; ii < GI_NUM_GLOBAL_INDEX; ++ii)
	{
		GLOBAL_INDEX eGlobalIndex = (GLOBAL_INDEX)ii;
		GlobalIndexGet(eGlobalIndex);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const GLOBAL_INDEX_ENTRY *sGlobalStringEntryFind( 
	GLOBAL_STRING eGlobalString)
{
	ASSERT_RETNULL((unsigned int)eGlobalString < ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_GLOBAL_STRING));
	return (const GLOBAL_INDEX_ENTRY *)ExcelGetData(NULL, DATATABLE_GLOBAL_STRING, eGlobalString);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * GlobalStringGetEx(
	GLOBAL_STRING eGlobalString,
	const WCHAR * pDefault)
{
	const GLOBAL_INDEX_ENTRY * pGlobalStringEntry = NULL;
	if ( eGlobalString != GS_INVALID )
		pGlobalStringEntry = sGlobalStringEntryFind( eGlobalString );
	if (pGlobalStringEntry && pGlobalStringEntry->puszString)
	{
		return pGlobalStringEntry->puszString;
	}
	return pDefault;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *GlobalStringGet(
	GLOBAL_STRING eGlobalString)
{
	const WCHAR * pMsg = GlobalStringGetEx(eGlobalString, 0);
	if (pMsg != 0)
	{
		return pMsg;
	}

	const int MAX_MESSAGE = 1024;
	static WCHAR uszMessage[ MAX_MESSAGE ];
	PStrPrintf( uszMessage, MAX_MESSAGE, L"Unknown global string '%d'\n", eGlobalString );
	return uszMessage;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char *sGlobalIndexEntryGetStringValue(
	const GLOBAL_INDEX_ENTRY *pGlobalStringEntry)
{			
	if (AppIsTugboat() && pGlobalStringEntry->pszTugboatString[ 0 ] != 0)
	{
		return pGlobalStringEntry->pszTugboatString;
	}
	else
	{
		return pGlobalStringEntry->pszString;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *GlobalStringGetKey(
	GLOBAL_STRING eGlobalString)
{
	const GLOBAL_INDEX_ENTRY *pGlobalStringEntry = sGlobalStringEntryFind( eGlobalString );
	if (pGlobalStringEntry)
	{
		return sGlobalIndexEntryGetStringValue( pGlobalStringEntry );
	}
	else
	{
		const int MAX_MESSAGE = 1024;
		static char szMessage[ MAX_MESSAGE ];
		PStrPrintf( szMessage, MAX_MESSAGE, "Unknown global string '%d'\n", eGlobalString );
		return szMessage;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GlobalStringGetStringIndex(
	GLOBAL_STRING eGlobalString)
{
	const GLOBAL_INDEX_ENTRY *pGlobalStringEntry = sGlobalStringEntryFind( eGlobalString );
	if (pGlobalStringEntry)
	{
		const char *pszKey = sGlobalIndexEntryGetStringValue( pGlobalStringEntry );
		return StringTableCommonGetStringIndexByKey( INVALID_INDEX, pszKey );
	}
	return INVALID_LINK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGlobalIndexExcelPostProcess(
	EXCEL_TABLE * table)
{
	unsigned int count = ExcelGetCountPrivate(table);
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		GLOBAL_INDEX_ENTRY * index_entry = (GLOBAL_INDEX_ENTRY *)ExcelGetDataPrivate(table, ii);
		if ( ! index_entry )
			continue;
		if (index_entry->eDataTable == DATATABLE_NONE)
		{
			ASSERTV_CONTINUE(0, "Index entry %s has a bad datatable", index_entry->pszName);
		}
		
		const char * pszStringToUse = sGlobalIndexEntryGetStringValue(index_entry);
		ASSERT((unsigned int)index_entry->eDataTable != table->m_Index);
		index_entry->nIndex = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), index_entry->eDataTable, pszStringToUse);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GlobalStringCacheStrings( 
	void)
{
	unsigned int count = ExcelGetNumRows( NULL, DATATABLE_GLOBAL_STRING );
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		GLOBAL_INDEX_ENTRY * index_entry = (GLOBAL_INDEX_ENTRY *)ExcelGetData( NULL, DATATABLE_GLOBAL_STRING, ii );
		if (index_entry)
		{
			const char * pszStringToUse = sGlobalIndexEntryGetStringValue(index_entry);
			index_entry->puszString = StringTableCommonGetStringByKeyVerbose( 
				INVALID_INDEX,
				pszStringToUse,
				0,
				0,
				NULL,
				NULL);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
const WCHAR * sGetGlobalString(unsigned nString)
{
	ASSERT_RETNULL((0 <= nString) && (nString < GS_NUM_GLOBAL_STRING));
	return GlobalStringGetEx(static_cast<GLOBAL_STRING>(nString), 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GlobalIndexExcelPostProcess( 
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	if (table->m_Index == DATATABLE_GLOBAL_INDEX)
	{
		return sGlobalIndexExcelPostProcess(table);
	}
	else if (table->m_Index == DATATABLE_GLOBAL_STRING)
	{
		AppCommonSetGlobalStringCallback(&sGetGlobalString);
		return GlobalStringCacheStrings();
	}
	ASSERT_RETFALSE(0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INT_PTR GlobalStringMessageBox(
	HWND hWnd,
	GLOBAL_STRING eText,
	GLOBAL_STRING eCaption,
	UINT uType,
	...)
{
	const WCHAR * pText = GlobalStringGetEx(eText, 0);
	const WCHAR * pCaption = GlobalStringGetEx(eCaption, L"Application Message");
	WCHAR szBuf[4096];
	if (pText == 0)
	{
		PStrPrintf(
			szBuf,
			_countof(szBuf),
			L"The application encountered an issue but was unable to retrieve the localized string describing the issue. (%d)",
			eText
		);
	}
	else
	{
		va_list args;
		va_start(args, uType);
		PStrVprintf(
			szBuf,
			_countof(szBuf),
			pText,
			args
		);
	}
	return ::MessageBoxW(hWnd, szBuf, pCaption, uType);
}
