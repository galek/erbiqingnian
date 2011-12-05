//----------------------------------------------------------------------------
// DatabaseManager_priv.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "ServerSuite/Database/DatabaseGame.h"

//----------------------------------------------------------------------------
// INTERNAL TYPEDEFS
//----------------------------------------------------------------------------

typedef BOOL			  (*FP_DBREQ_INIT_DATA)			( void *, void * );
typedef void			  (*FP_DBREQ_FREE_DATA)			( void * );
typedef DB_REQUEST_RESULT (*FP_DBREQ_DO_SINGLE_REQUEST)	( HDBC, void * );
typedef DB_REQUEST_RESULT (*FP_DBREQ_DO_PARTIAL_BATCH)	( HDBC, void * * );
typedef DB_REQUEST_RESULT (*FP_DBREQ_DO_FULL_BATCH)		( HDBC, void * * );
typedef void			  (*FP_DBREQ_COMP_CALLBACK)		( void *, void *, BOOL );
typedef void			  (*FP_DBREQ_BATCH_CLEANUP)		( struct DB_BATCHED_REQUEST *, BOOL);

//----------------------------------------------------------------------------
// INTERNAL TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct DB_REQUEST_DATA : LIST_SL_NODE
{
	BYTE						Data[MAX_DB_REQUEST_OBJ_SIZE - sizeof(LIST_SL_NODE)];
};

//----------------------------------------------------------------------------
struct DB_APP_CALLBACK
{
	FP_DBREQ_COMP_CALLBACK		fpCompletionCallback;		// the callback (an app level callback)
	void *						CallbackContext;			// the callback context
};

//----------------------------------------------------------------------------
struct DB_COMPLETION_CALLBACK
{
	DB_APP_CALLBACK AppCallback;
	DB_GAME_CALLBACK GameCallback;
};

//----------------------------------------------------------------------------
struct DB_SINGLE_REQUEST
{
	BOOL						bWaitForCompletion;
	DB_COMPLETION_CALLBACK		CompletionCallback;
	DB_REQUEST_DATA *			RequestData;
};

//----------------------------------------------------------------------------
struct DB_BATCHED_REQUEST : LIST_SL_NODE
{
	DB_REQUEST_DATA *			RequestData[MAX_DB_BATCH_SIZE];
	DB_COMPLETION_CALLBACK		CompletionCallback[MAX_DB_BATCH_SIZE];
	UINT						RequestCount;
	FP_DBREQ_BATCH_CLEANUP		fpBatchedRequestCleanup;
};

//----------------------------------------------------------------------------
struct db_request
{
	DATABASE_MANAGER*			pDBM;
	DB_BATCHED_REQUEST *		BatchData;
	DB_SINGLE_REQUEST			SingleData;
	FP_DBREQ_DO_SINGLE_REQUEST	fpDoSingleReq;
	FP_DBREQ_DO_PARTIAL_BATCH	fpDoPartialBatch;
	FP_DBREQ_DO_FULL_BATCH		fpDoFullBatch;
	FP_DBREQ_FREE_DATA			fpFreeData;
	UINT						PartialBatchSize;
	UINT						FullBatchSize;
	PGUID						SerializationId;
};


//----------------------------------------------------------------------------
// INTERNAL METHODS
//----------------------------------------------------------------------------

struct MEMORYPOOL *
	DatabaseManagerPrivGetMemPool(
		DATABASE_MANAGER* pDBM );

DB_REQUEST_DATA *
	DatabaseManagerPrivGetRequestData(
		DATABASE_MANAGER* pDBM  );

void
	DatabaseManagerPrivFreeRequestData(
		DATABASE_MANAGER* pDBM, 
		DB_REQUEST_DATA * );

DB_BATCHED_REQUEST *
	DatabaseManagerPrivGetBatchedRequest(
		DATABASE_MANAGER* pDBM );

void
	DatabaseManagerPrivFreeBatchedRequest(
		DATABASE_MANAGER* pDBM,
		DB_BATCHED_REQUEST * toFree );

BOOL
	DatabaseManagerPrivRegisterBatchWaitCallback(
		DATABASE_MANAGER* pDBM,
		DWORD timeout,
		void (*fpTimeoutCallback)( DWORD ),
		DWORD requestContext );

void
	DatabaseManagerPrivRegisterBatchCleanupCallback(
		DATABASE_MANAGER* pDBM,
		void (*fpOnClose)( MEMORYPOOL * ) );

DB_REQUEST_TICKET
	DatabaseManagerPrivQueueRequest(
		db_request &,
		SVRID,
		BOOL &);

void 
	DatabaseManagerPrivCancelWaitingRequests(
		DATABASE_MANAGER* pDBM) ;

void 
	DatabaseManagerDBCompletionCallbackInit(
		DB_COMPLETION_CALLBACK &CompletionCallback);
	
void 
	DatabaseManagerDoCompletionCallbacks(
		BOOL bSuccess,	
		const DB_COMPLETION_CALLBACK &tDBCompletionCallback,
		DB_REQUEST_DATA *pRequestData);
	
//----------------------------------------------------------------------------
// INTERNAL REQUEST BATCHING
//----------------------------------------------------------------------------
#include "DatabaseRequestBatcher.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class CustomDatabaseRequest>
BOOL DatabaseManagerQueueRequest(
	DATABASE_MANAGER* pDBM,
	BOOL	bWaitForCompletion,
	DB_REQUEST_TICKET & outRequestTicket,
	void *	callbackContext,
	void  (*fp_onRequestCompletionApp)(
				void *,
				typename CustomDatabaseRequest::DataType * ,
				BOOL ),
	BOOL	bBatchOperation,
	typename CustomDatabaseRequest::InitData * initData,
	PGUID idSerialization /*= INVALID_GUID*/,
	DB_GAME_CALLBACK *pGameCallback /*= NULL*/)
{
	outRequestTicket = INVALID_DB_REQUEST_TICKET;
	STATIC_CHECK(sizeof(CustomDatabaseRequest::DataType) <= MAX_DB_REQUEST_OBJ_SIZE, EXCEEEDS_MAX_DB_REQUEST_OBJ_SIZE);

	if (!DatabaseManagerIsConnected(pDBM))
		return false;

	if(bBatchOperation)
	{
		ASSERTX_RETFALSE(
			!bWaitForCompletion,
			"Cannot wait for a batched database request to complete.");

		UINT maxSize = CustomDatabaseRequest::FullBatchSize;
		ASSERTX_RETFALSE(
			maxSize > 0,
			"Specified database object does not support batched requests." );
		ASSERTX_RETFALSE(
			maxSize <= MAX_DB_BATCH_SIZE,
			"Specified database object max batch size is too large." );

		UINT partialSize = CustomDatabaseRequest::PartialBatchSize;
		ASSERTX_RETFALSE(
			partialSize > 1 && partialSize < maxSize,
			"Specified database object has an invalid partial batch size." );

		return DB_REQUEST_BATCHER<CustomDatabaseRequest>::AddRequest(
				callbackContext,
				fp_onRequestCompletionApp,
				initData,
				pGameCallback );
	}
	else
	{
		db_request reqData;
		reqData.pDBM				= pDBM;
		reqData.BatchData			= NULL;

		reqData.fpDoSingleReq		= (FP_DBREQ_DO_SINGLE_REQUEST)CustomDatabaseRequest::DoSingleRequest;
		reqData.fpDoPartialBatch	= (FP_DBREQ_DO_PARTIAL_BATCH)CustomDatabaseRequest::DoPartialBatchRequest;
		reqData.fpDoFullBatch		= (FP_DBREQ_DO_FULL_BATCH)CustomDatabaseRequest::DoFullBatchRequest;
		reqData.fpFreeData			= (FP_DBREQ_FREE_DATA)CustomDatabaseRequest::FreeRequestData;
		reqData.PartialBatchSize	= CustomDatabaseRequest::PartialBatchSize;
		reqData.FullBatchSize		= CustomDatabaseRequest::FullBatchSize;
		reqData.SerializationId		= idSerialization;

		reqData.SingleData.bWaitForCompletion	= bWaitForCompletion;

		// get callback struct		
		DB_COMPLETION_CALLBACK &CompletionCallback = reqData.SingleData.CompletionCallback;
		DatabaseManagerDBCompletionCallbackInit( CompletionCallback );
		
		// save app callback
		DB_APP_CALLBACK &AppCallback = CompletionCallback.AppCallback;
		AppCallback.fpCompletionCallback = (FP_DBREQ_COMP_CALLBACK)fp_onRequestCompletionApp;
		AppCallback.CallbackContext = callbackContext;

		// save in game callback
		if (pGameCallback)
		{
			CompletionCallback.GameCallback = *pGameCallback;
		}
				
		reqData.SingleData.RequestData			= DatabaseManagerPrivGetRequestData(pDBM);

		ASSERT_RETFALSE(reqData.SingleData.RequestData);

		ASSERT_GOTO(
			CustomDatabaseRequest::InitRequestData(
				(CustomDatabaseRequest::DataType*)reqData.SingleData.RequestData,
				initData),
			_QUEUE_ERROR );

		BOOL success = FALSE;
		outRequestTicket = DatabaseManagerPrivQueueRequest(
								reqData,
								ServerId(
									CurrentSvrGetType(),
									CurrentSvrGetInstance() ),
								success );

		if (bWaitForCompletion)
			return success;
		else
			return (outRequestTicket != INVALID_DB_REQUEST_TICKET);

_QUEUE_ERROR:
		if(reqData.SingleData.RequestData)
			DatabaseManagerPrivFreeRequestData(pDBM, reqData.SingleData.RequestData );

		return FALSE;
	}
}
