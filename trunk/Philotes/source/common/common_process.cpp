//----------------------------------------------------------------------------
// FILE: common_process.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "process.h"
#include <psapi.h>

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetProcessFullName(
	DWORD dwProcessID,
	WCHAR *puszFullName,
	int nMaxFullName)
{
	ASSERTX_RETFALSE( dwProcessID != 0, "Expected process ID" );
	ASSERTX_RETFALSE( puszFullName, "Expected name" );
	ASSERTX_RETFALSE( nMaxFullName > 0, "Invalid name size" );

	// Get a handle to the process.
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );

	// Get the process name.
	if (hProcess )
	{

		// is the process still active
		DWORD dwExitCode = 0;
		if (GetExitCodeProcess( hProcess, &dwExitCode ) != 0)
		{
			if (dwExitCode == STILL_ACTIVE)
			{

				HMODULE hMod;
				DWORD cbNeeded;
				if ( EnumProcessModules( hProcess, &hMod, sizeof( hMod ), &cbNeeded ))
				{

					GetModuleFileNameExW(
						hProcess,
						hMod,
						puszFullName,
						nMaxFullName);

				}

			}

		}

	}

	// close handle
	CloseHandle( hProcess );	

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ProcessIsRunning(
	const WCHAR *puszFullName)
{
	ASSERTX_RETFALSE( puszFullName, "Expected base name" );

	// get all processes running on machine
	const int MAX_PROCESSES = 2048;
	DWORD dwProcesses[ MAX_PROCESSES ];
	DWORD dwBytesReturned = 0;
	EnumProcesses( dwProcesses, MAX_PROCESSES, &dwBytesReturned );

	// how many processes were there returned
	DWORD dwNumProcesses = dwBytesReturned / sizeof( DWORD );

	// find the process in question
	for (DWORD dwIndex = 0; dwIndex < dwNumProcesses; ++dwIndex)
	{
		DWORD dwProcessID = dwProcesses[ dwIndex ];
		if (dwProcessID != 0)
		{
			const int MAX_NAME = 1024;
			WCHAR uszProcessFullName[ MAX_NAME ] = { 0 };

			// get process name			
			if (sGetProcessFullName( dwProcessID, uszProcessFullName, MAX_NAME ))
			{

				// must match full name
				if (PStrICmp( uszProcessFullName, puszFullName ) == 0)
				{
					return TRUE;
				}

			}

		}

	}

	return FALSE;

}

