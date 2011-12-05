//----------------------------------------------------------------------------
// stringtable_common.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "excel_private.h"
#include "filepaths.h"
#include "stringtable_common.h"
#include "unicode.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define STRING_TABLE_KEY_HASH_SIZE				4096
#define	STRING_TABLE_CODE_HASH_SIZE				256
#define NUM_STRINGS_PER_ALLOCATION_BLOCK		256

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define STRING_TABLE_HASH_KEY(key)				((key) % STRING_TABLE_KEY_HASH_SIZE)
#define STRING_TABLE_HASH_CODE(key)				((key) % STRING_TABLE_CODE_HASH_SIZE)

//----------------------------------------------------------------------------
// Attributes
//----------------------------------------------------------------------------
#define PLURAL_ATTRIBUTE_NAME					L"plural"
#define SINGULAR_ATTRIBUTE_NAME					L"singular"

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct STRING_ENTRY
{
	DWORD										m_dwAttributes;		// attributes of this string
	WCHAR *										m_puszText;			// the string itself
	struct STRING_ENTRY *						m_pNext;			// next string at this node (presumably with a different attributes and therefore a slightly different translation)
};

//----------------------------------------------------------------------------
struct STRING_TABLE_NODE
{
	STRING_ENTRY *								m_pStringEntries;
};

//----------------------------------------------------------------------------
struct STRING_TABLE_KEY_NODE
{
	char *										m_pszKey;
	STRING_TABLE_KEY_NODE *						m_pNext;
	int											m_nStringIndex;
	BYTE										bySourceStringFileIndex;					
	WORD 										wStringNumberInSourceFile;	
};

//----------------------------------------------------------------------------
struct STRING_TABLE_CODE_NODE
{
	DWORD										m_dwCode;
	STRING_TABLE_CODE_NODE *					m_pNext;
	int											m_nStringIndex;
};

//----------------------------------------------------------------------------
struct RANKED_STRING
{
	int nNumMatchedAttributes;
	const STRING_ENTRY *pStringEntry;
};

//----------------------------------------------------------------------------
struct STRING_ATTRIBUTE
{
	WCHAR uszAttributeName[ MAX_ATTRIBUTE_NAME_LENGTH ];	// name of attribute
	DWORD nBit;	// bit that represents this attribute
};

//----------------------------------------------------------------------------
struct STRING_TABLE
{	
	STRING_TABLE_NODE *							m_Strings;
	int											m_nCount;
	int											m_nMaxStrings;

	STRING_TABLE_KEY_NODE *						m_KeyHash[STRING_TABLE_KEY_HASH_SIZE];
	STRING_TABLE_CODE_NODE *					m_CodeHash[STRING_TABLE_CODE_HASH_SIZE];
	
	STRING_ATTRIBUTE tAttributes[ MAX_STRING_TABLE_ATTRIBUTES ];
	int nNumAttributes;

	int nStringCount[ MAX_STRING_SOURCE_FILES ];
	
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
static WCHAR guszUnknown[ MAX_STRING_ENTRY_LENGTH ];

//----------------------------------------------------------------------------
struct STRING_TABLE_GLOBALS
{
	STRING_TABLE *pStringTables;	// the string table(s)
	int nNumStringTables;
	int nDefaultTableIndex;
	
	WCHAR uszDefaultAttributeName[ MAX_ATTRIBUTE_NAME_LENGTH ];
		
};
static STRING_TABLE_GLOBALS sgtStringTableGlobals = { 0 };
static BOOL sgbInitialized = FALSE;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_GLOBALS *sStringTableGlobalsGet(
	void)
{
	return &sgtStringTableGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringTableInit(	
	STRING_TABLE *pTable)
{
	memclear( pTable, sizeof( STRING_TABLE ) );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE *sStringTableGetByIndex(
	int nIndex)
{
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	
	// get default index if none specified
	if (nIndex == INVALID_INDEX)
	{
		nIndex = pGlobals->nDefaultTableIndex;
	}

	// must have an index	
	ASSERTX_RETNULL( nIndex >= 0 && nIndex < pGlobals->nNumStringTables, "Invalid index for string table" );
	
	// return the string table
	return &pGlobals->pStringTables[ nIndex ];	
	
}

//----------------------------------------------------------------------------
// return a hash key for a string & the length of the string
//----------------------------------------------------------------------------
static inline DWORD sStringTableGetKey(
	const char * keystr,
	unsigned int & len)
{
	DWORD key = 0;
	const char * curr = keystr;
	while (*curr)
	{
		key = STR_HASH_ADDC(key, *curr);
		curr++;
	}
	len = (unsigned int)(curr - keystr);

	return STRING_TABLE_HASH_KEY(key);
}

//----------------------------------------------------------------------------
// return a hash key for a string
//----------------------------------------------------------------------------
static inline DWORD sStringTableGetKey(
	const char * keystr)
{
	unsigned int keylen = 0;
	return sStringTableGetKey(keystr, keylen);
}

//----------------------------------------------------------------------------
// find a string table entry given a key string
//----------------------------------------------------------------------------
static STRING_TABLE_KEY_NODE * sFindStringTableKeyNode(
	STRING_TABLE *pTable,
	const char * keystr,
	DWORD nKeyIndex)
{
	ASSERTV_RETNULL( pTable, "Expected string table" );
	ASSERT_RETNULL( nKeyIndex < STRING_TABLE_KEY_HASH_SIZE );
	
	STRING_TABLE_KEY_NODE * curr = NULL;
	STRING_TABLE_KEY_NODE * next = pTable->m_KeyHash[nKeyIndex];
	while ((curr = next) != NULL)
	{
		next = curr->m_pNext;
		if (PStrCmp(keystr, curr->m_pszKey) == 0)
		{
			return curr;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_KEY_NODE *sFindStringTableKeyNodeByStringIndex(
	STRING_TABLE *pTable,
	int nStringIndex)
{

	for (int i = 0; i < STRING_TABLE_KEY_HASH_SIZE; ++i)
	{
		STRING_TABLE_KEY_NODE *pKeyNode = pTable->m_KeyHash[ i ];
		while (pKeyNode)
		{
			if (pKeyNode->m_nStringIndex == nStringIndex)
			{
				return pKeyNode;
			}
			pKeyNode = pKeyNode->m_pNext;
		}
	}	
	return NULL;
}

//----------------------------------------------------------------------------
// find a string table entry given a code
//----------------------------------------------------------------------------
static STRING_TABLE_CODE_NODE * sStringTableFindByCode(
	STRING_TABLE *pTable,
	DWORD code,
	DWORD key)
{
	ASSERT_RETNULL(key < STRING_TABLE_CODE_HASH_SIZE);

	STRING_TABLE_CODE_NODE * curr = NULL;
	STRING_TABLE_CODE_NODE * next = pTable->m_CodeHash[key];
	while ((curr = next) != NULL)
	{
		next = curr->m_pNext;
		if (code == curr->m_dwCode)
		{
			return curr;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringEntryFree( 
	STRING_ENTRY *pStringEntry)
{
	ASSERTX_RETURN( pStringEntry, "Expected string entry" );
	
	if (pStringEntry->m_puszText)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE( g_StaticAllocator, pStringEntry->m_puszText );
#else	
		FREE( NULL, pStringEntry->m_puszText );
#endif
		pStringEntry->m_puszText = NULL;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableCommonFree(
	void)
{

	if (sgbInitialized == TRUE)
	{
		STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();	
			
		// free all string tables
		for (int i = 0; i < pGlobals->nNumStringTables; ++i)
		{	
			STRING_TABLE *pTable = sStringTableGetByIndex( i );
		
			for (int ii = 0; ii < STRING_TABLE_CODE_HASH_SIZE; ii++)
			{
				STRING_TABLE_CODE_NODE * curr = NULL;
				STRING_TABLE_CODE_NODE * next = pTable->m_CodeHash[ii];
				while ((curr = next) != NULL)
				{
					next = curr->m_pNext;
					FREE(NULL, curr);
				}
			}

			for (int ii = 0; ii < STRING_TABLE_KEY_HASH_SIZE; ii++)
			{
				STRING_TABLE_KEY_NODE * curr = NULL;
				STRING_TABLE_KEY_NODE * next = pTable->m_KeyHash[ii];
				while ((curr = next) != NULL)
				{
					next = curr->m_pNext;
					if (curr->m_pszKey)
					{
						FREE(g_StaticAllocator, curr->m_pszKey);						
					}
					FREE(g_StaticAllocator, curr);
				}
			}

			if (pTable->m_Strings)
			{
				STRING_TABLE_NODE * curr = pTable->m_Strings;
				for (int ii = 0; ii < pTable->m_nCount; ii++, curr++)
				{
					STRING_TABLE_NODE *pNode = &pTable->m_Strings[ ii ];
					
					while (pNode->m_pStringEntries)
					{
						STRING_ENTRY *pStringEntry = pNode->m_pStringEntries;
						
						// get next entry
						pNode->m_pStringEntries = pStringEntry->m_pNext;
						
						// free data for this entry 
						sStringEntryFree( pStringEntry );
						
						// free entry itself
						FREE( g_StaticAllocator, pStringEntry );
						
					}			
				}
				FREE(NULL, pTable->m_Strings);
			}

		}

		// free the string tables
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE( g_StaticAllocator, pGlobals->pStringTables );
#else		
		FREE( NULL, pGlobals->pStringTables );
#endif
		pGlobals->pStringTables = NULL;
		pGlobals->nNumStringTables = 0;
		
		// no longer initialzied
		sgbInitialized = FALSE;

	}
							
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------		
static BOOL sAddString(
	int nLanguage,
	const char *pszKey,
	DWORD dwCode,
	const WCHAR *puszString,
	const LANGUAGE_COLUMN_HEADER *pHeader,
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	int nStringFileIndex)
{	
	// add string to table
	STRING_TABLE_ADD_RESULT tResult;
	StringTableCommonAddString(nLanguage, pszKey, dwCode, puszString, pHeader->uszAttribute, pHeader->nNumAttributes, (DWORD)INVALID_STRING, &tResult, nStringFileIndex);
	
	// callback
	if (pfnStringAddedCallback)
	{
		// setup callback param
		ADDED_STRING tAdded;
		tAdded.puszString = puszString;
		tAdded.dwStringIndex = tResult.dwStringIndex;
		tAdded.pszKey = pszKey;
		tAdded.dwKeyIndex = tResult.dwKeyIndex;
		tAdded.dwCode = dwCode;
		tAdded.dwNumAttributes = pHeader->nNumAttributes;
		for (unsigned int i = 0; i < pHeader->nNumAttributes; ++i)
		{
			PStrCopy( tAdded.uszAttributes[ i ], pHeader->uszAttribute[ i ], MAX_ATTRIBUTE_NAME_LENGTH );
		}
				
		// do callback
		pfnStringAddedCallback( tAdded, pCallbackData );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadTabDelimitedData(
	int nStringTableIndex,
	const BYTE *pFileData,
	DWORD dwFileSize,
	int nStringFileIndex,  // if present
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	DWORD *pdwNumStringsLoaded,
	BOOL bValidateUnicodeHeader)	
{	
	ASSERTX_RETFALSE( pdwNumStringsLoaded, "Expected num strings loaded" );
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTX_RETFALSE( pTable, "Expected string table" );

	int nNumChars = 0;
	WCHAR *text = UnicodeFileGetEncoding( pFileData, dwFileSize, nNumChars, bValidateUnicodeHeader );
	if (text == NULL)
	{
		return FALSE;
	}

	// create an array representing what the columns in the string table are
	LANGUAGE_COLUMN_HEADER tHeaders[EXCEL_STRING_TABLE_MAX_LANGUAGES];
	structclear(tHeaders);

	unsigned int numHeaders = 0;
	unsigned int numColumnsInFile = 0;
	*pdwNumStringsLoaded = 0;
		
	WCHAR * cur = text;
	WCHAR * end = cur + nNumChars;
	BOOL bFirstLine = TRUE;
	while (cur < end && *cur)
	{
		WCHAR * prev = cur;
		REF(prev);

		WCHAR * key;
		if (ExcelGetStrTableGetToken(cur, key) != EXCEL_TOK_TOKEN)
		{
			break;
		}
		ASSERT(key);

		WCHAR * code;
		if (ExcelGetStrTableGetToken(cur, code) != EXCEL_TOK_TOKEN)
		{
			break;
		}

		WCHAR * strings[EXCEL_STRING_TABLE_MAX_LANGUAGES];
		ZeroMemory(strings, sizeof(strings));
		unsigned int col = 0;
		while (ExcelGetStrTableGetToken(cur, strings[col]) == EXCEL_TOK_TOKEN)
		{
			++col;
			if (col >= EXCEL_STRING_TABLE_MAX_LANGUAGES)
			{
				while (*cur && *cur != L'\n')
				{
					++cur;
				}
				if (*cur)
				{
					++cur;
				}
				break;
			}
		}

		if (bFirstLine)
		{
			for (unsigned int ii = 0; ii < col; ++ii)
			{
				++numColumnsInFile;
				LANGUAGE_COLUMN_HEADER * header = &tHeaders[numHeaders];
				if (ExcelStringTableLoadDirectAddHeader(header, strings[ii], ii) == TRUE)
				{
					++numHeaders;
				}				
			}
			bFirstLine = FALSE;

			// add each of the attributes to the string table
			for (unsigned int ii = 0; ii < numHeaders; ++ii)
			{
				LANGUAGE_COLUMN_HEADER * header = &tHeaders[ii];
								
				// add each of the attributes
				for (unsigned int jj = 0; jj < header->nNumAttributes; ++jj)
				{
					const WCHAR * puszAttributeName = header->uszAttribute[jj];
					
					// add to string table
					DWORD dwAttributeBit = StringTableCommonAddAttribute( nStringTableIndex, puszAttributeName );
					
					// add this attribute bit to the bit flags for this column
					SETBIT(header->dwAttributeFlags, dwAttributeBit);
				}
			}
		}
		else
		{
			if (key && key[0])
			{
				// get code
				char temp[5];
				if (code)
				{
					PStrCvt(temp, code, arrsize(temp));
				}
				else
				{
					temp[0] = 0;
				}
				DWORD dwCode = STRTOFOURCC(temp);
				
				// convert key to ascii
				char chKey[255];
				PStrCvt(chKey, key, arrsize(chKey));
								
				// add the string to the string table
				int nNumStringsAddedThisRow = 0;
				for (unsigned int ii = 0; ii < numHeaders; ++ii)
				{
					const LANGUAGE_COLUMN_HEADER * header = &tHeaders[ii];
					
					// get string and add if we have one
					ASSERT(header->nColumnIndex < arrsize(strings));
					const WCHAR * str = strings[header->nColumnIndex];
					if (str)
					{
						WCHAR* wstr = (WCHAR*)MALLOCZ(NULL, EXCEL_MAX_FIELD_LEN);
						AUTOFREE tAutoFree( NULL, wstr );
						WCHAR *s = wstr;
						while (*str)
						{
							if (str[0] == L'\\' && str[1] == L'n')
							{
								*s = L'\n';
								++s;
								str += 2;
							}
							else if (str[0] == L'"' && str[1] == L'"')
							{
								// when exporting tab delimited text from excel, excel will automatically
								// enclose some strings with quotes ... which means that any quotes we
								// have in the string itself are exported using the excel escape
								// sequence of two double quote ("") ... we want to remove the duplicate
								// double quote and make it a single double quote
								*s = L'"';
								++s;
								str += 2;							
							}
							else
							{
								*s = *str;
								++s;
								++str;
							}

						}
						*s = 0;

						if (wstr && wstr[0])
						{
							if (sAddString(nStringTableIndex, chKey, dwCode, wstr, header, pfnStringAddedCallback, pCallbackData, nStringFileIndex) == FALSE)
							{
								return FALSE;
							}
							*pdwNumStringsLoaded = *pdwNumStringsLoaded + 1;
							nNumStringsAddedThisRow++;
						}
					}
				}

				// if no string, and this is the first column, we force there to be at
				// least a blank string for this string label ... we need to do this
				// because some languages intentionally leave some strings blank
				// (like word wrap characters), but the want to be sure all languages
				// have at least the same number of strings loaded cause it makes
				// making the SKUs and patches easier				
				if (nNumStringsAddedThisRow == 0)
				{
					LANGUAGE_COLUMN_HEADER *pHeader = &tHeaders[ 0 ];
					if (sAddString(nStringTableIndex, chKey, dwCode, L"", pHeader, pfnStringAddedCallback, pCallbackData, nStringFileIndex) == FALSE)
					{
						return FALSE;
					}
					*pdwNumStringsLoaded = *pdwNumStringsLoaded + 1;
					nNumStringsAddedThisRow++;				
				}
			}
		}
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL STC_LoadTabDelimitedResource(
	int nStringTableIndex,
	const BYTE *pFileData,
	DWORD dwFileSize,
	int nStringFileIndex,
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	DWORD *pdwNumStringsLoaded,
	BOOL bValidateUnicodeHeader)
{
	ASSERTX_RETFALSE( pFileData, "Expected file data" );

	// load data from memory	
	return sLoadTabDelimitedData( 
		nStringTableIndex, 
		pFileData, 
		dwFileSize,
		nStringFileIndex, 
		pfnStringAddedCallback,
		pCallbackData,
		pdwNumStringsLoaded,
		bValidateUnicodeHeader);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableCommonLoadTabDelimitedFile(
	int nStringTableIndex,
	const WCHAR *uszFilePath,
	int nStringFileIndex,  // if present
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	DWORD *pdwNumStringsLoaded)	
{
	ASSERTX_RETFALSE( uszFilePath, "Expected file path" );
	ASSERTX_RETFALSE( pdwNumStringsLoaded, "Expected count" );

	// load file data
	DWORD dwFileSize;
	BYTE *pFileData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE( NULL, uszFilePath, &dwFileSize ));
	if (pFileData == NULL)
	{
		return FALSE;
	}
	
	// setup auto free on the file data
	AUTOFREE tAutoFree( NULL, pFileData );

	// load data from memory	
	return STC_LoadTabDelimitedResource(
		nStringTableIndex, 
		pFileData, 
		dwFileSize,
		nStringFileIndex, 
		pfnStringAddedCallback,
		pCallbackData,
		pdwNumStringsLoaded,
		TRUE);

}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableCommonInit(
	int nMaxTables,
	int nDefaultTableIndex,
	const WCHAR *puszDefaultAttributeName /*= NULL*/)
{

	// if initialized, shutdown
	if (sgbInitialized)
	{
		StringTableCommonFree();
	}

	// must be unitiallized
	ASSERTX( sgbInitialized == FALSE, "String library already initialized!" );
		
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	
	// allocate enough string tables to hold all the tables
	pGlobals->nNumStringTables = nMaxTables;
	pGlobals->nDefaultTableIndex = nDefaultTableIndex;
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	pGlobals->pStringTables = (STRING_TABLE *)MALLOCZ( g_StaticAllocator, sizeof( STRING_TABLE ) * pGlobals->nNumStringTables );
#else	
	pGlobals->pStringTables = (STRING_TABLE *)MALLOCZ( NULL, sizeof( STRING_TABLE ) * pGlobals->nNumStringTables );
#endif

	// save default attribute name
	if (puszDefaultAttributeName)
	{
		PStrCopy( pGlobals->uszDefaultAttributeName, puszDefaultAttributeName, MAX_ATTRIBUTE_NAME_LENGTH );
	}
		
	// init string tables
	for (int i = 0; i < pGlobals->nNumStringTables; ++i)
	{
		STRING_TABLE *pTable = sStringTableGetByIndex( i );
		sStringTableInit( pTable );
	}
	
	// we are now initialized
	sgbInitialized = TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_NODE *sStringTableGetNode(
	STRING_TABLE *pTable,
	const char *pszKey,
	int *pnStringIndex)
{
	
	// transform key to index code
	DWORD nKeyIndex = sStringTableGetKey( pszKey );
	STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNode( pTable, pszKey, nKeyIndex );
	
	// get the string table node
	if (pKeyNode)
	{

		// save index if present
		if (pnStringIndex)
		{
			*pnStringIndex = pKeyNode->m_nStringIndex;
		}
					
		// return the node
		return &pTable->m_Strings[ pKeyNode->m_nStringIndex ];	
		
	}
	
	// not found
	return NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_KEY_NODE *sAddKeyNode(
	STRING_TABLE *pTable,
	const char *pszKey,
	int nSourceStringFileIndex)
{
	
	// allocate node
	STRING_TABLE_KEY_NODE *pKeyNode = (STRING_TABLE_KEY_NODE *)MALLOCZ( g_StaticAllocator, sizeof( STRING_TABLE_KEY_NODE ) );

	// get key
	unsigned int nKeyLen = 0;
	DWORD nKeyIndex = sStringTableGetKey( pszKey, nKeyLen );
	
	// allocate space for the key
	int nBufferChars = nKeyLen + 1;
	int nBufferSize = nBufferChars * sizeof( pszKey[ 0 ] );
	pKeyNode->m_pszKey = (char *)MALLOCZ( g_StaticAllocator, nBufferSize );
	
	// save the key string in the node
	PStrCopy( pKeyNode->m_pszKey, pszKey, nBufferChars );
	
	// hook into key hash
	pKeyNode->m_pNext = pTable->m_KeyHash[ nKeyIndex ];
	pTable->m_KeyHash[ nKeyIndex ] = pKeyNode;

	// init node index
	pKeyNode->m_nStringIndex = INVALID_STRING;

	// if we're tracking the source file counts, do so now
	if (nSourceStringFileIndex >= 0 && nSourceStringFileIndex < MAX_STRING_SOURCE_FILES)
	{
		pKeyNode->bySourceStringFileIndex = (BYTE)nSourceStringFileIndex;
		pKeyNode->wStringNumberInSourceFile = (WORD)pTable->nStringCount[ nSourceStringFileIndex ];
		pTable->nStringCount[ nSourceStringFileIndex ]++;
	}
	else
	{
		pKeyNode->bySourceStringFileIndex = 0;
		pKeyNode->wStringNumberInSourceFile = 0;
	}
	
	// return the new node
	return pKeyNode;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_CODE_NODE *sAddCodeNode( 
	STRING_TABLE *pTable, 
	DWORD dwCode,
	int nStringIndex)
{
	
	DWORD nCodeIndex = STRING_TABLE_HASH_CODE( dwCode );
	STRING_TABLE_CODE_NODE *pCodeNode = sStringTableFindByCode( pTable, dwCode, nCodeIndex );
	if (!pCodeNode)
	{
	
		// allocate new node
		pCodeNode = (STRING_TABLE_CODE_NODE *)MALLOCZ( NULL, sizeof( STRING_TABLE_CODE_NODE ) );
		
		// save code data
		pCodeNode->m_dwCode = dwCode;
		
		// hook into hash
		pCodeNode->m_pNext = pTable->m_CodeHash[ nCodeIndex ];
		pTable->m_CodeHash[ nCodeIndex ] = pCodeNode;
		
	}
	
	// set the string index
	pCodeNode->m_nStringIndex = nStringIndex;

	// return the node
	return pCodeNode;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_TABLE_NODE *sStringTableAddNode( 
	STRING_TABLE *pTable,
	const char *pszKey,
	int nAtSpecificIndex,
	int *pnStringIndex,
	int nSourceStringFileIndex)
{
	
	// add a key node
	STRING_TABLE_KEY_NODE *pKeyNode = sAddKeyNode( pTable, pszKey, nSourceStringFileIndex );

	// what index will we add the string at
	int nStringIndex = nAtSpecificIndex;
	if (nStringIndex == INVALID_STRING)
	{
		// add as last string in table
		nStringIndex = pTable->m_nCount;
	}
	
	// reallocate the string nodes if we don't have any more storage for the nodes
	if (nStringIndex >= pTable->m_nMaxStrings)
	{
	
		// what will the new size be
		while (pTable->m_nMaxStrings <= nStringIndex)
		{
			pTable->m_nMaxStrings += NUM_STRINGS_PER_ALLOCATION_BLOCK;
		}
		
		// what is the new allocation size
		int nNewAllocSize = pTable->m_nMaxStrings * sizeof( STRING_TABLE_NODE );
		
		// realloc to that size
		pTable->m_Strings = (STRING_TABLE_NODE *)REALLOC( NULL, pTable->m_Strings, nNewAllocSize);
										
	}

	// set the new count of allocated nodes in the string table
	pTable->m_nCount = MAX( pTable->m_nCount, nStringIndex + 1 );
	
	// link the key node to this new index
	pKeyNode->m_nStringIndex = nStringIndex;
	
	// get node and init
	STRING_TABLE_NODE *pNode = &pTable->m_Strings[ nStringIndex ];
	pNode->m_pStringEntries = NULL;

	// save index if present
	if (pnStringIndex)
	{
		*pnStringIndex = nStringIndex;
	}
			
	// return the node
	return pNode;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STRING_ENTRY *sNewStringEntryAtNode(
	STRING_TABLE_NODE *pNode,
	const WCHAR *puszString,
	DWORD dwAttributes,
	int *pBufferSize)
{	

	// allocate a new entry
	STRING_ENTRY *pStringEntry = (STRING_ENTRY *)MALLOCZ( g_StaticAllocator, sizeof( STRING_ENTRY ) );

	// set the string text data
	int nStringLength = PStrLen( puszString );
	int nBufferChars = nStringLength + 1;
	int nBufferSize = nBufferChars * sizeof( puszString[ 0 ] );
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	pStringEntry->m_puszText = (WCHAR *)MALLOCZ( g_StaticAllocator, nBufferSize );
#else	
	pStringEntry->m_puszText = (WCHAR *)MALLOCZ( NULL, nBufferSize );
#endif
	
	PStrCopy( pStringEntry->m_puszText, puszString, nBufferChars );
	
	// save attribute flags
	pStringEntry->m_dwAttributes = dwAttributes;
		
	// tie this string entry to the node
	pStringEntry->m_pNext = pNode->m_pStringEntries;
	pNode->m_pStringEntries = pStringEntry;

	// save buffer size if desired
	if (pBufferSize)
	{
		*pBufferSize = nBufferSize;
	}
	
	// return the new string entry
	return pStringEntry;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sAddAttributesToTable( 
	int nStringTableIndex,
	const WCHAR puszAttributes[ MAX_STRING_TABLE_ATTRIBUTES ][ MAX_ATTRIBUTE_NAME_LENGTH ],
	int nNumAttributes)
{
	DWORD dwAttributeFlags = 0;
	
	// add each attribute
	for (int i = 0; i < nNumAttributes; ++i)
	{
		SETBIT( dwAttributeFlags, StringTableCommonAddAttribute( nStringTableIndex, puszAttributes[ i ] ) );
	}
	
	return dwAttributeFlags;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableCommonIterateStringsAtKey(
	int nTableIndex,
	const char *pszKey,
	PFN_STRING_ENTRY_CALLBACK pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pszKey, "Expected key" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	STRING_TABLE *pTable = sStringTableGetByIndex( nTableIndex );
	ASSERTX_RETURN( pTable, "Expected table" );
	
	// get key node
	DWORD nKeyIndex = sStringTableGetKey( pszKey );	
	STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNode( pTable, pszKey, nKeyIndex );
	ASSERTX_RETURN( pKeyNode, "Unable to find string table key node" );

	// get string entry
	const STRING_TABLE_NODE *pNode = &pTable->m_Strings[ pKeyNode->m_nStringIndex ];
		
	// go through entries	
	STRING_ENTRY *pStringEntry = pNode->m_pStringEntries;
	while (pStringEntry)
	{
	
		// setup result param
		STRING_ENTRY_INFO tEntryInfo;		
		tEntryInfo.puszString = pStringEntry->m_puszText;
		tEntryInfo.nStringIndex = pKeyNode->m_nStringIndex;
		tEntryInfo.pszKey = pszKey;
		tEntryInfo.nKeyIndex = nKeyIndex;
		
		// call callback
		pfnCallback( tEntryInfo, pCallbackData );	
						
		// go to next entry
		pStringEntry = pStringEntry->m_pNext;
	}
}


//----------------------------------------------------------------------------
// return the index for a given keystr
//----------------------------------------------------------------------------
int StringTableCommonGetStringIndexByKey(
	int nStringTableIndex,
	const char * keystr)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTV_RETINVALID( pTable, "Expected string table" );
	DWORD keyidx = sStringTableGetKey(keystr);
	
	STRING_TABLE_KEY_NODE * keynode = sFindStringTableKeyNode( pTable, keystr, keyidx );
	if (!keynode)
	{
		return EXCEL_LINK_INVALID;
	}
	return keynode->m_nStringIndex;
}

//----------------------------------------------------------------------------
// return the source file index for a given keystr
//----------------------------------------------------------------------------
int StringTableCommonGetStringSourceFileIndexByKey(
	int nStringTableIndex,
	const char * keystr)
{

	if (keystr == NULL)
	{
		return INVALID_INDEX;
	}
	
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTV_RETINVALID( pTable, "Expected string table" );
	DWORD keyidx = sStringTableGetKey(keystr);
	
	STRING_TABLE_KEY_NODE * keynode = sFindStringTableKeyNode( pTable, keystr, keyidx );
	if (!keynode)
	{
		return INVALID_INDEX;
	}
	return keynode->bySourceStringFileIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRankString( 
	const STRING_ENTRY *pStringEntry, 
	DWORD dwAttributes, 
	RANKED_STRING *pRankedString,
	DWORD dwStringFlags)
{

	// assign ranked string data
	pRankedString->pStringEntry = pStringEntry;
	pRankedString->nNumMatchedAttributes = 0;

	// check for exact attribute matching
	if (TESTBIT( dwStringFlags, SF_ATTRIBUTE_MATCH_EXACT_BIT ))
	{
		if (pStringEntry->m_dwAttributes != dwAttributes)
		{
			return FALSE;  // cannot accept string
		}
	}
			
	// walk all the bits set
	if (dwAttributes != 0)
	{
		const int MAX_BITS = 32;
		ASSERTX_RETFALSE( MAX_STRING_TABLE_ATTRIBUTES <= MAX_BITS, "Too many string attributes to fit in the packed attribute format" );
			
		// walk the bits of the attributes we're searching for
		int nNumAttributesToCheck = 0;
		for (int i = 0; i < MAX_BITS; ++i)
		{
			if (TESTBIT( dwAttributes, i ))
			{
				nNumAttributesToCheck++;
				if (TESTBIT( pStringEntry->m_dwAttributes, i ))
				{
					pRankedString->nNumMatchedAttributes++;
				}			
			}
		}
		
		// check for when we must match all of the attributes we're looking for
		if (TESTBIT( dwStringFlags, SF_ATTRIBUTE_MATCH_AT_LEAST_BIT ))
		{
			if (pRankedString->nNumMatchedAttributes < nNumAttributesToCheck)
			{
				return FALSE;  // cannot accept string
			}
		}
		
	}

	return TRUE;  // OK to accept string
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRankStrings( 
	const STRING_TABLE_NODE *pNode, 
	RANKED_STRING *pRankedStrings, 
	int nMaxRankedStrings, 
	DWORD dwAttributes,
	DWORD dwStringFlags)
{
	int nCount = 0;
	
	// go through all the strings at this node
	STRING_ENTRY *pStringEntry = pNode->m_pStringEntries;
	while (pStringEntry)
	{
	
		// add this string to the array of ranked strings
		ASSERTX_BREAK( nCount < nMaxRankedStrings - 1, "Too many strings at string node for ranking" );
		RANKED_STRING *pRankedString = &pRankedStrings[ nCount ];  // don't increment count until we can accpet the string
		
		// rank this string
		if (sRankString( pStringEntry, dwAttributes, pRankedString, dwStringFlags ))
		{
			// we can accept this string
			nCount++;
		}
		
		// on to the next string
		pStringEntry = pStringEntry->m_pNext;
		
	}

	return nCount;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringAttributeInit( 
	STRING_ATTRIBUTE *pAttribute)
{
	pAttribute->uszAttributeName[ 0 ] = 0;
	pAttribute->nBit = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const STRING_ATTRIBUTE *sStringTableGetAttribute( 
	STRING_TABLE *pTable,
	const WCHAR *puszAttribute,
	BOOL bCreateIfNotFound)
{
	ASSERTX_RETNULL( pTable, "Expected table" );

	// search for existing attribute	
	for (int i = 0; i < pTable->nNumAttributes; ++i)
	{
		const STRING_ATTRIBUTE *pAttribute = &pTable->tAttributes[ i ];
		
		if (PStrICmp( pAttribute->uszAttributeName, puszAttribute ) == 0)
		{
			return pAttribute;
		}
		
	}

	if (bCreateIfNotFound)
	{
	
		// create new attribute
		ASSERTX_RETNULL( pTable->nNumAttributes < MAX_STRING_TABLE_ATTRIBUTES - 1, "Too many attributes for string table" ); 
		STRING_ATTRIBUTE *pAttribute = &pTable->tAttributes[ pTable->nNumAttributes++ ];		
		sStringAttributeInit( pAttribute );

		// assign name
		PStrCopy( pAttribute->uszAttributeName, puszAttribute, MAX_ATTRIBUTE_NAME_LENGTH );
		
		// assign bit
		pAttribute->nBit = pTable->nNumAttributes - 1;

		// return the new attribute
		return pAttribute;

	}

	return NULL;  // not found
				
}	

//----------------------------------------------------------------------------
// return a string by index, flags are used to indicate gender, number etc.
//----------------------------------------------------------------------------
static inline const WCHAR * sStringTableGetStringByIndex(
	STRING_TABLE *pTable,
	int nIndex,
	DWORD dwAttributesToMatch,
	DWORD dwStringFlags,
	DWORD *pdwAttributesOfString,
	BOOL *pbFound)
{		
	ASSERTX_RETNULL( pTable, "Expected table" );
	
	// check for out of range
	if (pTable == NULL || nIndex < 0 || nIndex >= pTable->m_nCount)	
	{
		goto _unknown_index;
	}

	// string exists
	if (pbFound)
	{
		*pbFound = TRUE;
	}

	// if all we want is the label, find it now
	if (TESTBIT( dwStringFlags, SF_RETURN_LABEL_BIT ))
	{
		STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNodeByStringIndex( pTable, nIndex );
		if (pKeyNode)
		{
			static WCHAR uszKey[ MAX_STRING_ENTRY_LENGTH ];
			PStrCvt( uszKey, pKeyNode->m_pszKey, MAX_STRING_ENTRY_LENGTH );
			return uszKey;			
		}
	}
	
	// get string node
	const STRING_TABLE_NODE *pNode = &pTable->m_Strings[ nIndex ];

	// create storage for the strings we can select from
	RANKED_STRING tRankedStrings[ MAX_STRING_TABLE_ATTRIBUTES ];
	int nNumStrings = 0;

	// we will never try to match the default attribute
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();	
	const STRING_ATTRIBUTE *pDefaultAttribute = sStringTableGetAttribute( 
		pTable, 
		pGlobals->uszDefaultAttributeName, 
		FALSE);
	if (pDefaultAttribute)
	{
		CLEARBIT( dwAttributesToMatch, pDefaultAttribute->nBit );
	}	

	// rank and sort all the strings here in order of most matched attributes
	nNumStrings = sRankStrings( 
		pNode, 
		tRankedStrings, 
		MAX_STRING_TABLE_ATTRIBUTES, 
		dwAttributesToMatch, 
		dwStringFlags );
		
	// if we were unable to match any string given our criteria, bail out
	if (nNumStrings == 0)
	{
		goto _unknown_index;
	}
	
	// find the highest ranked string
	const RANKED_STRING *pTopString = NULL;
	for (int i = 0; i < nNumStrings; ++i)
	{
		const RANKED_STRING *pRankedString = &tRankedStrings[ i ];
		if (pTopString == NULL)
		{
			if (pRankedString->nNumMatchedAttributes > 0)
			{
				pTopString = pRankedString;
			}
		}
		else if (pRankedString->nNumMatchedAttributes > pTopString->nNumMatchedAttributes)
		{
			// has more matching attributes
			pTopString = pRankedString;
		}
		else if (pRankedString->nNumMatchedAttributes == pTopString->nNumMatchedAttributes) 
		{ 
			// matches the same number of attributes, if the new one found has the "default"  
			// attribute on it, we will prefer that one  
			if (pDefaultAttribute && TESTBIT( pRankedString->pStringEntry->m_dwAttributes, pDefaultAttribute->nBit ))  
			{ 
				pTopString = pRankedString;  
			} 
		}
	}

	// if no ranked string found, see if there is a string with the default attribute on it
	if (pTopString == NULL)
	{					
		if (pDefaultAttribute)
		{			
			for (int i = 0; i < nNumStrings; ++i)
			{
				const RANKED_STRING *pRankedString = &tRankedStrings[ i ];
				if (TESTBIT( pRankedString->pStringEntry->m_dwAttributes, pDefaultAttribute->nBit ))
				{
					pTopString = pRankedString;
					break;
				}
			}
		}
	}
	
	// if still no string, use the last string (the last string was added first
	// to the table and in excel would appear as the first column)
	if (pTopString == NULL)
	{
		pTopString = &tRankedStrings[ nNumStrings - 1 ];
	}

	// save the attributes of the string used
	if (pdwAttributesOfString)
	{
		*pdwAttributesOfString = pTopString->pStringEntry->m_dwAttributes;
	}
	
	// return string
	ASSERTX_RETNULL( pTopString, "Unable to pick a top string at this string node" );
	const WCHAR *puszText = pTopString->pStringEntry->m_puszText;
	
	return puszText;
							
_unknown_index:

	// string does not exist
	if (pbFound)
	{
		*pbFound = FALSE;
	}
	
	PStrPrintf( guszUnknown, MAX_STRING_ENTRY_LENGTH, L"MISSING STRING INDEX '%d'", nIndex );
	return guszUnknown;
			
}

//----------------------------------------------------------------------------
// return a string by index
//----------------------------------------------------------------------------
const WCHAR * StringTableCommonGetStringByIndex(
	int nStringTableIndex,
	int nStringIndex)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	return sStringTableGetStringByIndex( pTable, nStringIndex, 0, 0, NULL, NULL );
}	

//----------------------------------------------------------------------------
// return a string by index using matching attributes etc
//----------------------------------------------------------------------------
const WCHAR * StringTableCommonGetStringByIndexVerbose(
	int nStringTableIndex,
	int nStringIndex,
	DWORD dwAttributesToMatch /*= 0*/,
	DWORD dwStringFlags,
	DWORD *pdwAttributesOfString /*= NULL*/,
	BOOL *pbFound /*= NULL*/)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	
	return sStringTableGetStringByIndex(
		pTable,
		nStringIndex, 
		dwAttributesToMatch, 
		dwStringFlags,
		pdwAttributesOfString, 
		pbFound);
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sStringTableGetStringByKey(
	const char *pszKey,
	int nStringTableIndex)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTX_RETNULL( pTable, "Expected string table" );
	
	DWORD nKeyIndex = sStringTableGetKey( pszKey );
	STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNode( pTable, pszKey, nKeyIndex );
	if (!pKeyNode)
	{
		PStrPrintf( guszUnknown, MAX_STRING_ENTRY_LENGTH, L"MISSING STRING '%S'", pszKey );
		return guszUnknown;	
	}
		
	return sStringTableGetStringByIndex( pTable, pKeyNode->m_nStringIndex, 0, 0, NULL, NULL );
}

//----------------------------------------------------------------------------
// return a string by key lookup
//----------------------------------------------------------------------------
const WCHAR *StringTableCommonGetStringByKey(
	int nStringTableIndex,
	const char *pszKey)
{	
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERT_RETNULL( pTable );
	return sStringTableGetStringByKey( pszKey, nStringTableIndex );	
}

//----------------------------------------------------------------------------
// return a string by key lookup with verbose info
//----------------------------------------------------------------------------
const WCHAR *StringTableCommonGetStringByKeyVerbose(
	int nStringTableIndex,
	const char *pszKey,
	DWORD dwAttributesToMatch,
	DWORD dwStringFlags,
	DWORD *pdwAttributesOfString,
	BOOL *pbFound)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	
	// find key node
	DWORD nKeyIndex = sStringTableGetKey( pszKey );
	STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNode( pTable, pszKey, nKeyIndex );
	if (!pKeyNode)
	{
		PStrPrintf( guszUnknown, MAX_STRING_ENTRY_LENGTH, L"MISSING STRING '%S'", pszKey );
		if (pbFound)
		{
			*pbFound = FALSE;
		}
		return guszUnknown;	
	}

	// find by index		
	return StringTableCommonGetStringByIndexVerbose(
		nStringTableIndex,
		pKeyNode->m_nStringIndex,
		dwAttributesToMatch,
		dwStringFlags,
		pdwAttributesOfString,
		pbFound);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *StringTableCommonGetKeyByStringIndex(
	int nStringTableIndex,
	int nStringIndex)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	const STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNodeByStringIndex( pTable, nStringIndex );
	if (pKeyNode)
	{
		return pKeyNode->m_pszKey;
	}
	static char *szUnknown = "UNKNOWN STRING INDEX";
	return szUnknown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *StringTableCommonGetKeyByKeyIndex(
	int nKeyIndex,
	int nTableIndex)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nTableIndex );
	if (pTable)
	{
		const STRING_TABLE_KEY_NODE *pKeyNode = sFindStringTableKeyNodeByStringIndex( pTable, nKeyIndex );
		if (pKeyNode)
		{
			return pKeyNode->m_pszKey;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableCommonAddString(
	int nStringTableIndex,
	const char *pszKey,
	DWORD dwCode,
	const WCHAR *puszString,
	const WCHAR puszAttributes[ MAX_STRING_TABLE_ATTRIBUTES ][ MAX_ATTRIBUTE_NAME_LENGTH ],
	DWORD dwNumAttributes,
	DWORD dwAddAtSpecificIndex /*= INVALID_STRING*/,
	STRING_TABLE_ADD_RESULT *pResult /*= NULL*/,
	int nSourceStringFileIndex /*= INVALID_LINK*/)
{
	ASSERTX_RETURN( pszKey && pszKey[ 0 ], "Invalid string key");
	ASSERTX_RETURN( puszString, "Invalid string text" );

	const WCHAR * puszStringToUse = puszString;
			
	// get the string table
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	
	// add attributes
	DWORD dwAttributeFlags = sAddAttributesToTable( nStringTableIndex, puszAttributes, dwNumAttributes );
	
	// get existing node (if present)
	int nStringIndex = INVALID_STRING;
	BOOL bNewNode = FALSE;
	STRING_TABLE_NODE *pNode = sStringTableGetNode( pTable, pszKey, &nStringIndex );
	if (pNode == NULL)
	{
	
		// node is not here, add a new node
		pNode = sStringTableAddNode( pTable, pszKey, dwAddAtSpecificIndex, &nStringIndex, nSourceStringFileIndex );
		bNewNode = TRUE;
		
	}
	ASSERTX_RETURN( pNode, "Expected string table node" );
	ASSERTX_RETURN( nStringIndex != INVALID_STRING, "Expected string index" );
	
	// add a new entry at this string node for this string
	int nBufferSize = 0;
	STRING_ENTRY *pStringEntry = sNewStringEntryAtNode( 
		pNode, 
		puszStringToUse, 
		dwAttributeFlags, 
		&nBufferSize );
	ASSERTX_RETURN( pStringEntry, "Expected string entry" );
	
	// add code node if code is present
	if (bNewNode == TRUE && dwCode != 0)
	{
		sAddCodeNode( pTable, dwCode, nStringIndex );
	}
	
	// put results in param if requested
	if (pResult)
	{
		pResult->dwStringIndex = nStringIndex;
		pResult->dwKeyIndex = sStringTableGetKey( pszKey );
	}
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableCommonAddAttribute(
	int nStringTableIndex,
	const WCHAR *puszAttributeName)
{
	ASSERTX_RETFALSE( puszAttributeName, "Expected attribute name" );
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
			
	// find or create this attribute in the string table
	const STRING_ATTRIBUTE *pAttribute = 
		sStringTableGetAttribute( pTable, puszAttributeName, TRUE );		
	ASSERTX_RETINVALID( pAttribute, "Unable to find or create string table attribute" );
	
	// return the bit representing this attribute
	return pAttribute->nBit;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableCommonGetAttributeBit(
	int nStringTableIndex,
	const WCHAR *puszAttributeName,
	BOOL *pbFound)
{
	ASSERTX_RETZERO( puszAttributeName, "Expected attribute name" );
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTX_RETZERO( pTable, "Expected string table" );
	ASSERTX_RETZERO( pbFound, "Expected found param" );
			
	// find or create this attribute in the string table
	const STRING_ATTRIBUTE *pAttribute = sStringTableGetAttribute( pTable, puszAttributeName, FALSE );		
	if (pAttribute)
	{
		*pbFound = TRUE;
		return pAttribute->nBit;
	}
	
	// not found
	*pbFound = FALSE;
	
	return 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StringTableCommonGetCount(
	int nStringTableIndex)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTX_RETZERO( pTable, "Expected string table" );
	return pTable->m_nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableCommonIterateKeys(
	int nStringTableIndex,
	int nSourceStringFileIndex,
	PFN_STRING_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData)
{
	STRING_TABLE *pTable = sStringTableGetByIndex( nStringTableIndex );
	ASSERTX_RETURN( pTable, "Expected string table" );
	ASSERTX_RETURN( pfnCallback, "Invalid callback" );
	
	// go through all keys
	for (int i = 0; i < STRING_TABLE_KEY_HASH_SIZE; ++i)
	{
		STRING_TABLE_KEY_NODE *pKeyNode = pTable->m_KeyHash[ i ];
		while (pKeyNode)
		{
		
			// must be in the source file we care about
			if (pKeyNode->bySourceStringFileIndex == nSourceStringFileIndex)
			{
				// call callback
				pfnCallback( pKeyNode->m_pszKey, pKeyNode->wStringNumberInSourceFile, pCallbackData );
			}
						
			// go to next node
			pKeyNode = pKeyNode->m_pNext;
		}
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableCommonGetGrammarNumberAttribute(
	int nStringTableIndex,
	GRAMMAR_NUMBER eNumber)
{	
	const WCHAR *puszAttributeName = eNumber == PLURAL ? PLURAL_ATTRIBUTE_NAME : SINGULAR_ATTRIBUTE_NAME;
	
	// get bit used to represent attribute
	BOOL bFound = FALSE;
	int nBit = StringTableCommonGetAttributeBit( nStringTableIndex, puszAttributeName, &bFound );
	
	// return as a bit mask
	DWORD dwAttributeFlags = 0;		
	if (bFound == TRUE)
	{
		SETBIT( dwAttributeFlags, nBit );
	}
	return dwAttributeFlags;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *StringTableCommonGetStringFileGetExcelPath(
	BOOL bIsCommon)
{
	if (bIsCommon)
	{
		return FILE_PATH_EXCEL_COMMON;
	}
	return FILE_PATH_EXCEL;
}

//----------------------------------------------------------------------------
static const unsigned int MAX_STR_TABLE_ENTRY_LEN = 32768;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableCommonWriteStringToPak(
	const ADDED_STRING &tAdded,
	WRITE_BUF_DYNAMIC *pCookedFile)
{
	// setup byte buffer to write string to
	const int COOK_DATA_BUFFER = 128;  // not accurate, just enough to hold all the binary cooked data
	BYTE tStorage[MAX_STR_TABLE_ENTRY_LEN + COOK_DATA_BUFFER];
	BYTE_BUF tByteBuffer(tStorage, arrsize(tStorage));

	// string and key index
	ASSERT(tByteBuffer.PushUInt(tAdded.dwStringIndex, sizeof(DWORD)));
	ASSERT(tByteBuffer.PushUInt(tAdded.dwKeyIndex, sizeof(DWORD)));

	// key		
	DWORD dwKeyLen = PStrLen(tAdded.pszKey);		
	ASSERT(tByteBuffer.PushUInt(dwKeyLen, sizeof(DWORD)));
	ASSERT(tByteBuffer.PushBuf(tAdded.pszKey, dwKeyLen + 1));
	
	// code
	ASSERT(tByteBuffer.PushUInt(tAdded.dwCode, sizeof(DWORD)));

	// string data		
	int nStringLength = PStrLen(tAdded.puszString);
	int nBufferChars = nStringLength + 1;
	DWORD dwBufferSize = nBufferChars * sizeof(tAdded.puszString[0]);
	ASSERT(tByteBuffer.PushUInt(dwBufferSize, sizeof(DWORD)));
	ASSERT(tByteBuffer.PushBuf(tAdded.puszString, dwBufferSize));
	
	// attributes
	ASSERT(tByteBuffer.PushUInt(tAdded.dwNumAttributes, sizeof(DWORD)));
	for (unsigned int i = 0; i < tAdded.dwNumAttributes; ++i)
	{
		const WCHAR *puszAttributeName = tAdded.uszAttributes[i];
		DWORD dwLen = PStrLen(puszAttributeName);
		ASSERT(tByteBuffer.PushUInt(dwLen, sizeof(DWORD)));			
		DWORD dwBufferSize2 = (dwLen + 1)* sizeof(puszAttributeName[0]);
		ASSERT(tByteBuffer.PushBuf(puszAttributeName, dwBufferSize2));						
	}

	// write to cooked file
	ASSERT_RETFALSE(pCookedFile->PushBuf(tStorage, tByteBuffer.GetLen()));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableCommonReadStringFromPak(
	int nLangugage,
	int nStringFileIndex,
	BYTE_BUF & buf)
{
	DWORD dwStringIndex;
	ASSERT_RETFALSE(buf.PopBuf( &dwStringIndex, sizeof( DWORD ) ) );
	
	DWORD dwKeyIndex;
	ASSERT_RETFALSE( buf.PopBuf( &dwKeyIndex, sizeof( DWORD  ) ) );

	DWORD dwKeyLen;
	ASSERT_RETFALSE( buf.PopBuf( &dwKeyLen, sizeof( DWORD ) ) );

	char szKey[ MAX_STRING_ENTRY_LENGTH ];
	ASSERT_RETFALSE( buf.PopBuf( szKey, dwKeyLen + 1) );

	DWORD dwCode;
	ASSERT_RETFALSE( buf.PopBuf( &dwCode, sizeof( DWORD ) ) );

	DWORD dwDataLen;
	ASSERT_RETFALSE( buf.PopBuf( &dwDataLen, sizeof( DWORD ) ) );

	WCHAR uszString[ MAX_STRING_ENTRY_LENGTH ];
	ASSERT_RETFALSE( buf.PopBuf( uszString, dwDataLen ) );

	WCHAR uszAttributes[ MAX_STRING_TABLE_ATTRIBUTES ][ MAX_ATTRIBUTE_NAME_LENGTH ];
	DWORD dwNumAttributes;
	ASSERT_RETFALSE(buf.PopBuf( &dwNumAttributes, sizeof( DWORD ) ) );
	for (unsigned int i = 0; i < dwNumAttributes; ++i)
	{
		DWORD dwLen;
		ASSERT_RETFALSE(buf.PopBuf( &dwLen, sizeof( DWORD ) ) );		
		WCHAR *puszAttributeName = uszAttributes[ i ];
		DWORD dwBufferSize = (dwLen + 1) * sizeof( puszAttributeName[ 0 ] );
		ASSERT_RETFALSE( buf.PopBuf( puszAttributeName, dwBufferSize ) );		
	}
		
	StringTableCommonAddString( 
		nLangugage,
		szKey, 
		dwCode, 
		uszString, 
		uszAttributes,
		dwNumAttributes,
		dwStringIndex, 
		NULL,
		nStringFileIndex);	

	return TRUE;
	
}
