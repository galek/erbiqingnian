//----------------------------------------------------------------------------
// FILE: asyncrequest.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "asyncrequest.h"
#include "game.h"
#include "stlallocator.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

// an async request id consists of a bucket in the high bits, 
// and the id part in the lower bits
#define ASYNC_BUCKET_MAX			0x000000FF
#define ASYNC_ID_MAX				0x00FFFFFF

#define ASYNC_ID_MASK_BUCKET		0xFF000000
#define ASYNC_ID_MASK_ID			0x00FFFFFF

#define ASYNC_ID_CREATE(bucket,id)	(((bucket & ASYNC_BUCKET_MAX) << 24) | (id & ASYNC_ID_MAX))
#define ASYNC_ID_GET_BUCKET(id)		((id & ASYNC_ID_MASK_BUCKET) >> 24)
#define ASYNC_ID_GET_ID(id)			(id & ASYNC_ID_MASK_ID)

typedef map_mp<ULONGLONG, ASYNC_REQUEST_ID>::type KEY_MAP;

//----------------------------------------------------------------------------
struct ASYNC_REQUEST
{

	ASYNC_REQUEST_ID idRequest;			// unique id for this request
	void *pData;						// request data
	time_t timeExpire;					// time the request expires and can be cleaned up
	ULONGLONG ullRequestKey;			// user defined key for this data, currently not guaranteed unique by this file
	
	ASYNC_REQUEST *pNextBucket;			// next in bucket list
	ASYNC_REQUEST *pPrevBucket;			// prev in bucket list
		
};

//----------------------------------------------------------------------------
struct ASYNC_REQUEST_GLOBALS
{

	ASYNC_REQUEST *pBucket[ ASYNC_BUCKET_MAX + 1 ];	// the bucket list
	ASYNC_REQUEST *pRequestLastSearch;				// the request that was last searched for
		
	int nNextID;									// id allocator	
	int nNextBucket;								// bucket id allocator

	KEY_MAP keyMap;
	
};

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAsyncRequestFree( 
	GAME *pGame,
	ASYNC_REQUEST_GLOBALS *pGlobals,
	ASYNC_REQUEST *pRequest,
	BOOL bFreeRequestData)
{
	ASSERTX_RETURN( pRequest, "Expected request" );
	
	// remove from bucket list
	int nBucket = ASYNC_ID_GET_BUCKET( pRequest->idRequest );

	if (pRequest->pNextBucket)
	{
		pRequest->pNextBucket->pPrevBucket = pRequest->pPrevBucket;
	}
	if (pRequest->pPrevBucket)
	{
		pRequest->pPrevBucket->pNextBucket = pRequest->pNextBucket;
	}
	else
	{
		pGlobals->pBucket[ nBucket ] = pRequest->pNextBucket;
	}

	// clear cache
	if (pGlobals->pRequestLastSearch == pRequest)
	{
		pGlobals->pRequestLastSearch = NULL;
	}

	//	remove from the key map
	if (pRequest->ullRequestKey != INVALID_GUID)
	{
		pGlobals->keyMap.erase(pRequest->ullRequestKey);
	}
		
	// free request data
	ASSERTX( pRequest->pData, "Expected request data" );	
	if (bFreeRequestData)
	{
		if (pRequest->pData)
		{
			GFREE( pGame, pRequest->pData );
		}
	}
		
	// free request
	GFREE( pGame, pRequest );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAsyncGameGlobalsInit( 
	ASYNC_REQUEST_GLOBALS *pGlobals,
	MEMORYPOOL * pool)
{
	ASSERTX_RETURN( pGlobals, "Expected Async game globals" );
		
	// empty bucket lists
	for (int i = 0; i <= ASYNC_BUCKET_MAX; ++i)
	{
		pGlobals->pBucket[ i ] = NULL;
	}

	// clear cache
	pGlobals->pRequestLastSearch = NULL;
		
	// init it and bucket tracker
	pGlobals->nNextID = 0;
	pGlobals->nNextBucket = 0;

	// init the key map
	new (&pGlobals->keyMap) KEY_MAP(KEY_MAP::key_compare(), pool);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAsyncGameGlobalsFree( 
	GAME *pGame,
	ASYNC_REQUEST_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected Async game globals" );
	
	// free all the request
	for (int i = 0; i <= ASYNC_BUCKET_MAX; ++i)
	{
		while (pGlobals->pBucket[ i ])
		{
			sAsyncRequestFree( pGame, pGlobals, pGlobals->pBucket[ i ], TRUE );
		}
	}

	pGlobals->keyMap.clear();
	pGlobals->keyMap.~KEY_MAP();
		
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncRequestInitForGame(
	GAME *pGame)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	
	// allocate the Async game globals
	pGame->m_pAsyncRequestGlobals = (ASYNC_REQUEST_GLOBALS *)GMALLOCZ( pGame, sizeof( ASYNC_REQUEST_GLOBALS ) );

	// initialize
	sAsyncGameGlobalsInit( pGame->m_pAsyncRequestGlobals, pGame->m_MemPool );
			
	return TRUE;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncRequestFreeForGame(
	GAME *pGame)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	
	// free Async game globals
	if (pGame->m_pAsyncRequestGlobals)
	{
		sAsyncGameGlobalsFree( pGame, pGame->m_pAsyncRequestGlobals );
		pGame->m_pAsyncRequestGlobals = NULL;
	}
	
	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAsyncRequestExpireOld(
	GAME *pGame,
	int nBucket)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETURN( pGlobals, "Expected Async game globals" );
	time_t timeNow = time( 0 );	
		
	// cull any requests that have been around for too long
	ASYNC_REQUEST *pCurr;
	ASYNC_REQUEST *pNext;	
	for ( pCurr = pGlobals->pBucket[ nBucket ]; pCurr; pCurr = pNext )
	{

		// get next
		pNext = pCurr->pNextBucket;
		
		// see if we can cull this old request	
		if (timeNow > pCurr->timeExpire)
		{
			sAsyncRequestFree( pGame, pGlobals, pCurr, TRUE );
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ASYNC_REQUEST *sAsyncRequestFind(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest )
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( idRequest != INVALID_ID, "Invalid async id and key" );
	int nBucket = ASYNC_ID_GET_BUCKET( idRequest );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETNULL( pGlobals, "Expected globals" );
	
	// check the cache
	if (pGlobals->pRequestLastSearch && pGlobals->pRequestLastSearch->idRequest == idRequest)
	{
		return pGlobals->pRequestLastSearch;
	}
		
	// If we start heavily using this system, we should make this more
	// sophisticated than a linear search
	for (ASYNC_REQUEST *pRequest = pGlobals->pBucket[ nBucket ]; 
		 pRequest; 
		 pRequest = pRequest->pNextBucket)
	{
		if (pRequest->idRequest == idRequest)
		{
		
			// save to cache
			pGlobals->pRequestLastSearch = pRequest;
			
			// return result
			return pRequest;
			
		}
		
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ASYNC_REQUEST_ID sGetNextAsyncID(
	GAME *pGame,
	ASYNC_REQUEST_GLOBALS *pGlobals)
{
	ASSERTX_RETINVALID( pGame, "Expected game" );
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );
	ASYNC_REQUEST_ID idRequest = INVALID_ID;

	// we will be putting the request into this bucket
	int nBucket = pGlobals->nNextBucket++;
	
	// wrap the next bucket id
	if (pGlobals->nNextBucket > ASYNC_BUCKET_MAX)
	{
		pGlobals->nNextBucket = 0;
	}
	
	// we have a few tries to find an id that doesn't collide with an existing one
	int nTries = 32;
	while (nTries--)
	{
	
		// create the id and increment counters
		idRequest = ASYNC_ID_CREATE( pGlobals->nNextID++, nBucket );
		
		// do id wrapping
		if (pGlobals->nNextID >= ASYNC_ID_MAX)
		{
			pGlobals->nNextID = 0;		
		}

		// validate that this id will not collide with any id already allocated
		// if this system is used for a lot of requests this is a potential
		// area for optimization
		if (sAsyncRequestFind( pGame, idRequest ) == NULL)
		{
			break;
		}
		else
		{
			// failed validation
			idRequest = INVALID_ID;
		}
		
	}

	return idRequest;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ASYNC_REQUEST_ID AsyncRequestNew(
	GAME *pGame,
	void *pRequestData,
	DWORD dwRequestDataSize,
	DWORD dwTimeoutInSeconds)
{
	ASSERTX_RETINVALID( pGame, "Expected game" );
	ASSERTX_RETINVALID( pRequestData, "Expected request data" );
	ASSERTX_RETINVALID( dwRequestDataSize > 0, "Invalid request data size" );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETINVALID( pGlobals, "Expected Async game globals" );
	time_t timeNow = time( 0 );	

	// allocate a new async ID
	ASYNC_REQUEST_ID idRequest = sGetNextAsyncID( pGame, pGlobals );
	ASSERTX_RETINVALID( idRequest != INVALID_ID, "Unable to allocate async request id" );

	// use this time to expire old requests ... we will want to move this
	// out of here should we start using this system heavily
	int nBucket = ASYNC_ID_GET_BUCKET( idRequest );
	sAsyncRequestExpireOld( pGame, nBucket );
	
	// allocate new request
	ASYNC_REQUEST *pNewRequest = (ASYNC_REQUEST *)GMALLOCZ( pGame, sizeof( ASYNC_REQUEST ) );
	ASSERTX_RETINVALID( pNewRequest, "Expected request" );
	
	// allocate the request data
	pNewRequest->pData = GMALLOCZ( pGame, dwRequestDataSize );
	
	// copy request data
	memcpy( pNewRequest->pData, pRequestData, dwRequestDataSize );
	
	// assign id
	pNewRequest->idRequest = idRequest;
	pNewRequest->ullRequestKey = INVALID_GUID;

	// assign expiration time	
	pNewRequest->timeExpire = timeNow + dwTimeoutInSeconds;
	
	// tie to bucket list
	// we have a potential optimization here in that we could keep this list
	// sorted with would make searches faster
	pNewRequest->pPrevBucket = NULL;
	pNewRequest->pNextBucket = pGlobals->pBucket[ nBucket ];
	if ( pGlobals->pBucket[ nBucket ])
	{
		pGlobals->pBucket[ nBucket ]->pPrevBucket = pNewRequest;
	}
	pGlobals->pBucket[ nBucket ] = pNewRequest;
	
	return pNewRequest->idRequest;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncRequestDelete(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest,
	BOOL bFreeRequestData /*= TRUE*/)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( idRequest != INVALID_ID, "Invalid async id" );
	
	// find the request
	BOOL bFound = FALSE;
	ASYNC_REQUEST *pRequest = sAsyncRequestFind( pGame, idRequest );
	if (pRequest)
	{
		sAsyncRequestFree( pGame, pGame->m_pAsyncRequestGlobals, pRequest, bFreeRequestData );
		bFound = TRUE;
	}
	
	// return if we found the request and deleted it
	return bFound;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void *AsyncRequestGetData(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( idRequest != INVALID_ID, "Invalid async id" );
	ASYNC_REQUEST *pRequest = sAsyncRequestFind( pGame, idRequest );	
	if (pRequest)
	{
		return pRequest->pData;
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncRequestNewByKey(
	GAME *pGame,
	ULONGLONG ullKey,
	void *pRequestData,
	DWORD dwRequestDataSize,
	DWORD dwTimeoutInSeconds )
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( ullKey != INVALID_GUID, "Invalid async request key." );
	ASSERTX_RETFALSE( pRequestData, "Expected request data" );
	ASSERTX_RETFALSE( dwRequestDataSize > 0, "Invalid request data size" );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETFALSE( pGlobals, "Expected Async game globals" );

	ASYNC_REQUEST_ID newId = AsyncRequestNew(pGame, pRequestData, dwRequestDataSize, dwTimeoutInSeconds);
	ASSERT_RETFALSE(newId != INVALID_ID);

	ASYNC_REQUEST * entry = sAsyncRequestFind(pGame, newId);
	ASSERT_RETFALSE(entry);
	entry->ullRequestKey = ullKey;

	if (!pGlobals->keyMap.insert(std::make_pair(ullKey, newId)).second)
	{
		sAsyncRequestFree(pGame, pGlobals, entry, TRUE);
		return FALSE;
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AsyncRequestDeleteByKey(
	GAME *pGame,
	ULONGLONG ullKey,
	BOOL bFreeRequestData /*= TRUE*/)
{
	ASSERTX_RETFALSE( pGame, "Expected game." );
	ASSERTX_RETFALSE( ullKey != INVALID_GUID, "Invalid key value specified." );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETFALSE( pGlobals, "Expected Async game globals" );

	KEY_MAP::const_iterator itr = pGlobals->keyMap.find(ullKey);
	if (itr == pGlobals->keyMap.end())
		return FALSE;

	return AsyncRequestDelete(pGame, itr->second, bFreeRequestData);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void *AsyncRequestGetDataByKey(
	GAME *pGame,
	ULONGLONG ullKey )
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( ullKey != INVALID_GUID, "Invalid async key" );
	ASYNC_REQUEST_GLOBALS *pGlobals = pGame->m_pAsyncRequestGlobals;
	ASSERTX_RETNULL( pGlobals, "Expected Async game globals" );

	KEY_MAP::const_iterator itr = pGlobals->keyMap.find(ullKey);
	if (itr == pGlobals->keyMap.end())
		return NULL;

	return AsyncRequestGetData(pGame, itr->second);

}