//----------------------------------------------------------------------------
// FILE: s_adventure.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "excel.h"
#include "level.h"
#include "objects.h"
#include "picker.h"
#include "quests.h"
#include "s_adventure.h"
#include "stats.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum ADVENTURE_CONSTANTS
{
	MAX_ADVENTURE_TYPES = 256,
};

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sQuestAllowed(
	int nQuest,
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( nQuest != INVALID_LINK, "Expected quest" );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	const DRLG_DEFINITION *ptDRLGDef = LevelGetDRLGDefinition( pLevel );
	ASSERTX_RETFALSE( ptDRLGDef, "Expected DRLG Defintiion" );
	
	// get quest definition
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
	if ( !ptQuestDef )
		return FALSE;
	
	// check any drlg restrictions
	BOOL bDRLGAllowed = TRUE;	
	for (int i = 0; i < MAX_QUEST_ALLOWED_DRLG; ++i)
	{
		int nDRLG = ptQuestDef->nDRLGStyleAllowed[ i ];
		if (nDRLG != INVALID_LINK)
		{
		
			// no no match
			bDRLGAllowed = FALSE;
			
			// unless this one is a match
			if (nDRLG == ptDRLGDef->nStyle)
			{
				bDRLGAllowed = TRUE;
				break;  // found a match, don't search anymore
			}
			
		}
				
	}
	if (bDRLGAllowed == FALSE)
	{
		return FALSE;  // DRLG is restricted
	}
	
	return TRUE;  // ok to use
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sAdventureGetAvailable(
	LEVEL *pLevel,
	int *pnBuffer,
	int nBufferSize)
{
	int nCount = 0;
	
	// go through quest table
	int nNumQuests = ExcelGetNumRows( NULL, DATATABLE_QUEST );
	for (int nQuest = 0; nQuest < nNumQuests; ++nQuest)
	{
		const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
		if (!ptQuestDef)
			continue;
		
		// only consider adventure style quests
		if (ptQuestDef->eStyle == QS_ADVENTURE)
		{

			// must be OK for this level
			if (sQuestAllowed( nQuest, pLevel ))
			{
						
				// add to array
				ASSERTX_BREAK( nCount < nBufferSize, "Buffer too small" );
				pnBuffer[ nCount++ ] = nQuest;

			}
						
		}
				
	}
	
	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsRoomAllowed(	
	GAME *pGame,
	ROOM *pRoom,
	int nQuest)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( nQuest != INVALID_LINK, "Expected quest" );
	
	// room must allow adventures
	if (RoomAllowsAdventures( pGame, pRoom ) == FALSE)
	{
		return FALSE;
	}

	// other more specific tests here one day ... like can't be close to other
	// adventures perhaps???
	// --->
	
	return TRUE; // all good
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM *sPickRoom(
	LEVEL *pLevel,
	int nQuest,
	VECTOR *pvPosition,
	VECTOR *pvNormal)
{
	GAME *pGame = LevelGetGame( pLevel );
	
	// load up an array with all the valid rooms for adventures in this level
	const int MAX_ROOMS = 256;
	ROOM *pRooms[ MAX_ROOMS ];
	int nNumRooms = 0;
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
	
		// must be OK for adventure
		if (sIsRoomAllowed( pGame, pRoom, nQuest ))
		{
		
			// check for not being able to hold any more rooms
			if (nNumRooms == MAX_ROOMS - 1)
			{
				// just remove a random room by putting the last one at another index
				int nPick = RandGetNum( pGame, 0, nNumRooms - 1 );
				ASSERTX_BREAK( nPick >= 0 && nPick < nNumRooms, "Illegal pick" );
				pRooms[ nPick ] = pRooms[ nNumRooms - 1 ];
				nNumRooms = nNumRooms - 1;
			}
			
			// assign new room at end
			pRooms[ nNumRooms++ ] = pRoom;
			
		}
		
	}
	
	// must have rooms
	if (nNumRooms)
	{
		
		// what is the spacial requirements for this quest
		const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
		if (!ptQuestDef)
			return FALSE;
		float flMinNodeHeight = ptQuestDef->flHeight;
		float flRadius = ptQuestDef->flRadius;
		float flZTolerance = ptQuestDef->flFlatZTolerance;
						
		// we have max tries for this
		int nMaxTries = 16;
		while (nMaxTries--)
		{
		
			// select a random room
			int nIndex = RandGetNum( pGame, nNumRooms - 1 );
			ROOM *pRoom = pRooms[ nIndex ];
		
			// find a spot in this room
			BOOL bClear = RoomGetClearFlatArea( 
				pRoom, 
				flMinNodeHeight, 
				flRadius, 
				pvPosition, 
				pvNormal, 
				FALSE, 
				flZTolerance);
				
			if (bClear)
			{
				return pRoom;
			}
									
		}

	}

	return FALSE;  // no room with a suitable location was found
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sPlaceAdventure(
	int nQuest,
	LEVEL *pLevel)
{
	
	// get a location for this
	VECTOR vPosition;
	VECTOR vNormal;
	ROOM *pRoom = sPickRoom( pLevel, nQuest, &vPosition, &vNormal );
	if (pRoom)
	{
		GAME *pGame = LevelGetGame( pLevel );
			
		// if this is cool enough, we might want to load layouts instead of single objects
		// to do that, create a layout.xml, but then to load it into this
		// room you will have to do the following:
		//
		// 1) Create a ROOM_LAYOUT_GROUP_DEFINITION
		// 2) Call RoomLayoutDefinitionPostXMLLoad() on it
		// 3) Load it using RoomLayoutSelectLayoutItems()	
		
		// create the adventure object
		const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
		if (!ptQuestDef)
			return FALSE;
		if (ptQuestDef->nObjectAdventure != INVALID_LINK)
		{
			
			// pick a random orientation
			VECTOR vDirection = RandGetVectorXY( pGame );
			
			// create object
			OBJECT_SPEC tSpec;
			tSpec.nClass = ptQuestDef->nObjectAdventure;
			tSpec.pRoom = pRoom;
			tSpec.vPosition = vPosition;
			tSpec.pvFaceDirection = &vDirection;
			tSpec.pvNormal = &vNormal;
			UNIT *pObject = s_ObjectSpawn( pGame, tSpec );

			// post process any objects we created							
			UnitSetStat( pObject, STATS_ADVENTURE_QUEST, nQuest );
			
			// mark this room as no more adventures
			RoomSetFlag( pRoom, ROOM_NO_ADVENTURES_BIT );
			
		}

						
	}
	
	// not placed
	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_AdventurePopulate(
	LEVEL *pLevel,
	UNIT *pPlayerCreator)
{
	
	// get the available adventure quests for this level
	int nQuests[ MAX_ADVENTURE_TYPES ];
	int nNumQuests = sAdventureGetAvailable( pLevel, nQuests, MAX_ADVENTURE_TYPES );
	if (nNumQuests)
	{
	
		// put all the quests into a picker
		GAME *pGame = LevelGetGame( pLevel );
		PickerStart( pGame, tPicker );
		for (int i = 0; i < nNumQuests; ++i)
		{
		
			// get quest
			int nQuest = nQuests[ i ];
			const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
			if (!ptQuestDef)
				continue;

			// add index to picker			
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, ptQuestDef->nWeight));			
			
		}
			
		// roll how many adventures we will try to place
		int nLevelDef = LevelGetDefinitionIndex( pLevel );
		int nAdventuresToPlace = ExcelEvalScript(
			pGame, 
			NULL, 
			NULL, 
			NULL, 
			DATATABLE_LEVEL, 
			OFFSET(LEVEL_DEFINITION, codeNumAdventures), 
			nLevelDef);
			
		// place all the adventures in this level			
		for (int i = 0; i < nAdventuresToPlace; ++i)
		{
		
			// pick one
			int nIndex = PickerChoose( pGame );
			int nQuest = nQuests[ nIndex ];
			
			// find place it somewhere
			sPlaceAdventure( nQuest, pLevel );
			
		}			

	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_AdventureGetPassageDest( 
	UNIT *pPassage)
{
	ASSERTX_RETINVALID( pPassage, "Expected unit" );

	// what adventure quest is this a part of
	int nQuest = UnitGetStat( pPassage, STATS_ADVENTURE_QUEST );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
	if (!ptQuestDef)
		return INVALID_LINK;

	// count up how many destinations we have
	int nCount = 0;
	for (int i = 0; i < MAX_QUEST_LEVEL_DEST; ++i)
	{
		int nLevelDef = ptQuestDef->nLevelDefDestinations[ i ];
		if (nLevelDef != INVALID_LINK)
		{
			nLevelDef++;
			nCount++;			
		}
	}

	// pick one
	int nLevelDefDest = INVALID_LINK;
	if (nCount > 0)
	{
		GAME *pGame = UnitGetGame( pPassage );
		int nPick = RandGetNum( pGame, 0, nCount - 1 );
		nLevelDefDest = ptQuestDef->nLevelDefDestinations[ nPick ];
	}

	// return the level definition of the destination
	return nLevelDefDest;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_AdventurePassageGetTreasureClass( 
	UNIT *pWarpToDestLevel)
{
	
	if (pWarpToDestLevel)
	{

		// we can only ask this of objects that we spawned in a level that are leading
		// to an adventure, and of that, it really should only be a warp object
		if (ObjectIsWarp( pWarpToDestLevel ))
		{
			
			// get the adventure quest this unit is a part of (if any)
			int nQuest = UnitGetStat( pWarpToDestLevel, STATS_ADVENTURE_QUEST );
			if (nQuest != INVALID_LINK)
			{
			
				// check for a treasure adventure
				const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );
				if ( !ptQuestDef )
					return INVALID_LINK;
				return ptQuestDef->nTreasureClassTreasureRoom;
				
			}
			
		}

	}
		
	return INVALID_LINK;
	
}
