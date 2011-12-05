//----------------------------------------------------------------------------
// FILE: unitfile.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef __UNITFILE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __UNITFILE_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "bookmarks.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
//#define TRACE_SYNCH

#define PLAYERSAVE_MAGIC_NUMBER				0x67616c46
#define PLAYERSAVE_HOTKEYS_MAGIC			0x91103a74
#define UNITSAVE_MAGIC_NUMBER				0x2b523460


// this is a hard limit for any given single unit
#include "unitfile_maxsize.h"

// these numbers help us to judge how big a save file should be given a
// player unit and a number of inventory units
#define UNITFILE_MAX_SIZE_PLAYER_UNIT			UNITFILE_MAX_SIZE_SINGLE_UNIT
#define UNITFILE_AVERAGE_SIZE_INVENTORY_UNIT	512  // a generous guess based on what we've seen on far
#define UNITFILE_MAX_NUMBER_OF_UNITS			1536

// our max unit file size with all units would technically be the absolute hard max
// limit * the number of allowed units, but this is incredibly wasteful since most units
// are inventory units, and they are very small.  So we'll use a guess of what
// the average size of an inventory unit is, how many of them we can have, and the 
// max size of the player unit to arrive at a "reasonable" guess for what the 
// max limit on a unit save file that contains all of a players inventory is
#define UNITFILE_MAX_SIZE_ALL_UNITS				(UNITFILE_MAX_SIZE_PLAYER_UNIT + (UNITFILE_AVERAGE_SIZE_INVENTORY_UNIT * (UNITFILE_MAX_NUMBER_OF_UNITS - 1)))
#define UNITFILE_MIN_SIZE_ALL_UNITS				32


//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum STATS_WRITE_METHOD;
struct UNIT;
struct CLIENT_SAVE_FILE;
struct GAME;
enum UNIT_BOOKMARK;
struct DATABASE_UNIT;
void UnitFileHeaderInit( struct UNIT_FILE_HEADER &tHeader );

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum UNITSAVEBITS
{

	// ***********************************************************************
	// !!! DO NOT CHANGE THE VALUES OF the existing ENUMS !!!
	// !!! DO NOT REMOVE ENUM VALUES FOR RE-USE !!!
	// !!! These these values are written as bit flags into character save
	// !!! files and can not be altered or you will invalidate all the save games out there
	// *** ADD ONLY TO THE BOTTOM OF THIS LIST ***
	// ***********************************************************************	
	UNITSAVE_BIT_HEADER						= 0,
	UNITSAVE_BIT_POSITION					= 1,
	UNITSAVE_BIT_CONTAINER_ID				= 2,
	UNITSAVE_BIT_INVENTORY_LOCATION			= 3,
	UNITSAVE_BIT_VELOCITY_BASE				= 4,
	UNITSAVE_BIT_UNITID						= 5,
	UNITSAVE_BIT_TEAM						= 6,
	UNITSAVE_BIT_APPEARANCE_SHAPE			= 7,
	UNITSAVE_BIT_NAME						= 8,
	UNITSAVE_BIT_LOOKGROUP					= 9,
	UNITSAVE_BIT_SAVESTATES					= 10,
	UNITSAVE_BIT_INTERACT_INFO				= 11,
	UNITSAVE_BIT_QUESTS_AND_TASKS			= 12,
	UNITSAVE_BIT_STATS						= 13,
	UNITSAVE_BIT_WARDROBE					= 14,
	UNITSAVE_BIT_WARDROBE_WEAPON_IDS		= 15,
	UNITSAVE_BIT_WARDROBE_LAYERS			= 16,
	UNITSAVE_BIT_WARDROBE_LAYERS_IN_BODY	= 17,
	UNITSAVE_BIT_INVENTORY					= 18,
	UNITSAVE_BIT_ITEMS_IGNORE_NOSAVE		= 19,
	UNITSAVE_BIT_UNIT_LEVEL					= 20,
	UNITSAVE_BIT_UNITID_ALIASING			= 21,
	UNITSAVE_BIT_COLORSET_ITEM_GUID 		= 22,
	UNITSAVE_BIT_GUID						= 23,
	UNITSAVE_BIT_WARDROBE_WEAPON_AFFIXES	= 24,	// maybe move this with other wardrobe bits, but don't know if order matters ... too close to g-star to guess -Colin
	UNITSAVE_BIT_TAGS						= 25,
	UNITSAVE_BIT_HOTKEYS					= 26,
	UNITSAVE_BIT_BOOKMARKS					= 27,
	UNITSAVE_BIT_CRC						= 28,
	UNITSAVE_BIT_INCLUDE_BLOCK_SIZES		= 29,	// include size of each unit data block in the data so we can skip it if necessary in an error state
	UNITSAVE_BIT_MAX_DIFFICULTY				= 30,	// this is usually stored in a stat, but 
	UNITSAVE_BIT_START_LOCATION				= 31,	// start level def or level area def
	UNITSAVE_BIT_SAVESTATES_IN_HEADER		= 32,	// put save states in the header that can go there
	UNITSAVE_BIT_IS_CONTROL_UNIT			= 33,
	// ***********************************************************************
	// !!! DO NOT CHANGE THE VALUES OF the existing ENUMS !!!
	// !!! DO NOT REMOVE ENUM VALUES FOR RE-USE !!!
	// !!! These these values are written as bit flags into character save
	// !!! files and can not be altered or you will invalidate all the save games out there
	// *** ADD ONLY TO THE BOTTOM OF THIS LIST ***
	// ***********************************************************************	
	
	NUM_UNITSAVE_BITS,
	
};

//----------------------------------------------------------------------------
enum UNITFILE_CONSTANTS
{
	UF_MAX_START_LOCATIONS = 3,				// arbitrary, but make sure old characters can load with new code
	NUM_UNIT_SAVE_FLAG_BLOCKS = DWORD_FLAG_SIZE( NUM_UNITSAVE_BITS ),
	UNIT_FILE_HEADER_MAX_SAVE_STATES = 8,	// arbitrary, but want to keep small
	UNIT_FILE_MAX_GUID_LIST = 2048,
};

//----------------------------------------------------------------------------
enum UNITSAVEMODE
{
	UNITSAVEMODE_INVALID = -1,
	
	UNITSAVEMODE_FILE,
	UNITSAVEMODE_DATABASE,
	UNITSAVEMODE_DATABASE_FOR_SELECTION,
	UNITSAVEMODE_SENDTOSELF,
	UNITSAVEMODE_SENDTOOTHER,
	UNITSAVEMODE_SENDTOOTHER_NOID,
	UNITSAVEMODE_CLIENT_ONLY,

	NUM_UNITSAVEMODES,	

	UNITSAVEMODE_UNKNOWN,
	UNITSAVEMODE_PAK,
};

//----------------------------------------------------------------------------
enum UNIT_BUFFER_KNOWN_SIZES
{
	STREAM_CRC_BITS =						32,
	STREAM_NUM_BYTES_TO_CRC_BITS =			32,
	STREAM_BITS_FLOAT =						32,
	STREAM_BITS_TOTAL_SIZE_BITS =			32,

	STREAM_STAT_WRITE_METHOD_BITS =			3,	
	STREAM_STAT_ENTRY_BITS =				16,	// number of stats
	STREAM_STAT_RIDER_BITS =				6,	// number of riders
	STREAM_STAT_SELECTOR_BITS =				4,	// number of bits for selector

	STREAM_MAX_INVENTORY_BITS =				10,
	STREAM_MAX_INVLOC_BITS =				12,
	STREAM_MAX_INVLOC_CODE_BITS =			16,	
	STREAM_MAX_INVPOS_X_BITS =				12,	// Mythos needs these bigger than Hellgate!
	STREAM_MAX_INVPOS_Y_BITS =				12,
	STREAM_MAX_HOTKEY_BITS =				6,

	STREAM_BITS_TIME =						64,
	STREAM_BITS_UNITID =					32,
	STREAM_BITS_ROOMID =					32,
	STREAM_BITS_TYPE =						4,
	STREAM_BITS_CLASS_CODE =				16,
	STREAM_BITS_ANGLE =						10,
	STREAM_BITS_MODLIST_COUNT =				8,
	STREAM_BITS_STATS_SELECTOR =			5,
	STREAM_BITS_STATS_SELECTOR_CODE =		16,
	STREAM_BITS_STATSLIST_ID =				32,
	STREAM_BITS_INTERACT_INFO =				4,
	STREAM_BITS_BLOCKING_WARP =				5,
	STREAM_BITS_IS_DEAD	=					1,
	STREAM_BITS_GAMEID =					64,
	STREAM_BITS_POSITION_TYPE_BITS =		1,
	STREAM_BITS_LOOK_GROUP_CODE_BITS =		8,
	STREAM_BITS_APPEARANCE_SHAPE_BITS =		8,
	STREAM_BITS_GUID_HIGH =					32,
	STREAM_BITS_GUID_LOW =					32,
	STREAM_BITS_NAME =						8,
	STREAM_BITS_SAVED_STATE_COUNT =			8,
	STREAM_BITS_STATE_CODE =				16,
	STREAM_BITS_STATE_DURATION =				32,
	STREAM_BITS_EXPERIENCE_LEVEL =			8,
	STREAM_BITS_EXPERIENCE_RANK =			8,
	STREAM_BITS_MAX_DIFFICULTY =			3,
	STREAM_BITS_NUM_WARDROBE_WEAPONS =		3,
	STREAM_BITS_HOTKEY_CODE_BITS =			16,
	STREAM_BITS_HOTKEY_NUM_ITEMS =			4,
	STREAM_BITS_NUM_HOTKEY_SKILLS =			4,
	STREAM_BITS_SKILL_CODE =				32,
	STREAM_BITS_UNIT_ALIAS =				32,
	STREAM_BITS_UNITTYPE_CODE =				32,
	STREAM_BITS_BOOKMARK_CODE =				16,
	STREAM_BITS_WCHAR =						16,
	STREAM_BITS_SAVE_MODE =					8,
	STREAM_BITS_EXCEL_TABLE_CODE =			16,
	STREAM_BITS_DIFFICULTY_CODE =			16,

	STREAM_BITS_SIMILAR_STAT_COUNT_8BIT		 = 8,	// never ever change
	STREAM_BITS_SIMILAR_STAT_COUNT_10BIT	 = 10,	
	STREAM_BITS_STAT_PARAM_COUNT			 = 2,
	STREAM_BITS_STAT_PARAM_VALUE_TYPE		 = 1,
	STREAM_BITS_STAT_PARAM_NUM_BITS_FILE	 = 6,
	STREAM_BITS_STAT_PARAM_OPTIONAL_ELEMENTS = 2,
	STREAM_BITS_STAT_PARAM_NUM_BITS_SHIFT	 = 3,
	STREAM_BITS_STAT_PARAM_NUM_BITS_OFFSET	 = 1,
	STREAM_BITS_STAT_OPTIONAL_ELEMENTS		 = 3,
	STREAM_BITS_STAT_VALUE_TYPE				 = 2,
	STREAM_BITS_STAT_VALUE_NUM_BITS_FILE	 = 6,
	STREAM_BITS_STAT_VALUE_NUM_BITS_SHIFT	 = 4,
	STREAM_BITS_STAT_VALUE_NUM_BITS_OFFSET	 = 12,
	STREAM_BITS_STAT_NUM_BITS_OFFSET		 = 1,	
	STREAM_BITS_STAT_CODE					 = 16,
		
	STREAM_CURSOR_SIZE_BITS =				32,
	STREAM_BOOKMARK_COUNT_BITS =			5,
	
	STREAM_MAGIC_NUMBER_BITS =				32,
	STREAM_VERSION_BITS =					16,  // this can never be changed without wiping characters
	STREAM_HAS_HEADER_BITS =				8,
	STREAM_NUM_SAVED_STATES_BITS =			8,
	
	STREAM_LEVEL_CODE_BITS =				16,
	STREAM_NUM_STARTING_LOCATION_BITS =		4,
	STREAM_NUM_FLAG_BLOCKS_BITS =			8,  // must be byte aligned
	STREAM_SAVE_FLAG_BLOCK_BITS =			32,	// a full DWORD of bits, must be byte aligned
	
};

//----------------------------------------------------------------------------
enum READ_UNIT_FLAGS
{
	RUF_IGNORE_INVENTORY_BIT,			// skip inventory in data
	RUF_IGNORE_HOTKEYS_BIT,				// skip hotkeys in data
	RUF_NO_DATABASE_OPERATIONS,			// do not do any database operations with unit
	RUF_USE_DBUNIT_GUID_BIT,			// ignore the GUID in the file and use the GUID in the dbunit
	RUF_RESTORED_ITEM_BIT,				// unit is a restored item from CSR
};

//----------------------------------------------------------------------------
struct BOOKMARK_DEFINITION
{
	char szName[DEFAULT_INDEX_SIZE];
	WORD wCode;
};

//----------------------------------------------------------------------------
template <XFER_MODE mode>
struct UNIT_FILE_XFER_SPEC
{

	XFER<mode> 				tXfer;					// the xfer object to use, it's important that 
													//this not be a reference or pointer so that we 
													//can truly copy it and change the copy without 
													// affecting the original
	
	UNIT *					pUnitExisting;			// for reading data onto an existing unit (if any)
	GAMECLIENTID			idClient;				// when reading client units (ie. players)
	struct ROOM *			pRoom;					// room unit is in (if any)
	UNIT *					pContainer;				// who contains this unit (if any)
	DWORD					dwReadUnitFlags;		// see READ_UNIT_FLAGS
	UNIT_BOOKMARK			eOnlyBookmark;			// read only this bookmark block of data
	UNITSAVEMODE			eSaveMode;				// savemode the unit file is for
	DWORD					dwCRC;					// the CRC of the unit file (if any)
	BOOL					bIsInInventory;			// unit we are reading/writing is contained in an inventory	
	BOOL					bIsControlUnit;			// unit we are reading/writing is the control unit
	int						nUnitsXfered;			// num units xfered (for debugging)
	DATABASE_UNIT *			pDBUnit;				// the db unit this xfer is for (if any)
	
	UNIT_FILE_XFER_SPEC(
		XFER<mode> & tXferObject) :	
			tXfer( tXferObject ),
			pUnitExisting( NULL ),
			idClient( INVALID_GAMECLIENTID ),
			pRoom( NULL ),
			pContainer( NULL ),
			dwReadUnitFlags( 0 ),
			eOnlyBookmark( UB_INVALID ),
			eSaveMode( UNITSAVEMODE_INVALID ),
			bIsInInventory( FALSE ),
			bIsControlUnit( FALSE ),
			nUnitsXfered( 0 ),
			pDBUnit( NULL )
	{}
};

//----------------------------------------------------------------------------
struct START_LOCATION
{
	int nDifficulty;					// which difficulty this start location is for
	int nLevelDef;						// level definition
	int nLevelAreaDef;					// level area definition
};

//----------------------------------------------------------------------------
struct UNIT_FILE_HEADER
{
	DWORD					dwMagicNumber;			// magic number of this unit file
	DWORD					dwTotalSizeInBits;		// total size of this unit written in bits (everything, header and all)
	DWORD					dwCursorPosOfTotalSize;	// position where dwTotalSizeInBits is located at
	DWORD					dwCursorCRC;			// the position of the CRC in the buffer
	DWORD					dwCRC;					// the CRC value itself
	DWORD					dwCursorCRCDataStart;	// where in the buffer we start computing the CRC from
	DWORD					dwNumBytesToCRC;		// # of bytes to CRC starting at nCursorCRCDataStart
	unsigned int			nVersion;				// the version of this entire unti file
	START_LOCATION			tStartLocation[ UF_MAX_START_LOCATIONS ];	// starting locations for this player at each difficulty level
	DWORD					dwSaveFlags[ NUM_UNIT_SAVE_FLAG_BLOCKS ];  // bit descriptors of what to find/write in file
	UNITSAVEMODE			eSaveMode;				// mode file is saved using
	int						nSavedStates[ UNIT_FILE_HEADER_MAX_SAVE_STATES ]; // saved stats that we want to have in the header (is a subset of saved states)
	DWORD					dwNumSavedStates;		// number of entries in nSavedStates[]
	
	UNIT_FILE_HEADER::UNIT_FILE_HEADER( void )
	{ 
		UnitFileHeaderInit( *this );
	}
	
};

//----------------------------------------------------------------------------
struct UNIT_FILE
{
	BYTE *pBuffer;
	DWORD dwBufferSize;
};

//----------------------------------------------------------------------------
enum UNIT_FILE_RESULT
{	
	UFR_OK,					// a-ok!
	UFR_ERROR,				// something went wrong
	UFR_REMOVE_ON_LOAD,		// a unit was successfully ignored and flagged for removal
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL UnitFileSetHeader(
	UNIT_FILE_HEADER &tHeader,
	UNITSAVEMODE eSaveMode,
	UNIT *pUnit,
	BOOL bUnitIsInInventory,
	BOOL bUnitIsControlUnit );

BOOL UnitFileCheckCRC(
	DWORD dwCRC,
	BYTE *pBuffer,
	DWORD dwBufferSize);

BOOL UnitFileHeaderCanBeLoaded(
	const UNIT_FILE_HEADER &tHeader,
	DWORD dwMagicNumber);

BOOL UnitFileReadHeader(
	BYTE *pBuffer,
	unsigned int nBufferSize,
	UNIT_FILE_HEADER &hdr);

BOOL UnitFileVersionCanBeLoaded(
	CLIENT_SAVE_FILE *pClientSaveFile);

BOOL UnitFileGetSaveDescriptors(
	UNIT * pUnit,
	UNITSAVEMODE eSavemode,
	DWORD *pdwFlag,		// UNITSAVEBITS
	STATS_WRITE_METHOD *peStatsWriteMethod);

BOOL UnitFileResolveInventoryLocationCode(
	UNIT *pContainer,
	DWORD dwInvLocationCode,
	int *pnInvLocation);

template <XFER_MODE mode>
UNIT * UnitFileXfer(
	GAME * game,
	UNIT_FILE_XFER_SPEC<mode> & tSpec);

template UNIT * UnitFileXfer<XFER_SAVE>(GAME * game, UNIT_FILE_XFER_SPEC<XFER_SAVE> & tSpec);
template UNIT * UnitFileXfer<XFER_LOAD>(GAME * game, UNIT_FILE_XFER_SPEC<XFER_LOAD> & tSpec);

template <XFER_MODE mode>
BOOL UnitFileCRCXfer(
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER & tHeader);
		
template <XFER_MODE mode>
BOOL UnitFileXferHeader(
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader);

int UnitFileValidateAll(
	UNIT *pPlayer);

const START_LOCATION *UnitFileHeaderGetStartLocation( 
	const UNIT_FILE_HEADER &tHeader,
	int nDifficulty);	

BOOL UnitFileIsRemovedOnLoad(
	GAME *pGame,
	GENUS eGenus,
	int nClass);
	
#endif
