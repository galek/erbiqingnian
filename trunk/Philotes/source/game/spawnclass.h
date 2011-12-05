//----------------------------------------------------------------------------
// FILE: spawnclass.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __SPAWNCLASS_H_
#define __SPAWNCLASS_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct GAME;
enum EXCEL_SET_FIELD_RESULT;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum SPAWN_CLASS_CONSTANTS
{
	CODE_SPAWN_ENTRY_SINGLE_COUNT = -1,
};

//----------------------------------------------------------------------------
enum SPAWN_ENTRY_TYPE
{
	SPAWN_ENTRY_TYPE_NONE,				// invalid or unrecognized entry
	SPAWN_ENTRY_TYPE_MONSTER,			// entry is for a specific monster
	SPAWN_ENTRY_TYPE_UNIT_TYPE,			// entry is for a type of units (see unitstypes.xls)
	SPAWN_ENTRY_TYPE_SPAWN_CLASS,		// entry is for a spawn class
	SPAWN_ENTRY_TYPE_OBJECT,			// entry is for an object
};

//----------------------------------------------------------------------------
struct SPAWN_ENTRY
{
	SPAWN_ENTRY_TYPE eEntryType;	// tell us what type of table nIndex refers to
	int nIndex;						// index into excel table		
	PCODE codeCount;				// how many to spawn

//#if ISVERSION(DEVELOPMENT)
	int	nSpawnClassSource;			// used for SPAWN_ENTRIES that are gathered up at runtime
//#endif

	SPAWN_ENTRY::SPAWN_ENTRY( void )
		:	eEntryType( SPAWN_ENTRY_TYPE_NONE ),
			nIndex( INVALID_LINK ),
			codeCount( NULL_CODE )			
//#if ISVERSION(DEVELOPMENT)
								  ,	// <-- don't for get this comma ;)
			nSpawnClassSource( INVALID_LINK )								  
//#endif
			
	{ }		
	
};

//----------------------------------------------------------------------------
struct SPAWN_CLASS_PICK
{
	SPAWN_ENTRY tEntry;
	int nPickWeight;
};

//----------------------------------------------------------------------------
enum SPAWN_CLASS_PICK_TYPE
{
	SPAWN_CLASS_PICK_ALL = 0,	//this was -1 but with the new dictionary pick in Excel it can be moved to 0.
	SPAWN_CLASS_PICK_ONE,
	SPAWN_CLASS_PICK_TWO,
	SPAWN_CLASS_PICK_THREE,
	SPAWN_CLASS_PICK_FOUR,
	SPAWN_CLASS_PICK_FIVE,
	SPAWN_CLASS_PICK_SIX,
	SPAWN_CLASS_PICK_SEVEN,
	SPAWN_CLASS_PICK_EIGHT,
	SPAWN_CLASS_PICK_NINE,		
	MAX_SPAWN_CLASS_CHOICES		// this effectively defines the # of pick columns in spawnclass.xls
};

#define SPAWN_CLASS_INDEX_SIZE ((DEFAULT_INDEX_SIZE) * 2)

//----------------------------------------------------------------------------
struct SPAWN_CLASS_DATA
{
	char szSpawnClass[SPAWN_CLASS_INDEX_SIZE];			// spawn class name
	WORD wCode;											// unique row identifier
	SPAWN_CLASS_PICK_TYPE ePickType;					// pick type for this row (all, one, two, etc)
	BOOL bLevelRemembersPicks;							// when evaluated in a level, the level evaluates this row once and from then on, will always evaluate it the same way
	SPAWN_CLASS_PICK pPicks[MAX_SPAWN_CLASS_CHOICES];	// picks
	int nNumPicks;										// total valid picks in this row
	int nTotalWeight;									// total weight of row
};

//----------------------------------------------------------------------------
struct SPAWN_CLASS_MEMORY
{
	int	nSpawnClassIndex;		// index of spawn class
	int nNumPicks;				// how many picks are in nPicks
	int nPicks[ MAX_SPAWN_CLASS_CHOICES ];	// indicies of the spawn class row we picked
	SAFE_PTR(SPAWN_CLASS_MEMORY*, pNext);
};

//----------------------------------------------------------------------------
typedef void (* PFN_EVALUATE_SPAWN_TREE_CALLBACK)(
	GAME *game,
	int nSpawnClassIndex,
	int nPicks[ MAX_SPAWN_CLASS_CHOICES],	
	int nNumPicks,
	void *pUserData);

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
int SpawnClassEvaluatePicks(
	GAME *game,
	LEVEL *pLevel,
	int nSpawnClassIndex, 
	int nPicks[MAX_SPAWN_CLASS_CHOICES],
	RAND &rand,
	int nMonsterExperienceLevel = -1);

int SpawnClassEvaluatePicks(
	GAME *game,
	LEVEL *pLevel,
	int nSpawnClassIndex, 
	int nPicks[MAX_SPAWN_CLASS_CHOICES],
	int nMonsterExperienceLevel = -1 );

//----------------------------------------------------------------------------
enum SPAWN_CLASS_EVALUATE_FLAGS
{
	SCEF_RANDOMIZE_PICKS_BIT,			// randomize the picked spawn class results
};

typedef BOOL (* SPAWN_CLASS_EVALUATE_FILTER)( GAME * pGame, int nClass, const struct UNIT_DATA * pUnitData );
#define MAX_SPAWNS_AT_ONCE (5)
int SpawnClassEvaluate(
	GAME* game,
	LEVELID idLevel,
	int spawnclass,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter = NULL,
	int nLevelDepthOverride = -1,
	DWORD dwSpawnClassEvaluateFlags = 0);		// see SPAWN_CLASS_EVALUATE_FLAGS
	
const SPAWN_CLASS_DATA* SpawnClassGetData(
	GAME* game,
	int nClass);

const SPAWN_CLASS_DATA* SpawnClassGetDataByName(
	GAME* game,
	const char *pszName);

EXCEL_SET_FIELD_RESULT SpawnClassParseExcel(
	struct EXCEL_TABLE * table, 
	const struct EXCEL_FIELD * field,
	unsigned int row,
	BYTE * fielddata,
	const char * token,
	unsigned int toklen,
	int param);

BOOL SpawnClassExcelPostProcess(
	struct EXCEL_TABLE * table);
	
void SpawnTreeEvaluateDecisions(
	GAME *pGame,
	LEVEL *pLevel,
	int nSpawnClassIndex,
	PFN_EVALUATE_SPAWN_TREE_CALLBACK pfnCallback,
	void *pUserData);

int SpawnEntryEvaluateCount(
	GAME *pGame,
	const SPAWN_ENTRY *pSpawn);

void s_SpawnClassGetPossibleMonsters( 
	GAME* pGame, 
	LEVEL* pLevel, 
	int nSpawnClass,
	int *pnMonsterClassBuffer,
	int nBufferSize,
	int * pnPossibleMonsterCount,
	BOOL bUseLevelSpawnClassMemory);
		
#endif
