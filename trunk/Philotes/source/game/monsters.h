//----------------------------------------------------------------------------
// monsters.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _MONSTERS_H_
#define _MONSTERS_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#include "excel.h"
#endif

#ifndef _UNIT_PRIV_H_
#include "unit_priv.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
enum
{
	NUM_MONSTER_COMBAT_RESULTS =		5,
	MAX_MONSTER_AFFIX =					3,		// max # of affixes on champion monsters
	MAX_MONSTER_CLASSES_FOR_NAME =		4,		// max # of monster classes a proper name can be restricted to
};

#define MONSTERS_SPAWN_IN_AIR_ALTITUDE 2.25f

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct MONSTER_LEVEL
{
	int		nLevel;
	int		nHitPoints;
	int		nExperience;
	int		nDamage;
	int		nAttackRating;
	int		nArmor;
	int		nArmorBuffer;
	int		nArmorRegen;
	int		nToHitBonus;
	int		nShield;
	int		nShieldBuffer;
	int		nShieldRegen;
	int		nSfxAttack;
	int		nSfxStrengthPercent;
	int		nSfxDefense;
	int		nInterruptAttack;
	int		nInterruptDefense;
	int		nStealthDefense;
	int		nAIChangeAttack;
	int		nAIChangeDefense;
	int		nCorpseExplodePoints;
	int		nMoneyChance;
	int		nMoneyMin;
	int		nMoneyDelta;	
	int		nStrength;
	int		nDexterity;
	int		nStatPoints;
	PCODE	codeItemLevel;
};

//----------------------------------------------------------------------------
#define NUM_PLAYER_VS_MONSTER_INCREMENT_PCTS 2
struct MONSTER_SCALING
{
	int									nPlayerCount;
	float								fPlayerVsMonsterIncrementPct[NUM_PLAYER_VS_MONSTER_INCREMENT_PCTS];
	int									nMonsterVsPlayerIncrementPct;
	int									nExperience;
	int									nTreasure;
};

//----------------------------------------------------------------------------
enum MONSTER_QUALITY_TYPE
{
	MQT_ANY,			// allow any type (champions or uniques)
	MQT_CHAMPION,		// a champion is an enhanced monster that is *not* a unique (rare, epic, legendary, mutant)
	MQT_TOP_CHAMPION,	// the top range of a chamption monster, but not a unique (mutant)
	MQT_UNIQUE,			// a unique is a named, hand crafted, special monster
};

//----------------------------------------------------------------------------
struct MONSTER_QUALITY_DATA
{
	char	szMonsterQuality[DEFAULT_INDEX_SIZE];		// quality string
	WORD	wCode;										// code for saving
	int		nRarity;									// general rarity
	int		nNameColor;									// display name color
	int		nDisplayNameStringKey;						// display name string key
	int		nChampionFormatStringKey;					// display string for champion name formatting
	BOOL	bPickProperName;							// monsters of this quality pick unique names	
	MONSTER_QUALITY_TYPE eType;							// type grouping of this quality
	APPEARANCE_SHAPE_CREATE tAppearanceShapeCreate;		// shape of champions
	int		nState;										// state quality causes (we may want to move this to the monsters.xls so we can set a different one for every monster)	
	float	flMoneyChanceMultiplier;					// money drop chance is multiplied by this
	float	flMoneyAmountMultiplier;					// money drop value is multiplied by this
	int		nTreasureLevelBoost;						// treasure level +/- this level
	float	flHealthMultiplier;							// min/max of hit gets this inherent bonus
	float	flToHitMultiplier;							// improve tohit calc
	PCODE	codeMinionPackSize;							// how many minions appear with the champion pack
	PCODE	codeProperties[MAX_MONSTER_AFFIX];			// properties
	int		nAffixCount;								// affix count
	int		nAffixType[MAX_MONSTER_AFFIX];				// affixes on this quality of monster
	PCODE	codeAffixChance[MAX_MONSTER_AFFIX];			// probability of affix	
	int		nExperienceMultiplier;						// how much to multiply the base exp
	int		nLuckMultiplier;							// used by Tugboat for increasing luck
	int		nMonsterQualityDowngrade;					// downgrade quality
	int		nMinionQuality;								// downgrade quality
	BOOL	bShowLabel;									// show name label in world
	int		nTreasureClass;								// a treasure class for this monste type
};

//----------------------------------------------------------------------------
enum MONSTER_SPEC_FLAGS
{
	MSF_ONLY_AFFIX_SPECIFIED =			MAKE_MASK(0),	// don't pick affixes for champions, use the affix array provided
	MSF_FORCE_CLIENT_ONLY =				MAKE_MASK(1),	// force the monster to be client-side (for video head mainly)
	MSF_KEEP_NETWORK_RESTRICTED =		MAKE_MASK(2),	// do not enable the monster for network sending
	MSF_DONT_DEACTIVATE_WITH_ROOM =		MAKE_MASK(3),	// monster deactivates with level, but not room
	MSF_DONT_DEPOPULATE =				MAKE_MASK(4),	// monster doesn't depopulate with room
	MSF_NO_DATABASE_OPERATIONS =		MAKE_MASK(5),	// do not do any database operations
	MSF_IS_LEVELUP =					MAKE_MASK(6),	// pet levelup
};

//----------------------------------------------------------------------------
void MonsterSpecInit(
	struct MONSTER_SPEC &tSpec);
	
//----------------------------------------------------------------------------
struct MONSTER_SPEC
{
	UNITID id;										// existing id
	PGUID guidUnit;									// existing GUID
	int nClass;										// monster class
	int nExperienceLevel;							// monster experience level
	int nMonsterQuality;							// monster quality
	int nTeam;										// team (INVALID_TEAM)
	ROOM *pRoom;									// room where to spawn
	struct ROOM_PATH_NODE * pPathNode;				// room path node to spawn at
	VECTOR vPosition;								// position to spawn at
	VECTOR vDirection;								// direction to face
	int nWeaponClass;								// weapon class to carry maybe?
	UNIT *pOwner;									// owner
	float flVelocityBase;							// 0.0f
	float flScale;									// -1.0f
	VECTOR *pvUpDirection;							// used only if MSF_USE_VUP_BIT is set	
	APPEARANCE_SHAPE *pShape;						// used only if MSF_USE_SHAPE_BIT is set	
	DWORD dwFlags;									// see MOSNTER_SPEC_FLAGS
	int nAffix[ MAX_MONSTER_AFFIX ];				// affix to apply to monster
	
	MONSTER_SPEC::MONSTER_SPEC( void )	{ MonsterSpecInit( *this ); }
	
};

//----------------------------------------------------------------------------
struct MONSTER_NAME_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];		// name
	WORD wCode;								// code for saving
	int nStringKey;							// index in string table of string for name
	int nMonsterNameType;					// matches Monster Name Type column value for monsters that only use names from this type, or INVALID_LINK
	BOOL bIsNameOverride;					// use this string key with UnitSetNameOverride instead of MonsterProperNameSet, it's a whole name, title+type
};

//----------------------------------------------------------------------------
struct MONSTER_NAME_TYPE_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];		// name
	WORD wCode;								// code for saving
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

UNIT * MonsterInit(
	GAME * game,
	MONSTER_SPEC &tSpec);

UNIT* s_MonsterSpawn(
	GAME *pGame,
	MONSTER_SPEC &tSpec);

void MonsterSetFuse(
	UNIT * pMonster,
	int nTicks);

void MonsterUnFuse(
	UNIT * pMonster);

int MonsterGetFuse(
	UNIT * pMonster);

UNIT* MonsterSpawnClient(
	GAME* game,
	int nClass,
	ROOM* room,
	const VECTOR& vPosition,
	const VECTOR& vFaceDirection);

BOOL ExcelMonstersPostProcess(
	struct EXCEL_TABLE * table);

void MonsterSpawnPickClasses(
	GAME * game,
	struct LEVEL* level);

int MonsterGetQuality(
	UNIT *pUnit);
	
BOOL MonsterIsChampion(
	UNIT *pUnit);
		
int MonsterQualityPick(
	GAME *pGame,
	MONSTER_QUALITY_TYPE eType,
	RAND *rand = NULL );
		
int MonsterProperNamePick(
	GAME *pGame,
	int nMonsterClass = INVALID_LINK,
	RAND *rand = NULL );

int MonsterNumProperNames(
	void);
	
void MonsterProperNameSet(
	UNIT *pMonster,
	int nNameIndex,
	BOOL bForce);

int MonsterUniqueNameGet(
	UNIT *pMonster);
	
void MonsterGetUniqueNameWithTitle(
	int nNameIndex,
	int nMonsterClass,
	WCHAR *puszNameWithTitle,
	DWORD dwNameWithTitleSize,
	DWORD *pdwAttributesOfString = NULL);

void MonsterStaggerSpawnPosition(
	GAME *pGame,
	VECTOR *pPosition,
	const VECTOR *pPositionCenter,
	const UNIT_DATA *pMonsterData,
	int nNumSpawnedSoFar);

int MonsterChampionEvaluateMinionPackSize(
	GAME *pGame, 
	const int nMonsterQuality);

int MonsterChampionMinionGetQuality( 
	GAME *pGame, 
	int nMonsterQuality);

int MonsterEvaluateMinionPackSize(
	GAME *pGame, 
	const UNIT_DATA *pUnitData);

float MonsterGetMoneyChanceMultiplier( 
	UNIT *pUnit);
	
float MonsterGetMoneyAmountMultiplier( 
	UNIT *pUnit);

int MonsterGetTreasureLevelBoost(
	UNIT *pUnit);

int MonsterGetAffixes( 
	UNIT *pUnit, 
	int *pnAffix, 
	int nMaxAffix);

const MONSTER_NAME_DATA	*MonsterGetNameData(
	int nIndex);

MONSTER_QUALITY_TYPE MonsterQualityGetType(
	int nMonsterQuality);
	
BOOL MonsterQualityShowsLabel( 
	int nMonsterQuality);

#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_MonsterInitStats(
	GAME * game,
	UNIT * unit,
	const MONSTER_SPEC &tSpec);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

				
//----------------------------------------------------------------------------
// ACCESSOR FUNCTIONS
//----------------------------------------------------------------------------
#ifdef _DEBUG
#define MonsterGetData(g, c)		MonsterGetDataDbg(g, c, __FILE__, __LINE__)
inline const struct UNIT_DATA * MonsterGetDataDbg(
	GAME* game,
	int nClass,
	const char* file,
	int line)
{
	if ((unsigned int)nClass >= ExcelGetCount(game, DATATABLE_MONSTERS))
	{
		ASSERTV(0, "MonsterGetData() exceeds bounds from file:%s  line:%d", file, line);
	}
	return (const struct UNIT_DATA *)ExcelGetData(game, DATATABLE_MONSTERS, nClass);
}
#else
inline const struct UNIT_DATA* MonsterGetData(
	GAME* game,
	int nClass)
{
	return (const struct UNIT_DATA*)ExcelGetData(game, DATATABLE_MONSTERS, nClass);
}
#endif 

inline int MonsterGetByCode(
	GAME* game,
	DWORD dwCode)
{
	return ExcelGetLineByCode(game, DATATABLE_MONSTERS, dwCode);
}

inline const MONSTER_LEVEL* MonsterLevelGetData(
	GAME* game,
	int level)
{
	return (const MONSTER_LEVEL *)ExcelGetData(game, DATATABLE_MONLEVEL, level);
}

inline const MONSTER_SCALING * MonsterGetScaling(
	GAME * game,
	int player_count)
{
	return (const MONSTER_SCALING *)ExcelGetData(game, DATATABLE_MONSCALING, player_count);
}

inline int MonsterGetMaxLevel(
	GAME* game)
{
	return ExcelGetCount(game, DATATABLE_MONLEVEL);
}

inline const MONSTER_QUALITY_DATA *MonsterQualityGetData(
	GAME *game,
	int nQuality)
{
	return (const MONSTER_QUALITY_DATA *)ExcelGetData(game, DATATABLE_MONSTER_QUALITY, nQuality);
}

BOOL MonsterCanSpawn(
	int nMonsterClass,
	LEVEL *pLevel,
	int nMonsterExperienceLevel = -1);

void MonsterAddPossible(
	LEVEL *pLevel,
	int nSpawnClass,
	int nMonsterClass,
	int* pnBuffer,
	int nBufferSize,
	int* pnBufferCount);

void s_MonsterVersion(
	UNIT *pMonster);

void s_MonsterFirstKill(
	UNIT * pKiller,
	UNIT * pVictim );

#endif // _MONSTERS_H_