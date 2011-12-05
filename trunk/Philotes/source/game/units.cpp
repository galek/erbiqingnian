//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "act.h"
#include "affix.h"
#include "c_ui.h"
#include "uix_quests.h"
#include "c_townportal.h"
#include "prime.h"
#include "game.h"
#include "gameconfig.h"
#include "gamelist.h"
#include "globalindex.h"
#include "units.h"
#include "clients.h"
#include "boundingbox.h"
#include "level.h"
#include "globalIndex.h"
#include "gameunits.h"
#include "pets.h"
#include "unit_priv.h"
#include "c_camera.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_backgroundsounds.h"
#include "c_footsteps.h"
#include "c_font.h"
#include "player.h"
#include "characterclass.h"
#include "uix_money.h"
#include "fontcolor.h"
#include "s_message.h"
#include "inventory.h"
#include "monsters.h"
#include "objects.h"
#include "items.h"
#include "missiles.h"
#include "proc.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "movement.h"
#include "c_appearance_priv.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "skills.h"
#include "ai.h"
#include "combat.h"
#include "unitmodes.h"
#include "teams.h"
#include "c_particles.h"
#include "unittag.h"
#include "console.h"
#include "stringtable.h"
#include "excel_private.h"
#include "states.h"
#include "uix.h"
#include "uix_graphic.h"			// for some text color-coding
#include "script.h"
#include "warp.h"
#include "filepaths.h"
#include "colors.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "npc.h"
#include "quests.h"
#include "config.h"
#include "e_model.h"
#include "e_texture.h"
#include "unitdebug.h"
#include "colorset.h"
#include "dbhellgate.h"
#include "s_trade.h"
#include "room_path.h"
#include "unit_metrics.h"
#include "tasks.h"
#include "e_frustum.h"
#include "s_quests.h"
#include "path.h"
#include "s_partyCache.h"
#include "svrstd.h"
#include "e_visibilitymesh.h"
#include "weaponconfig.h"
#include "weapons_priv.h"
#include "c_tasks.h"
#include "Quest.h"
#include "c_QuestClient.h"
#include "weather.h"
#include "imports.h"	// for VMDamageDone
#include "GameMaps.h"
#include "c_GameMapsClient.h"
#include "s_GameMapsServer.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "s_LevelAreasServer.h"
#include "c_recipe.h"
#include "s_townportal.h"
#include "dictionary.h"
#include "windowsmessages.h"
#include "c_xfirestats.h"
#include "weapons.h"
#include "unitfileversion.h"
#include "dbunit.h"
#include "guidcounters.h"
#include "s_QuestServer.h"
#include "e_environment.h"
#include "stringreplacement.h"
#include "e_budget.h"
#include "s_chatNetwork.h"
#include "demolevel.h"
#include "gameserver.h"
#include "global_themes.h"
#include "openlevel.h"
#include "e_optionstate.h"
#include "waypoint.h"
#include "party.h"
#include "partymatchmaker.h"
#include "achievements.h"
#include "s_playerEmail.h"
#include "versioning.h"

#if ! ISVERSION(SERVER_VERSION)
#include "automap.h"
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
#include "s_crafting.h"
#endif

#if ISVERSION(SERVER_VERSION)
#include "units.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// DEBUG DEFINES
//----------------------------------------------------------------------------
#define	TRACE_UNIT_FREE(fmt, ...)		// trace(fmt, __VA_ARGS__)
//#define LOST_CLIENT_CONTROL_UNIT_DEBUG

#if ISVERSION(DEVELOPMENT)
BOOL gbLostItemDebug =					FALSE;
BOOL gbHeadstoneDebug =					FALSE;
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define UNIT_HASH_SIZE					1024
#define CLIENT_UNIT_MASK				0x80000000
#define MAKE_UNIT_HASH(id)				((id) % UNIT_HASH_SIZE)

#if ISVERSION(DEVELOPMENT)
#define GUID_HASH_SIZE					((QWORD)8)
#else
#define GUID_HASH_SIZE					((QWORD)1024)
#endif
#define MAKE_GUID_HASH(id)				(unsigned int)((id) % GUID_HASH_SIZE)

#define DISABLE_BREATH


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const VECTOR INVALID_POSITION(0.0f, 0.0f, 0.0f);
const VECTOR INVALID_DIRECTION(0.0f, 0.0f, 0.0f);

// textures
static const TEXTURE_TYPE gpeUnitOverrideToTextureType[UNIT_TEXTURE_OVERRIDE_COUNT] = 
{
	TEXTURE_DIFFUSE,	//	UNIT_TEXTURE_OVERRIDE_DIFFUSE = 0,
	TEXTURE_NORMAL,		//	UNIT_TEXTURE_OVERRIDE_NORMAL,
	TEXTURE_SPECULAR,	//	UNIT_TEXTURE_OVERRIDE_SPECULAR,
	TEXTURE_SELFILLUM,	//	UNIT_TEXTURE_OVERRIDE_SELFILLUM,
};

//----------------------------------------------------------------------------
static const int GHOST_CHECK_INTERVAL_IN_TICKS = GAME_TICKS_PER_SECOND * 1;

static const float GHOST_RESURRECT_AT_HEADSTONE_RADIUS = 15.0f;
static const float GHOST_RESURRECT_AT_HEADSTONE_RADIUS_SQ = GHOST_RESURRECT_AT_HEADSTONE_RADIUS * GHOST_RESURRECT_AT_HEADSTONE_RADIUS;

static const float GHOST_SLOPPY_RESURRECT_AT_HEADSTONE_READIUS = GHOST_RESURRECT_AT_HEADSTONE_RADIUS / 2.0f;
static const float GHOST_SLOPPY_RESURRECT_AT_HEADSTONE_RADIUS_SQ = GHOST_SLOPPY_RESURRECT_AT_HEADSTONE_READIUS * GHOST_SLOPPY_RESURRECT_AT_HEADSTONE_READIUS;


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct UNIT_HASH
{
	UNIT *							pHash[UNIT_HASH_SIZE];
	UNITID							uidCur;
	UNITID							uidCurClient;
	int								nCount;

	UNIT *							pGuidHash[GUID_HASH_SIZE];
};

//----------------------------------------------------------------------------
struct GENUS_LOOKUP
{
	GENUS eGenus;
	const char *pszName;
};
static const GENUS_LOOKUP sgtGenusTable[] = 
{
	{ GENUS_NONE,		"None" },
	{ GENUS_PLAYER,		"Player" },
	{ GENUS_MONSTER,	"Monster" },
	{ GENUS_MISSILE,	"Missile" },
	{ GENUS_ITEM,		"Item" },
	{ GENUS_OBJECT,		"Object" },
};
static const int sgnNumGenusLookup = arrsize( sgtGenusTable );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const GENUS_LOOKUP *sGetGenusLookup(
	const GENUS eGenus)
{
	for (int i = 0; i < sgnNumGenusLookup; ++i)
	{
		const GENUS_LOOKUP *pLookup = &sgtGenusTable[ i ];
		if (pLookup->eGenus == eGenus)
		{
			return pLookup;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *GenusGetString(
	GENUS eGenus)
{
	static const char * sszUnknown = "Unknown";
	const GENUS_LOOKUP *pLookup = sGetGenusLookup( eGenus );
	if (pLookup)
	{
		return pLookup->pszName;
	}
	return sszUnknown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_UNIT_TRACKING

#define GLOBAL_UNIT_TRACKER_MAX_CLASS	2500

struct GLOBAL_UNIT_TRACKER
{
	const char *					m_szName[NUM_GENUS - 1][GLOBAL_UNIT_TRACKER_MAX_CLASS];
	volatile long					m_Active[NUM_GENUS - 1][GLOBAL_UNIT_TRACKER_MAX_CLASS];
	volatile long					m_Count[NUM_GENUS - 1][GLOBAL_UNIT_TRACKER_MAX_CLASS];
	volatile long					m_Lifetime[NUM_GENUS - 1][GLOBAL_UNIT_TRACKER_MAX_CLASS];
	CCritSectLite					m_CS;
	unsigned int					m_GenusCount[NUM_GENUS - 1];
	BOOL							m_bInitialized;

	//------------------------------------------------------------------------
	GLOBAL_UNIT_TRACKER(
		void) : m_bInitialized(FALSE)
	{
		static const char * szUnknown = "unknown";

		m_CS.Init();

		for (unsigned int ii = 0; ii < NUM_GENUS - 1; ++ii)
		{
			m_GenusCount[ii] = 0;
			for (unsigned int jj = 0; jj < GLOBAL_UNIT_TRACKER_MAX_CLASS; ++jj)
			{
				m_Active[ii][jj] = 0;
				m_Count[ii][jj] = 0;
				m_Lifetime[ii][jj] = 0;
				m_szName[ii][jj] = szUnknown;
			}
		}
	}

	//------------------------------------------------------------------------
	void Initialize(
		void)
	{
		CSLAutoLock autolock(&m_CS);

		if (m_bInitialized)
		{
			return;
		}

		for (unsigned int genus = 1; genus < NUM_GENUS; ++genus)
		{
			EXCELTABLE eTable = UnitGenusGetDatatableEnum((GENUS)genus);
			ASSERT_CONTINUE(eTable != DATATABLE_NONE);
			m_GenusCount[genus-1] = MIN((unsigned int)ExcelGetCount(NULL, eTable), (unsigned int)GLOBAL_UNIT_TRACKER_MAX_CLASS);

			for (unsigned int unitclass = 0; unitclass < m_GenusCount[genus-1]; ++unitclass)
			{
				const UNIT_DATA * unit_data = (const UNIT_DATA *)ExcelGetData(NULL, eTable, unitclass);
				if (!unit_data)
				{
					continue;
				}
				m_szName[genus-1][unitclass] = unit_data->szName;
			}
		}
		m_bInitialized = TRUE;
	}

	//------------------------------------------------------------------------
	void Add(
		SPECIES species)
	{
		unsigned int genus = GET_SPECIES_GENUS(species) - 1;
		ASSERT_RETURN(genus < NUM_GENUS - 1);
		unsigned int unitclass = GET_SPECIES_CLASS(species);	
		ASSERT_RETURN(unitclass < GLOBAL_UNIT_TRACKER_MAX_CLASS);
		unsigned long count = InterlockedIncrement(&(m_Count[genus][unitclass]));
		ASSERT(count > 0);
		// also activate it
		count = InterlockedIncrement(&(m_Active[genus][unitclass]));
		ASSERT(count > 0);
		// increment lifetime
		InterlockedIncrement(&(m_Lifetime[genus][unitclass]));
	}

	//------------------------------------------------------------------------
	void Remove(
		SPECIES species)
	{
		unsigned int genus = GET_SPECIES_GENUS(species) - 1;
		ASSERT_RETURN(genus < NUM_GENUS - 1);
		unsigned int unitclass = GET_SPECIES_CLASS(species);	
		ASSERT_RETURN(unitclass < GLOBAL_UNIT_TRACKER_MAX_CLASS);
		unsigned long count = InterlockedDecrement(&(m_Count[genus][unitclass]));
		ASSERT(count != INVALID_ID);
	}

	//------------------------------------------------------------------------
	void Activate(
		SPECIES species)
	{
		unsigned int genus = GET_SPECIES_GENUS(species) - 1;
		ASSERT_RETURN(genus < NUM_GENUS - 1);
		unsigned int unitclass = GET_SPECIES_CLASS(species);	
		ASSERT_RETURN(unitclass < GLOBAL_UNIT_TRACKER_MAX_CLASS);
		unsigned long count = InterlockedIncrement(&(m_Active[genus][unitclass]));
		ASSERT(count > 0);
	}

	//------------------------------------------------------------------------
	void Deactivate(
		SPECIES species)
	{
		unsigned int genus = GET_SPECIES_GENUS(species) - 1;
		ASSERT_RETURN(genus < NUM_GENUS - 1);
		unsigned int unitclass = GET_SPECIES_CLASS(species);	
		ASSERT_RETURN(unitclass < GLOBAL_UNIT_TRACKER_MAX_CLASS);
		unsigned long count = InterlockedDecrement(&(m_Active[genus][unitclass]));
		ASSERT(count != INVALID_ID);
	}

	//------------------------------------------------------------------------
	void TraceAll(
		void)
	{
		Initialize();
		TraceGameInfo("--= GLOBAL UNIT TRACKER =--");
		TraceGameInfo("%40s  %20s  %20s  %20s", "UNIT", "ACTIVE", "TOTAL", "LIFETIME");
		for (unsigned int genus = 0; genus < NUM_GENUS - 1; ++genus)
		{
			TraceGameInfo("%s:", GenusGetString((GENUS)(genus + 1)));
			for (unsigned int species = 0; species < m_GenusCount[genus]; ++species)
			{
				unsigned int lifetime = (unsigned int)m_Lifetime[genus][species];
				unsigned int count = (unsigned int)m_Count[genus][species];
				unsigned int active = (unsigned int)m_Active[genus][species];
				if (lifetime > 0)
				{
					const char * name = m_szName[genus][species];
					TraceGameInfo("%40s  %20d  %20d  %20d", name, active, count, lifetime);
				}
			}
		}
	}
};

GLOBAL_UNIT_TRACKER g_UnitTracker;

#define GlobalUnitTrackerAdd(s)			g_UnitTracker.Add(s);
#define GlobalUnitTrackerRemove(s)		g_UnitTracker.Remove(s);

void GlobalUnitTrackerActivate(
	SPECIES species)
{
	g_UnitTracker.Activate(species);
}

void GlobalUnitTrackerDeactivate(
	SPECIES species)
{
	g_UnitTracker.Deactivate(species);
}

void GlobalUnitTrackerTrace(
	void)
{
	g_UnitTracker.TraceAll();
}

#else

#define GlobalUnitTrackerAdd(s)			
#define GlobalUnitTrackerRemove(s)		

#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
BOOL UnitGetSpeedsByMode(
	UNIT * pUnit,
	UNITMODE eMode,
	float & fSpeedBase,
	float & fSpeedMin,
	float & fSpeedMax);


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelUnitPostProcessCommon( 
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	// do individual unit post process functions
	switch (table->m_Index)
	{
		case DATATABLE_ITEMS:		ExcelItemsPostProcess(table);		break;
		case DATATABLE_MISSILES:	ExcelMissilesPostProcess(table);	break;
		case DATATABLE_PLAYERS:		ExcelPlayersPostProcess(table);		break;
		case DATATABLE_MONSTERS:	ExcelMonstersPostProcess(table);	break;
		case DATATABLE_OBJECTS:		ExcelObjectsPostProcess(table);		break;
		default:					return TRUE;
	}

	// go through all rows
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		UNIT_DATA * unit_data = (UNIT_DATA *)ExcelGetDataPrivate(table, ii);
		if ( ! unit_data )
			continue;

		// ContentVersionMagic this should get removed when Peter does some excel magic to set the entire row to default values
		if (unit_data->nVersion > 1)
		{
			CLEARBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_SPAWN);
			SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_IGNORE_IN_DAT);
		}
	}

	return TRUE;			
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitHashInit(
	GAME* game)
{
	if (game->pUnitHash)
	{
		UnitHashFree(game);
	}
	game->pUnitHash = (UNIT_HASH*)GMALLOCZ(game, sizeof(UNIT_HASH));
	ASSERT_RETURN(game->pUnitHash);
	game->pUnitHash->uidCur = INVALID_ID;
	game->pUnitHash->uidCurClient = INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitHashFree(
	GAME* game)
{
	if (!game->pUnitHash)
	{
		return;
	}

	trace( "%s: UnitHashFree()\n", IS_SERVER( game ) ? "SERVER" : "CLIENT" );
		
	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT * pNext = NULL;
		for (UNIT * pUnit = game->pUnitHash->pHash[ii]; pUnit; pUnit = pNext)
		{		
			// get next unit
			ASSERT_BREAK(UnitGetGameId(pUnit) == GameGetId(game));
			pNext = pUnit->hashnext;
			
			// free only units that are not contained by other units because UnitFree()
			// will automatically free anything it contains itself
			if (UnitGetContainer( pUnit ) == NULL)
			{
				UnitFree( pUnit, UFF_HASH_BEING_FREED );
			}
		}
	}

	// all units should be gone
#ifdef _DEBUG
	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		ASSERTX_CONTINUE( game->pUnitHash->pHash[ii] == NULL, "Unit hash bucket should be empty" );
	}
#endif
	
	UnitFreeListFree(game);

	GFREE(game, game->pUnitHash);
	game->pUnitHash = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void UnitRoomListDebug(
	UNIT* unit,
	ROOM* room)
{
#ifdef _DEBUG
	if (!room)
	{
		return;
	}

	// walk the room list
	for (int ii = 0; ii < NUM_TARGET_TYPES; ii++)
	{
		UNIT* test = room->tUnitList.ppFirst[ii];
		if (test)
		{
			ASSERT(test->tRoomNode.pPrev == NULL);
			UNIT* prev = test;
			test = test->tRoomNode.pNext;
			while (test)
			{
				ASSERT(test->tRoomNode.pPrev == prev);
				ASSERT(test->pRoom == room);

				prev = test;
				test = test->tRoomNode.pNext;
			}
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitRoomListAdd(
	UNIT* unit,
	ROOM* room,
	TARGET_TYPE eTargetType,
	bool bAddToLevelProxMap )
{
	ASSERT_RETURN(unit);
	UnitRoomListDebug(unit, room);
	UnitRoomListDebug(unit, unit->pRoom);

	ASSERT(unit->tRoomNode.pPrev == NULL && unit->tRoomNode.pNext == NULL);
	ASSERT(room->tUnitList.ppFirst[unit->eTargetType] != unit);

#if ISVERSION(SERVER_VERSION)
	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	if( UnitIsA( unit, UNITTYPE_PLAYER ) )
	{
		if (bAddToLevelProxMap && room->pLevel && room->pLevel->m_pUnitPosProxMap) {
			ASSERT(unit->LevelProxMapNode == INVALID_ID);
			unit->LevelProxMapNode = room->pLevel->m_pUnitPosProxMap->Insert
				(unit->unitid, 0, unit->vPosition.x, unit->vPosition.y, unit->vPosition.z);
			ASSERT(unit->LevelProxMapNode != INVALID_ID);
		}
	}

#endif //ISVERSION(SERVER_VERSION)

	unit->tRoomNode.pNext = room->tUnitList.ppFirst[eTargetType];
	if (unit->tRoomNode.pNext)
	{
		unit->tRoomNode.pNext->tRoomNode.pPrev = unit;
	}
	room->tUnitList.ppFirst[eTargetType] = unit;
	unit->eTargetType = eTargetType;
	unit->pRoom = room;

	UnitRoomListDebug(unit, room);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitRemoveFromRoomList( 
	UNIT *unit, 
	bool bRemoveFromLevelProxMap )
{
	ASSERT_RETURN(unit);
	if (!unit->pRoom)
	{
		return;
	}
	UnitRoomListDebug(unit, unit->pRoom);

	ASSERT_RETURN(unit->eTargetType >= 0 && unit->eTargetType < NUM_TARGET_TYPES);
	ASSERT(unit->tRoomNode.pPrev || unit->pRoom->tUnitList.ppFirst[unit->eTargetType] == unit);

#if ISVERSION(SERVER_VERSION)
	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	if (bRemoveFromLevelProxMap && unit->LevelProxMapNode != INVALID_ID) {
		ASSERT(unit->pRoom->pLevel && unit->pRoom->pLevel->m_pUnitPosProxMap);
		if (unit->pRoom->pLevel && unit->pRoom->pLevel->m_pUnitPosProxMap)
			unit->pRoom->pLevel->m_pUnitPosProxMap->Delete(unit->LevelProxMapNode);
		unit->LevelProxMapNode = INVALID_ID;
	}
#endif // ISVERSION(SERVER_VERSION)

	if (unit->tRoomNode.pPrev)
	{
		unit->tRoomNode.pPrev->tRoomNode.pNext = unit->tRoomNode.pNext;
	}
	else
	{
		unit->pRoom->tUnitList.ppFirst[unit->eTargetType] = unit->tRoomNode.pNext;
	}
	if (unit->tRoomNode.pNext)
	{
		unit->tRoomNode.pNext->tRoomNode.pPrev = unit->tRoomNode.pPrev;
	}
	unit->tRoomNode.pPrev = NULL;
	unit->tRoomNode.pNext = NULL;

	UnitRoomListDebug(unit, unit->pRoom);

	unit->pRoom = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitOverrideAppearanceSet(
	GAME *pGame,
	UNIT * pUnit,
	EXCELTABLE eTable,
	int nOverrideClass )
{
	ASSERT_RETURN(pUnit);
	const UNIT_DATA * pOverrideData = (UNIT_DATA *) ExcelGetData( pGame, eTable, nOverrideClass );
	ASSERT_RETURN( pOverrideData );

	ASSERT_RETURN( pUnit->nAppearanceDefIdOverride == INVALID_ID );
	UnitDataLoad( pGame, eTable, nOverrideClass );
	ASSERT_RETURN( pOverrideData->nAppearanceDefId != INVALID_ID );
	pUnit->nAppearanceDefIdOverride = pOverrideData->nAppearanceDefId;

	if ( IS_CLIENT( pGame ) )
	{
		ASSERT_RETURN( pUnit->pGfx->nModelIdThirdPersonOverride == INVALID_ID );
		ASSERT_RETURN( !UnitDataTestFlag(pOverrideData, UNIT_DATA_FLAG_WARDROBE_PER_UNIT) );

		UnitSetFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE, TRUE ); // I'm doing this here so that the ASSERT_RETURN calls don't hit it

		pUnit->pGfx->nModelIdThirdPersonOverride = c_AppearanceNew( pUnit->nAppearanceDefIdOverride, 0, pUnit, pOverrideData, NULL, STANCE_DEFAULT, pUnit->pAppearanceShape );
		c_AnimationPlayByMode( pUnit, pGame, pUnit->pGfx->nModelIdThirdPersonOverride, MODE_IDLE, 0, 0.0f, PLAY_ANIMATION_FLAG_ONLY_ONCE | PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_DEFAULT_ANIM);

		e_ModelSetScale( pUnit->pGfx->nModelIdThirdPersonOverride, pOverrideData->fScale );
		const APPEARANCE_SHAPE * pShape = c_AppearanceShapeGet( pUnit->pGfx->nModelIdThirdPerson );
		ASSERT_RETURN( pShape );
		c_AppearanceShapeSet( pUnit->pGfx->nModelIdThirdPersonOverride, *pShape );
		UnitModelGfxInit( pGame, pUnit, eTable, nOverrideClass, pUnit->pGfx->nModelIdThirdPersonOverride );

		e_ModelSetDrawAndShadow( pUnit->pGfx->nModelIdCurrent, FALSE );
		pUnit->pGfx->nModelIdCurrent = c_UnitGetModelIdThirdPersonOverride( pUnit );
		e_ModelSetDrawAndShadow( pUnit->pGfx->nModelIdCurrent, TRUE );

		if ( ! DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnit->nAppearanceDefIdOverride ) )
			UnitSetFlag( pUnit, UNITFLAG_APPEARANCE_LOADED, FALSE );

		pUnit->pAppearanceOverrideUnitData = pOverrideData;
	} else {
		UnitSetFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE, TRUE );
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitOverrideAppearanceClear(
	GAME *pGame,
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);
	UnitSetFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE, FALSE );
	pUnit->nAppearanceDefIdOverride = INVALID_ID;
	if ( IS_CLIENT( pGame ) )
	{
		c_AppearanceDestroy( pUnit->pGfx->nModelIdThirdPersonOverride );
		pUnit->pGfx->nModelIdThirdPersonOverride = INVALID_ID; 
		if ( GameGetControlUnit( pGame ) == pUnit )
		{
			c_UnitUpdateViewModel( pUnit, TRUE, FALSE );
		} else {
			pUnit->pGfx->nModelIdCurrent = c_UnitGetModelIdThirdPerson( pUnit );
			e_ModelSetDrawAndShadow( pUnit->pGfx->nModelIdCurrent, TRUE );
		}

		c_AppearanceUpdateAnimationGroup( pUnit, pGame, c_UnitGetModelIdThirdPerson( pUnit ), UnitGetAnimationGroup( pUnit ) );
		pUnit->pAppearanceOverrideUnitData = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitGetWorldMatrixFor1stPerson(
	GAME * pGame,
	VECTOR & vPosition,
	MATRIX & matWorld )
{
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( TRUE );

	if ( ! pCameraInfo )
	{
		MatrixIdentity( &matWorld );
		return;
	}

	float fScale = 1.0f;
	{
		UNIT * pCameraUnit = GameGetCameraUnit( pGame );
		int nModelId = c_UnitGetModelIdFirstPerson( pCameraUnit );
		if ( nModelId == INVALID_ID )
			nModelId = c_UnitGetModelIdThirdPerson( pCameraUnit );
		if ( nModelId != INVALID_ID )
			fScale = e_ModelGetScale( nModelId ).fX;
	}
	VECTOR vCameraPosition = CameraInfoGetPosition(pCameraInfo);
	VECTOR vEyeToFeet = vPosition - vCameraPosition;
	vEyeToFeet.fZ += c_CameraGetZDelta();

	float fCameraAngle = CameraInfoGetAngle(pCameraInfo);
	MatrixTransform(&matWorld, &vCameraPosition, fCameraAngle, CameraInfoGetPitch(pCameraInfo));
	MATRIX mOffsetToFeet;
	MatrixTransform(&mOffsetToFeet, NULL, - fCameraAngle );
	MatrixMultiply(&vEyeToFeet, &vEyeToFeet, &mOffsetToFeet);
	vEyeToFeet *= fScale;

	//all animated models are off by 90 degrees
	MatrixTransform(&mOffsetToFeet, &vEyeToFeet, PI / 2.0f);
	MatrixMultiply(&matWorld, &mOffsetToFeet, &matWorld);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInLineOfSight(
	GAME *pGame,
	UNIT* pUnit,
	UNIT* pTarget )
{
	ASSERTX_RETFALSE( ( pUnit && pTarget ), "Unit & Target required" );
	LEVEL* pLevel = UnitGetLevel( pUnit );

	VECTOR CollisionNormal;
	VECTOR CurrentPoint = UnitGetAimPoint(pUnit);
	VECTOR NextPoint = UnitGetAimPoint(pTarget);
	VECTOR Delta = NextPoint - CurrentPoint;
	float fDist = VectorLength( Delta );
	VectorNormalize( Delta );
 
	DWORD dwMoveFlags = 0;
//	DWORD dwMoveResult = 0;
	dwMoveFlags |= MOVEMASK_TEST_ONLY;
	dwMoveFlags |= MOVEMASK_DOORS;
	dwMoveFlags |= MOVEMASK_NOT_WALLS;
	VECTOR TestPoint = NextPoint;
	//ConsoleString( CONSOLE_ERROR_COLOR, "LOS TEST!" );
	float fCollideDistance = LevelLineCollideLen(pGame, pLevel, CurrentPoint, Delta, fDist, &CollisionNormal);
	COLLIDE_RESULT result;
	if (fCollideDistance >= fDist)
	{
		RoomCollide(pUnit, CurrentPoint, Delta, &TestPoint, UnitGetCollisionRadius( pUnit ), .5f, dwMoveFlags, &result);
	}
	if( fCollideDistance < fDist ||
		  TESTBIT(&result.flags, MOVERESULT_COLLIDE_UNIT) )
	{
		return FALSE;	
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitSetModelLocation(
	GAME * game,
	UNIT* unit,
	ROOM * room,
	int nModelId,
	MODEL_SORT_TYPE SortType = MODEL_DYNAMIC )
{
	ASSERT_RETURN(unit);

	if( AppIsTugboat() )
	{
		if ( e_ModelExists( nModelId ) && room )
		{
			ROOMID nModelRoomID = (int)e_ModelGetRoomID( nModelId );
			if( nModelRoomID == room->idRoom  &&
				( c_UnitGetNoDrawTemporary( unit ) || c_UnitGetNoDraw( unit )  ) )
			{
				return;
			}
		}
	}

	//VECTOR vPosition = unit->vPosition;
	VECTOR vPosition = UnitGetDrawnPosition(unit);
	MATRIX matWorld;

	ROOMID idRoom = UnitGetRoomId(unit);

	if ( unit == GameGetCameraUnit(game) )
	{
//		if ( (c_UnitGetModelIdFirstPerson( unit ) == nModelId || unit != GameGetControlUnit(game)) 
		// KCK: Why do this for non-first person views? It's breaking views of other players.
		if ( c_UnitGetModelIdFirstPerson( unit ) == nModelId
			&& !AppGetDetachedCamera())
		{
			c_UnitGetWorldMatrixFor1stPerson(game, unit->vPosition, matWorld);
		}
		else
		{
			VECTOR vFace = -UnitGetFaceDirection( unit, TRUE );
			MatrixFromVectors( matWorld, unit->vPosition, unit->vUpDirection, vFace );
			vPosition = unit->vPosition;	// because the drawnposition isn't properly updated for control unit 3p
		}
	}
	//else if ( unit == GameGetCameraUnit( game ) )
	//{
	//	c_UnitGetWorldMatrixFor1stPerson(game, unit->vPosition, matWorld);
	//}
#ifdef HAVOK_ENABLED
	else if (unit->pHavokRigidBody)
	{
		int nAppearanceDef = c_AppearanceGetDefinition( nModelId );
		VECTOR vBoundingBoxOffset = AppearanceDefinitionGetBoundingBoxOffset( nAppearanceDef );
		HavokRigidBodyGetMatrix( unit->pHavokRigidBody, matWorld, vBoundingBoxOffset );
	}
#endif
	else
	{
		VECTOR vFaceAway = UnitGetFaceDirection( unit, TRUE );
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		if ( ! UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_FLIP_FACE_DIRECTION) ) // usually we flip it... so this is correct...
			vFaceAway = - vFaceAway;
		if ( pUnitData->fOffsetUp != 0.0f )
		{
			float fOffsetUp = pUnitData->fOffsetUp * UnitGetScale( unit );
			VECTOR vOffsetUp = unit->vUpDirection;
			vOffsetUp *= fOffsetUp;
			vPosition += vOffsetUp;
		}
		MatrixFromVectors(matWorld, vPosition, unit->vUpDirection, vFaceAway);
	}

	if (nModelId != INVALID_ID)
	{
#ifdef HAVOK_ENABLED
		VECTOR vRagdollPosition;
		if ( IsUnitDeadOrDying( unit ) && c_RagdollGetPosition( nModelId, vRagdollPosition ) 
			)
		{
			float fScale = UnitGetScale( unit );
			if ( fScale != 0.0f && fScale != 1.0f )
			{
				VECTOR vDelta = vRagdollPosition - UnitGetPosition( unit );
 				vDelta *= fScale;
				vRagdollPosition = UnitGetPosition( unit ) + vDelta;
			}
			// update model location with ragdoll
			// this is causing the dying to pop up into the air on the first frame
			// we still need the position to get the right room
			//vPosition = vRagdollPosition;
			//*(VECTOR*)&matWorld._41	= vPosition;
			ROOM * pNewRoom = RoomGetFromPosition( game, room, &vRagdollPosition );
			if ( pNewRoom )
				idRoom = pNewRoom->idRoom;  // if we don't find a new room, don't change it

			// CML 2007.06.21: This forces the next ModelSetLocation to go through, which updates culling boxes with the ragdoll pos.
			V( e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_FORCE_SETLOCATION, TRUE ) );
		}
#endif
		// if an item on the ground, do a special matrix
		if ( AppIsHellgate() && UnitTestFlag( unit, UNITFLAG_CANBEPICKEDUP ) )
		{
			ASSERT( UnitGetGenus( unit ) == GENUS_ITEM );
			VECTOR vFace;
			int dwTime = (int)AppCommonGetCurTime();
			float fTimeInSeconds = (float) dwTime / 1000.0f;
			float fDelta = fTimeInSeconds - ( FLOOR( fTimeInSeconds / 5.0f ) * 5.0f );
			float angle = ( fDelta * TWOxPI ) / 5.0f;

			MATRIX mRotation;
			MatrixTransform( &mRotation, NULL, angle );

			int nAppearanceDef = c_AppearanceGetDefinition( nModelId );
			VECTOR vOffset = -AppearanceDefinitionGetBoundingBoxOffset( nAppearanceDef );
			VECTOR vSize = AppearanceDefinitionGetBoundingBoxSize( nAppearanceDef );

			MATRIX mToCenter;
			MatrixTransform( &mToCenter, &vOffset, 0.0f );

			vPosition = VECTOR(0.0f);
			MatrixMultiply( &vPosition, &vPosition, &matWorld );
			vPosition.fZ += 0.5f;

			MatrixTransform( &matWorld, &vPosition, 0.0f );
			MatrixMultiply( &matWorld, &mRotation, &matWorld );
			MatrixMultiply( &matWorld, &mToCenter, &matWorld );
		}
		c_ModelSetLocation( nModelId, &matWorld, vPosition, idRoom, SortType );
	}
}

static void sUnitCheckKarma( UNIT * unit )
{

	int newvalue = UnitGetStat( unit, STATS_KARMA );

	if ( IS_CLIENT( unit ) &&
		PlayerIsInPVPWorld( unit ) )
	{
		if (newvalue >= MAX_KARMA_PENALTY * .25f && newvalue < MAX_KARMA_PENALTY * .5f)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_THREEQUARTERS_KARMA))
			{
				c_StateSet(unit, unit, STATE_THREEQUARTERS_KARMA, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_HALF_HEALTH, 0);
		}

		if (newvalue >= MAX_KARMA_PENALTY * .5f && newvalue < MAX_KARMA_PENALTY * .75f)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_HALF_KARMA))
			{
				c_StateSet(unit, unit, STATE_HALF_KARMA, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_HALF_KARMA, 0);
		}

		if (newvalue >= MAX_KARMA_PENALTY * .75f && newvalue < MAX_KARMA_PENALTY)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_QUARTER_KARMA))
			{
				c_StateSet(unit, unit, STATE_QUARTER_KARMA, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_QUARTER_KARMA, 0);
		}

		if (newvalue >= MAX_KARMA_PENALTY)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_ZERO_KARMA))
			{
				c_StateSet(unit, unit, STATE_ZERO_KARMA, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_ZERO_KARMA, 0);
		}
	}

}

#define HOSTILE_COLOR .1f
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_UnitUpdateVisibilityAndAmbientLight(
	UNIT * unit )
{
	ASSERT_RETURN(unit);
	if ( AppIsHellgate() )
		return;

	GAME * game = UnitGetGame( unit );

	if( UnitIsA( unit, UNITTYPE_PLAYER ) )
	{
		sUnitCheckKarma( unit );
	}

	ROOM* room = UnitGetRoom(unit);
	if (!room)
	{
		//if( !( UnitIsA( unit, UNITTYPE_ITEM ) && UnitGetUltimateOwner( unit ) != unit ) )
		{
			int nModelId = c_UnitGetModelId(unit);
			if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
				nModelId = c_UnitGetModelIdThirdPersonOverride( unit );

			int nWardrobe = UnitGetWardrobe( unit );
			BOOL bIsNoDraw = e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW );
			if( !UnitTestFlag( unit, UNITFLAG_APPEARANCE_LOADED ) ||
				bIsNoDraw )
			{
				/*if( !UnitHasState( game, unit, STATE_FADE_OUT ) )
				{
					c_StateSet(unit, unit, STATE_FADE_OUT, 0, 0, INVALID_ID);
				}*/
				e_ModelSetDrawTemporary( nModelId, FALSE );
				unit->bVisible = FALSE;
			}
			else
			{
				/*if( UnitHasState( game, unit, STATE_FADE_OUT ) )
				{
					StateClearAllOfType(unit, STATE_FADE_OUT);
				}*/
				e_ModelSetDrawTemporary( nModelId, TRUE );
				unit->bVisible = TRUE;
			}
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( game, nWardrobe, i );

					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					e_ModelSetDrawTemporary( nWeaponModelId, unit->bVisible );
				}
				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				for( int i = 0; i < pClient->nAttachmentCount; i++ )
				{
					ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
					if( pAttachment )
					{
						c_AttachmentSetDraw( *pAttachment, unit->bVisible );		
					}
				}
			}

		}

		return;
	}

	int nModelId = c_UnitGetModelId(unit);
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		nModelId = c_UnitGetModelIdThirdPersonOverride( unit );
	BOOL bWasVisible = unit->bVisible;
	if ( !UnitTestFlag( unit, UNITFLAG_DONT_SET_WORLD ) && 
		 !c_AppearanceIsHidden( nModelId ))
	{
		VECTOR vPosition = unit->vDrawnPosition;

		//const UNIT_DATA * pUnitData = UnitGetData( unit ); //not used.

		if (nModelId != INVALID_ID)
		{

			LEVEL* pLevel = UnitGetLevel( unit );
			int Visible =  10;

			{
				Visible = 0;
				const VECTOR pvOffsets[] = 
				{
					VECTOR(  0,  0,  0 ),
					VECTOR(  1,  0,  0 ),
					VECTOR( -1,  0,  0 ),
					VECTOR(  0,  1,  0 ),
					VECTOR(  0, -1,  0 ),
					VECTOR( -1,  1,  0 ),
					VECTOR( -1, -1,  0 ),
					VECTOR(  1,  1,  0 ),
					VECTOR(  1, -1,  0 ),
				};
				const int nOffsetCount = arrsize( pvOffsets );
				for ( int i = 0; i < nOffsetCount; i++ )
				{
					if ( Visible >= 5 )
						break;
					VECTOR vOffset = pvOffsets[ i ];
					vOffset *= 0.75f;
					if( LevelIsVisible( pLevel, vPosition + vOffset ) )
					{
						Visible++;
					}
				}
			}

			BOOL ForcePosition = FALSE;
			BOOL IsDoor = ObjectIsDoor( unit );
			if( IsDoor )
			{
				ForcePosition = TRUE;
			}
			UNIT *pPlayer = GameGetControlUnit(game);
			if( ( unit->bStructural && !IsDoor )||
				( unit == pPlayer ) )
			{
				Visible = 10;
			}

			BOOL bIsNoDraw = e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW );
			if( bIsNoDraw )
			{
				Visible = 0;
				unit->bVisible = FALSE;
			}

			if( !UnitTestMode( unit, MODE_SPAWN ) &&
				( UnitGetGenus(unit) == GENUS_MONSTER ||
				UnitIsA( unit, UNITTYPE_PLAYER ) ||
				UnitIsA( unit, UNITTYPE_ITEM ) ||
				UnitIsA( unit, UNITTYPE_DESTRUCTIBLE ) ||
				( UnitIsA(unit, UNITTYPE_OBJECT ) &&
				UnitCanInteractWith(unit, pPlayer) &&
				!IsUnitDeadOrDying(unit) ) ) &&
				//!UnitIsA( unit, UNITTYPE_DESTRUCTIBLE) &&
				!IsUnitDeadOrDying( unit ) &&
				!IsDoor &&
				!unit->bStructural )
			{
				if( ( UnitIsA( unit, UNITTYPE_DESTRUCTIBLE ) ||
					( UnitIsA(unit, UNITTYPE_OBJECT) &&
					UnitCanInteractWith(unit, pPlayer) ) ||
					IsUnitDeadOrDying(unit) ) && Visible < 3  )
				{
					Visible = 0;
					c_AppearanceSetDrawBehind( nModelId, FALSE );
					c_sUnitSetModelLocation( game, unit, room, nModelId, MODEL_DYNAMIC );
				}
				else
				{

					c_AppearanceSetDrawBehind( nModelId, TRUE );
					c_sUnitSetModelLocation( game, unit, room, nModelId, MODEL_ICONIC );
				}

			}
			else
			{
				c_AppearanceSetDrawBehind( nModelId, FALSE );
				c_sUnitSetModelLocation( game, unit, room, nModelId, MODEL_DYNAMIC );
			}
			if( Visible < 5 &&
				IsDoor )
			{
				Visible = 5;
			}
			float Ambient = (float)Visible / 5.0f;
			if( Ambient > 1 )
			{
				Ambient = 1;
			}			
			if( !RoomIsOutdoorRoom( game, room ) &&
				!UnitIsA( unit, UNITTYPE_PLAYER ) &&
				pLevel->m_pVisibility )
			{
				VECTOR DistFromCenter = vPosition - pLevel->m_pVisibility->CenterFloat();
				DistFromCenter.fZ = 0;
				float FadeDistance = VectorLength( DistFromCenter );
				FadeDistance /= 10.0f;
				FadeDistance *= FadeDistance;
				FadeDistance = 1.0f - FadeDistance;
				if( FadeDistance < .05f )
				{
					FadeDistance = .05f;
				}
				if( !unit->bStructural || IsDoor )
				{
					Ambient *= FadeDistance;
				}
			}
			/*if( UnitIsA( unit, UNITTYPE_MONSTER ) ||
			UnitIsA( unit, UNITTYPE_DESTRUCTIBLE) ||
			( UnitIsA(unit, UNITTYPE_OBJECT) &&
			UnitCanInteractWith(unit, pPlayer) &&
			!IsUnitDeadOrDying(unit) ) )//&&
			//!UnitIsA( unit, UNITTYPE_DESTRUCTIBLE) )*/
			//if( !UnitTestMode( unit, MODE_IDLE_PAPERDOLL )  )
			{
				float fAlphaThreshhold = ( 1.0f - pLevel->m_pLevelDefinition->fVisibilityOpacity );
				if( Ambient <= 0 )
				{
					if( bIsNoDraw ||
						( c_AppearanceGetModelAmbientLight( nModelId ) <= fAlphaThreshhold &&
						  !IsDoor ) )
					{

						if( AppGetState() != APP_STATE_IN_GAME )
						{
							e_ModelSetDrawTemporary( nModelId, FALSE );
						}
						else
						{
							if( !UnitHasState( game, unit, STATE_FADE_OUT ) )
							{
								c_StateSet(unit, unit, STATE_FADE_OUT, 0, 0, INVALID_ID);
							}
						}
						unit->bVisible = FALSE;
					}
					else
					{
						if( AppGetState() == APP_STATE_IN_GAME )
						{
							if( UnitHasState( game, unit, STATE_FADE_OUT ) )
							{
								StateClearAllOfType(unit, STATE_FADE_OUT);
								CLEARBIT(unit->pdwStateFlags, STATE_FADE_OUT);

							}
						}
						e_ModelSetDrawTemporary( nModelId, TRUE );
						unit->bVisible = TRUE;
					}
				}
				else
				{
					if( bIsNoDraw )
					{
						if( AppGetState() == APP_STATE_IN_GAME )
						{
							if( !UnitHasState( game, unit, STATE_FADE_OUT ) )
							{
								c_StateSet(unit, unit, STATE_FADE_OUT, 0, 0, INVALID_ID);
							}
						}
						else
						{
							e_ModelSetDrawTemporary( nModelId, FALSE );
						}
						unit->bVisible = FALSE;
					}
					else
					{
						if( AppGetState() == APP_STATE_IN_GAME )
						{
							if( UnitHasState( game, unit, STATE_FADE_OUT ) )
							{
								StateClearAllOfType(unit, STATE_FADE_OUT);
								CLEARBIT(unit->pdwStateFlags, STATE_FADE_OUT);

							}
						}
						e_ModelSetDrawTemporary( nModelId, TRUE );
						unit->bVisible = TRUE;
					}
				}
			}


			if( game->eGameState == GAMESTATE_LOADING )
			{
				Ambient = 1;
			}
			float fColor = 0;
			if( unit->bVisible )
			{
				BOOL bHostile = FALSE;
				if( UnitIsA( UnitGetUltimateOwner( unit ), UNITTYPE_MONSTER ) )
					bHostile = GameGetControlUnit( game ) ? TestHostile(game, GameGetControlUnit( game ) ,UnitGetUltimateOwner( unit ) ) : FALSE;
				if( bHostile )
				{
					fColor = HOSTILE_COLOR;
				}
				Ambient = 1.0f - ( ( 1.0f - Ambient ) * pLevel->m_pLevelDefinition->fVisibilityOpacity );
				c_AppearanceSetAmbientLight( nModelId, Ambient );
				c_AppearanceSetBehindColor( nModelId, fColor );

			}
		
			if( AppIsTugboat() &&
				IS_CLIENT( game ) &&
				unit->bVisible && !bWasVisible &&
				unit->m_pPathing )
			{
				// snap 'em to the ground!
				UnitForceToGround(unit);
			}


			// hacky, but ok for now maybe ;) -Travis
			if (UnitIsA( unit, UNITTYPE_ANCHORSTONE_MARKER ) )
			{
				if( pLevel && LevelGetAnchorMarkers( pLevel ) && GameGetControlUnit( game ) )
				{
					LevelGetAnchorMarkers( pLevel )->c_AnchorMarkerSetState( GameGetControlUnit( game ), unit );
				}
			}

			int nWardrobe = UnitGetWardrobe( unit );
			if( nWardrobe != INVALID_ID )
			{
				BOOL bHideWeapons = c_AppearanceAreWeaponsHidden( nModelId ) ? FALSE : unit->bVisible;

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( game, nWardrobe, i );


					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					if( unit->bVisible )
					{
						c_AppearanceSetAmbientLight( nWeaponModelId, Ambient );
						c_AppearanceSetBehindColor( nWeaponModelId, fColor );
					}
					e_ModelSetDrawTemporary( nWeaponModelId, !UnitTestFlag( unit, UNITFLAG_APPEARANCE_LOADED  ) ? FALSE : bHideWeapons );
				}

				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				ASSERT_RETURN( pClient );
				for( int i = 0; i < pClient->nAttachmentCount; i++ )
				{
					ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
					//e_ModelSetAmbientLight(  pAttachment->nId, Ambient, 1000 );
					if( unit->bVisible )
					{
						c_AppearanceSetAmbientLight( pAttachment->nAttachedId, Ambient );
						c_AppearanceSetBehindColor( pAttachment->nAttachedId, fColor );
					}
					if ( !UnitTestFlag( unit, UNITFLAG_APPEARANCE_LOADED ) )
					{
						c_AttachmentSetDraw( *pAttachment, FALSE );
					}
					

				}
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define FORCE_FACE_TARGET_MIN_VELOCITY 7.0f
void c_UnitUpdateFaceTarget(
	UNIT * unit,
	BOOL bForce )
{
#if !ISVERSION(SERVER_VERSION)
	// Mythos REALLY doesn't need to do this, and it's super expensive!
	if( AppIsTugboat() )
	{
		return;
	}
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame( unit );

	if ( unit == GameGetCameraUnit( game ) && unit != GameGetControlUnit( game ) )
		bForce = TRUE;

	UNITID idTarget = UnitGetAITargetId(unit);
	if (idTarget == INVALID_ID && unit == GameGetControlUnit( game ) )
	{
		UNIT * pTarget = UIGetTargetUnit();
		idTarget = UnitGetId( pTarget );
	}
	else if (idTarget == INVALID_ID)
	{
		UNIT * pTarget = SkillGetAnyTarget( unit, INVALID_ID, NULL, TRUE );
		idTarget = UnitGetId( pTarget );
	}
	VECTOR vFaceTarget(0);
	if ( idTarget != INVALID_ID && 
		(UnitIsA( unit, UNITTYPE_MONSTER) || SkillIsOnWithFlag( unit, SKILL_FLAG_IS_MELEE )) )
	{
		UNIT* pTarget = UnitGetById(game, idTarget);
		if (pTarget)
		{
			vFaceTarget = UnitGetAimPoint(pTarget);
		}
	} else {
		int nWardrobe = UnitGetWardrobe( unit );
		
		int nWeaponIndex = 0;
		UNIT * pWeapon = WardrobeGetWeapon( game, nWardrobe, nWeaponIndex );
		if ( ! pWeapon || UnitIsA( pWeapon, UNITTYPE_MELEE ) || UnitIsA( pWeapon, UNITTYPE_SHIELD ) )
			nWeaponIndex++;
		pWeapon = WardrobeGetWeapon( game, nWardrobe, nWeaponIndex );
		if ( ! pWeapon || UnitIsA( pWeapon, UNITTYPE_MELEE ) || UnitIsA( pWeapon, UNITTYPE_SHIELD ) )
			nWeaponIndex = INVALID_ID;
		if ( UnitIsInTown( unit ) )
			nWeaponIndex = INVALID_ID;
		if ( nWeaponIndex != INVALID_ID )
		{
			VECTOR vWeaponPosition;
			VECTOR vWeaponDirection;
			UnitGetWeaponPositionAndDirection( game, unit, nWeaponIndex, &vWeaponPosition, &vWeaponDirection );
			vWeaponDirection *= 5.0f;
			vFaceTarget = vWeaponPosition + vWeaponDirection;
		} else {
			VECTOR vFaceDirection = UnitGetFaceDirection( unit, FALSE );
			if ( unit == GameGetControlUnit( game ) || unit != GameGetCameraUnit( game ) )
				vFaceDirection.fZ = 0.0f;
			VectorNormalize( vFaceDirection );
			vFaceTarget = UnitGetPositionAtPercentHeight( unit, 0.8f ) + 3.0f * vFaceDirection;
		}
	}
	if (IsUnitDeadOrDying(unit) || UnitHasState( game, unit, STATE_NO_HEAD_TURNING ))
	{
		vFaceTarget = VECTOR(0);
	}
	if ( unit == GameGetControlUnit( game ) )
	{
		switch ( c_CameraGetViewType())
		{
		case VIEW_VANITY:
			if ( !c_WeaponGetLockOnVanity() )
				vFaceTarget = VECTOR(0);
			break;
		case VIEW_CHARACTER_SCREEN:
			vFaceTarget = VECTOR(0);
			break;
		}
	}

	//if ( !bForce && UnitGetVelocity( unit ) > FORCE_FACE_TARGET_MIN_VELOCITY )
	//	bForce = TRUE;

	int nModelId = c_UnitGetModelId(unit);

	c_AppearanceFaceTarget(nModelId, vFaceTarget, bForce);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitUpdateGfx(
	UNIT* unit,
	BOOL bUpdateLighting /* = TRUE */ )
{
	ASSERT_RETURN(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	UNITGFX *gfx = unit->pGfx;
	if (!gfx)
	{
		return;
	}

	ROOM* room = UnitGetRoom(unit);

	int nModelId = c_UnitGetModelId(unit);

	// in Mythos, this is handled in the updatelighting area
	if( AppIsHellgate() )
	{
		if ( room && ! UnitTestFlag( unit, UNITFLAG_DONT_SET_WORLD ) )
		{
			if ( GameGetControlUnit( game ) == unit )
			{
				c_sUnitSetModelLocation( game, unit, room, c_UnitGetModelIdFirstPerson( unit ) );
				c_sUnitSetModelLocation( game, unit, room, c_UnitGetModelIdThirdPerson( unit ) );
				if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
					c_sUnitSetModelLocation( game, unit, room, c_UnitGetModelIdThirdPersonOverride( unit ) );
			} else {
				c_sUnitSetModelLocation( game, unit, room, nModelId );
			}
		}
	}

	c_UnitUpdateFaceTarget( unit, FALSE );

	if ( !UnitTestFlag( unit, UNITFLAG_APPEARANCE_LOADED ) &&
		 UnitGetAppearanceDefId( unit ) != INVALID_ID )
	{
#define NUM_MODELS_PER_UNIT 5
		int pnModelIds[ NUM_MODELS_PER_UNIT ] = { INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID };

		pnModelIds[ 0 ] = unit->pGfx->nModelIdFirstPerson;
		pnModelIds[ 1 ] = unit->pGfx->nModelIdThirdPerson;
		pnModelIds[ 2 ] = unit->pGfx->nModelIdThirdPersonOverride;
		pnModelIds[ 3 ] = unit->pGfx->nModelIdInventory;
		pnModelIds[ 4 ] = unit->pGfx->nModelIdInventoryInspect;

		BOOL bAllLoaded = TRUE;
		for ( int i = 0; i < NUM_MODELS_PER_UNIT; i++ )
		{
			int nAppearanceDefId = e_ModelGetAppearanceDefinition( pnModelIds[ i ] );
			if ( nAppearanceDefId == INVALID_ID )
				continue;
			APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_APPEARANCE, nAppearanceDefId);
			if ( ! pAppearanceDef )
				bAllLoaded = FALSE;
		}
		if ( unit->pGfx->nModelIdThirdPerson != INVALID_ID && ! c_AppearanceBonesLoaded( unit->pGfx->nModelIdThirdPerson ) )
			bAllLoaded = FALSE;
		if ( unit->pGfx->nModelIdInventory != INVALID_ID && ! c_AppearanceBonesLoaded( unit->pGfx->nModelIdInventory ) )
			bAllLoaded = FALSE;
		if ( unit->pGfx->nModelIdInventoryInspect != INVALID_ID && ! c_AppearanceBonesLoaded( unit->pGfx->nModelIdInventoryInspect ) )
			bAllLoaded = FALSE;

		if ( bAllLoaded )
		{
			if ( nModelId != INVALID_ID )
			{
				UnitSetFlag(unit, UNITFLAG_APPEARANCE_LOADED);
				c_UnitInitializeAnimations( unit );

				if( AppIsTugboat() &&
					UnitTestFlag(unit, UNITFLAG_DONT_PLAY_IDLE) &&
					!UnitTestMode(unit, MODE_IDLE_CHARSELECT) )
				{
					UnitClearFlag( unit, UNITFLAG_DONT_PLAY_IDLE);

					if( IsUnitDeadOrDying( unit ) )
					{
						BOOL bHasMode = FALSE;
						UnitGetModeDuration( game, unit, MODE_SPAWNDEAD, bHasMode );
						if( bHasMode )
						{
							c_UnitSetMode( unit, MODE_SPAWNDEAD, 0, 0);
						}
						else
						{
							c_UnitSetMode( unit, MODE_DYING, 0, 0);
						}
					}
					else
					{
						c_AnimationPlayByMode(unit, MODE_IDLE, 0.0f, PLAY_ANIMATION_FLAG_ONLY_ONCE | PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_DEFAULT_ANIM);
					}

					UnitSetFlag(unit, UNITFLAG_DONT_PLAY_IDLE);
				}
				
				int nWardrobe = c_AppearanceGetWardrobe( nModelId );
				if ( nWardrobe != INVALID_ID )
					c_WardrobeUpdateAttachments ( unit, nWardrobe, nModelId, TRUE, FALSE, TRUE );

				if ( unit == GameGetControlUnit( game ) )
					c_UnitUpdateViewModel( unit, FALSE );

				if( AppIsTugboat() &&
					AppGetState() == APP_STATE_IN_GAME &&
					( UnitIsA( unit, UNITTYPE_MONSTER ) ||
					UnitIsA( unit, UNITTYPE_PLAYER ) ||
					UnitIsA( unit, UNITTYPE_OBJECT ) ||
					UnitIsA( unit, UNITTYPE_DESTRUCTIBLE ) )
					&& !UnitTestMode( unit, MODE_SPAWN ) )
				{
					UnitSetStat( unit, STATS_FADE_TIME_IN_TICKS, 0 );
					UnitSetStat( unit, STATS_NO_DRAW, 1 ); 
					e_ModelSetDrawTemporary( nModelId, FALSE );
					UnitSetStat( unit, STATS_FADE_TIME_IN_TICKS, 30 );
					UnitSetStat( unit, STATS_NO_DRAW, 0 ); 
					
				}
				else
				{
					if( AppIsTugboat() && nModelId != INVALID_ID)
						e_ModelSetDrawTemporary( nModelId, TRUE );			
				}

			}
			//trace("c_WardrobeUpdateAttachments called for %s\n", unit->pUnitData->szName );
		}
		else
		{
			if( AppIsTugboat() && nModelId != INVALID_ID )
				e_ModelSetDrawTemporary( nModelId, FALSE );			
		}
	}

	// client needs to do this AFTER a bone update, in case the state requires bone attachments
	if( !UnitTestFlag( unit, UNITFLAG_STATE_GRAPHICS_APPLIED ) &&
			UnitTestFlag( unit, UNITFLAG_APPEARANCE_LOADED ) )
	{
		int nModelId = c_UnitGetModelId(unit);
		int nModelRoomID = c_ModelGetRoomID( nModelId );
		int nModelDefId = e_ModelGetDefinition( nModelId );

		if( nModelId != INVALID_ID && 
			(nModelDefId == INVALID_ID || e_ModelDefinitionGetBoundingBox( nModelDefId, LOD_ANY )) && 
			nModelRoomID != INVALID_ID )
		{
			UnitSetFlag( unit, UNITFLAG_STATE_GRAPHICS_APPLIED );
			c_StatesApplyAfterGraphicsLoad( unit );
		}
	}


	if ( AppIsTugboat() && bUpdateLighting )
		c_UnitUpdateVisibilityAndAmbientLight( unit );

	int nWardrobe = UnitGetWardrobe( unit );
	if ( nWardrobe != INVALID_ID )
	{
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			UNIT * pWeapon = WardrobeGetWeapon( game, nWardrobe, i );
			if ( pWeapon )
				c_UnitUpdateGfx( pWeapon, FALSE );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD UnitGetAppearanceNewFlags( 
	const UNIT_DATA * pUnitData )
{
	ASSERT_RETINVALID(pUnitData);
	DWORD dwAppearanceNewFlags = 0;
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_WARDROBE_PER_UNIT) )
		dwAppearanceNewFlags |= APPEARANCE_NEW_FLAG_FORCE_WARDROBE;
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_WARDROBE_SHARES_MODEL_DEF) )
		dwAppearanceNewFlags |= APPEARANCE_NEW_FLAG_WARDROBE_SHARES_MODELDEF;

	// AE 2007.10.02: Ignoring MIP bonus from monsters.xls because we want to
	//					control it via a priority/budget approach.
	//if (pUnitData->nWardrobeMipBonus)
	//{
	//	int nMipBonus = MIN(pUnitData->nWardrobeMipBonus, (int)(APPEARANCE_NEW_MASK_WARDROBE_MIP_BONUS >> APPEARANCE_NEW_SHIFT_WARDROBE_MIP_BONUS) );
	//	dwAppearanceNewFlags |= nMipBonus << APPEARANCE_NEW_SHIFT_WARDROBE_MIP_BONUS;
	//}

	return dwAppearanceNewFlags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDelayedModelGFXInit(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	//trace( "sDelayedModelGFXInit() '%s' (%d)\n", UnitGetDevName( pUnit ), GameGetTick( pGame ));
	int nModelId = (int)tEventData.m_Data1;
	EXCELTABLE eTable = (EXCELTABLE)tEventData.m_Data2;
	int nClass = (int)tEventData.m_Data3;
	UnitModelGfxInit( pGame, pUnit, eTable, nClass, nModelId );	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitDataLoadTextures(
	APPEARANCE_DEFINITION * pAppearanceDef, 
	EXCELTABLE eTable,
	int nClass )
{
#if !ISVERSION(SERVER_VERSION)
	EXCEL_TABLE * table = ExcelGetTableNotThreadSafe(eTable);
	ASSERT_RETURN(table);
	UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetDataPrivate( table, nClass ); 

	if ( ! pUnitData )
		return;
	for ( int i = 0; i < MAX_UNIT_TEXTURE_OVERRIDES; i++ )
	{
		if ( PStrLen( pUnitData->szTextureSpecificOverrides[ i * 2 ] ) != 0 &&
			PStrLen( pUnitData->szTextureSpecificOverrides[ i * 2 + 1] ) != 0 )
		{
			c_AppearanceLoadTextureSpecificOverrides(&pUnitData->pnTextureSpecificOverrideIds[ i ], pAppearanceDef->pszFullPath, pUnitData->szTextureSpecificOverrides[ i * 2 + 1], TEXTURE_DIFFUSE, pAppearanceDef->nModelDefId);
		} 
	}

	for ( int i = 0; i < UNIT_TEXTURE_OVERRIDE_COUNT; i++ )
	{
		if ( pUnitData->szTextureOverrides[ i ][ 0 ] != 0 )
		{
			TEXTURE_TYPE eTextureType = gpeUnitOverrideToTextureType[ i ];
			c_AppearanceLoadTextureOverrides(pUnitData->pnTextureOverrideIds[ i ], pAppearanceDef->pszFullPath, pUnitData->szTextureOverrides[ i ], eTextureType, pAppearanceDef->nModelDefId);

			for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
			{
				ASSERTV( pUnitData->pnTextureOverrideIds[ i ][ nLOD ] != 0, "Erroneous zero texture ID encountered in unit data overrides!\n\n%s", pUnitData->szName );
			}
		} 
	}
#endif //  ! ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitModelGfxInit(
	GAME * pGame,
	UNIT *pUnit,
	EXCELTABLE eTable,
	int nClass,
	int nModelId)
{
	const UNIT_DATA * pUnitDataForModel = (UNIT_DATA *) ExcelGetData( NULL, eTable, nClass );
	if ( ! pUnitDataForModel )
		return;
	if( UnitDataTestFlag( pUnitDataForModel, UNIT_DATA_FLAG_USE_SOURCE_APPEARANCE ) && pUnit && pUnit->pSourceAppearanceUnitData)
	{
		pUnitDataForModel = pUnit->pSourceAppearanceUnitData;
		ASSERT_RETURN(pUnitDataForModel);
	}

	BOOL bRegisterEvent = FALSE;

	int nModelDef = e_ModelGetDefinition( nModelId );

	if ( nModelDef != INVALID_ID )
	{
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			if ( e_ModelDefinitionExists( nModelDef, nLOD )
			  && e_ModelDefinitionGetMeshCount(nModelDef, nLOD ) == 0)
			{
				bRegisterEvent = TRUE;
				break;
			}
		}
	}
	else 
	{
		if( pUnit && pUnit->nAppearanceDefId != INVALID_ID )
		{
			APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitDataForModel->nAppearanceDefId );
			// we don't want to register a delayed event for things with no actual models -
			// like warps, or what have you - otherwise they'll delay forever!
			if( !pAppearanceDef || PStrLen( pAppearanceDef->pszModel ) != 0 )
			{
				bRegisterEvent = TRUE;
			}
		}
	}

	APPEARANCE_DEFINITION * pAppearanceDef = NULL;
	if ( !bRegisterEvent && pUnitDataForModel->nAppearanceDefId != INVALID_ID )
	{
		pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitDataForModel->nAppearanceDefId );
		if ( ! pAppearanceDef )
		{
			bRegisterEvent = TRUE;
		}
	}


	if ( ! bRegisterEvent )
	{
		c_sUnitDataLoadTextures( pAppearanceDef, eTable, nClass ); // just in case any of the textures have been removed since UNIT_DATA does not refcount them
		for ( int i = 0; i < MAX_UNIT_TEXTURE_OVERRIDES; i++ )
		{
			// CHB 2006.11.28
			ASSERTV( pUnitDataForModel->pnTextureSpecificOverrideIds[ i ] != 0, "Erroneous zero texture ID encountered in unit data overrides!\n\n%s", pUnitDataForModel->szName );
			if ( pUnitDataForModel->pnTextureSpecificOverrideIds[ i ] != INVALID_ID)
			{
				if( e_ModelSetTextureOverride( nModelId, pUnitDataForModel->szTextureSpecificOverrides[i * 2 ], TEXTURE_DIFFUSE, pUnitDataForModel->pnTextureSpecificOverrideIds[ i ]) != S_OK )
				{
					bRegisterEvent = TRUE;
				}
			}
			else if ( PStrLen( pUnitDataForModel->szTextureSpecificOverrides[ i * 2 ] ) != 0)
			{
				bRegisterEvent = TRUE;
			}
		}
		for ( int i = 0; i < UNIT_TEXTURE_OVERRIDE_COUNT; i++ )
		{
			// CHB 2006.11.28
			TEXTURE_TYPE eTextureType = gpeUnitOverrideToTextureType[ i ];
			ASSERTV( pUnitDataForModel->pnTextureOverrideIds[ i ][ LOD_HIGH ] != 0, "Erroneous zero texture ID encountered in unit data overrides!\n\n%s", pUnitDataForModel->szName );
			if ( pUnitDataForModel->pnTextureOverrideIds[ i ][ LOD_HIGH ] != INVALID_ID)
			{
				e_ModelSetTextureOverrideLOD(nModelId, eTextureType, pUnitDataForModel->pnTextureOverrideIds[ i ]);
			}
			else if ( pUnitDataForModel->szTextureOverrides[ i ][ 0 ] != 0 &&
				e_TextureSlotIsUsed(eTextureType))
			{
				bRegisterEvent = TRUE;
			}
		}
	}

	if ( bRegisterEvent )
	{
		// there is a texture specified, but it hasn't been loaded yet, come back here in a bit and try again
		EVENT_DATA tEventData( nModelId, eTable, nClass );
		// I'm wanting to do this a ton faster for Mythos - I hate these 1-second delayed texture loads, it looks crappy.

		if(pUnit)
		{
			UnitRegisterEventTimed(pUnit, sDelayedModelGFXInit, tEventData, 1);
		}
		else
		{
			GameEventRegister(pGame, sDelayedModelGFXInit, pUnit, NULL, tEventData, 1);
		}
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITGFX * sUnitDataGfxInit(
	GAME * game,
	const UNIT_DATA * pUnitData,
	int nUnitClass,
	UNIT * pUnit,
	WARDROBE_INIT * pWardrobeInit,
	int nUnitAppearanceDefId,
	APPEARANCE_SHAPE * pUnitAppearanceShape,
	BOOL bUnitIsClientOnly,
	BOOL bIsControlUnit,
	GENUS eUnitGenus,
	float fScale)
{
	ASSERT_RETNULL(pUnitData);

	BOOL bMakeWardrobe = FALSE;

	if ( IS_CLIENT(game) && ( bUnitIsClientOnly && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_WARDROBE_PER_UNIT) ) )
		bMakeWardrobe = TRUE;

	if ( IS_SERVER(game) && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_WARDROBE_PER_UNIT) )
		bMakeWardrobe = TRUE;

	if ( IS_CLIENT(game) && c_GetToolMode() )
		bMakeWardrobe = !! ( pWardrobeInit );

	int nWardrobeFallback = INVALID_ID;
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( game ) )
	{
		int nWardrobeLimit = e_OptionState_GetActive()->get_nWardrobeLimit();
		if (   nWardrobeLimit > 0 && c_WardrobeGetCount() > nWardrobeLimit 
			|| UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK ) )
		{
			if ( eUnitGenus == GENUS_MONSTER || eUnitGenus == GENUS_PLAYER )
			{
				if ( pUnitData->nWardrobeFallback != INVALID_LINK )
				{
					const struct UNIT_DATA* pFallbackData = MonsterGetData( game, pUnitData->nWardrobeFallback );
					ASSERT( pFallbackData );

					if ( pFallbackData )
					{
						if ( ! WardrobeExists( game, pFallbackData->nWardrobeFallbackId ) )
						{
							if ( UnitDataLoad( game, DATATABLE_MONSTERS, pUnitData->nWardrobeFallback ) )
							{
								BOOL bWardrobeChanged = FALSE;
								int nWardrobe = WardrobeAllocate( game, pFallbackData, NULL, 
									INVALID_ID, bWardrobeChanged );
								ASSERT( nWardrobe != INVALID_ID );

								if ( nWardrobe != INVALID_ID )
								{
									const_cast<UNIT_DATA*>(pFallbackData)->nWardrobeFallbackId = nWardrobe;
									WardrobeSetMask( game, nWardrobe, 
										WARDROBE_FLAG_FORCE_USE_BODY 
										| WARDROBE_FLAG_WARDROBE_FALLBACK );
									c_WardrobeSetColorSetIndex( nWardrobe, pFallbackData->nColorSet, pFallbackData->nUnitType, TRUE );
								}
							}
						}

						nWardrobeFallback = pFallbackData->nWardrobeFallbackId;
					}
				}
			}
		}
	}
#endif

	int nUnitWardrobe = INVALID_ID;
	if ( bMakeWardrobe )
	{
		BOOL bWardrobeChanged = FALSE;
		int nWardrobe = WardrobeAllocate( game, pUnitData, pWardrobeInit, 
			nWardrobeFallback, bWardrobeChanged );
		ASSERT( nWardrobe != INVALID_ID );
		nUnitWardrobe = nWardrobe;
		if(pUnit)
		{
			pUnit->nWardrobe = nWardrobe;

			if ( bWardrobeChanged )
				UnitSetWardrobeChanged(pUnit, TRUE);	

			if ( IS_SERVER(game) )
				WardrobeUpdateFromInventory ( pUnit );
		}
	}

	if(pUnit && nUnitWardrobe != INVALID_ID)
	{
		nUnitWardrobe = pUnit->nWardrobe;
	}

	UNITGFX* pGfx = NULL;
	if (IS_CLIENT(game))
	{
		pGfx = (UNITGFX*)GMALLOCZ(game, sizeof(UNITGFX));
		if (!pGfx)
		{
			return NULL;
		}
		if(pUnit)
		{
			pUnit->pGfx = pGfx;
		}

		DWORD dwAppearanceNewFlags = UnitGetAppearanceNewFlags( pUnitData );

		if ( bIsControlUnit )
			SET_MASK( dwAppearanceNewFlags, APPEARANCE_NEW_UNIT_IS_CONTROL_UNIT );

		if ( nUnitWardrobe != INVALID_ID )
			dwAppearanceNewFlags |= APPEARANCE_NEW_DONT_FREE_WARDROBE;

		int nStartingStance = pUnitData->nStartingStance;

		pGfx->nModelIdThirdPerson = c_AppearanceNew( nUnitAppearanceDefId, dwAppearanceNewFlags, 
			pUnit, pUnitData, nUnitWardrobe, nStartingStance, pUnitAppearanceShape,
			nWardrobeFallback );
		pGfx->nModelIdCurrent = pGfx->nModelIdThirdPerson;
		pGfx->nModelIdInventory   = INVALID_ID;
		pGfx->nModelIdInventoryInspect   = INVALID_ID;
		pGfx->nModelIdFirstPerson = INVALID_ID;
		pGfx->nModelIdPaperdoll   = INVALID_ID;
		pGfx->nModelIdThirdPersonOverride = INVALID_ID;
		pGfx->nAppearanceDefIdForCamera = INVALID_ID;

		float fModelScale = pUnitData->fScaleMultiplier * fScale;
		e_ModelSetScale( pGfx->nModelIdThirdPerson, fModelScale );

		int nModelId = pUnit ? c_UnitGetModelId( pUnit ) : pGfx->nModelIdCurrent;
		UnitModelGfxInit( game, pUnit, ExcelGetDatatableByGenus( eUnitGenus ), nUnitClass, nModelId );
		if(pUnit)
		{
			c_UnitUpdateGfx( pUnit );
		}
		if( AppIsTugboat() )
		{
			e_ModelSetDrawTemporary( nModelId, FALSE );
			if(pUnit)
			{
				pUnit->bVisible = FALSE;
			}
		}
	} 

	return pGfx;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITGFX * UnitDataGfxInit(
	const UNIT_DATA * pUnitData,
	int nUnitDataClass,
	WARDROBE_INIT * pWardrobeInit,
	BOOL bUnitIsClientOnly,
	BOOL bIsControlUnit,
	GENUS eUnitGenus,
	float fScale)
{
	return sUnitDataGfxInit(AppGetCltGame(), pUnitData, nUnitDataClass, NULL, pWardrobeInit, pUnitData->nAppearanceDefId, NULL, bUnitIsClientOnly, bIsControlUnit, eUnitGenus, fScale);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline UNITGFX* UnitGfxInit(
	UNIT* pUnit,
	UNIT_CREATE* pUnitCreate)
{
	ASSERT_RETNULL(pUnit);
	GAME *game = UnitGetGame(pUnit);
	ASSERT_RETNULL(game);

	const UNIT_DATA * pUnitData = UnitGetData( pUnit );

	if ( pUnitCreate->pAppearanceShape )
	{
		pUnit->pAppearanceShape = (APPEARANCE_SHAPE *) GMALLOC( game, sizeof( APPEARANCE_SHAPE ) );
		MemoryCopy( pUnit->pAppearanceShape, sizeof( APPEARANCE_SHAPE ), pUnitCreate->pAppearanceShape, sizeof( APPEARANCE_SHAPE ) );
	}  
	else if ( pUnitData->tAppearanceShapeCreate.bHasShape ) 
	{
		pUnit->pAppearanceShape = (APPEARANCE_SHAPE *) GMALLOC( game, sizeof( APPEARANCE_SHAPE ) );
		if (pUnitCreate->nQuality == INVALID_LINK)
		{
			if ( TESTBIT( pUnitData->pdwFlags, UNIT_DATA_FLAG_SET_SHAPE_PERCENTAGES ) )
				UnitSetShape( pUnit, pUnitData->fHeightPercent / 100.0f, pUnitData->fWeightPercent / 100.0f );
			else
				AppearanceShapeRandomize( pUnitData->tAppearanceShapeCreate, *pUnit->pAppearanceShape );
		}
		else
		{
			AppearanceShapeRandomizeByQuality( pUnit, pUnitCreate->nQuality, pUnitData->tAppearanceShapeCreate );
		}
	}

	if ( pUnit->pAppearanceShape )
	{
		UnitSetStatFloat(pUnit, STATS_UNIT_HEIGHT_ORIGINAL, 0, (float)pUnit->pAppearanceShape->bHeight / 255.0f );
		UnitSetStatFloat(pUnit, STATS_UNIT_WEIGHT_ORIGINAL, 0, (float)pUnit->pAppearanceShape->bWeight / 255.0f );
	}

	return sUnitDataGfxInit(game, pUnitData, UnitGetClass(pUnit), pUnit, pUnitCreate->pWardrobeInit, pUnit->nAppearanceDefId, pUnit->pAppearanceShape, UnitTestFlag( pUnit, UNITFLAG_CLIENT_ONLY ), TEST_MASK( pUnitCreate->dwUnitCreateFlags, UCF_IS_CONTROL_UNIT ), UnitGetGenus(pUnit), pUnitCreate->fScale);
}

//----------------------------------------------------------------------------
BOOL c_UnitCreatePaperdollModel(
	UNIT *pUnit,
	DWORD dwFlags)
{
	if (!pUnit)
		return FALSE;

	if (pUnit->pGfx->nModelIdPaperdoll == INVALID_ID)
	{
		int nWardrobeThird = c_AppearanceGetWardrobe(c_UnitGetModelIdThirdPerson(pUnit));
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		ASSERT_RETFALSE (pUnitData);
		if ( nWardrobeThird != INVALID_ID )
			dwFlags |= APPEARANCE_NEW_DONT_FREE_WARDROBE;
		pUnit->pGfx->nModelIdPaperdoll = c_AppearanceNew(pUnitData->nAppearanceDefId, 
			dwFlags, pUnit, pUnitData, nWardrobeThird, STANCE_DEFAULT, pUnit->pAppearanceShape );
		c_AnimationPlayByMode(pUnit, UnitGetGame(pUnit), pUnit->pGfx->nModelIdPaperdoll, 
			MODE_IDLE_PAPERDOLL, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS);
		V( e_ModelSetFlagbit( pUnit->pGfx->nModelIdPaperdoll, MODEL_FLAGBIT_NOSHADOW, TRUE ) );
		V( e_ModelSetDrawLayer( pUnit->pGfx->nModelIdPaperdoll, DRAW_LAYER_UI ) );
		UnitModelGfxInit( UnitGetGame(pUnit), pUnit, UnitGetDatatableEnum( pUnit ), UnitGetClass( pUnit ), pUnit->pGfx->nModelIdPaperdoll );
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGfxInventoryInit(
	UNIT* unit,
	BOOL bCreateWardrobe)
{
	ASSERT( unit );
	if ( !unit->pGfx )
		return;
	ASSERT_RETURN( (!bCreateWardrobe && unit->pGfx->nModelIdInventory == INVALID_ID) || (bCreateWardrobe && unit->pGfx->nModelIdInventoryInspect == INVALID_ID) );

	int nWardrobe = INVALID_ID;
	GAME* pGame = UnitGetGame( unit );
	const UNIT_DATA * pUnitData = UnitGetData( unit );
	int nWardrobeLink = INVALID_LINK;
	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_DRAW_USING_CUT_UP_WARDROBE))
	{
		nWardrobeLink = ItemGetWardrobe( pGame, UnitGetClass( unit ), 
						UnitGetLookGroup( unit ) );
	}

	int nAppearanceDefId = INVALID_ID;
	if ( AppIsHellgate() && nWardrobeLink != INVALID_LINK && bCreateWardrobe )
	{
		UNIT* pUltimateOwner = UnitGetUltimateOwner(unit);
		UNIT* pPlayer = GameGetControlUnit( pGame );
		
		// merchants need to have their inventories viewable by the player.
		//  we're also going to need to do this for other players once we enable.
		//  trading between players.  We may want to generalize this more with a 
		//  unit flag or something.		
		if ( pUltimateOwner && pPlayer ) //&& pUltimateOwner == pPlayer || UnitIsMerchant(pUltimateOwner) )
		{
			const UNIT_DATA * pItemData = UnitGetData( unit );
			//const UNIT_DATA * pOwnerData = UnitGetData( pUltimateOwner );
			const UNIT_DATA * pPlayerData = UnitGetData( pPlayer ); // pOwnerData;

			int nWardrobeAppearanceDefId = pPlayerData->nAppearanceDefId;	// this will always be based on the control unit
			if ( UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_DONT_USE_CONTAINER_APPEARANCE ) )
				nWardrobeAppearanceDefId = pItemData->nAppearanceDefId;

			if ( pItemData->nContainerUnitTypes[ 0 ] != INVALID_ID &&
				 ! UnitIsA( pPlayer, pItemData->nContainerUnitTypes[ 0 ] ) )
			{
				GENDER eGender = UnitGetGender( pPlayer );
				int nRace = UnitGetRace( pPlayer );
				const UNIT_DATA* pItemData = UnitGetData( unit );

				int nCharacterClassCount = CharacterClassNumClasses();
				for ( int i = 0; i < nCharacterClassCount; i++ )
				{
					int nPlayerClass = CharacterClassGetPlayerClass( i, nRace, eGender );
					if(nPlayerClass == INVALID_ID)
						continue;

					const UNIT_DATA * pTestData = PlayerGetData( pGame, nPlayerClass );

					if ( pTestData && UnitIsA( pTestData->nUnitType, pItemData->nContainerUnitTypes[ 0 ] ) )
					{
						pPlayerData = pTestData;
						break;
					}
				}
			}

			int nWardrobeAppearanceGroup = pPlayerData->pnWardrobeAppearanceGroup[ 
				UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON ];

#define DEFAULT_INVENTORY_MIP_BONUS		1
			WARDROBE_INIT tWardrobeInit( nWardrobeAppearanceGroup, nWardrobeAppearanceDefId, pPlayer ? UnitGetType( pPlayer ) : UnitGetType( pUltimateOwner ) );
			tWardrobeInit.ePriority = WARDROBE_PRIORITY_INVENTORY;
			if ( e_ModelBudgetGetMaxLOD( MODEL_GROUP_WARDROBE ) > LOD_LOW )
				tWardrobeInit.nMipBonus = DEFAULT_INVENTORY_MIP_BONUS;
			
			nWardrobe = WardrobeInit( pGame, tWardrobeInit );
			ASSERT_RETURN( nWardrobe != INVALID_ID );

			// We ignore the appearance def's layers because we don't want the 
			// full body (i.e. we only want the specific item to be wardrobed).
			WardrobeSetMask( pGame, nWardrobe,
				  WARDROBE_FLAG_IGNORE_APPEARANCE_DEF_LAYERS
				| WARDROBE_FLAG_DOES_NOT_NEED_SERVER_UPDATE
				| WARDROBE_FLAG_DONT_APPLY_BODY_COLORS );

			int nColorSetPriority = -1;
			WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
			ASSERT_RETURN( pClient );
			COSTUME& tCostume = pClient->pCostumes[ 0 ];
			c_WardrobeSetBaseLayer( nWardrobe, nWardrobeLink );
			WardrobeGetItemColorSetIndexAndPriority( pGame, unit, 
				tCostume.nColorSetIndex, nColorSetPriority );		

			// copy the player's body appearance groups to help with finding wardrobe parts
			{
				int nPlayerWardrobe = UnitGetWardrobe( pUltimateOwner );
//				ASSERT( nPlayerWardrobe != INVALID_ID );
				if ( nPlayerWardrobe != INVALID_ID )
				{
					WARDROBE_BODY* pBody = c_WardrobeGetBody( nWardrobe );
					WARDROBE_BODY* pPlayerBody = c_WardrobeGetBody( nPlayerWardrobe );

					if ( pBody && pPlayerBody )
					{
						MemoryCopy( pBody->pnAppearanceGroups, 
							sizeof(int) * NUM_WARDROBE_BODY_SECTIONS, 
							pPlayerBody->pnAppearanceGroups, 
							sizeof(int) * NUM_WARDROBE_BODY_SECTIONS );
					}
				}
			}
			
			nAppearanceDefId = nWardrobeAppearanceDefId;
			unit->pGfx->nAppearanceDefIdForCamera = UnitGetAppearanceDefId( unit, TRUE );
		}
	}
	else
		nAppearanceDefId = UnitGetAppearanceDefId( unit, TRUE );

	DWORD dwAppearanceNewFlags = UnitGetAppearanceNewFlags( pUnitData );
	dwAppearanceNewFlags |= APPEARANCE_NEW_FORCE_ANIMATE;

	// don't set the APPEARANCE_NEW_DONT_FREE_WARDROBE flag - we need the appearance to clear it
	int nModelId = c_AppearanceNew( nAppearanceDefId, 
		dwAppearanceNewFlags, unit, pUnitData, nWardrobe );
	ASSERT(nModelId != INVALID_ID);
	if(bCreateWardrobe)
	{
		unit->pGfx->nModelIdInventoryInspect = nModelId;
		e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_TWO_SIDED, TRUE ); // render backfaces of wardrobed inventory models
	}
	else
	{
		unit->pGfx->nModelIdInventory = nModelId;
	}

	//e_ModelSetFlags( unit->pGfx->nModelIdInventory, 
	//	/*MODEL_FLAG_NODRAW |*/ MODEL_FLAG_MUTESOUND | MODEL_FLAG_NOLIGHTS, TRUE ); // specially ignored at render time

	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND, TRUE ); // specially ignored at render time
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NOLIGHTS,  TRUE ); // specially ignored at render time

	V( e_ModelSetDrawLayer( nModelId, DRAW_LAYER_UI ) );

	// Set the "item in inventory" mode for wardrobed items, so the item gets
	// posed correctly.
	if ( nWardrobeLink != INVALID_LINK )
		c_UnitSetMode( unit, MODE_ITEM_IN_INVENTORY );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGfxInventoryFree(
	UNIT* unit)
{
	ASSERT( unit );
	if (!unit->pGfx)
		return;
	if ( unit->pGfx->nModelIdInventory != INVALID_ID )
	{
		c_AppearanceDestroy( unit->pGfx->nModelIdInventory );
		unit->pGfx->nModelIdInventory = INVALID_ID;
	}
	if ( unit->pGfx->nModelIdInventoryInspect != INVALID_ID )
	{
		c_AppearanceDestroy( unit->pGfx->nModelIdInventoryInspect );
		unit->pGfx->nModelIdInventoryInspect = INVALID_ID;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGfxInventoryInspectFree(
	UNIT* unit)
{
	ASSERT( unit );
	if (!unit->pGfx)
		return;
	if ( unit->pGfx->nModelIdInventoryInspect != INVALID_ID )
	{
		c_AppearanceDestroy( unit->pGfx->nModelIdInventoryInspect );
		unit->pGfx->nModelIdInventoryInspect = INVALID_ID;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGfxFree(
	UNIT* unit)
{
	if (!unit->pGfx)
	{
		return;
	}

	GAME* game = UnitGetGame(unit);

	if (IS_CLIENT(game))
	{
		UnitGfxInventoryFree( unit );
		int nModelId = c_UnitGetModelIdThirdPerson( unit );
		if ( nModelId != INVALID_ID )
			c_AppearanceDestroy( nModelId );

		nModelId = c_UnitGetModelIdFirstPerson( unit );
		if ( nModelId != INVALID_ID )
			c_AppearanceDestroy( nModelId );

		nModelId = c_UnitGetModelIdInventory( unit );
		if ( nModelId != INVALID_ID )
			c_AppearanceDestroy( nModelId );

		nModelId = c_UnitGetPaperdollModelId( unit );
		if ( nModelId != INVALID_ID )
			c_AppearanceDestroy( nModelId );

		nModelId = unit->pGfx->nModelIdThirdPersonOverride;
		if ( nModelId != INVALID_ID )
			c_AppearanceDestroy( nModelId );
	}

	GFREE(game, unit->pGfx);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UnitHashAdd(
	UNIT_HASH * uh,
	UNIT * unit)
{
	int hash = MAKE_UNIT_HASH(unit->unitid);

	if (uh->pHash[hash])
	{
		uh->pHash[hash]->hashprev = unit;
	}
	unit->hashnext = uh->pHash[hash];
	uh->pHash[hash] = unit;

	uh->nCount++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UnitHashRemove(
	GAME * game,
	UNIT * unit)
{
	if (unit->hashprev)
	{
		unit->hashprev->hashnext = unit->hashnext;
	}
	else
	{
		game->pUnitHash->pHash[MAKE_UNIT_HASH(unit->unitid)] = unit->hashnext;
	}
	if (unit->hashnext)
	{
		unit->hashnext->hashprev = unit->hashprev;
	}
	unit->hashprev = NULL;
	unit->hashnext = NULL;
	game->pUnitHash->nCount--;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetById(
	GAME * game,
	UNITID unitid)
{
	ASSERT_RETNULL(game);

	UNIT_HASH * uh = game->pUnitHash;
	ASSERT_RETNULL(uh);

	int hash = MAKE_UNIT_HASH(unitid);

	UNIT * unit = uh->pHash[hash];
	while (unit)
	{
		if (unit->unitid == unitid)
		{
			return unit;
		}
		unit = unit->hashnext;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL GuidHashValidate(
	UNIT_HASH * uh,
	unsigned int hash)
{
#if ISVERSION(DEVELOPMENT)
	UNIT * unit = uh->pGuidHash[hash];
	while (unit)
	{
		if (unit->guidprev)
		{
			ASSERT_RETFALSE(unit->guidprev->guidnext == unit);
		}
		else
		{
			ASSERT_RETFALSE(uh->pGuidHash[hash] == unit);
		}
		if (unit->guidnext)
		{
			ASSERT_RETFALSE(unit->guidnext->guidprev == unit);
		}
		unit = unit->guidnext;
	}
	return TRUE;
#else
	return TRUE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL GuidHashAdd(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);

	UNIT_HASH * uh = game->pUnitHash;
	ASSERT_RETFALSE(uh);
#if VALIDATE_UNIQUE_GUIDS
	if (game->pServerContext && game->pServerContext->m_GuidTracker)
	{
		ASSERT_DO(
			game->pServerContext->m_GuidTracker->
			Increment(UnitGetGuid(unit), GameGetId(game), UnitGetId(unit)))
		{
			game->pServerContext->m_GuidTracker->
				Decrement(UnitGetGuid(unit) );
			return FALSE;
		}
	}
#endif

	unsigned int hash = MAKE_GUID_HASH(UnitGetGuid(unit));

	ASSERT(GuidHashValidate(uh, hash));

	if (uh->pGuidHash[hash])
	{
		uh->pGuidHash[hash]->guidprev = unit;
	}
	unit->guidnext = uh->pGuidHash[hash];
	uh->pGuidHash[hash] = unit;

	ASSERT(GuidHashValidate(uh, hash));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void GuidHashRemove(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	if (UnitGetGuid(unit) == INVALID_GUID)
	{
		ASSERT(unit->guidprev == NULL && unit->guidnext == NULL);
		return;
	}

	UNIT_HASH * uh = game->pUnitHash;
	ASSERT_RETURN(uh);

	unsigned int hash = MAKE_GUID_HASH(UnitGetGuid(unit));

	ASSERT(GuidHashValidate(uh, hash));

	if (unit->guidprev)
	{
		unit->guidprev->guidnext = unit->guidnext;
	}
	else
	{
		uh->pGuidHash[hash] = unit->guidnext;
	}
	if (unit->guidnext)
	{
		unit->guidnext->guidprev = unit->guidprev;
	}
	unit->guidprev = NULL;
	unit->guidnext = NULL;

	ASSERT(GuidHashValidate(uh, hash));

#if VALIDATE_UNIQUE_GUIDS
	if (game->pServerContext && game->pServerContext->m_GuidTracker)
	{
		game->pServerContext->m_GuidTracker->Decrement(UnitGetGuid(unit));
	}
#endif

	unit->guid = INVALID_GUID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetByGuid(
	GAME * game,
	PGUID guid)
{
	ASSERT_RETNULL(game);

	UNIT_HASH * uh = game->pUnitHash;
	ASSERT_RETNULL(uh);

	int hash = MAKE_GUID_HASH(guid);

	UNIT * unit = uh->pGuidHash[hash];
	while (unit)
	{
		if (UnitGetGuid(unit) == guid)
		{
			return unit;
		}
		unit = unit->guidnext;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitSetGuid(
	UNIT * unit,
	PGUID guid)
{
	ASSERT_RETFALSE(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	if (UnitGetGuid(unit) == guid)
	{
		return TRUE;
	}
	if (UnitGetGuid(unit) != INVALID_GUID)
	{
		GuidHashRemove(game, unit);
		unit->guid = INVALID_GUID; //Don't leave a unit with a removed guid for below error cases.
	}
	if (guid == INVALID_GUID)
	{
		unit->guid = INVALID_GUID;
		return TRUE;
	}
	ASSERT_RETFALSE(unit->guidprev == NULL && unit->guidnext == NULL);
/*
	trace("[%s]  UnitSetGuid() [%d: %s]  OLD:%I64d  NEW:%I64d\n",
		IS_SERVER(unit) ? "SRV" : "CLT", UnitGetId(unit), UnitGetDevName(unit), UnitGetGuid(unit), guid);
*/
#if VALIDATE_UNIQUE_GUIDS
	UNIT * test = UnitGetByGuid(game, guid);
	ASSERT_RETFALSE(test == NULL || test == unit);

	if (test == NULL)
#endif
	{
		unit->guid = guid;

		ASSERT_DO(GuidHashAdd(UnitGetGame(unit), unit))
		{
			unit->guid = INVALID_GUID;
			return FALSE;
		}
	}
	return TRUE;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitIterateUnits(
	GAME *pGame,
	PFN_UNIT_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pfnCallback, "Expected unit iterate callback" );
	
	// yeah, this is slow - don't do it too much m'kay!
	for (int i = 0; i < UNIT_HASH_SIZE; ++i)
	{
		UNIT *pUnit = pGame->pUnitHash->pHash[ i ];
		UNIT *pNext;
		while (pUnit)
		{
		
			// get next in case the callback deletes it or something
			pNext = pUnit->hashnext;
			
			// do the callback
			UNIT_ITERATE_RESULT eResult = pfnCallback( pUnit, pCallbackData );
			if (eResult == UIR_STOP)
			{
				return;
			}
			
			// next unit
			pUnit = pNext;
			
		}
		
	}

}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitRecreateIcons(
	GAME *pGame )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Client Only" );
	
	// yeah, this is slow - don't do it too much m'kay!
	for (int i = 0; i < UNIT_HASH_SIZE; ++i)
	{
		UNIT *pUnit = pGame->pUnitHash->pHash[ i ];
		UNIT *pNext;
		while (pUnit)
		{
		
			// get next in case the callback deletes it or something
			pNext = pUnit->hashnext;
			
			UnitCreateIconTexture( pGame, pUnit );

			// next unit
			pUnit = pNext;
			
		}	
	}
#endif
}
	

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearIcons(
	GAME *pGame )
{

	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Client Only" );
	
	// yeah, this is slow - don't do it too much m'kay!
	for (int i = 0; i < UNIT_HASH_SIZE; ++i)
	{
		UNIT *pUnit = pGame->pUnitHash->pHash[ i ];
		UNIT *pNext;
		while (pUnit)
		{
		
			// get next in case the callback deletes it or something
			pNext = pUnit->hashnext;
			
			pUnit->pIconTexture = NULL;

			// next unit
			pUnit = pNext;
			
		}		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *UnitGetFirstByClassSearchAll(
	GAME * pGame,
	GENUS eGenus,	
	int nUnitClass)
{

	// yes!  this searches *EVERY* unit in the game ... use this function only
	// at times when performance won't be impacted!  Also note, that if there is
	// more than one unit matching genus and class, the first one found is
	// returned, as the name implies ... this function is useful for quests
	// and finding specific unit instances in the game

	// note by phu - this is totally sucky
	// note by colin - yeah
	for (int i = 0; i < UNIT_HASH_SIZE; ++i)
	{
		UNIT *pUnit = pGame->pUnitHash->pHash[ i ];
		while (pUnit)
		{
			if (UnitGetGenus( pUnit ) == eGenus && UnitGetClass( pUnit ) == nUnitClass)
			{
				return pUnit;
			}
			pUnit = pUnit->hashnext;
		}
		
	}

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct MEMORY_THREADPOOL_HELPER
{
	GAME * game;
	MEMORY_THREADPOOL_HELPER(
		GAME * in_game) : game(NULL)
	{
		if (in_game && IS_SERVER(in_game))
		{
			game = in_game;
			MemorySetThreadPool(NULL);
		}
	}
	~MEMORY_THREADPOOL_HELPER(
		void)
	{
		if (game)
		{
			MemorySetThreadPool(GameGetMemPool(game));
		}
	}
};

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef HAVOK_ENABLED
static void sUnitDataInitHavokShape(
	GAME * game,
	UNIT_DATA * unit_data)
{
	if (unit_data->tHavokShapePrefered.pHavokShape)
	{
		return;
	}

	BOOL bAppearanceLoaded = unit_data->nAppearanceDefId != INVALID_ID && DefinitionGetById(DEFINITION_GROUP_APPEARANCE, unit_data->nAppearanceDefId);
	if (!bAppearanceLoaded && unit_data->tHavokShapeFallback.pHavokShape)
	{
		return; // we already have a fallback, and we can't make what we want
	}

	switch (unit_data->nHavokShape) 
	{
	case HAVOK_SHAPE_SPHERE:
		if (unit_data->fCollisionRadius >= 0.0f)
		{
			float flRadius = unit_data->fCollisionRadius * unit_data->fScale;
			MEMORY_THREADPOOL_HELPER mth(game);
			HavokCreateShapeSphere( unit_data->tHavokShapePrefered, flRadius);
		}
		break;

	case HAVOK_SHAPE_BOX:
		{
			VECTOR vBoxSize = AppearanceDefinitionGetBoundingBoxSize(unit_data->nAppearanceDefId);
			// maybe adjust by at least pUnitData->fScale here? -Colin
			if (bAppearanceLoaded)
			{
				ASSERT_RETURN(!unit_data->tHavokShapePrefered.pHavokShape);
				MEMORY_THREADPOOL_HELPER mth(game);
				HavokCreateShapeBox( unit_data->tHavokShapePrefered, vBoxSize);
			} 
			else 
			{
				ASSERT_RETURN(!unit_data->tHavokShapeFallback.pHavokShape);
				MEMORY_THREADPOOL_HELPER mth(game);
				HavokCreateShapeBox( unit_data->tHavokShapeFallback, vBoxSize);
			}
		}
		break;
	}
}
#endif


//----------------------------------------------------------------------------
// note this is only being used by quests!!!
//----------------------------------------------------------------------------
#define MONITOR_PLAYER_APPROACH_RATE (GAME_TICKS_PER_SECOND / 2)
static BOOL sMonitorPlayerApproach(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));

	if (UnitCheckCanRunEvent(unit) == FALSE)
	{
		UnitSetFlag(unit, UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE);
		return FALSE;
	}

	LEVEL * level = UnitGetLevel(unit);
	if (!level)
	{
		return FALSE;
	}

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);

	float flMonitorDistSq = unit_data->flMonitorApproachRadius * unit_data->flMonitorApproachRadius;

	// this is too much with a large # of clients, if we have that, we need to make
	// this a list of clients on the level or room or something smaller
	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		UNIT * player = ClientGetControlUnit(client);
		if (!player)
		{
			continue;
		}
		LEVEL * levelPlayer = UnitGetLevel(player);
		
		// only pay attention to those on the same level
		if (level != levelPlayer)
		{
			continue;
		}

		float flDistSq = UnitsGetDistanceSquared(unit, player);
		if (flDistSq <= flMonitorDistSq)
		{
			// check to see if we must have clear LOS
			BOOL bApproached = TRUE;
			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_MONITOR_APPROACH_CLEAR_LOS))
			{
				if (ClearLineOfSightBetweenUnits(unit, player) == FALSE)
				{
					bApproached = FALSE;
				}
			}
			
			if (bApproached)
			{
				s_QuestEventApproachPlayer(player, unit);
			}
		}
	}
	
	UnitRegisterEventTimed(unit, sMonitorPlayerApproach, NULL, MONITOR_PLAYER_APPROACH_RATE);

	return TRUE;
}


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitSetState(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit, 
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	int nState = (int)((EVENT_DATA*)data)->m_Data1;	
	BOOL bHadState = (BOOL)((EVENT_DATA*)data)->m_Data3;	
	switch (nState)
	{
	case STATE_PLAYER_LIMBO:
		if ( ! bHadState )
		{
			UnitSetFlag( unit, UNITFLAG_IN_LIMBO, TRUE );
			UnitEventEnterLimbo( game, unit );
			if(unit->m_pPathing)
			{
				UnitErasePathNodeOccupied(unit);
			}
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitClearState(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit, 
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	int nState = (int)((EVENT_DATA*)data)->m_Data1;	
	switch (nState)
	{
	case STATE_PLAYER_LIMBO:
		UnitClearFlag( unit, UNITFLAG_IN_LIMBO );
		UnitEventExitLimbo( game, unit );
		StatsExitLimboCallback( game, unit );
		break;
	}
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitCreateRigidBody(
	UNIT * unit )
{
#ifdef HAVOK_ENABLED
	const UNIT_DATA * pUnitData = UnitGetData( unit );

	LEVEL* pLevel = UnitGetLevel( unit );
	if ( ! pLevel || unit->pHavokRigidBody )
	{
		return;
	}

	hkShape * pShape = pUnitData->tHavokShapePrefered.pHavokShape ? pUnitData->tHavokShapePrefered.pHavokShape : pUnitData->tHavokShapeFallback.pHavokShape;

	if ( pShape )
	{
		GAME * game = UnitGetGame( unit );
		LEVEL* pLevel = UnitGetLevel( unit );
		unit->pHavokRigidBody = HavokCreateUnitRigidBody(game, pLevel ? pLevel->m_pHavokWorld : NULL, 
			pShape, c_UnitGetPosition(unit), unit->vFaceDirection, unit->vUpDirection, unit->pUnitData->fBounce, 
			unit->pUnitData->fDampening, unit->pUnitData->fFriction);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitCreateIconTexture( GAME* game,
							UNIT* unit )
{
	unit->pIconTexture = NULL;
#if !ISVERSION(SERVER_VERSION)
	// client units may have an inventory icon
	if( IS_CLIENT(game) )
	{
		if( PStrLen( ( (UNIT_DATA *)unit->pUnitData )->szIcon )  != 0 )
		{

		
			char szFullName[MAX_PATH];
			PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
				FILE_PATH_ICONS,
				( (UNIT_DATA *)unit->pUnitData )->szIcon,
				".png" );
			UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);
			// old texture we dumped - let's fill it in again
			if( texture && texture->m_nTextureId == INVALID_ID )
			{
				UIX_TEXTURE* pTempTexture = UITextureLoadRaw( szFullName, 1280, 1024, TRUE );
				texture->m_nTextureId = pTempTexture->m_nTextureId;
				FREE( NULL, pTempTexture );
			}
			else if (!texture)
			{
				texture = UITextureLoadRaw( szFullName, 1280, 1024, TRUE );
				if( texture )
				{
					StrDictionaryAdd( g_UI.m_pTextures, szFullName, texture);
				}
			}
			else if(texture)
			{
				e_TextureAddRef( texture->m_nTextureId );
			}
			unit->pIconTexture = texture;
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInitGetUnitIdClientOnly(
	GAME * game,
	UNIT_CREATE * uc)
{
#if ISVERSION(SERVER_VERSION)
	REF(game);
	REF(uc);
	return FALSE;
#else
	ASSERT_RETFALSE(IS_CLIENT(game));
	UNIT_HASH * uh = game->pUnitHash;
	ASSERT_RETFALSE(uh);

	++uh->uidCurClient;
#if ISVERSION(DEBUG_ID)
	uc->unitid = ID(CLIENT_UNIT_MASK + uh->uidCurClient, uc->szDbgName);
#else
	uc->unitid = CLIENT_UNIT_MASK + uh->uidCurClient;
#endif
#if ISVERSION(DEVELOPMENT)
	CLT_VERSION_ONLY(HALT(uc->unitid >= CLIENT_UNIT_MASK));	// CHB 2007.08.01 - String audit: development
    ASSERT(uc->unitid >= CLIENT_UNIT_MASK);
#endif
	return TRUE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitInitGetUnitId(
	GAME * game,
	UNIT_CREATE * uc)
{
	if (uc->unitid == INVALID_ID)	// get a valid unitid
	{
		if (TEST_MASK(uc->dwUnitCreateFlags, UCF_CLIENT_ONLY))
		{
			ASSERT_RETFALSE(sUnitInitGetUnitIdClientOnly(game, uc));
		}
		else
		{
			UNIT_HASH * uh = game->pUnitHash;
			ASSERT_RETFALSE(uh);
			++uh->uidCur;
#if ISVERSION(DEBUG_ID)
			uc->unitid = ID(uh->uidCur, uc->szDbgName);
#else
			uc->unitid = uh->uidCur;
#endif
#if ISVERSION(DEVELOPMENT)
			CLT_VERSION_ONLY(HALT(uc->unitid < CLIENT_UNIT_MASK));	// CHB 2007.08.01 - String audit: development
			ASSERT(uc->unitid < CLIENT_UNIT_MASK);
#endif
		}
	}
	else
	{
#if ISVERSION(DEBUG_ID)
		PStrCopy(uc->unitid.m_szName, uc->szDbgName, MAX_ID_DEBUG_NAME);
#endif
	}
	if (UnitGetById(game, uc->unitid))
	{
		// the unit is already in the game, don't create it again.
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_sUnitDeactivateUnregisterEvents(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETURN(unit_data);

	if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH))
	{
		UnitUnregisterTimedEvent(unit, sMonitorPlayerApproach);
	}
	if( AppIsTugboat() )
	{

		// we don't keep dead corpses of respawning units around at all.
		if( UnitTestFlag(unit, UNITFLAG_SHOULD_RESPAWN) &&
			IsUnitDeadOrDying(unit) )
		{
			DWORD dwFreeFlags = UFF_SEND_TO_CLIENTS;
			UnitFree( unit, dwFreeFlags );
			return;
		}

		if( UnitGetClass( unit ) == GlobalIndexGet( GI_MONSTER_RESPAWNER ) &&
			UnitHasTimedEvent(unit, s_MonsterRespawnFromRespawner))
		{
			UnitUnregisterTimedEvent(unit, s_MonsterRespawnFromRespawner );
		}
		else if( UnitGetClass( unit ) == GlobalIndexGet( GI_OBJECT_RESPAWNER ) &&
			UnitHasTimedEvent(unit, s_ObjectRespawnFromRespawner))
		{
			UnitUnregisterTimedEvent(unit, s_ObjectRespawnFromRespawner );
		}
	}


	SkillsUnregisterTimedEventsForUnitDeactivate(unit);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUpdateTemplarPulse(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_sUnitActivateRegisterEvents(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	// We're going to re-unregister, just to make sure that nothing pathological happens
	s_sUnitDeactivateUnregisterEvents(game, unit);

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETURN(unit_data);

	// reactivate quest timed event
	if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH))
	{
		UnitRegisterEventTimed(unit, sMonitorPlayerApproach, NULL, MONITOR_PLAYER_APPROACH_RATE);
	}

	if( AppIsTugboat() )
	{

		if( UnitGetClass( unit ) == GlobalIndexGet( GI_MONSTER_RESPAWNER ) &&
			UnitGetStat( unit, STATS_RESPAWN_CLASS_ID ) != 0 &&
			!UnitHasTimedEvent( unit, s_MonsterRespawnFromRespawner) )
		{
			DWORD dwNow = GameGetTick( game );
			DWORD dwLastTick = UnitGetStat( unit, STATS_LAST_INTERACT_TICK );
			dwLastTick += UnitGetData( unit )->nRespawnPeriod;
			UnitRegisterEventTimed(unit, s_MonsterRespawnFromRespawner, NULL, MAX( 1,(int)dwLastTick - (int)dwNow)  );
		}
		else if( UnitGetClass( unit ) == GlobalIndexGet( GI_OBJECT_RESPAWNER ) &&
			UnitGetStat( unit, STATS_RESPAWN_CLASS_ID ) != 0 &&
			!UnitHasTimedEvent( unit, s_ObjectRespawnFromRespawner)  )
		{
			DWORD dwNow = GameGetTick( game );
			DWORD dwLastTick = UnitGetStat( unit, STATS_LAST_INTERACT_TICK );
			dwLastTick += UnitGetData( unit )->nRespawnPeriod;
			UnitRegisterEventTimed(unit, s_ObjectRespawnFromRespawner, NULL, MAX( 1,(int)dwLastTick - (int)dwNow)  );
		}
	}


	// reactivate start skills
	for(int i=0; i<NUM_INIT_SKILLS; i++)
	{
		if (unit_data->nInitSkill[i] != INVALID_ID)
		{
			const SKILL_DATA * skill_data = SkillGetData(game, unit_data->nInitSkill[i]);
			if (skill_data && SkillDataTestFlag(skill_data, SKILL_FLAG_RESTART_SKILL_ON_REACTIVATE))
			{
				DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER;
				SkillStartRequest(game, unit, unit_data->nInitSkill[i], INVALID_ID, VECTOR(0), dwFlags, SkillGetNextSkillSeed(game));
			}
		}
	}
	if (unit_data->nSkillLevelActive != INVALID_LINK)
	{
		const SKILL_DATA * skill_data = SkillGetData(game, unit_data->nSkillLevelActive);
		if (skill_data && SkillDataTestFlag(skill_data, SKILL_FLAG_RESTART_SKILL_ON_REACTIVATE))
		{
			DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest(game, unit, unit_data->nSkillLevelActive, INVALID_ID, VECTOR(0), dwFlags, SkillGetNextSkillSeed(game));		
		}
	}

	// reactivate stats based event
	if (UnitGetStat(unit, STATS_HOLY_RADIUS) > 0)
	{
		sUpdateTemplarPulse(game, unit, EVENT_DATA());
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_UnitActivate(
	UNIT * unit)
{
	ASSERT_RETURN(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	ROOM * room = UnitGetRoom(unit);
	ASSERT_RETURN(room);

	if (!UnitTestFlag(unit, UNITFLAG_DEACTIVATED))
	{
		if (UnitTestFlag(unit, UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE))
		{
			s_sUnitActivateRegisterEvents(game, unit);
		}
		return;
	}
	UnitClearFlag(unit, UNITFLAG_DEACTIVATED);
	GlobalUnitTrackerActivate(UnitGetSpecies(unit));

	int ai = UnitGetStat(unit, STATS_AI);

	TRACE_ACTIVE("ROOM [%d: %s]  UNIT [%d: %s]  AI [%d]  Activate Unit\n", RoomGetId(room), RoomGetDevName(room), UnitGetId(unit), UnitGetDevName(unit), ai);
	
	// start ai again for living units
	if (ai)
	{
		if (IsUnitDeadOrDying(unit) == FALSE)
		{
			AI_Register(unit, TRUE);
		}
	}

	s_sUnitActivateRegisterEvents(game, unit);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitClassCanBeDepopulated(
	const UNIT_DATA * unit_data)
{
	ASSERT_RETFALSE(unit_data);

	if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_DONT_DEPOPULATE))
	{
		return FALSE;
	}
	// warps, once instanced, should never go away except by explicit game logic actions
	if (UnitIsA(unit_data->nUnitType, UNITTYPE_WARP))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitCanBeDepopulated(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	if (UnitTestFlag(unit, UNITFLAG_DONT_DEPOPULATE) == TRUE)
	{
		return FALSE;
	}

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);

	if (s_UnitClassCanBeDepopulated(unit_data) == FALSE)
	{
		return FALSE;
	}
	return TRUE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitClassCanBeDeactivated(
	const UNIT_DATA * unit_data)
{
	ASSERT_RETFALSE(unit_data);

	//
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitCanBeDeactivatedEver(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);

	// can't deactivate things owned by players
	UNIT * owner = UnitGetUltimateContainer(unit);
	if (owner && UnitIsPlayer(owner))
	{
		return FALSE;
	}

	return TRUE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitCanBeDeactivatedWithRoom(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	if (UnitTestFlag(unit, UNITFLAG_DONT_DEACTIVATE_WITH_ROOM) == TRUE)
	{
		return FALSE;
	}	

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);

	if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_DONT_DEACTIVATE_WITH_ROOM) == TRUE)
	{
		return FALSE;
	}	

	return s_UnitCanBeDeactivatedEver(unit);	
}


//----------------------------------------------------------------------------
// for now, we just stop some processing, we need to replace this with
// an actual deactivate that will archive the unit and take less memory
//----------------------------------------------------------------------------
void s_UnitDeactivate(
	UNIT * unit,
	BOOL bDeactivateLevel)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	ROOM * room = UnitGetRoom(unit);
	ASSERT_RETURN(room);

	// don't bother if it's not a unit that can be deactivated
	if (bDeactivateLevel && s_UnitCanBeDeactivatedEver(unit) == FALSE)
	{
		return;
	}
	else if (!bDeactivateLevel && s_UnitCanBeDeactivatedWithRoom(unit) == FALSE)
	{
		return;
	}

	if (UnitIsA(unit, UNITTYPE_MISSILE))
	{
		UnitFree(unit);
		return;
	}

	BOOL bAlreadyDeactivated = UnitTestFlag(unit, UNITFLAG_DEACTIVATED);

	UnitSetFlag(unit, UNITFLAG_DEACTIVATED);

	// do all this stuff even if deactivated flag is set, just in case
	SkillStopAll(game, unit);

	// remove unit from step list
	UnitClearStepModes(unit);
	UnitStepListRemove(game, unit, SLRF_FORCE);

	// stop ai
	if (UnitGetStat(unit, STATS_AI))
	{
		AI_Unregister(unit);
	}
	s_sUnitDeactivateUnregisterEvents(game, unit);

	if( unit->m_pPathing )
	{
		UnitClearBothPathNodes( unit );
	}
	PathStructFree(game, unit->m_pPathing);
	unit->m_pPathing = NULL;
	if (bAlreadyDeactivated)
	{
		return;
	}

	TRACE_ACTIVE("ROOM [%d: %s]  UNIT [%d: %s]  Deactivate Unit\n", RoomGetId(UnitGetRoom(unit)), RoomGetDevName(UnitGetRoom(unit)), UnitGetId(unit), UnitGetDevName(unit));

	GlobalUnitTrackerDeactivate(UnitGetSpecies(unit));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitInit(
	GAME * game,
	UNIT_CREATE * uc)
{
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(game->pUnitHash);
	ASSERT_RETNULL(uc);
	ASSERT_RETNULL(uc->pData);

	if (UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_SERVER_ONLY) && !IS_SERVER(game))
	{
		return NULL;
	}
	if (UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_CLIENT_ONLY) && !IS_CLIENT(game))
	{
		return NULL;
	}
	if (!AppIsSinglePlayer() && UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_SINGLE_PLAYER_ONLY))
	{
		return NULL;
	}
	if (!AppIsMultiplayer() && UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_MULTIPLAYER_ONLY))
	{
		return NULL;
	}
#if !ISVERSION(DEVELOPMENT)
	if (AppIsSinglePlayer() && UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY))
	{
		return NULL;
	}
#endif

	if (!sUnitInitGetUnitId(game, uc))
	{
		return NULL;
	}

	if (UnitCanBeCreated( game, uc->pData ) == FALSE)
	{
		return NULL;
	}

	ASSERT_RETNULL(UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_INITIALIZED));

	UNIT * unit = (UNIT *)GMALLOCZ(game, sizeof(UNIT));
	ASSERT_RETNULL(unit);
#if ISVERSION(SERVER_VERSION)
	if(game->pServerContext )PERF_ADD64(game->pServerContext->m_pPerfInstance,GameServer,GameServerUnitCount,1);
#endif

	GlobalUnitTrackerAdd(uc->species);

	UnitSetFlag(unit, UNITFLAG_JUSTCREATED);
	if (IS_SERVER(game))
	{
		UnitSetCanSendMessages(unit, FALSE);
	}
	unit->pGame = game;
	unit->unitid = uc->unitid;
	unit->guid = INVALID_GUID;
	unit->pUnitData = uc->pData;
	unit->pSourceAppearanceUnitData = uc->pSourceData;
	unit->species = uc->species;
	UnitSetTeam( unit, INVALID_TEAM, FALSE );
	unit->nTeamOverride = INVALID_TEAM;
	unit->bStructural = UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_STRUCTURAL);
	unit->idLastUnitHit = INVALID_ID;
	unit->nWardrobe = INVALID_ID;
	unit->pEmailSpec = NULL;
	ArrayInit(unit->tEmailAttachmentsBeingLoaded, game->m_MemPool, 0);

	if (TEST_MASK(uc->dwUnitCreateFlags, UCF_NO_DATABASE_OPERATIONS))
	{
		UnitSetFlag( unit, UNITFLAG_NO_DATABASE_OPERATIONS );
	}
	
#ifdef _DEBUG_KNOWN_STUFF_TRACE
	trace("*KT* %s: UnitInit() - '%s' [%d] [Room=%d] (%.2f, %.2f, %.2f)\n", IS_SERVER( game ) ? "SERVER" : "CLIENT", 
		UnitGetDevName( unit ), UnitGetId( unit ), uc->pRoom ? RoomGetId( uc->pRoom ) : INVALID_ID,
		uc->vPosition.fX, uc->vPosition.fY, uc->vPosition.fZ);
#endif
	
	// clear owner (this clears client id too)
	UnitSetOwner(unit, NULL);
		
	unit->nAppearanceDefId = uc->nAppearanceDefId;
	if ( unit->nAppearanceDefId == INVALID_ID )
	{
		unit->nAppearanceDefId = unit->pUnitData->nAppearanceDefId;
	}
	unit->nAppearanceDefIdOverride = INVALID_ID;
	unit->bDropping = FALSE;
	unit->fDropPosition = 0;

	unit->m_pActivePath = (CPath*)GMALLOCZ( game, sizeof(CPath) );
	unit->m_pActivePath->CPathInit(FALSE, GameGetMemPool(game));

	unit->fUnitSpeed = 1;
	unit->events.unit = unit;
	unit->events.unitprev = &unit->events;	// set up dummy head of doubly linked circular list
	unit->events.unitnext = &unit->events;

	UNIT_HASH * uh = game->pUnitHash;
	UnitHashAdd(uh, unit);
	UnitInitStateFlags(unit);
	UnitStatsInit(unit);
	UnitMetricsInit( unit );
	
	// connect clients and units together for players
	UnitSetClientID(unit, INVALID_GAMECLIENTID);		
	if (uc->idClient != INVALID_GAMECLIENTID)
	{
		GAMECLIENT *pClient = ClientGetById( game, uc->idClient );
		ClientSetPlayerUnit( game, pClient, unit );
	}

	unit->vUpDirection = uc->vUpDirection;

	// setup direction (snap angle if necessary)
	VECTOR vDirection = uc->vFaceDirection;
	if (unit->pUnitData->nSnapAngleDegrees >= 0)
	{
	
		// what is the angle the unit was requested to be created at
		float flAngleRadians = VectorDirectionToAngle( uc->vFaceDirection );
		int nAngleDegrees = (int)RadiansToDegrees( flAngleRadians );
		
		// snap to snap angle
		int nNumberOfIncrements = nAngleDegrees / unit->pUnitData->nSnapAngleDegrees;
		int nNewAngleDegrees = unit->pUnitData->nSnapAngleDegrees * nNumberOfIncrements;
		float flNewAngleRadians = DegreesToRadians( nNewAngleDegrees );
		
		// get new face direction vector
		VectorDirectionFromAngleRadians( vDirection, flNewAngleRadians );

	}

	// setup direction
	UnitSetFaceDirection( unit, vDirection, TRUE, FALSE, !UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_FORCE_DRAW_TO_MATCH_VELOCITY) );

	// if no face direction is provided, pick one
	if (VectorIsZeroEpsilon( unit->vFaceDirection ))
	{
		VECTOR FaceDirection = RandGetVector( game );
		FaceDirection.fZ = 0.0;  // just random in X/Y plane
		VectorNormalize( FaceDirection );
		UnitSetFaceDirection( unit, FaceDirection, TRUE, TRUE );

	}
	
	unit->vQueuedFaceDirection = VECTOR(0, 0, 0);
	VectorNormalize(unit->vUpDirection, unit->vUpDirection);
	VectorNormalize(unit->vFaceDirection, unit->vFaceDirection);
	if (VectorIsZeroEpsilon(unit->vFaceDirection))
	{
		unit->vFaceDirection = VECTOR(1, 0, 0);
	}
	if (VectorIsZeroEpsilon(unit->vUpDirection))
	{
		unit->vUpDirection = VECTOR(0, 0, 1);
	}
	unit->vFaceDirectionDrawn = unit->vFaceDirection;
	unit->idMoveTarget = INVALID_ID;
	unit->nReservedType = UNIT_LOCRESERVE_NONE;

	UnitSetPosition(unit, uc->vPosition);
	unit->nPlayerAngle = uc->nAngle;

	if (TEST_MASK(uc->dwUnitCreateFlags, UCF_CLIENT_ONLY))
	{
		ASSERT( IS_CLIENT( game ) );
		UnitSetFlag( unit, UNITFLAG_CLIENT_ONLY, TRUE );
	}
	if (TEST_MASK(uc->dwUnitCreateFlags, UCF_DONT_DEACTIVATE_WITH_ROOM))
	{
		UnitSetFlag(unit, UNITFLAG_DONT_DEACTIVATE_WITH_ROOM, TRUE);		
	}
	if (TEST_MASK(uc->dwUnitCreateFlags, UCF_DONT_DEPOPULATE))
	{
		UnitSetFlag(unit, UNITFLAG_DONT_DEPOPULATE, TRUE);		
	}

	if ( TESTBIT( uc->pData->pdwFlags, UNIT_DATA_FLAG_ALWAYS_FACE_SKILL_TARGET ) )
	{
		UnitSetFlag( unit, UNITFLAG_FACE_SKILL_TARGET, TRUE );
	}

	if (UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_FACE_DURING_INTERACTION))
	{
		UnitSetFlag( unit, UNITFLAG_FACE_DURING_INTERACTION, TRUE );
	}
	
	//if ( UnitDataTestFlag(uc->pData, UNIT_DATA_FLAG_IS_GOOD) ) // we really should set this so that not everybody uses this memory
	{
		SkillsInitSkillInfo( unit );
	}

	unit->fScale = uc->fScale;
	UnitSetAcceleration(unit, uc->fAcceleration);
	UnitCalcVelocity(unit);
	UnitSetZVelocity(unit, 0.0f);	
	UnitSetOnGround(unit, TRUE);

	// unit's position is recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	unit->LevelProxMapNode = INVALID_ID;
	
	UnitSetType(unit, uc->pData->nUnitType);

	// I think that this is the earliest we can set a stat - after the UnitType has been set. - GS
	UnitSetStatFloat(unit, STATS_UNIT_SCALE_ORIGINAL, 0, uc->fScale);

	// set stat for current version
	UnitSetStat( unit, STATS_UNIT_VERSION, USV_CURRENT_VERSION );

	// assign existing GUID (if present)
	if (uc->guidUnit != INVALID_GUID && UnitSetGuid(unit, uc->guidUnit) == FALSE)
	{
		ASSERTV_GOTO(
			0, 
			UNIT_INIT_ERROR, 
			"Cannot init GUID of unit [%s] to [%I64x], GUID is in use by [%s]", 
			UnitGetDevName( unit ), 
			uc->guidUnit,
			UnitGetDevName( UnitGetByGuid( game, uc->guidUnit ) ));				
	}

	UNITGFX* pGfx = UnitGfxInit(unit, uc);
	if (!pGfx && IS_CLIENT(unit))
	{
		goto UNIT_INIT_ERROR;	
	}

	// for players, init quest stuff before we start asking quest questions when we change
	// rooms and are put into the world
	if ( AppIsHellgate() && UnitIsA( unit, UNITTYPE_PLAYER ) )
	{
		QuestsInit( unit );
	}
	
	// for recipe properties
	if( AppIsTugboat() && UnitIsA( unit, UNITTYPE_PLAYER ) )
	{
		RecipeInitUnit( game, unit );
	}
	// now that we have sufficient information about a unit, put it in the room 
	// specified, as a result the unit will be sent to clients who should know about it
	if (uc->pRoom)
	{
		UnitUpdateRoom(unit, uc->pRoom, TRUE);
	}

	UNITMODE eMode = MODE_IDLE;
	if ( UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_START_IN_TOWN_IDLE) && UnitHasMode( game, unit, MODE_IDLE_TOWN ) )
		eMode = MODE_IDLE_TOWN;

	if (IS_SERVER(game))
	{
		s_UnitSetMode(unit, eMode, FALSE);

		if (UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_MODES_IGNORE_AI) && UnitGetStat(unit, STATS_AI) != INVALID_ID)
		{
			AI_Register(unit, TRUE);
		}
	}
	else
	{
		if(uc->pData->eModeGroupClient >= 0)
		{
			c_UnitSetRandomModeInGroup(unit, uc->pData->eModeGroupClient);
		}
		else
		{
			c_UnitSetMode(unit, eMode, FALSE);
		}
	}

#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() )
	{
		if ( ! unit->pUnitData->tHavokShapePrefered.pHavokShape && unit->pUnitData->tHavokShapeFallback.pHavokShape )
		{
			sUnitDataInitHavokShape(game, (UNIT_DATA *)uc->pData);	// not thread safe!!!
		}
	
		sUnitCreateRigidBody( unit );
	}
#endif

	// if this unit faces players during interaction, save their original direction as
	// their preferred direction
	BOOL bHasPreferredDirection = UnitGetStat( unit, STATS_HAS_PREFERRED_DIRECTION );			
	if (bHasPreferredDirection == FALSE && UnitTestFlag(unit, UNITFLAG_FACE_DURING_INTERACTION) == TRUE)
	{
		VECTOR vDirection = UnitGetFaceDirection( unit, FALSE );
		UnitSetStatVector( unit, STATS_PREFERRED_DIRECTION_X, 0, vDirection ); 
		UnitSetStat( unit, STATS_HAS_PREFERRED_DIRECTION, TRUE );  // we have a preferred direction now
	}



	// init untargetable
	if (UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_UNTARGETABLE))
	{
		UnitSetDontTarget( unit, TRUE );
	}

	// check for tracking visual portals
	if (ObjectIsVisualPortal( unit ))
	{
		RoomAddVisualPortal( game, UnitGetRoom( unit ), unit );
	}

	// set bounce stats
	const UNIT_DATA *pUnitData = UnitGetData( unit );
	UnitSetStat( unit, STATS_BOUNCE_ON_HIT_UNIT,		TESTBIT( pUnitData->dwBounceFlags, BF_BOUNCE_ON_HIT_UNIT_BIT ) );
	UnitSetStat( unit, STATS_BOUNCE_ON_HIT_BACKGROUND,	TESTBIT( pUnitData->dwBounceFlags, BF_BOUNCE_ON_HIT_BACKGROUND_BIT ) );
	UnitSetStat( unit, STATS_BOUNCE_NEW_DIRECTION,		TESTBIT( pUnitData->dwBounceFlags, BF_BOUNCE_NEW_DIRECTION_BIT ) );
	UnitSetStat( unit, STATS_BOUNCE_RETARGET,			TESTBIT( pUnitData->dwBounceFlags, BF_BOUNCE_RETARGET_BIT ) );

	// set other states common to units
	UnitSetStatShift(unit, STATS_DAMAGE_INCREMENT,	pUnitData->nDmgIncrement);
	UnitSetStatShift(unit, STATS_SPLASH_INCREMENT,	pUnitData->nDmgIncrement);
	UnitSetStatShift(unit, STATS_CRITICAL_CHANCE,	pUnitData->nCriticalChance);
	UnitSetStatShift(unit, STATS_CRITICAL_MULT,		pUnitData->nCriticalMultiplier);
	UnitSetStatShift(unit, STATS_DAMAGE_INCREMENT_RADIAL,		pUnitData->nDmgIncrementRadial);
	UnitSetStatShift(unit, STATS_DAMAGE_INCREMENT_FIELD,		pUnitData->nDmgIncrementField);
	UnitSetStatShift(unit, STATS_DAMAGE_INCREMENT_DOT,			pUnitData->nDmgIncrementDOT);
	UnitSetStatShift(unit, STATS_DAMAGE_INCREMENT_AI_CHANGER,	pUnitData->nDmgIncrementAIChanger);

	// set tick unit was created on
	UnitSetStat( unit, STATS_SPAWN_TICK, GameGetTick(game) );

	float fWalkBase;
	float fRunBase;
	float fVelocityMin;
	float fVelocityMax;
	UnitGetSpeedsByMode( unit, MODE_WALK, fWalkBase, fVelocityMin, fVelocityMax );
	UnitGetSpeedsByMode( unit, MODE_RUN, fRunBase, fVelocityMin, fVelocityMax );
	UnitSetStat( unit, STATS_CAN_RUN, fRunBase > fWalkBase );

	// some units mark the rooms they are created in as no spawn
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ROOM_NO_SPAWN))
	{
		ROOM *pRoom = UnitGetRoom( unit );
		if (pRoom)
		{
			RoomSetFlag( pRoom, ROOM_NO_SPAWN_BIT );
		}
	}
	
	// setup restricted to GUID (if any)
	if (TEST_MASK(uc->dwUnitCreateFlags, UCF_RESTRICTED_TO_GUID) &&
		uc->guidRestrictedTo != INVALID_GUID)
	{
		UnitSetRestrictedToGUID( unit, uc->guidRestrictedTo );
	}

	if (uc->guidOwner != INVALID_GUID)
	{
		UNIT *pGUIDOwner = UnitGetByGuid( game, uc->guidOwner );
		if (pGUIDOwner)
		{
			UnitSetGUIDOwner( unit, pGUIDOwner );
		}
	}
	
	if ( AppIsTugboat() )
	{
		UnitCreateIconTexture( game, unit );
	}

	// set initial states
	BOOL bIsClientOnly = TEST_MASK(uc->dwUnitCreateFlags, UCF_CLIENT_ONLY);
	int nDuration = uc->pData->nInitStateTicks;
	StateArrayEnable( unit, uc->pData->pnInitStates, NUM_INIT_STATES, nDuration, bIsClientOnly );

	// give initial faction scores
	FactionGiveInitialScores( unit );

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))
	{
		UnitRegisterEventHandler( game, unit, EVENT_STATE_SET, c_sUnitSetState, &EVENT_DATA( ) );
		UnitRegisterEventHandler( game, unit, EVENT_STATE_CLEARED, c_sUnitClearState, &EVENT_DATA( ) );
	}
#endif //!ISVERSION(SERVER_VERSION)

	if (IS_SERVER(game))
	{
		// assign GUID to unit if its unit data requires us to and we don't have one yet,
		// note that we could set the data flag for players, but I don't want to touch the
		// excel data just right now
		if (UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_ASSIGN_GUID ) &&
			UnitGetGuid( unit ) == INVALID_GUID)
		{
			if (UnitSetGuid(unit, GameServerGenerateGUID()) == FALSE)
			{
				goto UNIT_INIT_ERROR;
			}
		}
	}

	// success
	return unit;

UNIT_INIT_ERROR:
	
	if (unit)
	{
		UnitFree(unit, 0);
		unit = NULL;
	}

	return NULL;		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitPostCreateDo(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	ROOM * room = UnitGetRoom(unit);
	if (room == NULL)
	{
		return;
	}

	if (IS_SERVER(game))
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)		
		if (RoomIsActive(room))
		{
			s_sUnitActivateRegisterEvents(game, unit);
		}
		else
		{
			UnitSetFlag(unit, UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE);
		}
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetControlUnit(
	GAME* game,
	UNIT* unit)
{
	ASSERT_RETURN(game);
	ASSERT(IS_CLIENT(game));
#if !ISVERSION(SERVER_VERSION)
	game->pControlUnit = unit;
	if (unit)
	{
		unit->nPlayerAngle = VectorDirectionToAngleInt( UnitGetFaceDirection( unit, FALSE ) );
		c_PlayerSetAngles(game, UnitGetPlayerAngle(unit));

		GameSetCameraUnit( game, UnitGetId( unit ) );

		int nModel = c_UnitGetModelId(unit);
		ASSERT_RETURN(nModel != INVALID_ID);

		if (UnitIsA(unit, UNITTYPE_PLAYER))
		{
			c_PlayerInitControlUnit(unit);
		}
	
		// initialize gold for client side ui	
		cCurrency playerCurrency = UnitGetCurrency( unit );
		DWORD dwFlags = 0;
		SETBIT( dwFlags, MONEY_FLAGS_SILENT_BIT );
		UIMoneySet( game, unit, playerCurrency, dwFlags );

		if ( AppIsMultiplayer() )
		{
			c_LoadAutomap( game, AppGetCurrentLevel() );
		}
	}
	else
	{
		GameSetCameraUnit( game, INVALID_ID );
	}
#endif //!SERVER_VERSION
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitRemove(
	GAME *pGame,
	UNITID idUnit,
	DWORD dwUnitFreeFlags )
{
	UNIT *pUnit = UnitGetById( pGame, idUnit );
	if (pUnit == NULL)
	{
#ifdef _DEBUG_KNOWN_STUFF_TRACE	
		trace("UNIT_REMOVE; Client received unit remove for unknown unit id '%d'\n", idUnit);
#else
		return;
#endif		
	}
	else
	{
//trace( "*UNIT_REMOVE* %s [%d]", UnitGetDevName( pUnit ), UnitGetId( pUnit ) );	
		if ( UnitGetStat( pUnit, STATS_ITEM_LOCKED_IN_INVENTORY) )
			return;

		UNIT* pControlUnit = GameGetControlUnit(pGame);
		if (pControlUnit && UnitGetContainer(pUnit) == pControlUnit)
		{
			UnitInventoryRemove(pUnit, ITEM_ALL);
			ASSERT(UnitGetContainer(pUnit) == NULL);
		}

		UnitLostItemDebug( pUnit );

		// free unit
		UnitFree( pUnit, dwUnitFreeFlags );
		
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitRemoveFromWorld(
	UNIT* unit,
	BOOL bSendRemoveMessage)
{
	ASSERT_RETURN(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);
	SkillStopAll( game, unit );
	
	// not in any room anymore
	UnitUpdateRoom( unit, NULL, bSendRemoveMessage );

	// stop moving	
	UnitStepListRemove(game, unit, SLRF_FORCE );

	// item is no longer in world
	UnitSetPosition(unit, INVALID_POSITION);	
	unit->bDropping = FALSE;

#if !ISVERSION(SERVER_VERSION)
	// client removal stuffs	
	if (IS_CLIENT(game))
	{
		UNITGFX *gfx = unit->pGfx;
		if (!gfx)
		{
			return;
		}

		int nModel = c_UnitGetModelIdFirstPerson( unit );
		if ( nModel != INVALID_ID )
			e_ModelSetDrawAndShadow( nModel, FALSE);

		nModel = c_UnitGetModelIdThirdPerson( unit );
		if ( nModel != INVALID_ID )
			e_ModelSetDrawAndShadow( nModel, FALSE);
	}
#endif //!SERVER_VERSION
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSendRemoveMessage(
	UNIT * unit,
	DWORD dwUnitFreeFlags,
	BOOL bFree)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	// do nothing if can't send messages
	if (UnitCanSendMessages(unit) == FALSE)
	{
		if (bFree)
		{
			for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
			{
				if (UnitIsKnownByClient(client, unit))
				{
					ClientRemoveKnownUnit(client, unit);
				}
			}
		}
		return;
	}

	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		if (UnitIsKnownByClient(client, unit))
		{
			s_RemoveUnitFromClient(unit, client, dwUnitFreeFlags);		
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoUnitDelayedFree(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERT(pUnit->m_nEventHandlerIterRef <= 0);
	DWORD dwFlags = (DWORD)tEventData.m_Data1;
	UnitFree(pUnit, dwFlags);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitDelayedFree(
	UNIT* unit,
	DWORD dwFlags,
	int nDelay = 0)
{
	ASSERT_RETURN(unit);
	BOOL bSend = (dwFlags & UFF_SEND_TO_CLIENTS) != 0;
	
	UnitSetFlag( unit, UNITFLAG_DELAYED_FREE, TRUE );
	UnitSetStat(unit, STATS_HP_CUR, 0);
	UnitChangeTargetType(unit, TARGET_NONE);

	// strip off the send flag since we will do the send now
	dwFlags &= ~UFF_SEND_TO_CLIENTS;
	dwFlags &= ~UFF_DELAYED;
	
	// build event data
	EVENT_DATA tEventData;
	tEventData.m_Data1 = dwFlags;
	
	// setup event to free unit in future
	UnitRegisterEventTimed(unit, sDoUnitDelayedFree, &tEventData, nDelay);

	// if instructed to send, do it
	if (bSend && IS_SERVER(unit))
	{
		UnitSendRemoveMessage(unit, 0, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerFree(
	UNIT *pPlayer,
	DWORD dwUnitFreeFlags)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	if (IS_SERVER( pGame ))
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)

		// leaving a level will remove an open level from the pool if there is nobody left
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		if (pLevel)
		{
			OpenLevelPlayerLeftLevel( pGame, LevelGetID( pLevel ) );
		}

		// call delete on player
		s_PlayerRemove( pGame, pPlayer );

		// cancel any tasks this player had
		s_TaskAbanbdonAll( pPlayer );

		// cancel all trade
		s_TradeCancel( pPlayer, TRUE );

		// close any town portals if exiting the game, we don't close them when switching instances
		// because in multiplayer, entering your own town portal will take you to town in another
		// game instance, and we of course want to be able to come back to our own portal
		if ( (dwUnitFreeFlags & UFF_SWITCHING_INSTANCE) == 0)
		{
		
			// close town portals
			s_TownPortalsClose( pPlayer );
						
		}

		// if this player is not in a party, close any party portals in this game instance
		// that are open to them
		if (UnitGetPartyId( pPlayer ) == INVALID_ID)
		{
			// close any party portals to this unit
			s_WarpPartyMemberCloseLeadingTo( pGame, UnitGetGuid( pPlayer ) );
		}
						
		// close any warps to party members and others
		s_WarpCloseAll( pPlayer, WARP_TYPE_PARTY_MEMBER );
		s_WarpCloseAll( pPlayer, WARP_TYPE_RECALL );

		// a player being freed will now trigger a room update for all other players
		// on this level to give the other client a chance to forget the room
		// this player was in
		s_LevelQueuePlayerRoomUpdate( UnitGetLevel( pPlayer ), pPlayer );
		
		// when players leave a level, the aggression lists for other players there need to be updated
		if( AppIsTugboat() )
		{
			for (GAMECLIENT * client = ClientGetFirstInGame(pGame); client; client = ClientGetNextInGame(client))
			{
				UNIT* pUnit = ClientGetControlUnit(client);
				RemoveFromAggressionList( pUnit, pPlayer );
			}
		}
		// remove player from this level
		//s_LevelRemovePlayer( UnitGetLevel( pPlayer ), pPlayer );
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)	
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		// remove player from client knowledge
		c_PlayerRemove( pGame, pPlayer );
		if( AppIsTugboat() )
		{
			//c_QuestsFreeClientLocalValues( pPlayer );
		}
	}
#endif// !ISVERSION(SERVER_VERSION)

	// common stuff
	
	// free quest structures
	if( AppIsHellgate() )
	{
		QuestsFree( pPlayer );		
	}
	else
	{
		RecipeFreeUnit( pGame, pPlayer );
	}


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitFreeClearControlUnit(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	GAMECLIENT * client = UnitGetClient(unit);
	if (client)
	{
		if (ClientGetPlayerUnit(client, TRUE) == unit)
		{
			ClientSetPlayerUnit(game, client, NULL);
		}
		if (ClientGetControlUnit(client, TRUE) == unit)
		{
			ClientSetControlUnit(game, client, NULL);
		}
	}
	if (game->pControlUnit == unit)
	{
		GameSetControlUnit(game, NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL c_UnitFreeAllowDelayedFree(
	DWORD flags)
{
	return !(flags & UFF_TRUE_FREE);
}
#endif


//----------------------------------------------------------------------------
// on the client, in certain situations, we do delayed free to give a unit
// time to disappear
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL c_UnitFreeCheckDelayedFree(
	GAME * game,
	UNIT * unit,
	DWORD flags)
{
	if (!IS_CLIENT(game))
	{
		return FALSE;
	}
	if (!c_UnitFreeAllowDelayedFree(flags))
	{
		return FALSE;
	}

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);
	if (unit_data->nClientMinimumLifetime <= 0)
	{
		return FALSE;
	}

	GAME_TICK tiSpawn = UnitGetStat(unit, STATS_SPAWN_TICK);
	if ((GameGetTick(game) - tiSpawn) < (DWORD)unit_data->nClientMinimumLifetime)
	{
		int ticks = unit_data->nClientMinimumLifetime - (GameGetTick(game) - tiSpawn);
		ASSERT_RETFALSE(ticks > 0);
		sUnitDelayedFree(unit, flags, ticks);
		return TRUE;
	}

	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitFreeAddToFreeList(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(!UnitTestFlag(unit, UNITFLAG_ONFREELIST));
	ASSERT_RETURN(unit->m_FreeListNext == NULL);
	UnitSetFlag(unit, UNITFLAG_ONFREELIST, TRUE);
	unit->m_FreeListNext = game->m_FreeList;
	game->m_FreeList = unit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UnitFreeListFree(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	TRACE_UNIT_FREE("[%s] UnitFreeListFree() [%d: %s]\n", (IS_SERVER(unit) ? "SVR" : "CLT"), UnitGetId(unit), UnitGetDevName(unit));

	// here we should assert that the unit doesn't have any of the stuff that might get added between UnitFree() and here
	ASSERT(UnitTestFlag(unit, UNITFLAG_FREED));
	ASSERT(UnitTestFlag(unit, UNITFLAG_ONFREELIST));
	ASSERT(unit->invnode == NULL);
	ASSERT(unit->inventory == NULL);
	ASSERT(unit->events.next == NULL && unit->events.prev == NULL);
	DBG_ASSERT(unit->m_nEventHandlerCount == 0);
	
	UnitFreeClearControlUnit(game, unit);
	UnitFreeEventHandlers(game, unit);

	if (unit->pSkillInfo)
	{
		SkillsFreeSkillInfo(unit);
	}

	if (unit->m_pActivePath)
	{
		unit->m_pActivePath->~CPath();
		GFREE(game, unit->m_pActivePath);
	}
	PathStructFree(game, unit->m_pPathing);
	AI_Free(game, unit);

	// this also runs shutdown functions for quests, so not sure if it should be here
	if (unit->m_pQuestGlobals)
	{
		QuestsFree(unit);
	}

	if( unit->pRecipePropertyValues )
	{
		RecipeFreeUnit( game, unit );	//free those recipe properties
	}

	UnitFreeTags(unit);
	UnitStatsFree(unit);
	UnitFreeStateFlags(unit);

	//free those points of interest if they have any..
	if( unit->m_pPointsOfInterest )
	{
		GFREE( game, unit->m_pPointsOfInterest );
		unit->m_pPointsOfInterest = NULL;
	}

	if (unit->szName)
	{
		GFREE(game, unit->szName);
		unit->szName = NULL;
	}

	if (unit->pPlayerMoveInfo)
	{
		GFREE(game, unit->pPlayerMoveInfo);
	}
	if (unit->pMusicInfo)
	{
		GFREE(game, unit->pMusicInfo);
	}
	if (unit->pAppearanceShape)
	{
		GFREE(game, unit->pAppearanceShape);
	}
	if (unit->nWardrobe)
	{
		WardrobeFree(UnitGetGame(unit), unit->nWardrobe);
	}
	UnitGfxFree(unit);

#ifdef HAVOK_ENABLED
	if (unit->pHavokImpact)
	{
		GFREE(game, unit->pHavokImpact);
	}
#endif

	GuidHashRemove(game, unit);

	if (!UnitTestFlag(unit, UNITFLAG_DEACTIVATED))
	{
		GlobalUnitTrackerDeactivate(UnitGetSpecies(unit));
	}

	GlobalUnitTrackerRemove(UnitGetSpecies(unit));

	GFREE(game, unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitFreeListFree(
	GAME * game)
{
	ASSERT_RETURN(game);
	while (game->m_FreeList)
	{
		UNIT * curr = game->m_FreeList;
		game->m_FreeList = curr->m_FreeListNext;
		UnitFreeListFree(game, curr);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDebugMissingControlUnit(
	UNIT *pUnit,
	GAMECLIENT *pClient)
{

	#ifdef LOST_CLIENT_CONTROL_UNIT_DEBUG	
	{
	
		ASSERTX_RETURN( pUnit, "Expected unit" );
		GAME *pGame = UnitGetGame( pUnit );
		
		if (pGame->eGameState == GAMESTATE_RUNNING)
		{
		
			if (UnitIsPlayer( pUnit ))
			{
				BOOL bIsControlUnit = FALSE;
				
				if (IS_SERVER( pGame ))
				{
				
					if (pClient)
					{
						// does unit belong to the client in question
						if (ClientGetControlUnit( pClient ) == pUnit)
						{
							bIsControlUnit = TRUE;
						}
					}
					else
					{
						// check if belong to any client
						for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); 
							 pClient; 
							 pClient = ClientGetNextInGame( pClient ))
						{
							UNIT *pControlUnit = ClientGetControlUnit( pClient );
							if (pControlUnit == pUnit)
							{
								bIsControlUnit = TRUE;
							}
							
						}
					}
				}
				else
				{
					UNIT *pControlUnit = GameGetControlUnit( pGame );
					if (pControlUnit == pUnit && AppGetState() == APP_STATE_IN_GAME)
					{
						bIsControlUnit = TRUE;
					}
				}

				ASSERTV( 
					bIsControlUnit == FALSE, 
					"[%s] Submit this bug to the database!  Attempting to remove the control unit '%s' of client while they are in game!",
					IS_SERVER( pGame ) ? "SERVER" : "CLIENT",
					UnitGetDevName( pUnit ));
							
			}

		}
				
	}
	#endif

}

//----------------------------------------------------------------------------
// this doesn't free the unit immediately, rather it either schedules a free
// or puts the unit on the free list to be freed at the end of the frame
//----------------------------------------------------------------------------
void UnitFree(
	UNIT * unit,
	DWORD flags,
	UNIT_FREE_CONTEXT context /*= UFC_NONE*/)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	// Clear path nodes if this is a blocking type
	if ( UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_BLOCKS_EVERYTHING ) )
		ObjectClearBlockedPathNodes(unit);

	UnitLostItemDebug( unit );
	sDebugMissingControlUnit( unit, NULL );
	
	if ((flags & UFF_ROOM_BEING_FREED) || (flags & UFF_HASH_BEING_FREED))
	{
		flags |= UFF_TRUE_FREE;
	}

	BOOL bSendToClients = (flags & UFF_SEND_TO_CLIENTS) != 0;	

	if (IS_SERVER( game ))
	{
	
		// mark this unit and everything in its inventory as about to be freed
		// we do this for players as a way to attempt to prevent dupe bugs
		// that occur when players are being freed (leaving the instance) but
		// during some logic during destruction we drop any of their items to
		// the ground
		if (UnitTestFlag( unit, UNITFLAG_ABOUT_TO_BE_FREED ) == FALSE)
		{
		
			// mark this unit
			UnitSetFlag( unit, UNITFLAG_ABOUT_TO_BE_FREED, TRUE );
			
			// mark everything we contain
			UNIT *pContained = NULL;
			while ((pContained = UnitInventoryIterate( unit, pContained, 0, TRUE )) != NULL)
			{
				UnitSetFlag( pContained, UNITFLAG_ABOUT_TO_BE_FREED, TRUE );
			}			
			
		}		
		
		// always fade out any players leaving
		if( AppIsTugboat() && UnitIsA( unit, UNITTYPE_PLAYER ) )
		{
			flags |= UFF_FADE_OUT;
		}
	
		// when leaving instance levels, we want to clear our instance game recorded
		// at the app client level
		LEVEL *pLevel = UnitGetLevel( unit );
		if (pLevel && LevelGetSrvLevelType( pLevel ) == SRVLEVELTYPE_DEFAULT)
		{
			CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
			if (pClientSystem)
			{
				GAMECLIENT *pGameClient = UnitGetClient( unit );
				if (pGameClient)
				{
					APPCLIENTID idAppClient = ClientGetAppId( pGameClient );
					ClientSystemSetLevelGoingTo( 
						AppGetClientSystem(), 
						idAppClient, 
						INVALID_ID, 
						NULL);
				}
			}
		}
		
	}
		
#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(unit) && c_UnitFreeCheckDelayedFree(game, unit, flags))
	{
		TRACE_UNIT_FREE("[CLT] UnitFree() DELAY [%d: %s]\n", UnitGetId(unit), UnitGetDevName(unit));
		return;
	}
	if (IS_CLIENT(unit) && c_UnitFreeAllowDelayedFree(flags) && (flags & UFF_FADE_OUT) && !UnitIsInLimbo( unit))
	{
		if( AppIsHellgate() )
		{
			TRACE_UNIT_FREE("[CLT] UnitFree() FADE  [%d: %s]\n", UnitGetId(unit), UnitGetDevName(unit));
			c_StateSet(unit, unit, STATE_FADE_AND_FREE_CLIENT, 0, 60, INVALID_ID);
			return;
		}
		else
		{
			if( unit && UnitGetData( unit )->szAppearance[0] != 0 && unit->pGfx )
			{
				TRACE_UNIT_FREE("[CLT] UnitFree() FADE  [%d: %s]\n", UnitGetId(unit), UnitGetDevName(unit));
				if( !UnitHasState( game, unit, STATE_FADE_AND_FREE_CLIENT ) )
				{
					c_StateSet(unit, unit, STATE_FADE_AND_FREE_CLIENT, 0, 60, INVALID_ID);
				}
				return;
			}
		}
		
	}
#endif

	if (UnitTestFlag(unit, UNITFLAG_FREED))
	{
		TRACE_UNIT_FREE("[%s] UnitFree() ALREADY FREED [%d: %s]\n", (IS_SERVER(unit) ? "SVR" : "CLT"), UnitGetId(unit), UnitGetDevName(unit));
		return;
	}
	UnitSetFlag(unit, UNITFLAG_FREED);
	TRACE_UNIT_FREE("[%s] UnitFree() [%d: %s]\n", (IS_SERVER(unit) ? "SVR" : "CLT"), UnitGetId(unit), UnitGetDevName(unit));

	// clear all of the states to make sure that things get cleaned up
	UnitSetFlag( unit, UNITFLAG_DEAD_OR_DYING ); //this unit is dead or is dying now.
	StateClearAllOfType(unit, INVALID_ID);

#if !ISVERSION(SERVER_VERSION)
	// client units may have an inventory icon
	if( IS_CLIENT(game) )
	{
		if( unit->pIconTexture  &&
			unit->pIconTexture->m_nTextureId != INVALID_ID )
		{

			e_TextureReleaseRef(unit->pIconTexture->m_nTextureId, FALSE);
			if( e_TextureGetRefCount(unit->pIconTexture->m_nTextureId) <= 0 )
			{
				unit->pIconTexture->m_nTextureId = INVALID_ID;
			}
			unit->pIconTexture = NULL;
		}
	}
#endif

#ifdef HAVOK_ENABLED
	if (unit->pHavokRigidBody)
	{
		LEVEL * level = UnitGetLevel(unit);
		HavokReleaseRigidBody(UnitGetGame(unit), level ? level->m_pHavokWorld : NULL, unit->pHavokRigidBody);
	}
#endif

	UnitClearStat(unit, STATS_ITEM_LOCKED_IN_INVENTORY, 0);

	UnitUnFuse(unit);

	// we need to figure out how to free units within a triggered event
	UnitTriggerEvent(game, EVENT_ON_FREED, unit, NULL, CAST_TO_VOIDPTR(flags));

	// set flag
	UnitSetFlag(unit, UNITFLAG_JUSTFREED);

#ifdef _DEBUG_KNOWN_STUFF_TRACE
	UNIT * container = UnitGetContainer(unit);
	trace(container ? "*KT* %s: UnitFree() - '%s' [%d] (contained by '%s' [%d])\n" : "*KT* %s: UnitFree() - '%s' [%d]\n",
		IS_SERVER( game) ? "SERVER" : "CLIENT", UnitGetDevName( unit ), UnitGetId( unit ), 
		container ? UnitGetDevName(container) : "nothing", container ? UnitGetId(container) : INVALID_ID);
#endif

	// remove count in room
	if (ObjectIsVisualPortal(unit))
	{
		RoomRemoveVisualPortal(game, UnitGetRoom(unit), unit);
	}

	// on the client, tell the UI that this thing is going away
	if (IS_CLIENT(game))
	{
		IF_NOT_SERVER_ONLY(UIOnUnitFree(unit));
	}
	
	if (IS_SERVER(game) && bSendToClients)
	{
		UnitSendRemoveMessage(unit, flags, TRUE);
	}

	TeamRemoveUnit(unit);

	// free player specific data, some of this involves cleaning up task inventory, so we like
	// to do this before we free the inventory itself
	if (UnitIsA(unit, UNITTYPE_PLAYER))
	{
		sPlayerFree(unit, flags);
		SVR_VERSION_ONLY(s_PlayerEmailFreeEmail(game, unit));
		unit->tEmailAttachmentsBeingLoaded.Destroy();
	}

	// free inventory - do this before removing unit from room and before we restrict
	// message sending so we can send messages to those in rooms that care

	if (UnitGetContainer(unit))
	{
		// map unit free context to inventory context
		INV_CONTEXT eInvContext;
		switch (context)
		{
			case UFC_ITEM_DROP:
				eInvContext = INV_CONTEXT_DROP;
				break;
			case UFC_ITEM_DISMANTLE:
				eInvContext = INV_CONTEXT_DISMANTLE;
				break;
			case UFC_ITEM_DESTROY:
				eInvContext = INV_CONTEXT_DESTROY;
				break;
			case UFC_ITEM_UPGRADE:
				eInvContext = INV_CONTEXT_UPGRADE;
				break;
			case UFC_NONE:
			default:
				eInvContext = INV_CONTEXT_NONE;
		}
		UnitInventoryRemove(unit, ITEM_ALL, 0, TRUE, eInvContext);
	}
	UnitInventoryFree(unit, bSendToClients);
	UnitInvNodeFree(unit);

	UnitFreeClearControlUnit(game, unit);

	// stop any other messages from being sent about this unit or this units inventory
	UnitSetCanSendMessages(unit, FALSE);

	UnitUnregisterAllTimedEvents(unit);

	UnitUnregisterAllEventHandlers(game, unit);

	// clear the "temporarily disable pathing" flag - otherwise the unit tries to do a PathingUnitWarped, 
	// which can end in badness it tries to do anything complicated, like clear step modes (which, forces the unit
	// to the ground if it's in knockback, which checks the room collision, which crashes)
	UnitClearFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
	UnitStepListRemove(game, unit, SLRF_FORCE);
	ASSERTX(UnitGetGame(unit) == game, "Unit has bad game.  Unit corrupted/deleted?");
	if ((flags & UFF_HASH_BEING_FREED) == 0)
	{	//Fix for GameFree crash: we free dungeons/levels/rooms first, so we don't need to do this on unithashfree.
		//UnitRemoveFromWorld( unit, TRUE );
		UnitRemoveFromRoomList(unit, TRUE);
	}

	GuidHashRemove(game, unit);
	UnitHashRemove(game, unit);

	// KCK: Make sure our control unit and camera isn't pointing to this unit
	if (game->pControlUnit == unit)
		game->pCameraUnit = NULL;
	if (game->pCameraUnit == unit)
		game->pCameraUnit = game->pControlUnit;

	UnitFreeAddToFreeList(game, unit);
#if ISVERSION(SERVER_VERSION)
	if(game->pServerContext )PERF_ADD64(game->pServerContext->m_pPerfInstance,GameServer,GameServerUnitCount,-1);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetName(
	UNIT* unit,
	const WCHAR* szName)
{
	ASSERT(unit && szName);
	if (!(unit && szName))
	{
		return;
	}
	UnitClearName(unit);
	int len = PStrLen(szName, MAX_CHARACTER_NAME-1);
	if (len <= 0)
	{
		return;
	}
	unit->szName = (WCHAR*)GMALLOC(UnitGetGame(unit), (len + 1) * sizeof(WCHAR));
	PStrCopy(unit->szName, szName, len + 1);
	unit->szName[len] = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearName(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	if (pUnit->szName)
	{
		GAME *pGame = UnitGetGame( pUnit );
		GFREE( pGame, pUnit->szName );
		pUnit->szName = NULL;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetNameOverride(
	UNIT *pUnit,
	int nString)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitSetStat( pUnit, STATS_NAME_OVERRIDE_STRING, nString );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetNameOverride(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_NAME_OVERRIDE_STRING );
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCELTABLE UnitGenusGetDatatableEnum(
	GENUS eGenus)
{
	EXCELTABLE eTable = ExcelGetDatatableByGenus( eGenus );
	if (eTable == DATATABLE_NONE)
	{
		return NUM_DATATABLES;
	}
	return eTable;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCELTABLE UnitGetDatatableEnum(
	UNIT* unit)
{
	ASSERT_RETVAL(unit, NUM_DATATABLES);
	return UnitGenusGetDatatableEnum( UnitGetGenus(unit) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeTargetType(
	UNIT * unit,
	TARGET_TYPE eTargetType)
{
	ASSERT_RETURN(unit);

	if (UnitGetTargetType(unit) == eTargetType)
	{
		return;
	}

	ROOM * room = UnitGetRoom(unit);
	if (!room)
	{
		unit->eTargetType = eTargetType;
		return;
	}

	UnitRemoveFromRoomList(unit, false);
	sUnitRoomListAdd(unit, room, eTargetType, false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UnitSetStartupLocation(
	UNIT* unit,
	ROOM* room,
	VECTOR& position,
	int nAngle)
{
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitFixNonvisible(
	UNIT * pUnit)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(IS_CLIENT(pUnit));

	UNIT_TAG_LAST_KNOWN_GOOD_POSITION * pTag = UnitGetLastKnownGoodPositionTag(pUnit);
	if(!pTag)
	{
		return;
	}

	BOOL bCheckBoundingBox = FALSE;

	// All of these checks are to see if we should do an OBB check or a point check
	int nAppearanceDefId = UnitGetAppearanceDefId(pUnit, FALSE);
	const BOUNDING_BOX * pBoundingBox = NULL;
	
	// If there's no appearance definition, then there won't be a model to worry about
	if(nAppearanceDefId != INVALID_ID)
	{
		// If the appearance definition hsa no bounding box, then we can't do a bounding box check
		pBoundingBox = AppearanceDefinitionGetBoundingBox(nAppearanceDefId);
		if(pBoundingBox)
		{
			bCheckBoundingBox = TRUE;
		}
	}

	// If, after all of this, there's no appearance instance, then we can't do a bounding box check
	int nAppearanceId = c_UnitGetAppearanceId(pUnit);
	if(nAppearanceId == INVALID_ID)
	{
		bCheckBoundingBox = FALSE;
	}

	if(bCheckBoundingBox)
	{
		// First get the world matrix.  Either the "real" one, or a fake one if that one's not available
		ORIENTED_BOUNDING_BOX tOrientedBox;
		const MATRIX * pWorldScaled = e_ModelGetWorldScaled(nAppearanceId);
		MATRIX tWorldMatrix;
		if(pWorldScaled)
		{
			tWorldMatrix = *pWorldScaled;
		}
		else
		{
			UnitGetWorldMatrix(pUnit, tWorldMatrix);
		}

		// Now make an OBB out of it.
		TransformAABBToOBB(tOrientedBox, &tWorldMatrix, pBoundingBox);

		// Get the current view frustum
		VIEW_FRUSTUM * pFrustum = e_GetCurrentViewFrustum();
		ASSERT_RETURN(pFrustum);

		// If the current position is in the view frustum, then we can't warp
		if(e_OBBInFrustum(tOrientedBox, pFrustum->GetPlanes(TRUE)))
		{
			return;
		}

		// Now make an OBB out of the world matrix at a new position
		tWorldMatrix._41 = pTag->vLastKnownGoodPosition.fX;
		tWorldMatrix._42 = pTag->vLastKnownGoodPosition.fY;
		tWorldMatrix._43 = pTag->vLastKnownGoodPosition.fZ;
		TransformAABBToOBB(tOrientedBox, &tWorldMatrix, pBoundingBox);

		// If the proposed position is in the view frustum, then we can't warp
		if(e_OBBInFrustum(tOrientedBox, pFrustum->GetPlanes(TRUE)))
		{
			return;
		}

		// At this point, both the latest server and client positions are not in the view frustum, 
		// so we should be able to warp the unit to its server position
		UnitWarp(pUnit, UnitGetRoom(pUnit), pTag->vLastKnownGoodPosition, UnitGetFaceDirection(pUnit, TRUE), UnitGetUpDirection(pUnit), 0, FALSE);
	}
	else
	{
	}
#else
	UNREFERENCED_PARAMETER(pUnit);
#endif
}

#define UNIT_SYNCH_TO_LAST_KNOWN_GOOD_TIME 10

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_UnitPositionSynchToLastKnownGood(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	//c_sUnitFixNonvisible(unit);
	//UnitRegisterEventTimed(unit, c_UnitPositionSynchToLastKnownGood, &EVENT_DATA(), UNIT_SYNCH_TO_LAST_KNOWN_GOOD_TIME);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitSetLastKnownGoodPosition(
	UNIT * pUnit,
	const VECTOR & vPosition,
	BOOL bSetOnly)
{
	if( AppIsTugboat() )
	{
		return;
	}
	ASSERT_RETURN(pUnit);
	if(!UnitIsA(pUnit, UNITTYPE_MONSTER))
	{
		return;
	}

	GAME * pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(IS_CLIENT(pGame));

	UNIT_TAG_LAST_KNOWN_GOOD_POSITION * pTag = UnitGetLastKnownGoodPositionTag(pUnit);
	pTag->vLastKnownGoodPosition = vPosition;
	pTag->tiLastKnownGoodPositionReceivedTick = GameGetTick(pGame);

	if(bSetOnly)
	{
		return;
	}

	if(!UnitHasTimedEvent(pUnit, c_UnitPositionSynchToLastKnownGood))
	{
		UnitRegisterEventTimed(pUnit, c_UnitPositionSynchToLastKnownGood, &EVENT_DATA(), UNIT_SYNCH_TO_LAST_KNOWN_GOOD_TIME);
	}

	c_sUnitFixNonvisible(pUnit);

	if(UnitPathIsActiveClient(pUnit))
	{
		UnitPathAbort(pUnit);
		UnitClearStepModes(pUnit); // <-- this should recalculate the client path if necessary
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sJumpLandedOnGround(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{

	// do not remove from step list if we're moving to a target, this is specifically
	// for the health/power modules that move into players that pick them up, but seems
	// like a generally good thing to handle when we're coming out of a jump
	UNITID idMoveTarget = UnitGetMoveTargetId( unit );
	if (idMoveTarget != INVALID_ID)
	{
		UNIT *pMoveTarget = UnitGetById( game, idMoveTarget );
		if (pMoveTarget && UnitGetVelocity( unit ) != 0.0f)
		{
			return;
		}
	}
	
	// remove from step list
	UnitStepListRemove(game, unit, SLRF_FORCE);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathingUnitWarped( 
	UNIT *pUnit,
	int nAIRegisterDelay /*= 1*/)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pUnit->m_pPathing, "Expected unit that can path" );
	GAME *pGame = UnitGetGame( pUnit );

	if( !pUnit->m_pPathing )
	{
		pUnit->m_pPathing = PathStructInit(pGame);
	}

	// clear any movement modes, otherwise things walk in place and it looks fugly!
	if (UnitIsInLimbo( pUnit) == FALSE)
	{
		UnitClearStepModes( pUnit );
	}

	// take them off the step list
	UnitStepListRemove( pGame, pUnit, SLRF_FORCE );	
	// clear path nodes that we were occupying
	
//	ROOM * pTestRoom = RoomGetByID( pGame, pUnit->m_pPathing->m_nRoomId );
//	if( pTestRoom )
	{
		UnitClearBothPathNodes( pUnit );
	}

	// clear any path stuff they had before
	PathStructClear( pUnit );	
	
	// where are we now
	LEVEL *pLevel = UnitGetLevel( pUnit );

	VECTOR vPosition = UnitGetPosition( pUnit );
	
	// which is our new path node
	// The following two luines are from Tugboat
//	ROOM *pRoomDestination = pRoom;
//	ROOM_PATH_NODE * pPathNode = RoomGetNearestFreeConnectedPathNode( pGame, pLevel, vPosition, &pRoomDestination, pUnit );
	ROOM *pRoomDestination = NULL;
	DWORD dwFlags = 0;
	if( AppIsTugboat() )
	{
		SETBIT(dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(dwFlags, NPN_CHECK_DISTANCE);
	}
	ROOM_PATH_NODE *pPathNode = RoomGetNearestFreePathNode(
		pGame,
		pLevel,
		vPosition,
		&pRoomDestination,
		NULL,
		0.0f,
		0.0f,
		NULL,
		dwFlags ); // TRAVIS : let's try a NULL room here, let it get room by position, and also search neighboring rooms.
	ASSERTX_RETURN( pRoomDestination, "Cannot find new room after warp" );
	ASSERTX_RETURN( pPathNode, "Cannot find new room path node after warp" );

	// reserve our new location
	UnitInitPathNodeOccupied( pUnit, pRoomDestination, pPathNode->nIndex );
	UnitReservePathOccupied( pUnit );

	// register another ai event so the unit can immediately have a chance to not look stupid
	// at their new location
	if(nAIRegisterDelay >= 0 && UnitIsInLimbo( pUnit ) == FALSE && !UnitIsA(pUnit, UNITTYPE_PLAYER))
	{
		AI_Register( pUnit, FALSE, nAIRegisterDelay );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitWarp(
	UNIT* unit,
	ROOM* room,
	const VECTOR& vPosition,
	const VECTOR& vFaceDirection,
	const VECTOR& vUpDirection,
	DWORD dwUnitWarpFlags,
	BOOL bSendToClient,
	BOOL bSendToOwner,
	LEVELID idLevel /*= INVALID_ID*/)
{
	ASSERT_RETURN(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	// if this unit has been marked as being part of a unit free, we never
	// want to warp it into the world.  typically this can result in a 
	// false item that exists in the game world and in some saved player
	// buffer that is transitioning another game instance
	if (UnitTestFlag( unit, UNITFLAG_ABOUT_TO_BE_FREED ))
	{
		// note that we're choosing to free it now even though it's slated to be
		// freed ... the reason being is that we've had situations where a player
		// is freed because they are leaving the instance, but something along
		// the way tried to put items back into an inventory slot of the freed
		// player, which fails, and causes the item to drop, which will
		// eventually call UnitWarp() to put it in the world ... and at that
		// point the unit is disconnected from the players inventory it was
		// leaving and we'll end up with an item on the ground that is a
		// possible duplicate -cday
		UnitFree( unit, UFF_SEND_TO_CLIENTS );
		return;	
	}

	// save the time of this warp	
	unit->tiLastGameTickWarped = GameGetTick(game);

	// soft warps on the client ( used only for player pathing broadcast to others ) don't happen if it is a short distance.
	// helps prevent popping on quick re-paths.
	if ( TESTBIT( dwUnitWarpFlags, UW_SOFT_WARP ) &&
		 IS_CLIENT( game ) )
	{
		if( VectorLengthSquared( ( vPosition - UnitGetPosition( unit ) ) ) < 4.0f )
		{
			UnitSetFaceDirection(unit, vFaceDirection );
			return;
		}
	}
	if ( TESTBIT( dwUnitWarpFlags, UW_PREMATURE_STOP ) &&
		IS_CLIENT( game ) )
	{
		
		if( PathPrematureEnd( game, unit, vPosition ) )
		{
			UnitSetFaceDirection(unit, vFaceDirection );
			return;
		}
	}

	// similar to soft warps, but for end-of-paths.
	if ( TESTBIT( dwUnitWarpFlags, UW_PATHING_WARP ) &&
		 IS_CLIENT( game ) )
	{
		if( VectorLength( ( vPosition - UnitGetPosition( unit ) ) ) < .4f )
		{
			UnitSetFaceDirection(unit, vFaceDirection );
			return;
		}
	}

	UnitSetPosition(unit, vPosition);
	UnitSetDrawnPosition(unit, vPosition);

	UnitSetUpDirection(unit, vUpDirection);
	UnitSetFaceDirection(unit, vFaceDirection, TRUE, TRUE);

	if( AppIsTugboat() &&
		IS_CLIENT( game ) )
	{
		// snap 'em to the ground!
		UnitForceToGround(unit);
	}

	if ( UnitIsA(unit, UNITTYPE_PLAYER) && vFaceDirection != NULL )
	{
		unit->nPlayerAngle = VectorDirectionToAngleInt(vFaceDirection);
	}

	ROOM * roomtmp = room; //code for assert in case roomget fails
	room = RoomGetFromPosition(game, room, &unit->vPosition);
	if (room == NULL)
	{
	
		// use the temp room passed in
		room = roomtmp;

		// we should have a room now, but we have had a problem with players joining games
		// and not being able to be given an initial room, so we are going to assert 
		// and display the level, position, DRLG, seed, anything that might help us find
		// this case
		if (room == NULL)
		{
			LEVEL *pLevel = idLevel != INVALID_ID ? LevelGetByID( game, idLevel ) : NULL;
					
			ASSERTV( 
				room, 
				"UnitWarp() is unable to put player '%s' in a room at position (%.2f,%.2f,%.2f) in level '%s'",
				UnitGetDevName( unit ),
				UnitGetPosition( unit ).fX,
				UnitGetPosition( unit ).fY,
				UnitGetPosition( unit ).fZ,
				pLevel ? LevelGetDevName( pLevel ) : "UNKNOWN LEVEL" );

		}
				
	}
		
	// update the room
	UnitUpdateRoom(unit, room, TRUE);

	StateClearAllOfType( unit, STATE_REMOVE_ON_MOVE_OR_WARP );  // we need to do this before attacking anything to remove digin and similar skills

	// do pathing changes
	if (unit->m_pPathing && ( AppIsTugboat() || IS_SERVER(game) ) )
	{
		PathingUnitWarped( unit );
	}
	
	if (IS_SERVER(game))
	{
		if (bSendToClient)
		{
			if( bSendToOwner )
			{
				s_SendUnitWarp( 
					unit, 
					UnitGetRoom(unit), 
					vPosition, 
					UnitGetFaceDirection( unit, FALSE ), 
					UnitGetUpDirection( unit ),
					dwUnitWarpFlags);
			}
			else
			{
				s_SendUnitWarpToOthers( 
					unit, 
					UnitGetRoom(unit), 
					vPosition, 
					UnitGetFaceDirection( unit, FALSE ), 
					UnitGetUpDirection( unit ),
					dwUnitWarpFlags);
			}

			// this will send down a new color index if the unit has changed levels
			if ( TESTBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT ) )
			{
				int nWardrobe = UnitGetWardrobe( unit );
				if ( nWardrobe != INVALID_ID && room && room->pLevel )
				{
					WardrobeUpdateFromInventory ( unit );
				}
				if( AppIsTugboat() )
				{
					UnitForceToGround( unit );
				}
			}
		}
	}
	else if(IS_CLIENT(game))
	{
#if !ISVERSION(SERVER_VERSION)
		BOOL bIsControlUnit = unit == GameGetControlUnit(game);
		if (TESTBIT( dwUnitWarpFlags, UW_FORCE_VISIBLE_BIT ))
		{
			e_ModelSetDrawAndShadow(c_UnitGetModelId(unit), TRUE);

		}
		if (TESTBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT ))
		{
			int nModelID = c_UnitGetModelId(unit);
			e_ModelSetRoomID( nModelID, INVALID_ID );

			if ( UnitTestMode( unit, MODE_JUMP_RECOVERY_TO_RUN_FORWARD ) )
				 UnitEndMode ( unit, MODE_JUMP_RECOVERY_TO_RUN_FORWARD );
			else if ( UnitTestMode( unit, MODE_JUMP_RECOVERY_TO_IDLE ) )
				 UnitEndMode ( unit, MODE_JUMP_RECOVERY_TO_IDLE );
			else if ( UnitTestMode( unit, MODE_JUMP_RECOVERY_TO_RUN_RIGHT ) )
				 UnitEndMode ( unit, MODE_JUMP_RECOVERY_TO_RUN_RIGHT );
			else if ( UnitTestMode( unit, MODE_JUMP_RECOVERY_TO_RUN_LEFT ) )
				 UnitEndMode ( unit, MODE_JUMP_RECOVERY_TO_RUN_LEFT );
			else if ( UnitTestMode( unit, MODE_JUMP_RECOVERY_TO_RUN_BACK ) )
				 UnitEndMode ( unit, MODE_JUMP_RECOVERY_TO_RUN_BACK );
			else
				UnitModeEndJumping( unit, TRUE );

			if( AppIsTugboat() && bIsControlUnit )
			{
				c_CameraSetViewType( c_CameraGetViewType() );
			}
		}

		if (bIsControlUnit)
		{
			if (TESTBIT( dwUnitWarpFlags, UW_PRESERVE_CAMERA_BIT ) == FALSE)
			{
				c_PlayerSetAngles(game, unit->nPlayerAngle);
			}
		}

		if ( bIsControlUnit )
		{
			BOOL bForceUpdate = TESTBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT ); // this helps update the animation group on the model
			c_UnitUpdateViewModel( unit, bForceUpdate );
			if ( bForceUpdate )
				c_WardrobeResetWeaponAttachments( UnitGetWardrobe( unit ) );
			
			//Inform the server of our change in position.
			MSG_CCMD_UNITWARP msgWarp;
			msgWarp.idRoom = RoomGetId(room);
			msgWarp.vPosition = vPosition;
			msgWarp.vFaceDirection = vFaceDirection;
			msgWarp.vUpDirection = vUpDirection;
			msgWarp.dwUnitWarpFlags = dwUnitWarpFlags;
			c_SendMessage(CCMD_UNITWARP, &msgWarp);
		}

#endif
	}

	if ( AppIsHellgate() && TESTBIT( dwUnitWarpFlags, UW_ITEM_DO_FLIPPY_BIT ) )
	{
		ItemFlippy( unit, IS_SERVER( unit ) );
	}

	if(TESTBIT(dwUnitWarpFlags, UW_TRIGGER_EVENT_BIT))
	{
		UnitTriggerEvent(game, EVENT_WARPED_POSITION, unit, NULL, NULL);
	}

#if HELLGATE_ONLY
	if (unit->pHavokRigidBody)
	{
		LEVEL* pLevel = UnitGetLevel( unit );
		HavokRigidBodyWarp( game, pLevel->m_pHavokWorld, unit->pHavokRigidBody, vPosition, UnitGetFaceDirection(unit, FALSE), UnitGetUpDirection(unit) );
	} else {
		// it might not have the rigid body yet and needs it to be created
		sUnitCreateRigidBody( unit );
	}
#endif

	if ( IS_CLIENT(game) )
	{
		c_UnitUpdateGfx(unit);
		c_UnitUpdateFaceTarget( unit, TRUE ); // we don't want to lerp from where we faced before
	}

#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT(game) )
	{
		if ( TESTBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT ) )
		{
			c_WardrobeResetWeaponAttachments( UnitGetWardrobe( unit ) );
		}

		//some weather systems set additional states on the client. So we need to make sure to clear those too...
		StateClearAllOfType(unit, STATE_WEATHERCLIENT);
	}
#endif// !ISVERSION(SERVER_VERSION)

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetQueuedFaceDirection(
	UNIT* unit,
	const VECTOR& vFaceDirection )
{
	if( UnitPathIsActive( unit ) )
	{
		unit->vQueuedFaceDirection = vFaceDirection;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetFaceDirection(
	UNIT* unit,
	const VECTOR& vFaceDirection,
	BOOL bForceDrawnDirection,
	BOOL bCheckUpDirection,
	BOOL bClearZ )
{
	if ( VectorIsZeroEpsilon( vFaceDirection ) )
		return;
	ASSERT_RETURN( unit );
	unit->vFaceDirection = vFaceDirection;
	// flying can face any dir, otherwise only x,y facing... no z!
	if ( bClearZ && ( unit->pdwMovementFlags[0] & MOVEMASK_FLY ) == 0 )
	{
		unit->vFaceDirection.fZ = 0.0f;
		VectorNormalize( unit->vFaceDirection );
	}
	if ( bForceDrawnDirection )
		unit->vFaceDirectionDrawn = vFaceDirection;
	if ( bCheckUpDirection && VectorDot( unit->vFaceDirection, unit->vUpDirection ) != 0  &&
		 unit->vUpDirection.fZ != 0 )
	{
		VECTOR vSide;
		VectorCross( vSide, unit->vFaceDirection, VECTOR( 0, 0, 1 ));
		if ( VectorIsZero( vSide ) )
			vSide = VECTOR( 0, 1, 0 );
		VectorCross( unit->vUpDirection, unit->vFaceDirection, vSide );
		if ( unit->vUpDirection.fZ < 0.0f )
			unit->vUpDirection = - unit->vUpDirection;
		VectorNormalize( unit->vUpDirection, unit->vUpDirection );
	}
	if(AppIsHellgate() && IS_CLIENT(unit))
	{
		c_UnitUpdateGfx(unit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_UnitUpdateFaceDirection(
	UNIT* pUnit,
	float fTimeDelta )
{
	VECTOR vTargetDirection;
	BOOL bDirectionIsSet = FALSE;
	GAME * pGame = UnitGetGame( pUnit);
	REF( pGame );

	if( UnitTestFlag( pUnit, UNITFLAG_SLIDE_TO_DESTINATION ) &&
		!IsUnitDeadOrDying( pUnit ) )
	{
		PathSlideToDestination( pGame, pUnit, fTimeDelta );
	}


	if ( UnitTestFlag( pUnit, UNITFLAG_FACE_SKILL_TARGET ) )
	{
		UNIT * pTarget = SkillGetAnyTarget( pUnit );
		if( AppIsTugboat() && GameGetControlUnit( pGame ) == pUnit )
		{
			pTarget = UIGetClickedTargetUnit();
		}
		BOOL bDead = IsUnitDeadOrDying( pUnit );
	
		if ( pTarget && !bDead  )
		{
			VECTOR vDelta = UnitGetAimPoint( pTarget ) - UnitGetAimPoint( pUnit );
			VectorNormalize( vDelta, vDelta );
			VECTOR vSide;
			VectorCross( vSide, vDelta, UnitGetUpDirection( pUnit ) );
			VectorCross( vTargetDirection, -vSide, UnitGetUpDirection( pUnit ) );
			VectorNormalize( vTargetDirection );
			UnitSetFaceDirection( pUnit, vTargetDirection );
			bDirectionIsSet = TRUE;
		}
	
	}

	if ( AppIsHellgate() && !bDirectionIsSet && UnitTestFlag( pUnit, UNITFLAG_TURN_WITH_MOVEMENT )  )
	{ // when you are strafing or backing up face the correct direction
		vTargetDirection = UnitGetMoveDirection( pUnit );
		VECTOR vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		if ( VectorDot( vFaceDirection, vTargetDirection ) < 0 )
			vTargetDirection = - vTargetDirection;
		bDirectionIsSet = TRUE;
	} 
	if( AppIsTugboat() && pUnit->bDropping &&
		UnitGetRoom( pUnit ) )
	{
		pUnit->fDropPosition += fTimeDelta * 3.0f;
		BOOL bReachedEnd = FALSE;
		if( pUnit->fDropPosition > 1 )
		{

			pUnit->bDropping = FALSE;
			pUnit->fDropPosition = 1;
			const UNIT_DATA* item_data = ItemGetData(UnitGetClass(pUnit));
			if (item_data && item_data->m_nInvPutdownSound != INVALID_ID)
			{
#if !ISVERSION(SERVER_VERSION)
				c_SoundPlay(item_data->m_nInvPutdownSound, &c_UnitGetPosition(pUnit), NULL);
#endif
			}
			UnitSetFlag( pUnit, UNITFLAG_ONGROUND, TRUE );

			bReachedEnd = TRUE;

		}

		GAME * pGame = UnitGetGame( pUnit );
		UnitUpdateRoomFromPosition(pGame, pUnit, NULL, false);

		// manage any room changes as we flip thru the air
		pUnit->vPosition = pUnit->m_pActivePath->GetSplinePositionAtDistance( pUnit->fDropPosition * pUnit->m_pActivePath->Length() );
		pUnit->vDrawnPosition = pUnit->vPosition;

		if( bReachedEnd )
		{
			if( UnitIsA( pUnit, UNITTYPE_MONEY ) )
			{
				c_UnitSetMode( pUnit, MODE_SPAWN );
			}
		}

	}
		
	if ( !bDirectionIsSet ) {
		vTargetDirection = UnitGetFaceDirection( pUnit, FALSE );
		if ( ( pUnit->pdwMovementFlags[0] & MOVEMASK_FLY ) == 0 && !UnitDataTestFlag(UnitGetData(pUnit), UNIT_DATA_FLAG_ANGLE_WHILE_PATHING) && !UnitIsA( pUnit, UNITTYPE_MISSILE ) )
		{
			vTargetDirection.fZ = 0.0f;
			VectorNormalize( vTargetDirection );
		}
		bDirectionIsSet = TRUE;
	}

	if ( AppIsTugboat() )
	{
		// dying units shouldn't spin!
		if( IsUnitDeadOrDying( pUnit ) &&
			!UnitTestMode( pUnit, MODE_IDLE_CHARSELECT ) )
		{
			return;
		}
		// find the angle delta between direction currently facing, 
		// and desired direction
		float CurrentAngle = RadiansToDegrees( ( (float)atan2f( -pUnit->vFaceDirectionDrawn.fX, pUnit->vFaceDirectionDrawn.fY ) ) );
		float TargetAngle  = RadiansToDegrees( ( (float)atan2f( -vTargetDirection.fX, vTargetDirection.fY ) ) );;
		
		TargetAngle -= CurrentAngle;
	
		// make sure this is halved into left/right turns
		if (TargetAngle > 180)
		{
			TargetAngle = TargetAngle - 360;
		}
		if (TargetAngle < -180) 
		{
			TargetAngle = TargetAngle + 360;
		}
		float DegreesToTurn( 0 );
		float fSpinSpeed =  pUnit->pUnitData->fSpinSpeed;
		fSpinSpeed *= 30.0f;
		
		// turn faster the further from the destination angle we are
		if( TargetAngle < 0 )
		{
			float Turn = -fSpinSpeed * fabs( TargetAngle );
			if( Turn < -pUnit->pUnitData->fMaxTurnRate )
			{
				Turn = -pUnit->pUnitData->fMaxTurnRate;
			}
			DegreesToTurn = Turn * fTimeDelta;
			if( DegreesToTurn < TargetAngle )
			{
				DegreesToTurn = TargetAngle;
			}
		}
		else if( TargetAngle > 0 )
		{
			float Turn = fSpinSpeed * fabs( TargetAngle );
			if( Turn > pUnit->pUnitData->fMaxTurnRate )
			{
				Turn = pUnit->pUnitData->fMaxTurnRate;
			}
			DegreesToTurn = Turn * fTimeDelta;
			if( DegreesToTurn > TargetAngle )
			{
				DegreesToTurn = TargetAngle;
			}
	 
		}
	
		DegreesToTurn = (float)DegreesToRadians( DegreesToTurn /** SpeedMultiplier*/ );
		DegreesToTurn = VerifyFloat( DegreesToTurn, DegreesToRadians( 1 ) );
		// perform rotation about the Y axis
		MATRIX zRot;
		VECTOR vRotated;
		MatrixRotationAxis( zRot, VECTOR(0, 0, 1), DegreesToTurn );
		MatrixMultiplyNormal( &pUnit->vFaceDirectionDrawn, &pUnit->vFaceDirectionDrawn, &zRot );
	} 
	else 
	{ // Hellgate version
			
		const UNIT_DATA * pUnitData = UnitGetData(pUnit);
		if(!IsUnitDeadOrDying( pUnit ) && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_TURN_NECK_INSTEAD_OF_BODY))
		{
			float fDot = VectorDot( vTargetDirection, pUnit->vFaceDirectionDrawn );
			int nModelId = c_UnitGetModelId( pUnit );
			BOOL bAggressive =  c_AppearanceIsAggressive( nModelId );
			BOOL bMoving = UnitTestModeGroup( pUnit, MODEGROUP_MOVE );
			BOOL bCanTwist = ! bMoving && c_AppearanceCanTwistNeck( nModelId ) && !SkillIsOnWithFlag( pUnit, SKILL_FLAG_MUST_FACE_FORWARD );
			float fDeltaFromTwist = 0.0f;
			if ( c_CameraGetViewType() == VIEW_CHARACTER_SCREEN )
			{
				pUnit->vFaceDirectionDrawn = vTargetDirection;
			}
			else if ( pUnit == GameGetControlUnit( pGame ) && ! bAggressive && ! bMoving )
			{

			}
			else if ( bCanTwist && c_AppearanceCanFaceCurrentTarget( nModelId, &fDeltaFromTwist ) )
			{
			}
			else if ( fDot > 0.999f )
			{
				pUnit->vFaceDirectionDrawn = vTargetDirection;
			}
			else if ( bCanTwist )
			{
				if ( fDeltaFromTwist < DegreesToRadians(30.0f) )
				{
					// figure out which direction fDeltaDegrees should be.
					MATRIX mRotation;
					MatrixRotationAxis( mRotation, UnitGetUpDirection( pUnit ), fDeltaFromTwist );
					VECTOR vFaceDirectionOld = pUnit->vFaceDirectionDrawn;
					MatrixMultiplyNormal( &pUnit->vFaceDirectionDrawn, &vFaceDirectionOld, &mRotation );

					if ( VectorDot( vFaceDirectionOld, vTargetDirection ) > VectorDot( pUnit->vFaceDirectionDrawn, vTargetDirection ))
					{
						MatrixRotationAxis( mRotation, UnitGetUpDirection( pUnit ), -fDeltaFromTwist );
						MatrixMultiplyNormal( &pUnit->vFaceDirectionDrawn, &vFaceDirectionOld, &mRotation );
					}
				} else {
					pUnit->vFaceDirectionDrawn = vTargetDirection;
				}
			}
			else 
			{
				float fSpinSpeed = c_AppearanceGetTurnSpeed( nModelId );
				BOOL bConstantSpeed = bCanTwist || fSpinSpeed != 0.0f;
				if ( fSpinSpeed == 0.0f )
				{
					int nAppearanceDef = c_AppearanceGetDefinition( nModelId );
					if ( nAppearanceDef != INVALID_ID )
					{
						fSpinSpeed = AppearanceDefinitionGetSpinSpeed(nAppearanceDef);
					}
				}
				if ( fDot < -0.99f ) 
				{// are they nearly opposite?
					VECTOR vSide;
					VectorCross( vSide, vTargetDirection, pUnit->vUpDirection );
					pUnit->vFaceDirectionDrawn = VectorLerp( vSide, pUnit->vFaceDirectionDrawn, fSpinSpeed );
				}
				else
				{
					if ( ! bConstantSpeed )
					{
						pUnit->vFaceDirectionDrawn = VectorLerp( vTargetDirection, pUnit->vFaceDirectionDrawn, fSpinSpeed );
					} else {
						float fDirectionDot = VectorDot( vTargetDirection, pUnit->vFaceDirectionDrawn );
						float fAngle = acos( PIN( fDirectionDot, -1.0f, 1.0f ) );
						ASSERT_RETURN( fAngle != 0.0f );
						float fFraction = fTimeDelta * fSpinSpeed / fAngle;
	
						if ( fFraction > 1.0f )
							pUnit->vFaceDirectionDrawn = vTargetDirection;
						else
							pUnit->vFaceDirectionDrawn = VectorLerp( vTargetDirection, pUnit->vFaceDirectionDrawn, fFraction );
	
					}
	
				}
				VectorNormalize( pUnit->vFaceDirectionDrawn, pUnit->vFaceDirectionDrawn);
			}
		}
	}
}
#endif //!SERVER_VERSION

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetVisible(
	UNIT* unit,
	BOOL bValue)
{
	ASSERTX_RETURN( 0, "Deprecated!" );

	ASSERT_RETURN(unit);
	ASSERT_RETURN(IS_CLIENT(unit));
	if (unit->pGfx)
	{
		int nModelId = c_UnitGetModelId( unit );
		if ( bValue )
			e_ModelUpdateVisibilityToken( nModelId );
	}
	if (bValue)
	{
		c_UnitUpdateGfx(unit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUnitFadeModelUpdate(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nModelId = (int) event_data.m_Data1;
	BOOL bFadeIn = (BOOL) event_data.m_Data2;
	float fRate = EventParamToFloat( event_data.m_Data3 );

	float fOverride = UnitGetStatFloat( unit, STATS_OPACITY_OVERRIDE );

	float fAlphaCurr = e_ModelGetAlpha( nModelId );
	fAlphaCurr += (bFadeIn ? fRate : -fRate );
	fAlphaCurr = PIN( fAlphaCurr, 0.0f, 1.0f );

	{
		float fAlphaToSet = fOverride != 0.0f ? PIN( fAlphaCurr, 0.0f, fOverride ) : fAlphaCurr;
		V( e_ModelSetAlpha( nModelId, fAlphaToSet ) );

		if( AppIsTugboat() )
		{
			int nWardrobe = UnitGetWardrobe( unit );
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( game, nWardrobe, i );

					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					V( e_ModelSetAlpha( nWeaponModelId, fAlphaToSet ) );
				}
				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				if( pClient )
				{
					for( int i = 0; i < pClient->nAttachmentCount; i++ )
					{
						ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
						//e_ModelSetAmbientLight(  pAttachment->nId, Ambient, 1000 );
						if( pAttachment && pAttachment->eType == ATTACHMENT_MODEL )
						{
							V( e_ModelSetAlpha( pAttachment->nAttachedId, fAlphaToSet ) );
						}				
					}
				}
			}
		}
	}

	if ( !bFadeIn && fAlphaCurr == 0.0f )
	{
		// This doesn't store the Alpha, so if you fade out, but DON'T fade in, then the alpha will still be zero
		//V( e_ModelSetAlpha( nModelId, 1.0f ) );
		if( AppIsTugboat() )
		{
			e_ModelSetDrawTemporary( nModelId, FALSE );
			int nWardrobe = UnitGetWardrobe( unit );
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( game, nWardrobe, i );

					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					e_ModelSetDrawTemporary( nWeaponModelId, FALSE );
				}
				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				if( pClient )
				{
					for( int i = 0; i < pClient->nAttachmentCount; i++ )
					{
						ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
						//e_ModelSetAmbientLight(  pAttachment->nId, Ambient, 1000 );
						if( pAttachment && pAttachment->eType == ATTACHMENT_MODEL )
						{
							e_ModelSetDrawTemporary( pAttachment->nAttachedId, FALSE );
						}				
					}
				}
			}
		}
		else
			e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, TRUE );
		e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND, TRUE );
		return TRUE;
	}

	if ( bFadeIn && fAlphaCurr == 1.0f )
		return TRUE;

	UnitRegisterEventTimed( unit, sUnitFadeModelUpdate, event_data, 1 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitFadeModel(
	UNIT* pUnit,
	int nModelId,
	BOOL bFadeIn,
	float fFadeTime )
{
	if ( ! e_ModelExists( nModelId ) )
		return;

	BOOL bIsNoDraw    = AppIsTugboat() ? e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY ) : e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW );
	BOOL bIsMuteSound = e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND );
	if ( ( bIsNoDraw || bIsMuteSound ) && bFadeIn )
	{
		if( AppIsTugboat() )
		{
			e_ModelSetDrawTemporary( nModelId, TRUE );
			int nWardrobe = UnitGetWardrobe( pUnit );
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( UnitGetGame( pUnit ), nWardrobe, i );

					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					e_ModelSetDrawTemporary( nWeaponModelId, TRUE );
				}
				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				if( pClient )
				{
					for( int i = 0; i < pClient->nAttachmentCount; i++ )
					{
						ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
						if( pAttachment && pAttachment->eType == ATTACHMENT_MODEL )
						{
							e_ModelSetDrawTemporary( pAttachment->nAttachedId, TRUE );
						}				
					}
				}
			}
		}
		else
			e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, FALSE );
		V( e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND, FALSE ) );
		V( e_ModelSetAlpha( nModelId, 0.0f ) );

		if( AppIsTugboat() )
		{
			int nWardrobe = UnitGetWardrobe( pUnit );
			if( nWardrobe != INVALID_ID )
			{

				for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
				{
					UNIT *  pWeapon = WardrobeGetWeapon( UnitGetGame( pUnit ), nWardrobe, i );

					if ( ! pWeapon )
						continue;
					int nWeaponModelId = c_UnitGetModelId(pWeapon);
					V( e_ModelSetAlpha( nWeaponModelId, 0 ) );
				}
				WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
				if( pClient )
				{
					for( int i = 0; i < pClient->nAttachmentCount; i++ )
					{
						ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
						//e_ModelSetAmbientLight(  pAttachment->nId, Ambient, 1000 );
						if( pAttachment && pAttachment->eType == ATTACHMENT_MODEL )
						{
							V( e_ModelSetAlpha( pAttachment->nAttachedId, 0 ) );
						}				
					}
				}
			}
		}
	}
	//else if ( e_ModelGetAlpha( nModelId ) == (bFadeIn ? 1.0f : 0.0f) )
	//{
	//	e_ModelSetAlpha( nModelId, (bFadeIn ? 0.0f : 1.0f ) );
	//}

	UnitUnregisterTimedEvent( pUnit, sUnitFadeModelUpdate, CheckEventParam1, EVENT_DATA( nModelId ) );
	float fRate = fFadeTime ? 1.0f / (fFadeTime * GAME_TICKS_PER_SECOND_FLOAT) : 0.0f;
	UnitRegisterEventTimed( pUnit, sUnitFadeModelUpdate, EVENT_DATA( nModelId, bFadeIn, EventFloatToParam( fRate )), 1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sUnitModelSetNoDraw(
	UNIT* unit,
	int nModelId,
	BOOL bValue,
	float fFadeTime,
	BOOL bTemporary = FALSE )
{
	if ( AppIsHellgate() )
	{
		if( fFadeTime != 0.0f )
		{
			c_UnitFadeModel( unit, nModelId, !bValue, fFadeTime );
		}
		else
		{
			e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, bValue );
			e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND, bValue );
		}
	}
	else
	{
		if( fFadeTime != 0 )
		{
			c_UnitFadeModel( unit, nModelId, !bValue, fFadeTime );
		}
		else
		{

			if( AppIsTugboat() )
			{
				float fAlphaToSet = bValue ? 0.0f : 1.0f;
				V( e_ModelSetAlpha( nModelId, fAlphaToSet ) );
				int nWardrobe = UnitGetWardrobe( unit );
				if( nWardrobe != INVALID_ID )
				{

					for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
					{
						UNIT *  pWeapon = WardrobeGetWeapon( UnitGetGame( unit ), nWardrobe, i );

						if ( ! pWeapon )
							continue;
						int nWeaponModelId = c_UnitGetModelId(pWeapon);
						V( e_ModelSetAlpha( nWeaponModelId, fAlphaToSet ) );
					}
					WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
					if( pClient )
					{
						for( int i = 0; i < pClient->nAttachmentCount; i++ )
						{
							ATTACHMENT* pAttachment = c_ModelAttachmentGet( nModelId, pClient->pnAttachmentIds[ i ] );
							//e_ModelSetAmbientLight(  pAttachment->nId, Ambient, 1000 );
							if( pAttachment && pAttachment->eType == ATTACHMENT_MODEL )
							{
								V( e_ModelSetAlpha( pAttachment->nAttachedId, fAlphaToSet ) );
							}				
						}
					}
				}
			}

			if( bTemporary )
			{
				e_ModelSetDrawTemporary( nModelId, !bValue );
			}
			else
			{
				e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, bValue );
				e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_MUTESOUND, bValue );
			}
		}
	}
	
}
//---------------------------------------------------------------------------
BOOL c_UnitGetNoDraw( UNIT* unit )
{
	int nModelId = c_UnitGetModelId(unit);
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		nModelId = c_UnitGetModelIdThirdPersonOverride( unit );
	if( nModelId != INVALID_ID )
	{
		return e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW );
	}
	return TRUE;
}

//---------------------------------------------------------------------------
BOOL c_UnitGetNoDrawTemporary( UNIT* unit )
{
	int nModelId = c_UnitGetModelId(unit);
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		nModelId = c_UnitGetModelIdThirdPersonOverride( unit );
	if( nModelId != INVALID_ID )
	{
		return e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitSetNoDraw(
	UNIT* unit,
	BOOL bValue,
	BOOL bAllModels, /*= FALSE*/ 
	float fFadeTime /*= 0.0f*/,
	BOOL bTemporary /*= FALSE*/ )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(unit);
	if (unit->pGfx)
	{
		if ( bAllModels )
		{
			sUnitModelSetNoDraw( unit, c_UnitGetModelIdInventory( unit ),	bValue, fFadeTime, bTemporary );
			sUnitModelSetNoDraw( unit, c_UnitGetModelIdFirstPerson( unit ), bValue, fFadeTime, bTemporary );
			sUnitModelSetNoDraw( unit, c_UnitGetModelIdThirdPerson( unit ), bValue, fFadeTime, bTemporary );
			sUnitModelSetNoDraw( unit, c_UnitGetPaperdollModelId( unit ),	bValue, fFadeTime, bTemporary );
			sUnitModelSetNoDraw( unit, c_UnitGetPaperdollModelId( unit ),	bValue, fFadeTime, bTemporary );
		} 
		else
		{
			sUnitModelSetNoDraw( unit, c_UnitGetModelId( unit ),			bValue, fFadeTime, bTemporary );
		}
	}
	
	// handle visual portals
	if (ObjectIsVisualPortal( unit ))
	{
		const ROOM_VISUAL_PORTAL *pVisualPortal = RoomVisualPortalFind( unit );
		if (pVisualPortal->nEnginePortalID != INVALID_ID)
		{
			e_PortalSetFlags( pVisualPortal->nEnginePortalID, PORTAL_FLAG_NODRAW, bValue );
		}
	}
	
#else
	UNREFERENCED_PARAMETER(unit);
	UNREFERENCED_PARAMETER(bValue);
	UNREFERENCED_PARAMETER(bAllModels);
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitSetVisibleAll(
	GAME* game,
	BOOL bValue)
{
	ASSERTX_RETURN( 0, "Deprecated!" );

	ASSERT_RETURN(game);
	ASSERT(IS_CLIENT(game));

#if !ISVERSION(SERVER_VERSION)
	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT* unit = game->pUnitHash->pHash[ii];
		while (unit)
		{
			UnitSetVisible(unit, bValue);
			unit = unit->hashnext;
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitUpdateDrawAll(
	GAME* pGame,
	float fTimeDelta )
{
	ASSERT(pGame);
	ASSERT(IS_CLIENT(pGame));
#if !ISVERSION(SERVER_VERSION)
	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT * pUnit = pGame->pUnitHash->pHash[ii];
		while ( pUnit )
		{
			int nModelId = c_UnitGetModelId( pUnit );
			REF(nModelId);
			if(  AppIsHellgate() || ( UnitGetRoom( pUnit ) && e_ModelCurrentVisibilityToken( nModelId )  ) )
			{
				c_UnitUpdateFaceDirection( pUnit, fTimeDelta );
				c_UnitUpdateGfx( pUnit );
			}
			pUnit = pUnit->hashnext;
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_UnitKillAll(
	GAME* game,
	LEVEL * pLevel,
	BOOL bFreeUnits )
{
	ASSERT_RETURN(game);
	ASSERT(IS_SERVER(game));

	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT* unit = game->pUnitHash->pHash[ii];
		while (unit)
		{
			UNIT * pNext = unit->hashnext;
			if (UnitGetGenus(unit) == GENUS_MONSTER && 
				UnitGetTargetType(unit) == TARGET_BAD && 
				!IsUnitDeadOrDying(unit) && 
				(!pLevel || UnitGetLevel(unit) == pLevel))
			{
				UnitDie(unit, NULL);
				if ( bFreeUnits )
				{
					UnitFree( unit, UFF_SEND_TO_CLIENTS );
				}
			}
			unit = pNext;
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetDefaultTargetType(
	GAME * game,
	UNIT * unit,
	const struct UNIT_DATA * unit_data /* = NULL */)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	switch (UnitGetGenus(unit))
	{
	case GENUS_PLAYER:
		UnitChangeTargetType(unit, TARGET_GOOD);
		break;

	case GENUS_MONSTER:
		{
			if (!unit_data)
			{
				unit_data = MonsterGetData(game, UnitGetClass(unit));
			}
			ASSERT_RETURN(unit_data);

			UNIT * owner = UnitGetOwnerUnit(unit);
			if (owner != unit)
			{
				UnitChangeTargetType(unit, UnitGetTargetType(owner));
				break;
			}

			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_IS_GOOD))
			{
				UnitChangeTargetType(unit, TARGET_GOOD);
			}
			else
			{
				UnitChangeTargetType(unit, TARGET_BAD);
			}
		}
		break;

	case GENUS_ITEM:
		UnitChangeTargetType(unit, TARGET_OBJECT);
		break;

	case GENUS_OBJECT:
		{
			if (!unit_data)
			{
				unit_data = ObjectGetData(game, unit);
			}
			ASSERT_RETURN(unit_data);
			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_DESTRUCTIBLE))
			{
				UnitChangeTargetType(unit, TARGET_BAD);
			}
			else
			{
				UnitChangeTargetType(unit, TARGET_OBJECT);
			}
		}
		break;

	case GENUS_MISSILE:
		UnitChangeTargetType(unit, TARGET_MISSILE);
		break;

	default:
		ASSERT(0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitResurrect(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game && unit);
	ASSERT_RETURN(UnitTestFlag(unit, UNITFLAG_PLAYER_WAITING_FOR_RESPAWN) == FALSE);

	UnitClearFlag(unit, UNITFLAG_DEAD_OR_DYING);
	UnitSetDefaultTargetType(game, unit);
	//Marsh added - Not sure if we need to check for monster here or not.
	if( UnitIsA(unit, UNITTYPE_MONSTER) )
	{
		// a respawner has already been created - don't stack up the respawns!
		UnitClearFlag( unit, UNITFLAG_SHOULD_RESPAWN );
		if( !unit->m_pPathing ) //Pathing is removed on monsters via their death
		{
			unit->m_pPathing = PathStructInit(game);
			if ( UnitGetRoom( unit ) )
				PathingUnitWarped( unit ); //does some nice checking to make sure we are in a good spot. Not sure if this is over kill.
		}
		UnitEndMode( unit, MODE_DEAD );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearHealing(
	UNIT *pUnit)
{				

	// clear state		
	StateClearAllOfType( pUnit, STATE_HEALING );
	
	// clear stat for points left
	UnitClearStat( pUnit, STATS_HEALING_POINTS_LEFT, 0 );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearPowering(
	UNIT *pUnit)
{				
	
	// clear state		
	s_StateClear( pUnit, UnitGetId( pUnit ), STATE_POWERING, 0 );
	
	// clear stat for points left
	UnitClearStat( pUnit, STATS_POWERING_POINTS_LEFT, 0 );	
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeHpCur(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT(wStat == STATS_HP_CUR);
	if (oldvalue <= 0 && newvalue > 0 && IsUnitDeadOrDying( unit ) )
	{
	
		// BAND AID: we are having a problem with characters very rarely not being able to
		// hit the buttons to come back to life (as ghost, or resurrect, etc)
		// because when they hit the button it does nothing.  So, one of the places
		// this can appear to fail is because their health was regenerated for some
		// reason, and they are no longer considered "dead" by the server.  So we're
		// gonna base the respawn logic off this simple flag that gets set when they
		// die and cleared only when they respawn.  Also, to prevent a player from 
		// resurrecting when they're note supposed to we check it here 
		// too before we resurrect
		BOOL bWaitingForRespawn = UnitTestFlag(unit, UNITFLAG_PLAYER_WAITING_FOR_RESPAWN);
		if (bWaitingForRespawn == FALSE)
		{
			sUnitResurrect(UnitGetGame(unit), unit);
		}	

	}


	int hp_max = UnitGetStat(unit, STATS_HP_MAX);

	if (IS_SERVER(UnitGetGame(unit)))
	{
		// check for healing
		if (newvalue > oldvalue)
		{
		
			// do we have limited healing points from using a health pack
			int nHealingPointsLeft = UnitGetStat( unit, STATS_HEALING_POINTS_LEFT );
			if (nHealingPointsLeft > 0)
			{
				int nHealedAmount = newvalue - oldvalue;
				
				// take the points away
				nHealingPointsLeft -= nHealedAmount;
				if (nHealingPointsLeft <= 0)
				{
					sClearHealing( unit );
				}
				else
				{
					UnitSetStat( unit, STATS_HEALING_POINTS_LEFT, nHealingPointsLeft );
				}

			}
						
			// check for reached max health			
			if (newvalue == hp_max)
			{
				sClearHealing( unit );
				UnitTriggerEvent( UnitGetGame( unit ), EVENT_FULLY_HEALED, unit, NULL, NULL );
			}
			
		}
	}

	if ( IS_CLIENT( unit ))
	{
		if (newvalue < hp_max/2)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_HALF_HEALTH))
			{
				c_StateSet(unit, unit, STATE_HALF_HEALTH, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_HALF_HEALTH, 0);
		}
		if (newvalue < hp_max/4)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_QUARTER_HEALTH))
			{
				c_StateSet(unit, unit, STATE_QUARTER_HEALTH, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_QUARTER_HEALTH, 0);
		}
		if (newvalue < hp_max/8)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_EIGHTH_HEALTH))
			{
				c_StateSet(unit, unit, STATE_EIGHTH_HEALTH, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_EIGHTH_HEALTH, 0);
		}
		if (newvalue < hp_max/16)
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_LOW_HEALTH))
			{
				c_StateSet(unit, unit, STATE_LOW_HEALTH, 0, 0);
			}
		}
		else
		{
			c_StateClear(unit, UnitGetId(unit), STATE_LOW_HEALTH, 0);
		}
	}

	if (UnitIsA(unit, UNITTYPE_PLAYER) && newvalue < oldvalue)
	{
		UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, TAG_SELECTOR_HP_LOST);
		if (pTag)
		{
			pTag->m_nCounts[pTag->m_nCurrent] += oldvalue - newvalue;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangePowerCur(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	if (IS_SERVER(pUnit))
	{
		int nPowerMax = UnitGetStat( pUnit, STATS_POWER_MAX );
		
		// check for powering
		if (nValueNew > nValueOld)
		{
		
			// do we have limited powering points from using a pack
			int nPoweringPointsLeft = UnitGetStat( pUnit, STATS_POWERING_POINTS_LEFT );
			if (nPoweringPointsLeft > 0)
			{
				int nPoweredAmount = nValueNew - nValueOld;
				
				// take the points away
				nPoweringPointsLeft -= nPoweredAmount;
				if (nPoweringPointsLeft <= 0)
				{
					sClearPowering( pUnit );
				}
				else
				{
					UnitSetStat( pUnit, STATS_POWERING_POINTS_LEFT, nPoweringPointsLeft );
				}

			}
						
			// check for reached max health			
			if (nValueNew == nPowerMax)
			{
				sClearPowering( pUnit );
				UnitTriggerEvent( UnitGetGame( pUnit ), EVENT_FULLY_POWERED, pUnit, NULL, NULL );
			}
		
		}
		
	}

#if ISVERSION(DEVELOPMENT)
	if(GameGetDebugFlag(UnitGetGame(pUnit), DEBUGFLAG_INFINITE_POWER))
	{
		int nPowerMax = UnitGetStat( pUnit, STATS_POWER_MAX );
		if(nValueNew < nPowerMax)
		{
			UnitSetStat(pUnit, STATS_POWER_CUR, nPowerMax);
		}
	}
#endif
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeKarmaCur(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT(wStat == STATS_KARMA);
	sUnitCheckKarma( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUpdateStats(
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	ASSERT_RETFALSE( unit );
	UnitUnregisterTimedEvent( unit, sUpdateStats );	
	if (IS_CLIENT( pGame ))
	{
		if (GameGetControlUnit( pGame ) == unit ||
			UnitIsA( unit, UNITTYPE_HIRELING ) )
		{
			UnitCalculateVisibleDamage( unit );
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeStrength(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN( AppIsTugboat() );
	UnitRegisterEventTimed( pUnit, sUpdateStats, NULL, 10 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeDexterity(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN( AppIsTugboat() );
	UnitRegisterEventTimed( pUnit, sUpdateStats, NULL, 10 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeDamage(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN( AppIsTugboat() );
	UnitRegisterEventTimed( pUnit, sUpdateStats, NULL, 10 );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeTeamOverride(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	int nTeamOverrideIndex = UnitGetStat( pUnit, STATS_TEAM_OVERRIDE_INDEX );
	pUnit->nTeamOverride = (nTeamOverrideIndex >= 0) ?  UnitGetStat( pUnit, STATS_TEAM_OVERRIDE, nTeamOverrideIndex, 0 ) : INVALID_TEAM;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangePlayerWaitingForRespawn(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA &data,
	BOOL bSend)
{
	UnitSetFlag( pUnit, UNITFLAG_PLAYER_WAITING_FOR_RESPAWN, nValueNew );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeSkillLevel(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	GAME * game = UnitGetGame(unit);
	int nSkill = StatGetParam( wStat, dwParam, 0 );
	if (nSkill == INVALID_ID)
	{
		if ( newvalue )
		{// clear out invalid stats.
			UnitSetStat( unit, wStat, nSkill, 0 );
		}
		return;
	}

	SkillLevelChanged( unit, nSkill, oldvalue != 0 );

	const SKILL_DATA * skill_data = SkillGetData(game, nSkill);
	ASSERT_RETURN(skill_data);

	for ( int i = 0; i < MAX_LINKED_SKILLS; i++ ) 
	{
		if ( skill_data->pnLinkedLevelSkill[ i ] != INVALID_ID )
		{
			BOOL bHadSkill = UnitGetStat( unit, STATS_SKILL_LEVEL, skill_data->pnLinkedLevelSkill[ i ] ) != 0;
			SkillLevelChanged( unit, nSkill, bHadSkill );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeSkillLevelGroup(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	GAME * game = UnitGetGame(unit);
	int nSkillGroup = StatGetParam( wStat, dwParam, 0 );
	if (nSkillGroup == INVALID_ID)
	{
		if ( newvalue )
		{
			// clear out invalid stats.
			UnitSetStat( unit, wStat, nSkillGroup, 0 );
		}
		return;
	}

	UNIT_ITERATE_STATS_RANGE( unit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = StatGetParam(STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0);
		if (nSkill == INVALID_ID)
		{
			continue;
		}

		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );
		if (!pSkillData)
		{
			continue;
		}

		for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
		{
			if ( pSkillData->pnSkillGroup[ i ] == nSkillGroup )
				SkillLevelChanged( unit, nSkill, TRUE );
		}
	}
	UNIT_ITERATE_STATS_END;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeRealWorldMoney(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

#if !ISVERSION(SERVER_VERSION)
	GAME *pGame = UnitGetGame( pUnit );
	if (IS_CLIENT( pGame ))
	{
		if (GameGetControlUnit( pGame ) == pUnit)
		{
			cCurrency currencyPlayer = UnitGetCurrency( pUnit );
			UIMoneySet( pGame, pUnit, cCurrency( currencyPlayer.GetValue( KCURRENCY_VALUE_INGAME ),  nNewValue ) );
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeGold(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

#if !ISVERSION(SERVER_VERSION)
	GAME *pGame = UnitGetGame( pUnit );
	if (IS_CLIENT( pGame ))
	{
		if (GameGetControlUnit( pGame ) == pUnit)
		{
			cCurrency currencyPlayer = UnitGetCurrency( pUnit );			
			UIMoneySet( pGame, pUnit, cCurrency( nNewValue, currencyPlayer.GetValue( KCURRENCY_VALUE_REALWORLD ) ) );
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangePower(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	if (nNewValue > 0)
	{
		return;
	}

	if ( SkillIsOnWithFlag( pUnit, SKILL_FLAG_DRAINS_POWER ) )
		SkillUpdateActiveSkills( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangePetSlots(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

#if !ISVERSION(SERVER_VERSION)
	GAME *pGame = UnitGetGame( pUnit );
	if (IS_CLIENT( pGame ))
	{
		if (nOldValue != nNewValue)
		{
			UNIT *pLocalPlayer = GameGetControlUnit( pGame );
			if (pLocalPlayer == pUnit)
			{
				UIRepaint();
			}
		}
	}
#endif //!ISVERSION(SERVER_VERSION)

	if(nNewValue > nOldValue)
	{
		return;
	}

	int nInventoryLocation = dwParam;
	BOOL bFreeUnits = UnitInventoryTestFlag(pUnit, nInventoryLocation, INVLOCFLAG_FREE_ON_SIZE_CHANGE);
	UNIT * pPet = UnitInventoryLocationIterate(pUnit, nInventoryLocation, NULL);
	UNIT * pNext = NULL;
	int nPetCount = 0;
	while(pPet)
	{
		pNext = UnitInventoryLocationIterate( pUnit, nInventoryLocation, pPet );
		nPetCount++;
		if(nPetCount > nNewValue)
		{
			if(bFreeUnits)
			{
				UnitFree(pPet, UFF_SEND_TO_CLIENTS);
			}
			else
			{
				UnitDie(pPet, NULL);
			}
		}
		pPet = pNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangePetPowerCostReductionPct(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	if (IS_SERVER(pUnit))
	{
		PetsReapplyPowerCostToOwner(pUnit);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangePreventAllSkills(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	if ( nNewValue && ! nOldValue )
	{
		SkillStopAll( UnitGetGame( pUnit ), pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChanceToFizzleMissiles(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT(wStat == STATS_CHANCE_TO_FIZZLE_MISSILES);
	if ( ! UnitHasTimedEvent( unit, MissilesFizzle ) )
	{
		MissilesFizzle( UnitGetGame( unit ), unit, EVENT_DATA() );
	} 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChanceToReflectMissiles(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT(wStat == STATS_CHANCE_TO_REFLECT_MISSILES);
	if ( ! UnitHasTimedEvent( unit, MissilesReflect ) )
	{
		MissilesReflect( UnitGetGame( unit ), unit, EVENT_DATA(TRUE) );
	} 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeMaxRatio(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	if (IS_CLIENT(game) && UnitTestFlag(unit, UNITFLAG_JUSTCREATED))
	{
		return;
	}
	const STATS_DATA * max_stat_data = StatsGetData(game, wStat);
	ASSERT_RETURN(max_stat_data && max_stat_data->m_nAssociatedStat[0] > INVALID_LINK);
	int cur_stat = max_stat_data->m_nAssociatedStat[0];

	// if the current stat is really a current stat, we don't change it if you're dead
	const STATS_DATA* cur_stat_data = StatsGetData(game, cur_stat);
	if (StatsDataTestFlag(cur_stat_data, STATS_DATA_NO_REACT_WHEN_DEAD) && IsUnitDeadOrDying(unit))
	{
		return;
	}
	
	int cur = UnitGetStatDontApplyMinMax(unit, cur_stat);

	if (!UnitTestFlag(unit, UNITFLAG_IGNORE_STAT_MAX_FOR_XFER) ||
		!(StatsDataTestFlag(cur_stat_data, STATS_DATA_SEND_TO_ALL) || StatsDataTestFlag(cur_stat_data, STATS_DATA_SEND_TO_SELF)))
	{
		if (oldvalue == 0 && newvalue > 0)
		{
			if (bSend)
			{
				UnitSetStat(unit, cur_stat, newvalue);
			}
			else
			{
				UnitSetStatNoSend(unit, cur_stat, newvalue);
			}
		}
		else if (oldvalue > 0 && newvalue >= 0)
		{
			if (cur != 0)
			{
				float ratio = (float)cur / (float)oldvalue;
				int val = (int)(ratio * (float)newvalue);
				if (bSend)
				{
					UnitSetStat(unit, cur_stat, val);
				}
				else
				{
					UnitSetStatNoSend(unit, cur_stat, val);
				}
			}
		}
	}

	if (cur_stat_data->m_nSpecialFunction == STATSSPEC_REGEN_ON_GET ||
		cur_stat_data->m_nSpecialFunction == STATSSPEC_PCT_REGEN_ON_GET)
	{
		// start the cur stat regenerating
		StatsChangeStatSpecialFunctionSetRegenOnGetStat(UnitGetGame(unit), unit, cur_stat, dwParam, cur_stat_data, cur, cur, newvalue);

		// We want to make sure that the cur stat is recalculated whenever you set the max stat, so force it to recalc now.
		// We don't care what the value is - we just care that it gets recalculated, and the easiest way to do that is UnitGetStat()
		UnitGetStat(unit, cur_stat, dwParam);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeMaxRatioDecrease(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	if (IS_CLIENT(game) && UnitTestFlag(unit, UNITFLAG_JUSTCREATED))
	{
		return;
	}
	const STATS_DATA * max_stat_data = StatsGetData(game, wStat);
	ASSERT_RETURN(max_stat_data && max_stat_data->m_nAssociatedStat[0] > INVALID_LINK);
	int cur_stat = max_stat_data->m_nAssociatedStat[0];

	// if the current stat is really a current stat, we don't change it if you're dead
	const STATS_DATA* cur_stat_data = StatsGetData(game, cur_stat);
	if (StatsDataTestFlag(cur_stat_data, STATS_DATA_NO_REACT_WHEN_DEAD) && IsUnitDeadOrDying(unit))
	{
		return;
	}

	int cur = UnitGetStatDontApplyMinMax(unit, cur_stat);

	if (!UnitTestFlag(unit, UNITFLAG_IGNORE_STAT_MAX_FOR_XFER) ||
		!(StatsDataTestFlag(cur_stat_data, STATS_DATA_SEND_TO_ALL) || StatsDataTestFlag(cur_stat_data, STATS_DATA_SEND_TO_SELF)))
	{
		if (oldvalue == 0 && newvalue > 0)
		{
			if (bSend)
			{
				UnitSetStat(unit, cur_stat, newvalue);
			}
			else
			{
				UnitSetStatNoSend(unit, cur_stat, newvalue);
			}
		}
		else if (oldvalue > 0 && newvalue < oldvalue && newvalue >= 0)
		{
			if (cur != 0)
			{
				float ratio = (float)cur / (float)oldvalue;
				int val = (int)(ratio * (float)newvalue);
				if (bSend)
				{
					UnitSetStat(unit, cur_stat, val);
				}
				else
				{
					UnitSetStatNoSend(unit, cur_stat, val);
				}
			}
		}
		else if (oldvalue > 0 && newvalue >= oldvalue)
		{
			int val = cur + newvalue - oldvalue;
			if (bSend)
			{
				UnitSetStat(unit, cur_stat, val);
			}
			else
			{
				UnitSetStatNoSend(unit, cur_stat, val);
			}
		}
	}

	if (cur_stat_data->m_nSpecialFunction == STATSSPEC_REGEN_ON_GET ||
		cur_stat_data->m_nSpecialFunction == STATSSPEC_PCT_REGEN_ON_GET)
	{
		// start the cur stat regenerating
		StatsChangeStatSpecialFunctionSetRegenOnGetStat(UnitGetGame(unit), unit, cur_stat, dwParam, cur_stat_data, cur, cur, newvalue);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeMax(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN(unit);
	STATS_DATA* max_stat_data = (STATS_DATA*)ExcelGetData(UnitGetGame(unit), DATATABLE_STATS, wStat);
	ASSERT_RETURN(max_stat_data && max_stat_data->m_nAssociatedStat[0] > INVALID_LINK);
	int cur_stat = max_stat_data->m_nAssociatedStat[0];

	int cur = UnitGetStatDontApplyMinMax(unit, cur_stat);
	if (oldvalue <= 0 && newvalue > 0)
	{
		if (bSend)
		{
			UnitSetStat(unit, cur_stat, newvalue);
		}
		else
		{
			UnitSetStatNoSend(unit, cur_stat, newvalue);
		}
	}
	else if (oldvalue > 0 && newvalue >= 0)
	{
		if (cur != 0)
		{
			if (cur > newvalue)
			{
				if (bSend)
				{
					UnitSetStat(unit, cur_stat, newvalue);
				}
				else
				{
					UnitSetStatNoSend(unit, cur_stat, newvalue);
				}
			}
		}
	}

	const STATS_DATA * cur_stat_data = StatsGetData(UnitGetGame(unit), cur_stat);
	ASSERT_RETURN(cur_stat_data);

	if (cur_stat_data->m_nSpecialFunction == STATSSPEC_REGEN_ON_GET ||
		cur_stat_data->m_nSpecialFunction == STATSSPEC_PCT_REGEN_ON_GET)
	{
		// start the cur stat regenerating
		StatsChangeStatSpecialFunctionSetRegenOnGetStat(UnitGetGame(unit), unit, cur_stat, dwParam, cur_stat_data, cur, cur, newvalue);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeMaxPure(
	UNIT* unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	ASSERT_RETURN(unit);
	STATS_DATA* max_stat_data = (STATS_DATA*)ExcelGetData(UnitGetGame(unit), DATATABLE_STATS, wStat);
	ASSERT_RETURN(max_stat_data && max_stat_data->m_nAssociatedStat[0] > INVALID_LINK);
	int cur_stat = max_stat_data->m_nAssociatedStat[0];

	int cur = UnitGetStatDontApplyMinMax(unit, cur_stat);
	if (cur != 0)
	{
		if (cur > newvalue)
		{
			if (bSend)
			{
				UnitSetStat(unit, cur_stat, newvalue);
			}
			else
			{
				UnitSetStatNoSend(unit, cur_stat, newvalue);
			}
		}
	}

	STATS_DATA * cur_stat_data = (STATS_DATA *)ExcelGetData(UnitGetGame(unit), DATATABLE_STATS, cur_stat);
	ASSERT_RETURN(cur_stat_data);

	if (cur_stat_data->m_nSpecialFunction == STATSSPEC_REGEN_ON_GET ||
		cur_stat_data->m_nSpecialFunction == STATSSPEC_PCT_REGEN_ON_GET)
	{
		// start the cur stat regenerating
		StatsChangeStatSpecialFunctionSetRegenOnGetStat(UnitGetGame(unit), unit, cur_stat, dwParam, cur_stat_data, cur, cur, newvalue);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeVelocity(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(wStat);
	REF(dwParam);
	REF(oldvalue);
	REF(newvalue);
	REF(data);
	REF(bSend);
	UnitCalcVelocity(unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeSkillIsSelected(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
#if ISVERSION(SERVER_VERSION)
	REF(unit);
	REF(wStat);
	REF(dwParam);
	REF(oldvalue);
	REF(newvalue);
	REF(data);
	REF(bSend);
#else
	if (!IS_CLIENT(unit))
	{
		return;
	}
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game);
	int nSkill = StatGetParam( wStat, dwParam, 0 );

	if ( oldvalue && !newvalue )
	{
		SkillDeselect(game, unit, nSkill, TRUE );
	} 
	else if ( !oldvalue && newvalue )
	{
		SkillSelect(game, unit, nSkill, TRUE );
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeCanFly(
	UNIT* unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(wStat);
	REF(dwParam);
	REF(oldvalue);
	REF(data);
	REF(bSend);
	ASSERT_RETURN(unit);
	SETBIT(unit->pdwMovementFlags, MOVEFLAG_FLY, newvalue != 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEventUpdateEnemyCount(
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MONSTER;
	tTargetingQuery.fMaxRange	= UnitGetHolyRadius( unit );
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( pGame, tTargetingQuery );
	int nTargets = tTargetingQuery.nUnitIdCount;

	if(PlayerPvPIsEnabled(unit))
	{
		SKILL_TARGETING_QUERY tTargetingQueryPlayer;
		SETBIT( tTargetingQueryPlayer.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
		SETBIT( tTargetingQueryPlayer.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
		tTargetingQueryPlayer.tFilter.nUnitType = UNITTYPE_PLAYER;
		tTargetingQueryPlayer.fMaxRange	= UnitGetHolyRadius( unit );
		tTargetingQueryPlayer.nUnitIdMax  = 1;
		tTargetingQueryPlayer.pnUnitIds   = pnUnitIds;
		tTargetingQueryPlayer.pSeekerUnit = unit;
		SkillTargetQuery( pGame, tTargetingQueryPlayer );
		nTargets += tTargetingQueryPlayer.nUnitIdCount;
	}

	UnitSetStat( unit, STATS_HOLY_RADIUS_ENEMY_COUNT, nTargets );

	UnitRegisterEventTimed(unit, sEventUpdateEnemyCount, NULL, GAME_TICKS_PER_SECOND/4);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEventUpdateFriendCount(
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	UNITID pnUnitIds[ 1 ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetingQuery.fMaxRange	= UnitGetHolyRadius( unit );
	tTargetingQuery.nUnitIdMax  = 1;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = unit;
	SkillTargetQuery( pGame, tTargetingQuery );

	UnitSetStat( unit, STATS_HOLY_RADIUS_FRIEND_COUNT, tTargetingQuery.nUnitIdCount );

	UnitRegisterEventTimed(unit, sEventUpdateFriendCount, NULL, GAME_TICKS_PER_SECOND/4);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetPulseRate(
	UNIT * pUnit )
{
	if(AppIsHellgate())
		return GAME_TICKS_PER_SECOND;
	else
		return GAME_TICKS_PER_SECOND * 3;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUpdateTemplarPulse(
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	if (UnitCheckCanRunEvent(unit) == FALSE)
	{
		UnitSetFlag(unit, UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE);
		return FALSE;
	}

	if ( ! IsUnitDeadOrDying( unit ) )
		UnitTriggerEvent( pGame, EVENT_TEMPLAR_PULSE, unit, unit, NULL );

	UnitUnregisterTimedEvent( unit, sUpdateTemplarPulse );

	UnitRegisterEventTimed( unit, sUpdateTemplarPulse, NULL, sGetPulseRate( unit ) );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitUpdateHolyRadius( 
	UNIT* pUnit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);

	GAME * pGame = UnitGetGame( pUnit );

	if ( pUnit && oldvalue == 0 && newvalue > 0 && !UnitTestFlag( pUnit, UNITFLAG_JUSTFREED ) &&
		wStat != STATS_HOLY_RADIUS_PERCENT )
	{
		sUpdateTemplarPulse(pGame, pUnit, EVENT_DATA());		// this needs a better home
	}

	if (IS_SERVER( pGame ) && !oldvalue && newvalue )
	{ 
		if(UnitIsA(pUnit, UNITTYPE_PLAYER))
		{
			sEventUpdateEnemyCount(pGame, pUnit, EVENT_DATA());		// this needs a better home
			//sEventUpdateFriendCount(pGame, pUnit, EVENT_DATA());	// this needs a better home
		}
	} else {
#if !ISVERSION(SERVER_VERSION)
		c_UnitUpdateHolyRadiusSize( pUnit );
#endif// !ISVERSION(SERVER_VERSION)
 	}
}

BOOL UnitKillByHPAndPower(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	UnitDie( pUnit, NULL );
	return false;
}


void UnitLevelChanged(
	UNIT* pUnit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	if( !pUnit )
		return;	
	if( newvalue > pUnit->pUnitData->nCapLevel &&
		pUnit->pUnitData->nCapLevel >= 0 )
	{
		UnitSetExperienceLevel( pUnit, pUnit->pUnitData->nCapLevel );
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAchievementUpdated(
					  UNIT* pUnit,
					  int wStat,
					  PARAM dwParam,
					  int oldvalue,
					  int newvalue,
					  const STATS_CALLBACK_DATA & data,
					  BOOL bSend)
{
	REF(data);
#if !ISVERSION(SERVER_VERSION)
	if( !pUnit )
		return;	

	if( IS_SERVER(UnitGetGame(pUnit)) )
		return;

	if( UnitIsInLimbo( pUnit ) )
		return;

	if( pUnit != GameGetControlUnit(UnitGetGame(pUnit)) )
		return;


	int nAchievement = (int)dwParam;
	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(UnitGetGame(pUnit), DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETURN	(pAchievement);

	UnitGetStat(pUnit, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);

	if( oldvalue < pAchievement->nCompleteNumber &&
		newvalue >= pAchievement->nCompleteNumber )
	{
		if(AppIsHellgate())
		{
			ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_ACHIEVEMENT_COMPLETE ), StringTableGetStringByIndex(pAchievement->nNameString) );
		}
		else if(AppIsTugboat())
		{
			WCHAR szName[256];
			c_AchievementGetName(pUnit, pAchievement, szName, arrsize(szName));
			ConsoleString( CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_ACHIEVEMENT_COMPLETE ), szName );
		}
		UIShowAchievementUpMessage();
	}	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitUpdateWeaponsForSkills( 
	UNIT * pUnit,
	int wStat,
	PARAM dwPariam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	GAME * pGame = UnitGetGame( pUnit );

	if ( pUnit && (UnitIsA( pUnit, UNITTYPE_PLAYER ) || UnitGetGenus(pUnit) == GENUS_MONSTER ) && /*oldvalue != newvalue &&*/ !UnitTestFlag( pUnit, UNITFLAG_JUSTFREED ) )
	{
		//if ( ! IsUnitDeadOrDying( pUnit ) )
		//	UnitTriggerEvent( pGame, EVENT_TEMPLAR_PULSE, pUnit, pUnit, NULL );
		sUpdateTemplarPulse(pGame, pUnit, EVENT_DATA());		// this needs a better home
	}

}

//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct PROC_EVENT_LOOKUP
{
	STATS_TYPE eStat;
	UNIT_EVENT eEvent;
	FP_UNIT_EVENT_HANDLER pfHandler;
};
static const PROC_EVENT_LOOKUP sgtProcEventLookupTable[] = 
{
	{ STATS_PROC_CHANCE_ON_ATTACK,		EVENT_ATTACK,		ProcOnAttack },
	{ STATS_PROC_CHANCE_ON_DID_KILL,	EVENT_DIDKILL,		ProcOnDidKill },
	{ STATS_PROC_CHANCE_ON_DID_HIT,		EVENT_DIDHIT,		ProcOnDidHit },	
	{ STATS_PROC_CHANCE_ON_GOT_HIT,		EVENT_GOTHIT,		ProcOnGotHit },
	{ STATS_PROC_CHANCE_ON_GOT_KILLED,	EVENT_GOTKILLED,	ProcOnGotKilled },
};
static const int sgnNumProcEventLookup = arrsize( sgtProcEventLookupTable );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangeProcChance( 
	UNIT * pUnit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// get proc information
	int nProc = StatGetParam(wStat, dwParam, 0);
	int nProcLevel = StatGetParam(wStat, dwParam, 1);  // need to get this from somewhere in data eventually
	nProcLevel = MAX(1, nProcLevel);
	int nProcChance = abs( newvalue - oldvalue );  // this stat is a running total, the difference is what it is currently being modified by
	DWORD dwLastRunAttemptTick = 0;

	// setup the even data for this proc
	EVENT_DATA tEventData;
	tEventData.m_Data1 = nProc;
	tEventData.m_Data2 = nProcChance;
	tEventData.m_Data3 = nProcLevel;
	tEventData.m_Data4 = dwLastRunAttemptTick;
	
	// find the lookup for this stat
	const PROC_EVENT_LOOKUP *pLookup = NULL;
	for (int i = 0; i < sgnNumProcEventLookup; ++i)
	{	
		pLookup = &sgtProcEventLookupTable[ i ];
		if (pLookup->eStat == wStat)
		{
			break;  // found a match
		}
	}
	ASSERTX_RETURN( pLookup, "Unable to find lookup for proc stat" );

	// Proc chances accrue only up to an item.  Each proc has a separate
	// event registered for it ... ie, if you have a gun with 1% chance fire nova, and there
	// are 2 mods in it, each mod with a 2% chance of fire nova, the gun will have 3 events
	// that will try to execute each proc separately so you properly get 3 dice rolls for it.  
	// The stat changing in this function is then essentially a "runnning total" 
	// of all chances for a single proc type and delivery method.
		
	// either add or remove this event
	GAME *pGame = UnitGetGame( pUnit );
	if (newvalue > oldvalue)
	{
		// add new event
		UnitRegisterEventHandler( pGame, pUnit, pLookup->eEvent, pLookup->pfHandler, &tEventData );
	}
	else
	{
		// which event data fields to we compare on
		DWORD dwEventDataCompare = 0;
		SETBIT( dwEventDataCompare, EDC_PARAM1_BIT, TRUE );
		SETBIT( dwEventDataCompare, EDC_PARAM2_BIT, TRUE );
		SETBIT( dwEventDataCompare, EDC_PARAM3_BIT, TRUE );
		
		// remove event
		int nEventID = UnitEventFind( pGame, pUnit, pLookup->eEvent, pLookup->pfHandler, &tEventData, dwEventDataCompare );
		ASSERTX_RETURN( nEventID != INVALID_ID, "Unable to find event to remove for proc" );
		UnitUnregisterEventHandler( pGame, pUnit, pLookup->eEvent, nEventID );
	}
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangeExperience( 
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// for players that are gaining experience, and that are not in the process of gaining
	// experience through a level up action itself, check for a level up!
	if (UnitIsPlayer( pUnit ) && 
		nValueNew > nValueOld && 
		UnitTestFlag( pUnit, UNITFLAG_DONT_CHECK_LEVELUP ) == FALSE)
	{
		PlayerCheckLevelUp( pUnit );
	}
}
#endif


#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangeRankExperience( 
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// for players that are gaining experience, and that are not in the process of gaining
	// experience through a level up action itself, check for a level up!
	if (UnitIsPlayer( pUnit ) && 
		nValueNew > nValueOld && 
		UnitTestFlag( pUnit, UNITFLAG_DONT_CHECK_LEVELUP ) == FALSE)
	{
		PlayerCheckRankUp( pUnit );
	}
}
#endif


#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangeDifficultyCurrent( 
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// send all the active or completed quests for the new difficulty to the player
	s_QuestSendKnownQuestsToClient( pUnit );
	s_QuestsUpdateAvailability( pUnit );
	// TODO cmarch: need to send a message to the client to call this func UIResetQuestPanel();
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeUpdateWardrobe(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
#if !ISVERSION(CLIENT_ONLY_VERSION)

	UnitSetWardrobeChanged(unit, TRUE);
	WardrobeUpdateFromInventory(unit);

#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ESTATE sGetWarpBlockingState(
	UNIT *pUnit,
	BLOCKING_STAT_VALUE eBlocking)
{
	ASSERTX_RETVAL( pUnit, STATE_NONE, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// get trigger data
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );	
	const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pUnitData->nTriggerType );
	
	if (pTriggerData != NULL)
	{		
		if (eBlocking == BSV_BLOCKING)
		{
			if (pTriggerData->nStateBlocking != INVALID_LINK)
			{
				return (ESTATE)pTriggerData->nStateBlocking;
			}
		}
		else if (eBlocking == BSV_OPEN)
		{
			if (pTriggerData->nStateOpen != INVALID_LINK)
			{
				return (ESTATE)pTriggerData->nStateOpen;
			}
		}
	}
	
	// no state
	return STATE_NONE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitStatsChangeBlockMovement( 
	UNIT * pUnit,
	int wStat,
	PARAM dwParam,
	int nValueOld,
	int nValueNew,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// only care about active warps (note that return warps are handled separately by
	// the town portal system)
	BLOCKING_STAT_VALUE eBlocking = (BLOCKING_STAT_VALUE)nValueNew;
	BLOCKING_STAT_VALUE eBlockingPrevious = (BLOCKING_STAT_VALUE)nValueOld;

	if (IS_CLIENT( pUnit ) && 
		UnitTestFlag( pUnit, UNITFLAG_TRIGGER ) && 
		ObjectTriggerIsWarp( pUnit ))
	{
						
		ESTATE eStateOld = sGetWarpBlockingState( pUnit, eBlockingPrevious );
		ESTATE eStateNew = sGetWarpBlockingState( pUnit, eBlocking );
				
		// clear the old state
		if (eStateOld != STATE_NONE)
		{
			c_StateClear( pUnit, UnitGetId( pUnit ), eStateOld, 0 );
		}
		
		// set the new state
		if (eStateNew != STATE_NONE)
		{
			c_StateSet( pUnit, pUnit, eStateNew, 0, 0, INVALID_ID );			
		}
	}
}


//----------------------------------------------------------------------------
// EVENT_DATA:
// m_Data1:  stat to changed
// m_Data2:  regen amount (per call to this function)
// m_Data3:  game ticks between calls to this callback
// m_Data4:  the stat that is doing the regen
//----------------------------------------------------------------------------
BOOL sEventStatRegen(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	ASSERT_RETFALSE(unit);
/*
	if (UnitCheckCanRunEvent(unit) == FALSE)
	{
		UnitSetFlag(unit, UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE);
		return TRUE;
	}
*/
	WORD stat = (WORD)STAT_GET_STAT(event_data.m_Data1);
	PARAM param = STAT_GET_PARAM(event_data.m_Data1);
	int regenVal = (int)event_data.m_Data2;
	int interval = (int)event_data.m_Data3;
	//WORD regenStat = (WORD)event_data.m_Data4;
	
	// health exceptions
	if (stat == STATS_HP_CUR)
	{
		// if we're trying to do damage and the unit is invulnerable, do nothing
		// also, if we're trying to heal, but we're dead, do nothing
		if ((regenVal < 0 && UnitIsInvulnerable(unit)) || 
			(regenVal > 0 && IsUnitDeadOrDying(unit)))
		{
			UnitRegisterEventTimed(unit, sEventStatRegen, &event_data, interval);
			return TRUE;
		}

		if(AppIsHellgate())
		{
			if(UnitHasState(game, unit, STATE_SFX_TOXIC) && regenVal > 0)
			{
				regenVal = 0;
			}
		}
	}
	
	UnitSetFlag(unit, UNITFLAG_REGENCHANGE);
	UnitChangeStat(unit, stat, param, regenVal);
	UnitClearFlag(unit, UNITFLAG_REGENCHANGE);

	int curVal = UnitGetStat(unit, stat, param);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (regenVal < 0 && stat == STATS_HP_CUR && IS_SERVER(game))
	{
		if (curVal <= 0)
		{
			s_UnitKillUnit(game, NULL, unit, unit, NULL);
			return TRUE;
		}
	}
#endif

	// if we have regenerated to max, stop
	const STATS_DATA * stats_data = StatsGetData(game, stat);
	if (regenVal > 0)
	{
		ASSERT_RETFALSE(stats_data);
		if (stats_data->m_nMaxStat > INVALID_LINK)
		{
			int maxvalue = UnitGetStat(unit, stats_data->m_nMaxStat, param);
			if (curVal >= maxvalue)
			{
				return TRUE;
			}
		} 
	}
	else if (regenVal < 0 && curVal <= stats_data->m_nMinSet )
	{
		return TRUE;
	}

	// do regeneration
	ASSERT_DO(interval != 0)
	{ 
		interval = 1; 
	}
//#if ISVERSION(DEBUG_VERSION)
//	static const STATS_DATA * stats_data = StatsGetData(game, regenStat);
//	ASSERT_RETFALSE(stats_data);
//	if (stats_data->m_nStatsType == STATSTYPE_REGEN)
//	{
//		ASSERT(UnitGetStat(unit, regenStat, param) == regenVal);
//	}
//	else
//	{
//		ASSERT(UnitGetStat(unit, regenStat, param) == -regenVal);
//	}
//#endif

	UnitRegisterEventTimed(unit, sEventStatRegen, &event_data, interval);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatIsAnyRegenStat(
	const STATS_DATA * pStatsData)
{
	ASSERT_RETFALSE(pStatsData);
	switch(pStatsData->m_nStatsType)
	{
	case STATSTYPE_REGEN:
	case STATSTYPE_DEGEN:
	case STATSTYPE_REGEN_CLIENT:
	case STATSTYPE_DEGEN_CLIENT:
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatIsRegenStat(
	const STATS_DATA * pStatsData)
{
	ASSERT_RETFALSE(pStatsData);
	switch(pStatsData->m_nStatsType)
	{
	case STATSTYPE_REGEN:
	case STATSTYPE_REGEN_CLIENT:
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatIsDegenStat(
	const STATS_DATA * pStatsData)
{
	ASSERT_RETFALSE(pStatsData);
	switch(pStatsData->m_nStatsType)
	{
	case STATSTYPE_DEGEN:
	case STATSTYPE_DEGEN_CLIENT:
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// helper function for setting regen or degen to non-zero value
//----------------------------------------------------------------------------
static void sSetRegenStat(
	GAME * game,
	UNIT * unit,
	int regenStat,								// this is the regen stat (hp_regen)
	PARAM dwParam,								// this is the param (from hp_regen)
	const STATS_DATA * regen_stats_data,		// this is the data for the regen stat
	int regenVal)								// this is the regen amount
{
	ASSERT_RETURN(regenVal != 0);
	ASSERT_RETURN(regen_stats_data);
	ASSERT_RETURN(sStatIsAnyRegenStat(regen_stats_data));

	// number of ticks between regen "pulses"
	int interval = GAME_TICKS_PER_SECOND * regen_stats_data->m_nRegenIntervalInMS / MSECS_PER_SEC;
	interval = MAX(interval, 1);	
	int value = regenVal * interval / GAME_TICKS_PER_SECOND;
	if (value == 0)
	{
		return;
	}

	UnitRegisterEventTimed(unit, sEventStatRegen, EVENT_DATA(MAKE_STAT(regen_stats_data->m_nAssociatedStat[0], dwParam), value, interval, regenStat), interval);
}


//----------------------------------------------------------------------------
// helper function for setting regen or degen to non-zero value
//----------------------------------------------------------------------------
static void sStartRegenStat(
	GAME * game,
	UNIT * unit,
	int regenStat,								// this is the regen stat (hp_regen)
	PARAM dwParam,								// this is the param (from hp_regen)
	const STATS_DATA * regen_stats_data,		// this is the data for the regen stat
	int regenVal)								// this is the regen amount
{
	ASSERT_RETURN(regenVal != 0);
	ASSERT_RETURN(regen_stats_data);
	ASSERT_RETURN(sStatIsAnyRegenStat(regen_stats_data));
	if (sStatIsDegenStat(regen_stats_data))
	{
		regenVal = -regenVal;
	}

	// don't start regen if we have degen (positive or negative)
	if (regenVal > 0 && sStatIsRegenStat(regen_stats_data))
	{
		const STATS_DATA * regen_target_data = StatsGetData(game, regen_stats_data->m_nAssociatedStat[0]);
		ASSERT_RETURN(regen_target_data);

		for (int ii = 0; ii < STATS_NUM_ASSOCATED; ii++)
		{
			if (regen_target_data->m_nRegenByStat[ii] == INVALID_LINK)
			{
				break;
			}
			if (regen_target_data->m_nRegenByStat[ii] == regenStat)
			{
				continue;
			}
			const STATS_DATA * other_regen_stats_data = StatsGetData(game, regen_target_data->m_nRegenByStat[ii]);
			ASSERT_BREAK(other_regen_stats_data);
			if (sStatIsDegenStat(other_regen_stats_data))
			{
				int otherRegenVal = UnitGetStat(unit, regen_target_data->m_nRegenByStat[ii], dwParam);
				if (otherRegenVal != 0)
				{
					return;
				}
			}
		}
	}
	// turn off positive regen if we're setting degen
	if (sStatIsDegenStat(regen_stats_data))
	{
		const STATS_DATA * regen_target_data = StatsGetData(game, regen_stats_data->m_nAssociatedStat[0]);
		ASSERT_RETURN(regen_target_data);

		for (int ii = 0; ii < STATS_NUM_ASSOCATED; ii++)
		{
			if (regen_target_data->m_nRegenByStat[ii] == INVALID_LINK)
			{
				break;
			}
			if (regen_target_data->m_nRegenByStat[ii] == regenStat)
			{
				continue;
			}
			const STATS_DATA * other_regen_stats_data = StatsGetData(game, regen_target_data->m_nRegenByStat[ii]);
			ASSERT_BREAK(other_regen_stats_data);
		
			if (sStatIsRegenStat(other_regen_stats_data))
			{


				if (UnitGetStat(unit, regen_target_data->m_nRegenByStat[ii], dwParam) > 0)
				{
					UnitUnregisterTimedEvent(unit, sEventStatRegen, CheckEventParam14, EVENT_DATA(MAKE_STAT(regen_stats_data->m_nAssociatedStat[0], dwParam), 0, 0, regen_target_data->m_nRegenByStat[ii]));
				}
			}
		}
	}

	// finally set the regen or degen timer event
	sSetRegenStat(game, unit, regenStat, dwParam, regen_stats_data, regenVal);
}


//----------------------------------------------------------------------------
// helper function for setting degen value to zero
//----------------------------------------------------------------------------
static void sClearDegenStat(
	GAME * game,
	UNIT * unit,
	int regenStat,								// this is the regen stat (hp_regen)
	PARAM dwParam,								// this is the param (from hp_regen)
	const STATS_DATA * regen_stats_data)		// this is the data for the regen stat
{
	ASSERT_RETURN(regen_stats_data);
	ASSERT_RETURN(sStatIsDegenStat(regen_stats_data));

	// start up any positive regen stats again (they got cleared when we set degen)
	const STATS_DATA * regen_target_data = StatsGetData(game, regen_stats_data->m_nAssociatedStat[0]);
	ASSERT_RETURN(regen_target_data);

	for (int ii = 0; ii < STATS_NUM_ASSOCATED; ii++)
	{
		if (regen_target_data->m_nRegenByStat[ii] == INVALID_LINK)
		{
			break;
		}
		if (regen_target_data->m_nRegenByStat[ii] == regenStat)
		{
			continue;
		}
		const STATS_DATA * other_regen_stats_data = StatsGetData(game, regen_target_data->m_nRegenByStat[ii]);
		ASSERT_BREAK(other_regen_stats_data);
		if (sStatIsRegenStat(other_regen_stats_data))
		{
			int otherRegenVal = UnitGetStat(unit, regen_target_data->m_nRegenByStat[ii], dwParam);
			if (otherRegenVal > 0)
			{
				sSetRegenStat(game, unit, regen_target_data->m_nRegenByStat[ii], dwParam, other_regen_stats_data, otherRegenVal);
			}
		}
	}
}


//----------------------------------------------------------------------------
// stats change call back for a regen stat
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeRegen(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	ASSERT_RETURN(unit);
	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return;
	}
	if (oldvalue == newvalue)
	{
		return;
	}

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	const STATS_DATA * regen_stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(regen_stats_data);
	ASSERT_RETURN(sStatIsAnyRegenStat(regen_stats_data));
	ASSERT_RETURN(regen_stats_data->m_nAssociatedStat[0] != INVALID_LINK);

	// unregister the old event
	if (oldvalue)
	{
		UnitUnregisterTimedEvent(unit, sEventStatRegen, CheckEventParam14, EVENT_DATA(MAKE_STAT(regen_stats_data->m_nAssociatedStat[0], dwParam), 0, 0, wStat));
	}

	if (sStatIsRegenStat(regen_stats_data) && newvalue == 0)
	{
		return;
	}

	const STATS_DATA * regen_target_data = StatsGetData(game, regen_stats_data->m_nAssociatedStat[0]);
	ASSERT_RETURN(regen_target_data);

	// don't regenerate if we have zero in the target stat - otherwise things like health packs will start to regen!
	if (regen_target_data->m_nMaxStat != INVALID_LINK)
	{
		int regenTargetMax = UnitGetStat(unit, regen_target_data->m_nMaxStat, dwParam);
		if (regenTargetMax <= 0)
		{
			return;
		}
	}

	if (newvalue == 0)
	{
		sClearDegenStat(game, unit, wStat, dwParam, regen_stats_data);
		return;
	}

	sStartRegenStat(game, unit, wStat, dwParam, regen_stats_data, newvalue);
}


//----------------------------------------------------------------------------
// stats change call back for a regen target
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeRegenTarget(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	ASSERT_RETURN(unit);
	if (UnitTestFlag(unit, UNITFLAG_REGENCHANGE))
	{
		return;
	}

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data && (stats_data->m_nRegenByStat[0] != INVALID_LINK || stats_data->m_nRegenByStat[1]!= INVALID_LINK));

	// if this stat gets regenerated by any stat, then we may want to register a regen callback for regen stats
	for (int ii = 0; ii < STATS_NUM_ASSOCATED; ii++)
	{
		int regenStat = stats_data->m_nRegenByStat[ii];
		if (regenStat == INVALID_LINK)
		{
			break;
		}

		int regenVal = UnitGetStat(unit, regenStat, dwParam);
		if (regenVal == 0)
		{
			continue;
		}

		const STATS_DATA * regen_stats_data = StatsGetData(game, regenStat);
		ASSERT_BREAK(regen_stats_data && regen_stats_data->m_nAssociatedStat[0] == wStat);

		if (sStatIsDegenStat(regen_stats_data))
		{
			regenVal = -regenVal;
		}

		// if we went to zero and we have some sort of degen
		if (newvalue <= 0 && oldvalue > 0 && regenVal < 0)
		{
			UnitUnregisterTimedEvent(unit, sEventStatRegen, CheckEventParam14, EVENT_DATA(MAKE_STAT(wStat, dwParam), 0, 0, regenStat));
			continue;
		}

		// unregister any events if we've reached max stat
		if (stats_data->m_nMaxStat != INVALID_LINK)
		{
			int maxvalue = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
			if (newvalue >= maxvalue && regenVal > 0)
			{
				UnitUnregisterTimedEvent(unit, sEventStatRegen, CheckEventParam14, EVENT_DATA(MAKE_STAT(wStat, dwParam), 0, 0, regenStat));
				continue;
			}
		}

		if (!UnitHasTimedEvent(unit, sEventStatRegen, CheckEventParam14, EVENT_DATA(MAKE_STAT(wStat, dwParam), 0, 0, regenStat)))
		{
			sStartRegenStat(game, unit, regenStat, dwParam, regen_stats_data, regenVal);
			continue;
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckHPDegenDeath(
	GAME *pGame,
	UNIT *pUnit,
	int nOldHealth,
	int nNewHealth,
	int nDelta,
	UNITID idAttacker,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ] )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	
	TraceCombat(
		"unit %s(%d) health stat degenerated. old hp=%d, delta=%d, current hp=%d.",
		UnitGetDevName(pUnit),
		UnitGetId(pUnit), 
		nOldHealth >> StatsGetShift(pGame, STATS_HP_CUR),
		nDelta,
		nNewHealth >> StatsGetShift(pGame, STATS_HP_CUR));

	if (nNewHealth <= 0)
	{
	
		// get attacker of unit (if no attacker, target unit is the attacker unit to itself)
		UNIT *pAttacker = UnitGetById( pGame, idAttacker );
		if (pAttacker == NULL)
		{
			pAttacker = pUnit;
		}

		s_UnitKillUnit( pGame, NULL, pAttacker, pUnit, pWeapons );
		return TRUE;  // was killed
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	return FALSE;  // *NOT* killed		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sEventStatDegenPercent(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERT_RETTRUE( pUnit );
	if (IsUnitDeadOrDying( pUnit ))
	{
		return TRUE;
	}

	WORD wStatToDegen = (WORD)STAT_GET_STAT(tEventData.m_Data1);
	PARAM dwParamOfStatToDegen = STAT_GET_PARAM(tEventData.m_Data1);
	int nDegenPercent = (int)tEventData.m_Data2;
	int wStatDegenSource = (int)tEventData.m_Data3;			// the stat that is directing the degeneration
	UNITID idAttacker = (UNITID)tEventData.m_Data4;
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	{
		ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
		pWeapons[ 0 ] = UnitGetById( pGame, (UNITID)tEventData.m_Data5 );
		pWeapons[ 1 ] = UnitGetById( pGame, (UNITID)tEventData.m_Data6 );
	}

	// get stat data for the stat directing the degeneration
	const STATS_DATA *pStatDataDirectingDegen = StatsGetData( pGame, wStatDegenSource );
	
	// what is the degeneration interval (we get this from the source stat directing the degeneration)
	int nInterval = GAME_TICKS_PER_SECOND * pStatDataDirectingDegen->m_nRegenIntervalInMS / MSECS_PER_SEC;
	nInterval = MAX( nInterval, 1 );
	
	// check for invulnerabilities when dealing with health
	if (wStatToDegen == STATS_HP_CUR && UnitIsInvulnerable( pUnit ))
	{
		// don't do any degen, but keep the event going
		UnitRegisterEventTimed( pUnit, sEventStatDegenPercent, &tEventData, nInterval );
		return TRUE;
	}

	// if don't have any stat to degen, do nothing
	int nStatToDegenCurrentValue = UnitGetStat( pUnit, wStatToDegen, dwParamOfStatToDegen );
	if (nStatToDegenCurrentValue <= 0)
	{
		return TRUE;
	}

	// this degeneration works in 2 ways depending on the associated stats setup in the
	// wStatDegenSource ...
	// 1) assocstat1 only is specified, this means "decrease wStatToDegen's current value by nDegenPercent%"
	// 2) assocstat1 and 2 are specified, this means "decrease wStatToDegen by nDegenPercent% of assocstat2"
	int nMaxValue = nStatToDegenCurrentValue;
	if (pStatDataDirectingDegen->m_nAssociatedStat[1] != INVALID_LINK)
	{
		int wStatDirectingMaxValue = pStatDataDirectingDegen->m_nAssociatedStat[1];
		nMaxValue = UnitGetStat(pUnit, wStatDirectingMaxValue, dwParamOfStatToDegen);
	}
	
	// how much will we change the stat by
//	int nDelta = -PCT( nStatToDegenCurrentValue, nDegenPercent ) / 100;
	int nDelta = -PCT( nMaxValue, nDegenPercent );
	nDelta = PIN( nDelta, -nStatToDegenCurrentValue, -(1 << StatsGetShift( pGame, wStatToDegen )));

	// change the stat
	UnitChangeStat( pUnit, wStatToDegen, dwParamOfStatToDegen, nDelta );

	// do special monitoring for degeneration of health
	if (wStatToDegen == STATS_HP_CUR)
	{
		int nOldHealth = nStatToDegenCurrentValue;
		int nNewHealth = UnitGetStat( pUnit, wStatToDegen, dwParamOfStatToDegen );

		if (sCheckHPDegenDeath( pGame, pUnit, nOldHealth, nNewHealth, nDelta, idAttacker, pWeapons ) == TRUE)
		{
			return TRUE;  // was killed, don't degen stat anymore
		}
		
	}

	// continue degeneration
	UnitRegisterEventTimed( pUnit, sEventStatDegenPercent, &tEventData, nInterval );		
	return TRUE;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeDegenPercent(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	ASSERT_RETURN( pUnit );

	const STATS_DATA *pStatsDataDirectingDegen = StatsGetData( UnitGetGame( pUnit ), wStat );
	ASSERT_RETURN(pStatsDataDirectingDegen && pStatsDataDirectingDegen->m_nStatsType == STATSTYPE_DEGEN_PERCENT && pStatsDataDirectingDegen->m_nAssociatedStat[0] > 0);

	if (nOldValue)
	{
		EVENT_DATA tEventData;
		tEventData.m_Data1 = MAKE_STAT( pStatsDataDirectingDegen->m_nAssociatedStat[0], dwParam );
		tEventData.m_Data2 = nNewValue;
		tEventData.m_Data3 = wStat;
		tEventData.m_Data4 = INVALID_ID;
		tEventData.m_Data5 = INVALID_ID;
		tEventData.m_Data6 = INVALID_ID;
	
		UnitUnregisterTimedEvent( pUnit, sEventStatDegenPercent, CheckEventParam1, &tEventData );
	}
	if (nNewValue)
	{
		int interval = GAME_TICKS_PER_SECOND * pStatsDataDirectingDegen->m_nRegenIntervalInMS / MSECS_PER_SEC;
		interval = MAX(interval, 1);

		UNITID idAttacker = UnitGetStat( pUnit, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_ATTACKER, 0 );
		UNITID idWeapon = UnitGetStat( pUnit, STATS_CURRENT_COMBAT_SPECIAL_EFFECT_WEAPON, 0 );
		
		EVENT_DATA tEventData;
		tEventData.m_Data1 = MAKE_STAT( pStatsDataDirectingDegen->m_nAssociatedStat[0], dwParam );
		tEventData.m_Data2 = nNewValue;
		tEventData.m_Data3 = wStat;
		tEventData.m_Data4 = idAttacker;
		tEventData.m_Data5 = idWeapon;
		tEventData.m_Data6 = INVALID_ID;
		
		UnitRegisterEventTimed( pUnit, sEventStatDegenPercent, &tEventData, interval );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sEventStatRegenPercent(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERT_RETTRUE( pUnit );
	if (IsUnitDeadOrDying( pUnit ))
	{
		return TRUE;
	}

	WORD wStatToRegen = (WORD)STAT_GET_STAT(tEventData.m_Data1);
	PARAM dwParamOfStatToRegen = STAT_GET_PARAM(tEventData.m_Data1);
	int nRegenPercent = (int)tEventData.m_Data2;
	int wStatRegenSource = (int)tEventData.m_Data3;			// the stat that is directing the regeneration

	// get stat data for the stat directing the regeneration
	const STATS_DATA *pStatDataDirectingRegen = StatsGetData( pGame, wStatRegenSource );

	// what is the regeneration interval (we get this from the source stat directing the regeneration)
	int nInterval = GAME_TICKS_PER_SECOND * pStatDataDirectingRegen->m_nRegenIntervalInMS / MSECS_PER_SEC;
	nInterval = MAX( nInterval, 1 );

	// don't regen beyond max value
	int nStatToRegenCurrentValue = UnitGetStat( pUnit, wStatToRegen, dwParamOfStatToRegen );
	int wStatToRegenMax = StatsDataGetMaxStat( pGame, wStatToRegen );
	int nStatToRegenMaxValue = INT_MAX;
	if ( wStatToRegenMax >= 0 )
	{
		nStatToRegenMaxValue = UnitGetStat( pUnit, wStatToRegenMax );
	}

	// TODO cmarch the player starts out with full shields on the server after 
	// s_UnitRestoreVitals, but the client does not. So, until that is fixed
	// shield regen needs to happen even if the server is at the max value.
	// This function isn't called that often anyways.
	//if ( nStatToRegenCurrentValue < nStatToRegenMaxValue )
	{
		// this regeneration works in 2 ways depending on the associated stats setup in the
		// wStatRegenSource ...
		// 1) assocstat1 only is specified, this means "decrease wStatToRegen's current value by nRegenPercent%"
		// 2) assocstat1 and 2 are specified, this means "decrease wStatToRegen by nRegenPercent% of assocstat2"
		int nMaxValue = nStatToRegenCurrentValue;
		if (pStatDataDirectingRegen->m_nAssociatedStat[1] != INVALID_LINK)
		{
			int wStatDirectingMaxValue = pStatDataDirectingRegen->m_nAssociatedStat[1];
			nMaxValue = UnitGetStat(pUnit, wStatDirectingMaxValue, dwParamOfStatToRegen);
		}

		// how much will we change the stat by
		int nDelta = 0;
		if (nStatToRegenMaxValue > nStatToRegenCurrentValue && nRegenPercent) 
		{
			nDelta = PCT( nMaxValue, nRegenPercent );
			nDelta = MAX( nDelta, (1 << StatsGetShift( pGame, wStatToRegen )) ); // make sure the change has some effect, since the time interval is well defined in the data (as opposed to every frame)
		}

		// change the stat
		UnitSetStat( pUnit, wStatToRegen, dwParamOfStatToRegen, nStatToRegenCurrentValue + nDelta );
	}

	// continue regeneration
	UnitRegisterEventTimed( pUnit, sEventStatRegenPercent, &tEventData, nInterval );		
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeRegenPercent(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	ASSERT_RETURN( pUnit );

	const STATS_DATA *pStatsDataDirectingRegen = StatsGetData( UnitGetGame( pUnit ), wStat );
	ASSERT_RETURN(pStatsDataDirectingRegen && pStatsDataDirectingRegen->m_nStatsType == STATSTYPE_REGEN_PERCENT && pStatsDataDirectingRegen->m_nAssociatedStat[0] > 0);

	// TODO cmarch fix the data and check unit version
	nNewValue /= 40;

	if (nOldValue)
	{
		EVENT_DATA tEventData;
		tEventData.m_Data1 = MAKE_STAT( pStatsDataDirectingRegen->m_nAssociatedStat[0], dwParam );
		tEventData.m_Data2 = nNewValue;
		tEventData.m_Data3 = wStat;
		tEventData.m_Data4 = INVALID_ID;
		tEventData.m_Data5 = INVALID_ID;

		UnitUnregisterTimedEvent( pUnit, sEventStatRegenPercent, CheckEventParam1, &tEventData );
	}
	if (nNewValue)
	{
		// TODO calc any initial delay
		int interval = GAME_TICKS_PER_SECOND * pStatsDataDirectingRegen->m_nRegenIntervalInMS / MSECS_PER_SEC;
		interval = MAX(interval, 1);

		EVENT_DATA tEventData;
		tEventData.m_Data1 = MAKE_STAT( pStatsDataDirectingRegen->m_nAssociatedStat[0], dwParam );
		tEventData.m_Data2 = nNewValue;
		tEventData.m_Data3 = wStat;
		tEventData.m_Data4 = INVALID_ID;
		tEventData.m_Data5 = INVALID_ID;

		UnitRegisterEventTimed( pUnit, sEventStatRegenPercent, &tEventData, interval );

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeEnemyCount(
	UNIT* unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// If you have an old value, but no new value, then you disable the holy radius
	// UNLESS there are any friends
	if ( oldvalue && !newvalue && UnitGetStat(unit, STATS_HOLY_RADIUS_FRIEND_COUNT) == 0 )
		c_UnitDisableHolyRadius( unit );
	else if ( !oldvalue && newvalue )
		c_UnitEnableHolyRadius( unit );
	else
		c_UnitUpdateHolyRadiusUnitCount( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeFriendCount(
	UNIT* unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// If you have an old value, but no new value, then you disable the holy radius
	// UNLESS there are any enemies
	if ( oldvalue && !newvalue && UnitGetStat(unit, STATS_HOLY_RADIUS_ENEMY_COUNT) == 0 )
		c_UnitDisableHolyRadius( unit );
	else if ( !oldvalue && newvalue )
		c_UnitEnableHolyRadius( unit );
	else
		c_UnitUpdateHolyRadiusUnitCount( unit );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeWeaponAmmo(
	UNIT* unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	int nAmmoCurr = UnitGetStat( unit, STATS_WEAPON_AMMO_CUR );
	int nAmmoMax  = UnitGetStat( unit, STATS_WEAPON_AMMO_CAPACITY );

	float fPercent = (float)nAmmoCurr / (float) nAmmoMax;

	int nModelId = c_UnitGetModelId( unit );
	if ( nModelId != INVALID_ID )
		c_ModelSetParticleSystemParam( nModelId, PARTICLE_SYSTEM_PARAM_PARTICLE_SPAWN_THROTTLE, fPercent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeQueueItemWindowsRepaint(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	UIQueueItemWindowsRepaint();
}

BOOL UnitsThemeIsVisibleInLevel( UNIT *pUnit,
								 int nThemeOverride /*= INVALID_ID*/)
{
	ASSERT_RETFALSE( pUnit );	
	int nPlayerTheme = nThemeOverride;
	if( nPlayerTheme == INVALID_ID )
	{
		nPlayerTheme = UnitGetStat( pUnit, STATS_UNIT_THEME );
	}
	if( UnitGetLevel( pUnit ) &&
		nPlayerTheme != INVALID_ID )
	{
		int *nThemes( NULL );
		LevelGetSelectedThemes( UnitGetLevel( pUnit ), &nThemes ); 
		for( int t = 0; t < MAX_THEMES_PER_LEVEL; t++ )
		{
			if( nPlayerTheme < 0 ) //not theme
			{
				if( !ThemeIsA( nThemes[ t ], nPlayerTheme * -1 ) )
				{
					return TRUE;
				}
			}
			else
			{
				if( ThemeIsA( nThemes[ t ], nPlayerTheme ) )
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}
	return TRUE;
}

//this will set the visibilty on a unique item based on it's theme 
//and if it's a dungeon entrance
void c_UnitSetVisibilityOnClientByThemesAndGameEvents( UNIT *pUnit,
													   int nThemeOverride )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( pUnit ) );	//ONLY CLIENTS DO THIS!
	ASSERT_RETURN( pUnit );
	if( UnitGetGenus( pUnit ) != GENUS_MONSTER &&
		UnitGetGenus( pUnit ) != GENUS_OBJECT )
	{
		return;
	}
	if( UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_HIDDEN ) )
	{
		return;
	}

	LEVEL *pLevel = UnitGetLevel( pUnit );

	if( pLevel->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes )   //make sure the level area allows for theme on the client versus the server		
	{	
		
		BOOL bIsVisible = UnitsThemeIsVisibleInLevel( pUnit, nThemeOverride );
		if( bIsVisible )
		{
			//secondary checks...

			// First Check. Is this Unit a warp to dungeon ( needs to be hidden )
			bIsVisible = c_LevelEntranceVisible( pUnit ); //returns true if it's not even an entrance
		}
		//set the unit visible or not depending on the theme.
		if( !bIsVisible )
		{
			c_UnitSetNoDraw( pUnit, !bIsVisible, TRUE, 0.0f, FALSE );
		}
		else
		{
			int nModelId = c_UnitGetModelId(pUnit);
			if ( UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
				nModelId = c_UnitGetModelIdThirdPersonOverride( pUnit );
			e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW, FALSE );
			UnitSetStat( pUnit, STATS_FADE_TIME_IN_TICKS, 30 );
			UnitSetStat( pUnit, STATS_NO_DRAW, 0 ); 
		}
		if( !bIsVisible ) //set the unit not active
		{			
			UnitSetInteractive( pUnit, UNIT_INTERACTIVE_RESTRICED );
		}
		else
		{			
			UnitSetInteractive( pUnit, (UnitDataTestFlag(pUnit->pUnitData, UNIT_DATA_FLAG_INTERACTIVE))?UNIT_INTERACTIVE_ENABLED:UNIT_INTERACTIVE_RESTRICED );
		}
	}
#endif	
}






//----------------------------------------------------------------------------
// Tugboat and client for now only.  Check where the stats changed callback is init'ed
//----------------------------------------------------------------------------
static inline void sUnitStatsThemeChanged(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	c_UnitSetVisibilityOnClientByThemesAndGameEvents( unit, newvalue );	
}

//----------------------------------------------------------------------------
// Tugboat updates the quest system
//----------------------------------------------------------------------------
static inline void sUnitQuestTimerChanged(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	if( IS_SERVER( UnitGetGame( unit ) ) )
	{
		s_QuestTimerUpdate( unit, (int)dwParam, oldvalue, newvalue );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeAnimationFreeze(
	UNIT* unit,
	WORD wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	BOOL bSend)
{
	BOOL bCallAppearanceFreeze = FALSE;
	BOOL bFreeze = FALSE;
	if ( !oldvalue && newvalue )
	{
		bCallAppearanceFreeze = TRUE;
		bFreeze = TRUE;
	} 
	else if ( oldvalue && ! newvalue )
	{
		bCallAppearanceFreeze = TRUE;
		bFreeze = FALSE;
	}
	if ( bCallAppearanceFreeze )
	{
		int nModelId = c_UnitGetModelId( unit );
		if ( nModelId != INVALID_ID )
		{
			c_AppearanceFreeze( nModelId, bFreeze );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeSkillCooling(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	ASSERT_RETURN( IS_CLIENT(unit) );
	sUnitStatsChangeQueueItemWindowsRepaint( unit, wStat, dwParam, oldvalue, newvalue, data, bSend );

	GAME * pGame = UnitGetGame( unit );
	UNIT * pControlUnit = GameGetControlUnit( pGame );
	if ( !newvalue && oldvalue && 
		(pControlUnit == unit || UnitGetContainer( unit ) == pControlUnit ) )
	{
		int nSkill = StatGetParam( wStat, dwParam, 0 );
		if (nSkill != INVALID_ID)
		{
			const SKILL_DATA * pSkillData = SkillGetData(NULL, nSkill);

			if (( !SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAY_COOLDOWN_ON_WEAPON ) || unit != pControlUnit ) &&
				pSkillData->nCooldownFinishedSound != INVALID_ID )
			{
				ATTACHMENT_DEFINITION tAttachmentDef;
				tAttachmentDef.eType = ATTACHMENT_SOUND;
				tAttachmentDef.nAttachedDefId = pSkillData->nCooldownFinishedSound;

				int nModel = c_UnitGetModelId( unit );
				if ( nModel != INVALID_ID )
					c_ModelAttachmentAdd( nModel, tAttachmentDef, "", TRUE, ATTACHMENT_OWNER_NONE, INVALID_ID, INVALID_ID );
			}
		}
	}

	if ( oldvalue && ! newvalue )
		SkillUpdateActiveSkillsDelayed( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeSkillGroupCooling(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	ASSERT_RETURN( IS_CLIENT(unit) );
	sUnitStatsChangeQueueItemWindowsRepaint( unit, wStat, dwParam, oldvalue, newvalue, data, bSend );

	if ( oldvalue && ! newvalue )
		SkillUpdateActiveSkillsDelayed( unit );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeNoDraw(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// set no draw
	if ( oldvalue != 0 && newvalue != 0 )
		return;
	if ( oldvalue == 0 && newvalue == 0 )
		return;

	int nTicks = UnitGetStat( unit, STATS_FADE_TIME_IN_TICKS );
	c_UnitSetNoDraw( unit, newvalue != 0, TRUE, nTicks ? ((float)nTicks / GAME_TICKS_PER_SECOND) : 1.0f, AppIsTugboat() );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeDontDrawWeapons(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	// set no draw
	if ( oldvalue != 0 && newvalue != 0 )
		return;
	if ( oldvalue == 0 && newvalue == 0 )
		return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeIdentified(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	GAME *pGame = UnitGetGame( unit );
	if (IS_CLIENT( pGame ))
	{
		if (oldvalue != newvalue)
		{
			UNIT *pLocalPlayer = GameGetControlUnit( pGame );
			UNIT *pUltimateContainer = UnitGetUltimateContainer( unit );
			if (pLocalPlayer == pUltimateContainer)
			{
				UIRepaint();
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeAutoParty(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	if (AppIsMultiplayer())
	{
		GAME *pGame = UnitGetGame( unit );
		if (IS_SERVER( pGame ))
		{
			if (newvalue == TRUE)
			{
				s_PartyMatchmakerEnable( unit );
			}
			else
			{
				s_PartyMatchmakerDisable( unit );
			}
			
		}
		else
		{		
			#if !ISVERSION(SERVER_VERSION)
			{
				if (newvalue == TRUE && PartyMatchmakerIsEnabledForGame( pGame ))
				{
					c_StateSet( unit, unit, STATE_AUTO_PARTY, 0, 0, INVALID_LINK );		
				}
				else
				{
					c_StateClear( unit, UnitGetId( unit ), STATE_AUTO_PARTY, 0 );
				}
			}
			#endif
		}
		
	}
	
}

//----------------------------------------------------------------------------
// KCK: Added to allow the fuse timer to be updated upon changing the fuse
// end time stat.
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeFuseEndTick(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	// KCK: Only reset the fuse if the value has actually changed.
	if ( oldvalue != newvalue )
	{
		DWORD dwNowTick = GameGetTick( UnitGetGame(unit) );
		DWORD dwEndTick = UnitGetStatShift( unit, STATS_FUSE_END_TICK );
		UnitUnFuse( unit );
		UnitSetFuse( unit, dwEndTick - dwNowTick );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeItemQuantity(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	GAME *pGame = UnitGetGame( unit );
	UNIT *pControlUnit = GameGetControlUnit( pGame );	
	if (UnitGetUltimateOwner( unit ) == pControlUnit)
	{
		if (InventoryIsInIngredientsLocation( unit ))
		{
			c_RecipeCurrentUpdateUI();				
		}

		// Update quest log when quest items are picked up.
		// Pickup of items which do not result in item quantity change are 
		// covered by sUnitStatsChangeItemQuantity in units.cpp.
		if (UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP))
		{
			UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT );
		}
		
		// send UI message
		UNIT *pContainer = UnitGetContainer( unit );
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation( unit, &tInvLoc );
		UISendMessage( WM_INVENTORYCHANGE, UnitGetId( pContainer ), tInvLoc.nInvLocation );
				
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeItemClassLastUsed(
	UNIT* unit,
	WORD wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
//	GAME *pGame = UnitGetGame( unit );
	
	// repaint the bottom bar on the UI now
	UISendMessage( WM_PAINT, 0, 0 );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeUpdateQuestLog(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
#if !ISVERSION(SERVER_VERSION)

	GAME *pGame = UnitGetGame( unit );
	UNIT *pControlUnit = GameGetControlUnit( pGame );	
	if (UnitGetUltimateOwner( unit ) == pControlUnit)
	{
		// update the quest log, so that keywords will resolve correctly
		UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT, dwParam );
	}	
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
// this doesn't support fading, but it does respect
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeOpacityOverride(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int oldvalue,
	int newvalue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( unit ) );

	if (!unit->pGfx)
		return;

	if ( UnitGetStat( unit, STATS_NO_DRAW ) )
		return; // if we aren't drawing, then we shouldn't mess with opacity

	// only values between 0.0f and 1.0f make any sense here.  If you want the unit to be invisible, use STATS_NO_DRAW
	float fOpacity = *((float *)&newvalue);
	fOpacity = PIN( fOpacity, 0.0f, 1.0f );
	if ( fOpacity == 0.0f )
		fOpacity = 1.0f;

	e_ModelSetAlpha( c_UnitGetModelIdInventory( unit ),	fOpacity );
	e_ModelSetAlpha( c_UnitGetModelIdFirstPerson( unit ), fOpacity );
	e_ModelSetAlpha( c_UnitGetModelIdThirdPerson( unit ), fOpacity );
	e_ModelSetAlpha( c_UnitGetPaperdollModelId( unit ),	fOpacity );
	e_ModelSetAlpha( c_UnitGetPaperdollModelId( unit ),	fOpacity );

#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
static inline void sUnitUpdateDatabase(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
	UNIT *pOwner = UnitGetUltimateOwner( pUnit );
	GAME *pGame = UnitGetGame( pUnit );
	const STATS_DATA *pStatsData = StatsGetData( pGame, wStat );
	int nMinTicksBetweenCommits = pStatsData->m_nMinTicksBetweenDatabaseCommits;
	DBUNIT_FIELD eField = pStatsData->m_eDBUnitField;
	s_DatabaseUnitOperation( pOwner, pUnit, DBOP_UPDATE, nMinTicksBetweenCommits, eField );
#endif
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeXfire(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	REF(data);
	GAME* game = UnitGetGame(pUnit);
	if (pUnit == GameGetControlUnit(game))
	{
		XfireStateUpdateUnitState(pUnit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitStatsChangeAnimationGroupOverride(
	UNIT *pUnit,
	int wStat,
	PARAM dwParam,
	int nOldValue,
	int nNewValue,
	const STATS_CALLBACK_DATA & data,
	BOOL bSend)
{
	c_AppearanceUpdateAnimationGroup( pUnit, UnitGetGame( pUnit ), c_UnitGetModelId( pUnit ), UnitGetAnimationGroup( pUnit ) );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitInitStatsChangeCallbackTable(
	void)
{
	// server & client callbacks
	StatsAddStatsChangeCallback(STATS_VELOCITY, sUnitStatsChangeVelocity, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_VELOCITY_BONUS, sUnitStatsChangeVelocity, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_CAN_FLY, sUnitStatsChangeCanFly, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_HP_CUR, sUnitStatsChangeHpCur, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_POWER_CUR, sUnitStatsChangePowerCur, 0, TRUE, TRUE);	
	StatsAddStatsChangeCallback(STATS_CHANCE_TO_FIZZLE_MISSILES, sUnitStatsChanceToFizzleMissiles, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_CHANCE_TO_REFLECT_MISSILES, sUnitStatsChanceToReflectMissiles, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_HOLY_RADIUS, UnitUpdateHolyRadius, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_SKILL_LEVEL, sUnitStatsChangeSkillLevel, 0, TRUE, TRUE);	
	StatsAddStatsChangeCallback(STATS_SKILL_GROUP_LEVEL, sUnitStatsChangeSkillLevelGroup, 0, TRUE, TRUE);	
	StatsAddStatsChangeCallback(STATS_GOLD, sUnitStatsChangeGold, 0, TRUE, TRUE);		
	StatsAddStatsChangeCallback(STATS_MONEY_REALWORLD, sUnitStatsChangeRealWorldMoney, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_POWER_CUR, sUnitStatsChangePower, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_PET_SLOTS, sUnitStatsChangePetSlots, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_PET_POWER_COST_REDUCTION_PCT, sUnitStatsChangePetPowerCostReductionPct, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_PREVENT_ALL_SKILLS, sUnitStatsChangePreventAllSkills, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_TEAM_OVERRIDE_INDEX, sUnitStatsChangeTeamOverride, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_TEAM_OVERRIDE, sUnitStatsChangeTeamOverride, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_STATE, UnitStatsChangeState, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_PLAYER_WAITING_TO_RESPAWN, sUnitStatsChangePlayerWaitingForRespawn, 0, TRUE, TRUE);
	StatsAddStatsChangeCallback(STATS_AUTO_PARTY_ENABLED, sUnitStatsChangeAutoParty, 0, TRUE, TRUE);		
	StatsAddStatsChangeCallback(STATS_FUSE_END_TICK, sUnitStatsChangeFuseEndTick, 0, TRUE, TRUE);
	
	if (AppIsTugboat())
	{
		StatsAddStatsChangeCallback(STATS_STRENGTH, sUnitStatsChangeStrength, 0, TRUE, TRUE);	
		StatsAddStatsChangeCallback(STATS_DEXTERITY, sUnitStatsChangeDexterity, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_DAMAGE_PERCENT, sUnitStatsChangeDamage, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_DAMAGE_PERCENT_MELEE, sUnitStatsChangeDamage, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_DAMAGE_PERCENT_RANGED, sUnitStatsChangeDamage, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_DAMAGE_BONUS, sUnitStatsChangeDamage, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_EQUIPPED_SLASHING, UnitUpdateWeaponsForSkills, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_EQUIPPED_PIERCING, UnitUpdateWeaponsForSkills, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_EQUIPPED_CRUSHING, UnitUpdateWeaponsForSkills, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_EQUIPPED_MISSILE, UnitUpdateWeaponsForSkills, 0, TRUE, TRUE);
		StatsAddStatsChangeCallback(STATS_EQUIPPED_SHIELD, UnitUpdateWeaponsForSkills, 0, TRUE, TRUE);		
		StatsAddStatsChangeCallback(STATS_LEVEL, UnitLevelChanged, 0, TRUE, TRUE);				
		StatsAddStatsChangeCallback(STATS_ACHIEVEMENT_PROGRESS, UnitAchievementUpdated, 0, TRUE, TRUE);				
		StatsAddStatsChangeCallback(STATS_KARMA, sUnitStatsChangeKarmaCur, 0, TRUE, TRUE);
	}
	
#if !ISVERSION(SERVER_VERSION)
	// client only callbacks
	StatsAddStatsChangeCallback(STATS_BLOCK_MOVEMENT, sUnitStatsChangeBlockMovement, 0, FALSE, TRUE);	
	StatsAddStatsChangeCallback(STATS_SKILL_IS_SELECTED, sUnitStatsChangeSkillIsSelected, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_HOLY_RADIUS_PERCENT, UnitUpdateHolyRadius, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_HOLY_RADIUS_ENEMY_COUNT, sUnitStatsChangeEnemyCount, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_HOLY_RADIUS_FRIEND_COUNT, sUnitStatsChangeFriendCount, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_SKILL_COOLING, sUnitStatsChangeSkillCooling, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_SKILL_GROUP_COOLING, sUnitStatsChangeSkillGroupCooling, 0, FALSE, TRUE);	
	StatsAddStatsChangeCallback(STATS_TOXIC_DEGEN, sUnitStatsChangeQueueItemWindowsRepaint, 0, FALSE, TRUE);		
	StatsAddStatsChangeCallback(STATS_NO_DRAW, sUnitStatsChangeNoDraw, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_DONT_DRAW_WEAPONS, sUnitStatsChangeDontDrawWeapons, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_IDENTIFIED, sUnitStatsChangeIdentified, 0, FALSE, TRUE);		
	StatsAddStatsChangeCallback(STATS_ITEM_QUANTITY, sUnitStatsChangeItemQuantity, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_SAVE_QUEST_INFESTATION_KILLS, sUnitStatsChangeUpdateQuestLog, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_QUEST_OPERATED_OBJS, sUnitStatsChangeUpdateQuestLog, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_QUEST_NUM_COLLECTED_ITEMS, sUnitStatsChangeUpdateQuestLog, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_QUEST_CUSTOM_BOSS_CLASS, sUnitStatsChangeUpdateQuestLog, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_OPACITY_OVERRIDE, sUnitStatsChangeOpacityOverride, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_WEAPON_AMMO_CUR, sUnitStatsChangeWeaponAmmo, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_WEAPON_AMMO_CAPACITY, sUnitStatsChangeWeaponAmmo, 0, FALSE, TRUE);				
	StatsAddStatsChangeCallback(STATS_ANIMATION_GROUP_OVERRIDE, sUnitStatsChangeAnimationGroupOverride, 0, FALSE, TRUE);
		
	if (AppIsTugboat())
	{
		StatsAddStatsChangeCallback(STATS_ITEM_CLASS_LAST_USED_TICK, sUnitStatsChangeQueueItemWindowsRepaint, 0, FALSE, TRUE);
		StatsAddStatsChangeCallback(STATS_UNIT_THEME, sUnitStatsThemeChanged, 0, FALSE, TRUE);
		StatsAddStatsChangeCallback(STATS_QUEST_TIMER, sUnitQuestTimerChanged, 0, FALSE, TRUE);
	}

	// xfire stats callback
	StatsAddStatsChangeCallback(STATS_LEVEL, sUnitStatsChangeXfire, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_HP_CUR, sUnitStatsChangeXfire, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_HP_MAX_BONUS, sUnitStatsChangeXfire, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_POWER_CUR, sUnitStatsChangeXfire, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_POWER_MAX_BONUS, sUnitStatsChangeXfire, 0, FALSE, TRUE);
	StatsAddStatsChangeCallback(STATS_EXPERIENCE, sUnitStatsChangeXfire, 0, FALSE, TRUE);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
	// server only callbacks
	StatsAddStatsChangeCallback(STATS_PROC_CHANCE_ON_ATTACK, sUnitStatsChangeProcChance, 0, TRUE, FALSE);
	StatsAddStatsChangeCallback(STATS_PROC_CHANCE_ON_DID_HIT, sUnitStatsChangeProcChance, 0, TRUE, FALSE);
	StatsAddStatsChangeCallback(STATS_PROC_CHANCE_ON_DID_KILL, sUnitStatsChangeProcChance, 0, TRUE, FALSE);
	StatsAddStatsChangeCallback(STATS_PROC_CHANCE_ON_GOT_HIT, sUnitStatsChangeProcChance, 0, TRUE, FALSE);
	StatsAddStatsChangeCallback(STATS_PROC_CHANCE_ON_GOT_KILLED, sUnitStatsChangeProcChance, 0, TRUE, FALSE);
	StatsAddStatsChangeCallback(STATS_EXPERIENCE, sUnitStatsChangeExperience, 0, TRUE, FALSE);		
	StatsAddStatsChangeCallback(STATS_RANK_EXPERIENCE, sUnitStatsChangeRankExperience, 0, TRUE, FALSE);		
	StatsAddStatsChangeCallback(STATS_DIFFICULTY_CURRENT, sUnitStatsChangeDifficultyCurrent, 0, TRUE, FALSE);		
	StatsAddStatsChangeCallback(STATS_CHAR_OPTION_HIDE_HELMET, sUnitStatsChangeUpdateWardrobe, 0, TRUE, FALSE);				
#endif

	unsigned int num_stats = GetNumStats();
	for (unsigned int stat = 0; stat < num_stats; stat++)
	{
		STATS_DATA * stats_data = (STATS_DATA *)ExcelGetDataNotThreadSafe(NULL, DATATABLE_STATS, stat);
		if (!stats_data)
		{
			continue;
		}

		switch (stats_data->m_nStatsType)
		{
		case STATSTYPE_REGEN:
		case STATSTYPE_DEGEN:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeRegen, 0, TRUE, FALSE);
			}
			break;
		case STATSTYPE_REGEN_CLIENT:
		case STATSTYPE_DEGEN_CLIENT:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeRegen, 0, TRUE, TRUE);
			}
			break;
		case STATSTYPE_MAX_RATIO:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeMaxRatio, 0, TRUE, TRUE);
			}
			break;
		case STATSTYPE_MAX_RATIO_DECREASE:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeMaxRatioDecrease, 0, TRUE, TRUE);
			}
			break;
		case STATSTYPE_MAX:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeMax, 0, TRUE, TRUE);
			}
			break;
		case STATSTYPE_MAX_PURE:
			if (stats_data->m_nAssociatedStat[0] > INVALID_LINK)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeMaxPure, 0, TRUE, TRUE);
			}
			break;
		}

		if (stats_data->m_nRegenByStat[0] >= 0 || stats_data->m_nRegenByStat[1] >= 0)
		{
			StatsAddStatsChangeCallback(stat, sUnitStatsChangeRegenTarget, 0, TRUE, FALSE);
		}
		if (stats_data->m_nAssociatedStat[0] >= 0)
		{
			if (stats_data->m_nStatsType == STATSTYPE_DEGEN_PERCENT)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeDegenPercent, 0, TRUE, FALSE);
			}
			else if (stats_data->m_nStatsType == STATSTYPE_REGEN_PERCENT)
			{
				StatsAddStatsChangeCallback(stat, sUnitStatsChangeRegenPercent, 0, TRUE, FALSE);
			}
		}
		if (StatsDataTestFlag(stats_data, STATS_DATA_STATE_CAN_MONITOR_SERVER))
		{
			StatsAddStatsChangeCallback(stat, StateMonitorStat, 0, TRUE, FALSE);
		}
		else if (StatsDataTestFlag(stats_data, STATS_DATA_STATE_CAN_MONITOR_CLIENT))
		{
			StatsAddStatsChangeCallback(stat, StateMonitorStat, 0, FALSE, TRUE);
		}
#if ISVERSION(SERVER_VERSION) || ISVERSION( DEVELOPMENT )
		if (StatsDataTestFlag(stats_data, STATS_DATA_UPDATE_DATABASE))
		{
			StatsAddStatsChangeCallback(stat, sUnitUpdateDatabase, 0, TRUE, FALSE);
		}
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitSphereCheck( 
	UNIT *pTestUnit, 
	const VECTOR *pvPosition, 
	VECTOR *pvEdge, 
	float fLengthSq, 
	float fCollisionRadius, 
	float fCollisionHeight, 
	float *pfDist )
{
	if( AppIsTugboat() )
	{
		// test if ray intersects with unit (sphere intersection)
		VECTOR pos = pTestUnit->vPosition;
		VECTOR norm = *pvEdge;
		VECTOR nextpos = *pvPosition + *pvEdge;
		VectorNormalize( norm );
		float fU;
		float fU2;
		float fCollRadSquared = UnitGetCollisionRadius(pTestUnit) + fCollisionRadius;
		VECTOR pos2 = pos - norm * fCollRadSquared;
		fCollRadSquared *= fCollRadSquared;
		if (fCollRadSquared < .25f)
		{
			fCollRadSquared = .25f;
		}
		if ( fLengthSq != 0.0f )
		{
			fU =  ( ( ( pos.fY - pvPosition->fY ) * pvEdge->fY ) +
				( ( pos.fX - pvPosition->fX ) * pvEdge->fX ) );
			fU2 =  ( ( ( pos2.fY - pvPosition->fY ) * pvEdge->fY ) +
				( ( pos2.fX - pvPosition->fX ) * pvEdge->fX ) );

			fU /= fLengthSq;
			fU2 /= fLengthSq;
			BOOL Inside1 = ( fU >= 0 && fU <= 1 );
			BOOL Inside2 = ( fU2 >= 0 && fU2 <= 1 );


			// Check if point falls within the line segment

			if ( Inside1 == Inside2 &&
				 !( Inside1 && Inside2 ) )
			{
				return FALSE;
			}
		}
		else 
		{
			float fDeltaX = pos.fX - pvPosition->fX;
			float fDeltaY = pos.fY - pvPosition->fY;
			fU = fDeltaX * fDeltaX + fDeltaY * fDeltaY;
			// Check if point falls within the line segment
			if ( fU < 0 || fU > 1 )
			{
				return FALSE;
			}
		}


		VECTOR vSegmentPos = *pvPosition + *pvEdge * fU;
		vSegmentPos -= pos;
		vSegmentPos.fZ = 0;
		*pfDist = VectorLength( vSegmentPos );
		return TRUE;

	}
	else
	{
		ASSERTX(FALSE, "Cannot call sUnitSphereCheck from Hellgate");
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//
// DEBUG DRAWING IN GDI
//
//----------------------------------------------------------------------------
/*
#include "debugwindow.h"
#ifdef DEBUG_WINDOW
#include "s_gdi.h"

//----------------------------------------------------------------------------

static BOUNDING_BOX drbDebugBox;
static VECTOR drbWorldPos;
static float drbRadius;

static BOUNDING_BOX drbOthers[8];
static BOOL drbOthersHit[8];
static int drbNumOthers = 0;

//----------------------------------------------------------------------------

#define DRB_DEBUG_SCALE		10.0f

static void drbTransform( VECTOR & vPoint, int *pX, int *pY )
{
	VECTOR vTrans = vPoint - drbWorldPos;
	VectorScale( vTrans, vTrans, DRB_DEBUG_SCALE );
	*pX = (int)FLOOR( vTrans.fX ) + DEBUG_WINDOW_X_CENTER;
	*pY = (int)FLOOR( vTrans.fY ) + DEBUG_WINDOW_Y_CENTER;
}

static void drbDebugDraw()
{
	int nX1, nY1;
	drbTransform( drbDebugBox.vMin, &nX1, &nY1 );
	int nX2, nY2;
	drbTransform( drbDebugBox.vMax, &nX2, &nY2 );
	gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_red );

	gdi_drawpixel( DEBUG_WINDOW_X_CENTER, DEBUG_WINDOW_Y_CENTER, gdi_color_red );

	int nRadius = (int)FLOOR( ( drbRadius * DRB_DEBUG_SCALE ) + 0.5f );
	gdi_drawcircle( DEBUG_WINDOW_X_CENTER, DEBUG_WINDOW_Y_CENTER, nRadius, gdi_color_red );

	for ( int i = 0; i < drbNumOthers; i++ )
	{
		drbTransform( drbOthers[i].vMin, &nX1, &nY1 );
		drbTransform( drbOthers[i].vMax, &nX2, &nY2 );
		BYTE bColor;
		if ( drbOthersHit[i] )
			bColor = gdi_color_yellow;
		else
			continue;
			//bColor = gdi_color_blue;
		gdi_drawbox( nX1, nY1, nX2, nY2, bColor );
	}
}

//----------------------------------------------------------------------------

#endif
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RayIntersectsUnitCylinder(
	const UNIT * pUnit,
	const VECTOR & vRayStart,
	const VECTOR & vRayEnd,
	const float fRayThickness,
	const float fFudgeFactor,
	VECTOR * pvPointAlongRay)
{
	// The first step is to collapse everything into two dimensions
	VECTOR vPosition3D = UnitGetPosition(pUnit);
	VECTOR vRay3D = vRayEnd - vRayStart;

	VECTOR vPosition(vPosition3D.fX, vPosition3D.fY, 0.0f);
	VECTOR vRay(vRay3D.fX, vRay3D.fY, 0.0f);

	// Now we find the nearest point along the ray to the center of the unit
	float fT = VectorDot(vPosition - vRayStart, vRay);

	// Find a point along the ray
	VECTOR vPointAlongRay;
	if(fT <= 0.0f)
	{
		// Out of bounds!  Clamp to this end of the ray
		fT = 0.0f;
		vPointAlongRay = vRayStart;
	}
	else
	{
		float fRayLengthSquared = VectorLengthSquared(vRay);
		if(fT >= fRayLengthSquared)
		{
			// whoah, why clamp to the end of the ray? Things will continue
			// to collide long after they have passed through.
			if( AppIsTugboat() )
			{
				// Make sure that we're not dividing by zero here
				if(fRayLengthSquared > 0.0f)
				{
					fT /= fRayLengthSquared;
					vPointAlongRay = vRayStart + fT * vRay;
				}
				fT = 1.0f;

			}
			else
			{
				// Out of bounds on the OTHER side!  Clamp to the other end of the ray
				fT = 1.0f;
				vPointAlongRay = vRayEnd;
			}
		}
		else
		{
			// Make sure that we're not dividing by zero here
			if(fRayLengthSquared > 0.0f)
			{
				fT /= fRayLengthSquared;
				vPointAlongRay = vRayStart + fT * vRay;
			}
			else
			{
				fT = 1.0f;
				vPointAlongRay = vRayEnd;
			}
		}
	}
	vPointAlongRay.fZ = 0.0f;

	// New that we have the point, we calculate its distance from the target center
	float fDistanceToPointAlongRaySquared = VectorDistanceSquared(vPosition, vPointAlongRay);
	float fCheckCollisionRadiusSquared = UnitGetCollisionRadius(pUnit) + fRayThickness + fFudgeFactor;
	fCheckCollisionRadiusSquared *= fCheckCollisionRadiusSquared;

	// If that distance is greater than the check distance, then we haven't collided
	if(fDistanceToPointAlongRaySquared >= fCheckCollisionRadiusSquared)
	{
		return FALSE;
	}

	if( AppIsTugboat() )
	{
		if(pvPointAlongRay)
		{
			*pvPointAlongRay = vPointAlongRay;
		}
		return TRUE;
	}
	// We think we might have collided - now we check the Z distance to make sure that we've actually collided in three dimensions
	VECTOR vPointAlongRay3D = vRayStart + fT * vRay3D;
	if((vPointAlongRay3D.fZ + fRayThickness) > vPosition3D.fZ &&
		(vPointAlongRay3D.fZ - fRayThickness) < (vPosition3D.fZ + UnitGetCollisionHeight(pUnit)))
	{
		if(pvPointAlongRay)
		{
			*pvPointAlongRay = vPointAlongRay3D;
		}
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sComputeMissileBox(
	BOUNDING_BOX & missilebox, 
	const VECTOR & prevpos, 
	const VECTOR & newpos,
	float fRadius, 
	float fHeight)
{
	missilebox.vMin.x = MIN(prevpos.x, newpos.x) - fRadius;
	missilebox.vMin.y = MIN(prevpos.y, newpos.y) - fRadius;
	missilebox.vMin.z = MIN(prevpos.z, newpos.z) - fHeight;
	missilebox.vMax.x = MAX(prevpos.x, newpos.x) + fRadius;
	missilebox.vMax.y = MAX(prevpos.y, newpos.y) + fRadius;
	missilebox.vMax.z = MAX(prevpos.z, newpos.z) + fHeight;
}


//----------------------------------------------------------------------------
// used only by missiles
//----------------------------------------------------------------------------
UNIT * TestUnitUnitCollision(
	UNIT * unit,
	const VECTOR * pvPos,
	const VECTOR * pvPrevPos,
	DWORD dwTestMask)
{
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT(game);

	ROOM * room = UnitGetRoom(unit);
	if (!room)
	{
		return NULL;
	}

	// box around where we were and where we are now
	float fRadius = UnitGetCollisionRadius(unit);
	float fHeight = UnitGetCollisionHeight(unit);
	BOUNDING_BOX missilebox;
	sComputeMissileBox(missilebox, *pvPrevPos, *pvPos, fRadius, fHeight);

	VECTOR vDelta;
	vDelta = *pvPos - *pvPrevPos;
	float fXYLengthSq = ( vDelta.fX * vDelta.fX ) + ( vDelta.fY * vDelta.fY );

	const UNIT_DATA * pUnitData = UnitGetData( unit );

	if ( AppIsTugboat() )
	{
		UNIT* pTarget = UnitGetAITarget(unit);
		vDelta.fZ = 0;

		if( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_PRIORITIZE_TARGET ) && pTarget && !IsUnitDeadOrDying( pTarget ) ) 
		{
			float fDist;
			if ( sUnitSphereCheck( pTarget, pvPrevPos, &vDelta, fXYLengthSq, UnitGetCollisionRadius( unit ) + .25f, fHeight, &fDist ) )
			{
				if ( fDist < UnitGetCollisionRadius( pTarget ) + UnitGetCollisionRadius( unit ) + .25f )
					return pTarget;
			}

			VECTOR vDeltaTest = UnitGetPosition( pTarget ) - *pvPos;
			vDeltaTest.fZ = 0;
			
			float Dot = VectorDot( UnitGetFaceDirection(unit, FALSE), vDeltaTest );
			if( Dot < -( UnitGetCollisionRadius( pTarget ) + UnitGetCollisionRadius( unit ) + .25f )  )
			{
				vDeltaTest -= UnitGetFaceDirection(unit, FALSE) * Dot;
				vDeltaTest.fZ = 0;
	
				if( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_MUST_HIT ) /*||
					VectorLength( vDelta ) < .5f */)
				{
					return pTarget;
				}
			}
			else
			{
				return NULL;
			}
		}
	}


	// get near by rooms, including center room
	ROOM_LIST tNearbyRooms;
	SIMPLE_DYNAMIC_ARRAY<ROOM*> RoomsList;
	ArrayInit(RoomsList, GameGetMemPool(game), 1);		
	int nNearbyRoomCount = 0;
	if( IS_CLIENT( game ) && AppIsTugboat() )
	{
		UnitGetLevel( unit )->m_pQuadtree->GetItems( missilebox, RoomsList );	
		// add this room to the list of currently known rooms	
		nNearbyRoomCount = (unsigned int)RoomsList.Count();
	}
	else
	{
		RoomGetAdjacentRoomList(room, &tNearbyRooms, TRUE);
		nNearbyRoomCount = tNearbyRooms.nNumRooms;
	}

	BOOL bDontCollideWithDestructibles = UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_DONT_COLLIDE_DESTRUCTIBLES);

	// KCK: Override to the override
	BOOL bBlocksEverything = FALSE;
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_BLOCKS_EVERYTHING))
		bBlocksEverything = TRUE;

	// go through them all
	for( int j = 0; j < nNearbyRoomCount; j++ )
	{
		ASSERT_BREAK(j < MAX_ROOMS_PER_LEVEL);
		for (int eTargetType = TARGET_COLLISIONTEST_START; eTargetType < NUM_TARGET_TYPES; eTargetType++)
		{
			if (!(dwTestMask & MAKE_MASK(eTargetType)))
			{
				continue;
			}
			
			ROOM * pRoomToTest = NULL;
			if( IS_CLIENT( game ) && AppIsTugboat() )
			{
				pRoomToTest = RoomsList[j];
			}
			else
			{
				pRoomToTest = tNearbyRooms.pRooms[j];
			}
			ASSERT_CONTINUE(pRoomToTest);
			
			// TODO: add a check between the bounding box and the room so that we can cull from here
			
			UNIT * test = NULL;
			UNIT * next = pRoomToTest->tUnitList.ppFirst[eTargetType];
			while ((test = next) != NULL)
			{
				next = test->tRoomNode.pNext;
	
				// missiles shouldn't collide with other missiles.
				if( UnitIsA( test, UNITTYPE_MISSILE ))
				{
					continue;
				}
				if (IsUnitDeadOrDying(test))
				{
					continue;
				}

				if( AppIsTugboat() &&
					UnitIsA( test, UNITTYPE_BLOCKING_OPENABLE ) &&
					( UnitTestMode( test, MODE_OPEN ) ||
					UnitTestMode( test, MODE_OPENED ) ) )
				{
					continue;
				}

				if( AppIsTugboat() &&
					UnitIsA( test, UNITTYPE_HEADSTONE ) )
				{
					continue;
				}
				
				if ( !bBlocksEverything &&
					 ( AppIsHellgate() || UnitGetGenus( test ) != GENUS_OBJECT ) &&  //tugboat collides here against objects.Objects fail here
					 ( UnitGetTeam( test ) == INVALID_TEAM  || !TestHostile(game, unit, test)) )
				{
					continue;
				}

				if ( !bBlocksEverything && pUnitData->nOnlyCollideWithUnitType != INVALID_ID && pUnitData->nOnlyCollideWithUnitType != UNITTYPE_NONE &&
					 !UnitIsA( test, pUnitData->nOnlyCollideWithUnitType ) )
					 continue;

				if ( !bBlocksEverything && bDontCollideWithDestructibles && UnitIsA(test, UNITTYPE_DESTRUCTIBLE))
				{
					continue;
				}
				if ( !UnitGetCanTarget(test))
				{
					continue;
				}
	
				BOUNDING_BOX target;
				target.vMin = UnitGetPosition( test );
				target.vMax = target.vMin;
				VECTOR vTemp = target.vMin;
				float fTestRadius = UnitGetCollisionRadius( test );
				if( AppIsTugboat() )
				{
					fTestRadius += UnitGetCollisionRadius( unit ) + .25f;
				}
				target.vMin.fX -= fTestRadius;
				target.vMin.fY -= fTestRadius;
				vTemp.fX += fTestRadius;
				vTemp.fY += fTestRadius;
				vTemp.fZ += UnitGetCollisionHeight( test );
				BoundingBoxExpandByPoint( &target, &vTemp );
				if( AppIsTugboat() )
				{
					// we don't allow hitting the same unit twice in a row.
					if ( unit->idLastUnitHit != UnitGetId( test ) &&
						 BoundingBoxCollideXY( &missilebox, &target, 0.1f ) )
					{
						float fDist;
						if ( sUnitSphereCheck( test, pvPrevPos, &vDelta, fXYLengthSq, UnitGetCollisionRadius( unit ) + .25f, fHeight, &fDist ) )
						{
							if ( fDist < UnitGetCollisionRadius( test ) + UnitGetCollisionRadius( unit ) + .25f )
								return test;
						}
					}
				}
				else if( AppIsHellgate() )
				{
					if ( BoundingBoxCollide( &missilebox, &target, 0.1f ) )
					{
						if(RayIntersectsUnitCylinder(test, *pvPrevPos, *pvPos, UnitGetCollisionRadius( unit ), 0.0f))
						{
							return test;
						}
					}
				}
			}
		}
	}
	RoomsList.Destroy();

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMonsterGetAffixDesc(
	GAME *game,
	UNIT *unit,
	WCHAR *uszDesc,
	int nDescLen)
{
	ASSERT( uszDesc );
	*uszDesc = 0;
	
	// append affixes (if any) - this is temporary, feel free to remove when we have a real ui
	//																	(oh that's harsh)
	//																		haha u know what i mean ;)

	// get monster name attributes
	DWORD dwNameAttributes = 0;
	const UNIT_DATA *pUnitData = UnitGetData( unit );
	if (pUnitData)
	{
		int nStringIndex = pUnitData->nString;
		if (nStringIndex != INVALID_LINK)
		{
			StringTableGetStringByIndexVerbose( 
				nStringIndex,
				0,
				0,
				&dwNameAttributes,
				NULL);
		}
	}
			
	STATS_ENTRY list[16];
	int nAffixCount = UnitGetStatsRange( unit, STATS_APPLIED_AFFIX, 0, list, 16 );
	if (nAffixCount > 0)
	{
			
		// add affix descriptive names
		for (int i = 0; i < nAffixCount; ++i)
		{
			int nAffix = list[ i ].value;
			const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
			if (pAffixData)
			{
				if (PStrLen(uszDesc) > 0)
				{
					PStrCat( uszDesc, L" ", nDescLen );
				}

				// get affix on monster without any name modification tokens
				WCHAR uszAffixNameClean[ MAX_ITEM_NAME_STRING_LENGTH ];				
				AffixGetMagicName(
					nAffix,
					dwNameAttributes,
					uszAffixNameClean,
					MAX_ITEM_NAME_STRING_LENGTH,
					TRUE);
					
				PStrCat( uszDesc, uszAffixNameClean, nDescLen );

			}
			
		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMonsterGetName(
	UNIT *unit,
	WCHAR *uszName,
	int nNameLen,
	DWORD dwFlags)
{
	const UNIT_DATA * monster_data = MonsterGetData(NULL, UnitGetClass(unit));
	ASSERT(monster_data);

	// get monster name, if a unique name is present we will use that in place of the definition name
	const WCHAR* puszMonsterName = NULL;
	int nMonsterUniqueNameIndex = UnitGetStat( unit, STATS_MONSTER_UNIQUE_NAME );
	#define MAX_NAME_LENGTH	(128)
	WCHAR uszNameWithTitle[ MAX_NAME_LENGTH ];
	DWORD dwAttributesOfString = 0;
	if (nMonsterUniqueNameIndex != MONSTER_UNIQUE_NAME_INVALID)
	{
		MonsterGetUniqueNameWithTitle( 
			nMonsterUniqueNameIndex, 
			UnitGetClass( unit ),
			uszNameWithTitle,
			MAX_NAME_LENGTH,
			&dwAttributesOfString);
		puszMonsterName = uszNameWithTitle;
	}
	else
	{
	
		// get the override name string (if any)
		int nStringIndex = UnitGetNameOverride( unit );
		
		// if nothing else, use the string from the monster row
		if (nStringIndex == INVALID_LINK)
		{
			nStringIndex = monster_data->nString;
		}
		
		// get name
		puszMonsterName = StringTableGetStringByIndexVerbose(
			nStringIndex,
			0,
			0,
			&dwAttributesOfString,
			NULL);		
	}
	ASSERT(puszMonsterName);

	// get quality data
	BOOL bHasQualityName = FALSE;
	WCHAR uszTemp[MAX_ITEM_NAME_STRING_LENGTH] = { 0 };
	int nMonsterQuality = UnitGetStat(unit, STATS_MONSTERS_QUALITY);			
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( NULL, nMonsterQuality );
		
		// get quality name (if no unique name is present, unique names look silly with qualities)
		if (nMonsterUniqueNameIndex == MONSTER_UNIQUE_NAME_INVALID && 
			!UnitDataTestFlag(monster_data, UNIT_DATA_FLAG_NO_RANDOM_PROPER_NAME) &&
			pMonsterQualityData->nChampionFormatStringKey != INVALID_LINK)
		{

			// get quality format string (ie "Rare [object]" or "Legendary [object]")
			const WCHAR *puszQualityFormat = StringTableGetStringByIndexVerbose(
				pMonsterQualityData->nChampionFormatStringKey,
				dwAttributesOfString,
				0,
				NULL,
				NULL);			

			// copy format string to tmep
			PStrCopy( uszTemp, puszQualityFormat, MAX_ITEM_NAME_STRING_LENGTH );
			
			// setup tokens
			const WCHAR *puszObjectToken = StringReplacementTokensGet( SR_OBJECT );
			
			// replace name and quality according to format string
			PStrReplaceToken( uszTemp, MAX_ITEM_NAME_STRING_LENGTH, puszObjectToken, puszMonsterName );

			// this monster has a quality name
			bHasQualityName = TRUE;
							
		}

	}
		
	// if no quality name was found, just use the monster name
	if (bHasQualityName == FALSE)
	{
		PStrCopy(uszTemp, puszMonsterName, MAX_ITEM_NAME_STRING_LENGTH);
	}
	
	// append to string
	PStrCat(uszName, uszTemp, nNameLen);
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL UnitGetName(
	UNIT* unit,
	WCHAR* szName,
	int len,
	DWORD dwUnitNameFlags,
	DWORD *pdwColor /* = NULL*/)
{
	static const WCHAR* szUnknown = L"???";

	szName[0] = L'\0';

	ASSERT(unit);
	if (!unit)
	{
		PStrCat(szName, szUnknown, len);
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		PStrCat(szName, szUnknown, len);
		return FALSE;
	}

	int color = UnitGetNameColor(unit);
	if (color != FONTCOLOR_INVALID)
	{
		if (pdwColor)
		{
			*pdwColor = GetFontColor(color);
		}
		if (TESTBIT( dwUnitNameFlags, UNF_EMBED_COLOR_BIT ))
		{
			UIAddColorToString(szName, (FONTCOLOR)color, len);
		}
	}

	// if has a custom name use it ... note that we're not doing this for headstones
	// because they store the players name in the custom name, but we want to put
	// additional information around it in the ObjectGetName() function
	BOOL bFound = FALSE;
	if (unit->szName && UnitIsA( unit, UNITTYPE_HEADSTONE ) == FALSE)
	{
		// titles
		if (AppIsHellgate() && 
			TESTBIT( dwUnitNameFlags, UNF_INCLUDE_TITLE_BIT ) &&
			UnitGetGenus(unit) == GENUS_PLAYER)
		{
			WCHAR uszTitle[1024];
			int nCurrentTitleID = x_PlayerGetTitleStringID(unit);
			if (nCurrentTitleID != INVALID_LINK)
			{
				PStrCopy(uszTitle, StringTableGetStringByIndex(nCurrentTitleID), len);
				PStrReplaceToken(uszTitle, arrsize(uszTitle), L"[PLAYERNAME]", unit->szName);
				PStrCat(szName, uszTitle, len);
				bFound = TRUE;
			}
		}

		if (!bFound)
		{
			PStrCat(szName, unit->szName, len);
			bFound = TRUE;
		}
	}

	if (!bFound)
	{
		switch (UnitGetGenus(unit))
		{
			//----------------------------------------------------------------------------
			case GENUS_PLAYER:
			{
				static const WCHAR* szPlayer = DEFAULT_PLAYER_NAME;
				PStrCat(szName, szPlayer, len);
				bFound = TRUE;
				break;			
			}
			
			//----------------------------------------------------------------------------
			case GENUS_MONSTER:
			{
				WCHAR szTemp[MAX_ITEM_NAME_STRING_LENGTH] = L"";
				sMonsterGetName(unit, szTemp, arrsize(szTemp), dwUnitNameFlags);
				PStrCat(szName, szTemp, len);
				bFound = TRUE;
				break;
			}
			
			//----------------------------------------------------------------------------
			case GENUS_MISSILE:
			{
				const UNIT_DATA* missile_data = MissileGetData(game, unit);
				if (!missile_data)
				{
					break;
				}
				WCHAR szTemp[MAX_ITEM_NAME_STRING_LENGTH] = L"";
				PStrCvt(szTemp, missile_data->szName, arrsize(szTemp));
				PStrCat(szName, szTemp, len);
				bFound = TRUE;
				break;
			}
			
			//----------------------------------------------------------------------------
			case GENUS_ITEM:
			{
				WCHAR szTemp[MAX_ITEM_NAME_STRING_LENGTH] = L"";
				bFound = ItemGetName(unit, szTemp, arrsize(szTemp), dwUnitNameFlags);
				PStrCat(szName, szTemp, len);
				break;
			}			
			
			//----------------------------------------------------------------------------
			case GENUS_OBJECT:
			{
				WCHAR szTemp[MAX_ITEM_NAME_STRING_LENGTH] = L"";
				bFound = ObjectGetName(game, unit, szTemp, arrsize(szTemp), dwUnitNameFlags);			
				PStrCat(szName, szTemp, len);
				break;			
			}
			
		}
		
	}

	if (bFound)
	{
		// add dev name in some debug modes
		if (UnitDebugTestLabelFlags() || gbDisplayCollisionHeightLadder)
		{
			const char *pszDevName = UnitGetDevName( unit );
			if (pszDevName)
			{
				const int MAX_LENGTH = 128;
				WCHAR uszDevName[ MAX_LENGTH ];
				PStrCvt( uszDevName, pszDevName, arrsize(uszDevName) );
				PStrCat( szName, L" (", len );
				PStrCat( szName, uszDevName, len );
				PStrCat( szName, L")", len );
			}
		}

	}
	else
	{
		PStrCat(szName, szUnknown, len);
	}

	// add additional debug info if requested	
	if ( IS_CLIENT( unit ) )
	{
		const int MAX_STRING = 128;
		WCHAR uszString[ MAX_STRING ];
		uszString[ 0 ] = 0;  // terminate
			
		if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_POSITION_BIT ) )
		{
			const VECTOR &vPosition = UnitGetPosition( unit );
			PStrPrintf( 
				uszString, 
				arrsize(uszString), 
				L"\npos=(%.2f,%.2f,%.2f)", 
				vPosition.fX, 
				vPosition.fY, 
				vPosition.fZ);
				
			PStrCat( szName, uszString, len );

			const VECTOR &vDirection = UnitGetFaceDirection( unit, FALSE );
			PStrPrintf( 
				uszString, 
				arrsize(uszString), 
				L"\ndir=(%.2f,%.2f,%.2f)", 
				vDirection.fX, 
				vDirection.fY, 
				vDirection.fZ);

			PStrCat( szName, uszString, len );

			UNIT * pControlUnit = GameGetControlUnit(game);
			if(pControlUnit)
			{
				const VECTOR &vControlUnitPosition = UnitGetPosition( pControlUnit );
				float fDistance = VectorLength(vControlUnitPosition - vPosition);
				PStrPrintf( 
					uszString, 
					arrsize(uszString), 
					L"\ndistance=%.2f", 
					fDistance);

				PStrCat( szName, uszString, len );
			}

			UNIT *pContainer = UnitGetContainer( unit );
			if (pContainer)
			{
				INVENTORY_LOCATION tInvLoc;
				UnitGetInventoryLocation( unit, &tInvLoc );
				const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, tInvLoc.nInvLocation );
				ASSERT( pInvLocData );				
				PStrPrintf( 
					uszString, 
					arrsize(uszString), 
					L"\nInvLoc = %d (%S), loc XY = (%d,%d)", 
					tInvLoc.nInvLocation,
					pInvLocData->szName,
					tInvLoc.nX,
					tInvLoc.nY);
				PStrCat( szName, uszString, len );
				
			}
			
		}

		if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_ID_BIT ))
		{
			PStrPrintf( uszString, arrsize(uszString), L"\nID=%d", UnitGetId( unit ) );
			PStrCat( szName, uszString, len );
			PStrPrintf( uszString, arrsize(uszString), L"\nGUID=%I64d", UnitGetGuid( unit ) );
			PStrCat( szName, uszString, len );
		}
	
		if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_ROOM_BIT ))
		{
			ROOM *pRoom = UnitGetRoom( unit );
			PStrPrintf( 
				uszString, 
				arrsize(uszString), 
				L"%d", 
				pRoom ? RoomGetId( pRoom ) : INVALID_ID);
			PStrCat( szName, L"\nRoom ID=", len );		
			UIColorCatString(szName, len, FONTCOLOR_LIGHT_YELLOW, uszString);
			
			PStrPrintf(
				uszString,
				arrsize(uszString),
				L" (%S)",
				pRoom ? RoomGetDevName( pRoom ) : "Unknown" );
			UIColorCatString(szName, len, FONTCOLOR_VERY_LIGHT_BLUE, uszString);
			
		}

		if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_FACTION_BIT ))
		{
			int nNumFactions = FactionGetNumFactions();
			for (int i = 0; i < nNumFactions; ++i)
			{
				int nScore = FactionScoreGet( unit, i );
				if (nScore > 0)
				{
					const int MAX_STRING = 1024;
					WCHAR uszBuffer[ MAX_STRING ] = { 0 };
					FactionDebugString( i, nScore, uszBuffer, MAX_STRING );
					PStrCat( szName, L"\n", len );
					PStrCat( szName, uszBuffer, len );
				}
			}
		}

		if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_VELOCITY_BIT ))
		{
			PStrPrintf( uszString, arrsize(uszString), L"\nVel=%.2f", UnitGetVelocity( unit ) );
			PStrCat( szName, uszString, len );		
		}
				
	}
		
	// get additional name decoration
	if (AppIsTugboat() && PetIsPet( unit ))
	{
		PStrCat( szName, L" ", len );
		PStrCat( szName, GlobalStringGet( GS_PET ), len );
	}

	if (gflNameHeightOverride != -1)
	{
		WCHAR uszNameHeight[ MAX_NAME_LENGTH ];
		PStrPrintf( uszNameHeight, arrsize(uszNameHeight), L"<%.2f>", gflNameHeightOverride );
		PStrCat( szName, uszNameHeight, len );
	}

	if (color != FONTCOLOR_INVALID && TESTBIT( dwUnitNameFlags, UNF_EMBED_COLOR_BIT ))
	{
		UIAddColorReturnToString(szName, len);
	}

	return bFound;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasMagicName(
	UNIT* unit)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		return FALSE;
	}

	if (unit->szName)
	{
		return FALSE;
	}

	switch (UnitGetGenus(unit))
	{
	case GENUS_PLAYER:
		break;
	case GENUS_MONSTER:
		// Might want to put something here for rare monsters
		break;
	case GENUS_MISSILE:
		break;
	case GENUS_ITEM:
		{
			STATS_ENTRY list[ MAX_AFFIX_ARRAY_PER_UNIT ];
			int count = UnitGetStatsRange(unit, STATS_APPLIED_AFFIX, 0, list, AffixGetMaxAffixCount() );
			const AFFIX_DATA* dom = NULL;
			const AFFIX_DATA* sub = NULL;
			for (int ii = 0; ii < count; ii++)
			{
				int affix = list[ii].value;
				const AFFIX_DATA* affix_data = AffixGetData(affix);
				if (!affix_data)
				{
					continue;
				}
				if (affix_data->nStringName[ ANT_MAGIC ] <= INVALID_LINK)
				{
					continue;
				}
				if (!dom ||
					affix_data->nDominance > dom->nDominance)
				{
					dom = affix_data;
				}
			}
			if (!dom)
			{
				// loop through all affix other than dom, and append sub name of the highest dominance one
				int nHighestSubDom = -1;
				for (int ii = 0; ii < count; ii++)
				{
					int affix = list[ii].value;
					const AFFIX_DATA* affix_data = AffixGetData(affix);
					if (!affix_data)
					{
						continue;
					}
					if (!sub ||
						nHighestSubDom < affix_data->nDominance)
					{
						sub = affix_data;
						nHighestSubDom = affix_data->nDominance;
					}
				}
			}

			if (dom && dom->nStringName[ ANT_MAGIC ] > INVALID_LINK)
			{
				return TRUE;
			}
			if (sub && sub->nStringName[ ANT_MAGIC ] > INVALID_LINK)
			{
				return TRUE;
			}
		}
		break;
	case GENUS_OBJECT:
		break;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float c_UnitGetHeightFactor(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	int idAppearance = c_UnitGetAppearanceId( pUnit );
	return c_AppearanceGetHeightFactor( idAppearance );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetShape(
	UNIT *pUnit,
	float fHeightPercent,
	float fWeightPercent )
{
	ASSERT_RETURN( pUnit );
	if ( ! pUnit->pAppearanceShape )
		return;

	const UNIT_DATA * pUnitData = UnitGetData( pUnit );

	const APPEARANCE_SHAPE_CREATE & tShapeCreate = pUnitData->tAppearanceShapeCreate;

	BYTE bHeight = tShapeCreate.bHeightMin + (BYTE)(fHeightPercent * (tShapeCreate.bHeightMax - tShapeCreate.bHeightMin));

	int nWeightMin = 0;
	int nWeightMax = 255;
	if ( tShapeCreate.bUseLineBounds )
	{
		nWeightMin = (int) (tShapeCreate.bWeightLineMin0 + 
			fHeightPercent * ( tShapeCreate.bWeightLineMax0 - tShapeCreate.bWeightLineMin0 ));

		nWeightMax = (int) (tShapeCreate.bWeightLineMin1 + 
			fHeightPercent * ( tShapeCreate.bWeightLineMax1 - tShapeCreate.bWeightLineMin1 ));

		if ( nWeightMin < tShapeCreate.bWeightMin )
			nWeightMin = tShapeCreate.bWeightMin;
		if ( nWeightMax > tShapeCreate.bWeightMax )
			nWeightMax = tShapeCreate.bWeightMax;

		nWeightMin = MIN( nWeightMin, nWeightMax );
	} else {
		nWeightMin = tShapeCreate.bWeightMin;
		nWeightMax = tShapeCreate.bWeightMax;
	}

	pUnit->pAppearanceShape->bWeight = (BYTE)(nWeightMin + (int)(fWeightPercent * (nWeightMax - nWeightMin)));
	pUnit->pAppearanceShape->bHeight = bHeight;

	if ( IS_CLIENT( pUnit ) )
	{
		int nModelId = c_UnitGetModelIdThirdPerson( pUnit );
		if ( nModelId != INVALID_ID )
			c_AppearanceShapeSet( nModelId, *pUnit->pAppearanceShape );
	}
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGetShape(
	UNIT *pUnit,
	float& fHeightPercent,
	float& fWeightPercent )
{
	ASSERT_RETURN( pUnit );
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );

	const APPEARANCE_SHAPE_CREATE & tShapeCreate = pUnitData->tAppearanceShapeCreate;

	if ( tShapeCreate.bHeightMax == tShapeCreate.bHeightMin )
		fHeightPercent = 1.0f;
	else
		fHeightPercent = (float)(pUnit->pAppearanceShape->bHeight - tShapeCreate.bHeightMin) / (float)(tShapeCreate.bHeightMax - tShapeCreate.bHeightMin);

	int nWeightMin = 0;
	int nWeightMax = 255;
	if ( tShapeCreate.bUseLineBounds )
	{
		nWeightMin = (int) (tShapeCreate.bWeightLineMin0 + 
			fHeightPercent * ( tShapeCreate.bWeightLineMax0 - tShapeCreate.bWeightLineMin0 ));

		nWeightMax = (int) (tShapeCreate.bWeightLineMin1 + 
			fHeightPercent * ( tShapeCreate.bWeightLineMax1 - tShapeCreate.bWeightLineMin1 ));

		if ( nWeightMin < tShapeCreate.bWeightMin )
			nWeightMin = tShapeCreate.bWeightMin;
		if ( nWeightMax > tShapeCreate.bWeightMax )
			nWeightMax = tShapeCreate.bWeightMax;

		nWeightMin = MIN( nWeightMin, nWeightMax );
	} else {
		nWeightMin = tShapeCreate.bWeightMin;
		nWeightMax = tShapeCreate.bWeightMax;
	}

	if ( nWeightMax == nWeightMin )
		fWeightPercent = 1.0;
	else
		fWeightPercent = (float)(pUnit->pAppearanceShape->bWeight - (BYTE)nWeightMin) / (float)(nWeightMax - nWeightMin);
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct APPEARANCE_DEFINITION * UnitGetAppearanceDef(
	const UNIT* unit)
{
	int nDefId = UnitGetAppearanceDefId( unit );
	if ( nDefId == INVALID_ID )
		return NULL;
	return (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nDefId );
}

//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
BOOL IsInAttackRange( 
					 UNIT* pUnit,
					 GAME *pGame, 
					 UNIT *pTarget,
					 int nSkill,
					 BOOL bLineOfSightCheck )
{


	float LongestRange = 1.0f;
	BOOL UsingWeapon = FALSE;
	float DistanceSquared = UnitsGetDistanceSquaredXY( pUnit, pTarget );


	if( nSkill == INVALID_ID )
	{
		return FALSE;
	}
	
	if( nSkill != INVALID_ID )
	{
		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

		BOOL bObstructed = FALSE;
		int nWeapons = 0;
		for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
		{
			if ( !pWeapons[ i ] )
				continue;
			nWeapons++;
		}

		for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
		{
			if ( !pWeapons[ i ] )
				continue;
	  
			int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ i ] );
			if ( nWeaponSkill == INVALID_ID )
				continue;

			float fSkillRange = SkillGetRange( pUnit, nWeaponSkill, pWeapons[i]  );
			fSkillRange += UnitGetData( pWeapons[i] )->fServerMissileOffset;
			if( bLineOfSightCheck  && ( fSkillRange * fSkillRange ) >= DistanceSquared )
			{
				const SKILL_DATA * pSkillData = SkillGetData( pGame, nWeaponSkill );
				if( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_RANGED ) &&
					SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) )
				{
					if( !UnitInLineOfSight( pGame, pUnit, pTarget ) )
					{
						bObstructed = TRUE;
						fSkillRange = 0;
					}
				}
			}

			if( fSkillRange > LongestRange )
			{
				LongestRange = fSkillRange;
			}
			if( nSkill != nWeaponSkill )
			{
				int nCheckSkill = nSkill;
				if ( !SkillCanStart( pGame, pUnit, nSkill, UnitGetId( pWeapons[i] ), pTarget, FALSE, FALSE, SKILL_START_FLAG_IGNORE_COOLDOWN ) )
				{
					const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
					for ( int i = 0; i < NUM_FALLBACK_SKILLS; i++ )
					{
						if ( pSkillData->pnFallbackSkills[ i ] != INVALID_ID )
						{
							nCheckSkill = pSkillData->pnFallbackSkills[ i ];
						}
					}
				}

				fSkillRange = SkillGetRange( pUnit, nCheckSkill, pWeapons[i]  );
				fSkillRange += UnitGetData( pWeapons[i] )->fServerMissileOffset;
				if( bLineOfSightCheck  && ( fSkillRange * fSkillRange ) >= DistanceSquared )
				{
					const SKILL_DATA * pSkillData = SkillGetData( pGame, nCheckSkill );
					if( SkillDataTestFlag( pSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) &&
						nWeapons == 1 &&
						bObstructed )
					{
						fSkillRange = 0;
					}
					if( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_RANGED ) &&
						SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) )
					{
						if( !UnitInLineOfSight( pGame, pUnit, pTarget ) )
						{
							fSkillRange = 0;
						}
					}
				}

				if( fSkillRange > LongestRange )
				{
					LongestRange = fSkillRange;
				}
			}
			UsingWeapon = TRUE;

		}

	}
	if( !UsingWeapon )
	{
		int nCheckSkill = nSkill;
		if ( !SkillCanStart( pGame, pUnit, nSkill, INVALID_ID, pTarget, FALSE, FALSE, SKILL_START_FLAG_IGNORE_COOLDOWN ) )
		{
			const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
			for ( int i = 0; i < NUM_FALLBACK_SKILLS; i++ )
			{
				if ( pSkillData->pnFallbackSkills[ i ] != INVALID_ID )
				{
					nCheckSkill = pSkillData->pnFallbackSkills[ i ];
				}
			}
		}
		float fSkillRange = SkillGetRange( pUnit, nCheckSkill  );
		if( bLineOfSightCheck  && ( fSkillRange * fSkillRange ) >= DistanceSquared )
		{
			const SKILL_DATA * pSkillData = SkillGetData( pGame, nCheckSkill );
			if( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_RANGED ) &&
				SkillDataTestFlag( pSkillData, SKILL_FLAG_CHECK_LOS ) )
			{
				if( !UnitInLineOfSight( pGame, pUnit, pTarget ) )
				{
					fSkillRange = 0;
				}
			}
		}
		if( fSkillRange > LongestRange )
		{
			LongestRange = fSkillRange;
		}
		UsingWeapon = TRUE;
	}
	if( !UsingWeapon )
	{
		float fSkillRange = pUnit->pUnitData->fMeleeRangeDesired;
		if( fSkillRange > LongestRange )
		{
			LongestRange = fSkillRange;
		}
	}

	return( UnitsAreInRange( pUnit, pTarget, 0.0f, LongestRange - 1, &DistanceSquared ) );


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetLookGroup(
	UNIT* unit)
{
	ASSERT_RETZERO(unit);

	GAME* game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	if (UnitGetGenus(unit) != GENUS_ITEM)
	{
		return INVALID_LINK;
	}

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
	ASSERT_RETZERO(item_data);

	// look for look group set in stat by server
	int nItemLookGroup = UnitGetStat( unit, STATS_ITEM_LOOK_GROUP );
	if (nItemLookGroup != INVALID_LINK)
	{
		return nItemLookGroup;
	}
	
	// look group from affixes
	STATS_ENTRY list[16];
	int count = UnitGetStatsRange(unit, STATS_APPLIED_AFFIX, 0, list, 16);
	const AFFIX_DATA* dom = NULL;
	for (int ii = 0; ii < count; ii++)
	{
		int affix = list[ii].value;
		const AFFIX_DATA* affix_data = (const AFFIX_DATA*)ExcelGetData(game, DATATABLE_AFFIXES, affix);
		if (!affix_data)
		{
			continue;
		}
		if (affix_data->nLookGroup <= INVALID_LINK)
		{
			continue;
		}
		if (!dom)
		{
			dom = affix_data;
		}
		else if (affix_data->nDominance > dom->nDominance)
		{
			dom = affix_data;
		}
	}
	if (dom && dom->nLookGroup > INVALID_LINK)
	{
		return dom->nLookGroup;
	}

	// next look group from item quality
	int quality = ItemGetQuality( unit );
	if (quality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData(quality);
		ASSERT_RETVAL(quality_data, INVALID_LINK);
		if (quality_data->nLookGroup > INVALID_LINK)
		{
			return quality_data->nLookGroup;
		}
	}
	
	return INVALID_LINK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetClassString(
	UNIT* unit,
	WCHAR* szName,
	int len)
{
	enum
	{
		TEMP_STR_LEN = 128,
	};
	static const WCHAR* szUnknown = L"???";

	*szName = 0;

	ASSERT(unit);
	if (!unit)
	{
		PStrCat(szName, szUnknown, len);
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		PStrCat(szName, szUnknown, len);
		return FALSE;
	}

	switch (UnitGetGenus(unit))
	{
	case GENUS_PLAYER:
		{
			const UNIT_DATA * player_data = PlayerGetData(game, UnitGetClass(unit));
			ASSERT_BREAK(player_data);

			const WCHAR* str = StringTableGetStringByIndex(player_data->nString);
			ASSERT_BREAK(str);

			PStrCat(szName, str, len);
			return TRUE;
		}
		break;
	case GENUS_MONSTER:
		{
			const UNIT_DATA * monster_data = MonsterGetData(game, UnitGetClass(unit));
			ASSERT_BREAK(monster_data);

			const WCHAR* str = StringTableGetStringByIndex(monster_data->nString);
			ASSERT_BREAK(str);

			WCHAR szTemp[TEMP_STR_LEN];
			PStrCopy(szTemp, str, TEMP_STR_LEN);
			PStrCat(szName, szTemp, len);
			return TRUE;
		}
		break;
	case GENUS_MISSILE:
		{
			const UNIT_DATA* missile_data = MissileGetData(game, unit);
			if (!missile_data)
			{
				break;
			}
			WCHAR szTemp[TEMP_STR_LEN];
			PStrCvt(szTemp, missile_data->szName, TEMP_STR_LEN);
			PStrCat(szName, szTemp, len);
			return TRUE;
		}
		break;
	case GENUS_ITEM:
		{
			const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
			ASSERT_BREAK(item_data);

			const WCHAR* str = StringTableGetStringByIndex(item_data->nString);
			ASSERT_BREAK(str);

			WCHAR szTemp[TEMP_STR_LEN];
			PStrCopy(szTemp, str, TEMP_STR_LEN);
			PStrCat(szName, szTemp, len);
			return TRUE;
		}
		break;
	case GENUS_OBJECT:
		{
			const UNIT_DATA * object_data = ObjectGetData(game, UnitGetClass(unit));
			ASSERT_BREAK(object_data);

			const WCHAR* str = StringTableGetStringByIndex(object_data->nString);
			ASSERT_BREAK(str);

			WCHAR szTemp[TEMP_STR_LEN];
			PStrCopy(szTemp, str, TEMP_STR_LEN);
			PStrCat(szName, szTemp, len);
			return TRUE;
		}
		break;
	}

	PStrCat(szName, szUnknown, len);
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetTypeString(
	UNIT* unit,
	WCHAR* szType,
	int len,
	BOOL bLastOnly /*= FALSE*/,
	DWORD *pdwAttributesOfTypeString /*= NULL*/)
{
	enum
	{
		TEMP_STR_LEN = 128,
	};
	static const WCHAR* szUnknown = L"???";

	*szType = 0;

	ASSERT(unit);
	if (!unit)
	{
		PStrCat(szType, szUnknown, len);
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		PStrCat(szType, szUnknown, len);
		return FALSE;
	}

	// we want to match attributes of quantity
	int nQuantity = ItemGetQuantity( unit );
	DWORD dwAttributesToMatch = StringTableGetGrammarNumberAttribute( nQuantity > 1 ? PLURAL : SINGULAR );

	// also want to match attributes of gender
	GENDER eGender = UnitGetGender( unit );
	if (eGender != GENDER_INVALID)
	{
		dwAttributesToMatch |= StringTableGetGenderAttribute( eGender );
	}
	
	// First check and see if there's a description in the item definition
	if (UnitGetGenus(unit) == GENUS_ITEM)
	{
		const UNIT_DATA* item_data = (UNIT_DATA*)ExcelGetData(game, DATATABLE_ITEMS, UnitGetClass(unit));
		if (item_data)
		{
			if (item_data->nTypeDescrip > INVALID_LINK)
			{
				const WCHAR * str = StringTableGetStringByIndexVerbose(
					item_data->nTypeDescrip, 
					dwAttributesToMatch, 
					0, 
					pdwAttributesOfTypeString, 
					NULL);
				PStrCat(szType, str, len);
				return TRUE;
			}
		}
	}

	int nNumRows = ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_UNITTYPES);
	if (bLastOnly)
	{
		for (int iTypeRow = nNumRows - 1; iTypeRow >= 0; iTypeRow--)
		{
			const UNITTYPE_DATA* unittype_data = (UNITTYPE_DATA*)ExcelGetData(NULL, DATATABLE_UNITTYPES, iTypeRow);
			if (unittype_data != NULL && UnitIsA(unit, iTypeRow))
			{
				int nString = unittype_data->nDisplayName;
				if (nString != INVALID_LINK)
				{
					const WCHAR * str = StringTableGetStringByIndexVerbose(
						nString, 
						dwAttributesToMatch, 
						0, 
						pdwAttributesOfTypeString, 
						NULL);
					PStrCopy(szType, str, len);
					break;
				}
			}
		}
	}
	else
	{
		for (int iTypeRow = 0; iTypeRow < nNumRows; iTypeRow++)
		{
			const UNITTYPE_DATA* unittype_data = (UNITTYPE_DATA*)ExcelGetData(NULL, DATATABLE_UNITTYPES, iTypeRow);
			if (unittype_data != NULL && UnitIsA(unit, iTypeRow))
			{
				int nString = unittype_data->nDisplayName;
				if (nString != INVALID_LINK)
				{
					if (PStrLen(szType) > 0)
					{
						PStrCat(szType, L" - ", len);
					}
					const WCHAR * str = StringTableGetStringByIndexVerbose(
						nString, 
						dwAttributesToMatch, 
						0, 
						pdwAttributesOfTypeString, 
						NULL);
					if (str)
					{
						PStrCat(szType, str, len);
					}
				}
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetQualityString(
	UNIT* unit,
	WCHAR *uszBuffer,
	int nBufferLen,
	DWORD dwUnitNameFlags,
	DWORD dwStringAttributesToMatch /*= -1*/)
{
	*uszBuffer = 0;
	ASSERT_RETFALSE(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	ASSERT_RETFALSE(UnitGetGenus(unit) == GENUS_ITEM);
	int quality = ItemGetQuality( unit );
	if (quality <= INVALID_LINK)
	{
		return FALSE;
	}
	const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData(quality);
	ASSERT_RETFALSE(quality_data);

	int nString = quality_data->nDisplayName;
	if (TESTBIT( dwUnitNameFlags, UNF_INCLUDE_ITEM_TOKEN_WITH_ITEM_QUALITY ))
	{
		nString = quality_data->nDisplayNameWithItemFormat;
	}
	
	if (nString <= INVALID_LINK)
	{
		return TRUE;
	}

	// we will attempt to match the string attributes passed in (if any) or the 
	// string attributes of the item noun itself
	DWORD dwAttributesToMatch = 0;
	if (TESTBIT( dwUnitNameFlags, UNF_OVERRIDE_ITEM_NOUN_ATTRIBUTES ))
	{
		dwAttributesToMatch = dwStringAttributesToMatch;
	}
	else
	{
		// get item name string information
		int nQuantity = ItemGetQuantity( unit );
		ITEM_NAME_INFO tItemNameInfo;
		ItemGetNameInfo( tItemNameInfo, unit, nQuantity, 0 );
		
		// attributes to match will come from the item name
		dwAttributesToMatch = tItemNameInfo.dwStringAttributes;
		
	}
			
	// get quality string
	const WCHAR * puszQuality = StringTableGetStringByIndexVerbose(
		nString,
		dwAttributesToMatch,
		0,
		NULL,
		NULL);
						
	if (quality_data->nNameColor > 0 && TESTBIT( dwUnitNameFlags, UNF_EMBED_COLOR_BIT ) )
	{
		uszBuffer[0] = L'\0';
		UIColorCatString(uszBuffer, nBufferLen, (FONTCOLOR)quality_data->nNameColor, puszQuality);
	}
	else
	{
		PStrCopy(uszBuffer, puszQuality, nBufferLen );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetClassReqString(
	UNIT* unit,
	WCHAR* szReqString,
	int len)
{
	*szReqString = 0;
	ASSERT_RETFALSE(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	const WCHAR * szFormat = GlobalStringGet( GS_CLASS_REQUIREMENT );

	int nClassListLen = len - PStrLen(szFormat);

	ASSERT_RETFALSE(nClassListLen > 0);
	WCHAR * szClassList = (WCHAR *)MALLOCZ(NULL, nClassListLen * sizeof(WCHAR));
	ASSERT_RETFALSE(szClassList);

	// This is a ...... hold on, I may have a better idea
	GAME * pGame = UnitGetGame(unit);
	int nPlayerClassMax = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_PLAYERS );
	for (int ii = 0; ii < NUM_CONTAINER_UNITTYPES; ii++)
	{
		int nReqClass = unit->pUnitData->nContainerUnitTypes[ii];

		if (nReqClass > 0 && !UnitIsA(UNITTYPE_ANY, nReqClass) && !UnitIsA(UNITTYPE_PLAYER, nReqClass))		// Don't display an "any" requirement
		{
			int nLastString = -1;
			for ( int jj = 0; jj < nPlayerClassMax; jj++ )
			{
				UNIT_DATA * player_data = (UNIT_DATA *)ExcelGetData( pGame, DATATABLE_PLAYERS, jj );
				if (player_data == NULL)
					continue;

				// This will keep the male/female versions from differentiating
				if (player_data->nString == nLastString)
					continue;

				nLastString = player_data->nString;

				if ( UnitIsA(player_data->nUnitType, nReqClass) )
				{
					if (PStrLen(szClassList) > 0)
					{
						PStrCat(szClassList, L", ", nClassListLen);
					}
					PStrCat(szClassList, StringTableGetStringByIndex(nLastString), nClassListLen);
				}
			}
		}
	}

	if (PStrLen(szClassList) > 0)
	{
		PStrPrintf(szReqString, len, szFormat, szClassList);
	}

	FREE(NULL, szClassList);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ItemGetFactionReqString(
	UNIT* unit,
	WCHAR* szReqString,
	int len)
{
	*szReqString = L'\0';
	ASSERT_RETFALSE(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(IS_CLIENT(game));

	UNIT * pPlayer = GameGetControlUnit(game);
	UNIT_ITERATE_STATS_RANGE( unit, STATS_FACTION_REQUIREMENT, pStatsEntry, jj, 32 )
	{
		int nFaction = pStatsEntry[jj].GetParam();
		int nReqStanding = pStatsEntry[jj].value;
		int nPlayerValue = UnitGetStat(pPlayer, STATS_FACTION_SCORE, nFaction);

		const WCHAR * szFormat = GlobalStringGet( GS_FACTION_REQUIREMENT );
		if (nReqStanding > nPlayerValue)
		{
			UIColorCatString(szReqString, len, FONTCOLOR_LIGHT_RED, szFormat);
		}
		else
		{
			PStrCopy(szReqString, szFormat, len);
		}

		const FACTION_DEFINITION *pFaction = FactionGetDefinition(nFaction);
		if (pFaction)
		{
			const WCHAR *szFactionName = StringTableGetStringByIndex(pFaction->nDisplayString);
			const WCHAR *szStandingName = FactionScoreToDecoratedStandingName(nReqStanding);

			PStrReplaceToken(szReqString, len, L"<faction>", szFactionName);
			PStrReplaceToken(szReqString, len, L"<standing>", szStandingName);

			return TRUE;		// only supporting one right now
		}
	}
	UNIT_ITERATE_STATS_END;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL UnitGetFlavorTextString(
	UNIT* unit,
	WCHAR* szFlavor,
	int len)
{
	*szFlavor = 0;

	ASSERT_RETFALSE(unit);

	GAME* game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	const WCHAR* domflavor = NULL;
	int dom = INT_MIN;

	DWORD dwAttributesToMatch = 0;

	// get player
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit( pGame );
		if (pPlayer && UnitHasBadge(pPlayer, ACCT_STATUS_UNDERAGE) )	//UNDER_18
		{
			dwAttributesToMatch |= StringTableGetUnder18Attribute();
		}
	}

	UNIT_ITERATE_STATS_RANGE(unit, STATS_APPLIED_AFFIX, list, ii, 32)
	{
		int nAffix = list[ii].value;
		const AFFIX_DATA* affix_data = AffixGetData(nAffix);
		if (affix_data && affix_data->nFlavorText > INVALID_LINK)
		{
			if (affix_data->nDominance > dom)
			{
				domflavor = StringTableGetStringByIndexVerbose(affix_data->nFlavorText, dwAttributesToMatch, 0, NULL, NULL);
				dom = affix_data->nDominance;
			}
		}
	}
	STATS_ITERATE_STATS_END; 

	if (!domflavor)
	{
		const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
		int flavoredText( INVALID_LINK );
		if( UnitDataTestFlag(item_data, UNIT_DATA_FLAG_QUEST_FLAVOR_TEXT) )
		{
			if( AppIsTugboat() )
			{
				//get Quest text based off of the Quest item's owner
				flavoredText = c_QuestGetFlavoredText( unit, UnitGetUltimateOwner( unit ) );
				if( flavoredText == INVALID_LINK &&
					( MYTHOS_MAPS::IsMAP(unit) ||
					  MYTHOS_MAPS::IsMapSpawner(unit) ) )
				{
					flavoredText = MYTHOS_MAPS::c_GetFlavoredText( unit );
				}

				if( flavoredText == INVALID_LINK &&
					ItemIsRecipe( unit ) )
				{
					flavoredText = c_RecipeGetFlavoredText( unit );	
				}
			}
		}
		
		if ( flavoredText == INVALID_LINK && 
			 item_data && 
			 item_data->nFlavorText > INVALID_LINK )
		{
			flavoredText = item_data->nFlavorText;
		}

		if( flavoredText != INVALID_LINK )
		{
			domflavor = StringTableGetStringByIndexVerbose(flavoredText, dwAttributesToMatch, 0, NULL, NULL);
		}
	}

	if (!domflavor)
	{
		return FALSE;
	}

	PStrCat(szFlavor, domflavor, len);
	if( AppIsTugboat() )
	{				
		if( QuestUnitIsQuestUnit( unit ) )
		{
			c_QuestFillFlavoredText( QuestUnitGetTask( unit ), UnitGetClass( unit ), szFlavor, len, 0 );	
		}
		MYTHOS_MAPS::c_FillFlavoredText( unit, szFlavor, len );	//add recipe name if need be
		c_RecipeInsertName( unit, szFlavor, len );	//add recipe name if need be
		
	}
	return TRUE;
}
#endif //!SERVER_VERSION

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetNameColor(
	UNIT* unit)
{

	// pets
	if (PetIsPet( unit ))
	{
		return FONTCOLOR_BLUE;
	}

	// check item quality	
	if (UnitIsA( unit, UNITTYPE_ITEM ))
	{
		if (UnitDataTestFlag(unit->pUnitData, UNIT_DATA_FLAG_USE_QUEST_NAME_COLOR))
		{
			return FONTCOLOR_QUEST_ITEM;
		}

		int nItemQuality = ItemGetQuality( unit );
		if (nItemQuality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA* pItemQualityData = (ITEM_QUALITY_DATA*)ExcelGetData(UnitGetGame(unit), DATATABLE_ITEM_QUALITY, nItemQuality);
			if (pItemQualityData)
			{
				if (pItemQualityData->nNameColor == INVALID_LINK)
					return FONTCOLOR_WHITE;

				return pItemQualityData->nNameColor;
			}
		}
	}
		
	// check unique monsters (not pets)
	//int nMonsterNameIndex = UnitGetStat(unit, STATS_MONSTER_UNIQUE_NAME);	
	//if (nMonsterNameIndex != MONSTER_UNIQUE_NAME_INVALID)
	//{
	//	return FONTCOLOR_LIGHT_RED;
	//}

	// champions	
	int nMonsterQuality = UnitGetStat(unit, STATS_MONSTERS_QUALITY);
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData(UnitGetGame(unit), nMonsterQuality);
		if (pMonsterQualityData)
		{
			return pMonsterQualityData->nNameColor;
		}	
	}
	
	return FONTCOLOR_INVALID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetAffixDescription(
	UNIT* unit,
	WCHAR* szDesc,
	int nDescLen)
{
	static const WCHAR* szUnknown = L"???";
	
	ASSERT( szDesc );
	*szDesc = 0;

	ASSERT(unit);
	if (!unit)
	{
		PStrCat(szDesc, szUnknown, nDescLen);
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		PStrCat(szDesc, szUnknown, nDescLen);
		return FALSE;
	}

	switch (UnitGetGenus(unit))
	{
	
		//----------------------------------------------------------------------------
		case GENUS_MONSTER:
		{
			sMonsterGetAffixDesc(game, unit, szDesc, nDescLen);
			break;
		}

		//----------------------------------------------------------------------------
		default:
		{
			break;
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetResistanceDescription(
	UNIT* unit,
	WCHAR* szDesc,
	int nDescLen)
{
	static const WCHAR* szUnknown = L"???";
	
	ASSERT( szDesc );
	*szDesc = 0;

	ASSERT(unit);
	if (!unit)
	{
		PStrCat(szDesc, szUnknown, nDescLen);
		return FALSE;
	}

	GAME* game = UnitGetGame(unit);
	ASSERT(game);
	if (!game)
	{
		PStrCat(szDesc, szUnknown, nDescLen);
		return FALSE;
	}
	WCHAR szTemp[512];
	for( int dmg_type = 0; dmg_type < NUM_DAMAGE_TYPES; dmg_type++ )
	{
		const DAMAGE_TYPE_DATA* damage_type = DamageTypeGetData(AppGetCltGame(), dmg_type );
		if(!damage_type)
			continue;
		if( damage_type->szDisplayString && PStrLen( damage_type->szDisplayString ) == 0 )
			continue;
		int vulnerability = UnitGetStat( unit, STATS_ELEMENTAL_VULNERABILITY ) + UnitGetStat( unit, STATS_ELEMENTAL_VULNERABILITY, dmg_type );
		vulnerability = MAX(vulnerability, -100);
		
		if ( vulnerability > 0 )
		{
			PStrPrintf(szTemp, 512, L"\n%d%% %s Vulnerability", vulnerability, StringTableGetStringByKey( damage_type->szDisplayString ) );
			PStrCat( szDesc, szTemp, nDescLen );
		}
		else if ( vulnerability < 0 )
		{
			PStrPrintf(szTemp, 512, L"\n%d%% %s Resistance", abs(vulnerability), StringTableGetStringByKey( damage_type->szDisplayString ) );
			PStrCat( szDesc, szTemp, nDescLen );
		}
	}


	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsPhysicallyInContainer(
	UNIT *pUnit)
{

	// get container
	UNIT *pContainer = UnitGetContainer( pUnit );
	ASSERTX_RETFALSE( pContainer, "Expected container" );	

	// get inventory location
	int nInvLocation;
	if (UnitGetInventoryLocation( pUnit, &nInvLocation ) == FALSE)
	{
		ASSERTX_RETFALSE( 0, "Can't get inv location of contained item" );
	}

	// get inventory location information
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInvLocation );	
	ASSERTX_RETFALSE( pInvLocData, "Expected inventory loc data" );
	return pInvLocData->bPhysicallyInContainer;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendInventory(
	GAMECLIENT *pClient,
	UNIT *pUnit)
{
	// Send "inside of" inventory
	
	// send inventory, note that we're ignoring the player state here because we're assuming
	// that if we're really already sending to a client that they are in on the right
	// level and know about the room etc which is convenient because a part of the knows
	// inventory logic and knows room logic asks if the player is really in game yet.
	UNIT *pContainer = UnitGetUltimateOwner( pUnit );
	if (ClientCanKnowInventoryOf( pClient, pContainer, TRUE ))
	{
		UNIT *pItem = NULL;
		while ((pItem = UnitInventoryIterate( pUnit, pItem, 0, FALSE )) != NULL)
		{
			if (UnitIsPhysicallyInContainer( pItem ))
			{
				if (ClientCanKnowUnit( pClient, pItem, TRUE ))
				{
					if (!s_SendUnitToClient( pItem, pClient ))
					{
						return FALSE;
					}
					if (ItemIsInEquipLocation(pUnit, pItem) && !ItemIsEquipped(pItem, pUnit))
					{
						MSG_SCMD_INV_ITEM_UNUSABLE msg;
						msg.idItem = UnitGetId(pItem);
						s_SendMessage(UnitGetGame(pUnit), ClientGetId(pClient), SCMD_INV_ITEM_UNUSABLE, &msg);
					}
				}
			}
		}		
	}
	
	// Send "outside of" inventory links 
	
	// We do this for objects that are
	// party of a units inventory that are not physically contained by the
	// unit.  For instance, a maggot spawn that is in the Taint inventory, but
	// it not physically contained by the Taint ... if we receive a new unit
	// message of the taint, if the client already has some of his maggot spawns
	// in their known region, we want to link them into the inventory of
	// the taint.  In the reverse case, when we receive a new unit message for a
	// maggot spawn, but we don't know about the taint, it will be recorded
	// in the maggot spawn who their container id is even though we don't know about the
	// taint unit itself.
	
	// TODO: We should change all of this so that if a client knows about any part
	// of a inventory chain of units that they know about *all* of them, including
	// any rooms that each of them are in, but it's playday and I can't make
	// a change that big right now -Colin (2006-07-28)
	
	GAME *pGame = UnitGetGame( pUnit );
	GAMECLIENTID idClient = ClientGetId( pClient );
	UNIT *pContained = NULL;
	while ((pContained = UnitInventoryIterate( pUnit, pContained, 0, FALSE )) != NULL)
	{
		if (UnitIsPhysicallyInContainer( pContained ) == FALSE)
		{
			
			MSG_SCMD_INVENTORY_LINK_TO tMessage;
			tMessage.idUnit = UnitGetId( pContained );
			tMessage.idContainer = UnitGetId( pUnit );
			UnitGetInventoryLocation( pContained, &tMessage.tLocation );						
			s_SendMessage( pGame, idClient, SCMD_INVENTORY_LINK_TO, &tMessage );
		}
	}			

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendMultiNewUnitBuffer(
	GAMECLIENTID idClientDest,
	UNIT *pUnit,
	BIT_BUF &tBitBuffer)
{
	ASSERTV_RETURN( idClientDest != INVALID_GAMECLIENTID, "Expected game client id dest" );
	ASSERTV_RETURN( pUnit, "Expected unit" );

	DWORD dwTotalDataSize = tBitBuffer.GetLen();
	DWORD dwBytesLeftToSend = dwTotalDataSize;
	DWORD dwDataStartIndex = 0;
	BYTE bSequence = 0;
	while (dwBytesLeftToSend)
	{
	
		// how much data will we send with this block
		DWORD dwBytesCurrentSend = MIN( (DWORD)MAX_NEW_UNIT_BUFFER, dwBytesLeftToSend );
		
		// setup the message for this block
		MSG_SCMD_UNIT_NEW tMessage;
		tMessage.idUnit = UnitGetId( pUnit );
		tMessage.bSequence = bSequence++;
		tMessage.dwTotalDataSize = dwTotalDataSize;
		
		// get the data for this block
		int nBitCursor = dwDataStartIndex * 8;
		tBitBuffer.SetCursor( nBitCursor );
		BYTE *pCurrentSendDataBuffer = tBitBuffer.GetPtr();
		memcpy( tMessage.pData, pCurrentSendDataBuffer, dwBytesCurrentSend );
		
		// set the data size
		MSG_SET_BLOB_LEN( tMessage, pData, dwBytesCurrentSend );
				
		// send to client
		s_SendUnitMessageToClient( pUnit, idClientDest, SCMD_UNIT_NEW, &tMessage );

		// update the bytes left to send and the index of the next data
		dwDataStartIndex += dwBytesCurrentSend;
		dwBytesLeftToSend -= dwBytesCurrentSend;
			
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendUnitToClient(
	UNIT *pUnit,
	GAMECLIENT *pClient,
	DWORD dwKnownFlags /*= 0*/,		// see UNIT_KNOWN_FLAGS
	const NEW_MISSILE_MESSAGE_DATA *pNewMissileMessageData /*= NULL*/)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	GAME *pGame = UnitGetGame( pUnit );
		
	// don't send yet if we can
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return FALSE;
	}

	// if client can never know about this unit, we will not even send it
	if (!TESTBIT(dwKnownFlags, UKF_ALLOW_INSPECTION) && ClientCanKnowUnit( pClient, pUnit, TRUE, TESTBIT(dwKnownFlags, UKF_ALLOW_CLOSED_STASH) ) == FALSE)
	{
		return FALSE;
	}

	if (UnitIsKnownByClient(pClient, pUnit))
	{
		return FALSE;
	}
	
#ifdef _DEBUG_KNOWN_STUFF_TRACE
	{
		UNIT *pContainer = UnitGetContainer( pUnit );
		
		trace(pContainer ? "*KT* SERVER: Send unit to client '%llu' - '%s' [%d] (contained by '%s' [%d])\n" : "*KT* SERVER: Send unit to client '%llu' - '%s' [%d]\n",
			ClientGetId( pClient ), UnitGetDevName( pUnit ), UnitGetId( pUnit ), pContainer ? UnitGetDevName( pContainer ) : "UNKNOWN",
			pContainer ? UnitGetId( pContainer ) : INVALID_ID);
	}
#endif

	// client know knows about that unit
	GAMECLIENTID idClient = ClientGetId( pClient );

	ClientAddKnownUnit(pClient, pUnit, dwKnownFlags);

	// missiles
	if (UnitIsA( pUnit, UNITTYPE_MISSILE ))
	{
		if ( pNewMissileMessageData )
		{
			// send message
			s_SendUnitMessageToClient( 
				pUnit, 
				idClient, 
				pNewMissileMessageData->bCommand, 
				pNewMissileMessageData->pMessage );
		}
	}
	else
	{
		// setup buffer for new unit message
		// KCK: This stuff wasn't being used:
//		BYTE bStorage[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
//		BIT_BUF tBitBuffer( bStorage, UNITFILE_MAX_SIZE_SINGLE_UNIT );

		// what is the save mode
		UNITSAVEMODE savemode = UNITSAVEMODE_SENDTOOTHER;
		// KCK: If we're inspecting this item, send it with no unit ID for security purposes
		if (TESTBIT(dwKnownFlags, UKF_ALLOW_INSPECTION))
			savemode = UNITSAVEMODE_SENDTOOTHER_NOID;
		GAMECLIENTID idClient = ClientGetId(pClient);
		UNIT * control_unit = ClientGetControlUnit(pClient, TRUE);
		ASSERT(control_unit)
		BOOL bIsControlUnit = (control_unit == pUnit);
		if ( bIsControlUnit || UnitGetOwnerId(pUnit) == UnitGetId(control_unit))
		{
			savemode = UNITSAVEMODE_SENDTOSELF;
		}
		
		// the vast majority of new unit sends are small and do not exceed
		// the max size allowed.  To save a memory copy for these units, we want
		// to build the data directly into the message buffer.  It's a little ugly
		// to put both on the stack here
		MSG_SCMD_UNIT_NEW tMessageNonPlayer;
		BIT_BUF tBitBufferNonPlayer( tMessageNonPlayer.pData, MAX_NEW_UNIT_BUFFER );
		BYTE bPlayerDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		BIT_BUF tBitBufferPlayer( bPlayerDataBuffer, UNITFILE_MAX_SIZE_SINGLE_UNIT );
		BIT_BUF *pBitBuffer = NULL;  // the actual bit buffer to use for xfer
		BOOL bSendMultiMessage = FALSE;
		if (UnitIsPlayer( pUnit ))
		{
			pBitBuffer = &tBitBufferPlayer;
			bSendMultiMessage = TRUE;
		}
		else
		{
			pBitBuffer = &tBitBufferNonPlayer;
		}
		
		// write unit to buffer
		XFER<XFER_SAVE> tXfer( pBitBuffer );
		UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec( tXfer );
		tSpec.pUnitExisting = pUnit;
		tSpec.idClient = ClientGetId( pClient );
		tSpec.bIsControlUnit = bIsControlUnit;
		tSpec.eSaveMode = savemode;
		if (UnitFileXfer( pGame, tSpec ) != pUnit)
		{
			const int MAX_MESSAGE = 256;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Unable to send unit '%s' [%d] to client '%I64x'",
				UnitGetDevName( pUnit ),
				UnitGetId( pUnit ),
				ClientGetId( pClient ));
			ASSERTX_RETFALSE( 0, szMessage );
		}

		// send all the data to the dest client
		if (bSendMultiMessage == FALSE)
		{
		
			// set message data
			DWORD dwTotalDataSize = pBitBuffer->GetLen();		
			tMessageNonPlayer.idUnit = UnitGetId( pUnit );
			tMessageNonPlayer.bSequence = 0;  // unused
			tMessageNonPlayer.dwTotalDataSize = dwTotalDataSize;
			MSG_SET_BLOB_LEN( tMessageNonPlayer, pData, dwTotalDataSize );
			
			// send to client
			s_SendUnitMessageToClient(pUnit, idClient, SCMD_UNIT_NEW, &tMessageNonPlayer );
			
		}
		else
		{
			sSendMultiNewUnitBuffer( idClient, pUnit, *pBitBuffer );
		}

		// inform client this is our unit if necessary
		if ( bIsControlUnit )
		{
			MSG_SCMD_SET_CONTROL_UNIT msg;
			msg.id = UnitGetId( pUnit );
			msg.guid = UnitGetGuid( pUnit );
			s_SendMessage( pGame, idClient, SCMD_SET_CONTROL_UNIT, &msg );
			trace("sent SCMD_SET_CONTROL_UNIT(%d)\n", msg.id);
			
			UnitSetStat( pUnit, STATS_CONTROL_UNIT_SENT, TRUE );
		}

		// send inventory we can know about
		s_SendInventory( pClient, pUnit );

		// send wardrobe update
		s_WardrobeUpdateClient( pGame, pUnit, pClient );
		
		// send all states to client
		s_StatesSendAllToClient( pGame, pUnit, idClient );		

		if( AppIsTugboat() && ObjectIsDoor( pUnit ) )
		{
	
	//		BOOL bIsIdle = UnitTestMode( pUnit, MODE_IDLE ) && ! UnitTestMode( pUnit, MODE_OPEN );
			BOOL bIsOpened = ( UnitTestMode( pUnit, MODE_OPENED ) || UnitTestMode( pUnit, MODE_OPEN ) )&& ! UnitTestMode( pUnit, MODE_CLOSE );
			int FinalMode = MODE_IDLE;
			if( bIsOpened )
			{
				FinalMode = MODE_OPENED;
			}
				// setup message			
				MSG_SCMD_UNITMODE msg;
				msg.id = UnitGetId(pUnit);
				msg.mode = FinalMode;
				msg.wParam = 0;
				msg.duration = 0;
				// send it
				s_SendUnitMessageToClient(pUnit, idClient, SCMD_UNITMODE, &msg);
		}
		
		// if unit is item and is jumping start the jump on the client ... if we use
		// this for players and such too it might be good enough?? -Colin
		if ( AppIsHellgate() && UnitGetRoom(pUnit) != NULL && UnitIsA( pUnit, UNITTYPE_ITEM ) && UnitIsOnGround( pUnit ) == FALSE)
		{
			s_SendUnitJump( pUnit, pClient );
		}

		// inform client that we've finished sending them the control unit
		if (bIsControlUnit)
		{
			MSG_SCMD_CONTROLUNIT_FINISHED msg;
			msg.id = UnitGetId( pUnit );
			s_SendMessage( pGame, idClient, SCMD_CONTROLUNIT_FINISHED, &msg );
			trace("sent SCMD_CONTROLUNIT_FINISHED(%d)\n", msg.id);
		}

		//	send them their guild association
		s_ChatNetSendUnitGuildAssociationToClient(pGame, pClient, pUnit);

		// they'll need to know the team of other players already in the game they're joining.
		if ( !bIsControlUnit && UnitPvPIsEnabled( pUnit ) && ( UnitIsA( pUnit, UNITTYPE_PLAYER ) || PetIsPet( pUnit ) ) )
		{
			s_SendTeamJoin(pGame, idClient, pUnit, UnitGetTeam( pUnit ) );
		}
	}
	
	return TRUE;
#else
	REF( pUnit );
	REF( pClient );
	REF( pNewMissileMessageData );
	return TRUE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SendUnitToAllWithRoom(
	GAME * game,
	UNIT * unit,
	ROOM * room)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETFALSE( UnitCanSendMessages( unit ), "Can't send unit who isn't ready to send messages yet" );
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(room);
	
	for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
		 pClient; 
		 pClient = ClientGetNextInGame( pClient ))
	{
		if (ClientIsRoomKnown( pClient, room ))
		{
			s_SendUnitToClient(unit, pClient);
		}
	}	
	
	return TRUE;
#else
	REF( game );
	REF( unit );
	REF( room );
	return TRUE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RemoveUnitFromClient( 
	UNIT * unit, 
	GAMECLIENT * client,
	DWORD dwUnitFreeFlags)
{
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(unit);
	REF(client);
	REF(dwUnitFreeFlags);
#else
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(client);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(IS_SERVER(game));

	// if we're not freeing this unit, if this is marked as never remove 
	// for this client, don't remove it
	if (UnitTestFlag( unit, UNITFLAG_FREED ) == FALSE && UnitIsCachedOnClient( client, unit ))
	{
		return FALSE;
	}
	
	// debug
	sDebugMissingControlUnit( unit, client );	

#ifdef _DEBUG_KNOWN_STUFF_TRACE
	UNIT * container = UnitGetContainer(unit);
	if (container)
	{
		trace("*KT*  Remove unit [%d: %s] from client [%s] (contained by [%d: %s]\n", 
			UnitGetId(unit), UnitGetDevName(unit), ClientGetName(client), UnitGetId(container), UnitGetDevName(container));
	}
	else
	{
		trace("*KT*  Remove unit [%d: %s] from client [%s]\n", UnitGetId(unit), UnitGetDevName(unit), ClientGetName(client));
	}
#endif

	MSG_SCMD_UNIT_REMOVE msg;
	msg.id = UnitGetId(unit);
	msg.flags = (BYTE)dwUnitFreeFlags;
	
	s_SendUnitMessageToClient(unit, ClientGetId(client), SCMD_UNIT_REMOVE, &msg);

	// this unit or any of their inventory is not known by this client anymore
	ClientRemoveKnownUnit(client, unit);
#endif
	return TRUE;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORD UnitGetClassCode(
	UNIT* unit)
{
	switch (UnitGetGenus(unit))
	{
	case GENUS_MISSILE:
		return (WORD)UnitGetClass(unit);
	default:
		{
			const UNIT_DATA * pUnitData = UnitGetData( unit );
			ASSERT_RETZERO(pUnitData);
			return pUnitData->wCode;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitDoDelayedDeath(
	UNIT * pUnit )
{
	BOOL bHasMode;
	float fDuration = UnitGetModeDuration( UnitGetGame( pUnit ), pUnit, MODE_DYING, bHasMode );
	// set mode to dying so they get a corpse
	c_StateSet( pUnit, pUnit, STATE_CLIENT_HIDE_DYING, 0, 100, INVALID_ID );
	if ( UnitTestMode ( pUnit, MODE_DEAD ) )
		UnitEndMode( pUnit, MODE_DEAD );
	c_UnitSetMode( pUnit, MODE_DYING, 0, fDuration );						
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetWeaponPositionAndDirection(
	GAME * pGame,
	UNIT * unit,
	int nWeaponIndex,
	VECTOR * pvPosition,
	VECTOR * pvDirection )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( nWeaponIndex < MAX_WEAPONS_PER_UNIT );
	BOOL InvalidWeapon = FALSE;
	if ( nWeaponIndex < 0 )
	{
		// if we aren't using a valid weapon, then why do we rely on the previous weaponpos
		// of an actual weapon? What if it hasn't been fired, and its position updated for a while?
		// this'll be the wrong position.
		nWeaponIndex = 0;
		InvalidWeapon = TRUE;
	}

	VECTOR vOffset;
	if ( pvPosition && 
		UnitTestFlag( unit, UNITFLAG_USE_APPEARANCE_MUZZLE_OFFSET ) &&
		UnitGetAppearanceDefId( unit ) != INVALID_ID &&
		AppearanceDefinitionGetWeaponOffset( UnitGetAppearanceDefId( unit ) , nWeaponIndex, vOffset ) )
	{
		MATRIX mUnitWorld;
		UnitGetWorldMatrix( unit, mUnitWorld );
		MatrixMultiply( pvPosition, &vOffset, &mUnitWorld );
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		ASSERT_RETFALSE( pUnitData );
		pvPosition->fZ += pUnitData->fOffsetUp;
	}
	else if ( pvPosition )
	{
		if ( InvalidWeapon || VectorIsZero( unit->pvWeaponPos[ nWeaponIndex ] ) )
			*pvPosition = UnitGetCenter( unit );
		else
			*pvPosition = unit->pvWeaponPos[ nWeaponIndex ];
	}

	if ( pvDirection )
	{
		if ( InvalidWeapon || VectorIsZero( unit->pvWeaponDir[ nWeaponIndex ] ) )
			*pvDirection = unit->vFaceDirection;
		else
			*pvDirection = unit->pvWeaponDir[ nWeaponIndex ];
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitGetDataFolder( 
	 GAME* game,
	 UNIT* unit,
	 char* pszFolder,
	 int nBufLen)
{
	const char * pszDataFolder = NULL;
	switch(UnitGetGenus(unit))
	{
	case GENUS_PLAYER:
		pszDataFolder = FILE_PATH_PLAYERS;
		break;

	case GENUS_MONSTER:
		pszDataFolder = FILE_PATH_MONSTERS;
		break;

	case GENUS_OBJECT:
		pszDataFolder = FILE_PATH_OBJECTS;
		break;

	case GENUS_ITEM:
		pszDataFolder = FILE_PATH_ITEMS;
		break;

	case GENUS_MISSILE:
		pszDataFolder = FILE_PATH_MISSILES;
		break;
	}

	const UNIT_DATA * pUnitData = UnitGetData( unit );
	ASSERT(pUnitData);
	PStrPrintf(pszFolder, nBufLen, "%s%s\\", pszDataFolder, pUnitData->szFileFolder);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef HAVOK_ENABLED
void UnitAddImpact( 
	GAME * pGame,
	UNIT * pUnit,
	const HAVOK_IMPACT * pImpact)
{
	if ( AppIsTugboat() )
		return;
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( pGame ) )
	{
		// Is the unit in ragdoll mode?
		int nModelId = c_UnitGetModelId( pUnit );
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );

		c_RagdollAddImpact ( nModelId, *pImpact, e_ModelGetScale( nModelId ).fX, pUnitData->fRagdollForceMultiplier );
		return;
	}
#endif// !ISVERSION(SERVER_VERSION)
	if ( !pUnit->pHavokImpact )
		pUnit->pHavokImpact = (HAVOK_IMPACT *) GMALLOC( pGame, sizeof(HAVOK_IMPACT) );
	MemoryCopy( pUnit->pHavokImpact, sizeof(HAVOK_IMPACT), pImpact, sizeof( HAVOK_IMPACT ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitApplyImpacts( 
	GAME * pGame,
	UNIT * pUnit )
{
	if ( AppIsTugboat() )
		return;
#ifdef HAVOK_ENABLED
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( pGame ) )
		c_RagdollApplyAllImpacts ( c_UnitGetModelId( pUnit ) );
#endif// !ISVERSION(SERVER_VERSION)

	if (! pUnit || !pUnit->pHavokImpact )
		return;
	if ( GameGetTick( pGame ) - pUnit->pHavokImpact->dwTick < RAGDOLL_DELAY_ALLOWED_FOR_IMPULSE_IN_TICKS )
	{
		if ( IS_SERVER( pGame ) )
		{
			MSG_SCMD_UNITADDIMPACT msg;
			msg.id = UnitGetId(pUnit);
			msg.wFlags  = (WORD) pUnit->pHavokImpact->dwFlags;
			msg.vSource = pUnit->pHavokImpact->vSource;
			msg.fForce  = pUnit->pHavokImpact->fForce;
			msg.vDirection = pUnit->pHavokImpact->vDirection;
			s_SendUnitMessage(pUnit, SCMD_UNITADDIMPACT, &msg);
		} 

		if ( pUnit->pHavokRigidBody )
		{
			HavokRigidBodyApplyImpact(pGame->rand, pUnit->pHavokRigidBody, *pUnit->pHavokImpact);
			UnitStepListAdd(pGame, pUnit);
		} 
		else if ( IS_CLIENT( pGame ) )
		{
#if !ISVERSION(SERVER_VERSION)
			int nModelId = c_UnitGetModelId( pUnit );
			const UNIT_DATA * pUnitData = UnitGetData( pUnit );
			c_RagdollAddImpact( nModelId, 
								*pUnit->pHavokImpact,
								e_ModelGetScale( nModelId ).fX,
								pUnitData->fRagdollForceMultiplier );
			pUnit->pHavokImpact->dwTick = 0;
#endif// !ISVERSION(SERVER_VERSION)
		}
	}

	pUnit->pHavokImpact->dwTick = 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Added for tugboat
void UnitDrop(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR& vPosition )
{
	if ( AppIsHellgate() )
		return;
	
	if (! pUnit )
		return;
	{
		if ( IS_SERVER( pGame ) )
		{
			VECTOR vDirection = VECTOR(0.0f, 0.0f, -1 );
			VECTOR vTarget = 0;
			VECTOR vStart;
			VECTOR vCollisionNormal;
			LEVEL* pLevel = RoomGetLevel( UnitGetRoom( pUnit ) );
			// make sure it is a good spot to drop it
			for (int nTries = 0; nTries < 200; ++nTries)
			{
				ASSERT_BREAK(pLevel);				

				vTarget.fX = RandGetFloat(pGame, -1.5f, 1.5f);
				vTarget.fY = RandGetFloat(pGame, -1.5f, 1.5f);
				vTarget.fZ = 0;

				VectorNormalize( vTarget );
				vTarget *= RandGetFloat( pGame, .8f, 2.0f );
				vTarget += vPosition;
				vTarget.fZ = vPosition.fZ;

				ROOM * pRoom = RoomGetFromPosition(pGame, pLevel, &vTarget);
				ROOM* pFinalRoom = NULL;
				if( !pRoom )
				{
					vTarget = vPosition;
					pRoom = RoomGetFromPosition(pGame, pLevel, &vTarget);
					if( !pRoom )
					{
						continue;
					}
				}
				ROOM_PATH_NODE* pPathNode;
				if( nTries < 100 )
				{
					pPathNode = RoomGetNearestFreePathNode( pGame, pLevel, vTarget, &pFinalRoom, NULL, 0, 0, pRoom  );
				}
				else
				{
					pPathNode = RoomGetNearestPathNode( pGame, pRoom, vTarget, &pFinalRoom );
				}
				if(pFinalRoom && pPathNode )
				{
					vTarget = RoomPathNodeGetExactWorldPosition(pGame, pFinalRoom, pPathNode);
					//vTarget.fZ = vPosition.fZ;
				}

				/*vStart = vTarget + VECTOR( 0, 0, 40 );
				float fCollideDistance = LevelLineCollideLen( pGame, pLevel, vStart, vDirection, 100, &vCollisionNormal );

				fCollideDistance = vStart.fZ - fCollideDistance;
				{

					if( fabs( fCollideDistance ) <= 6.0f &&
						RoomGetFromPosition( pGame, pLevel, &vTarget ) != NULL )
					{
						vTarget.fZ = fCollideDistance + .05f;
						break;
					}

				}*/

				if( RoomGetFromPosition( pGame, pLevel, &vTarget ) != NULL )
				{
					vTarget.fZ += .05f;
					break;
				}

			}
			ASSERT( RoomGetFromPosition( pGame, pLevel, &vTarget ) );
			
			if( ( UnitIsA( pUnit, UNITTYPE_WEAPON ) ||
				 UnitIsA( pUnit, UNITTYPE_SHIELD ) ) &&
				 !UnitIsA( pUnit, UNITTYPE_CROSSBOW ) )
			{
				// we lay units on their sides, so that they don't point upward. 
				VECTOR vUp( 0, 1, 0 );
				VECTOR vForward( -1, 0, 0 );
				MATRIX zRot;
				MatrixRotationAxis( zRot, VECTOR(0, 0, 1), DegreesToRadians( RandGetFloat( pGame, 0, 360 ) ) );
				//MatrixMultiplyNormal( &pUnit->vUpDirection, &vUp, &zRot );
				UnitSetUpDirection( pUnit, vUp );
				MatrixMultiplyNormal( &vForward, &vForward, &zRot );
				MatrixMultiplyNormal( &vUp, &vUp, &zRot );

				UnitSetFaceDirection( pUnit, vForward, TRUE );
			}

			pUnit->vPosition = vPosition;
			UnitUpdateRoomFromPosition( pGame, pUnit );
			ASSERT( UnitGetRoom( pUnit ) );
			ASSERT( pUnit->vPosition != INVALID_POSITION );

			MSG_SCMD_UNITDROP msg_drop;
			msg_drop.id = UnitGetId(pUnit);
			msg_drop.idRoom =UnitGetRoomId( pUnit );
			msg_drop.vPosition = pUnit->vPosition;
			msg_drop.vNewPosition = vTarget;
			msg_drop.vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);
			msg_drop.vUpDirection = UnitGetUpDirection(pUnit);
			s_SendMessageToAll(pGame, SCMD_UNITDROP, &msg_drop);
			UnitSetFlag(pUnit, UNITFLAG_ONGROUND, TRUE);
			pUnit->vPosition = vTarget;
			UnitUpdateRoomFromPosition( pGame, pUnit );
		} 
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetModeDurationScaled( 
	GAME* pGame, 
	UNIT* unit, 
	UNITMODE eMode,
	BOOL & bHasMode )
{
	int nAppearanceDefId = UnitGetAppearanceDefId( unit, TRUE );
	if ( nAppearanceDefId == INVALID_ID)
	{
		bHasMode = FALSE;
		return 1.0f;
	}

	int nAnimationGroup = UnitGetAnimationGroup(unit);
	float fScale = UnitGetScale( unit );
	if( fScale != 0 )
	{	
		if( fScale < .01f )
		{
			fScale = .01f;
		}
	}
	else
	{
		fScale = 1;
	}

	return AppearanceDefinitionGetAnimationDuration ( pGame, nAppearanceDefId, nAnimationGroup, eMode, bHasMode ) * fScale;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetModeDuration( 
	GAME* pGame, 
	UNIT* unit, 
	UNITMODE eMode,
	BOOL & bHasMode,
	BOOL bIgnoreBackups /*=FALSE*/ )
{
	int nAppearanceDefId = UnitGetAppearanceDefId( unit, TRUE );
	if ( nAppearanceDefId == INVALID_ID)
	{
		bHasMode = FALSE;
		return 1.0f;
	}

	int nAnimationGroup = UnitGetAnimationGroup(unit);

	return AppearanceDefinitionGetAnimationDuration ( pGame, nAppearanceDefId, nAnimationGroup, eMode, bHasMode, bIgnoreBackups );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasMode(
	GAME* pGame, 
	UNIT* unit, 
	UNITMODE eMode )
{
	BOOL bHasMode = FALSE;
	UnitGetModeDuration( pGame, unit, eMode, bHasMode );
	return bHasMode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasOriginalMode(
				 GAME* pGame, 
				 UNIT* unit, 
				 UNITMODE eMode )
{
	BOOL bHasMode = FALSE;
	UnitGetModeDuration( pGame, unit, eMode, bHasMode, TRUE );
	return bHasMode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetVelocityForMode(
	UNIT* pUnit,
	UNITMODE eMode )
{
	float fVelocityBase;
	float fVelocityMin;
	float fVelocityMax;

	if ( ! UnitGetSpeedsByMode( pUnit, eMode, fVelocityBase, fVelocityMin, fVelocityMax ) )
	{
		return 0.0f;
	}

	float fVelocity = fVelocityBase;

	fVelocity += pUnit->fVelocityFromAcceleration;

	if ( eMode == INVALID_ID )
		return fVelocity;

	const UNITMODE_DATA * pModeData = UnitModeGetData( eMode );

	if ( pModeData->bVelocityUsesMeleeSpeed && AppIsHellgate() )
	{
		int nPercent = UnitGetMeleeSpeed( pUnit );
		fVelocity += fVelocity * (float)nPercent / 100.0f;		
	} 
	else if ( pModeData->bVelocityChangedByStats ) 
	{
		int nVelocityPct = UnitGetStat(pUnit, STATS_VELOCITY_BONUS );
		if (nVelocityPct)
		{
			if (nVelocityPct < -100)
			{
				nVelocityPct = -100;
			}
			fVelocity += fVelocity * (float)nVelocityPct / 100.0f;
		}
		nVelocityPct = UnitGetStat(pUnit, STATS_VELOCITY );
		if (nVelocityPct)
		{
			if (nVelocityPct < -100)
			{
				nVelocityPct = -100;
			}
			fVelocity += fVelocity * (float)nVelocityPct / 100.0f;
		}
	}

	ASSERT(fVelocity == 0.0f || fVelocityMax != 0.0f);
	if (fVelocity > fVelocityMax)
	{
		fVelocity = fVelocityMax;
	}
	if ( fVelocity < fVelocityMin )
	{
		fVelocity = fVelocityMin;
	}

	return fVelocity;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitCalcVelocity(
	UNIT* pUnit)
{
	UNITMODE eMoveMode = UnitGetMoveMode( pUnit );
	float fVelocity = UnitGetVelocityForMode( pUnit, eMoveMode );
	UnitSetVelocity( pUnit, fVelocity );
	if( AppIsTugboat() )
	{
		float fUnitSpeed = 1;
		int nVelocityPct = UnitGetStat(pUnit, STATS_VELOCITY_BONUS);
		if (nVelocityPct)
		{
			if (nVelocityPct < -100)
			{
				nVelocityPct = -100;
			}
			fUnitSpeed += (float)nVelocityPct / 100.0f;
		}
		nVelocityPct = UnitGetStat(pUnit, STATS_VELOCITY);
		if (nVelocityPct)
		{
			if (nVelocityPct < -100)
			{
				nVelocityPct = -100;
			}
	
			fUnitSpeed += (float)nVelocityPct / 100.0f;
		}
		fUnitSpeed /= UnitGetScale( pUnit );
		pUnit->fUnitSpeed = fUnitSpeed;
		UnitSetVelocity( pUnit, fVelocity );
	}
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT( pUnit ) )
	{
		int nModelId = c_UnitGetModelId( pUnit );
		if ( nModelId != INVALID_ID )
		{
			c_AppearanceUpdateSpeed( nModelId, fVelocity );
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetAnimationGroup(
	UNIT * pUnit)
{
	int nAnimationGroup = DEFAULT_ANIMATION_GROUP;

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return nAnimationGroup;

	int nOverride = UnitGetStat( pUnit, STATS_ANIMATION_GROUP_OVERRIDE );
	if ( nOverride != INVALID_ID )
		return nOverride;

	GAME * pGame = UnitGetGame( pUnit );

	UNIT * pRightUnit = WardrobeGetWeapon( pGame, nWardrobe, WEAPON_INDEX_RIGHT_HAND );
	UNIT * pLeftUnit  = WardrobeGetWeapon( pGame, nWardrobe, WEAPON_INDEX_LEFT_HAND );

 	const UNIT_DATA * pRightData = pRightUnit ? ItemGetData( pRightUnit ) : NULL;
	const UNIT_DATA * pLeftData  = pLeftUnit  ? ItemGetData( pLeftUnit  ) : NULL;

	int nNumGroups = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_ANIMATION_GROUP );
	if ( ! pLeftData )
	{
		if ( pRightData )
			nAnimationGroup = pRightData->nAnimGroup;
	} else {
		for ( int i = nNumGroups - 1; i >= 0; i-- )  // most of what we want is at the end of the list
		{
			ANIMATION_GROUP_DATA * pGroup = (ANIMATION_GROUP_DATA *) ExcelGetData( pGame, DATATABLE_ANIMATION_GROUP, i );
			if ( pRightData )
			{
				if ( pGroup->nRightWeapon == pRightData->nAnimGroup &&
					 pGroup->nLeftWeapon  == pLeftData ->nAnimGroup )
				{
					nAnimationGroup = i;
					break;
				}
			} else {
				if ( pGroup->nRightWeapon == INVALID_ID &&
					 pGroup->nLeftWeapon  == pLeftData->nAnimGroup )
				{
					nAnimationGroup = i;
					break;
				}
			}
		}
	}

	if ( nAnimationGroup == INVALID_ID )
		nAnimationGroup = DEFAULT_ANIMATION_GROUP;

	nAnimationGroup = AppearanceDefinitionFilterAnimationGroup( pGame, UnitGetAppearanceDefId( pUnit ), nAnimationGroup );

	return nAnimationGroup;
}


//----------------------------------------------------------------------------
//
// DEBUG DRAWING IN GDI
//
//----------------------------------------------------------------------------

#include "debugwindow.h"
#ifdef DEBUG_WINDOW
#include "s_gdi.h"

//----------------------------------------------------------------------------

static GAME * drbGame = NULL;
static VECTOR drbWorldPos;
static UNIT * drbPlayer = NULL;

//----------------------------------------------------------------------------

#define DRB_DEBUG_SCALE		10.0f

static void drbTransform( VECTOR & vPoint, int *pX, int *pY )
{
	VECTOR vTrans = vPoint - drbWorldPos;
	VectorScale( vTrans, vTrans, DRB_DEBUG_SCALE );
	*pX = (int)FLOOR( vTrans.fX ) + DEBUG_WINDOW_X_CENTER;
	*pY = (int)FLOOR( vTrans.fY ) + DEBUG_WINDOW_Y_CENTER;
}

static void drbDebugDraw()
{
	if ( !drbPlayer )
		return;

	drbWorldPos = drbPlayer->vPosition;

	int nX1, nY1;
	drbTransform( drbPlayer->vPosition, &nX1, &nY1 );
	gdi_drawpixel( nX1, nY1, gdi_color_red );
	int nRadius = (int)FLOOR( ( UnitGetCollisionRadius( drbPlayer ) * DRB_DEBUG_SCALE ) + 0.5f );
	gdi_drawcircle( nX1, nY1, nRadius, gdi_color_red );

	ROOM * room = UnitGetRoom( drbPlayer );
	for ( UNIT * monster = room->tUnitList.ppFirst[TARGET_BAD]; monster; monster = monster->tRoomNode.pNext )
	{
		if ( monster->nReservedType != UNIT_LOCRESERVE_NONE )
		{
			drbTransform( monster->vPosition, &nX1, &nY1 );
			gdi_drawpixel( nX1, nY1, gdi_color_green );
			nRadius = (int)FLOOR( ( UnitGetCollisionRadius( monster ) * DRB_DEBUG_SCALE ) + 0.5f );
			gdi_drawcircle( nX1, nY1, nRadius, gdi_color_green );
			int nX2, nY2;
			if ( monster->nReservedType == UNIT_LOCRESERVE_TARGET )
			{
				if ( UnitGetMoveTargetId(monster) != INVALID_ID)
				{
					UNIT * target = UnitGetById( drbGame, UnitGetMoveTargetId(monster) );
					if ( target )
					{
						BOUNDING_BOX box = monster->tReservedLocation;
						box.vMin += target->vPosition;
						box.vMax += target->vPosition;
						drbTransform( box.vMin, &nX1, &nY1 );
						drbTransform( box.vMax, &nX2, &nY2 );
					}
				}
			}
			else
			{
				drbTransform( monster->tReservedLocation.vMin, &nX1, &nY1 );
				drbTransform( monster->tReservedLocation.vMax, &nX2, &nY2 );
			}
			gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_cyan );
		}
	}

}

//----------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sUnitGetAimPointFromBone(
	UNIT* pUnit,
	VECTOR & vPosition )
{
	ASSERT_RETFALSE( IS_CLIENT( pUnit ) );
	int nModelId = c_UnitGetModelId( pUnit );
	if ( nModelId == INVALID_ID )
		return FALSE;
	VECTOR vOffset;
	int nAimBone = c_AppearanceGetAimBone( nModelId, vOffset );
	if ( nAimBone == INVALID_ID )
		return FALSE;

	MATRIX mBone;
	if ( ! c_AppearanceGetBoneMatrix( nModelId, &mBone, nAimBone ) )
		return FALSE;

	const MATRIX * pmWorld = e_ModelGetWorldScaled( nModelId );
	ASSERT_RETFALSE( pmWorld );

	MATRIX mTransform;
	MatrixMultiply( &mTransform, &mBone, pmWorld );

	MatrixMultiply( &vPosition, &vOffset, &mTransform );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetAimPoint(
	UNIT* pUnit,
	BOOL bForceOffset /* = FALSE */)
{
	BOOL bDoOffset = FALSE;
	VECTOR vPosition;
	if ( bForceOffset )
	{
		bDoOffset = TRUE;
	}
	else
	{
		if ( IS_SERVER( pUnit ) )
		{
			bDoOffset = TRUE;
		} 
		else
		{
			if ( ! c_sUnitGetAimPointFromBone( pUnit, vPosition ) )
				bDoOffset = TRUE;
		}
	}

	if ( bDoOffset )
	{
		ASSERT(pUnit);
		float fAimHeight = UnitGetStatFloat( pUnit, STATS_AIM_HEIGHT );
		if ( fAimHeight == 0.0f )
		{
			fAimHeight = UnitGetCollisionHeight( pUnit ) / 2.0f;
		} else {
			fAimHeight *= UnitGetScale( pUnit );
		}
		VECTOR vDelta = pUnit->vUpDirection;
		vDelta *= fAimHeight;
		vPosition = UnitGetPosition( pUnit );
		vPosition += vDelta;
	} 
	return vPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStartJump(
	GAME* game,
	UNIT* unit,
	float fZVelocity,
	BOOL bSend,
	BOOL bRemoveFromStepList)
{
	if (!UnitIsOnGround( unit ))
	{
		return;
	}

	if (IsUnitDeadOrDying(unit))
	{
		return;
	}

	if(UnitGetRoom(unit) == NULL)
	{
		return;
	}

	if(UnitHasState(game, unit, STATE_DISABLE_JUMP))
	{
		return;
	}

	UnitSetZVelocity(unit, fZVelocity);
	UnitSetOnGround( unit, FALSE );

	if (!UnitIsA(unit, UNITTYPE_PLAYER) && !UnitOnStepList(game, unit))
	{
		VECTOR vDirection = cgvZAxis;
		UnitSetMoveDirection(unit, vDirection);
		UnitStepListAdd(game, unit);
	}

	// ItemStartFlippy would be a better place to put this, but
	// it's only called on the server and I don't feel like re-arranging
	// the messages
	if (IS_CLIENT(game) && UnitGetGenus(unit) == GENUS_ITEM)
	{
#if !ISVERSION(SERVER_VERSION)
		int quality = ItemGetQuality( unit );
		if (quality <= INVALID_LINK)
		{
			return;
		}
		const ITEM_QUALITY_DATA* quality_data = ItemQualityGetData(quality);
		ASSERT_RETURN(quality_data);

		if (quality_data->nFlippySound != INVALID_LINK)
		{
			c_SoundPlay(quality_data->nFlippySound, &c_UnitGetPosition(unit), NULL);
		}
		else
		{
			const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
			if (item_data && item_data->m_nFlippySound != INVALID_ID)
			{
				c_SoundPlay(item_data->m_nFlippySound, &c_UnitGetPosition(unit), NULL);
			}
		}
#endif// !ISVERSION(SERVER_VERSION)
	}

	if (bRemoveFromStepList)
	{
		UnitRegisterEventHandler(game, unit, EVENT_LANDED_ON_GROUND, sJumpLandedOnGround, EVENT_DATA());
	}

	if (IS_SERVER(game) && bSend)
	{
		s_SendUnitJump(unit, NULL);
	}

	// set jumping flag
	UnitSetFlag( unit, UNITFLAG_JUMPING );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitJumpCancel(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// do nothing if not jumping
	if (UnitIsJumping( pUnit ) == FALSE)
	{
		return;
	}

	// clear flag
	UnitClearFlag( pUnit, UNITFLAG_JUMPING );
	
	// put unit on ground
	UnitSetOnGround( pUnit, TRUE );
	UnitSetZVelocity( pUnit, 0.0f );
	
	// trigger landed event
	GAME *pGame = UnitGetGame( pUnit );
	UnitTriggerEvent( pGame, EVENT_LANDED_ON_GROUND, pUnit, NULL, NULL );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsJumping(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitTestFlag( pUnit, UNITFLAG_JUMPING );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetSpeedsByMode(
	UNIT * unit,
	UNITMODE eMode,
	float & fSpeedBase,
	float & fSpeedMin,
	float & fSpeedMax)
{
	GAME * game = UnitGetGame(unit);
	const UNIT_DATA * pUnitData = UnitGetData( unit );
	ASSERT_RETFALSE(pUnitData);
	BOOL bModified( FALSE );
	switch (UnitGetGenus(unit))
	{
	case GENUS_MONSTER:
	case GENUS_PLAYER:
		{
			if (eMode == INVALID_ID)
			{
				return FALSE;
			}

			const UNITMODE_DATA * unitmode_data = (UNITMODE_DATA *)ExcelGetData(game, DATATABLE_UNITMODES, eMode);
			ASSERT_RETFALSE(unitmode_data->nVelocityIndex != INVALID_ID);

			fSpeedBase = pUnitData->tVelocities[ unitmode_data->nVelocityIndex ].fVelocityBase;
			fSpeedMin  = pUnitData->tVelocities[ unitmode_data->nVelocityIndex ].fVelocityMin;
			fSpeedMax  = pUnitData->tVelocities[ unitmode_data->nVelocityIndex ].fVelocityMax;
		}
		bModified = TRUE;
		break;

	case GENUS_MISSILE:
	case GENUS_ITEM:
		{

			fSpeedBase = pUnitData->tVelocities[0].fVelocityBase;
			fSpeedMin  = pUnitData->tVelocities[0].fVelocityMin;
			fSpeedMax  = pUnitData->tVelocities[0].fVelocityMax;
		}
		bModified = TRUE;
		break;
	case GENUS_OBJECT:
		return FALSE;
	}
	
	if( AppIsTugboat() &&
		unit &&
		pUnitData &&
		pUnitData->fDeltaVelocityModify > .001f)
	{
		RAND rand;
		RandInit( rand, UnitGetId( unit ) );	
		float rndNum = RandGetFloat( rand, 1.0f - pUnitData->fDeltaVelocityModify, 1.0f ); 
		fSpeedBase *= rndNum;
		fSpeedBase *= rndNum;
		fSpeedBase *= rndNum;
	}

	return bModified;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_UnitGetBloodParticleDef(
	GAME * game,
	UNIT * unit,
	int nCombatResult)
{
	const UNIT_DATA * pUnitData = UnitGetData( unit );
	return pUnitData->nBloodParticleId[ nCombatResult ];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float c_UnitGetMeleeImpactOffset(
	GAME * game,
	UNIT * unit)
{
	const UNIT_DATA * pUnitData = UnitGetData( unit );
	return pUnitData->fMeleeImpactOffset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitUpdateViewModel(
	UNIT * pUnit,
	BOOL bForce,
	BOOL bWeaponOnly )
{
	if ( !pUnit )
		return;
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( IS_CLIENT( pGame ) );
#if !ISVERSION(SERVER_VERSION)
	BOOL bChange = FALSE;
	BOOL bUseFirst = (c_CameraGetViewType() == VIEW_1STPERSON && GameGetCameraUnit( pGame ) == pUnit);

	int nModelDesired = bUseFirst ? c_UnitGetModelIdFirstPerson( pUnit ) : c_UnitGetModelIdThirdPerson( pUnit );

	if ( UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
	{
		int nOverride = c_UnitGetModelIdThirdPersonOverride( pUnit );
		ASSERT( nOverride != INVALID_ID );
		if ( nOverride != INVALID_ID )
			nModelDesired = nOverride;
		e_ModelSetDrawAndShadow( nModelDesired, TRUE );
	}

	if ( c_UnitGetModelId( pUnit ) != nModelDesired && nModelDesired != INVALID_ID )
		bChange = TRUE;

	if ( bForce )
		bChange = TRUE;

	if ( bChange )
	{
		c_UnitDisableHolyRadius( pUnit );
		int nModelFirst = c_UnitGetModelIdFirstPerson( pUnit );
		int nModelThird = c_UnitGetModelIdThirdPerson( pUnit );

		int nWardrobeThird = c_AppearanceGetWardrobe( nModelThird );
		ASSERT_RETURN ( nWardrobeThird != INVALID_ID );
		c_WardrobeUpdateAttachments( pUnit, nWardrobeThird, nModelThird, ! bUseFirst, bWeaponOnly );

		if ( AppIsHellgate() )
		{
			int nWardrobeFirst = c_AppearanceGetWardrobe( nModelFirst );
			ASSERT_RETURN( nWardrobeFirst != INVALID_ID );
			c_WardrobeUpdateAttachments ( pUnit, nWardrobeFirst, nModelFirst, bUseFirst, bWeaponOnly );
		}
		if ( AppIsHellgate() )
			e_ModelSetDrawAndShadow( nModelFirst, nModelDesired == nModelFirst );
		e_ModelSetDrawAndShadow( nModelThird, nModelDesired == nModelThird );

		int nAnimationGroup = UnitGetAnimationGroup( pUnit );
		c_AppearanceUpdateAnimationGroup( pUnit, pGame, nModelThird, nAnimationGroup );
		if ( AppIsHellgate() )
			c_AppearanceUpdateAnimationGroup( pUnit, pGame, nModelFirst, nAnimationGroup );

		if ( AppIsHellgate() )
			e_ModelSetFlagbit( nModelFirst, MODEL_FLAGBIT_FIRST_PERSON_PROJ, TRUE  );
		e_ModelSetFlagbit( nModelThird, MODEL_FLAGBIT_FIRST_PERSON_PROJ, FALSE );
		pUnit->pGfx->nModelIdCurrent = nModelDesired;

		if ( UnitGetStat( pUnit, STATS_HOLY_RADIUS_ENEMY_COUNT ) > 0 )
			c_UnitEnableHolyRadius( pUnit );

		if ( nModelDesired == nModelThird )
			c_UnitUpdateFaceTarget( pUnit, TRUE );
	}
#endif// !ISVERSION(SERVER_VERSION)
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sUnitUpdateRoomChangedClient(
	GAME * game,
	UNIT * unit,
	ROOM * roomOld,
	ROOM * roomNew,
	BOOL bSend)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(IS_CLIENT(game));

	if (GameGetControlUnit(game) != unit)
	{
		return;
	}

	if (roomNew)
	{
		
		const ROOM_INDEX * room_index = RoomGetRoomIndex(game, roomNew);
		ASSERT_RETURN(room_index);
		c_PlayerDisplayRoomMessage( roomNew->idRoom, room_index->nMessageString );
		c_SoundSetEnvironment(room_index->pReverbDefinition);
		c_BGSoundsPlay(room_index->nBackgroundSound);
	}
	else
	{
		c_SoundSetEnvironment(NULL);
		c_BGSoundsPlay(INVALID_ID);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sUnitUpdateRoomChangedServer(
	GAME * game,
	UNIT * unit,
	ROOM * roomOld,
	ROOM * roomNew,
	BOOL bSend)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(IS_SERVER(game));



	if (UnitIsA(unit, UNITTYPE_PLAYER) && roomNew )
	{
		//ASSERT_RETURN(roomNew);
				
		// activate this room if it's not active
		s_RoomActivate(roomNew, unit);
		
		ObjectsCheckForTriggersOnRoomChange(game, UnitGetClient(unit), unit, roomNew);
		s_TaskEventRoomEntered(game, unit, roomNew);
		s_QuestEventRoomEntered(game, unit, roomNew);
		
		// do quest sub level transitions
		if (roomOld != roomNew)
		{
			SUBLEVELID idSubLevelOld = roomOld ? RoomGetSubLevelID(roomOld) : INVALID_ID;
			SUBLEVELID idSubLevelNew = RoomGetSubLevelID( roomNew);
			if (idSubLevelOld != idSubLevelNew)
			{
				s_SubLevelTransition(unit, idSubLevelOld, roomOld, idSubLevelNew, roomNew);				
			}
		}
	}

	if (!bSend)
	{
		goto check_room_active;
	}

	// if this is a client control unit, send this room (and nearby rooms) that this client
	// does not know about to the client.  The contents of those rooms will also be sent.

	// send room information to this client	if they are ready to be a part of the game world
	if (UnitHasClient(unit))
	{
		RoomUpdateClient(unit);
	}

	if (!UnitCanSendMessages(unit))
	{
		goto check_room_active;
	}

	// if a unit moves from a room that any client did not know about into a room
	// that they do know about, we need to send the unit to them ... note that we do
	// not send this unit if this client "always knows" about it (ie, it is the
	// player itself or a pet or something)
	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		BOOL bUnitKnown = UnitIsKnownByClient(client, unit);
		BOOL bRoomKnown = (roomNew)?ClientIsRoomKnown(client, roomNew):FALSE;
		UNIT* pPlayer = ClientGetControlUnit(client);
		BOOL bUnitAlwaysKnownByPlayer = UnitIsAlwaysKnownByPlayer( unit, pPlayer );
		
		// when always known units change rooms, we queue a room update for those players
		// that should always know about them so that they can get the new room this unit is in
		if (bUnitAlwaysKnownByPlayer)
		{
			s_PlayerQueueRoomUpdate( pPlayer, 1 );
		}
		
		if (bUnitKnown && (roomNew == NULL || bRoomKnown == FALSE))
		{
		
			// if a unit is transitioning from a known room to an unknown room of this client
			// and is not always known by this client, we remove it from the client
			if (bUnitAlwaysKnownByPlayer == FALSE)
			{
				DWORD dwFlags = 0;
				if( AppIsTugboat() &&
					UnitIsA( unit, UNITTYPE_PLAYER ) ||
					UnitIsA( unit, UNITTYPE_MONSTER ) ||
					UnitIsA( unit, UNITTYPE_OBJECT ) ||
					UnitIsA( unit, UNITTYPE_DESTRUCTIBLE ) )
				{
					dwFlags = UFF_FADE_OUT;
				}
				s_RemoveUnitFromClient(unit, client, dwFlags);
			}			
		}
		else if (!bUnitKnown && bRoomKnown == TRUE)
		{
			// unit was not known by client, but entered into a known room of this client
			s_SendUnitToClient(unit, client);
		}
	}

check_room_active:
	if (roomOld && roomNew && !RoomIsActive(roomNew))
	{
		UnitEmergencyDeactivate(unit, "Moving unit into Inactive room");
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitUpdateRoomChanged(
	GAME * game,
	UNIT * unit,
	ROOM * roomOld,
	ROOM * roomNew,
	BOOL bSend)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	// check for players being assigned a room before they are allowed
	if (roomNew != NULL && IS_SERVER( game ) && UnitIsPlayer( unit ))
	{
		ASSERTV_RETURN( 
			UnitTestFlag( unit, UNITFLAG_ALLOW_PLAYER_ROOM_CHANGE ),
			"Attempting to put player '%s' in room '%s' before they are ready",
			UnitGetDevName( unit ),
			RoomGetDevName( roomNew ));
	}
	
	// take them out of the old room, put the unit in the new room
	bool bLevelTransition = !roomOld || !roomNew || roomOld->pLevel != roomNew->pLevel;
	UnitRemoveFromRoomList(unit, bLevelTransition);

	if (roomNew)
	{
		sUnitRoomListAdd(unit, roomNew, UnitGetTargetType(unit), bLevelTransition);
		roomNew = UnitGetRoom(unit);
	}
	else 
	{
		unit->pRoom = NULL;
	}
	
#if !ISVERSION(SERVER_VERSION)	
	if (IS_CLIENT(game) )
	{

		sUnitUpdateRoomChangedClient(game, unit, roomOld, roomNew, bSend);
	}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)	
	if (IS_SERVER(game) )
	{
		sUnitUpdateRoomChangedServer(game, unit, roomOld, roomNew, bSend);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitUpdateRoom(
	UNIT * unit,
	ROOM * roomNew,
	BOOL bSend)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	ROOM * roomOld = UnitGetRoom(unit);
	if (roomOld != roomNew)
	{
		sUnitUpdateRoomChanged(game, unit, roomOld, roomNew, bSend);
	}


	// Points of interest
	if( IS_SERVER( game ) && 
		( UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_IS_POINT_OF_INTEREST ) ||
		  UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_IS_STATIC_POINT_OF_INTEREST ) ) )
	{
		LEVEL *pLevel = RoomGetLevel( roomNew );
		ASSERT_RETURN( pLevel );
		//add the item to points of Interest
		s_LevelAddUnitOfInterest( pLevel, unit );

	}
#if ISVERSION(SERVER_VERSION)
	// update the unit's position in the level prox map.  regardless of whether the room changed, we need to notify 
	// clients that have just come in range of our current position and movement. 
	if (bSend)
	{
		s_SendDeferredMoveMsgsToProximity(unit);
	}


#endif //ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * UnitUpdateRoomFromPosition(
	GAME * game,
	UNIT * unit,
	const VECTOR * oldpos,	// = NULL
	bool bAssertOnNoRoom)	// = true
{
	if (oldpos && UnitGetPosition(unit) == *oldpos && unit->pRoom)
	{
		return unit->pRoom;
	}

	// call UnitUpdateRoom regardless of whether room has changed, because we may need to change level prox map
	// we do this is UnitUpdateRoom	because many parts of the code call that function directly after a position change
	ROOM * newroom = RoomGetFromPosition(game, unit->pRoom, &unit->vPosition);
	UnitUpdateRoom(unit, newroom ? newroom : unit->pRoom, true);

#if ISVERSION(DEVELOPMENT)
	if (bAssertOnNoRoom && !newroom && !UnitTestFlag(unit, UNITFLAG_ROOMASSERT)) 
	{
		WCHAR szName[DEFAULT_INDEX_SIZE];
		UnitGetName(unit, szName, arrsize(szName), 0);
		trace("Unit %S can't find a room\n", szName);
		UnitSetFlag(unit, UNITFLAG_ROOMASSERT);
	}
#endif
	return newroom;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitSinglePlayerSynch(
	GAME * cltGame,
	GAME * srvGame,
	UNIT * unit)
{
	ASSERT_RETURN(IS_CLIENT(unit));

#ifdef HAVOK_ENABLED
	if (unit->pHavokRigidBody)
	{
		return;
	}
#endif
	if (GameGetControlUnit(cltGame) == unit)
	{
		return;
	}

	UNIT * s_unit = UnitGetById(srvGame, UnitGetId(unit));
	if (!s_unit)
	{
		return;
	}

	const UNIT_DATA * pUnitData = UnitGetData(unit);
	if (!UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_CLIENT_ONLY ) && unit->vPosition != s_unit->vPosition && unit->pRoom)
	{
		UnitChangePosition(unit, s_unit->vPosition);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SinglePlayerSynch(
	GAME * game)
{
#if !ISVERSION(SERVER_VERSION) && !ISVERSION(CLIENT_ONLY_VERSION)
	if (!AppGetForceSynch())
	{
		return;
	}
	if (AppGetType() != APP_TYPE_SINGLE_PLAYER)
	{
		return;
	}
	ASSERT_RETURN(game);
	if (!IS_CLIENT(game))
	{
		return;
	}
	GAME * srvGame = AppGetSrvGame();
	if (!srvGame)
	{
		return;
	}

	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT * unit = game->pUnitHash->pHash[ii];
		while (unit)
		{
			sUnitSinglePlayerSynch(game, srvGame, unit);
			unit = unit->hashnext;
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//If the unit is a skill, this will return the skill ID, otherwise INVALID_ID
int UnitGetSkillID(		
	UNIT *pUnit)
{
	STATS_ENTRY tSkillStats;
	int nValues = UnitGetStatsRange(pUnit, STATS_SKILL_LEVEL, 0, &tSkillStats, 1);
	if ( nValues > 0 )
	{
		int nSkill = StatGetParam( STATS_SKILL_LEVEL, tSkillStats.GetParam(), 0 );

		return nSkill;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT UnitIsUsable(
	UNIT* unit,
	UNIT* item)
{
	ASSERTX_RETVAL(item, USE_RESULT_NOT_USABLE, "Expected item");
	
	// only items are usable
	if (UnitIsA( item, UNITTYPE_ITEM ))
	{
		// normal use item
		return ItemIsUsable( unit, item, NULL, ITEM_USEABLE_MASK_NONE );

	}
	
	return USE_RESULT_NOT_USABLE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsNoDrop(
	UNIT * item )
{
	ASSERT_RETFALSE( item );

	if ( UnitGetStat( item, STATS_ITEM_LOCKED_IN_INVENTORY) )
		return TRUE;

	// only items can be No Drop
	if ( UnitIsA( item, UNITTYPE_ITEM ) )
	{
		const UNIT_DATA * data = ItemGetData( item );
		if ( UnitDataTestFlag(data, UNIT_DATA_FLAG_NO_DROP_EXCEPT_FOR_DUPLICATES) )
		{
			// if they have duplicates, it's droppable
			UNIT *pContainer = UnitGetContainer( item );
			if (pContainer)
			{
				DWORD dwInvIterateFlags = 0;
				SETBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);
				int nItemCount = 
					UnitInventoryCountItemsOfType(
						pContainer,
						UnitGetClass(item),
						dwInvIterateFlags,
						ItemGetQuality(item),
						INVLOC_NONE);
				return nItemCount < 2;
			}
			return TRUE;
		}
		else if ( UnitDataTestFlag(data, UNIT_DATA_FLAG_NO_DROP) )
		{
			// NoDrop is ignored if NoDropExceptForDuplicates is set
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitAskForPickup(
	UNIT * item )
{
	ASSERT_RETFALSE( item );

	// only items can be No Drop
	if ( UnitIsA( item, UNITTYPE_ITEM ) )
	{
		const UNIT_DATA * data = ItemGetData( item );
		if ( UnitDataTestFlag(data, UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP) )
		{
			return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsExaminable(
	UNIT * item )
{
	ASSERT_RETFALSE(item);
	
	// only items are examinable
	if ( UnitIsA( item, UNITTYPE_ITEM ) )
	{
		if (UnitIsUsable(NULL, item))
		{
			return FALSE;
		}

		const UNIT_DATA * data = ItemGetData( item );
		return UnitDataTestFlag(data, UNIT_DATA_FLAG_EXAMINABLE);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsMerchant(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_MERCHANT);
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitMerchantFactionType(
	UNIT * unit )
{
	if (unit)
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		return pUnitData->nMerchantFactionType;
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitMerchantFactionValueNeeded(
	UNIT * unit )
{
	if (unit)
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		return pUnitData->nMerchantFactionValueNeeded;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitMerchantSharesInventory(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsMerchant( pUnit ) , "Expected merchant" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_MERCHANT_SHARES_INVENTORY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitTradesmanSharesInventory(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsTradesman( pUnit ), "Expected tradesman" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_MERCHANT_SHARES_INVENTORY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsTrader(
	UNIT *pUnit)
{
	if (pUnit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRADER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsTrainer(
	UNIT *pUnit)
{
	if (pUnit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRAINER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsTaskGiver(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TASKGIVER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsTradesman(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRADESMAN);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGambler(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GAMBLER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsStatTrader(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_STATTRADER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGodMessanger(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GOD_MESSENGER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsItemUpgrader(
	UNIT *pUnit)
{
	if (pUnit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ITEM_UPGRADER);
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsItemAugmenter(
	UNIT *pUnit)
{
	if (pUnit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ITEM_AUGMENTER);
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitAutoIdentifiesInventory(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_AUTO_IDENTIFIES_INVENTORY);
	}
	return FALSE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsDungeonWarper(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_DUNGEON_WARPER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGuildMaster(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GUILDMASTER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsRespeccer(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_RESPECCER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsCraftingRespeccer(
					 UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_CRAFTING_RESPECCER);
	}
	return FALSE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsTransporter(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRANSPORTER);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsForeman(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_FOREMAN);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsPvPSignerUpper(
	UNIT* unit)
{
	if (unit && AppIsMultiplayer())
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_PVP_SIGNER_UPPER);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char* UnitGetDevName(
	UNIT* unit)
{
	static const char szDefault[] = "unknown";
	ASSERT_RETVAL(unit, szDefault);
	const UNIT_DATA * pUnitData = UnitGetData( unit );
	ASSERT_RETVAL(pUnitData, szDefault);
	return pUnitData->szName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *UnitGetDevName( 
	GAME *pGame,
	UNITID idUnit)
{
	static const char *pszUnknown = "UNKNOWN";
	UNIT *pUnit = UnitGetById( pGame, idUnit );
	if (pUnit)
	{
		return UnitGetDevName( pUnit );
	}
	return pszUnknown;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * UnitGetDevNameBySpecies(
	SPECIES species)
{
	static const char * szUnknown = "unknown";

	// handle no species	
	if (species == SPECIES_NONE)
	{
		return szUnknown;
	}
	
	GENUS genus = (GENUS)GET_SPECIES_GENUS(species);
	int unitclass = GET_SPECIES_CLASS(species);	
	EXCELTABLE eTable = ExcelGetDatatableByGenus(genus);
	ASSERTV_RETVAL(eTable != DATATABLE_NONE, szUnknown, "Invalid genus (%d) in unit", genus);

	// get unit data		
	const UNIT_DATA * unit_data = (const UNIT_DATA *)ExcelGetData(NULL, eTable, unitclass);
	if (!unit_data)
	{
		return szUnknown;
	}
	return unit_data->szName;
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameTraceUnits(
	GAME* game,
	BOOL bVerbose)
{
	ASSERT_RETURN(game);
	struct UNIT_DEBUG
	{
		UNIT*	unit;
		int		count;
		SPECIES species;
	};
	UNIT_DEBUG unit_debug[2048];
	int size = 0;
	int total[NUM_GENUS];
	memclear(total, NUM_GENUS * sizeof(int));

	const int MAX_MESSAGE = 1024;
	char szMessage[ MAX_MESSAGE ];
	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"--------------------------- Tracing units for: %s\n", 
		IS_SERVER(game) ? "SERVER" : "CLIENT");
	ConsoleString(CONSOLE_SYSTEM_COLOR, szMessage );
	trace( szMessage );

	// iterate units
	for (int ii = 0; ii < UNIT_HASH_SIZE; ii++)
	{
		UNIT* unit = game->pUnitHash->pHash[ii];
		while (unit)
		{
			total[0]++;
			total[UnitGetGenus(unit)]++;

			BOOL found = FALSE;
			for (int jj = 0; jj < size; jj++)
			{
				if (unit_debug[jj].species == unit->species)
				{
					unit_debug[jj].count++;
					found = TRUE;
					break;
				}
			}
			if (!found)
			{
				ASSERT(size < 2048);
				if (size < 2048)
				{
					unit_debug[size].unit = unit;
					unit_debug[size].species = unit->species;
					unit_debug[size].count = 1;
					size++;
				}
			}
			
			// if verbose, print info
			if (bVerbose)
			{
				UNIT *pContainer = UnitGetContainer( unit );
				
				if (pContainer)
				{
					PStrPrintf( 
						szMessage,
						MAX_MESSAGE,
						"'%s' [%d] Contained by '%s' [%d]\n",
						UnitGetDevName( unit ),
						UnitGetId( unit ),
						UnitGetDevName( pContainer ),
						UnitGetId( pContainer ));
				}
				else
				{
					PStrPrintf( 
						szMessage,
						MAX_MESSAGE,
						"'%s' [%d]\n",
						UnitGetDevName( unit ),
						UnitGetId( unit ));
				}

				ConsoleString(CONSOLE_SYSTEM_COLOR, szMessage );
				trace( szMessage );				
				
			}
			
			// next unit
			unit = unit->hashnext;
			
		}
		
	}
	
	// display short info	
	if (bVerbose == FALSE)
	{
	
		for (int ii = 0; ii < size; ii++)
		{
			 
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"%-32s  %12d\n", 
				UnitGetDevName(unit_debug[ii].unit),
				unit_debug[ii].count );
			ConsoleString(CONSOLE_SYSTEM_COLOR, szMessage );
			trace( szMessage );
		}

	}
		
	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"================= Total: %d  (%d monsters, %d items, %d missiles) ===============\n", 
		total[0], 
		total[GENUS_MONSTER], 
		total[GENUS_ITEM], 
		total[GENUS_MISSILE]);		
	ConsoleString(CONSOLE_SYSTEM_COLOR, szMessage );
	trace( szMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetOwner(
	UNIT * unit,
	UNIT * owner)
{
	if (owner)
	{
		unit->idOwner = UnitGetId(owner);
	}
	else
	{
		unit->idOwner = UnitGetId(unit);  // we are our own owner
	}

	//// Set any items contained in this item to the same owner
	//UNIT * pItem = NULL;
	//while (pItem = UnitInventoryIterate(unit, pItem))
	//{
	//	UnitSetOwner(pItem, owner);
	//}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInTown(
	UNIT* unit)
{
#if ISVERSION(DEVELOPMENT)
	if(AppTestDebugFlag( ADF_TOWN_FORCED ))
	{
		return TRUE;
	}
#endif

	ROOM * pRoom = UnitGetRoom( unit );
	if (! pRoom )
		return FALSE;

	LEVEL* pLevel = RoomGetLevel(pRoom);
	if (! pLevel )
		return FALSE;

	return LevelIsTown( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInPVPTown(
				  UNIT* unit)
{

	ROOM * pRoom = UnitGetRoom( unit );
	if (! pRoom )
		return FALSE;

	LEVEL* pLevel = RoomGetLevel(pRoom);
	if (! pLevel )
		return FALSE;

	return LevelIsTown( pLevel ) && LevelGetPVPWorld( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInRTSLevel(
	UNIT * unit)
{
	ROOM * pRoom = UnitGetRoom( unit );
	if (! pRoom )
		return FALSE;

	LEVEL* pLevel = RoomGetLevel(pRoom);
	if (! pLevel )
		return FALSE;

	return LevelIsRTS( pLevel );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInPortalSafeLevel(
	UNIT* unit)
{
	ROOM * pRoom = UnitGetRoom( unit );
	if (! pRoom )
		return FALSE;

	LEVEL* pLevel = RoomGetLevel(pRoom);
	if (! pLevel )
		return FALSE;

	return !pLevel->m_pLevelDefinition->bDisableTownPortals;	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInHellrift(
	UNIT *pUnit)
{
	ROOM * pRoom = UnitGetRoom( pUnit );
	if ( !pRoom )
	{
		return FALSE;
	}
	return RoomIsHellrift( pRoom );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsNPC(
	const UNIT* unit)
{
	ASSERT(unit);
	if (UnitGetGenus( unit ) == GENUS_MONSTER )
	{
		const UNIT_DATA *pMonsterData = MonsterGetData(UnitGetGame(unit), UnitGetClass(unit));
		return UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_IS_NPC);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitPvPIsEnabled(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	if ( UnitIsPlayer( pUnit ) )
	{
		// if we have an aggression list, then baby, we're in PVP!
		if( AppIsTugboat() )
		{
			if( UnitGetStat( pUnit, STATS_AGGRESSION_LIST_COUNT ) > 0 )
				return TRUE;
		}
		return PlayerPvPIsEnabled( pUnit );
	}
	else
	{
		UNIT *pOwner = UnitGetUltimateOwner( pUnit );
		if( pOwner && UnitIsPlayer(pOwner) )
		{
			// if we have an aggression list, then baby, we're in PVP!
			if( AppIsTugboat() )
			{
				if( UnitGetStat( pOwner, STATS_AGGRESSION_LIST_COUNT ) > 0 )
					return TRUE;
			}
			return PlayerPvPIsEnabled( pOwner );
		}
		else
		{
			return FALSE;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetChecksRadiusWhenPathing(
	const UNIT * pUnit)
{
	ASSERT_RETFALSE(pUnit);
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETFALSE(pUnitData);

	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CHECK_RADIUS_WHEN_PATHING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitGetChecksHeightWhenPathing(
	const UNIT * pUnit)
{
	ASSERT_RETFALSE(pUnit);
	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	ASSERT_RETFALSE(pUnitData);

	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CHECK_HEIGHT_WHEN_PATHING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetPathingCollisionRadius(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flRadius = pUnitData->fPathingCollisionRadius;
	
	// for monsters we support looking at the scale ... you may remove this check
	// and do it for all units if you like, but you will have to add data to
	// all the spreadsheets	
	//if (UnitGetGenus(pUnit) == GENUS_MONSTER && UnitGetScale( pUnit ) > 1 )  // Tugboat version... ?
	if ( UnitIsA( pUnit, UNITTYPE_MONSTER ) && ( AppIsHellgate() || UnitGetScale( pUnit ) > 1 ) )
	{
		flRadius *= UnitGetScale( pUnit );	
	}

#if ISVERSION(DEVELOPMENT)	// there seem to be some bugs if radius is too small?
	ASSERT(flRadius >= 0.1f);
	if (flRadius < 0.1f)
	{
		flRadius = 0.1f;
	}
#endif
	
	return flRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetCollisionRadius(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flRadius = pUnitData->fCollisionRadius;
	
	// for monsters we support looking at the scale ... you may remove this check
	// and do it for all units if you like, but you will have to add data to
	// all the spreadsheets	
	//if (UnitGetGenus(pUnit) == GENUS_MONSTER && UnitGetScale( pUnit ) > 1 )  // old Tugboat version
	if ( UnitIsA( pUnit, UNITTYPE_MONSTER ) && ( AppIsHellgate() || UnitGetScale( pUnit ) > 1 ) )
	{
		flRadius *= UnitGetScale( pUnit );	
	}
	
	return flRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetRawCollisionRadius(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flRadius = pUnitData->fCollisionRadius;
	return flRadius;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetCollisionRadiusHorizontal(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flRadius = pUnitData->fCollisionRadiusHorizontal;
	if( flRadius == -1.0f )
	{
		return UnitGetCollisionRadius( pUnit );
	}
	// for monsters we support looking at the scale ... you may remove this check
	// and do it for all units if you like, but you will have to add data to
	// all the spreadsheets	
	//if (UnitGetGenus(pUnit) == GENUS_MONSTER && UnitGetScale( pUnit ) > 1 )  // old Tugboat version
	if ( UnitIsA( pUnit, UNITTYPE_MONSTER ) && ( AppIsHellgate() || UnitGetScale( pUnit ) > 1 ) )
	{
		flRadius *= UnitGetScale( pUnit );	
	}
	
	return flRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetRawCollisionRadiusHorizontal(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flRadius = pUnitData->fCollisionRadiusHorizontal;
	if( flRadius == -1.0f )
	{
		return UnitGetRawCollisionRadius( pUnit );
	}

	return flRadius;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetCollisionHeight(
	const UNIT * pUnit)
{
	ASSERTX_RETVAL( pUnit, 1.0f, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	float flHeight = pUnitData->fCollisionHeight;
	
	// for monsters we support looking at the scale ... you may remove this check
	// and do it for all units if you like, but you will have to add data to
	// all the spreadsheets
//	if (UnitGetGenus(pUnit) == GENUS_MONSTER && UnitGetScale( pUnit ) > 1 )  // old Tugboat version
	if ( ( AppIsHellgate() || UnitGetScale( pUnit ) > 1 ) && UnitIsA( pUnit, UNITTYPE_MONSTER )  )
	{
		flHeight *= UnitGetScale( pUnit );	
	}
	
	return flHeight;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetMaxCollisionRadius( 
	GAME *pGame,
	GENUS eGenus,	
	int nUnitClass)
{
	const UNIT_DATA *pUnitData = UnitGetData( pGame, eGenus, nUnitClass );
	return pUnitData->fCollisionRadius * (pUnitData->fScale + pUnitData->fScaleDelta);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetMaxCollisionHeight( 
	GAME *pGame,
	GENUS eGenus,	
	int nUnitClass)
{
	const UNIT_DATA *pUnitData = UnitGetData( pGame, eGenus, nUnitClass );
	return pUnitData->fCollisionHeight * (pUnitData->fScale + pUnitData->fScaleDelta);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetAppearanceDefId(
	const UNIT* unit,
	BOOL bForceThirdPerson)
{
	if ( ! unit )
		return INVALID_ID;
	if ( UnitTestFlag( unit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
		return unit->nAppearanceDefIdOverride;
	int nThirdPersonAppearance = unit->nAppearanceDefId;
	if ( bForceThirdPerson )
		return nThirdPersonAppearance;
	return unit->pUnitData->nAppearanceDefId_FirstPerson != INVALID_ID ? unit->pUnitData->nAppearanceDefId_FirstPerson : nThirdPersonAppearance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int UnitGetAppearanceDefIdForCamera( const UNIT* unit )
{
	if ( !unit )
		return INVALID_ID;
	return ( unit->pGfx && unit->pGfx->nAppearanceDefIdForCamera != INVALID_ID ) ? 
		unit->pGfx->nAppearanceDefIdForCamera : UnitGetAppearanceDefId( unit );
}

//----------------------------------------------------------------------------
float UnitsGetDistanceSquared(
	const UNIT* unitFirst,
	const UNIT* unitSecond)
{
	ASSERT_RETZERO(unitFirst);
	ASSERT_RETZERO(unitSecond);

	if (UnitsAreInSameSubLevel( unitFirst, unitSecond ) == FALSE)
	{
		return DISTANCE_ON_ANOTHER_LEVEL;
	}

	return VectorDistanceSquared(UnitGetPosition(unitFirst), UnitGetPosition(unitSecond));
}

//----------------------------------------------------------------------------
float UnitsGetDistanceSquaredXY(
	const UNIT* unitFirst,
	const UNIT* unitSecond)
{
	ASSERT_RETZERO(unitFirst);
	ASSERT_RETZERO(unitSecond);

	if (UnitsAreInSameSubLevel( unitFirst, unitSecond ) == FALSE)
	{
		return DISTANCE_ON_ANOTHER_LEVEL;
	}
	
	return VectorDistanceSquaredXY(UnitGetPosition(unitFirst), UnitGetPosition(unitSecond));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitsAreInRange(
	const UNIT* unit,
	const UNIT* target,
	float fRangeMin,
	float fRangeMax,
	float * pfDistanceSquaredIn )
{
	if(UnitGetLevel(unit) != UnitGetLevel(target))
	{
		return FALSE;
	}
	float fDistanceSquared = pfDistanceSquaredIn ? *pfDistanceSquaredIn :
		( AppIsHellgate() ? UnitsGetDistanceSquared( unit, target ) : UnitsGetDistanceSquaredXY( unit, target ) );

	float fUnitRadius   = UnitGetCollisionRadius( unit );
	float fTargetRadius = UnitGetCollisionRadius( target );
	float fRadiusAndRange = fUnitRadius + fTargetRadius + fRangeMax;
	if ( fDistanceSquared > fRadiusAndRange * fRadiusAndRange )
		return FALSE;
	if ( fRangeMin )
	{
		fRadiusAndRange = fUnitRadius + fTargetRadius + fRangeMin;
		if ( fDistanceSquared < fRadiusAndRange * fRadiusAndRange )
			return FALSE;
	}
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInRange(
	const UNIT* unit,
	const VECTOR & vTarget,
	float fRangeMin,
	float fRangeMax,
	float * pfDistanceSquaredIn )
{
	ASSERT_RETFALSE(unit);
	VECTOR vUnitPosition = UnitGetPosition( unit );
	float fDistanceSquared;
	if ( pfDistanceSquaredIn && *pfDistanceSquaredIn != 0.0f )
		fDistanceSquared = *pfDistanceSquaredIn;
	else {
		fDistanceSquared = VectorDistanceSquaredXY( vUnitPosition, vTarget );

		if ( vUnitPosition.fZ > vTarget.fZ )
		{
			float fDistanceSquaredZ = vUnitPosition.fZ - vTarget.fZ;
			fDistanceSquaredZ *= fDistanceSquaredZ;
			fDistanceSquared += fDistanceSquaredZ;
		} else {
			float fDeltaZ = vTarget.fZ - vUnitPosition.fZ - UnitGetCollisionHeight( unit );
			if ( fDeltaZ > 0.0f )
			{
				fDeltaZ *= fDeltaZ;
				fDistanceSquared += fDeltaZ;
			}
		}
	}

	float fUnitRadius   = UnitGetCollisionRadius( unit );

	float fRadiusAndRange = fUnitRadius + fRangeMax;
	if ( fDistanceSquared > fRadiusAndRange * fRadiusAndRange )
		return FALSE;
	if ( fRangeMin )
	{
		fRadiusAndRange = fUnitRadius + fRangeMin;
		if ( fDistanceSquared < fRadiusAndRange * fRadiusAndRange )
			return FALSE;
	}

	if ( pfDistanceSquaredIn )
	{
		*pfDistanceSquaredIn = sqrt( fDistanceSquared ) - fUnitRadius;
		*pfDistanceSquaredIn = (*pfDistanceSquaredIn > 0.0f) ? ((*pfDistanceSquaredIn) * (*pfDistanceSquaredIn)) : 0.0f;
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInRange(
	const UNIT* unit,
	const VECTOR & vPosition,
	float fRange,
	float * pfDistanceSquared )
{
	return UnitIsInRange( unit, vPosition, 0.0f, fRange, pfDistanceSquared );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetPositionAtPercentHeight(
	UNIT* unit,
	float fPercent )
{
	ASSERT_RETVAL(unit, INVALID_POSITION);

	float fHeight = UnitGetCollisionHeight( unit );
	VECTOR vDelta = unit->vUpDirection;
	vDelta *= fHeight * fPercent;
	VECTOR vPosition = UnitGetPosition( unit );
	vPosition += vDelta;
	return vPosition;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetCenter(
	UNIT* unit)
{
	return UnitGetPositionAtPercentHeight( unit, 0.5f );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sExcelPostProcessUnitSetAppearanceDefId(
	const char * pszGenusFolder,
	const char * pszFolder,
	const char * pszName )
{
	if (!pszName || pszName[0] == 0)
	{
		return INVALID_ID;
	} 
	else 
	{
		char szFullName[MAX_PATH];
		PStrPrintf(szFullName, MAX_PATH, "%s%s\\%s%s", pszGenusFolder, pszFolder, pszName, APPEARANCE_SUFFIX);
		return DefinitionGetIdByFilename(DEFINITION_GROUP_APPEARANCE, szFullName);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_sGetParticleDefId(
	const char * pszName )
{
	if ( pszName && pszName[ 0 ] != 0 )
		return DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, pszName );
	else
		return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ExcelTableGetDataFolder(
	unsigned int idTable)
{
	switch (idTable)
	{
	case DATATABLE_ITEMS:				return FILE_PATH_ITEMS;
	case DATATABLE_MISSILES:			return FILE_PATH_MISSILES;
	case DATATABLE_MONSTERS:			return FILE_PATH_UNITS;
	case DATATABLE_OBJECTS:				return FILE_PATH_OBJECTS;
	case DATATABLE_PLAYERS:				return FILE_PATH_PLAYERS;
	default:							return FILE_PATH_DATA;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelPostProcessUnitDataInit(
	UNIT_DATA * unit_data,
	unsigned int idTable)
{
	ASSERT_RETURN(unit_data);

	unit_data->nAppearanceDefId	= sExcelPostProcessUnitSetAppearanceDefId(ExcelTableGetDataFolder(idTable), unit_data->szFileFolder, unit_data->szAppearance );
	unit_data->nAppearanceDefId_FirstPerson = sExcelPostProcessUnitSetAppearanceDefId(ExcelTableGetDataFolder(idTable), unit_data->szFileFolder, unit_data->szAppearance_FirstPerson);

	if (AppGetType() != APP_TYPE_CLOSED_SERVER) 
	{
		for (unsigned int ii = 0; ii < NUM_UNIT_COMBAT_RESULTS; ++ii)
		{
			unit_data->nBloodParticleId[ii] = c_sGetParticleDefId(unit_data->szBloodParticles[ii]);
		}
		unit_data->nHolyRadiusId = c_sGetParticleDefId(unit_data->szHolyRadius);
		unit_data->nFizzleParticleId = c_sGetParticleDefId(unit_data->szFizzleParticle);
		unit_data->nReflectParticleId = c_sGetParticleDefId(unit_data->szReflectParticle);
		unit_data->nRestoreVitalsParticleId = c_sGetParticleDefId(unit_data->szRestoreVitalsParticle);
		unit_data->nDamagingMeleeParticleId = c_sGetParticleDefId(unit_data->szDamagingMeleeParticle);

		for (unsigned int ii = 0; ii < UNIT_TEXTURE_OVERRIDE_COUNT; ++ii)
		{
			for (unsigned int jj = 0; jj < LOD_COUNT; ++jj)
			{
				unit_data->pnTextureOverrideIds[ii][jj] = INVALID_ID;
			}
		}
	
		for( unsigned int ii = 0; ii < MAX_UNIT_TEXTURE_OVERRIDES; ii++ )
		{
			unit_data->pnTextureSpecificOverrideIds[ii] = INVALID_ID;
		}

		for (unsigned int ii = 0; ii < NUM_MISSILE_EFFECTS; ii++)
		{
			c_ParticleEffectSetLoad( &unit_data->MissileEffects[ii], unit_data->szParticleFolder );
		}
	}

	// special effects
	SpecialEffectPostProcess(unit_data);

	SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_LOADED, FALSE);
	SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_INITIALIZED, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelPostProcessUnitDataInit(
	EXCEL_TABLE * table,
	BOOL bDelayLoad)
{
	if (AppIsHammer())
	{
		return;
	}

	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{
		BOOL QuickStartDesktopServerIsEnabled();
		bDelayLoad = QuickStartDesktopServerIsEnabled();
	}

	unsigned int count = ExcelGetCountPrivate(table);
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		UNIT_DATA * unit_data = (UNIT_DATA *)ExcelGetDataPrivate(table, ii);
		if ( ! unit_data )
			continue;

		SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_INITIALIZED, FALSE);

		if (!bDelayLoad)
		{
			sExcelPostProcessUnitDataInit(unit_data, table->m_Index);
		}

#ifdef HAVOK_ENABLED
		if (AppIsHellgate())
		{
			sUnitDataInitHavokShape(NULL, unit_data);
		}
#endif

		SETBIT(unit_data->pdwFlags, UNIT_DATA_FLAG_LOADED, FALSE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitDataFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	UNIT_DATA * unit_data = (UNIT_DATA *)rowdata;

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		if (unit_data->tHavokShapePrefered.pHavokShape)
		{
			HavokReleaseShape(unit_data->tHavokShapePrefered);
		}
		if (unit_data->tHavokShapeFallback.pHavokShape)
		{
			HavokReleaseShape(unit_data->tHavokShapeFallback);
		}
	}
#endif

#if !ISVERSION(SERVER_VERSION)
	for (unsigned int ii = 0; ii < UNIT_TEXTURE_OVERRIDE_COUNT; ii++)
	{
		for (unsigned int jj = 0; jj < LOD_COUNT; ++jj)
		{
			//ASSERTV( unit_data->pnTextureOverrideIds[ii][jj] != 0, "Erroneous zero texture ID encountered in unit data overrides!\n\n%s", unit_data->szName );
			//e_TextureReleaseRef(unit_data->pnTextureOverrideIds[ii][jj]);
			unit_data->pnTextureOverrideIds[ii][jj] = INVALID_ID;
		}
	}
	for (unsigned int ii = 0; ii < MAX_UNIT_TEXTURE_OVERRIDES; ii++)
	{
		//e_TextureReleaseRef(unit_data->pnTextureSpecificOverrideIds[ii]);
		unit_data->pnTextureSpecificOverrideIds[ii] = INVALID_ID;
	}
#endif

	if ( unit_data->nIconTextureId != INVALID_ID )
	{
		e_TextureReleaseRef( unit_data->nIconTextureId );
		unit_data->nIconTextureId = INVALID_ID;
	}

	if (unit_data->pnAchievements)
	{
		FREE(NULL, unit_data->pnAchievements);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnknownEffect(	
	const char *pszSourceName, 
	const char *szParticleName)
{
	const int MAX_MESSAGE = 2048;
	char szMessage[ MAX_MESSAGE ];

	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"Invalid particle effect '%s' in '%s'",
		szParticleName,
		pszSourceName);

	ASSERT_MSG( szMessage );		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InitEffectSet(
	PARTICLE_EFFECT_SET *pEffectSet,
	const char * pszParticleFolder,
	const char * pszSourceName)
{
	if (pEffectSet->bInitialized == FALSE)
	{

		// setup buffer where we will store particle name	
		char szParticleName[ DEFAULT_FILE_WITH_PATH_SIZE ];

		// setup folder where we will look for particle
		char szFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrCopy( szFolder, pszParticleFolder, DEFAULT_FILE_WITH_PATH_SIZE );
		PStrForceBackslash( szFolder, DEFAULT_FILE_WITH_PATH_SIZE );

		// look up default particle
		pEffectSet->nDefaultId = INVALID_LINK;
		if (pEffectSet->szDefault[ 0 ] != 0)
		{ 
			const char * pszName = pEffectSet->szDefault;
			if ( pEffectSet->szDefault[ 0 ] != '.' )
			{
				PStrPrintf( szParticleName, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szFolder, pEffectSet->szDefault );
				pszName = szParticleName;
			} else {
				pszName++;
			}
			pEffectSet->nDefaultId = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, pszName );
			if (pEffectSet->nDefaultId == INVALID_LINK)
			{
				sUnknownEffect( pszSourceName, pszName );
			}
		}

		// look up other particles
		for (int j = 0; j < NUM_DAMAGE_TYPES; ++j)
		{
			PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffectSet->tElementEffect[ j ];				

			pElementEffect->nPerElementId = INVALID_LINK;			
			if (pElementEffect->szPerElement && pElementEffect->szPerElement[0])
			{
				const char * pszName = pElementEffect->szPerElement;
				if ( pElementEffect->szPerElement[ 0 ] != '.' )
				{
					PStrPrintf( szParticleName, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szFolder, pElementEffect->szPerElement );
					pszName = szParticleName;
				}
				pElementEffect->nPerElementId = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, pszName );
				if (pElementEffect->nPerElementId == INVALID_LINK)
				{
					sUnknownEffect( pszSourceName, pszName );
				}
			}
		}	

		pEffectSet->bInitialized = TRUE;

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitDataLoadGfx(void * pDef, EVENT_DATA * pEventData)
{
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *)pDef;
	EXCELTABLE eTable = (EXCELTABLE)pEventData->m_Data1;
	int nClass = (int)pEventData->m_Data2;

	c_sUnitDataLoadTextures( pAppearanceDef, eTable, nClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitDataLoadAppearanceSounds(void * pDef, EVENT_DATA * pEventData)
{
#if !ISVERSION(SERVER_VERSION)
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *)pDef;
	c_AppearanceDefinitionFlagSoundsForLoad(pAppearanceDef, TRUE);
#endif
}

struct DATA_TO_LOAD
{
	UNIT_DATA_LOAD_TYPE	eLoadType;
	BOOL				bLoadIcon;
	BOOL				bLoadSounds;
	BOOL				bLoadAppearance;
	BOOL				bLoadSkills;
};

static DATA_TO_LOAD sgpDataToLoadMap[] =
{
//		Load Type,				Load Icon,		Load Sounds,		Load Appearance,	Load Skills
	{	UDLT_NONE,				FALSE,			FALSE,				FALSE,				FALSE		},
	{	UDLT_WORLD,				TRUE,			TRUE,				TRUE,				FALSE		},
	{	UDLT_INVENTORY,			TRUE,			TRUE,				FALSE,				FALSE		},
	{	UDLT_INSPECT,			TRUE,			TRUE,				TRUE,				FALSE		},
	{	UDLT_TOWN_OTHER,		FALSE,			FALSE,				TRUE,				FALSE		},
	{	UDLT_TOWN_YOU,			TRUE,			TRUE,				TRUE,				FALSE		},
	{	UDLT_WARDROBE_OTHER,	FALSE,			FALSE,				TRUE,				TRUE		},
	{	UDLT_WARDROBE_YOU,		TRUE,			TRUE,				TRUE,				TRUE		},
	{	UDLT_ALL,				TRUE,			TRUE,				TRUE,				TRUE		},
};

static const int sgnDataToLoadMapSize = arrsize(sgpDataToLoadMap);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitDataLoad(
	GAME * pGame,
	EXCELTABLE eTable,
	int nIndex,
	UNIT_DATA_LOAD_TYPE eLoadType)
{
	// CHB 2006.10.04 - This really needs to be defined.
#if !ISVERSION(SERVER_VERSION)
	BOOL bPreload = TRUE;
#endif

	ASSERTX_RETFALSE(eLoadType >= 0 && eLoadType <= UDLT_ALL, "Invalid load type!");

	// I don't want to break tugboat, so we'll just make it work the same way for now
	if(AppIsTugboat())
	{
		eLoadType = UDLT_ALL;
	}

	UNIT_DATA * pUnitData = (UNIT_DATA *)ExcelGetDataNotThreadSafe( NULL, eTable, nIndex ); 
	if ( ! pUnitData )
		return FALSE;

	if (!UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_INITIALIZED))
	{
		sExcelPostProcessUnitDataInit(pUnitData, eTable );	// not thread safe!
	}

	if (AppGetType() != APP_TYPE_CLOSED_SERVER && IS_CLIENT(pGame))
	{
#if !ISVERSION(SERVER_VERSION)
		DATA_TO_LOAD * pDataToLoad = &sgpDataToLoadMap[eLoadType];
		ASSERT_RETFALSE(pDataToLoad);

		// icon
		if ( AppIsHellgate() && pDataToLoad->bLoadIcon && UnitIsA( pUnitData->nUnitType, UNITTYPE_ITEM ) && ! c_GetToolMode() )
		{
			GENDER eGenders[ GENDER_NUM_CHARACTER_GENDERS ];
			int nGenders;
			if ( AppCommonIsAnyFillpak() && ItemUsesWardrobe( pUnitData ) && ! UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_NO_GENDER_FOR_ICON ) )
			{
				for ( int i = 0; i < GENDER_NUM_CHARACTER_GENDERS; ++i )
					eGenders[i] = (GENDER)(i);
				nGenders = GENDER_NUM_CHARACTER_GENDERS;
			}
			else
			{
				nGenders = 1;
				UNIT * pControlUnit = GameGetControlUnit( pGame );
				eGenders[0] = pControlUnit ? UnitGetGender( pControlUnit ) : GENDER_INVALID;
			}

			for ( int i = 0; i < nGenders; ++i )
			{
				// load the item's icon
				if ( pUnitData->nIconTextureId != INVALID_ID )
				{
					e_TextureReleaseRef( pUnitData->nIconTextureId );
					pUnitData->nIconTextureId = INVALID_ID;
				}
				OS_PATH_CHAR oszFilename[ DEFAULT_FILE_WITH_PATH_SIZE ] = OS_PATH_TEXT("");

				V( ItemUnitDataGetIconFilename( oszFilename, DEFAULT_FILE_WITH_PATH_SIZE, pUnitData, eGenders[i] ) );
				char szFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
				char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
				PStrCvt( szFilename, oszFilename, DEFAULT_FILE_WITH_PATH_SIZE );
				PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", ExcelTableGetDataFolder(DATATABLE_ITEMS), pUnitData->szFileFolder );
				PStrForceBackslash( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE );
				PStrCat( szFilePath, szFilename, DEFAULT_FILE_WITH_PATH_SIZE );

				V( e_TextureNewFromFile( &pUnitData->nIconTextureId, szFilePath, TEXTURE_GROUP_UI, TEXTURE_DIFFUSE ) );
				e_TextureAddRef( pUnitData->nIconTextureId );
			}
		}

		// appearance
		if(pDataToLoad->bLoadAppearance && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_LOADED_APPEARANCE))
		{
			APPEARANCE_DEFINITION * pAppearanceDef = NULL;
			if ( pUnitData->nAppearanceDefId != INVALID_ID )
			{
				pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId );
				EVENT_DATA eventData(eTable, nIndex);
				DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId, c_sUnitDataLoadGfx, &eventData);
				DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId, c_sUnitDataLoadAppearanceSounds, &eventData);
			}

			APPEARANCE_DEFINITION * pAppearanceDefFirst = NULL;
			if ( pUnitData->nAppearanceDefId_FirstPerson != INVALID_ID )
			{
				pAppearanceDefFirst = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId_FirstPerson );
				EVENT_DATA eventData(eTable, nIndex);
				DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId_FirstPerson, c_sUnitDataLoadGfx, &eventData);
				DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId_FirstPerson, c_sUnitDataLoadAppearanceSounds, &eventData);
			}
			
			SETBIT(pUnitData->pdwFlags, UNIT_DATA_FLAG_LOADED_APPEARANCE, TRUE);
		}

		// particles
		if(pDataToLoad->bLoadSkills && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_LOADED_SKILLS))
		{
			if ( bPreload )
			{
				for (int ii = 0; ii < NUM_UNIT_COMBAT_RESULTS; ii++)
				{
					c_ParticleSystemLoad( pUnitData->nBloodParticleId[ii] ); 
				}
				for (int ii = 0; ii < NUM_MISSILE_EFFECTS; ii++)
				{
					const PARTICLE_EFFECT_SET *pEffectSet = &pUnitData->MissileEffects[ ii ];
					c_ParticleSystemLoad( pEffectSet->nDefaultId );

					for (int jj = 0; jj < NUM_DAMAGE_TYPES; jj++)
					{
						const PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffectSet->tElementEffect[ jj ];
						c_ParticleSystemLoad( pElementEffect->nPerElementId );
					}
				}
				c_ParticleSystemLoad( pUnitData->nHolyRadiusId ); 
				c_ParticleSystemLoad( pUnitData->nFizzleParticleId ); 
				c_ParticleSystemLoad( pUnitData->nReflectParticleId ); 
				c_ParticleSystemLoad( pUnitData->nRestoreVitalsParticleId ); 
				c_ParticleSystemLoad( pUnitData->nDamagingMeleeParticleId ); 

			}

			// skills and states
			if ( bPreload )
			{
				for ( int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++ )
				{
					if ( pUnitData->nStartingSkills[ ii ] == INVALID_ID )
						continue;
					c_SkillLoad( pGame, pUnitData->nStartingSkills[ ii ] );

				}
				c_SkillLoad( pGame, pUnitData->nSkillIdHitUnit			);
				c_SkillLoad( pGame, pUnitData->nSkillIdHitBackground	);
				for( int t = 0; t < NUM_SKILL_MISSED; t++ )
				{
					c_SkillLoad( pGame, pUnitData->nSkillIdMissedArray[t] );
				}
				c_SkillLoad( pGame, pUnitData->nSkillIdOnFuse			);
				c_SkillLoad( pGame, pUnitData->nUnitDieBeginSkill		);
				c_SkillLoad( pGame, pUnitData->nUnitDieBeginSkillClient	);
				c_SkillLoad( pGame, pUnitData->nUnitDieEndSkill			);
				c_SkillLoad( pGame, pUnitData->nDeadSkill				);
				for(int i=0; i<NUM_INIT_SKILLS; i++)
				{
					c_SkillLoad( pGame, pUnitData->nInitSkill[i]		);
				}
				c_SkillLoad( pGame, pUnitData->nSkillWhenUsed			);
				c_SkillLoad( pGame, pUnitData->nKickSkill				);
				c_SkillLoad( pGame, pUnitData->nRightSkill				);
				c_SkillLoad( pGame, pUnitData->nPaperdollSkill			);
				c_SkillLoad( pGame, pUnitData->nSkillGhost				);
				c_SkillLoad( pGame, pUnitData->nSkillRef				);

				// states
				c_StateLoad(pGame, pUnitData->nStateSafe);
				for(int i=0; i<NUM_INIT_STATES; i++)
				{
					c_StateLoad(pGame, pUnitData->pnInitStates[i]);
				}
				c_StateLoad(pGame, pUnitData->nDyingState);
				c_StateLoad(pGame, pUnitData->m_nLevelUpState);
				c_StateLoad(pGame, pUnitData->m_nRankUpState);
			}

			STATS * pStats = ExcelGetStats(pGame, eTable, nIndex);
			if(bPreload && pStats)
			{
				int nAI = StatsGetStat(pStats, STATS_AI);
				if(nAI >= 0)
				{
					c_AIDataLoad(pGame, nAI);
				}
			}

			if(bPreload && pUnitData->nMeleeWeapon >= 0)
			{
				MELEE_WEAPON_DATA * pMeleeWeaponData = (MELEE_WEAPON_DATA *) ExcelGetData( pGame, DATATABLE_MELEEWEAPONS, pUnitData->nMeleeWeapon );
				if(pMeleeWeaponData)
				{
					c_SkillEventsLoad(pGame, pMeleeWeaponData->nSwingEventsId);
					for (int ii = 0; ii < NUM_COMBAT_RESULTS; ii++)
					{
						PARTICLE_EFFECT_SET *pEffectSet = &pMeleeWeaponData->pEffects[ ii ];
						c_InitEffectSet(pEffectSet, pMeleeWeaponData->szEffectFolder, pMeleeWeaponData->szName);
						c_ParticleSystemLoad( pEffectSet->nDefaultId );

						for (int jj = 0; jj < NUM_DAMAGE_TYPES; jj++)
						{
							const PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffectSet->tElementEffect[ jj ];
							if(pElementEffect && pElementEffect->szPerElement && pElementEffect->szPerElement[0] != '\0')
							{
								c_ParticleSystemLoad( pElementEffect->nPerElementId );
							}
						}
					}
				}
			}

			if ( pUnitData->nMissileHitUnit != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MISSILES, pUnitData->nMissileHitUnit );
			}
			if ( pUnitData->nMissileHitBackground != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MISSILES, pUnitData->nMissileHitBackground );
			}
			if ( pUnitData->nFieldMissile != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MISSILES, pUnitData->nFieldMissile );
			}
			if ( pUnitData->nMonsterToSpawn != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MONSTERS, pUnitData->nMonsterToSpawn );
			}
			if ( pUnitData->nSpawnMonsterClass != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MONSTERS, pUnitData->nSpawnMonsterClass );
			}
			if ( pUnitData->nMonsterDecoy != INVALID_ID )
			{
				UnitDataLoad( pGame, DATATABLE_MONSTERS, pUnitData->nMonsterDecoy );
			}

			SETBIT(pUnitData->pdwFlags, UNIT_DATA_FLAG_LOADED_SKILLS, TRUE);
		}

		// sounds
		if(pDataToLoad->bLoadSounds && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_LOADED_SOUNDS))
		{
			if ( bPreload )
			{
				c_SoundLoad(pUnitData->m_nSound);
				c_SoundLoad(pUnitData->m_nFlippySound);
				c_SoundLoad(pUnitData->m_nInvPickupSound);
				c_SoundLoad(pUnitData->m_nInvPutdownSound);
				c_SoundLoad(pUnitData->m_nInvEquipSound);
				c_SoundLoad(pUnitData->m_nPickupSound);
				c_SoundLoad(pUnitData->m_nInvUseSound);
				c_SoundLoad(pUnitData->m_nInvCantUseSound);
				c_SoundLoad(pUnitData->m_nCantFireSound);
				c_SoundLoad(pUnitData->m_nBlockSound);
				c_SoundLoad(pUnitData->nInteractSound);
				c_SoundLoad(pUnitData->nDamagingSound);

				// These are Mythos specific sounds - but maybe Hellgate wants them too?
				c_SoundLoad(pUnitData->m_nOutOfManaSound);
				c_SoundLoad(pUnitData->m_nInventoryFullSound);
				c_SoundLoad(pUnitData->m_nCantSound);
				c_SoundLoad(pUnitData->m_nCantUseSound);
				c_SoundLoad(pUnitData->m_nCantUseYetSound);
				c_SoundLoad(pUnitData->m_nCantCastSound);
				c_SoundLoad(pUnitData->m_nCantCastYetSound);
				c_SoundLoad(pUnitData->m_nCantCastHereSound);
				c_SoundLoad(pUnitData->m_nLockedSound);
				c_SoundLoad(pUnitData->m_nTriggerSound);

				for(int i=0; i<UNIT_HIT_SOUND_COUNT; i++)
				{
					c_SoundLoad(pUnitData->pnGetHitSounds[i]);
				}

				if(pUnitData->nFootstepBackwardLeft >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepBackwardLeft);
				}
				if(pUnitData->nFootstepBackwardRight >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepBackwardRight);
				}
				if(pUnitData->nFootstepForwardLeft >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepForwardLeft);
				}
				if(pUnitData->nFootstepForwardRight >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepForwardRight);
				}
				if(pUnitData->nFootstepJump1stPerson >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepJump1stPerson);
				}
				if(pUnitData->nFootstepLand1stPerson >= 0)
				{
					c_FootstepLoad(pUnitData->nFootstepLand1stPerson);
				}
			}

			SETBIT(pUnitData->pdwFlags, UNIT_DATA_FLAG_LOADED_SOUNDS, TRUE);
		}
#endif// !ISVERSION(SERVER_VERSION)
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sParticleEffectSetFlagSoundsForLoad(
	const PARTICLE_EFFECT_SET *pEffectSet,
	BOOL bLoadNow)
{
	ASSERT_RETURN(pEffectSet);
	c_ParticleSystemFlagSoundsForLoad( pEffectSet->nDefaultId, bLoadNow );

	for (int jj = 0; jj < NUM_DAMAGE_TYPES; jj++)
	{
		const PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffectSet->tElementEffect[ jj ];
		c_ParticleSystemFlagSoundsForLoad( pElementEffect->nPerElementId, bLoadNow );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitDataFlagSoundsForLoad(
	GAME * pGame,
	EXCELTABLE eTable,
	int nIndex,
	BOOL bLoadNow)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(IS_CLIENT(pGame));

	UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetData( NULL, eTable, nIndex ); 
	if ( ! pUnitData )
		return;

	if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_SOUNDS_FLAGGED) )
		return;
	SETBIT(pUnitData->pdwFlags, UNIT_DATA_FLAG_SOUNDS_FLAGGED, TRUE);

	// appearance
	APPEARANCE_DEFINITION * pAppearanceDef = NULL;
	if ( pUnitData->nAppearanceDefId != INVALID_ID )
	{
		pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId );
		if(pAppearanceDef)
		{
			c_AppearanceDefinitionFlagSoundsForLoad(pAppearanceDef, bLoadNow);
		}
		else
		{
			EVENT_DATA eventData(eTable, nIndex);
			DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId, c_sUnitDataLoadAppearanceSounds, &eventData);
		}
	}

	APPEARANCE_DEFINITION * pAppearanceDefFirst = NULL;
	if ( pUnitData->nAppearanceDefId_FirstPerson != INVALID_ID )
	{
		pAppearanceDefFirst = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId_FirstPerson );
		if(pAppearanceDefFirst)
		{
			c_AppearanceDefinitionFlagSoundsForLoad(pAppearanceDefFirst, bLoadNow);
		}
		else
		{
			EVENT_DATA eventData(eTable, nIndex);
			DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, pUnitData->nAppearanceDefId_FirstPerson, c_sUnitDataLoadAppearanceSounds, &eventData);
		}
	}

	// particles
	for (int ii = 0; ii < NUM_UNIT_COMBAT_RESULTS; ii++)
	{
		c_ParticleSystemFlagSoundsForLoad( pUnitData->nBloodParticleId[ii], bLoadNow );
	}
	for (int ii = 0; ii < NUM_MISSILE_EFFECTS; ii++)
	{
		const PARTICLE_EFFECT_SET *pEffectSet = &pUnitData->MissileEffects[ ii ];
		if(!pEffectSet)
			continue;
		c_sParticleEffectSetFlagSoundsForLoad(pEffectSet, bLoadNow);
	}
	c_ParticleSystemFlagSoundsForLoad( pUnitData->nHolyRadiusId, bLoadNow );
	c_ParticleSystemFlagSoundsForLoad( pUnitData->nFizzleParticleId, bLoadNow );
	c_ParticleSystemFlagSoundsForLoad( pUnitData->nReflectParticleId, bLoadNow );
	c_ParticleSystemFlagSoundsForLoad( pUnitData->nRestoreVitalsParticleId, bLoadNow );
	c_ParticleSystemFlagSoundsForLoad( pUnitData->nDamagingMeleeParticleId, bLoadNow );

	// skills and states
	for ( int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++ )
	{
		if ( pUnitData->nStartingSkills[ ii ] == INVALID_ID )
			continue;
		c_SkillFlagSoundsForLoad( pGame, pUnitData->nStartingSkills[ ii ], bLoadNow );

	}
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillIdHitUnit, bLoadNow			);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillIdHitBackground, bLoadNow		);
	for( int t = 0; t < NUM_SKILL_MISSED; t++ )
	{
		c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillIdMissedArray[t], bLoadNow);
	}
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillIdOnFuse, bLoadNow			);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nUnitDieBeginSkill, bLoadNow		);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nUnitDieBeginSkillClient, bLoadNow	);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nUnitDieEndSkill, bLoadNow			);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nDeadSkill, bLoadNow				);
	for(int i=0; i<NUM_INIT_SKILLS; i++)
	{
		c_SkillFlagSoundsForLoad( pGame, pUnitData->nInitSkill[i], bLoadNow			);
	}
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nKickSkill, bLoadNow				);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nRightSkill, bLoadNow				);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillWhenUsed, bLoadNow			);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nPaperdollSkill, bLoadNow			);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillGhost, bLoadNow				);
	c_SkillFlagSoundsForLoad( pGame, pUnitData->nSkillRef, bLoadNow					);

	STATS * pStats = ExcelGetStats(pGame, eTable, nIndex);
	if(pStats)
	{
		int nAI = StatsGetStat(pStats, STATS_AI);
		if(nAI >= 0)
		{
			c_AIFlagSoundsForLoad(pGame, nAI, bLoadNow);
		}
	}

	if(pUnitData->nMeleeWeapon >= 0)
	{
		const MELEE_WEAPON_DATA * pMeleeWeaponData = MeleeWeaponGetData(pGame, pUnitData->nMeleeWeapon);
		if(pMeleeWeaponData)
		{
			if(pMeleeWeaponData->nSwingEventsId >= 0)
			{
				SKILL_EVENTS_DEFINITION * pWeaponEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, pMeleeWeaponData->nSwingEventsId );
				if(pWeaponEvents)
				{
					c_SkillEventsFlagSoundsForLoad(pGame, pWeaponEvents, bLoadNow);
				}
			}
			for (int ii = 0; ii < NUM_COMBAT_RESULTS; ii++)
			{
				const PARTICLE_EFFECT_SET *pEffectSet = &pMeleeWeaponData->pEffects[ii];
				if(!pEffectSet)
					continue;
				c_sParticleEffectSetFlagSoundsForLoad(pEffectSet, bLoadNow);
			}
		}
	}

	if ( pUnitData->nMissileHitUnit != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MISSILES, pUnitData->nMissileHitUnit, bLoadNow );
	}
	if ( pUnitData->nMissileHitBackground != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MISSILES, pUnitData->nMissileHitBackground, bLoadNow );
	}
	if ( pUnitData->nFieldMissile != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MISSILES, pUnitData->nFieldMissile, bLoadNow );
	}
	if ( pUnitData->nMonsterToSpawn != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MONSTERS, pUnitData->nMonsterToSpawn, bLoadNow );
	}
	if ( pUnitData->nSpawnMonsterClass != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MONSTERS, pUnitData->nSpawnMonsterClass, bLoadNow );
	}
	if ( pUnitData->nMonsterDecoy != INVALID_ID )
	{
		c_UnitDataFlagSoundsForLoad( pGame, DATATABLE_MONSTERS, pUnitData->nMonsterDecoy, bLoadNow );
	}

	// states
	if(pUnitData->nStateSafe >= 0)
	{
		c_StateFlagSoundsForLoad(pGame, pUnitData->nStateSafe, bLoadNow);
	}
	for(int i=0; i<NUM_INIT_STATES; i++)
	{
		if(pUnitData->pnInitStates[i] >= 0)
		{
			c_StateFlagSoundsForLoad(pGame, pUnitData->pnInitStates[i], bLoadNow);
		}
	}
	if(pUnitData->nDyingState >= 0)
	{
		c_StateFlagSoundsForLoad(pGame, pUnitData->nDyingState, bLoadNow);
	}
	if(pUnitData->m_nLevelUpState >= 0)
	{
		c_StateFlagSoundsForLoad(pGame, pUnitData->m_nLevelUpState, bLoadNow);
	}
	if(pUnitData->m_nRankUpState >= 0)
	{
		c_StateFlagSoundsForLoad(pGame, pUnitData->m_nRankUpState, bLoadNow);
	}

	// NPC info
	if(pUnitData->nNPC >= 0)
	{
		c_NPCFlagSoundsForLoad(pGame, pUnitData->nNPC, bLoadNow);
	}

	// sounds
	c_SoundFlagForLoad(pUnitData->m_nSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nFlippySound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInvPickupSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInvPutdownSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInvEquipSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nPickupSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInvUseSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInvCantUseSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantFireSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nBlockSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->nInteractSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->nDamagingSound, bLoadNow);

	// These are Mythos specific sounds - but maybe Hellgate wants them too?
	c_SoundFlagForLoad(pUnitData->m_nOutOfManaSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nInventoryFullSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantUseSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantUseYetSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantCastSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantCastYetSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nCantCastHereSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nLockedSound, bLoadNow);
	c_SoundFlagForLoad(pUnitData->m_nTriggerSound, bLoadNow);

	for(int i=0; i<UNIT_HIT_SOUND_COUNT; i++)
	{
		c_SoundFlagForLoad(pUnitData->pnGetHitSounds[i], bLoadNow);
	}

	if(pUnitData->nFootstepBackwardLeft >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepBackwardLeft, bLoadNow);
	}
	if(pUnitData->nFootstepBackwardRight >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepBackwardRight, bLoadNow);
	}
	if(pUnitData->nFootstepForwardLeft >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepForwardLeft, bLoadNow);
	}
	if(pUnitData->nFootstepForwardRight >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepForwardRight, bLoadNow);
	}
	if(pUnitData->nFootstepJump1stPerson >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepJump1stPerson, bLoadNow);
	}
	if(pUnitData->nFootstepLand1stPerson >= 0)
	{
		c_FootstepFlagForLoad(pUnitData->nFootstepLand1stPerson, bLoadNow);
	}

#endif// !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitDataAlwaysKnownFlagSoundsForLoad(
	GAME * pGame)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(IS_CLIENT(pGame));

	for(int i=0; i<NUM_GENUS; i++)
	{
		EXCELTABLE eTable = UnitGenusGetDatatableEnum((GENUS)i);
		if(eTable < NUM_DATATABLES)
		{
			int nTableCount = ExcelGetNumRows(EXCEL_CONTEXT(pGame), eTable);
			for(int j=0; j<nTableCount; j++)
			{
				UNIT_DATA * pUnitData = (UNIT_DATA*)ExcelGetData(EXCEL_CONTEXT(pGame), eTable, j);
				if(pUnitData && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ALWAYS_KNOWN_FOR_SOUNDS))
				{
					c_UnitDataFlagSoundsForLoad( pGame, eTable, j, FALSE );
				}
			}
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitDataAllUnflagSoundsForLoad(
	GAME * pGame)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(IS_CLIENT(pGame));

	for(int i=0; i<NUM_GENUS; i++)
	{
		EXCELTABLE eTable = UnitGenusGetDatatableEnum((GENUS)i);
		if(eTable < NUM_DATATABLES)
		{
			int nTableCount = ExcelGetNumRows(EXCEL_CONTEXT(pGame), eTable);
			for(int j=0; j<nTableCount; j++)
			{
				UNIT_DATA * pUnitData = (UNIT_DATA*)ExcelGetData(EXCEL_CONTEXT(pGame), eTable, j);
				if(pUnitData)
				{
					SETBIT(pUnitData->pdwFlags, UNIT_DATA_FLAG_SOUNDS_FLAGGED, FALSE);
				}
			}
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitFlagSoundsForLoad(
	UNIT * pUnit)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pUnit);

	GAME * pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	EXCELTABLE eTable = UnitGenusGetDatatableEnum(UnitGetGenus(pUnit));
	// Unit data
	c_UnitDataFlagSoundsForLoad(pGame, eTable, UnitGetClass(pUnit), FALSE);

	// inventory
	if(UnitHasInventory(pUnit))
	{
		UNIT * pItem = UnitInventoryIterate(pUnit, NULL);
		while(pItem)
		{
			c_UnitFlagSoundsForLoad(pItem);
			pItem = UnitInventoryIterate(pUnit, pItem);
		}
	}

	// skills
	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = pStatsEntry[jj].GetParam();
		c_SkillFlagSoundsForLoad( pGame, nSkill, FALSE);
	}
	UNIT_ITERATE_STATS_END;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitKnockback( 
	GAME * pGame, 
	UNIT * pAttacker,
	UNIT * pDefender, 
	float fDistance )
{
#if 0  // knockback has been disabled since it causes so many sync issues
	if ( UnitGetGenus( pDefender ) == GENUS_PLAYER )
		return FALSE; // this has issues

	const UNITMODE_DATA * pModeData = (const UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, MODE_KNOCKBACK );
	int nState = pModeData->nClearStateEnd;
	ASSERT_RETFALSE( nState != INVALID_ID );

	if ( UnitHasState( pGame, pDefender, nState ) )
		return FALSE;

	BOOL bHasMode = FALSE;
	float fDuration = UnitGetModeDuration( pGame, pDefender, MODE_KNOCKBACK, bHasMode );
	if ( ! bHasMode )
	{
		fDuration = UnitGetModeDuration( pGame, pDefender, MODE_GETHIT, bHasMode );
		if(!bHasMode)
		{
			fDuration = UnitGetModeDuration( pGame, pDefender, MODE_GETHIT_NO_INTERRUPT, bHasMode );
			if(!bHasMode)
				return FALSE;
		}
	}

	UNITMODE eMode = MODE_KNOCKBACK;
	float fVelocity = 0.0f;
	if ( fDistance <= 0.0f )
	{
		eMode = MODE_GETHIT; // we want to at least place them in gethit mode
		UnitFaceUnit( pDefender, pAttacker );
	}
	else
	{
		fVelocity = UnitGetVelocityForMode(pDefender, MODE_KNOCKBACK);
		if (fVelocity == 0.0f)
			eMode = MODE_GETHIT;
	}

	if ( UnitHasState( pGame, pDefender, STATE_INVULNERABLE_KNOCKBACK ) )
	{
		fVelocity = 0.0f;
		eMode = MODE_GETHIT;
	}

	if( ! s_TryUnitSetMode(pDefender, eMode) )
		return FALSE;

	if ( eMode == MODE_GETHIT )
	{
		float fModeDuration = fVelocity ? fDistance / fVelocity : fDuration;
		s_UnitSetMode( pDefender, eMode, 0, fModeDuration, 0, TRUE );
		UnitJumpCancel(pDefender);
		return FALSE;
	}

	UnitSetFlag(pDefender, UNITFLAG_TEMPORARILY_DISABLE_PATHING);

	ASSERT( fVelocity != 0.0f );

	s_StateSet( pDefender, pAttacker, nState, 0 );

	VECTOR vDefenderPosition = UnitGetPosition( pDefender );
	VECTOR vDirection;
	if ( UnitTestFlag( pAttacker, UNITFLAG_KNOCKSBACK_WITH_MOVEMENT ) )
	{
		vDirection = UnitGetMoveDirection( pAttacker );
	} else {
		vDirection = vDefenderPosition - UnitGetPosition( pAttacker );
	}
	vDirection.fZ = 0.0f;
	VectorNormalize( vDirection );

	BYTE bFlags = 0;
	if ( ! TESTBIT( pDefender->pdwMovementFlags, MOVEFLAG_FLY ) )
	{ 
		UnitSetOnGround( pDefender, FALSE );
		bFlags |= MOVE_FLAG_NOT_ONGROUND;	

		UnitSetZVelocity(pDefender, 0.0f);
		bFlags |= MOVE_FLAG_CLEAR_ZVELOCITY;
	}

	VECTOR vDelta = vDirection;
	vDelta *= fDistance;
	VECTOR vTarget = vDefenderPosition + vDelta;
	ROOM * pDestinationRoom = NULL;
	ROOM * pSourceRoom = RoomGetFromPosition(pGame, UnitGetRoom(pDefender), &vTarget);
	if(!pSourceRoom)
	{
		pSourceRoom = UnitGetRoom(pDefender);
	}
	if(!pSourceRoom)
	{
		return FALSE;
	}
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode(pGame, pSourceRoom, vTarget, &pDestinationRoom);
	if(!pDestinationRoom || !pPathNode)
	{
		return FALSE;
	}
	vTarget = RoomPathNodeGetWorldPosition(pGame, pDestinationRoom, pPathNode);
	vDirection = vTarget - vDefenderPosition;
	float fMoveDistance = VectorNormalize(vDirection);

	float fModeDuration = fVelocity ? fMoveDistance / fVelocity : fDuration;
	if ( ! s_UnitSetMode( pDefender, eMode, 0, fModeDuration, 0, TRUE ) )
		return FALSE;

	UnitJumpCancel(pDefender);

	UnitSetMoveTarget( pDefender, vTarget, vDirection );

	UnitStepListAdd( pGame, pDefender );

	s_SendUnitMoveXYZ( pDefender, bFlags, MODE_KNOCKBACK, vTarget, vDirection, pAttacker, FALSE );
#endif //#if 0

	return TRUE;
}

//----------------------------------------------------------------------------
void UnitSetDontTarget(
	UNIT* unit,
	BOOL value)
{
	ASSERT_RETURN(unit);
	// we get in wierd client cases if this happens on the client too.
	if( IS_SERVER( unit ) )
	{
		int nDontTargetCount = UnitGetStat(unit, STATS_DONT_TARGET);
		if (value)
		{
			nDontTargetCount++;
		}
		else
		{
			if (nDontTargetCount > 0)
			{
				nDontTargetCount--;
			}
			else
			{
				nDontTargetCount = 0;
			}
		}
		UnitSetStat(unit, STATS_DONT_TARGET, nDontTargetCount);
	}
	
}

//----------------------------------------------------------------------------
void UnitSetDontAttack(
	UNIT *pUnit,
	BOOL bValue)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nDontAttackCount = UnitGetStat( pUnit, STATS_DONT_ATTACK );
	if (bValue)
	{
		nDontAttackCount++;
	}
	else
	{
		if (nDontAttackCount > 0)
		{
			nDontAttackCount--;
		}
		else
		{
			nDontAttackCount = 0;
		}
	}
	UnitSetStat( pUnit, STATS_DONT_ATTACK, nDontAttackCount );
	
}

//----------------------------------------------------------------------------
BOOL UnitGetCanTarget(UNIT* unit)
{
	int nDontTargetCount = UnitGetStat(unit, STATS_DONT_TARGET);
	return (nDontTargetCount <= 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetDontInterrupt(
	UNIT* unit,
	BOOL value)
{
	ASSERT_RETURN(unit);

	int nDontInterruptCount = UnitGetStat(unit, STATS_DONT_INTERRUPT);
	if (value)
	{
		nDontInterruptCount++;
	}
	else
	{
		if (nDontInterruptCount > 0)
		{
			nDontInterruptCount--;
		}
		else
		{
			nDontInterruptCount = 0;
		}
	}
	UnitSetStat(unit, STATS_DONT_INTERRUPT, nDontInterruptCount);
}

//----------------------------------------------------------------------------
BOOL UnitGetCanInterrupt(UNIT* unit)
{
	int nDontInterruptCount = UnitGetStat(unit, STATS_DONT_INTERRUPT);
	return (nDontInterruptCount <= 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetPreventAllSkills(
	UNIT* unit,
	BOOL value)
{
	ASSERT_RETURN(unit);

	int nCount = UnitGetStat(unit, STATS_PREVENT_ALL_SKILLS);
	if (value)
	{
		nCount++;
	}
	else
	{
		if (nCount > 0)
		{
			nCount--;
		}
		else
		{
			nCount = 0;
		}
	}
	UnitSetStat(unit, STATS_PREVENT_ALL_SKILLS, nCount);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetLevelDefinitionIndex( 
	UNIT* pUnit)
{
	ASSERT_RETVAL( pUnit, INVALID_LINK );
	ROOM* pRoom = UnitGetRoom( pUnit );
	ASSERT_RETVAL( pRoom, INVALID_LINK );
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETVAL( pLevel, INVALID_LINK );
	return LevelGetDefinitionIndex( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVELID UnitGetLevelID(
	const UNIT* unit)
{
	LEVEL *pLevel = UnitGetLevel( unit );
	if (pLevel)
	{
		return LevelGetID( pLevel );
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetLevelDepth( 
	UNIT* pUnit)
{
	ASSERT_RETVAL( pUnit, INVALID_LINK );
	ROOM* pRoom = UnitGetRoom( pUnit );
	ASSERT_RETVAL( pRoom, INVALID_LINK );
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETVAL( pLevel, INVALID_LINK );
	return LevelGetDepth( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetLevelAreaID( 
	UNIT* pUnit)
{
	ASSERT_RETVAL( pUnit, INVALID_LINK );
	ROOM* pRoom = UnitGetRoom( pUnit );
	ASSERT_RETVAL( pRoom, INVALID_LINK );
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETVAL( pLevel, INVALID_LINK );
	return LevelGetLevelAreaID( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetInteractive(
	UNIT *pUnit,
	UNIT_INTERACTIVE eInteractiveState )
{
	if( AppIsHellgate() ) //Tugboat allows for units to become interactive or not on the client
	{
		ASSERT_RETURN( IS_SERVER( pUnit ) );
	}

	UnitSetStat( pUnit, STATS_INTERACTIVE, (int)eInteractiveState );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInteractive(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unitl" );
	
	// first check dynamic flag
	UNIT_INTERACTIVE eInteractive = ( UNIT_INTERACTIVE )UnitGetStat( pUnit, STATS_INTERACTIVE );
	if ( eInteractive == UNIT_INTERACTIVE_RESTRICED )
	{
		return FALSE;
	}
	if ( eInteractive == UNIT_INTERACTIVE_ENABLED )
	{
		return TRUE;
	}

	if (UnitIsPlayer(pUnit))
	{
		return TRUE;
	}

	// dead/dying things are not interactive
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	// check excel props
	if (pUnitData == NULL)
	{
		return FALSE;
	}

	if ( IsUnitDeadOrDying( pUnit ) && ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_SELECTABLE_DEAD) )
	{
		return FALSE;
	}
	
	if( AppIsTugboat() &&
		UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_INTERACTIVE) &&
		UnitsThemeIsVisibleInLevel( pUnit ) == FALSE )
	{
		return FALSE;
	}
	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_INTERACTIVE);
		
}	



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanInteractWith(
	UNIT *pInteractiveUnit,
	UNIT *pOtherUnit)		
{
	BOOL bInteractive = UnitIsInteractive(pInteractiveUnit);
	if (AppIsTugboat())
	{
		if (bInteractive && UnitIsPlayer(pInteractiveUnit))
		{
			// don't interact with hostile players outside of town
			return !(UnitIsPlayer(pInteractiveUnit) && TestHostile(UnitGetGame(pInteractiveUnit), pInteractiveUnit, pOtherUnit));
		}
		return bInteractive;
	}

	if (bInteractive)
	{
		// don't interact with hostile players outside of town
		return !(UnitIsPlayer(pInteractiveUnit) && TestHostile(UnitGetGame(pInteractiveUnit), pInteractiveUnit, pOtherUnit));
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInAttack( UNIT *pUnit )
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// dead/dying things are not interactive
	if (IsUnitDeadOrDying( pUnit ))
	{
		return FALSE;
	}
	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	GAME * game = UnitGetGame(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );
	int nSkillRight = UnitGetStat( pUnit, STATS_SKILL_RIGHT );
	if( nSkill == INVALID_ID &&
		nSkillRight == INVALID_ID )
	{
		return FALSE;
	}
	int nRowCount = SkillsGetCount(game);
	for (int nSkill = 0; nSkill < nRowCount; nSkill++)
	{
		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );

		if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_REQUIRES_SKILL_LEVEL ) ||
			 SkillGetLevel( pUnit, nSkill) > 0 )
		{

			UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );


			for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
			{
				if ( !pWeapons[ i ] )
					continue;
		  
				int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ i ] );
				if ( nWeaponSkill == INVALID_ID )
					continue;
				int nTicksToHold = SkillNeedsToHold( pUnit, pWeapons[ i ], nWeaponSkill );
				if( nTicksToHold > 0 )
				{
					return TRUE;
				}

			}
			int nTicksToHold = SkillNeedsToHold( pUnit, NULL, nSkill, TRUE );
			if( nTicksToHold > 0 )
			{
				return TRUE;
			}
		}		
	}	

	return FALSE;
}	

BOOL UnitIsInAttackLoose( UNIT *pUnit  )
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// dead/dying things are not interactive
	if (IsUnitDeadOrDying( pUnit ))
	{
		return FALSE;
	}
	GAME * game = UnitGetGame(pUnit);
	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );
	int nSkillRight = UnitGetStat( pUnit, STATS_SKILL_RIGHT );
	if( nSkill == INVALID_ID &&
		nSkillRight == INVALID_ID )
	{
		return FALSE;
	}
	
	int nRowCount = SkillsGetCount(game);
	for (int nSkill = 0; nSkill < nRowCount; nSkill++)
	{
		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );

		if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_REQUIRES_SKILL_LEVEL ) ||
			SkillGetLevel( pUnit, nSkill) > 0 )
		{

			UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );


			for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
			{
				if ( !pWeapons[ i ] )
					continue;
		  
				int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ i ] );
				if ( nWeaponSkill == INVALID_ID )
					continue;
				int nTicksToHold = SkillNeedsToHold( pUnit, pWeapons[ i ], nWeaponSkill );
				if( nTicksToHold - 2 > 0 )
				{
					return TRUE;
				}

			}

			int nTicksToHold = SkillNeedsToHold( pUnit, NULL, nSkill, TRUE );
			if( nTicksToHold - 2 > 0 )
			{
				return TRUE;
			}
		}		
	}	

	return FALSE;
}	


inline void UnitGetCaculatedDamageTypeOnWeapon( UNIT *pUnit, UNIT *pWeapon, int nDamageType, int &nMinDamage, int &nMaxDamage )
{
	nMinDamage = nMaxDamage = 0;
 	ASSERTX_RETURN( pUnit, "Expected unit" );	
		
	
	ASSERTX_RETURN( nDamageType >= 0 && nDamageType < NUM_DAMAGE_TYPES, "Expected Valid Damage type" );
	UNIT *pPlayer = ( UnitGetGenus( pUnit ) == GENUS_PLAYER || UnitIsA( pUnit, UNITTYPE_HIRELING ) )?pUnit:UnitGetUltimateOwner( pUnit );
	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );	
	GAME *pGame = UnitGetGame( pPlayer );	
	// dead/dying things are not interactive
	if (IsUnitDeadOrDying( pUnit ) ||
		IS_SERVER( pGame ) ||
		nSkill == INVALID_ID)
	{
		return;
	}			

	int nPercentDamageBonusNoneWeaponsMelee(0);	
	int nPercentDamageBonusNoneWeaponsRanged(0);	
	int EquipedBonusDamage(0);					
	//we have to check everything the player has to see if it mods items
	UNIT * pItemCurr = UnitInventoryIterate( pPlayer, NULL);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pPlayer, pItemCurr );			
		if ( ItemIsEquipped(pItemCurr, pPlayer) )  //we probably will need another check if we had runes to players inventories
		{
			int Type = UnitGetType(pItemCurr);
			//this seems like a terrible way of doing this...					
			if( UnitIsA( pItemCurr, UNITTYPE_WEAPON ) == FALSE || ( pItemCurr == pWeapon && pWeapon != NULL ) )
			{
				EquipedBonusDamage += CombatGetBonusDamage(pGame, pItemCurr, Type, NULL, nDamageType, COMBAT_LOCAL_DELIVERY, TRUE);															
				//nPercentDamageBonusNoneWeaponsMelee += CombatGetBonusDamagePCT( pItemCurr, nDamageType, TRUE );
				//nPercentDamageBonusNoneWeaponsRanged += CombatGetBonusDamagePCT( pItemCurr, nDamageType, FALSE );
			}
		}
		pItemCurr = pItemNext;
	}
	//ok now add all those damages and percent damages up.
	const DAMAGE_TYPE_DATA *pDamageType = (const DAMAGE_TYPE_DATA *)ExcelGetData(UnitGetGame( pUnit ), DATATABLE_DAMAGETYPES, nDamageType);
	REF( pDamageType );
	nPercentDamageBonusNoneWeaponsMelee += UnitGetStat(pUnit, STATS_DAMAGE_PERCENT_MELEE);	
	nPercentDamageBonusNoneWeaponsRanged += UnitGetStat(pUnit, STATS_DAMAGE_PERCENT_RANGED);	
	int nPercentDamage = UnitGetStat( pUnit, STATS_DAMAGE_PERCENT, 0 );				
	if( nDamageType != 0 )
	{		
		nPercentDamageBonusNoneWeaponsMelee += UnitGetStat( pUnit, STATS_DAMAGE_PERCENT, nDamageType ) + nPercentDamage;
		nPercentDamageBonusNoneWeaponsRanged += UnitGetStat( pUnit, STATS_DAMAGE_PERCENT, nDamageType ) + nPercentDamage;
	}
	else
	{
		nPercentDamageBonusNoneWeaponsMelee += nPercentDamage;
		nPercentDamageBonusNoneWeaponsRanged += nPercentDamage;
	}
	
	
	
	BOOL bIsMelee( FALSE );
	//BOOL bIsRanged( FALSE );
	int nWeaponDamageMin(0);
	int nWeaponDamageMax(0);
	if( pWeapon )
	{
		int nWeaponSkill = ItemGetPrimarySkill( pWeapon );			
		if( nWeaponSkill == INVALID_ID )
			return;
		const SKILL_DATA * pSkillData = SkillGetData( NULL, nWeaponSkill );
		bIsMelee = SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_MELEE );
		int nBaseMinDmgPct = UnitGetStat( pWeapon, STATS_DAMAGE_MIN, nDamageType );
		int nBaseMinDamage = UnitGetStatShift(pWeapon, STATS_BASE_DMG_MIN, 0);
		nWeaponDamageMin = PCT( nBaseMinDamage, nBaseMinDmgPct ) + EquipedBonusDamage;	
		int nBaseMaxDmgPct = UnitGetStat( pWeapon, STATS_DAMAGE_MIN, nDamageType );
		int nBaseMaxDamage = UnitGetStatShift( pWeapon, STATS_BASE_DMG_MAX, 0);
		nWeaponDamageMax = PCT( nBaseMaxDamage, nBaseMaxDmgPct ) + EquipedBonusDamage;			
	}
	else
	{
		bIsMelee = TRUE;
		nWeaponDamageMin = EquipedBonusDamage + 1;
		nWeaponDamageMax = EquipedBonusDamage + 1;
	}


	if( bIsMelee )
	{
		nMinDamage = nWeaponDamageMin + PCT(nWeaponDamageMin, nPercentDamageBonusNoneWeaponsMelee );
		nMaxDamage = nWeaponDamageMax + PCT(nWeaponDamageMax, nPercentDamageBonusNoneWeaponsMelee );
	}
	else
	{
		nMinDamage = nWeaponDamageMin + PCT(nWeaponDamageMin, nPercentDamageBonusNoneWeaponsRanged );
		nMaxDamage = nWeaponDamageMax + PCT(nWeaponDamageMax, nPercentDamageBonusNoneWeaponsRanged );
	}
	
}

void UnitCalculateVisibleDamage( UNIT *pUnit)
{
 	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );
	GAME *pGame = UnitGetGame( pUnit );	

	// dead/dying things are not interactive
	if (IsUnitDeadOrDying( pUnit ) ||
		IS_SERVER( pGame ) ||
		nSkill == INVALID_ID )
	{
		return;
	}
		

	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );		
	UNIT *pPlayer = ( UnitGetGenus( pUnit ) == GENUS_PLAYER || UnitIsA( pUnit, UNITTYPE_HIRELING ) )?pUnit:UnitGetUltimateOwner( pUnit );

	int TotalMinDamageOne = 0;
	int TotalMaxDamageOne = 0;	
	if( pWeapons[ 0 ] != NULL &&
		UnitIsA( pWeapons[ 0 ], UNITTYPE_WEAPON ) )	//right handed weapon
	{
		for (int j = 0; j < NUM_DAMAGE_TYPES; j++)
		{
			int nMinDmg( 0 ), nMaxDmg( 0 );
			UnitGetCaculatedDamageTypeOnWeapon( pPlayer, pWeapons[ 0 ], j, nMinDmg, nMaxDmg );
			TotalMinDamageOne += nMinDmg;
			TotalMaxDamageOne += nMaxDmg;
		}
	}
	int TotalMinDamage = TotalMinDamageOne;
	int TotalMaxDamage = TotalMaxDamageOne;
	
	if( pWeapons[ 1 ] != NULL &&
		UnitIsA( pWeapons[ 1 ], UNITTYPE_WEAPON ) )	//left handed weapon
	{
		int TotalMinDamageTwo = 0;
		int TotalMaxDamageTwo = 0;
		for (int j = 0; j < NUM_DAMAGE_TYPES; j++)
		{
			int nMinDmg( 0 ), nMaxDmg( 0 );
			UnitGetCaculatedDamageTypeOnWeapon( pPlayer, pWeapons[ 1 ], j, nMinDmg, nMaxDmg );
			TotalMinDamageTwo += nMinDmg;
			TotalMaxDamageTwo += nMaxDmg;
		}
		TotalMinDamage = MIN( TotalMinDamageOne, TotalMinDamageTwo );
		TotalMaxDamage = MAX( TotalMaxDamageOne, TotalMaxDamageTwo );
	}

	if( pWeapons[ 0 ] == NULL &&
		pWeapons[ 1 ] == NULL )
	{
		UnitGetCaculatedDamageTypeOnWeapon( pPlayer, NULL, 0, TotalMinDamage, TotalMaxDamage );
	}

	TotalMaxDamage = MAX(TotalMaxDamage, 1);
	UnitSetStat( pUnit, STATS_VISIBLE_DAMAGE_MIN, TotalMinDamage );
	UnitSetStat( pUnit, STATS_VISIBLE_DAMAGE_MAX, TotalMaxDamage );


}	


void UnitCalculatePVPGearDrop( UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );	

	// dead/dying things are not interactive
	if (IsUnitDeadOrDying( pUnit ) )
	{
		return;
	}

	int nChance = InventoryCalculatePVPGearDrop( pGame, pUnit );

	UnitSetStat( pUnit, STATS_GEAR_DROP_CHANCE, nChance);

}	

BOOL UnitIsDualWielding(
						 UNIT *pUnit, 
						 int nSkill )
{ 
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
 /*
	if( UnitGetWardrobe( pUnit ) == NULL )
	{
		return FALSE;
	}
	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );
	if( nSkill == INVALID_ID )
	{
		return FALSE;
	}*/
	if( nSkill == INVALID_ID )
	{
		return FALSE;
	}
	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

	int Weapons = 0;
	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		if ( pWeapons[ i ] )
			Weapons++;

	}

	/*nSkill = UnitGetStat( pUnit, STATS_SKILL_RIGHT );
	UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

	for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
	{
		if ( pWeapons[ i ] )
			Weapons++;

	}*/
	return Weapons > 1;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsArmed( UNIT *pUnit )
{ 
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	int nWardrobe = UnitGetWardrobe( pUnit );
	if( nWardrobe == INVALID_ID )
	{
		return FALSE;
	}
	GAME *pGame = UnitGetGame( pUnit );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		UNIT *  pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );

		if ( ! pWeapon || UnitIsA( pWeapon, UNITTYPE_SHIELD ) )
		{
			continue;
		}
		
		return TRUE;
		
	}
	
	return FALSE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetFactions( 
	UNIT *pUnit, 
	int *pnFactions)
{
	int nCount = 0;
	
	UNIT_ITERATE_STATS_RANGE(pUnit, STATS_FACTION_SCORE, tStatsList, ii, FACTION_MAX_FACTIONS);
		int nFactionScore = tStatsList[ ii ].value;
		int nFaction = tStatsList[ ii ].GetParam();
		if (nFactionScore != FACTION_SCORE_NONE)
		{
			pnFactions[ nCount++ ] = nFaction;
		}
	UNIT_ITERATE_STATS_END;
	
	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetPrimaryFaction( 
	UNIT *pUnit)
{
	int nFactionPrimary = INVALID_LINK;
	int nScorePrimary = 0;

	// a "primary" faction is the faction with the highest score	
	int nFactions[ FACTION_MAX_FACTIONS ];
	int nFactionCount = UnitGetFactions( pUnit, nFactions );
	for (int i = 0; i < nFactionCount; ++i)
	{
		int nFaction = nFactions[ i ];
		int nScore = FactionScoreGet( pUnit, nFaction );
		
		if (nScore > nScorePrimary)
		{
			nScorePrimary = nScore;
			nFactionPrimary = nFaction;
		}
		
	}
		
	return nFactionPrimary;
	
}

//----------------------------------------------------------------------------
//
//


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInvulnerable(
	UNIT *pUnit,
	int nDamageType)
{	
	ASSERTX_RETTRUE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// check all invulnerability
	if (GameGetDebugFlag( pGame, DEBUGFLAG_ALL_INVULNERABLE ))
	{
		return TRUE;
	}	
	
	// check player invulnerability
	if (GameGetDebugFlag( pGame, DEBUGFLAG_PLAYER_INVULNERABLE ) && UnitIsA( pUnit, UNITTYPE_PLAYER ))
	{
		return TRUE;
	}

	int num_damage_types = GetNumDamageTypes(pGame);
	if (nDamageType >= 0 && nDamageType < num_damage_types)
	{
		const DAMAGE_TYPE_DATA * pDamageTypeData = DamageTypeGetData(pGame, nDamageType);
		if(pDamageTypeData && UnitHasState(pGame, pUnit, pDamageTypeData->nInvulnerableState))
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetExtraDyingTicks(
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return (int)(pUnitData->flExtraDyingTimeInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitAlwaysShowsLabel(
	UNIT *pUnit)
{
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ALWAYS_SHOW_LABEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanHoldMoreMoney(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	cCurrency currency;
	return UnitGetCurrency( pUnit ).CanCarry( UnitGetGame( pUnit ), currency );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void SetMonitaryValueByStat( UNIT *pUnit, STATS_TYPE stat, int nValue )
{
	ASSERTX_RETURN( pUnit, "Expected unit" );	
	
	// keep within range
	GAME * game = UnitGetGame(pUnit);
	const STATS_DATA * stats_data = StatsGetData(game, stat);
	int nMaxAmount = stats_data->m_nMaxSet;	
	nValue = PIN( nValue, 0, nMaxAmount );
	// set new stat	
	UnitSetStat( pUnit, stat, 0, nValue );	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAddCurrency(
	UNIT *pUnit,
	cCurrency currency)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nGoldAmount = UnitGetStat( pUnit, STATS_GOLD ) + currency.GetValue( KCURRENCY_VALUE_INGAME );
	int nCreditsAmount = UnitGetStat( pUnit, STATS_MONEY_REALWORLD ) + currency.GetValue( KCURRENCY_VALUE_REALWORLD );
	int nPVPAmount = UnitGetStat( pUnit, STATS_PVP_POINTS ) + currency.GetValue( KCURRENCY_VALUE_PVPPOINTS );
	SetMonitaryValueByStat( pUnit, STATS_GOLD, nGoldAmount );
	SetMonitaryValueByStat( pUnit, STATS_MONEY_REALWORLD, nCreditsAmount );	
	SetMonitaryValueByStat( pUnit, STATS_PVP_POINTS, nPVPAmount );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitRemoveCurrencyUnsafe(
	UNIT *pUnit,
	cCurrency currencyToRemove )
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	int nGoldAmount = UnitGetStat( pUnit, STATS_GOLD ) - currencyToRemove.GetValue( KCURRENCY_VALUE_INGAME );
	int nCreditsAmount = UnitGetStat( pUnit, STATS_MONEY_REALWORLD ) - currencyToRemove.GetValue( KCURRENCY_VALUE_REALWORLD );
	int nPVPAmount = UnitGetStat( pUnit, STATS_PVP_POINTS ) - currencyToRemove.GetValue( KCURRENCY_VALUE_PVPPOINTS );
	SetMonitaryValueByStat( pUnit, STATS_GOLD, nGoldAmount );
	SetMonitaryValueByStat( pUnit, STATS_MONEY_REALWORLD, nCreditsAmount );
	SetMonitaryValueByStat( pUnit, STATS_PVP_POINTS, nPVPAmount );	
}

//----------------------------------------------------------------------------
// Validate we have enough money, and the cost is non-negative.
//----------------------------------------------------------------------------
BOOL UnitRemoveCurrencyValidated(
	UNIT *pUnit,
	cCurrency currencyToRemove )
{
	ASSERTX_RETFALSE(pUnit, "No unit");
	ASSERTV_RETFALSE(!currencyToRemove.IsNegative(), "Negative cost %d %d",
		currencyToRemove.GetValue( KCURRENCY_VALUE_INGAME ),
		currencyToRemove.GetValue( KCURRENCY_VALUE_REALWORLD ));

	cCurrency playerCurrency = UnitGetCurrency( pUnit );
	ASSERTX_RETFALSE(!(playerCurrency < currencyToRemove),
		"Insufficient funds");

	UnitRemoveCurrencyUnsafe(pUnit, currencyToRemove);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
BOOL UnitSetMoney(
	UNIT *pUnit,
	int nMoney)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	int nOldMoney = UnitGetMoney( pUnit );
	
	// keep within range
	int nMaxMoney = UnitGetMaxMoney( pUnit );
	nMoney = PIN( nMoney, 0, nMaxMoney );

	// set new stat	
	UnitSetStat( pUnit, STATS_GOLD, 0, nMoney );	

	// return TRUE if the money value has changed
	return UnitGetMoney( pUnit ) != nOldMoney;	
	
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
int UnitGetMoney(
	UNIT * unit)
{
	ASSERT_RETZERO(unit);
	return UnitGetStat(unit, STATS_GOLD);
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
int UnitGetMaxMoney(
	UNIT * unit)
{
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	const STATS_DATA * stats_data = StatsGetData(game, STATS_GOLD);
	return stats_data->m_nMaxSet;
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sUnitBecameUntargetableHandler(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pEventData,
	void* pData,
	DWORD dwId )
{
	UNITID idAttacker = (UNITID)pEventData->m_Data1;
	UNIT * pAttacker = UnitGetById(pGame, idAttacker);
	if(!pAttacker)
		return;

	UnitPathAbort(pAttacker);
	UnitSetMoveTarget(pAttacker, INVALID_ID);
	UnitSetAITarget(pAttacker, INVALID_ID, FALSE, TRUE);
	UnitSetAITarget(pAttacker, INVALID_ID, TRUE);
	UnitClearStepModes(pAttacker);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetMoveTarget(
	UNIT* unit,
	UNITID idTarget)
{
	unit->idMoveTarget = idTarget;
	unit->vMoveDirection.fX = unit->vMoveDirection.fY = unit->vMoveDirection.fZ = 0.0f;
	unit->vMoveTarget.fX = unit->vMoveTarget.fY = unit->vMoveTarget.fZ = 0.0f;

#ifdef TRACE_SYNCH
	{
		trace("UnitSetMoveTarget()[%c]: UNIT:" ID_PRINTFIELD "  POS:(%4.4f, %4.4f, %4.4f)  TARGET:" ID_PRINTFIELD "  TIME:%d\n",
			IS_SERVER(unit) ? 'S' : 'C', 
			ID_PRINT(UnitGetId(unit)),
			unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ,
			ID_PRINT(idTarget),
			GameGetTick(UnitGetGame(unit)));
	}
#endif
}

//----------------------------------------------------------------------------
void UnitSetMoveTarget(
	UNIT* unit,
	const VECTOR & vMoveTarget,
	const VECTOR & vMoveDirection)
{
	UnitSetMoveDirection( unit, vMoveDirection );
	unit->idMoveTarget = INVALID_ID;
	unit->vMoveTarget = vMoveTarget;

#ifdef TRACE_SYNCH
	{
		trace("UnitSetMoveTarget()[%c]:  UNIT:" ID_PRINTFIELD "  POS:(%4.4f, %4.4f, %4.4f)  DIR:(%4.4f, %4.4f, %4.4f)  TGT:(%4.4f, %4.4f, %4.4f)  TIME:%d\n",
			IS_SERVER(unit) ? 'S' : 'C', 
			ID_PRINT(UnitGetId(unit)),
			unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ,
			vMoveDirection.fX, vMoveDirection.fY, vMoveDirection.fZ,
			vMoveTarget.fX, vMoveTarget.fY, vMoveTarget.fZ,
			GameGetTick(UnitGetGame(unit)));
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsHealer(
	UNIT* unit)
{
	if (unit)
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_HEALER);
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGravekeeper( 
	UNIT* unit)
{
	
	if (unit)
	{
		const UNIT_DATA * pUnitData = UnitGetData( unit );
		return UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GRAVEKEEPER);		
	}
	return FALSE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetLabelScale( 
	const UNIT *pUnit)
{
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return pUnitData->flLabelScale;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRestorePetVitals(
	UNIT *pPet,
	void *pUserData )
{
	ASSERT_RETURN(pPet);
	const VITALS_SPEC *pVitals = (const VITALS_SPEC *)pUserData;
	if ( ! IsUnitDeadOrDying( pPet ) )
		s_UnitRestoreVitals(pPet, FALSE, pVitals );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VitalsSpecZero(
	VITALS_SPEC &tVitalsSpec)
{
	tVitalsSpec.nHealthAbsolute = 0;
	tVitalsSpec.nHealthPercent = 0;
	tVitalsSpec.nPowerPercent = 0;
	tVitalsSpec.nShieldPercent = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitRestoreVitals(
	UNIT *pUnit,
	BOOL bPlayEffects,
	const VITALS_SPEC *pVitalsSpec /*= NULL*/)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// server only
	if (IS_CLIENT( pGame ))
	{
		return;
	}

	// we are in some way restoring vitals and coming back to life, we are no longer
	// waiting for respawninput from a client who has died
	UnitClearStat( pUnit, STATS_PLAYER_WAITING_TO_RESPAWN, 0 );

	// if no vitals spec provided, use our default one
	VITALS_SPEC tVitalsDefault;
	const VITALS_SPEC *pVitals = NULL;
	if (pVitalsSpec)
	{
		pVitals = pVitalsSpec;
	}
	else
	{
		pVitals = &tVitalsDefault;
	}
		
	// init if we did any healing
	BOOL bDidHealing = FALSE;
	
	// restore health	
	int nHitPointsMax = UnitGetStat( pUnit, STATS_HP_MAX );
	int nHitPointsCurrent = UnitGetStat( pUnit, STATS_HP_CUR );
	int nNewHitPoints = nHitPointsCurrent;
	if (pVitals->nHealthAbsolute != -1)
	{
		nNewHitPoints = pVitals->nHealthAbsolute;
	}
	else
	{
		nNewHitPoints = PCT( nHitPointsMax, pVitals->nPowerPercent );
	}
	if (nHitPointsCurrent != nNewHitPoints)
	{
		UnitSetStat( pUnit, STATS_HP_CUR, nNewHitPoints );
		bDidHealing = TRUE;
	}
	
	// restore power
	int nPowerMax = UnitGetStat( pUnit, STATS_POWER_MAX );
	int nPowerCurrent = UnitGetStat( pUnit, STATS_POWER_CUR );
	int nPowerNew = PCT( nPowerMax, pVitals->nPowerPercent );
	if (nPowerCurrent != nPowerNew)
	{
		UnitSetStat( pUnit, STATS_POWER_CUR, nPowerNew );
		bDidHealing = TRUE;
	}

	// restore stamina
	/*if( AppIsTugboat() )
	{
		int nStaminaMax = UnitGetStat( pUnit, STATS_STAMINA_MAX );
		int nStaminaCurrent = UnitGetStat( pUnit, STATS_STAMINA_CUR );
		int nStaminaNew = PCT( nStaminaMax, pVitals->nStaminaPercent );
		if (nStaminaCurrent != nStaminaNew)
		{
			UnitSetStat( pUnit, STATS_STAMINA_CUR, nStaminaNew );
		}
	}*/

	// restore shields
	if ( AppIsHellgate() )
	{
		int nShieldsMax = UnitGetStat( pUnit, STATS_SHIELD_BUFFER_MAX );
		int nShieldsCurrent = UnitGetStat( pUnit, STATS_SHIELD_BUFFER_CUR );
		int nShieldsNew = PCT( nShieldsMax, pVitals->nShieldPercent );
		if (nShieldsCurrent != nShieldsNew)
		{
			UnitSetStat( pUnit, STATS_SHIELD_BUFFER_CUR, nShieldsNew );
			bDidHealing = TRUE;
		}
		
		// restore armor buffer
		int num_damage_types = GetNumDamageTypes(pGame);
		for (int i = 0; i < num_damage_types; i++)
		{
			int nArmorMax = UnitGetStat( pUnit, STATS_ARMOR_BUFFER_MAX, i );
			int nArmorCurrent = UnitGetStat( pUnit, STATS_ARMOR_BUFFER_CUR, i );
			if (nArmorCurrent != nArmorMax)
			{
				UnitSetStat( pUnit, STATS_ARMOR_BUFFER_CUR, i, nArmorMax );
				bDidHealing = TRUE;
			}
		}
	}

	// is there a posibility of breathing bar?
	s_UnitBreathInit( pUnit );

	// clear any "bad" states
	if (s_StateClearBadStates( pUnit ) == TRUE)
	{
		bDidHealing = TRUE;
	}
	
	// do effect (if requested)
	if (bPlayEffects == TRUE && bDidHealing == TRUE)
	{
		int nDurationInTicks = GAME_TICKS_PER_SECOND * 3;
		s_StateSet( pUnit, pUnit, STATE_RESTORE_VITALS, nDurationInTicks );
	}
	
	// restore vitals of pets
	PetIteratePets( pUnit, s_sRestorePetVitals, (void *)pVitals );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENT * UnitGetClient(
	const UNIT * unit)
{
	if (UnitHasClient(unit))
	{
		GAMECLIENTID idClient = UnitGetClientId(unit);
		return ClientGetById(UnitGetGame(unit), idClient);
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetClientID(
	UNIT * unit,
	GAMECLIENTID idClient)
{
	ASSERT_RETURN(unit);
	unit->idClient = idClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APPCLIENTID UnitGetAppClientID(
	const UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
	
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETINVALID( pClient, "Expected client" );
	
	return ClientGetAppId( pClient );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanSendMessages( 
	UNIT *pUnit) 
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// units cannot send message when the no message send flag is on (typically during creation)
	return UnitTestFlag( pUnit, UNITFLAG_NO_MESSAGE_SEND ) == FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetCanSendMessages( 
	UNIT *pUnit, 
	BOOL bCanSend)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// set the flag on this unit
	UnitSetFlag( pUnit, UNITFLAG_NO_MESSAGE_SEND, !bCanSend );
	
	// set the flag on any contained items
	UNIT *pContained = NULL;
	while ((pContained = UnitInventoryIterate( pUnit, pContained )) != NULL)
	{
		UnitSetCanSendMessages( pContained, bCanSend );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInWorld(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// maybe there is a better way to check this? -Colin
	UNIT *pContainer = UnitGetContainer( pUnit );
	if (pContainer)
	{
		if (UnitIsPhysicallyInContainer( pUnit ) ==	TRUE)
		{
			return FALSE;
		}
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitJump(
	UNIT *pUnit,
	GAMECLIENT *pClient)
{
	MSG_SCMD_UNITJUMP msg;
	msg.id = UnitGetId( pUnit );
	msg.fZVelocity = UnitGetZVelocity( pUnit );
	
	// if no client, send to all with room
	if (pClient == NULL)
	{
		s_SendUnitMessage(pUnit, SCMD_UNITJUMP, &msg);
	}
	else
	{
		GAMECLIENTID idClient = ClientGetId( pClient );
		s_SendUnitMessageToClient(pUnit, idClient, SCMD_UNITJUMP, &msg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetZVelocity(
	UNIT *pUnit,
	float flZVelocity)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	if ( flZVelocity > 0.0f )
	{
		if ( flZVelocity > MAX_ALLOWED_Z_VELOCITY_MAGNITUDE )
			flZVelocity = MAX_ALLOWED_Z_VELOCITY_MAGNITUDE;
	}
	else
	{
		if ( flZVelocity < -MAX_ALLOWED_Z_VELOCITY_MAGNITUDE )
			flZVelocity = -MAX_ALLOWED_Z_VELOCITY_MAGNITUDE;
	}
	pUnit->fZVelocity = flZVelocity;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeZVelocity(
	UNIT *pUnit,
	float flZVelocityDelta)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitSetZVelocity( pUnit, UnitGetZVelocity( pUnit ) + flZVelocityDelta );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetZVelocity(
	const UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	return pUnit->fZVelocity;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetOnGround(
	UNIT *pUnit,
	BOOL bOnGround)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitSetFlag( pUnit, UNITFLAG_ONGROUND, bOnGround );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsOnGround(
	const UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitTestFlag( pUnit, UNITFLAG_ONGROUND );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsOnTrain(
	const UNIT * pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitTestFlag(pUnit, UNITFLAG_ON_TRAIN);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInTutorial(
	UNIT * pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_TUTORIAL_TIPS_ACTIVE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARTYID UnitGetPartyId(
	const UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_ID, "Expected unit" );

	GAMECLIENT * client = UnitGetClient( pUnit );
	if( !client )
	{
		return INVALID_ID;
	}

	return ClientSystemGetParty( AppGetClientSystem(), ClientGetAppId( client ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL c_UnitIsInMyParty(
	const UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_CLIENT( pGame ), "Client only" );
	if ( AppIsSinglePlayer() )
		return FALSE;

	PGUID guidUnit = UnitGetGuid( pUnit );
	if ( guidUnit == INVALID_GUID )
		return FALSE;

	for( int nMember = 0; nMember < MAX_CHAT_PARTY_MEMBERS; nMember++ )
	{			
		if( guidUnit == c_PlayerGetPartyMemberUnitGuid(nMember) )
			return TRUE;
	}
	return FALSE;
}
#endif

//----------------------------------------------------------------------------
// calculate a party difficulty offset
// basically, level - 1 offsets the difficulty of a dungeon.
// currently, in a party, we just take the highest level party member and use theirs.

int UnitGetDifficultyOffset( UNIT * pUnit )
{
	
	return UnitGetExperienceLevel( pUnit ) - 1;
	/*	if( !UnitCanDoPartyOperations( pUnit ) ) //single party
	{
	}
	else //multi party
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		GAMECLIENT *pClient = UnitGetClient( pUnit );
		PARTYID idParty = ClientSystemGetParty( AppGetClientSystem(), ClientGetAppId( pClient ));		
		GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
		ASSERT_RETFALSE( svr );
		if( svr )
		{
			CachedPartyPtr party(svr->m_PartyCache);
			party = PartyCacheLockParty(
						svr->m_PartyCache,
						idParty );	
			int nOffset = 0;
			if( party )
			{
				const int MAX_MEMBERS = 128;
				UNIT* pPartyMembers[MAX_MEMBERS];
				int nMembers = 0;
				for( int t = 0; t < (int)party->Members.Count(); t++ )
				{
					CachedPartyMember * newMember = party->Members[ t ];
					APPCLIENTID AppId = ClientSystemFindByPguid( svr->m_ClientSystem, newMember->GuidPlayer );
					if( AppId != INVALID_ID )
					{
						GAMECLIENTID GameClientId = ClientSystemGetClientGameId( svr->m_ClientSystem, AppId );
						if( GameClientId != INVALID_ID )
						{
							GAMECLIENT* pClient = ClientGetById( UnitGetGame( pUnit ), GameClientId );
							if( pClient )
							{
								ASSERTX_BREAK( nMembers <= MAX_MEMBERS -1, "Too many members in party" );
								pPartyMembers[ nMembers++ ] = ClientGetPlayerUnit( pClient, TRUE );
							}
						}
					}
					
				}
				
				for( int i = 0; i < nMembers; i++ )
				{
					UNIT *unitMember = pPartyMembers[ i ];
					if( unitMember )
					{			
						if( UnitGetExperienceLevel( pUnit ) - 1 > nOffset )
						{
							nOffset = UnitGetExperienceLevel( pUnit ) - 1;
						}
					}
				}
				return  nOffset;
			}

		}
#endif
	}
	return UnitGetExperienceLevel( pUnit ) - 1;*/
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStartLevelDefinition(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_LINK, "Expected unit" );	
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETVAL( pGame, INVALID_LINK, "Expected game" );

	GAME_GLOBAL_DEFINITION *pGlobalDef = DefinitionGetGameGlobal();

	// check for hammer override
	int nLevelDefStart = pGlobalDef->nLevelDefinition;

	// check for cmd line override
	if ( AppTestDebugFlag( ADF_START_SOUNDFIELD_BIT ) || 
		(nLevelDefStart == INVALID_LINK) && (pGlobalDef->nDRLGOverride != INVALID_LINK))
	{
		nLevelDefStart = GlobalIndexGet( GI_LEVEL_SOUND_FIELD );
	}
	
	// check what's saved on the player
	int nDifficulty = DifficultyGetCurrent( pUnit );
	if (nLevelDefStart == INVALID_LINK)
	{
	
		// get start level definition
		nLevelDefStart = UnitGetStat( pUnit, STATS_LEVEL_DEF_START, nDifficulty);
		
		if(AppIsHellgate())
		{
			// there are some players out there that beat the game, and upon beating the game it changed their
			// current difficulty to nightmare, if they then went to st. pauls station (or any other station)
			//  at the end of the game, it would set their "start level" for *nightmare* mode to that station.  
			// We need to fix those characters here so that they start at holborn station
			if (nDifficulty == DIFFICULTY_NIGHTMARE &&
				WaypointIsMarked( pGame, pUnit, GlobalIndexGet( GI_LEVEL_HOLBORN_STATION ), nDifficulty ) == FALSE &&
				nLevelDefStart != GlobalIndexGet(GI_LEVEL_RUSSELL_SQUARE) &&
				nLevelDefStart != INVALID_ID)
			{
				// this is a nightmare player, but they don't have the holborn station waypoint, which is the very
				// first waypoint in the game, ignore anything in the save file and start them at holborn
				nLevelDefStart = GlobalIndexGet(GI_LEVEL_HOLBORN_STATION);
			}
		}		
	}
	
	// nothing saved on player, do the first level
	if (nLevelDefStart == INVALID_LINK)
	{
		nLevelDefStart = LevelGetStartNewGameLevelDefinition();
	}

	// validate level
	nLevelDefStart = LevelDestinationValidate( pUnit, nLevelDefStart );
		
	return nLevelDefStart;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetReturnLevelDefinition(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_LINK, "Expected unit" );	
	
	// check what's saved on the player
	int nLevelDefStart = UnitGetStat( pUnit, STATS_LEVEL_DEF_RETURN, UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT));
	
	// check where they might start
	if (nLevelDefStart == INVALID_LINK)
	{
		nLevelDefStart = UnitGetStat( pUnit, STATS_LEVEL_DEF_START , UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT));
	}

	// nothing saved on player, do the first level
	if (nLevelDefStart == INVALID_LINK)
	{
		nLevelDefStart = LevelGetStartNewGameLevelDefinition();
	}

	nLevelDefStart = LevelDestinationValidate( pUnit, nLevelDefStart );

	return nLevelDefStart;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetScale(
	UNIT* unit,
	float fScale)
{
	unit->fScale = fScale;

	if(unit->pGfx && unit->pGfx->nModelIdThirdPerson != INVALID_ID)
	{
		float fModelScale = unit->pUnitData->fScaleMultiplier * fScale;
		e_ModelSetScale( unit->pGfx->nModelIdThirdPerson, fModelScale );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanDoPartyOperations(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// can never do party ops if we're flagged as such
	if (UnitTestFlag( pUnit, UNITFLAG_DONT_ALLOW_PARTY_ACTIONS ))
	{
		return FALSE;
	}
	
	// if I have no party, and there are others in this game, and this is a dungeon 
	// level, I can't do party stuff
	GAMECLIENT *pClient = UnitGetClient( pUnit );
	if ( ! pClient )
		return FALSE;

	PARTYID idParty = ClientSystemGetParty( AppGetClientSystem(), ClientGetAppId( pClient ));
	if (idParty == INVALID_ID && 
		GameGetClientCount( pGame ) > 1 && 
		GameAllowsActionLevels( pGame ) == TRUE)
	{
		return FALSE;
	}

	return TRUE;  // all ok
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveFromPartyGame(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{

	// if the unit can't do party operations (ie, they are not in a party still) then
	// remove them back to town
	{
	
		// setup warp spec
		WARP_SPEC tWarpSpec;
		tWarpSpec.nLevelDefDest = UnitGetStartLevelDefinition( pUnit );
		SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
		
		// warp
		s_WarpToLevelOrInstance( pUnit, tWarpSpec );
			
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitDelayedRemoveFromPartyGame(
	UNIT *pUnit)
{	
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	GAMECLIENTID idClient = UnitGetClientId( pUnit );

	// only do then when in an action level game
	if (GameAllowsActionLevels( pGame ) == FALSE)
	{
		return;
	}

	// set flag on unit
	UnitSetFlag( pUnit, UNITFLAG_DONT_ALLOW_PARTY_ACTIONS );
	
	// how long is the delay
	DWORD dwDelayInTicks = GAME_TICKS_PER_SECOND * 15;
	
	// send message to client telling them so
	MSG_SCMD_REMOVING_FROM_RESERVED_GAME tMessage;
	tMessage.dwDelayInTicks = dwDelayInTicks;
	s_SendMessage( pGame, idClient, SCMD_REMOVING_FROM_RESERVED_GAME, &tMessage);
	
	// setup event to remove them
	UnitRegisterEventTimed( pUnit, sRemoveFromPartyGame, NULL, dwDelayInTicks );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define UNIT_BREATH_MAX		90

void c_UnitBreathChanged( UNIT * unit )
{
#ifndef DISABLE_BREATH
	ASSERT_RETURN( unit );
	ASSERT_RETURN( IS_CLIENT( unit ) );

#if !ISVERSION(SERVER_VERSION)
	if ( IsUnitDeadOrDying( unit ) )
		return;

	int breath = UnitGetStat( unit, STATS_BREATH );
	if ( breath != UNIT_BREATH_MAX )
	{
		UIShowBreathMeter();
	}
	else
	{
		UIHideBreathMeter();
	}
#endif// !ISVERSION(SERVER_VERSION)
#endif// DISABLE_BREATH
}

#ifndef DISABLE_BREATH

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitCanHaveBreath( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );  
	return UnitIsPlayer( pUnit ); //only players can have breath
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(CHEATS_ENABLED)
BOOL gInfiniteBreath = FALSE;
#endif

static BOOL sUpdateBreath( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERT_RETFALSE( IS_SERVER( game ) );

	if ( AppIsTugboat() )
		return TRUE;
		
	if ( IsUnitDeadOrDying( unit ) )
		return TRUE;

	if (sUnitCanHaveBreath( unit ) == FALSE)
	{
		return TRUE;
	}

	if (UnitIsGhost( unit ))
	{
		return TRUE;
	}
	
	int delay = GAME_TICKS_PER_SECOND;

	ROOM * room = UnitGetRoom( unit );
	if ( room )
	{
		int breath = UnitGetStat( unit, STATS_BREATH );
		
		// am I in hellrift area?
		if (UnitIsInHellrift( unit ))
		{
			breath--;
			if ( breath <= 0 )
			{
				// die
				UnitSetStat( unit, STATS_BREATH, 0 );
				UnitDie( unit, NULL );
				return TRUE;
			}
		}
		else if ( breath < UNIT_BREATH_MAX )
		{
			breath++;
			delay = GAME_TICKS_PER_SECOND / 4;
		}
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
		if ( !GameGetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH) ) // TODO: May need to fix this; without this, the client might go to hell w/ breath
#endif
		UnitSetStat( unit, STATS_BREATH, breath );

		if ( breath < ( UNIT_BREATH_MAX / 4 ) )
		{
			if (!UnitHasState(UnitGetGame(unit), unit, STATE_QUARTER_BREATH))
			{
				s_StateSet(unit, unit, STATE_QUARTER_BREATH, 0);
			}
		}
		else
		{
			s_StateClear(unit, UnitGetId(unit), STATE_QUARTER_BREATH, 0);
		}
	}

	if ( !UnitHasTimedEvent( unit, sUpdateBreath ) )
		UnitRegisterEventTimed( unit, sUpdateBreath, NULL, delay );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBreathExitLimbo(
	GAME *pGame,
	UNIT *pOwner,
	UNIT *pUnitOther,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	if ( AppIsHellgate() )
	{
		UnitSetStat( pOwner, STATS_BREATH, UNIT_BREATH_MAX );
		sUpdateBreath( pGame, pOwner, NULL );
	}
}

#endif// DISABLE_BREATH

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitBreathInit(
	UNIT *pUnit)
{
#ifndef DISABLE_BREATH

	ASSERT_RETURN( pUnit );
	GAME * game = UnitGetGame( pUnit );
	ASSERT_RETURN( IS_SERVER( game ) );
	LEVEL * level = UnitGetLevel( pUnit );
	if ( !level )
		return;

	if (LevelHasHellrift( level ) == FALSE)
	{
		return;
	}

	if (sUnitCanHaveBreath( pUnit ) == FALSE)
	{
		return;
	}
	
	if ( UnitIsInLimbo( pUnit ) )
	{
		UnitRegisterEventHandler( game, pUnit, EVENT_EXIT_LIMBO, sBreathExitLimbo, NULL );
	}
	else
	{
		if ( !UnitHasTimedEvent( pUnit, sUpdateBreath ) )
		{
			sBreathExitLimbo( game, pUnit, NULL, NULL, NULL, INVALID_ID );
		}
	}
#endif// DISABLE_BREATH
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitBreathLevelChange(
	UNIT *pUnit)
{
#ifndef DISABLE_BREATH

	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( IS_SERVER( pUnit ) );

	UnitUnregisterTimedEvent( pUnit, sUpdateBreath );

#endif// DISABLE_BREATH
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitFaceUnit(
	UNIT *pUnit,
	UNIT *pUnitOther)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pUnitOther, "Expected other unit" );
	
	// face who clicked me
	VECTOR vDirection = UnitGetPosition( pUnitOther ) - UnitGetPosition( pUnit );
	VectorNormalize( vDirection );
	UnitSetFaceDirection( pUnit, vDirection );
	
	// tell clients
	if (IS_SERVER( pUnit ))
	{
	
		MSG_SCMD_UNITTURNID tMessage;
		tMessage.id = UnitGetId( pUnit );
		tMessage.targetid = UnitGetId( pUnitOther );
		s_SendUnitMessage( pUnit, SCMD_UNITTURNID, &tMessage );

		UnitSetAITarget( pUnit, pUnitOther, FALSE, TRUE );
	}	
}


static const int DPS_MESSAGE_INTERVAL_SECS	= 2;
static const int DPS_AVERAGE_SECS			= 5;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitUpdateValueTimeTag(
	GAME* pGame, 
	UNIT* unit, 
	const EVENT_DATA& event_data)
{
	static BOOL bSentZeroes = FALSE;

	TAG_SELECTOR eTag = (TAG_SELECTOR) event_data.m_Data1;
	ASSERT_RETFALSE( eTag >= 0 && eTag < TAG_SELECTOR_LAST );

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, eTag);
	if (pTag)
	{
		// Tell the client of the player how they're doing
		if (eTag == TAG_SELECTOR_DAMAGE_DONE)
		{
			int nTotalDmgDone = VMDamageDone( unit, DPS_AVERAGE_SECS);	

					//if (nTotalDmgDone > 0)
					//{
					//	char szTemp[256];
					//	int nBucket = pTag->m_nCurrent + 1;
					//	for (int i=0; i < VALUE_TIME_HISTORY; i++)
					//	{
					//		if (nBucket >= VALUE_TIME_HISTORY)
					//			nBucket = 0;
					//		PStrPrintf(szTemp, 256, "%d ", pTag->m_nCounts[nBucket++] >> StatsGetShift( pGame, STATS_HP_CUR ));
			
					//		trace(szTemp);
					//	}
					//	PStrPrintf(szTemp, 256, "%d\t%d\n", nTotalDmgDone, nTotalDmgDone >> StatsGetShift( pGame, STATS_HP_CUR ));
					//	trace(szTemp);
					//}

			if (nTotalDmgDone > UnitGetStat(unit, STATS_DPS_MAX_RECORDED))
			{
				UnitSetStat(unit, STATS_DPS_MAX_RECORDED, nTotalDmgDone);
			}

			if (nTotalDmgDone != 0 || UnitGetStat(unit, STATS_DPS_LAST_SENT) != 0 || !bSentZeroes)
			{
				if (nTotalDmgDone != 0 || UnitGetStat(unit, STATS_DPS_LAST_SENT) != 0)
				{
					bSentZeroes = FALSE;
				}
				TIME tiCurTime = AppCommonGetCurTime();
				if ( ( tiCurTime / GAME_TICKS_PER_SECOND ) % DPS_MESSAGE_INTERVAL_SECS == 0)
				{
					UnitSetStat(unit, STATS_DPS_LAST_SENT, nTotalDmgDone);

					if (UnitCanSendMessages( unit ))
					{
						MSG_SCMD_DPS_INFO tMessage;
						tMessage.fDPS = ((float)(nTotalDmgDone >> StatsGetShift( pGame, STATS_HP_CUR )) / DPS_AVERAGE_SECS );

						s_SendMessage( pGame, UnitGetClientId(unit), SCMD_DPS_INFO, &tMessage );

						if ((nTotalDmgDone == 0 || UnitGetStat(unit, STATS_DPS_LAST_SENT) == 0) && !bSentZeroes)
						{
							bSentZeroes = TRUE;
						}
					}
				}
			}
		}

		// advance the timer
		pTag->m_nCurrent++;
		if (pTag->m_nCurrent >= VALUE_TIME_HISTORY)
		{
			pTag->m_nCurrent = 0;
		}

		pTag->m_nMinuteCounts[pTag->m_nMinuteCurrent] += pTag->m_nCounts[pTag->m_nCurrent];

		pTag->m_nSecondCurrent++;
		if(pTag->m_nSecondCurrent >= SECONDS_PER_MINUTE)
		{
			pTag->m_nSecondCurrent = 0;
			pTag->m_nMinuteCurrent++;
			if(pTag->m_nMinuteCurrent >= VALUE_TIME_HISTORY_MINUTES)
			{
				pTag->m_nMinuteCurrent = 0;
			}

			pTag->m_nMinuteCounts[pTag->m_nMinuteCurrent] = 0;
		}

		pTag->m_nCounts[pTag->m_nCurrent] = 0;
	}

	UnitRegisterEventTimed(unit, sUnitUpdateValueTimeTag, &event_data, GAME_TICKS_PER_SECOND);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateDamageDone(
	GAME* game, 
	UNIT* unit, 
	UNIT* otherunit, 
	EVENT_DATA* event_data, 
	void* data, 
	DWORD dwId )
{
	ASSERT_RETURN( data );
	STATS * pDamageStats = (STATS * ) data;

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, TAG_SELECTOR_DAMAGE_DONE);
	if ( pTag )
	{
		int nDamage = StatsGetStat( pDamageStats, STATS_DAMAGE_DONE );
		pTag->m_nCounts[ pTag->m_nCurrent ] += nDamage;
		pTag->m_nTotalMeasured += nDamage;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitInitValueTimeTag(
	GAME* game, 
	UNIT* unit, 
	int nTag )
{
	if ( ! UnitHasTimedEvent( unit, sUnitUpdateValueTimeTag, CheckEventParam1, EVENT_DATA( nTag ) ) )
	{
		UnitRegisterEventTimed(unit, sUnitUpdateValueTimeTag, EVENT_DATA(nTag), GAME_TICKS_PER_SECOND);
	}

	if ( nTag == TAG_SELECTOR_DAMAGE_DONE )
	{
		if ( ! UnitHasEventHandler( game, unit, EVENT_DIDDAMAGE, sUpdateDamageDone ) )
		{
			UnitRegisterEventHandler( game, unit, EVENT_DIDDAMAGE, sUpdateDamageDone, NULL );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GENDER UnitGetGender(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, GENDER_INVALID, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return pUnitData->eGender;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetRace(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, GENDER_INVALID, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return pUnitData->nRace;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitWaitingForInventoryMove(
	UNIT *pUnit, 
	BOOL bWaiting)
{
	UnitSetFlag( pUnit, UNITFLAG_WAITING_FOR_SERVER_INV_MOVE, bWaiting );
#if !ISVERSION(SERVER_VERSION)
	// how come?
	if( AppIsHellgate() )
	{
		UIRepaint();
	}
#endif // !ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsWaitingForInventoryMove(
	UNIT *pUnit)
{
	return UnitTestFlag( pUnit, UNITFLAG_WAITING_FOR_SERVER_INV_MOVE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_UnitPlayCantUseSound(
	GAME* game,
	UNIT* container,
	UNIT* item)
{
	ASSERT_RETURN(game && container && item);
	ASSERT_RETURN(IS_CLIENT(game));
	if (UnitGetGenus(item) != GENUS_ITEM)
	{
		return;
	}

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(item));
	if (!item_data)
	{
		return;
	}

	if (item_data->m_nInvCantUseSound != INVALID_ID)
	{
		c_SoundPlay(item_data->m_nInvCantUseSound, &c_UnitGetPosition(container), NULL);
	}
}
#endif// !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_UnitPlayCantSound(
							GAME* game,
							UNIT* pUnit )
{
	ASSERT_RETURN(game && pUnit);
	ASSERT_RETURN(IS_CLIENT(game));


	const UNIT_DATA* unit_data = UnitGetData( pUnit );
	if (!unit_data)
	{
		return;
	}

	if (unit_data->m_nCantSound != INVALID_ID)
	{
		c_SoundPlay(unit_data->m_nCantSound, &c_UnitGetPosition(pUnit), NULL);
	}
}
#endif// !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam1(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return event_data.m_Data1 == check_data->m_Data1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam12(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return (event_data.m_Data1 == check_data->m_Data1) && 
		   (event_data.m_Data2 == check_data->m_Data2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam2(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return event_data.m_Data2 == check_data->m_Data2;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam123(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return (event_data.m_Data1 == check_data->m_Data1) && 
		   (event_data.m_Data2 == check_data->m_Data2) &&
		   (event_data.m_Data3 == check_data->m_Data3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam13(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return (event_data.m_Data1 == check_data->m_Data1) && 
		(event_data.m_Data3 == check_data->m_Data3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam14(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return (event_data.m_Data1 == check_data->m_Data1) && 
		(event_data.m_Data4 == check_data->m_Data4);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CheckEventParam4(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data)
{
	return event_data.m_Data4 == check_data->m_Data4;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsDecoy(
	UNIT *pUnit)
{
	return UnitTestFlag( pUnit, UNITFLAG_IS_DECOY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetPosition( 
	UNIT *pUnit,
	const VECTOR &vPosition)
{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT)		// don't assert on live servers to reduce server spam
	IF_NOT_RETURN(_finite(vPosition.fX) && !_isnan(vPosition.fX));
	IF_NOT_RETURN(_finite(vPosition.fY) && !_isnan(vPosition.fY));
#else
	ASSERT_RETURN(_finite(vPosition.fX) && !_isnan(vPosition.fX));
	ASSERT_RETURN(_finite(vPosition.fY) && !_isnan(vPosition.fY));
#endif
	pUnit->vPosition = vPosition;
	UnitSetDrawnPosition( pUnit, vPosition );
//	UnitPositionTrace(pUnit, "UnitSetPosition" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetDrawnPosition( 
	UNIT *pUnit,
	const VECTOR &vPosition)
{
	pUnit->vDrawnPosition = vPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangePosition(
	UNIT * unit,
	const VECTOR & position)
{
	VECTOR oldpos = unit->vPosition;
	UnitSetPosition(unit, position);
	UnitUpdateRoomFromPosition(UnitGetGame(unit), unit, &oldpos);
//	UnitPositionTrace( unit, "UnitChangePosition" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include "e_main.h"
void UnitDisplayGetOverHeadPositions( 
	UNIT *pUnit,
	UNIT *pControlUnit,
	VECTOR vEye,
	float fLabelHeight, 
	VECTOR& vScreenPos,
	float &fLabelScale,
	float &fControlDistSq,
	float &fCameraDistSq)
{
#if !ISVERSION(SERVER_VERSION)
	// get height for name
	float flNameHeight = AppIsHellgate() ? UnitGetCollisionHeight( pUnit ) : UnitGetCollisionHeight( pUnit ) + 1.5f;

	flNameHeight += UnitGetStat(pUnit, STATS_NAME_LABEL_ADJ) / 256.0f;

	// apply the override (this is for debugging and setting all the heights in the first place)
	if (gflNameHeightOverride != -1.0f)
	{
		flNameHeight = gflNameHeightOverride;
	}

	// I'm totally guessing here ... if this doesn't work for all everything feel free to change it
	flNameHeight *= c_UnitGetHeightFactor( pUnit );

	VECTOR vDelta = pUnit->vUpDirection;
	vDelta *= flNameHeight;
	VECTOR vWorldPos = UnitGetDrawnPosition( pUnit );
	vWorldPos += vDelta;

	// do forward offset
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	if (pUnitData->flLabelForwardOffset != 0.0f)
	{
		VECTOR vForward = UnitGetFaceDirection( pUnit, TRUE );
		VectorNormalize( vForward );
		VectorScale( vForward, vForward, pUnitData->flLabelForwardOffset );
		vWorldPos += vForward;
	}

	VECTOR vTopOfLabel = vWorldPos + (fLabelHeight * pUnit->vUpDirection);
					
	// get unit distance from eye
	fCameraDistSq = VectorDistanceSquared( vEye, vWorldPos );
	ASSERT( fCameraDistSq > 0.f );

	// distance from the player
	VECTOR vControlPos = UnitGetDrawnPosition(pControlUnit);
	fControlDistSq = VectorDistanceSquared( vControlPos, vWorldPos );

	if( AppIsTugboat() )
	{
		int x, y;
		TransformUnitSpaceToScreenSpace( pUnit, &x, &y );
		vScreenPos.fX = (float)x;
		vScreenPos.fY = (float)y;
		vScreenPos.fZ = -1.0f;
		fLabelScale = 1.0f;
		if( UnitIsA( pUnit, UNITTYPE_PLAYER ) )
		{
			vScreenPos.fY -= 40.0f;
		}
		return;
	}

	// transform world pos into screen space
	MATRIX vMat;
	VECTOR4 vTransformed;
	V( e_GetWorldViewProjMatrix( &vMat ) );
	MatrixMultiply( &vTransformed, &vWorldPos, &vMat );
	vScreenPos.fX = vTransformed.fX / vTransformed.fW;
	vScreenPos.fY = vTransformed.fY / vTransformed.fW;
	vScreenPos.fZ = vTransformed.fZ / vTransformed.fW;

	// transform the top of the label (to get the height scale)
	MatrixMultiply( &vTransformed, &vTopOfLabel, &vMat );
	VECTOR vTransLabelTop, vTransLabelBottom;
	vTransLabelTop.fX = vTransformed.fX / vTransformed.fW;
	vTransLabelTop.fY = vTransformed.fY / vTransformed.fW;
	vTransLabelTop.fZ = vTransformed.fZ / vTransformed.fW;
	vTransLabelBottom = vScreenPos;

	float fTransformedHeight = vTransLabelTop.fY - vTransLabelBottom.fY; //sqrtf(VectorDistanceSquared( vTransLabelTop, vTransLabelBottom ));

	// BSP - this is sort of a hack to control the condition of looking straight down on the label.  In that case the projected top will
	//  be very close to the projected bottom.  So just pin it to full size.
	if ( c_CameraGetViewType() == VIEW_1STPERSON &&
		 VectorDistanceSquaredXY(vControlPos, vWorldPos) <= 1.0f &&
		 fTransformedHeight < fLabelHeight )
	{
		fLabelScale = 1.0f;
	}
	else
	{
		// record a scale corresponding to the apparent distance between the top and bottom of the label
		fLabelScale = fabs(fTransformedHeight / fLabelHeight);
	}

#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetGUIDOwner(
	UNIT *pUnit,
	UNIT *pOwner)
{		
	ASSERTX_RETURN( pUnit, "Expected unit" );

	if (pOwner)
	{
	
		// set stat on unit
		PGUID guidOwner = UnitGetGuid( pOwner );	
		DWORD dwHigh;
		DWORD dwLow;
		SPLIT_GUID( guidOwner, dwHigh, dwLow );	
		UnitSetStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_LOW, dwLow );
		UnitSetStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_HIGH, dwHigh );
	
	}
	else
	{
		UnitClearStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_LOW );
		UnitClearStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_HIGH );
			
	}
		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID UnitGetGUIDOwner(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_GUID, "Expected unit" );

	// get stats
	DWORD dwLow = UnitGetStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_LOW );
	DWORD dwHigh = UnitGetStat( pUnit, STATS_OWNER_GUID, GUID_PARAM_HIGH );

	// construct GUID
	PGUID guid = CREATE_GUID( dwHigh, dwLow );
	return guid;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoSafetyClear(
	UNIT *pUnit,
	void *)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// get state to clear
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	int nState = pUnitData->nStateSafe;
	ASSERTX_RETURN( nState != INVALID_LINK, "Unit has no safe state" );

	// clear the state
	s_StateClear( pUnit, UnitGetId( pUnit ), nState, 0 );	

	// clear "safe" from all our pets too
	PetIteratePets( pUnit, sDoSafetyClear, NULL );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveSafety(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pUnitOther,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	sDoSafetyClear( pUnit, pData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetSafe(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// for now, Tugboat is not going to do 'safety' - 
	// very exploitable in high-level dungeons.
	// OK, now Tug does it for PVP - and MAYBE again for hostile levels. We'll see!
	//
	// Perhaps you should changing the safety time for Mythos, or maybe make is shorter the
	// deeper you go.  You should not remove it IMO, especially for the casual gamer, any
	// situation of death and resurrection and immediate death is likely to cause people to quit playing.
	// And who cares if you get one free hit on a boss on the lower level anyway?  Isn't that OK?
	// Think about those old skool NES games where you came back invulnerable for a short time, there
	// was no great exploitable badness was there?
	//
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	//LEVEL *pLevel = UnitGetLevel( pUnit );
	if( AppIsHellgate() ||
		( AppIsTugboat() &&
		   ( UnitPvPIsEnabled( pUnit ) || PlayerIsInPVPWorld( pUnit )) /*|| !( pLevel && LevelIsSafe( pLevel ) )*/ ) )
	{
		
		// get state
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		int nState = pUnitData->nStateSafe;
		ASSERTX_RETURN( nState != INVALID_LINK, "Trying to set unit as a safe, but unit has no safe state to use" );
					
		// set state	
		s_StateSet( pUnit, pUnit, nState, 0 );
			
		// set event to remove the safe state when an aggressive skill is started.
		// Aggressive unit modes use UNITMODE_DATA::nClearState to clear the safe state, in unitmodes.cpp
		UnitRegisterEventHandler(pGame, pUnit, EVENT_AGGRESSIVE_SKILL_STARTED, sRemoveSafety, NULL);

		//// do our pets too
		//PetIteratePets( pUnit, sDoSafetySet, NULL );	
		
	}
		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearSafe(
	UNIT *pUnit)
{
	UnitUnregisterEventHandler(
		UnitGetGame(pUnit), 
		pUnit, 
		EVENT_AGGRESSIVE_SKILL_STARTED, 
		sRemoveSafety);

	sDoSafetyClear( pUnit, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckHeadstone(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pUnit ), "Expected player" );

	// get last headstone
	UNITID idLastHeadstone = UnitGetStat( pUnit, STATS_NEWEST_HEADSTONE_UNIT_ID );
	ASSERTX_RETFALSE( idLastHeadstone != INVALID_ID, "Expected last headstone unit id" );
	UNIT *pHeadstone = UnitGetById( pGame, idLastHeadstone );
	ASSERTX_RETFALSE( pHeadstone, "Expected headstone unit" );
	
	// get distance between us and the headstone
	BOOL bNearHeadstone = FALSE;
	float flDistSq = UnitsGetDistanceSquaredXY( pUnit, pHeadstone );
	if (flDistSq <= GHOST_RESURRECT_AT_HEADSTONE_RADIUS_SQ)
	{
		if (ClearLineOfSightBetweenUnits( pUnit, pHeadstone ))
		{
			bNearHeadstone = TRUE;
		}
		else
		{
		
			// OK, we're within distance, but we don't have clear line of sight ... 
			// we're encountering bugs where the player is standing right near their
			// headstone but will not come back to life.  In an effort to make this more
			// rock solid we'll do a slightly more sloppy check here to hopefully
			// catch all edge cases
			if (flDistSq <= GHOST_SLOPPY_RESURRECT_AT_HEADSTONE_RADIUS_SQ)
			{
				const VECTOR &vHeadstonePos = UnitGetPosition( pHeadstone );
				const VECTOR &vUnitPos = UnitGetPosition( pUnit );
				float flZdifference = fabs( vHeadstonePos.fZ - vUnitPos.fZ );
				if (flZdifference <= 8.0f)
				{
					bNearHeadstone = TRUE;
				}
			}			
		}
		
	}
	
	// if we're near the headstone, we come back to life
	if (bNearHeadstone == TRUE)
	{
		UnitGhost( pUnit, FALSE );
	}	
	else
	{
		// nope, not near it ... lets check again soon and be friends ok? 
		UnitRegisterEventTimed( pUnit, sCheckHeadstone, NULL, GHOST_CHECK_INTERVAL_IN_TICKS );		
	}

	// debug info
#if ISVERSION(DEVELOPMENT)
	if (gbHeadstoneDebug)
	{
		const VECTOR &vPosPlayer = UnitGetPosition( pUnit );
		const VECTOR &vPosHeadstone = UnitGetPosition( pHeadstone );
		ConsoleString( "Debug sCheckHeadstone() information" );
		ConsoleString( 
			"Player '%d' at (%.2f,%.2f,%.2f), LevelID=%d", 
			UnitGetId( pUnit ), 
			vPosPlayer.fX,
			vPosPlayer.fY,
			vPosPlayer.fZ,
			LevelGetID( UnitGetLevel( pUnit ) ));
		ConsoleString( 
			"headstone '%d' at (%.2f,%.2f,%.2f), LevelID=%d", 
			UnitGetId( pHeadstone ), 
			vPosHeadstone.fX,
			vPosHeadstone.fY,
			vPosHeadstone.fZ,
			LevelGetID( UnitGetLevel( pHeadstone ) ));
		ConsoleString(
			"Distance Squared to headstone '%f'",
			UnitsGetDistanceSquaredXY( pUnit, pHeadstone ));			
	}
#endif
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitGhost( 
	UNIT *pUnit,
	BOOL bEnable)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// set ghost state
	if (bEnable)
	{
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );

		// enable state or do skill
		if (pUnitData->nSkillGhost != INVALID_LINK)
		{
			GAME *pGame = UnitGetGame( pUnit );
			SkillStartRequest( 
				pGame, 
				pUnit, 
				pUnitData->nSkillGhost, 
				UnitGetId( pUnit ), 
				UnitGetPosition( pUnit ), 
				0, 
				0);
		}
		else
		{
			s_StateSet( pUnit, pUnit, STATE_GHOST, 0 );
		}

		// set event to check for getting close to our headstone for players
		if (UnitIsPlayer( pUnit ))
		{
			UnitRegisterEventTimed( pUnit, sCheckHeadstone, NULL, GHOST_CHECK_INTERVAL_IN_TICKS );
		}

	}
	else
	{

		// clear state
		s_StateClear( pUnit, UnitGetId( pUnit ), STATE_GHOST, 0 );

		// remove check headstone event
		UnitUnregisterTimedEvent( pUnit, sCheckHeadstone );

		// do effect
		s_StateSet( pUnit, pUnit, STATE_RESURRECT, 0 );

		// make unit safe for a while
		UnitSetSafe( pUnit );

		// set their vitals
		s_PlayerRestoreVitals( pUnit, FALSE );


		if (AppIsHellgate( ) )
		{
			s_MetagameEventPlayerRespawn( pUnit );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGhost(
	UNIT *pUnit,
	BOOL bIncludeHardcoreDead /*= TRUE*/)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// dead ghost players
	if (UnitHasState( pGame, pUnit, STATE_GHOST ))
	{
		return TRUE;
	}

	// player specific checks
	if (UnitIsPlayer( pUnit ))
	{
	
		// hardcore players that are dead are also considered ghosts
		if (bIncludeHardcoreDead)
		{
			if (PlayerIsHardcoreDead( pUnit ))
			{
				return TRUE;
			}
		}
		
	}

	return FALSE;
				
}

//----------------------------------------------------------------------------
// Recursively checks the unit, returning true if any units are subscriber only 
//----------------------------------------------------------------------------
BOOL UnitIsSubscriberOnly(UNIT* pUnit)
{
	ASSERT_RETFALSE(pUnit);

	if(UnitDataTestFlag(pUnit->pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY))
		return TRUE;

	UNIT * item = NULL;
	for (item = UnitInventoryIterate(pUnit, NULL); item;item = UnitInventoryIterate(pUnit, item))
	{
		if(UnitIsSubscriberOnly(item))
			return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
// Is pUnit 'in front of' pUnitFrontOf
//----------------------------------------------------------------------------
BOOL UnitIsInFrontOf(
	UNIT *pUnit,
	UNIT *pUnitFrontOf)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pUnitFrontOf, "Expected unit front of" );

	return VectorPosIsInFrontOf( 
		UnitGetPosition( pUnit ), 
		UnitGetPosition( pUnitFrontOf ), 
		UnitGetFaceDirection( pUnitFrontOf, FALSE ));
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsBehindOf(
	UNIT *pUnit,
	UNIT *pUnitBehindOf)
{
	return !UnitIsInFrontOf( pUnit, pUnitBehindOf );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsPlayer(
	const UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsA( pUnit, UNITTYPE_PLAYER );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID UnitGetSubLevelID(
	const UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	ROOM *pRoom = UnitGetRoom( pUnit );
	if (pRoom)
	{
		return RoomGetSubLevelID( pRoom );
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *UnitGetSubLevel(
	const UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	SUBLEVELID idSubLevel = UnitGetSubLevelID( pUnit );
	if (idSubLevel != INVALID_ID)
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );
		ASSERTX_RETNULL( pLevel, "Expected level" );
		return SubLevelGetById( pLevel, idSubLevel );
	}
	return NULL;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitsAreInSameLevel(
	const UNIT *pUnitA,
	const UNIT *pUnitB)
{
	ASSERTX_RETFALSE( pUnitA, "Expected unit" );
	ASSERTX_RETFALSE( pUnitB, "Expected unit" );
	LEVEL *pLevelA = UnitGetLevel( pUnitA );
	LEVEL *pLevelB = UnitGetLevel( pUnitB );
	return (pLevelA == pLevelB) && pLevelA != NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitsAreInSameSubLevel(
	const UNIT *pUnitA,
	const UNIT *pUnitB)
{
	ASSERTX_RETFALSE( pUnitA, "Expected unit" );
	ASSERTX_RETFALSE( pUnitB, "Expected unit" );
	SUBLEVELID idSubLevelA = UnitGetSubLevelID( pUnitA );
	SUBLEVELID idSubLevelB = UnitGetSubLevelID( pUnitB );	
	return (idSubLevelA == idSubLevelB) && (idSubLevelA != INVALID_ID);
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGI( 
	UNIT *pUnit,
	GLOBAL_INDEX eGlobalIndex)
{
	ASSERTX_RETFALSE( pUnit, "Epected unit" );
	ASSERTX_RETFALSE( eGlobalIndex != GI_INVALID, "Expeceted global index" );
	return UnitGetClass( pUnit ) == GlobalIndexGet( eGlobalIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//luck
float UnitGetLuck( UNIT *pUnit )							//unit
{
	float returnValue( 0.0f );
	int luck = 0;
	luck = UnitGetStat( pUnit, STATS_LUCK_ITEM_BONUS );
	float luckMax = (float)MAX_LUCK_HELLGATE;//(float)UnitGetStat( pUnit, STATS_LUCK_MAX );
	if( AppIsTugboat() )
	{
		luckMax = (float)MAX_LUCK_TUGBOAT;
	}
	if( pUnit && luck != 0 && luckMax != 0.0f )	
	{		
		returnValue = (float)luck/luckMax;
	}
	return returnValue * returnValue * returnValue;  //Lets ramp it!
}

//end luck
//tugboat added

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanRun(	
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_CAN_RUN );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanIdentify(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return InventoryGetFirstAnalyzer( pUnit ) != NULL;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_ENEMIES_FOR_THROTTLE 8.0f
static float sHolyRadiusGetThrottle(
	int nEnemyCount )
{
	return MIN( nEnemyCount / MAX_ENEMIES_FOR_THROTTLE, 1.0f );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HolyRadiusAdd(
	int nModelId,
	int nParticleSystemDefId,
	int nState,
	float fHolyRadius,
	int nEnemyCount )
{
	ATTACHMENT_DEFINITION tAttachmentDef;
	tAttachmentDef.nAttachedDefId = nParticleSystemDefId;
	tAttachmentDef.eType = ATTACHMENT_PARTICLE;

	int nAttachmentId = c_ModelAttachmentAdd( nModelId, tAttachmentDef, FILE_PATH_DATA, FALSE, 
		ATTACHMENT_OWNER_HOLYRADIUS, nState, INVALID_ID );

	ASSERT_RETURN( nAttachmentId != INVALID_ID );

	ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
	if ( pAttachment )
	{
		c_ParticleSystemSetParam( pAttachment->nAttachedId, PARTICLE_SYSTEM_PARAM_CIRCLE_RADIUS, fHolyRadius );

		float fParticleSystemThrottle = sHolyRadiusGetThrottle( nEnemyCount );
		c_ParticleSystemSetParam( pAttachment->nAttachedId, PARTICLE_SYSTEM_PARAM_PARTICLE_SPAWN_THROTTLE, fParticleSystemThrottle );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if  !ISVERSION(SERVER_VERSION)
void c_UnitDisableHolyRadius(
	UNIT * pUnit )
{
	int nModelId = c_UnitGetModelIdThirdPerson( pUnit );
	if ( nModelId != INVALID_ID )
	{
		c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_HOLYRADIUS, INVALID_ID );
	}

	nModelId = c_UnitGetModelIdFirstPerson( pUnit );
	if ( nModelId != INVALID_ID )
	{
		c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_HOLYRADIUS, INVALID_ID );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sUnitCreateHolyRadius( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nParticleSystemDefId,
	int nModelId )
{
	ASSERT_RETURN( IS_CLIENT(pGame) );

	if ( nModelId == INVALID_ID )
		nModelId = c_UnitGetModelId( pUnit );

	float fHolyRadius = UnitGetHolyRadius( pUnit );
	if ( fHolyRadius == 0.0f )
		fHolyRadius = 1.0f;

	int nEnemyCount = UnitGetStat( pUnit, STATS_HOLY_RADIUS_ENEMY_COUNT );

	HolyRadiusAdd( nModelId, nParticleSystemDefId, INVALID_ID, fHolyRadius, nEnemyCount );

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_STATE, pStatsEntries, ii, 32)
	{
		int nState = pStatsEntries[ ii ].GetParam();
		if ( StateIsA( nState, STATE_USES_HOLY_RADIUS ) )
		{
			const STATE_DEFINITION * pStateDefinition = StateGetDefinition( nState );
			for ( int i = 0; i < pStateDefinition->nEventCount; i++ )
			{
				STATE_EVENT * pEvent = & pStateDefinition->pEvents[ i ];
				
				// make sure this is holy radius event type
				const STATE_EVENT_TYPE_DATA *pStateEventTypeData = StateEventTypeGetData( pGame, pEvent->eType );
				if (c_StateEventIsHolyRadius(pStateEventTypeData))
				{
					// add holy radius
					HolyRadiusAdd( nModelId, pEvent->tAttachmentDef.nAttachedDefId, nState, 
						fHolyRadius, nEnemyCount );				
				}
			}
		}
	}
	STATS_ITERATE_STATS_END; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_UnitUpdateHolyRadiusParticleParam(
	UNIT * pUnit,
	PARTICLE_SYSTEM_PARAM eParam,
	float fParam )
{
	int nModelId = c_UnitGetModelId( pUnit );
	{
		int nAttachmentId = c_ModelAttachmentFind( nModelId, ATTACHMENT_OWNER_HOLYRADIUS, INVALID_ID );
		ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
		if ( pAttachment )
		{
			c_ParticleSystemSetParam( pAttachment->nAttachedId, eParam, fParam );
		}
	}

	UNIT_ITERATE_STATS_RANGE( pUnit, STATS_STATE, pStatsEntries, ii, 32)
	{
		int nState = pStatsEntries[ ii ].GetParam();
		if ( StateIsA( nState, STATE_USES_HOLY_RADIUS ) )
		{
			int nAttachmentId = c_ModelAttachmentFind( nModelId, ATTACHMENT_OWNER_HOLYRADIUS, nState );
			ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
			if ( pAttachment )
			{
				c_ParticleSystemSetParam( pAttachment->nAttachedId, eParam, fParam );
			}
		}
	}
	STATS_ITERATE_STATS_END; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitUpdateHolyRadiusSize(
	UNIT * pUnit )
{
	float fHolyRadius = UnitGetHolyRadius( pUnit );
	c_UnitUpdateHolyRadiusParticleParam( pUnit, PARTICLE_SYSTEM_PARAM_CIRCLE_RADIUS, fHolyRadius );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitUpdateHolyRadiusUnitCount(
	UNIT * pUnit )
{
	int nEnemyCount = MAX(UnitGetStat( pUnit, STATS_HOLY_RADIUS_ENEMY_COUNT ), UnitGetStat( pUnit, STATS_HOLY_RADIUS_FRIEND_COUNT ));
	float fThrottle = sHolyRadiusGetThrottle( nEnemyCount );
	c_UnitUpdateHolyRadiusParticleParam( pUnit, PARTICLE_SYSTEM_PARAM_PARTICLE_SPAWN_THROTTLE, fThrottle );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitEnableHolyRadius( 
	UNIT * unit)
{
	ASSERT_RETURN(unit);

	const UNIT_DATA * pUnitData = UnitGetData( unit );
	ASSERT_RETURN(pUnitData);
	if ( pUnitData->nHolyRadiusId != INVALID_ID )
		c_sUnitCreateHolyRadius( UnitGetGame( unit ), unit, pUnitData->nHolyRadiusId, INVALID_ID);
}

#endif//  !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
struct FUSE_LOOKUP
{
	int eGenus;
	PFN_FUSE_FUNCTION pfnFuse;
	PFN_UNFUSE_FUNCTION pfnUnFuse;
	PFN_GETFUSE_FUNCTION pfnQueryFuse;
};
static FUSE_LOOKUP sgtFuseTable[] = 
{
	{ GENUS_ITEM,		ItemSetFuse,		ItemUnFuse,			NULL },
	{ GENUS_MONSTER,	MonsterSetFuse,		MonsterUnFuse,		MonsterGetFuse },
	{ GENUS_MISSILE,	MissileSetFuse,		MissileUnFuse,		NULL },
	{ GENUS_OBJECT,		ObjectSetFuse,		ObjectUnFuse,		NULL },	
	{ GENUS_NONE,		NULL,				NULL,				NULL }		
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const FUSE_LOOKUP *sGetFuseLookup(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	
	// go through table
	GENUS eGenus = UnitGetGenus( pUnit );
	for (int i = 0; sgtFuseTable[ i ].eGenus != GENUS_NONE; ++i)
	{
		const FUSE_LOOKUP *pLookup = &sgtFuseTable[ i ];
		if (pLookup->eGenus == eGenus)
		{
			return pLookup;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetFuse(
	UNIT *pUnit, 
	int nTicks)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	const FUSE_LOOKUP *pLookup = sGetFuseLookup( pUnit );
	if (pLookup)
	{
		ASSERTX_RETURN( pLookup->pfnFuse, "Expected fuse function" );
		pLookup->pfnFuse( pUnit, nTicks );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitUnFuse(
	UNIT * pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	const FUSE_LOOKUP *pLookup = sGetFuseLookup( pUnit );
	if (pLookup)
	{
		ASSERTX_RETURN( pLookup->pfnUnFuse, "Expected unfuse function" );
		pLookup->pfnUnFuse( pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetFuse(
	UNIT * pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	const FUSE_LOOKUP *pLookup = sGetFuseLookup( pUnit );
	if (pLookup)
	{
		ASSERTX_RETZERO( pLookup->pfnQueryFuse, "Expected getfuse function" );
		return pLookup->pfnQueryFuse( pUnit );
	}

	return 0;
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsControlUnitDeadOrDying()
{
	GAME * pGame = AppGetCltGame();
	if(!pGame)
		return FALSE;

	UNIT * pControlUnit = GameGetControlUnit(pGame);
	if(!pControlUnit)
		return FALSE;

	return IsUnitDeadOrDying(pControlUnit);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUndoAllowTrigger(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	BOOL bAllow = (BOOL)tEventData.m_Data1;

	// undo what we did before	
	UnitAllowTriggers( pUnit, !bAllow, 0.0f );
	return TRUE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAllowTriggers(
	UNIT *pUnit,
	BOOL bAllow,
	float flDurationInSeconds)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	if (bAllow)
	{
		if (UnitGetStat( pUnit, STATS_DONT_CHECK_FOR_TRIGGERS_COUNTER ) == 0)
		{
			return;  // already allowed ... you can't allow any more, only restrict more
		}
	}
		
	// increase or decrease stat the stat count
	UnitChangeStat( pUnit, STATS_DONT_CHECK_FOR_TRIGGERS_COUNTER, 0, bAllow ? -1 : 1 );
	
	// if there is a duration, setup an event to undo this action after the duration has expired
	if (flDurationInSeconds > 0.0f)
	{
	
		// setup event data
		EVENT_DATA tEventData;
		tEventData.m_Data1 = bAllow;
		
		// register event
		DWORD dwDelay = (int)(GAME_TICKS_PER_SECOND_FLOAT * flDurationInSeconds);
		UnitRegisterEventTimed( pUnit, sUndoAllowTrigger, NULL, dwDelay );	
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetBaseClass(
	GENUS eGenus,
	int nUnitClass)
{
	ASSERTX_RETINVALID( nUnitClass != INVALID_LINK, "Expected unit class" );
	const UNIT_DATA *pUnitData = UnitGetData( NULL, eGenus, nUnitClass );
	if (pUnitData->nUnitDataBaseRow != INVALID_LINK)
	{
		return pUnitData->nUnitDataBaseRow;
	}
	return nUnitClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInHierarchyOf(
	GENUS eGenus,
	int nUnitClass,
	const int *pnUnitClassesApartOf,
	int nApartOfCount)
{
	ASSERTX_RETFALSE( eGenus > GENUS_NONE && eGenus < NUM_GENUS, "Invalid genus" );
	ASSERTX_RETFALSE( nUnitClass != INVALID_LINK, "Expected unit class" );
	ASSERTX_RETFALSE( pnUnitClassesApartOf, "Expected classes apart of" );

	// is this unit class part of the array itself
	for (int i = 0; i < nApartOfCount; ++i)
	{
	
		// get the other class
		int nClassOther = pnUnitClassesApartOf[ i ];

		// check for match on this row
		if (nUnitClass == nClassOther)
		{
			return TRUE;
		}
				
	}

	// check base class (if the unit we're checking)
	int nUnitClassBase = UnitGetBaseClass( eGenus, nUnitClass );
	if (nUnitClassBase != nUnitClass)
	{
		return UnitIsInHierarchyOf( eGenus, nUnitClassBase, pnUnitClassesApartOf, nApartOfCount );
	}

	// unit class or any of it's base classes are not apart of the array of classes passed in
	return FALSE;		
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitCanBeSaved(
	UNIT *pUnit,
	BOOL bIgnoreNoSave)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// check nosave flag explicitly
	if ( bIgnoreNoSave == FALSE && UnitTestFlag( pUnit, UNITFLAG_NOSAVE ))
	{
		return FALSE;
	}

	// don't save if item is in a slot that we don't save
//	if (UnitTestFlag( pUnit, UNITFLAG_IN_NOSAVE_INVENOTRY_SLOT ))
//	{
//		return FALSE;
//	}
	UNIT *pContainer = UnitGetContainer( pUnit );
	if (pContainer)
	{
	
		// get location in inventory
		INVENTORY_LOCATION tInvLoc;
		UnitGetInventoryLocation( pUnit, &tInvLoc );

		// get infomation about the slot the unit is in		
		INVLOC_HEADER tHeader;
		memclear( &tHeader, sizeof( INVLOC_HEADER ) );
		UnitInventoryGetLocInfo( pContainer, tInvLoc.nInvLocation, &tHeader );
		if (InvLocTestFlag( &tHeader, INVLOCFLAG_DONTSAVE ))
		{
			return FALSE;
		}
				
	}
					
	// do not save reward items for tasks that cannot be saved
	GAME *pGame = UnitGetGame( pUnit );
	GAMETASKID idTask = UnitGetStat( pUnit, STATS_TASK_REWARD );
	if (idTask != INVALID_ID)
	{
		if (s_TaskCanBeSaved( pGame, idTask ) == FALSE)
		{
			return FALSE;
		}
	}

	return TRUE;

#else 
	REF( pUnit );
	REF( bIgnoreNoSave );
	return FALSE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitHasSkill(	
	UNIT *pUnit,
	int nSkill)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nSkill != INVALID_LINK, "Invalid skill" );
	
	// must have level 1+
	int nSkillLevel = SkillGetLevel( pUnit, nSkill );
	return nSkillLevel > 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetForClientOnly(
	UNIT *pUnit,
	GAMECLIENT *pClient)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pClient, "Expected client" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetRestrictedToGUID(
	UNIT *pUnit,
	PGUID guidRestrictedTo)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// set stat on unit
	DWORD dwHigh;
	DWORD dwLow;
	SPLIT_GUID( guidRestrictedTo, dwHigh, dwLow );	
	UnitSetStat( pUnit, STATS_RESTRICTED_TO_GUID, GUID_PARAM_LOW, dwLow );
	UnitSetStat( pUnit, STATS_RESTRICTED_TO_GUID, GUID_PARAM_HIGH, dwHigh );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID UnitGetRestrictedToGUID(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, INVALID_GUID, "Expected unit" );
	
	// get stats
	DWORD dwLow = UnitGetStat( pUnit, STATS_RESTRICTED_TO_GUID, GUID_PARAM_LOW );
	DWORD dwHigh = UnitGetStat( pUnit, STATS_RESTRICTED_TO_GUID, GUID_PARAM_HIGH );

	// construct GUID
	PGUID guid = CREATE_GUID( dwHigh, dwLow );
	return guid;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanUse(
	UNIT *pUnit,
	UNIT *pObject)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pObject, "Expected object unit" );

	// check items
	if (UnitIsA( pObject, UNITTYPE_ITEM ))
	{
		if (InventoryItemMeetsClassReqs( pObject, pUnit ) == FALSE)
		{
			return FALSE;
		}
	}

	// check for town portals
	if (UnitIsA( pObject, UNITTYPE_PORTAL_TO_TOWN ))
	{
	
		// we only care about portals in the world, not the ones in the hidden town portal sublevel
		ROOM *pRoom = UnitGetRoom( pObject );
		ASSERTX_RETFALSE( pRoom, "Expected room" );
		if (RoomIsInDefaultSubLevel( pRoom ))
		{
			// for hellgate, you can only use your own town portals
			if (TownPortalCanUse( pUnit, pObject ) == FALSE)
			{
				return FALSE;
			}
		}		
		
	}

	// can use
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_UnitGetPossibleExperiencePenaltyPercent(
	UNIT *pUnit)
{
	int nPercent = 100;
	ASSERTX_RETVAL( pUnit, nPercent, "Expected unit" );	
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETVAL( IS_SERVER( pGame ), nPercent, "Server only" );	
		
	int nExperienceLevel = UnitGetExperienceLevel( pUnit );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	int nUnitType = pUnitData->nUnitTypeForLeveling;
	const PLAYERLEVEL_DATA *pPlayerLevelData = PlayerLevelGetData( pGame, nUnitType, nExperienceLevel );
	if (pPlayerLevelData)
	{
		nPercent = pPlayerLevelData->nDeathExperiencePenaltyPercent;
	}

	return nPercent;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitGiveExperience(
	UNIT * unit,
	int experience)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
	
	if (IS_CLIENT(game))
	{
		return;
	}

	// don't give any xp to hardcore dead players - they might levelup and resurrect!
	if( UnitIsA( unit, UNITTYPE_PLAYER ) &&
		PlayerIsHardcoreDead( unit ) )
	{
		return;
	}
	
	if ( AppIsHellgate() )
	{
		// check for anti-fatigue system experience penalty, for minors in China who aren't taking enough breaks
		int nAntifatiguePct = PIN(UnitGetStat(unit, STATS_ANTIFATIGUE_XP_DROP_PENALTY_PCT), 0, 100);
		if (nAntifatiguePct)
			experience = PCT(experience, 100 - nAntifatiguePct);
	}

	// check for death experience penalty, we're choosing to check the stat here instead of
	// the state for no other reason than we are explicitly saving it and manipulating it
	// on the server, so it's kinda the more "core" component of the stat/state 
	int nExpPenaltyDurInSeconds = UnitGetStat(unit, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS);
	if (nExpPenaltyDurInSeconds > 0)
	{
		int pct = s_UnitGetPossibleExperiencePenaltyPercent(unit);
		pct = PIN(pct, 0, 100);
		experience = PCT(experience, pct);
	}
	
	int bonus = UnitGetStat(unit, STATS_EXP_BONUS);
	if(bonus)
		experience = PCT(experience, 100 + bonus);

	// must still have experience
	if (experience <= 0)
	{
		return;
	}

	// check for no experience state
	if (UnitHasState(game, unit, STATE_NO_EXPERIENCE))
	{
		return;
	}

	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETURN(unit_data);
	if (UnitGetExperienceLevel(unit) >= (AppIsDemo()?5:unit_data->nMaxLevel))
	{
		if(AppIsHellgate() && !AppIsDemo())
		{
			if(UnitGetPlayerRank(unit) >= unit_data->nMaxRank)
			{
				return;
			}
			else
			{
				// change experience stat, level up will be checked via a stats callback on the server
#if ISVERSION(SERVER_VERSION)
				GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
				if (pContext)
				{
					PERF_ADD64(pContext->m_pPerfInstance,GameServer,ExperienceGainCount,1);
				}
#endif
				UnitChangeStat(unit, STATS_RANK_EXPERIENCE, experience);	
			}
		}
		else
		{
			return;
		}
	}
	else
	{
#if ISVERSION(SERVER_VERSION)
		GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
		if (pContext)
		{
			PERF_ADD64(pContext->m_pPerfInstance,GameServer,ExperienceGainCount,1);
		}
#endif
		// change experience stat, level up will be checked via a stats callback on the server
		UnitChangeStat(unit, STATS_EXPERIENCE, experience);	
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetMaxLevel(
	UNIT* unit )
{
	ASSERT_RETZERO( unit );

	const UNIT_DATA * pUnitData = UnitGetData( unit );
	if ( ! pUnitData )
		return 0;

	int nMaxLevel = pUnitData->nMaxLevel;
	if ( UnitIsPlayer( unit ) )
	{
		if ( AppIsDemo() )
			nMaxLevel = 5;
		else 
		{
			if(IS_SERVER(unit))
			{
				GAMECLIENT *pClient = UnitGetClient( unit );
				ASSERTV_RETZERO( pClient, "Expected client" );
				if ( ! ClientHasPermission( pClient, BADGE_PERMISSION_BETA_FULL_GAME ) )
				{
					if ( AppIsBeta() || UnitHasBadge( unit, ACCT_MODIFIER_BETA_GRACE_PERIOD_USER ) )
					{
						nMaxLevel = 22;
					}
				}
			}
		}
	}

	return nMaxLevel;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetMaxRank(
	UNIT* unit )
{
	ASSERT_RETZERO( unit );

	const UNIT_DATA * pUnitData = UnitGetData( unit );
	if ( ! pUnitData )
		return 0;

	int nMaxRank = pUnitData->nMaxRank;
	if ( UnitIsPlayer( unit ) )
	{
		if ( AppIsDemo() )
			nMaxRank = 0;
		else 
		{
			GAMECLIENT *pClient = UnitGetClient( unit );
			ASSERTV_RETZERO( pClient, "Expected client" );
			if ( ! ClientHasPermission( pClient, BADGE_PERMISSION_BETA_FULL_GAME ) )
			{
				if ( AppIsBeta() || UnitHasBadge( unit, ACCT_MODIFIER_BETA_GRACE_PERIOD_USER ) )
				{
					nMaxRank = 0;
				}
			}
		}
	}

	return nMaxRank;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsMonsterClass(
	UNIT *pUnit,
	int nMonsterClass)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsGenusClass( pUnit, GENUS_MONSTER, nMonsterClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsItemClass(
	UNIT *pUnit,
	int nItemClass)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsGenusClass( pUnit, GENUS_ITEM, nItemClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsObjectClass(
	UNIT *pUnit,
	int nObjectClass)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsGenusClass( pUnit, GENUS_OBJECT, nObjectClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsGenusClass(
	UNIT *pUnit,
	GENUS eGenus,
	int nClass)
{
	if (pUnit)
	{
		return UnitGetGenus( pUnit ) == eGenus && UnitGetClass( pUnit ) == nClass;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitsAllTriggerEvent(
	GAME *pGame,
	UNIT_EVENT eEvent,
	UNIT *pUnitOther,
	void *pData)
{
	// iterate all units, yah this is sucky
	for (int i = 0; i < UNIT_HASH_SIZE; ++i)
	{
		UNIT *pUnit = pGame->pUnitHash->pHash[ i ];
		while (pUnit)
		{
			UnitTriggerEvent( pGame, eEvent, pUnit, pUnitOther, pData );
			pUnit = pUnit->hashnext;
		}
	}	
}

//----------------------------------------------------------------------------
struct DO_EVENT_CALLBACK_DATA
{
	UNIT_EVENT eEvent;
	UNIT *pOtherUnit;
	void *pData;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDoUnitEvent( 
	UNIT *pUnit, 
	void *pCallbackData)
{
	const DO_EVENT_CALLBACK_DATA *pDoEventCallbackData = (const DO_EVENT_CALLBACK_DATA *)pCallbackData;
	GAME *pGame = UnitGetGame( pUnit );
	UnitTriggerEvent(
		pGame,
		pDoEventCallbackData->eEvent,
		pUnit,
		pDoEventCallbackData->pOtherUnit,
		pDoEventCallbackData->pData);
	return RIU_CONTINUE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsControlUnit(
   UNIT *pUnit )
{
#if ISVERSION(SERVER_VERSION)
	return FALSE;
#else
	GAME *pGame = UnitGetGame(pUnit);
	ASSERT_RETFALSE(pGame);
	if(IS_SERVER(pGame)) // on the real server, faster to just immediately return false
		return FALSE; //but single player server should *also* return false...
	return pUnit == GameGetControlUnit(pGame);
#endif
}

BOOL UnitHasBadge(
	UNIT *pUnit,
	int nBadge )
{
	if ( IS_SERVER( pUnit ) )
	{
		return ClientHasBadge( UnitGetClient( pUnit ), nBadge );
	} 
#if !ISVERSION(SERVER_VERSION)

	ASSERTX_RETFALSE(UnitIsControlUnit(pUnit), "Can't ask for badge info on the client for a unit that is not the control unit.");	// this info is only useful on the client for the control unit.
	const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();
	return pBadges ? pBadges->HasBadge( nBadge ) : FALSE;
#else
	REF( pUnit );
	REF( nBadge );
	return FALSE;
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetPlayerRank(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_PLAYER_RANK );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetPlayerRank(
	UNIT *pUnit,
	int nRank)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nRank >= 0, "Invalid experience level" );
	UnitSetStat( pUnit, STATS_PLAYER_RANK, nRank );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetExperienceLevel(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_LEVEL );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetExperienceRank(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_PLAYER_RANK );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetExperienceLevel(
	UNIT *pUnit,
	int nExperienceLevel)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nExperienceLevel >= 0, "Invalid experience level" );
	UnitSetStat( pUnit, STATS_LEVEL, nExperienceLevel );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetExperienceValue(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_EXPERIENCE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetRankExperienceValue(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_RANK_EXPERIENCE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD UnitGetPlayedTime(
	UNIT * pUnit )
{
	ASSERT_RETZERO( pUnit );
	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETZERO( pGame );
	ASSERTX(IS_SERVER(pGame), "Warning: the client does NOT accurately know its play time.  This information is only valid on the server.");

	const STATS_DATA * pStatsDataSaveTime = StatsGetData( pGame, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS );
	ASSERT_RETZERO( pStatsDataSaveTime );
	if ( ! pStatsDataSaveTime->m_bUnitTypeDebug[ UnitGetGenus(pUnit) ])
		return 0;

	GAME_TICK tPrevSave = (GAME_TICK)GAME_TICKS_PER_SECOND * (GAME_TICK)UnitGetStat( pUnit, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS );
	GAME_TICK tGameTick = GameGetTick( pGame );

	DWORD dwSecondsElapsed = (tGameTick - tPrevSave) / GAME_TICKS_PER_SECOND;
	DWORD dwPlayedTime = UnitGetStat( pUnit, STATS_PLAYED_TIME_IN_SECONDS );
	ASSERT( dwPlayedTime + dwSecondsElapsed >= dwPlayedTime );
	if ( dwPlayedTime + dwSecondsElapsed >= dwPlayedTime )
		dwPlayedTime += dwSecondsElapsed;

	return dwPlayedTime;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitInventoryCanBeRemoved(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// we may data drive this someday
	if (UnitIsA( pUnit, UNITTYPE_WEAPON ) || UnitIsA( pUnit, UNITTYPE_ARMOR ))
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetTeam(
	UNIT *pUnit,
	int nTeam,
	BOOL bValidate /*= TRUE*/)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );	
	
	// check for setting invalid teams ... we're currently having a problem with some monsters
	// being set to an invalid team, so we're explicity going to look for it here
	if (bValidate == TRUE)
	{
		ASSERTV_RETURN( 
			nTeam != INVALID_TEAM, 
			"Cannot set invalid team on unit (%s) [id=%d]",
			UnitGetDevName( pUnit ),
			UnitGetId( pUnit ));
	}	
	
	// set the team
	pUnit->nTeam = nTeam;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitVersion(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	UNITID idUnit = UnitGetId( pUnit );
	
	// version this unit
	switch (UnitGetGenus( pUnit ))
	{
	
		//----------------------------------------------------------------------------	
		case GENUS_PLAYER:
			s_PlayerVersion( pUnit );
			break;
			
		//----------------------------------------------------------------------------
		case GENUS_MONSTER:
			s_MonsterVersion( pUnit );
			break;
			
		//----------------------------------------------------------------------------
		case GENUS_ITEM:
			s_ItemVersion( pUnit );
			break;
			
	}

	// Use the versioning spreadsheets to modify the unit's stats due to rebalancing
	s_UnitVersionProps( pUnit );

	// version inventory if unit is still around
	pUnit = UnitGetById( pGame, idUnit );
	if (pUnit)
	{

		// this unit is now the current version
		UnitSetStat( pUnit, STATS_UNIT_VERSION, USV_CURRENT_VERSION );
	
		DWORD dwFlags = 0;
		UNIT *pContained = UnitInventoryIterate( pUnit, NULL, dwFlags );
		UNIT *pNextContained;
		while (pContained != NULL )
		{
		
			// get next item now incase the version function deletes it
			pNextContained = UnitInventoryIterate( pUnit, pContained, dwFlags );		

			// version this unit
			s_UnitVersion( pContained );			
			
			// go to next itme
			pContained = pNextContained;

		}
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetVersion(
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_UNIT_VERSION );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitsGetNearbyUnitsForParticles(
	int nMaxUnits, 
	const VECTOR & vCenter, 
	float fRadius, 
	VECTOR * pvPositions, 
	VECTOR * pvHeightsRadiiSpeeds, 
	VECTOR * pvDirections )
{
	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return 0;
	UNIT * pControlUnit = GameGetControlUnit( pGame );
	if ( ! pControlUnit )
		return 0;

	ROOM * pRoom = RoomGetFromPosition( pGame, UnitGetRoom( pControlUnit ), &vCenter );
	if ( ! pRoom )
		return 0;

	UNITID idTargets[ MAX_TARGETS_PER_QUERY ];

	SKILL_TARGETING_QUERY tTargetQuery;
	tTargetQuery.pnUnitIds = idTargets;
	tTargetQuery.fMaxRange = fRadius;
	tTargetQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	//SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_SEEKER );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetQuery.pSeekerUnit = pControlUnit;
	SkillTargetQuery( pGame, tTargetQuery );
	if (tTargetQuery.nUnitIdCount > 0)
	{
		int nCount = min( tTargetQuery.nUnitIdCount, nMaxUnits );
		for ( int i = 0; i < nCount; i++ )
		{
			UNIT * pCurr = UnitGetById( pGame, idTargets[ i ] );
			ASSERT_RETZERO( pCurr );
			pvPositions[ i ] = UnitGetPosition( pCurr );
			pvHeightsRadiiSpeeds[ i ].fX = UnitGetCollisionHeight( pCurr );
			pvHeightsRadiiSpeeds[ i ].fY = UnitGetCollisionRadius( pCurr );
			pvHeightsRadiiSpeeds[ i ].fZ = UnitGetVelocity( pCurr );
			pvDirections[ i ] = UnitGetMoveDirection( pCurr );
		}
		return nCount;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitMakeBoundingBox(
	UNIT * pUnit,
	BOUNDING_BOX * pBoundingBox)
{
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN(pBoundingBox);
	VECTOR vCenter = UnitGetPosition(pUnit);
	float fRadius = UnitGetCollisionRadius(pUnit);
	float fHeight = UnitGetCollisionHeight(pUnit);

	pBoundingBox->vMin.fX = vCenter.fX - fRadius;
	pBoundingBox->vMin.fY = vCenter.fY - fRadius;
	pBoundingBox->vMin.fZ = vCenter.fZ;
	pBoundingBox->vMax = pBoundingBox->vMin;

	VECTOR vNew;
	vNew.fX = vCenter.fX + fRadius;
	vNew.fY = vCenter.fY + fRadius;
	vNew.fZ = vCenter.fZ + fHeight;

	BoundingBoxExpandByPoint( pBoundingBox, &vNew );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsUnitDualWielding(
	GAME * pGame,
	UNIT * pUnit,
	UNITTYPE eUnitType)
{
	int nWardrobe = UnitGetWardrobe(pUnit);
	if( nWardrobe == INVALID_ID )
	{
		return FALSE;
	}
	for(int i=0; i<MAX_WEAPONS_PER_UNIT; i++)
	{
		UNIT * pWeapon = WardrobeGetWeapon(pGame, nWardrobe, i);
		if(pWeapon)
		{
			if(!UnitIsA(pWeapon, eUnitType))
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void UnitLostItemDebug(
	UNIT *pUnit)
{
	if (gbLostItemDebug)
	{
		UNIT *pUltimateContainer = UnitGetUltimateContainer( pUnit );
		ASSERT_RETURN( pUltimateContainer );
		if ( UnitIsPlayer( pUltimateContainer ) && UnitIsA( pUnit, UNITTYPE_ITEM ))
		{
			if (InventoryIsInStandardLocation( pUnit ))
			{			
				ASSERTV( 
					0, 
					"[%s] Lost Item Debug: Item '%s' being removed from '%s', please check bugzilla checkbox, grab a programmer and attach process to debugger",
					IS_CLIENT( UnitGetGame( pUnit )) ? "CLIENT" : "SERVER",
					UnitGetDevName( pUnit ),
					UnitGetDevName( pUltimateContainer ));
			}
		}
	}
}
#endif	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPECIES UnitResolveSpecies(
	GENUS eGenus,
	DWORD dwClassCode)
{
	SPECIES spResult = SPECIES_NONE;
	
	// get the excel table from genus
	EXCELTABLE eTable = ExcelGetDatatableByGenus( eGenus );
	if (eTable != DATATABLE_NONE)
	{
		int nClass = ExcelGetLineByCode( NULL, eTable, dwClassCode );
		if (nClass != INVALID_LINK)
		{
			spResult = MAKE_SPECIES( eGenus, nClass );
		}
	}

	return spResult;		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCheckCanRunEvent(
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return FALSE;
	}
	if (!UnitIsA(unit, UNITTYPE_MONSTER))
	{
		return TRUE;
	}
	if (IsUnitDeadOrDying(unit))
	{
		// keep running events if the unit is flagged as "never destroy dead"
		GAME *game = UnitGetGame(unit);
		if (!game)
			return FALSE;
		int nClass = UnitGetClass(unit);
		if (UnitGetGenus(unit) != GENUS_MONSTER || nClass == INVALID_ID)
			return FALSE;

		const UNIT_DATA *pMonsterData = UnitGetData(game, GENUS_MONSTER, nClass);
		if (!pMonsterData || !UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
			return FALSE;
	}
	if (UnitTestFlag(unit, UNITFLAG_DEACTIVATED))
	{
		return FALSE;
	}
	ROOM * room = UnitGetRoom(unit);
	if (!room)
	{
		return FALSE;
	}

	LEVEL * level = RoomGetLevel(room);
	if (!level)
	{
		return FALSE;
	}

	if (LevelIsActive(level) == FALSE)
	{
		return FALSE;
	}

	if (RoomIsActive(room) == FALSE)
	{
		if (s_UnitCanBeDeactivatedWithRoom(unit) == TRUE)
		{
			return FALSE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL UnitEmergencyDeactivate(
	UNIT * unit,
	const char * szDebugString)
#else
BOOL UnitEmergencyDeactivateRelease(
	UNIT * unit)
#endif
{
	ASSERT_RETFALSE(unit);

	ROOM * room = UnitGetRoom(unit);
	ASSERT_RETFALSE(room);

	LEVEL * level = RoomGetLevel(room);
	ASSERT_RETFALSE(level);

	if (LevelIsActive(level))
	{
		if (RoomIsActive(room))
		{
			return FALSE;
		}
		if (s_UnitCanBeDeactivatedWithRoom(unit) == FALSE)
		{
			return FALSE;
		}
	}

#if ISVERSION(DEVELOPMENT)
	TRACE_ACTIVE("ROOM [%d: %s]  UNIT [%d: %s]  %s\n", RoomGetId(room), RoomGetDevName(room), UnitGetId(unit), UnitGetDevName(unit), szDebugString);
#endif
	s_UnitDeactivate(unit, LevelIsActive(level) == FALSE);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInInstanceLevel(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	if (pLevel)
	{
		SRVLEVELTYPE eServerLevelType = LevelGetSrvLevelType( pLevel );
		return eServerLevelType == SRVLEVELTYPE_DEFAULT || eServerLevelType == SRVLEVELTYPE_TUTORIAL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCanBeCreated(
	GAME *pGame,
	const UNIT_DATA *pUnitData)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pUnitData, "Expected unit data" );
	
	if( pUnitData->nGlobalThemeRequired >= 0 && 
		!GlobalThemeIsEnabled( pGame, pUnitData->nGlobalThemeRequired ))
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsAlwaysKnownByPlayer(
	UNIT *pUnit,
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTV_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );
	
	// we never want to remove pets from their owner client on a room change -
	if( UnitIsContainedBy( pUnit, pPlayer ))
	{
		return TRUE;
	}
	
	// we always always know about our party members
	if (UnitGetPartyId(pUnit ) == UnitGetPartyId( pPlayer ) &&
		UnitGetLevel(pUnit) == UnitGetLevel(pPlayer))
	{
		return TRUE;
	}

	// we always know our duel opponents too
	if( UnitGetStat(pPlayer, STATS_PVP_DUEL_OPPONENT_ID) ==
		(int)UnitGetId( pUnit ) )
	{
		return TRUE;
	}

	// not always known by us	
	return FALSE;
		
}
#if !ISVERSION(CLIENT_ONLY_VERSION)
//////////////////////////////////////////////////////////////////
// Respawns a unit from their dead corpse
//////////////////////////////////////////////////////////////////
BOOL s_UnitRespawnFromCorpse( UNIT *pUnit )
{
	if( pUnit != NULL )
	{
		GAME *pGame = UnitGetGame( pUnit );
		ASSERT_RETFALSE( pGame );

		if( UnitHasState( pGame, pUnit, STATE_NO_RESPAWN_ALLOWED ) )
		{
			return FALSE;	//if it has the state we can't respawn it
		}
		if( IsUnitDeadOrDying( pUnit )  )
		{
			s_UnitRestoreVitals( pUnit, FALSE, NULL );
		}
		else
		{
			return FALSE;	//they aren't dead
		}
		// a respawner has already been created - don't stack up the respawns!
		UnitClearFlag( pUnit, UNITFLAG_SHOULD_RESPAWN );

		UnitSetFlag(pUnit, UNITFLAG_GIVESLOOT); //make them drop loot again
		s_UnitActivate( pUnit );//now lets activate the unit
		return TRUE; //if the unit isn't null ( it hasn't been destroyed and if it's not dead or dying then we don't need to respawn it
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////
// Respawns a unit from a respawner dummy
//////////////////////////////////////////////////////////////////
BOOL s_MonsterRespawnFromRespawner( 
	   GAME * pGame,
	   UNIT * unit,
	   const struct EVENT_DATA & tEventData)

{

	if( unit != NULL )
	{
		ROOM* pRoom = UnitGetRoom( unit );
		LEVEL* pLevel = UnitGetLevel( unit );
		int nClass = UnitGetStat( unit, STATS_RESPAWN_CLASS_ID );
		SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];
		const UNIT_DATA* pData = MonsterGetData( NULL, nClass );
		int nMonsterExperienceLevel = UnitGetExperienceLevel( unit );
		int nDesiredMonsterQuality = MonsterGetQuality( unit );
		if( !UnitTestFlag( unit, UNITFLAG_RESPAWN_EXACT ) )
		{
			nMonsterExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
			float flChampionSpawnRate = s_LevelGetChampionSpawnRate( pGame, pLevel );
			// roll
			nDesiredMonsterQuality = INVALID_LINK;
			if (RandGetFloat( pGame ) >= (1.0f - flChampionSpawnRate))
			{

				nDesiredMonsterQuality = MonsterQualityPick( pGame, MQT_ANY );
			}
			if (pData && !UnitDataTestFlag(pData, UNIT_DATA_FLAG_CAN_BE_CHAMPION))
			{
				nDesiredMonsterQuality = INVALID_LINK;
			}

			if( pData->nRespawnMonsterClass != INVALID_LINK )
			{
				nClass = pData->nRespawnMonsterClass;
			}
			else if ( pData && pData->nRespawnSpawnClass != INVALID_LINK )
			{			
				DWORD dwSpawnClassEvaluateFlags = 0;
				SETBIT( dwSpawnClassEvaluateFlags, SCEF_RANDOMIZE_PICKS_BIT );

				int nPicks = SpawnClassEvaluate( pGame, 
					pLevel->m_idLevel, 
					pData->nRespawnSpawnClass,
					nMonsterExperienceLevel, 
					tSpawns, 
					NULL,
					-1,
					dwSpawnClassEvaluateFlags);
				if( nPicks > 0 )
				{
					nClass = tSpawns[0].nIndex;
				}
			}

		}
		VECTOR Normal( 0, 0, -1 );
		VECTOR vSpawn = UnitGetPosition( unit );
		VECTOR Start = vSpawn;

		if( pLevel )
		{
			Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
			float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
			VECTOR vCollisionNormal;
			int Material;
			float fCollideDistance = LevelLineCollideLen( pGame, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
			if (fCollideDistance < fDist &&
				Material != PROP_MATERIAL &&
				Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
				Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
			{
				vSpawn.fZ = Start.fZ - fCollideDistance;
			}
		}

		float fAngle = TWOxPI * RandGetFloat(pGame);
		VECTOR vDirection;
		VectorDirectionFromAngleRadians( vDirection, fAngle );
		MONSTER_SPEC tSpec;
		tSpec.nClass = nClass;
		tSpec.nExperienceLevel = nMonsterExperienceLevel;
		tSpec.nMonsterQuality = nDesiredMonsterQuality;
		tSpec.pRoom = UnitGetRoom( unit );
		tSpec.vPosition= vSpawn;
		tSpec.vDirection = vDirection;
		tSpec.nWeaponClass = INVALID_ID;

		UNIT* pMonster = s_MonsterSpawn( pGame, tSpec );

		if( pMonster && QuestUnitIsQuestUnit( unit ) )
		{
			UnitSetFlag(pMonster, UNITFLAG_SHOULD_RESPAWN);
			if( UnitTestFlag( unit, UNITFLAG_RESPAWN_EXACT ) )
			{
				UnitSetFlag(pMonster, UNITFLAG_RESPAWN_EXACT);
			}

			const QUEST_TASK_DEFINITION_TUGBOAT*  pQuestTask = QuestUnitGetTask( unit );
			int nIndex = QuestUnitGetIndex( unit );
			if( pQuestTask )
			{
				s_SetUnitAsQuestUnit( pMonster, NULL, pQuestTask, nIndex );
			}
		}
		
		// throw away the respawner - worthless now!
		UnitDie(unit, NULL);
		UnitFree( unit, UFF_SEND_TO_CLIENTS );
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////
// Respawns a unit from a respawner dummy
//////////////////////////////////////////////////////////////////
BOOL s_ObjectRespawnFromRespawner( 
								   GAME * pGame,
								   UNIT * unit,
								   const struct EVENT_DATA & tEventData)

{

	if( unit != NULL )
	{	


		ROOM* pRoom = UnitGetRoom( unit );
		LEVEL* pLevel = UnitGetLevel( unit );
		int nClass = UnitGetStat( unit, STATS_RESPAWN_CLASS_ID );
		SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];
		UNIT_DATA* pData = UnitGetData( pGame, GENUS_OBJECT, nClass );
		int nMonsterExperienceLevel = UnitGetExperienceLevel( unit );


		if ( pData && pData->nRespawnSpawnClass != INVALID_LINK && !UnitTestFlag( unit, UNITFLAG_RESPAWN_EXACT ) )
		{			
			nMonsterExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
			DWORD dwSpawnClassEvaluateFlags = 0;
			SETBIT( dwSpawnClassEvaluateFlags, SCEF_RANDOMIZE_PICKS_BIT );

			int nPicks = SpawnClassEvaluate( pGame, 
				pLevel->m_idLevel, 
				pData->nRespawnSpawnClass,
				nMonsterExperienceLevel, 
				tSpawns, 
				NULL,
				-1,
				dwSpawnClassEvaluateFlags);
			if( nPicks > 0 )
			{
				nClass = tSpawns[0].nIndex;
			}
		}

		VECTOR vSpawn = UnitGetPosition( unit );
		VECTOR vOriginalRespawn = vSpawn;

		if( pData->fRespawnRadius > 0 )
		{
			int nTries = 0;
			while( nTries < 50 )
			{

				vSpawn.fX += pData->fRespawnRadius * 2.0f * ( RandGetFloat(pGame) - .5f);
				vSpawn.fY += pData->fRespawnRadius * 2.0f * ( RandGetFloat(pGame) - .5f);

				// still in room?
				if ( !ConvexHullPointTest( pRoom->pHull, &vSpawn ) )
					continue;

				FREE_PATH_NODE tFreePathNodes[2];
				NEAREST_NODE_SPEC tSpec;
				tSpec.fMaxDistance = 10;
				SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
				SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
				SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
				SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
				SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
				SETBIT(tSpec.dwFlags, NPN_ONE_ROOM_ONLY);
				SETBIT(tSpec.dwFlags, NPN_USE_GIVEN_ROOM);

				int nCount = RoomGetNearestPathNodes(pGame, pRoom, vSpawn, 1, tFreePathNodes, &tSpec);
				if(nCount == 1 && pRoom)
				{
					const FREE_PATH_NODE * freePathNode = &tFreePathNodes[0];
					if( freePathNode )
					{
						vSpawn = RoomPathNodeGetWorldPosition(pGame, freePathNode->pRoom, freePathNode->pNode);
						vSpawn.fZ = unit->vPosition.fZ;
						vSpawn.fZ = VerifyFloat( vSpawn.fZ, 0.1f );
						break;
					}
					else
					{
						continue;
					}
				}
				else
				{
					continue;
				}

			}
			if( nTries >= 50 )
			{
				vSpawn = UnitGetPosition( unit );
			}
		}

		VECTOR Normal( 0, 0, -1 );
		VECTOR Start = vSpawn;

		if( pLevel )
		{
			Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
			float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
			VECTOR vCollisionNormal;
			int Material;
			float fCollideDistance = LevelLineCollideLen( pGame, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
			if (fCollideDistance < fDist &&
				Material != PROP_MATERIAL &&
				Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
				Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
			{
				vSpawn.fZ = Start.fZ - fCollideDistance;
			}
		}

		float fAngle = TWOxPI * RandGetFloat(pGame);
		VECTOR vDirection;
		VectorDirectionFromAngleRadians( vDirection, fAngle );
		//create the object now
		OBJECT_SPEC objectSpec;
		objectSpec.nLevel = nMonsterExperienceLevel;
		objectSpec.nClass = nClass;
		objectSpec.pRoom = UnitGetRoom( unit );
		objectSpec.vPosition= vSpawn;
		objectSpec.pvFaceDirection = &vDirection;

		UNIT* pObject = s_ObjectSpawn( pGame, objectSpec ); //creating object
		if( pObject )
		{

			UnitSetFlag(pObject, UNITFLAG_SHOULD_RESPAWN);
			if( UnitTestFlag( unit, UNITFLAG_RESPAWN_EXACT ) )
			{
				UnitSetFlag(pObject, UNITFLAG_RESPAWN_EXACT);
			}
			UnitSetStatVector(pObject, STATS_AI_ANCHOR_POINT_X, 0, vOriginalRespawn );
			if( QuestUnitIsQuestUnit( unit ) )
			{
				const QUEST_TASK_DEFINITION_TUGBOAT*  pQuestTask = QuestUnitGetTask( unit );
				int nIndex = QuestUnitGetIndex( unit );
				if( pQuestTask )
				{
					s_SetUnitAsQuestUnit( pObject, NULL, pQuestTask, nIndex );
				}
			}
		}

	
		// throw away the respawner - worthless now!
		UnitDie(unit, NULL);
		UnitFree( unit, UFF_SEND_TO_CLIENTS );
		return TRUE;
	}
	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_UnitGetModelIdInventoryInspect(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	const UNIT_DATA * pUnitData = UnitGetData(unit);
	if(pUnitData && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_DRAW_USING_CUT_UP_WARDROBE))
	{
		return unit->pGfx->nModelIdInventoryInspect; // models and appearances share the same id
	}
	return unit->pGfx->nModelIdInventory;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ControlUnitLoadInventoryUnitData(
	UNIT * pUnit)
{
	GAME * pGame = UnitGetGame(pUnit);
	LEVEL * pUnitLevel = UnitGetLevel(pUnit);
	BOOL bIsTown = pUnitLevel ? LevelIsTown(pUnitLevel) : FALSE;
	UNIT * pItemCurr = UnitInventoryIterate( pUnit, NULL);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pUnit, pItemCurr );

		int nItemLocation;
		int nItemLocationX;
		int nItemLocationY;
		if (ItemIsEquipped(pItemCurr, pUnit) && UnitGetInventoryLocation(pItemCurr, &nItemLocation, &nItemLocationX, &nItemLocationY))
		{
			INVLOC_HEADER tLocInfo;
			if (UnitInventoryGetLocInfo(pUnit, nItemLocation, &tLocInfo))
			{
				if (InvLocTestFlag(&tLocInfo, INVLOCFLAG_WARDROBE))
				{
					int wardrobe = ItemGetWardrobe( pGame, UnitGetClass(pItemCurr), 
						UnitGetLookGroup(pItemCurr) );
					const UNIT_DATA * pItemCurrData = UnitGetData( pItemCurr );
					ASSERT( pItemCurrData );
					if ( pItemCurrData && UnitDataTestFlag( pItemCurrData, UNIT_DATA_FLAG_DONT_ADD_WARDROBE_LAYER ) )
						wardrobe = INVALID_ID; // we use this for dye kits

					if (UnitIsA(pItemCurr, UNITTYPE_HELM) && 
						UnitGetStat( pUnit, STATS_CHAR_OPTION_HIDE_HELMET ) != 0)
					{
						wardrobe = INVALID_ID;
					}

					if (wardrobe != INVALID_ID)
					{
						EXCELTABLE eTable = ExcelGetDatatableByGenus(UnitGetGenus(pItemCurr));
						if(eTable == DATATABLE_ITEMS)
						{
							if(bIsTown)
							{
								UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_TOWN_YOU);
							}
							else
							{
								UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_WARDROBE_YOU);
							}
						}
					}
				}
			}
		}
		pItemCurr = pItemNext;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetLostAtTime(
	UNIT *pUnit,
	time_t utcLostAt)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// set stat
	DWORD dwHigh = 0;
	DWORD dwLow = 0;
	UTCTIME_SPLIT( utcLostAt, dwHigh, dwLow );	
	UnitSetStat( pUnit, STATS_UNIT_LOST_TIME_UTC, SPLIT_VALUE_64_PARAM_LOW, dwLow );
	UnitSetStat( pUnit, STATS_UNIT_LOST_TIME_UTC, SPLIT_VALUE_64_PARAM_HIGH, dwHigh );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
time_t UnitGetLostAtTime(
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	
	// get time from stat and combine
	DWORD dwLow = UnitGetStat( pUnit, STATS_UNIT_LOST_TIME_UTC, SPLIT_VALUE_64_PARAM_LOW );
	DWORD dwHigh = UnitGetStat( pUnit, STATS_UNIT_LOST_TIME_UTC, SPLIT_VALUE_64_PARAM_HIGH );
	
	// combine
	time_t utcLostAt = UTCTIME_CREATE( dwLow, dwHigh );
	
	// result
	return utcLostAt;

}

//----------------------------------------------------------------------------
// This function will return TRUE if the flag passed in is set on ANY
// of the units contained in the hierarchy starting at unit and 
// traversing upwards to the root
//----------------------------------------------------------------------------
BOOL UnitTestFlagHierarchy(
	UNIT *pUnit,
	UNIT_HIERARCHY eHiearchy,
	int nUnitFlag)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// test this unit
	if (UnitTestFlag( pUnit, nUnitFlag ))
	{
		return TRUE;
	}

	// test the parent
	UNIT *pParent = NULL;
	if (eHiearchy == UH_OWNER)
	{
		UNITID idParent = UnitGetOwnerId( pUnit );
		if (idParent != INVALID_ID)
		{
			GAME *pGame = UnitGetGame( pUnit );		
			pParent = UnitGetById( pGame, idParent );
		}
	}
	else if (eHiearchy == UH_CONTAINER)
	{
		pParent = UnitGetContainer( pUnit );
	}

	// do the same test on the parent (if any)
	if (pParent != NULL && pParent != pUnit)
	{
		return UnitTestFlagHierarchy( pParent, eHiearchy, nUnitFlag );
	}

	return FALSE;		

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
static BOOL sArrayContainsItem(
	SIMPLE_DYNAMIC_ARRAY<T>& arr,
	T item)
{
	for (UINT32 i = 0; i < arr.Count(); i++) {
		if (arr[i] == item) {
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitTypeGetAllEquivTypes(
	int nUnitType,
	SIMPLE_DYNAMIC_ARRAY<int>& nEquivTypes)
{
	ArrayClear(nEquivTypes);

	ArrayAddItem(nEquivTypes, nUnitType);
	for (UINT32 i = 0; i < nEquivTypes.Count(); i++) {
		const UNITTYPE_DATA* pUnitTypeData = UnitTypeGetData(nEquivTypes[i]);
		ASSERT_RETFALSE(pUnitTypeData != NULL);

		for (UINT32 j = 0; j < MAX_UNITTYPE_EQUIV; j++) {
			if (pUnitTypeData->nTypeEquiv[j] != EXCEL_LINK_INVALID &&
				!sArrayContainsItem<int>(nEquivTypes, pUnitTypeData->nTypeEquiv[j])) {
				ArrayAddItem(nEquivTypes, pUnitTypeData->nTypeEquiv[j]);
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int UnitDataGetIconTextureID(
	const UNIT_DATA * pUnitData )
{
	ASSERT_RETINVALID( pUnitData );
	return pUnitData->nIconTextureId;
}


//----------------------------------------------------------------------------
//returns the level area ID that the unit will warp the player to
//----------------------------------------------------------------------------
int UnitGetWarpToLevelAreaID( UNIT *pUnit )
{
	ASSERT_RETINVALID( pUnit );
	if( pUnit->pUnitData->nLinkToLevelArea == INVALID_ID )
		return INVALID_ID;
	//this levelarea is a random area so we'll use our position to create the link name
	VECTOR vPos = UnitGetPosition( pUnit );
	return MYTHOS_LEVELAREAS::LevelAreaGenerateLevelIDByPos( pUnit->pUnitData->nLinkToLevelArea, (int)vPos.fX, (int)vPos.fY );
}

//----------------------------------------------------------------------------
//returns the depth
//----------------------------------------------------------------------------
int UnitGetWarpToDepth( UNIT *pUnit )
{
	ASSERT_RETINVALID( pUnit );
	return pUnit->pUnitData->nLinkToLevelAreaFloor;


}