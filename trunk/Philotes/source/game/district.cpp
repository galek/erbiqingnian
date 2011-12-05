//----------------------------------------------------------------------------
// FILE: district.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "district.h"
#include "game.h"
#include "level.h"
#include "room.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct DISTRICT_ROOM
{
	ROOM* pRoom;
	float flArea;
	DISTRICT_ROOM *pNext;
	DISTRICT_ROOM *pPrev;
};

//----------------------------------------------------------------------------
struct DISTRICT_ROOM_LIST
{
	DISTRICT_ROOM *pHead;
	DISTRICT_ROOM *pTail;
	int nCount;
};

//----------------------------------------------------------------------------
struct DISTRICT
{
	LEVEL* pLevel;			// level district is in
	int nSpawnClass;		// what spawns in this district
	float flArea;			// area district covers
	ROOM* pRoomList;		// head of rooms list in district
	int nRoomCount;			// # of rooms on room list	
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDistrictRoomListInit(
	DISTRICT_ROOM_LIST* pList)
{
	ASSERTX_RETURN( pList, "Expected list" );
	pList->pHead = NULL;
	pList->pTail = NULL;
	pList->nCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppendToList(
	GAME* pGame,
	DISTRICT_ROOM_LIST* pList,
	ROOM* pRoom)
{
	ASSERTX_RETURN( pList, "Expected list" );
	ASSERTX_RETURN( pRoom, "Expected room" );

	// create node
	DISTRICT_ROOM* pToAdd = (DISTRICT_ROOM*)GMALLOCZ( pGame, sizeof( DISTRICT_ROOM ) );
	pToAdd->pRoom = pRoom;
	pToAdd->flArea = RoomGetArea( pRoom );

	pToAdd->pPrev = NULL;
	pToAdd->pNext = NULL;
	
	if (pList->pHead == NULL)
	{
		// first on list, add as head
		pList->pHead = pToAdd;
		ASSERT( pList->pTail == NULL );
	}
	else
	{
		// connect existing tail to us and us to the tail
		ASSERT( pList->pTail != NULL );
		ASSERT( pList->pTail->pNext == NULL );
		pList->pTail->pNext = pToAdd;
		pToAdd->pPrev = pList->pTail;
	}
	
	// we are now the tail
	pList->pTail = pToAdd;
	pList->nCount++;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveFromList(
	DISTRICT_ROOM_LIST* pList,
	DISTRICT_ROOM* pToRemove)
{
	ASSERTX_RETURN( pList, "Expected list" );
	ASSERTX_RETURN( pToRemove, "Expected node to remove" );

	if (pToRemove->pPrev)
	{
		pToRemove->pPrev->pNext = pToRemove->pNext;
	}
	else
	{
		// we were the head
		pList->pHead = pToRemove->pNext;
	}
	
	if (pToRemove->pNext)
	{
		pToRemove->pNext->pPrev = pToRemove->pPrev;
	}
	else
	{
		// we were the tail
		pList->pTail = pToRemove->pPrev;
	}
	
	pToRemove->pPrev = NULL;
	pToRemove->pNext = NULL;
	pList->nCount--;	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DISTRICT_ROOM_LIST sGetAvailableDistrictRooms(
	GAME* pGame,
	LEVEL* pLevel)
{
	DISTRICT_ROOM_LIST tList;
	sDistrictRoomListInit( &tList );
	
	// go through all rooms
	for (ROOM* pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
	
		// only consider rooms that are not part of districts already
		if (RoomGetDistrict( pRoom ) == NULL)
		{
								
			// add to list
			sAppendToList( pGame, &tList, pRoom );
				
		}
						
	}
	
	return tList;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreeDistrictRoomList(
	GAME* pGame,
	DISTRICT_ROOM_LIST* pList)
{
	ASSERTX_RETURN( pList, "Expected list" );
	
	while (pList->pHead)
	{
		DISTRICT_ROOM* pDistrictRoom = pList->pHead;
		sRemoveFromList( pList, pDistrictRoom );
		GFREE( pGame, pDistrictRoom );
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDistrictInit( 
	DISTRICT* pDistrict,
	LEVEL* pLevel)
{
	ASSERTX_RETURN( pDistrict, "Expected district" );
	
	pDistrict->pLevel = pLevel;
	pDistrict->nSpawnClass = INVALID_LINK;
	pDistrict->flArea = 0.0f;
	pDistrict->pRoomList = NULL;
	pDistrict->nRoomCount = 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSelectSpawnClass(
	DISTRICT* pDistrict, 
	LEVEL* pLevel)
{
	ASSERTX_RETURN( pDistrict, "Expected district" );
	ASSERTX_RETURN( pLevel, "Expected level" );	
	
	int nNumDistricts = s_LevelGetNumDistricts( pLevel );
	ASSERTX_RETURN( nNumDistricts >= 0 && nNumDistricts < MAX_LEVEL_DISTRICTS, "Level already full of max districts" );
	
	// use the spawn class at this index (note that nNumDistricts will be +1 next time
	// we create a district so it will grab the next spawn class)
	int nSpawnClass = pLevel->m_nDistrictSpawnClass[ nNumDistricts ];
	
	ASSERTX_RETURN( nSpawnClass != INVALID_LINK, "Invalid spawn class specified for district in level" );
	pDistrict->nSpawnClass = nSpawnClass;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM* sSelectDistrictRoot(
	GAME* pGame,
	LEVEL* pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected Level" );

	// gather all the rooms in this level that are not part of a district already
	DISTRICT_ROOM_LIST tAvailableRooms = sGetAvailableDistrictRooms( pGame, pLevel );
	if (tAvailableRooms.nCount == 0)
	{
		return NULL;
	}
	
	// for now, just select one of the rooms at random, with a growing algorithm, it might
	// be better for us to pick a room that is somewhere along the "edge" of the level,
	// that would let keep any area from going in the middle and possibly fracturing
	// other districts
	int nPick = RandGetNum( pGame, 0, tAvailableRooms.nCount - 1 );
	DISTRICT_ROOM* pDistrictRoom = tAvailableRooms.pHead;
	for (int i = 0; i < nPick; ++i)
	{
		pDistrictRoom = pDistrictRoom->pNext;	
	}
	ROOM* pRootRoom = pDistrictRoom->pRoom;
	
	// free district rooms		
	sFreeDistrictRoomList( pGame, &tAvailableRooms );

	// return room selected	
	return pRootRoom;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGrowDistrict( 
	GAME* pGame,
	ROOM *pRoot,
	DISTRICT* pDistrict,
	float flDesiredArea)
{
	DISTRICT_ROOM_LIST tList;
	sDistrictRoomListInit( &tList );

	// add root to list
	sAppendToList( pGame, &tList, pRoot );

	// process list	
	DISTRICT_ROOM* pCurrent = tList.pHead;
	while (pCurrent)
	{
	
		// get list room
		ROOM* pRoom = pCurrent->pRoom;

		// if district is full - stop
		if (pDistrict->flArea >= flDesiredArea)
		{
			break;
		}

		// only process this room if it's not in a district already (this happens
		// becuase our room links are bi-directional)
		if (RoomGetDistrict( pRoom ) != NULL)
		{
			
			// it should only be in this district tho
			ASSERTX( RoomGetDistrict( pRoom ) == pDistrict, "Room should be in district we're building" );
			
		}
		else
		{
		
			// add this room to the district
			DistrictAddRoom( pDistrict, pRoom );
			
			// add this rooms connections to the end of the list for processing (breadth first)				
			for (int i = 0; i < pRoom->nConnectionCount; ++i)
			{
				ROOM_CONNECTION* pRoomConnection = &pRoom->pConnections[ i ];
				ROOM* pConnectedRoom = pRoomConnection->pRoom;
				
				// only consider rooms that are not already part of any district
				if (pConnectedRoom != NULL && RoomGetDistrict( pConnectedRoom ) == NULL)
				{
					sAppendToList( pGame, &tList, pConnectedRoom );
				}
				
			}

		}
		
		// go to next node
		pCurrent = pCurrent->pNext;
				
	}

	// free the list we've build thus far
	sFreeDistrictRoomList( pGame, &tList );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DistrictGrow( 
	GAME* pGame, 
	DISTRICT* pDistrict,
	float flDistrictArea)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pDistrict, "Expected district" );
	ASSERTX_RETURN( flDistrictArea > 0.0f, "Invalid district area" );	
	
	// keep running this until we have chosen enough rooms to fill the desired
	// district area ... if we're going through this loop more than once, it could
	// possibly fracture the district so that it's not all lumped together in a blob,
	// which might be bad, but might be ok ... we'll do this another way if it feels wierd.
	LEVEL* pLevel = DistrictGetLevel( pDistrict );
	while (pDistrict->flArea < flDistrictArea)
	{
						
		// select start room for this district
		ROOM* pRootRoom = sSelectDistrictRoot( pGame, pLevel );
		if (pRootRoom == NULL)
		{
			break;  // no rooms left to pick from
		}

		// do a breadth first search from the room room
		sGrowDistrict( pGame, pRootRoom, pDistrict, flDistrictArea );
		
	}

	// if district has no rooms, issue a warning
	ASSERTX( pDistrict->nRoomCount > 0, "District has no rooms, you may safely continue but this is something we should fix." );
	
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DISTRICT* DistrictCreate( 
	GAME* pGame, 
	LEVEL* pLevel)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( pLevel, "Expected level" );

	// allocate new district and inti
	DISTRICT* pDistrict = (DISTRICT*)GMALLOCZ( pGame, sizeof( DISTRICT ) );
	sDistrictInit( pDistrict, pLevel );
	
	// setup spawn class to use
	sSelectSpawnClass( pDistrict, pLevel );
		
	return pDistrict;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DistrictFree(
	GAME* pGame,
	DISTRICT* pDistrict)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pDistrict, "Expected district" );	
	
	GFREE( pGame, pDistrict );		
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DistrictAddRoom( 
	DISTRICT* pDistrict, 
	ROOM* pRoom)
{
	ASSERTX_RETURN( pDistrict, "Expected district" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX( RoomGetDistrict( pRoom ) == NULL, "Room is already in district" );
			
	// add to room list
	pRoom->pNextRoomInDistrict = pDistrict->pRoomList;
	pDistrict->pRoomList = pRoom;
	pDistrict->nRoomCount++;
	
	// add area to district ... we fudge the area a little bit to make sure that
	// floating inprecision doesn't mess us up ... if there are any rooms left
	// over after the districting process, they will be put into a default district anyway
	pDistrict->flArea += RoomGetArea( pRoom ) + 1.0f;

	// set district in room
	RoomSetDistrict( pRoom, pDistrict );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DistrictGetSpawnClass(
	DISTRICT* pDistrict)
{
	ASSERTX_RETVAL( pDistrict, INVALID_LINK, "Expected district" );
	return pDistrict->nSpawnClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM* DistrictGetFirstRoom(
	DISTRICT* pDistrict)
{
	ASSERTX_RETNULL( pDistrict, "Expected district" );
	return pDistrict->pRoomList;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM* DistrictGetNextRoom(
	ROOM* pRoom)
{
	if (pRoom)
	{
		return pRoom->pNextRoomInDistrict;
	}
	return NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL* DistrictGetLevel(
	DISTRICT* pDistrict)
{
	ASSERTX_RETNULL( pDistrict, "Expected district" );
	return pDistrict->pLevel;
}
