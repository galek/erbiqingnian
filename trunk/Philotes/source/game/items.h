//----------------------------------------------------------------------------
// items.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ITEMS_H_
#define _ITEMS_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _AFFIX_H_
#include "affix.h"
#endif

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef _EXCEL_H_
#include "excel.h"
#endif

#ifndef _INVENTORY_H_
#include "inventory.h"
#endif

#include "itemupgrade.h"
#include "s_message.h"
#include "dbunit.h"

#if ISVERSION(SERVER_VERSION)
#include "playertrace.h"
#endif

#ifndef __CHARACTER_CLASS_H_
#include "characterclass.h"
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define ITEM_SPAWN_FORCE				0.25f

#define PICKUP_RADIUS					10.0f
#define PICKUP_RADIUS_SQ				(PICKUP_RADIUS * PICKUP_RADIUS)

#define NAMELABEL_VIEW_DISTANCE			(15.0f)
#define NAMELABEL_VIEW_DISTANCE_SQ		(225.0f)

#define MAX_QUALITY_DOWNGRADE_ATTEMPTS	6

// Tugboat-specific
#define ALT_DISTANCE					(20.0f)
#define ALT_DIST_SQ						(ALT_DISTANCE * ALT_DISTANCE)


//----------------------------------------------------------------------------
enum ITEM_CONSTANTS
{
	RARENAME_UNITTYPES			= 6,
	MAX_WARDROBES_PER_ITEM		= 6,
	MAX_AFFIX_GEN				= 6,
	MAX_AFFIX_GEN_TYPE			= 6,
	MAX_SPEC_AFFIXES			= 32,		
};

#define MAX_ALT_LIST_SIZE				32

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct ROOM;
struct UNIT_DATA;
struct VECTOR;
enum SKILL_FAMILY;
struct UI_COMPONENT;

//----------------------------------------------------------------------------
enum ITEM_SPAWN_ACTION
{
	ITEM_SPAWN_OK,				// ok, item is uh, "ok" or something
	ITEM_SPAWN_DESTROY,			// nevermind, we don't want this item, destroy it
};

enum ITEM_USEABLE_MASKS
{
	ITEM_USEABLE_MASK_NONE = 0,
	ITEM_USEABLE_MASK_ITEM_TEST_TARGETITEM = MAKE_MASK( 1 )
};

//----------------------------------------------------------------------------
// typedef
//----------------------------------------------------------------------------
typedef ITEM_SPAWN_ACTION (*PFN_ITEMSPAWN_CALLBACK)(GAME * game, UNIT * spawner, UNIT * item, int nItemClass, struct ITEM_SPEC* spec, struct ITEMSPAWN_RESULT * spawnResult, void *pUserData);


//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct PROPERTYDEF_DATA
{
	PCODE			codeProperty;
};

//----------------------------------------------------------------------------
struct ITEM_AFFIX_PICK_DATA
{
	PCODE						codeAffixChance;
	unsigned int				codeAffixTypeWeight[MAX_AFFIX_GEN_TYPE];
	int							linkAffixType[MAX_AFFIX_GEN_TYPE];
};

//----------------------------------------------------------------------------
struct ITEM_QUALITY_DATA
{
	char						szName[DEFAULT_INDEX_SIZE];
	WORD						wCode;	
	int							nBreakDownTreasure;	//treasure class for when an item is broken down
	int							nDisplayName;
	int							nDisplayNameWithItemFormat;
	BOOL						bShowBaseDesc;
	BOOL						bRandomlyNamed;
	int							nBaseDescFormat;	
	int							nItemClassScrap;
	int							nNameColor;
	int							nGridColor;
	BOOL						bDoTransactionLogging;
	BOOL						bChangeClass;
	BOOL						bAlwaysIdentified;
	float						fPriceMultiplier;
	float						fRecipeQuantityMultiplier;
	int							nLevel;
	int							nMinLevel;
	int							nRarity;
	int							nVendorRarity;
	int							nRarityLuck;
	int							nGamblingRarity;
	int							nLookGroup;
	int							nState;
	int							nFlippySound;
	BOOL						bUseable;
	int							nScrapQuality;
	BOOL						bIsSpecialScrapQuality;	
	int							nScrapQualityDefault;
	int							nExtraScrapChancePct;
	int							nExtraScrapItem;
	int							nExtraScrapQuality;
	int							nDismantleResultSound;
	int							nDowngrade;
	int							nUpgrade;
	int							nQualityLevel;
	BOOL						bPrefixName;
	int							nPrefixType;
	PCODE						codePrefixCount;
	BOOL						bForce;
	BOOL						bSuffixName;
	int							nSuffixType;
	PCODE						codeSuffixCount;
	float						flProcChance;						// items of this quality have this chance (0-100) of allowing a proc affix go into the pool for picking
	float						flLuckMod;							// marsh added - modifies the chances of an affix occurring
	ITEM_AFFIX_PICK_DATA		tAffixPicks[MAX_AFFIX_GEN];
	char						szTooltipBadgeFrame[DEFAULT_INDEX_SIZE]; 
	BOOL						bAllowCraftingToCreateQuality;		// tells the crafting system if it can craft this quality of an item
//	int				nAffixType[MAX_AFFIX_GEN];
//	PCODE			codeAffixChance[MAX_AFFIX_GEN];
};


//----------------------------------------------------------------------------
enum QUALITY_NAME
{
	QN_INVALID = -1,
	QN_ITEM_QUALITY_DATATABLE,		// for quality name, use the itemquality datatable
	QN_AFFIX_DATATABLE,				// for quality name, use affix attached to item
};

//----------------------------------------------------------------------------
struct RARENAME_DATA
{
	char			szName[DEFAULT_INDEX_SIZE];
	BOOL			bSuffix;
	int				nLevel;
	DWORD			dwCode;
	int				nAllowTypes[AFFIX_UNITTYPES];
	PCODE			codeRareNameCond;
};

//----------------------------------------------------------------------------
struct ITEM_NAME_INFO
{
	WCHAR uszName[ MAX_ITEM_NAME_STRING_LENGTH ];	// the text
	DWORD dwStringAttributes;						// attributes of the item string (gender, number, etc)
};

//----------------------------------------------------------------------------
enum ITEMSPAWN_FLAGS
{
	ISF_STARTING_BIT,						// TODO: for BSP - remove this
	ISF_PLACE_IN_WORLD_BIT,					// put item in the world
	ISF_PICKUP_BIT,							// pickup treasure that is spawned
	ISF_REPORT_ONLY_BIT,					// send to trace window - for debugging
	ISF_AFFIX_SPEC_ONLY_BIT,				// do not random select affixes, use only the ones in the spec nAffixes
	ISF_SKIP_REQUIRED_AFFIXES_BIT,			// don't do the required affix logic at all	
	ISF_SKIP_HAND_CRAFTED_AFFIXES_BIT,		// don't do the hand crafted affix logic at all
	ISF_MAX_SLOTS_BIT,						// create items with max slots for mods
	ISF_GENERATE_ALL_MODS_BIT,				// auto fill item with mods
	ISF_IDENTIFY_BIT,						// auto identify item after spawning
	ISF_USEABLE_BY_SPAWNER_BIT,				// item must be usable by the spawner
	ISF_USEABLE_BY_OPERATOR_BIT,			// item must be usable by the operator (killer, chest opener, etc ... in general ... the player)
	ISF_USEABLE_BY_ENABLED_PLAYER_CLASS,	// item must be usable by some player class that is enabled for this version package
	ISF_RESTRICTED_TO_GUID_BIT,				// item is for a specific client, no other clients can see it or try to pick it up
	ISF_NO_DATABASE_OPERATIONS_BIT,			// create items with no database interactions
	ISF_MONSTER_INVENTORY_BIT,				// item spaws are for monster inventory
	ISF_NO_FLIPPY_BIT,						// do not flippy or play spawn sounds
	ISF_USE_SPEC_LEVEL_ONLY_BIT,			// do not do normal experience level dice rolls, use experience level provided in item spec
	ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT,	// do not let affixes change the level of this item
	ISF_AVAILABLE_AT_MERCHANT_ONLY_BIT,		// only create items that 
	ISF_IGNORE_GLOBAL_THEME_RULES_BIT,		// when set, you can spawn items that require a global theme, even if that global theme is not on
	ISF_SKIP_QUALITY_AFFIXES_BIT,			// skips affixes given by the quality of an item
	ISF_ZERO_OUT_SOCKETS,					// zero's out max sockets
	ISF_IS_CRAFTED_ITEM,					// is a crafted item
	ISF_SKIP_PROPS,							// don't do the excel script props stuff
	ISF_BASE_LEVEL_ON_PLAYER_BIT,			// for treasure, base the level of the items on the player instead of the spawner or merchant
};


//----------------------------------------------------------------------------
struct OPEN_NODES
{
	struct FREE_PATH_NODE *pFreePathNodes;		// array of path nodes
	int nNumPathNodes;							// size of path node array
	int nCurrentNode;							// current node
};

//----------------------------------------------------------------------------
void ItemSpawnSpecInit(
	ITEM_SPEC &tSpec);

//----------------------------------------------------------------------------
struct ITEM_SPEC
{
	UNITID idUnit;
	PGUID guidUnit;
	int nItemClass;
	int nItemLookGroup;
	int nTeam;
	int nAngle;
	float flScale;
	VECTOR vDirection;
	VECTOR vUp;
	enum UNIT_DATA_LOAD_TYPE eLoadType;
	
	int nLevel;
	int nQuality;
	int nAffixes[MAX_SPEC_AFFIXES];
	int nSkill;
	int nNumber;
	int nMoneyAmount;
	int nRealWorldMoneyAmount;
	DWORD dwFlags;					// see ITEMSPAWN_FLAGS	
	UNIT *pSpawner;					// unit that the item is spawning from (like the monster you get treasure from)
	UNIT *unitOperator;				// unit that's causing the item to spawn
	PGUID guidOperatorParty;		// party of the unit causing the item to spawn (in case the unit's no longer there)
	PGUID guidRestrictedTo;			// for items that are restricted to a specific PGUID
	VECTOR *pvPosition;				// position to spawn at (instead of at unit)
	ROOM *pRoom;					// room of pvPosition provided
	OPEN_NODES *pOpenNodes;			// positions to use for spawning (if any)
	int nOverrideInventorySizeX;
	int nOverrideInventorySizeY;
	float nLuckModifier;			// the luck value for how it's going ot modify the quality	
	int iLuckModifiedItem;			// gets set when luck actually modified a drop
	UNIT *pLuckOwner;	
//	UNIT *pClientUnit;				// client to send spawned item
	UNIT *pMerchant;				// merchant item is created "from" for personal stores
	PFN_ITEMSPAWN_CALLBACK pfnSpawnCallback;	// callback for each item spawned
	void *pSpawnCallbackData;					// userdata for pfnSpawnCallback
	
	ITEM_SPEC::ITEM_SPEC( void ) { ItemSpawnSpecInit(*this); }	
	
};

//----------------------------------------------------------------------------
struct ITEMSPAWN_RESULT
{
	UNIT **pTreasureBuffer;		// set by caller, items that are created can be stored here
	int nTreasureBufferSize;	// set by caller, max size of the treasure buffer
	int nTreasureBufferCount;	// set by system, count of items placed in pTreasureBuffer

	ITEMSPAWN_RESULT(
		void) :
		pTreasureBuffer(NULL),
		nTreasureBufferSize(0),
		nTreasureBufferCount(0)
	{
	}	
};

struct ITEM_LOOK_GROUP_DATA
{
	char			szName[DEFAULT_INDEX_SIZE];
	BYTE			bCode;
};


struct ITEM_LOOK_DATA
{
	int				nItemClass;
	int				nLookGroup;
	char			szFileFolder[DEFAULT_FILE_SIZE];
	char			szAppearance[DEFAULT_FILE_SIZE];
	int				nWardrobe;
	int				nAppearanceId;
};


struct ALTITEMLIST
{
	BOOL			bInPickupRange;
	WCHAR			szDescription[ MAX_ITEM_NAME_STRING_LENGTH ];
	UNITID			idItem;
};


struct ITEM_LEVEL
{
	int				nLevel;
	int				nDamageMult;
	int				nBaseArmor;
	int				nBaseArmorBuffer;
	int				nBaseArmorRegen;
	int				nBaseShields;
	int				nBaseShieldBuffer;
	int				nBaseShieldRegen;
	int				nBaseSfxAttackAbility;
	int				nBaseSfxDefenseAbility;
	int				nBaseInterruptAttack;
	int				nBaseInterruptDefense;
	int				nBaseStealthAttack;
	int				nBaseAIChangerAttack;
	int				nBaseFeed;
	int				nLevelRequirement;
	int				nItemLevelRequirement;
	int				nItemLevelLimit;
	PCODE			codeBuyPriceBase;
	PCODE			codeSellPriceBase;
	int				nAugmentCost[ IAUGT_NUM_TYPES ];
	int				nQuantityScrap;
	int				nQuantityScrapSpecial;
	int				nItemLevelsPerUpgrade;
	
};

//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct ITEM_INIT_CONTEXT
{
	GAME *						game;
	UNIT *						unit;
	UNIT *						spawner;
	const UNIT_DATA *			item_data;
	const ITEM_LEVEL *			item_level;
	const ITEM_QUALITY_DATA *	quality_data;
	ITEM_SPEC *					spec;
	unsigned int				unitclass;
	int							level;
	int							quality;
	unsigned int				affix_count;

	ITEM_INIT_CONTEXT(
		GAME * in_game,
		UNIT * in_unit,
		UNIT * in_spawner,
		int in_class,
		const UNIT_DATA * in_item_data,
		ITEM_SPEC * in_spec) :
		game(in_game),
		unit(in_unit),
		spawner(in_spawner),
		unitclass(in_class),
		item_data(in_item_data),
		item_level(NULL),
		spec(in_spec),
		level(0),
		quality(EXCEL_LINK_INVALID),
		affix_count(0) {}
};
#endif

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL ExcelItemsPostProcess(
	struct EXCEL_TABLE * table);

const UNIT_DATA * ItemGetData(
	int nClass);
	
const UNIT_DATA * ItemGetData(
	UNIT *pItem);
	
const UNIT_DATA *ItemModGetData(
	int nModClass);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int ItemGetClassByCode(
	GAME * game,
	DWORD dwCode)
{
	return ExcelGetLineByCode(game, DATATABLE_ITEMS, dwCode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const ITEM_LEVEL * ItemLevelGetData(
	GAME * game,
	int level)
{
	level = PIN(level, 1, (int)ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_ITEM_LEVELS));
	return (const ITEM_LEVEL *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_ITEM_LEVELS, level);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const RARENAME_DATA * RareNameGetData(
	GAME * game,
	int affix)
{
	return (const RARENAME_DATA *)ExcelGetData(game, DATATABLE_RARENAMES, affix);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int RareNameGetByCode(
	GAME * game,
	DWORD dwCode)
{
	return ExcelGetLineByCode(game, DATATABLE_RARENAMES, dwCode);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const ITEM_LOOK_GROUP_DATA * LookGroupGetData(
	GAME * game,
	int nLookGroup)
{
	return (const ITEM_LOOK_GROUP_DATA *)ExcelGetData(game, DATATABLE_ITEM_LOOK_GROUPS, nLookGroup);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const AFFIX_TYPE_DATA * AffixTypeGetData(
	GAME * game,
	int affixtype)
{
	return (const AFFIX_TYPE_DATA *)ExcelGetData(game, DATATABLE_AFFIXTYPES, affixtype);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const ITEM_QUALITY_DATA * ItemQualityGetData(
	int quality)
{
	return (const ITEM_QUALITY_DATA *)ExcelGetData(NULL, DATATABLE_ITEM_QUALITY, quality);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const ITEM_QUALITY_DATA * ItemQualityGetData(
	GAME * game,
	int quality)
{
	return (const ITEM_QUALITY_DATA *)ExcelGetData(game, DATATABLE_ITEM_QUALITY, quality);
}


//returns the level area the item is bounded to. returns invalid if the item is not bounded to anything.
int ItemGetBindedLevelArea( UNIT *item );
//binds the item to the level area
void ItemBindToLevelArea( UNIT *item, 
						  LEVEL *pLevel );
BOOL ItemQualityIsBetterThan(
	int nItemQuality1,
	int nItemQuality2);

void ItemShine(
	GAME * game,
	UNIT * unit);

void ItemShine(
	GAME * game,
	UNIT * unit,
	VECTOR & vPosition);

void ItemFlippy( 
	UNIT * pUnit,
	BOOL bSendToClient);

UNIT * s_ItemSpawn(
	GAME * game,
	ITEM_SPEC &tSpec,
	ITEMSPAWN_RESULT * spawnResult);

BOOL s_ItemInitProperties(
	GAME * game,
	UNIT * unit,
	UNIT * spawner,
	int nClass,
	const UNIT_DATA * item_data,
	ITEM_SPEC * spec);

UNIT * c_ItemSpawnForDisplay(
	GAME * game,
	ITEM_SPEC &tSpec );

#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
#define ITEM_SPAWN_CHEAT_FLAG_PICKUP		MAKE_MASK( 0 )
#define ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP	MAKE_MASK( 1 )
#define ITEM_SPAWN_CHEAT_FLAG_ADD_MODS		MAKE_MASK( 2 )
#define ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT	MAKE_MASK( 3 )
#define ITEM_SPAWN_CHEAT_FLAG_DONT_SEND		MAKE_MASK( 4 )
UNIT * s_ItemSpawnForCheats(
	GAME * game,
	ITEM_SPEC specCopy,
	struct GAMECLIENT * client,
	DWORD dwFlags,
	BOOL * pbElementEffects );
#endif

UNIT * ItemInit(
	GAME * game,
	ITEM_SPEC &tItemSpec);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemInitPropertiesSetLevelBasedStats(
	ITEM_INIT_CONTEXT & context);

BOOL s_ItemInitPropertiesSetDefenseStats(
	ITEM_INIT_CONTEXT & context);

BOOL s_ItemInitPropertiesSetPerLevelStats(
	ITEM_INIT_CONTEXT & context);
#endif

void ItemUnFuse(
	UNIT * pUnit);

void ItemSetFuse(
	UNIT *unit,
	int nTicks);

BOOL ItemsAreIdentical(
	UNIT * item1,
	UNIT * item2);

BOOL ItemHasStackSpaceFor(
	UNIT * pDest,
	UNIT * pSource);
	
void s_ItemPullToPickup(
	UNIT *pUnit,
	UNIT *pItem);

void ItemRemovePickupRestrictions(
	UNIT *pItem);

//----------------------------------------------------------------------------
enum PICKUP_RESULT
{

	PR_OK,				// OK to pickup

	PR_FAILURE,			// cannot pickup, failure, error, unknown	
	PR_FULL_ITEMS,		// holding too many items
	PR_FULL_MONEY,		// holding too much money (heh)
	PR_LOCKED,			// item is locked for somebody else
	PR_QUEST_ITEM_PROHIBITED,	// a quest prohibits pickup of a quest item for some reason (may have enough already)
	PR_MAX_ALLOWED,		// cannot pickup because we have the max allowed of an item of that type
	
	PR_NUM_RESULTS
	
};

enum
{
	ICP_DONT_CHECK_CONDITIONS_MASK	= MAKE_MASK(0),
	ICP_DONT_CHECK_KNOWN_MASK		= MAKE_MASK(1),
};

PICKUP_RESULT ItemCanPickup(
	UNIT *pUnit,
	UNIT *pItem,
	DWORD dwFlags = 0);

void s_ItemSendCannotPickup(
	UNIT *pUnit,
	UNIT *pItem,
	PICKUP_RESULT eReason);
	
UNIT* s_ItemPickup(
	UNIT * unit,
	UNIT * item);
	
//----------------------------------------------------------------------------
enum DROP_RESULT
{
	DR_OK,					// A-OK! ;)
	DR_FAILED,				// a generic failed case
	DR_NO_DROP,				// item is a no drop item
	DR_ACTIVE_OFFERING,		// item is in active offering, must take into inv, not throw away
	DR_OK_DESTROY,			// item can be dropped but it needs to be destroyed
	DR_OK_DESTROYED,		// item has been dropped and destroyed
};

DROP_RESULT s_ItemDrop(
	UNIT *pDropper,
	UNIT *pItem,
	BOOL bForce = FALSE,
	BOOL bNoFlippy = FALSE,
	BOOL bNoRestriction = FALSE);

void c_ItemTryDrop(
	UNIT *pItem);

void s_ItemSendCannotDrop( 
	UNIT *pUnit,
	UNIT *pItem, 
	DROP_RESULT eReason);

//----------------------------------------------------------------------------
enum USE_RESULT
{
	USE_RESULT_INVALID = -1,

	USE_RESULT_NOT_USABLE,	
	USE_RESULT_CANNOT_USE_IN_TOWN,
	USE_RESULT_CANNOT_USE_IN_HELLRIFT,
	USE_RESULT_CONDITION_FAILED,
	USE_RESULT_CANNOT_USE_COOLDOWN,
	USE_RESULT_CANNOT_USE_POISONED,	
	
	USE_RESULT_OK
	
};

BOOL ItemHasSkillWhenUsed(
	UNIT *pItem);

USE_RESULT ItemIsUsable(
	UNIT *pUnit,
	UNIT *pItem,
	UNIT *pItemTarget,
	DWORD nItemUsuableMasks );

USE_RESULT ItemIsQuestUsable(
	UNIT * pUnit,
	UNIT * pItem );

USE_RESULT ItemIsQuestActiveBarOk(
	UNIT * pUnit,
	UNIT * pItem );

UNIT *ItemGetMerchant(
	UNIT *pItem);
	
BOOL ItemBelongsToAnyMerchant(
	UNIT *pItem);

BOOL ItemBelongsToSpecificMerchant( 
	UNIT *pItem, 
	int nMerchantUnitClass);

BOOL ItemBelongsToSpecificMerchant(
	UNIT *pItem,
	UNIT *pMerchant);

BOOL ItemIsInSharedStash(
	UNIT *pItem);
		
void s_ItemSendCannotUse(
	UNIT *pUnit,
	UNIT *pItem,
	USE_RESULT eReason);
	
void s_ItemUseOnItem(
	GAME *pGame,
	UNIT *pOwner,
	UNIT *pItem,
	UNIT *pItemTarget );

void s_ItemUse(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pItem);

void c_ItemUse(
	UNIT *pItem);

void c_ItemUseOn(
   UNIT *pItem,
   UNIT *pTarget);

void c_ItemShowInChat(
	UNIT *pItem);

BOOL s_ItemDecrement(
	UNIT * pItem);
	
int ItemGetModSlots(
	UNIT * item);

void c_ItemInitInventoryGfx( 
	GAME * game,
	UNITID id,
	BOOL bCreateWardrobe);

void c_ItemFreeInventoryGfx( 
	GAME * game,
	UNITID id);

int ItemGetByUnitType(
	GAME * game,
	int nUnitType);

void c_ClearAltSelectList( 
	void );

ALTITEMLIST * c_FindClosestItem(
	UNIT * unit);

ALTITEMLIST * c_GetCurrentClosestItem(
	void);

const ITEM_LOOK_DATA * ItemLookGetData(
	GAME * game,
	int nClass,
	int nLookGroup);

int ItemGetAppearanceDefId(
	GAME * game,
	int nClass,
	int nLookupGroup);

int ItemGetWardrobe(
	GAME * game,
	int nClass,
	int nLookupGroup);

int ItemGetPrimarySkill(
	UNIT * item);

BOOL c_ItemSell(	
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pSellToUnit,
	UNIT * pItem);

BOOL s_ItemSell(	
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pSellToUnit,
	UNIT * pItem);

BOOL c_ItemBuy(	
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pBuyFromUnit,
	UNIT * pItem,
	int nSuggestedLocation = INVLOC_NONE, 
	int nSuggestedX = -1,
	int nSuggestedY = -1);

BOOL ItemIsIdentified( 
	UNIT * item );

BOOL ItemIsUnidentified(
	UNIT *pItem);

BOOL ItemIsACraftedItem(
	UNIT *pItem);

BOOL s_ItemIdentify(
	UNIT *pOwner,
	UNIT *pAnalyzer,
	UNIT *pItem);

int s_ItemIdentifyAll( 
	UNIT *pPlayer);

void s_ItemResend(
	UNIT *pItem);

//----------------------------------------------------------------------------
enum ITEM_MODE
{
	ITEM_MODE_INVALID = 0,
	
	ITEM_MODE_IDENTIFY,			// cursor is in identify mode, clicking on an item will try to id it
	ITEM_MODE_DISMANTLE,		// cursor is in dismangle mode, clicking on item will try to dismantle it
	ITEM_MODE_CREATE_GUILD,		// "enter guild name" dialog is up, pending a guild create
	ITEM_SKILL_SELECT,

	NUM_ITEM_MODES
};

void c_ItemEnterMode(
	ITEM_MODE eItemMode,
	UNIT *pSource);
	
void c_ItemTryIdentify(
	UNIT *pItem,
	BOOL bUsingAnalyzer);

void c_ItemPickItemExecuteSkill(
	UNIT *pItem,
	UNIT *pSource);

//----------------------------------------------------------------------------
enum { MAX_SCRAP_PIECES = 16 };

//----------------------------------------------------------------------------
struct SCRAP_LOOKUP
{
	int nItemClass;
	int nItemQuality;
	int nItemSellPrice;
	UNIT *pScrap;
	BOOL bGiven;
	BOOL bExtraResource;
};

void ItemGetScrapComponents( 
	UNIT *pItem, 
	SCRAP_LOOKUP *pBuffer, 
	int nBufferSize,
	int *pnCurrentBufferCount,
	BOOL bForUpgrade);
	
void s_ItemDismantle(
	UNIT *pOwner,
	UNIT *pItemToDismantle,
	UNIT *pDismantler);
	
void c_ItemTryDismantle(
	UNIT *pItem,
	BOOL bUsingDismantler);

BOOL ItemCanBeDismantled(
	UNIT *pOwner,
	UNIT *pItem);

void c_ItemTrySplit(
	UNIT *pItem,
	int nNewStackSize);

//----------------------------------------------------------------------------

void ItemSetLookGroup(
	UNIT *pItem, 
	int nItemLookGroup);

BOOL ItemModIsAllowedOn(
	UNIT* pMod,
	UNIT* pItem);

BOOL s_ItemModAdd(
	UNIT* pItem,
	UNIT* pMod,
	BOOL bSendToClients);

//----------------------------------------------------------------------------
typedef void (*PFN_ADD_MOD_CALLBACK)(UNIT *pItem, void *pUserData);
		
BOOL s_ItemModGenerateAll(
	UNIT* pItem,
	UNIT* pSpawner,
	int nItemLookGroup,
	PFN_ADD_MOD_CALLBACK pfnCallback,
	void *pCallbackData);

void s_ItemModDestroyAllEngineered(
	UNIT *pItem);
	
#if ISVERSION(DEVELOPMENT)	// CHB 2006.12.22
BOOL s_ItemGenerateRandomArmor(
	UNIT * pUnit,
	BOOL bDebugOutput );

BOOL s_ItemGenerateRandomWeapons(
	UNIT * pUnit,
	BOOL bDebugOutput,
	int nNumItemsToSpawn);
#endif

//----------------------------------------------------------------------------
enum ITEM_NAME_INFO_FLAGS
{
	INIF_MODIFY_NAME_BY_ANY_QUANTITY,
	INIF_POSSIVE_FORM,
};

void ItemGetNameInfo(
	ITEM_NAME_INFO &tItemNameInfo, 
	UNIT *pItem,
	int nQuantity, 
	DWORD dwFlags);		// see ITEM_NAME_INFO_FLAGS
	
void ItemGetNameInfo(
	ITEM_NAME_INFO &tItemNameInfo,
	int nItemClass,
	int nQuantity,
	DWORD dwFlags);

void s_ItemNetRemoveFromContainer(
	GAME *pGame,
	UNIT *pItem,
	DWORD dwUnitFreeFlags = 0);

void s_ItemRemoveUpdateHotkeys(
	GAME* game,
	UNIT *pPreviousUltimateOwner,
	UNIT* item);

VECTOR OpenNodeGetNextWorldPosition( 
	GAME *pGame,
	OPEN_NODES *pOpenNodes);
				
BOOL c_ItemTryPickup(
	GAME * game,
	UNITID id );

void c_ItemCannotUse(
	UNIT *pItem,
	USE_RESULT eReason);

void c_ItemCannotPickup(
	UNITID idItem,
	PICKUP_RESULT eReason);

void c_ItemCannotDrop(
	UNITID idItem,
	DROP_RESULT eReason);

void ItemLockForPlayer( 
	UNIT *pItem, 
	UNIT *pPlayer);
	
BOOL ItemIsLocked(
	UNIT *pItem);
	
BOOL ItemIsLockedForPlayer(
	UNIT *pItem,
	UNIT *pPlayer);
	
int ItemGetRank(
	UNIT *pItem);

void ItemSetQuantity(
	UNIT *pItem,
	int nQuantity,
	DBOPERATION_CONTEXT *pOpContext = NULL);

int ItemGetQuantity(
	UNIT *pItem);
	
int ItemGetUIQuantity(
	UNIT *pUnit);

BOOL ItemGetName(
	UNIT *pUnit,
	WCHAR *puszNameBuffer,
	int nBufferMaxChars,
	DWORD dwFlags);

void ItemProperNameSet(
	UNIT *pItem,
	int nNameIndex,
	BOOL bForce);

int ItemUniqueNameGet(
	UNIT *pItem);

void ItemGetAffixListString(
	UNIT *pUnit,
	WCHAR *puszString,
	int nStringLength);

BOOL ItemGetAffixPropertiesString(
	UNIT *pUnit,
	WCHAR *puszString,
	int nStringLength,
	BOOL bExtended);

BOOL ItemGetInvItemsPropertiesString(
	UNIT *pUnit,
	int nLocation, 
	WCHAR *puszString,
	int nStringLength);

const WCHAR *c_ItemGetUseResultString(
	USE_RESULT eResult);

BOOL ItemIsInCursor(
	UNIT *pItem);

void ItemGetEmbeddedTextModelString(
	UNIT *pItem,
	WCHAR *puszBuffer,
	int nMaxBuffer,
	BOOL bBackgroundFrame,
	int nGridSize = 32);

int ItemGetInteractChoices( 
	UNIT * pUser, 
	UNIT * pItem,
	struct INTERACT_MENU_CHOICE *ptChoices,
	int nBufferSize,
	UI_COMPONENT *pComponent = NULL);

void ItemSpawnSpecAddLuck(
	ITEM_SPEC &tSpec,
	UNIT * unit);
	
void ItemUniqueGetBaseString(
	UNIT *pUnit,
	WCHAR *puszBuffer,
	int nBufferSize);

int ItemGetQuality(
	UNIT *pItem);

void ItemSetQuality(
	UNIT *pItem,
	int nItemQuality);

BOOL ItemCanDowngradeQuality(
	int nItemClass);

int ItemGetScrapQuality(
	UNIT *pItem);

const char *ItemGetQualityName(
	UNIT *pItem);

int ItemGetTurretizeSkill(	
	UNIT *pItem);

BOOL ItemCanBeTurretized(
	UNIT *pUser,
	UNIT *pItem,
	SKILL_FAMILY eFamily);

void c_ItemTurretize(
	UNIT *pItem,
	SKILL_FAMILY eFamily);

void s_ItemsRestoreToStandardLocations(
	UNIT *pUnit,
	int nInvLocSource,
	BOOL bDropItemsOnError = TRUE);

BOOL s_ItemRestoreToStandardInventory( 
	UNIT *pItem, 
	UNIT *pPlayerDest,
	INV_CONTEXT eContext);

BOOL ItemIsRecipeComponent(
	UNIT *pItem);

BOOL ItemIsRecipeIngredient(
	UNIT *pItem);

BOOL ItemMeetsStatReqsForUI(
	UNIT *pItem,
	int nStat = INVALID_LINK,
	UNIT *pCompareUnit = NULL);

BOOL ItemIsInRewardLocation( 
	UNIT *pUnit);

BOOL InventoryIsInLocationWithFlag(
	UNIT *pUnit,
	INVLOC_FLAG eInvLocFlag);

BOOL ItemIsNoTrade(	
	UNIT *pItem);

void ItemSetNoTrade(
	UNIT *pItem,
	BOOL bValue);

int ItemGetUpgradeQuality(
	UNIT *pItem);

void s_ItemsReturnToStandardLocations(	
	UNIT *pPlayer,
	BOOL bReturnCursorItems);

#if !ISVERSION(CLIENT_ONLY_VERSION)
UNIT* s_ItemCloneAndCripple(
	UNIT *pItem);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

void s_ItemVersion(
	UNIT *pItem);

void c_ItemsRestored(
	int nNumItemsRestored,
	int nNumItemsDropped);

struct INVENTORY_ITEM
{
	UNIT *pUnit;
	INVENTORY_LOCATION tInvLocation;
};

int s_ItemRemoveInventoryToBuffer(
	UNIT *pItem,
	INVENTORY_ITEM *pBuffer,
	int nMaxBufferSize);

void s_ItemReplicateSocketSlots(
	UNIT *pSource,
	UNIT *pDest);

int ItemGetMaxPossibleLevel(
	GAME *pGame);
			
BOOL ItemQualityIsEqualToOrLessThanQuality(
	int nQuality1,
	int nQuality2);

BOOL sItemDecrementOrUse(
	GAME* game, 
	UNIT* item,
	BOOL bFromPickup);

BOOL s_ItemPickupFromStack(
	UNIT * pPlayer,
	UNIT * pItem,
	int nQuantity);

BOOL s_ItemPropIsValidForUnitType( int nPropID,
								   const UNIT_DATA *pUnitData,
								   int nUnitType );

BOOL s_ItemPropsInit( GAME *pGame,
					  UNIT *pItem,
					  UNIT *pSpawner,
					  UNIT *pItemInsertingInto = NULL );


//returns true if the item is a recipe
BOOL ItemIsRecipe( UNIT *pItem );

//returns the ID of the recipe if the item is a recipe item
int ItemGetRecipeID( UNIT *pItem );

//returns if the item should be bind'ed to the player if picked up
BOOL ItemBindsOnPickup( UNIT *pItem );

//returns if the item should be binded if it's equipped
BOOL ItemBindsOnEquip( UNIT *pItem );

//returns if the item binds on to level area
BOOL ItemBindsToLevel( UNIT *pItem );

//returns if the item is binded to a player
BOOL ItemIsBinded( UNIT *pItem );

//Sets the item binded to the player
void s_ItemBindToPlayer( UNIT *pItem );

BOOL ItemIsInCanTakeLocation(	
	UNIT *pItem);

void ItemRemovingFromCanTakeLocation(
	UNIT *pPlayer,
	UNIT *pItem);

BOOL ItemGetCreatorsName( UNIT *pItem,
						  WCHAR *uszPlayerNameReturned,
						  int nStringArraySize );

BOOL ItemGetMinAC( const UNIT_DATA *pItemData,
				   int nDamageType,
				   int nLevelOffset = 0,
				   int nMinACPercentMod = 0 );
BOOL ItemGetMaxAC( const UNIT_DATA *pItemData,
				   int nDamageType,
				   int nLevelOffset = 0,
				   int nMaxACPercentMod = 0 );
BOOL ItemCaculateAC( GAME *pGame,
					 const UNIT_DATA *pItemData,								    
					 int &nAC,
					 int nLevelOffset = 0,
					 int nMinACPercentMod = 0,
					 int nMaxACPercentMod = 0,
					 BOOL bForceMax = FALSE);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ItemSetMinAndMaxAC( UNIT *pItem,
						   int nLevelOffset = 0,
						   int nMinACPercentMod = 0,
						   int nMaxACPercentMod = 0,
						   BOOL bForceMax = FALSE );
#endif

BOOL ItemCaculateMinAndMaxBaseDamage( GAME *pGame,
									  const UNIT_DATA *pItemData,								    
									  int &nMinDmg,
									  int &nMaxDmg,
									  int nLevelOffset = 0,
									  int nMinDmgPercentMod = 0,
									  int nMaxDmgPercentMod = 0);

#if !ISVERSION(CLIENT_ONLY_VERSION)	
BOOL s_ItemSetMinAndMaxBaseDamage( UNIT *pItem,
								   int nLevelOffset = 0,
								   int nMinDmgPercentMod = 0,
								   int nMaxDmgPercentMod = 0 );
#endif
BOOL ItemGetPlayerLogString(
							UNIT *pItem,
							WCHAR *szName,
							int len);
void ItemPlayerLogDataInitFromUnit(
								   struct ITEM_PLAYER_LOG_DATA &tItemData,
								   UNIT *pUnit);

void ItemPlayerLogDataInitFromUnitData(
									   struct ITEM_PLAYER_LOG_DATA &tItemData,
									   const UNIT_DATA *pUnitData);


BOOL ItemWasCreatedByPlayer( 
	UNIT *pItem,
	UNIT *pPlayer );

BOOL ItemIsSpawningForGambler(
	const ITEM_SPEC * spec);

BOOL ItemUsesWardrobe(
	const UNIT_DATA * pItemUnitData );

PRESULT ItemUnitDataGetIconFilename(
	OS_PATH_CHAR * poszFilename,
	int nBufLen,
	const struct UNIT_DATA * pItemUnitData,
	GENDER eGender = GENDER_INVALID );
	
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern float gflItemProcChanceOverride;

#endif // _ITEMS_H_


