//----------------------------------------------------------------------------
// FILE: unitfileversion.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __UNITFILEVERSION_H_
#define __UNITFILEVERSION_H_

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

// NOTE: Bumping up this # no longer automatically wipes characters, we don't want to wipe characters!
const unsigned int USV_CURRENT_VERSION = 197;
const unsigned int USV_REBALANCE_AND_VERSIONING_FIX = 196;				// 196 - Due to a timing issue with releases and changelists, some characters in development builds (ML and possibly also RC) are at version 196, but haven't been versioned with Alan's rebalance
const unsigned int USV_REBALANCE_AND_VERSIONING = 195;					// 195 - Alan's item rebalance.  Versioning spreadsheet implemented.
const unsigned int USV_SPLIT_SHARED_STASH = 194;						// 194 - items in shared stash have been placed into appropriate sections based on game variant
const unsigned int USV_RANK_WITH_LEVEL = 193;							// 193 - rank and level are stored together when Xfered
const unsigned int USV_WARDROBE_AFFIX_COUNT_CHANGED_FROM_SIX = 192;		// 192 - mythos added more affixes
const unsigned int USV_INVENTORY_NEEDS_GAME_VARIANTS = 191;		// 191 - we need to recurse player inventory and set STATS_GAME_VARIANT for each item
const unsigned int USV_NONTOWN_WAYPOINTS_BROKEN = 190;
const unsigned int USV_POTION_REBALANCING = 189;				 // 189 - All Potions got rebalanced, need to reset stats on the items
const unsigned int USV_BITFIELD_CRAFTING_STATS = 188;			 // 188 - Mythos crafting system initially introduced
const unsigned int USV_BITFIELD_LEVEL_VISITED_STATS = 187;			 // Mods starting in version 187 can have different values per item socketed with. We need to set a flag on the old items saying they already have the stats set
const unsigned int USV_MULTIPLE_STAT_VALUES_FOR_MODS_ALLOWED = 186;			 // Mods starting in version 187 can have different values per item socketed with. We need to set a flag on the old items saying they already have the stats set
const unsigned int USV_SKILL_STAT_FIX_FOR_MANA_AND_POWER_MOD = 185;			 // my fix for version 185 that was saving stats on the player when it should of not been :(
const unsigned int USV_GRANT_RESPEC_ITEM = 186;			 // we are giving old characters a respec item in Hellgate because of the many skill changes
const unsigned int USV_BROKEN_EXPERIENCE_MARKERS = 184;	 // my fix for version 183 had a problem in it :(
const unsigned int USV_POSSIBLY_BROKEN_EXPERIENCE_LEVEL_BENEFITS = 183;  // a misconfigured asia server has caused some players to not receive the proper number of stat and skill points when they level up
const unsigned int USV_BROKEN_FAWKES_DAY_QUEST_REWARD = 182;	// the final blueprint for the main fawkes day quest reward was screwed up and many people didn't get it. now fixing by giving it to people that completed the quest
const unsigned int USV_OLD_QUEST_ITEMS = 181;	// there were several quest items that could remain in your inventory when you completed the quest that should have been removed. this is the fix.
const unsigned int USV_NIGHTMARE_WAYPOINTS_BROKEN = 180;  // some characters could have their nightmare waypoints/last level screwed up...after this, should be fixed
const unsigned int USV_MESSED_UP_LEVEL_ATTRIBUTES = 179;  // asia turned off a level cap incorrectly on live servers, this is our attempt to fix those player save files
const unsigned int USV_GUID_NOT_NEAR_UNIT_ID = 178;  // after this version, unit GUIDs are near the unit id in the xfer stream
const unsigned int USV_MAX_LEVEL_BROKEN_FOR_BETA_FULL_GAME_ACCOUNTS = 177;
const unsigned int USV_ONE_DWORD_FOR_SAVE_FLAGS = 176;	// this version supported only one DWORD of UNITSAVEFLAGs
const unsigned int USV_STARTING_LOCATION_SINGLE_DIFFICULTY = 175;  // in the save header only a single level was saved for the starting level of a character, after this version we have changed it so that there is multiple entries, one for every difficulty

#endif
