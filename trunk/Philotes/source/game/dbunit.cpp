//----------------------------------------------------------------------------
// FILE: dbunit.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "bookmarks.h"
#include "console.h"
#include "database.h"
#include "items.h"
#include "itemrestore.h"
#include "unitfile.h"
#include "dbunit.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "gameunits.h"
#include "guidcounters.h"
#include "player.h"
#include "playertrace.h"

#if ISVERSION(SERVER_VERSION)
#include "playertrace.h"
#include "dbunit.cpp.tmh"
#include "GameEventClass_PETSdef.h"
#include "ServerRunnerAPI.h"
#endif

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
#if !VALIDATE_UNIQUE_GUIDS	//As DB unit validation duplicates items for testing purposes, it's incompatible with dupe checking.
//#define VALIDATE_DB_UNITS
#endif

static const int NUM_UNIT_BUCKETS = 16;				// buckets for unit db operations
static const int MAX_DB_MULTIPLIER = 10;			// maximum database transaction iterations (LOAD TESTING)
static int		 NUM_DB_TRANSACTIONS = 1;			// number of times to multiply a database transaction (LOAD TESTING)
static DWORD     INVLOC_DBOP_DELAY = 600;			// number of ticks to delay commits for item inventory location changes
static DWORD     QUANTITY_DBOP_DELAY = 600;			// number of ticks to delay commits for item quantity changes
static DWORD     GLOBAL_DBOP_DELAY = 0;             // number of ticks to delay all unit db operations

static const BOOL sgbDatabaseUnitsEnabled = TRUE;	// turn this on to enable new database saving
static const BOOL sgbTrackUnitsInDatabase = TRUE;	// turn this when enabling the database to test if the game is properly tracking if units are in the database or not

// the following are flags turn on to help debug incremental saving and DB logic
static		 BOOL sgbIncrementalDBUnitsEnabled		= TRUE;	// enable this to do incremental saving
static		 BOOL sgbDBUnitsUseFieldsEnabled		= TRUE;	// enable this to do write directly to db unit fields instead of writing whole blobs (when possible)
static		 BOOL sgbDBUnitLoadMultiplyEnabled		= FALSE;	// enable this to do incremental saving (LOAD TESTING)
static const BOOL sgbAppRequiresDatabaseForLogic	= TRUE;		// when true, the DB log requires the app using the actual database
static const BOOL sgbMarkUnitsInDatabaseOnLoad		= FALSE;	// turn this on to mark a player and their inventory as in the database for debugging
			 BOOL gbDBUnitLogEnabled				= TRUE;	// turn this on to see trace messages of the db ops
			 BOOL gbDBUnitLogInformClients			= FALSE;	// with this on, the server will send db log messages to all clients

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct UNIT_INFO
{
	const UNIT_DATA *pUnitData;									// unit data (for debugging)	
	GENUS eGenus;												// unit genus
	const char *pszQuality;										// unit quality name (only useful for items)
	WCHAR uszItemInfo[MAX_ITEM_PLAYER_LOG_STRING_LENGTH];		// human-readable info text for items
};

//----------------------------------------------------------------------------
struct PENDING_OPERATION
{
	UNITID idUnit;						// unitid the operation is for
	PGUID guidUnit;						// GUID of unit
	PGUID guidUltimateOwner;			// GUID of ultimate owner of unit
	UNIT_INFO tUnitInfo;				// other unit info (for player action logging and debugging)
	GAMECLIENTID idClient;				// the client that is causing this operation
	DATABASE_OPERATION eOperation;		// the operation
	DBOPERATION_CONTEXT opContext;		// the action that is causing this operation, plus associated metadata
	PENDING_OPERATION *pNext;			// next operation in this bucket
	PENDING_OPERATION *pPrev;			// prev operation in this bucket
	DWORD dwTickCommit;					// the tick to commit this operation on
	DWORD dwDBUnitUpdateFieldFlags;		// see DBUNIT_FIELD
	DWORD dwDatabaseOperationFlags;		// see DATABASE_OPERATION_FLAG enum
	
	BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];		// data buffer for the DB unit
	DATABASE_UNIT tDBUnit;									// the DB unit itself
	
};

//----------------------------------------------------------------------------
struct PENDING_OPERATION_BUCKET
{
	PENDING_OPERATION *pOperations;
};

//----------------------------------------------------------------------------
struct DBUNIT_GLOBALS
{
	PENDING_OPERATION_BUCKET tBuckets[ NUM_UNIT_BUCKETS ];
};

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DatabaseUnitsAreEnabled(
	void)
{
	return sgbDatabaseUnitsEnabled;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_IncrementalDBUnitsAreEnabled(
	void)
{
	return sgbIncrementalDBUnitsEnabled;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VOID s_SetIncrementalSave(
	BOOL useIncrementalDBUnits)
{
	if(useIncrementalDBUnits)
	{
		sgbIncrementalDBUnitsEnabled = TRUE;
	}
	else
	{
		sgbIncrementalDBUnitsEnabled = FALSE;
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VOID s_SetDBUnitLoadMultiply(
	BOOL useDBLoadMultiply)
{
	if(useDBLoadMultiply)
	{
		sgbDBUnitLoadMultiplyEnabled = TRUE;
		NUM_DB_TRANSACTIONS = MAX_DB_MULTIPLIER;
	}
	else
	{
		sgbDBUnitLoadMultiplyEnabled = FALSE;
		NUM_DB_TRANSACTIONS = 1;
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD s_GetDBUnitTickDelay(
	DBUNIT_FIELD eField)
{
	switch (eField)
	{
	case DBUF_INVENTORY_LOCATION:
		return INVLOC_DBOP_DELAY;
		break;
	case DBUF_QUANTITY:
		return QUANTITY_DBOP_DELAY;
		break;
	default:
		return GLOBAL_DBOP_DELAY;
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTrackUnitsInDatabase(
	void)
{
	return sgbTrackUnitsInDatabase;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PENDING_OPERATION_BUCKET *sGetPendingOperationBucket(
	GAME *pGame,
	UNITID idUnit)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	int nBucketIndex = idUnit % NUM_UNIT_BUCKETS;
	DBUNIT_GLOBALS *pGlobals = pGame->m_pDBUnitGlobals;	
	ASSERTX_RETNULL( pGlobals, "Expected DBUNIT_GLOBALS" );
	ASSERTX_RETNULL( nBucketIndex >= 0 && nBucketIndex < arrsize( pGlobals->tBuckets ), "array index out of range" );
	return &pGlobals->tBuckets[ nBucketIndex ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DBUNIT_GLOBALS *sGetDBUnitGlobals(
	GAME *pGame)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	DBUNIT_GLOBALS *pGlobals = pGame->m_pDBUnitGlobals;
	ASSERTX_RETNULL( pGlobals, "Expected pending database operation globals" );
	return pGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PENDING_OPERATION *sFindPendingOperation(
	GAME *pGame,
	UNITID idUnit,
	PENDING_OPERATION_BUCKET *pBucket)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( idUnit != INVALID_ID, "Invalid unit id" );
	ASSERTX_RETNULL( pBucket, "Expected bucket" );
	
	PENDING_OPERATION *pPending = pBucket->pOperations;
	while (pPending)
	{
		if (pPending->idUnit == idUnit)
		{
			return pPending;
		}
		pPending = pPending->pNext;
	}
	return NULL;  // not found
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetCommitTick(
	GAME *pGame,
	PENDING_OPERATION *pPending,
	DWORD dwTickCommit,
	BOOL bMinOfExisting)
{

	// if the operation is an update or remove, it is always immediate
	if (pPending->eOperation == DBOP_ADD || pPending->eOperation == DBOP_REMOVE)
	{
		pPending->dwTickCommit = GameGetTick( pGame );
	}
	else
	{
			
		// take the sooner of the commit times
		if (bMinOfExisting)
		{
			pPending->dwTickCommit = MIN( pPending->dwTickCommit, dwTickCommit );
		}
		else
		{
			pPending->dwTickCommit = dwTickCommit;
		}

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sIncorporateFieldUpdate(
	PENDING_OPERATION *pOperation,
	DBUNIT_FIELD eField)
{
	ASSERTX_RETURN( pOperation, "Expected pending operation" );
	
	if (eField == DBUF_INVALID)
	{
		// by default do a full update when the commit time comes
		SETBIT( pOperation->dwDBUnitUpdateFieldFlags, DBUF_FULL_UPDATE );
	}
	else
	{
		// we have a specific operation we want to do during the update, set a bit for it only
		SETBIT( pOperation->dwDBUnitUpdateFieldFlags, eField );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSetPendingDBUnitForDeleteOperation( 
	PENDING_OPERATION *pPending, 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pPending, "Expected pending operation" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// for pending operations that are a remove operation, it's very likely
	// that the unit that is being removed is not going to be around at the 
	// end of the frame when it comes time to submit the remove to the
	// database.  This is all fine and good if all we're doing is just
	// marking a unit as removed in the database, but for item logging we want
	// the full data associated with this unit, and this is our only chance to
	// grab it because the unit could be deleted after this point
	if (pPending->eOperation == DBOP_REMOVE)
	{
	
		int nValidationErrors = 0;	
		if (s_DBUnitCreate( 
					pUnit, 
					&pPending->tDBUnit, 
					pPending->bDataBuffer, 
					arrsize( pPending->bDataBuffer ), 
					nValidationErrors, 
					TRUE) == FALSE)
		{
			ASSERTV_RETFALSE( 
				0, 
				"Unable to create db unit for unit (%s) [id=%d]", 
				UnitGetDevName( pUnit ), 
				UnitGetId( pUnit ));
		}

		// set a bit that says we have this dbunit information and that it's good to use
		pPending->dwDatabaseOperationFlags |= DOF_HAS_DELETE_DB_UNIT;
		
	}
	
	return TRUE;  // successfully set the db unit or it doens't matter for this operation
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePendingOperation(
	GAME *pGame,
	PENDING_OPERATION *pPending,
	PENDING_OPERATION_BUCKET *pBucket)
{
	ASSERTX_RETURN( pPending, "Expected pending operation" );

	if (pPending->pNext)
	{
		pPending->pNext->pPrev = pPending->pPrev;
	}
	if (pPending->pPrev)
	{
		pPending->pPrev->pNext = pPending->pNext;
	}
	else
	{
		pBucket->pOperations = pPending->pNext;
	}
	
	// free the pending operation
	GFREE( pGame, pPending );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChangePendingOperation(
	GAME *pGame,
	UNIT *pUnit,
	PENDING_OPERATION *pPending,
	PENDING_OPERATION_BUCKET *pBucket,
	DATABASE_OPERATION eOperation,
	DBOPERATION_CONTEXT* pOpContext,
	DBUNIT_FIELD eField,
	DWORD dwTickCommit,
	DWORD dwDatabaseOperationFlags)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pPending, "Expected pending operation" );
	ASSERTX_RETURN( pBucket, "Expected pending operation bucket" );
	ASSERTX_RETURN( eOperation != DBOP_INVALID, "Invalid database operation" );
	
	// set the new operation
	pPending->eOperation = eOperation;
	
	// set the new operation context
	if (pOpContext)
	{
		pPending->opContext = *pOpContext;
	}
	
	// set the new commit time
	sSetCommitTick( pGame, pPending, dwTickCommit, TRUE );

	// incorporate any update operation that was requested (if any)
	sIncorporateFieldUpdate( pPending, eField );

	// combine the operation flags, we could make the combination more sophisticated
	// if a union of all operations is not appropriate future options
	pPending->dwDatabaseOperationFlags |= dwDatabaseOperationFlags;

	// update the database unit for this operation
	sSetPendingDBUnitForDeleteOperation( pPending, pUnit );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitInfoInit(
	UNIT_INFO &tUnitInfo,
	UNIT *pUnit)
{

	// init
	tUnitInfo.pUnitData = NULL;
	tUnitInfo.eGenus = GENUS_NONE;
	tUnitInfo.pszQuality = NULL;
	tUnitInfo.uszItemInfo[ 0 ] = 0;
		
	// set data based on unit (if we have one)
	if (pUnit)
	{
	
		// generic unit information
		tUnitInfo.pUnitData = UnitGetData( pUnit );
		tUnitInfo.eGenus = UnitGetGenus( pUnit );
		
		// item specific information
		if ( UnitIsA( pUnit, UNITTYPE_ITEM ) )
		{
			tUnitInfo.pszQuality = ItemGetQualityName( pUnit );
			ItemGetPlayerLogString( pUnit, tUnitInfo.uszItemInfo, MAX_ITEM_PLAYER_LOG_STRING_LENGTH );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddPendingOperation( 
	UNIT *pUnit,
	DATABASE_OPERATION eOperation,
	DBOPERATION_CONTEXT* pOpContext,
	DBUNIT_FIELD eField,
	UNIT *pUltimateOwner,
	DWORD dwTickCommit,
	DWORD dwDatabaseOperationFlags)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( eOperation != DBOP_INVALID, "Invalid database operation" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( pGame, "Expected game" );
	UNITID idUnit = UnitGetId( pUnit );
		
	// which bucket will it go in
	PENDING_OPERATION_BUCKET *pBucket = sGetPendingOperationBucket( pGame, idUnit );
	ASSERTX_RETURN( pBucket, "Expected PENDING_OPERATION_BUCKET" );
	
	// debug
	#ifdef _DEBUG
	{
		PENDING_OPERATION *pPending = sFindPendingOperation( pGame, idUnit, pBucket );
		if (pPending != NULL)
		{
			ASSERTX( 0, "Adding a pending database operation for unit, but we already found one" );
			sChangePendingOperation( 
				pGame, 
				pUnit,
				pPending, 
				pBucket,
				eOperation, 
				pOpContext, 
				eField, 
				dwTickCommit, 
				dwDatabaseOperationFlags);
			return;
		}
	}
	#endif
	
	// allocate new pending op
	PENDING_OPERATION *pNewPending = (PENDING_OPERATION *)GMALLOC( pGame, sizeof( PENDING_OPERATION ) );
	pNewPending->idUnit = idUnit;
	pNewPending->guidUnit = UnitGetGuid( pUnit );
	pNewPending->guidUltimateOwner = UnitGetGuid(pUltimateOwner);
	
	// init info about this unit
	sUnitInfoInit( pNewPending->tUnitInfo, pUnit );

	pNewPending->eOperation = eOperation;
	if (pOpContext)
	{
		pNewPending->opContext = *pOpContext;
	}
	else
	{
		// initialize the op context struct
		pNewPending->opContext.eContext = DBOC_INVALID;
		pNewPending->opContext.idAccount = INVALID_ID;
		pNewPending->opContext.guidCharacter = INVALID_GUID;
		pNewPending->opContext.wszCharacterName[0] = L'\0';
		pNewPending->opContext.wszUnitDisplayName[0] = L'\0';
		pNewPending->opContext.bIsLogged = FALSE;
	}
	pNewPending->dwTickCommit = 0;
	pNewPending->dwDBUnitUpdateFieldFlags = 0;
	pNewPending->dwDatabaseOperationFlags = dwDatabaseOperationFlags;

	// set bit for update type (if present)
	sIncorporateFieldUpdate( pNewPending, eField );
	
	// set the commit tick
	sSetCommitTick( pGame, pNewPending, dwTickCommit, FALSE );

	// update the database unit for this operation
	sSetPendingDBUnitForDeleteOperation( pNewPending, pUnit );
				
	// add to bucket list
	pNewPending->pPrev = NULL;
	pNewPending->pNext = pBucket->pOperations;
	if (pBucket->pOperations)
	{
		pBucket->pOperations->pPrev = pNewPending;
	}
	pBucket->pOperations = pNewPending;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBGlobalsInit(
	DBUNIT_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected db unit globals" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBGlobalsFree(
	GAME *pGame,
	DBUNIT_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected db unit globals" );
	
	// free any pending requests from all buckets
	for (int i = 0; i < NUM_UNIT_BUCKETS; ++i)
	{
		PENDING_OPERATION_BUCKET *pBucket = &pGlobals->tBuckets[ i ];
		while (pBucket->pOperations)
		{
			sRemovePendingOperation( pGame, pBucket->pOperations, pBucket );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DBUnitInit(
	DATABASE_UNIT *pDBUnit)
{
	ASSERTX_RETURN( pDBUnit, "Expected DB unit" );
	
	pDBUnit->dwCRC = 0;
	pDBUnit->guidUnit = INVALID_GUID;
	pDBUnit->spSpecies = SPECIES_NONE;
	pDBUnit->eGenus = GENUS_NONE;
	pDBUnit->dwClassCode = INVALID_CODE;
	pDBUnit->szName[ 0 ] = 0;

	pDBUnit->dwSecondsPlayed = 0;
	pDBUnit->dwMoney = 0;
	pDBUnit->dwExperience = 0;
	pDBUnit->dwQuantity = 0;
	pDBUnit->dwRankExperience = 0;
	pDBUnit->dwRank = 0;

	pDBUnit->guidUltimateOwner = INVALID_GUID;
	pDBUnit->guidContainer = INVALID_GUID;
	
	DBUNIT_INVENTORY_LOCATION &tDBUnitInvLoc = pDBUnit->tDBUnitInvLoc;
	tDBUnitInvLoc.dwInvLocationCode = INVALID_CODE;
	tDBUnitInvLoc.tInvLoc.nInvLocation = INVLOC_NONE;
	tDBUnitInvLoc.tInvLoc.nX = 0;
	tDBUnitInvLoc.tInvLoc.nY = 0;

	pDBUnit->dwDataIndex = 0;
	pDBUnit->dwDataSize = 0;
	
	pDBUnit->pUnitLoadResult = NULL;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DatabaseUnitsInitForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pGame->m_pDBUnitGlobals == NULL, "DB Unit globals already exist" );
	
	// allocate new globals
	DBUNIT_GLOBALS *pGlobals = (DBUNIT_GLOBALS *)GMALLOCZ( pGame, sizeof ( DBUNIT_GLOBALS ) );
	sDBGlobalsInit( pGlobals );
	
	// save in game
	pGame->m_pDBUnitGlobals = pGlobals;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAnyCommitsExist(
	DBUNIT_GLOBALS *pGlobals)
{
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );
	
	for (int i = 0; i < NUM_UNIT_BUCKETS; ++i)
	{
		const PENDING_OPERATION_BUCKET *pBucket = &pGlobals->tBuckets[ i ];
		if (pBucket->pOperations != NULL)
		{
			return TRUE;
		}
		
	}

	return FALSE;  // no pending operations exist	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DatabaseUnitsFreeForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	
	// check to see if we have globals
	if (pGame->m_pDBUnitGlobals != NULL)
	{
		DBUNIT_GLOBALS *pGlobals = sGetDBUnitGlobals( pGame );

		// there should be no pending commits for the database or we will lose those actions
		ASSERTX( sAnyCommitsExist( pGlobals ) == FALSE, "Pending database commits exist in game logic ... queries must be sent to database system before game shutdown or the transactions will be lost!" );
		
		// free globals
		sDBGlobalsFree( pGame, pGlobals );
		GFREE( pGame, pGame->m_pDBUnitGlobals );
		pGame->m_pDBUnitGlobals = NULL;
		
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DatabaseUnitCollectionInit(
	const WCHAR *puszCharacterName,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	ASYNC_REQUEST_ID idAsyncRequest /*= INVALID_ID*/)
{
	ASSERTX_RETURN( puszCharacterName, "Expected character name" );
	ASSERTX_RETURN( pDBUCollection, "Expected db unit collection" );
	
	// initialize
	PStrCopy( pDBUCollection->uszCharacterName, puszCharacterName, MAX_CHARACTER_NAME );
	pDBUCollection->dwUsedBlobDataSize = 0;
	pDBUCollection->nNumUnits = 0;
	pDBUCollection->dwStatus = 0;
	pDBUCollection->idAsyncRequest = idAsyncRequest;
	
	// we could zero the memory for the unit array and blob storage, but what's the point?
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreeDatabaseUnit( 
	DATABASE_UNIT *pDBUnit)
{
	pDBUnit->dwDataIndex = 0;
	pDBUnit->dwDataSize = 0;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DatabaseUnitCollectionFree(
	DATABASE_UNIT_COLLECTION *&pDBUCollection)
{
	ASSERTX_RETURN( pDBUCollection, "Expected db unit collection" );
	
	// free each db unit
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		sFreeDatabaseUnit( pDBUnit );
	}
	
	// free the collection pointer itself
	GFREE( NULL, pDBUCollection );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DATABASE_UNIT *sDBUGetByGUID( 
	DATABASE_UNIT_COLLECTION *pDBUCollection, 
	PGUID guid)
{
	ASSERTX_RETNULL( pDBUCollection, "Expected DBU collection" );
	
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		const DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		if (pDBUnit->guidUnit == guid)
		{
			return pDBUnit;
		}
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DatabaseUnitCollectionAddDBUnit(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DWORD dwUsedDataBufferSize)
{
	ASSERTX_RETFALSE( pDBUCollection, "Expected db unit collection" );
	ASSERTX_RETFALSE( pDBUnit, "Expected db unit" );
	ASSERTX_RETFALSE( pDataBuffer, "Expected data buffer for db unit" );
	ASSERTX_RETFALSE( dwUsedDataBufferSize > 0, "Invalid data size for db unit data buffer" );
	ASSERTV_RETFALSE( 
		pDBUnit->guidUnit != INVALID_GUID, 
		"DBUnit (%s) has invalid GUID %I64d", 
		UnitGetDevNameBySpecies( pDBUnit->spSpecies ), 
		pDBUnit->guidUnit);
	
	// db collection must have enough storage to hold the blob data
	ASSERTX_RETFALSE( pDBUCollection->dwUsedBlobDataSize + dwUsedDataBufferSize < UNITFILE_MAX_SIZE_ALL_UNITS, "DB Unit collection out of space to store data buffer from new DB unit" );

	// must have enough units
	ASSERTX_RETFALSE( pDBUCollection->nNumUnits < UNITFILE_MAX_NUMBER_OF_UNITS - 1, "Not enough space to store db unit in collection" );

	// cannot add units that are already in the collection
	const DATABASE_UNIT *pDBUnitExisting = sDBUGetByGUID( pDBUCollection, pDBUnit->guidUnit );
	ASSERTV_RETTRUE(		// true, I guess we'll try to keep going in this error state
		pDBUnitExisting == NULL,
		"Character %S, guid=%I64d.  Existing DB Unit (%s) guid=%I64d is already in in the DBU collection.  Trying to add (%s) guid=%I64d.",
		pDBUCollection->uszCharacterName,
		pDBUnitExisting->guidUltimateOwner,
		UnitGetDevNameBySpecies( pDBUnitExisting->spSpecies ),
		pDBUnitExisting->guidUnit,
		UnitGetDevNameBySpecies( pDBUnit->spSpecies ),
		pDBUnit->guidUnit);
		
	// copy data buffer to storage in db unit collection
	MemoryCopy( 
		&pDBUCollection->bUnitBlobData[ pDBUCollection->dwUsedBlobDataSize ],
		UNITFILE_MAX_SIZE_ALL_UNITS,
		pDataBuffer,
		dwUsedDataBufferSize);
		
	// save where the data is stored at on the db unit (NOTE that this modifies the param passed in)
	pDBUnit->dwDataIndex = pDBUCollection->dwUsedBlobDataSize;
	pDBUnit->dwDataSize = dwUsedDataBufferSize;
	
	// update the used blob data in the collection
	pDBUCollection->dwUsedBlobDataSize += dwUsedDataBufferSize;

	// add a name to this DB unit based on species if one is not already set
	if (pDBUnit->szName[ 0 ] == 0)
	{
		PStrCopy( pDBUnit->szName, UnitGetDevNameBySpecies( pDBUnit->spSpecies ), MAX_DB_UNIT_NAME );
	}
	
	// add the new db unit to the collection
	pDBUCollection->tUnits[ pDBUCollection->nNumUnits++ ] = *pDBUnit;
		
	return TRUE;  // success
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindRootUnitIndex(
	const DATABASE_UNIT_COLLECTION *pDBUCollection,
	UNIT_COLLECTION_TYPE eCollectionType,
	PGUID guidUnitTreeRoot)
{
	ASSERTX_RETINVALID( pDBUCollection, "Expected db unit collection" );

	// scan through units
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		const DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		
		if (eCollectionType == UCT_CHARACTER)
		{
			if (pDBUnit->guidUnit == pDBUnit->guidUltimateOwner &&
				pDBUnit->guidContainer == INVALID_GUID)
			{
				return i;
			}
		}
		else if (eCollectionType == UCT_ITEM_COLLECTION)
		{
			ASSERTX_RETINVALID( guidUnitTreeRoot != INVALID_GUID, "Expected guid to use for item tree root" );
			if (pDBUnit->guidUnit == guidUnitTreeRoot)
			{
				return i;
			}
		}
				
	}
				
	return INVALID_INDEX;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE *s_DBUnitGetDataBuffer(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit)
{
	ASSERTX_RETNULL( pDBUCollection, "Expected db unit collection" );
	ASSERTX_RETNULL( pDBUnit, "Expected DB unit" );

	// index to data must be valid
	ASSERTX_RETNULL( 
		pDBUnit->dwDataIndex >= 0 && 
		pDBUnit->dwDataIndex < UNITFILE_MAX_SIZE_ALL_UNITS &&
		pDBUnit->dwDataIndex < pDBUCollection->dwUsedBlobDataSize,
		"Invalid index for blob data of DB unit" );

	// return pointer to beginning of blob data buffer
	return &pDBUCollection->bUnitBlobData[ pDBUnit->dwDataIndex ];
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDBUnitCheckCRC( 
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit)
{
	ASSERTX_RETFALSE( pDBUCollection, "Expected db unit collection" );
	ASSERTX_RETFALSE( pDBUnit, "Expected DB unit" );
	
	BYTE *pDataBuffer = s_DBUnitGetDataBuffer( pDBUCollection, pDBUnit );
	ASSERTX_RETFALSE( pDataBuffer, "Unable to get data buffer of DB unit" );

	// check CRC on saved data	
	if (UnitFileCheckCRC( pDBUnit->dwCRC, pDataBuffer, pDBUnit->dwDataSize ) == FALSE)
	{
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE, 
			"DB Unit '%s' [guid=%I64d] failed CRC check", 
			UnitGetDevNameBySpecies( pDBUnit->spSpecies ),
			pDBUnit->guidUnit);
		ASSERTX( 0 , szMessage );
		//TraceError( szMessage );
		
		return FALSE;
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DatabaseUnitsPostProcess(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	PLAYER_LOAD_TYPE ePlayerLoadType,	
	UNIT_COLLECTION_TYPE eCollectionType,
	PGUID guidUnitTreeRoot)
{

	// load types that should have a root unit must find it now
	if (ePlayerLoadType == PLT_CHARACTER ||	
		ePlayerLoadType == PLT_ITEM_TREE ||
		ePlayerLoadType == PLT_ITEM_RESTORE ||
		ePlayerLoadType == PLT_AH_ITEMSALE_SEND_TO_AH)
	{
		// find the root unit for the hierarchy
		int nRootIndex = sFindRootUnitIndex( pDBUCollection, eCollectionType, guidUnitTreeRoot );
		ASSERTX_RETFALSE( nRootIndex != INVALID_INDEX, "Cannot find root unit in db unit collection" );
		
		// place the root unit at the front of the db unit array
		if (nRootIndex != 0 && pDBUCollection->nNumUnits > 1)
		{
			DATABASE_UNIT tDBUnitTemp = pDBUCollection->tUnits[ 0 ];
			pDBUCollection->tUnits[ 0 ] = pDBUCollection->tUnits[ nRootIndex ];
			pDBUCollection->tUnits[ nRootIndex ] = tDBUnitTemp;
		}

		// we have found the root unit for this collection
		SETBIT( pDBUCollection->dwStatus, DUS_HAS_ROOT_UNIT_BIT, TRUE );
				
	}
				
	// we have post processed this now
	SETBIT( pDBUCollection->dwStatus, DUS_POST_PROCESSED_BIT, TRUE );

	// check the CRC for each unit
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		if (sDBUnitCheckCRC( pDBUCollection, pDBUnit ) == FALSE)
		{
			return FALSE;
		}		
	}
		
	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_DBUnitLoad(	
	GAME *pGame,
	UNIT *pUnitExisting,
	DATABASE_UNIT *pDBUnit,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpecControl,
	BOOL bIsInIventory,
	BOOL bErrorCheckAlreadyLoaded)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( pDBUnit, "Expected db unit" );
	ASSERTX_RETNULL( pDBUCollection, "Expected db unit collection" );

	// if we are error checking that a unit cannot be loaded again, do so now
	if (bErrorCheckAlreadyLoaded)
	{
		ASSERTV_RETVAL( 
			pDBUnit->pUnitLoadResult == NULL,
			pDBUnit->pUnitLoadResult,
			"DB Unit alread loaded: '%s', unit=%s, id=%d, guid=%I64d, containerguid=%I64d, ultownerguid=%I64d",
			UnitGetDevNameBySpecies( pDBUnit->spSpecies ),
			UnitGetDevName( pDBUnit->pUnitLoadResult ),
			UnitGetId( pDBUnit->pUnitLoadResult ),
			pDBUnit->guidUnit,
			pDBUnit->guidContainer,
			pDBUnit->guidUltimateOwner);
	}
	
	// setup a new copy of the read spec with the params of the original one
	UNIT_FILE_XFER_SPEC<XFER_LOAD> tReadSpec = tReadSpecControl;
	SETBIT( tReadSpec.dwReadUnitFlags, RUF_NO_DATABASE_OPERATIONS );
	
	// setup a new buffer for the read spec
	BYTE *pDataBuffer = s_DBUnitGetDataBuffer( pDBUCollection, pDBUnit );
	BIT_BUF tBitBuffer( pDataBuffer, pDBUnit->dwDataSize );
	tReadSpec.tXfer.SetBuffer( &tBitBuffer );
	tReadSpec.pUnitExisting = pUnitExisting;
	tReadSpec.bIsInInventory = bIsInIventory;
	tReadSpec.pDBUnit = pDBUnit;
	
	// read this single unit
	UNIT *pUnit = UnitFileXfer( pGame, tReadSpec );
	ASSERTX_RETNULL( pUnit, "Unable to read database unit buffer" );

	// this unit is in the database
	UnitSetFlag( pUnit, UNITFLAG_IN_DATABASE, TRUE );

	// this DB unit has now been loaded
	pDBUnit->pUnitLoadResult = pUnit;
			
	return pUnit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddUnitToResult(
	UNIT *pUnit,
	DATABASE_UNIT_COLLECTION_RESULTS &tResults)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( tResults.nNumUnits - 1 < (int)arrsize( tResults.pUnits ), "Too many result units" );
	tResults.pUnits[ tResults.nNumUnits++ ] = pUnit;	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadContainedUnits(
	UNIT *pUnit,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpecControl,
	DATABASE_UNIT_COLLECTION_RESULTS &tResults,
	BOOL bAddToResults)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pDBUCollection, "Expected db unit collection" );
	
	GAME *pGame = UnitGetGame( pUnit );
	PGUID guidUnit = UnitGetGuid( pUnit );
	ASSERTV_RETFALSE( guidUnit != INVALID_GUID, "Expected guid for unit %s.  Character %S", UnitGetDevName( pUnit ), pDBUCollection->uszCharacterName);

	// setup a read unit spec with our container
	UNIT_FILE_XFER_SPEC<XFER_LOAD> tSpec = tReadSpecControl;
	tSpec.pContainer = pUnit;  // setup the container unit to the unit passed in
		
	// scan the db units for any unit directly contained by the unit passed in
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		if (pDBUnit->guidContainer == guidUnit)
		{
			if (pDBUnit->pUnitLoadResult != NULL)
			{
				char szCharacterName[ MAX_CHARACTER_NAME ];
				PStrCvt( szCharacterName, pDBUCollection->uszCharacterName, MAX_CHARACTER_NAME );
				ASSERTV_CONTINUE(
					"Database unit collection (Character %S) trying to load contained unit (%s) [%id] more than once.  Container (%s) id=%d guid=%I64d",
					szCharacterName,
					UnitGetDevName( pDBUnit->pUnitLoadResult ),
					UnitGetId( pDBUnit->pUnitLoadResult ),
					UnitGetDevName( pUnit ),
					UnitGetId( pUnit ),
					UnitGetGuid( pUnit ));
			}
			else
			{			
				UNIT *pContainedUnit = s_DBUnitLoad( pGame, NULL, pDBUnit, pDBUCollection, tSpec, TRUE, TRUE );
				ASSERTV_CONTINUE( 
					pContainedUnit, 
					"Could not load contained DB unit, GUID:%I64d.  Character: %S, Unit: %s", 
					pDBUnit->guidUnit,
					pDBUCollection->uszCharacterName,
					UnitGetDevNameBySpecies( pDBUnit->spSpecies ));

				// add this unit to the results if requested
				BOOL bInvalid = FALSE;
				if (bAddToResults)
				{
					if (sAddUnitToResult( pContainedUnit, tResults ) == FALSE)
					{
						bInvalid = TRUE;
					}
				}
				
				// the contained unit that we loaded should now actually be contained
				// by the unit passed in, if it's not, something went wrong and we throw it away
				UNIT *pContainer = UnitGetContainer( pContainedUnit );
				if (bInvalid == TRUE || pContainer != pUnit)
				{
					// log the error
					ASSERTV( 
						pContainer == pUnit, 
						"DB Unit '%s' should now be contained by '%s' but is instead contained by '%s'!",
						UnitGetDevName( pContainedUnit ),
						UnitGetDevName( pUnit ),
						pContainer ? UnitGetDevName( pContainer ) : "NOTHING" )				
					UnitFree( pContainedUnit );
					continue;
				}
							
				// read all of of the new units contained units, note that we're
				// not adding the next level down of contained units to the results because
				// they are linked by their container which is in the result set
				if (TESTBIT( tSpec.dwReadUnitFlags, RUF_IGNORE_INVENTORY_BIT ) == FALSE)
				{
					sLoadContainedUnits( pContainedUnit, pDBUCollection, tSpec, tResults, FALSE );
				}
			}
		}
	}
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitCollectionLoad(
	GAME *pGame,
	UNIT *pCollectionRoot,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpec,
	DATABASE_UNIT_COLLECTION_RESULTS &tResults)
{
	ASSERTX_RETFALSE( pDBUCollection, "Expected database unit collection" );
	BOOL bAddContainedToResults = FALSE;
	BOOL bCreatedNewRoot = FALSE;
	
	// initialize the results
	tResults.nNumUnits = 0;
	
	// load the root unit (if needed)
	if (pCollectionRoot == NULL)
	{
		DATABASE_UNIT *pDBUnit = s_DatabaseUnitGetRoot( pDBUCollection );
		pCollectionRoot = s_DBUnitLoad( 
			pGame, 
			NULL, 
			pDBUnit, 
			pDBUCollection, 
			tReadSpec, 
			tReadSpec.pContainer != NULL, 
			TRUE);
		ASSERTV_RETFALSE( pCollectionRoot, "Unable to load root database unit.  Character %S", pDBUCollection->uszCharacterName );

		// add the collection root to the result set, this will be the one and
		// only unit that we track in the result set because all other items that
		// are loaded can be found in the inventory of the newly created
		// root unit
		if (sAddUnitToResult( pCollectionRoot, tResults ) == FALSE)
		{
			ASSERTX( 0, "Unable to add root unit to collection tracking" );
			UnitFree( pCollectionRoot );
			return FALSE;
		}
		bCreatedNewRoot = TRUE;
		
	}
	else
	{
	
		// since we are not loading the root unit from the collection itself, 
		// we will want to track each unit that we load into the direct inventory
		// of the collection root in the result set.  This allows us, for instance, 
		// to load an arbitrary collection of units into a player inventory and then
		// be able to see all those units
		bAddContainedToResults = TRUE;
		
	}
	
	// load all the units that are directly contained by this unit
	if (TESTBIT( tReadSpec.dwReadUnitFlags, RUF_IGNORE_INVENTORY_BIT ) == FALSE)	
	{
		if (sLoadContainedUnits( 
				pCollectionRoot, 
				pDBUCollection, 
				tReadSpec, 
				tResults, 
				bAddContainedToResults ) == FALSE)
		{
			ASSERTX( 0, "s_DBUnitCollectionLoad(), sLoadContainedUnits returned FALSE" );
			
			// error loading our contained units, forget it and delete everything
			// we've got so far
			for (int i = 0; i < tResults.nNumUnits; ++i)
			{
				UNIT *pUnit = tResults.pUnits[ i ];
				UnitFree( pUnit );
				tResults.pUnits[ i ] = NULL;				
			}
			
			// zero out results
			tResults.nNumUnits = 0;

			return FALSE;						
			
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_DatabaseUnitsLoadOntoExistingRootOnly(
	UNIT *pUnitRoot,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpec)
{	
	ASSERTX_RETNULL( pUnitRoot, "Expected unit" );
	ASSERTX_RETNULL( pDBUCollection, "Expected database unit collection" );
	GAME *pGame = UnitGetGame( pUnitRoot );
	
	// get the root unit
	DATABASE_UNIT *pDBUnit = s_DatabaseUnitGetRoot( pDBUCollection );
	s_DBUnitLoad( 
		pGame, 
		pUnitRoot, 
		pDBUnit, 
		pDBUCollection, 
		tReadSpec, 
		tReadSpec.pContainer != NULL, 
		FALSE);
	
	// return the existing root
	return pUnitRoot;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DATABASE_UNIT *s_DatabaseUnitGetRoot(
	DATABASE_UNIT_COLLECTION *pDBUCollection)
{
	ASSERTX_RETNULL( pDBUCollection, "Expected database unit collection" );
	
	// we can't do this until we've been post processed
	ASSERTX_RETNULL( TESTBIT( pDBUCollection->dwStatus, DUS_POST_PROCESSED_BIT ), "DB Units has not yet been post processed" );

	// we can't do this if the collection itself does not have a root unit
	ASSERTX_RETNULL( TESTBIT( pDBUCollection->dwStatus, DUS_HAS_ROOT_UNIT_BIT ), "This DB Unit collection does not have a root unit, it could be an item collection?" );
	
	// the root unit is always the fist unit in the array
	return &pDBUCollection->tUnits[ 0 ];	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitGetPendingDatabaseOperation(	
	UNIT *pUnit,
	PENDING_OPERATION_BUCKET *&pBucket,
	PENDING_OPERATION *&pPending)

{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	UNITID idUnit = UnitGetId( pUnit );
	
	// get bucket for pending operation
	pBucket = sGetPendingOperationBucket( pGame, idUnit );
	ASSERTX_RETFALSE( pBucket, "Expected PENDING_OPERATION_BUCKET" );
	
	// find operation in bucket (if any)
	pPending = sFindPendingOperation( pGame, idUnit, pBucket );
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DATABASE_OPERATION sCombineOperations( 
	UNIT *pUnit,
	DATABASE_OPERATION eOperationNew,
	DATABASE_OPERATION eOperationPending)
{
	ASSERTX_RETVAL( pUnit, DBOP_INVALID, "Expected unit" );
	ASSERTX_RETVAL( eOperationNew != DBOP_INVALID, DBOP_INVALID, "Invalid db operation" );
	ASSERTX_RETVAL( eOperationPending != DBOP_INVALID, DBOP_INVALID, "Invalid db operation" );
		
	// is the unit in the database already
	BOOL bInDatabase = UnitTestFlag( pUnit, UNITFLAG_IN_DATABASE );
	
	// you cannot add units to the database if they are already a part of it
	if (eOperationNew == DBOP_ADD)
	{
		
		// if we're in the database already, this is just an update instead
		if (bInDatabase == TRUE)
		{		
			eOperationNew = DBOP_UPDATE;  // just update it instead
		}
		
	}
	else if (eOperationNew == DBOP_UPDATE)
	{
	
		// if we have an add or remove pending, forget it
		if (eOperationPending == DBOP_ADD || eOperationPending == DBOP_REMOVE)
		{
			eOperationNew = eOperationPending;  // the add or remove always takes priority
		}
		else
		{
			if (bInDatabase == FALSE)
			{
				eOperationNew = DBOP_ADD;
			}	
		}
		
	}
	else if (eOperationNew == DBOP_REMOVE)
	{
	
		// if not in the database, forget it
		if (bInDatabase == FALSE)
		{
			eOperationNew = DBOP_INVALID;
		}
		
	}
	else
	{
		ASSERTX_RETVAL( 0, DBOP_INVALID, "Unknown database operation" );
	}

	// return the operation to do		
	return eOperationNew;
	
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitCanDoDatabaseOperation(
	UNIT *pUltimateOwner,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pUltimateOwner, "Expected ultimate owner unit" );

	// if app does not save, do nothing
	if (AppIsSaving() == FALSE)
	{
		return FALSE;
	}
		
	// global flag to for this whole system
	if (s_DatabaseUnitsAreEnabled() == FALSE)
	{
		return FALSE;
	}

	// we can only care if there is a database connected for our official servers
	if (sgbAppRequiresDatabaseForLogic == TRUE)
	{
		#if !ISVERSION(SERVER_VERSION)
			return FALSE;  // only ping0 servers can use the database
		#endif	
	}
	
	// unit cannot do database operations when flagged not to (like when loading)
	if (UnitTestFlag( pUnit, UNITFLAG_NO_DATABASE_OPERATIONS ) == TRUE ||
		UnitTestFlag( pUltimateOwner, UNITFLAG_NO_DATABASE_OPERATIONS ) == TRUE)
	{
		return FALSE;
	}

	// must be a player, monster (for hirelings), or item
	if (UnitIsA( pUnit, UNITTYPE_PLAYER ) == FALSE &&
		UnitIsA( pUnit, UNITTYPE_MONSTER ) == FALSE &&
		UnitIsA( pUnit, UNITTYPE_ITEM ) == FALSE)
	{
		return FALSE;
	}
	
	// if the item is in the database, it must be able to do database operations, 
	// an especially critical one will be the ability to remove it from the database
	// when it transitions to a non-player or a non-saved inventory slot
	if (UnitTestFlag( pUnit, UNITFLAG_IN_DATABASE ))
	{
		return TRUE;
	}
	
	// we only care about units who are owned (or were owned) by players
	if (UnitIsPlayer( pUltimateOwner ) == FALSE)
	{
		return FALSE;
	}
						
	// check OK to save inventory item units
	if (s_UnitCanBeSaved( pUnit, FALSE ) == FALSE)
	{
		return FALSE;
	}
	
	return TRUE;  // ok to goooo!
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DatabaseUnitAddAll(
	UNIT *pPlayer,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// add this unit (this will add the inventory contents as well)
	if (s_DatabaseUnitOperation( pPlayer, pUnit, DBOP_ADD ) == FALSE)
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitCreate(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DWORD dwDataBufferSize,
	int &nValidationErrors,
	BOOL bCreateBlobData)
{

#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )

	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pDBUnit, "Expected db unit" );
	ASSERTX_RETFALSE( pDataBuffer, "Expected db unit data buffer" );
	ASSERTX_RETFALSE( dwDataBufferSize > 0, "Invalid db unit data buffer size" );
	
	UNIT *pUltimateOwner = UnitGetUltimateOwner( pUnit );
	UNIT *pContainer = UnitGetContainer( pUnit );
		
	// fill out db a new unit structure
	s_DBUnitInit( pDBUnit );
	pDBUnit->guidUnit = UnitGetGuid( pUnit );
	ASSERTV_RETFALSE( 
		pDBUnit->guidUnit != INVALID_GUID,
		"Unable to assign invalid guid for DB unit '%s' [id=%d]",
		UnitGetDevName( pUnit ),
		UnitGetId( pUnit ));
	pDBUnit->spSpecies = UnitGetSpecies( pUnit );
	pDBUnit->eGenus = UnitGetGenus( pUnit );
	pDBUnit->dwClassCode = UnitGetClassCode( pUnit );
	PStrCopy( pDBUnit->szName, UnitGetDevName( pUnit ), MAX_DB_UNIT_NAME );
	pDBUnit->guidUltimateOwner = pUltimateOwner ? UnitGetGuid( pUltimateOwner ) : INVALID_GUID;
	pDBUnit->dwSecondsPlayed = UnitGetPlayedTime( pUnit );
	int nMoney = UnitGetCurrency( pUnit ).GetValue( KCURRENCY_VALUE_INGAME );
	ASSERTX( nMoney >= 0, "Money can't be negative ... or can it?  Hmmm." );
	pDBUnit->dwMoney = MAX( 0, nMoney );
	pDBUnit->dwExperience = UnitGetExperienceValue( pUnit );
	pDBUnit->dwRankExperience = UnitGetRankExperienceValue( pUnit );
	pDBUnit->dwRank = UnitGetPlayerRank( pUnit );
	pDBUnit->dwQuantity = ItemGetQuantity( pUnit );
	if (pContainer)
	{
	
		// get container
		pDBUnit->guidContainer = UnitGetGuid( pContainer );
		ASSERTV_RETFALSE( 
			pDBUnit->guidContainer != UnitGetGuid( pUnit ),
			"DB Unit (%s) cannot be contained by itself!",
			UnitGetDevNameBySpecies( pDBUnit->spSpecies ));

		// where is it contained
		DBUNIT_INVENTORY_LOCATION &tDBUnitInvLoc = pDBUnit->tDBUnitInvLoc;
		UnitGetInventoryLocation( pUnit, &tDBUnitInvLoc.tInvLoc );
		tDBUnitInvLoc.dwInvLocationCode = UnitGetInvLocCode( pContainer, tDBUnitInvLoc.tInvLoc.nInvLocation );
		
	}
				
	// write unit data to buffer if we are creating the blob data
	if (bCreateBlobData)
	{
	
		GAME *pGame = UnitGetGame( pUnit );	
		BIT_BUF tBitBuffer( pDataBuffer, dwDataBufferSize );			
		UNITSAVEMODE eSaveMode = UNITSAVEMODE_DATABASE;
		XFER<XFER_SAVE> tXfer(&tBitBuffer);
		UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);
		tSpec.pUnitExisting = pUnit;
		tSpec.eSaveMode = eSaveMode;
		tSpec.bIsInInventory = pContainer ? TRUE : FALSE;
		tSpec.pDBUnit = pDBUnit;
		if (UnitFileXfer( pGame, tSpec ) != pUnit)
		{
			const int MAX_MESSAGE = 256;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Unable to xfer unit name:%s guid:%I64d id:%d for db create.  Buffer size %d/%d",
				UnitGetDevName( pUnit ),
				UnitGetGuid( pUnit ),
				UnitGetId( pUnit ),
				tBitBuffer.GetLen(),
				dwDataBufferSize);			
			ASSERTX_RETFALSE( 0, szMessage );			
		}
		
		// save the CRC result in the param
		pDBUnit->dwCRC = tSpec.dwCRC;

		// save size of data in DB unit
		pDBUnit->dwDataSize = tBitBuffer.GetLen();
		pDBUnit->dwDataIndex = 0;  // this is not valid unless a db unit is inside a db unit collection
			
		// sanity check we had enough space to save all the unit data
		ASSERTV_RETFALSE( 
			pDBUnit->dwDataSize < UNITFILE_MAX_SIZE_SINGLE_UNIT, 
			"UNITFILE_MAX_SIZE_SINGLE_UNIT too small for entire unit (%s) [id=%d] data to be saved.  Buffer size %d/%d", 
			UnitGetDevName( pUnit ),
			UnitGetId( pUnit ),
			pDBUnit->dwDataSize,
			dwDataBufferSize);

	}
					
#endif
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsLogged(
	UNIT *pUnit)
{
	ASSERT_RETFALSE(pUnit);

	if (UnitIsA(pUnit, UNITTYPE_ITEM))
	{
		if (UnitDataTestFlag(UnitGetData(pUnit), UNIT_DATA_FLAG_ALWAYS_DO_TRANSACTION_LOGGING))
		{
			return TRUE;
		}

		if (UnitGetStat(pUnit, STATS_ITEM_AUGMENTED_COUNT) > 0)
		{
			return TRUE;
		}

		int nItemQuality = ItemGetQuality(pUnit);

		if (nItemQuality == INVALID_LINK)
		{
			return FALSE;
		}

		const ITEM_QUALITY_DATA *pItemQuality = ItemQualityGetData(nItemQuality);
		ASSERT_RETFALSE(pItemQuality);

		return pItemQuality->bDoTransactionLogging;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static void sSendDBUnitLogToClients(
	GAME *pGame,
	const char *pszMessage,
	DATABASE_OPERATION eOperation)
{
	
	// setup message
	MSG_SCMD_DBUNIT_LOG tMessage;
	PStrCopy( tMessage.szMessage, pszMessage, MAX_DBLOG_MESSAGE );
	tMessage.nOperation = eOperation;
	
	// send to all clients
	s_SendMessageToAll( pGame, SCMD_DBUNIT_LOG, &tMessage );
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static BOOL sDatabaseUnitAdd(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DBOPERATION_CONTEXT_TYPE eOpContext)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pDBUnit, "Expected db unit" );
	ASSERTX_RETFALSE( pDataBuffer, "Expected db unit data buffer" );

	// the unit cannot already be in the database
	if (sTrackUnitsInDatabase())
	{
		ASSERTX_RETFALSE( UnitTestFlag( pUnit, UNITFLAG_IN_DATABASE ) == FALSE, "Can't add unit to database when they are already in it" );
	}
	
	// send add message to database
	// TODO: For Brian	

	if (sgbDBUnitLoadMultiplyEnabled == TRUE)
	{
		// DB Load Multiplier Loop (LOAD TEST)
		for(int nIterationCount = 0; nIterationCount < NUM_DB_TRANSACTIONS; nIterationCount++)
		{
			if (nIterationCount > 0)
			{
				pDBUnit->guidUnit          = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUnit, nIterationCount);
				pDBUnit->guidUltimateOwner = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUltimateOwner, nIterationCount);
				pDBUnit->guidContainer     = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidContainer, nIterationCount);
			}
			UnitAddToDatabase( pUnit, pDBUnit, pDataBuffer );
		}
	}
	else
	{
		// Single Operation
		UnitAddToDatabase( pUnit, pDBUnit, pDataBuffer );						
	}
	
	// the unit is now in the database
	UnitSetFlag( pUnit, UNITFLAG_IN_DATABASE );

#if ISVERSION(SERVER_VERSION)
	if (PlayerActionLoggingIsEnabled() && UnitIsLogged(pUnit))
	{
		// map db op context to action type - trades don't go through here
		PLAYER_LOG_TYPE eLogType;
		switch (eOpContext)
		{
			case DBOC_BUY:
				eLogType = PLAYER_LOG_ITEM_GAIN_BUY;
				break;
			case DBOC_MAIL:
				eLogType = PLAYER_LOG_ITEM_GAIN_MAIL;
				break;
			case DBOC_INVALID:
			default:
				eLogType = PLAYER_LOG_ITEM_GAIN_PICKUP;
		}

		ITEM_PLAYER_LOG_DATA tItemData;
		ItemPlayerLogDataInitFromUnit( tItemData, pUnit );

		WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
		UNIT * pCharacterUnit = UnitGetUltimateOwner( pUnit );
		UnitGetName( pCharacterUnit, wszCharacterName, MAX_CHARACTER_NAME, 0 );
		
		TracePlayerItemActionWithBlobData(
			eLogType,
			UnitGetAccountId( pCharacterUnit ),
			pDBUnit->guidUltimateOwner,
			wszCharacterName,
			&tItemData,
			pDBUnit,
			pDataBuffer );
	}
#endif

	// do log
	if (gbDBUnitLogEnabled)
	{
	
		// construct message
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf(
			szMessage,
			MAX_MESSAGE,
			"DBOP_ADD   : guid=%I64d id=%d (%s)\n", 
			UnitGetGuid( pUnit ),
			UnitGetId( pUnit ),
			UnitGetDevName( pUnit ));

		// trace
		trace( szMessage );

		// send to clients
		if (gbDBUnitLogInformClients)
		{
			GAME *pGame = UnitGetGame( pUnit );
			sSendDBUnitLogToClients( pGame, szMessage, DBOP_ADD );
		}
		
	}
		
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static BOOL sDatabaseUnitRemove(
	GAME *pGame,
	const PENDING_OPERATION *pPending)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pPending, "Expected pending operation for database unit remove" );	

	// get stuff out of the pending operation we need
	UNITID idUnit = pPending->idUnit;
	ASSERTX_RETFALSE( idUnit != INVALID_ID, "Expected unit id" );	
	PGUID guidUnit = pPending->guidUnit;
	PGUID guidUltimateOwner = pPending->guidUltimateOwner;
	const UNIT_INFO *pUnitInfo = &pPending->tUnitInfo;
	ASSERTX_RETFALSE( pUnitInfo->pUnitData, "Expected unit data" );		
#if ISVERSION(SERVER_VERSION)
	const DBOPERATION_CONTEXT* pOpContext = &pPending->opContext;
#endif

	// Don't send request if the unit doesn't have a GUID
	if (guidUnit == INVALID_GUID)
	{
		#if ISVERSION(SERVER_VERSION)
		TraceExtraVerbose(TRACE_FLAG_GAME_NET, "Tried to remove unit with invalid guid from database");
		#endif
		return FALSE;
	}
	
	// if unit is still around, clear flag for it being in the database
	UNIT *pUnit = UnitGetById( pGame, idUnit );
	if (pUnit)
	{
	
		// the unit must already be in the database
		if (sTrackUnitsInDatabase())
		{
			UNIT *pUltimateOwner = UnitGetByGuid( pGame, guidUltimateOwner );
			REF( pUltimateOwner );
			
			ASSERTV_RETFALSE( 
				UnitTestFlag( pUnit, UNITFLAG_IN_DATABASE ), 
				"DB Remove Failed, unit not marked in DB - unit=%s, guid=%I64d, owner=%s, owner_guid=%I64d",
				UnitGetDevName( pUnit ),
				guidUnit,
				pUltimateOwner ? UnitGetDevName( pUltimateOwner ) : "UNKNOWN",
				guidUltimateOwner);
		}

		// clear flag	
		UnitClearFlag( pUnit, UNITFLAG_IN_DATABASE );
		
	}
	
	BOOL bSuccess = FALSE;
	if (sgbDBUnitLoadMultiplyEnabled == TRUE)
	{
		// DB Load Multiplier Loop (LOAD TEST)
		for(int nIterationCount = 0; nIterationCount < NUM_DB_TRANSACTIONS; nIterationCount++)
		{
			if (nIterationCount > 0)
			{
				guidUnit          = s_DBUnitGuidIncrementForLoadTest(guidUnit, nIterationCount);
				guidUltimateOwner = s_DBUnitGuidIncrementForLoadTest(guidUltimateOwner, nIterationCount);
			}
			// send remove message to database
			bSuccess = UnitRemoveFromDatabase( guidUnit, guidUltimateOwner, pUnitInfo->pUnitData );
		}
	}
	else
	{
		// Single Operation
		bSuccess = UnitRemoveFromDatabase( guidUnit, guidUltimateOwner, pUnitInfo->pUnitData );
	}

	ASSERTV( 
		bSuccess, 
		"Error removing unit '%I64d' (%s) from the database",
		guidUnit,
		pUnitInfo->pUnitData->szName);

#if ISVERSION(SERVER_VERSION)
	ASSERTX(pOpContext, "Expected DB Op Context! Transaction not logged");
	if (PlayerActionLoggingIsEnabled() && pOpContext && pOpContext->bIsLogged)
	{
		// map db op context to action type
		PLAYER_LOG_TYPE eLogType;
		switch (pOpContext->eContext)
		{
			case DBOC_SELL:
				eLogType = PLAYER_LOG_ITEM_LOST_SELL;
				break;
			case DBOC_TRADE:
				eLogType = PLAYER_LOG_ITEM_LOST_TRADE;
				break;
			case DBOC_DROP:
				eLogType = PLAYER_LOG_ITEM_LOST_DROP;
				break;
			case DBOC_MAIL:
				eLogType = PLAYER_LOG_ITEM_LOST_MAIL;
				break;
			case DBOC_UPGRADE:
				eLogType = PLAYER_LOG_ITEM_LOST_UPGRADE;
				break;
			case DBOC_DISMANTLE:
				eLogType = PLAYER_LOG_ITEM_LOST_DISMANTLE;
				break;
			case DBOC_DESTROY:
				eLogType = PLAYER_LOG_ITEM_LOST_DESTROY;
				break;
			case DBOC_INVALID:
			default:
				eLogType = PLAYER_LOG_ITEM_LOST_UNKNOWN;
		}

		ITEM_PLAYER_LOG_DATA tItemData;
		ItemPlayerLogDataInitFromUnitData( tItemData, pUnitInfo->pUnitData );

		tItemData.guidUnit = guidUnit;
		EXCELTABLE eTable = ExcelGetDatatableByGenus( pUnitInfo->eGenus );
		tItemData.pszGenus = ExcelGetTableName( eTable );

		PStrCopy( tItemData.wszTitle, pOpContext->wszUnitDisplayName, MAX_ITEM_NAME_STRING_LENGTH );
		PStrCopy( tItemData.wszDescription, pUnitInfo->uszItemInfo, MAX_ITEM_PLAYER_LOG_STRING_LENGTH );
		tItemData.pszQuality = pUnitInfo->pszQuality;

		if (pPending->dwDatabaseOperationFlags & DOF_HAS_DELETE_DB_UNIT)
		{
		
			// get the database unit from the pending operation, we get it from here
			// because at this point the unit could be gone (if it was freed) and we
			// must have data to put in the log from the time it was scheduled for removal
			const DATABASE_UNIT *pDBUnit = &pPending->tDBUnit;
			
			// log the data
			TracePlayerItemActionWithBlobData(
				eLogType,
				pOpContext->idAccount,
				pDBUnit->guidUltimateOwner,
				pOpContext->wszCharacterName,
				&tItemData,
				pDBUnit,
				pPending->bDataBuffer);

		}
		else
		{
		
			// we don't have a dbunit to use for the delete operation, log without it
			TracePlayerItemAction(
				eLogType,
				pOpContext->idAccount,
				pOpContext->guidCharacter,
				pOpContext->wszCharacterName,
				&tItemData );
				
		}
					
	}
#endif
		
	// do log
	if (gbDBUnitLogEnabled)
	{
		
		// construct message
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf(
			szMessage,
			MAX_MESSAGE,
			"DBOP_REMOVE: guid=%I64d id=%d (%s)\n", 
			guidUnit,
			idUnit,
			pUnitInfo->pUnitData->szName);

		// trace
		trace( szMessage );

		// send to clients
		if (gbDBUnitLogInformClients)
		{
			sSendDBUnitLogToClients( pGame, szMessage, DBOP_REMOVE );
		}
				
	}
	
	return TRUE;
	
}	
#endif

//----------------------------------------------------------------------------
typedef void (* PFN_FIELD_UPDATE)( UNIT *pUnit, DATABASE_UNIT *pDBUnit );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFieldExperience(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	SVR_VERSION_ONLY(TrackEvent_ExperienceSavedEvent(pDBUnit->dwExperience,pDBUnit->szName,pDBUnit->guidUnit));
	// Update field for pDBUnit->dwExperience
	UnitUpdateExperienceInDatabase(pDBUnit->guidUnit, pDBUnit->guidUltimateOwner, pDBUnit->dwExperience);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFieldMoney(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	// Update field for pDBUnit->dwMoney
	ASSERT(pDBUnit->guidUnit == pDBUnit->guidUltimateOwner);
	// If we're updating money for non-players, we need to be very careful about money duping exploits,
	// and track the guids of players owning them.
	UnitUpdateMoneyInDatabase(pDBUnit->guidUnit, pDBUnit->guidUltimateOwner, pDBUnit->dwMoney);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFieldQuantity(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	// Update field for pDBUnit->dwQuantity
	UnitUpdateQuantityInDatabase(pDBUnit->guidUnit, 
		pDBUnit->guidUltimateOwner,
		(WORD)pDBUnit->dwQuantity);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFieldInventoryLocation(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	if (pDBUnit->tDBUnitInvLoc.tInvLoc.nInvLocation == INVLOC_EMAIL_INBOX)
		return;
	// Update field for pDBUnit->tInvLoc.*
	UNIT * pPlayer = UnitGetUltimateOwner(pUnit);
	UnitChangeLocationInDatabase(pPlayer, 
									pDBUnit->guidUnit, 
									pDBUnit->guidUltimateOwner, 
									pDBUnit->guidContainer, 
									&pDBUnit->tDBUnitInvLoc.tInvLoc, 
									pDBUnit->tDBUnitInvLoc.dwInvLocationCode, 
									FALSE);

	// Change Ultimate Owner For Contained Items if Item is in Shared Stash
	UNIT * pItem = NULL;
	INVENTORY_LOCATION tInvLoc;
	UnitGetInventoryLocation(pUnit, &tInvLoc);
	if (tInvLoc.nInvLocation == (int)INVLOC_SHARED_STASH)
	{
		while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
		{
			// create db unit		
			BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
			DATABASE_UNIT tDBUnit;
			BOOL bCreateBlobData = FALSE;
			int nValidationErrors = 0;
			if (s_DBUnitCreate( 
					pItem, 
					&tDBUnit, 
					bDataBuffer, 
					arrsize( bDataBuffer ), 
					nValidationErrors, 
					bCreateBlobData) == TRUE)
			{
				// Change location of contained units
				UnitChangeLocationInDatabase(pPlayer, 
												tDBUnit.guidUnit, 
												tDBUnit.guidUltimateOwner, 
												tDBUnit.guidContainer, 
												&tDBUnit.tDBUnitInvLoc.tInvLoc, 
												tDBUnit.tDBUnitInvLoc.dwInvLocationCode,
												TRUE );
			}
		}		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFieldRankExperience(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	// Update field for pDBUnit->dwExperience
	CharacterUpdateRankExperienceInDatabase(pDBUnit->guidUnit, pDBUnit->guidUltimateOwner, pDBUnit->dwRankExperience, pDBUnit->dwRank);
}


//----------------------------------------------------------------------------
enum DBUNIT_FIELD_TYPE
{
	DBFT_INT,
	DBFT_FLOAT,
	DBFT_STRUCT,
};

#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
//----------------------------------------------------------------------------
struct FIELD_UPDATE_LOOKUP
{
	DBUNIT_FIELD_TYPE eType;
	DBUNIT_FIELD eField;
	const char *pszName;
	PFN_FIELD_UPDATE pfnUpdate;
	int nDBUnitOffset;
};

//----------------------------------------------------------------------------
static FIELD_UPDATE_LOOKUP sgtFieldUpdateLookup[] = 
{
	{ DBFT_INT,		DBUF_EXPERIENCE,			"Experi",	sDoFieldExperience,			offsetof( DATABASE_UNIT, dwExperience ) },
	{ DBFT_INT,		DBUF_MONEY,					"Money ",	sDoFieldMoney,				offsetof( DATABASE_UNIT, dwMoney ) },
	{ DBFT_INT,		DBUF_QUANTITY,				"Quanty",	sDoFieldQuantity,			offsetof( DATABASE_UNIT, dwQuantity ) },	
	{ DBFT_STRUCT,	DBUF_INVENTORY_LOCATION,	"InvLoc",	sDoFieldInventoryLocation,	offsetof( DATABASE_UNIT, tDBUnitInvLoc ) },
	{ DBFT_INT,		DBUF_RANK_EXPERIENCE,		"RankExp",	sDoFieldRankExperience,		offsetof( DATABASE_UNIT, dwRankExperience ) },	
};
static const int sgnNumFieldUpdateLookup = arrsize( sgtFieldUpdateLookup );
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static BOOL sDatabaseUnitUpdate(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DWORD dwDBUnitFieldBits,
	const DBOPERATION_CONTEXT* pOpContext)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pDBUnit, "Expected db unit" );
	ASSERTX_RETFALSE( pDataBuffer, "Expected db unit data buffer" );

	// the unit must already be in the database
	if (sTrackUnitsInDatabase())
	{
		ASSERTX_RETFALSE( UnitTestFlag( pUnit, UNITFLAG_IN_DATABASE ), "Can't update a unit that is not in the database" );
	}

	// see if we want to do the full update
	if (TESTBIT( dwDBUnitFieldBits, DBUF_FULL_UPDATE ) || sgbDBUnitsUseFieldsEnabled == FALSE)
	{
		
		// we must have blob data to write
		ASSERTV_RETFALSE( 
			pDBUnit->dwDataSize > 0, 
			"Cannot do full update without any db blob data! unit=%s, guid=I64d",
			UnitGetDevName( pUnit ),
			UnitGetGuid( pUnit ));
		
		if (sgbDBUnitLoadMultiplyEnabled == TRUE)
		{
			// DB Load Multiplier Loop (LOAD TEST)
			for(int nIterationCount = 0; nIterationCount < NUM_DB_TRANSACTIONS; nIterationCount++)
			{
				if (nIterationCount > 0)
				{
					pDBUnit->guidUnit          = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUnit, nIterationCount);
					pDBUnit->guidUltimateOwner = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUltimateOwner, nIterationCount);
					pDBUnit->guidContainer     = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidContainer, nIterationCount);
				}
			}
			// send update message to database
			UnitFullUpdateInDatabase( pUnit, pDBUnit, pDataBuffer );
		}
		else
		{
			// Single Operation
			UnitFullUpdateInDatabase( pUnit, pDBUnit, pDataBuffer );
		}
		
	}
	else
	{
	
		// check each of the bits we have for each field
		for (int i = 0; i < sgnNumFieldUpdateLookup; ++i)
		{
			const FIELD_UPDATE_LOOKUP *pLookup = &sgtFieldUpdateLookup[ i ];
			if (TESTBIT( dwDBUnitFieldBits, pLookup->eField ))
			{
				ASSERTX_CONTINUE( pLookup->pfnUpdate, "Expected function for db field update" );

				if (sgbDBUnitLoadMultiplyEnabled == TRUE)
				{
					// DB Load Multiplier Loop (LOAD TEST)
					for(int nIterationCount = 0; nIterationCount < NUM_DB_TRANSACTIONS; nIterationCount++)
					{
						if (nIterationCount > 0)
						{
							pDBUnit->guidUnit          = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUnit, nIterationCount);
							pDBUnit->guidUltimateOwner = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidUltimateOwner, nIterationCount);
							pDBUnit->guidContainer     = s_DBUnitGuidIncrementForLoadTest(pDBUnit->guidContainer, nIterationCount);
						}

						pLookup->pfnUpdate( pUnit, pDBUnit );
					}
				}
				else
				{
					// Single Operation
					pLookup->pfnUpdate( pUnit, pDBUnit );
				}
			}
			
		}
						
	}

#if ISVERSION(SERVER_VERSION)
	if (PlayerActionLoggingIsEnabled() && pOpContext && pOpContext->bIsLogged && pOpContext->eContext == DBOC_TRADE)
	{
		// gather unit data
		ITEM_PLAYER_LOG_DATA tItemData;
		ItemPlayerLogDataInitFromUnit( tItemData, pUnit );
		UNIT * newOwner = UnitGetUltimateOwner( pUnit );
		WCHAR wszNewOwnerCharacterName[ MAX_CHARACTER_NAME ];
		UnitGetName(newOwner, wszNewOwnerCharacterName, MAX_CHARACTER_NAME, 0);

		// log trader's side of trade
		TracePlayerItemActionWithBlobData(
			PLAYER_LOG_ITEM_LOST_TRADE,
			pOpContext->idAccount,
			pOpContext->guidCharacter,
			pOpContext->wszCharacterName,
			&tItemData,
			pDBUnit,
			pDataBuffer,
			wszNewOwnerCharacterName );

		// log recipient's side of trade
		TracePlayerItemActionWithBlobData(
			PLAYER_LOG_ITEM_GAIN_TRADE,
			UnitGetAccountId( newOwner ),
			UnitGetGuid( newOwner ),
			wszNewOwnerCharacterName,
			&tItemData,
			pDBUnit,
			pDataBuffer,
			pOpContext->wszCharacterName );
	}
#endif
	
	// do log
	if (gbDBUnitLogEnabled)
	{

		// find out what kind of update we did
		const int MAX_STRING = 128;
		char szUpdateType[ MAX_STRING ];
		if (TESTBIT( dwDBUnitFieldBits, DBUF_FULL_UPDATE ) || sgbDBUnitsUseFieldsEnabled == FALSE)
		{
			PStrCopy( szUpdateType, "Full", MAX_STRING );
		}
		else
		{
			BOOL bFirstType = TRUE;
			for (int i = 0; i < sgnNumFieldUpdateLookup; ++i)
			{
				const FIELD_UPDATE_LOOKUP *pLookup = &sgtFieldUpdateLookup[ i ];
				if (TESTBIT( dwDBUnitFieldBits, pLookup->eField ))
				{
					if (bFirstType)
					{
						PStrCopy( szUpdateType, pLookup->pszName, MAX_STRING );
						bFirstType = FALSE;
					}
					else
					{
						PStrCat( szUpdateType, " ", MAX_STRING );
						PStrCat( szUpdateType, pLookup->pszName, MAX_STRING );
					}
					
				}
				
			}
		}
			
		// construct message
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf(
			szMessage,
			MAX_MESSAGE,
			"DBOP_UPDATE: [%s] guid=%I64d id=%d (%s)\n", 
			szUpdateType,
			UnitGetGuid( pUnit ), 
			UnitGetId( pUnit ),
			UnitGetDevName( pUnit ));

		// trace
		trace( szMessage );

		// send to clients
		if (gbDBUnitLogInformClients)
		{
			GAME *pGame = UnitGetGame( pUnit );
			sSendDBUnitLogToClients( pGame, szMessage, DBOP_UPDATE );
		}
				
	}
	
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DatabaseUnitOperation(
	UNIT *pUltimateOwner,
	UNIT *pUnit,
	DATABASE_OPERATION eOperation,
	DWORD dwMinTicksBetweenCommits /*= 0*/,
	DBUNIT_FIELD eField /*= DBUF_INVALID*/,
	DBOPERATION_CONTEXT* pOpContext /*= NULL*/,
	DWORD dwDatabaseOperationFlags /*= 0*/)
{
	ASSERTX_RETFALSE( pUltimateOwner, "Expected ultimate owner unit" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// server only
	GAME *pGame = UnitGetGame( pUnit );
	if (IS_SERVER( pGame ) == FALSE)
	{
		return TRUE;
	}
			
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )

	// if incremental db units are not enabled, forget it
	if (sgbIncrementalDBUnitsEnabled == FALSE)
	{
		return TRUE;  // not an error, just not enabled
	}

	// what tick would we commit this operation on
	DWORD dwTickCommit = dwMinTicksBetweenCommits + GameGetTick( pGame );
		
	// first make sure unit can interact with database at all
	if (s_DBUnitCanDoDatabaseOperation( pUltimateOwner, pUnit ) == TRUE)
	{
		DWORD dwDatabaseOperationFlagsToUse = dwDatabaseOperationFlags;
		
		// we will only keep the save owner digest flag if this is the owner
		if (pUltimateOwner != pUnit && 
			dwDatabaseOperationFlags & DOF_SAVE_OWNER_DIGEST)
		{
			dwDatabaseOperationFlagsToUse = dwDatabaseOperationFlags &= ~DOF_SAVE_OWNER_DIGEST;
		}
				 
		// get pending operation for this unit (if any)
		PENDING_OPERATION_BUCKET *pBucket = NULL;
		PENDING_OPERATION *pPending = NULL;
		if (sUnitGetPendingDatabaseOperation( pUnit, pBucket, pPending ) == FALSE)
		{
			return FALSE;
		}
		
		// given our pending operation, this new operation may override it	
		if (pPending != NULL)	
		{
			DATABASE_OPERATION eOperationNew = sCombineOperations( pUnit, eOperation, pPending->eOperation );
			GAME *pGame = UnitGetGame( pUnit );
			
			// if this resulted in a no-op, remove the pending operation, otherwise use the new operation
			if (eOperationNew == DBOP_INVALID)
			{
				sRemovePendingOperation( pGame, pPending, pBucket );
			}
			else
			{
				sChangePendingOperation( 
					pGame, 
					pUnit,
					pPending, 
					pBucket,
					eOperationNew, 
					pOpContext, 
					eField, 
					dwTickCommit,
					dwDatabaseOperationFlagsToUse);
			}
						
		}
		else
		{
			sAddPendingOperation( 
				pUnit, 
				eOperation, 
				pOpContext, 
				eField, 
				pUltimateOwner, 
				dwTickCommit,
				dwDatabaseOperationFlagsToUse);
		}

		// if we are adding or removing this unit, we have to do the same operation to all 
		// of its inventory contents (think picking up a sword with a gem in it, or dropping a
		// gun that has been modded)
		if (eOperation == DBOP_ADD || eOperation == DBOP_REMOVE)
		{
			ASSERTX( eField == DBUF_INVALID, "Expected invalid update type for add or remove operation" );
			
			UNIT *pItem = NULL;
			while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
			{
				s_DatabaseUnitOperation( 
					pUltimateOwner, 
					pItem, 
					eOperation, 
					dwMinTicksBetweenCommits, 
					DBUF_INVALID, 
					pOpContext, 
					dwDatabaseOperationFlagsToUse);
			}		
		}

		// if the flag to save the owner digest is on ... and this unit is not the owner,
		// do a database operation on the owner
		if (pUnit != pUltimateOwner && dwDatabaseOperationFlags & DOF_SAVE_OWNER_DIGEST)
		{
			s_DatabaseUnitOperation( 
				pUltimateOwner, 
				pUltimateOwner, 
				DBOP_UPDATE, 
				0, 
				DBUF_INVALID, 
				NULL, 
				DOF_SAVE_OWNER_DIGEST);
		}
		
	}
#endif

	return TRUE;  // not an error, unit just can't use database
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static void sCommitPendingOperation(
	GAME *pGame, 
	const PENDING_OPERATION *pPending)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pPending, "Expected pending operation" );

	// save character digest if requested on ultimate owner units
	if (pPending->dwDatabaseOperationFlags & DOF_SAVE_OWNER_DIGEST &&
		pPending->guidUnit == pPending->guidUltimateOwner)
	{
		UNIT *pUltimateOwner = UnitGetByGuid( pGame, pPending->guidUltimateOwner );
		ASSERTX( pUltimateOwner, "Expected ultimate owner unit for pending DB save digest information" );
		if (pUltimateOwner)
		{
			CharacterDigestSaveToDatabase( pUltimateOwner );
		}
	}
		
	// do the right operation
	if (pPending->eOperation == DBOP_ADD || pPending->eOperation == DBOP_UPDATE)
	{
		UNIT *pUnit = UnitGetById( pGame, pPending->idUnit );
		const UNIT_INFO &tUnitInfo = pPending->tUnitInfo;
		ASSERTV_RETURN( 
			pUnit, 
			"No unit for dbop:%d, guid:%I64d, devname:%s, genus:%d", 
			pPending->eOperation,
			pPending->guidUnit,
			tUnitInfo.pUnitData ? tUnitInfo.pUnitData->szName : "unknown",
			tUnitInfo.eGenus);

		// should we go through the whole process of creating the unit blob data,
		// we will create blob data when adding, when doing a full update, or
		// when we don't have partial updates enabled at all
		BOOL bCreateBlobData = FALSE;
		if (pPending->eOperation == DBOP_ADD ||
			TESTBIT( pPending->dwDBUnitUpdateFieldFlags, DBUF_FULL_UPDATE ) || 
			sgbDBUnitsUseFieldsEnabled == FALSE)
		{
			bCreateBlobData = TRUE;
		}
				
		// create db unit		
		BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		DATABASE_UNIT tDBUnit;
		int nValidationErrors = 0;
		if (s_DBUnitCreate( 
					pUnit, 
					&tDBUnit, 
					bDataBuffer, 
					arrsize( bDataBuffer ), 
					nValidationErrors, 
					bCreateBlobData) == FALSE)
		{
			ASSERTV_RETURN( 
				0, 
				"Unable to create db unit for unit (%s) [id=%d]", 
				UnitGetDevName( pUnit ), 
				UnitGetId( pUnit ));
		}
			
		// add or update
		if (pPending->eOperation == DBOP_ADD)
		{
			sDatabaseUnitAdd( pUnit, &tDBUnit, bDataBuffer, pPending->opContext.eContext );
		}
		else
		{
			sDatabaseUnitUpdate( pUnit, &tDBUnit, bDataBuffer, pPending->dwDBUnitUpdateFieldFlags, &pPending->opContext );
		}
			
	}
	else if (pPending->eOperation == DBOP_REMOVE)
	{
		sDatabaseUnitRemove( pGame, pPending );	
	}				
	else
	{
		ASSERTX_RETURN( 0, "Unknown database operation" );
	}
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitOperationsImmediateCommit(
	UNIT *pUnit)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION(DEVELOPMENT)
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	ASSERTX_RETFALSE( UnitIsPlayer(pUnit), "Expected Player Unit" );
	ASSERTX_RETFALSE( UnitGetGuid(pUnit) == UnitGetUltimateOwnerGuid(pUnit), "Unit is not its ultimate owner" );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( pGame, "Expected game" );

	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	ASSERTX_RETFALSE( idClient != INVALID_ID, "Invalid idClient" );
	
	PGUID guidUltimateOwner = UnitGetUltimateOwnerGuid(pUnit);

	ASSERTX_RETFALSE( guidUltimateOwner != INVALID_GUID, "Invalid Ultimate Owner Guid" );

	DBUNIT_GLOBALS *pGlobals = sGetDBUnitGlobals( pGame );

	// Loop through pending DB_OPs
	for (int i = 0; i < NUM_UNIT_BUCKETS; ++i)
	{
		// Get next operation
		PENDING_OPERATION_BUCKET *pBucket = &pGlobals->tBuckets[ i ];

		PENDING_OPERATION *pCurrent = pBucket->pOperations;
		while (pCurrent)
		{
			PENDING_OPERATION *pNext = pCurrent->pNext;

			// Commit this client's pending units
			if (pCurrent->guidUltimateOwner == guidUltimateOwner)
			{
				// commit to database
				sCommitPendingOperation( pGame, pCurrent );
				
				// remove pending operation
				sRemovePendingOperation( pGame, pCurrent, pBucket );
			}

			// Go to next operation
			pCurrent = pNext;
		}
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitOperationsBatchedCommit(
	UNIT *pUnit,
	DATABASE_OPERATION eOperation,
	BOOL bNewPlayer /* = FALSE */)
{
#if ISVERSION(SERVER_VERSION)
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	GAME * pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( pGame, "Expected game" );

	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	ASSERTX_RETFALSE( idClient != INVALID_ID, "Invalid idClient" );

	DBUNIT_GLOBALS *pGlobals = sGetDBUnitGlobals( pGame );

	GameServerContext * pGameContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETFALSE( pGameContext, "Expected GameServerContext" );

	// Allocate pDBUCollection from freelist
	DATABASE_UNIT_COLLECTION * pDBUCollection = FREELIST_GET( (pGameContext->m_DatabaseUnitCollectionList[0]), NULL );
	s_DatabaseUnitCollectionInit( pUnit->szName, pDBUCollection );

	int nErrorCount = 0;
	// Loop through pending DB_OPs
	for (int i = 0; i < NUM_UNIT_BUCKETS; ++i)
	{
		// Get next operation
		PENDING_OPERATION_BUCKET *pBucket = &pGlobals->tBuckets[ i ];

		PENDING_OPERATION *pCurrent = pBucket->pOperations;
		while (pCurrent)
		{
			PENDING_OPERATION *pNext = pCurrent->pNext;

			// Get pending unit
			UNIT * pCurrentUnit = UnitGetById( pGame, pCurrent->idUnit );

			// Commit this client's pending units
			if (pCurrentUnit && UnitGetGuid( pUnit ) == pCurrent->guidUltimateOwner)
			{
				// Batch like operations
				if (pCurrent->eOperation == eOperation)
				{
					// create db unit		
					DATABASE_UNIT tDBUnit;
					BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];

					int nValidationErrors = 0;
					if (s_DBUnitCreate( pCurrentUnit, &tDBUnit, bDataBuffer, arrsize( bDataBuffer ), nValidationErrors, TRUE ) == FALSE)
					{
						trace("Unable to create db unit");
						nErrorCount++;
						continue;
					}

					// add db unit to collection
					if (!s_DatabaseUnitCollectionAddDBUnit( pDBUCollection, &tDBUnit, bDataBuffer, tDBUnit.dwDataSize))
					{
						nErrorCount++;
						continue;
					}

					// Clear Buffer and get ready for another row result
					bDataBuffer[ 0 ];
					s_DBUnitInit( &tDBUnit );

					// remove pending operation
					sRemovePendingOperation( pGame, pCurrent, pBucket );
				}
			}

			// Go to next operation
			pCurrent = pNext;
		}
	}

#ifdef _DEBUG
	// List Contents of DB Unit Collection
	for (int i = 0; i < pDBUCollection->nNumUnits; i++)
	{
		trace("%I64d [%s], Ptr: %d, Size: %d\n", pDBUCollection->tUnits[ i ].guidUnit, pDBUCollection->tUnits[ i ].szName, &pDBUCollection->bUnitBlobData[ pDBUCollection->tUnits[ i ].dwDataIndex ], pDBUCollection->tUnits[ i ].dwDataSize);
	}
#endif

	if (nErrorCount > 0)
	{
		trace("DBUnit Collection for Batched DBOP_ADD Had %d Errors\n", nErrorCount);
	}

	// DB Multiplier Loop
	if (sgbDBUnitLoadMultiplyEnabled == TRUE)
	{
		for(int nIterationCount = 0; nIterationCount < NUM_DB_TRANSACTIONS; nIterationCount++)
		{
			// Apply mask
			for(int i = 0; i < pDBUCollection->nNumUnits; i++)
			{
				if (nIterationCount > 0)
				{
					pDBUCollection->tUnits[ i ].guidUnit          = s_DBUnitGuidIncrementForLoadTest(pDBUCollection->tUnits[ i ].guidUnit, nIterationCount);
					pDBUCollection->tUnits[ i ].guidUltimateOwner = s_DBUnitGuidIncrementForLoadTest(pDBUCollection->tUnits[ i ].guidUltimateOwner, nIterationCount);
					pDBUCollection->tUnits[ i ].guidContainer     = s_DBUnitGuidIncrementForLoadTest(pDBUCollection->tUnits[ i ].guidContainer, nIterationCount);
				}
			}

			// Init batched add DB request
			UnitBatchedAddToDatabase(pUnit, pDBUCollection, bNewPlayer);
		}
	}
	else
	{
		// Init batched add DB request
		UnitBatchedAddToDatabase(pUnit, pDBUCollection, bNewPlayer);
	}

	// free each db unit
	for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
	{
		DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];
		sFreeDatabaseUnit( pDBUnit );
	}

	// Free pDBUCollection
	pGameContext->m_DatabaseUnitCollectionList->Free( NULL, pDBUCollection );

#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitOperationsCommit(
	GAME *pGame)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETFALSE( pGame, "Expected game" );
	DWORD dwNow = GameGetTick( pGame );
	
	// go through all pending operations and commit them to the database
	DBUNIT_GLOBALS *pGlobals = sGetDBUnitGlobals( pGame );
	for (int i = 0; i < NUM_UNIT_BUCKETS; ++i)
	{
		PENDING_OPERATION_BUCKET *pBucket = &pGlobals->tBuckets[ i ];
		
		PENDING_OPERATION *pCurrent = pBucket->pOperations;
		while (pCurrent)
		{
			
			// get next operation
			PENDING_OPERATION *pNext = pCurrent->pNext;

			// if the commit tick is up, do it
			if (dwNow >= pCurrent->dwTickCommit)
			{
			
				// commit to database
				sCommitPendingOperation( pGame, pCurrent );
				
				// remove pending operation
				sRemovePendingOperation( pGame, pCurrent, pBucket );

			}
				
			// go to next operation
			pCurrent = pNext;
			
		}
		
	}
#endif	
	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DatabaseUnitEnable(
	UNIT *pUnit,
	BOOL bEnable)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// mark this unit as in database and now able to do operations
	UnitSetFlag( pUnit, UNITFLAG_NO_DATABASE_OPERATIONS, !bEnable );
	
	// do unit inventory too
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
	{
		s_DatabaseUnitEnable( pItem, bEnable );
	}		
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
PGUID s_DBUnitGuidIncrementForLoadTest(
	PGUID guid, 
	int nCount)
{
	PGUID mask = 0;
	switch (nCount)
	{
		case 1:
			mask = 10010000000;
			break;
		case 2:
			mask = 20020000000;
			break;
		case 3:
			mask = 30030000000;
			break;
		case 4:
			mask = 40040000000;
			break;
		case 5:
			mask = 50050000000;
			break;
		case 6:
			mask = 60060000000;
			break;
		case 7:
			mask = 70070000000;
			break;
		case 8:
			mask = 80080000000;
			break;
		case 9:
			mask = 90090000000;
			break;
		case 10:
			mask = 11011000000;
			break;
		default:
			mask = 0;
	}

	return guid + mask;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
void s_DBUCollectionCreateAll(
	UNIT *pUnit,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDBUnitDataBuffer,
	DWORD dwDBUnitDataBufferSize,
	int &nFatalErrors,
	int &nValidationErrors)	
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pDBUCollection, "Expected db unit collection" );
	ASSERTX_RETURN( pDBUnit, "Expected db unit" );
	ASSERTX_RETURN( pDBUnitDataBuffer, "Expected db unit data buffer" );
	ASSERTX_RETURN( dwDBUnitDataBufferSize > 0, "Invalid db unit data buffer size" );
	
	// this unit must be able to do db ops
	if (s_DBUnitCanDoDatabaseOperation( UnitGetUltimateOwner( pUnit ), pUnit ))
	{
	
		// create db unit
		if (!s_DBUnitCreate( pUnit, pDBUnit, pDBUnitDataBuffer, dwDBUnitDataBufferSize, nValidationErrors, TRUE ))
		{
			ASSERTV(
				0,
				"Unable to create db unit for (%s) [id=%d]",
				UnitGetDevName( pUnit ),
				UnitGetId( pUnit ));
			nFatalErrors++;
		}		
		else
		{
		
			// add to collection
			if (!s_DatabaseUnitCollectionAddDBUnit( pDBUCollection, pDBUnit, pDBUnitDataBuffer, pDBUnit->dwDataSize ))
			{
				ASSERTV(
					0,
					"Unable to add db unit to collection (%s) [id=%d]",
					UnitGetDevName( pUnit ),
					UnitGetId( pUnit ));				
				nFatalErrors++;
			}
		
		}

		if(sgbIncrementalDBUnitsEnabled == FALSE)
		{
			// do so for all of this units inventory
			UNIT *pItem = NULL;
			while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
			{
				s_DBUCollectionCreateAll( 
					pItem, 
					pDBUCollection, 
					pDBUnit, 
					pDBUnitDataBuffer, 
					dwDBUnitDataBufferSize,
					nFatalErrors,
					nValidationErrors);
			}		
		}
					
	}
			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTryMarkUnitInDatabase(
	UNIT *pUnit,
	UNIT *pUltimateOwner)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pUltimateOwner, "Expected ultiamte owner" );
	
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )

	if (s_DBUnitCanDoDatabaseOperation( pUltimateOwner, pUnit ))
	{
			
		// mark this unit
		UnitSetFlag( pUnit, UNITFLAG_IN_DATABASE, TRUE );
		
		// mark the inventory of this unit
		// add entire inventory of unit
		UNIT *pItem = NULL;
		while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
		{
			sTryMarkUnitInDatabase( pItem, pUltimateOwner );		
		}		

	}

#endif
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DBUnitPlayerAddedToGame(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player got '%s'", UnitGetDevName( pPlayer ) );
	BOOL bIsPing0Server = FALSE;
	REF( bIsPing0Server );

	#if ISVERSION(SERVER_VERSION)
		bIsPing0Server = TRUE;
	#endif
		
	// for debugging, we need to be able to mark all units as in the database
	if (sgbMarkUnitsInDatabaseOnLoad || bIsPing0Server)
	{
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pPlayer );
		sTryMarkUnitInDatabase( pPlayer, pUltimateOwner );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DBUnitLog( 
	DATABASE_OPERATION eOperation,
	const char *szMessage)
{
	ASSERTX_RETURN( szMessage, "Expected message" );
#if !ISVERSION(SERVER_VERSION)
	int nColor = CONSOLE_SYSTEM_COLOR;
	switch (eOperation)
	{
		case DBOP_ADD:		nColor = GetFontColor( FONTCOLOR_VERY_LIGHT_YELLOW ); break;
		case DBOP_UPDATE:	nColor = GetFontColor( FONTCOLOR_VERY_LIGHT_CYAN ); break;
		case DBOP_REMOVE:	nColor = GetFontColor( FONTCOLOR_VERY_LIGHT_RED ); break;
		case DBOP_ERROR:	nColor = GetFontColor( FONTCOLOR_RED ); break;
	}
	
	ConsoleString1( nColor, szMessage );
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DBUnitLogStatus( 
	BOOL bEnabled)
{
#if !ISVERSION(SERVER_VERSION)
	ConsoleString( 
		GetFontColor( FONTCOLOR_VERY_LIGHT_PURPLE ), 
		"Database Unit Log: %s", 
		bEnabled ? "ENABLED" : "DISABLED");
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DBUnitLogEnable(
	GAME *pGame,
	BOOL bLogEnabled,
	BOOL bInformClientsEnabled)
{

#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )

	// save new log status
	gbDBUnitLogEnabled = bLogEnabled;
	
	// inform client status
	gbDBUnitLogInformClients = bInformClientsEnabled;
	
	// send to all clients
	if (pGame)
	{
	
		// setup message
		MSG_SCMD_DBUNIT_LOG_STATUS tMessage;
		tMessage.nEnabled = bInformClientsEnabled;

		// send message	
		s_SendMessageToAll( pGame, SCMD_DBUNIT_LOG_STATUS, &tMessage );
		
	}

#endif
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetDBUnitFieldValue(
	DATABASE_UNIT *pDBUnit,
	DBUNIT_FIELD eField,
	void *pValue,
	DBUNIT_FIELD_TYPE eValueType)
{

#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETURN( pDBUnit, "Expected db unit" );
	ASSERTX_RETURN( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	
	// check each of the bits we have for each field
	for (int i = 0; i < sgnNumFieldUpdateLookup; ++i)
	{
		const FIELD_UPDATE_LOOKUP *pLookup = &sgtFieldUpdateLookup[ i ];
		if (pLookup->eField == eField)
		{
		
			// the param passed in must match the type of this field
			ASSERTX_RETURN( pLookup->eType == eValueType, "Value type mismatch" );

			// get the value		
			if (pLookup->eType == DBFT_INT)
			{
				*((DWORD *)pValue) = *((DWORD *)((char *)pDBUnit + pLookup->nDBUnitOffset));
			}
			else if (pLookup->eType == DBFT_FLOAT)
			{
				*((float *)pValue) = *((float *)((char *)pDBUnit + pLookup->nDBUnitOffset));
			}
			else
			{
				ASSERTX( 0, "Unsupported db unit field type for set/get" );
			}
			break;
			
		}	
			
	}
	
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetDBUnitFieldValue(
	DATABASE_UNIT *pDBUnit,
	DBUNIT_FIELD eField,
	void *pValue,
	DBUNIT_FIELD_TYPE eValueType)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )

	ASSERTX_RETURN( pDBUnit, "Expected db unit" );
	ASSERTX_RETURN( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	
	// check each of the bits we have for each field
	for (int i = 0; i < sgnNumFieldUpdateLookup; ++i)
	{
		const FIELD_UPDATE_LOOKUP *pLookup = &sgtFieldUpdateLookup[ i ];
		if (pLookup->eField == eField)
		{
		
			// the param passed in must match the type of this field
			ASSERTX_RETURN( pLookup->eType == eValueType, "Value type mismatch" );
		
			// set the value	
			if (pLookup->eType == DBFT_INT)
			{
				*((DWORD *)((char *)pDBUnit + pLookup->nDBUnitOffset)) = *((DWORD *)pValue);
			}
			else if (pLookup->eType == DBFT_FLOAT)
			{
				*((float *)((char *)pDBUnit + pLookup->nDBUnitOffset)) = *((float *)pValue);
			}
			else
			{
				ASSERTX( 0, "Unsupported db unit field type for set/get" );
			}
			break;
			
		}	
			
	}

#endif
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD s_DBUnitGetFieldValueDword( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField)
{
	DWORD dwValue = 0;
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETZERO( pDBUnit, "Expected db unit" );
	ASSERTX_RETZERO( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	sGetDBUnitFieldValue( pDBUnit, eField, &dwValue, DBFT_INT );		
#endif
	return dwValue;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_DBUnitSetFieldValueDword( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField,
	DWORD dwValue)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETURN( pDBUnit, "Expected db unit" );
	ASSERTX_RETURN( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	sSetDBUnitFieldValue( pDBUnit, eField, &dwValue, DBFT_INT );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float s_DBUnitGetFieldValueFloat( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField)
{
	float flValue = 0.0f;
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETZERO( pDBUnit, "Expected db unit" );
	ASSERTX_RETZERO( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	sGetDBUnitFieldValue( pDBUnit, eField, &flValue, DBFT_FLOAT );		
#endif
	return flValue;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_DBUnitSetFieldValueFloat( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField,
	float flValue)
{
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	ASSERTX_RETURN( pDBUnit, "Expected db unit" );
	ASSERTX_RETURN( eField > DBUF_FULL_UPDATE && eField < DBUF_NUM_FIELDS, "Invalid db unit field" );
	sSetDBUnitFieldValue( pDBUnit, eField, &flValue, DBFT_FLOAT );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DBUnitDeleteFailed(
	GAME *pGame,
	PGUID guidToDelete,
	const UNIT_DATA *pUnitData)
{
	//ASSERTX_RETURN( guidToDelete != INVALID_GUID, "Expected guid" );
	//ASSERTV_RETURN( pUnitData, "Expected unit data" );
	//const int MAX_MESSAGE = 1024;
	//char szMessage[ MAX_MESSAGE ];
	//
	//// setup message
	//PStrPrintf(
	//	szMessage,
	//	MAX_MESSAGE,
	//	"DBOP_REMOVE: Error removing unit from DB! guid=%I64d (%s)",
	//	guidToDelete,
	//	pUnitData->szName);

	//// trace
	//trace( szMessage );			
	//#if ISVERSION(SERVER_VERSION)
	//	TraceError( szMessage );
	//#endif

	//// send to clients 
	//if (gbDBUnitLogInformClients)
	//{
	//	sSendDBUnitLogToClients( pGame, szMessage, DBOP_ERROR );
	//}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitBindSQLOutput(
	PLAYER_LOAD_TYPE eLoadType,
	SQLHANDLE hSQLHandle,
	DATABASE_UNIT &tDBUnit,
	BYTE *pBlobData,
	SQLLEN &slBlobLen,
	PGUID *pguidPlayer)
{
	ASSERTX_RETFALSE( hSQLHandle != NULL, "Expected SQL handle" );
	ASSERTX_RETFALSE( pBlobData, "Expected blob data" );
	SQLRETURN hRet = NULL;

	SQLUSMALLINT nCol = 1;		
	SQLLEN len = 0;	
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_SBIGINT, &tDBUnit.guidUnit, sizeof(tDBUnit.guidUnit), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit GUID" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_SBIGINT, &tDBUnit.guidUltimateOwner, sizeof(tDBUnit.guidUltimateOwner), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit GUID ultimate owner" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_SBIGINT, &tDBUnit.guidContainer, sizeof(tDBUnit.guidContainer), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit GUID container" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.eGenus, sizeof(tDBUnit.eGenus), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit genus" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.dwClassCode, sizeof(tDBUnit.dwClassCode), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit class code" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.tDBUnitInvLoc.tInvLoc.nInvLocation, sizeof(tDBUnit.tDBUnitInvLoc.tInvLoc.nInvLocation), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit inv loc slot" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.tDBUnitInvLoc.tInvLoc.nX, sizeof(tDBUnit.tDBUnitInvLoc.tInvLoc.nX), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit inv loc X" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.tDBUnitInvLoc.tInvLoc.nY, sizeof(tDBUnit.tDBUnitInvLoc.tInvLoc.nY), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit inv loc Y" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.dwExperience, sizeof(tDBUnit.dwExperience), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit experience" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.dwMoney, sizeof(tDBUnit.dwMoney), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit money" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.dwQuantity, sizeof(tDBUnit.dwQuantity), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit quantity" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_LONG, &tDBUnit.dwSecondsPlayed, sizeof(tDBUnit.dwSecondsPlayed), &len);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit seconds played" );
	hRet = SQLBindCol(hSQLHandle, nCol++, SQL_C_BINARY, pBlobData, UNITFILE_MAX_SIZE_SINGLE_UNIT, &slBlobLen);
	ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit blob data" );

	// If we're loading a character, retrieve the PlayerUnitId (ultimate_owner_id) from sp_load_character
	if (eLoadType == PLT_CHARACTER)
	{
	
		// This has to be done in the result set instead of as an OUTPUT parameter because of the order
		// result sets are populated in.
		ASSERTX_RETFALSE( pguidPlayer, "Expected player guid" );
		hRet = SQLBindCol( hSQLHandle, nCol++, SQL_C_SBIGINT, pguidPlayer, sizeof( *pguidPlayer ), 0);
		ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind player unit id" );

		// Rank
		hRet = SQLBindCol( hSQLHandle, nCol++, SQL_INTEGER, &tDBUnit.dwRank, sizeof(tDBUnit.dwRank), &len);
		ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit rank" );

		// Rank Experience		
		hRet = SQLBindCol( hSQLHandle, nCol++, SQL_INTEGER, &tDBUnit.dwRankExperience, sizeof(tDBUnit.dwRankExperience), &len);
		ASSERTX_RETFALSE( SQL_OK( hRet ), "Unable to bind dbunit rank experience" );
		
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitSQLResultPostProcess( 
	int nRowCount,
	PLAYER_LOAD_TYPE eLoadType,
	DATABASE_UNIT &tDBUnit,
	SQLLEN slBlobLen,
	PGUID guidCharacterAccountId,
	PGUID guidPlayer)
{

	// Get shared stash inventory location code
	const INVLOCIDX_DATA *pInvLocIdxData = InvLocIndexGetData((int)INVLOC_SHARED_STASH);
	DWORD dwSharedStashLocationCode = pInvLocIdxData->wCode;

	// Set dwInvLocationCode
	tDBUnit.tDBUnitInvLoc.dwInvLocationCode = tDBUnit.tDBUnitInvLoc.tInvLoc.nInvLocation;

	// Check to see if unit is in the shared stash
	if (tDBUnit.tDBUnitInvLoc.dwInvLocationCode == dwSharedStashLocationCode ||
		eLoadType == PLT_ACCOUNT_UNITS)
	{
	
		if (tDBUnit.guidUltimateOwner != guidCharacterAccountId)
		{
			// Units in shared stash must have account id as the ultimate owner
			return FALSE;
		}
		else
		{
			// Account ID is set in ultimate owner field, so set guidUltimateOwner to PlayerUnitId
			tDBUnit.guidUltimateOwner = guidPlayer;
			// Container must be set to player for top-level items (i.e. those set to account ids)
			if (tDBUnit.guidContainer == guidCharacterAccountId)
			{
				tDBUnit.guidContainer = guidPlayer;
			}
		}
	}

	// save how much blob data there is
	tDBUnit.dwDataSize = (DWORD)slBlobLen;

	// resolve the species based on genus and class code
	tDBUnit.spSpecies = UnitResolveSpecies( tDBUnit.eGenus, tDBUnit.dwClassCode );

	// species should resolve to something, if it doesn't it means that we
	// have saved invalid codes into the database or the game server
	// is not running in a mode that can recognize the code that we
	// are trying to load (the server is not in ongoing content mode for instance)
	ASSERTV( tDBUnit.spSpecies != SPECIES_NONE,
		"Error loading database unit with GUID '%I64d' - Invalid genus (%d) and/or code (%d)",
		tDBUnit.guidUnit,
		tDBUnit.eGenus,
		tDBUnit.dwClassCode);

	if (tDBUnit.spSpecies != SPECIES_NONE)
	{
					
		TraceExtraVerbose( TRACE_FLAG_GAME_NET,
			"#: %d\tBufSize: %d\tGuidUnit: %I64d\tGuidUltO: %I64d\tGuidCont: %I64d\tGenus: %d\tClassCode: %d\tSecPlayed: %lu", 
			nRowCount, 
			tDBUnit.dwDataSize, 
			tDBUnit.guidUnit, 
			tDBUnit.guidUltimateOwner, 
			tDBUnit.guidContainer, 
			tDBUnit.eGenus, 
			tDBUnit.dwClassCode,
			tDBUnit.dwSecondsPlayed);
		
		// clear rank and rank_experience fields on non-player units
		if (tDBUnit.guidUnit != tDBUnit.guidUltimateOwner)
		{
			// unit is not ultimate owner
			tDBUnit.dwRank = 0;
			tDBUnit.dwRankExperience = 0;
		}

		return TRUE;  // success
				
	}			

	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_DBUnitsLoadFromClientSaveFile(
	GAME *pGame,
	UNIT *pPlayer, // may be NULL
	UNIT *pUnitRoot,
	const CLIENT_SAVE_FILE *pClientSaveFile,
	DATABASE_UNIT_COLLECTION_RESULTS *pResults,
	DWORD dwReadUnitFlags)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pClientSaveFile, "Expected client save file" );
	ASSERTX_RETFALSE( pResults, "Expected database unit collection results" );

	// format must be database
	ASSERTX_RETFALSE( pClientSaveFile->eFormat == PSF_DATABASE, "Expected save file in database format" );
	
	// setup bit buffer from the save data (colin, check if this is necessary, we
	// might be able to get away with NULL and 0 cause this buffer is not really used)
	BIT_BUF tBBClientSaveData( pClientSaveFile->pBuffer, pClientSaveFile->dwBufferSize );

	// setup the xfer spec for the load
	XFER<XFER_LOAD> tXfer( &tBBClientSaveData );
	UNIT_FILE_XFER_SPEC<XFER_LOAD> tReadSpec( tXfer );
	tReadSpec.pRoom = NULL;
	tReadSpec.idClient = pPlayer ? UnitGetClientId( pPlayer ) : INVALID_GAMECLIENTID;
	tReadSpec.pContainer = pPlayer; 
	tReadSpec.dwReadUnitFlags = dwReadUnitFlags;

	// get the collection we're going to load	
	DATABASE_UNIT_COLLECTION *pDBUnitCollection = (DATABASE_UNIT_COLLECTION *)pClientSaveFile->pBuffer;

	// save the async id in the results for easy access (we have this here because we're 
	// trying to hide the nasty cast of the client save file buffer as a db unit collection
	// which requires special knowledge to know we can in fact do that)
	pResults->idAsyncRequest = pDBUnitCollection->idAsyncRequest;
	
	// fix up database unit collections that are used for item restores
	if (pClientSaveFile->ePlayerLoadType == PLT_ITEM_RESTORE)
	{
		s_ItemRestoreDBUnitCollectionReadyForRestore( pGame, pDBUnitCollection );
	}
		
	// do the actual load
	return s_DBUnitCollectionLoad( pGame, pUnitRoot, pDBUnitCollection, tReadSpec, *pResults );

}
