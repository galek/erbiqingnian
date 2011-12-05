//----------------------------------------------------------------------------
// pakfile.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PAKFILE_H_
#define _PAKFILE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _ASYNCFILE_H_
#include "asyncfile.h"
#endif

#include "fileio.h"

#ifndef _SHA1_H_
#include "sha1.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
// extensions and magic numbers
#define NO_MAGIC						(DWORD)(-1)
#define NO_VERSION						(-1)


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define DECLARE_LOAD_SPEC(s, fn)		PAKFILE_LOAD_SPEC s(fn, __FILE__, __LINE__);
#else
#define DECLARE_LOAD_SPEC(s, fn)		PAKFILE_LOAD_SPEC s(fn);
#endif


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// pakfile types
//----------------------------------------------------------------------------
enum PAK_ENUM
{
	PAK_INVALID = -1,
	
	PAK_DEFAULT,
	PAK_GRAPHICS_HIGH,
	PAK_SOUND,
	PAK_SOUND_HIGH,
	PAK_SOUND_LOW,
	PAK_SOUND_MUSIC,
	PAK_LOCALIZED,
	PAK_MOVIES,
	PAK_GRAPHICS_HIGH_BACKGROUNDS,
	PAK_GRAPHICS_HIGH_PLAYERS,
	PAK_MOVIES_HIGH,
	PAK_MOVIES_LOW,
	NUM_PAK_ENUM
};

//----------------------------------------------------------------------------
// flags passed to FileNeedsUpdate()
//----------------------------------------------------------------------------
enum
{
	FILE_UPDATE_UPDATE_IF_NOT_IN_PAK	  =	MAKE_MASK(0),		// always update the file if it isn't in the pakfile
	FILE_UPDATE_ALWAYS_CHECK_DIRECT		  =	MAKE_MASK(1),		// always compare the source file vs. cooked or pakfile versions
	FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG =	MAKE_MASK(2),		// test for ADF_ALLOW_VERSION_UPDATE app debug flag
	FILE_UPDATE_OPTIONAL_FILE			  = MAKE_MASK(3),		// This file is optional, so don't error or warn if the file is missing
};


//----------------------------------------------------------------------------
// flags used in PAKFILE_LOAD_SPEC
//----------------------------------------------------------------------------
enum
{
	PAKFILE_LOAD_IMMEDIATE =				MAKE_MASK(0),	// force a synchronous load
	PAKFILE_LOAD_FORCE_FROM_PAK =			MAKE_MASK(1),	// force load from pak
	PAKFILE_LOAD_FORCE_FROM_DISK =			MAKE_MASK(2),	// force load from disk
	PAKFILE_LOAD_FREE_BUFFER =				MAKE_MASK(3),	// set if buffer is allocated by pakfile system, or we wish to auto free it in event of an error
	PAKFILE_LOAD_FREE_CALLBACKDATA =		MAKE_MASK(4),	// set to free callback data after all callbacks have been called or in event of an error
	PAKFILE_LOAD_ADD_TO_PAK =				MAKE_MASK(5),	// add the file to the pakfile once done
	PAKFILE_LOAD_WARN_IF_MISSING =			MAKE_MASK(6),	// warn if the file was missing, unless some global data warnings is off
	PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING =	MAKE_MASK(7),	// always warn if the file is missing
	PAKFILE_LOAD_IGNORE_BUFFER_SIZE = 		MAKE_MASK(8),	// we pass in a buffer & size, but don't error if size is too small (used for sound streaming)
	PAKFILE_LOAD_DUMMY_CALLBACK =			MAKE_MASK(9),	// create a request etc, as a dummy callback
	PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND =	MAKE_MASK(10),	// call the callback even if the file wasn't found
	PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE =		MAKE_MASK(11),	// Force add to pak immediately (requires ADD_TO_PAK flag to actually add it)
	PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS =	MAKE_MASK(12),  // Don't compress the file as it's added to the pakfile (compress by default)
	PAKFILE_LOAD_DONT_ADD_TO_FILL_XML =		MAKE_MASK(13),  // Don't add this file to the list of start-up file loads
	PAKFILE_LOAD_NODIRECT_IGNORE_MISSING =	MAKE_MASK(14),	// If loading -nodirect (pak-only), don't treat this missing file as an error
	PAKFILE_LOAD_FORCE_ALLOW_PAK_ADDING =	MAKE_MASK(15),	// Allow patch client to add to pak, even under release builds
	PAKFILE_LOAD_FILE_ALREADY_COMPRESSED =	MAKE_MASK(16),	// There's already a compressed buffer
	PAKFILE_LOAD_FILE_ALREADY_HASHED =		MAKE_MASK(17),	// There's already a hash
	PAKFILE_LOAD_OPTIONAL_FILE =			MAKE_MASK(18),	// This file is optional, so don't error or warn if the file is missing
	PAKFILE_LOAD_GIANT_FILE =				MAKE_MASK(19),	// This file is gigantic, so don't assert that it's "too big"
	PAKFILE_LOAD_NEVER_PUT_IN_PAK =			MAKE_MASK(20),	// Never put this file in a pak, no matter what
	PAKFILE_LOAD_DONT_LOAD_FROM_PAK =		MAKE_MASK(21),	// Don't load this file if it's already in the pak (used while fillpak)
	PAKFILE_LOAD_WAS_IN_PAK =				MAKE_MASK(22),	// returned by pakfile load if the file was in the pak
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct PAKFILE;

//----------------------------------------------------------------------------
// pakfile base name description (lists base file prefixes)
// i.e. "hellgate", "hellgate_sound"
//----------------------------------------------------------------------------
struct PAKFILE_DESC
{
	PAK_ENUM					m_Index;											// base name index
	TCHAR						m_BaseName[DEFAULT_FILE_WITH_PATH_SIZE];			// base pakfile name
	TCHAR						m_BaseNameX[DEFAULT_FILE_WITH_PATH_SIZE];			// base pakfile name for patches
	TCHAR						m_BaseNameSP[DEFAULT_FILE_WITH_PATH_SIZE];			// base pakfile name for patches for SP
	TCHAR						m_BaseNameMP[DEFAULT_FILE_WITH_PATH_SIZE];			// base pakfile name for patches for MP
	unsigned int				m_Count;											// number of pakfiles with this base name
	unsigned int				m_GenPakId;											// pakfileid of pakfile to add new files to
	BOOL						m_Create;											// is this pakfile used in this app?
	PAK_ENUM					m_AlternativeIndex;									// if m_Create is FALSE, then which index do we use instead?
};

//----------------------------------------------------------------------------
// file hash to uniquely identify file
//----------------------------------------------------------------------------
struct FILE_UNIQUE_HASH
{
	UINT8						m_Hash[SHA1HashSize];

	void Init(
		void)
	{
		structclear(m_Hash);
	}

	BOOL operator==(
		const FILE_UNIQUE_HASH & b) const
	{
		return (memcmp(m_Hash, b.m_Hash, sizeof(m_Hash)) == 0);
	}	

	BOOL operator!=(
		const FILE_UNIQUE_HASH & b) const
	{
		return (memcmp(m_Hash, b.m_Hash, sizeof(m_Hash)) != 0);
	}	

	FILE_UNIQUE_HASH & operator= (
		const FILE_UNIQUE_HASH & hash2)
	{
		ASSERT(MemoryCopy(this->m_Hash, SHA1HashSize, hash2.m_Hash, SHA1HashSize));
		return *this;
	}

	FILE_UNIQUE_HASH & operator= (
		const UINT8 * hash2)
	{
		ASSERT(MemoryCopy(this->m_Hash, SHA1HashSize, hash2, SHA1HashSize));
		return *this;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PAKFILE_LOAD_SPEC
{
	// in
	const TCHAR *				filename;								// file to load
	DWORD						flags;									// load flags
	struct MEMORYPOOL *			pool;									// if buffer is NULL, this is the pool used to allocate the buffer
	DWORD						offset;									// offset into file to start reading or writing
	unsigned int				priority;								// priority
	DWORD						threadId;								// load thread id
	PAK_ENUM					pakEnum;								// pakfile to add the file to if PAKFILE_LOAD_ADD_TO_PAK is specified
	ASYNC_FILE_CALLBACK			fpFileIOThreadCallback;					// callback to be executed in file io thread (asynchronous only)
	ASYNC_FILE_CALLBACK			fpLoadingThreadCallback;				// callback to be executed in loading (main) thread
	ASYNC_FILE_CALLBACK			fpFileIOThreadWriteCompleteCallback;	// callback to be executed in file io thread once the write has been completed (asynchronous only)
	ASYNC_FILE_CALLBACK			fpLoadingThreadWriteCompleteCallback;	// callback to be executed in loading (main) thread once the write has been completed
	void *						callbackData;							// data for callback functions

#if ISVERSION(DEVELOPMENT)
	const char *				dbg_file;					// allocating file & line number
	unsigned int				dbg_line;
	DWORD						dbg_issue_tick;				// issue time (tickcount)
#endif

	// in / out
	TCHAR						fixedfilename[MAX_PATH];	// fixed file name w/ path
	void *						buffer;						// can be NULL on in, in which case the loader will allocate it
	void *						buffer_compressed;
	DWORD						size;						// size of buffer if buffer is passed in (filled in if buffer is allocated by file system)
	DWORD						bytestoread;				// number of bytes to read, (0 = entire file)  (might be changed by file system)

	// out
	UINT64						gentime;					// gentime of file on disk or file in pak
	BOOL						frompak;					// set to TRUE if file was loaded from pakfile, FALSE otherwise
	DWORD						bytesread;					// number of bytes actually read

	// internal
	TCHAR						bufFilePath[MAX_PATH];		// data used to add me to a pakfile
	TCHAR						bufFileName[MAX_PATH];
	PStrT						strFilePath;
	PStrT						strFileName;
	DWORD						key;
	UINT64						fileoffset;					// offset into pakdata to start writing
	struct PAKFILE *			pakfile;
	FILE_UNIQUE_HASH			hash;
	FILE_HEADER					tmpHeader;

	DWORD						size_compressed;

	void Init(void)
	{
		flags = 0;
		pool = NULL;
		offset = 0;
		priority = 0;
		threadId = 0;
		pakEnum = PAK_DEFAULT;
		fpFileIOThreadCallback = NULL;
		fpLoadingThreadCallback = NULL;
		fpFileIOThreadWriteCompleteCallback = NULL;
		fpLoadingThreadWriteCompleteCallback = NULL;
		callbackData = NULL;

#if ISVERSION(DEVELOPMENT)
		dbg_file = NULL;
		dbg_line = 0;
		dbg_issue_tick = 0;
#endif

		fixedfilename[0] = 0;
		buffer = NULL;
		buffer_compressed = NULL;
		size = 0;
		bytestoread = 0;

		gentime = 0;
		frompak = FALSE;
		bytesread = 0;
		bufFilePath[0] = 0;
		bufFileName[0] = 0;
		strFilePath.Set(bufFilePath, 0);
		strFileName.Set(bufFileName, 0);
		fileoffset = 0;
		pakfile = NULL;
		hash.Init();
		size_compressed = 0;
	}

	PAKFILE_LOAD_SPEC(void)
	{
		Init();
	}

#if ISVERSION(DEVELOPMENT)
	PAKFILE_LOAD_SPEC(
		const TCHAR * _filename,
		const char * file,
		unsigned int line)
#else
	PAKFILE_LOAD_SPEC(
		const TCHAR * _filename)
#endif
	{
		Init();
		filename = _filename;
#if ISVERSION(DEVELOPMENT)
		dbg_file = file;
		dbg_line = line;
#endif
	}
};


//----------------------------------------------------------------------------
// index node for a file
//----------------------------------------------------------------------------
struct FILE_INDEX_NODE
{
	FILEID						m_fileId;					// one-up globally unique fileid
	UINT64						m_offset;					// offset into pakfile (on disk)
	UINT64						m_GenTime;					// filetime of original file when it was added to the pakfile
	FILE_UNIQUE_HASH			m_Hash;						// unique hash for the file
	FILE_UNIQUE_HASH			m_SvrHash;					// unique hash for the file on the server
	FILE_INDEX_NODE *			m_NextInHash;				// next node in fileid hash
	PStrT						m_FilePath;					// filepath
	PStrT						m_FileName;					// filename w/o path
	FILE_HEADER					m_FileHeader;				// includes magic number & file version
	unsigned int				m_idPakFile;				// id of pakfile
	PAK_ENUM					m_iPakType;					// pak type
	PAK_ENUM					m_iSvrPakType;
	DWORD						m_fileSize;					// size of file (on disk)
	DWORD						m_fileSizeCompressed;
	DWORD						m_flags;					// file flags

	BOOL						m_bHasSvrHash;
	BOOL						m_bAlreadyWrittenToXml;	

	void Init(
		FILEID id)
	{	
		m_fileId = id;
		m_offset = (UINT64)-1;
		m_GenTime = 0;
		m_NextInHash = NULL;
		m_FilePath.Init();
		m_FileName.Init();
		m_FileHeader.dwMagicNumber = (DWORD)NO_MAGIC;
		m_FileHeader.nVersion = NO_VERSION;
		m_idPakFile = INVALID_ID;
		m_iPakType = PAK_INVALID;
		m_iSvrPakType = PAK_INVALID;
		m_fileSize = 0;
		m_fileSizeCompressed = 0;
		m_bHasSvrHash = FALSE;
		m_bAlreadyWrittenToXml = FALSE;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileInitForApp(
	void);

BOOL PakFileInitForAppNoPakfile(
	void);

void PakFileFreeForApp(
	void);

BOOL PakFileUsedInApp(
	PAK_ENUM ePak);
	
BOOL FileNeedsUpdate(
	const TCHAR * filename,
	const TCHAR * sourcefile,
	DWORD dwFileMagicNumber = 0,
	int nFileVersion = 0,
	DWORD dwFlags = 0,
	UINT64 * gentime = NULL,
	BOOL * bInPak = NULL,
	int nMinFileVersion = 0 );

UINT32 PakFileCreateNewPakfile(
	LPCSTR strPath,
	LPCSTR strBaseName,
	PAK_ENUM pakType,
	LPCSTR strPakDatName = NULL,
	LPCSTR strPakIdxName = NULL);

UINT32 PakFileCreateNewPakfile(
	LPCWSTR strPath,
	LPCWSTR strBaseName,
	PAK_ENUM pakType,
	LPCWSTR strPakDatName = NULL,
	LPCWSTR strPakIdxName = NULL);

BOOL PakFileNeedsUpdate(
	PAKFILE_LOAD_SPEC & spec);

void * PakFileLoadNow(
	PAKFILE_LOAD_SPEC & spec);

BOOL PakFileLoad(
	PAKFILE_LOAD_SPEC & spec);

BOOL PakFileAddFile(
	PAKFILE_LOAD_SPEC & spec,
	BOOL bCleanUp = TRUE);

FILE_INDEX_NODE * PakFileLoadGetIndex(
	PStrT & strFilePath,
	PStrT & strFileName);

FILE_INDEX_NODE * PakFileAddToHash(
	const PStrT & strFilePath,
	const PStrT & strFileName,
	BOOL copy = FALSE);

PAKFILE * PakFileGetPakByID(
	UINT32 pakfileid);

PAK_ENUM PakFileGetTypeByID(
	UINT32 pakfileid);

BOOL PakFileGetGMBuildVersion(
	PAK_ENUM ePakFile,
	struct FILE_VERSION &tVersion);

PAK_ENUM PakFileGetTypeByName(
	LPCTSTR pakBaseName);

UINT32 PakFileGetGenPakfileId(
	PAK_ENUM pakfile);

FILEID PakFileGetFileId(
	const TCHAR * filename,
	PAK_ENUM pakEnum);

BOOL PakFileGetFileName(
	FILEID fileid,
	TCHAR * filename,
	unsigned int len);

void PakFileDecompressFile(
	ASYNC_DATA* pData);

int GameFileAdjustPriorityByDistance(
	int priority,
	float distance);

//BOOL PakFileCheckUpToDateWithServer(
//	TCHAR * filename);

void FixFilename(
	char * dest,
	const char * src,
	int destlen);

void FixFilename(
	WCHAR * dest,
	const WCHAR * src,
	int destlen);

UINT32 PakFileGetID(
	PAKFILE * pakfile);

UINT32 PakFileGetIdxCount(
	void);

FILE_INDEX_NODE * PakFileGetFileByIdx(
	UINT32 idx);

ASYNC_FILE * PakFileGetAsyncFile(
	PAKFILE * pakfile);

PAKFILE_LOAD_SPEC * PakFileLoadCopySpec(
	PAKFILE_LOAD_SPEC & spec);

PAKFILE_LOAD_SPEC* PakFileLoadCopySpec(
	PAKFILE_LOAD_SPEC& spec);

ASYNC_FILE * PakFileGetAsyncFileCopy(
	PAKFILE_LOAD_SPEC& tSpec);

#if ISVERSION(DEVELOPMENT)
void PakFileTrace(
	void);

inline void PakFileRequestTrace(
	void)
{
	// for later, when the pak is done initializing
	extern BOOL gbPakfileTrace;
	gbPakfileTrace = TRUE;
}
#endif

#if ISVERSION(DEVELOPMENT)
void PakFileLog(
	void);
#endif
	
#endif // _PAKFILE_H_
