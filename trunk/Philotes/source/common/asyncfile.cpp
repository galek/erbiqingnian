// ---------------------------------------------------------------------------
// FILE:	asynchio.cpp
// DESC:	io completion port based asynchronous file i/o library
//
// TODO:	tune loading thread count (MAX_LOADING_THREADS)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// INCLUDE
// ---------------------------------------------------------------------------

#include "appcommon.h"
#include "pakfile.h"

#include "queue.h"

#if ISVERSION(SERVER_VERSION)
#include "asyncfile.cpp.tmh"
#endif

#if ! ISVERSION(SERVER_VERSION)
#include "commontimer.h"
#endif
#include "performance.h"

using namespace FSCommon;

// ---------------------------------------------------------------------------
// DEBUGGING
// ---------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
#define ASYNC_FILE_DEBUG			1
#define FIFO_LIST_DEBUG				1
#define ASYNC_FILE_TRACE_1			1								// major traces
#define ASYNC_FILE_TRACE_2			0								// file open/close traces
#define ASYNC_FILE_TRACE_3			0								// all file operation traces
#define ASYNC_FILE_TRACE_4			0								// file refcount traces
#endif
#define FORCE_SYNCHRONOUS			FALSE							// TRUE = force synchronous file i/o


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define WORKER_THREAD_COUNT			2								// default # of worker threads
#define WORKER_PRIORITY				THREAD_PRIORITY_NORMAL			// worker thread priority
#define MAX_LOADING_THREADS			8								// maximum # of loading threads

#define	DEFAULT_LOW_THRESHOLD		1								// max number of outstanding file io ops open per priority
#define DEFAULT_HI_THRESHOLD		17								

#define MAX_ASYNCIO_BLOCK			(2 * MEGA)						// max async io block we can deal with at one time, if the buffer exceeds this, we split it up


// ---------------------------------------------------------------------------
// ENUMERATIONS
// ---------------------------------------------------------------------------
enum ASYNC_FILE_SYSTEM_STATE
{
	AFSS_SHUTDOWN,
	AFSS_READY,
};

enum IOType
{
	IO_ReadRequest,
	IO_Read,
	IO_ReadNow,
	IO_WriteRequest,
	IO_Write,
	IO_WriteNow,
	IO_Dummy,
};


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
// our custom file handle
struct ASYNC_FILE
{
	HANDLE					m_hFileHandle;					// os file handle
	HANDLE					m_hCloseEvent;					// trigger this event on close
	DWORD					m_dwFlags;						// flags used to open file
	OS_PATH_CHAR			m_szFileName[MAX_PATH];			// requires an extra copy?
	CRefCount				m_RefCount;						// refcount
	ASYNC_FILE *			m_pNext;						// next file
};

// autoclose class for asyncread
class ASYNCREAD_AUTOCLOSE
{
	ASYNC_FILE *			m_File;

public:
	ASYNCREAD_AUTOCLOSE(
		ASYNC_DATA * data)
	{
		m_File = NULL;
		ASSERT_RETURN(data);
		if (!TEST_MASK(data->m_flags, ADF_CLOSE_AFTER_READ))
		{
			return;
		}
		if (data->m_File && data->m_File->m_hFileHandle != INVALID_FILE)
		{
			m_File = data->m_File;
		}
	}
	void Close(
		void)
	{
		if (!m_File)
		{
			return;
		}
		if (m_File->m_hFileHandle != INVALID_FILE)
		{
			// Release the ref count on the file or close it 
			// immediately
			//
			AsyncFileClose(m_File);
		}
		m_File = NULL;
	}
	~ASYNCREAD_AUTOCLOSE(void)
	{
		Close();
	}
	void Release(
		void)
	{
		m_File = NULL;
	}
};


// events, used for event pool for blocking file i/o completion notifications
struct ASYNC_EVENT
{
	HANDLE							m_hEvent;						// os event handle
	ASYNC_EVENT *					m_pNext;
};


// async io request / overlap structure
struct ASYNC_REQUEST : OVERLAPPED
{
	IOType							m_eIoType;						// type of io operation
	ASYNC_DATA						m_AsyncData;					// async data structure with file, buffer, etc.
	unsigned int					m_nPriority;					// op priority
	DWORD							m_dwLoadingThreadId;			// thread id of the file io request issuer ("main" thread)
	HANDLE							m_hLoadCompleteEvent;			// event used to signal op complete for "now" requests

	// below are members used to chunk large buffers (> MAX_ASYNCIO_BLOCK)
	DWORD							m_bufoffs;						// offset into m_AsyncData.m_Buffer for this chunk
	DWORD							m_bufsize;						// size of the chunk

	ASYNC_REQUEST *					m_pNext;
};

// global async file system
struct ASYNC_FILE_SYSTEM
{
	HANDLE							m_hCompletionPort;							// completion port
	volatile LONG					m_eState;									// state of system
	volatile long					m_nThreadPoolCount;							// total number of worker threads desired
	volatile long					m_nCurrentThreads;							// total number of worker threads

	OS_PATH_CHAR					m_strRootDirectory[MAX_PATH];				// root directory
	unsigned int					m_lenRootDirectory;							// strlen of root directory

	unsigned int					m_nPriorityThreshold[NUM_ASYNC_PRIORITIES];	// thresholds to queuing requests
	volatile long					m_nCurrentOperations;						// counter for number of submitted file i/o operations

	CQueue<QB_LIFO, ASYNC_FILE> 	m_AsyncFileFreeList;						// pool of async_file
	volatile long					m_nAsyncFileAllocatedCount;					// count of ASYNC_FILE structures allocated

	CQueue<QB_LIFO, ASYNC_EVENT>	m_AsyncEventFreeList;
	volatile long					m_nAsyncEventAllocatedCount;				// count of ASYNC_EVENT structures allocated

	CQueue<QB_LIFO, ASYNC_REQUEST>	m_AsyncRequestFreeList;						// pool of async requests
	volatile long					m_nAsyncRequestAllocatedCount;				// count of ASYNC_REQUEST structures allocated

	volatile long					m_nPendingOperations;						// counter for number of pending operations
	CQueue<QB_FIFO, ASYNC_REQUEST>	m_PendingOperations[NUM_ASYNC_PRIORITIES];	// FIFO queues for pending operations (by priority) in case # of pending operations exceeds some maximum amount

	volatile long					m_nFinishedOperations;						// counter for number of finished operations
	CQueue<QB_FIFO, ASYNC_REQUEST>	m_FinishedOperations[MAX_LOADING_THREADS];	// list of FIFO queues for loading thread callback items

	volatile long					m_nRegisteredLoadingThreads;				// count of registered loading threads

	DWORD							m_dwLoadingThreadId[MAX_LOADING_THREADS];	// table linking real thread id with async thread id
	BOOL							m_bIsProcessing[MAX_LOADING_THREADS];		// processing callbacks
};


// ---------------------------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------------------------
BOOL								g_bSynchronousIORead = FORCE_SYNCHRONOUS;		// whether or not to do asynchronous file loading
BOOL								g_bSynchronousIOWrite = FORCE_SYNCHRONOUS;		// whether or not to do asynchronous file loading
ASYNC_FILE_SYSTEM					g_FS;
static CCriticalSection				g_csAsyncFile(TRUE);
static UINT32						g_iRefCount = 0;

void (*gpfn_AsyncFileProcessLoopCallback)() = NULL;

#define ASYNC_PRIORITY_INTERPOLATE(min, max, num, div)		(min + (max - min) * num / div)

unsigned int g_DefaultPriorityThresholds[] =
{
	DEFAULT_LOW_THRESHOLD,									// ASYNC_PRIORITY_0
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 1, 8),
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 2, 8),
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 3, 8),
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 4, 8),
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 5, 8),
	ASYNC_PRIORITY_INTERPOLATE(DEFAULT_LOW_THRESHOLD, DEFAULT_HI_THRESHOLD, 6, 8),
	DEFAULT_HI_THRESHOLD,									// ASYNC_PRIORITY_7
};


// ---------------------------------------------------------------------------
// STATIC FUNCTIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
int PrintSystemErrorFL(
	DWORD errorCode,
	const char * file,
	unsigned int line)
{
	char errorString[4096];
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, errorString, arrsize(errorString), NULL))
	{
		PStrCopy(errorString, "unknown error", arrsize(errorString));
	}

	trace("ERROR %d: %s  [FILE:%s  LINE:%d]\n", errorCode, errorString, file, line);
	return 0;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static const char * sAsyncFileGetIOTypeString(
	IOType iotype)
{
	static const char * sz_IOTypeUnknown = "??????";
#if !ISVERSION(DEVELOPMENT)
    UNREFERENCED_PARAMETER(iotype);
	return sz_IOTypeUnknown;
#else
	static const char * sz_IOTypeStrings[] =
	{
		"READ_ ",		// IO_ReadRequest
		"READ  ",		// IO_Read
		"READ* ",		// IO_ReadNow
		"WRITE ",		// IO_Write
		"WRITE_",		// IO_WriteRequest
		"WRITE*",		// IO_WriteNow
		"DUMMY ",		// IO_Dummy
	};
	ASSERT_DO(iotype >= 0 && iotype < arrsize(sz_IOTypeStrings))
	{
		return sz_IOTypeUnknown;
	}
	return sz_IOTypeStrings[iotype];
#endif
}



/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileGetFileStruct
//
// Returns
//	An ASYNC_FILE struct from the pool
//
// Remarks
//	Used to access the pool of ASYNC_FILE structures.
//	The ref count is set to 1.
//
/////////////////////////////////////////////////////////////////////////////
static ASYNC_FILE * AsyncFileGetFileStruct(
	void)
{
	ASYNC_FILE * file = g_FS.m_AsyncFileFreeList.Get();
	if (!file)
	{
		file = (ASYNC_FILE *)MALLOCZ(NULL, sizeof(ASYNC_FILE));
		ASSERT_RETNULL(file);
		InterlockedIncrement(&g_FS.m_nAsyncFileAllocatedCount);
	}
	file->m_RefCount.Init();
	file->m_hFileHandle = INVALID_HANDLE_VALUE;
	file->m_hCloseEvent = INVALID_HANDLE_VALUE;
	return file;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileRefFileStruct
//
// Parameters
//	file : A pointer to the ASYNC_FILE which will have its ref count increased
//
// Returns
//	TRUE if the function succeeds, FALSE if the ref count for the file has
//	not been intialized.
//
// Remarks
//	Increase the reference count on an ASYNC_FILE
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileRefFileStruct(
	ASYNC_FILE* asyncFile)
{
	if (!asyncFile)
	{
		return TRUE;
	}
	long ref = asyncFile->m_RefCount.RefAdd(CRITSEC_FUNCNOARGS_FILELINE());
	if (ref == 0)
	{
		return FALSE;
	}
#if ASYNC_FILE_TRACE_4
	trace("increment to %d\n", ref);
#endif
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileReleaseFileStruct
//
// Parameters
//	file : A pointer to the ASYNC_FILE which will have its ref count decreased
//
// Remarks
//	Decreases the ref count for the ASYNC_FILE, closing the file handle if it reaches zero.
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileReleaseFileStruct(
	ASYNC_FILE * file)
{
	if (!file)
	{
		return;
	}
	long ref = file->m_RefCount.RefDec(CRITSEC_FUNCNOARGS_FILELINE());
#if ASYNC_FILE_TRACE_4
	trace("decrement to %d\n", ref);
#endif
	if (ref == 0)
	{
#if ASYNC_FILE_TRACE_2
		// TraceDebugOnly logs this out to the log.
		// The problem comes if we're closing the log that we're writing to.
		// Thus, this was moved above the CloseHandle() call
		// BUT that still won't fix this problem.  If we close the log that this
		// goes into, and then close any OTHER logs, then we're still in trouble,
		// because closing those logs will try to write out to the log that we just closed.
		TraceDebugOnly("close file: %s", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(file->m_szFileName));
#endif

		file->m_RefCount.Free();
		ASSERT(CloseHandle(file->m_hFileHandle));		// can use GetLastError to figure out what the error was

		HANDLE closeEvent = file->m_hCloseEvent;

		memclear(file, sizeof(ASYNC_FILE));
		g_FS.m_AsyncFileFreeList.Put(file);

		if (closeEvent != INVALID_HANDLE_VALUE)
		{
			SetEvent(closeEvent);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileFreeFileStructPool
//
// Remarks
//	Releases all the memory for ASYNC_FILE structures in the pool
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileFreeFileStructPool(
	void)
{
	ASYNC_FILE * head = g_FS.m_AsyncFileFreeList.GetAll();
	while (ASYNC_FILE * curr = head)
	{
		head = curr->m_pNext;
		InterlockedDecrement(&g_FS.m_nAsyncFileAllocatedCount);
		FREE(NULL, curr);
	}
	ASSERT(g_FS.m_nAsyncFileAllocatedCount == 0);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileGetEvent
//
// Returns
//	An ASYNC_EVENT struct from the pool
//
// Remarks
//	Used to access the pool of ASYNC_EVENT structures.
//
/////////////////////////////////////////////////////////////////////////////
ASYNC_EVENT * AsyncFileGetEvent(
	void)
{
	ASYNC_EVENT * async_event = g_FS.m_AsyncEventFreeList.Get();
	if (!async_event)
	{
		HANDLE event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
		ASSERT_RETNULL(event_handle != NULL);

		async_event = (ASYNC_EVENT *)MALLOCZ(NULL, sizeof(ASYNC_EVENT));
		if (!async_event)
		{
			CloseHandle(event_handle);
			ASSERTX_RETNULL(0, "error allocating ASYNC_EVENT");
		}
		async_event->m_hEvent = event_handle;
		InterlockedIncrement(&g_FS.m_nAsyncEventAllocatedCount);
	}
	else
	{
		ASSERT(async_event->m_hEvent != NULL);
	}
	return async_event;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileRecycleEvent
//
// Parameters
//	async_event : A pointer to the event to release into the event pool
//
/////////////////////////////////////////////////////////////////////////////
void AsyncFileRecycleEvent(
	ASYNC_EVENT * async_event)
{
	ASSERT_RETURN(async_event);
	g_FS.m_AsyncEventFreeList.Put(async_event);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileFreeEventPool
//
// Remarks
//	Releases all the memory for ASYNC_EVENT structures in the pool
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileFreeEventPool(
	void)
{
	ASYNC_EVENT * head = g_FS.m_AsyncEventFreeList.GetAll();
	while (ASYNC_EVENT * curr = head)
	{
		head = curr->m_pNext;
		InterlockedDecrement(&g_FS.m_nAsyncEventAllocatedCount);
		ASSERT(curr->m_hEvent != NULL);
		CloseHandle(curr->m_hEvent);
		FREE(NULL, curr);
	}
	ASSERT(g_FS.m_nAsyncEventAllocatedCount == 0);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileGetRequestStruct
//
// Parameters
//	iotype : The type of IO to perform on the specified data
//	data : The data for the IO operation
//	priority : The priority for the IO operation
//	dwLoadingThreadId : The zero based index for the calling thread returned from
//		AsyncFileRegisterThread
//
// Returns
//	An ASYNC_REQUEST struct from the pool containing the given data for the specified
//	iotype
//
// Remarks
//	Used to access the pool of ASYNC_REQUEST structures.
//
/////////////////////////////////////////////////////////////////////////////
static ASYNC_REQUEST * AsyncFileGetRequestStruct(
	IOType iotype,
	ASYNC_DATA & data,
	unsigned int priority = ASYNC_PRIORITY_0,
	DWORD dwLoadingThreadId = 0)
{
	if (!AsyncFileRefFileStruct(data.m_File))
	{
		return NULL;
	}
	priority = PIN(priority, (unsigned int)ASYNC_PRIORITY_0, (unsigned int)(NUM_ASYNC_PRIORITIES - 1));
	ASSERT_RETFALSE(data.m_fpLoadingThreadCallback == NULL || dwLoadingThreadId < MAX_LOADING_THREADS);

	ASYNC_REQUEST * request = g_FS.m_AsyncRequestFreeList.Get();
	if (!request)
	{
		request = (ASYNC_REQUEST *)MALLOC(NULL, sizeof(ASYNC_REQUEST));
		ASSERT_RETNULL(request);
		InterlockedIncrement(&g_FS.m_nAsyncRequestAllocatedCount);
	}

#if ASYNC_FILE_TRACE_4
	ASSERT(data.m_File->m_hFileHandle != INVALID_HANDLE_VALUE);
	trace("AsyncFileGetRequestStruct:  IOTYPE: %s   REQUEST: %08x   FILE: %08x  %s\n", sAsyncFileGetIOTypeString(iotype), request, data.m_File, data.m_File->m_szFileName);
#endif
	memclear(request, sizeof(OVERLAPPED));
	request->m_eIoType = iotype;
	request->m_nPriority = priority;
	request->m_dwLoadingThreadId = dwLoadingThreadId;
	request->m_hLoadCompleteEvent = NULL;
	ASSERT_RETNULL(MemoryCopy(&request->m_AsyncData, sizeof(request->m_AsyncData), &data, sizeof(data)));
	request->m_bufoffs = 0;
	request->m_bufsize = 0;

	return request;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileReleaseData
//
// Parameters
//	data : A reference to the ASYNC_DATA given to the IO request
//
// Remarks
//	Releases both the user data and IO buffer if the appropriate flag is 
//	set
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileReleaseData(
	ASYNC_DATA & data)
{
	if (data.m_UserData && TEST_MASK(data.m_flags, ADF_FREEDATA))
	{
		FREE(data.m_Pool, data.m_UserData);
	}

	if (data.m_Buffer && TEST_MASK(data.m_flags, ADF_FREEBUFFER))
	{
		FREE(data.m_Pool, data.m_Buffer);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileReleaseRequestFile
//
// Parameters
//	request : A pointer to the request to release back to the pool
//
// Remarks
//	The request only gets released back to the pool if the ADF_CLOSE_AFTER_READ
//	flag has been set.
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileReleaseRequestFile(
	ASYNC_REQUEST * request)
{
	ASSERT_RETURN(request);
	if (!TEST_MASK(request->m_AsyncData.m_flags, ADF_CLOSE_AFTER_READ))
	{
		return;
	}
	if (request->m_AsyncData.m_File)
	{
#if ASYNC_FILE_TRACE_4
		trace("AsyncFileReleaseRequestFile:  IOTYPE: %s   REQUEST: %08x   FILE: %08x  %s\n", sAsyncFileGetIOTypeString(request->m_eIoType), request, request->m_AsyncData.m_File, request->m_AsyncData.m_File->m_szFileName);
#endif
		// Decref the file
		//
		AsyncFileReleaseFileStruct(request->m_AsyncData.m_File);

		// This will either close the file sync or decref the file again
		//
		AsyncFileClose(request->m_AsyncData.m_File);
		
		request->m_AsyncData.m_File = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileRecycleRequestStruct
//
// Parameters
//	request : A pointer to the request to release back to the pool
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileRecycleRequestStruct(
	ASYNC_REQUEST * request)
{
	ASSERT_RETURN(request);
	if (request->m_AsyncData.m_File)
	{
#if ASYNC_FILE_TRACE_4
		trace("AsyncFileRecycleRequestStruct:  IOTYPE: %s   REQUEST: %08x   FILE: %08x  %s\n", sAsyncFileGetIOTypeString(request->m_eIoType), request, request->m_AsyncData.m_File, request->m_AsyncData.m_File->m_szFileName);
#endif
		AsyncFileReleaseFileStruct(request->m_AsyncData.m_File);
	}

	AsyncFileReleaseData(request->m_AsyncData);

	g_FS.m_AsyncRequestFreeList.Put(request);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileFreeRequestPool
//
// Remarks
//	Frees the memory associated with all the ASYNC_REQUESTs in the pool
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileFreeRequestPool(
	void)
{
	ASYNC_REQUEST * head = g_FS.m_AsyncRequestFreeList.GetAll();
	while (ASYNC_REQUEST * curr = head)
	{
		head = curr->m_pNext;
		InterlockedDecrement(&g_FS.m_nAsyncRequestAllocatedCount);
		FREE(NULL, curr);
	}
	ASSERT(g_FS.m_nAsyncRequestAllocatedCount == 0);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncFileCancelOperation
//
// Parameters
//	request : The request which is being cancelled
//
// Remarks
//	Cancels the IO operation by calling its callback functions
//
/////////////////////////////////////////////////////////////////////////////
static void sAsyncFileCancelOperation(
	ASYNC_REQUEST * request)
{
	SET_MASK(request->m_AsyncData.m_flags, ADF_CANCEL);

	PRESULT pr = S_OK;

	if (TEST_MASK(request->m_AsyncData.m_flags, ADF_FILEIOCALLBACK_ONCANCEL) && request->m_AsyncData.m_fpFileIOThreadCallback)
	{
		V_SAVE( pr, request->m_AsyncData.m_fpFileIOThreadCallback(request->m_AsyncData) );
	}

	if (TEST_MASK(request->m_AsyncData.m_flags, ADF_LOADINGCALLBACK_ONCANCEL) && request->m_AsyncData.m_fpLoadingThreadCallback)
	{
		V_SAVE( pr, request->m_AsyncData.m_fpLoadingThreadCallback(request->m_AsyncData) );
	}

	AsyncFileRecycleRequestStruct(request);

	InterlockedDecrement(&g_FS.m_nPendingOperations);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileFreePendingOperations
//
// Remarks
//	Used when the system is shutting down.  It executes the callbacks of any pending
//	operations that haven't been executed yet.
//
//	All callbacks are executed from the context of the main thread.
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileFreePendingOperations(
	void)
{
	for (unsigned int ii = 0; ii < NUM_ASYNC_PRIORITIES; ++ii)
	{
		ASYNC_REQUEST * head = g_FS.m_PendingOperations[ii].GetAll();
		while (ASYNC_REQUEST * curr = head)
		{
			head = curr->m_pNext;
			sAsyncFileCancelOperation(curr);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileFreeFinishedOperations
//
// Remarks
//	Used when the system is shutting down.  It executes the callbacks of any completed
//	operations that haven't been handled yet.
//
//	All callbacks are executed from the context of the main thread.
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileFreeFinishedOperations(
	void)
{
	for (unsigned int ii = 0; ii < MAX_LOADING_THREADS; ++ii)
	{
		ASYNC_REQUEST * head = g_FS.m_FinishedOperations[ii].GetAll();
		while (ASYNC_REQUEST * curr = head)
		{
			head = curr->m_pNext;
			sAsyncFileCancelOperation(curr);

			InterlockedDecrement(&g_FS.m_nFinishedOperations);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncChunkRequest
//
// Parameters
//	request : A pointer to the IO request that will be used when determining
//		if chunking is necessary.
//	buffer : A reference to a buffer that will be set to point in the 
//		request buffer where the next operation needs to begin.
//	bytes : A reference to a variable that will contain the size of the next
//		operation.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
// Remarks
//	If the size of the requested operation exceeds a threshold, the
//	operation is broken into chunks.  This method extracts the chunk
//	buffer location and size from the request.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL sAsyncChunkRequest(
	ASYNC_REQUEST * request,
	void * & buffer,
	unsigned int & bytes)
{
	ASSERT_RETFALSE(request);

	// If the requested bytes for the operation is within the threshold,
	// issue a single IO operation to satisfy the request
	//
	if (request->m_AsyncData.m_Bytes <= MAX_ASYNCIO_BLOCK)
	{
		request->m_bufoffs = 0;
		request->m_bufsize = request->m_AsyncData.m_Bytes;
		buffer = request->m_AsyncData.m_Buffer;
		bytes = request->m_AsyncData.m_Bytes;

		request->Offset = (DWORD)(request->m_AsyncData.m_Offset);
		request->OffsetHigh = (DWORD)(request->m_AsyncData.m_Offset >> (UINT64)32);
		return TRUE;
	}
	// If the remaining bytes requested in operation exceeds the threshold,
	// extract the buffer and bytes for the next chunk operation
	//
	else
	{
		// m_bufoffs will point to the point in the buffer where the operation will begin.
		// m_bufsize is initially zero before the first chunk operation and then will contain
		// the last operation's bytes.
		//
		request->m_bufoffs += request->m_bufsize;

		// Make sure that the operation start point hasn't exceeded the total operation size
		//
		ASSERT_RETFALSE(request->m_AsyncData.m_Bytes > request->m_bufoffs);

		// Calculate how many bytes are remaining for the operation
		//
		unsigned int bytesleft = request->m_AsyncData.m_Bytes - request->m_bufoffs;

		// Figure out how many bytes the current chunk operation will be for
		//
		request->m_bufsize = MIN((unsigned int)MAX_ASYNCIO_BLOCK, bytesleft);

		// Calculate the parameters needed for the operation
		//
		buffer = (BYTE *)request->m_AsyncData.m_Buffer + request->m_bufoffs;
		bytes = request->m_bufsize;

		// Set the OVERLAPPED offset from the check operation offset
		//
		UINT64 fileoffset = request->m_AsyncData.m_Offset + (UINT64)request->m_bufoffs;
		request->Offset = (DWORD)(fileoffset);
		request->OffsetHigh = (DWORD)(fileoffset >> (UINT64)32);
		
		return TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAllocateBufferIfNeeded
//
// Parameters
//	data : A reference to the async data
//
// Returns
//	S_OK if a buffer was allocated, S_FALSE if not, E_FAIL if error.
//
// Remarks
//	Allocates a buffer as needed to delay allocation until the buffer is
//  actually needed (e.g. when the file is actually being read).
//
/////////////////////////////////////////////////////////////////////////////

static PRESULT sAllocateBufferIfNeeded( 
	ASYNC_DATA & data )
{
	// Allocate buffer here if it doesn't already exist.
	if ( !data.m_Buffer )
	{
		ASSERT_RETFAIL( data.m_BufferSize > 0 );
#if ISVERSION(DEVELOPMENT)
		if( data.m_flags & ADF_MALLOC_BIG )
		{
			data.m_Buffer = MALLOCBIGFL( data.m_Pool, data.m_BufferSize + 1, data.m_Dbg_file, data.m_Dbg_line );
		}
		else
		{
			data.m_Buffer = MALLOCFL( data.m_Pool, data.m_BufferSize + 1, data.m_Dbg_file, data.m_Dbg_line );
		}
#else
		data.m_Buffer = MALLOC( data.m_Pool, data.m_BufferSize + 1);
#endif
		ASSERT_RETFAIL( data.m_Buffer );
		// Patch the buffer pointer if passed in.
		if ( data.m_BufferAddr )
			*data.m_BufferAddr = data.m_Buffer;
		((BYTE*)data.m_Buffer)[data.m_BufferSize] = 0;		// 0 terminate for text files

		return S_OK;
	}

	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncFileRead
//
// Parameters
//	request : A pointer to the IO request to be issued
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
// Remarks
//	Issues the ReadFile operation to the device.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL sAsyncFileRead(
	ASYNC_REQUEST * request)
{
	ASSERT_RETFALSE(request);

	ASYNCREAD_AUTOCLOSE autoclose(&request->m_AsyncData);

	ASSERT_RETFALSE(request->m_AsyncData.m_File);
	ASSERT_RETFALSE(request->m_AsyncData.m_File->m_hFileHandle != INVALID_HANDLE_VALUE);
	ASSERT_RETFALSE(request->m_AsyncData.m_Bytes != 0);

	// because ReadFile might act synchronously even if the file is opened OVERLAPPED, let's make the actual readfile call on a different thread
	if (request->m_eIoType == IO_ReadRequest)
	{
		PostQueuedCompletionStatus(g_FS.m_hCompletionPort, 0, (DWORD_PTR)0, request);
		autoclose.Release();
		return TRUE;
	}

	sAllocateBufferIfNeeded( request->m_AsyncData );
	ASSERT_RETFALSE(request->m_AsyncData.m_Buffer);

	void * buffer = NULL;
	unsigned int bytes = 0;
	if (!sAsyncChunkRequest(request, buffer, bytes))
	{
		AsyncFileRecycleRequestStruct(request);
		return FALSE;
	}

	{
		if (ReadFile(request->m_AsyncData.m_File->m_hFileHandle, buffer, bytes, NULL, request))
		{
			autoclose.Release();
			return TRUE;
		}
	}
	DWORD dwError = GetLastError();
	switch (dwError)
	{
	case ERROR_IO_PENDING:
		autoclose.Release();
		return TRUE;
	default:
		ASSERT(PrintSystemError(dwError));
		AsyncFileRecycleRequestStruct(request);
		break;
	}

	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncFileWrite
//
// Parameters
//	request : A pointer to the IO request to be issued
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
// Remarks
//	Issues the WriteFile operation to the device.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL sAsyncFileWrite(
	ASYNC_REQUEST * request)
{
	ASSERT_RETFALSE(request);
	ASSERT_RETFALSE(request->m_AsyncData.m_File);
	ASSERT_RETFALSE(request->m_AsyncData.m_File->m_hFileHandle != INVALID_HANDLE_VALUE);
	ASSERT_RETFALSE(request->m_AsyncData.m_Buffer);
	ASSERT_RETFALSE(request->m_AsyncData.m_Bytes != 0);

	// because WriteFile might act synchronously even if the file is opened OVERLAPPED, let's make the actual writefile call on a different thread
	if (request->m_eIoType == IO_WriteRequest)
	{
		PostQueuedCompletionStatus(g_FS.m_hCompletionPort, 0, (DWORD_PTR)0, request);
		return TRUE;
	}

	void * buffer = NULL;
	unsigned int bytes = 0;
	if (!sAsyncChunkRequest(request, buffer, bytes))
	{
		AsyncFileRecycleRequestStruct(request);
		return FALSE;
	}

	if (WriteFile(request->m_AsyncData.m_File->m_hFileHandle, buffer, bytes, NULL, request))
	{
		return TRUE;
	}
	DWORD dwError = GetLastError();
	switch (dwError)
	{
	case ERROR_IO_PENDING:
		return TRUE;
	default:
		ASSERT(PrintSystemError(dwError));
		AsyncFileRecycleRequestStruct(request);
		break;
	}

	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncDummy
//
// Parameters
//	request : A pointer to the IO request to be issued
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
// Remarks	
//	A "Dummy" request doesn't do any file IO, but will
//	execute the loading thread callback from the context of the
//	worker thread.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL sAsyncDummy(
	ASYNC_REQUEST * request)
{
	ASSERT_RETFALSE(request);
	if (PostQueuedCompletionStatus(g_FS.m_hCompletionPort, 0, 0, request))
	{
		return TRUE;
	}
	DWORD dwError = GetLastError();
	switch (dwError)
	{
	case ERROR_IO_PENDING:
		return TRUE;
	default:
		ASSERT(PrintSystemError(dwError));
		AsyncFileRecycleRequestStruct(request);
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncfileIOComplete_ChunkRead
//
// Parameters
//	request : A pointer to the IO request that just completed
//
// Returns
//	TRUE if the next chunk in the IO request was issued.
//	FALSE if the entire IO request has been satisfied or
//	if there was a failure while attempting to issue the next
//	chunk read operation
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_ChunkRead(
	ASYNC_REQUEST * request)
{
	if (request->m_bufoffs + request->m_bufsize >= request->m_AsyncData.m_Bytes)
	{
		return FALSE;
	}
	memclear(request, sizeof(OVERLAPPED));
	if (!sAsyncFileRead(request))
	{
		return FALSE;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncfileIOComplete_ChunkWrite
//
// Parameters
//	request : A pointer to the IO request that just completed
//
// Returns
//	TRUE if the next chunk in the IO request was issued.
//	FALSE if the entire IO request has been satisfied or
//	if there was a failure while attempting to issue the next
//	chunk read operation
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_ChunkWrite(
	ASYNC_REQUEST * request)
{
	if (request->m_bufoffs + request->m_bufsize >= request->m_AsyncData.m_Bytes)
	{
		return FALSE;
	}
	memclear(request, sizeof(OVERLAPPED));
	if (!sAsyncFileWrite(request))
	{
		return FALSE;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_ReadRequest
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_ReadRequest(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_ReadRequest()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		AsyncFileReleaseRequestFile(request);
		AsyncFileRecycleRequestStruct(request);
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
	}
	else
	{
		request->m_eIoType = IO_Read;
		sAsyncFileRead(request);
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_Read
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_Read(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_Read()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		goto _done;
	}

	PRESULT pr = S_OK;

#if ASYNC_FILE_TRACE_3
	trace("READ  FILE:%s  OFFS:%I64d  BYTES:%d\n", request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif

	// Check to see if there is another chunk of the read request
	// that needs to be issued.  If so and the read operation was
	// successfully initiated, just return
	//
	if (AsyncFileIOComplete_ChunkRead(request))
	{
		return TRUE;
	}

	// Decompress the file if it's a compressed format
	PakFileDecompressFile(&request->m_AsyncData);

_done:

	// Release the reference on the async file
	//
	AsyncFileReleaseRequestFile(request);

	// execute the fileio thread callback if it has one
	if (request->m_AsyncData.m_fpFileIOThreadCallback)
	{
		V_SAVE(pr, request->m_AsyncData.m_fpFileIOThreadCallback(request->m_AsyncData));
	}

	// enqueue the loading thread callback if it has one
	if (request->m_AsyncData.m_fpLoadingThreadCallback)
	{
		ASSERT_GOTO(request->m_dwLoadingThreadId < MAX_LOADING_THREADS, _recycle);
		g_FS.m_FinishedOperations[request->m_dwLoadingThreadId].Put(request);
		
		InterlockedIncrement(&g_FS.m_nFinishedOperations);
	}
	else
	{
_recycle:

		// Release the ASYNC_REQUEST back into the pool now
		// that the requested operation has completed and all
		// callbacks have been fired
		//
		AsyncFileRecycleRequestStruct(request);
	}

	InterlockedDecrement(&g_FS.m_nCurrentOperations);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_ReadNow
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_ReadNow(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_ReadNow()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		goto _done;
	}

#if ASYNC_FILE_TRACE_3
	trace("READ  FILE:%s  OFFS:%I64d  BYTES:%d\n", request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif

	// Check to see if there is another chunk of the read request
	// that needs to be issued.  If so and the read operation was
	// successfully initiated, just return
	//
	if (AsyncFileIOComplete_ChunkRead(request))
	{
		return TRUE;
	}

_done:
	// Decompress the file if it's a compressed format
	PakFileDecompressFile(&request->m_AsyncData);

	InterlockedDecrement(&g_FS.m_nCurrentOperations);
	
	// signal the loading thread that read is complete
	HANDLE hEvent = request->m_hLoadCompleteEvent;
	ASSERT_RETFALSE(hEvent);
	SetEvent(hEvent);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_WriteRequest
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_WriteRequest(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_WriteRequest()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		AsyncFileRecycleRequestStruct(request);
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
	}
	else
	{
		request->m_eIoType = IO_Write;
		sAsyncFileWrite(request);
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_Write
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_Write(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_Write()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		goto _done;
	}

	PRESULT pr = S_OK;

#if ASYNC_FILE_TRACE_3
	trace("WRITE  FILE:%s  OFFS:%I64d  BYTES:%d\n", request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif

	// Check to see if there is another chunk of the write request
	// that needs to be issued.  If so and the write operation was
	// successfully initiated, just return
	//
	if (AsyncFileIOComplete_ChunkWrite(request))
	{
		return TRUE;
	}

_done:
	// execute the fileiothread callback if it has one
	if (request->m_AsyncData.m_fpFileIOThreadCallback)
	{
		V_SAVE(pr, request->m_AsyncData.m_fpFileIOThreadCallback(request->m_AsyncData));
	}

	// enqueue the loading thread callback if it has one
	if (request->m_AsyncData.m_fpLoadingThreadCallback)
	{
		ASSERT_GOTO(request->m_dwLoadingThreadId < MAX_LOADING_THREADS, _recycle);
		g_FS.m_FinishedOperations[request->m_dwLoadingThreadId].Put(request);
		
		InterlockedIncrement(&g_FS.m_nFinishedOperations);
	}
	else
	{
_recycle:
		AsyncFileRecycleRequestStruct(request);
	}

	InterlockedDecrement(&g_FS.m_nCurrentOperations);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_WriteNow
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_WriteNow(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	if (dwIOError != 0)
	{
#if ISVERSION(DEVELOPMENT)
		char errorstr[4096];
		GetErrorString(dwIOError, errorstr, arrsize(errorstr));
		ASSERTV_MSG("AsyncFileIOComplete_WriteNow()\nSYSTEM ERROR (%08x): %s\nFILE:%s  OFFS:%I64d  BYTES:%d\n", dwIOError, errorstr, 
			request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif
		goto _done;
	}

#if ASYNC_FILE_TRACE_3
	trace("WRITE  FILE:%s  OFFS:%I64d  BYTES:%d\n", request->m_AsyncData.m_File->m_szFileName, request->m_AsyncData.m_Offset + request->m_bufoffs, request->m_AsyncData.m_IOSize);
#endif

	// Check to see if there is another chunk of the write request
	// that needs to be issued.  If so and the write operation was
	// successfully initiated, just return
	//
	if (AsyncFileIOComplete_ChunkWrite(request))
	{
		return TRUE;
	}

_done:
	InterlockedDecrement(&g_FS.m_nCurrentOperations);

	HANDLE hEvent = request->m_hLoadCompleteEvent;

	// signal the loading thread that write is complete
	ASSERT_RETFALSE(hEvent);
	SetEvent(hEvent);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete_Dummy
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
// Remarks
//	"Dummy" requests don't actually issue any IO operations
//	but occupy pending and finished operation slots as well
//	as use the normal callback mechanism.  This allows any 
//	processing to happen in the worker thread context via
//	the FileIOThreadCallback.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete_Dummy(
	ASYNC_REQUEST * request,
	DWORD dwIOError)
{
	ASSERT(dwIOError == 0);

	PRESULT pr = S_OK;

	// execute the file iothread callback if it has one
	if (request->m_AsyncData.m_fpFileIOThreadCallback)
	{
		V_SAVE( pr, request->m_AsyncData.m_fpFileIOThreadCallback(request->m_AsyncData) );
	}

	// enqueue the loading thread callback if it has one
	if (request->m_AsyncData.m_fpLoadingThreadCallback)
	{
		ASSERT_GOTO(request->m_dwLoadingThreadId < MAX_LOADING_THREADS, _recycle);
		g_FS.m_FinishedOperations[request->m_dwLoadingThreadId].Put(request);
		
		InterlockedIncrement(&g_FS.m_nFinishedOperations);
	}
	else
	{
_recycle:
		AsyncFileRecycleRequestStruct(request);
	}

	InterlockedDecrement(&g_FS.m_nCurrentOperations);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileIOComplete
//
// Parameters
//	request : A pointer to the IO request that just completed
//	dwIOError : The error code returned by GetLastError if the 
//		IO operation failed.  Will be 0 if no error.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileIOComplete(
	ASYNC_REQUEST * request, 
	DWORD dwIOError)
{
	ASSERT_RETFALSE(request);
	switch (request->m_eIoType)
	{
	case IO_ReadRequest:
		return AsyncFileIOComplete_ReadRequest(request, dwIOError);
	case IO_Read:
		return AsyncFileIOComplete_Read(request, dwIOError);
	case IO_ReadNow:
		return AsyncFileIOComplete_ReadNow(request, dwIOError);
	case IO_WriteRequest:
		return AsyncFileIOComplete_WriteRequest(request, dwIOError);
	case IO_Write:
		return AsyncFileIOComplete_Write(request, dwIOError);
	case IO_WriteNow:
		return AsyncFileIOComplete_WriteNow(request, dwIOError);
	case IO_Dummy:
		return AsyncFileIOComplete_Dummy(request, dwIOError);
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileDoPending
//
// Remarks
//	Called from the WorkerThread, this function checks the pending queue
// 	for ASYNC_REQUESTS and issues them.
//
//	The purpose of the priority threshold system is to make sure there is 
//	always "room" for a higher priority IO request if it comes in, so if there are
//	a bunch of low priority requests it won't fill up the queue of outstanding 
// 	operations.
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileDoPending(
	void)
{
	if (g_FS.m_eState == AFSS_SHUTDOWN)
	{
		return;
	}

	unsigned int current_operations = InterlockedIncrement(&g_FS.m_nCurrentOperations);

	// start with high priority and work back
	for (int ii = NUM_ASYNC_PRIORITIES - 1; ii >= ASYNC_PRIORITY_0; --ii)
	{
		unsigned int threshold = g_FS.m_nPriorityThreshold[ii];
		if (current_operations > threshold)
		{
			break;
		}
		ASYNC_REQUEST * request = g_FS.m_PendingOperations[ii].Get();
		if (!request)
		{
			continue;
		}
		InterlockedDecrement(&g_FS.m_nPendingOperations);
		switch (request->m_eIoType)
		{
		case IO_ReadRequest:
			request->m_eIoType = IO_Read;
			if (!sAsyncFileRead(request))
			{
				break;
			}
			return;
		case IO_Read:
			if (!sAsyncFileRead(request))
			{
				break;
			}
			return;
		case IO_WriteRequest:
			request->m_eIoType = IO_Write;
			if (!sAsyncFileWrite(request))
			{
				break;
			}
			return;
		case IO_Write:
			if (!sAsyncFileWrite(request))
			{
				break;
			}
			return;
		case IO_Dummy:
			if (!sAsyncDummy(request))
			{
				break;
			}
			return;
		default:
			ASSERTX(0, "invalid io type for request in pending queue.");
		}
	}
	InterlockedDecrement(&g_FS.m_nCurrentOperations);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	WorkerThread
//
// Parameters
//	param : A context pointer passed in from the thread creator.  In this case
//		it is the thread index.
//
// Remarks
//	Called from the WorkerThread, this function checks the pending queue
// 	for ASYNC_REQUESTS and issues them.
//
//	The purpose of the priority threshold system is to make sure there is 
//	always "room" for a higher priority IO request if it comes in, so if there are
//	a bunch of low priority requests it won't fill up the queue of outstanding 
// 	operations.
//
/////////////////////////////////////////////////////////////////////////////
static DWORD WINAPI WorkerThread(
	LPVOID param)    
{
	REF(param);

	HANDLE hCompletionPort = g_FS.m_hCompletionPort;

	int threadId = InterlockedIncrement(&g_FS.m_nCurrentThreads);

	char threadName[256];
	PStrPrintf(threadName, 256, "async_%02d", threadId);

#if ASYNC_FILE_DEBUG
	SetThreadName(threadName, (DWORD)-1);
#endif
#if ASYNC_FILE_TRACE_1
	trace("begin worker thread [%s]\n", threadName);
#endif

	while (TRUE)
	{

		ULONG_PTR completion_key;										// get a completed io request
		ASYNC_REQUEST * request = NULL;
		DWORD dwIOSize;
		DWORD dwIOError = 0;
		BOOL bIORet = GetQueuedCompletionStatus(hCompletionPort, &dwIOSize, (PULONG_PTR)&completion_key, (LPOVERLAPPED *)&request, INFINITE);

		// The thread is signaled to shutdown by sending it a completion port message with a NULL overlapped 
		//
		if (request == NULL)											// shutdown
		{
			ASSERT(g_FS.m_eState == AFSS_SHUTDOWN);
			break;														// exit thread
		}

		// Keep track of the number of bytes completed for the IO operation
		//
		request->m_AsyncData.m_IOSize += dwIOSize;

		if (!bIORet)
		{
			PrintSystemError(dwIOError = GetLastError());
		}

		// Handle the completed IO operation which could issue another
		// operation if the current request has not been fully satisfied
		//
		if (!AsyncFileIOComplete(request, dwIOError))
		{
			ASSERT(g_FS.m_eState == AFSS_SHUTDOWN);
			break;
		}

		// see if there's any jobs on the pending queues that we could do
		AsyncFileDoPending();
	}

#if ASYNC_FILE_TRACE_1
	trace("exit worker thread [%s]\n", threadName);
#endif

	InterlockedDecrement(&g_FS.m_nCurrentThreads);

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileSpawnWorkerThread
//
// Parameters
//	index : The index that will be associated with the new thread
//
// Remarks
//	Creates a worker thread that will service completed IO requests associated with
//	the completion port for the async file system.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFileSpawnWorkerThread(
	unsigned int index)
{
	DWORD dwThreadId;
	HANDLE hWorker = (HANDLE)CreateThread(NULL,	0, WorkerThread, CAST_TO_VOIDPTR(index), 0, &dwThreadId);
    if (hWorker == NULL)
	{
		ASSERT_RETFALSE(0);
    }
	SetThreadPriority(hWorker, WORKER_PRIORITY);
	CloseHandle(hWorker);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileKillWorkerThreads
//
// Remarks
//	Waits for all outstanding IO operations to complete before shutting down 
//	all worker threads
//
/////////////////////////////////////////////////////////////////////////////
static void AsyncFileKillWorkerThreads(
	void)
{
	// wait for all threads to finish creating
	for (unsigned int ii = 0; ii < 300; ++ii)
	{
		if (g_FS.m_nCurrentThreads == g_FS.m_nThreadPoolCount)
		{
			break;
		}
		Sleep(10);
	}
	if (g_FS.m_nCurrentThreads != g_FS.m_nThreadPoolCount)
	{
		trace("AsyncFileKillWorkerThreads() -- shutting down threads before all threads have been initialized!\n");
	}

	// wait for pending io to finish
	{
		Sleep(100);

		unsigned int wait_iter = 0;
		while (g_FS.m_nCurrentOperations > 0 && wait_iter++ < 300)
		{
			Sleep(10);
		}
	}
	if (g_FS.m_nCurrentOperations > 0)
	{
		trace("AsyncFileKillWorkerThreads() -- shutting down threads while operations are pending!\n");
	}

	// shut down worker threads
	unsigned int thread_count = g_FS.m_nCurrentThreads;
	for (unsigned int ii = 0; ii < thread_count; ii++)
	{
		PostQueuedCompletionStatus(g_FS.m_hCompletionPort, 0, (DWORD_PTR)0, NULL);
	}

	// wait until all worker threads exit
	{
		unsigned int wait_count = 0;
		while (g_FS.m_nCurrentThreads && wait_count < 100)
		{
			Sleep(50);
			wait_count++;
		}
		ASSERT(wait_count < 100);
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static int AsyncFilePendRequestCompare(
	void * a,
	void * b)
{
	ASYNC_REQUEST * A = (ASYNC_REQUEST *)a;
	ASYNC_REQUEST * B = (ASYNC_REQUEST *)b;
	ASSERT_RETZERO(A && B);
	if (A->m_AsyncData.m_File < B->m_AsyncData.m_File)
	{
		return -1;
	}
	else if (A->m_AsyncData.m_File > B->m_AsyncData.m_File)
	{
		return 1;
	}
	if (A->m_AsyncData.m_Offset < B->m_AsyncData.m_Offset)
	{
		return -1;
	}
	else if (A->m_AsyncData.m_Offset > B->m_AsyncData.m_Offset)
	{
		return 1;
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFilePendRequest
//
// Parameters
//	request : A pointer to the IO request to check against.  
//
// Returns
//	FALSE if the IO operation should be issued immediately, TRUE if the request has
//	been queued.
//
// Remarks
//	This method gets called before any IO operation is issued on the caller thread.  If the
//	current outstanding requests exceeds the threshold for the argument request's priority,
//	the request will be queued.
//
//	This function also increments the count of outstanding operations if it returns FALSE.
//
/////////////////////////////////////////////////////////////////////////////
static BOOL AsyncFilePendRequest(
	ASYNC_REQUEST * request)
{
	// Since there could be concurrent calls to this function, increment the current operations first
	// to error on the conservative side and issue less operations if this happens
	//
	unsigned int current_operations = InterlockedIncrement(&g_FS.m_nCurrentOperations);

	// Check to see if the we would be over the threshold if we issued the current request 
	// immediately
	//
	ASSERT_RETFALSE(request->m_nPriority < NUM_ASYNC_PRIORITIES);
	unsigned int threshold = g_FS.m_nPriorityThreshold[request->m_nPriority];
	if (current_operations <= threshold)
	{
		return FALSE;
	}

	// We were over the threshold, so revert the operation count change
	//
	InterlockedDecrement(&g_FS.m_nCurrentOperations);

	// Add the operation to the pending operation queue
	//
	InterlockedIncrement(&g_FS.m_nPendingOperations);
//	g_FS.m_PendingOperations[request->m_nPriority].PutInOrder(request, AsyncFilePendRequestCompare);  // << tested slower
	g_FS.m_PendingOperations[request->m_nPriority].Put(request);

	return TRUE;
}


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL AsyncFileIsAsyncRead()
{
	return !g_bSynchronousIORead;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL AsyncFileSetSynchronousRead(
	BOOL val)
{
	g_bSynchronousIORead = val;
	return g_bSynchronousIORead;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL AsyncFileSetSynchronousWrite(
	BOOL val)
{
	g_bSynchronousIOWrite = val;
	return g_bSynchronousIOWrite;
}

// ---------------------------------------------------------------------------
// free file system
// todo: free all pools
// ---------------------------------------------------------------------------
static void sAsyncFileSystemFreeHelper(
	void)
{
	g_FS.m_eState = AFSS_SHUTDOWN;

	AsyncFileKillWorkerThreads();

	if (g_FS.m_hCompletionPort != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_FS.m_hCompletionPort);
		g_FS.m_hCompletionPort = INVALID_HANDLE_VALUE;
	}

	// free pending ops
	AsyncFileFreePendingOperations();
	AsyncFileFreeFinishedOperations();
	AsyncFileFreeRequestPool();
	AsyncFileFreeEventPool();
	AsyncFileFreeFileStructPool();

	structclear(g_FS);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AsyncFileSystemFree(
	void)
{
	CSAutoLock autoLock(&g_csAsyncFile);
	g_iRefCount--;
	if (g_iRefCount == 0) 
	{
		sAsyncFileSystemFreeHelper();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD AsyncFileSystemGetCurrentOperationCount(
	void)
{
	return g_FS.m_nCurrentOperations;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD AsyncFileSystemGetPendingOperationCount(
	void)
{
	return g_FS.m_nPendingOperations;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD AsyncFileSystemGetFinishedOperationCount(
	void)
{
	return g_FS.m_nFinishedOperations;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncFileGetRootDirectory(
	OS_PATH_CHAR * strRootDirectory,
	unsigned int & lenRootDirectory)
{
	unsigned nSize = lenRootDirectory;
	if (c_GetToolMode())
	{
		if (!PGetModuleDirectory(strRootDirectory, nSize))
		{
			return FALSE;
		}
		lenRootDirectory = MIN(nSize - 1, PStrLen(strRootDirectory));
	}
	else
	{
		ASSERT_RETFALSE(nSize >= MAX_PATH);
		int len = 0;
		const OS_PATH_CHAR * str = AppCommonGetRootDirectory(&len);
		ASSERT_RETFALSE(str != NULL && len > 0 && len < MAX_PATH);
		PStrCopy(strRootDirectory, str, nSize);
		lenRootDirectory = len;
	}
	strRootDirectory[lenRootDirectory] = 0;
	PStrLower(strRootDirectory, lenRootDirectory + 1);	// CHB 2007.01.11 - Is there really a benefit to this?
	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static BOOL sAsyncFileSystemInitHelper(
	void)
{
	if (g_iRefCount > 0) 
	{
		g_iRefCount++;
		return TRUE;
	}

	sAsyncFileSystemFreeHelper();
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileSystemInit
//
// Parameters
//	pnPriorityThresholds : [optional] A pointer to an array of priorities that will be assigned to the 
//		outstanding IO thresholds
//	nPriorityThresholdCount : [optional] The size of pnPriorityThresholds
//	strRootDirectoryOverride : [optional] A pointer to a string for the root directory
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////
BOOL AsyncFileSystemInit(
	unsigned int * pnPriorityThresholds,			// = NULL
	unsigned int nPriorityThresholdCount,			// = 0
	WCHAR * strRootDirectoryOverride)				// = NULL			
{
	// SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);	// phu - use this to prevent error dialog when trying to access missing files on floppy or cd
	// SetErrorMode(SEM_NOOPENFILEERRORBOX);		// or maybe use this?

	// This lock gets taken while the system is initialized and terminated
	//
	CSAutoLock autoLock(&g_csAsyncFile);

	// If we've already initialized the system, just increment the ref count
	// and return, otherwise clean everything up by freeing everything.	
	//
	if (sAsyncFileSystemInitHelper())
	{
		return TRUE;
	}

	// Set the file system's root directory
	//
	if(strRootDirectoryOverride)
		PStrCopy(g_FS.m_strRootDirectory, strRootDirectoryOverride, MAX_PATH);  
	g_FS.m_lenRootDirectory = arrsize(g_FS.m_strRootDirectory);

	ASSERT_RETFALSE(AsyncFileGetRootDirectory(g_FS.m_strRootDirectory, g_FS.m_lenRootDirectory));

	// Setup the operation priority thresholds
	//
	ASSERT(!pnPriorityThresholds || nPriorityThresholdCount == NUM_ASYNC_PRIORITIES);
	if (!pnPriorityThresholds || nPriorityThresholdCount != NUM_ASYNC_PRIORITIES)
	{
		ASSERT_RETFALSE(arrsize(g_DefaultPriorityThresholds) == NUM_ASYNC_PRIORITIES);
		pnPriorityThresholds = g_DefaultPriorityThresholds;
	}
	ASSERT_RETFALSE(MemoryCopy(&g_FS.m_nPriorityThreshold, sizeof(g_FS.m_nPriorityThreshold), pnPriorityThresholds, sizeof(g_FS.m_nPriorityThreshold)));


	structclear( g_FS.m_bIsProcessing );

	// Figure out the ideal number of worker threads for the completion port
	//
	CPU_INFO info;
	ASSERT_RETFALSE(CPUInfo(info));
	ASSERT_RETFALSE(info.m_AvailCore > 0);
	unsigned int num_async_threads = MAX((unsigned int)2, (unsigned int)info.m_AvailCore);
	g_FS.m_nThreadPoolCount = num_async_threads;

	// Create the completion port for completion IO requests that all async files will be
	// associated with.
	//
	ASSERT_RETFALSE((g_FS.m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, g_FS.m_nThreadPoolCount)) != INVALID_HANDLE_VALUE);

	// Spawn worker threads
	//
	for (unsigned int ii = 0; ii < num_async_threads; ++ii)
	{
		AsyncFileSpawnWorkerThread(ii);
	}

	g_FS.m_eState = AFSS_READY;

	// This method should be called on the main thread, so set the thread id now
	//
	AppCommonSetMainThreadAsyncId(AsyncFileRegisterThread());

	g_iRefCount++;

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileRegisterThread
//
// Returns
//	A zero based index for the calling thread which must be used when calling async IO 
//	methods.  Returns -1 if the registered thread count has exceeded the maximum allowed.
//
// Remarks
//	The returned thread index is used internally to store completed IO operations on a 
//	per thread basis so that AsyncFileProcessComplete can execute those loading thread
//	callbacks in the context of the caller.
//
/////////////////////////////////////////////////////////////////////////////
DWORD AsyncFileRegisterThread(
	void)
{
	DWORD dwThreadId = (InterlockedIncrement(&g_FS.m_nRegisteredLoadingThreads) - 1);
	if (dwThreadId >= MAX_LOADING_THREADS)
	{
		InterlockedDecrement(&g_FS.m_nRegisteredLoadingThreads);
		return (DWORD)-1;
	}

	g_FS.m_dwLoadingThreadId[dwThreadId] = GetCurrentThreadId();

	return dwThreadId;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileProcessComplete
//
// Parameters
//	dwThreadId : The thread index for which completed operations will have their loading
//		callbacks executed.
//
// Remarks
//	This function should be polled on any thread that makes asynchronous IO requests 
//	that contain loading thread callbacks.  If it is the main thread, the function will call the
//	loading animation function.
//
/////////////////////////////////////////////////////////////////////////////
void AsyncFileProcessComplete(
	DWORD dwThreadId,
	BOOL bSkipProcessLoopCallback /*= FALSE*/ )
{
#if ISVERSION(SERVER_VERSION)
	REF( bSkipProcessLoopCallback );
#else
	BOOL bProcessLoopCallback = (dwThreadId == AppCommonGetMainThreadAsyncId() 
		&& gpfn_AsyncFileProcessLoopCallback
		&& ! bSkipProcessLoopCallback );
#endif // ! SERVER_VERSION

	ASSERT_RETURN(dwThreadId < MAX_LOADING_THREADS);
	ASSERT_RETURN(g_FS.m_dwLoadingThreadId[dwThreadId] == GetCurrentThreadId());

	if ( g_FS.m_bIsProcessing[ dwThreadId ] )
		return;
	else
		g_FS.m_bIsProcessing[ dwThreadId ] = TRUE;

	// Async file loading operations should allocate out of the NULL pool
	MEMORYPOOL* pMemPoolToRestore = MemoryGetThreadPool();
	MemorySetThreadPool( NULL );

	ASYNC_REQUEST * tail;
	ASYNC_REQUEST * head = g_FS.m_FinishedOperations[dwThreadId].GetAll(&tail);
	ASYNC_REQUEST * next = head;
	unsigned int count = 0;

	PRESULT pr = S_OK;

	while (ASYNC_REQUEST * curr = next)
	{
		next = curr->m_pNext;
		++count;

		{
#if ASYNC_FILE_TRACE_4
			TIMER_START3("AsyncFileLoadCallback()  IOTYPE:%s  REQUEST:%p  FILEPTR:%p  FILE:%s", \
					sAsyncFileGetIOTypeString(curr->m_eIoType), curr, curr->m_AsyncData.m_File, curr->m_AsyncData.m_UserData ? ((PAKFILE_LOAD_SPEC *)curr->m_AsyncData.m_UserData)->fixedfilename : "???");
#else
			TIMER_STARTEX3(15, "AsyncFileLoadCallback()  IOTYPE:%s  REQUEST:%p  FILEPTR:%p  FILE:%s", \
					sAsyncFileGetIOTypeString(curr->m_eIoType), curr, curr->m_AsyncData.m_File, curr->m_AsyncData.m_UserData ? ((PAKFILE_LOAD_SPEC *)curr->m_AsyncData.m_UserData)->fixedfilename : "???");
#endif
				
#if ASYNC_FILE_DEBUG
			ASSERT(curr->m_dwLoadingThreadId == dwThreadId);
			ASSERT(curr->m_AsyncData.m_fpLoadingThreadCallback);
#endif
			if (curr->m_AsyncData.m_fpLoadingThreadCallback)
			{
				V_SAVE( pr, curr->m_AsyncData.m_fpLoadingThreadCallback(curr->m_AsyncData) );
			}

			AsyncFileReleaseData(curr->m_AsyncData);

			AsyncFileReleaseFileStruct(curr->m_AsyncData.m_File);
		}

#if ! ISVERSION(SERVER_VERSION)
		if (bProcessLoopCallback)
		{
			gpfn_AsyncFileProcessLoopCallback();
		}
#endif // ! SERVER_VERSION

		InterlockedDecrement(&g_FS.m_nFinishedOperations);

	}

	

	g_FS.m_AsyncRequestFreeList.PutList(head, tail, count);

	MemorySetThreadPool( pMemPoolToRestore );
	g_FS.m_bIsProcessing[ dwThreadId ] = FALSE;
}


// ---------------------------------------------------------------------------
// returns either filename or buf, doesn't necessarily fill buf if filename is ok
// ---------------------------------------------------------------------------
static const WCHAR * sAsyncFileGetFullPathName(
	const WCHAR * filename,
	WCHAR * buf,
	unsigned int buflen)
{
	// If the filename looks fully qualified, don't do anything (ie c:\whatever)
	//
	if (filename[0] && filename[1] == L':')
	{
		return filename;
	}
	if (filename[0] == L'\\' && filename[1] == L'\\') {
		return filename;
	}

	// Prepend the root directory to the given filename
	//
	PStrCvt(buf, g_FS.m_strRootDirectory, buflen);
	PStrCopy(buf + g_FS.m_lenRootDirectory, filename, buflen - g_FS.m_lenRootDirectory);
	
	return buf;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileOpen
//
// Parameters
//	filename : A pointer to the filename to open
//	flags : A set of creation flags
//
// Returns
//	A pointer to the ASYNC_FILE opened as a result of the call or NULL if the function fails
//
/////////////////////////////////////////////////////////////////////////////
ASYNC_FILE * AsyncFileOpen(
	const WCHAR * filename,
	DWORD flags)
{
	ASSERT_RETNULL(filename);
	WCHAR buf[MAX_PATH];
	const WCHAR * fullfilename = NULL;
	
	if (TEST_MASK(flags, FF_USE_REL_PATH)) 
	{
		fullfilename = filename;
	} 
	else 
	{
		fullfilename = sAsyncFileGetFullPathName(filename, buf, arrsize(buf));
	}

	// desired access
	DWORD dwDesiredAccess = 0;
	ASSERT_RETNULL(TEST_MASK(flags, FF_READ | FF_WRITE));
	if (TEST_MASK(flags, FF_READ))
	{
		dwDesiredAccess |= GENERIC_READ;
		
		if (g_bSynchronousIORead)
		{
			SET_MASK(flags, FF_SYNCHRONOUS_READ);		
		}
	}
	if (TEST_MASK(flags, FF_WRITE))
	{
		dwDesiredAccess |= GENERIC_WRITE;
		
		if (g_bSynchronousIOWrite)
		{
			SET_MASK(flags, FF_SYNCHRONOUS_WRITE);		
		}		
	}

	// share mode
	DWORD dwShareMode = 0;
	if (TEST_MASK(flags, FF_SHARE_DELETE))
	{
		dwShareMode |= FILE_SHARE_DELETE;
	}
	if (TEST_MASK(flags, FF_SHARE_READ))
	{
		dwShareMode |= FILE_SHARE_READ;
	}
	if (TEST_MASK(flags, FF_SHARE_WRITE))
	{
		dwShareMode |= FILE_SHARE_WRITE;
	}

	// creation disposition
	DWORD dwCreationDisposition = OPEN_EXISTING;
	if (TEST_MASK(flags, FF_CREATE_ALWAYS))
	{
		ASSERT(!TEST_MASK(flags, FF_CREATE_NEW | FF_OPEN_ALWAYS));
		dwCreationDisposition = CREATE_ALWAYS;
	}
	else if (TEST_MASK(flags, FF_CREATE_NEW))
	{
		ASSERT(!TEST_MASK(flags, FF_OPEN_ALWAYS));
		dwCreationDisposition = CREATE_NEW;
	}
	else if (TEST_MASK(flags, FF_OPEN_ALWAYS))
	{
		dwCreationDisposition = OPEN_ALWAYS;
	}

	// flags and attributes
	DWORD dwFlagsAndAttributes = 0;		
	
	if((TEST_MASK(flags, FF_READ) && !TEST_MASK(flags, FF_SYNCHRONOUS_READ)) ||
		(TEST_MASK(flags, FF_WRITE) && !TEST_MASK(flags, FF_SYNCHRONOUS_WRITE)))
	{
		dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
		flags |= FF_OVERLAPPED;
	}

	if (TEST_MASK(flags, FF_TEMPORARY))
	{
		dwFlagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY;
	}
	if (TEST_MASK(flags, FF_RANDOM_ACCESS))
	{
		dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
	}
	if (TEST_MASK(flags, FF_SEQUENTIAL_SCAN))
	{
		dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	}
	if (TEST_MASK(flags, FF_WRITE_THROUGH))
	{
		dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
	}
	if (TEST_MASK(flags, FF_NO_BUFFERING))
	{
		ASSERT(0);	// nyi
	}

	ASSERT_RETNULL(dwDesiredAccess != 0);
	HANDLE handle = CreateFileW(fullfilename, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
#if ASYNC_FILE_TRACE_1
		trace("Error opening file: %S\n", fullfilename);	// CHB 2007.01.11 - note capital '%S'
		PrintSystemError(GetLastError());
#endif
		return NULL;
	}

	ASYNC_FILE * file = AsyncFileGetFileStruct();
	if (!file)
	{
		CloseHandle(handle);
		ASSERTX_RETNULL(0, "AsyncFileGetFileStruct() returned NULL");
	}

	// Associate the file with the completion port for completed IO operations
	//
	if (TEST_MASK(flags, FF_OVERLAPPED))
	{
		if (g_FS.m_hCompletionPort != CreateIoCompletionPort(handle, g_FS.m_hCompletionPort, (ULONG_PTR)file, g_FS.m_nThreadPoolCount))
		{
			CloseHandle(handle);
			ASSERTX_RETNULL(0, "Error associating file with completion port");
		}
	}

	file->m_hFileHandle = handle;
	file->m_hCloseEvent = INVALID_HANDLE_VALUE;
	file->m_dwFlags = flags;
	PStrCvt(file->m_szFileName, fullfilename, arrsize(file->m_szFileName));

#if ASYNC_FILE_TRACE_2
	TraceDebugOnly("open file: %S", fullfilename);	// CHB 2007.01.11 - note capital '%S'
#endif

	return file;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileOpen
//
// Parameters
//	filename : A pointer to the filename to open
//	flags : A set of creation flags
//
// Returns
//	A pointer to the ASYNC_FILE opened as a result of the call or NULL if the function fails
//
/////////////////////////////////////////////////////////////////////////////
ASYNC_FILE * AsyncFileOpen(
	const char * filename,
	DWORD flags)
{
	WCHAR buf[MAX_PATH];
	PStrCvt(buf, filename, _countof(buf));
	return AsyncFileOpen(buf, flags);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sSyncFileClose
//
// Parameters
//	file : A pointer to the file to close
//
// Remarks
//	Immediately closes the file and released the ASYNC_FILE to the pool
//
/////////////////////////////////////////////////////////////////////////////
static void sSyncFileClose(
	ASYNC_FILE * file)
{
	ASSERT_RETURN(file && file->m_hFileHandle != INVALID_FILE);
		
	// Make sure that the file was opened for synchronous access
	//
	ASSERT_RETURN(!TEST_MASK(file->m_dwFlags, FF_READ) || TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_READ));	
	ASSERT_RETURN(!TEST_MASK(file->m_dwFlags, FF_WRITE) || TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_WRITE));	
	
#if ASYNC_FILE_TRACE_2
	// TraceDebugOnly logs this out to the log.
	// The problem comes if we're closing the log that we're writing to.
	// Thus, this was moved above the CloseHandle() call
	// BUT that still won't fix this problem.  If we close the log that this
	// goes into, and then close any OTHER logs, then we're still in trouble,
	// because closing those logs will try to write out to the log that we just closed.
	TraceDebugOnly("close file: %s", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(file->m_szFileName));
#endif

	CloseHandle(file->m_hFileHandle);

	memclear(file, sizeof(ASYNC_FILE));
	g_FS.m_AsyncFileFreeList.Put(file);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileClose
//
// Parameters
//	file : A pointer to the ASYNC_FILE to be closed
//
// Remarks
//	If the file was not opened with async operation enabled, the file
//	will be immediately closed, otherwise the ref count will be decremented
//	and the ASYNC_FILE will be reclaimed to the pool if the ref count reaches 0.
//
/////////////////////////////////////////////////////////////////////////////
void AsyncFileClose(
	ASYNC_FILE * file)
{
	ASSERT_RETURN(file);
	if (file->m_hFileHandle == INVALID_FILE)
	{
		return;
	}

	// We can only close the file now if we aren't doing any synchronous
	// io on the file
	//
	if (!TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
	{
		sSyncFileClose(file);
		return;
	}

#if ASYNC_FILE_TRACE_4
	trace("AsyncFileClose:  FILE: %08x  %s\n", file, file->m_szFileName);
#endif

	// Decrement the ref count on the ASYNC_FILE and release the struct
	// to the pool if the count reaches zero
	//
	AsyncFileReleaseFileStruct(file);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileCloseNow
//
// Parameters
//	file : A pointer to the ASYNC_FILE to be closed
//
// Remarks
//	If the file was not opened with async operation enabled, the file
//	will be immediately closed, otherwise the ref count will be decremented
//	and function will wait for all references to be released and the file reclaimed
//	before returning.  
//
//	This seems like it would potentially cause a deadlock.
//
/////////////////////////////////////////////////////////////////////////////
void AsyncFileCloseNow(
	ASYNC_FILE * file)
{
	ASSERT_RETURN(file);
	if (file->m_hFileHandle == INVALID_FILE)
	{
		return;
	}
	
	// We can only close the file now if we aren't doing any synchronous
	// io on the file
	//
	if (!TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
	{
		sSyncFileClose(file);
		return;
	}
	

	ASSERT_RETURN(file->m_hCloseEvent == INVALID_HANDLE_VALUE);

	ASYNC_EVENT * async_event = AsyncFileGetEvent();
	ASSERT_RETURN(async_event);
	file->m_hCloseEvent = async_event->m_hEvent;

#if ASYNC_FILE_TRACE_4
	trace("AsyncFileCloseNow:  FILE: %08x  %s\n", file, file->m_szFileName);
#endif
	AsyncFileReleaseFileStruct(file);

	// wait for done signal
	DWORD result = WaitForSingleObject(async_event->m_hEvent, INFINITE);
	REF(result);
	
	AsyncFileRecycleEvent(async_event);
}


// ---------------------------------------------------------------------------
// return pointer to file name
// ---------------------------------------------------------------------------
const OS_PATH_CHAR * AsyncFileGetFilename(
	ASYNC_FILE * file)
{
	return file->m_szFileName;
}


// ---------------------------------------------------------------------------
// return Windows file handle
// ---------------------------------------------------------------------------
HANDLE AsyncFileGetHandle(
	ASYNC_FILE * file)
{
	return file->m_hFileHandle;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD AsyncFileGetFlags(
	ASYNC_FILE* file)
{
	ASSERT_RETZERO(file != NULL);
	return file->m_dwFlags;
}

// ---------------------------------------------------------------------------
// return size of file
// ---------------------------------------------------------------------------
UINT64 AsyncFileGetSize(
	ASYNC_FILE * file)
{
	ASSERT_RETFALSE(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(file);
	ASSERT_RETZERO(file->m_hFileHandle != INVALID_HANDLE_VALUE);
	return FileGetSize(file->m_hFileHandle);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 AsyncFileGetCreationTime(
	ASYNC_FILE * file)
{
	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(file && file->m_hFileHandle != INVALID_FILE);

	BY_HANDLE_FILE_INFORMATION info;
	ASSERT_RETZERO(GetFileInformationByHandle(file->m_hFileHandle, &info));

	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(info.ftCreationTime));
	memcpy(&temp, &info.ftCreationTime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 AsyncFileGetLastModifiedTime(
	ASYNC_FILE * file)
{
	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(file && file->m_hFileHandle != INVALID_FILE);

	BY_HANDLE_FILE_INFORMATION info;
	ASSERT_RETZERO(GetFileInformationByHandle(file->m_hFileHandle, &info));

	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(info.ftLastWriteTime));
	memcpy(&temp, &info.ftLastWriteTime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 AsyncFileGetLastAccessTime(
	ASYNC_FILE * file)
{
	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(file && file->m_hFileHandle != INVALID_FILE);

	BY_HANDLE_FILE_INFORMATION info;
	ASSERT_RETZERO(GetFileInformationByHandle(file->m_hFileHandle, &info));

	ULARGE_INTEGER temp;
	ASSERT(sizeof(ULARGE_INTEGER) == sizeof(info.ftLastAccessTime));
	memcpy(&temp, &info.ftLastAccessTime, sizeof(ULARGE_INTEGER));
	return (UINT64)temp.QuadPart;
};


// ---------------------------------------------------------------------------
// get the handle to the wait event
// ---------------------------------------------------------------------------
HANDLE AsyncEventGetHandle(
	ASYNC_EVENT * pEvent)
{
	ASSERT_RETNULL(pEvent != NULL);
	return pEvent->m_hEvent;
}


// ---------------------------------------------------------------------------
// Post the finished file load on to the main threads to handle the callback
// ---------------------------------------------------------------------------
BOOL AsyncFilePostRequestForCallback(
	ASYNC_DATA * pAsyncData)
{
	ASSERT_RETFALSE(pAsyncData != NULL);

	PAKFILE_LOAD_SPEC * pSpec = (PAKFILE_LOAD_SPEC *)pAsyncData->m_UserData;
	ASSERT_RETFALSE(pSpec != NULL);
	ASSERT_RETFALSE(pSpec->threadId < MAX_LOADING_THREADS);

	ASYNC_REQUEST * pRequest = AsyncFileGetRequestStruct(IO_Read, *pAsyncData, pSpec->priority, pSpec->threadId);
	ASSERT_RETFALSE(pRequest != NULL);

	g_FS.m_FinishedOperations[pSpec->threadId].Put(pRequest);

	InterlockedIncrement(&g_FS.m_nFinishedOperations);
	
	return TRUE;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void sSyncFileComplete(
	ASYNC_DATA & data)
{
	PRESULT pr = S_OK;

	// execute the fileiothread callback if it has one
	if (data.m_fpFileIOThreadCallback)
	{
		V_SAVE( pr, data.m_fpFileIOThreadCallback(data) );
	}

	// execute the loading thread callback if it has one
	if (data.m_fpLoadingThreadCallback)
	{
		V_SAVE( pr, data.m_fpLoadingThreadCallback(data) );
	}

	AsyncFileReleaseData(data);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sSyncFileRead
//
// Parameters
//	data : A reference to the ASYNC_DATA that will be used for the read operation
//	
// Returns
//	The number of bytes read or 0 if there was an error.
//
// Remarks
//	This function is not thread safe when a single file is used in multiple threads
//	since it uses an unprotected call to SetFilePointerEx.
//
/////////////////////////////////////////////////////////////////////////////
static unsigned int sSyncFileRead(
	ASYNC_DATA & data)
{
	ASYNCREAD_AUTOCLOSE autoclose(&data);

	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(data.m_File && data.m_File->m_hFileHandle != INVALID_FILE);
	ASSERT_RETZERO(TEST_MASK(data.m_File->m_dwFlags, FF_SYNCHRONOUS_READ));
	ASSERT_RETZERO(!TEST_MASK(data.m_File->m_dwFlags, FF_OVERLAPPED));
	
	sAllocateBufferIfNeeded( data );	
	ASSERT_RETZERO(data.m_Buffer && data.m_Bytes > 0);

	OVERLAPPED overlap;
	structclear(overlap);
	overlap.Offset = (DWORD)(data.m_Offset);
	overlap.OffsetHigh = (DWORD)(data.m_Offset >> (UINT64)32);
	if (!ReadFile(data.m_File->m_hFileHandle, data.m_Buffer, data.m_Bytes, &data.m_IOSize, &overlap))
	{
		return 0;
	}

	PakFileDecompressFile(&data);
	DWORD bytesread = data.m_IOSize;
	autoclose.Close();
	sSyncFileComplete(data);

	return bytesread;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileRead
//
// Parameters
//	data : A reference to the ASYNC_DATA that will be used for the read operation
//	priority : The priority of the read operation
//	dwLoadingThreadId : The thread index which identifies the calling thread returned from
//		AsyncFileRegisterThread.
//
// Returns
//	TRUE if the method succeeds, FALSE otherwise
//
/////////////////////////////////////////////////////////////////////////////
BOOL AsyncFileRead(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId)
{
	// Make sure the file gets closed if there is an error
	//
	ASYNCREAD_AUTOCLOSE autoclose(&data);

	ASSERT_RETFALSE(g_FS.m_eState != AFSS_SHUTDOWN);
	ASYNC_FILE * file = data.m_File;
	ASSERT_RETFALSE(file && file->m_hFileHandle != INVALID_FILE);
	ASSERT_RETFALSE(dwLoadingThreadId < MAX_LOADING_THREADS);

	// Execute read synchronously if specified
	//
	if (TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_READ))
	{
		autoclose.Release();
		
		if (!TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
		{
			return sSyncFileRead(data);
		}
		else
		{
			return AsyncFileReadNow(data);
		}
	}
	
	// fill out a request form
	ASYNC_REQUEST * request = AsyncFileGetRequestStruct(IO_ReadRequest, data, priority, dwLoadingThreadId);
	ASSERT_RETFALSE(request);

	// if we have too many active requests, pend this one (NOTE: this function increments g_FS.m_nCurrentOperations if it returns FALSE)
	if (AsyncFilePendRequest(request))	
	{
		autoclose.Release();		// don't automatically close the file
		return TRUE;
	}

	// perform the read
	if (!sAsyncFileRead(request))
	{
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
		return FALSE;
	}
	autoclose.Release();			// don't automatically close the file
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sAsyncFileNowComplete
//
// Parameters
//	request : A pointer to the IO request that just completed
//
// Remarks
//	Called when a synchronous IO operation completes.  It reclaims the ASYNC_REQUEST
//	to the pool and immediately fires the two callbacks, bypassing the completed operation
//	list since the synchronous operation starts and ends on the calling thread.
//
/////////////////////////////////////////////////////////////////////////////
static void sAsyncFileNowComplete(
	ASYNC_REQUEST * request)
{
	AsyncFileReleaseRequestFile(request);

	PRESULT pr = S_OK;

	// execute the fileiothread callback if it has one
	if (request->m_AsyncData.m_fpFileIOThreadCallback)
	{
		V_SAVE( pr, request->m_AsyncData.m_fpFileIOThreadCallback(request->m_AsyncData) );
	}

	// execute the loading thread callback if it has one
	if (request->m_AsyncData.m_fpLoadingThreadCallback)
	{
		V_SAVE( pr, request->m_AsyncData.m_fpLoadingThreadCallback(request->m_AsyncData) );
	}

	AsyncFileRecycleRequestStruct(request);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileReadNow
//
// Parameters
//	data : A reference to the ASYNC_DATA for the read operation
//
// Returns
//	The number of bytes read.
//
// Remarks
//
//	Calling this function from multiple threads for non-overlapped files
//	assumes that the caller provides its own close protection.
//
/////////////////////////////////////////////////////////////////////////////
unsigned int AsyncFileReadNow(
	ASYNC_DATA & data)
{
	ASYNCREAD_AUTOCLOSE autoclose(&data);

	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASYNC_FILE * file = data.m_File;
	ASSERT_RETFALSE(file && file->m_hFileHandle != INVALID_FILE);

	if (TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_READ) && !TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
	{
		autoclose.Release();		
		return sSyncFileRead(data);
	}

	// fill out a request form
	ASYNC_REQUEST * request = AsyncFileGetRequestStruct(IO_ReadNow, data);
	ASSERT_RETZERO(request);

	// get a signaling event so we can communicate to this thread when the action is complete
	ASYNC_EVENT * async_event = AsyncFileGetEvent();
	ASSERT(async_event);
	if (!async_event)
	{
		AsyncFileRecycleRequestStruct(request);
		return 0;
	}
	request->m_hLoadCompleteEvent = async_event->m_hEvent;

	InterlockedIncrement(&g_FS.m_nCurrentOperations);

	// perform the read
	if (!sAsyncFileRead(request))
	{
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
		return FALSE;
	}
	autoclose.Release();

	// wait for done signal
	DWORD result;
#if ! ISVERSION(SERVER_VERSION)
	// Update the loading screen occasionally.
	BOOL bMainThread = ( GetCurrentThreadId() == g_FS.m_dwLoadingThreadId[AppCommonGetMainThreadAsyncId()] );
	DWORD dwWaitTime = INFINITE;
	if ( bMainThread && gpfn_AsyncFileProcessLoopCallback )
		dwWaitTime = 100;
	while ( TRUE )
	{
		result = WaitForSingleObject(async_event->m_hEvent, dwWaitTime);
		if ( result != WAIT_TIMEOUT )
			break;
		gpfn_AsyncFileProcessLoopCallback();
	}
#else
	result = WaitForSingleObject(async_event->m_hEvent, INFINITE);
#endif // SERVER_VERSION

	AsyncFileRecycleEvent(async_event);

	DWORD bytesread = request->m_AsyncData.m_IOSize;

	sAsyncFileNowComplete(request);

	ASSERT_RETZERO(result != WAIT_ABANDONED);

	return bytesread;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	sSyncFileWrite
//
// Parameters
//	data : A reference to the ASYNC_DATA that will be used for the write operation
//	
// Returns
//	The number of bytes written or 0 if there was an error.
//
// Remarks
//	This function is not thread safe when a single file is used in multiple threads
//	since it uses an unprotected call to SetFilePointerEx.
//
/////////////////////////////////////////////////////////////////////////////
static unsigned int sSyncFileWrite(
	ASYNC_DATA & data)
{
	ASSERT_RETZERO(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETZERO(data.m_File && data.m_File->m_hFileHandle != INVALID_FILE);
	ASSERT_RETZERO(TEST_MASK(data.m_File->m_dwFlags, FF_SYNCHRONOUS_WRITE));
	ASSERT_RETZERO(!TEST_MASK(data.m_File->m_dwFlags, FF_OVERLAPPED));	
	ASSERT_RETZERO(data.m_Buffer && data.m_Bytes > 0);


	OVERLAPPED overlap;
	structclear(overlap);
	overlap.Offset = (DWORD)(data.m_Offset);
	overlap.OffsetHigh = (DWORD)(data.m_Offset >> (UINT64)32);
	if (!WriteFile(data.m_File->m_hFileHandle, data.m_Buffer, data.m_Bytes, &data.m_IOSize, &overlap))
	{
		return 0;
	}

	DWORD byteswritten = data.m_IOSize;

	sSyncFileComplete(data);

	return byteswritten;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileWrite
//
// Parameters
//	data : A reference to the ASYNC_DATA that will be used for the write operation
//	priority : The priority of the write operation
//	dwLoadingThreadId : The thread index which identifies the calling thread returned from
//		AsyncFileRegisterThread.
//
// Returns
//	TRUE if the method succeeds, FALSE if there was a failure or if the operation was 
//	queued
//
/////////////////////////////////////////////////////////////////////////////
BOOL AsyncFileWrite(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId)
{
	if (g_FS.m_eState == AFSS_SHUTDOWN)
	{
		return FALSE;
	}
	ASYNC_FILE * file = data.m_File;
	ASSERT_RETFALSE(file && file->m_hFileHandle != INVALID_FILE);
	ASSERT_RETFALSE(dwLoadingThreadId < MAX_LOADING_THREADS);

	if (TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_WRITE))
	{
		if(!TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
		{
			return sSyncFileWrite(data);
		}
		else
		{
			return AsyncFileWriteNow(data);
		}
	}

	// fill out a request form
	ASYNC_REQUEST * request = AsyncFileGetRequestStruct(IO_WriteRequest, data, priority, dwLoadingThreadId);
	if (!request)
	{
		return FALSE;
	}
	// if we have too many active requests, pend this one (NOTE: this function increments g_FS.m_nCurrentOperations if it returns FALSE)
	if (AsyncFilePendRequest(request))	
	{
		return TRUE;
	}

	// perform the write
	if (!sAsyncFileWrite(request))
	{
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
		return FALSE;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileWriteNow
//
// Parameters
//	data : A reference to the ASYNC_DATA for the write operation
//
// Returns
//	The number of bytes written.
//
// Remarks
//
//	Calling this function from multiple threads for non-overlapped files
//	assumes that the caller provides its own close protection.
//
/////////////////////////////////////////////////////////////////////////////
unsigned int AsyncFileWriteNow(
	ASYNC_DATA & data)
{
	ASSERT_RETFALSE(g_FS.m_eState != AFSS_SHUTDOWN);
	ASYNC_FILE * file = data.m_File;
	ASSERT_RETFALSE(file && file->m_hFileHandle != INVALID_FILE);

	if (TEST_MASK(file->m_dwFlags, FF_SYNCHRONOUS_WRITE) && !TEST_MASK(file->m_dwFlags, FF_OVERLAPPED))
	{
		return sSyncFileWrite(data);
	}

	// fill out a request form
	ASYNC_REQUEST * request = AsyncFileGetRequestStruct(IO_WriteNow, data);
	ASSERT_RETZERO(request);

	// get a signaling event so we can communicate to this thread when the action is complete
	ASYNC_EVENT * async_event = AsyncFileGetEvent();
	ASSERT(async_event);
	if (!async_event)
	{
		AsyncFileRecycleRequestStruct(request);
		return 0;
	}
	request->m_hLoadCompleteEvent = async_event->m_hEvent;

	InterlockedIncrement(&g_FS.m_nCurrentOperations);
	
	// perform the write
	if (!sAsyncFileWrite(request))
	{
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
		return 0;
	}

	// wait for done signal
	DWORD result;
#if ! ISVERSION(SERVER_VERSION)
	// Update the loading screen occasionally.
	BOOL bMainThread = ( GetCurrentThreadId() == g_FS.m_dwLoadingThreadId[AppCommonGetMainThreadAsyncId()] );
	DWORD dwWaitTime = INFINITE;
	if ( bMainThread && gpfn_AsyncFileProcessLoopCallback )
		dwWaitTime = 100;
	while ( TRUE )
	{
		result = WaitForSingleObject(async_event->m_hEvent, dwWaitTime);
		if ( result != WAIT_TIMEOUT )
			break;
		gpfn_AsyncFileProcessLoopCallback();
	}
#else
	result = WaitForSingleObject(async_event->m_hEvent, INFINITE);
#endif // SERVER_VERSION
	
	AsyncFileRecycleEvent(async_event);

	DWORD byteswritten = request->m_AsyncData.m_IOSize;

	sAsyncFileNowComplete(request);

	ASSERT_RETZERO(result != WAIT_ABANDONED);

	return byteswritten;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncWriteFile
//
// Parameters
//	filename : The name of the file to write
//	buffer : A pointer to the buffer to be written
//	bytes : The size of the buffer pointed to by buffer
//	gentime : [optional] A pointer to a value that will be set to the last modified time of the file
//
// Returns
//	TRUE if the function succeeds, FALSE otherwise.
//
// Remarks
//	This function opens, writes synchronously, and closes the file
//
/////////////////////////////////////////////////////////////////////////////
BOOL AsyncWriteFile(
	const TCHAR * filename,
	const void * buffer,
	unsigned int bytes,
	UINT64 * gentime)
{
	ASSERT_RETFALSE(buffer || bytes == 0);
	ASYNC_FILE * file = AsyncFileOpen(filename, FF_WRITE | FF_CREATE_ALWAYS | FF_SEQUENTIAL_SCAN);
	ASSERT_RETFALSE(file);

	BOOL result = FALSE;
	if (bytes > 0)
	{
		ONCE
		{
			void * buf = MALLOC(NULL, bytes);
			ASSERT_BREAK(buf);
			ASSERT_BREAK(MemoryCopy(buf, bytes, buffer, bytes));

			ASYNC_DATA asyncData(file, buf, 0, bytes, ADF_FREEBUFFER);
//			ASSERT_BREAK(AsyncFileWrite(asyncData, ASYNC_PRIORITY_MAX, AppCommonGetMainThreadAsyncId()));
			ASSERT_BREAK(AsyncFileWriteNow(asyncData));

			if (gentime)
			{
				*gentime = AsyncFileGetLastModifiedTime(file);
			}
			result = TRUE;
		}
	}

	AsyncFileClose(file);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFilePostDummyRequest
//
// Parameters
//	data : A reference to the ASYNC_DATA that will be used for the write operation
//	priority : The priority of the dummy operation
//	dwLoadingThreadId : The thread index which identifies the calling thread returned from
//		AsyncFileRegisterThread.
//
// Returns
//	TRUE if the function succeeds, FALSE if there was a failure or the operation was queued.
//
// Remarks	
//	A "Dummy" request doesn't do any file IO, but will
//	execute the loading thread callback from the context of the
//	worker thread.
//
/////////////////////////////////////////////////////////////////////////////
BOOL AsyncFilePostDummyRequest(
	ASYNC_DATA & data,
	unsigned int priority,
	DWORD dwLoadingThreadId)
{
	ASSERT_RETFALSE(g_FS.m_eState != AFSS_SHUTDOWN);
	ASSERT_RETFALSE(dwLoadingThreadId < MAX_LOADING_THREADS);

	// fill out a request form
	ASYNC_REQUEST * request = AsyncFileGetRequestStruct(IO_Dummy, data, priority, dwLoadingThreadId);
	ASSERT_RETFALSE(request);

	if (AsyncFilePendRequest(request))
	{
		return TRUE;
	}

	if (!sAsyncDummy(request))
	{
		InterlockedDecrement(&g_FS.m_nCurrentOperations);
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int AsyncFileAdjustPriorityByDistance(
	unsigned int nPriority,
	float distance)
{
	return MAX(0, (int)nPriority - (int)distance / 5);
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AsyncFileGetQueuedRequests
//
// Returns
//	The number of IO requests that are currently queued across all priorities
//
// Remarks	
//	A "Dummy" request doesn't do any file IO, but will
//	execute the loading thread callback from the context of the
//	worker thread.
//
/////////////////////////////////////////////////////////////////////////////
unsigned int AsyncFileGetQueuedRequests(
	void)
{

	unsigned int total_requests = 0;

	for (unsigned int ii = 0; ii < NUM_ASYNC_PRIORITIES; ++ii)
	{
		total_requests += g_FS.m_PendingOperations[ii].GetCount();
	}

	return total_requests;
}


// ---------------------------------------------------------------------------
// debug report
// ---------------------------------------------------------------------------
unsigned int AsyncFileDebugReport(
	char * buffer,
	unsigned int buffersize,
	BOOL newline,
	BOOL verbose)
{
	static const char * szOperation[] =
	{
		"READ_ ",		// IO_ReadRequest
		"READ  ",		// IO_Read
		"READ* ",		// IO_ReadNow
		"WRITE ",		// IO_Write
		"WRITE_",		// IO_WriteRequest
		"WRITE*",		// IO_WriteNow
	};
	static const char * szPriority[] =
	{
		"ASYNC_PRIORITY_0",										// lowest priority
		"ASYNC_PRIORITY_1",
		"ASYNC_PRIORITY_2",									
		"ASYNC_PRIORITY_3",									
		"ASYNC_PRIORITY_4",									
		"ASYNC_PRIORITY_5",									
		"ASYNC_PRIORITY_6",									
		"ASYNC_PRIORITY_7"										// highest priority
	};

	if (g_bSynchronousIORead && g_bSynchronousIOWrite)
	{
		return 0;
	}
	unsigned int lines = 0;
	if (lines >= buffersize / ASYNC_REPORT_WIDTH)
	{
		return lines;
	}
	// print already submitted files

	// print pending files
	for (unsigned int ii = 0; ii < NUM_ASYNC_PRIORITIES; ++ii)
	{
		if (lines >= buffersize / ASYNC_REPORT_WIDTH)
		{
			break;
		}

		PStrPrintf(buffer + lines * ASYNC_REPORT_WIDTH, MIN(ASYNC_REPORT_WIDTH, buffersize - lines * ASYNC_REPORT_WIDTH), 
			"QUEUED OPERATIONS -- PRIORITY: %s  COUNT: %u%c", szPriority[ii], g_FS.m_PendingOperations[ii].GetCount(), newline ? '\n' : 0);
		if (++lines >= buffersize / ASYNC_REPORT_WIDTH)
		{
			break;
		}

		if (!verbose)
		{
			continue;
		}

		{
			CSLAutoLock lock(&g_FS.m_PendingOperations[ii].m_CS);

			if (lines >= buffersize / ASYNC_REPORT_WIDTH)
			{
				break;
			}
			ASYNC_REQUEST * next = g_FS.m_PendingOperations[ii].m_pHead;
			while (ASYNC_REQUEST * curr = next)
			{
				next = curr->m_pNext;

				char temp[MAX_PATH];
				PStrCvt(temp, curr->m_AsyncData.m_File->m_szFileName, _countof(temp));
				unsigned int len = PStrLen(temp, _countof(temp));
				unsigned int nameoffs = (len > 40 ? len - 40 : 0);
				PStrPrintf(buffer + lines * ASYNC_REPORT_WIDTH, MIN(ASYNC_REPORT_WIDTH, buffersize - lines * ASYNC_REPORT_WIDTH), 
					"%s  FILE: %-40s  OFFS:%8I64u  BYTES:%8u%c", szOperation[curr->m_eIoType], temp + nameoffs, 
					curr->m_AsyncData.m_Offset, curr->m_AsyncData.m_Bytes, newline ? '\n' : 0);
				if (++lines >= buffersize / ASYNC_REPORT_WIDTH)
				{
					break;
				}
			}
		}
	}

	return lines;
}


// ---------------------------------------------------------------------------
// async file test
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
DWORD gdwAsyncThreadId =				((DWORD)-1);
const unsigned int BLOCK_SIZE =			4096;
const unsigned int BLOCK_COUNT =		2097152;	// creates a 300 MB file
const unsigned int READ_SIZE_MIN =		1024;
const unsigned int READ_SIZE_MAX =		1024;
const unsigned int READ_ITER =			16384;
unsigned int gnReadCount = 0;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void AsyncFileTestGenerateReport(
	BOOL verbose)
{
	char buffer[100 * ASYNC_REPORT_WIDTH + 20];
	unsigned int lines = AsyncFileDebugReport(buffer, arrsize(buffer), TRUE, verbose);
	for (unsigned int ii = 0; ii < lines; ++ii)
	{
		trace(buffer + ii * ASYNC_REPORT_WIDTH);
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PRESULT AsyncFileTestReadCallback(
	ASYNC_DATA & data)
{
	REF(data);

	gnReadCount++;
//	trace("read %d bytes.\n", bytes);
	return S_OK;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
PRESULT AsyncFileTestWriteCallback(
	ASYNC_DATA & data)									
{
	REF(data);
//	trace("wrote %d bytes.\n", bytes);
	return S_OK;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL AsyncFileTestCreateTestFile(
	ASYNC_FILE * fp)
{
	for (unsigned int ii = 0; ii < BLOCK_COUNT; ++ii)
	{
		BYTE * buf = (BYTE *)MALLOC(NULL, BLOCK_SIZE);
		ASSERT_RETFALSE(buf);
		for (unsigned int jj = 0; jj < BLOCK_SIZE; jj += 4)
		{
			*(int *)(buf +jj) = ii * BLOCK_SIZE / 4 + jj;
		}
		ASYNC_DATA asyncData(fp, buf, BLOCK_SIZE * ii, BLOCK_SIZE, ADF_FREEBUFFER, AsyncFileTestWriteCallback, NULL);
		ASSERT(AsyncFileWrite(asyncData, ASYNC_PRIORITY_4 , gdwAsyncThreadId));
//		unsigned int bytes_written = AsyncFileWrite(asyncData);
//		ASSERT(bytes_written == BLOCK_SIZE);
		if (ii % 100 == 99)
		{
			while (AsyncFileGetQueuedRequests() > 400)
			{
				Sleep(3);
			}
		}
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL AsyncFileTestReadTestFile(
	ASYNC_FILE * fp)
{
	trace("start reading!\n");
	for (unsigned int ii = 0; ii < BLOCK_COUNT; ++ii)
	{
		BYTE * buf = (BYTE *)MALLOC(NULL, BLOCK_SIZE);
		ASSERT_RETFALSE(buf);

		ASYNC_DATA asyncData(fp, buf, BLOCK_SIZE * ii, BLOCK_SIZE, ADF_FREEBUFFER, AsyncFileTestReadCallback, NULL);
		ASSERT_RETFALSE(AsyncFileRead(asyncData, ASYNC_PRIORITY_0, gdwAsyncThreadId));
		AsyncFileProcessComplete(gdwAsyncThreadId);
	}

	while (gnReadCount < BLOCK_COUNT)
	{
		AsyncFileProcessComplete(gdwAsyncThreadId);
		Sleep(10);
	}
	trace("done reading!\n");

	return TRUE;
}


// ---------------------------------------------------------------------------
// async file test
// ---------------------------------------------------------------------------
BOOL AsyncFileTest(
	DWORD dwAsyncThreadId)	// = -1
{
	if (dwAsyncThreadId == -1)
	{
		dwAsyncThreadId = AsyncFileRegisterThread();
	}
	gdwAsyncThreadId = dwAsyncThreadId;

	char szDir[MAX_PATH];
	GetCurrentDirectory(arrsize(szDir), szDir);

	ASYNC_FILE * fp = AsyncFileOpen("..\\test.dat", FF_READ | FF_WRITE | FF_RANDOM_ACCESS | FF_CREATE_ALWAYS);
//	ASYNC_FILE * fp = AsyncFileOpen("..\\test.dat", FF_READ | FF_WRITE | FF_RANDOM_ACCESS);
	ASSERT_RETFALSE(fp);

	ASSERT_RETFALSE(AsyncFileTestCreateTestFile(fp));

//	ASSERT_RETFALSE(AsyncFileTestReadTestFile(fp));

/*
	BYTE buf[1024];
	ASSERT_RETFALSE(AsyncFileRead(fp, (LPVOID)buf, 0, arrsize(buf), ASYNC_PRIORITY_HIGH, NULL, AsyncFileTestReadCallback, gdwAsyncThreadId));
//	unsigned int bytes_read = AsyncFileReadNow(fp, (LPVOID)buf, 0, arrsize(buf));
//	trace("read %d bytes\n", bytes_read);
*/
/*
	for (unsigned int ii = 0; ii < 100; ++ii)
	{
		AsyncFileProcessComplete(gdwAsyncThreadId);
		Sleep(10);
	}
*/
	AsyncFileClose(fp);

	return TRUE;
}

