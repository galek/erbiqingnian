
//----------------------------------------------------------------------------
// unit_priv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNIT_PRIV_H_
#define _UNIT_PRIV_H_

#include "../data_common/excel/damagetypes_hdr.h"

#ifndef _AFFIX_H_
#include "affix.h"
#endif

#ifndef _MISSILES_PRIV_H_
#include "missiles_priv.h"
#endif

#ifndef __SPECIAL_EFFECT_H_
#include "specialeffect.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif

#ifndef __LOD__
#include "lod.h"	
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define UNIT_HIT_SOUND_COUNT			6
#define NUM_SKILL_TABS					7
#define MAX_SPELL_SLOTS_PER_TIER		3
#define NUM_CONTAINER_UNITTYPES			4
#define MAX_REQ_AFFIX_GROUPS			8
#define MAX_DAMAGE_TYPES				256		// pure maximum number of damage types (NUM_DAMAGE_TYPES should never exceed this)
#define NUM_INIT_STATES					4
#define NUM_INIT_SKILLS					4
#define MAX_PAPERDOLL_WEAPONS			2
#define MAX_UNIT_TEXTURE_OVERRIDES		5
#define MAX_TRIGGER_PROHIBITED_STATES	1		// arbitrary, increase as needed
#define MAX_RMT_BADGES					20		// arbitrary, increase as needed 
#define MAX_RECIPE_SELL_UNITTYPES		3		// for selling recipes
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
typedef UNIT* (*PFN_PICKUP_FUNCTION)(GAME * pGame, UNIT * unit, UNIT * item);
typedef BOOL (*PFN_USE_FUNCTION)(GAME * pGame, UNIT * unit, UNIT * item);

//----------------------------------------------------------------------------
struct UNITMODE_VELOCITIES
{
	float					fVelocityBase;
	float					fVelocityMin;
	float					fVelocityMax;
};

//----------------------------------------------------------------------------
struct PARTICLE_ELEMENT_EFFECT
{
	EXCEL_STR				(szPerElement);						// name of particle system
	int						nPerElementId;						// id of particle system, linked at run time
};

//----------------------------------------------------------------------------
struct PARTICLE_EFFECT_SET
{
	EXCEL_STR				(szDefault);
	int						nDefaultId;
	PARTICLE_ELEMENT_EFFECT	tElementEffect[NUM_DAMAGE_TYPES];
	BOOL					bInitialized;		// links to ids have been set at run time
};


struct APPEARANCE_SHAPE_CREATE
{
	BOOL bHasShape;

	// group 1 
	BYTE bHeightMin;
	BYTE bHeightMax;
	BYTE bWeightMin;
	BYTE bWeightMax;

	// group 2 - with line bounds
	BOOL bUseLineBounds;
	BYTE bWeightLineMin0;
	BYTE bWeightLineMax0;
	BYTE bWeightLineMin1;
	BYTE bWeightLineMax1;
};

enum RMT_TANGIBILITY
{
	RMT_TANGIBLE_NONE,
	RMT_TANGIBLE_ACCOUNTWIDE,
	RMT_TANGIBLE_ITEM,
};

enum RMT_PRICING
{
	RMT_PRICING_NONE,
	RMT_PRICING_ONETIME,
	RMT_PRICING_RECURRING,
};

//	DO NOT ALTER ENUM W/O MATCHING DATABASE UPDATE.
enum
{
	UNIT_TEXTURE_OVERRIDE_DIFFUSE = 0,
	UNIT_TEXTURE_OVERRIDE_NORMAL,
	UNIT_TEXTURE_OVERRIDE_SPECULAR,
	UNIT_TEXTURE_OVERRIDE_SELFILLUM,
	UNIT_TEXTURE_OVERRIDE_COUNT,
};

enum
{
	UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON,
	UNIT_WARDROBE_APPEARANCE_GROUP_FIRSTPERSON,
	NUM_UNIT_WARDROBE_APPEARANCE_GROUPS
};

//----------------------------------------------------------------------------
enum BOUNCE_FLAGS
{
	BF_BOUNCE_ON_HIT_UNIT_BIT,
	BF_BOUNCE_ON_HIT_BACKGROUND_BIT,
	BF_BOUNCE_NEW_DIRECTION_BIT,
	BF_CANNOT_RICOCHET_BIT,
	BF_BOUNCE_RETARGET_BIT,
};

enum UNIT_DATA_FLAGS
{
	UNIT_DATA_FLAG_SPAWN = 0,
	UNIT_DATA_FLAG_SPAWN_AT_MERCHANT,
	UNIT_DATA_FLAG_FORCE_IGNORES_SCALE,
	UNIT_DATA_FLAG_ADD_IMPACT_ON_FUSE,
	UNIT_DATA_FLAG_ADD_IMPACT_ON_FREE,
	UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_UNIT,
	UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_BACKGROUND,
	UNIT_DATA_FLAG_HAVOK_IMPACT_IGNORES_DIRECTION,
	UNIT_DATA_FLAG_DAMAGE_ONLY_ON_FUSE,
	UNIT_DATA_FLAG_HITS_UNITS,
	UNIT_DATA_FLAG_KILL_ON_UNIT_HIT,
	UNIT_DATA_FLAG_HITS_BACKGROUND,
	UNIT_DATA_FLAG_NO_RAY_COLLISION,
	UNIT_DATA_FLAG_KILL_ON_BACKGROUND_HIT,
	UNIT_DATA_FLAG_STICK_ON_HIT,
	UNIT_DATA_FLAG_STICK_ON_INIT,
	UNIT_DATA_FLAG_SYNC,
	UNIT_DATA_FLAG_CLIENT_ONLY,
	UNIT_DATA_FLAG_SERVER_ONLY,
	UNIT_DATA_FLAG_USE_SOURCE_VELOCITY,
	UNIT_DATA_FLAG_MUST_HIT,
	UNIT_DATA_FLAG_PRIORITIZE_TARGET,
	UNIT_DATA_FLAG_TRAIL_EFFECTS_USE_PROJECTILE,
	UNIT_DATA_FLAG_IMPACT_EFFECTS_USE_PROJECTILE,
	UNIT_DATA_FLAG_DESTROY_OTHER_MISSILES,
	UNIT_DATA_FLAG_DONT_HIT_SKILL_TARGET,
	UNIT_DATA_FLAG_FLIP_FACE_DIRECTION,
	UNIT_DATA_FLAG_DONT_USE_RANGE_FOR_SKILL,
	UNIT_DATA_FLAG_PULLS_TARGET,
	UNIT_DATA_FLAG_DAMAGES_ON_HIT_UNIT,
	UNIT_DATA_FLAG_PULSES_STATS_ON_HIT_UNIT,
	UNIT_DATA_FLAG_DAMAGES_ON_HIT_BACKGROUND,
	UNIT_DATA_FLAG_ALWAYS_CHECK_FOR_COLLISIONS,
	UNIT_DATA_FLAG_SET_SHAPE_PERCENTAGES,  
	UNIT_DATA_FLAG_USE_SOURCE_APPEARANCE,  // missiles
	UNIT_DATA_FLAG_DONT_TRANSFER_RIDERS_FROM_OWNER,  // missiles
	UNIT_DATA_FLAG_DONT_TRANSFER_DAMAGES_ON_CLIENT,  // missiles
	UNIT_DATA_FLAG_MISSILE_IGNORE_POSTLAUNCH, //missiles
	UNIT_DATA_FLAG_MISSILE_USE_ULTIMATEOWNER, //missiles
	UNIT_DATA_FLAG_ATTACKS_LOCATION_ON_HIT_UNIT,  // missiles
	UNIT_DATA_FLAG_DONT_DEACTIVATE_WITH_ROOM,  // monsters
	UNIT_DATA_FLAG_ANGER_OTHERS_ON_DAMAGED,  // monsters
	UNIT_DATA_FLAG_ANGER_OTHERS_ON_DEATH,  // monsters
	UNIT_DATA_FLAG_ALWAYS_FACE_SKILL_TARGET,  // monsters
	UNIT_DATA_FLAG_SET_ROPE_END_WITH_NO_TARGET,  // missiles
	UNIT_DATA_FLAG_FORCE_DRAW_TO_MATCH_VELOCITY, // missiles
	UNIT_DATA_FLAG_USE_QUEST_NAME_COLOR,			// mostly items... mostly
	UNIT_DATA_FLAG_DONT_SORT_WEAPONS,	
	UNIT_DATA_FLAG_IGNORES_EQUIP_CLASS_REQS,
	UNIT_DATA_FLAG_DONT_USE_SOURCE_FOR_TOHIT,
	UNIT_DATA_FLAG_ANGLE_WHILE_PATHING,
	UNIT_DATA_FLAG_DONT_ADD_WARDROBE_LAYER,
	UNIT_DATA_FLAG_DONT_USE_CONTAINER_APPEARANCE,
	UNIT_DATA_FLAG_SUBSCRIBER_ONLY,
	UNIT_DATA_FLAG_COMPUTE_LEVEL_REQUIREMENT,
	UNIT_DATA_FLAG_DONT_FATTEN_COLLISION_FOR_SELECTION,
	UNIT_DATA_FLAG_AUTOMAP_SAVE, //  automap
	UNIT_DATA_FLAG_REQUIRES_CAN_OPERATE_TO_BE_KNOWN, // object must be able to be "operated" by client in order to send them the object at all
	UNIT_DATA_FLAG_FORCE_FREE_ON_ROOM_RESET,	// this unit class must be freed when a room is reset
	UNIT_DATA_FLAG_CAN_REFLECT,	// this missile can be reflected back at the owner
	UNIT_DATA_FLAG_SELECT_TARGET_IGNORES_AIM_POS,
	UNIT_DATA_FLAG_CAN_MELEE_ABOVE_HEIGHT,
	UNIT_DATA_FLAG_QUEST_FLAVOR_TEXT,				// quests
	UNIT_DATA_FLAG_UNIDENTIFIED_NAME,				// item
	UNIT_DATA_FLAG_NO_RANDOM_PROPER_NAME,			// monsters
	UNIT_DATA_FLAG_NO_NAME_MODIFICATIONS,			// monsters
	UNIT_DATA_FLAG_PRELOAD,							// monsters, objects
	UNIT_DATA_FLAG_IGNORE_IN_DAT,					// monsters, objects
	UNIT_DATA_FLAG_IGNORE_SAVED_STATES,				// items
	UNIT_DATA_FLAG_DRAW_USING_CUT_UP_WARDROBE,		// items
	UNIT_DATA_FLAG_IS_GOOD,							// monsters
	UNIT_DATA_FLAG_IS_NPC,							// monsters
	UNIT_DATA_FLAG_CANNOT_BE_MOVED,					// monster, objects
	UNIT_DATA_FLAG_NO_LEVEL,						// items
	UNIT_DATA_FLAG_USES_SKILLS,
	UNIT_DATA_FLAG_AUTO_PICKUP,						// items; currently not used
	UNIT_DATA_FLAG_TRIGGER,							// objects
	UNIT_DATA_FLAG_DIE_ON_CLIENT_TRIGGER,			// objects
	UNIT_DATA_FLAG_NEVER_DESTROY_DEAD,				// monster, objects
	UNIT_DATA_FLAG_COLLIDE_WHEN_DEAD,				// monsters, objects
	UNIT_DATA_FLAG_START_DEAD,						// monsters
	UNIT_DATA_FLAG_GIVES_LOOT,						// objects
	UNIT_DATA_FLAG_DONT_TRIGGER_BY_PROXIMITY,		// objects
	UNIT_DATA_FLAG_TRIGGER_ON_ENTER_ROOM,			// objects
	UNIT_DATA_FLAG_DESTRUCTIBLE,					// objects, monsters
	UNIT_DATA_FLAG_SPAWN_IN_AIR,					// monsters
	UNIT_DATA_FLAG_WALL_WALK,						// monsters
	UNIT_DATA_FLAG_START_IN_TOWN_IDLE,				// monsters
	UNIT_DATA_FLAG_ON_DIE_DESTROY,					// monsters
	UNIT_DATA_FLAG_ON_DIE_END_DESTROY,				// monsters
	UNIT_DATA_FLAG_ON_DIE_HIDE_MODEL,				// monsters
	UNIT_DATA_FLAG_SELECTABLE_DEAD,					// monsters
	UNIT_DATA_FLAG_INTERACTIVE,						// monsters
	UNIT_DATA_FLAG_HIDE_DIALOG_HEAD,				// monsters
	UNIT_DATA_FLAG_COLLIDE_BAD,						// monsters
	UNIT_DATA_FLAG_COLLIDE_GOOD,					// monsters
	UNIT_DATA_FLAG_MODES_IGNORE_AI,					// monsters
	UNIT_DATA_FLAG_DONT_PATH,						// monsters
	UNIT_DATA_FLAG_SNAP_TO_PATH_NODE_ON_CREATE,		// monsters
	UNIT_DATA_FLAG_UNTARGETABLE,					// monsters
	UNIT_DATA_FLAG_FACE_DURING_INTERACTION,			// monsters
	UNIT_DATA_FLAG_DONT_SYNCH,						// monsters
	UNIT_DATA_FLAG_CANNOT_TURN,						// monsters
	UNIT_DATA_FLAG_TURN_NECK_INSTEAD_OF_BODY,		// monsters
	UNIT_DATA_FLAG_IS_MERCHANT,						// monsters
	UNIT_DATA_FLAG_MERCHANT_SHARES_INVENTORY,		// monsters
	UNIT_DATA_FLAG_IS_TRADER,						// monsters
	UNIT_DATA_FLAG_IS_TRADESMAN,					// monsters
	UNIT_DATA_FLAG_IS_GAMBLER,						// monsters
	UNIT_DATA_FLAG_IS_STATTRADER,					// monsters
	UNIT_DATA_FLAG_IS_MAP_VENDOR,					// monsters
	UNIT_DATA_FLAG_IS_GOD_MESSENGER,				// monsters
	UNIT_DATA_FLAG_IS_TRAINER,						// monsters
	UNIT_DATA_FLAG_IS_HEALER,						// monsters
	UNIT_DATA_FLAG_IS_GRAVEKEEPER,					// monsters
	UNIT_DATA_FLAG_IS_TASKGIVER,					// monsters
	UNIT_DATA_FLAG_ITEM_UPGRADER,					// monsters
	UNIT_DATA_FLAG_ITEM_AUGMENTER,					// monsters
	UNIT_DATA_FLAG_AUTO_IDENTIFIES_INVENTORY,		// monsters
	UNIT_DATA_FLAG_IS_DUNGEON_WARPER,				// monsters
	UNIT_DATA_FLAG_IS_PVP_SIGNER_UPPER,				// monsters
	UNIT_DATA_FLAG_IS_FOREMAN,						// monsters
	UNIT_DATA_FLAG_IS_TRANSPORTER,					// monsters
	UNIT_DATA_FLAG_SHOWS_PORTRAIT,					// monsters
	UNIT_DATA_FLAG_PET_GETS_STAT_POINTS_PER_LEVEL,	// monsters
	UNIT_DATA_FLAG_IGNORES_POWER_COST_FOR_SKILLS,	// monsters
	UNIT_DATA_FLAG_CHECK_RADIUS_WHEN_PATHING,		// monsters
	UNIT_DATA_FLAG_CHECK_HEIGHT_WHEN_PATHING,		// monsters
	UNIT_DATA_FLAG_QUEST_IMPORTANT_INFO,			// objects
	UNIT_DATA_FLAG_IGNORES_TO_HIT,					// any
	UNIT_DATA_FLAG_ASK_QUESTS_FOR_OPERATE,			// any
	UNIT_DATA_FLAG_ASK_FACTION_FOR_OPERATE,			// objects
	UNIT_DATA_FLAG_ASK_PVP_CENSORSHIP_FOR_OPERATE,	// objects
	UNIT_DATA_FLAG_STRUCTURAL,						// any
	UNIT_DATA_FLAG_ASK_QUESTS_FOR_KNOWN,			// any
	UNIT_DATA_FLAG_ASK_QUESTS_FOR_VISIBLE,			// any
	UNIT_DATA_FLAG_INFORM_QUESTS_ON_INIT,			// any
	UNIT_DATA_FLAG_INFORM_QUESTS_OF_LOOT_DROP,		// any
	UNIT_DATA_FLAG_INFORM_QUESTS_ON_DEATH,			// monsters
	UNIT_DATA_FLAG_NO_TRADE,						// items
	UNIT_DATA_FLAG_ROOM_NO_SPAWN,					// objects
	UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH,			// monsters, objects
	UNIT_DATA_FLAG_MONITOR_APPROACH_CLEAR_LOS,		// monsters
	UNIT_DATA_FLAG_CAN_FIZZLE,						// missiles
	UNIT_DATA_FLAG_INHERITS_DIRECTION,				// missiles
	UNIT_DATA_FLAG_CANNOT_BE_DISMANTLED,			// items
	UNIT_DATA_FLAG_CANNOT_BE_UPGRADED,				// items
	UNIT_DATA_FLAG_CANNOT_BE_AUGMENTED,				// items
	UNIT_DATA_FLAG_CANNOT_BE_DEMODDED,				// items
	UNIT_DATA_FLAG_IGNORE_SELL_WITH_INVENTORY_CONFIRM,	// items
	UNIT_DATA_FLAG_WARDROBE_PER_UNIT,				// everything
	UNIT_DATA_FLAG_WARDROBE_SHARES_MODEL_DEF,		// everything
	UNIT_DATA_FLAG_NO_WEAPON_MODEL,					// items
	UNIT_DATA_FLAG_PICKUP_REQUIRES_INVENTORY_SPACE,	// items
	UNIT_DATA_FLAG_NO_DROP,							// items
	UNIT_DATA_FLAG_NO_DROP_EXCEPT_FOR_DUPLICATES,	// items
	UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP,			// items
	UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP,			// items
	UNIT_DATA_FLAG_EXAMINABLE,						// items
	UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE,			// items
	UNIT_DATA_FLAG_CONSUMED_WHEN_USED,				// items
	UNIT_DATA_FLAG_ALWAYS_DO_TRANSACTION_LOGGING,	// items (force player transaction logging)
	UNIT_DATA_FLAG_IMMUNE_TO_CRITICAL,				// monsters (destructible)
	UNIT_DATA_FLAG_NO_RANDOM_AFFIXES,				// monsters, items
	UNIT_DATA_FLAG_CAN_BE_CHAMPION,					// monsters
	UNIT_DATA_FLAG_NO_QUALITY_DOWNGRADE,			// items
	UNIT_DATA_FLAG_NO_DRAW_ON_INIT,					// objects
	UNIT_DATA_FLAG_MUST_FACE_MELEE_TARGET,			// monsters, players
	UNIT_DATA_FLAG_DO_NOT_DESTROY_IF_VELOCITY_ZERO,	// missiles, players and monsters
	UNIT_DATA_FLAG_IGNORE_INTERACT_DISTANCE, 		// objects
	UNIT_DATA_FLAG_OPERATE_REQUIRES_GOOD_QUEST_STATUS,	// objects
	UNIT_DATA_FLAG_REVERSE_ARRIVE_DIRECTION,		// objects
	UNIT_DATA_FLAG_FACE_AFTER_WARP,					// objects
	UNIT_DATA_FLAG_NEVER_A_START_LOCATION,			// objects
	UNIT_DATA_FLAG_ALWAYS_SHOW_LABEL,				// anything
	UNIT_DATA_FLAG_INITIALIZED,						// set by code
	UNIT_DATA_FLAG_LOADED,							// set by code
	UNIT_DATA_FLAG_SOUNDS_FLAGGED,					// set by code
	UNIT_DATA_FLAG_IS_NONWEAPON_MISSILE,			//missiles
	UNIT_DATA_FLAG_CULL_BY_SCREENSIZE,
	UNIT_DATA_FLAG_LINK_WARP_DEST_BY_LEVEL_TYPE,	// objects
	UNIT_DATA_FLAG_IS_BOSS,							// monsters (can be marked as bosses independent of anything else)
	UNIT_DATA_FLAG_SINGLE_PLAYER_ONLY,				// monsters
	UNIT_DATA_FLAG_TAKE_RESPONSIBILITY_ON_KILL,		// monsters
	UNIT_DATA_FLAG_ALWAYS_KNOWN_FOR_SOUNDS,			// monsters
	UNIT_DATA_FLAG_IGNORE_AI_TARGET_ON_REPEAT_DMG,	// missiles
	UNIT_DATA_FLAG_BIND_TO_LEVELAREA,				// items
	UNIT_DATA_FLAG_DONT_COLLIDE_DESTRUCTIBLES,		// monsters
	UNIT_DATA_FLAG_BLOCKS_EVERYTHING,				// monsters
	UNIT_DATA_FLAG_EVERYONE_CAN_TARGET,				// monsters
	UNIT_DATA_FLAG_MISSILE_PLOT_ARC,				// missiles
	UNIT_DATA_FLAG_PET_ON_WARP_DESTROY,				// monsters
	UNIT_DATA_FLAG_MISSILE_IS_GORE,					// missiles
	UNIT_DATA_FLAG_CAN_ATTACK_FRIENDS,				// missiles,monsters
	UNIT_DATA_FLAG_IGNORES_ITEM_REQUIREMENTS,		// monsters
	UNIT_DATA_FLAG_LOW_LOD_IN_TOWN,					// monsters,players
	UNIT_DATA_FLAG_LEVELTREASURE_SPAWN_BEFORE_UNIT,	// objects
	UNIT_DATA_FLAG_IS_TASKGIVER_NO_STARTING_ICON,	// monsters
	UNIT_DATA_FLAG_ASSIGN_GUID,						// assign a GUID to this unit
	UNIT_DATA_FLAG_MERCHANT_NO_REFRESH,				// monsters
	UNIT_DATA_FLAG_DONT_DEPOPULATE,					// monsters
	UNIT_DATA_FLAG_DONT_SHRINK_BONES,				// monsters
	UNIT_DATA_FLAG_DISMISS_PET,						// monsters
	UNIT_DATA_FLAG_QUESTS_CAST_MEMBER,				// monsters (ask about quest info updates)
	UNIT_DATA_FLAG_MULTIPLAYER_ONLY,				// monsters
	UNIT_DATA_FLAG_NOSPIN,							// items
	UNIT_DATA_FLAG_IS_GUILDMASTER,					// monsters
	UNIT_DATA_FLAG_IDENTIFY_ON_CREATE,				// items
	UNIT_DATA_FLAG_IS_RESPECCER,					// monsters
	UNIT_DATA_FLAG_IS_CRAFTING_RESPECCER,			// monsters
	UNIT_DATA_FLAG_ALLOW_OBJECT_STEPPING,			// objects
	UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK,	// monsters
	UNIT_DATA_FLAG_NO_RANDOM_LEVEL_TREASURE,		// monsters
	UNIT_DATA_FLAG_XFER_MISSILE_STATS,				// missiles
	UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY,			// items (differentiate between items spawned in different difficulties)
	UNIT_DATA_FLAG_IS_FIELD_MISSILE,				// missiles
	UNIT_DATA_FLAG_IGNORE_FUSE_MS,					// missiles
	UNIT_DATA_FLAG_SET_OBJECT_TO_LEVEL_EXPERIENCE,
	UNIT_DATA_FLAG_USES_PETLEVEL,					// monsters
	UNIT_DATA_FLAG_ITEM_IS_TARGETED,				// items
	UNIT_DATA_FLAG_LOADED_ICON,						// set by code
	UNIT_DATA_FLAG_LOADED_SOUNDS,					// set by code
	UNIT_DATA_FLAG_LOADED_APPEARANCE,				// set by code
	UNIT_DATA_FLAG_LOADED_SKILLS,					// set by code
	UNIT_DATA_FLAG_NO_GENDER_FOR_ICON,				// items
	UNIT_DATA_FLAG_FADE_ON_DESTROY,					// monsters
	UNIT_DATA_FLAG_HIDE_NAME,						// any
	UNIT_DATA_FLAG_IS_POINT_OF_INTEREST,			// monsters/objects
	UNIT_DATA_FLAG_IS_STATIC_POINT_OF_INTEREST,		// static point of interest( will always be known )
	UNIT_DATA_FLAG_CONFIRM_ON_USE,					// items
	UNIT_DATA_FLAG_FOLLOW_GROUND,					// missiles
	UNIT_DATA_FLAG_MIRROR_IN_LEFT_HAND,				// weapons
	NUM_UNIT_DATA_FLAGS,
};

//----------------------------------------------------------------------------
#define UNIT_INDEX_SIZE (DEFAULT_INDEX_SIZE + DEFAULT_INDEX_SIZE)
struct UNIT_DATA
{
	EXCEL_STR				(szName);									// key
	EXCEL_STR				(szFileFolder);								// monsters, players, items
	EXCEL_STR				(szAppearance);								// monsters, players, items
	EXCEL_STR				(szAppearance_FirstPerson);					// players
	EXCEL_STR				(szIcon);									// inventory icon
	EXCEL_STR				(szHolyRadius);								// players
	EXCEL_STR				(szBloodParticles[NUM_UNIT_COMBAT_RESULTS]);	// monsters, players
	EXCEL_STR				(szFizzleParticle);							// missiles
	EXCEL_STR				(szReflectParticle);						// missiles
	EXCEL_STR				(szRestoreVitalsParticle);					// players
	EXCEL_STR				(szTextureOverrides[UNIT_TEXTURE_OVERRIDE_COUNT]);	// monsters
	EXCEL_STR				(szTextureSpecificOverrides[MAX_UNIT_TEXTURE_OVERRIDES * 2]);	// monsters
	EXCEL_STR				(szParticleFolder);							// missiles
	EXCEL_STR				(szPickupFunction);							// items
	EXCEL_STR				(szUseFunction);							// items
	EXCEL_STR				(szTriggerString1);
	EXCEL_STR				(szTriggerString2);
	EXCEL_STR				(szCharSelectFont);							// players
	EXCEL_STR				(szDamageIcon);								// items (weapons)
	EXCEL_STR				(szDamagingMeleeParticle);					// monsters
	PARTICLE_EFFECT_SET		MissileEffects[NUM_MISSILE_EFFECTS];		// missiles
	int						nUnitDataBaseRow;							// row this row inherited its values from
	DWORD					pdwFlags[DWORD_FLAG_SIZE(NUM_UNIT_DATA_FLAGS)]; // please help move bool values into here. Save some memory
	
	int						nString;									// monsters, players, items
	int						nTypeDescrip;								// items
	int						nFlavorText;								// items
	int						nAddtlDescripString;						// players
	int						nAddtlRaceDescripString;					// players
	int						nAddtlRaceBonusDescripString;				// players
	int						nAnalyze;									// npcs(monsters)
	int						nRecipeList;								// npcs(monsters)
	int						nRecipeSingleUse;							// items
	int						nRecipeToSellByUnitType[MAX_RECIPE_SELL_UNITTYPES];					// npcs to sell recipes by unittype
	int						nRecipeToSellByUnitTypeNot[MAX_RECIPE_SELL_UNITTYPES];				// npcs to sell recipes by unittype
	int						nRecipePane;								// npcs(monsters)

	int						nMerchantNotAvailableTillQuest;				// npcs(merchants)

	int						nPaperdollBackgroundLevel;					// players
	int						pnPaperdollWeapons[ MAX_PAPERDOLL_WEAPONS ];// players
	int						nPaperdollSkill;							// players
	int						nPaperdollColorset;							// players

	int						nRespawnPercentChance;						// objects,monsters
	int						nRespawnSpawnClass;							// objects,monsters
	int						nRespawnMonsterClass;						// monsters
	float					fRespawnRadius;								// objects,monsters

	WORD					wCode;										// everything
	int						nVersion;									// everything - release, ongoing, expansion, etc...

	int						nDensityValueOverride;						// monsters
	PCODE					codeMinionPackSize;							// how many minions appear with the monster

	int						nRespawnPeriod;								// monsters, objects

	float					fSpinSpeed;
	float					fMaxTurnRate;
	int						nUnitType;									// players, monsters, items
	int						nUnitTypeForLeveling;						// players
	int						nPreferedByUnitType;						// items - which unittype would like this item to drop
	int						nFamily;									// monsters
	int						nCensorBackupClass_NoHumans;				// monsters
	int						nCensorBackupClass_NoGore;					// monsters
	GENDER					eGender;									// players, monsters
	int						nRace;										// players
	int						nRarity;									// monsters, items
	int						nSpawnPercentChance;						// objects
	int						nMinMonsterExperienceLevel;					// monsters
	int						nItemExperienceLevel;						// items
	int						nMonsterQualityRequired;					// monsters
	int						nMonsterClassAtUniqueQuality;				// monsters
	int						nMonsterClassAtChampionQuality;				// monsters
	int						nMinionClass;								// monsters
	int						nMonsterNameType;							// monsters
	int						nItemQualityRequired;						// items
	int						nAngerRange;								// monsters
	int						nBaseLevel;									// players, monsters, items
	int						nCapLevel;									// players, monsters, items
	int						nObjectDownGradeToObject;					// objects
	int						nMinMerchantLevel;							// items
	int						nMaxMerchantLevel;							// items
	int						nMinSpawnLevel;								// players, monsters, items
	int						nMaxSpawnLevel;								// players, monsters, items
	int						nMaxLevel;									// items, players
	int						nMaxRank;									// items, players
	PCODE					codeFixedLevel;								// items
	int						nMinHitPointPct;							// monsters
	int						nMaxHitPointPct;							// players, monsters
	int						nMaxPower;									// players, monsters, etc
	int						nExperiencePct;								// monsters
	int						nAttackRatingPct;							// monsters
	PCODE					codeLuckBonus;								// monsters
	int						nLuckChanceBonus;							// monsters
	ROOM_POPULATE_PASS		eRoomPopulatePass;							// monsters, objects
	int						nWeaponBoneIndex;							// items
	int						bRequireAffix;								// items
	float					flAutoPickupDistance;						// items
	int						nPickupPullState;							// items
	float					flExtraDyingTimeInSeconds;					// monsters
	int						nNPC;										// monsters
	int						nBalanceTestCount;							// monsters
	int						nBalanceTestGroup;							// monsters
	int						nMerchantStartingPane;						// monsters
	int						nMerchantFactionType;						// monsters
	int						nMerchantFactionValueNeeded;				// monsters
	int						nQuestRequirement;							// any
	float					fNoSpawnRadius;								// objects
	float					flMonitorApproachRadius;					// monsters
	int						nTasksGeneratedStat;						// monsters
	float					fServerMissileOffset;						// missiles
	float					flHomingTurnAngleInRadians;					// missiles
	float					flHomingTurnAngleInRadiansModByDis;			// missiles
	float					flHomingUnitScalePct;						// missiles
	float					flSecondsBeforeBecomingCollidable;			// missiles
	float					flHomingAfterXSeconds;						// missiles
	DWORD					dwBounceFlags;								// missiles (see BOUNCE_FLAGS)
	float					fImpactCameraShakeDuration;					// missiles
	float					fImpactCameraShakeMagnitude;				// missiles
	float					fImpactCameraShakeDegrade;					// missiles
	int						nMaximumImpactFrequency;					// missiles
	int						nOnlyCollideWithUnitType;					// missiles
	int						nQuestItemDescriptionID;					// items	
	PCODE					codePickupCondition;						// items
	PCODE					codeUseCondition;							// items
	PCODE					codeUseScript;								// items
	PCODE					codeStackSize;								// items	
	int						nMaxPickup;									// items
	int						nBaseCost;	
	int						nPVPPointCost;
	PCODE					codeBuyPriceMult;							// multiplier to buy price calculation
	PCODE					codeBuyPriceAdd;							// post-multiplier add to buy price calculation
	PCODE					codeSellPriceMult;							// multiplier to sell price calculation
	PCODE					codeSellPriceAdd;							// post-multiplier add to sell price calculation
	int						nInventoryWardrobeLayer;					// items
	int						nCharacterScreenWardrobeLayer;				// players
	int						nCharacterScreenState;						// players
	int						nWardrobeBody;								// monsters
	int						nWardrobeFallback;							// monsters
	int						nWardrobeFallbackId;						// wardrobe id for wardrobe fallback
	int						nWardrobeMipBonus;							// monsters

	int						pnWardrobeAppearanceGroup[ NUM_UNIT_WARDROBE_APPEARANCE_GROUPS ];// players, monsters

	int						nStartingStance;							// players, monsters
	
	SAFE_FUNC_PTR(PFN_PICKUP_FUNCTION, pfnPickup);									// items
	
	int						nSummonedLocation;							// monstrs
	int						nContainerUnitTypes[ NUM_CONTAINER_UNITTYPES ];	// items
	int						nFiringErrorIncrease;						// items and players
	int						nFiringErrorDecrease;						// items and players
	int						nFiringErrorMax;							// items and players
	int						nAccuracyBase;								// players
	int						m_nUseFuncInvLoc;							// items
	int						m_nUseParamState;							// items
	int						m_nUseParamState2;							// items
	float					m_flUseParam;								// items
	int						m_nRefillHotkeyInvLoc;						// items
	int						nAnimGroup;									// items
	int						nMeleeWeapon;								// items (weapons)
	int						nCooldown;									// items (weapons), monsters
	int						nDuration;									// items (potions)
	float					fEstDPS;									// items (weapons)
	int						nDamageDescripString;						// items (weapons)
	int						nReqAffixGroup[MAX_REQ_AFFIX_GROUPS];		// items

	int						nSpawnMonsterClass;							// monsters
	int						nStateSafe;									// any
	int						nSkillGhost;								// players
	int						nSkillRef;									// items
	int						nDamageType;								// monsters
	int						nWeaponDamageScale;							// monsters
	int						nDontUseWeaponDamage;						// monsters
	PCODE					codeMinBaseDmg;								// monsters, items
	PCODE					codeMaxBaseDmg;
	int						nSfxAttackPercentFocus;						// monsters, weapons
	SPECIAL_EFFECT			tSpecialEffect[ NUM_DAMAGE_TYPES ];			// items (weapons/armor)
	int						nDmgIncrement;								// items (weapons)
	int						nDmgIncrementRadial;						// items (weapons)
	int						nDmgIncrementField;							// items (weapons)
	int						nDmgIncrementDOT;							// items (weapons)
	int						nDmgIncrementAIChanger;						// items (weapons)
	int						nToHitBonus;								// items (weapons)
	int						nCriticalChance;							// items (weapons)
	int						nCriticalMultiplier;						// items (weapons)
	int						nStaminaDrainChance;						// monsters
	int						nStaminaDrain;								// monsters
	int						nInterruptAttackPct;						// monsters, items
	int						nInterruptDefensePct;						// monsters, items
	int						nInterruptChance;							// monsters
	int						nStealthDefensePct;							// monsters
	int						nStealthAttackPct;							// items
	int						nAIChangeDefense;							// monsters
	float					fArmor[NUM_DAMAGE_TYPES];					// monsters, items
	int						nArmorMax[NUM_DAMAGE_TYPES];				// monsters, items
	float					fShields[NUM_DAMAGE_TYPES];					// monsters, items
	int						nStrengthPercent;							// monsters (Tugboat)
	int						nDexterityPercent;							// monsters (Tugboat)
	int						nStartingAccuracy;							// players
	int						nStartingStrength;							// players
	int						nStartingStamina;							// players
	int						nStartingWillpower;							// players
	PCODE					codeRecipeProperty;							// items
	PCODE					codeProperties[NUM_UNIT_PROPERTIES];		// players, monsters, items
	PCODE					codeSkillExecute[NUM_UNIT_SKILL_SCRIPTS];	// players, monsters, items
	int						nCodePropertiesApplyToUnitType[NUM_UNIT_PROPERTIES];		// players, monsters, items
	PCODE					codePerLevelProperties[NUM_UNIT_PERLEVEL_PROPERTIES];	// players, monsters, items
	PCODE					codeEliteProperties;						// monsters
	int						nAffixes[MAX_AFFIX_ARRAY_PER_UNIT];					// monsters, items
	int						nTreasure;									// monsters
	int						nTreasureChampion;							// monsters
	int						nTreasureFirstTime;							// monsters
	int						nTreasureCrafting;							// monsters

	int						nInventory;									// players, monsters, items
	int						nCraftingInventory;							// items
	int						nInvLocRecipeIngredients;					// players, items
	int						nInvLocRecipeResults;						// players, items
	
	int						nStartingTreasure;							// players, monsters	
	int						nInvWidth;									// items
	int						nInvHeight;									// items
	DWORD					pdwQualities[DWORD_FLAG_SIZE(NUM_UNIT_QUALITIES)];		// items
	int						nRequiredQuality;							// items
	enum QUALITY_NAME		eQualityName;								// items
	int						nFieldMissile;								// items

	int						nSkillIdHitUnit;							// missiles
	int						nSkillIdHitBackground;						// missiles
	int						nSkillIdMissedArray[ NUM_SKILL_MISSED ];	// missiles
	int						nSkillIdMissedCount;						// missiles
	int						nSkillIdOnFuse;								// missiles
	int						nSkillIdOnDamageRepeat;						// missiles
	int						nStartingSkills[NUM_UNIT_START_SKILLS];		// players, monsters
	int						nUnitDieBeginSkill;							// monsters
	int						nUnitDieBeginSkillClient;					// monsters
	PCODE					codeScriptUnitDieBegin;						// monsters
	int						nUnitDieEndSkill;							// monsters
	int						nDeadSkill;									// monsters
	int						nKickSkill;									// players, monsters
	int						nRightSkill;								// players
	int						nInitSkill[NUM_INIT_SKILLS];				// monsters, objects, missiles
	int						nClassSkill;								// players
	int						nRaceSkill;									// players
	int						nSkillLevelActive;							// units, sort of like nInitSkill
	int						pnInitStates[ NUM_INIT_STATES ];			// monsters
	int						nInitStateTicks;							// monsters
	int						nDyingState;								// monsters
	int						nSkillWhenUsed;								// objects
	int						nSkillTab[NUM_SKILL_TABS];					// players

	int						m_nLevelUpState;							// players
	int						m_nRankUpState;								// players

	int						m_nSound;									// missiles
	int						m_nFlippySound;								// items
	int						m_nInvPickupSound;							// items
	int						m_nInvPutdownSound;							// items
	int						m_nInvEquipSound;							// items
	int						m_nPickupSound;								// items
	int						m_nInvUseSound;								// items
	int						m_nInvCantUseSound;							// items
	int						m_nCantFireSound;							// weapons
	int						m_nBlockSound;								// weapons - shields
	int						pnGetHitSounds[ UNIT_HIT_SOUND_COUNT ];		// players
	int						nInteractSound;								// NPCs
	int						nDamagingSound;								// items, monsters
	int						nFootstepForwardLeft;						// players
	int						nFootstepForwardRight;						// players
	int						nFootstepBackwardLeft;						// players
	int						nFootstepBackwardRight;						// players
	int						nFootstepJump1stPerson;						// players
	int						nFootstepLand1stPerson;						// players


	// Mythos player notification sounds
	int						m_nOutOfManaSound;							// players
	int						m_nInventoryFullSound;						// players
	int						m_nCantSound;								// players
	int						m_nCantUseSound;							// players
	int						m_nCantUseYetSound;							// players
	int						m_nCantCastSound;							// players
	int						m_nCantCastYetSound;						// players
	int						m_nCantCastHereSound;						// players
	int						m_nLockedSound;								// players


	float					fPathingCollisionRadius;					// monsters
	float					fCollisionRadius;							// monsters, players, objects, missiles
	float					fCollisionRadiusHorizontal;					// monsters, players, objects, missiles
	float					fCollisionHeight;							// monsters, players, objects
	float					fBlockingCollisionRadiusOverride;			// objects
	float					fWarpPlaneForwardMultiplier;				// objects
	float					flWarpOutDistance;							// objects
	int						nSnapAngleDegrees;							// any
	float					fVanityHeight;								// players
	float					fOffsetUp;									// monsters, players
	float					fScale;										// monsters & players
	float					fWeaponScale;								// monsters
	float					fScaleDelta;								// monsters
	float					fChampionScale;								// monsters
	float					fChampionScaleDelta;						// monsters
	float					fScaleMultiplier;							// monsters - mostly for censor replacements
	float					fRagdollForceMultiplier;					// monsters
	float					fMeleeImpactOffset;							// monsters & players
	float					fMeleeRangeMax;								// monsters
	float					fMeleeRangeDesired;							// monsters
	float					flMaxAutomapRadius;							// objects
	float					fFuse;										// monsters
	int						nClientMinimumLifetime;						// missiles
	float					fRangeBase;									// missiles
	float					fDesiredRangeMultiplier;					// missiles
	float					fForce;										// missiles
	float					fHorizontalAccuracy;						// missiles
	float					fVerticleAccuracy;							// missiles
	float					fHitBackup;									// missiles
	float					fJumpVelocity;								// missiles
	float					fVelocityForImpact;							// missiles
	float					fAcceleration;								// missiles
	float					fBounce;									// missiles
	float					fDampening;									// missiles
	float					fFriction;									// missiles
	float					fMissileArcHeight;							// missiles
	float					fMissileArcDelta;							// missiles
	float					fSpawnRandomizePercent;						// monsters
	int						nTicksToDestoryAfter;						// missiles
	UNITMODE_VELOCITIES		tVelocities[NUM_MODE_VELOCITIES];			// monsters, players
	float					fDeltaVelocityModify;						// monsters
	int						nRangeMinDeltaPercent;						// missiles
	int						nRangeMaxDeltaPercent;						// missiles
	int						nMonsterDecoy;								// players

	int						nHavokShape;								// missiles, items

	int						nMissileHitUnit;							// missiles
	int						nMissileHitBackground;						// missiles	
	int						nMissileOnFreeOrFuse;						// missiles
	int						nMissileOnMissed;							// missiles
	int						nMissileTag;								// missiles
	int						nDamageRepeatRate;							// missiles
	int						nDamageRepeatChance;						// missiles
	BOOL					bRepeatDamageImmediately;					// missiles
	int						nServerSrcDamage;							// missiles
	int						bBlockGhosts;								// objects
	int						nMonsterToSpawn;							// objects
	int						m_nTriggerSound;							// objects
	int						nTriggerType;								// objects
	int						nOperatorStatesTriggerProhibited[ MAX_TRIGGER_PROHIBITED_STATES ];	// objects
	int						nSubLevelDest;								// objects
	
	int 					nQuestOperateRequired;						// objects
	int 					nQuestStateOperateRequired;					// objects
	int 					nQuestStateValueOperateRequired;			// objects
	int						nQuestStateValueOperateProhibited;			// objects

	int						nUnitTypeRequiredToOperate;					// objects
	int						nUnitTypeSpawnTreasureOnLevel;				// objects
	
	enum VISUAL_PORTAL_DIRECTION eOneWayVisualPortalDirection;			// objects
	enum WARP_RESOLVE_PASS	eWarpResolvePass;							// objects	

	float					flLabelScale;								// anything
	float					flLabelForwardOffset;						// anything	

	float					fHeightPercent;
	float					fWeightPercent;
	APPEARANCE_SHAPE_CREATE	tAppearanceShapeCreate;						// monsters
	int						nColorSet;
	int						nCorpseExplodePoints;						// monsters
	
	int						nRequiredAttackerUnitType;					// monsters

	int						nStartingLevelArea;							// player	
	
	int						nLinkToLevelArea;							// objects
	int						nLinkToLevelAreaFloor;						// objects
	int						nResidesInLevelArea;						// monsters and Objects

	int						nGlobalThemeRequired;						// monsters, items
	int						nLevelThemeRequired;						// items

	enum UNITMODE_GROUP		eModeGroupClient;							// monsters

	float					fCameraTarget[3];								// players
	float					fCameraEye[3];									// players

	// the following are RMT specific

	BOOL					bRMTAvailable;								// items
	int						nRealWorldCost;								// items
	int						nRMTBadgeIdArray[ MAX_RMT_BADGES ];			// list of badges awarded by this RMT item for accountwide benefits
	RMT_TANGIBILITY			nRMTTangibility;							// items
	RMT_PRICING				nRMTPricing;								// items

	// the following are set by code
	int						nAppearanceDefId;
	int						nAppearanceDefId_FirstPerson;
	int						nHolyRadiusId;
	int						pnTextureOverrideIds[ UNIT_TEXTURE_OVERRIDE_COUNT ][ LOD_COUNT ];	// CHB 2006.11.28
	int						pnTextureSpecificOverrideIds[ MAX_UNIT_TEXTURE_OVERRIDES ];
	int						nBloodParticleId[NUM_UNIT_COMBAT_RESULTS];
	int						nFizzleParticleId;
	int						nReflectParticleId;
	int						nRestoreVitalsParticleId;
	int						nDamagingMeleeParticleId;
	int						nIconTextureId;

	SAFE_PTR( int*, pnAchievements );
	USHORT					nNumAchievements;

//	char					cDifficulty;

	HAVOK_SHAPE_HOLDER		tHavokShapeFallback;
	HAVOK_SHAPE_HOLDER		tHavokShapePrefered;
};

//----------------------------------------------------------------------------
enum UNIT_CREATE_FLAGS
{
	UCF_CLIENT_ONLY =					MAKE_MASK(0),							// a client only unit
	UCF_DONT_DEACTIVATE_WITH_ROOM =		MAKE_MASK(1),							// don't deactivate this unit on room deactivate (still deactivate the unit on level deactivate)
	UCF_DONT_DEPOPULATE =				MAKE_MASK(2),							// don't deactivate this unit on room deactivate (still deactivate the unit on level deactivate)
	UCF_NO_DATABASE_OPERATIONS =		MAKE_MASK(3),							// unit cannot do database interactions (yet)
	UCF_RESTRICTED_TO_GUID =			MAKE_MASK(4),							// this unit is restricted to the specified GUID
	UCF_IS_CONTROL_UNIT	=				MAKE_MASK(5),
};

//----------------------------------------------------------------------------
struct UNIT_CREATE
{
	const UNIT_DATA *		pData;
	DWORD					dwUnitCreateFlags;	// see UNIT_CREATE_FLAGS
	int						nAppearanceDefId;
	UNITID					unitid;
	PGUID					guidUnit;
	GAMECLIENTID			idClient;
	SPECIES					species;
	ROOM*					pRoom;
	VECTOR					vPosition;
	VECTOR					vUpDirection;
	VECTOR					vFaceDirection;
	int						nAngle;
	float					fAcceleration;
	float					fScale;
	int						nQuality;
	PGUID					guidRestrictedTo;
	PGUID					guidOwner;
	const UNIT_DATA *		pSourceData;

	struct WARDROBE_INIT *  pWardrobeInit;
	struct APPEARANCE_SHAPE * pAppearanceShape;

#if (ISVERSION(DEBUG_ID))
	char					szDbgName[MAX_ID_DEBUG_NAME];
#endif

	UNIT_CREATE( const UNIT_DATA * pUnitData ) :
		dwUnitCreateFlags(0),
		nAppearanceDefId( pUnitData->nAppearanceDefId ),
		unitid(INVALID_ID),
		guidUnit(INVALID_GUID),
		idClient(INVALID_GAMECLIENTID),
		pRoom (NULL),
		vPosition(0.0f),
		vUpDirection(0.0f, 0.0f, 1.0f),
		vFaceDirection(1.0f, 0.0f, 0.0f),
		nAngle(0),
		fAcceleration(0.0f),
		fScale(1.0f),
		nQuality(-1),
		pData( pUnitData ),
		pWardrobeInit( NULL ),
		pAppearanceShape( NULL ),
		guidRestrictedTo( INVALID_GUID ),
		guidOwner( INVALID_GUID ),
		pSourceData( NULL )
	{}
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelPostProcessUnitDataInit(
	EXCEL_TABLE * table,
	BOOL bDelayLoad = FALSE);

void UnitDataFreeRow( 
	struct EXCEL_TABLE * table,
	BYTE * rowdata);

void c_InitEffectSet(
	PARTICLE_EFFECT_SET *pEffectSet,
	const char * pszParticleFolder,
	const char * pszSourceName);

enum UNIT_DATA_LOAD_TYPE
{
	UDLT_NONE,
	UDLT_WORLD,
	UDLT_INVENTORY,
	UDLT_INSPECT,
	UDLT_TOWN_OTHER,
	UDLT_TOWN_YOU,
	UDLT_WARDROBE_OTHER,
	UDLT_WARDROBE_YOU,
	UDLT_ALL,
};

BOOL UnitDataLoad(
	GAME * pGame,
	EXCELTABLE eTable,
	int nIndex,
	UNIT_DATA_LOAD_TYPE eLoadType = UDLT_ALL);

void c_UnitDataFlagSoundsForLoad(
	GAME * pGame,
	EXCELTABLE eTable,
	int nIndex,
	BOOL bLoadNow);

void c_UnitDataAlwaysKnownFlagSoundsForLoad(
	GAME * pGame);

void c_UnitDataAllUnflagSoundsForLoad(
	GAME * pGame);

void c_UnitFlagSoundsForLoad(
	UNIT * pUnit);

UNIT* UnitInit(
	GAME* game,
	UNIT_CREATE* uc);

//----------------------------------------------------------------------------
// ACCESSOR FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
inline UNIT_DATA * UnitGetData(
	GAME * game,
	GENUS eGenus,
	int nClass)
{	
	EXCELTABLE eTable = ExcelGetDatatableByGenus(eGenus);
	if (eTable != DATATABLE_NONE)
	{
		return (struct UNIT_DATA*)ExcelGetData(game, eTable, nClass);
	}
	ASSERT(0);
	return NULL;
}

inline const UNIT_DATA* UnitGetData(
	const UNIT * pUnit )
{
	return pUnit ? pUnit->pUnitData : NULL;
}

inline BOOL UnitDataTestFlag(
	const UNIT_DATA * pUnitData,
	int nFlag )
{
	ASSERT_RETFALSE(pUnitData);
	return TESTBIT( pUnitData->pdwFlags, nFlag );
}

//----------------------------------------------------------------------------
inline float UnitGetVelocityForImpact(
	const UNIT * unit)
{
	ASSERT_RETZERO(unit);
	const UNIT_DATA * pUnitData = UnitGetData(unit);
	ASSERT_RETZERO(pUnitData);
	return pUnitData->fVelocityForImpact;
}


#endif // _UNIT_PRIV_H_
