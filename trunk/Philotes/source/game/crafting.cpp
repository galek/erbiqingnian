#include "stdafx.h"  // must be first for pre-compiled headers
#include "crafting.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "picker.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "recipe.h"
#include "gameunits.h"
#include "trade.h"
#include "treasure.h"
#include "gameglobals.h"
#include "Currency.h"
#include "s_quests.h"
#include "achievements.h"
#include "skills.h"
#include "recipe.h"

void CRAFTING::CraftingAddBlankModSlot( UNIT *pItem,										 
										int nLevel )
										
{
	ASSERT_RETURN( pItem );
	ASSERT_RETURN( nLevel > 0 );	
	int inventoryLocation = UnitGetInventoryType( pItem );
	UnitSetStat( pItem, STATS_ITEM_SLOTS_MAX, inventoryLocation, 1 );
	UnitSetStat(pItem, STATS_ITEM_SLOTS, inventoryLocation, 1  );
}


