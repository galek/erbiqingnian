//----------------------------------------------------------------------------
// DatabaseManager.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _DATABASE_MANAGER_H_
#define _DATABASE_MANAGER_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include <memory>						//	for global placement new...
#include <string>


//----------------------------------------------------------------------------
// INTERFACE DEFINES
//----------------------------------------------------------------------------
#define MAX_DB_CONNECTIONS			32	   // arbitrary
#define MAX_DB_BATCH_SIZE			64	   // arbitrary
#define MAX_DB_REQUEST_OBJ_SIZE		2048*4 // size of request object memory buffer


//----------------------------------------------------------------------------
// INTERFACE TYPES
//----------------------------------------------------------------------------

typedef UINT32	DB_REQUEST_TICKET;
#define			INVALID_DB_REQUEST_TICKET	0

//----------------------------------------------------------------------------
//	setup information struct for the database manager.
//----------------------------------------------------------------------------
struct database_manager_setup
{
	database_manager_setup(void) { memclear(this, sizeof(this)); }
	struct MEMORYPOOL *		memPool;
	LPCTSTR					databaseAddress;
	LPCTSTR					databaseServer;
	LPCTSTR					databaseUserId;
	LPCTSTR					databaseUserPassword;
	LPCTSTR					databaseName;
	BOOL					bIsTrustedDatabase;
	LPCTSTR					databaseNetwork;
	UINT					nDatabaseConnections;
	UINT					nMaxRequestQueueLength;
	INT						nRequestsPerConnectionBeforeReestablishingConnection;
};

//----------------------------------------------------------------------------
//	internal database connection information used by ODBC connections.
//----------------------------------------------------------------------------
struct database_connect_info
{
	database_connect_info(void) { memclear(this, sizeof(this)); }
	TCHAR					DbAddress[MAX_NET_ADDRESS_LEN];
	TCHAR					DbServer[MAX_NET_ADDRESS_LEN];
	TCHAR					DbUserId[MAX_NET_ADDRESS_LEN];
	TCHAR					DbUserPassword[MAX_NET_ADDRESS_LEN];
	TCHAR					DbName[MAX_NET_ADDRESS_LEN];
	BOOL					bTrustedDatabase;
	TCHAR					DbNetworkName[MAX_NET_ADDRESS_LEN];
};

//----------------------------------------------------------------------------
//	result code to be returned by query request objects upon completion of a request.
//----------------------------------------------------------------------------
enum DB_REQUEST_RESULT
{
	DB_REQUEST_RESULT_SUCCESS,
	DB_REQUEST_RESULT_FAILED,
	DB_REQUEST_RESULT_CONNECTION_ERROR
};

enum DB_FATAL_EVENT
{
	DB_FATAL_EVENT_NONE,
	DB_FATAL_EVENT_ALL_CONNECTIONS_LOST,
	DB_FATAL_EVENT_QUEUE_LENGTH_EXCEEDED
};

//----------------------------------------------------------------------------
// INTERFACE
//----------------------------------------------------------------------------

struct DATABASE_MANAGER;
struct DB_GAME_CALLBACK;

typedef void (*FP_ON_DB_ERROR_EVENT)(DATABASE_MANAGER*, DB_FATAL_EVENT, LPVOID);

// DatabaseManagerInit() returns 0 on failure.  if bDefaultDatabaseManager, the database will be the default
// database used by any function where no database manager handle is provided.
DATABASE_MANAGER*
	DatabaseManagerInit(
		database_manager_setup &,
		FP_ON_DB_ERROR_EVENT,
		LPVOID callbackContext );

void
	DatabaseManagerFree(
		DATABASE_MANAGER* pDBM );

bool 
	DatabaseManagerIsConnected(
		DATABASE_MANAGER* pDBM );

BOOL
	DatabaseManagerSetConnectionCount(
		DATABASE_MANAGER *pDBM,
		DWORD dwConnectionCount );

DWORD
	DatabaseManagerGetQueueSize(
		DATABASE_MANAGER *pDBM );


//----------------------------------------------------------------------------
// CustomDatabaseRequest must implement the following template interface and
//   define the shown global template variable:
// struct CustomDatabaseRequest {
//
//     // type used to init the transport data
//     struct InitData          { ... };
//
//     // type used to transport data to the DoRequest methods
//     struct DataType			{ ... };
//
//     // fills a given DataType using the init data ptr given in the DatabaseManagerQueueRequest method
//     static BOOL				InitRequestData( DataType *, InitData * );
//
//     // frees any resources allocated in the InitData method
//     static void				FreeRequestData( DataType * );
//
//     // executes a request on a single DataType object
//     static DB_REQUEST_RESULT DoSingleRequest( HDBC, DataType * );
//
//     // intermediate batch size used for batches that time-out before
//     //   the full batch size has been reached
//     static const UINT		PartialBatchSize;
//
//     // intermediate batch processing method, passed an array of pointers
//     //   to data to be processed of size PartialBatchSize
//     static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * );
//
//     // number of data type objects expected in the DoFullBatchRequest method
//     //   and the maximum number of requests to be batched before processing
//     static const UINT		FullBatchSize;
//
//     // executes a request on an array of pointers to DataType objects of
//     //   size FullBatchSize
//     static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * );
//
//     // maximum timespan in ms that a request will sit in a list before being dispatched
//     static const DWORD		MaxBatchWaitTime = MAX_WAIT_TIME_BEFORE_FIRST_REQUEST_DISPATCHED;
// };
// DB_REQUEST_BATCHER<CustomDatabaseRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<CustomDatabaseRequest>::s_dat;
//----------------------------------------------------------------------------
template<class CustomDatabaseRequest>
BOOL
	DatabaseManagerQueueRequest(
		DATABASE_MANAGER* pDBM,
		BOOL	bWaitForCompletion,
		DB_REQUEST_TICKET & outRequestTicket,
		void *	callbackContext,
		void  (*fp_onRequestCompletionApp)(
					void * callbackContext,
					typename CustomDatabaseRequest::DataType * userRequestDataType,
					BOOL requestSuccess ),
		BOOL	bBatchedOperation,
		typename CustomDatabaseRequest::InitData * initData,
		PGUID idSerialization = INVALID_GUID,
		DB_GAME_CALLBACK *pGameCallback = NULL);

BOOL
	DatabaseManagerCancelRequest(
		DATABASE_MANAGER* pDBM,
		DB_REQUEST_TICKET ticket );

std::string 
	GetSQLErrorStr(
		SQLHANDLE Handle, 
		SQLSMALLINT iHandleType = SQL_HANDLE_STMT);

SQLHSTMT
	DatabaseManagerGetStatement(
		SQLHDBC dbHandle,
		const char * statementText );

void
	DatabaseManagerReleaseStatement(
		SQLHSTMT & stmtHdl );


class CScopedSQLStmt
{
public:
	CScopedSQLStmt(SQLHDBC hDBC, const char* szStatement) : hStmt(DatabaseManagerGetStatement(hDBC, szStatement)) { }
	~CScopedSQLStmt(void) { DatabaseManagerReleaseStatement(hStmt); }
	operator SQLHSTMT(void) { return hStmt; }

protected:
	SQLHSTMT hStmt;
};

#define NULL_BATCH_MEMBERS	static const UINT		 PartialBatchSize = 0; \
							static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * ) { return DB_REQUEST_RESULT_FAILED; } \
							static const UINT		 FullBatchSize = 0; \
							static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * ) { return DB_REQUEST_RESULT_FAILED; } \
							static const DWORD		 MaxBatchWaitTime = 0;


//----------------------------------------------------------------------------
// INTERNALS
//----------------------------------------------------------------------------
#include "DatabaseManager_priv.h"

#endif	//	_DATABASE_MANAGER_H_
