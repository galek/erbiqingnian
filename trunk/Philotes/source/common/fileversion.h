//----------------------------------------------------------------------------
// FILE: fileversion.h
//----------------------------------------------------------------------------

#ifndef __FILE_VERSION_H_
#define __FILE_VERSION_H_

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum FILE_VERSION_CONSTANTS
{
	FILE_VERSION_MAX_STRING = 64
};

//----------------------------------------------------------------------------
struct FILE_VERSION
{
	DWORD dwVerTitle;
	DWORD dwVerPUB;
	DWORD dwVerRC;
	DWORD dwVerML;

	FILE_VERSION::FILE_VERSION( 
		DWORD dwVerTitle,
		DWORD dwVerPUB,
		DWORD dwVerRC,
		DWORD dwVerML)
		:	dwVerTitle( dwVerTitle ),
			dwVerPUB( dwVerPUB ),
			dwVerRC( dwVerRC ),
			dwVerML( dwVerML )
	{ }
		
	FILE_VERSION::FILE_VERSION( void )
		:	dwVerTitle( 0 ),
			dwVerPUB( 0 ),
			dwVerRC( 0 ),
			dwVerML( 0 )
	{ }
	
	void FILE_VERSION::Set(
		DWORD dwVerTitleParam,
		DWORD dwVerPUBParam,
		DWORD dwVerRCParam,
		DWORD dwVerMLParam)
	{
		dwVerTitle = dwVerTitleParam;
		dwVerPUB = dwVerPUBParam;
		dwVerRC = dwVerRCParam;
		dwVerML = dwVerMLParam;
	}
	
};

enum VERSION_CMP_VALUE
{
	CMP_LESS = -1,
	CMP_EQUAL = 0,
	CMP_GREATER = 1,
};

//----------------------------------------------------------------------------
// Exported Functions
//----------------------------------------------------------------------------
	
BOOL FileVersionStringToVersion(
	LPCWSTR pszVersion, 
	FILE_VERSION& version);

BOOL FileVersionStringToVersion(
	LPCSTR pszVersion, 
	FILE_VERSION& version);

BOOL FileVersionGet(
	const WCHAR *szFileName, 
	FILE_VERSION &tVersion);

VERSION_CMP_VALUE FileVersionCompare(
	const FILE_VERSION& a, 
	const FILE_VERSION& b);

BOOL FileVersionToString(
	const FILE_VERSION &tVersion,
	WCHAR *puszBuffer,
	int nMaxBuffer);

BOOL FileVersionGetBuildVersion(
	FILE_VERSION &tVersion);

BOOL FileVersionGetInstallVersion(
	FILE_VERSION &tVersion);
		
#endif