#include "StdAfx.h"
#include "Quest.h"
#include "QuestScriptsTB.h"
#include "script_priv.h"
#include "npc.h"
#include "s_QuestServer.h"
#include "ai.h"
#include "states.h"
#include "teams.h"
#include "objects.h"
#include "units.h"
const enum QuestScriptTypes
{
	QS_INVISIBLE,
	QS_VISIBLE,
	QS_INTERACTIVE,
	QS_DISABLED,
	QS_COUNT
};

const enum QuestScriptMasks
{
	QS_MASK_INVISIBLE = MAKE_MASK( QS_INVISIBLE ),
	QS_MASK_VISIBLE = MAKE_MASK( QS_VISIBLE ) ,
	QS_MASK_INTERACTIVE = MAKE_MASK( QS_INTERACTIVE ),
	QS_MASK_DISABLED = MAKE_MASK( QS_DISABLED )	
};

struct QuestScriptDef
{
	SCRIPT_CONTEXT * pContext;
	int				 nMask;
	int				 nUnitType;
	QuestScriptDef( SCRIPT_CONTEXT * context, int unitType, int mask ) :
		pContext( context ),
		nUnitType( unitType ),
		nMask( mask )
	{

	}
};


inline const QUEST_TASK_DEFINITION_TUGBOAT * QuestScriptGetQuestTask( SCRIPT_CONTEXT * pContext )
{
	ASSERTX_RETZERO( pContext->nParam1 != INVALID_ID, "Param being passed in is not a valid Quest Task ( QuestScriptGetQuestTask )." );
	return QuestTaskDefinitionGet( pContext->nParam1 );
}
inline const QUEST_DEFINITION_TUGBOAT * QuestScriptGetQuest( SCRIPT_CONTEXT * pContext )
{	
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestScriptGetQuestTask( pContext );
	ASSERTX_RETZERO( pQuestTask, "Quest Task is NULL can't get quest ( QuestScriptGetQuest )." );
	return QuestDefinitionGet( pQuestTask->nParentQuest );
}
inline LEVEL * QuestScriptGetLevel( SCRIPT_CONTEXT * pContext )
{
	ASSERTX_RETZERO( pContext->nParam2 != INVALID_ID, "Param being passed in is not a valid Level( QuestScriptGetLevelArea )." );
	return LevelGetByID( pContext->game, pContext->nParam2 );
}

//this is the call back for iterating the units in a room.
static ROOM_ITERATE_UNITS sQuestUpdateUnitsByIterator( UNIT *pUnit,
													   void *pCallbackData)
{
	if( pCallbackData == NULL )
		return RIU_CONTINUE;
	QuestScriptDef *questDef = (QuestScriptDef *)pCallbackData;	
	if( UnitIsA( pUnit, questDef->nUnitType )  )
	{		
		for( int t = 0; t < QS_COUNT; t++ )
		{
			if( ( questDef->nMask & ( 1 << t ) ) > 0 )
			{
				switch( t )
				{
				case QS_INVISIBLE:
					c_UnitSetNoDraw( pUnit, FALSE, TRUE );
					break;
				case QS_VISIBLE:
					c_UnitSetNoDraw( pUnit, TRUE, TRUE );
					break;
				case QS_INTERACTIVE:
					UnitSetInteractive( pUnit, UNIT_INTERACTIVE_ENABLED );		
					break;
				case QS_DISABLED:
					UnitSetInteractive( pUnit, UNIT_INTERACTIVE_RESTRICED );		
					break;
				}		
			}
			
		}
	}
	return RIU_CONTINUE;
}


inline int IterateLevelForUnitType( 
	SCRIPT_CONTEXT * pContext,
	int nUnitType,
	int nLevelAreaID,
	int nFloor,
	int nMask)
{	
	ASSERTX_RETZERO( nUnitType != INVALID_ID, "Unit type not found in Unit Types data table( VMSetUnitTypeVisAreaFloor )." );
	//ASSERTX_RETZERO( nLevelAreaID != INVALID_ID, "Level Area not found in Level Area data table( VMSetUnitTypeVisAreaFloor )." );	
	LEVEL *pLevel = QuestScriptGetLevel( pContext );	
	if( nLevelAreaID != INVALID_ID )
	{
		if( nLevelAreaID != LevelGetLevelAreaID( pLevel ) )
		{
			return 0;	//wrong level
		} else if( nFloor != INVALID_ID &&
				   nFloor != pLevel->m_nDepth )
		{
			return 0;	//wrong floor
		}

	}
	//first find the units taht are NPC's
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_GOOD, TARGET_DEAD, TARGET_INVALID };
	QuestScriptDef questScript = QuestScriptDef( pContext, nUnitType, nMask );
	LevelIterateUnits( pLevel, eTargetTypes, sQuestUpdateUnitsByIterator, (void *)&questScript );
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetUnitTypeVisAreaFloor(
	SCRIPT_CONTEXT * pContext,
	/*int nUnitType,	*/
	int nLevelAreaID,
	int nFloor,
	int nVis )
{
	
	int nUnitType = UNITTYPE_STAIRS_DOWN;
	return IterateLevelForUnitType( pContext, nUnitType, nLevelAreaID, nFloor, ( nVis )?QS_MASK_VISIBLE:QS_MASK_INVISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetUnitTypeAreaFloorInteractive(
	SCRIPT_CONTEXT * pContext,
	/*int nUnitType,	*/
	int nLevelAreaID,
	int nFloor )
{
	int nUnitType = UNITTYPE_STAIRS_DOWN;
	return IterateLevelForUnitType( pContext, nUnitType, nLevelAreaID, nFloor, QS_MASK_INTERACTIVE );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetUnitTypeAreaFloorDisabled(
	SCRIPT_CONTEXT * pContext,
	/*int nUnitType,	*/
	int nLevelAreaID,
	int nFloor )
{
	int nUnitType = UNITTYPE_STAIRS_DOWN;
	return IterateLevelForUnitType( pContext, nUnitType, nLevelAreaID, nFloor, QS_MASK_DISABLED );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMShowQuestItemDialog(
	SCRIPT_CONTEXT * pContext )
{
	
	ASSERTX_RETZERO( pContext->unit, "Unit is not valid( VMShowDialog )." );
	int nDialog = pContext->unit->pUnitData->nQuestItemDescriptionID;
	ASSERTX_RETZERO( nDialog != INVALID_ID, "Dialog param is not valid( VMShowDialog )." );
	UNIT *pUnit = ( UnitGetGenus( pContext->unit ) == GENUS_ITEM )?UnitGetUltimateOwner( pContext->unit ):pContext->unit;
	ASSERTX_RETZERO( pUnit, "Unit is not valid( VMShowDialog )." );	
	s_NPCTalkStart( pUnit, pUnit, NPC_DIALOG_OK, nDialog );


	
	return 1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetQuestBit(
	SCRIPT_CONTEXT * pContext,	
	int nBit )
{
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestScriptGetQuest( pContext );
	ASSERTX_RETZERO( pQuest, "Quest is not found( VMSetQuestBit )." );	
	s_QuestSetMaskRandomStat( pContext->unit, pQuest, nBit );
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetQuestBit(
	SCRIPT_CONTEXT * pContext,	
	int nBit )
{
	
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestScriptGetQuest( pContext );
	ASSERTX_RETZERO( pQuest, "pQuest is not found( VMGetQuestBit )." );	
	return ( QuestGetMaskRandomStat( pContext->unit, pQuest, nBit ) )?1:0;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsTalkingTo(
	SCRIPT_CONTEXT * pContext,
	int nNPCID )
{
	ASSERTX_RETZERO( nNPCID != INVALID_ID, "NPC is not valid in function( VMIsTalkingTo )." );		
	ASSERTX_RETZERO( pContext->unit, "Unit is not valid trying to find NPC talking to( VMIsTalkingTo )." );	
	ASSERTX_RETZERO( pContext->object, "NPC was not passed into script. ( VMIsTalkingTo )." );	
	if( pContext->object->pUnitData->nNPC == nNPCID )
		return 1;
	return 0;	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetTargetVisibility(
	SCRIPT_CONTEXT * pContext,
	int nVis )
{
	ASSERT_RETZERO( pContext->game );		
	if( IS_SERVER( pContext->game ) )
		return 0;
	ASSERTX_RETZERO( pContext->unit, "Invalid Unit" );		
	ASSERTX_RETZERO( pContext->object, "Invalid Target" );		
	if( nVis == 1 )
	{
		c_UnitSetNoDraw( pContext->object, FALSE, FALSE );
		UnitSetStat(pContext->object, STATS_DONT_TARGET, 0);
		UnitSetInteractive( pContext->object, ((UnitDataTestFlag(pContext->object->pUnitData, UNIT_DATA_FLAG_INTERACTIVE))?UNIT_INTERACTIVE_ENABLED:UNIT_INTERACTIVE_RESTRICED) );		
		return 1;
	}
	else
	{
		c_UnitSetNoDraw( pContext->object, TRUE, TRUE  );		
		UnitSetStat(pContext->object, STATS_DONT_TARGET, 1);		
		UnitSetInteractive( pContext->object, UNIT_INTERACTIVE_RESTRICED );			
		return 1;

	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int VMSetTargetVisibilityOnFloor(
	SCRIPT_CONTEXT * pContext,
	int nVis,
	int nLevel )
{
	ASSERT_RETZERO( pContext->game );		
	if( IS_SERVER( pContext->game ) )
		return 0;
	ASSERTX_RETZERO( pContext->unit, "Invalid Unit" );		
	ASSERTX_RETZERO( pContext->object, "Invalid Target" );		
	LEVEL *pLevel = UnitGetLevel( pContext->object );
	if( nLevel < 0 ||
		nLevel == LevelGetDepth( pLevel ) )
	{
		return VMSetTargetVisibility( pContext, nVis );
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsQuestTaskComplete(
	SCRIPT_CONTEXT * pContext,
	int nQuestTask )
{
	ASSERTX_RETZERO( nQuestTask != INVALID_ID, "Invalid Quest Task index" );		
	ASSERTX_RETZERO( pContext->unit, "Invalid Unit" );		
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( nQuestTask );
	ASSERTX_RETZERO( pQuestTask, "No Quest Task Found" );
	return QuestIsMaskSet( QUEST_MASK_COMPLETED, pContext->unit, pQuestTask )?1:0;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMIsQuestTaskActive(
	SCRIPT_CONTEXT * pContext,
	int nQuestTask )
{
	ASSERTX_RETZERO( nQuestTask != INVALID_ID, "Invalid Quest Task index" );		
	ASSERTX_RETZERO( pContext->unit, "Invalid Unit" );		
	
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( nQuestTask );	
	ASSERTX_RETZERO( pQuestTask, "No Quest Task Found" );
	return QuestPlayerHasTask( pContext->unit, pQuestTask )?1:0;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetStateOnTarget(
	SCRIPT_CONTEXT * pContext,
	int nState )
{

	ASSERTX_RETZERO( nState != INVALID_ID, "State not found.( VMSetStateOnTarget )." );		
	ASSERTX_RETZERO( pContext->unit, "Unit is not valid( VMSetStateOnTarget )." );	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMSetStateOnTarget )." );	
	if( IS_SERVER( pContext->game ) )
	{
		s_StateSet( pContext->object, pContext->unit, nState, 0, INVALID_ID );
	}
	else
	{
		c_StateSet( pContext->object, pContext->unit, nState, 0, 0, INVALID_ID );
	}
	
	return 1;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetTargetInteractive(
	SCRIPT_CONTEXT * pContext,
	int nInteractive )
{
	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMSetInteractive )." );	
	if( IS_SERVER( pContext->game ) )
	{
		
		UnitSetInteractive( pContext->object,
							(( nInteractive )?UNIT_INTERACTIVE_ENABLED:UNIT_INTERACTIVE_RESTRICED) );
	}
	
	return 1;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetTargetTeam(
	SCRIPT_CONTEXT * pContext,
	int nTeam )
{
	ASSERTX_RETZERO( pContext->unit, "No Unit found. ( VMSetTargetTeam )." );
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMSetTargetTeam )." );
	if( IS_SERVER( pContext->game ) )
	{
		if( nTeam == 0 )
		{
			UnitOverrideTeam(pContext->game, pContext->object, TEAM_BAD, TARGET_GOOD);
			//Set the player as the target...
			if( UnitGetTeam( pContext->object ) != UnitGetTeam( pContext->unit ) )
			{
				UnitSetAITarget( pContext->object, UnitGetId( pContext->unit ) );
			}

		}
		else
		{
			UnitOverrideTeam(pContext->game, pContext->object, TEAM_PLAYER_START, TARGET_BAD);
			UnitSetAITarget( pContext->object, INVALID_ID, TRUE );
		}
	}

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetIsTargetOfType(
	SCRIPT_CONTEXT * pContext,
	int nUnitType )
{
	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMGetIsTargetOfType )." );		
	ASSERTX_RETZERO( nUnitType != INVALID_ID, "Invalid Unittype. ( VMGetIsTargetOfType )." );
	return UnitIsA( pContext->object, nUnitType );	
}

struct UnitIterateLevelStruct
{
	UNIT *pTargetUnitFound;
	UNIT_DATA *monsterDataToFind;
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//this is the call back for iterating the units in a room.
static ROOM_ITERATE_UNITS sUnitFindTarget( UNIT *pUnit,
										   void *pCallbackData)
{
	UnitIterateLevelStruct *levelStruct = ( UnitIterateLevelStruct*)pCallbackData;
	if( levelStruct->monsterDataToFind == pUnit->pUnitData )
	{
		levelStruct->pTargetUnitFound = pUnit;
		return RIU_STOP;
	}
	return RIU_CONTINUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetMonsterInLevelToTarget(
	SCRIPT_CONTEXT * pContext,
	int nMonsterID )
{
	ASSERTX_RETZERO( nMonsterID != INVALID_ID, "Monster not found." );
	ASSERTX_RETZERO( pContext->unit, "No unit found." );
	LEVEL *pLevel = UnitGetLevel( pContext->unit );
	ASSERTX_RETZERO( pLevel, "Unit is not in a level." );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_GOOD, TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	UnitIterateLevelStruct unitStruct;
	unitStruct.pTargetUnitFound = NULL;	
	unitStruct.monsterDataToFind = UnitGetData( pContext->game, GENUS_MONSTER, nMonsterID );	
	LevelIterateUnits( pLevel, eTargetTypes, sUnitFindTarget, (void *)&unitStruct );
	if( unitStruct.pTargetUnitFound != NULL )
	{
		pContext->object = unitStruct.pTargetUnitFound;
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMSetObjectInLevelToTarget(
	SCRIPT_CONTEXT * pContext,
	int nObjectID )
{
	ASSERTX_RETZERO( nObjectID != INVALID_ID, "Object not found." );
	ASSERTX_RETZERO( pContext->unit, "No unit found." );
	LEVEL *pLevel = UnitGetLevel( pContext->unit );
	ASSERTX_RETZERO( pLevel, "Unit is not in a level." );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_GOOD, TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	UnitIterateLevelStruct unitStruct;
	unitStruct.pTargetUnitFound = NULL;	
	unitStruct.monsterDataToFind = UnitGetData( pContext->game, GENUS_OBJECT, nObjectID );	
	LevelIterateUnits( pLevel, eTargetTypes, sUnitFindTarget, (void *)&unitStruct );
	if( unitStruct.pTargetUnitFound != NULL )
	{
		pContext->object = unitStruct.pTargetUnitFound;
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetIsTargetMonster(
	SCRIPT_CONTEXT * pContext,
	int nMonsterID )
{
	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMGetIsTargetMonster )." );
	ASSERTX_RETZERO( nMonsterID != INVALID_ID, "Invalid Monster. ( VMGetIsTargetMonster )." );
	UNIT_DATA *monsterData = UnitGetData( pContext->game, GENUS_MONSTER, nMonsterID );
	return ( monsterData == pContext->object->pUnitData );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMGetIsTargetObject(
	SCRIPT_CONTEXT * pContext,
	int nObjectID )
{	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMGetIsTargetObject )." );
	ASSERTX_RETZERO( nObjectID != INVALID_ID, "Invalid Object. ( VMGetIsTargetObject )." );
	UNIT_DATA *objectData = UnitGetData( pContext->game, GENUS_OBJECT, nObjectID );
	return ( objectData == pContext->object->pUnitData );	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMResetTargetObject(
	SCRIPT_CONTEXT * pContext )
{	
	ASSERTX_RETZERO( pContext->object, "No Target Object found. ( VMGetIsTargetObject )." );
	ObjectTriggerReset( pContext->game, pContext->object, UnitGetClass( pContext->object ) );
	return 1;
}


int VMMessageStatVal(
	SCRIPT_CONTEXT * pContext,
	int nStatId,
	int nIndex  )
{	
	ASSERTX_RETZERO( pContext->unit, "No unit found." );
#if ISVERSION(DEBUG_VERSION)
	char message[ 255 ];
	PStrPrintf( message, 255, "Stat value is %d. Value with index %d is %d", UnitGetStatShift( pContext->unit, nStatId, nIndex ), nIndex, UnitGetStat( pContext->unit, nStatId ) );
	MessageBox( NULL, message, "", 0 );
#endif
	return 1;
}


int VMGetStatVal(
	SCRIPT_CONTEXT * pContext,
	int nStatId,
	int nIndex  )
{	
	ASSERTX_RETZERO( pContext->unit, "No unit found." );
	int val = UnitGetStat( pContext->unit, nStatId, nIndex );	
	return val;
}