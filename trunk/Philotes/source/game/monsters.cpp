//----------------------------------------------------------------------------
// monsters.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "units.h" // also includes game.h
#include "unit_priv.h"
#include "monsters.h"
#include "ai.h"
#include "s_message.h"
#include "globalIndex.h"
#include "c_appearance_priv.h"
#include "quests.h"
#include "script.h"
#include "skills.h"
#include "movement.h"
#include "unitmodes.h"
#include "combat.h"
#include "inventory.h"
#include "treasure.h"
#include "teams.h"
#include "items.h"
#include "filepaths.h"
#include "affix.h"
#include "stringtable.h"
#include "level.h"
#include "spawnclass.h"
#include "picker.h"
#include "states.h"
#include "c_sound.h"
#include "e_texture.h"
#include "e_model.h"
#include "pets.h"
#include "astar.h"
#include "room_path.h"
#include "c_quests.h"
#include "recipe.h"
#include "global_themes.h"
#include "wardrobe.h"
#include "stringreplacement.h"
#include "clients.h"
#include "gameunits.h"
#include "gamevariant.h"
#include "objects.h"

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sChampionMonsterInit(
	GAME *game,
	UNIT *unit,
	const int nQuality,
	const MONSTER_SPEC &tSpec)
{	
	const MONSTER_QUALITY_DATA * pMonsterQualityData = MonsterQualityGetData( game, nQuality );
	ASSERTX( pMonsterQualityData, "Expected monster quality entry" );		
	int nExperienceLevel = UnitGetExperienceLevel(unit);
	
	// run through quality based properites
	for (int ii = 0; ii < MAX_MONSTER_AFFIX; ++ii)
	{
		if (pMonsterQualityData->codeProperties[ii] != NULL_CODE)
		{
			int code_len = 0;
			BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_MONSTER_QUALITY, pMonsterQualityData->codeProperties[ii], &code_len);
			if (code_ptr)
			{
				VMExecI(game, unit, nExperienceLevel, nQuality, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code_ptr, code_len);
			}
		}
	}

	// if we are supposed to use the affixes provided, just do that
	if (TEST_MASK(tSpec.dwFlags, MSF_ONLY_AFFIX_SPECIFIED))
	{
		
		for (int i = 0; i < MAX_MONSTER_AFFIX; ++i)
		{
			int nAffix = tSpec.nAffix[ i ];
			if (nAffix != INVALID_LINK)
			{
				s_AffixApply(unit, nAffix);			
			}
		}
	}
	else
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		
		// how many affixes will we pick
		if (!UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NO_RANDOM_AFFIXES))
		{
			int nAffixCount = pMonsterQualityData->nAffixCount;
			
			// pick generic affixes
			for (int i = 0; i < nAffixCount; ++i)
			{

				int codelen = 0;
				BYTE *code = ExcelGetScriptCode( game, DATATABLE_MONSTER_QUALITY, pMonsterQualityData->codeAffixChance[ i ], &codelen );
				if (!code)
				{
					continue;
				}
						
				s_AffixPick(
					unit, 
					0,
					pMonsterQualityData->nAffixType[ i ], 
					NULL, 
					INVALID_LINK);
			}
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMonsterSetQuality(
	GAME *pGame,
	UNIT *pUnit,
	const int nMonsterQuality)
{

	// set stat marking quality value	
	UnitSetStat( pUnit, STATS_MONSTERS_QUALITY, nMonsterQuality );

	// set additional information for special qualities
	if (nMonsterQuality != INVALID_LINK)
	{
	
		// set the champion state
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );
		const STATE_DATA *pStateData = StateGetData( pGame, pMonsterQualityData->nState );
		if (pStateData)
		{
			s_StateSet( pUnit, pUnit, pMonsterQualityData->nState, 0 );
		}

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )	
inline int sCapMaxStatForMonsterInit( GAME *game, unsigned int nStat, int nValue )
{
	const STATS_DATA *pStatValue = StatsGetData( game, nStat );	
	ASSERT_RETINVALID( pStatValue );	
	if( (unsigned int)nValue >= (unsigned int)(1 << pStatValue->m_nValBits) )   //monster asserts and won't appear on client
	{
		if( AppIsHellgate() )  //hellgate still asserts like before. Tugboat we set some stats at max
		{
			ASSERT( (unsigned int)nValue < (unsigned int)(1 << pStatValue->m_nValBits) );
		}
		return (1 << pStatValue->m_nValBits) -1;
	}
	return nValue;
}


void s_MonsterInitStats(
	GAME * game,
	UNIT * unit,
	const MONSTER_SPEC &tSpec)
{
	int nClass = tSpec.nClass;
	int nExperienceLevel = tSpec.nExperienceLevel;
	int nQuality = tSpec.nMonsterQuality;
	const UNIT_DATA * monster_data = MonsterGetData(game, UnitGetClass(unit));

#ifdef _DEBUG
	if (game->m_nDebugMonsterLevel != 0)
	{
		nExperienceLevel = game->m_nDebugMonsterLevel;
	}
#endif
	nExperienceLevel = MAX(nExperienceLevel, monster_data->nMinMonsterExperienceLevel);

	if( AppIsTugboat() )
	{
		if( nExperienceLevel < monster_data->nBaseLevel )
		{
			nExperienceLevel = monster_data->nBaseLevel;
		}
		if( nExperienceLevel < 0 )
		{
			nExperienceLevel = 1;
		}
	}

	UnitSetExperienceLevel(unit, nExperienceLevel);

	// evaluate props - you should do this before getting the unit's level
	if( !TEST_MASK(tSpec.dwFlags, MSF_IS_LEVELUP) )
	{
		for (int ii = 0; ii < NUM_UNIT_PROPERTIES; ii++)
		{
			ExcelEvalScript(game, unit, NULL, NULL, DATATABLE_MONSTERS, (DWORD)OFFSET(UNIT_DATA, codeProperties[ii]), nClass);
		}
	}

	nExperienceLevel = UnitGetExperienceLevel( unit );

	const MONSTER_LEVEL * monster_level = NULL;
	if(UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_USES_PETLEVEL))
	{
		monster_level = (MONSTER_LEVEL *)ExcelGetData(game, DATATABLE_PETLEVEL, nExperienceLevel);
	}
	else
	{
		monster_level = (MONSTER_LEVEL *)ExcelGetData(game, DATATABLE_MONLEVEL, nExperienceLevel);
	}
	if (!monster_level)
	{
		return;
	}

	const MONSTER_QUALITY_DATA *monster_quality = NULL;
	if (nQuality > INVALID_LINK)
	{
		monster_quality = MonsterQualityGetData(game, nQuality);
	}
	
	if ( AppIsTugboat() &&
		UnitIsA( unit, UNITTYPE_HIRELING ) )
	{// strength and dexterity
		int nDexterityPercent = monster_data->nDexterityPercent;
		int nStrengthPercent  = monster_data->nStrengthPercent;
		if (nQuality > INVALID_LINK)
		{
			nDexterityPercent = MAX( nDexterityPercent, 100 );
			nStrengthPercent  = MAX( nStrengthPercent, 100 );
		}
		int nDexterity = PCT( monster_level->nDexterity, nDexterityPercent );
		int nStrength  = PCT( monster_level->nStrength,  nStrengthPercent );

		UnitSetStat( unit, STATS_DEXTERITY,nDexterity );
		UnitSetStat( unit, STATS_STRENGTH, nStrength );
	}
	
	// hitpoints
	int hp;
	if (nQuality > INVALID_LINK)
	{// do champion quality health modifiers
		int nPercent = MAX( monster_data->nMaxHitPointPct, 100 );
		hp = (int)(((float)monster_level->nHitPoints * nPercent * monster_quality->flHealthMultiplier) / 100.0f);
	} else {
		int min_hp = monster_level->nHitPoints * monster_data->nMinHitPointPct / 100;
		int max_hp = monster_level->nHitPoints * monster_data->nMaxHitPointPct / 100;
		hp = RandGetNum(game, min_hp, max_hp);	
	}
	UnitSetStatShift(unit, STATS_HP_MAX, hp);

	// resistances
	if( !TEST_MASK(tSpec.dwFlags, MSF_IS_LEVELUP) )
	{
		if ( AppIsTugboat() && nQuality > INVALID_LINK)
		{
			// bosses get random resistances
			for (int ii = 1; ii < NUM_DAMAGE_TYPES; ii++)
			{
				//bit of a hack but lets multiply the max number by the max level of a monster
				int nLevel = UnitGetStat( unit, STATS_LEVEL );
				int maxNum = PCT( 200, nLevel );	//basically at level ~38 monsters will have the ability to reach a max of 75
				maxNum = min( 75, maxNum );			//max at 75
				int minNum = maxNum/2 * -1;			//why not make some monster be weak ot elements?
				if( RandGetNum( game, 10 ) < 4 )
				{
					int Vulnerability = RandGetNum( game, minNum, maxNum ) * -1;
					Vulnerability += UnitGetStat( unit, STATS_ELEMENTAL_VULNERABILITY, ii );
					Vulnerability = min( 100, Vulnerability );	
					Vulnerability = max( -100, Vulnerability );	
					UnitSetStat(unit, STATS_ELEMENTAL_VULNERABILITY, ii, Vulnerability);
				}
			}
		}
	}

	// experience
	int experience;
	if (nQuality > INVALID_LINK)
	{
		int nPercent = MAX( monster_data->nExperiencePct, 100 ); // make the wimpy guys better

		int nQualityMultiplier = monster_quality->nExperienceMultiplier;
		ASSERT_DO( nQualityMultiplier > 0 ) { nQualityMultiplier = 1; }

		experience = ( monster_level->nExperience * nPercent * nQualityMultiplier ) / 100;	
	}
	else
	{
		experience = PCT(monster_level->nExperience, monster_data->nExperiencePct);
		if( AppIsTugboat() && monster_data->nExperiencePct > 0 && experience <= 0)
		{
			experience = 1; //always make it 1 if the monster Experience is not zero
		}
	}
	UnitSetStatShift(unit, STATS_EXPERIENCE, experience);



	// attack rating
	int attack_rating = PCT(monster_level->nAttackRating, monster_data->nAttackRatingPct);
	UnitSetStatShift(unit, STATS_ATTACK_RATING, attack_rating);

	// damage
	int min_dmg_pct = ExcelEvalScript(game, unit, NULL, NULL, DATATABLE_MONSTERS, OFFSET(UNIT_DATA, codeMinBaseDmg), nClass);
	int max_dmg_pct = ExcelEvalScript(game, unit, NULL, NULL, DATATABLE_MONSTERS, OFFSET(UNIT_DATA, codeMaxBaseDmg), nClass);
	if ( min_dmg_pct || max_dmg_pct )
	{
		int min_damage = PCT((monster_level->nDamage << StatsGetShift(game, STATS_BASE_DMG_MIN)), min_dmg_pct);
		UnitSetStat(unit, STATS_BASE_DMG_MIN, min_damage);

		int max_damage = PCT((monster_level->nDamage << StatsGetShift(game, STATS_BASE_DMG_MAX)), max_dmg_pct);
		UnitSetStat(unit, STATS_BASE_DMG_MAX, max_damage);

		if (monster_data->nDamageType > 0)
		{
			UnitSetStatShift(unit, STATS_DAMAGE_MIN, monster_data->nDamageType, 100);
			UnitSetStatShift(unit, STATS_DAMAGE_MAX, monster_data->nDamageType, 100);								
		}
	}

	for (int ii = 0; ii < NUM_DAMAGE_TYPES; ii++)
	{
		DAMAGE_TYPES eDamageType = (DAMAGE_TYPES)ii;
		
		// armor
		if (monster_data->fArmor[eDamageType] > 0.0f)
		{
			if( AppIsHellgate() )
			{
				int armor = PCT(monster_level->nArmor << StatsGetShift(game, STATS_ARMOR), monster_data->fArmor[eDamageType]);
				int armor_buffer = PCT(monster_level->nArmorBuffer << StatsGetShift(game, STATS_ARMOR_BUFFER_MAX), monster_data->fArmor[eDamageType]);
				int armor_regen = PCT(monster_level->nArmorRegen << StatsGetShift(game, STATS_ARMOR_BUFFER_REGEN), monster_data->fArmor[eDamageType]);
				UnitSetStat(unit, STATS_ARMOR, eDamageType, armor);
				UnitSetStat(unit, STATS_ARMOR_BUFFER_MAX, eDamageType, armor_buffer);
				UnitSetStat(unit, STATS_ARMOR_BUFFER_CUR, eDamageType, armor_buffer);
				UnitSetStat(unit, STATS_ARMOR_BUFFER_REGEN, eDamageType, armor_regen);
			}
			else
			{
				int armor = PCT(monster_level->nArmor << StatsGetShift(game, STATS_AC), monster_data->fArmor[eDamageType]);
				UnitSetStat(unit, STATS_AC, eDamageType, armor);

			}
		} 
		else 
		{
			if( AppIsHellgate() )
			{
				UnitSetStat(unit, STATS_ELEMENTAL_VULNERABILITY, eDamageType, int(monster_data->fArmor[eDamageType]));
			}
		}
		
		// shields
		if ( AppIsHellgate() && monster_data->fShields[eDamageType] > 0.0f)
		{
			int shield = PCT(monster_level->nShield << StatsGetShift(game, STATS_SHIELDS), monster_data->fShields[eDamageType]);
			int shield_buffer = PCT(monster_level->nShield << StatsGetShift(game, STATS_SHIELD_BUFFER_MAX), monster_data->fShields[eDamageType]);
			//int shield_buffer = PCT(monster_level->nShieldBuffer << StatsGetShift(game, STATS_SHIELD_BUFFER_MAX), monster_data->nShields[eDamageType]);
			int shield_regen = monster_level->nShieldRegen << StatsGetShift(game, STATS_SHIELD_BUFFER_REGEN);
			UnitSetStat(unit, STATS_SHIELDS, shield);
			UnitSetStat(unit, STATS_SHIELD_BUFFER_MAX, shield_buffer);
			UnitSetStat(unit, STATS_SHIELD_BUFFER_CUR, shield_buffer);
			UnitSetStat(unit, STATS_SHIELD_BUFFER_REGEN, shield_regen);
		}

		// defense against special effects
		const SPECIAL_EFFECT *pSpecialEffect = &monster_data->tSpecialEffect[ eDamageType ];		
		int nSfxDefense = PCT( monster_level->nSfxDefense, pSpecialEffect->nDefensePercent ); 
		UnitSetStat( unit, STATS_SFX_DEFENSE_BONUS, eDamageType, nSfxDefense );

		int sfx_ability = PCT(monster_level->nSfxAttack, pSpecialEffect->nAbilityPercent);
		if ( sfx_ability )
			UnitSetStat(unit, STATS_SFX_ATTACK, eDamageType, sfx_ability);

		if ( pSpecialEffect->nStrengthPercent )
			UnitSetStat( unit, STATS_SFX_STRENGTH_PCT, eDamageType, pSpecialEffect->nStrengthPercent );

		if ( pSpecialEffect->nDurationInTicks )
			UnitSetStat( unit, STATS_SFX_DURATION_IN_TICKS, eDamageType, pSpecialEffect->nDurationInTicks );
		
	}

	int nInterruptAttack = StatsRoundStatPct(game, STATS_INTERRUPT_ATTACK, monster_level->nInterruptAttack, monster_data->nInterruptAttackPct);	
	nInterruptAttack = sCapMaxStatForMonsterInit( game, STATS_INTERRUPT_ATTACK, nInterruptAttack ); //lets cap this
	UnitSetStatShift(unit, STATS_INTERRUPT_ATTACK,	nInterruptAttack);
	int nInterruptDefense = StatsRoundStatPct(game, STATS_INTERRUPT_DEFENSE, monster_level->nInterruptDefense, monster_data->nInterruptDefensePct);
	nInterruptDefense = sCapMaxStatForMonsterInit( game, STATS_INTERRUPT_DEFENSE, nInterruptDefense ); //lets cap this
	UnitSetStatShift(unit, STATS_INTERRUPT_DEFENSE,	nInterruptDefense);
	UnitSetStatShift(unit, STATS_INTERRUPT_CHANCE, monster_data->nInterruptChance);

	int nStealthDefense = StatsRoundStatPct(game, STATS_STEALTH_DEFENSE, monster_level->nStealthDefense, monster_data->nStealthDefensePct);
	UnitSetStatShift(unit, STATS_STEALTH_DEFENSE,	nStealthDefense);

	int nAIChangeDefense = PCT(monster_level->nAIChangeDefense, monster_data->nAIChangeDefense);
	if(nAIChangeDefense)
	{
		UnitSetStat(unit, STATS_AI_CHANGE_DEFENSE, nAIChangeDefense);
	}

	// corpse explode points
	int nCorpseExplodePoints = PCT(monster_level->nCorpseExplodePoints, monster_data->nCorpseExplodePoints);
	if(nCorpseExplodePoints > 0)
	{
		UnitSetStat(unit, STATS_CORPSE_EXPLODE_POINTS, nCorpseExplodePoints);
	}

	// tohit
	if (monster_data->nToHitBonus > 0)
	{
		int tohit = PCT(monster_level->nToHitBonus << StatsGetShift(game, STATS_TOHIT), monster_data->nToHitBonus);
		if (nQuality > INVALID_LINK)
		{
			// do champion quality tohit modifiers
			tohit = (int)((float)tohit * monster_quality->flToHitMultiplier);
		} 
		UnitSetStat(unit, STATS_TOHIT, tohit );
	} 

	// set treasure for monster (champions have another treasure class)
	int nTreasure = monster_data->nTreasure;
	if (nQuality != INVALID_LINK && monster_data->nTreasureChampion != INVALID_LINK)
	{
		nTreasure = monster_data->nTreasureChampion;		
	}
	UnitSetStat(unit, STATS_TREASURE_CLASS, nTreasure);

	if( !TEST_MASK(tSpec.dwFlags, MSF_IS_LEVELUP) )
	{
		for (int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++)
		{
			if (monster_data->nStartingSkills[ii] != INVALID_ID)
			{
				UnitSetStat(unit, STATS_SKILL_LEVEL, monster_data->nStartingSkills[ii], 1);
			}
		}
		if (monster_data->nStartingSkills[0] != INVALID_ID)
		{
			UnitSetStat(unit, STATS_AI_SKILL_PRIMARY, monster_data->nStartingSkills[0]);
		}
	}

	UnitSetStat(unit, STATS_AI_FLAGS, 0);
	// champions cannot be interrupted!
	if ( AppIsTugboat() && nQuality > INVALID_LINK )
	{
		UnitSetStat(unit, STATS_INTERRUPT_DEFENSE, 100 );
	}
	
	// apply any hand picked affixes
	if( !TEST_MASK(tSpec.dwFlags, MSF_IS_LEVELUP) )
	{
		if (IS_SERVER( game ))
		{
			for (int i = 0; i < AffixGetMaxAffixCount(); ++i)
			{
				int nAffix = monster_data->nAffixes[ i ];
				if (nAffix != INVALID_LINK)
				{
					s_AffixApply( unit, nAffix );
				}			
			}

			if ( GameIsVariant( game, GV_ELITE ) )
			{
				ExcelEvalScript(game, unit, NULL, NULL, DATATABLE_MONSTERS, (DWORD)OFFSET(UNIT_DATA, codeEliteProperties), nClass);
			}
		}
			
		// pick affixes for champion monsters
		if (nQuality > INVALID_LINK)
		{
			sChampionMonsterInit(game, unit, nQuality, tSpec);
		}	
	}

	if( !TEST_MASK(tSpec.dwFlags, MSF_IS_LEVELUP) )
	{
		// set quality data	
		sMonsterSetQuality( game, unit, tSpec.nMonsterQuality );

		// pick unique name for monster if we can
		if (MonsterGetQuality( unit ) != INVALID_LINK)
		{
			int nMonsterName = MonsterProperNamePick( game );
			MonsterProperNameSet( unit, nMonsterName, FALSE);
		}
	}
		
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleDoEndOfLifeCode(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* ,
	EVENT_DATA* pEventData,
	void*,
	DWORD dwId )
{
	PCODE code( NULL );
	UNIT_EVENT eEvent = (UNIT_EVENT) pEventData->m_Data1;
	{
		
		ASSERT(UnitGetGenus(pUnit) == GENUS_MONSTER);
		const UNIT_DATA * monster_data = MonsterGetData(pGame, UnitGetClass(pUnit));

		switch ( eEvent )
		{
		case EVENT_UNITDIE_BEGIN:			
			break;
		case EVENT_UNITDIE_END:
			code = monster_data->codeScriptUnitDieBegin;
			break;
		case EVENT_DEAD:
		case EVENT_ON_FREED:			
			break;
		default:
			ASSERT_RETURN( 0 );
			break;
		}
	}

	if ( !code )
		return;
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_MONSTERS, code, &code_len );
	if ( code_ptr )
	{		
		VMExecI( pGame, pUnit, NULL, 0, 0, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code_ptr, code_len );
	}

	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleDoEndOfLifeSkill(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* ,
	EVENT_DATA* pEventData,
	void*,
	DWORD dwId )
{
	UNIT_EVENT eEvent = (UNIT_EVENT) pEventData->m_Data1;
	int nSkill;
	{// this should just be grabbing from the event data - not going to the class
		ASSERT(UnitGetGenus(pUnit) == GENUS_MONSTER);
		const UNIT_DATA * monster_data = MonsterGetData(pGame, UnitGetClass(pUnit));
		ASSERT_RETURN(monster_data);
		switch ( eEvent )
		{
		case EVENT_UNITDIE_BEGIN:
			if ( IS_SERVER( pGame ) )
			{
				nSkill = monster_data->nUnitDieBeginSkill;
			}
			else
			{
				nSkill = monster_data->nUnitDieBeginSkillClient;
			}
			break;
		case EVENT_UNITDIE_END:
			nSkill = monster_data->nUnitDieEndSkill;
			break;
		case EVENT_DEAD:
		case EVENT_ON_FREED:
			nSkill = monster_data->nDeadSkill;
			break;
		default:
			ASSERT_RETURN( 0 );
			break;
		}
	}

	if ( nSkill == INVALID_ID )
		return;

	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	SkillStartRequest( pGame, pUnit, nSkill, INVALID_ID, VECTOR(0), dwFlags, SkillGetNextSkillSeed( pGame ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleFuseMonsterEvent( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	UnitDie( pUnit, NULL );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterSetFuse(
	UNIT * pMonster,
	int nTicks)
{
	UnitRegisterEventTimed( pMonster, sHandleFuseMonsterEvent, &EVENT_DATA(), nTicks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterUnFuse(
	UNIT * pMonster)
{
	UnitUnregisterTimedEvent(pMonster, sHandleFuseMonsterEvent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterGetFuse(
	UNIT * pMonster)
{
	return UnitGetStatShift(pMonster, STATS_FUSE_END_TICK);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMonsterCheckShouldPath(
	UNIT* pUnit)
{
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_DONT_PATH))
	{
		return FALSE;
	}
	if (UnitIsA(pUnit, UNITTYPE_NOTARGET))
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void NPCSoundsLoad(
	GAME * game,
	UNIT * unit)
{
	const UNIT_DATA* pUnitData = unit->pUnitData;
	if (pUnitData->nInteractSound != INVALID_LINK)
	{
		CLT_VERSION_ONLY( c_SoundLoad( pUnitData->nInteractSound ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterSpecInit(
	MONSTER_SPEC &tSpec)
{
	tSpec.id = INVALID_ID;
	tSpec.guidUnit = INVALID_GUID;
	tSpec.nClass = INVALID_LINK;
	tSpec.nExperienceLevel = NONE;
	tSpec.nMonsterQuality = INVALID_LINK;
	tSpec.nTeam = INVALID_TEAM;
	tSpec.pRoom = NULL;
	tSpec.pPathNode = NULL;
	tSpec.vPosition = cgvNone;
	tSpec.vDirection = cgvXAxis;
	tSpec.nWeaponClass = INVALID_LINK;
	tSpec.pOwner = NULL;
	tSpec.flVelocityBase = 0.0f;
	tSpec.flScale = -1.0f;
	tSpec.pShape = NULL;
	tSpec.pvUpDirection = NULL;
	tSpec.dwFlags = 0;
	
	for (int i = 0; i < MAX_MONSTER_AFFIX; ++i)
	{
		tSpec.nAffix[ i ] = INVALID_LINK;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sModifyClassByQuality(	
	GAME *pGame,
	MONSTER_SPEC &tSpec)
{
	
	// if is at unique quality, see if we can change the monster row to a specially hand crafted monster
	if (tSpec.nMonsterQuality != INVALID_LINK)
	{
		if (MonsterQualityGetType( tSpec.nMonsterQuality ) == MQT_UNIQUE)
		{
			// only change rows that link to a different row as their unique
			const UNIT_DATA *pMonsterData = MonsterGetData( NULL, tSpec.nClass );
			if (MonsterQualityGetType( pMonsterData->nMonsterQualityRequired ) < MQT_UNIQUE)	
			{
				if (pMonsterData->nMonsterClassAtUniqueQuality != INVALID_LINK && 
					pMonsterData->nMonsterClassAtUniqueQuality != tSpec.nClass)
				{
					tSpec.nClass = pMonsterData->nMonsterClassAtUniqueQuality;
				}

				// if the chosen row is not for a hand crafted unique, can't make unique, downgrade quality
				pMonsterData = MonsterGetData( NULL, tSpec.nClass );
				if (MonsterQualityGetType( pMonsterData->nMonsterQualityRequired ) != MQT_UNIQUE)
				{
					const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( NULL, tSpec.nMonsterQuality );
					tSpec.nMonsterQuality = pMonsterQualityData->nMonsterQualityDowngrade;
				}
			}
			
		}
		else
		{
			// we have this fallback 'all champion' changeover too
			const UNIT_DATA *pMonsterData = MonsterGetData( NULL, tSpec.nClass );
			if (pMonsterData->nMonsterClassAtChampionQuality != INVALID_LINK && 
				pMonsterData->nMonsterClassAtChampionQuality != tSpec.nClass)
			{
				tSpec.nClass = pMonsterData->nMonsterClassAtChampionQuality;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsClientOnlyMonsterInit(
	GAME *pGame,
	const UNIT_DATA *pMonsterData,
	const MONSTER_SPEC &tSpec)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	return TEST_MASK( tSpec.dwFlags, MSF_FORCE_CLIENT_ONLY ) || 
			(UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_CLIENT_ONLY ) && IS_CLIENT( pGame ));
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * MonsterInit(
	GAME * game,
	MONSTER_SPEC &tSpec)
{
	ROOM *room = tSpec.pRoom;

	// change the monster class for uniques (if we can)
	sModifyClassByQuality( game, tSpec );
		
	//ASSERT_RETNULL(room || vPosition == INVALID_POSITION); // this is turned off because we are sending extra units to the client
	if ( IS_CLIENT( game ) )
	{
		if ( !room && tSpec.vPosition != INVALID_POSITION )
		{
			return NULL;
		}
	} 
	else 
	{
		ASSERT_RETNULL(room || tSpec.vPosition == INVALID_POSITION );
	}

	const UNIT_DATA * monster_data = MonsterGetData(game, tSpec.nClass);
	if (!monster_data)
	{
		return NULL;
	}
	if ( IS_CLIENT( game ) )
	{ // in some territories we have to substitute monsters to meet local laws
		if ( AppTestFlag( AF_CENSOR_NO_HUMANS ) && monster_data->nCensorBackupClass_NoHumans != INVALID_ID )
		{
			tSpec.nClass = monster_data->nCensorBackupClass_NoHumans;
			monster_data = MonsterGetData(game, tSpec.nClass);
			if (!monster_data)
			{
				return NULL;
			}
		}
		if ( AppTestFlag( AF_CENSOR_NO_GORE ) && monster_data->nCensorBackupClass_NoGore != INVALID_ID )
		{
			tSpec.nClass = monster_data->nCensorBackupClass_NoGore;
			monster_data = MonsterGetData(game, tSpec.nClass);
			if (!monster_data)
			{
				return NULL;
			}
		}
	}

	UnitDataLoad( game, DATATABLE_MONSTERS, tSpec.nClass );

	// adjust required monster qualitites
	if (monster_data->nMonsterQualityRequired != INVALID_LINK)
	{
		tSpec.nMonsterQuality = monster_data->nMonsterQualityRequired;
	}
	
	VECTOR vPosition = tSpec.vPosition;
	if(IS_SERVER(game))
	{
	
		// spawn in air - this needs to be better and see if that's ok to do!	
		if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_SPAWN_IN_AIR))
		{
			vPosition.fZ += MONSTERS_SPAWN_IN_AIR_ALTITUDE;
		}
		
		// if supposed to snap to nearest pathnode, do it now!
		if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_SNAP_TO_PATH_NODE_ON_CREATE) && tSpec.pRoom)
		{
			LEVEL *pLevel = RoomGetLevel( tSpec.pRoom );
			// removed from Mythos
//			ROOM * pRoomDestination = tSpec.pRoom;
//			ROOM_PATH_NODE * pPathNode = RoomGetNearestFreeConnectedPathNode( game, pLevel, vPosition, &pRoomDestination, NULL );

			ROOM_PATH_NODE *pPathNode = RoomGetNearestFreePathNode( 
				game, 
				pLevel,
				vPosition,
				NULL,
				NULL,
				0.0f,
				0.0f,
				tSpec.pRoom);
				
			if (pPathNode)
			{
				vPosition = RoomPathNodeGetExactWorldPosition( game, tSpec.pRoom, pPathNode );
			}
				
		}
		
	}


	UNIT_CREATE newunit( monster_data );
	newunit.species = MAKE_SPECIES(GENUS_MONSTER, tSpec.nClass);
	newunit.unitid = tSpec.id;
	newunit.guidUnit = tSpec.guidUnit;
	newunit.pRoom = tSpec.pRoom;
	newunit.vPosition = vPosition;
	newunit.vFaceDirection = tSpec.vDirection;
	newunit.fScale = tSpec.flScale;
	newunit.nQuality = tSpec.nMonsterQuality;
	newunit.pAppearanceShape = tSpec.pShape;
	SET_MASKV(newunit.dwUnitCreateFlags, UCF_NO_DATABASE_OPERATIONS, TEST_MASK(tSpec.dwFlags, MSF_NO_DATABASE_OPERATIONS));
	if (tSpec.pvUpDirection)	
	{
		newunit.vUpDirection = *tSpec.pvUpDirection;
	}
	
	// if no scale was explicitly given, pick one
	if (tSpec.flScale < 0)
	{
		if (newunit.nQuality == INVALID_LINK)
		{
			newunit.fScale = RandGetFloat(game, monster_data->fScale, monster_data->fScale + monster_data->fScaleDelta);
		}
		else
		{
			newunit.fScale = RandGetFloat(game, monster_data->fChampionScale, monster_data->fChampionScale + monster_data->fChampionScaleDelta);			
		}	
	}
	
#if ISVERSION(DEBUG_ID)
	PStrPrintf(newunit.szDbgName, MAX_ID_DEBUG_NAME, "%s", monster_data->szName);
#endif

	// create flags
	if ( sIsClientOnlyMonsterInit( game, monster_data, tSpec ) == TRUE )
	{
		SET_MASK(newunit.dwUnitCreateFlags, UCF_CLIENT_ONLY);
	}
	if (TEST_MASK(tSpec.dwFlags, MSF_DONT_DEACTIVATE_WITH_ROOM) || UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_DONT_DEACTIVATE_WITH_ROOM))
	{
		SET_MASK(newunit.dwUnitCreateFlags, UCF_DONT_DEACTIVATE_WITH_ROOM);	
	}
	if (TEST_MASK(tSpec.dwFlags, MSF_DONT_DEPOPULATE) || UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_DONT_DEPOPULATE))
	{
		SET_MASK(newunit.dwUnitCreateFlags, UCF_DONT_DEPOPULATE	);	
	}

	UNIT * unit = UnitInit(game, &newunit);
	if (!unit)
	{
		return NULL;
	}
	UnitInvNodeInit(unit);

	// do this BEFORE stats initialization - some things end up evaluating targets immediately and causing issues
	int nTeam = tSpec.nTeam;
	if (nTeam == INVALID_TEAM)
	{
		nTeam = UnitGetDefaultTeam(game, GENUS_MONSTER, tSpec.nClass, monster_data);
	}
	TeamAddUnit(unit, nTeam);

	if (IS_SERVER(game))
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )
		s_MonsterInitStats(game, unit, tSpec);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	}
	UnitSetFlag(unit, UNITFLAG_NOMELEEBLEND);

	// targeting & collision
	if (sMonsterCheckShouldPath(unit))
	{

		// init pathing stuff	
		UnitPathingInit(unit);

		ROOM_PATH_NODE * pPathNode = tSpec.pPathNode;
		if(!pPathNode)
		{
			DWORD dwFreePathNodeFlags = 0;
			// we have to check for NULL rooms, because this might be a saved hireling, and
			// there might not BE a room yet.
			if( room != NULL )
			{
				pPathNode = RoomGetNearestFreePathNode(game, RoomGetLevel(room), tSpec.vPosition, &room, NULL, 0.0f, 0.0f, NULL, dwFreePathNodeFlags);
			}

		}
		// we might not have these! This could be a loading hireling, and there's no room yet for him, nor pathnode.
		//ASSERT(room);
		//ASSERT(pPathNode);
		if (room && pPathNode)
		{
			VECTOR vPosition = tSpec.vPosition;
			// we're close enough in Mythos, and don't require exacting node placement.
			if( AppIsHellgate() )
			{
				if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_SPAWN_IN_AIR))
				{
					UnitSetStatFloat( unit, STATS_ALTITUDE, 0, MONSTERS_SPAWN_IN_AIR_ALTITUDE );
					vPosition = UnitCalculatePathTargetFly(game, unit, room, pPathNode);
				}
				else
				{
					vPosition = RoomPathNodeGetExactWorldPosition(game, room, pPathNode);
				}
			}
			
			UnitChangePosition(unit, vPosition);
			UnitInitPathNodeOccupied( unit, room, pPathNode->nIndex );
			UnitReservePathOccupied( unit );

			if( AppIsTugboat() )
			{
				// snap 'em to the ground!
				UnitForceToGround(unit);
			}
		}
	}
	else
	{
		UnitSetFlag(unit, UNITFLAG_ROOMCOLLISION);
	}

	// KCK: Block path nodes if this is a blocking type 
	// Ideally I would have liked to put this in UnitInit(), but the position isn't finalized
	// at that point, and I would just have to do it again here anyway.
	if ( UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_BLOCKS_EVERYTHING ) )
		ObjectReserveBlockedPathNodes(unit);

	UnitSetFlag(unit, UNITFLAG_COLLIDABLE);
	UnitSetFlag(unit, UNITFLAG_CANBELOCKEDON);
	UnitSetFlag(unit, UNITFLAG_ON_DIE_DESTROY, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_ON_DIE_DESTROY));
	UnitSetFlag(unit, UNITFLAG_ON_DIE_END_DESTROY, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_ON_DIE_END_DESTROY));
	UnitSetFlag(unit, UNITFLAG_FORCE_FADE_ON_DESTROY, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_FADE_ON_DESTROY));
	UnitSetFlag(unit, UNITFLAG_ON_DIE_HIDE_MODEL, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_ON_DIE_HIDE_MODEL));	
	UnitSetFlag(unit, UNITFLAG_WALLWALK, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_WALL_WALK));
	UnitSetFlag(unit, UNITFLAG_NOSYNCH, UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_DONT_SYNCH));
	if( AppIsHellgate() )
	{
		UnitSetFlag(unit, UNITFLAG_ALWAYS_AIM_AT_TARGET, TRUE);
		UnitSetFlag(unit, UNITFLAG_USE_APPEARANCE_MUZZLE_OFFSET, TRUE);
	}
	UnitSetFlag(unit, UNITFLAG_CAN_REQUEST_LEFT_HAND, TRUE);
	UnitSetFlag(unit, UNITFLAG_BOUNCE_OFF_CEILING);

	// targeting & collision

	UnitSetDefaultTargetType(game, unit, monster_data);

	unit->dwTestCollide = 0;
	if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_COLLIDE_GOOD))
	{
		unit->dwTestCollide |= TARGETMASK_GOOD;
	}
	if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_COLLIDE_BAD))
	{
		unit->dwTestCollide |= TARGETMASK_BAD;
	}

	if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
	{
		UnitSetFlag( unit, UNITFLAG_DESTROY_DEAD_NEVER, TRUE );
	}

	
	if ( monster_data->codeScriptUnitDieBegin && IS_SERVER(game))
	{
		UnitRegisterEventHandler(game, unit, EVENT_UNITDIE_END,		sHandleDoEndOfLifeCode, &EVENT_DATA( EVENT_UNITDIE_END ));
	}

	if ( monster_data->nUnitDieBeginSkill	!= INVALID_ID && IS_SERVER(game) )
	{
		UnitRegisterEventHandler(game, unit, EVENT_UNITDIE_BEGIN,	sHandleDoEndOfLifeSkill, &EVENT_DATA( EVENT_UNITDIE_BEGIN ));
	}

	if ( monster_data->nUnitDieBeginSkillClient	!= INVALID_ID && IS_CLIENT(game) )
	{
		UnitRegisterEventHandler(game, unit, EVENT_UNITDIE_BEGIN,	sHandleDoEndOfLifeSkill, &EVENT_DATA( EVENT_UNITDIE_BEGIN ));
	}


	if ( monster_data->nUnitDieEndSkill		!= INVALID_ID && IS_SERVER(game))
	{
		UnitRegisterEventHandler(game, unit, EVENT_UNITDIE_END,		sHandleDoEndOfLifeSkill, &EVENT_DATA( EVENT_UNITDIE_END ));
	}

	if ( monster_data->nDeadSkill			!= INVALID_ID && IS_SERVER(game))
	{
		UnitRegisterEventHandler(game, unit, EVENT_DEAD,			sHandleDoEndOfLifeSkill, &EVENT_DATA( EVENT_DEAD ));
	}

	if ( GameIsVariant( game, GV_ELITE ) )
	{
		// TRAVIS: In Mythos, we don't want to increase the range and speed of pets!
		// Revisit and make this a better solution, with a flag - need to determine
		// what should actually be disabled pet-wise, and get this in Hellgate as well,
		// when I'm not terrified of breaking it.
		if( AppIsHellgate() || !UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_IS_GOOD))
		{
			if( AppIsHellgate() )
			{
				if ( UnitGetStat( unit, STATS_AI_SIGHT_RANGE ) )			
					UnitChangeStat( unit, STATS_AI_SIGHT_RANGE, ELITE_MONSTER_AWARENESS_INCREASE );
				
				UnitChangeStat( unit, STATS_VELOCITY, ELITE_MONSTER_SPEED_PERCENT );
			}
			else
			{
				if ( UnitGetStat( unit, STATS_AI_SIGHT_RANGE ) )			
					UnitChangeStat( unit, STATS_AI_SIGHT_RANGE, MYTHOS_ELITE_MONSTER_AWARENESS_INCREASE );

				UnitChangeStat( unit, STATS_VELOCITY, MYTHOS_ELITE_MONSTER_SPEED_PERCENT );
			}
		}
	}
	if (monster_data->fFuse > 0.0f)
	{
		int nTicks = (int)(monster_data->fFuse * GAME_TICKS_PER_SECOND_FLOAT);
		nTicks = MAX(1, nTicks);
		UnitRegisterEventTimed( unit, sHandleFuseMonsterEvent, &EVENT_DATA(), nTicks);
	}
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))
	{
		c_AnimationPlayByMode(unit, MODE_IDLE, 0.0f, PLAY_ANIMATION_FLAG_ONLY_ONCE | PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_DEFAULT_ANIM);
		UnitSetFlag(unit, UNITFLAG_DONT_PLAY_IDLE);

		if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_INFORM_QUESTS_ON_INIT))
		{
			c_QuestEventMonsterInit(unit);
		}

		if (UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_ASK_QUESTS_FOR_VISIBLE) && !UnitTestFlag( unit, UNITFLAG_CLIENT_ONLY ) )
		{
			c_QuestEventUnitVisibleTest(unit);
		}

		if(UnitIsA(unit, UNITTYPE_CRITTER))
		{
			int nPeriod = UnitGetStat(unit, STATS_AI_PERIOD);
			if (UnitGetStat(unit, STATS_AI) != 0)
			{
				WARNX(nPeriod, "AI period set to 0 - defaulting to 10");
				if (nPeriod == 0)
				{
					nPeriod = 10;
					UnitSetStat(unit, STATS_AI_PERIOD, nPeriod);
				}
				AI_Init(game, unit);
			}
		}
	}
#endif

	// init inventory
	UnitInventoryInit(unit);

	if ( sIsClientOnlyMonsterInit( game, monster_data, tSpec ) == TRUE )
	{
		c_WardrobeMakeClientOnly( unit );
	}


	return unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* s_MonsterSpawn(
	GAME *pGame,
	MONSTER_SPEC &tSpec)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )	
	ASSERT_RETNULL(IS_SERVER(pGame));
	ASSERTX_RETNULL( tSpec.pRoom, "Expected room for monster spawn" );
	
	VECTOR vPosition = tSpec.vPosition;
	VECTOR vFaceDirection = tSpec.vDirection;
	
	// init monster unit
	UNIT * pUnit = MonsterInit( pGame, tSpec );
	if (!pUnit)
	{
		return NULL;
	}

	UnitTriggerEvent( pGame, EVENT_CREATED, pUnit, NULL, NULL );

	// if there is an owner, the monster is the same target type as the owner
	UNIT *pOwner = tSpec.pOwner;
	if (pOwner)
	{
		UnitSetOwner(pUnit, pOwner);
		if( UnitGetTargetType( pOwner ) != TARGET_DEAD )
		{
			UnitChangeTargetType(pUnit, UnitGetTargetType(pOwner));		
		}
	}

	// get monster data
	const UNIT_DATA *pMonsterData = MonsterGetData( pGame, tSpec.nClass );
	
	// do starting treasure
	int nTreasureStart = pMonsterData->nStartingTreasure;
	if (nTreasureStart != INVALID_LINK)
	{
		BOOL bDoStartingTreasure = TRUE;
		
		if (UnitIsMerchant( pUnit ) && UnitMerchantSharesInventory( pUnit ) == FALSE)
		{
			bDoStartingTreasure = FALSE;
		}

		if (bDoStartingTreasure)
		{	
		
			// set spawn spec
			ITEM_SPEC tItemSpec;
			SETBIT(tItemSpec.dwFlags, ISF_STARTING_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_PICKUP_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_MONSTER_INVENTORY_BIT );
			if (UnitIsMerchant( pUnit ))
			{
				SETBIT( tItemSpec.dwFlags, ISF_AVAILABLE_AT_MERCHANT_ONLY_BIT );
			}
			tItemSpec.unitOperator = pUnit;
			
			s_TreasureSpawnClass(pUnit, nTreasureStart, &tItemSpec, NULL);
			
			// for merchants, save the time of the store refreshing
			if (UnitIsMerchant( pUnit ))
			{
				DWORD dwNow = GameGetTick( pGame );
				UnitSetStat( pUnit, STATS_MERCHANT_REFRESH_TICK, dwNow );
			}

		}
		
	}
	
	// for tradesmen, we want to fill their inventory with the results of their recipes,
	// so that we can view stats on them on clients.
	if( UnitIsTradesman( pUnit ) && UnitTradesmanSharesInventory( pUnit ))
	{
		ITEM_SPEC tItemSpec;
		SETBIT( tItemSpec.dwFlags, ISF_STARTING_BIT );
		SETBIT( tItemSpec.dwFlags, ISF_PICKUP_BIT );
		SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );
		tItemSpec.nQuality = 0;
		const UNIT_DATA* pUnitData = UnitGetData( pUnit );

		if( pUnitData->nRecipeList != INVALID_ID)
		{

			const RECIPELIST_DEFINITION *pRecipeList =  RecipeListGetDefinition( pUnitData->nRecipeList );
			ASSERTX( pRecipeList, "Missing Recipe List" );
			int Index = 0;
			// find the recipe
			for( int t = 0; t < MAX_RECIPES; t++ )
			{
				if( pRecipeList->nRecipes[ t ] == INVALID_ID )
				{
					break; //no more items to check against
				}
				const RECIPE_DEFINITION *pRecipe =  RecipeGetDefinition( pRecipeList->nRecipes[ t ] );			
				ASSERTX( pRecipe, "Missing Recipe" );

				tItemSpec.nItemClass = pRecipe->nRewardItemClass[ 0 ];
				tItemSpec.pSpawner = pUnit;
				UNIT * pItem = s_ItemSpawn( pGame, tItemSpec, NULL );
				s_ItemPickup( pUnit, pItem );

				Index++;
			}

		}
		
	}

	if( AppIsTugboat() && UnitIsTrainer( pUnit ) )
	{

		int nCount = (int)ExcelGetNumRows(pGame, DATATABLE_RECIPES);
		for( int t = 0; t < nCount; t++ )
		{
			const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( t );
			
			if( pRecipeDefinition )
			{
				if( pRecipeDefinition->nWeight > 0 &&
					RecipeCanSpawnAtMerchant( pRecipeDefinition ) &&
					( pMonsterData->nMinMerchantLevel <= pRecipeDefinition->nSpawnLevels[0] &&
					  pMonsterData->nMaxMerchantLevel >= pRecipeDefinition->nSpawnLevels[0] && 
				    ( pRecipeDefinition->nSpawnLevels[1] || pMonsterData->nMaxMerchantLevel <= pRecipeDefinition->nSpawnLevels[1] ) ) )
				{
					BOOL bAllowed = ( ( pMonsterData->nRecipeToSellByUnitType[0] == INVALID_ID ||
									    pMonsterData->nRecipeToSellByUnitType[0] == UNITTYPE_ANY ) && 
									   pMonsterData->nRecipeToSellByUnitTypeNot[0] == INVALID_ID );
					if( !bAllowed )
					{
						const UNIT_DATA *pUnitCreating = UnitGetData( pGame, GENUS_ITEM, pRecipeDefinition->nRewardItemClass[ 0 ] );
						if( pUnitCreating )
						{
							for( int nUnitTypesAllowed = 0; nUnitTypesAllowed < MAX_RECIPE_SELL_UNITTYPES; nUnitTypesAllowed++ )
							{
								if( pMonsterData->nRecipeToSellByUnitType[ nUnitTypesAllowed ] != INVALID_ID )
								{									
									if( pUnitCreating &&
										UnitIsA( pUnitCreating->nUnitType, pMonsterData->nRecipeToSellByUnitType[ nUnitTypesAllowed ] ) )
									{
										bAllowed = TRUE;
										break;
									}
								}
								else
								{
									break;
								}
							}
							if( bAllowed )
							{
								for( int nUnitTypesNotAllowed = 0; nUnitTypesNotAllowed < MAX_RECIPE_SELL_UNITTYPES; nUnitTypesNotAllowed++ )
								{
									if( pMonsterData->nRecipeToSellByUnitTypeNot[ nUnitTypesNotAllowed ] != INVALID_ID )
									{									
										if( pUnitCreating &&
											UnitIsA( pUnitCreating->nUnitType, pMonsterData->nRecipeToSellByUnitTypeNot[ nUnitTypesNotAllowed ] ) )
										{
											bAllowed = FALSE;
											break;
										}
									}
									else
									{
										break;
									}
								}
							}

						}
					}
					if( bAllowed )
					{
						if( pMonsterData->nRecipePane != INVALID_ID )
						{
							const RECIPE_CATEGORY *pRecipeCategory = RecipeGetCategoryDefinition( pRecipeDefinition->nRecipeCategoriesNecessaryToCraft[ 0 ] );
							ASSERT( pRecipeCategory );
							if( pRecipeCategory )
							{
								bAllowed = ( pMonsterData->nRecipePane == pRecipeCategory->nRecipePane );
							}
						}
					}
					if( bAllowed )
					{
						PlayerLearnRecipe( pUnit, t );					
					}
				}
			}
		}
	}

	GlobalThemeMonsterInit( pGame, pUnit );

	UnitClearFlag(pUnit, UNITFLAG_JUSTCREATED);
	UnitSetFlag(pUnit, UNITFLAG_GIVESLOOT);

	// TRAVIS: FIXME hack - clearing min/max damage for armed critters
	if( AppIsTugboat() && UnitIsArmed( pUnit ) &&
		UnitGetData(pUnit)->nDamageType > 0 )
	{
		
		if( UnitGetData(pUnit)->nDontUseWeaponDamage )
		{
			int nWardrobe = UnitGetWardrobe( pUnit );
			int nDamageType = UnitGetData(pUnit)->nDamageType;
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );

					if ( ! pWeapon || UnitIsA( pWeapon, UNITTYPE_SHIELD ) )
					{
						continue;
					}
					for (int ii = 0; ii < NUM_DAMAGE_TYPES; ii++)
					{
						if( UnitGetStat( pWeapon, STATS_DAMAGE_MAX, ii ) > 0)
						{
							nDamageType = ii;
							break;
						}
					}
					// OK, just wipe any monster weapon speed bonuses -
					// these should be taken care of by the monster, and the dummy
					// weapons on the clients will cause big desynch issues. Trust me.
					// hmm - will that be the case on players too? Seems like it would be....

					UnitSetStat( pWeapon, STATS_RANGED_SPEED, 0 );
					UnitSetStat( pWeapon, STATS_MELEE_SPEED, 0 );

					// clear out the weapon damage - we're not using it.
					UnitClearStatsRange(pWeapon, STATS_BASE_DMG_MIN );
					UnitClearStatsRange(pWeapon, STATS_BASE_DMG_MAX );
					UnitSetStatShift(pWeapon, STATS_DAMAGE_MIN, nDamageType, 0);
					UnitSetStatShift(pWeapon, STATS_DAMAGE_MAX, nDamageType, 0);
				}			
			}

			if (UnitGetData(pUnit)->nDamageType != nDamageType)
			{
				UnitSetStatShift(pUnit, STATS_DAMAGE_MIN, UnitGetData(pUnit)->nDamageType, 0);
				UnitSetStatShift(pUnit, STATS_DAMAGE_MAX, UnitGetData(pUnit)->nDamageType, 0);
				UnitSetStatShift(pUnit, STATS_DAMAGE_MIN, nDamageType, 100);
				UnitSetStatShift(pUnit, STATS_DAMAGE_MAX, nDamageType, 100);								

				
			}
		}
		else
		{
			UnitClearStatsRange(pUnit, STATS_BASE_DMG_MIN );
			UnitClearStatsRange(pUnit, STATS_BASE_DMG_MAX );
			UnitSetStatShift(pUnit, STATS_DAMAGE_MIN, UnitGetData(pUnit)->nDamageType, 0);
			UnitSetStatShift(pUnit, STATS_DAMAGE_MAX, UnitGetData(pUnit)->nDamageType, 0);
			UnitSetStatShift( pUnit, STATS_DAMAGE_PERCENT, DAMAGE_TYPE_SLASHING, ( UnitGetData(pUnit)->nWeaponDamageScale ) );
			UnitSetStatShift( pUnit, STATS_DAMAGE_PERCENT, DAMAGE_TYPE_PIERCING, ( UnitGetData(pUnit)->nWeaponDamageScale ) );
			UnitSetStatShift( pUnit, STATS_DAMAGE_PERCENT, DAMAGE_TYPE_CRUSHING, ( UnitGetData(pUnit)->nWeaponDamageScale ) );
			UnitSetStatShift( pUnit, STATS_DAMAGE_PERCENT, DAMAGE_TYPE_MISSILE, ( UnitGetData(pUnit)->nWeaponDamageScale ) );
		}

	}

	// activate ai
	AI_Init(pGame, pUnit);

	// network
	if (IS_SERVER( pGame ) && TEST_MASK(tSpec.dwFlags, MSF_KEEP_NETWORK_RESTRICTED) == FALSE)
	{
	
		// the monster itself is now ready
		ASSERTX( UnitIsInWorld( pUnit ), "Monster is not in world" );
		UnitSetCanSendMessages( pUnit, TRUE );
			
		// inform clients about new monster
		s_SendUnitToAllWithRoom( pGame, pUnit, UnitGetRoom(pUnit) );
		
	}
	
	if ( IS_SERVER( pGame ) )
		s_SkillsSelectAll( pGame, pUnit );

	for(int i=0; i<NUM_INIT_SKILLS; i++)
	{
		if (pMonsterData->nInitSkill[i] != INVALID_ID)
		{
			DWORD dwFlags = 0;
			if ( IS_SERVER( pGame ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

			SkillStartRequest(pGame, pUnit, pMonsterData->nInitSkill[i], INVALID_ID, VECTOR(0), dwFlags, SkillGetNextSkillSeed(pGame));
		}
	}

	if (UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_START_DEAD))
	{
		UnitDie( pUnit, NULL );
		// if dead and interactive, then set special interactive stat
		if ( IS_SERVER( pGame ) && UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_INTERACTIVE) )
		{
			UnitSetInteractive( pUnit, UNIT_INTERACTIVE_ENABLED );
		}
	}

	// in open worlds, we need to respawn monsters after they die - on a delay, basically
	// need to check whether this mob can in fact respawn though
	if( AppIsTugboat() )
	{
		LEVEL* pLevel = UnitGetLevel( pUnit );
		if( pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_OPENWORLD &&
			pMonsterData->nRespawnPeriod != 0 && 
			!UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_IS_GOOD) )
		{
			UnitSetFlag( pUnit, UNITFLAG_SHOULD_RESPAWN );
		}

		// Init the monster stats for the level	
		MYTHOS_LEVELAREAS::LevelAreaExecuteLevelAreaScriptOnMonster( pUnit );
#if !ISVERSION(SERVER_VERSION)
		if ( IS_CLIENT(pGame) )
		{
			c_UnitSetVisibilityOnClientByThemesAndGameEvents( pUnit );		
		}
#endif
	}
	
	UnitPostCreateDo(pGame, pUnit);

	return pUnit;
#else
	REF( pGame );
	REF( tSpec );
	return NULL;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * MonsterSpawnClient(
	GAME * game,
	int nClass,
	ROOM * room,
	const VECTOR & vPositionIn,
	const VECTOR & vFaceDirectionIn)
{
	ASSERT_RETNULL(IS_CLIENT(game));

	MONSTER_SPEC tSpec;
	tSpec.nClass = nClass;
	tSpec.nExperienceLevel = 0;
	tSpec.pRoom = room;
	tSpec.vPosition = vPositionIn;
	tSpec.vDirection = vFaceDirectionIn;
	
	UNIT*  unit = MonsterInit( game, tSpec );
	if (!unit)
	{
		return NULL;
	}

	AI_Init(game, unit);

	//SkillsSelectAll( game, unit );

	return unit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelMonstersPostProcess(
	EXCEL_TABLE * table)
{
	ExcelPostProcessUnitDataInit(table, TRUE);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterGetQuality(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_LINK, "Expected unit" );
	return UnitGetStat( pUnit, STATS_MONSTERS_QUALITY );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MonsterIsChampion(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// must be a monster
	if (!UnitIsA( pUnit, UNITTYPE_MONSTER ))
	{
		return FALSE;
	}

	// get quality
	int nMonsterQuality = UnitGetStat( pUnit, STATS_MONSTERS_QUALITY );
	return nMonsterQuality != INVALID_LINK;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterQualityPick(
	GAME *pGame,
	MONSTER_QUALITY_TYPE eType,
	RAND *rand /* = NULL */)
{
	PickerStart(pGame, picker);
	int nNumQualities = ExcelGetNumRows(pGame, DATATABLE_MONSTER_QUALITY);
	for (int i = 0; i < nNumQualities; ++i)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, i );
		
		// add to picker if it's of same (or allowed) type ... ahh paint fumes ...
		// there's probably a better way to do this, but I can't think of it today ;)
		if (eType == MQT_ANY ||  // <-- paint fume bug fix #1 ;)
			pMonsterQualityData->eType == eType ||
			pMonsterQualityData->eType == MQT_TOP_CHAMPION && eType == MQT_CHAMPION)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pMonsterQualityData->nRarity));
		}
		
	}
	if( rand ) 
	{
		return PickerChoose( pGame, *rand );
	}
	else
	{
		return PickerChoose( pGame );
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterProperNamePick(
	GAME *pGame,
	int nMonsterClass, /* = INVALID_LINK */
	RAND *rand /* = NULL */ )
{
	int nNumNames = MonsterNumProperNames();
	if(nMonsterClass >= 0)
	{
		const UNIT_DATA *pUnitData = UnitGetData(pGame, GENUS_MONSTER, nMonsterClass);
		// look for names that are tagged for this exact monster class first
		PickerStart(pGame, pickerExactMatch);
		for (int i = 0; i < nNumNames; ++i)
		{
			const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( i );

			if (pMonsterNameData->nMonsterNameType == pUnitData->nMonsterNameType)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
			}
		}

		// return exact match for monster class if found
		int nPick = 0;
		if( rand )
		{
			nPick = PickerChoose( pGame, *rand );
		}
		else
		{
			nPick = PickerChoose( pGame );
		}
		if (nPick >= 0) 
			return nPick;
	}

	PickerStart(pGame, pickerDefault);
	for (int i = 0; i < nNumNames; ++i)
	{
		const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( i );

		// first entry must be blank if this is a default name
		if (pMonsterNameData->nMonsterNameType == INVALID_LINK)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
		}
	}
	int nPick = 0;
	if( rand )
	{
		nPick = PickerChoose( pGame, *rand );
	}
	else
	{
		nPick = PickerChoose( pGame );
	}
	ASSERTX_RETVAL(nPick >= 0, 0, "couldn't find proper name appropriate for monster class");
	return nPick;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterNumProperNames(
	void)
{
	return ExcelGetNumRows( NULL, DATATABLE_MONSTER_NAMES );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterProperNameSet(
	UNIT *pMonster,
	int nNameIndex,
	BOOL bForce)
{
	ASSERTX_RETURN( 
		pMonster != NULL && 
		nNameIndex >= 0 && 
		nNameIndex < MonsterNumProperNames(), 
		"Invalid params for MonsterProperNameSet()" );

	// don't set names for monsters that don't allow random ones
	BOOL bSet = TRUE;	
	if (bForce == FALSE)
	{
	
		// check for unit datas that don't allow unique names
		const UNIT_DATA *pUnitData = UnitGetData( pMonster );
		if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NO_RANDOM_PROPER_NAME))
		{			
			bSet = FALSE;
		}

		// check for monster qualities that don't allow unique names			
		int nMonsterQuality = MonsterGetQuality( pMonster );
		if (nMonsterQuality != INVALID_LINK)
		{
			const MONSTER_QUALITY_DATA *pMonsterQuality = MonsterQualityGetData( NULL, nMonsterQuality );
			if (pMonsterQuality->bPickProperName == FALSE)
			{
				bSet = FALSE;
			}
		}
							
	}

	// set name
	if (bSet == TRUE)
	{
		const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( nNameIndex );
		if (pMonsterNameData->bIsNameOverride)
			UnitSetNameOverride( pMonster, pMonsterNameData->nStringKey );
		else
			UnitSetStat( pMonster, STATS_MONSTER_UNIQUE_NAME, nNameIndex );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterUniqueNameGet(
	UNIT *pMonster)
{
	ASSERTX_RETINVALID( pMonster != NULL, "Expected monster" );
	return UnitGetStat( pMonster, STATS_MONSTER_UNIQUE_NAME );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR *sMonsterUniqueNameGetString(
	int nNameIndex)
{
	const MONSTER_NAME_DATA *pMonsterNameData = MonsterGetNameData( nNameIndex );
	return StringTableGetStringByIndex( pMonsterNameData->nStringKey );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterGetUniqueNameWithTitle(
	int nNameIndex,
	int nMonsterClass,
	WCHAR *puszNameWithTitle,
	DWORD dwNameWithTitleSize,
	DWORD *pdwAttributesOfString /*=NULL*/)
{
	const char *pszTitleFormatKey = GlobalStringGetKey( GS_MONSTER_CLASS_TITLE );
		
	// get class and replace [class] token
	const WCHAR *puszTitle = L"";
	const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nMonsterClass );
	DWORD dwMonsterClassAttributes = 0;	
	ASSERT( pMonsterData );
	if (pMonsterData)
	{
		puszTitle = StringTableGetStringByIndexVerbose( 
			pMonsterData->nString, 
			0, 
			0, 
			&dwMonsterClassAttributes,
			NULL);
		ASSERT( puszTitle );
	}

	// get the class title format string (note that we're matching as many of the attributes
	// of the monster class string as we can, this is necessary for Hungarian for instance because
	// there are two class title strings to use
	// [monster] a [class]
	// [monster] az [class]
	// and which one we use depends on attributes in the monster class string
	const WCHAR *puszTitleFormat = StringTableGetStringByKeyVerbose( 
		pszTitleFormatKey, 
		dwMonsterClassAttributes, 
		0, 
		NULL, 
		NULL);

	if (pdwAttributesOfString)
	{
		*pdwAttributesOfString = dwMonsterClassAttributes;
	}
	
	// copy format to name string	
	PStrCopy( puszNameWithTitle, puszTitleFormat, dwNameWithTitleSize );

	// get name of unit and replace [monster] token
	const WCHAR *puszTargetName = sMonsterUniqueNameGetString( nNameIndex );
	PStrReplaceToken( puszNameWithTitle, dwNameWithTitleSize, StringReplacementTokensGet( SR_MONSTER ), puszTargetName );
	
	// replace the [class] token
	PStrReplaceToken( puszNameWithTitle, dwNameWithTitleSize, StringReplacementTokensGet( SR_CLASS ), puszTitle );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterStaggerSpawnPosition(
	GAME *pGame,
	VECTOR *pPosition,
	const VECTOR *pPositionCenter,
	int nMonsterClass,
	int nNumSpawnedSoFar)
{				
	ASSERT_RETURN( pPosition && pPositionCenter );
	const UNIT_DATA *pMonsterData = MonsterGetData( pGame, nMonsterClass );
	float flRadius = UnitGetMaxCollisionRadius( pGame, GENUS_MONSTER, nMonsterClass );
	float flMaxHeight = UnitGetMaxCollisionHeight( pGame, GENUS_MONSTER, nMonsterClass );
	*pPosition= *pPositionCenter;
	float fXOffset = nNumSpawnedSoFar ? (nNumSpawnedSoFar / 2 + 1) * flRadius * 2.0f : 0.0f;
	if ( nNumSpawnedSoFar % 2 )
	{
		fXOffset = - fXOffset;
	}
	pPosition->fX += fXOffset; 
	if ( UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_SPAWN_IN_AIR) )
	{
		pPosition->fZ += flMaxHeight * 2.0f;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterChampionEvaluateMinionPackSize(
	GAME *pGame, 
	const int nMonsterQuality)
{
	ASSERTX_RETZERO( nMonsterQuality != INVALID_LINK, "Invalid monster quality" );
	
	// get monster quality data
	const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );

	int nCodeLength = 0;
	BYTE *pCode = ExcelGetScriptCode( pGame, DATATABLE_MONSTER_QUALITY, pMonsterQualityData->codeMinionPackSize, &nCodeLength );
	if (pCode)
	{
		return VMExecI( pGame, pCode, nCodeLength );
	}
	
	return 0;
				
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterEvaluateMinionPackSize(
	GAME *pGame, 
	const UNIT_DATA *pUnitData)
{

	int nCodeLength = 0;
	BYTE *pCode = ExcelGetScriptCode( pGame, DATATABLE_MONSTERS, pUnitData->codeMinionPackSize, &nCodeLength );
	if (pCode)
	{
		return VMExecI( pGame, pCode, nCodeLength );
	}
	
	return 0;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterChampionMinionGetQuality( 
	GAME *pGame, 
	int nMonsterQuality)
{
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );
		if( pMonsterQualityData->nMinionQuality != INVALID_ID )
		{
			return pMonsterQualityData->nMinionQuality;
		}
	}
	// we'll say that minions are always one below in quality for now
	if (nMonsterQuality >= 1)
	{
		return nMonsterQuality - 1;
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float MonsterGetMoneyChanceMultiplier( 
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pUnit, UNITTYPE_MONSTER ), 1.0f, "Expected monster" );
	
	GAME *pGame = UnitGetGame( pUnit );
	float flMultiplier = 1.0f;
	
	// do champion quality multiplier	
	int nMonsterQuality = UnitGetStat( pUnit, STATS_MONSTERS_QUALITY );
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );
		flMultiplier = pMonsterQualityData->flMoneyChanceMultiplier;
	}

	return flMultiplier;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float MonsterGetMoneyAmountMultiplier( 
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pUnit, UNITTYPE_MONSTER ), 1.0f, "Expected monster" );
	
	GAME *pGame = UnitGetGame( pUnit );
	float flMultiplier = 1.0f;
	
	// do champion quality multiplier	
	int nMonsterQuality = UnitGetStat( pUnit, STATS_MONSTERS_QUALITY );
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );
		flMultiplier = pMonsterQualityData->flMoneyAmountMultiplier;
	}

	flMultiplier *= float(UnitGetStat(pUnit, STATS_LOOT_BONUS_PERCENT)+100)/100.0f;

	return flMultiplier;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterGetTreasureLevelBoost(
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	ASSERTX_RETZERO( UnitIsA( pUnit, UNITTYPE_MONSTER ), "Expected monster" );
	
	GAME *pGame = UnitGetGame( pUnit );
	int nTreasureLevelBoost = 0;
	
	// do champion quality multiplier	
	int nMonsterQuality = UnitGetStat( pUnit, STATS_MONSTERS_QUALITY );
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nMonsterQuality );
		nTreasureLevelBoost += pMonsterQualityData->nTreasureLevelBoost;
	}

	return nTreasureLevelBoost;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MonsterGetAffixes( 
	UNIT *pUnit, 
	int *pnAffix, 
	int nMaxAffix)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	ASSERTX_RETZERO( pnAffix, "Expected affix storage" );
	ASSERTX_RETZERO( nMaxAffix > 0, "Expected affix storage" );
	int nNumAffixCopied = 0;
	
	STATS_ENTRY tStatsEntries[ MAX_MONSTER_AFFIX ];
	int nAffixCount = UnitGetStatsRange( pUnit, STATS_APPLIED_AFFIX, 0, tStatsEntries, MAX_MONSTER_AFFIX );
	for (int i = 0; i < nAffixCount; ++i)
	{
		int nAffix = tStatsEntries[ i ].value;
		
		// save in array if there is room
		if (nNumAffixCopied < nMaxAffix - 1)
		{
			pnAffix[ nNumAffixCopied++ ] = nAffix;
		}
				
	}
	
	return nNumAffixCopied;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MONSTER_NAME_DATA	*MonsterGetNameData(
	int nIndex)
{
	return (const MONSTER_NAME_DATA *)ExcelGetData( NULL, DATATABLE_MONSTER_NAMES, nIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MONSTER_QUALITY_TYPE MonsterQualityGetType(
	int nMonsterQuality)
{
	if (nMonsterQuality == INVALID_LINK)
	{
		return MQT_ANY;
	}
	const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( NULL, nMonsterQuality );
	return pMonsterQualityData->eType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MonsterQualityShowsLabel( 
	int nMonsterQuality)
{
	if (nMonsterQuality == INVALID_LINK)
	{
		return FALSE;
	}
	const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( NULL, nMonsterQuality );
	return pMonsterQualityData->bShowLabel;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MonsterCanSpawn(
	int nMonsterClass,
	LEVEL *pLevel,
	int nMonsterExperienceLevel /*=-1*/)
{
	ASSERTX_RETFALSE( nMonsterClass != INVALID_LINK, "Expected monster class" );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// get unit data
	const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nMonsterClass );
	
	// check spawn flag
	if (UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_SPAWN ) == FALSE)
	{
		return FALSE;
	}
	
	// do not allow monsters who's level is too high
	if( nMonsterExperienceLevel == -1 )
	{
		nMonsterExperienceLevel = LevelGetMonsterExperienceLevel( pLevel );
	}

	if (pMonsterData->nMinMonsterExperienceLevel > 0 && 
		pMonsterData->nMinMonsterExperienceLevel > nMonsterExperienceLevel)
	{
		return FALSE;
	}
	
	
	if(AppIsTugboat())
	{		
		// this calculation needs to happen in data, not here!
		int nLevelDepth = nMonsterExperienceLevel == -1 ? LevelGetMonsterExperienceLevel( pLevel ) : nMonsterExperienceLevel;
		
		if( nLevelDepth < 0 )
		{
			nLevelDepth = 0;
		}
		
		if( nLevelDepth == 0 )
		{
			nLevelDepth = 1;
		}


		if (pMonsterData->nMinSpawnLevel > 0 &&
			pMonsterData->nMinSpawnLevel > nLevelDepth) 
		{
			return FALSE;
		}
		if (pMonsterData->nMaxSpawnLevel > 0 &&
			pMonsterData->nMaxSpawnLevel < nLevelDepth)
		{
			return FALSE;
		}

	}

	return TRUE;  // OK to spawn	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMonsterInBuffer( 
	int nMonsterClass, 
	int* pnMonsterClassBuffer, 
	int nBufferCount)
{
	for (int i = 0; i < nBufferCount; ++i)
	{
		if (pnMonsterClassBuffer[ i ] == nMonsterClass)
		{
			return TRUE;
		}
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MonsterAddPossible(
	LEVEL *pLevel,
	int nSpawnClass,
	int nMonsterClass,
	int* pnBuffer,
	int nBufferSize,
	int* pnBufferCount)
{	
	
	if (nMonsterClass != INVALID_LINK)
	{
	
		// only add if spawn class system can not really make monster
		if (pLevel == NULL || MonsterCanSpawn( nMonsterClass, pLevel ))
		{
		
			// only add if not already in buffer			
			if (sMonsterInBuffer( nMonsterClass, pnBuffer, *pnBufferCount) == FALSE)
			{
				// monster is not in buffer of possible monsters
				ASSERTV_RETURN( 
					*pnBufferCount <= nBufferSize - 1, 
					"Buffer size (%d) for possible monsters is too small",
					nBufferSize);
				pnBuffer[ *pnBufferCount ] = nMonsterClass;
				*pnBufferCount= (*pnBufferCount) + 1;
				
				// add some links in the monster data itself
				const UNIT_DATA *pUnitData = MonsterGetData( NULL, nMonsterClass );
				if (pUnitData)
				{
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nMonsterClassAtUniqueQuality, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nMonsterClassAtChampionQuality, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );						
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nMinionClass, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );						
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nSpawnMonsterClass, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );						
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nMonsterToSpawn, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );						
					MonsterAddPossible( 
						pLevel, 
						nSpawnClass, 
						pUnitData->nMonsterDecoy, 
						pnBuffer, 
						nBufferSize, 
						pnBufferCount );						

				}
				
			}

		}

	}
	
}				

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MonsterVersion(
	UNIT *pMonster)
{
	ASSERTX_RETURN( pMonster, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pMonster, UNITTYPE_MONSTER ), "Expected monster" );
	
	// bring monster (probably a pet in a players inventory) into the current balanced
	// game universe ... it might be an old monster with wildly unbalanced stats that
	// we want to fix up here
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_MonsterFirstKill(
	UNIT * pKiller,
	UNIT * pVictim )
{
	ASSERTX_RETURN( AppIsHellgate(), "Hellgate only" );
	ASSERTX_RETURN( pVictim, "Invalid victim" );
	ASSERTX_RETURN( UnitGetGenus( pVictim ) == GENUS_MONSTER, "Victim is not a monster" );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pVictim ) ), "Server only" );
	if ( !pKiller || !UnitTestFlag( pVictim, UNITFLAG_GIVESLOOT ))
	{
		return;
	}

	// first find out if we need to bother
	const UNIT_DATA * pMonsterData = UnitGetData( pVictim );
	if ( pMonsterData->nTreasureFirstTime == INVALID_LINK )
	{
		return;
	}

	// find out who the player is...
	UNIT *pPlayer = NULL;
	GAME *pGame = UnitGetGame( pVictim );

	// was it a destructible?
	if ( UnitDataTestFlag( UnitGetData( pKiller ), UNIT_DATA_FLAG_DESTRUCTIBLE ) )
	{
		UNITID idAttackerUltimate = UnitGetStat( pKiller, STATS_LAST_ULTIMATE_ATTACKER_ID );
		if ( idAttackerUltimate != INVALID_ID )
		{
			pKiller = UnitGetById( pGame, idAttackerUltimate );
			if (!pKiller)
			{
				return;
			}
		}
	}

	// was it a player?
	if ( UnitIsA( pKiller, UNITTYPE_PLAYER ) )
	{
		pPlayer = pKiller;
	}
	// was it a pet?
	else if ( PetIsPlayerPet( pGame, pKiller ) )
	{
		UNITID idOwner = PetGetOwner( pKiller );
		ASSERTX_RETURN( idOwner != INVALID_ID, "unexpected invalid owner ID for player pet");
		pPlayer = UnitGetById( pGame, idOwner );
	}

	// couldn't figure it out...
	if ( pPlayer == NULL )
	{
		// maybe some sort of trace/error message here
		return;
	}

	PARTYID idParty = INVALID_ID;
	if ( AppIsMultiplayer() )
	{
		idParty = UnitGetPartyId( pPlayer );
	}

	// no multiplayer/party?
	if ( idParty == INVALID_ID )
	{
		// check if the player has killed this monster before in this difficulty
		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
		int nMonsterClass = UnitGetClass( pVictim );
		if ( UnitGetStat( pPlayer, STATS_MONSTER_FIRST_KILL, nMonsterClass, nDifficulty ) == 0 )
		{
			// spew out the first-time treasure

			// setup item spec
			ITEM_SPEC tSpec;
			SETBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
			SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
			tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
			tSpec.unitOperator = pPlayer;  // the operator for *this* instance drop is *this* player
			// do spawn
			s_TreasureSpawn( pVictim, pMonsterData->nTreasureFirstTime, &tSpec, NULL );

			// save as killed...
			UnitSetStat( pPlayer, STATS_MONSTER_FIRST_KILL, nMonsterClass, nDifficulty, 1 );
		}
	}
	else
	{
		for ( GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			pClient;
			pClient = ClientGetNextInGame( pClient ))
		{
			UNIT * pUnit = ClientGetControlUnit( pClient );
			ASSERTX_RETURN( pUnit != NULL, "Unexpected NULL Control Unit from Client" );
			// must be in the same level and party
			if ( UnitGetPartyId( pUnit ) == idParty &&
				UnitsAreInSameLevel( pUnit, pPlayer ))
			{
				// check if the player has killed this monster before in this difficulty
				int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
				int nMonsterClass = UnitGetClass( pVictim );
				if ( UnitGetStat( pUnit, STATS_MONSTER_FIRST_KILL, nMonsterClass, nDifficulty ) == 0 )
				{
					// spew out the first-time treasure

					// setup item spec
					ITEM_SPEC tSpec;
					SETBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
					SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
					tSpec.guidRestrictedTo = UnitGetGuid( pUnit );
					tSpec.unitOperator = pUnit;  // the operator for *this* instance drop is *this* player
					// do spawn
					s_TreasureSpawn( pVictim, pMonsterData->nTreasureFirstTime, &tSpec, NULL );

					// save as killed...
					UnitSetStat( pUnit, STATS_MONSTER_FIRST_KILL, nMonsterClass, nDifficulty, 1 );
				}
			}
		}
	}
}
#endif
