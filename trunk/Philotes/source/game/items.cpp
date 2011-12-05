//----------------------------------------------------------------------------
// items.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// todo: 
// fix AffixPick to validate affix groups 
// AffixPick has a hardcoded max to the number of affixes it
//   can populate its' choose list with.  it would be good to change it so
//   that we allocate a choose list buffer in GAME as part of the post
//   process function for DATATABLE_AFFIXES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "characterclass.h"
#include "prime.h"
#include "globalindex.h"
#include "units.h" // also includes game.h
#include "level.h"
#include "unit_priv.h"
#include "items.h"
#include "excel_private.h"
#include "clients.h"
#include "s_message.h"
#include "c_message.h"
#include "inventory.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "c_appearance_priv.h"
#include "skills.h"
#include "c_camera.h"
#include "picker.h"
#include "console.h"
#include "player.h"
#include "states.h"
#include "unittag.h"
#include "c_sound.h"
#include "filepaths.h"
#include "appcommon.h"
#include "treasure.h"
#include "script.h"
#include "quests.h"
#include "affix.h"
#include "e_model.h"
#include "e_light.h"
#include "language.h"
#include "stringtable.h"
#include "performance.h"
#include "teams.h"
#include "s_townportal.h"
#include "movement.h"
#include "unitmodes.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_chat.h"
#include "dialog.h"
#include "room_path.h"
#include "svrstd.h"
#include "GameServer.h"
#include "wardrobe.h"
#include "uix_graphic.h"
#include "s_quests.h"
#include "c_quests.h"
#include "c_sound_util.h"
#include "objects.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "s_store.h"
#include "s_QuestServer.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "gameunits.h"
#include "s_recipe.h"
#include "monsters.h"
#include "c_input.h"
#include "hotkeys.h"
#include "GameMaps.h"
#include "chat.h"
#include "gameglobals.h"
#include "Economy.h"
#include "Currency.h"
#include "stringreplacement.h"
#include "achievements.h"
#include "c_crafting.h"
#include "s_crafting.h"
#include "pets.h"
#include "stash.h"
#include "offer.h"
#include "crafting_properties.h"
#include "consolecmd.h"
#include "playertrace.h"
#include "utilitygame.h"

#if ISVERSION(SERVER_VERSION)
	#include "items.cpp.tmh"
	#include "playertrace.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define ITEM_COLLISION_RADIUS		0.25f
#define ITEM_COLLISION_HEIGHT		1.0f
#define ITEM_COLLISION_RADIUS_MYTHOS		0.4f
#define ITEM_COLLISION_HEIGHT_MYTHOS		0.5f
//#define MAX_SELL_PRICE				1000000		// amt vendor will give you for item
//#define MAX_BUY_PRICE				50000000	// amt vendor will sell item for
#define ITEM_PULL_TO_VELOCITY		12.0f		// how fast items get sucked into you during pickup
#define LOCKED_ITEM_TIME_IN_TICKS	(GAME_TICKS_PER_SECOND * 60 * 5)

#define ICON_SUBDIRECTORY_STR			"icons\\"

float gflItemProcChanceOverride = -1.0f;
static const int NUM_PICKUP_ATTEMPTS_TO_CANCEL_ITEM_PULL_TO = 3;


int ItemGetBindedLevelArea( UNIT *item )
{
	ASSERT_RETINVALID(item);
	ASSERT_RETINVALID( UnitGetGenus( item ) == GENUS_ITEM );
	return UnitGetStat( item, STATS_ITEM_BIND_TO_LEVELAREA );
}

void ItemBindToLevelArea( UNIT *item, 
						  LEVEL *pLevel )
{
	ASSERT_RETURN(item);
	ASSERT_RETURN(pLevel);
	ASSERT_RETURN( UnitGetGenus( item ) == GENUS_ITEM );
	UnitSetStat( item, STATS_ITEM_BIND_TO_LEVELAREA, LevelGetLevelAreaID( pLevel ) );
	UnitSetFlag( item, UNITFLAG_NOSAVE, TRUE );	
	ItemSetNoTrade( item, TRUE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemInitStats(
	GAME* game,
	UNIT* unit,
	const UNIT_DATA* item_data)
{
	if (item_data->nInvWidth > 1)
	{
		UnitSetStat(unit, STATS_INVENTORY_WIDTH, item_data->nInvWidth);
	}
	if (item_data->nInvHeight > 1)
	{
		UnitSetStat(unit, STATS_INVENTORY_HEIGHT, item_data->nInvHeight);
	}
	if (item_data->nCooldown > 0)
	{
		UnitSetStat(unit, STATS_SKILL_COOLDOWN, item_data->nCooldown);
	}

	if ( AppIsTugboat() &&
		 UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_BIND_TO_LEVELAREA) )
	{
		ASSERT_RETFALSE( UnitGetLevel( unit ) );
		ItemBindToLevelArea( unit, UnitGetLevel( unit ) );
	}

	//Well, we don't have the player's difficulty...but this isn't a quest item, so make sure it isn't
	for(int nDifficulty= DIFFICULTY_NORMAL; nDifficulty < NUM_DIFFICULTIES;nDifficulty++)
	{
		UnitSetStat(unit, STATS_REMOVE_WHEN_DEACTIVATING_QUEST, nDifficulty, INVALID_LINK);
		UnitSetStat(unit, STATS_QUEST_STARTING_ITEM, nDifficulty, INVALID_LINK);
	}

	UnitSetFlag(unit, UNITFLAG_AUTOPICKUP, item_data->flAutoPickupDistance > 0.0f);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleFuseItemEvent( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	UnitFree( pUnit, UFF_SEND_TO_CLIENTS );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemUnFuse(
	UNIT * pUnit)
{
	UnitUnregisterTimedEvent(pUnit, sHandleFuseItemEvent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSetFuse(
	UNIT *unit,
	int nTicks)
{		
	nTicks = MAX(1, nTicks);
	UnitRegisterEventTimed( unit, sHandleFuseItemEvent, EVENT_DATA(), nTicks);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * ItemInit(
	GAME * game,
	ITEM_SPEC &tSpec)
{
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(tSpec.nTeam == INVALID_TEAM);
	//ASSERT_RETNULL(room || *pvPosition == INVALID_POSITION); // this is turned off because we are sending extra units to the client

	VECTOR vPosition = INVALID_POSITION;
	if (tSpec.pvPosition)
	{
		vPosition = *tSpec.pvPosition;
	}
	if ( IS_CLIENT( game ) )
	{
		if ( !tSpec.pRoom && vPosition != INVALID_POSITION )
			return NULL;
	} 
	else 
	{
		ASSERT_RETNULL(tSpec.pRoom || vPosition == INVALID_POSITION );
	}

	const UNIT_DATA * item_data = ItemGetData(tSpec.nItemClass);
	if (!item_data)
	{
		return NULL;
	}

	if ( ! UnitDataLoad( game, DATATABLE_ITEMS, tSpec.nItemClass, tSpec.eLoadType ) )
	{
		return NULL;
	}

	int nAppearanceDefId = ItemGetAppearanceDefId(game, tSpec.nItemClass, tSpec.nItemLookGroup);
	if (IS_CLIENT(game))
	{
		ASSERT_RETNULL(nAppearanceDefId != INVALID_ID);
	}

	UNIT_CREATE newunit( item_data );
	newunit.species = MAKE_SPECIES(GENUS_ITEM, tSpec.nItemClass);
	newunit.unitid = tSpec.idUnit;
	newunit.guidUnit = tSpec.guidUnit;
	newunit.pRoom = tSpec.pRoom;
	newunit.vPosition = vPosition;
	newunit.nAppearanceDefId = nAppearanceDefId;
	newunit.nAngle = tSpec.nAngle;
	newunit.vFaceDirection = tSpec.vDirection;
	newunit.vUpDirection = tSpec.vUp;
	ASSERT(item_data->nUnitType > UNITTYPE_ANY);
	newunit.fScale = tSpec.flScale != 1.0f ? tSpec.flScale : item_data->fScale;
	SET_MASKV(newunit.dwUnitCreateFlags, UCF_NO_DATABASE_OPERATIONS, TESTBIT(tSpec.dwFlags, ISF_NO_DATABASE_OPERATIONS_BIT));
	if (TESTBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT ))
	{
		ASSERTX( tSpec.guidRestrictedTo != INVALID_GUID, "Expected guid" );
		if (tSpec.guidRestrictedTo != INVALID_GUID)
		{
			SET_MASK(newunit.dwUnitCreateFlags, UCF_RESTRICTED_TO_GUID);
			newunit.guidRestrictedTo = tSpec.guidRestrictedTo;
		}
	}
#if ISVERSION(DEBUG_ID)
	PStrPrintf(newunit.szDbgName, MAX_ID_DEBUG_NAME, "%s", item_data->szName);
#endif
	if ( tSpec.idUnit == INVALID_ID && IS_CLIENT( game ) )
	{
		SET_MASK( newunit.dwUnitCreateFlags, UCF_CLIENT_ONLY );	
	}

	UNIT * unit = UnitInit(game, &newunit);
	if (!unit)
	{
		return NULL;
	}

	UnitInvNodeInit(unit);
	if( ItemInitStats(game, unit, item_data) == FALSE )
	{
		UnitFree( unit );
		return NULL;
	}
	if (tSpec.nItemLookGroup != INVALID_LINK)
	{
		ItemSetLookGroup( unit, tSpec.nItemLookGroup );
	}

	// assign no trade stat if present
	if (UnitDataTestFlag(item_data, UNIT_DATA_FLAG_NO_TRADE) == TRUE)
	{
		ItemSetNoTrade( unit, TRUE );
	}
	
	// targeting & collision
	UnitSetFlag(unit, UNITFLAG_CANBEPICKEDUP);
	UnitSetFlag(unit, UNITFLAG_ROOMCOLLISION);
	if( AppIsHellgate() )
	{
		UnitSetFlag(unit, UNITFLAG_BOUNCE_OFF_CEILING);
	}
	
	UnitSetDefaultTargetType(game, unit, item_data);

	UnitClearFlag(unit, UNITFLAG_JUSTCREATED);

	if (item_data->fFuse > 0.0f)
	{
		int nTicks = (int)(item_data->fFuse * GAME_TICKS_PER_SECOND_FLOAT);
		UnitSetFuse( unit, nTicks );
	}

	if (tSpec.nQuality != INVALID_LINK)
	{
		ItemSetQuality( unit, tSpec.nQuality );
	}
	
	UnitPostCreateDo(game, unit);


	return unit;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemInitInventoryGfx( 
	GAME* game,
	UNITID id,
	BOOL bCreateWardrobe)
{
	ASSERT( IS_CLIENT(game) );

	// weapons need this

	UNIT * pUnit = UnitGetById( game, id );
	UnitGfxInventoryInit( pUnit, bCreateWardrobe );
	ASSERT( pUnit && pUnit->pGfx );

	// set matrices and other inventory settings out of app def
	int nModelId = c_UnitGetModelIdInventory( pUnit );
	UnitModelGfxInit( game, pUnit, UnitGetDatatableEnum( pUnit ), UnitGetClass( pUnit ), nModelId );
	e_ModelSetLightGroup( nModelId, LIGHT_GROUP_ENVIRONMENT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemFreeInventoryGfx( 
	GAME* game,
	UNITID id )
{
	ASSERT( IS_CLIENT(game) );

	UNIT * pUnit = UnitGetById( game, id );
	UnitGfxInventoryFree( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetInitialItemLevel(
	ITEM_INIT_CONTEXT & context)
{
	context.level = UnitGetExperienceLevel(context.unit);
	ASSERT_RETFALSE(context.level > 0);

	// when items that are for a monster inventory to fight with, 
	// we forget all that level stuff specified by affixes and make the level of the item
	// the same as the level of the monster it is for
	if (TESTBIT(context.spec->dwFlags, ISF_MONSTER_INVENTORY_BIT) && 
		context.spawner && 
		UnitIsA(context.spawner, UNITTYPE_MONSTER) &&
		UnitIsNPC(context.spawner) == FALSE)
	{
		int level = UnitGetExperienceLevel(context.spawner);
		if (level > 0)
		{
			UnitSetExperienceLevel(context.unit, level);
			context.level = level;
		}
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetRequiredAffixes(
	ITEM_INIT_CONTEXT & context)
{

	// bail out if we can't do this
	if (TESTBIT( context.spec->dwFlags, ISF_SKIP_REQUIRED_AFFIXES_BIT ))
	{
		return TRUE;
	}
	
	for (int ii = 0; ii < MAX_REQ_AFFIX_GROUPS; ii++)
	{
		if (context.item_data->nReqAffixGroup[ii] == INVALID_LINK)
		{
			continue;
		}

		DWORD dwAffixPickFlags = 0;
		SETBIT(dwAffixPickFlags, APF_REQUIRED_AFFIX_BIT);
		int affix = s_AffixPick(context.unit, dwAffixPickFlags,	INVALID_LINK, context.spec, context.item_data->nReqAffixGroup[ii]);
		if (affix == INVALID_ID)
		{
			ASSERTV_CONTINUE(FALSE, "Item %s (level: %d) could not find an affix from a required group\n", context.item_data->szName, context.level);
		}
	}

	// affixes might have changed level
	context.level = UnitGetExperienceLevel(context.unit);
	ASSERT_RETFALSE(context.level > 0);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetLevelRequirement(
	ITEM_INIT_CONTEXT & context)
{
	if (!UnitDataTestFlag(context.item_data, UNIT_DATA_FLAG_COMPUTE_LEVEL_REQUIREMENT))
	{
		return TRUE;
	}

	if (UnitIsA(context.unit, UNITTYPE_MOD))
	{
		UnitSetStat(context.unit, STATS_ITEM_LEVEL_REQ, context.item_level->nItemLevelRequirement);
		UnitSetStat(context.unit, STATS_ITEM_LEVEL_LIMIT, context.item_level->nItemLevelLimit);
	}
	else
	{
		UnitSetStat(context.unit, STATS_LEVEL_REQ, context.item_level->nLevelRequirement);
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetStackSizeAndQuantity(
	ITEM_INIT_CONTEXT & context)
{
	if (!ExcelHasScript(context.game, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeStackSize), context.unitclass))
	{
		return TRUE;
	}
	int stacksize = ExcelEvalScript(context.game, context.unit, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeStackSize), context.unitclass);
	if (stacksize <= 0)
	{
		return TRUE;
	}
	UnitSetStat(context.unit, STATS_ITEM_QUANTITY_MAX, stacksize);
	if (context.spec && context.spec->nNumber > 0)
	{
		ItemSetQuantity(context.unit, context.spec->nNumber);
	}
	else
	{
		ItemSetQuantity(context.unit, 1);
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//returns the min and max base damage of an item, allowing for level offset and percent modification 
//----------------------------------------------------------------------------
BOOL ItemCaculateMinAndMaxBaseDamage( GAME *pGame,
									  const UNIT_DATA *pItemData,								    
									  int &nMinDmg,
									  int &nMaxDmg,
									  int nLevelOffset, /* = 0 */										
									  int nMinDmgPercentMod, /* = 0 */
									  int nMaxDmgPercentMod /* = 0 */ )
{
	ASSERT_RETFALSE( AppIsTugboat() );	//only tugboat
	ASSERT_RETFALSE( pItemData );					
	nLevelOffset += pItemData->nItemExperienceLevel;
	nLevelOffset = MAX( 0, nLevelOffset );	
	const ITEM_LEVEL *item_level = ItemLevelGetData(pGame, nLevelOffset);
	ASSERT_RETFALSE( item_level );	
	int nRow = ExcelGetLineByCode( EXCEL_CONTEXT( pGame ), DATATABLE_ITEMS, pItemData->wCode );
	nMinDmg = ExcelEvalScript(pGame, NULL, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMinBaseDmg), nRow) + nMinDmgPercentMod;
	nMaxDmg = ExcelEvalScript(pGame, NULL, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMaxBaseDmg), nRow) + nMaxDmgPercentMod;
	if( AppIsHellgate() )
	{		
		if (nMinDmg)
		{
			nMinDmg = PCT(nMinDmg, item_level->nDamageMult);
		}	
		if (nMaxDmg)
		{
			nMaxDmg = PCT(nMaxDmg, item_level->nDamageMult);			
		}
	}
	else // Mythos does this the reverse, where overall damage values are handled in a main curve.
	{		
		if (nMinDmg)
		{
			nMinDmg = PCT_ROUNDUP(item_level->nDamageMult, nMinDmg );		
		}
		if (nMaxDmg)
		{
			nMaxDmg = PCT_ROUNDUP(item_level->nDamageMult, nMaxDmg );		
		}
	}
	return TRUE;
}
#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//sets the min and max base damage of an item, allowing for level offset and percent modification 
//----------------------------------------------------------------------------
BOOL s_ItemSetMinAndMaxBaseDamage( UNIT *pItem,
								   int nLevelOffset, /* =0 */
								   int nMinDmgPercentMod, /* = 0 */
								   int nMaxDmgPercentMod /* = 0 */ )
{
	int min_base_dmg(0),
		max_base_dmg(0);
	if( ItemCaculateMinAndMaxBaseDamage( UnitGetGame( pItem ), pItem->pUnitData, min_base_dmg, max_base_dmg, nLevelOffset, nMinDmgPercentMod, nMaxDmgPercentMod ) )
	{
		if( min_base_dmg )
		{
			UnitSetStatShift(pItem, STATS_BASE_DMG_MIN, min_base_dmg);
		}
		if( max_base_dmg )
		{
			UnitSetStatShift(pItem, STATS_BASE_DMG_MAX, max_base_dmg);	
		}
		return TRUE;
	}

	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetBaseDamage(
	ITEM_INIT_CONTEXT & context)
{
	if (AppIsHellgate())
	{
		int min_base_dmg = ExcelEvalScript(context.game, context.unit, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMinBaseDmg), context.unitclass);
		if (min_base_dmg)
		{
			min_base_dmg = PCT(min_base_dmg, context.item_level->nDamageMult);

			UnitSetStatShift(context.unit, STATS_BASE_DMG_MIN, min_base_dmg);
		}

		int max_base_dmg = ExcelEvalScript(context.game, context.unit, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMaxBaseDmg), context.unitclass);
		if (max_base_dmg)
		{
			max_base_dmg = PCT(max_base_dmg, context.item_level->nDamageMult);
			UnitSetStatShift(context.unit, STATS_BASE_DMG_MAX, max_base_dmg);
		}
	}
	else
	{
		ASSERT_RETFALSE	( context.item_data );	
		return s_ItemSetMinAndMaxBaseDamage( context.unit, context.item_level->nLevel - context.item_data->nItemExperienceLevel );
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetSpecialEffects(
	ITEM_INIT_CONTEXT & context)
{	
	// init each of the special effect stats ... the only we even have these as columns
	// instead of the props that they used to be, is because the props are too hard for
	// people to edit and at a glance see what items have what props
	for (unsigned int ii = 0; ii < NUM_DAMAGE_TYPES; ++ii)
	{
		WORD wParam = (WORD)ii;  // param for these stats is the damage type
		const SPECIAL_EFFECT * special_effect = &context.item_data->tSpecialEffect[ii];

		// ability to cause special this type of special effect
		if (AppIsHellgate())
		{
			int nSfxAttack = PCT(context.item_level->nBaseSfxAttackAbility, special_effect->nAbilityPercent);		
			UnitSetStat(context.unit, STATS_SFX_ATTACK, wParam, nSfxAttack);
		}
		if (AppIsTugboat())
		{
			int nSfxAttack = PCT(special_effect->nAbilityPercent, context.item_level->nBaseSfxAttackAbility );		
			UnitSetStat( context.unit, STATS_SFX_ATTACK, wParam, nSfxAttack);
		}

		// ability to defend against this type of special effect
		if (AppIsHellgate())
		{
			int nSfxDefense = PCT(context.item_level->nBaseSfxDefenseAbility, special_effect->nDefensePercent);
			UnitSetStat(context.unit, STATS_SFX_DEFENSE_BONUS, wParam, nSfxDefense);
		}
		if (AppIsTugboat())
		{
			int nSfxDefense = PCT(special_effect->nDefensePercent, context.item_level->nBaseSfxDefenseAbility);
			UnitSetStat(context.unit, STATS_SFX_DEFENSE_BONUS, wParam, nSfxDefense);
		}		
		// strength of special effect
		UnitSetStat(context.unit, STATS_SFX_STRENGTH_PCT, wParam, special_effect->nStrengthPercent);

		// duration of special effect (will be added to base duration, see damagetypes.xls)
		UnitSetStat(context.unit, STATS_SFX_DURATION_IN_TICKS, wParam, special_effect->nDurationInTicks);
	}

	// this non-elemental value is used by Cabalist Focus items
	int nSfxAttackFocus = PCT(context.item_level->nBaseSfxAttackAbility, context.item_data->nSfxAttackPercentFocus);		
	if ( nSfxAttackFocus )
		UnitSetStat(context.unit, STATS_SFX_ATTACK_FOCUS, nSfxAttackFocus);

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetAttackStats(
	ITEM_INIT_CONTEXT & context)
{
	ASSERT_RETFALSE(s_sItemInitPropertiesSetBaseDamage(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetSpecialEffects(context));

	int nInterruptAttack = StatsRoundStatPct(context.game, STATS_INTERRUPT_ATTACK, context.item_level->nBaseInterruptAttack, context.item_data->nInterruptAttackPct);
	UnitSetStatShift(context.unit, STATS_INTERRUPT_ATTACK,	nInterruptAttack);
	int nInterruptDefense = StatsRoundStatPct(context.game, STATS_INTERRUPT_DEFENSE, context.item_level->nBaseInterruptDefense, context.item_data->nInterruptDefensePct);
	UnitSetStatShift(context.unit, STATS_INTERRUPT_DEFENSE,	nInterruptDefense);

	int nStealthAttack = StatsRoundStatPct(context.game, STATS_STEALTH_ATTACK, context.item_level->nBaseStealthAttack, context.item_data->nStealthAttackPct);
	UnitSetStatShift(context.unit, STATS_STEALTH_ATTACK, nStealthAttack);

	if (context.item_data->nFiringErrorIncrease)
	{
		UnitSetStat(context.unit, STATS_FIRING_ERROR_INCREASE_SOURCE_WEAPON, context.item_data->nFiringErrorIncrease);
	}
	if (context.item_data->nFiringErrorDecrease)
	{
		UnitSetStat(context.unit, STATS_FIRING_ERROR_DECREASE_SOURCE_WEAPON, context.item_data->nFiringErrorDecrease);
	}
	if (context.item_data->nFiringErrorMax)
	{
		UnitSetStat(context.unit, STATS_FIRING_ERROR_MAX_WEAPON, context.item_data->nFiringErrorMax);
	}

	return TRUE;
}
#endif

BOOL ItemGetMinAC( const UNIT_DATA *pItemData,
				   int nDamageType,
				   int nLevelOffset, /* = 0 */
				   int nMinACPercentMod /*= 0*/ )
{
	ASSERT_RETFALSE( pItemData );
	int nAC = int(pItemData->fArmor[nDamageType]) + nMinACPercentMod;
	nLevelOffset += pItemData->nItemExperienceLevel;
	nLevelOffset = MAX( 0, nLevelOffset );	
	const ITEM_LEVEL *item_level = ItemLevelGetData(NULL, nLevelOffset);
	ASSERT_RETFALSE( item_level );
	nAC = StatsRoundStatPct( NULL, STATS_AC, item_level->nBaseArmor, nAC);
	return nAC;
}

BOOL ItemGetMaxAC( const UNIT_DATA *pItemData,
				   int nDamageType,
				   int nLevelOffset, /* = 0 */
				   int nMaxACPercentMod /*= 0*/ )
{
	ASSERT_RETFALSE( pItemData );
	int nAC = pItemData->nArmorMax[nDamageType] + nMaxACPercentMod;
	nLevelOffset += pItemData->nItemExperienceLevel;
	nLevelOffset = MAX( 0, nLevelOffset );	
	const ITEM_LEVEL *item_level = ItemLevelGetData(NULL, nLevelOffset);
	ASSERT_RETFALSE( item_level );
	nAC = StatsRoundStatPct( NULL, STATS_AC, item_level->nBaseArmor, nAC);
	return nAC;
}

BOOL ItemCaculateAC( GAME *pGame,
						 const UNIT_DATA *pItemData,								    
						 int &nAC,						 
						 int nLevelOffset,/* = 0 */
						 int nMinACPercentMod,/* = 0 */
						 int nMaxACPercentMod, /*= 0*/
						 BOOL bForceMax /* =FALSE */ )
{
	int nMaxAC = ItemGetMaxAC( pItemData, DAMAGE_TYPE_ALL, nLevelOffset, nMaxACPercentMod );
	if( bForceMax )
	{
		nAC = nMaxAC;
		return TRUE;
	}
	int nMinAC = ItemGetMinAC( pItemData, DAMAGE_TYPE_ALL, nLevelOffset, nMinACPercentMod );	
	nAC = RandGetNum(pGame, nMinAC, nMaxAC );
	return TRUE;
}

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemSetMinAndMaxAC( UNIT *pItem,
						   int nLevelOffset, /*= 0 */
						   int nMinACPercentMod, /*= 0*/
						   int nMaxACPercentMod, /*= 0*/
						   BOOL bForceMax /* =FALSE */ )
{
	ASSERT_RETFALSE( AppIsTugboat() ); //tugboat only
	ASSERT_RETFALSE( pItem ); 
	int nAC( 0 );
	if( ItemCaculateAC( UnitGetGame( pItem ), pItem->pUnitData, nAC, nLevelOffset, nMinACPercentMod, nMaxACPercentMod, bForceMax ) )
	{
		UnitSetStat(pItem, STATS_AC, DAMAGE_TYPE_ALL, nAC);
		return TRUE;
	}
	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemInitPropertiesSetDefenseStats(
	ITEM_INIT_CONTEXT & context)
{
	if( AppIsTugboat() )
	{
		return s_ItemSetMinAndMaxAC( context.unit, context.item_level->nLevel - context.item_data->nItemExperienceLevel, 0, 0, context.level > context.item_data->nItemExperienceLevel );
	}
	if (AppIsHellgate())
	{
		for (unsigned int ii = 0; ii < NUM_DAMAGE_TYPES; ++ii)
		{
			if (context.item_data->fArmor[ii] > 0.0f)
			{
				int armor = StatsRoundStatPct(context.game, STATS_ARMOR, context.item_level->nBaseArmor, context.item_data->fArmor[ii]);
				UnitSetStat(context.unit, STATS_ARMOR, ii, armor);
				if (context.item_level->nBaseArmorBuffer > 0)
				{
					int buffer = StatsRoundStatPct(context.game, STATS_ARMOR_BUFFER_MAX, context.item_level->nBaseArmorBuffer, context.item_data->fArmor[ii]);
					UnitSetStat(context.unit, STATS_ARMOR_BUFFER_MAX, ii, buffer);
				}
				if (context.item_level->nBaseArmorRegen > 0)
				{
					int regen = StatsRoundStatPct(context.game, STATS_ARMOR_BUFFER_REGEN, context.item_level->nBaseArmorRegen, context.item_data->fArmor[ii]);
					UnitSetStat(context.unit, STATS_ARMOR_BUFFER_REGEN, ii, regen);

				}
			}
			if (context.item_data->fShields[ii] > 0.0f)
			{
				int shields = StatsRoundStatPct(context.game, STATS_SHIELDS, context.item_level->nBaseShields, context.item_data->fShields[ii]);
				UnitSetStat(context.unit, STATS_SHIELDS, ii, shields);
				//if (item_level->nBaseShieldBuffer > 0)
				{
					int buffer = StatsRoundStatPct(context.game, STATS_SHIELD_BUFFER_MAX, context.item_level->nBaseShields, context.item_data->fShields[ii]);
					//int buffer = StatsRoundStatPct(context.game, STATS_SHIELD_BUFFER_MAX, context.item_level->nBaseShieldBuffer, context.item_data->nShields[ii]);
					UnitSetStat(context.unit, STATS_SHIELD_BUFFER_MAX, ii, buffer);
				}
				if (context.item_level->nBaseShieldRegen < 0)
				{
					int regen = StatsRoundStatPct(context.game, STATS_SHIELD_BUFFER_REGEN, context.item_level->nBaseShieldRegen, context.item_data->fShields[ii]);
					UnitSetStat(context.unit, STATS_SHIELD_BUFFER_REGEN, ii, regen);
				}
			}
		}
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemInitPropertiesSetPerLevelStats(
	ITEM_INIT_CONTEXT & context)
{
	// set level based code properties
	for (unsigned int ii = 0; ii < NUM_UNIT_PERLEVEL_PROPERTIES; ++ii)
	{
		ExcelEvalScript(context.game, context.unit, context.spawner, NULL, DATATABLE_ITEMS, (DWORD)OFFSET(UNIT_DATA, codePerLevelProperties[ii]), context.unitclass);
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemInitPropertiesSetLevelBasedStats(
	ITEM_INIT_CONTEXT & context)
{
	// get per-level data
	context.item_level = ItemLevelGetData(context.game, context.level);
	ASSERT_RETFALSE(context.item_level);

	ASSERT_RETFALSE(s_sItemInitPropertiesSetLevelRequirement(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetAttackStats(context));
	
	ASSERT_RETFALSE(s_ItemInitPropertiesSetDefenseStats(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetBaseDamage(context));

	ASSERT_RETFALSE(s_ItemInitPropertiesSetPerLevelStats(context));

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
enum ITEM_INIT_RESULT
{
	IIR_ERROR,		// an error
	IIR_FAILED,		// failed to init (not an error)
	IIR_SUCCESS,	// success
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemPropIsValidForUnitType( int nPropID,		
								   const UNIT_DATA *pUnitData,
								   int nUnitType )
{
	ASSERT_RETFALSE( nPropID >= 0 && nPropID < NUM_UNIT_PROPERTIES );
	return UnitIsA( nUnitType, pUnitData->nCodePropertiesApplyToUnitType[nPropID]);
}

//sets the actual properties on the item via the scripts found in the props1, props2...props5. Returns FALSE if no properties are set
//NOTE: Tugboat's Mod items can have different properties depending on what they are getting inserted
//into. If no item is passed in for pItemInsertingInto and the pItem requires a unittype to for
//given properties, those properties won't be set. However when dropping the item into a socketable item,
//this function needs to be called again, and pass in pItemInsertingInto. Check out s_network.cpp func: sCCmdItemInvPut
BOOL s_ItemPropsInit( GAME *pGame,
					  UNIT *pItem,
					  UNIT *pSpawner, //not sure what this is for but the old code was using it and I don't want to break anything
					  UNIT *pItemInsertingInto /*= NULL */ )
{
	ASSERT_RETFALSE( pItem );
	BOOL bSetAStat( FALSE );
	if( AppIsTugboat() ) //this all should work for hellgate too, but for now lets not even try it
	{
		if( pItemInsertingInto == NULL )
			pItemInsertingInto = pItem;
		BOOL bIsModItem = UnitIsA( pItem, UNITTYPE_MOD );  //for now we are only going to save this for MOD's. This is very dangerous. ( what if a unique item has this called multiple times! )
		if( bIsModItem && //only if it's a mod item
			UnitGetStat( pItem, STATS_ITEM_PROPS_SET ) > 0 )
			return FALSE;

		
		for (unsigned int ii = 0; ii < NUM_UNIT_PROPERTIES; ++ii)
		{
			if( pItem->pUnitData->nCodePropertiesApplyToUnitType[ii] == INVALID_ID ||	//the item's don't need to check the unit type to set it's properties - so just set it ( will work for hellgate )
				s_ItemPropIsValidForUnitType( ii, pItem->pUnitData, pItemInsertingInto->pUnitData->nUnitType ) )	//we need to check the item we are inserting into to set properties.
			{		
				if( pItem->pUnitData->codeProperties[ ii ] != NULL_CODE )
				{
					bSetAStat = TRUE;			
					ExcelEvalScript( pGame, pItem, pSpawner, NULL, DATATABLE_ITEMS, (DWORD)OFFSET(UNIT_DATA, codeProperties[ii]), UnitGetClass( pItem ) );
				}

			}
		}
		
		if( bIsModItem &&  //only mod items this stat for now.
			bSetAStat ) //we need to save the fact we set this stat
		{
			UnitSetStat( pItem, STATS_ITEM_PROPS_SET, 1 );
		}
	}
	else
	{
		//for hellgate to get mods to have different properties via the type of object equiped to it requires:
		//1) Version change - check out function sApplyPlayerFixForModStats in player.cpp
		//2) Change in fuction s_network.cpp func: sCCmdItemInvPut - this will actually set the mods stats when the mod gets equiped.
		//3) Change UI flow. Check out stats.cpp func:PrintStatsForUnitTypes
		for (unsigned int ii = 0; ii < NUM_UNIT_PROPERTIES; ++ii)
		{
			if( pItem->pUnitData->codeProperties[ ii ] != NULL_CODE )
			{
				bSetAStat = TRUE;			
				ExcelEvalScript( pGame, pItem, pSpawner, NULL, DATATABLE_ITEMS, (DWORD)OFFSET(UNIT_DATA, codeProperties[ii]), UnitGetClass( pItem ) );
			}
		}		
	}
	return bSetAStat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static ITEM_INIT_RESULT s_sItemInitPropertiesSetClassProperties(
	ITEM_INIT_CONTEXT & context)
{
	ITEM_INIT_RESULT eResult = IIR_SUCCESS;

	if(context.spec && TESTBIT(context.spec->dwFlags, ISF_SKIP_PROPS))
	{
		return eResult;
	}

	//init scripted properties on the item
	s_ItemPropsInit( context.game, context.unit, context.spawner, NULL );


	// pick a recipe for those items that need them 
	if (UnitIsA(context.unit, UNITTYPE_SINGLE_USE_RECIPE))
	{
		if (s_RecipeSingleUseInit(context.unit) == FALSE)
		{
			eResult = IIR_FAILED;  // not an error
		}
	}

	return eResult;
}
#endif


//----------------------------------------------------------------------------
// this needs to go after evaluating the default properties for the unit type
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetMoney(
	ITEM_INIT_CONTEXT & context)
{
	if (!UnitIsA(context.unit, UNITTYPE_MONEY))	
	{
		return TRUE;
	}

	ASSERT_RETFALSE(context.spec->nMoneyAmount > 0 || context.spec->nRealWorldMoneyAmount > 0);
	cCurrency currencyAdd( context.spec->nMoneyAmount, context.spec->nRealWorldMoneyAmount );
	UnitAddCurrency( context.unit, currencyAdd );

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// comes after class properties
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetSkillOverride(
	ITEM_INIT_CONTEXT & context)
{
	if (!context.spec || context.spec->nSkill == INVALID_LINK)
	{
		return TRUE;
	}

	STATS_ENTRY skill_entries[1];
	int nSkills = UnitGetStatsRange(context.unit, STATS_SKILL_LEVEL, 0, skill_entries, 1);
	if (nSkills <= 0)
	{
		if (!AppCommonGetSilentAssert())
		{
			ConsoleString(CONSOLE_ERROR_COLOR, L"ITEM command tried to specify a skill for a non-skill item");
		}
		return TRUE;
	}

	int nSkill = StatGetParam(STATS_SKILL_LEVEL, skill_entries[0].GetParam(), 0);
	if (nSkill != INVALID_ID && nSkill != context.spec->nSkill)
	{
		// Re-set the stats on the skill item to be the skill that was specified 
		// This should only be used for cheats right now, but we may need a better system to do this
		UnitSetStat(context.unit, STATS_SKILL_LEVEL, nSkill, 0);

		SkillUnitSetInitialStats(context.unit, context.spec->nSkill, context.level);

		// UnitSetStat(context.unit, STATS_SKILL_LEVEL, context.spec->nSkill, skill_entries[0].value);
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetMaxSlotsCheat(
	ITEM_INIT_CONTEXT & context)
{
	if (!context.spec || !TESTBIT(context.spec->dwFlags, ISF_MAX_SLOTS_BIT))
	{
		return TRUE;
	}
	// can't use UNIT_ITERATE_STATS_RANGE because it's a default stat!
	if (AppIsHellgate())
	{
		static const int gDebugSlots[] =
		{
			INVLOC_AMMO,
			INVLOC_ROCKETS,
			INVLOC_FUEL,
			INVLOC_BATTERY,
			INVLOC_RELIC,
			INVLOC_TECH,
			INVLOC_ALTFIRE
		};
		for (unsigned int ii = 0; ii < arrsize(gDebugSlots); ii++)
		{
			int max_slots = UnitGetStat(context.unit,  STATS_ITEM_SLOTS_MAX, gDebugSlots[ii]);
			UnitSetStat(context.unit, STATS_ITEM_SLOTS, gDebugSlots[ii], max_slots);
		}
	}
	else if (AppIsTugboat())
	{
		static const int gDebugSlots[] =
		{
			INVLOC_GEMSOCKET
		};
		for (unsigned int ii = 0; ii < arrsize(gDebugSlots); ii++)
		{
			int max_slots = UnitGetStat(context.unit,  STATS_ITEM_SLOTS_MAX, gDebugSlots[ii]);
			UnitSetStat(context.unit, STATS_ITEM_SLOTS, gDebugSlots[ii], max_slots);
		}
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetCheatOverrides(
	ITEM_INIT_CONTEXT & context)
{
	ASSERT_RETFALSE(s_sItemInitPropertiesSetSkillOverride(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetMaxSlotsCheat(context));

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static DWORD s_sItemInitPropertiesGetAffixPickFlags(
	ITEM_INIT_CONTEXT & context)
{
	DWORD dwAffixPickFlags = 0;
	ASSERT(context.quality_data->flProcChance >= 0.0f && context.quality_data->flProcChance <= 100.0f);
	float procChanceMod = context.quality_data->flProcChance;
	// NOTE: this is a bad way to do this, talk to phu for why
	//procChanceMod += (int)(context.spec->nLuckModifier * context.quality_data->flLuckMod); 
	if (procChanceMod <= 0.0)
	{					
		return dwAffixPickFlags;
	}
	if (gflItemProcChanceOverride != -1)
	{
		procChanceMod = gflItemProcChanceOverride + (int)(context.spec->nLuckModifier * 100.0f);
	}
	if (RandGetFloat(context.game, 100.0f) <= procChanceMod)
	{
		SETBIT(dwAffixPickFlags, APF_ALLOW_PROCS_BIT);
	}
	// when affixes are specified, always allow procs
	if (TESTBIT(context.spec->dwFlags, ISF_AFFIX_SPEC_ONLY_BIT))
	{
		SETBIT(dwAffixPickFlags, APF_ALLOW_PROCS_BIT);
	}

	return dwAffixPickFlags;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesCheckRandomAffix(
	ITEM_INIT_CONTEXT & context,
	unsigned int index,
	BOOL & bUsedLuck)
{
	if (context.spec->nAffixes[0] != INVALID_LINK)
	{
		return FALSE;		// apply pre-specified affix
	}
	int chance = ExcelEvalScript(context.game, context.unit, context.spawner, NULL, DATATABLE_ITEM_QUALITY, 
		(DWORD)OFFSET(ITEM_QUALITY_DATA, tAffixPicks[index].codeAffixChance), context.quality);
	if ( chance <= 0 )
	{
		return FALSE;
	}
	int roll = (int)RandGetNum(context.game, 100);
	if (roll >= chance)
	{

		return FALSE;	
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
enum AFFIX_PICK_RESULT
{
	AFFIX_NONE =		-2,
	AFFIX_ERROR =		INVALID_LINK,
};
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sItemInitPropertiesPickRandomAffixType(
	ITEM_INIT_CONTEXT & context,
	unsigned int index,
	BOOL & bUsedLuck)
{
	ASSERT_RETINVALID(index < MAX_AFFIX_GEN);

	if (!s_sItemInitPropertiesCheckRandomAffix(context, index, bUsedLuck))
	{
		return AFFIX_NONE;
	}

	PickerStart(context.game, picker);
	for (unsigned int ii = 0; ii < MAX_AFFIX_GEN_TYPE; ++ii)
	{
		if (context.quality_data->tAffixPicks[index].linkAffixType[ii] == INVALID_LINK)
		{
			continue;
		}
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(context.game, DATATABLE_ITEM_QUALITY, context.quality_data->tAffixPicks[index].codeAffixTypeWeight[ii], &codelen);
		if (!code)
		{
			continue;
		}
		int weight = VMExecI(context.game, context.unit, context.spawner, NULL, context.quality, context.level, 0, 0, 0, code, codelen);
		if (weight <= 0)
		{
			continue;
		}

		PickerAdd(MEMORY_FUNC_FILELINE(context.game, (int)ii, weight));
	}

	if (PickerGetCount(context.game) <= 0)
	{
		return AFFIX_ERROR;
	}

	int affixtypeidx = PickerChoose(context.game);
	ASSERT_RETINVALID(affixtypeidx >= 0 && affixtypeidx < MAX_AFFIX_GEN_TYPE);

	return context.quality_data->tAffixPicks[index].linkAffixType[affixtypeidx];
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sItemInitPropertiesPickRandomAffix(
	ITEM_INIT_CONTEXT & context,
	unsigned int index,
	DWORD dwAffixPickFlags)
{
	static const unsigned int MAX_AFFIX_DOWNGRADE = 6;

	ASSERT_RETINVALID(index < MAX_AFFIX_GEN);

	BOOL bUsedLuck = FALSE;
	int affixtype = s_sItemInitPropertiesPickRandomAffixType(context, index, bUsedLuck);
	if (affixtype < 0)
	{
		return affixtype;
	}

	// pick affix
	int affix = INVALID_LINK;
	for (unsigned int ii = 0; ii < MAX_AFFIX_DOWNGRADE; ++ii)
	{
		affix = s_AffixPick(context.unit, dwAffixPickFlags, affixtype, context.spec, INVALID_LINK);
		if (affix != INVALID_LINK)
		{
			break;
		}

		const AFFIX_TYPE_DATA * affixtype_data = AffixTypeGetData(context.game, affixtype);
		ASSERT_BREAK(affixtype_data);

		if (affixtype_data->bRequired)
		{
			break;
		}
		if (affixtype_data->nDowngradeType == INVALID_LINK)
		{
			affix = AFFIX_NONE;		// affix isn't required, so return none instead of error
			break;
		}
		affixtype = affixtype_data->nDowngradeType;
	}

	// refund luck if no affix found
	if (bUsedLuck && affix < 0)
	{
		context.spec->iLuckModifiedItem--;
	}

	return affix;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesPickRandomAffixes(
	ITEM_INIT_CONTEXT & context,
	DWORD dwAffixPickFlags)
{
	if (UnitDataTestFlag(context.item_data, UNIT_DATA_FLAG_NO_RANDOM_AFFIXES) ||
		(context.spec && TESTBIT( context.spec->dwFlags, ISF_SKIP_QUALITY_AFFIXES_BIT ) ))
	{
		return TRUE;
	}

	// pick "magical" affixes
	UNIT *item = context.unit;
	context.affix_count = AffixGetAffixCount(item, AF_ALL_FAMILIES);
	for (unsigned int ii = 0; ii < MAX_AFFIX_GEN && context.affix_count < MAX_AFFIX_GEN; ii++)
	{
		int affixtype = s_sItemInitPropertiesPickRandomAffix(context, ii, dwAffixPickFlags);
		if (affixtype == AFFIX_NONE)
		{
			continue;
		}
		if (affixtype == AFFIX_ERROR)
		{
			ASSERTX_RETFALSE( 0, "Unable to pick random affix" );
		}

		context.affix_count++;
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesRemoveAffixes(
	ITEM_INIT_CONTEXT & context)
{
	// can't figure out how to do this right now
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesDowngradeQuality(
	ITEM_INIT_CONTEXT & context)
{
	ASSERT_RETFALSE(context.quality_data);
	ASSERT_RETFALSE(context.item_data);

	ASSERT_RETFALSE(s_sItemInitPropertiesRemoveAffixes(context));

	const ITEM_QUALITY_DATA * quality_data = context.quality_data;

	for (unsigned int ii = 0; ii < MAX_QUALITY_DOWNGRADE_ATTEMPTS; ++ii)
	{
		ASSERT_RETFALSE(context.quality != quality_data->nDowngrade);
		int quality = quality_data->nDowngrade;
		if (quality == INVALID_LINK)
		{
			return TRUE;
		}
		quality_data = ItemQualityGetData(context.game, quality);
		ASSERT_RETFALSE(quality_data);

		if (TESTBIT(context.item_data->pdwQualities, quality))
		{
			context.quality = quality;
			context.quality_data = quality_data;
			ItemSetQuality(context.unit, quality);
			return TRUE;
		}
	}
	return FALSE;
}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemPickFixedAffixes(
	ITEM_INIT_CONTEXT &context)
{

	// pick hand crafted affixes
	if (TESTBIT( context.spec->dwFlags, ISF_SKIP_HAND_CRAFTED_AFFIXES_BIT ) == FALSE)
	{
		for (unsigned int ii = 0; ii < (unsigned int)AffixGetMaxAffixCount(); ++ii)
		{
			int nAffix = context.item_data->nAffixes[ii];
			if (nAffix != INVALID_LINK)
			{
				int nAffixPicked = s_AffixPickSpecific(context.unit, nAffix, context.spec);
				if (nAffixPicked == INVALID_LINK)
				{
					const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
					REF( pAffixData );
					const int MAX_STRING = 512;
					char szAffixList[ MAX_STRING ];
					AffixGetStringListOnUnit( context.unit, szAffixList, MAX_STRING );
					ASSERTV_RETFALSE(
						0,
						"Unable to init hand crafted affix '%s' on item '%s'. Item Affixes: '%s'",
						pAffixData->szName,
						UnitGetDevName(context.unit),
						szAffixList);					
				}
			}
		}
	}

	// apply any specific affixes that are requested in the spec
	for (unsigned int ii = 0; ii < (unsigned int)AffixGetMaxAffixCount(); ++ii)
	{
		int nAffix = context.spec->nAffixes[ii];
		if (nAffix != INVALID_LINK)
		{
			int nAffixPicked = s_AffixPickSpecific(context.unit, nAffix, context.spec);
			if (nAffixPicked == INVALID_LINK)
			{
				const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
				REF( pAffixData );
				const int MAX_STRING = 512;
				char szAffixList[ MAX_STRING ];
				AffixGetStringListOnUnit( context.unit, szAffixList, MAX_STRING );
				ASSERTV_RETFALSE(
					0,
					"Unable to init spec supplied affix '%s' on item '%s'. Item Affixes: '%s'",
					pAffixData->szName,
					UnitGetDevName(context.unit),
					szAffixList);									
			}
		}
	}
	
	return TRUE;
	
}		
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetQualityAndAffixes(
	ITEM_INIT_CONTEXT & context )
{
	BOOL bAppliedInitialAffixes = FALSE;
	for (unsigned int ii = 0; ii < MAX_QUALITY_DOWNGRADE_ATTEMPTS; ++ii)
	{
		context.quality_data = (const ITEM_QUALITY_DATA *)ExcelGetData(context.game, DATATABLE_ITEM_QUALITY, context.quality);
		ASSERTV_RETFALSE(
			context.quality_data, 
			"Invalid quality '%d' for item '%s'", 
			context.quality,
			UnitGetDevName(context.unit));

		// what are the affix flags we can use
		DWORD dwAffixPickFlags = s_sItemInitPropertiesGetAffixPickFlags(context);

		// do initial affixes
		if (bAppliedInitialAffixes == FALSE)
		{
			if (sItemPickFixedAffixes( context ) == FALSE)
			{
				ASSERTV_RETFALSE( 0, "Unable to apply fixed affixes to item '%s'", UnitGetDevName( context.unit ) );
			}

			// we have applied the initial affixes now			
			bAppliedInitialAffixes = TRUE;
			
		}
		
		BOOL bForceDownGrade( FALSE );
		if( context.item_level->nLevel < context.quality_data->nMinLevel )  //some qualities require you to be at a given level before they show up.
		{
			bForceDownGrade = TRUE;
		}

		if ( bForceDownGrade ||
			!s_sItemInitPropertiesPickRandomAffixes(context, dwAffixPickFlags))
		{
			// downgrade if we didn't get anything and we can do so
			if (context.affix_count == 0 &&
				context.quality_data->nDowngrade >= 0 &&
				ItemCanDowngradeQuality(UnitGetClass(context.unit)))
			{
				ASSERT_RETFALSE(s_sItemInitPropertiesDowngradeQuality(context));
				continue;
			}
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemPickRandomQuality(
	ITEM_INIT_CONTEXT & context)
{
	context.quality = s_TreasureSimplePickQuality(context.game, context.level, context.item_data);
	if (context.quality == INVALID_LINK)
	{
		TraceGameInfo("Couldn't find a valid quality level for item %s.", context.item_data->szName);
		return FALSE;
	}
	ItemSetQuality(context.unit, context.quality);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetProperName(
	ITEM_INIT_CONTEXT & context)
{
	ASSERT_RETFALSE(context.quality_data);

	if (context.quality_data->bRandomlyNamed)
	{
		int nMonsterName = MonsterProperNamePick(context.game);
		if (nMonsterName != INVALID_LINK)
		{
			ItemProperNameSet(context.unit, nMonsterName, FALSE);
		}
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// set identified properties
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitPropertiesSetIdentified(
	ITEM_INIT_CONTEXT & context)
{
	// init the identified stat, if there are any attached affixes that means
	// there is something for the player to identify
	BOOL bIdentified = AffixGetAffixCount(context.unit, AF_ATTACHED) == 0;

	if (ItemIsSpawningForGambler( context.spec ))
	{
		bIdentified = FALSE;
	}

	UnitSetStat(context.unit, STATS_IDENTIFIED, bIdentified);

	if (bIdentified)
	{
		return TRUE;
	}

	ASSERT_RETFALSE(context.spec);
	// identify unidentified items in some situations
	if (TESTBIT(context.spec->dwFlags, ISF_IDENTIFY_BIT) ||
		TESTBIT(context.spec->dwFlags, ISF_STARTING_BIT) ||
		TESTBIT(context.spec->dwFlags, ISF_PICKUP_BIT) ||
		UnitDataTestFlag( context.unit->pUnitData, UNIT_DATA_FLAG_IDENTIFY_ON_CREATE ) )
	{
		s_ItemIdentify(context.unit, NULL, context.unit);
	}
	else
	{
		if (context.spec->pMerchant && UnitIsGambler(context.spec->pMerchant))
		{
			cCurrency currency = EconomyItemBuyPrice( context.unit, context.spec->pMerchant, TRUE );
			int nPrice = currency.GetValue( KCURRENCY_VALUE_INGAME );

			float fGamblePriceRangeMin = 1.0f;
			float fGamblePriceRangeMax = 1.0f;

			const UNIT_DATA *pMerchantUnitData = UnitGetData( context.spec->pMerchant );	
			if (pMerchantUnitData && pMerchantUnitData->nStartingTreasure != INVALID_LINK)
			{
				const TREASURE_DATA *pTreasureData = TreasureGetData(context.game, pMerchantUnitData->nStartingTreasure);
				fGamblePriceRangeMin = pTreasureData->fGamblePriceRangeMin;
				fGamblePriceRangeMax = pTreasureData->fGamblePriceRangeMax;
			}

			nPrice = (int)((float)nPrice * RandGetFloat(context.game, fGamblePriceRangeMin, fGamblePriceRangeMax));		// TODO: this should be data-driven!!!!
			UnitSetStat(context.unit, STATS_BUY_PRICE_OVERRIDE, nPrice);
		}
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemInitProperties(
	ITEM_INIT_CONTEXT & context)
{
	ASSERT_RETFALSE(s_sItemInitPropertiesSetInitialItemLevel(context));

	// pre-set quality (this can get overriden later
	context.quality = context.spec->nQuality;
	ItemSetQuality(context.unit, context.quality);

	// if this item requires some affixes be picked first, do it here (they will probably affect the level stat)
	ASSERT_RETFALSE(s_sItemInitPropertiesSetRequiredAffixes(context));

	ASSERT_RETFALSE(s_ItemInitPropertiesSetLevelBasedStats(context));

	ITEM_INIT_RESULT eResult = s_sItemInitPropertiesSetClassProperties(context);
	if (eResult == IIR_ERROR)
	{
		ASSERTV_RETFALSE( 0, "Failed to init item class properties for '%s'", UnitGetDevName( context.unit ) );
	}

	ASSERT_RETFALSE(s_sItemInitPropertiesSetStackSizeAndQuantity(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetMoney(context));

	// needs to come after class properties have been set
	ASSERT_RETFALSE(s_sItemInitPropertiesSetCheatOverrides(context));

	if (context.quality < 0)
	{
		ASSERT_RETFALSE(s_sItemPickRandomQuality(context));
	}

	ASSERT_RETFALSE(s_sItemInitPropertiesSetQualityAndAffixes(context));

	ASSERT_RETFALSE(s_sItemInitPropertiesSetProperName(context));

	// if item level changed, recalculate damage/armor (Mythos only)
	// for weapon/armor grades
	if (UnitGetExperienceLevel(context.unit) != context.level)
	{
		context.level = UnitGetExperienceLevel(context.unit);
		ASSERT_RETFALSE(s_ItemInitPropertiesSetLevelBasedStats(context));
	}

	ASSERT_RETFALSE(s_sItemInitPropertiesSetIdentified(context));

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemInitProperties(
	GAME * game,
	UNIT * unit,
	UNIT * spawner,
	int nClass,
	const UNIT_DATA * item_data,
	ITEM_SPEC * spec)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(nClass != INVALID_LINK);
	ASSERT_RETFALSE(item_data);
	ASSERT_RETFALSE(spec);

	ITEM_INIT_CONTEXT context(game, unit, spawner, nClass, item_data, spec);
	return (s_sItemInitProperties(context));
}
#endif


//----------------------------------------------------------------------------

static void sItemUnShine(
	GAME * game,
	UNIT * unit )
{
	int quality = ItemGetQuality( unit );
	if (quality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA * quality_data = ItemQualityGetData( quality );
		ASSERT_RETURN(quality_data);
		s_StateClear( unit, UnitGetId( unit ), quality_data->nState, 0 );
	}
}
//----------------------------------------------------------------------------
void ItemShine(
	GAME * game,
	UNIT * unit,
	VECTOR & vPosition )
{
	sItemUnShine( game, unit );
	if( IS_SERVER( game ) )	// server doesn't play a 'drop' anim
	{
		UnitDrop( game, unit, vPosition );
	}
	if (UnitIsA( unit, UNITTYPE_MONEY ))
	{
		int nGoldState = GlobalIndexGet( GI_STATE_ITEM_GOLD );
		s_StateSet( unit, NULL, nGoldState, 0 );
	}
	else
	{
		int quality = ItemGetQuality( unit );
		if (quality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA * quality_data = ItemQualityGetData( quality );
			ASSERT_RETURN(quality_data);
			if (quality_data->nState != INVALID_LINK)
			{
				s_StateSet( unit, unit, quality_data->nState, 0 );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemQualityIsBetterThan(
	int nItemQuality1,
	int nItemQuality2)
{

	if (nItemQuality1 != nItemQuality2)
	{
	
		// checking against invalid quality
		if (nItemQuality2 == INVALID_LINK)
		{
			if (nItemQuality1 != INVALID_LINK)
			{
				return TRUE;
			}		
		}
		else if (nItemQuality1 != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA *pQualityData1 = ItemQualityGetData( nItemQuality1 );
			ASSERT_RETFALSE(pQualityData1);
			
			// item quality 1 is only better than quality 2 if item quality 2 appears somewhere in the
			// downgrade chain of quality 2
			if (pQualityData1->nDowngrade != INVALID_LINK)
			{
				if (pQualityData1->nDowngrade == nItemQuality2)
				{
					return TRUE;
				}				
				return ItemQualityIsBetterThan( pQualityData1->nDowngrade, nItemQuality2 );
			}
			
		}

	}
	
	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemShine(
	GAME * game,
	UNIT * unit )
{
	if (UnitIsA( unit, UNITTYPE_MONEY ))
	{
		int nGoldState = GlobalIndexGet( GI_STATE_ITEM_GOLD );
		s_StateSet( unit, NULL, nGoldState, 0 );
	}
	else
	{
		int quality = ItemGetQuality( unit );
		if (quality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA * quality_data = ( const ITEM_QUALITY_DATA * )ExcelGetData( game, DATATABLE_ITEM_QUALITY, quality );
			if (quality_data && quality_data->nState != INVALID_LINK)
			{
				s_StateSet( unit, unit, quality_data->nState, 0 );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemAddSpawnResult(
	UNIT *pUnit, 
	ITEMSPAWN_RESULT *spawnResult)
{
	ASSERTX_RETURN(spawnResult, "sItemAddSpawnResult() - Missing required param" );
	ASSERTX_RETURN( spawnResult->nTreasureBufferCount < spawnResult->nTreasureBufferSize, "Spawn result buffer size is too small" );
	
	// add to buffer of spawned units and increment count
	spawnResult->pTreasureBuffer[ spawnResult->nTreasureBufferCount++ ] = pUnit;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ITEM_DROPPING_Z_VELOCITY 5.0f
void ItemFlippy( 
	UNIT * pUnit,
	BOOL bSendToClient )
{
	GAME *pGame = UnitGetGame( pUnit );

	UnitStartJump( pGame, pUnit, ITEM_DROPPING_Z_VELOCITY, bSendToClient, TRUE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static ROOM * s_sItemSpawnInitSpawnPosition(
	GAME * game,
	const UNIT * spawner,
	const ITEM_SPEC * spec,
	VECTOR & position,
	VECTOR & facing)
{
	ROOM * room = NULL;
	position = INVALID_POSITION;

	if (!TESTBIT(spec->dwFlags, ISF_PLACE_IN_WORLD_BIT)) 
	{
		facing = cgvNull;
		return NULL;
	}

	if (AppIsHellgate())
	{
		if (spec->pvPosition)
		{
			position = *spec->pvPosition;
		}
		else
		{
			position = spawner->vPosition;		
		}
		if(!TESTBIT(spec->dwFlags, ISF_NO_FLIPPY_BIT))
		{
			const float MAX_FLIPPY_HEIGHT = 2.5f;
			const float MAX_FLIPPY_START_HEIGHT = 2.0f;
			const float FLIPPY_RAYCAST_LENGTH = MAX_FLIPPY_HEIGHT + MAX_FLIPPY_START_HEIGHT;
			float fFlippyHeight = LevelLineCollideLen(game, UnitGetLevel(spawner), position, VECTOR(0, 0, 1), FLIPPY_RAYCAST_LENGTH);
			fFlippyHeight -= MAX_FLIPPY_HEIGHT;
			fFlippyHeight = MAX(fFlippyHeight, 0.0f);
			fFlippyHeight = MIN(fFlippyHeight, MAX_FLIPPY_START_HEIGHT);
			position.fZ += fFlippyHeight;
		}
	}
	if (AppIsTugboat())
	{
		// TRAVIS: Let's just always spawn from the spawner - we flip to good locations
		position = spawner->vPosition;		
		position.fZ += 0.2f;
	}
	// temporary invalid position = 0, 0, 0
	if (position == INVALID_POSITION)
	{
		position.fX = 0.001f;
	}

	if (spawner)
	{
		room = UnitGetRoom(spawner);
		facing = spawner->vFaceDirection;
	}
	else
	{
		room = spec->pRoom;
		facing = RandGetVectorXY(game);
	}
	ASSERTX(room, "No room to spawn item in provided");

	return room;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sItemSpawnInitLevel(
	GAME * game,
	UNIT * spawner,
	UNIT * item,
	int nClass,
	const UNIT_DATA * item_data, 
	const ITEM_SPEC * spec)
{
	int level = 1;
	// phu note - both apps should merge how they set levels
	if (AppIsHellgate())
	{
		if (spec->nLevel > 0)
		{
			level = spec->nLevel;
		}
		else if (spawner)
		{
			level = UnitGetExperienceLevel(spawner);
		}
		level = PIN(level, 1, (int)ExcelGetNumRows(EXCEL_CONTEXT(game), DATATABLE_ITEM_LEVELS));
	
		UnitSetStat(item, STATS_ITEM_SPAWNER_LEVEL, level);

		if (TESTBIT( spec->dwFlags, ISF_USE_SPEC_LEVEL_ONLY_BIT ) == FALSE)
		{				
			int codelen;
			BYTE * code = ExcelGetScriptCode(game, DATATABLE_ITEMS, item_data->codeFixedLevel, &codelen);
			if (code)
			{
				//should the level of the item be passed in here as the skill level? If so the variable sklvl will be used as the item level as well...hmm
				level = VMExecI(game, item, level, nClass, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);
			}
		}
		
	}
	if( AppIsTugboat() )
	{
		if (TESTBIT( spec->dwFlags, ISF_USE_SPEC_LEVEL_ONLY_BIT ) && spec->nLevel > 0)
		{
			level = spec->nLevel;
		}
		else
		{
			if( UnitIsA( item, UNITTYPE_MAP_KNOWN ) && spec->nLevel > 0 )
			{
				level = spec->nLevel;
			}
			else if (UnitDataTestFlag(item_data, UNIT_DATA_FLAG_NO_LEVEL))
			{
				level = item_data->nItemExperienceLevel;
			}
			else
			{
				UnitSetStat(item, STATS_ITEM_SPAWNER_LEVEL, level);
			}
		}
	}
#if ISVERSION(CHEATS_ENABLED)
	if ( gnItemLevelOverride != -1 )
		level = gnItemLevelOverride;
	else if ( game->m_nDebugItemFixedLevel )
		level = game->m_nDebugItemFixedLevel;
#endif

	level = PIN(level, 1, (int)ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_ITEM_LEVELS));
	UnitSetExperienceLevel(item, level);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemCanDowngradeQuality( 
	int nItemClass)
{

	// check for no class
	if (nItemClass == INVALID_LINK)
	{
		return FALSE;
	}
	
	// check unit data
	const UNIT_DATA *pItemData = UnitGetData( NULL, GENUS_ITEM, nItemClass );
	if (UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_NO_QUALITY_DOWNGRADE))
	{
		return FALSE;
	}
	
	return TRUE;  // ok to downgrade
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sModifyClassByQuality(	
	GAME *pGame,
	int nItemClass,
	ITEM_SPEC &tSpec)
{
	if (tSpec.nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( tSpec.nQuality );
		if (pItemQualityData && pItemQualityData->bChangeClass)
		{
			
			// init picker
			PickerStart( pGame, tPicker );

			// find all the items that use the chosen item as a base row
			int nNumChoices = 0;
			int nNumItems = ExcelGetNumRows( NULL, DATATABLE_ITEMS );
			for (int nItemClassOther = 0; nItemClassOther < nNumItems; ++nItemClassOther)
			{
								
				// must match required quality of the item class chosen
				const UNIT_DATA *pItemDataOther = ItemGetData( nItemClassOther );
				if ( ! pItemDataOther )
					continue;

				if (pItemDataOther->nItemQualityRequired == tSpec.nQuality)
				{
									
					// see if this row is a child of the item class passed in
					if (UnitIsInHierarchyOf( GENUS_ITEM, nItemClassOther, &nItemClass, 1 ))
					{
						PickerAdd(MEMORY_FUNC_FILELINE(pGame, nItemClassOther, 1));
						nNumChoices++;
					}

				}
														
			}
			
			// pick a choice if we have one
			if (nNumChoices)
			{
				// pick one of the item classes to change to
				nItemClass = PickerChoose( pGame );
			}
			else
			{
				
				// no option found to change class, downgrade quality if allowed
				if (ItemCanDowngradeQuality( nItemClass ))
				{
					tSpec.nQuality = pItemQualityData->nDowngrade;
				}
				
			}
			
		}
		
	}
	
	return nItemClass;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * s_ItemSpawn(
	GAME * game,
	ITEM_SPEC &tSpec,
	ITEMSPAWN_RESULT * spawnResult)
{
	UNIT *unit = NULL;
	
#if !ISVERSION(CLIENT_ONLY_VERSION)

	TIMER_STARTEX("s_ItemSpawn()", 2);

	ASSERT_RETNULL(IS_SERVER(game));

	const UNIT_DATA * item_data = ItemGetData(tSpec.nItemClass);
	ASSERT_RETNULL(item_data);

	if (UnitIsA(item_data->nUnitType, UNITTYPE_MONEY) && tSpec.nMoneyAmount <= 0)	// probably should assert, but for gstar
	{
		return NULL;
	}

	// adjust required item qualitites
	if (item_data->nItemQualityRequired != INVALID_LINK)
	{
		tSpec.nQuality = item_data->nItemQualityRequired;
	}

	// modify the class by the quality
	tSpec.nItemClass = sModifyClassByQuality( game, tSpec.nItemClass, tSpec );

	VECTOR position;	
	VECTOR facing;
	ROOM * room = s_sItemSpawnInitSpawnPosition(game, tSpec.pSpawner, &tSpec, position, facing);

	tSpec.pRoom = room;
	tSpec.pvPosition = &position;
	tSpec.vDirection = facing;
	unit = ItemInit(game, tSpec);
	if (!unit)
	{
		return NULL;
	}
	item_data = UnitGetData(unit);
	if (!item_data)
	{
		UnitFree(unit);
		ASSERT_RETNULL(0);
	}
	if (!UnitSetGuid(unit, GameServerGenerateGUID()))
	{
		UnitFree(unit);
		ASSERT_RETNULL(0);
	}

	UnitTriggerEvent( game, EVENT_CREATED, unit, NULL, NULL );

	s_sItemSpawnInitLevel(game, tSpec.pSpawner, unit, tSpec.nItemClass, item_data, &tSpec);

	if( TESTBIT( tSpec.dwFlags, ISF_IS_CRAFTED_ITEM ) )
	{		
		if( CRAFTING::CraftingItemCanHaveModLevel( unit ) )
		{
			UnitSetStat( unit, STATS_ITEM_CRAFTING_LEVEL, 1 );
		}
	}


	if (!s_ItemInitProperties(game, unit, tSpec.pSpawner, tSpec.nItemClass, item_data, &tSpec))
	{
		UnitFree(unit);
		return NULL;
	}

	if( TESTBIT( tSpec.dwFlags, ISF_ZERO_OUT_SOCKETS ) )	//zero out slots
	{		
		UnitSetStat( unit, STATS_ITEM_SLOTS, INVLOC_GEMSOCKET, 0 );
	}

	if (tSpec.nOverrideInventorySizeX > 0)
	{
		UnitSetStat(unit, STATS_INVENTORY_WIDTH, tSpec.nOverrideInventorySizeX);
	}
	if (tSpec.nOverrideInventorySizeY > 0)
	{
		UnitSetStat(unit, STATS_INVENTORY_HEIGHT, tSpec.nOverrideInventorySizeY);
	}

	UnitSetStat(unit, STATS_ITEM_DIFFICULTY_SPAWNED, game->nDifficulty);

	// generate mods
	if (TESTBIT(tSpec.dwFlags, ISF_GENERATE_ALL_MODS_BIT))
	{
		if (ItemGetModSlots(unit))
		{
			s_ItemModGenerateAll(unit, unit, INVALID_LINK, NULL, NULL);
		}
	}

	// callback
	if (tSpec.pfnSpawnCallback)
	{
		if (tSpec.pfnSpawnCallback(game, tSpec.pSpawner, unit, tSpec.nItemClass, &tSpec, spawnResult, tSpec.pSpawnCallbackData) == ITEM_SPAWN_DESTROY)
		{
			UnitFree(unit);
			return NULL;
		}
	}
	
	// add to spawn result
	if (spawnResult)
	{
		sItemAddSpawnResult(unit, spawnResult);
	}

	if (TESTBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT))
	{
		if (!TESTBIT(tSpec.dwFlags, ISF_NO_FLIPPY_BIT))
		{
			if (AppIsHellgate())
			{
				ItemShine( game, unit );
			}

			if (AppIsHellgate())
			{
				ItemFlippy(unit, TRUE);
			}
		}

		// if item is in world, it is now ready to be sent over the network and should be sent
		if (UnitIsInWorld(unit))
		{
			// unit is now ready for messages
			UnitSetCanSendMessages(unit, TRUE);
			
			// send to clients who care			
			s_SendUnitToAllWithRoom(game, unit, UnitGetRoom(unit));
			
		}

		if (!TESTBIT(tSpec.dwFlags, ISF_NO_FLIPPY_BIT))
		{
			if (AppIsTugboat())
			{
				ItemShine(game, unit, position);
			}
		}
	}

#endif

	return unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// create an item client-side for display purposes. We won't have any details on attributes, but we can give base item info.

UNIT * c_ItemSpawnForDisplay(
	GAME * game,
	ITEM_SPEC &tSpec )
{
	UNIT * unit = NULL;

#if !ISVERSION(SERVER_VERSION)

	ASSERT_RETNULL( IS_CLIENT(game) );

	const UNIT_DATA * item_data = ItemGetData( tSpec.nItemClass );
	ASSERT_RETNULL( item_data );

	// can't do money
	if ( UnitIsA( item_data->nUnitType, UNITTYPE_MONEY ) )
	{
		return NULL;
	}

	// can't do qualities... (i think)
	if (item_data->nItemQualityRequired != INVALID_LINK)
	{
		return NULL;
	}

	// check for anything we can't do...
	if ( TESTBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT ) )
	{
		return NULL;
	}

	// make a unit
	unit = ItemInit(game, tSpec);
	if (!unit)
	{
		return NULL;
	}
	item_data = UnitGetData(unit);
	if (!item_data)
	{
		UnitFree(unit);
		ASSERT_RETNULL(0);
	}

	c_ItemInitInventoryGfx( game, UnitGetId( unit ), FALSE );

	UnitSetStat( unit, STATS_IDENTIFIED, TRUE );
#endif

	return unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
UNIT * s_ItemSpawnForCheats(
	GAME * game,
	ITEM_SPEC specCopy,
	GAMECLIENT * client,
	DWORD dwFlags,
	BOOL * pbElementEffects )
{
	ASSERT_RETNULL(game && IS_SERVER(game));

	if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP )
	{
		dwFlags |= ITEM_SPAWN_CHEAT_FLAG_PICKUP;
		SETBIT(specCopy.dwFlags, ISF_IDENTIFY_BIT);
	}

	// put in world
	if ( (dwFlags & ITEM_SPAWN_CHEAT_FLAG_PICKUP) == 0 )
	{
		SETBIT(specCopy.dwFlags, ISF_PLACE_IN_WORLD_BIT);
	}

	if ( (dwFlags & ITEM_SPAWN_CHEAT_FLAG_DONT_SEND) != 0 )
	{
		CLEARBIT(specCopy.dwFlags, ISF_PLACE_IN_WORLD_BIT);
	}

	// all mods flag
	if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_ADD_MODS )
	{
		SETBIT(specCopy.dwFlags, ISF_GENERATE_ALL_MODS_BIT);
	}
	
	const UNIT_DATA * itemData = ItemGetData(specCopy.nItemClass);
	ASSERT_RETNULL(itemData);

	// spawn the item
	UNIT *unit = specCopy.pSpawner;
	ASSERT( unit );
	UNIT * item = s_ItemSpawn(game, specCopy, NULL);
	if (!item)
	{
		if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT )
		{
			WCHAR strItemName[MAX_ITEM_NAME_STRING_LENGTH];
			DWORD dwFlags = 0;
			SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
			UnitGetName(unit, strItemName, arrsize(strItemName), dwFlags);
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unable to spawn item [%s] for some reason.", strItemName);	
		}
		return NULL;
	}

	for (unsigned int ii = 0; ii < MAX_DAMAGE_TYPES; ii++)
	{
		if (pbElementEffects[ii])
		{
			UnitSetStat(item, STATS_MISSILE_MUZZLE_TYPE, ii, 1);
			UnitSetStat(item, STATS_MISSILE_TRAIL_TYPE, ii, 1);
			UnitSetStat(item, STATS_MISSILE_PROJECTILE_TYPE, ii, 1);
			UnitSetStat(item, STATS_MISSILE_IMPACT_TYPE, ii, 1);
		}
	}

	if (dwFlags & ITEM_SPAWN_CHEAT_FLAG_PICKUP)
	{
		if ( UnitGetRoom( specCopy.pSpawner ) )
		{
			// this item is now ready to do network communication
			UnitSetCanSendMessages( item, TRUE );

			// put in world (yeah, kinda lame, but it keeps the pickup code path clean)
			UnitWarp( 
				item,
				UnitGetRoom( specCopy.pSpawner ), 
				UnitGetPosition( specCopy.pSpawner ),
				cgvYAxis,
				cgvZAxis,
				0);
		}

		// pick up item
		s_ItemPickup(unit, item);

		if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP )
		{
			if (ItemIsInEquipLocation(unit, item))
			{
				return item;
			}

			// delete anything blocking me from picking up the item
			int location, x, y;
			BOOL bLocOpen = ItemGetEquipLocation(unit, item, &location, &x, &y);
			if (location != INVLOC_NONE)
			{
				UNIT * itemCurrentlyEquipped = UnitInventoryGetByLocationAndXY(unit, location, x, y);
				if (itemCurrentlyEquipped != item)
				{
					while (!bLocOpen)	// a space was found, but it's occupied : note this can happen if we don't meet the requirements too!
					{
						UNIT * olditem = UnitInventoryGetByLocationAndXY(unit, location, x, y);

						// if we're blocked, but there's not item in the slot, check the preventing slot
						int nPreventingLoc = INVLOC_NONE;
						if (!olditem && IsUnitPreventedFromInvLoc(unit, item, location, &nPreventingLoc))
						{
							olditem = UnitInventoryGetByLocation(unit, nPreventingLoc);
						}
						if (!olditem)
						{
							break;
						}
						UnitFree(olditem, UFF_SEND_TO_CLIENTS);
						bLocOpen = ItemGetEquipLocation(unit, item, &location, &x, &y);
					}

					BOOL bItemAdded = InventoryChangeLocation(unit, item, location, x, y, CLF_ALLOW_NEW_CONTAINER_BIT);
					if (!bItemAdded)
					{
						if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT )
						{
							WCHAR strItemName[MAX_ITEM_NAME_STRING_LENGTH];
							DWORD dwFlags = 0;
							SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
							UnitGetName(item, strItemName, arrsize(strItemName), dwFlags);
							ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unable to auto-equip spawned item [%s].", strItemName);	
						}
						return NULL;
					}
				}
			}
			else if ( dwFlags & ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT )
			{
				WCHAR strItemName[MAX_ITEM_NAME_STRING_LENGTH];
				DWORD dwFlags = 0;
				SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
				UnitGetName(unit, strItemName, arrsize(strItemName), dwFlags);
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unable to auto-equip spawned item [%s].", strItemName);	
			}
		}
	}
	return item;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL s_ItemGenerateRandomArmor(
	UNIT * pUnit,
	BOOL bDebugOutput )
{
	ASSERT_RETFALSE( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(pGame && IS_SERVER(pGame));

	int nNumItems = ExcelGetNumRows( pGame, DATATABLE_ITEMS );

	int nArmorStart = 0;
	for ( ; nArmorStart < nNumItems; nArmorStart++ )
	{
		const UNIT_DATA * pUnitData = ItemGetData( nArmorStart );
		if (! pUnitData )
			continue;

		if ( (UnitIsA( pUnitData->nUnitType, UNITTYPE_ARMOR ) || 
			UnitIsA( pUnitData->nUnitType, UNITTYPE_CLOTHING )) &&
			!UnitIsA( pUnitData->nUnitType, UNITTYPE_SHIELD ))
			break;
	}

	if ( nArmorStart >= nNumItems )
		return FALSE;


	int nInventoryLocations = ExcelGetNumRows( NULL, DATATABLE_INVLOC );

	int nItemLevel = MAX( UnitGetExperienceLevel( pUnit ), 1 );

	for ( int nInvLoc = 0; nInvLoc < nInventoryLocations; nInvLoc++ )
	{
		INVLOC_DATA* invloc_data = (INVLOC_DATA*)ExcelGetData(NULL, DATATABLE_INVLOC, nInvLoc);
		if ( ! invloc_data )
			continue;

		if ( invloc_data->nInventoryType != UnitGetInventoryType( pUnit ) )
			continue;
		if ( ! TESTBIT( invloc_data->dwFlags, INVLOCFLAG_USE_IN_RANDOM_ARMOR ) )
			continue;

		PickerStart( pGame, picker );

		for ( int nItemClass = nArmorStart; nItemClass < nNumItems; nItemClass ++ )
		{
			const UNIT_DATA * pItemData = ItemGetData( nItemClass );
			if ( ! pItemData )
				continue;
			if (IsAllowedUnit( pItemData->nUnitType, invloc_data->nAllowTypes, INVLOC_UNITTYPES ) &&
				IsAllowedUnit( pUnit, pItemData->nContainerUnitTypes, NUM_CONTAINER_UNITTYPES ) &&
				UnitIsA( pUnit, pItemData->nPreferedByUnitType ) &&
				UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_SPAWN ) &&
				pItemData->nItemExperienceLevel <= nItemLevel &&
				MAX( pItemData->nMaxLevel, pItemData->nItemExperienceLevel + 5 ) > nItemLevel)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, nItemClass, UnitGetType( pUnit ) == pItemData->nPreferedByUnitType ? 2 : 1));
			}
		}

		int nClassToSpawn = PickerChoose( pGame );
		if ( nClassToSpawn == INVALID_ID )
			continue;

		ITEM_SPEC spec;
		spec.nLevel = nItemLevel;
		SETBIT( spec.dwFlags, ISF_IDENTIFY_BIT );
		SETBIT( spec.dwFlags, ISF_USEABLE_BY_SPAWNER_BIT );
		spec.nItemClass = nClassToSpawn;
		spec.pSpawner = pUnit;
		BOOL bElementEffects[MAX_DAMAGE_TYPES];
		memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

		DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_PICKUP;
		if ( bDebugOutput )
			dwFlags |= ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT;
		s_ItemSpawnForCheats( pGame, spec, UnitGetClient( pUnit ), dwFlags, bElementEffects );
	}


	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL s_ItemGenerateRandomWeapons(
	UNIT * pUnit,
	BOOL bDebugOutput,
	int nNumItemsToSpawn)
{
	ASSERT_RETFALSE( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE(pGame && IS_SERVER(pGame));

	int nNumItems = ExcelGetNumRows( pGame, DATATABLE_ITEMS );

	int nWeaponStart = 0;
	for ( ; nWeaponStart < nNumItems; nWeaponStart++ )
	{
		const UNIT_DATA * pUnitData = ItemGetData( nWeaponStart );
		if (! pUnitData )
			continue;
		if (UnitIsA( pUnitData->nUnitType, UNITTYPE_WEAPON ))
			break;
	}

	if ( nWeaponStart >= nNumItems )
		return FALSE;

	int nItemLevel = MAX( UnitGetExperienceLevel( pUnit ), 1 );

	int nItemLevelRange = 5;
	int nNumItemsSpawned = 0;
	for ( int j = 0; j < nItemLevel; j += nItemLevelRange)
	{
		PickerStart( pGame, picker );

		for ( int nItemClass = nWeaponStart; nItemClass < nNumItems; nItemClass ++ )
		{
			const UNIT_DATA * pItemData = ItemGetData( nItemClass );
			if (! pItemData )
				continue;

			if (UnitIsA( pUnit, pItemData->nPreferedByUnitType ) &&
				UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_SPAWN ) &&
				(UnitIsA( pItemData->nUnitType, UNITTYPE_WEAPON ) || UnitIsA( pItemData->nUnitType, UNITTYPE_SHIELD)) &&
				pItemData->nItemExperienceLevel <= nItemLevel &&
				pItemData->nItemExperienceLevel > nItemLevel - (j + nItemLevelRange) )
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, nItemClass, UnitGetType( pUnit ) == pItemData->nPreferedByUnitType ? 10 : 1));
			}
		}

		if(PickerGetCount(pGame) <= 0)
		{
			continue;
		}

		for ( int i = 0; i < nNumItemsToSpawn && nNumItemsSpawned < nNumItemsToSpawn; i++ )
		{
			int nClassToSpawn = PickerChooseRemove( pGame );
			if ( nClassToSpawn == INVALID_ID )
				break;

			ITEM_SPEC spec;
			spec.nLevel = nItemLevel;
			SETBIT( spec.dwFlags, ISF_IDENTIFY_BIT );
			SETBIT( spec.dwFlags, ISF_USEABLE_BY_SPAWNER_BIT );
			spec.nItemClass = nClassToSpawn;
			spec.pSpawner = pUnit;
			BOOL bElementEffects[MAX_DAMAGE_TYPES];
			memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

			DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_PICKUP;
			if ( bDebugOutput )
				dwFlags |= ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT;
			s_ItemSpawnForCheats( pGame, spec, UnitGetClient( pUnit ), dwFlags, bElementEffects );

			nNumItemsSpawned++;
		}
	}

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetModSlots(
	UNIT* item)
{
	ASSERT_RETZERO(item);
	return UnitGetStatsTotal(item, STATS_ITEM_SLOTS);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSendItemPlayPickupSound(
	GAME* game,
	UNIT* unit,
	UNIT* item)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(item);
	ASSERT_RETURN(UnitGetGenus(item) == GENUS_ITEM);

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(item));
	if (item_data && item_data->m_nInvPickupSound != INVALID_ID)
	{
		MSG_SCMD_UNITPLAYSOUND msg;
		msg.id = UnitGetId(unit);
		msg.idSound = item_data->m_nInvPickupSound;
		s_SendUnitMessage(unit, SCMD_UNITPLAYSOUND, &msg);
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sItemDecrementOrUse(
	GAME* game, 
	UNIT* item,
	BOOL bFromPickup)
{
	ASSERT_RETFALSE(game && item);
	int quantity = ItemGetQuantity(item);
	if (quantity <= 1)
	{
		BOOL bSendFreeMessage = TRUE;
		
		// we allow the the pickup and decrement/use of items that are not in the 
		// world in the case of gold that xfers a stat and goes away completely
		if (bFromPickup == TRUE && UnitGetRoom( item ) == NULL)
		{
			bSendFreeMessage = FALSE;
		}
		
		// free unit
		UnitFree(item, bSendFreeMessage ? UFF_SEND_TO_CLIENTS : 0 );

		// all items in stack used		
		return TRUE;
		
	}
	
	ItemSetQuantity( item, ItemGetQuantity( item ) - 1 );

	// ok, here's something for ya: if we're decrementing the stack on items that are in the player's hotkeys, 
	//   the UI needs to know about it so it can update the count display.  So what we're gonna do is if this item
	//   (or another of its type, 'cause the count includes those) is in the player's hotkeys, we're going to 
	//   re-send the hotkey info which will cause the UI to reload for that hotkey.
	if (IS_SERVER(game))
	{
		UNIT *pUltimateOwner = UnitGetUltimateOwner(item);
		if (pUltimateOwner && UnitIsA(pUltimateOwner, UNITTYPE_PLAYER))
		{
			for (int ii = 0; ii <= TAG_SELECTOR_HOTKEY12; ii++)
			{
				UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(pUltimateOwner, ii);
				if (!tag || tag->m_idItem[0] == INVALID_ID)
				{
					continue;
				}

				UNIT *pTagItem = UnitGetById(game, tag->m_idItem[0]);

				if (!UnitIsA(item, UnitGetType(pTagItem)))
				{
					continue;
				}

				// send the hotkey tag again
				GAMECLIENT *pClient = UnitGetClient( pUltimateOwner );
				ASSERTX_RETFALSE( pClient, "Item has no client id" );
				HotkeySend( pClient, pUltimateOwner, (TAG_SELECTOR)ii );
			}
		}
	}

	return FALSE;  // more items left in stack
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* sPickupStatTransfer(
	GAME* game, 
	UNIT* unit,
	UNIT* item)
{
	StatsTransfer(unit, item);
	if( AppIsHellgate() )
	{
		sSendItemPlayPickupSound(game, unit, item);
	}

	if (sItemDecrementOrUse(game, item, TRUE))
	{
		return NULL;  // item was totally used up and is now gone
	}
	return item;  // there are still items left in this item stack
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* sPickupUse(
	GAME* game, 
	UNIT* unit,
	UNIT* item)
{
	s_ItemUse(game, unit, item);
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sCancelPullTo(GAME * game, UNIT * unit, const struct EVENT_DATA & eventData);
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAtTargetItemPickup( 
	UNIT *pItem,
	UNIT *pPlayer)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)	
	ASSERTX_RETURN( pItem, "Expected item" );
	// cancel the pull to
	GAME *pGame = UnitGetGame( pItem );
	s_sCancelPullTo( pGame, pItem, NULL );

	// pick up if we have a player
	if (pPlayer)
	{
		s_ItemPickup( pPlayer, pItem );
	}
	
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sAtTargetDoItemPickup(
	GAME * game,
	UNIT * unit,
	UNIT * target,
	EVENT_DATA * eventData,
	void * data,
	DWORD dwId)
{
	UNITID idPlayer = (UNITID)eventData->m_Data1;
	UNIT * player = UnitGetById(game, idPlayer);
	sDoAtTargetItemPickup( unit, player );	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sCancelPullTo( 
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & eventData)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);
	const UNIT_DATA * item_data = ItemGetData(unit);
	ASSERT_RETFALSE(item_data);
	
	// allow pickup again
	UnitSetFlag(unit, UNITFLAG_CANBEPICKEDUP);
	
	// not being picked up by anybody
	UnitClearStat(unit, STATS_BEING_PICKED_UP_BY_UNITID, 0);
	UnitClearStat(unit, STATS_PICKUP_ATTEMPTS, 0);
	
	// clear state
	if (item_data->nPickupPullState != INVALID_LINK)
	{
		s_StateClear(unit, UnitGetId(unit), item_data->nPickupPullState, 0);
	}
		
	// clear mode
	if (AppIsHellgate())
	{
		UnitEndMode(unit, MODE_ITEM_PULLED);
	}
	
	// take off step list
	UnitStepListRemove(game, unit, SLRF_FORCE);
	UnitSetVelocity(unit, 0.0f);
		
	// remove event handler
	UnitUnregisterEventHandler(game, unit, EVENT_REACHED_PATH_END, s_sAtTargetDoItemPickup);
		
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemGetPickupLocation(
	UNIT * player,
	UNIT * item,
	UNIT ** container,
	int * location,
	int * x,
	int * y,
	DWORD dwChangeLocationFlags = 0)
{
	if (UnitIsPlayer(player) && PlayerIsSubscriber(player))
		//have to do "is player" check at this point, since "player" can actually be a monster (ex: imp + item)
	{
		// try backpack first, if we have one
		UNIT *pInvItem = NULL;
		while ((pInvItem = UnitInventoryIterate(player, pInvItem, 0)) != NULL)
		{
			if (UnitIsA(pInvItem, UNITTYPE_BACKPACK))
			{
				if (ItemGetOpenLocation(pInvItem, item, TRUE, location, x, y, FALSE, dwChangeLocationFlags | CLF_CHECK_PLAYER_PUT_RESTRICTED, 0))
				{
					if (container)
					{
						*container = pInvItem;
					}
					return TRUE;
				}
			}
		}
	}

	if (container)
	{
		*container = player;
	}
	if( UnitIsPlayer(player) )
	{
		dwChangeLocationFlags |= CLF_CHECK_PLAYER_PUT_RESTRICTED;
	}
	return ItemGetOpenLocation(player, item, TRUE, location, x, y, FALSE, dwChangeLocationFlags, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemCanTryToPickup(
	UNIT * unit,
	UNIT * item)
{
		
	// tugboat always can do this, even if you can't it will flippy
	if (AppIsTugboat())
	{
		return TRUE;
	}

	if (ItemCanAutoStack(unit, item) != NULL)
	{
		return TRUE;
	}
	
	int location, x, y;
	if (!sItemGetPickupLocation(unit, item, NULL, &location, &x, &y))
	{
		return FALSE;
	}

	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sDoPickupPullTo(
	UNIT * unit,
	UNIT * item)
{
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	const UNIT_DATA * item_data = ItemGetData(item);

	// if pickup requires inventory space, we must be able to pick the item up
	// must be able to pickup
	if (UnitDataTestFlag(item_data, UNIT_DATA_FLAG_PICKUP_REQUIRES_INVENTORY_SPACE))
	{
		if (sItemCanTryToPickup(unit, item) == FALSE)
		{
			s_ItemSendCannotPickup(unit, item, PR_FULL_ITEMS);
			return;
		}
	}

	// do not pickup money if you can't hold anymore
	if (UnitIsA(item, UNITTYPE_MONEY) && UnitCanHoldMoreMoney(unit) == FALSE)
	{
		return;
	}

	// if this same client has already put in a pickup request for this item, we will just
	// insta-pick-it-up ... this will maybe solve items getting mysteriously stuck
	// in some levels (covent garden market is particularly problematic for some reason)
	UNITID idBeingPickedUpBy = UnitGetStat( item, STATS_BEING_PICKED_UP_BY_UNITID );
	if (idBeingPickedUpBy == UnitGetId( unit ))
	{
		// we now have another pickup attempt
		int nNumAttempts = UnitGetStat( item, STATS_PICKUP_ATTEMPTS );
		ASSERTX_RETURN( nNumAttempts >= 1, "Expected 1 or more pickup attempts" );
		nNumAttempts++;
		if (nNumAttempts >= NUM_PICKUP_ATTEMPTS_TO_CANCEL_ITEM_PULL_TO)
		{
			sDoAtTargetItemPickup( item, unit );
		}
		else
		{
			UnitSetStat( item, STATS_PICKUP_ATTEMPTS, nNumAttempts );
		}
		return;
	}
	
	// record who is trying to pick the item up
	UnitSetStat( item, STATS_BEING_PICKED_UP_BY_UNITID, UnitGetId( unit ));
	UnitSetStat( item, STATS_PICKUP_ATTEMPTS, 1 );
	
	// nobody else can pick this up now
	UnitClearFlag(item, UNITFLAG_CANBEPICKEDUP);
	
	// end any jump
	UnitJumpCancel(item);
	
	// setup what to do when we reach the target
	EVENT_DATA tEventData;
	tEventData.m_Data1 = UnitGetId(unit);
	UnitRegisterEventHandler(game, item, EVENT_REACHED_PATH_END, s_sAtTargetDoItemPickup, &tEventData);
	
	// set a speed and target
	UnitSetVelocity(item, ITEM_PULL_TO_VELOCITY);
	UnitSetMoveTarget(item, UnitGetId(unit));
	UnitSetStatFloat(item, STATS_STOP_DISTANCE, 0, 0.1f);
	UnitSetStat(item, STATS_STOP_DISTANCE_IGNORE_TARGET_COLLISION, TRUE);

	// put on the step list to do movement
	UnitStepListAdd(game, item);

	// tell clients to move the unit
	s_SendUnitMoveID(item, 0, MODE_ITEM_PULLED, UnitGetId(unit), cgvNone);

	// set state (if specified)
	if (item_data->nPickupPullState != INVALID_LINK)
	{
		s_StateSet(item, unit, item_data->nPickupPullState, 0);
	}

	// set an event to make us pickup-able again in case something keeps us from our target
	UnitRegisterEventTimed(item, s_sCancelPullTo, NULL, GAME_TICKS_PER_SECOND * 4);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemCheckPickupTag(
	UNIT * unit,
	UNIT * item)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(item);
	UNIT_TAG_PICKUP * tag = (UNIT_TAG_PICKUP *)UnitGetTag(item, TAG_SELECTOR_PICKUP);
	if (!tag || tag->guidUnit == INVALID_GUID)
	{
		return TRUE;
	}
	if (IS_SERVER(UnitGetGame(unit)))
	{
		if (tag->guidUnit == UnitGetGuid(unit))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PICKUP_RESULT ItemCanPickup(
	UNIT *pUnit,
	UNIT *pItem,
	DWORD dwFlags)
{

	if (pUnit == NULL || pItem == NULL)
	{
		return PR_FAILURE;
	}
	
	GAME *pGame = UnitGetGame( pUnit );

	// units being deleted cannot pickup or be picked up, this is necessessary as an always
	// done check instead of relying on the ClientCanKnowUnit() call below
	// because it can be bypassed
	if (UnitTestFlag( pUnit, UNITFLAG_FREED ) ||
		UnitTestFlag( pItem, UNITFLAG_FREED ))
	{
		return PR_FAILURE;
	}
	
	// you cannot pickup stuff when you're a ghost
	if (UnitIsGhost( pUnit ))
	{
		return PR_FAILURE;
	}
		
	// check pickup tag
	if (sItemCheckPickupTag( pUnit, pItem ) == FALSE)
	{
		return PR_FAILURE;
	}
	
	// make sure the player can know about this item
	if ( !( dwFlags & ICP_DONT_CHECK_KNOWN_MASK ) && UnitHasClient( pUnit ))
	{
		GAMECLIENT *pClient = UnitGetClient( pUnit );		
		if (ClientCanKnowUnit( pClient, pItem ) == FALSE)
		{
			return PR_FAILURE;
		}
	}

	// check conditions
	if ( !( dwFlags & ICP_DONT_CHECK_CONDITIONS_MASK ) )
	{
	
		// check the pickup script
		int nItemClass = UnitGetClass( pItem );
		if (ExcelHasScript( pGame, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codePickupCondition), nItemClass ))
		{
			if (!ExcelEvalScript(
					pGame, 
					pUnit, 
					pItem, 
					NULL, 
					DATATABLE_ITEMS, OFFSET( UNIT_DATA, codePickupCondition ), 
					nItemClass))
			{
				return PR_FAILURE;
			}
			
		}
		
		// check for max pickup
		const UNIT_DATA *pItemData = ItemGetData( pItem );
		if (pItemData->nMaxPickup != 0)
		{
			int nCount = UnitInventoryCountItemsOfType( pUnit, nItemClass, 0 );
			if (nCount >= pItemData->nMaxPickup)
			{
				return PR_MAX_ALLOWED;
			}
		}

	}
		
	return PR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ItemPullToPickup(
	UNIT * unit,
	UNIT * item)
{
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	if (UnitGetGenus(item) == GENUS_ITEM)
	{
		if (ItemCanPickup( unit, item ) != PR_OK)
		{
			return;
		}
	}
	if (!AppIsHellgate())
	{
		s_ItemPickup(unit, item);
		return;
	}

	s_sDoPickupPullTo(unit, item);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* sPickupAddToInventory(
	GAME* game, 
	UNIT* unit,
	UNIT* item)
{
	if( !s_QuestEventCanPickup( unit, item )  )
	{
		return NULL;
	}

	UNIT* stackedItem = ItemAutoStack(unit, item);
	
	// item auto stack should return either the item or the stack the item is now a part of, but it
	// should *not* return NULL
	if (stackedItem == NULL)
	{
		return NULL;
	}

	// if item was absorbed into another item
	if (stackedItem != item)
	{
		// Items picked up - we better add it to the achievements
		if( UnitIsPlayer(unit ) )
		{
			s_AchievementRequirmentTest( unit, unit, item, NULL );
		}
		//set the item as having been picked up at least once
		UnitSetStat( stackedItem, STATS_ITEM_PICKEDUP, 1 );		
		// invalidate old item
		item = NULL;  // old item is no longer valid, this is just to be explicit so we don't use it
		
		// if player picked up an item, tell quest system
		if( UnitIsPlayer( unit ) )
		{
			// quest system needs to know about stacked items too, non stacked items will do
			// this from inside the inventory code
			s_QuestEventPickup( unit, stackedItem ); 
		}		

		return stackedItem;
		
	}
	int location, x, y;
	UNIT *container;
	DWORD nFlags( 0 );
	if( ItemBindsOnEquip( item ) &&
		!ItemIsBinded( item ) )
	{
		nFlags = CLF_DO_NOT_EQUIP;
	}
	if (!sItemGetPickupLocation(unit, item, &container, &location, &x, &y, nFlags))
	{
		return item;	// this will be an error, but the item representation itself hasn't changed (ie, was not absorbed into an existing stack of items)
	}
	ASSERT_RETVAL(location != INVLOC_NONE, item);

	// unshine the item
	sItemUnShine( game, item );
	
	BOOL bItemAdded = UnitInventoryAdd(INV_CMD_PICKUP, container, item, location, x, y, nFlags );
	ASSERT(bItemAdded);

#if !ISVERSION( CLIENT_ONLY_VERSION )
	if (UnitIsPlayer(unit))
	{
		s_PlayerCheckMinigame(game, unit, MINIGAME_ITEM_TYPE, UnitGetType(item));

		if (ItemGetQuality(item) != GlobalIndexGet( GI_ITEM_QUALITY_NORMAL ))
		{
			s_PlayerCheckMinigame(game, unit, MINIGAME_ITEM_MAGIC_TYPE, 0);
		}
	}
#endif

	// When an item flippies, it has to be on the ground or it won't move.
	UnitSetOnGround( item, FALSE );
	// Items picked up - we better add it to the achievements
	if( UnitIsPlayer(unit ) )
	{
		s_AchievementRequirmentTest( unit, unit, item, NULL );
	}
	//set the item as having been picked up at least once
	UnitSetStat( item, STATS_ITEM_PICKEDUP, 1 );
	return item;
	
}

//----------------------------------------------------------------------------
struct PICKUP_FUNCTION_LOOKUP
{
	const char* pszName;				// function name
	PFN_PICKUP_FUNCTION pfnFunction;	// function pointer
	BOOL bRequireFreeInventorySpace;	// requires free invnetory space before pickup logic is run
};
static PICKUP_FUNCTION_LOOKUP sgtPickupFunctionLookup[] = 
{
	{ "PickupStatTransfer",				sPickupStatTransfer,			FALSE },
	{ "PickupAddToInventory",			sPickupAddToInventory,			TRUE },
	{ "PickupUse",						sPickupUse,						FALSE },

	{ NULL,	NULL }		// keep this last
};

//----------------------------------------------------------------------------
static const PICKUP_FUNCTION_LOOKUP *sLookupPickupFunction( 
	const char* pszName)
{
	// go through lookup table
	for (int i = 0; sgtPickupFunctionLookup[ i ].pszName != NULL; ++i)
	{
		const PICKUP_FUNCTION_LOOKUP* pLookup = &sgtPickupFunctionLookup[ i ];
		if (PStrICmp( pLookup->pszName, pszName ) == 0)
		{
			return pLookup;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelItemsPostProcess(
	struct EXCEL_TABLE * table)
{
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		UNIT_DATA * unit_data = (UNIT_DATA *)ExcelGetDataPrivate(table, ii);
		if ( ! unit_data )
			continue;
		if( AppIsHellgate() )
		{
			unit_data->fCollisionRadius	= ITEM_COLLISION_RADIUS;
			unit_data->fCollisionHeight = ITEM_COLLISION_HEIGHT;
			unit_data->fPathingCollisionRadius = ITEM_COLLISION_RADIUS;
		}
		else
		{
			unit_data->fCollisionRadius	= ITEM_COLLISION_RADIUS_MYTHOS;
			unit_data->fCollisionHeight = ITEM_COLLISION_HEIGHT_MYTHOS;
			unit_data->fPathingCollisionRadius = ITEM_COLLISION_RADIUS_MYTHOS;
		}

		// special effects
		SpecialEffectPostProcess(unit_data);
				
		// do function pointer lookups
		if (PStrLen(unit_data->szPickupFunction) > 0)
		{
			const PICKUP_FUNCTION_LOOKUP * pickupLookup = sLookupPickupFunction(unit_data->szPickupFunction);
			ASSERTX_CONTINUE(pickupLookup, "Unable to find pickup lookup for item");
			
			// save function
			unit_data->pfnPickup = pickupLookup->pfnFunction;
			ASSERTX_CONTINUE(unit_data->pfnPickup, "Unable to find pickup function for item");
			
			// does it require free inventory space to pickup
			SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_PICKUP_REQUIRES_INVENTORY_SPACE, pickupLookup->bRequireFreeInventorySpace);
		}		
	}

	ExcelPostProcessUnitDataInit(table, TRUE);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const UNIT_DATA * ItemGetData(
	int nClass)
{
	return (const UNIT_DATA *)ExcelGetData(NULL, DATATABLE_ITEMS, nClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const UNIT_DATA * ItemGetData(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected unit" );
	return (const UNIT_DATA *)ExcelGetData(NULL, DATATABLE_ITEMS, UnitGetClass( pItem ));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const UNIT_DATA * ItemModGetData(
	int nModClass)
{
	return (const UNIT_DATA *)ExcelGetData(NULL, DATATABLE_ITEMS, nModClass);
}

//----------------------------------------------------------------------------
//tugboat luck Added
void ItemSpawnSpecAddLuck( 
	ITEM_SPEC &tSpec,
	UNIT * unit )
{		
	tSpec.nLuckModifier = ( unit && UnitIsA( unit, UNITTYPE_PLAYER ) )?UnitGetLuck( unit ):0.0f;
	tSpec.pLuckOwner = unit;
}
//end tugboat luck added

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSpawnSpecInit(
	ITEM_SPEC &tSpec)
{
	tSpec.idUnit = INVALID_ID;
	tSpec.guidUnit = INVALID_GUID;
	tSpec.nItemClass = INVALID_LINK;
	tSpec.nItemLookGroup = INVALID_LINK;
	tSpec.nTeam = INVALID_TEAM;
	tSpec.nAngle = 0;
	tSpec.flScale = 1.0F;
	tSpec.vDirection = cgvNone;
	tSpec.vUp = cgvZAxis;
	tSpec.nLevel = -1;
	tSpec.nQuality = INVALID_LINK;
	tSpec.nSkill = INVALID_LINK;
	tSpec.nNumber = 0;
	tSpec.nMoneyAmount = -1;
	tSpec.nRealWorldMoneyAmount = 0;
	tSpec.dwFlags = 0;
	tSpec.pSpawner = NULL;
	tSpec.unitOperator = NULL;
	tSpec.guidOperatorParty = INVALID_GUID;
	tSpec.guidRestrictedTo = INVALID_GUID;
	tSpec.pvPosition = NULL;
	tSpec.pRoom = NULL;
	tSpec.pOpenNodes = NULL;
	tSpec.nOverrideInventorySizeX = 0;
	tSpec.nOverrideInventorySizeY = 0;
	tSpec.nLuckModifier = 0.0f;
	tSpec.iLuckModifiedItem = 0;
	tSpec.pLuckOwner = NULL;
	tSpec.pMerchant = NULL;
	tSpec.pfnSpawnCallback = NULL;
	tSpec.eLoadType = UDLT_ALL;

	for (int ii = 0; ii < MAX_SPEC_AFFIXES; ii++)
	{
		tSpec.nAffixes[ii] = INVALID_LINK;
	}

}	

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
BOOL ItemsAreIdentical(
	UNIT * item1,
	UNIT * item2)
{
	if (UnitGetRider(item1, NULL))
	{
		return FALSE;
	}
	if (UnitGetRider(item2, NULL))
	{
		return FALSE;
	}
	if (item1->species != item2->species)
	{
		return FALSE;
	}
	if (ItemGetQuality( item1 ) != ItemGetQuality( item2 ))
	{
		return FALSE;
	}

	const UNIT_DATA * item1data = UnitGetData(item1);
	const UNIT_DATA * item2data = UnitGetData(item2);
	if ((UnitDataTestFlag(item1data, UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY) ||
		 UnitDataTestFlag(item2data, UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY)) &&
		UnitGetStat(item1, STATS_ITEM_DIFFICULTY_SPAWNED) != UnitGetStat(item2, STATS_ITEM_DIFFICULTY_SPAWNED))
	{
		return FALSE;
	}

	// for now this does not take into account affixes and all that stuff, etc.
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemHasStackSpaceFor(
	UNIT * pDest,
	UNIT * pSource)
{
	if (ItemsAreIdentical( pDest, pSource ))
	{
		int nMaxQuantity = UnitGetStat( pDest, STATS_ITEM_QUANTITY_MAX);
			
		int nSourceQuantity = ItemGetQuantity( pSource );
		int nDestQuantity = ItemGetQuantity( pDest );
		int nDestReservedQuantity = UnitGetStat( pDest, STATS_ITEM_QUANTITY_RESERVED );
		
		if (nDestQuantity + nDestReservedQuantity + nSourceQuantity <= nMaxQuantity)
		{
			return TRUE;
		}
		
	}
	
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemPickupFromStack(
	UNIT * pPlayer,
	UNIT * pItem,
	int nQuantity)
{
	GAME *pGame = UnitGetGame(pItem);
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(IS_SERVER(pGame));
	ASSERT_RETFALSE(UnitIsA(pItem, UNITTYPE_ITEM));

#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(UnitIsPlayer(pPlayer));
	ASSERT_RETFALSE(pPlayer == UnitGetUltimateContainer(pItem));

	int nOrigQuantity = ItemGetQuantity(pItem);
	if (nOrigQuantity > nQuantity)
	{
		// make sure the cursor is empty (cause the result will go there)
		int nCursorLoc = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
		if (UnitInventoryLocationIterate(pPlayer, nCursorLoc, NULL) != NULL)
		{
			return FALSE;
		}

		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation(pItem, &tInvLoc);
		if (InvLocIsOnlyKnownWhenStashOpen(pPlayer, tInvLoc.nInvLocation))
		{
			if (!StashIsOpen(pPlayer))
			{
				return FALSE;
			}
		} 
		else if (!InvLocIsOnPersonLocation(pPlayer, tInvLoc.nInvLocation))
		{
			return FALSE;
		}

		// make a COPY OF THE ITEM in limbo
		int nMaxStack = UnitGetStat(pItem, STATS_ITEM_QUANTITY_MAX);
		ASSERT_RETFALSE(nMaxStack > 1);
		
		ITEM_SPEC tSpec;
		tSpec.nItemClass = UnitGetClass(pItem);
		tSpec.nQuality = ItemGetQuality(pItem);
		SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
		UNIT *pNewItem = s_ItemSpawn( pGame, tSpec, NULL );
		UnitSetStat( pNewItem, STATS_ITEM_QUANTITY_MAX, nMaxStack );

		// set the appropriate stack sizes
		ItemSetQuantity( pNewItem, nQuantity );
		ItemSetQuantity( pItem, nOrigQuantity - nQuantity );

		// this item can now be sent to the client
		UnitSetCanSendMessages( pNewItem, TRUE );

		// put the new item in the cursor
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pPlayer, pNewItem, nCursorLoc, CLF_IGNORE_NO_DROP_GRID_BIT);
		return TRUE;
	}

#endif

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemSendCannotPickup(
	UNIT *pUnit,
	UNIT *pItem,
	PICKUP_RESULT eReason)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pItem, "Expected item" );

	// only send for units that have clients
	if (UnitHasClient( pUnit ))
	{
	
		// setup message		
		MSG_SCMD_CANNOT_PICKUP tMessage;
		tMessage.idUnit = UnitGetId( pItem );
		tMessage.eReason = eReason;
		
		// send message
		GAME *pGame = UnitGetGame( pUnit );
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		s_SendMessage( pGame, idClient, SCMD_CANNOT_PICKUP, &tMessage );	

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemRemovePickupTag(
	UNIT * unit)
{		
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	UNIT_TAG_GUID_OWNER * tag = (UNIT_TAG_GUID_OWNER *)UnitGetTag(unit, TAG_SELECTOR_PICKUP);
	if (tag)
	{
		UnitRemoveTag(unit, tag);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemRemovePickupRestrictions(
	UNIT *pItem)
{

	// remove any pickup tag
	sItemRemovePickupTag( pItem );
	
	// remove any GUID restrictions
	UnitSetRestrictedToGUID( pItem, INVALID_GUID );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
UNIT * s_ItemPickup(
	UNIT * container,
	UNIT * item)
{
	ASSERT_RETNULL(container && item);
	GAME * game = UnitGetGame(container);
	ASSERT_RETNULL(game && IS_SERVER(game));
	ASSERTX_RETNULL(UnitIsA(item, UNITTYPE_ITEM), "Expected item");

	// I'm removing this check so we can have merchants & monsters pickup objects, 
	// the right answer is to make them able to pickup stuff too ... but today
	// is a build press day so its the safe answer -Colin
//	ASSERT_RETNULL(UnitTestFlag(container, UNITFLAG_CANPICKUPSTUFF));

	// item must be able to be picked up
	ASSERT_RETNULL(UnitTestFlag(item, UNITFLAG_CANBEPICKEDUP));
	
	// can't be being picked up by somebody already
	ASSERTX_RETNULL( UnitGetStat( item, STATS_BEING_PICKED_UP_BY_UNITID ) == INVALID_ID, "Item already being picked up by somebody, can't pick it up now" );
	ASSERTX_RETNULL( UnitGetStat( item, STATS_PICKUP_ATTEMPTS ) == 0, "Pickup attempts not properly cleared" );

	// you cannot pickup items that are locked for other people
	if (ItemIsLocked(item) == TRUE)
	{
		if (ItemIsLockedForPlayer(item, container) == FALSE)
		{
			s_ItemSendCannotPickup(container, item, PR_LOCKED);
			return NULL;
		}
	}
	
	UNIT *pPickedUpItem = item;
	
	// special case for money style items
	BOOL bCanPickup = TRUE;
	if (UnitIsA(item, UNITTYPE_MONEY))
	{
		if (UnitCanHoldMoreMoney(container) == FALSE)
		{
			bCanPickup = FALSE;
		}
	}
	
	// does picking up this item require inventory space
	const UNIT_DATA* pItemData = ItemGetData(item);	
	if (UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_PICKUP_REQUIRES_INVENTORY_SPACE))
	{
		// if item cannot auto stack we must have a space
		if (ItemCanAutoStack(container, item) == FALSE)
		{		
			// check for free space
			int location, x, y;
			if (!sItemGetPickupLocation(container, item, NULL, &location, &x, &y))
			{
				bCanPickup = FALSE;
				if (AppIsHellgate())
				{
					s_ItemSendCannotPickup(container, item, PR_FULL_ITEMS);			
				}
				if (AppIsTugboat())
				{
					if (IS_SERVER(game))	// flip the item, if we're full
					{
						if ( UnitIsMerchant(container) )
						{
							UnitFree(item);
							return NULL;
						}
						s_SendInventoryFullSound( game, container );
						//s_ItemDrop( container, pPickedUpItem );
						UnitDrop(game, pPickedUpItem, container->vPosition);
					}
				}
			}
		}
	}	
	
	if (bCanPickup && pItemData->pfnPickup)
	{
		// save some info about where the item is before we pick it up
		ROOM * pRoomItem = UnitGetRoom(item);
		if (pRoomItem == NULL)
		{
			pRoomItem = UnitGetRoom(container);
		}
		if (AppIsHellgate())
		{
			// stop any jumping 
			UnitJumpCancel(item);
		}
				
		if( AppIsTugboat() && UnitIsA( item, UNITTYPE_MONEY ) )
		{
			s_StateSet( item, container, STATE_AWARDVISUALGOLD, 0 );
		}
		// do the pickup logic (this might not actually pick up the item ... for instance
		// if the inventory is full)
		pPickedUpItem = pItemData->pfnPickup(game, container, item);

		// if there is still an item (some items go away when you pick them up, like gold
		// or some items stack into other existing items, like health packs), if there
		// is no item (or the item is different) the original one has already been freed
		// and there is no need to inform clients because the free has done it
		if (pPickedUpItem == item)
		{
			if (AppIsTugboat())
			{
				// the item picked up is the same item in the inventory, tell all clients it was picked up
				if (pRoomItem != NULL)
				{
					if (!UnitIsContainedBy(pPickedUpItem, container))
					{
						if(IS_SERVER(game))	// flip the item, if we're full
						{
							bCanPickup = FALSE;
							s_SendInventoryFullSound( game, container );
							//s_ItemDrop( container, pPickedUpItem );
							UnitDrop(game, pPickedUpItem, container->vPosition);
						}
					}
				}
			}
			
			// remove any pickup restrictions this item may have had
			ItemRemovePickupRestrictions( item );
		}
		else
		{
			if( AppIsTugboat() && pPickedUpItem ) // stacking items should still play sounds!
			{
				sSendItemPlayPickupSound( game, container, pPickedUpItem );
			}
		}
	}
	if( bCanPickup &&
		pPickedUpItem &&
		ItemBindsOnPickup( pPickedUpItem ) )
	{
		s_ItemBindToPlayer( pPickedUpItem ); //bind the item on pickup
	}
	return pPickedUpItem;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemNetRemoveFromContainer(
	GAME *pGame,
	UNIT *pItem,
	DWORD dwUnitFreeFlags)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pItem, "Expected item" );
	
	// server only
	if (IS_SERVER( pGame ) == FALSE)
	{
		return;
	}
			
	// get container
	UNIT *pContainer = UnitGetContainer( pItem );
	if (pContainer == NULL)
	{
		return;
	}
	
	// iterate clients
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
	
		// send a remove message for everybody that knows the inventory of the owner
		if (ClientCanKnowUnit( pClient, pItem ))
		{
			s_RemoveUnitFromClient( pItem, pClient, dwUnitFreeFlags );		
		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemSendCannotDrop( 
	UNIT *pUnit,
	UNIT *pItem, 
	DROP_RESULT eReason)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pItem, "Expected item" );
	
	MSG_SCMD_CANNOT_DROP tMessage;
	tMessage.idUnit = UnitGetId( pItem );
	tMessage.eReason = eReason;
	
	GAME *pGame = UnitGetGame( pUnit );
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendMessage( pGame, idClient, SCMD_CANNOT_DROP, &tMessage );	
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DROP_RESULT s_ItemDrop(
	UNIT *pDropper,
	UNIT *pItem,
	BOOL bForce /* = FALSE */,
	BOOL bNoFlippy /*= FALSE*/,
	BOOL bNoRestriction /* = FALSE*/)	
{
	ASSERTX_RETVAL( pDropper, DR_FAILED, "Expected dropper" );
	ASSERTX_RETVAL( pItem, DR_FAILED, "Expected item" );
	GAME *pGame = UnitGetGame( pDropper );
	ASSERTX_RETVAL( IS_SERVER( pGame ), DR_FAILED, "Sever only" );
	int nItemClass = UnitGetClass( pItem );
	UNIT *pOriginalDropper = pDropper;
	
	// a dropper who is not in a room cannot drop an item
	if (UnitGetRoom( pDropper ) == NULL)
	{
		return DR_FAILED;
	}
	
	// unit must be a player
	GAMECLIENT *pClient = UnitGetClient( pDropper );
	if( AppIsTugboat() )
	{
		if( !pClient )
		{
			if( PetIsPet( pDropper ) )
			{
				UNIT* pOwner = UnitGetById( pGame, PetGetOwner( pDropper ) );
				if( pOwner )
				{
					pDropper = pOwner;
					pClient = UnitGetClient( pOwner );
				}
			}
		}
	}
	ASSERTX_RETVAL( pClient, DR_FAILED, "s_ItemDrop() can only be used for players dropping items" );

	DROP_RESULT eResult = InventoryCanDropItem( pDropper, pItem );
	if ( eResult != DR_OK &&
		 eResult != DR_OK_DESTROY &&
		 !bForce )
	{
		return eResult;
	}
		
	// you can only drop items that you are the owner of
	UNITLOG_ASSERT_RETVAL(UnitGetUltimateContainer(pItem) == pDropper, DR_FAILED, pDropper);

	// remove item from the inventory
	if (!UnitInventoryRemove( pItem, ITEM_ALL, CLF_FORCE_SEND_REMOVE_MESSAGE))
	{
		return DR_FAILED;
	}
	ASSERTX_RETVAL( UnitGetContainer( pItem ) == NULL, DR_FAILED, "Unable to remove item from inventory for drop" );

	// restrict for this player
	// tug players drop things and only they can see them.
	// wanna give something away? Trade it! no scamming please.
	if( !bNoRestriction )
	{
		UnitSetRestrictedToGUID( pItem, UnitGetGuid( pDropper ) );
	}

	// don't lose dropped items in room reset depopulation
	if( !AppIsTugboat() )
	{
		UnitSetFlag(pItem, UNITFLAG_DONT_DEPOPULATE, TRUE);		
	}
	// inventory has now changed
	s_InventoryChanged( pDropper );

	// if this was an item that any quests were interested in, we want to give all the
	// quest cast members to see if they have something to say now
	if (AppIsHellgate())
	{
		s_QuestsDroppedItem( pDropper, nItemClass );
	}

	// destroy item if requested, we also do this when the dropper player is being freed because
	// we don't want any items that were on a player to be dropped to the ground as they are
	// removed from the game (via warps, or /leave in a party game etc)
	if (eResult == DR_OK_DESTROY || 
		(UnitIsPlayer( pDropper ) && UnitTestFlag( pDropper, UNITFLAG_JUSTFREED )))
	{
		UnitFree( pItem );	//free the item
		return DR_OK_DESTROYED;
	}

	// Drop it on the ground
	// the owner might not be the ultimateowner - we want to drop at the actual owner ( pet for instance )
	VECTOR vPosition = UnitGetPosition( pOriginalDropper );
	if (bNoFlippy == FALSE)
	{
		vPosition.fZ += 1.0f;  // flippies start of the ground a litte bit
	}
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, UW_FORCE_VISIBLE_BIT );
	SETBIT( dwWarpFlags, UW_ITEM_DO_FLIPPY_BIT, !bNoFlippy );
	UnitWarp( 
		pItem, 
		UnitGetRoom( pDropper ), 
		vPosition, 
		VECTOR(0, 1, 0), 
		VECTOR(0, 0, 1), 
		dwWarpFlags,
		TRUE,
		FALSE);

	// items are hard to see without a sparkley!
	if( AppIsHellgate() )
	{
		ItemShine( pGame, pItem );
	}
	else if( AppIsTugboat() )
	{
		ItemShine( pGame, pItem, vPosition );
	}
				
	return DR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sDoTryItemDrop(
	UNIT *pItem)
{		
	ASSERTX_RETURN( pItem, "Expected unit" );
	
	// setup message and send to server
	MSG_CCMD_INVDROP tMessage;
	tMessage.id = UnitGetId( pItem );
	c_SendMessage( CCMD_INVDROP, &tMessage );

	// hide the item
	UnitWaitingForInventoryMove( pItem, TRUE );

	// play a sound
	static int nDestroySound = INVALID_LINK;
	if (nDestroySound == INVALID_LINK)
	{
		nDestroySound = GlobalIndexGet(  GI_SOUND_DESTROY_ITEM );
	}
	if (nDestroySound != INVALID_LINK)
	{
		c_SoundPlay( nDestroySound );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDropConfirmComplete(
	UI_POSITION *pPrevMousePosition)
{
	// return the cursor back to where it was before
	InputSetMousePosition( pPrevMousePosition->m_fX, pPrevMousePosition->m_fY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientDropItemConfirmed(
	void *pCallbackData,
	DWORD dwCallbackData)
{
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "Expected game" );

	UNITID idItem = (UNITID)dwCallbackData;
	UNIT *pItem = UnitGetById( pGame, idItem );
	if (pItem)
	{
	
		// do the item drop attempt
		sDoTryItemDrop( pItem );

		// restore cursor to position with item
		UI_POSITION *pMousePosition = (UI_POSITION *)pCallbackData;		
		sDropConfirmComplete( pMousePosition );
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientDropItemCancelled(
	void *pCallbackData,
	DWORD dwCallbackData)
{
	UI_POSITION *pMousePosition = (UI_POSITION *)pCallbackData;		
	sDropConfirmComplete( pMousePosition );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemTryDrop(
	UNIT *pItem)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitGetGenus( pItem ) == GENUS_ITEM, "Expected item" );

	if (AppIsTugboat())
	{
		// forget asking, just do it
		sDoTryItemDrop( pItem );
	}
	else
	{
	
		// make sure we can drop it
		UNIT *pPlayer = UIGetControlUnit();
		ASSERTX_RETURN( pPlayer, "Expected player" );
		DROP_RESULT eResult = InventoryCanDropItem( pPlayer, pItem );
		if (eResult == DR_OK)
		{
			static UI_POSITION tMousePosition;  // must be static, we will access it through callbacks
			UIGetCursorPosition( &tMousePosition.m_fX, &tMousePosition.m_fY, FALSE );  // it'd be better if this was the radial menu open point
			
			// setup callbacks
			DIALOG_CALLBACK tOKCallback;
			tOKCallback.pfnCallback = sClientDropItemConfirmed;
			tOKCallback.pCallbackData = &tMousePosition;  // OK since this is a static
			tOKCallback.dwCallbackData = UnitGetId( pItem );

			DIALOG_CALLBACK tCancelCallback;
			tCancelCallback.pfnCallback = sClientDropItemCancelled;
			tCancelCallback.pCallbackData = &tMousePosition;  // OK since this is a static
			tCancelCallback.dwCallbackData = UnitGetId( pItem );
							
			// show confirmation dialog
			UIShowGenericDialog(
				GlobalStringGet( GS_CONFIRM_ITEM_DROP_HEADER ),
				GlobalStringGet( GS_CONFIRM_ITEM_DROP_MESSAGE ),
				TRUE,
				&tOKCallback,
				&tCancelCallback,
				GDF_CENTER_MOUSE_ON_CANCEL);
				
		}

	}
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemSendCannotUse(
	UNIT *pUnit,
	UNIT *pItem,
	USE_RESULT eReason)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pItem, "Expected item" );
	
	MSG_SCMD_CANNOT_USE tMessage;
	tMessage.idUnit = UnitGetId( pItem );
	tMessage.eReason = eReason;
	
	GAME *pGame = UnitGetGame( pUnit );
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendMessage( pGame, idClient, SCMD_CANNOT_USE, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemHasSkillWhenUsed(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected unit" );
	if (UnitIsA( pItem, UNITTYPE_ITEM ))
	{
		const UNIT_DATA * pUnitData = UnitGetData( pItem );
		ASSERT_RETFALSE( pUnitData );
		return pUnitData->nSkillWhenUsed != INVALID_ID;
	}
	return FALSE;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT ItemIsUsable(
	UNIT *pUnit,
	UNIT *pItem,
	UNIT *pItemTarget,
	DWORD nItemUsuableMasks )
{
	ASSERTX_RETVAL( pItem, USE_RESULT_INVALID, "Expected item" );
	ASSERTX_RETVAL( UnitGetGenus( pItem ) == GENUS_ITEM, USE_RESULT_INVALID, "Expected item genus" );
	GAME *pGame = UnitGetGame( pItem );
	const UNIT_DATA* pItemData = ItemGetData( pItem );			

	// quest use item?
	if ( pUnit && ItemIsQuestUsable( pUnit, pItem ) == USE_RESULT_OK )
	{
		return USE_RESULT_OK;
	}

	if (pItemData->nSkillWhenUsed == INVALID_ID)
	{
		return USE_RESULT_NOT_USABLE;
	}

	if (!InventoryIsInLocationType( pItem, LT_STANDARD ))
	{
		return USE_RESULT_CONDITION_FAILED;
	}

	// check for stuff that requires the user unit
	if (pUnit)
	{	
		if (UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY ) &&
			UnitIsA( pUnit, UNITTYPE_PLAYER ) &&
			!PlayerIsSubscriber(pUnit) )
		{
			return USE_RESULT_CONDITION_FAILED;
		}

		if( !InventoryCheckStatRequirements(pUnit, pItem, NULL, NULL, INVLOC_NONE ) )
		{
			return USE_RESULT_CONDITION_FAILED;
		}


		if( (nItemUsuableMasks & ITEM_USEABLE_MASK_ITEM_TEST_TARGETITEM) )
		{
			if( pItemTarget )
			{
				if ( ! SkillCanStart( pGame, pItem, pItemData->nSkillWhenUsed, INVALID_ID, pItemTarget, FALSE, FALSE, 0 ) )
					return USE_RESULT_CONDITION_FAILED;
			}
			else
			{
				return USE_RESULT_CONDITION_FAILED;
			}
		}
		else
		{
			if ( ! SkillCanStart( pGame, pUnit, pItemData->nSkillWhenUsed, INVALID_ID, pItem, FALSE, FALSE, 0 ) )
				return USE_RESULT_CONDITION_FAILED;
		}

		// check the use script
		int nItemClass = UnitGetClass( pItem );
		if (ExcelHasScript( pGame, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeUseCondition), nItemClass ))
		{
			if (!ExcelEvalScript(
				pGame, 
				pUnit, 
				pItem, 
				NULL, 
				DATATABLE_ITEMS, OFFSET( UNIT_DATA, codeUseCondition ), 
				nItemClass))
			{
				return USE_RESULT_CONDITION_FAILED;
			}

		}
	}
	
	return USE_RESULT_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsIdentified(
	UNIT * item )
{
	ASSERT( item );
	return UnitGetStat( item, STATS_IDENTIFIED );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsUnidentified(
	UNIT *pItem)
{
	return ItemIsIdentified( pItem ) == FALSE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsACraftedItem(
	UNIT * item )
{
	ASSERT( item );
	return UnitGetStat( item, STATS_ITEM_CRAFTING_LEVEL, 0 ) > 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT ItemIsQuestUsable(
	UNIT * pUnit,
	UNIT * pItem )
{
	ASSERT_RETVAL( pUnit, USE_RESULT_INVALID );
	ASSERT_RETVAL( pItem, USE_RESULT_INVALID );

	// do I even ask quests?
	const UNIT_DATA * data = ItemGetData( pItem );
	if ( !UnitDataTestFlag(data, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE) )
		return USE_RESULT_INVALID;

	// ask quests if this player can use this item now
	return QuestsCanUseItem( pUnit, pItem );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT ItemIsQuestActiveBarOk(
	UNIT * pUnit,
	UNIT * pItem )
{
	ASSERT_RETVAL( pUnit, USE_RESULT_INVALID );
	ASSERT_RETVAL( pItem, USE_RESULT_INVALID );

	// do I even ask quests?
	const UNIT_DATA * data = ItemGetData( pItem );
	if ( !UnitDataTestFlag(data, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE) )
		return USE_RESULT_INVALID;

	return USE_RESULT_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *ItemGetMerchant(
	UNIT *pItem)
{
	ASSERTX_RETNULL( pItem, "Expected item" );
	ASSERTX_RETNULL( ItemBelongsToAnyMerchant( pItem ), "Item is not in a merchant store" );
	
	int nMerchantClass = UnitGetStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS );
	if (nMerchantClass != INVALID_LINK)
	{
	
		// item is in a personal store, get the player
		UNIT *pPlayer = UnitGetUltimateContainer( pItem );
		ASSERTX_RETNULL( pPlayer, "Expected unit" );
		ASSERTV_RETNULL( UnitIsPlayer( pPlayer ), "Expected player unit: %s, pItem: %s, nMerchantClass: %d", pPlayer->pUnitData->szName, pItem->pUnitData->szName, nMerchantClass );
		
		// get the merchant is who the player is trading with if any, note that a case where there
		// is none is when we exit from a shop ... we are marked as no longer trading with the
		// merchant, but the inventory is sliding off the screen, so the inventory drawing code will
		// still ask for the merchant associated with an item so it can properly draw in the grid
		UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
		if (pTradingWith)
		{
			int nInvLocItem = INVLOC_NONE;
			UnitGetInventoryLocation( pItem, &nInvLocItem );
			if (ItemBelongsToSpecificMerchant( pItem, pTradingWith ) ||
				nInvLocItem == INVLOC_MERCHANT_BUYBACK)		// special case: the buyback items belong to all merchants.
			{
				return pTradingWith;
			}
		}
		return NULL;
		
	}

	// check for items on actual merchants
	UNIT *pOwner = UnitGetUltimateContainer( pItem );
	if (UnitIsMerchant( pOwner ))
	{
		return pOwner;
	}
	
	return NULL;  // item has no merchant
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemBelongsToAnyMerchant(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );

	// if they have any "merchant source" they are in a store
	int nMerchantClass = UnitGetStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS );
	if (nMerchantClass != INVALID_LINK)
	{
		return TRUE;  // an item that is in a personal store
	}

	// check for items actually on shared merchants
	UNIT *pOwner = UnitGetUltimateContainer( pItem );
	if (UnitIsMerchant( pOwner ))
	{
		return TRUE;
	}
	
	return FALSE;  // does not belong to merchant
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemBelongsToSpecificMerchant( 
	UNIT *pItem, 
	int nMerchantUnitClass)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( nMerchantUnitClass != INVALID_LINK, "Invalid merchant unit class" );
	return UnitGetStat( pItem, STATS_MERCHANT_SOURCE_MONSTER_CLASS ) == nMerchantUnitClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemBelongsToSpecificMerchant( 
	UNIT *pItem, 
	UNIT *pMerchant)
{
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	ASSERTX_RETFALSE( pMerchant, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsMerchant( pMerchant ), "Expected merchant" );
	
	int nMerchantUnitClass = UnitGetClass( pMerchant );
	return ItemBelongsToSpecificMerchant( pItem, nMerchantUnitClass );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInSharedStash(
	UNIT *pItem)
{
	ASSERT_RETFALSE(pItem);

	int nLoc;
	UnitGetInventoryLocation(pItem, &nLoc);
	return (nLoc == INVLOC_SHARED_STASH);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUseOnItem(
	GAME *pGame,
	UNIT *pOwner,
	UNIT *pItem,
	UNIT *pItemTarget )
{
	ASSERTX_RETURN( pGame && IS_SERVER( pGame ), "Invalid game" );
	ASSERTX_RETURN( pOwner && pItem && pItemTarget, "Invalid params for item use" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItemTarget, UNITTYPE_ITEM ), "Expected item" );
	

	UNITID idItemToUse = UnitGetId( pItem );
	const UNIT_DATA *pItemData = ItemGetData( pItem );	

	if ( pItemData->nSkillWhenUsed != INVALID_ID )
	{
		// see if the item can be used
		USE_RESULT eResult = ItemIsUsable( pOwner, pItem, pItemTarget, ITEM_USEABLE_MASK_ITEM_TEST_TARGETITEM );
		if (eResult != USE_RESULT_OK)
		{
			s_ItemSendCannotUse( pOwner, pItem, eResult );
			return;
		}
		
		// do the skill
		DWORD dwSkillStartFlags = SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT | SKILL_START_FLAG_INITIATED_BY_SERVER;
		if ( SkillStartRequest( pGame, 
								pItem, 
								pItemData->nSkillWhenUsed, 
								UnitGetId( pItemTarget ), 
								VECTOR(0.0f), 
								dwSkillStartFlags,
								SkillGetNextSkillSeed(pGame) ) )
		{
			const SKILL_DATA * pSkillData = SkillGetData(pGame, pItemData->nSkillWhenUsed);
			if ( UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_CONSUMED_WHEN_USED) && ( !pSkillData || !SkillDataTestFlag(pSkillData, SKILL_FLAG_CONSUME_ITEM_ON_EVENT) ) )
			{
				sItemDecrementOrUse( pGame, pItem, FALSE );
				pItem = NULL;  // NOTE: Item can be invalid at this point ... lets NULL it to be sure
			}
		}
	}
	
	// maybe it is a quest item too
	if ( UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE) )
	{
	
		// validate that the item is still valid
		UNIT *pItem = UnitGetById( pGame, idItemToUse );
		if (pItem == NULL)
		{
			const int MAX_MESSAGE = 1024;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Item '%s' was consumed, but we still want to do things with it",
				pItemData->szName);
			ASSERTX_RETURN( 0, szMessage );
		}
		
		// tell quest systems
		if( AppIsHellgate() )
		{
			s_QuestsUseItem( pOwner, pItem );
		}
		else //must be tugboat
		{
			s_QuestsTBUseItem( pOwner, pItem );
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemUse(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pItem)
{
	ASSERTX_RETURN( pGame && IS_SERVER( pGame ), "Invalid game" );
	ASSERTX_RETURN( pUnit && pItem, "Invalid params for item use" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	

	UNITID idItemToUse = UnitGetId( pItem );
	const UNIT_DATA *pItemData = ItemGetData( pItem );	

	if ( pItemData->nSkillWhenUsed != INVALID_ID )
	{
		// see if the item can be used
		USE_RESULT eResult = ItemIsUsable( pUnit, pItem, NULL, ITEM_USEABLE_MASK_NONE );
		if (eResult != USE_RESULT_OK)
		{
			s_ItemSendCannotUse( pUnit, pItem, eResult );
			return;
		}

		//Tell the achievement system we're using an item
		const UNIT_DATA *pUnitData = UnitGetData(pItem);
		GAME *pGame = UnitGetGame(pUnit);
		if(pUnitData && pGame)
		{	
			if ( pUnitData->pnAchievements )
			{
				// send the use to the achievements system
				for (int i=0; i < pUnitData->nNumAchievements; i++)
				{
					ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, pUnitData->pnAchievements[i]);
					if(pAchievement->eAchievementType == ACHV_ITEM_USE)
					{
						s_AchievementsSendItemUse(pUnitData->pnAchievements[i], pItem, pUnit);
					}
				}
			}
		}

		// do the skill
		DWORD dwSkillStartFlags = SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT | SKILL_START_FLAG_INITIATED_BY_SERVER;
		if( AppIsTugboat() )
		{
			UnitSetStat( pUnit, STATS_SKILL_EXECUTED_BY_ITEMCLASS, UnitGetClass( pItem ) );
		}
		if ( SkillStartRequest( pGame, pUnit, pItemData->nSkillWhenUsed, UnitGetId( pItem ), VECTOR(0.0f), 
			dwSkillStartFlags, SkillGetNextSkillSeed(pGame) ) )
		{
			//set the item that used that skill
			
			const SKILL_DATA * pSkillData = SkillGetData(pGame, pItemData->nSkillWhenUsed);
			if ( UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_CONSUMED_WHEN_USED) && ( !pSkillData || !SkillDataTestFlag(pSkillData, SKILL_FLAG_CONSUME_ITEM_ON_EVENT) ) )
			{
				sItemDecrementOrUse( pGame, pItem, FALSE );
				pItem = NULL;  // NOTE: Item can be invalid at this point ... lets NULL it to be sure
			}
		}
	}

	if( pItemData->codeUseScript != NULL_CODE )
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(pGame, DATATABLE_ITEMS, pItemData->codeUseScript, &codelen);
		if (code)
		{
			VMExecI( pGame, pItem, UnitGetUltimateOwner( pItem ), NULL, code, codelen);	
		}
	}	
		
	// maybe it is a quest item too
	if ( UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE) )
	{
	
		// validate that the item is still valid
		UNIT *pItem = UnitGetById( pGame, idItemToUse );
		if (pItem == NULL)
		{
			const int MAX_MESSAGE = 1024;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Item '%s' was consumed, but we still want to do things with it",
				pItemData->szName);
			ASSERTX_RETURN( 0, szMessage );
		}
		
		// tell quest systems
		if( AppIsHellgate() )
		{
			s_QuestsUseItem( pUnit, pItem );
		}
		else //must be tugboat
		{
			s_QuestsTBUseItem( pUnit, pItem );
		}
		
	}
	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemDecrement(
	UNIT * pItem)
{
	ASSERT_RETFALSE(pItem);
	GAME * pGame = UnitGetGame(pItem);
	ASSERT_RETFALSE(pGame);

	return sItemDecrementOrUse(pGame, pItem, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIPlayItemUseSound(
	UNIT* unit)
{
	ASSERT_RETURN(unit);

	if (UnitGetGenus(unit) != GENUS_ITEM)
	{
		return;
	}
	UNIT* container = UnitGetContainerInRoom(unit);
	if (!container)
	{
		return;
	}
	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
	if (item_data && item_data->m_nInvUseSound != INVALID_ID)
	{
		c_SoundPlay(item_data->m_nInvUseSound, &c_SoundGetListenerPosition(), NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sUseItemCallback( 
	void *pCallbackData, 
	DWORD dwCallbackData )
{
	// tell server to use item
	MSG_CCMD_INVUSE msg;
	msg.idItem = dwCallbackData;
	c_SendMessage(CCMD_INVUSE, &msg);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ItemUse(
	UNIT *pItem)
{
	ASSERT_RETURN(pItem);

	// you cannot use any items in any stores
	if (ItemBelongsToAnyMerchant( pItem ))
	{
		return;
	}

	const UNIT_DATA * pItemData = ItemGetData(pItem);
	ASSERT_RETURN(pItemData);

	// for some items, we know what it's supposed to do so we want to shortcut a round
	// trip to the server if we can in favor of UI responsiveness
	if( UnitIsA( pItem, UNITTYPE_SKILL_SELECTOR ) )
	{
		c_ItemEnterMode( ITEM_SKILL_SELECT, pItem );
	}
	else if (UnitIsA( pItem, UNITTYPE_ANALYZER ))
	{
		c_ItemEnterMode( ITEM_MODE_IDENTIFY, pItem );
	}
	else if (UnitIsA( pItem, UNITTYPE_GUILD_HERALD ))
	{
		c_ItemEnterMode( ITEM_MODE_CREATE_GUILD, pItem );
	}
	else if (UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_CONFIRM_ON_USE))
	{
		DIALOG_CALLBACK tOkCallback;
		DialogCallbackInit( tOkCallback );
		tOkCallback.pfnCallback = sUseItemCallback;
		tOkCallback.pCallbackData = NULL;
		tOkCallback.dwCallbackData = UnitGetId(pItem);
		WCHAR sName[MAX_ITEM_NAME_STRING_LENGTH];
		UnitGetName(pItem, sName, MAX_ITEM_NAME_STRING_LENGTH, 0);
		UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), sName, TRUE, &tOkCallback);
	}
	else
	{
		if( ItemHasSkillWhenUsed(pItem) )
		{
			int nSkillID = UnitGetData( pItem )->nSkillWhenUsed;
			const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkillID );
		
			if ( pSkillData &&
				SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
			{
				GAME *pGame = UnitGetGame( pItem );
				c_PlayerStopPath( pGame );
			}
		}
		// tell server to use item
		MSG_CCMD_INVUSE msg;
		msg.idItem = UnitGetId(pItem);

		if( ItemHasSkillWhenUsed(pItem) &&
			UnitDataTestFlag(UnitGetData(pItem), UNIT_DATA_FLAG_ITEM_IS_TARGETED) )
		{
			msg.TargetPosition = UnitGetStatVector(UnitGetUltimateOwner(pItem), STATS_SKILL_TARGET_X, UnitGetData(pItem)->nSkillWhenUsed, 0 );
		}

		c_SendMessage(CCMD_INVUSE, &msg);
		
	}
			
	// play use sound
	sUIPlayItemUseSound(pItem);

	// clear some UI mouse text
	UIClearHoverText();
	UISendMessage(WM_PAINT, 0, 0);
		
}

void c_ItemUseOn(
			   UNIT *pItem,
			   UNIT *pTarget)
{

	// you cannot use any items in any stores
	if (ItemBelongsToAnyMerchant( pItem ))
	{
		return;
	}

	
	// tell server to use item
	MSG_CCMD_INVUSEON msg;
	msg.idItem = UnitGetId(pItem);
	msg.idTarget = UnitGetId(pTarget);
	c_SendMessage(CCMD_INVUSEON, &msg);

	// play use sound
	sUIPlayItemUseSound(pItem);

	// clear some UI mouse text
	UIClearHoverText();
	UISendMessage(WM_PAINT, 0, 0);

}

void c_ItemShowInChat(
	UNIT *pItem)
{
	ConsoleAddItem(pItem);
}
#endif //!SERVER_VERSION


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ItemSell(	
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pSellToUnit,
	UNIT * pItem)
{
	ASSERT_RETFALSE(pGame && IS_CLIENT(pGame));
	ASSERT_RETFALSE(pUnit && pItem);
	ASSERT_RETFALSE(pUnit == GameGetControlUnit(pGame))

	if (UnitGetUltimateOwner(pItem) == pUnit)
	{
		if ( VectorDistanceSquared( pUnit->vPosition, pSellToUnit->vPosition ) <= ( UNIT_INTERACT_DISTANCE_SQUARED ) )
		{
			if (StoreMerchantWillBuyItem(pSellToUnit, pItem))
			{
				UnitWaitingForInventoryMove( pItem, TRUE );
				MSG_CCMD_ITEMSELL msg;
				msg.idItem = UnitGetId(pItem);
				msg.idSellTo = UnitGetId(pSellToUnit);
				c_SendMessage(CCMD_ITEMSELL, &msg);
				return TRUE;
			}
			else if ( UnitIsNoDrop( pItem ) )
			{
				UIShowMessage( QUIM_DIALOG, DIALOG_NO_SELL );
			}
		}
	}

	return FALSE;
}
#endif //!SERVER_VERSION

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ItemBuy(	
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pShopkeeper,
	UNIT * pItem,
	int nSuggestedLocation /*= INVLOC_NONE*/, 
	int nSuggestedX /*= -1*/,
	int nSuggestedY /*= -1*/)
{
	ASSERT_RETFALSE(pGame && IS_CLIENT(pGame));
	ASSERT_RETFALSE(pUnit && pItem);
	ASSERT_RETFALSE(ItemGetMerchant(pItem) == pShopkeeper)
	ASSERT_RETFALSE(pUnit == GameGetControlUnit(pGame));

	if (ItemGetMerchant(pItem) == pShopkeeper)
	{
		if (UnitIsMerchant(pShopkeeper))
		{
			if ( VectorDistanceSquared( pUnit->vPosition, pShopkeeper->vPosition ) <= ( UNIT_INTERACT_DISTANCE_SQUARED ) )
			{
				cCurrency buyPrice = EconomyItemBuyPrice( pItem );				
				if ( buyPrice.IsZero() == FALSE )
				{
					cCurrency unitCurrency = UnitGetCurrency(pUnit);
					const UNIT_DATA *pItemData = UnitGetData(pItem);
					if (pItemData && UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY) && !UnitHasBadge(pUnit, ACCT_TITLE_SUBSCRIBER) && !(AppTestFlag(AF_TEST_CENTER_SUBSCRIBER) && UnitHasBadge(pUnit, ACCT_TITLE_TEST_CENTER_SUBSCRIBER)))
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
					else if (unitCurrency >= buyPrice)
					{
						int nLocation = INVLOC_NONE;
						DWORD dwItemEquipFlags = 0;
						SETBIT(dwItemEquipFlags, IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT);
						if (ItemGetOpenLocation(pUnit, pItem, TRUE, &nLocation, NULL, NULL, TRUE, 0, dwItemEquipFlags))
						{
							if (UnitGetStat(pItem, STATS_UNLIMITED_IN_MERCHANT_INVENTORY) == 0)
							{
								UnitWaitingForInventoryMove( pItem, TRUE );
							}

							MSG_CCMD_ITEMBUY msg;
							msg.idItem = UnitGetId(pItem);
							msg.idBuyFrom = UnitGetId(pShopkeeper);
							msg.nSuggestedLocation = nSuggestedLocation;
							msg.nSuggestedX = nSuggestedX;
							msg.nSuggestedY = nSuggestedY;

							c_SendMessage(CCMD_ITEMBUY, &msg);
							return TRUE;
						}
						else
						{
							// Gotta watch out for Mythos. Keep getting missing strings everywhere from these uncareful additions
							if( AppIsHellgate() )
							{
								const WCHAR *szMsg = GlobalStringGet(GS_INVENTORY_FULL);
								if (szMsg && szMsg[0])
								{
									UIShowQuickMessage(szMsg, 3.0f);
								}
							}
							
							int nSoundId = AppIsHellgate() ? GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO ) :
															( UnitGetData( pUnit )->m_nInventoryFullSound );
							if (nSoundId != INVALID_LINK)
							{
								c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
							}
						}
					}
					else
					{
						// Gotta watch out for Mythos. Keep getting missing strings everywhere from these uncareful additions
						if( AppIsHellgate() )
						{
							const WCHAR *szMsg = GlobalStringGet(GS_UI_NOT_ENOUGH_MONEY);
							if (szMsg && szMsg[0])
							{
								UIShowQuickMessage(szMsg, 3.0f);
							}
						}
						int nSoundId = AppIsHellgate() ? GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO ) :
														 ( UnitGetData( pUnit )->m_nCantSound );
						if (nSoundId != INVALID_LINK)
						{
							c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
						}
					}
				}
			}
		}
	}

	return FALSE;
}

#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define PICKUP_VIEW_DISTANCE	15.0f
//#define PICKUP_VIEW_DIST_SQ		( PICKUP_VIEW_DISTANCE * PICKUP_VIEW_DISTANCE )

static ALTITEMLIST sgAltItemNameList[ MAX_ALT_LIST_SIZE ];
//static ALTITEMLIST sgFarItems[ MAX_ALT_LIST_SIZE ];
static int sgnAltItemListSize = 0;
static ALTITEMLIST sgClosest = { FALSE, 0, NULL };

typedef void ITEM_ITERATE_FUNCTION( UNIT * testunit, float dist_sq, DWORD * pParam1, DWORD * pParam2 );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sItemIterateAltList( UNIT * testunit, float dist_sq, DWORD * pParam1, DWORD * pParam2  )
{
	ASSERT( pParam1 );
	ASSERT( pParam2 );
	if( AppIsTugboat() )
	{
		if ( *pParam1 < MAX_ALT_LIST_SIZE )
		{
			if ( dist_sq < ALT_DIST_SQ &&
				testunit->bVisible &&
				//!testunit->bStructural &&
				!UnitIsA( testunit, UNITTYPE_DESTRUCTIBLE ) )
			{
				ALTITEMLIST *pAltItemList = &sgAltItemNameList[ *pParam1 ];		
				pAltItemList->bInPickupRange = TRUE;
	
				// Name with affixes
				DWORD dwFlags = 0;
				SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
				UnitGetName( testunit, pAltItemList->szDescription, MAX_ITEM_NAME_STRING_LENGTH, dwFlags );
	
				// Name without affixes
				pAltItemList->idItem = UnitGetId(testunit);
				(*pParam1)++;
			}
		}
	}
	if( AppIsHellgate() )
	{
		if ( *pParam1 < MAX_ALT_LIST_SIZE )
		{
			if ( dist_sq < PICKUP_RADIUS_SQ )
	
	
	
			{
				ALTITEMLIST *pAltItemList = &sgAltItemNameList[ *pParam1 ];		
				pAltItemList->bInPickupRange = TRUE;
	
				// Name with affixes
				DWORD dwFlags = 0;
				SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
				UnitGetName( testunit, pAltItemList->szDescription, arrsize(pAltItemList->szDescription), dwFlags );
	
				// Name without affixes
				pAltItemList->idItem = UnitGetId(testunit);
				(*pParam1)++;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sItemIterateClosest( UNIT * testunit, float dist_sq, DWORD * pParam1, DWORD * pParam2  )
{
	ASSERT( pParam1 );
	float * pClosest = ( float * )pParam1;
	if ( dist_sq < *pClosest )
	{
		sgClosest.bInPickupRange = TRUE;
		sgClosest.idItem = UnitGetId(testunit);

		// Name with affixes
		DWORD dwFlags = 0;
		SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );									
		UnitGetName( testunit, sgClosest.szDescription, arrsize(sgClosest.szDescription), dwFlags );

		// Name without affixes
//		UnitGetClassString( testunit, sgClosest.szDescription, MAX_ITEM_NAME_STRING_LENGTH );
		*pClosest = dist_sq;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sIterateNearbyItems( UNIT * unit, ITEM_ITERATE_FUNCTION * pfnFuncion, DWORD * pParam1, DWORD * pParam2, UNIT *SelectedUnit  )
{
	ASSERT( unit );
	ASSERT( pfnFuncion );

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];

	GAME * game = UnitGetGame( unit );

	if( SelectedUnit )
	{
		if( !( UnitIsA(SelectedUnit, UNITTYPE_OBJECT) ||
				   UnitIsA(SelectedUnit, UNITTYPE_DESTRUCTIBLE) ||
				   UnitTestFlag( SelectedUnit, UNITFLAG_CANBEPICKEDUP ) )  )
			{
				return;
			}
			/*if ( UnitTestFlag(testunit, UNITFLAG_AUTOPICKUP) )
			{
				continue;
			}*/
			float dist_sq = VectorDistanceSquared( unit->vPosition, SelectedUnit->vPosition );
			pfnFuncion( SelectedUnit, dist_sq, pParam1, pParam2 );
			return;
	}
	SKILL_TARGETING_QUERY query;
	// we include enemy targets so we can target crates.
	SETBIT( query.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
	SETBIT( query.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
	SETBIT( query.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	query.tFilter.nUnitType = UNITTYPE_ANY;
	query.fMaxRange	= ALT_DISTANCE;

	query.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	query.pnUnitIds   = pnUnitIds;
	query.pSeekerUnit = unit;
	SkillTargetQuery( game, query );

	for ( int i = 0; i < query.nUnitIdCount; i++ )
	{
		UNIT * testunit = UnitGetById( game, query.pnUnitIds[ i ] );
		if ( ! testunit )
		{
			continue;
		}

		if( !( UnitIsA(testunit, UNITTYPE_OBJECT) ||
			   UnitIsA(testunit, UNITTYPE_DESTRUCTIBLE) ||
			   UnitTestFlag( testunit, UNITFLAG_CANBEPICKEDUP ) ) ||
			   ObjectIsDoor( testunit ) )
		{
			continue;
		}
		float dist_sq = VectorDistanceSquared( unit->vPosition, testunit->vPosition );
		pfnFuncion( testunit, dist_sq, pParam1, pParam2 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_3RD_PERSON

static UNITID sCursorSelectItem(
	GAME* game )
{
	if (c_CameraGetViewType() != VIEW_3RDPERSON)
	{
		return INVALID_ID;
	}

	if ( !game )
	{
		return INVALID_ID;
	}

	UNIT * player = GameGetControlUnit( game );
	if ( !player )
	{
		return INVALID_ID;
	}

	if ( !UnitGetRoom(player) )
	{
		return INVALID_ID;
	}

	#define SELECTION_ERROR		0.1f

	VECTOR vPosition, vDirection;
	void MouseToScene( GAME * pGame, VECTOR *pvOrig, VECTOR *pvDir );
	MouseToScene( game, &vPosition, &vDirection );
	VectorNormalize( vDirection, vDirection );
	UNIT* selected_unit = NULL;
	float fRange = 500.0f;
	float fMaxLen = LevelLineCollideLen( game, UnitGetLevel( player ), vPosition, vDirection, fRange ) + SELECTION_ERROR;

	// used in point line calcs
	VECTOR vEndPoint;
	vEndPoint.fX = (vDirection.fX * fMaxLen) + vPosition.fX;
	vEndPoint.fY = (vDirection.fY * fMaxLen) + vPosition.fY;
	vEndPoint.fZ = (vDirection.fZ * fMaxLen) + vPosition.fZ;

	VECTOR vEdge;
	VectorSubtract(vEdge, vEndPoint, vPosition);
	float fLengthSq = VectorLengthSquared(vEdge);
	float f2dLengthSq = (vEdge.fX * vEdge.fX) + (vEdge.fY * vEdge.fY);
	float fClosestSq = fLengthSq;

	// this needs to be optimized to only search through rooms that matter
	for (ROOM * room = LevelGetFirstRoom(UnitGetLevel(player)); room; room = LevelGetNextRoom(room))
	{
		for (UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext)
		{
			if ( UnitGetGenus( test ) != GENUS_ITEM )
			{
				continue;
			}

			// is it the closest unit we are dealing with?
			//float dist_sq = VectorDistanceSquared( vPosition, test->vPosition );
			VECTOR vPos = test->vPosition;
			vPos.fZ += 0.5f;
			float dist_sq = VectorDistanceSquared( vEndPoint, vPos );
			if ( dist_sq > fClosestSq )
			{
				continue;
			}

			BOUNDING_BOX box;
			// first grab the unit's model bounding box
			int nModelId = c_UnitGetModelId( test );
			if ( nModelId == INVALID_ID )
			{
				continue;
			}

			e_GetModelRenderAABBInWorld( nModelId, &box );

			// find center point
			VECTOR vCenter = box.vMax + box.vMin;
			VectorScale( vCenter, vCenter, 0.5f );

			// find closest point from our selection ray to this unit's center of their bounding box

			// I have no idea where the bug is in this code. I have looked at it for 2 days
			// and can't figure it out, so I put back the unoptimized vector-point distance
			// routine and will figure this out later - drb
			float fU = ( ( ( vCenter.fX - vPosition.fX ) * vEdge.fX ) +
							( ( vCenter.fY - vPosition.fY ) * vEdge.fY ) +
							( ( vCenter.fZ - vPosition.fZ ) * vEdge.fZ ) ) /
							fLengthSq;

			// Check if point falls within the line segment
			if ( fU < 0.0f || fU > 1.0f )
				continue;

			VECTOR vClosest;
			vClosest.fX = vPosition.fX + ( fU * vEdge.fX );
			vClosest.fY = vPosition.fY + ( fU * vEdge.fY );
			vClosest.fZ = vPosition.fZ + ( fU * vEdge.fZ );

			// check if that point is within the bounding box
			if ( !BoundingBoxTestPoint( &box, &vClosest ) )
			{
				continue;
			}
			
			selected_unit = test;
			fClosestSq = dist_sq;
		}
	}

	if ( !selected_unit )
	{
		return INVALID_ID;
	}

	return UnitGetId( selected_unit );
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if  !ISVERSION(SERVER_VERSION)
#ifdef DRB_3RD_PERSON

ALTITEMLIST * c_FindClosestItem(
	UNIT * unit )
{
	if ( c_CameraGetViewType() == VIEW_1STPERSON )
	{
		sgClosest.bInPickupRange = FALSE;
		sgClosest.idItem = INVALID_ID;
		float fClosest = PICKUP_RADIUS_SQ;
		sIterateNearbyItems( unit, sItemIterateClosest, (DWORD *)&fClosest, NULL );
	}
	else
	{
		GAME * game = UnitGetGame( unit );
		sgClosest.bInPickupRange = FALSE;
		sgClosest.idItem = sCursorSelectItem( game );
		if ( sgClosest.idItem != INVALID_ID )
		{
			UNIT * item = UnitGetById( game, sgClosest.idItem );
			float fDistanceSquared = VectorDistanceSquared( unit->vPosition, item->vPosition );
			if ( fDistanceSquared <= PICKUP_RADIUS_SQ )
			{
				sgClosest.bInPickupRange = TRUE;
			}
			else
			{
				sgClosest.idItem = INVALID_ID;
				sgClosest.bInPickupRange = FALSE;
			}
		}
	}
	return & sgClosest;
}

#else

ALTITEMLIST * c_FindClosestItem(
	UNIT * unit )
{
	//sgClosest.bInPickupRange = FALSE;
	//sgClosest.idItem = INVALID_ID;
	//float fClosest = PICKUP_RADIUS_SQ;
	//sIterateNearbyItems( unit, sItemIterateClosest, (DWORD *)&fClosest, NULL );
	return & sgClosest;
}

#endif

#endif // !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

ALTITEMLIST * c_GetCurrentClosestItem()
{
	return & sgClosest;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ITEM_LOOK_DATA* ItemLookGetData(
	GAME* game,
	int nClass,
	int nLookGroup)
{
	ITEM_LOOK_DATA key;
	key.nItemClass = nClass;
	key.nLookGroup = nLookGroup;
	return (ITEM_LOOK_DATA*)ExcelGetDataByKey(game, DATATABLE_ITEM_LOOKS, &key, sizeof(key));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetAppearanceDefId(
	GAME* game,
	int nClass,
	int nLookGroup)
{
	ASSERT_RETNULL(game);

	const UNIT_DATA* item_data = ItemGetData(nClass);
	ASSERT_RETNULL(item_data);

	int nAppearanceDefId = INVALID_ID;
	if (nLookGroup > 0)
	{
		ITEM_LOOK_DATA key;
		key.nItemClass = nClass;
		key.nLookGroup = nLookGroup;
		ITEM_LOOK_DATA* look_data = (ITEM_LOOK_DATA*)ExcelGetDataByKey(game, DATATABLE_ITEM_LOOKS, &key, sizeof(key));
		if (look_data)
		{
			if (IS_CLIENT(game))
			{
				if (look_data->nAppearanceId == INVALID_ID && look_data->szAppearance[0])
				{
					char folder[DEFAULT_FILE_WITH_PATH_SIZE];
					PStrPrintf(folder, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_ITEMS, look_data->szFileFolder);
					PStrForceBackslash( folder, DEFAULT_FILE_WITH_PATH_SIZE );

					look_data->nAppearanceId = AppearanceDefinitionLoad(game, look_data->szAppearance, folder);
					ASSERTX_RETFALSE(look_data->nAppearanceId != INVALID_ID, look_data->szAppearance);
				}
			}
			nAppearanceDefId = look_data->nAppearanceId;			
		}
	}
	if (nAppearanceDefId == INVALID_ID)
	{
		nAppearanceDefId = item_data->nAppearanceDefId;
	}

	return nAppearanceDefId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetWardrobe(
	GAME* game,
	int nClass,
	int nLookGroup)
{
	ASSERT_RETNULL(game);

	const UNIT_DATA* item_data = ItemGetData(nClass);
	ASSERT_RETNULL(item_data);

	int wardrobe = INVALID_LINK;
	if (nLookGroup > 0)
	{
		ITEM_LOOK_DATA key;
		key.nItemClass = nClass;
		key.nLookGroup = nLookGroup;
		ITEM_LOOK_DATA* look_data = (ITEM_LOOK_DATA*)ExcelGetDataByKey(game, DATATABLE_ITEM_LOOKS, &key, sizeof(key));
		if (look_data)
		{
			wardrobe = look_data->nWardrobe;			
		}
	}
	if (wardrobe == INVALID_LINK)
	{
		wardrobe = item_data->nInventoryWardrobeLayer;
	}

	return wardrobe;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetByUnitType(
	GAME* pGame,
	int nUnitType)
{
	// yeah, it's slow.
	int nClassCount = ExcelGetNumRows( pGame, DATATABLE_ITEMS );
	for ( int i = 0; i < nClassCount; i++ )
	{
		const UNIT_DATA * pItemData = ItemGetData( i );
		if (! pItemData )
			continue;
		if ( UnitIsA( pItemData->nUnitType, nUnitType ) )
			return i;
	}
	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetPrimarySkill(
	UNIT * item)
{
	if (!item) 
	{
		return INVALID_ID;
	}
	const UNIT_DATA * item_data = ItemGetData(item);
	if( AppIsTugboat() && item_data )
	{
		if( item_data->nStartingSkills[1] != INVALID_ID )
		{
			UNIT *pOwner = UnitGetUltimateOwner( item );
			GAME *pGame = UnitGetGame( item );
			for( int t = 0; t < NUM_UNIT_START_SKILLS; t++ )
			{
				if( item_data->nStartingSkills[ t ] == INVALID_ID)
					return INVALID_ID;
				if( SkillCanStart( pGame, pOwner, item_data->nStartingSkills[ t ], UnitGetId( item ), NULL, FALSE, FALSE, 0 ) )
				{
					return item_data->nStartingSkills[ t ];
				}
			}
		}

	}
	return item_data ? item_data->nStartingSkills[0] : INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSetLookGroup(
	UNIT *pItem, 
	int nItemLookGroup)
{
	ASSERTX_RETURN( pItem, "Expected item unit" );
	UnitSetStat( pItem, STATS_ITEM_LOOK_GROUP, nItemLookGroup );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemModIsAllowedOn(
	UNIT* pMod,
	UNIT* pItem)
{
	int nModLocation, x, y;
	return ItemGetEquipLocation( pItem, pMod, &nModLocation, &x, &y );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemModAdd(
	UNIT* pItem,
	UNIT* pMod,
	BOOL bSendToClients)
{
	// add mod to item
	BOOL bModAdded = FALSE;
	int nModLocation, x, y;
	BOOL bLocOpen = ItemGetEquipLocation( pItem, pMod, &nModLocation, &x, &y );
	if (bLocOpen)
	{
		bModAdded = UnitInventoryAdd(INV_CMD_PICKUP, pItem, pMod, nModLocation, x, y );
	}
	
	// send to clients who care
	if (bModAdded == TRUE && bSendToClients == TRUE)
	{
		GAME *pGame = UnitGetGame( pItem );
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			if (ClientCanKnowUnit( pClient, pItem ))
			{
				s_SendUnitToClient( pMod, pClient );
			}
		}
		WardrobeUpdateFromInventory( pItem );
	}
	
	return bModAdded;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_ItemModGenerateAll(
	UNIT* pItem,
	UNIT* pSpawner,
	int nItemLookGroup,
	PFN_ADD_MOD_CALLBACK pfnCallback,
	void *pCallbackData)
{		
	GAME* pGame = UnitGetGame( pItem );
	ASSERTX_RETFALSE( ItemGetModSlots( pItem ) > 0, "Item has no mod slots" );
	
	UNIT_ITERATE_STATS_RANGE( pItem, STATS_ITEM_SLOTS, tStatsEntry, nStat, 16)
	{
		int nInvLocation = tStatsEntry[ nStat ].GetParam();
		int nNumSlots = tStatsEntry[ nStat ].value;

		// start the picker
		PickerStart( pGame, picker );
		
		INVLOC_HEADER tLocInfo;
		if (!UnitInventoryGetLocInfo( pItem, nInvLocation, &tLocInfo))
		{
			continue;
		}

		// figure out all the mods that could go in this inventory location		
		int nNumItems = ExcelGetNumRows( pGame, DATATABLE_ITEMS);
		for (int i = 0; i < nNumItems; ++i)
		{
			const UNIT_DATA* pModData = ItemModGetData( i );
			if ( !pModData )
				continue;
			if (!UnitIsA(pModData->nUnitType, UNITTYPE_MOD))
			{
				continue;
			}
			if (IsAllowedUnit( pModData->nUnitType, tLocInfo.nAllowTypes, INVLOC_UNITTYPES ))
			{
				WORD wCode = pModData->wCode;
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, ItemGetClassByCode( pGame, wCode ), 1));
			}
		}
		
		// create mods in all the free slots
		int nNumUsedSlots = UnitInventoryGetCount( pItem, nInvLocation );
		int nNumFreeSlots = nNumSlots - nNumUsedSlots;
		for (int nSlot = 0; nSlot < nNumFreeSlots; ++nSlot)
		{
		
			// pick a mod from the available pool
			int nModClass = PickerChoose( pGame );
			
			// spawn the mod
			ITEM_SPEC tModSpec;
			tModSpec.nItemClass = nModClass;
			tModSpec.pSpawner = pSpawner;
			tModSpec.nItemLookGroup = nItemLookGroup;
			UNIT* pMod = s_ItemSpawn( pGame, tModSpec, NULL );

			// add mod to item
			BOOL bModAdded = s_ItemModAdd( pItem, pMod, FALSE );
			if (bModAdded)
			{
				int nModInvLocation = INVALID_LINK;
				if( UnitGetInventoryLocation( pMod, &nModInvLocation ))
				{
					if (nModInvLocation != nInvLocation)
					{
						ASSERTX( 0, "Mod not added at expected inventory location" );
						UnitInventoryRemove( pMod, ITEM_ALL );
						bModAdded = FALSE;
					}
				}
				else
				{
					ASSERTX( 0, "Cannnot get inventory location of mod added to item" );
					UnitInventoryRemove( pMod, ITEM_ALL );
					bModAdded = FALSE;
					
				}
				
			}
						
			// free item if not added
			if (bModAdded == FALSE)
			{
				UnitFree( pMod, UFF_SEND_TO_CLIENTS );
				ConsoleString( CONSOLE_ERROR_COLOR, L"Attach mod to item" );
				return FALSE;
			}			
			else if (pfnCallback)
			{
				pfnCallback( pMod, pCallbackData );
			}
					
		}
		
	}
	UNIT_ITERATE_STATS_END;
	WardrobeUpdateFromInventory( pItem );
	return TRUE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_ItemModDestroyAllEngineered(
	UNIT *pItem)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	
	// iterate inventory
	UNIT *pContained = UnitInventoryIterate( pItem, NULL, 0 );
	while (pContained)
	{
		UNIT *pNext = UnitInventoryIterate( pItem, pContained, 0 );
		if (UnitGetStat( pContained, STATS_ENGINEERED ) == TRUE)
		{
			UnitFree( pContained, UFF_SEND_TO_CLIENTS );
		}
		pContained = pNext;
	}
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sItemNameModifyByQuantity(
	int nQuantity,
	WCHAR *puszName,
	int nBufferSize)
{		
	const WCHAR *puszItemToken = StringReplacementTokensGet( SR_ITEM );
	const WCHAR *puszNumberToken = StringReplacementTokensGet( SR_NUMBER );

	// get quantity as string
	WCHAR uszQuantity[ MAX_ITEM_NAME_STRING_LENGTH ];
	LanguageFormatIntString( uszQuantity, MAX_ITEM_NAME_STRING_LENGTH, nQuantity );
	
	// print stacking format
	WCHAR szTemp[ MAX_ITEM_NAME_STRING_LENGTH ];		
	PStrCopy( szTemp, GlobalStringGet( GS_ITEM_STACK_FORMAT ), MAX_ITEM_NAME_STRING_LENGTH );
	PStrReplaceToken( szTemp, MAX_ITEM_NAME_STRING_LENGTH, puszItemToken, puszName );
	PStrReplaceToken( szTemp, MAX_ITEM_NAME_STRING_LENGTH, puszNumberToken, uszQuantity );

	// copy to working name				
	PStrCopy( puszName, szTemp, nBufferSize );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemGetNameInfo(
	ITEM_NAME_INFO &tItemNameInfo,
	int nItemClass,
	int nQuantity,
	DWORD dwFlags)
{
	const UNIT_DATA *pItemData = ItemGetData( nItemClass );
	
	// get the attributes to match for number
	DWORD dwAttributesToMatch = StringTableGetGrammarNumberAttribute( nQuantity > 1 ? PLURAL : SINGULAR );

	// are we to match the possessive form
	if (TESTBIT( dwFlags, INIF_POSSIVE_FORM ))
	{
		int nPossessiveAttributeBit = StringTableGetAttributeBit( ATTRIBUTE_POSSESSIVE );		
		SETBIT( dwAttributesToMatch, nPossessiveAttributeBit );
	}

#if !ISVERSION( SERVER_VERSION )
	// get player
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit( pGame );
		if (pPlayer && UnitHasBadge(pPlayer, ACCT_STATUS_UNDERAGE) )	//UNDER_18
		{
			dwAttributesToMatch |= StringTableGetUnder18Attribute();
		}
	}
#endif

	
	// get string
	const WCHAR *puszString = StringTableGetStringByIndexVerbose( 
		pItemData->nString, 
		dwAttributesToMatch,
		0,
		&tItemNameInfo.dwStringAttributes,
		NULL);

	// copy to struct
	PStrCopy( tItemNameInfo.uszName, puszString, MAX_ITEM_NAME_STRING_LENGTH );

	// modify by quantity if more than one and we want to
	if( TESTBIT( dwFlags, INIF_MODIFY_NAME_BY_ANY_QUANTITY ))
	{
		sItemNameModifyByQuantity( nQuantity, tItemNameInfo.uszName, MAX_ITEM_NAME_STRING_LENGTH );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemGetNameInfo(
	ITEM_NAME_INFO &tItemNameInfo,
	UNIT *pItem,
	int nQuantity,
	DWORD dwFlags)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	int nItemClass = UnitGetClass( pItem );
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	
	// see if we're supposed to get the item name from the base row instead of this row
	if (ItemIsUnidentified( pItem ) && UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_UNIDENTIFIED_NAME) && pItemData->nUnitDataBaseRow != INVALID_LINK)
	{
		nItemClass = pItemData->nUnitDataBaseRow;
	}
	
	// get name
	ItemGetNameInfo( tItemNameInfo, nItemClass, nQuantity, dwFlags );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_ItemIdentify(
	UNIT *pOwner,
	UNIT *pAnalyzer,
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pOwner, "Expected owner unit" );
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	ASSERTX_RETFALSE( UnitGetUltimateOwner( pItem ) == pOwner, "Expected item owned by owner param" );
#if ISVERSION(SERVER_VERSION)	// this is because we don't check the message from client earlier, and it's being sent twice.. generally a legitimate thing to happen
	IF_NOT_RETFALSE(ItemIsUnidentified(pItem));
#else
	ASSERTX_RETFALSE( ItemIsUnidentified( pItem ), "Expected unidientified item" );
#endif
	
	// server only
	GAME *pGame = UnitGetGame( pItem );
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	
	if (ItemBelongsToAnyMerchant(pItem))
	{
		return FALSE;
	}

	// turn all the attached affixes into actual applied affixes
	s_AffixApplyAttached( pItem );
		
	// use up the analyzer ... we don't have these all the time when items are created
	// and automatically identify themselves ... or when we use a cheat etc.
	if (pAnalyzer)
	{
		ASSERTX_RETFALSE( UnitIsA( pAnalyzer, UNITTYPE_ANALYZER ), "Expected analyzer unit type" );
		ASSERTX_RETFALSE(pAnalyzer != pItem && pAnalyzer != pOwner, "You can't analyze something with itself or yourself!");
		ASSERTX_RETFALSE(UnitGetUltimateOwner(pAnalyzer) == pOwner, "Expected analyzer owned by owner");
		sItemDecrementOrUse( pGame, pAnalyzer, FALSE );
	}

	// make it identified
	UnitSetStat( pItem, STATS_IDENTIFIED, TRUE );

	// inform the achievements system
	s_AchievementsSendItemIdentify(pItem, pOwner);

	// resed item to clients
	s_ItemResend( pItem );

	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
int s_ItemIdentifyAll( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected player unit" );
	int nIdentifiedCount = 0;
	
	// go through all inventory items on the player
	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	
	UNIT * pItem = NULL;	
	while ((pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags)) != NULL)
	{
		if (ItemIsUnidentified( pItem ))
		{
			s_ItemIdentify( pPlayer, NULL, pItem );
			nIdentifiedCount++;
		}
	}

	// tell the client some identification was done
	s_ConsoleMessage( pPlayer, GS_INVENTORY_IDENTIFIED );
		
	return nIdentifiedCount;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemResend(
	UNIT *pItem)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	GAME *pGame = UnitGetGame( pItem );
	ASSERTX_RETURN( pGame, "Expected game" );
	
	// iterate clients
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
	
		// send a remove message for everybody that knows the inventory of the owner
		if (ClientCanKnowUnit( pClient, pItem ))
		{
		
			// preserve cached status
			BOOL bCached = UnitIsCachedOnClient( pClient, pItem );
			
			// clear cached status
			if (bCached == TRUE)
			{
				ClientSetKnownFlag( pClient, pItem, UKF_CACHED, FALSE );
			}
			
			// remove unit
			s_RemoveUnitFromClient( pItem, pClient, 0 );		
			
			// resend unit
			DWORD dwKnownFlags = 0;
			SETBIT( dwKnownFlags, UKF_CACHED, bCached );
			s_SendUnitToClient(pItem, pClient, dwKnownFlags);
			
		}
		
	}
#endif
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemEnterMode(
	ITEM_MODE eItemMode,
	UNIT *pSource)
{
	UNITID idCursorUseUnit = INVALID_ID;
	CURSOR_STATE eCursorState = UICURSOR_STATE_INVALID;

	// get player
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected unit" );	
	
	// client only
	ASSERTX_RETURN( IS_CLIENT( pPlayer ), "Client only" );
	
	switch (eItemMode)
	{
		//----------------------------------------------------------------------------
		case ITEM_MODE_IDENTIFY:
		{
			ASSERTX_RETURN( pSource, "Expected source unit" );
			ASSERTX_RETURN( UnitIsA( pSource, UNITTYPE_ANALYZER ), "Expected analyzer unit" );
			idCursorUseUnit = UnitGetId( pSource );
			eCursorState = UICURSOR_STATE_IDENTIFY;
			break;
		}
		case ITEM_SKILL_SELECT:
		{
			ASSERTX_RETURN( pSource, "Expected source unit" );
			ASSERTX_RETURN( UnitIsA( pSource, UNITTYPE_SKILL_SELECTOR ), "Expected analyzer unit" );
			idCursorUseUnit = UnitGetId( pSource );
			eCursorState = UICURSOR_STATE_ITEM_PICK_ITEM;
			break;
		}
		//----------------------------------------------------------------------------
		case ITEM_MODE_DISMANTLE:
		{
			eCursorState = UICURSOR_STATE_DISMANTLE;
			break;
		}
		
		//----------------------------------------------------------------------------
		case ITEM_MODE_CREATE_GUILD:
		{
			UNITLOG_ASSERT_RETURN( PlayerIsSubscriber(pPlayer), pPlayer );
			ASSERTX_RETURN( pSource, "Expected source unit" );
			ASSERTX_RETURN( UnitIsA( pSource, UNITTYPE_GUILD_HERALD ), "Expected guild herald unit" );
			ASSERTX_RETURN( AppIsMultiplayer(), "Attempting to use a Guild Herald in a single-player game" );

			if (UnitGetStat(pSource, STATS_GUILD_HERALD_IN_USE))
			{
				UIShowGenericDialog(GlobalStringGet(GS_GUILD_CREATION_FAILURE_HEADER), GlobalStringGet(GS_GUILD_CREATION_FAILURE_UNKNOWN));
				return;
			}

			const WCHAR * szGuildName = c_PlayerGetGuildName();
			if (szGuildName && szGuildName[0])
			{
				UIShowGenericDialog(GlobalStringGet(GS_GUILD_CREATION_FAILURE_HEADER), GlobalStringGet(GS_GUILD_CREATION_FAILURE_ALREADY_IN_A_GUILD));
				return;
			}

			idCursorUseUnit = UnitGetId( pSource );
			eCursorState = UICURSOR_STATE_POINTER;

			UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GUILD_CREATE_DIALOG);
			ASSERT_RETURN(pDialog);
			UIComponentActivate(pDialog, TRUE);
			break;
		}
	}
	
	// set the identify cursor
//	UISetCursorUnit( idCursorUseUnit, TRUE );
	UISetCursorUseUnit( idCursorUseUnit  );
	UISetCursorState( eCursorState );
	
}
#endif

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemPickItemExecuteSkill(
	UNIT *pItem,
	UNIT *pSource)
{
	UNIT *pPlayer = UIGetControlUnit();
	GAME *pGame = UnitGetGame( pPlayer  );
	ASSERTX_RETURN( pGame && IS_CLIENT( pGame ), "Expected client" );
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	
	if ( pItem &&
		 pSource )
	{
		if (UnitGetUltimateOwner( pItem ) == pPlayer && !ItemIsUnidentified( pItem ) && ItemIsUsable( pPlayer, pSource, pItem, ITEM_USEABLE_MASK_ITEM_TEST_TARGETITEM ) == USE_RESULT_OK )
		{
		
			// Ok, we've passed all the checks required for the client, 
			// send the message to the server so it can do its thing (above)

			MSG_CCMD_ITEM_PICK_ITEM msg;
			msg.idItem = UnitGetId(pSource);
			msg.idItemPicked = UnitGetId(pItem);			
			c_SendMessage(CCMD_ITEM_PICK_ITEM, &msg);

			int nAnalyzeSound = GlobalIndexGet( GI_SOUND_ANALYZE_ITEM );	
			if (nAnalyzeSound != INVALID_LINK)
			{
				c_SoundPlay( nAnalyzeSound, &c_UnitGetPosition( pPlayer ), NULL );
			}

		}
		else
		{
			UIPlayCantUseSound( pGame, pPlayer, pItem );
		}

	}

	// clear cursor states
	UISetCursorUnit( INVALID_ID, TRUE );
	UISetCursorUseUnit( INVALID_ID );
	UISetCursorState( UICURSOR_STATE_POINTER );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemTryIdentify(
	UNIT *pItem, 
	BOOL bUsingAnalyzer)
{
	UNIT *pPlayer = UIGetControlUnit();
	GAME *pGame = UnitGetGame( pPlayer  );
	ASSERTX_RETURN( pGame && IS_CLIENT( pGame ), "Expected client" );
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	
	if (pItem)
	{
		if (UnitGetUltimateOwner( pItem ) == pPlayer && ItemIsUnidentified( pItem ) && !ItemBelongsToAnyMerchant(pItem))
		{
		
			// get the analyzer that is identifying the item ... we don't have one
			// when using the radial menu
			UNIT *pAnalyzer = UnitGetById( pGame, UIGetCursorUseUnit() );
			if (bUsingAnalyzer)
			{
				ASSERTX_RETURN( pAnalyzer, "No unit in player cursor on client" );
				ASSERTX_RETURN( UnitIsA( pAnalyzer, UNITTYPE_ANALYZER ), "Expected analyzer unit" );
			}
			
			// Ok, we've passed all the checks required for the client, 
			// send the message to the server so it can do its thing (above)

			MSG_CCMD_TRY_IDENTIFY msg;
			msg.idItem = UnitGetId(pItem);
			msg.idAnalyzer = pAnalyzer ? UnitGetId( pAnalyzer ) : INVALID_ID;
			c_SendMessage(CCMD_TRY_IDENTIFY, &msg);

			int nAnalyzeSound = GlobalIndexGet( GI_SOUND_ANALYZE_ITEM );	
			if (nAnalyzeSound != INVALID_LINK)
			{
				c_SoundPlay( nAnalyzeSound, &c_UnitGetPosition( pPlayer ), NULL );
			}

		}
		else
		{
			UIPlayCantUseSound( pGame, pPlayer, pItem );
		}

	}

	// clear cursor states
	UISetCursorUnit( INVALID_ID, TRUE );
	UISetCursorUseUnit( INVALID_ID );
	UISetCursorState( UICURSOR_STATE_POINTER );
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddScrapToLookup(
	int nItemClassScrap,
	int nItemQualityScrap,
	SCRAP_LOOKUP *pBuffer,
	int nBufferSize,
	int *pnCurrentBufferCount,
	BOOL bExtraResource)
{

	// don't add scrap with no quality at all
	if (nItemQualityScrap == INVALID_LINK)
	{
		return;
	}
	
	BOOL bFound = FALSE;						
	for (int j = 0; j < *pnCurrentBufferCount; ++j)
	{
		SCRAP_LOOKUP *pScrapLookup = &pBuffer[ j ];
		if (pScrapLookup->nItemClass == nItemClassScrap &&
			pScrapLookup->nItemQuality == nItemQualityScrap)
		{
			bFound = TRUE;
			break;
		}
	}

	// add to buffer
	if (bFound == FALSE)
	{

		// verify room in buffer
		ASSERTX_RETURN( *pnCurrentBufferCount < nBufferSize - 1, "Buffer too small for scrap item types" );

		// add to buffer
		SCRAP_LOOKUP *pScrapLookup = &pBuffer[ *pnCurrentBufferCount ];
		*pnCurrentBufferCount = *pnCurrentBufferCount + 1;
		pScrapLookup->nItemClass = nItemClassScrap;
		pScrapLookup->nItemQuality = nItemQualityScrap;
		pScrapLookup->bExtraResource = bExtraResource;
		
		// also add default scrap quality if not already there too
		if (!bExtraResource)
		{
			const ITEM_QUALITY_DATA *pItemQualityDataScrap = ItemQualityGetData( nItemQualityScrap );
			ASSERTX_RETURN( pItemQualityDataScrap, "Expected item quality data for scrap" );
			if (pItemQualityDataScrap->nScrapQualityDefault != nItemQualityScrap)
			{
				sAddScrapToLookup( nItemClassScrap, pItemQualityDataScrap->nScrapQualityDefault, pBuffer, nBufferSize, pnCurrentBufferCount, bExtraResource );
			}
		}

	}
	
}	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemGetScrapComponents( 
	UNIT *pItem, 
	SCRAP_LOOKUP *pBuffer,
	int nBufferSize,
	int *pnCurrentBufferCount,
	BOOL bForUpgrade)
{
	ASSERTX_RETURN( pItem, "Expected item unit" );
	ASSERTX_RETURN( pBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize > 0, "Invalid buffer size" )
	ASSERTX_RETURN( pnCurrentBufferCount, "Expected current buffer count" );
	const UNIT_DATA *pItemData = ItemGetData( pItem );

	// what item quality of scrap does this item produce
	int nItemQualityScrap = ItemGetScrapQuality( pItem );
	
	// add generic scrap to the lookup
	sAddScrapToLookup( GlobalIndexGet( GI_SCRAP ), nItemQualityScrap, pBuffer, nBufferSize, pnCurrentBufferCount, FALSE );
		
	// go through the character class datatable
	int nNumCharClasses = ExcelGetNumRows( NULL, DATATABLE_CHARACTER_CLASS );
	for (int i = 0; i < nNumCharClasses; ++i)
	{
		const CHARACTER_CLASS_DATA *pCharClassData = CharacterClassGetData( i );
		if(!pCharClassData)
			continue;

		int nItemClassScrapSpecial = pCharClassData->nItemClassScrapSpecial;

		// go through each unit type for this class class
		for (int nSex = 0; nSex < GENDER_NUM_CHARACTER_GENDERS; ++nSex)
		{
			const CHARACTER_CLASS *pCharacterClass = &pCharClassData->tClasses[ nSex ];

			// go through all the races
			for (int nRace = 0; nRace < MAX_CHARACTER_RACES; ++nRace)
			{
				int nPlayerClass = pCharacterClass->nPlayerClass[ nRace ];

				if (nPlayerClass != INVALID_LINK)
				{
					const UNIT_DATA *pPlayerData = PlayerGetData( NULL, nPlayerClass );
					if (IsAllowedUnit( pPlayerData->nUnitType, pItemData->nContainerUnitTypes, NUM_CONTAINER_UNITTYPES ))
					{

						// add to buffer if it's not in it already
						sAddScrapToLookup( nItemClassScrapSpecial, nItemQualityScrap, pBuffer, nBufferSize, pnCurrentBufferCount, FALSE );

					}

				}

			}

		}

	}

	// see if we need to give extra scrap
	int nItemQuality = ItemGetQuality(pItem);
	const ITEM_QUALITY_DATA *pQualityData = ItemQualityGetData( nItemQuality );
	ASSERT_RETURN(pQualityData);

	if (pQualityData->nExtraScrapItem != INVALID_LINK)
	{
		GAME *pGame = UnitGetGame(pItem);
		if (bForUpgrade ||															// if we're getting requirements for an upgrade, always add one nanoshard
			(pQualityData->nExtraScrapChancePct > 0 &&								//   otherwise if we're disassembling an item check for the chance to
			(int)RandGetNum( pGame, 0, 99 ) < pQualityData->nExtraScrapChancePct))		//   return a nanoshard
		{
			sAddScrapToLookup( pQualityData->nExtraScrapItem, pQualityData->nExtraScrapQuality, pBuffer, nBufferSize, pnCurrentBufferCount, TRUE );
		}
	}


}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
inline int sItemDismentleGetClassID( const UNIT *pItem )
{
	if( !pItem )
		return INVALID_ID;
	return UnitGetClass( pItem );

}
inline void s_sItemDismantleTugboat( UNIT *pOwner,
									  UNIT *pItemToDismantle )
{
	ASSERT_RETURN( CRAFTING::CraftingItemCanBeBrokenDown( pItemToDismantle, pOwner ) );		
	int nItemQuality = ItemGetQuality( pItemToDismantle );
	nItemQuality = ( nItemQuality == INVALID_ID )?1:nItemQuality;
	const ITEM_QUALITY_DATA * quality_data = ItemQualityGetData( nItemQuality );
	ASSERT_RETURN( quality_data );
	ASSERT_RETURN( quality_data->nBreakDownTreasure );
	UNIT* pItems[ MAX_OFFERINGS ];
	for( int t = 0; t < MAX_OFFERINGS; t++ )
		pItems[ t ] = NULL;
	ITEMSPAWN_RESULT tOfferItems;
	tOfferItems.pTreasureBuffer = pItems;
	tOfferItems.nTreasureBufferSize = arrsize( pItems);
	ITEM_SPEC tItemSpec;	
	SETBIT(tItemSpec.dwFlags, ISF_PICKUP_BIT);	

	s_TreasureSpawnClass( pOwner, quality_data->nBreakDownTreasure, &tItemSpec, &tOfferItems, 0, INVALID_ID, UnitGetData( pItemToDismantle )->nItemExperienceLevel );
	MSG_SCMD_CRAFT_BREAKDOWN tMsg;
	tMsg.classID0 = sItemDismentleGetClassID( tOfferItems.pTreasureBuffer[ 0 ] );
	tMsg.classID1 = sItemDismentleGetClassID( tOfferItems.pTreasureBuffer[ 1 ] );
	tMsg.classID2 = sItemDismentleGetClassID( tOfferItems.pTreasureBuffer[ 2 ] );
	tMsg.classID3 = sItemDismentleGetClassID( tOfferItems.pTreasureBuffer[ 3 ] );
	tMsg.classID4 = sItemDismentleGetClassID( tOfferItems.pTreasureBuffer[ 4 ] );

	GAMECLIENTID idClient = UnitGetClientId( pOwner );
	s_SendUnitMessageToClient(pOwner, idClient, SCMD_CRAFT_BREAKDOWN, &tMsg);

	// inform the achievements system
	s_AchievementsSendItemDismantle(pItemToDismantle, pOwner);
	// destroy the item we're dismantling
	UnitFree( pItemToDismantle, UFF_SEND_TO_CLIENTS, UFC_ITEM_DISMANTLE );
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_ItemDismantle(
	UNIT *pOwner,
	UNIT *pItemToDismantle,
	UNIT *)//pDismantler) //pDismantler is currently unused.
{
	ASSERTX_RETURN( pOwner, "Expected owner unit" );
	ASSERTX_RETURN( pItemToDismantle, "Expected item" );

	if( AppIsTugboat() )
	{
		//tugboat does a whole other scheme for Dismantling
		s_sItemDismantleTugboat( pOwner, pItemToDismantle );
		return;
	}

	ASSERTX_RETURN( ItemCanBeDismantled( pOwner, pItemToDismantle ), "Expected item to be able to be dismantled" );
	GAME *pGame = UnitGetGame( pOwner );

	// how much space does this item take up in your inventory
	int nInvHeight = 0;
	int nInvWidth = 0;
	UnitGetSizeInGrid( pItemToDismantle, &nInvHeight, &nInvWidth, NULL );
	ASSERTX_RETURN( nInvHeight > 0 && nInvWidth > 0, "Expected size in inventory for item to dismantle" );
		
	// setup buffer for special scrap pieces this item can create
	SCRAP_LOOKUP tScrapLookup[ MAX_SCRAP_PIECES ];
	//Initialize scrap lookup so we're not dereferencing null scraps later.
	memclear(tScrapLookup, sizeof(tScrapLookup)); 

	int nNumScrapTypes = 0;	
		
	// get the scrap pieces this item is capable of making
	ItemGetScrapComponents( pItemToDismantle, tScrapLookup, MAX_SCRAP_PIECES, &nNumScrapTypes, FALSE );
	
	int nDismantleSound = INVALID_LINK;
	int nHighestQuality = INVALID_LINK;

	// create all the scrap pieces
	if (nNumScrapTypes > 0)
	{
	
		// create unit for each scrap piece
		for (int i = 0; i < nNumScrapTypes; ++i)
		{
			SCRAP_LOOKUP *pScrapLookup = &tScrapLookup[ i ];
			ASSERTX_BREAK( pScrapLookup->nItemClass != INVALID_LINK, "Expected item class for scrap" );
			
			// fill out item spec
			ITEM_SPEC tSpec;
			tSpec.nQuality = pScrapLookup->nItemQuality;
			tSpec.nLevel = 1;
			tSpec.nItemClass = pScrapLookup->nItemClass;
			tSpec.pSpawner = pOwner;
			SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
			tSpec.guidRestrictedTo = UnitGetGuid( pOwner );  // restrict it to this owner
			UNIT *pScrap = s_ItemSpawn( pGame, tSpec, NULL );
			ASSERTX_BREAK( pScrap, "Expected scrap unit" );
			pScrapLookup->pScrap = pScrap;
			cCurrency currency = EconomyItemSellPrice( pScrap );
			pScrapLookup->nItemSellPrice = currency.GetValue( KCURRENCY_VALUE_INGAME );// ItemGetSellPrice( pScrap );
			pScrapLookup->bGiven = FALSE;
		
			// get the sound of the highest-quality scrap
			//		this might not be the best check, because the qualities aren't necessarily guaranteed to be in order from low to high
			//		for example all the mod qualities come later, but they're not used for scrap quality so this will work for now
			if (pScrapLookup->nItemQuality > nHighestQuality)
			{
				const ITEM_QUALITY_DATA *pScrapQuality = ItemQualityGetData(pGame, pScrapLookup->nItemQuality);
				nDismantleSound = pScrapQuality->nDismantleResultSound;
				nHighestQuality = pScrapLookup->nItemQuality;
			}
		}

		// get item sell price
		cCurrency currency = EconomyItemSellPrice( pItemToDismantle );
		int nItemSellPrice = currency.GetValue( KCURRENCY_VALUE_INGAME ); //ItemGetSellPrice( pItemToDismantle );

		// how much money value will we create of scrap
		int nScrapValue = MAX( 1, (int)(nItemSellPrice * SCRAP_VALUE_PERCENT_OF_ITEM_SELL_PRICE) );
		
		// add scrap until we get to the target scrap value
		int nValueGiven = 0;
		BOUNDED_WHILE(nValueGiven <= nScrapValue, 100)
		{	
			// select a piece of scrap at random
			int nPick = RandGetNum( pGame, 0, nNumScrapTypes - 1 );
			SCRAP_LOOKUP *pScrapLookup = &tScrapLookup[ nPick ];
		
			if (pScrapLookup->bExtraResource)
			{
				continue;	// always give these
			}
			// add this scrap value to the value given
			ASSERTX_BREAK(pScrapLookup->nItemSellPrice > 0, "Got zero value scrap from item dismantle.  Not going to give any more scraps, then")
			//Zero value items are a recipe for disaster, server crashes.

			if(nValueGiven + pScrapLookup->nItemSellPrice > nScrapValue)
			{	//Sorry guys, no more profiting from dismantling items.  We won't give you over the value.
				break;
			}
			nValueGiven += pScrapLookup->nItemSellPrice;

			// if this is not the first time we've picked this scrap, we increment the scrap quantity
			if (pScrapLookup->bGiven == TRUE)
			{
				int nQuantity = ItemGetQuantity( pScrapLookup->pScrap );
				ItemSetQuantity( pScrapLookup->pScrap, nQuantity + 1 );
			}
			
			// we will now give this scrap item to the player in a sec
			pScrapLookup->bGiven = TRUE;
							
		}

		// move scrap items we selected to array
		const int MAX_ITEMS = 16;
		UNIT *pScrapItems[ MAX_ITEMS ];
		int nNumScrapUnitsToGive = 0;
		for (int i = 0; i < nNumScrapTypes; ++i)
		{
			SCRAP_LOOKUP *pScrapLookup = &tScrapLookup[ i ];
			ASSERTX_CONTINUE( pScrapLookup->pScrap, "Expected scrap unit" );
			if (pScrapLookup->bGiven || pScrapLookup->bExtraResource)
			{
				pScrapItems[ nNumScrapUnitsToGive++ ] = pScrapLookup->pScrap;
			}
			else
			{
				// free units we didn't pick
				UnitFree( pScrapLookup->pScrap );
				pScrapLookup->pScrap = NULL;
			}
		}

		// remember where the item to dismantle was in the owner inventory
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation( pItemToDismantle, &tInvLoc );

		// remove the item to be scrapped from their inventory so we can make room 
		// for all of the scrap items if necessary
		pItemToDismantle = UnitInventoryRemove( pItemToDismantle, ITEM_ALL, 0, TRUE, INV_CONTEXT_DISMANTLE );
		
		// the owner must be able to pickup and hold all the scrap items
		BOOL bShouldDismantle = TRUE;
		if (nNumScrapUnitsToGive != 0 &&
			UnitInventoryCanPickupAll( pOwner, pScrapItems, nNumScrapUnitsToGive, TRUE ) == FALSE)
		{
			bShouldDismantle = FALSE;
		}

		if(nNumScrapUnitsToGive == 0)
		{
			//TODO: Give them worthless (s)crap.  So they at least get something
			//for the effort of destroying an almost-worthless item.

			//Stopgap: in lieu of an existing worthless (s)crap item, we'll give them 1 money.
			if(nItemSellPrice > 0 && bShouldDismantle)
			{
				UnitAddCurrency( pOwner, cCurrency(1,0) );
			}
			//When we remove the stopgap, we should move this above bShouldDismantle setting.
		}
		
		// put the item to be dismantled back in their inventory where it was before
		InventoryChangeLocation( pOwner, pItemToDismantle, tInvLoc.nInvLocation, tInvLoc.nX, tInvLoc.nY, CLF_ALLOW_NEW_CONTAINER_BIT, INV_CONTEXT_DISMANTLE );

		// destroy the scrap items or complete the dismantling process
		if (bShouldDismantle)
		{
			// inform the achievements system
			s_AchievementsSendItemDismantle(pItemToDismantle, pOwner);

			// destroy the item we're dismantling
			UnitFree( pItemToDismantle, UFF_SEND_TO_CLIENTS, UFC_ITEM_DISMANTLE );
				
			// go through all scrap items
			for (int i = 0; i < nNumScrapUnitsToGive; ++i)
			{
				UNIT *pScrap = pScrapItems[ i ];

				// this item is now ready to do network communication
				UnitSetCanSendMessages( pScrap, TRUE );					
			
				// put in world (yeah, kinda lame, but it keeps the pickup code path clean),
				// the unit was restricted to this pOwner GUID, so at least no other players
				// are getting new unit messages for it
				UnitWarp( 
					pScrap,
					UnitGetRoom( pOwner ), 
					UnitGetPosition( pOwner ),
					cgvYAxis,
					cgvZAxis,
					0);
				
				// pick it up
				s_ItemPickup( pOwner, pScrap );
																
			}

			// play a sound for the highest-rarity scrap that resulted from the dismantle
			if (nDismantleSound != INVALID_LINK)
			{
				MSG_SCMD_UNITPLAYSOUND msg;
				msg.id = UnitGetId(pOwner);
				msg.idSound = nDismantleSound;
				s_SendMessage(pGame, UnitGetClientId(pOwner), SCMD_UNITPLAYSOUND, &msg);
			}

		}
		else
		{

			// destroy all the scrap items
			for (int i = 0; i < nNumScrapUnitsToGive; ++i)
			{
				UnitFree( pScrapItems[ i ] );
			}
			
		}
	
	}
				
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
struct TRY_DISMANTLE_DATA
{
	UNITID idItem;
	BOOL bUsingDismantler;
	UI_POSITION tMousePosition;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sDoTryItemDismantle(
	UNIT *pItem,
	BOOL bUsingDismantler)
{

	// get the item we're dismantling with (if any)
	UNITID idItemDismantler = INVALID_ID;
	if (bUsingDismantler)
	{
		idItemDismantler = UIGetCursorUseUnit();
	}

	// tell server to try to dismantle item
	MSG_CCMD_TRY_DISMANTLE tMessage;
	tMessage.idItem = UnitGetId( pItem );
	tMessage.idItemDismantler = idItemDismantler;  
	c_SendMessage( CCMD_TRY_DISMANTLE, &tMessage );

}		
#endif

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDismantleConfirmComplete(
	const TRY_DISMANTLE_DATA *pTryDismantleData)
{
	// return the cursor back to where it was before
	const UI_POSITION *pPrevMousePosition = &pTryDismantleData->tMousePosition;
	InputSetMousePosition( pPrevMousePosition->m_fX, pPrevMousePosition->m_fY );
}
#endif

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientDismantleItemConfirmed(
	void *pCallbackData,
	DWORD dwCallbackData)
{
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETURN( pGame, "Expected game" );
	const TRY_DISMANTLE_DATA *pTryDismantleData = (const TRY_DISMANTLE_DATA *)pCallbackData;
		
	UNITID idItem = pTryDismantleData->idItem;
	UNIT *pItem = UnitGetById( pGame, idItem );
	if (pItem)
	{

		// do the item drop attempt
		sDoTryItemDismantle( pItem, pTryDismantleData->bUsingDismantler );

		// restore cursor to position with item
		sDismantleConfirmComplete( pTryDismantleData );
				
	}
	
}
#endif

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientDismantleItemCancelled(
	void *pCallbackData,
	DWORD dwCallbackData)
{
	const TRY_DISMANTLE_DATA *pTryDismantleData = (const TRY_DISMANTLE_DATA *)pCallbackData;
	sDismantleConfirmComplete( pTryDismantleData );
}
#endif

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemTryDismantle(
	UNIT *pItem,
	BOOL bUsingDismantler)
{

	// clear cursor states (if we had them set at all)
	UISetCursorUnit( INVALID_ID, TRUE );
	UISetCursorUseUnit( INVALID_ID );
	UISetCursorState( UICURSOR_STATE_POINTER );

	if( AppIsHellgate() )
	{
		// if there is an item, tell the server we want to dismantle it
		if (pItem)
		{
		
			GAME *pGame = AppGetCltGame();
			UNIT *pPlayer = GameGetControlUnit( pGame );
			ASSERTX_RETURN( pPlayer, "Expected control unit" );
			ASSERTX_RETURN( ItemCanBeDismantled( pPlayer, pItem ), "Item cannot be dismantled" );

			// save callback data (note this is in a static)
			static TRY_DISMANTLE_DATA tTryDismantleData;
			UIGetCursorPosition( &tTryDismantleData.tMousePosition.m_fX, &tTryDismantleData.tMousePosition.m_fY, FALSE );  // it'd be better if this was the radial menu open point
			tTryDismantleData.idItem = UnitGetId( pItem );
			tTryDismantleData.bUsingDismantler = bUsingDismantler;
			
			// setup callbacks
			DIALOG_CALLBACK tOKCallback;
			tOKCallback.pfnCallback = sClientDismantleItemConfirmed;
			tOKCallback.pCallbackData = &tTryDismantleData;  // OK since this is a static

			DIALOG_CALLBACK tCancelCallback;
			tCancelCallback.pfnCallback = sClientDismantleItemCancelled;
			tCancelCallback.pCallbackData = &tTryDismantleData;  // OK since this is a static
							
			// show confirmation dialog
			UIShowGenericDialog(
				GlobalStringGet( GS_CONFIRM_ITEM_DISMANTLE_HEADER ),
				GlobalStringGet( GS_CONFIRM_ITEM_DISMANTLE_MESSAGE ),
				TRUE,
				&tOKCallback,
				&tCancelCallback,
				GDF_CENTER_MOUSE_ON_OK);
					
		}
	}
	else
	{
		//Tugboat just removes the item. No need to confirm.
		sDoTryItemDismantle( pItem, FALSE );
	}
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemCanBeDismantled(
	UNIT *pOwner,
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETFALSE( pOwner, "Expected owner unit" );

	// the item must be contained by the owner
	if (UnitGetUltimateContainer( pItem ) != pOwner)
	{
		return FALSE;
	}

	// item must be in a location that the player an take the item out of
	if (ItemIsInCanTakeLocation( pItem ) == FALSE)
	{
		return FALSE;
	}
	
	// check for items that cannot be disabled themselves
	const UNIT_DATA *pItemData = ItemGetData( pItem );
	if (UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_CANNOT_BE_DISMANTLED))
	{
		return FALSE;
	}

	// check for mods in the item.  Don't dismantle if it has them.
	if (UnitInventoryIterate(pItem, NULL, 0, FALSE) != NULL)
	{
		return FALSE;
	}

	// items in equipable locations cannot be dismantled
	UNIT *pContainer = UnitGetContainer( pItem );
	if (ItemIsInEquipLocation( pContainer, pItem ))
	{
		return FALSE;
	}

	if (ItemIsInNoDismantleLocation( pContainer, pItem ))
	{
		return FALSE;
	}
	
	return TRUE;  // OK to dismantle item
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemRemoveUpdateHotkeys(
	GAME* game,
	UNIT *pPreviousUltimateOwner,
	UNIT* item)
{
	ASSERT_RETURN(IS_SERVER(game));
	if (!item || !pPreviousUltimateOwner)
	{
		return;
	}

	int nUnitType = UnitGetType(item);
	int nUnitClass = UnitGetClass(item);

	for (int ii = 0; ii <= TAG_SELECTOR_HOTKEY12; ii++)
	{
		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(pPreviousUltimateOwner, ii);
		if (!tag || tag->m_idItem[0] != UnitGetId(item))
		{
			continue;
		}
		tag->m_idItem[0] = INVALID_ID;

		// Don't do a get best item for skills
		if (UnitGetSkillID(item) == INVALID_ID)
		{
			const UNIT_DATA* item_data = UnitGetData(item);
			if (!item_data)
			{
				continue;
			}
			// If we've got a location from which to draw a replacement...
			if (item_data->m_nRefillHotkeyInvLoc != INVLOC_NONE &&		// need to change the default for this field to INVLOC_NONE
				item_data->m_nRefillHotkeyInvLoc != -1)
			{
				UNIT* newitem = InventoryGetReplacementItemFromLocation(pPreviousUltimateOwner, item_data->m_nRefillHotkeyInvLoc, nUnitType, nUnitClass);
				if (newitem)
				{
					tag->m_idItem[0] = UnitGetId(newitem);
				}
			}
		}

		GAMECLIENT *pClient = UnitGetClient( pPreviousUltimateOwner );
		ASSERTX_RETURN( pClient, "Item has no client id" );
		HotkeySend( pClient, pPreviousUltimateOwner, (TAG_SELECTOR)ii );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR OpenNodeGetNextWorldPosition( 
	GAME *pGame,
	OPEN_NODES *pOpenNodes)
{
	ASSERTX( pGame && pOpenNodes, "Invalid params" );
	ASSERTX( pOpenNodes->nNumPathNodes > 0, "Must have at least one node" );
	
	// pick the next node
	FREE_PATH_NODE *pFreePathNode = &pOpenNodes->pFreePathNodes[ pOpenNodes->nCurrentNode ];
	
	// increment current node (wrap around to beginning if at end)
	pOpenNodes->nCurrentNode++;
	if (pOpenNodes->nCurrentNode >= pOpenNodes->nNumPathNodes)
	{
		pOpenNodes->nCurrentNode = 0;
	}
	
	// get world position
	return RoomPathNodeGetWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL c_ItemTryPickup( 
	GAME *pGame,
	UNITID idItem)
{

	// client only
	ASSERTX_RETFALSE( IS_CLIENT( pGame ), "Client only" );
	
	// get item to pickup
	UNIT *pItem = UnitGetById( pGame, idItem );
	ASSERTX_RETFALSE( pItem, "Client cannot find item to pickup" );
	
	// get player
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	
	if (!c_QuestEventCanPickup(pPlayer, pItem))
	{
		// can't pickup
		c_ItemCannotPickup( idItem, PR_QUEST_ITEM_PROHIBITED );
		return FALSE;
	}

	if (AppIsHellgate())
	{
		if (UIGetCursorUnit() != INVALID_ID)	// can't pickup something if there's an item in your cursor
												// this is to try and prevent a situation where you have an item
												// in your cursor and your inventory is full so you have nowhere
												// to put it down.
		{
			UIClearCursor();						// try to drop the item in the cursor
			if (UIGetCursorUnit() != INVALID_ID)		// couldn't clear cursor
			{
				// can't pickup
				c_ItemCannotPickup( idItem, PR_FULL_ITEMS );
				return FALSE;
			}
		}
	}

	// check can pickup
	PICKUP_RESULT eResult = ItemCanPickup( pPlayer, pItem );
	if (eResult != PR_OK)
	{
		c_ItemCannotPickup( idItem, eResult );		
		return FALSE;
	}
	
	if (sItemCanTryToPickup( pPlayer, pItem ) == TRUE)
	{

		// setup message
		MSG_CCMD_PICKUP tMessage;
		tMessage.id = idItem;
		
		// send to server
		c_SendMessage( CCMD_PICKUP, &tMessage );
		return TRUE;
		
	}

	// can't pickup
	c_ItemCannotPickup( idItem, PR_FULL_ITEMS );
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemCannotUse(
	UNIT *pItem,
	USE_RESULT eReason)
{

	// display message	
	// for Tugboat, sounds are enough!
	if( AppIsHellgate() )
	{
		const WCHAR *puszString = c_ItemGetUseResultString( eReason );
		if (puszString)
		{
			ConsoleString( CONSOLE_SYSTEM_COLOR, puszString );	
		}
	}
		
	// play the sound
	GAME *pGame = UnitGetGame( pItem );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer)
	{
		c_UnitPlayCantUseSound( pGame, pPlayer, pItem );
		c_UnitPlayCantSound( pGame, pPlayer );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemCannotPickup(
	UNITID idItem,   // may be invalid_id!
	PICKUP_RESULT eReason)
{

	// display message	
	GLOBAL_STRING eString = GS_INVALID;
	switch (eReason)
	{
		case PR_FULL_ITEMS:				eString = GS_INVENTORY_FULL;			break;
		case PR_FULL_MONEY:				eString = GS_INVENTORY_FULL_MONEY;		break;
		case PR_LOCKED:					eString = GS_LOCKED_TO_OTHER;			break;
		case PR_QUEST_ITEM_PROHIBITED:	eString = GS_CANNOT_PICKUP_QUEST_ITEM;	break;
		case PR_MAX_ALLOWED:			eString = GS_CANNOT_PICKUP_MAX_ALLOWED;	break;
		default:						eString = GS_CANNOT_PICKUP;				break;
	}
	if (eString != GS_INVALID)
	{
		const WCHAR *puszString = GlobalStringGet( eString );
		//ConsoleString( CONSOLE_SYSTEM_COLOR, puszString );
		UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), puszString );
	}

	// play the sound
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer)
	{
		if (idItem != INVALID_ID)
		{
			UNIT *pItem = UnitGetById( pGame, idItem );
			if (pItem)
			{
				c_UnitPlayCantUseSound( pGame, pPlayer, pItem );
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemCannotDrop(
	UNITID idItem,
	DROP_RESULT eReason)
{
	
	GLOBAL_STRING eString = GS_INVALID;
	switch (eReason)
	{
		case DR_ACTIVE_OFFERING:	eString = GS_CANNOT_DROP_ACTIVE_OFFERING;	break;
		case DR_NO_DROP:			eString = GS_CANNOT_DROP_NO_DROP;			break;
		default:					eString = GS_CANNOT_DROP;					break;
	}
	if (eString != GS_INVALID)
	{
		const WCHAR *puszString = GlobalStringGet( eString );
		ConsoleString( CONSOLE_SYSTEM_COLOR, puszString );
	}

	// play the sound
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer)
	{
		if( AppIsHellgate() )
		{
			UNIT *pItem = UnitGetById( pGame, idItem );
			if (pItem)
			{
				c_UnitPlayCantUseSound( pGame, pPlayer, pItem );
			}
		}
		else
		{
			c_UnitPlayCantSound( pGame, pPlayer );
		}
		
	}
	
}
	
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemLockForPlayer( 
	UNIT *pItem, 
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( pPlayer, "Expected player" );	
	GAME *pGame = UnitGetGame( pPlayer );
	
	// set stats
	UnitSetStat( pItem, STATS_LOCKED_FOR_UNIT_ID, UnitGetId( pPlayer ) );
	UnitSetStat( pItem, STATS_LOCKED_FOR_UNIT_ON_TICK, GameGetTick( pGame ) );
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsLocked(
	UNIT *pItem)
{
	ASSERTX_RETTRUE( pItem, "Expected item" );
	if (UnitGetStat( pItem, STATS_LOCKED_FOR_UNIT_ID  ) != INVALID_ID)
	{
		GAME *pGame = UnitGetGame( pItem );
		DWORD dwLockedOnTick = UnitGetStat( pItem, STATS_LOCKED_FOR_UNIT_ON_TICK );
		if (GameGetTick( pGame ) - dwLockedOnTick <= LOCKED_ITEM_TIME_IN_TICKS)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsLockedForPlayer(
	UNIT *pItem,
	UNIT *pPlayer)
{
	ASSERTX_RETTRUE( pItem, "Expected item" );
	ASSERTX_RETTRUE( pPlayer, "Expected player" );	
	
	UNITID idLockedTo = UnitGetStat( pItem, STATS_LOCKED_FOR_UNIT_ID  );
	if (idLockedTo != INVALID_ID && idLockedTo == UnitGetId( pPlayer ))
	{
		return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetRank(
	UNIT *pItem)
{
	ASSERTX_RETINVALID( pItem, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	
	return (UnitGetExperienceLevel( pItem ) / 6) + 1;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSetQuantity(
	UNIT *pItem,
	int nQuantity,
	DBOPERATION_CONTEXT *pOpContext /*= NULL */)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitGetStat( pItem, STATS_ITEM_QUANTITY_MAX ) >= nQuantity, "Trying to set quantity on unit that is larger than the max quantity allowed for this unit" );
	ASSERTX_RETURN( nQuantity >= 0, "Trying to set negative quantity on unit" );	
	
	// change quantity
	int nQuantityPrevious = UnitGetStat( pItem , STATS_ITEM_QUANTITY );
	UnitSetStat( pItem , STATS_ITEM_QUANTITY, nQuantity );
	
	// commit change to database for items owned by players
	if (nQuantityPrevious != nQuantity)
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pItem );
		s_DatabaseUnitOperation( 
			pUltimateOwner, 
			pItem, 
			DBOP_UPDATE, 
			s_GetDBUnitTickDelay(DBUF_QUANTITY), 
			DBUF_QUANTITY,
			pOpContext,
			0);
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetQuantity(
	UNIT *pItem)
{
	ASSERTX_RETZERO( pItem, "Expected unit" );
	int nQuantity = 1;
	
	if (UnitIsA( pItem, UNITTYPE_ITEM ))
	{

		nQuantity = UnitGetStat( pItem , STATS_ITEM_QUANTITY );
		if (nQuantity <= 0)
		{
			nQuantity = 1;
		}
	}
	
	return nQuantity;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetUIQuantity(
	UNIT *pUnit)
{	

	int nQuantity = UnitGetStat( pUnit , STATS_ITEM_QUANTITY );
	if (nQuantity == 0)
	{
		// try money
		if (UnitIsA(pUnit, UNITTYPE_MONEY))
		{
			//For now using ingame currency only. We need to change this....
			cCurrency currency = UnitGetCurrency( pUnit );
			nQuantity = currency.GetValue( KCURRENCY_VALUE_INGAME );
		}
	}
	return nQuantity;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sAffixStatlistSortByDominance( const void * item1, const void * item2 )
{
	ASSERT_RETZERO(item1);
	ASSERT_RETZERO(item2);

	STATS_ENTRY *pStat1 = (STATS_ENTRY*)item1;
	STATS_ENTRY *pStat2 = (STATS_ENTRY*)item2;

	int affix1 = pStat1->value;
	int affix2 = pStat2->value;

	const AFFIX_DATA* affix_data1 = AffixGetData(affix1);
	const AFFIX_DATA* affix_data2 = AffixGetData(affix2);

	if ( affix_data1->nDominance > affix_data2->nDominance ) return -1;
	if ( affix_data1->nDominance < affix_data2->nDominance ) return 1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct NAME_TOKEN
{
	WCHAR uszRule[ MAX_ITEM_NAME_STRING_LENGTH ];		// the rule being used to construct name
	int nString;		// the string
	const AFFIX_DATA *pAffixData;				// affix the string came from (if present)
	const ITEM_QUALITY_DATA *pItemQualityData;  // item quality data string came from (if present)
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sModifyNameByAffixName( 
	const NAME_TOKEN *pNameToken,
	DWORD dwNameAttributes,
	WCHAR *puszNameBuffer, 
	int nBufferMaxChars)
{
	ASSERTX_RETURN( pNameToken, "Expected name token" );
	ASSERTX_RETURN( puszNameBuffer, "Expected name buffer" );
	ASSERTX_RETURN( nBufferMaxChars > 0, "Invalid name buffer size" );
	
	// get affix string
	WCHAR uszTemp[ MAX_ITEM_NAME_STRING_LENGTH ];			
	int nStringKeyAffixName = pNameToken->nString;	
	const WCHAR *puszName = StringTableGetStringByIndexVerbose( nStringKeyAffixName, dwNameAttributes, 0, NULL, NULL );
	if (puszName && puszName[ 0 ] != 0)
	{
		// copy name to temp buffer
		PStrCopy( uszTemp, puszName, MAX_ITEM_NAME_STRING_LENGTH );	
	}
	else
	{
		if (pNameToken->pAffixData)
		{
			PStrPrintf(
				uszTemp,
				MAX_ITEM_NAME_STRING_LENGTH,
				L"<MISSING AFFIX STRING '%S'> [item]",
				pNameToken->pAffixData->szName);
		}
		else if (pNameToken->pItemQualityData)
		{
			PStrPrintf( 
				uszTemp, 
				MAX_ITEM_NAME_STRING_LENGTH, 
				L"<MISSING ITEM QUALITY STRING '%S'> [item]", 
				pNameToken->pItemQualityData->szName);
		}
		else
		{
			PStrPrintf( 
				uszTemp, 
				MAX_ITEM_NAME_STRING_LENGTH,
				L"MISSING AFFIX [item]");
		}
	}
	
	// replace the [item] token with the name that we have so far
	PStrReplaceToken( uszTemp, MAX_ITEM_NAME_STRING_LENGTH, StringReplacementTokensGet( SR_ITEM ), puszNameBuffer );
	
	// copy new name back to buffer
	PStrCopy( puszNameBuffer, uszTemp, nBufferMaxChars );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemProperNameSet(
	UNIT *pItem,
	int nNameIndex,
	BOOL bForce)
{
	ASSERTX_RETURN( 
		pItem != NULL && 
		nNameIndex >= 0 && 
		nNameIndex < MonsterNumProperNames(), 
		"Invalid params for MonsterProperNameSet()" );

	// don't set names for items that don't allow random ones
	BOOL bSet = TRUE;	
	if (bForce == FALSE)
	{
	
		// check for unit datas that don't allow unique names
		const UNIT_DATA *pUnitData = UnitGetData( pItem );
		if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NO_RANDOM_PROPER_NAME))
		{			
			bSet = FALSE;
		}
							
	}

	// set name
	if (bSet == TRUE)
	{
		const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( nNameIndex );
		if (pMonsterNameData->bIsNameOverride)
			UnitSetNameOverride( pItem, pMonsterNameData->nStringKey );
		else
			UnitSetStat( pItem, STATS_MONSTER_UNIQUE_NAME, nNameIndex );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemUniqueNameGet(
	UNIT *pItem)
{
	ASSERTX_RETINVALID( pItem != NULL, "Expected item" );
	return UnitGetStat( pItem, STATS_MONSTER_UNIQUE_NAME );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sItemUniqueNameGetString(
	int nNameIndex)
{
	const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( nNameIndex );
	return StringTableGetStringByIndex( pMonsterNameData->nStringKey );
}

//----------------------------------------------------------------------------
struct ITEM_NAME_MODIFIERS
{
	const AFFIX_DATA *pAffixNames[ ANT_NUM_TYPES ];
	const AFFIX_DATA *pSuffixNames[ ANT_NUM_TYPES ];
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sItemHasPossessiveAffixes( 
	UNIT *pItem,
	const ITEM_NAME_MODIFIERS *pNameModifiers)
{
	ASSERTX_RETFALSE( pItem, "Expected unit" );
	ASSERTX_RETFALSE( pNameModifiers, "Expected item name modifiers" );
	
	// what attribute bit is the possessive attribute in the string table (if we have one, Hungarian does)
	int nPossessiveAttributeBit = StringTableGetAttributeBit( ATTRIBUTE_POSSESSIVE );	
	if (nPossessiveAttributeBit != NONE)
	{

		// go through the affixes
		for (int i = 0; i < ANT_NUM_TYPES; ++i)
		{
		
			// check affix
			const AFFIX_DATA *pAffix = pNameModifiers->pAffixNames[ i ];
			if (pAffix)
			{
				if (AffixNameIsPossessive( pAffix, nPossessiveAttributeBit ))
				{
					return TRUE;
				}
			}
			
			// check suffix
			pAffix = pNameModifiers->pSuffixNames[ i ];
			if (pAffix)
			{
				if (AffixNameIsPossessive( pAffix, nPossessiveAttributeBit ))
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
static void sGetItemNameModifiers(
	UNIT *pUnit,
	ITEM_NAME_MODIFIERS *pNameModifiers)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pUnit, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETURN( pNameModifiers, "Expected item name modifiers" );

	// clear out the item name modifiers
	for (int i = 0; i < ANT_NUM_TYPES; ++i)
	{
		pNameModifiers->pAffixNames[ i ] = NULL;
		pNameModifiers->pSuffixNames[ i ] = NULL;
	}
	
	// get affixes on item	
	STATS_ENTRY tStatsListAffixes[ MAX_AFFIX_ARRAY_PER_UNIT ];
	int nAffixCount = UnitGetStatsRange( pUnit, STATS_APPLIED_AFFIX, 0, tStatsListAffixes, AffixGetMaxAffixCount()) ;
	if (nAffixCount > 0)
	{
		qsort( tStatsListAffixes, nAffixCount, sizeof( STATS_ENTRY ), sAffixStatlistSortByDominance );
	}

	// find affixes that are capable of modifying our name
	for (int i = 0; i < nAffixCount; ++i)
	{
		int nAffix = tStatsListAffixes[ i ].value;
		const AFFIX_DATA *pAffixData = AffixGetData( nAffix );

		if (pAffixData->bIsSuffix)
		{
			// check each of the name types
			for (int j = 0; j < ANT_NUM_TYPES; ++j)
			{
				if (pAffixData->nStringName[ j ] != INVALID_LINK)
				{
					if (pNameModifiers->pSuffixNames[ j ] == NULL || 
						pAffixData->nDominance > pNameModifiers->pSuffixNames[ j ]->nDominance)
					{
						pNameModifiers->pSuffixNames[ j ] = pAffixData;
					}
				}
			}
		}
		else
		{
			// check each of the name types
			for (int j = 0; j < ANT_NUM_TYPES; ++j)
			{
				if (pAffixData->nStringName[ j ] != INVALID_LINK)
				{
					if (pNameModifiers->pAffixNames[ j ] == NULL || 
						pAffixData->nDominance > pNameModifiers->pAffixNames[ j ]->nDominance)
					{
						pNameModifiers->pAffixNames[ j ] = pAffixData;
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *sGetAffixRule( 
	const ITEM_NAME_INFO &tItemNameInfo, 
	GLOBAL_STRING eGlobalStringRule)
{
	static const WCHAR *puszUnknown = L"";
	
	// get the key of the string
	const char *pszKey = GlobalStringGetKey( eGlobalStringRule );
	ASSERTX_RETVAL( pszKey, puszUnknown, "Expected string key from rule" );
	
	// we want to match as many attributes of the item name attributes as we can,
	// this will allow for possessive affixes
	const WCHAR *puszRule = StringTableGetStringByKeyVerbose( 
		pszKey, 
		tItemNameInfo.dwStringAttributes, 
		0, 
		NULL, 
		NULL);
	
	// return the rule	
	return puszRule;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetName(
	UNIT *pUnit,
	WCHAR *puszNameBuffer,
	int nBufferMaxChars,
	DWORD dwFlags)
{
	WCHAR uszWorkingName[ MAX_ITEM_NAME_STRING_LENGTH ];
	
	const UNIT_DATA *pItemData = ItemGetData( UnitGetClass( pUnit ) );
	if (!pItemData)
	{
		return FALSE;
	}

	// get quantity of item
	int nQuantity = ItemGetUIQuantity( pUnit );

	// get all the affixes that can modify the name
	ITEM_NAME_MODIFIERS tItemNameModifiers;
	sGetItemNameModifiers( pUnit, &tItemNameModifiers );	

	// can we do affix modifiactions on the name
	BOOL bAllowAffixModifications = FALSE;
	if (!UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_NO_NAME_MODIFICATIONS) &&
		TESTBIT( dwFlags, UNF_NAME_NO_MODIFICATIONS ) == FALSE )
	{
		bAllowAffixModifications = TRUE;
	}

	// should we get the possessive form of the item noun
	DWORD dwItemNameInfoFlags = 0;
	if (bAllowAffixModifications == TRUE && sItemHasPossessiveAffixes( pUnit, &tItemNameModifiers ))
	{
		SETBIT( dwItemNameInfoFlags, INIF_POSSIVE_FORM );
	}
	
	// get the item name in singular or plural form
	ITEM_NAME_INFO tItemNameInfo;
	ItemGetNameInfo( tItemNameInfo, pUnit, nQuantity, dwItemNameInfoFlags );	
	if (UnitIsA( pUnit, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		UNIT *pResult = RecipeSingleUseGetFirstResult( pUnit );
		if (pResult)
		{
		
			// get name info for first result
			WCHAR uszResultName[ MAX_ITEM_NAME_STRING_LENGTH ];
			ItemGetName( pResult, uszResultName, MAX_ITEM_NAME_STRING_LENGTH, 0 );
						
			// construct final name info with result a part of the name
			PStrReplaceToken( 
				tItemNameInfo.uszName, 
				MAX_ITEM_NAME_STRING_LENGTH, 
				StringReplacementTokensGet( SR_ITEM ),
				uszResultName);
				
		}
		
	}

	// init string
	uszWorkingName[ 0 ] = 0;

	// apply first implicit rule (item base name itself)
	PStrCopy( uszWorkingName, tItemNameInfo.uszName, MAX_ITEM_NAME_STRING_LENGTH );

	// check for affixes modifying the item name
	if (bAllowAffixModifications)
	{
											
		// apply dominant affix as name replacement (if any)
		const AFFIX_DATA *pAffixReplacement = tItemNameModifiers.pAffixNames[ ANT_REPLACEMENT ];		
		const AFFIX_DATA *pSuffixReplacement = tItemNameModifiers.pSuffixNames[ ANT_REPLACEMENT ];		
		if (pAffixReplacement != NULL)
		{
			// get string, passing along any attributes to match if possible	
			const WCHAR *puszName = StringTableGetStringByIndexVerbose(
				pAffixReplacement->nStringName[ ANT_REPLACEMENT ],
				tItemNameInfo.dwStringAttributes,
				0,
				NULL,
				NULL);

			// save dom affix as replacement
			PStrCopy( uszWorkingName, puszName, MAX_ITEM_NAME_STRING_LENGTH );			
			
		}
		else if (pSuffixReplacement != NULL)
		{
			// get string, passing along any attributes to match if possible	
			const WCHAR *puszName = StringTableGetStringByIndexVerbose(
				pSuffixReplacement->nStringName[ ANT_REPLACEMENT ],
				tItemNameInfo.dwStringAttributes,
				0,
				NULL,
				NULL);

			// save dom affix as replacement
			PStrCopy( uszWorkingName, puszName, MAX_ITEM_NAME_STRING_LENGTH );			
			
		}
		else
		{
		
			const int NUM_AFFIX_NAME_RULES = 4;
			NAME_TOKEN tNameToken[ NUM_AFFIX_NAME_RULES ];
			for (int i = 0; i < NUM_AFFIX_NAME_RULES; ++i)
			{
				NAME_TOKEN *pNameToken = &tNameToken[ i ];
				pNameToken->nString = INVALID_LINK;
				pNameToken->uszRule[ 0 ] = 0;
				pNameToken->pAffixData = NULL;
				pNameToken->pItemQualityData = NULL;
			}
			
			// setup the localized tokens which govern the name construction order rules
			PStrPrintf( tNameToken[ 0 ].uszRule, MAX_ITEM_NAME_STRING_LENGTH, sGetAffixRule( tItemNameInfo, GS_AFFIX_NAME_APPLY_1 ) );
			PStrPrintf( tNameToken[ 1 ].uszRule, MAX_ITEM_NAME_STRING_LENGTH, sGetAffixRule( tItemNameInfo, GS_AFFIX_NAME_APPLY_2 ) );
			PStrPrintf( tNameToken[ 2 ].uszRule, MAX_ITEM_NAME_STRING_LENGTH, sGetAffixRule( tItemNameInfo, GS_AFFIX_NAME_APPLY_3 ) );
			PStrPrintf( tNameToken[ 3 ].uszRule, MAX_ITEM_NAME_STRING_LENGTH, sGetAffixRule( tItemNameInfo, GS_AFFIX_NAME_APPLY_4 ) );

			// set the matching strings for each localized token
			const WCHAR *puszMagicToken			= StringReplacementTokensGet( SR_MAGIC );
			const WCHAR *puszMagicSuffixToken	= StringReplacementTokensGet( SR_MAGIC_SUFFIX );
			const WCHAR *puszSetToken			= StringReplacementTokensGet( SR_SET );
			const WCHAR *puszQualityToken		= StringReplacementTokensGet( SR_QUALITY );
			for (int i = 0; i < NUM_AFFIX_NAME_RULES; ++i)
			{
				NAME_TOKEN *pNameToken = &tNameToken[ i ];
				if (PStrICmp( pNameToken->uszRule, puszMagicToken ) == 0)
				{
					if (TESTBIT( dwFlags, UNF_BASE_NAME_ONLY_BIT ) == FALSE)
					{
						const AFFIX_DATA *pMagicAffix = tItemNameModifiers.pAffixNames[ ANT_MAGIC ];
						pNameToken->nString = pMagicAffix ? pMagicAffix->nStringName[ ANT_MAGIC ] : INVALID_LINK;
						pNameToken->pAffixData = pMagicAffix;
					}
				}
				if (PStrICmp( pNameToken->uszRule, puszMagicSuffixToken ) == 0)
				{
					if (TESTBIT( dwFlags, UNF_BASE_NAME_ONLY_BIT ) == FALSE)
					{
						const AFFIX_DATA *pMagicSuffix = tItemNameModifiers.pSuffixNames[ ANT_MAGIC ];
						pNameToken->nString = pMagicSuffix ? pMagicSuffix->nStringName[ ANT_MAGIC ] : INVALID_LINK;
						pNameToken->pAffixData = pMagicSuffix;
					}
				}
				else if (PStrICmp( pNameToken->uszRule, puszSetToken ) == 0)
				{
					const AFFIX_DATA *pSetAffix = tItemNameModifiers.pAffixNames[ ANT_SET ];
					pNameToken->nString = pSetAffix ? pSetAffix->nStringName[ ANT_SET ] : INVALID_LINK;
					pNameToken->pAffixData = pSetAffix;
				}
				else if (PStrICmp( pNameToken->uszRule, puszQualityToken ) == 0)
				{
					// for some items we will actually use the item quality (normal, rare, epic, etc)
					ONCE
					{
						if (pItemData->eQualityName == QN_ITEM_QUALITY_DATATABLE)
						{
							int nItemQuality = ItemGetQuality( pUnit );
							if (nItemQuality != INVALID_LINK)
							{
								const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nItemQuality );
								if(pItemQualityData)
								{
									pNameToken->nString = pItemQualityData->nDisplayNameWithItemFormat;
									pNameToken->pItemQualityData = pItemQualityData;
									break;
								}
							}
						}
						// use the rank affix
						{
							const AFFIX_DATA *pQualityAffix = tItemNameModifiers.pAffixNames[ ANT_QUALITY ];
							pNameToken->nString = pQualityAffix ? pQualityAffix->nStringName[ ANT_QUALITY ] : INVALID_LINK;
							pNameToken->pAffixData = pQualityAffix;
						}
					}					
				}

			}
						
			// apply the rules, in order
			for (int i = 0; i < NUM_AFFIX_NAME_RULES; ++i)
			{
				const NAME_TOKEN *pNameToken = &tNameToken[ i ];
				if (pNameToken->nString != INVALID_LINK)
				{				
					sModifyNameByAffixName( 
						pNameToken,
						tItemNameInfo.dwStringAttributes, 
						uszWorkingName, 
						MAX_ITEM_NAME_STRING_LENGTH);
				}												
				
			}
						
		}

	}
	int nUniqueNameIndex = UnitGetStat( pUnit, STATS_MONSTER_UNIQUE_NAME );
	// randomly 'named' item
	if( nUniqueNameIndex != INVALID_ID && !ItemIsUnidentified( pUnit ) )
	{
		// copy name as we know it
		WCHAR uszTemp[ MAX_ITEM_NAME_STRING_LENGTH ];		
		PStrCopy( uszTemp, uszWorkingName, MAX_ITEM_NAME_STRING_LENGTH );
		
		// copy unidentified string to buffer
		PStrCopy( uszWorkingName, GlobalStringGet( GS_NAMEDITEM ), MAX_ITEM_NAME_STRING_LENGTH );
		
		// replace the item token in the buffer with the actual item name
		PStrReplaceToken( 
			uszWorkingName, 
			MAX_ITEM_NAME_STRING_LENGTH, 
			StringReplacementTokensGet( SR_ITEM ),
			uszTemp );

		// replace the item token in the buffer with the actual item name
		PStrReplaceToken( 
			uszWorkingName, 
			MAX_ITEM_NAME_STRING_LENGTH, 
			StringReplacementTokensGet( SR_MONSTER ),
			sItemUniqueNameGetString( nUniqueNameIndex ) );

	}

	// do unidentified labels
	if( ItemIsUnidentified( pUnit ) &&
		TESTBIT( dwFlags, UNF_NAME_NO_MODIFICATIONS ) == FALSE )
	{

		// copy name a we know it
		WCHAR uszTemp[ MAX_ITEM_NAME_STRING_LENGTH ];		
		PStrCopy( uszTemp, uszWorkingName, MAX_ITEM_NAME_STRING_LENGTH );
		
		// copy unidentified string to buffer
		const char *szUnidentifiedKey = GlobalStringGetKey( GS_UNIDENTIFIED );
		const WCHAR *puszUnidentified = StringTableGetStringByKeyVerbose( 
			szUnidentifiedKey, 
			tItemNameInfo.dwStringAttributes, 
			0, 
			NULL, 
			NULL);
		PStrCopy( uszWorkingName, puszUnidentified, MAX_ITEM_NAME_STRING_LENGTH );
		
		// replace the item token in the buffer with the actual item name
		PStrReplaceToken( 
			uszWorkingName, 
			MAX_ITEM_NAME_STRING_LENGTH, 
			StringReplacementTokensGet( SR_ITEM ),
			uszTemp );

	}

	// add quantity or money amount numbers to the item name
	if (nQuantity > ( AppIsHellgate() ? 0 : 1 ) )
	{
		sItemNameModifyByQuantity( nQuantity, uszWorkingName, MAX_ITEM_NAME_STRING_LENGTH );		
	}	

	//// add rank to mods
	//if (UnitIsA(pUnit, UNITTYPE_MOD))
	//{
	//	// add the mod's "rank"
	//	int nRank = ItemGetRank( pUnit );
	//	PStrPrintf( szTemp, MAX_ITEM_NAME_STRING_LENGTH, L" %d", nRank );
	//	PStrCat( puszNameBuffer, szTemp, nBufferMaxChars );
	//}

	// finally, copy to resulting buffer
	PStrCopy( puszNameBuffer, uszWorkingName, nBufferMaxChars );
				
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemGetAffixListString(
	UNIT *pUnit,
	WCHAR *puszString,
	int nStringLength)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( puszString, "Expected string buffer" );	
	ASSERTX_RETURN( nStringLength > 0, "Expected buffer larger than zero" );
		
	// init string
	puszString[ 0 ] = 0;

	// if not identified, use instructions
	if (ItemIsIdentified( pUnit ) == FALSE)
	{
		const WCHAR *puszText = GlobalStringGet( GS_UNIDENTIFIED_INSTRUCTIONS );
		PStrCopy( puszString, puszText, nStringLength );
		return;
	}
	
	int nQuality = ItemGetQuality( pUnit );
	if (nQuality != INVALID_LINK)
	{
			
		// get quantity for purposes of display
		int nQuantity = ItemGetUIQuantity( pUnit );
	
		// get the item name info
		ITEM_NAME_INFO tItemNameInfo;
		ItemGetNameInfo( tItemNameInfo, pUnit, nQuantity, 0 );
			
		// insert coloring
		int nColor = UnitGetNameColor( pUnit );
		if (nColor != FONTCOLOR_INVALID)
		{
			UIAddColorToString(puszString, (FONTCOLOR)nColor, nStringLength );
		}

		// get affixes
		STATS_ENTRY tStatsListAffix[ MAX_AFFIX_ARRAY_PER_UNIT ];
		int nAffixCount = UnitGetStatsRange( pUnit, STATS_APPLIED_AFFIX, 0, tStatsListAffix, AffixGetMaxAffixCount() );

		int nAdded = 0;
		for (int i = 0; i < nAffixCount; ++i)
		{
			int nAffix = tStatsListAffix[ i ].value;
			const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
						
			// add affixes with a sub name
			if (pAffixData->nStringName[ ANT_MAGIC ] != INVALID_LINK)
			{
			
				// add a separator for more than one affix
				if (nAdded > 0)
				{
					PStrCat( puszString, L", ", nStringLength );
				}

				// get affix name (make it clean without any name construction tokens)
				WCHAR uszAffixNameClean[ MAX_ITEM_NAME_STRING_LENGTH ];
				AffixGetMagicName( 
					nAffix, 
					tItemNameInfo.dwStringAttributes, 
					uszAffixNameClean, 
					MAX_ITEM_NAME_STRING_LENGTH,
					TRUE);
					
				// append to name
				PStrCat( puszString, uszAffixNameClean, nStringLength );
				nAdded++;
			
			}

		}
		
		if (nColor != FONTCOLOR_INVALID)
		{
			UIAddColorReturnToString(puszString, nStringLength );
		}

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetAffixPropertiesString(
	UNIT *pUnit,
	WCHAR *puszString,
	int nStringLength,
	BOOL bExtended)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( puszString, "Expected string buffer" );	
	ASSERTX_RETFALSE( nStringLength > 0, "Expected buffer larger than zero" );
		
	// init string
	puszString[ 0 ] = 0;

	// if not identified, use instructions
	if (ItemIsIdentified( pUnit ) == FALSE)
	{
		const WCHAR *puszText = GlobalStringGet( GS_UNIDENTIFIED_AFFIX_PROPERTIES_LIST );
		PStrCopy( puszString, puszText, nStringLength );
		return TRUE;
	}
	
	// clear the buffer
	puszString[0] = L'\0';

	// get the item name info
	ITEM_NAME_INFO tItemNameInfo;
	ItemGetNameInfo( tItemNameInfo, pUnit, 1, 0 );

	int nQuality = ItemGetQuality( pUnit );
	if (nQuality != INVALID_LINK)
	{

		STATS * modlist = NULL;
		STATS * nextlist = StatsGetModList(pUnit, SELECTOR_AFFIX);
		while ((modlist = nextlist) != NULL)
		{
			nextlist = StatsGetModNext(modlist, SELECTOR_AFFIX);

			int nAffix = StatsGetStat(modlist, STATS_AFFIX_ID);
			const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
			ASSERTV_CONTINUE( pAffixData, "Couldn't find affix data for item '%s'", UnitGetDevName( pUnit ) );

			// Start with the affix name
			// get affix name (make it clean without any name construction tokens)
			WCHAR uszAffixNameClean[ MAX_ITEM_NAME_STRING_LENGTH ] = L"";
			AffixGetMagicName( 
				nAffix, 
				tItemNameInfo.dwStringAttributes, 
				uszAffixNameClean, 
				MAX_ITEM_NAME_STRING_LENGTH,
				TRUE);

			if (!uszAffixNameClean[0])
			{
//				continue;
			}

			// get the color of this affix type
			int nColor = INVALID_LINK;
			if (pAffixData->nAffixType[0] != INVALID_LINK)
			{
				const AFFIX_TYPE_DATA * pAffixTypeData = AffixTypeGetData(UnitGetGame(pUnit), pAffixData->nAffixType[0]);
				ASSERT_RETFALSE(pAffixTypeData);
				nColor = pAffixTypeData->nNameColor;
			}

			WCHAR szStats[1024];
			PrintStats(UnitGetGame(pUnit), DATATABLE_ITEMDISPLAY, -1, NULL, szStats, arrsize(szStats), modlist);
//			ASSERTX_CONTINUE(szStats[0], "Couldn't find any property descriptions for affix");

			// Now put it together and add it
			WCHAR szAffixLine[2048] = L"\0";
			if (!bExtended || !uszAffixNameClean[0])
			{
				PStrCat(szAffixLine,  szStats, arrsize(szAffixLine));
//				UIColorCatString(szAffixLine, arrsize(szAffixLine), (FONTCOLOR)nColor, szStats);
			}
			else
			{
				PStrCopy(szAffixLine, GlobalStringGet(GS_PROPERY_HEADING), arrsize(szAffixLine));

				if (nColor != INVALID_LINK)
				{
					WCHAR uszAffixNameColor[256] = L"\0";
					UIColorCatString(uszAffixNameColor, arrsize(uszAffixNameColor), (FONTCOLOR)nColor, uszAffixNameClean);
					PStrReplaceToken(szAffixLine, arrsize(szAffixLine), StringReplacementTokensGet(SR_PROPERTY), uszAffixNameColor);
				}
				else
				{
					PStrReplaceToken(szAffixLine, arrsize(szAffixLine), StringReplacementTokensGet(SR_PROPERTY), uszAffixNameClean);
				}

				PStrReplaceToken(szAffixLine, arrsize(szAffixLine), StringReplacementTokensGet(SR_VALUE), szStats);
			}

			if (puszString[0] != L'\0' && szAffixLine[0] != L'\0')
				PStrCat(puszString, L"\n", nStringLength);
			PStrCat(puszString, szAffixLine, nStringLength);
		}
	}

	return (puszString[0] != L'\0');
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetInvItemsPropertiesString(
	UNIT *pUnit,
	int nLocation, 
	WCHAR *puszString,
	int nStringLength)
{
	if (!pUnit)
	{
		return FALSE;
	}

	if (nLocation == INVALID_LINK)
	{
		return FALSE;
	}

	if (!UnitInventoryHasLocation(pUnit, nLocation))
	{
		return FALSE;
	}
	DWORD dwFlags = 0;
	SETBIT(dwFlags, UNF_EMBED_COLOR_BIT);
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryLocationIterate(pUnit, nLocation, pItem, 0, FALSE)) != NULL)
	{
		WCHAR szName[MAX_ITEM_NAME_STRING_LENGTH];
		UnitGetName(pItem, szName, MAX_ITEM_NAME_STRING_LENGTH, dwFlags);

		WCHAR szStats[1024];
		PrintStats(UnitGetGame(pUnit), DATATABLE_ITEMDISPLAY, SDTTA_OTHER, pItem, szStats, arrsize(szStats));

		if (puszString[0] != L'\0')
			PStrCat(puszString, L"\n", nStringLength);

		PStrCat(puszString, szName, nStringLength);
		PStrCat(puszString, L": ", nStringLength);
		PStrCat(puszString, szStats, nStringLength);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *c_ItemGetUseResultString(
	USE_RESULT eResult)
{
	GLOBAL_STRING eString = GS_INVALID;
	
	switch (eResult)
	{
		case USE_RESULT_CANNOT_USE_IN_TOWN:		eString = GS_CANNOT_USE_IN_TOWN;		break;
		case USE_RESULT_CANNOT_USE_IN_HELLRIFT:	eString = GS_CANNOT_USE_IN_HELLRIFT;	break;
		case USE_RESULT_CANNOT_USE_COOLDOWN:	/*eString = GS_CANNOT_USE_COOLDOWN;*/	break;
		case USE_RESULT_CANNOT_USE_POISONED:	eString = GS_CANNOT_USE_POISONED;		break;
		default:								eString = GS_CANNOT_USE;				break;
	}
	if (eString != GS_INVALID)
	{
		return GlobalStringGet( eString );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInCursor(
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected unit" );
	INVENTORY_LOCATION tInvLocation;
	
	// get inventory location
	UnitGetInventoryLocation( pItem, &tInvLocation );

	// get inv loc data
	UNIT *pContainer = UnitGetContainer( pItem );
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, tInvLocation.nInvLocation );
	if(!pInvLocData)
		return FALSE;
	return pInvLocData->bIsCursor;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemGetEmbeddedTextModelString(
	UNIT *pItem,
	WCHAR *puszBuffer,
	int nMaxBuffer,
	BOOL bBackgroundFrame,
	int nGridSize /*= 32*/)
{

#if !ISVERSION(SERVER_VERSION)
	// get grid size
	int nWidth, nHeight;
	UnitGetSizeInGrid( pItem, &nHeight, &nWidth, NULL );
	nWidth *= nGridSize;
	nHeight *= nGridSize;

	ASSERT_RETURN(puszBuffer);
	puszBuffer[0] = L'\0';

	WCHAR uszTemp[512];
	if (bBackgroundFrame)
	{
		PStrPrintf( 
			uszTemp, 
			arrsize(uszTemp), 
			L"[%s name=item in text background, texture=main_atlas, undertext=1, sx=%d sy=%d]",
			UIGetTextTag(TEXT_TAG_FRAME),
			nWidth,
			nHeight);


		PStrCat(puszBuffer, uszTemp, nMaxBuffer);
	}

	PStrPrintf( 
		uszTemp, 
		arrsize(uszTemp), 
		L"[%s id=%d sx=%d sy=%d]",
		UIGetTextTag(TEXT_TAG_UNIT),
		UnitGetId( pItem ),
		nWidth,
		nHeight);

	PStrCat(puszBuffer, uszTemp, nMaxBuffer);

#else
    ASSERT(FALSE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetInteractChoices( 
	UNIT * pUser, 
	UNIT * pItem,
	INTERACT_MENU_CHOICE *ptChoices,
	int nBufferSize,
	UI_COMPONENT *pComponent /*=NULL*/)
{

	int nNumChoices = 0;
	for (int i = 0; i < nBufferSize; ++i)
	{
		ptChoices[ i ].nInteraction = UNIT_INTERACT_INVALID;
		ptChoices[ i ].bDisabled = FALSE;
	}
	
	BOOL bItemIsInShop = ItemBelongsToAnyMerchant(pItem);		
	BOOL bItemIsInSharedStash = ItemIsInSharedStash(pItem);

	UNIT *pOwner = UnitGetUltimateOwner(pItem);
	if (pOwner == pUser)
	{
	
		if (bItemIsInShop == FALSE && bItemIsInSharedStash == FALSE)
		{

			// dismantle items
			if (ItemCanBeDismantled(pUser, pItem))
			{
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMDISMANTLE, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}
			
			// usable items
			if (ItemIsUsable( pUser, pItem, NULL, ITEM_USEABLE_MASK_NONE ))
			{
				BOOL bCanUse = UnitIsUsable(pUser, pItem) == USE_RESULT_OK;
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMUSE, bCanUse, &nNumChoices, ptChoices, nBufferSize );
			}

			// KCK: If it's stackable, it should be splitable
			if (UnitGetStat(pItem, STATS_ITEM_QUANTITY) > 1)
			{
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMSPLIT, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}

			// equip
			if (!ItemIsInEquipLocation(pUser, pItem))
			{
				int location, invx, invy;
				ItemGetEquipLocation(pUser, pItem, &location, &invx, &invy);
				if (location != INVLOC_NONE)
				{
					if (InventoryItemCanBeEquipped(pUser, pItem))
					{
						InteractAddChoice( pUser, UNIT_INTERACT_ITEMEQUIP, TRUE, &nNumChoices, ptChoices, nBufferSize );
					}
					
				}	
						
			}
			
			// pick item colors
			const UNIT_DATA * pItemUnitData = UnitGetData( pItem );
			if (ItemIsEquipped(pItem, pOwner) && (pItemUnitData && pItemUnitData->nColorSet != INVALID_ID ))
			{
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMPICKCOLOR, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}
			
			// item drop
			if (!UnitIsNoDrop(pItem) &&
				InventoryIsInLocationType(pItem, LT_STANDARD) &&
				!ItemIsInRewardLocation(pItem) &&
				ItemIsInCanTakeLocation(pItem))
			{
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMDROP, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}
		}

		if (bItemIsInShop == FALSE)
		{
			// item identify
			if (ItemIsUnidentified(pItem))
			{
				BOOL bCanIdentify = UnitCanIdentify( pUser );
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMIDENTIFY, bCanIdentify, &nNumChoices, ptChoices, nBufferSize );				
			}
			// Item in chat (only for items you own)
			else if (UnitGetUltimateOwner(pItem) == pUser)
			{
				InteractAddChoice( pUser, UNIT_INTERACT_ITEMSHOWINCHAT, TRUE, &nNumChoices, ptChoices, nBufferSize );				
			}
		}
		
		// item modes
		if (!ItemHasSkillWhenUsed(pItem) && UnitIsExaminable(pItem))
		{
			InteractAddChoice( pUser, UNIT_INTERACT_ITEMMOD, TRUE, &nNumChoices, ptChoices, nBufferSize );		
		}
		
	}
	
	// shop interactions
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(UnitGetGame(pUser)) && UIIsMerchantScreenUp() && !UnitIsNoDrop( pItem ) && pOwner == pUser)
	{
		UNIT *pMerchant = TradeGetTradingWith( pUser );
		if (bItemIsInShop == FALSE &&
			pMerchant &&
			StoreMerchantWillBuyItem(pMerchant, pItem))
		{
			InteractAddChoice( pUser, UNIT_INTERACT_ITEMSELL, TRUE, &nNumChoices, ptChoices, nBufferSize );				
		}

		if (bItemIsInShop == TRUE)
		{
			InteractAddChoice( pUser, UNIT_INTERACT_ITEMBUY, TRUE, &nNumChoices, ptChoices, nBufferSize );				
		}
	}
#endif //!SERVER_VERSION

	return nNumChoices;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemUniqueGetBaseString(
	UNIT *pUnit,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize, "Invalid buffer size" );

	int nQuality = ItemGetQuality( pUnit );
	if (nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pQualityData = ItemQualityGetData( nQuality );
		ASSERT_RETURN(pQualityData);
		if (pQualityData->bShowBaseDesc && ItemIsIdentified( pUnit ))
		{
		
			// get format string
			const WCHAR *puszQualityFormat = StringTableGetStringByIndex(pQualityData->nBaseDescFormat);
			PStrCopy( puszBuffer, puszQualityFormat, nBufferSize );
						
			// replace quality
			const WCHAR *puszQualityToken = StringReplacementTokensGet( SR_QUALITY );
			const int MAX_BUFFER = 128;			
			WCHAR uszQualityBuffer[ MAX_BUFFER ];
			ItemGetQualityString( pUnit, uszQualityBuffer, MAX_BUFFER, 0 );
			PStrReplaceToken( puszBuffer, nBufferSize, puszQualityToken, uszQualityBuffer );

			// replace item
			const WCHAR *puszItemToken = StringReplacementTokensGet( SR_ITEM );
			int nItemClass = UnitGetClass( pUnit );
			ITEM_NAME_INFO tItemNameInfo;
			ItemGetNameInfo( tItemNameInfo, nItemClass, ItemGetUIQuantity( pUnit ), 0 );
			PStrReplaceToken( puszBuffer, nBufferSize, puszItemToken, tItemNameInfo.uszName );

			// replace base item
			const WCHAR *puszItemBaseToken = StringReplacementTokensGet( SR_BASE_ITEM );
			GENUS eGenus = UnitGetGenus( pUnit );
			int nItemBaseClass = UnitGetBaseClass( eGenus, nItemClass );
			ITEM_NAME_INFO tBaseItemNameInfo;
			ItemGetNameInfo( tBaseItemNameInfo, nItemBaseClass, ItemGetUIQuantity( pUnit ), 0 );
			PStrReplaceToken( puszBuffer, nBufferSize, puszItemBaseToken, tBaseItemNameInfo.uszName );

		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetQuality(
	UNIT *pItem)
{
	ASSERTX_RETINVALID( pItem, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	return UnitGetStat( pItem, STATS_ITEM_QUALITY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSetQuality(
	UNIT *pItem,
	int nItemQuality)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	UnitSetStat( pItem, STATS_ITEM_QUALITY, nItemQuality );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetScrapQuality(
	UNIT *pItem)
{
	int nItemQuality = ItemGetQuality( pItem );
	
	// get the current quality, see if the quality of the item produces a different scrap quality
	if (nItemQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nItemQuality );
		if (pItemQualityData && pItemQualityData->nScrapQuality != INVALID_LINK)
		{
			nItemQuality = pItemQualityData->nScrapQuality;
		}
	}

	// return quality
	return nItemQuality;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *ItemGetQualityName(
	UNIT *pItem)
{
	ASSERT_RETNULL( pItem );

	int nQuality = ItemGetQuality( pItem );
	if (nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pQualityData = ItemQualityGetData( nQuality );
		ASSERT_RETNULL( pQualityData );
		
		return pQualityData->szName;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetTurretizeSkill(	
	UNIT *pItem,
	SKILL_FAMILY eFamily)
{
	ASSERTX_RETINVALID( pItem, "Expected item unit" );
	
	// we could cache this somewhere, but it's so in frequent I bet we won't care
	int nNumSkills = ExcelGetNumRows( NULL, DATATABLE_SKILLS );
	for (int i = 0; i < nNumSkills; ++i)
	{
		const SKILL_DATA *pSkillData = SkillGetData( NULL, i );
		if ( ! pSkillData )
			continue;
		if (pSkillData->eFamily == eFamily)
		{
		
			// check target unit type
			if (UnitIsA( pItem, pSkillData->tTargetQueryFilter.nUnitType ))
			{
				return i;
			}
			
		}
		
	}
	
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemCanBeTurretized(
	UNIT *pUser,
	UNIT *pItem,
	SKILL_FAMILY eFamily)
{
	ASSERTX_RETFALSE( pUser, "Expected user" );
	ASSERTX_RETFALSE( pItem, "Expected item" );
	
	// get skill	
	int nSkill = ItemGetTurretizeSkill( pItem, eFamily );
	if (nSkill != INVALID_LINK)
	{
		return UnitHasSkill( pUser, nSkill );
	}

	return FALSE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ItemTurretize(
	UNIT *pItem,
	SKILL_FAMILY eFamily)
{
	ASSERTX_RETURN( pItem, "Expected item unit" );
	GAME *pGame =  UnitGetGame( pItem );
	
	int nSkill = ItemGetTurretizeSkill( pItem, eFamily );
	if (nSkill != INVALID_LINK)
	{
		UNIT *pPlayer = GameGetControlUnit( pGame );
		if (pPlayer)
		{
			//c_StateSet( pPlayer, pPlayer, STATE_TURRETIZE_MODE, 0, 0, INVALID_LINK );
			c_SkillControlUnitRequestStartSkill( pGame, nSkill, pItem );
		}
	}
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ItemRestoreToStandardInventory( 
	UNIT *pItem, 
	UNIT *pPlayerDest,
	INV_CONTEXT eContext)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );	
	ASSERTX_RETFALSE( pPlayerDest, "Expected dest player" );

	// handle auto stacking
	if (ItemAutoStack( pPlayerDest, pItem ) == pItem)
	{
	
		// not auto stacked, find a spot for it
		int nInvSlot, nX, nY;		
		if (!ItemGetOpenLocation( pPlayerDest, pItem, TRUE, &nInvSlot, &nX, &nY ))
		{
			return FALSE;
		}

		// change location to that new spot
		return InventoryChangeLocation( pPlayerDest, pItem, nInvSlot, nX, nY, CLF_ALLOW_NEW_CONTAINER_BIT, eContext );
		
	}

	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemsRestoreToStandardLocations(
	UNIT *pUnit,
	int nInvLocSource,
	BOOL bDropItemsOnError /*= TRUE*/)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nInvLocSource != INVALID_LINK, "Expected inventory location" );
	int nNumItemsDroppedToGround = 0;
	
	// only deal with those that have clients
	if (UnitHasClient( pUnit ) == FALSE)
	{
		return;
	}
	
	// add all items left in the trade slot to the buffer
	GAME *pGame = UnitGetGame( pUnit );
	UNIT *pItem = UnitInventoryLocationIterate( pUnit, nInvLocSource, NULL );
	UNIT *pNext = NULL;
	while (pItem)
	{
	
		// get next
		pNext = UnitInventoryLocationIterate( pUnit, nInvLocSource, pItem );

		// find a spot and move it there
		if (s_ItemRestoreToStandardInventory( pItem, pUnit, INV_CONTEXT_RESTORE ) == FALSE)
		{					
			BOOL bAllowDrop = bDropItemsOnError;
			
			//don't allow items to drop that can't actually be dropped or if they are dropped are destroyed
			if (AppIsTugboat())
			{
				if (UnitIsNoDrop( pItem ) || QuestUnitIsUnitType_QuestItem( pItem ))
				{
					bAllowDrop = FALSE;
				}
			}
			
			if( bAllowDrop )
			{

				// restrict to this player so nobody else can take it
				UNITID idItem = UnitGetId( pItem );
				UnitSetRestrictedToGUID( pItem, UnitGetGuid( pUnit ) );
			
				// drop item to ground, but lock to this player so nobody can take it
				s_ItemDrop( pUnit, pItem, TRUE, TRUE );			
			
				// item must be in the world if it's still around
				pItem = UnitGetById( pGame, idItem );
				if (pItem)
				{			
						
					// count the # of items that were actually removed and put in a room
					if (UnitGetRoom( pItem ) != NULL)
					{
						nNumItemsDroppedToGround++;
					}
						
				}

			}
						
		}
		
		// goto next
		pItem = pNext;
		
	}

	// if any item was dropped to the ground, tell the player that some kind
	// of strange case has happened and that they should look for their items 
	// at their feet or something (yeah, not elegant, but this is only supposed
	// to happen in extreme fallback cases where there is literally nothing
	// else we can do)
	if (nNumItemsDroppedToGround > 0)
	{
	
		// setup message
		MSG_SCMD_ITEMS_RESTORED tMessage;
		tMessage.nNumItemsRestored = 0;
		tMessage.nNumItemsDropped = nNumItemsDroppedToGround;
		
		// send to client
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		GAME *pGame = UnitGetGame( pUnit );
		s_SendMessage( pGame, idClient, SCMD_ITEMS_RESTORED, &tMessage );	
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsRecipeComponent(
	UNIT *pItem)
{
	if (pItem)
	{
		int nRecipe = UnitGetStat( pItem, STATS_RECIPE_COMPONENT );
		return nRecipe != INVALID_LINK;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsRecipeIngredient(
	UNIT *pItem)
{
	if (pItem)
	{	
		int nRecipe = UnitGetStat( pItem, STATS_RECIPE_INGREDIENT );
		return nRecipe != INVALID_LINK;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemMeetsStatReqsForUI(
	UNIT *pItem,
	int nStat /*= INVALID_LINK*/,
	UNIT *pCompareUnit /*= NULL*/)
{
	ASSERT_RETFALSE(pItem);

	static const int MAX_ITEMS = 6;
	UNIT *pContainer = UnitGetContainer(pItem);
	UNIT* pIgnoreItems[MAX_ITEMS];
	structclear(pIgnoreItems);
	BOOL  pbPreventLoc[MAX_ITEMS];
	structclear(pbPreventLoc);
	GAME *pGame = UnitGetGame(pItem);
	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (pCompareUnit == NULL)
		pCompareUnit = pPlayer;
	
	if ( GameGetDebugFlag( pGame, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS) )
		return TRUE;

	if (pCompareUnit)
	{
		int nReqCount = StatsGetNumReqStats(pGame);
		for (int ii = 0; ii < nReqCount; ii++)
		{
			int nReqStat = StatsGetReqStat(pGame, ii);

			if (nStat == INVALID_LINK ||
				nStat == nReqStat)
			{
				if (InventoryCheckStatRequirement(nReqStat, pGame, pCompareUnit, pItem, NULL, 0) < 0)
				{
					return FALSE;
				}
			}
		}

		int nLimitCount = StatsGetNumReqLimitStats(pGame);
		for (int ii = 0; ii < nLimitCount; ii++)
		{
			int nLimitStat = StatsGetReqLimitStat(pGame, ii);

			if (nStat == INVALID_LINK ||
				nStat == nLimitStat)
			{
				if (InventoryCheckStatRequirement(nLimitStat, pGame, pCompareUnit, pItem, NULL, 0) < 0)
				{
					return FALSE;
				}
			}
		}

		if (!pContainer)
		{
			pContainer = pPlayer;	// setting pContainer to player, so we can check equip vs. player
		}
		int nFeedCount = StatsGetNumFeedStats(pGame);
		for (int ii = 0; ii < nFeedCount; ii++)
		{
			int nFeedStat = StatsGetFeedStat(pGame, ii);

			if (nStat == INVALID_LINK ||
				nStat == nFeedStat)
			{
				// Is the item already in an equip location?
				if (pContainer && !ItemIsInEquipLocation(pContainer, pItem))
				{
					BOOL bEmptySlot = FALSE;
					ItemGetItemsAlreadyInEquipLocations(pContainer, pItem, TRUE, bEmptySlot, pIgnoreItems, MAX_ITEMS, pbPreventLoc);
					int nNonPreventLocItems = 0;
					int nPreventLocItems = 0;
					UNIT* pIgnoreItemsForThisTry[MAX_ITEMS];
					structclear(pIgnoreItemsForThisTry);
					UNIT* pNonPreventLocItems[MAX_ITEMS];
					structclear(pNonPreventLocItems);
					for (int i=0; i < MAX_ITEMS; i++)
					{
						if (pIgnoreItems[i])
						{
							if (pbPreventLoc[i])
							{
								ASSERT(nPreventLocItems <= MAX_ITEMS);
								// put the preventing items first, since they would all have to 
								//   be removed in order to equip the item.  Since we're testing stats 
								//   required, we have to assume you'd be removing these items.
								pIgnoreItemsForThisTry[nPreventLocItems++] = pIgnoreItems[i];
							}
							else
							{
								ASSERT(nNonPreventLocItems <= MAX_ITEMS);
								pNonPreventLocItems[nNonPreventLocItems++] = pIgnoreItems[i];
							}
						}
					}

					ASSERT(nPreventLocItems + nNonPreventLocItems <= MAX_ITEMS);
					BOOL bOk = TRUE;

					if (nNonPreventLocItems > 0 &&
						!bEmptySlot)		// if there's an empty slot, only test for putting the item there, with no removals of non prev-loc items
					{
						for (int j=0; j < nNonPreventLocItems; j++)			// check all the different combinations of possible replacements
						{
							// set the last item (after the preventing items) to the item 
							//   we're going to test replacing
							pIgnoreItemsForThisTry[nPreventLocItems] = pNonPreventLocItems[j];
							bOk &= (InventoryCheckStatRequirement(nFeedStat, pGame, pCompareUnit, pItem, pIgnoreItemsForThisTry, MAX_ITEMS) >= 0);
						}
					}
					else
					{
						bOk = (InventoryCheckStatRequirement(nFeedStat, pGame, pCompareUnit, pItem, pIgnoreItemsForThisTry, MAX_ITEMS) >= 0);
					}

					if (!bOk)
						return FALSE;

				}
				else if (InventoryCheckStatRequirement(nFeedStat, pGame, pCompareUnit, pItem, pIgnoreItems, MAX_ITEMS) < 0)
				{
					return FALSE;
				}
			}
		}

	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInRewardLocation( 
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
		if (InvLocIsRewardLocation( pContainer, nInvLocation ) == TRUE)
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
BOOL InventoryIsInLocationWithFlag(
	UNIT *pUnit,
	INVLOC_FLAG eInvLocFlag)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( eInvLocFlag < NUM_INVLOCFLAGS, "Invalid invloc flag" );
	
	// check up containment chain
	UNIT *pContainer = UnitGetContainer( pUnit );
	while (pContainer)
	{
	
		// get the units inventory location
		int nInvLocation = INVLOC_NONE;
		UnitGetInventoryLocation( pUnit, &nInvLocation );

		// get inventory location information
		const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInvLocation );
		if (InvLocTestFlag( pInvLocData, eInvLocFlag ))
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
BOOL ItemIsNoTrade(	
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );
	ASSERTX_RETFALSE( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	return UnitGetStat( pItem, STATS_NO_TRADE )	== TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemSetNoTrade(
	UNIT *pItem,
	BOOL bValue)
{
	ASSERTX_RETURN( pItem, "Expected item" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	UnitSetStat( pItem, STATS_NO_TRADE, bValue );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetUpgradeQuality(
	UNIT *pItem)
{
	ASSERTX_RETINVALID( pItem, "Expected item" );
	int nItemQuality = ItemGetQuality( pItem );
	if (nItemQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nItemQuality );
		if(pItemQualityData) return pItemQualityData->nUpgrade;
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemsReturnToStandardLocations(	
	UNIT *pPlayer,
	BOOL bReturnCursorItems)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );

	int nInventoryIndex = UnitGetInventoryType( pPlayer );
	if (nInventoryIndex > 0)
	{
	
		// get inventory data for this unit
		const INVENTORY_DATA *pInventoryData = InventoryGetData( NULL, nInventoryIndex );
		if (pInventoryData)
		{

			// iterate the inventory slots on this unit
			for (int i = pInventoryData->nFirstLoc; i < pInventoryData->nFirstLoc + pInventoryData->nNumLocs; ++i)
			{
				const INVLOC_DATA *pInvLocData = InvLocGetData( NULL, i );
				if ( ! pInvLocData )
					continue;

				if (pInvLocData->bReturnStuckItemsToStandardLoc == TRUE ||
					(bReturnCursorItems == TRUE && pInvLocData->bIsCursor == TRUE))
				{
					s_ItemsRestoreToStandardLocations( pPlayer, pInvLocData->nLocation );			
				}
			}

		}

	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * s_ItemCloneAndCripple(
	UNIT *pItem)
{
	ASSERTX_RETNULL( pItem, "Expected unit" );
	ASSERTX_RETNULL( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	GAME *pGame = UnitGetGame( pItem );

	ITEM_SPEC tSpec;
	tSpec.nItemClass = UnitGetClass( pItem );
	tSpec.flScale = UnitGetScale( pItem );
	tSpec.nLevel = 1;
	UNIT *pClone = s_ItemSpawn( pGame, tSpec, NULL );
	ASSERTX_RETNULL( pClone, "Cold not create crippled item clone" );

	// now cripple the unit so that it's not interesting for people to even try to hack,
	// please feel free to add anything here you want to make it extra exploit proof,
	// it's crippled unit anyway and doesn't have to be just like the real one
	UnitSetFlag( pClone, UNITFLAG_NOSAVE );
	UnitSetFlag( pClone, UNITFLAG_NO_DATABASE_OPERATIONS );
	ItemSetNoTrade( pClone, TRUE );
	UnitSetStat( pClone, STATS_SELL_PRICE_OVERRIDE, 0 );	
	UnitSetFlag( pClone, UNITFLAG_IS_DECOY );

	// return the new unit
	return pClone;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemVersion(
	UNIT *pItem)
{
	ASSERTX_RETURN( pItem, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );

	// single use recipes	
	if (UnitIsA( pItem, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		s_RecipeVersion( pItem );
	}

	// we used to allow scrap qualities from all item qualities, now we force a limited subset
	if (UnitIsA( pItem, UNITTYPE_SCRAP ) && UnitGetVersion( pItem ) <= 172)
	{
		// scrap version 172 and below had qualities equal to the quality of the item
		// they were dismanted from.
		int nItemQuality = ItemGetQuality( pItem );
		if (nItemQuality == INVALID_LINK)
		{
			nItemQuality = GlobalIndexGet( GI_ITEM_QUALITY_NORMAL );
			ItemSetQuality( pItem, nItemQuality );
		}
		const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nItemQuality );
		if (pItemQualityData)
		{
			if (pItemQualityData->nScrapQuality != nItemQuality)
			{
				ItemSetQuality( pItem, pItemQualityData->nScrapQuality );
			}
		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ItemsRestored(
	int nNumItemsRestored,
	int nNumItemsDropped)
{
#if !ISVERSION(SERVER_VERSION)
	if (nNumItemsDropped)
	{
	
		// setup message
		WCHAR uszMessage[ MAX_STRING_ENTRY_LENGTH ];
		PStrPrintf(
			uszMessage,
			MAX_STRING_ENTRY_LENGTH,
			GlobalStringGet( GS_ITEMS_DROPPED_MESSAGE ),
			nNumItemsDropped);

		// generic dialog box for now
		UIShowGenericDialog(
			GlobalStringGet( GS_ITEMS_DROPPED_HEADER ),
			uszMessage,
			FALSE,
			NULL,
			NULL,
			0);

	}
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_ItemRemoveInventoryToBuffer(
	UNIT *pItem,
	INVENTORY_ITEM *pBuffer,
	int nMaxBufferSize)
{
	ASSERTX_RETZERO( pItem, "Expected unit" );
	ASSERTX_RETZERO( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );
	ASSERTX_RETZERO( pBuffer, "Expected unit buffer" );
	ASSERTX_RETZERO( nMaxBufferSize > 0, "Invalid unit buffer size" );
	GAME *pGame = UnitGetGame( pItem );
	ASSERTX_RETZERO( IS_SERVER( pGame ), "Server only" );
	int nNumRemoved = 0;
		
	// iterate the inventory of the item
	UNIT *pContained = UnitInventoryIterate( pItem, NULL );
	UNIT *pNext = NULL;
	while (pContained != NULL)
	{
	
		// get next
		pNext = UnitInventoryIterate( pItem, pContained );
		
		// must have room in buffer
		ASSERTV_RETVAL( 
			nNumRemoved < nMaxBufferSize, nNumRemoved, 
			"Buffer to remove inventory of is too small, size=%d", 
			nMaxBufferSize);
		
		// unlock
		UnitClearStat( pContained, STATS_ITEM_LOCKED_IN_INVENTORY, 0 );

		// get its location in the inventory slot
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation( pContained, &tInvLoc );
		
		// remove from inventory
		UNIT *pRemoved = UnitInventoryRemove( pContained, ITEM_ALL, 0, FALSE );
		ASSERTV( 
			pRemoved == pContained, 
			"Removed item (%s) is not the item in question (%s)", 
			UnitGetDevName( pRemoved ), 
			UnitGetDevName( pContained ));
		
		// add to buffer
		INVENTORY_ITEM *pInvItem = &pBuffer[ nNumRemoved++ ];
		pInvItem->pUnit = pContained;
		pInvItem->tInvLocation = tInvLoc;		

		// go to next item
		pContained = pNext;
				
	}

	return nNumRemoved;
				
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemReplicateSocketSlots(
	UNIT *pSource,
	UNIT *pDest)
{
	ASSERTX_RETURN( pSource, "Expected source unit" );
	ASSERTX_RETURN( UnitIsA( pSource, UNITTYPE_ITEM ), "Expected source item" );
	ASSERTX_RETURN( pDest, "Expected dest unit" );
	ASSERTX_RETURN( UnitIsA( pDest, UNITTYPE_ITEM ), "Expected dest item" );
	ASSERTX_RETURN( UnitInventoryGetCount( pDest ) == 0, "Dest item must not have an inventory" );
	
	// first, remove any slots that the dest item has
	const int MAX_SLOT_STATS = 64;  // arbitrary
	STATS_ENTRY tSlotStats[ MAX_SLOT_STATS ];
	int nNumSlotStats = UnitGetStatsRange( pDest, STATS_ITEM_SLOTS, 0, tSlotStats, MAX_SLOT_STATS );
	for (int i = 0; i < nNumSlotStats; ++i)
	{
		const STATS_ENTRY *pStat = &tSlotStats[ i ];
		int nStat = pStat->GetStat();
		ASSERTX_CONTINUE( nStat == STATS_ITEM_SLOTS, "Expected item slot stat" );
		int nParam = pStat->GetParam();
		
		// remove this entry
		UnitClearStat( pDest, nStat, nParam );
		
	}
	
	// get the stats on the source unit
	nNumSlotStats = UnitGetStatsRange( pSource, STATS_ITEM_SLOTS, 0, tSlotStats, MAX_SLOT_STATS );
	for (int i = 0; i < nNumSlotStats; ++i)
	{
		const STATS_ENTRY *pStat = &tSlotStats[ i ];
		int nStat = pStat->GetStat();
		ASSERTX_CONTINUE( nStat == STATS_ITEM_SLOTS, "Expected item slot stat" );
		int nParam = pStat->GetParam();
		
		// add the same stat to the dest unit
		UnitSetStat( pDest, nStat, nParam, pStat->value );
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ItemGetMaxPossibleLevel(
	GAME *pGame)
{
	ASSERTX_RETZERO( pGame, "Expected game" );
	return ExcelGetNumRows( pGame, DATATABLE_ITEM_LEVELS ) - 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemQualityIsEqualToOrLessThanQuality(
	int nQuality1,
	int nQuality2)
{
	ASSERT_RETFALSE (nQuality1 != INVALID_LINK &&
					 nQuality2 != INVALID_LINK);

	if (nQuality1 == nQuality2)
		return TRUE;

	const ITEM_QUALITY_DATA * pQualityData = (const ITEM_QUALITY_DATA *)ExcelGetData(NULL, DATATABLE_ITEM_QUALITY, nQuality2);
	ASSERT_RETFALSE(pQualityData);
	if (pQualityData->nDowngrade == INVALID_LINK)
		return FALSE;

	return ItemQualityIsEqualToOrLessThanQuality(nQuality1, pQualityData->nDowngrade);

}

//returns true if the item is a recipe
BOOL ItemIsRecipe( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );
	return UnitIsA( pItem, UNITTYPE_RECIPE );
}

//returns the ID of the recipe if the item is a recipe item
int ItemGetRecipeID( UNIT *pItem )
{
	ASSERT_RETINVALID( pItem );
	if( ItemIsRecipe( pItem ) )
	{
		return UnitGetStat( pItem, STATS_RECIPE_ASSIGNED );
	}
	return INVALID_ID;
}

//returns if the item should be bind'ed to the player if picked up
BOOL ItemBindsOnPickup( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );
	return ( UnitGetStat( pItem, STATS_BIND_ON_PICKUP ) == 1 || ItemBindsToLevel( pItem ) )?TRUE:FALSE;
}

//returns if the item should be binded if it's equiped
BOOL ItemBindsOnEquip( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );
	return ( UnitGetStat( pItem, STATS_BIND_ON_EQUIP ) == 1 )?TRUE:FALSE;
}

//returns if the item binds on to level area
BOOL ItemBindsToLevel( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );
	return UnitDataTestFlag(pItem->pUnitData, UNIT_DATA_FLAG_BIND_TO_LEVELAREA);
}

//returns if the item is binded to a player
BOOL ItemIsBinded( UNIT *pItem )
{
	ASSERT_RETFALSE( pItem );
	if( ItemBindsOnEquip( pItem ) ||
		ItemBindsOnPickup( pItem ) ||
		ItemBindsToLevel( pItem ) )
	{
		return ItemIsNoTrade( pItem );		
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ItemBindToPlayer( UNIT *pItem )
{
	ASSERT_RETURN( pItem );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pItem ) ) );	
	ItemSetNoTrade( pItem, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsInCanTakeLocation(	
	UNIT *pItem)
{
	ASSERTX_RETFALSE( pItem, "Expected item" );
	UNIT *pContainer = UnitGetContainer( pItem );

	// items that are not in a container cannot answer this question
	if (pContainer == NULL)
	{
		return FALSE;  // we could change our minds and return true here if it's helpful one day -Colin
	}

	// get th current inventory location	
	INVENTORY_LOCATION tInventoryLocation;
	UnitGetInventoryLocation( pItem, &tInventoryLocation );
	
	// check the location
	if (tInventoryLocation.nInvLocation != INVLOC_NONE &&
		InventoryLocPlayerCanTake( pContainer, pItem, tInventoryLocation.nInvLocation) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemRemovingFromCanTakeLocation(
	UNIT *pPlayer,
	UNIT *pItem)
{		
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pItem, "Expected unit" );
	
	// if dropping a single use recipe, close it
	if (UnitIsA( pItem, UNITTYPE_SINGLE_USE_RECIPE ))
	{
		s_RecipeForceClose( pPlayer, pItem );
	}

}

//----------------------------------------------------------------------------
//returns TRUE if it has a creators name else it returns FALSE
//----------------------------------------------------------------------------
BOOL ItemGetCreatorsName( UNIT *pItem,
						  WCHAR *uszPlayerNameReturned,
						  int nStringArraySize)
{
	ASSERT_RETFALSE(pItem);	
	ASSERTX_RETFALSE( nStringArraySize >= MAX_CHARACTER_NAME, "Array size has to be greater or eqaul to MAX_CHARACTER_NAME" );
	ZeroMemory( uszPlayerNameReturned, sizeof( WCHAR ) * nStringArraySize );	
	for( int t = 0; t < MAX_CHARACTER_NAME; t++ )
	{
		int nValue = UnitGetStat( pItem, STATS_CRAFTED_ITEM_CREATOR, t );
		if( nValue <= 0)
			return ( t == 0 )?FALSE:TRUE;
		uszPlayerNameReturned[ t ] = (WCHAR)nValue;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
// For player transaction logging. Gets human-readable info about the item
//----------------------------------------------------------------------------
BOOL ItemGetPlayerLogString(
	UNIT *pItem,
	WCHAR *szName,
	int len)
{
	static const GLOBAL_INDEX eIndices[] = {
		GI_DISPLAY_ITEM_TYPE_AND_QUALITY,
		GI_DISPLAY_ITEM_AFFIX_LIST_AND_PROPS,
		GI_DISPLAY_ITEM_MODCOUNT
	};

	ASSERT_RETFALSE( szName );
	*szName = 0;
	ASSERT_RETFALSE( pItem );
	GAME* pGame = UnitGetGame( pItem );
	ASSERT_RETFALSE( pGame );

	ASSERT_RETFALSE( UnitGetGenus( pItem ) == GENUS_ITEM );

	WCHAR wszItemString[ MAX_ITEM_PLAYER_LOG_STRING_LENGTH ];

	for (int ii = 0; ii < arrsize(eIndices); ++ii)
	{
		if ( PrintStatsLine(
				pGame,
				DATATABLE_ITEMDISPLAY,
				GlobalIndexGet( eIndices[ ii ] ),
				NULL,
				pItem,
				wszItemString,
				MAX_ITEM_PLAYER_LOG_STRING_LENGTH,
				-1,
				FALSE) && wszItemString[0] != 0 )
		{
			PStrCat( szName, wszItemString, len );
			PStrCat( szName, L"\n", len );
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemPlayerLogDataInitFromUnit(
	ITEM_PLAYER_LOG_DATA &tItemData,
	UNIT *pUnit)
{
	ASSERT_RETURN( pUnit );

	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	ASSERT_RETURN( pUnitData );

	ItemPlayerLogDataInitFromUnitData( tItemData, pUnitData );
	
	GENUS eGenus = UnitGetGenus( pUnit );
	EXCELTABLE eTable = ExcelGetDatatableByGenus( eGenus );
	tItemData.pszGenus = ExcelGetTableName( eTable );
	tItemData.pszQuality = ItemGetQualityName( pUnit );

	tItemData.guidUnit = UnitGetGuid( pUnit );

	UnitGetName( pUnit, tItemData.wszTitle, MAX_ITEM_NAME_STRING_LENGTH, 0 );
	ItemGetPlayerLogString( pUnit, tItemData.wszDescription, MAX_ITEM_PLAYER_LOG_STRING_LENGTH );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ItemPlayerLogDataInitFromUnitData(
	ITEM_PLAYER_LOG_DATA &tItemData,
	const UNIT_DATA *pUnitData)
{
	ASSERT_RETURN( pUnitData );

	tItemData.nLevel = pUnitData->nItemExperienceLevel;
	tItemData.pszDevName = pUnitData->szName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemWasCreatedByPlayer( UNIT *pItem,
							UNIT *pPlayer )
{
	ASSERT_RETFALSE( pItem );
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	if( !ItemIsACraftedItem( pItem ) )
		return FALSE;
	// get player name
	WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
	PStrCopy( uszPlayerName, L"", arrsize(uszPlayerName) );
	UnitGetName( pPlayer, uszPlayerName, arrsize(uszPlayerName), 0 );
	WCHAR uszItemsCreator[ MAX_CHARACTER_NAME ];
	ItemGetCreatorsName( pItem, uszItemsCreator, MAX_CHARACTER_NAME );	
	return PStrCmp( uszPlayerName, uszItemsCreator, MAX_CHARACTER_NAME );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemIsSpawningForGambler(
	const ITEM_SPEC * spec)
{	
	if( spec )
	{

		return (!TESTBIT(spec->dwFlags, ISF_IDENTIFY_BIT) &&
			spec && 
			((spec->pMerchant && UnitIsGambler(spec->pMerchant)) ||
			UnitIsGambler(spec->pSpawner)));
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL ItemUsesWardrobe( const UNIT_DATA * pItemUnitData )
{
	ASSERT_RETFALSE( pItemUnitData );

	BOOL bWardrobe = FALSE;
	if ( pItemUnitData->nInventoryWardrobeLayer != INVALID_ID )
	{
		if ( UnitDataTestFlag( pItemUnitData, UNIT_DATA_FLAG_DRAW_USING_CUT_UP_WARDROBE ) )
			bWardrobe = TRUE;
		//if ( UnitDataTestFlag( pItemUnitData, UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK ) )
		//	bWardrobe = FALSE;
	}

	return bWardrobe;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT ItemUnitDataGetIconFilename(
	OS_PATH_CHAR * poszFilename,
	int nBufLen,
	const UNIT_DATA * pItemUnitData,
	GENDER eGender /*= GENDER_INVALID*/ )
{
	ASSERT_RETINVALIDARG( poszFilename );
	ASSERT_RETINVALIDARG( pItemUnitData );

	OS_PATH_CHAR oszFilename[ DEFAULT_FILE_SIZE ];
	PStrCvt( oszFilename, pItemUnitData->szName, DEFAULT_FILE_SIZE );

	OS_PATH_CHAR oszGender[ 8 ] = OS_PATH_TEXT("");
	if ( ItemUsesWardrobe( pItemUnitData ) && ! UnitDataTestFlag( pItemUnitData, UNIT_DATA_FLAG_NO_GENDER_FOR_ICON ) )
	{
		ASSERT_RETINVALIDARG( eGender == GENDER_MALE || eGender == GENDER_FEMALE );
		PStrPrintf( oszGender, arrsize(oszGender), OS_PATH_TEXT("%s"), eGender == GENDER_MALE ? OS_PATH_TEXT("_m") : OS_PATH_TEXT("_f") );
	}
	//OS_PATH_CHAR oszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( poszFilename, nBufLen, OS_PATH_TEXT("%s%s%s.%s"), OS_PATH_TEXT(ICON_SUBDIRECTORY_STR), oszFilename, oszGender, OS_PATH_TEXT("dds") );

	return S_OK;
}

