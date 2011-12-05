//----------------------------------------------------------------------------
// inventory.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _INVENTORY_H_
#define _INVENTORY_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _PTIME_H_
#include "ptime.h"
#endif

#ifndef _APPCOMMON_H_
#include "appcommon.h"
#endif

#ifndef _DATATABLES_H_
#include "datatables.h"
#endif

#ifndef _EXCEL_H_
#include "excel.h"
#endif

//----------------------------------------------------------------------------
// DEBUG CONSTANTS & MACROS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define INVENTORY_DEBUG											1
#else
#define INVENTORY_DEBUG											0
#endif


#if INVENTORY_DEBUG
#define INV_FUNCNAME(f)											f##Dbg
#define INV_FUNCNOARGS()										const char * file = NULL, unsigned int line = 0
#define INV_FUNCARGS(...)										__VA_ARGS__, const char * file = NULL, unsigned int line = 0
#define INV_FUNCFLARGS(...)										const char * file, unsigned int line, __VA_ARGS__
#define INV_FUNCNOARGS_NODEFAULT()								const char * file, unsigned int line
#define INV_FUNCARGS_NODEFAULT(...)								__VA_ARGS__, const char * file, unsigned int line
#define INV_FUNC_FILELINE(...)									__VA_ARGS__, (const char *)__FILE__, (unsigned int)__LINE__
#define INV_FUNCNOARGS_PASSFL()									(const char *)(file), (unsigned int)(line)
#define INV_FUNC_PASSFL(...)									__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define INV_FUNC_FLPARAM(file, line, ...)						__VA_ARGS__, (const char *)(file), (unsigned int)(line)
#define INV_FUNC_FLARGS(file, line, ...)						file, line, __VA_ARGS__
#define INV_DEBUG_REF(x)										REF(x)
#define INV_NDEBUG_REF(x)									
#else
#define INV_FUNCNAME(f)											f
#define INV_FUNCNOARGS()										
#define INV_FUNCARGS(...)										__VA_ARGS__
#define INV_FUNCFLARGS(...)										__VA_ARGS__
#define INV_FUNCNOARGS_NODEFAULT()								
#define INV_FUNCARGS_NODEFAULT(...)								__VA_ARGS__
#define INV_FUNC_FILELINE(...)									__VA_ARGS__
#define INV_FUNCNOARGS_PASSFL()							
#define INV_FUNC_PASSFL(...)									__VA_ARGS__
#define INV_FUNC_FLPARAM(file, line, ...)						__VA_ARGS__
#define INV_FUNC_FLARGS(file, line, ...)						__VA_ARGS__
#define INV_DEBUG_REF(x)										
#define INV_NDEBUG_REF(x)										REF(x)
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
enum DROP_RESULT;
enum UNITSAVEMODE;
struct GAMECLIENT;
struct UNIT_FILE_HEADER;
struct INVLOC;
struct GAME;

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
enum
{
	INVLOC_UNITTYPES = 6,
	INVLOC_NUM_ALLOWTYPES_SKILLS = 3,
};

//----------------------------------------------------------------------------
enum INVLOC_FLAG
{
	INVLOCFLAG_DYNAMIC,
	INVLOCFLAG_ONESIZE,
	INVLOCFLAG_GRID,
	INVLOCFLAG_WARDROBE,
	INVLOCFLAG_WEAPON,
	INVLOCFLAG_AUTOPICKUP,
	INVLOCFLAG_PETSLOT,
	INVLOCFLAG_DONTSAVE,
	INVLOCFLAG_RESURRECTABLE,
	INVLOCFLAG_LINKDEATHS,
	INVLOCFLAG_RESERVED,
	INVLOCFLAG_EQUIPSLOT,
	INVLOCFLAG_USE_IN_RANDOM_ARMOR,
	INVLOCFLAG_OFFHANDWARDROBE,
	INVLOCFLAG_STORE,
	INVLOCFLAG_MERCHANT_WAREHOUSE,
	INVLOCFLAG_SKILL_CHECK_ON_ULTIMATE_OWNER,
	INVLOCFLAG_SKILL_CHECK_ON_CONTROL_UNIT,
	INVLOCFLAG_INGREDIENTS,
	INVLOCFLAG_DESTROY_PET_ON_LEVEL_CHANGE,
	INVLOCFLAG_ONLY_KNOWN_WHEN_STASH_OPEN,
	INVLOCFLAG_STASH_LOCATION,
	INVLOCFLAG_REMOVE_FROM_INVENTORY_ON_OWNER_DEATH,
	INVLOCFLAG_CANNOT_ACCEPT_NO_DROP_ITEMS,
	INVLOCFLAG_CANNOT_ACCEPT_NO_TRADE_ITEMS,
	INVLOCFLAG_CANNOT_DISMANTLE_ITEMS,
	INVLOCFLAG_WEAPON_CONFIG_LOCATION,	
	INVLOCFLAG_FREE_ON_SIZE_CHANGE,	
	INVLOCFLAG_CACHE_ENABLING,
	INVLOCFLAG_CACHE_DISABLING,
	INVLOCFLAG_EMAIL_LOCATION,
	INVLOCFLAG_EMAIL_INBOX,

	NUM_INVLOCFLAGS,
};

static const int SHARED_STASH_SECTION_HEIGHT = 10;

//----------------------------------------------------------------------------
enum INVENTORY_STYLE
{
	STYLE_INVALID = -1,
	
	STYLE_DEFAULT,																// player inventory only
	STYLE_MERCHANT,																// player and merchant inventory
	STYLE_TRADE,																// player and trade inventory
	STYLE_REWARD,																// player and reward inventory
	STYLE_STASH,																// player's stash and backpack inventory
	STYLE_OFFER,																// offering to player
	STYLE_TRAINER,																// trainer inventory
	STYLE_RECIPE,																// item crafting
	STYLE_RECIPE_SINGLE,														// single use item crafting from recipe
	STYLE_PET,																	// pet inventory
	STYLE_ITEM_UPGRADE,															// upgrade an item
	STYLE_ITEM_AUGMENT,															// augment an item
	STYLE_ITEM_UNMOD,															// remove modes from an item
	STYLE_CUBE,																	// the cube
	STYLE_BACKPACK,																// backpack item
	STYLE_STATTRADER,															// stat trader inventory
};


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct INVLOCIDX_DATA
{
	char								szName[DEFAULT_DESC_SIZE];
	WORD								wCode;
};

// excel table data describing inventory locations
struct INVLOC_DATA
{
	char								szName[DEFAULT_DESC_SIZE];
	int									nInventoryType;
	int									nLocation;
	int									nColorSetPriority;
	int									nSlotStat;
	int									nMaxSlotStat;
	int									nLocationCount;
	DWORD								dwFlags;
	int									nAllowTypes[INVLOC_UNITTYPES];
	int									nUsableByTypes[INVLOC_UNITTYPES];
	int									nWidth;
	int									nHeight;
	int									nFilter;
	BOOL								bCheckClass;
	int									nPreventLocation1;
	int									nPreventTypes1[INVLOC_UNITTYPES];
	int									nAllowTypeSkill[INVLOC_NUM_ALLOWTYPES_SKILLS];						// a list of skills that will allow certain item types to go in this location
	int									nSkillAllowTypes[INVLOC_NUM_ALLOWTYPES_SKILLS][INVLOC_UNITTYPES];	// the itemtypes unlocked by the corresponding skill
	int									nSkillAllowLevels[INVLOC_NUM_ALLOWTYPES_SKILLS];					// level required of the skill
	int									nWeaponIndex;
	BOOL								bPhysicallyInContainer;					// this inventory slot is a physical slot that is "on the container" (ie an item in your backpack is "on your player")
	BOOL								bTradeLocation;							// this inventory slot is used for player trading
	BOOL								bReturnStuckItemsToStandardLoc;			// items remaining in slots of this type will be automatically returned to standard locations whenever we want to make such a check
	BOOL								bStandardLocation;						// this slot is a standard slot the player interacts with regularly (paper doll slots, backpack, etc)
	BOOL								bOnPersonLocation;						// this is a slot that is considerd "with" the player at all times
	BOOL								bAllowedHotkeySourceLocation;			// only items that are in these locations can be put on/in hotkeys or weapon config slots
	BOOL								bRewardLocation;						// *the* single reward location slot for a player
	BOOL								bServerOnyLocation;						// the contents of this inventory location are not sent to clients
	BOOL								bIsCursor;								// is a cursor inventory location

	PCODE								codePlayerPutRestricted;				// player cannot place items into this inventory slot themselves, only the server can do that
	PCODE								codePlayerTakeRestricted;				// player cannot take items from this inventory slot themselves, only the server can do that
	
	int									nInvLocLoadFallbackOnLoadError;			// to recover from server load errors
};


// generated table describing inventories
struct INVENTORY_DATA
{
	int									nFirstLoc;								// first inventory location
	int									nNumLocs;
	int									nNumInitLocs;
};


// basic inventory location data
struct INVLOC_HEADER
{
	int									nLocation;
	DWORD								dwFlags;
	int									nColorSetPriority;
	int									nWeaponIndex;
	int									nFilter;
	int									nAllowTypes[INVLOC_UNITTYPES];
	int									nUsableByTypes[INVLOC_UNITTYPES];
	int									nWidth;
	int									nHeight;
	int									nCount;
	int									nPreventLocation1;
	int									nPreventTypes1[INVLOC_UNITTYPES];
};


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
// enum for UnitInventoryAdd
enum INV_CMD
{
	INV_CMD_PICKUP,																// pick up an item from the world (or from nowhere)
	INV_CMD_PICKUP_NOREQ,														// pick up an item from the world (or from nowhere), doesn't check requirements
	INV_CMD_REMOVE,
	INV_CMD_MOVE,																// move an item from one inventory location to another (possibly to a different container)
	INV_CMD_DROP,																// drop an item into the world
	INV_CMD_DESTROY,															// destroy an item
	INV_CMD_ADD_PET,
	INV_CMD_REMOVE_PET,
};

// enum for inventory command context
enum INV_CONTEXT
{
	INV_CONTEXT_NONE,
	INV_CONTEXT_TRADE,															// player traded away an item
	INV_CONTEXT_BUY,															// player bought an item
	INV_CONTEXT_SELL,															// player sold an item
	INV_CONTEXT_MAIL,															// player mailed an item
	INV_CONTEXT_DROP,															// player dropped an item
	INV_CONTEXT_DISMANTLE,														// player dismantled an item
	INV_CONTEXT_DESTROY,														// player destroyed an item
	INV_CONTEXT_UPGRADE,														// player upgraded an item, consuming this one
	INV_CONTEXT_RESTORE															// server tried to fix inventory that was in a bad state
};

#include "..\data_common\excel\invloc_hdr.h"									// auto-generated by inventory.xls | invloc


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL ExcelInventoryPostProcess(
	struct EXCEL_TABLE * table);

void UnitInvNodeInit(
	struct UNIT * unit);

void UnitInvNodeFree(
	struct UNIT * unit);

struct INVENTORY* UnitInventoryInit(
	struct UNIT * unit);

void UnitInventoryFree(
	struct UNIT * unit,
	BOOL bSendToClients);

BOOL UnitHasInventory(
	struct UNIT * unit);

void UnitInventoryContentsFree(
	struct UNIT * unit,
	BOOL bSendToClients);
	
BOOL UnitInventoryGetLocInfo(
	struct UNIT * unit,
	int location,
	INVLOC_HEADER * info);

int UnitInventoryGetArea(
	UNIT * unit,
	int location);

#if INVENTORY_DEBUG
#define UnitInventoryAdd(...)													INV_FUNCNAME(UnitInventoryAdd)(__FILE__, __LINE__, __VA_ARGS__)
#endif
BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,
	struct UNIT * container,
	struct UNIT * item,
	int location,
	DWORD dwChangeLocationFlags = 0,		// see CHANGE_LOCATION_FLAGS
	TIME tiEquipTime = TIME_ZERO));

BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int index,
	DWORD dwChangeLocationFlags = 0,		// see CHANGE_LOCATION_FLAGS
	TIME tiEquipTime = TIME_ZERO));

BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int x,
	int y,
	DWORD dwChangeLocationFlags = 0,		// see CHANGE_LOCATION_FLAGS
	TIME tiEquipTime = TIME_ZERO,
	INV_CONTEXT context = INV_CONTEXT_NONE));

BOOL INV_FUNCNAME(UnitWeaponConfigInventoryAdd)(INV_FUNCARGS(
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y));

UNIT * UnitGetContainer(
	UNIT * unit);
	
BOOL ItemIsEquipped(
	UNIT * item,
	UNIT * container);

BOOL UnitIsContainedBy(
	UNIT * unit,
	UNIT * container);

UNIT * UnitGetUltimateContainer(
	UNIT * unit);
	
struct UNIT * UnitInventoryGetByIndex(
	struct UNIT * container,
	int index);

#if INVENTORY_DEBUG
#define UnitInventoryGetByLocation(...)										INV_FUNCNAME(UnitInventoryGetByLocation)(__VA_ARGS__, __FILE__, __LINE__)
#endif
struct UNIT * INV_FUNCNAME(UnitInventoryGetByLocation)(INV_FUNCARGS(
	struct UNIT * container,
	int location));

#if INVENTORY_DEBUG
#define UnitInventoryGetByLocationAndIndex(...)								INV_FUNCNAME(UnitInventoryGetByLocationAndIndex)(__VA_ARGS__, __FILE__, __LINE__)
#endif
struct UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndIndex)(INV_FUNCARGS(
	struct UNIT * container,
	int location,
	int index));

#if INVENTORY_DEBUG
#define UnitInventoryGetByLocationAndXY(...)								INV_FUNCNAME(UnitInventoryGetByLocationAndXY)(__VA_ARGS__, __FILE__, __LINE__)
#endif
struct UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndXY)(INV_FUNCARGS(
	struct UNIT * container,
	int location,
	int x,
	int y));

#if INVENTORY_DEBUG
#define UnitInventoryGetByLocationAndUnitId(...)							INV_FUNCNAME(UnitInventoryGetByLocationAndUnitId)(__VA_ARGS__, __FILE__, __LINE__)
#endif
	struct UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndUnitId)(INV_FUNCARGS(
	struct UNIT * container,
	int location,
	UNITID unitid));

int InventoryGridGetItemsOverlap(
	UNIT * container,
	UNIT * pItemToCheck,
	UNIT *& pOverlappedItem,
	int location,
	int x,
	int y);

BOOL IsAllowedUnitInv(
	UNIT * unit,
	const int location,
	UNIT * container,
	DWORD dwChangeLocationFlags = 0);		// see CHANGE_LOCATION_FLAGS

BOOL InventoryItemMeetsClassReqs(
	const struct UNIT_DATA * unitdata,
	UNIT * container);

BOOL InventoryItemMeetsClassReqs(
	UNIT * unit,
	UNIT * container);

BOOL InventoryItemMeetsSkillGroupReqs(
	UNIT * unit,
	UNIT * container);

BOOL InventoryItemHasUseableQuality(
	UNIT * unit);

BOOL UnitInventoryTest(
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int x = 0,
	int y = 0,
	DWORD dwChangeLocationFlags = 0);		// see CHANGE_LOCATION_FLAGS

BOOL UnitInventoryTestIgnoreExisting(
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int x = 0,
	int y = 0);

BOOL UnitInventoryTestIgnoreItem(
	struct UNIT * container,
	struct UNIT * item,
	struct UNIT * ignoreitem,
	int location,
	int x = 0,
	int y = 0);

BOOL IsUnitPreventedFromInvLoc(
	struct UNIT * container,
	struct UNIT * item,
	int nLocation,
	int * pnPreventingLoc = NULL);

BOOL IsUnitPreventedFromInvLocIgnoreItem(
	struct UNIT * container,
	struct UNIT * item,
	struct UNIT * ignore_item,
	int nLocation);

int ItemGetItemsAlreadyInEquipLocations(
	UNIT * container,
	UNIT * item,
	BOOL bCheckEquipped,
	BOOL& bEmptySlot,
	UNIT ** ppItemList,
	int nListSize,
	BOOL *pbPreventLoc = NULL);

int UnitInventoryGetOverlap(
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int x,
	int y,
	struct UNIT ** itemlist,
	int listsize);

enum INVENTORY_REMOVE
{
	ITEM_ALL,		// take entire stack of item from inventory
	ITEM_SINGLE,	// take single item of stack from inventory
};

#if INVENTORY_DEBUG
#define UnitInventoryRemove(...)													INV_FUNCNAME(UnitInventoryRemove)(__FILE__, __LINE__, __VA_ARGS__)
#endif
UNIT * INV_FUNCNAME(UnitInventoryRemove)(INV_FUNCFLARGS(
	struct UNIT * item,
	INVENTORY_REMOVE eRemove,
	DWORD dwChangeLocationFlags = 0,
	BOOL bRemoveFromHotkeys = TRUE,
	INV_CONTEXT eContext = INV_CONTEXT_NONE));

const INVLOC_DATA * UnitGetInvLocData(
	UNIT * unit,
	int location);

DWORD UnitGetInvLocCode(
	UNIT *pContainer,
	int nInvLocation);

BOOL UnitInventoryHasLocation(
	UNIT *unit,
	int location);

UNIT * UnitInventoryGetOldestItem(
	UNIT * pContainer,
	int nLocation);

INVLOC * InventoryGetLocAdd(
	UNIT * unit,
	int location);


//----------------------------------------------------------------------------
enum INVENTORY_ITERATE_FLAGS
{
	IIF_ON_PERSON_SLOTS_ONLY_BIT,		// only items in slots that are "on the player"
	IIF_IGNORE_EQUIP_SLOTS_BIT,			// don't consider items in equipable slots
	IIF_ACTIVE_STORE_SLOTS_ONLY_BIT,	// only items in any active store slot
	IIF_MERCHANT_WAREHOUSE_ONLY_BIT,	// only items in the slot where we store all items for a player unique store
	IIF_EQUIP_SLOTS_ONLY_BIT,			// only items in equipped slots
	IIF_TRADE_SLOTS_ONLY_BIT,			// only items in trade slots
	IIF_ON_PERSON_AND_STASH_ONLY_BIT,	// all on_person_slots (equipped,backpack,weapon slots) and the stash
	IIF_PET_SLOTS_ONLY_BIT,				// only items in trade slots
	IFF_IN_CUBE_ONLY_BIT,				// only items in the cube
	IFF_CHECK_DIFFICULTY_REQ_BIT,		// only items that match the player's current difficulty, or that have no difficulty requirement
};


#if INVENTORY_DEBUG
#define UnitInventoryIterate(...)											INV_FUNCNAME(UnitInventoryIterate)(__FILE__, __LINE__, __VA_ARGS__)
#endif
UNIT * INV_FUNCNAME(UnitInventoryIterate)(INV_FUNCFLARGS(
	struct UNIT * container,
	struct UNIT * item,
	DWORD dwInvIterateFlags = 0,											// see INVENTORY_ITERATE_FLAGS
	BOOL bIterateChildren = TRUE));

#if INVENTORY_DEBUG
#define UnitInventoryLocationIterate(...)									INV_FUNCNAME(UnitInventoryLocationIterate)(__FILE__, __LINE__, __VA_ARGS__)
#endif
UNIT * INV_FUNCNAME(UnitInventoryLocationIterate)(INV_FUNCFLARGS(
	struct UNIT * container,
	int location,
	struct UNIT * item,
	DWORD dwInvIterateFlags = 0,											// see INVENTORY_ITERATE_FLAGS
	BOOL bIterateChildren = TRUE));

BOOL UnitGetInventoryLocation(
	struct UNIT * item,
	int * location);

BOOL UnitGetCurrentInventoryLocation(
	UNIT * item,
	int& location,
	int& x,
	int& y);

BOOL UnitGetInventoryLocation(
	struct UNIT * item,
	int * location,
	int * x,
	int * y);

BOOL UnitGetInventoryLocationNoEquip(
	struct UNIT * item,
	int * location,
	int * x,
	int * y);

void InventoryLocationInit(
	struct INVENTORY_LOCATION &tInventoryLocation);
	
BOOL UnitGetInventoryLocation(
	struct UNIT * item,
	struct INVENTORY_LOCATION *pLocation);
	
TIME UnitGetInventoryEquipTime(
	struct UNIT * unit);

BOOL UnitInventoryTestFlag(
	UNIT * container,
	int location,
	int nFlag );

int UnitInventoryGetCount(
	struct UNIT * container);

int UnitInventoryGetCount(
	struct UNIT * container,
	int location);

int UnitInventoryLocCountAliveInWorld(
	UNIT *pContainer,
	int nInvLoc);
	
int UnitInventoryCountItemsOfType(
	struct UNIT * container,
	int nItemClass,
	DWORD dwInvIterateFlags,		// see INVENTORY_ITERATE_FLAGS
	int nItemQuality = INVALID_LINK,
	int nInvLoc = INVLOC_NONE,
	UNIT *pUnitIgnore = NULL);		

int UnitInventoryCountUnitsOfType(
	struct UNIT * container,
	const SPECIES species,
	DWORD dwInvIterateFlags,		// see INVENTORY_ITERATE_FLAGS
	int nItemQuality = INVALID_LINK,
	int nInvLoc = INVLOC_NONE,
	UNIT *pUnitIgnore = NULL);		

BOOL UnitInventoryCanPickupAll( 
	UNIT* pUnit,
	UNIT** pToPickup,
	int nNumToPickup,
	BOOL bDontCheckCanKnowUnit = FALSE);

BOOL UnitInventoryFindSpace(
	struct UNIT * container,
	struct UNIT * item,
	int location,
	int * x,
	int * y,
	DWORD dwChangeLocationFlags = 0,		// see CHANGE_LOCATION_FLAGS
	struct UNIT *pIgnoreItem = NULL,
	DWORD dwItemEquipFlags = 0);

BOOL ItemGetOpenLocation(
	struct UNIT * container,
	struct UNIT * item,
	BOOL bAutoPickup,
	int * location,
	int * x,
	int * y,
	BOOL bCheckRequirements = FALSE,
	DWORD dwChangeLocationFlags = 0,
	DWORD dwItemEquipFlags = 0);

enum ITEM_STACK_RESULT
{
	ITEM_STACK_NO_STACK,
	ITEM_STACK_COMBINE,
	ITEM_STACK_FILL_STACK,
};

ITEM_STACK_RESULT ItemStack(
	struct UNIT * itemDest,
	struct UNIT * itemSrc);

UNIT* ItemCanAutoStack(
	struct UNIT * container,
	struct UNIT * item);
	
UNIT* ItemAutoStack(
	struct UNIT * container,
	struct UNIT * item);

BOOL ItemGetEquipLocation(
	struct UNIT * container,
	struct UNIT * item,
	int * location,
	int * x,
	int * y,
	int nExceptLocation = INVLOC_NONE,
	BOOL bSkipIdentifiedCheck = FALSE);

BOOL UnitInventoryTestSwap(
	struct UNIT * item1,
	struct UNIT * item2,
	int nAltDestX = -1,
	int nAltDestY = -1);

BOOL UnitInventorySetEquippedWeapons(
	struct UNIT * container,
	struct UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	BOOL bUpdateUI = FALSE );

void UnitInventoryGetEquippedWeapons(
	struct UNIT * container,
	struct UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ] );

struct UNIT * UnitGetContainerInRoom(
	struct UNIT * item);
	
template <XFER_MODE mode>
BOOL InventoryXfer(
	struct UNIT *pContainer,
	GAMECLIENTID idClient,
	class XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bSkipData,
	int *pnUnitsXfered);

template BOOL InventoryXfer<XFER_SAVE>(struct UNIT *, GAMECLIENTID, class XFER<XFER_SAVE> &, UNIT_FILE_HEADER &, BOOL, int *);
template BOOL InventoryXfer<XFER_LOAD>(struct UNIT *, GAMECLIENTID, class XFER<XFER_LOAD> &, UNIT_FILE_HEADER &, BOOL, int *);

int UnitGetInventoryType(
	struct UNIT * unit);

struct UNIT * InventoryGetReplacementItemFromLocation(
	struct UNIT * unit,
	int location,
	int nUnitType,
	int nUnitClass = INVALID_ID );

#ifdef _DEBUG
void trace_Inventory(
	struct UNIT * unit);
#endif // _DEBUG

void InventoryEquippedTriggerEvent( 
	GAME *pGame,
	enum UNIT_EVENT eEvent,
	UNIT *pUnit,
	UNIT *pOtherUnit,
	void *pData);

void SetEquippedItemUnusable(
	GAME * game,
	UNIT * unit,
	UNIT * item);

void SetEquippedItemUsable(
	GAME * game,
	UNIT * unit,
	UNIT * item);

inline EINVLOC UnitGetBigBagInvLoc( void )
{
	return INVLOC_BIGPACK;
}

inline BOOL InvLocTestFlag(
	INVLOC_HEADER * invloc,
	int flag)
{
	ASSERT_RETFALSE(invloc);
	return TESTBIT(&invloc->dwFlags, flag);
}
	
inline BOOL InvLocTestFlag(
	const INVLOC_DATA * invloc,
	int flag)
{
	ASSERT_RETFALSE(invloc);
	return TESTBIT(&invloc->dwFlags, flag);
}


inline void InvLocSetFlag(
	INVLOC_HEADER * invloc,
	int flag,
	BOOL value = TRUE)
{
	ASSERT_RETURN(invloc);
	SETBIT(&invloc->dwFlags, flag, value);
}


inline void InvLocSetFlag(
	INVLOC_DATA * invloc,
	int flag,
	BOOL value = TRUE)
{
	ASSERT_RETURN(invloc);
	SETBIT(&invloc->dwFlags, flag, value);
}

int InventoryCheckStatRequirement(
	int stat,
	GAME * game,
	UNIT * container,
	UNIT * item,
	UNIT ** listIgnoreItems,
	unsigned int numIgnoreItems);

BOOL InventoryCheckStatRequirements(
	struct UNIT * container,
	struct UNIT * item,
	struct UNIT * ignoreitem1,
	struct UNIT * ignoreitem2,
	int location,
	DWORD * pdwFailedStats = NULL,
	int nStatsArraySize = 0,
	DWORD dwEquipFlags = 0);

//----------------------------------------------------------------------------
enum ITEM_EQUIP_FLAGS
{
	IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT,	// ignore the "player can put" restriction on inv slots of the item
	IEF_CHECK_REQS_ON_IMMEDIATE_CONTAINTER_ONLY_BIT,		// when checking feed and limit requirements, don't check container's container
};

BOOL InventoryItemCanBeEquipped(
	UNIT * container,
	UNIT * item,
	UNIT * ignoreitem1,
	UNIT * ignoreitem2,
	int location,
	DWORD *pdwFailedStats = NULL,
	int nStatsArraySize = 0,
	DWORD dwItemEquipFlags = 0);

BOOL InventoryItemCanBeEquipped(
	UNIT * container,
	UNIT * item,
	int nLocation = INVLOC_NONE,
	DWORD *pdwFailedStats = NULL,
	int nStatsArraySize = 0,
	DWORD dwItemEquipFlags = 0);

BOOL InvLocIsEquipLocation ( 
	int nLocation,
	struct UNIT * pContainer);

BOOL InvLocIsWeaponLocation ( 
	int nLocation,
	struct UNIT * pContainer);

BOOL InvLocIsWarehouseLocation( 
	struct UNIT *pContainer,
	int nLocation);

BOOL InvLocIsOnlyKnownWhenStashOpen(
	UNIT *pContainer,
	int nLocation);

BOOL InvLocIsStashLocation(
	UNIT *pContainer,
	int nLocation);

BOOL ItemIsInEquipLocation ( 
	struct UNIT * pContainer,
	struct UNIT * pItem );

BOOL ItemIsInNoDismantleLocation ( 
	struct UNIT * container,
	struct UNIT * item);

BOOL ItemIsInWeaponSetLocation ( 
	struct UNIT * pContainer,
	struct UNIT * pItem );

BOOL InventoryDoubleEquip(
	struct GAME * game,
	struct UNIT *pContainer,
	UNITID idItem1,
	UNITID idItem2,
	int nLocation1,
	int nLocation2);

BOOL InventorySwapWeaponSet(
	struct GAME * game,
	struct UNIT * pContainer);

//----------------------------------------------------------------------------
enum CHANGE_LOCATION_FLAGS
{
	CLF_CONFIG_SWAP_BIT										= MAKE_MASK(0),		// is a config swap operation
	CLF_SWAP_BIT											= MAKE_MASK(1),		// is an inventory swap operation, the ultimate owner isn't really changing, but the item was probably dropped temporarily
	CLF_ALLOW_NEW_CONTAINER_BIT								= MAKE_MASK(2),		// allow change to new container unit
	CLF_ALLOW_EQUIP_WHEN_IN_PLAYER_PUT_RESTRICTED_LOC_BIT	= MAKE_MASK(3),		// allow equipping even if item is in an inv loc that the player is not allow to put item in (like a store)
	CLF_IGNORE_NO_DROP_GRID_BIT								= MAKE_MASK(4),		// ignore the check that see's if you can put a no drop item in the destination grid
	CLF_DISALLOW_ALT_GRID_PLACEMENT							= MAKE_MASK(5),		// if trying to swap two items and one won't fit in the other's old place in the grid, don't allow the function to see if it will fit somewhere else
	CLF_FORCE_SEND_REMOVE_MESSAGE							= MAKE_MASK(6),		// always send remove message even if item isn't equipped
	CLF_IGNORE_UNIDENTIFIED_MASK							= MAKE_MASK(7),		// allow sIsAllowedUnitInv to proceed without checking if the item's identified
	CLF_CACHE_ITEM_ON_SEND									= MAKE_MASK(8),		// this is an item we would like cached on the client
	CLF_PICKUP_NOREQ										= MAKE_MASK(9),		// this is coming from a trusted server command that wants to ignore most requirement checks
	CLF_IGNORE_REQUIREMENTS_ON_SWAP							= MAKE_MASK(10),	// on a mythos weaponswap, ignore requirements (item will be disabled properly after the swap)
	CLF_DO_NOT_EQUIP										= MAKE_MASK(11),
	CLF_CHECK_PLAYER_PUT_RESTRICTED							= MAKE_MASK(12),	// test the "player put restricted" condition from the spreadsheet
	CLF_ALWAYS_ALLOW_IN_EMAIL								= MAKE_MASK(13),	// always allow this item in any email related slot
};

BOOL InventoryLocPlayerCanPut( 
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLoc);
	
BOOL InventoryLocPlayerCanTake( 
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLoc);
	
BOOL InventoryTestPut(
	UNIT * unit, 
	UNIT * item, 
	int location, 
	int x, 
	int y,
	DWORD dwChangeLocationFlags);												// see CHANGE_LOCATION_FLAGS

#if INVENTORY_DEBUG
#define InventoryChangeLocation(...)											INV_FUNCNAME(InventoryChangeLocation)(__FILE__, __LINE__, __VA_ARGS__)
#endif
BOOL INV_FUNCNAME(InventoryChangeLocation)(INV_FUNCFLARGS(
	UNIT * unit,
	UNIT * item,
	int location,
	int x,
	int y,
	DWORD dwChangeLocationFlags,												// see CHANGE_LOCATION_FLAGS
	INV_CONTEXT context = INV_CONTEXT_NONE));

BOOL InventorySwapLocation(
	UNIT *itemSrc,
	UNIT *itemDest,
	int nAltDestX = -1,
	int nAltDestY = -1,
	DWORD dwChangeLocationFlags = 0);

BOOL InventoryMoveToAnywhereInLocation(
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLocation,
	DWORD dwChangInvFlags);

BOOL InventoryIsInStandardLocation( 
	UNIT *pUnit);

BOOL InventoryIsInIngredientsLocation(
	UNIT *pUnit);

BOOL InventoryIsIngredientLocation(
	UNIT *pContainer,
	int nInventoryLocation);

BOOL InventoryIsTradeLocation(
	UNIT *pContainer,
	int nInventoryLocation);

BOOL InventoryIsInTradeLocation( 
	UNIT *pUnit);

BOOL InventoryIsInCacheClearingLocation(
	UNIT *pUnit);

int InventoryGetTradeLocation(
	UNIT *pUnit);

BOOL InventoryIsPetLocation(
	UNIT *pContainer,
	int nInventoryLocation);

BOOL InventoryIsInPetLocation( 
	UNIT *pUnit);

BOOL InventoryIsInNoSaveLocation(
	UNIT *pUnit);
	
BOOL InventoryIsInServerOnlyLocation( 
	UNIT *pUnit);

enum LOCATION_TYPE
{
	LT_STANDARD,
	LT_ON_PERSON,
	LT_CACHE_DISABLING,
	LT_CACHE_ENABLING,
};

BOOL InventoryIsInLocationType(
	UNIT *pUnit,
	LOCATION_TYPE eType,
	BOOL bFollowContainerChain = TRUE);
	
BOOL InventoryIsInStashLocation( 
	UNIT *pUnit);

BOOL InventoryIsInAllowedHotkeySourceLocation(
	UNIT *pUnit);

BOOL InvLocIsRewardLocation(
	UNIT *pContainer, 
	int nInventoryLocation);

BOOL InvLocIsServerOnlyLocation(
	UNIT *pContainer, 
	int nInventoryLocation);

BOOL InvLocIsStandardLocation(
	UNIT *pContainer, 
	int nInventoryLocation);

BOOL InvLocIsOnPersonLocation(
	UNIT *pContainer, 
	int nInventoryLocation);

BOOL InventoryIsAllowedHotkeySourceLocation(
	UNIT *pContainer, 
	int nInventoryLocation);

BOOL InvLocIsStoreLocation( 
	UNIT *pContainer,
	int nLocation);

int InventoryGetRewardLocation(
	UNIT *pUnit);

void InventoryDropAll( 
	UNIT *pUnit );


//broadcasts an event such as equip to all the items in an inventory.
//or say broadcast an event to make all items of a certain type glow for a second
BOOL InvBroadcastEventOnUnitsByFlags( 
	UNIT *pUnit,
	UNIT_EVENT eventID,
	DWORD flags);

		
//----------------------------------------------------------------------------
enum INVENTORY_SEND_FLAGS
{
	ISF_EQUIPPED_ONLY_BIT,			// send only items that are equipped (maybe for inspecting other players in the future)
	ISF_USABLE_BY_CLASS_ONLY_BIT,	// send only items that are usable by the character class of the client
	ISF_CLIENT_CANT_KNOW_BIT,		// send only items that the client shouldn't know
	ISF_CLIENT_CAN_KNOW_BIT,		// send only items that the client can  know
};

void s_InventorySendToClient( 
	UNIT *pContainer, 
	GAMECLIENT *pClient,
	DWORD dwFlags);		// see INVENTORY_SEND_FLAGS

void s_InventoryRemoveFromClient( 
	UNIT *pContainer, 
	GAMECLIENT *pClient,
	DWORD dwFlags);		// see INVENTORY_SEND_FLAGS

void s_DropEquippedItemsPVP( 
	GAME * game,
	UNIT * unit,
	int nItems);

void s_InventoryChanged( 
	UNIT *pUnit);

DROP_RESULT InventoryCanDropItem(
	UNIT *pDropper,
	UNIT *pItem);

BOOL InventoryLinkUnitToInventory(			
	UNIT * unit,
	UNIT * container,
	const INVENTORY_LOCATION & tInvLoc,
	TIME tiEquipTime,
	BOOL bAllowFallback,
	DWORD dwReadUnitFlags);		// see READ_UNIT_FLAGS

BOOL UnitGetSizeInGrid(
	UNIT * unit,
	int* height,
	int* width,
	struct INVLOC * loc);

void s_SendInventoryFullSound(
	GAME * game,
	UNIT * unit );

void s_InventoryClearWaitingForMove(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pItem);

void s_InventoryPut(
	UNIT *pUnit,
	UNIT *pItem,
	INVENTORY_LOCATION & tInvLocation,
	DWORD dwChangeLocFlags = 0,		// see CHANGE_LOCATION_FLAGS
	INV_CONTEXT eContext = INV_CONTEXT_NONE);

void c_InventoryTryPut(
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLocation,
	int nX,
	int nY);
	
void c_InventoryTrySwap(
	UNIT *pItemSrc,
	UNIT *pItemDest,
	int nFromWeaponConfig = -1,
	int nAltDestX = -1,
	int nAltDestY = -1);

UNIT *InventoryGetFirstAnalyzer(
	UNIT *pUnit);

const INVENTORY_DATA * InventoryGetData(
	UNIT * unit);

void s_sSendStashToClient( 
	UNIT *pPlayer);

void s_sRemoveStashFromClient( 
	UNIT *pPlayer);

int UnitInventoryGridGetNumEmptySpaces(
	UNIT * container,
	int location);

BOOL ItemGetOpenLocationForPlayerToPutDown(
	UNIT * container,
	UNIT * item,
	BOOL bCheckRequirements =FALSE,
	BOOL bOnPersonOnly = FALSE);

void s_InventorysVerifyEquipped( 
	GAME * game,
	UNIT * unit);

int InventoryCalculatePVPGearDrop( 
	GAME * game,
	UNIT * unit);
//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const INVENTORY_DATA * InventoryGetData(
	GAME * game,
	int inventory)
{
	return (const INVENTORY_DATA *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_INVENTORY, inventory);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const INVLOC_DATA * InvLocGetData(
	GAME * game,
	int nInvLoc)
{
	return (const INVLOC_DATA *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_INVLOC, nInvLoc);
}		


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const INVLOCIDX_DATA * InvLocIndexGetData(
	int nIndex)
{
	return (const INVLOCIDX_DATA *)ExcelGetData(EXCEL_CONTEXT(NULL), DATATABLE_INVLOCIDX, nIndex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * FindInUseGuildHerald(
	UNIT *pPlayer);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_BackpackOpen(
	UNIT * pPlayer,
	UNIT * pBackpack);

void c_BackpackOpen( 
	UNIT * pBackpack );

void c_BackpackClose();

#endif // _INVENTORY_H_
