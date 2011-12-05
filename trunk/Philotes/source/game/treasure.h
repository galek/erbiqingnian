//----------------------------------------------------------------------------
// treasure.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _TREASURE_H_
#define _TREASURE_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct ITEM_SPEC;
struct ITEMSPAWN_RESULT;
struct UNIT_DATA;
enum TREASURE_RESULT;


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum TREASURE_CONSTANTS
{
	MAX_TREASURE_CHOICES				= 8,	// # of pick columns in treasure.xls
	MAX_NUM_CHANCES						= 10,	// # of possible number of items chances
	MAX_CATEGORY_INDICIES				= 10,
	MAX_TRACKED_LOOT_PER_DROP			= 128,
	MAX_MOD_LIST						= 100,
	TREASURE_ALLOWED_UNITTYPES  		= 6,	// max simultaneous unit types a treasure class can be unrestricted from
	DEFAULT_MAX_TREASURE_DROP_RADIUS	= 50,	// by default, you must be this close to monsters for them to drop booty
};

//----------------------------------------------------------------------------
enum TREASURE_PICK_TYPE
{
	PICKTYPE_RAND,
	PICKTYPE_ALL,
	PICKTYPE_MODIFIERS,
	PICKTYPE_INDEPENDENT_PERCENTS,
	PICKTYPE_RAND_ELIMINATE,
	PICKTYPE_FIRST_VALID,
};

//----------------------------------------------------------------------------
enum TREASURECLASS_CATEGORY
{
	TC_CATEGORY_NONE,
	TC_CATEGORY_ITEMCLASS,
	TC_CATEGORY_UNITTYPE,
	TC_CATEGORY_TREASURECLASS,
	TC_CATEGORY_QUALITY_MOD,
	TC_CATEGORY_UNITTYPE_MOD,
	TC_CATEGORY_MAX_SLOTS,
	TC_CATEGORY_NODROP,
};

//----------------------------------------------------------------------------
enum PICKINDEX_OPERATOR
{
	PICK_OP_NOOP,
	PICK_OP_NOT,
	PICK_OP_NEGATE,
	PICK_OP_MOD,
	PICK_OP_SET,
	PICK_OP_IGNORE_AND_GREATER,
};

//----------------------------------------------------------------------------
enum TREASURE_RESULT
{
	RESULT_INVALID = -1,
	RESULT_NO_DROP,
	RESULT_DROP_IMPOSSIBLE,
	RESULT_DROP_FAILED,
	RESULT_ITEM_SPAWNED,
};

//----------------------------------------------------------------------------
// STRUCT
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct PICKOP_SYMBOL
{
	char				cSymbol;
	PICKINDEX_OPERATOR	eOp;
};

//----------------------------------------------------------------------------
struct TREASURE_PICK
{
	TREASURECLASS_CATEGORY		eCategory;
	int							nIndex[MAX_CATEGORY_INDICIES];
	PICKINDEX_OPERATOR			eOperator[MAX_CATEGORY_INDICIES];
	int							nValue;

	TREASURE_PICK();
};

//----------------------------------------------------------------------------
struct TREASURE_MOD_LIST
{
	TREASURE_PICK	pModifiers[MAX_MOD_LIST];
	int				nNumModifiers;

	TREASURE_MOD_LIST::TREASURE_MOD_LIST() : nNumModifiers(0)	{}

	inline TREASURE_MOD_LIST operator+	( const TREASURE_MOD_LIST & rhs ) const
	{	
		TREASURE_MOD_LIST returnlist;
		for (int i=0; i < nNumModifiers; i++)
		{
			returnlist.pModifiers[i] = pModifiers[i];
		}
		for (int i=nNumModifiers; i < nNumModifiers + rhs.nNumModifiers && i < MAX_MOD_LIST; i++)
		{
			returnlist.pModifiers[i] = rhs.pModifiers[i];
		}
		returnlist.nNumModifiers = MIN(nNumModifiers + rhs.nNumModifiers, (int)MAX_MOD_LIST);
		return returnlist;
	}

	inline void operator+= ( const TREASURE_MOD_LIST & rhs )
	{
		for (int i=nNumModifiers; i < rhs.nNumModifiers; i++)
		{
			pModifiers[i] = rhs.pModifiers[i];
		}
		nNumModifiers += rhs.nNumModifiers;
	}

	inline void AddMod(const TREASURE_PICK& tPick)
	{
		ASSERT_RETURN(nNumModifiers < MAX_MOD_LIST);
		pModifiers[nNumModifiers] = tPick;
		nNumModifiers++;
	}
};

//----------------------------------------------------------------------------
struct TREASURE_NUM_PICK_CHANCES
{
	int		pnWeights[MAX_NUM_CHANCES];
	int		pnNumPicks[MAX_NUM_CHANCES];
};

//----------------------------------------------------------------------------
enum TREASURE_DATA_FLAG
{
	TDF_INVALID  = -1,
	
	TDF_CREATE_FOR_ALL_PLAYERS_IN_LEVEL,		// create loot for all players in level reguardless of the proximity or help heuristic
	TDF_REQUIRED_USABLE_BY_OPERATOR,
	TDF_REQUIRED_USABLE_BY_SPAWNER,
	TDF_SUBSCRIBER_ONLY,
	TDF_MAX_SLOTS,
	TDF_RESULT_NOT_REQUIRED,
	TDF_STACK_TREASURE,
	TDF_MULTIPLAYER_ONLY,
	TDF_SINGLE_PLAYER_ONLY,
	TDF_BASE_LEVEL_ON_PLAYER,

	TDF_NUM_FLAGS
	
};

//----------------------------------------------------------------------------
struct TREASURE_DATA
{
	char						szTreasure[DEFAULT_INDEX_SIZE];
	int							nAllowUnitTypes[TREASURE_ALLOWED_UNITTYPES];
	int							nGlobalThemeRequired;
	PCODE						codeCondition;
	TREASURE_PICK_TYPE			nPickType;
	TREASURE_NUM_PICK_CHANCES	tNumPicksChances;
	float						fNoDrop;
	PCODE						codeLevelBoost;
	float						fGamblePriceRangeMin;
	float						fGamblePriceRangeMax;
	float						flMoneyChanceMultiplier;
	float						flMoneyLuckChanceMultiplier;
	float						flMoneyAmountMultiplier;
	TREASURE_PICK				pPicks[MAX_TREASURE_CHOICES];
	TREASURE_MOD_LIST			tModList;
	DWORD						pdwFlags[ DWORD_FLAG_SIZE( TDF_NUM_FLAGS ) ];	
	int							nSourceUnitType;
	int							nSourceLevelTheme;
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

const TREASURE_DATA* TreasureGetData(
	GAME* game,
	int treasureclass);

BOOL TreasureDataTestFlag(
	const TREASURE_DATA *pTreasureData,
	TREASURE_DATA_FLAG eFlag);

BOOL TreasureDataTestFlag(
	GAME *pGame,
	int nTreasure,
	TREASURE_DATA_FLAG eFlag);
	
EXCEL_SET_FIELD_RESULT TreasureExcelParsePickItem(
	struct EXCEL_TABLE * table, 
	const struct EXCEL_FIELD * field,
	unsigned int row,
	BYTE * fielddata,
	const char * token,
	unsigned int toklen,
	int param);
	
EXCEL_SET_FIELD_RESULT TreasureExcelParseNumPicks(
	struct EXCEL_TABLE * table, 
	const struct EXCEL_FIELD * field,
	unsigned int row,
	BYTE * fielddata,
	const char * token,
	unsigned int toklen,
	int param);

BOOL TreasureExcelPostProcess(
	struct EXCEL_TABLE * table);
	
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TreasureSpawn(
	UNIT * unit,
	int nTreasureClassOverride,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult);	
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TreasureSpawnInstanced(
	UNIT *pUnitWithTreasure,
	UNIT *pOperator,
	int nTreasureClassOverride);
#endif

void s_TreasureSpawnClass(
	UNIT * unit,
	int nTreasureClass,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficulty = 0,
	int nLevel = INVALID_ID,
	int nSpawnLevel = INVALID_ID);

#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_TreasureSimplePickQuality(
	GAME * game,
	int nLevel,
	const UNIT_DATA * pItemData);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_TreasureClassHasNoTreasure(
	const TREASURE_DATA * pTreasureData);
#endif

#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
#if !ISVERSION(CLIENT_ONLY_VERSION)
TREASURE_RESULT s_DebugTreasureSpawnUnitType(
	UNIT * unit,
	int unit_type,
	struct ITEM_SPEC * spec,
	struct ITEMSPAWN_RESULT * spawnResult);
#endif
#endif

int TreasureGetFirstItemClass(
	GAME *pGame,
	int nTreasureClass );

BOOL s_TreasureGambleRecreateItem(
	UNIT *&pItem,
	UNIT *pMerchant,
	UNIT *pPlayer);

UNIT* s_TreasureSpawnSingleItemSimple(
	GAME* pGame,
	int treasureclass,
	int nTreasureLevelBoost);

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern float gflMoneyChanceOverride;

#endif // _TREASURE_H_
