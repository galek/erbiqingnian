//----------------------------------------------------------------------------
// s_townInstanceList.h
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _S_TOWNINSTANCELIST_H_
#define _S_TOWNINSTANCELIST_H_

#if !ISVERSION(CLIENT_ONLY_VERSION)

#ifndef _STLALLOCATOR_H_
#include "stlallocator.h"
#endif


//----------------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------------

struct TOWN_INSTANCE_LIST;

struct GAME_INSTANCE_INFO
{
	int    nLevelDefinition;
	int    nInstanceNumber;
	int    nInstanceType;
	int	   nPVPWorld;
	GAMEID idGame;
};

TOWN_INSTANCE_LIST *
	TownInstanceListCreate(
		struct MEMORYPOOL * pool );

void
	TownInstanceListDestroy(
		TOWN_INSTANCE_LIST * list );

void
	TownInstanceListClearList(
		TOWN_INSTANCE_LIST * list );

BOOL
	TownInstanceListGetInstanceInfo(
		TOWN_INSTANCE_LIST * list,
		int idLevelDef,
		int instanceNumber,
		GAME_INSTANCE_INFO & outInstanceInfo );

BOOL
	TownInstanceListGetInstanceInfo(
		TOWN_INSTANCE_LIST * list,
		int idLevelDef,
		GAMEID idHostingGame,
		GAME_INSTANCE_INFO & outInstanceInfo );

typedef vector_mp<GAME_INSTANCE_INFO>::type GAME_INSTANCE_INFO_VECTOR;	//	see stlallocator.h for vector_mp usage

BOOL
	TownInstanceListGetAllInstances(
		TOWN_INSTANCE_LIST * list,
		GAME_INSTANCE_INFO_VECTOR & dest );

BOOL
	TownInstanceListAddInstance(
		TOWN_INSTANCE_LIST * list,
		int idLevelDef,
		GAMEID idGame,
		int instanceNumber,
		int nInstanceType,
		int nPVPWorld );

BOOL
	TownInstanceListRemoveInstance(
		TOWN_INSTANCE_LIST * list,
		int idLevelDef,
		GAMEID idGame,
		int instanceNumber,
		int nPVPWorld );

UINT64 
	TownInstanceListGetLastUpdateTime(
		TOWN_INSTANCE_LIST * list );

#endif	//	!ISVERSION(CLIENT_ONLY_VERSION)
#endif	//	_S_TOWNINSTANCELIST_H_
