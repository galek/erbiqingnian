//----------------------------------------------------------------------------
// stringtable_common.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.

//----------------------------------------------------------------------------
#ifndef _STRINGTABLE_COMMON_H_
#define _STRINGTABLE_COMMON_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum STRING_TABLE_CONSTANTS
{
	STRING_TABLE_DEFAULT		= 0,
	MAX_STRING_ENTRY_LENGTH		= 32 * 1024,	// arbitrary
	MAX_STRING_KEY_LENGTH		= 256,			// arbitrary
	MAX_ATTRIBUTE_NAME_LENGTH	= 24,
	MAX_STRING_TABLE_ATTRIBUTES = 32,	
	STRING_TABLE_INDEX_SIZE		= 64,
	MAX_STRING_SOURCE_FILES		= 128,	  // arbitrary, increase if needed	
};

#define INVALID_STRING (-1)

//----------------------------------------------------------------------------
struct LANGUAGE_COLUMN_HEADER
{
	WCHAR				uszAttribute[MAX_STRING_TABLE_ATTRIBUTES][MAX_ATTRIBUTE_NAME_LENGTH];
	unsigned int		nNumAttributes;
	unsigned int		nColumnIndex;
	DWORD				dwAttributeFlags;
};
	
//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
	
void StringTableCommonInit(
	int nMaxTables,
	int nDefaultTableIndex,
	const WCHAR *puszDefaultAttributeName = NULL);

void StringTableCommonFree(
	void);

//----------------------------------------------------------------------------
struct ADDED_STRING
{
	const WCHAR *puszString;
	DWORD dwStringIndex;
	const char *pszKey;
	DWORD dwKeyIndex;
	DWORD dwCode;
	WCHAR uszAttributes[ MAX_STRING_TABLE_ATTRIBUTES ][ MAX_ATTRIBUTE_NAME_LENGTH ];
	DWORD dwNumAttributes;
};

//----------------------------------------------------------------------------
typedef void (* PFN_STRING_ADDED_CALLBACK)( const ADDED_STRING &tAdded, void *pCallbackData );
	
BOOL StringTableCommonLoadTabDelimitedFile(
	int nStringTableIndex,
	const WCHAR *uszFilePath,
	int nStringFileIndex,
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	DWORD *pdwNumStringsLoaded);

BOOL STC_LoadTabDelimitedResource(
	int nStringTableIndex,
	const BYTE *pFileData,
	DWORD dwFileSize,
	int nStringFileIndex,
	PFN_STRING_ADDED_CALLBACK pfnStringAddedCallback,
	void *pCallbackData,
	DWORD *pdwNumStringsLoaded,
	BOOL bValidateUnicodeHeader);
			
int StringTableCommonGetCount(
	int nStringTableIndex);
	
//----------------------------------------------------------------------------
struct STRING_TABLE_ADD_RESULT
{
	DWORD dwStringIndex;			// index string was added to in table
	DWORD dwKeyIndex;				// index of key node for string
};
	
void StringTableCommonAddString(
	int nStringTableIndex,
	const char *pszKey,
	DWORD dwCode,
	const WCHAR *puszString,
	const WCHAR puszAttributes[ MAX_STRING_TABLE_ATTRIBUTES ][ MAX_ATTRIBUTE_NAME_LENGTH ],
	DWORD dwNumAttributes,
	DWORD dwAddAtSpecificIndex = INVALID_STRING,
	STRING_TABLE_ADD_RESULT *pResult = NULL,
	int nStringFileIndex = INVALID_LINK);		// excel index of source file this string came from

//----------------------------------------------------------------------------
struct STRING_ENTRY_INFO
{
	const WCHAR *puszString;
	int nStringIndex;
	const char *pszKey;
	int nKeyIndex;
};

//----------------------------------------------------------------------------
typedef void (* PFN_STRING_ENTRY_CALLBACK)( const STRING_ENTRY_INFO &pEntryInfo, void *pCallbackData );

void StringTableCommonIterateStringsAtKey(
	int nTableIndex,
	const char *pszKey,
	PFN_STRING_ENTRY_CALLBACK pfnCallback,
	void *pCallbackData);
	
int StringTableCommonGetStringIndexByKey(
	int nStringTableIndex,
	const char * keystr);

int StringTableCommonGetStringSourceFileIndexByKey(
	int nStringTableIndex,
	const char * keystr);

const WCHAR * StringTableCommonGetStringByIndex(
	int nStringTableIndex,
	int nStringIndex);

//----------------------------------------------------------------------------
enum STRING_FLAGS
{
	SF_ATTRIBUTE_MATCH_AT_LEAST_BIT,	// only strings that have *at least* all of the attributes passed in will be considered
	SF_ATTRIBUTE_MATCH_EXACT_BIT,		// only strings with an exact attribute match of the attribs passed in will be considered
	SF_RETURN_LABEL_BIT,				// return label instead of string
};

const WCHAR * StringTableCommonGetStringByIndexVerbose(
	int nStringTableIndex,
	int nStringIndex,
	DWORD dwAttributesToMatch /*= 0*/,
	DWORD dwStringFlags,					// see STRING_FLAGS
	DWORD *pdwAttributesOfString /*= NULL*/,
	BOOL *pbFound /*= NULL*/);

const WCHAR * StringTableCommonGetStringByKey(
	int nStringTableIndex,
	const char *pszKey);

const WCHAR * StringTableCommonGetStringByKeyVerbose(
	int nStringTableIndex,
	const char *pszKey,
	DWORD dwAttributesToMatch,
	DWORD dwStringFlags,					// see STRING_FLAGS
	DWORD *pdwAttributesOfString,
	BOOL *pbFound);

const char *StringTableCommonGetKeyByStringIndex(
	int nStringTableIndex,
	int nStringIndex);

const char *StringTableCommonGetKeyByKeyIndex(
	int nKeyIndex,
	int nTableIndex);

DWORD StringTableCommonAddAttribute(
	int nStringTableIndex,
	const WCHAR *puszAttributeName);

DWORD StringTableCommonGetAttributeBit(
	int nStringTableIndex,
	const WCHAR *puszAttributeName,
	BOOL *pbFound);

typedef void (* PFN_STRING_ITERATE_CALLBACK)( const char *pszKey, WORD wStringNumberInSourceFile, void *pCallbackData );

void StringTableCommonIterateKeys(
	int nStringTableIndex,
	int nSourceStringFileIndex,
	PFN_STRING_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData);

DWORD StringTableCommonGetGrammarNumberAttribute(
	int nStringTableIndex,
	GRAMMAR_NUMBER eNumber);

const char *StringTableCommonGetStringFileGetExcelPath(
	BOOL bIsCommon);
					
BOOL StringTableCommonWriteStringToPak(
	const ADDED_STRING &tAdded,
	WRITE_BUF_DYNAMIC *pCookedFile);

BOOL StringTableCommonReadStringFromPak(
	int nLanguage,
	int nStringFileIndex,
	class BYTE_BUF & buf);
	
#endif // _STRINGTABLE_COMMON_H_