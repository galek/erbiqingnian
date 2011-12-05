//----------------------------------------------------------------------------
// FILE: QuestProperties.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "QuestProperties.h"
#include "QuestMessagePump.h"
#include "Quest.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "QuestObjectSpawn.h"
#include "QuestMonsterSpawn.h"
#include "unittag.h"
#include "script.h"
#include "script_priv.h"
#include "unittypes.h"
#include "s_QuestServer.h"
#include "c_QuestClient.h"
#include "s_quests.h"
#include "excel_private.h"
#include "skills.h"
#include "Player.h"
#include "SpawnClass.h"
#include "LevelAreas.h"
#include "offer.h"
#include "PointsOfInterest.h"

using namespace MYTHOS_QUESTS;


inline int GetRandomQuestSeed( UNIT *pPlayer,
							   int nQuestQueueIndex )
{
	ASSERT_RETZERO( pPlayer );	
	if( nQuestQueueIndex < 0 )
		return UnitGetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT );	
	int seed = UnitGetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT_SAVE, nQuestQueueIndex );
	if( seed == 0 )
	{
		seed = UnitGetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT );	
		ASSERTX( seed != 0, "Unable to get random number for quest.Random quest will break." );
	}
	return seed;
}

inline void InitRandomQuestRand( UNIT *pPlayer,
								 RAND &rnd,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	RandInit( rnd, GetRandomQuestSeed( pPlayer, MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask ) ) );	
}

inline void InitRandomQuestRand( UNIT *pPlayer,
								 RAND &rnd,
								 const QUEST_DEFINITION_TUGBOAT *pQuest )
{
	RandInit( rnd, GetRandomQuestSeed( pPlayer, MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuest ) ) );	
}



inline int GetRandomQuestLevel( UNIT *pPlayer,
							   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pQuestTask );	
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask );
	if( nQuestQueueIndex < 0 )
		return UnitGetStat( pPlayer, STATS_LEVEL );
	int nLevel = UnitGetStat( pPlayer, STATS_QUEST_RANDOM_LEVEL_TUGBOAT, nQuestQueueIndex );
	if( nLevel <= 0 ) //we need to set it! We can't base the level off the quest giver.
	{		
		nLevel = UnitGetStat( pPlayer, STATS_LEVEL );
	}
	return nLevel;
}

//THis only returns the random quest that is specified from the parent quest as a template
inline const QUEST_RANDOM_DEFINITION_TUGBOAT * GetRandomQuestDef( const QUEST_DEFINITION_TUGBOAT *pQuest )
{
	ASSERT_RETNULL( pQuest );
	if( pQuest->nQuestRandomID == INVALID_ID )
		return NULL;
	EXCEL_TABLE * tableQuestRandom = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_RANDOM_FOR_TUGBOAT);
	ASSERT( tableQuestRandom );
	return (const QUEST_RANDOM_DEFINITION_TUGBOAT*)ExcelGetData( NULL, DATATABLE_QUEST_RANDOM_FOR_TUGBOAT, pQuest->nQuestRandomID );
}
//THis only returns the random quest that is specified from the parent quest as a template
inline const QUEST_RANDOM_DEFINITION_TUGBOAT * GetRandomQuestDef( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETNULL( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETNULL( pQuest );
	return GetRandomQuestDef( pQuest );
}
//this actually chooses the random task to use.
inline const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT * GetRandomQuestTask( UNIT *pPlayer, const QUEST_DEFINITION_TUGBOAT *pQuest )
{
	ASSERT_RETNULL( pQuest );
	ASSERT_RETNULL( pPlayer );
	const QUEST_RANDOM_DEFINITION_TUGBOAT *pRandomQuest = GetRandomQuestDef( pQuest );
	ASSERT_RETNULL( pRandomQuest );
	RAND rnd;
	InitRandomQuestRand( pPlayer, rnd, pQuest );
	int nRandomTaskIndex = RandGetNum( rnd, 0, pRandomQuest->nRandomTaskCount -1 );
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomTask = pRandomQuest->pFirst;
	ASSERT_RETNULL( pRandomTask );
	while( nRandomTaskIndex > 0 )
	{
		pRandomTask = pRandomTask->pNext;
		ASSERT_RETNULL( pRandomTask );
		nRandomTaskIndex--;
	}
	return pRandomTask;
}

//this actually chooses the random task to use.
inline const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT * GetRandomQuestTask( UNIT *pPlayer, const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETNULL( pQuestTask );	
	return GetRandomQuestTask( pPlayer, QuestDefinitionGet( pQuestTask->nParentQuest ) );
}

//returns the string in question from the random task.
inline int GetRandomQuestString( UNIT *pPlayer, const QUEST_DEFINITION_TUGBOAT *pQuest, EQUEST_STRINGS nStringType )
{
	ASSERT_RETNULL( pQuest );
	ASSERT_RETNULL( pPlayer );
	if( pQuest->nQuestRandomID == INVALID_ID )
		return INVALID_ID;
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuest );	
	ASSERT_RETINVALID( pQuestRandomTask );
	return pQuestRandomTask->nStrings[ nStringType ];
}




inline int GetRandomQuestString( UNIT *pPlayer, 
								const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask, 
								EQUEST_STRINGS nStringType )
{
	ASSERT_RETINVALID( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	return GetRandomQuestString( pPlayer, pQuest, nStringType );
}

inline int QuestRandomGetNum( UNIT *pPlayer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  int nMin,
							  int nMax )
{
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pQuestTask );
	RAND rand;
	InitRandomQuestRand( pPlayer, rand, pQuestTask );
	return RandGetNum( rand, nMin, nMax );
}


inline int QuestRandomGetNum( UNIT *pPlayer,
							  const QUEST_DEFINITION_TUGBOAT *pQuest,
							  int nMin,
							  int nMax )
{
	ASSERT_RETZERO( pPlayer );
	ASSERT_RETZERO( pQuest );	
	return QuestRandomGetNum( pPlayer, pQuest->pFirst, nMin, nMax );
}

int MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( UNIT *pPlayer,
													  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													  EQUEST_STRINGS nStringType )
{
	ASSERT_RETINVALID( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuest );
	if( isQuestRandom( pQuest ) )
	{
		int nId = GetRandomQuestString( pPlayer, pQuest, nStringType );
		if( nId != INVALID_ID )
			return nId;
	}
	switch( nStringType )
	{
	case KQUEST_STRING_TITLE:
		return pQuestTask->nNameStringKey;
	case KQUEST_STRING_INTRO:
		return pQuest->nQuestDialogIntroID;		
	case KQUEST_STRING_ACCEPTED:	
		return pQuestTask->nQuestDialogID;		
	case KQUEST_STRING_RETURN:
		return pQuestTask->nQuestDialogComeBackID;
	case KQUEST_STRING_COMPLETE:
		return pQuestTask->nQuestDialogCompleteID;		
	case KQUEST_STRING_LOG:
		return pQuestTask->nDescriptionString;		
	case KQUEST_STRING_LOG_COMPLETE:
		return pQuestTask->nCompleteDescriptionString;
	case KQUEST_STRING_LOG_BEFORE_MEET:
		return pQuestTask->nDescriptionBeforeMeeting;
	case KQUEST_STRING_TASK_COMPLETE_MSG:
		return pQuestTask->nQuestTaskCompleteTextID;
	case KQUEST_STRING_QUEST_COMPLETE_MSG:
		return pQuest->nQuestCompleteTextID;
	case KQUEST_STRING_QUEST_TITLE:
		return  pQuest->nQuestTitleID;
	
	}
	return pQuest->nQuestDialogIntroID;		
}

///////////////////////////////////////////////////////////////////////////
// Quest Variables and Random Variables start.........
///////////////////////////////////////////////////////////////////////////
inline int QuestGetRandomBossClass( UNIT *pPlayer,
								    const QUEST_DEFINITION_TUGBOAT *pQuest )
{	
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomTask = GetRandomQuestTask( pPlayer, pQuest );
	ASSERT_RETINVALID( pRandomTask );
	if( pRandomTask->nBossSpawnClassesCount <= 0 )
		return INVALID_ID; //this random quest doesn't have any bosses
	RAND rand;
	InitRandomQuestRand( pPlayer, rand, pQuest );
	int nSpawnClassIndex = RandGetNum( rand, 0, pRandomTask->nBossSpawnClassesCount - 1 );
	int nSpawnClass = pRandomTask->nBossSpawnClasses[ nSpawnClassIndex ];
	int nPicks[ 500 ]; //this is number of monsters we want * num of qualities
	int nMonsterCount( 0 );
	s_SpawnClassGetPossibleMonsters( UnitGetGame( pPlayer ), NULL, nSpawnClass, nPicks, 500, &nMonsterCount, FALSE );
	ASSERTX_RETINVALID( nMonsterCount != 0, "No monster can be created from spawnclas" );
	int nMonsterIndex = RandGetNum( rand, 0, nMonsterCount - 1 );
	return nPicks[ nMonsterIndex ];
	//int nNumMonsterChoosen = SpawnClassEvaluatePicks( UnitGetGame( pPlayer ), NULL, nSpawnClass, nPicks, rand );
	//ASSERTX( nNumMonsterChoosen <= 1, "Random quests can only spawn 1 boss" );
	//ASSERTX_RETINVALID( nNumMonsterChoosen >= 1, "Random quests could not spawn any of the bosses in the spawn class." );
	//return nPicks[ 0 ];	//we only spawn the first choice.
}

int MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( UNIT *pPlayer, 
												 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												 int nIndex )
{	
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	if( pPlayer &&
		isQuestRandom( pQuestTask ) )
	{
		if( nIndex == 0 )
		{
			return QuestGetRandomBossClass( pPlayer, QuestDefinitionGet( pQuestTask->nParentQuest ) );
		}
		return INVALID_ID; //we only give one boss right now
	}
	return pQuestTask->nQuestBossIDsToSpawn[ nIndex ];
}

BOOL MYTHOS_QUESTS::QuestRequiresBossOrObjectInteractionToComplete( UNIT *pPlayer,
																	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pQuestTask );	
	for( int t = 0; t < MAX_OBJECTS_TO_SPAWN; t++ )
	{
		if( QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, t ) != INVALID_ID )
		{
			if( QuestGetBossNotNeededToCompleteQuest( pPlayer, pQuestTask, t ) == FALSE )
			{
				return TRUE;
			}
		}
		if( MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( pQuestTask, t ) != INVALID_ID )
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

BOOL MYTHOS_QUESTS::QuestGetBossNotNeededToCompleteQuest( UNIT *pPlayer,
														  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														  int nIndex )
{
	ASSERT_RETFALSE( pQuestTask );
	ASSERT_RETFALSE( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );

	if( isQuestRandom( pQuestTask ) )
	{
		if(GetRandomQuestTask(pPlayer,pQuestTask)->bBossNotRequiredtoComplete)
		{
			return TRUE;
		}
		return FALSE;
	}
	return (pQuestTask->nQuestBossNotNeededToCompleteQuest[ nIndex ] > 0 )?TRUE:FALSE;
}
BOOL MYTHOS_QUESTS::QuestBossShowsOnTaskScreen( UNIT *pPlayer,
														 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														 int nIndex )
{
	ASSERT_RETFALSE( pQuestTask );
	ASSERT_RETFALSE( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );

	if( isQuestRandom( pQuestTask ) )
	{
		if(GetRandomQuestTask(pPlayer,pQuestTask)->bBossNotRequiredtoComplete)
		{
			return FALSE;
		}
		return TRUE;
	}
	return (pQuestTask->nQuestBossNoTaskDescription[ nIndex ] > 0 )?FALSE:TRUE;
}

int MYTHOS_QUESTS::QuestGetBossQualityIDToSpawnByIndex( UNIT *pPlayer,
													    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	if( isQuestRandom( pQuestTask ) )
	{
		if( pPlayer &&
			nIndex == 0 )
		{
			const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
			ASSERT_RETINVALID( pRandomTask );
			RAND rand;
			InitRandomQuestRand( pPlayer, rand, pQuestTask );
			int nQuality = MonsterQualityPick( UnitGetGame( pPlayer ), MQT_ANY, &rand );
			return nQuality;

		}
		return INVALID_ID;
		
	}
	return pQuestTask->nQuestBossQuality[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetBossPrefixNameByIndex( UNIT *pPlayer,
												   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												   int nIndex )
{
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuestTask );
	int nNameIndex = INVALID_ID;
	if( isQuestRandom( pQuestTask  ) )
	{	
		const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
		ASSERT_RETINVALID( pRandomTask );
		RAND rand;
		InitRandomQuestRand( pPlayer, rand, pQuestTask );
		int nBossClass = QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask,nIndex );
		nNameIndex = MonsterProperNamePick( UnitGetGame( pPlayer ), nBossClass, &rand );
		if( nNameIndex == INVALID_LINK )
		{
			nNameIndex = MonsterProperNamePick( UnitGetGame( pPlayer ), INVALID_LINK, &rand );
		}
	}
	else
	{
		nNameIndex = MonsterProperNamePick( UnitGetGame( pPlayer ) );
	}
	return nNameIndex;
}

int MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex( UNIT *pPlayer,
													const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nIndex )
{
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_MONSTERS_TO_SPAWN );	
	if( isQuestRandom( pQuestTask  ) )
	{
		return QuestGetBossPrefixNameByIndex( pPlayer, pQuestTask, nIndex );
	}
	else
	{
		return pQuestTask->nQuestBossUniqueNames[nIndex];
	}
}

int MYTHOS_QUESTS::QuestGetBossItemFlavorIDByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestBossItemFlavoredText[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( UNIT *pPlayer,
													const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	if( isQuestRandom( pQuestTask ) )
	{	

		if( pPlayer && nIndex == 0 )
			return MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, nIndex );
		else
			return INVALID_ID;
	}
	return pQuestTask->nQuestDungeonsToSpawnBossObjects[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetBossSpawnDungeonFloorByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														 int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestLevelsToSpawnBossObjectsIn[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetBossIDSpawnsItemIndex( UNIT *pPlayer,
												  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												  int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	if( isQuestRandom( pQuestTask ) )
	{
		return MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, 0 );
	}
	return pQuestTask->nQuestBossItemDrop[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestObjectIDsToSpawn[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( UNIT *pPlayer,
													  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													  int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	if( isQuestRandom( pQuestTask ) )
	{
		if( pPlayer && nIndex == 0 )
		{
			return MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, 0 );
		}
		else
		{
			return INVALID_ID;
		}
	}
	return pQuestTask->nQuestDungeonsToSpawnObjects[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectSpawnDungeonFloorByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestLevelsToSpawnObjectsIn[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
										   int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestObjectCreateItemIDs[ nIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectSpawnItemDirectToPlayerByIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												 int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestObjectGiveItemDirectlyToPlayer[ nIndex ];
}


int MYTHOS_QUESTS::QuestGetLevelAreaDifficultyOverride( UNIT *pPlayer,
														 int nLevelAreaID,
														 BOOL askForMax )
{
	ASSERT_RETINVALID( pPlayer );	
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		//Quest task definition					
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		if( pQuestTask &&
			isQuestRandom( pQuestTask ) )
		{
			int nLevelAreaIDToCheckAgainst = MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, 0 );
			if( nLevelAreaIDToCheckAgainst == nLevelAreaID )
			{
				const QUEST_RANDOM_DEFINITION_TUGBOAT *pRandomQuest = GetRandomQuestDef( pQuestTask );			
				ASSERT_RETINVALID( pRandomQuest );
				RAND rand;
				InitRandomQuestRand( pPlayer, rand, pQuestTask );		
				int nQuestLevel = GetRandomQuestLevel( pPlayer, pQuestTask ) + ( RandGetNum( rand, 0, 2 ) - 1 );  //we either add or subtract a level
				nQuestLevel = max( 1, nQuestLevel );
				if( askForMax )
					return nQuestLevel + RandGetNum( rand, 3, 5 );	//we then add 3 to 5 additional level range
				return nQuestLevel;
			}
		}
	}
	return -1;
}

//Validates Points of Interest as valid dungeons for the random quest
inline BOOL QuestPofI_IsValidForQuest( UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									   const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuest,
									   int nPointOfInterestIndex )
{
	
	MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfIterest = PlayerGetPointsOfInterest( pPlayer );
	ASSERT_RETFALSE( pPointsOfIterest );
	int nLevelAreaID = pPointsOfIterest->GetLevelAreaID( nPointOfInterestIndex );
	//has to be a random map
	if( !MYTHOS_LEVELAREAS::LevelAreaIsRandom( nLevelAreaID ) )
		return FALSE;

	
	//make sure it is the correct level range
	int nMinLevelQuest = MYTHOS_QUESTS::QuestGetMinLevel( pPlayer, pQuestTask );
	int nMaxLevelQuest = MYTHOS_QUESTS::QuestGetMaxLevel( pPlayer, pQuestTask );
	int nMinLevelArea = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID, NULL );
	int nMaxLevelArea = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID, NULL );
	if( nMaxLevelQuest < nMinLevelArea ||
		nMinLevelQuest > nMaxLevelArea )
	{
		return FALSE;
	}

	//this map is totally ok
	return TRUE;

}
//returns a level area ID of a dungeon in the static world ( it goes through all the points of interest and sees if the point
//links to a dungeon and if it does, it checks to see if that dungeon is valid for the random quest. If so it puts it into an 
//array and picks a random one. Once it is picked, it sets the stat never to do it again. We have to save it to a stat
//because if the player leaves the area it won't have an idea of what PointsOfInterest there are. We could move this to looking
//at objects - but that would get called a lot. So lets not do that and just save it to a stat
inline int QuestGetLevelAreaInStaticWorldMatchingQuestTask( UNIT *pPlayer,
															const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
															const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuest )
{
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( pRandomQuest );
	
	int nLevelAreaIDs[ 200 ];	
	int nLevelAreaCount( INVALID_ID );
	MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfIterest = PlayerGetPointsOfInterest( pPlayer );	
	if( pPointsOfIterest == NULL ||
		pPointsOfIterest->GetPointOfInterestCount()== 0 )
	{
		return INVALID_ID;
	}
		
	for (int i = 0; i < pPointsOfIterest->GetPointOfInterestCount(); i++ )
	{
		if( QuestPofI_IsValidForQuest( pPlayer,
									   pQuestTask,
									   pRandomQuest,
									   i ) )
		{
			nLevelAreaIDs[ nLevelAreaCount++ ] = pPointsOfIterest->GetLevelAreaID( i );
		}
	}

	if( nLevelAreaCount == INVALID_ID )
	{
		return INVALID_ID;
	}
	RAND rand;
	InitRandomQuestRand( pPlayer, rand, pQuestTask );
	int nRandomAreaID = nLevelAreaIDs[ RandGetNum( rand, 0, nLevelAreaCount -1 ) ];
	return nRandomAreaID;
}

//returns the level area of the quest( random )
inline int QuestGetLevelAreaIDForRandomQuest( UNIT *pPlayer,
											  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( UnitGetStat( pPlayer, STATS_QUEST_STATIC_LEVELAREAS, pQuestTask->nParentQuest, 0 ) != INVALID_ID )
		return UnitGetStat( pPlayer, STATS_QUEST_STATIC_LEVELAREAS, pQuestTask->nParentQuest, 0 );
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuest = GetRandomQuestTask( pPlayer, pQuestTask );
	ASSERT_RETINVALID( pRandomQuest );
	if( pRandomQuest->bFindRandomLevelAreaInZone )
	{
		int nLevelAreaID = QuestGetLevelAreaInStaticWorldMatchingQuestTask( pPlayer,
																			pQuestTask,
																			pRandomQuest );
		if( IS_SERVER( UnitGetGame( pPlayer ) ) )
		{
			UnitSetStat( pPlayer, STATS_QUEST_STATIC_LEVELAREAS, pQuestTask->nParentQuest, 0, nLevelAreaID );
		}
		return nLevelAreaID;
	}
	if( pRandomQuest->nLevelAreasCount <= 0 )
		return INVALID_ID;
	RAND rand;
	InitRandomQuestRand( pPlayer, rand, pQuestTask );
	int nAreaID = pRandomQuest->nLevelAreas[ RandGetNum( rand, 0, pRandomQuest->nLevelAreasCount -1 ) ];
	int nRandomAreaID = MYTHOS_LEVELAREAS::LevelAreaGetRandomLevelID( nAreaID, rand );		
	if( IS_SERVER( UnitGetGame( pPlayer ) ) )
	{
		UnitSetStat( pPlayer, STATS_QUEST_STATIC_LEVELAREAS, pQuestTask->nParentQuest, 0, nRandomAreaID );
	}
	return nRandomAreaID;
}

int MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( UNIT *pPlayer,
															 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
															 int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_DUNGEONS_TO_GIVE );
	if( isQuestRandom( pQuestTask ) )
	{
		if( pPlayer &&
			nIndex == 0 )
		{
			return QuestGetLevelAreaIDForRandomQuest( pPlayer, pQuestTask );
		}
		return INVALID_ID; //we only give 1 area right now
	}		
	return pQuestTask->nQuestLevelAreaOnTaskAccepted[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetRecipeIDOnTaskAcceptedByIndex( UNIT *pPlayer,
														  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														  int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	return pQuestTask->nQuestGiveRecipesOnQuestAccept[ nIndex ];	
}



inline int QuestGetItemRandomIndex( UNIT *pPlayer,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pPlayer );
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
	ASSERT_RETINVALID( pQuestRandomTask );
	if( pQuestRandomTask->nItemsToCollectCount > 0 )
	{
		return QuestRandomGetNum( pPlayer, pQuestTask, 0, pQuestRandomTask->nItemsToCollectCount - 1 );		
	}
	else
	{
		return INVALID_ID;
	}	
}
								    

int MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( UNIT *pPlayer,
												  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												  int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		if( nIndex > 0 )
			return INVALID_ID;
		const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
		ASSERT_RETINVALID( pQuestRandomTask );

		int nItemIndex = QuestGetItemRandomIndex( pPlayer, pQuestTask );
		if( nItemIndex != INVALID_ID )
		{
			return pQuestRandomTask->nItemsToCollect[nItemIndex];
		}
		else
		{
			return INVALID_ID;
		}
	}
	return pQuestTask->nQuestItemsIDToCollect[ nIndex ];	
}


int MYTHOS_QUESTS::QuestGetNumberToCollectForItem( UNIT *pPlayer,
													const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nItemClassID )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( nItemClassID != INVALID_ID );
	if( isQuestRandom( pQuestTask ) )
	{
		if( QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, 0 ) == nItemClassID )
			return QuestGetNumberOfItemsToCollectByIndex( pPlayer, pQuestTask, 0 );
		return 0;
	}
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		if( pQuestTask->nUnitTypesToSpawnFrom[ t ] == nItemClassID )
			return pQuestTask->nQuestItemsNumberToCollect[ t ];
	}
	return 0;
}

int MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( UNIT *pPlayer,
														  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														  int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		if( nIndex > 0 )
			return INVALID_ID;
		int nItemIndex = QuestGetItemRandomIndex( pPlayer, pQuestTask );
		if( nItemIndex != INVALID_ID )
		{
			//this seems bad. But for now, when killing a boss you can only collect 1 item from him on random quests.
			if( QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, nIndex ) != INVALID_ID )
				return 1; 
			const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
			ASSERT_RETINVALID( pQuestRandomTask );
			int nMin = max( 1, pQuestRandomTask->nItemsMinCollect[nItemIndex] );
			int nMax = max( nMin, pQuestRandomTask->nItemsMaxCollect[nItemIndex] - 1 );
			return QuestRandomGetNum( pPlayer, pQuestTask, nMin, nMax );
		}
		else
		{
			return INVALID_ID;
		}
	}
	return pQuestTask->nQuestItemsNumberToCollect[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetItemSpawnFromUnitTypeByIndex( UNIT *pPlayer,
														 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														 int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		if( nIndex > 0 )
			return INVALID_ID;
		int nItemIndex = QuestGetItemRandomIndex( pPlayer, pQuestTask );
		if( nItemIndex != INVALID_ID )
		{
			const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
			ASSERT_RETINVALID( pQuestRandomTask );
			int nGroupIndex = pQuestRandomTask->nItemSpawnGroupIndex[ nItemIndex ] - 1;
			if( nGroupIndex < 0 ||
			    nGroupIndex >= KQUEST_RANDOM_TASK_ITEM_SPAWN_GROUP_COUNT ||
				pQuestRandomTask->nItemSpawnFromGroupOfUnitTypesCount[ nItemIndex ] <= 0 )
			{
				return pQuestRandomTask->nItemsSpawnFrom[ nItemIndex ];
			}
			else
			{
				if( pQuestRandomTask->nItemSpawnFromGroupOfUnitTypesCount[ nGroupIndex ] == 1 )
					return pQuestRandomTask->nItemSpawnFromGroupOfUnitTypes[ nGroupIndex ][0];  // no reason to init the rand if there is only 1
				int nIndex = QuestRandomGetNum( pPlayer, pQuestTask, 0, pQuestRandomTask->nItemSpawnFromGroupOfUnitTypesCount[ nGroupIndex ] - 1);
				return pQuestRandomTask->nItemSpawnFromGroupOfUnitTypes[nGroupIndex][nIndex];
			}
		}
		else
		{
			return INVALID_ID;
		}
	}
	return pQuestTask->nUnitTypesToSpawnFrom[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetItemSpawnFromMonsterClassByIndex( UNIT *pPlayer,
															 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
															 int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		return -1;
	}
	return pQuestTask->nUnitMonstersToSpawnFrom[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetItemSpawnDropsInDungeonByIndex( UNIT *pPlayer,
														   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
														   int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		return -1;
	}
	return pQuestTask->nQuestItemsAppearOnLevel[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetItemSpawnChance( UNIT *pPlayer,
										    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
											int nIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nIndex >= 0 && nIndex < MAX_COLLECT_PER_QUEST_TASK );
	if( isQuestRandom( pQuestTask ) )
	{
		if( nIndex > 0 )
			return INVALID_ID;
		int nItemIndex = QuestGetItemRandomIndex( pPlayer, pQuestTask );
		if( nItemIndex != INVALID_ID )
		{
			const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pQuestRandomTask = GetRandomQuestTask( pPlayer, pQuestTask );
			ASSERT_RETZERO( pQuestRandomTask );
			return pQuestRandomTask->nItemsChanceToSpawn[ nIndex ];
		}
		else
		{
			return INVALID_ID;
		}
	}
	return pQuestTask->nUnitPercentChanceOfDrop[ nIndex ];	
}

int MYTHOS_QUESTS::QuestGetRewardLevel( UNIT *pPlayer,
									    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );	
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pQuest );
	if( pPlayer &&
		isQuestRandom( pQuest ) )
	{		
		return GetRandomQuestLevel( pPlayer, pQuestTask );		
	}
	return pQuest->nQuestRewardLevel;
}


int MYTHOS_QUESTS::QuestGetExperienceEarned( UNIT *pPlayer,
											 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pQuest );
	
	if( isQuestRandom( pQuest ) == FALSE &&
		pQuest->nQuestRewardXP > 0 )
	{
		return pQuest->nQuestRewardXP;	//quick out. This is computed in the Excel Post Process function
	}

	

	int nQuestLevel = MYTHOS_QUESTS::QuestGetRewardLevel( pPlayer, pQuestTask );
	if( nQuestLevel <= 0 )
	{
		return -1;	//no rewards
	}
	nQuestLevel -= -1;	//0 is actually the first level

	EXCEL_TABLE * tableQuestReward = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_REWARD_BY_LEVEL_TUGBOAT);	
	ASSERT_RETINVALID(tableQuestReward);	
	QUEST_REWARD_LEVEL_TUGBOAT *questRewardProperty = (QUEST_REWARD_LEVEL_TUGBOAT *)ExcelGetDataPrivate(tableQuestReward, nQuestLevel );
	ASSERT_RETINVALID(questRewardProperty); //got a bad quest reward row
		
	RAND rand;
	const QUEST_RANDOM_DEFINITION_TUGBOAT *pRandomQuest = GetRandomQuestDef( pQuest );		
	BOOL isTruelyRandom = ( pPlayer &&
							pRandomQuest &&
							isQuestRandom( pQuest ) );
	if( isTruelyRandom )
	{

		InitRandomQuestRand( pPlayer, rand, pQuestTask );
	}
	else
	{
		RandInit( rand, (DWORD)pQuest->nID );
	}
	
	
	int xpReward = 0;
	if( isTruelyRandom == FALSE )
	{
		float valueRewardedFl = RandGetFloat( rand, (float)questRewardProperty->nExpMin, (float)questRewardProperty->nExpMax ); 
		xpReward = (int)( valueRewardedFl * ((float)pQuest->nQuestRewardXP_PCT/100.0f));	
	}	
	else
	{
		xpReward = RandGetNum( rand, questRewardProperty->nExpMin, questRewardProperty->nExpMax );
		const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuestTask = GetRandomQuestTask( pPlayer, pQuestTask );
		xpReward = (int)((float)xpReward * (float)QuestRandomGetNum( pPlayer, pQuestTask, pRandomQuestTask->nXPPCT[0], pRandomQuestTask->nXPPCT[1] )/100.0f );
	}
	return xpReward;
}

int MYTHOS_QUESTS::QuestGetGoldEarned( UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pQuest );
	
	if( isQuestRandom( pQuest ) == FALSE &&	//we aint no hillbilly random quest!
		pQuest->nQuestRewardGold > 0 )
	{
		return pQuest->nQuestRewardGold;	//quick out. This is computed in the Excel Post Process function
	}

	int nQuestLevel = MYTHOS_QUESTS::QuestGetRewardLevel( pPlayer, pQuestTask );
	if( nQuestLevel <= 0 )
	{
		return -1;	//no rewards
	}
	nQuestLevel -= -1;	//0 is actually the first level

	EXCEL_TABLE * tableQuestReward = ExcelGetTableNotThreadSafe(DATATABLE_QUEST_REWARD_BY_LEVEL_TUGBOAT);	
	ASSERT_RETINVALID(tableQuestReward);	
	QUEST_REWARD_LEVEL_TUGBOAT *questRewardProperty = (QUEST_REWARD_LEVEL_TUGBOAT *)ExcelGetDataPrivate(tableQuestReward, nQuestLevel );
	ASSERT_RETINVALID(questRewardProperty); //got a bad quest reward row
	
	RAND rand;
	const QUEST_RANDOM_DEFINITION_TUGBOAT *pRandomQuest = GetRandomQuestDef( pQuest );		
	BOOL isTruelyRandom = ( pPlayer &&
							pRandomQuest &&
							isQuestRandom( pQuest ) );
	if( isTruelyRandom )
	{
		InitRandomQuestRand( pPlayer, rand, pQuestTask );
	}
	else
	{
		RandInit( rand, (DWORD)pQuest->nID );
	}
		
	
	int goldReward = 0;
	if( !isTruelyRandom )
	{
		float goldRewardFl = RandGetFloat( rand, (float)questRewardProperty->nGoldMin, (float)questRewardProperty->nGoldMax ); 
		goldReward = (int)(goldRewardFl * ( (float)pQuest->nQuestRewardGold_PCT/100.0f));	
	}
	else
	{
		goldReward = RandGetNum( rand, questRewardProperty->nGoldMin, questRewardProperty->nGoldMax );
		const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuestTask = GetRandomQuestTask( pPlayer, pQuestTask );
		goldReward = (int)((float)goldReward * (float)QuestRandomGetNum( pPlayer, pQuestTask, pRandomQuestTask->nGoldPCT[0], pRandomQuestTask->nGoldPCT[1] )/100.0f );
	}
	return goldReward;
}

int MYTHOS_QUESTS::QuestGetRandomQuestRewardTreasure( UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );
	if( isQuestRandom( pQuestTask ) == FALSE  )
		return INVALID_ID;
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pQuest );
	ASSERT_RETINVALID( pPlayer );
	const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuestTask = GetRandomQuestTask( pPlayer, pQuest );
	ASSERT_RETINVALID( pRandomQuestTask );
	if( pRandomQuestTask->nRewardOfferingsCount > 0 )
	{
		RAND rand;
		InitRandomQuestRand( pPlayer, rand, pQuestTask );	
		int nTreasureReward = RandGetNum( rand, 0, pRandomQuestTask->nRewardOfferingsCount - 1 );
		return pRandomQuestTask->nRewardOfferings[ nTreasureReward ];
	}
	return INVALID_ID;
}

int MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );	
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETINVALID( pQuest );
	if( isQuestRandom( pQuest ) )
	{
		return GlobalIndexGet( GI_RANDOM_QUEST_OFFERING );		
	}
	int nQuestRewardOffer = pQuest->nQuestRewardOfferClass;
	if( nQuestRewardOffer != INVALID_ID )
	{
		const OFFER_DEFINITION *offerDef = OfferGetDefinition( nQuestRewardOffer );
		if( offerDef )
		{
			if( offerDef->tOffers[0].nTreasure != INVALID_ID )
			{
				return nQuestRewardOffer;
			}
		}
	}
	return INVALID_ID;
}




int MYTHOS_QUESTS::QuestGetTimeDurationMS( UNIT *pPlayer,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pQuestTask );
	if( isQuestRandom( pQuestTask ) )
	{
		ASSERT_RETINVALID( pPlayer );
		
		const QUEST_RANDOM_TASK_DEFINITION_TUGBOAT *pRandomQuestTask = GetRandomQuestTask( pPlayer, pQuestTask );
		ASSERT_RETINVALID( pRandomQuestTask );
		if( pRandomQuestTask->nDurationLength[0] > 0 )
		{
			if( pRandomQuestTask->nDurationLength[1] > 0 )
			{
				return QuestRandomGetNum( pPlayer, pQuestTask, pRandomQuestTask->nDurationLength[0], pRandomQuestTask->nDurationLength[1] );	
			}
			return pRandomQuestTask->nDurationLength[0];
		}		
	}
	return pQuestTask->nQuestDuration;
}

int MYTHOS_QUESTS::QuestGetTimeRemainingMS( UNIT *pPlayer,
						    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuestTask );
	return UnitGetStat( pPlayer, STATS_QUEST_TIMER, pQuestTask->nParentQuest );	
}


int MYTHOS_QUESTS::QuestGetObjectInteractedStringID( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													int nObjectIndex )
{
	ASSERT_RETINVALID( pQuestTask );
	ASSERT_RETINVALID( nObjectIndex >= 0 && nObjectIndex < MAX_OBJECTS_TO_SPAWN );
	return pQuestTask->nQuestObjectTextWhenUsed[ nObjectIndex ];
}

int MYTHOS_QUESTS::QuestGetObjectInteractedStringID( UNIT *pQuestUnitObject )
{
	ASSERT_RETINVALID( pQuestUnitObject );
	int index = UnitGetStat( pQuestUnitObject, STATS_QUEST_ID_REF );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pQuestUnitObject );
	if( pQuestTask == NULL ||
		index < 0 ||
		index >= MAX_OBJECTS_TO_SPAWN )
		return INVALID_ID;
	
	return pQuestTask->nQuestObjectTextWhenUsed[ index ];
}


int MYTHOS_QUESTS::QuestObjectGetSkillIDOnInteract( UNIT *pQuestUnitObject )
{
	ASSERT_RETINVALID( pQuestUnitObject );
	int index = UnitGetStat( pQuestUnitObject, STATS_QUEST_ID_REF );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pQuestUnitObject );
	if( pQuestTask == NULL ||
		index < 0 ||
		index >= MAX_OBJECTS_TO_SPAWN )
		return INVALID_ID;
	return pQuestTask->nQuestObjectSkillWhenUsed[ index ];
}


int MYTHOS_QUESTS::QuestObjectGetItemToRemoveOnInteract( UNIT *pQuestUnitObject )
{
	ASSERT_RETINVALID( pQuestUnitObject );
	int index = UnitGetStat( pQuestUnitObject, STATS_QUEST_ID_REF );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pQuestUnitObject );
	if( pQuestTask == NULL ||
		index < 0 ||
		index >= MAX_OBJECTS_TO_SPAWN )
		return INVALID_ID;
	return pQuestTask->nQuestObjectRemoveItemWhenUsed[ index ];
}


//returns the ID of the quest
int MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( UNIT *unit, int questIndex )
{
	ASSERT_RETZERO( unit );
	ASSERT_RETZERO( questIndex <  MAX_ACTIVE_QUESTS_PER_UNIT );
	return UnitGetStat( unit, STATS_QUEST_PLAYER_INDEXING, questIndex );
}



int MYTHOS_QUESTS::QuestGetQuestQueueIndex( UNIT *pPlayer,
											int nQuestID )
{
	ASSERT_RETINVALID( pPlayer );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		// the quest is active so we can't drop it
		if( QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) == nQuestID )
			return t;
	}	
	return -1;
}

int MYTHOS_QUESTS::QuestGetQuestInQueueByOrder( UNIT *pPlayer,
												int nOrderIndex )
{
	ASSERT_RETINVALID( pPlayer );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		// the quest is active so we can't drop it
		if( QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) != INVALID_ID )
			nOrderIndex--;
		if( nOrderIndex < 0 )
			return QuestGetQuestIDByQuestQueueIndex( pPlayer, t );
	}	
	return -1;
}

int MYTHOS_QUESTS::QuestGetQuestQueueIndex( UNIT *pPlayer,
											const QUEST_DEFINITION_TUGBOAT *pQuest )
{
	
	ASSERT_RETINVALID( pQuest );
	return MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuest->nID );
}

int MYTHOS_QUESTS::QuestGetQuestQueueIndex( UNIT *pPlayer,
											const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{	
	ASSERT_RETINVALID( pQuestTask );
	return MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask->nParentQuest );
}

int MYTHOS_QUESTS::QuestGetMinLevel( UNIT *pPlayer,
					  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( isQuestRandom( pQuestTask ) ) //random doesn't work right now
	{
		return UnitGetStat( pPlayer, STATS_LEVEL );
	}
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	return pQuest->nQuestPlayerStartLevel;
}


int MYTHOS_QUESTS::QuestGetMaxLevel( UNIT *pPlayer,
					  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( isQuestRandom( pQuestTask ) ) //random doesn't work right now
	{
		return UnitGetStat( pPlayer, STATS_LEVEL );
	}
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	return pQuest->nQuestPlayerEndLevel;

}

//returns the line number ( class ) of the point of interest looking for. Return INVALID_ID when maxed
int MYTHOS_QUESTS::QuestGetPointOfInterestByIndex( UNIT *pPlayer,
												   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
												   BOOL bCheckObjects,
												   int nIndex )
{
	ASSERT_RETINVALID( pPlayer );
	ASSERT_RETINVALID( pQuestTask );
	if( nIndex >= MAX_POINTS_OF_INTEREST )
	{
		return INVALID_ID;
	}
	BOOL bTasksAreComplete = QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask );
	if( bCheckObjects )
	{
		if( bTasksAreComplete )
		{
			return pQuestTask->nQuestPointsOfInterestObjectsOnComplete[ nIndex ];
		}
		else
		{
			return pQuestTask->nQuestPointsOfInterestObjects[ nIndex ];
		}
	}
	else
	{
		if( bTasksAreComplete )
		{
			return pQuestTask->nQuestPointsOfInterestMonstersOnComplete[ nIndex ];
		}
		else
		{
			return pQuestTask->nQuestPointsOfInterestMonsters[ nIndex ];
		}
	}
}


BOOL MYTHOS_QUESTS::QuestTestFlag( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								   EQUESTTB_FLAGS nFlag )
{
	return TESTBIT( pQuestTask->dwFlags, (int)nFlag );
}