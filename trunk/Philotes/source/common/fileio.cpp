//----------------------------------------------------------------------------
// fileio.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include <shlobj.h>
#include "appcommon.h"
#include "appcommontimer.h"
#include "pakfile.h"
#include "performance.h"
#if ISVERSION(SERVER_VERSION)
#include "fileio.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define FILE_TRACE		1
#endif

static BOOL sgbGenerateHistoryFiles = FALSE;


//----------------------------------------------------------------------------
struct FILE_TYPE_RECORD
{
	char szExample[ MAX_FILE_NAME_LENGTH ];
	const char *pszExtension;  // pointer into szExample at the extension
};

//----------------------------------------------------------------------------
struct FILEIO_GLOBALS
{

	// debug used file types info	
#if ISVERSION(DEBUG_VERSION)
	#define MAX_FILETYPES (1024)		// arbitrary, but easy
	FILE_TYPE_RECORD fileTypeRecords[ MAX_FILETYPES ];
	int nFileTypeRecordCount;  // num of valid entires in fileTypeRecords
#endif
	
};

//----------------------------------------------------------------------------
// STATIC
//----------------------------------------------------------------------------
static FILEIO_GLOBALS *sgpFileIOGlobals = NULL;
static CCriticalSection sgcsFileIO(TRUE);
static UINT32 sgiFileIORefCount = 0;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Overloaded wrappers for Windows ANSI and Unicode file functions
//----------------------------------------------------------------------------
static FILEIO_GLOBALS *sFileIOGlobalsGet(
	void)
{
	return sgpFileIOGlobals;
}

template<class T>
struct Win32Traits;

template<>
struct Win32Traits<char>
{
	typedef char value_type;
	typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
};

template<>
struct Win32Traits<wchar_t>
{
	typedef wchar_t value_type;
	typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
};

HANDLE sFindFirstFile(const char * pName, WIN32_FIND_DATAA * pFD)
{
	return ::FindFirstFileA(pName, pFD);
}

HANDLE sFindFirstFile(const WCHAR * pName, WIN32_FIND_DATAW * pFD)
{
	return ::FindFirstFileW(pName, pFD);
}

DWORD sGetFileAttributes(
	const char * szFilename)
{
	return GetFileAttributesA(szFilename);
}

DWORD sGetFileAttributes(
	const WCHAR * szFilename)
{
	return GetFileAttributesW(szFilename);
}

BOOL sSetFileAttributes(
	const char * szFilename,
	DWORD dwFileAttributes)
{
	return SetFileAttributesA(szFilename, dwFileAttributes);
}

BOOL sSetFileAttributes(
	const WCHAR * szFilename,
	DWORD dwFileAttributes)
{
	return SetFileAttributesW(szFilename, dwFileAttributes);
}

BOOL sDeleteFile(
	const char * szFilename)
{
	return DeleteFileA(szFilename);
}

BOOL sDeleteFile(
	const WCHAR * szFilename)
{
	return DeleteFileW(szFilename);
}

BOOL sMoveFile(
	const char * szExistingFilename,
	const char * szNewFilename)
{
	return MoveFileA(szExistingFilename, szNewFilename);
}

BOOL sMoveFile(
	const WCHAR * szExistingFilename,
	const WCHAR * szNewFilename)
{
	return MoveFileW(szExistingFilename, szNewFilename);
}

BOOL sCreateDirectory(
	const char * szDirName)
{
	return CreateDirectoryA(szDirName, NULL);
}

BOOL sCreateDirectory(
	const WCHAR * szDirName)
{
	return CreateDirectoryW(szDirName, NULL);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileIOInit(
	void)
{
	sgcsFileIO.Enter();
	if (sgiFileIORefCount == 0) 
	{
		ASSERT(sgpFileIOGlobals == NULL);

		// allocate globals
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		sgpFileIOGlobals = (FILEIO_GLOBALS *)MALLOCZ( g_StaticAllocator, sizeof( FILEIO_GLOBALS ) );
#else
		sgpFileIOGlobals = (FILEIO_GLOBALS *)MALLOCZ( NULL, sizeof( FILEIO_GLOBALS ) );
#endif		

#if ISVERSION(DEBUG_VERSION)
		sgpFileIOGlobals->nFileTypeRecordCount = 0;
#endif	
	} 
	else 
	{
		ASSERT(sgpFileIOGlobals != NULL);
	}
	sgiFileIORefCount++;
	sgcsFileIO.Leave();
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileIOFree(
	void)
{
	sgcsFileIO.Enter();
	sgiFileIORefCount--;
	if (sgiFileIORefCount == 0) 
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE( g_StaticAllocator, sgpFileIOGlobals );
#else
		FREE( NULL, sgpFileIOGlobals );
#endif		
	
		sgpFileIOGlobals = NULL;
	}
	sgcsFileIO.Leave();
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char *sLogLoadedFileType(
	const char *fullname)
{
	
	// get extension from full filename
	const char *pszExtension = PStrGetExtension( fullname );
	if (pszExtension)
	{
	
#if ISVERSION(DEBUG_VERSION)
		FILEIO_GLOBALS *fileIOGlobals = sFileIOGlobalsGet();
		// search for extension in storage
		BOOL bNewFileType = TRUE;
		for (int i = 0; i < fileIOGlobals->nFileTypeRecordCount; ++i)
		{
			const FILE_TYPE_RECORD &fileTypeRecord = fileIOGlobals->fileTypeRecords[ i ];
			
			// is match
			if (PStrICmp( fileTypeRecord.pszExtension, pszExtension) == 0)
			{
				bNewFileType = FALSE;  // not a new file type
				break;
			}

		}

		// if we haven't recorded this extension before, save it
		if (bNewFileType)
		{

			// must fit in table
			ASSERT_RETNULL( fileIOGlobals->nFileTypeRecordCount < MAX_FILETYPES - 1 );
			FILE_TYPE_RECORD &fileTypeRecord = fileIOGlobals->fileTypeRecords[ fileIOGlobals->nFileTypeRecordCount ];
			fileIOGlobals->nFileTypeRecordCount++;
			
			// save example string
			PStrCopy( fileTypeRecord.szExample, fullname, MAX_FILE_NAME_LENGTH );
			
			// get pointer to extension in the copied example string we're saving
			fileTypeRecord.pszExtension = PStrGetExtension( fileTypeRecord.szExample );

		}
#endif
	}
	
	return pszExtension;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileTypeLoadedDump(
	void)
{

	TraceDebugOnly( " >>> File types loaded off disk through FileLoad() <<<" );
	
#if ISVERSION(DEBUG_VERSION)
	FILEIO_GLOBALS *fileIOGlobals = sFileIOGlobalsGet();
	for (int i = 0; i < fileIOGlobals->nFileTypeRecordCount; ++i)
	{
		const FILE_TYPE_RECORD &fileTypeRecord = fileIOGlobals->fileTypeRecords[ i ];
		TraceDebugOnly( "\t\t%s - Ex: '%s'", fileTypeRecord.pszExtension, fileTypeRecord.szExample );
	}
#endif	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (FILE_TRACE)
static inline void FileTrace(
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	vtrace(format, args);
}
#else
static inline void FileTrace(
	const char * format,
	...)
{
	REF(format);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 FileGetSize(
	HANDLE file)
{
	if (file == INVALID_FILE)
	{
		return 0;
	}

	LARGE_INTEGER size;
	if (!GetFileSizeEx(file, &size))
	{
		return 0;
	}
	return size.QuadPart;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 FileGetSize(
	const WCHAR *puszFilePath)
{
	ASSERTX_RETZERO( puszFilePath, "Expected file path" );
	HANDLE hFile = FileOpen( puszFilePath, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	if (hFile == NULL)
	{
		return 0;
	}
	UINT64 uSize = FileGetSize( hFile );
	FileClose( hFile );
	return uSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
void _FileGetFullFileName(
	T * fullname,
	const T * filename,
	int nBufLen)
{
	ASSERT_RETURN(filename);
	ASSERT_RETURN(fullname != filename);
	if (filename[0] && filename[1] == ':')
	{
		PStrCopy(fullname, filename, nBufLen);
		return;
	}

	if (filename[0] == '\\' && filename[1] == '\\')
	{
		PStrCopy(fullname, filename, nBufLen);
		return;
	}

	int rootlen;
	const OS_PATH_CHAR * rootpath = AppCommonGetRootDirectory(&rootlen);

    ASSERT_RETURN(rootlen < nBufLen);

	PStrCvt(fullname, rootpath, nBufLen);

	if(PStrLen(fullname)) // If the pstrcvt worked, proceed as normal
	{
		if (filename[0] == '\\')
		{
			PStrCopy(fullname + rootlen, filename + 1, nBufLen - rootlen);
		}
		else
		{
			PStrCopy(fullname + rootlen, filename, nBufLen - rootlen);
		}
	}
	else //if the pstrcvt failed, we should at least copy the good stuff...
		//be careful though, as this means it won't necessarily be actually copying the fullpath!
	{
		PStrCopy(fullname, filename, nBufLen);
		
	}

	T * str = fullname;
	while (*str)
	{
		if (*str == '/')
		{
			*str = '\\';
		}
		str++;
	}
}

void FileGetFullFileName(
	char * fullname,
	const char * filename,
	int nBufLen)
{
	_FileGetFullFileName(fullname, filename, nBufLen);
}

void FileGetFullFileName(
	WCHAR * fullname,
	const WCHAR * filename,
	int nBufLen)
{
	_FileGetFullFileName(fullname, filename, nBufLen);
}

void FileGetFullFileName(
	WCHAR * fullname,
	const char * filename,
	int nBufLen)
{
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	_FileGetFullFileName(fullname, wfilename, nBufLen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
BOOL _FileExists(
	const T * filename)
{
	T fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	Win32Traits<T>::WIN32_FIND_DATA tFindData;
	FindFileHandle handle = sFindFirstFile(fullname, &tFindData);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	return (tFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

BOOL FileExists(
	const char * filename)
{
	WCHAR fullname[MAX_PATH];
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	FileGetFullFileName(fullname, wfilename, MAX_PATH);

	DWORD result = GetFileAttributesW(fullname);
	if (result & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL FileExists(
	const WCHAR * filename)
{
	TIMER_START_NEEDED_EX("FileExists()", 20);

	WCHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	DWORD result = GetFileAttributesW(fullname);
	if (result & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN))
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileGetLastModifiedFileTime(
	const TCHAR * filename,
	FILETIME &tLastModified)
{
	TCHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	WIN32_FIND_DATA tFindData;
	FindFileHandle handle = FindFirstFile(fullname, &tFindData);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	tLastModified = tFindData.ftLastWriteTime;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 FileGetLastModifiedTime(
	const TCHAR * filename)
{
	FILETIME tLastModified;
	if (FileGetLastModifiedFileTime( filename, tLastModified ) == FALSE)
	{
		return 0;
	}
	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(tLastModified));
	memcpy(&temp, &tLastModified, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileGetLastModifiedSystemTime(
	const TCHAR * filename,
	SYSTEMTIME &tLastModified)
{
	FILETIME tFileTime;
	if (FileGetLastModifiedFileTime( filename, tFileTime ) == FALSE)
	{
		return FALSE;
	}
	return FileTimeToSystemTime(&tFileTime, &tLastModified);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 FileGetCurrentFileTime(
	void)
{
	FILETIME filetime;
	GetSystemTimeAsFileTime(&filetime);
	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(filetime));
	memcpy(&temp, &filetime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template < typename C >
BOOL FileRename(
	const C * curfilename,
	const C * newfilename,
	BOOL overwrite)
{
	C curfullname[MAX_PATH];
	FileGetFullFileName(curfullname, curfilename, MAX_PATH);

	C newfullname[MAX_PATH];
	FileGetFullFileName(newfullname, newfilename, MAX_PATH);	

	// don't do anything if the name isn't changing
	if ( PStrCmp( newfullname, curfullname, MAX_PATH ) == 0 )
		return TRUE;

	if (!FileExists(curfilename))
	{
		return FALSE;
	}
	if (overwrite && FileExists(newfullname))
	{
		DWORD dwFileAttributes = sGetFileAttributes(newfullname);
		dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
		if (!sSetFileAttributes(newfullname, dwFileAttributes))
		{
			return FALSE;
		}
		if (!sDeleteFile(newfullname))
		{
			return FALSE;
		}
	}
	return sMoveFile(curfullname, newfullname);
}

BOOL FileRename(const char * curfilename, const char * newfilename, BOOL overwrite)
{
	return FileRename<char>( curfilename, newfilename, overwrite );
}
BOOL FileRename(const WCHAR * curfilename, const WCHAR * newfilename, BOOL overwrite)
{
	return FileRename<WCHAR>( curfilename, newfilename, overwrite );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileDelete(
	const OS_PATH_CHAR * filename)
{
	ASSERT_RETFALSE(filename && filename[0]);

	OS_PATH_CHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, _countof(fullname));

	return OS_PATH_FUNC(DeleteFile)(fullname);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileDeleteDirectory(
	const OS_PATH_CHAR * directory,
	BOOL bDeleteAllFiles)
{
	ASSERT_RETFALSE(directory && directory[0]);

	OS_PATH_CHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, directory, arrsize(fullname));

	OS_PATH_CHAR searchpath[MAX_PATH];
	ASSERT_RETFALSE(PStrPrintf(searchpath, arrsize(searchpath), OS_PATH_TEXT("%s\\*"), fullname) > 0);

	if (bDeleteAllFiles)
	{
		OS_PATH_FUNC(WIN32_FIND_DATA) findData;
		FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)(searchpath, &findData);
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (findData.cFileName[0] && findData.cFileName[0] != '.' && findData.cFileName[1] != '.')
				{
					OS_PATH_CHAR namewithpath[MAX_PATH];
					ASSERT_RETFALSE(PStrPrintf(namewithpath, arrsize(namewithpath), OS_PATH_TEXT("%s\\%s"), fullname, findData.cFileName) > 0);

					if (!FileDelete(namewithpath))
					{
						return FALSE;
					}
				}
			} while (OS_PATH_FUNC(FindNextFile)(handle, &findData));
		}
	}
	return OS_PATH_FUNC(RemoveDirectory)(fullname);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HANDLE FileOpen(
	const char * filename,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes)
{
	WCHAR fullname[MAX_PATH];
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	FileGetFullFileName(fullname, wfilename, MAX_PATH);

	return CreateFileW(fullname, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HANDLE FileOpen(
	const WCHAR * filename,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes)
{
	WCHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	return CreateFileW(fullname, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileClose(
	HANDLE file)
{
	if (file != INVALID_FILE)
	{
		CloseHandle(file);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 FileGetPos(
	HANDLE file)
{
	if (file == INVALID_FILE)
	{
		return 0;
	}
	LARGE_INTEGER offs;
	LARGE_INTEGER pos;
	offs.QuadPart = 0;
	if (!SetFilePointerEx(file, offs, &pos, FILE_CURRENT))
	{
		return 0;
	}
	return (UINT64)pos.QuadPart;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileSetPos(
	HANDLE file,
	UINT64 pos,
	DWORD start)
{
	if (file == INVALID_FILE)
	{
		return FALSE;
	}
	LARGE_INTEGER liPos;
	liPos.QuadPart = pos;
	DWORD result = SetFilePointerEx(file, liPos, NULL, start);
	if (result == INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}
	return TRUE;;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD FileRead(
	HANDLE file,
	void * buf,
	DWORD bytes)
{
	if (file == INVALID_FILE)
	{
		return 0;
	}

	DWORD bytesread;
	if (!ReadFile(file, buf, bytes, &bytesread, NULL))
	{
		return 0;
	}

	return bytesread;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD FileWrite(
	HANDLE file,
	const void * buf,
	DWORD bytes)
{
	if (file == INVALID_FILE)
	{
		return 0;
	}
	
	DWORD byteswritten;
	if (!WriteFile(file, buf, bytes, &byteswritten, NULL))
	{
		return 0;
	}

	return byteswritten;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void * sFileLoad(MEMORY_FUNCARGS(struct MEMORYPOOL * pool, HANDLE hFile,	const char * szDbgFilename,	DWORD * size))
{
	void * data = NULL;
	void * ptr = NULL;

	ONCE
	{
		if (hFile == INVALID_FILE)
		{
			break;
		}
		
		UINT64 size64 = FileGetSize(hFile);
		ASSERTX_BREAK(size64 && size64 < (UINT64)0xfffffffe, szDbgFilename);
		DWORD dwSize = (DWORD)size64;
		
		FileTrace("FileLoad(): %d bytes %s\n", dwSize, szDbgFilename);	
		
#if ISVERSION(DEBUG_VERSION)
		sLogLoadedFileType(szDbgFilename);
#endif

		ptr = MALLOCPASSFL(pool, dwSize + 1);
		ASSERTX_BREAK(ptr, szDbgFilename);

		DWORD dwNumRead = FileRead(hFile, ptr, dwSize);
		ASSERTX_BREAK(dwNumRead == dwSize, szDbgFilename);

		*(BYTE *)((BYTE*)ptr + dwSize) = 0;
		*size = dwNumRead;

		data = ptr;
	}

	FileClose(hFile);

	if (!data && ptr)
	{
		FREE(pool, ptr);
	}

	return data;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * FileLoad(MEMORY_FUNCARGS(struct MEMORYPOOL * pool, const WCHAR * filename, DWORD * size))
{
	WCHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	HANDLE hfile = FileOpen(fullname, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);

	char szDbgFilename[MAX_PATH];
	PStrCvt(szDbgFilename, fullname, MAX_PATH);
	return sFileLoad(MEMORY_FUNC_PASSFL(pool, hfile, szDbgFilename, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * FileLoad(MEMORY_FUNCARGS(struct MEMORYPOOL * pool, const char * filename, DWORD * size))
{
	WCHAR fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	HANDLE hFile = FileOpen(fullname, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);

	char szDbgFilename[MAX_PATH];
	PStrCvt(szDbgFilename, fullname, MAX_PATH); //This may be null, but that's fine, we just won't get as good debug info - bmanegold
	return sFileLoad(MEMORY_FUNC_PASSFL(pool, hFile, szDbgFilename, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename C>
BOOL FileSave(
	const C * filename,
	void * data,
	DWORD size)
{
	C fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, MAX_PATH);

	HANDLE file = FileOpen(fullname, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if (file == INVALID_FILE)
	{
//		Error("Unable to open file: '%s' for writing", fullname);
		return FALSE;
	}

	char szFullName[ MAX_PATH ];
#ifdef USE_VS2005_STRING_FUNCTIONS
	size_t nCharsConverted;
	wcstombs_s(&nCharsConverted, szFullName, arrsize(szFullName), fullname, _TRUNCATE);
#else
	wcstombs(szFullName, fullname, arrsize(szFullName));
#endif
	FileTrace("FileSave(): %s\n", szFullName);

	ONCE
	{
		DWORD dwBytesWritten = FileWrite(file, data, size);
		ASSERTX_BREAK(dwBytesWritten == size, "FileWrite() did not write the expected number of bytes to disk" );
	}

	FileClose(file);
	return TRUE;
}
BOOL FileSave( const char * filename, void * data, DWORD size)
{
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	return FileSave<WCHAR>(wfilename, data, size);
}

BOOL FileSave( const WCHAR * filename, void * data, DWORD size)
{
	return FileSave<WCHAR>(filename, data, size);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T>
static
BOOL _FileDirectoryExists(
	const T * directoryname)
{
	T fullname[MAX_PATH];
	FileGetFullFileName(fullname, directoryname, _countof(fullname));

	// Remove trailing backslash, if any.
	unsigned nLen = PStrLen(fullname, _countof(fullname));
	if ((nLen > 0) && (fullname[nLen - 1] == '\\'))
	{
		fullname[nLen - 1] = '\0';
	}

	Win32Traits<T>::WIN32_FIND_DATA tFileFindData;
	FindFileHandle handle = sFindFirstFile(fullname, &tFileFindData);
	if (handle == INVALID_FILE)
	{
		return FALSE;
	}
	return (tFileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

BOOL FileDirectoryExists(
	const char * directoryname)
{
	WCHAR wdirname[MAX_PATH];
	PStrCvt(wdirname, directoryname, MAX_PATH);
	return _FileDirectoryExists(wdirname);
}

BOOL FileDirectoryExists(
	const WCHAR * directoryname)
{
	return _FileDirectoryExists(directoryname);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename C>
BOOL FileCreateDirectory(
	const C * directoryname)
{
	C fullname[MAX_PATH];
	FileGetFullFileName(fullname, directoryname, MAX_PATH);

	if ( ! sCreateDirectory(fullname) )
	{  // this gets more complicated in case there is another folder in the path missing
		if ( GetLastError() == ERROR_ALREADY_EXISTS )
			return TRUE;

		int nLength = PStrLen( fullname );
		if ( ! nLength || nLength > MAX_PATH || (nLength < 0)  )
			return FALSE;
		// if we're trying to make the root dir, just allow it. this isn't caught by the
		// above condition because the error returned is ERROR_ACCESS_DENIED
		if ( (nLength == 1 && fullname[0] == (C)('\\')) ||
		     (nLength < 3 && fullname[1] == (C)(':')))
			return TRUE;

		if ( fullname[ nLength - 1 ] == (C)('\\') ||
			 fullname[ nLength - 1 ] == (C)('/') )
		{
			fullname[ nLength - 1 ] = (C)('\0');
		}
		for ( int i = nLength - 1; i > 0; i-- )
		{
			if ( fullname[ i ] == (C)('\\') ||
				 fullname[ i ] == (C)('/') )
			{
				fullname[ i ] = (C)('\0');
				break;
			}
		}
		if ( ! FileCreateDirectory ( fullname ) )
			return FALSE;
		FileGetFullFileName(fullname, directoryname, MAX_PATH);
		return sCreateDirectory(fullname);
	}
	return TRUE;
}

BOOL FileCreateDirectory(const char * directoryname)
{
	WCHAR wdirname[MAX_PATH];
	PStrCvt(wdirname, directoryname, MAX_PATH);
	return FileCreateDirectory<WCHAR>(wdirname);
}
BOOL FileCreateDirectory(const WCHAR * directoryname)
{
	return FileCreateDirectory<WCHAR>(directoryname);
}
//----------------------------------------------------------------------------
// returns FALSE if file doesn't exist
//----------------------------------------------------------------------------
template<class T, class F>
static
BOOL _FileIsReadOnly(
	const T * filename, F * pFunc)
{
	T fullname[MAX_PATH];
	FileGetFullFileName(fullname, filename, _countof(fullname));

	// guess at the source control status by the read-only flag
	DWORD dwAttributes = (*pFunc)(fullname);
	return (dwAttributes != INVALID_FILE_ATTRIBUTES) && (dwAttributes & FILE_ATTRIBUTE_READONLY);
}

BOOL FileIsReadOnly(const char * filename)
{
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	return _FileIsReadOnly(wfilename, &GetFileAttributesW);
}

BOOL FileIsReadOnly(const WCHAR * filename)
{
	return _FileIsReadOnly(filename, &GetFileAttributesW);
}


//----------------------------------------------------------------------------
// perforce integration, check-out or add
//----------------------------------------------------------------------------
void FileCheckOut(
	const TCHAR * filenameIn)
{
#if ISVERSION(DEVELOPMENT)
	enum
	{
		MAX_COMMAND_LEN	=		32
	};

	OS_PATH_CHAR filename[MAX_PATH];
	PStrCvt(filename, filenameIn, _countof(filename));

	OS_PATH_CHAR fullfilename[MAX_PATH];
	FileGetFullFileName(fullfilename, filename, _countof(filename));

	OS_PATH_CHAR foldername[MAX_PATH];
	PStrGetPath( foldername, MAX_PATH, fullfilename );

	{
		char sSearch[MAX_PATH];
		PStrCvt(sSearch, fullfilename, _countof(filename));

		WIN32_FIND_DATA tFindData;
		FindFileHandle handle = FindFirstFile( sSearch, &tFindData );
		if (handle != INVALID_HANDLE_VALUE)
		{	
			PStrCvt(filename, tFindData.cFileName, _countof(filename));
		} else {
			return;
		}
	}

	const int nMaxLen = DEFAULT_FILE_WITH_PATH_SIZE + MAX_COMMAND_LEN;
	OS_PATH_CHAR szCommand[nMaxLen];

	OS_PATH_CHAR szPath[MAX_PATH];
#ifdef _WIN64
	int nCSIDLFolder = CSIDL_PROGRAM_FILESX86;
#else
	int nCSIDLFolder = CSIDL_PROGRAM_FILES;
#endif
	OS_PATH_FUNC(SHGetFolderPath)( NULL, nCSIDLFolder, NULL, 0, szPath );
	PStrForceBackslash( szPath, _countof(szPath) );
	static const OS_PATH_CHAR sgszSCC[] = OS_PATH_TEXT("Perforce\\p4");

	if (FileIsReadOnly(fullfilename))
	{
		// if file is read-only attempt to check it out
		static const OS_PATH_CHAR sgszEdit[] = OS_PATH_TEXT("edit");
		PStrPrintf(szCommand, nMaxLen, OS_PATH_TEXT("\"%s%s\" %s \"%s\""), szPath, sgszSCC, sgszEdit, filename);
	}
	else
	{
		// else add to source control
		static const OS_PATH_CHAR sgszAdd[] = OS_PATH_TEXT("add");
		PStrPrintf(szCommand, nMaxLen, OS_PATH_TEXT("\"%s%s\" %s \"%s\""), szPath, sgszSCC, sgszAdd, filename);
	}
	trace("%s\n", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szCommand));

	char szDir[MAX_PATH];
	GetCurrentDirectory(arrsize(szDir), szDir);
	OS_PATH_CHAR szDirConverted[MAX_PATH];
	PStrCvt(szDirConverted, szDir, arrsize(szDirConverted));

	PSetCurrentDirectory( foldername );

	PExecuteCmdLine( szCommand, CMDLINE_HIDE_WINDOW );

	PSetCurrentDirectory( szDirConverted );

#else
	REF(filenameIn);
#endif // DEVELOPMENT
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileDebugCreateAndCompareHistory( 
	const char *filename)
{
	if (sgbGenerateHistoryFiles == FALSE)
	{
		return;
	}
	
	// find next free version
	const int MAX_VERSIONS = KILO * KILO;
	int nVersion = 1;
	char szHistoryFilename[ MAX_PATH ];	
	for (int i = nVersion; i < MAX_VERSIONS; ++i)
	{
		PStrPrintf( szHistoryFilename, MAX_PATH, "%s.history.%d", filename, i );
		if (FileExists( szHistoryFilename ) == FALSE)
		{
			nVersion = i;
			break;
		}		
	}
	
	// copy cooked file as history file
	CopyFile( filename, szHistoryFilename, TRUE );
	
	// compare all the history files with the cooked one
	DWORD dwSize;
	BYTE *pData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &dwSize));
	
	for (int i = 1; i <= nVersion; ++i)
	{
		char szFilename[ MAX_PATH ];
		PStrPrintf( szFilename, MAX_PATH, "%s.history.%d", filename, i );
		DWORD dwSizeHistory;
		BYTE *pDataHistory = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, szFilename, &dwSizeHistory));
		
		ASSERT_CONTINUE( pData && pDataHistory && dwSize == dwSizeHistory );
		
		for (DWORD j = 0; j < dwSize; ++j)
		{
			ASSERT( pData[ j ] == pDataHistory[ j ] );
		}
		
		FREE( NULL, pDataHistory );
	}
	
	FREE( NULL, pData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FileTouch(
	const char *pszFilename)
{

	// open file and get handle
	HANDLE hFile = FileOpen( pszFilename, GENERIC_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL );
	
	// get current system time as file time and set the modification time on the file
	FILETIME tCurrentTime;
	GetSystemTimeAsFileTime( &tCurrentTime );
	SetFileTime( hFile, NULL, &tCurrentTime, &tCurrentTime );

	// close handle	
	FileClose( hFile );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct FILES_ITERATE_INTERNAL_CONTEXT
{
	FILE_ITERATE_RESULT*	data;
	unsigned int*			dataCount;
	unsigned int			index;
};


void FilesIterateInternalCallback(const FILE_ITERATE_RESULT *pResult, void *pUserData)
{
	FILES_ITERATE_INTERNAL_CONTEXT* context = reinterpret_cast<FILES_ITERATE_INTERNAL_CONTEXT*>(pUserData);
	
	if(context->data == NULL)
	{
		++(*context->dataCount);
	}
	else if(context->index < *context->dataCount)
	{
		context->data[context->index] = *pResult;				
		++context->index;
	}
}


void FilesIterateInDirectory(
	const WCHAR *puszDirectory, 
	const WCHAR *puszExtensionMatch, 
	BOOL bLookInSubFolders,
	FILE_ITERATE_RESULT* results,
	unsigned int& resultsCount)
{
	FILES_ITERATE_INTERNAL_CONTEXT context;
	context.data = results;
	context.dataCount = &resultsCount;
	context.index = 0;
	
	FilesIterateInDirectory(puszDirectory, puszExtensionMatch, FilesIterateInternalCallback, &context, bLookInSubFolders);
}

void FilesIterateInDirectory(
	const char *pszDirectory,
	const char *pszExtensionMatch,
	FILE_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData,
	BOOL bLookInSubFolders )
{
	WCHAR dir[MAX_PATH];
	PStrCvt(dir, pszDirectory, MAX_PATH);

	WCHAR ext[MAX_PATH];
	PStrCvt(ext, pszExtensionMatch, MAX_PATH);
	FilesIterateInDirectory(dir, ext, pfnCallback	, pCallbackData, bLookInSubFolders);
}

void FilesIterateInDirectory(
	const WCHAR *puszDirectory,
	const WCHAR *puszExtensionMatch,
	FILE_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData,
	BOOL bLookInSubFolders )
{
	ASSERTX_RETURN( puszDirectory, "Expected directory string" );
	ASSERTX_RETURN( puszExtensionMatch, "Expected extension string" );	
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	
	// fix the directory name
	WCHAR uszDirectoryFixed[ MAX_PATH ];
	PStrCopy( uszDirectoryFixed, puszDirectory, MAX_PATH );
	PStrReplaceChars( uszDirectoryFixed, L'/', L'\\' );
	PStrForceBackslash( uszDirectoryFixed, MAX_PATH );
	
	// create search pattern string
	WCHAR uszFileSearchPattern[ MAX_PATH ];

	char szDirectoryFixed[MAX_PATH];
	PStrCvt(szDirectoryFixed, uszDirectoryFixed, MAX_PATH);

	PStrPrintf( uszFileSearchPattern, MAX_PATH, L"%s%s", uszDirectoryFixed, puszExtensionMatch );

	{
		WIN32_FIND_DATAW tFindData;
		FindFileHandle handle = FindFirstFileW( uszFileSearchPattern, &tFindData );
		if (handle != INVALID_HANDLE_VALUE)
		{	
			do
			{
				// fill out result for file
				FILE_ITERATE_RESULT tResult;
				PStrCvt( tResult.szFilename, tFindData.cFileName, MAX_PATH );
				PStrPrintf( 
					tResult.szFilenameRelativepath, 
					MAX_PATH,
					"%s%s", 
					szDirectoryFixed, 
					tResult.szFilename );
				
				// do the callback
				pfnCallback( &tResult, pCallbackData );
			} while (FindNextFileW( handle, &tFindData ));
		}
	}
	
	if (bLookInSubFolders)
	{
		PStrPrintf( uszFileSearchPattern, MAX_PATH, L"%s*", uszDirectoryFixed, puszExtensionMatch );

		WIN32_FIND_DATAW tFindData;
		structclear(tFindData);
		tFindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

		FindFileHandle handle = FindFirstFileW( uszFileSearchPattern, &tFindData );
		if (handle != INVALID_HANDLE_VALUE)
		{	
			do
			{
				if ( tFindData.cFileName[ 0 ] != L'.' && tFindData.cFileName[ 1 ] != L'.' )
				{
					PStrPrintf( uszFileSearchPattern, MAX_PATH, L"%s%s\\", uszDirectoryFixed, tFindData.cFileName );
					FilesIterateInDirectory( uszFileSearchPattern, puszExtensionMatch, pfnCallback, pCallbackData, bLookInSubFolders );
				}

			} while (FindNextFileW(handle, &tFindData));
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FileParseSimpleToken(
	const WCHAR *puszFilename,
	const char *pszIdentifier,
	char *pszTokenResult,
	int nMaxTokenLength)
{
	BOOL bSuccess = FALSE;

	// parse the first line of a text file that is in the format of
	// 		IDENTIFIER=TOKEN
	// 			such as
	// 				Language=Korean
	// 					or
	// 				Region=NorthAmerica
		
	// load file	
	DWORD dwFileSize = 0;
	const char *pFileData = (const char *)FileLoad(MEMORY_FUNC_FILELINE(NULL, puszFilename, &dwFileSize));
	if (pFileData)
	{

		// look for language token with poor mans parsing
		const char *szSeps = " =";
		
		// poor mans parsing to find "IDENTIFIER=TOKEN", such as
		const int MAX_TOKEN = 256;
		char szToken[ MAX_TOKEN ];
		const char *pBuffer = pFileData;
		int nBufferLen = PStrLen( pBuffer );
		
		// read "IDENTIFIER="
		pBuffer = PStrTok( szToken, pBuffer, szSeps, MAX_TOKEN, nBufferLen );
		ASSERTX( pBuffer, "Invalid simple token file" );
		if (pBuffer && PStrICmp( szToken, pszIdentifier ) == 0)
		{

			// Now that we're past the "IDENTIFIER=", the remainder of the buffer is the token, ghetto eh? :)
			// copy result to token
			PStrCopy( pszTokenResult, pBuffer, nMaxTokenLength );
			bSuccess = TRUE;
		}
		else
		{
			ASSERTV( pBuffer, "Invalid simple token file, expected '%s'", pszIdentifier );
		}
					
		// done with file data
		FREE( NULL, pFileData );
		pFileData = NULL;

	}

	return bSuccess;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR *FileGetBaseName(
	WCHAR *szPath)
{
	for(UINT i = PStrLen(szPath); i > 0; i--)
	{
		if(szPath[i] == '\\' || szPath[i] == '/')
		{
			WCHAR * szTemp = &szPath[i+1];
			return szTemp;
		}

	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char *FileGetBaseName(
	char *szPath)
{
	for(UINT i = 0; i < PStrLen(szPath); i++)
	{
		if(szPath[i] == '\\')
			return FileGetBaseName(&szPath[i+1]);
	}
	return szPath;
}
