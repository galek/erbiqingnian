//----------------------------------------------------------------------------
// FILE: 
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "dungeon.h"
#include "excel.h"
#include "globalindex.h"
#include "level.h"
#include "objects.h"
#include "picker.h"
#include "Quest.h"
#include "room_layout.h"
#include "room_path.h"
#include "s_message.h"
#include "s_quests.h"
#include "states.h"
#include "unit_priv.h"
#include "weather.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SUBLEVEL_DEFINITION *SubLevelGetDefinition(
	int nSubLevelDef)
{
	return (const SUBLEVEL_DEFINITION *)ExcelGetData( NULL, DATATABLE_SUBLEVEL, nSubLevelDef );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SUBLEVEL_DEFINITION *SubLevelGetDefinition(
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETNULL( pSubLevel, "Expected sub level" );
	return SubLevelGetDefinition( pSubLevel->nSubLevelDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *SubLevelGetDevName( 
	const SUBLEVEL *pSubLevel)
{
	static char *szUnknown = "Unknown";
	if (pSubLevel == NULL)
	{
		return szUnknown;
	}
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	return ptSubLevelDef->szName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *SubLevelGetDevName( 
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	return SubLevelGetDevName( pSubLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSubLevelFindEntrance(
	SUBLEVEL *pSubLevel)
{
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETURN( pSubLevelDef, "Expected sublevel definition" );
	
	int nObjectClass = pSubLevelDef->nObjectEntrance;
	if (nObjectClass != INVALID_LINK)
	{

		// we search for entrances to this sublevel in the default sublevel of the level
		LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
		ASSERTX_RETURN( pLevel, "Expected level" );
		SUBLEVEL *pSubLevelDefault = LevelGetPrimarySubLevel( pLevel );
		ASSERTX_RETURN( pSubLevelDefault, "Expected default sublevel" );

		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT *pEntrance = s_SubLevelFindFirstUnitOf( pSubLevelDefault, eTargetTypes, GENUS_OBJECT, nObjectClass );
		if (pEntrance)
		{
			s_SubLevelSetDoorway( pSubLevel, pEntrance, SD_ENTRANCE );
		}
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSubLevelFindExit(
	SUBLEVEL *pSubLevel)
{
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETURN( pSubLevelDef, "Expected sublevel definition" );
	
	if (pSubLevelDef->nObjectExit != INVALID_LINK)
	{

		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT *pExit = s_SubLevelFindFirstUnitOf( pSubLevel, eTargetTypes, GENUS_OBJECT, pSubLevelDef->nObjectExit );
		if (pExit)
		{
			s_SubLevelSetDoorway( pSubLevel, pExit, SD_EXIT );
		}
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelSetDoorway( 
	SUBLEVEL *pSubLevel, 
	UNIT *pDoorway,
	SUBLEVEL_DOORWAY_TYPE eDoorway,
	BOOL bInitializing /*= FALSE*/)
{
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );	
	SUBLEVEL_DOORWAY &tDoorway = pSubLevel->tDoorway[ eDoorway ];
	
	if (pDoorway)
	{
	
		// save doorway information
		tDoorway.idDoorway = UnitGetId( pDoorway );
		tDoorway.idRoom = UnitGetRoomId( pDoorway );
		tDoorway.vPosition = UnitGetPosition( pDoorway );
		tDoorway.vDirection = UnitGetFaceDirection( pDoorway, FALSE );		
		
		// room with sublevel doorways are no longer candidates for random adventure placement
		ROOM *pRoomDoorway = UnitGetRoom( pDoorway );
		ASSERTX_RETURN( pRoomDoorway, "Expected room for sublevel doorway" );
		RoomSetFlag( pRoomDoorway, ROOM_NO_ADVENTURES_BIT );
		
	}
	else
	{
	
		// if there was a room here before, it can have adventures again
		if (bInitializing == FALSE && tDoorway.idRoom != INVALID_ID)
		{	
			GAME *pGame = SubLevelGetGame( pSubLevel );
			ROOM *pRoomDoorwayLast = RoomGetByID( pGame, tDoorway.idRoom );
			if (pRoomDoorwayLast)
			{
				RoomSetFlag( pRoomDoorwayLast, ROOM_NO_ADVENTURES_BIT, FALSE );
			}
		}

		// clear out doorway information		
		tDoorway.idDoorway = INVALID_ID;
		tDoorway.idRoom = INVALID_ID;
		tDoorway.vDirection = cgvNone;
		tDoorway.vDirection = cgvNone;
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SUBLEVEL_DOORWAY *s_SubLevelGetDoorway(
	SUBLEVEL *pSubLevel,
	SUBLEVEL_DOORWAY_TYPE eDoorway)
{
	ASSERTX_RETNULL( pSubLevel, "Expected sublevel" );
	return &pSubLevel->tDoorway[ eDoorway ];
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSubLevelSetDefaults( 
	SUBLEVEL *pSubLevel)
{
	ASSERTX_RETURN( pSubLevel, "Expected sub level" );
	pSubLevel->idSubLevel = INVALID_ID;
	pSubLevel->nEngineRegion = -1;
	pSubLevel->nSubLevelDef = INVALID_LINK;
	pSubLevel->pLevel = NULL;
	pSubLevel->dwSubLevelStatus = 0;
	pSubLevel->vPosition = cgvNone;
	pSubLevel->idEntranceAux = INVALID_ID;
	pSubLevel->pWeather = NULL;
	for (int i = 0; i < NUM_DRLG_SPAWN_TYPE; ++i)
	{
		SPAWN_TRACKER *pTracker = &pSubLevel->tSpawnTracker[ i ];
		pTracker->nSpawnedCount = 0;
		pTracker->nDesiredCount = 0;
	}
	pSubLevel->nMonsterCount = 0;
	pSubLevel->nDesiredMonsters = 0;
	pSubLevel->nRoomCount = 0;

	for (int i = 0; i < SD_NUM_DOORWAYS; ++i)
	{
		SUBLEVEL_DOORWAY_TYPE eDoorway = (SUBLEVEL_DOORWAY_TYPE)i;
		s_SubLevelSetDoorway( pSubLevel, NULL, eDoorway, TRUE );
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SubLevelInit(
	SUBLEVEL *pSubLevel,
	SUBLEVELID idSubLevel,
	int nSubLevelDef,
	LEVEL *pLevel)
{
	ASSERTX_RETURN( pSubLevel, "Expected sub level" );	
	ASSERTX_RETURN( idSubLevel != INVALID_ID, "Expected sublevel id" );
	ASSERTX_RETURN( nSubLevelDef != INVALID_LINK, "Expected sublevel def index" );
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	ASSERTX_RETURN( pGame, "Expected game" );
	
	// init sublevel instance values
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( nSubLevelDef );
	ASSERTX_RETURN( ptSubLevelDef, "Expected sublevel definition" );
	pSubLevel->nSubLevelDef = nSubLevelDef;
	pSubLevel->idSubLevel = idSubLevel;
	pSubLevel->nEngineRegion = (int)idSubLevel;
	pSubLevel->pLevel = pLevel;
	pSubLevel->vPosition = ptSubLevelDef->vDefaultPosition;
	if(ptSubLevelDef->nWeatherSet >= 0)
	{
		pSubLevel->pWeather = WeatherSelect(pGame, ptSubLevelDef->nWeatherSet, DungeonGetSeed(pGame));
	}

	// init name, easy to see in debugger
	const DRLG_DEFINITION *ptDRLGDef = NULL;
	if (ptSubLevelDef->nDRLGDef != INVALID_LINK )
	{
		ptDRLGDef = DRLGDefinitionGet( ptSubLevelDef->nDRLGDef );
	}
	PStrPrintf(pSubLevel->szName, MAX_SUBLEVEL_NAME, "%s [%d] (%s)", ptSubLevelDef->szName, pSubLevel->idSubLevel, ptDRLGDef ? ptDRLGDef->pszName : "No DRLG");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME *SubLevelGetGame( 
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETNULL( pSubLevel, "Expected sublevel" );
	LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
	ASSERTX_RETNULL( pLevel, "Expected level" );
	return LevelGetGame( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SubLevelSetPosition(
	SUBLEVEL *pSubLevel,
	const VECTOR &vPosition)
{
	ASSERTX_RETURN( pSubLevel, "Expected sub level" );
	pSubLevel->vPosition = vPosition;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *SubLevelGetLevel(
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETNULL( pSubLevel, "Expected sub level" );
	ASSERTX_RETNULL( pSubLevel->pLevel, "Sublevel has no level" );
	return pSubLevel->pLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SubLevelGetDRLG(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	if (ptSubLevelDef && ptSubLevelDef->nDRLGDef != INVALID_LINK)
	{
		return ptSubLevelDef->nDRLGDef;
	}
	else
	{
		return pLevel->m_nDRLGDef;
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR SubLevelGetPosition(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	ASSERTX_RETINVALID( pSubLevel, "Expected sub level" );
	return pSubLevel->vPosition;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID SubLevelAdd(
	LEVEL *pLevel,
	int nSubLevelDef)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( nSubLevelDef != INVALID_LINK, "Expected sub level def index" );
	
	// for now you cannot add a sublevel of the same type to a level, if we want to
	// do that, we will need a way to specify portals to link to specific sub-level
	// areas instead of just refering to the type of sublevel they link to
	for (int i = 0; i < LevelGetNumSubLevels( pLevel ); ++i)
	{
		const SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		ASSERTX_RETVAL( pSubLevel->nSubLevelDef != nSubLevelDef, SubLevelGetId( pSubLevel ), "Level already has a sub level of that type" );
	}
		
	// allocate new sublevel id
	ASSERTX_RETNULL( pLevel->m_nNumSubLevels <= MAX_SUBLEVELS - 1, "Too many sublevels in level" );	
	SUBLEVELID idSubLevel = pLevel->m_nNumSubLevels++;
	
	// get sublevel
	SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ idSubLevel ];
	ASSERT_RETINVALID(pSubLevel);
	SubLevelInit( pSubLevel, idSubLevel, nSubLevelDef, pLevel );
	
	// return new sublevel	
	return pSubLevel->idSubLevel;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID SubLevelAdd(
	LEVEL *pLevel, 
	const SUBLEVEL_DESC *pSubLevelDesc)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( pSubLevelDesc, "Expected sub level desc" );	
	
	// add level
	SUBLEVELID idSubLevel = SubLevelAdd( pLevel, pSubLevelDesc->nSubLevelDef );
	
	// must be the same
	ASSERTX_RETINVALID( idSubLevel == pSubLevelDesc->idSubLevel, "Expected sublevel id match" );

	// assign rest of spec
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	ASSERTX_RETVAL(pSubLevel, idSubLevel, "Expected sub level");
	pSubLevel->vPosition = pSubLevelDesc->vPosition;

	return idSubLevel;
			
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *SubLevelGetById(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( idSubLevel != INVALID_ID, "Expected sublevel id" );	
	
	for (int i = 0; i < pLevel->m_nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		if (pSubLevel && pSubLevel->idSubLevel == idSubLevel)
		{
			return pSubLevel;
		}
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *SubLevelGetByIndex(
	LEVEL *pLevel,
	int nIndex)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( nIndex >= 0 && nIndex < LevelGetNumSubLevels( pLevel ), "Invalid sublevel index" );
	return &pLevel->m_SubLevels[ nIndex ];
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID SubLevelGetId(
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETINVALID( pSubLevel, "Expected sublevel" );
	ASSERTX_RETINVALID( pSubLevel->idSubLevel != INVALID_ID, "Expected sublevel id" );	
	return pSubLevel->idSubLevel;
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SubLevelsIterate(
	LEVEL *pLevel,
	PFN_SUBLEVEL_ITERATE pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );	
	
	for (int i = 0; i < pLevel->m_nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		pfnCallback( pLevel, pSubLevel, pCallbackData );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SubLevelClearAll( 
	LEVEL *pLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	for (int i = 0; i < MAX_SUBLEVELS; ++i)
	{
		SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		sSubLevelSetDefaults( pSubLevel );
	}
	pLevel->m_nNumSubLevels = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID SubLevelFindFirstOfType( 
	LEVEL *pLevel, 
	int nSubLevelDefDest)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( nSubLevelDefDest != INVALID_LINK, "Expected sub-level def" );
	SUBLEVELID idSubLevel = INVALID_ID;
	
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0; i < nNumSubLevels; ++i)
	{
		const SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		if (pSubLevel && pSubLevel->nSubLevelDef == nSubLevelDefDest)
		{
			// error checking
			ASSERTX( idSubLevel == INVALID_ID, "More than one sublevel matching the requested type was found on level" );
			
			// save this sublevel
			idSubLevel = SubLevelGetId( pSubLevel );
			
		}
	}
	return idSubLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL_TYPE SubLevelGetType(
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETVAL( pSubLevel, ST_INVALID, "Expected sub level" );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETVAL( ptSubLevelDef, ST_INVALID, "Expected sub level def" );
	return ptSubLevelDef->eType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SubLevelAllowsTownPortals( 
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETFALSE( ptSubLevelDef, "Expected sub level def" );
	return ptSubLevelDef->bAllowTownPortals;		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_SubLevelGetSpawnClass(
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETINVALID( ptSubLevelDef, "Expected sub level def" );
	return ptSubLevelDef->nSpawnClass;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SubLevelOverrideLevelSpawns(
	LEVEL *pLevel, 
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	ASSERTX_RETINVALID( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETINVALID( ptSubLevelDef, "Expected sub level def" );
	return ptSubLevelDef->bOverrideLevelSpawns;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelPopulate(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel,
	UNIT *pActivator)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( idSubLevel != INVALID_ID, "Expected sub level id" );	
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	ASSERT_RETURN(pSubLevel);

	// if sublevel is populated already, do nothing
	if (SubLevelTestStatus( pSubLevel, SLS_POPULATED_BIT ))
	{
		return;
	}

	// sublevel is now populated
	SubLevelSetStatus( pSubLevel, SLS_POPULATED_BIT );
	
	// tell all rooms in the sublevel that the sublevel is becoming active ... this is
	// a spawn phase where we can populate units across an entire sublevel before
	// runing the next phase of population
	pSubLevel->nRoomCount = 0;
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if (RoomGetSubLevelID( pRoom ) == idSubLevel)
		{
			s_RoomSubLevelPopulated( pRoom, pActivator );
			++pSubLevel->nRoomCount;
		}
	}
#else
	REF( pLevel );
	REF( idSubLevel );
	REF( pActivator );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SubLevelAllowsMonsterDistribution(
	LEVEL *pLevel,
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( idSubLevel != INVALID_ID, "Expected sub level id" );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETFALSE( ptSubLevelDef, "Expected sub level def" );
	return ptSubLevelDef->bAllowMonsterDistribution;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SubLevelSetStatus(
	SUBLEVEL *pSubLevel,
	SUB_LEVEL_STATUS eStatus,
	BOOL bEnable /*= TRUE*/)
{
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	DWORD dwStatusOld = pSubLevel->dwSubLevelStatus;
	
	// set new status
	SETBIT( pSubLevel->dwSubLevelStatus, eStatus, bEnable );
	
	// if status changed, inform clients
	GAME *pGame = SubLevelGetGame( pSubLevel );
	ASSERTX_RETURN( pGame, "Expected game" );
	if (IS_SERVER( pGame ))
	{
	
		// only send if status has changed		
		if (dwStatusOld != pSubLevel->dwSubLevelStatus)
		{
			s_SubLevelSendStatus( pSubLevel, NULL );			
		}

	}
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SubLevelTestStatus(
	SUBLEVEL *pSubLevel,
	SUB_LEVEL_STATUS eStatus)
{
	ASSERTX_RETFALSE( pSubLevel, "Expected sublevel" );
	return TESTBIT( pSubLevel->dwSubLevelStatus, eStatus );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelSendStatus( 
	SUBLEVEL *pSubLevel, 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	
	// setup message
	MSG_SCMD_SUBLEVEL_STATUS tMessage;
	tMessage.idLevel = LevelGetID( pLevel );
	tMessage.idSubLevel = SubLevelGetId( pSubLevel );
	tMessage.dwSubLevelStatus = pSubLevel->dwSubLevelStatus;
	
	// send message
	if (pUnit == NULL)
	{
		s_SendMessageToAllInLevel( pGame, pLevel, SCMD_SUBLEVEL_STATUS, &tMessage );
	}
	else
	{
		ASSERTX_RETURN( pUnit, "Expected unit" );
		ASSERTV_RETURN( UnitIsPlayer( pUnit ), "Expected player, got '%s'", UnitGetDevName( pUnit ) );	
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		ASSERTX_RETURN( idClient != INVALID_GAMECLIENTID, "Invalid client id" );
		s_SendMessage( pGame, idClient, SCMD_SUBLEVEL_STATUS, &tMessage );
	}
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sIsPlayer(
	UNIT *pUnit,
	void *pCallbackData)
{
	if (UnitIsA( pUnit, UNITTYPE_PLAYER ))
	{
		BOOL *bAnyPlayer = (BOOL *)pCallbackData;
		*bAnyPlayer = TRUE;
		return RIU_STOP;
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sIsMonster(
	UNIT *pUnit,
	void *pCallbackData)
{
	if (UnitIsA( pUnit, UNITTYPE_MONSTER ))
	{
		return RIU_CONTINUE;		
	}
	else
	{
		BOOL *bAllMonsters = (BOOL *)pCallbackData;	
		*bAllMonsters = FALSE;	
		return RIU_STOP;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomCanBePicked(
	GAME *pGame,
	SUBLEVEL *pSubLevel,
	ROOM *pRoom,
	DWORD dwRandomRoomFlags,
	PFN_RANDOM_ROOM_FILTER pfnFilter,
	void *pFilterCallbackData)	
{

	// must be in sublevel indicated
	if (RoomGetSubLevel( pRoom ) != pSubLevel)
	{
		return FALSE;
	}
				
	if (TESTBIT( dwRandomRoomFlags, RRF_SPAWN_POINTS_BIT ))
	{
		if (pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].nCount <= 0)
		{
			return FALSE;
		}
	}

	if (TESTBIT( dwRandomRoomFlags, RRF_PATH_NODES_BIT ))
	{
		if (!pRoom->pPathDef ||
			!pRoom->pPathDef->nPathNodeSetCount || !pRoom->pPathDef->pPathNodeSets ||
			!pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].nPathNodeCount ||
			!pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes)
		{
			return FALSE;
		}
	}

	if (TESTBIT( dwRandomRoomFlags, RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT ))
	{
		if (RoomTestFlag( pRoom, ROOM_HAS_NO_SPAWN_NODES_BIT ))
		{
			return FALSE;
		}
	}
	
	if (TESTBIT( dwRandomRoomFlags, RRF_ACTIVE_BIT ))
	{
		if (RoomIsActive(pRoom) == FALSE)
		{
			return FALSE;
		}
	}
	
	if (TESTBIT( dwRandomRoomFlags, RRF_MUST_ALLOW_MONSTER_SPAWN_BIT ))
	{
		if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
		{
			return FALSE;
		}
	}

	// check for room must have or not have certain unit types
	if (TESTBIT( dwRandomRoomFlags, RRF_PLAYERLESS_BIT ))
	{
		TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
		BOOL bAnyPlayer = FALSE;
		RoomIterateUnits( pRoom, eTargetTypes, sIsPlayer, &bAnyPlayer );
		if (bAnyPlayer == TRUE)
		{
			return FALSE;
		}
	}

	if (TESTBIT( dwRandomRoomFlags, RRF_HAS_MONSTERS_BIT ))
	{
		TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
		BOOL bAllMonsters = TRUE;
		RoomIterateUnits( pRoom, eTargetTypes, sIsMonster, &bAllMonsters );
		if (bAllMonsters == FALSE)
		{
			return FALSE;
		}	
	}

	// check filter (if present)
	if (pfnFilter)
	{
		if (pfnFilter( pRoom, pFilterCallbackData ) == FALSE)
		{
			return FALSE;
		}
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_SubLevelGetRandomRooms(
	SUBLEVEL *pSubLevel,
	ROOM **pResultRooms,
	int nNumResultRooms,
	DWORD dwRandomRoomFlags,	// see RANDOM_ROOM_FLAGS
	PFN_RANDOM_ROOM_FILTER pfnFilter,
	void *pFilterCallbackData)
{
	ASSERTX_RETNULL( pResultRooms && nNumResultRooms > 0, "Expected result rooms array of size > 0" );
	ASSERTX_RETNULL( nNumResultRooms <= MAX_RANDOM_ROOMS, "too many random rooms requested, increase MAX_RANDOM_ROOMS" );
	ASSERTX_RETNULL( pSubLevel, "Expected sub level" );
	GAME *pGame = SubLevelGetGame( pSubLevel );
	ASSERTX_RETNULL( pGame, "Expected game" );

	// we will hold the rooms we can pick here
	ROOM *pRoomsAvailable[ MAX_RANDOM_ROOMS ];
	int nAvailableRoomCount = 0;
	BOOL bTooManyRooms = FALSE;

	// get level
	LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
	ASSERTX_RETNULL( pLevel, "Expected level in sub level" );

	// add rooms to the picker		
	PickerStart( pGame, picker );	
	for (ROOM *room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ))
	{
		if (sRoomCanBePicked( pGame, pSubLevel, room, dwRandomRoomFlags, pfnFilter, pFilterCallbackData ))
		{
			if (nAvailableRoomCount < MAX_RANDOM_ROOMS)
			{
				// save available room			
				pRoomsAvailable[ nAvailableRoomCount ] = room;			

				// add to picker
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, nAvailableRoomCount, 1));

				nAvailableRoomCount++;

			}
			else
			{
				bTooManyRooms = TRUE;
			}

		}

	}

	// warn of too many rooms
	WARNX( bTooManyRooms == FALSE, "Too many rooms in level to pick random room, increase MAX_RANDOM_ROOMS" );

	// if we had rooms that passed the pick filter, pick some
	int nNumRoomsToReturn = MIN(nAvailableRoomCount, nNumResultRooms);
	for (int i = 0; i < nNumRoomsToReturn; ++i )
	{
		int nPick = PickerChooseRemove( pGame );
		ASSERTX_RETVAL( nPick >= 0 && nPick < nAvailableRoomCount, i, "Invalid room picked" );

		ROOM *roomPicked = pRoomsAvailable[ nPick ];
		ASSERTX_RETVAL( roomPicked, i, "Invalid room pointer for picked room" );
		pResultRooms[i] = roomPicked;
	}

	return nNumRoomsToReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM *s_SubLevelGetRandomRoom(
	SUBLEVEL *pSubLevel,
	DWORD dwRandomRoomFlags,	// see RANDOM_ROOM_FLAGS
	PFN_RANDOM_ROOM_FILTER pfnFilter,
	void *pFilterCallbackData)
{
	ROOM *pResultRoom = NULL;
	s_SubLevelGetRandomRooms( pSubLevel, &pResultRoom, 1, dwRandomRoomFlags, pfnFilter, pFilterCallbackData );
	return pResultRoom;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_SubLevelFindFirstUnitOf( 
	const SUBLEVEL *pSubLevel, 
	TARGET_TYPE *peTargetTypes, 
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETNULL( pSubLevel, "Expected sublevel" );
	
	// get the level
	LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
	
		// must be in same sublevel
		if (RoomGetSubLevel( pRoom ) == pSubLevel)
		{
		
			// find unit
			UNIT *pUnit = RoomFindFirstUnitOf( pRoom, peTargetTypes, eGenus, nClass );
			if (pUnit)
			{
				return pUnit;
			}
			
		}
		
	}

	return NULL;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_SubLevelHasALiveMonster( 
	const SUBLEVEL *pSubLevel)
{
	LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		if (RoomGetSubLevel( room ) == pSubLevel)
			for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
			{
				if ( UnitIsA( test, UNITTYPE_MONSTER ) &&
					 !IsUnitDeadOrDying( test ) )
					 return TRUE;
			}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
enum SUBLEVEL_APPLY_STATE
{
	SAS_INVALID,
	SAS_ENABLE,
	SAS_CLEAR
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplySubLevelStateInsideOf( 
	UNIT *pUnit, 
	SUBLEVEL *pSubLevel, 
	SUBLEVEL_APPLY_STATE eApplyState)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// must have sublevel
	if (pSubLevel)
	{
		const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
		if (pSubLevelDef->nStateWhenInside != INVALID_LINK)
		{
		
			// apply or clear the state
			if (eApplyState == SAS_ENABLE)
			{
				s_StateSet( pUnit, pUnit, pSubLevelDef->nStateWhenInside, 0 );
			}
			else if (eApplyState == SAS_CLEAR)
			{
				s_StateClear( pUnit, UnitGetId( pUnit ), pSubLevelDef->nStateWhenInside, 0 );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelTransition(
	UNIT *pUnit,
	SUBLEVELID idSubLevelOld,		// may be invalid id
	ROOM *pRoomOld,					// may be NULL
	SUBLEVELID idSubLevelNew,
	ROOM *pRoomNew)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( idSubLevelNew != INVALID_ID, "Expected sublevel new" );
	ASSERTX_RETURN( pRoomNew != NULL, "Expected new room" );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	
	// get sublevels	
	SUBLEVEL *pSubLevelOld = idSubLevelOld != INVALID_ID ? SubLevelGetById( pLevel, idSubLevelOld ) : NULL;
	SUBLEVEL *pSubLevelNew = SubLevelGetById( pLevel, idSubLevelNew );	
	
	// do quest event
	s_QuestEventTransitionSubLevel( pUnit, pLevel, idSubLevelOld, idSubLevelNew, pRoomOld );

	// do weather effects
	ASSERTX_RETURN( pSubLevelNew, "Expected sublevel new" );
	if(pSubLevelNew->pWeather)
	{
		WeatherApplyStateToUnit(pUnit, pSubLevelNew->pWeather);
	}
	else
	{
		WeatherApplyStateToUnit(pUnit, pLevel->m_pWeather);
	}

	// some sublevels apply states to units when they are inside of them
	sApplySubLevelStateInsideOf( pUnit, pSubLevelOld, SAS_CLEAR );
	sApplySubLevelStateInsideOf( pUnit, pSubLevelNew, SAS_ENABLE );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelSetupDoorways(
	SUBLEVEL *pSubLevel)
{		
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETURN( ptSubLevelDef, "Expected sublevel definition" );

	// entrance logic
	if (ptSubLevelDef->nObjectEntrance != INVALID_LINK)
	{
	
		if (ptSubLevelDef->bAutoCreateEntrance == TRUE)
		{
			// auto create entrance for sublevel		
			LEVEL *pLevel = SubLevelGetLevel( pSubLevel );
			s_LevelCreateSubLevelEntrance( pLevel, pSubLevel );
		}
		
		// if there is still no entrance, try to find one
		const SUBLEVEL_DOORWAY *pDoorwayEntrance = s_SubLevelGetDoorway( pSubLevel, SD_ENTRANCE );
		if (pDoorwayEntrance == NULL || pDoorwayEntrance->idDoorway == INVALID_ID)
		{
			s_sSubLevelFindEntrance( pSubLevel );		
		}
		
	}
			
	// gather up sublevel exit
	s_sSubLevelFindExit( pSubLevel );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SubLevelWarpCreated(
	UNIT *pWarp)
{
	ASSERTX_RETURN( pWarp, "Expected warp" );
	ASSERTX_RETURN( UnitIsA( pWarp, UNITTYPE_OBJECT ), "Expected object" );	
	LEVEL *pLevel = UnitGetLevel( pWarp );
	ASSERTX_RETURN( pLevel, "Expected level" );

	// is this an entrance or exit object for any sublevel inside this level
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0; i < nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = LevelGetSubLevelByIndex( pLevel, i );
		ASSERTX_CONTINUE( pSubLevel, "Expected sublevel" );
		
		// check for object
		int nObjectClass = UnitGetClass( pWarp );
		const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
		ASSERTX_CONTINUE( pSubLevelDef, "Expected sublevel definition" );
		if (pSubLevelDef->nObjectEntrance == nObjectClass)
		{
			s_SubLevelSetDoorway( pSubLevel, pWarp, SD_ENTRANCE );
		}
		else if (pSubLevelDef->nObjectExit == nObjectClass)
		{
			s_SubLevelSetDoorway( pSubLevel, pWarp, SD_EXIT );
		}
		
	}
		
}
