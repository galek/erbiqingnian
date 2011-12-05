//----------------------------------------------------------------------------
// FILE: fileversion.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "dataversion.h"
#include "fileversion.h"
#include "pakfile.h"
#include "version.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// returns CMP_LESS if a is less than b
// returns CMP_EQUAL if a and b are equal
// returns CMP_GREATER if a is greater than b
//----------------------------------------------------------------------------
VERSION_CMP_VALUE FileVersionCompare(
	const FILE_VERSION& a, 
	const FILE_VERSION& b)
{
	INT32 diff;

	diff = a.dwVerTitle - b.dwVerTitle;
	if (diff < 0) {
		return CMP_LESS;
	} else if (diff > 0) {
		return CMP_GREATER;
	}

	diff = a.dwVerML - b.dwVerML;
	if (diff < 0) {
		return CMP_LESS;
	} else if (diff > 0) {
		return CMP_GREATER;
	}

	diff = a.dwVerRC - b.dwVerRC;
	if (diff < 0) {
		return CMP_LESS;
	} else if (diff > 0) {
		return CMP_GREATER;
	}

	diff = a.dwVerPUB - b.dwVerPUB;
	if (diff < 0) {
		return CMP_LESS;
	} else if (diff > 0) {
		return CMP_GREATER;
	}

	return CMP_EQUAL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionStringToVersion(
	LPCWSTR pszVersion, 
	FILE_VERSION& version)
{
	ASSERTX_RETFALSE( pszVersion, "Expected version string" );
	INT32 num = swscanf_s(pszVersion, L"%d.%d.%d.%d",
		&version.dwVerTitle, &version.dwVerPUB,
		&version.dwVerRC, &version.dwVerML);
	return (num == 4);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionStringToVersion(
	LPCSTR pszVersion, 
	FILE_VERSION& version)
{
	ASSERTX_RETFALSE( pszVersion, "Expected version string" );
	INT32 num = sscanf_s(pszVersion, "%d.%d.%d.%d",
		&version.dwVerTitle, &version.dwVerPUB,
		&version.dwVerRC, &version.dwVerML);
	return (num == 4);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionGet(
	const WCHAR * szFileName, 
	FILE_VERSION &tVersion)
{
#if ISVERSION(SERVER_VERSION)	
	REF( szFileName );
	REF( tVersion );
#else
	DWORD  dwVerHandle;
	DWORD dwVerSize = GetFileVersionInfoSizeW( szFileName, &dwVerHandle );
	if (dwVerSize == 0) 
	{
		return false;
	}

	LPVOID  lpVerBuffer = new LPVOID[dwVerSize];
	LPVOID  lpVerData;
	UINT    cbVerData;
	if(GetFileVersionInfoW(szFileName, dwVerHandle, dwVerSize, lpVerBuffer)) 
	{
		if(VerQueryValue(lpVerBuffer, "\\", &lpVerData, &cbVerData)) 
		{
			VS_FIXEDFILEINFO *  FileInfo = (VS_FIXEDFILEINFO*)lpVerData;
			tVersion.dwVerTitle	= (FileInfo->dwFileVersionMS & 0xffff0000) >> 16;
			tVersion.dwVerPUB		= (FileInfo->dwFileVersionMS & 0x0000ffff);
			tVersion.dwVerRC		= (FileInfo->dwFileVersionLS & 0xffff0000) >> 16;
			tVersion.dwVerML		= (FileInfo->dwFileVersionLS & 0x0000ffff);
			delete [] lpVerBuffer;
			return true;
		}
	}
	delete [] lpVerBuffer;
#endif	
	return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionToString(
	const FILE_VERSION &tVersion,
	WCHAR *puszBuffer,
	int nMaxBuffer)
{
	ASSERTX_RETFALSE( puszBuffer, "Expected buffer" );
	ASSERTX_RETFALSE( nMaxBuffer > 0, "Invalid buffer" );
	
	// print in string
	PStrPrintf(
		puszBuffer,
		nMaxBuffer,
		L"%d.%d.%d.%d",
		tVersion.dwVerTitle, 
		tVersion.dwVerPUB, 
		tVersion.dwVerRC, 
		tVersion.dwVerML);

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionGetBuildVersion(
	FILE_VERSION &tVersion)
{
	DWORD dwVerTitle = 0;
	
	if (AppIsHellgate())
	{
		dwVerTitle = 1;
	}
	else if (AppIsTugboat())
	{
		dwVerTitle = 2;
	}
	
	tVersion.dwVerTitle = dwVerTitle;
	tVersion.dwVerPUB = PRODUCTION_BRANCH_VERSION;
	tVersion.dwVerRC = RC_BRANCH_VERSION;
	tVersion.dwVerML = MAIN_LINE_VERSION;

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileVersionGetInstallVersion(
	FILE_VERSION &tVersion)
{
	return PakFileGetGMBuildVersion( PAK_DEFAULT, tVersion );
}
