//----------------------------------------------------------------------------
// FILE: dataversion.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "dataversion.h"
#include "filepaths.h"
#include "fileversion.h"

#define HGL_VERSION_REG_DIR			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A2B4455D-1046-4732-BFBC-0821BEFC07BC}"
#define HGL_VERSION_REG_KEY			L"DisplayVersion"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

static FILE_VERSION sgtDataVersionOverride;
BOOL sgbDataVersionOverrideEnabled = FALSE;

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DataVersionSet(
	FILE_VERSION& versionMP)
{
	ASYNC_FILE* hFile = NULL;
	WCHAR szFileName[MAX_PATH];
	CHAR szVersion[MAX_PATH];
	BOOL bResult = FALSE;

	PStrPrintf(szFileName, MAX_PATH, L"%Smpv.dat", FILE_PATH_DATA);
	hFile = AsyncFileOpen(szFileName, FF_WRITE | FF_CREATE_ALWAYS);
	if (hFile) {
		PStrPrintf(szVersion, MAX_PATH, "%d.%d.%d.%d", 
			versionMP.dwVerTitle, versionMP.dwVerPUB, versionMP.dwVerRC, versionMP.dwVerML);

		UINT32 writeSize = PStrLen(szVersion)*sizeof(szVersion[0]);
		ASYNC_DATA asyncData(hFile, szVersion, 0, writeSize);
		if (AsyncFileWriteNow(asyncData) == writeSize) {
			bResult = TRUE;
		} else {
			ASSERTV( 0, "Error writing data version to file '%s'", szFileName );
		}
		AsyncFileCloseNow(hFile);
	}

	return bResult;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DataVersionGet(
	FILE_VERSION& versionMP,
	BOOL bCheckPakFiles /*= TRUE*/)
{
	ASYNC_FILE* hFile = NULL;
	WCHAR szFileName[MAX_PATH];
	CHAR szVersion[MAX_PATH];
	BOOL bResult = FALSE;

	// if the override is enabled, return that override version
	if (sgbDataVersionOverrideEnabled == TRUE)
	{
		versionMP = sgtDataVersionOverride;
		bResult = TRUE;
	}
	else
	{
		
		// setup filename
		PStrPrintf(szFileName, MAX_PATH, L"%Smpv.dat", FILE_PATH_DATA);
		
		// open file
		hFile = AsyncFileOpen(szFileName, FF_READ);
		if (hFile != NULL) 
		{
			memclear(szVersion, MAX_PATH*sizeof(szVersion[0]));
			UINT32 readSize = (UINT32) MIN<UINT64>(AsyncFileGetSize(hFile), MAX_PATH*sizeof(szVersion[0]));

			// setup a read operation	
			ASYNC_DATA asyncData(hFile, szVersion, 0, readSize);
			
			// read the data now
			if (AsyncFileReadNow(asyncData) == readSize) 
			{
				if (FileVersionStringToVersion(szVersion, versionMP)) 
				{
					bResult = TRUE;
				}
			}
			if (!bResult) 
			{
				ASSERTV( 0, "Unable to read from data version file '%s'", szFileName );
			}

			// close file handle
			AsyncFileCloseNow( hFile );
		}

		if (!bResult) 
		{
			if (AppIsHellgate()) 
			{
				HKEY hKeyHellgateExists = 0;
				WCHAR szTmp[MAX_PATH];

				DWORD dwLen = MAX_PATH*sizeof(szTmp[0]);
				if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, HGL_VERSION_REG_DIR, 0, KEY_READ | KEY_WOW64_64KEY, &hKeyHellgateExists) == ERROR_SUCCESS) 
				{
					if (RegQueryValueExW(hKeyHellgateExists,  HGL_VERSION_REG_KEY, 0, NULL, (BYTE*)szTmp, &dwLen) == ERROR_SUCCESS) 
					{
						if (FileVersionStringToVersion(szTmp, versionMP)) 
						{
							bResult = TRUE;
						}
					}
				}
			}
		}

		if (!bResult && bCheckPakFiles == TRUE)
		{
			// TODO: There are some internet cafes that copied the game installation around 
			// instead of going through the install processs, so the registry is going
			// to be broken for those installations and the dataversion will not be able
			// to be maintained and accessed from the launcher unless it is able to use the pakfile system	
			bResult = FileVersionGetInstallVersion( versionMP );
		}
		
		if (!bResult) 
		{
			versionMP.dwVerTitle = 1;
			versionMP.dwVerML = 0;
			versionMP.dwVerRC = 0;
			versionMP.dwVerPUB = 0;
		}
		
	}
	
	return bResult;	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DataVersionSetOverride(
	struct FILE_VERSION &tDataVersionOverride)
{
	sgtDataVersionOverride = tDataVersionOverride;
	sgbDataVersionOverrideEnabled = TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DataVersionClearOverride(
	void)
{
	sgbDataVersionOverrideEnabled = FALSE;
}
