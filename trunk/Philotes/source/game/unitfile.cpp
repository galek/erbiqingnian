//----------------------------------------------------------------------------
// FILE: unitfile.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "units.h"
#include "gameunits.h"
#include "c_appearance.h"
#include "c_townportal.h"
#include "clients.h"
#include "combat.h"
#include "dbunit.h"
#include "inventory.h"
#include "items.h"
#include "missiles.h"
#include "monsters.h"
#include "npc.h"
#include "objects.h"
#include "player.h"
#include "quests.h"
#include "s_quests.h"
#include "tasks.h"
#include "teams.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "unitidmap.h"
#include "unitmodes.h"
#include "units.h"
#include "unittag.h"
#include "wardrobe.h"
#include "unitfilecommon.h"
#include "unitfileversiontooold.h"
#include "unitfileversion.h"
#include "bookmarks.h"
#include "hotkeys.h"
#include "warp.h"
#include "states.h"
#include "s_store.h"
#include "s_questsave.h"
#include "statssave.h"
#include "gag.h"

#if ISVERSION(SERVER_VERSION)
#include "unitfile.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
// USV_CURRENT_VERSION is in unitfileversion.h

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitSaveModeIsRunTime(
	const UNIT_FILE_HEADER &tHeader)
{
	return UnitFileIsForDisk( tHeader ) == FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSaveFlagsChangeForInventory(
	DWORD *pdwFlags,
	UNITSAVEMODE eSaveMode )
{

	// only database inventory items save CRCs since they are each themselves entries
	// in the database unit table
	if (eSaveMode != UNITSAVEMODE_DATABASE)
	{
		CLEARBIT( pdwFlags, UNITSAVE_BIT_CRC );
	}

	// never save hotkeys for inventory items, only on player units)	
	CLEARBIT( pdwFlags, UNITSAVE_BIT_HOTKEYS );
	
	// we have an inv loc to save
	SETBIT( pdwFlags, UNITSAVE_BIT_INVENTORY_LOCATION );
	
	// we want to save our container if we were saving unit ids at all
	if ( TESTBIT( pdwFlags, UNITSAVE_BIT_UNITID ) )
	{
		SETBIT( pdwFlags, UNITSAVE_BIT_CONTAINER_ID );
	}
	
	// save states
	SETBIT( pdwFlags, UNITSAVE_BIT_SAVESTATES );
	CLEARBIT( pdwFlags, UNITSAVE_BIT_SAVESTATES_IN_HEADER );
	
	// save appearance information
	SETBIT( pdwFlags, UNITSAVE_BIT_LOOKGROUP );
	
	// do not save any quest information
	CLEARBIT( pdwFlags, UNITSAVE_BIT_QUESTS_AND_TASKS );
	
	// forget tags and colors
	CLEARBIT( pdwFlags, UNITSAVE_BIT_COLORSET_ITEM_GUID );
	CLEARBIT( pdwFlags, UNITSAVE_BIT_TAGS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHeaderSetSavedStates(
	UNIT *pUnit,
	int nState,
	void *pCallbackData)
{
	ASSERTV_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( nState != INVALID_LINK, "Expected state link" );
	GAME *pGame = UnitGetGame( pUnit );
	const STATE_DATA *pStateData = StateGetData( pGame, nState );
	ASSERTV_RETURN( pStateData, "Expected state data" );

	if (StateDataTestFlag( pStateData, SDF_SAVE_IN_UNITFILE_HEADER ))
	{
	
		// must have room for state
		UNIT_FILE_HEADER &tHeader = *(UNIT_FILE_HEADER *)pCallbackData;	
		ASSERTV_RETURN( 
			tHeader.dwNumSavedStates <= UNIT_FILE_HEADER_MAX_SAVE_STATES - 1, 
			"Too many saved states for unit file header, ignoring '%s'", 
			pStateData->szName);

		// save it
		tHeader.nSavedStates[ tHeader.dwNumSavedStates++ ] = nState;

	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileSetHeader(
	UNIT_FILE_HEADER &tHeader,
	UNITSAVEMODE eSaveMode,
	UNIT *pUnit,
	BOOL bUnitIsInInventory,
	BOOL bUnitIsControlUnit )
{

	// set to most current version
	tHeader.nVersion = USV_CURRENT_VERSION;

	// set save mode
	tHeader.eSaveMode = eSaveMode;
		
	// get the flags to use for this save mode
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	UnitFileGetSaveDescriptors( pUnit, eSaveMode, tHeader.dwSaveFlags, NULL );

	// adjust the save flags for units that are in the inventory of somebody
	if (bUnitIsInInventory == TRUE)
	{
		sSaveFlagsChangeForInventory( tHeader.dwSaveFlags, eSaveMode );
	}

	if ( eSaveMode == UNITSAVEMODE_SENDTOSELF && bUnitIsControlUnit )
		SETBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_IS_CONTROL_UNIT );

	// magic number for units
	tHeader.dwMagicNumber = UNITSAVE_MAGIC_NUMBER;
	if (UnitIsPlayer( pUnit ))
	{
		tHeader.dwMagicNumber = PLAYERSAVE_MAGIC_NUMBER;  // players have their own special magic #
	}
	
	// CRC information1 (this must be set by the act of writing out or reading in)
	tHeader.dwCursorCRC = 0;
	tHeader.dwCRC = 0;
	tHeader.dwCursorCRCDataStart = 0;
	tHeader.dwNumBytesToCRC = 0;
	
	// starting level definition (or level area)
	int nDifficultyDefault = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
	int nNumDifficulties = ExcelGetNumRows( NULL, DATATABLE_DIFFICULTY );
	for (int nDifficulty = 0; nDifficulty < nNumDifficulties; ++nDifficulty)
	{
		START_LOCATION *pStartLocation = &tHeader.tStartLocation[ nDifficulty ];
				
		// assign starting level/area
		if (AppIsHellgate())
		{
			pStartLocation->nLevelDef = UnitGetStat( pUnit, STATS_LEVEL_DEF_START, nDifficulty );
			pStartLocation->nDifficulty = nDifficulty;
		}
		else
		{
		
			// mythos doesn't yet do difficulties, so we'll just save the default one
			if (nDifficulty == nDifficultyDefault)
			{
				if( UnitGetGenus( pUnit ) == GENUS_PLAYER )
				{
					int nLevelDepth = 0;
					VECTOR vPosition;
					PlayerGetRespawnLocation( pUnit, KRESPAWN_TYPE_PRIMARY, pStartLocation->nLevelAreaDef,nLevelDepth, vPosition );
					//UnitGetStartLevelAreaDefinition( pUnit, pStartLocation->nLevelAreaDef, nLevelDepth );						
				}
				pStartLocation->nDifficulty = nDifficulty;
			}
			
		}	
		
	}

	// set saved states in the unit file header
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_SAVESTATES_IN_HEADER ))
	{
		StateIterateSaved( pUnit, sHeaderSetSavedStates, &tHeader );
	}
		
	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitNameXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_NAME))
	{
	
		// get name
		WCHAR uszName[ MAX_CHARACTER_NAME ] = { 0 };
		memset( uszName, 0, sizeof( uszName ) );
		if (tXfer.IsSave())
		{
			UnitGetName( pUnit, uszName, MAX_CHARACTER_NAME, 0 );
		}
		
		// name length		
		unsigned int nLength = PStrLen( uszName, MAX_CHARACTER_NAME - 1 );
		XFER_UINT( tXfer, nLength, STREAM_BITS_NAME );

		// do each character of the name
		for (unsigned int i = 0; i < nLength; ++i)
		{
			DWORD dwChar = 0;
			if (tXfer.IsSave())
			{
				dwChar = uszName[ i ];
			}
			XFER_DWORD( tXfer, dwChar, STREAM_BITS_WCHAR );
			
			// set character when loading
			if (tXfer.IsLoad())
			{
				if (i < MAX_CHARACTER_NAME)
				{
					uszName[ i ] = (WCHAR)dwChar;
				}
			}
			
		}
		uszName[ nLength ] = 0;  // terminate
				
		// set name when loading
		if (tXfer.IsLoad())
		{
			UnitSetName( pUnit, uszName );
		}
	
	}

	return TRUE;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConditionalBitSet(
	DWORD *pdwFlags,
	int nBit)
{
	if (pdwFlags)
	{
		SETBIT( pdwFlags, nBit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileGetSaveDescriptors(
	UNIT * pUnit,
	UNITSAVEMODE eSavemode,
	DWORD *pdwFlags,
	STATS_WRITE_METHOD *peStatsWriteMethod)
{
	
	// for units that are physically in their containers, we will always send
	// inventory information because the client is guaranteed to know about the container -Colin
	if (UnitGetContainer(pUnit) && UnitIsPhysicallyInContainer(pUnit) == TRUE)
	{
		sConditionalBitSet(pdwFlags, UNITSAVE_BIT_INVENTORY_LOCATION);
	}

	// everything has a header by default
	sConditionalBitSet( pdwFlags, UNITSAVE_BIT_HEADER );
		
	switch (eSavemode)
	{
	
		//----------------------------------------------------------------------------	
		case UNITSAVEMODE_CLIENT_ONLY:
		case UNITSAVEMODE_FILE:
		{
		
			sConditionalBitSet( pdwFlags, UNITSAVE_BIT_INVENTORY );
			sConditionalBitSet( pdwFlags, UNITSAVE_BIT_UNITID_ALIASING );

			// we store things like hardcore, hardcore dead and others in saved states.  
			// We need this in most save situations - if the unit has any of these states
			sConditionalBitSet( pdwFlags, UNITSAVE_BIT_SAVESTATES );

			// when saving to files we will allow the save code to skip units in an error 
			// state to prevent total character corruption
			sConditionalBitSet( pdwFlags, UNITSAVE_BIT_INCLUDE_BLOCK_SIZES );
			
			// !!!there is *NOT* a *break* here on purpose!!!

		}
		
		//----------------------------------------------------------------------------				
		case UNITSAVEMODE_DATABASE:
		case UNITSAVEMODE_DATABASE_FOR_SELECTION:
		{

			// flags
			if (pdwFlags)
			{
			
				SETBIT( pdwFlags, UNITSAVE_BIT_CRC );
				SETBIT( pdwFlags, UNITSAVE_BIT_SAVESTATES );
						
				if ( UnitGetContainer( pUnit ) )
				{
					SETBIT( pdwFlags, UNITSAVE_BIT_INVENTORY_LOCATION );
				}

				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE );
				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE_WEAPON_AFFIXES );						
				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE_LAYERS_IN_BODY );	
				SETBIT( pdwFlags, UNITSAVE_BIT_COLORSET_ITEM_GUID );			
				SETBIT( pdwFlags, UNITSAVE_BIT_GUID );

				if (UnitIsA( pUnit, UNITTYPE_PLAYER ))
				{
				
					// save save states in the header
					SETBIT( pdwFlags, UNITSAVE_BIT_SAVESTATES_IN_HEADER );
					
					// saving players to file or the DB (not the char select DB digest tho)
					if (eSavemode != UNITSAVEMODE_DATABASE_FOR_SELECTION)
					{
						SETBIT( pdwFlags, UNITSAVE_BIT_QUESTS_AND_TASKS );
						SETBIT( pdwFlags, UNITSAVE_BIT_HOTKEYS );
						SETBIT( pdwFlags, UNITSAVE_BIT_BOOKMARKS );
						SETBIT( pdwFlags, UNITSAVE_BIT_START_LOCATION );
					}
				
				}
								
			}

			// stats write method			
			if (peStatsWriteMethod)
			{
				if (eSavemode == UNITSAVEMODE_FILE ||
					eSavemode == UNITSAVEMODE_CLIENT_ONLY)
				{
					*peStatsWriteMethod = STATS_WRITE_METHOD_FILE;
				}
				else
				{
					*peStatsWriteMethod = STATS_WRITE_METHOD_DATABASE;
				}
			}
			
			break;

		}
		
		//----------------------------------------------------------------------------	
		case UNITSAVEMODE_SENDTOSELF:
		case UNITSAVEMODE_SENDTOOTHER:
		case UNITSAVEMODE_SENDTOOTHER_NOID:
		{
		
			if (pdwFlags)
			{
			
				SETBIT( pdwFlags, UNITSAVE_BIT_POSITION );
				
				SETBIT( pdwFlags, UNITSAVE_BIT_CONTAINER_ID );
				if (eSavemode != UNITSAVEMODE_SENDTOOTHER_NOID)
				{
					SETBIT( pdwFlags, UNITSAVE_BIT_UNITID );					
				}
				if (UnitGetGuid( pUnit ) != INVALID_GUID)
				{
					SETBIT( pdwFlags, UNITSAVE_BIT_GUID );
				}
				
				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE );	
				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE_WEAPON_IDS );	
				SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE_WEAPON_AFFIXES );
				SETBIT( pdwFlags, UNITSAVE_BIT_TAGS );

				// teams are written for some types of units
				if (UnitIsA( pUnit, UNITTYPE_ITEM ) == FALSE)
				{
					SETBIT( pdwFlags, UNITSAVE_BIT_TEAM );
				}

				if ( !TESTBIT( pdwFlags, UNITSAVE_BIT_INVENTORY_LOCATION ) )
				{
					SETBIT( pdwFlags, UNITSAVE_BIT_POSITION );
					if (UnitIsA( pUnit, UNITTYPE_MONSTER ) )
					{
						SETBIT( pdwFlags, UNITSAVE_BIT_VELOCITY_BASE );
					}
					
				}
			
			}
			
			if (peStatsWriteMethod)
			{
				*peStatsWriteMethod = STATS_WRITE_METHOD_NETWORK;
			}
			break;
		}
					
	}

	if (pdwFlags)
	{
	
		SETBIT( pdwFlags, UNITSAVE_BIT_WARDROBE_LAYERS );

		if ( pUnit->pAppearanceShape )
		{
			SETBIT( pdwFlags, UNITSAVE_BIT_APPEARANCE_SHAPE );
		}

		if ( pUnit->szName )
		{
			SETBIT( pdwFlags, UNITSAVE_BIT_NAME );
		}

		if ( eSavemode != UNITSAVEMODE_DATABASE_FOR_SELECTION )
		{
			int lookgroup = UnitGetLookGroup(pUnit);
			if (lookgroup >= 0 && LookGroupGetData( UnitGetGame(pUnit), lookgroup) ) 
			{
				SETBIT( pdwFlags, UNITSAVE_BIT_LOOKGROUP );
			}

			SETBIT( pdwFlags, UNITSAVE_BIT_INTERACT_INFO );
			SETBIT( pdwFlags, UNITSAVE_BIT_STATS );

		}
		else
		{
			SETBIT( pdwFlags, UNITSAVE_BIT_UNIT_LEVEL ); // usually you get this through stats
			SETBIT( pdwFlags, UNITSAVE_BIT_MAX_DIFFICULTY ); // usually you get this through stats
		}
		
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileCheckCRC(
	DWORD dwCRC,
	BYTE *pBuffer,
	DWORD dwBufferSize)
{

	// read the header
	UNIT_FILE_HEADER tHeader;
	if (UnitFileReadHeader( pBuffer, dwBufferSize, tHeader ) == FALSE)
	{
		return FALSE;
	}

	// buffer must have header
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_HEADER ) == FALSE)
	{
		return FALSE;
	}
	
	// check CRC in header against CRC passed in (if any)
	if (dwCRC != 0)
	{
		ASSERTX_RETFALSE( dwCRC == tHeader.dwCRC, "Save/Load Error, please send character file.  Unit File CRC passed in does not match CRC in unit file header" );
	}

	// if header has no CRC, forget it
	if (tHeader.dwCRC == 0)
	{
		return TRUE;  // not an error, just doesn't have one ... maybe want to change this someday?
	}
		
	// must have cursor for CRC data
	ASSERTX_RETFALSE( tHeader.dwCursorCRCDataStart > 0, "Save/Load Error, please send character file.  No cursor pos in unit buffer for CRC data start" );
	ASSERTX_RETFALSE( tHeader.dwNumBytesToCRC > 0, "Save/Load Error, please send character file.  Size of data to CRC in buffer is zero" );
	
	// CRC the data
	BIT_BUF tBitBuf( pBuffer, dwBufferSize );
	tBitBuf.SetCursor( tHeader.dwCursorCRCDataStart );
	BYTE *pDataToCRC = tBitBuf.GetPtr();
	ASSERTX_RETFALSE( (tBitBuf.GetAllocatedLen() - tBitBuf.GetCursor() + 7) / 8 >= tHeader.dwNumBytesToCRC,
		"Don't have enough remaining bytes in the buffer to CRC!");
	DWORD dwCRCOfData = CRC( 0, pDataToCRC, tHeader.dwNumBytesToCRC );	
	
	// check against the CRC that was saved in the header
	ASSERTX_RETFALSE( dwCRCOfData == tHeader.dwCRC, "Save/Load Error, please send character file.  Unit file CRC in header does not match the data we just did a CRC check on" );
	
	return TRUE; // all good
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileHeaderCanBeLoaded(
	const UNIT_FILE_HEADER &tHeader,
	DWORD dwMagicNumber)
{

	// must match magic number
	if (tHeader.dwMagicNumber != dwMagicNumber)
	{
		return FALSE;
	}
	
	// get the oldest version we can load
	if (tHeader.nVersion <= USV_TOO_OLD_TO_LOAD)
	{
		return FALSE;
	}
		
	// can't load future versions
	ASSERTX_RETFALSE( tHeader.nVersion <= USV_CURRENT_VERSION, "Invalid future version of save file" );
		
	// we can load this version
	return TRUE;  
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileVersionCanBeLoaded(
	CLIENT_SAVE_FILE *pClientSaveFile)
{

	UNIT_FILE tUnitFile;
	if (ClientSaveDataGetRootUnitBuffer( pClientSaveFile, &tUnitFile ) == FALSE)
	{
		return FALSE;
	}

	// read header
	UNIT_FILE_HEADER hdr;
	if (UnitFileReadHeader( tUnitFile.pBuffer, tUnitFile.dwBufferSize, hdr ) == FALSE)
	{
		return FALSE;
	}
	
	// check for correct version of player file
	if (UnitFileHeaderCanBeLoaded( hdr, PLAYERSAVE_MAGIC_NUMBER ) == FALSE)
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileReadHeader(
	BYTE *pBuffer,
	unsigned int nBufferSize,
	UNIT_FILE_HEADER &hdr)
{
	ASSERT_RETFALSE( pBuffer );
	ASSERT_RETFALSE( nBufferSize );
	BIT_BUF tBitBuffer( pBuffer, nBufferSize );
	
	// setup xfer object
	XFER<XFER_LOAD> tXfer(&tBitBuffer);
	
	// read into local param (so we don't trash what's being passed in if we fail)
	UNIT_FILE_HEADER hdr2;
	if (!UnitFileXferHeader(tXfer, hdr2))
	{
		return FALSE;
	}
	
	// success, copy to param
	MemoryCopy(&hdr, sizeof(hdr), &hdr2, sizeof(hdr));
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SPECIES sUnitGetSpeciesFromCode(
	DWORD type,
	DWORD code)
{
	switch (type)
	{
	case GENUS_PLAYER:
		return MAKE_SPECIES(GENUS_PLAYER, PlayerGetByCode(NULL, code));
	case GENUS_MONSTER:
		return MAKE_SPECIES(GENUS_MONSTER, MonsterGetByCode(NULL, code));
	case GENUS_MISSILE:
		return MAKE_SPECIES(GENUS_MISSILE, code);
	case GENUS_ITEM:
		return MAKE_SPECIES(GENUS_ITEM, ItemGetClassByCode(NULL, code));
	case GENUS_OBJECT:
		return MAKE_SPECIES(GENUS_OBJECT, ObjectGetByCode(NULL, code));
	default:
		ASSERT_RETINVALID(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sCreateUnit(
	GAME *pGame,
	UNITID idUnit,
	PGUID guidUnit,
	GENUS eGenus,
	int nClass,
	GAMECLIENTID idClient,
	int nTeam,
	WORLD_POSITION &tWorldPos,
	APPEARANCE_SHAPE &tAppearanceShape,
	const UNIT_POSITION & tUnitPosition,
	int nLookGroup,
	BOOL bIsInInventory,
	BOOL bIsControlUnit )
{	
	// init return unit
	UNIT *pUnit = NULL;
	
	// get room (if any)	
	ROOMID idRoom = tWorldPos.idRoom;
	ROOM *pRoom = NULL;	
	if (idRoom != INVALID_ID)
	{
		pRoom = RoomGetByID( pGame, idRoom );	
	}

	// create the right type of unit	
	switch (eGenus)
	{
		//----------------------------------------------------------------------------	
		case GENUS_PLAYER:
		{
		
			PLAYER_SPEC tPlayerSpec;
			tPlayerSpec.idClient = idClient;
			tPlayerSpec.idUnit = idUnit;
			tPlayerSpec.guidUnit = guidUnit;
			tPlayerSpec.nPlayerClass = nClass;
			tPlayerSpec.nTeam = nTeam;
			tPlayerSpec.pRoom = pRoom;
			tPlayerSpec.vPosition = tWorldPos.vPosition;
			tPlayerSpec.vDirection = tWorldPos.vDirection;
			tPlayerSpec.nAngle = tWorldPos.nAngle;
			tPlayerSpec.pAppearanceShape = &tAppearanceShape;
			if ( bIsControlUnit )
				SET_MASK( tPlayerSpec.dwFlags, PSF_IS_CONTROL_UNIT );
			
			pUnit = PlayerInit( pGame, tPlayerSpec );
			break;
			
		}
		
		//----------------------------------------------------------------------------	
		case GENUS_MONSTER:
		{
			MONSTER_SPEC tMonsterSpec;
			tMonsterSpec.id = idUnit;
			tMonsterSpec.guidUnit = guidUnit;
			tMonsterSpec.nClass = nClass;
			tMonsterSpec.nTeam = nTeam;
			tMonsterSpec.nExperienceLevel = 0;
			tMonsterSpec.pRoom = pRoom;
			tMonsterSpec.vPosition = tWorldPos.vPosition;
			tMonsterSpec.vDirection = tWorldPos.vDirection;
			tMonsterSpec.flVelocityBase = tWorldPos.flVelocityBase;
			tMonsterSpec.flScale = tWorldPos.flScale;
			tMonsterSpec.pvUpDirection = &tWorldPos.vUp;
			tMonsterSpec.pShape = &tAppearanceShape;		
			tMonsterSpec.dwFlags = MSF_NO_DATABASE_OPERATIONS;
			
			pUnit = MonsterInit( pGame, tMonsterSpec );
			if (pUnit)
			{
				UnitClearFlag( pUnit, UNITFLAG_JUSTCREATED );
			}
			break;
		}
				
		//----------------------------------------------------------------------------	
		case GENUS_OBJECT:
		{
		
			// objects don't have GUID
			ASSERTX( guidUnit == INVALID_GUID, "Expected no GUID for object" );
			
			OBJECT_SPEC tObjectSpec;
			tObjectSpec.id = idUnit;			
			tObjectSpec.nClass = nClass;
			tObjectSpec.nTeam = nTeam;
			tObjectSpec.pRoom = pRoom;
			tObjectSpec.vPosition = tWorldPos.vPosition;
			tObjectSpec.pvFaceDirection = &tWorldPos.vDirection;
			tObjectSpec.flScale = tWorldPos.flScale;
			SETBIT( tObjectSpec.dwFlags, OSF_NO_DATABASE_OPERATIONS );
			
			pUnit = ObjectInit( pGame, tObjectSpec );
			break;			
		}
		
		//----------------------------------------------------------------------------			
		case GENUS_ITEM:
		{
			ITEM_SPEC tItemSpec;
			tItemSpec.idUnit = idUnit;
			tItemSpec.guidUnit = guidUnit;
			tItemSpec.nItemClass = nClass;
			tItemSpec.nItemLookGroup = nLookGroup;
			tItemSpec.nTeam = nTeam;
			tItemSpec.pRoom = pRoom;
			tItemSpec.pvPosition = &tWorldPos.vPosition;
			tItemSpec.nAngle = 0;
			tItemSpec.flScale = tWorldPos.flScale;
			tItemSpec.vDirection = tWorldPos.vDirection;
			tItemSpec.vUp = tWorldPos.vUp;
			SETBIT( tItemSpec.dwFlags, ISF_NO_DATABASE_OPERATIONS_BIT );
			if(IS_CLIENT(pGame))
			{
				if(tUnitPosition.eType == POSITION_TYPE_INVENTORY)
				{
					tItemSpec.eLoadType = UDLT_INVENTORY;
				}
				else
				{
					tItemSpec.eLoadType = UDLT_WORLD;
				}
			}
			
			pUnit = ItemInit( pGame, tItemSpec );
			break;
			
		}

		//----------------------------------------------------------------------------	
		case GENUS_MISSILE:
		{
			const UNIT_DATA *pMissileData = MissileGetData( NULL, nClass );
			const int MAX_MESSAGE = 1024;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( szMessage, MAX_MESSAGE, "Missile (%s) should not be sent like this!", pMissileData ? pMissileData->szName : "unknown" );
			ASSERTX_RETNULL( 0, szMessage );		
			break;
		}
		
		//----------------------------------------------------------------------------	
		default:
		{
			ASSERT(0);
			break;
		}
	}
	
	// return the newly created unit
	return pUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitSavedStatesXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader)
{	
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_SAVESTATES))
	{
		int nNumStates = 0;
		if (tXfer.IsSave())
		{
			STATS* pStats = StatsGetModList( pUnit, SELECTOR_STATE );
			for ( ; pStats; pStats = StatsGetModNext(pStats, SELECTOR_STATE) )
			{
				PARAM dwState = 0;
				if ( ! StatsGetStatAny(pStats, STATS_STATE, &dwState ) )
					continue;

				if ( dwState == INVALID_PARAM )
					continue;

				const STATE_DATA * pStateData = StateGetData( NULL, dwState );
				if ( ! pStateData )
					continue;

				if ( !StateDataTestFlag( pStateData, SDF_SAVE_WITH_UNIT ) )
					continue;
				nNumStates++;
			}
		}

		XFER_UINT( tXfer, nNumStates, STREAM_BITS_SAVED_STATE_COUNT );

		if (tXfer.IsSave())
		{
			STATS* pStats = StatsGetModList( pUnit, SELECTOR_STATE );
			for ( ; pStats; pStats = StatsGetModNext(pStats, SELECTOR_STATE) )
			{
				PARAM dwState = 0;
				if ( ! StatsGetStatAny(pStats, STATS_STATE, &dwState ) )
					continue;

				if ( dwState == INVALID_PARAM )
					continue;

				const STATE_DATA * pStateData = StateGetData( NULL, dwState );
				if ( ! pStateData )
					continue;

				if ( !StateDataTestFlag( pStateData, SDF_SAVE_WITH_UNIT ))
					continue;

				DWORD dwStateCode = ExcelGetCode( NULL, DATATABLE_STATES, dwState );

				// xfer the state code

				XFER_DWORD( tXfer, dwStateCode, STREAM_BITS_STATE_CODE );
				if(StateDataTestFlag(pStateData, SDF_PERSISTANT))
				{
					int startTime = UnitGetStat(pUnit, STATS_STATE_START_TIME, dwState);
					unsigned int duration = (unsigned int)UnitGetStat(pUnit, STATS_STATE_DURATION_SAVED, dwState);
					GAME* game = UnitGetGame(pUnit);
					int endTime = (int)GameGetTick(game);
					duration = (startTime + duration) - endTime;
					XFER_UINT( tXfer, duration, STREAM_BITS_STATE_DURATION);
				}
			}

		} else {
			for ( int i = 0; i < nNumStates; i++ )
			{
				DWORD dwStateCode = 0;
				XFER_DWORD( tXfer, dwStateCode, STREAM_BITS_STATE_CODE );
				int nState = ExcelGetLineByCode( NULL, DATATABLE_STATES, dwStateCode );

				const STATE_DATA * pStateData = StateGetData( NULL, nState );

				if (nState != INVALID_LINK)
				{
					unsigned int duration = 0;
					if(StateDataTestFlag(pStateData, SDF_PERSISTANT))
					{
						XFER_UINT( tXfer, duration, STREAM_BITS_STATE_DURATION);
						if(duration >= 0)
						{
							if ( IS_CLIENT( pUnit ) )
								c_StateSet( pUnit, pUnit, nState, 0, 0, duration );
							else
								s_StateSet( pUnit, pUnit, nState, duration );
						}
						else
							UnitSetStat(pUnit, STATS_STATE_DURATION_SAVED, nState, 0);
					}
					else
					{
						if ( IS_CLIENT( pUnit ) )
							c_StateSet( pUnit, pUnit, nState, 0, duration, INVALID_ID );
						else
							s_StateSet( pUnit, pUnit, nState, duration );
					}
				}
			}
		}
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitInteractInfoXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	GAMECLIENTID idClient,
	UNIT_FILE_HEADER &tHeader)
{	
	ASSERTX_RETFALSE( pUnit, "Expeceted unit" );
	UNITID idUnit = UnitGetId( pUnit );
	GAME *pGame = UnitGetGame( pUnit );
			
	// run time only
	if( sUnitSaveModeIsRunTime( tHeader ) )
	{
	
		// get the info
		INTERACT_INFO eInfo = INTERACT_INFO_INVALID;
		if (tXfer.IsSave())
		{
		
			// we must have a player to answer this question
			GAMECLIENT *pClient = ClientGetById( pGame, idClient );
			ASSERTX_RETFALSE( pClient, "Required client structure");
			UNIT *pPlayer = ClientGetPlayerUnit( pClient, TRUE );  // get player regardless of their client state
			
			// get the value
			eInfo = s_QuestsGetInteractInfo( pPlayer, pUnit );

		}

		// has a value at all
		BOOL bHasInfo = eInfo != INTERACT_INFO_INVALID;
		XFER_BOOL( tXfer, bHasInfo );						
		if (bHasInfo)
		{

			// xfer the value		
			XFER_ENUM( tXfer, eInfo, STREAM_BITS_INTERACT_INFO );
			
			// set it when loading
			if (tXfer.IsLoad())
			{
	#if !ISVERSION(SERVER_VERSION)
				c_InteractInfoSet( pGame, idUnit, eInfo );
	#endif			
			}

		}
		
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitBlockingXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	GAMECLIENTID idClient,
	UNIT_FILE_HEADER &tHeader)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
		
	if (UnitTestFlag( pUnit, UNITFLAG_TRIGGER) && ObjectTriggerIsWarp( pUnit ))
	{
	
		// we only do this at run time sending network messages
		ASSERTX_RETFALSE( sUnitSaveModeIsRunTime( tHeader ), "Run time only!" );
		
		// init the result
		WARP_RESTRICTED_REASON eWarpRestrictedReason = WRR_OK;
				
		// get result when writing
		if (tXfer.IsSave())
		{
			
			//  we have to have a player to ask this question		
			GAMECLIENT *pClient = ClientGetById( pGame, idClient );
			ASSERTX_RETFALSE( pClient, "Required client" );
			UNIT *pPlayer = ClientGetPlayerUnit( pClient, TRUE );  // get player regardless of in game state
			if (WarpCanOperate( pPlayer, pUnit ) != OPERATE_RESULT_OK)
			{
				// get detailed information about 
				eWarpRestrictedReason = WarpDestinationIsAvailable( pPlayer, pUnit );
			}
			
		}
		
		// xfer value
		// 5/8/2007 - OK to bump up STREAM_BITS_BLOCKING_WARP because this is
		// a runtime value only (ie, not saved to disk or DB) -cday
		XFER_ENUM( tXfer, eWarpRestrictedReason, STREAM_BITS_BLOCKING_WARP );
		
		// set blocking value when loading on clients
		if (tXfer.IsLoad())
		{
			// set as blocking or open
			if (eWarpRestrictedReason != WRR_OK)			
			{
			
				// set object as blocking
				ObjectSetBlocking( pUnit );  // sets the blocking stat to blocking
				
				// set stat for clients to get detailed reason
				if (IS_CLIENT( pGame ))
				{
					c_WarpSetBlockingReason( pUnit, eWarpRestrictedReason );
				}
				
			}
			else
			{
				ObjectClearBlocking( pUnit );  // sets the blocking stat to something clear and is no longer "unset"
			}
		
		}
		
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPushQuestsToPlayerStats(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pUnit ), "Expected player" );

	// quest system	
	if(AppIsHellgate())
	{
		if (s_QuestsWriteToStats( pUnit ) == FALSE)
		{
			return FALSE;
		}
	}

	// do the tasks
#if !ISVERSION(CLIENT_ONLY_VERSION)	
	s_TaskWriteToPlayer( pUnit );
#endif	

	return TRUE;
	
}

//----------------------------------------------------------------------------
enum MOD_LIST_XFER_RESULT
{
	MOD_LIST_ERROR = 0,
	MOD_LIST_IGNORED,
	MOD_LIST_OK,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static MOD_LIST_XFER_RESULT sModListXfer( 
	UNIT *pUnit, 
	XFER<mode> &tXfer,
	STATS *pModList,
	STATS_WRITE_METHOD eStatsWriteMethod,
	int nStatsVersion,
	DATABASE_UNIT *pDBUnit)
{

	// sanity	
	ASSERTX_RETVAL( pUnit, MOD_LIST_ERROR, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// reject some lists
	STATS_SELECTOR eSelector = SELECTOR_NONE;
	if (pModList != NULL)
	{
		ASSERTX_RETVAL( tXfer.IsSave(), MOD_LIST_ERROR, "Expected xfer save mode" );
		
		if (StatsGetCount( pModList ) <= 0 && !StatsGetRider( pModList ))
		{
			return MOD_LIST_IGNORED; // not an error, just ignored
		}
		
		eSelector = StatsGetSelector( pModList );
		if (eSelector != SELECTOR_AFFIX)
		{
			return MOD_LIST_IGNORED;  // not an error, just ignored
		}
		
	}
	
	// xfer selector
	if (StatsSelectorXfer( tXfer, eSelector, eStatsWriteMethod, nStatsVersion ) == FALSE)
	{
		return MOD_LIST_ERROR;
	}

	// xfer the statslist ID for affixes
	DWORD dwStatsListId = pModList ? StatsListGetId(pModList) : INVALID_ID;
	if(eSelector == SELECTOR_AFFIX)
	{
		if (StatsListIdXfer( tXfer, dwStatsListId, eStatsWriteMethod, nStatsVersion ) == FALSE)
		{
			return MOD_LIST_ERROR;
		}
	}
		
	// allocate a mod list when loading
	if (tXfer.IsLoad())
	{
	
		// allocate new list
		ASSERTX_RETVAL( pModList == NULL, MOD_LIST_ERROR, "Expected NULL mod list" );
		pModList = StatsListInit( pGame );
		
		// add list to unit
		StatsListAdd(pUnit, pModList, FALSE, eSelector);

		if(dwStatsListId != INVALID_ID)
		{
			StatsListSetId(pModList, dwStatsListId);
		}
	}

	// xfer the mod list
	if (StatsXfer( pGame, pModList, tXfer, eStatsWriteMethod, NULL, pUnit, pDBUnit ) == FALSE)
	{
		return MOD_LIST_ERROR;
	}

	// we have succesfully transferred a list now
	return MOD_LIST_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitStatsXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	DATABASE_UNIT *pDBUnit)
{	
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_STATS ))
	{
	
		// get the method we're writing the stats in
		STATS_WRITE_METHOD eStatsWriteMethod;
		UnitFileGetSaveDescriptors( pUnit, tHeader.eSaveMode, NULL, &eStatsWriteMethod );
	
		// stream base stats
		int nStatsVersion = NONE;
		if (StatsXfer( pGame, UnitGetStats( pUnit ), tXfer, eStatsWriteMethod, &nStatsVersion, pUnit, pDBUnit ) == FALSE)
		{
			ASSERTX_RETFALSE( 0, "sUnitStatsXfer() - StatsXfer() failed" );
		}

		// stream mod stats
		int nCursorModListCount = tXfer.GetCursor();
		unsigned int nModListCount = 0;
		XFER_UINT( tXfer, nModListCount, STREAM_BITS_MODLIST_COUNT );

		if (tXfer.IsSave())
		{
		
			// write each list that we can
			STATS *pModList = NULL;
			STATS *pNextList = StatsGetModList( pUnit );
			while ((pModList = pNextList) != NULL)
			{
											
				// xfer mod list
				MOD_LIST_XFER_RESULT eResult = sModListXfer( pUnit, tXfer, pModList, eStatsWriteMethod, nStatsVersion, pDBUnit );
				if (eResult == MOD_LIST_ERROR)
				{
					ASSERTX_RETFALSE( 0, "sUnitStatsXfer() - save mod list result error" );
				}
				
				// if we transferred a list, increment a count
				if (eResult == MOD_LIST_OK)
				{
					nModListCount++;
					ASSERTX_RETFALSE( nModListCount < (1 << STREAM_BITS_MODLIST_COUNT), "Save/Load Error, please send character file.  Too many stats mod lists" );
				}

				// get next list
				pNextList = StatsGetModNext( pModList );
				
			}

			// save next position in stream
			int nCursorNext = tXfer.GetCursor();
						
			// go back and write correct number of stats written
			tXfer.SetCursor( nCursorModListCount );
			XFER_UINT( tXfer, nModListCount, STREAM_BITS_MODLIST_COUNT );
			
			// return buffer to next position
			tXfer.SetCursor( nCursorNext );
			
		}
		else
		{

			// read each list		
			for (unsigned int i = 0; i < nModListCount; ++i)
			{
				
				// xfer each mod list
				MOD_LIST_XFER_RESULT eResult = sModListXfer( pUnit, tXfer, NULL, eStatsWriteMethod, nStatsVersion, pDBUnit );
				if (eResult == MOD_LIST_ERROR)
				{
					ASSERTX_RETFALSE( 0, "sUnitStatsXfer() - load mod list result error" );				
				}
				
				ASSERTX_RETFALSE( eResult == MOD_LIST_OK, "Save/Load Error, please send character file.  Expected success for xfer mod list" );
				
			}
			
		}
		
	} 
	else 
	{
		if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_UNIT_LEVEL))
		{
			unsigned int nExperienceLevel = UnitGetExperienceLevel( pUnit );
			XFER_UINT( tXfer, nExperienceLevel, STREAM_BITS_EXPERIENCE_LEVEL );
			if (tXfer.IsLoad())
			{
				UnitSetExperienceLevel( pUnit, nExperienceLevel );		
			}

			if (tHeader.nVersion > USV_RANK_WITH_LEVEL)
			{
				unsigned int nExperienceRank = UnitGetExperienceRank( pUnit );
				XFER_UINT( tXfer, nExperienceRank, STREAM_BITS_EXPERIENCE_RANK );
				if (tXfer.IsLoad())
				{
					UnitSetStat( pUnit, STATS_PLAYER_RANK, nExperienceRank );
				}
			}
		}
		
		if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_MAX_DIFFICULTY))
		{
			unsigned int nMaxDifficulty = UnitGetStat( pUnit, STATS_DIFFICULTY_MAX );
			XFER_UINT( tXfer, nMaxDifficulty, STREAM_BITS_MAX_DIFFICULTY );
			if (tXfer.IsLoad())
			{
				UnitSetStat( pUnit, STATS_DIFFICULTY_MAX, nMaxDifficulty );		
			}
		}
	}

	// set the version # of this unit to that used in the xfer header
	if (tXfer.IsLoad())
	{
		UnitSetStat( pUnit, STATS_UNIT_VERSION, tHeader.nVersion );
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sUnitPostProcessXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	UNIT_FILE_XFER_SPEC<mode> &tSpec)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
		
	// update alias maps for units that are in an inventory
	if (tSpec.bIsInInventory)
	{
		if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_UNITID_ALIASING ))
		{
			UnitIdMapGetOrAdd( pGame->m_pUnitIdMap, UnitGetId( pUnit ) );
		}
	}

	// loading post processing
	if (tXfer.IsLoad())
	{
	
#if ISVERSION(SERVER_VERSION)
		// init any gagged until time on players
		if (UnitIsPlayer( pUnit ))
		{
			CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
			if (pClientSystem)
			{
				GAMECLIENT *pClient = UnitGetClient( pUnit );
				if (pClient)
				{
					time_t timeGaggedUntil = ClientSystemGetGaggedUntilTime( pClientSystem, pClient->m_idAppClient );
					s_GagEnable( pUnit, timeGaggedUntil, FALSE );
				}
			}
		}
#endif
	
#if !ISVERSION(SERVER_VERSION)
		// update gfx for clients
		if (IS_CLIENT( pGame ))
		{
			// doing this will set gfx model rooms and such
			c_UnitUpdateGfx( pUnit );

			// update party UI when receiving units that are in our party
			if (UnitIsPlayer( pUnit ))
			{
				PGUID guidUnit = UnitGetGuid( pUnit );
				if (guidUnit != INVALID_GUID && 
					c_PlayerFindPartyMember( guidUnit ) != INVALID_INDEX)
				{
					c_PlayerRefreshPartyUI();
				}
			}
			
		}
#endif

	#if !ISVERSION(SERVER_VERSION)
		if( AppIsTugboat() )
		{
			BOOL bIsDead = UnitTestFlag( pUnit, UNITFLAG_DEAD_OR_DYING  );
			if ( IS_CLIENT( pGame ) && 
				 ( UnitIsA( pUnit, UNITTYPE_MONSTER ) ||
				   UnitIsA( pUnit, UNITTYPE_OBJECT ) ) && bIsDead )
			{
				int nHealth = UnitGetStat( pUnit, STATS_HP_CUR );
				if (nHealth <= 0)
				{
				
					UnitDie( pUnit, NULL );
					BOOL bHasMode = FALSE;
					UnitGetModeDuration( pGame, pUnit, MODE_SPAWNDEAD, bHasMode );
					if( bHasMode )
					{
						c_UnitSetMode( pUnit, MODE_SPAWNDEAD, 0, 0);
					}
					else
					{
						c_UnitSetMode( pUnit, MODE_DYING, 0, 0);
					}
				}
			}

		}
		else if( AppIsHellgate() )
		{
			if (IS_CLIENT( pGame ) && UnitIsA( pUnit, UNITTYPE_MONSTER ))
		
			{
				int nHealth = UnitGetStat( pUnit, STATS_HP_CUR );
				if (nHealth <= 0)
				{
					c_UnitDoDelayedDeath( pUnit );
		
					UnitDie( pUnit, NULL );
				}
			}
		}
		
		// hacky, but ok for now maybe ;) -Colin
		if (ObjectIsPortalFromTown( pUnit ))
		{
			c_TownPortalSetReturnPortalState( pUnit, NULL );
		}
		
	#endif// !ISVERSION(SERVER_VERSION)

	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileResolveInventoryLocationCode(
	UNIT *pContainer,
	DWORD dwInvLocationCode,
	int *pnInvLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected unit" );
	ASSERTX_RETFALSE( pnInvLocation, "Expected inventory location pointer" );
	ASSERTX_RETFALSE( *pnInvLocation == INVLOC_NONE, "Save/Load Error, please send character file.  Expected unresolved inventory location" );
	ASSERTX_RETFALSE( dwInvLocationCode != INVALID_CODE, "Save/Load Error, please send character file.  Expected inv loc code" );

	// get inv loc index	
	int nInvLocIdx = ExcelGetLineByCode( NULL, DATATABLE_INVLOCIDX, dwInvLocationCode );
	
	// create loc if necessary
	INVLOC *pInvLoc = InventoryGetLocAdd( pContainer, nInvLocIdx );
	ASSERTX_RETFALSE( pInvLoc, "Save/Load Error, please send character file.  Container has no storage for inventory location to resolve" );
	
	// get loc info
	INVLOC_HEADER tInvLocHeader;
	if (UnitInventoryGetLocInfo( pContainer, nInvLocIdx, &tInvLocHeader ) == FALSE)
	{
		return FALSE;
	}

	// set the pos
	*pnInvLocation = tInvLocHeader.nLocation;

	return TRUE;
			
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitPreSave(
	UNIT * pUnit )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	if (! IS_SERVER( pUnit ))
		return;

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );

	const STATS_DATA * pStatsDataSaveTime = StatsGetData( pGame, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS );
	if ( pStatsDataSaveTime && pStatsDataSaveTime->m_bUnitTypeDebug[ UnitGetGenus(pUnit) ])
	{
		GAME_TICK tGameTick = GameGetTick( pGame );
		GAME_TICK tPrevSave = (GAME_TICK)GAME_TICKS_PER_SECOND * (GAME_TICK)UnitGetStat( pUnit, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS );
		if ( tGameTick > tPrevSave )
		{
			DWORD dwSecondsElapsed = (tGameTick - tPrevSave) / GAME_TICKS_PER_SECOND;
			DWORD dwPlayedTime = UnitGetStat( pUnit, STATS_PLAYED_TIME_IN_SECONDS );
			ASSERT( dwPlayedTime + dwSecondsElapsed >= dwPlayedTime );
			if ( dwPlayedTime + dwSecondsElapsed >= dwPlayedTime )
				UnitChangeStat( pUnit, STATS_PLAYED_TIME_IN_SECONDS, dwSecondsElapsed );
			UnitSetStat( pUnit, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS, tGameTick / GAME_TICKS_PER_SECOND );
		}
	}

	if (UnitIsPlayer( pUnit ))
	{
	
		// try to free any unnecessary space taken up by stores that can be expired	
		s_StoreTryExpireOldItems( pUnit, FALSE );

		// compress map stats to save space in save files
		LevelCompressOrExpandMapStats( pUnit, LME_COMPRESS );
			
	}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sGUIDXferOld(
	XFER<mode> tXfer,
	GAME *pGame,
	UNIT_FILE_HEADER &tHeader,
	UNIT_FILE_XFER_SPEC<mode> &tSpec,
	PGUID *pguid,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pguid, "Expected guid" );	
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer GUID UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());

	if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_GUID))
	{
		if (tHeader.nVersion <= USV_GUID_NOT_NEAR_UNIT_ID)
		{
	
			*pguid = pUnit ? UnitGetGuid( pUnit ) : INVALID_GUID;
			if (UnitFileGUIDXfer( tXfer, pGame, tHeader, tSpec, pguid, pUnit ) == FALSE)
			{
				ASSERTX_RETFALSE( 0, "Unable to xfer guid" );
			}
			if (tXfer.IsLoad())
			{
				ASSERTX_RETFALSE( pUnit, "Expected unit" );
				GAME *pGame = UnitGetGame( pUnit );
				ASSERTX_RETFALSE( pGame, "Expected game" );
				
				// must have a GUID
				ASSERTX_RETFALSE( *pguid != INVALID_GUID, "Invalid GUID encounted on load" );

				// set the GUID
				if (!UnitSetGuid( pUnit, *pguid ))
				{
					// cannot collide with any other existing GUID			
					UNIT *pUnitExistingWithGUID = UnitGetByGuid( pGame, *pguid );
					REF( pUnitExistingWithGUID );
					ASSERTV_RETFALSE(
						 0,
						"Attempting to load Unit [%s] with GUID [%I64x], but GUID is already set to unit [%s]", 
						UnitGetDevName( pUnit ), 
						*pguid,
						UnitGetDevName( pUnitExistingWithGUID ));							
				}
				
			}

		}
	
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
// If you modify this function, 
// please make equivalent modifications in sReadUnit in GameBot.cpp as well.
//----------------------------------------------------------------------------
template <XFER_MODE mode>
UNIT * UnitFileXfer(
	GAME *pGame,
	UNIT_FILE_XFER_SPEC<mode> &tSpec)
{
	BOOL bDestroyUnitOnError = TRUE;
		
	// get existing unit to xfer (if any)
	UNIT *pUnit = tSpec.pUnitExisting;
	PGUID guidUnit = INVALID_GUID;
		
	// get db unit for xfer (if any)
	DATABASE_UNIT *pDBUnit = tSpec.pDBUnit;
		
	// alias xfer object
	XFER<mode> &tXfer = tSpec.tXfer;
	UNIT_POSITION tUnitPosition;
	SPECIES spSpecies = SPECIES_NONE;
	
	// get the cursor position at the start of this whole operation
	DWORD dwCursorStart = tXfer.GetCursor();
	DWORD dwCursorNext = 0;
		
	// set header for this unit
	UNIT_FILE_HEADER tHeader;
	if (tXfer.IsSave())
	{
		UNIT *pUnit = tSpec.pUnitExisting;
		ASSERTV_GOTO( pUnit, UFX_ERROR, "Expected unit" );

		// run pre-save function on unit		
		sUnitPreSave( pUnit );

		// initialize the header
		if (UnitFileSetHeader( tHeader, tSpec.eSaveMode, pUnit, tSpec.bIsInInventory, tSpec.bIsControlUnit ) == FALSE)
		{
			ASSERTV_GOTO( 0, UFX_ERROR, "Unable to set unit file header" );
		}

		// in order to write out anything, a unit MUST have a GUID.  If it doesn't there
		// is no way we can save a unit to the database in multiplayer
		if (UnitFileIsForDisk( tHeader ) == TRUE)
		{
			ASSERTV_GOTO( 
				UnitGetGuid( pUnit ) != INVALID_GUID, 
				UFX_ERROR, 
				"GUID required for unit '%s'", 
				UnitGetDevName( pUnit ));
		}
	}

	// xfer header for unit
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer HEADER UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	if (UnitFileXferHeader( tXfer, tHeader ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit header" );
	}

	// xfer bookmark placeholders
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer BOOKMARKS UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	BOOKMARKS tBookmarks;
	if (BookmarksXfer( tHeader, tBookmarks, tXfer ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit bookmarks" );
	}

	// unit id
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer ID UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	UNITID idUnit = INVALID_ID;
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_UNITID ))
	{				
		idUnit = pUnit ? UnitGetId( pUnit ) : INVALID_ID;
		XFER_UNITID_GOTO( tXfer, idUnit, UFX_ERROR );	
	}		
	
	// genus
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer GENUS UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());
	GENUS eGenus = pUnit ? UnitGetGenus( pUnit ) : GENUS_NONE;
	XFER_ENUM_GOTO( tXfer, eGenus, STREAM_BITS_TYPE, UFX_ERROR );
	
	// code
	DWORD dwCode = pUnit ? UnitGetClassCode( pUnit ) : INVALID_CODE;
	if (tXfer.IsSave())
	{
		ASSERT( pUnit );
		ASSERTV( dwCode != INVALID_CODE, "Unit '%s' has an invalid code", UnitGetDevName( pUnit ));
	}
	XFER_DWORD_GOTO( tXfer, dwCode, STREAM_BITS_CLASS_CODE, UFX_ERROR );
	spSpecies = sUnitGetSpeciesFromCode( eGenus, dwCode );
	int nClass = GET_SPECIES_CLASS( spSpecies );	

	// GUID
	if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_GUID))
	{
		if (tHeader.nVersion > USV_GUID_NOT_NEAR_UNIT_ID)
		{
			TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer GUID UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());		
			guidUnit = pUnit ? UnitGetGuid( pUnit ) : INVALID_GUID;
			if (UnitFileGUIDXfer( tXfer, pGame, tHeader, tSpec, &guidUnit, pUnit ) == FALSE)
			{
				ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer guid" );
			}
			if (tXfer.IsLoad())
			{
			
				// must have a GUID
				ASSERTX_GOTO( guidUnit != INVALID_GUID, "Invalid GUID encounted on load", UFX_ERROR );

				UNIT* pPreExistingUnit = UnitGetByGuid( pGame, guidUnit );
				// what if this unit is slated for deletion? We should wipe it out in favor
				// of the new one! could be a delayed free.
				if( pPreExistingUnit && pUnit != pPreExistingUnit )
				{
					if( UnitTestFlag( pPreExistingUnit, UNITFLAG_DELAYED_FREE ) ||
						UnitHasState( pGame, pPreExistingUnit, STATE_FADE_AND_FREE_CLIENT ) )
					{
						UnitFree(pPreExistingUnit, UFF_TRUE_FREE);
					}
				}
				// cannot collide with any other existing GUID			
				ASSERTV_GOTO(
					UnitGetByGuid( pGame, guidUnit ) == NULL || UnitGetByGuid( pGame, guidUnit ) == pUnit, 
					UFX_ERROR, 
					"Attempting to load Unit [%s] GUID [%I64x], but GUID is already set to unit [%s]", 
					UnitGetDevNameBySpecies( spSpecies ), 
					guidUnit,
					UnitGetDevName( UnitGetByGuid( pGame, guidUnit ) ));			
			}		
		}
	}
		
	// position
	if (UnitPositionXfer( pGame, pUnit, tXfer, tHeader, tSpec, tUnitPosition, pDBUnit ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit position" );	
	}

	// we must have a position type
	if (UnitFileHasPosition( tHeader ))
	{
		ASSERTV_GOTO( 
			tUnitPosition.eType == POSITION_TYPE_WORLD ||
			tUnitPosition.eType == POSITION_TYPE_INVENTORY,
			UFX_ERROR,
			"Invalid unit position type in xfer '%d' for unit:%s dbunit:%s",
			tUnitPosition.eType,
			pUnit ? UnitGetDevName( pUnit ) : "UNKNOWN",
			pDBUnit ? UnitGetDevNameBySpecies( pDBUnit->spSpecies ) : "UNKNOWN");
	}
	
	// team information
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer TEAM UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	int nTeam = INVALID_TEAM;
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_TEAM ))
	{
		if (TeamXfer( pGame, tXfer, pUnit, eGenus, nClass, nTeam ) == FALSE)
		{
			ASSERTV_GOTO( 
				0, 
				UFX_ERROR, 
				"Unable to xfer unit team (%s) Team=%d", 
				UnitGetDevName( pUnit ), 
				UnitGetTeam( pUnit ));	
		}
	}

	// look groups
	int nLookGroup = pUnit ? UnitGetLookGroup( pUnit ) : INVALID_LINK;
	if (TESTBIT(tHeader.dwSaveFlags, UNITSAVE_BIT_LOOKGROUP))
	{
		const ITEM_LOOK_GROUP_DATA *pLookGroupData = (nLookGroup != INVALID_LINK) ? LookGroupGetData( pGame, nLookGroup ) : NULL;
		DWORD dwLookGroupCode = pLookGroupData ? pLookGroupData->bCode : 0;
		XFER_DWORD_GOTO( tXfer, dwLookGroupCode, STREAM_BITS_LOOK_GROUP_CODE_BITS, UFX_ERROR );
		nLookGroup = ExcelGetLineByCode( NULL, DATATABLE_ITEM_LOOK_GROUPS, dwLookGroupCode );
	}

	// appearance shapes
	APPEARANCE_SHAPE tAppearanceShape;
	tAppearanceShape.bHeight = 125;		// defaults
	tAppearanceShape.bWeight = 125;		// defaults
	if (pUnit)
	{
		if (pUnit->pAppearanceShape)
		{
			tAppearanceShape = *pUnit->pAppearanceShape;
		}
	}
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_APPEARANCE_SHAPE ))
	{
		unsigned int nHeight = tAppearanceShape.bHeight;
		unsigned int nWeight = tAppearanceShape.bWeight;
		XFER_UINT_GOTO( tXfer, nHeight, STREAM_BITS_APPEARANCE_SHAPE_BITS, UFX_ERROR );
		XFER_UINT_GOTO( tXfer, nWeight, STREAM_BITS_APPEARANCE_SHAPE_BITS, UFX_ERROR );		
		tAppearanceShape.bHeight = (BYTE)nHeight;
		tAppearanceShape.bWeight = (BYTE)nWeight;
	}

	// we are now ready to create the unit when loading
	if (tXfer.IsLoad() && pUnit == NULL)
	{
		if ( TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_IS_CONTROL_UNIT ) )
			tSpec.bIsControlUnit = TRUE;

		// when loading, the server is currently often telling us about units more than once,
		// which it should totally *not* do, but until we can get to the root of the problem,
		// we say that if the client already knows about a unit with this id to forget it 
		// since it won't create anyway
		if (idUnit != INVALID_ID)
		{
			pUnit = UnitGetById( pGame, idUnit );
			if (pUnit)
			{
				
				// what if this unit is slated for deletion? We should wipe it out in favor
				// of the new one! could be a delayed free.

				if( UnitTestFlag( pUnit, UNITFLAG_DELAYED_FREE ) ||
					UnitHasState( pGame, pUnit, STATE_FADE_AND_FREE_CLIENT ) )
				{
					UnitFree(pUnit, UFF_TRUE_FREE);
					pUnit = NULL;
				}
				else
				{
					// do *not* destroy this unit
					bDestroyUnitOnError = FALSE;
					goto UFX_ERROR;
				}

			}
		}

		pUnit = sCreateUnit(pGame, idUnit, guidUnit, eGenus, nClass, tSpec.idClient, 
					nTeam, tUnitPosition.tWorldPos, tAppearanceShape, tUnitPosition, nLookGroup, 
					tSpec.bIsInInventory, tSpec.bIsControlUnit );
	}
	ASSERTV_GOTO( 
		pUnit, 
		UFX_ERROR, 
		"Xfer expected unit.  GUID:%I64d, genus:%d (%s), class:%d, classcode:%d, inv:%d",
		guidUnit,
		eGenus,
		GenusGetString( eGenus ),
		nClass,
		dwCode,
		tSpec.bIsInInventory);
	
	ASSERTV_GOTO( UnitGetGame( pUnit ) == pGame, UFX_ERROR, "Unit not created in proper game" );

	// do read single bookmark sonly logic
	if (tXfer.IsLoad() && tSpec.eOnlyBookmark != UB_INVALID)
	{
	
		// set the buffer to the bookmark position
		if (tXfer.SetCursor( tBookmarks.nCursorPos[ tSpec.eOnlyBookmark ] ) == XFER_RESULT_ERROR)
		{
			ASSERTV_GOTO( 0, UFX_ERROR, "Unable to set cursor for eOnlyBookmark=%d", tSpec.eOnlyBookmark );
		}
		
		// do the appropriate xfer
		switch (tSpec.eOnlyBookmark)
		{
			case UB_HOTKEYS:
			{
				if (HotkeysXfer( pUnit, tXfer, tHeader, tBookmarks, FALSE ) == FALSE)
				{
					ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer hotkeys" );
				}
				return pUnit;
				
			}
			
		}
		
	}

	// guid (old verison)
	if (sGUIDXferOld( tXfer, pGame, tHeader, tSpec, &guidUnit, pUnit ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Error loading old GUID format" );
	}
	
	// unit name
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer NAME UNIT GUID:%I64d  CUR:%d", guidUnit, tXfer.GetBuffer()->GetCursor());	
	if (sUnitNameXfer( pUnit, tXfer, tHeader ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit name (%s)", UnitGetDevName( pUnit ) );
	}
	
	// xfer saved state ids for things like states that affixes attach to weapons/armor
	if (sUnitSavedStatesXfer( pUnit, tXfer, tHeader ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit saved states (%s)", UnitGetDevName( pUnit ) );	
	}

	// interactive info
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer INTERACTIVE UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	if (sUnitInteractInfoXfer( pUnit, tXfer, tSpec.idClient, tHeader ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit interact info (%s)", UnitGetDevName( pUnit ) );	
	}

	// blocking info
	if (sUnitBlockingXfer( pUnit, tXfer, tSpec.idClient, tHeader ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit blocking info (%s)", UnitGetDevName( pUnit ) );	
	}
	
	// when saving quests and tasks, transfer the state of those system into stats on the player
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer QUESTS UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	if (tXfer.IsSave())
	{
		if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_QUESTS_AND_TASKS ))
		{
			if (sPushQuestsToPlayerStats( pUnit ) == FALSE)
			{
				ASSERTV_GOTO( 0, UFX_ERROR, "Unable to push quests to player stats for (%s)", UnitGetDevName( pUnit ) );
			}
		}
	}

	// xfer dead	
	BOOL bIsDead = IsUnitDeadOrDying( pUnit );
	XFER_BOOL_GOTO( tXfer, bIsDead, UFX_ERROR );
	if (bIsDead)
	{
		UnitSetFlag( pUnit, UNITFLAG_DEAD_OR_DYING );	
	}

	// stats
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer STATS UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	UnitSetFlag( pUnit, UNITFLAG_IGNORE_STAT_MAX_FOR_XFER);	
	if (sUnitStatsXfer( pUnit, tXfer, tHeader, pDBUnit ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unable to xfer unit stats (%s)", UnitGetDevName( pUnit ) );	
	}
	UnitClearFlag( pUnit, UNITFLAG_IGNORE_STAT_MAX_FOR_XFER);	

	// if we're in an inventory, place me ... note that it's possible to get a unit
	// who is in an inventory but for whom we do not have the containing unit (like
	// a maggot spawn who is contained by a taint monster, and we dont' know about 
	// the taint) TODO: change this, we should know about entire inventory chains
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer INVENTORY UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	if (tXfer.IsLoad() && tUnitPosition.eType == POSITION_TYPE_INVENTORY)
	{
		INVENTORY_POSITION &tInvPos = tUnitPosition.tInvPos;
		UNIT *pContainer = tInvPos.pContainer;  // use container written in file if any
		if (pContainer == NULL && tSpec.bIsInInventory)
		{
			// no container in file, use the spec we're passing in
			pContainer = tSpec.pContainer; 
		}
		if (pContainer)
		{
			// resolve the inventory location for this unit if needed
			if (UnitFileIsForDisk( tHeader ))
			{			
				if (UnitFileResolveInventoryLocationCode( pContainer, tInvPos.dwInvLocationCode, &tInvPos.tInvLoc.nInvLocation ) == FALSE)
				{
					ASSERTV_GOTO( 
						0, 
						UFX_ERROR, 
						"Unable to resolve inventory location (code=%d, idContainer=%d, nInvLoc=%d, nX=%d, nY=%d) for unit (%s) container (%s)", 
						tInvPos.dwInvLocationCode,
						tInvPos.idContainer,
						tInvPos.tInvLoc.nInvLocation,
						tInvPos.tInvLoc.nX,
						tInvPos.tInvLoc.nY,
						UnitGetDevName( pUnit ),
						UnitGetDevName( pContainer ));
				}
			}
						
			// link to inventory, note that we're allowing placement into a fallback inventory
			// slot when we are loading a player from the DB.  The reason being is that we
			// are having some items be stuck in reward slots, and rather than delete these
			// items, we want to put them back into the offering storage slot.  Plus
			// this is also a good way to allow units that, for any reason, get doubled
			// up in a single inventory slot or any other kind of error, to get placed
			// somewhere that is recoverable by the player or by a CS rep
			BOOL bAllowFallback = FALSE;
			if (tXfer.IsLoad() && pDBUnit != NULL && tSpec.eSaveMode == UNITSAVEMODE_DATABASE)
			{
				bAllowFallback = TRUE;
			}
			
			// add to inventory
			InventoryLinkUnitToInventory(
				pUnit, 
				pContainer, 
				tInvPos.tInvLoc, 
				tInvPos.tiEquipTime, 
				bAllowFallback,
				tSpec.dwReadUnitFlags);
			
		}
		
	}

	// wardrobe
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer WARDROBE UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	int nWardrobe = UnitGetWardrobe( pUnit );	
	if (WardrobeXfer( pUnit, nWardrobe, tXfer, tHeader, FALSE, NULL ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unabled to xfer wardrobe for (%s)", UnitGetDevName( pUnit ));
	}

	// clear just created flag when reading on clients
	if (tXfer.IsLoad() && IS_CLIENT( pGame ))
	{
		UnitClearFlag( pUnit, UNITFLAG_JUSTCREATED );
	}

	// inventory	
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INVENTORY ))
	{

		// should we skip the inv data	
		BOOL bSkipInventory = TESTBIT( tSpec.dwReadUnitFlags, RUF_IGNORE_INVENTORY_BIT );
				
		UnitSetFlag(pUnit, UNITFLAG_INVENTORY_LOADING);
		// xfer inventory
		if (InventoryXfer( 
				pUnit, 
				tSpec.idClient, 
				tXfer, 
				tHeader, 
				bSkipInventory, 
				&tSpec.nUnitsXfered) == FALSE)
		{
			ASSERTV_GOTO( 0, UFX_ERROR, "Unabled to xfer invnentory for (%s)", UnitGetDevName( pUnit ));
		}
		
		// do wardrobe update if we read inventory information
		if (tXfer.IsLoad() && bSkipInventory == FALSE)
		{
			s_InventorysVerifyEquipped( pGame, pUnit );		// now that it's all loaded check to make sure the equipped stuff can all be equipped
			WardrobeUpdateFromInventory( pUnit );
		}

		UnitClearFlag(pUnit, UNITFLAG_INVENTORY_LOADING);

	}

	// hot keys
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UnitFileXfer HOTKEYS UNIT:%p  CUR:%d", pUnit, tXfer.GetBuffer()->GetCursor());	
	BOOL bSkipData = TESTBIT( tSpec.dwReadUnitFlags, RUF_IGNORE_HOTKEYS_BIT );	
	if (HotkeysXfer( pUnit, tXfer, tHeader, tBookmarks, bSkipData ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Unabled to xfer hotkeys for (%s)", UnitGetDevName( pUnit ));	
	}
	
	// magic value as last thing in file
	DWORD dwLastMagic = UNITSAVE_MAGIC_NUMBER;
	XFER_DWORD_GOTO( tXfer, dwLastMagic, STREAM_MAGIC_NUMBER_BITS, UFX_ERROR );
	ASSERTV_GOTO( dwLastMagic == UNITSAVE_MAGIC_NUMBER, UFX_ERROR, "Save/Load Error, please send character file.  Expected magic number at end of unit buffer" );

	// go back and do CRC logic (we only can do this for the first unit in the file
	// where we're sure the position of the buffer is byte aligned for CRC)
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_CRC ) && tXfer.IsSave())
	{
		
		// save the final cursor position
		unsigned int nCursorFinal = tXfer.GetCursor();
		
		// how much data is at the start of the buffer before the location that will start the data CRC on
		ASSERTV_GOTO( tHeader.dwCursorCRCDataStart > 0, UFX_ERROR, "Save/Load Error, please send character file.  Expected cursor for start of CRC data" );
		tXfer.SetCursor( tHeader.dwCursorCRCDataStart );
		unsigned int nNumBytesBeforeDataToCRC = tXfer.GetLen();

		// how many bytes is the data we will CRC
		tXfer.SetCursor( nCursorFinal );
		unsigned int nLength = tXfer.GetLen();
		tHeader.dwNumBytesToCRC = nLength - nNumBytesBeforeDataToCRC;
		
		// compute the CRC on the data
		tXfer.SetCursor( tHeader.dwCursorCRCDataStart );
		BYTE *pDataToCRC = tXfer.GetBufferPointer();
		ASSERTV_GOTO( pDataToCRC, UFX_ERROR, "Save/Load Error, please send character file.  Cannot get pointer to unit file buffer to CRC" );
		tHeader.dwCRC = tSpec.dwCRC = CRC( 0, pDataToCRC, tHeader.dwNumBytesToCRC );
		
		// save the CRC and the # of bytes used to CRC in in the buffer
		tXfer.SetCursor( tHeader.dwCursorCRC );
		if (UnitFileCRCXfer( tXfer, tHeader ) == FALSE)
		{
			ASSERTV_GOTO( 0, UFX_ERROR, "Unabled to xfer unit file CRC for (%s)", UnitGetDevName( pUnit ));		
		}
		// set cursor back to the end just to be clean
		tXfer.SetCursor( nCursorFinal );

	}

	// get next cursor pos
	dwCursorNext = tXfer.GetCursor();
	
	// block size logic
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INCLUDE_BLOCK_SIZES ))
	{
	
		// we're loading, verify that the number of bits we read is the size we have recorded in the header
		DWORD dwTotalSizeInBits = dwCursorNext - dwCursorStart;
		if (tXfer.IsLoad())
		{
			ASSERTV_GOTO( 
				tHeader.dwTotalSizeInBits == dwTotalSizeInBits, 
				UFX_ERROR, 
				"Total size in bits mismatch (%s), %d should be %d",
				pUnit ? UnitGetDevName( pUnit ) : "Unknown",
				dwTotalSizeInBits,
				tHeader.dwTotalSizeInBits);
		}
		else
		{
		
			// go back and write the total size in the header and stream
			if (tXfer.SetCursor( tHeader.dwCursorPosOfTotalSize ) == XFER_RESULT_ERROR)
			{
				ASSERTV_GOTO( 
					0, 
					UFX_ERROR, 
					"Unable to set cursor to tHeader.dwCursorPosOfTotalSize (%d) for (%s)",
					tHeader.dwCursorPosOfTotalSize,
					UnitGetDevName( pUnit ));
			}
			tHeader.dwTotalSizeInBits = dwTotalSizeInBits;
			XFER_DWORD_GOTO( tXfer, tHeader.dwTotalSizeInBits, STREAM_BITS_TOTAL_SIZE_BITS, UFX_ERROR );
			
			// go back to the next block of data
			tXfer.SetCursor( dwCursorNext );
			
		}

	}
			
	// OK, unit has been totally xfer'd ... do any post process
	if (sUnitPostProcessXfer( pUnit, tXfer, tHeader, tSpec ) == FALSE)
	{
		ASSERTV_GOTO( 0, UFX_ERROR, "Failed unit xfer post process for (%s)", UnitGetDevName( pUnit ));	
	}

	// record we've done another unit now
	tSpec.nUnitsXfered++;
	
	return pUnit;
	
UFX_ERROR:
		
	// when saving, make this a no-op by backing the cursor to the start position, when loading
	// skip to the next block of data
	if (tXfer.IsSave())
	{
		ASSERTV( 
			0, 
			"Error saving '%s', skipping unit and rewinding data buffer to '%d'",
			UnitGetDevName( pUnit ),
			dwCursorStart);
		tXfer.SetCursor( dwCursorStart );
	}
	else
	{
	
		// we can only fast forward if this save file has the block sizes recorded within it
		if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INCLUDE_BLOCK_SIZES ))
		{
			DWORD dwNextData = dwCursorStart + tHeader.dwTotalSizeInBits;
			
			ASSERTV(
				0,
				"Error loading '%s', skipping data buffer forward to '%d'",
				spSpecies != SPECIES_NONE ? UnitGetDevNameBySpecies( spSpecies ) : "Unknown",
				dwNextData);
			tXfer.SetCursor( dwNextData );
		}
		
		// destroy unit if we have it and we're allowed to
		if (pUnit != NULL && bDestroyUnitOnError == TRUE)
		{
			UnitFree( pUnit );
			pUnit = NULL;
		}
		
	}

	return NULL;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitFileValidateClass(
	UNIT *pPlayer,
	GENUS eGenus,
	int nClass)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitFileValidateAll(
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );
	int nNumErrors = 0;
	
	//// go through all items
	//int nNumItems = ExcelGetNumRows( pGame, DATATABLE_ITEMS );
	//for (int i = 0; i < nNumItems; ++i)
	//{
	//	if (sUnitFileValidateClass( pPlayer, GENUS_ITEM, i ) == FALSE)
	//	{
	//		nNumErrors++;
	//	}
	//}

	//// validate monsters (only pets that we can save need validation)
	//
	//// validate players
	//int nNumPlayers = ExcelGetNumRows( pGame, DATATABLE_PLAYERS );
	//for (int i = 0; i < nNumPlayers; ++i)
	//{
	//	if (sUnitFileValidateClass( pPlayer, GENUS_PLAYER, i ) == FALSE)
	//	{
	//		nNumErrors++;
	//	}
	//}
		
	return nNumErrors;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const START_LOCATION *UnitFileHeaderGetStartLocation( 
	const UNIT_FILE_HEADER &tHeader,
	int nDifficulty)
{
	ASSERTX_RETNULL( nDifficulty != INVALID_LINK, "Expected difficulty link" );
	const START_LOCATION *pStartLocation = NULL;
	
	// search each starting location for a matching difficulty
	for (int i = 0; i < UF_MAX_START_LOCATIONS; ++i)
	{
		const START_LOCATION *pLocation = &tHeader.tStartLocation[ i ];
		if (pLocation->nDifficulty == nDifficulty)
		{
		
			// we should only have one match in the unit file header for this difficulty
			ASSERTX( pStartLocation == NULL, "Multiple start locations for a single difficulty found" );
			pStartLocation = pLocation;
			
		}
		
	}

	return pStartLocation;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFileIsRemovedOnLoad(
	GAME *pGame,
	GENUS eGenus,
	int nClass)
{
	const UNIT_DATA *pUnitData = UnitGetData( pGame, eGenus, nClass );
	if (pUnitData)
	{
		// ignore units that cannot be created in the current game context
		if (UnitCanBeCreated( pGame, pUnitData ) == FALSE)
		{
			return FALSE;
		}
	}
	
	return FALSE;
	
}
