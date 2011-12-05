//--------------------------------------------------------------------------------------
// File: gameexplorer.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"

#define UNICODE  // for the right versions of add game to explorer

#include "gameexplorerhelper.h"
#include "gameexplorer.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static GUID sgGUID = GUID_NULL;

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddTasks(
	GUID &guid,
	WCHAR *puszGameExePath,
#ifdef GAME_EXPLORER_ENABLED
	GAME_EXPLORER_ERROR &tError)
#else
	GAME_EXPLORER_ERROR &)
#endif
{
#ifdef GAME_EXPLORER_ENABLED

	// Create tasks as needed
	HRESULT hr = CreateTaskW( 
		GIS_CURRENT_USER, 
		&guid, 
		FALSE, 
		0, 
		L"Play", 
		puszGameExePath, 
		NULL);
	
	if (FAILED( hr ))
	{
		wsprintf( tError.uszError, L"Adding task failed: 0x%0.8x", hr );
		return FALSE;
	}
#else
	REF(guid);
	REF(puszGameExePath);
#endif
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameExplorerAdd(
	WCHAR *puszGameExePath,
	WCHAR *puszGDFBinaryPath,
	WCHAR *puszInstallPath,
#ifdef GAME_EXPLORER_ENABLED
	GAME_EXPLORER_ERROR &tError)
#else
	GAME_EXPLORER_ERROR &)
#endif
{

#ifdef GAME_EXPLORER_ENABLED

	// add to explorer
	HRESULT hr = AddToGameExplorer( 
		puszGDFBinaryPath, 
		puszInstallPath, 
		GIS_CURRENT_USER, 
		&sgGUID);
		
	if (SUCCEEDED( hr ))
	{
		return sAddTasks( sgGUID, puszGameExePath, tError );
	}
	else	
	{
		wsprintf( tError.uszError, L"Adding game failed: 0x%0.8x", hr );
		return FALSE;
	}
	
#else
	REF(puszGameExePath);
	REF(puszGDFBinaryPath);
	REF(puszInstallPath);

	return FALSE;

#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameExplorerRemove(
	GAME_EXPLORER_ERROR &tError)
{

#ifdef GAME_EXPLORER_ENABLED

	// must be installed
	if (sgGUID == GUID_NULL)
	{
		wsprintf( tError.uszError, L"No GUID is registered" );
		return FALSE;
	}

	// remove from explorer
	HRESULT hr = RemoveFromGameExplorer( &sgGUID );
	if (FAILED( hr ))
	{
		wsprintf( tError.uszError, L"Removing game failed: 0x%0.8x", hr );
		return FALSE;
	}
	
	// remove tasks
	hr = RemoveTasks( &sgGUID );
	if (FAILED( hr ))
	{
		wsprintf( tError.uszError, L"Removing tasks failed: 0x%0.8x", hr );
		return FALSE;
	}

	sgGUID = GUID_NULL;	
	return TRUE;

#else
	UNREFERENCED_PARAMETER(tError);
	return FALSE;

#endif
		
}
