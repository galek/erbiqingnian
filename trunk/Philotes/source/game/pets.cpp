//----------------------------------------------------------------------------
// FILE: pets.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "prime.h"
#include "clients.h"
#include "combat.h"
#include "game.h"
#include "inventory.h"
#include "unit_priv.h"
#include "monsters.h"
#include "pets.h"
#include "skills.h"
#include "states.h"
#include "stats.h"
#include "s_message.h"
#include "gameunits.h"
#include "c_appearance.h"
#include "c_animation.h"
#include "unitmodes.h"
#include "unittypes.h"
#include "player.h"
#include "ai.h"
#include "movement.h"
#include "room_path.h"
#include "teams.h"
#include "e_model.h"
#include "achievements.h"
//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
struct PetOwnerEvents
{
	UNIT_EVENT eEvent;
	FP_UNIT_EVENT_HANDLER fpHandler;
	int nFlag;
};

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetDeathToOwnerDeath(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );
	
	if ( pPet && UnitGetRoom( pPet ) )// they won't have a room if they were removed from the world to be resurrected later
	{
		UnitDie(pPet, NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetDeathToOwnerForgettingSkill(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );
	int *pRequiredSkillIndex = (int *)pData;
	ASSERT_RETURN(pRequiredSkillIndex);

	if ( pPet && UnitGetStat(pPet, STATS_PET_REQUIRED_OWNER_SKILL) == *pRequiredSkillIndex )
	{
		UnitDie(pPet, NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetDestroyToOwnerWarpedLevels(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );

	if (pPet)
	{
		UnitFree( pPet, UFF_SEND_TO_CLIENTS );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetLimboToOwnerLimboEnter(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );

	if ( pPet )
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )
		if( UnitDataTestFlag( UnitGetData( pPet ), UNIT_DATA_FLAG_PET_ON_WARP_DESTROY ) ) 
		{
			UnitDie(pPet, NULL);
		}
		else
		{
			s_UnitPutInLimbo( pPet, FALSE ); 
		}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetLimboToOwnerLimboExit(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );
	
	if ( pPet )
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )
		if ( UnitTestMode( pPet, MODE_DEAD ) && UnitTestFlag( pPet, UNITFLAG_ON_DIE_HIDE_MODEL ) )
		{
			e_ModelSetDrawAndShadow( c_UnitGetModelId( pPet ), FALSE );
			s_UnitTakeOutOfLimbo( pPet );
		}
		else
		{
			s_UnitSetMode(pPet, MODE_IDLE);
			s_UnitTakeOutOfLimbo( pPet );
			AI_Register( pPet, TRUE, INVALID_ID );
		}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	}
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PET_WARP_RADIUS (7.0f)
static void sLinkPetWarpedPosition(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );

	if ( pPet )
	{
		VECTOR vPosition = UnitGetPosition(pUnit);
		VECTOR vDestination;

		ROOM * pDestinationRoom = s_LevelGetRandomPositionAround( pGame, UnitGetLevel( pUnit ), vPosition, PET_WARP_RADIUS, vDestination );
		if( pDestinationRoom )
		{
			DWORD dwWarpFlags = 0;	
			if ( !UnitTestMode( pPet, MODE_DEAD ) || !UnitTestFlag( pPet, UNITFLAG_ON_DIE_HIDE_MODEL ) ) 
			{
				SETBIT( dwWarpFlags, UW_FORCE_VISIBLE_BIT );
			}

			UnitWarp( 
				pPet, 
				pDestinationRoom, 
				vDestination, 
				UnitGetFaceDirection(pUnit, FALSE), 
				VECTOR( 0, 0, 1 ), 
				dwWarpFlags, 
				TRUE );

			// stop all skills
			SkillStopAll( pGame, pUnit );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetDoLevelUp(
	GAME * pGame,
	UNIT * pOwner,
	UNIT * pPet)
{
	REF(pGame);
	REF(pOwner);
	REF(pPet);

#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(IS_SERVER(pGame));
	ASSERT_RETURN(pOwner);
	ASSERT_RETURN(pPet);

	MONSTER_SPEC tSpec;
	MonsterSpecInit( tSpec );
	tSpec.nExperienceLevel = UnitGetExperienceLevel( pOwner );
	tSpec.nClass = UnitGetClass( pPet );
	tSpec.nMonsterQuality = MonsterGetQuality( pPet );
	int nOldLevel = MAX( 0, UnitGetExperienceLevel( pPet ) );
	int nNewLevel = tSpec.nExperienceLevel;
	REF(nOldLevel);
	REF(nNewLevel);

	if ( AppIsTugboat() )
	{
		tSpec.dwFlags = MSF_IS_LEVELUP;
		s_MonsterInitStats( pGame, pPet, tSpec );
	}

	// do what s_MonsterInitStats does, except don't evaluate starting properties again, and don't bother with monster quality stuff
	if ( AppIsHellgate() )
	{
		int nClass = tSpec.nClass;
		int nExperienceLevel = tSpec.nExperienceLevel;
		const UNIT_DATA * monster_data = MonsterGetData(pGame, UnitGetClass(pPet));

#ifdef _DEBUG
		if (pGame->m_nDebugMonsterLevel != 0)
		{
			nExperienceLevel = pGame->m_nDebugMonsterLevel;
		}
#endif
		nExperienceLevel = MAX(nExperienceLevel, monster_data->nMinMonsterExperienceLevel);

		UnitSetExperienceLevel(pPet, nExperienceLevel);

		nExperienceLevel = UnitGetExperienceLevel( pPet );

		const MONSTER_LEVEL * monster_level = (MONSTER_LEVEL *)ExcelGetData(pGame, DATATABLE_PETLEVEL, nExperienceLevel);
		if (!monster_level)
		{
			return;
		}

		// hitpoints
		int hp;

		int min_hp = monster_level->nHitPoints * monster_data->nMinHitPointPct / 100;
		int max_hp = monster_level->nHitPoints * monster_data->nMaxHitPointPct / 100;
		hp = RandGetNum(pGame, min_hp, max_hp);	

		UnitSetStatShift(pPet, STATS_HP_MAX, hp);

		// experience
		int experience = PCT(monster_level->nExperience, monster_data->nExperiencePct);
		UnitSetStatShift(pPet, STATS_EXPERIENCE, experience);


		// attack rating
		int attack_rating = PCT(monster_level->nAttackRating, monster_data->nAttackRatingPct);
		UnitSetStatShift(pPet, STATS_ATTACK_RATING, attack_rating);

		// damage
		int min_dmg_pct = ExcelEvalScript(pGame, pPet, NULL, NULL, DATATABLE_MONSTERS, OFFSET(UNIT_DATA, codeMinBaseDmg), nClass);
		int max_dmg_pct = ExcelEvalScript(pGame, pPet, NULL, NULL, DATATABLE_MONSTERS, OFFSET(UNIT_DATA, codeMaxBaseDmg), nClass);
		if ( min_dmg_pct || max_dmg_pct )
		{
			int min_damage = PCT((monster_level->nDamage << StatsGetShift(pGame, STATS_BASE_DMG_MIN)), min_dmg_pct);
			UnitSetStat(pPet, STATS_BASE_DMG_MIN, min_damage);

			int max_damage = PCT((monster_level->nDamage << StatsGetShift(pGame, STATS_BASE_DMG_MAX)), max_dmg_pct);
			UnitSetStat(pPet, STATS_BASE_DMG_MAX, max_damage);

			if (monster_data->nDamageType > 0)
			{
				UnitSetStatShift(pPet, STATS_DAMAGE_MIN, monster_data->nDamageType, 100);
				UnitSetStatShift(pPet, STATS_DAMAGE_MAX, monster_data->nDamageType, 100);								
			}
		}

		for (int ii = 0; ii < NUM_DAMAGE_TYPES; ii++)
		{
			DAMAGE_TYPES eDamageType = (DAMAGE_TYPES)ii;

			// armor
			if (monster_data->fArmor[eDamageType] > 0.0f)
			{
				int armor = PCT(monster_level->nArmor << StatsGetShift(pGame, STATS_ARMOR), monster_data->fArmor[eDamageType]);
				int armor_buffer = PCT(monster_level->nArmorBuffer << StatsGetShift(pGame, STATS_ARMOR_BUFFER_MAX), monster_data->fArmor[eDamageType]);
				int armor_regen = PCT(monster_level->nArmorRegen << StatsGetShift(pGame, STATS_ARMOR_BUFFER_REGEN), monster_data->fArmor[eDamageType]);
				UnitSetStat(pPet, STATS_ARMOR, eDamageType, armor);
				UnitSetStat(pPet, STATS_ARMOR_BUFFER_MAX, eDamageType, armor_buffer);
				UnitSetStat(pPet, STATS_ARMOR_BUFFER_CUR, eDamageType, armor_buffer);
				UnitSetStat(pPet, STATS_ARMOR_BUFFER_REGEN, eDamageType, armor_regen);
			} 
			else 
			{
				if( AppIsHellgate() )
				{
					UnitSetStat(pPet, STATS_ELEMENTAL_VULNERABILITY, eDamageType, int(monster_data->fArmor[eDamageType]));
				}
			}

			// shields
			if ( monster_data->fShields[eDamageType] > 0.0f)
			{
				int shield = PCT(monster_level->nShield << StatsGetShift(pGame, STATS_SHIELDS), monster_data->fShields[eDamageType]);
				int shield_buffer = PCT(monster_level->nShield << StatsGetShift(pGame, STATS_SHIELD_BUFFER_MAX), monster_data->fShields[eDamageType]);
				//int shield_buffer = PCT(monster_level->nShieldBuffer << StatsGetShift(pGame, STATS_SHIELD_BUFFER_MAX), monster_data->nShields[eDamageType]);
				int shield_regen = monster_level->nShieldRegen << StatsGetShift(pGame, STATS_SHIELD_BUFFER_REGEN);
				UnitSetStat(pPet, STATS_SHIELDS, shield);
				UnitSetStat(pPet, STATS_SHIELD_BUFFER_MAX, shield_buffer);
				UnitSetStat(pPet, STATS_SHIELD_BUFFER_CUR, shield_buffer);
				UnitSetStat(pPet, STATS_SHIELD_BUFFER_REGEN, shield_regen);
			}

			// defense against special effects
			const SPECIAL_EFFECT *pSpecialEffect = &monster_data->tSpecialEffect[ eDamageType ];		
			int nSfxDefense = PCT( monster_level->nSfxDefense, pSpecialEffect->nDefensePercent ); 
			UnitSetStat( pPet, STATS_SFX_DEFENSE_BONUS, eDamageType, nSfxDefense );

			int sfx_ability = PCT(monster_level->nSfxAttack, pSpecialEffect->nAbilityPercent);
			if ( sfx_ability )
				UnitSetStat(pPet, STATS_SFX_ATTACK, eDamageType, sfx_ability);

			if ( pSpecialEffect->nStrengthPercent )
				UnitSetStat( pPet, STATS_SFX_STRENGTH_PCT, eDamageType, pSpecialEffect->nStrengthPercent );

			if ( pSpecialEffect->nDurationInTicks )
				UnitSetStat( pPet, STATS_SFX_DURATION_IN_TICKS, eDamageType, pSpecialEffect->nDurationInTicks );

		}

		int nInterruptAttack = StatsRoundStatPct(pGame, STATS_INTERRUPT_ATTACK, monster_level->nInterruptAttack, monster_data->nInterruptAttackPct);
		UnitSetStatShift(pPet, STATS_INTERRUPT_ATTACK,	nInterruptAttack);
		int nInterruptDefense = StatsRoundStatPct(pGame, STATS_INTERRUPT_DEFENSE, monster_level->nInterruptDefense, monster_data->nInterruptDefensePct);
		UnitSetStatShift(pPet, STATS_INTERRUPT_DEFENSE,	nInterruptDefense);

		int nStealthDefense = StatsRoundStatPct(pGame, STATS_STEALTH_DEFENSE, monster_level->nStealthDefense, monster_data->nStealthDefensePct);
		UnitSetStatShift(pPet, STATS_STEALTH_DEFENSE,	nStealthDefense);

		int nAIChangeDefense = PCT(monster_level->nAIChangeDefense, monster_data->nAIChangeDefense);
		if(nAIChangeDefense)
		{
			UnitSetStat(pPet, STATS_AI_CHANGE_DEFENSE, nAIChangeDefense);
		}

		// corpse explode points
		int nCorpseExplodePoints = PCT(monster_level->nCorpseExplodePoints, monster_data->nCorpseExplodePoints);
		if(nCorpseExplodePoints > 0)
		{
			UnitSetStat(pPet, STATS_CORPSE_EXPLODE_POINTS, nCorpseExplodePoints);
		}

		// tohit
		if (monster_data->nToHitBonus > 0)
		{
			int tohit = PCT(monster_level->nToHitBonus << StatsGetShift(pGame, STATS_TOHIT), monster_data->nToHitBonus);
			UnitSetStat(pPet, STATS_TOHIT, tohit );
		} 

		// set treasure for monster 
		int nTreasure = monster_data->nTreasure;

		UnitSetStat(pPet, STATS_TREASURE_CLASS, nTreasure);

		for (int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++)
		{
			if (monster_data->nStartingSkills[ii] != INVALID_ID)
			{
				UnitSetStat(pPet, STATS_SKILL_LEVEL, monster_data->nStartingSkills[ii], 1);
			}
		}
		if (monster_data->nStartingSkills[0] != INVALID_ID)
		{
			UnitSetStat(pPet, STATS_AI_SKILL_PRIMARY, monster_data->nStartingSkills[0]);
		}

		UnitSetStat(pPet, STATS_AI_FLAGS, 0);

		// apply any hand picked affixes
		if (IS_SERVER( pGame ))
		{
			for (int i = 0; i < AffixGetMaxAffixCount(); ++i)
			{
				int nAffix = monster_data->nAffixes[ i ];
				if (nAffix != INVALID_LINK)
				{
					s_AffixApply( pPet, nAffix );
				}			
			}
		}

		if ( UnitDataTestFlag( monster_data, UNIT_DATA_FLAG_PET_GETS_STAT_POINTS_PER_LEVEL ) )
		{
			while ( nOldLevel < nNewLevel )
			{
				const MONSTER_LEVEL *pMonsterLevel = (MONSTER_LEVEL *) ExcelGetData( NULL, DATATABLE_MONLEVEL, nOldLevel + 1 );
				if ( !pMonsterLevel )
				{
					break;
				}

				// add stats
				UnitChangeStatShift( pPet, STATS_STAT_POINTS, pMonsterLevel->nStatPoints );
				nOldLevel++;
			}
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkPetLevelUp(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	if ( IS_CLIENT( pUnit ) )
		return;

#if !ISVERSION( CLIENT_ONLY_VERSION )
	UNIT *pPet = UnitGetById( pGame, (UNITID)pEventData->m_Data1 );
	if ( ! pPet )
		return;

	sPetDoLevelUp(pGame, pUnit, pPet);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePetFromInventoryDo(
	GAME * pGame,
	UNIT * pPet);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkRemovePetFromInventory(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UNITID idPet = (UNITID)pEventData->m_Data1;
	UNIT * pPet = UnitGetById(pGame, idPet);
	if(pPet)
	{
		sRemovePetFromInventoryDo(pGame, pPet);
	}
}

//----------------------------------------------------------------------------
static const PetOwnerEvents sgpPetOwnerEvents[] =
{
	{ EVENT_UNITDIE_BEGIN,	sLinkPetDeathToOwnerDeath,		INVLOCFLAG_LINKDEATHS	},
	{ EVENT_ENTER_LIMBO,	sLinkPetLimboToOwnerLimboEnter,	-1						},
	{ EVENT_EXIT_LIMBO,		sLinkPetLimboToOwnerLimboExit,	-1						},
	{ EVENT_WARPED_POSITION,sLinkPetWarpedPosition,			-1						},
	{ EVENT_WARPED_LEVELS,	sLinkPetDestroyToOwnerWarpedLevels,	INVLOCFLAG_DESTROY_PET_ON_LEVEL_CHANGE	},
	{ EVENT_LEVEL_UP,		sLinkPetLevelUp,				-1						},
	{ EVENT_UNITDIE_BEGIN,	sLinkRemovePetFromInventory,	INVLOCFLAG_REMOVE_FROM_INVENTORY_ON_OWNER_DEATH	},
	{ EVENT_ON_FREED,		sLinkRemovePetFromInventory,	INVLOCFLAG_REMOVE_FROM_INVENTORY_ON_OWNER_DEATH	},
	{ EVENT_SKILL_FORGOTTEN,sLinkPetDeathToOwnerForgettingSkill,	-1				},
};

static const int sgnPetOwnerEventsSize = sizeof(sgpPetOwnerEvents) / sizeof(PetOwnerEvents);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePetFromInventoryHandler(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	sRemovePetFromInventoryDo(pGame, pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePetFromInventoryDo(
	GAME * pGame,
	UNIT * pPet)
{
	UnitInventoryRemove(pPet, ITEM_ALL);

	UnitUnregisterEventHandler(pGame, pPet, EVENT_UNITDIE_BEGIN, sRemovePetFromInventoryHandler);

	UNITID idOwner = UnitGetStat(pPet, STATS_PET_OWNER_UNITID);
	UNIT * pOwner = UnitGetById(pGame, idOwner);

	// It's possible that the owner has been freed.
	if(pOwner)
	{
		// make sure to reduce current power when refunding max power
		int nOldPowerCur = UnitGetStat(pOwner, STATS_POWER_CUR );
		s_StateClear(pOwner, UnitGetId(pPet), STATE_PET_POWER_COST, 0);
		UnitSetStat(pOwner, STATS_POWER_CUR, 0, nOldPowerCur);

		GAMECLIENT *pClient = UnitGetClient( pOwner );
		if (pClient)
		{	
			MSG_SCMD_INVENTORY_REMOVE_PET msg;
			msg.idOwner = idOwner;
			msg.idPet = UnitGetId(pPet);
			s_SendMessage( pGame, ClientGetId( pClient ), SCMD_INVENTORY_REMOVE_PET, &msg );
		}

		for(int i = 0; i<sgnPetOwnerEventsSize ; i++)
		{
			DWORD dwEventId = UnitGetStat(pPet, STATS_PET_OWNER_CALLBACK_IDS, sgpPetOwnerEvents[i].eEvent);
			UnitUnregisterEventHandler(pGame, pOwner, sgpPetOwnerEvents[i].eEvent, dwEventId);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PetGetPetMaxPowerPenalty(
	UNIT * pOwner,
	UNIT * pPet)
{
	int nPetPowerCost = MAX(UnitGetStatShift(pPet, STATS_POWER_MAX_PENALTY_PETS), 0);
	if(pOwner)
	{
		int nPowerCostReductionPercent = UnitGetStat(pOwner, STATS_PET_POWER_COST_REDUCTION_PCT);
		ASSERT_RETVAL(nPowerCostReductionPercent >= 0 && nPowerCostReductionPercent < 100, nPetPowerCost);
		int nPowerCostPercent = 100 - nPowerCostReductionPercent;

		if(nPowerCostPercent < 100)
		{
			nPetPowerCost = PCT(nPetPowerCost, nPowerCostPercent);
		}
	}
	return nPetPowerCost;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetApplyPowerCostToOwner(
	UNIT * pPet,
	UNIT * pOwner,
	int nPowerCost)
{
	if (AppIsHellgate())
	{
		STATS * pPowerCostStats = s_StateSet(pOwner, pPet, STATE_PET_POWER_COST, 0);
		// minor pet power max reduction goes in a separate stat for hellgate,
		// since minor pets can be sacrificed to undo their power max reduction
		// and make way for newer pets
		if (UnitGetType(pPet) != UNITTYPE_MINOR_PET)
			StatsSetStat(UnitGetGame(pOwner), pPowerCostStats, STATS_POWER_MAX_BONUS, -nPowerCost);
		else
			StatsSetStat(UnitGetGame(pOwner), pPowerCostStats, STATS_POWER_MAX_PENALTY_PETS, -nPowerCost);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendPetMessage(
	GAME * game,
	UNIT * owner,
	UNIT * pet)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(owner);
	ASSERT_RETURN(pet);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (!IS_SERVER(game))
	{
		return;
	}
	UNIT * container = UnitGetContainer(pet);
	if (!container)
	{
		return;
	}
	if (!UnitHasClient(container))
	{
		return;
	}

	MSG_SCMD_INVENTORY_ADD_PET msg;
	if (!UnitGetInventoryLocation(pet, &msg.tLocation))
	{
		return;
	}
	ASSERT_RETURN(msg.tLocation.nInvLocation != INVALID_LINK);

	msg.idOwner = UnitGetId(container);
	msg.idPet = UnitGetId(pet);
	s_SendMessage(game, UnitGetClientId(container), SCMD_INVENTORY_ADD_PET, &msg);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ACCEPTABLE_DURATION_UPDATE_RATE 3
#define ACCEPTABLE_DURATION_WITH_NO_AI_UPDATES (ACCEPTABLE_DURATION_UPDATE_RATE*GAME_TICKS_PER_SECOND)
static BOOL sPetCheckAIIsRunning(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	if(!UnitHasTimedEvent(unit, AI_Update))
	{
		GAME_TICK tiLastUpdateTick = UnitGetStat(unit, STATS_LAST_AI_UPDATE_TICK);
		if(GameGetTick(game) - tiLastUpdateTick >= ACCEPTABLE_DURATION_WITH_NO_AI_UPDATES)
		{
			AI_Register(unit, FALSE, 1);
		}
	}
	UnitRegisterEventTimed(unit, sPetCheckAIIsRunning, event_data, ACCEPTABLE_DURATION_UPDATE_RATE*GAME_TICKS_PER_SECOND);
	return TRUE;
}


//----------------------------------------------------------------------------
// Returns FALSE if the add fails.  We should check the return and free
// the pet where appropriate.
//----------------------------------------------------------------------------
BOOL PetAdd(
	UNIT *pPet,
	UNIT *pOwner,
	int nInventoryLocation,
	int nRequiredOwnerSkill,
	int nOwnerMaxPowerPenalty,
	BOOL bAddToInventory /*=TRUE*/)
{			
	ASSERTX_RETFALSE( pPet, "Expected pet" );
	ASSERTX_RETFALSE( pOwner, "Expected owner" );
	GAME *pGame = UnitGetGame( pPet );

	// do server logic	
	if (IS_SERVER( pGame ))
	{
		ASSERTX_RETFALSE( PetIsPet( pPet ) == FALSE, "Pet is already a pet" );

		const INVLOC_DATA * pInvLoc = UnitGetInvLocData(pOwner, nInventoryLocation);
		ASSERTX_RETFALSE(pInvLoc, "Invalid inventory location for unit");

		if(UnitIsA(pOwner, UNITTYPE_PLAYER) && (UnitIsA(pPet, UNITTYPE_MAJOR_PET)))
		{
			// give pet a name
			MonsterProperNameSet( pPet, MonsterProperNamePick( pGame, UnitGetClass( pPet ) ), TRUE );
		}

		// setup events
		if ( TESTBIT( pInvLoc->dwFlags, INVLOCFLAG_RESURRECTABLE ) )
		{
			// we don't want this thing destroyed when it dies
			UnitSetFlag( pPet, UNITFLAG_DESTROY_DEAD_NEVER ); 
		}
		else
		{
			UnitRegisterEventHandler( pGame, pPet, EVENT_UNITDIE_BEGIN, sRemovePetFromInventoryHandler, NULL );
		}

		for(int i = 0; i<sgnPetOwnerEventsSize ; i++)
		{
			if(sgpPetOwnerEvents[i].nFlag < 0 || InvLocTestFlag(pInvLoc, sgpPetOwnerEvents[i].nFlag))
			{
				DWORD dwEventId;
				UnitRegisterEventHandlerRet( dwEventId, pGame, pOwner, sgpPetOwnerEvents[i].eEvent, sgpPetOwnerEvents[i].fpHandler, &EVENT_DATA( UnitGetId( pPet ) ) );
				UnitSetStat( pPet, STATS_PET_OWNER_CALLBACK_IDS, sgpPetOwnerEvents[i].eEvent, dwEventId);
			}
		}
		UnitRegisterEventTimed(pPet, sPetCheckAIIsRunning, EVENT_DATA(), ACCEPTABLE_DURATION_UPDATE_RATE*GAME_TICKS_PER_SECOND);

		int nPetExperienceLevel = UnitGetExperienceLevel(pPet);
		int nOwnerExperienceLevel = UnitGetExperienceLevel(pOwner);
		if(nPetExperienceLevel < nOwnerExperienceLevel)
		{
			int nExperienceLevelsToGive = nOwnerExperienceLevel - nPetExperienceLevel;
			for(int i=0; i<nExperienceLevelsToGive; i++)
			{
				sPetDoLevelUp(pGame, pOwner, pPet);
			}
		}

		// set stat for pet that we can check
		UNITID idOwner = UnitGetId( pOwner );
		UnitSetStat( pPet, STATS_PET_OWNER_UNITID, idOwner );

		UnitSetStat( pPet, STATS_PET_REQUIRED_OWNER_SKILL, nRequiredOwnerSkill );

		UNIT * container = UnitGetContainer(pPet);

		BOOL bAdded = TRUE;
		if (nInventoryLocation != INVALID_LINK && container != pOwner)
		{
			sPetApplyPowerCostToOwner(pPet, pOwner, nOwnerMaxPowerPenalty);

			if( bAddToInventory )
			{
				bAdded = UnitInventoryAdd(INV_CMD_ADD_PET, pOwner, pPet, nInventoryLocation);
				ASSERTX(bAdded, "Pet failed to be added to inventory!  Check the inventory location, unit type or inventory space.");			
			}
		}
		else if (container == pOwner && nInventoryLocation != INVALID_LINK)
		{
			// send add pet message to client (used for saved pets we already have in our inventory)
			s_SendPetMessage(pGame, pOwner, pPet);
		}

		// this should come after adding to the inventory - that way the state code knows who owns it
		s_StateSet( pPet, pOwner, STATE_PET, 0 );	

		if ( UnitIsA( pPet, UNITTYPE_MAJOR_PET ))
		{
			s_StateSet( pPet, pPet, STATE_MAJOR_PET, 0 );
		}
		
		// tell the owner client they have a pet now
		if (pOwner)
		{
			// add pets to the owner's team
			TeamAddUnit( pPet, pOwner->nTeam );
			
			//send update to achievements if the owner is a player
			if(UnitIsPlayer(pOwner))
			{
				s_AchievementsSendSummon(pPet,pOwner);
			}
		}
		return bAdded;
	}
	else
	{
		return TRUE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PetIsPet(
	UNIT *pUnit)
{
	return UnitGetStat( pUnit, STATS_PET_OWNER_UNITID ) != INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PetIsPlayerPet(
	GAME* pGame,
	UNIT *pUnit)
{
	if( !pUnit )
	{
		return FALSE;
	}
	UNITID Owner = PetGetOwner( pUnit );
	if( Owner == INVALID_ID )
	{
		return FALSE;
	}
	UNIT* pOwner = UnitGetById( pGame, Owner );
	return( pOwner ? PetIsPet( pUnit ) && UnitIsA( pOwner, UNITTYPE_PLAYER ) : FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID PetGetOwner(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	if (PetIsPet( pUnit ))
	{
		return UnitGetStat( pUnit, STATS_PET_OWNER_UNITID );
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PetIteratePets(
	UNIT *pOwner,
	PFN_PET_CALLBACK pfnCallback,
	void *pUserData)
{
	ASSERTX_RETZERO( pOwner, "Expected unit" )
	ASSERTX_RETZERO( pfnCallback, "Expected callback" );
	int nNumPetsIterated = 0;
	
	UNIT *pItem = UnitInventoryIterate( pOwner, NULL );
	UNIT *pNext = NULL;
	while (pItem)
	{
		pNext = UnitInventoryIterate( pOwner, pItem );
		if (PetIsPet( pItem ))
		{
			pfnCallback( pItem, pUserData );
			nNumPetsIterated++;
		}
		pItem = pNext;
	}

	return nNumPetsIterated;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPetCanAddPowerCost(
	UNIT * pOwner,
	int nPetPowerCost)
{
	if (AppIsHellgate() && nPetPowerCost > 0)
	{
		int nPowerMax = UnitGetStat(pOwner, STATS_POWER_MAX);
		int nPowerMaxPenaltyPets = UnitGetStat(pOwner, STATS_POWER_MAX_PENALTY_PETS);

		if(nPetPowerCost > nPowerMax - nPowerMaxPenaltyPets)
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PetCanAdd(
	UNIT * pOwner,
	int nPowerCost,
	int nInventoryLocation)
{
	if (AppIsTugboat())
	{
		if(nInventoryLocation >= 0)
		{
			int nMaxCount = UnitInventoryGetArea(pOwner, nInventoryLocation);
			if(nMaxCount <= 0)
				return FALSE;

			int nCurrentCount = UnitInventoryGetCount(pOwner, nInventoryLocation);
			if(nCurrentCount >= nMaxCount)
				return FALSE;
		}
	}

	if(!sPetCanAddPowerCost(pOwner, nPowerCost))
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetReapplyPowerCostToOwner(
	UNIT *pPet,
	void *pUserData )
{
	UNIT * pOwner = (UNIT*)pUserData;

	int nMaxPowerPenalty = PetGetPetMaxPowerPenalty(pOwner, pPet);
	if(!sPetCanAddPowerCost(pOwner, nMaxPowerPenalty))
	{
		UnitDie(pPet, NULL);
	}
	else
	{
		sPetApplyPowerCostToOwner(pPet, pOwner, nMaxPowerPenalty);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PetsReapplyPowerCostToOwner(
	UNIT * pOwner)
{
	StateClearAllOfType(pOwner, STATE_PET_POWER_COST);

	PetIteratePets(pOwner, sPetReapplyPowerCostToOwner, pOwner);
}
