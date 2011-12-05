//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __S_QUESTOBJECTSPAWN_H_
#define __S_QUESTOBJECTSPAWN_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _GAME_H_
#include "game.h"
#endif

#ifndef _LEVEL_H_
#include "level.h"
#endif

#ifndef _ROOM_H_
#include "room.h"
#endif

#ifndef _EXCEL_H_
#include "excel.h"
#endif

#ifndef _UNITS_H_
#include "units.h"
#endif

#ifndef __QUEST_H_
#include "Quest.h"
#endif


namespace TBQuestObjectCreate
{
	//----------------------------------------------------------------------------
	// FORWARD DECLARATIONS
	//----------------------------------------------------------------------------
	//This is an enum for array indexing of object types that are creatable by the Object Spawn
	const enum EQUEST_OBJECTS_CREATABLES
	{
		KQUEST_OBJECT_INTERACT,
		KQUEST_OBJECT_BOSS,
		KQUEST_OBJECT_MINI_BOSS,
		KQUEST_OBJECT_END_LEVEL,
		KQUEST_OBJECT_START_LEVEL,
		KQUEST_OBJECT_COUNT
	};
	//This is an array of unit types that match the EQUEST_OBJECTS_CREATABLES
	static int KQUEST_OBJECT_UNITTYPES_ARRAY[ KQUEST_OBJECT_COUNT ] = { UNITTYPE_QUEST_OBJECT_INTERACT,
																		 UNITTYPE_QUEST_OBJECT_BOSS,
																		 UNITTYPE_QUEST_OBJECT_MINI_BOSS,
																		UNITTYPE_QUEST_OBJECT_INTERACT_LVL_END,
																		UNITTYPE_QUEST_OBJECT_INTERACT_LVL_START};
	#define		MAX_OBJECTS_PER_ARRAY		100
	//----------------------------------------------------------------------------
	// classes
	//----------------------------------------------------------------------------
	class CQuestObjectParams
	{
	public:
		CQuestObjectParams( void )
		{
			ZeroMemory( &m_ObjectsFoundArray, sizeof( UNIT * ) * KQUEST_OBJECT_COUNT * MAX_OBJECTS_PER_ARRAY );
			ZeroMemory( &m_ObjectsCount, sizeof( int ) * KQUEST_OBJECT_COUNT );
		}
		~CQuestObjectParams( void )
		{

		}
		UNIT			*m_ObjectsFoundArray[ KQUEST_OBJECT_COUNT ][ MAX_OBJECTS_PER_ARRAY ];
		int				m_ObjectsCount[ KQUEST_OBJECT_COUNT ];
						
	};

}

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL s_QuestMonsterXPAwardedAlready( UNIT *pMonster,
									 UNIT *pPlayer );

void s_QuestMonsterSetXPAwarded( UNIT *pMonster,
								 UNIT *pPlayer );

UNIT * s_GetQuestObjectInLevel( LEVEL *pLevel,
							    TBQuestObjectCreate::EQUEST_OBJECTS_CREATABLES nType );

void s_SpawnObjectsUsingQuestPoints( int nObjectID,									 
									 LEVEL *pLevel );

void s_SpawnObjectsForQuestOnLevelAdd( GAME *pGame,
									   LEVEL *pLevelCreated,
									   UNIT *pPlayer,									   
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskToForce = NULL);

static ROOM_ITERATE_UNITS sUnitFoundInLevel( UNIT *pUnit,
											 void *pCallbackData);

BOOL s_QuestSpawnItemFromCreatedObject( UNIT *unitSpawning,									    
									    UNIT *pPlayer,
										UNIT *pClientPlayer,
									    BOOL monster = false );


BOOL QuestObjectSpawnIsComplete( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask );


BOOL s_IsTaskObjectCompleted( GAME *pGame,
							 UNIT *playerUnit,
							 const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
							 int ObjectIndex,
							 BOOL Monster,
							 BOOL bIgnoreBossNotNeededToCompleteQuest );

void s_QuestRoomActivatedRespawnFromObjects( ROOM *pRoom );

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------


#endif