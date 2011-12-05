//----------------------------------------------------------------------------
// fileio.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _FILEIO_H_
#define _FILEIO_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _ASYNCFILE_H_
#include "asyncfile.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct FILE_HEADER
{
	DWORD					dwMagicNumber;
	int						nVersion;
};

#define DEFAULT_SERIALIZED_FILE_SECTION_COUNT 10
struct SERIALIZED_FILE_SECTION
{
	int nSectionType;
	DWORD dwOffset;
	DWORD dwSize;
};

#define SERIALIZED_FILE_HEADER_VERSION		1
struct SERIALIZED_FILE_HEADER : FILE_HEADER
{
	int nSFHVersion;
	int	nNumSections;
	SERIALIZED_FILE_SECTION* pSections;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class FindFileHandle
{
private:
	HANDLE					m_Handle;

public:
	FindFileHandle & operator = (
		HANDLE handle)
	{
		if (m_Handle != INVALID_HANDLE_VALUE)
		{
			FindClose(m_Handle);
		}
		m_Handle = handle;
		return *this;
	}

	operator HANDLE () 
	{ 
		return m_Handle; 
	}

	FindFileHandle(
		void) : m_Handle(INVALID_HANDLE_VALUE)
	{
	}

	FindFileHandle(
		HANDLE handle) : m_Handle(handle)
	{
	}

	~FindFileHandle(
		void)
	{
		*this = INVALID_HANDLE_VALUE;
	}
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void FileIOInit(
	void);

void FileIOFree(
	void);
	
void FileTypeLoadedDump(
	void);

UINT64 FileGetSize(
	HANDLE file);

UINT64 FileGetSize(
	const WCHAR *puszFilePath);

BOOL FileExists(
	const char * filename);

BOOL FileExists(
	const WCHAR * filename);

HANDLE FileOpen(
	const char * filename,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes);

HANDLE FileOpen(
	const WCHAR * filename,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes);

void FileClose(
	HANDLE file);

UINT64 FileGetPos(
	HANDLE file);

BOOL FileSetPos(
	HANDLE file,
	UINT64 pos,
	DWORD start = FILE_BEGIN);

DWORD FileRead(
	HANDLE file,
	void * buf,
	DWORD bytes);

DWORD FileWrite(
	HANDLE file,
	const void * buf,
	DWORD bytes);

void * FileLoad(MEMORY_FUNCARGS(struct MEMORYPOOL * pool, const WCHAR * filename, DWORD * size));
void * FileLoad(MEMORY_FUNCARGS(struct MEMORYPOOL * pool, const char * filename, DWORD * size));

template <typename C>
BOOL FileSave(
	const C * filename,
	void * data,
	DWORD size);

BOOL FileDirectoryExists(
	const char * directoryname);

BOOL FileDirectoryExists(
	const WCHAR * directoryname);

BOOL FileCreateDirectory(
	const char * directoryname);

BOOL FileCreateDirectory(
	const WCHAR * directoryname);

UINT64 FileGetLastModifiedTime(
	const TCHAR * filename);

BOOL FileGetLastModifiedFileTime(
	const TCHAR * filename,
	FILETIME &tLastModified);

BOOL FileGetLastModifiedSystemTime(
	const TCHAR * filename,
	SYSTEMTIME &tLastModified);

UINT64 FileGetCurrentFileTime(
	void);

BOOL FileRename(
	const char * curfilename, 
	const char * newfilename, 
	BOOL overwrite);

BOOL FileRename(
	const WCHAR * curfilename, 
	const WCHAR * newfilename, 
	BOOL overwrite);

BOOL FileDelete(
	const OS_PATH_CHAR * filename);

BOOL FileDeleteDirectory(
	const OS_PATH_CHAR * directory,
	BOOL bDeleteAllFiles);

BOOL FileIsReadOnly( 
	const char * filename);

BOOL FileIsReadOnly( 
	const WCHAR * filename);

void FileCheckOut(
	const TCHAR * filename);

void FileGetFullFileName(
	WCHAR * fullname,
	const WCHAR * filename,
	int nBufLen);

void FileGetFullFileName(
	char * fullname,
	const char * filename,
	int nBufLen);

void FileGetFullFileName(
	WCHAR * fullname,
	const char * filename,
	int nBufLen);

void FileDebugCreateAndCompareHistory( 
	const char *filename);

void FileTouch(
	const char *pszFilename);

BOOL FileParseSimpleToken(
	const WCHAR *puszFilename,
	const char *pszIdentifier,
	char *pszToken,
	int nMaxTokenLength);

WCHAR *FileGetBaseName(
	WCHAR *szPath);

char *FileGetBaseName(
	char *szPath);

//----------------------------------------------------------------------------
struct FILE_ITERATE_RESULT
{
	char szFilename[ MAX_PATH ];				// filename only
	char szFilenameRelativepath[ MAX_PATH ];	// filename with relative path
};
typedef void (* FILE_ITERATE_CALLBACK)( const FILE_ITERATE_RESULT *pResult, void *pUserData );

void FilesIterateInDirectory(
	const WCHAR *puszDirectory, 
	const WCHAR *puszExtensionMatch, 
	BOOL bLookInSubFolders,
	FILE_ITERATE_RESULT* results,
	unsigned int& resultsCount);

void FilesIterateInDirectory(
	const char *pszDirectory,
	const char *pszExtensionMatch,
	FILE_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData,
	BOOL bLookInSubfolders = FALSE );

void FilesIterateInDirectory(
	const WCHAR *puszDirectory,
	const WCHAR *puszExtensionMatch,
	FILE_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData,
	BOOL bLookInSubfolders = FALSE );


//----------------------------------------------------------------------------
//  class WRITEFILE_BUF
//----------------------------------------------------------------------------
template <unsigned int bufsize>
class WRITEFILE_BUF
{
private:
	BYTE				buf[bufsize];											// byte buffer
	unsigned int		cur;													// current byte
	UINT64				pos;													// position in file
	HANDLE				file;													// file handle
	TCHAR				filename[MAX_PATH];										// file name

public:
	WRITEFILE_BUF(
		void)
	{
		cur = 0;
		pos = 0;
		file = INVALID_HANDLE_VALUE;
		filename[0] = 0;
	}

	BOOL Flush(
		void)
	{
		ASSERT_RETFALSE(file != INVALID_HANDLE_VALUE);
		if (cur == 0)
		{
			return TRUE;
		}
		ASSERT_RETFALSE(cur == FileWrite(file, buf, cur));
		cur = 0;
		return TRUE;
	}

	void Close(
		void)
	{
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}
		Flush();
		FileClose(file);
		file = INVALID_HANDLE_VALUE;
		filename[0] = 0;
		pos = 0;
	}

	~WRITEFILE_BUF(
		void)
	{
		Close();
	}

	BOOL IsOpen(
		void)
	{
		return (file != INVALID_HANDLE_VALUE);
	}

	UINT64 GetPos(
		void)
	{
		if (file == INVALID_HANDLE_VALUE)
		{
			return 0;
		}
		return pos;
	}

	BOOL SetPos(
		UINT64 pos_)
	{
		if (file == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		Flush();
		if (FileSetPos(file, pos_, FILE_BEGIN))
		{
			pos = pos_;
			return TRUE;
		}
		else
		{
			pos = FileGetPos(file);
			return FALSE;
		}
	}

	BOOL Open(
		const TCHAR * filename_,
		const TCHAR * directory = NULL)
	{
		Close();

		if (directory && directory[0])
		{
			FileCreateDirectory(directory);
			PStrPrintf(filename, MAX_PATH, _T("%s%s"), directory, filename_);
		}
		else
		{
			PStrCopy(filename, filename_, MAX_PATH);
		}
		file = FileOpen(filename, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN);
		ASSERTV_RETFALSE( file != INVALID_HANDLE_VALUE, "Unable to open file [%s] for write.", filename);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value)
	{
		if (cur + (unsigned int)sizeof(T) > bufsize)
		{
			Flush();
		}
		ASSERT_RETFALSE(cur + (unsigned int)sizeof(T) <= bufsize);
		*(T *)(buf + cur) = value;
		cur += sizeof(T);
		pos += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value,
		unsigned int bytes)
	{
		if (bytes > sizeof(T))
		{
			ASSERTX(0, "invalid size passed to WRITEFILE_BUF::PushInt()");
			bytes = sizeof(T);
		}
#ifdef _BIG_ENDIAN
		ASSERTX_RETFALSE(0, "WRITEFILE_BUF::PushInt() for Big Endian not implemented.");
#endif
		if (bytes == sizeof(T))
		{
			return PushInt(value);
		}
		return PushBuf(&value, bytes);
	}

	inline BOOL PushBuf(
		const void * buffer,
		unsigned int length)
	{
		BYTE * src = (BYTE *)buffer;
		while (length)
		{
			unsigned int tocopy = bufsize - cur;
			if (tocopy > length)
			{
				tocopy = length;
			}
			memcpy(buf + cur, src, tocopy);
			src += tocopy;
			cur += tocopy;
			pos += tocopy;
			length -= tocopy;

			if (length > 0)
			{
				if (!Flush())
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}
};


//----------------------------------------------------------------------------
//  class WRITE_BUF, pass in a static sized buffer
//----------------------------------------------------------------------------
class WRITE_BUF
{
private:
	BYTE *				buf;													// byte buffer
	unsigned int		bufsize;												// size of buf
	unsigned int		pos;													// current byte

public:
	WRITE_BUF(
		BYTE * _buf, unsigned int _bufsize) : buf(_buf), bufsize(_bufsize), pos(0)
	{
	}

	unsigned int GetPos(
		void)
	{
		return pos;
	}

	BOOL SetPos(
		unsigned int _pos)
	{
		ASSERT_RETFALSE(_pos <= bufsize);
		pos = _pos;
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value)
	{
		if (buf)
		{
			ASSERT_RETFALSE(pos + (unsigned int)sizeof(T) <= bufsize);
			*(T *)(buf + pos) = value;
		}
		pos += sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value,
		unsigned int bytes)
	{
		if (bytes > sizeof(T))
		{
			ASSERTX(0, "invalid size passed to WRITE_BUF::PushInt()");
			bytes = sizeof(T);
		}
#ifdef _BIG_ENDIAN
		ASSERTX_RETFALSE(0, "WRITEFILE_BUF::PushInt() for Big Endian not implemented.");
#endif
		if (bytes == sizeof(T))
		{
			return PushInt(value);
		}
		return PushBuf(&value, bytes);
	}

	inline BOOL PushBuf(
		const void * buffer,
		unsigned int length)
	{
		if (buf)
		{
			ASSERT_RETFALSE(pos + length <= bufsize);
			memcpy(buf + pos, buffer, length);
		}
		pos += length;
		return TRUE;
	}

	BOOL Save(
		const TCHAR * filename_,
		const TCHAR * directory = NULL)
	{
		ASSERT_RETFALSE(buf);

		TCHAR filename[MAX_PATH];
		if (directory && directory[0])
		{
			FileCreateDirectory(directory);
			PStrPrintf(filename, arrsize(filename), _T("%s%s"), directory, filename_);
		}
		else
		{
			PStrCopy(filename, filename_, arrsize(filename));
		}
		ASSERT_RETFALSE(FileSave(filename, buf, pos));
		return TRUE;
	}
};


//----------------------------------------------------------------------------
//  class WRITE_BUF, grows buffer by powers of 2
//----------------------------------------------------------------------------
class WRITE_BUF_DYNAMIC
{
private:
	struct MEMORYPOOL * pool;													// memory pool
	BYTE *				buf;													// byte buffer
	unsigned int		bufsize;												// size of buf
	unsigned int		pos;													// current byte

	template<class T>
	const T * CreateDirectoryAndFileName(
		T * fullfilename,
		unsigned int buflen,
		const T * filename,
		const T * directory)
	{
		ASSERT_RETNULL(fullfilename && filename && filename[0]);

		if (directory && directory[0])
		{
			FileCreateDirectory(directory);
			PStrCopy(fullfilename, directory, buflen);
			PStrCat(fullfilename, filename, buflen);
			return fullfilename;
		}
		else
		{
			return filename;
		}
	}

public:
	WRITE_BUF_DYNAMIC(
		struct MEMORYPOOL * _pool = NULL) : pool(_pool), pos(0)
	{
		bufsize = 256;
		buf = (BYTE *)MALLOC(pool, bufsize);
		if (!buf)
		{
			bufsize = 0;
			ASSERT(buf);
		}
	}

	~WRITE_BUF_DYNAMIC(
		void)
	{
		if (buf)
		{
			FREE(pool, buf);
			buf = NULL;
		}
	}

	unsigned int GetPos(
		void)
	{
		return pos;
	}

	BOOL SetPos(
		unsigned int _pos)
	{
		ASSERT_RETFALSE(_pos <= bufsize);
		pos = _pos;
		return TRUE;
	}

	void * GetBuf(
		void)
	{
		return buf;	
	}

	// get the buffer and don't let the class auto-free the buffer on destruct
	void * KeepBuf(
		unsigned int & len)
	{
		len = pos;
		void * buffer = buf;
		if (buf)
		{
			buf = NULL;
			bufsize = 0;
			pos = 0;
		}
		return buffer;
	}

	BOOL Resize(
		unsigned int bytes)
	{
		if (pos + bytes <= bufsize)
		{
			return TRUE;
		}
		while (pos + bytes >= bufsize)
		{
			bufsize *= 2;
		}
		buf = (BYTE *)REALLOC(pool, buf, bufsize);
		if (!buf)
		{
			bufsize = 0;
			ASSERT_RETFALSE(buf);
		}
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value)
	{
		ASSERT_RETFALSE(Resize((unsigned int)sizeof(T)));
		ASSERT_RETFALSE(pos + (unsigned int)sizeof(T) <= bufsize);
		*(T *)(buf + pos) = value;
		pos += (unsigned int)sizeof(T);
		return TRUE;
	}

	template <typename T>
	inline BOOL PushInt(
		T value,
		unsigned int bytes)
	{
		if (bytes > sizeof(T))
		{
			ASSERTX(0, "invalid size passed to WRITE_BUF::PushInt()");
			bytes = sizeof(T);
		}
#ifdef _BIG_ENDIAN
		ASSERTX_RETFALSE(0, "WRITEFILE_BUF::PushInt() for Big Endian not implemented.");
#endif
		if (bytes == sizeof(T))
		{
			return PushInt(value);
		}
		return PushBuf(&value, bytes);
	}

	inline BOOL PushBuf(
		const void * buffer,
		unsigned int length )
	{
		ASSERT_RETFALSE(Resize(length));
		ASSERT_RETFALSE(pos + length <= bufsize);
		ASSERT_RETFALSE(MemoryCopy(buf + pos, bufsize - pos, buffer, length));
		pos += length;
		return TRUE;
	}

	template<class T>
	BOOL SaveNow(
		const T * filename,
		const T * directory = NULL)
	{
		ASSERT_RETFALSE(buf);

		T str[MAX_PATH];
		const T * fullfilename = CreateDirectoryAndFileName(str, arrsize(str), filename, directory);
		ASSERTVONCE_RETFALSE(fullfilename, "WRITE_BUF_DYNAMIC::SaveNow() FAILED TO CREATE DIRECTORY AND/OR FILE -- DIRNAME: %s  FILE: %s\n", directory, filename);
		ASSERTVONCE_RETFALSE(FileSave(filename, buf, pos), "WRITE_BUF_DYNAMIC::SaveNow() FAILED TO SAVE FILE -- DIRNAME: %s  FILE: %s\n", directory, filename);
		return TRUE;
	}

	BOOL Save(
		const TCHAR * filename,
		const TCHAR * directory = NULL,
		UINT64 * gentime = NULL)
	{
		ASSERT_RETFALSE(buf);

		TCHAR str[MAX_PATH];
		const TCHAR * fullfilename = CreateDirectoryAndFileName(str, arrsize(str), filename, directory);
		ASSERTVONCE_RETFALSE(fullfilename, "WRITE_BUF_DYNAMIC::Save() FAILED TO CREATE DIRECTORY AND/OR FILE -- DIRNAME: %s  FILE: %s\n", directory, filename);
		ASSERTVONCE_RETFALSE(AsyncWriteFile(filename, buf, pos, gentime), "WRITE_BUF_DYNAMIC::Save() FAILED TO SAVE FILE -- DIRNAME: %s  FILE: %s\n", directory, filename);
		return TRUE;
	}
};


#endif // _FILEIO_H_