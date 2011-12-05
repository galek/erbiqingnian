//----------------------------------------------------------------------------
// FILE: dungeon.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __DUNGEON_H_
#define __DUNGEON_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "level.h"

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct LEVEL;
struct ROOM;
struct GAME;


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define	DUNGEON_ROOM_HASH_SIZE				256
#define MAX_DUNGEON_LEVELS					256


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct DUNGEON_SELECTION
{
	LEVEL_DRLG_CHOICE						m_tDLRGChoice;
	DWORD									m_dwAreaSeed;
	int										m_nAreaDifficulty;
	int										m_nAreaIDForDungeon;
};


//----------------------------------------------------------------------------
// per game structure, contains all levels for a game
//----------------------------------------------------------------------------
struct DUNGEON
{
	DWORD									m_dwSeed;			// starting seed
	RAND									m_randSeed;			// seed
	LEVEL **								m_Levels;
	unsigned int							m_nLevelCount;
	
	DUNGEON_SELECTION						m_DungeonSelections[MAX_DUNGEON_LEVELS];

	ROOM *									m_RoomHash[DUNGEON_ROOM_HASH_SIZE];
	DWORD									m_dwCurrentRoomId;
};


//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------
BOOL DungeonInit(
	GAME * game,
	DWORD dwSeed);

void DungeonFreeAds(
	GAME * game);

void DungeonFree(
	GAME * game);

void DungeonRemoveLevel(
	GAME * game,
	LEVEL * level);

DWORD DungeonGetSeed(
	GAME * game);

DWORD DungeonGetSeed(
	GAME * game,
	int nLevelArea);

void DungeonSetSeed(
	GAME * game,
	DWORD dwSeed);

void DungeonSetSeed(
	GAME * game,
	int nArea,
	DWORD dwSeed);

void DungeonSetDifficulty(
	GAME * game,
	UNIT *pPlayer,
	int nArea,
	int nDifficulty);

int DungeonGetDifficulty(
	GAME * game,
	int nArea,
	UNIT *pPlayer);

LEVELID DungeonFindFreeId(
	GAME * game);

int DungeonGetNumLevels(
	GAME * game);

LEVEL * DungeonGetFirstPopulatedLevel(
	GAME *game);

LEVEL * DungeonGetFirstLevel(
	GAME *game);

LEVEL * DungeonGetNextPopulatedLevel(
	LEVEL *level);

LEVEL * DungeonGetNextLevel(
	LEVEL *level);
	
ROOM * DungeonGetRoomByID(
	DUNGEON * dungeon,
	ROOMID idRoom);

void DungeonRemoveRoom(
	DUNGEON * dungeon,
	ROOM * room);

void DungeonAddRoom(
	DUNGEON * dungeon,
	LEVEL * level,
	ROOM * room);

void DungeonPickDRLGs(
	GAME * game,
	int nDifficulty);

int DungeonGetLevelDRLG(
	GAME * game,
	LEVEL * level);

int DungeonGetLevelDRLG(
	GAME * game,
	int nLevelDefinition,
	BOOL bAllowNonServerGame = FALSE);

const struct LEVEL_DRLG_CHOICE *DungeonGetLevelDRLGChoice(
	int nLevelDef);

const struct LEVEL_DRLG_CHOICE *DungeonGetLevelDRLGChoice(
	LEVEL *pLevel);
	
int DungeonGetLevelSpawnClass(
	GAME * game,
	LEVEL * level);

int DungeonGetLevelSpawnClass(
	GAME * game,
	int nLevelDefinition);

int DungeonGetLevelNamedMonsterClass(
	GAME * game,
	LEVEL * level);

float DungeonGetLevelNamedMonsterChance(
	GAME * game,
	LEVEL * level);

int DungeonGetLevelMusicRef(
	GAME * game,
	LEVEL * level);

int DungeonGetDRLGBaseEnvironment(
	int nDRLGDef );

void DungeonUpdateOnSetLevelMsg( 
	GAME * game, 
	const struct MSG_SCMD_SET_LEVEL *pMsg);


#endif
