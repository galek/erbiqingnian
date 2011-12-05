//----------------------------------------------------------------------------
// DatabaseRequestBatcher.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// template singleton database request object batcher
//----------------------------------------------------------------------------
template<class T>
struct DB_REQUEST_BATCHER
{
	struct _BATCHER_DATA
	{
		_BATCHER_DATA() :
			BatcherLock(TRUE),
			BatcherInitialized(FALSE),
			BatcherMemPool(NULL),
			CurrentBatch(NULL),
			CurrentBatchId(0),
			DispatchingServerId(INVALID_SVRID),
			pDBM(0)
		{ }
		volatile BOOL			BatcherInitialized;
		CCriticalSection		BatcherLock;
		MEMORYPOOL *			BatcherMemPool;
		DB_BATCHED_REQUEST *	CurrentBatch;
		DWORD					CurrentBatchId;
		SVRID					DispatchingServerId;
		DATABASE_MANAGER*		pDBM;
	};
	static _BATCHER_DATA		s_dat;

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static BOOL sInitBatcher(
		MEMORYPOOL * pool  )
	{
		CSAutoLock lock = &s_dat.BatcherLock;

		if(s_dat.BatcherInitialized)
			return TRUE;

		DatabaseManagerPrivRegisterBatchCleanupCallback(s_dat.pDBM, sFreeBatcher);

		s_dat.BatcherMemPool = pool;
		s_dat.BatcherInitialized = TRUE;
		s_dat.DispatchingServerId = ServerId(CurrentSvrGetType(),CurrentSvrGetInstance());

		return TRUE;
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static void sFreeBatcher(
		MEMORYPOOL * )
	{
		if(s_dat.CurrentBatch)
		{
			sBatchCleanup(s_dat.CurrentBatch,FALSE);
			s_dat.CurrentBatch = NULL;
		}
		s_dat.BatcherInitialized = FALSE;
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static BOOL AddRequest(
		void * callbackContext,
		void (*fpCompletionCallback)(void *, typename T::DataType *, BOOL),
		typename T::InitData * initData,
		DB_GAME_CALLBACK *pGameCallback = NULL)
	{
		if(!s_dat.BatcherInitialized)
			ASSERT_RETFALSE(sInitBatcher(DatabaseManagerPrivGetMemPool(s_dat.pDBM)));

		CSAutoLock lock = &s_dat.BatcherLock;

		if(!s_dat.CurrentBatch)
		{
			s_dat.CurrentBatch = DatabaseManagerPrivGetBatchedRequest(s_dat.pDBM);
			ASSERT_RETFALSE(s_dat.CurrentBatch);

			s_dat.CurrentBatch->fpBatchedRequestCleanup = sBatchCleanup;
			s_dat.CurrentBatch->RequestCount = 0;

			++s_dat.CurrentBatchId;
		}

		UINT curCount = s_dat.CurrentBatch->RequestCount;
		s_dat.CurrentBatch->RequestData[curCount] = DatabaseManagerPrivGetRequestData(s_dat.pDBM);
		ASSERT_RETFALSE(s_dat.CurrentBatch->RequestData[curCount]);
		ASSERT_DO(
			T::InitRequestData(
				(T::DataType*)s_dat.CurrentBatch->RequestData[curCount],
				initData ) )
		{
			DatabaseManagerPrivFreeRequestData(s_dat.pDBM, s_dat.CurrentBatch->RequestData[curCount]);
			return FALSE;
		}
		
		// init callbacks
		DB_COMPLETION_CALLBACK &CompletionCallback = s_dat.CurrentBatch->CompletionCallback[curCount];
		DatabaseManagerDBCompletionCallbackInit( CompletionCallback );
		
		// save the app level callback (if any)
		DB_APP_CALLBACK &AppCallback = CompletionCallback.AppCallback;
		AppCallback.fpCompletionCallback = (FP_DBREQ_COMP_CALLBACK)fpCompletionCallback;
		AppCallback.CallbackContext = callbackContext;
		
		// in game callback
		if (pGameCallback)
		{
			CompletionCallback.GameCallback = *pGameCallback;
		}
		
		s_dat.CurrentBatch->RequestCount++;

		if(s_dat.CurrentBatch->RequestCount == T::FullBatchSize)
		{
			return sDispatchRequests();
		}
		else if(s_dat.CurrentBatch->RequestCount == 1)
		{
			return DatabaseManagerPrivRegisterBatchWaitCallback(
						s_dat.pDBM,
						T::MaxBatchWaitTime,
						sOnBatchTimeout,
						s_dat.CurrentBatchId );
		}

		return TRUE;
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static void sOnBatchTimeout(
		DWORD batchId )
	{
		CSAutoLock lock = &s_dat.BatcherLock;

		if(  batchId != s_dat.CurrentBatchId ||	// old batch, already dispatched
			!s_dat.CurrentBatch )				// no batch, already dispatched
			return;

		ASSERT( sDispatchRequests() );
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static BOOL sDispatchRequests(
		void )
	{
		DB_BATCHED_REQUEST * request = s_dat.CurrentBatch;
		s_dat.CurrentBatch = NULL;

		ASSERT_RETFALSE(request);

		db_request reqData;
		structclear(reqData);

		reqData.pDBM				= s_dat.pDBM;
		reqData.BatchData			= request;
		reqData.fpDoSingleReq		= (FP_DBREQ_DO_SINGLE_REQUEST)T::DoSingleRequest;
		reqData.fpDoPartialBatch	= (FP_DBREQ_DO_PARTIAL_BATCH)T::DoPartialBatchRequest;
		reqData.fpDoFullBatch		= (FP_DBREQ_DO_FULL_BATCH)T::DoFullBatchRequest;
		reqData.fpFreeData			= (FP_DBREQ_FREE_DATA)T::FreeRequestData;
		reqData.PartialBatchSize	= T::PartialBatchSize;
		reqData.FullBatchSize		= T::FullBatchSize;

		BOOL success = FALSE;
		DatabaseManagerPrivQueueRequest(reqData,s_dat.DispatchingServerId, success);
		return success;
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	static void sBatchCleanup(
		DB_BATCHED_REQUEST * request,
		BOOL success )
	{
		ASSERT_RETURN(request);

		for(UINT ii = 0;
			ii < request->RequestCount &&
			ii < MAX_DB_BATCH_SIZE;
			++ii)
		{
			const DB_COMPLETION_CALLBACK &CompletionCallback = request->CompletionCallback[ii];
			DB_REQUEST_DATA *pRequestData = request->RequestData[ii];
		
			// do the callbacks
			DatabaseManagerDoCompletionCallbacks( success, CompletionCallback, pRequestData );

			T::FreeRequestData((T::DataType*)pRequestData);
			DatabaseManagerPrivFreeRequestData(s_dat.pDBM, pRequestData);
		}
		DatabaseManagerPrivFreeBatchedRequest(s_dat.pDBM, request);
	}
};
