//----------------------------------------------------------------------------
// FILE: LevelAreas.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __LEVELAREAS_H_
#define __LEVELAREAS_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UNITS_H_
#include "units.h"
#endif

struct DRLG_PASS;
struct STATS;
struct LEVEL;

namespace MYTHOS_LEVELAREAS
{

const enum KLEVELAREA_TYPE
{
	KLEVELTYPE_NORMAL,
	KLEVELTYPE_PVP,
	KLEVELTYPE_COUNT
};

const enum KLevelAreaScripts
{
	KSCRIPT_LEVELAREA_UNITINIT,
	KSCRIPT_LEVELAREA_AREALOAD,
	KSCRIPT_LEVELAREA_MONSTERINIT,
	KSCRIPT_LEVELAREA_BOSSINIT,
	KSCRIPT_LEVELAREA_COUNT
};

enum ELEVELAREA_DIFFICULTY
{
	LEVELAREA_DIFFICULTY_TO_EASY,	
	LEVELAREA_DIFFICULTY_JUST_RIGHT,
	LEVELAREA_DIFFICULTY_ALMOST_TO_HIGH,	
	LEVELAREA_DIFFICULTY_TO_HIGH,
	LEVELAREA_DIFFICULTY_COUNT
};

const enum ELEVELAREA_SHOWS_ON_MAPS_FLAGS
{
	KLEVELAREA_MAP_FLAGS_ALLOWINVISIBLE = 1
};

const enum LEVEL_AREA_DEFINES
{
	LVL_AREA_MAX_ARRAY =				25,
	LVL_AREA_MAX_RND_BOSSES =			10,
	LVL_AREA_MAX_ITEMS =				10,
	LVL_AREA_MAX_DRLGS_CHOOSE =			5,
	LVL_AREA_MAX_BOSSES_TO_CREATE =		5,
	LVL_AREA_MIN_MAX =					2,
	LVL_AREA_MAX_THEMES =				5,
	LVL_AREA_MAX_HUBS =					6,
	LVL_AREA_MAX_LEVELS_PER_TEMPLATE =	1000,	
	LVL_AREA_MAX_SECTIONS =				5,
	LVL_AREA_MAX_SPAWNCLASSES =			3,
	LVL_AREA_GENERIC_THEMES =			25,
	LVL_AREA_MAX_WEATHERS =				5,
	LVL_AREA_GIVE_ITEM =				5,
	LVL_AREA_MAX_ALWAYS_VISIBLE =		250,
};

//these are the conditionals
const enum LEVEL_AREA_CONDITIONALS
{
	LVL_AREA_CON_X_TIMES,
	LVL_AREA_CON_X_HOURS,
	LVL_AREA_CON_UNIQUE_ITEM,
	LVL_AREA_CON_ELITE_BOSS,
	LVL_AREA_CON_COUNT,
};



//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//STATIC FUNCTIONS and Defines

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const enum ELEVELAREA_THEME_PROPERTIES
{
	KLEVELAREA_THEME_JUST_SHOW,
	KLEVELAREA_THEME_HIDE_IFCOMPLETE,
	KLEVELAREA_THEME_HIDE_IFNOTCOMPLETE,
	KLEVELAREA_THEME_SHOW_IFCOMPLETE,
	KLEVELAREA_THEME_SHOW_IFNOTCOMPLETE,
	KLEVELAREA_THEME_SHOW_IFTASKISACTIVE,
	KLEVELAREA_THEME_HIDE_IFTASKISACTIVE,
	KLEVELAREA_THEME_SHOW_IFTASKISNOTACTIVE,
	KLEVELAREA_THEME_HIDE_IFTASKISNOTACTIVE,
	KLEVELAREA_THEME_COUNT
};
struct LEVEL_AREA_THEME
{
	int				nThemes[ LVL_AREA_MAX_THEMES ];	
	int				nQuestTask[ LVL_AREA_MAX_THEMES ];
	int				nThemeProperty[ LVL_AREA_MAX_THEMES ];
};
struct LEVEL_AREA_SECTION
{
	int				nLevels[ LVL_AREA_MAX_DRLGS_CHOOSE ];
	int				nDepthRange[ LVL_AREA_MIN_MAX ];
	WORD			nLevelCount;
	BOOL			bDontShowDepth;
	int				nLevelAreaNameOverride;
	int				nSpawnClassOverride[ LVL_AREA_MAX_SPAWNCLASSES ];
	WORD			nSpawnClassCount;
	LEVEL_AREA_THEME m_Themes;
	int				nLevelScale[LVL_AREA_MIN_MAX];
	BOOL			bAllowsForDiffThemesForClientVsServer;
	int				nWeathers[ LVL_AREA_MAX_WEATHERS];
	BOOL			bAllowLevelAreaTreasureSpawn;
};

//----------------------------------------------------------------------------
struct LEVEL_AREA_DEFINITION
{
	char			pszName[DEFAULT_INDEX_SIZE];
	
	int				nZone;
	int				nAreaHubs[ LVL_AREA_MAX_HUBS ];
	int				nNumAreaHubs;
	int				nLevelType;

	int				nTransporterAreas[ LVL_AREA_MAX_HUBS ];
	int				nNumTransporterAreas;

	int				nAreaWarpLevelRequirement;

	int				nLevelAreaDisplayNameStringIndex;
		
	WORD			wCode;
	BOOL			bShowsOnMap;
	BOOL			bAlwaysShowsOnMap;
	BOOL			bShowsOnWorldMap;
	BOOL			bIsHostile;
	BOOL			bIsStaticWorld;
	BOOL			bPrimarySave;
	BOOL			bRuneStone;
	BOOL			bAllowsForDiffThemesForClientVsServer;

	int				nCanWarpToSection;

	int				nDungeonsToCreateFromTemplate;

	int				nMinDifficultyLevel;
	int				nMaxDifficultyLevel;
	int				nDifficultyLevelAdditional[LVL_AREA_MIN_MAX];		//monster density
	
	int				nAreaThemes[ LVL_AREA_MAX_THEMES ];

	float			fDifficultyScaleByLevelMin;
	float			fDifficultyScaleByLevelMax;
	float			nLevelDMGScale;
	
	int				nLevelRequirement;	

	int				nLastLevel;
	int				nLinkToLevelArea;

	int				nQuestCompleteShow;
	int				nIconType;	
	int				nItemCheck;
	int				nX;
	int				nY;
	int				nWorldX;
	int				nWorldY;

	BOOL			bIntroArea;
	BOOL			bStartArea;

	int				nPartySizeMinMax[LVL_AREA_MIN_MAX];

	int				nMaxOpenPlayerSize;	
	int				nMaxOpenPlayerSizePVPWorld;	

	int				nPickWeight;	
	

	int				nLevelSizeMultMinMax[LVL_AREA_MIN_MAX];

	int				nChampionMultGlobal;		//for showing champions on the level
	int				nChampionMultLevel[ LVL_AREA_MAX_ARRAY ];
	int				nChampionMult[ LVL_AREA_MAX_ARRAY ];
	BOOL			nChampionNotAllowedOnLastLevel;	//end showing champions variables

	int				nMonsterDensityGlobal[LVL_AREA_MIN_MAX];		//monster density
	int				nMonsterDensityLevels[ LVL_AREA_MAX_ARRAY ];	//monster density
	int				nMonsterDensityMin[ LVL_AREA_MAX_ARRAY ];		//monster density
	int				nMonsterDensityMax[ LVL_AREA_MAX_ARRAY ];		//monster density

	int				nXPModPerNormalMonsters[ LVL_AREA_MAX_ARRAY ];		//Mod XP normal monsters
	int				nXPModLevel[ LVL_AREA_MAX_ARRAY ];					//Mod XP per level



	int				nSpawnClassFloorsToSpawnIn[ LVL_AREA_MAX_ARRAY ];
	int				nSpanwClassFloorsSpawnClasses[ LVL_AREA_MAX_ARRAY ];

	int				nObjectsToSpawn[ LVL_AREA_MAX_ARRAY ];				//objects that will have chance of getting created
	int				nObjectsToSpawnCount;
	int				nChanceForObjectSpawn;							//chance of object getting created
	

	int				nBossSpawnClass;
	int				nBossQuality[ LVL_AREA_MAX_ARRAY ];
	int				nBossQualityCount;
	

	int				nBossEliteSpawnClass;	
	int				nChanceForEliteBoss;
	int				nBossEliteQuality[ LVL_AREA_MAX_ARRAY ];
	int				nBossEliteQualityCount;
	
	PCODE			codeScripts[KSCRIPT_LEVELAREA_COUNT];
		

	int				nCon_VisitXTimesMin;
	int				nCon_VisitXTimesMax;
	int				nCon_AfterXNumberOfHoursMin;
	int				nCon_AfterXNumberOfHoursMax;
	BOOL			bCon_AfterUniqueItemFound;
	BOOL			bCon_AfterEliteBossFound;

	LEVEL_AREA_SECTION			m_Sections[ LVL_AREA_MAX_SECTIONS ];	

	int				nThemesGeneric[ LVL_AREA_GENERIC_THEMES ];	
	int				nThemesQuestTaskGeneric[ LVL_AREA_GENERIC_THEMES ];
	int				nThemePropertyGeneric[ LVL_AREA_GENERIC_THEMES ];
	
	BOOL			bRandomMonsterDropsLinkToObjectCreate;
	int				nRandomMonsterDropsTreasureClass[ LVL_AREA_GIVE_ITEM ];

};

struct LEVEL_AREA_OVERRIDES
{
	const LEVEL_AREA_DEFINITION		*pLevelArea;	
	int								nBossId;
	int								nRoomId;
	BOOL							bOverridesEnabled;
	LEVEL_AREA_CONDITIONALS			eConditional;
	float							fDRLGScaleSize;
	float							fChampionSpawnMult;
	float							fMonsterDensityMult;
	float							nLevelDMGScale;
	float							fDifficultyScaleByLevel;
	int								nSpawnClass;
	int								nLevelXPModForNormalMonsters;
	int								nObjectSpawn;	
	int								nBossesToCreate;
	int								nDungeonDepth;
	int								nCurrentDungeonDepth;
	int								nConditionalParam;
	int								nPartySizeRecommended;
	//int								nDifficulty;
	int								nZoneToAppearAt;
	BOOL							bIsLastLevel;
	BOOL							bIsElite;
	BOOL							bAllowsDiffClientAndServerThemes;
	BOOL							bHasObjectCreatedSomeWhereInDungeon;
};



//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

inline void LevelAreaInitRandByLevelArea( RAND *rand,
										  int nLevelAreaID )
{
	ASSERTX_RETURN( rand, "Need rand" );
	RandInit( *rand, nLevelAreaID );	//That's right the level area ID is the random seed for the level. Got to love it!

}

int LevelAreaGetStartNewGameLevelDefinition(
	void);

int LevelAreaHasHardCodedPosition( int nLevelAreaID );

//The post process for the Level Areas
BOOL LevelAreaExcelPostProcess(
	struct EXCEL_TABLE * table);

int LevelAreaBuildLevelAreaID(int nLevelArea, int nRandomOffset );
int LevelAreaGetLevelAreaIDWithNoRandom(int nLevelArea);
int LevelAreaGetRandomOffsetNumber( int nLevelArea);
BOOL LevelAreaIsRandom( int nLevelArea );
const LEVEL_AREA_DEFINITION *LevelAreaDefinitionGet(int nLevelArea);

//this initializes the level overrides
void LevelAreaInitializeLevelAreaOverrides( LEVEL *pLevel );

BOOL LevelAreaIsKnowByPlayer( UNIT *pPlayer, int nLevelArea );

BOOL LevelAreaShowsOnMap( UNIT *unit, int nLevelArea, int nFlags = 0 );
BOOL LevelAreaShowsOnMapWithZone( UNIT *unit, int nLevelZoneIn, int nLevelArea, int nFlags = 0 );

PGUID LevelAreaGetPartyMemberInArea( UNIT *pUnit, int nLevelArea, GAMEID *pIdGame = NULL);

int LevelAreaGetDifficulty( const LEVEL* pLevel );

ELEVELAREA_DIFFICULTY LevelAreaGetDifficultyComparedToUnit( int nLevelArea, UNIT *pPlayer );

int LevelAreaGetMaxDifficulty( int nLevelArea, UNIT *pPlayer = NULL );
int LevelAreaGetMinDifficulty( int nLevelArea, UNIT *pPlayer = NULL );

//int LevelAreaGetMinPartySizeRecommended( int nLevelArea );
int LevelAreaGetMaxPartySizeRecommended( int nLevelArea );

BOOL LevelAreaIsHostile( int nLevelArea );

//int LevelAreaGetLevelMaxDepth( int nLevelAreaID );

int LevelAreaGetMinPartySizeRecommended( int nLevelAreaID );

int LevelAreaGetIconType( int nLevelArea );

int LevelAreaGetModifiedDRLGSize( const LEVEL *pLevel,
								  const DRLG_PASS *pDRLG,
								  BOOL bMin );

int LevelAreaGetDefRandom( const LEVEL *pLevel,
						   int nLevelAreaID,
						   int nDepth );

int LevelAreaGetMaxDepth( int nLevelArea );

const WCHAR * LevelAreaGetDisplayName( int nLevelArea );

const char * LevelAreaGetDevName( int nLevelArea );

BOOL LevelAreaDontDisplayDepth( int nLevelArea, int nDepth );

int LevelAreaGetEndBossClassID( LEVEL *pLevel,
							    BOOL &bIsElite );


int LevelAreaGetHubAttachedTo( int nLevelAreaID, const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea );

int LevelAreaGetZoneToAppearIn( int nLevelAreaID, const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea );


int LevelAreaGetBossQuality( LEVEL *pLevel,
							 const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea,
							 BOOL bElite );

void LevelAreaFillLevelWithRandomTheme( LEVEL *pLevel,
						   const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea );

void LevelAreaExecuteLevelAreaScriptOnMonster( UNIT *pMonster );
											   


void ExecuteLevelAreaScript( LEVEL *pLevel,
							 KLevelAreaScripts scriptType,
							 UNIT *Unit = NULL,
							 UNIT *pTarget = NULL,
							 STATS *pStats = NULL,
							 int nParam1 = INVALID_ID,
							 int nParam2 = INVALID_ID,
							 int nSkillID = INVALID_ID,
							 int nSkillLevel = INVALID_ID,
							 int nState = INVALID_ID );

//int LevelAreaGetSectionByDepth( int nLevelAreaID, int nDepth );
int LevelAreaGetLevelAreaName( int nLevelAreaID, int nDepth, BOOL bForceGeneralNameOfArea = FALSE );

int LevelAreaGetLevelLink( int nLevelAreaID );

void LevelAreaPopulateThemes( LEVEL *pLevel );

int LevelAreaGetLevelDepthDisplay( int nLevelAreaID,
								   int nLevelDepth  );

//this will return a random levelarea ID from a templated level area ID
int LevelAreaGetRandomLevelID( int nLevelAreaID, RAND &rand );

//this chooses a random levelArea ID used for spawning maps
int s_LevelAreaGetRandomLevelID( GAME *pGame, 
								 BOOL epic, 
								 int nMin, 
								 int nMax, 
								 int nZoneToSpawnFrom, 
								 int nStartingLevelId = INVALID_ID,
								 BOOL bRuneStoneOnly = FALSE );

//returns the weather ID
int LevelAreaGetWeather( LEVEL *pLevel );

//returns if the level area ID can show always
BOOL DoesLevelAreaAlwaysShow( int nLevelAreaID );

//returns the angle of the level to the hub it's connected
int LevelAreaGetAngleToHub( int nLevelAreaID );

//returns the angle of the level to the hub it's connected
int LevelAreaGetAngleToHub( int nLevelAreaID, RAND &rand );

//returns the section index of a level area
int LevelAreaGetSectionByDepth( int nLevelAreaID, int nDepth );

//returns the type of level
KLEVELAREA_TYPE LevelAreaGetLevelType( int nLevelAreaID );  

//returns how many visible level area there are
int LevelAreaGetNumOfLevelAreasAlwaysVisible();

//returns the level area ID by index of visibility 
int LevelAreaGetLevelAreaIDThatIsAlwaysVisibleByIndex( int nIndex );

//returns the warp index that this level area will return to ( in the world there could be 5 to say 500 dungeons that all share the same random
int LevelAreaGetWarpIndexByWarpCount( int nLevelAreaID,
									 int nNumOfWarpsInWorld );

//returns if the level is a static world
BOOL LevelAreaGetIsStaticWorld( int nLevelAreaID );

//returns the number of levels that can be generated off the area
int LevelAreaGetNumOfGeneratedMapsAllowed( int nLevelAreaID );

//returns TRUE if the level area allows primary saves
BOOL LevelAreaAllowPrimarySave( int nLevelAreaID );

//returns TRUE if the level area is a rune stone Level Area ID
BOOL LevelAreaIsRuneStone( int nLevelAreaID );


//returns a random level ID generated by position
int LevelAreaGenerateLevelIDByPos( int nLevelAreaIDBase, int nX, int nY );
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------


} //end namespace
#endif
