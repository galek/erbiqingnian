//----------------------------------------------------------------------------
// FILE: asysncrequest.h
// Simplistic place to store data for long requests within a game
//
// TODO: Sort the requests into buckets to make searching faster
// TODO: Expire requests more efficiently
// TODO: Remove the game restriction and make it a threadsafe app resource
//----------------------------------------------------------------------------

#ifndef __ASYNC_REQUEST_H_
#define __ASYNC_REQUEST_H_

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define ASYNC_REQUEST_DEFAULT_TIMEOUT_IN_SECONDS 120

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL AsyncRequestInitForGame(
	GAME *pGame);

BOOL AsyncRequestFreeForGame(
	GAME *pGame);

ASYNC_REQUEST_ID AsyncRequestNew(
	GAME *pGame,
	void *pRequestData,
	DWORD dwRequestDataSize,
	DWORD dwTimeoutInSeconds = ASYNC_REQUEST_DEFAULT_TIMEOUT_IN_SECONDS);

BOOL AsyncRequestDelete(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest,
	BOOL bFreeRequestData = TRUE);

void *AsyncRequestGetData(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest);

BOOL AsyncRequestNewByKey(
	GAME *pGame,
	ULONGLONG ullKey,
	void *pRequestData,
	DWORD dwRequestDataSize,
	DWORD dwTimeoutInSeconds = ASYNC_REQUEST_DEFAULT_TIMEOUT_IN_SECONDS);

BOOL AsyncRequestDeleteByKey(
	GAME *pGame,
	ULONGLONG ullKey,
	BOOL bFreeRequestData = TRUE);

void *AsyncRequestGetDataByKey(
	GAME *pGame,
	ULONGLONG ullKey );
	
#endif