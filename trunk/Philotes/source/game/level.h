//----------------------------------------------------------------------------
// level.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma once
#endif
#ifdef	_LEVEL_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _LEVEL_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _SUBLEVEL_H_
#include "sublevel.h"
#endif

#ifndef _BOUNDINGBOX_H_
#include "boundingbox.h"
#endif

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __SPAWNCLASS_H_
#include "spawnclass.h"
#endif

#ifndef _S_MESSAGE_H_
#include "s_message.h"
#endif

#ifndef _ROOM_H_
#include "room.h"
#endif

#ifndef __LEVELAREAS_H_
#include "LevelAreas.h"
#endif

#ifndef __LEVELAREANAMEGEN_H_
#include "LevelAreaNameGen.h"
#endif

#ifndef _PAKFILE_H_
#include "pakfile.h"
#endif

#ifndef _QUADTREE_H
#include "quadtree.h"
#endif

#ifndef _PROXMAP_H_
#include "ProxMap.h"
#endif

#ifndef __DIFFICULTY_H_
#include "difficulty.h"
#endif

#ifndef __QUESTSTRUCTS_H_
#include "QuestStructs.h"
#endif

#ifndef __ANCHORMARKERS_H_
#include "AnchorMarkers.h"
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define LEVEL_NAME_SIZE						128
#define LEVEL_MAX_POSSIBLE_UNITS 			128
#define MAX_LEVEL_MAP_CONNECTIONS			8
#define MAX_LEVEL_LINKS						100			// was 32, Mythos has static worlds where this number can easily go above 32
#define MAX_LEVEL_DISTRICTS					3			// if you increase this, add columns to the excel table
#define MAX_LEVEL_DRLGS_CHOICES				5			// if you increase this, add columns to the excel table
#define MAX_SUBLEVELS						MAX_SUBLEVEL_DESC
#define MAX_HELLRIFT_SUBLEVEL_CHOICES		5
#define MAX_DRLG_THEMES						20
#define MAX_THEMES_FROM_WEATHER				4			// This is a DUPLICATE of the value in weather.h!  Ugh.  Ick.  Phooey.
// Themes can come from:
// MAX_DRLG_THEMES - DRLG_DEFINITION
// MAX_THEMES_FROM_WEATHER - Weather
// 1 - Randomly selected from LEVEL_DEFINITION
#define MAX_THEMES_PER_LEVEL				(MAX_DRLG_THEMES + MAX_THEMES_FROM_WEATHER + 1)


//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct SPAWN_CLASS_MEMORY;
struct GAME;
struct BOUNDING_BOX;
struct DATA_TABLE;
struct DRLG_LEVEL_DEFINITION;
struct GAMECLIENT;
struct UNIT;
struct DRLG_PASS;
struct ROOM;
struct MSG_SCMD_SET_LEVEL;
struct DISTRICT;
enum TARGET_TYPE;
struct ENGINE_ROOM;
class CVisibilityMesh;

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum SRVLEVELTYPE
{
	SVRLEVELTYPE_INVALID = -1,
	
	SRVLEVELTYPE_DEFAULT,								// regular instanced adventuring level
	SRVLEVELTYPE_TUTORIAL,								// tutorial forces separate instance per player
	SRVLEVELTYPE_MINITOWN,								// minitown shared instance for server
	SRVLEVELTYPE_BIGTOWN,								// bigtown
	SRVLEVELTYPE_CUSTOM,								// custom game, behaves similar to a minitown. metagame code in customgame.cpp
	SRVLEVELTYPE_PVP_PRIVATE,							// private PvP game, special code warps players in. don't reserve after everyone leaves
	SRVLEVELTYPE_PVP_CTF,								// Player vs. Player Capture the Flag game. behaves similar to minitown. metagame code in ctf.cpp
	SRVLEVELTYPE_OPENWORLD,								// hostile shared instance for server
};


enum LEVEL_FLAG
{
	LEVEL_ACTIVE_BIT,									// level is currently active
	LEVEL_POPULATED_BIT,								// DRLG has been run on this level and it's contents "created"
	LEVEL_ACTIVATED_ONCE_BIT,							// level has been activated at least once
		
	NUM_LEVEL_FLAGS										// keep this last please
};


enum WORLD_MAP_STATE
{
	WORLDMAP_HIDDEN = 0,								// level not drawn in world map
	WORLDMAP_REVEALED,									// level drawn in outline form
	WORLDMAP_VISITED,									// level drawn in solid form
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct WAIT_FOR_LOAD_CALLBACK_DATA
{
	DWORD				dwFlags;
	int					nCount;
	int					nLevelDefinition;
	int					nDRLGDefinition;
};

//----------------------------------------------------------------------------
struct LEVEL_SPAWN_FREE_ZONE 
{
	VECTOR				vPosition;
	float				fRadiusSquared;
};

//----------------------------------------------------------------------------
struct LEVEL_ENVIRONMENT_DEFINITION
{
	char				pszName[DEFAULT_INDEX_SIZE];
	int					nPathIndex;
	char				pszEnvironmentFile[DEFAULT_FILE_SIZE];
};

//----------------------------------------------------------------------------
struct LEVEL_DRLG_CHOICE
{
	EXCEL_STR		(pszName);
	float			fNamedMonsterChance;	// percentage chance to spawn a named monster [0-100]
	int				nBaseRow;
	int				nLevel;
	int				nDifficulty;
	int				nDRLG;					// the DRLG def 
	int				nSpawnClass;			// spawn class to use for level
	int				nNamedMonsterClass;		// monster class to use for spawning a named monster
	int				nDRLGWeight;			// weight of this selection
	int				nMusicRef;				// music
	int				nEnvironmentOverride;
	float			fEnvRoomDensityPercent;	// environmental/extra spawn class density
	int				nEnvSpawnClass;			// environmental/extra spawn class
	WORD			wCode;
};

//----------------------------------------------------------------------------
struct LEVEL_DEFINITION
{
	char				pszName[DEFAULT_INDEX_SIZE];
	char				pszDisplayName[DEFAULT_INDEX_SIZE];
	WORD				wCode;
	int					nBitIndex;
	int					nIndex;
	
	int					nSubLevelDefault;		// the default sublevel type for all rooms in this level
	
	int					nPrevLevel;
	int					nNextLevel;

	int					nLevelDisplayNameStringIndex;
	int					nFloorSuffixName;
	int					nFinalFloorSuffixName;

	float				fAutomapWidth;
	float				fAutomapHeight;

	float				fVisibilityTileSize;
	BOOL				bClientDiscardRooms;
	BOOL				bTown;
	BOOL				bRTS;
	BOOL				bAlwaysActive;	
	BOOL				bStartingLocation;
	int					nLevelRestartRedirect;
	BOOL				bCheatStartingLocation;
	BOOL				bPortalAndRecallLoc;
	BOOL				bFirstPortalAndRecallLoc;
	BOOL				bSafe;
	int					nSubLevelDefTownPortal;
	BOOL				bTutorial;
	BOOL				bDisableTownPortals;
	BOOL				bOnEnterClearBadStates;
	BOOL				bPlayerOwnedCannotDie;
	BOOL				bHardcoreDeadCanVisit;
	BOOL				bCanFormAutoParties;
	
	float				flCameraDollyScale;

	float				fMinPathingZ;
	float				fMaxPathingZ;
	BOOL				bContoured;

	float				fVisibilityDistanceScale;
	float				fVisibilityOpacity;
	float				fFixedOrientation;
	BOOL				bOrientedToMap;
	BOOL				bAllowRandomOrientation;

	int					nHellriftChance;		// % chance (0-100) there is a hellrift here
	int					nSubLevelHellrift[ MAX_HELLRIFT_SUBLEVEL_CHOICES ];		// sublevel definitions to pick from for hellrift in this level

	int					nLevelMadlibNames[ MYTHOS_LEVELAREAS::MYTHOS_MADLIB_COUNT ];

	int					nSubLevel[ MAX_SUBLEVELS ];	// sublevels in level
			
	BOOL				bAutoWaypoint;
	BOOL				bMultiplayerOnly;
	BOOL				bUseTeamColors;
	int					nWaypointMarker;
	SRVLEVELTYPE		eSrvLevelType;
	unsigned int		nMaxPlayerCount;

	PCODE				codeMonsterLevel[ NUM_DIFFICULTIES ];
	BOOL				bMonsterLevelFromCreatorLevel;

	BOOL				bMonsterLevelFromActivatorLevel;	
	int					nMonsterLevelActivatorDelta;	

	int					nSelectRandomThemePct;

	int					nPartySizeRecommended;

	int					nQuestSpawnClass;
	int					nInteractableSpawnClass;

	int					nStringEnter;
	int					nStringLeave;

	float				flChampionSpawnRate;
	float				flUniqueSpawnChance;

	int					nMapX;
	int					nMapY;
	
	BOOL				bFirstLevel;				// new players start at this location - global flag enables this
	BOOL				bFirstLevelCheating;		// when we are working on the game, this is the first level that we start at
	int					nPlayerExpLevel;			// this is the experience level a new player will be boosted to when they start the game
	int					nBonusStartingTreasure;		// used for boosting a starting player up.  If this is present and this is the starting level a new player will be given this treasure
	int					nSequenceNumber;			// in general, we visit level seq 1 before 2 before 3, etc
	int					nStartingGold;
	int					nAct;						// the quest Act this level is first associated with

	int					nMapRow;
	int					nMapCol;			
	char				szMapFrame[256];
	char				szMapFrameUnexplored[256];
	int					nMapColor;
	int					eMapLabelPos;
	float				fMapLabelXOffs;
	float				fMapLabelYOffs;	
	int					pnMapConnectIDs[MAX_LEVEL_MAP_CONNECTIONS];
	
	int					nAdventureChancePercent;
	PCODE				codeNumAdventures;	// possible adventures in level

	BOOL				bRoomResetEnabled;	
	int					nRoomResetDelayInSeconds;	// when a room in this level is deactivated, after this long it will "reset"
	int					nRoomResetMonsterPercent;	// a room will only be auto reset if more than this many monsters have been killed

	float				flMonsterRoomDensityMultiplier;		
	BOOL				bContentsAlwaysRevealed;	// all rooms of this level are automatically populated and all clients know about every room

	PCODE				codePlayerEnterLevel;
	BOOL				bAllowOverworldTravel;

	// these are filled by code
	SAFE_PTR( int*, pnAchievements );
	USHORT		nNumAchievements;

};

//----------------------------------------------------------------------------
struct AUTOMAP_SAVE_NODE
{
	DWORD				idRoom;
	unsigned int		nNodeCount;
	DWORD *				pNodeFlags;
};

//----------------------------------------------------------------------------
struct LEVEL_LINK
{
	UNITID				idUnit;			// linking unit (currently only warps)
	LEVELID				idLevel;		// level it links to
};

//----------------------------------------------------------------------------
struct LEVEL_POSSIBLE_UNITS
{
	int nClass[LEVEL_MAX_POSSIBLE_UNITS];
	int nCount;
};

class CRoomBoundsRetriever
{

	public:
	
		const BOUNDING_BOX& operator()(ROOM* room)
		{		
			m_BoundsReturn.vMin = VECTOR(room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX, room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY, 0);
			m_BoundsReturn.vMax = VECTOR(room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX, room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY, 0);
			
			return m_BoundsReturn;
		}
		
		// A temporary storage used for the return value.		
		BOUNDING_BOX m_BoundsReturn;
};

//----------------------------------------------------------------------------
struct LEVEL
{
	char						m_szLevelName[LEVEL_NAME_SIZE];	// convenient to see in the debugger
	GAME *						m_pGame;			// game this level is in
	const LEVEL_DEFINITION *	m_pLevelDefinition;	// levels.xls row
	struct ROOM *				m_pRooms;			// list of rooms
	struct LEVEL_COLLISION *	m_Collision;
	LEVEL_SPAWN_FREE_ZONE *		m_pSpawnFreeZones;	// list of spawn-free zones in the level
	SPAWN_CLASS_MEMORY *		m_pSpawnClassMemory;// memory of random rolls when evaluating spawn classes
	DISTRICT *					m_pDistrict[MAX_LEVEL_DISTRICTS];	// level districts
	AUTOMAP_SAVE_NODE *			m_pAutoMapSave;		// loaded automap
	CQuadtree<ROOM *, CRoomBoundsRetriever>*			m_pQuadtree;
	CVisibilityMesh	*			m_pVisibility;
	PointMap *					m_pUnitPosProxMap;	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	const struct WEATHER_DATA *	m_pWeather;
#ifdef HAVOK_ENABLED
	class hkWorld *				m_pHavokWorld;
#endif
	MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES		m_LevelAreaOverrides;
	QUEST_LEVEL_INFO_TUGBOAT					m_QuestTugboatInfo;
	BOUNDING_BOX				m_BoundingBox;		// bounding box around whole level
	
	SIMPLE_DYNAMIC_ARRAY<UNITID>	m_UnitsOfInterest;	//units of Interest

	LEVEL_LINK					m_LevelLink[MAX_LEVEL_LINKS];  // warp links to other levels
	SUBLEVEL					m_SubLevels[MAX_SUBLEVELS];		// sublevels of level
	LEVEL_POSSIBLE_UNITS		m_tPossibleUnits[NUM_GENUS];

	RAND						m_randSeed;			// seed

	UNITID						m_idWarpCreator;	// warp that created this level (if any)	
	LEVELID						m_idLevel;			// one up number for game
	LEVELID						m_idLevelNext;		// level used for next level warps
	LEVELID						m_idLevelPrev;		// level used for prev level warps
	CHANNELID					m_idChatChannel;	// communication channel
	DWORD						m_dwUID;			// uid
	DWORD						m_dwFlags;			// see LEVEL_FLAGS		
	DWORD						m_dwSeed;			// starting seed
	DWORD						m_dwLastRoomUpdateTick;

	BOOL						m_bChannelRequested;// TRUE if a request for a chat channel has been sent
	BOOL						m_bNeedRoomUpdate;
	BOOL						m_bThemesPopulated;

	BOOL						m_nMonsterLevelOverride;

	float						m_flArea;			// total level area taken by all rooms
	float						m_CellWidth;

	BOOL						m_bPVPWorld;			// PVP world!

	int							m_nLevelDef;		// link to levels.xls row
	int							m_nDRLGDef;			// link to levels.xls (levels_drlgs tab) row
	int							m_nRoomCount;		// number of rooms
	int							m_nDepth;			// depth within the dungeon
	int							m_nDifficultyOffset;//
	int							m_nAreaLevel;		//
	int							m_nLevelAreaId;		// dungeon area this level is part of
	MYTHOS_ANCHORMARKERS::cAnchorMarkers				m_AnchorMakers;			//Anchor Markers in Level
	int							m_bVisited;			// has a player arrived here yet?
	int							m_nChampionCount;	// # of champions spawned in level	
	int							m_nNumSpawnFreeZones;	// # of spawn-free zones in the level
	int							m_nNumLevelLinks;	// valid entries in m_LevelLink		
	int							m_nNumDistricts;	// number of districts or neighborhoods
	int							m_nQuestSpawnClass;
	int							m_nInteractableSpawnClass;
	int							m_nNumSubLevels;				// number of sub levels in level
	int							m_nTreasureClassInLevel;		// treasure class from treasure levels
	int							m_nThemeIndexCount;
	int							m_nPartySizeRecommended;
	float						m_fDistrictNamedMonsterChance[MAX_LEVEL_DISTRICTS]; // percentage chance to spawn a named monster [0-100]			
	int							m_nDistrictSpawnClass[MAX_LEVEL_DISTRICTS];  // spawn class for districts			
	int							m_nDistrictNamedMonsterClass[MAX_LEVEL_DISTRICTS];  // monster class to use for spawning a named monster		
	int							m_pnThemes[MAX_THEMES_PER_LEVEL];
	
	unsigned int				m_nAutoMapSaveCount;	// number of rooms
	unsigned int				m_nPlayerCount;		// number of players currently in the level
};


//----------------------------------------------------------------------------
void LevelTypeInit(
	struct LEVEL_TYPE &tLevelType);

//----------------------------------------------------------------------------
struct LEVEL_TYPE
{

	// this structure is meant to be a single "thing" that we can use to 
	// refer to a type of a level, and since hellgate and tugboat have different
	// requirements for that, we're wrapping that up with this structure here
	
	int nLevelDef;			// hellgate - level definition row
	int nLevelArea;			// tugboat - level area row
	int nLevelDepth;		// tugboat - level depth
	int nItemKnowingArea;	// tugboat - item knowing area
	int nAreaLevel;			// tugboat - difficulty level of random map (what a horrible name ... nAreaLevel vs nLevelArea ... how about making it nLevelAreaDifficultyLevel or something?)
	BOOL bPVPWorld;			// tugboat - is this a PVP world level?
	
	LEVEL_TYPE::LEVEL_TYPE( void ) { LevelTypeInit( *this ); }
	
};

//----------------------------------------------------------------------------
struct LEVEL_GOING_TO
{
	GAMEID idGame;					// game id this client is currently "going to"
	LEVEL_TYPE tLevelType;			// the level type of the destination level they are "going to"
	
	LEVEL_GOING_TO::LEVEL_GOING_TO( void )
		:	idGame( INVALID_ID )
	{ 
		LevelTypeInit( tLevelType );
	}
};

//----------------------------------------------------------------------------
#define MAX_LEVEL_THEME_ISA 8
#define MAX_LEVEL_THEME_ALLOWED_STYLES 16
struct LEVEL_THEME
{
	char	pszName[DEFAULT_INDEX_SIZE];
	int		nIsA[MAX_LEVEL_THEME_ISA];
	BOOL	bDontDisplayInLayoutEditor;
	BOOL	bHighlander;
	char	pszEnvironment[DEFAULT_INDEX_SIZE];
	int		nEnvironmentPriority;
	int		nAllowedStyles[MAX_LEVEL_THEME_ALLOWED_STYLES];
	int		nGlobalThemeRequired;
};

//----------------------------------------------------------------------------
struct DRLG_SPAWN
{
	float flRoomDensityPercent;			// how dense to populate the rooms
	int nSpawnClass;					// what to populate them with
};

//----------------------------------------------------------------------------
struct DRLG_DEFINITION
{
	char		pszName[DEFAULT_INDEX_SIZE];
	char		pszDRLGRuleset[DEFAULT_INDEX_SIZE];
	int			nDRLGDisplayNameStringIndex;
	int			nPathIndex;
	int			nStyle;
	int			nThemes[MAX_DRLG_THEMES];
	int			nWeatherSet;
	int			nEnvironment;

	BOOL		bPopulateAllVisible;
	
	float		flChampionSpawnRateOverride;
	float		flChampionZoneRadius;
	float		flMarkerSpawnChance;
	float		flMonsterRoomDensityPercent;
	float		flRoomMonsterChance;

	DRLG_SPAWN	tDRLGSpawn[ NUM_DRLG_SPAWN_TYPE ];

	float		flMaxTreasureDropRadius;  // you must be this close to a monster for it to drop treasure for you

	int			nRandomState;
	int			nRandomStateRateMin;
	int			nRandomStateRateMax;
	int			nRandomStateDuration;
	BOOL		bCanUseHavokFX;	//Moving from LEVEL_DEFINITION
	BOOL		bForceDrawAllRooms;
	BOOL		bIsOutdoors;
};

//----------------------------------------------------------------------------
struct LEVEL_CONNECTION
{
	LEVEL* pLevel;	// the connected level
	UNITID idWarp;	// the warp that is the passageway to the connected level
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------
BOOL ExcelLevelsPostProcess(
	struct EXCEL_TABLE * table);

BOOL ExcelLevelsFree(
	struct EXCEL_TABLE * table);
	
void ExcelLevelFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata);

int LevelGetStartNewGameLevelDefinition(
	void);


BOOL LevelIsActionLevelDefinition(
	const LEVEL_DEFINITION *pLevelDefinition);

BOOL LevelIsActionLevel(
	LEVEL *pLevel);

BOOL LevelAllowsUnpartiedPlayers(
	LEVEL *pLevel);
	
//----------------------------------------------------------------------------
struct LEVEL_SPEC
{

	LEVELID idLevel;						// level id to use for new level	
	LEVEL_TYPE tLevelType;					// level definition or area+depth
	
	int nDRLGDef;							// DRLG for level
	DWORD dwSeed;							// seed for DRLG
	BOUNDING_BOX *pBoundingBox;				// bounding box for level
	BOOL bPopulateLevel;					// level should be populated after created
	BOOL bClientOnlyLevel;					// is a client only level
	LEVEL *pLevelCreator;					// creator level that spawned this level (via warp)
	UNITID idWarpCreator;					// warp that created level
	UNITID idActivator;						// id of the unit that entered the level. used to spawn quests or not...	
	BOOL nTreasureClassInLevel;				// this treasure class is randomly populated around the level
	int nDifficultyOffset;					// difficulty (tugboat)
	float flCellWidth;						// width of cells (tubgoat)
	
	LEVEL_SPEC::LEVEL_SPEC( void )
		:	idLevel( INVALID_ID ),
			nDRLGDef( INVALID_LINK ),
			dwSeed( 0 ),
			pBoundingBox( NULL ),
			bPopulateLevel( FALSE ),
			bClientOnlyLevel( FALSE ),
			pLevelCreator( NULL ),
			idWarpCreator( INVALID_ID ),
			idActivator( INVALID_ID ),
			nTreasureClassInLevel( INVALID_LINK ),
			nDifficultyOffset( 0 ),
			flCellWidth( 0.0f )
	{ }
	
};
	
LEVEL* LevelAdd(
	GAME *pGame,
	const LEVEL_SPEC &tSpec);

void LevelFree(
	GAME* pGame,
	LEVELID idLevel);

void LevelAddSpawnFreeZone(
	GAME *pGame,
	LEVEL *pLevel,
	const VECTOR &vPosition,
	const float flRadius,
	BOOL bMarkPathNodes);

BOOL LevelPositionIsInSpecificSpawnFreeZone(
	const LEVEL *pLevel,
	const LEVEL_SPAWN_FREE_ZONE *pZone,
	const VECTOR *pvPositionWorld);

BOOL LevelPositionIsInSpawnFreeZone(
	const LEVEL *pLevel,
	const VECTOR *pvPositionWorld);
	
BOOL LevelHasRooms(
	GAME* game,
	LEVELID idLevel);

BOOL LevelIsPopulated(
	LEVEL *pLevel);

int LevelGetMonstersSpawnedRoomsPercent(
	LEVEL *pLevel);

#if ISVERSION(DEVELOPMENT)
void s_DebugLevelActivateAllRooms(
	LEVEL *pLevel,
	UNIT *pActivator);
#endif
	
void LevelAddRoom(
	GAME* game,
	LEVEL* level,
	ROOM* room);

void LevelRemoveRoom(
	GAME *pGame,
	LEVEL *pLevel,
	ROOM *pRoom);

ROOM* LevelGetStartPosition( 
	GAME* game, 
	UNIT *pUnit,
	LEVELID idLevel,
	VECTOR& vPosition, 
	VECTOR& vFaceDirection, 
	DWORD dwWarpFlags);			// see WARP_FLAGS


//returns the warp unit for a player to connect to
UNIT * LevelGetWarpConnectionUnit(
	GAME *pGame,
	LEVEL *pLevelGoingTo,
	int nLevelAreaLeaving,
	int nLevelAreaLeavingDepth = 0);

//----------------------------------------------------------------------------
enum WARP_TO_TYPE
{
	WARP_TO_TYPE_PREV,
	WARP_TO_TYPE_NEXT,
	WARP_TO_TYPE_TRIGGER,
};
	
ROOM * LevelGetWarpToPosition( 
	GAME * game,
	LEVELID idLevel,
	WARP_TO_TYPE eWarpToType,
	char * szTriggerString,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection,
	int * pnRotation = NULL);

LEVEL* LevelGetByID(
	GAME* game,
	LEVELID idLevel);

SPAWN_CLASS_MEMORY *LevelGetSpawnMemory(
	GAME *pGame,
	LEVELID idLevel,
	int nSpawnClass);

const char *LevelGetDevName(
	const LEVEL *pLevel);

const WCHAR *LevelGetDisplayName(
	const LEVEL *pLevel);

const WCHAR *LevelDefinitionGetDisplayName(
	const LEVEL_DEFINITION *pLevelDefinition);

const WCHAR *LevelDefinitionGetDisplayName(
	int nLevelDef);
		
void LevelResetSpawnMemory(
	GAME *game, 
	LEVEL* level);

int LevelRuleGetLine( 
	char * pszLabel);

BOOL ExcelLevelRulesPostProcess(
	struct EXCEL_TABLE * table);

BOOL ExcelLevelRulesPostLoadAll(
	struct EXCEL_TABLE * table);

void ExcelLevelRulesFreeRow( 
	struct EXCEL_TABLE * table,
	BYTE * rowdata);

BOOL ExcelLevelFilePathPostProcessRow(
	struct EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata);

BOOL ExcelRoomIndexPostProcess(
	struct EXCEL_TABLE * table);

void ExcelRoomIndexFreeRow( 
	struct EXCEL_TABLE * table,
	BYTE * rowdata);

DRLG_LEVEL_DEFINITION* DrlgGetLevelRuleDefinition(
	DRLG_PASS* pPass );

//----------------------------------------------------------------------------
enum LEVEL_CONNECTION_DIRECTION
{
	LCD_NEXT,
	LCD_PREV
};

int LevelFindConnectedRootLevelDefinition( 
	int nLevelDef,
	LEVEL_CONNECTION_DIRECTION eLevelConnection);

void s_LevelCreateRoot(
	GAME *pGame,
	int nLevelDef,
	UNITID idActivator);
	
BOOL s_LevelWarpIntoGame(
	UNIT *pPlayer,
	struct WARP_SPEC &tWarpSpec,
	int nLevelDefLeaving);

#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_LevelWarp(
	UNIT *pPlayer,
	struct WARP_SPEC &tSpec);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
enum LEVEL_RECALL_FLAGS
{
	LRF_FORCE,
	LRF_PRIMARY_ONLY,
	LRF_BEING_BOOTED,
};

BOOL s_LevelRecall(
	GAME * game,
	UNIT * unit,
	DWORD dwLevelRecallFlags = 0);  // see LEVEL_RECALL_FLAGS

BOOL s_LevelRecallEvent(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA &tData);
	
void LevelSetFlag(
	LEVEL* pLevel,
	LEVEL_FLAG eFlag);

int LevelGetFirstPortalAndRecallLocation(
	GAME *pGame);

void LevelClearFlag(
	LEVEL* pLevel,
	LEVEL_FLAG eFlag);
		
BOOL LevelTestFlag(
	const LEVEL* pLevel,
	LEVEL_FLAG eFlag);

ROOM* LevelGetFirstRoom(
	const LEVEL* level);

ROOM* LevelGetNextRoom(
	ROOM* room);

int LevelGetMonsterExperienceLevel(
	const LEVEL* pLevel,
	int nDifficultyOverride = DIFFICULTY_INVALID);

GAME *LevelGetGame(
	const LEVEL* pLevel);

LEVELID LevelGetID(
	const LEVEL* pLevel);
	
LEVEL *LevelFindFirstByType(
	GAME* pGame,
	const LEVEL_TYPE &tLevelType);

BOOL LevelTypesAreEqual(
	const LEVEL_TYPE &tLevelType1,
	const LEVEL_TYPE &tLevelType2);

BOOL LevelGetLevelType(
	const LEVEL *pLevel,
	LEVEL_TYPE &tLevelType);
	
unsigned int LevelGetDefinitionCount(
	void);
	
SRVLEVELTYPE LevelGetSrvLevelType(
	const LEVEL *pLevel);

SRVLEVELTYPE LevelDefinitionGetSrvLevelType(
	int nLevelDef);
	
/*
int LevelDefinitionGetRandom(
	GAME *pGame,
	int nArea,
	int nDepth);

int LevelDefinitionGetRandom(
	DWORD dwSeed,
	int nArea,
	int nDepth);
*/
LEVEL_TYPE LevelGetType(
	const LEVEL *pLevel);

BOOL LevelTypeIsValid(
	const LEVEL_TYPE &tLevelType);
	
int LevelGetDefinitionIndex(
	const LEVEL * level);

int LevelGetDefinitionIndex(
	const char * level_name);

void s_LevelStartEvents( 
	GAME * pGame,
	LEVEL * pLevel );

const LEVEL_DEFINITION * LevelDefinitionGet(
	int level_definition);

const LEVEL_DEFINITION * LevelDefinitionGet(
	const LEVEL * level);

const LEVEL_DEFINITION * LevelDefinitionGet(
	const char * level_name);

const DRLG_DEFINITION * LevelGetDRLGDefinition(
	const LEVEL *pLevel);

const DRLG_DEFINITION * DRLGDefinitionGet(
	int nDRLGDefinition);




DRLG_LEVEL_DEFINITION* DRLGLevelDefinitionGet(
	const char* pszFolder,
	const char* pszFileName);

int DRLGGetDefinitionIndexByName(
	const char *pszName);
	
int LevelDefinitionGetRandom( LEVEL *pLevel,
							  int nLevelAreaID,
						      int nDepth );
/*
int LevelDefinitionGetRandom(
	DWORD dwSeed,
	int nArea,
	int nDepth);
*/
int LevelGetDepth(
	const LEVEL* level);

BOOL LevelGetPVPWorld(
	const LEVEL* level);

int LevelGetDifficultyOffset(
	const LEVEL* level);

int LevelGetDifficulty( 
	const LEVEL* level );


inline int LevelGetLevelAreaID( const LEVEL *level )
{
	ASSERT_RETINVALID(level);
	return level->m_nLevelAreaId;

}

inline MYTHOS_ANCHORMARKERS::cAnchorMarkers * LevelGetAnchorMarkers( LEVEL * pLevel )
{
	ASSERT_RETNULL(pLevel);
	return &pLevel->m_AnchorMakers;
}

inline const MYTHOS_LEVELAREAS::LEVEL_AREA_OVERRIDES * LevelGetLevelAreaOverrides( const LEVEL* level )
{
	ASSERT_RETNULL(level);
	return &level->m_LevelAreaOverrides;
}

float LevelGetArea( 
	LEVEL* pLevel);

int LevelGetAreaLevel( 
	const LEVEL* pLevel);


BOOL LevelDepthGetDisplayName( 
	GAME* pGame,
	int nLevelArea,
	int nLevelDefDest,
	int nLevelDepth,
	WCHAR *puszBuffer,
	int nBufferSize,
	BOOL bIncludeQuests = false );

BOOL LevelDefinitionIsSafe(
	const LEVEL_DEFINITION* pLevelDefinition);

BOOL LevelIsSafe(
	const LEVEL* pLevel);

BOOL LevelIsTown(
	const LEVEL* pLevel);

BOOL LevelIsRTS(
	const LEVEL* pLevel);

BOOL LevelPlayerOwnedCannotDie(
	const LEVEL *pLevel);
		
BOOL LevelHasHellrift(	
	LEVEL *pLevel);

BOOL LevelCanHaveHellrift(
	LEVEL * pLevel);

void LevelDirtyVisibility( LEVEL* pLevel );

void LevelUpdateVisibility( 
	GAME *pGame,
	LEVEL* pLevel,
	const VECTOR& Position );	// position of light source

BOOL LevelIsVisible( 
	LEVEL* pLevel,
	const VECTOR& Position );	// position to check

void LevelUpdateVisibility(
	GAME *pGame,
	LEVEL* pLevel,
	const VECTOR& Position );	// position of light source

BOOL LevelIsVisible(
	LEVEL* pLevel,
	const VECTOR& Position );	// position to check

void s_LevelSendToClient( 
	GAME *pGame, 
	UNIT *pPlayer,
	GAMECLIENTID idClient,
	LEVELID idLevel);

void s_LevelsUpdate(
	GAME *pGame);

int s_LevelGetPlayerCount(
	LEVEL *pLevel);

void s_LevelResendToClient( 
	GAME * pGame, 
	LEVEL *pLevel,
	GAMECLIENT *pClient,
	UNIT *pUnit);

int s_LevelGetConnectedChain( 
	LEVEL *pLevel, 
	int nCurrentCount,
	LEVEL **pAvailableLevels, 
	int nMaxLevels);

enum { MAX_CONNECTED_LEVELS = 256 };

int s_LevelGetConnectedLevels( 
	LEVEL* pLevel, 
	UNITID idWarpToLevel,
	LEVEL_CONNECTION* pConnectedLevels, 
	int nMaxConnectedLevels,
	BOOL bFollowEntireChain);
	
DISTRICT* s_LevelGetDistrict(
	LEVEL* pLevel,
	int nDistrict);
	
void s_LevelOrganizeDistricts(
	GAME* pGame,
	LEVEL* pLevel);

int s_LevelGetNumDistricts( 
	const LEVEL* pLevel);

DISTRICT* s_LevelGetRandomDistrict( 
	GAME* pGame,
	const LEVEL* pLevel);

int s_LevelSelectRandomSpawnableMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter);

int s_LevelSelectRandomSpawnableQuestMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	int nMonsterExperienceLevel,
	int nLevelDepth,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter);

int s_LevelSelectRandomSpawnableInteractableMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter);

int s_LevelGetPossibleRandomMonsters( 
	LEVEL* pLevel, 
	int *pnMonsterClassBuffer,
	int nBufferSize,
	int nCurrentBufferCount,
	BOOL bUseLevelSpawnClassMemory);

float s_LevelGetChampionSpawnRate(
	GAME *pGame,
	LEVEL *pLevel);

void s_LevelSetChampionSpawnRateOverride(
	float flRate);

int s_LevelSpawnMonsters(
	GAME* pGame,
	LEVEL* pLevel,
	int nMonsterClass,
	int nMonsterCount,
	int nMonsterQuality,
	ROOM *pRoom = NULL,
	PFN_MONSTER_SPAWNED_CALLBACK pfnSpawnedCallback = NULL,
	void *pCallbackData = NULL);

void s_LevelClearCommunicationChannel(
	GAME * pGame,
	LEVEL * pLevel );

BOOL s_LevelSetCommunicationChannel(
	GAME * pGame,
	LEVEL * pLevel,
	CHANNELID idChannel );

void s_LevelAddPlayer(
	LEVEL * pLevel,
	UNIT * pUnit );

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelRemovePlayer(
	LEVEL * pLevel,
	UNIT * pUnit );
#endif

void c_LevelSetAndLoad( 
	GAME *pGame, 
	MSG_SCMD_SET_LEVEL* msg );

void c_LevelInit( 
	GAME *pGame, 
	int nLevelDefinition,
	int nEnvDefinition,
	int nRandomTheme);


BOOL c_LevelEntranceVisible( UNIT *pWarp );
void c_LevelResetDungeonEntranceVisibility( LEVEL *pLevel,
										    UNIT *pPlayer );

void c_LevelFilloutLevelWarps( LEVEL *pLevel );

int s_LevelGetEnvironmentIndex(
	GAME * pGame,
	LEVEL * pLevel );

void c_SetEnvironmentDef(
	GAME * pGame,
	int nRegion,
	const LEVEL_ENVIRONMENT_DEFINITION * pEnvDefintion,
	const char * pszEnvSuffix = "",
	BOOL bForceSyncLoad = FALSE,
	BOOL bTransition = FALSE );

void c_SetEnvironmentDef(
	GAME * pGame,
	int nRegion,
	int nEnvDefinition,
	const char * pszEnvSuffix = "",
	BOOL bForceSyncLoad = FALSE,
	BOOL bTransition = FALSE );

void c_SetEnvironmentDef(
	int nRegion,
	const char * pszFilePath,
	const char * pszFilePathBackup = "",
	BOOL bTransition = FALSE,
	BOOL bForceSyncLoad = FALSE );

int c_LevelCreatePaperdollLevel( 
	int nPlayerClass );

void c_LevelLoadComplete(
	int nLevelDefinition, 
	int nDRLGDefinition,
	int nLevelId, 
	int nLevelDepth );

void c_LevelWaitForLoaded(
	GAME *pGame,
	int nLevelDefinition,
	int nDRLGDefinition,
	ASYNC_FILE_CALLBACK pOverrideCallback = NULL );

void LevelProjectToFloor(
	LEVEL *pLevel,
	const VECTOR &vPos,
	VECTOR &vPosOnFloor);

LEVELID s_LevelFindLevelOfTypeLinkingTo(
	GAME *pGame,
	const LEVEL_TYPE &tLevelTypeSource,
	LEVELID idLevelDest);
	
BOOL s_LevelGetArrivalPortalLocation( 
	GAME *pGame,
	LEVELID idLevelDest,
	LEVELID idLevelLeaving,
	int nPrevAreaID,
	ROOM **pRoom,
	VECTOR *pvPosition,
	VECTOR *pvDirection);

int LevelGetNumSubLevels(
	const LEVEL *pLevel);

SUBLEVEL *LevelGetSubLevelByIndex(
	LEVEL *pLevel,
	int nIndex);
	
int LevelGetDefaultSubLevelDefinition( 
	const LEVEL *pLevel);

SUBLEVEL *LevelGetPrimarySubLevel(
	LEVEL *pLevel);

void s_LevelCreateSubLevelEntrance(
	LEVEL *pLevel, 
	SUBLEVEL *pSubLevel);
		
void LevelIterateUnits( 
	LEVEL *pLevel, 
	TARGET_TYPE *peTargetTypes, 
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData);

UNIT *LevelFindFirstUnitOf(
	LEVEL *pLevel,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass);

UNIT *LevelFindClosestUnitOfType(
	LEVEL *pLevel,
	const VECTOR &vPosition,
	TARGET_TYPE *peTargetTypes,
	int nUnitType);

UNIT *LevelFindFarthestUnitOf(
	LEVEL *pLevel,
	const VECTOR &vPosition,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass);

UNIT *LevelFindRandomUnitOf(
	LEVEL *pLevel,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass);

int LevelCountMonsters(
	LEVEL *pLevel,
	int nMonsterClassHierarchy=INVALID_ID,
	int nUnitType=INVALID_ID);

int s_LevelCountMonstersUseEstimate(
	LEVEL *pLevel);

typedef void (* PFN_ITERATE_PLAYERS)( UNIT *pUnit, void *pCallbackData );

void LevelIteratePlayers( 
	LEVEL *pLevel, 
	PFN_ITERATE_PLAYERS pfnCallback,
	void *pCallbackData);
	
BOOL LevelAddWarp(
	UNIT *pWarp,
	UNITID idPlayer,
	enum WARP_RESOLVE_PASS eWarpResolvePass);

BOOL ThemeIsA(
	const int nLevelTheme,
	const int nTestTheme);

void LevelThemeForce(
	LEVEL * pLevel,
	int nTheme,
	BOOL bForceOn );

BOOL DRLGThemeIsA(
	const DRLG_DEFINITION * pDRLGDef,
	const int nTestTheme);

BOOL LevelThemeIsA(
	const LEVEL * pLevel,
	const int nTestTheme);

BOOL LevelHasThemeEnabled(
	LEVEL * pLevel,
	int nThemeToTest );

void cLevelUpdateUnitsVisByThemes( LEVEL *pLevel );

int LevelGetSelectedThemes(
	LEVEL * pLevel,
	int ** pnThemes);

typedef void (* PFN_LEVEL_ITERATE_ROOMS)( ROOM *pRoom, void *pCallbackData );

void LevelIterateRooms(
	LEVEL *pLevel,
	PFN_LEVEL_ITERATE_ROOMS pfnCallback,
	void *pUserData);

int s_LevelGetRandomSpawnRooms(
	GAME* pGame,
	LEVEL* pLevel,
	ROOM** pRooms,
	int nMaxRooms);

void LevelNeedRoomUpdate(
	LEVEL *pLevel,
	BOOL bNeedUpdate);

float LevelGetMaxTreasureDropRadius(
	LEVEL *pLevel);

int LevelGetMonsterCount(
	LEVEL *pLevel,
	ROOM_MONSTER_COUNT_TYPE eCountType = RMCT_ABSOLUTE,
	const TARGET_TYPE * peTargetTypes = NULL);
	
int LevelGetMonsterCountLivingAndBad(
	LEVEL *pLevel,
	ROOM_MONSTER_COUNT_TYPE eCountType = RMCT_ABSOLUTE);

BOOL c_LevelLoadLocalizedTextures(
	GAME *pGame,
	int nLevelFilePath,
	int nSKU);


void s_LevelSendStartLoad( 
	GAME *pGame,
	GAMECLIENTID idClient,
	int nLevelDef, 
	int nLevelDepth, 
	int nLevelArea,
	int nDRLGDef);

void LevelDRLGChoiceInit(
	LEVEL_DRLG_CHOICE &tDRLGChoice);

enum LEVEL_POSSIBLE_FLAGS
{
	PF_DRLG_ALL_BIT,				// look at *all* of the possible DRLGs in the level definition
};

void LevelDefGetPossibleMonsters(
	GAME *pGame,
	int nLevelDef,
	DWORD dwPossibleFlags,			// see LEVEL_POSSIBLE_FLAGS
	int *pnMonsterClassBuffer,
	int *pnCurrentBufferCount,
	int nMaxBufferSize);
	
void LevelDRLGChoiceGetPossilbeMonsters(
	GAME *pGame,
	const LEVEL_DRLG_CHOICE *pDRLGChoice,
	int *pnMonsterClassBuffer,
	int nMaxBufferSize,
	int *pnCurrentBufferCount,
	BOOL bUseSpawnClassMemory,
	LEVEL *pLevel);

void LevelGetLayoutMonsters( 
	LEVEL *pLevel,
	BOOL bUseSpawnClassMemory, 
	int *pnMonsterClassBuffer, 
	int nMaxBufferSize, 
	int *pnCurrentBufferCount);

void LevelExecutePlayerEnterLvlScript(
    LEVEL *pLevel,
	UNIT *pUnit );

int LevelDefinitionGetAct(
	int nLevelDef);

int LevelGetAct(
	const LEVEL *pLevel);

BOOL LevelIsAvailable(
	UNIT *pUnit,
	LEVEL *pLevel);

int LevelDestinationValidate(
	UNIT *pPlayer,
	int nLevelDef);

enum WARP_RESTRICTED_REASON LevelDefIsAvailableToUnit(
	UNIT *pUnit,
	int nLevelDef);

WARP_RESTRICTED_REASON LevelIsAvailableToUnit(
	UNIT *pUnit,
	LEVEL *pLevel);
	
void s_LevelSendAllSubLevelStatus( 
	UNIT *pUnit);
	
void s_LevelRegisterUnitToDropOnDie( GAME *pGame,
									 UNIT *pUnit, 
									 int nTreasureClass );

BOOL s_LevelRegisterTreasureDropRandom( GAME *pGame,
									    LEVEL *pLevel,
										int nTreasureClass,
										int nObjectClassFallbackSpawner,
										ROOM *pBeforeRoom = NULL );
									 
BOOL LevelIsOnWorldMap(
	int nLevelDef);

void LevelMarkVisited(
	UNIT *pPlayer,	
	int nLevelDef,
	WORLD_MAP_STATE eState,
	int nDifficultyOverride = -1);

void s_LevelQueuePlayerRoomUpdate(
	LEVEL *pLevel,
	UNIT *pPlayerIgnore = NULL);

BOOL LevelCanFormAutoParties(
	LEVEL *pLevel);

BOOL LevelIsOutdoors(
	LEVEL * pLevel);

BOOL PlayerFixNontownWaypoints(
   UNIT* pPlayer );

//level of interest stuff
void s_LevelAddUnitOfInterest( LEVEL *pLevel, UNIT *pUnit );
UNIT * s_LevelGetUnitOfInterestByIndex( LEVEL *pLevel, int nIndex );
int s_LevelGetUnitOfInterestCount( LEVEL *pLevel );
void s_LevelSendPointsOfInterest( LEVEL *pLevel, UNIT *pPlayer );


//----------------------------------------------------------------------------
enum LEVEL_MAP_EXPAND
{
	LME_EXPAND,
	LME_COMPRESS,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelCompressOrExpandMapStats(
	UNIT *pPlayer,
	LEVEL_MAP_EXPAND eExpand);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL LevelIsActive(
	LEVEL * level)
{
	ASSERT_RETFALSE(level);
	return (LevelTestFlag(level, LEVEL_ACTIVE_BIT));
}


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#if ISVERSION(CHEATS_ENABLED)
extern int gnMonsterLevelOverride;
extern int gnItemLevelOverride;
#endif
extern BOOL gbClientsHaveVisualRoomsForEntireLevel;
extern LEVEL_TYPE gtLevelTypeAny;

#endif // _LEVEL_H_
