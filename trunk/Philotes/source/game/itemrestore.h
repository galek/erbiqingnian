//----------------------------------------------------------------------------
// FILE: itemrestore.h
//----------------------------------------------------------------------------

#ifndef __ITEM_RESTORE_H_
#define __ITEM_RESTORE_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
	
void s_ItemRestoreSendResult(
	GAME *pGame,
	const struct UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest,	
	BOOL bSuccess,
	const char *pszMessage = NULL);
	
void s_ItemRestoreComplete(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest,
	BOOL bSuccess,
	const char *pszMessage = NULL);
	
BOOL s_ItemRestore(
	GAME *pGame,
	const struct UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest,
	ASYNC_REQUEST_ID idRequest);

BOOL s_ItemRestoreDatabaseLoadComplete( 
	GAME *pGame,
	const struct CLIENT_SAVE_FILE &tClientSaveFile);

void s_ItemRestoreDBUnitCollectionReadyForRestore( 
	GAME *pGame, 
	struct DATABASE_UNIT_COLLECTION *pDBUnitCollection);
		
void s_ItemRestoreAttachmentError( 
	GAME *pGame, 
	UNIT *pAttachment);

#endif