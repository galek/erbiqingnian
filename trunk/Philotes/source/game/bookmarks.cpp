//----------------------------------------------------------------------------
// FILE: bookmarks.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "bookmarks.h"
#include "unitfile.h"
#include "excel.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BookmarksInit( 
	BOOKMARKS &tBookmarks)
{
	for (int i = 0; i < NUM_UNIT_BOOKMARKS; ++i)
	{
		tBookmarks.nCursorPos[ i ] = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL SetBookmark( 
	const BOOKMARKS &tBookmarks, 
	UNIT_BOOKMARK eBookmark, 
	XFER<mode> &tXfer,
	unsigned int nBookmarkPosition,
	const UNIT_FILE_HEADER &tHeader)
{

	// we only do something if we're saving bookmarks
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_BOOKMARKS ))
	{
	
		// save the current cursor position
		unsigned int nCurrentPos = tXfer.GetCursor();
		
		// go back to the cursor position of the requested bookmark
		ASSERTX_RETFALSE( tBookmarks.nCursorPos[ eBookmark ] > 0, "Invalid bookmark cursor position" );
		tXfer.SetCursor( tBookmarks.nCursorPos[ eBookmark ] );
		
		// write the cursor position for the bookmark
		XFER_UINT( tXfer, nBookmarkPosition, STREAM_CURSOR_SIZE_BITS );
		
		// place the buffer position back where it was
		tXfer.SetCursor( nCurrentPos );

	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
// Note: Excel is now in common, so the bots COULD link it, if we want.
// They don't currently load the excel tables, but could if desired.
//----------------------------------------------------------------------------
static DWORD sGetBookmarkCode(
	UNIT_BOOKMARK eBookmark)
{
#ifndef _BOT
	return ExcelGetCode( NULL, DATATABLE_BOOKMARKS, eBookmark );
#else
	return INVALID_CODE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_BOOKMARK sGetBookmarkByCode(
	DWORD dwCode)
{
#ifndef _BOT
	return (UNIT_BOOKMARK)ExcelGetLineByCode( NULL, DATATABLE_BOOKMARKS, dwCode );
#else
	return UB_INVALID;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL BookmarksXfer(
	UNIT_FILE_HEADER &tHeader,
	BOOKMARKS &tBookmarks,
	XFER<mode> &tXfer)
{

	// init bookmarks
	BookmarksInit( tBookmarks );

	// only if bookmark data is present		
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_BOOKMARKS ))
	{

		// num bookmarks
		unsigned int nNumBookmarks = NUM_UNIT_BOOKMARKS;
		XFER_UINT( tXfer, nNumBookmarks, STREAM_BOOKMARK_COUNT_BITS );

		// xfer each bookmark
		for (unsigned int i = 0; i < nNumBookmarks; ++i)
		{
			DWORD dwCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				UNIT_BOOKMARK eBookmark = (UNIT_BOOKMARK)i;
				dwCode = sGetBookmarkCode( eBookmark );
			}
			XFER_DWORD( tXfer, dwCode, STREAM_BITS_BOOKMARK_CODE );

			// bookmark cursor pos
			DWORD dwCursorPos = 0;
			if (tXfer.IsSave())
			{
				UNIT_BOOKMARK eBookmark = (UNIT_BOOKMARK)i;
				tBookmarks.nCursorPos[ eBookmark ] = tXfer.GetCursor();
				dwCursorPos = tBookmarks.nCursorPos[ eBookmark ];
			}
			XFER_DWORD( tXfer, dwCursorPos, STREAM_CURSOR_SIZE_BITS );

			// save in bookmarks when loading if it's still a valid bookmark
			if (tXfer.IsLoad())
			{
				UNIT_BOOKMARK eBookmark = sGetBookmarkByCode( dwCode );
				if (eBookmark != UB_INVALID)
				{
					tBookmarks.nCursorPos[ eBookmark ] = dwCursorPos;
				}
			}

		}

	}

	return TRUE;

}