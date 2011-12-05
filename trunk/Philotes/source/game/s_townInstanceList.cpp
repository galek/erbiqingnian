//----------------------------------------------------------------------------
// s_townInstanceList.cpp
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "svrstd.h"
#include "game.h"
#if !ISVERSION(CLIENT_ONLY_VERSION)
#include "s_townInstanceList.h"
#include "hlocks.h"
#if ISVERSION(SERVER_VERSION)
#include "s_townInstanceList.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
DEF_HLOCK(TownInstanceListLock, HLReaderWriter)
END_HLOCK

//----------------------------------------------------------------------------
struct MAP_BY_INSTANCE_KEY
{
	UINT64 m_key;

	MAP_BY_INSTANCE_KEY(
		int idLevelDef,
		int instanceNumber)
	{
		m_key = (((UINT64)idLevelDef << 32) | (UINT64)instanceNumber);
	}

	bool operator < (
		const MAP_BY_INSTANCE_KEY & other) const
	{
		return m_key < other.m_key;
	}
};

//----------------------------------------------------------------------------
typedef map_mp<
			MAP_BY_INSTANCE_KEY,
			GAME_INSTANCE_INFO>::type MAP_BY_INSTANCE;

//----------------------------------------------------------------------------
struct MAP_BY_GAME_KEY
{
	int m_idLevelDefinition;
	GAMEID m_idGame;

	MAP_BY_GAME_KEY(
		int idLevelDef,
		GAMEID idHostingGameId) :
		m_idLevelDefinition(idLevelDef),
		m_idGame(idHostingGameId)
	{ }
	
	bool operator < (
		const MAP_BY_GAME_KEY & other ) const
	{
		if (m_idLevelDefinition == other.m_idLevelDefinition)
			return m_idGame < other.m_idGame;
		return m_idLevelDefinition < other.m_idLevelDefinition;
	}
};

//----------------------------------------------------------------------------
typedef map_mp<
			MAP_BY_GAME_KEY,
			GAME_INSTANCE_INFO>::type MAP_BY_GAME;

//----------------------------------------------------------------------------
struct TOWN_INSTANCE_LIST
{
	TownInstanceListLock m_lock;
	MAP_BY_INSTANCE		 m_mapByInstanceNumber;
	MAP_BY_GAME			 m_mapByHostingGame;
	MEMORYPOOL *		 m_pool;
	UINT64				 m_nLastUpdateTime;
};


//----------------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TOWN_INSTANCE_LIST * TownInstanceListCreate(
	struct MEMORYPOOL * pool )
{
	TOWN_INSTANCE_LIST * toRet = (TOWN_INSTANCE_LIST*)MALLOC(pool, sizeof(TOWN_INSTANCE_LIST));
	ASSERT_RETNULL(toRet);

	toRet->m_pool = pool;
	toRet->m_lock.Init();
	new (&toRet->m_mapByInstanceNumber) MAP_BY_INSTANCE(MAP_BY_INSTANCE::key_compare(), pool);
	new (&toRet->m_mapByHostingGame) MAP_BY_GAME(MAP_BY_GAME::key_compare(), pool);
	toRet->m_nLastUpdateTime = 0;
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TownInstanceListDestroy(
	TOWN_INSTANCE_LIST * list )
{
	if (!list)
		return;

	list->m_lock.Free();
	list->m_mapByInstanceNumber.~MAP_BY_INSTANCE();
	list->m_mapByHostingGame.~MAP_BY_GAME();
	FREE(list->m_pool, list);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TownInstanceListClearList(
	TOWN_INSTANCE_LIST * list )
{
	ASSERT_RETURN(list);

	HLRWWriteLock lock(&list->m_lock);

	list->m_mapByInstanceNumber.clear();
	list->m_mapByHostingGame.clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownInstanceListGetInstanceInfo(
	TOWN_INSTANCE_LIST * list,
	int idLevelDef,
	int instanceNumber,
	GAME_INSTANCE_INFO & outInstanceInfo )
{
	ASSERT_RETFALSE(list);
	ASSERT_RETFALSE(idLevelDef != INVALID_ID);
	ASSERT_RETFALSE(instanceNumber != INVALID_ID);

	outInstanceInfo.idGame = INVALID_GAMEID;
	outInstanceInfo.nLevelDefinition = INVALID_ID;
	outInstanceInfo.nInstanceNumber = INVALID_ID;
	outInstanceInfo.nInstanceType = GAME_INSTANCE_NONE;
	outInstanceInfo.nPVPWorld = INVALID_ID;

	HLRWReadLock readLock(&list->m_lock);

	MAP_BY_INSTANCE::const_iterator itr = list->m_mapByInstanceNumber.find(MAP_BY_INSTANCE_KEY(idLevelDef,instanceNumber));
	if (itr == list->m_mapByInstanceNumber.end())
		return FALSE;

	outInstanceInfo.idGame = itr->second.idGame;
	outInstanceInfo.nLevelDefinition = itr->second.nLevelDefinition;
	outInstanceInfo.nInstanceNumber = itr->second.nInstanceNumber;
	outInstanceInfo.nInstanceType = itr->second.nInstanceType;
	outInstanceInfo.nPVPWorld = itr->second.nPVPWorld;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownInstanceListGetInstanceInfo(
	TOWN_INSTANCE_LIST * list,
	int idLevelDef,
	GAMEID idHostingGame,
	GAME_INSTANCE_INFO & outInstanceInfo )
{
	ASSERT_RETFALSE(list);
	ASSERT_RETFALSE(idLevelDef != INVALID_ID);
	ASSERT_RETFALSE(idHostingGame != INVALID_GAMEID);

	outInstanceInfo.idGame = INVALID_GAMEID;
	outInstanceInfo.nLevelDefinition = INVALID_ID;
	outInstanceInfo.nInstanceNumber = INVALID_ID;
	outInstanceInfo.nInstanceType = GAME_INSTANCE_NONE;
	outInstanceInfo.nPVPWorld = INVALID_ID;

	HLRWReadLock readLock(&list->m_lock);

	MAP_BY_GAME::const_iterator itr = list->m_mapByHostingGame.find(MAP_BY_GAME_KEY(idLevelDef, idHostingGame));
	if (itr == list->m_mapByHostingGame.end())
		return FALSE;

	outInstanceInfo.idGame = itr->second.idGame;
	outInstanceInfo.nLevelDefinition = itr->second.nLevelDefinition;
	outInstanceInfo.nInstanceNumber = itr->second.nInstanceNumber;
	outInstanceInfo.nInstanceType = itr->second.nInstanceType;
	outInstanceInfo.nPVPWorld = itr->second.nPVPWorld;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownInstanceListGetAllInstances(
	TOWN_INSTANCE_LIST * list,
	GAME_INSTANCE_INFO_VECTOR & dest )
{
	ASSERT_RETFALSE(list);

	HLRWReadLock readLock(&list->m_lock);

	if (!list->m_mapByInstanceNumber.empty())
		dest.reserve(list->m_mapByInstanceNumber.size());

	MAP_BY_INSTANCE::const_iterator itr = list->m_mapByInstanceNumber.begin();
	while (itr != list->m_mapByInstanceNumber.end())
	{
		dest.push_back(itr->second);
		itr++;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 TownInstanceListGetLastUpdateTime(
	TOWN_INSTANCE_LIST * list )
{
	ASSERT_RETFALSE(list);

	HLRWReadLock readLock(&list->m_lock);
	return list->m_nLastUpdateTime;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownInstanceListAddInstance(
	TOWN_INSTANCE_LIST * list,
	int idLevelDef,
	GAMEID idGame,
	int instanceNumber,
	int nInstanceType,
	int nPVPWorld )
{
	ASSERT_RETFALSE(list);
	ASSERT_RETFALSE(idLevelDef != INVALID_ID);
	ASSERT_RETFALSE(idGame != INVALID_GAMEID);
	ASSERT_RETFALSE(instanceNumber != INVALID_SVRID);

	TraceInfo(
		TRACE_FLAG_GAME_MISC_LOG,
		"Received town instance for ADD. Area Id: %d, Game Id: %I64x, Instance Number: %u",
		idLevelDef, idGame, instanceNumber);

	HLRWWriteLock writeLock(&list->m_lock);

	list->m_nLastUpdateTime = PGetPerformanceCounter();

	GAME_INSTANCE_INFO info;
	info.idGame = idGame;
	info.nInstanceNumber = instanceNumber;
	info.nLevelDefinition = idLevelDef;
	info.nInstanceType = nInstanceType;
	info.nPVPWorld = nPVPWorld;


	MAP_BY_INSTANCE::_Pairib instInsertPair = list->m_mapByInstanceNumber.insert(std::make_pair(MAP_BY_INSTANCE_KEY(idLevelDef, instanceNumber), info));
	ASSERT_RETFALSE(instInsertPair.second);

	MAP_BY_GAME::_Pairib gameInsertPair = list->m_mapByHostingGame.insert(std::make_pair(MAP_BY_GAME_KEY(idLevelDef, idGame), info));
	ASSERT_DO(gameInsertPair.second)
	{
		list->m_mapByInstanceNumber.erase(instInsertPair.first);
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownInstanceListRemoveInstance(
	TOWN_INSTANCE_LIST * list,
	int idLevelDef,
	GAMEID idGame,
	int instanceNumber,
	int nPVPWorld )
{
	ASSERT_RETFALSE(list);
	ASSERT_RETFALSE(idLevelDef != INVALID_ID);
	ASSERT_RETFALSE(idGame != INVALID_GAMEID);
	ASSERT_RETFALSE(instanceNumber != INVALID_SVRID);
	REF( nPVPWorld );

	TraceInfo(
		TRACE_FLAG_GAME_MISC_LOG,
		"Received town instance for REMOVE. Area Id: %d, Game Id: %I64x, Instance Number: %u",
		idLevelDef, idGame, instanceNumber);

	HLRWWriteLock writeLock(&list->m_lock);
	list->m_nLastUpdateTime = PGetPerformanceCounter();

	BOOL removeByInstance = (list->m_mapByInstanceNumber.erase(MAP_BY_INSTANCE_KEY(idLevelDef, instanceNumber)) == 1);
	BOOL removeByGame = (list->m_mapByHostingGame.erase(MAP_BY_GAME_KEY(idLevelDef, idGame)) == 1);

	return (removeByInstance && removeByGame);
}


#endif	//	!ISVERSION(CLIENT_ONLY_VERSION)
