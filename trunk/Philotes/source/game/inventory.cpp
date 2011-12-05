//----------------------------------------------------------------------------
// inventory.cpp
// (C) Copyright 2003, Flagship Studios, all rights reserved
//
// NOTES: handitem could be a separate loc, but why?  instead, when you
// pick up an item from your inventory, just "ghost" the item until you
// actually do something with it, at which point, remove the item
//
// TODO: proper stat filters, make network calls to update client
// TODO: charms -- make a callback which determines if an item applies its
// stats to its container
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "console.h"
#include "game.h"
#include "units.h"
#include "gameunits.h"
#include "unit_priv.h"
#include "stats.h"
#include "inventory.h"
#include "s_message.h"
#include "wardrobe.h"
#include "items.h"
#include "colors.h"
#include "uix.h"
#include "uix_components_complex.h"
#include "c_sound.h"
#include "unitmodes.h"
#include "skills.h"
#include "player.h"
#include "monsters.h"
#include "unitidmap.h"
#include "c_camera.h"
#include "clients.h"
#include "c_message.h"
#include "pets.h"
#include "ptime.h"
#include "windowsmessages.h"
#include "quests.h"
#include "tasks.h"
#include "e_model.h"
#include "performance.h"
#include "unittag.h"
#include "s_trade.h"
#include "s_reward.h"
#include "weaponconfig.h"
#include "s_quests.h"
#include "globalindex.h"
#include "Quest.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "uix_priv.h"
#include "s_recipe.h"
#include "states.h"
#include "stash.h"
#include "excel_private.h"
#include "hotkeys.h"
#include "Economy.h"
#include "Currency.h"
#include "script.h"
#include "unitfilecommon.h"
#include "achievements.h"
#include "stringtable.h"
#include "c_sound_util.h"
#include "gamevariant.h"
#include "s_playerEmail.h"

#if ISVERSION(SERVER_VERSION)
#include "inventory.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// DEBUG CONSTANTS & MACROS
//----------------------------------------------------------------------------
#define INVENTORY_TRACE_LEVEL			2						// 0 = no tracing, 1 = errors, 2 = major ops, 3 = minor operations

#if	INVENTORY_DEBUG && ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
# if (INVENTORY_TRACE_LEVEL >= 1)
#  define INVENTORY_TRACE1(fmt, ...)							TraceInventory(fmt, __VA_ARGS__)
# endif
# if (INVENTORY_TRACE_LEVEL >= 2)
#  define INVENTORY_TRACE2(fmt, ...)							TraceInventory(fmt, __VA_ARGS__)
# endif
# if (INVENTORY_TRACE_LEVEL >= 3)
#  define INVENTORY_TRACE2(fmt, ...)							TraceInventory(fmt, __VA_ARGS__)
# endif
#endif

#ifndef INVENTORY_TRACE1
#define INVENTORY_TRACE1(fmt, ...)
#endif

#ifndef INVENTORY_TRACE2
#define INVENTORY_TRACE2(fmt, ...)
#endif

#ifndef INVENTORY_TRACE3
#define INVENTORY_TRACE3(fmt, ...)
#endif

#define INVENTORY_CHECK_DEBUGMAGIC(inv)							ASSERT(inv->dwDebugMagic == INVENTORY_DEBUG_MAGIC)


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define	INVENTORY_DEBUG_MAGIC			('INV ')						// this dword is the front of every inventory structure
#define INVENTORY_ERROR					-101
#define INVALID_LOCATION				-102
#define INVALID_COORDINATE				-1
#define MAX_LOCATIONS					96
#define RESERVED_MARK					3								// what we temporarily put in the inventory grid when asking reservation questions
																		// i don't want to make this something other than 0 or 1 so it's more "obvious" for debugging
#define INVFLAG_DYNAMIC					0


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// any unit that can be in an inventory has one of these
struct INVENTORY_NODE
{
	UNIT *								container;						// who's inventory i'm in
	UNIT *								invprev;						// linked list of items in [container]
	UNIT *								invnext;
	UNIT *								locprev;						// linked list of items in [location]
	UNIT *								locnext;
	UNIT *								equipprev;						// linked list of equipped items
	UNIT *								equipnext;
	TIME								m_tiEquipTime;					// time this item was placed in its current location
	int									invloc;							// inventory location i'm in
	int									x;								// my coordinates in the inventory
	int									y;
};

//----------------------------------------------------------------------------
// a unit's inventory
struct INVENTORY
{
	DWORD								dwDebugMagic;
	DWORD								dwFlags;

	const INVENTORY_DATA *				inv_data;						// excel data
	INVLOC *							pLocs;							// inventory locations
	UNIT *								itemfirst;						// doubly-linked list contains all items in the inventory
	UNIT *								itemlast;
	UNIT *								equippedfirst;					// equipped items
	UNIT *								equippedlast;

	UNITID								pWeapons[MAX_WEAPONS_PER_UNIT];	// currently applied weapons in inventory
	int									nInventory;						// inventory index in inventory.xls
	int									nLocs;							// number of inventory locations  (size of pLocs)
	int									nCount;							// number of items in an inventory (itemfirst to itemlast)
};


//----------------------------------------------------------------------------
// a location in an inventory (instance)
// if you add anything here, be sure to add it to sInventoryCopyLocData()
struct INVLOC : INVLOC_HEADER
{
	UNIT *								itemfirst;						// doubly-linked list of all items in a location
	UNIT *								itemlast;
	UNIT **								ppUnitGrid;						// grid of items, each square points to unit occupying that square
	int									nAllocatedWidth;				// allocated grid size (may be different from actual grid size for dynamic locations)
	int									nAllocatedHeight;
};




//----------------------------------------------------------------------------
// forward declaration used by INVENTORY_COMMAND
static inline const INVLOC_DATA * sInventoryGetInvLocData(
	GAME * game,
	INVENTORY * inv,
	int location);

static BOOL sItemIsEquipped(
	UNIT * container,
	UNIT * item);

inline INVLOC * InventoryGetLoc(
	UNIT * unit,
	int location,
	INVLOC * locdata);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct INVENTORY_COMMAND
{
	GAME *								game;							// game
	UNIT *								newContainer;					// destination container
	UNIT *								item;							// item to add to inventory
	UNIT *								oldContainer;					// container prior to move
	UNIT *								newUltimateContainer;			// ultimate container (not always owner) for container
	UNIT *								oldUltimateOwner;
	UNIT *								newUltimateOwner;
	ROOM *								oldRoom;						// room of item prior to move (if item was in world)
	INVENTORY_NODE *					invnode;						// item's invnode (any unit that can go into an inventory must have one)
	INVENTORY *							newInv;							// container's inventory structure
	INVENTORY *							oldInv;
	INVLOC *							newLoc;							// data for container's location
	INVLOC *							oldLoc;
	const INVLOC_DATA *					newLocData;
	const INVLOC_DATA *					oldLocData;
	TIME								tiEquipTime;					// real equip time
	INV_CMD								cmd;							// inventory command enum
	INVENTORY_REMOVE					removecmd;						// inventory remove sub command (single vs. all)
	int									newLocation;					// inventory location
	int									oldLocation;					// old location
	int									newIndex;						// index in list-based location (used only for non-grid lists)
	int									oldIndex;
	int									newX;							// x position in grid
	int									newY;							// y position in grid
	int									oldX;
	int									oldY;
	int									wd;								// item width
	int									ht;								// item height
	DWORD								dwChangeLocationFlags;			// see CHANGE_LOCATION_FLAGS
	BOOL								oldIsInTradeLocation;			// is the item currently in a trade location (prior to move)
	BOOL								newIsGrid;						// set if loc is a grid
	BOOL								isValid;						// set to TRUE if constructor was a success
	BOOL								wasEquipped;					// was equipped in old position
	BOOL								newEquipped;					// is equipped in new position
	BOOL								bSkillsStoppedOldContainer;		// set after call to SkillsStopAll for oldContainer
	BOOL								bSkillsStoppedNewContainer;		// set after call to SkillsStopAll for container
	INV_CONTEXT							context;						// why the action is occurring (sell, dismantle, etc.)
	BOOL								bWasInEmailInbox;				// this item was in th emeail inbox at old position
#if INVENTORY_DEBUG
	const char *						m_file;
	unsigned int						m_line;
#endif

	INVENTORY_COMMAND(INV_FUNCARGS_NODEFAULT(
		INV_CMD in_cmd,
		UNIT * in_container,
		UNIT * in_item,
		int in_location,
		int in_index,
		int in_x,
		int in_y,
		DWORD in_flags,
		TIME in_tiEquipTime,
		INV_CONTEXT in_context)) : cmd(in_cmd), newContainer(in_container), item(in_item), newLocation(in_location), 
			oldUltimateOwner(NULL), newUltimateOwner(NULL),
			newIndex(in_index), newX(in_x), newY(in_y), dwChangeLocationFlags(in_flags), tiEquipTime(in_tiEquipTime), ht(0), wd(0),
			bSkillsStoppedOldContainer(FALSE), bSkillsStoppedNewContainer(FALSE), context(in_context)
	{
		isValid = FALSE;

#if INVENTORY_DEBUG
		m_file = file;
		m_line = line;
#endif
		ASSERT_RETURN(cmd == INV_CMD_REMOVE || newContainer);
		ASSERT_RETURN(item);

		game = UnitGetGame(item);
		ASSERT_RETURN(game);

		invnode = item->invnode;
		ASSERT_RETURN(invnode);

		oldContainer = UnitGetContainer(item);
		oldLocation = invnode->invloc;
		oldIndex = -1;
		oldX = invnode->x;
		oldY = invnode->y;
		if (oldContainer)
		{
			oldInv = oldContainer->inventory;
			oldUltimateOwner = UnitGetUltimateOwner(oldContainer);
			oldLoc = InventoryGetLoc(oldContainer, invnode->invloc, NULL);
			ASSERT_RETURN(oldLoc);
			oldLocData = sInventoryGetInvLocData(game, oldInv, oldLocation);	
			wasEquipped = sItemIsEquipped(oldContainer, item);
			oldIsInTradeLocation = InventoryIsInTradeLocation(item);
			bWasInEmailInbox = InventoryIsInLocationWithFlag(item, INVLOCFLAG_EMAIL_INBOX);
		}
		else
		{
			oldInv = NULL;
			oldLoc = NULL;
			oldLocData = NULL;
			oldUltimateOwner = NULL;
			wasEquipped = FALSE;
			oldIsInTradeLocation = FALSE;
		}
		oldRoom = UnitGetRoom(item);

		if (newContainer)
		{
			newInv = newContainer->inventory;
			if (!newInv)
			{
				newInv = UnitInventoryInit(newContainer);
			}
			ASSERT_RETURN(newInv);
			newUltimateContainer = UnitGetUltimateContainer(newContainer);
		}
		else
		{
			newInv = NULL;
			newUltimateContainer = NULL;
		}
		newEquipped = FALSE;

		if (newLocation >= 0)
		{
			newLoc = InventoryGetLocAdd(newContainer, newLocation);
			ASSERT_RETURN(newLoc);
			ASSERT_RETURN(newLoc->nLocation == newLocation);
			newIsGrid = InvLocTestFlag(newLoc, INVLOCFLAG_GRID);

			newLocData = sInventoryGetInvLocData(game, newInv, newLocation);	
			ASSERT_RETURN(newLocData);
		}
		else
		{
			newLoc = NULL;
			newIsGrid = FALSE;
			newLocData = NULL;
		}

		isValid = TRUE;
	}

	inline UNIT * GetNewUltimateOwner(
		void)
	{
		if (newUltimateOwner)
		{
			return newUltimateOwner;
		}
		ASSERT_RETFALSE(newContainer);
		newUltimateOwner = UnitGetUltimateOwner(newContainer);
		return newUltimateOwner;
	}
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sInventoryTestFlag(
	INVENTORY * inv,
	int flag)
{
	return TESTBIT(&(inv->dwFlags), flag);
}


//----------------------------------------------------------------------------
// validate invloc table and build inventory table as part of excel loading
//----------------------------------------------------------------------------
BOOL ExcelInventoryPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(!table->m_bRowsAdded);

	EXCEL_TABLE * invloc_table = ExcelGetTableNotThreadSafe(DATATABLE_INVLOC);
	ASSERT_RETFALSE(invloc_table);

	INVLOC_DATA * prevloc = NULL;
	int nInventory = INVALID_ID;
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(invloc_table); ++ii)
	{
		INVLOC_DATA * invloc = (INVLOC_DATA *)ExcelGetDataPrivate(invloc_table, ii);
		if (!invloc)
		{
			continue;
		}

		// set weapon flag if applicable
		if (invloc->nWeaponIndex != INVALID_ID)
		{
			InvLocSetFlag(invloc, INVLOCFLAG_WEAPON);
		}

		// make sure locations are given in numerical order
 		ASSERT(invloc->nInventoryType != nInventory || (prevloc && invloc->nLocation > prevloc->nLocation));
		nInventory = MAX(nInventory, invloc->nInventoryType);

		// make sure that height and width are > 0
		ASSERT(invloc->nHeight > 0 && invloc->nWidth > 0);
		invloc->nHeight = MAX(invloc->nHeight, 1);
		invloc->nWidth = MAX(invloc->nWidth, 1);

		// make sure if we have a two dimensional inventory that either width or height is 1
		ASSERT(InvLocTestFlag(invloc, INVLOCFLAG_GRID) || invloc->nHeight == 1 || invloc->nWidth == 1);
		if (invloc->nHeight > 1 && invloc->nWidth > 1)
		{
			InvLocSetFlag(invloc, INVLOCFLAG_GRID);
		}
		prevloc = invloc;
	}
	nInventory++;

	// build inventory table
	ASSERT(table->m_Data.Count() == 0);
	int inventoryType = EXCEL_LINK_INVALID;

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(invloc_table); ++ii)
	{
		INVLOC_DATA * invloc = (INVLOC_DATA *)ExcelGetDataPrivate(invloc_table, ii);
		if (!invloc)
		{
			continue;
		}
		ASSERT_RETFALSE(invloc->nInventoryType >= 0);

		BOOL bFirstRow = FALSE;

		if (invloc->nInventoryType != inventoryType)
		{
			ASSERT_RETFALSE(invloc->nInventoryType >= inventoryType);
			inventoryType = invloc->nInventoryType;
			ASSERT_RETFALSE(table->AddRow(inventoryType));
			bFirstRow = TRUE;
		}
		BYTE * rowdata = (BYTE *)ExcelGetDataPrivate(table, inventoryType);
		ASSERT_RETFALSE(rowdata);
		INVENTORY_DATA * inventory = (INVENTORY_DATA *)rowdata;

		if (bFirstRow)
		{
			inventory->nFirstLoc = ii;
		}
		if (!InvLocTestFlag(invloc, INVLOCFLAG_DYNAMIC))
		{
			inventory->nNumInitLocs++;
		}
		inventory->nNumLocs++;
		ASSERT(inventory->nNumLocs < MAX_LOCATIONS);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const INVENTORY_DATA * InventoryGetData(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	int invindex = UnitGetInventoryType(unit);
	return InventoryGetData(UnitGetGame(unit), invindex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline INVENTORY * UnitGetInventory(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	INVENTORY * inv = unit->inventory;
	INVENTORY_CHECK_DEBUGMAGIC(inv);
	return inv;
}


//----------------------------------------------------------------------------
// retrieve inventory location data
//----------------------------------------------------------------------------
static inline const INVLOC_DATA * sInventoryGetInvLocData(
	GAME * game,
	INVENTORY * inv,
	int location)
{	
	ASSERT_RETNULL(inv);

	INVLOC_DATA key;
	key.nInventoryType = inv->nInventory;
	key.nLocation = location;
	return (const INVLOC_DATA *)ExcelGetDataByKey(EXCEL_CONTEXT(game), DATATABLE_INVLOC, &key, sizeof(key));
}


//----------------------------------------------------------------------------
// retrieve inventory location data
//----------------------------------------------------------------------------
static inline const INVLOC_DATA * sInventoryGetInvLocData(
	GAME * game,
	int inv,
	int location)
{	
	ASSERT_RETNULL(inv);
	ASSERT_RETNULL(location > 0);

	INVLOC_DATA key;
	key.nInventoryType = inv;
	key.nLocation = location;
	return (const INVLOC_DATA *)ExcelGetDataByKey(EXCEL_CONTEXT(game), DATATABLE_INVLOC, &key, sizeof(key));
}


//----------------------------------------------------------------------------
// retrieve inventory location data
//----------------------------------------------------------------------------
static inline const INVLOC_DATA * sUnitGetInvLocData(
	GAME * game,
	UNIT * unit,
	int location)
{
	if (unit->inventory)
	{
		return sInventoryGetInvLocData(game, unit->inventory, location);
	}

	return sInventoryGetInvLocData(game, UnitGetInventoryType(unit), location);
}


//----------------------------------------------------------------------------
// retrieve inventory location data
//----------------------------------------------------------------------------
const INVLOC_DATA * UnitGetInvLocData(
	UNIT * unit,
	int location)
{	
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETNULL(game);

	return sUnitGetInvLocData(game, unit, location);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD UnitGetInvLocCode(
	UNIT *pContainer,
	int nInvLocation)		
{	
	ASSERTX_RETINVALID( pContainer, "Expected container" );
	ASSERTX_RETINVALID( nInvLocation != INVLOC_NONE, "Invalid inventory location" );
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInvLocation );			
	ASSERTX_RETINVALID( pInvLocData, "Expected inv loc data" );
	const INVLOCIDX_DATA *pInvLocIdxData = InvLocIndexGetData( pInvLocData->nLocation );
	return pInvLocIdxData->wCode;
}

//----------------------------------------------------------------------------
// get a unit's size in inventory, return FALSE if it isn't an inv item
//----------------------------------------------------------------------------
BOOL UnitGetSizeInGrid(
	UNIT * unit,
	int * height,
	int * width,
	INVLOC * loc)
{
	ASSERT_RETFALSE(unit);
	ASSERT(height);
	ASSERT(width);

	if (loc && InvLocTestFlag(loc, INVLOCFLAG_ONESIZE))
	{
		*height = 1;
		*width = 1;
	}
	else
	{
		int w = UnitGetStat(unit, STATS_INVENTORY_WIDTH);
		int h = UnitGetStat(unit, STATS_INVENTORY_HEIGHT);
		*width = MAX(w, 1);
		*height = MAX(h, 1);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// allocate inv node for any units that can be contained in inventory
//----------------------------------------------------------------------------
void UnitInvNodeInit(
	UNIT * unit)
{
	UnitInvNodeFree(unit);

	unit->invnode = (INVENTORY_NODE*)GMALLOCZ(UnitGetGame(unit), sizeof(INVENTORY_NODE));

	unit->invnode->invloc = INVLOC_NONE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitInvNodeFree(
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	if (!unit->invnode)
	{
		return;
	}

	UnitInventoryRemove(unit, ITEM_ALL);

	GFREE(UnitGetGame(unit), unit->invnode);
	unit->invnode = NULL;
}


//----------------------------------------------------------------------------
// return the default inventory for the unit (index in INVENTORY_DATA)
//----------------------------------------------------------------------------
int UnitGetInventoryType(
	UNIT * unit)
{
	const UNIT_DATA * unit_data = UnitGetData(unit);
	if (!unit_data)
	{
		return INVALID_ID;
	}
	if( AppIsTugboat() && ItemIsACraftedItem(unit) )
	{
		return unit_data->nCraftingInventory;
	}
	return unit_data->nInventory;
}


//----------------------------------------------------------------------------
// copy inventory location info from INVLOC_DATA to INVLOC
//----------------------------------------------------------------------------
static void sInventoryCopyLocData(
	GAME * game,
	INVLOC * dest,
	const INVLOC_DATA * src)
{
	ASSERT_RETURN(dest);
	ASSERT_RETURN(src);

	memclear(dest, sizeof(INVLOC));
	dest->nLocation = src->nLocation;
	dest->nColorSetPriority = src->nColorSetPriority;
	dest->dwFlags = src->dwFlags;
	dest->nWeaponIndex = src->nWeaponIndex;
	dest->nFilter = src->nFilter;
	MemoryCopy(dest->nAllowTypes, INVLOC_UNITTYPES * sizeof(int), src->nAllowTypes, INVLOC_UNITTYPES * sizeof(int));
	MemoryCopy(dest->nUsableByTypes, INVLOC_UNITTYPES * sizeof(int), src->nUsableByTypes, INVLOC_UNITTYPES * sizeof(int));
	dest->nWidth = src->nWidth;
	dest->nHeight = src->nHeight;
	dest->nPreventLocation1 = src->nPreventLocation1;
	MemoryCopy(dest->nPreventTypes1, INVLOC_UNITTYPES * sizeof(int), src->nPreventTypes1, INVLOC_UNITTYPES * sizeof(int));
	// used to allocate grid here
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int invloccompare(
	const void * a,
	const void * b)
{
	const INVLOC * A = (INVLOC *)a;
	const INVLOC * B = (INVLOC *)b;

	if (A->nLocation == INVALID_LINK)
	{
		return 1;
	}
	if (B->nLocation == INVALID_LINK)
	{
		return -1;
	}
	if (A->nLocation < B->nLocation)
	{
		return -1;
	}
	if (A->nLocation > B->nLocation)
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventorySortLocs(
	INVENTORY * inv)
{
	qsort(inv->pLocs, inv->nLocs, sizeof(INVLOC), invloccompare);
}


//----------------------------------------------------------------------------
// allocate a container's inventory
//----------------------------------------------------------------------------
INVENTORY * UnitInventoryInit(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETNULL(game);
	
	//in some instances, such as an NPC getting pets via an init skill, the inventory is created - monsters added. And then at the end of 
	//monsterInit it recreates the inventory, and wipes out the NPC's pets.  Bad bad bad. Why free the inventory if it's already created -
	//just exit. Leaving for tugboat for now
	if( AppIsTugboat() &&
		unit->inventory )
	{
		return unit->inventory;
	}

	ASSERT(!unit->inventory);
	if (unit->inventory)
	{
		UnitInventoryFree(unit, FALSE);
	}

	int inventory_index = UnitGetInventoryType(unit);
	if (inventory_index <= 0)
	{
		return NULL;
	}

	const INVENTORY_DATA * inv_data = InventoryGetData(game, inventory_index);
	ASSERT_RETNULL(inv_data);

	INVENTORY * inv = (INVENTORY *)GMALLOCZ(UnitGetGame(unit), sizeof(INVENTORY));

	inv->dwDebugMagic = INVENTORY_DEBUG_MAGIC;
	inv->nInventory = inventory_index;
	inv->inv_data = inv_data;

	unit->inventory = inv;

	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ++ii)
	{
		unit->inventory->pWeapons[ii] = INVALID_ID;
	}

	ASSERT(inv_data->nNumLocs > 0);
	if (inv_data->nNumInitLocs != inv_data->nNumLocs)
	{
		SETBIT(&(inv->dwFlags), INVFLAG_DYNAMIC);
	}
	if (inv_data->nNumInitLocs <= 0)
	{
		return inv;
	}

	// create locations based on table data
	inv->nLocs = inv_data->nNumInitLocs;
	inv->pLocs = (INVLOC *)GMALLOCZ(UnitGetGame(unit), inv->nLocs * sizeof(INVLOC));
	for (int ii = 0; ii < inv->nLocs; ++ii)
	{
		inv->pLocs[ii].nLocation = INVALID_LINK;
	}

	INVLOC * loc = inv->pLocs;
	for (int ii = inv_data->nFirstLoc; ii < inv_data->nFirstLoc + inv_data->nNumLocs; ++ii)
	{
		const INVLOC_DATA * locdata = InvLocGetData(game, ii);
		if (!locdata)
		{
			continue;
		}
		if (!InvLocTestFlag(locdata, INVLOCFLAG_DYNAMIC))
		{
			sInventoryCopyLocData(UnitGetGame(unit), loc, locdata);
			loc++;
		}
	}
	sInventorySortLocs(inv);

	return inv;
}


//----------------------------------------------------------------------------
// just free the contents of an inventory
//----------------------------------------------------------------------------
void UnitInventoryContentsFree(
	struct UNIT * unit,
	BOOL bSendToClients)
{
	DWORD dwFlags = 0;
	if (bSendToClients)
	{
		dwFlags |= UFF_SEND_TO_CLIENTS;
	}
	
	UNIT * item = NULL;
	UNIT * next = UnitInventoryIterate(unit, NULL);
	while ((item = next) != NULL)
	{
		next = UnitInventoryIterate(unit, item);
		
		UnitFree(item, dwFlags);
	}
}

	
//----------------------------------------------------------------------------
// free inventory (including all contained items)
//----------------------------------------------------------------------------
void UnitInventoryFree(
	UNIT * unit,
	BOOL bSendToClients)
{
	ASSERT_RETURN(unit);
	if (!unit->inventory)
	{
		return;
	}

	INVENTORY * inv = unit->inventory;
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	// free all contained items
	while (inv->itemfirst)
	{
		// if we are sending to clients, we don't need to send for any inventory units
		// unless the units are physically in the world somewhere
		DWORD dwUnitFreeFlags = 0;
		if (bSendToClients == TRUE && UnitIsPhysicallyInContainer(inv->itemfirst) == FALSE)
		{
			dwUnitFreeFlags |= UFF_SEND_TO_CLIENTS;
		}
		
		UnitClearStat(inv->itemfirst, STATS_ITEM_LOCKED_IN_INVENTORY, 0);
		// free contained unit
		UnitFree(inv->itemfirst, dwUnitFreeFlags);
	}

	// free all location data
	INVLOC * loc = inv->pLocs;
	for (int ii = 0; ii < inv->nLocs; ii++)
	{
		if (loc->ppUnitGrid)
		{
			GFREE(UnitGetGame(unit), loc->ppUnitGrid);
		}
		loc++;
	}

	// free grid
	if (inv->pLocs)
	{
		GFREE(UnitGetGame(unit), inv->pLocs);
	}

	GFREE(UnitGetGame(unit), inv);
	unit->inventory = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline INVLOC * sInvClearLoc(
	INVLOC * locdata)
{
	ASSERT_RETNULL(locdata);
	memclear(locdata, sizeof(INVLOC));

	// a lot of calls do a compare on this and sort of assume 0 as an invalid value which it isn't.  So we need to set it to one.
	locdata->nLocation = INVALID_LINK;	
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const INVLOC_DATA * sInvFindDefaultDynamicLocData(
	GAME * game,
	int inventory_index,
	int location)
{
	const INVENTORY_DATA * inv_data = InventoryGetData(game, inventory_index);
	if (!inv_data)
	{
		return NULL;
	}
	if (inv_data->nNumInitLocs == inv_data->nNumLocs)		// no dynamic locations
	{
		return NULL;
	}
	for (int ii = inv_data->nFirstLoc; ii < inv_data->nFirstLoc + inv_data->nNumLocs; ii++)
	{
		const INVLOC_DATA * locdata = InvLocGetData(game, ii);
		if (!locdata)
		{
			continue;
		}
		if (locdata->nLocation == location)
		{
			return locdata;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryHasLocation(
	UNIT * unit,
	int location)
{	
	return (UnitGetInvLocData(unit, location) != NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasInventory(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);
	int inventory_index = UnitGetInventoryType(unit);
	if (inventory_index <= 0)
	{
		return FALSE;
	}

	const INVENTORY_DATA * inv_data = InventoryGetData(UnitGetGame(unit), inventory_index);
	return (inv_data != NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryGetArea(
	UNIT * unit,
	int location)
{
	ASSERT_RETZERO(unit);

	const INVLOC_DATA * locdata = UnitGetInvLocData(unit, location);
	ASSERT_RETZERO(locdata);

	if (InvLocTestFlag(locdata, INVLOCFLAG_DYNAMIC) && locdata->nSlotStat != INVALID_LINK)
	{
		return UnitGetStat(unit, locdata->nSlotStat, location);
	}
	return locdata->nHeight * locdata->nWidth;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static INVLOC * sInvGetDefaultLoc(
	UNIT * unit,
	int location,
	INVLOC * locdata)
{
	ASSERT_RETNULL(locdata);
	int inventory_index = UnitGetInventoryType(unit);
	if (inventory_index <= 0)
	{
		return sInvClearLoc(locdata);
	}
	const INVLOC_DATA * inv_data = sInvFindDefaultDynamicLocData(UnitGetGame(unit), inventory_index, location);
	if (!inv_data)
	{
		return sInvClearLoc(locdata);	// no dynamic locations
	}

	sInventoryCopyLocData(UnitGetGame(unit), locdata, inv_data);
	if (InvLocTestFlag(locdata, INVLOCFLAG_DYNAMIC) && inv_data->nSlotStat != INVALID_LINK)
	{
		locdata->nWidth = UnitGetStat(unit, inv_data->nSlotStat, location);
		ASSERT(locdata->nWidth >= locdata->nAllocatedWidth && locdata->nHeight >= locdata->nAllocatedHeight);
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline INVLOC * InventoryGetLoc(
	UNIT * unit,
	int location,
	INVLOC * locdata)
{
	ASSERT(unit);
	if (!unit)
	{
		return sInvClearLoc(locdata);
	}
	INVENTORY * inv = unit->inventory;
	if (!inv)
	{
		return sInvGetDefaultLoc(unit, location, locdata);
	}
	if (!inv->pLocs)
	{
		return sInvGetDefaultLoc(unit, location, locdata);
	}

	// do a binary search for the exact location, if we have one
	int min = 0;
	int max = inv->nLocs;
	int ii = (min + max)/2;

	while (min < max)
	{
		if (location < inv->pLocs[ii].nLocation)
		{
			max = ii;
		}
		else if (location > inv->pLocs[ii].nLocation)
		{
			min = ii + 1;
		}
		else
		{
			INVLOC * loc = inv->pLocs + ii;
			if (InvLocTestFlag(loc, INVLOCFLAG_DYNAMIC))
			{
				int inventory_index = UnitGetInventoryType(unit);
				if (inventory_index <= 0)
				{
					return loc;
				}
				const INVLOC_DATA * inv_data = sInvFindDefaultDynamicLocData(UnitGetGame(unit), inventory_index, location);
				if (!inv_data)
				{
					return loc;
				}
				if (inv_data->nSlotStat != INVALID_LINK)
				{
					loc->nWidth = UnitGetStat(unit, inv_data->nSlotStat, location);
				}
			}
			return loc;		// if we have an actual loc, don't copy it to locdata
		}
		ii = min + (max - min)/2;
	}

	// we have the unit's inventory, but we might not have the location yet
	return sInvGetDefaultLoc(unit, location, locdata);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InventoryExpandGrid(
	UNIT * unit,
	INVLOC * loc)
{
	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		return;
	}
	if (loc->nWidth <= loc->nAllocatedWidth && loc->nHeight <= loc->nAllocatedHeight)
	{
		return;
	}
	if (loc->nWidth <= 0 || loc->nHeight <= 0)
	{
		return;
	}
	if (!loc->ppUnitGrid)
	{
		ASSERT(loc->nAllocatedWidth == 0 && loc->nAllocatedHeight == 0);
		loc->nAllocatedWidth = loc->nWidth;
		loc->nAllocatedHeight = loc->nHeight;
		loc->ppUnitGrid = (UNIT **)GMALLOCZ(UnitGetGame(unit), loc->nWidth * loc->nHeight * sizeof(UNIT *));
		return;
	}
	ASSERT(loc->nAllocatedWidth > 0 && loc->nAllocatedHeight > 0);
	int newwidth = MAX(loc->nAllocatedWidth, loc->nWidth);
	int newheight = MAX(loc->nAllocatedHeight, loc->nHeight);
	int newsize = newwidth * newheight;
	int oldsize = loc->nAllocatedWidth * loc->nAllocatedHeight;
	ASSERT_RETURN(newsize >= oldsize);
	if (newsize == oldsize)
	{
		return;
	}
	loc->ppUnitGrid = (UNIT **)GREALLOCZ(UnitGetGame(unit), loc->ppUnitGrid, newsize * sizeof(UNIT *));
	// move everything accordingly
	for (int yy = 0; yy < loc->nAllocatedHeight; yy++)
	{
		for (int xx = 0; xx < loc->nAllocatedWidth; xx++)
		{
			loc->ppUnitGrid[yy * newwidth + xx] = loc->ppUnitGrid[yy * loc->nAllocatedWidth + xx];
		}
		memclear(loc->ppUnitGrid + yy * newwidth + loc->nAllocatedWidth, (newwidth - loc->nAllocatedWidth) * sizeof(UNIT *));
	}
	loc->nAllocatedWidth = newwidth;
	loc->nAllocatedHeight = newheight;
}


//----------------------------------------------------------------------------
// get the loc, create it or expand it if necessary... therefore should 
// only call this if we're actually adding something to the inventory
//----------------------------------------------------------------------------
INVLOC * InventoryGetLocAdd(
	UNIT * unit,
	int location)
{
	ASSERT_RETNULL(unit);
	INVENTORY * inv = unit->inventory;
	if (!inv)
	{
		inv = UnitInventoryInit(unit);
		if (!inv)
		{
			return NULL;
		}
	}
	INVLOC newloc;
	INVLOC * loc = InventoryGetLoc(unit, location, &newloc);
	if (loc)
	{
		InventoryExpandGrid(unit, loc);
		return loc;
	}

	ASSERT_RETNULL(sInventoryTestFlag(inv, INVFLAG_DYNAMIC));
	if (newloc.nLocation != location)
	{
		return NULL;
	}

	inv->nLocs++;
	inv->pLocs = (INVLOC *)GREALLOC(UnitGetGame(unit), inv->pLocs, inv->nLocs * sizeof(INVLOC));
	MemoryCopy(inv->pLocs + inv->nLocs - 1, sizeof(INVLOC), &newloc, sizeof(INVLOC));
	InventoryExpandGrid(unit, inv->pLocs + inv->nLocs - 1);
	sInventorySortLocs(inv);

	loc = InventoryGetLoc(unit, location, NULL);
	ASSERT(loc);
	return loc;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryGetLocInfo(
	UNIT * unit,
	int location,
	INVLOC_HEADER* info)
{
	ASSERT(info);
	ASSERT(unit);
	if (!unit)
	{
		memclear(info, sizeof(INVLOC_HEADER));
		return FALSE;
	}

	INVLOC defaultloc;
	INVLOC * loc = InventoryGetLoc(unit, location, &defaultloc);
	if (!loc)
	{
		loc = &defaultloc;
	}
	if (loc->nLocation != location)
	{
		memclear(info, sizeof(INVLOC_HEADER));
		return FALSE;
	}
	MemoryCopy(info, sizeof(INVLOC_HEADER), loc, sizeof(INVLOC_HEADER));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sGridClear(
	UNIT * item,
	INVLOC * loc)
{
	if (!loc->ppUnitGrid)
	{
		return;
	}

	INVENTORY_NODE* invnode = item->invnode;

	int ht, wd;
	BOOL fResult = UnitGetSizeInGrid(item, &ht, &wd, loc);
	ASSERT(fResult);
	if (!fResult)
	{
		return;
	}

	ASSERT_RETURN(loc->nWidth == loc->nAllocatedWidth && loc->nHeight == loc->nAllocatedHeight);
	ASSERT_RETURN(invnode->x >= 0 && invnode->x + wd <= loc->nAllocatedWidth);
	ASSERT_RETURN(invnode->y >= 0 && invnode->y + ht <= loc->nAllocatedHeight);

	for (int jj = invnode->y; jj < invnode->y + ht; jj++)
	{
		UNIT ** ptr = loc->ppUnitGrid + jj * loc->nAllocatedWidth + invnode->x;
		for (int ii = invnode->x; ii < invnode->x + wd; ii++)
		{
			ASSERT(*ptr == item);
			*ptr = NULL;
			ptr++;
		}
	}
	invnode->x = 0;
	invnode->y = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEquippedIsEquipped(
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item);

	INVENTORY * inventory = container->inventory;
	if (!inventory)
	{
		return FALSE;
	}

	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETFALSE(invnode)

	if (item == inventory->equippedfirst)
	{
		return TRUE;
	}

	if (invnode->container != container)
	{
		return FALSE;
	}

	return (invnode->equipprev != NULL);
}


//----------------------------------------------------------------------------
// add an item to the container's equipped list
//----------------------------------------------------------------------------
static void sEquippedAdd( 
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.newContainer);
	ASSERT_RETURN(cmd.item);
	ASSERT_RETURN(cmd.newInv);
	ASSERT_RETURN(cmd.invnode);

	ASSERT_RETURN(!sEquippedIsEquipped(cmd.newContainer, cmd.item));

	ASSERT_RETURN(cmd.invnode->equipprev == NULL && cmd.invnode->equipnext == NULL);

	cmd.invnode->equipprev = cmd.newInv->equippedlast;
	if (cmd.newInv->equippedlast)
	{
		cmd.newInv->equippedlast->invnode->equipnext = cmd.item;
	}
	else
	{
		cmd.newInv->equippedfirst = cmd.item;
	}
	cmd.newInv->equippedlast = cmd.item;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEquippedRemove( 
	UNIT * container, 
	UNIT * item)
{
	ASSERT_RETURN(container);
	ASSERT_RETURN(item);

	if (!sEquippedIsEquipped(container, item))
	{
		return;
	}

	INVENTORY * inventory = container->inventory;
	ASSERT_RETURN(inventory);

	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETURN(invnode);

	if (inventory->equippedfirst == item)
	{
		inventory->equippedfirst = invnode->equipnext;
	}
	if (inventory->equippedlast == item)
	{
		inventory->equippedlast = invnode->equipprev;
	}
	if (invnode->equipprev)
	{
		invnode->equipprev->invnode->equipnext = invnode->equipnext;
	}
	if (invnode->equipnext)
	{
		invnode->equipnext->invnode->equipprev = invnode->equipprev;
	}
	invnode->equipprev = NULL;
	invnode->equipnext = NULL;
}	

				
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveItemFromInventoryList(
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETURN(container);
	INVENTORY * inventory = container->inventory;
	ASSERT_RETURN(inventory);

	ASSERT_RETURN(item);
	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETURN(invnode)

	if (inventory->itemfirst == item)
	{
		inventory->itemfirst = invnode->invnext;
	}
	if (inventory->itemlast == item)
	{
		inventory->itemlast = invnode->invprev;
	}
	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ++ii)
	{
		if (inventory->pWeapons[ii] == UnitGetId( item ) )
		{
			inventory->pWeapons[ii] = INVALID_ID;
		}
	}
	if (invnode->invprev)
	{
		invnode->invprev->invnode->invnext = invnode->invnext;
	}
	if (invnode->invnext)
	{
		invnode->invnext->invnode->invprev = invnode->invprev;
	}
	invnode->invprev = NULL;
	invnode->invnext = NULL;
	inventory->nCount--;

	sEquippedRemove(container, item);

	invnode->container = NULL;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveItemFromLocationList(
	UNIT * container,
	UNIT * item,
	INVLOC * loc)
{
	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETURN(invnode);

	const INVLOC_DATA * invlocdata = UnitGetInvLocData(container, invnode->invloc);
	ASSERT_RETURN(invlocdata);

	if (InvLocTestFlag(invlocdata, INVLOCFLAG_EQUIPSLOT))
	{
		sEquippedRemove(container, item);
	}
	if (InvLocTestFlag(loc, INVLOCFLAG_WARDROBE))
	{
		UnitSetWardrobeChanged(container, TRUE);
	}
	
	if (loc->itemfirst == item)
	{
		loc->itemfirst = invnode->locnext;
	}
	if (loc->itemlast == item)
	{
		loc->itemlast = invnode->locprev;
	}
	if (invnode->locprev)
	{
		invnode->locprev->invnode->locnext = invnode->locnext;
	}
	if (invnode->locnext)
	{
		invnode->locnext->invnode->locprev = invnode->locprev;
	}
	loc->nCount--;

	invnode->locprev = NULL;
	invnode->locnext = NULL;
	invnode->invloc = INVLOC_NONE;
	UnitSetOwner(item, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemIsEquipped(
	UNIT * container,
	UNIT * item)
{
	return (StatsGetParent(item) == container);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsEquipped(
	UNIT * item,
	UNIT * container)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item);
	return sItemIsEquipped(container, item);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventoryRemoveItemStats(
	GAME * game,
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(container);
	ASSERT_RETURN(item);
	if (StatsGetParent(item) != container)
	{
		return;
	}

	SkillStopAll(game, container);

#if !ISVERSION(CLIENT_ONLY_VERSION)		
	if (IS_SERVER(game))
	{
		if (UnitIsA(container, UNITTYPE_PLAYER))
		{
			UnitTriggerEvent(game, EVENT_UNEQUIPED, container, item, NULL);					
			StateClearAllOfType(item, STATE_REMOVE_ON_UNEQUIP);		// remove any states that need to be on this event.
		}
	}
#endif
	StatsListRemove(item);
}


//----------------------------------------------------------------------------
// cleanup: update weapon configs, ui messages
//----------------------------------------------------------------------------
static void sInventoryRemoveCleanup(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.oldUltimateOwner);
	if (IS_SERVER(cmd.game))
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)		
		if (TEST_MASK(cmd.dwChangeLocationFlags, CLF_CONFIG_SWAP_BIT) == FALSE && UnitIsA(cmd.oldUltimateOwner, UNITTYPE_PLAYER))
		{
			if (cmd.oldLocation == INVLOC_WEAPONCONFIG ||
				cmd.oldLocation == GetWeaponconfigCorrespondingInvLoc(0) ||
				cmd.oldLocation == GetWeaponconfigCorrespondingInvLoc(1))
			{
				int nHotKey = -1;
				s_PlayerRemoveItemFromWeaponConfigs(cmd.oldUltimateOwner, cmd.item, nHotKey);
			}
		}
#endif
	} 
	else
	{
#if !ISVERSION(SERVER_VERSION)
		UNIT * control = GameGetControlUnit(cmd.game);
		if (control == cmd.oldUltimateOwner ||
			(control && TradeIsTradingWith(control, cmd.oldUltimateOwner)))
		{
			UISendMessage(WM_INVENTORYCHANGE, UnitGetId(cmd.oldContainer), cmd.oldLocation);
		}
#endif
	}
}


//----------------------------------------------------------------------------
// call this internally when we remove an item from a location before the
// statslist is removed
//----------------------------------------------------------------------------
static void sInventoryRemoveItemMessage(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	if (IS_CLIENT(cmd.game))
	{
		return;
	}
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(cmd.item);

	MSG_SCMD_REMOVE_ITEM_FROM_INV msg;
	msg.idItem = UnitGetId(cmd.item);

	for (GAMECLIENT * client = ClientGetFirstInGame(cmd.game); client; client = ClientGetNextInGame(client))
	{
		// only send to clients who should know the inventory of the owner
		if (UnitIsKnownByClient(client, cmd.item))		
		{
			s_SendUnitMessageToClient(cmd.item, ClientGetId(client), SCMD_REMOVE_ITEM_FROM_INV, &msg);
		}
	}
#endif
}


//----------------------------------------------------------------------------
// return TRUE if successful, FALSE if error (this will be a critical error)
//----------------------------------------------------------------------------
static BOOL sRemoveItemFromLocation(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETFALSE(cmd.isValid);

	if (!cmd.oldContainer)
	{
		return TRUE;
	}
	if (cmd.invnode->invloc == INVLOC_NONE)
	{
		return TRUE;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(cmd.oldContainer, cmd.invnode->invloc, &locdata);
	ASSERT_RETFALSE(loc);

	BOOL wasEquipped = sItemIsEquipped(cmd.oldContainer, cmd.item);
	if (wasEquipped || TEST_MASK(cmd.dwChangeLocationFlags, CLF_FORCE_SEND_REMOVE_MESSAGE))
	{
		if (IS_SERVER(cmd.game))
		{
			sInventoryRemoveItemMessage(cmd);
		}
	}

	sGridClear(cmd.item, loc);

	sRemoveItemFromLocationList(cmd.newContainer, cmd.item, loc);

	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ++ii)
	{
		if (cmd.newInv->pWeapons[ii] == UnitGetId(cmd.item))
		{
			cmd.newInv->pWeapons[ii] = INVALID_ID;
		}
	}

	if (wasEquipped)
	{
		sInventoryRemoveItemStats(cmd.game, cmd.oldContainer, cmd.item);
	}

	sInventoryRemoveCleanup(cmd);
	if( AppIsTugboat() )
	{
		if( cmd.game->eGameType == GAME_TYPE_CLIENT )
		{
			UnitCalculateVisibleDamage( GameGetControlUnit( cmd.game ) );
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// todo - why not have the client send the stash id and we just check if it's valid?
//----------------------------------------------------------------------------
static BOOL sInventoryCanUseStash(
	UNIT * unit)
{
	if (!unit)
	{
		return FALSE;
	}

	if (!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return FALSE;
	}

	UNITID idStashID = UnitGetStat(unit, STATS_LAST_STASH_UNIT_ID);
	if (idStashID == INVALID_ID)
	{
		return FALSE;
	}

	UNIT * unitStash = UnitGetById(UnitGetGame(unit), idStashID);
	if (!unitStash || UnitsGetDistanceSquared(unit, unitStash) > UNIT_INTERACT_DISTANCE_SQUARED)
	{
		s_StashClose(unit);
		return FALSE;
	}

	return TRUE;
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventorySwapWeaponSet(
	GAME * game,
	UNIT * container)
{
	ASSERT_RETFALSE(container);

	UNIT * CurrentItem[2];
	UNIT * SwapSetItem[2];
	const INVLOC_DATA * invlocdata = UnitGetInvLocData(container, INVLOC_LHAND);
	ASSERT_RETFALSE(invlocdata);

	CurrentItem[0] = UnitInventoryGetByLocation(container, INVLOC_LHAND);
	CurrentItem[1] = UnitInventoryGetByLocation(container, INVLOC_RHAND);
	SwapSetItem[0] = UnitInventoryGetByLocation(container, INVLOC_LHAND2);
	SwapSetItem[1] = UnitInventoryGetByLocation(container, INVLOC_RHAND2);

	DWORD dwFlags = 0;
	SET_MASK(dwFlags, CLF_CONFIG_SWAP_BIT);
	SET_MASK(dwFlags, CLF_IGNORE_REQUIREMENTS_ON_SWAP);

	if (SwapSetItem[0] && CurrentItem[0])
	{
		InventorySwapLocation(SwapSetItem[0], CurrentItem[0], -1, -1, dwFlags);
	}
	else if (SwapSetItem[0])
	{
		InventoryChangeLocation(container, SwapSetItem[0], INVLOC_LHAND, 0, 0, dwFlags);
	}
	else if (CurrentItem[0])
	{

		InventoryChangeLocation(container, CurrentItem[0], INVLOC_LHAND2, 0, 0, dwFlags);
	}

	if (SwapSetItem[1] && CurrentItem[1])
	{
		InventorySwapLocation(SwapSetItem[1], CurrentItem[1], -1, -1, dwFlags);
	}
	else if (SwapSetItem[1])
	{
		InventoryChangeLocation(container, SwapSetItem[1], INVLOC_RHAND, 0, 0, dwFlags);
	}
	else if (CurrentItem[1])
	{

		InventoryChangeLocation(container, CurrentItem[1], INVLOC_RHAND2, 0, 0, dwFlags);
	}
		
	s_InventoryChanged(container);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DROP_RESULT InventoryCanDropItem(
	UNIT * dropper,
	UNIT * item)
{
	ASSERT_RETVAL(dropper, DR_FAILED);

	if (!item)
	{
		return DR_FAILED;
	}

	if (!item->invnode)
	{
		return DR_FAILED;
	}

	if (IsUnitDeadOrDying(dropper))
	{
		return DR_FAILED;
	}

	// you can only drop items that you are the owner of
	UNITLOG_ASSERT_RETVAL(UnitGetUltimateContainer(item) == dropper, DR_FAILED, dropper);

	// you cannot drop an item that is in the active reward location
	if (UnitGetStat(item, STATS_IN_ACTIVE_OFFERING) == TRUE)
	{
		return DR_ACTIVE_OFFERING;
	}

	// if the item is in a slot that the player cannot normally take an item
	// from, like the reward or offering inv slots, the player cannot directly drop
	// from that slot either
	if (UnitIsPlayer(dropper))
	{
		UNIT * container = UnitGetContainer(item);
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation(item, &tInvLoc);
		if (InventoryLocPlayerCanTake(container, NULL, tInvLoc.nInvLocation) == FALSE)
		{
			return DR_FAILED;
		}
	}
	
	if (UnitIsNoDrop(item) || !QuestUnitCanDropQuestItem(item))
	{
		return DR_NO_DROP;
	}
	
	if (item->invnode->invloc == INVLOC_STASH && !sInventoryCanUseStash(UnitGetUltimateContainer(item)))
	{
		return DR_FAILED;
	}

	// it must be ok to drop but if it's a quest unit we need to destroy it
	if (AppIsTugboat() && QuestUnitIsUnitType_QuestItem(item))
	{
		return DR_OK_DESTROY;
	}

	// you can only drop items that you are the owner of
	UNITLOG_ASSERT_RETVAL(UnitGetUltimateContainer(item) == dropper, DR_FAILED, dropper);

	return DR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL InvLocIsWardrobeLocation(
	UNIT * unit,
	int loc)
{
	ASSERT_RETFALSE(unit);

	INVLOC_HEADER tLocInfo;
	ASSERT_RETFALSE(UnitInventoryGetLocInfo(unit, loc, &tLocInfo));

	return InvLocTestFlag(&tLocInfo, INVLOCFLAG_WARDROBE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DBOPERATION_CONTEXT_TYPE sInvContextGetDbOpContextType(
	INV_CONTEXT context)
{
	switch (context)
	{
		case INV_CONTEXT_TRADE:
			return DBOC_TRADE;
		case INV_CONTEXT_BUY:
			return DBOC_BUY;
		case INV_CONTEXT_SELL:
			return DBOC_SELL;
		case INV_CONTEXT_MAIL:
			return DBOC_MAIL;
		case INV_CONTEXT_DESTROY:
			return DBOC_DESTROY;
		case INV_CONTEXT_DISMANTLE:
			return DBOC_DISMANTLE;
		case INV_CONTEXT_DROP:
			return DBOC_DROP;
		case INV_CONTEXT_UPGRADE:
			return DBOC_UPGRADE;
		case INV_CONTEXT_NONE:
		default:
			return DBOC_INVALID;
	}
}

//----------------------------------------------------------------------------
// return the removed item or NULL on failure
//----------------------------------------------------------------------------
static UNIT * sUnitInventoryRemove(INV_FUNCARGS(
	UNIT * item,
	INVENTORY_REMOVE eRemove,
	DWORD dwChangeLocationFlags,
	BOOL bRemoveFromHotkeys,
	INV_CONTEXT eContext /*= INV_CONTEXT_NONE*/))
{
	ASSERT_RETNULL(item);
	GAME * game = UnitGetGame(item);	
	ASSERT_RETNULL(game);
	
	// if not in an inventory
	if (item->invnode == NULL)
	{
		return item;  // just return item as is
	}

	if (IS_SERVER(game) && UnitGetStat(item, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		return NULL;
	}

	INVENTORY_NODE * invnode = item->invnode;
	UNIT * container = invnode->container;
	if (!container)
	{
		return item;  // not contained in anything, just return the item
	}

	UnitLostItemDebug(item);
	
	INVENTORY * inv = container->inventory;
	ASSERT_RETNULL(inv);

	// is the item in a certain slots
	BOOL bIsInTradeLoc = InventoryIsInTradeLocation(item);
	REF(bIsInTradeLoc);
	BOOL wasEquipped = sItemIsEquipped(container, item);

	UNIT * ultimateOwner = UnitGetUltimateOwner(item);

	BOOL bWasClientControlUnitItem = FALSE;

#if !ISVERSION(SERVER_VERSION)	
	if (IS_CLIENT(game))
	{
		bWasClientControlUnitItem = (ultimateOwner == GameGetControlUnit(game));
	}
#endif
	
	// how many of these items are represented by this item structure
	int nQuantity = ItemGetQuantity(item);

	int nLocationRemovedFrom = INVALID_ID;
	UNIT * itemRemoved = NULL;
	if (eRemove == ITEM_ALL || nQuantity <= 1)
	{
		// take the entire item stack out of the inventory
		INVENTORY_COMMAND cmd(INV_FUNC_PASSFL(INV_CMD_REMOVE, container, item, -1, -1, -1, -1, dwChangeLocationFlags, TIME_ZERO, eContext));
		ASSERT_RETNULL(sRemoveItemFromLocation(cmd));
		nLocationRemovedFrom = cmd.oldLocation;

		if (InvLocIsWardrobeLocation(container, nLocationRemovedFrom))
		{
			UnitSetWardrobeChanged(container, TRUE);
		}

		UnitSetOwner(item, NULL);

		sRemoveItemFromInventoryList(container, item);

		itemRemoved = item;
		
		// remove this item from the database   
		if(!UnitTestFlag(ultimateOwner, UNITFLAG_JUSTFREED)) 
		{
			// save unit-related data so we'll have it when the logging occurs
			DBOPERATION_CONTEXT opContext(itemRemoved, ultimateOwner, sInvContextGetDbOpContextType(eContext));

			s_DatabaseUnitOperation(ultimateOwner, itemRemoved, DBOP_REMOVE, 0, DBUF_INVALID, &opContext, 0 );
		}
		
	}
	else
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		ASSERT_RETNULL(IS_SERVER(game));
		ASSERT_RETNULL(eRemove == ITEM_SINGLE);

		UnitGetInventoryLocation(item, &nLocationRemovedFrom);
		
		ItemSetQuantity(item, ItemGetQuantity(item) - 1);
				
		ASSERT_RETNULL(ultimateOwner);
		GAMECLIENTID idClient = UnitGetClientId(ultimateOwner);
		ASSERT_RETNULL(idClient != INVALID_GAMECLIENTID);

		// create a new item of the same class, note that this assumes that we do not
		// allow units to stack that have any other properties that are important,
		// like affixes or whatnot
		ITEM_SPEC tSpec;
		tSpec.nItemClass = UnitGetClass( item );
		tSpec.nItemLookGroup = UnitGetLookGroup( item );
		tSpec.flScale = UnitGetScale( item );
		tSpec.nLevel = UnitGetExperienceLevel( item );
		tSpec.nQuality = ItemGetQuality( item );
		itemRemoved = s_ItemSpawn( game, tSpec, NULL );
		ASSERTX_RETNULL( itemRemoved, "Could not create item from item stack split" );
		
		ItemSetQuantity(itemRemoved, 1);
#endif 
	}
	ASSERT_RETNULL(itemRemoved);
	
	if (IS_CLIENT(game))
	{
#if !ISVERSION(SERVER_VERSION)
		c_UnitSetMode(itemRemoved, MODE_IDLE);

		UNIT * ultimateContainer = UnitGetUltimateContainer(item);		
		UNIT * unitControl = GameGetControlUnit(game);
		if (ultimateContainer == unitControl || bWasClientControlUnitItem == TRUE ||
			(unitControl && TradeIsTradingWith(unitControl, ultimateContainer)))
		{
			UISendMessage(WM_INVENTORYCHANGE, UnitGetId(container), nLocationRemovedFrom);
		}
#endif
	} 
	else 
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)	
		if(!UnitTestFlag(itemRemoved, UNITFLAG_JUSTFREED))
		{
			s_UnitSetMode(itemRemoved, MODE_IDLE, FALSE);
		}

		// see if the item was in a (non-weapon config) hot key and if so replace or remove it.
		if (bRemoveFromHotkeys)
		{
			s_ItemRemoveUpdateHotkeys(game, ultimateOwner, itemRemoved);
		}
				
		// hooks for removing item from player inventory on server
		if (ultimateOwner &&
			!UnitTestFlag(ultimateOwner, UNITFLAG_JUSTFREED) &&
			UnitIsPlayer(ultimateOwner))
		{
			if (!TEST_MASK(dwChangeLocationFlags, CLF_SWAP_BIT))
			{
				s_TaskEventInventoryRemove(ultimateOwner, itemRemoved);
				if( AppIsHellgate() )
				{
					s_QuestsDroppedItem(ultimateOwner, UnitGetClass(itemRemoved));
				}
			}

			// make sure it's not cached on the client.  If it needs to be it will be set if it's put back somewhere else.
			GAMECLIENT *pClient = UnitGetClient(ultimateOwner);
			if (pClient)
			{
				ClientSetKnownFlag( pClient, item, UKF_CACHED, FALSE );
			}
		}
		
		// removing from a trade slot, tell trade system
		if (bIsInTradeLoc)
		{
			s_TradeOfferModified(ultimateOwner);
		}
		
		// removing from no save inv loc
		UnitSetFlag( item, UNITFLAG_IN_NOSAVE_INVENOTRY_SLOT, FALSE );

		// this is to update the client when an item is destroyed
		if (wasEquipped)
		{
			s_InventoryChanged(container);
		}
#endif 			
	}
	
	INVENTORY_TRACE2("[%s]  sUnitInventoryRemove() -- removed item [%d: %s] from container [%d: %s].  FILE:%s  LINE:%d", 
		CLTSRVSTR(game), UnitGetId(item), UnitGetDevName(item), UnitGetId(container), UnitGetDevName(container), file, line);
	return itemRemoved;	
}


//----------------------------------------------------------------------------
// return TRUE if successful, FALSE if error (this will be a critical error)
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryRemove)(INV_FUNCFLARGS(
	UNIT * item,
	INVENTORY_REMOVE eRemove,
	DWORD dwChangeLocationFlags,												// = 0
	BOOL bRemoveFromHotkeys,													// = TRUE
	INV_CONTEXT eContext))														// = INV_CONTEXT_NONE
{
	return sUnitInventoryRemove(item, eRemove, dwChangeLocationFlags, bRemoveFromHotkeys, eContext);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddItemToInventoryList(
	UNIT * item,
	UNIT * container)
{
	ASSERT_RETURN(item);
	ASSERT_RETURN(container);
	INVENTORY * inventory = container->inventory;
	ASSERT_RETURN(inventory);

	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETURN(invnode);

	invnode->invprev = inventory->itemlast;
	invnode->invnext = NULL;
	if (inventory->itemlast)
	{
		inventory->itemlast->invnode->invnext = item; 
	}
	else
	{
		inventory->itemfirst = item;
	}
	inventory->itemlast = item;

	invnode->container = container;
	inventory->nCount++;		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sInventorySkillsStopForContainer(
	INVENTORY_COMMAND & cmd)
{
	if (!cmd.bSkillsStoppedNewContainer)
	{
		SkillStopAll(cmd.game, cmd.newContainer);
		cmd.bSkillsStoppedNewContainer = TRUE;
		if (cmd.oldContainer == cmd.newContainer)
		{
			cmd.bSkillsStoppedOldContainer = TRUE;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sInventoryAddApplyStats(
	INVENTORY_COMMAND & cmd)
{
	if (cmd.newLoc->nFilter <= 0)
	{
		return;
	}
	sInventorySkillsStopForContainer(cmd);

	BOOL bCombat = !InvLocTestFlag(cmd.newLoc, INVLOCFLAG_WEAPON);
	StatsListAdd(cmd.newContainer, cmd.item, bCombat, cmd.newLoc->nFilter);
	cmd.newEquipped = TRUE;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sInventoryAddApplyStats(
	GAME * game,
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(container);
	ASSERT_RETURN(item);

	ASSERT_RETURN(UnitGetContainer(item) == container);

	if (StatsGetParent(item) == container)
	{
		return;
	}

	int location, x, y;
	ASSERT_RETURN(UnitGetInventoryLocation(item, &location, &x, &y));

	INVLOC * loc = InventoryGetLoc(container, location, NULL);
	ASSERT_RETURN(loc);
	
	if (loc->nFilter <= 0)
	{
		return;
	}

	SkillStopAll(game, container);

	BOOL bCombat = !InvLocTestFlag(loc, INVLOCFLAG_WEAPON);
	StatsListAdd(container, item, bCombat, loc->nFilter);
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInvLocIsEquipLocation( 
	INVLOC * loc,
	UNIT * container)
{
	REF(container);

	ASSERT_RETFALSE(loc);
	if (loc->nFilter > 0)
	{
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInvLocIsEquipLocationTolerant( 
	INVLOC * loc)
{
	if (!loc)
		return FALSE;

	if (loc->nFilter > 0)
	{
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsEquipLocation( 
	int nLocation,
	UNIT * container)
{
	if (nLocation == INVLOC_NONE)
	{
		return FALSE;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, nLocation, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == nLocation);

	return sInvLocIsEquipLocation(loc, container);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsNoDismantleLocation( 
	int nLocation,
	UNIT * container)
{
	if (nLocation == INVLOC_NONE)
	{
		return FALSE;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, nLocation, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == nLocation);

	const INVLOC_DATA * invloc_data = UnitGetInvLocData(container, loc->nLocation);
	return InvLocTestFlag(invloc_data, INVLOCFLAG_CANNOT_DISMANTLE_ITEMS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddItemToLocationListCommon(
	INVENTORY_COMMAND & cmd)
{
	cmd.invnode->invloc = cmd.newLoc->nLocation;
	cmd.newLoc->nCount++;

	// set owner to container ... we do *not* use the ultimate container because
	// we would have to update it when container was moved to/from inventory and world
	// *if* we need to have containment without explicit ownership we will have to
	// change this concept anyway
	UnitSetOwner(cmd.item, cmd.newContainer);

	if (InvLocTestFlag(cmd.newLocData, INVLOCFLAG_EQUIPSLOT) &&
		!TEST_MASK( cmd.dwChangeLocationFlags, CLF_DO_NOT_EQUIP ) )
	{
		sEquippedAdd(cmd);
	}
	if (InvLocTestFlag(cmd.newLocData, INVLOCFLAG_WARDROBE))
	{
		UnitSetWardrobeChanged(cmd.newContainer, TRUE);
	}

	if (AppIsHellgate() &&
		IS_SERVER(cmd.game))
	{
		if ((cmd.newContainer && !UnitTestFlag( cmd.newContainer, UNITFLAG_JUSTCREATED )) ||
			(cmd.oldContainer && !UnitTestFlag( cmd.oldContainer, UNITFLAG_JUSTCREATED )) )
		{
			if (cmd.item->pUnitData->pnAchievements)
			{
				if ((sInvLocIsEquipLocationTolerant(cmd.newLoc) || sInvLocIsEquipLocationTolerant(cmd.oldLoc)))
				{
					// send the equipping to the achievements system
					for (int i=0; i < cmd.item->pUnitData->nNumAchievements; i++)
					{
						if (cmd.newContainer && UnitIsPlayer(cmd.newContainer) && !UnitTestFlag( cmd.newContainer, UNITFLAG_JUSTCREATED ))
							s_AchievementsSendItemEquip(cmd.item->pUnitData->pnAchievements[i], cmd.item, cmd.newContainer);
						if (cmd.oldContainer && UnitIsPlayer(cmd.oldContainer) && !UnitTestFlag( cmd.oldContainer, UNITFLAG_JUSTCREATED ))
							s_AchievementsSendItemEquip(cmd.item->pUnitData->pnAchievements[i], cmd.item, cmd.oldContainer);
					}
				}
			}

			// 
			s_AchievementsSendInvMoveUndirected(cmd.newContainer, cmd.newLoc, cmd.oldContainer, cmd.oldLoc);

			if (cmd.newUltimateContainer && 
				cmd.newUltimateContainer == cmd.oldUltimateOwner &&
				UnitIsPlayer(cmd.newUltimateContainer) &&
				UnitIsA(cmd.newContainer, UNITTYPE_ITEM) &&
				UnitIsA(cmd.item, UNITTYPE_MOD))
			{
				s_AchievementsSendItemMod(cmd.newContainer, cmd.newUltimateContainer);
			}
		}
	}
}


//----------------------------------------------------------------------------
// add an item to the linked list for an inventory location
//----------------------------------------------------------------------------
static void sAddItemToLocationList(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	ASSERT_RETURN(cmd.item);
	ASSERT_RETURN(cmd.newContainer);
	ASSERT_RETURN(cmd.newLoc);
	ASSERT_RETURN(cmd.invnode);
	ASSERT_RETURN(cmd.newLocData);

	cmd.invnode->locprev = cmd.newLoc->itemlast;
	cmd.invnode->locnext = NULL;
	if (cmd.newLoc->itemlast)
	{
		cmd.newLoc->itemlast->invnode->locnext = cmd.item;
	}
	else
	{
		cmd.newLoc->itemfirst = cmd.item;
	}
	cmd.newLoc->itemlast = cmd.item;

	sAddItemToLocationListCommon(cmd);
}


//----------------------------------------------------------------------------
// add item to an indexed loc list
//----------------------------------------------------------------------------
static void sAddItemToLocationListIndexed(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	ASSERT_RETURN(cmd.item);
	ASSERT_RETURN(cmd.newContainer);
	ASSERT_RETURN(cmd.newLoc);
	ASSERT_RETURN(cmd.invnode);
	ASSERT_RETURN(cmd.newLocData);

	// find place in loc list to add me
	UNIT * prev = NULL;
	if (cmd.newLoc->itemfirst && cmd.newIndex > 0)
	{
		prev = cmd.newLoc->itemfirst;

		int count = 1;
		while (count < cmd.newIndex && prev->invnode->locnext)
		{
			count++;
			prev = prev->invnode->locnext;
		}
	}

	// add me to loc list
	cmd.invnode->locprev = prev;
	if (!prev)
	{
		cmd.invnode->locnext = cmd.newLoc->itemfirst;
		cmd.newLoc->itemfirst = cmd.item;
	}
	else
	{
		cmd.invnode->locnext = prev->invnode->locnext;
		prev->invnode->locnext = cmd.item;
	}
	if (!cmd.invnode->locnext)
	{
		cmd.newLoc->itemlast = cmd.item;
	}
	else
	{
		cmd.invnode->locnext->invnode->locprev = cmd.item;
	}

	sAddItemToLocationListCommon(cmd);
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL IndexListCanAdd(
	INVLOC * loc)
{
	if (loc->nWidth > 1)
	{
		if (loc->nCount >= loc->nWidth)
		{
			return FALSE;
		}
	}
	else
	{
		if (loc->nCount >= loc->nHeight)
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitPlayPickupSound(
	GAME * game,
	UNIT * container,
	UNIT * item)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(game);
	if (!IS_CLIENT(game))
	{
		return;
	}
	ASSERT_RETURN(item);
	const UNIT_DATA * item_data = ItemGetData(UnitGetClass(item));
	if (!item_data)
	{
		return;
	}
	if (item_data && item_data->m_nPickupSound != INVALID_ID)
	{
		if (!container)
		{
			return;
		}
		CLT_VERSION_ONLY(c_SoundPlay(item_data->m_nPickupSound, & c_UnitGetPosition(container), NULL));
	}
#else
	REF(game);
	REF(container);
	REF(item);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGridTest(
	INVLOC * loc,
	int x,
	int y,
	int wd,
	int ht,
	UNIT * item = NULL,
	UNIT * ignoreitem = NULL,
	UNIT * ignoreitem2 = NULL)
{
	ASSERT_RETFALSE(loc);
	ASSERT_RETFALSE(InvLocTestFlag(loc, INVLOCFLAG_GRID));

	// test that the item fits in the location
	if (x < 0 || x + wd > loc->nWidth ||
		y < 0 || y + ht > loc->nHeight)
	{
		return FALSE;
	}
	// maybe we haven't allocated the grid yet
	if (!loc->ppUnitGrid)
	{
		return TRUE;
	}
	if (y >= loc->nAllocatedHeight || x >= loc->nAllocatedWidth)
	{
		return TRUE;
	}

	// make sure it is the right "section" of the shared stash
	if (loc->nLocation == INVLOC_SHARED_STASH && item)
	{
		int nGameVariant = UnitGetStat(item, STATS_GAME_VARIANT);
		if (nGameVariant == -1)
		{
			UNIT * pPlayer = UnitGetUltimateContainer(item);
			if (pPlayer && UnitIsPlayer(pPlayer))
			{
				nGameVariant = (int)GameVariantFlagsGetStaticUnit(pPlayer);
			}
		}
		ASSERT_RETFALSE(nGameVariant >= 0);

		if (y < nGameVariant * SHARED_STASH_SECTION_HEIGHT)
		{
			if (UnitTestFlag(item, UNITFLAG_IN_DATABASE))
			{
				return FALSE;
			}
		}
		if (y + ht - 1 >= (nGameVariant + 1) * SHARED_STASH_SECTION_HEIGHT)
		{
			if (UnitTestFlag(item, UNITFLAG_IN_DATABASE))
			{
				return FALSE;
			}
		}
	}

	// test that there are no other items in the location (but only for allocated portions of the grid)
	int test_ht = MIN(y + ht, loc->nAllocatedHeight);
	int test_wd = MIN(x + wd, loc->nAllocatedWidth);

	for (int jj = y; jj < test_ht; jj++)
	{
		UNIT ** ptr = loc->ppUnitGrid + jj * loc->nAllocatedWidth + x;
		for (int ii = x; ii < test_wd; ii++)
		{
			if (*ptr != NULL && *ptr != ignoreitem && *ptr != ignoreitem2)
			{
				return FALSE;
			}
			ptr++;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// check for grid collision, ignore given item
//----------------------------------------------------------------------------
static BOOL sGridTestIgnoreItem(
	INVLOC * loc,
	int x,
	int y,
	UNIT * item,
	UNIT * ignoreitem)
{
	ASSERT_RETFALSE(loc);
	ASSERT_RETFALSE(InvLocTestFlag(loc, INVLOCFLAG_GRID));
	ASSERT_RETFALSE(item);

	int ht, wd;
	if (!UnitGetSizeInGrid(item, &ht, &wd, loc))
	{
		return FALSE;
	}

	return sGridTest(loc, x, y, wd, ht, item, ignoreitem, item);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGridMark(
	UNIT * item,
	INVLOC * loc,
	int x,
	int y,
	int wd,
	int ht)
{	
	ASSERT_RETFALSE(loc);
	ASSERT_RETFALSE(x >= 0 && y >= 0 && x + wd <= loc->nAllocatedWidth && y + ht <= loc->nAllocatedHeight);
	ASSERT_RETFALSE(loc->nWidth == loc->nAllocatedWidth && loc->nHeight == loc->nAllocatedHeight);
	for (int jj = y; jj < y + ht; jj++)
	{
		UNIT ** ptr = loc->ppUnitGrid + jj * loc->nAllocatedWidth + x;
		for (int ii = x; ii < x + wd; ii++)
		{
			*ptr = item;
			ptr++;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGridPlace(
	UNIT * item,
	INVLOC * loc,
	int x,
	int y,
	int wd,
	int ht)
{
	ASSERT_RETFALSE(item);
	INVENTORY_NODE * invnode = item->invnode;
	ASSERT_RETFALSE(invnode);

	if (sGridMark(item, loc, x, y, wd, ht) == FALSE)
	{
		return FALSE;
	}

	invnode->x = x;
	invnode->y = y;
	return TRUE;
}


//----------------------------------------------------------------------------
// create 3d inventory graphic
//----------------------------------------------------------------------------
static BOOL c_sShouldCreateInventoryGraphic(
	UNIT * item)
{	
	if (!AppIsHellgate())
	{
		return FALSE;
	}

	ASSERT_RETFALSE(item);
	GAME * game = UnitGetGame(item);
	ASSERT_RETFALSE(game);

	UNIT * ultimateOwner = UnitGetUltimateOwner(item);
	
	// check logic against our control unit
	UNIT * unitControl = GameGetControlUnit(game);
	if (unitControl == NULL)
	{
		return FALSE;
	}
	
	// we always create inventory graphics for stuff that is ours		
	if (ultimateOwner == unitControl)
	{
		return TRUE;
	}

	// we create graphics at units that we are trading with (npcs, other players, etc)
	if (TradeIsTradingWith(unitControl, ultimateOwner))
	{
		return TRUE;
	}
	
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sDoDatabaseAddOrUpdate(
	UNIT * item,
	UNIT * prevContainer,
	const INVENTORY_LOCATION * prevLoc,
	INV_CONTEXT context = INV_CONTEXT_NONE)
{
	ASSERT_RETURN(item);
	ASSERT_RETURN(prevLoc);

	GAME * game = UnitGetGame(item);
	if (IS_SERVER(game) == FALSE)
	{
		return;
	}

	// get owner of previous container (if any)	
	UNIT * prevOwner = NULL;
	if (prevContainer != NULL)
	{
		prevOwner = UnitGetUltimateOwner(prevContainer);
	}
		
	// where is the item contained now
	UNIT * currContainer = UnitGetContainer(item);
	ASSERT_RETURN(currContainer);

	UNIT * currOwner = UnitGetUltimateOwner(currContainer);
	ASSERT_RETURN(currOwner);
	
	// figure out what kind of operation we should do for the item in its new location
	DATABASE_OPERATION eOperationItem = DBOP_INVALID;
	DBOPERATION_CONTEXT_TYPE eOperationContext = sInvContextGetDbOpContextType(context);
	DBUNIT_FIELD eField = DBUF_INVALID;
	if (prevOwner != NULL && UnitIsPlayer(prevOwner))
	{
		// see if this item transitioned to/from a save/nosave inventory slot
		BOOL bInSavedSlotPrev = !UnitInventoryTestFlag( prevContainer, prevLoc->nInvLocation, INVLOCFLAG_DONTSAVE );
		BOOL bInSavedSlotCurr = !InventoryIsInNoSaveLocation( item );
	
		if (UnitIsPlayer(currOwner) == FALSE ||	 // from player to NPC, remove from DB
			(bInSavedSlotPrev == TRUE && bInSavedSlotCurr == FALSE))	// from saved slot to no save slot
		{
			eOperationItem = DBOP_REMOVE;	
		}
		else
		{
					
			// if the container has changed (putting a gem in a socket) then that gem
			// has been removed from the player inventory and put into the socket inventory,
			// which means we have to "add" it to the DB to compensate for the remove
			if (currContainer != prevContainer ||
				(bInSavedSlotPrev == FALSE && bInSavedSlotCurr == TRUE))
			{
				eOperationItem = DBOP_ADD;		// moving from inv of player to inv of item contained by player
			}
			else
			{
				ASSERTX( bInSavedSlotPrev == bInSavedSlotCurr == TRUE, "Item expected to be in a savable inventory location prev and current" );
				eOperationItem = DBOP_UPDATE;	// from player to player, update DB
				eField = DBUF_INVENTORY_LOCATION;  // update only the inventory location of this item
			}
		}
	}
	else
	{
		if (UnitIsPlayer(currOwner) == TRUE)
		{
			eOperationItem = DBOP_ADD;		// from NPC to player, add to DB
		}
		else
		{
			eOperationItem = DBOP_INVALID;  // from NPC to NPC if we use it someday, do nothing
		}
	}
	
	// do item operation
	if (eOperationItem != DBOP_INVALID)
	{
		if (s_DBUnitCanDoDatabaseOperation( UnitGetUltimateOwner( item ), item ) == TRUE)
		{
			// save unit-related data so we'll have it when the logging occurs
			// really only necessary for sells, because they don't go through sUnitInventoryRemove()
			DBOPERATION_CONTEXT opContext(item, prevOwner, eOperationContext);
			s_DatabaseUnitOperation(
				currOwner, 
				item, 
				eOperationItem, 
				s_GetDBUnitTickDelay(eField), 
				eField, 
				&opContext,
				0);
		}
	}

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsAllowedUnitInvTestSkillExemption(
	GAME * game,
	UNIT * container,
	UNIT * unit,
	INVLOC * loc,
	const INVLOC_DATA * invloc_data,
	DWORD flags)
{
	REF(flags);

	if (TESTBIT(invloc_data->dwFlags, INVLOCFLAG_SKILL_CHECK_ON_ULTIMATE_OWNER))
	{
		container = UnitGetUltimateOwner(container);
	}

	// TODO cmarch: this could be cleaner if the owner was set while the unit was being sent to the client
	if (IS_CLIENT(game) && TESTBIT(invloc_data->dwFlags, INVLOCFLAG_SKILL_CHECK_ON_CONTROL_UNIT))
	{
		container = GameGetControlUnit(game);
	}
		
	if (!container)
	{
		return FALSE;
	}

	// see if the unit has a skill which will give them an exemption
	for (unsigned int ii = 0; ii < INVLOC_NUM_ALLOWTYPES_SKILLS; ++ii)
	{
		if (!IsAllowedUnit(unit, invloc_data->nSkillAllowTypes[ii], INVLOC_UNITTYPES))
		{
			continue;
		}
		// it's the right type.  now see if they have the skill
		if (invloc_data->nAllowTypeSkill[ii] == INVALID_LINK)
		{
			continue;
		}
		const SKILL_DATA * skill_data = SkillGetData(game, invloc_data->nAllowTypeSkill[ii]);
		if (!skill_data)
		{
			continue;
		}
		if (UnitIsA(container, skill_data->nRequiredUnittype) &&
			SkillGetLevel(container, invloc_data->nAllowTypeSkill[ii]) >= invloc_data->nSkillAllowLevels[ii])
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsAllowedUnitInvTestUsableByType(
	GAME * game,
	UNIT * container,
	UNIT * unit,
	INVLOC * loc,
	const INVLOC_DATA * invloc_data,
	DWORD flags)
{
	REF(flags);

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);
	if (UnitIsA(UNITTYPE_ANY, unit_data->nContainerUnitTypes[0]))
	{
		return TRUE;
	}

	for (unsigned int ii = 0; ii < NUM_CONTAINER_UNITTYPES; ++ii)
	{
		if (unit_data->nContainerUnitTypes[ii] == 0)
		{
			return FALSE;
		}

		for (unsigned int jj = 0; jj < INVLOC_UNITTYPES; ++jj)
		{
			if (loc->nUsableByTypes[jj] == 0)
			{
				break;
			}

			if (UnitIsA(unit_data->nContainerUnitTypes[ii], loc->nUsableByTypes[jj]))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsAllowedUnitInv(
	UNIT * unit,
	INVLOC * loc,
	UNIT * container,
	DWORD dwChangeLocationFlags = 0)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(loc);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	// units that are being freed or are a part of units being freed 
	// can't go anywhere
	if (UnitTestFlagHierarchy( unit, UH_OWNER, UNITFLAG_FREED ) ||
		UnitTestFlagHierarchy( unit, UH_CONTAINER, UNITFLAG_FREED ) ||
		UnitTestFlagHierarchy( container, UH_OWNER, UNITFLAG_FREED ) ||
		UnitTestFlagHierarchy( container, UH_CONTAINER, UNITFLAG_FREED ))
	{
		return FALSE;
	}
	
	const INVLOC_DATA * invloc_data = UnitGetInvLocData(container, loc->nLocation);
	ASSERT_RETFALSE(invloc_data);

	// if the inventory location is an email location, and we always allow in email
	// slots we say it's OK
	if (InvLocTestFlag( invloc_data, INVLOCFLAG_EMAIL_LOCATION ) &&
		( (dwChangeLocationFlags & CLF_ALWAYS_ALLOW_IN_EMAIL ) ||
		   UnitGetStat( unit, STATS_ALWAYS_ALLOW_IN_EMAIL_SLOTS ) == TRUE) )
	{
		return TRUE;
	}
	
	// you cannot put items into reward slots unless the item has not yet been fully taken out of the offering
	if (invloc_data->bRewardLocation)
	{
		if (UnitGetStat(unit, STATS_IN_ACTIVE_OFFERING) == FALSE)
		{
			return FALSE;
		}
	}

	// you cannot put no trade items in trade slots
	if (UnitIsA(unit, UNITTYPE_ITEM) && ItemIsNoTrade(unit))
	{
		if (invloc_data->bTradeLocation)
		{
#if !ISVERSION(SERVER_VERSION)
			if (IS_CLIENT(game))
			{
				const WCHAR *szMsg = GlobalStringGet(GS_NOTRADE_HEADER);
				if (szMsg && szMsg[0])
				{
					UIShowQuickMessage(szMsg, 3.0f);
				}
				int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
				if (nSoundId != INVALID_LINK)
				{
					c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
				}
			}
#endif
			return FALSE;
		}

		if (InvLocTestFlag(invloc_data, INVLOCFLAG_CANNOT_ACCEPT_NO_TRADE_ITEMS))
		{
			return FALSE;
		}
	}

	// can't trade subscriber only items to non-subscribers...need to make sure to check recursively! - bmanegold
	if(invloc_data->bTradeLocation)
	{
		if(UnitIsSubscriberOnly(unit))
		{
			if(UnitIsPlayer(container))
			{
				UNIT *pTradingWith = TradeGetTradingWith(container);
				if(pTradingWith && UnitIsPlayer(pTradingWith))
				{
					if(!PlayerIsSubscriber(pTradingWith))
					{
#if !ISVERSION(SERVER_VERSION)
						if (IS_CLIENT(game))
						{
							const WCHAR *szMsg = GlobalStringGet(GS_SUBSCRIBER_ONLY_CHARACTER_HEADER);
							if (szMsg && szMsg[0])
							{
								UIShowQuickMessage(szMsg, 3.0f);
							}
							int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
							if (nSoundId != INVALID_LINK)
							{
								c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
							}
						}
#endif
						return FALSE;
					}
				}
			}
		}
	}

	// you cannot put no drop items in some slots
	if (TEST_MASK(dwChangeLocationFlags, CLF_IGNORE_NO_DROP_GRID_BIT) == FALSE &&
		InvLocTestFlag(invloc_data, INVLOCFLAG_CANNOT_ACCEPT_NO_DROP_ITEMS) && 
		UnitIsNoDrop(unit))
	{
		return FALSE;
	}

	// if we are not checking requirements, don't do the following checks.  This command means put it here and is 
	//    for trusted server actions.
	if (TEST_MASK(dwChangeLocationFlags, CLF_PICKUP_NOREQ) != FALSE)
	{
		return TRUE;
	}

	// check the allowed types
	if (!IsAllowedUnit(unit->nUnitType, loc->nAllowTypes, INVLOC_UNITTYPES))
	{
		if (!sIsAllowedUnitInvTestSkillExemption(game, container, unit, loc, invloc_data, dwChangeLocationFlags))
		{
			return FALSE;
		}
	}

	if (!UnitIsA(UNITTYPE_ANY, loc->nUsableByTypes[0]))
	{
		if (!sIsAllowedUnitInvTestUsableByType(game, container, unit, loc, invloc_data, dwChangeLocationFlags))
		{
			return FALSE;
		}
	}

	// check to see if this is an item that is restricted to certain classes
	if (sInvLocIsEquipLocation(loc, container))
	{
		if (TEST_MASK(dwChangeLocationFlags, CLF_IGNORE_UNIDENTIFIED_MASK) == FALSE &&
			ItemIsUnidentified(unit))
		{
			return FALSE;				// don't allow unidentified items in equip locations.
		}

		if (!InventoryItemMeetsClassReqs(unit, container))
		{
			return FALSE;
		}

		if (!InventoryItemHasUseableQuality(unit))
		{
			return FALSE;
		}
	}
		
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsAllowedUnitInv(
	UNIT * unit,
	const int location,
	UNIT * container,
	DWORD dwChangeLocationFlags /*= 0*/)
{
	ASSERT_RETFALSE(container);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	return sIsAllowedUnitInv(unit, loc, container, dwChangeLocationFlags);
}


//----------------------------------------------------------------------------
// call this when item changes location
//----------------------------------------------------------------------------
static void sInventoryChangeLocationMessage(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	if (IS_CLIENT(cmd.game))
	{
		return;
	}
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(cmd.item);
	ASSERT_RETURN(UnitGetContainer(cmd.item) == cmd.newContainer);

	// setup message			
	MSG_SCMD_CHANGE_INV_LOCATION msg;
	msg.idContainer = UnitGetId(cmd.newContainer);
	msg.idItem = UnitGetId(cmd.item);
	UnitGetInventoryLocation(cmd.item, &msg.tLocation);

#if ISVERSION(DEBUG_VERSION)
	BOOL bSent = FALSE;
	BOOL bClientsExist = FALSE;
#endif
	for (GAMECLIENT * client = ClientGetFirstInGame(cmd.game); client; client = ClientGetNextInGame(client))
	{
#if ISVERSION(DEBUG_VERSION)
		bClientsExist = TRUE;
#endif
		// only send to clients who should know the inventory of the owner
		if (ClientCanKnowUnit(client, cmd.item))		
		{
			if (UnitIsKnownByClient(client, cmd.item))
			{
				// send change location if client knew about the item prior to the move
				GAMECLIENTID idClient = ClientGetId(client);
				s_SendUnitMessageToClient(cmd.item, idClient, SCMD_CHANGE_INV_LOCATION, &msg);
#if ISVERSION(DEBUG_VERSION)
				bSent = TRUE;
#endif
			}
			else
			{
				// send new unit if the client didn't know about the item prior to the move
				DWORD dwUnitKnownFlags = 0;
				if (cmd.dwChangeLocationFlags & CLF_CACHE_ITEM_ON_SEND)
				{
					SETBIT( dwUnitKnownFlags, UKF_CACHED );
				}
				s_SendUnitToClient(cmd.item, client, dwUnitKnownFlags);
#if ISVERSION(DEBUG_VERSION)
				bSent = TRUE;
#endif
			}		
		}
		else if (UnitIsKnownByClient(client, cmd.item))
		{
			// remove the item if we used to know about the item but now we don't
			s_RemoveUnitFromClient(cmd.item, client, 0);
#if ISVERSION(DEBUG_VERSION)
			bSent = TRUE;
#endif
		}
	}
#if ISVERSION(DEBUG_VERSION)
	if (!bClientsExist)
	{
		return;
	}
	if ((!cmd.newContainer || !UnitIsPlayer(cmd.newContainer)) &&
		(!cmd.oldContainer || !UnitIsPlayer(cmd.oldContainer)))
	{
		return;
	}
	if (InventoryIsInServerOnlyLocation(cmd.item))
	{
		return;
	}
	//ASSERT(bSent);
#endif

#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventoryAddPetMessage(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	if (IS_CLIENT(cmd.game))
	{
		return;
	}
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(cmd.item);
	ASSERT_RETURN(cmd.newContainer);
	// tell the owner client they have a pet now
	if (UnitHasClient(cmd.newContainer))
	{
		MSG_SCMD_INVENTORY_ADD_PET msg;
		msg.idOwner = UnitGetId(cmd.newContainer);
		msg.idPet = UnitGetId(cmd.item);
		UnitGetInventoryLocation(cmd.item, &msg.tLocation);
		s_SendMessage(cmd.game, UnitGetClientId(cmd.newContainer), SCMD_INVENTORY_ADD_PET, &msg);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventoryMessage(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETURN(cmd.game);
	if (IS_CLIENT(cmd.game))
	{
		return;
	}
#if !ISVERSION(CLIENT_ONLY_VERSION)
	switch (cmd.cmd)
	{
	case INV_CMD_PICKUP:
		sInventoryChangeLocationMessage(cmd);
		break;
	case INV_CMD_PICKUP_NOREQ:
		sInventoryChangeLocationMessage(cmd);
		break;
	case INV_CMD_REMOVE:
		ASSERT_RETURN(0);
	case INV_CMD_MOVE:
		ASSERT_RETURN(0);
	case INV_CMD_DROP:
		ASSERT_RETURN(0);
	case INV_CMD_DESTROY:
		ASSERT_RETURN(0);
	case INV_CMD_ADD_PET:
		sInventoryAddPetMessage(cmd);
		break;
	case INV_CMD_REMOVE_PET:
		ASSERT_RETURN(0);
	default:
		ASSERT_RETURN(0);
	}
#endif
}

	
//----------------------------------------------------------------------------
// return FALSE if the item is in a stash and the container can't use that stash
//----------------------------------------------------------------------------
static inline BOOL sUnitInventoryAddCheckCantUseStash(
	INVENTORY_COMMAND & cmd)
{
	if (!IS_SERVER(cmd.game))
	{
		return TRUE;
	}
	if (!(cmd.newLocation == INVLOC_STASH || cmd.invnode->invloc == INVLOC_STASH))
	{
		return TRUE;
	}
	if (cmd.newUltimateContainer && !UnitTestFlag(cmd.newUltimateContainer, UNITFLAG_JUSTCREATED) &&
		!sInventoryCanUseStash(cmd.newUltimateContainer))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventoryTestAddGrid(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETFALSE(cmd.newX >= 0);
	ASSERT_RETFALSE(cmd.newY >= 0);

	if (!UnitGetSizeInGrid(cmd.item, &cmd.ht, &cmd.wd, cmd.newLoc))
	{
		INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- failed to get size of item [%d: %s].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), cmd.m_file, cmd.m_line);
		return FALSE;
	}

	if (!sGridTest(cmd.newLoc, cmd.newX, cmd.newY, cmd.wd, cmd.ht, cmd.item))
	{
		INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- tried to add item [%d: %s] to occupied space in container [%d: %s] grid location [%d: %s].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLocation, cmd.newLocData->szName, cmd.m_file, cmd.m_line);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventoryTestAddIndexed(
	INVENTORY_COMMAND & cmd)
{
	if (!IndexListCanAdd(cmd.newLoc))
	{
		INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- tried to add item [%d: %s] to full list location of container [%d: %s] location [%d: %s].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLocation, cmd.newLocData->szName, cmd.m_file, cmd.m_line);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventoryAddPrepareItem(
	INVENTORY_COMMAND & cmd)
{
	if (cmd.newLoc->nFilter > 0)
	{
		// you need to call this before the inventory changes - so that we know what skills to stop
		SkillStopAll(cmd.game, cmd.newContainer);
	}

	if (cmd.newLocData->bPhysicallyInContainer && UnitGetRoom(cmd.item) != NULL)
	{
		UnitRemoveFromWorld(cmd.item, FALSE);	
	} 
	else if ( cmd.newLocData->bPhysicallyInContainer && IS_CLIENT( cmd.item ) )
	{
		int nModelId = c_UnitGetModelId( cmd.item );
		if ( nModelId != INVALID_ID )
			e_ModelSetDrawAndShadow( nModelId, FALSE );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_sInventoryAddCleanupServer(
	INVENTORY_COMMAND & cmd)
{
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(cmd);
#else
	ASSERT(cmd.newUltimateContainer);
	if (UnitIsA(cmd.newUltimateContainer, UNITTYPE_PLAYER))
	{
		if (cmd.oldContainer != cmd.newContainer &&
			cmd.cmd != INV_CMD_PICKUP_NOREQ &&
			!TEST_MASK(cmd.dwChangeLocationFlags, CLF_SWAP_BIT))
		{
			s_QuestEventPickup(cmd.newUltimateContainer, cmd.item);
			s_TaskEventInventoryAdd(cmd.newUltimateContainer, cmd.item);		
		}

		if (cmd.newEquipped)
		{
			UnitTriggerEvent(cmd.game, EVENT_EQUIPED, cmd.newUltimateContainer, cmd.item, NULL);
		}
	}
	else
	{
		if (UnitInventoryCanBeRemoved(cmd.newContainer) == FALSE)
		{
			UnitSetStat(cmd.item, STATS_ITEM_LOCKED_IN_INVENTORY, 1);
		}
	}

	// if the item is/was in a trade slot, the offer is modified
	if (cmd.oldIsInTradeLocation || InventoryIsInTradeLocation(cmd.item))
	{
		s_TradeOfferModified(cmd.newUltimateContainer);
	}
	
	// if item was in reward slot, tell reward system
	if (UnitGetStat(cmd.item, STATS_IN_ACTIVE_OFFERING) && ItemIsInRewardLocation(cmd.item) == FALSE && ItemIsInCursor(cmd.item) == FALSE)
	{
		s_RewardItemTaken(cmd.newUltimateContainer, cmd.item);
	}

	// if item was removed from the email inbox, tell the email system
#if ISVERSION(SERVER_VERSION)
	if (cmd.bWasInEmailInbox == TRUE && InventoryIsInLocationWithFlag(cmd.item, INVLOCFLAG_EMAIL_INBOX) == FALSE)
	{
		UNIT *pPlayer = UnitGetUltimateContainer( cmd.item );
		if (pPlayer && UnitIsPlayer( pPlayer ))
		{
			s_PlayerEmailItemAttachmenTaken( pPlayer, cmd.item );
		}
	}
#endif  // ISVERSION(SERVER_VERSION)
		
	// if item has been placed in a "no save" location, we will set a flag on the unit
	// that says so (we do this, because to ask the question is an expensive operation
	// as we search for inv slots etc)
	BOOL bInNoSaveLoc = InventoryIsInNoSaveLocation(cmd.item);
	UnitSetFlag(cmd.item, UNITFLAG_IN_NOSAVE_INVENOTRY_SLOT, bInNoSaveLoc);
	
	if ( AppIsHellgate() && !TEST_MASK(cmd.dwChangeLocationFlags, CLF_CONFIG_SWAP_BIT) && 
		UnitIsA(cmd.newContainer, UNITTYPE_PLAYER) && !UnitTestFlag(cmd.newContainer, UNITFLAG_JUSTCREATED))
	{
		int nCurrentConfig = UnitGetStat(cmd.newContainer, STATS_CURRENT_WEAPONCONFIG);
		for (unsigned int ii = 0; ii < MAX_HOTKEY_ITEMS; ++ii)
		{
			if (cmd.newLocation == GetWeaponconfigCorrespondingInvLoc(ii))
			{
				s_WeaponconfigSetItem(cmd.newContainer, UnitGetId(cmd.item), nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1, ii);
			}
		}
	}	

	if (!InventoryIsInLocationType(cmd.item, LT_STANDARD))
	{
		s_ItemRemoveUpdateHotkeys(cmd.game, cmd.newContainer, cmd.item);
	}

	if (cmd.oldContainer)
	{
		GAMECLIENT *pClientOldContainer = UnitGetClient( cmd.oldContainer );
		
		if (pClientOldContainer)
		{
			// don't do this if we're not in game and ready to go yet	
			if (UnitCanSendMessages( cmd.oldContainer ))
			{
				// if the item is now in a cache clearing slot, clear any cache
				if (InventoryIsInLocationType( cmd.item, LT_CACHE_DISABLING ) )
				{
					// clear the cache block
					ClientSetKnownFlag( pClientOldContainer, cmd.item, UKF_CACHED, FALSE );

					// remove the unit if we can't know about it anymore as a result of clearing 
					// it's cached status
					if (ClientCanKnowUnit( pClientOldContainer, cmd.item ) == FALSE)
					{
						s_RemoveUnitFromClient( cmd.item, pClientOldContainer, 0 );
					}
				}
			}
		}
	}

	if (cmd.newContainer)
	{
		GAMECLIENT *pClientNewContainer = UnitGetClient( cmd.newContainer );
		if (pClientNewContainer)		
		{
			// don't do this if we're not in game and ready to go yet	
			if (UnitCanSendMessages( cmd.newContainer ))
			{
				// if the item is now in a cache enabling slot, but is not cached, do so now
				if (InventoryIsInLocationType( cmd.item, LT_CACHE_ENABLING ) )
				{
					// send to unit incase we don't have it yet
					s_SendUnitToClient( cmd.item, pClientNewContainer, UKF_CACHED );
					
					// keep it on the client
					ClientSetKnownFlag( pClientNewContainer, cmd.item, UKF_CACHED, TRUE );			
				}
			}
		}
	}
					
	if (cmd.wasEquipped || cmd.newEquipped || s_UnitNeedsWeaponConfigUpdate(cmd.newContainer))
	{
		if( cmd.item &&
			ItemBindsOnEquip( cmd.item ) )
		{
			s_ItemBindToPlayer( cmd.item );  //bind the item if need be
		}
		s_InventoryChanged(cmd.newContainer);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void c_sInventoryAddCleanupClient(
	INVENTORY_COMMAND & cmd)
{
#if ISVERSION(SERVER_VERSION)
	REF(cmd);
#else
	UNIT * control = GameGetControlUnit(cmd.game);

	// play pickup sound (ONLY the player character plays pickup sounds, and only if the item was on the ground. otherwise these play when you load a char)
	if (control == cmd.newContainer && cmd.item->fDropPosition > 0)
	{
		c_UnitPlayPickupSound(cmd.game, cmd.newContainer, cmd.item);
		cmd.item->fDropPosition = 0;
	}				

	if (c_sShouldCreateInventoryGraphic(cmd.item))
	{
		UIItemInitInventoryGfx(cmd.item, FALSE, FALSE);
	}
	UnitWaitingForInventoryMove(cmd.item, FALSE);
			
	ASSERT(cmd.newUltimateContainer);
	if (cmd.newUltimateContainer == control || (control && TradeIsTradingWith(control, cmd.newUltimateContainer)))
	{
		UISendMessage(WM_INVENTORYCHANGE, UnitGetId(cmd.newContainer), cmd.newLocation);

		// Update quest log when quest items are picked up.  Stacking items are covered by sUnitStatsChangeItemQuantity in units.cpp
		if (UnitDataTestFlag(cmd.item->pUnitData, UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP))
		{
			UIUpdateQuestLog(QUEST_LOG_UI_UPDATE_TEXT);
		}
	}

	// tugboat is ALWAYS third person.
	if (AppIsHellgate())
	{
		if (c_CameraGetViewType() == VIEW_1STPERSON && cmd.newContainer == control)
		{
			if (cmd.newLoc->nWeaponIndex != INVALID_ID && (UnitIsA(cmd.item, UNITTYPE_MELEE) || UnitIsA(cmd.item, UNITTYPE_SHIELD)))
			{	
				c_CameraSetViewType(VIEW_3RDPERSON);
			}
		}
	}

	// in case you are just moving weapons from right to left, but not changing the wardrobe,
	// we have to make the item draw again - it was turned off in UnitRemoveFromWorld
	ASSERT(cmd.GetNewUltimateOwner());
	if (WardrobeGetWeaponIndex(cmd.newUltimateOwner, UnitGetId(cmd.item)) != INVALID_ID)
	{
		if (AppIsHellgate() || UnitIsA(cmd.item, UNITTYPE_WEAPON))
		{
			int idModel = c_UnitGetModelIdThirdPerson(cmd.item);
			if (idModel != INVALID_ID)
			{
				e_ModelSetDrawAndShadow(idModel, TRUE);
			}
		}
	}

	if (cmd.newUltimateOwner == control && UnitIsA(cmd.item, UNITTYPE_ITEM))
	{
		const UNIT_DATA * unit_data = UnitGetData(cmd.item);
		if (unit_data)
		{
			for (unsigned int ii = 0; ii < NUM_UNIT_START_SKILLS; ++ii)
			{
				int skill = unit_data->nStartingSkills[ii];
				if (skill != INVALID_ID)
				{
					c_SkillLoad(cmd.game, skill);
				}
			}
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInventoryAddCleanup(
	INVENTORY_COMMAND & cmd)
{
	UNITMODE mode = MODE_ITEM_IN_INVENTORY;
	if (cmd.newLoc->nFilter > 0)
	{
		mode = MODE_ITEM_EQUIPPED_IDLE;
	}

	if (IS_SERVER(cmd.game))
	{
		s_UnitSetMode(cmd.item, mode, FALSE);	
	
		s_sInventoryAddCleanupServer(cmd);
	}
	else
	{
		c_UnitSetMode(cmd.item, mode);

		c_sInventoryAddCleanupClient(cmd);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitAddedToLostUnitLocation(
	UNIT *pUnit)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// record in a stat the current UTC time this unit was placed in the lost unit slot
	time_t utcNow= time(0);
	UnitSetLostAtTime( pUnit, utcNow );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL GRID>
static BOOL sUnitInventoryAddGridOrIndexed(
	INVENTORY_COMMAND & cmd)
{
	if (GRID)
	{
		if (!sUnitInventoryTestAddGrid(cmd))
		{
			return FALSE;
		}
	}
	else
	{
		if (!sUnitInventoryTestAddIndexed(cmd))
		{
			return FALSE;
		}
	}

	if (!sUnitInventoryAddPrepareItem(cmd))
	{
		return FALSE;
	}

	if (cmd.invnode->container != cmd.newContainer)
	{
		// note: message must be sent here, or stats will change in sUnitInventoryRemove
		// note: if after this point anything fails, the results are catastrophic!!!!
		// change containers
		UNIT * item = sUnitInventoryRemove(INV_FUNC_FLPARAM(cmd.m_file, cmd.m_line, cmd.item, ITEM_ALL, 0, TRUE, cmd.context));
		ASSERT(item == cmd.item)
		if (item != cmd.item)
		{
			INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- unable to remove item [%d: %s] from old inventory location.  FILE:%s  LINE:%d", 
				CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), cmd.m_file, cmd.m_line);
			return FALSE;
		}
		if (GRID)
		{
			sGridPlace(cmd.item, cmd.newLoc, cmd.newX, cmd.newY, cmd.wd, cmd.ht);
		}
		sAddItemToInventoryList(cmd.item, cmd.newContainer);
		if (!GRID)
		{
			sAddItemToLocationListIndexed(cmd);
		}
		else
		{
			sAddItemToLocationList(cmd);
		}
		sInventoryMessage(cmd);
		sInventoryAddApplyStats(cmd);
	}
	else if (cmd.newLocation != cmd.invnode->invloc)
	{
		// same container, different locations
		if (!sRemoveItemFromLocation(cmd))
		{
			INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- unable to remove item [%d: %s] from old inventory location.  FILE:%s  LINE:%d", 
				CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), cmd.m_file, cmd.m_line);
			return FALSE;
		}

		if (GRID)
		{
			sGridPlace(cmd.item, cmd.newLoc, cmd.newX, cmd.newY, cmd.wd, cmd.ht);
		}
		if (GRID)
		{
			sAddItemToLocationList(cmd);
		}
		else
		{
			sAddItemToLocationListIndexed(cmd);
		}
		sInventoryMessage(cmd);
		sInventoryAddApplyStats(cmd);
	}
	else
	{
		// same container, same location
		if (GRID)
		{
			sGridClear(cmd.item, cmd.newLoc);
			sGridPlace(cmd.item, cmd.newLoc, cmd.newX, cmd.newY, cmd.wd, cmd.ht);
		}
		else
		{
			sRemoveItemFromLocationList(cmd.newContainer, cmd.item, cmd.newLoc);
			sAddItemToLocationListIndexed(cmd);
		}
		sInventoryMessage(cmd);
	}

	if (cmd.tiEquipTime == TIME_ZERO)
	{
		cmd.item->invnode->m_tiEquipTime = AppCommonGetTrueTime();
	}
	else
	{
		cmd.item->invnode->m_tiEquipTime = cmd.tiEquipTime;
	}

	// do database updates for the item
#if !ISVERSION(CLIENT_ONLY_VERSION)
	INVENTORY_LOCATION loc;
	loc.nInvLocation = cmd.oldLocation;
	loc.nX = cmd.oldX;
	loc.nY = cmd.oldY;
	sDoDatabaseAddOrUpdate(cmd.item, cmd.oldContainer, &loc, cmd.context);
#endif

	// when adding to the lost unit inventory slot, we record the time
	INVENTORY_LOCATION tInvLocNew;
	if (cmd.item && UnitGetInventoryLocation( cmd.item, &tInvLocNew ))
	{
		if (tInvLocNew.nInvLocation == INVLOC_LOST_UNIT)
		{
			sUnitAddedToLostUnitLocation( cmd.item );
		}
	}
	
	if (GRID)
	{
		INVENTORY_TRACE2("[%s]  UnitInventoryAdd() -- added item [%d: %s] to container [%d: %s] grid location [%d: %s] position [%d, %d].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLoc->nLocation, cmd.newLocData->szName, cmd.newX, cmd.newY, cmd.m_file, cmd.m_line);
	}
	else
	{
		INVENTORY_TRACE2("[%s]  UnitInventoryAdd() -- added item [%d: %s] to container [%d: %s] list location [%d: %s] index[%d].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLoc->nLocation, cmd.newLocData->szName, cmd.newIndex, cmd.m_file, cmd.m_line);
	}

	sInventoryAddCleanup(cmd);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventoryAdd(
	INVENTORY_COMMAND & cmd)
{
	ASSERT_RETFALSE(cmd.isValid);
	ASSERT_RETFALSE(cmd.newLocation != INVLOC_NONE);

	if (!cmd.item || UnitTestFlag(cmd.item, UNITFLAG_FREED))
	{
		return FALSE;
	}
		
	if (!cmd.newContainer || UnitTestFlag(cmd.newContainer, UNITFLAG_FREED))
	{
		return FALSE;
	}
		
	if (IS_SERVER(cmd.game) && UnitGetStat(cmd.item, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		return FALSE;
	}

	if (!sUnitInventoryAddCheckCantUseStash(cmd))
	{
		return FALSE;
	}

	if (cmd.cmd == INV_CMD_PICKUP_NOREQ)
	{
		SET_MASK(cmd.dwChangeLocationFlags, CLF_PICKUP_NOREQ);
	}

	if (!sIsAllowedUnitInv(cmd.item, cmd.newLoc, cmd.newContainer, cmd.dwChangeLocationFlags))
	{
		INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- tried to add item [%d: %s] of improper type to container [%d: %s] location [%d: %s].  FILE:%s  LINE:%d", 
			CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLocation, cmd.newLocData->szName, cmd.m_file, cmd.m_line);
		return FALSE;
	}

	// Store the game variant with this item (used for shared stash)
	if (cmd.newUltimateContainer && UnitIsPlayer(cmd.newUltimateContainer) &&
		IS_SERVER(cmd.game) &&
//		!UnitTestFlag(cmd.newUltimateContainer, UNITFLAG_JUSTCREATED) &&
		UnitCanSendMessages(cmd.newUltimateContainer) &&
		UnitGetStat(cmd.item, STATS_GAME_VARIANT) == -1)
	{
		DWORD dwGameVariant = GameVariantFlagsGetStaticUnit(cmd.newUltimateContainer);
		ASSERT_GOTO(dwGameVariant <= 0xff, _skip); // STATS_GAME_VARIANT is only 8 bits for now
		UnitSetStat(cmd.item, STATS_GAME_VARIANT, (int)dwGameVariant);
		_skip:;
	}

	BOOL bUseIndex = (cmd.newIndex >= 0);
	BOOL bUseGrid = ((cmd.newX >= 0) && (cmd.newY >= 0));
	if (!cmd.newIsGrid)
	{
		bUseGrid = FALSE;
	}

	if (cmd.newIsGrid)
	{
		if (bUseIndex)
		{
			cmd.newX = cmd.newIndex;
			cmd.newY = 0;
			cmd.newIndex = -1;
			return sUnitInventoryAddGridOrIndexed<TRUE>(cmd);
		}
		if (!bUseGrid)
		{
			if (!UnitInventoryFindSpace(cmd.newContainer, cmd.item, cmd.newLocation, &cmd.newX, &cmd.newY))
			{
				INVENTORY_TRACE1("[%s]  ERROR: UnitInventoryAdd() -- tried to add item [%d: %s] to full grid location of container [%d: %s] location [%d: %s].  FILE:%s  LINE:%d", 
					CLTSRVSTR(cmd.game), UnitGetId(cmd.item), UnitGetDevName(cmd.item), UnitGetId(cmd.newContainer), UnitGetDevName(cmd.newContainer), cmd.newLocation, cmd.newLocData->szName, cmd.m_file, cmd.m_line);
				return FALSE;
			}
			cmd.newIndex = -1;
			return sUnitInventoryAddGridOrIndexed<TRUE>(cmd);
		}
		return sUnitInventoryAddGridOrIndexed<TRUE>(cmd);
	}
	else	// non-grid loc
	{
		return sUnitInventoryAddGridOrIndexed<FALSE>(cmd);
	}
}	


//----------------------------------------------------------------------------
// add an item to an inventory, if it's not a grid, add it as the last
// item in the inventory.  if it's a grid, find an empty location and insert
// it there
//----------------------------------------------------------------------------
BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,																// INV_CMD_PICKUP
	UNIT * container,
	UNIT * item,
	int location,
	DWORD dwChangeLocationFlags,												// = 0
	TIME tiEquipTime))															// = TIME_ZERO
{
	INVENTORY_COMMAND cmd(INV_FUNC_PASSFL(invcmd, container, item, location, -1, -1, -1, dwChangeLocationFlags, tiEquipTime, INV_CONTEXT_NONE));
	return sUnitInventoryAdd(cmd);
}


//----------------------------------------------------------------------------
// add item to an index list with no grid
//----------------------------------------------------------------------------
BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,																// INV_CMD_PICKUP
	UNIT * container,
	UNIT * item,
	int location,
	int index,
	DWORD dwChangeLocationFlags,												// = 0
	TIME tiEquipTime))															// = TIME_ZERO
{
	INVENTORY_COMMAND cmd(INV_FUNC_PASSFL(invcmd, container, item, location, index, -1, -1, dwChangeLocationFlags, tiEquipTime, INV_CONTEXT_NONE));
	return sUnitInventoryAdd(cmd);
}


//----------------------------------------------------------------------------
// add an item into inventory, this only works if the specified location is a
// grid
//----------------------------------------------------------------------------
BOOL INV_FUNCNAME(UnitInventoryAdd)(INV_FUNCFLARGS(
	INV_CMD invcmd,																// INV_CMD_PICKUP
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y,
	DWORD dwChangeLocationFlags,												// = 0
	TIME tiEquipTime,															// = TIME_ZERO
	INV_CONTEXT context /*= INV_CONTEXT_NONE*/))
{
	INVENTORY_COMMAND cmd(INV_FUNC_PASSFL(invcmd, container, item, location, -1, x, y, dwChangeLocationFlags, tiEquipTime, context));
	return sUnitInventoryAdd(cmd);
}


//----------------------------------------------------------------------------
// add an item into inventory, this only works if the specified location is a
// grid (Used with weapon configs)
//----------------------------------------------------------------------------
BOOL INV_FUNCNAME(UnitWeaponConfigInventoryAdd)(INV_FUNCARGS_NODEFAULT(
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y))
{
	ASSERT_RETFALSE(IS_SERVER(UnitGetGame(container)));
	INVENTORY_COMMAND cmd(INV_FUNC_PASSFL(INV_CMD_PICKUP, container, item, location, -1, x, y, CLF_CONFIG_SWAP_BIT, TIME_ZERO, INV_CONTEXT_NONE));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsContainedBy(
	UNIT * unit,
	UNIT * container)
{
	if (unit == NULL || container == NULL)
	{
		return FALSE;
	}
	
	// get inventory node
	INVENTORY_NODE * invnode = unit->invnode;
	if (invnode == NULL)
	{
		return FALSE;  // not contained by anything
	}

	// search up containment chain
	while (invnode != NULL && invnode->container != NULL)
	{
		// is this the container
		if (invnode->container == container)
		{
			return TRUE;
		}		
		
		// go up containment chain
		invnode = invnode->container->invnode;	
	}
	
	return FALSE;		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetUltimateContainer(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	
	UNIT * container = unit;
	
	while (1) 
	{
		UNIT * containerContainer = UnitGetContainer(container);
		if (containerContainer == NULL)
		{
			return container;
		}
		
		container = containerContainer;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetContainer(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	INVENTORY_NODE * invnode = unit->invnode;
	if (invnode == NULL)
	{
		return NULL;
	}
	return invnode->container;
}
		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitInventoryGetByIndex(
	UNIT * container,
	int index)
{
	if (index < 0 || index >= UnitInventoryGetCount(container))
	{
		return NULL;
	}
	UNIT * item = NULL;
	UNIT * next = UnitInventoryIterate(container, NULL, 0, FALSE);
	int count = -1;
	while ((item = next) != NULL)
	{
		next = UnitInventoryIterate(container, item, 0, FALSE);
		count++;

		if (count == index)
		{
			return item;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UNIT * INV_FUNCNAME(UnitInventoryGetByLocation)(INV_FUNCARGS_NODEFAULT(
	struct UNIT * container,
	int location))
{
#if	INVENTORY_DEBUG
	FL_ASSERT_RETNULL(container, file, line);
#else
	ASSERT_RETNULL(container);
#endif
	if (!container->inventory)
	{
		return NULL;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
#if	INVENTORY_DEBUG
	FL_ASSERT_RETNULL(!InvLocTestFlag(loc, INVLOCFLAG_GRID), file, line);
#endif
	return loc->itemlast;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndIndex)(INV_FUNCARGS_NODEFAULT(
	UNIT * container,
	int location,
	int index))
{
#if	INVENTORY_DEBUG
	FL_ASSERT_RETNULL(container, file, line);
#else
	ASSERT_RETNULL(container)
#endif
	if (!container->inventory)
	{
		return NULL;
	}
#if	INVENTORY_DEBUG
	INVENTORY * inv = container->inventory;
#endif

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
#if	INVENTORY_DEBUG
		if (!sInventoryTestFlag(inv, INVFLAG_DYNAMIC))
		{
			TraceDebugOnly("UnitInventoryGetByLocationAndIndex() ERROR  location:%d  index:%d\n", location, index);
		}
		FL_ASSERT_RETNULL(sInventoryTestFlag(inv, INVFLAG_DYNAMIC), file, line);
#endif
		return NULL;
	}
	if (loc->ppUnitGrid)
	{
		return UnitInventoryGetByLocationAndXY(container, location, index, 0);
	}

	UNIT * iter = loc->itemfirst;
	int count = 0;
	while (iter && count < index)
	{
		count++;
		iter = iter->invnode->locnext;
	}
	return iter;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndXY)(INV_FUNCARGS_NODEFAULT(
	UNIT * container,
	int location,
	int x,
	int y))
{
#if	INVENTORY_DEBUG
	FL_ASSERT_RETNULL(container, file, line);
#else
	ASSERT_RETNULL(container);
#endif
	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return NULL;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		return NULL;
	}
	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
#if	INVENTORY_DEBUG
		FL_ASSERT(y == 0, file, line);
#endif
		return UnitInventoryGetByLocationAndIndex(container, location, x);
	}

	if (x >= loc->nAllocatedWidth || y >= loc->nAllocatedHeight)
	{
		return NULL;
	}
	ASSERT_RETNULL(loc->ppUnitGrid);

	UNIT ** ptr = loc->ppUnitGrid + y * loc->nAllocatedWidth + x;
	UNIT * item = *ptr;
	if (!item)
	{
		if (InvLocTestFlag(loc, INVLOCFLAG_ONESIZE))
		{
			return NULL;
		}
		else
		{
			// the items in this grid have varying sizes.  We need to see if we're looking at a grid square
			//  that's in the item but not in the upper-left corner.
			UNIT * item = NULL;
			while ((item = UnitInventoryLocationIterate(container, location, item, 0, FALSE)) != NULL)
			{
				ASSERT_CONTINUE(item->invnode);

				int nWidth, nHeight;
				UnitGetSizeInGrid(item, &nHeight, &nWidth, loc);
				if (x >= item->invnode->x && 
					x < item->invnode->x + nWidth &&
					y >= item->invnode->y && 
					y < item->invnode->y + nHeight)
				{
					return item;
				}
			}

			return NULL;
		}
	}

#if	INVENTORY_DEBUG
	FL_ASSERT(item->invnode, file, line);
	FL_ASSERT(item->invnode->container == container, file, line);
	FL_ASSERT(item->invnode->invloc == location, file, line);
#endif

	return item;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryGetByLocationAndUnitId)(INV_FUNCARGS_NODEFAULT(
	UNIT * container,
	int location,
	UNITID unitid))
{
#if	INVENTORY_DEBUG
	FL_ASSERT_RETNULL(container, file, line);
#else
	ASSERT_RETNULL(container)
#endif
	if (!container->inventory)
	{
		return NULL;
	}
#if	INVENTORY_DEBUG
	INVENTORY * inv = container->inventory;
#endif

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
#if	INVENTORY_DEBUG
		if (!sInventoryTestFlag(inv, INVFLAG_DYNAMIC))
		{
			TraceDebugOnly("UnitInventoryGetByLocationAndUnitId() ERROR  location:%d  unitid:%d\n", location, unitid);
		}
		FL_ASSERT_RETNULL(sInventoryTestFlag(inv, INVFLAG_DYNAMIC), file, line);
#endif
		return NULL;
	}

	UNIT * iter = loc->itemfirst;
	int count = 0;
	while (iter && count < loc->nCount)
	{
		if (iter->unitid == unitid)
		{
			return iter;
		}
		count++;
		iter = iter->invnode->locnext;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int InventoryGridGetItemsOverlap(
	UNIT * container,
	UNIT * pItemToCheck,
	UNIT *& pOverlappedItem,
	int location,
	int x,
	int y)
{
	pOverlappedItem = NULL;

	ASSERT_RETZERO(container);
	ASSERT_RETZERO(pItemToCheck);

	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return 0;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	ASSERT_RETZERO(loc);

	ASSERT_RETZERO(InvLocTestFlag(loc, INVLOCFLAG_GRID));

	if (x >= loc->nAllocatedWidth || y >= loc->nAllocatedHeight)
	{
		return 0;
	}

	int nCheckWidth, nCheckHeight;
	UnitGetSizeInGrid(pItemToCheck, &nCheckHeight, &nCheckWidth, loc);

	int nOverlapCount = 0;
	UNIT * item = NULL;
	while ((item = UnitInventoryLocationIterate(container, location, item, 0, FALSE)) != NULL)
	{
		int nWidth, nHeight;
		UnitGetSizeInGrid(item, &nHeight, &nWidth, loc);
		if (x >= item->invnode->x + nWidth ||
			x + nCheckWidth <= item->invnode->x ||
			y >= item->invnode->y + nHeight ||
			y + nCheckHeight <= item->invnode->y)
		{
			continue;
		}

		nOverlapCount++;
		pOverlappedItem = item;
	}

	return nOverlapCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsUnitPreventedFromInvLoc(
	UNIT * container,
	UNIT * item,
	INVLOC * loc,
	int * pnPreventingLoc = NULL)
{
	if (pnPreventingLoc)
	{
		*pnPreventingLoc = INVLOC_NONE;
	}

	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(loc);

	UNIT * pUnitInPrevLoc = UnitInventoryGetByLocation(container, loc->nPreventLocation1);
	if (pUnitInPrevLoc)
	{
		INVLOC locdata;
		INVLOC * preventloc = InventoryGetLoc(container, loc->nPreventLocation1, &locdata);
		if (!preventloc)
		{
			preventloc = &locdata;
		}
		ASSERT_RETFALSE(preventloc->nLocation == loc->nPreventLocation1);

		if (IsAllowedUnit(pUnitInPrevLoc, loc->nPreventTypes1, INVLOC_UNITTYPES))
		{
			// Return which location is blocking
			if (pnPreventingLoc)
			{
				*pnPreventingLoc = preventloc->nLocation;
			}
			return TRUE;
		}
	}
	
	// Check the other locations to see if this item in this slot would invalidate them
	INVENTORY * inv = container->inventory;
	if (inv)
	{
		for (int ii=0; ii < inv->nLocs; ii++)
		{
			if (inv->pLocs[ii].nPreventLocation1 == loc->nLocation &&
				inv->pLocs[ii].itemlast != NULL)
			{
				if (IsAllowedUnit(item, inv->pLocs[ii].nPreventTypes1, INVLOC_UNITTYPES))
				{
					// Return which location is blocking
					if (pnPreventingLoc)
					{
						*pnPreventingLoc = inv->pLocs[ii].nLocation;
					}
					return TRUE;
				}			
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsUnitPreventedFromInvLoc(
	UNIT * container,
	UNIT * item,
	int nLocation,
	int *pnPreventingLoc /*= NULL*/)
{
	ASSERT_RETFALSE(container && item);
	if (pnPreventingLoc)
	{
		*pnPreventingLoc = INVLOC_NONE;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, nLocation, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == nLocation);

	return IsUnitPreventedFromInvLoc(container, item, loc, pnPreventingLoc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsUnitPreventedFromInvLocIgnoreItem(
	UNIT * container,
	UNIT * item,
	UNIT * ignore_item,
	int nLocation)
{
	ASSERT_RETFALSE(container && item);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, nLocation, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == nLocation);

	UNIT * prevent_unit = UnitInventoryGetByLocation(container, loc->nPreventLocation1);
	if (prevent_unit && prevent_unit != ignore_item)
	{
		INVLOC locdata;
		INVLOC * preventloc = InventoryGetLoc(container, loc->nPreventLocation1, &locdata);
		if (!preventloc)
		{
			preventloc = &locdata;
		}
		ASSERT_RETFALSE(preventloc->nLocation == loc->nPreventLocation1);

		if (IsAllowedUnit(prevent_unit, loc->nPreventTypes1, INVLOC_UNITTYPES))
		{
			return TRUE;
		}
	}
	
	// Check the other locations to see if this item in this slot would invalidate them
	INVENTORY * inv = container->inventory;
	if (inv)
	{
		for (int ii=0; ii < inv->nLocs; ii++)
		{
			if (inv->pLocs[ii].nPreventLocation1 == loc->nLocation &&
				inv->pLocs[ii].itemlast != NULL)
			{
				if (IsAllowedUnit(item, inv->pLocs[ii].nPreventTypes1, INVLOC_UNITTYPES))
				{
					return TRUE;
				}			
			}
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryTest(
	UNIT * container,
	UNIT * item,
	int location,
	int x /*= 0*/,
	int y /*= 0*/,
	DWORD dwChangeLocationFlags /*= 0*/)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item && item->invnode);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		if (locdata.nLocation == INVALID_LINK)
		{
			return FALSE;
		}
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	// is this item allowed in this inventory slot?
	if (!sIsAllowedUnitInv(item, loc, container, dwChangeLocationFlags))
	{
		return FALSE;
	}

	// Is there something in another inventory slot (two-hander) that would prevent this item
	//  from going in this slot?
	if (IsUnitPreventedFromInvLoc(container, item, loc))
	{
		return FALSE;
	}

	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		ASSERT_RETFALSE(x == 0 && y == 0);
		return (UnitInventoryGetByLocation(container, location) == NULL);
	}

	int ht, wd;
	BOOL fResult = UnitGetSizeInGrid(item, &ht, &wd, loc);
	if (!fResult)
	{
		return FALSE;
	}

	return sGridTest(loc, x, y, wd, ht, item);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryTestIgnoreExisting(
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y)
{
	if (location == INVALID_LINK)
		return FALSE;

	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item && item->invnode);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	if (!sIsAllowedUnitInv(item, loc, container))
	{
		return FALSE;
	}
	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		ASSERT_RETFALSE(x == 0 && y == 0);
		return TRUE;
	}

	int ht, wd;
	BOOL fResult = UnitGetSizeInGrid(item, &ht, &wd, loc);
	if (!fResult)
	{
		return FALSE;
	}

	// test that the item fits in the location
	return (x >= 0 && x + wd <= loc->nWidth &&
		y >= 0 && y + ht <= loc->nHeight);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryTestIgnoreItem(
	UNIT * container,
	UNIT * item,
	UNIT * ignoreitem,
	int location,
	int x,
	int y)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item && item->invnode);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	if (loc->nLocation != location)
	{
		return FALSE;
	}

	if (!sIsAllowedUnitInv(item, loc, container))
	{
		return FALSE;
	}

	// Is there something in another inventory slot (two-hander) that would prevent this item
	//  from going in this slot?
	if (IsUnitPreventedFromInvLocIgnoreItem(container, item, ignoreitem, location))
	{
		return FALSE;
	}

	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		ASSERT_RETFALSE(x == 0 && y == 0);
		UNIT * curitem = UnitInventoryGetByLocation(container, location);
		return (!curitem || curitem == ignoreitem || curitem == item);
	}

	return sGridTestIgnoreItem(loc, x, y, item, ignoreitem);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryFindSpace(
	UNIT * container,
	UNIT * item,
	int location,
	int* x,
	int* y,
	DWORD dwChangeLocationFlags /*= 0*/,
	UNIT *pIgnoreItem /*= NULL*/,
	DWORD dwItemEquipFlags /*= 0*/ )
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(item && item->invnode);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	if (InvLocTestFlag(loc, INVLOCFLAG_RESERVED))
	{
		return FALSE;
	}
	
	if (!sIsAllowedUnitInv(item, loc, container, dwChangeLocationFlags))
	{
		return FALSE;
	}

	// Is there something in another inventory slot (two-hander) that would prevent this item
	//  from going in this slot?
	if (IsUnitPreventedFromInvLoc(container, item, loc))
	{
		return FALSE;
	}

	if (!InventoryItemCanBeEquipped(container, item, location, NULL, 0, dwItemEquipFlags))
	{
		return FALSE;
	}

	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		if (x) *x = 0;
		if (y) *y = 0;
		UNIT *pUnit = UnitInventoryGetByLocation(container, location);
		return ( pUnit == NULL );
	}

	int ht, wd;
	BOOL fResult = UnitGetSizeInGrid(item, &ht, &wd, loc);
	if (!fResult)
	{
		return FALSE;
	}

	if (!(ht <= loc->nHeight && wd <= loc->nWidth))
	{
		return FALSE;	
	}

	// todo: more intelligent method of finding free space
	for (int jj = 0; jj < loc->nHeight - ht + 1; jj++)
	{
		for (int ii = 0; ii < loc->nWidth - wd + 1; ii++)
		{
			if ((!pIgnoreItem && sGridTest(loc, ii, jj, wd, ht, item)) ||
				(pIgnoreItem && sGridTestIgnoreItem(loc, ii, jj, item, pIgnoreItem)))
			{
				if (x) *x = ii;
				if (y) *y = jj;
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryGridGetNumEmptySpaces(
	UNIT * container,
	int location)
{
	ASSERT_RETFALSE(container);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	if (InvLocTestFlag(loc, INVLOCFLAG_RESERVED))
	{
		return 0;
	}
	
	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		return 0;
	}

	int nCount = 0;
	for (int jj = 0; jj < loc->nHeight + 1; jj++)
	{
		for (int ii = 0; ii < loc->nWidth + 1; ii++)
		{
			if (sGridTest(loc, ii, jj, 1, 1))
			{
				nCount++;
			}
		}
	}
	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetItemsAlreadyInEquipLocations(
	UNIT * container,
	UNIT * item,
	BOOL bCheckEquipped,
	BOOL& bEmptySlot,
	UNIT ** ppItemList,
	int nListSize,
	BOOL *pbPreventLoc /*= NULL*/)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(ppItemList);

	bEmptySlot = FALSE;
	if (!container->inventory)
	{
		return 0;
	}

	int nCount = 0;

	INVENTORY * inv = container->inventory;
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;

		if (loc->nFilter <= 0)
		{
			continue;
		}

		BOOL bAllowed = sIsAllowedUnitInv(item, loc, container);
		BOOL bPreventLoc = IsAllowedUnit(item, loc->nPreventTypes1, INVLOC_UNITTYPES);
		if (!bAllowed &&
			!bPreventLoc)
		{
			continue;
		}

		UNIT * iter = loc->itemfirst;

		if (bAllowed && !iter)
		{
			if (!IsUnitPreventedFromInvLoc(container, item, loc->nLocation))
			{
				bEmptySlot = TRUE;
			}
		}

		while (iter &&
			nCount < nListSize)
		{
			if (iter != item && (!bCheckEquipped || ItemIsEquipped(iter, container)))
			{
				ppItemList[nCount] = iter;
				if (pbPreventLoc)
					pbPreventLoc[nCount] = bPreventLoc;
				nCount++;
			}
			iter = iter->invnode->locnext;
		}
	}

	return nCount;
}


//----------------------------------------------------------------------------
// INVENTORY_ERROR is error condition, otherwise return # of items in overlap
//----------------------------------------------------------------------------
int UnitInventoryGetOverlap(
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y,
	UNIT ** itemlist,
	int listsize)
{
	// TODO: combine this function with InventoryGridGetItemsOverlap

	ASSERT_RETX(container, INVENTORY_ERROR);
	ASSERT_RETX(item && item->invnode, INVENTORY_ERROR);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETX(loc->nLocation == location, INVENTORY_ERROR);

	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		ASSERT_RETZERO(listsize >= 1);
		ASSERT_RETZERO(itemlist);
		itemlist[0] = UnitInventoryGetByLocationAndXY(container, location, x, y);
		return (itemlist[0] ? 1 : 0);
	}

	int ht, wd;
	BOOL fResult = UnitGetSizeInGrid(item, &ht, &wd, loc);
	ASSERT_RETX(fResult, INVENTORY_ERROR);

	ASSERT(listsize >= ht * wd);

	// test that the item fits in the location
	ASSERT_RETX(x >= 0 && x + wd <= loc->nWidth &&
		y >= 0 && y + ht <= loc->nHeight, INVENTORY_ERROR);

	int count = 0;
	int test_ht = MIN(y + ht, loc->nAllocatedHeight);
	int test_wd = MIN(x + wd, loc->nAllocatedWidth);

	for (int jj = y; jj < test_ht; jj++)
	{
		UNIT ** ptr = loc->ppUnitGrid + jj * loc->nAllocatedWidth + x;
		for (int ii = x; ii < test_wd; ii++)
		{
			if (*ptr != NULL)
			{
				BOOL fFound = FALSE;
				for (int kk = 0; kk < count; kk++)
				{
					if (*ptr == itemlist[kk])
					{
						fFound = TRUE;
						break;
					}
				}
				if (!fFound && count < listsize)
				{
					itemlist[count] = *ptr;
					count++;
				}
			}
			ptr++;
		}
	}

	return count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFilterIterateItem(
	UNIT * container,
	UNIT * item,
	DWORD dwInvIterateFlags)
{	
	ASSERT_RETVAL(item, TRUE);

	// skip items that must be in standard slots only
	if (TESTBIT(dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT) &&
		InventoryIsInLocationType(item, LT_ON_PERSON) == FALSE)
	{
		return FALSE;
	}

	// skip items that aren't on player on in stash
	if ( TESTBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT) )
	{
		if ( InventoryIsInLocationType(item, LT_ON_PERSON) == FALSE )
		{
			if ( InventoryIsInStashLocation(item) == FALSE )
				return FALSE;

			DWORD dwContainerVariant = GameVariantFlagsGetStaticUnit(container);
			DWORD dwItemVariant = UnitGetStat(item, STATS_GAME_VARIANT);
			if (dwContainerVariant != dwItemVariant)
				return FALSE;
		}
	}

	// skip items that must be in trade slots only (one day we might have more than one slot for trade)
	if (TESTBIT( dwInvIterateFlags, IIF_TRADE_SLOTS_ONLY_BIT ) &&
		InventoryIsInTradeLocation( item ) == FALSE)
	{
		return FALSE;
	}

	// skip items that must be in pet slots only
	if (TESTBIT( dwInvIterateFlags, IIF_PET_SLOTS_ONLY_BIT ) &&
		InventoryIsInPetLocation( item ) == FALSE)
	{
		return FALSE;
	}

	if (TESTBIT(dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT) || TESTBIT(dwInvIterateFlags, IIF_EQUIP_SLOTS_ONLY_BIT))
	{
		BOOL bEquipped = ItemIsInEquipLocation(container, item);
		// skip items that in equip slots if requested		
		if (TESTBIT(dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT) &&
			bEquipped)
		{
			return FALSE;
		}
		// skip item if it's not in an equipped slot
		if (TESTBIT(dwInvIterateFlags, IIF_EQUIP_SLOTS_ONLY_BIT) &&
			!bEquipped)
		{
			return FALSE;
		}
	}

	if (TESTBIT(dwInvIterateFlags, IIF_ACTIVE_STORE_SLOTS_ONLY_BIT) || TESTBIT(dwInvIterateFlags, IIF_MERCHANT_WAREHOUSE_ONLY_BIT))
	{
		int invloc = INVLOC_NONE;
		UnitGetInventoryLocation(item, &invloc);
		const INVLOC_DATA * invloc_data = UnitGetInvLocData(container, invloc);
		ASSERT_RETFALSE(invloc_data);

		if (TESTBIT(dwInvIterateFlags, IIF_ACTIVE_STORE_SLOTS_ONLY_BIT))
		{
			if (invloc == INVLOC_MERCHANT_BUYBACK || InvLocTestFlag(invloc_data, INVLOCFLAG_STORE) == FALSE)
			{
				return FALSE;
			}
		}
		if (TESTBIT(dwInvIterateFlags, IIF_MERCHANT_WAREHOUSE_ONLY_BIT))
		{
			if (InvLocTestFlag(invloc_data, INVLOCFLAG_MERCHANT_WAREHOUSE) == FALSE)
			{
				return FALSE;
			}
		}	
	}

	if ( TESTBIT( dwInvIterateFlags, IFF_IN_CUBE_ONLY_BIT ) )
	{
		int invloc = INVLOC_NONE;
		UnitGetInventoryLocation(item, &invloc);
		if ( invloc != INVLOC_CUBE )
			return FALSE;
	}

	if ( TESTBIT( dwInvIterateFlags, IFF_CHECK_DIFFICULTY_REQ_BIT ) )
	{
		const UNIT_DATA * item_data = UnitGetData(item);
		UNIT *ultimateContainer = UnitGetUltimateContainer(item);
		if (UnitDataTestFlag(item_data, UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY) &&
			UnitIsPlayer(ultimateContainer) &&
			UnitGetStat(item, STATS_ITEM_DIFFICULTY_SPAWNED) != UnitGetStat(ultimateContainer, STATS_DIFFICULTY_CURRENT))
		{
			return FALSE;
		}
	}

	return TRUE;
}	


//----------------------------------------------------------------------------
// phu note - this is really slow!!!  (and i didn't write this)
// please use the location version (below) if possible
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryIterate)(INV_FUNCFLARGS(
	UNIT * container,
	UNIT * item,
	DWORD dwInvIterateFlags /*=0*/,
	BOOL bIterateChildren /*= TRUE*/))
{
#if INVENTORY_DEBUG
	REF(file);
	REF(line);
#endif

	ASSERT_RETNULL(container);
	if (!container->inventory)
	{
		return NULL;
	}

	BOOL bNewLevel = FALSE;
	if (bIterateChildren && item && UnitIsA(item, UNITTYPE_CONTAINER) && item->inventory)
	{
		// up one level
		UNIT *child = item->inventory->itemfirst;
		if (child)
		{
			item = child;
			bNewLevel = TRUE;
		}
	}

	UNIT *parent = item ? UnitGetContainer(item) : NULL;
	BOOL bTopLevel = !bIterateChildren || !parent || parent == container;
	INVENTORY * inv = bTopLevel ? container->inventory : parent->inventory;
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	if (!bNewLevel)
	{
		if (item)
		{
			ASSERT_RETNULL(item->invnode);
			ASSERT_RETNULL(!bTopLevel || item->invnode->container == container);
			item = item->invnode->invnext;	
		}
		else
		{
			item = inv->itemfirst;
		}
	}

	do
	{
		bTopLevel = !bIterateChildren || !parent || parent == container;

		while (item)
		{
			ASSERT_RETNULL(item->invnode);
			ASSERT_RETNULL(!bTopLevel || item->invnode->container == container);

			if (sFilterIterateItem(container, item, dwInvIterateFlags))
			{
				return item;
			}
			item = item->invnode->invnext;
		}

		if (!bTopLevel)
		{
			//down one level
			ASSERT_RETNULL(parent);
			ASSERT_RETNULL(parent->invnode);
			item = parent->invnode->invnext;
		}

		parent = item ? UnitGetContainer(item) : NULL;
	}
	while (!bTopLevel);

	return NULL;
}


//----------------------------------------------------------------------------
// phu note - this is really slow!!!  (and i didn't write this)
//----------------------------------------------------------------------------
UNIT * INV_FUNCNAME(UnitInventoryLocationIterate)(INV_FUNCFLARGS(
	UNIT * container,
	int location,
	UNIT * item,
	DWORD dwInvIterateFlags /*=0*/,
	BOOL bIterateChildren /*= TRUE*/))
{
	ASSERT_RETNULL(container);
	if (!container->inventory)
	{
		return NULL;
	}

#if INVENTORY_DEBUG
	//if (IS_SERVER(container))
	//{
	//	trace("UnitInventoryLocationIterate()  LOC:%d  FILE:%s  LINE:%d\n", location, file, line);
	//}
	REF(file);
	REF(line);
#endif

	BOOL bNewLevel = FALSE;
	if (bIterateChildren && item && UnitIsA(item, UNITTYPE_CONTAINER) && item->inventory)
	{
		// up one level
		UNIT *child = item->inventory->itemfirst;
		if (child)
		{
			item = child;
			bNewLevel = TRUE;
		}
	}

	UNIT *parent = item ? UnitGetContainer(item) : NULL;
	BOOL bTopLevel = !bIterateChildren || !parent || parent == container;
	INVENTORY * inv = bTopLevel ? container->inventory : parent->inventory;
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	INVLOC locdata;
	INVLOC * loc = NULL;
	if (bTopLevel)
	{
		loc = InventoryGetLoc(container, location, &locdata);
		if (!loc)
		{
			loc = &locdata;
		}
		if (loc->nLocation != location)
		{
			return NULL;
		}
	}

	if (!bNewLevel)
	{
		if (item)
		{
			ASSERT_RETNULL(item->invnode);
			ASSERT_RETNULL(!bTopLevel || item->invnode->container == container);
			ASSERT_RETNULL(!bTopLevel || item->invnode->invloc == location);

			item = item->invnode->locnext;	
		}
		else
		{
			if (loc)
			{
				item = loc->itemfirst;
			}
			else
			{
				item = inv->itemfirst;
			}
		}
	}

	do
	{
		bTopLevel = !bIterateChildren || !parent || parent == container;

		while (item)
		{
			ASSERT_RETNULL(item->invnode);
			ASSERT_RETNULL(!bTopLevel || item->invnode->container == container);
			ASSERT_RETNULL(!bTopLevel || item->invnode->invloc == location);

			if (sFilterIterateItem(container, item, dwInvIterateFlags))
			{
				return item;
			}
			item = item->invnode->locnext;
		}

		if (!bTopLevel)
		{
			//down one level
			ASSERT_RETNULL(parent);
			ASSERT_RETNULL(parent->invnode);
			item = parent->invnode->locnext;
		}

		parent = item ? UnitGetContainer(item) : NULL;
	}
	while (!bTopLevel);

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetCurrentInventoryLocation(
	UNIT * item,
	int& location,
	int& x,
	int& y)
{
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);
	if (!item->invnode->container)
	{
		return FALSE;
	}
	location = item->invnode->invloc;
	x = item->invnode->x;
	y = item->invnode->y;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InventoryLocationInit(
	INVENTORY_LOCATION &tInventoryLocation)
{
	tInventoryLocation.nInvLocation = INVLOC_NONE;
	tInventoryLocation.nX = 0;
	tInventoryLocation.nY = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetInventoryLocation(
	UNIT * item,
	int * location)
{
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);
	if (!item->invnode->container)
	{
		return FALSE;
	}
	*location = item->invnode->invloc;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetInventoryLocation(
	UNIT * item,
	int * location,
	int * x,
	int * y)
{
	int temp_location = INVLOC_NONE;
	if (location)
	{
		*location = INVLOC_NONE;
	}
	else
	{
		location = &temp_location;
	}
	if (x)
	{
		*x = 0;
	}
	if (y)
	{
		*y = 0;
	}
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);
	UNIT * container = item->invnode->container;
	if (!container)
	{
		return FALSE;
	}
	*location = item->invnode->invloc;
	if (*location == INVLOC_NONE)
	{
		return FALSE;
	}

	INVENTORY * inv = container->inventory;
	ASSERT_RETFALSE(inv);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, *location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == *location);

	if (!InvLocTestFlag(loc, INVLOCFLAG_GRID))
	{
		// get index in list
		int count = 0;
		UNIT * iter = loc->itemfirst;

		while (iter && iter != item)
		{
			count++;
			iter = iter->invnode->locnext;
		}
		ASSERT_RETFALSE(iter == item);
		if (x)
		{
			*x = count;
		}
		if (y)
		{
			*y = 0;
		}
		return TRUE;
	}
	
	if (x)
	{
		*x = item->invnode->x;
	}
	if (y)
	{
		*y = item->invnode->y;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetInventoryLocation(
	UNIT * item,
	INVENTORY_LOCATION * pLocation)
{
	ASSERTX_RETFALSE( item, "Expected item" );
	ASSERTX_RETFALSE( pLocation, "Invalid params" );
	return UnitGetInventoryLocation(item, &pLocation->nInvLocation, &pLocation->nX, &pLocation->nY);
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TIME UnitGetInventoryEquipTime(
	UNIT * unit)
{
	ASSERT_RETVAL(unit, TIME_ZERO);
	ASSERT_RETVAL(unit->invnode, TIME_ZERO);
	return unit->invnode->m_tiEquipTime;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsWeaponLocation ( 
	int nLocation,
	UNIT * container)
{
	if (nLocation == INVLOC_NONE)
	{
		return FALSE;
	}

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, nLocation, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == nLocation);

	return InvLocTestFlag(loc, INVLOCFLAG_WEAPON);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInEquipLocation ( 
	UNIT * container,
	UNIT * item)
{
	if (!item || !container)
	{
		return FALSE;
	}

	if (UnitGetContainer(item) != container)
	{
		return FALSE;
	}

	int nItemLocation;
	int nItemLocationX;
	int nItemLocationY;
	if (UnitGetInventoryLocation(item, &nItemLocation, &nItemLocationX, &nItemLocationY))
	{
		return InvLocIsEquipLocation(nItemLocation, container);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInNoDismantleLocation ( 
	UNIT * container,
	UNIT * item)
{
	if (!item || !container)
	{
		return FALSE;
	}

	if (UnitGetContainer(item) != container)
	{
		return FALSE;
	}

	int nItemLocation;
	int nItemLocationX;
	int nItemLocationY;
	if (UnitGetInventoryLocation(item, &nItemLocation, &nItemLocationX, &nItemLocationY))
	{
		return InvLocIsNoDismantleLocation(nItemLocation, container);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInWeaponSetLocation( 
	UNIT * container,
	UNIT * item)
{
	if (!item || !container)
	{
		return FALSE;
	}

	if (UnitGetContainer(item) != container)
	{
		return FALSE;
	}

	int nItemLocation;
	int nItemLocationX;
	int nItemLocationY;
	if (UnitGetInventoryLocation(item, &nItemLocation, &nItemLocationX, &nItemLocationY))
	{
		return (nItemLocation == INVLOC_RHAND2 || nItemLocation == INVLOC_LHAND2);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryGetCount(
	UNIT * container)
{
	ASSERT_RETZERO(container);
	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return 0;
	}
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	return inv->nCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryGetCount(
	UNIT * container,
	int location)
{
	ASSERT_RETZERO(container);
	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return 0;
	}
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	if (loc->nLocation != location)
	{
		return 0;
	}
	return loc->nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryLocCountAliveInWorld(
	UNIT *pContainer,
	int nInvLoc)
{
	ASSERTX_RETZERO( pContainer, "Expected container unit" );
	ASSERTX_RETZERO( nInvLoc != INVALID_LINK, "Invalid inventory location" );
	int nCount = 0;
	
	DWORD dwInvIterateFlags = 0;
	UNIT *pItem = UnitInventoryLocationIterate( pContainer, nInvLoc, NULL, dwInvIterateFlags );
	while (pItem)
	{
		
		// must be alive and in world
		if (UnitGetRoom( pItem ) != NULL && IsUnitDeadOrDying( pItem ) == FALSE)
		{
			nCount++;
		}
			
		// next item
		pItem = UnitInventoryLocationIterate( pContainer, nInvLoc, pItem, dwInvIterateFlags );
				
	}

	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryTestFlag(
	UNIT * container,
	int location,
	int nFlag)
{
	ASSERT_RETFALSE(container);
	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return 0;
	}
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif

	INVLOC locdata;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	if (loc->nLocation != location)
	{
		return FALSE;
	}

	ASSERT_RETFALSE(nFlag >= 0 && nFlag < NUM_INVLOCFLAGS);
	return TESTBIT(loc->dwFlags, nFlag);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryCountItemsOfType(
	struct UNIT * container,
	int nItemClass,
	DWORD dwInvIterateFlags,
	int nItemQuality /*= INVALID_LINK*/,
	int nInvLoc /*= INVLOC_NONE*/,
	UNIT *pUnitIgnore /*= NULL*/)
{
	SPECIES spUnit = MAKE_SPECIES(GENUS_ITEM, nItemClass);
	return UnitInventoryCountUnitsOfType(
				container, 
				spUnit, 
				dwInvIterateFlags, 
				nItemQuality, 
				nInvLoc,
				pUnitIgnore);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsMatchingItem(
	UNIT * item,
	SPECIES spSpecies,
	int nItemQuality)
{
	ASSERT_RETFALSE(item);	
	if (UnitGetSpecies(item) == spSpecies)
	{
		if (nItemQuality == INVALID_LINK || ItemGetQuality(item) == nItemQuality)
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitInventoryCountUnitsOfType(
	struct UNIT * container,
	const SPECIES species,
	DWORD dwInvIterateFlags,
	int nItemQuality /*= INVALID_LINK*/,
	int nInvLoc /*= INVLOC_NONE*/,
	UNIT *pUnitIgnore /*= NULL*/)
{
	ASSERT_RETZERO(container);

	int count = 0;
	
	// iterate requested slots in inventory and count items
	if (nInvLoc == INVLOC_NONE)
	{
		UNIT * item = NULL;	
		while ((item = UnitInventoryIterate(container, item, dwInvIterateFlags)) != NULL)
		{
			if (sIsMatchingItem(item, species, nItemQuality))
			{
				if (pUnitIgnore == NULL || item != pUnitIgnore)
				{
					count += ItemGetQuantity(item);
				}
			}			
		}
	}
	else
	{
		UNIT * item = NULL;	
		while ((item = UnitInventoryLocationIterate(container, nInvLoc, item, dwInvIterateFlags)) != NULL)
		{
			if (sIsMatchingItem(item, species, nItemQuality))
			{
				if (pUnitIgnore == NULL || item != pUnitIgnore)
				{
					count += ItemGetQuantity(item);
				}
			}			
		}	
	}

	return count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sReserveQuantityLocation( 
	UNIT* pUnitWithSpace,
	int nQuantity)
{
	UnitChangeStat( pUnitWithSpace, STATS_ITEM_QUANTITY_RESERVED, nQuantity );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sReserveInventoryLocation( 
	UNIT* pUnitWithSpace, 
	int nInventoryLocation, 
	int nInvX, 
	int nInvY,
	UNIT* pReservedFor)
{
	INVLOC *pInventoryLoc = InventoryGetLocAdd( pUnitWithSpace, nInventoryLocation );
	
	if (InvLocTestFlag( pInventoryLoc, INVLOCFLAG_GRID ))
	{
	
		// get size to reserve
		int nUnitWidth;
		int nUnitHeight;
		UnitGetSizeInGrid( pReservedFor, &nUnitHeight, &nUnitWidth, pInventoryLoc );
		
		// mark as reserved in grid .. TODO: We may want to actually put the item in
		// the inventory so that stackable items could then stack on it, but this seems
		// like a dangerous place for item duplication, so we're holding off on it
		// unless its absolutely necessary
		sGridMark( (UNIT*)RESERVED_MARK, pInventoryLoc, nInvX, nInvY, nUnitWidth, nUnitHeight );

	}
	else
	{
		InvLocSetFlag( pInventoryLoc, INVLOCFLAG_RESERVED, TRUE );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearAllReservations( 
	UNIT* pUnit)
{

	// clear all reserved counts
	UNIT* pContained = NULL;
	while ((pContained = UnitInventoryIterate( pUnit, pContained )) != NULL)
	{
		UnitSetStat( pContained, STATS_ITEM_QUANTITY_RESERVED, 0 );
		sClearAllReservations( pContained );
	}
	
	// clear all reserved locations
	INVENTORY* pInventory = pUnit->inventory;
	if (pInventory)
	{
		for (int i = 0; i < pInventory->nLocs; ++i)
		{
			INVLOC* pInventoryLoc = &pInventory->pLocs[ i ];
			InvLocSetFlag( pInventoryLoc, INVLOCFLAG_RESERVED, FALSE );
			UNIT** ptr = pInventoryLoc->ppUnitGrid;
			for (int j = 0; j < pInventoryLoc->nAllocatedWidth * pInventoryLoc->nAllocatedHeight; ++j)
			{
				if (*ptr == (UNIT*)RESERVED_MARK)
				{
					*ptr = NULL;
				}
				ptr++;				
			}
		}
	}			
}

					
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryCanPickupAll( 
	UNIT * pUnit,
	UNIT ** pUnitsToPickup,
	int nNumUnitsToPickup,
	BOOL bDontCheckCanKnowUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pUnitsToPickup, "Invalid pickup array" );
	ASSERTX_RETFALSE( nNumUnitsToPickup > 0, "Empty pickup array" );
	
	// go through each item to pickup
	BOOL bCanPickupAll = TRUE;
	for ( int i = 0; i < nNumUnitsToPickup; ++i)
	{
		UNIT * pToPickup = pUnitsToPickup[ i ];

		DWORD dwFlags = 0;
		if(bDontCheckCanKnowUnit)
		{
			dwFlags |= ICP_DONT_CHECK_KNOWN_MASK;
		}

		// test basic can pickup
		if (ItemCanPickup( pUnit, pToPickup, dwFlags ) != PR_OK)
		{
			bCanPickupAll = FALSE;
			break;
		}
				
		// see if item can autostack
		UNIT * pToStackOn = ItemCanAutoStack( pUnit, pToPickup );
		if (pToStackOn != NULL)
		{
			int nQuantity = ItemGetQuantity( pToPickup );
			sReserveQuantityLocation( pToStackOn, nQuantity );
		}
		else
		{
		
			// look for regular inventory slot
			int nInventoryLocation;
			int nInvX, nInvY;
			if (ItemGetOpenLocation( pUnit, pToPickup, TRUE, &nInventoryLocation, &nInvX, &nInvY ))
			{
				sReserveInventoryLocation( pUnit, nInventoryLocation, nInvX, nInvY, pToPickup );
			}
			else
			{
				bCanPickupAll = FALSE;
				break;
			}
						
		}
				
	}

	// clear all reservations
	sClearAllReservations( pUnit );
	
	// return result
	return bCanPickupAll;
	
}
		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sItemGetEquipLocationDynamic(
	UNIT * container,
	UNIT * item,
	int * location,
	int * x,
	int * y)
{
	GAME * game = UnitGetGame(container);
	ASSERT_RETFALSE(game);

	if (container->inventory)
	{
		if (!sInventoryTestFlag(container->inventory, INVFLAG_DYNAMIC))
		{
			return FALSE;
		}
	}
	int inventory_index = UnitGetInventoryType(container);
	if (inventory_index <= 0)
	{
		return FALSE;
	}

	const INVENTORY_DATA * inv_data = InventoryGetData(UnitGetGame(container), inventory_index);
	if (!inv_data)
	{
		return NULL;
	}

	ASSERT_RETNULL(inv_data->nNumLocs > 0);

	for (int ii = inv_data->nFirstLoc; ii < inv_data->nFirstLoc + inv_data->nNumLocs; ii++)
	{
		const INVLOC_DATA * locdata = InvLocGetData(game, ii);
		if(!locdata)
			continue;

		if (!InvLocTestFlag(locdata, INVLOCFLAG_DYNAMIC))
		{
			continue;
		}
		if (UnitInventoryFindSpace(container, item, locdata->nLocation, x, y))
		{
			if (location)
			{
				*location = locdata->nLocation;
			}
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// this should return the location in which we ought to equip an item
// this depends on which locations are empty.  if there are multiple possible
// locations all of which are filled, return the location (item) which has been 
// equipped the longest, return TRUE if open space or FALSE if occupied space
// or invalid call (with *location = INVLOC_NONE)
//----------------------------------------------------------------------------
BOOL ItemGetEquipLocation(
	UNIT * container,
	UNIT * item,
	int * location,
	int * x,
	int * y,
	int nExceptLocation /*= INVLOC_NONE*/,
	BOOL bSkipIdentifiedCheck /*= FALSE*/)
{
	ASSERT_RETFALSE(location);
	ASSERT_RETFALSE(x);
	ASSERT_RETFALSE(y);

	*location = INVLOC_NONE;

	ASSERT_RETFALSE(container);
	if (!container->inventory)
	{
		return sItemGetEquipLocationDynamic(container, item, location, x, y);
	}

	INVENTORY * inv = container->inventory;
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	TIME tiEarliestTime = TIME_ZERO;
	BOOL bFoundAnItemInLocation = FALSE;
	DWORD dwChangeLocationFlags = 0;
	if (bSkipIdentifiedCheck)
	{
		SET_MASK(dwChangeLocationFlags, CLF_IGNORE_UNIDENTIFIED_MASK);
	}

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;

		if (loc->nFilter <= 0)
		{
			continue;
		}

		if (!sIsAllowedUnitInv(item, loc, container, dwChangeLocationFlags))
		{
			continue;
		}

		if (loc->nLocation == nExceptLocation)
		{
			continue;
		}

		// if we get here then this is the right location for the item, no matter what
		//   but if we've found an old item and filled the values with its location, 
		//   we don't want to overwrite them.
		if (!bFoundAnItemInLocation)
		{
			*location = loc->nLocation;
			*x = 0;
			*y = 0;
		}

		if (UnitInventoryFindSpace(container, item, loc->nLocation, x, y))
		{
			*location = loc->nLocation;
			return TRUE;
		}

		UNIT * iter = loc->itemfirst;
		while (iter)
		{
			if (!bFoundAnItemInLocation || (tiEarliestTime != TIME_ZERO && iter->invnode->m_tiEquipTime < tiEarliestTime))
			{
				bFoundAnItemInLocation = TRUE;
				*location = loc->nLocation;
				*x = iter->invnode->x;
				*y = iter->invnode->y;
				tiEarliestTime = iter->invnode->m_tiEquipTime;
			}
			iter = iter->invnode->locnext;
		}
	}
	return sItemGetEquipLocationDynamic(container, item, location, x, y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitInventoryGetOldestItem(
	UNIT * container,
	int location)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(location);
	INVENTORY * inventory = container->inventory;
	ASSERT_RETFALSE(inventory);
	INVENTORY_CHECK_DEBUGMAGIC(inventory);

	INVLOC locdata;
	locdata.nLocation = -1;
	INVLOC * loc = InventoryGetLoc(container, location, &locdata);
	if (!loc)
	{
		loc = &locdata;
	}
	ASSERT_RETFALSE(loc->nLocation == location);

	TIME timeOldest = TIME_LAST;
	UNIT * oldest = NULL;

	UNIT * item = NULL;
	UNIT * next = loc->itemfirst;
	while ((item = next) != NULL)
	{
		ASSERT_BREAK(item->invnode);
		next = item->invnode->locnext;
		if (item->invnode->m_tiEquipTime < timeOldest)
		{
			oldest = item;
			timeOldest = item->invnode->m_tiEquipTime;
		}
	}

	return oldest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sInventoryGetCheapestInLocation(
	INVLOC * loc,
	UNIT * bestunit,
	int & bestprice)
{
	ASSERT_RETNULL(loc);

	UNIT * item = loc->itemfirst;
	while (item)
	{
		ASSERT_BREAK(item->invnode);
		cCurrency currency = EconomyItemBuyPrice( item );
		int price = currency.GetValue( KCURRENCY_VALUE_INGAME );// ItemGetBuyPrice(item);
		if (price <= bestprice)
		{
			bestprice = price;
			bestunit = item;
		}
		item = item->invnode->locnext;
	}
	return bestunit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sInventoryGetOldestInLocation(
	INVLOC * loc,
	UNIT * oldestunit,
	TIME & oldesttime)
{
	ASSERT_RETNULL(loc);

	UNIT * item = loc->itemfirst;
	while (item)
	{
		ASSERT_BREAK(item->invnode);

		if (item->invnode->m_tiEquipTime <= oldesttime)
		{
			oldesttime = item->invnode->m_tiEquipTime;
			oldestunit = item;
		}
		item = item->invnode->locnext;
	}
	return oldestunit;
}


//----------------------------------------------------------------------------
// this should return the location in which we ought to equip an item
// this depends on which locations are empty.  return TRUE if open space 
// is found or FALSE if no unoccupied spaces are found.
//----------------------------------------------------------------------------
BOOL ItemGetOpenLocation(
	UNIT * container,
	UNIT * item,
	BOOL bAutoPickup,
	int * location,
	int * x,
	int * y,
	BOOL bCheckRequirements /*=FALSE*/,
	DWORD dwChangeLocationFlags /*=0*/,
	DWORD dwItemEquipFlags /*=0*/)
{
	if (location) 
	{
		*location = INVLOC_NONE;
	}

	ASSERT_RETFALSE(container);
	if (!container->inventory)
	{
		return sItemGetEquipLocationDynamic(container, item, location, x, y);
	}

	INVENTORY * inventory = container->inventory;
	ASSERT_RETFALSE(inventory);
	INVENTORY_CHECK_DEBUGMAGIC(inventory);

	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	BOOL bIsMerchant = UnitIsMerchant(container);
	UNIT * best = NULL;
	TIME timeOldest = TIME_LAST;
	int cheapestPrice = INT_MAX;

	for (int ii = 0; ii < inventory->nLocs; ii++)
	{
		INVLOC * loc = inventory->pLocs + ii;

		if (bAutoPickup && !InvLocTestFlag(loc, INVLOCFLAG_AUTOPICKUP))
		{
			continue;
		}
		if (!sIsAllowedUnitInv(item, loc, container))
		{
			continue;
		}

		if( bCheckRequirements &&
			!InventoryCheckStatRequirements(container, item, NULL, NULL, loc->nLocation))
		{
			continue;
		}

		if ( TEST_MASK( dwChangeLocationFlags, CLF_CHECK_PLAYER_PUT_RESTRICTED ) &&
			 !(UnitDataTestFlag( UnitGetData( container ), UNIT_DATA_FLAG_IGNORES_ITEM_REQUIREMENTS ) ) &&			
			 !InventoryLocPlayerCanPut(container, item, loc->nLocation))
		{
			continue;
		}

		if ( TEST_MASK( dwChangeLocationFlags, CLF_DO_NOT_EQUIP ) &&
			 InvLocTestFlag(loc, INVLOCFLAG_EQUIPSLOT))
		{
			continue;
		}

		if (UnitInventoryFindSpace(container, item, loc->nLocation, x, y, dwChangeLocationFlags, NULL, dwItemEquipFlags))
		{
			if (location) 
			{
				*location = loc->nLocation;
			}
			return TRUE;
		}

		UNIT * found = NULL;
		if (bIsMerchant)
		{
			found = sInventoryGetCheapestInLocation(loc, best, cheapestPrice);
		}
		else
		{
			found = sInventoryGetOldestInLocation(loc, best, timeOldest);
		}
		if (found)
		{
			ASSERT_CONTINUE(found->invnode);
			if (location) 
			{
				ASSERT_CONTINUE(found->invnode->invloc == loc->nLocation);
				*location = found->invnode->invloc;
			}
			if (x) 
			{
				*x = found->invnode->x;
			}
			if (y)
			{
				*y = found->invnode->y;
			}
		}
	}
	return sItemGetEquipLocationDynamic(container, item, location, x, y);
}

	
//----------------------------------------------------------------------------
// this should return the location in which we ought to equip an item
// this depends on which locations are empty.  return TRUE if open space 
// is found or FALSE if no unoccupied spaces are found.
//----------------------------------------------------------------------------
BOOL ItemGetOpenLocationNoEquip(
	UNIT * container,
	UNIT * item,
	BOOL bAutoPickup,
	int * location,
	int * x,
	int * y)
{
	if (location) 
	{
		*location = INVLOC_NONE;
	}

	ASSERT_RETFALSE(container);
	
	INVENTORY * inventory = container->inventory;
	if (!inventory)
	{
		return sItemGetEquipLocationDynamic(container, item, location, x, y);
	}
	INVENTORY_CHECK_DEBUGMAGIC(inventory);

	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	BOOL bIsMerchant = UnitIsMerchant(container);
	UNIT * best = NULL;
	TIME timeOldest = TIME_LAST;
	int cheapestPrice = INT_MAX;

	for (int ii = 0; ii < inventory->nLocs; ii++)
	{
		INVLOC * loc = inventory->pLocs + ii;

		if (bAutoPickup && !InvLocTestFlag(loc, INVLOCFLAG_AUTOPICKUP))
		{
			continue;
		}
		if (!sIsAllowedUnitInv(item, loc, container))
		{
			continue;
		}
		if (sInvLocIsEquipLocation(loc, container))
		{
			continue;
		}
		
		if (UnitInventoryFindSpace(container, item, loc->nLocation, x, y))
		{
			if (location) 
			{
				*location = loc->nLocation;
			}
			return TRUE;
		}

		UNIT * found = NULL;
		if (bIsMerchant)
		{
			found = sInventoryGetCheapestInLocation(loc, best, cheapestPrice);
		}
		else
		{
			found = sInventoryGetOldestInLocation(loc, best, timeOldest);
		}
		if (found)
		{
			ASSERT_CONTINUE(found->invnode);
			if (location) 
			{
				ASSERT_CONTINUE(found->invnode->invloc == loc->nLocation);
				*location = found->invnode->invloc;
			}
			if (x) 
			{
				*x = found->invnode->x;
			}
			if (y)
			{
				*y = found->invnode->y;
			}
		}
	}
	return sItemGetEquipLocationDynamic(container, item, location, x, y);
}

//----------------------------------------------------------------------------
// This will return TRUE if the player has room in their inventory to put the item down
//   once it's in the cursor
//----------------------------------------------------------------------------
BOOL ItemGetOpenLocationForPlayerToPutDown(
	UNIT * container,
	UNIT * item,
	BOOL bCheckRequirements /*=FALSE*/,
	BOOL bOnPersonOnly /*= FALSE*/)
{
	ASSERT_RETFALSE(container);
	if (!container->inventory)
	{
		return FALSE;
	}

	INVENTORY * inventory = container->inventory;
	ASSERT_RETFALSE(inventory);
	INVENTORY_CHECK_DEBUGMAGIC(inventory);

	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	if (!UnitIsPlayer(container))
	{
		return FALSE;
	}

	int nCursorLoc = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	for (int ii = 0; ii < inventory->nLocs; ii++)
	{
		INVLOC * loc = inventory->pLocs + ii;

		if (loc->nLocation == nCursorLoc)
		{
			continue;
		}

		if (bOnPersonOnly && !InvLocIsOnPersonLocation( container, loc->nLocation ))
		{
			continue;
		}

		if (!InvLocIsStandardLocation( container, loc->nLocation ))
		{
			continue;
		}

		if (!InventoryLocPlayerCanPut(container, item, loc->nLocation))
		{
			continue;
		}

		if (!sIsAllowedUnitInv(item, loc, container))
		{
			continue;
		}

		if( bCheckRequirements &&
			!InventoryCheckStatRequirements(container, item, NULL, NULL, loc->nLocation))
		{
			continue;
		}
		if (UnitInventoryFindSpace(container, item, loc->nLocation, NULL, NULL, 0, NULL, 0))
		{
			return TRUE;
		}

	}
	return FALSE;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ITEM_STACK_RESULT ItemStack(
	UNIT * itemDest,
	UNIT * itemSrc)
{
	ASSERT_RETVAL(itemDest && itemSrc, ITEM_STACK_NO_STACK);

	int src_quantity = ItemGetQuantity( itemSrc );
	if (src_quantity <= 0)
	{
		return ITEM_STACK_NO_STACK;
	}
	int maxstack = UnitGetStat(itemDest, STATS_ITEM_QUANTITY_MAX);
	if (maxstack <= 0)
	{
		return ITEM_STACK_NO_STACK;
	}
	if (ItemsAreIdentical(itemDest, itemSrc))
	{
		int dest_quantity = ItemGetQuantity( itemDest );
		if (dest_quantity + src_quantity <= maxstack)
		{
			ItemSetQuantity( itemDest, src_quantity + ItemGetQuantity( itemDest ) );
			UnitSetStat(itemDest, STATS_ITEM_QUANTITY_MAX, maxstack);			
			UnitFree(itemSrc, UFF_SEND_TO_CLIENTS);
			return ITEM_STACK_COMBINE;
		}
		else
		{
			// put as many as we can into the "bottom" stack
			//
			ItemSetQuantity( itemDest, maxstack );
			ItemSetQuantity( itemSrc, dest_quantity + src_quantity - maxstack );
			UnitSetStat(itemDest, STATS_ITEM_QUANTITY_MAX, maxstack);
			UnitSetStat(itemSrc, STATS_ITEM_QUANTITY_MAX, maxstack);
			return ITEM_STACK_FILL_STACK;
		}
	}
	return ITEM_STACK_NO_STACK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * ItemCanAutoStack(
	struct UNIT * container,
	struct UNIT * item)
{
	ASSERT_RETNULL(container);

	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return NULL;
	}

	if (UnitIsMerchant(container))
	{
		return NULL;
	}
	
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	int quantity = ItemGetQuantity( item );
	if (quantity <= 0)
	{
		return NULL;
	}
	int maxstack = UnitGetStat(item, STATS_ITEM_QUANTITY_MAX);
	if (maxstack <= 0)
	{
		return NULL;
	}

	if (UnitIsPlayer(container) && PlayerIsSubscriber(container))
	{
		// try backpack first, if we have one
		UNIT *pInvItem = NULL;
		while ((pInvItem = UnitInventoryIterate(container, pInvItem, 0)) != NULL)
		{
			if (UnitIsA(pInvItem, UNITTYPE_BACKPACK))
			{
				UNIT *ret;
				if ((ret = ItemCanAutoStack(pInvItem, item)) != NULL)
				{
					return ret;
				}
			}
		}
	}

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;

		if (!InvLocTestFlag(loc, INVLOCFLAG_AUTOPICKUP))
		{
			continue;
		}
		if (!sIsAllowedUnitInv(item, loc, container))
		{
			continue;
		}
		
		UNIT * iter = loc->itemfirst;
		while (iter)
		{
			if (ItemHasStackSpaceFor( iter, item ))
			{
				return iter;
			}
			iter = iter->invnode->locnext;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * ItemAutoStack(
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETFALSE(container);

	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return NULL;
	}

	if (UnitIsMerchant(container))
	{
		return item;
	}

#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif
	ASSERT_RETFALSE(item);
	ASSERT_RETFALSE(item->invnode);

	int quantity = ItemGetQuantity(item);
	if (quantity <= 0)
	{
		return item;
	}
	int maxstack = UnitGetStat(item, STATS_ITEM_QUANTITY_MAX);
	if (maxstack <= 0)
	{
		return item;
	}

	if (UnitIsPlayer(container) && PlayerIsSubscriber(container))
	{
		// try backpack first, if we have one
		UNIT *pInvItem = NULL;
		while ((pInvItem = UnitInventoryIterate(container, pInvItem, 0)) != NULL)
		{
			if (UnitIsA(pInvItem, UNITTYPE_BACKPACK))
			{
				UNIT *ret;
				if ((ret = ItemAutoStack(pInvItem, item)) != NULL)
				{
					return ret;
				}
			}
		}
	}

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;

		if (!InvLocTestFlag(loc, INVLOCFLAG_AUTOPICKUP))
		{
			continue;
		}
		if (!sIsAllowedUnitInv(item, loc, container))
		{
			continue;
		}
		
		UNIT * iter = loc->itemfirst;
		while (iter)
		{
			if (ItemsAreIdentical(iter, item))
			{
				if (ItemStack(iter, item) == ITEM_STACK_COMBINE)
				{
					item = NULL;			// item HAS MOST LIKELY BEEN FREED
					// item was absorbed into a stack of like items, return the item stack as the new representation of the item that we picked up

					// ok, here's something for ya: if we're incrementing the stack on items that are in the player's hotkeys, 
					//   the UI needs to know about it so it can update the count display.  So what we're gonna do is if this item
					//   (or another of its type, 'cause the count includes those) is in the player's hotkeys, we're going to 
					//   re-send the hotkey info which will cause the UI to reload for that hotkey.
					GAME *pGame = UnitGetGame(iter);
					if (IS_SERVER(pGame))
					{
						UNIT *pUltimateOwner = UnitGetUltimateOwner(iter);
						if (pUltimateOwner && UnitIsA(pUltimateOwner, UNITTYPE_PLAYER))
						{
							for (int ii = 0; ii <= TAG_SELECTOR_HOTKEY12; ii++)
							{
								UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(pUltimateOwner, ii);
								if (!tag || tag->m_idItem[0] == INVALID_ID)
								{
									continue;
								}

								UNIT *pTagItem = UnitGetById(pGame, tag->m_idItem[0]);

								if (!UnitIsA(iter, UnitGetType(pTagItem)))
								{
									continue;
								}

								// send the hotkey tag again
								GAMECLIENT *pClient = UnitGetClient( pUltimateOwner );
								ASSERTX_RETVAL( pClient, iter, "Item has no client" );
								HotkeySend( pClient, pUltimateOwner, (TAG_SELECTOR)ii );

							}
						}
					}

					return iter;  // return what item stack the item was absorbed into
				}
			}
			iter = iter->invnode->locnext;
		}
	}
	return item;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryTestSwap(
	UNIT * item1,
	UNIT * item2,
	int nAltDestX /*= -1*/,
	int nAltDestY /*= -1*/)
{
	ASSERT_RETFALSE(item1 && item2);
	if (item1 == item2)
	{
		return FALSE;
	}
	if (UnitGetUltimateOwner(item1) != UnitGetUltimateOwner(item2))
	{
		return FALSE;
	}

	UNIT * container1 = UnitGetContainer(item1);
	int oldLoc1, oldX1, oldY1;
	UnitGetInventoryLocation(item1, &oldLoc1, &oldX1, &oldY1);
	if (nAltDestX != -1)
		oldX1 = nAltDestX;
	if (nAltDestY != -1)
		oldY1 = nAltDestY;

	UNIT * container2 = UnitGetContainer(item2);
	if (!container2)
	{
		return FALSE;
	}
	int oldLoc2, oldX2, oldY2;
	UnitGetInventoryLocation(item2, &oldLoc2, &oldX2, &oldY2);

	if (!UnitInventoryTestIgnoreItem(container2, item1, item2, oldLoc2, oldX2, oldY2) ||
		!UnitInventoryTestIgnoreItem(container1, item2, item1, oldLoc1, oldX1, oldY1))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventorySetEquippedWeaponsDetectChange(
	INVENTORY * inv,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT])
{
	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ii++)
	{
		if (!weapons)
		{
			if (inv->pWeapons[ii] != INVALID_ID)
			{
				return TRUE;
			}
		}
		else if (inv->pWeapons[ii] != UnitGetId( weapons[ii] ) )
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInventorySetEquippedWeaponsSearch(
	UNIT * weapon,
	UNIT * pWeapons[MAX_WEAPONS_PER_UNIT])
{
	ASSERT_RETFALSE(weapon);
	ASSERT_RETFALSE(pWeapons);

	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ii++)
	{
		if ( pWeapons[ii] == weapon )
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventorySetEquippedWeapons(
	UNIT * container,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT],
	BOOL bUpdateUI)
{
	ASSERT_RETFALSE(container);
	INVENTORY * inv = container->inventory;
	if (!inv)
	{
		return FALSE;
	}
	INVENTORY_CHECK_DEBUGMAGIC(inv);
	
	GAME * game = UnitGetGame(container);
	ASSERT_RETFALSE(game);

	// first remove any weapons in inventory not in the weapons list
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game) && !bUpdateUI)
	{
		UISetIgnoreStatMsgs(TRUE);	// make sure that there are no returns between this and where we turn them back on
	}
#endif

	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ii++)
	{
		if (inv->pWeapons[ii] == INVALID_ID)
		{
			continue;
		}
		UNIT * weapon = UnitGetById(game, inv->pWeapons[ii]);
		if (!weapon)
		{
			inv->pWeapons[ii] = INVALID_ID;
			continue;
		}
		if (weapons && sUnitInventorySetEquippedWeaponsSearch(weapon, weapons))
		{
			continue;
		}
		StatsListRemoveWeaponStats(container, weapon);
	}

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game) && !bUpdateUI)
	{
		UISetIgnoreStatMsgs(FALSE); 
	}
#endif

	if (!weapons)
	{
		for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ii++)
		{
			inv->pWeapons[ii] = INVALID_ID;
		}
		return TRUE;
	}

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game) && !bUpdateUI)
	{
		UISetIgnoreStatMsgs(TRUE); // make sure that there are no returns between this and where we turn them back on
	}
#endif

	// now equip all weapons in weapons list (it's okay to call StatsListApplyWeaponStats for equipped items)
	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ii++)
	{
		if (weapons[ii] == NULL)
		{
			inv->pWeapons[ii] = INVALID_ID;
		}
		else
		{
			inv->pWeapons[ii] = UnitGetId(weapons[ii]);
		}
		// weapons with no container are clientonly weapons - let's leave them alone, they
		// don't have proper stats.
		if (inv->pWeapons[ii] != INVALID_ID && UnitGetContainer(weapons[ii]))
		{
			StatsListApplyWeaponStats(container, weapons[ii]);
		}
	}

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(container) && !bUpdateUI)
	{
		UISetIgnoreStatMsgs(FALSE); 
	}
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitInventoryGetEquippedWeapons(
	UNIT * container,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ] )
{
	ASSERT_RETURN(container);
	ASSERT_RETURN(pWeapons);

	INVENTORY * inv = container->inventory;
	ASSERTX_RETURN(inv, "Expected inventory");
	INVENTORY_CHECK_DEBUGMAGIC(inv);

	GAME * pGame = UnitGetGame( container );

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pWeapons[ i ] = UnitGetById( pGame, inv->pWeapons[ i ] ); 
	}
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetContainerInRoom(
	UNIT * item)
{
	while (item)
	{
		if (UnitGetRoom(item))
		{
			return item;
		}
		item = UnitGetContainer(item);
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL InventoryXfer(
	UNIT *pContainer,
	GAMECLIENTID idClient,
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bSkipData,
	int *pnUnitsXfered)
{
	ASSERTX_RETFALSE( pContainer, "Expected container unit" );
	GAME *pGame = UnitGetGame( pContainer );
		
	// get the method we're writing the stats in
	UNITSAVEMODE eSaveMode = tHeader.eSaveMode;
	DWORD dwSaveFlags[ NUM_UNIT_SAVE_FLAG_BLOCKS ];
	if (UnitFileGetSaveDescriptors( pContainer, eSaveMode, dwSaveFlags, NULL ) == FALSE)
	{
		ASSERTX_RETFALSE( 0, "Invalid save flags for inventory unit" );
	}
	
	unsigned int nNextBlockCursor = tXfer.GetCursor();
	unsigned int nNextBlock = 0;
	XFER_UINT( tXfer, nNextBlock, STREAM_CURSOR_SIZE_BITS );

	// allow skipping of data when loading if desired
	if (bSkipData)
	{
		ASSERTX_RETFALSE( tXfer.IsLoad(), "Can only skip the inventory data when loading" );
		tXfer.SetCursor( nNextBlock );
		return TRUE;
	}

	// get client
	GAMECLIENT *pClient = ClientGetById( pGame, idClient );
	if (eSaveMode != UNITSAVEMODE_CLIENT_ONLY)
	{
		ASSERTX_RETFALSE( pClient, "InventoryXfer requires a specific client" );
	}
	
	// number of items
	unsigned int nCursorPosNumItems = tXfer.GetCursor();
	unsigned int nNumItems = 0;	
	XFER_UINT( tXfer, nNumItems, STREAM_MAX_INVENTORY_BITS );
	
	// setup xfer spec for each of the inventory items
	UNIT_FILE_XFER_SPEC<mode> tXferInvItemSpec( tXfer );
	tXferInvItemSpec.bIsInInventory = TRUE;
	tXferInvItemSpec.eSaveMode = eSaveMode;
	tXferInvItemSpec.idClient = (pClient != NULL ? ClientGetId( pClient ) : INVALID_GAMECLIENTID);
	if (pnUnitsXfered)
	{
		tXferInvItemSpec.nUnitsXfered = *pnUnitsXfered;
	}
	
	// xfer each item
	if (tXfer.IsSave())
	{
	
		UNIT *pItem = NULL;
		while ((pItem = UnitInventoryIterate(pContainer, pItem, 0, FALSE)) != NULL)
		{
			if (s_UnitCanBeSaved( pItem, TESTBIT( dwSaveFlags, UNITSAVE_BIT_ITEMS_IGNORE_NOSAVE )))
			{
				
				// when saving, set the unit to the item we're saving
				tXferInvItemSpec.pUnitExisting = pItem;
				tXferInvItemSpec.pRoom = UnitGetRoom( pItem );
				tXferInvItemSpec.pContainer = UnitGetContainer( pItem );

				// save item
				if (UnitFileXfer( pGame, tXferInvItemSpec ) != pItem)
				{
					ASSERTV_CONTINUE(
						0,
						"Error with save, please get a programmer now if you can -Colin.  Also please attach character files to bug.  Unable to xfer save unit '%s' [%d] when writing inventory of '%s' [%d] - Buffer size %d/%d - Units Xfered so far: %d",
						UnitGetDevName( pItem ),
						UnitGetId( pItem ),
						UnitGetDevName( pContainer ),
						UnitGetId( pContainer ),
						tXfer.GetCursor(),
						tXfer.GetAllocatedLen(),
						tXferInvItemSpec.nUnitsXfered);
											
				}
				else
				{
				
					// have another item now
					nNumItems++;

				}
												
			}
			
		}	

		// go back and write the # of items written
		int nCursorPosNext = tXfer.GetCursor();
		tXfer.SetCursor( nCursorPosNumItems );
		XFER_UINT( tXfer, nNumItems, STREAM_MAX_INVENTORY_BITS );
		tXfer.SetCursor( nCursorPosNext );
					
	}
	else
	{
	
		// read each unit
		for (unsigned int i = 0; i < nNumItems; ++i)
		{

			// when reading, we have to have a container and their room		
			tXferInvItemSpec.pContainer = pContainer;
			tXferInvItemSpec.pRoom = UnitGetRoom( pContainer );
		
			// load item 
			UNIT *pItem = UnitFileXfer( pGame, tXferInvItemSpec );
			if(pItem == NULL || UnitGetContainer( pItem ) != pContainer )
			{
				const int MAX_MESSAGE = 256;
				char szMessage[ MAX_MESSAGE ];			
				PStrPrintf( 
					szMessage,
					MAX_MESSAGE,
					"Error with load, please attach character files to bug.  Unable to xfer load unit '%s' for inventory of '%s' [%d] - Buffer Size %d/%d - Units Xfered so far: %d",
					pItem ? UnitGetDevName( pItem ) : "UNKNOWN",
					UnitGetDevName( pContainer ),
					UnitGetId( pContainer ),
					tXfer.GetCursor(),
					tXfer.GetAllocatedLen(),
					tXferInvItemSpec.nUnitsXfered);

				// free the unit if we have one
				if (pItem)
				{
					// lets be forgiving and not delete it from the database just yet in the
					// hopes that during a future load this item can be recovered
					UnitSetFlag( pItem, UNITFLAG_NO_DATABASE_OPERATIONS );
					UnitFree( pItem );
					pItem = NULL;
				}
									
				// if this is a version that can handle gracefully skipping bad save data
				// when loaded, then just continue, otherwise bail out
				if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_INCLUDE_BLOCK_SIZES ))
				{
					ASSERTX_CONTINUE( 0, szMessage );
				}
				else
				{
					ASSERTX_RETFALSE( 0, szMessage );
				}
					
			}
			
		}
		
	}		
	
	// go back and write what the position *after* all this inventory data is, we do this
	// so that we can skip the inventory data when desired
	if (tXfer.IsSave())
	{
	
		// go back to position where we write the position of the next block
		unsigned int nCursorCurrent = tXfer.GetCursor();	
		tXfer.SetCursor( nNextBlockCursor );
		
		// do the value of the next block
		nNextBlock = nCursorCurrent;
		XFER_UINT( tXfer, nNextBlock, STREAM_CURSOR_SIZE_BITS );
		
		// set position back to current spot in stream
		tXfer.SetCursor( nCursorCurrent );
	
	}

	// keep running count of how many units have been xfered total
	if (pnUnitsXfered)
	{
		*pnUnitsXfered = tXferInvItemSpec.nUnitsXfered;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
void trace_Inventory(
	UNIT * unit)
{
	ASSERT_RETURN(unit);

	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	trace("inventory for unit: %d: %s\n", UnitGetId(unit), UnitGetDevName(unit));
	if (!unit->inventory)
	{
		trace("empty\n\n");
		return;
	}

	INVENTORY * inv = unit->inventory;

	INVLOC_DATA key;
	key.nInventoryType = UnitGetInventoryType(unit);

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;
		key.nLocation = loc->nLocation;
		INVLOC_DATA* invloc = (INVLOC_DATA*)ExcelGetDataByKey(game, DATATABLE_INVLOC, &key, sizeof(key));
		if(!invloc)
			continue;

		trace("%s (%d items): ", invloc->szName, loc->nCount);

		if (!loc->ppUnitGrid)
		{
			UNIT * iter = loc->itemfirst;
			while (iter)
			{
				trace("%08x", iter);
				iter = iter->invnode->locnext;
				if (iter)
				{
					trace(", ");
				}
			}
			trace("\n");
		}
		else
		{
			trace("grid[%d, %d]:\n", loc->nWidth, loc->nHeight);
			for (int jj = 0; jj < loc->nHeight; jj++)
			{
				UNIT ** ptr = loc->ppUnitGrid + jj * loc->nWidth;
				for (int ii = 0; ii < loc->nWidth; ii++)
				{
					if (*ptr)
					{
						trace("[%08x] ", *ptr);
					}
					else
					{
						trace("[%08s] ", "");
					}
					ptr++;
				}
				trace("\n");
			}
		}
	}
	trace("\n");
}
#endif // _DEBUG


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemMeetsClassReqs(
	const UNIT_DATA * unitdata,
	UNIT * container)
{
	ASSERT_RETFALSE(unitdata);
	ASSERT_RETFALSE(container);

	if (UnitDataTestFlag(container->pUnitData, UNIT_DATA_FLAG_IGNORES_EQUIP_CLASS_REQS))
	{
		return TRUE;
	}

	return IsAllowedUnit(container->nUnitType, unitdata->nContainerUnitTypes, NUM_CONTAINER_UNITTYPES);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemMeetsClassReqs(
	UNIT * unit,
	UNIT * container)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(container);
	if (GameGetDebugFlag(UnitGetGame(unit), DEBUGFLAG_CAN_EQUIP_ALL_ITEMS))
	{
		return TRUE;
	}

	return InventoryItemMeetsClassReqs(unit->pUnitData, container);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemMeetsSkillGroupReqs(
	UNIT * unit,
	UNIT * container)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(container);
	if( !UnitIsA( container, UNITTYPE_PLAYER ) ||
		!UnitIsA( unit, UNITTYPE_SPELLSCROLL ) )
	{
		return TRUE;
	}

	const SKILL_DATA * pSkillData = SkillGetData( UnitGetGame(unit), UnitGetSkillID( unit ) );

	int SkillsOpen = PlayerGetAvailableSkillsOfGroup( UnitGetGame(unit), container, pSkillData->pnSkillGroup[ 0 ] ) -
						 PlayerGetKnownSkillsOfGroup( UnitGetGame(unit), container, pSkillData->pnSkillGroup[ 0 ] );

	return SkillsOpen > 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemHasUseableQuality(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	if (!UnitIsA(unit, UNITTYPE_ITEM))
	{
		return TRUE;
	}

	int quality = ItemGetQuality(unit);
	if (quality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA * quality_data = (const ITEM_QUALITY_DATA *)ExcelGetData(UnitGetGame(unit), DATATABLE_ITEM_QUALITY, quality);
		if (quality_data)
		{
			if (!quality_data->bUseable)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// retval >= 0: amount in excess of the required, < 0: deficient amount
//   so if retval < 0, we can't equip the item because of the stat
// if the stat isn't a requirement, return 0
//----------------------------------------------------------------------------
static int sCheckFeedStat(
	int stat,
	const STATS_DATA * stats_data,
	GAME * game,
	UNIT * container,
	UNIT * item,
	UNIT ** listIgnoreItems,
	unsigned int numIgnoreItems)
{
	int statCapacity = StatsGetAssociatedStat(stats_data, 0);
	ASSERT_RETZERO(statCapacity != INVALID_LINK);
	int statBonus = StatsGetAssociatedStat(stats_data, 1);

	int capacity = UnitGetStat(container, statCapacity);
	if (capacity <= 0)
	{
		return 0;
	}

	int curFeed = UnitGetStat(container, stat);
	int itemFeed = 0;
	if (item && !UnitIsInDyeChain(container, item))
	{
		itemFeed = UnitGetStat(item, stat);
		itemFeed -= (statBonus != INVALID_LINK ? UnitGetStat(item, statBonus) : 0);
	}

	int ignoreFeed = 0;
	for (unsigned int ii = 0; ii < numIgnoreItems; ++ii)
	{
		UNIT * ignoreitem = listIgnoreItems[ii];
		if (ignoreitem)
		{
			ignoreFeed += UnitGetStat(ignoreitem, stat);
			ignoreFeed -= (statBonus != INVALID_LINK ? UnitGetStat(ignoreitem, statBonus) : 0);
		}
	}

	return capacity - (curFeed + itemFeed - ignoreFeed);
}


//----------------------------------------------------------------------------
// retval >= 0: amount in excess of the required, < 0: deficient amount
//   so if retval < 0, we can't equip the item because of the stat
// if the stat isn't a requirement, return 0
//----------------------------------------------------------------------------
static int sCheckRequirementStat(
	int stat,
	const STATS_DATA * stats_data,
	GAME * game,
	UNIT * container,
	UNIT * item,
	UNIT ** listIgnoreItems,
	unsigned int numIgnoreItems)
{
	REF(listIgnoreItems);
	REF(numIgnoreItems);

	int statRequirement = StatsGetAssociatedStat(game, stat, stats_data);
	ASSERT_RETZERO(statRequirement != INVALID_LINK);

	int retval = INT_MAX;

	UNIT_ITERATE_STATS_RANGE(item, stat, statEntry, ii, 32)
	{
		PARAM param = statEntry[ii].GetParam();
		int valStat = statEntry[ii].value;
		int valReq = UnitGetStat(container, statRequirement, param);
		int diff = valReq - valStat;
		if (diff < retval)
		{
			retval = diff;
		}
	}
	UNIT_ITERATE_STATS_END;

	return (retval == INT_MAX ? 0 : retval);
}


//----------------------------------------------------------------------------
// retval >= 0: amount in excess of the required, < 0: deficient amount
//   so if retval < 0, we can't equip the item because of the stat
// if the stat isn't a requirement, return 0
//----------------------------------------------------------------------------
static int sCheckLimitStat(
	int stat,
	const STATS_DATA * stats_data,
	GAME * game,
	UNIT * container,
	UNIT * item,
	UNIT ** listIgnoreItems,
	unsigned int numIgnoreItems)
{
	REF(listIgnoreItems);
	REF(numIgnoreItems);

	int statLimit = StatsGetAssociatedStat(game, stat, stats_data);
	ASSERT_RETZERO(statLimit != INVALID_LINK);

	int valLimit = UnitGetStat(container, statLimit);
	if (valLimit < 0)
	{
		return 0;
	}

	int value = UnitGetStat(item, stat);
	if (value <= 0)
	{
		return 0;
	}

	return value - valLimit;
}


//----------------------------------------------------------------------------
// determine if a given stat prevents an item from being equipped
// retval >= 0: amount in excess of the required, < 0: deficient amount
//   so if retval < 0, we can't equip the item because of the stat
// if the stat isn't a requirement, return 0
//----------------------------------------------------------------------------
int InventoryCheckStatRequirement(
	int stat,
	GAME * game,
	UNIT * container,
	UNIT * item,
	UNIT ** listIgnoreItems,
	unsigned int numIgnoreItems)
{
	ASSERT_RETVAL(container, -1);
	ASSERT_RETVAL(item, 0);

	if (GameGetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS))			// cheat flag
	{
		return 0;
	}

	const STATS_DATA * stats_data = StatsGetData(game, stat);
	ASSERT_RETVAL(stats_data, -1);

	if (!StatsCheckRequirementType(container, stats_data))
	{
		return 0;
	}

	switch (stats_data->m_nStatsType)
	{
	case STATSTYPE_FEED:
		return sCheckFeedStat(stat, stats_data, game, container, item, listIgnoreItems, numIgnoreItems);
		break;
	case STATSTYPE_REQUIREMENT:
		return sCheckRequirementStat(stat, stats_data, game, container, item, listIgnoreItems, numIgnoreItems);
		break;
	case STATSTYPE_REQ_LIMIT:
		return sCheckLimitStat(stat, stats_data, game, container, item, listIgnoreItems, numIgnoreItems);
		break;
	}

	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryCheckStatRequirements(
	UNIT * container,
	UNIT ** ppItems,
	int nNumItems,
	UNIT ** ppIgnoreItems,
	int nNumIgnoreItems,
	DWORD * pdwFailedStats,														// = NULL
	int nStatsArraySize,														// = 0
	DWORD dwEquipFlags /*= 0*/)														
{
	ASSERT_RETFALSE(container);
	GAME * game = UnitGetGame(container);
	ASSERT_RETFALSE(game);

	if (GameGetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS))
	{
		return TRUE;
	}

	// We may need to take an array of locations as well, and check to see if they are equip locations
	int nNumFailed = 0;

	while (container)
	{
		int feed_count = StatsGetNumFeedStats(game);
		for (int ii = 0; ii < feed_count; ii++)
		{
			int feed_stat = StatsGetFeedStat(game, ii);
			const STATS_DATA * feed_data = StatsGetData(game, feed_stat);
			int limit_stat = StatsGetAssociatedStat(feed_data, 0);
			ASSERT_RETFALSE(limit_stat != INVALID_LINK);
			int bonus_stat = StatsGetAssociatedStat(feed_data, 1);

			if (!StatsCheckRequirementType(container, feed_data))
			{
				continue;
			}

			int capacity = UnitGetStat(container, limit_stat);
			if (capacity > 0)
			{
				int cur_feed = UnitGetStat(container, feed_stat);
				
				int nItem = 0;
				while (nItem < nNumItems)
				{
					UNIT *item = ppItems[nItem++];
					if (item)
					{
						// If the item is already equipped, we're gonna assume we met the requirements
						if (!sItemIsEquipped(container, item))
						{
							cur_feed += UnitGetStat(item, feed_stat);
							cur_feed -= (bonus_stat != INVALID_LINK ? UnitGetStat(item, bonus_stat) : 0);
						}
					}
				}

				int nIgnoreItem = 0;
				while (nIgnoreItem < nNumIgnoreItems)
				{
					UNIT *ignoreitem = ppIgnoreItems[nIgnoreItem++];
					if (ignoreitem)
					{
						cur_feed -= UnitGetStat(ignoreitem, feed_stat);
						cur_feed += (bonus_stat != INVALID_LINK ? UnitGetStat(ignoreitem, bonus_stat) : 0);
					}
				}

				if (cur_feed > capacity)
				{
					if (!pdwFailedStats || nNumFailed >= nStatsArraySize)
					{
						return FALSE;
					}
					else
					{
						pdwFailedStats[nNumFailed++] = MAKE_STAT(feed_stat, 0);
					}
				}
			}
		}

		int req_count = StatsGetNumReqStats(game);
		for (int ii = 0; ii < req_count; ii++)
		{
			int req_stat = StatsGetReqStat(game, ii);
			const STATS_DATA * req_data = StatsGetData(game, req_stat);
			int limit_stat = StatsGetAssociatedStat(req_data, 0);
			ASSERT_RETFALSE(limit_stat != INVALID_LINK);
			if (!StatsCheckRequirementType(container, req_data))
			{
				continue;
			}

			for (int iItem = 0; iItem < nNumItems; iItem++)
			{
				UNIT_ITERATE_STATS_RANGE( ppItems[iItem], req_stat, pStatsEntry, jj, 32 )
				{
					PARAM param = pStatsEntry[jj].GetParam();
					int nReqValue = pStatsEntry[jj].value;

					if (nReqValue > UnitGetStat(container, limit_stat, param))
					{
						if (!pdwFailedStats || nNumFailed >= nStatsArraySize)
						{
							return FALSE;
						}
						else
						{
							pdwFailedStats[nNumFailed++] = MAKE_STAT(req_stat, param);
						}
					}
				}
				UNIT_ITERATE_STATS_END;
			}

		}

		int req_limit_count = StatsGetNumReqLimitStats(game);
		for (int ii = 0; ii < req_limit_count; ii++)
		{
			int req_limit_stat = StatsGetReqLimitStat(game, ii);
			const STATS_DATA * req_limit_data = StatsGetData(game, req_limit_stat);
			int limit_stat = StatsGetAssociatedStat(req_limit_data, 0);
			ASSERT_RETFALSE(limit_stat != INVALID_LINK);
			if (!StatsCheckRequirementType(container, req_limit_data))
			{
				continue;
			}

			int capacity = UnitGetStat(container, limit_stat);
			if (capacity > 0)
			{
				int nItem = 0;
				while (nItem < nNumItems)
				{
					UNIT *item = ppItems[nItem++];
					if (item)
					{
						int nReqLimitStat = UnitGetStat(item, req_limit_stat);
						if (nReqLimitStat > 0 && nReqLimitStat < capacity)
						{
							if (!pdwFailedStats || nNumFailed >= nStatsArraySize)
							{
								return FALSE;
							}
							else
							{
								pdwFailedStats[nNumFailed++] = (req_limit_stat, 0);
							}
						}
					}
					item++;
				}
			}
		}

		// If we're trying to put an item in another item that's not in an equip slot, we don't care about the requirements anymore.
		UNIT *pGrandContainer = UnitGetContainer(container);
		if (pGrandContainer)
		{
			if (TESTBIT(dwEquipFlags, IEF_CHECK_REQS_ON_IMMEDIATE_CONTAINTER_ONLY_BIT) ||
				!ItemIsInEquipLocation(pGrandContainer, container))
			{
				break;	//break to the TRUE condition
			}
		}

		container = pGrandContainer;
	}
	return (nNumFailed == 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryCheckStatRequirements(
	struct UNIT * container,
	struct UNIT * item,
	struct UNIT * ignoreitem1,
	struct UNIT * ignoreitem2,
	int location,
	DWORD * pdwFailedStats,														// = NULL
	int nStatsArraySize /*= 0*/,
	DWORD dwEquipFlags /*= 0*/)														
{
	if (location != INVLOC_NONE && !InvLocIsEquipLocation(location, container))
	{
		return TRUE;
	}

	if (!item)
	{
		return FALSE;
	}

	UNIT * ppIgnore[2];
	ppIgnore[0] = ignoreitem1;
	ppIgnore[1] = ignoreitem2;

	return InventoryCheckStatRequirements(container, &item, 1, ppIgnore, 2, pdwFailedStats, nStatsArraySize, dwEquipFlags);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemCanBeEquipped(
	UNIT * container,
	UNIT * item,
	UNIT * ignoreitem1,
	UNIT * ignoreitem2,
	int location,
	DWORD * pdwFailedStats,														// = NULL
	int nStatsArraySize,														// = 0
	DWORD dwItemEquipFlags)														// = 0
{
	if (location != INVLOC_NONE && !InvLocIsEquipLocation(location, container))
	{
		return TRUE;
	}

	if (!item)
	{
		return FALSE;
	}

	UNIT * ppIgnore[2];
	ppIgnore[0] = ignoreitem1;
	ppIgnore[1] = ignoreitem2;

	// unidentified items can never be equipped
	if (ItemIsUnidentified(item))
	{
		return FALSE;
	}

	const UNIT_DATA * item_data = UnitGetData(item);
	ASSERT_RETFALSE(item_data);
	if (UnitDataTestFlag(item_data, UNIT_DATA_FLAG_SUBSCRIBER_ONLY))
	{
		UNIT * ultimateContainer = UnitGetUltimateContainer(container);
		if (ultimateContainer && UnitIsPlayer(ultimateContainer) && !PlayerIsSubscriber(ultimateContainer))
		{
			return FALSE;
		}
	}


	// you cannot equip items that are in invalid slots
	if (TESTBIT(dwItemEquipFlags, IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT) == FALSE)
	{
		if (UnitIsPlayer(container))
		{
			UNIT * oldContainer = UnitGetContainer(item);
			if (oldContainer)
			{
				INVENTORY_LOCATION tInvLocCurrent;
				UnitGetInventoryLocation(item, &tInvLocCurrent);
				if (tInvLocCurrent.nInvLocation != INVLOC_NONE &&
					InventoryLocPlayerCanTake(oldContainer, item, tInvLocCurrent.nInvLocation) == FALSE)
				{
					return FALSE;
				}
			}
		}
	}
	if(UnitDataTestFlag( UnitGetData( container ), UNIT_DATA_FLAG_IGNORES_ITEM_REQUIREMENTS ) )
	{
		return TRUE;
	}

	if (!InventoryItemHasUseableQuality(item))
	{
		return FALSE;
	}

	if (!InventoryItemMeetsClassReqs(item, container))
	{
		return FALSE;
	}

	if (ignoreitem1 != NULL || ignoreitem2 != NULL)
	{
		if (!InventoryCheckStatRequirements(container, &item, 1, ppIgnore, 2, pdwFailedStats, nStatsArraySize, dwItemEquipFlags))
		{
			return FALSE;
		}
	}
	else
	{
		if (!InventoryCheckStatRequirements(container, &item, 1, NULL, 0, pdwFailedStats, nStatsArraySize, dwItemEquipFlags))
		{
			return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryItemCanBeEquipped(
	UNIT * container,
	UNIT * item,
	int nLocation /*= INVLOC_NONE*/,
	DWORD *pdwFailedStats /*= NULL*/,
	int nStatsArraySize /*= 0*/,
	DWORD dwItemEquipFlags /*= 0*/)
{
	return InventoryItemCanBeEquipped(container, item, NULL, NULL, nLocation, pdwFailedStats, nStatsArraySize, dwItemEquipFlags);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UNIT * InventoryGetReplacementItemFromLocation(
	struct UNIT * unit,
	int location,
	int nUnitType,
	int nUnitClass /*=INVALID_ID*/)
{
	ASSERT_RETNULL(unit);
	UNIT * item = NULL;
	UNIT * best = NULL;
	int bestlevel = 0;
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );

	while (TRUE)
	{
		item = UnitInventoryLocationIterate(unit, location, item, dwFlags);
		if (!item)
		{
			break;
		}
		// TRAVIS - In Mythos, we only want to replace with EXACTLY the same item,
		// not something similar.
		if ( ( ( AppIsHellgate() || nUnitClass == INVALID_ID ) &&  UnitIsA(item, nUnitType) ) ||
			 ( AppIsTugboat() && nUnitClass != INVALID_ID && UnitGetClass( item ) == nUnitClass ))
		{
			if (!best)
			{
				best = item;
				bestlevel = UnitGetExperienceLevel(item);
				continue;
			}
			int itemlevel = UnitGetExperienceLevel(item);
			if (itemlevel > bestlevel)
			{
				best = item;
				bestlevel = itemlevel;
			}
		}
	}
	return best;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryDoubleEquip(
	GAME * game,
	UNIT * pContainer,
	UNITID idItem1,
	UNITID idItem2,
	int nLocation1,
	int nLocation2)
{
	ASSERT_RETFALSE(pContainer);

	UNIT *pItem1 = UnitGetById(game, idItem1);
	UNIT *pItem2 = UnitGetById(game, idItem2);

	UNITLOG_ASSERT_RETFALSE(pItem1, pContainer);
	UNITLOG_ASSERT_RETFALSE(pItem2, pContainer);

	if (UnitGetOwnerId(pItem1) != UnitGetId(pContainer) ||
		UnitGetOwnerId(pItem2) != UnitGetId(pContainer))
	{
		UNITLOG_ASSERT(FALSE, pContainer);
		return FALSE;
	}

	if (!IsAllowedUnitInv(pItem1, nLocation1, pContainer) ||
		!IsAllowedUnitInv(pItem2, nLocation2, pContainer))
	{
		UNITLOG_ASSERT(FALSE, pContainer);
		return FALSE;
	}

	UNIT * ppCurrentItem[2];
	UNIT * ppItem[2];
	ppCurrentItem[0] = UnitInventoryGetByLocation(pContainer, nLocation1);
	ppCurrentItem[1] = UnitInventoryGetByLocation(pContainer, nLocation2);
	ppItem[0] = pItem1;
	ppItem[1] = pItem2;

	if (!InventoryCheckStatRequirements(pContainer, &(ppItem[0]), 2, &(ppCurrentItem[0]), 2, NULL, 0, 0 ))
	{
		return FALSE;
	}

	if (IS_CLIENT(game))
	{
#if !ISVERSION(SERVER_VERSION)
		MSG_CCMD_DOUBLEEQUIP msg;
		msg.idItem1 = idItem1;
		msg.idItem2 = idItem2;
		msg.bLocation1 = (BYTE)nLocation1;
		msg.bLocation2 = (BYTE)nLocation2;
		c_SendMessage(CCMD_DOUBLEEQUIP, &msg);
#endif// !ISVERSION(SERVER_VERSION)
	}
	else
	{
		// Do two swaps
		for (int i=0; i < 2; i++)
		{
			int nSrcLoc, nSrcLocX, nSrcLocY;
			if (!UnitGetInventoryLocation(ppItem[i], &nSrcLoc, &nSrcLocX, &nSrcLocY))
			{
				return FALSE;
			}
			int nLocation = (i==0 ? nLocation1 : nLocation2);

			if (ppCurrentItem[i] && !UnitInventoryRemove(ppCurrentItem[i], ITEM_ALL))
			{
				return FALSE;
			}
			if (!UnitInventoryRemove(ppItem[i], ITEM_ALL))
			{
				if (ppCurrentItem[i])
					UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainer, ppCurrentItem[i], nLocation);
				return FALSE;
			}
			if (ppCurrentItem[i] && !UnitInventoryAdd(INV_CMD_PICKUP, pContainer, ppCurrentItem[i], nSrcLoc, nSrcLocX, nSrcLocY))
			{
				UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainer, ppItem[i], nSrcLoc, nSrcLocX, nSrcLocY);
				UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainer, ppCurrentItem[i], nLocation);
				return FALSE;
			}
			if (!UnitInventoryAdd(INV_CMD_PICKUP, pContainer, ppItem[i], nLocation))
			{
				if (ppCurrentItem[i])
				{
					UnitInventoryRemove(ppCurrentItem[i], ITEM_ALL);
					UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainer, ppCurrentItem[i], nLocation);
				}
				UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainer, ppItem[i], nSrcLoc, nSrcLocX, nSrcLocY);
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// this whole system is incredibly ugly, mister!!!  -- phu
//----------------------------------------------------------------------------
void InventoryEquippedTriggerEvent( 
	GAME * game,
	UNIT_EVENT eEvent,
	UNIT * unit,
	UNIT * other,
	void * data)
{
	ASSERT_RETURN(unit);
	INVENTORY * inventory = unit->inventory;
	if (!inventory)
	{
		return;
	}

	UNIT * item = NULL;
	UNIT * next = inventory->equippedfirst;
	while ((item = next) != NULL)
	{
		ASSERT_RETURN(item->invnode);
		next = item->invnode->equipnext;
		UnitTriggerEvent(game, eEvent, item, other, data);				
	}
}


//----------------------------------------------------------------------------
// Play an inventory full sound on the client
//----------------------------------------------------------------------------
void s_SendInventoryFullSound(
	GAME * game,
	UNIT * unit)
{
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(game);
	REF(unit);
#else
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETURN(unit_data);
	if (unit_data->m_nInventoryFullSound == INVALID_ID)
	{
		return;
	}

	GAMECLIENT *pClient = UnitGetClient( unit );


	MSG_SCMD_UNITPLAYSOUND msg;
	msg.id = UnitGetId(unit);
	msg.idSound = unit_data->m_nInventoryFullSound;
	s_SendUnitMessageToClient(unit, ClientGetId(pClient), SCMD_UNITPLAYSOUND, &msg);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetEquippedItemUnusable(
	GAME * game,
	UNIT * unit,
	UNIT * item)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(item);

	// check that item is in unit's inventory
	ASSERT(UnitGetContainer(item) == unit);

	// check that the item is currently modifying the unit
	UNIT * parent = StatsGetParent(item);
	if (parent == NULL)
	{
		return;
	}
	ASSERT_RETURN(parent == unit);

	int location;
	if (UnitGetInventoryLocation(item, &location) &&
		InvLocIsWardrobeLocation(unit, location))
	{
		UnitSetWardrobeChanged(unit, TRUE);
	}

	// remove the item's stats
	sInventoryRemoveItemStats(game, unit, item);

#if !ISVERSION(SERVER_VERSION)
	if (!IS_SERVER(game))
	{
		UISendMessage(WM_INVENTORYCHANGE, UnitGetId(unit), location);
		if (AppIsHellgate() && GameGetControlUnit(game) == unit)
		{
			// tell the player something got removed
			UIShowQuickMessage(L"", 3.0f, StringTableGetStringByKey("ui_equipped_item_removed"));
		}
	}
#endif

	INVENTORY_TRACE3("[%s]  SetEquippedItemUnusable() -- container[%d: %s]  item [%d: %s]", 
		CLTSRVSTR(game), UnitGetId(unit), UnitGetDevName(unit), UnitGetId(item), UnitGetDevName(item));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SendEquippedItemUnusableMsg(
	GAME * game,
	UNIT * unit,
	UNIT * item)
{
	// send a message to any client that knows about the item to un-apply the stats
	if (IS_SERVER(game))
	{
		ASSERT_RETURN(game);
		ASSERT_RETURN(unit);
		ASSERT_RETURN(item);

		// check that item is in unit's inventory
		ASSERT(UnitGetContainer(item) == unit);


		MSG_SCMD_INV_ITEM_UNUSABLE msg;
		msg.idItem = UnitGetId(item);
		s_SendUnitMessage(item, SCMD_INV_ITEM_UNUSABLE, &msg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SetEquippedItemUsable(
	GAME * game,
	UNIT * unit,
	UNIT * item)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(item);

	// check that item is in unit's inventory
	ASSERT(UnitGetContainer(item) == unit);

	// check that the item is currently not modifying the unit
	UNIT * parent = StatsGetParent(item);
	if (parent == unit)
	{
		return;
	}
	ASSERT_RETURN(parent == NULL);

	// check that the item is in a equip slot
	int location;
	ASSERT_RETURN(UnitGetInventoryLocation(item, &location));
	ASSERT_RETURN(InvLocIsEquipLocation(location, unit));

	if (InvLocIsWardrobeLocation(unit, location))
	{
		UnitSetWardrobeChanged(unit, TRUE);
	}

	// apply the item's stats
	sInventoryAddApplyStats(game, unit, item);

#if !ISVERSION(SERVER_VERSION)
	if (!IS_SERVER(game))
	{
		UISendMessage(WM_INVENTORYCHANGE, UnitGetId(unit), location);
	}
#endif

	INVENTORY_TRACE3("[%s]  SetEquippedItemUsable() -- container[%d: %s]  item [%d: %s]", 
		CLTSRVSTR(game), UnitGetId(unit), UnitGetDevName(unit), UnitGetId(item), UnitGetDevName(item));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SendEquippedItemUsableMsg(
	GAME * game,
	UNIT * unit,
	UNIT * item)
{
	// send a message to any client that knows about the item to re-apply the stats
	if (IS_SERVER(game))
	{
		ASSERT_RETURN(game);
		ASSERT_RETURN(unit);
		ASSERT_RETURN(item);

		// check that item is in unit's inventory
		ASSERT(UnitGetContainer(item) == unit);

		MSG_SCMD_INV_ITEM_USABLE msg;
		msg.idItem = UnitGetId(item);
		s_SendUnitMessage(item, SCMD_INV_ITEM_USABLE, &msg);
	}
}

//----------------------------------------------------------------------------
// calculate PVP gear drop chance
//----------------------------------------------------------------------------
int InventoryCalculatePVPGearDrop( 
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETZERO(unit);
	INVENTORY * inventory = unit->inventory;
	if (!inventory)
	{
		return 0;
	}

	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return 0;
	}
	int nChance = 0;
	int nPlayerLevel = UnitGetExperienceLevel( unit	);
	for (int ii = 0; ii < inventory->nLocs; ii++)
	{
		INVLOC * loc = inventory->pLocs + ii;

		if (!sInvLocIsEquipLocation(loc, unit))
		{
			continue;
		}

		int location = loc->nLocation;

		UNIT * item = NULL;
		UNIT * next = UnitInventoryLocationIterate(unit, location, NULL, 0);		// note if we remove an item, this might screw up the iterate!
		while ((item = next) != NULL)
		{
			next = UnitInventoryLocationIterate(unit, location, item, 0);
			int nLevel = UnitGetExperienceLevel(item);
			if( (float)nLevel * 1.5f >= nPlayerLevel - 10)
			{
				nChance += 15;
			}
		}
	}
	return nChance;
}

//----------------------------------------------------------------------------
// verifies that equipped items actually meet requirements, 
// and removes them if they don't.
//----------------------------------------------------------------------------
void s_InventorysVerifyEquipped( 
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	INVENTORY * inventory = unit->inventory;
	if (!inventory)
	{
		return;
	}

	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return;
	}

	// lists of items for which we need to send messages
	PList<UNIT *>	lstUsableItems;
	PList<UNIT *>	lstUnusableItems;
	lstUsableItems.Init();
	lstUnusableItems.Init();

	if( AppIsTugboat() )
	{
		// TRAVIS: Let's equip EVERYTHING to start -
		// then we'll iterate through the inventory, disable each item so that it does not
		// contribute to the stats as we check it, and only disable it if in that case
		// it is no longer allowed
		// That way we guarantee the contribution of stats from items that may be required
		// to equip others.
		for (int ii = 0; ii < inventory->nLocs; ii++)
		{
			INVLOC * loc = inventory->pLocs + ii;

			if (!sInvLocIsEquipLocation(loc, unit))
			{
				continue;
			}

			int location = loc->nLocation;

			UNIT * item = NULL;
			UNIT * next = UnitInventoryLocationIterate(unit, location, NULL, 0);		// note if we remove an item, this might screw up the iterate!
			while ((item = next) != NULL)
			{
				next = UnitInventoryLocationIterate(unit, location, item, 0);

				BOOL bEquipped = sItemIsEquipped(unit, item);

				SetEquippedItemUsable(game, unit, item);
				if( !bEquipped )
				{
					lstUsableItems.PListPushTail(item);
					lstUnusableItems.FindAndDelete(item);
				}
			}
		}
	}
	
	// the general algorithm is to try to equip/unequip items until equilibrium state
	// since equilibrium state may not be possible, we set a maximum number of iterations
	static const unsigned int MAX_INVENTORY_VERIFY_ITERATIONS = 8;

	for (unsigned int iter = 0; iter < MAX_INVENTORY_VERIFY_ITERATIONS; ++iter)
	{
		BOOL bChanged = FALSE;
		for (int ii = 0; ii < inventory->nLocs; ii++)
		{
			INVLOC * loc = inventory->pLocs + ii;

			if (!sInvLocIsEquipLocation(loc, unit))
			{
				continue;
			}

			int location = loc->nLocation;

			UNIT * item = NULL;
			UNIT * next = UnitInventoryLocationIterate(unit, location, NULL, 0);       // note if we remove an item, this might screw up the iterate!
			while ((item = next) != NULL)
			{
				next = UnitInventoryLocationIterate(unit, location, item, 0);

				BOOL bEquipped = sItemIsEquipped(unit, item);
				// we want to make sure we don't include this item's stats in the check
				if( AppIsTugboat() && bEquipped )
				{
					if( StatsGetParent(item) != NULL )
					{
						StatsListRemove(item);
					}
					//SetEquippedItemUnusable(game, unit, item);
				}
				DWORD dwEquipFlags = 0;
				if (UnitTestFlag(unit, UNITFLAG_INVENTORY_LOADING))
					SETBIT(dwEquipFlags, IEF_CHECK_REQS_ON_IMMEDIATE_CONTAINTER_ONLY_BIT);

				BOOL bCanBeEquipped = InventoryItemCanBeEquipped(unit, item, location, NULL, 0, dwEquipFlags);
				// make sure to put us back on after the check - 
				if( AppIsTugboat() && bEquipped )
				{
					if( StatsGetParent(item) != unit )
					{
						if (loc->nFilter > 0)
						{
							BOOL bCombat = !InvLocTestFlag(loc, INVLOCFLAG_WEAPON);
							StatsListAdd(unit, item, bCombat, loc->nFilter);
						}

					}
					//SetEquippedItemUsable(game, unit, item);
				}

				if (bEquipped == bCanBeEquipped)
				{
					continue;
				}

				if (bCanBeEquipped)
				{
					SetEquippedItemUsable(game, unit, item);
					lstUsableItems.PListPushTail(item);
					lstUnusableItems.FindAndDelete(item);
				}
				else
				{
					SetEquippedItemUnusable(game, unit, item);
					lstUnusableItems.PListPushTail(item);
					lstUsableItems.FindAndDelete(item);
				}
				bChanged = TRUE;
			}
		}
		if (!bChanged)
		{
			// equilibrium achieved
			break;
		}
	}

	// now send the final state(s) to the client
	//   delaying it to here this prevents sending multiple equip/unequip messages
	PList<UNIT *>::USER_NODE *pItr = NULL;
	while ((pItr = lstUsableItems.GetNext(pItr)) != NULL)
	{
		SendEquippedItemUsableMsg(game, unit, pItr->Value);
	}

	pItr = NULL;
	while ((pItr = lstUnusableItems.GetNext(pItr)) != NULL)
	{
		SendEquippedItemUnusableMsg(game, unit, pItr->Value);
	}

	lstUsableItems.Destroy(NULL);
	lstUnusableItems.Destroy(NULL);

	if( AppIsTugboat() )
	{
		UnitCalculatePVPGearDrop( unit );
	}
}


//----------------------------------------------------------------------------
// drops equipped items on PVP death!
//----------------------------------------------------------------------------
void s_DropEquippedItemsPVP( 
	GAME * game,
	UNIT * unit,
	int nItems)
{
	ASSERT_RETURN(unit);
	INVENTORY * inventory = unit->inventory;
	if (!inventory)
	{
		return;
	}

	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return;
	}

	// TRAVIS: First, find all the equipped items so that we can randomly pick the ones we need.
	UNIT* pItems[MAX_LOCATIONS];
	int nItemsFound = 0;
	for (int ii = 0; ii < inventory->nLocs; ii++)
	{
		INVLOC * loc = inventory->pLocs + ii;

		if (!sInvLocIsEquipLocation(loc, unit))
		{
			continue;
		}

		int location = loc->nLocation;

		UNIT * item = NULL;
		UNIT * next = UnitInventoryLocationIterate(unit, location, NULL, 0);		// note if we remove an item, this might screw up the iterate!
		while ((item = next) != NULL)
		{
			next = UnitInventoryLocationIterate(unit, location, item, 0);
			if( UnitIsA( item, UNITTYPE_ITEM ) && 
				nItemsFound < MAX_LOCATIONS &&
				!( ItemIsNoTrade(item) || UnitIsNoDrop(item) ||
					UnitIsA( item, UNITTYPE_STARTING_ITEM ) ) )
			{
				pItems[nItemsFound++] = item;
			}
		}
	}

	nItems = MIN( nItems, nItemsFound );
	int nItemsLeft = nItemsFound;
	for( int i = 0; i < nItems; i++ )
	{
		if( nItemsLeft > 0 )
		{
			int nItem = RandGetNum( game, 0, nItemsLeft - 1 );
			UNIT* pItem = pItems[nItem];
			if( pItem )
			{
				pItems[nItem] = pItems[nItemsLeft - 1];
				nItemsLeft--;
				s_ItemDrop( unit, pItem, FALSE, FALSE, TRUE ); // unrestricted drop - anybody can get it!
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryLocPlayerCanPut( 
	UNIT * container,
	UNIT * item,	// may be NULL for a generic "can put anything here" question
	int location)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(location != INVLOC_NONE);

	// get inv loc
	const INVLOC_DATA * loc_data = UnitGetInvLocData(container, location);
	ASSERT_RETFALSE(loc_data);

	GAME * game = UnitGetGame(container);
	ASSERT_RETFALSE(game);

	int codelen = 0;
	BYTE * condition = ExcelGetScriptCode(game, DATATABLE_INVLOC, loc_data->codePlayerPutRestricted, &codelen);
	if (condition)
	{
		if (VMExecIUnitObjectBuffer(game, container, item, condition, codelen) != 0)
		{
			return FALSE;
		}
	}

	if (InvLocIsStashLocation(container, location) &&
		!StashIsOpen(container))
	{
		return FALSE;
	}

	// check the container's location too
	UNIT * containerParent = UnitGetContainer(container);
	if (containerParent)
	{
		if (UnitIsPhysicallyInContainer(container))
		{
			INVENTORY_LOCATION invloc;
			ASSERT_RETFALSE(UnitGetInventoryLocation(container, &invloc));
			return InventoryLocPlayerCanPut(containerParent, item, invloc.nInvLocation);
		}
	}
			
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryLocPlayerCanTake( 
	UNIT *pContainer,
	UNIT * item,
	int nInvLoc)
{
	ASSERTX_RETFALSE( pContainer, "Expected container unit" );
	ASSERTX_RETFALSE( nInvLoc != INVLOC_NONE, "Expected inventory location" );

	// get inv loc
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInvLoc );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );

	GAME *pGame = UnitGetGame(pContainer);
	int codelen = 0;
	BYTE * condition = ExcelGetScriptCode(pGame, DATATABLE_INVLOC, pInvLocData->codePlayerTakeRestricted, &codelen);
	if (condition)
	{
		if (VMExecIUnitObjectBuffer(pGame, pContainer, item, condition, codelen) != 0)
		{
			return FALSE;
		}
	}


	// check the container's location too
	UNIT *pContainerParent = UnitGetContainer( pContainer );
	if (pContainerParent)
	{
		if (UnitIsPhysicallyInContainer(pContainer))
		{
			INVENTORY_LOCATION tInvLoc;
			ASSERTX_RETFALSE( UnitGetInventoryLocation( pContainer, &tInvLoc ), "Expected inv loc" );
			return InventoryLocPlayerCanTake( pContainerParent, item, tInvLoc.nInvLocation );
		}
	}
			
	return TRUE;
		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryTestPut(
	UNIT * unit, 
	UNIT * item, 
	int location, 
	int x, 
	int y,
	DWORD dwChangeLocationFlags)
{
	if (!item)
	{
		return FALSE;
	}

	// when you're a ghost you can't move stuff around
	if (unit && UnitIsGhost(unit))
	{
		return FALSE;
	}
		
	// we need to have some sort of validation here that prevents players from
	// hacking the network messages and moving items from the personal store, quest rewards
	// task rewards, offers, any of the hidden inventory like that into their actual real inventory
	// unless they are in the act of being allowed to take those items from the slot -Colin
	
	if (TEST_MASK(dwChangeLocationFlags, CLF_ALLOW_NEW_CONTAINER_BIT) == FALSE)
	{
		if (UnitGetUltimateOwner(item) != UnitGetUltimateOwner(unit))
		{
			return FALSE;
		}
	}
	
	if (!UnitInventoryTest(unit, item, location, x, y, dwChangeLocationFlags))
	{
		return FALSE;
	}
	
	DWORD dwItemEquipFlags = 0;
	if (TEST_MASK(dwChangeLocationFlags, CLF_ALLOW_EQUIP_WHEN_IN_PLAYER_PUT_RESTRICTED_LOC_BIT))
	{
		SETBIT(dwItemEquipFlags, IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT);
	}
	
	if (!InventoryItemCanBeEquipped(unit, item, location, NULL, 0, dwItemEquipFlags))
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL INV_FUNCNAME(InventoryChangeLocation)(INV_FUNCFLARGS(
	UNIT * unit,
	UNIT * item,
	int location,
	int x,
	int y,
	DWORD dwChangeLocationFlags,
	INV_CONTEXT context /*= INV_CONTEXT_NONE*/))
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(item);
	
	if (IS_SERVER(item) && UnitGetStat(item, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		return FALSE;
	}
	
	// test for putting things into weapon config slots ... don't allow class restricted items	
	const INVLOC_DATA * invloc_data = UnitGetInvLocData(unit, location);
	if (!invloc_data)
	{
		return FALSE;
	}

	// test if we can put the unit at the requested location
	BOOL bIsConfigSwap = TEST_MASK(dwChangeLocationFlags, CLF_CONFIG_SWAP_BIT);
	if (bIsConfigSwap == FALSE &&
		invloc_data->bCheckClass == TRUE)
	{
		if (InventoryItemMeetsClassReqs(item, unit) == FALSE)
		{
			return FALSE;
		}
	}
	
	if (bIsConfigSwap == FALSE)
	{
		// apparently (-1, -1), means to look for a spot, so we won't test that
		if (x != -1 && y != -1)
		{
			if (InventoryTestPut(unit, item, location, x, y, dwChangeLocationFlags) == FALSE)
			{
				return FALSE;
			}
		}
	}

	// put item at this location
	return INV_FUNCNAME(UnitInventoryAdd)(INV_FUNC_FLARGS(file, line, INV_CMD_PICKUP, unit, item, location, x, y, dwChangeLocationFlags, TIME_ZERO, context));
}

 
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sInventoryOffSwap(
	UNIT * unit,
	UNIT * itemSrc,
	UNIT * itemDest)
{
	ASSERT_RETFALSE(unit && !IsUnitDeadOrDying(unit));
	
	int src_loc, src_x, src_y;
	if (!UnitGetInventoryLocation(itemSrc, &src_loc, &src_x, &src_y))
	{
		return FALSE;
	}
	int dest_loc, dest_x, dest_y;
	if (!UnitGetInventoryLocation(itemDest, &dest_loc, &dest_x, &dest_y))
	{
		return FALSE;
	}
	INVLOC_HEADER loc_info;
	if (!UnitInventoryGetLocInfo(unit, dest_loc, &loc_info))
	{
		return FALSE;
	}
	if (loc_info.nPreventLocation1 <= INVALID_LINK)
	{
		return FALSE;
	}
	if (!InventoryItemCanBeEquipped(unit, itemSrc, itemDest, NULL, dest_loc))
	{
		return FALSE;
	}
	if (!UnitInventoryRemove(itemSrc, ITEM_ALL))
	{
		return FALSE;
	}
	if (!UnitInventoryRemove(itemDest, ITEM_ALL))
	{
		// cant remove dest, put source back
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, unit, itemSrc, src_loc, src_x, src_y);
		return FALSE;  // failure
	}
	if (!UnitInventoryAdd(INV_CMD_PICKUP, unit, itemSrc, loc_info.nPreventLocation1, CLF_SWAP_BIT))
	{
		// cant add source to new loc, put both items back where they were
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, unit, itemSrc, src_loc, src_x, src_y);
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, unit, itemDest, dest_loc, dest_x, dest_y);
		return FALSE;
	}
	if (!UnitInventoryAdd(INV_CMD_PICKUP, unit, itemDest, src_loc, src_x, src_y, CLF_SWAP_BIT))
	{
		// can't put dest at source loc, put both items back where they were
		UnitInventoryRemove(itemSrc, ITEM_ALL);
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, unit, itemSrc, src_loc, src_x, src_y);
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, unit, itemDest, dest_loc, dest_x, dest_y);
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventorySwapLocation(
	UNIT *itemSrc,
	UNIT *itemDest,
	int nAltDestX,																// = -1
	int nAltDestY,																// = -1
	DWORD dwChangeLocationFlags)												// = 0
{

	// validate items
	if (itemSrc == NULL || itemDest == NULL)
	{
		return FALSE;
	}
	
	if (UnitGetStat(itemSrc, STATS_ITEM_LOCKED_IN_INVENTORY) ||
		UnitGetStat(itemDest, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		return FALSE;
	}

	// get ultimate containers
	UNIT *pUltimateSource = UnitGetUltimateContainer( itemSrc );
	UNIT *pUltimateDest = UnitGetUltimateContainer( itemDest );	

	// you can only swap two units inventory locations if they are both under the same owner
	if (pUltimateSource != pUltimateDest)
	{
		return FALSE;
	}
	
	// alias to owner
	UNIT *pOwner = pUltimateSource;
	ASSERTX_RETFALSE( pOwner, "Swapping inventory locations with no owner" );
	
	// can't do then when yer dead!
	if (IsUnitDeadOrDying( pOwner ))
	{
		return FALSE;
	}
	
	// handle item stacking
	if (ItemStack(itemDest, itemSrc) != ITEM_STACK_NO_STACK)
	{
		// item was "swapped", the stack absorbed the item, no additional network messages are needed
		return TRUE;
	}

	UNIT * container1 = UnitGetContainer(itemSrc);
	if (!container1)
	{
		return FALSE;
	}
	int oldSrcLoc, oldSrcX, oldSrcY;
	UnitGetInventoryLocation(itemSrc, &oldSrcLoc, &oldSrcX, &oldSrcY);

	UNIT * container2 = UnitGetContainer(itemDest);
	if (!container2)
	{
		return FALSE;
	}
	int oldDestLoc, oldDestX, oldDestY;
	UnitGetInventoryLocation(itemDest, &oldDestLoc, &oldDestX, &oldDestY);
	if( nAltDestX != -1 && nAltDestY != -1 )
	{
		oldDestX = nAltDestX;
		oldDestY = nAltDestY;
	}
	if (oldSrcLoc == INVLOC_WEAPONCONFIG && oldDestLoc == INVLOC_WEAPONCONFIG)
	{
		// yeah yeah.  Position inside this invloc doesn't matter at 
		// all and swapping might cause problems.	
		return TRUE;
	}

	if (oldSrcLoc == INVLOC_WEAPONCONFIG || oldDestLoc == INVLOC_WEAPONCONFIG)
	{
		// don't allow swapping with the secret inventory with this message.
		//  This was apparently happening via very fast weaponconfig switches while clicking 
		//  on the paperdoll hand slot.

		if (!TEST_MASK(dwChangeLocationFlags, CLF_CONFIG_SWAP_BIT))	// BSP 12/3/07 added this exception because preventing
																	// this prevents a one-for-one swapping of an item in to a non-current
																	//  weaponconfig.
		{
			return FALSE;
		}
	}

	if ((oldSrcLoc == INVLOC_STASH || oldDestLoc == INVLOC_STASH) &&
		!sInventoryCanUseStash(pOwner))
	{
		return FALSE;
	}

	// do "off swapping"
	BOOL bSwapped = FALSE;
	// test if we can put the unit at the requested location
	BOOL bIsConfigSwap = TEST_MASK( dwChangeLocationFlags, CLF_CONFIG_SWAP_BIT );
	BOOL bTestSwap = UnitInventoryTestSwap(itemSrc, itemDest);
	if ( !bIsConfigSwap && !bTestSwap)
	{
		// I think this is to try to swap preventing items (two-handed and such)
		//  not sure if it's ever used
		bSwapped = sInventoryOffSwap(pOwner, itemSrc, itemDest);
	}

	if (!TEST_MASK( dwChangeLocationFlags, CLF_DISALLOW_ALT_GRID_PLACEMENT ) && !bSwapped && !bTestSwap)
	{
		if (!UnitInventoryTestIgnoreItem(container2, itemSrc, itemDest, oldDestLoc, oldDestX, oldDestY))
		{
			int oldDestX2 = 0, oldDestY2 = 0;
			if (UnitInventoryFindSpace(container2, itemSrc, oldDestLoc, &oldDestX2, &oldDestY2, dwChangeLocationFlags, itemDest))
			{
				oldDestX = oldDestX2;
				oldDestY = oldDestY2;
			}
		}
		if (!UnitInventoryTestIgnoreItem(container1, itemDest, itemSrc, oldSrcLoc, oldSrcX, oldSrcY))
		{
			int oldSrcX2 = 0, oldSrcY2 = 0;
			if (UnitInventoryFindSpace(container1, itemDest, oldSrcLoc, &oldSrcX2, &oldSrcY2, dwChangeLocationFlags, itemSrc))
			{
				oldSrcX = oldSrcX2;
				oldSrcY = oldSrcY2;
			}
		}
	}

	if (bSwapped == FALSE)
	{
	
		if( !TEST_MASK( dwChangeLocationFlags, CLF_IGNORE_REQUIREMENTS_ON_SWAP ) )
		{
			if (!InventoryItemCanBeEquipped(container2, itemSrc, itemDest, NULL, oldDestLoc))
			{
				return FALSE;
			}

			if (!InventoryItemCanBeEquipped(container1, itemDest, itemSrc, NULL, oldSrcLoc))
			{
				return FALSE;
			}
		}

		BOOL bRemoveFromHotkeys = (container1 != container2) && !bIsConfigSwap;

		if (!sUnitInventoryRemove(INV_FUNC_FILELINE(itemSrc, ITEM_ALL, dwChangeLocationFlags | CLF_FORCE_SEND_REMOVE_MESSAGE  | CLF_SWAP_BIT, bRemoveFromHotkeys, INV_CONTEXT_NONE)))
		{
			ASSERT(0);
			return FALSE;
		}
		if (!sUnitInventoryRemove(INV_FUNC_FILELINE(itemDest, ITEM_ALL, dwChangeLocationFlags | CLF_FORCE_SEND_REMOVE_MESSAGE | CLF_SWAP_BIT, bRemoveFromHotkeys, INV_CONTEXT_NONE)))
		{
			ASSERT(0);
			UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container1, itemSrc, oldSrcLoc, oldSrcX, oldSrcY);
			return FALSE;
		}
		if( AppIsHellgate() )
		{
			if (!UnitInventoryAdd(INV_CMD_PICKUP, container2, itemSrc, oldDestLoc, oldDestX, oldDestY, dwChangeLocationFlags | CLF_SWAP_BIT))
			{
				ASSERT(0);
				return FALSE;
			}
			if (!UnitInventoryAdd(INV_CMD_PICKUP, container1, itemDest, oldSrcLoc, oldSrcX, oldSrcY, dwChangeLocationFlags | CLF_SWAP_BIT))
			{
				ASSERT(0);
				return FALSE;
			}
		}
		// because Tug uses stat requirements, we get lots more interesting
		// cases to handle...
		if( AppIsTugboat() )
		{
			if (!UnitInventoryAdd(INV_CMD_PICKUP, container2, itemSrc, oldDestLoc, oldDestX, oldDestY, CLF_SWAP_BIT))
			{
				// doh! failed to add on the swap, probably because the required stat went
				// down after the unequip!
				int new_loc, new_x, new_y;
				int eResult( DR_OK );
				if (!ItemGetOpenLocation(container2, itemSrc, TRUE, &new_loc, &new_x, &new_y))
				{
	
					s_SendInventoryFullSound( UnitGetGame( pOwner ), pOwner );
					// crap! let's drop it.
					eResult = s_ItemDrop( pOwner, itemSrc );
				}
				if( eResult != DR_OK_DESTROYED )	//this could occure for TUgboat
				{
					// lock the item to this player
					ItemLockForPlayer( itemSrc, pOwner );
					UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container2, itemSrc, new_loc, new_x, new_y);
				}
			}
			if (!UnitInventoryAdd(INV_CMD_PICKUP, container1, itemDest, oldSrcLoc, oldSrcX, oldSrcY, CLF_SWAP_BIT))
			{
				// doh! failed to add on the swap, probably because the required stat went
				// down after the unequip!
				int new_loc, new_x, new_y;
				DROP_RESULT eResult( DR_OK );
				if (!ItemGetOpenLocationNoEquip(container1, itemDest, TRUE, &new_loc, &new_x, &new_y))
				{
					// crap! let's drop it.
					s_SendInventoryFullSound( UnitGetGame( pOwner ), pOwner );
					eResult = s_ItemDrop( pOwner, itemDest );
				}
				if( eResult != DR_OK_DESTROYED )	//this never should be but just in case
				{
					// lock the item to this player
					ItemLockForPlayer( itemDest, pOwner );
					UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container1, itemDest, new_loc, new_x, new_y);
				}
			}
		}
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryMoveToAnywhereInLocation(
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLocation,
	DWORD dwChangeInvFlags)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );
	ASSERTX_RETFALSE( pItem, "Expected item" );

	// find a grid space
	int x, y;
	if (UnitInventoryFindSpace( pContainer, pItem, nInvLocation, &x, &y ))
	{
		// move the item to the inventory
		return InventoryChangeLocation( pContainer, pItem, nInvLocation, x, y, dwChangeInvFlags );	
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsTradeLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bTradeLocation;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsPetLocation(
	UNIT *pContainer,
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return TESTBIT(pInvLocData->dwFlags, INVLOCFLAG_PETSLOT);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsRewardLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bRewardLocation;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsServerOnlyLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bServerOnyLocation;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsOnPersonLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bOnPersonLocation;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsStandardLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bStandardLocation;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsAllowedHotkeySourceLocation(
	UNIT *pContainer, 
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bAllowedHotkeySourceLocation;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//broadcasts an event such as equip to all the items in an inventory.
//or say broadcast an event to make all items of a certain type glow for a second
//returns count of items
BOOL InvBroadcastEventOnUnitsByFlags( 
	UNIT *pUnit,
	UNIT_EVENT eventID,
	DWORD flags )
{
	ASSERTX_RETFALSE( pUnit, "expected unit" );	
			
	UNIT * pItemCurr = UnitInventoryIterate( pUnit, NULL, flags);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pUnit, pItemCurr, flags );
		if( pItemCurr )
		{
			UnitTriggerEvent( UnitGetGame( pUnit ), eventID, pUnit, pItemCurr, NULL ); //broadcast the event	
		}
		pItemCurr = pItemNext;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InventoryDropAll( 
	UNIT *pUnit )
{
	ASSERTX_RETURN( pUnit, "expected unit" );	

	UNIT * pItemCurr = UnitInventoryIterate( pUnit, NULL, 0, FALSE);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pUnit, pItemCurr, 0, FALSE );
		if( pItemCurr )
		{
			s_ItemDrop( pUnit, pItemCurr );
		}
		pItemCurr = pItemNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsStoreLocation( 
	UNIT *pContainer,
	int nLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// awww shucks ... rejected
	if (nLocation == INVALID_LINK)
	{
		return FALSE;
	}
	
	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	
	// test for active personal store bit
	return InvLocTestFlag( pInvLocData, INVLOCFLAG_STORE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsWarehouseLocation( 
	UNIT *pContainer,
	int nLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	
	// test for active personal store bit
	return InvLocTestFlag( pInvLocData, INVLOCFLAG_MERCHANT_WAREHOUSE );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsOnlyKnownWhenStashOpen(
	UNIT *pContainer,
	int nLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	
	// test for active personal store bit
	return InvLocTestFlag( pInvLocData, INVLOCFLAG_ONLY_KNOWN_WHEN_STASH_OPEN );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InvLocIsStashLocation(
	UNIT *pContainer,
	int nLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	
	// test for active personal store bit
	return InvLocTestFlag( pInvLocData, INVLOCFLAG_STASH_LOCATION );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInIngredientsLocation(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	UNIT *pContainer = UnitGetContainer( pUnit );
	if (pContainer)
	{
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation( pUnit, &tInvLoc );
		UNIT *pContainer = UnitGetContainer( pUnit );
		return InventoryIsIngredientLocation( pContainer, tInvLoc.nInvLocation );					
	}	
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsIngredientLocation(
	UNIT *pContainer,
	int nInventoryLocation)
{
	ASSERTX_RETFALSE( pContainer, "Expected unit" );
	
	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventoryLocation );
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );

	// test for active personal store bit
	return InvLocTestFlag( pInvLocData, INVLOCFLAG_INGREDIENTS );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInTradeLocation( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// is it a trade slot
		if (InventoryIsTradeLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInPetLocation( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// check all the way up the containment chain if we are in a slot that is considered
	// a pet slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{

		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// is it a pet slot
		if (InventoryIsPetLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}

		// unit is now the container
		pUnit = pContainer;

		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );

	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInNoSaveLocation(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// is a no save slot
		if (UnitInventoryTestFlag( pContainer, nInvLocation, INVLOCFLAG_DONTSAVE ))
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInServerOnlyLocation( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// is it a reward slot
		if (InvLocIsServerOnlyLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInStashLocation( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		if (nInvLocation == INVLOC_NONE)
		{
			return FALSE;
		}

		// is it a standard location
		if (InvLocIsStashLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInStandardLocation( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		if (nInvLocation == INVLOC_NONE)
		{
			return FALSE;
		}

		// is it a standard location
		if (InvLocIsStandardLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInLocationType( 
	UNIT *pUnit,
	LOCATION_TYPE eType,
	BOOL bFollowContainerChain /*= TRUE*/)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		if (nInvLocation == INVLOC_NONE)
		{
			return FALSE;
		}

		// get inventory location information
		const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInvLocation );
		ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );

		// filter
		switch (eType)
		{
		
			//----------------------------------------------------------------------------
			case LT_STANDARD:
			{
				if (InvLocIsStandardLocation( pContainer, nInvLocation ) == TRUE)
				{
					return TRUE;
				}
				break;
			}
			
			//----------------------------------------------------------------------------
			case LT_ON_PERSON:
			{
				if (InvLocIsOnPersonLocation( pContainer, nInvLocation ) == TRUE)
				{
					return TRUE;
				}
				break;
			}
			
			//----------------------------------------------------------------------------
			case LT_CACHE_DISABLING:
			{
				if (UnitInventoryTestFlag( pContainer, nInvLocation, INVLOCFLAG_CACHE_DISABLING ))
				{
					return TRUE;
				}
				break;
			}

			//----------------------------------------------------------------------------
			case LT_CACHE_ENABLING:
			{
				if (UnitInventoryTestFlag( pContainer, nInvLocation, INVLOCFLAG_CACHE_ENABLING ))
				{
					return TRUE;
				}
				break;
			}
		
		}
		
		// if we're not following up the containment chain, bail out now
		if (bFollowContainerChain == FALSE)
		{
			break;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryIsInAllowedHotkeySourceLocation(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check all the way up the containment chain if we are in a slot that is considered
	// a trade slot
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
		
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// is it a standard location
		if (InventoryIsAllowedHotkeySourceLocation( pContainer, nInvLocation ) == TRUE)
		{
			return TRUE;
		}
		
		// unit is now the container
		pUnit = pContainer;
		
		// go up one containment level from the container
		pContainer = UnitGetContainer( pContainer );
		
	}
		
	return FALSE;

}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int InventoryGetRewardLocation(
	UNIT * unit)
{
	ASSERT_RETVAL(unit, INVLOC_NONE);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETVAL(game, INVLOC_NONE);

	int nInventoryIndex = UnitGetInventoryType(unit);
	if (nInventoryIndex <= 0)
	{
		return INVLOC_NONE;
	}

	const INVENTORY_DATA * inv_data = InventoryGetData(UnitGetGame(unit), nInventoryIndex);
	ASSERT_RETVAL(inv_data, INVLOC_NONE);

	for (int ii = inv_data->nFirstLoc; ii < inv_data->nFirstLoc + inv_data->nNumLocs; ii++)
	{
		const INVLOC_DATA * locdata = InvLocGetData(game, ii);
		if(!locdata)
			continue;

		if (locdata->bRewardLocation == TRUE)
		{
			return locdata->nLocation;
		}
	}

	return INVLOC_NONE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int InventoryGetTradeLocation(
	UNIT * unit)
{
	ASSERT_RETVAL(unit, INVLOC_NONE);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETVAL(game, INVLOC_NONE);

	int nInventoryIndex = UnitGetInventoryType(unit);
	if (nInventoryIndex <= 0)
	{
		return INVLOC_NONE;
	}

	const INVENTORY_DATA * inv_data = InventoryGetData(UnitGetGame(unit), nInventoryIndex);
	ASSERT_RETVAL(inv_data, INVLOC_NONE);

	for (int ii = inv_data->nFirstLoc; ii < inv_data->nFirstLoc + inv_data->nNumLocs; ii++)
	{
		const INVLOC_DATA * locdata = InvLocGetData(game, ii);

		if (locdata && locdata->bTradeLocation == TRUE)
		{
			return locdata->nLocation;
		}
	}

	return INVLOC_NONE;
}

		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sFilterInventorySend(
	GAMECLIENT *pClient,
	UNIT *pContainer,
	UNIT *pItem,
	DWORD dwFlags)		// see INVENTORY_SEND_FLAGS
{
	ASSERTX_RETFALSE( pContainer, "Expected container" );
	ASSERTX_RETFALSE( pItem, "Expected item" );

	// check for equipped only
	if (TESTBIT( dwFlags, ISF_EQUIPPED_ONLY_BIT ))
	{
		if (ItemIsInEquipLocation( pContainer, pItem ) == FALSE)
		{
			return FALSE;
		}
	}

	if (TESTBIT( dwFlags, ISF_CLIENT_CANT_KNOW_BIT ) ) 
	{
		if( ClientCanKnowUnit( pClient, pItem, TRUE ) ) 
		{
			return FALSE;
		}
	}

	if (TESTBIT( dwFlags, ISF_CLIENT_CAN_KNOW_BIT ) ) 
	{
		if( !ClientCanKnowUnit( pClient, pItem, TRUE ) )
		{
			return FALSE;
		}
	}

	// check for usable by class
	if (TESTBIT( dwFlags, ISF_USABLE_BY_CLASS_ONLY_BIT ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		ASSERTX_RETFALSE( pPlayer, "Expected player" );
		
		// check is allowed unit for items
		if (InventoryItemMeetsClassReqs( pItem, pPlayer ) == FALSE)
		{
			return FALSE;
		}
		
	}
			
	// ok to send
	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InventorySendToClient( 
	UNIT *pContainer, 
	GAMECLIENT *pClient,
	DWORD dwFlags)
{
	ASSERTX_RETURN( pContainer, "Expected unit" );
	ASSERTX_RETURN( pClient, "Expected client" );
	
	// iterate the unit inventory
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate( pContainer, pItem )) != NULL)
	{
		// does the item pass the filter based on flags
		if (s_sFilterInventorySend( pClient, pContainer, pItem, dwFlags ))
		{
			s_SendUnitToClient( pItem, pClient );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InventoryRemoveFromClient( 
	UNIT *pContainer, 
	GAMECLIENT *pClient,
	DWORD dwFlags)
{
	ASSERTX_RETURN( pContainer, "Expected unit" );
	ASSERTX_RETURN( pClient, "Expected client" );
	
	// iterate the unit inventory
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate( pContainer, pItem )) != NULL)
	{
		// does the item pass the filter based on flags
		if (s_sFilterInventorySend( pClient, pContainer, pItem, dwFlags ))
		{
			if( !ClientCanKnowUnit( pClient, pItem, TRUE ) )
			{
				s_RemoveUnitFromClient( pItem, pClient, 0 );
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InventoryChanged( 
	UNIT * unit)
{
	INVENTORY_TRACE3("s_InventoryChanged()");
	ASSERT_RETURN(unit);
	ASSERT_RETURN(IS_SERVER(unit));

	if (UnitTestFlag(unit, UNITFLAG_FREED))
	{
		return;
	}
		
	if (!UnitIsA(unit, UNITTYPE_ITEM) &&
		!UnitTestFlag(unit, UNITFLAG_INVENTORY_LOADING))
	{
		// verify equip requirements
		s_InventorysVerifyEquipped(UnitGetGame(unit), unit);		
	}

	// update client hot keys if necessary of the owner
	s_UnitUpdateWeaponConfig(unit);

	// do any wardrobe update if necessary
	WardrobeUpdateFromInventory(unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogInventoryLinkError( 
	UNIT *pUnit,
	UNIT *pContainer,
	const INVENTORY_LOCATION &tInvLoc,
	const char *pszHeader)
{
	ASSERTV_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( pContainer, "Expected container unit" );

#if !ISVERSION(SERVER_VERSION)
		LogMessage(
			ERROR_LOG, "%s: Unable to link unit [%d: %s] and place in the inventory of [%d: %s]",
			pszHeader,
			UnitGetId( pUnit ), 
			UnitGetDevName( pUnit ), 
			UnitGetId( pContainer ), 
			UnitGetDevName( pContainer ));
#else
		TraceError(
			"%s: Unable to link unit [%d: %s] and place in the inventory of [%d: %s]",
			pszHeader,
			UnitGetId( pUnit ), 
			UnitGetDevName( pUnit ), 
			UnitGetId( pContainer ), 
			UnitGetDevName( pContainer ));
#endif
	
}

//----------------------------------------------------------------------------
enum INVLOC_FALLBACK_TYPE
{
	IFT_FALLBACK,
	IFT_LOST_ITEM,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static INVENTORY_LOCATION sGetLoadFallbackInventoryLocation( 
	UNIT *pContainer,
	UNIT *pItem,
	const INVENTORY_LOCATION &tInvLoc,
	INVLOC_FALLBACK_TYPE eType)
{

	// init result
	INVENTORY_LOCATION tInvLocFallback;
	tInvLocFallback.nInvLocation = INVALID_LINK;

	// validate
	ASSERTV_RETVAL( pContainer, tInvLocFallback, "Expected container" );
	ASSERTV_RETVAL( pItem, tInvLocFallback, "Expected unit" );
	
	// check datatable for a fallback location
	int nInvLocFallback = INVALID_LINK;
	if (eType == IFT_FALLBACK)
	{
		const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, tInvLoc.nInvLocation );
		if (pInvLocData)
		{
			nInvLocFallback = pInvLocData->nInvLocLoadFallbackOnLoadError;		
		}
	}
	else if (eType == IFT_LOST_ITEM)
	{
		// only players are allowed to do this
		UNIT *pUltimateContainer = UnitGetUltimateContainer( pContainer );
		if (UnitIsPlayer( pUltimateContainer ))
		{
			nInvLocFallback = INVLOC_LOST_UNIT;  // a special slot that we accumulate lost items in
		}
	}

	// find space in the slot
	if (nInvLocFallback != INVALID_LINK)
	{
		// find space for it
		if (UnitInventoryFindSpace( 
					pContainer, 
					pItem,
					nInvLocFallback,
					&tInvLocFallback.nX,
					&tInvLocFallback.nY))
		{
			// set location now that we have a x,y
			tInvLocFallback.nInvLocation = nInvLocFallback;
		}	
		
	}
	
	// return the location
	return tInvLocFallback;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITID sFindOldestLostUnitIfFull( 
	UNIT *pContainer)
{
	ASSERTX_RETINVALID( pContainer, "Expected unit" );

	if (UnitIsPlayer( pContainer ))
	{
	
		// how many items can we fit in the lost unit slot
		const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, INVLOC_LOST_UNIT );
		ASSERTX_RETINVALID( pInvLocData, "Expected inv loc data" );
		ASSERTX_RETINVALID( InvLocTestFlag( pInvLocData, INVLOCFLAG_ONESIZE ), "Expected one size inv slot" );
		int nMaxUnits = pInvLocData->nWidth * pInvLocData->nHeight;

		// setup our search results	
		int nCount = 0;
		UNITID idUnitOldest = INVALID_ID;
		time_t utcLostAtOldest = 0;
		
		// iterate the inventory slot
		UNIT *pItem = NULL;
		while ((pItem = UnitInventoryLocationIterate( pContainer, 
													  INVLOC_LOST_UNIT, 
													  pItem, 
													  0, 
													  FALSE )) != NULL)
		{
		
			// increase count
			nCount++;
			
			// keep track of the oldest item
			time_t utcLostAt = UnitGetLostAtTime( pItem );			
			if (idUnitOldest == INVALID_ID ||
				utcLostAt < utcLostAtOldest)
			{
				idUnitOldest = UnitGetId( pItem );
				utcLostAtOldest = utcLostAt;
			}
			
		}
		
		// we will only return the oldest unit if the inventory slot is full
		if (nCount >= nMaxUnits)
		{
			return idUnitOldest;
		}
						
	}
	
	return INVALID_ID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InventoryLinkUnitToInventory(			
	UNIT * unit,
	UNIT * container,
	const INVENTORY_LOCATION & tInvLoc,
	TIME tiEquipTime,
	BOOL bAllowFallback,
	DWORD dwReadUnitFlags)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(container);	
	GAME *pGame = UnitGetGame( unit );
	
	if (UnitIsContainedBy(unit, container) == FALSE)
	{	
		BOOL bResult = FALSE;
		
		// setup the change location flags
		DWORD dwChangeLocationFlags = 0;
		if (TESTBIT( dwReadUnitFlags, RUF_RESTORED_ITEM_BIT ))
		{
			dwChangeLocationFlags |= CLF_ALWAYS_ALLOW_IN_EMAIL;
		}
				
		// add to unit
		bResult = UnitInventoryAdd(
					INV_CMD_PICKUP_NOREQ, 
					container, 
					unit, 
					tInvLoc.nInvLocation, 
					tInvLoc.nX, 
					tInvLoc.nY, 
					dwChangeLocationFlags, 
					tiEquipTime);
		
		// check for error
		if (bResult == FALSE)
		{
			// log error
			sLogInventoryLinkError( unit, container, tInvLoc, "PRIMARY" );
			
			// try fallback position
			if (bAllowFallback)
			{
				INVENTORY_LOCATION tInvLocFallback = sGetLoadFallbackInventoryLocation( container, unit, tInvLoc, IFT_FALLBACK );
				if (tInvLoc.nInvLocation != INVALID_LINK)
				{
					bResult = UnitInventoryAdd(
									INV_CMD_PICKUP_NOREQ, 
									container, 
									unit, 
									tInvLocFallback.nInvLocation, 
									tInvLocFallback.nX, 
									tInvLocFallback.nY, 
									dwChangeLocationFlags, 
									tiEquipTime);
				}

				// check for fallback error
				if (bResult == FALSE)
				{
					
					// log fallback error
					sLogInventoryLinkError( unit, container, tInvLocFallback, "SECONDARY" );
					
					// try buyback position (or other slot that CS can look at and possibly administer)
					INVENTORY_LOCATION tInvLocFallback = sGetLoadFallbackInventoryLocation( container, unit, tInvLoc, IFT_LOST_ITEM );
					if (tInvLoc.nInvLocation != INVALID_LINK)
					{
						
						// try again
						bResult = UnitInventoryAdd(
										INV_CMD_PICKUP_NOREQ, 
										container, 
										unit, 
										tInvLocFallback.nInvLocation, 
										tInvLocFallback.nX, 
										tInvLocFallback.nY, 
										dwChangeLocationFlags, 
										tiEquipTime);
						
						if (bResult == FALSE)
						{
							sLogInventoryLinkError( unit, container, tInvLocFallback, "LOST_ITEM" );
							
							// OK, last ditch effort ... we were unable to add to the lost item
							// inventory slot, that slot might be full, if so what we'll do is 
							// remove the oldest item in the lost inventory slot
							// and try the placement onemore time
							UNITID idUnitOldest = sFindOldestLostUnitIfFull( container );
							if (idUnitOldest != INVALID_ID)
							{
								UNIT *pUnitOldest = UnitGetById( pGame, idUnitOldest );
								if (pUnitOldest)
								{
									UnitFree( pUnitOldest, UFF_SEND_TO_CLIENTS, UFC_LOST );
									bResult = UnitInventoryAdd(
													INV_CMD_PICKUP_NOREQ, 
													container, 
													unit, 
													tInvLocFallback.nInvLocation, 
													tInvLocFallback.nX, 
													tInvLocFallback.nY, 
													dwChangeLocationFlags, 
													tiEquipTime);
									if (bResult == FALSE)
									{
										sLogInventoryLinkError( unit, container, tInvLocFallback, "LOST_ITEM_FINAL" );
									}
								}
							}						
						}
					}
				}
			}
		}

		return bResult;
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InventoryClearWaitingForMove(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pItem)
{
	if (!pItem)
		return;

	MSG_SCMD_CLEAR_INV_WAITING tMessage;
	tMessage.idItem = UnitGetId(pItem);
	
	s_SendMessage( pGame, ClientGetId( pClient ), SCMD_CLEAR_INV_WAITING, &tMessage );
}


//----------------------------------------------------------------------------
// nothing got moved, tell the disappointed client
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_InventoryCancelMove(
	UNIT * unit,
	UNIT * item)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
	GAMECLIENT * client = UnitGetClient(UnitGetUltimateContainer(unit));
	ASSERT_RETURN(client);
	s_InventoryClearWaitingForMove(game, client, item);
}
#endif


//----------------------------------------------------------------------------
// handles CCMD_INVPUT message
//----------------------------------------------------------------------------
void s_InventoryPut(
	UNIT *pUnit,
	UNIT *pItem,
	INVENTORY_LOCATION &tInvLocation,
	DWORD dwChangeLocFlags /*= 0*/,  // see CHANGE_LOCATION_FLAGS
	INV_CONTEXT eContext /*= INV_CONTEXT_NONE*/)
{
	ASSERT_RETURN(pUnit);
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(pItem);
	REF(tInvLocation);
	REF(dwChangeLocFlags);
#else
	// shouldn't ask to move items that do not exist
	if (!pItem)
	{
		UNITLOG_ASSERT(FALSE, pUnit);	
	}

	// shouldn't ask to move items that you don't have
	if (UnitGetUltimateContainer(pItem) != UnitGetUltimateOwner(pUnit))
	{
		UNITLOG_ASSERT(FALSE, pUnit);	
	}

	// if putting an item into an ingredient location, we will try to auto split stacks if we can
	BOOL bStackWasSplit = FALSE;
	if (InventoryIsIngredientLocation(pUnit, tInvLocation.nInvLocation))
	{
		bStackWasSplit = s_RecipeSplitIngredientToLocation(pUnit, pItem, tInvLocation);
	}
	
	// move item if stack was not split
	if (bStackWasSplit == FALSE)
	{
		if (InventoryChangeLocation(pUnit, pItem, tInvLocation.nInvLocation, tInvLocation.nX, tInvLocation.nY, dwChangeLocFlags, eContext))
		{
			return;
		}
	}

	s_InventoryCancelMove(pUnit, pItem);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_InventoryTryPut(
	UNIT *pContainer,
	UNIT *pItem,
	int nInvLocation,
	int nX,
	int nY)
{
	ASSERTX_RETURN( pContainer, "Expected container unit" );
	ASSERTX_RETURN( pItem, "Expected item unit" );
	GAME *pGame = UnitGetGame( pItem );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	
	// setup message
	MSG_CCMD_INVPUT tMessage;
	tMessage.idContainer = UnitGetId( pContainer );
	tMessage.idItem = UnitGetId( pItem );
	tMessage.bLocation = (BYTE)nInvLocation;
	tMessage.bX = (BYTE)nX;
	tMessage.bY = (BYTE)nY;
	UnitWaitingForInventoryMove(pItem, TRUE);
	
	// send to server
	c_SendMessage( CCMD_INVPUT, &tMessage );
	
}
#endif	//	!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_InventoryTrySwap(
	UNIT *pItemSrc,
	UNIT *pItemDest,
	int nFromWeaponConfig /*= -1*/,
	int nAltDestX /*= -1*/,
	int nAltDestY /*= -1*/)
{
	ASSERTX_RETURN( pItemSrc, "Expected unit" );
	ASSERTX_RETURN( pItemDest, "Expected unit" );
	GAME *pGame = UnitGetGame( pItemSrc );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	
	if (UnitIsWaitingForInventoryMove(pItemSrc) ||
		UnitIsWaitingForInventoryMove(pItemDest))
	{
		return;
	}

	MSG_CCMD_INVSWAP tSwapMsg;
	tSwapMsg.idSrc = UnitGetId(pItemSrc);
	tSwapMsg.idDest = UnitGetId(pItemDest);
	tSwapMsg.nFromWeaponConfig = nFromWeaponConfig;
	tSwapMsg.nAltDestX = nAltDestX;
	tSwapMsg.nAltDestY = nAltDestY;

	UnitWaitingForInventoryMove(pItemSrc, TRUE);
	UnitWaitingForInventoryMove(pItemDest, TRUE);

	// send to server
	c_SendMessage(CCMD_INVSWAP, &tSwapMsg);
	
}
#endif	//	!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *InventoryGetFirstAnalyzer(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );

	// only search on person	
	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	
	UNIT * pItem = NULL;	
	while ((pItem = UnitInventoryIterate( pUnit, pItem, dwInvIterateFlags )) != NULL)
	{
		if (UnitIsA( pItem, UNITTYPE_ANALYZER ))
		{
			return pItem;
		}
	}
	
	return NULL;

}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sSendStashToClient( 
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	
	// iterate inventory

	INVENTORY * inv = pPlayer->inventory;
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif

	// setup known flags
	DWORD dwUnitKnownFlags = 0;
	SETBIT( dwUnitKnownFlags, UKF_CACHED );
	SETBIT( dwUnitKnownFlags, UKF_ALLOW_CLOSED_STASH );
	
	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;
		if (TESTBIT(&loc->dwFlags, INVLOCFLAG_STASH_LOCATION))
		{
			UNIT *pItem = NULL;
			while ( (pItem = UnitInventoryLocationIterate( pPlayer, loc->nLocation, pItem, 0 )) != NULL)
			{
				// send to client and cache it on the client for a while so we don't
				// have to send all the data again if the exit->open->exit->open etc
				s_SendUnitToClient( pItem, pClient, dwUnitKnownFlags );
			}
		}		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sRemoveStashFromClient( 
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	
	// iterate inventory

	INVENTORY * inv = pPlayer->inventory;
#ifdef _DEBUG
	INVENTORY_CHECK_DEBUGMAGIC(inv);
#endif

	for (int li = 0; li < inv->nLocs; li++)
	{
		INVLOC * loc = inv->pLocs + li;
		if (TESTBIT(&loc->dwFlags, INVLOCFLAG_STASH_LOCATION))
		{
			UNIT *pItem = NULL;
			while ( (pItem = UnitInventoryLocationIterate( pPlayer, loc->nLocation, pItem, 0 )) != NULL)
			{
				// remove from client
				s_RemoveUnitFromClient( pItem, pClient, 0 );
			}
		}		
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * FindInUseGuildHerald(
	UNIT *pPlayer)
{
	ASSERT_RETNULL(pPlayer);

	DWORD dwFlags = 0;
	UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags );
	UNIT * next;

	while ( item != NULL )
	{
		next = UnitInventoryIterate( pPlayer, item, dwFlags );		
		if (UnitIsA(item, UNITTYPE_GUILD_HERALD) && UnitGetStat(item, STATS_GUILD_HERALD_IN_USE))
		{
			return item;
		}
		item = next;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_BackpackOpen(
	UNIT * pPlayer,
	UNIT * pBackpack)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pBackpack, "Expected Backpack unit" );

	// you cannot open a recipe if you're at a craftsman or trading (unless you're at a merchant, cause
	// we support that UI crazyness)
	UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
	if (pTradingWith && UnitIsMerchant( pTradingWith ) == FALSE)
	{
		return;
	}

	// setup message
	MSG_SCMD_BACKPACK_OPEN tMessage;			
	tMessage.idBackpack = UnitGetId( pBackpack );

	// send it
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_BACKPACK_OPEN, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_BackpackOpen( 
	UNIT * pBackpack )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( pBackpack, "Expected Backpack" );

	// close any previously displayed UI and reset back to starting state
	c_BackpackClose();

	UI_COMPONENT *pBackpackComp = UIComponentGetByEnum(UICOMP_BACKPACK_PANEL);
	if (pBackpackComp)
	{
		UI_COMPONENT *pInvGrid = UIComponentIterateChildren(pBackpackComp, NULL, UITYPE_INVGRID, TRUE);
		if (pInvGrid)
		{
			UIComponentSetFocusUnit(pInvGrid, pBackpack->unitid);
		}
	}

	// open UI if not open already
	TRADE_SPEC tTradeSpec;
	tTradeSpec.eStyle = STYLE_BACKPACK;		
	UIShowTradeScreen( UnitGetId( pBackpack ), FALSE, &tTradeSpec );		
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_BackpackClose()
{
#if !ISVERSION(SERVER_VERSION)
	if( AppIsHellgate() )
	{
		UIComponentActivate(UICOMP_BACKPACK_PANEL, FALSE);
	}
	else
	{
		UIHidePaneMenu( KPaneMenuCube );
	}
#endif
}
