//----------------------------------------------------------------------------
// combat.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "excel.h"
#include "gameglobals.h"
#include "combat.h"
#include "player.h"
#include "monsters.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "clients.h"
#include "console.h"
#include "movement.h"
#include "states.h"
#include "unitmodes.h"
#include "s_message.h"
#include "s_quests.h"
#include "tasks.h"
#include "customgame.h"
#include "ctf.h"
#include "c_camera.h"
#include "uix.h"
#include "QuestObjectSpawn.h"
#include "items.h"
#include "unittag.h"
#include "treasure.h"
#include "teams.h"
#include "party.h"
#include "script.h"
#include "skills.h"
#include "missiles.h"
#include "room_path.h"
#include "level.h"
#include "e_model.h"
#include "excel_private.h"
#include "npc.h"
#include "ai.h"
#include "achievements.h"
#include "gamevariant.h"
#include "globalindex.h"
#if ISVERSION(SERVER_VERSION)
#include "combat.cpp.tmh"
#include "svrstd.h"
#include "s_partyCache.h"
#include <string>
#endif //SERVER_VERSION


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
int gnSfxChanceOverride =					-1;
#endif

#if ISVERSION(CHEATS_ENABLED)
static int sgnCritProbabilityOverride =		-1;
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
// constants for BLOCK and EVADE
#define PARAM_AVOIDANCE_MELEE				1
#define PARAM_AVOIDANCE_MISSILE				2
#define PARAM_AVOIDANCE_ALL					0
#define DAMAGES_SIZE						64

//----------------------------------------------------------------------------
enum COMBAT_FLAGS
{
	COMBAT_BASE_DAMAGES =					MAKE_MASK(0),
	COMBAT_ATTACKER_ANTIEVADE =				MAKE_MASK(1),
	COMBAT_DEFENDER_EVADE =					MAKE_MASK(2),
	COMBAT_ATTACKER_ANTIBLOCK =				MAKE_MASK(3),
	COMBAT_DEFENDER_BLOCK =					MAKE_MASK(4),
	COMBAT_ATTACKER_TOHIT =					MAKE_MASK(5),
	COMBAT_DEFENDER_ARMORCLASS =			MAKE_MASK(6),
	COMBAT_ALLOW_ATTACK_SELF =				MAKE_MASK(7),
	COMBAT_ATTACKER_LEVEL =					MAKE_MASK(8),
	COMBAT_DEFENDER_LEVEL =					MAKE_MASK(9),
	COMBAT_WEAPON_ENERGY_INCREMENT =		MAKE_MASK(10),
	COMBAT_DONT_ALLOW_DAMAGE_EFFECTS =		MAKE_MASK(11),
	COMBAT_DUALWEAPON_MELEE_INCREMENT =		MAKE_MASK(12),
	COMBAT_DUALWEAPON_FOCUS_INCREMENT =		MAKE_MASK(13),
	COMBAT_IS_THORNS =						MAKE_MASK(14),
	COMBAT_ALLOW_ATTACK_FRIENDS =			MAKE_MASK(15),
	COMBAT_IS_PVP =							MAKE_MASK(16)
};

//----------------------------------------------------------------------------
struct COMBAT
{
	DWORD					flags;
	GAME *					game;
	const MONSTER_SCALING *	monscaling;
	UNIT *					attacker;
	UNIT *					defender;
	UNIT *					attacker_ultimate_source;
	UNIT *					defender_ultimate_source;	
	UNIT *					weapons[MAX_WEAPONS_PER_UNIT];
	COMBAT *				next;									// The GAME stores a stack of COMBAT structures so that we can always ask about the current COMBAT
	int						attacker_level;
	int						defender_level;
	int						attack_rating_base;
	int						attack_rating;
	int						delivery_type;
	int						base_min;
	int						base_max;
	int						damage_increment;
	int						damage_increment_pct;					// modify damage_increment by a percent (used to override w/ damage_increment_radial, etc.
	int						damage_increment_energy;				// damage increment based on weapon energy
	int						damage_increment_dualweapon_melee;		// damage increment due to using dual melee weapons
	int						damage_increment_dualweapon_focus;		// damage increment due to using dual focus items
	int						weapon_energy_max;
	int						weapon_energy_cur;
	DWORD					dwUnitAttackUnitFlags;
	BOOL					bIgnoreMissileTag;

	int						nSoftHitState;
	int						nMediumHitState;
	int						nBigHitState;
	int						nCritHitState;
	int						nFumbleHitState;

	int						interrupt_chance;
	int						knockback_chance;
	int						result;

	int						evade;
	int						anti_evade;
	int						block;
	int						anti_block;
	int						tohit;
	int						armorclass;

	// ranged combat values
	struct ROOM *			center_room;
	VECTOR					center_point;
	float					max_radius;
	float					max_radius_sq;
	unsigned int			TARGET_MASK;

	// used only in hellgate
	const HAVOK_IMPACT *	pImpact;

	// damage multiplier added from tugboat
	float					damagemultiplier;
	int						damagetypeoverride;
};


//----------------------------------------------------------------------------
// FORWARD DECLARATION
//----------------------------------------------------------------------------
void s_UnitAttackUnit(
	COMBAT & combat,
	STATS * stats);

static inline void UnitAttackUnitSendDamageMessage(
	COMBAT & combat,
	int hp);


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// EXCEL Post Process Function
//----------------------------------------------------------------------------
BOOL LevelScalingExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	table->m_nDataExRowSize = sizeof(int);
	table->m_nDataExCount = 1;
	table->m_DataEx = (BYTE *)MALLOCZ(NULL, table->m_nDataExRowSize * table->m_nDataExCount);
	int * pnLevelScalingMin = (int *)table->m_DataEx;

	BOOL bFoundFirst = FALSE;
	int nPrevLevelDiff = 0;

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		LEVEL_SCALING_DATA * level_scaling_data = (LEVEL_SCALING_DATA *)ExcelGetDataPrivate(table, ii);
		if (!level_scaling_data)
		{
			continue;
		}

		if (!bFoundFirst)
		{
			*pnLevelScalingMin = level_scaling_data->nLevelDiff;
			ASSERT(*pnLevelScalingMin <= 0);
			nPrevLevelDiff = *pnLevelScalingMin;
			bFoundFirst = TRUE;
		}
		else
		{
			ASSERT(level_scaling_data->nLevelDiff == nPrevLevelDiff + 1);
			nPrevLevelDiff = level_scaling_data->nLevelDiff;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// Client and Server functions first
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DAMAGE_EFFECT_DATA * DamageEffectGetData(
	GAME * game,
	int effect_type)
{
	return (const DAMAGE_EFFECT_DATA *)ExcelGetData(game, DATATABLE_DAMAGE_EFFECTS, effect_type);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DAMAGE_TYPE_DATA * DamageTypeGetData(
	GAME * game,
	int damage_type)
{
	return (const DAMAGE_TYPE_DATA *)ExcelGetData(game, DATATABLE_DAMAGETYPES, damage_type);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GetNumDamageTypes(
	GAME * game)
{
	return ExcelGetNumRows(game, DATATABLE_DAMAGETYPES);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetResponsibleUnit(
	UNIT * pUnit)
{
	ASSERT_RETNULL(pUnit);

	UNITID idResponsibleUnit = UnitGetUnitIdTag(pUnit, TAG_SELECTOR_RESPONSIBLE_UNIT);
	if(idResponsibleUnit != INVALID_ID && idResponsibleUnit != UnitGetId(pUnit))
	{
		UNIT * pResponsibleUnit = UnitGetById(UnitGetGame(pUnit), idResponsibleUnit);
		// It's possible that our responsible unit has been freed!  In that case, we'll go to the owner
		if(pResponsibleUnit)
		{
			UNIT * pUltimateOwner = UnitGetResponsibleUnit(pResponsibleUnit);
			return pUltimateOwner ? pUltimateOwner : pResponsibleUnit;
		}
	}

	UNIT * pUltimateOwner = UnitGetUltimateOwner(pUnit);
	return pUltimateOwner ? pUltimateOwner : pUnit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveDead(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
#define MIN_DEAD_DISSAPPEAR_RANGE 20.0f
	BOOL bDestroy = FALSE;
	BOOL bFadeOnDestroy = TRUE;
	if (UnitTestFlag(unit, UNITFLAG_ON_DIE_DESTROY) || UnitTestFlag(unit, UNITFLAG_ON_DIE_END_DESTROY))
	{
		bDestroy = TRUE;
		bFadeOnDestroy = FALSE;
		if(UnitTestFlag(unit, UNITFLAG_FORCE_FADE_ON_DESTROY))
		{
			bFadeOnDestroy = TRUE;
		}
	}

	if (!bDestroy && !RoomIsPlayerInConnectedRoom(game, UnitGetRoom(unit), UnitGetPosition(unit), MIN_DEAD_DISSAPPEAR_RANGE))
	{
		bDestroy = TRUE;
	}
	if (!bDestroy)
	{
		UnitRegisterEventTimed(unit, sRemoveDead, EVENT_DATA(), GAME_TICKS_PER_SECOND * 30);
	}
	else
	{
		DWORD dwFreeFlags = UFF_SEND_TO_CLIENTS;
		if ( bFadeOnDestroy )
			dwFreeFlags |= UFF_FADE_OUT;
		UnitFree( unit, dwFreeFlags );
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerDieEndEvent(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA& event_data)
{
	UnitTriggerEvent(game, EVENT_DEAD, unit, NULL, NULL);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void UnitDieSvrdbg(
	UNIT * unit,
	UNIT * pKiller,
	const char * file,
	unsigned int line)
#else
void UnitDie(
	UNIT * unit,
	UNIT * pKiller)
#endif
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
#if ISVERSION(SERVER_VERSION)
	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "Unit %d in game %I64x died at %s line: %d.", 
		UnitGetId(unit), GameGetId(game), file, line);
#endif
	const UNIT_DATA *pUnitData = UnitGetData( unit );
	REF( pUnitData );
	if( AppIsTugboat() )
	{
		//this seems like a very bad place for this....
		LEVEL *pLevel = UnitGetLevel( unit );
		if( pLevel &&
			pLevel->m_LevelAreaOverrides.nBossId != INVALID_ID &&
			pLevel->m_LevelAreaOverrides.nBossId == (int)UnitGetId( unit ) )
		{
			if( IS_SERVER( game ) )
			{
				s_StateSet( unit, unit, STATE_NO_RESPAWN_ALLOWED, BOSS_RESPAWN_DELAY * GAME_TICKS_PER_SECOND / MSECS_PER_SEC );
			}
		}
		//bad place for this...
		// if the player has the state they cannot die even if they 
		if( UnitHasState( game, unit, STATE_CANNOT_DIE ) )
		{			
			// vitals that could have got them here in the first place
			VITALS_SPEC tVitals;
			VitalsSpecZero( tVitals );			
			s_UnitRestoreVitals( unit, FALSE, &tVitals );
			return;
		}
		// In Mythos Hirelings drop all their stuff - since you can equip them with goodies
		if( IS_SERVER( game ) && UnitIsA( unit, UNITTYPE_HIRELING ) )
		{
			InventoryDropAll( unit );
		}
	}
	// must set this flag first to prevent endless loop for dying events, etc.
	UnitSetFlag(unit, UNITFLAG_DEAD_OR_DYING);
	
	//notify achievement system if the unit is a player
	if(UnitIsA(unit,UNITTYPE_PLAYER) &&
	   IS_SERVER( game ))
	{
		s_AchievementsSendDie(unit,pKiller,(pKiller ? UnitGetUltimateOwner(pKiller) : NULL) );
	}

	// never let players (or things they own) die in levels that don't allow it
	//...unless they're trying to dismiss their pet; they should be able to do that... - bmanegold
	LEVEL *pLevelVictim = UnitGetLevel( unit );
	if (pLevelVictim != NULL && LevelPlayerOwnedCannotDie( pLevelVictim ) == TRUE && !TESTBIT(unit->pdwFlags, UNIT_DATA_FLAG_DISMISS_PET) &&
		!LevelGetPVPWorld( pLevelVictim ) )
	{
		UNIT *pOwner = UnitGetUltimateOwner( unit );
		ASSERTV_RETURN( pOwner, "Expected ultimate owner of unit '%s'", UnitGetDevName( unit ));
		if (UnitIsPlayer( pOwner ))
		{
		
			// set them to 1 health, but remove any of the "bad" stuff by restoring their
			// vitals that could have got them here in the first place
			VITALS_SPEC tVitals;
			VitalsSpecZero( tVitals );
			tVitals.nHealthAbsolute = 1;
			s_UnitRestoreVitals( unit, FALSE, &tVitals );
			return;
			
		}
		
	}	

	UNIT * pResponsibleKiller = pKiller;
	if(pResponsibleKiller)
	{
		UNITID idResponsibleUnit = UnitGetUnitIdTag(pResponsibleKiller, TAG_SELECTOR_RESPONSIBLE_UNIT);
		if(idResponsibleUnit != INVALID_ID)
		{
			pResponsibleKiller = UnitGetById(game, idResponsibleUnit);
		}
	}

	UNIT * pOwner = pResponsibleKiller;
	while (pOwner && UnitIsA(pOwner, UNITTYPE_MISSILE) && UnitGetOwnerUnit(pOwner) != pOwner)
	{
		pOwner = UnitGetOwnerUnit(pOwner);
		if (!pOwner)
		{
			break;
		}
		UNITID idResponsibleUnit = UnitGetUnitIdTag(pOwner, TAG_SELECTOR_RESPONSIBLE_UNIT);
		if (idResponsibleUnit != INVALID_ID)
		{
			pOwner = UnitGetById(game, idResponsibleUnit);
		}
	}

	// for monster deaths, notify task/quest system ... we want to do this first before
	// any events run and potentially alter the unit ... for instance, pets will be removed
	// from their owners inventory on death, which the quest system probably wants
	// to look at (Like for the RTS quest for instance) -Colin
	if (IS_SERVER(game))
	{	
#if !ISVERSION( CLIENT_ONLY_VERSION )		
		if (UnitIsA(unit, UNITTYPE_MONSTER))
		{
			s_QuestEventMonsterDying( pOwner, unit );
		}
#endif		
	}
	
	UnitTriggerEvent(game, EVENT_UNITDIE_BEGIN, unit, NULL, NULL);

	UnitUnFuse(unit);

	UnitChangeTargetType(unit, TARGET_DEAD);
	UnitStepListRemove(game, unit, SLRF_FORCE);
	UnitClearFlag(unit, UNITFLAG_COLLIDABLE);
	UnitSetStat(unit, STATS_HP_CUR, 0);
	UnitSetStat(unit, STATS_POWER_CUR, 0);

	UnitClearAllSkillCooldowns( game, unit );

	if (IS_SERVER(game))
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )	
		// this must be called before we set the mode - so that the states are still around
		int nDeathState = UnitGetOnDeathState(game, unit);
		if (nDeathState != INVALID_ID)
		{
			s_StateSet(unit, unit, nDeathState, 0);
		}

		if (UnitIsPlayer( unit ))
		{
			// maybe use an event or something on a player instead?  Don't know, I want this rock solid		
			s_PlayerOnDie( unit, pKiller );
			// Log how they died.
#if ISVERSION(SERVER_VERSION)
			WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
			UnitGetName( unit, uszPlayerName, arrsize(uszPlayerName), 0 );

			WCHAR uszKillerName[ MAX_CHARACTER_NAME ];
			UnitGetName( pKiller, uszKillerName, arrsize(uszKillerName), 0 );	//This should return "???" if pKiller == NULL

			if(PlayerIsHardcore(unit))
			{	//We care about hardcore death a lot more than softcore.
				TraceWarn(TRACE_FLAG_GAME_MISC_LOG, "Hardcore player %ls killed by %ls at file %s line: %d.",
					uszPlayerName, uszKillerName, file, line);
			}
			else
			{
				TraceInfo(TRACE_FLAG_GAME_MISC_LOG, "Softcore player %ls killed by %ls at file %s line: %d.",
					uszPlayerName, uszKillerName, file, line);
			}
#endif
		}
		

		BOOL bHasMode = FALSE;
		float fDyingDurationInTicks = 0;
		if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_START_DEAD))
		{
			UnitGetModeDuration( game, unit, MODE_SPAWNDEAD, bHasMode );
		}
		if( bHasMode )
		{
			s_UnitSetMode( unit, MODE_SPAWNDEAD, 0, 0);
			UnitRegisterEventTimed(unit, sTriggerDieEndEvent, EVENT_DATA(), 0);
		}
		else
		{
			fDyingDurationInTicks = UnitGetModeDuration(game, unit, MODE_DYING, bHasMode);
			//fDyingDurationInTicks += UnitGetExtraDyingTicks(unit);
			if ( bHasMode != TRUE && pUnitData->szAppearance[0] != 0 )
			{
				ASSERTV(0, "Unit with appearance is missing Dying Mode:\n\n%s", pUnitData->szAppearance );
			}
			if( AppIsHellgate() )
			{
				s_UnitSetMode(unit, MODE_DYING, 0, fDyingDurationInTicks);
			}
			else
			{
				// TRAVIS: In Mythos, we don't specify the death duration - let it pick from the possibly
				// variable death durations on the client, so all our deaths don't have to be the EXACT same lenght.
				// could the dead event be a little off on the client? Yeah. But so what.
				s_UnitSetMode(unit, MODE_DYING, 0, 0, fDyingDurationInTicks );
			}
			UnitRegisterEventTimed(unit, sTriggerDieEndEvent, EVENT_DATA(), int(fDyingDurationInTicks*GAME_TICKS_PER_SECOND_FLOAT));
		}


		MSG_SCMD_UNITDIE msg;
		msg.id = UnitGetId(unit);
		msg.idKiller = pOwner ? UnitGetId(pOwner) : INVALID_ID;
		s_SendUnitMessage(unit, SCMD_UNITDIE, &msg);

		UnitApplyImpacts(game, unit);
		
		// quest system messages
		if (UnitIsA(unit, UNITTYPE_MONSTER))
		{
			s_QuestEventMonsterKill( pOwner, unit );
			s_TaskEventMonsterKill( pOwner, unit );
			if ( AppIsHellgate() )
			{
				s_MonsterFirstKill( pOwner, unit );
			}
		}

		switch (GameGetSubType(game))
		{
		case GAME_SUBTYPE_CUSTOM:
			s_CustomGameEventKill(game, pOwner, unit);
			break;
		}

		// if we are a monster that must respawn, then we'll do so now, in a deactivated state-
		// and then register an event to activate us at the appropriate location later
		if (UnitTestFlag(unit, UNITFLAG_SHOULD_RESPAWN))
		{
			VECTOR vSpawnPoint = UnitGetStatVector(unit, STATS_AI_ANCHOR_POINT_X, 0);
			if(VectorIsZero(vSpawnPoint))
			{
				vSpawnPoint = UnitGetPosition( unit );
			}
			MONSTER_SPEC tSpec;
			tSpec.nClass = UnitIsA( unit, UNITTYPE_OBJECT ) ? GlobalIndexGet( GI_OBJECT_RESPAWNER ) : GlobalIndexGet( GI_MONSTER_RESPAWNER );
			tSpec.nExperienceLevel = UnitGetExperienceLevel( unit );
			tSpec.nMonsterQuality = MonsterGetQuality( unit );
			tSpec.pRoom = UnitGetRoom( unit );
			tSpec.vPosition= vSpawnPoint;
			tSpec.vDirection = UnitGetFaceDirection( unit, FALSE );
			tSpec.nWeaponClass = INVALID_ID;

			UNIT * pMonster = NULL;

			pMonster = s_MonsterSpawn( game, tSpec );
			if( pMonster )
			{
				DWORD dwNow = GameGetTick( game );
				UnitSetStat( pMonster, STATS_LAST_INTERACT_TICK, dwNow );
				UnitSetStat( pMonster, STATS_RESPAWN_CLASS_ID, UnitGetClass( unit ) );
				if( QuestUnitIsQuestUnit( unit ) )
				{
					const QUEST_TASK_DEFINITION_TUGBOAT*  pQuestTask = QuestUnitGetTask( unit );
					int nIndex = QuestUnitGetIndex( unit );
					if( pQuestTask )
					{
						s_SetUnitAsQuestUnit( pMonster, NULL, pQuestTask, nIndex );
					}
				}
				if( UnitTestFlag( unit, UNITFLAG_RESPAWN_EXACT ) )
				{
					UnitSetFlag( pMonster, UNITFLAG_RESPAWN_EXACT );
				}

				if( UnitIsA( unit, UNITTYPE_OBJECT ) )
				{
					UnitRegisterEventTimed( pMonster, s_ObjectRespawnFromRespawner, NULL, (int)UnitGetData( unit )->nRespawnPeriod );
				}
				else
				{
					UnitRegisterEventTimed( pMonster, s_MonsterRespawnFromRespawner, NULL, (int)UnitGetData( unit )->nRespawnPeriod );
				}
			}
			

		}
		if (!UnitTestFlag(unit, UNITFLAG_DESTROY_DEAD_NEVER) ||
			UnitTestFlag(unit, UNITFLAG_SHOULD_RESPAWN) )	// if they respawn, we HAVE to destroy the corpses
		{

			// setup timer to destroy dead body
			int nTicksTillDestroy = 1;
			if ( UnitTestFlag(unit, UNITFLAG_ON_DIE_END_DESTROY) )		// KCK: We need these to die at the end of our death anim, not 30 secs later
			{
				nTicksTillDestroy = (int)(fDyingDurationInTicks * GAME_TICKS_PER_SECOND_FLOAT);
			}
			else if ( UnitTestFlag(unit, UNITFLAG_ON_DIE_DESTROY) == FALSE )
			{
				if( AppIsHellgate() )
				{
					nTicksTillDestroy = (int)(fDyingDurationInTicks * GAME_TICKS_PER_SECOND_FLOAT * 30.0f);
				}
				else
				{
					nTicksTillDestroy = (int)(fDyingDurationInTicks * GAME_TICKS_PER_SECOND_FLOAT * 10.0f);
				}
			}
			// KCK: Note, this case is no longer necessary, Tugboat could now simply set the UNITFLAG_ON_DIE_END_DESTROY flag
			else if( AppIsTugboat() ) 	// we keep 'em aroound so they doon't get removed on the clients if the client hasn't destroyed them yet
			{
				nTicksTillDestroy = (int)(fDyingDurationInTicks * GAME_TICKS_PER_SECOND_FLOAT);
			}
			// if we're a respawner, then our corpse needs to go away, no matter what.
			if( UnitTestFlag(unit, UNITFLAG_SHOULD_RESPAWN ) )
			{
				UnitSetFlag(unit, UNITFLAG_ON_DIE_DESTROY);
				UnitSetFlag(unit, UNITFLAG_FORCE_FADE_ON_DESTROY);
			}
			UnitRegisterEventTimed(unit, sRemoveDead, EVENT_DATA(), nTicksTillDestroy);
		}

		if ( pUnitData->nDyingState != INVALID_ID )
		{
			s_StateSet( unit, unit, pUnitData->nDyingState, 0 );
		}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )	
	}
	if (IS_CLIENT(game))
	{
#if !ISVERSION(SERVER_VERSION)
		UnitApplyImpacts(game, unit);
		if (UnitTestFlag(unit, UNITFLAG_ON_DIE_HIDE_MODEL))
		{
			e_ModelSetDrawAndShadow(c_UnitGetModelId(unit), FALSE);
		}
		if (GameGetControlUnit(game) == unit)
		{
			c_PlayerClearAllMovement(game);
			c_CameraSetViewType(VIEW_3RDPERSON, TRUE);
			UIHideNPCDialog(CCT_CANCEL);
			UIHideNPCVideo();
		}
#endif// !ISVERSION(SERVER_VERSION)
	}

	UnitTriggerEvent(game, EVENT_UNITDIE_END, unit, NULL, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CombatGetBaseDamage(
	COMBAT * combat,
	UNIT * unit,
	int * base_dmg_min,
	int * base_dmg_max)
{
	*base_dmg_min = 0;
	*base_dmg_max = 0;

	ASSERT_RETFALSE(unit);

	*base_dmg_min = UnitGetStat(unit, STATS_BASE_DMG_MIN);
	*base_dmg_max = UnitGetStat(unit, STATS_BASE_DMG_MAX);
	if (*base_dmg_max < *base_dmg_min)
	{
		*base_dmg_min = *base_dmg_max = (*base_dmg_min + *base_dmg_max) / 2;
	}

	if (!combat)
	{
		return TRUE;
	}

	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return TRUE;
	}

	return TRUE;
}


BOOL CombatGetWeaponMinMaxBaseDamage( UNIT *pWeapon, int *min_damage, int *max_damage )
{
	ASSERT_RETFALSE( pWeapon );
	return CombatGetBaseDamage( NULL, pWeapon, min_damage, max_damage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatGetStatByDamageEffectAndDeliveryAndCaste(
	UNIT * unit,
	int defender_type,
	int base_stat,
	int local_stat,
	int splash_stat,
	int caste_stat,
	int per_enemy_stat,
	int dmg_index,
	int delivery_type)
{
	int value = UnitGetStat(unit, base_stat) + (dmg_index > 0 ? UnitGetStat(unit, base_stat, dmg_index) : 0);

	switch (delivery_type)
	{
	case COMBAT_LOCAL_DELIVERY:
		value += UnitGetStat(unit, local_stat) + (dmg_index > 0 ? UnitGetStat(unit, local_stat, dmg_index) : 0);
		break;
	case COMBAT_SPLASH_DELIVERY:
		value += UnitGetStat(unit, splash_stat) + (dmg_index > 0 ? UnitGetStat(unit, splash_stat, dmg_index) : 0);
		break;
	}

	if (per_enemy_stat != INVALID_ID)
	{
		int nEnemies = UnitGetStat(unit, STATS_HOLY_RADIUS_ENEMY_COUNT);
		if (nEnemies)
		{
			nEnemies *= UnitGetStat(unit, per_enemy_stat, dmg_index);
			value += nEnemies;
		}
	}

	if (defender_type > UNITTYPE_NONE)
	{
		UNIT_ITERATE_STATS_RANGE(unit, caste_stat, caste_bonus, ii, 32)
		{
			if (UnitIsA(defender_type, caste_bonus[ii].GetParam()))
			{
				value += caste_bonus[ii].value;
			}
		}
		UNIT_ITERATE_STATS_END;
	}

	return value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sCombatCalcDamageBonusPercent(
	UNIT * attacker,
	int defender_type,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee)
{
	ASSERT_RETZERO(attacker);

	int bonus = CombatGetStatByDamageEffectAndDeliveryAndCaste(attacker, defender_type,
		STATS_DAMAGE_PERCENT, STATS_DAMAGE_PERCENT_LOCAL, STATS_DAMAGE_PERCENT_SPLASH,
		STATS_DAMAGE_PERCENT_CASTE, STATS_DAMAGE_PERCENT_PER_ENEMY, dmg_type, delivery_type);
	if (bIsMelee)
	{
		bonus += UnitGetStat(attacker, STATS_DAMAGE_PERCENT_MELEE);
	}
	
	if( attacker &&
		( UnitGetGenus( attacker ) == GENUS_MISSILE ))
	{
		int nSkillFireMissile = UnitGetStat( attacker, STATS_AI_SKILL_PRIMARY );
		if( nSkillFireMissile != INVALID_ID )
		{
			const SKILL_DATA *pSkillData = SkillGetData( UnitGetGame( attacker ), nSkillFireMissile );
			if(  SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_SPELL) && SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_RANGED ))
			{
				bonus += UnitGetStat( UnitGetUltimateOwner( attacker ), STATS_DAMAGE_PERCENT_SPELL );
			}
			else if( SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_RANGED ) && !SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_SPELL))
			{
				bonus += UnitGetStat( UnitGetUltimateOwner( attacker ), STATS_DAMAGE_PERCENT_RANGED );
			}
		}
	}

	{
		const STATS_DATA * pStatsData = StatsGetData(NULL, STATS_DAMAGE_PERCENT_STATE);
		UNIT_ITERATE_STATS_RANGE(attacker, STATS_DAMAGE_PERCENT_STATE, pStatsEntry, jj, 100)
		{
			unsigned int nState		 = StatGetParam( pStatsData, pStatsEntry[ jj ].GetParam(), 0 );
			unsigned int nCombatType = StatGetParam( pStatsData, pStatsEntry[ jj ].GetParam(), 1 );

			if ( nCombatType == PARAM_AVOIDANCE_ALL ||
				(nCombatType == PARAM_AVOIDANCE_MISSILE && !bIsMelee ) ||
				(nCombatType == PARAM_AVOIDANCE_MELEE && bIsMelee ))
			{
				if ( (nState != INVALID_ID) && UnitHasState( UnitGetGame( attacker ), attacker, nState ) )
					bonus += pStatsEntry[ jj ].value;
			}
		}
		UNIT_ITERATE_STATS_END;
	}

	// damage percent skill multiplies the damage bonus
	int damage_percent_skill = UnitGetStat(attacker, STATS_DAMAGE_PERCENT_SKILL);
	if(damage_percent_skill > 0)
	{
		damage_percent_skill += PCT(damage_percent_skill, bonus);
		bonus += damage_percent_skill;
	}

	bonus = MAX(bonus, -100);
	return bonus;
}


//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetMinDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee )
{
	ASSERT_RETZERO(game && unit);
	int base_dmg_min, base_dmg_max;
	if (!CombatGetBaseDamage(NULL, unit, &base_dmg_min, &base_dmg_max))
	{
		return 0;
	}
	int dmg_min_pct = 0;
	int dmg_max_pct = 0;
	if (stats)
	{
		dmg_min_pct = StatsGetStat(stats, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = StatsGetStat(stats, STATS_DAMAGE_MAX, dmg_type);
	}
	else
	{
		dmg_min_pct = UnitGetStat(unit, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = UnitGetStat(unit, STATS_DAMAGE_MAX, dmg_type);
	}
	int dmg_min = PCT(base_dmg_min, dmg_min_pct);
	int dmg_max = PCT(base_dmg_max, dmg_max_pct);
	if (dmg_max < dmg_min)
	{
		dmg_min = dmg_max = (dmg_min + dmg_max) / 2;
	}

	int dmg_bonus_pct = sCombatCalcDamageBonusPercent(unit, objecttype, dmg_type, delivery_type, bIsMelee);

	dmg_min = dmg_min + PCT(dmg_min, dmg_bonus_pct);
	int shift = StatsGetShift(game, STATS_BASE_DMG_MIN);
	dmg_min = (dmg_min + (1 << shift) - 1) >> shift;

	int dmg_bonus_add = UnitGetStat(unit, STATS_DAMAGE_BONUS, dmg_type) + (dmg_type != 0 ? UnitGetStat(unit, STATS_DAMAGE_BONUS, 0) : 0);
	dmg_bonus_add = dmg_bonus_add >> StatsGetShift(game, STATS_DAMAGE_BONUS);
	dmg_min += dmg_bonus_add;

	dmg_min = MAX(dmg_min, 0);

	return dmg_min;
}


//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetMaxDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee)
{
	ASSERT_RETZERO(game && unit);
	int base_dmg_min, base_dmg_max;
	if (!CombatGetBaseDamage(NULL, unit, &base_dmg_min, &base_dmg_max))
	{
		return 0;
	}
	int dmg_min_pct = 0;
	int dmg_max_pct = 0;
	if (stats)
	{
		dmg_min_pct = StatsGetStat(stats, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = StatsGetStat(stats, STATS_DAMAGE_MAX, dmg_type);
	}
	else
	{
		dmg_min_pct = UnitGetStat(unit, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = UnitGetStat(unit, STATS_DAMAGE_MAX, dmg_type);
	}
	int dmg_min = PCT(base_dmg_min, dmg_min_pct);
	int dmg_max = PCT(base_dmg_max, dmg_max_pct);
	if (dmg_max < dmg_min)
	{
		dmg_min = dmg_max = (dmg_min + dmg_max) / 2;
	}

	int dmg_bonus_pct = sCombatCalcDamageBonusPercent(unit, objecttype, dmg_type, delivery_type, bIsMelee);

	dmg_max = dmg_max + PCT(dmg_max, dmg_bonus_pct);
	int shift = StatsGetShift(game, STATS_BASE_DMG_MAX);
	dmg_max = (dmg_max + (1 << shift) - 1) >> shift;

	int dmg_bonus_add = UnitGetStat(unit, STATS_DAMAGE_BONUS, dmg_type) + (dmg_type != 0 ? UnitGetStat(unit, STATS_DAMAGE_BONUS, 0) : 0);
	dmg_bonus_add = dmg_bonus_add >> StatsGetShift(game, STATS_DAMAGE_BONUS);
	dmg_max += dmg_bonus_add;

	if( AppIsHellgate() )
	{
		dmg_max = MAX(dmg_max, 1);
	}
	return dmg_max;
}

//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetBaseMinDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee )
{
	ASSERT_RETZERO(game && unit);
	int base_dmg_min, base_dmg_max;
	if (!CombatGetBaseDamage(NULL, unit, &base_dmg_min, &base_dmg_max))
	{
		return 0;
	}
	int dmg_min_pct = 0;
	int dmg_max_pct = 0;
	if (stats)
	{
		dmg_min_pct = StatsGetStat(stats, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = StatsGetStat(stats, STATS_DAMAGE_MAX, dmg_type);
	}
	else
	{
		dmg_min_pct = UnitGetStat(unit, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = UnitGetStat(unit, STATS_DAMAGE_MAX, dmg_type);
	}
	int dmg_min = PCT(base_dmg_min, dmg_min_pct);
	int dmg_max = PCT(base_dmg_max, dmg_max_pct);
	if (dmg_max < dmg_min)
	{
		dmg_min = dmg_max = (dmg_min + dmg_max) / 2;
	}

	int dmg_bonus_pct = sCombatCalcDamageBonusPercent(unit, objecttype, dmg_type, delivery_type, bIsMelee);

	dmg_min = dmg_min + PCT(dmg_min, dmg_bonus_pct);
	int shift = StatsGetShift(game, STATS_BASE_DMG_MIN);
	dmg_min = (dmg_min + (1 << shift) - 1) >> shift;

	dmg_min = MAX(dmg_min, 0);

	return dmg_min;
}


//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetBaseMaxDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee)
{
	ASSERT_RETZERO(game && unit);
	int base_dmg_min, base_dmg_max;
	if (!CombatGetBaseDamage(NULL, unit, &base_dmg_min, &base_dmg_max))
	{
		return 0;
	}
	int dmg_min_pct = 0;
	int dmg_max_pct = 0;
	if (stats)
	{
		dmg_min_pct = StatsGetStat(stats, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = StatsGetStat(stats, STATS_DAMAGE_MAX, dmg_type);
	}
	else
	{
		dmg_min_pct = UnitGetStat(unit, STATS_DAMAGE_MIN, dmg_type);
		dmg_max_pct = UnitGetStat(unit, STATS_DAMAGE_MAX, dmg_type);
	}
	int dmg_min = PCT(base_dmg_min, dmg_min_pct);
	int dmg_max = PCT(base_dmg_max, dmg_max_pct);
	if (dmg_max < dmg_min)
	{
		dmg_min = dmg_max = (dmg_min + dmg_max) / 2;
	}

	int dmg_bonus_pct = sCombatCalcDamageBonusPercent(unit, objecttype, dmg_type, delivery_type, bIsMelee);

	dmg_max = dmg_max + PCT(dmg_max, dmg_bonus_pct);
	int shift = StatsGetShift(game, STATS_BASE_DMG_MAX);
	dmg_max = (dmg_max + (1 << shift) - 1) >> shift;

	if( AppIsHellgate() )
	{
		dmg_max = MAX(dmg_max, 1);
	}
	return dmg_max;
}

//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetBonusDamagePCT( UNIT *unit, int dmg_type, BOOL bIsMelee )
{
	return sCombatCalcDamageBonusPercent( unit, INVALID_ID, dmg_type, COMBAT_LOCAL_DELIVERY, bIsMelee );
}
//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetBonusDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee )
{
	ASSERT_RETZERO(game && unit);
	int dmg_bonus_add = UnitGetStat(unit, STATS_DAMAGE_BONUS, dmg_type);
	dmg_bonus_add = dmg_bonus_add >> StatsGetShift(game, STATS_DAMAGE_BONUS);
	return dmg_bonus_add;
}


//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetMinThornsDamage(
	GAME * game,
	UNIT * unit,
	STATS * stats,
	int dmg_type )
{
	ASSERT_RETZERO(game && unit);

	int dmg_min = 0;
	int dmg_max = 0;
	if (stats)
	{
		dmg_min = StatsGetStat(stats, STATS_DAMAGE_THORNS_MIN, dmg_type);
		dmg_max = StatsGetStat(stats, STATS_DAMAGE_THORNS_MAX, dmg_type);
	}
	else
	{
		dmg_min = UnitGetStat(unit, STATS_DAMAGE_THORNS_MIN, dmg_type);
		dmg_max = UnitGetStat(unit, STATS_DAMAGE_THORNS_MAX, dmg_type);
	}

	int shift = StatsGetShift(game, STATS_BASE_DMG_MIN);
	dmg_min = (dmg_min + (1 << shift) - 1) >> shift;
	dmg_min = MAX(dmg_min, 0);
	
	int nPercent = UnitGetStat( unit, STATS_DAMAGE_THORNS_PCT, dmg_type ) ;
	if(dmg_type != 0)
		nPercent += UnitGetStat( unit, STATS_DAMAGE_THORNS_PCT, 0);
	if ( nPercent )
		dmg_min += PCT(dmg_min, nPercent );
	return dmg_min;
}

//----------------------------------------------------------------------------
// for display
//----------------------------------------------------------------------------
int CombatGetMaxThornsDamage(
	GAME * game,
	UNIT * unit,
	STATS * stats,
	int dmg_type )
{
	ASSERT_RETZERO(game && unit);

	int dmg_min = 0;
	int dmg_max = 0;
	if (stats)
	{
		dmg_min = StatsGetStat(stats, STATS_DAMAGE_THORNS_MIN, dmg_type);
		dmg_max = StatsGetStat(stats, STATS_DAMAGE_THORNS_MAX, dmg_type);
	}
	else
	{
		dmg_min = UnitGetStat(unit, STATS_DAMAGE_THORNS_MIN, dmg_type);
		dmg_max = UnitGetStat(unit, STATS_DAMAGE_THORNS_MAX, dmg_type);
	}

	int shift = StatsGetShift(game, STATS_BASE_DMG_MIN);
	dmg_max = (dmg_max + (1 << shift) - 1) >> shift;
	dmg_max = MAX(dmg_max, 0);

	int nPercent = UnitGetStat( unit, STATS_DAMAGE_THORNS_PCT, dmg_type );
	if(dmg_type)
		nPercent += UnitGetStat( unit, STATS_DAMAGE_THORNS_PCT, 0 );
	if ( nPercent )
		dmg_max += PCT(dmg_max, nPercent );

	return dmg_max;
}


//----------------------------------------------------------------------------
// make sure the attacker is equipping the weapons
//----------------------------------------------------------------------------
static BOOL sCombatSetWeapons(
	UNIT * source,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT])
{
	if (!weapons || !weapons[0])
	{
		if (source->inventory)
		{
			ASSERT_RETFALSE(UnitInventorySetEquippedWeapons(source, weapons));
			ASSERT_RETFALSE(!weapons || !weapons[1]);
		}
		return TRUE;
	}

	// if the source attacker is not carrying the weapon, do nothing (ie. a missile flying
	// through the air is a source attacker that is not actually carrying the gun/weapon it was fired from)
	for (unsigned int ii = 0; ii < MAX_WEAPONS_PER_UNIT; ++ii)
	{
		if (!weapons[ii])
		{
			continue;
		}

		if (UnitIsContainedBy(weapons[ii], source) == FALSE)
		{
			return TRUE;
		}

		if (ItemIsEquipped(weapons[ii], source) == FALSE)
		{
			return TRUE;
		}

		ASSERT_RETFALSE(UnitIsInDyeChain(source, weapons[ii]));
	}

	UNIT * cloth = StatsGetParent(weapons[0]);
	ASSERT_RETFALSE(cloth);
	ASSERT_RETFALSE(UnitInventorySetEquippedWeapons(cloth, weapons));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUnitTransferDamageStats(
	UNIT * target,
	UNIT * source,
	BOOL bRiders = FALSE,
	BOOL bNonWeapon = FALSE)
{
	ASSERT_RETURN(source && target);

	GAME * game = UnitGetGame(source);
	ASSERT_RETURN(game);

	// iterate through combat damage stats and transfer them
	int num_combat_stats = StatsGetNumCombatStats(game);
	for (int ii = 0; ii < num_combat_stats; ii++)
	{
		int stat = StatsGetCombatStat(game, ii);
		const STATS_DATA* stats_data = StatsGetData(game, stat);
		ASSERT_BREAK(stats_data && StatsDataTestFlag(stats_data, STATS_DATA_COMBAT));
		if (StatsDataTestFlag(stats_data, STATS_DATA_NO_SET))
		{
			continue;
		}
		if (!StatsDataTestFlag(stats_data, STATS_DATA_TRANSFER_TO_MISSILE))
		{
			continue;
		}

		if (StatsDataTestFlag(stats_data, STATS_DATA_DONT_TRANSFER_TO_NONWEAPON_MISSILE) &&
			bNonWeapon)
		{
			continue;
		}

		UNIT_ITERATE_STATS_RANGE(source, stat, stats_list, jj, 64);
			UnitChangeStat(target, stats_list[jj].GetStat(), stats_list[jj].GetParam(), stats_list[jj].value + stats_data->m_nStatOffset);
		UNIT_ITERATE_STATS_END;
	}

	if (bRiders)
	{
		UnitCopyRidersToUnit(source, target, SELECTOR_DAMAGE_RIDER);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitTransferDamages(
	UNIT * target,
	UNIT * source,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int attack_index,
	BOOL bRiders /*=TRUE*/,
	BOOL bNonWeapon /*=FALSE*/)
{
	ASSERT_RETURN(source && target);

	GAME * game = UnitGetGame(source);
	ASSERT_RETURN(game);

	if (!sCombatSetWeapons(source, weapons))
	{
		return;
	}

	sUnitTransferDamageStats(target, source, bRiders, bNonWeapon);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatCalcDamageIncrementByEnergy(
	int energy_max,
	int energy_cur)
{
	if (energy_max <= 0)
	{
		return 0;
	}
	if (energy_cur <= 0)
	{
		energy_cur = 0;
	}
	if (energy_cur >= energy_max)
	{
		energy_cur = energy_max;
	}
	return MIN(100, DAMAGE_INCREMENT_AT_ZERO_ENERGY + ((125 - DAMAGE_INCREMENT_AT_ZERO_ENERGY) * energy_cur) / energy_max);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CombatCalcDamageIncrementByEnergy(
	UNIT * weapon)
{
	if (!weapon)
	{
		return 100;
	}

	int energy_max = UnitGetStat(weapon, STATS_ENERGY_MAX);
	if (energy_max <= 0)
	{
		return 0;
	}

	return sCombatCalcDamageIncrementByEnergy(energy_max, UnitGetStat(weapon, STATS_ENERGY_CURR));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatCalcDamageIncrementByEnergy(
	COMBAT & combat,
	UNIT * weapon)
{
	if (!weapon)
	{
		return 100;
	}

	if (combat.weapon_energy_max <= 0)
	{
		return 0;
	}
	return sCombatCalcDamageIncrementByEnergy(combat.weapon_energy_max, combat.weapon_energy_cur);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// start of Giant section of Server only code
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CombatClear(
	COMBAT & combat)
{
	structclear(combat);
	combat.damagemultiplier = -1.0f;
	combat.damagetypeoverride = -1;
	combat.base_min = -1;
	combat.base_max = -1;
	combat.nSoftHitState = INVALID_LINK;
	combat.nMediumHitState = INVALID_LINK;
	combat.nBigHitState = INVALID_LINK;
	combat.nCritHitState = INVALID_LINK;
	combat.nFumbleHitState = INVALID_LINK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CombatInitWeapons(
	COMBAT & combat,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT])
{
	if (weapons)
	{
		MemoryCopy(combat.weapons, sizeof(UNIT *) * MAX_WEAPONS_PER_UNIT, weapons, sizeof(UNIT *) * MAX_WEAPONS_PER_UNIT);
		if (combat.weapons[0])
		{
			if (UnitUsesEnergy(combat.weapons[0]))
			{
				combat.weapon_energy_max = UnitGetStat(combat.weapons[0], STATS_ENERGY_MAX);
				combat.weapon_energy_cur = UnitGetStat(combat.weapons[0], STATS_ENERGY_CURR);				
			}
		}
	}
	else
	{
		ZeroMemory(combat.weapons, sizeof(UNIT *) * MAX_WEAPONS_PER_UNIT);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int GameGetMonsterScalingCount(
	GAME * game,
	LEVEL * level,
	UNIT * player)
{
#if ISVERSION(DEVELOPMENT)
	int monsterscaling = AppCommonGetMonsterScalingOverride();
	if (monsterscaling)
	{
		return monsterscaling;
	}
#endif	
	PARTYID idParty = player ? UnitGetPartyId( player ) : INVALID_ID;
	if( AppIsTugboat() && idParty == INVALID_ID )
	{
		return 1;
	}
	int distrocount = 0;
	if( level )
	{
		for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
		{
			UNIT * unit = ClientGetPlayerUnit(client);
			if (!unit ||
				(unit && UnitIsInLimbo( unit ) ))
			{
				continue;
			}
			// make sure that this player is actually in the party tied to this attack
			if( AppIsTugboat() && UnitGetPartyId(unit) != idParty )
			{
				continue;
			}

			++distrocount;
		}
	}
	
	return MAX( 1, distrocount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int GameGetMonsterScalingCountForXP(
	GAME * game,
	LEVEL * level,
	UNIT * attacker)
{
#if ISVERSION(DEVELOPMENT)
	int monsterscaling = AppCommonGetMonsterScalingOverride();
	if (monsterscaling)
	{
		return monsterscaling;
	}
#endif	
	int distrocount = 0;
	PARTYID idParty = attacker ? UnitGetPartyId( attacker ) : INVALID_ID;
	if( AppIsTugboat() && idParty == INVALID_ID )
	{
		return 1;
	}
	float DistributionDistance = AppIsHellgate() ? ITEM_DISTRO_MAX_DISTANCE_SQUARED : ITEM_DISTRO_MAX_DISTANCE_SQUARED_MYTHOS;

	if( level )
	{
		for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
		{
			UNIT * unit = ClientGetPlayerUnit(client);
			if (!unit ||
				(unit && UnitIsInLimbo( unit ) ))
			{
				continue;
			}
			if (UnitsGetDistanceSquared(unit, attacker) > DistributionDistance &&
				unit != attacker )
			{
				continue;
			}
			// make sure that this player is actually in the attacker's party
			if( UnitGetPartyId(unit) != idParty )
			{
				continue;
			}
			++distrocount;
		}
	}
	return distrocount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MONSTER_SCALING * GameGetMonsterScalingFactor(
	GAME * game,
	LEVEL *level,
	UNIT *player)
{
	int nPartySize = 1;
	if ( level != NULL && level->m_nPartySizeRecommended > 1 )
	{
		nPartySize = level->m_nPartySizeRecommended;
	} else {
		nPartySize = GameGetMonsterScalingCount(game, level, player);
	}

	return MonsterGetScaling( game, nPartySize );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MONSTER_SCALING * GameGetMonsterScalingFactorForXP(
	GAME * game,
	LEVEL *level,
	UNIT *attacker)
{
	int nPartySize = GameGetMonsterScalingCountForXP(game, level, attacker);

	return MonsterGetScaling( game, nPartySize );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define COMBAT_HISTORY	100
struct CombatHistoryNode
{
	COMBAT_TRACE_FLAGS	m_Category;
	UNITID				m_idUnit;
	char				m_String[512];
	DWORD				m_Color;
};
CombatHistoryNode		g_CombatHistory[COMBAT_HISTORY];
unsigned int			g_CombatHistoryCurr = 0;
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void DumpCombatHistory(
	GAME * game,
	GAMECLIENT * client)
{
	for (unsigned int ii = 0, curr = g_CombatHistoryCurr; ii < COMBAT_HISTORY; ++ii, curr = (curr + 1) % COMBAT_HISTORY)
	{
		if (g_CombatHistory[curr].m_String[0] && g_CombatHistory[curr].m_Color && GameGetCombatTrace(game, g_CombatHistory[curr].m_Category))
		{
			if (GameGetCombatTrace(game, COMBAT_TRACE_DEBUGUNIT_ONLY))
			{
				if (game->m_idDebugUnit == INVALID_ID)
				{
					goto _next;
				}
				if (g_CombatHistory[curr].m_idUnit != INVALID_ID && g_CombatHistory[curr].m_idUnit != game->m_idDebugUnit)
				{
					goto _next;
				}
			}
			WCHAR str[512];
			PStrCvt(str, g_CombatHistory[curr].m_String, arrsize(str));
			ConsoleString1(game, client, g_CombatHistory[curr].m_Color, str);
		}
_next:
		g_CombatHistory[curr].m_String[0] = 0;
		g_CombatHistory[curr].m_idUnit = INVALID_ID;
		g_CombatHistory[curr].m_Color = 0;
		g_CombatHistory[curr].m_Category = COMBAT_TRACE_INVALID;
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CombatLog(
	COMBAT_TRACE_FLAGS cat,
	UNIT * unit,
	const char * format,
	...)
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETURN(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	UNIT * owner = UnitGetUltimateOwner(unit);
	DWORD dwColor = UnitIsA(owner, UNITTYPE_PLAYER) ? CONSOLE_COMBATLOG_PLAYER : CONSOLE_COMBATLOG_MONSTER;
	SVR_VERSION_ONLY(REF(dwColor));

	char msg[1024];
	PStrPrintf(msg, arrsize(msg), "[TICK: %d]  ", GameGetTick(game));
	char msg2[1024];

	va_list args;
	va_start(args, format);
	PStrVprintf(msg2, arrsize(msg2), format, args);
	PStrCat(msg, msg2, arrsize(msg));

	if (GameGetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE) && game->m_idDebugClient != INVALID_CLIENTID)
	{
		if (GameGetCombatTrace(game, cat))
		{
			if (!GameGetCombatTrace(game, COMBAT_TRACE_DEBUGUNIT_ONLY) || owner == NULL ||
				game->m_idDebugUnit == INVALID_ID || UnitGetId(owner) == game->m_idDebugUnit)
			{
				WCHAR strText[1024];
				PStrCvt(strText, msg, arrsize(strText));
				ConsoleString1(game, ClientGetById(game, game->m_idDebugClient), dwColor, strText);
			}
		}
	}
	else
	{
		PStrCopy(g_CombatHistory[g_CombatHistoryCurr].m_String, msg, arrsize(g_CombatHistory[g_CombatHistoryCurr].m_String));
		g_CombatHistory[g_CombatHistoryCurr].m_Category = cat;
		g_CombatHistory[g_CombatHistoryCurr].m_idUnit = (unit ? UnitGetId(unit) : INVALID_ID);
		g_CombatHistory[g_CombatHistoryCurr].m_Color = dwColor;
		g_CombatHistoryCurr = (g_CombatHistoryCurr + 1) % COMBAT_HISTORY;
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetExperienceReward(
	UNIT * attacker_ultimate_source,
	UNIT * defender,
	int experience_multiplier)
{
	if (!UnitTestFlag(defender, UNITFLAG_GIVESLOOT))
	{
		return 0;
	}

	ASSERT_RETZERO(attacker_ultimate_source);
	if (attacker_ultimate_source == defender)
	{
		return 0;
	}

	if (!UnitTestFlag(attacker_ultimate_source, UNITFLAG_CANGAINEXPERIENCE))
	{
		return 0;
	}
	int experience = UnitGetExperienceValue(defender);
	if (experience <= 0)	// change this to == if we want to be able to assess experience penalties
	{
		return 0;
	}
	UnitSetStat(defender, STATS_EXPERIENCE, 0);

	if (experience_multiplier > 0 && experience_multiplier != 100)
	{
		experience = PCT(experience, experience_multiplier);
	}

	if( AppIsTugboat() )
	{
		LEVEL *pLevel = UnitGetLevel( defender );
		if( pLevel &&
			pLevel->m_LevelAreaOverrides.nLevelXPModForNormalMonsters != INVALID_ID )
		{
			if( !MonsterIsChampion( defender ) )
			{
				experience = PCT(experience, pLevel->m_LevelAreaOverrides.nLevelXPModForNormalMonsters);
			}
		}
	}


	if (experience <= 0)
	{
		return 0;
	}
	return experience;
}


//----------------------------------------------------------------------------
// note:	luck reward can be negative
//----------------------------------------------------------------------------
static int sGetLuckReward(
	UNIT * attacker_ultimate_source,
	UNIT * defender,
	int luck_multiplier)
{
	REF(attacker_ultimate_source);

	if (!UnitTestFlag(defender, UNITFLAG_GIVESLOOT))
	{
		return 0;
	}
	//int luck = defender->pUnitData->nLuckBonus;
	int luck = UnitGetStat(defender, STATS_LUCK_BONUS);
	if (luck == 0)
	{
		return 0;
	}
	UnitSetStat(defender, STATS_LUCK_BONUS, 0);

	// tugboat was factoring in attacker & defender level, but this would be a double-factor since the rewards already depend on level

	if (luck_multiplier != 100)
	{
		luck = PCT(luck, luck_multiplier);
	}
	return luck;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRewardLuckToUnit(
	UNIT * unit,
	int luck)
{
	if (luck <= 0)
	{
		luck = 0;
	}
	UnitChangeStat(unit, STATS_LUCK_CUR, luck );//UnitGetStat( unit, STATS_LUCK_CUR ) + luck);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingExperience(
	UNIT * attacker,
	UNIT * defender,
	int experience)
{
	ASSERT_RETVAL(attacker, experience);
	ASSERT_RETVAL(defender, experience);

	UNIT * attacker_source = UnitGetResponsibleUnit(attacker);
	ASSERT_RETVAL(attacker_source, experience);
	UNIT * defender_source = UnitGetResponsibleUnit(defender);
	ASSERT_RETVAL(defender_source, experience);

	BOOL attacker_is_player = UnitIsA(attacker_source, UNITTYPE_PLAYER);
	if (!attacker_is_player)
	{
		return 0;
	}
	BOOL defender_is_player = UnitIsA(defender_source, UNITTYPE_PLAYER);
	if (defender_is_player)
	{
		return 0;
	}

	int attacker_level = UnitGetExperienceLevel(attacker_source);
	int defender_level = UnitGetExperienceLevel(defender);	// not defender_source!

	const LEVEL_SCALING_DATA * level_scaling_data = CombatGetLevelScalingData(UnitGetGame(attacker), attacker_level, defender_level);
	ASSERT_RETVAL(level_scaling_data, experience);

	if( AppIsTugboat() )
	{
		return PCT_ROUNDUP(experience, level_scaling_data->nPlayerAttacksMonsterExp);
	}
	else
	{
		return PCT(experience, level_scaling_data->nPlayerAttacksMonsterExp);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartySplitExperienceAndLuck(
	UNIT * attacker,		// unit in this case is the unit's ultimate source
	UNIT * defender,
	int experience,
	int luck)
{
	if (!UnitCanDoPartyOperations(attacker))
	{
		goto _noparty;
	}
	PARTYID idParty = attacker ? UnitGetPartyId( attacker ) : INVALID_ID;
	float DistributionDistance = AppIsHellgate() ? ITEM_DISTRO_MAX_DISTANCE_SQUARED : ITEM_DISTRO_MAX_DISTANCE_SQUARED_MYTHOS;
	// multi party (TODO: use in-game party cache)
	{
		GAME * game = UnitGetGame(defender);
		ASSERT_RETURN(game);

		int distrocount = 0;
		LEVEL * level = UnitGetLevel(defender);
		if (!level)
		{
			goto _noparty;
		}
		for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
		{
			UNIT * unit = ClientGetPlayerUnit(client);
			if (!unit ||
			    (unit && UnitIsInLimbo( unit ) ))
			{
				continue;
			}
			if( AppIsTugboat() )
			{
				PARTYID idPlayerParty = UnitGetPartyId( unit );
				if( unit != attacker && 
					( idPlayerParty == INVALID_ID ||
					 idPlayerParty != idParty ) )
				{
					continue;
				}
			}
			if (UnitsGetDistanceSquared(unit, defender) > DistributionDistance &&
				unit != attacker )
			{
				continue;
			}
			++distrocount;
		}
		if (distrocount <= 1)
		{
			goto _noparty;
		}

		int start = RandGetNum(game, distrocount);
		int experience_share = experience / distrocount;
		//int luck_share = luck / distrocount;
		int experience_remainder = experience % distrocount;
		int luck_remainder = luck % distrocount;

		int ii = 0;
		for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
		{
			UNIT * unit = ClientGetPlayerUnit(client);
			if (!unit ||
				(unit && UnitIsInLimbo( unit ) ))
			{
				continue;
			}
			if( AppIsTugboat() )
				{
				PARTYID idPlayerParty = UnitGetPartyId( unit );
				if( unit != attacker && 
					( idPlayerParty == INVALID_ID ||
					idPlayerParty != idParty ) )
				{
					continue;
				}
			}
			if (UnitsGetDistanceSquared(unit, defender) > DistributionDistance)
			{
				continue;
			}

			int remainder = 0;
			int jj = (distrocount + ii - start) % distrocount;
			if (jj < experience_remainder)
			{
				remainder = 1;
			}

			//why does Tugboat do this check here? Because we still want the XP split up
			//for the number of people in a group. BUT we don't want to keep awarding 
			//XP to players who have already killed a boss.
			if( AppIsHellgate() ||
				( AppIsTugboat() &&
				  s_QuestMonsterXPAwardedAlready( defender, unit ) == FALSE ) )
			{
				int xp = sCombatApplyScalingExperience(unit, defender, experience_share + remainder);
				s_UnitGiveExperience(unit, xp);
				if (AppIsTugboat())
				{
					s_StateSet(defender, attacker, STATE_AWARDXP, 5);
				}

				remainder = 0;
				jj = (distrocount + ii - start) % distrocount;
				if (jj < luck_remainder)
				{
					remainder = 1;
				}
				/*int luck = luck_share + remainder;
				sRewardLuckToUnit(unit, luck );
				if( luck_share )
				{	
					s_StateSet( defender, attacker, STATE_AWARDLUCK, 5 );
				}*/
				s_QuestMonsterSetXPAwarded(defender, unit);								
			}
		}
	}
	return;

_noparty:
	//why does Tugboat do this check here? Because we still want the XP split up
	//for the number of people in a group. BUT we don't want to keep awarding 
	//XP to players who have already killed a boss.
	if (AppIsHellgate() ||
		(AppIsTugboat() && s_QuestMonsterXPAwardedAlready(defender, attacker) == FALSE))
	{
		int xp = sCombatApplyScalingExperience(attacker, defender, experience);
		s_UnitGiveExperience(attacker, xp);

		if (AppIsTugboat())
		{
			s_StateSet(defender, attacker, STATE_AWARDXP, 5);
		}
		
		/*if( luck )
		{	
			sRewardLuckToUnit(attacker, luck);
			s_StateSet( defender, attacker, STATE_AWARDLUCK, 5 );
		}*/
		s_QuestMonsterSetXPAwarded(defender, attacker);		
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRewardExperienceAndLuck(
	UNIT * attacker_ultimate_source,
	UNIT * defender,
	int experience_multiplier,
	int luck_multiplier)
{
	ASSERT_RETZERO(attacker_ultimate_source && defender);

 	int experience = sGetExperienceReward(attacker_ultimate_source, defender, experience_multiplier);
	int luck = sGetLuckReward(attacker_ultimate_source, defender, luck_multiplier);

	sPartySplitExperienceAndLuck(attacker_ultimate_source, defender, experience, luck);

	return experience;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
ITEM_SPAWN_ACTION s_TreasurePartyDistribute(
	GAME * game, 
	UNIT * spawner,
	UNIT * item,
	int nClass,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	void *pUserData)
{
	ASSERT_RETVAL(game, ITEM_SPAWN_DESTROY);
	ASSERT_RETVAL(item, ITEM_SPAWN_DESTROY);

	UNIT * player = spec->unitOperator;
	if (player)
	{
		PARTYID idParty = player ? UnitGetPartyId( player ) : INVALID_ID;
		int distrocount = 0;
		LEVEL * level = UnitGetLevel(player);
		if (level)
		{
			for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
			{
				UNIT * unit = ClientGetPlayerUnit(client);
				if (!unit)
				{
					continue;
				}
				if( AppIsTugboat() )
				{
					PARTYID idPlayerParty = UnitGetPartyId( unit );
					if( unit != player && 
						( idPlayerParty == INVALID_ID ||
						idPlayerParty != idParty ) )
					{
						continue;
					}
				}
				if (!InventoryItemMeetsClassReqs(item, unit))
				{
					continue;
				}
				if (UnitsGetDistanceSquared(unit, item) > ITEM_DISTRO_MAX_DISTANCE_SQUARED)
				{
					continue;
				}
				++distrocount;
			}
			if (distrocount > 1)
			{
				int pick = RandGetNum(game, distrocount);
				for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
				{
					UNIT * unit = ClientGetPlayerUnit(client);
					if (!unit)
					{
						continue;
					}
					if( AppIsTugboat() )
					{
						PARTYID idPlayerParty = UnitGetPartyId( unit );
						if( unit != player && 
							( idPlayerParty == INVALID_ID ||
							  idPlayerParty != idParty ) )
						{
							continue;
						}
					}
					if (!InventoryItemMeetsClassReqs(item, unit))
					{
						continue;
					}
					if (UnitsGetDistanceSquared(unit, item) > ITEM_DISTRO_MAX_DISTANCE_SQUARED)
					{
						continue;
					}
					if (pick <= 0)
					{
						player = unit;
						break;
					}
					--pick;
				}
			}
		}
	}

	PGUID guidUnit = (player ? UnitGetGuid(player) : INVALID_GUID);
	UnitAddPickupTag(game, item, guidUnit, spec->guidOperatorParty);

	return ITEM_SPAWN_OK;	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sDoFreedTreasureSpawn(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId );

static BOOL s_sDoDeathTreasureSpawn(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	UNITID idAttacker = (UNITID)tEventData.m_Data1;
	UNIT *pAttacker = UnitGetById( pGame, idAttacker );
	s_TreasureSpawnInstanced( pUnit, pAttacker, INVALID_LINK );

	UnitUnregisterEventHandler(pGame, pUnit, EVENT_ON_FREED, s_sDoFreedTreasureSpawn);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDoFreedTreasureSpawn(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	s_sDoDeathTreasureSpawn(game, unit, *event_data);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * CombatGetKiller(
	GAME * game,
	UNIT * defender)
{
	STATS * stats = StatsGetModList(defender, SELECTOR_STATE);
	while (stats)
	{
		STATS_ENTRY stats_entries[1];
		if (StatsGetRange(stats, STATS_STATE_SOURCE, 0, stats_entries, 1) != 0)
		{
			if (stats_entries[0].value != INVALID_ID)
			{
				UNIT * unit = UnitGetById(game, stats_entries[0].value);
				if (unit && TestHostile(game, unit, defender))
				{
					return unit;
				}
			}
		}
		stats = StatsGetModNext(stats, SELECTOR_STATE);
	}
	return defender;
}


//----------------------------------------------------------------------------
// TODO: (phu) i didn't write this, and it's totally the wrong way to do
// unit events.
//----------------------------------------------------------------------------
static void sUnitKillUnitTriggerEvent(
	GAME * game,
	UNIT * attacker,
	UNIT * defender,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ])
{
	if ( ! attacker )
		return;

	UNIT * killunit = attacker;
	BOOL bDoWeaponKillEvents = TRUE;
	do
	{
		if (!UnitIsA(killunit, UNITTYPE_MISSILE))
		{
			UnitTriggerEvent(game, EVENT_DIDKILL, killunit, defender, NULL );
			if( UnitGetOwnerUnit( killunit ) != killunit &&
				defender &&
				defender != killunit )
			{
				UnitTriggerEvent(game, EVENT_MINION_DIDKILL, UnitGetOwnerUnit( killunit ), defender, NULL );
			}
			// record if we did the kill event of the weapon
			if (weapons && killunit == weapons[ 0 ])
			{
				bDoWeaponKillEvents = FALSE;
			}

		}
		if( UnitGetOwnerUnit(killunit) == killunit )
			break; //break out of the loop if it's the same kill unit
		killunit = UnitGetOwnerUnit(killunit);
	}while (killunit );

	// do a did kill event for the weapon (unless we already did it)
	if ( bDoWeaponKillEvents && weapons )
	{
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			if ( weapons[ i ] )
				UnitTriggerEvent(game, EVENT_DIDKILL, weapons[ i ], defender, NULL );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_UnitKillUnit(
	GAME * game,
	const MONSTER_SCALING * monscaling,
	UNIT * attacker,
	UNIT * defender,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	UNIT * attacker_ultimate_source,	// = NULL
	UNIT * defender_ultimate_source)	// = NULL
{

	ASSERT_RETURN(defender);
	if (IsUnitDeadOrDying(defender))
	{
		return;
	}
	if (!attacker)
	{
		attacker = defender;
	}
	if (attacker == defender)
	{
		attacker = CombatGetKiller(game, defender);
	}
	if (!attacker_ultimate_source)
	{
		attacker_ultimate_source = UnitGetResponsibleUnit(attacker);
	}
	REF(defender_ultimate_source);

	if(UnitDataTestFlag(UnitGetData(defender), UNIT_DATA_FLAG_TAKE_RESPONSIBILITY_ON_KILL))
	{
		UnitSetUnitIdTag(defender, TAG_SELECTOR_RESPONSIBLE_UNIT, UnitGetId(attacker_ultimate_source));
	}

	if (IS_SERVER(game))
	{
		if (UnitIsA(attacker_ultimate_source, UNITTYPE_PLAYER))
		{
			// PVP loot drops and Karmic ratings for Mythos
			// but not in duels!
			if( AppIsTugboat() && UnitIsA( defender, UNITTYPE_PLAYER )  &&
				!UnitHasExactState( defender, STATE_PVP_MATCH ) &&
				UnitGetStat(defender, STATS_PVP_DUEL_OPPONENT_ID) == INVALID_ID)
			{

				// Should drop items based on Karmic rating
				// in Asian countries, can't drop items! Must do XP or something
				int nKarma = UnitGetStat( defender, STATS_KARMA );
				int nItems = (int)floor((float)nKarma / KARMA_PER_ITEM );
				float fChance = (float)UnitGetStat( attacker_ultimate_source, STATS_GEAR_DROP_CHANCE);
				if( RandGetFloat( game, 0, 100  ) > fChance  ||
					( UnitGetExperienceLevel(attacker_ultimate_source) - UnitGetExperienceLevel( defender ) ) >= 10 )
				{
					// no equipment drops if they're 10 levels below the attacker!
					// but.... should we do SOMETHING?
				}
				else
				{
					s_DropEquippedItemsPVP( game, defender, 1 + nItems );
				}

				if( InHonorableAggression( game,
					defender,
					attacker_ultimate_source ) )
				{
					nKarma = UnitGetStat( attacker_ultimate_source, STATS_KARMA );
					// now we modify the attacker's Karma ( afterward )
					const LEVEL_SCALING_DATA * level_scaling_data = CombatGetLevelScalingData(game, UnitGetExperienceLevel(attacker_ultimate_source), UnitGetExperienceLevel(defender));
					nKarma += level_scaling_data->nPlayerAttacksPlayerKarma;
					nKarma = MIN( nKarma, MAX_KARMA_PENALTY );
					UnitSetStat( attacker_ultimate_source, STATS_KARMA, nKarma );
				}
			}
			else if( AppIsTugboat() && !UnitIsA( defender, UNITTYPE_PLAYER ) &&
					 !UnitIsA( defender, UNITTYPE_DESTRUCTIBLE ) )
			{
				if( PlayerIsInPVPWorld( attacker_ultimate_source ) )
				{
					int nKarma = UnitGetStat( attacker_ultimate_source, STATS_KARMA );

					// now we modify the attacker's Karma 
					const LEVEL_SCALING_DATA * level_scaling_data = CombatGetLevelScalingData(game, UnitGetExperienceLevel(attacker_ultimate_source), UnitGetExperienceLevel(defender));
					nKarma -= level_scaling_data->nPlayerAttacksMonsterKarma;
					nKarma = MAX( nKarma, 0 );
					UnitSetStat( attacker_ultimate_source, STATS_KARMA, nKarma );

				}
			}
		}
	}

	sUnitKillUnitTriggerEvent(game, attacker, defender, weapons);

	UNIT * killunit = attacker;	
	do
	{
		if ( UnitIsA(killunit, UNITTYPE_CREATURE ))
		{
			UnitTriggerEvent(game, EVENT_INITIATEDKILL, killunit, defender, NULL );			
			break;
		}
		if( UnitGetOwnerUnit(killunit) == killunit )
			break; //break out of the loop if it's the same kill unit
		killunit = UnitGetOwnerUnit(killunit);
	}while( killunit );	

	// kill the defender
	InventoryEquippedTriggerEvent(game, EVENT_GOTKILLED, defender, attacker, NULL);
	UnitTriggerEvent(game, EVENT_GOTKILLED, defender, attacker, NULL);
	UnitDie(defender, attacker);

	int experience_multiplier = 100;
	int luck_multiplier = 100;

	if (IS_SERVER(game))
	{
		if (UnitIsA(attacker_ultimate_source, UNITTYPE_PLAYER))
		{
		
			//test if achievement requirments are met
			s_AchievementRequirmentTest( attacker_ultimate_source, attacker, defender, weapons );
			// -----------------------------------------------

			if (AppIsHellgate())
			{
				if(!UnitIsA(defender, UNITTYPE_DESTRUCTIBLE))
				{
					UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(attacker_ultimate_source, TAG_SELECTOR_MONSTERS_KILLED);
					if (pTag)
					{
						pTag->m_nCounts[pTag->m_nCurrent]++;
					}
				}

				// Check for kill monster type minigame goals
				s_PlayerCheckMinigame(game, attacker_ultimate_source, MINIGAME_KILL_TYPE, UnitGetType(defender));

				// Check for damage kill type minigame goals
				UNIT_ITERATE_STATS_RANGE(defender, STATS_DAMAGE_TAKEN_FROM_ID, pStats, ii, 32)
				{
					if (UnitGetId(attacker_ultimate_source) == (UNITID)pStats[ii].value)
					{
						s_PlayerCheckMinigame(game, attacker_ultimate_source, MINIGAME_DMG_TYPE, pStats[ii].GetParam());
					}
				}
				UNIT_ITERATE_STATS_END;

			}
			const MONSTER_SCALING * monScalingExperience = GameGetMonsterScalingFactorForXP(game, UnitGetLevel( attacker_ultimate_source ), attacker_ultimate_source );
			if (monScalingExperience)
			{
				experience_multiplier = monScalingExperience->nExperience;
				// luck shouldn't scale up - too much good loot, man!
				//luck_multiplier = monscaling->nExperience;		// use xp mult for luck too for now
			}
			experience_multiplier += PCT(experience_multiplier, UnitGetStat(defender, STATS_LOOT_BONUS_PERCENT));
		}
	}

	int experience = 0;
	if (UnitTestFlag(defender, UNITFLAG_GIVESLOOT))
	{
		// award experience and luck
		experience = sRewardExperienceAndLuck(attacker_ultimate_source, defender, experience_multiplier, luck_multiplier);

		// setup event data for treasure spawning
		UNITID idAttacker = UnitGetId(attacker_ultimate_source);
		PGUID guidParty = s_UnitGetPartyGuid(game, attacker_ultimate_source);
		EVENT_DATA tEventData(idAttacker, GUID_GET_HIGH_DWORD(guidParty), GUID_GET_LOW_DWORD(guidParty));

		if (UnitTestFlag(defender, UNITFLAG_ON_DIE_DESTROY))
		{
			s_sDoDeathTreasureSpawn(game, defender, tEventData);
		}
		else
		{
			// this seems like a likely place for a bug that could rob you of
			// loot or worse a critical drop if something else frees the unit before
			// this happens!!!! -Colin
			// GS - Okay!  Then we'll register an EVENT_ON_FREED, too.  And we won't forget to unregister
			//      the ON_FREED handler in s_sDoDeathTreasureSpawn() either, or we'll have quite the
			//      opposite problem.
			UnitRegisterEventHandler(game, defender, EVENT_ON_FREED, s_sDoFreedTreasureSpawn, tEventData);
			UnitRegisterEventTimed(defender, s_sDoDeathTreasureSpawn, &tEventData, 7);
		}
		UnitClearFlag(defender, UNITFLAG_GIVESLOOT);
	}
#if ISVERSION(DEVELOPMENT)
	CombatLog(COMBAT_TRACE_KILLS, attacker, "%s killed by %s, %d xp rewarded", UnitGetDevName(defender),
		UnitGetDevName(attacker), experience);
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUnitTransferIndirectDamageStats(
	UNIT * target,
	UNIT * source)
{
	ASSERT_RETURN(source && target);

	GAME * game = UnitGetGame(source);
	ASSERT_RETURN(game);

	// iterate through combat damage stats and transfer them
	int num_combat_stats = StatsGetNumCombatStats(game);
	for (int ii = 0; ii < num_combat_stats; ii++)
	{
		int stat = StatsGetCombatStat(game, ii);
		const STATS_DATA* stats_data = StatsGetData(game, stat);
		ASSERT_BREAK(stats_data && StatsDataTestFlag(stats_data, STATS_DATA_COMBAT));
		if (StatsDataTestFlag(stats_data, STATS_DATA_DIRECT_DAMAGE) || StatsDataTestFlag(stats_data, STATS_DATA_NO_SET))
		{
			continue;
		}

		UNIT_ITERATE_STATS_RANGE(source, stat, stats_list, jj, 64);
			UnitChangeStat(target, stats_list[jj].GetStat(), stats_list[jj].GetParam(), stats_list[jj].value);
		UNIT_ITERATE_STATS_END;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void CombatSetTargetType(
	COMBAT & combat)
{
	ASSERT_RETURN(combat.attacker);
	UNIT * owner = UnitGetResponsibleUnit(combat.attacker);
	if (!owner)
	{
		owner = combat.attacker;
	}
	combat.TARGET_MASK = 0;

	switch (UnitGetTargetType(owner))
	{
	case TARGET_GOOD:
		combat.TARGET_MASK |= TARGETMASK_BAD;
		if (UnitPvPIsEnabled(owner))
		{
			combat.TARGET_MASK |= TARGETMASK_GOOD;
		}
		break;
	case TARGET_BAD:
		combat.TARGET_MASK |= TARGETMASK_GOOD;
		break;
	default:
		combat.TARGET_MASK |= TARGETMASK_BAD;
		break;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sGetNextDamageTypeMinMax(
	STATS_ENTRY * damage_min,
	int * min_count,
	STATS_ENTRY * damage_max,
	int * max_count,
	STATS_ENTRY * damage_bonus,
	int * bonus_count,
	int * min_damage,				// pct of base  (so 100)
	int * max_damage,
	int * bonus_damage,				// points of damage (ie 10)
	int * damage_type)
{
	STATS_ENTRY * min_entry = (*min_count > 0 ? damage_min + *min_count - 1 : NULL);
	STATS_ENTRY * max_entry = (*max_count > 0 ? damage_max + *max_count - 1 : NULL);
	STATS_ENTRY * bonus_entry = (*bonus_count > 0 ? damage_bonus + *bonus_count - 1 : NULL);

	unsigned int ii = (min_entry ? 1 : 0);
	ii += (max_entry ? 2 : 0);
	ii += (bonus_entry ? 4 : 0);

	switch (ii)
	{
	case 0:		// no damages
		return FALSE;
	case 1:		// just min damage
labelMinOnly:
		*min_damage = MAX(0, min_entry->value);
		*max_damage = *min_damage;
		*bonus_damage = 0;
		*min_count -= 1;
		*damage_type = min_entry->GetParam();
		return TRUE;
	case 2:		// just max damage
labelMaxOnly:
		*min_damage = 0;
		*max_damage = MAX(0, max_entry->value);
		*bonus_damage = 0;
		*max_count -= 1;
		*damage_type = max_entry->GetParam();
		return TRUE;
	case 3:		// min && max
		if (min_entry->GetParam() != max_entry->GetParam())
		{
			if (min_entry->GetParam() < max_entry->GetParam())
			{
				goto labelMinOnly;
			}
			else
			{
				goto labelMaxOnly;
			}
		}
labelMinAndMax:
		*min_damage = MAX(0, min_entry->value);
		*max_damage = MAX(*min_damage, max_entry->value);
		*bonus_damage = 0;
		*min_count -= 1;  *max_count -= 1;
		*damage_type = max_entry->GetParam();
		return TRUE;
	case 4:		// just bonus
labelBonusOnly:
		*min_damage = 0;
		*max_damage = 0;
		*bonus_damage = MAX(0, bonus_entry->value);		// no - bonus damage if no damage
		*bonus_count -= 1;
		*damage_type = bonus_entry->GetParam();
		return TRUE;
	case 5:		// bonus && min
labelBonusAndMin:
		if (min_entry->GetParam() != bonus_entry->GetParam())
		{
			if (min_entry->GetParam() < bonus_entry->GetParam())
			{
				goto labelMinOnly;
			}
			else
			{
				goto labelBonusOnly;
			}
		}
		*min_damage = MAX(0, min_entry->value);
		*max_damage = *min_damage;
		*bonus_damage = bonus_entry->value;
		*min_count -= 1;  *bonus_count -= 1;
		*damage_type = min_entry->GetParam();
		return TRUE;
	case 6:		// bonus && max
labelBonusAndMax:
		if (max_entry->GetParam() != bonus_entry->GetParam())
		{
			if (max_entry->GetParam() < bonus_entry->GetParam())
			{
				goto labelMaxOnly;
			}
			else
			{
				goto labelBonusOnly;
			}
		}
		*min_damage = 0;
		*max_damage = MAX(0, max_entry->value);
		*bonus_damage = bonus_entry->value;
		*max_count -= 1;  *bonus_count -= 1;
		*damage_type = max_entry->GetParam();
		return TRUE;
	case 7:		// bonus && min && max
		if (min_entry->GetParam() != max_entry->GetParam())
		{
			if (min_entry->GetParam() < max_entry->GetParam())
			{
				goto labelBonusAndMin;
			}
			else
			{
				goto labelBonusAndMax;
			}
		}
		if (min_entry->GetParam() != bonus_entry->GetParam())
		{
			if (min_entry->GetParam() < bonus_entry->GetParam())
			{
				goto labelMinAndMax;
			}
			else
			{
				goto labelBonusOnly;
			}
		}
		*min_damage = MAX(0, min_entry->value);
		*max_damage = MAX(*min_damage, max_entry->value);
		*bonus_damage = bonus_entry->value;
		*min_count -= 1;  *max_count -= 1;  *bonus_count -= 1;
		*damage_type = max_entry->GetParam();
		return TRUE;
	default:
		__assume(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sGetCollapsedDamages(
	STATS_ENTRY * damage_min,
	int * min_count,
	STATS_ENTRY * damage_max,
	int * max_count,
	STATS_ENTRY * damage_bonus,
	int * bonus_count,
	int * min_damage,				// pct of base  (so 100)
	int * max_damage,
	int * bonus_damage)				// points of damage (ie 10)
{
	*min_damage = 0;
	*max_damage = 0;
	*bonus_damage = 0;
	int temp_min, temp_max, temp_bonus, temp_type;
	BOOL bResult = FALSE;
	while (sGetNextDamageTypeMinMax(damage_min, min_count, damage_max, max_count, damage_bonus, bonus_count, &temp_min, &temp_max, &temp_bonus, &temp_type))
	{
		bResult = TRUE;
		*min_damage += temp_min;
		*max_damage += temp_max;
		*bonus_damage += temp_bonus;
	}
	return bResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sGetNextDamageTypeMinMax(
	COMBAT & combat,
	STATS_ENTRY * damage_min,
	int * min_count,
	STATS_ENTRY * damage_max,
	int * max_count,
	STATS_ENTRY * damage_bonus,
	int * bonus_count,
	int * min_damage_pct,			// pct of base
	int * max_damage_pct,
	int * bonus_damage,				// points of damage (not shifted, so 10 or some such)
	int * damage_type,
	int * dmg_typeminmax_count)
{
	if(TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		if(*dmg_typeminmax_count == 0)
		{
			*min_count = 0;
			*max_count = 0;
			*bonus_count = 0;
			// Thorns requires a damage type override
			*damage_type = combat.damagetypeoverride;
			*min_damage_pct = 100;
			*max_damage_pct = 100;
			*bonus_damage = 0;
			(*dmg_typeminmax_count)++;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	(*dmg_typeminmax_count)++;

	if (combat.damagetypeoverride >= 0)
	{
		*damage_type = combat.damagetypeoverride;
		return sGetCollapsedDamages(damage_min, min_count, damage_max, max_count, damage_bonus, bonus_count, min_damage_pct, max_damage_pct, bonus_damage);
	}

	return sGetNextDamageTypeMinMax(damage_min, min_count, damage_max, max_count, damage_bonus, bonus_count, min_damage_pct, max_damage_pct, bonus_damage, damage_type);
}


//----------------------------------------------------------------------------
// note: not sure why tugboat doesn't just use evasion for to hit
// Travis: Evasion didn't exist when ToHit was written, I believe.
// tohit stat is also affected by other stats, calculated based on level and dexterity plus bonuses
// we can re-evaluate it as needed.
//----------------------------------------------------------------------------
static inline BOOL UnitDoToHit(
	COMBAT & combat)
{
	if(   UnitDataTestFlag(UnitGetData( combat.attacker ), UNIT_DATA_FLAG_IGNORES_TO_HIT) )
	{
		return TRUE;
	}
	if (!(combat.flags & COMBAT_DEFENDER_ARMORCLASS))
	{
		combat.armorclass = UnitGetStat(combat.defender, STATS_AC);
		combat.flags |= COMBAT_DEFENDER_ARMORCLASS;
	}
	if (combat.armorclass <= 0)
	{
		return TRUE;
	}
	if (!(combat.flags & COMBAT_ATTACKER_TOHIT)  )
	{
		if( UnitDataTestFlag( combat.attacker->pUnitData, UNIT_DATA_FLAG_DONT_USE_SOURCE_FOR_TOHIT )  )
		{
			combat.tohit = UnitGetStat(combat.attacker, STATS_TOHIT);
		}
		else //default always use ultimate source
		{
			combat.tohit = UnitGetStat(combat.attacker_ultimate_source, STATS_TOHIT); //default
			
		}
		combat.flags |= COMBAT_ATTACKER_TOHIT;
	}
	if (combat.tohit >= 10000 )
	{
		return TRUE;
	}
	if (combat.tohit <= 0)
	{
		return FALSE;
	}
	float ToHitChance = 100.0f * ( (float)combat.tohit / ( (float)combat.tohit + (float)combat.armorclass ) );
	if( ToHitChance < 5 )
	{
		ToHitChance = 5;
	}
	if( ToHitChance > 95 )
	{
		ToHitChance = 95;
	}

	return( RandGetFloat( combat.game, 0, 100  ) <= ToHitChance ); //why would this return true for a fail?
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL UnitDoEvasion(
	COMBAT & combat)
{
	WORD param = (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) ? PARAM_AVOIDANCE_MELEE : PARAM_AVOIDANCE_MISSILE);

	if (!(combat.flags & COMBAT_DEFENDER_EVADE))
	{
		combat.evade = UnitGetStat(combat.defender, STATS_EVADE, param) + UnitGetStat(combat.defender, STATS_EVADE, PARAM_AVOIDANCE_ALL);
		combat.flags |= COMBAT_DEFENDER_EVADE;
	}
	if (combat.evade <= 0)
	{
		return FALSE;
	}
	if (!(combat.flags & COMBAT_ATTACKER_ANTIEVADE))
	{
		combat.anti_evade = BASE_ANTI_EVADE + UnitGetStat(combat.attacker, STATS_ANTI_EVADE, param) + UnitGetStat(combat.attacker, STATS_ANTI_EVADE, PARAM_AVOIDANCE_ALL);
		combat.flags |= COMBAT_ATTACKER_ANTIEVADE;
	}
	if (combat.anti_evade <= 0 ||
		(int)RandGetNum(combat.game, combat.evade + combat.anti_evade) < combat.evade)
	{
		UnitTriggerEvent(combat.game, EVENT_WASEVADED, combat.attacker, combat.defender, NULL);
		UnitTriggerEvent(combat.game, EVENT_EVADED, combat.defender, combat.attacker, NULL);

#if ISVERSION(DEVELOPMENT)
		CombatLog(COMBAT_TRACE_EVASION, combat.defender, "%s evaded %s (%d%% chance to evade)",
			UnitGetDevName(combat.defender), UnitGetDevName(combat.attacker), combat.evade * 100 / (combat.evade + combat.anti_evade));
#endif
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL UnitDoBlocking(
	COMBAT & combat)
{
	// Tugboat uses shields, which contribute to the block stat, and do stuff iwth damage reflection based on shieldmastery.
	// probably needs to be nicer overall.
	if( AppIsTugboat() )
	{
		int BlockChanceMelee = UnitGetStat(combat.defender, STATS_BLOCK );

		//if( BlockChanceMelee > 50 )
		//{
		//	BlockChanceMelee = 50;
		//}
		int Roll = RandGetNum( combat.game, 0, 100 );
		if( Roll < BlockChanceMelee )
		{

			UnitTriggerEvent(combat.game, EVENT_WASBLOCKED, combat.attacker, combat.defender, NULL);
			UnitTriggerEvent(combat.game, EVENT_BLOCKED, combat.defender, combat.attacker, NULL);

			const DAMAGE_TYPE_DATA* damage_type_data = DamageTypeGetData(combat.game, DAMAGE_TYPE_ALL);
			s_StateSet(combat.defender, combat.attacker_ultimate_source, damage_type_data->nShieldHitState, 10);

#if ISVERSION(DEVELOPMENT)
		CombatLog(COMBAT_TRACE_BLOCKING, combat.defender, "%s blocked %s (%d%% chance to block)", UnitGetDevName(combat.defender),
			UnitGetDevName(combat.attacker), BlockChanceMelee );
#endif

			int hp = UnitGetStat(combat.defender, STATS_HP_CUR);
			combat.result = COMBAT_RESULT_BLOCK;
			UnitAttackUnitSendDamageMessage(combat, hp);
			return TRUE;
		}
		return FALSE;
	}
	WORD param = (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) ? PARAM_AVOIDANCE_MELEE : PARAM_AVOIDANCE_MISSILE);

	if (!(combat.flags & COMBAT_DEFENDER_BLOCK))
	{
		combat.block = UnitGetStat(combat.defender, STATS_BLOCK, param) + UnitGetStat(combat.defender, STATS_BLOCK, PARAM_AVOIDANCE_ALL);
		combat.flags |= COMBAT_DEFENDER_BLOCK;
	}
	if (combat.block <= 0)
	{
		return FALSE;
	}
	if (!(combat.flags & COMBAT_ATTACKER_ANTIBLOCK))
	{
		combat.anti_block = BASE_ANTI_BLOCK + UnitGetStat(combat.attacker, BASE_ANTI_BLOCK, param) + UnitGetStat(combat.attacker, BASE_ANTI_BLOCK, PARAM_AVOIDANCE_ALL);
		combat.flags |= COMBAT_ATTACKER_ANTIBLOCK;
	}
	if (combat.anti_block <= 0 ||
		(int)RandGetNum(combat.game, combat.block + combat.anti_block) < combat.block)
	{
		UnitTriggerEvent(combat.game, EVENT_WASBLOCKED, combat.attacker, combat.defender, NULL);
		UnitTriggerEvent(combat.game, EVENT_BLOCKED, combat.defender, combat.attacker, NULL);

#if ISVERSION(DEVELOPMENT)
		CombatLog(COMBAT_TRACE_BLOCKING, combat.defender, "%s blocked %s (%d%% chance to block)", UnitGetDevName(combat.defender),
			UnitGetDevName(combat.attacker), combat.block * 100 / (combat.block + combat.anti_block));
#endif

		int hp = UnitGetStat(combat.defender, STATS_HP_CUR);
		combat.result = COMBAT_RESULT_BLOCK;
		UnitAttackUnitSendDamageMessage(combat, hp);

		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_sUnitDoRangedThorns(
	COMBAT & combat,
	int dmg)
{
	if(UnitIsA(combat.attacker, UNITTYPE_MISSILE))
	{
		return;
	}

	//damagetype 0 = all; all gets applied to all other damage types, yay! -bmanegold
	for ( int i = 0; i < GetNumDamageTypes( combat.game ); ++i )
	{
		int nDmgThornsMin = UnitGetStatShift( combat.defender, STATS_DAMAGE_RANGED_THORNS_MIN, i ); 
		int nDmgThornsMax = UnitGetStatShift( combat.defender, STATS_DAMAGE_RANGED_THORNS_MAX, i ); 

		nDmgThornsMin = PCT(dmg, nDmgThornsMin);
		nDmgThornsMin = PCT(dmg, nDmgThornsMax);

		if ( nDmgThornsMin > 0 && nDmgThornsMax > 0 )
		{
			DWORD dwUAUFlags = combat.dwUnitAttackUnitFlags;
			SETBIT(dwUAUFlags, UAU_IS_THORNS_BIT);
			if( AppIsTugboat() )
			{
				SETBIT(dwUAUFlags, UAU_NO_DAMAGE_EFFECTS_BIT);				
			}

			s_UnitAttackUnit( 
				combat.defender, 
				combat.attacker, 
				combat.weapons, 
				0,
#ifdef HAVOK_ENABLED
				NULL,
#endif
				dwUAUFlags, 
				combat.damagemultiplier, 
				i, 
				nDmgThornsMin, 
				nDmgThornsMax, 
				combat.defender );

			// run the skill to do 
			const DAMAGE_TYPE_DATA * pDamageTypeData = DamageTypeGetData(combat.game, i);
			if( !pDamageTypeData )
				continue;
			if ( pDamageTypeData->nThornsState != INVALID_ID )
				s_StateSet( combat.defender, combat.defender, pDamageTypeData->nThornsState, 0 );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_sUnitDoThorns(
	COMBAT & combat)
{
	//damagetype 0 = all; all gets applied to all other damage types, yay! -bmanegold
	for ( int i = 0; i < GetNumDamageTypes( combat.game ); ++i )
	{
		int nDmgThornsMin = UnitGetStat( combat.defender, STATS_DAMAGE_THORNS_MIN, i ); 
		int nDmgThornsMax = UnitGetStat( combat.defender, STATS_DAMAGE_THORNS_MAX, i ); 

																				  //this would do double damage
		int nPercent = UnitGetStat( combat.defender, STATS_DAMAGE_THORNS_PCT, i );// + UnitGetStat( combat.defender, STATS_DAMAGE_THORNS_PCT );
		if(i != 0)
			nPercent += UnitGetStat(combat.defender, STATS_DAMAGE_THORNS_PCT, 0);
		if ( nPercent )
		{
			nDmgThornsMin += PCT(nDmgThornsMin, nPercent);
			nDmgThornsMax += PCT(nDmgThornsMax, nPercent);
		}

		if ( nDmgThornsMin > 0 && nDmgThornsMax > 0 )
		{
			DWORD dwUAUFlags = combat.dwUnitAttackUnitFlags;
			SETBIT(dwUAUFlags, UAU_IS_THORNS_BIT);
			if( AppIsTugboat() )
			{
				SETBIT(dwUAUFlags, UAU_NO_DAMAGE_EFFECTS_BIT);				
			}

			s_UnitAttackUnit( 
				combat.defender, 
				combat.attacker, 
				combat.weapons, 
				0,
#ifdef HAVOK_ENABLED
				NULL,
#endif
				dwUAUFlags, 
				combat.damagemultiplier, 
				i, 
				nDmgThornsMin, 
				nDmgThornsMax, 
				combat.defender );

			// run the skill to do 
			const DAMAGE_TYPE_DATA * pDamageTypeData = DamageTypeGetData(combat.game, i);
			if( !pDamageTypeData )
				continue;
			if ( pDamageTypeData->nThornsState != INVALID_ID )
				s_StateSet( combat.defender, combat.defender, pDamageTypeData->nThornsState, 0 );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL UnitGetBaseDamage(
	COMBAT & combat)
{
	if (combat.flags & COMBAT_BASE_DAMAGES)
	{
		return (combat.base_max > 0);
	}

	combat.flags |= COMBAT_BASE_DAMAGES;

	if (combat.base_min <= 0 || combat.base_max <= 0)
	{
		CombatGetBaseDamage(&combat, combat.attacker, &combat.base_min, &combat.base_max);
	}
	if (combat.base_max <= 0)
	{
		return FALSE;
	}

	combat.damage_increment = UnitGetStat(combat.attacker, STATS_DAMAGE_INCREMENT);
	if (combat.damage_increment <= 0)
	{
		combat.damage_increment = DAMAGE_INCREMENT_FULL;
	}
	if (combat.damage_increment_pct > 0 && combat.damage_increment_pct != DAMAGE_INCREMENT_FULL)
	{
		combat.damage_increment = PCT(combat.damage_increment, combat.damage_increment_pct);
	}

	int nDamageIncrementPercent = UnitGetStat(combat.attacker, STATS_DAMAGE_INCREMENT_PCT);
	if (nDamageIncrementPercent > 0 && nDamageIncrementPercent != DAMAGE_INCREMENT_FULL)
	{
		combat.damage_increment = PCT(combat.damage_increment, nDamageIncrementPercent);
	}

	if (combat.damagemultiplier > 0)
	{
		combat.base_min = (int)(float(combat.base_min) * combat.damagemultiplier);
		combat.base_max = (int)(float(combat.base_max) * combat.damagemultiplier);
	}
	if (AppIsHellgate())
	{
		combat.attack_rating = UnitGetStat(combat.attacker, STATS_ATTACK_RATING);
		combat.attack_rating = MAX(1, combat.attack_rating);
		combat.attack_rating_base = combat.attack_rating;

		const STATS_DATA * stats_data = StatsGetData(combat.game, STATS_ARMOR);
		ASSERT_RETFALSE(stats_data);
		combat.attack_rating = combat.attack_rating << stats_data->m_nShift;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void UnitAttackUnitSetStates(
	GAME * game,
	UNIT * attacker,
	UNIT * defender,
	UNIT * attacker_ultimate_source,
	STATS * stats)
{
	if (stats)
	{
		STATS_ITERATE_STATS_RANGE(stats, STATS_SET_STATE, pSetStates, ii, 32)
		{
			int nState = pSetStates[ii].GetParam();
			int nDuration = pSetStates[ii].value;
			s_StateSet(defender, attacker_ultimate_source, nState, nDuration);
		}
		STATS_ITERATE_STATS_END;
	}
	else
	{
		UNIT_ITERATE_STATS_RANGE(attacker, STATS_SET_STATE, pSetStates, ii, 32)
		{
			int nState = pSetStates[ii].GetParam();
			int nDuration = pSetStates[ii].value;
			s_StateSet(defender, attacker_ultimate_source, nState, nDuration);
		}
		UNIT_ITERATE_STATS_END;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatGetStatByDelivery(
	UNIT * unit,
	int base_stat,
	int local_stat,
	int splash_stat,
	int delivery_type)
{
	int value = UnitGetStat(unit, base_stat);

	switch (delivery_type)
	{
	case COMBAT_LOCAL_DELIVERY:
		value += UnitGetStat(unit, local_stat);
		break;
	case COMBAT_SPLASH_DELIVERY:
		value += UnitGetStat(unit, splash_stat);
		break;
	}
	return value;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatGetStatByDamageEffectAndDelivery(
	UNIT * unit,
	int base_stat,
	int local_stat,
	int splash_stat,
	int dmg_index,
	int delivery_type)
{
	int value = UnitGetStat(unit, base_stat, dmg_index) + UnitGetStat(unit, base_stat);

	switch (delivery_type)
	{
	case COMBAT_LOCAL_DELIVERY:
		value += UnitGetStat(unit, local_stat, dmg_index) + UnitGetStat(unit, local_stat);
		break;
	case COMBAT_SPLASH_DELIVERY:
		value += UnitGetStat(unit, splash_stat, dmg_index) + UnitGetStat(unit, splash_stat);
		break;
	}
	return value;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatGetStatByDeliveryAndCaste(
	UNIT * unit,
	int defender_type,
	int base_stat,
	int local_stat,
	int splash_stat,
	int caste_stat,
	int delivery_type)
{
	int value = UnitGetStat(unit, base_stat);

	switch (delivery_type)
	{
	case COMBAT_LOCAL_DELIVERY:
		value += UnitGetStat(unit, local_stat);
		break;
	case COMBAT_SPLASH_DELIVERY:
		value += UnitGetStat(unit, splash_stat);
		break;
	}
	if (defender_type > UNITTYPE_NONE)
	{
		UNIT_ITERATE_STATS_RANGE(unit, caste_stat, caste_bonus, ii, 32)
		{
			if (UnitIsA(defender_type, caste_bonus[ii].GetParam()))
			{
				value += caste_bonus[ii].value;
			}
		}
		UNIT_ITERATE_STATS_END;
	}

	return value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatGetStatByDamageEffectAndCaste(
	UNIT * unit,
	int defender_type,
	int base_stat,
	int caste_stat,
	int dmg_index)
{
	int value = UnitGetStat(unit, base_stat, dmg_index) + UnitGetStat(unit, base_stat);

	if (defender_type > UNITTYPE_NONE)
	{
		UNIT_ITERATE_STATS_RANGE(unit, caste_stat, caste_bonus, ii, 32)
		{
			if (UnitIsA(defender_type, caste_bonus[ii].GetParam()))
			{
				value += caste_bonus[ii].value;
			}
		}
		UNIT_ITERATE_STATS_END;
	}

	return value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombatGetEnergyIncrement(
	COMBAT & combat)
{
	if (!(combat.flags & COMBAT_WEAPON_ENERGY_INCREMENT))
	{
		combat.flags |= COMBAT_WEAPON_ENERGY_INCREMENT;
		combat.damage_increment_energy = DAMAGE_INCREMENT_FULL;
		if (combat.weapons[0] && UnitUsesEnergy(combat.weapons[0]))
		{
			combat.damage_increment_energy = sCombatCalcDamageIncrementByEnergy(combat, combat.weapons[0]);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombatGetDualWeaponMeleeIncrement(
	COMBAT & combat)
{
	if (!(combat.flags & COMBAT_DUALWEAPON_MELEE_INCREMENT))
	{
		combat.flags |= COMBAT_DUALWEAPON_MELEE_INCREMENT;
		combat.damage_increment_dualweapon_melee = DAMAGE_INCREMENT_FULL;
		if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_APPLY_DUALWEAPON_MELEE_SCALING_BIT))
		{
			combat.damage_increment_dualweapon_melee = DUAL_WEAPON_DAMAGE_INCREMENT_MELEE;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCombatGetDualWeaponFocusIncrement(
	COMBAT & combat)
{
	if (!(combat.flags & COMBAT_DUALWEAPON_FOCUS_INCREMENT))
	{
		combat.flags |= COMBAT_DUALWEAPON_FOCUS_INCREMENT;
		combat.damage_increment_dualweapon_focus = DAMAGE_INCREMENT_FULL;
		if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT))
		{
			combat.damage_increment_dualweapon_focus = DUAL_WEAPON_DAMAGE_INCREMENT_FOCUS;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingDamageIncrement(
	COMBAT & combat,
	int dmg)
{
	if (dmg <= 0)
	{
		return 0;
	}

	// compute the total damage increment first:
	float damage_increment = 1.0f;
	if (combat.damage_increment != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = PctToFloat(combat.damage_increment);
	}

	sCombatGetEnergyIncrement(combat);
	if (combat.damage_increment_energy != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = damage_increment * PctToFloat(combat.damage_increment_energy);
	}

	sCombatGetDualWeaponMeleeIncrement(combat);
	if (combat.damage_increment_dualweapon_melee != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = damage_increment * PctToFloat(combat.damage_increment_dualweapon_melee);
	}

	sCombatGetDualWeaponFocusIncrement(combat);
	if (combat.damage_increment_dualweapon_focus != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = damage_increment * PctToFloat(combat.damage_increment_dualweapon_focus);
	}

	// now apply it to the damage
	float fdmg = (float)dmg * damage_increment;
	dmg = ROUND(fdmg);

	return dmg;
}


//----------------------------------------------------------------------------
// chance modified by damage increment:
// assume .05 chance.  a damage increment of 200 should be the equivalent of
// 2 rolls on chance.  a damage increment of 300 should be the equivalent of
// 3 rolls, etc.
//----------------------------------------------------------------------------
float CombatModifyChanceByIncrement(
	float chance,
	float increment)
{
	if (increment <= 0)
	{
		0.0f;
	}
	if (increment == 1.0f)
	{
		return chance;
	}
	if (chance <= EPSILON)
	{
		return 0.0f;
	}
	if(chance >= 1.0f)
	{
		return 1.0f;
	}
	float exponent = (float)increment;
	float base = (1.0f - chance);
	return (1.0f - powf(base, exponent));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingChanceIncrement(
	COMBAT & combat,
	float chance,
	BOOL attacker_is_player, 
	BOOL defender_is_player)
{
	if (chance <= 0.0f)
	{
		return 0.0f;
	}

	// compute the total damage increment first:
	float damage_increment = 1.0f;
	if (combat.damage_increment != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = PctToFloat(combat.damage_increment);
	}

	sCombatGetEnergyIncrement(combat);
	if (combat.damage_increment_energy != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = damage_increment * PctToFloat(combat.damage_increment_energy);
	}

	// we don't apply dual melee increment to chance
	// sCombatGetDualWeaponMeleeIncrement(combat);
	// if (combat.damage_increment_dualweapon_melee != DAMAGE_INCREMENT_FULL)
	// {
	// 	damage_increment = damage_increment * PctToFloat(combat.damage_increment_dualweapon_melee);
	// }

	sCombatGetDualWeaponFocusIncrement(combat);
	if (combat.damage_increment_dualweapon_focus != DAMAGE_INCREMENT_FULL)
	{
		damage_increment = damage_increment * PctToFloat(combat.damage_increment_dualweapon_focus);
	}

	// compute chance
	chance = CombatModifyChanceByIncrement(chance, damage_increment);

	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingDamageMultiPlayer(
	COMBAT & combat,
	int dmg,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	if (dmg <= 0)
	{
		return dmg;
	}

	// this applies if the attacker is player-owned and the defender isn't player-owned
	if (attacker_is_player && !defender_is_player)
	{
		// crates shouldn't get damage reduction
		if (UnitIsA(combat.defender, UNITTYPE_DESTRUCTIBLE))
		{
			return dmg;
		}
		// apply damage reduction  
		float damage_multiplier = combat.monscaling->fPlayerVsMonsterIncrementPct[0];
		if (damage_multiplier == 100.0f)
		{
			return dmg;
		}
		ASSERT_RETVAL(damage_multiplier > 0.0f, dmg);
		{
			dmg = FLOAT_TO_INT_ROUND((float)dmg * PctToFloat(damage_multiplier));
		}
	}
	// this applies if the attacker isn't player-owned and the defender is player-owned
	else if (!attacker_is_player && defender_is_player)
	{
		// apply damage multiplier
		unsigned int damage_multiplier = combat.monscaling->nMonsterVsPlayerIncrementPct;
		if (damage_multiplier == 100)
		{
			return dmg;
		}
		ASSERT_RETVAL(damage_multiplier > 0, dmg);
		dmg = PCT(dmg, int(damage_multiplier));
	}
	// don't allow damage to be scaled below a value of one.
	// this is a shifted value, so we are capping to 1/256th of a point
	if (dmg < 1)
	{
		dmg = 1;
	}
	return dmg;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingDamageEliteMode(
	COMBAT & combat,
	int dmg,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	if ( !GameIsVariant( combat.game, GV_ELITE ) )
		return dmg;

	if (dmg <= 0)
	{
		return dmg;
	}

	// this applies if the attacker is player-owned and the defender isn't player-owned
	float damage_multiplier = 100.0f;
	if (attacker_is_player && !defender_is_player)
	{
		// crates shouldn't get damage reduction
		if (UnitIsA(combat.defender, UNITTYPE_DESTRUCTIBLE))
		{
			return dmg;
		}
		// apply damage reduction 

		if( AppIsHellgate() )
		{
			damage_multiplier = 100.0f + ELITE_PLAYER_DAMAGE_PERCENT;
		}
		else
		{
			damage_multiplier = 100.0f + MYTHOS_ELITE_PLAYER_DAMAGE_PERCENT;
		}
	}
	// this applies if the attacker isn't player-owned and the defender is player-owned
	else if (!attacker_is_player && defender_is_player)
	{
		// apply damage multiplier
		if( AppIsHellgate() )
		{
			damage_multiplier = 100.0f + ELITE_MONSTER_DAMAGE_PERCENT;
		}
		else
		{
			damage_multiplier = 100.0f + MYTHOS_ELITE_MONSTER_DAMAGE_PERCENT;
		}
	}

	if (damage_multiplier == 100.0f)
	{
		return dmg;
	}
	ASSERT_RETVAL(damage_multiplier > 0.0f, dmg);
	{
		dmg = FLOAT_TO_INT_ROUND((float)dmg * damage_multiplier / 100.0f);
	}

	// don't allow damage to be scaled below a value of one.
	// this is a shifted value, so we are capping to 1/256th of a point
	if (dmg < 1)
	{
		dmg = 1;
	}
	return dmg;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingChanceMultiPlayer(
	COMBAT & combat,
	float chance,
	BOOL attacker_is_player,
	BOOL defender_is_player,
	int nPlayerVsMonsterScalingIndex)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}

	if (!combat.monscaling)
	{
		return chance;
	}

	if(nPlayerVsMonsterScalingIndex < 0 || nPlayerVsMonsterScalingIndex >= NUM_PLAYER_VS_MONSTER_INCREMENT_PCTS)
	{
		nPlayerVsMonsterScalingIndex = 0;
	}

	if (attacker_is_player && !defender_is_player)
	{
		// apply damage reduction  
		float chance_multiplier = combat.monscaling->fPlayerVsMonsterIncrementPct[nPlayerVsMonsterScalingIndex];
		if (chance_multiplier == 100.0f)
		{
			return chance;
		}
		ASSERT_RETVAL(chance_multiplier > 0.0f, chance);
		{
			chance = CombatModifyChanceByIncrement(chance, PctToFloat(chance_multiplier));
		}
	}
	else if (!attacker_is_player && defender_is_player)
	{
		// apply damage reduction  
		unsigned int chance_multiplier = combat.monscaling->nMonsterVsPlayerIncrementPct;
		if (chance_multiplier == 100)
		{
			return chance;
		}
		ASSERT_RETVAL(chance_multiplier > 0, chance);
		{
			chance = CombatModifyChanceByIncrement(chance, PctToFloat(chance_multiplier));
		}
	}
	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingChanceEliteMode(
	COMBAT & combat,
	float chance,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}

	if (!GameIsVariant(combat.game, GV_ELITE))
	{
		return chance;
	}

	float chance_multiplier = 100.0f;
	if (attacker_is_player && !defender_is_player)
	{
		if( AppIsHellgate() )
		{
			chance_multiplier = 100.0f + ELITE_PLAYER_SFX_CHANCE_PERCENT;
		}
		else
		{
			chance_multiplier = 100.0f + MYTHOS_ELITE_PLAYER_SFX_CHANCE_PERCENT;
		}
	}
	else if (!attacker_is_player && defender_is_player)
	{
		if( AppIsHellgate() )
		{
			chance_multiplier = 100.0f + ELITE_MONSTER_SFX_CHANCE_PERCENT;
		}
		else
		{
			chance_multiplier = 100.0f + MYTHOS_ELITE_MONSTER_SFX_CHANCE_PERCENT;
		}
	}
	else
	{
		return chance;
	}
	if (chance_multiplier == 100.0f)
	{
		return chance;
	}
	ASSERT_RETVAL(chance_multiplier > 0.0f, chance);
	{
		chance = CombatModifyChanceByIncrement(chance, PctToFloat(chance_multiplier));
	}

	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingChancePvP(
	COMBAT & combat,
	float chance,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}

	float chance_multiplier = 100.0f;

	if (attacker_is_player && defender_is_player)
	{
		chance_multiplier = 100.0f + PVP_SFX_CHANCE_PERCENT;
	} 
	else 
	{
		return chance;
	}

	ASSERT_RETVAL(chance_multiplier > 0.0f, chance);
	{
		chance = CombatModifyChanceByIncrement(chance, PctToFloat(chance_multiplier));
	}

	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatGetUnitLevel(
	COMBAT & combat,
	UNIT * unit,
	int flag,
	int & level_store)
{
	if (!(combat.flags & flag))
	{
		combat.flags |= flag;
		ASSERT_RETZERO(unit);
		level_store = UnitGetExperienceLevel(unit);
	}
	return level_store;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const LEVEL_SCALING_DATA * sCombatGetLevelScalingData(
	COMBAT & combat)
{
	int attacker_level = sCombatGetUnitLevel(combat, combat.attacker, COMBAT_ATTACKER_LEVEL, combat.attacker_level);
	int defender_level = sCombatGetUnitLevel(combat, combat.defender, COMBAT_DEFENDER_LEVEL, combat.defender_level);

	return CombatGetLevelScalingData(combat.game, attacker_level, defender_level);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingDamageLevel(
	COMBAT & combat,
	int dmg,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	const LEVEL_SCALING_DATA * level_scaling_data = sCombatGetLevelScalingData(combat);
	ASSERT_RETVAL(level_scaling_data, dmg);

	int level_scaling = 100;
	if (attacker_is_player)
	{
		if (defender_is_player)
		{
			level_scaling = level_scaling_data->nPlayerAttacksPlayerDmg;
		}
		else
		{
			level_scaling = level_scaling_data->nPlayerAttacksMonsterDmg;
		}
	}
	else
	{
		if (defender_is_player)
		{
			level_scaling = level_scaling_data->nMonsterAttacksPlayerDmg;
		}
	}

	return PCT(dmg, level_scaling);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingChanceLevel(
	COMBAT & combat,
	float chance,
	BOOL attacker_is_player,
	BOOL defender_is_player)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}
	const LEVEL_SCALING_DATA * level_scaling_data = sCombatGetLevelScalingData(combat);
	ASSERT_RETVAL(level_scaling_data, chance);

	int level_scaling = 100;
	if (attacker_is_player)
	{
		if (defender_is_player)
		{
			level_scaling = level_scaling_data->nPlayerAttacksPlayerDmg;
		}
		else
		{
			level_scaling = level_scaling_data->nPlayerAttacksMonsterDmg;
		}
	}
	else
	{
		if (defender_is_player)
		{
			level_scaling = level_scaling_data->nMonsterAttacksPlayerDmg;
		}
	}

	if (level_scaling == 100)
	{
		return chance;
	}
	chance = CombatModifyChanceByIncrement(chance, PctToFloat(level_scaling));
	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sCombatGetAttackerUltimateSource(
	COMBAT & combat)
{
	if (!combat.attacker_ultimate_source)
	{
		ASSERT(combat.attacker);
		combat.attacker_ultimate_source = UnitGetResponsibleUnit(combat.attacker);
	}
	return combat.attacker_ultimate_source;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sCombatGetDefenderUltimateSource(
	COMBAT & combat)
{
	if (!combat.defender_ultimate_source)
	{
		ASSERT(combat.defender);
		combat.defender_ultimate_source = UnitGetResponsibleUnit(combat.defender);
	}
	return combat.defender_ultimate_source;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCombatApplyScalingDamage(
	COMBAT & combat,
	int dmg)
{
	if (dmg <= 0)
	{
		return 0;
	}

	ASSERT_RETVAL(combat.attacker && combat.defender, dmg);
	ASSERT_RETVAL(combat.monscaling, dmg);

	UNIT * attacker_source = sCombatGetAttackerUltimateSource(combat);
	ASSERT_RETVAL(attacker_source, dmg);
	UNIT * defender_source = sCombatGetDefenderUltimateSource(combat);
	ASSERT_RETVAL(defender_source, dmg);

	BOOL attacker_is_player = UnitIsA(attacker_source, UNITTYPE_PLAYER);
	BOOL defender_is_player = UnitIsA(defender_source, UNITTYPE_PLAYER);

	// apply scaling for level differences
	dmg = sCombatApplyScalingDamageLevel(combat, dmg, attacker_is_player, defender_is_player);

	// apply damage increment scaling
	dmg = sCombatApplyScalingDamageIncrement(combat, dmg);

	// apply scaling for multi-player
	dmg = sCombatApplyScalingDamageMultiPlayer(combat, dmg, attacker_is_player, defender_is_player);

	// apply scaling for multi-player
	dmg = sCombatApplyScalingDamageEliteMode(combat, dmg, attacker_is_player, defender_is_player);

	return dmg;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingInterruptChance(
	COMBAT & combat,
	float chance,
	int dmg,
	int dmg_avoided)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}

	ASSERT_RETVAL(combat.attacker && combat.defender, chance);
	ASSERT_RETVAL(combat.monscaling, chance);

	UNIT * attacker_source = sCombatGetAttackerUltimateSource(combat);
	ASSERT_RETVAL(attacker_source, chance);
	UNIT * defender_source = sCombatGetDefenderUltimateSource(combat);
	ASSERT_RETVAL(defender_source, chance);

	BOOL attacker_is_player = UnitIsA(attacker_source, UNITTYPE_PLAYER);
	BOOL defender_is_player = UnitIsA(defender_source, UNITTYPE_PLAYER);

	// factor in dmg vs. damage avoided
	if (AppIsHellgate())
	{
		if (dmg > dmg_avoided)
		{
			chance = chance + (chance * (float)dmg / (float)(dmg + dmg_avoided));
		}
		else
		{
			chance = chance - (chance * (float)dmg_avoided / (float)(dmg + dmg_avoided));
		}
	}

	// apply scaling for level differences
	chance = sCombatApplyScalingChanceLevel(combat, chance, attacker_is_player, defender_is_player);

	// apply damage increment scaling
	chance = sCombatApplyScalingChanceIncrement(combat, chance, attacker_is_player, defender_is_player);

	// apply scaling for multi-player
	chance = sCombatApplyScalingChanceMultiPlayer(combat, chance, attacker_is_player, defender_is_player, 0);

	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sCombatApplyScalingSfxChance(
	COMBAT & combat,
	float chance,
	int nPlayerVsMonsterScalingIndex)
{
	if (chance <= EPSILON)
	{
		return 0.0f;
	}

	ASSERT_RETVAL(combat.attacker && combat.defender, chance);
	ASSERT_RETVAL(combat.monscaling, chance);

	UNIT * attacker_source = sCombatGetAttackerUltimateSource(combat);
	ASSERT_RETVAL(attacker_source, chance);
	UNIT * defender_source = sCombatGetDefenderUltimateSource(combat);
	ASSERT_RETVAL(defender_source, chance);

	BOOL attacker_is_player = UnitIsA(attacker_source, UNITTYPE_PLAYER);
	BOOL defender_is_player = UnitIsA(defender_source, UNITTYPE_PLAYER);

	// apply scaling for level differences
	chance = sCombatApplyScalingChanceLevel(combat, chance, attacker_is_player, defender_is_player);

	// apply damage increment scaling
	chance = sCombatApplyScalingChanceIncrement(combat, chance, attacker_is_player, defender_is_player);

	// apply scaling for multi-player
	chance = sCombatApplyScalingChanceMultiPlayer(combat, chance, attacker_is_player, defender_is_player, nPlayerVsMonsterScalingIndex);

	// apply scaling for elite-mode
	chance = sCombatApplyScalingChanceEliteMode(combat, chance, attacker_is_player, defender_is_player);

	// apply scaling for pvp
	chance = sCombatApplyScalingChancePvP(combat, chance, attacker_is_player, defender_is_player);

	return chance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcDamage(
	COMBAT & combat,
	int dmg_min_pct,		// pct values (eg 100)
	int dmg_max_pct,
	int dmg_bonus,
	BOOL bForceFullDMG )			// value in unshifted points (eg 10)
{
	int dmg_roll = 0;
	int dmg_min = PCT(combat.base_min, dmg_min_pct);
	int dmg_max = PCT(combat.base_max, dmg_max_pct);
	if (dmg_max < dmg_min)
	{
		dmg_min = dmg_max = (dmg_min + dmg_max) / 2;	// average of min & max
		dmg_roll = dmg_max;
	}
	else if( !bForceFullDMG )
	{
		dmg_roll = RandGetNum(combat.game, dmg_min, dmg_max);
	}
	else
	{
		dmg_roll = dmg_max;
	}

	int dmg = MAX(0, dmg_roll + (dmg_bonus << StatsGetShift(combat.game, STATS_BASE_DMG_MIN)));

	// apply scaling
	dmg = sCombatApplyScalingDamage(combat, dmg);

	return dmg;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcDamageBonusPercent(
	COMBAT & combat,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data,
	BOOL bIsMelee )
{
	if(TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return 0;
	}
	ASSERT_RETZERO(combat.attacker);
	ASSERT_RETZERO(combat.defender);
	return sCombatCalcDamageBonusPercent(combat.attacker, UnitGetType(combat.defender), dmg_type, combat.delivery_type, bIsMelee);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCombatGetInterruptChanceFirstHitException(
	COMBAT & combat)
{
	if (combat.attacker_ultimate_source == NULL || !UnitIsA(combat.attacker_ultimate_source, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	if (UnitIsA(combat.defender, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return FALSE;
	}
	if (!TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) && 
		!TESTBIT(combat.dwUnitAttackUnitFlags, UAU_DIRECT_MISSILE_BIT))
	{
		return FALSE;
	}

	if (UnitTestFlag(combat.defender, UNITFLAG_HAS_BEEN_HIT))
	{
		return FALSE;
	}
	UnitSetFlag(combat.defender, UNITFLAG_HAS_BEEN_HIT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int CombatGetInterruptChance(
	COMBAT & combat,
	int dmg,
	int dmg_avoided)
{
	ASSERT_RETZERO(combat.attacker);
	ASSERT_RETZERO(combat.defender);

	if (AppIsHellgate() && UnitGetGenus(combat.defender) == GENUS_PLAYER)
	{
		return 0;
	}

	if (!UnitGetCanInterrupt(combat.defender))
	{
		return 0;
	}

	if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_GETHIT))
	{
		return 1;
	}
	else if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_SOFTHIT))
	{
		return 0;
	}

	int base_attack = 0;
	int base_defense = 0;

	// KCK: If the defender has an interrupt chance, just use that - otherwise calculate using
	// attack/defense values
	float chance = (float)UnitGetStat(combat.defender, STATS_INTERRUPT_CHANCE);
	if (chance <= EPSILON)
	{
		base_attack = UnitGetStat(combat.attacker, STATS_INTERRUPT_ATTACK);
		// in Mythos, we don't care about any of their equipped weapon interrupt stats
		// for monsters. But, we might have spells, right? So we need a better solution. But for now.
		if (AppIsTugboat() && UnitGetGenus(combat.attacker) != GENUS_PLAYER)
		{
			base_attack = UnitGetBaseStat(combat.attacker, STATS_INTERRUPT_ATTACK);
		}
		if (base_attack <= 0)
		{
			return 0;
		}
		base_defense = UnitGetStat(combat.defender, STATS_INTERRUPT_DEFENSE);
		base_defense = MAX(1, base_defense);
		chance = (float)base_attack / float(base_attack + base_defense);
		chance = sCombatApplyScalingInterruptChance(combat, chance, dmg, dmg_avoided);
	}

	if (chance <= EPSILON)
	{
		return 0;
	}

	float roll;
	// Mythos doesn't use this, screws up our boss interrupt chances.
	if (AppIsHellgate() && sCombatGetInterruptChanceFirstHitException(combat))
	{
		roll = 1.0f;
	}
	else
	{
		if( (combat.flags & COMBAT_IS_PVP ) )
		{
			chance *= PVP_INTERRUPT_PERCENT;
		}
		roll = RandGetFloat(combat.game);
		if (roll >= chance)
		{
			return 0;
		}
	}

#if ISVERSION(DEVELOPMENT)
	CombatLog(COMBAT_TRACE_INTERRUPT, combat.attacker, "%s interrupted %s (base_att: %d  base_def: %d  dmg: %d  avoided:%d  inc:%d  chance:(%d/%d%%))",
		UnitGetDevName(combat.attacker), UnitGetDevName(combat.defender), base_attack, base_defense, dmg, dmg_avoided,
		combat.damage_increment, (int)(roll * 100), (int)(chance * 100));
#endif

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsUnitImmuneToCritical(
	UNIT * unit)
{
	if (!unit)
	{
		return FALSE;
	}
	const UNIT_DATA * unit_data = UnitGetData(unit);
	if (!unit_data)
	{
		return FALSE;
	}
	return UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_IMMUNE_TO_CRITICAL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
int s_CombatSetCriticalProbabilityOverride(
	int nProbability)
{
	sgnCritProbabilityOverride = nProbability;
	return sgnCritProbabilityOverride;
}
#endif


//----------------------------------------------------------------------------
// return 0 for no critical, 1 for regular critical, 2 for double critical
//----------------------------------------------------------------------------
static inline int CombatCalcCriticalLevel(
	COMBAT & combat)
{
	// phu note: tugboat was using a different crit scheme, but there's no point since tugboat's method can be done in data trivially
	ASSERT_RETZERO(combat.attacker);
	ASSERT_RETZERO(combat.defender);

	if (IsUnitImmuneToCritical(combat.defender))
	{
		return 0;
	}

	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return 0;
	}

	int critical_chance = UnitGetStat(combat.attacker, STATS_CRITICAL_CHANCE);
	if (AppIsTugboat())
	{
		//tugboat wants critical chance to be based off of a weapon type. say unittype bladed or unittype sword1h or unittype sword2h
		critical_chance += UnitGetStatMatchingUnittype( combat.attacker, combat.weapons, MAX_WEAPONS_PER_UNIT, STATS_CRITICAL_CHANCE_UNITTYPE );
	}
	int critical_bonus = CombatGetStatByDeliveryAndCaste(combat.attacker, UnitGetType(combat.defender),
		STATS_CRITICAL_CHANCE_PCT, STATS_CRITICAL_CHANCE_PCT_LOCAL, STATS_CRITICAL_CHANCE_PCT_SPLASH, STATS_CRITICAL_CHANCE_PCT_CASTE, combat.delivery_type);

	critical_chance += PCT(critical_chance, critical_bonus);

	// check defenders critical defense
	int critical_chance_defense = UnitGetStat(combat.defender, STATS_CRITICAL_DEFENSE);
	critical_chance -= PCT( critical_chance, critical_chance_defense );
	if (critical_chance <= 0)
	{
		return 0;	// no chance for crit
	}
	// end check for defender

	float fcritical_chance = PctToFloat(critical_chance);

	// two focus items add up their critical chance in a single attack, so reduce it here
	sCombatGetDualWeaponFocusIncrement(combat);
	if (combat.damage_increment_dualweapon_focus > 0 && combat.damage_increment_dualweapon_focus != DAMAGE_INCREMENT_FULL)
	{
		fcritical_chance = CombatModifyChanceByIncrement(fcritical_chance, PctToFloat(combat.damage_increment_dualweapon_focus));
	}

	fcritical_chance = PIN(fcritical_chance, 0.0f, 0.95f);

#if ISVERSION(CHEATS_ENABLED)
	if (sgnCritProbabilityOverride >= 0)
	{
		fcritical_chance = PctToFloat(sgnCritProbabilityOverride);
	}
#endif

	int critical_level = 0;

	if (fcritical_chance > 0.0f && critical_level < 1 && RandGetFloat(combat.game) < fcritical_chance)
	{
		critical_level++;
	}

	return critical_level;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcCriticalMultiplier(
	COMBAT & combat,
	int critical,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data)
{
	if (critical <= 0)
	{
		return 0;
	}
	ASSERT_RETZERO(combat.attacker);

	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return 0;
	}

	int critical_multiplier = UnitGetStat(combat.attacker, STATS_CRITICAL_MULT) + UnitGetStat(combat.attacker, STATS_CRITICAL_MULT, dmg_type);	// if the stat = 100, that should mean double-damage. 
	if (critical_multiplier <= 0)
	{
		return 0;
	}

	int critical_bonus = UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT) + UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT, dmg_type);
	switch (combat.delivery_type)
	{
	case COMBAT_LOCAL_DELIVERY:
		critical_bonus += UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT_LOCAL) + UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT_LOCAL, dmg_type);
		break;
	case COMBAT_SPLASH_DELIVERY:
		critical_bonus += UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT_SPLASH) + UnitGetStat(combat.attacker, STATS_CRITICAL_MULT_PCT_SPLASH, dmg_type);
		break;
	}

	if (combat.defender)
	{
		UNIT_ITERATE_STATS_RANGE(combat.attacker, STATS_CRITICAL_MULT_PCT_CASTE, caste_bonus, ii, 32)
		{
			if (UnitIsA(combat.defender, caste_bonus[ii].GetParam()))
			{
				critical_bonus += caste_bonus[ii].value;
			}
		}
		UNIT_ITERATE_STATS_END;
	}
	critical_bonus = MAX(critical_bonus, -100);
	critical_multiplier += critical_bonus;

	// two focus items add up their critical multiplier in a single attack, so reduce it here
	sCombatGetDualWeaponFocusIncrement(combat);
	if (combat.damage_increment_dualweapon_focus > 0 && combat.damage_increment_dualweapon_focus != DAMAGE_INCREMENT_FULL)
	{
		critical_multiplier = PCT(critical_multiplier, combat.damage_increment_dualweapon_focus);
	}

	return critical_multiplier * critical;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcShieldInstantDamage(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data)
{
	ASSERT_RETZERO(damage_type_data);

	// phu note: tugboat was returning right away, but there's no point now since it'll return anyway if the defender doesn't have shields
	if (dmg <= 0)
	{
		return 0;
	}
	if ( TESTBIT( combat.dwUnitAttackUnitFlags, UAU_IGNORE_SHIELDS_BIT ) )
	{
		return 0;
	}
	int shields = UnitGetStat(combat.defender, STATS_SHIELDS);
	if (shields <= 0)
	{
		return 0;
	}
	int shields_cur = UnitGetStat(combat.defender, STATS_SHIELD_BUFFER_CUR);
	if (shields_cur <= 0)
	{
		UnitSetStatDelayedTimer(combat.defender, STATS_SHIELD_BUFFER_CUR, 0, 0);
		return 0;
	}
	shields = MIN(shields, shields_cur);
	int dmg_reduction = MIN(dmg, shields);

	// do always kill cheat to penetrate shields
#if ISVERSION(CHEATS_ENABLED)
	if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_KILL))
	{
		dmg_reduction = 0;
	}
#endif

	ASSERT(shields_cur >= dmg_reduction);

	// this is the amount of damage in points that ignores shields
	int shield_penetrate_direct = CombatGetStatByDelivery(combat.attacker, STATS_SHIELD_PENETRATION_DIR,
		STATS_SHIELD_PENETRATION_DIR_LOCAL, STATS_SHIELD_PENETRATION_DIR_SPLASH, combat.delivery_type);
	shield_penetrate_direct = MIN(dmg_reduction, shield_penetrate_direct);

	// this is the percentage of the shield which is ignored by damage
	int shield_penetrate_percent = CombatGetStatByDelivery(combat.attacker, STATS_SHIELD_PENETRATION_PCT,
		STATS_SHIELD_PENETRATION_PCT_LOCAL, STATS_SHIELD_PENETRATION_PCT_SPLASH, combat.delivery_type);
	shield_penetrate_percent = PCT(dmg_reduction, shield_penetrate_percent);

	// this is how much dmg_reduction is bypassed by penetration
	int shield_penetrate_total = MIN(dmg_reduction, shield_penetrate_direct + shield_penetrate_percent);
	dmg_reduction -= shield_penetrate_total;
	dmg_reduction = MAX(0, dmg_reduction);

	// this is the percent extra damage done to shields by the damage that doesn't penetrate shields
	int shield_overload = CombatGetStatByDelivery(combat.attacker, STATS_SHIELD_OVERLOAD_PCT,
		STATS_SHIELD_OVERLOAD_PCT_LOCAL, STATS_SHIELD_OVERLOAD_PCT_SPLASH, combat.delivery_type);
	int dmg_vs_shields = PCT(dmg_reduction, 100 + shield_overload);
	int extra_damage_after_shields = MAX(0, dmg_vs_shields - shields_cur);
	int shields_new = MAX(0, shields_cur - dmg_vs_shields);

	UnitSetStat(combat.defender, STATS_SHIELD_BUFFER_CUR, shields_new);

	if (extra_damage_after_shields > 0 && shield_overload > -100)
	{
		int damage_applied_after_shields = extra_damage_after_shields * 100 / (100 + shield_overload);
		dmg_reduction -= damage_applied_after_shields;
		dmg_reduction = MAX(0, dmg_reduction);
	}

#if ISVERSION(DEVELOPMENT)
	CombatLog(COMBAT_TRACE_SHIELDS, combat.defender, "[%s shields]  %3.1f %s damage  red:%3.1f  pen:%3.1f  load:%3.1f  buf:%3.1f",
		UnitGetDevName(combat.defender), (float)dmg/256.0f, damage_type_data->szDamageType,
		(float)dmg_reduction/256.0f, (float)shield_penetrate_total/256.0f, (float)dmg_vs_shields/256.0f,
		(float)shields_new/256.0f);
#endif

	return dmg_reduction;
}


//----------------------------------------------------------------------------
// returns how much to reduce damage by
//----------------------------------------------------------------------------
static inline int CombatCalcArmorInstantDamage(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data,
	int armor_type)
{
	ASSERT_RETZERO(damage_type_data);

	// early-out: damage is <= 0
	if (dmg <= 0)
	{
		return 0;
	}

	// get armor (armor = armor + sfx_defense)
	int armor = UnitGetStat(combat.defender, STATS_ARMOR, armor_type);
	int nArmorPercent = UnitGetStat(combat.defender, STATS_ARMOR_PCT_BONUS );
	// Armor percent bonus is taken care of using statsfunc.  This causes armor percent bonus to be included twice.
	//armor += PCT(armor, nArmorPercent);
	int effective_armor = armor;
	if (effective_armor <= 0)
	{
		return 0;
	}

	// compute amount of damage subject to reduction (that is, damage - armor penetration)
	int reducible_dmg = dmg;

	int armor_penetrate_direct = CombatGetStatByDamageEffectAndDelivery(combat.attacker, STATS_ARMOR_PENETRATION_DIR,
		STATS_ARMOR_PENETRATION_DIR_LOCAL, STATS_ARMOR_PENETRATION_DIR_SPLASH, dmg_type, combat.delivery_type);
	armor_penetrate_direct = MIN(reducible_dmg, armor_penetrate_direct);
	reducible_dmg -= armor_penetrate_direct;

	int armor_penetrate_percent = CombatGetStatByDamageEffectAndDelivery(combat.attacker, STATS_ARMOR_PENETRATION_PCT,
		STATS_ARMOR_PENETRATION_PCT_LOCAL, STATS_ARMOR_PENETRATION_PCT_SPLASH, dmg_type, combat.delivery_type);
	armor_penetrate_percent = PCT(reducible_dmg, armor_penetrate_percent);
	reducible_dmg -= armor_penetrate_percent;
	if (reducible_dmg <= 0)
	{
		return 0;
	}
#if ISVERSION(DEVELOPMENT)
	int armor_penetrate_total = MIN(dmg, armor_penetrate_direct + armor_penetrate_percent);
#endif

	// compute damage reduction (armor / (armor + attack_rating))
	int armor_cur = UnitGetStat(combat.defender, STATS_ARMOR_BUFFER_CUR, armor_type);

	float dmg_reduction_pct = (float)(effective_armor) / (float)(effective_armor + combat.attack_rating);
	int dmg_reduction = (int)((float)reducible_dmg * dmg_reduction_pct);
	dmg_reduction = MIN(dmg_reduction, reducible_dmg);
	dmg_reduction = MIN(dmg_reduction, armor_cur);
	if (dmg_reduction <= 0)
	{
		return 0;
	}

	// compute dmg vs. armor buffer by factoring in overload
	int armor_overload = CombatGetStatByDamageEffectAndDeliveryAndCaste(combat.attacker, UnitGetType(combat.defender),
		STATS_ARMOR_OVERLOAD_PCT, STATS_ARMOR_OVERLOAD_PCT_LOCAL, STATS_ARMOR_OVERLOAD_PCT_SPLASH,
		STATS_ARMOR_OVERLOAD_PCT_CASTE, INVALID_ID, dmg_type, combat.delivery_type);
	armor_overload = MAX(0, armor_overload);

	int dmg_vs_armor = PCT(dmg_reduction, 100 + armor_overload - nArmorPercent);

	// compute new armor buffer
	int armor_new = MAX(0, armor_cur - dmg_vs_armor);
	UnitSetStat(combat.defender, STATS_ARMOR_BUFFER_CUR, armor_type, armor_new);

	// apply spectral phased defense reduction
	int defense_reduction_pct = UnitGetStat(combat.defender, STATS_DEFENSE_REDUCTION_PERCENT);
	if (defense_reduction_pct != 0)
	{
		dmg_reduction += PCT(dmg_reduction, defense_reduction_pct);
	}

#if ISVERSION(DEVELOPMENT)
	CombatLog(COMBAT_TRACE_ARMOR, combat.defender, "[%s armor]  %3.1f %s damage  red:%3.1f  pen:%3.1f  load:%3.1f  buf:%3.1f",
		UnitGetDevName(combat.defender), (float)dmg/256.0f, damage_type_data->szDamageType,
		(float)dmg_reduction/256.0f, (float)armor_penetrate_total/256.0f, (float)dmg_vs_armor/256.0f,
		(float)armor_new/256.0f);
#endif
	return dmg_reduction;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcVulnerability(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data)
{
	int vulnerability = UnitGetStat(combat.defender, STATS_ELEMENTAL_VULNERABILITY) + UnitGetStat(combat.defender, STATS_ELEMENTAL_VULNERABILITY, dmg_type);

	if( AppIsTugboat() && (combat.flags & COMBAT_IS_PVP ) )
	{
		vulnerability = MIN(vulnerability, PVP_VULNERABILITY_MAX);
		vulnerability = MAX(vulnerability, PVP_VULNERABILITY_MIN);
	}
	else
	{
		vulnerability = MAX(vulnerability, -100);
	}
	return vulnerability;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int CombatCalcSecondaryVulnerability(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	const DAMAGE_TYPE_DATA * damage_type_data)
{
	int vulnerability = 0;
	//defender vulnerability to attacker by unittype.
	// TRAVIS: I'm appistugboating this.
	if( AppIsTugboat() )
	{
		// AT present this is really used for player vs player damage reduction, which also applies to player pets.
		// this logic may heavily change.
		STATS_ENTRY tStatsUnitTypes[ 20 ]; //hard coded for now. Testing..
		int nUnitTypeCounts = UnitGetStatsRange( combat.defender, STATS_DEFENDERS_VULNERABILITY_ATTACKER, 0, tStatsUnitTypes, 20 );	
		int nAdditionalVulnerability( 0 );
		for( int t = 0; t < nUnitTypeCounts; t++ )
		{
			int nUnitType = STAT_GET_PARAM( tStatsUnitTypes[ t ].stat );			
			if( nUnitType != INVALID_ID )
			{				
				if( UnitIsA( combat.attacker, nUnitType ) ||
					UnitIsA( combat.attacker_ultimate_source, nUnitType ))
				{
					nAdditionalVulnerability += tStatsUnitTypes[ t ].value;	
				}
			}
		}			
		vulnerability += nAdditionalVulnerability;
	}

	vulnerability = MAX(vulnerability, -90);
	return vulnerability;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void COMBAT_DEBUG(
	UNIT *pUnit,
	const char *pszLabel,
	int nValue)
{
//	if (pUnit->unitid == 0)
	{
		GAME *pGame = UnitGetGame( pUnit );
		SVR_VERSION_ONLY(REF(pGame));
		ConsoleString(
			"[%d] (%s) - %s: %d",
			GameGetTick( pGame ),
			UnitGetDevName( pUnit ),
			pszLabel,
			nValue);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum SFX_STAT_TYPE
{
	SST_ATTACK,
	SST_ATTACK_LOCAL,
	SST_ATTACK_SPLASH,
	SST_ATTACK_PCT,
	SST_ATTACK_PCT_LOCAL,
	SST_ATTACK_PCT_SPLASH,
	SST_ATTACK_PCT_CASTE,
	SST_DEFENSE,
	SST_EFFECT_DEFENSE,
	SST_EFFECT_DEFENSE_PCT,
};

static inline int sCombatGetStatIndex(
	const DAMAGE_EFFECT_DATA * pDamageEffect,
	SFX_STAT_TYPE eType)
{
	if(pDamageEffect)
	{
		if(pDamageEffect->bUsesOverrideStats)
		{
			switch(eType)
			{
			case SST_ATTACK:
				return pDamageEffect->nAttackStat;
			case SST_ATTACK_LOCAL:
				return pDamageEffect->nAttackLocalStat;
			case SST_ATTACK_SPLASH:
				return pDamageEffect->nAttackSplashStat;
			case SST_ATTACK_PCT:
				return pDamageEffect->nAttackPctStat;
			case SST_ATTACK_PCT_LOCAL:
				return pDamageEffect->nAttackPctLocalStat;
			case SST_ATTACK_PCT_SPLASH:
				return pDamageEffect->nAttackPctSplashStat;
			case SST_ATTACK_PCT_CASTE:
				return pDamageEffect->nAttackPctCasteStat;
			case SST_DEFENSE:
				return pDamageEffect->nDefenseStat;
			case SST_EFFECT_DEFENSE:
				return pDamageEffect->nEffectDefenseStat;
			case SST_EFFECT_DEFENSE_PCT:
				return pDamageEffect->nEffectDefensePctStat;
			}
		}
		else
		{
			switch(eType)
			{
			case SST_ATTACK:
				return STATS_SFX_EFFECT_ATTACK;
			case SST_ATTACK_LOCAL:
				return STATS_SFX_EFFECT_ATTACK_LOCAL;
			case SST_ATTACK_SPLASH:
				return STATS_SFX_EFFECT_ATTACK_SPLASH;
			case SST_ATTACK_PCT:
				return STATS_SFX_EFFECT_ATTACK_PCT;
			case SST_ATTACK_PCT_LOCAL:
				return STATS_SFX_EFFECT_ATTACK_PCT_LOCAL;
			case SST_ATTACK_PCT_SPLASH:
				return STATS_SFX_EFFECT_ATTACK_PCT_SPLASH;
			case SST_ATTACK_PCT_CASTE:
				return STATS_SFX_EFFECT_ATTACK_PCT_CASTE;
			case SST_DEFENSE:
				return STATS_SFX_DEFENSE;
			case SST_EFFECT_DEFENSE:
				return STATS_SFX_EFFECT_DEFENSE;
			case SST_EFFECT_DEFENSE_PCT:
				return STATS_SFX_DEFENSE_PCT;
			}
		}
	}
	else
	{
		switch(eType)
		{
		case SST_ATTACK:
			return STATS_SFX_ATTACK;
		case SST_ATTACK_LOCAL:
			return STATS_SFX_ATTACK_LOCAL;
		case SST_ATTACK_SPLASH:
			return STATS_SFX_ATTACK_SPLASH;
		case SST_ATTACK_PCT:
			return STATS_SFX_ATTACK_PCT;
		case SST_ATTACK_PCT_LOCAL:
			return STATS_SFX_ATTACK_PCT_LOCAL;
		case SST_ATTACK_PCT_SPLASH:
			return STATS_SFX_ATTACK_PCT_SPLASH;
		case SST_ATTACK_PCT_CASTE:
			return STATS_SFX_ATTACK_PCT_CASTE;
		case SST_DEFENSE:
			return STATS_SFX_DEFENSE;
		case SST_EFFECT_DEFENSE:
			return STATS_SFX_EFFECT_DEFENSE;
		case SST_EFFECT_DEFENSE_PCT:
			return STATS_SFX_DEFENSE_PCT;
		}
	}
	return INVALID_ID;
}


//----------------------------------------------------------------------------
// Roll for special effect
//----------------------------------------------------------------------------
static inline BOOL RollForSpecialEffect(
	COMBAT &combat,
	int dmg_type,			//dmg type index
	int dmg_effect,			//effect index
	const DAMAGE_TYPE_DATA *   pDamageType,
	const DAMAGE_EFFECT_DATA * pDamageEffect,
	char *szString,
	int nStringSize )
{
	//early outs
	if( !pDamageType && !pDamageEffect )
		return FALSE;

	if(pDamageType && pDamageType->nInvulnerableSFXState >= 0)
	{
		if(UnitHasState(combat.game, combat.defender, pDamageType->nInvulnerableSFXState))
		{
			return FALSE;
		}
	}
	//done early outs

	// this is the attacker's chance to cause the effect

	int sfx_attack = CombatGetStatByDamageEffectAndDelivery(
					combat.attacker,
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_LOCAL),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_SPLASH),
					( pDamageEffect ) ? dmg_effect : dmg_type,
					combat.delivery_type);

	int sfx_attack_pct = CombatGetStatByDamageEffectAndDeliveryAndCaste(
					combat.attacker,
					UnitGetType(combat.defender),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_PCT),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_PCT_LOCAL),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_PCT_SPLASH),
					sCombatGetStatIndex(pDamageEffect, SST_ATTACK_PCT_CASTE),
					INVALID_ID,
					( pDamageEffect ) ? dmg_effect : dmg_type,
					combat.delivery_type);

	sfx_attack += PCT(sfx_attack, sfx_attack_pct);
	// this is the defender's chance to stop the effect ... see statsfunc.xls to see how
	// the bonus formula is incorporated into this
	int sfx_defense = (pDamageEffect && pDamageEffect->bDoesntUseSFXDefense) ? 0 :
					UnitGetStat( combat.defender, sCombatGetStatIndex(pDamageEffect, SST_DEFENSE) ) +
					  UnitGetStat( combat.defender, sCombatGetStatIndex(pDamageEffect, SST_DEFENSE), dmg_type );
	if( dmg_effect > 0 )
		sfx_defense = UnitGetStat( combat.defender, sCombatGetStatIndex(pDamageEffect, SST_EFFECT_DEFENSE) ) +
					  UnitGetStat( combat.defender, sCombatGetStatIndex(pDamageEffect, SST_EFFECT_DEFENSE), dmg_effect );

	int nDefensePctIndex = sCombatGetStatIndex(pDamageEffect, SST_EFFECT_DEFENSE_PCT);
	if(nDefensePctIndex >= 0)
	{
		int nEffectDefensePct = 100 + UnitGetStat( combat.defender, nDefensePctIndex );
		sfx_defense = PCT( sfx_defense, nEffectDefensePct );
	}

	int nPlayerVsMonsterScalingIndex = pDamageEffect ? pDamageEffect->nPlayerVsMonsterScalingIndex : 0;

	ASSERTX_RETFALSE(combat.damage_increment > 0, UnitGetDevName(combat.attacker));
	float sfx_chance = 0.0f;
	if ((sfx_attack > 0) && ((sfx_attack + sfx_defense) > 0))
	{
		sfx_chance = (float)sfx_attack / (float)(sfx_attack + sfx_defense);
		sfx_chance = sCombatApplyScalingSfxChance(combat, sfx_chance, nPlayerVsMonsterScalingIndex);
	}

	float roll = RandGetFloat(combat.game);

#if ISVERSION(DEVELOPMENT)
	if (gnSfxChanceOverride != -1)
	{
		roll = 1.0f - (float)gnSfxChanceOverride / 100.0f;
	}
	if (roll < sfx_chance) //succesful roll
	{
		if (szString)
		{			
			if (dmg_type >= 0)
			{
				PStrPrintf(szString, nStringSize, "%s %s did %s to %s (sfx_attack:%d  sfx_defense:%d  dmg_increment:%d  chance:%d%%  roll:%d)", 
					szString, UnitGetDevName(combat.attacker), ExcelGetStringIndexByLine(combat.game, DATATABLE_DAMAGETYPES, dmg_type),
					UnitGetDevName(combat.defender), sfx_attack, sfx_defense, combat.damage_increment, (int)(sfx_chance * 100.0f), (int)(roll * 100.0f));
			}
			else if (dmg_effect >= 0)
			{
				PStrPrintf(szString, nStringSize, "%s\n%s effect (sfx_attack:%d  sfx_defense:%d  dmg_increment:%d  chance:%d%%  roll:%d )", 
					szString, ExcelGetStringIndexByLine(combat.game, DATATABLE_DAMAGE_EFFECTS, dmg_effect), sfx_attack, sfx_defense, combat.damage_increment,
					(int)(sfx_chance * 100.0f), (int)(roll * 100.0f));
			}
		}
		return TRUE;
	}
	return FALSE;
#else
	return (roll < sfx_chance); //succesful roll
#endif
}


//----------------------------------------------------------------------------
// Do actual effect
//----------------------------------------------------------------------------
static inline BOOL CombatDoActualSpecialEffect(
	COMBAT &combat,
	int dmg,
	int dmg_type,			//dmg type index
	int dmg_effect,			//effect index
	const DAMAGE_EFFECT_DATA * pDamageEffect,
	char *szString,
	int nStringSize )
{
	//early outs
	if( !pDamageEffect )
		return FALSE;
	//done early out
	// base duration


	// add any duration via a stat
	int durationInMS = (dmg_type > 0) ? UnitGetStat(combat.attacker, STATS_SFX_DURATION_IN_TICKS, dmg_type) : 0;
	if ( !durationInMS && pDamageEffect->nDefaultDuration )
		durationInMS = pDamageEffect->nDefaultDuration;
	durationInMS = (durationInMS * MSECS_PER_GAME_TICK);

	int duration_pct = CombatGetStatByDamageEffectAndCaste(	combat.attacker,
															UnitGetType(combat.defender),
															STATS_SFX_DURATION_PCT,
															STATS_SFX_DURATION_PCT_CASTE,
															dmg_type);
	duration_pct = MAX(duration_pct, -90);

	int nCodeLenDuration = 0;
	BYTE * codeDuration = NULL;
	if (!UnitGetStat( combat.attacker, STATS_SFX_DURATION_IGNORE_DAMAGEEFFECTS_DURATION, dmg_type))
		codeDuration = ExcelGetScriptCode(combat.game, DATATABLE_DAMAGE_EFFECTS, pDamageEffect->codeSfxDuration, &nCodeLenDuration);

	if (codeDuration )
	{
		durationInMS += VMExecI( combat.game, combat.attacker, combat.defender, NULL, dmg, duration_pct + 100, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, codeDuration, nCodeLenDuration);
	}
	else
	{
		durationInMS += PCT(durationInMS, duration_pct);
	}

	int duration_pct_defense = CombatGetStatByDamageEffectAndCaste(	combat.defender,
		UnitGetType(combat.attacker),
		STATS_SFX_DURATION_PCT_DEFENSE,
		STATS_SFX_DURATION_PCT_CASTE_DEFENSE,
		dmg_type);
	duration_pct_defense = MIN(duration_pct_defense, 90);
	durationInMS -= PCT(durationInMS, duration_pct_defense);

	if (durationInMS <= 0)
	{
		durationInMS = 1;
	}

	// get duration in ticks
	int nDurationInTicks = GAME_TICKS_PER_SECOND * durationInMS / MSECS_PER_SEC;

	// log special effect
	TraceCombat("*Special Effect* Attacker unitid %d did effect '%s(%s)' to defender unitid %d",
					UnitGetId(combat.attacker),
					ExcelGetStringIndexByLine(combat.game, DATATABLE_DAMAGE_EFFECTS, dmg_effect),
					ExcelGetStringIndexByLine(combat.game, DATATABLE_DAMAGETYPES, dmg_type),
					UnitGetId(combat.defender));

#if ISVERSION(DEVELOPMENT)
		if( szString && dmg_type >= 0 )
		{						
			PStrPrintf( szString, nStringSize, "%s duration:%d ticks DMG:%s", 
				szString,
				nDurationInTicks,
				ExcelGetStringIndexByLine(combat.game, DATATABLE_DAMAGETYPES, dmg_type) );		

			CombatLog(COMBAT_TRACE_SFX, combat.attacker, "%s", szString );
		}

#endif
	// set special effect state for the duration
	STATS * stats = NULL;
	int nAttackSkill = UnitGetStat( combat.attacker, STATS_SFX_EFFECT_SKILL, dmg_effect );
	nAttackSkill = ( nAttackSkill == INVALID_ID )?pDamageEffect->nAttackerSkill:nAttackSkill;
	nAttackSkill = ( nAttackSkill == INVALID_ID && UnitGetGenus( combat.attacker ) == GENUS_MISSILE )?UnitGetStat( combat.attacker_ultimate_source, STATS_SFX_EFFECT_SKILL, dmg_effect ):nAttackSkill;
	if ( nAttackSkill!= INVALID_ID )
	{
		UNIT *attacker( combat.attacker );
		if( AppIsTugboat() && UnitGetGenus( combat.attacker ) == GENUS_MISSILE )
		{
			attacker = combat.attacker_ultimate_source;
		}
		if( attacker )
		{
			SkillStartRequest( combat.game, attacker, nAttackSkill, UnitGetId( combat.defender ), UnitGetPosition( combat.defender ), 
				SKILL_START_FLAG_INITIATED_BY_SERVER, SkillGetNextSkillSeed( combat.game ) );
		}
	}

	int nTargetSkill = UnitGetStat( combat.attacker, STATS_SFX_EFFECT_SKILL_ON_TARGET, dmg_effect );
	nTargetSkill = ( nTargetSkill == INVALID_ID ) ? pDamageEffect->nTargetSkill : nTargetSkill;
	nTargetSkill = ( nTargetSkill == INVALID_ID && UnitGetGenus( combat.attacker ) == GENUS_MISSILE ) ? UnitGetStat( combat.attacker_ultimate_source, STATS_SFX_EFFECT_SKILL_ON_TARGET, dmg_effect ) : nTargetSkill;
	if ( nTargetSkill != INVALID_ID )
	{
		UNIT *target = combat.defender;
		if( target )
		{
			UNIT *aiTarget = NULL;
			SkillSetTarget( target, nTargetSkill, INVALID_ID, UnitGetId( combat.attacker ) );
			if( UnitGetGenus( target ) == GENUS_MONSTER )
			{
				aiTarget = UnitGetAITarget( target );
				UnitSetAITarget( target, combat.attacker, TRUE ); 
			}
			SkillStartRequest( combat.game, target, nTargetSkill, UnitGetId( UnitGetUltimateOwner( combat.attacker ) ), UnitGetPosition( combat.attacker ), 
				SKILL_START_FLAG_INITIATED_BY_SERVER, SkillGetNextSkillSeed( combat.game ) );
			if( aiTarget )
			{
				UnitSetAITarget( target, aiTarget, TRUE );
			}
		}
	}

	// run the special effect code
	int nStrength = CombatGetStatByDamageEffectAndCaste(
		combat.attacker,
		UnitGetType(combat.defender),
		STATS_SFX_STRENGTH_PCT,
		STATS_SFX_STRENGTH_PCT_CASTE,
		dmg_type );

	if ( ! nStrength && pDamageEffect->nDefaultStrength )
		nStrength = pDamageEffect->nDefaultStrength;

	nStrength = MAX(nStrength, -90);

	if (pDamageEffect->nSfxState != INVALID_ID)
	{
		// Ask Tyler why this can't be zero.
		// This can't be zero because otherwise the state will never go away.  s_StateSet(blah, 0) means "set this state forever until I explicitly clear it"
		if (nDurationInTicks <= 0)
		{
			return FALSE;
		}

		UNIT *pStateSource = ( pDamageEffect->bDontUseUltimateAttacker ) ? combat.attacker : combat.attacker_ultimate_source;
		stats = s_StateSet(combat.defender, pStateSource, pDamageEffect->nSfxState, nDurationInTicks);
		if (!stats)
		{
			return FALSE;
		}

		// stun lock prevention. set the invulnerability state for the duration of the stun plus the duration times a multiplier (if the mult is non zero)
		const DAMAGE_TYPE_DATA* damage_type_data = DamageTypeGetData(combat.game, dmg_type);
		if (damage_type_data)
		{
			if (AppIsHellgate() && damage_type_data->fSFXInvulnerabilityDurationMultInPVPHellgate > 0.0f)
			{
				s_StateSet(combat.defender, pStateSource, damage_type_data->nInvulnerableSFXState, nDurationInTicks + PrimeFloat2Int(float(nDurationInTicks) * damage_type_data->fSFXInvulnerabilityDurationMultInPVPHellgate));
			}
			else if (AppIsTugboat() && damage_type_data->fSFXInvulnerabilityDurationMultInPVPTugboat > 0.0f)
			{
				s_StateSet(combat.defender, pStateSource, damage_type_data->nInvulnerableSFXState, nDurationInTicks + PrimeFloat2Int(float(nDurationInTicks) * damage_type_data->fSFXInvulnerabilityDurationMultInPVPTugboat));
			}

		}

		UNITID idMissile = StatsGetStat( stats, STATS_STATE_MISSILE_ID, 0 );
		if ( idMissile != INVALID_ID )
		{
			ONCE
			{
				ASSERT_DO(idMissile != UnitGetId(combat.attacker))
				{
					TraceError("Attempting to free missile with UnitID %d when missile is attacker.\n", idMissile);
					break;
				}
				ASSERT_DO(idMissile != UnitGetId(combat.attacker_ultimate_source))
				{
					TraceError("Attempting to free missile with UnitID %d when missile is attacker_ultimate_source.\n", idMissile);
					break;
				}
				ASSERT_DO(idMissile != UnitGetId(combat.defender))
				{
					TraceError("Attempting to free missile with UnitID %d when missile is defender.\n", idMissile);
					break;
				}
				UNIT * pMissile = UnitGetById( combat.game, idMissile );
				if ( pMissile )
					UnitFree( pMissile );
			}
			StatsClearStat( combat.game, stats, STATS_STATE_MISSILE_ID, 0 );
		}

		if ( pDamageEffect->nMissileToAttach != INVALID_ID )
		{
			UNIT * pMissile = MissileFire( combat.game, pDamageEffect->nMissileToAttach, combat.attacker_ultimate_source, NULL,
				INVALID_ID, NULL, VECTOR(0), UnitGetPosition( combat.defender ), UnitGetMoveDirection( combat.defender ), 0, 0 );

			if ( pMissile )
			{
				UnitAttachToUnit( pMissile, combat.defender );
				int nCodeLenMissile = 0;
				BYTE * codeMissile = ExcelGetScriptCode(combat.game, DATATABLE_DAMAGE_EFFECTS, pDamageEffect->codeMissileStats, &nCodeLenMissile);
				if ( codeMissile )
					VMExecI(combat.game, pMissile, combat.attacker, NULL, dmg, nStrength, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, codeMissile, nCodeLenMissile);

				StatsSetStat( combat.game, stats, STATS_STATE_MISSILE_ID, 0, UnitGetId( pMissile ) );
			}
		}
	}

	// save who the ultimate source of this special effect is
	UnitSetStat(combat.defender, STATS_SFX_STATE_GIVEN_BY, dmg_type, UnitGetId(combat.attacker_ultimate_source));
	UnitSetStat(combat.defender, STATS_SFX_EFFECT_GIVEN_BY, dmg_effect, UnitGetId(combat.attacker_ultimate_source));

	// save the attacker and weapon of the combat special effect (if no weapon, the weapon is the attacker)
	UnitSetStat(combat.defender, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_ATTACKER, 0, UnitGetId(combat.attacker));
	UnitSetStat(combat.defender, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_WEAPON, 0, UnitGetId(combat.weapons[ 0 ]));
	UnitSetStat(combat.defender, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_WEAPON, 1, UnitGetId(combat.weapons[ 1 ]));

	if( pDamageEffect > 0 && stats )
	{
		int nCodeLenSfx = 0;
		BYTE * codeSfx = ExcelGetScriptCode(combat.game, DATATABLE_DAMAGE_EFFECTS, pDamageEffect->codeBaseSfxEffect, &nCodeLenSfx);
		if (codeSfx)
		{
			VMExecI(combat.game, combat.attacker, combat.defender, stats, dmg, nStrength, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, codeSfx, nCodeLenSfx);
		}
	}

	// clear combat special effect instrument
	UnitClearStat(combat.defender, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_ATTACKER, 0);
	UnitClearStat(combat.defender, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_WEAPON, 0);

	return TRUE;
}


//----------------------------------------------------------------------------
//chance for damage effect occur after the roll
//----------------------------------------------------------------------------
static inline BOOL sCombatRollForDmgEffectChance( COMBAT & combat,
												  int nEffectType )
{
	if( AppIsTugboat() )
	{
		float fAttackerChance = (float)UnitGetStat( combat.attacker, STATS_SKILL_EFFECT_CHANCE, nEffectType );
		if( fAttackerChance <= 0.0f )
			return FALSE;
		//lets go ahead and scale the chances here...
		int attacker_level = UnitGetExperienceLevel( combat.attacker_ultimate_source);
		int defender_level = UnitGetExperienceLevel( combat.defender);	// not defender_source!
		const LEVEL_SCALING_DATA * level_scaling_data = CombatGetLevelScalingData(combat.game, attacker_level, defender_level);
		ASSERT_RETFALSE(level_scaling_data );
		float fPercent( 100.0f );
		if( UnitGetGenus( combat.attacker_ultimate_source ) == GENUS_MONSTER )
		{
			fPercent = (float)level_scaling_data->nMonsterAttacksPlayerDmgEffect;
		}
		else
		{
			if( UnitGetGenus( combat.attacker_ultimate_source ) == GENUS_PLAYER &&
				UnitGetGenus( combat.defender_ultimate_source ) == GENUS_PLAYER )
			{
				fPercent = (float)level_scaling_data->nPlayerAttacksPlayerDmgEffect;
			}
			else
			{
				fPercent = (float)level_scaling_data->nPlayerAttacksMonsterDmgEffect;
			}
		}
#if ISVERSION(DEBUG_VERSION)
		int nParam0 = UnitGetStat( combat.attacker_ultimate_source, STATS_SFX_EFFECT_PARAM0 );
		int nParam1 = UnitGetStat( combat.attacker_ultimate_source, STATS_SFX_EFFECT_PARAM1 );
		int nParam2 = UnitGetStat( combat.attacker_ultimate_source, STATS_SFX_EFFECT_PARAM2 );
		REF( nParam0 );
		REF( nParam1 );
		REF( nParam2 );
#endif
		//get the percent chance based off of level
		fAttackerChance *= (float)fPercent/100.0f;
		//lets do the roll here
		if( (fAttackerChance/1000.0f) < RandGetFloat( combat.game, 0, 1.0f ) )
		{
			return FALSE;	//failed the roll here.
		}

	}
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sCombatEvaluateSpecialEffect(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	int nEffectType,
	BOOL bCritical,
	BOOL parentDmgTypeSuccess,
	char *szString,
	int nStringSize)
{
	const DAMAGE_EFFECT_DATA * pDamageEffect = DamageEffectGetData(combat.game, nEffectType );
	ASSERT_RETFALSE(pDamageEffect);
	
	if( pDamageEffect->bDoesntRequireDamage == FALSE )
	{
		if((dmg <= 0 && !pDamageEffect->bRequiresNoDamage) || (dmg > 0 && pDamageEffect->bRequiresNoDamage))
			return FALSE;
	}

	if( pDamageEffect->nDamageTypeID == dmg_type  )
	{
		BOOL rollSuccessful = pDamageEffect->bNoRollNeeded || parentDmgTypeSuccess;	//the roll is successful
		//some early outs
		// Check invulnerable first
		if( pDamageEffect->nInvulnerableState != INVALID_ID &&
			UnitHasState( combat.game, combat.defender, pDamageEffect->nInvulnerableState ) )
			return FALSE;
		if( pDamageEffect->bMustBeCrit &&
			bCritical == FALSE )
			return FALSE;	//must be a crit to do state.
		if( pDamageEffect->bMonsterMustDie )
		{
			int hp_cur = UnitGetStat(combat.defender, STATS_HP_CUR);
			if(hp_cur > dmg )
			{
				return FALSE; //monster must be dieing
			}
		}
		if( pDamageEffect->nAttackerProhibitingState != INVALID_ID &&
			UnitHasState( combat.game, combat.attacker, pDamageEffect->nAttackerProhibitingState ) )
			return FALSE;
		if( pDamageEffect->nDefenderProhibitingState != INVALID_ID &&
			UnitHasState( combat.game, combat.defender, pDamageEffect->nDefenderProhibitingState ) )
			return FALSE;
		
		if( pDamageEffect->nAttackerRequiresState != INVALID_ID &&
			!UnitHasState( combat.game, combat.attacker, pDamageEffect->nAttackerRequiresState ) )			 
		{
			if( AppIsTugboat() )
			{
				if( combat.weapons[0] != NULL &&			
					!UnitHasState( combat.game, combat.weapons[0], pDamageEffect->nAttackerRequiresState ) )			 
					return FALSE;			
			}
			else
			{
				return FALSE;
			}
		}

		if( pDamageEffect->nDefenderRequiresState != INVALID_ID &&
			!UnitHasState( combat.game, combat.defender, pDamageEffect->nDefenderRequiresState ) )
			return FALSE;
		//end early outs
		//chance rolls
		if( !pDamageEffect->bDontUseEffectChance )
		{
			if(	!sCombatRollForDmgEffectChance( combat, nEffectType ) )
			{
				return FALSE;
			}
		}


		//we need to check the conditional
		int nCodeConditional = 0;
		BYTE *codeConditional = ExcelGetScriptCode(combat.game, DATATABLE_DAMAGE_EFFECTS, pDamageEffect->codeConditional, &nCodeConditional);
		if ( codeConditional )			
			if( VMExecI(combat.game, combat.attacker, combat.defender, NULL, codeConditional, nCodeConditional ) == 0 )				
				return FALSE;							
		//conditional done
		if( !pDamageEffect->bUseParentsRoll &&
			!pDamageEffect->bNoRollNeeded )				//some effects don't need the parent type to be successfull
		{
			rollSuccessful = RollForSpecialEffect( combat, -1, nEffectType, NULL, pDamageEffect, szString, nStringSize );
		}
		if( rollSuccessful )
		{
			CombatDoActualSpecialEffect( combat, dmg, dmg_type, nEffectType, pDamageEffect, szString, nStringSize );
		}
		return rollSuccessful;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL CombatDoSpecialEffect(
	COMBAT & combat,
	int dmg,
	int dmg_type,
	BOOL bCritical = FALSE )
{
	//A few early outs
	ASSERT_RETFALSE(combat.attacker && combat.defender);
	if (UnitIsInvulnerable( combat.defender ) ||
		(combat.flags & COMBAT_DONT_ALLOW_DAMAGE_EFFECTS) ||
		TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT)
		)
	{
		return FALSE;
	}

#if ISVERSION(DEVELOPMENT)
	const int nStringSize = 2048;
	char szString[ nStringSize ];
	ZeroMemory( szString, sizeof( char ) * nStringSize );
#else
	const int nStringSize = 0;
	char * szString = NULL;
#endif

	BOOL parentDmgTypeSuccess = FALSE;
	if (dmg > 0 && 
		dmg_type != INVALID_ID )
	{
		//end early outs
		const DAMAGE_TYPE_DATA * pDamageType = DamageTypeGetData(combat.game, dmg_type );
		ASSERT_RETFALSE(pDamageType);

		//first roll the parent damage type
		parentDmgTypeSuccess = RollForSpecialEffect( combat, dmg_type, -1, pDamageType, NULL, szString, nStringSize );
	}

	BOOL returnValue = parentDmgTypeSuccess;	//the return value
	int nEffectsCount = ExcelGetNumRows( combat.game, DATATABLE_DAMAGE_EFFECTS );
	//so lets walk through and do the effects based off the damage type.
	for( int eIndex = 0; eIndex < nEffectsCount; eIndex++ )
	{
		BOOL rollSuccessful = sCombatEvaluateSpecialEffect(combat, dmg, dmg_type, eIndex, bCritical, parentDmgTypeSuccess, szString, nStringSize);
		returnValue = returnValue || rollSuccessful;
	}

	return returnValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_CombatExplicitSpecialEffect(
	UNIT * defender,
	UNIT * attacker,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int damageAmount,
	int damageType)
{
	// setup combat structure
	COMBAT combat;
	CombatClear(combat);
	combat.game = UnitGetGame(defender);
	ASSERT_RETURN(combat.game && IS_SERVER(combat.game));

	GamePushCombat(combat.game, &combat);

	UNIT* pPlayer = UnitIsA( defender, UNITTYPE_PLAYER ) ? defender : NULL;
	if( !pPlayer )
	{
		pPlayer = UnitGetResponsibleUnit(attacker);
		if( !UnitIsA( pPlayer, UNITTYPE_PLAYER ) )
		{
			pPlayer = NULL;
		}
	}
	combat.monscaling = GameGetMonsterScalingFactor(combat.game, UnitGetLevel( attacker ), pPlayer);

	combat.attacker = attacker;
	combat.defender = defender;
	CombatInitWeapons(combat, weapons);
	CombatSetTargetType(combat);

	combat.delivery_type = COMBAT_LOCAL_DELIVERY;

	combat.center_room = UnitGetRoom(combat.defender);
	combat.center_point = UnitGetPosition(combat.defender);

	combat.damage_increment = DAMAGE_INCREMENT_FULL;

	// try effect
	CombatDoSpecialEffect(combat, damageAmount, damageType);

	GamePopCombat(combat.game, &combat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sApplySpectralWeakening(
	COMBAT &tCombat,
	int nDamage)
{
	int nNewDamage = nDamage;

	// do outgoing damage weakening
	int nWeakenPercent = UnitGetStat( tCombat.attacker, STATS_SPECTRALWEAKEN_OUTGOING_DAMAGE_PERCENT );
	if (nWeakenPercent != 0)
	{
		nNewDamage = PCT( nNewDamage, nWeakenPercent );
	}

	return nNewDamage;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sApplySpectralAmplification(
	COMBAT &tCombat,
	int nDamage)
{
	int nNewDamage = nDamage;

	// do incoming damage amplification
	int nAmplifyPercent = UnitGetStat( tCombat.defender, STATS_SPECTRALWEAKEN_INCOMING_DAMAGE_PERCENT );
	if (nAmplifyPercent != 0)
	{
		nNewDamage = PCT( nNewDamage, nAmplifyPercent );
	}

	return nNewDamage;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL DOT>
static inline int UnitAttackUnitApplyDamages(
	COMBAT & combat,
	STATS * stats,
	int duration,
	BOOL bDoSpecialEffects,
	int & dmg_avoided,
	BOOL bFumbled = FALSE,
	BOOL bIgnoreShieldReduction = FALSE)
{
	ASSERT(MAX_WEAPONS_PER_UNIT == 2);

	if (!(combat.flags & COMBAT_ALLOW_ATTACK_SELF) && //this allows for reflection. We want a monster to reflect it's own damage onto themselves. 
		!(combat.flags & COMBAT_ALLOW_ATTACK_FRIENDS) && 
		 !TestHostile(combat.game, combat.attacker_ultimate_source, combat.defender_ultimate_source))
	{
		return 0;
	}

	// if an event gets triggered that requires the damage done, create a statslist for it
	STATS * pStatsDamageDone = NULL;
	if (!DOT &&
		UnitHasEventHandler(combat.game, combat.attacker, EVENT_DIDDAMAGE) ||
		UnitHasEventHandler(combat.game, combat.attacker_ultimate_source, EVENT_DIDDAMAGE) ||
		(combat.weapons[0] && UnitHasEventHandler(combat.game, combat.weapons[0], EVENT_DIDDAMAGE)) ||
		(combat.weapons[1] && UnitHasEventHandler(combat.game, combat.weapons[1], EVENT_DIDDAMAGE)) ||
		UnitHasEventHandler(combat.game, combat.defender, EVENT_GOTDAMAGE) ||
		UnitHasEventHandler(combat.game, combat.defender_ultimate_source, EVENT_GOTDAMAGE) ||
		UnitHasEventHandler(combat.game, combat.defender, EVENT_GOTDAMAGE_DEFENDER_ONLY))
	{
		pStatsDamageDone = StatsListInit(combat.game);
	}

	// calculate critical
	int critical = CombatCalcCriticalLevel(combat);
	if (critical)
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		// see if we're trying to get a critical for the minigame
		if (AppIsHellgate() && combat.attacker_ultimate_source && UnitIsA(combat.attacker_ultimate_source, UNITTYPE_PLAYER))
		{
			s_PlayerCheckMinigame(combat.game, combat.attacker_ultimate_source, MINIGAME_CRITICAL, 0);
		}
#endif
		UnitTriggerEvent(combat.game, EVENT_DIDCRITICAL, combat.attacker_ultimate_source, combat.defender, NULL);
	}

	// set check for damage-over-time flag
	BOOL check_dot = !DOT;

	STATS_ENTRY damage_min[DAMAGES_SIZE], damage_max[DAMAGES_SIZE], damage_bonus[DAMAGES_SIZE], damage_augmentation[DAMAGES_SIZE];
	int nDamageMin = 0, nDamageMax = 0, nDamageAugmentation = 0, nDamageBonus = 0;
	if (stats)
	{
		nDamageMin = StatsGetRange(stats, STATS_DAMAGE_MIN, 0, damage_min, DAMAGES_SIZE);
		nDamageMax = StatsGetRange(stats, STATS_DAMAGE_MAX, 0, damage_max, DAMAGES_SIZE);
		nDamageBonus = StatsGetRange(stats, STATS_DAMAGE_BONUS, 0, damage_bonus, DAMAGES_SIZE);
		nDamageAugmentation = StatsGetRange(stats, STATS_DAMAGE_AUGMENTATION, 0, damage_augmentation, DAMAGES_SIZE);
		nDamageAugmentation = StatsGetRange(stats, STATS_DAMAGE_PERCENT_PER_ENEMY, 0, damage_augmentation, nDamageAugmentation, DAMAGES_SIZE);
		nDamageAugmentation = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_AUGMENTATION, 0, damage_augmentation, nDamageAugmentation, DAMAGES_SIZE);
		nDamageAugmentation = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_PERCENT_PER_ENEMY, 0, damage_augmentation, nDamageAugmentation, DAMAGES_SIZE);
	}
	else
	{
		ASSERT_RETZERO(combat.attacker);
		nDamageMin = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_MIN, 0, damage_min, DAMAGES_SIZE);
		nDamageMax = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_MAX, 0, damage_max, DAMAGES_SIZE);
		nDamageBonus = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_BONUS, 0, damage_bonus, DAMAGES_SIZE);
		nDamageAugmentation = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_AUGMENTATION, 0, damage_augmentation, DAMAGES_SIZE);
		nDamageAugmentation = UnitGetStatsRange(combat.attacker, STATS_DAMAGE_PERCENT_PER_ENEMY, 0, damage_augmentation, nDamageAugmentation, DAMAGES_SIZE);
	}

	int dmg_min_pct = 0, dmg_max_pct = 0, dmg_bonus = 0, dmg_type = 0;			// dmg_min_pct & dmg_max_pct are expressed in % of base damage, dmg_bonus is in points
	int dmg_total = 0;
	int dmg_carry_bonus = 0;													// for dmg_bonus w/ type = 0, carry it over to next damage type
	dmg_avoided = 0;
	int dmg_typeminmax_count = 0;
	BOOL bForceFullDmg( false );
	while (sGetNextDamageTypeMinMax(combat, damage_min, &nDamageMin, damage_max, &nDamageMax, damage_bonus, &nDamageBonus, &dmg_min_pct, &dmg_max_pct, &dmg_bonus, &dmg_type, &dmg_typeminmax_count))
	{
		if (dmg_type == 0 && dmg_max_pct == 0)	// odd bit, tugboat uses dmg_bonus to convey bonus physical damage, but doesn't specify type, so carry it over to next type
		{
			dmg_carry_bonus = dmg_bonus;
			continue;
		}
		else
		{
			dmg_bonus += dmg_carry_bonus;
			dmg_carry_bonus = 0;
		}

		// if a rider is passed in, check to see if the damage is actually damage over time
		if (check_dot && stats)
		{
			check_dot = FALSE;
			int duration = StatsGetCalculatedStat(combat.attacker, UnitGetType(combat.defender), stats, STATS_DAMAGE_DURATION, 0);
			if (duration)
			{
				if (pStatsDamageDone)
				{
					StatsListFree(combat.game, pStatsDamageDone);
				}
				return UnitAttackUnitApplyDamages<TRUE>(combat, stats, duration, TRUE, dmg_avoided, bFumbled);
			}
		}
		int dmg = 0; 
		
		bForceFullDmg = (UnitGetStat( combat.attacker, STATS_COMBAT_FULL_DAMAGE, dmg_type ) != 0 )?TRUE:FALSE; //this seems dangerous to me....!
		//is this correct - check out changelist 12 to 13.
		dmg = CombatCalcDamage(combat, dmg_min_pct, dmg_max_pct, dmg_bonus, bForceFullDmg );
		/*
		if( AppIsHellgate() )
		{
			dmg = CombatCalcDamage(combat, dmg_min_pct, dmg_max_pct, dmg_bonus, bForceFullDmg );
		}
		else
		{
			//we might rip this out.....but lets try it for right now. Criticals and if a player has a full damage modifier on. YIKES 
			dmg = CombatCalcDamage(combat, dmg_min_pct, dmg_max_pct, dmg_bonus, ( bForceFullDmg || critical > 0 )?TRUE:FALSE );
		}
		*/
		if (dmg <= 0)
		{
#if !ISVERSION(SERVER_VERSION) //WPP can't handle this
			LogMessage(COMBAT_LOG, "unit[" ID_PRINTFIELD "] (source[" ID_PRINTFIELD "]) hits unit[" ID_PRINTFIELD "] for no local damage.",
				ID_PRINT(UnitGetId(combat.attacker)), ID_PRINT(UnitGetId(UnitGetResponsibleUnit(combat.attacker))), ID_PRINT(UnitGetId(combat.defender)));
#endif //!SERVER_VERSION
			continue;
		}

		int pre_augment_dmg = dmg;

		// iterate through dmg_type and all augmentations (augmentation example: adds +10% fire damage, which adds an additional bit of fire damage = 10% of your regular damage
		for (int ii = -1; ii < nDamageAugmentation; ++ii)
		{
			if (ii >= 0)
			{
				STATS_ENTRY * aug_entry = damage_augmentation + ii;
				dmg_type = aug_entry->GetParam();
				dmg = PCT(pre_augment_dmg, aug_entry->value);
				if (dmg <= 0)
				{
					continue;
				}
			}

			const DAMAGE_TYPE_DATA * damage_type_data = DamageTypeGetData(combat.game, dmg_type);
			if (!damage_type_data)
			{
				continue;
			}

			if (UnitIsInvulnerable(combat.defender, dmg_type))
			{
				continue;
			}

			int damage_bonus_pct = CombatCalcDamageBonusPercent(combat, dmg_type, damage_type_data, TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT));

			int critical_damage_pct = CombatCalcCriticalMultiplier(combat, critical, dmg_type, damage_type_data);

			int damage_pct = MAX(damage_bonus_pct + critical_damage_pct, -100);

			dmg += PCT(dmg, damage_pct);

			if (dmg <= 0)
			{
				continue;
			}

			// apply spectral weakening
			if (!DOT)
			{
				dmg = sApplySpectralWeakening(combat, dmg);
			}

			// do vulnerabilities
			int vulnerability_multiplier = CombatCalcVulnerability(combat, dmg, dmg_type, damage_type_data);
			if (combat.flags & COMBAT_IS_PVP)
			{
				if ( AppIsTugboat() )
					vulnerability_multiplier -= damage_type_data->nVulnerabilityInPVPTugboat;
				else
					vulnerability_multiplier -= damage_type_data->nVulnerabilityInPVPHellgate;
			}
			dmg = dmg + PCT(dmg, vulnerability_multiplier);
			if (dmg <= 0)
			{
				continue;
			}

			// do secondary vulnerabilities
			int secondary_vulnerability_multiplier = CombatCalcSecondaryVulnerability(combat, dmg, dmg_type, damage_type_data);
			dmg = dmg + PCT(dmg, secondary_vulnerability_multiplier);
			if (dmg <= 0)
			{
				continue;
			}

			// do spectral amplification
			if (!DOT)
			{
				dmg = sApplySpectralAmplification(combat, dmg);
			}

			// do shields stuff
			if( !bIgnoreShieldReduction )
			{
				int shield_reduction = CombatCalcShieldInstantDamage(combat, dmg, dmg_type, damage_type_data);
				dmg_avoided += MIN(dmg, shield_reduction);
				dmg -= shield_reduction;
				if (shield_reduction > (dmg + shield_reduction)/2)
				{
					if (damage_type_data->nShieldHitState > STATE_NONE)
					{
						s_StateSet(combat.defender, combat.attacker_ultimate_source, damage_type_data->nShieldHitState, 10);
					}
				}
			}
			

			if (dmg <= 0 || TESTBIT(combat.dwUnitAttackUnitFlags, UAU_SHIELD_DAMAGE_ONLY_BIT))
			{
				continue;
			}

			// do armor stuff
			// tug doesn't do reduction of damage by armor
			if (AppIsHellgate())
			{
				int armor_reduction = CombatCalcArmorInstantDamage(combat, dmg, dmg_type, damage_type_data, dmg_type);
				dmg_avoided += MIN(dmg, armor_reduction);
				dmg -= armor_reduction;
				if (dmg <= 0)
				{
					continue;
				}

				// do armor stuff
				int armor_reduction_untyped = CombatCalcArmorInstantDamage(combat, dmg, dmg_type, damage_type_data, DAMAGE_TYPE_ALL);
				dmg_avoided += MIN(dmg, armor_reduction_untyped);
				dmg -= armor_reduction_untyped;
				if (dmg <= 0)
				{
					continue;
				}
			}
			else //tugboat
			{
				// apply spectral phased defense reduction
				int dmg_reduction( 0 );
				int defense_reduction_pct = UnitGetStat(combat.defender, STATS_DEFENSE_REDUCTION_PERCENT);
				if (defense_reduction_pct != 0)
				{
					dmg_reduction += PCT(dmg, defense_reduction_pct);
					dmg_avoided += MIN(dmg, dmg_reduction);
					dmg -= dmg_reduction;
				}
				if (dmg <= 0)
				{
					continue;
				}

				defense_reduction_pct = UnitGetStat(combat.defender, STATS_DEFENSE_REDUCTION_PERCENT, dmg_type );
				if (defense_reduction_pct != 0)
				{
					dmg_reduction += PCT(dmg, defense_reduction_pct);
					dmg_avoided += MIN(dmg, dmg_reduction);
					dmg -= dmg_reduction;
				}

				if (dmg <= 0)
				{
					continue;
				}
			}

			// we don't allow fumble on a crit
			if (bFumbled && combat.nCritHitState == INVALID_ID)
			{
				const DAMAGE_TYPE_DATA* damage_type_data = DamageTypeGetData(combat.game, DAMAGE_TYPE_ALL);
				combat.nFumbleHitState = damage_type_data->nFumbleHitState;
				dmg = PCT_ROUNDUP(dmg, 5);
			}
			if (dmg <= 0)
			{
				continue;
			}
			// special effect
			if (bDoSpecialEffects)
			{
				CombatDoSpecialEffect(combat, dmg, dmg_type, critical_damage_pct > 0);
			}

			// save critical state - roll dice if we already have one
			if (critical_damage_pct > 0 && damage_type_data->nCriticalState > STATE_NONE )
			{
				if (combat.nCritHitState == INVALID_ID || RandGetNum(combat.game, 2))
				{
					combat.nCritHitState = damage_type_data->nCriticalState;
				}
			}

			int hp_cur = UnitGetStat(combat.defender, STATS_HP_CUR);
			// sometimes we have a killing blow and the hp is zero here, but with valid damage-
			// we're getting a lot of 'silent' deaths in Mythos.
			if( ( AppIsTugboat() || hp_cur > 0 ) && dmg > 0)
			{
				int hp_cur_small = hp_cur / 9;
				int hp_cur_medium = 2 * hp_cur / 9;
				if(dmg <= hp_cur_small)
				{
					// Soft hit
					// save soft hit state - roll dice if we already have one
					// Don't bother setting this if any of the higher states are already set
					if (damage_type_data->nSoftHitState > STATE_NONE && 
						combat.nMediumHitState == INVALID_ID &&
						combat.nBigHitState == INVALID_ID)
					{
						if (combat.nSoftHitState == INVALID_ID || RandGetNum(combat.game, 2))
						{
							combat.nSoftHitState = damage_type_data->nSoftHitState;
						}
					}

				}
				else if(dmg > hp_cur_small && dmg <= hp_cur_medium)
				{
					// Medium hit
					// save soft hit state - roll dice if we already have one
					// Don't bothing setting this if the big hit state is set
					if (damage_type_data->nMediumHitState > STATE_NONE &&
						combat.nBigHitState == INVALID_ID)
					{
						if (combat.nMediumHitState == INVALID_ID || RandGetNum(combat.game, 2))
						{
							combat.nMediumHitState = damage_type_data->nMediumHitState;
						}
					}

				}
				else // if(dmg > hp_cur_medium) // It'll always be bigger than medium if it's failed the other two
				{
					// Big hit
					// save soft hit state - roll dice if we already have one
					if (damage_type_data->nBigHitState > STATE_NONE)
					{
						if (combat.nBigHitState == INVALID_ID || RandGetNum(combat.game, 2))
						{
							combat.nBigHitState = damage_type_data->nBigHitState;
						}
					}

				}
			}

			// damage is done - save the fact for the minigame (if it's a monster.  We can take out the player/monster check if we want to broaden the use of the stat))
			if (UnitIsA(combat.attacker_ultimate_source, UNITTYPE_PLAYER) &&
				UnitIsA(combat.defender_ultimate_source, UNITTYPE_MONSTER))
			{
				UnitSetStat(combat.defender_ultimate_source, STATS_DAMAGE_TAKEN_FROM_ID, dmg_type, UnitGetId(combat.attacker_ultimate_source));
			}

			if (!DOT && pStatsDamageDone)
			{
				StatsChangeStat(combat.game, pStatsDamageDone, STATS_DAMAGE_DONE, dmg_type, dmg);
			}

			if (DOT)
			{
				if (UnitIsInvulnerable(combat.defender))
				{
					continue;
				}
				int gameticks = GAME_TICKS_PER_SECOND * duration * 100 / MSECS_PER_SEC;
				STATS * dotStats = s_StateSet(combat.defender, combat.attacker_ultimate_source, damage_type_data->nDamageOverTimeState, gameticks);
				if (dotStats)
				{
					StatsSetStat(combat.game, dotStats, STATS_HP_REGEN, -dmg);
				}
			}

			dmg_total += dmg;

#if ISVERSION(DEVELOPMENT)
#if !ISVERSION(SERVER_VERSION) //WPP can't handle this
			char strDuration[256];
			strDuration[0] = 0;
			if (DOT)
			{
				PStrPrintf(strDuration, arrsize(strDuration), " over %3.1f seconds", (float)duration/10.0f);
			}

			LogMessage(COMBAT_LOG, "unit[" ID_PRINTFIELD "] (source[" ID_PRINTFIELD "]) hits unit[" ID_PRINTFIELD "] for %d (%d bonus/%d crit/%d vuln) %s%s damage%s.  hp=%d.",
				ID_PRINT(UnitGetId(combat.attacker)), ID_PRINT(UnitGetId(UnitGetResponsibleUnit(combat.attacker))), ID_PRINT(UnitGetId(combat.defender)),
				dmg >> StatsGetShift(combat.game, STATS_HP_CUR), damage_bonus_pct, critical_damage_pct, vulnerability_multiplier,
				(DOT ? "DOT " : ""), damage_type_data->szDamageType, strDuration, UnitGetStatShift(combat.defender, STATS_HP_CUR));

			CombatLog(COMBAT_TRACE_DAMAGE, combat.attacker, "%.10s[" ID_PRINTFIELD "] hits %.10s[" ID_PRINTFIELD "] for %3.1f (%d%% bonus/%d%% crit/%d%% vuln) %s%.10s damage%s.",
				UnitGetDevName(combat.attacker), ID_PRINT(UnitGetId(combat.attacker)), UnitGetDevName(combat.defender), ID_PRINT(UnitGetId(combat.defender)),
				(float)dmg / (float)(1 << StatsGetShift(combat.game, STATS_HP_CUR)), damage_bonus_pct, critical_damage_pct, vulnerability_multiplier, 
				(DOT ? "DOT " : ""), damage_type_data->szDamageType, strDuration);
#endif // !ISVERSION(SERVER_VERSION)
#endif
		}
	}

	// special effect without any parent states needed
	if (bDoSpecialEffects)
	{
		CombatDoSpecialEffect(combat, dmg_total, INVALID_ID, critical > 0 );
	}

	if (!DOT && pStatsDamageDone)
	{
		if (dmg_total > 0)
		{
			StatsSetStat(combat.game, pStatsDamageDone, STATS_DAMAGE_DONE, dmg_total);
			StatsSetStat(combat.game, pStatsDamageDone, STATS_DAMAGE_AVOIDED, dmg_avoided);
			UnitTriggerEvent(combat.game, EVENT_DIDDAMAGE, combat.attacker,					combat.defender, pStatsDamageDone);
			if( AppIsHellgate() || combat.attacker != combat.attacker_ultimate_source )
			{
				UnitTriggerEvent(combat.game, EVENT_DIDDAMAGE, combat.attacker_ultimate_source, combat.defender, pStatsDamageDone);
			}
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				if ( combat.weapons[ i ] )
					UnitTriggerEvent(combat.game, EVENT_DIDDAMAGE, combat.weapons[ i ],	combat.defender, pStatsDamageDone);
			}
			UnitTriggerEvent(combat.game, EVENT_GOTDAMAGE, combat.defender,					combat.attacker, pStatsDamageDone);
			if( AppIsHellgate() || combat.defender != combat.defender_ultimate_source )
			{
				UnitTriggerEvent(combat.game, EVENT_GOTDAMAGE, combat.defender_ultimate_source, combat.attacker, pStatsDamageDone);
			}
			UnitTriggerEvent(combat.game, EVENT_GOTDAMAGE_DEFENDER_ONLY, combat.defender,	combat.attacker, pStatsDamageDone);
		}
		StatsListFree(combat.game, pStatsDamageDone);
	}
	
	return dmg_total;
}


//a bit different then thorns. Just reflects a percent of the damage done.
static inline void s_sUnitReflectDamage(
	COMBAT & combat,
	STATS * stats,
	int nPercentToReflect )
{
	BOOL bFumble( FALSE );
	int dmg_avoided( 0 );
	int nDamage = UnitAttackUnitApplyDamages<FALSE>( combat, stats, 0, FALSE, dmg_avoided, FALSE, AppIsTugboat() );
	if( bFumble )
		return; //the monster fumbled...
	nDamage = PCT( nDamage, nPercentToReflect );

	if ( nDamage > 0 )
	{
		bool removeDamageEffects( !(combat.flags & COMBAT_DONT_ALLOW_DAMAGE_EFFECTS) );
		combat.flags |= COMBAT_DONT_ALLOW_DAMAGE_EFFECTS;
		
		s_UnitAttackUnit( 
			combat.attacker, 
			combat.attacker, 
			combat.weapons, 
			0,
#ifdef HAVOK_ENABLED
			NULL,
#endif
			0, // TODO cmarch: maybe this should be melee, but I think it might cause an infinite loop in thorns ATM, combat.bMelee, 
			combat.damagemultiplier, 
			0, 
			nDamage, 
			nDamage, 
			combat.defender );

		if( removeDamageEffects &&
			(combat.flags & COMBAT_DONT_ALLOW_DAMAGE_EFFECTS) )
		{
			combat.flags ^= COMBAT_DONT_ALLOW_DAMAGE_EFFECTS;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void UnitAttackUnitSendDamageMessage(
	COMBAT & combat,
	int hp)
{
	WORD wHitState = 0;
	if ( combat.nCritHitState != INVALID_ID )
		wHitState = (WORD) combat.nCritHitState;
	else if ( combat.nFumbleHitState != INVALID_ID )
	{
		wHitState = (WORD) combat.nFumbleHitState;
		combat.result = COMBAT_RESULT_FUMBLE;
	}
	else if ( combat.nBigHitState != INVALID_ID )
		wHitState = (WORD) combat.nBigHitState;
	else if ( combat.nMediumHitState != INVALID_ID )
		wHitState = (WORD) combat.nMediumHitState;
	else if ( combat.nSoftHitState != INVALID_ID )
		wHitState = (WORD) combat.nSoftHitState;

	MSG_SCMD_UNIT_DAMAGED tMsg;
	ASSERT(combat.result <= UCHAR_MAX);
	tMsg.bResult = (BYTE)combat.result;
	tMsg.idDefender = UnitGetId(combat.defender);
	tMsg.nCurHp = hp;
	tMsg.wHitState = wHitState;
	tMsg.bIsMelee = (BYTE)TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT);

	UNIT * pAttackerToUse = combat.attacker;
	VECTOR vSource(0);
	UNITID idWeapon = UnitGetId(combat.weapons[ 0 ]);
	// get angle damage came from
	if (UnitGetGenus(combat.attacker) == GENUS_MISSILE)
	{
		// source for position
		UNIT * missilesource = UnitGetUltimateOwner(combat.attacker);
		if (missilesource)
		{
			vSource = UnitGetPosition(missilesource);
		}
		else
		{
			vSource = UnitGetPosition(combat.attacker);
		}

		// source for attacker passed to client
		if ( idWeapon == INVALID_ID )
		{
			missilesource = UnitGetOwnerUnit( combat.attacker );
			BOUNDED_WHILE ( missilesource && UnitGetGenus( missilesource ) == GENUS_MISSILE && missilesource != UnitGetOwnerUnit( missilesource ), 50 )
			{
				missilesource = UnitGetOwnerUnit( missilesource );
			}
			if ( missilesource )
				pAttackerToUse = missilesource;
		}
	}
	else
	{
		vSource = UnitGetPosition(combat.attacker);
	}

	tMsg.idAttacker = idWeapon != INVALID_ID ? idWeapon : UnitGetId( pAttackerToUse );

	CombatGetDamagePositionFromVector(vSource, tMsg.nSourceX, tMsg.nSourceY);

	s_SendUnitMessage(combat.defender, SCMD_UNIT_DAMAGED, &tMsg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsStone(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// data drive this better
	if (UnitHasExactState( pUnit, STATE_STONE_AWAKENING ) ||
		UnitHasExactState( pUnit, STATE_STONE ))
	{
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAttackUnitGetHit(
	COMBAT & combat,
	int dmg,
	int dmg_avoided)
{
	// do nothing for stone units
	if (combat.defender && sUnitIsStone( combat.defender ))
	{
		return;
	}
	
	int interrupt = CombatGetInterruptChance(combat, dmg, dmg_avoided);
	if (interrupt > 0)
	{
		// TRAVIS: OK, let's try something different - getting dazed!
		if (AppIsTugboat() && UnitIsA(combat.defender, UNITTYPE_PLAYER))
		{
			UNIT* pAttacker = UnitGetResponsibleUnit(combat.attacker);
			if( pAttacker )
			{
				// only get dazed while running, or while facing away from attacking target
				UnitGetFaceDirection( combat.defender, TRUE );
				if( VectorDot( UnitGetFaceDirection( combat.defender, TRUE ) , UnitGetFaceDirection( combat.attacker, TRUE ) ) > .25f ||
					UnitTestMode( combat.defender, MODE_RUN ) )
				{
					if( (int)RandGetNum(combat.game, 100) < UnitGetData( pAttacker )->nStaminaDrainChance )
					{
						s_StateSet( combat.defender, pAttacker, STATE_DAZED, GAME_TICKS_PER_SECOND * 3 );
					}
				}

			}
		}

#ifdef HAVOK_ENABLED
		if (combat.pImpact)
		{
			UnitAddImpact(combat.game, combat.defender, combat.pImpact);
		}
#endif

		BOOL bHasMode;
		float fDuration = UnitGetModeDuration(combat.game, combat.defender, MODE_GETHIT, bHasMode);
		if (bHasMode)
		{
			if (AppIsTugboat())
			{
				s_StateSet(combat.defender, combat.defender, STATE_EFFECT_HITSTUN, (int)(fDuration * GAME_TICKS_PER_SECOND));
			}
			else
			{
				s_UnitSetMode(combat.defender, MODE_GETHIT, 0, fDuration);
			}
			return;
		}
	}
	UNITMODE eMode = MODE_GETHIT_NO_INTERRUPT;
	BOOL bHasMode;
	float fDuration = UnitGetModeDuration(combat.game, combat.defender, eMode, bHasMode);
	if (!bHasMode)
	{
		eMode = MODE_SOFTHIT;
		fDuration = UnitGetModeDuration(combat.game, combat.defender, eMode, bHasMode);
	}
	s_UnitSetMode(combat.defender, eMode, 0, fDuration);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct RADIAL_DAMAGE
{
	STATS *	stats;
	float	radius_sq;
	int		damage_effect;
	int		damage_increment;
	int		damage_increment_pct;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int __cdecl RadialDamageCompare(
	const void* a,
	const void* b)
{
	const RADIAL_DAMAGE* A = (const RADIAL_DAMAGE*)a;
	const RADIAL_DAMAGE* B = (const RADIAL_DAMAGE*)b;

	if (A->radius_sq < B->radius_sq)
	{
		return 1;
	}
	else if (A->radius_sq > B->radius_sq)
	{
		return -1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COMBAT_SUBDEFENDER_OVERRIDE
{
	COMBAT *	combat;
	DWORD		flags;
	UNIT *		defender_old;
	UNIT *		defender_ultimate_source_old;
	int			damage_increment_old;
	int			damage_increment_pct_old;
	int			defender_level_old;
	int			evade_old;
	int			block_old;
	int			armorclass_old;

	void SetOverrideFlag(
		COMBAT * combat,
		int flag,
		int & old_store,
		int & combat_store)
	{
		old_store = combat_store;
		if (combat->flags & flag)
		{
			flags |= flag;
			combat->flags &= ~flag;
			combat_store = 0;
		}
	}

	void RestoreOverrideFlag(
		COMBAT * combat,
		int flag,
		int & old_store,
		int & combat_store)
	{
		combat_store = old_store;
		if (flags & flag)
		{
			combat->flags |= flag;
		}
		else
		{
			combat->flags &= ~flag;
		}
	}

	COMBAT_SUBDEFENDER_OVERRIDE(
		COMBAT & _combat,
		UNIT * defender,
		int damage_increment_pct) : combat(&_combat), flags(0)
	{
		ASSERT_RETURN(combat);

		defender_old = combat->defender;
		combat->defender = defender;

		defender_ultimate_source_old = combat->defender_ultimate_source;
		if (defender_old != defender)
		{
			combat->defender_ultimate_source = NULL;
			SetOverrideFlag(combat, COMBAT_DEFENDER_LEVEL, defender_level_old, combat->defender_level);
			SetOverrideFlag(combat, COMBAT_DEFENDER_EVADE, evade_old, combat->evade);
			SetOverrideFlag(combat, COMBAT_DEFENDER_BLOCK, block_old, combat->block);
			SetOverrideFlag(combat, COMBAT_DEFENDER_ARMORCLASS, armorclass_old, combat->armorclass);
		}

		damage_increment_old = combat->damage_increment;
		damage_increment_pct_old = combat->damage_increment_pct;
		combat->damage_increment_pct = damage_increment_pct;

		if (combat->damage_increment <= 0)
		{
			combat->damage_increment = DAMAGE_INCREMENT_FULL;
		}
		if (damage_increment_pct > 0)
		{
			combat->damage_increment = PCT(combat->damage_increment, damage_increment_pct);
			if (combat->damage_increment <= 0)
			{ 
				combat->damage_increment = 1;
			}
		}
	}

	~COMBAT_SUBDEFENDER_OVERRIDE(void)
	{
		ASSERT_RETURN(combat);

		if (combat->defender != defender_old)
		{
			RestoreOverrideFlag(combat, COMBAT_DEFENDER_LEVEL, defender_level_old, combat->defender_level);
			RestoreOverrideFlag(combat, COMBAT_DEFENDER_EVADE, evade_old, combat->evade);
			RestoreOverrideFlag(combat, COMBAT_DEFENDER_BLOCK, block_old, combat->block);
			RestoreOverrideFlag(combat, COMBAT_DEFENDER_ARMORCLASS, armorclass_old, combat->armorclass);
		}
		combat->defender = defender_old;
		combat->defender_ultimate_source = defender_ultimate_source_old;
		combat->damage_increment = damage_increment_old;
		combat->damage_increment_pct = damage_increment_pct_old;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void (*CombatDoDamageUnitList)(COMBAT & combat, UNITID * units, int unit_count, RADIAL_DAMAGE * radial_damages, int num_radial_damages);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void CombatDoRadialDamagesUnitList(
	COMBAT & combat,
	UNITID * units,
	int unit_count,
	RADIAL_DAMAGE * radial_damages,
	int num_radial_damages)
{
	for (int ii=0; ii<unit_count; ii++)
	{
		UNIT * unit = UnitGetById(combat.game, units[ii]);
		if (!unit)
		{
			continue;
		}

		if (UnitGetCanTarget(unit))
		{
			float range_sq = 0.0f;
			if (UnitIsInRange(unit, combat.center_point, combat.max_radius, &range_sq))
			{
				for (int ii = 0; ii < num_radial_damages; ii++)
				{
					if (range_sq <= radial_damages[ii].radius_sq)
					{
						COMBAT_SUBDEFENDER_OVERRIDE override(combat, unit, radial_damages[ii].damage_increment_pct);
						s_UnitAttackUnit(combat, radial_damages[ii].stats);
					}
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void CombatDoSpecialEffectsUnitList(
	COMBAT & combat,
	UNITID * units,
	int unit_count,
	RADIAL_DAMAGE * radial_damages,
	int num_radial_damages)
{
	for (int ii = 0; ii < unit_count; ii++)
	{
		UNIT * unit = UnitGetById(combat.game, units[ii]);
		if (!unit)
		{
			continue;
		}

		if (UnitGetCanTarget(unit))
		{
			float range_sq = 0.0f;
			if (UnitIsInRange(unit, combat.center_point, combat.max_radius, &range_sq))
			{
				for (int ii = 0; ii < num_radial_damages; ii++)
				{
					if (range_sq <= radial_damages[ii].radius_sq)
					{
						COMBAT_SUBDEFENDER_OVERRIDE override(combat, unit, radial_damages[ii].damage_increment_pct);
						sCombatEvaluateSpecialEffect(combat, 0, INVALID_ID, radial_damages[ii].damage_effect, FALSE, TRUE, NULL, 0);
					}
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void CombatDoRadialDamages(
	COMBAT & combat,
	RADIAL_DAMAGE* radial_damages,
	int num_radial_damages,
	CombatDoDamageUnitList pfnDoDamagesUnitList)
{
	ASSERT_RETURN(combat.center_room);
	ASSERT_RETURN(pfnDoDamagesUnitList);

	combat.delivery_type = COMBAT_SPLASH_DELIVERY;
	combat.max_radius_sq = combat.max_radius * combat.max_radius;

	CLEARBIT(combat.dwUnitAttackUnitFlags, UAU_DIRECT_MISSILE_BIT);

	UNITID pnUnitIds[MAX_TARGETS_PER_QUERY];
	SKILL_TARGETING_QUERY tTargetQuery;
	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_RADIAL_ONLY_ATTACKS_DEFENDER_BIT) && combat.defender)
	{
		pnUnitIds[0] = UnitGetId(combat.defender);
		tTargetQuery.nUnitIdCount = 1;
	}
	else
	{
		tTargetQuery.pSeekerUnit = combat.attacker;
		tTargetQuery.pCenterRoom = combat.center_room;
		tTargetQuery.pvLocation = &combat.center_point;
		tTargetQuery.fMaxRange = combat.max_radius;
		// Hardcoded for now, revisit this value later - GS
		tTargetQuery.fLOSDistance = 2.0f;
		tTargetQuery.pnUnitIds = pnUnitIds;
		tTargetQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
		if(AppIsTugboat() || !UnitIsA(combat.attacker, UNITTYPE_FIELD_MISSILE))
		{
			SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
		}
		else if(UnitIsA(combat.attacker, UNITTYPE_FIELD_MISSILE) && !LevelIsOutdoors(RoomGetLevel(combat.center_room)))
		{
			SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_SIMPLE_PATH_EXISTS );
		}

		// speaking of hardcoded.... seems like we need to put together a way to specify these flags elsewhere in the skills.
		if( AppIsTugboat() )		
		{
			SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES );
			if( UnitIsA( combat.attacker, UNITTYPE_DESTRUCTIBLE ) )
			{
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
			}
		}
		SkillTargetQuery(combat.game, tTargetQuery);
	}

	pfnDoDamagesUnitList(combat, pnUnitIds, tTargetQuery.nUnitIdCount, radial_damages, num_radial_damages);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CombatCheckMissileTag(
	COMBAT & combat)
{
	if (combat.bIgnoreMissileTag)
	{
		return TRUE;
	}
	if (!UnitTestFlag(combat.attacker, UNITFLAG_MISSILETAG))
	{
		combat.bIgnoreMissileTag = TRUE;
		return TRUE;
	}

	PARAM dwMissileTag;
	int nMissileTagLen = UnitGetStatAny(combat.attacker, STATS_MISSILE_TAG, &dwMissileTag);
	if (nMissileTagLen > 0 && dwMissileTag != 0)
	{
		if (UnitGetStat(combat.defender, STATS_MISSILE_TAG, dwMissileTag))
		{
			//trace("missile:" ID_PRINTFIELD "found missile tag: %d, exit.\n", ID_PRINT(UnitGetId(combat.attacker)), dwMissileTag);
			return FALSE;
		}
		STATS * taglist = StatsListInit(combat.game);
		StatsSetStat(combat.game, taglist, STATS_MISSILE_TAG, dwMissileTag, nMissileTagLen);
		StatsListAddTimed(combat.defender, taglist, SELECTOR_MISSILETAG, 0, nMissileTagLen, NULL, FALSE);
		//trace("missile:" ID_PRINTFIELD "set missile tag: %d.\n", ID_PRINT(UnitGetId(combat.attacker)), dwMissileTag);
	}
	combat.bIgnoreMissileTag = TRUE;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct FIELD_SETSTATS_PARAMS
{
	STATS_ENTRY damage_min[1];
	STATS_ENTRY damage_max[1];
	int nDamageMax;
	int damage_increment;
	int nRadius;
};
typedef void (*FN_FIELDSETSTATS)(COMBAT & combat, STATS* newrider, FIELD_SETSTATS_PARAMS* params);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUnitAttackUnitSetFieldStats(COMBAT & combat, STATS* newrider, FIELD_SETSTATS_PARAMS* params)
{
	if (newrider && params)
	{
		StatsSetStat(combat.game, newrider, STATS_DAMAGE_MIN, params->damage_min[0].GetParam(), params->damage_min[0].value);

		if (params->nDamageMax > 0)
		{
			StatsSetStat(combat.game, newrider, STATS_DAMAGE_MAX, params->damage_max[0].GetParam(), params->damage_max[0].value);
		}

		StatsSetStat(combat.game, newrider, STATS_DAMAGE_RADIUS, params->nRadius);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUnitAttackUnitSetAIChangerFieldStats(
	COMBAT & combat,
	STATS * newrider,
	FIELD_SETSTATS_PARAMS * params)
{
	if (!newrider)
	{
		return;
	}
	if (!params)
	{
		return;
	}

	StatsSetStat(combat.game, newrider, STATS_AI_CHANGE_DAMAGE_EFFECT, params->damage_min[0].GetParam(), params->damage_min[0].value);

	StatsSetStat(combat.game, newrider, STATS_DAMAGE_RADIUS, params->nRadius);

	if (params->damage_increment > 0)
	{
		StatsSetStat(combat.game, newrider, STATS_DAMAGE_INCREMENT_AI_CHANGER, params->damage_increment);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitAttackUnitPlaceField(
	COMBAT & combat,
	STATS * rider,
	int nFieldMissile,
	int field_dur,
	FN_FIELDSETSTATS pfnSetFieldStats,
	FIELD_SETSTATS_PARAMS* params)
{
	VECTOR vMissilePosition = combat.center_point;
	VECTOR vMissileNormal = cgvZAxis;

	FREE_PATH_NODE tFreePathNode;
	DWORD dwNearestNodeFlags = 0;
	SETBIT(dwNearestNodeFlags, NPN_ONE_ROOM_ONLY);
	SETBIT(dwNearestNodeFlags, NPN_USE_GIVEN_ROOM);
	SETBIT(dwNearestNodeFlags, NPN_ALLOW_OCCUPIED_NODES);
	SETBIT(dwNearestNodeFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(dwNearestNodeFlags, NPN_DONT_CHECK_RADIUS);
	SETBIT(dwNearestNodeFlags, NPN_CHECK_LOS);
	SETBIT(dwNearestNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
	NEAREST_NODE_SPEC tSpec;
	tSpec.dwFlags = dwNearestNodeFlags;
	int nPathNodeCount = RoomGetNearestPathNodes(combat.game, combat.center_room, combat.center_point, 1, &tFreePathNode, &tSpec);
	if(nPathNodeCount == 1 && tFreePathNode.pNode && tFreePathNode.pRoom)
	{
		vMissilePosition = RoomPathNodeGetExactWorldPosition(combat.game, tFreePathNode.pRoom, tFreePathNode.pNode);
#if HELLGATE_ONLY
		vMissileNormal = AppIsHellgate() ? RoomPathNodeGetWorldNormal(combat.game, tFreePathNode.pRoom, tFreePathNode.pNode) : VECTOR( 0, 0, 1 );
#else
		vMissileNormal = VECTOR( 0, 0, 1 );
#endif
	}

	UNIT * pOwner = combat.attacker_ultimate_source;
	if(!pOwner)
	{
		pOwner = UnitGetResponsibleUnit(combat.attacker);
	}
	if(!pOwner)
	{
		pOwner = combat.attacker;
	}
	DWORD dwMissileTag = 0;
	UnitGetStatAny( combat.attacker, STATS_MISSILE_TAG, &dwMissileTag);
	if ( ! dwMissileTag )
		dwMissileTag = 1; // GameGetMissileTag(combat.game);
	// GameGetMissileTag() is the proper thing to do here, but field stacking is causing us some headaches.  Once we figure out what to do, we should put it back this way
	UNIT * field = MissileFire(
		combat.game,
		nFieldMissile,
		pOwner,
		NULL,
		INVALID_ID,
		NULL,
		VECTOR(0),
		vMissilePosition,
		vMissileNormal,
		0,
		dwMissileTag);
	UnitSetFuse(field, field_dur);
	sUnitTransferIndirectDamageStats(field, combat.attacker);

	if(combat.attack_rating_base > 0)
	{
		UnitSetStat(field, STATS_ATTACK_RATING, combat.attack_rating_base);
	}
	else
	{
		UnitSetStat(field, STATS_ATTACK_RATING, UnitGetStat(pOwner, STATS_ATTACK_RATING));
	}

	// The total damage increment for the field is the field damage increment of the attacker
	if(params->damage_increment > 0)
	{
		UnitSetStat(field, STATS_DAMAGE_INCREMENT, params->damage_increment);
	}
	// The radial damage increment is set to the energy decrease damage increment
	sCombatGetEnergyIncrement(combat);
	if(combat.damage_increment_energy != DAMAGE_INCREMENT_FULL)
	{
		UnitSetStat(field, STATS_DAMAGE_INCREMENT_RADIAL, combat.damage_increment_energy);
	}
	else
	{
		// If there is no energy increment, then we must clear the radial damage increment so that any accrued values don't stop on the field's proper damage
		UnitSetStat(field, STATS_DAMAGE_INCREMENT_RADIAL, 0);
	}

	if(TESTBIT(combat.dwUnitAttackUnitFlags, UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT))
	{
		UnitSetStat(field, STATS_APPLY_DUALWEAPON_FOCUS_SCALING, 1);
	}

	STATS * newrider = StatsListAddRider(combat.game, UnitGetStats(field), SELECTOR_DAMAGE_RIDER);
	if (newrider)
	{
		pfnSetFieldStats(combat, newrider, params);
	}

	if(nPathNodeCount == 1 && tFreePathNode.pNode && tFreePathNode.pRoom)
	{
		MSG_SCMD_PLACE_FIELD_PATHNODE tMsg;
		tMsg.idOwner = UnitGetId(pOwner);
		tMsg.nFieldMissile = nFieldMissile;
		tMsg.idRoom = RoomGetId(tFreePathNode.pRoom);
		tMsg.nPathNode = tFreePathNode.pNode->nIndex;
		tMsg.nRadius = params->nRadius;
		tMsg.nDuration = field_dur;

		s_SendMessageToAllWithRoom(combat.game, UnitGetRoom(field), SCMD_PLACE_FIELD_PATHNODE, &tMsg);
	}
	else
	{
		MSG_SCMD_PLACE_FIELD tMsg;
		tMsg.idOwner = UnitGetId(pOwner);
		tMsg.nFieldMissile = nFieldMissile;
		tMsg.vPosition = vMissilePosition;
		tMsg.nRadius = params->nRadius;
		tMsg.nDuration = field_dur;

		s_SendMessageToAllWithRoom(combat.game, UnitGetRoom(field), SCMD_PLACE_FIELD, &tMsg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAttackUnitDoSecondaryAttacks(
	COMBAT & combat)
{
	if (combat.defender && !CombatCheckMissileTag(combat))
	{
		return;
	}

	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return;
	}

	enum
	{
		MAX_RADIAL_DAMAGES = 64
	};
	RADIAL_DAMAGE radial_damages[MAX_RADIAL_DAMAGES];
	int num_radial_damages = 0;
	int max_radius = 0;

	// loop through riders to find radial damages, fields, and damage over time
	STATS * rider = UnitGetRider(combat.attacker, NULL);
	while (rider)
	{
		STATS * next = UnitGetRider(combat.attacker, rider);

		STATS_ENTRY damage_min[1];
		int nDamageMin = StatsGetRange(rider, STATS_DAMAGE_MIN, 0, damage_min, 1);
		if (nDamageMin > 0)
		{
			int damage_type = damage_min[0].GetParam();

			int radius = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_RADIUS, (WORD)damage_type);
			if (radius <= 0)
			{
				if (combat.defender)
				{
					int duration = StatsGetCalculatedStat(combat.attacker, UnitGetType(combat.defender), rider, STATS_DAMAGE_DURATION, 0);
					ASSERT(duration > 0);
					if (duration > 0)
					{
						int damage_increment_pct = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_INCREMENT_DOT);
						COMBAT_SUBDEFENDER_OVERRIDE override(combat, combat.defender, damage_increment_pct);
						int dmg_avoided = 0;
						UnitAttackUnitApplyDamages<TRUE>(combat, rider, duration, TRUE, dmg_avoided);
					}
				}
			}
			else
			{
				int field_dur = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_FIELD, (WORD)damage_type);
				if (field_dur <= 0)
				{
					ASSERT(num_radial_damages < MAX_RADIAL_DAMAGES);
					if (num_radial_damages < MAX_RADIAL_DAMAGES)
					{
						int damage_increment_radial = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_INCREMENT_RADIAL);
						radial_damages[num_radial_damages].radius_sq = (float)(radius * radius) / (RADIUS_DIVISOR * RADIUS_DIVISOR);
						radial_damages[num_radial_damages].stats = rider;
						radial_damages[num_radial_damages].damage_increment = combat.damage_increment;
						radial_damages[num_radial_damages].damage_increment_pct = MAX(0, damage_increment_radial);
						if (radius > max_radius)
						{
							max_radius = radius;
						}
						num_radial_damages++;
					}
				}
				else
				{
					ASSERTXONCE_RETURN( !UnitIsA( combat.attacker, UNITTYPE_FIELD_MISSILE ), "fields can't spawn fields" );
					const DAMAGE_TYPE_DATA * damage_type_data = DamageTypeGetData(combat.game, damage_type);
					ASSERT_RETURN(damage_type_data);

					int nFieldMissile = damage_type_data->nFieldMissile;
					if(combat.weapons)
					{
						const UNIT_DATA * pWeaponData = UnitGetData(combat.weapons[0]);
						if(pWeaponData && pWeaponData->nFieldMissile >= 0)
						{
							nFieldMissile = pWeaponData->nFieldMissile;
						}
					}

					int nFieldOverride = UnitGetStat(combat.attacker, STATS_FIELD_OVERRIDE);
					if(nFieldOverride >= 0)
					{
						nFieldMissile = nFieldOverride;
					}

					ASSERT_RETURN(nFieldMissile >= 0);
					
					FIELD_SETSTATS_PARAMS tFieldParams;
					tFieldParams.damage_min->stat = damage_min->stat;
					tFieldParams.damage_min->value = damage_min->value;
					tFieldParams.nDamageMax = StatsGetRange(rider, STATS_DAMAGE_MAX, 0, tFieldParams.damage_max, 1);
					tFieldParams.damage_increment = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_INCREMENT_FIELD);
					tFieldParams.nRadius = radius;
					sUnitAttackUnitPlaceField(combat, rider, nFieldMissile, field_dur, sUnitAttackUnitSetFieldStats, &tFieldParams);
				}
			}
		}

		rider = next;
	}

	if (num_radial_damages > 0)
	{
		combat.max_radius = (float)max_radius / RADIUS_DIVISOR;
		CombatDoRadialDamages(combat, radial_damages, num_radial_damages, CombatDoRadialDamagesUnitList);
	}

	num_radial_damages = 0;
	max_radius = 0;

	int nDamageIncrementOld = combat.damage_increment;
	if (combat.damage_increment <= 0)
	{
		if (combat.weapons[0])
		{
			combat.damage_increment = UnitGetStat(combat.weapons[0], STATS_DAMAGE_INCREMENT);
		}
		if (combat.damage_increment <= 0)
		{
			combat.damage_increment = UnitGetStat(combat.attacker, STATS_DAMAGE_INCREMENT);
		}
		if (combat.damage_increment <= 0)
		{
			combat.damage_increment = DAMAGE_INCREMENT_FULL;
		}
	}

	// loop through riders again to find ai changers
	rider = UnitGetRider(combat.attacker, NULL);
	while (rider)
	{
		STATS * next = UnitGetRider(combat.attacker, rider);

		STATS_ENTRY ai_change_damage_effect[1];
		int nDamageEffect = StatsGetRange(rider, STATS_AI_CHANGE_DAMAGE_EFFECT, 0, ai_change_damage_effect, 1);
		if (nDamageEffect > 0)
		{
			int damage_effect = ai_change_damage_effect[0].value;

			int damage_increment_ai_changer = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_INCREMENT_AI_CHANGER);

			int radius = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_RADIUS, (WORD)damage_effect);
			if (radius <= 0)
			{
				ASSERTX_RETURN(combat.defender, "AI Changer special effect must have a defender");
				COMBAT_SUBDEFENDER_OVERRIDE override(combat, combat.defender, damage_increment_ai_changer);
				sCombatEvaluateSpecialEffect(combat, 0, INVALID_ID, damage_effect, FALSE, TRUE, NULL, 0);
			}
			else
			{
				int field_dur = StatsGetCalculatedStat(combat.attacker, NULL, rider, STATS_DAMAGE_FIELD, (WORD)damage_effect);
				if (field_dur <= 0)
				{
					ASSERT(num_radial_damages < MAX_RADIAL_DAMAGES);
					if (num_radial_damages < MAX_RADIAL_DAMAGES)
					{
						radial_damages[num_radial_damages].radius_sq = (float)(radius * radius) / (RADIUS_DIVISOR * RADIUS_DIVISOR);
						radial_damages[num_radial_damages].stats = rider;
						radial_damages[num_radial_damages].damage_effect = damage_effect;
						radial_damages[num_radial_damages].damage_increment = combat.damage_increment;
						radial_damages[num_radial_damages].damage_increment_pct = damage_increment_ai_changer;
						if (radius > max_radius)
						{
							max_radius = radius;
						}
						num_radial_damages++;
					}
				}
				else
				{
					ASSERTXONCE_RETURN( !UnitIsA( combat.attacker, UNITTYPE_FIELD_MISSILE ), "fields can't spawn fields" );
					const DAMAGE_EFFECT_DATA * damage_effect_data = DamageEffectGetData(combat.game, damage_effect);
					ASSERT_RETURN(damage_effect_data);

					int nFieldMissile = damage_effect_data->nFieldMissile;
					if(combat.weapons)
					{
						const UNIT_DATA * pWeaponData = UnitGetData(combat.weapons[0]);
						if(pWeaponData && pWeaponData->nFieldMissile >= 0)
						{
							nFieldMissile = pWeaponData->nFieldMissile;
						}
					}

					int nFieldOverride = UnitGetStat(combat.attacker, STATS_FIELD_OVERRIDE);
					if(nFieldOverride >= 0)
					{
						nFieldMissile = nFieldOverride;
					}

					FIELD_SETSTATS_PARAMS tFieldParams;
					tFieldParams.damage_min->stat = ai_change_damage_effect->stat;
					tFieldParams.damage_min->value = ai_change_damage_effect->value;
					tFieldParams.damage_increment = damage_increment_ai_changer;
					tFieldParams.nRadius = radius;
					sUnitAttackUnitPlaceField(combat, rider, nFieldMissile, field_dur, sUnitAttackUnitSetAIChangerFieldStats, &tFieldParams);
				}
			}
		}

		rider = next;
	}
	combat.damage_increment = nDamageIncrementOld;

	if (num_radial_damages > 0)
	{
		combat.max_radius = (float)max_radius / RADIUS_DIVISOR;
		CombatDoRadialDamages(combat, radial_damages, num_radial_damages, CombatDoSpecialEffectsUnitList);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitAttackUnitCalculateResults(
	COMBAT & combat,
	STATS * stats,
	int hp,
	int dmg,
	int dmg_avoided)
{
	// check death or gethit
	if (hp <= 0)
	{
#ifdef HAVOK_ENABLED
		if (combat.pImpact)
		{
			UnitAddImpact(combat.game, combat.defender, combat.pImpact);
		}
#endif

		combat.result = COMBAT_RESULT_KILL;
		s_UnitKillUnit(combat.game, combat.monscaling, combat.attacker, combat.defender, combat.weapons, combat.attacker_ultimate_source, combat.defender_ultimate_source);
	}
	else
	{
		int pct_damaged = 100;
		int maxhp = UnitGetStat(combat.defender, STATS_HP_MAX);
		if (maxhp > 0)
		{
			pct_damaged = (100 * dmg) / maxhp;
		}

		int hittype = 0;
		if (pct_damaged < 3)
		{
			hittype = COMBAT_RESULT_HIT_TINY;
		}
		else if (pct_damaged < 10)
		{
			hittype = COMBAT_RESULT_HIT_LIGHT;
		}
		else if (pct_damaged < 30)
		{
			hittype = COMBAT_RESULT_HIT_MEDIUM;
		}
		else
		{
			hittype = COMBAT_RESULT_HIT_HARD;
		}
		combat.result = MAX(combat.result, hittype);

		UnitAttackUnitGetHit(combat, dmg, dmg_avoided);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitAttackUnit(
	COMBAT & combat,
	STATS * stats)
{
	ASSERT_RETURN(combat.attacker && combat.defender);
	if( !combat.attacker_ultimate_source ||
		!combat.defender_ultimate_source )
	{
		combat.attacker_ultimate_source = UnitGetResponsibleUnit(combat.attacker);
		combat.defender_ultimate_source = UnitGetResponsibleUnit(combat.defender);
	}
#if !ISVERSION(SERVER_VERSION)
	LogMessage(COMBAT_LOG, "unit[" ID_PRINTFIELD "] (source[" ID_PRINTFIELD "]) attacks unit[" ID_PRINTFIELD "].",
		ID_PRINT(UnitGetId(combat.attacker)), ID_PRINT(UnitGetId(UnitGetResponsibleUnit(combat.attacker))), ID_PRINT(UnitGetId(combat.defender)));
#endif // !ISVERSION(SERVER_VERSION)
	if (!combat.attacker_ultimate_source)
	{
		return;
	}
	if (!combat.defender_ultimate_source)
	{
		return;
	}

	if (IsUnitDeadOrDying(combat.defender))	// dead units can't be attacked
	{
		return;
	}
	if (!(combat.flags & COMBAT_ALLOW_ATTACK_SELF) && //this allows for reflection. We want a monster to reflect it's own damage onto themselves.  
		 !(combat.flags & COMBAT_ALLOW_ATTACK_FRIENDS) && 
		 !TestHostile(combat.game, combat.attacker_ultimate_source, combat.defender_ultimate_source))  
	{
		return;
	}
	if (!s_UnitCanAttackUnit(combat.attacker, combat.defender))
	{
		return;
	}
	if (combat.defender && !CombatCheckMissileTag(combat))
	{
		return;
	}

	// if the attacker is a decoy, don't do any real attacks
	if (UnitIsDecoy(combat.attacker) == TRUE || UnitIsDecoy(combat.attacker_ultimate_source) == TRUE)
	{
		return;
	}

	// get weapon base damages and damage increments
	if (!UnitGetBaseDamage(combat))
	{
		return;
	}

	UNIT * pAttackerOwner;
	{	// this is handy for many units.
		UNITID idAttackerSource = UnitGetOwnerId(combat.attacker);
		if (idAttackerSource == INVALID_ID)
		{
			idAttackerSource = UnitGetId(combat.attacker);
		}
		pAttackerOwner = UnitGetById( combat.game, idAttackerSource );
		if (pAttackerOwner == NULL)
		{
			pAttackerOwner = combat.attacker;
		} 
		else if ( pAttackerOwner && !UnitGetCanTarget( pAttackerOwner ) )
		{
			idAttackerSource = UnitGetOwnerId( pAttackerOwner );
			pAttackerOwner = UnitGetById( combat.game, idAttackerSource );
			if ( ! pAttackerOwner )
			{
				pAttackerOwner = combat.attacker;
				idAttackerSource = UnitGetId( combat.attacker );
			}

		}
		UnitSetStat(combat.defender, STATS_AI_LAST_ATTACKER_ID, idAttackerSource);

		UNITID idAttackerUltimate = UnitGetStat( pAttackerOwner, STATS_LAST_ULTIMATE_ATTACKER_ID );
		if ( idAttackerUltimate == INVALID_ID )
			idAttackerUltimate = idAttackerSource;
		UnitSetStat(combat.defender, STATS_LAST_ULTIMATE_ATTACKER_ID, idAttackerUltimate);
	}

#if ISVERSION(DEVELOPMENT)
	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) && UnitIsPlayer(combat.defender))
	{
		GAMECLIENT * client = UnitGetClient(combat.defender);
		if (client)
		{
			ASSERT(UnitIsKnownByClient(client, combat.attacker));
		}
	}
#endif

	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT))
	{
		UnitTriggerEvent(combat.game, EVENT_DIDHITMELEE, combat.attacker, combat.defender, &combat);
	}
	else
	{
		UnitTriggerEvent(combat.game, EVENT_DIDHITRANGE, combat.attacker, combat.defender, &combat);
	}
	UnitTriggerEvent(combat.game, EVENT_DIDHIT_ULTIMATE_ATTACKER, UnitGetResponsibleUnit( combat.attacker ), combat.defender, &combat);
	UnitTriggerEvent(combat.game, EVENT_DIDHIT, combat.attacker, combat.defender, &combat);
	UnitTriggerEvent(combat.game, EVENT_GOTHIT, combat.defender, pAttackerOwner, &combat);
	s_StateSet( combat.defender, combat.attacker_ultimate_source, STATE_BEING_ATTACKED, 0 ); // this helps AIs know that they are under attack
	InventoryEquippedTriggerEvent( combat.game, EVENT_GOTHIT, combat.defender, pAttackerOwner, &combat );

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if (combat.weapons[ i ])
		{
			UnitTriggerEvent(combat.game, EVENT_DIDHIT, combat.weapons[ i ], combat.defender, &combat);
		}
	}

	BOOL bFumbled = FALSE;
	if ( combat.delivery_type == COMBAT_LOCAL_DELIVERY
 		 && !( AppIsTugboat() && UnitIsA( combat.defender, UNITTYPE_DESTRUCTIBLE ) ) )
	{
		// to hit - this was set to do nothing if true. But if true it hit so it should continue on. So I made 
		// if it misses to just exit - talk to travis about this...
		// TRAVIS: It was calculating failure. You reversed the signs on the return value, and the if,
		// so it does exactly the same thing. Why it was commented out though? I dunno. Let's ask Tyler!
		if (AppIsTugboat() && !UnitDoToHit(combat) ) //failed to hit?
		{
			if( !UnitIsA(combat.attacker_ultimate_source, UNITTYPE_PLAYER) )
			{
				return; //this was rem'ed out
			}
			else
			{
				bFumbled = TRUE;
			}
		}
		// evasion
		if (!AppIsTugboat() && UnitDoEvasion(combat))
		{
			return;
		}
	}

	// defender may do damage to attacker, with thorns_dmg_* properties
	if( TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) && !TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT) )
	{
		s_sUnitDoThorns(combat);
	}

	// blocking
	if (UnitDoBlocking(combat))
	{
		int blockMastery = UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERY );
		REF( blockMastery );
		if( (int)RandGetNum( combat.game, 1, 100 ) <= UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERY ) )
		{
			if( combat.defender != combat.attacker )
			{
				int nPCT = UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERYPCT );
				s_sUnitReflectDamage( combat, stats, (nPCT == 0 )?100:nPCT );							
			}
		}
		return;
	}

	// set states
	UnitAttackUnitSetStates(combat.game, combat.attacker, combat.defender, combat.attacker_ultimate_source, stats);

	BOOL bDoDamage = TRUE;
	if (UnitIsInvulnerable(combat.defender))
	{
		bDoDamage = FALSE;
	}
	if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_GETHIT))
	{
		bDoDamage = FALSE;
	}
	if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_SOFTHIT))
	{
		bDoDamage = FALSE;
	}

	// apply local damages
	int dmg_avoided;
	int dmg = UnitAttackUnitApplyDamages<FALSE>(combat, stats, 0, bDoDamage, dmg_avoided, bFumbled);
	if (dmg <= 0)
	{
		// this is also effectively a 'block' as far as we are concerned with Mythos right now.
		// but Marsh about this!
		if( AppIsTugboat() &&
			dmg_avoided != 0 )
		{
			int blockMastery = UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERY );
			REF( blockMastery );
			if( (int)RandGetNum( combat.game, 1, 100 ) <= UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERY ) )
			{
				if( combat.defender != combat.attacker )
				{
					int nPCT = UnitGetStat( combat.defender, STATS_SKILL_BLOCKMASTERYPCT );
					s_sUnitReflectDamage( combat, stats, (nPCT == 0 ) ? 100 : nPCT );							
				}
			}
		}
		return;
	}
	// to ensure that a specific attack did successful damage - this only happens here, unlike standard TOOKDAMAGE
	UnitTriggerEvent(combat.game, EVENT_DIDHIT_TOOKDAMAGE, UnitGetResponsibleUnit( combat.attacker ), combat.defender, &combat);
	if (TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT))
	{
		UnitTriggerEvent(combat.game, EVENT_DIDHITMELEE_TOOKDAMAGE, combat.attacker, combat.defender, &combat);
	}
	else
	{
		UnitTriggerEvent(combat.game, EVENT_DIDHITRANGED_TOOKDAMAGE, combat.attacker, combat.defender, &combat);
	}

	// defender may do damage to attacker, with ranged_thorns_dmg_* properties, which is based on actual damage done
	if( !TESTBIT(combat.dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT) && !TESTBIT(combat.dwUnitAttackUnitFlags, UAU_MELEE_BIT) )
	{
		s_sUnitDoRangedThorns(combat, dmg);
	}

	// update hit points
	int hp = UnitGetStat(combat.defender, STATS_HP_CUR);
	if (bDoDamage)
	{
		hp = hp - dmg;
	}
	hp = MAX(hp, 0);

	if (GameGetDebugFlag(combat.game, DEBUGFLAG_ALWAYS_KILL) && !UnitIsA(combat.defender, UNITTYPE_PLAYER))
	{
		hp = 0;
	}

	UnitSetStat(combat.defender, STATS_HP_CUR, hp);

	// unit has taken damage..
	UnitTriggerEvent(combat.game, EVENT_TOOKDAMAGE, combat.defender, combat.attacker, stats);
	hp = UnitGetStat( combat.defender, STATS_HP_CUR );
	
	UnitAttackUnitCalculateResults(combat, stats, hp, dmg, dmg_avoided);

	// send damage message
	UnitAttackUnitSendDamageMessage(combat, hp);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sIsUsingDualFocus(
	UNIT * weapons[MAX_WEAPONS_PER_UNIT])
{
	if(!weapons )
	{
		return FALSE;
	}


	for(int i=0; i<MAX_WEAPONS_PER_UNIT; i++)
	{
		if(weapons[i])
		{
			if(!UnitIsA(weapons[i], UNITTYPE_CABALIST_FOCUS))
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitAttackUnit(
	UNIT * attacker,
	UNIT * defender,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT],
	int attack_index,
#ifdef HAVOK_ENABLED
	const HAVOK_IMPACT * pImpact,
#endif
	DWORD dwFlags,
	float fDamageMultiplier,			// damage multiplier, damage type override, min & max damage are all tugboat specific skill overrides
	int nDamageTypeOverride,			// which ought not be done the way they currently are (rather they should be done via stats)
	int minDamage,
	int maxDamage,
	UNIT * unitCausingAttack )
{
	ASSERT_RETURN(attacker && defender);

	COMBAT combat;
	CombatClear(combat);
	combat.game = UnitGetGame(attacker);
	ASSERT_RETURN(combat.game && IS_SERVER(combat.game));

	GamePushCombat(combat.game, &combat);

	if (RoomIsActive(UnitGetRoom(attacker)) == FALSE)
	{
		if (UnitEmergencyDeactivate(attacker, "Attempt to call s_UnitAttackUnit() in Inactive Room"))
		{
			return;
		}
	}

	UNIT* pPlayer = UnitIsA( defender, UNITTYPE_PLAYER ) ? defender : NULL;
	if( !pPlayer )
	{
		pPlayer = UnitGetResponsibleUnit(attacker);
		if( !UnitIsA( pPlayer, UNITTYPE_PLAYER ) )
		{
			pPlayer = NULL;
		}
	}
	combat.monscaling = GameGetMonsterScalingFactor(combat.game, UnitGetLevel( attacker ), pPlayer);

	combat.attacker = attacker;
	combat.defender = defender;

	ASSERT_RETURN(combat.attacker && combat.defender);
	combat.attacker_ultimate_source = UnitGetResponsibleUnit(combat.attacker);
	combat.defender_ultimate_source = UnitGetResponsibleUnit(combat.defender);

	// If another unit is causing this attack, then it becomes "responsible" for it
	UNITID idPreviouslyResponsibleUnit = INVALID_ID;
	if( unitCausingAttack != NULL )			
	{
		idPreviouslyResponsibleUnit = UnitGetUnitIdTag(combat.attacker, TAG_SELECTOR_RESPONSIBLE_UNIT);
		UnitSetUnitIdTag( combat.attacker, TAG_SELECTOR_RESPONSIBLE_UNIT, UnitGetId(unitCausingAttack) );
	}

	if (combat.attacker == combat.defender)		
	{
		combat.flags |= COMBAT_ALLOW_ATTACK_SELF;	//set the flag for reflecting.
	}
	if (combat.attacker &&
		UnitDataTestFlag(UnitGetData(combat.attacker), UNIT_DATA_FLAG_CAN_ATTACK_FRIENDS))
	{
		combat.flags |= COMBAT_ALLOW_ATTACK_FRIENDS;
	}

	if( TESTBIT( dwFlags, UAU_NO_DAMAGE_EFFECTS_BIT ) )
	{
		combat.flags |= COMBAT_DONT_ALLOW_DAMAGE_EFFECTS;
	}

	if( UnitGetGenus( combat.attacker_ultimate_source ) == GENUS_PLAYER &&
		UnitGetGenus( combat.defender_ultimate_source ) == GENUS_PLAYER )
	{
		combat.flags |= COMBAT_IS_PVP;
	}

	CombatInitWeapons(combat, weapons);
	ASSERT_RETURN(sCombatSetWeapons(attacker, weapons));

	CombatSetTargetType(combat);

	combat.damagemultiplier = fDamageMultiplier;
	combat.damagetypeoverride = nDamageTypeOverride;
	combat.base_min = minDamage;
	combat.base_max = maxDamage;

	combat.dwUnitAttackUnitFlags = dwFlags;
	if (s_sIsUsingDualFocus(weapons))
	{
		SETBIT(combat.dwUnitAttackUnitFlags, UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT);
	}
	combat.delivery_type = COMBAT_LOCAL_DELIVERY;

	combat.center_room = UnitGetRoom(combat.defender);
	combat.center_point = UnitGetPosition(combat.defender);

#ifdef HAVOK_ENABLED
	combat.pImpact = pImpact;
#endif

	s_UnitAttackUnit(combat, NULL);

	// do secondary attacks (DOT/AOE/FIELDS etc)
	UnitAttackUnitDoSecondaryAttacks(combat);

	StateClearAttackState(combat.attacker);

	if (unitCausingAttack != NULL)
	{
		UnitSetUnitIdTag(combat.attacker, TAG_SELECTOR_RESPONSIBLE_UNIT, idPreviouslyResponsibleUnit );
	}

	GamePopCombat(combat.game, &combat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitAttackLocation(
	UNIT * attacker,
	UNIT * weapons[MAX_WEAPONS_PER_UNIT],
	int attack_index,
	ROOM * room,
	VECTOR location,
#ifdef HAVOK_ENABLED
	const HAVOK_IMPACT & tImpact,
#endif
	DWORD dwFlags)
{
	ASSERT_RETURN(attacker && room);

	COMBAT combat;
	CombatClear(combat);
	combat.game = UnitGetGame(attacker);
	ASSERT_RETURN(combat.game && IS_SERVER(combat.game));

	GamePushCombat(combat.game, &combat);

	if (RoomIsActive(room) == FALSE)
	{
		if (UnitEmergencyDeactivate(attacker, "Attempt to call s_UnitAttackLocation() in Inactive Room"))
		{
			return;
		}
	}

	
	UNIT* pPlayer = (combat.defender && UnitIsA( combat.defender, UNITTYPE_PLAYER )) ? combat.defender : NULL;
	if( !pPlayer )
	{
		pPlayer = UnitGetResponsibleUnit(attacker);
		if( !UnitIsA( pPlayer, UNITTYPE_PLAYER ) )
		{
			pPlayer = NULL;
		}
	}
	combat.monscaling = GameGetMonsterScalingFactor(combat.game, UnitGetLevel( attacker ), pPlayer);

	combat.flags = 0;
	combat.attacker = attacker;
	combat.defender = NULL;
	CombatInitWeapons(combat, weapons);
	combat.center_room = room;
	combat.center_point = location;
	CombatSetTargetType(combat);

	ASSERT_RETURN(sCombatSetWeapons(attacker, combat.weapons));

	combat.dwUnitAttackUnitFlags = dwFlags;
	if(s_sIsUsingDualFocus(weapons))
	{
		SETBIT(combat.dwUnitAttackUnitFlags, UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT);
	}
	combat.delivery_type = COMBAT_SPLASH_DELIVERY;

#ifdef HAVOK_ENABLED
	combat.pImpact = &tImpact;
#endif

	// We do this here because the damage increment needs to get set in order for the balance to work properly
	combat.damage_increment = UnitGetStat(combat.attacker, STATS_DAMAGE_INCREMENT);

	// do secondary attacks (DOT/AOE/FIELDS etc)
	UnitAttackUnitDoSecondaryAttacks(combat);

	GamePopCombat(combat.game, &combat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitCanAttackUnit(
	const UNIT * attacker,
	const UNIT * defender)
{
	const UNIT_DATA * target_unit_data = UnitGetData(defender);
	ASSERT_RETFALSE(target_unit_data);

	if (target_unit_data->nRequiredAttackerUnitType != INVALID_ID && ! UnitIsA(attacker, target_unit_data->nRequiredAttackerUnitType))
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_CombatGetDamageIncrement(
	const COMBAT * pCombat)
{
	ASSERTX_RETVAL(pCombat, 0, "Expected Combat structure");
	return pCombat->damage_increment;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL GameToggleCombatTrace(
	GAME * game,
	DWORD flag)
{
	BOOL result = (BOOL)(TOGGLEBIT(game->m_dwCombatTraceFlags, flag) ? 1 : 0);
	if (game->m_dwCombatTraceFlags)
	{
		GameSetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE, TRUE);
	}
	else
	{
		GameSetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE, FALSE);
	}
	return result;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void GameSetCombatTrace(
	GAME * game,
	DWORD flag,
	BOOL val)
{
	SETBIT(game->m_dwCombatTraceFlags, flag, val);
	if (game->m_dwCombatTraceFlags)
	{
		GameSetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE, TRUE);
	}
	else
	{
		GameSetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE, FALSE);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL GameGetCombatTrace(
	GAME * game,
	DWORD flag)
{
	return (BOOL)(TESTBIT(game->m_dwCombatTraceFlags, flag) ? 1 : 0);
}
#endif

#endif // !ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_CombatSystemFlagSoundsForLoad(
	GAME * game)
{
	int nDamageTypeCount = ExcelGetNumRows(EXCEL_CONTEXT(game), DATATABLE_DAMAGETYPES);
	for(int ii=0; ii<nDamageTypeCount; ii++)
	{
		const DAMAGE_TYPE_DATA * pDamageTypeData = (const DAMAGE_TYPE_DATA *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_DAMAGETYPES, ii);
		if(!pDamageTypeData)
			continue;

		c_StateFlagSoundsForLoad(game, pDamageTypeData->nShieldHitState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nCriticalState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nSoftHitState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nMediumHitState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nBigHitState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nFumbleHitState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nDamageOverTimeState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nInvulnerableState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nInvulnerableSFXState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageTypeData->nThornsState, FALSE);

		if ( pDamageTypeData->nFieldMissile != INVALID_ID )
		{
			c_UnitDataFlagSoundsForLoad( game, DATATABLE_MISSILES, pDamageTypeData->nFieldMissile, FALSE );
		}
	}

	int nDamageEffectCount = ExcelGetNumRows(EXCEL_CONTEXT(game), DATATABLE_DAMAGE_EFFECTS);
	for(int ii=0; ii<nDamageEffectCount; ii++)
	{
		const DAMAGE_EFFECT_DATA * pDamageEffectData = (const DAMAGE_EFFECT_DATA *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_DAMAGE_EFFECTS, ii);
		if(!pDamageEffectData)
			continue;

		c_StateFlagSoundsForLoad(game, pDamageEffectData->nInvulnerableState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nAttackerProhibitingState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nDefenderProhibitingState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nAttackerRequiresState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nDefenderRequiresState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nSfxState, FALSE);
		c_StateFlagSoundsForLoad(game, pDamageEffectData->nSfxState, FALSE);

		if ( pDamageEffectData->nMissileToAttach != INVALID_ID )
		{
			c_UnitDataFlagSoundsForLoad( game, DATATABLE_MISSILES, pDamageEffectData->nMissileToAttach, FALSE );
		}
		if ( pDamageEffectData->nFieldMissile != INVALID_ID )
		{
			c_UnitDataFlagSoundsForLoad( game, DATATABLE_MISSILES, pDamageEffectData->nFieldMissile, FALSE );
		}

		c_SkillFlagSoundsForLoad(game, pDamageEffectData->nAttackerSkill, FALSE);
		c_SkillFlagSoundsForLoad(game, pDamageEffectData->nTargetSkill, FALSE);
	}
}
#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COMBAT * CombatGetNext(struct COMBAT * pCurrent)
{
	ASSERT_RETNULL(pCurrent);
	return pCurrent->next;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CombatSetNext(struct COMBAT * pCurrent, struct COMBAT * pNext)
{
	ASSERT_RETURN(pCurrent);
	pCurrent->next = pNext;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_CombatHasSecondaryAttacks(
	COMBAT * pCombat)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(pCombat);

	if (pCombat->defender && !CombatCheckMissileTag(*pCombat))
	{
		return FALSE;
	}

	if (TESTBIT(pCombat->dwUnitAttackUnitFlags, UAU_IS_THORNS_BIT))
	{
		return FALSE;
	}

	int num_radial_damages = 0;

	// loop through riders to find radial damages, fields, and damage over time
	STATS * rider = UnitGetRider(pCombat->attacker, NULL);
	while (rider)
	{
		STATS * next = UnitGetRider(pCombat->attacker, rider);

		STATS_ENTRY damage_min[1];
		int nDamageMin = StatsGetRange(rider, STATS_DAMAGE_MIN, 0, damage_min, 1);
		if (nDamageMin > 0)
		{
			int damage_type = damage_min[0].GetParam();

			int radius = StatsGetCalculatedStat(pCombat->attacker, NULL, rider, STATS_DAMAGE_RADIUS, (WORD)damage_type);
			if (radius > 0)
			{
				num_radial_damages++;
			}
		}

		rider = next;
	}

	if (num_radial_damages > 0)
	{
		return TRUE;
	}
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_CombatIsPrimaryAttack(
	COMBAT * pCombat)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(pCombat);

	if(pCombat->delivery_type == COMBAT_LOCAL_DELIVERY)
	{
		return TRUE;
	}
#endif
	return FALSE;
}
