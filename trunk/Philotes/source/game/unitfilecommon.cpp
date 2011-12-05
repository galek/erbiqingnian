//----------------------------------------------------------------------------
// FILE: unitfile.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
// Factored out useful non-game specific unitreading functions for Bot use.

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "dbunit.h"
#include "gameserver.h"
#include "excel.h"
#include "prime.h"
#include "clients.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "s_message.h"
#include "unitfilecommon.h"
#include "unitfileversion.h"
#ifndef _BOT
#include "units.h"
#endif

#ifdef _BOT
const VECTOR INVALID_POSITION(0.0f, 0.0f, 0.0f);
const VECTOR INVALID_DIRECTION(0.0f, 0.0f, 0.0f);
#endif

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartLocationInit(
	START_LOCATION &tStartLocation)
{
	tStartLocation.nLevelAreaDef = INVALID_LINK;
	tStartLocation.nLevelDef = INVALID_LINK;
	tStartLocation.nDifficulty = INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitFileHeaderInit(
	UNIT_FILE_HEADER &tHeader)
{
	tHeader.dwMagicNumber = 0;
	tHeader.dwTotalSizeInBits = 0;
	tHeader.dwCursorPosOfTotalSize = 0;
	tHeader.dwCursorCRC = 0;
	tHeader.dwCRC = 0;
	tHeader.dwCursorCRCDataStart = 0;
	tHeader.dwNumBytesToCRC = 0;
	tHeader.nVersion = 0xFFFFFFFF;
	tHeader.eSaveMode = UNITSAVEMODE_INVALID;

	for (int i = 0; i < UF_MAX_START_LOCATIONS; ++i)
	{
		START_LOCATION &tLocation = tHeader.tStartLocation[ i ];
		sStartLocationInit( tLocation );
	}

	// clear out bits of file descriptors
	memclear( tHeader.dwSaveFlags, sizeof( tHeader.dwSaveFlags ) );

	// clear saved states
	tHeader.dwNumSavedStates = 0;
	for (int i = 0; i < UNIT_FILE_HEADER_MAX_SAVE_STATES; ++i)
	{
		tHeader.nSavedStates[ i ] = INVALID_LINK;
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL UnitFileCRCXfer(
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER & tHeader)
{

	// the CRC value
	XFER_DWORD( tXfer, tHeader.dwCRC, STREAM_CRC_BITS );

	// # of bytes of data to use to compute CRC value
	XFER_DWORD( tXfer, tHeader.dwNumBytesToCRC, STREAM_NUM_BYTES_TO_CRC_BITS );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLinkToCode(
	int nExcelRowLink,
	EXCELTABLE eTable,
	DWORD &dwExcelCode)
{
#ifndef _BOT
	dwExcelCode = ExcelGetCode( NULL, eTable, nExcelRowLink );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCodeToLink(
	DWORD dwExcelCode,
	EXCELTABLE eTable,
	int &nExcelRowLink)
{
#ifndef _BOT
	nExcelRowLink = ExcelGetLineByCode( NULL, eTable, dwExcelCode );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL sXferStartingLocation(
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER & tHeader)
{

	// which datatable are we interested in for starting location (level def or level area def)
	EXCELTABLE eLevelDefOrAreaTable = AppIsHellgate() ? DATATABLE_LEVEL : DATATABLE_LEVEL_AREAS;

	// older versions only stored 1 startlocation in the header (not multiple ones for each difficulty)			
	if (tHeader.nVersion <= USV_STARTING_LOCATION_SINGLE_DIFFICULTY)
	{	
	
		// we'll use the first starting location
		START_LOCATION *pStartLocation = &tHeader.tStartLocation[ 0 ];
		
		// init code for xfer
		DWORD dwLevelDefOrAreaCode = 0;
		
		// fill out value to write to disk
		int nLevelDefOrAreaLink = AppIsHellgate() ? pStartLocation->nLevelDef : pStartLocation->nLevelAreaDef;
		if (tXfer.IsSave() && nLevelDefOrAreaLink != INVALID_LINK)
		{
			sLinkToCode( nLevelDefOrAreaLink, eLevelDefOrAreaTable, dwLevelDefOrAreaCode );
		}
		
		// read or write to disk
		XFER_DWORD( tXfer, dwLevelDefOrAreaCode, STREAM_LEVEL_CODE_BITS );
		
		// when loading, resolve code to excel link and store in right structure member
		if (tXfer.IsLoad())
		{
		
			// restore code to link
			int *pnLevelDefOrAreaLink = AppIsHellgate() ? &pStartLocation->nLevelDef : &pStartLocation->nLevelAreaDef;
			sCodeToLink( dwLevelDefOrAreaCode, eLevelDefOrAreaTable, *pnLevelDefOrAreaLink );
					
			// since no difficulty information was ever saved here before, we'll say it
			// was for the default difficulty (best we can do)
#ifndef _BOT
			pStartLocation->nDifficulty = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
#else
			pStartLocation->nDifficulty = 0;
#endif
		}
		
	}
	else if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_START_LOCATION ))
	{
	
		// xfer number of starting locations to follow
		int nNumLocations = UF_MAX_START_LOCATIONS;
		XFER_INT( tXfer, nNumLocations, STREAM_NUM_STARTING_LOCATION_BITS );
		
		// xfer each location
		START_LOCATION tStartLocation;
		for (int i = 0; i < nNumLocations; ++i)
		{
		
			// get pointer to starting location (either in the header, or on the stack
			// if at some point we decreased the storage)
			START_LOCATION *pStartLocation = NULL;
			if (i < UF_MAX_START_LOCATIONS)
			{
				pStartLocation = &tHeader.tStartLocation[ i ];
			}
			else
			{
				// the file contains more start locations then we currently support,
				// we're going to throw the extra ones away, but really we should
				// have a version function for this if it ever happens
				pStartLocation = &tStartLocation;
				sStartLocationInit( *pStartLocation );
			}
			ASSERTX_RETFALSE( pStartLocation, "Expected start location" );
		
			// init xfer variables
			DWORD dwLevelDefOrAreaCode = INVALID_CODE;
			DWORD dwDifficultyCode = INVALID_CODE;
			
			// set the level def or level area def
			int nLevelDefOrArea = AppIsHellgate() ? pStartLocation->nLevelDef : pStartLocation->nLevelAreaDef;			
			if (tXfer.IsSave() && nLevelDefOrArea != INVALID_LINK)
			{
				sLinkToCode( nLevelDefOrArea, eLevelDefOrAreaTable, dwLevelDefOrAreaCode );
				sLinkToCode( pStartLocation->nDifficulty, DATATABLE_DIFFICULTY, dwDifficultyCode );
			}
			
			// read or write to disk
			dwLevelDefOrAreaCode = dwLevelDefOrAreaCode & 0x0000FFFF;  // make it fit in a word
			dwDifficultyCode = dwDifficultyCode & 0x0000FFFF;  // make it fit in a word
			XFER_DWORD( tXfer, dwLevelDefOrAreaCode, STREAM_LEVEL_CODE_BITS );
			XFER_DWORD( tXfer, dwDifficultyCode, STREAM_BITS_DIFFICULTY_CODE );
							
			// resolve back to excel link when reading
			if (tXfer.IsLoad())
			{
			
				// restore code to link for level def or level area def
				int *pnLevelDefOrAreaLink = AppIsHellgate() ? &pStartLocation->nLevelDef : &pStartLocation->nLevelAreaDef;
				sCodeToLink( dwLevelDefOrAreaCode, eLevelDefOrAreaTable, *pnLevelDefOrAreaLink );
				
				// restore code to link for difficulty
				sCodeToLink( dwDifficultyCode, DATATABLE_DIFFICULTY, pStartLocation->nDifficulty );
				
			}

		}			
		
	}
				
	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL UnitFileXferHeader(
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader)
{

	// the version must always the very first thing you ever read/write or you can't change anything before it
	unsigned int nCurrentVersion = USV_CURRENT_VERSION;
	unsigned int nVersion = nCurrentVersion;
	XFER_VERSION( tXfer, nVersion );
	tHeader.nVersion = nVersion;  // save off in header

	// xfer save mode
	XFER_ENUM( tXfer, tHeader.eSaveMode, STREAM_BITS_SAVE_MODE );

	// check for old version where we only saved one DWORD of flags
	if (tHeader.nVersion <= USV_ONE_DWORD_FOR_SAVE_FLAGS)
	{
	
		// read bits into first block
		XFER_DWORD( tXfer, tHeader.dwSaveFlags[ 0 ], STREAM_SAVE_FLAG_BLOCK_BITS );
		
	}
	else
	{
	
		// xfer num groups of flags we have
		DWORD dwNumFlagGroups = NUM_UNIT_SAVE_FLAG_BLOCKS;
		XFER_DWORD( tXfer, dwNumFlagGroups, STREAM_NUM_FLAG_BLOCKS_BITS );
		ASSERTX_RETFALSE( dwNumFlagGroups <= NUM_UNIT_SAVE_FLAG_BLOCKS, "Save file encountered with more save bit blocks that the code no longer supports!" );

		// xfer each of the blocks of bits
		for (unsigned int i = 0; i < dwNumFlagGroups; ++i)
		{
			XFER_DWORD( tXfer, tHeader.dwSaveFlags[ i ], STREAM_SAVE_FLAG_BLOCK_BITS );
		}
				
	}
	
	// set cursor position of where the total size can be found in the stream
	tHeader.dwCursorPosOfTotalSize = tXfer.GetCursor();
	tHeader.dwTotalSizeInBits = 0;
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INCLUDE_BLOCK_SIZES ))
	{
		XFER_DWORD( tXfer, tHeader.dwTotalSizeInBits, STREAM_BITS_TOTAL_SIZE_BITS );
	}

	// header information
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_HEADER ))
	{

		// magic number for type of unit to follow in file
		XFER_DWORD( tXfer, tHeader.dwMagicNumber, STREAM_MAGIC_NUMBER_BITS );
		ASSERTX_RETFALSE( tHeader.dwMagicNumber == UNITSAVE_MAGIC_NUMBER || tHeader.dwMagicNumber == PLAYERSAVE_MAGIC_NUMBER, "Invalid unit file magic number" );

		// CRC data
		if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_CRC ))
		{

			// position of the CRC in the file
			tHeader.dwCursorCRC = tXfer.GetCursor();

			// xfer CRC (for real saves, we will back up and write the real place holder after we're done writing data)
			if (UnitFileCRCXfer( tXfer, tHeader ) == FALSE)
			{
				return FALSE;
			}

			// the next position in the buffer is the beginning of the data to CRC,
			// the buffer must be byte aligned at this point so we can get the pointer
			// at the start of the data
			ASSERTX_RETFALSE( tXfer.IsByteAligned(), "Can't get pointer to beginning of data to CRC ... data before this point must be byte aligned" );		

			// cursor position used for the start of the data to CRC
			tHeader.dwCursorCRCDataStart = tXfer.GetCursor();
			XFER_DWORD( tXfer, tHeader.dwCursorCRCDataStart, STREAM_CURSOR_SIZE_BITS );

		}

		// starting location
		if (sXferStartingLocation( tXfer, tHeader ) == FALSE)
		{
			ASSERTX_RETFALSE( 0, "Error xfering starting level def" );
		}

	}

	// saved states
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_SAVESTATES_IN_HEADER ))
	{
	
		// how many states are there
		XFER_DWORD( tXfer, tHeader.dwNumSavedStates, STREAM_NUM_SAVED_STATES_BITS );
		
		// xfer each state
		for (DWORD i = 0; i < tHeader.dwNumSavedStates; ++i)
		{
			DWORD dwStateCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				int nState = tHeader.nSavedStates[ i ];
				dwStateCode = ExcelGetCode( NULL, DATATABLE_STATES, nState );
			}
			
			// xfer state code
			XFER_DWORD( tXfer, dwStateCode, STREAM_BITS_STATE_CODE );
			
			// resolve to state from code upon load
			if (tXfer.IsLoad())
			{
				sCodeToLink( dwStateCode, DATATABLE_STATES, tHeader.nSavedStates[ i ] );
			}
			
		}
		
	}
	
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWorldPositionSetDefaults(
	WORLD_POSITION &tWorldPos)
{	
	tWorldPos.idRoom = INVALID_ID;
	tWorldPos.vPosition = INVALID_POSITION;
	tWorldPos.vDirection = cgvYAxis;
	tWorldPos.vUp = cgvZAxis;
	tWorldPos.flVelocityBase = 0.0f;
	tWorldPos.flScale = 1.0f;
	tWorldPos.nAngle = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventoryPositionSetDefaults(
	INVENTORY_POSITION & tInvPos)
{
	tInvPos.idContainer = INVALID_ID;
	tInvPos.pContainer = NULL;
	tInvPos.dwInvLocationCode = INVALID_CODE;
#ifndef _BOT
	tInvPos.tInvLoc.nInvLocation = INVLOC_NONE;
#endif
	tInvPos.tInvLoc.nX = 0;
	tInvPos.tInvLoc.nY = 0;
	tInvPos.tiEquipTime = TIME_ZERO;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitPositionSetDefaults(
	UNIT_POSITION & tUnitPos)
{

	tUnitPos.eType = POSITION_TYPE_INVALID;

	WORLD_POSITION &tWorldPos = tUnitPos.tWorldPos;
	sWorldPositionSetDefaults(tWorldPos);

	INVENTORY_POSITION &tInvPos = tUnitPos.tInvPos;
	sInventoryPositionSetDefaults(tInvPos);
}


//----------------------------------------------------------------------------
// Not defined for bots because bots don't save or have UNIT *
//----------------------------------------------------------------------------
#ifndef _BOT
static BOOL sSetUnitPosition(
	UNIT *pUnit,
	UNIT_POSITION &tUnitPosition,
	UNIT_FILE_HEADER &tHeader)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// get inv and world position structs to make this read a little better
	INVENTORY_POSITION &tInvPos = tUnitPosition.tInvPos;
	WORLD_POSITION &tWorldPos = tUnitPosition.tWorldPos;

	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INVENTORY_LOCATION ))
	{
		UNIT *pContainer = UnitGetContainer( pUnit );
		ASSERTX_RETFALSE( pContainer, "Expected container" );

		// in an inventory, get container
		tInvPos.idContainer = UnitGetId( pContainer );
		tInvPos.pContainer = pContainer;
		
		// get position in inventory
		UnitGetInventoryLocation( pUnit, &tInvPos.tInvLoc );

		// get code for inv location
		tInvPos.dwInvLocationCode = UnitGetInvLocCode( pContainer, tInvPos.tInvLoc.nInvLocation );
		
		// this pos in an inventory pos			
		tUnitPosition.eType = POSITION_TYPE_INVENTORY;

		// equip time
		tUnitPosition.tInvPos.tiEquipTime = UnitGetInventoryEquipTime(pUnit);
		
	}
	else if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_POSITION ))
	{

		// in the world
		ROOM *pRoom = UnitGetRoom( pUnit );
		ASSERTX_RETFALSE( pRoom, "Expected room" );
		tWorldPos.idRoom = RoomGetId( pRoom );
		tWorldPos.vPosition = UnitGetPosition( pUnit );
		tWorldPos.vDirection = UnitGetFaceDirection( pUnit, FALSE );
		tWorldPos.vUp = UnitGetUpDirection( pUnit );
		tWorldPos.nAngle = UnitIsPlayer( pUnit ) ? UnitGetPlayerAngle( pUnit ) : 0;
		tWorldPos.flScale = UnitGetScale( pUnit );
		tWorldPos.flVelocityBase = 0.0f;
		tUnitPosition.eType = POSITION_TYPE_WORLD;  // this pos is a world pos

	}

	return TRUE;

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL UnitPositionXfer(
	GAME *pGame,
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	UNIT_FILE_XFER_SPEC<mode> &tSpec,
	UNIT_POSITION &tUnitPosition,
	DATABASE_UNIT *pDBUnit)
{

	// set defaults
	sUnitPositionSetDefaults( tUnitPosition );

	// will we save anything at all
	if (UnitFileHasPosition( tHeader ))
	{

		// fill out unit position if necessary
#ifndef _BOT
		if (tXfer.IsSave())
		{
			if (sSetUnitPosition( pUnit, tUnitPosition, tHeader ) == FALSE)
			{
				return FALSE;
			}
		}
#endif
		
		// xfer type of data to follow
		XFER_ENUM( tXfer, tUnitPosition.eType, STREAM_BITS_POSITION_TYPE_BITS );
			
		// inv position	
		if (tUnitPosition.eType == POSITION_TYPE_INVENTORY)
		{
			INVENTORY_POSITION &tInvPos = tUnitPosition.tInvPos;

			// container unit id
			if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_CONTAINER_ID))
			{
				// container id
				XFER_UNITID(tXfer, tInvPos.idContainer);

				// look up container when loading
#ifndef _BOT
				tInvPos.pContainer = UnitGetById(pGame, tInvPos.idContainer);
#endif
			}			

			// inventory location slot
			if (UnitFileIsForDisk( tHeader ))
			{

				// if we have a database unit, the dbunit inv location takes priority over the blob data
				if (pDBUnit)
				{
					tInvPos.dwInvLocationCode = pDBUnit->tDBUnitInvLoc.dwInvLocationCode;
				}
			
				// xfer to blob
				XFER_DWORD( tXfer, tInvPos.dwInvLocationCode, STREAM_MAX_INVLOC_CODE_BITS );

				// if we have a database unit, the dbunit inv location takes priority over the blob data
				if (pDBUnit)
				{
					tInvPos.dwInvLocationCode = pDBUnit->tDBUnitInvLoc.dwInvLocationCode;
				}
								
				// must have valid code
				ASSERTX_RETFALSE( tInvPos.dwInvLocationCode != INVALID_CODE, "Expected code" );
				
			}
			else
			{
				// not for disk, just use the value
				XFER_UINT( tXfer, (unsigned int &)tInvPos.tInvLoc.nInvLocation, STREAM_MAX_INVLOC_BITS );
			}

			// if db unit is present, the field data takes priority over blob data
			if (pDBUnit)
			{
				tInvPos.tInvLoc.nX = pDBUnit->tDBUnitInvLoc.tInvLoc.nX;
				tInvPos.tInvLoc.nY = pDBUnit->tDBUnitInvLoc.tInvLoc.nY;
			}

			// inventory location position
			XFER_UINT(tXfer, (unsigned int &)tInvPos.tInvLoc.nX, STREAM_MAX_INVPOS_X_BITS);
			XFER_UINT(tXfer, (unsigned int &)tInvPos.tInvLoc.nY, STREAM_MAX_INVPOS_Y_BITS);

			// if db unit is present, the field data takes priority over blob data
			if (pDBUnit)
			{
				tInvPos.tInvLoc.nX = pDBUnit->tDBUnitInvLoc.tInvLoc.nX;
				tInvPos.tInvLoc.nY = pDBUnit->tDBUnitInvLoc.tInvLoc.nY;
			}
			
			// equip time
			XFER_TIME(tXfer, tInvPos.tiEquipTime);
			
		}
		else
		{
			WORLD_POSITION &tWorldPos = tUnitPosition.tWorldPos;

			// room
			XFER_ROOMID( tXfer, tWorldPos.idRoom );

			// position
			XFER_VECTOR( tXfer, tWorldPos.vPosition );

			// direction
			XFER_VECTOR( tXfer, tWorldPos.vDirection );

			// up
			XFER_VECTOR( tXfer, tWorldPos.vUp );

			// angle
			XFER_UINT( tXfer, tWorldPos.nAngle, STREAM_BITS_ANGLE );

			// scale
			XFER_FLOAT( tXfer, tWorldPos.flScale );

			// velocity
			XFER_FLOAT( tXfer, tWorldPos.flVelocityBase );
		}
	}
	else if (tXfer.IsLoad() && tSpec.bIsInInventory && pDBUnit)
	{
		
		// there is no inventory location saved, but if we have a db unit, we want that to take
		// priority ... this is a wacky case that can happen when we create items out of thin
		// air and try to email them to people via the CSR tools or other system emails
		tUnitPosition.eType = POSITION_TYPE_INVENTORY;
		INVENTORY_POSITION &tInvPos = tUnitPosition.tInvPos;		
		tInvPos.dwInvLocationCode = pDBUnit->tDBUnitInvLoc.dwInvLocationCode;		
		tInvPos.tInvLoc.nX = pDBUnit->tDBUnitInvLoc.tInvLoc.nX;
		tInvPos.tInvLoc.nY = pDBUnit->tDBUnitInvLoc.tInvLoc.nY;
		
	}
	

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileIsForDisk(
	const UNIT_FILE_HEADER &tHeader)
{

	switch (tHeader.eSaveMode)
	{

		//----------------------------------------------------------------------------
	case UNITSAVEMODE_FILE:
	case UNITSAVEMODE_CLIENT_ONLY:
	case UNITSAVEMODE_DATABASE:
	case UNITSAVEMODE_DATABASE_FOR_SELECTION:
		{
			return TRUE;
		}

		//----------------------------------------------------------------------------
	case UNITSAVEMODE_SENDTOSELF:
	case UNITSAVEMODE_SENDTOOTHER:
	case UNITSAVEMODE_SENDTOOTHER_NOID:
		{
			return FALSE;
		}

		//----------------------------------------------------------------------------
	default:
		{
			ASSERTX_RETFALSE( 0, "Unknown save mode" );
		}

	}

	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileHasPosition(
	UNIT_FILE_HEADER &tHeader)
{
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INVENTORY_LOCATION ) ||
		TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_POSITION ))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL UnitFileGUIDXfer( 
	XFER<mode> &tXfer, 
	GAME *pGame,
	UNIT_FILE_HEADER &tHeader, 
	UNIT_FILE_XFER_SPEC<mode> &tSpec,
	PGUID *pGuid,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pGuid, "Expected GUID storage" );

	// get the GUID
	PGUID guidUnitBeforeXfer = pUnit ? UnitGetGuid( pUnit ) : INVALID_GUID;

	// use the GUID in the db unit (if present)
	if (TESTBIT( tSpec.dwReadUnitFlags, RUF_USE_DBUNIT_GUID_BIT ))
	{
		const DATABASE_UNIT *pDBUnit = tSpec.pDBUnit;
		if (pDBUnit)
		{
			guidUnitBeforeXfer = pDBUnit->guidUnit;
		}
	}
	
	// xfer the GUID					
	XFER_GUID(tXfer, *pGuid);			

	// use the GUID in the db unit (if present)
	if (TESTBIT( tSpec.dwReadUnitFlags, RUF_USE_DBUNIT_GUID_BIT ))
	{
		const DATABASE_UNIT *pDBUnit = tSpec.pDBUnit;
		if (pDBUnit)
		{
			*pGuid = pDBUnit->guidUnit;
		}
	}

#ifndef _BOT

	if (tXfer.IsLoad())
	{
	
		// really, you must have a GUID ... however, for the GM single player
		// save files we incorrectly saved a turret in the players inventory
		// that didnt' have a GUID.  So, for single player, we'll be a little
		// more forgiving about having a GUID because it's only absolutely 
		// necessary for the multiplayer database environment anyway
		if (*pGuid == INVALID_GUID && 
			tHeader.eSaveMode == UNITSAVEMODE_FILE &&
			AppIsSinglePlayer() &&
			pGame != NULL && 
			IS_SERVER( pGame ))
		{
		
			// go back to the one the unit had
			*pGuid = guidUnitBeforeXfer;
			
			// if still no GUID, generate a new one
			if (*pGuid == INVALID_GUID)
			{
				*pGuid = GameServerGenerateGUID();
			}
			
		}

		// you must have a GUID
		ASSERTV_RETFALSE( 
			*pGuid != INVALID_GUID,
			"Invalid GUID encounted on load for %s",
			pUnit ? UnitGetDevName( pUnit ) : "UNKNOWN" );

	}

#endif
		
	return TRUE;	
	
}