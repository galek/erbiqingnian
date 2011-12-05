//----------------------------------------------------------------------------
//	skills.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _SKILLS_H_
#define _SKILLS_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#include "excel.h"
#endif

#include "c_attachment.h"

#ifndef _DEFINITION_COMMON_H_
#include "definition.h"
#endif

#ifndef _TARGET_H_
#include "target.h"
#endif

#ifndef _UNITTYPES_H_
#include "unittypes.h"
#endif

#ifndef _CONDITION_H_
#include "condition.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif

enum
{
	SKILL_STARTFUNCTION_NULL = 0,
	SKILL_STARTFUNCTION_GENERIC,
	SKILL_STARTFUNCTION_NOMODES,
	SKILL_STARTFUNCTION_WARMUP,

	SKILL_STARTFUNCTION_COUNT,
};

enum SKILL_VARIABLES
{
	SKILL_VARIABLE_ONE,
	SKILL_VARIABLE_TWO,
	SKILL_VARIABLE_THREE,
	SKILL_VARIABLE_FOUR,
	SKILL_VARIABLE_FIVE,
	SKILL_VARIABLE_SIX,
	SKILL_VARIABLE_SEVEN,
	SKILL_VARIABLE_EIGHT,
	SKILL_VARIABLE_NINE,
	SKILL_VARIABLE_TEN,
	SKILL_VARIABLE_COUNT
};

enum 
{
	SKILL_FLAG_LOADED = 0,		// are all of the assets loaded for the skill - particles, sounds, etc..
	SKILL_FLAG_INITIALIZED,		// has the skills flags and events been loaded and processed?
	SKILL_FLAG_USES_WEAPON,
	SKILL_FLAG_WEAPON_IS_REQUIRED,
	SKILL_FLAG_USES_WEAPON_TARGETING,
	SKILL_FLAG_USES_WEAPON_SKILL,
	SKILL_FLAG_USES_WEAPON_COOLDOWN,
	SKILL_FLAG_COOLDOWN_UNIT_INSTEAD_OF_WEAPON,
	SKILL_FLAG_COOLDOWN_TARGET_INSTEAD_OF_UNIT,
	SKILL_FLAG_USES_WEAPON_ICON,
	SKILL_FLAG_USE_ALL_WEAPONS,
	SKILL_FLAG_COMBINE_WEAPON_DAMAGE,
	SKILL_FLAG_USES_UNIT_FIRING_ERROR,
	SKILL_FLAG_DISPLAY_FIRING_ERROR,
	SKILL_FLAG_USES_HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD,
	SKILL_FLAG_CHECK_LOS,
	SKILL_FLAG_NO_LOW_AIMING_THIRDPERSON,
	SKILL_FLAG_NO_HIGH_AIMING_THIRDPERSON,
	SKILL_FLAG_CAN_TARGET_UNIT,
	SKILL_FLAG_FIND_TARGET_UNIT,
	SKILL_FLAG_MUST_TARGET_UNIT,	
	SKILL_FLAG_MUST_NOT_TARGET_UNIT,
	SKILL_FLAG_MONSTER_MUST_TARGET_UNIT,
	SKILL_FLAG_CANNOT_RETARGET,
	SKILL_FLAG_VERIFY_TARGET,
	SKILL_FLAG_VERIFY_TARGETS_ON_REQUEST,
	SKILL_FLAG_TARGETS_POSITION,
	SKILL_FLAG_KEEP_TARGET_POSITION_ON_REQUEST,
	SKILL_FLAG_TARGET_POS_IN_STAT,
	SKILL_FLAG_TARGET_DEAD,
	SKILL_FLAG_TARGET_DYING_ON_START,
	SKILL_FLAG_TARGET_DYING_AFTER_START,
	SKILL_FLAG_TARGET_SELECTABLE_DEAD,
	SKILL_FLAG_TARGET_FRIEND,
	SKILL_FLAG_TARGET_ENEMY,
	SKILL_FLAG_TARGET_CONTAINER,
	SKILL_FLAG_TARGET_DESTRUCTABLES,
	SKILL_FLAG_TARGET_ONLY_DYING,
	SKILL_FLAG_DONT_TARGET_DESTRUCTABLES,
	SKILL_FLAG_TARGET_PETS,
	SKILL_FLAG_IGNORE_TEAM,
	SKILL_FLAG_ALLOW_UNTARGETABLES,
	SKILL_FLAG_ALLOW_OBJECTS,
	SKILL_FLAG_UI_USES_TARGET,
	SKILL_FLAG_DONT_FACE_TARGET,
	SKILL_FLAG_MUST_FACE_FORWARD,
	SKILL_FLAG_AIM_AT_TARGET,
	SKILL_FLAG_USES_TARGET_INDEX,
	SKILL_FLAG_USE_MOUSE_SKILLS_TARGETING,
	SKILL_FLAG_IS_MELEE,
	SKILL_FLAG_DO_MELEE_ITEM_EVENTS,
	SKILL_FLAG_DELAY_MELEE,
	SKILL_FLAG_DEAD_CAN_DO,
	SKILL_FLAG_USE_STOP_ALL,
	SKILL_FLAG_STOP_ON_DEAD,
	SKILL_FLAG_START_ON_SELECT,
	SKILL_FLAG_ALWAYS_SELECT,
	SKILL_FLAG_CAN_BE_SELECTED,
	SKILL_FLAG_HIGHLIGHT_WHEN_SELECTED,
	SKILL_FLAG_REPEAT_FIRE,	// do the skill over and over again - used by fire bolter
	SKILL_FLAG_REPEAT_ALL,	// do the skill over and over again - used by fire bolter
	SKILL_FLAG_HOLD,		// start the skill, do the mode once, the just hold the mode as active - used for HARP
	SKILL_FLAG_LOOP_MODE,	// just loop the mode the first time that you start - used by zombie eating
	SKILL_FLAG_STOP_HOLDING_SKILLS,
	SKILL_FLAG_HOLD_OTHER_SKILLS,
	SKILL_FLAG_PREVENT_OTHER_SKILLS,
	SKILL_FLAG_PREVENT_OTHER_SKILLS_BY_PRIORITY,
	SKILL_FLAG_RUN_TO_RANGE,
	SKILL_FLAG_SKILL_IS_ON,
	SKILL_FLAG_ON_GROUND_ONLY,
	SKILL_FLAG_LEARNABLE,
	SKILL_FLAG_HOTKEYABLE,
	SKILL_FLAG_ALLOW_IN_MOUSE,
	SKILL_FLAG_ALLOW_IN_LEFT_MOUSE,
	SKILL_FLAG_USES_POWER,
	SKILL_FLAG_DRAINS_POWER,
	SKILL_FLAG_ADJUST_POWER_BY_LEVEL,
	SKILL_FLAG_POWER_COST_BOUNDED_BY_MAX_POWER,
	SKILL_FLAG_ALLOW_REQUEST,
	SKILL_FLAG_TRACK_REQUEST,
	SKILL_FLAG_TRACK_METRICS,
	SKILL_FLAG_SAVE_MISSILES,
	SKILL_FLAG_REMOVE_MISSILES_ON_STOP,
	SKILL_FLAG_STOP_ON_COLLIDE,
	SKILL_FLAG_NO_PLAYER_SKILL_INPUT,
	SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT,
	SKILL_FLAG_NO_IDLE_ON_STOP,
	SKILL_FLAG_DO_NOT_CLEAR_REMOVE_ON_MOVE_STATES,
	SKILL_FLAG_USE_RANGE,
	SKILL_FLAG_DISPLAY_RANGE,
	SKILL_FLAG_GETHIT_CAN_DO,
	SKILL_FLAG_MOVING_CANT_DO,
	SKILL_FLAG_PLAYER_STOP_MOVING,
	SKILL_FLAG_CONSTANT_COOLDOWN,
	SKILL_FLAG_IGNORE_COOLDOWN_ON_START,
	SKILL_FLAG_PLAY_COOLDOWN_ON_WEAPON,
	SKILL_FLAG_USE_UNIT_COOLDOWN,
	SKILL_FLAG_ADD_UNIT_COOLDOWN,
	SKILL_FLAG_DISPLAY_COOLDOWN,
	SKILL_FLAG_CHECK_RANGE_ON_START,
	SKILL_FLAG_CHECK_TARGET_IN_MELEE_RANGE_ON_START,
	SKILL_FLAG_DONT_USE_WEAPON_RANGE,
	SKILL_FLAG_CAN_START_IN_TOWN,
	SKILL_FLAG_CANT_START_IN_PVP,
	SKILL_FLAG_CAN_START_IN_RTS_MODE,
	SKILL_FLAG_ALWAYS_TEST_CAN_START_SKILL,
	SKILL_FLAG_IS_AGGRESSIVE,
	SKILL_FLAG_ANGERS_TARGET_ON_EXECUTE,
	SKILL_FLAG_IS_RANGED,  //tugboat unique flag
	SKILL_FLAG_IS_SPELL,	//tugboat unique flag
	SKILL_FLAG_SERVER_ONLY,
	SKILL_FLAG_CLIENT_ONLY,
	SKILL_FLAG_CHECK_INVENTORY_SPACE,
	SKILL_FLAG_ACTIVATE_WHILE_MOVING,
	SKILL_FLAG_ACTIVATE_IGNORE_MOVING,
	SKILL_FLAG_CAN_NOT_DO_IN_HELLRIFT,
	SKILL_FLAG_UI_IS_RED_ON_FALLBACK,
	SKILL_FLAG_IMPACT_USES_AIM_BONE,
	SKILL_FLAG_DECOY_CANNOT_USE,
	SKILL_FLAG_USES_LASERS,
	SKILL_FLAG_CAN_MULTI_BULLETS,
	SKILL_FLAG_DO_NOT_PREFER_FOR_MOUSE,
	SKILL_FLAG_USE_HOLY_AURA_FOR_RANGE,
	SKILL_FLAG_USES_MISSILES,
	SKILL_FLAG_DISALLOW_SAME_SKILL,
	SKILL_FLAG_DONT_USE_RANGE_FOR_MELEE,
	SKILL_FLAG_FORCE_SKILL_RANGE_FOR_MELEE,
	SKILL_FLAG_DISABLED,
	SKILL_FLAG_SKILL_LEVEL_FROM_STATE_TARGET,
	SKILL_FLAG_USES_ITEM_REQUIREMENTS,
	SKILL_FLAG_NO_BLEND,
	SKILL_FLAG_MOVES_UNIT,
	SKILL_FLAG_SELECTS_A_MELEE_SKILL,
	SKILL_FLAG_REQUIRES_SKILL_LEVEL,
	SKILL_FLAG_DISABLE_CLIENTSIDE_PATHING,
	SKILL_FLAG_EXECUTE_REQUESTED_SKILL_ON_MELEE_ATTACK,
	SKILL_FLAG_CAN_BE_EXECUTED_FROM_MELEE_ATTACK,
	SKILL_FLAG_DONT_STOP_REQUEST_AFTER_EXECUTE,
	SKILL_FLAG_LERP_CAMERA_WHILE_ON,
	SKILL_FLAG_FORCE_USE_WEAPON_TARGETING,
	SKILL_FLAG_DONT_CLEAR_COOLDOWN_ON_DEATH,
	SKILL_FLAG_DONT_COOLDOWN_ON_START,
	SKILL_FLAG_POWER_ON_EVENT,
	SKILL_FLAG_CONSUME_ITEM_ON_EVENT,
	SKILL_FLAG_DEFAULT_SHIFT_SKILL_ENABLED,
	SKILL_FLAG_SHIFT_SKILL_ALWAYS_ENABLED,
	SKILL_FLAG_CAN_KILL_PETS_FOR_POWER_COST,
	SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST,
	SKILL_FLAG_UNIT_EVENT_TRIGGER_SKILL_NEEDS_DAMAGE_INCREMENT,
	SKILL_FLAG_AI_IS_BUSY_WHEN_ON,
	SKILL_FLAG_DONT_SELECT_ON_PURCHASE,
	SKILL_FLAG_USES_ENERGY,
	SKILL_FLAG_NEVER_SET_COOLDOWN,
	SKILL_FLAG_IGNORE_PREVENT_ALL_SKILLS,
	SKILL_FLAG_MUST_START_IN_PORTAL_SAFE_LEVEL,
	SKILL_FLAG_HAS_EVENT_TRIGGER,
	SKILL_FLAG_DONT_TARGET_PETS,
	SKILL_FLAG_REQUIRE_PATH_TO_TARGET,
	SKILL_FLAG_FIRE_TO_LOCATION,
	SKILL_FLAG_FIRE_TO_DEFAULT_UNIT,
	SKILL_FLAG_TEST_FIRING_CONE_ON_START,
	SKILL_FLAG_IGNORE_CHAMPIONS,
	SKILL_FLAG_FACE_TARGET_POSITION,
	SKILL_FLAG_TARGET_DEAD_AND_ALIVE,
	SKILL_FLAG_RESTART_SKILL_ON_REACTIVATE,
	SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_LEARN,
	SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_USE,
	SKILL_FLAG_FORCE_UI_SHOW_EFFECT,
	SKILL_FLAG_TARGET_DONT_IGNORE_OWNED_STATE,
	SKILL_FLAG_GHOST_CAN_DO,
	SKILL_FLAG_USE_BONE_FOR_MISSILE_POSITION,
	SKILL_FLAG_TRANSFER_DAMAGES_TO_PETS,
	SKILL_FLAG_DONT_STAGGER,
	SKILL_FLAG_AVERAGE_COMBINED_DAMAGE,
	SKILL_FLAG_CAN_NOT_START_IN_RTS_LEVEL,
	SKILL_FLAG_DOES_NOT_ACTIVELY_USE_WEAPON,
	SKILL_FLAG_CAN_START_IN_PVP_TOWN,
	SKILL_FLAG_DONT_ALLOW_RESPEC,
	SKILL_FLAG_APPLY_SKILLGROUP_DAMAGE_PERCENT,
	SKILL_FLAG_REQUIRES_STAT_UNLOCK_TO_PURCHASE,
	NUM_SKILL_FLAGS,
};

enum
{
	SKILL_TARGETING_BIT_TARGET_DYING_ON_START = 0,
	SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START,	
	SKILL_TARGETING_BIT_TARGET_DEAD,				
	SKILL_TARGETING_BIT_IGNORE_TEAM,				
	SKILL_TARGETING_BIT_TARGET_FRIEND,				
	SKILL_TARGETING_BIT_TARGET_ENEMY,				
	SKILL_TARGETING_BIT_TARGET_OBJECT,				
	SKILL_TARGETING_BIT_TARGET_CONTAINER,			
	SKILL_TARGETING_BIT_CLOSEST,					
	SKILL_TARGETING_BIT_FIRSTFOUND,					
	SKILL_TARGETING_BIT_USE_LOCATION,				
	SKILL_TARGETING_BIT_DISTANCE_TO_LINE,			
	SKILL_TARGETING_BIT_CHECK_DIRECTION,			
	SKILL_TARGETING_BIT_RANDOM,						
	SKILL_TARGETING_BIT_CHECK_UNIT_RADIUS,			
	SKILL_TARGETING_BIT_CHECK_CAN_MELEE,			
	SKILL_TARGETING_BIT_CHECK_LOS,					
	SKILL_TARGETING_BIT_JUST_COUNT,					
	SKILL_TARGETING_BIT_NO_FLYERS,					
	SKILL_TARGETING_BIT_NO_DESTRUCTABLES,			
	SKILL_TARGETING_BIT_NO_PETS,					
	SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES,		
	SKILL_TARGETING_BIT_ALLOW_SEEKER,				
	SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES,		
	SKILL_TARGETING_BIT_TARGET_PETS,				
	SKILL_TARGETING_BIT_NO_MERCHANTS,				
	SKILL_TARGETING_BIT_TARGET_DYING_FORCE,			
	SKILL_TARGETING_BIT_SORT_BY_UNITID,				
	SKILL_TARGETING_BIT_SORT_BY_DISTANCE,			
	SKILL_TARGETING_BIT_TARGET_ONLY_DYING,			
	SKILL_TARGETING_BIT_TARGET_ONLY_CHAMPIONS,		
	SKILL_TARGETING_BIT_STATE_GIVEN_BY_OWNER,		
	SKILL_TARGETING_BIT_IGNORE_CHAMPIONS,				
	SKILL_TARGETING_BIT_ALLOW_TARGET_DEAD_OR_DYING,	
	SKILL_TARGETING_BIT_TARGET_BOTH_DEAD_AND_ALIVE,	
	SKILL_TARGETING_BIT_DONT_IGNORE_OWNED_STATE,
	SKILL_TARGETING_BIT_CHECK_SIMPLE_PATH_EXISTS,
	NUM_SKILL_TARGETING_BITS
};

#define NUM_TARGET_QUERY_FILTER_FLAG_DWORDS (DWORD_FLAG_SIZE(NUM_SKILL_TARGETING_BITS))
struct TARGET_QUERY_FILTER
{
	enum
	{
		IGNORE_STATE_COUNT = 2,
		ALLOW_ONLY_STATES_COUNT = 2,
	};
	DWORD		pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
	int			nUnitType;
	int			nIgnoreUnitType;
	int			pnIgnoreUnitsWithState[ IGNORE_STATE_COUNT ];
	int			pnAllowUnitsOnlyWithState[ ALLOW_ONLY_STATES_COUNT ];

	TARGET_QUERY_FILTER(void) :
		nUnitType( 0 ),
		nIgnoreUnitType( 0 )
	{
		ZeroMemory( pdwFlags, NUM_TARGET_QUERY_FILTER_FLAG_DWORDS * sizeof( DWORD ) );
		for ( int i = 0; i < IGNORE_STATE_COUNT; i++ )
			pnIgnoreUnitsWithState[ i ] = INVALID_ID;
		for ( int i = 0; i < ALLOW_ONLY_STATES_COUNT; i++ )
			pnAllowUnitsOnlyWithState[ i ] = INVALID_ID;
	}
};

//----------------------------------------------------------------------------
enum SKILL_FAMILY
{
	SKILL_FAMILY_INVALID = -1,
	
	SKILL_FAMILY_TURRET_GROUND,
	SKILL_FAMILY_TURRET_FLYING,
	// other families here ... we don't have many right now, feel free to make family mean whatever you want ;) 
	
	SKILL_FAMILY_NUM_FAMALIES		// keep this last
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
enum ESKILL_LEVEL_DEFUALTS
{
	KSkillLevel_DMG,
	KSkillLevel_Armor,
	KSkillLevel_AttackSpeed,
	KSkillLevel_ToHit,
	KSkillLevel_PercentDamage,
	KSkillLevel_Tier,
	KSkillLevel_CraftingTier,
	KSkillLevel_Count,
};
struct SKILL_LEVELS
{
	int			nLevel;
	int			nSkillLevelValues[ KSkillLevel_Count ];
};



#define MAX_SKILL_BONUS_COUNT 5		//the number of bonuses allowed
struct SKILL_BONUS_BY_SKILL
{
	int			nSkillBonusBySkills[ MAX_SKILL_BONUS_COUNT ];	//number of skills that a given skill can get bonuses from a percent	
	PCODE		nSkillBonusByValueByScript[ MAX_SKILL_BONUS_COUNT ];	//the value of the bonus by script
};

//----------------------------------------------------------------------------
enum SKILL_USAGE
{
	USAGE_INVALID = NONE,
	
	USAGE_ACTIVE,
	USAGE_PASSIVE,
	USAGE_TOGGLE,
	
	USAGE_NUM_USAGE
};

enum SKILL_STRING_TYPES
{
	SKILL_STRING_DESCRIPTION,
	SKILL_STRING_EFFECT,
	SKILL_STRING_SKILL_BONUSES,
	SKILL_STRING_EFFECT_ACCUMULATION,
	SKILL_STRING_AFTER_REQUIRED_WEAPON,
	NUM_SKILL_STRINGS,
};


//----------------------------------------------------------------------------

struct SKILL_ACTIVATOR_DATA;
enum SKILL_ACTIVATOR_KEY;

typedef BOOL (*PFN_SKILL_ACTIVATOR)( const SKILL_ACTIVATOR_DATA & tData );

#define MAX_SKILL_LEVEL_INCLUDE_SKILLS 5
#define MAX_SKILLS_ON 64
#define MAX_LEVELS_PER_SKILL	30
#define MAX_WEAPON_LOCATIONS_PER_SKILL 2
#define MAX_PREREQUISITE_SKILLS 4
#define MAX_MISSILES_PER_SKILL 8
#define MAX_SKILLS_PER_SKILL 5
#define ACTIVATOR_STRING_LENGTH 32
#define MAX_SKILLS_PER_UNIT 128
#define NUM_FALLBACK_SKILLS 3
#define MAX_SKILL_GROUPS_PER_SKILL 4
#define MAX_PREREQUISITE_STATS 2
#define MAX_SUMMONED_INVENTORY_LOCATIONS 6
#define MAX_LINKED_SKILLS 3
#define MAX_SKILL_EVENT_TRIGGERS 4
#define MAX_PROHIBITING_STATES 4
const int INVALID_SKILL_LEVEL  = 0;
#define HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD 0.15f
#define MAX_SKILL_ACCULATE 3
#define MAX_SKILL_POINTS_PER_TREE 100
#define MAX_SKILLS_IN_TAB 60
#define MAX_SKILL_POINTS_IN_TIER 30
#define MAX_SKILL_POINTS_IN_CRAFTING_TIER 10
#define MAX_SKILL_STRING_CODE_LENGTH 128
struct SKILL_STATS
{	
	char		szName[DEFAULT_INDEX_SIZE];
	int			nSkillId;
	int			nLinksTo;	//links to. If you ask for a level higher
	int			nUsingXColumns; //since the MAX_LEVELS_PER_SKILL can change we need to really specify how many we use
	int			nSkillModValueByTreeInvestment;
	int			nSkillModValueForPVP;
	int			nSkillStatValues[ MAX_LEVELS_PER_SKILL ];
};

enum SKILL_SCRIPT_ENUM
{
	SKILL_SCRIPT_TRANSFER_ON_ATTACK,
	SKILL_SCRIPT_STATE_REMOVED,
	SKILL_SCRIPT_CRAFTING,
	SKILL_SCRIPT_CRAFTING_PROPERTIES,
	SKILL_SCRIPT_ITEM_SKILL_SCRIPT1,
	SKILL_SCRIPT_ITEM_SKILL_SCRIPT2,
	SKILL_SCRIPT_ITEM_SKILL_SCRIPT3,
	SKILL_SCRIPT_COUNT
};

struct SKILL_DATA
{
	char		szName[DEFAULT_INDEX_SIZE];
	DWORD		dwCode;
	int			nId;
	DWORD		dwFlags[DWORD_FLAG_SIZE(NUM_SKILL_FLAGS)];
	int			nDisplayName;
	int			nSkillLevelIncludeSkills[ MAX_SKILL_LEVEL_INCLUDE_SKILLS ];
	char		pszStringFunctions[NUM_SKILL_STRINGS][ MAX_SKILL_STRING_CODE_LENGTH ];
	int			pnStringFunctions[NUM_SKILL_STRINGS];
	int			pnStrings[NUM_SKILL_STRINGS];
	int			nSkillsToAccumulate[ MAX_SKILL_ACCULATE ]; //for UI
	char		szEvents[DEFAULT_FILE_SIZE];
	char		szActivator[ACTIVATOR_STRING_LENGTH];
	char		szLargeIcon[DEFAULT_XML_FRAME_NAME_SIZE];
	char		szSmallIcon[DEFAULT_XML_FRAME_NAME_SIZE];
	SAFE_FUNC_PTR(PFN_SKILL_ACTIVATOR, pfnActivator);
	int			nIconColor;
	int			nIconBkgndColor;
	int			nSkillTab;
	int			nSkillsInSameTab[ MAX_SKILLS_IN_TAB ];
	int			nSkillsInSameTabCount;
	int			pnSkillGroup[ MAX_SKILL_GROUPS_PER_SKILL ];
	int			nSkillPageX;
	int			nSkillPageY;
	int			pnSkillLevelToMinLevel[ MAX_LEVELS_PER_SKILL ];
	int			nMaxLevel;
	char		szSummonedAI[DEFAULT_FILE_SIZE];
	int			nSummonedAI;
	int			pnSummonedInventoryLocations[ MAX_SUMMONED_INVENTORY_LOCATIONS ];
	int			nMaxSummonedMinorPetClasses;
	int			nUIThrobsOnState;
	float		fPowerCost;
	float		fPowerCostPerLevel;
	int			nPowerCost;
	int			nPowerCostPerLevel;
	int			pnPerkPointCost[ MAX_LEVELS_PER_SKILL ];

	int			nPriority;
	int			pnRequiredStats[ MAX_PREREQUISITE_STATS ];
	int			pnRequiredStatValues[ MAX_PREREQUISITE_STATS ][ MAX_LEVELS_PER_SKILL ];
	int			pnRequiredSkills[ MAX_PREREQUISITE_SKILLS ];
	int			pnRequiredSkillsLevels[ MAX_PREREQUISITE_SKILLS ];
	BOOL		bOnlyRequireOneSkill;
	BOOL		bUsesCraftingPoints;
	int			pnWeaponInventoryLocations[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	int			pnFallbackSkills[ NUM_FALLBACK_SKILLS ];
	int			pnWeaponIndices[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	int			pnMissileIds[ MAX_MISSILES_PER_SKILL ]; // these are pulled from the skill events
	int			pnSkillIds[ MAX_SKILLS_PER_SKILL ]; // these are pulled from the skill events
	TARGET_QUERY_FILTER tTargetQueryFilter;
	int			nEventsId;
	int			nModeOverride;
	
	SKILL_BONUS_BY_SKILL tSkillBonus;
	PCODE		codeVariables[ SKILL_VARIABLE_COUNT ];
	PCODE		codeCrafting;							// script run on crafted items
	PCODE		codeCraftingProperties;					// script run on skills to get crafting points
	PCODE		codeSkillRangeScript;					// skill Range
	PCODE		codeCost;
	PCODE		codeCooldownPercentChange;
	PCODE		codeStatsSkillEvent;					// run on client and server when skill events are executed or stats are transfered to a pet
	PCODE		codeStatsSkillTransferOnAttack;			// code to on when doing an attack ( ranged or melee )
	PCODE		codeStatsSkillEventServer;				// run on server when skill events are executed or stats are transfered to a pet
	PCODE		codeStatsSkillEventServerPostProcess;	// gets run on statslist as second pass (for manipulations that shouldn't get displayed in skill mouse over)
	PCODE		codeStatsServerPostLaunch;			// used for missiles
	PCODE		codeStatsServerPostLaunchPostProcess;	// run after codeStatsServerPostLaunch
	PCODE		codeStatsPostLaunch;				// used for missiles
	PCODE		codeStatsOnStateSet;					// Script to run upon State set on Server and Client
	PCODE		codeStatsOnStateSetServerOnly;
	PCODE		codeStatsOnStateSetServerPostProcess;	// run after codeStatsOnStateSetServerOnly (server only)
	PCODE		codeStatsOnStateSetPostProcess;			// run after codeStatsOnStateSetServerOnly (server & client)
	PCODE		codeStatsOnPulseServerOnly;
	PCODE		codeStatsOnDeselectServerOnly;
	PCODE		codeStatsOnPulse;
	PCODE		codeStatsOnLevelChange;
	PCODE		codeStatsOnSkillStart;
	PCODE		codeStatsScriptOnTarget;
	PCODE		codeStatsScriptFromEvents;
	PCODE		codeSelectCost;
	PCODE		codeStartCondition;
	PCODE		codeStartConditionOnTarget;
	PCODE		codeStateDuration;
	PCODE		codeStateDurationBonus;
	PCODE		codeActivatorCondition;
	PCODE		codeActivatorConditionOnTargets;
	PCODE		codeEventChance;
	PCODE		codeEventParam0;
	PCODE		codeEventParam1;
	PCODE		codeEventParam2;
	PCODE		codeEventParam3;
	PCODE		codeInfoScript;
	PCODE		codeStateRemoved;
	PCODE		codePowerCost;
	PCODE		codePowerCostMultPCT;
	PCODE		codeCoolDown;
	PCODE		codeSelectCondition;
	int			nPulseSkill;
	int			nSelectCheckStat;
	int			nStartFunc;
	int			nTargetingFunc;
	int			nAimHolderIndex;
	int			nAimEventIndex;
	int			nGivesSkill;
	int			nExtraSkillToTurnOn;
	int			nPlayerInputOverride;
	int			nRequiredUnittype;
	int			nRequiredWeaponUnittype;
	int			pnPreferedWeaponUnittype[ MAX_WEAPON_LOCATIONS_PER_SKILL ];  //tugboat unique
	int			nFuseMissilesOnState;
	int			nRequiredState;
	int			nProhibitingStates[ MAX_PROHIBITING_STATES ];
	int			nStateOnSelect;
	int			nClearStateOnSelect;
	int			nClearStateOnDeselect;
	int			nPreventClearStateOnSelect;
	int			nMinHoldTicks;
	int			nHoldWithMode;
	int			nWarmupTicks;
	int			nTestTicks;
	int			nCooldownTicks;
	int			nCooldownSkillGroup;
	int			nCooldownTicksForSkillGroup;
	int			nCooldownFinishedSound;
	int			nCooldownMinPercent;	
	SKILL_ACTIVATOR_KEY eActivatorKey;
	int			nActivateMode;
	int			nActivateSkill;
	int			nActivatePriority;
	float		fRangeMin;
	float		fRangeMax;
	float		fFiringCone;
	float		fRangeDesired;
	float		fRangePercentPerLevel;
	float		fWeaponRangeMultiplier;
	float		fImpactForwardBias;		//hellgate unique
	float		fModeSpeedMultiplier;	//tugboat unique
	int			nDamageTypeOverride;	//tugboat unique
	float		fDamageMultiplier;		//tugboat unique
	int			nMaxExtraSpreadBullets;		// max # of extra bullets allowed to be added to missile firing via spread skill
	int			nSpreadBulletMultiplier;	// The # of extra bullets from spread skill is mulitplied by this 
	float		flReflectiveLifetimeInSeconds;	// max lifetime of bullets that become reflective created from this skill
	float		fParam1;			// a well-commented param to be used by designers to guide the behavior of a skill
	float		fParam2;			// a well-commented param to be used by designers to guide the behavior of a skill
	SKILL_USAGE eUsage;
	SKILL_FAMILY eFamily;			// skill family grouping
	int			nUnitEventTrigger[ MAX_SKILL_EVENT_TRIGGERS ];	// UnitEvent that triggers this skill when it fires
	PCODE		codeUnitEventTriggerChance;	// Chance that the unitevent trigger will fire this skill
	int			nMissileCount;
	int			nLaserCount;
	int			pnLinkedLevelSkill[ MAX_LINKED_SKILLS ];	// when this skill level changes, this linked skill level mirrors it
	int			nSkillParent;	
	int			nFieldMissile;
	int			nUnlockPurchaseItem;

	// these are filled by code
	SAFE_PTR( int*, pnAchievements );
	USHORT		nNumAchievements;

};


//----------------------------------------------------------------------------
struct SKILL_EVENT_FUNCTION_DATA
{
	GAME *							pGame;
	UNIT *							pUnit;
	const struct SKILL_EVENT *		pSkillEvent;
	int								nSkill;
	int								nSkillLevel;
	const SKILL_DATA *				pSkillData;
	UNITID							idWeapon;
	float							fDuration;
	DWORD_PTR						pParam;

	SKILL_EVENT_FUNCTION_DATA( 
		GAME * pGameIn,
		UNIT * pUnitIn,
		const struct SKILL_EVENT * pSkillEventIn,
		int nSkillIn,
		int in_nSkillLevel,
		const SKILL_DATA * pSkillDataIn,
		UNITID idWeaponIn
		) :
		pGame( pGameIn ),
		pUnit( pUnitIn ),
		pSkillEvent( pSkillEventIn ),
		nSkill( nSkillIn ),
		nSkillLevel( in_nSkillLevel),
		pSkillData( pSkillDataIn ),
		idWeapon( idWeaponIn ),
		fDuration( 0.0f ),
		pParam( NULL )
		{}
};
typedef BOOL (*PFN_SKILL_EVENT)( const SKILL_EVENT_FUNCTION_DATA & tData );

//----------------------------------------------------------------------------
struct SKILL_EVENT_PREVIEW_DATA
{
	int nModel;
	struct SKILL_EVENT * pSkillEvent;

	SKILL_EVENT_PREVIEW_DATA( 
		int nModelIn,
		struct SKILL_EVENT * pSkillEventIn ) :
		nModel( nModelIn ),
		pSkillEvent( pSkillEventIn )
	{}
};
typedef BOOL (*PFN_SKILL_EVENT_PREVIEW)( const SKILL_EVENT_PREVIEW_DATA & tData );

#define MAX_SKILL_EVENT_PARAM (4)
#define SKILL_EVENT_STRING_SIZE 32
#define SKILL_EVENT_INDEX_SIZE 48
struct SKILL_EVENT_TYPE_DATA
{
	char pszName[ SKILL_EVENT_INDEX_SIZE ];
	char szParamStrings[ MAX_SKILL_EVENT_PARAM ][ SKILL_EVENT_STRING_SIZE ];
	//char pszParamString[ SKILL_EVENT_STRING_SIZE ];
	DWORD dwFlagsUsed;
	DWORD dwFlagsUsed2;
	EXCELTABLE	eAttachedTable;
	int		nParamContainsCount;
	BOOL	bDoesNotRequireTableEntry;
	BOOL	bApplySkillStats;
	BOOL	bUsesAttachment;
	BOOL	bUsesBones;
	BOOL	bUsesBoneWeights;
	BOOL	bServerOnly;
	BOOL	bClientOnly;
	BOOL	bNeedsDuration;
	BOOL	bAimingEvent;
	BOOL	bIsMelee;
	BOOL	bIsRanged;	//tugboat unique
	BOOL	bIsSpell;	//tugboat unique
	BOOL	bSubSkillInherit;
	BOOL	bConvertParamFromDegreesToDot;
	BOOL	bUsesTargetIndex;
	BOOL	bTestsItsCondition;
	BOOL	bUsesLasers;
	BOOL	bUsesMissiles;
	BOOL	bCanMultiBullets;
	BOOL	bParamsUsedInSkillString;
	BOOL	bStartCoolingAndPowerCost;
	BOOL	bConsumeItem;
	BOOL	bCheckPetPowerCost;
	BOOL	bCausesAttackEvent;
	char	szEventHandler[ MAX_FUNCTION_STRING_LENGTH ];	
	int		nEventStringFunction;
	
	// runtime data
	SAFE_FUNC_PTR(PFN_SKILL_EVENT, pfnEventHandler);		// event handler function pointer (set at runtime)
	SAFE_FUNC_PTR(PFN_SKILL_EVENT_PREVIEW, pfnPreviewHandler);
};

BOOL SkillEventTypeExcelPostProcess(
	struct EXCEL_TABLE * table);

//----------------------------------------------------------------------------
#define SKILL_EVENT_FLAG_LASER_TURNS						MAKE_MASK(0)
#define	SKILL_EVENT_FLAG_REQUIRES_TARGET					MAKE_MASK(1)
#define SKILL_EVENT_FLAG_FORCE_NEW							MAKE_MASK(2)
#define SKILL_EVENT_FLAG_LASER_SEEKS_SURFACES				MAKE_MASK(3)
#define SKILL_EVENT_FLAG_FACE_TARGET						MAKE_MASK(4)
#define SKILL_EVENT_FLAG_USE_UNIT_TARGET					MAKE_MASK(5)
#define SKILL_EVENT_FLAG_USE_EVENT_OFFSET					MAKE_MASK(6)
#define SKILL_EVENT_FLAG_LOOP								MAKE_MASK(7)
#define SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE			MAKE_MASK(8)
#define SKILL_EVENT_FLAG_PLACE_ON_TARGET					MAKE_MASK(9)
#define SKILL_EVENT_FLAG_USE_ANIM_CONTACT_POINT				MAKE_MASK(10)
#define SKILL_EVENT_FLAG_TRANSFER_STATS						MAKE_MASK(11)
#define SKILL_EVENT_FLAG_DO_WHEN_TARGET_IN_RANGE			MAKE_MASK(12)
#define SKILL_EVENT_FLAG_ADD_TO_CENTER						MAKE_MASK(13)
#define SKILL_EVENT_FLAG_360_TARGETING						MAKE_MASK(14)
#define SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION			MAKE_MASK(15)
#define SKILL_EVENT_FLAG_USE_AI_TARGET						MAKE_MASK(16)
#define SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON					MAKE_MASK(17)
#define SKILL_EVENT_FLAG_FLOAT								MAKE_MASK(18)
#define SKILL_EVENT_FLAG_DONT_VALIDATE_TARGET				MAKE_MASK(19)
#define SKILL_EVENT_FLAG_RANDOM_FIRING_DIRECTION			MAKE_MASK(20)
#define SKILL_EVENT_FLAG_AUTOAIM_PROJECTILE					MAKE_MASK(21)
#define SKILL_EVENT_FLAG_TARGET_WEAPON						MAKE_MASK(22)
#define SKILL_EVENT_FLAG_USE_WEAPON_FOR_CONDITION			MAKE_MASK(23)
#define SKILL_EVENT_FLAG_FORCE_CONDITION_ON_EVENT			MAKE_MASK(24)
#define SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE			MAKE_MASK(25)
#define SKILL_EVENT_FLAG_USE_CHANCE_PCODE					MAKE_MASK(26)
#define SKILL_EVENT_FLAG_SERVER_ONLY						MAKE_MASK(27)
#define SKILL_EVENT_FLAG_CLIENT_ONLY						MAKE_MASK(28)
#define SKILL_EVENT_FLAG_LASER_ATTACKS_LOCATION				MAKE_MASK(29)
#define SKILL_EVENT_FLAG_AT_NEXT_COOLDOWN					MAKE_MASK(30)
#define SKILL_EVENT_FLAG_AIM_WITH_WEAPON					MAKE_MASK(31)

#define SKILL_EVENT_FLAG2_AIM_WITH_WEAPON_ZERO				MAKE_MASK(0)
#define SKILL_EVENT_FLAG2_USE_PARAM0_PCODE					MAKE_MASK(1)
#define SKILL_EVENT_FLAG2_USE_PARAM1_PCODE					MAKE_MASK(2)
#define SKILL_EVENT_FLAG2_USE_PARAM2_PCODE					MAKE_MASK(3)
#define SKILL_EVENT_FLAG2_USE_PARAM3_PCODE					MAKE_MASK(4)
#define SKILL_EVENT_FLAG2_USE_ULTIMATE_OWNER				MAKE_MASK(5)
#define SKILL_EVENT_FLAG2_CHARGE_POWER_AND_COOLDOWN			MAKE_MASK(6)
#define SKILL_EVENT_FLAG2_MARK_SKILL_AS_SUCCESSFUL			MAKE_MASK(7)
#define SKILL_EVENT_FLAG2_LASER_INCLUDE_IN_UI				MAKE_MASK(8)
#define SKILL_EVENT_FLAG2_CAUSE_ATTACK_EVENT				MAKE_MASK(9)
#define SKILL_EVENT_FLAG2_DONT_CAUSE_ATTACK_EVENT			MAKE_MASK(10)
#define SKILL_EVENT_FLAG2_LASER_DONT_TARGET_UNITS			MAKE_MASK(11)
#define SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS				MAKE_MASK(12)


//----------------------------------------------------------------------------
struct SKILL_EVENT_PARAM
{
	float flValue;
	int nValue;
};

//----------------------------------------------------------------------------
struct SKILL_EVENT
{
	DWORD					dwFlags;
	DWORD					dwFlags2;
	int						nType;
	float					fTime;
	SKILL_EVENT_PARAM		tParam[ MAX_SKILL_EVENT_PARAM ];
	float					fRandChance;
	ATTACHMENT_DEFINITION	tAttachmentDef;
	CONDITION_DEFINITION	tCondition;
};

//----------------------------------------------------------------------------
struct SKILL_EVENT_HOLDER
{
	int					nUnitMode;
	SKILL_EVENT			*pEvents;
	int					nEventCount;
	float				fDuration;
};


//----------------------------------------------------------------------------
struct SKILL_EVENTS_DEFINITION
{
	DEFINITION_HEADER	tHeader;
	SKILL_EVENT_HOLDER	*pEventHolders;
	int					nEventHolderCount;
	char				szPreviewAppearance[ MAX_XML_STRING_LENGTH ];
	int					nPreviewAppearance;
};

typedef BOOL (*PFN_SKILL_TARGET_FILTER)(const UNIT * pUnit, const EVENT_DATA * pEventData);

struct UNIT;

#define MAX_TARGETS_PER_QUERY 64

struct SKILL_TARGETING_QUERY
{
	struct ROOM * pCenterRoom;
	UNIT * pSeekerUnit;
	TARGET_QUERY_FILTER tFilter;	
	const VECTOR * pvLocation;
	float fMaxRange;
	UNITID * pnUnitIds;
	int nUnitIdMax;
	int nUnitIdCount;
	DWORD dwSkillSeed;
	float fDirectionTolerance;
	int nPower;
	float fLOSDistance;
	int pnSummonedInventoryLocations[ MAX_SUMMONED_INVENTORY_LOCATIONS ];
	const SKILL_DATA * pSkillData;
	PCODE codeSkillFilter;
	int nRangeModifierStat_Percent;
	int nRangeModifierStat_Min;
	PFN_SKILL_TARGET_FILTER pfnFilter;
	EVENT_DATA * pFilterEventData;

	SKILL_TARGETING_QUERY(void) :
		pCenterRoom( NULL ),
		pSeekerUnit( NULL ),
		pvLocation( NULL ),
		fMaxRange( 0.0f ),
		pnUnitIds( NULL ),
		nUnitIdMax( 0 ),
		nUnitIdCount( 0 ),
		dwSkillSeed( 0 ),
		fDirectionTolerance( 0.5f ),
		nPower( 0 ),
		fLOSDistance( 0.0f ),
		pSkillData( NULL ),
		nRangeModifierStat_Percent( INVALID_ID ),
		nRangeModifierStat_Min( INVALID_ID ),
		codeSkillFilter( 0 ),
		pfnFilter(NULL),
		pFilterEventData(NULL)
	{
		tFilter.nUnitType = UNITTYPE_ANY;
		SETBIT( tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
		for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
			pnSummonedInventoryLocations[ i ] = INVALID_ID;
	}

	SKILL_TARGETING_QUERY(const SKILL_DATA * pSkillData) :
		pCenterRoom( NULL ),
		pSeekerUnit( NULL ),
		pvLocation( NULL ),
		fMaxRange( 0.0f ),
		pnUnitIds( NULL ),
		nUnitIdMax( 0 ),
		nUnitIdCount( 0 ),
		dwSkillSeed( 0 ),
		fDirectionTolerance( 0.5f ),
		nPower( 0 ),
		fLOSDistance( 0.0f ),
		pSkillData( pSkillData ),
		nRangeModifierStat_Percent( INVALID_ID ),
		nRangeModifierStat_Min( INVALID_ID ),
		codeSkillFilter( 0 ),
		pfnFilter(NULL),
		pFilterEventData(NULL)
	{
		if( !pSkillData )
			return;
		tFilter = pSkillData->tTargetQueryFilter;
		for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
			pnSummonedInventoryLocations[ i ] = pSkillData->pnSummonedInventoryLocations[ i ];
	}
};

//----------------------------------------------------------------------------
struct SKILLTAB_DATA 
{
	char	szName[DEFAULT_INDEX_SIZE];
	WORD	wCode;
	int		nDisplayString;
	BOOL	bDrawOnlyKnown;
	BOOL	bIsPerkTab;
	char	szSkillIconTexture[DEFAULT_INDEX_SIZE];
};

//----------------------------------------------------------------------------
struct SKILLGROUP_DATA 
{
	char	szName[DEFAULT_INDEX_SIZE];
	WORD	wCode;
	int		nDisplayString;
	BOOL	bDisplayInSkillString;
	BOOL	bDontClearCooldownOnDeath;
	char	szBackgroundIcon[DEFAULT_XML_FRAME_NAME_SIZE];
};

#define SKILL_START_FLAG_IS_REPEATING			MAKE_MASK( 0 )
#define SKILL_START_FLAG_NO_REPEAT_EVENT		MAKE_MASK( 1 )
#define SKILL_START_FLAG_DONT_RETARGET			MAKE_MASK( 2 )
#define SKILL_START_FLAG_DONT_SET_COOLDOWN		MAKE_MASK( 3 )
#define SKILL_START_FLAG_IGNORE_COOLDOWN		MAKE_MASK( 4 )
#define SKILL_START_FLAG_INITIATED_BY_SERVER	MAKE_MASK( 5 )
#define SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT	MAKE_MASK( 6 )
#define SKILL_START_FLAG_SERVER_ONLY			MAKE_MASK( 7 )
#define SKILL_START_FLAG_STAGGER				MAKE_MASK( 8 )
#define SKILL_START_FLAG_DELAY_MELEE			MAKE_MASK( 9 )
#define SKILL_START_FLAG_DONT_SEND_TO_SERVER	MAKE_MASK( 10 )
#define SKILL_START_FLAG_IGNORE_POWER_COST		MAKE_MASK( 11 )

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

void SkillsInitSkillInfo(
	UNIT * pUnit );

void SkillsFreeSkillInfo(
	UNIT * pUnit );

enum SKILLS_INFO_ARRAY
{
	SIA_REQUEST,
	SIA_IS_ON,
	SIA_TARGETS,
};

int SkillsGetSkillInfoArrayLength(
	UNIT * pUnit,
	SKILLS_INFO_ARRAY eArray);

void SkillsGetSkillInfoItemText(
	UNIT * pUnit,
	SKILLS_INFO_ARRAY eArray,
	unsigned int nIndex,
	int nTextLength,
	WCHAR * pwszText);

BOOL SkillEventsDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad);

const SKILL_EVENT_PARAM *SkillEventGetParam(
	const SKILL_EVENT *pSkillEvent,
	int nParam);

int SkillEventGetParamInt(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript = NULL);

float SkillEventGetParamFloat(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript = NULL);

BOOL SkillEventGetParamBool(
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	const SKILL_EVENT *pSkillEvent,
	int nParamIndex,
	BOOL * pbHadScript = NULL);

const char *SkillGetDevName(
	GAME *pGame,
	int nSkill);

const WCHAR *SkillGetDisplayName(
	GAME *pGame,
	int nSkill);
	
BOOL SkillFindTarget(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon,
	UNIT ** ppTargetUnit,
	VECTOR & vTarget );

void SkillStartCooldown( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nSkill,
	int nSkillLevel,
	const SKILL_DATA * pSkillData,
	int nMinTicks,
	BOOL bForceMinTicks = FALSE,
	BOOL bSendMessage = FALSE );

void SkillClearCooldown(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill);

int SkillGetStartTick(
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill,
	BOOL bIgnoreWeapon);

BOOL SkillCanStart(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	UNIT * pTargetUnit,
	BOOL bCheckIsOn,
	BOOL bPlaySounds,
	DWORD dwSkillStartFlags );

BOOL SkillShouldStart(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pUnitTarget,
	VECTOR * pvTarget );

BOOL c_SkillControlUnitRequestStartSkill(
	GAME * pGame,
	int nSkill,
	UNIT * pTargetOverride = NULL,
	VECTOR vTargetPosOverride = VECTOR( 0, 0, 0 ) );

BOOL SkillStartRequest(
	GAME * game,
	UNIT * unit,
	int skill,
	UNITID idTarget,
	const VECTOR & vTarget,
	DWORD dwStartFlags,
	DWORD dwRandSeed,
	int skillLevel = 0);

void SkillStopRequest(
	UNIT * pUnit,
	int nSkill,
	BOOL bSend = FALSE,
	BOOL bCheckIsRequested = TRUE,
	BOOL bForceSendToAll = FALSE);

void SkillStopAll(
	GAME * pGame,
	UNIT * pUnit);

void SkillStopAllRequests(
	UNIT * pUnit );

BOOL SkillsEnteredDead(
	GAME * pGame,
	UNIT * pUnit);

BOOL SkillStop(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	BOOL bSend = FALSE,
	BOOL bForceSendToAll = FALSE);

BOOL SkillsIgnoreUnitInput(
	GAME * pGame,
	UNIT * pUnit,
	BOOL & bStopMoving );

DWORD SkillGetNextSkillSeed(
	GAME * pGame );

UNIT * SkillGetNearestTarget( 
	GAME * pGame, 
	UNIT * pUnit, 
	const SKILL_DATA * pSkillData,
	float fMaxRange, 
	VECTOR * pvLocation = NULL,
	DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ] = NULL );		//tugboat added

void SkillTargetQuery( 
	GAME * pGame, 
	SKILL_TARGETING_QUERY & tTargetingQuery );

BOOL SkillIsValidTarget( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pTarget,
	UNIT * pWeapon,
	int nSkill,
	BOOL bStartingSkill,
	BOOL * pbIsInRange = NULL,
	BOOL bIgnoreWeaponTargeting = FALSE,
	BOOL bDontCheckAttackerUnittype = FALSE);

void c_SkillControlUnitChangeTarget( 
	UNIT * pTarget );

void c_SkillLoad(
	GAME * pGame,
	int nSkill);

void c_SkillEventsLoad(
	GAME * pGame,
	int nSkillEventsId);

void c_SkillFlagSoundsForLoad(
	GAME * pGame,
	int nSkill,
	BOOL bLoadNow);

void c_SkillEventsFlagSoundsForLoad(
	GAME * pGame,
	SKILL_EVENTS_DEFINITION * pEvents,
	BOOL bLoadNow);

DWORD c_SkillGetPlayerInputOverride(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vForward,
	BOOL & bSetModesToIdle );

void UnitGetWeapons(
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeaponArray[ MAX_WEAPON_LOCATIONS_PER_SKILL ],
	BOOL bCheckCooldown );

UNIT * SkillGetTarget(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	int nIndex = 0,
	DWORD * pdwFlags = NULL );

UNITID SkillGetTargetId(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	int nIndex = 0 );

UNIT * SkillGetAnyTarget(
	UNIT * pUnit,
	int nSkill = INVALID_ID,
	UNIT * pWeapon = NULL,
	BOOL bCheckValid = FALSE );

UNIT * SkillGetAnyValidTarget(
	UNIT * pUnit,
	int nSkill );

void SkillSetTarget(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	UNITID idTarget,
	int nIndex = 0 );

void SkillClearTargets(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon );

void SkillSelect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bForce = FALSE,
	BOOL bUseSeed = FALSE,
	DWORD dwSkillSeed = 0);

void SkillDeselect(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	BOOL bForce );

void s_SkillsSelectAll(
	GAME * pGame,
	UNIT * pUnit );

BOOL SkillIsSelected(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill);

int SkillGetPowerCost( 
	UNIT * pUnit, 
	const SKILL_DATA * pSkillData,
	int nSkillLevel,
	BOOL bCanCapByPowerMax = TRUE );

int SkillGetSelectCost( 
	UNIT * pUnit, 
	int nSkill,
	const SKILL_DATA * pSkillData,
	int nSkillLevel );

int SkillItemGetSkill(
	UNIT* pSkillItem );

int SkillGetSkillCurrentlyUsingWeapon(
	UNIT * pUnit,
	UNIT * pWeapon);

float UnitGetHolyRadius( 
	UNIT * pUnit );

float SkillGetRange(
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon = NULL,
	BOOL bGetMeleeRange = FALSE,
	int nSkillLevel = 0 );

float SkillGetDesiredRange(
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon = NULL,
	BOOL bGetMeleeRange = FALSE,
	int nSkillLevel = 0 );

int SkillGetLevel(
	UNIT * pUnit,
	int nSkill,
	BOOL ignoreBonus = FALSE );

void SkillLevelChanged(
	UNIT * unit,
	int nSkill,
	BOOL bHadSkill );

void SkillUnitSetInitialStats(
	UNIT *pUnit, 
	int nSkillID, 
	int nLevel);

BOOL SkillTestFlag(
	GAME *pGame,
	UNIT *pSkillUnit,
	int nFlag );

BOOL UnitCanLearnSkill( 
	UNIT * pUnit,
	int nSkill );

int SkillNeedsToHold(
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill,
	BOOL bIgnoreWeapon = FALSE );

void SkillSaveUnitForRemovalOnSkillStop( 
	GAME * pGame, 
	UNIT * pSource, 
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill );

void SkillSaveUnitForFuseOnStateClear( 
	GAME * pGame, 
	UNIT * pSource, 
	UNIT * pUnit,
	int nState );

BOOL UnitCanMeleeAttackUnit(
	UNIT* attacker,
	UNIT* defender,
	UNIT* weapon,
	int attack_index,
	float fRange,
	BOOL bForceIsInRange,
	const SKILL_EVENT * pSkillEvent,
	BOOL bAllowDead,
	float fFiringCone = 0.0f,
	VECTOR * pvProposedAttackerLocation = NULL);

BOOL UnitInFiringCone(
					  UNIT* attacker,
					  UNIT* defender,
					  const SKILL_EVENT * pSkillEvent,
					  float fFiringCone,
					  VECTOR * pvProposedAttackerLocation = NULL);

#define MELEE_IMPACT_FORCE	40
BOOL UnitMeleeAttackUnit(
	UNIT* attacker,
	UNIT* defender,
	UNIT* weapon,
	UNIT* weapon_other,
	int nSkill,		
	int attack_index,
	float fRange,
	BOOL bForceIsInRange,
	const struct SKILL_EVENT * pvImpactDelta,	
	float fDamageMultiplier,
	int nDamageTypeOverride,
	float fForce = MELEE_IMPACT_FORCE,
	DWORD dwUnitAttackUnitFlags = 0);

BOOL UnitUsesEnergy(
	UNIT * pWeapon );

int SkillNextLevelAvailableAt( 
	UNIT * pUnit,
	int nSkill);
	
enum {
	SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL,		
	SKILLLEARN_BIT_NO_SKILL_POINTS,		
	SKILLLEARN_BIT_SKILL_IS_DISABLED,	
	SKILLLEARN_BIT_LEVEL_IS_TOO_LOW,		
	SKILLLEARN_BIT_SKILL_IS_MAX_LEVEL,	
	SKILLLEARN_BIT_WRONG_STYLE,			
	SKILLLEARN_BIT_WRONG_UNITTYPE,		
	SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL,		
	SKILLLEARN_BIT_ERROR,				
	SKILLLEARN_BIT_STATS_ARE_TOO_LOW,	
	SKILLLEARN_BIT_SUBSCRIPTION_REQUIRED,
	SKILLLEARN_BIT_INSUFFICIENT_PERK_POINTS,
	SKILLLEARN_BIT_SKILL_IS_LOCKED,
// the ones below here are only used by the UI
	SKILLLEARN_BIT_NAME,					
	SKILLLEARN_BIT_DESCRIPTION,			
	SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT,	
	SKILLLEARN_BIT_NEXT_LEVEL_EFFECT,	
	SKILLLEARN_BIT_USE_INSTRUCTIONS,		
	SKILLLEARN_BIT_SKILL_BONUSES,
	SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT_ACCUMULATION,
	SKILLLEARN_BIT_SKILL_LEVEL_ENHANCED_BY_ITEMS
};

DWORD SkillGetNextLevelFlags(
	UNIT * pUnit,
	int nSkill );

BOOL SkillPurchaseTier(
						UNIT * pUnit,
						int nSkill );

BOOL CraftingPurchaseTier(
					   UNIT * pUnit,
					   int nSkill );

enum SKILL_REPEC_FLAGS
{
	SRF_CHARGE_CURRENCY_BIT,
	SRF_USE_RESPEC_STAT_BIT,
	SRF_CRAFTING_BIT,
	SRF_PERKS_BIT,
	NUM_SRF_BITS,

	SRF_CHARGE_CURRENCY_MASK	= MAKE_MASK(SRF_CHARGE_CURRENCY_BIT),
	SRF_USE_RESPEC_STAT_MASK	= MAKE_MASK(SRF_USE_RESPEC_STAT_BIT),
	SRF_CRAFTING_MASK			= MAKE_MASK(SRF_CRAFTING_BIT),
	SRF_PERKS_MASK				= MAKE_MASK(SRF_PERKS_BIT),
};

BOOL SkillRespec( 
	UNIT * pUnit,
	DWORD dwFlags = 0);

BOOL SkillPurchaseLevel(
	UNIT * pUnit,
	int nSkill,
	BOOL bCostsSkillPoints = TRUE,
	BOOL bForce = FALSE );

int SkillGetMaxLevelForUI ( 
	UNIT * pUnit,
	int nSkill);

BOOL SkillUnlearn(
	UNIT * pUnit,
	int nSkill );

BOOL SkillTestLineOfSight(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData,
	UNIT * pWeapon,
	UNIT * pUnitTarget,
	VECTOR * pvTarget );

DWORD SkillGetCRC(
	const VECTOR & vPosition,
	const VECTOR & vWeaponPosition,
	const VECTOR & vWeaponDirection);

float SkillGetDuration( 
						GAME * pGame, 
						UNIT * pUnit,
						int nSkill,
						BOOL bSkipWarmup,
						UNIT * pWeapon );

void SkillNewWeaponIsEquipped(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon );

BOOL c_SkillGetIconInfo( 
	UNIT * pUnit, 
	int nSkill,
	BOOL & bIconIsRed,
	float & fCooldownScale,
	int & nTicksRemaining,
	BOOL bCheckFallbacks );

int SkillGetNumOn(
	UNIT * pUnit,
	BOOL bCheckAI = FALSE );

BOOL SkillGetIsOn(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	BOOL bIgnoreWeapon = FALSE);

BOOL SkillIsOnWithFlag(
	UNIT * pUnit,
	int nSkillFlag );

enum
{
	SIO_SUCCESSFUL_BIT,
};

void SkillIsOnSetFlag(
	UNIT * pUnit,
	int nFlag,
	BOOL bSet,
	int nSkill,
	UNITID idWeapon);

BOOL SkillIsOnTestFlag(
	UNIT * pUnit,
	int nFlag,
	int nSkill,
	UNITID idWeapon);

void SkillsGetSkillsOn(
	UNIT * pUnit, 
	int *pnSkills,
	int nBufLen);

void SkillUpdateActiveSkills( 
	UNIT * pUnit );

void SkillUpdateActiveSkillsDelayed(
	UNIT *pUnit);

int UnitGetTicksSinceLastAggressiveSkill(
	GAME * pGame,
	UNIT * pUnit);

void SkillsSortWeapons(
	 GAME * pGame,
	 UNIT * pUnit,
	 UNITID piWeapons[ MAX_WEAPONS_PER_UNIT ] );

int SkillDataGetStateDuration(
	GAME * game,
	UNIT * unit,
	int skill,
	int sklvl,
	BOOL bApplyBonus);

int SkillGetCooldown( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nSkill,
	const SKILL_DATA * pSkillData,
	int nMinTicks,
	int nSkillLevel = -1 );

int UnitGetMeleeSpeed( 
	UNIT * pUnit );

BOOL c_SkillUnitShouldRunForward(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon,
	const VECTOR & vForward,
	BOOL bCheckRunToRangeFlag );

void UnitClearAllSkillCooldowns(
	GAME * pGame,
	UNIT * pUnit);

void UnitChangeAllSkillCooldowns(
	GAME * pGame,
	UNIT * pUnit,
	int	   nDelta);

BOOL SkillIsCooling(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon );

BOOL SkillIsRequested( 
	UNIT * pUnit, 
	int nSkill );

BOOL SkillTakePowerCost(
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData);

BOOL ExcelSkillsFuncPostProcess(
	struct EXCEL_TABLE * table);

void SkillGetAccuracy(
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill,
	const UNIT_DATA * pMissileData,
	float fHorizontalSpread,
	float & fVerticalRange,
	float & fHorizontalRange );

void SkillsUpdateMovingFiringError(
	UNIT * unit,
	float fDistanceMoved );

void SkillsUnregisterTimedEventsForUnitDeactivate(
	UNIT * pUnit);

BOOL SkillGetWeaponsForAttack(
	const SKILL_EVENT_FUNCTION_DATA &tData,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ]);

//----------------------------------------------------------------------------
// ACCESSOR FUNCTIONS
//----------------------------------------------------------------------------
inline const SKILL_DATA * SkillGetData(
	GAME * game,
	int nSkill)
{
	return (const SKILL_DATA *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_SKILLS, nSkill);
}

inline unsigned int SkillsGetCount(
	GAME * game)
{
	return ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_SKILLS);
}

inline SKILL_EVENT_TYPE_DATA * SkillGetEventTypeData(
	GAME * pGame,
	int nType)
{
	if ( nType == INVALID_ID )
		return NULL;
	return (SKILL_EVENT_TYPE_DATA *)ExcelGetData(pGame, DATATABLE_SKILLEVENTTYPES, nType);
}


inline BOOL SkillDataTestFlag(
	const SKILL_DATA * pSkillData,
	int nFlag )
{
	if ( ! pSkillData )
		return FALSE;
	if ( TESTBIT( pSkillData->dwFlags, nFlag ) )
		return TRUE;// this helps with debugging
	else
		return FALSE;
}

inline void SkillDataSetFlag(
	SKILL_DATA * pSkillData,
	int nFlag,
	BOOL bValue )
{
	if ( pSkillData )
		SETBIT( pSkillData->dwFlags, nFlag, bValue );
}

int SkillGetVariable( UNIT *pUnit,
					  UNIT *pObject,
					  const SKILL_DATA * pSkillData,
					  SKILL_VARIABLES skillVariable,
					  int nSkillLevelOverride = -1 );

int SkillGetVariable( UNIT *pUnit,
					  UNIT *pObject,
					  int nSkill,
					  SKILL_VARIABLES skillVariable,
					  int nSkillLevelOverride = -1  );

STATS * SkillCreateSkillEventStats(
	GAME * game,
	UNIT * unit,
	int skill,
	const SKILL_DATA * skill_data,
	int skillLevel,
	UNIT *pObjectTarget = NULL );

void SkillTransferStatsToUnit(
	UNIT *pUnit,
	UNIT *pGiver,
	int nSkill,
	int nSkillLevel,
	int nSelector = 0 );

void SkillDataFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata);

BOOL ExcelSkillsPostProcessAll( 
	struct EXCEL_TABLE * table);

inline const SKILL_LEVELS * SkillGetSkillLevelData( int nLevel )
{
	return (const SKILL_LEVELS *)ExcelGetData(NULL, DATATABLE_SKILL_LEVELS, nLevel);
}

inline int SkillGetSkillLevelValue( int nLevel, ESKILL_LEVEL_DEFUALTS eSkillMult )
{
	const SKILL_LEVELS *skillLevel =  SkillGetSkillLevelData( nLevel );
	ASSERTX_RETZERO( skillLevel, "Skill level not found" );
	return skillLevel->nSkillLevelValues[ eSkillMult ];
}

int SkillGetBonusSkillValue( UNIT *pUnit, const SKILL_DATA *pSkillData, int nBonusIndex );

void SkillExecuteScript( SKILL_SCRIPT_ENUM scriptToExecute, 
						 UNIT *pUnit, 
						 int nSkill, 
						 int nSkillLevel = INVALID_SKILL_LEVEL,
						 UNIT *pObject = NULL,  
						 STATS *stats = NULL, 
						 int param1 = INVALID_ID, 
						 int param2 = INVALID_ID, 								 								  
						 int nStateToPass = INVALID_ID,
						 int nItemClassExecutedSkill = INVALID_ID );

void SkillInitRand( RAND &tRand, GAME * pGame, UNIT * pUnit, UNIT * pWeapon, int nSkill );

int GetSkillStat( int nSkillStatID, UNIT *pUnit,  int nSkillLvl = INVALID_SKILL_LEVEL );

BOOL SkillTierCanBePurchased( UNIT *pUnit,
							  int nSkillTab,
							  BOOL bCostsSkillPoints,
							  BOOL bForce );

BOOL CraftingTierCanBePurchased( UNIT *pUnit,
							 int nSkillTab,
							 BOOL bCostsSkillPoints,
							 BOOL bForce );

BOOL SkillUsesPerkPoints(
	const SKILL_DATA * pSkillData);

int SkillGetPerkPointsRequiredForNextLevel(
	const SKILL_DATA * pSkillData,
	int nCurrentLevel);

int SkillGetTotalPerkPointCostForLevel(
	const SKILL_DATA * pSkillData,
	int nLevel);

BOOL SkillIsInSkillGroup(
	const SKILL_DATA * pSkillData,
	int nSkillGroup);

#endif // _SKILLS_H_

