//----------------------------------------------------------------------------
// FILE: dbunit.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __DB_UNIT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __DB_UNIT_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _INVENTORY_H_
#include "inventory.h"
#endif

#ifndef _S_MESSAGE_H_
#include "s_message.h"
#endif

#include "player.h"
#include "unitfile.h"

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
void s_DBUnitInit( struct DATABASE_UNIT *pDBUnit );
BOOL UnitIsLogged( UNIT *pUnit );

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
#define MAX_BATCHED_OPERATIONS					8
#define MAX_DBUNIT_FREELIST_SIZE	32

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum DATABASE_UNIT_CONSTANTS
{
	MAX_DB_UNIT_NAME = 64
};

//----------------------------------------------------------------------------
enum DATABASE_UNITS_STATUS
{
	DUS_POST_PROCESSED_BIT,		// file has been post processed
	DUS_HAS_ROOT_UNIT_BIT,		// this collection has a root unit
};

//----------------------------------------------------------------------------
enum DATABASE_OPERATION
{
	DBOP_ERROR = -2,
	DBOP_INVALID = -1,
	
	DBOP_ADD,
	DBOP_UPDATE,
	DBOP_REMOVE,
	
	DBOP_NUM_OPERATIONS				// keep this last please
	
};

//----------------------------------------------------------------------------
enum DATABASE_OPERATION_FLAG
{
	DOF_SAVE_OWNER_DIGEST =		MAKE_MASK( 0 ),		// save character digest of ultimate owner unit
	DOF_HAS_DELETE_DB_UNIT =	MAKE_MASK( 1 ),		// delete operation has successfully saved a db unit when the delete request was issued
};

//----------------------------------------------------------------------------
enum DBOPERATION_CONTEXT_TYPE
{
	DBOC_INVALID = -1,

	// TODO add contexts for acquiring items? (buy, pickup, trade, reward)
	DBOC_PICKUP,
	DBOC_BUY,
	DBOC_SELL,
	DBOC_TRADE,
	DBOC_MAIL,
	DBOC_DROP,
	DBOC_UPGRADE,
	DBOC_DISMANTLE,
	DBOC_DESTROY,
};

//----------------------------------------------------------------------------
enum DBUNIT_FIELD
{
	DBUF_INVALID = -1,
	
	DBUF_FULL_UPDATE,				// a full update of all unit information in the DB
	
	// stats fields
	DBUF_EXPERIENCE,				// just update the xp
	DBUF_MONEY,						// just update the money
	DBUF_QUANTITY,					// just update the quantity field
	
	// other fields
	DBUF_INVENTORY_LOCATION,		// just update the inventory location

	DBUF_RANK_EXPERIENCE,			// update rank experience
	
	DBUF_NUM_FIELDS					// keep this last please
	
};

//----------------------------------------------------------------------------
struct DBUNIT_INVENTORY_LOCATION
{
	DWORD dwInvLocationCode;		// code of inv slot in tInvLoc.nInvLocation
	INVENTORY_LOCATION tInvLoc;		// excel row dwInvLocationCode and (x,y) position in inventory
};

//----------------------------------------------------------------------------
struct DBOPERATION_CONTEXT
{
	UNIQUE_ACCOUNT_ID idAccount;
	PGUID guidCharacter;

	WCHAR wszCharacterName[ MAX_CHARACTER_NAME ];
	WCHAR wszUnitDisplayName[ MAX_ITEM_NAME_STRING_LENGTH ];

	DBOPERATION_CONTEXT_TYPE eContext;
	BOOL bIsLogged;

	DBOPERATION_CONTEXT::DBOPERATION_CONTEXT()
		: idAccount(INVALID_ID), guidCharacter(INVALID_GUID), eContext(DBOC_INVALID), bIsLogged(FALSE)
	{
		wszCharacterName[0] = L'\0';
		wszUnitDisplayName[0] = L'\0';
	}

	DBOPERATION_CONTEXT::DBOPERATION_CONTEXT(UNIT * item, UNIT * owner, DBOPERATION_CONTEXT_TYPE context)
	{
		ASSERT(item);
		//ASSERT(owner);
			
		eContext = context;
#if ISVERSION(SERVER_VERSION)
		bIsLogged = UnitIsLogged(item);
		UnitGetName(item, wszUnitDisplayName, MAX_ITEM_NAME_STRING_LENGTH, 0);
		if( owner )
		{
			guidCharacter = UnitGetGuid(owner);
			idAccount = UnitGetAccountId(owner);
			UnitGetName(owner, wszCharacterName, MAX_CHARACTER_NAME, 0);
		}
		else
		{
			guidCharacter = INVALID_GUID;
			idAccount = INVALID_UNIQUE_ACCOUNT_ID;
			wszCharacterName[0] = L'\0';
		}
#else
		UNREFERENCED_PARAMETER(owner);

		// Client doesn't care about any of these
		bIsLogged = FALSE;
		guidCharacter = INVALID_GUID;
		idAccount = INVALID_UNIQUE_ACCOUNT_ID;
		wszCharacterName[0] = L'\0';
		wszUnitDisplayName[0] = L'\0';
#endif
	}
};

//----------------------------------------------------------------------------
struct DATABASE_UNIT
{
	DWORD dwCRC;								// CRC of the data file
	PGUID guidUnit;								// this unit GUID
	SPECIES spSpecies;							// what type of unit this is (do NOT write in DB, use genus and class code)
	GENUS eGenus;								// genus of unit
	DWORD dwClassCode;							// class code of unit
	char szName[ MAX_DB_UNIT_NAME ];			// friendly name for debugging or logs etc

	DWORD dwSecondsPlayed;						// total seconds that this unit has existed in a game
	DWORD dwExperience;							// experience
	DWORD dwMoney;								// money/gold
	DWORD dwQuantity;							// item quantity
	DWORD dwRankExperience;						// player rank experience
	DWORD dwRank;								// player rank

	PGUID guidUltimateOwner;					// who is the ultimate owner of this unit (if any)
	PGUID guidContainer;						// what unit is this unit contained inside of (if any)	

	DBUNIT_INVENTORY_LOCATION tDBUnitInvLoc;	// location in containers inventory (if any)

	DWORD dwDataIndex;							// index into db unit collection buffer that contains the blob data
	DWORD dwDataSize;							// bytes of data present at dwDataIndex

	UNIT *pUnitLoadResult;						// when loading, this is the unit result	
		
	DATABASE_UNIT::DATABASE_UNIT( void )
	{ 
		s_DBUnitInit( this );
	}			
			
};

//----------------------------------------------------------------------------
void s_DBUnitInit(
	DATABASE_UNIT *pDBUnit);

//----------------------------------------------------------------------------
struct DATABASE_UNIT_COLLECTION : LIST_SL_NODE
{

	WCHAR uszCharacterName[ MAX_CHARACTER_NAME ];			// the character name
	
	DATABASE_UNIT tUnits[ UNITFILE_MAX_NUMBER_OF_UNITS ];	// the units
	int nNumUnits;											// number of units actually present
	
	DWORD dwStatus;								// status of db unit collection see DATABASE_UNITS_STATUS
	ASYNC_REQUEST_ID idAsyncRequest;			// async request associated with this collection (if any)
	
	DWORD dwUsedBlobDataSize;					// how many bytes are currently used in bUnitBlobData
	BYTE bUnitBlobData[ UNITFILE_MAX_SIZE_ALL_UNITS ];	// blob data for the units
	
};

//----------------------------------------------------------------------------
struct DATABASE_UNIT_COLLECTION_RESULTS
{
	UNIT *pUnits[ UNITFILE_MAX_NUMBER_OF_UNITS ];	// the units loaded
	int nNumUnits;
	
	ASYNC_REQUEST_ID idAsyncRequest;				// async request id in db unit collection (if any)
	
	DATABASE_UNIT_COLLECTION_RESULTS::DATABASE_UNIT_COLLECTION_RESULTS( void )
		:	nNumUnits( 0 ),
			idAsyncRequest( INVALID_ID )
	{
	}
	
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_DatabaseUnitsInitForGame(
	GAME *pGame);

void s_DatabaseUnitsFreeForGame(
	GAME *pGame);
	
BOOL s_DatabaseUnitsAreEnabled(
	void);

BOOL s_IncrementalDBUnitsAreEnabled(
	void);

VOID s_SetIncrementalSave(
	BOOL useIncrementalDBUnits);

void s_DatabaseUnitCollectionInit(
	const WCHAR *puszCharacterName,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	ASYNC_REQUEST_ID idAsyncRequest = INVALID_ID);

void s_DatabaseUnitCollectionFree(
	DATABASE_UNIT_COLLECTION *&pDBUCollection);

BOOL s_DatabaseUnitCollectionAddDBUnit(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DWORD dwDataBufferSize);

BYTE *s_DBUnitGetDataBuffer(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit);
	
BOOL s_DatabaseUnitsPostProcess(
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	PLAYER_LOAD_TYPE ePlayerLoadType,
	UNIT_COLLECTION_TYPE eCollectionType,
	PGUID guidUnitTreeRoot);

UNIT *s_DBUnitLoad(	
	GAME *pGame,
	UNIT *pUnitExisting,
	DATABASE_UNIT *pDBUnit,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpecControl,
	BOOL bIsInIventory,
	BOOL bErrorCheckAlreadyLoaded);

BOOL s_DBUnitCollectionLoad(
	GAME *pGame,
	UNIT *pCollectionRoot,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpec,
	DATABASE_UNIT_COLLECTION_RESULTS &tResults);

UNIT *s_DatabaseUnitsLoadOntoExistingRootOnly(
	UNIT *pUnitRoot,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	const UNIT_FILE_XFER_SPEC<XFER_LOAD> &tReadSpec);

DATABASE_UNIT *s_DatabaseUnitGetRoot(
	DATABASE_UNIT_COLLECTION *pDBUCollection);

BOOL s_DBUnitCanDoDatabaseOperation(
	UNIT *pUltimateOwner,
	UNIT *pUnit);

BOOL s_DatabaseUnitAddAll(
	UNIT *pPlayer,
	UNIT *pUnit);

BOOL s_DBUnitCreate(
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDataBuffer,
	DWORD dwDataBufferSize,
	int &nValidationErrors,
	BOOL bCreateBlobData);
		
BOOL s_DatabaseUnitOperation(
	UNIT *pUltimateOwner,						// ultimate owner of unit
	UNIT *pUnit,								// the unit to do the operation on
	DATABASE_OPERATION eOperation,				// add, update, remove
	DWORD dwMinTicksBetweenCommits = 0,			// delay before this op becomes committed to db
	DBUNIT_FIELD eField = DBUF_INVALID,			// for updates, can be a specific update
	DBOPERATION_CONTEXT *pOpContext = NULL,
	DWORD dwDatabaseOperationFlags = 0);		// see DATABASE_OPERATION_FLAG

BOOL s_DBUnitOperationsImmediateCommit(
	UNIT *pUnit);

BOOL s_DBUnitOperationsBatchedCommit(
	UNIT *pUnit,
	DATABASE_OPERATION eOperation,
	BOOL bNewPlayer = FALSE);

BOOL s_DBUnitOperationsCommit(
	GAME *pGame);
	
void s_DatabaseUnitEnable(
	UNIT *pUnit,
	BOOL bEnable);

void s_DBUCollectionCreateAll(
	UNIT *pUnit,
	DATABASE_UNIT_COLLECTION *pDBUCollection,
	DATABASE_UNIT *pDBUnit,
	BYTE *pDBUnitDataBuffer,
	DWORD dwDBUnitDataBufferSize,
	int &nFatalErrors,
	int &nValidationErrors);

void s_DBUnitPlayerAddedToGame(
	UNIT *pPlayer);

void s_DBUnitLogEnable(
	GAME *pGame,
	BOOL bLogEnabled,
	BOOL bInformClientsEnabled);
	
void c_DBUnitLog( 
	DATABASE_OPERATION eOperation,
	const char *szMessage);
	
void c_DBUnitLogStatus( 
	BOOL bEnabled);

DWORD s_DBUnitGetFieldValueDword( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField);
	
void s_DBUnitSetFieldValueDword( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField,
	DWORD dwValue);

float s_DBUnitGetFieldValueFloat( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField);
	
void s_DBUnitSetFieldValueFloat( 
	DATABASE_UNIT *pDBUnit, 
	DBUNIT_FIELD eField,
	float flValue);

void s_DBUnitDeleteFailed(
	GAME *pGame,
	PGUID guidToDelete,
	const UNIT_DATA *pUnitData);

PGUID s_DBUnitGuidIncrementForLoadTest(
	PGUID guid, 
	int nCount);

VOID s_SetDBUnitLoadMultiply(
	BOOL useDBLoadMultiply);
	
DWORD s_GetDBUnitTickDelay(
	DBUNIT_FIELD eField);

BOOL UnitIsLogged(
	UNIT *pUnit);
	
BOOL s_DBUnitBindSQLOutput(
	PLAYER_LOAD_TYPE eLoadType,
	SQLHANDLE hSQLHandle,
	DATABASE_UNIT &tDBUnit,
	BYTE *pBlobData,
	SQLLEN &slBlobLen,
	PGUID *pguidPlayer);

BOOL s_DBUnitSQLResultPostProcess( 
	int nRowCount,
	PLAYER_LOAD_TYPE eLoadType,
	DATABASE_UNIT &tDBUnit,
	SQLLEN slBlobLen,
	PGUID guidCharacterAccountId,
	PGUID guidPlayer);

BOOL s_DBUnitsLoadFromClientSaveFile(
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pUnitRoot,
	const CLIENT_SAVE_FILE *pClientSaveFile,
	DATABASE_UNIT_COLLECTION_RESULTS *pResults,
	DWORD dwReadUnitFlags = 0);		// see READ_UNIT_FLAGS
				
//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern BOOL gbDBUnitLogEnabled;
extern BOOL gbDBUnitLogInformClients;

#endif