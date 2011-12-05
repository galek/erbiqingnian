//----------------------------------------------------------------------------
// FILE: gameexplorer.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __GAME_EXPLORER_H_
#define __GAME_EXPLORER_H_

#define MAX_GAME_EXPLORER_ERROR_STRING (2048)

//----------------------------------------------------------------------------
struct GAME_EXPLORER_ERROR
{
	WCHAR uszError[ MAX_GAME_EXPLORER_ERROR_STRING ];
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

BOOL GameExplorerAdd(
	WCHAR *puszGameExePath,
	WCHAR *puszGDFBinaryPath,
	WCHAR *puszInstallPath,
	GAME_EXPLORER_ERROR &tError);

BOOL GameExplorerRemove(
	GAME_EXPLORER_ERROR &tError);

#endif