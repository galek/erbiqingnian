//----------------------------------------------------------------------------
// FILE: c_unitnew.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_unitnew.h"
#include "game.h"
#include "s_message.h"
#include "unitfile.h"
#include "units.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum NEW_UNIT_CONSTANTS
{
	NEW_UNIT_BUCKETS = 16,	// arbitrary
};

//----------------------------------------------------------------------------
struct PARTIAL_UNIT
{
	UNITID idUnit;
	BYTE bNextSequence;
	DWORD dwTotalSize;
	DWORD dwCurrentSize;
	BYTE bData[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
	PARTIAL_UNIT *pNext;
	PARTIAL_UNIT *pPrev;
};

//----------------------------------------------------------------------------
struct PARTIAL_UNIT_GLOBALS
{
	PARTIAL_UNIT *tPartialUnit[ NEW_UNIT_BUCKETS ];
};
static PARTIAL_UNIT_GLOBALS tGlobals;

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartialUnitInit(
	PARTIAL_UNIT *pPartialUnit)
{
	ASSERTV_RETURN( pPartialUnit, "Expected partial unit" );
	
	pPartialUnit->idUnit = INVALID_ID;
	pPartialUnit->bNextSequence = 0;
	pPartialUnit->dwTotalSize = 0;
	pPartialUnit->dwCurrentSize = 0;
	pPartialUnit->pNext = NULL;
	pPartialUnit->pPrev = NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetPartialUnitBucket(
	UNITID idUnit)
{
	return idUnit % NEW_UNIT_BUCKETS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTIAL_UNIT *sPartialUnitGet(
	GAME *pGame,
	UNITID idUnit,
	DWORD dwTotalDataSize,
	BOOL bAllowCreateNew)
{
	ASSERTV_RETNULL( idUnit != INVALID_ID, "Expected unit id link" );
	ASSERTV_RETNULL( dwTotalDataSize, "Invalid total data size" );
	
	// get bucket index
	int nBucketIndex = sGetPartialUnitBucket( idUnit );

	// search for existing entry
	PARTIAL_UNIT *pPartialUnit = tGlobals.tPartialUnit[ nBucketIndex ];
	while (pPartialUnit)
	{
		
		// match unit id
		if (pPartialUnit->idUnit == idUnit)
		{
			break;
		}
		
	}

	// if not found, create a new one if we can
	if (pPartialUnit == NULL && bAllowCreateNew)
	{
	
		// allocate an init
		pPartialUnit = (PARTIAL_UNIT *)GMALLOC( pGame, sizeof( PARTIAL_UNIT ) );
		sPartialUnitInit( pPartialUnit );

		// set data
		pPartialUnit->idUnit = idUnit;
		pPartialUnit->dwTotalSize = dwTotalDataSize;
		
		// tie to list
		if (tGlobals.tPartialUnit[ nBucketIndex ] != NULL)
		{
			tGlobals.tPartialUnit[ nBucketIndex ]->pPrev = pPartialUnit;
			pPartialUnit->pNext = tGlobals.tPartialUnit[ nBucketIndex ];
			tGlobals.tPartialUnit[ nBucketIndex ] = pPartialUnit;
		}
		else
		{
			tGlobals.tPartialUnit[ nBucketIndex ] = pPartialUnit;
		}
				
	}

	return pPartialUnit;
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartialUnitRemove(	
	GAME *pGame,
	PARTIAL_UNIT *pPartialUnit)
{
	ASSERTV_RETURN( pPartialUnit, "Expected partial unit" );

	// fix up pointers in list
	if (pPartialUnit->pNext)
	{
		pPartialUnit->pNext->pPrev = pPartialUnit->pNext;
	}
	if (pPartialUnit->pPrev)
	{
		pPartialUnit->pPrev->pNext = pPartialUnit->pNext;
	}
	else
	{
		int nBucketIndex = sGetPartialUnitBucket( pPartialUnit->idUnit );
		tGlobals.tPartialUnit[ nBucketIndex ] = NULL;	
	}
	
	// free the data
	GFREE( pGame, pPartialUnit );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitNewInitForGame(
	GAME *pGame)
{

	// init all the buckets
	for (int i = 0; i < NEW_UNIT_BUCKETS; ++i)
	{
		tGlobals.tPartialUnit[ i ] = NULL;
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitNewFreeForGame(
	GAME *pGame)
{

	// free any leftover partial units we have
	for (int i = 0; i < NEW_UNIT_BUCKETS; ++i)
	{
		PARTIAL_UNIT *pPartialUnit = tGlobals.tPartialUnit[ i ];
		while (pPartialUnit)
		{
			PARTIAL_UNIT *pNext = pPartialUnit->pNext;
			sPartialUnitRemove( pGame, pPartialUnit );
			pPartialUnit = pNext;
		}
		
	}

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sCreateNewUnit(
	GAME *pGame,
	BYTE *pData,
	DWORD dwDataSize)
{
	ASSERTV_RETNULL( pData, "Expected data buffer" );
	ASSERTV_RETNULL( dwDataSize, "Invalid data size" );
	ASSERTV_RETNULL( pGame, "Expected client game" );
	
	// create a new xfer with bit buffer of data
	BIT_BUF tBitBuffer( pData, dwDataSize );
	XFER<XFER_LOAD> tXfer( &tBitBuffer );
	
	// read the new unit
	UNIT_FILE_XFER_SPEC<XFER_LOAD> tSpec(tXfer);	
	//tSpec.bIsInInventory = something maybe;?
	UNIT *pUnit = UnitFileXfer( pGame, tSpec );
	ASSERTX_RETNULL( pUnit, "Unable to create unit on client" );

//	trace( "*UNIT_NEW* %s [%d]", UnitGetDevName( pUnit ), UnitGetId( pUnit ) );	
	
	return pUnit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sReceivePartialMessage( 
	GAME *pGame,
	UNITID idUnit, 
	BYTE bSequence, 
	DWORD dwTotalDataSize, 
	const BYTE *pData, 
	DWORD dwThisDataSize)
{
	ASSERTV_RETFALSE( idUnit != INVALID_ID, "Expected unit id" );
	ASSERTV_RETFALSE( dwTotalDataSize, "Invalid total data size for partial new unit message" );
	ASSERTV_RETFALSE( pData, "Expected data buffer" );
	ASSERTV_RETFALSE( dwThisDataSize, "Invalid data buffer size" );

	// find the partial unit 
	BOOL bCreateNew = bSequence == 0;  // only allow creating on the first sequence
	PARTIAL_UNIT *pPartialUnit = sPartialUnitGet( pGame, idUnit, dwTotalDataSize, bCreateNew );
	ASSERTV_RETFALSE( pPartialUnit, "Expected partial unit" );

	// validate what we expect some of partial unit to be
	ASSERTV_GOTO( pPartialUnit->bNextSequence == bSequence, PARTIAL_UNIT_ERROR, "Partial new unit message out of sequence" );
	ASSERTV_GOTO( pPartialUnit->dwCurrentSize + dwThisDataSize <= dwTotalDataSize, PARTIAL_UNIT_ERROR, "Overrun of partial new unit data buffer" );
	ASSERTV_GOTO( pPartialUnit->dwTotalSize == dwTotalDataSize, PARTIAL_UNIT_ERROR, "Partial new unit total data size mismatch" );
	
	// add data to partial unit
	pPartialUnit->bNextSequence = bSequence + 1;
	BYTE *pPartialData = &pPartialUnit->bData[ pPartialUnit->dwCurrentSize ];
	memcpy( pPartialData, pData, dwThisDataSize );
	pPartialUnit->dwCurrentSize += dwThisDataSize;

	// if we're done, create the unit
	if (pPartialUnit->dwCurrentSize == pPartialUnit->dwTotalSize)
	{
	
		// create unit
		sCreateNewUnit( pGame, pPartialUnit->bData, pPartialUnit->dwTotalSize );
		
		// clear partial unit entry
		sPartialUnitRemove( pGame, pPartialUnit );
		
	}
	
	// success
	return TRUE;
	
PARTIAL_UNIT_ERROR:

	// delete all partial unit information
	sPartialUnitRemove( pGame, pPartialUnit );
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitNewMessageReceived(
	GAME *pGame,
	MSG_SCMD_UNIT_NEW *pMessage)
{
	ASSERTV_RETURN( pMessage, "Expected message" );

	// get information out of the message
	UNITID idUnit = pMessage->idUnit;
	BYTE bSequence = pMessage->bSequence;
	DWORD dwTotalDataSize = pMessage->dwTotalDataSize;
	BYTE *pData = pMessage->pData;
	DWORD dwThisDataSize = MSG_GET_BLOB_LEN( pMessage, pData );

	// do the simple case of this new unit message is totally contained within
	// this single message (which happens for most new unit messages)
	if (dwTotalDataSize == dwThisDataSize)
	{
		sCreateNewUnit( pGame, pData, dwThisDataSize );
	}
	else
	{
	
		// this is an incomplete message, we need to hang on to it until we have them all
		sReceivePartialMessage( pGame, idUnit, bSequence, dwTotalDataSize, pData, dwThisDataSize );
		
	}	
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * c_CreateNewClientOnlyUnit(
	BYTE *pData,
	DWORD dwDataSize)
{
	GAME *pGame = AppGetCltGame();
	ASSERT_RETNULL(pGame);

	return sCreateNewUnit(pGame, pData, dwDataSize);
}
