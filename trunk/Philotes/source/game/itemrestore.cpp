//----------------------------------------------------------------------------
// FILE: itemrestore.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "asyncrequest.h"
#include "dbunit.h"
#include "GameServer.h"
#include "itemrestore.h"
#include "player.h"
#include "s_playerEmail.h"
#include "ServerRunnerAPI.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameRequestMsgs.h"
#include "utilitygame.h"

//----------------------------------------------------------------------------
// LOAD DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemRestoreSendResult(
	GAME *pGame,
	const struct UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest,	
	BOOL bSuccess,
	const char *pszMessage /*= NULL*/)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pRestoreRequest, "Expected restore request" );

	// setup message	
	MSG_GAMESERVER_TO_CSRBRIDGE_ITEM_RESTORE_RESULT tMessage;
	tMessage.bSuccess = (BYTE)bSuccess;
	tMessage.dwCSRPipeID = pRestoreRequest->dwCSRPipeID;
	if (pszMessage)
	{
		PStrCopy( tMessage.szMessage, pszMessage, MAX_CSR_RESULT_MESSAGE );
	}
		
	// send it
	SRNET_RETURN_CODE eResult = 
		SvrNetSendRequestToService(
			ServerId( CSR_BRIDGE_SERVER, 0 ), 
			&tMessage, 
			GAMESERVER_TO_CSRBRIDGE_ITEM_RESTORE_RESULT);
	ASSERTX( eResult == SRNET_SUCCESS, "Unable to send csr item restore result to bridge" );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemRestoreComplete(
	GAME *pGame,
	ASYNC_REQUEST_ID idRequest,
	BOOL bSuccess,
	const char *pszMessage /*= NULL*/)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( idRequest != INVALID_ID, "Invalid item restore request id" );	
	
	// get the async request data
	UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest = (UTILITY_GAME_CSR_ITEM_RESTORE *)AsyncRequestGetData( pGame, idRequest );
	if (pRestoreRequest)
	{

		// send the result to the CSR bridge
		s_ItemRestoreSendResult( pGame, pRestoreRequest, bSuccess, pszMessage );

		// free the item if it's still around
		UNIT *pItem = UnitGetById( pGame, pRestoreRequest->idTempRestoreItem );
		if (pItem)
		{
			ASSERTX( bSuccess == FALSE, "CSR item still exists in a success case, but should have already been removed from the server after successfull move into escrow" );
			UnitFree( pItem );
			pItem = NULL;
		}
		
		// free the request
		AsyncRequestDelete( pGame, idRequest, TRUE );
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemRestore(
	GAME *pGame,
	const UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreRequest,
	ASYNC_REQUEST_ID idRequest)
{
	ASSERTX_RETFALSE( pGame, "Expectd game" );
	ASSERTX_RETFALSE( pRestoreRequest, "Expected restore request" );
	ASSERTX_RETFALSE( idRequest != INVALID_ID, "Expected async request id" );
	
#if ISVERSION(SERVER_VERSION)

	// issue request to load the item data from the warehouse database
	if (PlayerRestoreItemFromWarehouseDatabase( 
			pRestoreRequest->guidCharacter, 
			pRestoreRequest->guidItem,
			idRequest) == FALSE)
	{
		s_ItemRestoreComplete( pGame, idRequest, FALSE, "Unable to issue load request to warehouse DB" );
	}
	
#endif // ISVERSION(SERVER_VERSION)

	return TRUE;
	
}

#if ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRestoreItemAddedToDB(
	GAME *pGame,
	BOOL bSuccess,
	const BYTE *pCallbackData)
{
	ASYNC_REQUEST_ID idRequest = *(ASYNC_REQUEST_ID *)pCallbackData;

	// get the restore request data from the async system
	UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreData = (UTILITY_GAME_CSR_ITEM_RESTORE * )AsyncRequestGetData( pGame, idRequest );
	ASSERTX_RETURN( pRestoreData, "Unable to find async data after restore item add" );

	// get the item
	UNIT *pItem = UnitGetById( pGame, pRestoreData->idTempRestoreItem );
	if (pItem == NULL)
	{
		s_ItemRestoreComplete( pGame, idRequest, FALSE, "Unable to find restore item after database add" );
		goto _ERROR;
	}
	else
	{
				
		if (bSuccess == TRUE)
		{
						
			// email the item to the player
			if (s_PlayerEmailCSRSendItemRestore( pGame, pItem, pRestoreData, idRequest ) == FALSE)
			{
			
				// send failure to bridge
				s_ItemRestoreComplete( pGame, idRequest, FALSE, "Unable to send item restore email" );
				
				// remove from db
				UnitRemoveFromDatabase( 
					UnitGetGuid( pItem ), 
					UnitGetUltimateOwnerGuid( pItem ), 
					UnitGetData( pItem ));

				goto _ERROR;
								
			}
		
		}
		else
		{
			s_ItemRestoreComplete( pGame, idRequest, FALSE, "Error adding restore item to database" );
			goto _ERROR;			
		}

	}
	
	return;
	
_ERROR:

	// free the item on error
	UnitFree( pItem );

	// free async request data
	AsyncRequestDelete( pGame, idRequest, TRUE );
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemRestorePostProcessNewUnit( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// clear any owner
	UnitSetOwner( pUnit, NULL );
	
	// clear stats that are marked as such when restoring items
	GAME *pGame = UnitGetGame( pUnit );
	int nNumStats = ExcelGetNumRows( pGame, DATATABLE_STATS );
	for (int i = 0; i < nNumStats; ++i)
	{
		const STATS_DATA *pStatsData = StatsGetData( pGame, i );
		if (pStatsData && StatsDataTestFlag( pStatsData, STATS_DATA_CLEAR_ON_ITEM_RESORE ))
		{
			UnitClearStat( pUnit, i, 0 );
		}
	}
	
	// make item ok to be in a email slot (even if it's no trade, or quest or no drop, whatever)
	UnitSetStat( pUnit, STATS_ALWAYS_ALLOW_IN_EMAIL_SLOTS, TRUE );
	
	// if unit is subscriber only, but account is not a subscriber, 
	// that's a failure, kill the unit?  or is it?  not sure -cday
	
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemRestoreDatabaseLoadComplete( 
	GAME *pGame,
	const CLIENT_SAVE_FILE &tClientSaveFile)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	BOOL bSuccess = FALSE;

#if ISVERSION(SERVER_VERSION)

	// setup flags for a restore read
	DWORD dwReadUnitFlags = 0;
	SETBIT( dwReadUnitFlags, RUF_USE_DBUNIT_GUID_BIT, TRUE );
	SETBIT( dwReadUnitFlags, RUF_RESTORED_ITEM_BIT, TRUE );
	
	// load the items
	DATABASE_UNIT_COLLECTION_RESULTS tResults;
	if (s_DBUnitsLoadFromClientSaveFile( 
			pGame, 
			NULL,
			NULL, 
			&tClientSaveFile, 
			&tResults,
			dwReadUnitFlags))
	{

		// post process these units for loading
		for (int i = 0; i < tResults.nNumUnits; ++i)
		{
			UNIT *pUnit = tResults.pUnits[ i ];
			
			// disable db ops
			s_DatabaseUnitEnable( pUnit, FALSE );
			
			// do post processing
			sItemRestorePostProcessNewUnit( pUnit );
						
		}
	
		// must have an async load request to examine
		ASSERTX_GOTO( tResults.idAsyncRequest != INVALID_ID, "Expected async request id for item restore in result structure", _CLEANUP );
		
		// must have one unit
		if (tResults.nNumUnits != 1)
		{
			s_ItemRestoreComplete( pGame, tResults.idAsyncRequest, FALSE, "More than 1 item retrieved in the restoration process" );
			goto _CLEANUP;
		}
		
		// get the item to restore
		UNIT *pItem = tResults.pUnits[ 0 ];
		if (pItem == NULL) 
		{
			s_ItemRestoreComplete( pGame, tResults.idAsyncRequest, FALSE, "Item is NULL in the db collection results" );
			goto _CLEANUP;
		}
		
		// get the restore request data from the async system
		UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreData = (UTILITY_GAME_CSR_ITEM_RESTORE * )AsyncRequestGetData( pGame, tResults.idAsyncRequest );
		if (pRestoreData == NULL)
		{
			s_ItemRestoreComplete( pGame, tResults.idAsyncRequest, FALSE, "Item is NULL in the db collection results" );
			goto _CLEANUP;
		}

		// save the item unitid so we can get them later
		pRestoreData->idTempRestoreItem = UnitGetId( pItem );
		
		// create a database unit for immediate commit to the database, note that
		DATABASE_UNIT tDBUnit;
		BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		int nValidationErrors = 0;
		if (s_DBUnitCreate( 
				pItem, 
				&tDBUnit, 
				bDataBuffer, 
				arrsize( bDataBuffer ), 
				nValidationErrors, 
				TRUE) == FALSE)
		{
			s_ItemRestoreComplete( pGame, tResults.idAsyncRequest, FALSE, "Unable to create db unit for add" );
			goto _CLEANUP;
		}

		// fill out a callback to receive in game after the DB add is complete
		DB_GAME_CALLBACK tCallback;
		tCallback.pServerContext = (GameServerContext *)CurrentSvrGetContext();
		tCallback.idGame = GameGetId( pGame );
		tCallback.fpGameCompletionCallback = sRestoreItemAddedToDB;
		ASSERT( sizeof( tCallback.bCallbackData ) >= sizeof( tResults.idAsyncRequest ));
		memcpy( tCallback.bCallbackData, &tResults.idAsyncRequest, sizeof( tResults.idAsyncRequest ) );
		
		// add the unit to the database		
		UnitAddToDatabase( pItem, &tDBUnit, bDataBuffer, UDC_ITEM_RESTORE, &tCallback );
						
		// email has been queued successfully
		bSuccess = TRUE;
								
	}

_CLEANUP:

	// if unsuccessfully, clean up after ourselves
	if (bSuccess == FALSE)
	{
		
		// remove the async request data
		if (tResults.idAsyncRequest != INVALID_ID)
		{
			AsyncRequestDelete( pGame, tResults.idAsyncRequest, TRUE );
		}

		// free any items in the collection we had loaded for restoring
		for (int i = 0; i < tResults.nNumUnits; ++i)
		{
			UnitFree( tResults.pUnits[ i ] );
		}

	}

#endif  // ISVERSION(SERVER_VERSION)
		
	return bSuccess;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemRestoreDBUnitCollectionReadyForRestore( 
	GAME *pGame, 
	struct DATABASE_UNIT_COLLECTION *pDBUnitCollection)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pDBUnitCollection, "Expected db unit collection" );
	ASSERTX_RETURN( pDBUnitCollection->idAsyncRequest != INVALID_ID, "Expected async request in db unit collection for item restore" );

	// find the data for this item restore request
	UTILITY_GAME_CSR_ITEM_RESTORE *pRestoreData = (UTILITY_GAME_CSR_ITEM_RESTORE * )AsyncRequestGetData( pGame, pDBUnitCollection->idAsyncRequest );
	ASSERTX_RETURN( pRestoreData, "Unable to find async restore data for item resore" );
		
	// go through each item and fix up the GUIDs so that every unit in this
	// unit collection is unique and can actually be instanced
	for (int i = 0; i < pDBUnitCollection->nNumUnits; ++i)
	{
		DATABASE_UNIT *pDBUnit = &pDBUnitCollection->tUnits[ i ];
					
		// change this GUID
		PGUID guidOld = pDBUnit->guidUnit;
		pDBUnit->guidUnit = GameServerGenerateGUID();

		// clear old ultimate owner for consistency
		pDBUnit->guidUltimateOwner = INVALID_GUID;
				
		// change any units that were contained by this GUID to be 
		// contained by the new GUID
		BOOL bFoundContainedItem = FALSE;
		for (int j = 0; j < pDBUnitCollection->nNumUnits; ++j)
		{
			DATABASE_UNIT *pDBUnitContained = &pDBUnitCollection->tUnits[ j ];
			if (pDBUnitContained->guidContainer == guidOld)
			{
				pDBUnitContained->guidContainer = pDBUnit->guidUnit;
				bFoundContainedItem = TRUE;
			}
		}				

		// if we didn't find any items are contained by this dbunit, it means
		// that this unit is a "top level" unit that should be contained
		// by the new owner in a well known location for item restore
		if (bFoundContainedItem == FALSE)
		{
		
			// clear guid that this unit is contained by
			pDBUnit->guidContainer = INVALID_GUID;

			// NOTE that we're not changing the inv location of the db unit
			// here because the xfer functions have no container unit to put
			// it inside of anyway
			
		}
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemRestoreAttachmentError( 
	GAME *pGame, 
	UNIT *pItem)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pItem, "Expected attachment" );
	
	// remove this unit from the database
	UnitRemoveFromDatabase( 
		UnitGetGuid( pItem ), 
		UnitGetUltimateOwnerGuid( pItem ), 
		UnitGetData( pItem ));

	// remove the item
	UnitFree( pItem );
	pItem = NULL;
		
}
