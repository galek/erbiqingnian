//----------------------------------------------------------------------------
// FILE: wordfilter.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "filepaths.h"
#include "unicode.h"
#include "excel.h"
#include "wordfilter.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct WORD_FILTER_LOOKUP
{
	const WCHAR *pszFilename;
	BOOL bUsePakFile;
};

//----------------------------------------------------------------------------
struct FILTERED_WORD
{
	int nIndex;						// index into word buffer for word string
	BOOL bAllowPrefixed;
	BOOL bAllowSuffixed;
};

//----------------------------------------------------------------------------
struct WORD_FILTER_GLOBALS
{

	WCHAR *puszWordBuffer;			// where to store the words
	DWORD dwWordBufferSize;			// # of bytes allcoated in puszWordBuffer
	int nNextFreeIndex;				// index of where to put the next word in the buffer
	
	FILTERED_WORD *pWords;			// entries of individual words
	int nWordsAllocated;			// # of words allocated in pWords
	int nNumWords;					// # of used words in pWords
	
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static WORD_FILTER_GLOBALS tChatFilterGlobals;
static WORD_FILTER_GLOBALS tNameFilterGlobals;
static const DWORD WORD_BUFFER_ALLOC_SIZE	= KILO * 16;
static const DWORD WORD_ALLOC_SIZE			= KILO;
static const DWORD MAX_FILTER_WORD_LENGTH	= 256;

//----------------------------------------------------------------------------
static const WORD_FILTER_LOOKUP sgtChatFilterLookup[] = 
{

	// filename									// use pak file
	{ L"word_filter_user.unicode.txt",			FALSE },
	{ L"word_filter_official.unicode.txt",		TRUE },
	
};
static const int sgnNumChatFilterLookup = arrsize( sgtChatFilterLookup );

//----------------------------------------------------------------------------
static const WORD_FILTER_LOOKUP sgtNameFilterLookup[] = 
{

	// filename									// use pak file
	{ L"name_filter_official.unicode.txt",		TRUE },

};
static const int sgnNumNameFilterLookup = arrsize( sgtNameFilterLookup );

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORD_FILTER_GLOBALS *sGetChatFilterGlobals(
	void)
{
	return &tChatFilterGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORD_FILTER_GLOBALS *sGetNameFilterGlobals(
	void)
{
	return &tNameFilterGlobals;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWordFilterGlobalsInit(
	WORD_FILTER_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	
	pGlobals->nNumWords = 0;
	pGlobals->puszWordBuffer = NULL;
	pGlobals->dwWordBufferSize = 0;
	pGlobals->nNextFreeIndex = 0;
	pGlobals->pWords = NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILTERED_WORD *sFilteredWordCreate( 
	WORD_FILTER_GLOBALS *pGlobals)
{
	ASSERTX_RETNULL( pGlobals, "Expected globals" );
	
	// see if we need to allocate more words
	if (pGlobals->nNumWords >= pGlobals->nWordsAllocated)
	{
	
		// allocate more words
		pGlobals->nWordsAllocated += WORD_ALLOC_SIZE;
		
		// realloc words
		pGlobals->pWords = (FILTERED_WORD *)REALLOC( NULL, pGlobals->pWords, pGlobals->nWordsAllocated * sizeof( FILTERED_WORD ) );
		
	}

	// return next free word
	return &pGlobals->pWords[ pGlobals->nNumWords++ ];
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sWordFilterAdd(
	WORD_FILTER_GLOBALS *pGlobals,
	const WCHAR *puszWord,
	BOOL bAllowPrefixed = FALSE,
	BOOL bAllowSuffixed = FALSE)
{
	ASSERTX_RETFALSE( puszWord, "Expected word" );
	
	// how long is the string we're adding
	int nLength = PStrLen( puszWord );
	if (nLength > 0 && nLength < (MAX_FILTER_WORD_LENGTH - 1))
	{
	
		// how much space will the new word take up
		DWORD dwSize = sizeof( WCHAR ) * (nLength + 1);
		
		// do we need to allocate more memory to hold the string data
		DWORD dwUsedSize = pGlobals->nNextFreeIndex * sizeof( WCHAR );
		if (dwSize + dwUsedSize >= pGlobals->dwWordBufferSize)
		{
		
			// not enough room, set the new size we want to allocate to
			while (dwSize + dwUsedSize >= pGlobals->dwWordBufferSize)
			{
				pGlobals->dwWordBufferSize += WORD_BUFFER_ALLOC_SIZE;
			}
			
			// reallocate the word buffer
			pGlobals->puszWordBuffer = (WCHAR *)REALLOC( NULL, pGlobals->puszWordBuffer, pGlobals->dwWordBufferSize );
			
		}
	
		// create filtered word
		FILTERED_WORD *pWord = sFilteredWordCreate( pGlobals );

		// save index of word
		pWord->nIndex = pGlobals->nNextFreeIndex;
		// set prefix/suffix
		pWord->bAllowPrefixed = bAllowPrefixed;
		pWord->bAllowSuffixed = bAllowSuffixed;
				
		// copy string to word buffer
		int nMaxCharacters = (pGlobals->dwWordBufferSize / sizeof( WCHAR )) - pGlobals->nNextFreeIndex;
		PStrCopy( &pGlobals->puszWordBuffer[ pGlobals->nNextFreeIndex ], puszWord, nMaxCharacters );
		
		// update next free index
		pGlobals->nNextFreeIndex += nLength;
		
		// terminate string
		pGlobals->puszWordBuffer[ pGlobals->nNextFreeIndex++ ] = 0;
		
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sWordFilterLoadFile(
	WORD_FILTER_GLOBALS *pGlobals,
	const WCHAR *puszFilename,
	BOOL bUsePakFile)
{
	ASSERTX_RETFALSE( puszFilename, "Expected filename" );

	// construct path to file
	WCHAR uszFilePath[ MAX_PATH ];
	PStrPrintf( uszFilePath, MAX_PATH, L"%S%s", FILE_PATH_DATA_COMMON, puszFilename );

	// load the file
	DWORD dwFileSize = 0;
	BYTE *pFileData = NULL;
	if (bUsePakFile)
	{
	
				
		// construct path
		TCHAR tszFilePath[ MAX_PATH ] = { 0 };
		PStrCat( tszFilePath, FILE_PATH_DATA_COMMON, MAX_PATH );
		char szFilename[ MAX_PATH ];		
		PStrCvt( szFilename, puszFilename, MAX_PATH );
		PStrCat( tszFilePath, szFilename, MAX_PATH );		

		// load now		
		DECLARE_LOAD_SPEC( tSpec, tszFilePath );
		tSpec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
		tSpec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
		pFileData = (BYTE *)PakFileLoadNow( tSpec );
		dwFileSize = tSpec.bytesread;
	}
	else
	{
		if (FileExists( uszFilePath ) && FileGetSize( uszFilePath ) > 0 )
		{
			pFileData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE( NULL, uszFilePath, &dwFileSize ));
		}
	}

	// if no file found forget it
	if (pFileData == NULL)
	{
		return TRUE;  // no file found
	}	
	
	// setup auto free on the file data
	AUTOFREE tAutoFree( NULL, pFileData );

	// get the text buffer
	int nNumCharacters = 0;
	const WCHAR *puszBuffer = UnicodeFileGetEncoding( pFileData, dwFileSize, nNumCharacters );

	if (nNumCharacters > 0)
	{
	
		// scan each line in the file
		// note that we are not space delimited.  We instead trim leading and trailing spaces.
		// this allows us to ban phrases without banning the individual words.
		const WCHAR *uszSeps = L"\n\r\t";
		const int MAX_TOKEN = 256;
		WCHAR uszToken[ MAX_TOKEN ];
		int nAllTokensLength = nNumCharacters;
		puszBuffer = PStrTok( uszToken, puszBuffer, uszSeps, MAX_TOKEN, nAllTokensLength );
		while (uszToken[ 0 ])
		{
			PStrTrimLeadingSpaces(uszToken, MAX_TOKEN);
			PStrTrimTrailingSpaces(uszToken, MAX_TOKEN);
		
			// add this word
			if (sWordFilterAdd( pGlobals, uszToken ) == FALSE)
			{
				return FALSE;
			}
					
			// get next token
			puszBuffer = PStrTok( uszToken, puszBuffer, uszSeps, MAX_TOKEN, nAllTokensLength );
			
		}

	}
						
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sWordFilterLoadFromExcel(
	WORD_FILTER_GLOBALS *pGlobals,
	unsigned int idTable)
{
	ASSERT(idTable == DATATABLE_FILTER_CHATFILTER || idTable == DATATABLE_FILTER_NAMEFILTER);
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );
	int nRows = ExcelGetCount(EXCEL_CONTEXT(), idTable);
	for(int i = 0; i < nRows; i++)
	{
		FILTER_WORD_DEFINITION *pFilterWordDef = (FILTER_WORD_DEFINITION *)
			ExcelGetData(NULL, idTable, i);
		if(!pFilterWordDef) continue;
		WCHAR wszWord[256];
		PStrCvt(wszWord, pFilterWordDef->szWord, arrsize(wszWord));
		ASSERT(sWordFilterAdd(pGlobals, wszWord,
			pFilterWordDef->bAllowPrefixed, pFilterWordDef->bAllowSuffixed) );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ChatFilterInitForApp(
	void)
{
	WORD_FILTER_GLOBALS *pChatGlobals = sGetChatFilterGlobals();
	ASSERT_RETFALSE(pChatGlobals);
	sWordFilterGlobalsInit(pChatGlobals);
	// load each file
	for (int i = 0; i < sgnNumChatFilterLookup; ++i)
	{
		const WORD_FILTER_LOOKUP *pLookup = &sgtChatFilterLookup[ i ];
		if (sWordFilterLoadFile( pChatGlobals, pLookup->pszFilename, pLookup->bUsePakFile ) == FALSE)
		{
			ASSERTV_CONTINUE( 0, "Unable to load word filter file '%s'", pLookup->pszFilename );
		}
	}
	// load the excel table
	ASSERTX(sWordFilterLoadFromExcel(pChatGlobals, DATATABLE_FILTER_CHATFILTER), 
		"Unable to load excel chat word filter");
	
	return TRUE;
}

//----------------------------------------------------------------------------
// A little bit of functionality duplication, but you can only factor so much!
//----------------------------------------------------------------------------
BOOL NameFilterInitForApp(
	void)
{
	WORD_FILTER_GLOBALS *pNameGlobals = sGetNameFilterGlobals();
	ASSERT_RETFALSE(pNameGlobals);
	sWordFilterGlobalsInit(pNameGlobals);
	// load each file
	for (int i = 0; i < sgnNumNameFilterLookup; ++i)
	{
		const WORD_FILTER_LOOKUP *pLookup = &sgtNameFilterLookup[ i ];
		if (sWordFilterLoadFile( pNameGlobals, pLookup->pszFilename, pLookup->bUsePakFile ) == FALSE)
		{
			ASSERTV_CONTINUE( 0, "Unable to load word filter file '%s'", pLookup->pszFilename );
		}
	}
	// load the excel table
	ASSERTX(sWordFilterLoadFromExcel(pNameGlobals, DATATABLE_FILTER_NAMEFILTER), 
		"Unable to load excel name word filter");

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWordFilterFreeForApp(
	WORD_FILTER_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected globals" );

	// free words
	FREE( NULL, pGlobals->pWords );
	pGlobals->pWords = NULL;

	// free word buffer
	FREE( NULL, pGlobals->puszWordBuffer );
	pGlobals->puszWordBuffer = NULL;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ChatFilterFreeForApp(
	void)
{
	sWordFilterFreeForApp(sGetChatFilterGlobals() );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NameFilterFreeForApp(
	void)
{
	sWordFilterFreeForApp(sGetNameFilterGlobals() );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ChatFilterStringInPlace(
	WCHAR *puszString)
{
	ASSERTX_RETURN( puszString, "Expected string buffer" );
	const WORD_FILTER_GLOBALS *pGlobals = sGetChatFilterGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	int nStringLength = PStrLen( puszString );
			
	// brute force for now, Korea needs this today
	WCHAR uszReplacement[ MAX_FILTER_WORD_LENGTH ];
	for (int i = 0; i < pGlobals->nNumWords; ++i)
	{
	
		// get string to filter
		const FILTERED_WORD *pWord = &pGlobals->pWords[ i ];
		WCHAR *puszFilterString = &pGlobals->puszWordBuffer[ pWord->nIndex ];
		
		// what will we replace it with
		int nFilterLen = PStrLen( puszFilterString );
		for (int j = 0; j < nFilterLen; ++j)
		{
			ASSERTX_BREAK( j < MAX_FILTER_WORD_LENGTH - 1, "Filter word too long" );
			uszReplacement[ j ] = L'*';
		}
		uszReplacement[ nFilterLen ] = 0;
		
		// do the replacement
		PStrReplaceTokenI1337( puszString, nStringLength + 1, puszFilterString, uszReplacement,
			!pWord->bAllowPrefixed, !pWord->bAllowSuffixed);
	}
	
}

//----------------------------------------------------------------------------
// brute force for now, may later convert to using a trie for matching.
//----------------------------------------------------------------------------
BOOL NameFilterString(
	WCHAR *szName)
{
	ASSERTX_RETFALSE( szName, "Expected string buffer" );
	const WORD_FILTER_GLOBALS *pGlobals = sGetNameFilterGlobals();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );

	for (int i = 0; i < pGlobals->nNumWords; ++i)
	{
		const FILTERED_WORD *pFilterWordDef = &pGlobals->pWords[ i ];
		WCHAR *wszFilteredName = 
			&pGlobals->puszWordBuffer[ pFilterWordDef->nIndex ];
		if(pFilterWordDef->bAllowPrefixed && pFilterWordDef->bAllowSuffixed)
		{	//exact string match: prevent robertdonald
			if(PStrCmpI1337(szName, wszFilteredName, MAX_CHARACTER_NAME) == 0 )
			{
				return FALSE;	
			}
		}
		else if(!pFilterWordDef->bAllowPrefixed && !pFilterWordDef->bAllowSuffixed)
		{	//substring match: prevent Frobertdonaldsucks
			if(PStrStrI1337(szName, wszFilteredName, MAX_CHARACTER_NAME) != NULL)
			{
				return FALSE;
			}
		}
		else if(!pFilterWordDef->bAllowPrefixed && pFilterWordDef->bAllowSuffixed)
		{	//suffix match: prevent screwrobertdonald
			//Known issue with this method: this will let through a word repeated twice, like suckssucks--the PStrStr only sees the first sucks.
			WCHAR * pWord = PStrStrI1337(szName, wszFilteredName, MAX_CHARACTER_NAME);
			if(pWord != NULL)
			{
				int nNameLength = PStrLen(szName);
				int nFilteredNameLength = PStrLen(wszFilteredName);
				if(pWord == (szName + nNameLength - nFilteredNameLength) ) return FALSE;
			}
		}
		else if(pFilterWordDef->bAllowPrefixed && !pFilterWordDef->bAllowSuffixed)
		{	//prefix match: prevent robertdonaldsucks
			if(PStrStrI1337(szName, wszFilteredName, MAX_CHARACTER_NAME) == szName)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}