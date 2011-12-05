//----------------------------------------------------------------------------
// movement.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "ai_priv.h"
#include "console.h"
#include "gamelist.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "movement.h"
#include "player.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "items.h"
#include "ai.h"
#include "unitmodes.h"
#include "ptime.h"
#include "gameunits.h"
#include "astar.h"
#include "performance.h"
#include "level.h"
#include "s_message.h"
#include "perfhier.h"
#include "path.h"
#include "s_message.h"
#include "room_path.h"
#include "missiles.h"
#include "c_message.h"
#include "s_message.h"
#include "states.h"
#include "skills.h"
#include "objects.h"
#include "picker.h"
#ifdef _DEBUG
#include "c_particles.h"
#include "e_primitive.h"
#endif
#if ISVERSION(SERVER_VERSION)
#include "movement.cpp.tmh"
#endif

#if ISVERSION(SERVER_VERSION)
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#endif

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL drbRESERVE_NODES	= TRUE;
//----------------------------------------------------------------------------

#ifdef DRB_SUPER_TRACKER

#define DRB_SUPERTRACKER_HASH_SIZE		512		// must be power of 2
#define DRB_SUPERTRACKER_DEPTH			64		// must be power of 2

#define DRB_SUPERTRACKER_HASH_MASK		(DRB_SUPERTRACKER_HASH_SIZE-1)
#define DRB_SUPERTRACKER_DEPTH_MASK		(DRB_SUPERTRACKER_DEPTH-1)

#define DRB_SUPERTRACKER_NAMELEN		16

struct DRB_SUPERTRACKER_DATA
{
	WORD		type;
	WORD		line;
	DWORD		data[3];
	char		name[16];
};

struct DRB_SUPERTRACKER_IDX
{
	int			gid;
	int			id;
};

static DRB_SUPERTRACKER_DATA drbSTHashTbl[DRB_SUPERTRACKER_HASH_SIZE][DRB_SUPERTRACKER_DEPTH];
static int drbSTStart[DRB_SUPERTRACKER_HASH_SIZE];
static int drbSTEnd[DRB_SUPERTRACKER_HASH_SIZE];
static DRB_SUPERTRACKER_IDX drbSTIndex[DRB_SUPERTRACKER_HASH_SIZE];
BOOL drbSTOn = TRUE;
BOOL drbSTInited = FALSE;

enum STTYPES
{
	STTYPE_INVALID = -1,
	STTYPE_NONE = 0,
	STTYPE_INT,
	STTYPE_FLOAT,
	STTYPE_VECTOR,
};

static void drbInitSuperTracker()
{
	ASSERT( !drbSTInited );
	for ( int i = 0; i < DRB_SUPERTRACKER_HASH_SIZE; i++ )
	{
		drbSTIndex[i].id = INVALID_ID;
		drbSTStart[i] = 0;
		for ( int j = 0; j < DRB_SUPERTRACKER_DEPTH; j++ )
			drbSTHashTbl[i][j].type = STTYPE_INVALID;
	}
	drbSTInited = TRUE;
}

BOOL drbToggleSuperTracker()
{
	drbSTOn = !drbSTOn;
	if ( drbSTOn )
		drbInitSuperTracker();
	else
		drbSTInited = FALSE;
	return drbSTOn;
}

static int drbSTGetIndex( int gid, int id, BOOL bAdd = FALSE )
{
	int index = id & (DRB_SUPERTRACKER_HASH_SIZE-1);
	int start = index;
	while ( ( drbSTIndex[index].id != id ) && ( drbSTIndex[index].gid != gid ) && ( drbSTIndex[index].id != INVALID_ID ) )
	{
		index = ( index + 1 ) & (DRB_SUPERTRACKER_HASH_MASK);
		if ( index == start )
			return -1;
	}
	if ( drbSTIndex[index].id == INVALID_ID )
	{
		if ( bAdd )
		{
			drbSTIndex[index].gid = gid;
			drbSTIndex[index].id = id;
		}
		else
			return -1;
	}
	return index;
}

void drbSTDump( int gid, int id )
{
	if ( !drbSTOn )
	{
		trace( "@@@ drbST is off\n" );
		return;
	}

	if ( !drbSTInited )
	{
		trace( "@@@ drbST not initialized\n" );
		return;
	}

	int index = drbSTGetIndex(gid,id);
	if ( index == -1 )
		return;

	int end = ( drbSTStart[index] - 1 ) & DRB_SUPERTRACKER_DEPTH_MASK;
	for( int cur = drbSTStart[index]; cur != end; cur = ( cur + 1 ) & DRB_SUPERTRACKER_DEPTH_MASK )
	{
		switch ( drbSTHashTbl[index][cur].type )
		{
		case STTYPE_INVALID:
			break;
		case STTYPE_NONE:
			trace( "@@@ drbST - Unit(%i) - %s - Line(%i) (none)\n", id, drbSTHashTbl[index][cur].name, drbSTHashTbl[index][cur].line );
			break;
		case STTYPE_INT:
			trace( "@@@ drbST - Unit(%i) - %s - Line(%i) (int): %i, %i, %i\n", id, drbSTHashTbl[index][cur].name, drbSTHashTbl[index][cur].line,
				drbSTHashTbl[index][cur].data[0], drbSTHashTbl[index][cur].data[1], drbSTHashTbl[index][cur].data[2] );
			break;
		case STTYPE_FLOAT:
			{
				float * f0 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[0]);
				float * f1 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[1]);
				float * f2 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[2]);
				trace( "@@@ drbST - Unit(%i) - %s - Line(%i) (float): %.2f, %.2f, %.2f\n", id, drbSTHashTbl[index][cur].name, drbSTHashTbl[index][cur].line, *f0, *f1, *f2 );
			}
			break;
		case STTYPE_VECTOR:
			{
				float * f0 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[0]);
				float * f1 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[1]);
				float * f2 = (float*)((DWORD*)&drbSTHashTbl[index][cur].data[2]);
				trace( "@@@ drbST - Unit(%i) - %s - Line(%i) (vector): %.2f, %.2f, %.2f\n", id, drbSTHashTbl[index][cur].name, drbSTHashTbl[index][cur].line, *f0, *f1, *f2 );
			}
			break;
		}
	}
}

static void drbSTSave( UNIT * unit, WORD type, const char * szFunction, WORD line, DWORD d0, DWORD d1, DWORD d2)
{
	if ( !drbSTOn )
		return;

	GAME * game = UnitGetGame( unit );
	if ( IS_CLIENT( game ) )
		return;

	if ( !drbSTInited )
		drbInitSuperTracker();

	int id = (int)UnitGetId( unit );
	int gid = GameGetId( game );
	int index = drbSTGetIndex( gid, id, TRUE );
	if ( index == -1 )
		return;
	int cur = drbSTStart[index];
	drbSTHashTbl[index][cur].type = type;
	drbSTHashTbl[index][cur].line = line;
	drbSTHashTbl[index][cur].data[0] = d0;
	drbSTHashTbl[index][cur].data[1] = d1;
	drbSTHashTbl[index][cur].data[2] = d2;
	PStrCopy( drbSTHashTbl[index][cur].name, szFunction, DRB_SUPERTRACKER_NAMELEN );
	drbSTStart[index] = ( drbSTStart[index] + 1 ) & DRB_SUPERTRACKER_DEPTH_MASK;
}

void drbST( UNIT * unit, const char * szFunction, int line )
{
	drbSTSave( unit, (WORD)STTYPE_NONE, szFunction, (WORD)line, 0, 0, 0 );
}

void drbSTI( UNIT * unit, const char * szFunction, int line, int d0, int d1, int d2 )
{
	DWORD * dw0 = (DWORD*)((int*)&d0);
	DWORD * dw1 = (DWORD*)((int*)&d1);
	DWORD * dw2 = (DWORD*)((int*)&d2);
	drbSTSave( unit, (WORD)STTYPE_INT, szFunction, (WORD)line, *dw0, *dw1, *dw2 );
}

void drbSTF( UNIT * unit, const char * szFunction, int line, float d0, float d1, float d2 )
{
	DWORD * dw0 = (DWORD*)((float*)&d0);
	DWORD * dw1 = (DWORD*)((float*)&d1);
	DWORD * dw2 = (DWORD*)((float*)&d2);
	drbSTSave( unit, (WORD)STTYPE_FLOAT, szFunction, (WORD)line, *dw0, *dw1, *dw2 );
}

void drbSTV( UNIT * unit, const char * szFunction, int line, VECTOR v )
{
	DWORD * dw0 = (DWORD*)((float*)&v.x);
	DWORD * dw1 = (DWORD*)((float*)&v.y);
	DWORD * dw2 = (DWORD*)((float*)&v.z);
	drbSTSave( unit, (WORD)STTYPE_VECTOR, szFunction, (WORD)line, *dw0, *dw1, *dw2 );
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
// pathing stats!

#if ISVERSION(DEVELOPMENT)
static int drbSrvStepCount = 0;
static int drbCltStepCount = 0;
static int drbNumPaths = 0;
static int drbMaxPaths = 0;
static int drbOldMax = 0;
static int drbPathTotal = 0;
static float drbAvgPaths = 0.0f;
static DWORD drbTickStart = 0;
#endif

#if ISVERSION(DEVELOPMENT)
void drbStepDebug( int & clt, int & srv, int & paths, int & max_paths, float & avg_paths )
{
	clt = drbCltStepCount;
	srv = drbSrvStepCount;
	paths = drbNumPaths;
	max_paths = drbOldMax;
	avg_paths = drbAvgPaths;
}
#endif

#if ISVERSION(DEVELOPMENT)
void drbDebugClearPathCount()
{
	drbPathTotal += drbNumPaths;
	if ( drbNumPaths > drbMaxPaths )
		drbMaxPaths = drbNumPaths;
	drbNumPaths = 0;
	if ( drbTickStart == 0 )
	{
		drbTickStart = GetTickCount();
	}

	DWORD tick = GetTickCount();
	if ( tick - drbTickStart > 1000 )
	{
		drbTickStart += 1000;
		drbOldMax = drbMaxPaths;
		drbAvgPaths = (float)drbPathTotal / 20.0f;
		drbMaxPaths = 0;
		drbPathTotal = 0;
	}
}
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
float GRAVITY_ACCELERATION = 24.45f;		// not 9.8, but feels better (this number was painstakingly chosen with the jump accel to look good)


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDestroyPathParticles(
	GAME * game,
	PATH_STATE* path)
{
#ifdef _DEBUG
	if (path->m_pParticleIds)
	{
		for(int i=0; i<path->m_nParticleIdCount; i++)
		{
			c_ParticleSystemDestroy(path->m_pParticleIds[i]);
		}
		GFREE(game, path->m_pParticleIds);
		path->m_pParticleIds = NULL;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdatePathingSplineDebug()
{
#ifdef _DEBUG
	UNIT * pDebugUnit = GameGetDebugUnit(AppGetCltGame());
	if(!pDebugUnit || !pDebugUnit->m_pActivePath)
		return;

	VECTOR vPoint1, vPoint2;
	float fPathLength = pDebugUnit->m_pActivePath->Length();
	vPoint1 = pDebugUnit->m_pActivePath->GetSplinePositionAtDistance(0.0f);
	DWORD dwFlags;
	dwFlags |= PRIM_FLAG_RENDER_ON_TOP;
	for(float fCurrent = 0.1f; fCurrent < fPathLength; fCurrent += 0.1f)
	{
		vPoint2 = pDebugUnit->m_pActivePath->GetSplinePositionAtDistance(fCurrent);
		e_PrimitiveAddLine(dwFlags, &vPoint1, &vPoint2);
		vPoint1 = vPoint2;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreatePathParticles(
	GAME * game,
	UNIT * unit,
	PATH_STATE * path)
{
#ifdef _DEBUG
	if (IS_CLIENT(game) && GameGetDebugUnit(game) == unit && path->m_nPathNodeCount)
	{
		if(path->m_pParticleIds)
		{
			sDestroyPathParticles(game, path);
		}

		int nNodeParticleDefinition = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, "gspathnode.xml" );
		int nPathParticleDefinition = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, "gspathnodeconnection.xml" );
		int nOccupiedParticleDefinition = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, "gspathnodeoccupied.xml" );

		//*
		path->m_nParticleIdCount = path->m_nPathNodeCount*2 - 1;
		path->m_pParticleIds = (int*)GMALLOCZ(game, path->m_nParticleIdCount*sizeof(int));
		for(int i=0; i<path->m_nPathNodeCount; i++)
		{
			ROOM * pNodeRoom = RoomGetByID(game, path->m_pPathNodes[i].nRoomId);
			if(pNodeRoom)
			{
				ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pNodeRoom, path->m_pPathNodes[i].nPathNodeIndex);
			
				VECTOR vPos = RoomPathNodeGetExactWorldPosition(game, pNodeRoom, pPathNode);
				if(i == path->m_nPathNodeCount-1)
				{
					path->m_pParticleIds[i*2] = c_ParticleSystemNew(nOccupiedParticleDefinition, &vPos, &cgvZAxis, path->m_pPathNodes[i].nRoomId);
				}
				else
				{
					path->m_pParticleIds[i*2] = c_ParticleSystemNew(nNodeParticleDefinition, &vPos, &cgvZAxis, path->m_pPathNodes[i].nRoomId);
				}

				if(i < path->m_nPathNodeCount-1)
				{
					ROOM * pNodeRoomOther = RoomGetByID(game, path->m_pPathNodes[i+1].nRoomId);
					if(pNodeRoomOther)
					{
						ROOM_PATH_NODE * pPathNodeOther = RoomGetRoomPathNode(pNodeRoomOther, path->m_pPathNodes[i+1].nPathNodeIndex);

						VECTOR vOtherPosition = RoomPathNodeGetExactWorldPosition(game, pNodeRoomOther, pPathNodeOther);
						VECTOR vNormal = vOtherPosition - vPos;
						VectorNormalize(vNormal);

						path->m_pParticleIds[i*2 + 1] = c_ParticleSystemNew(nPathParticleDefinition, &vPos, &vNormal);

						if (path->m_pParticleIds[i*2 + 1] != INVALID_ID)
						{
							c_ParticleSystemSetRopeEnd(path->m_pParticleIds[i*2 + 1], vOtherPosition, vNormal);
						}
					}
				}
			}
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PATH_STATE* PathStructInit(
	GAME* game)
{
	PATH_STATE* path = (PATH_STATE*)GMALLOCZ(game, sizeof(PATH_STATE));
	path->m_eState = PATHING_STATE_INACTIVE;
	path->m_nCurrentPath = INVALID_ID;
	path->m_tOccupiedLocation.m_nRoomId = INVALID_ID;
	path->m_tOccupiedLocation.m_nRoomPathNodeIndex = INVALID_ID;
#if HELLGATE_ONLY
	path->cFastPathIndex = INVALID_ID;


	for ( int i = 0; i < MAX_CLIENT_FAST_PATH_SIZE; i++ )
	{
		path->cFastPathNodes[i].nRoomId = INVALID_ID;
		path->cFastPathNodes[i].nPathNodeIndex = INVALID_ID;
	}
	for ( int i = 0; i < MAX_HAPPY_PLACE_HISTORY; i++ )
	{
		path->m_HappyPlaces[i].m_HappyPlace.m_nRoomId = INVALID_ID;
		path->m_HappyPlaces[i].m_HappyPlace.m_nRoomPathNodeIndex = INVALID_ID;
		path->m_HappyPlaces[i].m_tiCreateTick = TIME_ZERO;
	}
	path->m_nHappyPlaceCount = 0;
#else
	ArrayInit(path->m_SortedFaces, GameGetMemPool(game), 2);
	path->m_LastSortPosition = VECTOR( -1, -1, -1 );
#endif
	HashInit(path->m_tShouldTestHash, NULL, DEFAULT_ASTAR_LIST_SIZE / 16);
	//SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
	//ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);
	return path;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathStructClear(
	UNIT* pUnit)
{
	ASSERT(pUnit);
	if (!pUnit->m_pPathing)
	{
		return;
	}

	GAME * game = UnitGetGame(pUnit);

	sDestroyPathParticles(game, pUnit->m_pPathing);
	pUnit->m_pPathing->m_eState = PATHING_STATE_INACTIVE;
	pUnit->m_pPathing->m_nCurrentPath = INVALID_ID;
	memclear(pUnit->m_pPathing->m_pPathNodes, pUnit->m_pPathing->m_nPathNodeCount * sizeof(ASTAR_OUTPUT));
	pUnit->m_pPathing->m_nPathNodeCount = 0;
#if HELLGATE_ONLY
	pUnit->m_pPathing->cFastPathIndex = INVALID_ID;
	for ( int i = 0; i < MAX_HAPPY_PLACE_HISTORY; i++ )
	{
		pUnit->m_pPathing->m_HappyPlaces[i].m_HappyPlace.m_nRoomId = INVALID_ID;
		pUnit->m_pPathing->m_HappyPlaces[i].m_HappyPlace.m_nRoomPathNodeIndex = INVALID_ID;
		pUnit->m_pPathing->m_HappyPlaces[i].m_tiCreateTick = TIME_ZERO;
	}
	pUnit->m_pPathing->m_nHappyPlaceCount = 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathStructFree(
	GAME* game,
	PATH_STATE* path)
{
	ASSERT_RETURN(game);
	if (!path)
	{
		return;
	}
	sDestroyPathParticles(game, path);

	HashFree(path->m_tShouldTestHash);
#if HELLGATE_ONLY
#else
	path->m_SortedFaces.Destroy();
#endif
	GFREE(game, path);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define HAPPY_PLACE_TIME_DELTA 200
BOOL UnitFindGoodHappyPlace(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vHappyPlace)
{
#if HELLGATE_ONLY
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pUnit);
	ASSERT_RETFALSE(pUnit->m_pPathing);

	ROOM * pUnitRoom = UnitGetRoom(pUnit);

	int nAllHappyPlaceCount = 0;
	PATH_STATE_LOCATION tAllHappyPlaces[MAX_HAPPY_PLACE_HISTORY];
	float fDistances[MAX_HAPPY_PLACE_HISTORY];

	VECTOR vUnitPosition = UnitGetPosition(pUnit);

	// Iterate through the current room and all nearby rooms to find the nearest happy place that we haven't been to recently
	int nNeighboringRoomCount = RoomGetAdjacentRoomCount(pUnitRoom);
	for(int i=-1; i<nNeighboringRoomCount; i++)
	{
		ROOM * pRoom = (i == -1) ? pUnitRoom : RoomGetAdjacentRoom(pUnitRoom, i);
		if(!pRoom)
			continue;

		int nHappyNodeCount = RoomPathNodeGetHappyPlaceCount(pGame, pRoom);
		for(int j=0; j<nHappyNodeCount; j++)
		{
			ROOM_PATH_NODE * pPathNode = RoomPathNodeGetHappyPlace(pGame, pRoom, j);
			if(!pPathNode)
				continue;

			VECTOR vRoomHappyPlace = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
			float fDistanceSquared = VectorDistanceSquared(vUnitPosition, vRoomHappyPlace);
			if(nAllHappyPlaceCount < MAX_HAPPY_PLACE_HISTORY)
			{
				tAllHappyPlaces[nAllHappyPlaceCount].m_nRoomId = RoomGetId(pRoom);
				tAllHappyPlaces[nAllHappyPlaceCount].m_nRoomPathNodeIndex = pPathNode->nIndex;
				fDistances[nAllHappyPlaceCount] = fDistanceSquared;
				nAllHappyPlaceCount++;
			}
			else
			{
				// Find the smallest distance greater than our distance
				float fLargestDistance = -1.0f;
				int nLargestIndex = -1;
				for(int k=0; k<MAX_HAPPY_PLACE_HISTORY; k++)
				{
					if(fDistances[k] > fLargestDistance)
					{
						fLargestDistance = fDistanceSquared;
						nLargestIndex = k;
					}
				}
				if(nLargestIndex >= 0 && fDistances[nLargestIndex] > fDistanceSquared)
				{
					tAllHappyPlaces[nLargestIndex].m_nRoomId = RoomGetId(pRoom);
					tAllHappyPlaces[nLargestIndex].m_nRoomPathNodeIndex = pPathNode->nIndex;
					fDistances[nLargestIndex] = fDistanceSquared;
				}
			}
		}
	}

	GAME_TICK tiCurrentGameTick = GameGetTick(pGame);
	int nIndex = INVALID_ID;
	for(int i=0; i<(MAX_HAPPY_PLACE_HISTORY+1) && nIndex < 0; i++)
	{
		PickerStart(pGame, picker);
		for(int j=0; j<nAllHappyPlaceCount; j++)
		{
			PATH_STATE_LOCATION * pHappyPlace = &tAllHappyPlaces[j];

			BOOL bSkipOut = FALSE;
			for(int k=0; k<(MAX_HAPPY_PLACE_HISTORY-i); k++)
			{
				if(pUnit->m_pPathing->m_HappyPlaces[k].m_HappyPlace.m_nRoomId == pHappyPlace->m_nRoomId &&
					pUnit->m_pPathing->m_HappyPlaces[k].m_HappyPlace.m_nRoomPathNodeIndex == pHappyPlace->m_nRoomPathNodeIndex &&
					pUnit->m_pPathing->m_HappyPlaces[k].m_tiCreateTick - tiCurrentGameTick <= HAPPY_PLACE_TIME_DELTA)
				{
					bSkipOut = TRUE;
					break;
				}
			}
			if(bSkipOut)
				continue;

			PickerAdd(MEMORY_FUNC_FILELINE(pGame, j, 1));
		}
		nIndex = PickerChoose(pGame);
	}

	if(nIndex < 0)
	{
		return FALSE;
	}

	if(tAllHappyPlaces[nIndex].m_nRoomId < 0 || tAllHappyPlaces[nIndex].m_nRoomPathNodeIndex < 0)
	{
		return FALSE;
	}

	ROOM * pRoom = RoomGetByID(pGame, tAllHappyPlaces[nIndex].m_nRoomId);
	if(!pRoom)
	{
		return FALSE;
	}

	ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pRoom, tAllHappyPlaces[nIndex].m_nRoomPathNodeIndex);
	if(!pPathNode)
	{
		return FALSE;
	}

	// We have now used a happy place, so add it to the list
	for(int i=MAX_HAPPY_PLACE_HISTORY-1; i>=1; i--)
	{
		pUnit->m_pPathing->m_HappyPlaces[i] = pUnit->m_pPathing->m_HappyPlaces[i-1];
	}
	pUnit->m_pPathing->m_HappyPlaces[0].m_HappyPlace.m_nRoomId = tAllHappyPlaces[nIndex].m_nRoomId;
	pUnit->m_pPathing->m_HappyPlaces[0].m_HappyPlace.m_nRoomPathNodeIndex = tAllHappyPlaces[nIndex].m_nRoomPathNodeIndex;
	pUnit->m_pPathing->m_HappyPlaces[0].m_tiCreateTick = GameGetTick(pGame);

	vHappyPlace = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_NODE * PathStructGetEstimatedLocation(
	UNIT* unit,
	ROOM *& pEstimatedRoom)
{
	ASSERT_RETNULL(unit && unit->m_pPathing);
	PATH_STATE * path = unit->m_pPathing;

	if(path->m_tOccupiedLocation.m_nRoomId >= 0)
	{
		pEstimatedRoom = RoomGetByID(UnitGetGame(unit), path->m_tOccupiedLocation.m_nRoomId);
		if(!pEstimatedRoom)
		{
			pEstimatedRoom = UnitGetRoom( unit );
		}
		if(!pEstimatedRoom)
		{
			return NULL;
		}
	}

	ROOM_PATH_NODE * pEstimatedPathNode = NULL;
	if(path->m_tOccupiedLocation.m_nRoomPathNodeIndex >= 0)
	{
		pEstimatedPathNode = RoomGetRoomPathNode(pEstimatedRoom, path->m_tOccupiedLocation.m_nRoomPathNodeIndex);
	}
	return pEstimatedPathNode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHash<PATH_NODE_SHOULD_TEST> * PathStructGetShouldTest(
	UNIT* unit)
{
	ASSERT_RETNULL(unit && unit->m_pPathing);
	return &unit->m_pPathing->m_tShouldTestHash;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHash<PATH_NODE_SHOULD_TEST> * PathStructGetShouldTest(
	PATH_STATE* path)
{
	ASSERT_RETNULL(path);
	return &path->m_tShouldTestHash;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define NEW_PATH_SERIALIZE 1
#if NEW_PATH_SERIALIZE
#define MIN_INDEX_COUNT_BITS	8
#define MIN_ROOM_DICT_BITS		6
#define MIN_ROOM_ID_BITS		16

//#define PATH_STRUCT_SERIALIZE_TRACE(...) trace(__VA_ARGS__)
#define PATH_STRUCT_SERIALIZE_TRACE(...)

struct PATH_STRUCT_SERIALIZE_ROOM_DICTIONARY
{
	ROOMID		idRoom;
	ROOM *		pRoom;
	int			nPathNodeIndexBits;
};

static int sPathStructSerializeFindRoomIndexInDictionary(
	const ROOMID idRoom,
	const PATH_STRUCT_SERIALIZE_ROOM_DICTIONARY * pRoomDictionary,
	const int nRoomDictionarySize)
{
	for(int i=0; i<nRoomDictionarySize; i++)
	{
		if(pRoomDictionary[i].idRoom == idRoom)
		{
			return i;
		}
	}
	return nRoomDictionarySize;
}

void PathStructSerialize(
	GAME * pGame,
	PATH_STATE * path,
	BIT_BUF & buf)
{
	PATH_STRUCT_SERIALIZE_TRACE("Serializing a path\n");

	unsigned int nOriginalCursor = buf.GetCursor();
	ASSERT_GOTO(pGame, err);
	if (!path || path->m_nPathNodeCount == 0)
	{
		PATH_STRUCT_SERIALIZE_TRACE("Path is NULL or Path Node Count is Zero.  Writing out Zero and exiting\n");
		buf.PushUInt( 0, MIN_INDEX_COUNT_BITS );
		return;
	}

	ASSERT_DO(path->m_nPathNodeCount <= DEFAULT_ASTAR_LIST_SIZE)
	{
		path->m_nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	}

	PATH_STRUCT_SERIALIZE_ROOM_DICTIONARY pRoomDictionary[1<<MIN_ROOM_DICT_BITS];
	int nRoomDictionarySize = 0;

	// First build the dictionary
	PATH_STRUCT_SERIALIZE_TRACE("Building the Room Dictionary\n");
	for(int i=0; i<path->m_nPathNodeCount; i++)
	{
		ROOMID idRoom = path->m_pPathNodes[i].nRoomId;
		PATH_STRUCT_SERIALIZE_TRACE("Finding Dict Index for Room %d\n", idRoom);
		int nRoomDictIndex = sPathStructSerializeFindRoomIndexInDictionary(idRoom, pRoomDictionary, nRoomDictionarySize);
		PATH_STRUCT_SERIALIZE_TRACE("Dict Index is %d - Dict Size is %d\n", nRoomDictIndex, nRoomDictionarySize);
		ASSERT_GOTO(nRoomDictionarySize < (1<<MIN_ROOM_DICT_BITS), err);
		if(nRoomDictIndex == nRoomDictionarySize)
		{
			PATH_STRUCT_SERIALIZE_TRACE("Adding Room %d to the Dictionary\n", idRoom);
			pRoomDictionary[nRoomDictionarySize].idRoom = idRoom;
			pRoomDictionary[nRoomDictionarySize].pRoom = RoomGetByID(pGame, idRoom);
			ASSERT_GOTO(pRoomDictionary[nRoomDictionarySize].pRoom, err);

			// Figure out how many bits each room path node index is going to use
			const ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoomDictionary[nRoomDictionarySize].pRoom);
			ASSERT_GOTO(pPathNodeSet, err);
			pRoomDictionary[nRoomDictionarySize].nPathNodeIndexBits = BITS_TO_STORE(pPathNodeSet->nPathNodeCount);
			PATH_STRUCT_SERIALIZE_TRACE("Path Node Count is %d, Bits To Store is %d\n", pPathNodeSet->nPathNodeCount, pRoomDictionary[nRoomDictionarySize].nPathNodeIndexBits);
			ASSERT_GOTO(pRoomDictionary[nRoomDictionarySize].nPathNodeIndexBits > 0, err);
			nRoomDictionarySize++;
		}
	}

	// Now save out the dictionary
	PATH_STRUCT_SERIALIZE_TRACE("Writing out dictionary\n");
	PATH_STRUCT_SERIALIZE_TRACE("Writing Size: %d\n", nRoomDictionarySize);
	buf.PushUInt(nRoomDictionarySize, MIN_ROOM_DICT_BITS);
	for(int i=0; i<nRoomDictionarySize; i++)
	{
		PATH_STRUCT_SERIALIZE_TRACE("Writing Room: %d\n", pRoomDictionary[i].idRoom);
		buf.PushUInt(pRoomDictionary[i].idRoom, MIN_ROOM_ID_BITS);
	}

	ROOMID idOldRoom = INVALID_ID;
	int nPathNodeIndex = 0;
	unsigned int nNextCountLocation = buf.GetCursor();
	int nLastCount = INVALID_ID;
	int nPathNodeIndexBits = 0;
	while(nPathNodeIndex < path->m_nPathNodeCount)
	{
		ROOMID idRoom = path->m_pPathNodes[nPathNodeIndex].nRoomId;
		if(idRoom != idOldRoom)
		{
			PATH_STRUCT_SERIALIZE_TRACE("Found New Room: %d (old room was %d)\n", idRoom, idOldRoom);
			if(nLastCount >= 0)
			{
				PATH_STRUCT_SERIALIZE_TRACE("Writing Last Count (%d) at %d\n", nLastCount, nNextCountLocation);
				unsigned int nOldCursor = buf.GetCursor();
				buf.SetCursor(nNextCountLocation);
				buf.PushUInt(nLastCount, MIN_INDEX_COUNT_BITS);
				buf.SetCursor(nOldCursor);
			}

			nLastCount = 0;

			PATH_STRUCT_SERIALIZE_TRACE("Writing out a header\n");
			// Output a header
			int nRoomDictIndex = sPathStructSerializeFindRoomIndexInDictionary(idRoom, pRoomDictionary, nRoomDictionarySize);
			ASSERT_GOTO(nRoomDictIndex < nRoomDictionarySize, err);
			nNextCountLocation = buf.GetCursor();
			PATH_STRUCT_SERIALIZE_TRACE("Storing next count location %d\n", nNextCountLocation);
			PATH_STRUCT_SERIALIZE_TRACE("Writing temp zero\n");
			buf.PushUInt( 0, MIN_INDEX_COUNT_BITS );
			PATH_STRUCT_SERIALIZE_TRACE("Writing Room Dict Index %d\n", nRoomDictIndex);
			buf.PushUInt( nRoomDictIndex, MIN_ROOM_DICT_BITS );
			idOldRoom = idRoom;

			nPathNodeIndexBits = pRoomDictionary[nRoomDictIndex].nPathNodeIndexBits;
			PATH_STRUCT_SERIALIZE_TRACE("Path Node Index bits: %d\n", nPathNodeIndexBits);
		}

		PATH_STRUCT_SERIALIZE_TRACE("Writing out Path Node Index %d\n", path->m_pPathNodes[nPathNodeIndex].nPathNodeIndex);
		buf.PushUInt(path->m_pPathNodes[nPathNodeIndex].nPathNodeIndex, nPathNodeIndexBits);
		nLastCount++;

		nPathNodeIndex++;
	}
	PATH_STRUCT_SERIALIZE_TRACE("Writing Last Count (%d) at %d\n", nLastCount, nNextCountLocation);
	unsigned int nOldCursor = buf.GetCursor();
	buf.SetCursor(nNextCountLocation);
	buf.PushUInt(nLastCount, MIN_INDEX_COUNT_BITS);
	buf.SetCursor(nOldCursor);

	PATH_STRUCT_SERIALIZE_TRACE("Writing out Zero terminator\n");
	buf.PushUInt(0, MIN_INDEX_COUNT_BITS);

	return;

err:
	PATH_STRUCT_SERIALIZE_TRACE("An error occured.  Writing out Zero and exiting\n");
	buf.SetCursor(nOriginalCursor);
	buf.PushUInt( 0, MIN_INDEX_COUNT_BITS );
	return;
}

#define PATH_STRUCT_DESERIALIZE_UPOP(x,y) ASSERT_RETURN(tBuffer.PopUInt(x,y))
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathStructDeserialize(
	UNIT* pUnit,
	BYTE * buf,
	BYTE bFlags)
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->m_pPathing );

	GAME * pGame = UnitGetGame(pUnit);
	// in Tugboat, clients can send paths to the server.
	if( AppIsHellgate() )
	{
		ASSERT_RETURN( IS_CLIENT( pGame ) );
	}

	PATH_STATE* path = pUnit->m_pPathing;

	PathStructClear(pUnit);
	//UnitSetStatFloat(pUnit, STATS_PATH_DISTANCE, 0, 0.0f);

	BIT_BUF tBuffer(buf, MAX_PATH_BUFFER);

	PATH_STRUCT_SERIALIZE_TRACE("Deserializing a path\n");

	PATH_STRUCT_SERIALIZE_ROOM_DICTIONARY pRoomDictionary[1<<MIN_ROOM_DICT_BITS];
	memclear(pRoomDictionary, sizeof(pRoomDictionary));

	int nRoomDictionarySize = 0;

	// Read in the dictionary:
	PATH_STRUCT_SERIALIZE_TRACE("Reading the Room dictionary\n");
	PATH_STRUCT_DESERIALIZE_UPOP(&nRoomDictionarySize, MIN_ROOM_DICT_BITS);
	PATH_STRUCT_SERIALIZE_TRACE("Read Room Dict Size %d\n", nRoomDictionarySize);
	if(nRoomDictionarySize <= 0)
	{
		PATH_STRUCT_SERIALIZE_TRACE("Room Dict Size is zero, exiting\n");
		return;
	}
	for(int i=0; i<nRoomDictionarySize; i++)
	{
		PATH_STRUCT_DESERIALIZE_UPOP(&pRoomDictionary[i].idRoom, MIN_ROOM_ID_BITS);
		PATH_STRUCT_SERIALIZE_TRACE("Room Room ID %d\n", pRoomDictionary[i].idRoom);
		pRoomDictionary[i].pRoom = RoomGetByID(pGame, pRoomDictionary[i].idRoom);
		ASSERT_RETURN(pRoomDictionary[i].pRoom);

		// Figure out how many bits each room path node index is going to use
		const ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet(pRoomDictionary[i].pRoom);
		ASSERT_RETURN(pPathNodeSet);
		pRoomDictionary[i].nPathNodeIndexBits = BITS_TO_STORE(pPathNodeSet->nPathNodeCount);
		PATH_STRUCT_SERIALIZE_TRACE("Path Node Count is %d, Bits To Store is %d\n", pPathNodeSet->nPathNodeCount, pRoomDictionary[i].nPathNodeIndexBits);
		ASSERT_RETURN(pRoomDictionary[i].nPathNodeIndexBits > 0);
	}

	path->m_nPathNodeCount = 0;
	int nIndexCount = 0;
	do
	{
		PATH_STRUCT_SERIALIZE_TRACE("Reading Header\n");
		int nRoomDictIndex = INVALID_ID;
		PATH_STRUCT_DESERIALIZE_UPOP(&nIndexCount, MIN_INDEX_COUNT_BITS);
		PATH_STRUCT_SERIALIZE_TRACE("Read Index count of %d\n", nIndexCount);
		if(nIndexCount > 0)
		{
			PATH_STRUCT_DESERIALIZE_UPOP(&nRoomDictIndex, MIN_ROOM_DICT_BITS);
			PATH_STRUCT_SERIALIZE_TRACE("Read Room Dict Index of %d\n", nRoomDictIndex);
		}
		for(int i=0; i<nIndexCount; i++)
		{
			ASSERT_BREAK(path->m_nPathNodeCount < DEFAULT_ASTAR_LIST_SIZE);
			path->m_pPathNodes[path->m_nPathNodeCount].nRoomId = pRoomDictionary[nRoomDictIndex].idRoom;
			PATH_STRUCT_DESERIALIZE_UPOP(&path->m_pPathNodes[path->m_nPathNodeCount].nPathNodeIndex, pRoomDictionary[nRoomDictIndex].nPathNodeIndexBits);
			PATH_STRUCT_SERIALIZE_TRACE("Read Path Node Index %d\n", path->m_pPathNodes[path->m_nPathNodeCount].nPathNodeIndex);
#ifdef _DEBUG
			ASSERT(pRoomDictionary[nRoomDictIndex].pRoom && pRoomDictionary[nRoomDictIndex].pRoom->pPathDef);
			if(pRoomDictionary[nRoomDictIndex].pRoom && pRoomDictionary[nRoomDictIndex].pRoom->pPathDef)
			{
				ASSERT(int(path->m_pPathNodes[path->m_nPathNodeCount].nPathNodeIndex) < pRoomDictionary[nRoomDictIndex].pRoom->pPathDef->pPathNodeSets[pRoomDictionary[nRoomDictIndex].pRoom->nPathLayoutSelected].nPathNodeCount);
			}
#endif
			path->m_nPathNodeCount++;
		}
	}
	while(nIndexCount > 0);

	if (path->m_nPathNodeCount)
	{
		path->m_nCurrentPath = 0;
		path->m_eState = PATHING_STATE_ACTIVE;
#ifdef DRB_SPLINE_PATHING
		path->vStart = pUnit->vPosition;
		path->m_Time = AppCommonGetCurTime();
#endif
		sCreatePathParticles(pGame, pUnit, path);
		UnitStepListAdd( pGame, pUnit );
#if HELLGATE_ONLY
		path->cFastPathIndex = -1;
		path->cFastPathSize = 0;
#endif
#define CLIENT_SIDE_PATHING_ENABLED 0

#if CLIENT_SIDE_PATHING_ENABLED
		// try and create a "fast" client path
		if ( path->m_nPathNodeCount > 1 )
		{
			// first prune stuffs
			int startnode = -1;
			int endnode = path->m_nPathNodeCount - 1;
			BOOL bDone = FALSE;
			float cradius = UnitGetCollisionRadius( pUnit );
			while ( !bDone )
			{
				VECTOR vStart;
				if ( startnode < 0 )
					vStart = pUnit->vPosition;
				else
					vStart = AStarGetWorldPosition( pGame, &path->m_pPathNodes[startnode] );
				VECTOR vEnd = AStarGetWorldPosition( pGame, &path->m_pPathNodes[endnode] );
				BOOL path_okay = true;
				// If the server tells us that the path is just straight, then we can just go
				// to the end node without testing whether it's okay.
				if(!TESTBIT(bFlags, PSS_CLIENT_STRAIGHT_BIT))
				{
					for ( int c = startnode+1; ( c < endnode ) && path_okay; c++ )
					{
						ASTAR_OUTPUT * anode = &path->m_pPathNodes[c];
						VECTOR vNode = AStarGetWorldPosition( pGame, anode );
						float dist = sqrt(DistanceSquaredPointLineXY( vNode, vStart, vEnd ));
						if ( ( dist > 2.0f ) || ( dist < 0.0f ) )
							path_okay = FALSE;
						if ( path_okay )
						{
							ROOM * room = RoomGetByID( pGame, anode->nRoomId );
							ROOM_PATH_NODE * node = RoomGetRoomPathNode( room, anode->nPathNodeIndex );
							if(node)
							{
								if ( dist + cradius > node->fRadius )
									path_okay = FALSE;
							}
							else
							{
								path_okay = FALSE;
							}
						}
					}
				}
				if(AppIsTugboat())
				{
					path->cFastPathIndex = -1;
					path->cFastPathSize = 0;
					bDone = TRUE;
				}
				else
				{
					if ( path_okay )
					{
						if ( path->cFastPathSize < MAX_CLIENT_FAST_PATH_SIZE )
						{
							// add node
							path->cFastPathNodes[path->cFastPathSize] = path->m_pPathNodes[endnode];
							path->cFastPathSize++;
							path->cFastPathIndex = 0;
							startnode = endnode;
							if ( endnode == path->m_nPathNodeCount - 1 )
							{
								bDone = TRUE;
								//trace( "fastpath success\n" );
							}
							else
								endnode = path->m_nPathNodeCount - 1;
						}
						else
						{
							// fast path too big for buffer
							path->cFastPathIndex = -1;
							path->cFastPathSize = 0;
							bDone = TRUE;
							//trace( "fastpath fail\n" );
						}
					}
					else
					{
						// try closer end point
						endnode--;
						if ( startnode+1 == endnode )
						{
							// start = end and we are not done... abort
							path->cFastPathIndex = -1;
							path->cFastPathSize = 0;
							bDone = TRUE;
							//trace( "fastpath fail\n" );
						}
					}
				}
			}
		}
#endif
	}
/*	else
	{
		trace( "deserializing empty path for unit %ls\n", pUnit->szName?pUnit->szName:L"");	
	}*/
	if( !UnitIsA( pUnit, UNITTYPE_PLAYER ))
	{
		UnitSetFlag( pUnit, UNITFLAG_SLIDE_TO_DESTINATION, FALSE );
	}

	CopyPathToSpline( pGame, pUnit, UnitGetMoveTarget( pUnit ), TESTBIT(bFlags, PSS_APPEND_DESTINATION_BIT) );
}

#else	// NEW_PATH_SERIALIZE

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MIN_ROOM_ID_BITS	16
#define MIN_PATH_INDEX_BITS 16
#define MIN_PATH_COUNT_BITS 16


void PathStructSerialize(
	GAME * pGame,
	PATH_STATE * path,
	BIT_BUF & buf)
{
	REF(pGame);
	if (!path)
	{
		buf.PushUInt( 0, MIN_PATH_COUNT_BITS );
		return;
	}

	ASSERT(path->m_nPathNodeCount < (1 << MIN_PATH_COUNT_BITS));
	ASSERT_DO(path->m_nPathNodeCount <= DEFAULT_ASTAR_LIST_SIZE)
	{
		path->m_nPathNodeCount = DEFAULT_ASTAR_LIST_SIZE;
	}

	buf.PushUInt(path->m_nPathNodeCount, MIN_PATH_COUNT_BITS);

	for(int i=0; i<path->m_nPathNodeCount; i++)
	{
		ASTAR_OUTPUT * pCurrent = &path->m_pPathNodes[i];

		ASSERT(pCurrent->nRoomId < (1 << MIN_ROOM_ID_BITS));
		ASSERT_BREAK(buf.PushUInt(pCurrent->nRoomId, MIN_ROOM_ID_BITS) );

		ASSERT(pCurrent->nPathNodeIndex < (1 << MIN_PATH_INDEX_BITS));
		ASSERT_BREAK(buf.PushUInt(pCurrent->nPathNodeIndex, MIN_PATH_INDEX_BITS) );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathStructDeserialize(
	UNIT* pUnit,
	BYTE * buf,
	BYTE bFlags)
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pUnit->m_pPathing );

	GAME * pGame = UnitGetGame(pUnit);
	// in Tugboat, clients can send paths to the server.
	if( AppIsHellgate() )
	{
		ASSERT_RETURN( IS_CLIENT( pGame ) );
	}

	PATH_STATE* path = pUnit->m_pPathing;

	PathStructClear(pUnit);
	//UnitSetStatFloat(pUnit, STATS_PATH_DISTANCE, 0, 0.0f);

	BIT_BUF tBuffer(buf, MAX_PATH_BUFFER);
	UINT nPathNodeCount;
	ASSERT_RETURN(tBuffer.PopUInt(&nPathNodeCount, MIN_PATH_COUNT_BITS) );
	ASSERT_RETURN(nPathNodeCount <= DEFAULT_ASTAR_LIST_SIZE);
	path->m_nPathNodeCount = nPathNodeCount;

	for(int i=0; i<path->m_nPathNodeCount; i++)
	{
		ASTAR_OUTPUT * pCurrent = &path->m_pPathNodes[i];
		UINT nRoomId;
		UINT nPathNodeIndex;
		ASSERT_BREAK(tBuffer.PopUInt(&nRoomId, MIN_ROOM_ID_BITS) );
		ASSERT_BREAK(tBuffer.PopUInt(&nPathNodeIndex, MIN_PATH_INDEX_BITS) );
		ASSERT(nRoomId >= 0);
		ASSERT(nPathNodeIndex >= 0);

		pCurrent->nRoomId = nRoomId;
		pCurrent->nPathNodeIndex = nPathNodeIndex;

#ifdef _DEBUG
		ROOM * pRoom = RoomGetByID(pGame, nRoomId);
		if(pRoom)
		{
			ASSERT(pRoom->pPathDef);
			ASSERT(int(nPathNodeIndex) < pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].nPathNodeCount);
		}
#endif
	}
	if (path->m_nPathNodeCount)
	{
		path->m_nCurrentPath = 0;
		path->m_eState = PATHING_STATE_ACTIVE;
		sCreatePathParticles(pGame, pUnit, path);
		UnitStepListAdd( pGame, pUnit );

		path->cFastPathIndex = -1;
		path->cFastPathSize = 0;

#define CLIENT_SIDE_PATHING_ENABLED 0

#if CLIENT_SIDE_PATHING_ENABLED
		// try and create a "fast" client path
		if ( path->m_nPathNodeCount > 1 )
		{
			// first prune stuffs
			int startnode = -1;
			int endnode = path->m_nPathNodeCount - 1;
			BOOL bDone = FALSE;
			float cradius = UnitGetCollisionRadius( pUnit );
			while ( !bDone )
			{
				VECTOR vStart;
				if ( startnode < 0 )
					vStart = pUnit->vPosition;
				else
					vStart = AStarGetWorldPosition( pGame, &path->m_pPathNodes[startnode] );
				VECTOR vEnd = AStarGetWorldPosition( pGame, &path->m_pPathNodes[endnode] );
				BOOL path_okay = true;
				// If the server tells us that the path is just straight, then we can just go
				// to the end node without testing whether it's okay.
				if(!TESTBIT(bFlags, PSS_CLIENT_STRAIGHT_BIT))
				{
					for ( int c = startnode+1; ( c < endnode ) && path_okay; c++ )
					{
						ASTAR_OUTPUT * anode = &path->m_pPathNodes[c];
						VECTOR vNode = AStarGetWorldPosition( pGame, anode );
						float dist = sqrt(DistanceSquaredPointLineXY( vNode, vStart, vEnd ));
						if ( ( dist > 2.0f ) || ( dist < 0.0f ) )
							path_okay = FALSE;
						if ( path_okay )
						{
							ROOM * room = RoomGetByID( pGame, anode->nRoomId );
							ROOM_PATH_NODE * node = RoomGetRoomPathNode( room, anode->nPathNodeIndex );
							if(node)
							{
								if ( dist + cradius > node->fRadius )
									path_okay = FALSE;
							}
							else
							{
								path_okay = FALSE;
							}
						}
					}
				}
				if(AppIsTugboat())
				{
					path->cFastPathIndex = -1;
					path->cFastPathSize = 0;
					bDone = TRUE;
				}
				else
				{
					if ( path_okay )
					{
						if ( path->cFastPathSize < MAX_CLIENT_FAST_PATH_SIZE )
						{
							// add node
							path->cFastPathNodes[path->cFastPathSize] = path->m_pPathNodes[endnode];
							path->cFastPathSize++;
							path->cFastPathIndex = 0;
							startnode = endnode;
							if ( endnode == path->m_nPathNodeCount - 1 )
							{
								bDone = TRUE;
								//trace( "fastpath success\n" );
							}
							else
								endnode = path->m_nPathNodeCount - 1;
						}
						else
						{
							// fast path too big for buffer
							path->cFastPathIndex = -1;
							path->cFastPathSize = 0;
							bDone = TRUE;
							//trace( "fastpath fail\n" );
						}
					}
					else
					{
						// try closer end point
						endnode--;
						if ( startnode+1 == endnode )
						{
							// start = end and we are not done... abort
							path->cFastPathIndex = -1;
							path->cFastPathSize = 0;
							bDone = TRUE;
							//trace( "fastpath fail\n" );
						}
					}
				}
			}
		}
#endif
	}

	CopyPathToSpline( pGame, pUnit, UnitGetMoveTarget( pUnit ), TESTBIT(bFlags, PSS_APPEND_DESTINATION_BIT) );
}
#endif // NEW_PATH_SERIALIZE

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float DistanceSquaredPointLineXY( VECTOR & vPoint, VECTOR & vStart, VECTOR & vEnd )
{
	VECTOR edge = vEnd - vStart;
	float fLineMagSq = ( edge.x * edge.x ) + ( edge.y * edge.y );
	if ( fLineMagSq == 0.0f )
		return -1.0f;

	float u = ( ( ( vPoint.x - vStart.x ) * ( vEnd.x - vStart.x ) ) + ( ( vPoint.y - vStart.y ) * ( vEnd.y - vStart.y ) ) ) / fLineMagSq;

	// Check if point falls within the line segment
	if ( u < 0.0f || u > 1.0f )
		return -1.0f;

	VECTOR intersection;
	intersection.x = vPoint.x - ( vStart.x + ( u * edge.x ) );
	intersection.y = vPoint.y - ( vStart.y + ( u * edge.y ) );
	return ( ( intersection.x * intersection.x ) + ( intersection.y * intersection.y ) );
}

BOOL PathPrematureEnd( GAME * pGame,
					   UNIT * pUnit,
					   const VECTOR& vPosition )
{
	PATH_STATE* path = pUnit->m_pPathing;
	if ( path && path->m_eState == PATHING_STATE_ACTIVE )
	{
		VECTOR NearestPoint;
		VECTOR Perpendicular;
		VECTOR Up;
		int		Segment;
		float	Distance;

		CPath* pPath = pUnit->m_pActivePath;
		VECTOR ClosestPosition;
		VECTOR AheadClosestPosition;
		VECTOR vFlatPosition = vPosition;
		vFlatPosition.fZ = 0;

		// find the nearest point to our current location
		pPath->FindNearestPoint( vFlatPosition, ClosestPosition, Perpendicular, Up, Segment, Distance );
		Perpendicular.fZ = 0;
		if( VectorLength( Perpendicular ) < 1.0f )
		{
			pUnit->m_pActivePath->SetLengthOverride( Distance );
			UnitSetFlag( pUnit, UNITFLAG_SLIDE_TO_DESTINATION, TRUE );
			return TRUE;
		}
	}
	return FALSE;
}


void PathSlideToDestination( GAME * pGame,
							 UNIT * pUnit,
							 float fTimeDelta )
{


	{
		VECTOR NearestPoint;
		VECTOR Perpendicular;
		VECTOR Up;
		int		Segment;
		float	Distance;

		CPath* pPath = pUnit->m_pActivePath;
		VECTOR ClosestPosition;
		VECTOR AheadClosestPosition;
		VECTOR vFlatPosition = pUnit->vPosition;
		vFlatPosition.fZ = 0;
		float fVelocity = UnitGetVelocityForMode( pUnit, MODE_RUN );

		// find the nearest point to our current location
		pPath->FindNearestPoint( vFlatPosition, ClosestPosition, Perpendicular, Up, Segment, Distance );
		Distance += ( fVelocity * fTimeDelta * .5f );
		if( Distance >= pPath->LengthOverride() )
		{
			Distance = pPath->LengthOverride();
			UnitSetFlag( pUnit, UNITFLAG_SLIDE_TO_DESTINATION, FALSE );
		}
		vFlatPosition = pPath->GetPositionAtDistance( Distance );
		vFlatPosition.fZ = pUnit->vPosition.fZ;
		vFlatPosition = VectorLerp( vFlatPosition, pUnit->vPosition, .3f );
		UnitSetPosition( pUnit, vFlatPosition );

	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CopyPathToSpline( 
	GAME * pGame,
	UNIT* pUnit,
	const VECTOR* vDestination,
	BOOL AppendDestination )
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN(pUnit->m_pActivePath);
	if( !pUnit->m_pPathing ||
		pUnit->m_pPathing->m_nPathNodeCount <= 0 )
	{
		return;
	}

	pUnit->m_fPathDistance = 0;
	VECTOR Delta;
	VECTOR CurrentPoint;
	VECTOR NextPoint;
	VECTOR CollisionNormal;
	pUnit->m_pActivePath->Clear();
	PATH_STATE * path = pUnit->m_pPathing;
	int NodeCount = path->m_nPathNodeCount;

	// fixup our realworld target dest to be within range of the last pathnode
	ROOM* pRoom = UnitGetRoom( pUnit );
	REF( pRoom );
	float MaxDistance = MAX_PATH_NODE_FUDGE;
	if( AppIsTugboat() && pRoom )
	{
		MaxDistance = pRoom->pPathDef->fNodeFrequencyX -.15f;
	}
	VECTOR vFixedDestination;
	if( vDestination )
	{
		vFixedDestination = *vDestination;
	}
	else
	{
		vFixedDestination = AStarGetWorldPosition( pGame, &path->m_pPathNodes[NodeCount - 1] );
	}
	CurrentPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[NodeCount - 1] );
	Delta = VECTOR( vFixedDestination.fX, vFixedDestination.fY, vFixedDestination.fZ ) - CurrentPoint;
	if(AppIsTugboat())
	{
		Delta.fZ = 0;
	}
	float Distance = VectorLength( Delta );
	if( Distance > MaxDistance )
	{
		VectorNormalize( Delta );
		Delta *= MaxDistance;
	}
	vFixedDestination = CurrentPoint + Delta;

	CurrentPoint = VECTOR( pUnit->vPosition.fX, pUnit->vPosition.fY, pUnit->vPosition.fZ + .25f );


	if( IS_CLIENT( pGame ) && AppIsTugboat() &&
		GameGetControlUnit( pGame ) == pUnit )
	{
		LEVEL* pLevel = UnitGetLevel( pUnit );
		VECTOR TestPoint;
		VECTOR NextNextPoint;
		AppendDestination = FALSE;
		int Material = 0;

		pUnit->m_pActivePath->CPathAddPoint(VECTOR(CurrentPoint.fX, CurrentPoint.fY, 0), VECTOR(0, 0, 1));
		//CurrentPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[0]);
		//CurrentPoint.fZ += .25f;
		BOUNDING_BOX PathBox;

		PathBox.vMin = CurrentPoint;
		PathBox.vMax = CurrentPoint;
		for ( int j = 0; j < NodeCount; j++ )
		{
			NextPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[j]);
			BoundingBoxExpandByPoint( &PathBox, &NextPoint );
		}

		BoundingBoxExpandByPoint( &PathBox, &vFixedDestination );			


		SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
		ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);
		Delta = PathBox.vMax - PathBox.vMin;
		Delta.fZ = 0;
		float Dist = VectorLength( Delta );
		VectorNormalize( Delta );
		LevelSortFaces(pGame, pLevel, SortedFaces, PathBox.vMin, Delta, Dist );


		ROOM *pRoom = UnitGetRoom( pUnit );
		VECTOR StartPoint = vFixedDestination;
		StartPoint.fZ += 200;
		float fTestDist = 400;
		VECTOR TestDelta = VECTOR(0, 0, -1);

		float fTestCollideDistance = LevelLineCollideLen(pGame, pLevel, SortedFaces, StartPoint, TestDelta, fTestDist, Material, &CollisionNormal);
		if( pLevel &&  pLevel->m_pLevelDefinition->bContoured && Material != PROP_MATERIAL &&
			fTestCollideDistance < fTestDist &&
			CollisionNormal.fZ > .1f )
		{
			fTestCollideDistance = fTestDist;
		}
		if( pRoom && fTestCollideDistance < fTestDist )
		{
			StartPoint.fZ -= fTestCollideDistance;
			if( !( StartPoint.fZ <= pLevel->m_pLevelDefinition->fMaxPathingZ &&
				StartPoint.fZ >= pLevel->m_pLevelDefinition->fMinPathingZ ) )
			{
				vFixedDestination = AStarGetWorldPosition( pGame, &path->m_pPathNodes[NodeCount - 1] );
			}
			else
			{
				vFixedDestination.fZ = ( StartPoint.fZ - fTestCollideDistance );
			}
		}


		for( int i = 0; i <= NodeCount; i++ )
		{
			bool Added( FALSE );
			if( i == path->m_nPathNodeCount )
			{
				NextPoint = vFixedDestination;
				NextPoint.fZ = pUnit->vPosition.fZ;
			} 
			else
			{
				NextPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[i]);
			}
			NextNextPoint = NextPoint;
			if( i < path->m_nPathNodeCount - 1 )
			{
				NextNextPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[i + 1] );
			}
			else if ( i == path->m_nPathNodeCount - 1 )
			{
				NextNextPoint = vFixedDestination;
				NextNextPoint.fZ = pUnit->vPosition.fZ;
			}
			NextPoint.fZ += .35f;
			NextNextPoint.fZ += .35f;
			Delta = NextNextPoint - CurrentPoint;
			//Delta.fZ = 0; 
			float fDist = VectorLength( Delta );
			VectorNormalize( Delta );

	 
			DWORD dwMoveFlags = 0;
			dwMoveFlags |= MOVEMASK_TEST_ONLY;
			dwMoveFlags |= MOVEMASK_SOLID_MONSTERS;
			dwMoveFlags |= MOVEMASK_NOT_WALLS;
			float fCollideDistance = fDist;

			fCollideDistance = LevelLineCollideLen(pGame, pLevel, SortedFaces, CurrentPoint, Delta, fDist, &CollisionNormal);

			if( fCollideDistance < fDist &&
				CollisionNormal.fZ > .1f )
			{
				fCollideDistance = fDist;
			}
			TestPoint = NextNextPoint;
			COLLIDE_RESULT result;

			if (fCollideDistance >= fDist)
			{
				RoomCollide(pUnit, CurrentPoint, Delta, &TestPoint, UnitGetCollisionRadius( pUnit ), 5.0f, dwMoveFlags, &result);
			}
			// OK, now let's make sure there's actual walkable ground here!
			if (fCollideDistance >= fDist)
			{
				ROOM *pRoom = UnitGetRoom( pUnit );
				VECTOR StartPoint = ( NextNextPoint - CurrentPoint ) / 2 + CurrentPoint;
				StartPoint.fZ += 200;
				float fTestDist = 400;
				VECTOR TestDelta = VECTOR(0, 0, -1);
				float fTestCollideDistance = LevelLineCollideLen(pGame, pLevel, SortedFaces, StartPoint, TestDelta, fTestDist, Material, &CollisionNormal);
				if( pRoom && fTestCollideDistance < fTestDist )
				{
					StartPoint.fZ -= fTestCollideDistance;
					if( !( StartPoint.fZ <= pLevel->m_pLevelDefinition->fMaxPathingZ &&
						   StartPoint.fZ >= pLevel->m_pLevelDefinition->fMinPathingZ ) ||
						   Material == PROP_MATERIAL )
					{
						fCollideDistance = 0;
					}
				}
				else
				{
					fCollideDistance = 0;
				}

			}

			if( i  != 0 &&				
				( fCollideDistance < fDist ||
				 TESTBIT(&result.flags, MOVERESULT_COLLIDE_UNIT) ||
				  i == NodeCount ) )
			{

				Added = TRUE;
				if( i == NodeCount && 
					fCollideDistance < fDist )
				{
					if( fCollideDistance > UnitGetCollisionRadius( pUnit ) )
					{
						CurrentPoint += Delta * ( fCollideDistance - UnitGetCollisionRadius( pUnit ) );
					}
					if( fCollideDistance < fDist )
					{
						AppendDestination = FALSE;
					}
				}
				else
				{
					CurrentPoint = NextPoint;
				}
				pUnit->m_pActivePath->CPathAddPoint(VECTOR(CurrentPoint.fX, CurrentPoint.fY, 0), VECTOR(0, 0, 1));
			}


		}
		SortedFaces.Destroy();

		path->m_fPathNodeDistance[0] = 0;
		float MinDistance = 0;
		float Distance;
		int Segment;
		VECTOR Point;
		VECTOR Perpendicular;
		VECTOR Up;

		for( int i = 0; i < NodeCount; i++ )
		{
			NextPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[i]);
			pUnit->m_pActivePath->FindNearestPoint( NextPoint, Point, Perpendicular, Up, Segment, Distance, MinDistance );
			MinDistance = Distance;
			path->m_fPathNodeDistance[i] = Distance;
		}
		pUnit->m_pActivePath->FindNearestPoint( CurrentPoint, Point, Perpendicular, Up, Segment, Distance, MinDistance );
		path->m_fPathNodeDistance[NodeCount] = Distance;
	}
	else
	{
		for( int i = 0; i < NodeCount; i++ )
		{
			NextPoint = AStarGetWorldPosition( pGame, &path->m_pPathNodes[i]);
			/*if( i == NodeCount - 1 )
			{
				ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(RoomGetByID(pGame, path->m_pPathNodes[i].nRoomId), path->m_pPathNodes[i].nPathNodeIndex);
				MaxDistance = pPathNode->fRadius;
			}*/ // don't know that we actually want to use the pathnode's real radius - this usually isn't set to anything good in Tug.
			
			CurrentPoint = NextPoint;
#if HELLGATE_ONLY
			pUnit->m_pActivePath->CPathAddPoint(VECTOR(CurrentPoint.fX, CurrentPoint.fY, AppIsHellgate() ? CurrentPoint.fZ : 0.0f), AppIsHellgate() ? AStarGetWorldNormal(pGame, &path->m_pPathNodes[i]) : VECTOR( 0, 0, 1 ) );
#else
			pUnit->m_pActivePath->CPathAddPoint(VECTOR(CurrentPoint.fX, CurrentPoint.fY, AppIsHellgate() ? CurrentPoint.fZ : 0.0f), VECTOR( 0, 0, 1 ) );
#endif
			path->m_fPathNodeDistance[pUnit->m_pActivePath->Points() - 1] = pUnit->m_pActivePath->GetPointDistance( pUnit->m_pActivePath->Points() - 1 );
		}
	}
	if( AppendDestination )
	{
		if( IS_CLIENT( pGame ) && AppIsTugboat() &&
			GameGetControlUnit( pGame ) == pUnit )
		{

			LEVEL* pLevel = UnitGetLevel( pUnit );
			// have to collide toward it
			Delta = vFixedDestination - CurrentPoint;
			float fDist = VectorLength( Delta );
			float fCollideDistance = LevelLineCollideLen(pGame, pLevel, CurrentPoint, Delta, fDist, &CollisionNormal);
			if( fCollideDistance >= fDist )
			{
				pUnit->m_pActivePath->CPathAddPoint(VECTOR(vFixedDestination.fX, vFixedDestination.fY, 0.0f), VECTOR(0, 0, 1));
				path->m_fPathNodeDistance[pUnit->m_pActivePath->Points() - 1] = pUnit->m_pActivePath->GetPointDistance( pUnit->m_pActivePath->Points() - 1 );
			}
		}
		else
		{
			// GS - THIS SHOULDN'T BE NORMAL OF (0, 0, 1)!
			pUnit->m_pActivePath->CPathAddPoint(VECTOR(vFixedDestination.fX, vFixedDestination.fY, AppIsHellgate() ? vFixedDestination.fZ : 0.0f), VECTOR(0, 0, 1));
			path->m_fPathNodeDistance[pUnit->m_pActivePath->Points() - 1] = pUnit->m_pActivePath->GetPointDistance( pUnit->m_pActivePath->Points() - 1 );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sTryAutoPickup(
	UNIT *pItem,
	void *pCallbackData)
{

	if (UnitTestFlag(pItem, UNITFLAG_CANBEPICKEDUP))
	{

		if (UnitTestFlag(pItem, UNITFLAG_AUTOPICKUP) != FALSE)
		{		
			UNIT *pPlayer = (UNIT *)pCallbackData;
			float flCollisionHeight = UnitGetCollisionHeight( pPlayer );	
						
			if(AppIsHellgate())
			{
				const float PICKUP_HEIGHT_GRACE	= 2.0f;
				if (pItem->vPosition.fZ < (pPlayer->vPosition.fZ - PICKUP_HEIGHT_GRACE) || 
					pItem->vPosition.fZ >= (pPlayer->vPosition.fZ + flCollisionHeight + PICKUP_HEIGHT_GRACE))
				{
					return RIU_CONTINUE;
				}
			}

			// must be close enough
			float flDistSq = VectorDistanceSquaredXY(pPlayer->vPosition, pItem->vPosition);
			const UNIT_DATA *pUnitData = UnitGetData( pItem );	
			if (pUnitData->flAutoPickupDistance > 0.0f && 
				flDistSq > pUnitData->flAutoPickupDistance * pUnitData->flAutoPickupDistance)
			{
				return RIU_CONTINUE;
			}

			if (ItemCanPickup(pPlayer, pItem) == PR_OK)
			{
#if !ISVERSION( CLIENT_ONLY_VERSION )
				s_ItemPullToPickup(pPlayer, pItem);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
			}

		}
		
	}

	return RIU_CONTINUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAutoPickupInRoom(
	UNIT *pUnit,
	ROOM *pRoom)
{		
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	RoomIterateUnits( pRoom, eTargetTypes, sTryAutoPickup, pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DEBUG_PICKUP

#include "s_gdi.h"
#include "debugwindow.h"
#include "colors.h"

#define DRB_DEBUG_SCALE		5.0f

static BOOL drbPickupDebugInited = FALSE;
static UNIT * drbPlayer = NULL;
BOOL gDrawNearby = FALSE;

static void drbRoomDebugDraw( ROOM * room )
{
	if ( !room )
		return;

	int x1 = (int)FLOOR( ( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX - drbPlayer->vPosition.fX ) * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_X_CENTER;
	int y1 = (int)FLOOR( ( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY - drbPlayer->vPosition.fY ) * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_Y_CENTER;
	int x2 = (int)FLOOR( ( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX - drbPlayer->vPosition.fX ) * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_X_CENTER;
	int y2 = (int)FLOOR( ( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY - drbPlayer->vPosition.fY ) * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_Y_CENTER;

	gdi_drawbox( x1, y1, x2, y2, gdi_color_white );

	char szTemp[128];
	PStrPrintf( szTemp, 128, "%i", room->idRoom );
	gdi_drawstring( x1+2, y1+2, szTemp, gdi_color_white );
}

static void drbPickupDebugDraw()
{
	if ( !drbPlayer )
		return;

	if ( UnitIsInLimbo( drbPlayer ) )
		return;

	// get nearby rooms
	ROOM * room = UnitGetRoom( drbPlayer );	
	ROOM_LIST tNearbyRooms;
	RoomGetNearbyRoomList(room, &tNearbyRooms, TRUE);

	for (int i = 0; i < tNearbyRooms.nNumRooms; ++i)
	{
		ASSERT_BREAK(i < MAX_ROOMS_PER_LEVEL);
		ROOM *pRoom = tNearbyRooms.pRooms[ i ];
		drbRoomDebugDraw( pRoom );
	}

	int x1 = DEBUG_WINDOW_X_CENTER;
	int y1 = DEBUG_WINDOW_Y_CENTER;
	gdi_drawpixel( x1, y1, gdi_color_red );
	int nRadius = (int)FLOOR( ( UnitGetCollisionRadius( drbPlayer ) * DRB_DEBUG_SCALE * 0.6f ) + 0.5f );
	gdi_drawcircle( x1, y1, nRadius, gdi_color_red );

	VECTOR vFace = drbPlayer->vFaceDirection * ( UnitGetCollisionRadius( drbPlayer ) * 1.5f * 0.6f );
	int x2 = (int)FLOOR( vFace.fX * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_X_CENTER;
	int y2 = (int)FLOOR( vFace.fY * DRB_DEBUG_SCALE ) + DEBUG_WINDOW_Y_CENTER;
	gdi_drawline( x1, y1, x2, y2, gdi_color_red );
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitAutoPickupObjectsAround(
	UNIT *pUnit)
{
	ASSERT_RETURN( pUnit );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( IS_SERVER(pGame) );

#ifdef DEBUG_PICKUP
	if ( gDrawNearby && !drbPickupDebugInited )
	{
		drbPlayer = pUnit;
		DaveDebugWindowSetDrawFn( drbPickupDebugDraw );
		DaveDebugWindowShow();
		drbPickupDebugInited = TRUE;
	}
#endif

	if (!UnitTestFlag( pUnit, UNITFLAG_CANPICKUPSTUFF))
	{
		return;
	}

	if (UnitIsInLimbo( pUnit ))
	{
		return;
	}
		
	ROOM *pRoomCenter = UnitGetRoom( pUnit );
	if (pRoomCenter == NULL)
	{
		return;
	}

	// pickup stuff in nearby rooms
	int nNumNeighborRooms = RoomGetAdjacentRoomCount(pRoomCenter);
	for (int i = -1; i < nNumNeighborRooms; ++i)
	{
		ROOM *pRoom = i == -1 ? pRoomCenter : RoomGetAdjacentRoom(pRoomCenter, i);
		if (pRoom)
		{
			sDoAutoPickupInRoom( pUnit, pRoom );		
		}
		
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitForceToGround(
	UNIT* unit )
{
	ASSERTX_RETURN( unit, "Expected unit" );
	GAME *game = UnitGetGame( unit );
	VECTOR vNewPosition = UnitGetPosition(unit);
	if( AppIsHellgate() )
	{
		DWORD dwFlags = MOVEMASK_FORCEGROUND;
		if(UnitTestFlag(unit, UNITFLAG_PATHING) == TRUE)
		{
			dwFlags |= MOVEMASK_FLOOR_ONLY;
		}
		VECTOR vZero = VECTOR(0.0f, 0.0f, 0.0f);
		COLLIDE_RESULT result;

		RoomCollide(
			unit, 
			unit->vPosition, 
			vZero, 
			&vNewPosition,
			UnitGetPathingCollisionRadius( unit ), 
			UnitGetCollisionHeight( unit ), 
			dwFlags, 
			&result);
	}
	else
	{

		VECTOR Normal( 0, 0, -1 );
		VECTOR Start = vNewPosition;
		LEVEL* pLevel = UnitGetLevel( unit );
		if( pLevel )
		{
			Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
			float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
			VECTOR vCollisionNormal;
			int Material;
			float fCollideDistance = LevelLineCollideLen( game, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
			if (fCollideDistance < fDist &&
				Material != PROP_MATERIAL &&
				Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
				Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
			{
				vNewPosition.fZ = Start.fZ - fCollideDistance;
			}
		}
		
	}
	UnitSetOnGround(unit, TRUE);

	UnitChangePosition(unit, vNewPosition);

	if(IS_CLIENT(game))
	{
		UnitChangePosition(unit, unit->vPosition);
		c_UnitUpdateGfx(unit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline DWORD sUnitGetMoveFlags(
	UNIT* unit)
{
	return unit->pdwMovementFlags[0];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static int c_SearchPathList( PATH_STATE * path, int start, int end, ROOMID roomid, unsigned int index )
{
	for (int i = start; i < end; i++)
	{
		if ( ( path->m_pPathNodes[i].nRoomId == roomid ) && ( path->m_pPathNodes[i].nPathNodeIndex == index ) )
		{
			return i;
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR CalculatePathTargetFly( GAME * game, float fAltitude, float fCollisionHeight, ROOM *room, ROOM_PATH_NODE *roomPathNode )
{
	VECTOR target(0);
	ASSERT_RETVAL( roomPathNode, target );

	target = RoomPathNodeGetExactWorldPosition( game, room, roomPathNode );
#if HELLGATE_ONLY
	VECTOR normal = AppIsHellgate() ? RoomPathNodeGetWorldNormal( game, room, roomPathNode ) : VECTOR( 0, 0, 1 );
	// scale node normal by altitude
	normal *= 
		MIN(																	// take the smaller of...
		MAX( 0.0f, roomPathNode->fHeight - ( fCollisionHeight + 1.0f ) ),	// max height allowed for collision height, plus some room to avoid collision
		MAX( 0.0f, fAltitude ) );											// and desired altitude
#else
	VECTOR normal( 0, 0, 1 );
#endif

	target += normal;
	return target;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitCalculatePathTargetFly( GAME * game, UNIT * unit, ROOM *room, ROOM_PATH_NODE *roomPathNode )
{
	return CalculatePathTargetFly( 
				game, 
				UnitGetStatFloat( unit, STATS_ALTITUDE ), 
				UnitGetCollisionHeight( unit ),
				room,
				roomPathNode );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitCalculatePathTargetFly( GAME * game, UNIT * unit, ROOMID nRoomId, int nPathNodeIndex )
{
	ROOM *room = RoomGetByID( game, nRoomId );
	ASSERT_RETVAL( room, cgvNull );
	ROOM_PATH_NODE *roomPathNode = RoomGetRoomPathNode( room, nPathNodeIndex );
	ASSERT_RETVAL( roomPathNode, cgvNull );

	return UnitCalculatePathTargetFly( game, unit, room, roomPathNode );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitCalculatePathTargetFly( GAME * game, UNIT * unit, ASTAR_OUTPUT * outputNode )
{
	ASSERT_RETVAL( outputNode, cgvNull );
	return UnitCalculatePathTargetFly( game, unit, outputNode->nRoomId, outputNode->nPathNodeIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static void s_UnitPathMoveDirection( GAME * game, UNIT * unit, VECTOR & direction )
{
	PATH_STATE* path = unit->m_pPathing;
	if (path->m_nCurrentPath >= 0 && 
		path->m_nCurrentPath < path->m_nPathNodeCount && 
		path->m_eState == PATHING_STATE_ACTIVE )
	{
		VECTOR position = UnitGetPosition( unit );
		if ( TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) )
		{
			VECTOR target = UnitCalculatePathTargetFly( game, unit, &path->m_pPathNodes[path->m_nCurrentPath] );
			direction = target - position;
		}
		else
		{
			VECTOR target = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nCurrentPath] );
			direction = target - position;
			direction.fZ = 0.0f;
		}
	}
}

#if 0
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static BOOL c_UnitPathMoveDirection(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vDirection,
	VECTOR & vFaceDirection,
	VECTOR & vUp,
	BOOL bRealStep,
	BOOL bIsJumping)
{
	PATH_STATE* pPathState = pUnit->m_pPathing;

	if ( pPathState->m_eState != PATHING_STATE_ACTIVE && pPathState->m_eState != PATHING_STATE_ACTIVE_CLIENT )
	{
		// when not jumping, we clear the direction
		if (bIsJumping == FALSE)
		{
			vDirection = cgvNone;	
		}
		vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		return FALSE;
	}

	if(TESTBIT( pUnit->pdwMovementFlags, MOVEFLAG_FLY ))
	{
		VECTOR vTarget;
		VECTOR vPosition = UnitGetPosition( pUnit );
		ASTAR_OUTPUT *pPathNode = &pPathState->m_pPathNodes[pPathState->m_nCurrentPath];
		vTarget = UnitCalculatePathTargetFly( pGame, pUnit, pPathNode );
		vDirection = vTarget - vPosition;
		vUp = VECTOR(0, 0, 1);
		pPathState->vLastDirection = vDirection;
		return TRUE;
	}

	VECTOR vNearestPoint;
	VECTOR vPerpendicular;
	int		nSegment;
	float	fDistance;
	float	fAheadDistance;
	CPath* pPath = pUnit->m_pActivePath;
	VECTOR vClosestPosition;
	VECTOR vAheadClosestPosition;
	VECTOR vFlatPosition = UnitGetPosition(pUnit);
	if(AppIsTugboat())
	{
		vFlatPosition.fZ = 0;
	}
	VECTOR vAheadPosition = vFlatPosition + UnitGetFaceDirection( pUnit, FALSE );

	float fCurrentPathDistance = UnitGetStatFloat(pUnit, STATS_PATH_DISTANCE);

	// find the nearest point to our current location
	pPath->FindNearestPoint( vFlatPosition, vClosestPosition, vPerpendicular, vUp, nSegment, fDistance, fCurrentPathDistance, fCurrentPathDistance + MAX_PATH_NODE_FUDGE );

	pPath->FindNearestPoint( vAheadPosition, vAheadClosestPosition, vPerpendicular, vUp, nSegment, fAheadDistance, fCurrentPathDistance + MAX_PATH_NODE_FUDGE, fCurrentPathDistance + 2.0f*MAX_PATH_NODE_FUDGE );

	UnitSetStatFloat(pUnit, STATS_PATH_DISTANCE, 0, fDistance);

	if( fAheadDistance <= fDistance )
	{
		vAheadPosition = pPath->GetPositionAtDistance( fDistance + .1f );
	}
	else
	{
		if( VectorLength( vPerpendicular ) > MAX_PATH_NODE_FUDGE )
		{
			VectorNormalize( vPerpendicular );
			vAheadPosition = vAheadClosestPosition + vPerpendicular * MAX_PATH_NODE_FUDGE;
		}
	}

	// if we have reached the end of the path, stop pathing
	if( pPath->Points() == 0  ||
		fDistance >= pPath->Length() ||
		pPath->Length() == 0 )
	{
		vDirection = cgvNone;
		vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		pPathState->vLastDirection = vDirection;
		pUnit->m_fPathDistance = pPath->Length();
	}
	else	// otherwise, find a point ahead of us, and turn toward it.
	{
		float fTargetDistance = fDistance + .1f;
		if( fTargetDistance < pUnit->m_fPathDistance )
		{
			fTargetDistance = pUnit->m_fPathDistance;
		}

		if( fTargetDistance > pPath->Length() )
		{
			fTargetDistance = pPath->Length();
		}
		pUnit->m_fPathDistance = fTargetDistance;

		vNearestPoint = vAheadPosition;
		VECTOR vTargetPosition = vAheadPosition;

		VECTOR vUnitPosition = UnitGetPosition(pUnit);
		vFaceDirection = vNearestPoint - vUnitPosition;
		vFaceDirection.fZ = 0;
		vDirection = vTargetPosition - vUnitPosition;
		VectorNormalize( vFaceDirection );
		if( VectorIsZeroEpsilon( vDirection ) )
		{ 
			vDirection = cgvNone;
			vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
			pPathState->vLastDirection = vDirection;
			pUnit->m_fPathDistance = pPath->Length();
			return TRUE;
		}
		if( VectorIsZeroEpsilon( vFaceDirection ) )
		{
			vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
			pUnit->m_fPathDistance = pPath->Length();
		}
		VectorNormalize( vDirection );

		// PROBLEM - THE UP DIRECTION ISN'T RIGHT HERE!

		if(!UnitDataTestFlag(UnitGetData(pUnit), UNIT_DATA_FLAG_ANGLE_WHILE_PATHING))
		{
			vDirection.fZ = 0.0f;
			vUp = VECTOR(0, 0, 1);
		}
		/*
		else
		{
			VECTOR vCross;
			VectorCross(vCross, vDirection, VECTOR(0, 0, 1));
			VectorCross(vUp, vCross, vDirection);
			VectorNormalize(vUp);
		}
		// */

		pPathState->vLastDirection = vDirection;
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static BOOL c_UnitPathMoveDirection( GAME * game, UNIT * unit, VECTOR & direction, VECTOR & up, BOOL bRealStep, BOOL bIsJumping )
{
	PATH_STATE* path = unit->m_pPathing;

	//ASSERT( ( path->m_eState == PATHING_STATE_ACTIVE ) || !bRealStep );
	if ( path->m_eState != PATHING_STATE_ACTIVE && path->m_eState != PATHING_STATE_ACTIVE_CLIENT )
	{
		// when not jumping, we clear the direction
		if (bIsJumping == FALSE)
		{
			direction = cgvNone;	
		}
		return FALSE;
	}

	VECTOR target;
	VECTOR position = UnitGetPosition( unit );
	ASTAR_OUTPUT *pathNode = NULL;
#if HELLGATE_ONLY
	if ( path->cFastPathIndex != INVALID_ID )
	{
		pathNode = &path->cFastPathNodes[path->cFastPathIndex];
	}
	else
#endif
	{
		// default place to move to...
		pathNode = &path->m_pPathNodes[path->m_nCurrentPath];
	}

	if ( TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) )
	{
		target = UnitCalculatePathTargetFly( game, unit, pathNode );
	}
	else
	{
		target = AStarGetWorldPosition( game, pathNode );
	}

	/*
	if ( path->cFastPathIndex != INVALID_ID )
	{
		//trace( "target (f=%i/%i)(%.2f,%.2f,%.2f)", path->cFastPathIndex, path->cFastPathSize, target.x, target.y, target.z );
	}
	else
	{
		// default place to move to...
		//trace( "target (n=%i/%i)(%.2f,%.2f,%.2f)", path->m_nCurrentPath, path->m_nPathNodeCount, target.x, target.y, target.z );
	}
	*/

	direction = target - position;
	if ( !TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) )
	{
		if(!UnitDataTestFlag(UnitGetData(unit), UNIT_DATA_FLAG_ANGLE_WHILE_PATHING))
		{
			direction.fZ = 0.0f;
			up = VECTOR(0, 0, 1);
		}
		else
		{
			VECTOR vCross;
			VectorCross(vCross, direction, VECTOR(0, 0, 1));
			VectorCross(up, vCross, direction);
			VectorNormalize(up);
		}
	}
	else
	{
		up = VECTOR(0, 0, 1);
	}

	//trace( "pos(%.2f,%.2f,%.2f) dir(%.2f,%.2f,%.2f)\n", position.x, position.y, position.z, direction.x, direction.y, direction.z );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static void c_UnitPathMoveDirectionTugboat( GAME * game, UNIT * unit, VECTOR & direction, VECTOR & facedirection, BOOL bRealStep )
{
	PATH_STATE* path = unit->m_pPathing;

	if ( path->m_eState != PATHING_STATE_ACTIVE )
	{
		direction = VECTOR(0.0f,0.0f,0.0f);
		facedirection = UnitGetFaceDirection( unit, FALSE );
		return;
	}

 	VECTOR NearestPoint;
	VECTOR Perpendicular;
	VECTOR Up;
	int		Segment;
	float	Distance;
	float	AheadDistance;
	CPath* pPath = unit->m_pActivePath;
	VECTOR ClosestPosition;
	VECTOR AheadClosestPosition;
	VECTOR vFlatPosition = unit->vPosition;
	vFlatPosition.fZ = 0;
	VECTOR vAheadPosition = vFlatPosition + UnitGetFaceDirection( unit, FALSE );
	// find the nearest point to our current location
	pPath->FindNearestPoint( vFlatPosition, ClosestPosition, Perpendicular, Up, Segment, Distance );
	
	pPath->FindNearestPoint( vAheadPosition, AheadClosestPosition, Perpendicular, Up, Segment, AheadDistance );

	if( AheadDistance <= Distance )
	{
		vAheadPosition = pPath->GetPositionAtDistance( Distance + .1f );
	}
	else
	{
		if( VectorLength( Perpendicular ) > MAX_PATH_NODE_FUDGE )
		{
			VectorNormalize( Perpendicular, Perpendicular );
			vAheadPosition = AheadClosestPosition + Perpendicular * MAX_PATH_NODE_FUDGE;
		}
	}
	
	// if we have reached the end of the path, stop pathing
	if( pPath->Points() == 0  ||
		Distance >= pPath->Length() ||
		pPath->Length() == 0 )
	{
		direction = VECTOR(0.0f,0.0f,0.0f);
		facedirection = UnitGetFaceDirection( unit, FALSE );
		unit->m_fPathDistance = pPath->Length();
	}
	else	// otherwise, find a point ahead of us, and turn toward it.
	{
		float TargetDistance = Distance + .1f;
		if( TargetDistance < unit->m_fPathDistance )
		{
			TargetDistance = unit->m_fPathDistance;
		}

		if( TargetDistance > pPath->Length() )
		{
			TargetDistance = pPath->Length();
		}
		unit->m_fPathDistance = TargetDistance;

		NearestPoint = vAheadPosition;
		VECTOR TargetPosition = vAheadPosition;

		VECTOR vUnitPosition = UnitGetPosition(unit);
		facedirection = NearestPoint - vUnitPosition;
		facedirection.fZ = 0;
		direction = TargetPosition - vUnitPosition;
		direction.fZ = 0;
		VectorNormalize( facedirection );
		if( VectorIsZeroEpsilon( direction ) )
		{ 
			direction = VECTOR( 0, 0, 0 );
			facedirection = UnitGetFaceDirection( unit, FALSE );
			unit->m_fPathDistance = pPath->Length();
			return;
		}
		if( VectorIsZeroEpsilon( facedirection ) )
		{
			facedirection = UnitGetFaceDirection( unit, FALSE );
			unit->m_fPathDistance = pPath->Length();
		}
		VectorNormalize( direction );

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static void s_UnitPathMoveDirection( GAME * game, UNIT * unit, VECTOR & direction, VECTOR & facedirection )
{
	PATH_STATE* path = unit->m_pPathing;
	if ( path->m_eState == PATHING_STATE_ACTIVE )
	{
		VECTOR NearestPoint;
		VECTOR Perpendicular;
		VECTOR Up;
		int		Segment;
		float	Distance;
		float	AheadDistance;
		CPath* pPath = unit->m_pActivePath;
		VECTOR ClosestPosition;
		VECTOR AheadClosestPosition;
		VECTOR vFlatPosition = unit->vPosition;
		vFlatPosition.fZ = 0;
		VECTOR vAheadPosition = vFlatPosition + UnitGetFaceDirection( unit, FALSE );
		// find the nearest point to our current location
		pPath->FindNearestPoint( vFlatPosition, ClosestPosition, Perpendicular, Up, Segment, Distance );
		
		pPath->FindNearestPoint( vAheadPosition, AheadClosestPosition, Perpendicular, Up, Segment, AheadDistance );

		if( AheadDistance <= Distance )
		{

			vAheadPosition = pPath->GetPositionAtDistance( Distance + .1f );
		
		}
		else
		{
			if( VectorLength( Perpendicular ) > .2f )
			{
				VectorNormalize( Perpendicular, Perpendicular );
				vAheadPosition = AheadClosestPosition + Perpendicular * .2f;
				//pPath->FindNearestPoint( vAheadPosition, AheadClosestPosition, Perpendicular, Segment, AheadDistance );
				//vAheadPosition = AheadClosestPosition;
			}
		}	
		// if we have reached the end of the path, stop pathing
		if( pPath->Points() == 0  ||
			Distance >= pPath->Length() ||
			pPath->Length() == 0 )
		{
			direction = VECTOR(0.0f,0.0f,0.0f);
			facedirection = UnitGetFaceDirection( unit, FALSE );
			unit->m_fPathDistance = pPath->Length();
		}
		else	// otherwise, find a point ahead of us, and turn toward it.
		{
			float TargetDistance = Distance + .1f;
			if( TargetDistance < unit->m_fPathDistance )
			{
				TargetDistance = unit->m_fPathDistance;
			}


			if( TargetDistance > pPath->Length() )
			{
				TargetDistance = pPath->Length();
			}
			unit->m_fPathDistance = TargetDistance;

			NearestPoint = vAheadPosition;
			VECTOR TargetPosition = vAheadPosition;

			facedirection = NearestPoint -  unit->vPosition;
			facedirection.fZ = 0;
			direction = TargetPosition - unit->vPosition;
			direction.fZ = 0;
			VectorNormalize( facedirection );
			if( VectorIsZeroEpsilon( direction ) )
			{ 
				direction = VECTOR( 0, 0, 0 );
				facedirection = UnitGetFaceDirection( unit, FALSE );
				unit->m_fPathDistance = pPath->Length();
				return;
			}
			if( VectorIsZeroEpsilon( facedirection ) )
			{
				facedirection = UnitGetFaceDirection( unit, FALSE );
				unit->m_fPathDistance = pPath->Length();
			}
			VectorNormalize( direction );

		}
		
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHomingGetMoveDirection(
	UNIT *pMissile,
	VECTOR *pvDirection,
	float *pflSpeed,
	BOOL *pbHavok)
{
	*pbHavok = FALSE;
#ifdef HAVOK_ENABLED
	if (AppIsHellgate() && pMissile->pHavokRigidBody)
	{
		HavokRigidBodyGetVelocity( pMissile->pHavokRigidBody, *pvDirection );
		*pbHavok = TRUE;
	}
	else
#endif
	{
		*pvDirection = UnitGetMoveDirection( pMissile );
	}

	*pflSpeed = VectorNormalize( *pvDirection );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHomingSetMoveDirection(
	UNIT *pMissile,
	const VECTOR &vDirection,
	float flSpeed)
{
#ifdef HAVOK_ENABLED
	if (AppIsHellgate() && pMissile->pHavokRigidBody)
	{
		VECTOR vVelocity;
		VectorScale( vVelocity, vDirection, flSpeed );
		HavokRigidBodySetVelocity( pMissile->pHavokRigidBody, &vVelocity );
	}
	else
#endif
	{
		UnitSetMoveDirection( pMissile, vDirection );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateHoming(
	UNIT *pMissile,
	VECTOR *pNewMoveDirection)
{		
	ASSERTX_RETURN( pMissile, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pMissile, UNITTYPE_MISSILE ), "Expected missile" );
		
	UNIT *pTarget = MissileGetHomingTarget( pMissile );
	if (pTarget)
	{
		VECTOR vTargetPos = UnitGetAimPoint( pTarget );
		VECTOR vMoveDirection;
		float flSpeed;
		BOOL bHavok = FALSE;
		sHomingGetMoveDirection( pMissile, &vMoveDirection, &flSpeed, &bHavok );
		float flOriginalZ = vMoveDirection.fZ;
				
		// get vector to target
		VECTOR vToTarget = vTargetPos - UnitGetPosition( pMissile );
		VectorNormalize( vToTarget );
		
		// find angle between
		float flDot = VectorDot( vToTarget, vMoveDirection );

		// if we're not "close enough" to heading toward the target, do the homing logic		
		if (flDot < 0.98f)
		{
			GAME *pGame = UnitGetGame( pMissile );
			const UNIT_DATA *pMissileData = MissileGetData( pGame, pMissile );
			ASSERTX_RETURN( pMissileData, "Expected missile data" );

			// how much can we turn
			flDot = PIN( flDot, -1.0f, 1.0f );
			float flCurrentAngleBetween = acos( flDot );			
			float flTurnAngle = pMissileData->flHomingTurnAngleInRadians;
			if( pMissileData->flHomingTurnAngleInRadiansModByDis > 0 )
			{
				float fDistanceTraveledPct = pMissileData->fRangeBase * MissileGetRangePercent(pMissile, NULL, pMissileData);
				fDistanceTraveledPct = 1.0f - ( UnitGetStatFloat(pMissile, STATS_MISSILE_DISTANCE )/fDistanceTraveledPct );
				if( fDistanceTraveledPct > 1.0f ) //this can happen when STATS_MISSILE_DISTANCE is negative
					fDistanceTraveledPct = 0;
				flTurnAngle += pMissileData->flHomingTurnAngleInRadiansModByDis * fDistanceTraveledPct;
			}

			if (flCurrentAngleBetween < flTurnAngle)
			{
				flTurnAngle = flCurrentAngleBetween;
			}
												
			// find perpendicular vector
			VECTOR vPerp;
			VectorCross( vPerp, vToTarget, vMoveDirection );
			VectorNormalize( vPerp );

			// create matrix for transformation about Z axis
			MATRIX m, mInverse;
			MatrixFromVectors( m, cgvNone, vPerp, vMoveDirection );
			MatrixInverse( &mInverse, &m );

			// transform move direction into new coord system
			VECTOR vNewDirectionOrigin;
			MatrixMultiply( &vNewDirectionOrigin, &vMoveDirection, &mInverse );

			// rotate towards target
			VectorZRotation( vNewDirectionOrigin, -flTurnAngle );

			// put back in world space
			VECTOR vNewDirection;					
			MatrixMultiply( &vNewDirection, &vNewDirectionOrigin, &m );
				
			// leave z intact if there is physics
			if (AppIsHellgate() && bHavok)
			{
				vNewDirection.fZ = flOriginalZ;
			}
			
			// set the new move direction
			sHomingSetMoveDirection( pMissile, vNewDirection, flSpeed );
			if (pNewMoveDirection)
			{
				*pNewMoveDirection = vNewDirection;
			}
																
		}
									
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
VECTOR UnitCalculateMoveDirection(
	GAME* game, 								  
	UNIT* unit,
	BOOL bRealStep)
{
	
	UNIT* target = UnitGetById(game, UnitGetMoveTargetId(unit));
	if ((!target && (!unit->m_pPathing || UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))))
	{
		VECTOR vMoveDirection = UnitGetMoveDirection(unit);
		if(!UnitIsOnGround(unit))
		{
			vMoveDirection.fZ = 0.0f;
			VectorNormalize(vMoveDirection);
		}
		/*
		Not sure if this is the best place for this...
		*/
		if( IS_CLIENT( unit ) &&
			UnitDataTestFlag( unit->pUnitData, UNIT_DATA_FLAG_FORCE_DRAW_TO_MATCH_VELOCITY ) )
		{
			UnitSetFaceDirection( unit, vMoveDirection, TRUE );
		}
		return vMoveDirection;
		
	}

	VECTOR position = UnitGetPosition(unit);
	VECTOR targetpos = (target) ? UnitGetPosition(target) : unit->vMoveTarget;
	if (target && TESTBIT(unit->pdwMovementFlags, MOVEFLAG_FLY))
	{
		targetpos.fZ += UnitGetCollisionHeight(target) / 2.0f;
	}
	VECTOR direction = targetpos - position;
	VECTOR facedirection = direction;
	VECTOR up(0, 0, 1);

	if (unit->m_pPathing && !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
	{
		if ( IS_CLIENT( game ) )
		{
			/*
			BOOL bIsJumping = UnitIsJumping( unit );
			c_UnitPathMoveDirection( game, unit, direction, facedirection, up, bRealStep, bIsJumping);
			// */
			//*
			if(AppIsHellgate())
			{
				BOOL bIsJumping = UnitIsJumping( unit );
				c_UnitPathMoveDirection( game, unit, direction, up, bRealStep, bIsJumping );
			}
			else
			{
				c_UnitPathMoveDirectionTugboat( game, unit, direction, facedirection, bRealStep );
			}
			// */
		}
		else
		{
			if(AppIsHellgate())
			{
				s_UnitPathMoveDirection( game, unit, direction );
			}
			else
			{
				s_UnitPathMoveDirection( game, unit, direction, facedirection );
			}
		}
	}

	if (VectorIsZeroEpsilon(direction))
	{
		return VECTOR( 0, 0, 0 );
	}
	
	VectorNormalize(direction, direction);

	if ( bRealStep && !VectorIsZero(facedirection) )
	{
		if(AppIsHellgate())
		{
			if (unit->m_pPathing)
			{
				if (UnitTestMode(unit, MODE_BACKUP))
				{
					UnitSetFaceDirection(unit, -direction);
				}
				else if (!UnitTestMode(unit, MODE_STRAFE_LEFT) && !UnitTestMode(unit, MODE_STRAFE_RIGHT))
				{
					if(UnitDataTestFlag(UnitGetData(unit), UNIT_DATA_FLAG_ANGLE_WHILE_PATHING))
					{
						UnitSetFaceDirection(unit, direction, FALSE, FALSE, FALSE);
						UnitSetUpDirection(unit, up);
					}
					else
					{
						UnitSetFaceDirection(unit, direction);
					}
				}
			}
			else
			{
				UnitSetFaceDirection(unit, direction);
			}
		}
		else
		{
			UnitSetFaceDirection(unit, facedirection);
		}
	}
	return direction;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_SPLINE_PATHING

#include "spline.h"

static void UnitCalculateSplinePosition(
	UNIT* unit,
	float fTimeDelta,
	VECTOR & vNewPosition )
{
	ASSERT_RETURN( AppIsHellgate() );
	ASSERT_RETURN( unit );
	ASSERT_RETURN( unit->m_pPathing );
	ASSERT_RETURN( !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING) );

	VECTOR vOldPosition = unit->vPosition;
	vNewPosition = unit->vPosition;

	GAME * game = UnitGetGame( unit );
	PATH_STATE * path = unit->m_pPathing;

	if ( path->m_nCurrentPath < 0 )
		return;
	if ( path->m_nCurrentPath >= path->m_nPathNodeCount )
		return;
	if ( path->m_eState != PATHING_STATE_ACTIVE )
		return;

	float velocity = UnitGetVelocity( unit );

	// how much time is it going to take to do this part of the path?
	VECTOR start;
	if ( path->m_nCurrentPath != 0 )
		start = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nCurrentPath-1] );
	else
		start = path->vStart;
	VECTOR end = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nCurrentPath] );

	float fDistanceSquared = VectorDistanceSquaredXY( start, end );
	if ( fDistanceSquared == 0 )
		return;
	float fTotalTime = sqrtf( fDistanceSquared ) / velocity;
	TIME tElapsedTime = AppCommonGetCurTime() - path->m_Time;
	float fElapsedTime = (float)tElapsedTime / MSECS_PER_SEC;
	float t = fElapsedTime / fTotalTime;
	if ( t < 0.0f ) t = 0.0f;
	if ( t > 1.0f ) t = 1.0f;

	//if ( IS_SERVER( game ) )
	//	trace( "distance=%.2f, total_time=%.2f, elapsed_time=%.2f, t=%.2f\n", fDistanceSquared, fTotalTime, fElapsedTime, t );

	VECTOR vKnots[4];
	int index = path->m_nCurrentPath - 2;
	if ( index < 0 )
		vKnots[0] = path->vStart;
	else
		vKnots[0] = AStarGetWorldPosition( game, &path->m_pPathNodes[index] );
	vKnots[1] = start;
	vKnots[2] = end;
	index = path->m_nCurrentPath + 1;
	if ( index >= path->m_nPathNodeCount )
		vKnots[3] = vKnots[2];
	else
		vKnots[3] = AStarGetWorldPosition( game, &path->m_pPathNodes[index] );
	VECTOR direction = VECTOR(0.0f,0.0f,0.0f);
	BOOL bForceDrawn = FALSE;
	if ( IS_CLIENT( game ) )
	{
		SplineGetCatmullRomPosition( &vKnots[0], 4, 1, t, &vNewPosition, &direction );
		bForceDrawn = TRUE;
	}
	else
	{
		SplineGetCatmullRomPosition( &vKnots[0], 4, 1, t, &vNewPosition );
		direction = vNewPosition - vOldPosition;
	}

	direction.z = 0.0f;
	if ( !VectorIsZero( direction ) )
		VectorNormalize( direction );

	if ( VectorIsZero( direction ) )
		return;

	if ( UnitTestMode( unit, MODE_BACKUP ) )
	{
		UnitSetFaceDirection( unit, -direction );
	}
	else if ( !UnitTestMode( unit, MODE_STRAFE_LEFT ) && !UnitTestMode( unit, MODE_STRAFE_RIGHT ) )
	{
		if( UnitDataTestFlag( UnitGetData( unit ), UNIT_DATA_FLAG_ANGLE_WHILE_PATHING ) )
		{
			VECTOR up = VECTOR(0.0f,0.0f,1.0f);
			UnitSetFaceDirection(unit, direction, FALSE, FALSE, FALSE);
			UnitSetUpDirection(unit, up);
		}
		else
		{
			UnitSetFaceDirection(unit, direction, bForceDrawn);
		}
	}
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCalculateTargetReached(
	GAME* game,
	UNIT* unit,
	const VECTOR& vPositionPrev,
	VECTOR& vNewPosition,
	BOOL bCollided,
	BOOL bOldOnGround,
	BOOL bOnGround)
{
	BOOL bReturnValue = FALSE;
	BOOL bHasTarget = TRUE;
	BOOL bReachedTarget = FALSE;
	BOOL bStoppedMotion = FALSE;
	BOOL bPrematureStop = FALSE;

	if(UnitTestFlag(unit, UNITFLAG_PATHING))
	{
		PATH_STATE * path = unit->m_pPathing;
		if(path)
		{
			if ( path->m_eState == PATHING_STATE_ABORT &&
				path->m_nPathNodeCount > 0 )
			{
				drbSuperTracker( unit );
				bReachedTarget = TRUE;
			}
			else if (AppIsHellgate() && path->m_nCurrentPath >= path->m_nPathNodeCount)
			{
				drbSuperTracker( unit );
				bReachedTarget = TRUE;
			}
			// am I close enough to my target unit?
			else if ( unit->idMoveTarget != INVALID_ID  )
			{
				// in Tugboat, we don't do these checks on the client for monsters.
				// they cause no end of synch issues. Just go to your original target!
				// if the server stops you to attack somebody, well, you'll stop!
				if( !( AppIsTugboat() &&
					   UnitIsA( unit, UNITTYPE_MONSTER ) && IS_CLIENT( game ) ) )
				{
					UNIT * target = UnitGetById( game, unit->idMoveTarget );
					// The target may not exist, and we should be okay with that
					if(target)
					{
						float fDistanceToTargetSquared = AppIsHellgate() ? UnitsGetDistanceSquared(unit, target) : UnitsGetDistanceSquaredXY(unit, target);
						float fStopDistance = UnitGetStatFloat(unit, STATS_STOP_DISTANCE);
						if (fStopDistance == 0.0f)
						{
							fStopDistance = UNIT_DISTANCE_CLOSE;
						}
						float fMinDistance = UnitGetCollisionRadius( unit ) + fStopDistance;

						// take into account target collision if we must
						if (UnitGetStat(unit, STATS_STOP_DISTANCE_IGNORE_TARGET_COLLISION ) == FALSE)
						{
							fMinDistance += UnitGetCollisionRadius( target );
						}

						if (fDistanceToTargetSquared < fMinDistance * fMinDistance)
						{
							bReachedTarget = TRUE;
							bPrematureStop = TRUE;
						}
						drbSuperTrackerI( unit, (int)bReachedTarget );
					}
				}
			}
		}
	}
	else if (UnitGetMoveTargetId(unit) != INVALID_ID)
	{
		drbSuperTracker( unit );
		UNIT* target = UnitGetById(game, UnitGetMoveTargetId(unit));
		if (target)
		{
			float fDistanceToTargetSquared = AppIsHellgate() ? UnitsGetDistanceSquared(unit, target) : UnitsGetDistanceSquaredXY(unit, target);
			float fStopDistance = UnitGetStatFloat(unit, STATS_STOP_DISTANCE);
			if (fStopDistance == 0.0f)
			{
				fStopDistance = UNIT_DISTANCE_CLOSE;
			}
			float fMinDistance = UnitGetCollisionRadius( unit ) + fStopDistance;
			
			// take into account target collision if we must
			if (UnitGetStat(unit, STATS_STOP_DISTANCE_IGNORE_TARGET_COLLISION ) == FALSE)
			{
				fMinDistance += UnitGetCollisionRadius( target );
			}
			
			if (fDistanceToTargetSquared < fMinDistance * fMinDistance)
			{
				bReachedTarget = TRUE;
			}
		}
		else 
		{
			bHasTarget = FALSE;
		}
	} 
	else 
	{
		drbSuperTracker( unit );
		const VECTOR* target = UnitGetMoveTarget(unit);
		if (target)
		{
			VECTOR vDelta;
			VectorSubtract(vDelta, *target, vNewPosition);
			float fDistanceToTargetSquared = AppIsTugboat() ? VectorLengthSquaredXY(vDelta) : VectorLengthSquared(vDelta);
			vDelta = *target - vPositionPrev;
			float fOldDistanceSquared = VectorLengthSquared( vDelta );
			float fStopDistance = UnitGetStatFloat( unit, STATS_STOP_DISTANCE );
			if (fStopDistance == 0.0f)
			{
				fStopDistance = UNIT_DISTANCE_CLOSE;
			}
			float fMinDistance = UnitGetPathingCollisionRadius( unit ) + fStopDistance;
			
			if (fDistanceToTargetSquared < fMinDistance * fMinDistance)
			{
				bReachedTarget = TRUE;
			}
			else if (fDistanceToTargetSquared > fOldDistanceSquared)
			{
				if (UnitTestFlag(unit, UNITFLAG_HASPATH))
				{
					vNewPosition = *target;
				}
				bReachedTarget = TRUE;
			}
		} 
		else
		{
			bHasTarget = FALSE;
		}
	}

	if (bHasTarget && (bCollided || bReachedTarget))
	{
		if (!bOnGround)
		{
			if (UnitIsA( unit, UNITTYPE_MISSILE ) && 
			    UnitGetStat( unit, STATS_BOUNCE_ON_HIT_BACKGROUND ) == FALSE)
			{
				unit->fAcceleration = 0.0f;
				unit->fVelocity = 0.0f;
			}
		}
		else
		{
			if (bReachedTarget || (bCollided && !IS_CLIENT(unit)))
			{
				if (UnitTestFlag(unit, UNITFLAG_HASPATH))
				{
					// old code for NPC interaction?
					if (UnitGetStat(unit, STATS_AI_PATH_INDEX) == UnitGetStat(unit, STATS_AI_PATH_END))
					{
						UnitClearFlag(unit, UNITFLAG_HASPATH);
						UnitStepTargetReached(unit);
						bStoppedMotion = TRUE;
					}
					else
					{
						AI_Register(unit, FALSE, 1);
					}
				}
				else
				{
					if( AppIsTugboat() && unit->m_pPathing )
					{
						BOOL bSuccess = UnitClearBothPathNodes( unit );
						// set current to next
						// set us to be the nearest pathing id
						float BestDistance = INFINITY;
						for( int i = 0; i < unit->m_pPathing->m_nPathNodeCount; i++ )
						{
							VECTOR vPos = AStarGetWorldPosition( game, &unit->m_pPathing->m_pPathNodes[i] );
							float Dist = VectorLength( UnitGetPosition( unit ) - vPos );
							if( Dist < BestDistance )
							{
								BestDistance = Dist;
								unit->m_pPathing->m_nCurrentPath = i;
								unit->m_pPathing->m_tOccupiedLocation.m_nRoomId = unit->m_pPathing->m_pPathNodes[i].nRoomId;
								unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex = unit->m_pPathing->m_pPathNodes[i].nPathNodeIndex;
							}
	
						}
						
						bSuccess = UnitReservePathOccupied( unit );
					}
					drbSuperTracker( unit );
					if(AppIsTugboat())
					{
						if(IS_SERVER(game))
						{
							if(UnitIsA(unit, UNITTYPE_PLAYER))
							{
								if(UnitOnStepList(game, unit))
								{
									s_UnitSetMode( unit, MODE_IDLE );
									UnitStepListRemove( game, unit );
								}
							}
							else
							{
								s_SendUnitPathPositionUpdate( unit );
							}
						}
					}
					// most monsters go here
					if( AppIsTugboat() &&
						IS_CLIENT( game ) && 
						GameGetControlUnit( game ) == unit )
					{
						c_UnitSetMode( unit, MODE_IDLE );
						UnitDoPathTermination( game, unit );
					}
					else
					{


						if( IS_SERVER( game ) )
						{
							// TRAVIS : this is for monsters who have stopped pathing -
							// we need to warp them to their destination, otherwise
							// the clients will be trying to attack out-of-range monsters.
							// Instead of this though - maybe we just need to invest more client trust?
							// up to a point, anyway? It'd certainly pop around less!
							if( AppIsTugboat() )
							{
								if ( bPrematureStop && !UnitIsA(unit, UNITTYPE_PLAYER) && UnitOnStepList( game, unit ) )
								{
									DWORD dwUnitWarpFlags = 0;	
									SETBIT( dwUnitWarpFlags, UW_PATHING_WARP );
									SETBIT( dwUnitWarpFlags, UW_PREMATURE_STOP );
									s_SendUnitWarp( unit, UnitGetRoom( unit ), UnitGetPosition( unit ), UnitGetFaceDirection( unit, FALSE ), UnitGetVUpDirection( unit ), dwUnitWarpFlags  );
								}
							}
						}
						UnitStepTargetReached(unit);
						if( IS_CLIENT( game ) )
						{
							if( AppIsTugboat() )
							{
								if ( UnitOnStepList( game, unit ) )
								{
									c_UnitSetMode( unit, MODE_IDLE );
									UnitStepListRemove( game, unit );
								}

								bStoppedMotion = TRUE;
								bReturnValue = TRUE;
							}
							if( AppIsHellgate() )	
							{
								// sometimes on the client, the unit will recalculate a client-only path to try to
								// get in synch with the server.  In that case, we don't stop motion, we haven't
								// reached our target, and we don't want to remove ourselves from the steplist
								if(!UnitPathIsActiveClient( unit ))
								{
									// sometimes, on the client we can be aborting the path early. so idle us
									if ( UnitOnStepList( game, unit ) )
									{
										c_UnitSetMode( unit, MODE_IDLE );
										UnitStepListRemove( game, unit );
									}
									bStoppedMotion = TRUE;
									bReturnValue = TRUE;
								}
							}
						}
						else
						{
							// On the server, though, there is none of that!  We reach the end, and we STOP.
							bStoppedMotion = TRUE;
							bReturnValue = TRUE;
						}
					}
					//trace( "Target reached (%c) : (%c) = [c]ollided or [t]arget reached\n", HOST_CHARACTER(game), (bCollided ? 'c' : 't') );
				}
			}
		}
	}
	else if (!bHasTarget && bCollided)
	{
		drbSuperTracker( unit );
		if (!bOnGround)
		{
			if (UnitIsA( unit, UNITTYPE_MISSILE ) && 
				UnitGetStat( unit, STATS_BOUNCE_ON_HIT_BACKGROUND ) == FALSE)
			{
				unit->fAcceleration = 0.0f;
				unit->fVelocity = 0.0f;
			}
		}
		//*
		else
		{
			UnitStepTargetReached(unit);
			bReturnValue = TRUE;
		}
		// */
		//trace( "Target reached (%c) : (c) = [c]ollided or [t]arget reached\n", HOST_CHARACTER(game) );
	}
	else if (!bHasTarget && !bOldOnGround && bOnGround)
	{
		drbSuperTracker( unit );
		// I was only on the step list to jump and I have landed
		UnitStepTargetReached(unit);
		bReturnValue = TRUE;
	}

	return bReturnValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sCheckDidCollideEvent(
	GAME * game,
	UNIT * unit,
	const VECTOR & vNewPosition,
	const VECTOR & vPositionPrev,
	VECTOR & vCollisionNormal,
	BOOL bCollided )
{
	if (UnitTestFlag(unit, UNITFLAG_DONT_COLLIDE_NEXT_STEP))
	{
		UnitClearFlag(unit, UNITFLAG_DONT_COLLIDE_NEXT_STEP);
		return;
	}

	if (UnitHasEventHandler(game, unit, EVENT_DIDCOLLIDE))
	{
		UNIT * hit = TestUnitUnitCollision(unit, &vNewPosition, &vPositionPrev, UnitGetCollisionMask(unit));
		if (hit || bCollided)
		{
			if (hit)
			{
				unit->idLastUnitHit = UnitGetId(hit);
				vCollisionNormal = UnitGetPosition(hit) - vNewPosition;
				VectorNormalize(vCollisionNormal);
			}

			UnitTriggerEvent(game, EVENT_DIDCOLLIDE, unit, hit, (void*)&vCollisionNormal);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sCheckOutOfBoundsEvent(
	GAME * game,
	UNIT * unit,
	BOOL bCollided )
{
	if ( bCollided && UnitHasEventHandler( game, unit, EVENT_OUT_OF_BOUNDS ) )
	{
		UnitTriggerEvent( game, EVENT_OUT_OF_BOUNDS, unit, NULL, NULL );
	}
}

#define PATHING_MULTINODE_OCCUPATION 0

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static BOOL sUnitTestPathLocationIsClear(
	UNIT * pUnit,
	ASTAR_OUTPUT * pLocation)
{
	ASSERTX_RETFALSE(pUnit, "Expected Unit");
	GAME * pGame = UnitGetGame( pUnit );
	if ( IS_CLIENT( pGame ) || !drbRESERVE_NODES )
	{
		return TRUE;
	}

	ROOM * pRoom = RoomGetByID( pGame, pLocation->nRoomId );
	ASSERT_RETFALSE( pRoom );
	UNIT_NODE_MAP * pUnitNodeMap = HashGet( pRoom->tUnitNodeMap, pLocation->nPathNodeIndex );

	if( pUnitNodeMap )
	{
		// if it gets here, then it will be blocked. Just doing some extra checking for now
		if ( pUnitNodeMap->bBlocked )
		{
			return FALSE;
		}

#if PATHING_MULTINODE_OCCUPATION
		// We don't collide against ourselves
		if( pUnitNodeMap->pUnit == pUnit )
		{
			return TRUE;
		}
#endif

#ifndef DRB_HAPPY_PATHING
		drbSTASSERT_RETFALSE( pUnitNodeMap->pUnit != pUnit, pUnit );
#endif
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path

static BOOL sUnitReservePathLocation(
	UNIT * pUnit,
	ASTAR_OUTPUT * pLocation)
{
	ASSERTX_RETFALSE(pUnit, "Expected Unit");
	GAME * pGame = UnitGetGame( pUnit );
	if ( ( AppIsHellgate() && IS_CLIENT( pGame ) ) || !drbRESERVE_NODES )
	{
		return TRUE;
	}

	ROOM * pRoom = RoomGetByID( pGame, pLocation->nRoomId );
	// TRAVIS: - since we have player pathing in tugboat, we have switchover cases from level to
	// level, unlike monsters - so this can happen!
	if(AppIsTugboat() && !pRoom)
	{
		return TRUE;
	}
	ASSERT_RETFALSE( pRoom );

	// In Mythos, players don't occupy pathnodes in towns
	if( AppIsTugboat() &&
		UnitIsA( pUnit, UNITTYPE_PLAYER ) &&
		( LevelIsSafe( UnitGetLevel( pUnit ) ) || LevelIsTown( UnitGetLevel( pUnit ) ) ) )
	{
		return TRUE;
	}

	UNIT_NODE_MAP * pUnitNodeMap = HashGet( pRoom->tUnitNodeMap, pLocation->nPathNodeIndex );

	drbSuperTrackerI2( pUnit, pLocation->nRoomId, pLocation->nPathNodeIndex );

	if( !pUnitNodeMap )
	{
		pUnitNodeMap = HashAdd( pRoom->tUnitNodeMap, pLocation->nPathNodeIndex );
		ASSERT_RETFALSE( pUnitNodeMap );
		pUnitNodeMap->pUnit = pUnit;
		pUnitNodeMap->bBlocked = FALSE;
	}
	else
	{
#ifndef DRB_HAPPY_PATHING
		drbSTASSERT_RETFALSE( !pUnitNodeMap->bBlocked, pUnit );
		drbSTDump( GameGetId( pGame ), UnitGetId( pUnitNodeMap->pUnit ) );
		drbSTASSERT_RETFALSE( pUnitNodeMap->pUnit == NULL, pUnit );
#endif
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
static BOOL sUnitClearPathLocation(
	UNIT * pUnit,
	ASTAR_OUTPUT * pLocation)
{
	ASSERTX_RETFALSE(pUnit, "Expected Unit");
	GAME * pGame = UnitGetGame( pUnit );
	if ( ( AppIsHellgate() && IS_CLIENT( pGame ) ) || !drbRESERVE_NODES )
	{
		return TRUE;
	}

	drbSuperTrackerI2( pUnit, pLocation->nRoomId, pLocation->nPathNodeIndex );

	if(pLocation->nRoomId == INVALID_ID || pLocation->nPathNodeIndex == INVALID_ID)
	{
		return TRUE;
	}

	ROOM * pRoom = RoomGetByID( pGame, pLocation->nRoomId );
	// if there isn't a room anymore, that counts as clearing!!
	if( !pRoom )
	{
		return TRUE;
	}
	ASSERT_RETFALSE( pRoom );
	UNIT_NODE_MAP * pUnitNodeMap = HashGet( pRoom->tUnitNodeMap, pLocation->nPathNodeIndex );
#ifdef DRB_HAPPY_PATHING
	if ( !pUnitNodeMap )
	{
		return TRUE;
	}
#else
	ASSERT_RETFALSE( pUnitNodeMap );
#endif

	if(!UnitTestFlag(pUnit, UNITFLAG_BEING_REPLACED))
	{
#ifndef DRB_HAPPY_PATHING
		drbSTASSERT_RETFALSE( !pUnitNodeMap->bBlocked, pUnit );
		drbSTASSERT_RETFALSE( pUnitNodeMap->pUnit == pUnit, pUnit );
#endif
	}

	if(!pUnitNodeMap->bBlocked && pUnitNodeMap->pUnit == pUnit)
	{
		HashRemove( pRoom->tUnitNodeMap, pLocation->nPathNodeIndex );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitClearPathOccupied( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( unit->m_pPathing );
	
	ASTAR_OUTPUT loc;
	loc.nRoomId = unit->m_pPathing->m_tOccupiedLocation.m_nRoomId;
	loc.nPathNodeIndex = unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex;
	BOOL bReturnValue = sUnitClearPathLocation( unit, &loc );

#if PATHING_MULTINODE_OCCUPATION
	// We only do this in Hellgate for now because Tugboat is so close to Alpha.
	if(AppIsHellgate())
	{
		for(int i=0; i<MAX_PATH_OCCUPIED_NODES; i++)
		{
			PATH_STATE_LOCATION * pLocation = &unit->m_pPathing->m_pOccupiedNodes[i];
			// Room ID is a DWORD, so we must compare against INVALID_ID instead of 0
			if(pLocation->m_nRoomId != INVALID_ID && pLocation->m_nRoomPathNodeIndex >= 0 &&
				(pLocation->m_nRoomId != unit->m_pPathing->m_tOccupiedLocation.m_nRoomId || 
				pLocation->m_nRoomPathNodeIndex != unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex))
			{
				loc.nRoomId = pLocation->m_nRoomId;
				loc.nPathNodeIndex = pLocation->m_nRoomPathNodeIndex;
				bReturnValue = bReturnValue && sUnitClearPathLocation( unit, &loc );
			}
		}
	}
#endif
	return bReturnValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitClearPathNextNode( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( unit->m_pPathing );

	PATH_STATE * path = unit->m_pPathing;
	if ( path->m_eState == PATHING_STATE_ACTIVE )
	{
		return sUnitClearPathLocation( unit, &path->m_pPathNodes[path->m_nCurrentPath] );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitClearBothPathNodes( UNIT * unit )
{
	BOOL bSuccess = UnitClearPathOccupied( unit );
	//if( AppIsHellgate() )
	{
		ASSERT_RETFALSE( bSuccess );
	}
	bSuccess = UnitClearPathNextNode( unit );
	//if( AppIsHellgate() )
	{
		ASSERT_RETFALSE( bSuccess );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitReservePathOccupied( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( unit->m_pPathing );
	ASTAR_OUTPUT loc;
	loc.nRoomId = unit->m_pPathing->m_tOccupiedLocation.m_nRoomId;
	loc.nPathNodeIndex = unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex;
	BOOL bReturnValue = sUnitReservePathLocation( unit, &loc );

#if PATHING_MULTINODE_OCCUPATION
	// We only do this in Hellgate for now because Tugboat is so close to Alpha.
	if(AppIsHellgate())
	{
		NEAREST_NODE_SPEC tNearestNodeSpec;
		tNearestNodeSpec.fMaxDistance = UnitGetCollisionRadius(unit);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		tNearestNodeSpec.pUnit = unit;

		FREE_PATH_NODE pFreePathNodes[MAX_PATH_OCCUPIED_NODES];
		int nNodeCount = RoomGetNearestPathNodes(UnitGetGame(unit), UnitGetRoom(unit), UnitGetPosition(unit), MAX_PATH_OCCUPIED_NODES, pFreePathNodes, &tNearestNodeSpec);

		for(int i=0; i<MAX_PATH_OCCUPIED_NODES; i++)
		{
			PATH_STATE_LOCATION * pLocation = &unit->m_pPathing->m_pOccupiedNodes[i];
			FREE_PATH_NODE * pFreePathNode = &pFreePathNodes[i];

			if(i < nNodeCount && pFreePathNode->pRoom && pFreePathNode->pNode)
			{
				pLocation->m_nRoomId = RoomGetId(pFreePathNode->pRoom);
				pLocation->m_nRoomPathNodeIndex = pFreePathNode->pNode->nIndex;

				if(pLocation->m_nRoomId != unit->m_pPathing->m_tOccupiedLocation.m_nRoomId ||
					pLocation->m_nRoomPathNodeIndex != unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex)
				{
					loc.nRoomId = pLocation->m_nRoomId;
					loc.nPathNodeIndex = pLocation->m_nRoomPathNodeIndex;
					bReturnValue = bReturnValue && sUnitReservePathLocation( unit, &loc );
				}
			}
			else
			{
				pLocation->m_nRoomId = INVALID_ID;
				pLocation->m_nRoomPathNodeIndex = INVALID_ID;
			}
		}
	}
#endif
	return bReturnValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitTestPathNextNodeIsClear( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( unit->m_pPathing );

	PATH_STATE * path = unit->m_pPathing;
	if ( path->m_nPathNodeCount && path->m_nCurrentPath < path->m_nPathNodeCount && path->m_nCurrentPath >= 0 )
	{
		return sUnitTestPathLocationIsClear( unit, &path->m_pPathNodes[path->m_nCurrentPath] );
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitReservePathNextNode( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	ASSERT_RETFALSE( unit->m_pPathing );

	// we're just going to try things without this for a while. It's so problematic.
	if( AppIsTugboat() )
	{
		return TRUE;
	}
	PATH_STATE * path = unit->m_pPathing;
	if ( path->m_nPathNodeCount && path->m_nCurrentPath < path->m_nPathNodeCount && path->m_nCurrentPath >= 0 )
	{
		return sUnitReservePathLocation( unit, &path->m_pPathNodes[path->m_nCurrentPath] );
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path - used by train atm, may be used more later
BOOL UnitReservePathNodeArea( UNIT * pUnit, ROOM * pRoom, BOUNDING_BOX * pBox )
{
	ASSERT_RETFALSE( pUnit );
	ASSERT_RETFALSE( pRoom );

	ROOM_PATH_NODE_SET * pPathNodeSet = RoomGetPathNodeSet( pRoom );
	ASSERT_RETFALSE( pPathNodeSet );

	ROOMID idRoom = RoomGetId( pRoom );
	for ( int i = 0; i < pPathNodeSet->nPathNodeCount; i++ )
	{

		// get node delta

		if ( BoundingBoxTestPoint( pBox, &pRoom->pPathNodeInstances[i].vWorldPosition ) )
		{
			// reserve
			ASTAR_OUTPUT tNode;
			tNode.nPathNodeIndex = i;
			tNode.nRoomId = idRoom;
			BOOL bOk = sUnitReservePathLocation( pUnit, &tNode );
			if ( !bOk ) return FALSE;
		}

	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitClearBothSetOccupied( UNIT * unit )
{
	BOOL bSuccess = UnitClearBothPathNodes( unit );
	ASSERT_RETFALSE( bSuccess );
	bSuccess = UnitReservePathOccupied( unit );
	ASSERT_RETFALSE( bSuccess );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
void UnitInitPathNodeOccupied( UNIT * unit, ROOM * room, int nIndex )
{
	ASSERT_RETURN( unit );
	PATH_STATE * path = unit->m_pPathing;
	ASSERT_RETURN( path );
	ASSERT_RETURN( room );
	ASSERT_RETURN( nIndex != INVALID_ID );
	path->m_tOccupiedLocation.m_nRoomId = RoomGetId( room );
	path->m_tOccupiedLocation.m_nRoomPathNodeIndex = nIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
void UnitErasePathNodeOccupied( UNIT * unit )
{
	ASSERT_RETURN( unit );
	PATH_STATE * path = unit->m_pPathing;
	ASSERT_RETURN( path );
	path->m_tOccupiedLocation.m_nRoomId = INVALID_ID;
	path->m_tOccupiedLocation.m_nRoomPathNodeIndex = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
void UnitGetPathNodeOccupied( UNIT * unit, ROOM ** room, int * index )
{
	PATH_STATE * path = unit->m_pPathing;
	ASSERT_RETURN( path );
	if(room)
	{
		*room = RoomGetByID( UnitGetGame( unit ), path->m_tOccupiedLocation.m_nRoomId );
	}
	if(index)
	{
		*index = path->m_tOccupiedLocation.m_nRoomPathNodeIndex;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitPathIsActive( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	if ( !unit->m_pPathing )
		return FALSE;

	return ( unit->m_pPathing->m_eState == PATHING_STATE_ACTIVE || (IS_CLIENT(unit) && unit->m_pPathing->m_eState == PATHING_STATE_ACTIVE_CLIENT) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int UnitGetPathNodeCount( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	if ( !unit->m_pPathing )
		return 0;

	return ( unit->m_pPathing->m_nPathNodeCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
BOOL UnitPathIsActiveClient( UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	if ( !unit->m_pPathing )
		return FALSE;

	return ( unit->m_pPathing->m_eState == PATHING_STATE_ACTIVE_CLIENT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
void UnitPathAbort( UNIT * unit )
{
	ASSERT_RETURN(unit);
	if(!unit->m_pPathing)
		return;

	unit->m_pPathing->m_eState = PATHING_STATE_ABORT;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
void UnitEndPathReserve( UNIT * unit )
{
	ASSERT_RETURN( unit );
	GAME * game = UnitGetGame( unit );
	// tug can path on the client
	if( AppIsHellgate() )
	{
		ASSERT_RETURN( IS_SERVER( game ) );
	}
	ASSERT_RETURN( unit->m_pPathing );

	PATH_STATE * path = unit->m_pPathing;
	ASSERT_RETURN( path->m_eState != PATHING_STATE_INACTIVE );

	drbSuperTrackerI( unit, (int)path->m_eState );
	// clear where I was
	UnitClearBothPathNodes( unit );
	// decide if I am closer to next node or current
	VECTOR vPos = UnitGetPosition( unit );
	VECTOR vFrom = AStarGetWorldPosition( game, (ROOMID)path->m_tOccupiedLocation.m_nRoomId, path->m_tOccupiedLocation.m_nRoomPathNodeIndex );
	VECTOR vTo = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nCurrentPath] );

	if(AppIsHellgate())
	{
		float fFrom = VectorDistanceSquaredXY( vFrom, vPos );
		float fTo = VectorDistanceSquaredXY( vTo, vPos );
		if ( ( fTo < fFrom ) && ( path->m_eState == PATHING_STATE_ACTIVE ) )	// don't go here if we are aborting/removing from step list
		{
			drbSuperTracker( unit );
			path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
			path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
			ASSERT(path->m_tOccupiedLocation.m_nRoomId != INVALID_ID);
			ASSERT(path->m_tOccupiedLocation.m_nRoomPathNodeIndex != INVALID_ID);
			path->m_nCurrentPath++;
			if ( path->m_nCurrentPath >= path->m_nPathNodeCount )
			{
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;
			}
			// check to see if next node is occupied... 
			else if ( !UnitTestPathNextNodeIsClear( unit ) )
			{
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;
			}
		}
	}
	else
	{
		if( unit->m_fPathDistance >= unit->m_pActivePath->Length() - .1f  &&
			path->m_nPathNodeCount > 0 && ( path->m_eState == PATHING_STATE_ACTIVE ) )
		{
			drbSuperTracker( unit );
			path->m_eState = PATHING_STATE_ABORT;
			if( path->m_nCurrentPath >= 0 )
			{
				path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
				path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;	
			}
			UnitReservePathOccupied( unit );
			return;
		}
		if ( unit->m_fPathDistance >= path->m_fPathNodeDistance[ path->m_nCurrentPath ]/*unit->m_pActivePath->GetPointDistance( path->m_nCurrentPath )*/ &&
			path->m_nCurrentPath < path->m_nPathNodeCount - 1 && ( path->m_eState == PATHING_STATE_ACTIVE ) )	// don't go here if we are aborting/removing from step list
		{
			drbSuperTracker( unit );
			if( path->m_nCurrentPath >= 0 )
			{
				path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
				path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
			}
			path->m_nCurrentPath++;

			// check to see if next node is occupied... 
			if ( !UnitTestPathNextNodeIsClear( unit ) )
			{
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;

			}
		}
	}
	UnitReservePathOccupied( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path

#define UNIT_OUT_OF_SYNC_DISTANCE				4.0f
#define UNIT_OUT_OF_SYNC_DISTANCE_SQUARED		( UNIT_OUT_OF_SYNC_DISTANCE * UNIT_OUT_OF_SYNC_DISTANCE )

void c_UnitSetPathServerLoc( UNIT * unit, ROOM * room, unsigned int index )
{
	ASSERT_RETURN( unit );
	ASSERT_RETURN( room );

	PATH_STATE * path = unit->m_pPathing;
	ASSERTV_RETURN( path, "Unit '%s' received a path update from the server, but has no pathing structure on the client.", UnitGetDevName(unit));

	ROOMID room_id = RoomGetId( room );

	// lets see if this is where I am
	if ( ( path->m_tOccupiedLocation.m_nRoomId == room_id ) && ( path->m_tOccupiedLocation.m_nRoomPathNodeIndex == (int)index ) )
	{
		// all good
		return;
	}

	// is this where I am headed?
	// note: this may be a bad check, because we are getting this message from a server when it either starts or ends a path.
	// if it is ending a path, then it is going to be too late for us to arrive at the node...
	if ( path->m_eState == PATHING_STATE_ACTIVE )
	{
#if HELLGATE_ONLY
		if ( path->cFastPathIndex != INVALID_ID )
		{
			if ( ( path->cFastPathNodes[path->cFastPathIndex].nRoomId == room_id ) && ( path->cFastPathNodes[path->cFastPathIndex].nPathNodeIndex == index ) )
			{
				// all good
				return;
			}
		}
		else
#endif
		{
			if ( ( path->m_pPathNodes[path->m_nCurrentPath].nRoomId == room_id ) && ( path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex == index ) )
			{
				// =)
				return;
			}
		}
	}
	VECTOR serverpos;
	// Let's get the server position, so that we know what the last known good position was
	if( AppIsHellgate() )
	{
		ROOM_PATH_NODE * node = RoomGetRoomPathNode( room, index );
		ASSERTX_RETURN( node, room->pDefinition->tHeader.pszName );

		if (TESTBIT(unit->pdwMovementFlags, MOVEFLAG_FLY))
			serverpos = UnitCalculatePathTargetFly(UnitGetGame(unit), unit, room, node);
		else
			serverpos = RoomPathNodeGetExactWorldPosition(UnitGetGame(unit), room, node);
		c_UnitSetLastKnownGoodPosition(unit, serverpos);
	}

	if( AppIsTugboat() )
	{
		ROOM_PATH_NODE_SET * pRoomPathNodeSet = RoomGetPathNodeSet( room );
		ASSERT_RETURN( pRoomPathNodeSet );
		ROOM_PATH_NODE * node = RoomGetRoomPathNode( room, index );
		ASSERT_RETURN( node );
		serverpos = RoomPathNodeGetExactWorldPosition(UnitGetGame(unit), room, node);
	
		// switch nodes!
		// we HAVE to do this, because we need synched client nodes for client-side pathing
		BOOL bSuccess = UnitClearBothPathNodes( unit );
		ASSERT_RETURN( bSuccess );
		// set current to next
		path->m_tOccupiedLocation.m_nRoomId = room_id;
		path->m_tOccupiedLocation.m_nRoomPathNodeIndex = index;
		bSuccess = UnitReservePathOccupied( unit );
	}

	// okay now what... lets do the warp if we are far away
	// I got the position where the server is...
	float delta = AppIsTugboat() ? VectorDistanceSquaredXY( serverpos, unit->vPosition ) : VectorDistanceSquared( serverpos, unit->vPosition );
	if ( delta >= UNIT_OUT_OF_SYNC_DISTANCE_SQUARED )
	{
		if( AppIsTugboat() )
		{
			/*UnitSetPosition(unit, serverpos);
			UnitSetDrawnPosition(unit, serverpos);

			// snap 'em to the ground!
			UnitForceToGround(unit);

			// update the room
			UnitUpdateRoom(unit, room, TRUE);*/
		}
		else
		{
			// oh crap! we are too far!
			VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			UnitWarp( unit, room, serverpos, UnitGetFaceDirection( unit, FALSE ), vUp, 0, FALSE );
		}
	}
	// things okayish.. let's continue
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path

static BOOL sSwitchNodes( GAME * game, UNIT * unit, ASTAR_OUTPUT * node, VECTOR & vPositionPrev, VECTOR & vNewPosition )
{
	ASSERT( game && unit && node );
	VECTOR vNodePosition;
	if ( TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) )
		vNodePosition = UnitCalculatePathTargetFly( game, unit, node ); // may be offset from XY position of node because of non vertical normal
	else
		vNodePosition = AStarGetWorldPosition( game, node );
	//float fFrom = VectorDistanceSquaredXY( vPositionPrev, vNodePosition );
	//ASSERT( fFrom < DRB_DIST );
	float fTo = VectorDistanceSquaredXY( vNewPosition, vNodePosition );
	//ASSERT( fTo < DRB_DIST );
	VECTOR vFrom, vTo;
	vFrom.x = vPositionPrev.x - vNodePosition.x;
	vFrom.y = vPositionPrev.y - vNodePosition.y;
	vFrom.z = 0.0f;
	vTo.x = vNewPosition.x - vNodePosition.x;
	vTo.y = vNewPosition.y - vNodePosition.y;
	vTo.z = 0.0f;
	// did I pass the node yet?
	return ( ( VectorDot( vFrom, vTo ) <= 0.0f ) || ( fTo < 0.01f ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
// client pathing
void c_UpdatePathing( GAME * game, UNIT * unit, VECTOR & vPositionPrev, VECTOR & vNewPosition, COLLIDE_RESULT * result )
{
	PATH_STATE * path = unit->m_pPathing;
	if ( path->m_eState != PATHING_STATE_ACTIVE && path->m_eState != PATHING_STATE_ACTIVE_CLIENT )
	{
		return;
	}

	if ( UnitTestMode( unit, MODE_IDLE ) && UnitGetMoveMode(unit) == INVALID_ID )
	{
		// oh dear...
		c_UnitSetMode( unit, MODE_WALK );
	}

	// did I hit another unit?
	if ( AppIsHellgate() && result->unit )
	{
		// okay.. now check if that unit is moving...
		UNIT * monster = result->unit;
		if ( ( UnitGetGenus( monster ) == GENUS_MONSTER ) && monster->m_pPathing )
		{
			if ( monster->m_pPathing->m_eState != PATHING_STATE_ACTIVE && monster->m_pPathing->m_eState != PATHING_STATE_ACTIVE_CLIENT )
			{
				// okay.. i hit another monster.. it is stopped. how far am I from my goal?
				VECTOR vNodePosition;
#if HELLGATE_ONLY
				if ( path->cFastPathIndex != INVALID_ID )
					vNodePosition = AStarGetWorldPosition( game, &path->cFastPathNodes[path->cFastPathSize-1] );
				else
#endif
					vNodePosition = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nPathNodeCount-1] );
				float goal_dist = VectorDistanceSquaredXY( vNewPosition, vNodePosition );
				float col_dist = UnitGetCollisionRadius( unit ) + ( 2.0f * UnitGetCollisionRadius( monster ) ) + 0.5f;
				if ( ( col_dist * col_dist ) > goal_dist )
				{
					path->m_eState = PATHING_STATE_ABORT;
					return;
				}
			}
		}
	}

#if HELLGATE_ONLY
	if ( path->cFastPathIndex != INVALID_ID )
	{
		if ( sSwitchNodes( game, unit, &path->cFastPathNodes[path->cFastPathIndex], vPositionPrev, vNewPosition ) )
		{
			// set current to next
			path->m_tOccupiedLocation.m_nRoomId = path->cFastPathNodes[path->cFastPathIndex].nRoomId;
			path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->cFastPathNodes[path->cFastPathIndex].nPathNodeIndex;
			// next in path
			path->cFastPathIndex++;
			if ( path->cFastPathIndex >= path->cFastPathSize )
			{
				path->m_eState = PATHING_STATE_ABORT;
			}
		}
	}
	else
#endif
	{
		if(AppIsHellgate())
		{
			if ( sSwitchNodes( game, unit, &path->m_pPathNodes[path->m_nCurrentPath], vPositionPrev, vNewPosition ) )
			{
				// set current to next
				path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
				path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
				// next in path
				path->m_nCurrentPath++;
#ifdef DRB_SPLINE_PATHING
				path->m_Time = AppCommonGetCurTime();
#endif
				if ( path->m_nCurrentPath >= path->m_nPathNodeCount )
				{
					path->m_eState = PATHING_STATE_ABORT;
				}
			}
		}
		else
		{
			if( unit->m_fPathDistance >= unit->m_pActivePath->Length() - .1f  &&
				path->m_nPathNodeCount > 0 )
			{
				BOOL bSuccess = UnitClearBothPathNodes( unit );
				if( path->m_nCurrentPath >= 0 )
				{
					path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
					path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
				}
				bSuccess = UnitReservePathOccupied( unit );
				path->m_eState = PATHING_STATE_ABORT;

				return;
			}
			if( unit->m_fPathDistance >= path->m_fPathNodeDistance[ path->m_nCurrentPath ]/*unit->m_pActivePath->GetPointDistance( path->m_nCurrentPath )*/ &&
				path->m_nCurrentPath < path->m_nPathNodeCount - 1 )
			{
				// switch nodes!
				BOOL bSuccess = UnitClearBothPathNodes( unit );
				ASSERT_RETURN( bSuccess );

				// set current to next
				if( path->m_nCurrentPath >= 0 )
				{
					path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
					path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
				}
				bSuccess = UnitReservePathOccupied( unit );

				// next in path
				path->m_nCurrentPath++;

				// set next!
				if ( UnitTestPathNextNodeIsClear( unit ) )
				{
					bSuccess = UnitReservePathNextNode( unit );
					ASSERT_RETURN( bSuccess );
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
// server pathing
void s_UpdatePathing( GAME * game, UNIT * unit, VECTOR & vPositionPrev, VECTOR & vNewPosition, BOOL bIsPlayer )
{
	PERF(STEPPING_PATH);
	PATH_STATE * path = unit->m_pPathing;
	if ( path->m_eState == PATHING_STATE_ACTIVE )
	{
		if(AppIsTugboat())
		{
			if( unit->m_fPathDistance >= unit->m_pActivePath->Length() - .1f  &&
				path->m_nPathNodeCount > 0 )
			{
				BOOL bSuccess = UnitClearBothPathNodes( unit );
				if( unit->m_pPathing->m_nCurrentPath >= 0 )
				{
					unit->m_pPathing->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[unit->m_pPathing->m_nCurrentPath].nRoomId;
					unit->m_pPathing->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[unit->m_pPathing->m_nCurrentPath].nPathNodeIndex;
				}
				bSuccess = UnitReservePathOccupied( unit );
				// all done
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;

				return;
			}
		}

		if ( ( AppIsHellgate() && sSwitchNodes( game, unit, &path->m_pPathNodes[path->m_nCurrentPath], vPositionPrev, vNewPosition ) ) ||
			 ( AppIsTugboat() && unit->m_fPathDistance >= path->m_fPathNodeDistance[ path->m_nCurrentPath ]/*unit->m_pActivePath->GetPointDistance( path->m_nCurrentPath )*/ &&
			 path->m_nCurrentPath < path->m_nPathNodeCount - 1 ))
		{
			drbSuperTracker( unit );
			BOOL bSuccess;
			// switch nodes!
			bSuccess = UnitClearBothPathNodes( unit );
			ASSERT_RETURN( bSuccess );
			// set current to next
			if(path->m_nCurrentPath >= 0)
			{
				path->m_tOccupiedLocation.m_nRoomId = path->m_pPathNodes[path->m_nCurrentPath].nRoomId;
				path->m_tOccupiedLocation.m_nRoomPathNodeIndex = path->m_pPathNodes[path->m_nCurrentPath].nPathNodeIndex;
				ASSERT(path->m_tOccupiedLocation.m_nRoomId != INVALID_ID);
				ASSERT(path->m_tOccupiedLocation.m_nRoomPathNodeIndex != INVALID_ID);
			}
			bSuccess = UnitReservePathOccupied( unit );
			ASSERT_RETURN( bSuccess );
			// next in path
			path->m_nCurrentPath++;
			if ( AppIsHellgate() && 
				 path->m_nCurrentPath >= path->m_nPathNodeCount )
			{
				// all done
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;
				return;
			}

			// check if occupied
			if ( !UnitTestPathNextNodeIsClear( unit ) && !bIsPlayer )
			{
				// abort!
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ABORT;
				return;
			}

			// set next!
			bSuccess = UnitReservePathNextNode( unit );
			if( AppIsHellgate() )
			{
#ifdef DRB_SPLINE_PATHING
				path->m_Time = AppCommonGetCurTime();
#endif
				ASSERT_RETURN( bSuccess );
			}
			else if( AppIsTugboat() )
			{
				if( !bSuccess )
				{
					return;
				}
			}

			//*
			// once I get to a new node in the list... check to see if the target I am running towards (if id)
			// is even close to where I am running...
			if ( IS_SERVER( UnitGetGame( unit ) ) && unit->idMoveTarget != INVALID_ID && !bIsPlayer )
			{
				UNIT * pTarget = UnitGetById( game, unit->idMoveTarget );
				// On the server, we should ALWAYS know about the target unit (this may not be true on the client, though)
				ASSERT_RETURN( pTarget );
				VECTOR vEndNodePosition = AStarGetWorldPosition(game, &path->m_pPathNodes[path->m_nPathNodeCount-1]);
				float fEndPathDistance = VectorDistanceSquaredXY( pTarget->vPosition, vEndNodePosition );
				float fStopDistance = UnitGetStatFloat(unit, STATS_STOP_DISTANCE);
				if (fStopDistance == 0.0f)
				{
					fStopDistance = UNIT_DISTANCE_CLOSE;
				}
				float fMinDistance = UnitGetCollisionRadius( unit ) + fStopDistance;
				float fDynamicRepathFudge = //The closer we are to a unit (and thus the easier it is to path to him) the more we're willing to repath.
					REPATH_FUDGE_FACTOR*VectorDistanceSquaredXY(pTarget->vPosition, unit->vPosition);
				if ( fEndPathDistance >= ( fMinDistance * fMinDistance + fDynamicRepathFudge ) )
				{
					drbSuperTracker( unit );
					// re-path?!
					BOOL newpath = UnitCalculatePath( game, unit, PATH_FULL, 0 );
					if ( newpath )
					{
						drbSuperTracker( unit );
						s_SendUnitPathChange( unit );
						return;
					}
				}
			}
			// */
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path
#define MONSTER_COLLIDE_DIST			15.0f

static BOOL sMonsterToMonster( GAME * game, UNIT * monster )
{
	if ( IS_SERVER( game ) )
		return FALSE;

	if ( UnitGetGenus( monster ) != GENUS_MONSTER )
		return FALSE;

	// dist from player...
	UNIT * player = GameGetControlUnit( game );
	float distance = VectorDistanceSquaredXY( player->vPosition, monster->vPosition );
	if ( distance > ( MONSTER_COLLIDE_DIST * MONSTER_COLLIDE_DIST ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// drb.path

#define MAX_Z_OKAY_BETWEEN_NODES			0.5f

static void sQuickCollide( GAME * game, UNIT * unit, VECTOR & vNewPosition, DWORD & dwMoveFlags )
{
	ASSERT_RETURN( game && unit );
	ASSERT_RETURN( unit->m_pPathing );

	PATH_STATE * path = unit->m_pPathing;
	if ( ( path->m_eState == PATHING_STATE_ACTIVE || path->m_eState == PATHING_STATE_ACTIVE_CLIENT ) && ( UnitGetGenus( unit ) == GENUS_MONSTER ) )
	{
		VECTOR vCurNode = AStarGetWorldPosition( game, path->m_tOccupiedLocation.m_nRoomId, path->m_tOccupiedLocation.m_nRoomPathNodeIndex );
		VECTOR vNextNode;
#if HELLGATE_ONLY
		if ( IS_CLIENT( game ) && ( path->cFastPathIndex != INVALID_ID ) )
			return;		// too many issues...
			//vNextNode = AStarGetWorldPosition( game, &path->cFastPathNodes[path->cFastPathIndex] );
		else
#endif
			vNextNode = AStarGetWorldPosition( game, &path->m_pPathNodes[path->m_nCurrentPath] );
		// calculate the target position for the unit, on, or above the node (in the case of flying)
		VECTOR vCurNodeTarget, vNextNodeTarget;
		if (TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ))
		{
			if ( path->m_tOccupiedLocation.m_nRoomId == INVALID_ID )
				return;

			vCurNodeTarget = UnitCalculatePathTargetFly( game, unit, path->m_tOccupiedLocation.m_nRoomId, path->m_tOccupiedLocation.m_nRoomPathNodeIndex );
			vNextNodeTarget = UnitCalculatePathTargetFly( game, unit, &path->m_pPathNodes[path->m_nCurrentPath] );
		}
		else 
		{
			vCurNodeTarget = vCurNode;
			vNextNodeTarget = vNextNode;
		}
		if ( fabs( vNextNode.z - vCurNode.z ) < MAX_Z_OKAY_BETWEEN_NODES )
		{
			// dist between nodes
			float fDeltaNode = VectorDistanceSquared( vCurNode, vNextNode );
			if ( fDeltaNode > 0.0f )
			{
				float fDeltaCur = VectorDistanceSquared( vNewPosition, vCurNode );
				float percent = fDeltaCur / fDeltaNode;
				if ( percent > 1.0f )
					percent = 1.0f;
				vNewPosition.z = vCurNodeTarget.z + ( ( vNextNodeTarget.z - vCurNodeTarget.z ) * percent );
			}
			else
			{
				vNewPosition.z = vCurNodeTarget.z;
			}
			// set and clear
			dwMoveFlags &= ~MOVEMASK_FLOOR_ONLY;
			dwMoveFlags |= MOVEMASK_NO_GEOMETRY;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitPositionTrace(
	UNIT *pUnit,
	const char *pszLabel)
{
#ifdef _DEBUG
	GAME *pGame = UnitGetGame( pUnit );
	VECTOR vPosition = UnitGetPosition( pUnit);
	
	const int MAX_MESSAGE = 1024;
	char szMessage[ MAX_MESSAGE ];
	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"%s: [%d] - '%s'(%d) - (%.02f,%.02f,%.02f) - %s\n",
		IS_SERVER( pGame ) ? "Server" : "Client",
		GameGetTick( pGame ),
		UnitGetDevName( pUnit ),
		UnitGetId( pUnit ),
		vPosition.fX, 
		vPosition.fY,
		vPosition.fZ,
		pszLabel);
	trace( szMessage );
#endif	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL REAL_STEP>
static void sUnitDoStep(
	GAME * game,
	UNIT * unit,
	VECTOR & vNewPosition,
	float fTimeDelta = GAME_TICK_TIME)
{
	static const VECTOR vZero(0);

	if (!unit)
	{
		vNewPosition = vZero;
		ASSERT_RETURN(unit);
	}

	if (UnitTestFlag(unit, UNITFLAG_FREED))
	{
		vNewPosition = vZero;
		return;
	}

	vNewPosition = UnitGetPosition(unit);

	ROOM * room = UnitGetRoom(unit);
	if (room == NULL)
	{
		UnitStepListRemove(game, unit, SLRF_FORCE);
		ASSERT_RETURN(room);
	}

	if (RoomIsActive(UnitGetRoom(unit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(unit, "Attempt to step Inactive Room"))
		{
			return;
		}
	}

	LEVEL * level = RoomGetLevel(room);
	if (level == NULL || !LevelIsActive(level))
	{
		UnitStepListRemove(game, unit, SLRF_FORCE);
		ASSERT_RETURN(0);
	}

	if ( ( UnitHasState( game, unit, STATE_STOP_MOVING ) || ( AppIsTugboat() && SkillIsOnWithFlag(unit, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT) ) )&& 
		!UnitTestMode( unit, MODE_KNOCKBACK ) ) // I don't know what to do here.   We should have a cleaner way of checking this. Tyler
	{
		UnitStepTargetReached( unit );
		UnitStepListRemove(game, unit);
		return;
	}

	if ( UnitGetGenus( unit ) == GENUS_PLAYER ) 
	{// players don't really belong here
		if( AppIsTugboat() )	// tugboat ALWAYS has to do this! EVEN on the server!
		{
			vNewPosition = c_PlayerDoStep( game, unit, fTimeDelta, REAL_STEP );
		}
		else if( AppIsHellgate() )
		{
#if !ISVERSION(SERVER_VERSION)
			if ( UnitTestFlag( unit, UNITFLAG_CLIENT_PLAYER_MOVING ) )
			{ // although in multiplayer they should call this
				vNewPosition = c_PlayerDoStep( game, unit, fTimeDelta, REAL_STEP );
			}
#endif 
		}
		return;
	}

	if ( UnitIsA( unit, UNITTYPE_TRAIN ) )
	{
		vNewPosition = ObjectTrainMove( unit, fTimeDelta, REAL_STEP );
		if ( REAL_STEP )
		{
			VECTOR oldpos = unit->vPosition;
			unit->vPosition = vNewPosition;
			UnitUpdateRoomFromPosition(game, unit, &oldpos);
			if ( IS_SERVER( game ) )
			{
				// did I kill someone?
				s_ObjectTrainCheckKill( unit );
			}
		}
		return;
	}
	
	// if we have a move target, and they are no longer on our level, clear it
	UNITID idMoveTarget = UnitGetMoveTargetId( unit );
	if (idMoveTarget != INVALID_ID)
	{
		UNIT *pMoveTarget = UnitGetById( game, idMoveTarget );
		if ( !pMoveTarget || UnitGetLevel( unit ) != UnitGetLevel( pMoveTarget ))
		{
			UnitSetMoveTarget( unit, INVALID_ID );
		}
	}
	
	if (REAL_STEP && UnitTestFlag(unit, UNITFLAG_SKIPFIRSTSTEP))
	{
		UnitClearFlag(unit, UNITFLAG_SKIPFIRSTSTEP);
		return;
	}
		
	// handle homing missiles
	if (REAL_STEP &&
		UnitIsA( unit, UNITTYPE_MISSILE ) && 
		MissileIsHoming( unit ) && 
		UnitGetMoveTargetId( unit ) == INVALID_ID )
	{
		sUpdateHoming( unit, NULL);
	}



#ifdef HAVOK_ENABLED
	if (unit->pHavokRigidBody)
	{
		const VECTOR vPositionPrev = UnitGetPosition(unit);
		HavokRigidBodyGetPosition(unit->pHavokRigidBody, &vNewPosition);
		UnitSetPosition(unit, vNewPosition);  // we should not set the position in this function, but maybe cause it's havok it's ok? -Colin
		UnitUpdateRoomFromPosition(game, unit, &vPositionPrev);
		VECTOR vUpVec(0, 0, 1);
		sCheckDidCollideEvent( game, unit, unit->vPosition, vPositionPrev, vUpVec, FALSE );
		//UnitClearFlag( unit, UNITFLAG_DONT_COLLIDE_NEXT_STEP );
		return;
	}
#endif

	if ( unit->fVelocity == 0.0f && UnitGetZVelocity(unit) == 0.0f )
	{
		const UNIT_DATA *pUnitData = UnitGetData( unit );
		ASSERT_RETURN(pUnitData);
		if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_ALWAYS_CHECK_FOR_COLLISIONS ) )
		{ // some missiles just sit there and hope that something bumps into them... they check for the bumping here
			VECTOR vCollisionNormal(0.0f, 1.0f, 0.0f);
			sCheckDidCollideEvent( game, unit, vNewPosition, vNewPosition, vCollisionNormal, FALSE );
		} else {
			UnitStepTargetReached( unit );
		}
		return;
	}

	if (REAL_STEP && unit->fAcceleration)
	{
		unit->fVelocityFromAcceleration += fTimeDelta * unit->fAcceleration;
		UnitCalcVelocity(unit);
	}

	float fDist = unit->fVelocity * fTimeDelta;
	float fLength = UnitGetStatFloat(unit, STATS_MISSILE_DISTANCE);
	if (fLength > 0.0f && fLength <= fDist)
	{
		fDist = fLength;
	}
	VECTOR vPositionPrev = unit->vPosition;
	VECTOR vVelocity;

#ifdef DRB_SPLINE_PATHING
	if ( AppIsHellgate() &&
		 unit->m_pPathing && 
		 unit->m_pPathing->m_eState == PATHING_STATE_ACTIVE &&
		 !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING) &&
		 !UnitIsJumping(unit) &&
		 !TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) )
	{
		UnitCalculateSplinePosition(unit, fTimeDelta, vNewPosition);
		vVelocity = vNewPosition - vPositionPrev;
	}
	else
#endif
	{
		vVelocity = UnitCalculateMoveDirection(game, unit, REAL_STEP);
		VectorScale(vVelocity, vVelocity, fDist);
		VectorAdd(vNewPosition, vPositionPrev, vVelocity);
	}

#ifdef _DEBUG
	if (AppIsHellgate() &&
		IS_SERVER( game ) && 
		unit->m_pPathing && 
		unit->m_pPathing->m_eState != PATHING_STATE_INACTIVE && 
		!UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
	{
		// TODO cmarch: flying
		PATH_STATE * path = unit->m_pPathing;
		VECTOR vNodePos = AStarGetWorldPosition( game, (ROOMID)path->m_tOccupiedLocation.m_nRoomId, path->m_tOccupiedLocation.m_nRoomPathNodeIndex );
		VECTOR vDelta = vNewPosition - vNodePos;
		float fDelta = VectorLengthSquaredXY( vDelta );
		if (fDelta > DRB_DIST)
		{
			TraceGameInfo("UNIT [%d: %s] has node delta %.2f.  Too far away!", UnitGetId(unit), UnitGetDevName(unit), fDelta);
		}
	}
#endif

	if (!UnitIsOnGround( unit ) && 
		!( unit->m_pPathing &&												// no gravity when pathing + flying
		   !UnitTestFlag( unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING ) &&
		   TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) ) )
	{
		vNewPosition.fZ += UnitGetZVelocity(unit) * fTimeDelta;
		if (REAL_STEP)
		{
			UnitChangeZVelocity( unit, -GRAVITY_ACCELERATION * fTimeDelta);
		}
	} 

	BOOL bOldOnGround = UnitIsOnGround( unit );

	/*
	if ( REAL_STEP )
		trace(
		"DoStep (%c): UNIT(%d), DIST(%.2f) - NEW(%.2f, %.2f, %.2f) OLD(%.2f, %.2f, %.2f) MOVE(%.2f, %.2f, %.2f)\n",
		HOST_CHARACTER(game),
		UnitGetId(unit),
		fDist,
		vNewPosition.fX, vNewPosition.fY, vNewPosition.fZ,
		vPositionPrev.fX, vPositionPrev.fY, vPositionPrev.fZ,
		vVelocity.fX, vVelocity.fY, vVelocity.fZ);
	// */
				
	if( UnitTestFlag( unit, UNITFLAG_IGNORE_COLLISION ) )
	{
		// set the new position
		UnitSetPosition(unit, vNewPosition);
		return;
	}

	COLLIDE_RESULT move_result;
	move_result.flags = 0;
	move_result.unit = NULL;
	float fCollideDistance = 0.0f;
	REF(fCollideDistance);
	VECTOR vCollisionNormal(0.0f, 1.0f, 0.0f);
	if (UnitTestFlag(unit, UNITFLAG_RAYTESTCOLLISION))
	{
		LEVEL* pLevel = UnitGetLevel( unit );
		VECTOR vCollideDirection = vVelocity;
		if(unit->fZVelocity != 0.0f)
		{
			vCollideDirection = vNewPosition - vPositionPrev;
		}
		VectorNormalize(vCollideDirection);
		float fCollideDistance = 0;
		int Material = -1;
		if( AppIsHellgate() )
		{
			fCollideDistance = LevelLineCollideLen(game, pLevel, vPositionPrev, vCollideDirection, fDist + fDist / 2.0f, &vCollisionNormal);
		}
		else
		{
			fCollideDistance = LevelLineCollideLen( game, pLevel, vPositionPrev, vCollideDirection, fDist + fDist / 2.0f, Material, &vCollisionNormal);

		}
		if (fCollideDistance < fDist || ( IS_CLIENT( game ) && UnitDataTestFlag( UnitGetData( unit ), UNIT_DATA_FLAG_FOLLOW_GROUND ) ) )
		{
			if( AppIsTugboat()  && vCollideDirection.fX == 0 && vCollideDirection.fY == 0 )
			{
				unit->fVelocity = 0;
				unit->fZVelocity = 0;
			}
			if( UnitDataTestFlag( UnitGetData( unit ), UNIT_DATA_FLAG_FOLLOW_GROUND ) )
			{
				float fThreshhold = pLevel->m_pLevelDefinition->bContoured ? .3f : .8f;
				if( fCollideDistance >= fDist ||
					( vCollisionNormal.fZ > fThreshhold && Material != PROP_MATERIAL ) )
				{

					VECTOR vStart = vNewPosition + VECTOR( 0, 0, 50 );
					float fDownDist = 100;
					VECTOR vCollisionNormal;
					float fCollideDistance = LevelLineCollideLen( game, UnitGetLevel( unit ), vStart, VECTOR( 0, 0, -1 ), fDownDist, &vCollisionNormal, 0, (int)PROP_MATERIAL);
					if (fCollideDistance < fDownDist )
					{
						vNewPosition = vStart;
						vNewPosition.fZ -= fCollideDistance;
					}

				}
				else
				{
					SETBIT( &move_result.flags, MOVERESULT_COLLIDE ); 
					// back up to where we collided
					VECTOR vDelta = UnitGetMoveDirection(unit);
					vDelta *= fCollideDistance - 0.001f; // don't rest right on top of collision - in case you want to move another direction
					vNewPosition = vPositionPrev + vDelta;
				}
			}
			else
			{
				SETBIT( &move_result.flags, MOVERESULT_COLLIDE ); 
				// back up to where we collided
				VECTOR vDelta = UnitGetMoveDirection(unit);
				vDelta *= fCollideDistance - 0.001f; // don't rest right on top of collision - in case you want to move another direction
				vNewPosition = vPositionPrev + vDelta;
			}
		}
	}

	BOOL bCheckRoom = FALSE;
	DWORD dwMoveFlags = sUnitGetMoveFlags( unit );

	if ( UnitTestFlag(unit, UNITFLAG_PATHING) && unit->m_pPathing )
	{
		bCheckRoom = TRUE;
		if( AppIsHellgate() )
		{
			dwMoveFlags |= MOVEMASK_TEST_ONLY;
			if(!UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
			{
				dwMoveFlags |= MOVEMASK_FLOOR_ONLY;
			}

		}
		else if ( AppIsTugboat() )
		{
			dwMoveFlags |= MOVEMASK_NOT_WALLS | MOVEMASK_NO_GEOMETRY | MOVEMASK_SOLID_MONSTERS;
			if( IS_SERVER( game ) )
			{
				bCheckRoom = FALSE;
			}
		}
		if ( IS_SERVER( unit ) && sMonsterToMonster( game, unit ) )
			dwMoveFlags |= MOVEMASK_SOLID_MONSTERS;

	}

	if ( UnitTestFlag(unit, UNITFLAG_ROOMCOLLISION) )
	{
		// we're only testing the position and keeping it inside a room, 
		// we're not actually changing the position on the unit
		dwMoveFlags |= MOVEMASK_TEST_ONLY;
		bCheckRoom = TRUE;
		if( AppIsTugboat() )
		{
			dwMoveFlags |= MOVEMASK_NOT_WALLS | MOVEMASK_NO_GEOMETRY;
		}
	}

	if(!REAL_STEP)
	{
		dwMoveFlags |= MOVEMASK_NOT_REAL_STEP;
	}

	if ( bCheckRoom )
	{
		// just interpolate the Z if it close-by
		BOOL bDoRoomCollide = TRUE;
		if( AppIsTugboat() && IS_SERVER( game ) )
		{
			bDoRoomCollide = FALSE;
		}
		if(AppIsHellgate())
		{
			if ( dwMoveFlags & MOVEMASK_FLOOR_ONLY )
			{
				sQuickCollide( game, unit, vNewPosition, dwMoveFlags );
				bDoRoomCollide = (dwMoveFlags & MOVEMASK_FLOOR_ONLY) != 0;
			}
		}

		// only do this if we are going to collide with the floor or monsters...
		if ( bDoRoomCollide || ( dwMoveFlags & MOVEMASK_SOLID_MONSTERS ) )
		{

			RoomCollide(unit, vPositionPrev, vVelocity, &vNewPosition, UnitGetPathingCollisionRadius( unit ), UnitGetCollisionHeight( unit ), dwMoveFlags, &move_result);

			if(AppIsTugboat() )
			{
				if( TESTBIT(&move_result.flags, MOVERESULT_COLLIDE_UNIT) )
				{

					vNewPosition = vPositionPrev + vVelocity;

				}
			}
		}
/*
		trace("DoStep (%c%d : %I64u, %4.4f) [unit" ID_PRINTFIELD "]: Vel(%8.2f) - Move(%8.2f, %8.2f, %8.2f) Old(%8.2f, %8.2f, %8.2f) New(%8.2f, %8.2f, %8.2f)  DistSq(%8.2f/%8.2f)\n",
				(IS_SERVER(game) ? 's' : (REAL_STEP ? 'c' : 'i')),
				GameGetTick(game), AppGetCurTime(), AppGetElapsedTickTime(),
				ID_PRINT(UnitGetId(unit)),
				fDist,
				vNewPosition.fX - vPositionPrev.fX, vNewPosition.fY - vPositionPrev.fY, vNewPosition.fZ - vPositionPrev.fZ,
				vPositionPrev.fX, vPositionPrev.fY, vPositionPrev.fZ,
				vNewPosition.fX, vNewPosition.fY, vNewPosition.fZ,
				real_len_sq,
				ideal_len_sq);
*/
	}
	if ( !REAL_STEP )
	{		
		return;
	}

	// set the new position
	UnitSetPosition(unit, vNewPosition);

	if ( bCheckRoom )
	{
		// did this unit leave the hulls?
		ROOM * room = UnitUpdateRoomFromPosition( UnitGetGame(unit), unit, &vPositionPrev, false );
		if ( !room )
		{
			vNewPosition = vPositionPrev;
			SETBIT( &move_result.flags, MOVERESULT_COLLIDE );
		}
		
		if(AppIsTugboat())
		{
			float fVelocityCurr = UnitGetVelocity( unit );
			if( fVelocityCurr != 0 )
			{
				LEVEL* pLevel = UnitGetLevel( unit );

				// shoot a vertical ray to find the ground height
				if( pLevel->m_pLevelDefinition->bContoured ||
					IS_CLIENT( game )  )
				{
					if( IS_SERVER( game ) ||
						( unit->bVisible ||
						  ( !c_UnitGetNoDrawTemporary( unit ) && !c_UnitGetNoDraw( unit ) ) ) )
					{
						VECTOR Normal( 0, 0, -1 );
						VECTOR Start = vNewPosition;
						Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
						float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
						VECTOR vCollisionNormal;
						int Material = 0;
						float fCollideDistance = 0;
#if HELLGATE_ONLY
#else
						if( unit->m_pPathing )
						{
							PATH_STATE * path = unit->m_pPathing;
							VECTOR vStart = vNewPosition + VECTOR( -2.0f, -2.0f, 0 );
							VECTOR vEnd = vNewPosition + VECTOR( 2.0f, 2.0f, 0 );
							vEnd -= vStart;
							float fLen = VectorLength( vEnd );
							vEnd /= fLen;
							if( path->m_LastSortPosition.fX == -1.0f ||
								VectorDistanceXY( path->m_LastSortPosition, vNewPosition ) >= fLen * .5f )
							{
								ArrayClear(path->m_SortedFaces);
								LevelSortFaces( game, pLevel, path->m_SortedFaces, vStart, vEnd, fLen );
								path->m_LastSortPosition = vNewPosition;
							}
							fCollideDistance = LevelLineCollideLen( game, pLevel, path->m_SortedFaces, Start, Normal, fDist, Material, &vCollisionNormal);
						}
						else
#endif
						{
							fCollideDistance = LevelLineCollideLen( game, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
						}
						if (fCollideDistance < fDist &&
							Material != PROP_MATERIAL &&
							Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
							Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
						{
							vNewPosition.fZ = Start.fZ - fCollideDistance;
						}
					}
				}
				else
				{
					vNewPosition.fZ = 0;
				}
			}
		}

		// set new position			
		UnitChangePosition(unit, vNewPosition);
		
		// are we now landing?
		if (TESTBIT(&move_result.flags, MOVERESULT_SETONGROUND))
		{
			if (!UnitIsOnGround( unit ))
			{
				UnitSetOnGround( unit, TRUE );
			}
		}
		else if (TESTBIT(&move_result.flags, MOVERESULT_TAKEOFFGROUND))
		{
			if (UnitIsOnGround( unit ))
			{
				UnitSetOnGround( unit, FALSE );
			}
		}
	}
	else
	{
		ROOM * room = UnitUpdateRoomFromPosition( UnitGetGame(unit), unit, &vPositionPrev, false );
		if ( !room )
		{
			SETBIT( &move_result.flags, MOVERESULT_OUTOFBOUNDS );
		}
	}

	// drb.path
	if ( UnitTestFlag(unit, UNITFLAG_PATHING) && !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING) && unit->m_pPathing )
	{
		if ( IS_CLIENT( game ) )
			c_UpdatePathing( game, unit, vPositionPrev, vNewPosition, &move_result );
		else
			s_UpdatePathing( game, unit, vPositionPrev, vNewPosition );
	}

	if( AppIsTugboat() )
	{
		// what if we got bumped back by a collision? newposition won't be the same as prev + vel
		sCheckDidCollideEvent( game, unit, vNewPosition, vPositionPrev, vCollisionNormal, TESTBIT( &move_result.flags, MOVERESULT_COLLIDE ) );
	}
	if( AppIsHellgate() )
	{
		VECTOR vPredictedPosition = vPositionPrev + vVelocity;
		sCheckDidCollideEvent( game, unit, vPredictedPosition, vPositionPrev, vCollisionNormal, TESTBIT( &move_result.flags, MOVERESULT_COLLIDE ) );
	}
	sCheckOutOfBoundsEvent( game, unit, TESTBIT( &move_result.flags, MOVERESULT_OUTOFBOUNDS ) );

	if (fLength > 0.0f )
	{
		// A collide event might have changed the missile distance, so let's reget it here.
		float fRevisedMissileDistance = UnitGetStatFloat(unit, STATS_MISSILE_DISTANCE, 0);
		if(fRevisedMissileDistance != fLength)
		{
			fLength = fRevisedMissileDistance;
		}
		else
		{
			fLength -= fDist;
		}
		UnitSetStatFloat(unit, STATS_MISSILE_DISTANCE, 0, fLength);
		if (fLength <= 0.0f)
		{
			UNIT *pTarget = UnitGetAITarget( unit );
			UnitTriggerEvent( game, EVENT_HITENDRANGE, unit, pTarget, NULL);
		}
		if( IS_CLIENT( game ) &&
			UnitDataTestFlag( UnitGetData( unit ), UNIT_DATA_FLAG_MISSILE_PLOT_ARC ) )
		{
			float fMaxRange = UnitGetStatFloat( unit, STATS_MISSILE_DISTANCE, 1 );
			float fDistance = ( fMaxRange - fLength ) / fMaxRange;
			VECTOR vDisplayPosition = unit->m_pActivePath->GetSplinePositionAtDistance( fDistance * unit->m_pActivePath->Length() );

			UnitSetDrawnPosition( unit, vDisplayPosition );
		}
	}

	BOOL bOnGround = UnitIsOnGround( unit );
	if ( !( unit->m_pPathing &&												// no automatic landing when pathing + flying
		    !UnitTestFlag( unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING ) &&
			TESTBIT( unit->pdwMovementFlags, MOVEFLAG_FLY ) ) )
	{		
		// did I fall?
		if (bOldOnGround && !bOnGround)
		{
			UnitSetZVelocity( unit, 0.0f );
		}

		if (!bOldOnGround && bOnGround)
		{
			UnitClearFlag(unit, UNITFLAG_JUMPING);
			UnitTriggerEvent(game, EVENT_LANDED_ON_GROUND, unit, NULL, NULL);
		}
	}

	//trace( "Collided = %i\n", (int)TESTBIT( &move_result.flags, MOVERESULT_COLLIDE ) );
	UnitCalculateTargetReached(game, unit, vPositionPrev, vNewPosition, TESTBIT( &move_result.flags, MOVERESULT_COLLIDE ), bOldOnGround, bOnGround);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStepListAdd(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

#if ISVERSION(DEVELOPMENT)
	ASSERT_RETURN(UnitGetGenus(unit) != GENUS_MONSTER || UnitGetTeam(unit) != INVALID_ID);
#endif
	if (UnitTestFlag(unit, UNITFLAG_FREED))
	{
		return;
	}

	ROOM * room = UnitGetRoom(unit);
	ASSERT_RETURN(room);

	if (UnitTestFlag(unit, UNITFLAG_DEACTIVATED))
	{
		return;
	}
	if (RoomIsActive(room) == FALSE)
	{
		if (UnitEmergencyDeactivate(unit, "Attempt to add unit from inactive room to step list"))
		{
			return;
		}
	}
	LEVEL * level = RoomGetLevel(room);
	ASSERT_RETURN(level && LevelIsActive(level));

	if ( IS_CLIENT( game ) && GameGetControlUnit( game ) == unit )
		return;
	if(AppIsHellgate())
	{
		if ( IS_SERVER( game ) && UnitGetGenus( unit ) == GENUS_PLAYER )
			return;
	}

	if ( UnitOnStepList( game, unit ) )
	{
		return;
	}

	unit->stepprev = NULL;
	unit->stepnext = game->pStepList;
	if (unit->stepnext)
	{
		unit->stepnext->stepprev = unit;
	}
	game->pStepList = unit;
	drbSuperTracker( unit );

	//trace( "start path[%c] - unit(%i)\n", HOST_CHARACTER(unit), UnitGetId( unit ) ); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitLandedOnGroundRemoveFromStepList(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId)
{
	if (UnitTestFlag(unit, UNITFLAG_STEPLIST_REMOVE_REQUESTED))
	{
		UnitStepListRemove(game, unit);
	}
	UnitUnregisterEventHandler(game, unit, EVENT_LANDED_ON_GROUND, dwId);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStepListRemove(
	GAME * game,
	UNIT * unit,
	DWORD dwFlags)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);

	if (!unit->stepprev && game->pStepList != unit)
	{
		ASSERT(unit->stepnext == NULL);
		return;		// unit isn't in the steplist
	}

	BOOL bForce = TEST_MASK(dwFlags, SLRF_FORCE);
	BOOL bDontSendPathPositionUpdate = TEST_MASK(dwFlags, SLRF_DONT_SEND_PATH_POSITION_UPDATE);

	if (AppIsHellgate() && !bForce && UnitTestFlag(unit, UNITFLAG_CLIENT_PLAYER_MOVING))
	{
		// players on the client should stay on the step list if this flag is set
		return;
	}

	if (bForce || UnitIsOnGround(unit) || TESTBIT(unit->pdwMovementFlags, MOVEFLAG_FLY) || unit->m_pPathing)
	{
		if (unit->stepnext)
		{
			unit->stepnext->stepprev = unit->stepprev;
		}
		if (unit->stepprev)
		{
			unit->stepprev->stepnext = unit->stepnext;
		}
		else
		{
			game->pStepList = unit->stepnext;
		}
		unit->stepnext = NULL;
		unit->stepprev = NULL;
		unit->fVelocityFromAcceleration = 0.0f;

		UnitClearFlag(unit, UNITFLAG_STEPLIST_REMOVE_REQUESTED);
		if (AppIsTugboat())
		{
			// always, always stop moving if you're not on the steplist in Mythos!
			if( !UnitTestFlag(unit, UNITFLAG_JUSTFREED) )
			{
				UnitEndModeGroup(unit, MODEGROUP_MOVE);
			}
			UnitDoPathTermination(game, unit);
		}
		if (AppIsHellgate())
		{
			if (UnitIsJumping(unit))
			{
				UnitForceToGround(unit);
			}
			if (unit->m_pPathing)
			{
				if (IS_CLIENT(game))
				{
#if HELLGATE_ONLY
					unit->m_pPathing->cFastPathIndex = INVALID_ID;
#endif
				}
				else if (IS_SERVER(game) && unit->m_pPathing->m_eState != PATHING_STATE_INACTIVE)
				{
					if (!UnitTestFlag(unit, UNITFLAG_JUSTFREED))
					{
						drbSuperTracker(unit);
						UnitEndPathReserve(unit);
						if(!bDontSendPathPositionUpdate)
						{
							s_SendUnitPathPositionUpdate(unit);
						}
					}
				}
	
				if (UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
				{
					UnitClearFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
					PathingUnitWarped(unit);
				}
				// We must check this again here because in certain crazy cases PathingUnitWarped() will cause
				// an event (such as UNITDIE_BEGIN) to get triggered which (naturally) happens immediately, and where we have
				// an event handler that frees the pathing structure.
				if(unit->m_pPathing)
				{
					unit->m_pPathing->m_nCurrentPath = INVALID_ID;
					unit->m_pPathing->m_eState = PATHING_STATE_INACTIVE;
				}
			}
		}
		UnitClearFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);

		//trace( "end path[%c] - unit(%i)\n", HOST_CHARACTER(unit), UnitGetId( unit ) ); 
		return;
	}
	else
	{
		if (!UnitTestFlag(unit, UNITFLAG_STEPLIST_REMOVE_REQUESTED))
		{
			UnitSetFlag(unit, UNITFLAG_STEPLIST_REMOVE_REQUESTED, TRUE);
			UnitRegisterEventHandler(game, unit, EVENT_LANDED_ON_GROUND, sUnitLandedOnGroundRemoveFromStepList, EVENT_DATA());
		}
	}
	return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitDoPathTermination(
	GAME* game,
	UNIT* unit )
{
	if ( unit->m_pPathing )
	{
		// TRAVIS: Client players need to reserve on the local machine so that they can path client-side.
		 if ( IS_CLIENT( game ) && GameGetControlUnit( game ) == unit && 
			  unit->m_pPathing->m_eState != PATHING_STATE_INACTIVE )
		{
			drbSuperTracker( unit );
			UnitEndPathReserve( unit );
			// TRAVIS: FIXME we probably DO need to do this. But let's get pathing working client-side first.
			//c_SendUnitPathPositionUpdate( unit );
			// send a warp to the server - tell 'em to STOP!
			//VECTOR vUp = VECTOR( 0.0f, 0.0f, 1.0f );
			
#if !ISVERSION(SERVER_VERSION)
			DWORD dwUnitWarpFlags = 0;	
			SETBIT( dwUnitWarpFlags, UW_SOFT_WARP );
			c_SendUnitWarp( unit, UnitGetRoom( unit ), UnitGetPosition( unit ), UnitGetFaceDirection( unit, FALSE ), UnitGetVUpDirection( unit ), dwUnitWarpFlags  );
#endif
			// MOVING THIS TO the START of pathing for the player
		}
		else if ( IS_SERVER( game ) &&
				  unit->m_pPathing->m_eState != PATHING_STATE_INACTIVE )
		{
			drbSuperTracker( unit );
			UnitEndPathReserve( unit );
			// players don't send these stops from the server - the client sends them.
			if( !UnitIsA( unit, UNITTYPE_PLAYER ) )
			{
				s_SendUnitPathPositionUpdate( unit );
			}
		}
		else if ( IS_CLIENT( game ) )
		{
			// if the server asked us to turn before we finished walking,\
			// take care of that now!
			if( AppIsTugboat() && VectorLengthSquared(  unit->vQueuedFaceDirection ) != 0 )
			{
				UnitSetFaceDirection( unit, unit->vQueuedFaceDirection );
				unit->vQueuedFaceDirection = VECTOR( 0, 0, 0 );
			}

#if HELLGATE_ONLY
			unit->m_pPathing->cFastPathIndex = INVALID_ID;
#endif
		}
		
		if(UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
		{
			PathingUnitWarped(unit);
		}
		ROOM * pTestRoom = RoomGetByID( game, unit->m_pPathing->m_tOccupiedLocation.m_nRoomId );
		if( pTestRoom )
		{
			UnitClearBothPathNodes( unit );
		}
		unit->m_pPathing->m_nCurrentPath = INVALID_ID;
		unit->m_pPathing->m_eState = PATHING_STATE_INACTIVE;
		UnitReservePathOccupied( unit );



	}
	UnitClearFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitOnStepList(
	GAME* game,
	UNIT* unit)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);

	return (game->pStepList == unit || unit->stepprev);
}


//----------------------------------------------------------------------------
// TODO: items that aren't moving should be taken off of the steplist!!!
//----------------------------------------------------------------------------
void DoUnitStepping(
	GAME* game)
{
	PERF(STEPPING);

	UNIT * unit = game->pStepList;

#if ISVERSION(DEVELOPMENT)
	if ( IS_CLIENT( game ) )
		drbCltStepCount = 0;
	else
		drbSrvStepCount = 0;
#endif

	while (unit)
	{
		UNIT * next = unit->stepnext;

#ifdef TRACE_SYNCH
		{
			VECTOR vVelocity = UnitGetMoveDirection(unit);
			trace("stepping[%c]: UNIT:" ID_PRINTFIELD "  FROM:(%4.4f, %4.4f, %4.4f)  VEL:%4.4f (%4.4f, %4.4f, %4.4f)  ",
				IS_SERVER(unit) ? 'S' : 'C', 
				ID_PRINT(UnitGetId(unit)), 
				unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ,
				unit->fVelocity, 
				vVelocity.fX, vVelocity.fY, vVelocity.fZ);
		}
#endif
		if(ABS(unit->fZVelocity) > MAX_ALLOWED_Z_VELOCITY_MAGNITUDE)
		{
			//possibly trace the unit here.  It's probably gold.
			ASSERTV_MSG("Unit [%s] has absurd Z velocity at position (%.2f, %.2f, %.2f), removing from steplist.", UnitGetDevName(unit), unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ);
			unit->fZVelocity = 0.0f;
			UnitStepListRemove(game, unit, SLRF_FORCE);
		}
		else
		{
			VECTOR pos;
			sUnitDoStep<TRUE>(game, unit, pos);
		}
#if ISVERSION(DEVELOPMENT)
		if (IS_CLIENT(game))
			drbCltStepCount++;
		else
			drbSrvStepCount++;
#endif

#if ISVERSION(SERVER_VERSION)
		GameServerContext * pContext = game->pServerContext;
		if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerUnitStepRate,1);

		GENUS eGenus = UnitGetGenus(unit);
		switch (eGenus)
		{
		case GENUS_MONSTER:
			if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,
				GameServerUnitStepMonsterRate,1);
			break;
		case GENUS_ITEM:
			if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,
				GameServerUnitStepItemRate,1);
			break;
		case GENUS_MISSILE:
			if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,
				GameServerUnitStepMissileRate,1);
			break;
		case GENUS_PLAYER:
			if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,
				GameServerUnitStepPlayerRate,1);
			break;
		case GENUS_OBJECT:
			{
				BOOL bAssert = TRUE;
				const UNIT_DATA * pUnitData = UnitGetData(unit);
				if(pUnitData && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_ALLOW_OBJECT_STEPPING))
				{
					bAssert = FALSE;
				}
				if(bAssert)
				{
					ASSERTV_MSG("Stepping unit of GENUS_OBJECT '%s' [%d]", UnitGetDevName(unit), UnitGetId(unit));
				}
			}
			break;
		default:
			break;
		}

#endif

		// trace egregious sync errors
#ifdef TRACE_SYNCH
		{
			if (!unit)
			{
				trace("DEAD!\n");
				return;
			}
			trace("TO:(%4.4f, %4.4f, %4.4f)  TIME:%d\n",
				unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ,
				GameGetTick(UnitGetGame(unit)));

			if (IS_CLIENT(game))
			{
#if !ISVERSION(CLIENT_ONLY_VERSION)
				GAME* srvGame = AppGetSrvGame();
				if (srvGame)
				{
					UNIT* s_unit = UnitGetById(srvGame, UnitGetId(unit));
   					if (s_unit)
   					{
						if (unit->vPosition.fX - s_unit->vPosition.fX > 0.1 || 
							unit->vPosition.fY - s_unit->vPosition.fY > 0.1 || 
							unit->vPosition.fZ - s_unit->vPosition.fZ > 0.1)
						{
							trace("synch error: UNIT:" ID_PRINTFIELD "  POS:(%4.4f, %4.4f, %4.4f)  SRV:(%4.4f, %4.4f, %4.4f)  TIME:%d\n",
								ID_PRINT(UnitGetId(unit)),
								unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ,
								s_unit->vPosition.fX, s_unit->vPosition.fY, s_unit->vPosition.fZ,
								GameGetTick(UnitGetGame(unit)));
						}
   					}
				}
#endif
			}
		}
#endif
		unit = next;
	}
}


//----------------------------------------------------------------------------
// note that the gfCos360Tbl[pGfx->nAngle] * pGfx->fVelocity etc is a constant!!!
//----------------------------------------------------------------------------
static void sUnitDoStepInterpolate(
	GAME * pGame,
	UNIT * unit,
	GAME * srvGame,
	UNIT * srvUnit,
	float fTimeDelta,
	float fRealTimeDelta)
{
	if ( UnitTestFlag( unit, UNITFLAG_DONT_STEP_JUST_INTERPOLATE ) )
	{
		if(fTimeDelta > 0)
		{
			float fDeltaPercent;
			const VECTOR * pvMoveTarget = UnitGetMoveTarget( unit );
			if ( pvMoveTarget )
			{
				fDeltaPercent = 0.8f;

				VECTOR oldpos = UnitGetDrawnPosition(unit);
				VECTOR vDelta = *pvMoveTarget - unit->vPosition;
				UnitSetDrawnPosition(unit, (vDelta * fDeltaPercent) + unit->vPosition);

				VECTOR vMovement = UnitGetDrawnPosition(unit) - unit->vPosition;
				float fVelocity = VectorLength( vMovement );
				UnitSetVelocity( unit, fVelocity );
				UnitSetPosition(unit, UnitGetDrawnPosition(unit));
				UnitUpdateRoomFromPosition(pGame, unit, &oldpos);
			}
		}
	}
	else
	{
		VECTOR vNextPosition;
		if ( srvUnit && srvGame )
		{
			sUnitDoStep<FALSE>(srvGame, srvUnit, vNextPosition);
			VECTOR vDelta = vNextPosition - unit->vPosition;
			UnitSetDrawnPosition(unit, (vDelta * fTimeDelta) + unit->vPosition);
		}
		else
		{
			// why does tugboat do this? well, we really do move along the spline, not just tween
			// from position to position. note that Tugboat is treating this like a REAL step-
			if( AppIsTugboat() )
			{
				sUnitDoStep<TRUE>( pGame, unit, vNextPosition, fRealTimeDelta );
			}
			else if( AppIsHellgate() )
			{
				// Note that there is some Tugboat code here that has been removed.
				// If weird stuff starts happening to missiles, then this might be one place to look.
				sUnitDoStep<FALSE>(pGame, unit, vNextPosition);
				VECTOR vDelta = vNextPosition - unit->vPosition;
				UnitSetDrawnPosition(unit, (vDelta * fTimeDelta) + unit->vPosition);
			}
		}
	}

	c_UnitUpdateGfx(unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DoUnitStepInterpolating(
	GAME* game,
	float fTimeDelta,
	float fRealTimeDelta)
{
	ASSERT(IS_CLIENT(game));
	fTimeDelta /= GAME_TICK_TIME;
	if (fTimeDelta <= 0.0f &&
		fRealTimeDelta <= 0.0f)
	{
		return;
	}
	if (fTimeDelta > 1.0f)
	{
		fTimeDelta = 1.0f;
	}

	GAME * srvGame = NULL;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	if ( AppGetForceSynch() && ( AppGetType() == APP_TYPE_SINGLE_PLAYER ) )
	{
		GAME * srvGame = AppGetSrvGame();
		REF(srvGame);
	}
#endif
	UNIT* unit = game->pStepList;
	while (unit)
	{
		UNIT *next = unit->stepnext;  // just incase we ever come up with logic where we remove during step
		UNIT * s_unit = NULL;

		if ( srvGame 
#ifdef HAVOK_ENABLED
			&& !unit->pHavokRigidBody 
#endif
			)
			s_unit = UnitGetById(srvGame, UnitGetId(unit));

		sUnitDoStepInterpolate(game, unit, srvGame, s_unit, fTimeDelta, fRealTimeDelta);
		unit = next;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitActivatePathClient( UNIT * unit )
{
	ASSERT_RETURN( unit );
	if ( !unit->m_pPathing )
		return;

	PATH_STATE* path = unit->m_pPathing;
#if HELLGATE_ONLY
	if ( path->m_eState != PATHING_STATE_ACTIVE_CLIENT && UnitOnStepList( UnitGetGame( unit ), unit ) && ( path->cFastPathSize > 0 ) )
	{
		drbSuperTracker( unit );
		path->m_eState = PATHING_STATE_ACTIVE_CLIENT;
		path->cFastPathIndex = 0;
	}
#else
	if ( path->m_eState != PATHING_STATE_ACTIVE_CLIENT && UnitOnStepList( UnitGetGame( unit ), unit )  )
	{
		drbSuperTracker( unit );
		path->m_eState = PATHING_STATE_ACTIVE_CLIENT;

	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientCalculatePath(
	GAME * pGame,
	UNIT * pUnit,
	UNITMODE eMode,
	PATH_CALCULATION_TYPE eMaxPathCalculation,
	DWORD dwFlags)
{
#if HELLGATE_ONLY
	// drb.path - ONLY pathing on the server now...
	ASSERT( IS_CLIENT( pGame ) );
	if (UnitTestFlag(pUnit, UNITFLAG_PATHING))
	{
		drbSuperTracker( pUnit );

		if(SkillIsOnWithFlag(pUnit, SKILL_FLAG_DISABLE_CLIENTSIDE_PATHING))
		{
			return FALSE;
		}

		if(UnitTestFlag(pUnit, UNITFLAG_DISABLE_CLIENTSIDE_PATHING))
		{
			return FALSE;
		}

		if(!UnitIsOnGround( pUnit ))
		{
			return FALSE;
		}

		PATH_STATE* path = pUnit->m_pPathing;
		if (!path)
		{
			pUnit->m_pPathing = PathStructInit(pGame);
			if ( UnitGetRoom( pUnit ) )
				PathingUnitWarped( pUnit ); //does some nice checking to make sure we are in a good spot. Not sure if this is over kill.

			path = pUnit->m_pPathing;
			ASSERT_RETFALSE(path);
		}

		if( AppIsTugboat() )
		{
			if ( path->m_eState != PATHING_STATE_INACTIVE )
			{
				drbSuperTracker( pUnit );
				UnitEndPathReserve( pUnit );
			}
	 		// clear where I was
			UnitClearBothPathNodes( pUnit );
		}
		PATHING_STATES old_eState = path->m_eState;
		int nCurrentPathOld = INVALID_ID;
		int nPathNodeCountOld = 0;

		ASTAR_OUTPUT pPathNodesOld[MAX_CLIENT_FAST_PATH_SIZE];
		if (path->m_pPathNodes)
		{

			MemoryCopy(pPathNodesOld, MAX_CLIENT_FAST_PATH_SIZE * sizeof(ASTAR_OUTPUT), path->cFastPathNodes, path->cFastPathSize * sizeof(ASTAR_OUTPUT));
			nPathNodeCountOld = path->cFastPathSize;
			nCurrentPathOld = path->cFastPathIndex;
			path->cFastPathSize = 0;
			path->cFastPathIndex = INVALID_ID;
		}

		path->m_eState = PATHING_STATE_INACTIVE;
		sDestroyPathParticles(pGame, path);

		// drb.path ----------------------------------------------------------------------------
		// path either to a target location, or in-front of a target unit
		VECTOR vTarget;
		BOOL bTarget = FALSE;
		UNIT* pTargetUnit = NULL;
		if (pUnit->idMoveTarget != INVALID_ID)
		{
			pTargetUnit = UnitGetById(UnitGetGame(pUnit), pUnit->idMoveTarget);
			if (pTargetUnit)
			{
				// target loc
				vTarget = UnitGetPosition( pTargetUnit );
				// if jumping, use my z instead.
				if ( UnitIsJumping( pTargetUnit ) )
				{
					vTarget.z = pUnit->vPosition.z;
				}
				VECTOR vDirection = pUnit->vPosition - vTarget;
				VectorNormalize( vDirection );
				vTarget += vDirection;
				AI_FindPathNode(pGame, pUnit, vTarget, pTargetUnit);
				bTarget = TRUE;
			}
		}
		else
		{
			if (pUnit->vMoveTarget.fX || pUnit->vMoveTarget.fY || pUnit->vMoveTarget.fZ)
			{
				vTarget = pUnit->vMoveTarget;
				AI_FindPathNode(pGame, pUnit, vTarget, pTargetUnit);
				bTarget = TRUE;
			}
		}

		// either path to vTarget, or move forward
		BOOL bSuccess;
		if (bTarget)
		{
			if(AppIsHellgate())
			{
				SETBIT(dwFlags, PCF_CALCULATE_SOURCE_POSITION_BIT);
			}
			bSuccess = UnitCalculatePath(pGame, pUnit, pTargetUnit, &path->cFastPathSize, path->cFastPathNodes, vTarget, eMaxPathCalculation, dwFlags, MAX_CLIENT_FAST_PATH_SIZE);
		}
		else
		{
			if(AppIsHellgate())
			{
				SETBIT(dwFlags, PCF_CALCULATE_SOURCE_POSITION_BIT);
			}

			VECTOR vMoveDirection = UnitGetMoveDirection(pUnit);
			float fVelocity = UnitGetVelocity(pUnit);
			vTarget = UnitGetPosition(pUnit) + fVelocity * vMoveDirection;
			bSuccess = UnitCalculatePath(pGame, pUnit, NULL, &path->cFastPathSize, path->cFastPathNodes, vTarget, PATH_STRAIGHT, dwFlags, MAX_CLIENT_FAST_PATH_SIZE);
		}

		if (bSuccess && path->cFastPathSize)
		{
			drbSuperTracker( pUnit );
			MemoryCopy(path->m_pPathNodes, MAX_CLIENT_FAST_PATH_SIZE * sizeof(ASTAR_OUTPUT), path->cFastPathNodes, path->cFastPathSize * sizeof(ASTAR_OUTPUT));
			path->m_nPathNodeCount = path->cFastPathSize;
			path->m_nCurrentPath = 0;

			sCreatePathParticles(pGame, pUnit, path);

			UnitStepListAdd(pGame, pUnit);

			c_UnitSetMode(pUnit, eMode);

			UnitActivatePathClient( pUnit );

			return TRUE;
		}
		else
		{
			drbSuperTracker( pUnit );
			// restore old path
			MemoryCopy(path->cFastPathNodes, MAX_CLIENT_FAST_PATH_SIZE * sizeof(ASTAR_OUTPUT), pPathNodesOld, nPathNodeCountOld * sizeof(ASTAR_OUTPUT));
			path->cFastPathSize = nPathNodeCountOld;
			path->cFastPathIndex = nCurrentPathOld;
			path->m_eState = old_eState;

			if ( path->cFastPathSize && path->cFastPathIndex < path->cFastPathSize && path->cFastPathIndex >= 0 && path->m_eState == PATHING_STATE_ACTIVE_CLIENT )
			{
				drbSuperTracker( pUnit );
				path->m_eState = PATHING_STATE_ACTIVE_CLIENT;
				sCreatePathParticles(pGame, pUnit, path);
			}

			return FALSE;
		}
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitCalculatePath(
	GAME* game,
	UNIT * unit,
	PATH_CALCULATION_TYPE eMaxPathCalculation,
	DWORD dwFlags)
{
	// drb.path - ONLY pathing on the server now...
	// except for Tugboat
	if( AppIsHellgate() )
	{
		ASSERT( IS_SERVER( game ) );
	}
#if ISVERSION(DEVELOPMENT)
	drbNumPaths++;
#endif
	if (UnitTestFlag(unit, UNITFLAG_PATHING))
	{
		drbSuperTracker( unit );
		if(!UnitIsOnGround( unit ))
		{
			return FALSE;
		}

		PATH_STATE* path = unit->m_pPathing;
		if (!path)
		{
			// this will recreate the pathstate, as well as occupy nodes for us.
			unit->m_pPathing = PathStructInit(game);
			if ( UnitGetRoom( unit ) )
				PathingUnitWarped( unit ); //does some nice checking to make sure we are in a good spot. Not sure if this is over kill.
			path = unit->m_pPathing;
			ASSERT_RETFALSE(path);
		}

		if ( path->m_eState != PATHING_STATE_INACTIVE )
		{
			drbSuperTracker( unit );
			UnitEndPathReserve( unit );
		}
		if( AppIsTugboat() )
		{
	 		// clear where I was
			UnitClearBothPathNodes( unit );
		}

		PATHING_STATES old_eState = path->m_eState;
		int nCurrentPathOld = INVALID_ID;
		int nPathNodeCountOld = 0;
		ASTAR_OUTPUT pPathNodesOld[DEFAULT_ASTAR_LIST_SIZE];
		if (path->m_pPathNodes)
		{
			MemoryCopy(pPathNodesOld, DEFAULT_ASTAR_LIST_SIZE * sizeof(ASTAR_OUTPUT), path->m_pPathNodes, path->m_nPathNodeCount * sizeof(ASTAR_OUTPUT));
			nPathNodeCountOld = path->m_nPathNodeCount;
			nCurrentPathOld = path->m_nCurrentPath;
			path->m_nPathNodeCount = 0;
			path->m_nCurrentPath = INVALID_ID;
		}

		path->m_eState = PATHING_STATE_INACTIVE;
		sDestroyPathParticles(game, path);

		// drb.path ----------------------------------------------------------------------------
		// path either to a target location, or in-front of a target unit
		VECTOR vTarget;
		BOOL bTarget = FALSE;
		UNIT* pTargetUnit = NULL;
		if (unit->idMoveTarget != INVALID_ID)
		{
			pTargetUnit = UnitGetById(UnitGetGame(unit), unit->idMoveTarget);
			if (pTargetUnit)
			{
				// target loc
				vTarget = UnitGetPosition( pTargetUnit );
				// if jumping, use my z instead.
				if ( UnitIsJumping( pTargetUnit ) )
				{
					vTarget.z = unit->vPosition.z;
				}
				VECTOR vDirection = unit->vPosition - vTarget;
				VectorNormalize( vDirection );
				vTarget += vDirection;
				AI_FindPathNode(game, unit, vTarget, pTargetUnit);
				bTarget = TRUE;
			}
		}
		else
		{
			if (unit->vMoveTarget.fX || unit->vMoveTarget.fY || unit->vMoveTarget.fZ)
			{
				vTarget = unit->vMoveTarget;
				if(!TESTBIT(dwFlags, PCF_ALREADY_FOUND_PATHNODE_BIT))
				{
					// In cases where we're clicking nowhere, 
					// step back a few times to try to find a near point to path to
					// for player pathing.
					if( AppIsTugboat() && IS_CLIENT(game) && UnitIsA( unit, UNITTYPE_PLAYER ))
					{
						VECTOR vDelta = vTarget - UnitGetPosition( unit );
						vDelta.fZ = 0;
						int nMaxTries = (int)VectorLength( vDelta ) - 1;
						nMaxTries = MIN( 10, nMaxTries );
						VectorNormalize( vDelta );
						int nTries = 0;
						VECTOR vSavedTarget = vTarget;
						while( nTries < nMaxTries && !AI_FindPathNode(game, unit, vTarget, pTargetUnit) )
						{
							vTarget -= vDelta;
							nTries++;
						}
						//vDelta = vSavedTarget - vTarget;
						//vDelta.fZ = 0;
						// if the found location is more than 10 meters away from our target
						// it probably sucks. Why don't we just try walking in the intended direction instead.
						float fDot = VectorDot( vDelta, vTarget - UnitGetPosition( unit ) );
						if( nTries >=  nMaxTries  || fDot <= 0  )
						{
							VECTOR vDelta = vSavedTarget - UnitGetPosition( unit );
							vDelta.fZ = 0;
							VectorNormalize( vDelta );
							vTarget = unit->vPosition + vDelta * 2;
							AI_FindPathNode(game, unit, vTarget, pTargetUnit);
						}
					}
					else
					{
						AI_FindPathNode(game, unit, vTarget, pTargetUnit);
					}
				}
				bTarget = TRUE;
			}
		}

		// either path to vTarget, or move forward
		BOOL bSuccess;
		if (bTarget)
		{
			bSuccess = UnitCalculatePath(game, unit, pTargetUnit, &path->m_nPathNodeCount, path->m_pPathNodes, vTarget, eMaxPathCalculation, dwFlags);
		}
		else
		{
			VECTOR vMoveDirection = UnitGetMoveDirection(unit);
			float fVelocity = UnitGetVelocity(unit);
			vTarget = UnitGetPosition(unit) + fVelocity * vMoveDirection;
			bSuccess = UnitCalculatePath(game, unit, NULL, &path->m_nPathNodeCount, path->m_pPathNodes, vTarget, PATH_STRAIGHT, dwFlags);
		}
		
		// units that allow non-node pathing can stay at the same node, but change
		// locations within that range ( players )
		if( !bSuccess && UnitTestFlag( unit, UNITFLAG_ALLOW_NON_NODE_PATHING ) )
		{
			ROOM * pSourceRoom = NULL;
			ROOM_PATH_NODE * pSourceNode = NULL;
			int nSourceIndex = INVALID_ID;
			if(!TESTBIT(dwFlags, PCF_CALCULATE_SOURCE_POSITION_BIT))
			{
				UnitGetPathNodeOccupied( unit, &pSourceRoom, &nSourceIndex );
			}
			if(pSourceRoom && nSourceIndex != INVALID_ID)
			{
				pSourceNode = RoomGetRoomPathNode( pSourceRoom, nSourceIndex );
			}
			else
			{
				pSourceNode = RoomGetNearestPathNode(game, UnitGetRoom(unit), UnitGetPosition(unit), &pSourceRoom);
			}
			if(pSourceNode)
			{
				path->m_nPathNodeCount = 1;
				path->m_pPathNodes[0].nPathNodeIndex = pSourceNode->nIndex;
				path->m_pPathNodes[0].nRoomId = RoomGetId(pSourceRoom);
				bSuccess = TRUE;
			}
		}
		// do a little check to see if we got an aborted path heading in the wrong direction
		if( AppIsTugboat() && bSuccess && path->m_nPathNodeCount == 1 )
		{
			ROOM * pNodeRoom = RoomGetByID(game, path->m_pPathNodes[0].nRoomId);
			if(pNodeRoom)
			{
				ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pNodeRoom, path->m_pPathNodes[0].nPathNodeIndex);

				VECTOR vPos = RoomPathNodeGetExactWorldPosition(game, pNodeRoom, pPathNode);
				vPos -= UnitGetPosition(unit);
				vPos.fZ = 0;
				VectorNormalize( vPos );
				VECTOR vDir = *UnitGetMoveTarget( unit );
				vDir -= UnitGetPosition(unit);
				vDir.fZ = 0;
				VectorNormalize( vDir );

				float flDot = VectorDot( vPos, vDir );
				if( flDot <= 0 )
				{
					bSuccess = FALSE;
				}
			}
			else
			{
				bSuccess = FALSE;
			}
		}
		BOOL bTestOnly = TESTBIT(dwFlags, PCF_TEST_ONLY_BIT);
		if (!bTestOnly && bSuccess && path->m_nPathNodeCount)
		{
			drbSuperTracker( unit );
			sCreatePathParticles(game, unit, path);

			UnitActivatePath( unit );

			return TRUE;
		}
		else
		{
			/*
			VECTOR vPos = UnitGetPosition(unit);
			trace("[%c] A* Failed!  From {%.2f, %.2f, %.2f} To {%.2f, %.2f, %.2f}\n",
			HOST_CHARACTER(game),
			vPos.fX, vPos.fY, vPos.fZ, 
			vTarget->fX, vTarget->fY, vTarget->fZ);
			// */
			
			drbSuperTracker( unit );
			// restore old path
			MemoryCopy(path->m_pPathNodes, DEFAULT_ASTAR_LIST_SIZE * sizeof(ASTAR_OUTPUT), pPathNodesOld, nPathNodeCountOld * sizeof(ASTAR_OUTPUT));
			path->m_nPathNodeCount = nPathNodeCountOld;
			path->m_nCurrentPath = nCurrentPathOld;
			path->m_eState = old_eState;

			if ( path->m_nPathNodeCount && path->m_nCurrentPath < path->m_nPathNodeCount && path->m_nCurrentPath >= 0 && path->m_eState == PATHING_STATE_ACTIVE )
			{
				drbSuperTracker( unit );
				path->m_eState = PATHING_STATE_ACTIVE;
				sCreatePathParticles(game, unit, path);

				// should never fail on a path restore
				BOOL bReserveSuccess = UnitReservePathNextNode( unit );
				ASSERT( bReserveSuccess );
			}

			if(bTestOnly)
			{
				return bSuccess;
			}
			return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitActivatePath(
	UNIT * unit,
	BOOL bSendPathPositionUpdate )
{
	ASSERT_RETURN( unit );
	if ( !unit->m_pPathing )
		return;

	PATH_STATE* path = unit->m_pPathing;
	if ( !UnitPathIsActive( unit ) && UnitOnStepList( UnitGetGame( unit ), unit ) && ( path->m_nPathNodeCount > 0 ) )
	{
		drbSuperTracker( unit );
		path->m_eState = PATHING_STATE_ACTIVE;
		path->m_nCurrentPath = 0;
#ifdef DRB_SPLINE_PATHING
		path->vStart = unit->vPosition;
		path->m_Time = AppCommonGetCurTime();
#endif
		if( bSendPathPositionUpdate && ( AppIsTugboat() || IS_SERVER(unit) ) )
		{
			BOOL bSuccess = UnitReservePathNextNode( unit );
			ASSERT( bSuccess );
			if( IS_SERVER(unit) )
			{
				s_SendUnitPathPositionUpdate( unit );
			}
		}
	}
	else
	{
		if(AppIsTugboat())
		{
			if( !UnitPathIsActive( unit ) && ( path->m_nPathNodeCount > 0 ) )
			{
				if( UnitIsA( unit, UNITTYPE_PLAYER ) )
				{
					drbSuperTracker( unit );
					path->m_eState = PATHING_STATE_ACTIVE;
					path->m_nCurrentPath = 0;
					BOOL bSuccess = UnitReservePathNextNode( unit );
					ASSERT( bSuccess );
					GAME * game = UnitGetGame( unit );
					if ( IS_CLIENT( game ) )
					{
						if( GameGetControlUnit( game ) == unit )
						{
#if !ISVERSION(SERVER_VERSION)
							// trying this at the start of path to remove popping at path termination
							c_SendUnitMoveXYZ( unit, 0, MODE_RUN,  unit->vMoveTarget, unit->vMoveDirection, TRUE );
#endif
						}
					}
					else
					{
						if(bSendPathPositionUpdate)
						{
							s_SendUnitPathPositionUpdate( unit );
						}
					}
				}
				else
				{
					DWORD dwUnitWarpFlags = 0;	
					SETBIT( dwUnitWarpFlags, UW_PATHING_WARP );
					SETBIT( dwUnitWarpFlags, UW_SOFT_WARP );
					s_SendUnitWarp( unit, UnitGetRoom( unit ), UnitGetPosition( unit ), UnitGetFaceDirection( unit, FALSE ), UnitGetVUpDirection( unit ), dwUnitWarpFlags  );
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
BOOL UnitIsMovementDebugUnit(
	UNIT *pUnit)
{
	char *pszMoveSendDebugUnitClass = NULL;
//	char *pszMoveSendDebugUnitClass = "carnagor_pet";
	
	if (pszMoveSendDebugUnitClass != NULL)
	{
		if (PStrStrI( UnitGetDevName( pUnit ), pszMoveSendDebugUnitClass ))
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
void MovementCheckDebugUnit(
	UNIT *pUnit,
	const char *pszLabel,
	const VECTOR *pVector)
{

	if (UnitIsMovementDebugUnit( pUnit ))	
	{
#if !ISVERSION(SERVER_VERSION)
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE,
			"%s %s (%.2f, %.2f, %.2f) [On Step List=%s]", 
			UnitGetDevName( pUnit ), 
			pszLabel, 
			pVector ? pVector->fX : 0.0f,
			pVector ? pVector->fY : 0.0f,
			pVector ? pVector->fZ : 0.0f,
			(pUnit->stepnext || pUnit->stepprev) ? "yes" : "no" );
		ConsoleString( CONSOLE_SYSTEM_COLOR, szMessage );
		LogMessage( szMessage );
#else
		TraceDebugOnly(
			"%s %s (%.2f, %.2f, %.2f) [On Step List=%s]", 
			UnitGetDevName( pUnit ), 
			pszLabel, 
			pVector ? pVector->fX : 0.0f,
			pVector ? pVector->fY : 0.0f,
			pVector ? pVector->fZ : 0.0f,
			(pUnit->stepnext || pUnit->stepprev) ? "yes" : "no" );
#endif
			
	}
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitWarp(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_WARP", &vPosition );
#endif
	
	MSG_SCMD_UNITWARP msg;	
	msg.id = UnitGetId( pUnit );
	msg.idRoom = RoomGetId( pRoom );
	msg.vPosition = vPosition;
	msg.vFaceDirection = vFaceDirection;
	msg.vUpDirection = vUpDirection;
	msg.dwUnitWarpFlags = dwUnitWarpFlags;
	
	s_SendUnitMessage( pUnit, SCMD_UNITWARP, &msg );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitWarpToOthers(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_WARP", &vPosition );
#endif
	
	MSG_SCMD_UNITWARP msg;	
	msg.id = UnitGetId( pUnit );
	msg.idRoom = RoomGetId( pRoom );
	msg.vPosition = vPosition;
	msg.vFaceDirection = vFaceDirection;
	msg.vUpDirection = vUpDirection;
	msg.dwUnitWarpFlags = dwUnitWarpFlags;
	
	s_SendUnitMessage( pUnit, SCMD_UNITWARP, &msg, UnitGetClientId( pUnit ) );
}	

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendUnitWarp(
	UNIT *pUnit,
	ROOM *pRoom,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection,
	const VECTOR &vUpDirection,
	DWORD dwUnitWarpFlags)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_WARP", &vPosition );
#endif
	
	MSG_CCMD_UNITWARP msg;	

	msg.idRoom = RoomGetId( pRoom );
	msg.vPosition = vPosition;
	msg.vFaceDirection = vFaceDirection;
	msg.vUpDirection = vUpDirection;
	msg.dwUnitWarpFlags = dwUnitWarpFlags;
	
	c_SendMessage( CCMD_UNITWARP, &msg );
	
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitPathPositionUpdate(
	UNIT *pUnit )
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	if ( !pUnit->m_pPathing )
		return;

	ASSERT_RETURN( IS_SERVER( UnitGetGame( pUnit ) ) );

	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

	PATH_STATE * path = pUnit->m_pPathing;
	ASSERT_RETURN(path);

	if (path->m_tOccupiedLocation.m_nRoomId == INVALID_ID)
	{
		return;
	}

	MSG_SCMD_UNIT_PATH_POSITION_UPDATE msg;	
	msg.id = UnitGetId( pUnit );
	msg.idPathRoom = path->m_tOccupiedLocation.m_nRoomId;
	msg.nPathIndex = path->m_tOccupiedLocation.m_nRoomPathNodeIndex;

#ifdef _DEBUG
	ROOM * pRoom = RoomGetByID(UnitGetGame(pUnit), path->m_tOccupiedLocation.m_nRoomId);
	ASSERT(pRoom);
	if(pRoom)
	{
		ROOM_PATH_NODE * pNode = RoomGetRoomPathNode(pRoom, path->m_tOccupiedLocation.m_nRoomPathNodeIndex);
		ASSERT(pNode);
	}
#endif

	s_SendUnitMessage( pUnit, SCMD_UNIT_PATH_POSITION_UPDATE, &msg );
	
}	

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendUnitPathPositionUpdate(
	UNIT *pUnit )
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	if ( !pUnit->m_pPathing )
		return;

	ASSERT_RETURN( IS_CLIENT( UnitGetGame( pUnit ) ) );

	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

	PATH_STATE * path = pUnit->m_pPathing;

	MSG_CCMD_UNIT_PATH_POSITION_UPDATE msg;	

	msg.idPathRoom = path->m_tOccupiedLocation.m_nRoomId;
	msg.nPathIndex = path->m_tOccupiedLocation.m_nRoomPathNodeIndex;

	c_SendMessage( CCMD_UNIT_PATH_POSITION_UPDATE, &msg );
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitMoveXYZ(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vTargetPosition,
	const VECTOR &vMoveDirection,
	UNIT * pOptionalFaceTarget,
	BOOL bSendPath,
	GAMECLIENTID idClientExclude)
{
	ASSERTX_RETURN(pUnit, "Expected unit");
	if (!UnitCanSendMessages(pUnit))
		return;

	GAME* pGame = UnitGetGame(pUnit);
	ASSERT_RETURN(pGame);
	if (GameGetState(pGame) != GAMESTATE_RUNNING)
		return;

	ROOM* pRoom = UnitGetRoom(pUnit);
	ASSERTX_RETURN(pRoom, "Cannot send unit message for unit that is not in a room");	
	LEVEL* pLevel = RoomGetLevel(pRoom);
	ASSERT_RETURN(pRoom);	

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_MOVE_XYZ", &vTargetPosition );
#endif

	MSG_SCMD_UNITMOVEXYZ msg;
	msg.id = UnitGetId( pUnit );
	msg.bFlags = bFlags;
	msg.mode = eMode;
	msg.fVelocity = UnitGetVelocity( pUnit );
	msg.fStopDistance = UnitGetStatFloat(pUnit, STATS_STOP_DISTANCE, 0);
	msg.TargetPosition = vTargetPosition;
	msg.MoveDirection  = vMoveDirection;
	msg.idFaceTarget = UnitGetId(pOptionalFaceTarget);
	
	if (bSendPath)
	{
		int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf);
		MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
	}
	else
	{
		BIT_BUF tBuf(msg.buf, MAX_PATH_BUFFER);
		PathStructSerialize(pGame, NULL, tBuf);
		MSG_SET_BLOB_LEN(msg, buf, tBuf.GetLen());	
	}

	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	if (pLevel->m_pUnitPosProxMap) { // notify all other units within detection range of the unit's current position
		GAMECLIENT* SendToClients[MAX_CHARACTERS_PER_GAME_SERVER];
		unsigned int iCount = 0;
		float DetectionRange = (LevelGetPVPWorld( pLevel ) ? DetectionRangeShadowLands : DetectionRangeNormal);

		PROXNODE LevelProxNode = pLevel->m_pUnitPosProxMap->Query
			(pUnit->vPosition.x, pUnit->vPosition.y, pUnit->vPosition.z, DetectionRange); 
		for (; LevelProxNode != INVALID_ID; LevelProxNode = pLevel->m_pUnitPosProxMap->GetNext(LevelProxNode)) {

			UNIT* pOtherUnit = UnitGetById(pGame, pLevel->m_pUnitPosProxMap->GetId(LevelProxNode));
			ASSERT_CONTINUE(pOtherUnit);
			ASSERT_CONTINUE(pOtherUnit->pRoom->pLevel == pUnit->pRoom->pLevel);

			GAMECLIENTID ClientId = UnitGetClientId(pOtherUnit);
			if (ClientId != INVALID_GAMECLIENTID && ClientId != idClientExclude)
				if (GAMECLIENT* pGameClient = ClientGetById(pGame, ClientId))
					SendToClients[iCount++] = pGameClient;
		}

		s_SendMessageClientArray(pGame, SendToClients, iCount, SCMD_UNITMOVEXYZ, &msg);
	}
	else // for non-town move messages, use the normal room visibility algorithm
		s_SendUnitMessage(pUnit, SCMD_UNITMOVEXYZ, &msg, idClientExclude);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void PrepareDeferredUnitMoveXYZMsg(UNIT* pUnit, MSG_SCMD_UNITMOVEXYZ* pDeferredUnitMoveMsg)
{
	ASSERT_RETURN(pDeferredUnitMoveMsg);
	pDeferredUnitMoveMsg->id = UnitGetId(pUnit);
	pDeferredUnitMoveMsg->bFlags = 0; // not currently stored
	pDeferredUnitMoveMsg->fVelocity = UnitGetVelocity(pUnit);
	pDeferredUnitMoveMsg->fStopDistance = UnitGetStatFloat(pUnit, STATS_STOP_DISTANCE, 0);
	pDeferredUnitMoveMsg->mode = static_cast<UNITMODE>(UnitGetStat(pUnit, STATS_AI_MODE)); // last mode set
	pDeferredUnitMoveMsg->TargetPosition = pUnit->vMoveTarget; // we don't want to resolve target id here
	pDeferredUnitMoveMsg->MoveDirection  = UnitGetMoveDirection(pUnit);
	pDeferredUnitMoveMsg->idFaceTarget = INVALID_ID; // not currently stored
	int nBufferLength = AICopyPathToMessageBuffer(pUnit->pGame, pUnit, pDeferredUnitMoveMsg->buf);
	MSG_SET_BLOB_LEN(*pDeferredUnitMoveMsg, buf, nBufferLength);
}


static int CompareProxNodeFxn(const void* pOp1, const void* pOp2)
{ 
	DBG_ASSERT(pOp1 && pOp2); 
	return *static_cast<const PROXNODE*>(pOp1) < *static_cast<const PROXNODE*>(pOp2) ? -1 
		: *static_cast<const PROXNODE*>(pOp1) == *static_cast<const PROXNODE*>(pOp2) ? 0 : 1; 
}


// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
void s_SendDeferredMoveMsgsToProximity(UNIT* pUnit)
{
	ASSERT_RETURN(pUnit)
	if (!pUnit->pRoom || !pUnit->pRoom->pLevel->m_pUnitPosProxMap || pUnit->LevelProxMapNode == INVALID_ID)
		return;
	ASSERT_RETURN(pUnit->LevelProxMapNode != INVALID_ID);

	// throttle level prox map updates by updating only if the unit has moved a sufficient distance
	float x, y, z;
	PointMap& LevelProxMap = *pUnit->pRoom->pLevel->m_pUnitPosProxMap;
	ASSERT(LevelProxMap.GetPosition(pUnit->LevelProxMapNode, x, y, z));
	if (VectorDistanceSquaredXY(VECTOR(x, y, z), pUnit->vPosition) < MoveThresholdSq)
		return;

	SIMPLE_DYNAMIC_ARRAY<PROXNODE> OldProxNodes, NewProxNodes, NotifyProxNodes;
	float DetectionRange = ( LevelGetPVPWorld( pUnit->pRoom->pLevel ) ? DetectionRangeShadowLands : DetectionRangeNormal);

	// get sorted vector of nodes in detection range of old position
	ArrayInit(OldProxNodes, GameGetMemPool(UnitGetGame(pUnit)), 32);
	PROXNODE LevelProxNode = LevelProxMap.Query(pUnit->LevelProxMapNode, DetectionRange);
	for (; LevelProxNode != INVALID_ID; LevelProxNode = LevelProxMap.GetNext(LevelProxNode))
		ArrayAddItem(OldProxNodes, LevelProxNode);	
	OldProxNodes.QSort(&CompareProxNodeFxn);

	LevelProxMap.Move(pUnit->LevelProxMapNode, pUnit->vPosition.x, pUnit->vPosition.y, pUnit->vPosition.z);

	// get sorted vector of nodes in detection range of new position
	ArrayInit(NewProxNodes, GameGetMemPool(UnitGetGame(pUnit)), 32);
	LevelProxNode = LevelProxMap.Query(pUnit->LevelProxMapNode, DetectionRange);
	for (; LevelProxNode != INVALID_ID; LevelProxNode = LevelProxMap.GetNext(LevelProxNode))
		ArrayAddItem(NewProxNodes, LevelProxNode);	
	NewProxNodes.QSort(&CompareProxNodeFxn);
		
	// create a list of nodes that are now in detection range, but previously weren't (rewriting set_difference)
	ArrayInit(NotifyProxNodes, GameGetMemPool(UnitGetGame(pUnit)), max(OldProxNodes.Count(), NewProxNodes.Count()));
	for (unsigned uNew = 0, uOld = 0; uNew < NewProxNodes.Count(); ++uNew) {
		while (uOld < OldProxNodes.Count() && OldProxNodes[uOld] < NewProxNodes[uNew])
			++uOld;
		if (uOld >= OldProxNodes.Count() || OldProxNodes[uOld] > NewProxNodes[uNew])
			ArrayAddItem(NotifyProxNodes, NewProxNodes[uNew]);	
	}

	GAMECLIENT* pClient = pUnit->idClient != INVALID_GAMECLIENTID ? ClientGetById(pUnit->pGame, pUnit->idClient) : 0;
	BOOL bUnitCanSendMessages = UnitCanSendMessages(pUnit);
	GAMECLIENT* OtherClients[MAX_CHARACTERS_PER_GAME_SERVER];
	int iOtherClientCount = 0;

	// iterate over each newly in range node, notifying both clients 
	for (unsigned u = 0, uEnd = NotifyProxNodes.Count(); u < uEnd; ++u) {
		UNIT* pOtherUnit = UnitGetById(pUnit->pGame, LevelProxMap.GetId(NotifyProxNodes[u]));
		ASSERT_CONTINUE(pOtherUnit);
		ASSERT_CONTINUE(pOtherUnit->pRoom->pLevel == pUnit->pRoom->pLevel);

		// notify this client of the newly detected other unit
		if (pClient && UnitCanSendMessages(pOtherUnit) && ClientCanKnowUnit(pClient, pOtherUnit)) { 
			MSG_SCMD_UNITMOVEXYZ DeferredOtherUnitMoveMsg;
			PrepareDeferredUnitMoveXYZMsg(pOtherUnit, &DeferredOtherUnitMoveMsg);
			SendMessageToClient(pUnit->pGame, pClient, SCMD_UNITMOVEXYZ, &DeferredOtherUnitMoveMsg);
		}

		// if the other unit has a game client, add it to the list to be notified
		if (bUnitCanSendMessages && pOtherUnit->idClient != INVALID_GAMECLIENTID)
			if (GAMECLIENT* pOtherClient = ClientGetById(pOtherUnit->pGame, pOtherUnit->idClient))
				if (ClientCanKnowUnit(pOtherClient, pUnit))
					OtherClients[iOtherClientCount++] = pOtherClient;
	} // for (unsigned u = 0, uEnd = NotifyProxNodes.Count(); u < uEnd; ++u)

	if (iOtherClientCount > 0) { // notify other clients of this newly detected unit
		MSG_SCMD_UNITMOVEXYZ DeferredUnitMoveMsg;
		PrepareDeferredUnitMoveXYZMsg(pUnit, &DeferredUnitMoveMsg);
		s_SendMessageClientArray(pUnit->pGame, OtherClients, iOtherClientCount, SCMD_UNITMOVEXYZ, &DeferredUnitMoveMsg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_SendUnitMoveXYZ(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vTargetPosition,
	const VECTOR &vMoveDirection,
	BOOL bSendPath)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_MOVE_XYZ", &vTargetPosition );
#endif

	MSG_CCMD_UNITMOVEXYZ msg;

	msg.bFlags = bFlags;
	msg.mode = eMode;
	msg.fVelocity = UnitGetVelocity( pUnit );
	msg.TargetPosition = vTargetPosition;
	msg.MoveDirection  = vMoveDirection;
	
	if (bSendPath)
	{
		int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf);
		MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
	}
	else
	{
		BIT_BUF tBuf(msg.buf, MAX_PATH_BUFFER);
		PathStructSerialize(pGame, NULL, tBuf);
		MSG_SET_BLOB_LEN(msg, buf, tBuf.GetLen());	
	}
	
	c_SendMessage( CCMD_UNITMOVEXYZ, &msg);
	
}		
#endif	//	!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_SendUnitSetFaceDirection(
	UNIT * unit,
	const VECTOR &vFaceDirection)
{
	ASSERTX_RETURN(unit, "Expected unit");
	
	if (UnitCanSendMessages(unit) == FALSE)
	{
		return;
	}

	MSG_CCMD_UNITSETFACEDIRECTION msg;
	msg.FaceDirection  = vFaceDirection;

	c_SendMessage(CCMD_UNITSETFACEDIRECTION, &msg);
}
#endif	//	!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitMoveID(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	UNITID idTarget,
	const VECTOR &vDirection)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );	
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}	

#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_MOVE_ID", &UnitGetPosition( UnitGetById( pGame, idTarget ) ) );
#endif

	MSG_SCMD_UNITMOVEID msg;
	msg.id = UnitGetId( pUnit );
	msg.bFlags = bFlags;
	msg.mode = eMode;
	msg.fVelocity = UnitGetVelocity( pUnit );
	msg.fStopDistance = UnitGetStatFloat(pUnit, STATS_STOP_DISTANCE, 0);
	msg.idTarget = idTarget;
	BOOL bEmptyPath = FALSE;
	if(TESTBIT(bFlags, UMID_USE_EXISTING_PATH))
	{
		bEmptyPath = TRUE;
	}
	int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf, bEmptyPath);
	MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
	
	s_SendUnitMessage(pUnit, SCMD_UNITMOVEID, &msg);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitMoveDirection(
	UNIT *pUnit,
	BYTE bFlags,
	UNITMODE eMode,
	const VECTOR &vDirection)
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}
	
#ifdef _DEBUG
	MovementCheckDebugUnit( pUnit, "SEND_MOVE_DIRECTION", &vDirection );
#endif

	MSG_SCMD_UNITMOVEDIRECTION msg;
	msg.id = UnitGetId( pUnit );
	msg.bFlags = bFlags;
	msg.mode = eMode;
	msg.fVelocity = UnitGetVelocity( pUnit );
	msg.fStopDistance = UnitGetStatFloat(pUnit, STATS_STOP_DISTANCE, 0);
	msg.MoveDirection = vDirection;
	int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf);
	MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
	s_SendUnitMessage(pUnit, SCMD_UNITMOVEDIRECTION, &msg);
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitPathChange(
	UNIT *pUnit )
{			
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );	
	
	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}	

	MSG_SCMD_UNITPATHCHANGE msg;
	msg.id = UnitGetId( pUnit );
	int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf);
	MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
	
	s_SendUnitMessage(pUnit, SCMD_UNITPATHCHANGE, &msg);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoPathingCleanup(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * ,
	EVENT_DATA *,
	void * pData,
	DWORD dwId )
{
	DWORD dwUnitFreeFlags = CAST_PTR_TO_INT(pData);
	if ( dwUnitFreeFlags & UFF_ROOM_BEING_FREED )
		return;
	if(pUnit->m_pPathing && !UnitTestFlag(pUnit, UNITFLAG_DESTROY_DEAD_NEVER))
	{
		UnitClearBothPathNodes( pUnit );
		PathStructFree(pGame, pUnit->m_pPathing);
		pUnit->m_pPathing = NULL;
	}
	UnitUnregisterEventHandler(pGame, pUnit, EVENT_UNITDIE_BEGIN, sDoPathingCleanup);
	UnitUnregisterEventHandler(pGame, pUnit, EVENT_ON_FREED, sDoPathingCleanup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitPathingInit(
	UNIT *unit)
{		
	GAME *game = UnitGetGame(unit);
	UnitSetFlag(unit, UNITFLAG_PATHING);
	unit->m_pPathing = PathStructInit(game);
	UnitRegisterEventHandler(game, unit, EVENT_UNITDIE_BEGIN, sDoPathingCleanup, NULL);
	UnitRegisterEventHandler(game, unit, EVENT_ON_FREED, sDoPathingCleanup, NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStopPathing( UNIT * unit )
{
	UnitClearBothPathNodes(unit);
	UnitPathAbort(unit);
	VECTOR vNewPosition = UnitGetPosition(unit);
	UnitCalculateTargetReached(UnitGetGame(unit), unit, vNewPosition, vNewPosition, FALSE, TRUE, TRUE);
	if(!UnitOnStepList(UnitGetGame(unit), unit))
	{
		UnitReservePathOccupied(unit);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_FIRST_STEP_DESYNC 13.0f
#define MAX_FIRST_STEP_DESYNC_SQUARED MAX_FIRST_STEP_DESYNC * MAX_FIRST_STEP_DESYNC
#define MAX_FIRST_STEP_DESYNC_MYTHOS 20.0f
#define MAX_FIRST_STEP_DESYNC_SQUARED_MYTHOS MAX_FIRST_STEP_DESYNC_MYTHOS * MAX_FIRST_STEP_DESYNC_MYTHOS

BOOL s_UnitTestPathSync(GAME *game, UNIT *unit)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);
	if(!unit->m_pPathing) return FALSE;
	if(unit->m_pPathing->m_nPathNodeCount < 1) return TRUE;
	ASTAR_OUTPUT firstNode = unit->m_pPathing->m_pPathNodes[0];
	VECTOR vFirstPosition = AStarGetWorldPosition(
		game,
		firstNode.nRoomId,
		firstNode.nPathNodeIndex );

	float fDistanceSquared = VectorLengthSquared(UnitGetPosition(unit)- vFirstPosition);

	if( (AppIsHellgate() && fDistanceSquared > MAX_FIRST_STEP_DESYNC_SQUARED ) ||
		(AppIsTugboat() && fDistanceSquared > MAX_FIRST_STEP_DESYNC_SQUARED_MYTHOS ) )
	{
		TraceError("Unit %ls's path has excessive distancesq %f on the first step, warping to start position.",
			(unit->szName?unit->szName:L""),
			fDistanceSquared);

		PlayerWarpToStartLocation(game, unit);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
