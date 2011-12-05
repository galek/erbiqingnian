//----------------------------------------------------------------------------
// combat.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _COMBAT_H_
#define _COMBAT_H_


#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef _GAMEGLOBALS_H_
#include "gameglobals.h"
#endif


#ifndef _DATATABLES_H_
#include "datatables.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ROOM;
struct HAVOK_IMPACT;
struct STATS;
struct MONSTER_SCALING;


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
#define DAMAGE_INCREMENT_FULL			(100)
#define DAMAGE_INCREMENT_FULL_FLOAT		(100.0f)
#define MAX_KARMA_PENALTY					5600
#define KARMA_PER_ITEM						400

//----------------------------------------------------------------------------
enum COMBAT_RESULT
{
	COMBAT_RESULT_HIT_TINY = 0,
	COMBAT_RESULT_HIT_LIGHT,
	COMBAT_RESULT_HIT_MEDIUM,
	COMBAT_RESULT_HIT_HARD,
	COMBAT_RESULT_KILL,
	COMBAT_RESULT_BLOCK,
	COMBAT_RESULT_FUMBLE,
	NUM_COMBAT_RESULTS
};

//----------------------------------------------------------------------------
enum DELIVERY_TYPE
{
	COMBAT_LOCAL_DELIVERY,
	COMBAT_SPLASH_DELIVERY,
};

//----------------------------------------------------------------------------
enum COMBAT_TRACE_FLAGS
{
	COMBAT_TRACE_INVALID,
	COMBAT_TRACE_DAMAGE,
	COMBAT_TRACE_SFX,
	COMBAT_TRACE_KILLS,
	COMBAT_TRACE_EVASION,
	COMBAT_TRACE_BLOCKING,
	COMBAT_TRACE_SHIELDS,
	COMBAT_TRACE_ARMOR,
	COMBAT_TRACE_INTERRUPT,
	
	NUM_COMBAT_TRACE,
	COMBAT_TRACE_DEBUGUNIT_ONLY,
};

//----------------------------------------------------------------------------
enum COMBAT_UNIT_ATTACK_UNIT_FLAGS
{
	UAU_MELEE_BIT,
	UAU_RADIAL_ONLY_ATTACKS_DEFENDER_BIT,
	UAU_APPLY_DUALWEAPON_MELEE_SCALING_BIT,
	UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT,
	UAU_IS_THORNS_BIT,
	UAU_DIRECT_MISSILE_BIT,
	UAU_SHIELD_DAMAGE_ONLY_BIT,
	UAU_NO_DAMAGE_EFFECTS_BIT,
	UAU_IGNORE_SHIELDS_BIT,
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct DAMAGE_TYPE_DATA
{
	char	szDamageType[DEFAULT_INDEX_SIZE];
	WORD	wCode;
	char	szMiniIcon[DEFAULT_INDEX_SIZE];
	char	szDisplayString[DEFAULT_INDEX_SIZE];
	int		nColor;
	int		nShieldHitState;
	int		nCriticalState;
	int		nSoftHitState;
	int		nMediumHitState;
	int		nBigHitState;
	int		nFumbleHitState;
	int		nDamageOverTimeState;
	int		nFieldMissile;
	int		nInvulnerableState;
	int		nInvulnerableSFXState;
	int		nThornsState;
	int		nVulnerabilityInPVPTugboat;
	int		nVulnerabilityInPVPHellgate;
	float	fSFXInvulnerabilityDurationMultInPVPTugboat;
	float	fSFXInvulnerabilityDurationMultInPVPHellgate;
};


struct DAMAGE_EFFECT_DATA
{
	char	szDamageEffect[DEFAULT_INDEX_SIZE];
	WORD	wCode;
	int		nDamageTypeID;
	PCODE	codeSfxDuration;
	PCODE	codeBaseSfxEffect;
	PCODE	codeConditional;
	PCODE	codeMissileStats;
	int		nInvulnerableState;
	int		nAttackerProhibitingState;
	int		nDefenderProhibitingState;
	int		nAttackerRequiresState;	
	int		nDefenderRequiresState;	
	int		nSfxState;
	int		nMissileToAttach;
	int		nFieldMissile;
	int		nAttackerSkill;
	int		nTargetSkill;
	BOOL	bUseParentsRoll;
	BOOL	bNoRollNeeded;
	BOOL	bMustBeCrit;
	BOOL	bMonsterMustDie;
	BOOL	bRequiresNoDamage;
	BOOL	bDoesntRequireDamage;
	BOOL	bDontUseUltimateAttacker;
	BOOL	bDoesntUseSFXDefense;
	BOOL	bUsesOverrideStats;
	BOOL	bDontUseEffectChance;
	int		nPlayerVsMonsterScalingIndex;
	int		nAttackStat;
	int		nAttackLocalStat;
	int		nAttackSplashStat;
	int		nAttackPctStat;
	int		nAttackPctLocalStat;
	int		nAttackPctSplashStat;
	int		nAttackPctCasteStat;
	int		nDefenseStat;
	int		nEffectDefenseStat;
	int		nEffectDefensePctStat;
	int		nDefaultDuration;
	int		nDefaultStrength;
};


struct LEVEL_SCALING_DATA
{
	int		nLevelDiff;
	int		nPlayerAttacksMonsterDmg;
	int		nPlayerAttacksMonsterExp;
	int		nMonsterAttacksPlayerDmg;
	int		nPlayerAttacksPlayerDmg;
	int		nPlayerAttacksPlayerKarma;
	int		nPlayerAttacksMonsterKarma;
	int		nPlayerAttacksMonsterDmgEffect;
	int		nPlayerAttacksPlayerDmgEffect;
	int		nMonsterAttacksPlayerDmgEffect;
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL LevelScalingExcelPostProcess(
	struct EXCEL_TABLE * table);

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_UnitAttackUnit(
	UNIT * attacker,
	UNIT * defender,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int attack_index,
#ifdef HAVOK_ENABLED
	const HAVOK_IMPACT * pImpact,
#endif
	DWORD dwFlags = 0,
	float fDamageMultiplier = -1.0f,
	int nDamageTypeOverride = -1,
	int minDamageOverride = -1,
	int maxDamageOverride = -1,
	UNIT * unitCausingAttack = NULL);

void s_UnitAttackLocation(
	UNIT * attacker,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int attack_index,
	ROOM * room,
	VECTOR location,
#ifdef HAVOK_ENABLED
	const HAVOK_IMPACT & Impact,
#endif
	DWORD dwFlags);

void s_UnitKillUnit(
	GAME * game,
	const struct MONSTER_SCALING * monscaling,
	UNIT * attacker,
	UNIT * defender,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	UNIT * attacker_ultimate_source = NULL,
	UNIT * defender_ultimate_source = NULL);

void s_CombatExplicitSpecialEffect(
	UNIT *defender,
	UNIT *attacker,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int damageAmount,
	int damageType);

int s_CombatGetDamageIncrement(
	const struct COMBAT * pCombat);

BOOL s_UnitCanAttackUnit(
	const UNIT * pAttacker,
	const UNIT * pDefender);

#if ISVERSION(CHEATS_ENABLED)
int s_CombatSetCriticalProbabilityOverride(
	int nProbability);
#endif

#if ISVERSION(DEVELOPMENT)
void DumpCombatHistory(
	GAME * game,
	struct GAMECLIENT * client);

BOOL GameToggleCombatTrace(
	GAME * game,
	DWORD flag);

void GameSetCombatTrace(
	GAME * game,
	DWORD flag,
	BOOL val = TRUE);

BOOL GameGetCombatTrace(
	GAME * game,
	DWORD flag);
#endif // ISVERSION(DEVELOPMENT)

#endif // !ISVERSION(CLIENT_ONLY_VERSION)

void UnitTransferDamages(
	UNIT * target,
	UNIT * source,
	UNIT * weapons[ MAX_WEAPONS_PER_UNIT ],
	int attack_index,
	BOOL bRiders=TRUE,
	BOOL bNonWeapon = FALSE);

const DAMAGE_TYPE_DATA* DamageTypeGetData(
	GAME * game,
	int damage_type);

const DAMAGE_EFFECT_DATA * DamageEffectGetData(
	GAME * game,
	int effect_type);

int GetNumDamageTypes(
	GAME * game);




#if ISVERSION(SERVER_VERSION)
#define UnitDie(u, k)		UnitDieSvrdbg(u, k, __FILE__, __LINE__)
void UnitDieSvrdbg(
	UNIT * unit,
	UNIT * pKiller,
	const char * file,
	unsigned int line);
#else
void UnitDie(
	UNIT * unit,
	UNIT * pKiller );
#endif

int CombatGetMinDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee);

int CombatGetMaxDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee);

int CombatGetBaseMinDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee);

int CombatGetBaseMaxDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee);

int CombatGetBonusDamagePCT( UNIT *unit, int dmg_type, BOOL bIsMelee );
BOOL CombatGetWeaponMinMaxBaseDamage( UNIT *pWeapon, int *min_damage, int *max_damage );

int CombatGetBonusDamage(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int dmg_type,
	int delivery_type,
	BOOL bIsMelee);

int CombatGetMinThornsDamage(
	GAME * game,
	UNIT * unit,
	STATS * stats,
	int dmg_type );

int CombatGetMaxThornsDamage(
	GAME * game,
	UNIT * unit,
	STATS * stats,
	int dmg_type );

int CombatCalcDamageIncrementByEnergy(
	UNIT * pWeapon );

#if !ISVERSION(SERVER_VERSION)
void c_CombatSystemFlagSoundsForLoad(
	GAME * game);
#endif // !ISVERSION(SERVER_VERSION)

UNIT * UnitGetResponsibleUnit(
	UNIT * pUnit);

float CombatModifyChanceByIncrement(
	float chance,
	float increment);

BOOL s_CombatHasSecondaryAttacks(
	struct COMBAT * pCombat);

BOOL s_CombatIsPrimaryAttack(
	struct COMBAT * pCombat);

struct COMBAT * CombatGetNext(struct COMBAT * pCurrent);
void CombatSetNext(struct COMBAT * pCurrent, struct COMBAT * pNext);

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void CombatGetDamagePositionFromVector(
	const VECTOR & vPos,
	int & nX,
	int & nY)
{
	nX = (int)FLOOR((vPos.fX * 1000.0f) + 0.5f);
	nY = (int)FLOOR((vPos.fY * 1000.0f) + 0.5f);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const LEVEL_SCALING_DATA * CombatGetLevelScalingData(
	GAME * game,
	int attacker_level,
	int defender_level)
{
	static const LEVEL_SCALING_DATA default_data = {0, 0, 0, 10000, 0};

	if (defender_level <= 0)
	{
		return &default_data;
	}

	ASSERT_RETVAL(attacker_level >= 0 && attacker_level <= ABSOLUTE_LEVEL_MAX, &default_data);
	ASSERT_RETVAL(defender_level >= 0 && defender_level <= ABSOLUTE_LEVEL_MAX, &default_data);
	int level_diff = attacker_level - defender_level;
	
	const int * min_level_scaling_diff = (const int *)ExcelGetDataEx(game, DATATABLE_LEVEL_SCALING, 0);
	ASSERT_RETVAL(min_level_scaling_diff, &default_data);
	ASSERT_RETVAL(level_diff >= *min_level_scaling_diff, &default_data);
	unsigned int row = level_diff + -(*min_level_scaling_diff);
	const LEVEL_SCALING_DATA * level_scaling_data = (const LEVEL_SCALING_DATA *)ExcelGetData(game, DATATABLE_LEVEL_SCALING, row);
	ASSERT_RETVAL(level_scaling_data, &default_data);
	ASSERT_RETVAL(level_scaling_data->nLevelDiff == level_diff, &default_data);
	return level_scaling_data;
}


#endif // _COMBAT_H_