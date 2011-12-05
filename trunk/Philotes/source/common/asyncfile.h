// ---------------------------------------------------------------------------
// FILE:	asyncfile.h
// DESC:	asynchronous file i/o library
//
// DETAIL:	handles asynchronous file i/o via i/o completion ports
//			creates N worker threads (named "async_##" in debug)
//			allows multiple pending reads & writes on the same file
//			provides callback hooks for post-read or post-write which can be
//			called either from the async thread or from the loading thread
//
//			because the system can refuse to handle too many pending requests
//			there are tiered thresholds of how many requests can be active
//			based on priority.  for example, a low priority request will be
//			queued if the number of total active requests exceeds 
//
// TODO:	allow cancel, handle errors
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// FUNCTION GUIDE
//
// AsyncFileSystemInit - intialize async file system, call once only
// AsyncFileSystemFree - shutdown async file system, call at exit
// AsyncFileRegisterThread - each thread that can pick up and process completed
//		i/o requests should register itself and pass the returned threadid
//		to any file i/o functions that have a loading thread callback
// AsyncFileProcessComplete - call from the registered loading thread to
//		process all completed i/o requests
// AsyncFileOpen - open a disk file for async file i/o
// AsyncFileRead - request an async read from an open async file
// AsyncFileReadNow - request an immediate (blocking) read from an open async file
// AsyncFileWrite - request an async write to an open async file
// AsyncFileWriteNow - request an immediate (blocking) write to an open async file
// ---------------------------------------------------------------------------
#ifndef _ASYNCFILE_H_
#define _ASYNCFILE_H_
#include "ospathchar.h"


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define ASYNC_REPORT_WIDTH		(unsigned int)128			// line width for async reports


// ---------------------------------------------------------------------------
// TYPES
// ---------------------------------------------------------------------------
struct ASYNC_FILE;
struct ASYNC_DATA;

typedef PRESULT (*ASYNC_FILE_CALLBACK)(ASYNC_DATA & data);


// ---------------------------------------------------------------------------
// ENUMERATIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// flags for file open
// ---------------------------------------------------------------------------
enum FILE_FLAGS
{
	FF_READ						= MAKE_MASK(0),				// open a file for reading
	FF_WRITE					= MAKE_MASK(1),				// open a file for writing  (must be either FF_READ and/or FF_WRITE)
	FF_SHARE_DELETE				= MAKE_MASK(2),				// allow someone else to delete the file from under me
	FF_SHARE_READ				= MAKE_MASK(3),				// allow someone else to open the file for reading
	FF_SHARE_WRITE				= MAKE_MASK(4),				// allow someone else to open the file for writing
	FF_CREATE_ALWAYS			= MAKE_MASK(5),				// overwrite a file
	FF_CREATE_NEW				= MAKE_MASK(6),				// fails if the file already exists
	FF_OPEN_ALWAYS				= MAKE_MASK(7),				// creates a file if it doesn't already exist
	FF_TEMPORARY				= MAKE_MASK(8),				// temporary file
	FF_NO_BUFFERING				= MAKE_MASK(9),				// don't use: will give max asynch performance once implemented, but requires file access to be on sector boundaries
	FF_WRITE_THROUGH			= MAKE_MASK(10),			// bypass system cache
	FF_RANDOM_ACCESS			= MAKE_MASK(11),			// hint: file is random access
	FF_SEQUENTIAL_SCAN			= MAKE_MASK(12),			// hint: file is sequential scan
	FF_SYNCHRONOUS_READ			= MAKE_MASK(13),			// all read operations on the file is done synchronously
	FF_SYNCHRONOUS_WRITE		= MAKE_MASK(14),			// all write operations ont he file are done synchronously
	FF_OVERLAPPED				= MAKE_MASK(15),			// the file was opened using OVERLAPPED
	FF_USE_REL_PATH				= MAKE_MASK(16),			// doesn't use absolute path
};


// ---------------------------------------------------------------------------
// async priorities
// ---------------------------------------------------------------------------
enum 
{
	ASYNC_PRIORITY_0,										// lowest priority
	ASYNC_PRIORITY_1,
	ASYNC_PRIORITY_2,									
	ASYNC_PRIORITY_3,									
	ASYNC_PRIORITY_4,									
	ASYNC_PRIORITY_5,									
	ASYNC_PRIORITY_6,									
	ASYNC_PRIORITY_7,										// highest priority

	NUM_ASYNC_PRIORITIES,

	ASYNC_PRIORITY_AI_DEFINITIONS		= ASYNC_PRIORITY_7,
	ASYNC_PRIORITY_APPEARANCES			= ASYNC_PRIORITY_6,
	ASYNC_PRIORITY_EFFECTS				= ASYNC_PRIORITY_6,
	ASYNC_PRIORITY_HAVOK_SHAPES			= ASYNC_PRIORITY_5,
	ASYNC_PRIORITY_ANIMATED_MODELS		= ASYNC_PRIORITY_5,
	ASYNC_PRIORITY_MODELS				= ASYNC_PRIORITY_5,
	ASYNC_PRIORITY_ANIMATIONS_MAX		= ASYNC_PRIORITY_4,
	ASYNC_PRIORITY_WARDROBE_MODELS		= ASYNC_PRIORITY_4,
	ASYNC_PRIORITY_PARTICLE_DEFINITIONS	= ASYNC_PRIORITY_3,
	ASYNC_PRIORITY_RAGDOLLS				= ASYNC_PRIORITY_2,
	ASYNC_PRIORITY_SOUNDS				= ASYNC_PRIORITY_2,
	ASYNC_PRIORITY_LOAD_READY			= ASYNC_PRIORITY_2,
	ASYNC_PRIORITY_ANIMATIONS_MIN		= ASYNC_PRIORITY_0,
	ASYNC_PRIORITY_MAX					= ASYNC_PRIORITY_7,

	ENGINE_FILE_PRIORITY_MATERIAL		= ASYNC_PRIORITY_6,
	ENGINE_FILE_PRIORITY_TEXTURE		= ASYNC_PRIORITY_6,
	ENGINE_FILE_PRIORITY_LIGHT			= ASYNC_PRIORITY_2,
	ENGINE_FILE_PRIORITY_SKYBOX			= ASYNC_PRIORITY_4,
	ENGINE_FILE_PRIORITY_ENVIRONMENT	= ASYNC_PRIORITY_6,
	ENGINE_FILE_PRIORITY_SCREENFX		= ASYNC_PRIORITY_3,
};


// ---------------------------------------------------------------------------
// ASYNC_DATA flags
// ---------------------------------------------------------------------------
enum ASYNC_DATA_FLAGS
{
	ADF_FREEBUFFER					= MAKE_MASK(0),			// free the data buffer after we're done
	ADF_FREEDATA					= MAKE_MASK(1),			// free the use data after we're done
	ADF_FILEIOCALLBACK_ONCANCEL		= MAKE_MASK(2),			// call m_fpFileIOThreadCallback even on failure to read or cancel 
	ADF_LOADINGCALLBACK_ONCANCEL	= MAKE_MASK(3),			// call m_fpLoadingThreadCallback even on failure to read or cancel (specifying this means that the loading thread callback need to be thread safe)
	ADF_CANCEL						= MAKE_MASK(4),			// operation is being canceld
	ADF_CLOSE_AFTER_READ			= MAKE_MASK(5),			// close the file after read attempt
	ADF_MALLOC_BIG					= MAKE_MASK(6),			// use MALLOCBIGFL()
};


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
struct ASYNC_EVENT;


// ---------------------------------------------------------------------------
// both request data and callback data
// ---------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
	#define ASYNC_DATA_FL_INIT( file, line )		,m_Dbg_file(file),m_Dbg_line(line)
	#define ASYNC_DATA_FL_PARAMS( file, line )		,const char * file,unsigned int line
#else
	#define ASYNC_DATA_FL_INIT( file, line )
	#define ASYNC_DATA_FL_PARAMS( file, line )
#endif

struct ASYNC_DATA
{
	// in
	ASYNC_FILE *				m_File;						// async file
	void *						m_Buffer;					// buffer w/ data from either read or write operation
	void **						m_BufferAddr;				// address of a pointer to patch if using delayed allocation
	unsigned int				m_BufferSize;				// size of buffer for delayed allocation
	UINT64						m_Offset;					// offset into physical file from which we began reading (don't confuse this with offset into file in pakfile)!
	unsigned int				m_Bytes;					// #of bytes for operation (m_Buffer must be at least this amount)
	DWORD						m_flags;					// various flags
	ASYNC_FILE_CALLBACK			m_fpFileIOThreadCallback;	// callback after operation is complete to be run in worker (file io) thread
	ASYNC_FILE_CALLBACK			m_fpLoadingThreadCallback;	// callback after operation is complete to be run in calling ("main") thread
	void *						m_UserData;					// user data for callbacks
	struct MEMORYPOOL *			m_Pool;						// memory pool buffer and data should be freed from

#if ISVERSION(DEVELOPMENT)
	const char *				m_Dbg_file;					// allocating file & line number
	unsigned int				m_Dbg_line;
#endif

	// out
	DWORD						m_IOSize;					// actual completed byte count
	UINT32						m_GentimeHigh;
	UINT32						m_GentimeLow;

	ASYNC_DATA(void) : 
		m_File(NULL), m_Buffer(NULL), m_Offset(0), m_Bytes(0), m_flags(0), 
		m_fpFileIOThreadCallback(NULL), m_fpLoadingThreadCallback(NULL), 
		m_UserData(NULL), m_Pool(NULL), m_IOSize(0), m_BufferAddr(NULL), 
		m_BufferSize(0) ASYNC_DATA_FL_INIT(NULL,0) {}

	ASYNC_DATA(ASYNC_FILE * file, void * buffer, UINT64 offset, unsigned int bytes) :
		m_File(file), m_Buffer(buffer), m_Offset(offset), m_Bytes(bytes), m_flags(0), 
			m_fpFileIOThreadCallback(NULL), m_fpLoadingThreadCallback(NULL), 
			m_UserData(NULL), m_Pool(NULL), m_IOSize(0), m_BufferAddr(NULL),
			m_BufferSize(0) ASYNC_DATA_FL_INIT(NULL,0) {}

	ASYNC_DATA(ASYNC_FILE * file, void * buffer, UINT64 offset, unsigned int bytes, DWORD flags) :
		m_File(file), m_Buffer(buffer), m_Offset(offset), m_Bytes(bytes), m_flags(flags), 
			m_fpFileIOThreadCallback(NULL), m_fpLoadingThreadCallback(NULL), m_UserData(NULL), 
			m_Pool(NULL), m_IOSize(0), m_BufferAddr(NULL), m_BufferSize(0)
			ASYNC_DATA_FL_INIT(NULL,0) {}

	ASYNC_DATA(ASYNC_FILE * file, void * buffer, UINT64 offset, unsigned int bytes, DWORD flags, 
		ASYNC_FILE_CALLBACK fpFileIOThreadCallback, ASYNC_FILE_CALLBACK	fpLoadingThreadCallback) :
		m_File(file), m_Buffer(buffer), m_Offset(offset), m_Bytes(bytes), m_flags(flags), 
			m_fpFileIOThreadCallback(fpFileIOThreadCallback), m_fpLoadingThreadCallback(fpLoadingThreadCallback), 
			m_UserData(NULL), m_Pool(NULL), m_IOSize(0), m_BufferAddr(NULL), 
			m_BufferSize(0) ASYNC_DATA_FL_INIT(NULL,0) {}

	ASYNC_DATA(ASYNC_FILE * file, void * buffer, UINT64 offset, unsigned int bytes, DWORD flags, 
		ASYNC_FILE_CALLBACK fpFileIOThreadCallback, ASYNC_FILE_CALLBACK	fpLoadingThreadCallback, 
		void * userData, struct MEMORYPOOL * pool ) :
			m_File(file), m_Buffer(buffer), m_Offset(offset), m_Bytes(bytes), m_flags(flags), 
			m_fpFileIOThreadCallback(fpFileIOThreadCallback), m_fpLoadingThreadCallback(fpLoadingThreadCallback), 
			m_UserData(userData), m_Pool(pool), m_IOSize(0), m_BufferAddr(NULL),
			m_BufferSize(0) ASYNC_DATA_FL_INIT(NULL,0) {}

	ASYNC_DATA(ASYNC_FILE * file, void * buffer, UINT64 offset, unsigned int bytes, DWORD flags, 
		ASYNC_FILE_CALLBACK fpFileIOThreadCallback, ASYNC_FILE_CALLBACK	fpLoadingThreadCallback, 
		void * userData, struct MEMORYPOOL * pool, void ** buffer_addr, unsigned int buffer_size
		ASYNC_DATA_FL_PARAMS(dbg_file, dbg_line) ) :
		m_File(file), m_Buffer(buffer), m_Offset(offset), m_Bytes(bytes), m_flags(flags), 
		m_fpFileIOThreadCallback(fpFileIOThreadCallback), m_fpLoadingThreadCallback(fpLoadingThreadCallback), 
		m_UserData(userData), m_Pool(pool), m_IOSize(0), m_BufferAddr(buffer_addr), m_BufferSize(buffer_size)
		ASYNC_DATA_FL_INIT(dbg_file,dbg_line) {}
};

struct ASYNC_EVENT;
// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------

inline void AsyncFileSetProcessLoopCallback( void (*pfn_Callback)() )
{
	extern void (*gpfn_AsyncFileProcessLoopCallback)();
	gpfn_AsyncFileProcessLoopCallback = pfn_Callback;
}

#if ISVERSION(DEVELOPMENT)
#define PrintSystemError(ec)		PrintSystemErrorFL(ec, __FILE__, __LINE__)
int PrintSystemErrorFL(
	DWORD errorCode,
	const char * file,
	unsigned int line);
#else
#define PrintSystemError(x)			0
#endif

BOOL AsyncFileGetRootDirectory(
	OS_PATH_CHAR * strRootDirectory,
	unsigned int & lenRootDirectory);

BOOL AsyncFileSystemInit(
	unsigned int * pnPriorityThresholds = NULL,
	unsigned int nPriorityThresholdCount = 0, WCHAR* strRootDirectoryOverride = NULL );

void AsyncFileSystemFree(
	void);
	
DWORD AsyncFileSystemGetCurrentOperationCount(
	void);

DWORD AsyncFileSystemGetPendingOperationCount(
	void);
	
DWORD AsyncFileSystemGetFinishedOperationCount(
	void);
	
DWORD AsyncFileRegisterThread(
	void);

void AsyncFileProcessComplete(
	DWORD dwThreadId,
	BOOL bSkipProcessLoopCallback = FALSE );

ASYNC_FILE * AsyncFileOpen(
	const char * filename,
	DWORD flags);

ASYNC_FILE * AsyncFileOpen(
	const WCHAR * filename,
	DWORD flags);

void AsyncFileClose(
	ASYNC_FILE * file);

void AsyncFileCloseNow(
	ASYNC_FILE * file);

const OS_PATH_CHAR * AsyncFileGetFilename(
	ASYNC_FILE * file);

UINT64 AsyncFileGetSize(
	ASYNC_FILE * file);

UINT64 AsyncFileGetLastModifiedTime(
	ASYNC_FILE * file);

HANDLE AsyncFileGetHandle(
	ASYNC_FILE * file);

DWORD AsyncFileGetFlags(
	ASYNC_FILE* file);

BOOL AsyncFileRead(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId);

unsigned int AsyncFileReadNow(
	ASYNC_DATA & data);

BOOL AsyncFileWrite(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId);

unsigned int AsyncFileWriteNow(
	ASYNC_DATA & data);

BOOL AsyncWriteFile(
	const TCHAR * filename,
	const void * buffer,
	unsigned int bytes,
	UINT64 * gentime);

BOOL AsyncFilePostDummyRequest(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId);

BOOL AsyncFilePostRequestForCallback(
	ASYNC_DATA* pAsyncData);

unsigned int AsyncFileAdjustPriorityByDistance(
	unsigned int nPriority,
	float distance);

BOOL AsyncFileIsAsyncRead();

BOOL AsyncFileSetSynchronousRead(
	BOOL val);

BOOL AsyncFileSetSynchronousWrite(
	BOOL val);	

BOOL AsyncFilePostRequestForCallback(
	ASYNC_DATA * pAsyncData);

HANDLE AsyncEventGetHandle(
	ASYNC_EVENT* pEvent);

ASYNC_EVENT * AsyncFileGetEvent(
	void);

void AsyncFileRecycleEvent(
	ASYNC_EVENT * async_event);

BOOL AsyncFileTest(
	DWORD dwAsyncThreadId = -1);

#endif // _ASYNCFILE_H_
