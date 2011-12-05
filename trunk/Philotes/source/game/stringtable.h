//----------------------------------------------------------------------------
// stringtable.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _STRINGTABLE_H_
#define _STRINGTABLE_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stringtable_common.h"

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct DATA_TABLE;
enum GRAMMAR_NUMBER;
enum LANGUAGE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
#define STRING_TABLE_MAGIC					'hfst'
#define STRING_TABLE_VERSION				(5 + GLOBAL_FILE_VERSION)

#define CREDIT_ATTRIBUTE_SECTION				L"section"
#define CREDIT_ATTRIBUTE_TITLE					L"title"
#define CREDIT_ATTRIBUTE_CREDIT					L"credit"

#define ATTRIBUTE_POSSESSIVE					L"possessive"

//----------------------------------------------------------------------------
struct STRING_FILE_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];		// string filename
	BOOL bIsCommon;
	BOOL bLoadedByGame;
	BOOL bCreditsFile;
};

//----------------------------------------------------------------------------
enum STRING_TABLE_ENUM
{
	STE_INVALID = -1,
	
	STE_DEFAULT,			// string table for the client language
	STE_DEVELOPER,		// string table of the developer language (English)
	
	STE_MAX_STRING_TABLES
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

const STRING_FILE_DEFINITION *StringFileDefinitionGet(
	int nStringFile);

BOOL StringTableInitForApp(
	void);
	
void StringTableInit(
	APP_GAME eAppGame);

void StringTableFree(
	void);
	
int StringTableGetStringIndexByKey(
	const char * keystr,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

const WCHAR * StringTableGetStringByIndex(
	int nStringIndex,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

const WCHAR * StringTableGetStringByIndexVerbose(
	int nStringIndex,
	DWORD dwAttributesToMatch /*= 0*/,
	DWORD dwStringFlags,					// see STRING_FLAGS
	DWORD *pdwAttributesOfString /*= NULL*/,
	BOOL *pbFound /*= NULL*/,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

const WCHAR * StringTableGetStringByKey(
	const char *pszKey,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

const WCHAR * StringTableGetStringByKeyVerbose(
	const char *pszKey,
	DWORD dwAttributesToMatch,
	DWORD dwStringFlags,					// see STRING_FLAGS
	DWORD *pdwAttributesOfString,
	BOOL *pbFound,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

const char *StringTableGetKeyByStringIndex(
	int nStringIndex,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

BOOL StringTablesAreAllCurrent(
	void);
	
enum EXCEL_UPDATE_STATE StringTablesLoadAll(
	void);
	
BOOL StringTablesLoad(
	BOOL bForceDirect,
	BOOL bAddToPak,
	int nSKU);

BOOL StringTablesFillPak(
	int nSKU);
	
void StringTableTouchSourceFiles(
	LANGUAGE eLanguage);

DWORD StringTableAddAttribute(
	LANGUAGE eLanguage,
	const WCHAR *puszAttributeName);

DWORD StringTableGetGrammarNumberAttribute(
	GRAMMAR_NUMBER eNumber,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

DWORD StringTableGetUnder18Attribute(
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

DWORD StringTableGetTimeCodeAttribute(
	BOOL *pbFound = NULL,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

DWORD StringTableGetGenderAttribute(
	enum GENDER eGender,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);

int StringTableGetAttributeBit(
	const WCHAR *puszAttributeName,
	STRING_TABLE_ENUM eTable = STE_DEFAULT);
	
// CHB 2006.09.07 - Is referenced using ISVERSION(DEVELOPMENT) in consolecmd.cpp.
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT)
void StringTableVerifyLanguage(
	void);
#endif

#if ISVERSION(DEVELOPMENT)
BOOL StringTableCompareSKULanguages(
	int nSKU);
#endif
	
int StringGetForApp(
	const int nStringArray[ APP_GAME_NUM_GAMES ]);

BOOL StringTableFillPakDevLanguage(
	void);

BOOL StringTableSetFillPakDevLanguage(
	BOOL bEnable);
	
#endif // _STRINGTABLE_H_