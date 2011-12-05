//----------------------------------------------------------------------------
// FILE: gameexplorerhelper.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __GAME_EXPLORER_HELPER_H_
#define __GAME_EXPLORER_HELPER_H_

//#define GAME_EXPLORER_ENABLED
#ifdef GAME_EXPLORER_ENABLED

#include <windows.h>
#include <gameux.h>
#include <msi.h>
#include <msiquery.h>

//--------------------------------------------------------------------------------------
// UNICODE/ANSI define mappings
//--------------------------------------------------------------------------------------
#ifdef UNICODE
#define AddToGameExplorer AddToGameExplorerW
#else
#define AddToGameExplorer AddToGameExplorerA
#endif 

#ifdef UNICODE
#define RetrieveGUIDForApplication RetrieveGUIDForApplicationW
#else
#define RetrieveGUIDForApplication RetrieveGUIDForApplicationA
#endif 

//--------------------------------------------------------------------------------------
// Given a game instance GUID and path to GDF binary, registers game with Game Explorer
//--------------------------------------------------------------------------------------
STDAPI AddToGameExplorerW( WCHAR* strGDFBinPath, WCHAR* strGameInstallPath, GAME_INSTALL_SCOPE InstallScope, GUID* pGameInstanceGUID );
STDAPI AddToGameExplorerA( CHAR* strGDFBinPath, CHAR* strGameInstallPath, GAME_INSTALL_SCOPE InstallScope, GUID* pGameInstanceGUID );

//--------------------------------------------------------------------------------------
// Given a path to a GDF binary that has already been registered, returns a game instance GUID
//--------------------------------------------------------------------------------------
STDAPI RetrieveGUIDForApplicationW( WCHAR* szPathToGDFdll, GUID* pGameInstanceGUID );
STDAPI RetrieveGUIDForApplicationA( CHAR* szPathToGDFdll, GUID* pGameInstanceGUID );

//--------------------------------------------------------------------------------------
// Given a game instance GUID, unregisters a game from Game Explorer
//--------------------------------------------------------------------------------------
STDAPI RemoveFromGameExplorer( GUID* pGameInstanceGUID );

//--------------------------------------------------------------------------------------
// Creates a unique game instance GUID
//--------------------------------------------------------------------------------------
STDAPI GenerateGUID( GUID* pGameInstanceGUID );

//--------------------------------------------------------------------------------------
// Given a a game instance GUID, creates a task shortcut in the proper location
//--------------------------------------------------------------------------------------
STDAPI CreateTaskW( GAME_INSTALL_SCOPE InstallScope, GUID* pGUID, BOOL bSupportTask, int nTaskID, WCHAR* strTaskName, WCHAR* strLaunchPath, WCHAR* strCommandLineArgs );
STDAPI CreateTaskA( GAME_INSTALL_SCOPE InstallScope, GUID* pGUID, BOOL bSupportTask, int nTaskID, CHAR* strTaskName, CHAR* strLaunchPath, CHAR* strCommandLineArgs );

//--------------------------------------------------------------------------------------
// This removes all the tasks associated with a game instance GUID
// Pass in a valid GameInstance GUID that was passed to AddGame()
//--------------------------------------------------------------------------------------
STDAPI RemoveTasks( GUID* pGUID ); 

//--------------------------------------------------------------------------------------
// For use during an MSI custom action install. 
// This sets up the CustomActionData properties for the deferred custom actions. 
//--------------------------------------------------------------------------------------
UINT WINAPI SetMSIGameExplorerProperties( MSIHANDLE hModule );

//--------------------------------------------------------------------------------------
// For use during an MSI custom action install. 
// This adds the game to the Game Explorer
//--------------------------------------------------------------------------------------
UINT WINAPI AddToGameExplorerUsingMSI( MSIHANDLE hModule );

//--------------------------------------------------------------------------------------
// For use during an MSI custom action install. 
// This removes the game to the Game Explorer
//--------------------------------------------------------------------------------------
UINT WINAPI RemoveFromGameExplorerUsingMSI( MSIHANDLE hModule );

#endif	// end GAME_EXPLORER_ENABLED

#endif  // end __GAME_EXPLORER_HELPER_H_