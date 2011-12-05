//----------------------------------------------------------------------------
// pakfile.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "prime.h"
#include "filepaths.h"
#include "fileversion.h"
#include "pakfile.h"
#include "sha1.h"
#include "markup.h"
#include "fillpak.h"
#include "version.h"
#include "performance.h"
#include "simple_hash.h"


#if ISVERSION(SERVER_VERSION)
#include "pakfile2.cpp.tmh"
#endif

#if !ISVERSION(SERVER_VERSION)
#include "patchclient.h"
#endif
#include "compression.h"

using namespace COMMON_SHA1;


//----------------------------------------------------------------------------
// COMPILE SWITCHES
//----------------------------------------------------------------------------
#define TRACE_STRING_TABLE				1
#define TRACE_FILE_NOT_FOUND			1
#define TRACE_RACE_CONDITION			1
#define TRACE_PAKFILE_LOAD				0
#define TRACE_FILLPAK					1
#define PAKFILE_COMPARE_TO_DATA			FALSE									// compare every load from the pakfile with the file on disk (SLOW!!!)


//----------------------------------------------------------------------------
// EXPORTED GLOBALS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
// extensions and magic numbers
#define PAK_DATA_EXT					_T("dat")								// file extension for pak data file
#define PAK_DATA_MAGIC					'hgda'									// pak data file magic number
#define PAK_DATA_VERSION				4
#define PAK_INDEX_EXT					_T("idx")								// file extension for pak index file
#define PAK_INDEX_MAGIC					'hgin'									// pak index file magic number
#define PAK_INDEX_BETA_VERSION			3										// version number for pak index from beta
#define PAK_INDEX_RELEASE_VERSION		4										// version number for pak index from release
#define PAK_INDEX_NODE_MAGIC			'hgio'									// magic number before and after index nodes
#define PAK_STRING_MAGIC				'hgps'									// magic number for string table (file paths)
#define PAK_STRING_HDR_SIZE				(sizeof(DWORD) * 5)						// size of "header" when streamed (includes other constant data like footer)
#define PAK_STRING_NODE_SIZE			(sizeof(DWORD) + sizeof(PAK_STRLEN))	// size of "header" when streamed (includes other constant data like footer)
#define PAK_STRING_OFFSET_MARKER		0xffff									// used while reading temp filename/paths for invalid len

#define PAK_INDEX_OBS_MULT				69069
#define PAK_INDEX_OBS_ADD				256203221
#define FILEID_HASH_SIZE				32749									// size of global file hash by fileid

// hash & buffer sizes
#define PAKFILE_INDEX_BUFFER			1024									// allocate this number of file indexes per bucket
#define PAKFILE_HASH_SIZE				32749									// size of global file hash by name
#define PAK_STRING_BUFFER				256										// expand string table by this amount
#define PAK_STRING_HASH_SIZE			1024									// size of path hash
#define PAK_TEMP_INDEX_BUFSIZE			256										// buf size for temp index

// file buffer sizes
#define PAKDATA_HEADER_SIZE				sizeof(PAK_DATA_HEADER)					// size of pakdata header
#define PAK_FILE_INDEX_BUFSIZE			(MAX_PATH * sizeof(TCHAR) * 2 + 1024)	// buffer size for file index node
#define ASSUMED_BYTES_PER_SECTOR		512						

// limits
//#define PAKFILE_MAX_INDEX_SIZE		(2 * (UINT64)MEGA)						// max size of index file
#define FILE_MAX_SIZE					(128 * (UINT64)MEGA)					// max size of any individual (non pak) file

// files
#if !ISVERSION(SERVER_VERSION)
#define STARTUP_LOAD_XML				"ntlfl.cfg"
char xml_filename[DEFAULT_FILE_WITH_PATH_SIZE];
#endif

#if ISVERSION(DEVELOPMENT)
// EA's "fingerprint string"
#define FP_PADDING_SIZE 8
static char* FingerPrintString[] = 
{
	"00000000xa37dd45ffe100b",
	"00000001fffcc9753aabac3",
	"0000001025f07cb3fa23114",
	"000000114fe2e33ae4783fe",
	"00000100ead2b8a73ff021f",
	"00000101ac326df0ef9753a",
	"00000110b9cdf6573ddff03",
	"0000011112fab0b0ff39779",
	"00001000eaff312a4f5de65",
	"00001001892ffee33a44569",
	"00001010bebf21f66d22e54",
	"00001011a22347efd375981",
	"00001100188743afd99baac",
	"00001101c342d88a9932123",
	"000011105798725fedcbf43",
	"00001111252669dade32415",
	"00010000fee89da543bf23d",
	"000100014ex",
};
#endif

typedef DWORD	FILEID_ALPHA;													// fileid in alpha index is DWORD


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum
{
	// saved
	PAKFLAG_INDEX_ONLY,															// set if indexnode is added by PakFileGetFileId(), index node has no data, is just a placeholder for a file

	// temp
	PAKFLAG_DEPECRATED_TEMPNODE,												// this tempnode has been deprecated
	PAKFLAG_VALID_NODE,															// this indexnode has been added to the hash (it's not a skipped, empty node)
	PAKFLAG_FREE_FILENAME,														// need to free buffer for filename
	PAKFLAG_IS_CURRENT,															// set if we KNOW the file is current
	PAKFLAG_NOT_CURRENT,														// set if checkdirect() determined this file to not be current already
	PAKFLAG_FILE_NOT_FOUND,														// set if checkdirect() determined file doesn't exist on disk
	PAKFLAG_ADDING,																// set this flag if we're currently writing data for this file

	PAK_INDEX_FLAG_SAVE_MASK =													// save the following flags
		(MAKE_MASK(PAKFLAG_INDEX_ONLY)),
};


// temporary enum used to decide where to load a given file from
enum PAKFILE_LOAD_MODE
{
	PAKFILE_LOAD_FROM_DISK,
	PAKFILE_LOAD_FROM_PAK,
	PAKFILE_LOAD_FROM_SRV_NEW,
	PAKFILE_LOAD_FROM_SRV_UPDATE,
	PAKFILE_FILE_NOT_FOUND,
};


//----------------------------------------------------------------------------
// typedef
//----------------------------------------------------------------------------
typedef WORD							PAK_STRLEN;								// size of strlen of pstr on disk


//----------------------------------------------------------------------------
// macros
//----------------------------------------------------------------------------
#if TRACE_PAKFILE_LOAD && ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
#define PAKLOAD_TRACE(fmt, ...)			LogMessage(NODIRECT_LOG, fmt, __VA_ARGS__)
#else
#define PAKLOAD_TRACE(fmt, ...)
#endif

#if TRACE_FILLPAK && ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
#define FILLPAK_TRACE(fmt, ...)			LogMessage(NODIRECT_LOG, fmt, __VA_ARGS__)
#else
#define FILLPAK_TRACE(fmt, ...)
#endif

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// PAK_STRING_TABLE is a string table containing all file paths
// each pakfile has an array of path/name strings, each node is also 
// inserted into a hash for lookup by string.  
// this allows us to not store duplicate paths
//----------------------------------------------------------------------------
struct PAK_STRING_TABLE
{
	// nested structure
	struct PAK_STRING_NODE
	{
		static const DWORD	PAK_STRING_WRITTEN =		MAKE_MASK(0);			// path has been written to disk
		static const DWORD	PAK_STRING_UNINITIALIZED =	MAKE_MASK(31);			// path is uninitialized

		DWORD							m_Crc32;								// crc of string
		DWORD							m_Flags;					
		PStrT							m_Str;									// str
		unsigned int					m_idNext;								// next in hash
	};

	// data members
	CCritSectLite						m_CS;									// critical section
	struct MEMORYPOOL *					m_Pool;									// memory pool

	PAK_STRING_NODE *					m_Paths;								// array of PATH_STRING_NODE
	unsigned int						m_Count;								// number of path strings in table

	unsigned int						m_Hash[PAK_STRING_HASH_SIZE];			// hash table

	BYTE *								m_StrBuf;								// when we load it from a file, store raw string table here
	DWORD								m_sizeStrBuf;							// size of m_StrBuf

	void Init(
		struct MEMORYPOOL * mempool);

	void Free(
		void);

	unsigned int GetArraySize(
		void) const
	{
		return ((m_Count + PAK_STRING_BUFFER - 1) / PAK_STRING_BUFFER) * PAK_STRING_BUFFER;
	}

	unsigned int GetHashIndex(
		DWORD crc) const
	{
		return (crc % PAK_STRING_HASH_SIZE);
	}

	unsigned int GetIndexFromPtr(
		PAK_STRING_NODE * ptr) const
	{
		ASSERT_RETINVALID(ptr >= m_Paths && ptr < m_Paths + m_Count);
		return (unsigned int)(ptr - m_Paths);
	}

	PAK_STRING_NODE * GetNodeFromIndex(
		unsigned int index) const
	{
		if (index >= m_Count)
		{
			return NULL;
		}
		return m_Paths + index;
	}

	PAK_STRING_NODE * AddArrayNode(
		void);

	PAK_STRING_NODE * AddArrayNode(
		unsigned int offset);

	BOOL Insert(
		const PStrT & str, 
		unsigned int offset);

	unsigned int Get(
		const PStrT & str, 
		BOOL & inserted, 
		BOOL copy);

	PStrT * Get(
		unsigned int index) const;

	BOOL ReadFromBuffer(
		BYTE_BUF_OBFUSCATED & bbuf);

	unsigned int GetStringTableSize(
		void) const;

	unsigned int CalcSize(
		void) const;

	BOOL WriteToBuffer(
		BYTE_BUF_OBFUSCATED & bbuf) const;

	// write empty table to a memory buffer
	static BOOL WriteEmptyToBuffer(
		BYTE_BUF_OBFUSCATED & bbuf)
	{
		ONCE
		{
			ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));
			ASSERT_BREAK(bbuf.PushUInt(0, sizeof(DWORD)));	// m_Count
			ASSERT_BREAK(bbuf.PushUInt(0, sizeof(DWORD)));	// m_sizeStrBuf
			ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));
			ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));
			return TRUE;
		}
		return FALSE;
	}
};


//----------------------------------------------------------------------------
// pakfile
//----------------------------------------------------------------------------
struct PAKFILE
{
	ASYNC_FILE *						m_filePakData;							// file pointer to pakdata
	ASYNC_FILE *						m_filePakIndex;							// file pointer to pakindex
	UINT64								m_sizePakData;							// file size of pakdata
	UINT64								m_sizePakIndex;							// file size of pakindex
	UINT64								m_genTime;								// time pakfile was last modified
	PAK_STRING_TABLE					m_strFilepaths;							// string table of file paths

	CCriticalSection					m_csPakData;							// critical section for modifying pakdata
	CCriticalSection					m_csPakIndex;							// critical section for modifying pakindex

	PAK_ENUM							m_ePakEnum;								// enum of this pakfile
	unsigned int						m_idPakFile;							// id of this pakfile
//	unsigned int						m_indexNumber;							// index number of pakfile (i.e. hellgate000.dat is index 0)
	unsigned int						m_indexVersion;							// file version for pakindex

	// temporary index used only during file loading
	struct PAKFILE_TEMP_INDEX
	{
		FILE_INDEX_NODE *				m_IndexNodes;							// index nodes
		unsigned int					m_NodeCount;							// number of nodes in m_IndexNodes
		unsigned int					m_Size;									// size of m_IndexNodes
		unsigned int					m_ActualCount;							// actual number of unique nodes in temp index after pruning
	};
	PAKFILE_TEMP_INDEX					m_tempIndex;
	FILE_VERSION						m_tVersionBuildGM;						// build version that was used to to create GM version of this pakfile

	// close the pakfile
	void Close(
		void);
};


//----------------------------------------------------------------------------
// array of pakfiles
//----------------------------------------------------------------------------
struct PAKFILE_LIST
{
	CReaderWriterLock_NS_RP				m_CS;									// critical section used only to add pakfiles to list
	struct MEMORYPOOL *					m_Pool;									// memory pool
	PAKFILE **							m_Pakfiles;								// array of pointers to PAKFILEs
	unsigned int						m_PakfileCount;							// # of pakfiles

	// initialize PAKFILE_LIST
	void Init(
		struct MEMORYPOOL * mempool);

	// free (& close) all PAKFILEs in PAKFILE_LIST
	void Free(
		void);

	BOOL IsValidPakfileId(
		unsigned int pakfileid);

	PAKFILE * GetById(
		unsigned int pakfileid);

	PAK_ENUM GetTypeById(
		unsigned int pakfileid);

	BOOL GetGMBuildVersion(
		PAK_ENUM ePakEnum,	
		FILE_VERSION &tVersion);

	// add a new pakfile
	PAKFILE * AddPakfile(
		ASYNC_FILE * pakdata, 
		UINT64 pakdataSize, 
		ASYNC_FILE * pakindex, 
		UINT64 pakindexSize,
		PAK_ENUM ePakEnum,
		FILE_VERSION *pVersionBuild);
				
};


//----------------------------------------------------------------------------
// a bucket of file indexes
//----------------------------------------------------------------------------
struct FILE_INDEX_BUCKET
{
	FILE_INDEX_NODE						m_IndexArray[PAKFILE_INDEX_BUFFER];
};


//----------------------------------------------------------------------------
// index of all files in entire PAKFILE_SYSTEM
//----------------------------------------------------------------------------
struct PAKFILE_INDEX
{
	CReaderWriterLock_NS_RP				m_CS;									// critical section for index
	struct MEMORYPOOL *					m_Pool;									// memory pool (used for file system internal structures only, not loaded files)
	
	FILE_INDEX_BUCKET **				m_IndexBuckets;							// we allocate file indexes in buckets
	unsigned int						m_BucketCount;							// number of buckets allocated

	FILE_INDEX_NODE *					m_FileIdHash[FILEID_HASH_SIZE];			// hash table by fileid
	FILE_INDEX_NODE *					m_Garbage;								// unused file index nodes

	void Init(
		struct MEMORYPOOL * pool);

	void Free(
		void);

	FILE_INDEX_NODE * GetByIndex(
		unsigned int index);

	// get FILE_INDEX_NODE based on fileid
	FILE_INDEX_NODE * GetById(
		FILEID fileid);

	FILE_INDEX_NODE * GetNewIndexNode(
		void);

	// add a new fileid to the fileid hash
	void AddFileIdToHash(
		FILE_INDEX_NODE * node);

	// add a new fileid to the index (using one-up fileid)
	FILE_INDEX_NODE * AddFileIndex(
		const PStrT & strFilePath, 
		const PStrT & strFileName, 
		BOOL copy);

	// add a fixed fileid to the index
	FILE_INDEX_NODE * AddFileIndexFixId(
		FILEID fileid, 
		const PStrT & strFilePath, 
		const PStrT & strFileName, 
		BOOL copy);

	// get filename from fileid
	BOOL GetFileNameById(
		FILEID fileid,
		TCHAR * filename,
		unsigned int len);
};


//----------------------------------------------------------------------------
// a node in the PAKFILE_HASH
//----------------------------------------------------------------------------
struct PAKFILE_HASH_NODE
{
	PAKFILE_HASH_NODE *					m_next;									// next node in hash bin
	PStrT 								m_FilePath;								// filepath (must have trailing slash)
	PStrT 								m_FileName;								// filename w/o path
	FILEID								m_fileId;								// globally unique file id
};


//----------------------------------------------------------------------------
// this PAKFILE_SYSTEM hash lets us look up files by name
//----------------------------------------------------------------------------
struct PAKFILE_HASH
{
	CReaderWriterLock_NS_RP				m_CS;									// critical section for hash as a whole
	struct MEMORYPOOL *					m_Pool;									// memory pool (used for file system internal structures only, not loaded files)

	PAKFILE_HASH_NODE *					m_Hash[PAKFILE_HASH_SIZE];				// hash by name of all files

	void Init(
		struct MEMORYPOOL * pool);

	void Free(
		void);

	DWORD GetKey(
		const PStrT & filepath, 
		const PStrT & filename) const;

	FILE_INDEX_NODE * FindInHash(
		const PStrT & filepath, 
		const PStrT & filename, 
		DWORD key);

	FILE_INDEX_NODE * FindInHash(
		const PStrT & filepath, 
		const PStrT & filename, 
		DWORD * key);

	FILE_INDEX_NODE * FindInHash(
		const PStrT & filepath, 
		const PStrT & filename);

	void AddToHash(
		FILE_INDEX_NODE * indexnode,
		unsigned int key);
};


//----------------------------------------------------------------------------
// file header of both pakdata & pakindex
//----------------------------------------------------------------------------
struct PAK_FILE_HEADER
{
	DWORD								dwMagic;
	DWORD								dwVersion;
	FILE_VERSION						tVersionBuild;
};


//----------------------------------------------------------------------------
// header for pakindex file.  composed of two sections: 
// one is baked index composed of an array of FILE_INDEX_NODE followed by 
//		streamed PAK_STRING_TABLE.
// second is a list of individual FILE_INDEX_NODE followed by 
//		filepath (if necessary) and filename for that node.
// when the pakindex is first opened, it moves the second section into the
// first.
//----------------------------------------------------------------------------
struct PAK_INDEX_HEADER
{
	struct {
		DWORD							dwMagic;
		DWORD							dwVersion;
	} m_Hdr;																	// magic # + version
	unsigned int						m_Count;								// number of entries in section 1
};


//----------------------------------------------------------------------------
// header for pakdata file.
//----------------------------------------------------------------------------
struct PAK_DATA_HEADER
{
	PAK_FILE_HEADER						m_Hdr;									// magic # + version
};


//----------------------------------------------------------------------------
// file global pakfile system
//----------------------------------------------------------------------------
struct PAKFILE_SYSTEM
{
	BOOL								m_bInitialized;							// system has been initialized
	struct MEMORYPOOL *					m_Pool;									// memory pool
	WCHAR								m_strRootDirectory[MAX_PATH];			// CHB 2007.01.11 - This may want to be OS_PATH_CHAR some day.
	unsigned int						m_lenRootDirectory;

	PAKFILE_LIST						m_Pakfiles;								// list of pakfiles
	PAKFILE_INDEX						m_FileIndex;							// global file index
	PAKFILE_HASH						m_FileHash;								// hash by name of all files

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	CMarkup								m_xmlStartupLoad;						// load log
	CCriticalSection					m_csStartupLoad;
#endif
};


//----------------------------------------------------------------------------
// simple file-hash info
//----------------------------------------------------------------------------
typedef struct FILE_HASH_INFO
{
	TCHAR filename[DEFAULT_FILE_WITH_PATH_SIZE];
	FILE_UNIQUE_HASH hash;
} *PFILE_HASH_INFO;


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static CCriticalSection					g_csPakfile(TRUE);						// global crit sect for creation of PAKFILE_SYSTEM
static INT32							g_iRefCount = 0;
PAKFILE_SYSTEM							g_Pakfile;
static CSimpleHash<LPTSTR, PFILE_HASH_INFO> g_FileHashTable;

#if ISVERSION(DEVELOPMENT)
BOOL									gbPakfileTrace = FALSE;
#endif

PAKFILE_DESC g_PakFileDescHellgate[] =
{
	{ PAK_DEFAULT,					_T("hellgate"),					_T("x_hellgate"),				_T("sp_hellgate"),					_T("mp_hellgate"),					0,	0,	TRUE,	PAK_DEFAULT },
	{ PAK_GRAPHICS_HIGH,			_T("hellgate_graphicshigh"),	_T("x_hellgate_graphicshigh"),	_T("sp_hellgate_graphicshigh"),		_T("mp_hellgate_graphicshigh"),		0,	0,	TRUE,	PAK_GRAPHICS_HIGH },
	{ PAK_SOUND,					_T("hellgate_sound"),			_T("x_hellgate_sound"),			_T("sp_hellgate_sound"),			_T("mp_hellgate_sound"),			0,	0,	TRUE,	PAK_SOUND },
	{ PAK_SOUND_HIGH,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_SOUND },
	{ PAK_SOUND_LOW,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_SOUND },
	{ PAK_SOUND_MUSIC,				_T("hellgate_soundmusic"),		_T("x_hellgate_soundmusic"),	_T("sp_hellgate_soundmusic"),		_T("mp_hellgate_soundmusic"),		0,	0,	TRUE,	PAK_SOUND_MUSIC },
	{ PAK_LOCALIZED,				_T("hellgate_localized"),		_T("x_hellgate_localized"),		_T("sp_hellgate_localized"),		_T("mp_hellgate_localized"),		0,	0,	TRUE,	PAK_LOCALIZED },
	{ PAK_MOVIES,					_T("hellgate_movies"),			_T("x_hellgate_movies"),		_T("sp_hellgate_movies"),			_T("mp_hellgate_movies"),			0,	0,	TRUE,	PAK_MOVIES },
	{ PAK_GRAPHICS_HIGH_BACKGROUNDS,_T("hellgate_bghigh"),			_T("x_hellgate_bghigh"),		_T("sp_hellgate_bghigh"),			_T("mp_hellgate_bghigh"),			0,	0,	TRUE,	PAK_GRAPHICS_HIGH_BACKGROUNDS },
	{ PAK_GRAPHICS_HIGH_PLAYERS,	_T("hellgate_playershigh"),		_T("x_hellgate_playershigh"),	_T("sp_hellgate_playershigh"),		_T("mp_hellgate_playershigh"),		0,	0,	TRUE,	PAK_GRAPHICS_HIGH_PLAYERS },
	{ PAK_MOVIES_HIGH,				_T("hellgate_movieshigh"),		_T("x_hellgate_movieshigh"),	_T("sp_hellgate_movieshigh"),		_T("mp_hellgate_movieshigh"),		0,	0,	TRUE,	PAK_MOVIES_HIGH },
	{ PAK_MOVIES_LOW,				_T("hellgate_movieslow"),		_T("x_hellgate_movieslow"),		_T("sp_hellgate_movieslow"),		_T("mp_hellgate_movieslow"),		0,	0,	TRUE,	PAK_MOVIES_LOW },
};

PAKFILE_DESC g_PakFileDescTugboat[] =
{
	{ PAK_DEFAULT,					_T("mythos"),					_T("x_mythos"),					_T("sp_mythos"),					_T("mp_mythos"),					0,	0,	TRUE,	PAK_DEFAULT },
	{ PAK_GRAPHICS_HIGH,			_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
	{ PAK_SOUND,					_T("mythos_sound"),				_T("x_mythos_sound"),			_T("sp_mythos_sound"),				_T("mp_mythos_sound"),				0,	0,	TRUE,	PAK_SOUND },
	{ PAK_SOUND_HIGH,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_SOUND },
	{ PAK_SOUND_LOW,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_SOUND },
	{ PAK_SOUND_MUSIC,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_SOUND },
	{ PAK_LOCALIZED,				_T("mythos_localized"),			_T("x_mythos_localized"),		_T("sp_mythos_localized"),			_T("mp_mythos_localized"),			0,	0,	TRUE,	PAK_LOCALIZED },
	{ PAK_MOVIES,					_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
	{ PAK_GRAPHICS_HIGH_BACKGROUNDS,_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
	{ PAK_GRAPHICS_HIGH_PLAYERS,	_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
	{ PAK_MOVIES_HIGH,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
	{ PAK_MOVIES_LOW,				_T(""),							_T(""),							_T(""),								_T(""),								0,	0,	FALSE,	PAK_DEFAULT },
};

SIMPLE_DYNAMIC_ARRAY<PAKFILE_DESC> g_PakFileDesc;
//PAKFILE_DESC * g_PakFileDesc = NULL;


//----------------------------------------------------------------------------
// FORWARD DECLARATION for STATIC FUNCTIONS
//----------------------------------------------------------------------------
static BOOL sPakFileAddFileToLoadXML(
	const TCHAR * filename,
	UINT32 pakfileid,
	UINT8* hash,
	BOOL bFileInPak,
	BOOL bSave = TRUE);


//----------------------------------------------------------------------------
// Generic utility functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// convert filename to universal representation
//----------------------------------------------------------------------------
template <typename T>
static void sFixFilename(
	T * dest,
	const T * src,
	int destlen)
{
	if (src[1] == ':') 
	{
		PStrCopy(dest, src + g_Pakfile.m_lenRootDirectory, destlen);
	} 
	else 
	{
		PStrCopy(dest, src, destlen);
	}
	PStrFixPathBackslash(dest);
	PStrLower(dest, destlen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FixFilename(
	char * dest,
	const char * src,
	int destlen)
{
	sFixFilename<char>(dest, src, destlen);
}

// THIS IS A VERY DANGEROUS FUNCTION! if what we're copying from has any unicode, this will fail! - bmanegold
void static sFixFilename(
	 char * dest,
	 const WCHAR * src,
	 int destlen)
{
	if (wcsncmp(src+1, L":", 1) == 0) 
	{
		PStrCvt(dest, src + g_Pakfile.m_lenRootDirectory, destlen);
	} 
	else 
	{
		PStrCvt(dest, src, destlen);
	}
	PStrFixPathBackslash(dest);
	PStrLower(dest, destlen);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FixFilename(
	WCHAR * dest,
	const WCHAR * src,
	int destlen)
{
	sFixFilename<WCHAR>(dest, src, destlen);
}


//----------------------------------------------------------------------------
// construct full filename from path & file
//----------------------------------------------------------------------------
static BOOL sCombineFilename(
	TCHAR * str,
	size_t len,
	const PStrT & strFilePath,
	const PStrT & strFileName)
{
	ASSERT_RETFALSE(strFilePath.Len() + strFileName.Len() < len);
	strFilePath.CopyTo(str);
	strFileName.CopyTo(str + strFilePath.Len());
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSeparateFilename(
	const TCHAR * src,
	PStrT & strFilePath,
	PStrT & strFileName)
{
	ASSERT_RETFALSE(src);
	ASSERT_RETFALSE(strFilePath.str && strFileName.str);

	strFilePath.len = PStrGetPath(strFilePath.str, MAX_PATH, src) + 1;
	PStrCopy(strFileName.str, src + strFilePath.len, MAX_PATH);
	strFileName.len = PStrLen(strFileName.str, MAX_PATH);
	return TRUE;
}


//----------------------------------------------------------------------------
// fix & separate filename into buffers, all OUT buffers are assumed len
// MAX_PATH
//----------------------------------------------------------------------------
static BOOL sSeparateFilename(
	const WCHAR * src,
	TCHAR * fixed,
	PStrT & strFilePath,
	PStrT & strFileName)
{
	sFixFilename(fixed, src, MAX_PATH);
	ASSERT_RETFALSE(sSeparateFilename(fixed, strFilePath, strFileName));
	return TRUE;
}

static BOOL sSeparateFilename(
	const char * src,
	TCHAR * fixed,
	PStrT & strFilePath,
	PStrT & strFileName)
{	
	sFixFilename(fixed, src, MAX_PATH);
	ASSERT_RETFALSE(sSeparateFilename(fixed, strFilePath, strFileName));
	return TRUE;
}



//----------------------------------------------------------------------------
// PAK_STRING_TABLE member functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// initialize the string table
//----------------------------------------------------------------------------
void PAK_STRING_TABLE::Init(
	struct MEMORYPOOL * mempool)
{
	m_CS.Init();
	m_Pool = mempool;
	m_Paths = NULL;
	m_Count = 0;
	m_StrBuf = NULL;
	m_sizeStrBuf = 0;
	for (unsigned int ii = 0; ii < PAK_STRING_HASH_SIZE; ii++)
	{
		m_Hash[ii] = INVALID_ID;
	}
}


//----------------------------------------------------------------------------
// free the string table
//----------------------------------------------------------------------------
void PAK_STRING_TABLE::Free(
	void)
{
	// free strings if they weren't allocated in block
	PAK_STRING_NODE * cur = m_Paths;
	PAK_STRING_NODE * end = m_Paths + m_Count;
	BYTE * bufStart = m_StrBuf;
	BYTE * bufEnd = m_StrBuf + m_sizeStrBuf;
	while (cur < end)
	{
		if ((BYTE *)cur->m_Str.str < bufStart || (BYTE *)cur->m_Str.str >= bufEnd)
		{
			FREE(m_Pool, cur->m_Str.str);
		}
		++cur;
	}
	if (m_Paths)
	{
		FREE(m_Pool, m_Paths);
		m_Paths = NULL;
	}
	if (m_StrBuf)
	{
		FREE(m_Pool, m_StrBuf);
		m_StrBuf = NULL;
		m_sizeStrBuf = 0;
	}
	structclear(m_Hash);
	m_Count = 0;
	m_Pool = NULL;
	m_CS.Free();
}


//----------------------------------------------------------------------------
// add a new node to the array, realloc if needed, increment count
// not thread safe, done in two places, on pakindex load (not locked)
// and from ::Get() which is locked
//----------------------------------------------------------------------------
PAK_STRING_TABLE::PAK_STRING_NODE * PAK_STRING_TABLE::AddArrayNode(
	void)
{
	unsigned int size = GetArraySize();
	// expand the array
	if (m_Count >= size)						
	{
		size += PAK_STRING_BUFFER;
		m_Paths = (PAK_STRING_NODE *)REALLOCZ(m_Pool, m_Paths, size * sizeof(PAK_STRING_NODE));
		ASSERT(m_Paths);
	}
	ASSERT(m_Paths);
	unsigned int index = m_Count++;
	PAK_STRING_NODE * node = m_Paths + index;

	node->m_idNext = INVALID_ID;
	return node;
}


//----------------------------------------------------------------------------
// add a new node to the array, realloc if needed, increment count
// not thread safe, done in two places, on pakindex load (not locked)
// and from ::Get() which is locked
//----------------------------------------------------------------------------
PAK_STRING_TABLE::PAK_STRING_NODE * PAK_STRING_TABLE::AddArrayNode(
	unsigned int offset)
{
	unsigned int size = GetArraySize();
	// expand the array
	if (offset >= size)						
	{
		size = (offset / PAK_STRING_BUFFER + 1) * PAK_STRING_BUFFER;
		m_Paths = (PAK_STRING_NODE *)REALLOCZ(m_Pool, m_Paths, size * sizeof(PAK_STRING_NODE));
		ASSERT(m_Paths);
	}
	ASSERT(m_Paths);
	if (m_Count <= offset)
	{
		PAK_STRING_NODE * node = m_Paths + m_Count;
		PAK_STRING_NODE * end = m_Paths + offset;
		while (node < end)
		{
			node->m_Flags = PAK_STRING_NODE::PAK_STRING_UNINITIALIZED;
			node->m_idNext = INVALID_ID;
			++node;
		}
		m_Count = offset + 1;
	}
	unsigned int index = offset;
	PAK_STRING_NODE * node = m_Paths + index;
	ASSERT_RETFALSE(node->m_Str.IsEmpty());

	node->m_idNext = INVALID_ID;
	return node;
}


//----------------------------------------------------------------------------
// insert a string in a given offset
// return TRUE if success
//----------------------------------------------------------------------------
BOOL PAK_STRING_TABLE::Insert(							
	const PStrT & str,
	unsigned int offset)
{
	BOOL inserted = FALSE;
	ASSERT_RETFALSE(!str.IsEmpty());
	DWORD crc = CRC(0, (BYTE *)str.str, str.len * sizeof(TCHAR));
	unsigned int hashidx = GetHashIndex(crc);

	CSLAutoLock lock(&m_CS);

	PAK_STRING_NODE * next = GetNodeFromIndex(m_Hash[hashidx]);
	PAK_STRING_NODE * curr = NULL;
	while ((curr = next) != NULL)
	{
		if (curr->m_Crc32 == crc && str.len == curr->m_Str.len)
		{
			return TRUE;
		}
		next = GetNodeFromIndex(curr->m_idNext);
	}

	curr = AddArrayNode(offset);
	ASSERT_RETFALSE(curr);
	CLEAR_MASK(curr->m_Flags, PAK_STRING_NODE::PAK_STRING_UNINITIALIZED);
	curr->m_Crc32 = crc;
	curr->m_Str.Copy(str, m_Pool);	
	curr->m_idNext = m_Hash[hashidx];
	m_Hash[hashidx] = GetIndexFromPtr(curr);
	inserted = TRUE;
	return TRUE;
}


//----------------------------------------------------------------------------
// get the index of the string from the string, add if not found, 
// set inserted to TRUE if inserted
//----------------------------------------------------------------------------
unsigned int PAK_STRING_TABLE::Get(							
	const PStrT & str,
	BOOL & inserted,
	BOOL copy)
{
	inserted = FALSE;
	ASSERT_RETINVALID(!str.IsEmpty());
	DWORD crc = CRC(0, (BYTE *)str.str, str.len * sizeof(TCHAR));
	unsigned int hashidx = GetHashIndex(crc);
	unsigned int index = 0;

	m_CS.Enter();
	{
		PAK_STRING_NODE * next = GetNodeFromIndex(m_Hash[hashidx]);
		PAK_STRING_NODE * curr = NULL;
		while ((curr = next) != NULL)
		{
			if (curr->m_Crc32 == crc && str.len == curr->m_Str.len)
			{
				break;
			}
			next = GetNodeFromIndex(curr->m_idNext);
		}
		// didn't find the string in the hash, add it
		if (!curr)								
		{
			curr = AddArrayNode();
			curr->m_Crc32 = crc;
			if (copy)
			{
				curr->m_Str.Copy(str, m_Pool);	
			}
			else
			{
				curr->m_Str.Set(str.str, str.Len());
			}
			curr->m_idNext = m_Hash[hashidx];
			m_Hash[hashidx] = GetIndexFromPtr(curr);
			inserted = TRUE;
		}
		index = GetIndexFromPtr(curr);
	}
	m_CS.Leave();

	return index;
}


//----------------------------------------------------------------------------
// return the filepath string from the index
//----------------------------------------------------------------------------
PStrT * PAK_STRING_TABLE::Get(								
	unsigned int index) const
{
	ASSERT_RETNULL(index < m_Count);
	ASSERT_RETNULL(m_Paths);
	PAK_STRING_NODE * node = m_Paths + index;
	ASSERT_RETNULL(!node->m_Str.IsEmpty());
	return &node->m_Str;
}


//----------------------------------------------------------------------------
// write the entire table to a memory buffer
//----------------------------------------------------------------------------
BOOL PAK_STRING_TABLE::WriteToBuffer(						
	BYTE_BUF_OBFUSCATED & bbuf) const
{
	ONCE
	{
		// write header info
		ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));
		ASSERT_BREAK(bbuf.PushUInt(m_Count, sizeof(m_Count)));
		unsigned int cursorSizeStrBuf = bbuf.GetCursor();
		ASSERT_BREAK(bbuf.PushUInt(m_sizeStrBuf, sizeof(m_sizeStrBuf)));

		// write string table
		unsigned int bufsize = 0;
		PAK_STRING_NODE * cur = m_Paths;
		PAK_STRING_NODE * end = m_Paths + m_Count;
		for (; cur < end; ++cur)
		{
			ASSERT_BREAK(bbuf.PushBuf(cur->m_Str.GetStr(), (cur->m_Str.Len() + 1) * sizeof(TCHAR)));
			bufsize += cur->m_Str.Len() + 1;
		}

		// go back and write strbuf size
		bufsize *= sizeof(TCHAR);
		int cursorEnd = bbuf.GetCursor();
		bbuf.SetCursor(cursorSizeStrBuf);
		ASSERT_BREAK(bbuf.PushUInt(bufsize, sizeof(m_sizeStrBuf)));

		bbuf.SetCursor(cursorEnd);
		ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));

		// write string nodes (crc + size)
		cur = m_Paths;
		for (; cur < end; ++cur)
		{
			PAK_STRLEN size = (PAK_STRLEN)cur->m_Str.Len();
#ifdef _BIG_ENDIAN
#pragma			stop("look");
#endif
			ASSERT_BREAK(bbuf.PushUInt(size, sizeof(size)));
			ASSERT_BREAK(bbuf.PushUInt(cur->m_Crc32, sizeof(cur->m_Crc32)));
		}

		ASSERT_BREAK(bbuf.PushUInt(PAK_STRING_MAGIC, sizeof(DWORD)));

		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
// read the table from a buffer written by WriteToBuffer
//----------------------------------------------------------------------------
BOOL PAK_STRING_TABLE::ReadFromBuffer(						
	BYTE_BUF_OBFUSCATED & bbuf)
{
	ASSERT_RETFALSE(m_Count == 0);
	ASSERT_RETFALSE(m_StrBuf == NULL && m_sizeStrBuf == 0);

	// read header info
	unsigned int dw;
	ASSERT_RETFALSE(bbuf.PopUInt(&dw, sizeof(dw)));
	ASSERT_RETFALSE(dw == PAK_STRING_MAGIC);

	unsigned int count;
	ASSERT_RETFALSE(bbuf.PopUInt(&count, sizeof(count)));	
	ASSERT_GOTO(bbuf.PopUInt(&m_sizeStrBuf, sizeof(m_sizeStrBuf)), _error);

#if TRACE_STRING_TABLE
	trace("PAK_STRING_TABLE::ReadFromBuffer() -- Count:%d  Size:%d\n", count, m_sizeStrBuf);
#endif

	// read str table
	if (m_sizeStrBuf > 0)
	{
		m_StrBuf = (BYTE *)MALLOC(m_Pool, m_sizeStrBuf);
		ASSERT_GOTO(m_StrBuf, _error);
		ASSERT_GOTO(bbuf.PopBuf(m_StrBuf, m_sizeStrBuf), _error);
	}

	ASSERT_GOTO(bbuf.PopUInt(&dw, sizeof(DWORD)), _error);
	ASSERT_GOTO(dw == PAK_STRING_MAGIC, _error);
	
	TCHAR * curstr = (TCHAR *)m_StrBuf;						// advancing str ptr to current string
	TCHAR * endstr = curstr + m_sizeStrBuf / sizeof(TCHAR);	// end of string table
	// read string nodes (crc + size)
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		PAK_STRLEN len;
		ASSERT_GOTO(bbuf.PopUInt(&len, sizeof(len)), _error);
		unsigned int crc;
		ASSERT_GOTO(bbuf.PopUInt(&crc, sizeof(DWORD)), _error);

		ASSERT_GOTO(curstr + len + 1 <= endstr, _error);		// validate we're within the strbuf
		ASSERT_GOTO(*(curstr + len) == 0, _error);				// validate term
		DBG_ASSERT_GOTO(CRC(0, (BYTE *)curstr, len * sizeof(TCHAR)) == crc, _error);
		// add the node to the hash & the array
		unsigned int hashidx = GetHashIndex(crc);
		PAK_STRING_NODE * node = AddArrayNode();
		node->m_Crc32 = crc;
		node->m_Flags = 0;
		node->m_Str.len = len;
		node->m_Str.str = curstr;
		node->m_idNext = m_Hash[hashidx];
		m_Hash[hashidx] = GetIndexFromPtr(node);

		curstr += (len + 1);								// advance to the next string
	}

	ASSERT_GOTO(bbuf.PopUInt(&dw, sizeof(DWORD)), _error);
	ASSERT_GOTO(dw == PAK_STRING_MAGIC, _error);

#if TRACE_STRING_TABLE
	trace("PAK_STRING_TABLE::ReadFromBuffer() -- Successfully read %d strings.\n", m_Count);
#endif

	return TRUE;

_error:
	m_sizeStrBuf = 0;
	if (m_StrBuf)
	{
		FREE(g_Pakfile.m_Pool, m_StrBuf);
		m_StrBuf = NULL;
	}
	m_Count = 0;
	return FALSE;
}


//----------------------------------------------------------------------------
// set up a PStrT from a offset into a pakfile's filename_buf
//----------------------------------------------------------------------------
static BOOL sPakFileGetStrFromOffset(
	PStrT & str,
	unsigned int offset,
	const PAKFILE * pakfile)
{
	PStrT * lookup = pakfile->m_strFilepaths.Get(offset);
	ASSERT_RETFALSE(lookup);

	str.Set(lookup->str, lookup->Len());
	return TRUE;
}


//----------------------------------------------------------------------------
// read a PStrT from a buffer (used for consolidated index nodes)
//----------------------------------------------------------------------------
static BOOL sPakFileReadPStrFromBufferOffset(
	BYTE_BUF_OBFUSCATED & bbuf,
	PStrT & str,
	const PAKFILE * pakfile)
{
	unsigned int offset;
	ASSERT_RETFALSE(bbuf.PopUInt(&offset, sizeof(offset)));
	return sPakFileGetStrFromOffset(str, offset, pakfile);
}


//----------------------------------------------------------------------------
// read a PStrT from a memory buffer (used for unconsolidated index nodes)
// because unconsolidated indexes might get written out-of-order, we
// store temporary data for offsets & resolve only after all strings have been
// added
//----------------------------------------------------------------------------
static BOOL sPakFileReadPakStrFromBuffer(
	BYTE_BUF_OBFUSCATED & bbuf,
	PStrT & str,
	PAKFILE * pakfile)
{
	BYTE by;										
	ASSERT_RETFALSE(bbuf.PopUInt(&by, sizeof(by)));
	if (by == 0)											// read offset
	{
		unsigned int offset;
		ASSERT_RETFALSE(bbuf.PopUInt(&offset, sizeof(offset)));
		str.str = (TCHAR *)((BYTE *)0 + offset);
		str.len = PAK_STRING_OFFSET_MARKER;
	}
	else													// read entire string
	{
		unsigned int offset;
		ASSERT_RETFALSE(bbuf.PopUInt(&offset, sizeof(offset)));
		PAK_STRLEN len;
		ASSERT_RETFALSE(bbuf.PopUInt(&len, sizeof(len)));
		ASSERT_RETFALSE(len < MAX_PATH);
		ASSERT_RETFALSE(bbuf.GetRemainder() >= (len + 1) * sizeof(TCHAR));
		str.str = (TCHAR *)MALLOC(g_Pakfile.m_Pool, (len + 1) * sizeof(TCHAR));
		ASSERT_GOTO(bbuf.PopBuf(str.str, (len + 1) * sizeof(TCHAR)), _error);
		ASSERT_GOTO(str.str[len] == 0, _error);
		ASSERT_GOTO(PStrLen(str.str, len + 1) == len, _error);
		str.len = len;

		ASSERT_GOTO(pakfile->m_strFilepaths.Insert(str, offset), _error);
	}
	return TRUE;

_error:
	if (str.str)
	{
		FREE(g_Pakfile.m_Pool, str.str);
		str.str = NULL;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// write a string to a buffer, offset only
//----------------------------------------------------------------------------
BOOL sPakFileWritePakStrOffsetToBuffer(
	BYTE_BUF_OBFUSCATED & bbuf,
	const PStrT & str,
	PAKFILE * pakfile)
{
	ASSERT_RETFALSE(pakfile);

	BOOL inserted;
	unsigned int offset = pakfile->m_strFilepaths.Get(str, inserted, TRUE);
	ASSERT_RETFALSE(!inserted);
	ASSERT_RETFALSE(bbuf.PushUInt(offset, sizeof(offset)));
	return TRUE;
}


//----------------------------------------------------------------------------
// write a string to a buffer
// there are two possibilities, if the string is in m_strFilepaths, then write
// the offset into m_strFilepaths, otherwise, write the string
//----------------------------------------------------------------------------
BOOL sPakFileWritePakStrToBuffer(
	BYTE_BUF_OBFUSCATED & bbuf,
	const PStrT & str,
	PAKFILE * pakfile)
{
	ASSERT_RETFALSE(pakfile);

	BOOL inserted;
	unsigned int offset = pakfile->m_strFilepaths.Get(str, inserted, TRUE);

	if (!inserted)	
	{
		BYTE by = 0;										// string is in m_strFilepaths, just write offset
		ASSERT_RETFALSE(bbuf.PushUInt(by, sizeof(by)));
		ASSERT_RETFALSE(bbuf.PushUInt(offset, sizeof(offset)));
	}
	else													// write out full string
	{
		BYTE by = 1;
		ASSERT_RETFALSE(bbuf.PushUInt(by, sizeof(by)));
		ASSERT_RETFALSE(bbuf.PushUInt(offset, sizeof(offset)));
		PAK_STRLEN len = (PAK_STRLEN)str.Len();
		ASSERT_RETFALSE(bbuf.PushUInt(len, sizeof(len)));
		ASSERT_RETFALSE(bbuf.PushBuf(str.str, (len + 1) * sizeof(TCHAR)));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// return sum of sizes of all strings
// not thread safe, so we only do it at a specific point (on pakindex load)
//----------------------------------------------------------------------------
unsigned int PAK_STRING_TABLE::GetStringTableSize(			
	void) const
{
	unsigned int size = 0;
	PAK_STRING_NODE * cur = m_Paths;
	PAK_STRING_NODE * end = m_Paths + m_Count;
	for (; cur < end; ++cur)
	{
		ASSERT(!cur->m_Str.IsEmpty());
		ASSERT(cur->m_Str.Len() < USHRT_MAX);
		size += (cur->m_Str.Len() + 1) * sizeof(TCHAR);	// str w/ terminator
	}
	return size;
}


//----------------------------------------------------------------------------
// calc size of streamed table
//----------------------------------------------------------------------------
unsigned int PAK_STRING_TABLE::CalcSize(
	void) const
{
	unsigned int size = PAK_STRING_HDR_SIZE;
	size += GetStringTableSize();
	size += PAK_STRING_NODE_SIZE * m_Count;
	return size;
}


//----------------------------------------------------------------------------
// PAKFILE_INDEX functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PAKFILE_INDEX::Init(
	struct MEMORYPOOL * pool)
{
	m_CS.Init();
	m_Pool = pool;
	m_IndexBuckets = NULL;
	m_BucketCount = 0;
	memclear(m_FileIdHash, sizeof(m_FileIdHash));
	m_Garbage = NULL;
}


//----------------------------------------------------------------------------
// free everything
//----------------------------------------------------------------------------
void PAKFILE_INDEX::Free(
	void)
{
	CRW_RP_AutoWriteLock autolock(&m_CS);

	// free file index nodes
	unsigned int indexcount = m_BucketCount * PAKFILE_INDEX_BUFFER;
	for (unsigned int ii = 0; ii < indexcount; ++ii)
	{
		FILE_INDEX_NODE * indexnode = GetByIndex(ii);
		ASSERT_CONTINUE(indexnode);
		if (TESTBIT(indexnode->m_flags, PAKFLAG_FREE_FILENAME))
		{
			indexnode->m_FilePath.Free(m_Pool);
			indexnode->m_FileName.Free(m_Pool);
		}
	}
	
	// free buckets
	for (unsigned int ii = 0; ii < m_BucketCount; ++ii)
	{ 
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, m_IndexBuckets[ii]);
#else	
		FREE(m_Pool, m_IndexBuckets[ii]);
#endif
		m_IndexBuckets[ii] = NULL;
	}	
	if (m_IndexBuckets)
	{
		FREE(m_Pool, m_IndexBuckets);
		m_IndexBuckets = NULL;
	}

	memclear(m_FileIdHash, sizeof(m_FileIdHash));
	m_Garbage = NULL;
	m_BucketCount = 0;
	m_Pool = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_INDEX::GetNewIndexNode(
	void)
{
	if (m_Garbage == NULL)
	{
		// allocate a new bucket
		m_IndexBuckets = (FILE_INDEX_BUCKET **)REALLOCZ(m_Pool, m_IndexBuckets, sizeof(FILE_INDEX_BUCKET *) * (m_BucketCount + 1));
		
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FILE_INDEX_BUCKET * bucket = m_IndexBuckets[m_BucketCount] = (FILE_INDEX_BUCKET *)MALLOCZ(g_StaticAllocator, sizeof(FILE_INDEX_BUCKET));
#else		
		FILE_INDEX_BUCKET * bucket = m_IndexBuckets[m_BucketCount] = (FILE_INDEX_BUCKET *)MALLOCZ(m_Pool, sizeof(FILE_INDEX_BUCKET));
#endif
		++m_BucketCount;

		// put all of the nodes in the new bucket onto the garbage list
		for (int ii = PAKFILE_INDEX_BUFFER - 1; ii >= 0; --ii)
		{
			FILE_INDEX_NODE * node = bucket->m_IndexArray + ii;
			node->m_NextInHash = m_Garbage;
			m_Garbage = node;
		}
	}

	ASSERT_RETNULL(m_Garbage);
	FILE_INDEX_NODE * node = m_Garbage;
	m_Garbage = node->m_NextInHash;
	return node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_INDEX::GetByIndex(
	unsigned int index)
{
	unsigned int bucketIndex = index / PAKFILE_INDEX_BUFFER;
	ASSERT_RETNULL(bucketIndex < m_BucketCount);
	FILE_INDEX_BUCKET * bucket = m_IndexBuckets[bucketIndex];
	ASSERT_RETNULL(bucket);
	return bucket->m_IndexArray + (index % PAKFILE_INDEX_BUFFER);
}


//----------------------------------------------------------------------------
// get FILE_INDEX_NODE based on fileid
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_INDEX::GetById(
	FILEID fileid)
{
	FILE_INDEX_NODE * node = NULL;

	unsigned int hashidx = (unsigned int)(fileid.id % FILEID_HASH_SIZE);

	{
		CRW_RP_AutoReadLock autolock(&m_CS);
		node = m_FileIdHash[hashidx];
		while (node)
		{
			if (node->m_fileId == fileid)
			{
				return node;
			}
			node = node->m_NextInHash;
		}
	}
	return node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILEID PakFileComputeFileId(
	const PStrT & strFilePath, 
	const PStrT & strFileName)
{
	FILEID fileid;

	UINT8 temp[SHA1HashSize];

	if (!strFilePath.IsEmpty())
	{
		ASSERT(SHA1Calculate((UINT8 *)strFilePath.str, strFilePath.len, temp));
		fileid.idDirectory = *(DWORD *)temp;
	}
	if (!strFileName.IsEmpty())
	{
		ASSERT(SHA1Calculate((UINT8 *)strFileName.str, strFileName.len, temp));
		fileid.idFile = *(DWORD *)temp;
	}

	return fileid;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PAKFILE_INDEX::AddFileIdToHash(
	FILE_INDEX_NODE * fileindex)
{
	ASSERT_RETURN(fileindex);
	ASSERT_RETURN(fileindex->m_fileId != INVALID_FILEID);
	unsigned int hashidx = (unsigned int)(fileindex->m_fileId.id % FILEID_HASH_SIZE);

	fileindex->m_NextInHash = m_FileIdHash[hashidx];
	m_FileIdHash[hashidx] = fileindex;
}


//----------------------------------------------------------------------------
// add a new FILE_INDEX_NODE
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_INDEX::AddFileIndex(
	const PStrT & strFilePath,
	const PStrT & strFileName,
	BOOL copy)
{
	FILE_INDEX_NODE * fileindex = NULL;

	FILEID fileid = PakFileComputeFileId(strFilePath, strFileName);
	{
		CRW_RP_AutoWriteLock autolock(&m_CS);
		fileindex = GetNewIndexNode();
		fileindex->Init(fileid);
		AddFileIdToHash(fileindex);
	}

	ASSERT_RETNULL(fileindex);

	if (copy)
	{
		SETBIT(fileindex->m_flags, PAKFLAG_FREE_FILENAME);
		fileindex->m_FilePath.Copy(strFilePath, m_Pool);
		fileindex->m_FileName.Copy(strFileName, m_Pool);
	}
	else
	{
		fileindex->m_FilePath.Set(strFilePath.str, strFilePath.Len());
		fileindex->m_FileName.Set(strFileName.str, strFileName.Len());
	}

	return fileindex;
}


//----------------------------------------------------------------------------
// add a new FILE_INDEX_NODE of a particular id
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_INDEX::AddFileIndexFixId(
	FILEID fileid,
	const PStrT & strFilePath,
	const PStrT & strFileName,
	BOOL copy)
{
	if (fileid == INVALID_FILEID)
	{
		fileid = PakFileComputeFileId(strFilePath, strFileName);
	}

	FILE_INDEX_NODE * fileindex = GetById(fileid);

	if (fileindex == NULL)
	{
		CRW_RP_AutoWriteLock autolock(&m_CS);

		fileindex = GetNewIndexNode();
		if (TESTBIT(fileindex->m_flags, PAKFLAG_FREE_FILENAME))
		{
			fileindex->m_FilePath.Free(m_Pool);
			fileindex->m_FileName.Free(m_Pool);
		}
		fileindex->Init(fileid);
		AddFileIdToHash(fileindex);
	}

	ASSERT_RETNULL(fileindex);

	if (copy)
	{
		SETBIT(fileindex->m_flags, PAKFLAG_FREE_FILENAME);
		fileindex->m_FilePath.Copy(strFilePath, m_Pool);
		fileindex->m_FileName.Copy(strFileName, m_Pool);
	}
	else
	{
		fileindex->m_FilePath.Set(strFilePath.str, strFilePath.Len());
		fileindex->m_FileName.Set(strFileName.str, strFileName.Len());
	}

	return fileindex;
}


//----------------------------------------------------------------------------
// get filename from fileid
//----------------------------------------------------------------------------
BOOL PAKFILE_INDEX::GetFileNameById(
	FILEID fileid,
	TCHAR * filename,
	unsigned int len)
{
	if (fileid == INVALID_FILEID)
	{
		return FALSE;
	}

	FILE_INDEX_NODE * fileindex = GetById(fileid);
	ASSERT_RETFALSE(fileindex);

	ASSERT_RETFALSE(!fileindex->m_FilePath.IsEmpty() && !fileindex->m_FileName.IsEmpty());

	return sCombineFilename(filename, len, fileindex->m_FilePath, fileindex->m_FileName);
}


//----------------------------------------------------------------------------
// PAKFILE_HASH functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// PAKFILE_HASH FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// initialize
//----------------------------------------------------------------------------
void PAKFILE_HASH::Init(
	struct MEMORYPOOL * pool)
{
	m_CS.Init();
	m_Pool = pool;
	structclear(m_Hash);
}


//----------------------------------------------------------------------------
// free all
//----------------------------------------------------------------------------
void PAKFILE_HASH::Free(
	void)
{
	m_CS.WriteLock();

	// free file index hash nodes
	for (unsigned int ii = 0; ii < PAKFILE_HASH_SIZE; ii++)
	{
		PAKFILE_HASH_NODE * curr = NULL;
		PAKFILE_HASH_NODE * next = m_Hash[ii];
		while ((curr = next) != NULL)
		{
			next = curr->m_next;
			FREE(m_Pool, curr);
		}
	}
	memclear(m_Hash, sizeof(m_Hash));

	m_CS.Free();
}


//----------------------------------------------------------------------------
// return key from filename for hash of FILE_HASH_NODE
//----------------------------------------------------------------------------
static DWORD sGetKey(
	const PStrT & filepath,
	const PStrT & filename)
{
	DWORD key = 0;
	const TCHAR * cur = filepath.str;
	const TCHAR * end = filepath.str + filepath.Len();
	while (cur < end)
	{
		key = STR_HASH_ADDC(key, *cur);
		cur++;
	}

	cur = filename.str;
	end = filename.str + filename.Len();
	while (cur < end)
	{
		key = STR_HASH_ADDC(key, *cur);
		cur++;
	}

	return key % PAKFILE_HASH_SIZE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD PAKFILE_HASH::GetKey(
	const PStrT & filepath,
	const PStrT & filename) const
{
	return sGetKey(filepath, filename);
}


//----------------------------------------------------------------------------
// add a FILE_INDEX_NODE to the hash
//----------------------------------------------------------------------------
void PAKFILE_HASH::AddToHash(
	FILE_INDEX_NODE * indexnode,
	unsigned int key)
{
	ASSERT_RETURN(indexnode);
	ASSERT_RETURN(key < PAKFILE_HASH_SIZE);

	PAKFILE_HASH_NODE * hashnode = (PAKFILE_HASH_NODE *)MALLOCZ(m_Pool, sizeof(PAKFILE_HASH_NODE));
	ASSERT_RETURN(hashnode);
	hashnode->m_FilePath.Set(indexnode->m_FilePath.str, indexnode->m_FilePath.Len());
	hashnode->m_FileName.Set(indexnode->m_FileName.str, indexnode->m_FileName.Len());
	hashnode->m_fileId = indexnode->m_fileId;

	hashnode->m_next = m_Hash[key];
	m_Hash[key] = hashnode;

	SETBIT(indexnode->m_flags, PAKFLAG_VALID_NODE);
}


//----------------------------------------------------------------------------
// return the FILE_INDEX_NODE given a filepath & filename
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_HASH::FindInHash(
	const PStrT & filepath,
	const PStrT & filename,
	DWORD key)
{
	PAKFILE_HASH_NODE * cur;
	PAKFILE_HASH_NODE * next = m_Hash[key % PAKFILE_HASH_SIZE];
	while ((cur = next) != NULL)
	{
		next = cur->m_next;
		if (filename != cur->m_FileName || filepath != cur->m_FilePath)
		{
			continue;
		}

		FILE_INDEX_NODE * node = g_Pakfile.m_FileIndex.GetById(cur->m_fileId);
		if (node)
		{
			if (!TESTBIT(node->m_flags, PAKFLAG_VALID_NODE))
			{
				return NULL;
			}
		}
		return node;
	}

	return NULL;
}


//----------------------------------------------------------------------------
// return the FILE_INDEX_NODE given a filepath & filename
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_HASH::FindInHash(
	const PStrT & filepath,
	const PStrT & filename,
	DWORD * key)
{
	ASSERT_RETNULL(key);
	*key = GetKey(filepath, filename);
	return FindInHash(filepath, filename, *key);
}


//----------------------------------------------------------------------------
// return the FILE_INDEX_NODE given a filepath & filename
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PAKFILE_HASH::FindInHash(
	const PStrT & filepath,
	const PStrT & filename)
{
	DWORD key = GetKey(filepath, filename);
	return FindInHash(filepath, filename, key);
}


//----------------------------------------------------------------------------
// add a file to the FILE_INDEX_NODE array & hash, 
// assigns a new fileid to the file
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileAddToHash(
	const PStrT & strFilePath,
	const PStrT & strFileName,
	DWORD key,
	BOOL copy)
{
	ASSERT_RETNULL(!strFilePath.IsEmpty() && !strFileName.IsEmpty());

	FILE_INDEX_NODE * fileindex = g_Pakfile.m_FileIndex.AddFileIndex(strFilePath, strFileName, copy);
	ASSERT_RETNULL(fileindex);

	g_Pakfile.m_FileHash.AddToHash(fileindex, key);

	return fileindex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PakFileAddToHash(
	const PStrT & strFilePath,
	const PStrT & strFileName,
	BOOL copy)
{
	DWORD key = sGetKey(strFilePath, strFileName);
	return sPakFileAddToHash(strFilePath, strFileName, key, copy);
}


//----------------------------------------------------------------------------
// add a file to the FILE_INDEX_NODE array & hash, 
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileAddToHashFixId(
	FILEID fileid,
	const PStrT & strFilePath,
	const PStrT & strFileName,
	DWORD key,
	BOOL copy)
{
	ASSERT_RETNULL(!strFilePath.IsEmpty() && !strFileName.IsEmpty());

	FILE_INDEX_NODE * fileindex = g_Pakfile.m_FileIndex.AddFileIndexFixId(fileid, strFilePath, strFileName, copy);
	ASSERT_RETNULL(fileindex);

	g_Pakfile.m_FileHash.AddToHash(fileindex, key);

	return fileindex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileGetFileIndexNode(
	PStrT & strFilePath,
	PStrT & strFileName,
	DWORD * key)
{
	return g_Pakfile.m_FileHash.FindInHash(strFilePath, strFileName, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileGetFileIndexNode(
	PStrT & strFilePath,
	PStrT & strFileName,
	DWORD key)
{
	return g_Pakfile.m_FileHash.FindInHash(strFilePath, strFileName, key);
}


//----------------------------------------------------------------------------
// PAKFILE member functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// close a PAKFILE
//----------------------------------------------------------------------------
void PAKFILE::Close(
	void)
{
	ASSERT(m_tempIndex.m_IndexNodes == NULL);

	if (m_filePakData)
	{
		AsyncFileCloseNow(m_filePakData);
		m_filePakData = NULL;
	}
	m_sizePakData = 0;
	if (m_filePakIndex)
	{
		AsyncFileCloseNow(m_filePakIndex);
		m_filePakIndex = NULL;
	}
	m_sizePakIndex = 0;

	m_strFilepaths.Free();
	m_idPakFile = INVALID_ID;
//	m_indexNumber = 0;
	m_genTime = 0;

	m_tempIndex.m_IndexNodes = NULL;
	m_tempIndex.m_NodeCount = 0;
	m_tempIndex.m_Size = 0;
	m_tempIndex.m_ActualCount = 0;

	m_csPakData.Free();
	m_csPakIndex.Free();
}


//----------------------------------------------------------------------------
// return a temporary index node from the pakfile (allocate if necessary)
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileGetTempIndexNode(
	PAKFILE * pakfile)
{
	ASSERT_RETNULL(pakfile);
	if (pakfile->m_tempIndex.m_NodeCount >= pakfile->m_tempIndex.m_Size)
	{
		pakfile->m_tempIndex.m_Size += PAK_TEMP_INDEX_BUFSIZE;
		pakfile->m_tempIndex.m_IndexNodes = (FILE_INDEX_NODE *)REALLOC(g_ScratchAllocator, pakfile->m_tempIndex.m_IndexNodes, sizeof(FILE_INDEX_NODE) * pakfile->m_tempIndex.m_Size);
	}
	FILE_INDEX_NODE * tempnode = pakfile->m_tempIndex.m_IndexNodes + pakfile->m_tempIndex.m_NodeCount;
	++pakfile->m_tempIndex.m_NodeCount;
	tempnode->m_fileId = INVALID_FILEID;
	tempnode->m_idPakFile = pakfile->m_idPakFile;
	tempnode->m_iPakType = pakfile->m_ePakEnum;
	return tempnode;
}


//----------------------------------------------------------------------------
// make an aborted temp index node available again
//----------------------------------------------------------------------------
static void sPakFileRecycleTempIndexNode(
	PAKFILE * pakfile,
	FILE_INDEX_NODE * tempnode)
{
	ASSERT_RETURN(tempnode == pakfile->m_tempIndex.m_IndexNodes + pakfile->m_tempIndex.m_NodeCount - 1);
	--pakfile->m_tempIndex.m_NodeCount;
}


//----------------------------------------------------------------------------
// PAKFILE_LIST member functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// initialize (not thread safe)
//----------------------------------------------------------------------------
void PAKFILE_LIST::Init(
	struct MEMORYPOOL *	mempool)
{
	m_CS.Init();
	m_Pool = mempool;
	m_Pakfiles = NULL;
	m_PakfileCount = 0;
}


//----------------------------------------------------------------------------
// close all pakfiles (not thread safe)
//----------------------------------------------------------------------------
void PAKFILE_LIST::Free(
	void)
{
	// should check here that all file threads are closed
	PAKFILE ** ptrPakfiles = m_Pakfiles;
	for (unsigned int ii = 0; ii < m_PakfileCount; ii++, ptrPakfiles++)
	{
		PAKFILE * pakfile = *ptrPakfiles;
		pakfile->Close();
		
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, pakfile);
#else		
		FREE(m_Pool, pakfile);
#endif
		
	}
	FREE(m_Pool, m_Pakfiles);
	m_Pakfiles = NULL;
	m_PakfileCount = 0;
	m_Pool = NULL;
}


//----------------------------------------------------------------------------
// return if index is valid pakfile
//----------------------------------------------------------------------------
BOOL PAKFILE_LIST::IsValidPakfileId(
	unsigned int pakfileid)
{
	BOOL result = FALSE;
	m_CS.ReadLock();
	ONCE
	{
		ASSERT_BREAK(pakfileid < m_PakfileCount);
		ASSERT_BREAK(m_Pakfiles);
		ASSERT_BREAK(m_Pakfiles[pakfileid]->m_idPakFile == pakfileid);
		ASSERT_BREAK(m_Pakfiles[pakfileid]->m_filePakData != NULL);
		result = TRUE;
	}
	m_CS.EndRead();
	return result;
}


//----------------------------------------------------------------------------
// return a pakfile pointer by the pakfile id
//----------------------------------------------------------------------------
PAKFILE * PAKFILE_LIST::GetById(
	unsigned int idPakfile)
{
	PAKFILE * pakfile = NULL;
	m_CS.ReadLock();
	ONCE
	{
		ASSERT_BREAK(idPakfile < m_PakfileCount);
		ASSERT_BREAK(m_Pakfiles[idPakfile]->m_idPakFile == idPakfile);
		pakfile = m_Pakfiles[idPakfile];
	}
	m_CS.EndRead();
	return pakfile;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PAK_ENUM PAKFILE_LIST::GetTypeById(
	unsigned int idPakfile)
{
	m_CS.ReadLock();
	ONCE
	{
		ASSERT_BREAK(idPakfile < m_PakfileCount);
		ASSERT_BREAK(m_Pakfiles[idPakfile]->m_idPakFile == idPakfile);
	}
	m_CS.EndRead();
	return (PAK_ENUM)m_Pakfiles[idPakfile]->m_ePakEnum;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PAKFILE_LIST::GetGMBuildVersion(
	PAK_ENUM ePakEnum,
	FILE_VERSION &tVersion)
{
	BOOL bFound = FALSE;
	m_CS.ReadLock();
	ONCE
	{
		PAKFILE ** ptrPakfiles = m_Pakfiles;
		for (unsigned int ii = 0; ii < m_PakfileCount; ii++, ptrPakfiles++)
		{
			PAKFILE * pakfile = *ptrPakfiles;
			if (pakfile->m_ePakEnum == ePakEnum)
			{
				tVersion = pakfile->m_tVersionBuildGM;
				bFound = TRUE;
				break;
			}			
		}	
	}
	m_CS.EndRead();
	return bFound;
}	

//----------------------------------------------------------------------------
// add a new pakfile
//----------------------------------------------------------------------------
PAKFILE * PAKFILE_LIST::AddPakfile(
	ASYNC_FILE * pakdata, 
	UINT64 pakdataSize, 
	ASYNC_FILE * pakindex, 
	UINT64 pakindexSize,
	PAK_ENUM ePakEnum,
	FILE_VERSION *pVersionBuild)
{
	ASSERT_RETNULL(pakdata != NULL);
	ASSERTX_RETNULL(pVersionBuild, "Expected version param");

#if ISVERSION(DEVELOPMENT)
	ASSERT_RETNULL(pakdataSize == AsyncFileGetSize(pakdata));
	if (pakindex)
	{
		ASSERT_RETNULL(pakindexSize == AsyncFileGetSize(pakindex));
	}
#endif

#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	PAKFILE * pakfile = (PAKFILE *)MALLOCZ(g_StaticAllocator, sizeof(PAKFILE));
#else
	PAKFILE * pakfile = (PAKFILE *)MALLOCZ(m_Pool, sizeof(PAKFILE));
#endif
	ASSERT_RETNULL(pakfile);

	m_CS.WriteLock();
	ONCE
	{
		unsigned int ii = m_PakfileCount++;
		m_Pakfiles = (PAKFILE **)REALLOCZ(m_Pool, m_Pakfiles, sizeof(PAKFILE *) * m_PakfileCount);
		m_Pakfiles[ii] = pakfile;
		pakfile->m_idPakFile = ii;
	}
	m_CS.EndWrite();

	pakfile->m_filePakData = pakdata;
	pakfile->m_sizePakData = pakdataSize;
	pakfile->m_csPakData.Init();

	pakfile->m_filePakIndex = pakindex;
	pakfile->m_sizePakIndex = pakindexSize;
	pakfile->m_csPakIndex.Init();

	pakfile->m_genTime = AsyncFileGetLastModifiedTime(pakdata);
	pakfile->m_strFilepaths.Init(m_Pool);
	pakfile->m_ePakEnum = ePakEnum;
	pakfile->m_tVersionBuildGM = *pVersionBuild;

	return pakfile;
}


//----------------------------------------------------------------------------
// PAKFILE_SYSTEM functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// read a consolidated pakindex node from memory
//----------------------------------------------------------------------------
static BOOL sPakFileReadPakIndexNodeFromBuffer(
	PAKFILE * pakfile,
	BYTE_BUF_OBFUSCATED & bbuf,
	FILE_INDEX_NODE * indexnode)
{
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(indexnode);

	DWORD dw;
	UINT8 tmpBuffer[SHA1HashSizeBuffer];

	ASSERT_RETFALSE(bbuf.PopUInt(&dw, sizeof(dw)));
	ASSERT_RETFALSE(dw == PAK_INDEX_NODE_MAGIC);

	indexnode->m_idPakFile = pakfile->m_idPakFile;
	indexnode->m_iPakType = pakfile->m_ePakEnum;

	if (pakfile->m_indexVersion == PAK_INDEX_BETA_VERSION)
	{
		FILEID_ALPHA fileIdAlpha;
		ASSERT_RETFALSE(bbuf.PopUInt(&fileIdAlpha, sizeof(fileIdAlpha)));
		// discard alpha file id
	}
	else
	{
		ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_fileId.id, sizeof(indexnode->m_fileId.id)));
	}
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_offset, sizeof(indexnode->m_offset)));
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_fileSize, sizeof(indexnode->m_fileSize)));
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_fileSizeCompressed, sizeof(indexnode->m_fileSizeCompressed)));
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_flags, sizeof(indexnode->m_flags)));
	ASSERT_RETFALSE(sPakFileReadPStrFromBufferOffset(bbuf, indexnode->m_FilePath, pakfile));
	ASSERT_RETFALSE(sPakFileReadPStrFromBufferOffset(bbuf, indexnode->m_FileName, pakfile));
	ASSERT_RETFALSE(bbuf.PopBuf(&indexnode->m_GenTime, sizeof(indexnode->m_GenTime)));
	ASSERT_RETFALSE(bbuf.PopBuf(&indexnode->m_Hash, sizeof(indexnode->m_Hash)));
	ASSERT_RETFALSE(bbuf.PopBuf(tmpBuffer, SHA1HashSizeBuffer));
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_FileHeader.dwMagicNumber, sizeof(indexnode->m_FileHeader.dwMagicNumber)));
	ASSERT_RETFALSE(bbuf.PopUInt(&indexnode->m_FileHeader.nVersion, sizeof(indexnode->m_FileHeader.nVersion)));

	ASSERT_RETFALSE(bbuf.PopUInt(&dw, sizeof(dw)));
	ASSERT_RETFALSE(dw == PAK_INDEX_NODE_MAGIC);

	ASSERT_RETFALSE(indexnode->m_GenTime != 0);
	ASSERT_RETFALSE(!indexnode->m_FilePath.IsEmpty() && !indexnode->m_FileName.IsEmpty());

	indexnode->m_bHasSvrHash = FALSE;
//	indexnode->m_SrvHash.Init();

	return TRUE;
}


//----------------------------------------------------------------------------
// read an unconsolidated pakindex node from a buffer
//----------------------------------------------------------------------------
static BOOL sPakFileReadUncosolidatedPakIndexNodeFromBuffer(
	PAKFILE * pakfile,
	BYTE_BUF_OBFUSCATED & bbuf,
	FILE_INDEX_NODE * indexnode)
{
	ASSERT_RETFALSE(indexnode);
	DWORD dw;
	UINT8 tmpBuffer[SHA1HashSizeBuffer];

	BYTE_BUF_OBFUSCATED ubuf(bbuf.GetPtr(), bbuf.GetRemainder(), PAK_INDEX_OBS_MULT, PAK_INDEX_OBS_ADD);

	ASSERT_RETFALSE(ubuf.PopUIntNoObs(&dw, sizeof(dw)));
	DWORD magic = dw;
	ubuf.SetMultAdd(PAK_INDEX_OBS_MULT, magic);

	ASSERT_RETFALSE(ubuf.PopUInt(&dw, sizeof(dw)));
	ASSERT_RETFALSE(dw == PAK_INDEX_NODE_MAGIC);

	if (pakfile->m_indexVersion == PAK_INDEX_BETA_VERSION)
	{
		FILEID_ALPHA fileIdAlpha;
		ASSERT_RETFALSE(ubuf.PopUInt(&fileIdAlpha, sizeof(fileIdAlpha)));
		// discard alpha file id
	}
	else
	{
		ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_fileId.id, sizeof(indexnode->m_fileId.id)));
	}
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_offset, sizeof(indexnode->m_offset)));
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_fileSize, sizeof(indexnode->m_fileSize)));
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_fileSizeCompressed, sizeof(indexnode->m_fileSizeCompressed)));
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_flags, sizeof(indexnode->m_flags)));

	ASSERT_RETFALSE(sPakFileReadPakStrFromBuffer(ubuf, indexnode->m_FilePath, pakfile));
	ASSERT_RETFALSE(sPakFileReadPakStrFromBuffer(ubuf, indexnode->m_FileName, pakfile));
	ASSERT_RETFALSE(ubuf.PopBuf(&indexnode->m_GenTime, sizeof(indexnode->m_GenTime)));
	ASSERT_RETFALSE(ubuf.PopBuf(&indexnode->m_Hash, sizeof(indexnode->m_Hash)));
	ASSERT_RETFALSE(ubuf.PopBuf(tmpBuffer, SHA1HashSizeBuffer));
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_FileHeader.dwMagicNumber, sizeof(indexnode->m_FileHeader.dwMagicNumber)));
	ASSERT_RETFALSE(ubuf.PopUInt(&indexnode->m_FileHeader.nVersion, sizeof(indexnode->m_FileHeader.nVersion)));

	ASSERT_RETFALSE(ubuf.PopUInt(&dw, sizeof(dw)));
	ASSERT_RETFALSE(dw == PAK_INDEX_NODE_MAGIC);
	ASSERT_RETFALSE(indexnode->m_GenTime != 0);
	ASSERT_RETFALSE(indexnode->m_FilePath.len == PAK_STRING_OFFSET_MARKER || !indexnode->m_FilePath.IsEmpty()); 
	ASSERT_RETFALSE(indexnode->m_FileName.len == PAK_STRING_OFFSET_MARKER || !indexnode->m_FileName.IsEmpty());

	indexnode->m_bHasSvrHash = FALSE;

	bbuf.SetCursor(bbuf.GetCursor() + ubuf.GetCursor());
	return TRUE;
}


//----------------------------------------------------------------------------
// validate data in temp index
//----------------------------------------------------------------------------
static BOOL sPakFileValidateTempIndex(
	PAKFILE * pakfile)
{
	ASSERT_RETFALSE(pakfile);

	FILE_INDEX_NODE * cur = pakfile->m_tempIndex.m_IndexNodes;
	FILE_INDEX_NODE * end = pakfile->m_tempIndex.m_IndexNodes + pakfile->m_tempIndex.m_NodeCount;
	for (; cur < end; ++cur)
	{
		// fix up offset written filename/path
		if (cur->m_FilePath.len == PAK_STRING_OFFSET_MARKER)
		{
			unsigned int offset = (unsigned int)((BYTE *)cur->m_FilePath.str - (BYTE *)0);
			cur->m_FilePath.Set(NULL, 0);
			ASSERT_RETFALSE(sPakFileGetStrFromOffset(cur->m_FilePath, offset, pakfile));
		}
		if (cur->m_FileName.len == PAK_STRING_OFFSET_MARKER)
		{
			unsigned int offset = (unsigned int)((BYTE *)cur->m_FileName.str - (BYTE *)0);
			cur->m_FileName.Set(NULL, 0);
			ASSERT_RETFALSE(sPakFileGetStrFromOffset(cur->m_FileName, offset, pakfile));
		}
		ASSERT_RETFALSE(!cur->m_FileName.IsEmpty());
		ASSERT_RETFALSE(!cur->m_FilePath.IsEmpty());

		if (pakfile->m_indexVersion == PAK_INDEX_BETA_VERSION)
		{
			cur->m_fileId = PakFileComputeFileId(cur->m_FilePath, cur->m_FileName);
		}
		else
		{
			ASSERT_RETFALSE(cur->m_fileId != INVALID_FILEID);
		}

		ASSERT_RETFALSE(cur->m_idPakFile == pakfile->m_idPakFile);
		ASSERT_RETFALSE(cur->m_iPakType == pakfile->m_ePakEnum);
		if (cur->m_fileSizeCompressed == 0) 
		{
			ASSERT_RETFALSE(cur->m_offset + cur->m_fileSize <= pakfile->m_sizePakData);
		} 
		else 
		{
			ASSERT_RETFALSE(cur->m_offset + cur->m_fileSizeCompressed <= pakfile->m_sizePakData);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sPakFilePruneTempIndexCompare(
	const void * elem1,
	const void * elem2)
{
	const FILE_INDEX_NODE * fileindexA = (const FILE_INDEX_NODE *)elem1;
	const FILE_INDEX_NODE * fileindexB = (const FILE_INDEX_NODE *)elem2;
	if (fileindexA->m_fileId.id < fileindexB->m_fileId.id)
	{
		return -1;
	}
	if (fileindexA->m_fileId.id < fileindexB->m_fileId.id)
	{
		return 1;
	}
	if (fileindexA < fileindexB)
	{
		return -1;
	}
	if (fileindexA > fileindexB)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
// find duplicate (overwritten) nodes (mark with PAKFLAG_DEPECRATED_TEMPNODE)
// calculate actual # of unique indexes in temp index
//----------------------------------------------------------------------------
static BOOL sPakFilePruneTempIndex(
	PAKFILE * pakfile,
	ASYNC_FILE * pakindex)
{
	REF(pakindex);
	pakfile->m_tempIndex.m_ActualCount = pakfile->m_tempIndex.m_NodeCount;
/*
	pakfile->m_tempIndex.m_ActualCount = 0;

	if (pakfile->m_tempIndex.m_NodeCount == 0)
	{
		return TRUE;
	}

	// for each node in tempIndex, mark it as deprecated if we find a matching node later on
	qsort(pakfile->m_tempIndex.m_IndexNodes, pakfile->m_tempIndex.m_NodeCount, sizeof(FILE_INDEX_NODE), sPakFilePruneTempIndexCompare);

	for (unsigned int ii = 0; ii < pakfile->m_tempIndex.m_NodeCount - 1; ++ii)
	{
		FILE_INDEX_NODE * cur = pakfile->m_tempIndex.m_IndexNodes + ii;
		FILE_INDEX_NODE * cmp = pakfile->m_tempIndex.m_IndexNodes + ii + 1;

		if (cur->m_fileId == cmp->m_fileId)
		{
			SETBIT(cur->m_flags, PAKFLAG_DEPECRATED_TEMPNODE);
			ASSERTONCE(PStrCmp(cur->m_FilePath.str, cmp->m_FilePath.str) == 0);
			ASSERTONCE(PStrCmp(cur->m_FileName.str, cmp->m_FileName.str) == 0);
		}
		else
		{
			++pakfile->m_tempIndex.m_ActualCount;
		}
	}
	++pakfile->m_tempIndex.m_ActualCount;

	PAKLOAD_TRACE("PakIndex Load [%s]  Pruned %d of %d temporary index nodes.", 
		OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(pakindex)), pakfile->m_tempIndex.m_NodeCount - pakfile->m_tempIndex.m_ActualCount, pakfile->m_tempIndex.m_NodeCount);
*/
	return TRUE;
}


//----------------------------------------------------------------------------
// run through the temp nodes and validate them, prune overwritten nodes
//----------------------------------------------------------------------------
static BOOL sPakFileFixTempIndex(
	PAKFILE * pakfile,
	ASYNC_FILE * pakindex)
{
	if (!sPakFileValidateTempIndex(pakfile))
	{
		return FALSE;
	}

	if (!sPakFilePruneTempIndex(pakfile, pakindex))
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// add a global index node from a temp index node
//----------------------------------------------------------------------------
static BOOL sPakFileAddGlobalIndexNodeFromTemp(
	const FILE_INDEX_NODE * pakindex)
{
	ASSERT_RETFALSE(pakindex);

	// add the node to the global index
	DWORD key;
	FILE_INDEX_NODE * filenode = NULL;

	g_Pakfile.m_FileHash.m_CS.ReadLock();
	{
		filenode = g_Pakfile.m_FileHash.FindInHash(pakindex->m_FilePath, pakindex->m_FileName, &key);
	}
	g_Pakfile.m_FileHash.m_CS.EndRead();
	if (!filenode)
	{
		filenode = sPakFileAddToHashFixId(pakindex->m_fileId, pakindex->m_FilePath, pakindex->m_FileName, key, FALSE);
	}
	ASSERT_RETFALSE(filenode);

	BOOL bReplace = FALSE;
	TCHAR fullfilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(fullfilename, DEFAULT_FILE_WITH_PATH_SIZE, _T("%s%s"), pakindex->m_FilePath.str, pakindex->m_FileName.str);
	PFILE_HASH_INFO pFileHashInfo;
	if ((pFileHashInfo = g_FileHashTable.GetItem(fullfilename)) != NULL) 
	{
		if (filenode->m_Hash != pFileHashInfo->hash) 
		{
			if (pakindex->m_Hash == pFileHashInfo->hash) 
			{
				//TracePersonal("Loaded from specific %s", fullfilename);
				bReplace = TRUE;
			} 
			else if (filenode->m_GenTime < pakindex->m_GenTime) 
			{
				//TracePersonal("Loaded from time %s", fullfilename);
				bReplace = TRUE;
			}
		}
	} 
	else if (filenode->m_GenTime < pakindex->m_GenTime) 
	{
		//TracePersonal("Loaded from time %s", fullfilename);
		bReplace = TRUE;
	}

	if (bReplace) 
	{
		filenode->m_offset = pakindex->m_offset;
		filenode->m_fileSize = pakindex->m_fileSize;
		filenode->m_fileSizeCompressed = pakindex->m_fileSizeCompressed;
		filenode->m_idPakFile = pakindex->m_idPakFile;
		filenode->m_iPakType = pakindex->m_iPakType;
		filenode->m_GenTime = pakindex->m_GenTime;
		filenode->m_Hash = pakindex->m_Hash;
		filenode->m_SvrHash = pakindex->m_SvrHash;
		filenode->m_bHasSvrHash = pakindex->m_bHasSvrHash;
		filenode->m_bAlreadyWrittenToXml = FALSE;
		//	MemoryCopy(&filenode->m_Hash, sizeof(filenode->m_Hash), &pakindex->m_Hash, sizeof(pakindex->m_Hash));
		//	MemoryCopy(&filenode->m_SrvHash, sizeof(filenode->m_SrvHash), &pakindex->m_SrvHash, sizeof(pakindex->m_SrvHash));
		ASSERT_RETFALSE(MemoryCopy(&filenode->m_FileHeader, sizeof(filenode->m_FileHeader), &pakindex->m_FileHeader, sizeof(pakindex->m_FileHeader)));
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// run through the temp nodes and add them to the global index
//----------------------------------------------------------------------------
static BOOL sPakFileAddTempNodes(
	PAKFILE * pakfile,
	ASYNC_FILE * pakindex)
{
	REF(pakindex);

	const FILE_INDEX_NODE * cur = pakfile->m_tempIndex.m_IndexNodes;
	const FILE_INDEX_NODE * end = pakfile->m_tempIndex.m_IndexNodes + pakfile->m_tempIndex.m_NodeCount;
	for (; cur < end; ++cur)
	{
		if (TESTBIT(cur->m_flags, PAKFLAG_DEPECRATED_TEMPNODE))
		{
			continue;
		}
		if (!sPakFileAddGlobalIndexNodeFromTemp(cur))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// free temp nodes
//----------------------------------------------------------------------------
static void sPakFileFreeTempIndex(
	PAKFILE * pakfile)
{
	ASSERT_RETURN(pakfile);
	if (pakfile->m_tempIndex.m_IndexNodes)
	{
		FREE(g_ScratchAllocator, pakfile->m_tempIndex.m_IndexNodes);
		pakfile->m_tempIndex.m_IndexNodes = NULL;
	}
	pakfile->m_tempIndex.m_Size = 0;
	pakfile->m_tempIndex.m_NodeCount = 0;
	pakfile->m_tempIndex.m_ActualCount = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sPakFileGetWastedSpace(
	PAKFILE * pakfile)
{
	ASSERT_RETVAL(pakfile, 0.0f);

	UINT64 totalSize = pakfile->m_sizePakData - PAKDATA_HEADER_SIZE;
	if (totalSize <= PAKDATA_HEADER_SIZE)
	{
		return 0.0f;
	}

	UINT64 usedSize = 0;

	FILE_INDEX_NODE * cur = pakfile->m_tempIndex.m_IndexNodes;
	FILE_INDEX_NODE * end = pakfile->m_tempIndex.m_IndexNodes + pakfile->m_tempIndex.m_NodeCount;
	for (; cur < end; ++cur)
	{
		if (TESTBIT(cur->m_flags, PAKFLAG_DEPECRATED_TEMPNODE))
		{
			continue;
		}
		if (cur->m_fileSizeCompressed == 0) {
			usedSize += cur->m_fileSize;
		} else {
			usedSize += cur->m_fileSizeCompressed;
		}
	}
	ASSERT(totalSize >= usedSize);
	return ((float)(totalSize - usedSize) / (float)totalSize);
}


//----------------------------------------------------------------------------
// write a consolidated pakindex node to a buffer
//----------------------------------------------------------------------------
static BOOL sPakFileWritePakIndexNodeToBuffer(
	BYTE_BUF_OBFUSCATED & bbuf,
	PAKFILE * pakfile,
	const FILE_INDEX_NODE * indexnode,
	BOOL bCalcOnly = FALSE)
{
	if (!bCalcOnly)		// we're just calling this to get the size of an index node
	{
		ASSERT_RETFALSE(!indexnode->m_FilePath.IsEmpty() && !indexnode->m_FilePath.IsEmpty());
		ASSERT_RETFALSE(indexnode->m_GenTime != 0);
	}

	ASSERT_RETFALSE(bbuf.PushUInt(PAK_INDEX_NODE_MAGIC, sizeof(DWORD)));
	if (pakfile->m_indexVersion == PAK_INDEX_BETA_VERSION)
	{
		// discard alpha file id
		FILEID_ALPHA fileIdAlpha = 0;
		ASSERT_RETFALSE(bbuf.PushUInt(fileIdAlpha, sizeof(fileIdAlpha)));
	}
	else
	{
		ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileId.id, sizeof(indexnode->m_fileId.id)));
	}
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_offset, sizeof(indexnode->m_offset)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileSize, sizeof(indexnode->m_fileSize)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileSizeCompressed, sizeof(indexnode->m_fileSizeCompressed)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_flags & PAK_INDEX_FLAG_SAVE_MASK, sizeof(indexnode->m_flags)));
	if (!bCalcOnly)
	{
		ASSERT_RETFALSE(sPakFileWritePakStrOffsetToBuffer(bbuf, indexnode->m_FilePath, pakfile));
		ASSERT_RETFALSE(sPakFileWritePakStrOffsetToBuffer(bbuf, indexnode->m_FileName, pakfile));
	}
	else
	{
		unsigned int offset = 0;
		ASSERT_RETFALSE(bbuf.PushUInt(offset, sizeof(offset)));
		ASSERT_RETFALSE(bbuf.PushUInt(offset, sizeof(offset)));
	}
	ASSERT_RETFALSE(bbuf.PushBuf(&indexnode->m_GenTime, sizeof(indexnode->m_GenTime)));
	ASSERT_RETFALSE(bbuf.PushBuf(&indexnode->m_Hash, sizeof(indexnode->m_Hash)));
	{
		UINT8 tmpBuffer[SHA1HashSizeBuffer];
		memset(tmpBuffer, 0, SHA1HashSizeBuffer);
		ASSERT_RETFALSE(bbuf.PushBuf(tmpBuffer, SHA1HashSizeBuffer));
	}
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_FileHeader.dwMagicNumber, sizeof(indexnode->m_FileHeader.dwMagicNumber)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_FileHeader.nVersion, sizeof(indexnode->m_FileHeader.nVersion)));
	ASSERT_RETFALSE(bbuf.PushUInt(PAK_INDEX_NODE_MAGIC, sizeof(DWORD)));
	return TRUE;
}


//----------------------------------------------------------------------------
// write an unconsolidated pakindex node to a buffer
//----------------------------------------------------------------------------
static BOOL sPakFileWriteUncosolidatedIndexToBuffer(
	BYTE_BUF_OBFUSCATED & bbuf,
	PAKFILE * pakfile,
	const FILE_INDEX_NODE * indexnode)
{
	ASSERT_RETFALSE(!indexnode->m_FilePath.IsEmpty() && !indexnode->m_FileName.IsEmpty());
	ASSERT_RETFALSE(indexnode->m_GenTime != 0);

	ASSERT_RETFALSE(bbuf.PushUInt(PAK_INDEX_NODE_MAGIC, sizeof(DWORD)));
	if (pakfile->m_indexVersion == PAK_INDEX_BETA_VERSION)
	{
		// discard alpha file id
		FILEID_ALPHA fileIdAlpha = 0;
		ASSERT_RETFALSE(bbuf.PushUInt(fileIdAlpha, sizeof(fileIdAlpha)));
	}
	else
	{
		ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileId.id, sizeof(indexnode->m_fileId.id)));
	}
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_offset, sizeof(indexnode->m_offset)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileSize, sizeof(indexnode->m_fileSize)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_fileSizeCompressed, sizeof(indexnode->m_fileSizeCompressed)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_flags & PAK_INDEX_FLAG_SAVE_MASK, sizeof(indexnode->m_flags)));

	ASSERT_RETFALSE(sPakFileWritePakStrToBuffer(bbuf, indexnode->m_FilePath, pakfile));
	ASSERT_RETFALSE(sPakFileWritePakStrToBuffer(bbuf, indexnode->m_FileName, pakfile));
	ASSERT_RETFALSE(bbuf.PushBuf(&indexnode->m_GenTime, sizeof(indexnode->m_GenTime)));
	ASSERT_RETFALSE(bbuf.PushBuf(&indexnode->m_Hash, sizeof(indexnode->m_Hash)));
	{
		UINT8 tmpBuffer[SHA1HashSizeBuffer];
		memset(tmpBuffer, 0, SHA1HashSizeBuffer);
		ASSERT_RETFALSE(bbuf.PushBuf(tmpBuffer, SHA1HashSizeBuffer));
	}
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_FileHeader.dwMagicNumber, sizeof(indexnode->m_FileHeader.dwMagicNumber)));
	ASSERT_RETFALSE(bbuf.PushUInt(indexnode->m_FileHeader.nVersion, sizeof(indexnode->m_FileHeader.nVersion)));
	ASSERT_RETFALSE(bbuf.PushUInt(PAK_INDEX_NODE_MAGIC, sizeof(DWORD)));
	return TRUE;
}


//----------------------------------------------------------------------------
// calculate the size of a consolidated pakindex node
//----------------------------------------------------------------------------
static unsigned int sPakFileCalcPakIndexNodeSize(
	PAKFILE * pakfile)
{
	BYTE buffer[256];
	BYTE_BUF_OBFUSCATED bbuf(buffer, 256, 0, 0);
	FILE_INDEX_NODE index_node;
	structclear(index_node);
	ASSERT_RETZERO(sPakFileWritePakIndexNodeToBuffer(bbuf, pakfile, &index_node, TRUE));
	return bbuf.GetLen();
}


//----------------------------------------------------------------------------
// return the size of a consolidated pakindex node
//----------------------------------------------------------------------------
static unsigned int sPakFileGetPakIndexNodeSize(
	PAKFILE * pakfile)
{
	static unsigned int pak_index_node_size = sPakFileCalcPakIndexNodeSize(pakfile);
	return pak_index_node_size;
}


//----------------------------------------------------------------------------
// calculate the amount of memory required to store a pakindex
// this is done immediately after reading a pakindex file in order to
// generate the consolidated index
//----------------------------------------------------------------------------
static BOOL sPakFileCalcPakIndexSize(
	PAKFILE * pakfile,
	unsigned int & bufsize)
{
	bufsize = 0;

	ASSERT_RETFALSE(pakfile);

	PAK_INDEX_HEADER hdr;
	bufsize += sizeof(hdr.m_Hdr.dwMagic);
	bufsize += sizeof(hdr.m_Hdr.dwVersion);
	bufsize += sizeof(hdr.m_Count);

	// calculate buffer size of filename buffer
	bufsize += pakfile->m_strFilepaths.CalcSize();

	// calculate size of index nodes
	bufsize += pakfile->m_tempIndex.m_NodeCount * sPakFileGetPakIndexNodeSize(pakfile);

	return TRUE;
}


//----------------------------------------------------------------------------
// write a pakindex to a buffer, allocates the buffer
// called after we open a pakindex, used to save the consolidated index
// entries back into a pakindex file
//----------------------------------------------------------------------------
static BOOL sPakFileWritePakIndexToBuffer(
	PAKFILE * pakfile,
	BYTE_BUF_OBFUSCATED & bbuf)
{
	ASSERT_RETFALSE(pakfile);

	PAK_INDEX_HEADER hdr;
	structclear(hdr);
	hdr.m_Hdr.dwMagic = PAK_INDEX_MAGIC;
	hdr.m_Hdr.dwVersion = pakfile->m_indexVersion;

	ASSERT_RETFALSE(bbuf.PushUInt(hdr.m_Hdr.dwMagic, sizeof(hdr.m_Hdr.dwMagic)));
	ASSERT_RETFALSE(bbuf.PushUInt(hdr.m_Hdr.dwVersion, sizeof(hdr.m_Hdr.dwVersion)));

	hdr.m_Count = pakfile->m_tempIndex.m_ActualCount;
	ASSERT_RETFALSE(bbuf.PushUInt(hdr.m_Count, sizeof(hdr.m_Count)));

	// write filename buffer
	ASSERT_RETFALSE(pakfile->m_strFilepaths.WriteToBuffer(bbuf));
	
	// write index nodes
	FILE_INDEX_NODE * cur = pakfile->m_tempIndex.m_IndexNodes;
	FILE_INDEX_NODE * end = cur + pakfile->m_tempIndex.m_NodeCount;

	unsigned int written = 0;

	for (; cur < end; ++cur)
	{
		if (TESTBIT(cur->m_flags, PAKFLAG_DEPECRATED_TEMPNODE))
		{
			continue;
		}
		ASSERT_RETFALSE(sPakFileWritePakIndexNodeToBuffer(bbuf, pakfile, cur));
		++written;
	}

	ASSERT(written == hdr.m_Count);

	return TRUE;
}


//----------------------------------------------------------------------------
// generate a temp pakindex filename based on the fullfilename of the pakindex
//----------------------------------------------------------------------------
static void sFileSystemGenerateTempPakIndexFilename(
	const OS_PATH_CHAR * fullfilename,
	OS_PATH_CHAR * tempfilename,
	unsigned int strsize)
{
	PStrPrintf(tempfilename, strsize, OS_PATH_TEXT("%s%s"), fullfilename, OS_PATH_TEXT(".tmp"));
}


//----------------------------------------------------------------------------
// write a consolidated pakindex to a file from the temp nodes
//----------------------------------------------------------------------------
static BOOL sPakFileWritePakIndex(
	PAKFILE * pakfile,
	const OS_PATH_CHAR * filename)
{
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(filename);

	pakfile->m_indexVersion = PAK_INDEX_RELEASE_VERSION;

	// get the size of the buffer needed & allocate the buffer
	unsigned int bufsize;
	ASSERT_RETFALSE(sPakFileCalcPakIndexSize(pakfile, bufsize));

	BYTE * buffer = (BYTE *)MALLOC(g_ScratchAllocator, bufsize);
	BYTE_BUF_OBFUSCATED bbuf(buffer, bufsize, PAK_INDEX_OBS_MULT, PAK_INDEX_OBS_ADD);

	BOOL result = FALSE;

	ONCE
	{
		// write the index to the buffer
		ASSERT_BREAK(sPakFileWritePakIndexToBuffer(pakfile, bbuf));

		OS_PATH_CHAR tempfilename[MAX_PATH];
		sFileSystemGenerateTempPakIndexFilename(filename, tempfilename, arrsize(tempfilename));
		ASSERT_BREAK(FileSave(tempfilename, buffer, bbuf.GetLen()));

		// delete the old pakindex file & rename the temp file
		ASSERT_BREAK(FileRename(tempfilename, filename, TRUE));

		result = TRUE;
	}

	if (buffer)
	{
		FREE(g_ScratchAllocator, buffer);
	}

	return result;
}


//----------------------------------------------------------------------------
// read a pakindex from a buffer
// called when we open a new pakindex
//----------------------------------------------------------------------------
BOOL sPakFileReadPakIndexFromBuffer(
	PAKFILE * pakfile,
	ASYNC_FILE * & pakindex,
	void * buffer,
	unsigned int buflen,
	BOOL & bRewrite)
{
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(buffer);

	BYTE_BUF_OBFUSCATED bbuf(buffer, buflen, PAK_INDEX_OBS_MULT, PAK_INDEX_OBS_ADD);

	PAK_INDEX_HEADER hdr = {0};
	ASSERT_GOTO(bbuf.PopUInt(&hdr.m_Hdr.dwMagic, sizeof(hdr.m_Hdr.dwMagic)), _error);
	if (hdr.m_Hdr.dwMagic != PAK_INDEX_MAGIC)
	{
		goto _error;
	}
	ASSERT_GOTO(bbuf.PopUInt(&hdr.m_Hdr.dwVersion, sizeof(hdr.m_Hdr.dwVersion)), _error);
	ASSERT_GOTO(hdr.m_Hdr.dwVersion == PAK_INDEX_BETA_VERSION || hdr.m_Hdr.dwVersion == PAK_INDEX_RELEASE_VERSION, _error);
	ASSERT_GOTO(bbuf.PopUInt(&hdr.m_Count, sizeof(hdr.m_Count)), _error);

	pakfile->m_indexVersion = hdr.m_Hdr.dwVersion;

	// read string buffer
	ASSERT_GOTO(pakfile->m_strFilepaths.ReadFromBuffer(bbuf), _error);

	// read consolidated file index nodes
	for (unsigned int ii = 0; ii < hdr.m_Count; ++ii)
	{
		FILE_INDEX_NODE * tempnode = sPakFileGetTempIndexNode(pakfile);

		// read the data
		if (!sPakFileReadPakIndexNodeFromBuffer(pakfile, bbuf, tempnode))
		{
			// recycle temp index node
			sPakFileRecycleTempIndexNode(pakfile, tempnode);
			goto _error;
		}
	}

	PAKLOAD_TRACE("PakIndex Load [%s]  Read %d consolidated index nodes.", 
		OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(pakindex)), hdr.m_Count);

	// read unconsolidated nodes  -- NOTE, they may not be read in fileid order because of asynchronous writes!!!
	unsigned int unconsolidatedCount = 0;
	BOOL bForceRewrite = FALSE;
	while (bbuf.GetRemainder() > 0)
	{
		FILE_INDEX_NODE * tempnode = sPakFileGetTempIndexNode(pakfile);
		
		if (!sPakFileReadUncosolidatedPakIndexNodeFromBuffer(pakfile, bbuf, tempnode))
		{
			sPakFileRecycleTempIndexNode(pakfile, tempnode);
			bForceRewrite = TRUE;
			break;
		}

		unconsolidatedCount++;
	}

	// run through all the temp nodes and validate that their data seems reasonable,
	// prune overwritten indexes
	if (!sPakFileFixTempIndex(pakfile, pakindex))
	{
		goto _error;
	}

	PAKLOAD_TRACE("Pakfile Load [%s] Wasted Space: %2.8f pct.", 
		OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(pakfile->m_filePakData)), sPakFileGetWastedSpace(pakfile) * 100.0f);

	// add all of the temp nodes to the global index
	if (!sPakFileAddTempNodes(pakfile, pakindex))
	{
		goto _error;
	}

	// write the consolidated file if we have any errors or unsocolidated indexes
	if (bRewrite && (bForceRewrite || unconsolidatedCount > 0) && (AsyncFileGetFlags(pakindex) & FF_WRITE))
	{
		OS_PATH_CHAR filename[MAX_PATH];
		PStrCopy(filename, AsyncFileGetFilename(pakindex), arrsize(filename));
		AsyncFileCloseNow(pakindex);
		pakindex = NULL;
		sPakFileWritePakIndex(pakfile, filename);
	}
	else
	{
		bRewrite = FALSE;
	}

	// free temp nodes
	sPakFileFreeTempIndex(pakfile);

	return TRUE;

_error:
	// free string table
	pakfile->m_strFilepaths.Free();

	// free temp nodes
	sPakFileFreeTempIndex(pakfile);

	return FALSE;
}


//----------------------------------------------------------------------------
// open a pakindex, parse it, and re-write the pakindex after consolidation
//----------------------------------------------------------------------------
template <class CHARTYPE>
static BOOL sPakFileOpenPakIndex(
	const CHARTYPE * fullfilename,
	PAKFILE * pakfile,
	BOOL bReadOnly)
{
	UINT64 pakindexSize = 0;
	BYTE * buffer = NULL;
	BOOL result = FALSE;
	BOOL reread = TRUE;

reopen:
	ASYNC_FILE * pakindex = NULL;

	ONCE
	{
		// open the file
		{
			pakindex = AsyncFileOpen(fullfilename, FF_READ | (bReadOnly ? FF_SHARE_READ : (FF_SHARE_READ | FF_WRITE)) | FF_RANDOM_ACCESS);
			ASSERT_BREAK(pakindex);
		}

		// read it into a buffer
		pakindexSize = AsyncFileGetSize(pakindex);

		if (reread)
		{
			buffer = (BYTE *)MALLOC(g_ScratchAllocator, (DWORD)pakindexSize);
			ASSERT_BREAK(buffer);
			ASYNC_DATA asyncData(pakindex, buffer, 0, (DWORD)pakindexSize);
			ASSERT_BREAK(pakindexSize == AsyncFileReadNow(asyncData));

			// parse the buffer
			if (!sPakFileReadPakIndexFromBuffer(pakfile, pakindex, buffer, (DWORD)pakindexSize, reread))
			{
				break;
			}

			// reopen the file if we wrote a consolidated file
			if (reread)
			{
				reread = FALSE;
				goto reopen;
			}
		}

		pakfile->m_filePakIndex = pakindex;
		pakfile->m_sizePakIndex = pakindexSize;
		result = TRUE;
	}

	if (buffer)
	{
		FREE(g_ScratchAllocator, buffer);
	}
	if (result == FALSE)
	{
		if (pakindex)
		{
			AsyncFileClose(pakindex);
		}
	}

	return result;
}

static BOOL sPakFileOpenPakIndex(
	const CHAR * fullfilename,
	PAKFILE * pakfile,
	BOOL bReadOnly)
{
	return sPakFileOpenPakIndex<CHAR>(fullfilename, pakfile, bReadOnly);
}

static BOOL sPakFileOpenPakIndex(
	const WCHAR * fullfilename,
	PAKFILE * pakfile,
	BOOL bReadOnly)
{
	return sPakFileOpenPakIndex<WCHAR>(fullfilename, pakfile, bReadOnly);
}

//----------------------------------------------------------------------------
// generate empty index file & data file, then add the pakfile 
//----------------------------------------------------------------------------
static UINT32 sGetTempFileName(LPCSTR szPath, LPCSTR szPrefix, UINT32 index, LPSTR szBuffer)
{
	return GetTempFileNameA(szPath, szPrefix, index, szBuffer);
}

static UINT32 sGetTempFileName(LPCWSTR szPath, LPCSTR szPrefix, UINT32 index, LPWSTR szBuffer)
{
	WCHAR szPrefixW[4];
	PStrCvt(szPrefixW, szPrefix, 4);
	return GetTempFileNameW(szPath, szPrefixW, index, szBuffer);
}

static INT32 sGenerateFullPakFileName(LPSTR szBuffer, INT32 iBufLen, LPCSTR szPath, LPCSTR szBaseName, INT32 iIndex, LPCSTR szExt)
{
	return PStrPrintf(szBuffer, iBufLen, "%s%s%03d.%s", szPath, szBaseName, iIndex, szExt);
}

static INT32 sGenerateFullPakFileName(LPWSTR szBuffer, INT32 iBufLen, LPCWSTR szPath, LPCWSTR szBaseName, INT32 iIndex, LPCSTR szExt)
{
	WCHAR szExtW[4];
	PStrCvt(szExtW, szExt, 4);
	return PStrPrintf(szBuffer, iBufLen, L"%s%s%03d.%s", szPath, szBaseName, iIndex, szExtW);
}

template <class CHARTYPE>
UINT32 PakFileCreateNewPakfile(
	const CHARTYPE* strPath,
	const CHARTYPE* strBaseName,
	PAK_ENUM pakType,
	const CHARTYPE* strPakDatName,
	const CHARTYPE* strPakIdxName)
{
	ASSERT_RETFALSE(strBaseName);

	CHARTYPE pakdataFilename[MAX_PATH];	// filename w/ path of pakdata
	CHARTYPE pakindexFilename[MAX_PATH];	// filename w/ path of pakindex
	DWORD flags;

	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) 
	{
		sGetTempFileName(strPath, "pak", 0, pakdataFilename);
		sGetTempFileName(strPath, "idx", 0, pakindexFilename);
		flags = FF_READ | FF_WRITE | FF_OPEN_ALWAYS | FF_RANDOM_ACCESS | FF_TEMPORARY;
	} 
	else 
	{
		if (strPakDatName != NULL) {
			PStrCopy(pakdataFilename, strPakDatName, arrsize(pakdataFilename));
		} else {
			sGenerateFullPakFileName(pakdataFilename, arrsize(pakdataFilename), strPath, strBaseName, 0, PAK_DATA_EXT);
		}
		if (strPakIdxName != NULL) {
			PStrCopy(pakindexFilename, strPakIdxName, arrsize(pakindexFilename));
		} else {
			sGenerateFullPakFileName(pakindexFilename, arrsize(pakindexFilename), strPath, strBaseName, 0, PAK_INDEX_EXT);
		}
		flags = FF_READ | FF_WRITE | FF_CREATE_ALWAYS | FF_RANDOM_ACCESS | FF_SHARE_READ;
	}

	ASYNC_FILE * pakdata = NULL;
	ASYNC_FILE * pakindex = NULL;
	UINT32 idPakfile = INVALID_ID;
	ONCE
	{
		
		// get the current build version
		FILE_VERSION tVersionBuild;
		FileVersionGetBuildVersion( tVersionBuild );
		
		// create pakdata file
		unsigned int sizePakdata = 0;
		{
			pakdata = AsyncFileOpen(pakdataFilename, flags);
			ASSERT_BREAK(pakdata != NULL);

			PAK_DATA_HEADER hdr;
			structclear(hdr);
			hdr.m_Hdr.dwMagic = PAK_DATA_MAGIC;
			hdr.m_Hdr.dwVersion = PAK_DATA_VERSION;
			hdr.m_Hdr.tVersionBuild = tVersionBuild;

			BYTE buffer[4096];
			BYTE_BUF bbuf(buffer, arrsize(buffer));

			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.dwMagic,						sizeof(hdr.m_Hdr.dwMagic)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.dwVersion,						sizeof(hdr.m_Hdr.dwVersion)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.tVersionBuild.dwVerTitle,		sizeof(hdr.m_Hdr.tVersionBuild.dwVerTitle)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.tVersionBuild.dwVerPUB,		sizeof(hdr.m_Hdr.tVersionBuild.dwVerPUB)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.tVersionBuild.dwVerRC,			sizeof(hdr.m_Hdr.tVersionBuild.dwVerRC)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.tVersionBuild.dwVerML,			sizeof(hdr.m_Hdr.tVersionBuild.dwVerML)));
			sizePakdata = bbuf.GetLen();
			ASSERT(sizePakdata == PAKDATA_HEADER_SIZE);

#if ISVERSION(DEVELOPMENT)
			// Write the "fingerprint" string for EA
			if (AppIsHellgate()) {
				for (UINT32 i = 0; i < ARRAYSIZE(FingerPrintString); i++) {
					bbuf.PushBuf(FingerPrintString[i]+FP_PADDING_SIZE,
						PStrLen(FingerPrintString[i]+FP_PADDING_SIZE));
				}
				sizePakdata = bbuf.GetLen();
			}
#endif

			ASYNC_DATA asyncData(pakdata, buffer, 0, sizePakdata);
			ASSERT_BREAK(AsyncFileWriteNow(asyncData));
		}

		// create pakindex file
		unsigned int sizePakIndex = 0;
		{
			pakindex = AsyncFileOpen(pakindexFilename, flags);
			ASSERT_BREAK(pakindex != NULL);

			PAK_INDEX_HEADER hdr;
			structclear(hdr);
			hdr.m_Hdr.dwMagic = PAK_INDEX_MAGIC;
			hdr.m_Hdr.dwVersion = PAK_INDEX_RELEASE_VERSION;

			BYTE buffer[4096];
			BYTE_BUF_OBFUSCATED bbuf(buffer, 4096, PAK_INDEX_OBS_MULT, PAK_INDEX_OBS_ADD);

			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.dwMagic,						sizeof(hdr.m_Hdr.dwMagic)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Hdr.dwVersion,						sizeof(hdr.m_Hdr.dwVersion)));
			ASSERT_BREAK(bbuf.PushUInt(hdr.m_Count,								sizeof(hdr.m_Count)));
			PAK_STRING_TABLE::WriteEmptyToBuffer(bbuf);
			sizePakIndex = bbuf.GetLen();
			ASYNC_DATA asyncData(pakindex, buffer, 0, sizePakIndex);
			ASSERT_BREAK(AsyncFileWriteNow(asyncData));
		}

		// add the pakdata file to the filesystem
		PAKFILE * pakfile = g_Pakfile.m_Pakfiles.AddPakfile(pakdata, (UINT64)sizePakdata, pakindex, (UINT64)sizePakIndex, pakType, &tVersionBuild);
		ASSERT_BREAK(pakfile);
		idPakfile = pakfile->m_idPakFile;
	}

	if (idPakfile == INVALID_ID)
	{
		if (pakdata != NULL)
		{
			AsyncFileClose(pakdata);
		}
		if (pakindex != NULL)
		{
			AsyncFileClose(pakindex);
		}
	}

	return idPakfile;
}

UINT32 PakFileCreateNewPakfile(
	LPCSTR strPath,
	LPCSTR strBaseName,
	PAK_ENUM pakType,
	LPCSTR strPakDatName,
	LPCSTR strPakIdxName)
{
	return PakFileCreateNewPakfile<CHAR>(strPath, strBaseName, pakType, strPakDatName, strPakIdxName);
}

UINT32 PakFileCreateNewPakfile(
	LPCWSTR strPath,
	LPCWSTR strBaseName,
	PAK_ENUM pakType,
	LPCWSTR strPakDatName,
	LPCWSTR strPakIdxName)
{
	return PakFileCreateNewPakfile<WCHAR>(strPath, strBaseName, pakType, strPakDatName, strPakIdxName);
}

//----------------------------------------------------------------------------
// open the specific pakdata/pakindex pair, return pakfileid
//----------------------------------------------------------------------------
template <class CHARTYPE>
unsigned int sPakFileOpenPakFile(
	const CHARTYPE * pakdataFilename,
	const CHARTYPE * pakindexFilename,
	unsigned int index,
	PAK_ENUM ePakEnum,
	BOOL bReadOnly)
{
	ASYNC_FILE * pakdata = NULL;
	unsigned int idPakfile = INVALID_ID;
	BOOL result = FALSE;
	UNREFERENCED_PARAMETER(index);
	ONCE
	{
		// open pakdata
		pakdata = AsyncFileOpen(pakdataFilename, FF_READ | (bReadOnly ? FF_SHARE_READ : (FF_SHARE_READ | FF_WRITE)) | FF_RANDOM_ACCESS);
		ASSERT_BREAK(pakdata);

		UINT64 pakdataSize = AsyncFileGetSize(pakdata);

		// read and validate header
		PAK_DATA_HEADER hdr;
		structclear(hdr);

		BYTE buffer[4096];
		BYTE_BUF bbuf(buffer, 4096);

		unsigned int pakdataHdrSize = sizeof(PAK_DATA_HEADER);
		ASYNC_DATA asyncData(pakdata, buffer, 0, pakdataHdrSize);
		ASSERT_BREAK(pakdataHdrSize == AsyncFileReadNow(asyncData));
		ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.dwMagic, sizeof(hdr.m_Hdr.dwMagic)));
		ASSERT_BREAK(hdr.m_Hdr.dwMagic == PAK_DATA_MAGIC);
		ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.dwVersion, sizeof(hdr.m_Hdr.dwVersion)));

		// Clean up the code once we make a non backwards-compatible build
		if (hdr.m_Hdr.dwVersion != PAK_DATA_VERSION) 
		{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
			break;
#else
			if (hdr.m_Hdr.dwVersion != 3) 
			{
				break;
			}
#endif
		}

		if (hdr.m_Hdr.dwVersion >= 4) 
		{
			ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.tVersionBuild.dwVerTitle,		sizeof(hdr.m_Hdr.tVersionBuild.dwVerTitle)));
			ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.tVersionBuild.dwVerPUB,		sizeof(hdr.m_Hdr.tVersionBuild.dwVerPUB)));
			ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.tVersionBuild.dwVerRC,			sizeof(hdr.m_Hdr.tVersionBuild.dwVerRC)));
			ASSERT_BREAK(bbuf.PopUInt(&hdr.m_Hdr.tVersionBuild.dwVerML,			sizeof(hdr.m_Hdr.tVersionBuild.dwVerML)));
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)		// Only checks the version in release
			ASSERT_BREAK(hdr.m_Hdr.tVersionBuild.dwVerTitle ==					TITLE_VERSION);
			ASSERT_BREAK(hdr.m_Hdr.tVersionBuild.dwVerPUB ==					PRODUCTION_BRANCH_VERSION);
			ASSERT_BREAK(hdr.m_Hdr.tVersionBuild.dwVerRC ==						RC_BRANCH_VERSION);
			ASSERT_BREAK(hdr.m_Hdr.tVersionBuild.dwVerML ==						MAIN_LINE_VERSION);
#endif
		}

		// get a pakfile structure
		PAKFILE * pakfile = g_Pakfile.m_Pakfiles.AddPakfile(pakdata, pakdataSize, NULL, 0, ePakEnum, &hdr.m_Hdr.tVersionBuild);
		ASSERT_BREAK(pakfile);

//		pakfile->m_indexNumber = index;

		// read the pakindex
		if (!sPakFileOpenPakIndex(pakindexFilename, pakfile, bReadOnly))
		{
			break;
		}

		idPakfile = pakfile->m_idPakFile;
		result = TRUE;
	}

	if (!result)
	{
		if (pakdata)
		{
			AsyncFileClose(pakdata);
		}
		idPakfile = INVALID_ID;
	}

	return idPakfile;
}

unsigned int sPakFileOpenPakFile(
	const CHAR * pakdataFilename,
	const CHAR * pakindexFilename,
	unsigned int index,
	PAK_ENUM ePakEnum,
	BOOL bReadOnly)
{
	return sPakFileOpenPakFile<CHAR>(pakdataFilename, pakindexFilename, index, ePakEnum, bReadOnly);
}

unsigned int sPakFileOpenPakFile(
	const WCHAR * pakdataFilename,
	const WCHAR * pakindexFilename,
	unsigned int index,
	PAK_ENUM ePakEnum,
	BOOL bReadOnly)
{
	return sPakFileOpenPakFile<WCHAR>(pakdataFilename, pakindexFilename, index, ePakEnum, bReadOnly);
}


//----------------------------------------------------------------------------
// validate that the filename has a valid base name and get the index
//----------------------------------------------------------------------------
static BOOL sPakFileValidatePakFileName(
	const TCHAR * fullfilename,
	const TCHAR * filepath,
	const TCHAR * basename,
	unsigned int * index)
{
	static unsigned int ext_len = PStrLen(PAK_DATA_EXT);

	ASSERT_RETFALSE(fullfilename != NULL);
	ASSERT_RETFALSE(filepath != NULL);
	ASSERT_RETFALSE(basename != NULL);
	ASSERT_RETFALSE(index != NULL);

	unsigned int len = PStrLen(fullfilename, MAX_PATH);
	if (len < ext_len + 1)
	{
		return FALSE;
	}
	// validate extension
	if (PStrICmp(fullfilename + len - ext_len, PAK_DATA_EXT) != 0)
	{
		return FALSE;
	}
	if (fullfilename[len - ext_len - 1] != _T('.'))
	{
		return FALSE;
	}

	unsigned int offset = 0;
	if (PStrICmp(fullfilename, filepath, PStrLen(filepath)) != 0) {
		return FALSE;
	}
	offset += PStrLen(filepath);

	if (PStrICmp(fullfilename+offset, basename, PStrLen(basename)) != 0) {
		return FALSE;
	}
	offset += PStrLen(basename);

	for (; offset < len - ext_len; offset++) {
		if (fullfilename[offset] == '_') continue;
		if (fullfilename[offset] == '.') continue;
		if (fullfilename[offset] >= '0' &&
			fullfilename[offset] <= '9') continue;
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// open all pakdata/pakindex pairs for which the base name matches
// NOTE: we can open them in whatever order they're returned by the file
// system so long as we parse the index number and have higher index numbers
// supersede lower index numbers
//----------------------------------------------------------------------------
static UINT32 sPakFileOpenPakFile(
	LPCTSTR strPath,
	PAK_ENUM pakType,
	LPCTSTR strBaseName,
	BOOL& bError,
	BOOL bReadOnly = FALSE)
{
	static unsigned int ext_len = PStrLen(PAK_DATA_EXT);

	bError = FALSE;
	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) 
	{
		return INVALID_ID;
	}

	UINT32 GenPakId = INVALID_ID;

	TCHAR pakdataSearchname[MAX_PATH];
	PStrPrintf(pakdataSearchname, arrsize(pakdataSearchname), "%s%s*.%s", strPath, strBaseName, PAK_DATA_EXT);

	{
		WIN32_FIND_DATA findData;
		FindFileHandle handle = FindFirstFile(pakdataSearchname, &findData);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return INVALID_ID;
		}

		do
		{
			ONCE
			{
				unsigned int index = 0;

				TCHAR pakdataFilename[MAX_PATH];
				PStrPrintf(pakdataFilename, arrsize(pakdataFilename), "%s%s", strPath, findData.cFileName);

				// validate that the pakdata has a valid filename
				if (!sPakFileValidatePakFileName(pakdataFilename, strPath, strBaseName, &index))
				{
					break;
				}

				// validate that the pakindex for this pakdata exists
				TCHAR pakindexFilename[MAX_PATH];
				PStrCopy(pakindexFilename, pakdataFilename, MAX_PATH);
				unsigned int len = PStrLen(pakindexFilename, MAX_PATH);
				ASSERT_BREAK(len > ext_len);
				PStrCopy(pakindexFilename + len - ext_len, PAK_INDEX_EXT, MAX_PATH - len + ext_len);
				if (!FileExists(pakindexFilename))
				{
					ASSERTV_BREAK(0, "pak index [%s] doesn't exist for file [%s]", pakindexFilename, pakdataFilename);
					// should check if a temp version of the pakindex exists maybe?
					break;
				}

				// open the files
				GenPakId = sPakFileOpenPakFile(pakdataFilename, pakindexFilename, index, pakType, bReadOnly);
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)		// Only checks the version in release
				ASSERT_DO(GenPakId != INVALID_ID) 
				{
					bError = TRUE;
					return INVALID_ID;
				}
#endif
				if (GenPakId == INVALID_ID)
				{
					break;
				}
			}
		} while(FindNextFile(handle, &findData));
	}
	return GenPakId;
}


//----------------------------------------------------------------------------
// return the genpakid for a given pakfile enum
//----------------------------------------------------------------------------
UINT32 PakFileGetGenPakfileId(
	PAK_ENUM pakfile)
{
	ASSERT_RETVAL(pakfile < NUM_PAK_ENUM, INVALID_ID);
	ASSERT_RETVAL(g_PakFileDesc[pakfile].m_Index == pakfile, INVALID_ID);
	return g_PakFileDesc[pakfile].m_GenPakId;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
#include "exe_file_list.h"

static void sPakFileInitLoadXML(
	void)
{
	UINT32 i;

	if (!AppGetAllowFillLoadXML()) 
	{
		return;
	}

	PStrPrintf(xml_filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, STARTUP_LOAD_XML);
	g_Pakfile.m_csStartupLoad.Init();
	g_Pakfile.m_xmlStartupLoad.Init();
	//ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.Load(STARTUP_LOAD_XML));
	g_Pakfile.m_xmlStartupLoad.Load(xml_filename);

	char sFileListLabel[MAX_PATH];
	if (gtAppCommon.m_sGenericName != NULL) {
		PStrPrintf(sFileListLabel, MAX_PATH, "%sFileList", gtAppCommon.m_sGenericName);
	} else {
		PStrCopy(sFileListLabel, "FileList", MAX_PATH);
	}
	if (g_Pakfile.m_xmlStartupLoad.FindElem("FileList") == FALSE) 
	{
		ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.AddElem("FileList"));
	}
	if (g_Pakfile.m_xmlStartupLoad.FindChildElem(sFileListLabel)) 
	{
		ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.RemoveChildElem());
	}
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.AddChildElem(sFileListLabel));
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.IntoElem());
	if (g_Pakfile.m_xmlStartupLoad.FindChildElem("Executable")) 
	{
		ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.RemoveChildElem());
	}
	if (g_Pakfile.m_xmlStartupLoad.FindChildElem("Executable64")) 
	{
		ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.RemoveChildElem());
	}
	if (g_Pakfile.m_xmlStartupLoad.FindChildElem("DataFileList")) 
	{
		ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.RemoveChildElem());
	}

	LPCTSTR* listReleaseFiles = NULL;
	LPCTSTR* listReleaseFiles64 = NULL;
	if (AppIsHellgate()) {
		listReleaseFiles = listReleaseFiles_Hellgate;
		listReleaseFiles64 = listReleaseFiles64_Hellgate;
	}
	if (AppIsTugboat()) {
		listReleaseFiles = listReleaseFiles_Mythos;
		listReleaseFiles64 = listReleaseFiles64_Mythos;
	}
	ASSERT_RETURN(listReleaseFiles != NULL);
	ASSERT_RETURN(listReleaseFiles64 != NULL);

	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.AddChildElem("Executable"));
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.IntoElem());

	for (i = 0; listReleaseFiles[i]; i++) 
	{
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.AddChildElem(_T("File")));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.IntoElem());
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.SetData(listReleaseFiles[i]));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	}
	if (AppIsHellgate()) for (i = 0; listReleaseFiles_Hellgate_LauncherFiles[i]; i++) {
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.AddChildElem(_T("LauncherFile")));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.IntoElem());
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.SetData(listReleaseFiles_Hellgate_LauncherFiles[i]));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	}
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.OutOfElem());

	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.AddChildElem("Executable64"));
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.IntoElem());
	for (i = 0; listReleaseFiles64[i]; i++) 
	{
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.AddChildElem(_T("File")));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.IntoElem());
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.SetData(listReleaseFiles64[i]));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	}
	if (AppIsHellgate()) for (i = 0; listReleaseFiles_Hellgate_LauncherFiles[i]; i++) {
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.AddChildElem(_T("LauncherFile")));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.IntoElem());
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.SetData(listReleaseFiles_Hellgate_LauncherFiles[i]));
		ASSERT_CONTINUE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	}
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.OutOfElem());

	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.Save(xml_filename));

	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.AddChildElem("DataFileList"));
	ASSERT_RETURN(g_Pakfile.m_xmlStartupLoad.IntoElem());
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UINT32 sPakFileHashStringHash(const LPTSTR& str)
{
	return CRC(0, (BYTE*)str, PStrLen(str)*sizeof(TCHAR));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileHashStringEual(const LPTSTR& str, const PFILE_HASH_INFO &pFI)
{
	return (PStrCmp(str, pFI->filename) == 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileHashDestroy(struct MEMORYPOOL* pool, PFILE_HASH_INFO&  fit)
{
	FREE(pool, fit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sPakFileSetupSpecificFile()
{
	g_FileHashTable.Init(NULL, 256, sPakFileHashStringHash, sPakFileHashStringEual, NULL);
#if ISVERSION(CLIENT_ONLY_VERSION)

	CMarkup xmlConfig;
	xmlConfig.Init();

	PStrPrintf(xml_filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, STARTUP_LOAD_XML);
	if (!xmlConfig.Load(xml_filename)) {
		return TRUE;
	}
	ASSERT_RETTRUE(xmlConfig.FindElem("FileList"));
	if (AppIsHellgate()) {
		ASSERT_RETTRUE(xmlConfig.FindChildElem("HellgateFileList"));
	} else if (AppIsTugboat()) {
		ASSERT_RETTRUE(xmlConfig.FindChildElem("MythosFileList"));
	}
	ASSERT_RETTRUE(xmlConfig.IntoElem());
	ASSERT_RETTRUE(xmlConfig.FindChildElem("DataFileList"));
	ASSERT_RETTRUE(xmlConfig.IntoElem());

	while (xmlConfig.FindChildElem("FileInPak")) {
		TCHAR fname[DEFAULT_FILE_WITH_PATH_SIZE];
		TCHAR strHash[2*SHA1HashSize+1];
		PStrTDef(strFilePath, MAX_PATH);
		PStrTDef(strFileName, MAX_PATH);

		ASSERT_CONTINUE(xmlConfig.GetChildData(fname, DEFAULT_FILE_WITH_PATH_SIZE));
		ASSERT_CONTINUE(xmlConfig.IntoElem());
		UINT32 len = PStrLen(fname);
		while (len > 0 && (fname[len-1] == _T('\r') || fname[len-1] == _T('\n'))) {
			fname[len-1] = _T('\0');
			len--;
		}
		ASSERT_GOTO(xmlConfig.FindChildElem("Hash"), _done);
		ASSERT_GOTO(xmlConfig.GetChildData(strHash, 2*SHA1HashSize+1), _done);
		ASSERT_GOTO(PStrLen(strHash) == 2*SHA1HashSize, _done);

		PFILE_HASH_INFO pFileHashInfo = (PFILE_HASH_INFO)MALLOC(NULL, sizeof(FILE_HASH_INFO));
		ASSERT_GOTO(pFileHashInfo != NULL, _done);
		PStrCopy(pFileHashInfo->filename, fname, DEFAULT_FILE_WITH_PATH_SIZE);

		// Seriously, is there an easier way to convert hex strings that I don't know of?
		for (UINT32 i = 0; i < SHA1HashSize; i++) {
			TCHAR tmp[3];
			int iTmp = 0;
			PStrCopy(tmp, 3, strHash+2*i, 2);
			sscanf_s(tmp, "%x", &iTmp);
			pFileHashInfo->hash.m_Hash[i] = (UINT8)iTmp;
		}

		g_FileHashTable.AddItem(fname, pFileHashInfo);
_done:
		xmlConfig.OutOfElem();
	}
	xmlConfig.Free();
#endif
	return TRUE;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileInitForApp(
	void)
{
	CSAutoLock lock(&g_csPakfile);
	if (g_iRefCount > 0) 
	{
		g_iRefCount++;
		return TRUE;
	}

	ASSERT_RETFALSE(!g_Pakfile.m_bInitialized);
	structclear(g_Pakfile);

	g_Pakfile.m_bInitialized = TRUE;
	g_Pakfile.m_Pool = NULL;
	// CHB 2007.01.11
#if 1
	OS_PATH_CHAR buf[arrsize(g_Pakfile.m_strRootDirectory)];
	unsigned nLen = _countof(buf);
	ASSERT_RETFALSE(AsyncFileGetRootDirectory(buf, nLen));
	PStrCvt(g_Pakfile.m_strRootDirectory, buf, _countof(g_Pakfile.m_strRootDirectory));
	g_Pakfile.m_lenRootDirectory = PStrLen(g_Pakfile.m_strRootDirectory);
#else
	g_Pakfile.m_lenRootDirectory = arrsize(g_Pakfile.m_strRootDirectory);
	ASSERT_RETFALSE(AsyncFileGetRootDirectory(g_Pakfile.m_strRootDirectory, g_Pakfile.m_lenRootDirectory));
#endif

	g_Pakfile.m_Pakfiles.Init(g_Pakfile.m_Pool);
	g_Pakfile.m_FileIndex.Init(g_Pakfile.m_Pool);
	g_Pakfile.m_FileHash.Init(g_Pakfile.m_Pool);

	ArrayInit(g_PakFileDesc, NULL, 10);
	if (AppIsHellgate())
	{
		for (UINT32 i = 0; i < arrsize(g_PakFileDescHellgate); i++) 
		{
			ArrayAddItem(g_PakFileDesc, g_PakFileDescHellgate[i]);
		}
	}
	else if (AppIsTugboat())
	{
		for (UINT32 i = 0; i < arrsize(g_PakFileDescTugboat); i++) 
		{
			ArrayAddItem(g_PakFileDesc, g_PakFileDescTugboat[i]);
		}
	}

	if(AppIsHammer())
	{
		// CML 2008.02.14 - Added initial refcount.  We were never addreffing in Hammer, resulting in a series of assertions on exit.
		g_iRefCount++;
		return TRUE;
	}

	sPakFileSetupSpecificFile();

	for (unsigned int ii = 0; ii < g_PakFileDesc.Count(); ++ii)
	{
		PAKFILE_DESC & desc = g_PakFileDesc[ii];
		ASSERT_RETFALSE(desc.m_Index == (PAK_ENUM)ii);
		if(desc.m_Create)
		{
			BOOL bError = FALSE;
			if (PStrLen(desc.m_BaseName) != 0)
			{
#if ISVERSION(DEVELOPMENT)
				BOOL bReadOnly = (AppCommonGetDirectLoad() == FALSE);
#else
				BOOL bReadOnly = TRUE;
#endif

				UINT32 idPakfile = sPakFileOpenPakFile(FILE_PATH_PAK_FILES, desc.m_Index, desc.m_BaseName, bError, bReadOnly);

#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
				// retail server should just return failure
				ASSERTX_RETFALSE(bError == FALSE, "Pakfile has bad version");
				UNREFERENCED_PARAMETER(idPakfile);
#else
				if (idPakfile == INVALID_ID) {
					// need to create a new pakfile
					idPakfile = PakFileCreateNewPakfile(FILE_PATH_PAK_FILES, desc.m_BaseName, desc.m_Index);
				}
				ASSERT_RETFALSE(idPakfile != INVALID_ID);
				desc.m_GenPakId = idPakfile;
#endif
			}
#if ISVERSION(CLIENT_ONLY_VERSION) || ISVERSION(DEVELOPMENT)
			if (AppGetAllowPatching() || ISVERSION(CLIENT_ONLY_VERSION)) // this is intentional. do not remove.
			{
				// Pakfiles for MP patches
				if (PStrLen(desc.m_BaseNameMP) != 0)
				{
					UINT32 idPakfile = sPakFileOpenPakFile(FILE_PATH_PAK_FILES, desc.m_Index, desc.m_BaseNameMP, bError);//, desc.m_MaxIndex);
					if (idPakfile != INVALID_ID) 
					{
//						desc.m_GenPakId = idPakfile;
					}
				}
				// Pakfiles for the downloaded files
				if (PStrLen(desc.m_BaseNameX) != 0)
				{
					UINT32 idPakfile = sPakFileOpenPakFile(FILE_PATH_PAK_FILES, desc.m_Index, desc.m_BaseNameX, bError);//, desc.m_MaxIndex);
					if (idPakfile == INVALID_ID) {
						// need to create a new pakfile
						idPakfile = PakFileCreateNewPakfile(FILE_PATH_PAK_FILES, desc.m_BaseNameX, desc.m_Index);
					}
					ASSERT_RETFALSE(idPakfile != INVALID_ID);
					desc.m_GenPakId = idPakfile;
				}
			}
#endif
#if !ISVERSION(SERVER_VERSION) && !ISVERSION(CLIENT_ONLY_VERSION)
			// Pakfiles for SP patches
			if (AppCommonSingleplayerOnly() || AppGetAllowPatching())
			{
				if (PStrLen(desc.m_BaseNameSP) != 0)
				{
					UINT32 idPakfile = sPakFileOpenPakFile(FILE_PATH_PAK_FILES, desc.m_Index, desc.m_BaseNameSP, bError);//, desc.m_MaxIndex);
					if (idPakfile != INVALID_ID) {
						desc.m_GenPakId = idPakfile;
					}
				}
			}
#endif
		}
	}
	g_FileHashTable.Destroy(sPakFileHashDestroy);

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	sPakFileInitLoadXML();
#endif

#if ISVERSION(DEVELOPMENT)
	if (gbPakfileTrace)
	{
		PakFileTrace();
		gbPakfileTrace = FALSE;

		// CML -- just exit now
		exit( 0 );
	}
#endif

	g_iRefCount++;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileInitForAppNoPakfile(
	void)
{
	CSAutoLock lock(&g_csPakfile);
	if (g_iRefCount > 0) 
	{
		g_iRefCount++;
		return TRUE;
	}

	ASSERT_RETFALSE(!g_Pakfile.m_bInitialized);
	structclear(g_Pakfile);

	g_Pakfile.m_bInitialized = TRUE;
	g_Pakfile.m_Pool = NULL;
	// CHB 2007.01.11
#if 1
	OS_PATH_CHAR buf[arrsize(g_Pakfile.m_strRootDirectory)];
	unsigned nLen = _countof(buf);
	ASSERT_RETFALSE(AsyncFileGetRootDirectory(buf, nLen));
	PStrCvt(g_Pakfile.m_strRootDirectory, buf, _countof(g_Pakfile.m_strRootDirectory));
	g_Pakfile.m_lenRootDirectory = PStrLen(g_Pakfile.m_strRootDirectory);
#else
	g_Pakfile.m_lenRootDirectory = arrsize(g_Pakfile.m_strRootDirectory);
	ASSERT_RETFALSE(AsyncFileGetRootDirectory(g_Pakfile.m_strRootDirectory, g_Pakfile.m_lenRootDirectory));
#endif

	g_Pakfile.m_Pakfiles.Init(g_Pakfile.m_Pool);
	g_Pakfile.m_FileIndex.Init(g_Pakfile.m_Pool);
	g_Pakfile.m_FileHash.Init(g_Pakfile.m_Pool);

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	sPakFileInitLoadXML();
#endif

	ArrayInit(g_PakFileDesc, NULL, 4);
	for (UINT32 i = 0; i < NUM_PAK_ENUM; i++) {
		PAKFILE_DESC desc;
		structclear(desc);
		desc.m_Index = (PAK_ENUM)i;
		desc.m_AlternativeIndex = (PAK_ENUM)i;

		ArrayAddItem(g_PakFileDesc, desc);
	}

	g_iRefCount++;
	return TRUE;
}


// ---------------------------------------------------------------------------
// returns either filename or buf, doesn't necessarily fill buf if filename is ok
// ---------------------------------------------------------------------------
/*
static const TCHAR * sPakFileGetFullPathName(
	const TCHAR * filename,
	TCHAR * buf,
	unsigned int buflen)
{
	if (PStrICmp(g_Pakfile.m_strRootDirectory, filename, g_Pakfile.m_lenRootDirectory) == 0)
	{
		return filename;
	}
	PStrCopy(buf, g_Pakfile.m_strRootDirectory, buflen);
	PStrCopy(buf + g_Pakfile.m_lenRootDirectory, filename, buflen - g_Pakfile.m_lenRootDirectory);
	return buf;
}
*/

// ---------------------------------------------------------------------------
// returns either filename or buf, doesn't necessarily fill buf if filename is ok
// ---------------------------------------------------------------------------
static const WCHAR * sPakFileGetFullPathName(
	const WCHAR * filename,
	WCHAR * buf,
	unsigned int buflen)
{
	WCHAR rootDir[MAX_PATH];
	PStrCvt(rootDir, g_Pakfile.m_strRootDirectory, MAX_PATH);
	if (PStrICmp(rootDir, filename, g_Pakfile.m_lenRootDirectory) == 0)
	{
		return filename;
	}
	PStrCopy(buf, rootDir, buflen);
	PStrCopy(buf + g_Pakfile.m_lenRootDirectory, filename, buflen - g_Pakfile.m_lenRootDirectory);
	return buf;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 sPakFileLoadGetLastModifiedTime(
	const TCHAR * filename)
{
	ASSERT_RETZERO(filename && filename[0]);

	WCHAR buf[MAX_PATH];
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	const WCHAR * fullfilename = sPakFileGetFullPathName(wfilename, buf, arrsize(buf));

	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExW(fullfilename, GetFileExInfoStandard, &data))
	{
		return 0;
	}
	
	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(data.ftLastWriteTime));
	memcpy(&temp, &data.ftLastWriteTime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 sPakFileLoadGetLastModifiedTime(
	const WCHAR * filename)
{
	ASSERT_RETZERO(filename && filename[0]);

	WCHAR buf[MAX_PATH];
	const WCHAR * fullfilename = sPakFileGetFullPathName(filename, buf, arrsize(buf));

	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExW(fullfilename, GetFileExInfoStandard, &data))
	{
		return 0;
	}

	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(data.ftLastWriteTime));
	memcpy(&temp, &data.ftLastWriteTime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileFileCheckWarning(
	PAKFILE_LOAD_SPEC & spec)
{
#if ISVERSION(DEVELOPMENT)
	if (spec.flags & PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING)
	{
		char szMessage[ 512 ];
		PStrPrintf( szMessage, 512, "Missing Critical File: '%s'", spec.fixedfilename );
		ASSERTX_RETURN( 0, szMessage );
		return;
	}
	if (spec.flags & PAKFILE_LOAD_WARN_IF_MISSING)
	{
		const GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();
		if ( globalData && (globalData->dwFlags & GLOBAL_FLAG_DATA_WARNINGS) 
			&& globalData->nDebugOutputLevel <= OP_HIGH )
		{
			char str[MAX_PATH + 256];
			PStrPrintf(str, arrsize(str), "WARNING - Missing File: '%s'", spec.fixedfilename);
			ASSERTX_RETURN( 0, str );
			return;
		}
	}
#else
	// CHB 2006.12.22
	REF(spec);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileFileNotFound(
	PAKFILE_LOAD_SPEC & spec)
{
	if ( spec.flags & PAKFILE_LOAD_OPTIONAL_FILE )
		return;
#ifdef TRACE_FILE_NOT_FOUND
	TraceError("FILE NOT FOUND: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
#endif
	sPakFileFileCheckWarning(spec);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileFileError(
	PAKFILE_LOAD_SPEC & spec)
{
#ifdef TRACE_FILE_NOT_FOUND
	TraceError("FILE IO ERROR: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
#endif
	sPakFileFileCheckWarning(spec);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PAKFILE_LOAD_SPEC * sPakFileLoadCopySpec(
	PAKFILE_LOAD_SPEC & spec)
{
	unsigned int lenFilename = 0;
	if (spec.filename)
	{
		lenFilename = PStrLen(spec.filename, MAX_PATH) + 1;
	}
	PAKFILE_LOAD_SPEC * data = (PAKFILE_LOAD_SPEC *)MALLOC(spec.pool, sizeof(PAKFILE_LOAD_SPEC) + (lenFilename * sizeof(TCHAR)));
	ASSERT_RETNULL(data);
	*data = spec;
	// set PStrT to point to correct buffer
	data->filename = (char *)(data + 1);
	PStrCopy((TCHAR *)data->filename, (TCHAR *)spec.filename, lenFilename);
	data->strFilePath.str = data->bufFilePath;
	data->strFileName.str = data->bufFileName;
	return data;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PAKFILE_LOAD_SPEC * PakFileLoadCopySpec(
	PAKFILE_LOAD_SPEC & spec)
{
	return sPakFileLoadCopySpec(spec);
}


//----------------------------------------------------------------------------
// debug function which compares a filenode to the actual data file
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && PAKFILE_COMPARE_TO_DATA
static BOOL sPakFileLoadCompareFilenodeToData(
	FILE_INDEX_NODE * fileindex)
{	
	if (fileindex->size == 0)
	{
		ASSERT_RETFALSE(!TESTBIT(fileindex->flags, PAKFLAG_VALID_NODE));
		return TRUE;
	}
	ASSERT_RETFALSE(!fileindex->m_FilePath.IsEmpty());
	ASSERT_RETFALSE(!fileindex->m_FileName.IsEmpty());

	// don't compare excel files -- TODO: we could optimize out the string compare if we added filepath codes
	if (filenode->m_FilePath == FILE_PATH_EXCEL ||
		filenode->m_FilePath == FILE_PATH_EXCEL_COMMON)
	{
		return TRUE;
	}

	TCHAR filename[MAX_PATH];
	ASSERT_RETFALSE(sCombineFilename(filename, MAX_PATH, filenode->m_FilePath, filenode->m_FileName));

	trace(_T("Comparing pak to data: '%s'\n"), filename);

	BYTE * filedata = NULL;
	BYTE * pakfiledata = NULL;

	BOOL result = FALSE;

	ONCE
	{
		DWORD filesize;
		filedata = (BYTE *)FileLoad(NULL, filename, &filesize);

		DWORD pakfilesize;
		pakfiledata = (BYTE *)FileIOThreadOnly_PakFileLoad(fs, NULL, filename, &pakfilesize, NULL, 0, 0, NULL, NULL, TRUE);

		ASSERT_BREAK(filedata != NULL && pakfiledata != NULL && filesize == pakfilesize);

		ASSERT_BREAK(memcmp(filedata, pakfiledata, filesize) == 0);

		result = TRUE;
	}

	FREE(NULL, filedata);
	FREE(NULL, pakfiledata);

	return result;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInitFileHeader(
	FILE_HEADER * hdr)
{
	ASSERT_RETURN(hdr);
	hdr->dwMagicNumber = NO_MAGIC;
	hdr->nVersion = NO_VERSION;
}


//----------------------------------------------------------------------------
// write some data to the pakindex
//----------------------------------------------------------------------------
static BOOL sPakFileIndexWrite(
	PAKFILE * pakfile,
	PAKFILE_LOAD_SPEC * spec,
	const void * buffer,
	DWORD buflen)
{
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(spec);
	ASSERT_RETFALSE(buffer && buflen > 0);

	ASSERT_RETFALSE(pakfile->m_filePakIndex != NULL);

	UINT64 offset = 0;
	pakfile->m_csPakIndex.Enter();
	{
		offset = pakfile->m_sizePakIndex;
		pakfile->m_sizePakIndex += buflen;
	}
	pakfile->m_csPakIndex.Leave();

	ASYNC_DATA asyncData(pakfile->m_filePakIndex, (void *)buffer, offset, buflen, ADF_FREEBUFFER);
	if (spec->flags & PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE) {
		ASSERT_RETFALSE(AsyncFileWriteNow(asyncData));
	} else {
		ASSERT_RETFALSE(AsyncFileWrite(asyncData, ASYNC_PRIORITY_MAX, AppCommonGetMainThreadAsyncId()));
	}

	// update the file index (theoretical race condition in that we will do this before we finish writing to disk, but not really probable)
	g_Pakfile.m_FileHash.m_CS.WriteLock();
	{
		FILE_INDEX_NODE * fileindex = sPakFileGetFileIndexNode(spec->strFilePath, spec->strFileName, spec->key);
		ASSERT(fileindex);
		if (fileindex)
		{
			CLEARBIT(fileindex->m_flags, PAKFLAG_ADDING);
		}
	}
	g_Pakfile.m_FileHash.m_CS.EndWrite();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sPakFileAddDataToPakCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec)
	ASSERT_RETINVALIDARG(spec->pakfile);

#if !ISVERSION(SERVER_VERSION)
#if ISVERSION(DEVELOPMENT)
	// If we're running a fillpak client, send the file to the server
	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) 
	{
		FillPakSendFileToServer(
			spec->bufFilePath,
			spec->bufFileName,
			spec->bytesread,
			spec->size_compressed,
			spec->pakEnum,
			spec->buffer,
			spec->hash.m_Hash,
			&spec->tmpHeader);
	}
#endif
#endif

	// free the buffer
	if ((spec->flags & PAKFILE_LOAD_FREE_BUFFER) && spec->buffer)
	{
		FREE(spec->pool, spec->buffer);
		data.m_Buffer = spec->buffer = NULL;
	}

	// update the file index
	FILE_INDEX_NODE * fileindex = NULL;

	g_Pakfile.m_FileHash.m_CS.ReadLock();
	{
		fileindex = sPakFileGetFileIndexNode(spec->strFilePath, spec->strFileName, spec->key);
	}
	g_Pakfile.m_FileHash.m_CS.EndRead();
	ASSERT_RETFAIL(fileindex);

	fileindex->m_offset = spec->fileoffset;
	fileindex->m_fileSize = spec->bytesread;
	fileindex->m_fileSizeCompressed = spec->size_compressed;

	fileindex->m_idPakFile = spec->pakfile->m_idPakFile;
	fileindex->m_iPakType = spec->pakfile->m_ePakEnum;
	fileindex->m_GenTime = spec->gentime;
	CLEARBIT(fileindex->m_flags, PAKFLAG_NOT_CURRENT);
	SETBIT(fileindex->m_flags, PAKFLAG_IS_CURRENT);
	fileindex->m_Hash = spec->hash;
	memcpy(&fileindex->m_FileHeader, &spec->tmpHeader, sizeof(FILE_HEADER));	

	BYTE * buffer = (BYTE *)MALLOC(g_Pakfile.m_Pool, PAK_FILE_INDEX_BUFSIZE);
	BYTE_BUF_OBFUSCATED bbuf(buffer, PAK_FILE_INDEX_BUFSIZE, PAK_INDEX_OBS_MULT, PAK_INDEX_OBS_ADD + (DWORD)fileindex->m_offset);
	ASSERT_RETFALSE(bbuf.PushUIntNoObs((DWORD)PAK_INDEX_OBS_ADD + (DWORD)fileindex->m_offset, sizeof(DWORD)));

	sPakFileWriteUncosolidatedIndexToBuffer(bbuf, spec->pakfile, fileindex);
	sPakFileIndexWrite(spec->pakfile, spec, buffer, bbuf.GetLen());

#if ISVERSION(DEVELOPMENT) && PAKFILE_COMPARE_TO_DATA
	sPakFileLoadCompareFilenodeToData(fileindex);
#endif


	PRESULT pr = S_OK;
	if(spec->fpFileIOThreadWriteCompleteCallback)
	{
		V_SAVE( pr, spec->fpFileIOThreadWriteCompleteCallback(data) );
	}

	if(spec->fpLoadingThreadWriteCompleteCallback)
	{
		PAKFILE_LOAD_SPEC * data_copy = sPakFileLoadCopySpec(*spec);
		ASSERT_RETFALSE(data_copy);

		ASYNC_DATA asyncData;
		ASSERT_RETFALSE(MemoryCopy(&asyncData, sizeof(ASYNC_DATA), &data, sizeof(ASYNC_DATA)));
		asyncData.m_Buffer = data_copy->buffer;
		asyncData.m_UserData = data_copy;
		asyncData.m_Pool = data_copy->pool;
		asyncData.m_fpLoadingThreadCallback = spec->fpLoadingThreadWriteCompleteCallback;
		AsyncFilePostRequestForCallback(&asyncData);
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileSpecFreeBuffers(
	PAKFILE_LOAD_SPEC & spec)
{
	if (TEST_MASK(spec.flags, PAKFILE_LOAD_FREE_BUFFER) && spec.buffer)
	{
		FREE(spec.pool, spec.buffer);
		spec.buffer = NULL;
	}
	if (TEST_MASK(spec.flags, PAKFILE_LOAD_FREE_CALLBACKDATA) && spec.callbackData)
	{
		FREE(spec.pool, spec.callbackData);
		spec.callbackData = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileSpecCleanup(
	PAKFILE_LOAD_SPEC & spec)
{
	PRESULT pr = S_OK;

	if (TEST_MASK(spec.flags, PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND))
	{
		{
			ASYNC_DATA asyncData(NULL, NULL, 0, 0, 0, spec.fpFileIOThreadCallback, spec.fpLoadingThreadCallback, &spec, NULL);

			if (asyncData.m_fpFileIOThreadCallback)
			{
				V_SAVE( pr, asyncData.m_fpFileIOThreadCallback(asyncData) );
			}
			if (asyncData.m_fpLoadingThreadCallback)
			{
				V_SAVE( pr, asyncData.m_fpLoadingThreadCallback(asyncData) );
			}
		}
		{
			ASYNC_DATA asyncData(NULL, NULL, 0, 0, 0, spec.fpFileIOThreadWriteCompleteCallback, spec.fpLoadingThreadWriteCompleteCallback, &spec, NULL);

			if (asyncData.m_fpFileIOThreadCallback)
			{
				V_SAVE( pr, asyncData.m_fpFileIOThreadCallback(asyncData) );
			}
			if (asyncData.m_fpLoadingThreadCallback)
			{
				V_SAVE( pr, asyncData.m_fpLoadingThreadCallback(asyncData) );
			}
		}
	}
	sPakFileSpecFreeBuffers(spec);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileAddFileToPak(
	PAKFILE_LOAD_SPEC & spec)
{
	// because our publishers are very sensitive to what we put in pak files to be 
	// distributed around the world, and they don't want any localized data
	// that they are not responsible for to be mixed in with their pak files, I'm 
	// going to restrict the putting of any file into the localized pakfile if we
	// are in just regular fillpak mode
	if (AppIsFillingPak() && spec.pakEnum == PAK_LOCALIZED && 
		AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT) == FALSE)
	{
		return TRUE;
	}

	if (spec.flags & PAKFILE_LOAD_NEVER_PUT_IN_PAK)
	{
		return TRUE;
	}
	
	ASSERT_RETFALSE(spec.gentime != 0);
	unsigned int size = spec.bytesread;
	if (spec.size_compressed != 0) 
	{
		ASSERT_RETFALSE(spec.buffer_compressed != NULL);
	} 
	else 
	{
		ASSERT_RETFALSE(spec.buffer && size > 0);
	}

	unsigned int idPakfile = PakFileGetGenPakfileId(spec.pakEnum);
	ASSERT_RETFALSE(g_Pakfile.m_Pakfiles.IsValidPakfileId(idPakfile));

	PAKFILE * pakfile = g_Pakfile.m_Pakfiles.GetById(idPakfile);
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(pakfile->m_filePakData != NULL);
	spec.pakfile = pakfile;
	
	if (!(spec.flags & PAKFILE_LOAD_FILE_ALREADY_COMPRESSED)) 
	{
		if (size >= sizeof(FILE_HEADER)) 
		{
			memcpy(&spec.tmpHeader, spec.buffer, sizeof(FILE_HEADER));	
		} 
		else
		{
			sInitFileHeader(&spec.tmpHeader);
		}
	}

	PAKFILE_LOAD_SPEC * data = sPakFileLoadCopySpec(spec);
	ASSERT_RETFALSE(data);

	if (spec.flags & PAKFILE_LOAD_GIANT_FILE)
	{
		data->buffer = MALLOCBIG(data->pool, size);
	}
	else
	{
		data->buffer = MALLOC(data->pool, size);
	}
	ASSERTX_RETFALSE(data->buffer != NULL, "What? No more memory?");

	SET_MASK(data->flags, PAKFILE_LOAD_FREE_BUFFER);	// free it a bit earlier than if we left it to asyncfile
	if (spec.flags & PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS) 
	{
		// copy the buffer, because we might free it later in this thread before the AsyncFileWrite is done
		ASSERT_RETFALSE(MemoryCopy(data->buffer, size, spec.buffer, size));
		spec.size_compressed = 0;
		data->size_compressed = 0;
	} 
	else if ((spec.flags & PAKFILE_LOAD_FILE_ALREADY_COMPRESSED) &&
		spec.size_compressed > 0 &&
		spec.buffer_compressed != NULL) 
	{
		data->size_compressed = spec.size_compressed;
		ASSERT_RETFALSE(MemoryCopy(data->buffer, size, spec.buffer_compressed, spec.size_compressed));
		size = spec.size_compressed;
	} 
	else 
	{
		// Compress the file
		spec.size_compressed = size;
		ASSERT_RETFALSE(ZLIB_DEFLATE((UINT8*)spec.buffer, size, (UINT8*)data->buffer, (UINT32*)&spec.size_compressed));

		// Don't compress if compressing produce a larger data
		if (spec.size_compressed >= size) 
		{
			ASSERT_RETFALSE(MemoryCopy(data->buffer, size, spec.buffer, size));
			spec.size_compressed = 0;
			data->size_compressed = 0;
		} 
		else 
		{
//			TracePersonal("compressing %s%s at %d%% full %d compressed %d\n", spec.strFilePath.str, spec.strFileName.str,
//				(100 * spec.size_compressed) / size, size, spec.size_compressed);
			data->size_compressed = spec.size_compressed;
			size = spec.size_compressed;
		}
	}

	// get write offset
	spec.fileoffset = 0;
	if (data->size_compressed > 0) 
	{
		size = data->size_compressed;
	}
	pakfile->m_csPakData.Enter();
	{
		UINT64 sizePakData = pakfile->m_sizePakData;
		
		// get next offset
		UINT64 fileoffset = sizePakData;
		fileoffset = ROUNDUP(fileoffset, (UINT64)ASSUMED_BYTES_PER_SECTOR);
		spec.fileoffset = fileoffset;
		pakfile->m_sizePakData = fileoffset + size;
	}
	pakfile->m_csPakData.Leave();
	data->fileoffset = spec.fileoffset;

	ASYNC_DATA asyncData(pakfile->m_filePakData, data->buffer, data->fileoffset, size, ADF_FREEDATA, sPakFileAddDataToPakCallback, NULL, data, data->pool);
	if (spec.flags & PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE) 
	{
		return AsyncFileWriteNow(asyncData);
	} 
	else 
	{
		return AsyncFileWrite(asyncData, ASYNC_PRIORITY_MAX, spec.threadId);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileLoadCheckAddToPak(
	PAKFILE_LOAD_SPEC & spec)
{
	if (AppTestDebugFlag(ADF_DONT_ADD_TO_PAKFILE))
	{
		return FALSE;
	}
	if (!AppCommonGetUpdatePakFile())
	{
		return FALSE;
	}
	if (!(spec.flags & PAKFILE_LOAD_ADD_TO_PAK))
	{
		return FALSE;
	}
	if (spec.flags & PAKFILE_LOAD_NEVER_PUT_IN_PAK)
	{
		return FALSE;
	}

	// If this is a build-server initiated build and it 
	// is distributed across multiple servers for the pak file 
	// generation process, only allow writing to the pak files
	// associated with the local system
	//	
	if(spec.pakEnum % max(1, gtAppCommon.m_nDistributedPakFileGenerationMax) != gtAppCommon.m_nDistributedPakFileGenerationIndex)
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPakFileLoadAddToPak(
	PAKFILE_LOAD_SPEC & spec)
{
	if (!sPakFileLoadCheckAddToPak(spec))
	{
		return;
	}

	if (!(spec.flags & PAKFILE_LOAD_FILE_ALREADY_HASHED)) 
	{
		ASSERT(SHA1Calculate((UINT8 *)spec.buffer, spec.bytesread, spec.hash.m_Hash));
	}

	// get the fileindex & see if it's an empty index or perhaps currently being added
	// add it to the file index
	FILE_INDEX_NODE * fileindex = NULL;
	BOOL bWriteLocked = FALSE;
	g_Pakfile.m_FileHash.m_CS.WriteLock();
	{
		fileindex = sPakFileGetFileIndexNode(spec.strFilePath, spec.strFileName, &spec.key);
		if (!fileindex)
		{
			fileindex = sPakFileAddToHash(spec.strFilePath, spec.strFileName, spec.key, TRUE);
			fileindex->m_idPakFile = PakFileGetGenPakfileId(spec.pakEnum);
			fileindex->m_iPakType = spec.pakEnum;
		}
		else if (TESTBIT(fileindex->m_flags, PAKFLAG_ADDING))
		{
			bWriteLocked = TRUE;
		}
		else if (TESTBIT(fileindex->m_flags, PAKFLAG_INDEX_ONLY))
		{
			CLEARBIT(fileindex->m_flags, PAKFLAG_INDEX_ONLY);
		}
		if (fileindex)
		{
			SETBIT(fileindex->m_flags, PAKFLAG_ADDING);
		}
	}
	g_Pakfile.m_FileHash.m_CS.EndWrite();
	if (!fileindex)
	{
		return;
	}

	ASSERT(sPakFileAddFileToPak(spec));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sPakFileLoadAddToPakCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);
	
	spec->bytesread = data.m_IOSize;
	sPakFileLoadAddToPak(*spec);

	if (spec->fpFileIOThreadCallback)	// call the original callback
	{
		data.m_fpFileIOThreadCallback = spec->fpFileIOThreadCallback;
	}

	if (!spec->fpLoadingThreadCallback)
	{
		sPakFileSpecFreeBuffers(*spec);
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sPakFileLoadingThreadCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	spec->bytesread = data.m_IOSize;

	PRESULT pr = S_OK;

#if ISVERSION(DEVELOPMENT)
	PAKLOAD_TRACE("PAKFILE LOAD %s-- PAKFILE [%s]  FILE [%s]  OFFSET [%I64d]  SIZE [%d]  TIME [%d msecs]", 
		((spec->flags & PAKFILE_LOAD_IMMEDIATE) ? "NOW " : ""),
		(data.m_File ? OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(data.m_File)) : "-"), (spec->filename ? spec->filename : "???"), 
		data.m_Offset, data.m_Bytes, GetTickCount() - spec->dbg_issue_tick);
#endif

	if (!TEST_MASK(data.m_flags, ADF_CANCEL) && spec->fpLoadingThreadCallback)
	{
		V_SAVE(pr, spec->fpLoadingThreadCallback(data));
	}

	sPakFileSpecFreeBuffers(*spec);

	return pr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileLoadSetupBuffer(
	PAKFILE_LOAD_SPEC & spec,
	DWORD size,
	BOOL bAddToPak = FALSE)
{
	if (bAddToPak)
	{
		if (spec.buffer && spec.size < size)
		{
			spec.offset = 0;
			spec.bytestoread = size;
			spec.size = 0;
			spec.buffer = 0;
			SET_MASK(spec.flags, PAKFILE_LOAD_FREE_BUFFER);
		}
	}
	else if (spec.offset >= size)
	{
		return FALSE;
	}
	if (spec.bytestoread == 0 || spec.offset + spec.bytestoread > size)
	{
		spec.bytestoread = size - spec.offset;
		if (spec.size > 0)
		{
			spec.bytestoread = MIN(spec.bytestoread, spec.size);
		}
	}
	ASSERT_RETFALSE(spec.size == 0 || spec.size >= spec.bytestoread);
	if (spec.buffer == NULL)
	{
#if 0 // AE 2007.10.16: We now delay buffer allocations until a thread begins processing a file read.
#if ISVERSION(DEVELOPMENT)
		if(spec.flags & PAKFILE_LOAD_GIANT_FILE)
		{
			spec.buffer = MALLOCBIGFL(spec.pool, spec.bytestoread + 1, spec.dbg_file, spec.dbg_line);
		}
		else
		{
			spec.buffer = MALLOCFL(spec.pool, spec.bytestoread + 1, spec.dbg_file, spec.dbg_line);
		}
#else
		spec.buffer = MALLOC(spec.pool, spec.bytestoread + 1);
#endif
		ASSERT_RETFALSE(spec.buffer);
		spec.size = spec.bytestoread + 1;
		((BYTE *)spec.buffer)[spec.bytestoread] = 0;		// 0 terminate for text files
#else
		spec.size = spec.bytestoread + 1;
#endif
	}
	if(TEST_MASK(spec.flags, PAKFILE_LOAD_IGNORE_BUFFER_SIZE))
	{
		spec.size = size;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class PAKLOAD_AUTOCLOSE
{
	ASYNC_FILE *			m_file;
	PAKFILE_LOAD_SPEC *		m_spec;

public:
	PAKLOAD_AUTOCLOSE(ASYNC_FILE * file, PAKFILE_LOAD_SPEC * spec) : m_file(file), m_spec(spec)
	{
		ASSERT(m_spec);
	}
	~PAKLOAD_AUTOCLOSE(void)
	{
		if (m_file)
		{
			AsyncFileClose(m_file);
		}
		if (!m_spec)
		{
			return;
		}
		sPakFileFileError(*m_spec);
		if ((m_spec->flags & PAKFILE_LOAD_FREE_BUFFER) && m_spec->buffer)
		{
			FREE(m_spec->pool, m_spec->buffer);
			m_spec->buffer = NULL;
		}
		m_spec->size = 0;
	}
	void Close(
		void)
	{
		if (m_file)
		{
			AsyncFileClose(m_file);
		}
		m_file = NULL;
		m_spec = NULL;
	}
	void HoldFile(
		void)
	{
		m_file = NULL;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)
	#define SPEC_FILE_LINE		,spec.dbg_file, spec.dbg_line
#else
	#define SPEC_FILE_LINE		
#endif

static BOOL sPakFileLoadFromDisk(
	PAKFILE_LOAD_SPEC & spec)
{
	ASSERT_RETFALSE(spec.fixedfilename[0]);
	
	if (!spec.gentime)
	{
		spec.gentime = sPakFileLoadGetLastModifiedTime(spec.fixedfilename);
		if (!spec.gentime)
		{
			sPakFileFileNotFound(spec);
			return FALSE;
		}
	}

	DWORD asyncFlags = FF_READ | FF_SEQUENTIAL_SCAN | FF_SHARE_READ;

	ASYNC_FILE * file = AsyncFileOpen(spec.fixedfilename, asyncFlags);
	if (!file)
	{
		sPakFileFileNotFound(spec);
		return FALSE;
	}

	PAKLOAD_AUTOCLOSE autoclose(file, &spec);

	BOOL bAddToPak = sPakFileLoadCheckAddToPak(spec);

	UINT64 size64 = AsyncFileGetSize(file);
	if(!(spec.flags & PAKFILE_LOAD_GIANT_FILE))
	{
		ASSERT_RETFALSE(size64 <= FILE_MAX_SIZE);
	}

	DWORD size = (DWORD)size64;

	if (!sPakFileLoadSetupBuffer(spec, size, bAddToPak))
	{
		return FALSE;
	}
	spec.frompak = FALSE;

	ASYNC_FILE_CALLBACK fpFileIOThreadCallback = NULL;
	if (bAddToPak)
	{
		fpFileIOThreadCallback = sPakFileLoadAddToPakCallback;
	}
	else
	{
		fpFileIOThreadCallback = spec.fpFileIOThreadCallback;
	}

	DWORD dwAsyncDataFlags = 0;

	if ( TEST_MASK( spec.flags, PAKFILE_LOAD_GIANT_FILE ) )
		SET_MASK( dwAsyncDataFlags, ADF_MALLOC_BIG );

	if (spec.flags & PAKFILE_LOAD_IMMEDIATE)
	{
		autoclose.HoldFile();		// prevents autoclose from closing the file due to ADF_CLOSE_AFTER_READ		
		SET_MASK( dwAsyncDataFlags, ADF_CLOSE_AFTER_READ );
		ASYNC_DATA asyncData(file, spec.buffer, spec.offset, spec.bytestoread, 
			dwAsyncDataFlags, fpFileIOThreadCallback, sPakFileLoadingThreadCallback, 
			&spec, spec.pool, &spec.buffer, spec.bytestoread SPEC_FILE_LINE );
		
		if ((spec.bytesread = AsyncFileReadNow(asyncData)) != spec.bytestoread)
		{
			return FALSE;
		}
	}
	else
	{
		PAKFILE_LOAD_SPEC * data = sPakFileLoadCopySpec(spec);
		ASSERT_RETFALSE(data);

		autoclose.HoldFile();		// prevents autoclose from closing the file due to ADF_CLOSE_AFTER_READ
		SET_MASK( dwAsyncDataFlags, ADF_FREEDATA | ADF_LOADINGCALLBACK_ONCANCEL | ADF_CLOSE_AFTER_READ );
		ASYNC_DATA asyncData(file, data->buffer, data->offset, data->bytestoread, 
			dwAsyncDataFlags, fpFileIOThreadCallback, sPakFileLoadingThreadCallback, 
			data, data->pool, &data->buffer, spec.bytestoread SPEC_FILE_LINE );

		if (!AsyncFileRead(asyncData, data->priority, data->threadId))
		{
			FREE(g_Pakfile.m_Pool, data);
			return FALSE;
		}
	}
	autoclose.Close();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileLoadFromPak(
	PAKFILE_LOAD_SPEC & spec,
	FILE_INDEX_NODE * fileindex)
{
	ASSERT_RETFALSE(fileindex);
	ASSERT_RETFALSE(AppCommonUsePakFile() == TRUE);
	if (fileindex->m_offset == (UINT64)-1) 
	{
		return FALSE;
	}
	ASSERT_RETFALSE(fileindex->m_fileSize >= 0);
	// It's okay for spec.offset to be equal to the file size - sound streaming does this occasionally.
	// In that case, spec.bytestoread will turn out zero and we'll exit early
	if (spec.offset >= fileindex->m_fileSize)
	{
		return FALSE;
	}

	if (spec.offset + spec.bytestoread > fileindex->m_fileSize)
	{
		spec.bytestoread = fileindex->m_fileSize - spec.offset;
	}
	spec.size_compressed = fileindex->m_fileSizeCompressed;

	PAKLOAD_AUTOCLOSE autoclose(NULL, &spec);

#if ISVERSION(DEVELOPMENT) && PAKFILE_COMPARE_TO_DATA
	sPakFileLoadCompareFilenodeToData(fileindex);
#endif

	PAKFILE * pakfile = g_Pakfile.m_Pakfiles.GetById(fileindex->m_idPakFile);
	ASSERT_RETFALSE(pakfile);
	ASSERT_RETFALSE(pakfile->m_filePakData != NULL);		

	UINT64 fileptr = fileindex->m_offset + (UINT64)spec.offset;
	UINT64 pakdataSize = 0;
	pakfile->m_csPakData.Enter();
	pakdataSize = pakfile->m_sizePakData;		// note: we don't really need to lock to access this if we were to make this atomic
	pakfile->m_csPakData.Leave();

	ASSERT_RETFALSE(fileptr + (UINT64)spec.bytestoread <= pakdataSize);

	if (!sPakFileLoadSetupBuffer(spec, fileindex->m_fileSize))		// todo: change to m_fileSizeUncompressed?
	{
		return FALSE;
	}

	spec.gentime = fileindex->m_GenTime;
	spec.frompak = TRUE;

	DWORD readsize;
	if (spec.size_compressed > 0) 
	{
		if (spec.bytestoread > 0) 
		{
			readsize = MIN(spec.bytestoread, spec.size_compressed);
		} 
		else 
		{
			readsize = spec.size_compressed;
		}
		ASSERT_RETFALSE(spec.offset == 0);
	} 
	else 
	{
		readsize = spec.bytestoread;
	}

	if (spec.flags & PAKFILE_LOAD_IMMEDIATE)
	{
		ASYNC_DATA asyncData(pakfile->m_filePakData, spec.buffer, fileptr, 
			readsize, 0, spec.fpFileIOThreadCallback, sPakFileLoadingThreadCallback, 
			&spec, spec.pool, &spec.buffer, spec.bytestoread SPEC_FILE_LINE);
		if ((spec.bytesread = AsyncFileReadNow(asyncData)) != spec.bytestoread)
		{
			return FALSE;
		}
	}
	else
	{
		PAKFILE_LOAD_SPEC * data = sPakFileLoadCopySpec(spec);
		ASSERT_RETFALSE(data);

		ASYNC_DATA asyncData(pakfile->m_filePakData, data->buffer, fileptr, 
			readsize, ADF_FREEDATA | ADF_LOADINGCALLBACK_ONCANCEL, 
			data->fpFileIOThreadCallback, sPakFileLoadingThreadCallback, 
			data, data->pool, &data->buffer, spec.bytestoread SPEC_FILE_LINE );
		if (!AsyncFileRead(asyncData, data->priority, data->threadId))
		{
			FREE(g_Pakfile.m_Pool, data);
			return FALSE;
		}
	}
	autoclose.Close();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static FILE_INDEX_NODE * sPakFileLoadGetIndex(
	PStrT & strFilePath,
	PStrT & strFileName,
	DWORD & key)
{
	FILE_INDEX_NODE * fileindex = NULL;
	g_Pakfile.m_FileHash.m_CS.ReadLock();
	{
		fileindex = sPakFileGetFileIndexNode(strFilePath, strFileName, &key);
		if (fileindex && TESTBIT(fileindex->m_flags, PAKFLAG_ADDING)) 
		{
			fileindex = NULL;
		}
	}
	g_Pakfile.m_FileHash.m_CS.EndRead();

	return fileindex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PakFileLoadGetIndex(
	PStrT & strFilePath,
	PStrT & strFileName)
{
	DWORD key;
	return sPakFileLoadGetIndex(strFilePath, strFileName, key);
}


//----------------------------------------------------------------------------
// decide between load form disk and load from pak (this always occurs
// after we decide between loading from server and loading locally)
// spec.fixedfilename must be filled out
//----------------------------------------------------------------------------
static PAKFILE_LOAD_MODE sPakFileLoadCheckDirect(
	FILE_INDEX_NODE * fileindex,
	const TCHAR * fixedfilename,
	DWORD flags,
	UINT64 & gentime,
	DWORD dwMagicNumber = 0,
	int nFileVersion = 0,
	const WCHAR* sourcefilename = NULL)
{
	ASSERT_RETVAL(fixedfilename, PAKFILE_FILE_NOT_FOUND);

	gentime = 0;			
	if (flags & PAKFILE_LOAD_FORCE_FROM_DISK)	// spec is to force from disk (config files, save files, etc)
	{
		goto _tryfromdisk;
	} 
	if (!AppCommonUsePakFile())		// not using pak files at all
	{
		goto _tryfromdisk;
	} 
	if (fileindex && TESTBIT(fileindex->m_flags, PAKFLAG_FILE_NOT_FOUND))
	{
		goto _filenotfound;
	}
	if (flags & PAKFILE_LOAD_FORCE_FROM_PAK)	// spec is to force from pak
	{
		goto _loadfrompak;
	} 
	if (!AppCommonGetDirectLoad())	// this flag is FALSE under the typical retail case (unless overridden using a command-line switch)
	{
		goto _loadfrompak;
	}

	// beyond this point, we will compare the file on disk with the pakfile version to determine which to load
	if (!fixedfilename[0])
	{
		goto _filenotfound;
	}
	if (!fileindex)
	{
		goto _tryfromdisk;
	}
	if (fileindex->m_fileId == INVALID_FILEID || fileindex->m_fileSize == 0 || fileindex->m_GenTime == 0)
	{
		goto _tryfromdisk;
	}
	if (fileindex->m_fileSize == 0 || fileindex->m_GenTime == 0 || 
		((dwMagicNumber != 0 && dwMagicNumber != INVALID_ID) && (fileindex->m_FileHeader.dwMagicNumber != dwMagicNumber || fileindex->m_FileHeader.nVersion != nFileVersion)))
	{
		goto _tryfromdisk;
	}
	if (TESTBIT(fileindex->m_flags, PAKFLAG_IS_CURRENT))
	{
		CLEARBIT(fileindex->m_flags, PAKFLAG_NOT_CURRENT);
		goto _loadfrompak;
	}
	if (TESTBIT(fileindex->m_flags, PAKFLAG_NOT_CURRENT))
	{
		goto _tryfromdisk;
	}

	// compare the filetime in the pakfile with the disk file last modified time
	UINT64 lastmodtime = sPakFileLoadGetLastModifiedTime(fixedfilename);
	static const UINT64 slop = 10000000 * 10;  // (10000000 * n) == n seconds
	if (sourcefilename == NULL) 
	{
		if (lastmodtime == 0)
		{
			// couldn't find it on disk load from pak
			// we can possibly set it to current here, but must set to not_current if we write the file later?
			goto _loadfrompak;
		} 
		else if (lastmodtime < fileindex->m_GenTime + slop) 
		{
			// we can possibly set it to current here, but must set to not_current if we write the file later?
			goto _loadfrompak;
		}
	} 
	else 
	{
		// If the source filename is available, also check against the source file       -rli
		UINT64 lastmodtime_source = sPakFileLoadGetLastModifiedTime(sourcefilename);
		if (lastmodtime == 0) 
		{
			if (lastmodtime_source == 0) 
			{
				// neither the source nor the cooked file is on disk, load from pak
				goto _loadfrompak;
			} 
			else 
			{
#if ISVERSION(SERVER_VERSION)
				if (AppTestDebugFlag(ADF_FILL_PAK_BIT)) 
				{
					goto _loadfrompak;
				}
#endif

				// source file's there but no cooked file, reload form disk
				goto _tryfromdisk;
			}
		} 
		else 
		{
			if (lastmodtime_source == 0) {
				// cooked file is there but source file isn't, report as missing?
				goto _filenotfound;
			} else if (lastmodtime_source > lastmodtime + slop) {
				// source file is modified after cooked file was cooked, reload from disk
				goto _tryfromdisk;
			} else if (lastmodtime < fileindex->m_GenTime + slop) {
				// cooked file is more recent than source file, and pak version is more recent than cooked file,
				// so load from pak
				goto _loadfrompak;
			}
		}
	}

	// the times differ, so reload the file
	gentime = lastmodtime;
	CLEARBIT(fileindex->m_flags, PAKFLAG_IS_CURRENT);
	SETBIT(fileindex->m_flags, PAKFLAG_NOT_CURRENT);
	return (gentime == 0 ? PAKFILE_FILE_NOT_FOUND : PAKFILE_LOAD_FROM_DISK);

_tryfromdisk:					// leave 0, we'll determine if the file actually exists & the time when we try to load it
	gentime = 0;			
	return PAKFILE_LOAD_FROM_DISK;

_loadfrompak:					// at this point, file may or may not be in pak, but we're going to look there anyway
	if (!fileindex)
	{
		goto _filenotfound;
	}
	gentime = fileindex->m_GenTime;
	return PAKFILE_LOAD_FROM_PAK;

_filenotfound:
	gentime = 0;
	return PAKFILE_FILE_NOT_FOUND;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PAKFILE_LOAD_MODE sPakFileLoadCheckDirect(
	PAKFILE_LOAD_SPEC & spec,
	FILE_INDEX_NODE * fileindex,
	DWORD dwMagicNumber = 0,
	int nFileVersion = 0)
{
	return sPakFileLoadCheckDirect(fileindex, spec.fixedfilename, spec.flags, spec.gentime, dwMagicNumber, nFileVersion);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PAKFILE_LOAD_MODE sFileNeedUpdateCheckDirect(
	const TCHAR * fixedfilename,
	PStrT & strFilePath,
	PStrT & strFileName,
	UINT64 & gentime,
	DWORD dwMagicNumber = 0,
	int nFileVersion = 0,
	const WCHAR* sourcefilename = NULL)
{
	DWORD key;
	FILE_INDEX_NODE * fileindex = sPakFileLoadGetIndex(strFilePath, strFileName, key);

	return sPakFileLoadCheckDirect(fileindex, fixedfilename, 0, gentime, dwMagicNumber, nFileVersion, sourcefilename);
}


//----------------------------------------------------------------------------
// called by: sPakFileLoad()
// determines load mode (disk, pak, srv)
//----------------------------------------------------------------------------
static PAKFILE_LOAD_MODE sPakFileLoadGetLoadMode(
	PAKFILE_LOAD_SPEC & spec,
	FILE_INDEX_NODE * & fileindex)
{
	if (!AppCommonUsePakFile())
	{
		return PAKFILE_LOAD_FROM_DISK;
	}

	fileindex = sPakFileLoadGetIndex(spec.strFilePath, spec.strFileName, spec.key);

	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT))
	{
		// If we're running fillpak client, the pakfile would be empty on startup, so then if the file index is
		// there, it's up to date. If the file index isn't there, try it from the server first.
		if (fileindex) 
		{
			return PAKFILE_LOAD_FROM_PAK;
		} 
		else 
		{
			if (spec.flags & PAKFILE_LOAD_ADD_TO_PAK) 
			{
				g_Pakfile.m_FileHash.m_CS.WriteLock();
				{
					fileindex = sPakFileGetFileIndexNode(spec.strFilePath, spec.strFileName, &spec.key);
					if (!fileindex)
					{
						fileindex = sPakFileAddToHash(spec.strFilePath, spec.strFileName, spec.key, TRUE);
					}
				}
				g_Pakfile.m_FileHash.m_CS.EndWrite();
			}

			return PAKFILE_LOAD_FROM_SRV_UPDATE;
		}
	}
	else if (AppCommonIsAnyFillpak()) 
	{
		if (fileindex) 
		{
			return PAKFILE_LOAD_FROM_PAK;
		} 
	}

	// if patch server is available, then consider the server version & always ignore the files on disk
	//if (AppIsPatching() && AppGetPatchChannel() != NULL) 
	if (AppGetAllowPatching())
	{
		if (!fileindex) 
		{
			//return PAKFILE_LOAD_FROM_SRV_NEW;
			//return PAKFILE_FILE_NOT_FOUND;
			//////////////////////////////////////
			// TMP FIX
			//////////////////////////////////////
			return PAKFILE_LOAD_FROM_DISK;
		} 
		if (TESTBIT(fileindex->m_flags, PAKFLAG_IS_CURRENT))
		{
			return PAKFILE_LOAD_FROM_PAK;
		}
		if (fileindex->m_bHasSvrHash && (fileindex->m_fileSize == 0 || fileindex->m_SvrHash != fileindex->m_Hash))
		{
			return PAKFILE_LOAD_FROM_SRV_UPDATE;
		}
		return PAKFILE_LOAD_FROM_PAK;
	} 
	else 
	{
		PAKFILE_LOAD_MODE mode = sPakFileLoadCheckDirect(spec, fileindex);
		return mode;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileParseSpec(
	PAKFILE_LOAD_SPEC & spec)
{
	ASSERT_RETFALSE(sSeparateFilename(spec.filename, spec.fixedfilename, spec.strFilePath, spec.strFileName));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileNeedsUpdate(
	PAKFILE_LOAD_SPEC & spec)
{
	if(!AppCommonUsePakFile())
	{
		return FALSE;
	}

	ASSERT_RETFALSE(sPakFileParseSpec(spec));

	FILE_INDEX_NODE * fileindex = NULL;
	PAKFILE_LOAD_MODE eLoadMode = sPakFileLoadGetLoadMode(spec, fileindex);

	return eLoadMode != PAKFILE_LOAD_FROM_PAK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PAK_ENUM sPakFileChangePakFileByType(
	const PAKFILE_LOAD_SPEC & spec)
{
	if(!AppCommonUsePakFile())
	{
		return spec.pakEnum;
	}
	if (gtAppCommon.m_bDisablePakTypeChange)
	{
		return spec.pakEnum;
	}

	if(!g_PakFileDesc[spec.pakEnum].m_Create)
	{
		ASSERT(g_PakFileDesc[g_PakFileDesc[spec.pakEnum].m_AlternativeIndex].m_Create);
		return g_PakFileDesc[spec.pakEnum].m_AlternativeIndex;
	}
	return spec.pakEnum;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPakFileLoad(
	PAKFILE_LOAD_SPEC & spec)
{
	spec.pakEnum = sPakFileChangePakFileByType(spec);

	if (   AppTestDebugFlag( ADF_FILL_PAK_CLIENT_BIT )
		|| AppTestDebugFlag( ADF_FORCE_SYNC_LOAD ) ) 
	{
		spec.flags |= PAKFILE_LOAD_IMMEDIATE;
	}

	ASSERT_RETFALSE(sPakFileParseSpec(spec));

#if ISVERSION(DEVELOPMENT)
	spec.dbg_issue_tick = GetTickCount();
#endif

	if (TEST_MASK(spec.flags, PAKFILE_LOAD_DUMMY_CALLBACK))
	{
		PAKFILE_LOAD_SPEC * data = sPakFileLoadCopySpec(spec);

		ASYNC_DATA asyncData(NULL, data->buffer, data->offset, data->bytestoread, ADF_FREEDATA | ADF_LOADINGCALLBACK_ONCANCEL, data->fpFileIOThreadCallback, sPakFileLoadingThreadCallback, data, data->pool);
		AsyncFilePostDummyRequest(asyncData, data->priority, data->threadId);
		return TRUE;
	}

	FILE_INDEX_NODE * fileindex = NULL;
	PAKFILE_LOAD_MODE mode = sPakFileLoadGetLoadMode(spec, fileindex);

	switch (mode) 
	{
	case PAKFILE_LOAD_FROM_DISK:
		if (sPakFileLoadFromDisk(spec)) 
		{
			return TRUE;
		} 
		else 
		{
			if (AppCommonGetDirectLoad() == FALSE) 
			{
				if (0 == (spec.flags & PAKFILE_LOAD_OPTIONAL_FILE))
				{
					TraceNoDirect("FILE NOT FOUND ON DISK: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
				}
			}
			return FALSE;
		}

	case PAKFILE_LOAD_FROM_PAK:
#if ISVERSION(DEVELOPMENT)
		if (AppCommonIsAnyFillpak())
		{
			if (spec.flags & PAKFILE_LOAD_DONT_LOAD_FROM_PAK)
			{
				spec.flags |= PAKFILE_LOAD_WAS_IN_PAK;
				return TRUE;
			}
		}
#endif
		if (sPakFileLoadFromPak(spec, fileindex))
		{
			spec.flags |= PAKFILE_LOAD_WAS_IN_PAK;
			if (AppGetAllowFillLoadXML() && spec.offset == 0 && !(spec.flags & PAKFILE_LOAD_DONT_ADD_TO_FILL_XML) &&
				fileindex && InterlockedExchange((LONG*) &fileindex->m_bAlreadyWrittenToXml, TRUE) == FALSE) 
			{
				sPakFileAddFileToLoadXML(spec.fixedfilename, spec.pakEnum, fileindex->m_Hash.m_Hash, TRUE);
			}
			return TRUE;
		} 
		else 
		{
			if (AppCommonGetDirectLoad() == FALSE) 
			{
				if ( 0 == ( spec.flags & PAKFILE_LOAD_OPTIONAL_FILE ) )
				{
					TraceNoDirect("FILE NOT FOUND IN PAK: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
				}
			}
			return FALSE;
		}

	case PAKFILE_LOAD_FROM_SRV_UPDATE:
#if !ISVERSION(SERVER_VERSION)
		if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) {
			// If we're running fillpak client
			PatchClientRequestFile(spec, fileindex);
			if (spec.bytesread == 0) {
				// the server does not have the file, load it from disk instead
				if (sPakFileLoadFromDisk(spec)) {
					return TRUE;
				} else {
					if (AppCommonGetDirectLoad() == FALSE) {
						if ( 0 == ( spec.flags & PAKFILE_LOAD_OPTIONAL_FILE ) )
						{
							TraceNoDirect("FILE NOT FOUND ON DISK: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
						}
					}
					return FALSE;
				}
			} else {
				return TRUE;
			}
		} else {
			TraceDownload("Downloading %s%s\r\n", spec.bufFilePath, spec.bufFileName);
			if (fileindex->m_fileSize == 0) {
				TraceDownload("    Client doesn't have the file\r\n");
			} else {
				TCHAR hashClt[SHA1HashSize*2+1];
				TCHAR hashSvr[SHA1HashSize*2+1];
				for (UINT32 i = 0; i < SHA1HashSize; i++) {
					UINT8 val = fileindex->m_Hash.m_Hash[i];
					PStrPrintf(hashClt+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
					val = fileindex->m_SvrHash.m_Hash[i];
					PStrPrintf(hashSvr+i*2, 3, (val < 16 ? _T("0%x") : _T("%x")), val);
				}
				TraceDownload("    Client %s Server %s\r\n", hashClt, hashSvr);
			}
			// Just download the file from the server regularly
			return PatchClientRequestFile(spec, fileindex);
		}
#else
		return FALSE;
#endif

	case PAKFILE_LOAD_FROM_SRV_NEW:
		ASSERT(0);	// NYI
		break;

	case PAKFILE_FILE_NOT_FOUND:
	default:
		if ( spec.flags & PAKFILE_LOAD_OPTIONAL_FILE )
			break;
#ifdef TRACE_FILE_NOT_FOUND
		TraceError("FILE NOT FOUND: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
#endif
		if (AppCommonGetDirectLoad() == FALSE && 0 == ( spec.flags & PAKFILE_LOAD_NODIRECT_IGNORE_MISSING ) ) {
			TraceNoDirect("FILE NOT FOUND AT ALL: %s", spec.fixedfilename[0] ? spec.fixedfilename : spec.filename);
		}
		break;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * PakFileLoadNow(
	PAKFILE_LOAD_SPEC & spec)
{
	spec.flags |= PAKFILE_LOAD_IMMEDIATE;
	if (!sPakFileLoad(spec))
	{
		sPakFileSpecCleanup(spec);
		return NULL;
	}
	return spec.buffer;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileLoad(
	PAKFILE_LOAD_SPEC & spec)
{
	if (!sPakFileLoad(spec))
	{
		sPakFileSpecCleanup(spec);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ASYNC_FILE * PakFileGetAsyncFileCopy(
	PAKFILE_LOAD_SPEC& tSpec)
{
	FILE_INDEX_NODE * fileindex = sPakFileLoadGetIndex(tSpec.strFilePath, tSpec.strFileName, tSpec.key);
	ASSERT_RETNULL(fileindex);
	PAKFILE * pPakFile = PakFileGetPakByID(fileindex->m_idPakFile);
	ASSERT_RETNULL(pPakFile);
	ASYNC_FILE * pAsyncFile = AsyncFileOpen(AsyncFileGetFilename(pPakFile->m_filePakData), FF_READ | FF_RANDOM_ACCESS | FF_SHARE_READ | FF_SHARE_WRITE | FF_SEQUENTIAL_SCAN | FF_SYNCHRONOUS_READ);
	ASSERT_RETNULL(pAsyncFile);

	LARGE_INTEGER temp;
	ASSERT(sizeof(LARGE_INTEGER) == sizeof(UINT64));
	ASSERT(MemoryCopy(&temp, sizeof(LARGE_INTEGER), &fileindex->m_offset, sizeof(UINT64)));
	SetFilePointerEx(AsyncFileGetHandle(pAsyncFile), temp, NULL, FILE_BEGIN);
	return pAsyncFile;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileAddFile(
	PAKFILE_LOAD_SPEC & spec,
	BOOL bCleanUp)
{
	ASSERT_GOTO(spec.size, _cleanup);
	ASSERT_GOTO(spec.gentime != 0, _cleanup);
	ASSERT_GOTO(spec.pakEnum < NUM_PAK_ENUM, _cleanup);

	if (!AppCommonUsePakFile() || AppTestDebugFlag(ADF_DONT_ADD_TO_PAKFILE))
	{
		goto _cleanup;
	}

	if (!(spec.flags & PAKFILE_LOAD_FORCE_ALLOW_PAK_ADDING) && !AppCommonGetUpdatePakFile())
	{
		goto _cleanup;
	}

	if (spec.flags & PAKFILE_LOAD_NEVER_PUT_IN_PAK)
	{
		goto _cleanup;
	}

	ASSERT_GOTO(sPakFileParseSpec(spec), _cleanup);

	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	spec.bytesread = spec.size;		// we use bytesread in sPakFileLoadAddToPak()

	// get the fileindex & see if it's an empty index or perhaps currently being added
	// add it to the file index if not with the flag ADDING
	FILE_INDEX_NODE * fileindex = NULL;
	BOOL bAlreadyAdding = FALSE;
#if ISVERSION(DEVELOPMENT)
	BOOL bAlreadyInPak = FALSE;
#endif
	g_Pakfile.m_FileHash.m_CS.WriteLock();
	{
		fileindex = sPakFileGetFileIndexNode(spec.strFilePath, spec.strFileName, &spec.key);
		if (!fileindex)
		{
			fileindex = sPakFileAddToHash(spec.strFilePath, spec.strFileName, spec.key, TRUE);
		}
#if ISVERSION(DEVELOPMENT)
		else if (AppIsFillingPak() && fileindex->m_offset != (UINT64)-1)
		{
			bAlreadyInPak = TRUE;
			fileindex = NULL;
		}
#endif
		else if (TESTBIT(fileindex->m_flags, PAKFLAG_ADDING))
		{
			bAlreadyAdding = TRUE;
			fileindex = NULL;
		}
		else if (TESTBIT(fileindex->m_flags, PAKFLAG_INDEX_ONLY))
		{
			CLEARBIT(fileindex->m_flags, PAKFLAG_INDEX_ONLY);
		}
		if (fileindex)
		{
			SETBIT(fileindex->m_flags, PAKFLAG_ADDING);
		}
	}
	g_Pakfile.m_FileHash.m_CS.EndWrite();
	if (!fileindex)
	{
#if ISVERSION(DEVELOPMENT)
		if (AppIsFillingPak() && bAlreadyInPak)
		{
			FILLPAK_TRACE("canceled PakFileAddFile() for FILE: [%s] because file is already in pak.", spec.fixedfilename);
		}
#endif
#if TRACE_RACE_CONDITION
		if (bAlreadyAdding)
		{
			FILLPAK_TRACE("canceled PakFileAddFile() for FILE: [%s] because add is already in progress.", spec.fixedfilename);
		}
#endif
		goto _cleanup;
	}

	if (!(spec.flags & PAKFILE_LOAD_FILE_ALREADY_HASHED)) 
	{
		ASSERT(SHA1Calculate((UINT8 *)spec.buffer, spec.size, spec.hash.m_Hash));
	}

	if (!sPakFileAddFileToPak(spec))
	{
		goto _cleanup;
	}

	if (bCleanUp)
	{
		sPakFileSpecCleanup(spec);
	}

	return TRUE;

_cleanup:
	if (bCleanUp)
	{
		sPakFileSpecCleanup(spec);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// Get a fileid from a filename
//----------------------------------------------------------------------------
FILEID PakFileGetFileId(
	const TCHAR * filename,
	PAK_ENUM pakEnum)
{
	FILEID fileid = INVALID_FILEID;
	
	ASSERT_RETVAL(filename, fileid);

	TCHAR fixedfilename[MAX_PATH];
	PStrTDef(strFilePath, MAX_PATH);
	PStrTDef(strFileName, MAX_PATH);
	ASSERT_RETVAL(sSeparateFilename(filename, fixedfilename, strFilePath, strFileName), fileid);

	if (!AppCommonUsePakFile())
	{
		fileid = PakFileComputeFileId(strFilePath, strFileName);
		return fileid;
	}

	DWORD key;
	FILE_INDEX_NODE * fileindex = NULL;
	g_Pakfile.m_FileHash.m_CS.ReadLock();
	{
		fileindex = g_Pakfile.m_FileHash.FindInHash(strFilePath, strFileName, &key);
	}
	g_Pakfile.m_FileHash.m_CS.EndRead();

	if (fileindex)
	{
		fileid = fileindex->m_fileId;
		return fileid;
	}

	unsigned int idGenPak = PakFileGetGenPakfileId(pakEnum);
	ASSERT_RETVAL(g_Pakfile.m_Pakfiles.IsValidPakfileId(idGenPak), INVALID_FILEID);

	g_Pakfile.m_FileHash.m_CS.WriteLock();
	ONCE
	{
		fileindex = g_Pakfile.m_FileHash.FindInHash(strFilePath, strFileName, key);		// need to do this again to validate it hasn't been added by another thread
		if (!fileindex)
		{
			fileindex = sPakFileAddToHash(strFilePath, strFileName, key, TRUE);
			SETBIT(fileindex->m_flags, PAKFLAG_INDEX_ONLY);		// mark this node as unlinked to data for the moment
		}
		ASSERT_BREAK(fileindex);
		fileid = fileindex->m_fileId;
	}
	g_Pakfile.m_FileHash.m_CS.EndWrite();
		
	return fileid;	
}


//----------------------------------------------------------------------------
// get filename from fileid
//----------------------------------------------------------------------------
BOOL PakFileGetFileName(
	FILEID fileid,
	TCHAR * filename,
	unsigned int len)
{
	return g_Pakfile.m_FileIndex.GetFileNameById(fileid, filename, len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PakFileFreeForApp(
	void)
{
	CSAutoLock lock(&g_csPakfile);
	g_iRefCount--;
	if (g_iRefCount > 0) 
	{
		return;
	}
	ASSERTV_DO( g_iRefCount >= 0, "Pakfile refcount went negative!" )
	{
		g_iRefCount = 0;
		return;
	}
	g_PakFileDesc.Destroy();
	ASSERT_RETURN(g_Pakfile.m_bInitialized);

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	g_Pakfile.m_xmlStartupLoad.Free();
#endif
	g_Pakfile.m_FileHash.Free();
	g_Pakfile.m_FileIndex.Free();
	g_Pakfile.m_Pakfiles.Free();
	structclear(g_Pakfile);

#if ISVERSION(DEVELOPMENT)
	if (AppIsFillingPak())
	{
		// reopen once to write all temp index nodes as consolidated
		static BOOL bReopened = FALSE;
		if (!bReopened)
		{
			bReopened = TRUE;
			PakFileInitForApp();
			PakFileFreeForApp();
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileUsedInApp(
	PAK_ENUM ePak)
{
	return g_PakFileDesc[ePak].m_Create;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sFileNeedsUpdateTrace(
	const char * format,
	...)
{
	va_list args;
	va_start(args, format);

	if ( AppCommonIsAnyFillpak() )
	{
		// In fillpak mode, log to the main logfile
		LogMessageV( format, args );
	}
	else
	{
		vtrace(format, args);
	}
}

//----------------------------------------------------------------------------
// determine if a file needs to be updated
//
// TODO: should optimize out so we don't have to call the string
//       manipulation functions on filenames
//----------------------------------------------------------------------------
BOOL FileNeedsUpdate(
	const TCHAR * filename,
	const TCHAR * sourcefile,
	DWORD dwFileMagicNumber,
	int nFileVersion,
	DWORD dwFlags,
	UINT64 * gentime,
	BOOL * bInPak,
	int nMinFileVersion )
{
	if (bInPak)
	{
		*bInPak = FALSE;
	}
	WCHAR fullname[MAX_PATH];
	WCHAR wfilename[MAX_PATH];
	PStrCvt(wfilename, filename, MAX_PATH);
	FileGetFullFileName(fullname, wfilename, MAX_PATH);

	WCHAR fullsource[MAX_PATH];
	WCHAR wsourcefile[MAX_PATH];
	PStrCvt(wsourcefile, sourcefile, MAX_PATH);
	FileGetFullFileName(fullsource, wsourcefile, MAX_PATH);

	TCHAR fixedfilename[MAX_PATH];
	PStrTDef(strFilePath, MAX_PATH);
	PStrTDef(strFileName, MAX_PATH);
	ASSERT_RETNULL(sSeparateFilename(fullname, fixedfilename, strFilePath, strFileName));

	// if patch server is available, then always ignore the files on disk
	//if (AppIsPatching() && AppGetPatchChannel() != NULL) 
	if (AppGetAllowPatching() && PakFileLoadGetIndex(strFilePath, strFileName) != NULL)
	{
		return FALSE;
	}

	// if we're not allowing direct loads, nothing should get updated
	if (!AppCommonGetDirectLoad())
	{
		return FALSE;
	}

	if (!sourcefile || sourcefile[0] == 0)
	{
		return FALSE;
	}

	if (    AppTestDebugFlag( ADF_IN_CONVERT_STEP ) 
		 && AppTestDebugFlag( ADF_FORCE_ASSET_CONVERSION_UPDATE ) )
	{
		return TRUE;
	}

	BOOL inPakFile = FALSE;
	UINT64 gentimeTemp = 0;
	if (!gentime)
	{
		gentime = &gentimeTemp;
	}
	if (AppCommonUsePakFile() && !TEST_MASK(dwFlags, FILE_UPDATE_ALWAYS_CHECK_DIRECT))
	{
		PAKFILE_LOAD_MODE mode = sFileNeedUpdateCheckDirect(fixedfilename, strFilePath, strFileName, *gentime, dwFileMagicNumber, nFileVersion, fullsource);
		inPakFile = (mode == PAKFILE_LOAD_FROM_PAK);
		if (inPakFile == FALSE && (dwFlags & FILE_UPDATE_UPDATE_IF_NOT_IN_PAK))
		{
			sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  Not found in pak && FILE_UPDATE_UPDATE_IF_NOT_IN_PAK was set", fixedfilename);
			return TRUE;
		}
	}
	if (*gentime == 0)
	{
		*gentime = sPakFileLoadGetLastModifiedTime(fixedfilename);
		if (*gentime == 0)
		{
			if ( ! TEST_MASK(dwFlags, FILE_UPDATE_OPTIONAL_FILE) )
				sFileNeedsUpdateTrace("FileNeedsUpdate() -- FILE NOT FOUND:  %s\n", fixedfilename);
			return TRUE;
		}
	}

	// We need more slop in game mode because synched assets can have larger differences
	// in time stamps from slow perforce transfers.
	if( !AppCommonIsAnyFillpak() || AppCommonIsFillpakInConvertMode() )
	{
		UINT pad_seconds = ( c_GetToolMode() || AppCommonIsFillpakInConvertMode() ) ? 30 : 300;
		const UINT64 slop = 10000000 * pad_seconds;  // (10000000 * n) == n seconds
		UINT64 sourcetime = sPakFileLoadGetLastModifiedTime(fullsource);
		if (sourcetime > 0) 
		{
			// is the current version filetime >= source filetime?
			if (*gentime + slop < sourcetime) 
			{
				sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  file time was less than source file time by %3.2f seconds\n", fixedfilename, float( (sourcetime - *gentime) / 10000000 ) );
				*gentime = sourcetime;
				return TRUE;
			}
		}
	}

	// sometimes we don't check the file version and magic number - textures are that way
	if (dwFileMagicNumber == INVALID_ID || dwFileMagicNumber == 0)
	{
		return FALSE;
	}

	// by this point, if it's in the pakfile, it's the right version, since we checked the magic number and version number already
	if (inPakFile)
	{
		if (bInPak)
		{
			*bInPak = TRUE;
		}
		return FALSE;
	}

	// check the version in file header
	FILE_HEADER hdr;
	HANDLE file = FileOpen(fullname, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN);
	if (file == INVALID_FILE) 
	{
		sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  file not on disk\n", fullname);
		return TRUE;
	}

	DWORD bytes_read;
	if (!ReadFile(file, &hdr, sizeof(FILE_HEADER), &bytes_read, NULL))
	{
		sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  system error follows:\n", fullname);
		PrintSystemError(GetLastError());
		return TRUE;
	}
	FileClose(file);
	if (bytes_read != sizeof(FILE_HEADER))
	{
		sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  error reading file\n", fullname);
		return TRUE;
	}

	if (hdr.dwMagicNumber != dwFileMagicNumber)
	{
		sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  magic number mismatch\n", fullname);
		return TRUE;
	}

	BOOL bCheckVersion = !TEST_MASK( dwFlags, FILE_UPDATE_CHECK_VERSION_UPDATE_FLAG )
					   || AppTestDebugFlag( ADF_ALLOW_FILE_VERSION_UPDATE );

	if ((nMinFileVersion > 0 && hdr.nVersion < nMinFileVersion)
		|| (bCheckVersion && hdr.nVersion != nFileVersion))
	{
		sFileNeedsUpdateTrace("FileNeedsUpdate():  FILE:%s  version mismatch\n", fullname);
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileCheckUpToDateWithServer(
	TCHAR * filename)
{
	TCHAR fixedfilename[MAX_PATH];
	PStrTDef(strFilePath, MAX_PATH);
	PStrTDef(strFileName, MAX_PATH);

	FILE_INDEX_NODE * fileindex = NULL;

	ASSERT_RETFALSE(sSeparateFilename(filename, fixedfilename, strFilePath, strFileName));

	if (AppCommonUsePakFile()) 
	{
		DWORD key= 0;
		fileindex = sPakFileGetFileIndexNode(strFilePath, strFileName, &key);
	}
	if (!fileindex)
	{
		return TRUE;
	}
	if (!fileindex->m_bHasSvrHash)
	{
		return TRUE;
	}
	return (fileindex->m_Hash == fileindex->m_SvrHash);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PakFileDecompressFile(
	ASYNC_DATA* pData)
{
	ASSERT_RETURN(pData != NULL);

	PAKFILE_LOAD_SPEC* pSpec = (PAKFILE_LOAD_SPEC*)pData->m_UserData;
	if (pSpec == NULL || pSpec->size_compressed == 0 || pSpec->bytestoread < pSpec->size_compressed) {
		return;
	}

	void* pBuffer = MALLOC(g_ScratchAllocator, pSpec->size_compressed);
	ASSERT_RETURN(pBuffer != NULL);

	UINT32 size = pSpec->bytestoread;
	ASSERT_GOTO(MemoryCopy(pBuffer, pSpec->size_compressed, pSpec->buffer, pSpec->size_compressed),_err);
	ASSERT_GOTO(ZLIB_INFLATE((UINT8*)pBuffer, pSpec->size_compressed, (UINT8*)pSpec->buffer, &size), _err);
	ASSERT_GOTO(size == pSpec->bytestoread, _err);

//	TracePersonal("uncompressing %s%s full %d compressed %d\n", pSpec->strFilePath.str, pSpec->strFileName.str, size, pSpec->size_compressed);

	pData->m_IOSize = pSpec->bytestoread;
	pData->m_Bytes = pSpec->bytestoread;
	pSpec->bytesread = size;
	FREE(g_ScratchAllocator, pBuffer);
	return;

_err:
	if (pBuffer != NULL) {
		FREE(g_ScratchAllocator, pBuffer);
	}
	pData->m_IOSize = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT32 PakFileGetID(
	PAKFILE * pakfile)
{
	ASSERT_RETVAL(pakfile != NULL, (UINT32)-1);
	return pakfile->m_idPakFile;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ASYNC_FILE * PakFileGetAsyncFile(
	PAKFILE * pakfile)
{
	return pakfile->m_filePakData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PAKFILE * PakFileGetPakByID(
	UINT32 pakfileid)
{
	return g_Pakfile.m_Pakfiles.GetById(pakfileid);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PAK_ENUM PakFileGetTypeByID(
	UINT32 pakfileid)
{
	return g_Pakfile.m_Pakfiles.GetTypeById(pakfileid);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PakFileGetGMBuildVersion(
	PAK_ENUM ePakFile,
	FILE_VERSION &tVersion)
{
	return g_Pakfile.m_Pakfiles.GetGMBuildVersion( ePakFile, tVersion );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PAK_ENUM PakFileGetTypeByName(
	LPCTSTR pakBaseName)
{
	UINT32 baseLen = PStrLen(pakBaseName);
	while (baseLen > 0 && isdigit((unsigned char)pakBaseName[baseLen-1])) {
		baseLen--;
	}
	if (baseLen > 0) {
		for (UINT32 i = 0; i < g_PakFileDesc.Count(); i++) {
			if (PStrICmp(g_PakFileDesc[i].m_BaseName, pakBaseName, baseLen) == 0) {
				return g_PakFileDesc[i].m_Index;
			}
		}
	}
	return PAK_INVALID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT32 PakFileGetIdxCount(
	void)
{
	unsigned int bucketcount;
	g_Pakfile.m_FileIndex.m_CS.ReadLock();
	bucketcount = g_Pakfile.m_FileIndex.m_BucketCount;
	g_Pakfile.m_FileIndex.m_CS.EndRead();
	return bucketcount * PAKFILE_INDEX_BUFFER;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILE_INDEX_NODE * PakFileGetFileByIdx(
	UINT32 idx)
{
	return g_Pakfile.m_FileIndex.GetByIndex(idx);
}


//----------------------------------------------------------------------------
// record the file to a global list of what gets loaded
// note: the really crappy thing about using CMarkup to do logging is that
// it has to write out the entire file each time you Save().
// putting that in a critical section is suckage.
//----------------------------------------------------------------------------
static BOOL sPakFileAddFileToLoadXML(
	const TCHAR * filename,
	UINT32 pakfileid,
	UINT8* hash,
	BOOL bFileInPak,
	BOOL bSave /*= TRUE*/ )
{
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	if (!AppGetAllowFillLoadXML()) 
	{
		return TRUE;
	}
	ASSERT_RETFALSE(filename && filename[0]);

	char pakfileid_str[16];
	PStrPrintf(pakfileid_str, 16, "%d", pakfileid);

	CSAutoLock lock(&g_Pakfile.m_csStartupLoad);
	bFileInPak = TRUE;
	if (bFileInPak) 
	{
		ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.AddChildElem("FileInPak"));
	} 
	else 
	{
		ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.AddChildElem("FileOnDisk"));
	}
	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.IntoElem());
	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.SetData(filename));

	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.AddChildElem("Hash"));
	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.IntoElem());

	{
		TCHAR strHash[2*SHA1HashSize+1];
		for (UINT32 i = 0; i < SHA1HashSize; i++) {
			if (hash[i] < 0x10) {
				PStrPrintf(strHash+2*i, 3, "0%x", hash[i]);
			} else {
				PStrPrintf(strHash+2*i, 3, "%x", hash[i]);
			}
		}
		ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.SetData(strHash));
	}

	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.OutOfElem());
	if( bSave )
	{
		ASSERT_RETFALSE(g_Pakfile.m_xmlStartupLoad.Save(xml_filename));
	}
	return TRUE;
#else
	UNREFERENCED_PARAMETER(filename);
	UNREFERENCED_PARAMETER(pakfileid);
	UNREFERENCED_PARAMETER(bFileInPak);
	UNREFERENCED_PARAMETER(bSave);
	UNREFERENCED_PARAMETER(hash);
	return TRUE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void PakFileTrace(
	void)
{
	if( AppGetAllowFillLoadXML() )
	{
		g_Pakfile.m_FileHash.m_CS.ReadLock();
		{
			unsigned int bucketcount = 0;
			g_Pakfile.m_FileIndex.m_CS.ReadLock();
			bucketcount = g_Pakfile.m_FileIndex.m_BucketCount;
			g_Pakfile.m_FileIndex.m_CS.EndRead();
			unsigned int indexcount = bucketcount * PAKFILE_INDEX_BUFFER;

			for (unsigned int ii = 0; ii < indexcount; ++ii)
			{
				FILE_INDEX_NODE * indexnode = g_Pakfile.m_FileIndex.GetByIndex(ii);
				if (!indexnode)
				{
					continue;
				}
				if (indexnode->m_FileName.IsEmpty())
				{
					continue;
				}

				const char * pakfilename = "";
				PAKFILE * pakfile = g_Pakfile.m_Pakfiles.GetById(indexnode->m_idPakFile);
				if (pakfile)
				{
					pakfilename = OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(pakfile->m_filePakIndex));
					TCHAR fullfilename[DEFAULT_FILE_WITH_PATH_SIZE];
					PStrPrintf(fullfilename, DEFAULT_FILE_WITH_PATH_SIZE, _T("%s%s"), indexnode->m_FilePath.str, indexnode->m_FileName.str);

					sPakFileAddFileToLoadXML(fullfilename, pakfile->m_ePakEnum, indexnode->m_Hash.m_Hash, TRUE, FALSE);
				}
			}
		}
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
		g_Pakfile.m_xmlStartupLoad.Save(xml_filename);
#endif //!ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
		g_Pakfile.m_FileHash.m_CS.EndRead();
	}
	else
	{
		TraceDebugOnly("-------------------------------------------------------- PAKFILE DUMP --------------------------------------------------------");
		TraceDebugOnly("%-50s  %16s  %-120s  %60s  %12s  %8s", "PAKFILE", "FILEID", "PATH", "FILENAME", "OFFSET", "SIZE");
		g_Pakfile.m_FileHash.m_CS.ReadLock();
		{
			unsigned int bucketcount = 0;
			g_Pakfile.m_FileIndex.m_CS.ReadLock();
			bucketcount = g_Pakfile.m_FileIndex.m_BucketCount;
			g_Pakfile.m_FileIndex.m_CS.EndRead();
			unsigned int indexcount = bucketcount * PAKFILE_INDEX_BUFFER;

			for (unsigned int ii = 0; ii < indexcount; ++ii)
			{
				FILE_INDEX_NODE * indexnode = g_Pakfile.m_FileIndex.GetByIndex(ii);
				if (!indexnode)
				{
					continue;
				}
				if (indexnode->m_FileName.IsEmpty())
				{
					continue;
				}

				const char * pakfilename = "";
				PAKFILE * pakfile = g_Pakfile.m_Pakfiles.GetById(indexnode->m_idPakFile);
				if (pakfile)
				{
					pakfilename = OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(AsyncFileGetFilename(pakfile->m_filePakIndex));
				}

				TraceDebugOnly("%-50s  %016I64x  %-120s  %60s  %12I64d  %8d", pakfilename, indexnode->m_fileId.id, indexnode->m_FilePath.GetStr(), indexnode->m_FileName.GetStr(), indexnode->m_offset, indexnode->m_fileSize);
			}
		}
		g_Pakfile.m_FileHash.m_CS.EndRead();
		TraceDebugOnly("------------------------------------------------------ END PAKFILE DUMP ------------------------------------------------------");

	}
	
	// in case we made a request for it
	gbPakfileTrace = FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static int PStrPrintToFile(
	HANDLE file,
	const char * format,
	...)
{
	char buf[4096];

	va_list args;
	va_start(args, format);

	int len = PStrVprintf(buf, arrsize(buf), format, args);
	ASSERT(FileWrite(file, buf, len) == (unsigned int)len);
	return len;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void PakFileLog(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	OS_PATH_CHAR szFilename[MAX_PATH];
	PStrPrintf(szFilename, _countof(szFilename), OS_PATH_TEXT("%s%s"), LogGetRootPath(), OS_PATH_TEXT("pakfile.csv"));

	HANDLE file = FileOpen(szFilename, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	ASSERT_RETURN(file != INVALID_FILE);

	PStrPrintToFile(file, "PAKID, FILEID, PATH, FILENAME, OFFSET, SIZE\n");

	g_Pakfile.m_FileHash.m_CS.ReadLock();
	{
		unsigned int bucketcount = 0;
		g_Pakfile.m_FileIndex.m_CS.ReadLock();
		bucketcount = g_Pakfile.m_FileIndex.m_BucketCount;
		g_Pakfile.m_FileIndex.m_CS.EndRead();
		unsigned int indexcount = bucketcount * PAKFILE_INDEX_BUFFER;

		for (unsigned int ii = 0; ii < indexcount; ++ii)
		{
			FILE_INDEX_NODE * indexnode = g_Pakfile.m_FileIndex.GetByIndex(ii);
			if (!indexnode)
			{
				continue;
			}
			if (!indexnode->m_FileName.GetStr())
			{
				continue;
			}

			PStrPrintToFile(file, "%d, %016I64x, %s, %s, %I64d, %8d\n", indexnode->m_iPakType, indexnode->m_fileId.id, indexnode->m_FilePath.GetStr(), indexnode->m_FileName.GetStr(), indexnode->m_offset, indexnode->m_fileSize);
		}
	}
	g_Pakfile.m_FileHash.m_CS.EndRead();

	FileClose(file);
#endif // !ISVERSION(SERVER_VERSION)
}
#endif
