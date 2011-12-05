#include "StdAfx.h"
#include "s_QuestServer.h"
#include "Quest.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "s_message.h"
#include "s_network.h"
#include "npc.h"
#include "quests.h"
#include "units.h"
#include "QuestObjectSpawn.h"
#include "QuestMonsterSpawn.h"
#include "globalindex.h"
#include "s_GameMapsServer.h"
#include "level.h"
#include "s_quests.h"
#include "Currency.h"
#include "QuestProperties.h"
#include "offer.h"
#include "skills.h"
#include "QuestObjectSpawn.h"
#include "achievements.h"



#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
BOOL				gQuestForceComplete(FALSE);
#endif

//this clears a mask
static void s_QuestClearMask( QUEST_MASK_TYPES type,
						  UNIT *pPlayer,
						  const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( questTask );
	if( !questTask ) return;		
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, questTask );
	ASSERT_RETURN( nQuestQueueIndex != INVALID_ID );
	int stat = UnitGetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex );
	UnitSetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex, stat &= ~QuestGetMaskForTask( type, questTask ) );	
	switch( type )
	{
	default:
		break;
	case QUEST_MASK_COMPLETED:
		//we need to say the quest is not complete incase we are cheating to abandon quests we already completed.
		UnitSetStat( pPlayer, STATS_QUEST_COMPLETED_TUGBOAT, questTask->nParentQuest, 0 );		
		break;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSendQuestMessage(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{

	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( (int)tEventData.m_Data2);
	UNIT *pTarget = ( (int)tEventData.m_Data1 != INVALID_ID )?UnitGetById( pGame, (int)tEventData.m_Data1 ):NULL;
	ASSERT_RETFALSE( pQuestTask );
	//send message to client that the task is complete
	MSG_SCMD_QUEST_TASK_STATUS message;
	message.nTaskIndex = pQuestTask->nTaskIndexIntoTable;
	message.nQuestIndex = pQuestTask->nParentQuest;
	if( pTarget )
	{
		message.nUnitID = UnitGetId( pTarget );
	}
	message.nMessage = (int)tEventData.m_Data3;
	// send it
	s_SendMessage( pGame, UnitGetClientId(pUnit), SCMD_QUEST_TASK_STATUS, &message );
	return TRUE;
}

static BOOL bQuest_Message_Pump_Invalidated( FALSE ); //HACK: this if for turning off item collection messaging. WHen an item is rewarded to the player it says pick up the item. During pickup it send a message that the player found an item. We don't want that. We want to say the player recieved an item.
inline void s_QuestSendMessage( UNIT *pPlayer,
							    UNIT *pTarget,
								const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								EQUEST_TASK_MESSAGES messageToSend )
{
	
	if( bQuest_Message_Pump_Invalidated ||
		!pPlayer ||
		!pQuestTask ||
		IS_CLIENT( UnitGetGame( pPlayer ) ) ) //this is server only
		return;

	// there is a texture specified, but it hasn't been loaded yet, come back here in a bit and try again
	EVENT_DATA tEventData( ( pTarget )?UnitGetId( pTarget ):INVALID_ID,
						   pQuestTask->nTaskIndexIntoTable,
						   messageToSend );
	UnitRegisterEventTimed( pPlayer, sSendQuestMessage, &tEventData, 5 );

}

//call this when the player accepted the quest
void s_QuestAccepted( GAME *pGame,
					 UNIT *pPlayer,	
					 UNIT *pQuestBearer,
					 const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	ASSERT_RETURN( questTask );
	QuestAssignQueueSpot( pGame, pPlayer, questTask->nParentQuest ); //make an active quest
	s_QuestSetMask( QUEST_MASK_ACCEPTED, pPlayer, questTask );	
	s_QuestSendMessage( pPlayer,
		NULL,
		questTask,
		QUEST_TASK_MSG_QUEST_ACCEPTED );	
}



//By Marsh
//Call this function each time the quest is interacted with. 
//this function will update all the players stats and so on via
//how the quest is setup.
const QUEST_TASK_DEFINITION_TUGBOAT * s_QuestUpdate( GAME *pGame,
													 UNIT *pPlayer,
													 UNIT *pQuestBearer,
													 int questId,
													 BOOL& bComplete )
{

	BOOL bForce = FALSE;
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
	bForce = gQuestForceComplete;
#endif

	bComplete = FALSE;

	ASSERTX_RETNULL( IS_SERVER( pGame ), "Trying to update quest for TB on client." );
	if( !QuestCanStart( pGame, pPlayer, questId ) ) return NULL; //can't start quest			
	const QUEST_TASK_DEFINITION_TUGBOAT *activeTask = QuestGetActiveTask( pPlayer, questId );		
	if( activeTask->nNPC != pQuestBearer->pUnitData->nNPC &&
		UnitIsGodMessanger( pQuestBearer ) == FALSE &&
		QuestNPCIsASecondaryNPCToTask( pGame, pQuestBearer, activeTask ) == FALSE )	//we don't need to do this check if it's a god Messanger
	{
		//RULE: if the previous Task has an NPC that matches the current one
		//we want to see if his final dialog is not invalid. If not show it.
		if( activeTask->pPrev != NULL &&
			activeTask->pPrev->nNPC == pQuestBearer->pUnitData->nNPC &&
			MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, activeTask->pPrev, KQUEST_STRING_COMPLETE ) != INVALID_ID)
		{
			return activeTask->pPrev;	
		}
		return NULL; //not the correct NPC
	}
	if( activeTask == NULL ) return NULL; //task not found - something BROKE.			
	//check to see if the quest is done AND if the NPC can end the quest.
	BOOL isTaskComplete = bForce | (QuestAllTasksCompleteForQuestTask( pPlayer, questId ) & QuestCanNPCEndQuestTask( pQuestBearer, pPlayer, activeTask ));	

	//We moved this line do to the fact we don't want you to get a map until you click ok on the UI when talking to a NPC
	//s_QuestGiveLevelAreaForTalkingWithNPC( pPlayer, pQuestBearer, activeTask ); //give any maps we should be for talking with the player
	// make quest reward offers available to them
#if !ISVERSION(CLIENT_ONLY_VERSION)
	s_QuestDialogMakeRewardItemsAvailableToClient( pPlayer, activeTask, TRUE );
#endif
	if( !isTaskComplete )
		return activeTask; //task is not complete, this must be the active task then

	bComplete = TRUE;

	s_QuestTaskSetMask( pPlayer, activeTask, 0, QUESTTASK_SAVE_GIVE_REWARD ); //this sets it so that the player needs to click the ok button to get the award..

	//QuestSetMask( QUEST_MASK_COMPLETED, pPlayer, activeTask );
	
#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
	UnitSetStat( pPlayer, STATS_TUTORIAL_FINISHED, 1 );
#endif
	return activeTask;

}

//this is a bit slower way of checking for a task is complete.
void s_QuestTaskPotentiallyComplete( UNIT *pPlayer )
{
	for( int i = 0; i < MAX_ACTIVE_QUESTS_PER_UNIT; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, i );
		if( pQuestTask )
			s_QuestTaskPotentiallyComplete( pPlayer, pQuestTask );
	}
}

//this is a bit misleading, but this function basically checks to see if all the tasks states are
//complete. However this does not include the talking with NPC's.  This is a method in which
//it will inform the player to go back and talk with the NPC. This only occurs on an item pickup and
//a monster killed.
void s_QuestTaskPotentiallyComplete( UNIT *pPlayer,
									 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( !pPlayer ||
		!pQuestTask ||
		IS_CLIENT( UnitGetGame( pPlayer ) ) ) //this is server only
		return;
	 
	if( pQuestTask->bDoNotShowCompleteMessage == FALSE &&
		QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask->nParentQuest, FALSE, TRUE ) &&
		QuestTaskCompletedTextShowed( pPlayer, pQuestTask ) == FALSE )
	{
		QuestUpdateWorld( UnitGetGame( pPlayer ), pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_TASK_COMPLETE, NULL );
		s_QuestSendMessage( pPlayer,
							NULL,
			                pQuestTask,
							QUEST_TASK_MSG_TASKS_COMPLETE );		
		s_QuestTaskCompletedTextSetShowed( pPlayer, pQuestTask );
		//Award items for talking or completing a quest
		s_QuestPlayerTalkedToQuestNPC( pPlayer, NULL, pQuestTask, INVALID_ID, INVALID_ID );		

	}
}

void s_QuestRemoveItemsOfTypeQuestComplete( UNIT *pPlayer,
										    const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
											UNITTYPE nType )
{
	if( pQuestTask->pNext != NULL )
		return;
	const QUEST_TASK_DEFINITION_TUGBOAT *pFirstQuestTask = pQuestTask;
	while( pFirstQuestTask->pPrev != NULL )
		pFirstQuestTask = pFirstQuestTask->pPrev;

	 
	//we need to remove items and maps...	
	int location, invX, invY;
	UNIT *pItemIter = NULL;
	pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
	while ( pItemIter )
	{		
		UNIT *pItem = pItemIter;
		pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
		UnitGetInventoryLocation( pItem, &location, &invX, &invY );

		if( pItem &&
			QuestUnitIsQuestUnit( pItem ) )
		{
			int taskID = QuestUnitGetTaskIndex( pItem );
			
			const QUEST_TASK_DEFINITION_TUGBOAT *pTestQuestTask = pFirstQuestTask;
			while( pTestQuestTask != NULL )
			{
				
				if( taskID == pTestQuestTask->nTaskIndexIntoTable )
				{
					if( MYTHOS_MAPS::IsMAP( pItem ) )
					{
						int nIndex = QuestUnitGetIndex( pItem );
						int nLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( pItem );						
						if( pTestQuestTask->nQuestLevelAreaOnTaskAccepted[ nIndex ] == nLevelAreaID &&
							pTestQuestTask->nQuestLevelAreaOnTaskCompleteNoRemove[ nIndex ] != 0 )
						{
							pTestQuestTask = pTestQuestTask->pNext;
							continue; //this map doesn't get removed
						}
						
					}
					//make sure we tell the client the unit is gone
					if( nType == INVALID_ID ||
						UnitIsA( pItem, nType ) )
					{
						taskID = taskID;
						UnitFree( pItem, UFF_SEND_TO_CLIENTS );
					}
					break;
				}
				pTestQuestTask = pTestQuestTask->pNext;
			}
		}
	}
}
//this was named s_QuestAbandon
void s_QuestClearFromUser( UNIT *pPlayer,
				   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( !pQuestTask ) return;	
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	if( !pQuest ) return;
	if( pQuest->bQuestCannnotBeAbandoned ) return;

	int nQueueID = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuest );
	REF( nQueueID );
	int nOffer = pQuestTask ? pQuestTask->nQuestGivesItems : INVALID_LINK;
	int nQuestTaskID = pQuestTask ? pQuestTask->nTaskIndexIntoTable : INVALID_ID;
	int nOfferChoice = pQuestTask ? MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask ) : INVALID_LINK;



	UNIT * pItem = NULL;
	int nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
	int nSourceLocation2 = nSourceLocation2 = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );

	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	if( pQuest )
	{
		pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem );
		while (pItem != NULL)
		{
			UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem );
			int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
			if( ( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) &&
				  ( nQuestTaskID == INVALID_ID ||
				    UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ) ) 
			{
				UnitFree( pItem, UFF_SEND_TO_CLIENTS );
			}
			pItem = pItemNext;

		}

		if( nSourceLocation2 != INVALID_ID )
		{
			pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem );
			while (pItem != NULL)
			{
				UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem );
				int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
				if( ( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) &&
					( nQuestTaskID == INVALID_ID ||
					UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ) ) 
				{
					UnitFree( pItem, UFF_SEND_TO_CLIENTS );
				}

				pItem = pItemNext;

			}
		}
	}

	//Some quests are complete so are not when they get cleared out. So if it's complete we need to make sure we don't send messages
	BOOL bQuestComplete = QuestIsQuestComplete( pPlayer, pQuest->nID );
	
	//remove any masks for that quest task
	if( bQuestComplete == FALSE ) //only for abandoning
	{
		s_QuestTaskClearMask( pPlayer, pQuestTask );
	}
	
	
	//get the first task in the quest
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskToRemoveStatsOn( pQuestTask );
	while( pQuestTaskToRemoveStatsOn->pPrev != NULL )
	{
		pQuestTaskToRemoveStatsOn = pQuestTaskToRemoveStatsOn->pPrev;
	}

	//no iterate all task and clear them...
	while( pQuestTaskToRemoveStatsOn )
	{
		//we need to remove items and maps...	
		int location, invX, invY;
		UNIT *pItemIter = NULL;
		pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
		while ( pItemIter )
		{		
			UNIT *pItem = pItemIter;
			pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
			UnitGetInventoryLocation( pItem, &location, &invX, &invY );

			if( pItem &&
				QuestUnitIsQuestUnit( pItem ) )
			{
				int taskID = QuestUnitGetTaskIndex( pItem );			
				if( taskID == pQuestTaskToRemoveStatsOn->nTaskIndexIntoTable )
				{
					//make sure we tell the client the unit is gone
					UnitFree( pItem, UFF_SEND_TO_CLIENTS );
				}
			}
		}

				
		

		pQuestTaskToRemoveStatsOn = pQuestTaskToRemoveStatsOn->pNext;
	}

	if( nQueueID != INVALID_ID ) //this can be invalid if cheating
	{
		// kill the whole quest, not just the task.
		s_QuestClearMask( QUEST_MASK_COMPLETED, pPlayer, pQuestTask );
		//s_QuestSetVistedClear( pPlayer, pQuestTask );
		s_QuestClearMask( QUEST_MASK_ACCEPTED, pPlayer, pQuestTask );
		QuestRemoveFromQueueSpot( UnitGetGame( pPlayer ), pPlayer, pQuestTask->nParentQuest); //remove the stat from the players stats	
	}
	//clear the stat for random quests
	QuestSetRandomQuestFree( pPlayer, pQuestTask );
	
	//quest recipes are a different beast all together. If the quest is going to give a recipe
	//the player better now have already had it.
	for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
	{
		int nRecipe = MYTHOS_QUESTS::QuestGetRecipeIDOnTaskAcceptedByIndex( pPlayer, pQuestTask,t );
		if( nRecipe != INVALID_ID )
		{
			PlayerRemoveRecipe( pPlayer, nRecipe );
		}
	}


	//now set the quest not complete
	UnitSetStat( pPlayer, STATS_QUEST_COMPLETED_TUGBOAT, pQuestTask->nParentQuest, 0 );
	//clear out any level areas it needed to know about
	UnitSetStat( pPlayer, STATS_QUEST_STATIC_LEVELAREAS, pQuestTask->nParentQuest, 0, INVALID_ID );

	// tell the client we abandoned it!
	if( bQuestComplete == FALSE )
	{
		MSG_SCMD_QUEST_TASK_STATUS message;
		message.nTaskIndex = pQuestTask->nTaskIndexIntoTable;
		message.nQuestIndex = pQuestTask->nParentQuest;	
		message.nMessage = QUEST_TASK_MSG_QUEST_ABANDON;
		// send it
		s_SendMessage( UnitGetGame( pPlayer ), UnitGetClientId(pPlayer), SCMD_QUEST_TASK_STATUS, &message );
	}

	s_QuestTimerClear( pPlayer, pQuestTask );
	MYTHOS_MAPS::s_MapHasBeenRemoved( pPlayer );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT s_QuestsTBUseItem(
	UNIT * pPlayer,
	UNIT * pItem )
{
	ASSERTX_RETVAL( pPlayer, USE_RESULT_INVALID, "Invalid Operator" );
	ASSERTX_RETVAL( UnitGetGenus( pPlayer ) == GENUS_PLAYER, USE_RESULT_INVALID, "Operator is not a player" );
	ASSERTX_RETVAL( pItem, USE_RESULT_INVALID, "Invalid Target" );
	ASSERTX_RETVAL( UnitGetGenus( pItem ) == GENUS_ITEM, USE_RESULT_INVALID, "Target is not an item" );
	const UNIT_DATA * data = ItemGetData( pItem );
	ASSERTX_RETVAL( UnitDataTestFlag(data, UNIT_DATA_FLAG_INTERACTIVE) || UnitDataTestFlag(data, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE), USE_RESULT_INVALID, "Item is not quest usable" );
	ASSERTX_RETVAL( IS_SERVER( UnitGetGame( pPlayer ) ), USE_RESULT_INVALID, "Server only" );
	ASSERTX_RETVAL( ( data->nQuestItemDescriptionID != INVALID_LINK || data->nNPC != INVALID_LINK ), USE_RESULT_INVALID, "Quest Item must have Description or NPC" );
	// must be able to use
	
	s_UnitInteract( pPlayer, pItem, UNIT_INTERACT_DEFAULT );

	return USE_RESULT_OK;
	
}




//----------------------------------------------------------------------------
// This stat is just for random crap that any quest needs to save. Such as
// saving when a player has placed a god shard into a shrine. Or when a dialog needs to
// pop up the first time you enter the game.
//----------------------------------------------------------------------------
void s_QuestSetMaskRandomStat( UNIT *pUnit,
							   const QUEST_DEFINITION_TUGBOAT *pQuest,
							   const int &rndStatShift )
{
	if( !pQuest ||
		rndStatShift < 0 ||
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return;	
	}
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pUnit, pQuest );
	ASSERT_RETURN( nQuestQueueIndex != INVALID_ID );
	
	int stat = UnitGetStat( pUnit, STATS_QUEST_RANDOM_DATA_TUGBOAT, nQuestQueueIndex ) | ( 1 << rndStatShift );
	UnitSetStat( pUnit, STATS_QUEST_RANDOM_DATA_TUGBOAT, nQuestQueueIndex, stat );	
}

//----------------------------------------------------------------------------
// This stat is used per task in a quest. WHen a task is initialized this variable is errased
//----------------------------------------------------------------------------
void s_QuestTaskSetMask( UNIT *pUnit,
					     const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
					     const int &rndStatShift,
						 QUESTTASK_SAVE_OFFSETS nType )
{
	if( !pQuestTask ||
		rndStatShift < 0 ||
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return;	
	}
	int index = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
	if( index != INVALID_INDEX )
	{
		int offset = nType + rndStatShift;
		int nValue = QuestTaskGetMaskValue( pUnit, pQuestTask ) | ( 1 << offset );
		int nIndex = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
		ASSERTX_RETURN( nIndex != INVALID_INDEX, "Trying to get Quest Task save mask passed back invalid index." );
		UnitSetStat( pUnit, STATS_QUEST_TASK_PLAYER_INDEXING, nIndex, nValue );
	}
}

void s_QuestTaskClearMask( UNIT *pUnit,
						 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
						 const int &rndStatShift,
						 QUESTTASK_SAVE_OFFSETS nType )
{
	if( !pQuestTask ||
		rndStatShift < 0 ||
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return;	
	}
	int index = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
	if( index != INVALID_INDEX )
	{
		int offset = nType + rndStatShift;
		int nValue = ( 1 << offset );
		int nActualValue = QuestTaskGetMaskValue( pUnit, pQuestTask );
		if( (nValue & nActualValue) != 0 )
		{
			nValue = nActualValue - nValue;
			int nIndex = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
			ASSERTX_RETURN( nIndex != INVALID_INDEX, "Trying to get Quest Task save mask passed back invalid index." );
			UnitSetStat( pUnit, STATS_QUEST_TASK_PLAYER_INDEXING, nIndex, nValue );
		}
	}

}

//this needs to happen everever the task is complete
void s_QuestTaskClearMask( UNIT *pUnit,
						   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	if( !pQuestTask ||		
		!pUnit ||
		!UnitIsPlayer( pUnit ) )
	{
		return;	
	}
	int index = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
	if( index != INVALID_INDEX ) //make sure we have a valid index
	{				
		int nIndex = QuestIndexGet( pUnit, pQuestTask->nParentQuest );
		ASSERTX_RETURN( nIndex != INVALID_INDEX, "Trying to get Quest Task save mask passed back invalid index." );
		UnitClearStat( pUnit, STATS_QUEST_TASK_PLAYER_INDEXING, nIndex  );	
	}
}


//----------------------------------------------------------------------------
// End get and set for random stuff in a quest.
//----------------------------------------------------------------------------

void s_QuestLevelActivated( GAME *pGame,
						    UNIT *pUnit,
							LEVEL *pLevel )
{		
	
}

void s_QuestRegisterItemCreationOnUnitDeath( GAME *pGame,
											 LEVEL *pLevelCreated,
											 UNIT *pPlayer )
{
	
}

//call this function when the level is created ( levelloaded level loaded)
void s_QuestLevelPopulate( GAME *pGame,
						   LEVEL *pLevelCreated,
						   UNIT *pPlayer )
{
	s_SpawnObjectsForQuestOnLevelAdd( pGame, pLevelCreated, pPlayer );
	s_SpawnMonstersForQuestOnLevelAdd( pGame, pLevelCreated );	
}


//Creates a quest unit by it's unit data
BOOL s_QuestCreateItemQuestUnit( UNIT *pPlayer,
								 int nItemClass,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
								 int nQuestUnitIndex,
								 BOOL givePlayer )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETFALSE( nItemClass != INVALID_ID, "Trying to spawn an item but the class was invalide.(s_QuestCreateQuestUnit)" );		
	ASSERTX_RETFALSE( pPlayer, "Trying to spawn an item to player but the player was NULL.(s_QuestCreateQuestUnit)" );
	
	const UNIT_DATA *unitData = ItemGetData( nItemClass );
	if( givePlayer &&
		UnitIsA( unitData->nUnitType, UNITTYPE_QUEST_ITEM ) )
	{	
		int stacksize = 1;
		if (ExcelHasScript( UnitGetGame(pPlayer), DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeStackSize), nItemClass ) )
		{
			stacksize = ExcelEvalScript(UnitGetGame(pPlayer), pPlayer, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeStackSize), nItemClass );	
			stacksize = max( 1, stacksize );
		}
		
		if( UnitInventoryCountItemsOfType( pPlayer, nItemClass, 0 ) >= stacksize )
		{			
			return FALSE; //it's already created
		}
	}

	if( unitData )
	{
	// spawn the item
		// init spawn spec
		ITEM_SPEC tSpec;
		SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		SETBIT(tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT);
		tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
		tSpec.nItemClass = nItemClass;
		tSpec.pSpawner = pPlayer;
		UNIT * pReward = s_ItemSpawn( UnitGetGame( pPlayer ), tSpec, NULL);			
		if( pReward )
		{
			if( pQuestTask )
			{
				s_SetUnitAsQuestUnit( pReward, pPlayer, pQuestTask, nQuestUnitIndex );
			}

			UnitWarp( 
				pReward,
				UnitGetRoom( pPlayer ),
				UnitGetPosition( pPlayer ),
				UnitGetFaceDirection( pPlayer, FALSE ),
				UnitGetVUpDirection( pPlayer ),
				0,
				FALSE);

			// pick up item
			if( givePlayer )
			{
				bQuest_Message_Pump_Invalidated = TRUE; //we don't want to send to the player that they found an item.
				s_ItemPickup( pPlayer, pReward );
				bQuest_Message_Pump_Invalidated = FALSE;
			}
		}
		else
		{
			return FALSE;
		}
		return TRUE; //got created
	}
	return FALSE;
#else
	REF( pPlayer );
	REF( nItemClass );
	REF( pQuestTask );
	REF( nQuestUnitIndex );
	REF( givePlayer );
	return FALSE;
#endif
}

BOOL s_QuestIsWaitingToGiveRewards( UNIT *pPlayer,
								    const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask )
{
	if( QuestTaskGetMask( pPlayer, pQuestTask, 0, QUESTTASK_SAVE_GIVE_REWARD ) == FALSE )
		return FALSE; //Quest Task is not complete

	if( QuestIsMaskSet( QUEST_MASK_COMPLETED, pPlayer, pQuestTask ) )
		return FALSE; //must make sure to give awards before finishing the quest

	return TRUE;
}

int s_QuestCompleteTaskForPlayer( GAME *pGame,
								   UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask,
								   int nRewardSelection,
								   BOOL bForce )

{

#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
	if( bForce == FALSE )
	{
		bForce = gQuestForceComplete;
	}
	gQuestForceComplete = FALSE;
#endif

	ASSERTX_RETZERO( pPlayer, "No Player" );
	ASSERTX_RETZERO( pQuestTask, "No Quest Task" );
	UnitSetStat( pPlayer, STATS_TUTORIAL_FINISHED, 1 );
	if( QuestPlayerHasTask( pPlayer, pQuestTask ) == FALSE )
	{
		return 0;
	}
	if( bForce )
	{
		//QuestSetMask( QUEST_MASK_COMPLETED, pPlayer, pQuestTask );
		s_QuestTaskSetMask( pPlayer, pQuestTask, 0, QUESTTASK_SAVE_GIVE_REWARD );
	}

	if( !s_QuestIsWaitingToGiveRewards( pPlayer, pQuestTask ) )
		return FALSE;

	//ok so the task is complete, need to set the mask on the quest stat to mark it complete
	if( QuestIsMaskSet( QUEST_MASK_ITEMS_REMOVED, pPlayer, pQuestTask ) == FALSE )
	{
		RemoveQuestTaskItems( pGame, pPlayer, pQuestTask ); //remove items from the player
		
	}


	//remove all maps for this quest chain...
	//We now remove quests upon Tasks completes.
	///if( pQuestTask->pNext == NULL )
	{
		//we use to use this for when maps would need to get converted over from a map spawner to a map. - here we are removing the map so we can give them a 
		//map spawner again.
		//s_QuestRemoveItemsOfTypeQuestComplete( pPlayer, pQuestTask, UNITTYPE_MAP );	
		//s_QuestRemoveItemsOfTypeQuestComplete( pPlayer, pQuestTask, UNITTYPE_MAP_KNOWN );
	}
	//Sometimes the player completes a quest on accept. So we need to go ahead and give them an items needed:
	s_QuestGiveItemsOnAccepted( pPlayer, pQuestTask );
	//MARSH - TODO: we might want to award the unit when they firt talk to a NPC. If so we'll need
	//to put in a bool here to do a check.For now removing...and placing at the quest event on NPC closed.
	//s_QuestRewardPlayerForTaskComplete( pGame, pPlayer, activeTask ); //reward the player
	int nItemRecieved( 0 );
	nItemRecieved = s_QuestRewardPlayerForTaskComplete( pGame, pPlayer, pQuestTask, nRewardSelection );	
	s_QuestSetMask( QUEST_MASK_COMPLETED, pPlayer, pQuestTask ); //set as completed....
	
	
	s_QuestTaskClearMask( pPlayer, pQuestTask ); //remove the mask for this task...



	s_QuestTimerClear( pPlayer, pQuestTask );	//clear the timer

	//send message to client that the task is complete
	s_QuestSendMessage( pPlayer,
						NULL,
						pQuestTask,
						QUEST_TASK_MSG_COMPLETE );

	//Award achievements
	s_AchievementsSendQuestComplete( pQuestTask, pPlayer );
	BOOL bRemoveFromQueueSpot( FALSE );
	if( pQuestTask->pNext == NULL ) //quest must be done!
	{

		int nGoldReward = MYTHOS_QUESTS::QuestGetGoldEarned( pPlayer, pQuestTask );
		UnitAddCurrency( pPlayer, cCurrency( nGoldReward, 0 ) );
		//UnitSetStat( pPlayer, STATS_GOLD, UnitGetStat( pPlayer, STATS_GOLD ) + questTask->nGold );
		int xp = MYTHOS_QUESTS::QuestGetExperienceEarned( pPlayer, pQuestTask );
		s_UnitGiveExperience( pPlayer, xp );
			

		bRemoveFromQueueSpot = TRUE;		

		//send message to client that the quest is complete
		s_QuestSendMessage( pPlayer,
							NULL,
							pQuestTask,
							QUEST_TASK_MSG_QUEST_COMPLETE );

	}
	
	if( isQuestRandom( pQuestTask ) )
	{
		bRemoveFromQueueSpot = FALSE;
		s_QuestClearFromUser( pPlayer, pQuestTask );
	}

	if( bRemoveFromQueueSpot )
	{
		QuestRemoveFromQueueSpot( pGame, pPlayer, pQuestTask->nParentQuest ); //remove the stat from the players stats	
	}

	return nItemRecieved;
}


//----------------------------------------------------------------------------
// Move reward items to the player's inventory - drop them if we can't take them!
// they can pick 'em up and juggle as they see fit.
//----------------------------------------------------------------------------
void s_QuestTakeRewards( UNIT *pPlayer,
						 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
						 int nRewardSelection )
{

	


	int nOffer = pQuestTask->nQuestGivesItems;

	int nOfferChoice = MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask );

	//only the last quest gives the player the chance to pick
	if( pQuestTask->pNext != NULL )  
		nOfferChoice = INVALID_LINK;

	if ( nOffer == INVALID_LINK && nOfferChoice == INVALID_LINK)
		return;


	int nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	int nChoiceIndex = 0;
	UNIT * pItem = NULL;
	pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem );
	while (pItem != NULL)
	{
		UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem );
		int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
		if ( nItemOfferID == nOffer || nItemOfferID == nOfferChoice )
		{

			if( nItemOfferID == nOfferChoice )
			{
				if( nRewardSelection == -1 ||
					nChoiceIndex != nRewardSelection )
				{
					// didn't choose this item - get rid of it!
					UnitFree( pItem, UFF_SEND_TO_CLIENTS );
					nChoiceIndex++;
					pItem = pItemNext;
					continue;
				}
				nChoiceIndex++;
			}
			// find a place for the item
			int location, x, y;
			if (ItemGetOpenLocation(pPlayer, pItem, TRUE, &location, &x, &y))
			{
				// move the item to the reward inventory
				InventoryChangeLocation( pPlayer, pItem, location, x, y, 0 );
			}
			else
			{
				// no room? Just drop it. 
				// we have to FORCE a drop here, as the offer inventory normally won't allow it.
				DROP_RESULT eResult = s_ItemDrop( pPlayer, pItem, TRUE );
				if( eResult != DR_OK_DESTROYED )  // this never should be but just in case
				{
					//removed the OFFERID stat
					UnitSetStat( pItem, STATS_OFFERID, INVALID_ID );
					// lock the item to this player
					ItemLockForPlayer( pItem, pPlayer );
					
				}
			}
			//only rewards shouldn't be linked any longer
			if( nOfferChoice != INVALID_ID )
			{
				//make sure the item isn't ref'ing the quest anylonger.
				UnitSetStat( pItem, STATS_QUEST_TASK_REF, -1 );
			}
		}
		pItem = pItemNext;
	}
}

//this awards the player for the task being completed
int s_QuestRewardPlayerForTaskComplete( GAME *pGame,
									   UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask,
									   int nRewardSelection  )
{
	if( pQuestTask == NULL )
		return 0;
	if( QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ) )
		return 0; //If the quest is complete you already got your reward.
	
	//give items now
	int nItemsRecieved( 0 );
	// build array of rewards			
	for (int i = 0; i < MAX_QUEST_REWARDS; i++) 
	{		
		//make sure it's valid, if not we aren't awarding any more
		if (pQuestTask->nQuestRewardItemID[ i ] != INVALID_ID )		
		{			
			if( s_QuestCreateItemQuestUnit( pPlayer, pQuestTask->nQuestRewardItemID[ i ], pQuestTask, i ) )
			{
				nItemsRecieved++;
			}
		}

	}
	// take our proper offer-style rewards
	s_QuestTakeRewards( pPlayer, pQuestTask, nRewardSelection );

	if( nItemsRecieved > 0 )
	{
		//send message we picked up a quest item
		s_QuestSendMessage( pPlayer, 
							NULL,
							pQuestTask,
							QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_COMPLETE );

	}

	//nItemsRecieved += MYTHOS_MAPS::s_CreateGenericMaps_FromAreaIDArray( pPlayer, &questTask->nQuestLevelAreaOnTaskCompleted[0], MAX_DUNGEONS_TO_GIVE, TRUE, TRUE );	
#ifndef	MYTHOS_QUEST_MAPS_DISABLED
	UNIT *unitsCreated[ MAX_DUNGEONS_TO_GIVE ];
	ZeroMemory( unitsCreated, sizeof( UNIT * ) * MAX_DUNGEONS_TO_GIVE );	
	int count = MYTHOS_MAPS::s_CreateMapLocations_FromAreaIDArray( pPlayer, &pQuestTask->nQuestLevelAreaOnTaskCompleted[0], MAX_DUNGEONS_TO_GIVE, TRUE, TRUE, &unitsCreated[ 0 ] );
	nItemsRecieved+=count;
	for( int t = 0; t < count; t++ )
	{
		if( unitsCreated[ t ] )
		{
			s_SetUnitAsQuestUnit( unitsCreated[ t ], NULL, pQuestTask, t );
		}
	}
#endif
	return nItemsRecieved;
}

//Gives the player items for the quest
int s_QuestGiveItemsOnAccepted( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETZERO(pQuestTask);
	if( QuestPlayerHasTask( pPlayer, pQuestTask ) == FALSE )
		return 0;		

	//see if we have given the correct items to the player that the quest needs to give to complete ( for example take this diamond to jobe over in there - gives you a diamond )
	if( QuestTaskGetMask( pPlayer, pQuestTask, 0, QUESTTASK_SAVE_HAS_GIVE_ACCEPT_ITEMS ) == TRUE )
		return 0;

	s_QuestTaskSetMask( pPlayer, pQuestTask, 0, QUESTTASK_SAVE_HAS_GIVE_ACCEPT_ITEMS );


	int nItemsRecieved( 0 );
	for( int i = 0; i < MAX_ITEMS_TO_GIVE_ACCEPTING_TASK; i++ )
	{
		int nItemOnAccept = pQuestTask->nQuestGiveItemsOnAccept[ i ];
		if( nItemOnAccept != INVALID_ID )
		{
			if( s_QuestCreateItemQuestUnit( pPlayer, nItemOnAccept, pQuestTask, i ) )
			{
				nItemsRecieved++;
			}
		}
		//gives recipes
		int nRecipe = MYTHOS_QUESTS::QuestGetRecipeIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, i );
		if( nRecipe != INVALID_ID )
		{
			PlayerLearnRecipe( pPlayer, nRecipe );	
			if( i == 0 )
			{
				s_PlayerShowRecipePane( pPlayer, nRecipe );
			}
			nItemsRecieved++;			
		}

		if( nItemOnAccept == INVALID_ID && nRecipe == INVALID_ID )
			break;
	}

	if( nItemsRecieved > 0 )
	{
		//send message we picked up a quest item
		s_QuestSendMessage( pPlayer, 
							NULL,
							pQuestTask,
							QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_ACCEPT );

	}

	if( pQuestTask->bDoNotAutoGiveMaps )
		return nItemsRecieved; //done
	
#ifndef	MYTHOS_QUEST_MAPS_DISABLED	
	UNIT *unitsCreated[ MAX_DUNGEONS_TO_GIVE ];
	ZeroMemory( unitsCreated, sizeof( UNIT * ) * MAX_DUNGEONS_TO_GIVE );		
	//ok now lets add the maps to the player	
	//Quest now just add them to the players maps	
	int count = s_CreateMapsForPlayerOnAccept( pPlayer, pQuestTask,  TRUE, TRUE, &unitsCreated[ 0 ] );
	nItemsRecieved+=count;
	for( int t = 0; t < count; t++ )
	{
		if( unitsCreated[ t ] )
		{
			s_SetUnitAsQuestUnit( unitsCreated[ t ], NULL, pQuestTask, t );
		}
	}
#endif
	
	return nItemsRecieved;

}

//clears the visited tag
void s_QuestSetVistedClear( UNIT *pPlayer,						
							const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETURN( pQuestTask );	
	ASSERT_RETURN( pPlayer );
	s_QuestClearMask( QUEST_MASK_VISITED, pPlayer, pQuestTask );		
}


//call this after you have talked tothe quest giver - call after update quest
void s_QuestSetVisited( UNIT *pPlayer,							
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERTX_RETURN( pPlayer, "No Player passed in.(QuestSetVisited)" );	
	s_QuestSetMask( QUEST_MASK_VISITED, pPlayer, pQuestTask );	
}

//this handles giving rewards and items for quests after the player has clicked the oK button
void s_QuestPlayerTalkedToQuestNPC( UNIT *pPlayer,
								    UNIT *pNPC,
									const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
									int nDialogID,
									int nRewardSelection )
{
	
	if( QuestCanNPCEndQuestTask( pNPC, pPlayer, pQuestTask ) == FALSE )
		return;
	
	//items recieved...
	int nItemsRecieved( 0 );
	//this gives maps
	nItemsRecieved = s_QuestGiveLevelAreaForTalkingWithNPC( pPlayer, pNPC, pQuestTask ); //give any maps we should be for talking with the player

	if( //!isPotentiallyComplete &&
		QuestTaskGetMask( pPlayer, pQuestTask, 0, QUESTTASK_SAVE_GIVE_REWARD ) == FALSE )
	{
		//give items for quest...
		nItemsRecieved += s_QuestGiveItemsOnAccepted( pPlayer, pQuestTask );
	}
	else
	{
		//This completes the quest task if everything is done.
		nItemsRecieved += s_QuestCompleteTaskForPlayer( UnitGetGame( pPlayer ), pPlayer, pQuestTask, nRewardSelection );
		//so when a quest completes and the last dialog is not included it automatically goes to the next quest. 
		//We need to make sure that the quest is complete and the quest is not finished and the current task doesn't 
		//have a complete ID then give accept award for next task....
		if( QuestIsMaskSet( QUEST_MASK_COMPLETED, pPlayer, pQuestTask ) &&
			pQuestTask->pNext != NULL &&
			MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_COMPLETE ) == INVALID_ID )
		{
			s_QuestPlayerTalkedToQuestNPC( pPlayer, pNPC, pQuestTask->pNext, nDialogID, nRewardSelection );
			//some quests, after completing have you talking to another NPC at a different location. AND if that 
			//NPC only has an Complete text then we need to reward any accepted items for the next task - such
			//as maps.
			if( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask->pNext, KQUEST_STRING_ACCEPTED ) == INVALID_ID ) //doesn't have starting dialog. So lets reward the player
			{
				nItemsRecieved += s_QuestGiveItemsOnAccepted( pPlayer, pQuestTask->pNext );
			}
		}
	}

	if( nItemsRecieved > 0 )
	{
		//s_QuestSendMessage( pPlayer, NULL, pQuestTask, QUEST_TASK_MSG_UI_INVENTORY );
	}
	//and lets start the timer
	s_QuestTimerStart( pPlayer, pQuestTask );

}



//gets called everytime a item is picked up
BOOL s_QuestItemPickedUp( UNIT *itemPickedUp,
						  UNIT *playerPickingUp )
{
	if( !itemPickedUp ||
		!playerPickingUp )
	{
		return FALSE;
	}	
	//make sure it's a quest unit
	if( !QuestUnitIsQuestUnit( itemPickedUp ) )
		return FALSE;
	int taskID = UnitGetStat( itemPickedUp, STATS_QUEST_TASK_REF );
	int index = UnitGetStat( itemPickedUp, STATS_QUEST_ID_REF );
	//must be a quest created from an object
	GAME *pGame = UnitGetGame( itemPickedUp );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( taskID ); 
	if( pQuestTask == NULL )
		return FALSE;


	int nSpecies = MAKE_SPECIES(UnitGetGenus( itemPickedUp ), UnitGetClass( itemPickedUp ) );
	//send message we picked up a quest item
	if( QuestPlayerHasTask( playerPickingUp, pQuestTask ) )
	{
	
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
		SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		
	
		int count = UnitInventoryCountUnitsOfType( playerPickingUp, nSpecies, dwInvIterateFlags ); 
		int needed = QuestGetTotalNumberOfItemsToCollect( UnitGetGame( playerPickingUp ), playerPickingUp, UnitGetClass( itemPickedUp ) );
		if( count <= needed )
		{

			s_QuestSendMessage( playerPickingUp, 
								itemPickedUp,
								pQuestTask,
								QUEST_TASK_MSG_QUEST_ITEM_FOUND );
		}
	}
	BOOL returnValue( FALSE );
	//see if it's the item created by an object
	if( index != INVALID_ID && 
		MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( pQuestTask, index ) != INVALID_ID )
	{
		int nObjectItemSpecies = MAKE_SPECIES(GENUS_ITEM, MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( pQuestTask, index ) );
		if( nObjectItemSpecies == nSpecies )
		{
			s_SetTaskObjectCompleted( pGame,
									 playerPickingUp,
									 pQuestTask,
									 index,
									 false );
		}
		returnValue = TRUE;
	}
	//see if it's the item created by a monster( boss )
	if( index != INVALID_ID && 
		MYTHOS_QUESTS::QuestGetBossIDSpawnsItemIndex( playerPickingUp, pQuestTask, index ) != INVALID_ID )
	{
		int nBossItemSpecies = MAKE_SPECIES(GENUS_ITEM, MYTHOS_QUESTS::QuestGetBossIDSpawnsItemIndex( playerPickingUp, pQuestTask, index ) );
		if( nBossItemSpecies == nSpecies )
		{
			s_SetTaskObjectCompleted( pGame,
									 playerPickingUp,
									 pQuestTask,
									 index,
									 true );
		}
		returnValue = TRUE;
	}
	return returnValue;
	
}

void s_SetTaskObjectCompleted( GAME *pGame,
							  UNIT *playerUnit,
							  const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
							  int ObjectIndex,
							  BOOL Monster )
{
	ASSERT_RETURN( pQuestTask );
	ASSERT_RETURN( playerUnit );
	STATS_TYPE useStat = ((Monster)?STATS_QUEST_TASK_MONSTERS_TUGBOAT:STATS_QUEST_TASK_OBJECTS_TUGBOAT);
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( playerUnit, pQuestTask );
	ASSERT_RETURN( nQuestQueueIndex != INVALID_ID );
	int TaskMask = UnitGetStat( playerUnit, useStat, nQuestQueueIndex );
	TaskMask = TaskMask | ( 1 << ObjectIndex );
	//set the mask complete
	UnitSetStat( playerUnit, useStat, nQuestQueueIndex, TaskMask );
	//Let the client know if the task is complete
	s_QuestTaskPotentiallyComplete( playerUnit, pQuestTask );
	//this might not be the best way, but basically the NPC you talked to if he doesn't ahve a complete text then the questTask must not finish that task - the next one in line does..
	if( MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( playerUnit,  pQuestTask, KQUEST_STRING_COMPLETE ) == INVALID_ID )
	{				
		s_QuestTaskSetMask( playerUnit, pQuestTask, 0, QUESTTASK_SAVE_GIVE_REWARD );
		s_QuestCompleteTaskForPlayer( pGame, playerUnit, pQuestTask, INVALID_ID, TRUE );		
	}

}

void s_QuestNoneQuestItemNeedsToBeQuestItem( UNIT *pPlayer,
											 UNIT *pUnit )
{
	ASSERTX_RETURN( pPlayer, "Player not passed in" );
	ASSERTX_RETURN( pUnit, "Unit not passed in" );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex(pPlayer, t );
		if( pQuestTask )
		{
			for( int i = 0; i < MAX_COLLECT_PER_QUEST_TASK; i++ )
			{
				if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pPlayer, pQuestTask, i ) == INVALID_ID )
					break;//no more items
				if( QuestValidateItemForQuestTask( pPlayer, pUnit, pQuestTask ) )				
				{
				
					DWORD dwInvIterateFlags = 0;
					SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
					SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );		
				
					int count = UnitInventoryCountUnitsOfType( pPlayer, MAKE_SPECIES( UnitGetGenus( pUnit ), UnitGetClass( pUnit ) ), dwInvIterateFlags ); 
					int needed = QuestGetTotalNumberOfItemsToCollect( UnitGetGame( pPlayer ), pPlayer, UnitGetClass( pUnit ) );
					if( count < needed )
					{						
							s_SetUnitAsQuestUnit( pUnit, NULL, pQuestTask, i ); 					
					}
				}
			}
		}		
	}
}


int s_CreateMapsForPlayerOnAccept( UNIT *pPlayer,
								   const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
								   BOOL bGivePlayer,
								   BOOL bPlaceInWorld,
								   UNIT **pMapArrayBack)
{
	int count( 0 );
	for( int t = 0; t < MAX_DUNGEONS_TO_GIVE; t++ )
	{
		int nAreaID = MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, t );
		if( nAreaID != INVALID_ID  ) //Make sure it's valid
		{			
			UNIT *pMap = NULL;
			if( bGivePlayer && bPlaceInWorld )
			{
				pMap = MYTHOS_MAPS::s_CreateMapLocation( pPlayer, nAreaID, bGivePlayer, bPlaceInWorld );
			}
			else
			{
				pMap = MYTHOS_MAPS::s_CreateMap_Generic( pPlayer, nAreaID, bGivePlayer, bPlaceInWorld );
			}
			if( pMapArrayBack != NULL )
				pMapArrayBack[ count ] = pMap;
			count++;
		}
		else
		{
			break; //we're done
		}
	}
	return count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestDialogMakeRewardItemsAvailableToClient( UNIT *pPlayer,
													const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
													BOOL bMakeAvailable )
{
	int nOffer = pQuestTask ? pQuestTask->nQuestGivesItems : -1;

	int nOfferChoice = pQuestTask ? MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask ) : -1;
	if ( nOffer == INVALID_LINK && nOfferChoice == INVALID_LINK && bMakeAvailable)
		return;


	int nSourceLocation = INVALID_ID;		
	int nDestinationLocation = INVALID_ID;
	if ( bMakeAvailable )
	{
		nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );		
		nDestinationLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
	}
	else
	{
		nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
		nDestinationLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );		
	}

	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	UNIT * pItem = NULL;
	pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
	const QUEST_TASK_DEFINITION_TUGBOAT *pLastTask = NULL;
	if( pQuestTask )
	{
		pLastTask = QuestDefinitionGet( pQuestTask->nParentQuest )->pFirst;
		while( pLastTask )
		{
			if( !pLastTask->pNext )
			{
				break;
			}
			pLastTask = pLastTask->pNext;
		}
	}
	while (pItem != NULL)
	{
		UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
		int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );

		int nTaskRef = UnitGetStat( pItem, STATS_QUEST_TASK_REF );
		BOOL bOfferValid = ( ( nOffer == nItemOfferID ) && nTaskRef == pQuestTask->nTaskIndexIntoTable );
		// check quest items - if this is the last stage
		// we can check for 'choices' established anywhere.
		if( !bOfferValid )
		{
			bOfferValid = ( ( nOfferChoice == nItemOfferID ) && nTaskRef == pQuestTask->nTaskIndexIntoTable );
		}

		if( ( nOffer == INVALID_ID || pQuestTask == pLastTask ) && nItemOfferID == nOfferChoice )
		{
			const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
			if( pQuest )
			{
				const QUEST_TASK_DEFINITION_TUGBOAT *pTask = pQuest->pFirst;
				while( pTask )
				{
					if( pTask->nTaskIndexIntoTable == nTaskRef )
					{
						bOfferValid = TRUE;
						break;
					}
					pTask = pTask->pNext;
				}
			}
		}
		if ( !bMakeAvailable ||
			 bOfferValid )
		{

			// find a place for the item
			int x, y;
			if (UnitInventoryFindSpace( pPlayer, pItem, nDestinationLocation, &x, &y ))
			{
				// move the item to the reward inventory
				InventoryChangeLocation( pPlayer, pItem, nDestinationLocation, x, y, 0 );
			}
			else
			{
				ASSERTX( 0, "Failed to change quest dialog reward item location" );
			}
		}
		pItem = pItemNext;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestTaskRewardItemsExist( UNIT *pPlayer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE(pQuestTask);
	int nOffer = pQuestTask->nQuestGivesItems;
	if ( nOffer != INVALID_ID )
	{
		// Get the existing offer items, then check if they are associated
		// with the quest.
		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);
		OfferGetExistingItems( pPlayer, nOffer, tOfferItems );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];
			if ( UnitGetStat( pReward, STATS_QUEST_TASK_REF ) == pQuestTask->nTaskIndexIntoTable )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_QuestRewardItemsExist( UNIT *pPlayer,
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETFALSE( pQuest );
	const QUEST_TASK_DEFINITION_TUGBOAT *pTask = pQuest->pFirst;
	while( pTask )
	{
		int nOfferChoice = MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pTask );
		if ( nOfferChoice != INVALID_ID )
		{
			// Get the existing offer items, then check if they are associated
			// with the quest.
			UNIT* pItems[ MAX_OFFERINGS ];
			ITEMSPAWN_RESULT tOfferItems;
			tOfferItems.pTreasureBuffer = pItems;
			tOfferItems.nTreasureBufferSize = arrsize( pItems);		
			OfferGetExistingItems( pPlayer, nOfferChoice, tOfferItems );

			for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
			{
				UNIT* pReward = tOfferItems.pTreasureBuffer[i];
				if ( UnitGetStat( pReward, STATS_QUEST_TASK_REF ) == pTask->nTaskIndexIntoTable )
				{
					return TRUE;
				}
			}
		}
		pTask = pTask->pNext;
	}
	
	return FALSE;
}
#define RANDOM_TREASURE_LEVEL_BOOST		4

void s_QuestCreateRewardItems( UNIT *pPlayer,
							    const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{

#if !ISVERSION( CLIENT_ONLY_VERSION )	
	ASSERT_RETURN(pQuestTask);
	int nOffer = pQuestTask->nQuestGivesItems;

	if ( nOffer != INVALID_ID && !s_OfferIsEmpty( pPlayer, nOffer ) && !s_QuestTaskRewardItemsExist( pPlayer, pQuestTask ) )
	{
		// Create the quest reward items through the offer system.
		// Creating the item UNITs early makes it possible to display them in quest 
		// NPC dialogs, and to save/restore items which were randomly created.
		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);
		
		s_OfferCreateItems( pPlayer, nOffer, &tOfferItems, INVALID_ID, pQuestTask->nTaskIndexIntoTable );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];			
			UnitSetStat( pReward, STATS_QUEST_REWARD_GUARANTEED, 1 );	// in Mythos, we're using this stat to delineate whether
			// this is a selectable reward or not.

			ASSERTX( s_UnitCanBeSaved( pReward, FALSE ), "reward items should be able to be saved");
		}
	}

	int nOfferChoice = MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask );

	int nTreasureOverride = pQuestTask? MYTHOS_QUESTS::QuestGetRandomQuestRewardTreasure( pPlayer, pQuestTask ) : INVALID_LINK;
	if ( nOfferChoice != INVALID_ID && !s_OfferIsEmpty( pPlayer, nOfferChoice, nTreasureOverride )  && !s_QuestRewardItemsExist( pPlayer, pQuestTask ) )
	{
		// Create the quest reward items through the offer system.
		// Creating the item UNITs early makes it possible to display them in quest 
		// NPC dialogs, and to save/restore items which were randomly created.
		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);			
		int nTreasureLevel = isQuestRandom( pQuestTask ) ? ( UnitGetStat( pPlayer, STATS_LEVEL ) + RANDOM_TREASURE_LEVEL_BOOST ) : INVALID_LINK;
		s_OfferCreateItems( pPlayer, nOfferChoice, &tOfferItems, nTreasureOverride, pQuestTask->nTaskIndexIntoTable, nTreasureLevel );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];
			UnitSetStat( pReward, STATS_QUEST_TASK_REF, pQuestTask->nTaskIndexIntoTable );
			UnitSetStat( pReward, STATS_QUEST_REWARD_GUARANTEED, 0 );	// in Mythos, we're using this stat to delineate whether
			// this is a selectable reward or not.
			ASSERTX( s_UnitCanBeSaved( pReward, FALSE ), "reward items should be able to be saved");
		}
	}
#endif
}

void s_QuestTimerUpdate( UNIT *pPlayer,
						 int nQuest,
						 int nCurrentValue,
						 int nNewValue )

{
#if !ISVERSION( CLIENT_ONLY_VERSION )	
	if( nNewValue <= 0 && nCurrentValue > 0 )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetActiveTask( pPlayer, nQuest );
		if( pQuestTask ) //the task might be over.
		{
			s_QuestClearFromUser( pPlayer, pQuestTask ); //if not it sure will be now
		}
	}
#endif
}

void s_QuestTimerStart( UNIT *pPlayer,
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pQuestTask );
	int nQuestTimer = MYTHOS_QUESTS::QuestGetTimeDurationMS( pPlayer, pQuestTask );
	if( nQuestTimer > 0 )
	{
		UnitSetStat( pPlayer, STATS_QUEST_TIMER, pQuestTask->nParentQuest, nQuestTimer );
		UnitSetStat( pPlayer, STATS_QUEST_TIMER_SUBTRACT, pQuestTask->nParentQuest, 1 );
	}
}

void s_QuestTimerClear( UNIT *pPlayer,
						const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pQuestTask );
	UnitClearStat( pPlayer, STATS_QUEST_TIMER, pQuestTask->nParentQuest );	
}

void s_QuestBroadcastMessageOnObjectOperate( UNIT *unitSpawning,									    
											UNIT *pPlayer )
{
	if( !unitSpawning ||		
		!pPlayer )
	{
		return;
	}	
	int nMessageStringID = MYTHOS_QUESTS::QuestGetObjectInteractedStringID( unitSpawning );
	if( nMessageStringID == INVALID_ID )
		return;	

	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( unitSpawning );
	if( pQuestTask )
	{
		s_QuestSendMessage( pPlayer,
			unitSpawning,
			pQuestTask,
			QUEST_TASK_MSG_OBJECT_OPERATED );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ObjectOperatedUseSkill( UNIT *pPlayer,
							   UNIT *pObject )
{	
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pObject );
	GAME *pGame = UnitGetGame( pObject );
	ASSERT_RETFALSE( pGame );
	if( QuestUnitIsQuestUnit( pObject ) )
	{		
		int nSkillID = MYTHOS_QUESTS::QuestObjectGetSkillIDOnInteract( pObject );
		if( nSkillID != INVALID_ID )
		{		
			DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest( pGame, pObject, nSkillID, UnitGetId(pPlayer), VECTOR(0), dwFlags, SkillGetNextSkillSeed(pGame));
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ObjectRemoveItemOnInteract( UNIT *pPlayer,
								   UNIT *pObject )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( pObject );
	GAME *pGame = UnitGetGame( pObject );
	ASSERT_RETFALSE( pGame );
	if( QuestUnitIsQuestUnit( pObject ) )
	{		
		int taskID = UnitGetStat( pObject, STATS_QUEST_TASK_REF );
		int index = UnitGetStat( pObject, STATS_QUEST_ID_REF );
		//must be a quest created from an object						
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( taskID ); 
		int nItemToRemove = MYTHOS_QUESTS::QuestObjectGetItemToRemoveOnInteract( pObject );
		if( pQuestTask &&
			nItemToRemove != INVALID_ID &&
			s_IsTaskObjectCompleted( pGame, pPlayer, pQuestTask, index, FALSE, FALSE  ) == FALSE ) //we haven't removed the item for this yet
		{		


			if( QuestRemoveItemsFromPlayer( UnitGetGame(pPlayer),
											pPlayer,
											nItemToRemove,
											1 ) )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}


BOOL QuestRemoveItemsFromPlayer( GAME *pGame,
								   UNIT *pPlayer,
								   int nItemClass,
								   int nCount,
								   DWORD dwFlags /*= 0 */)
{
	if( !pPlayer ||
		nItemClass == INVALID_ID ||
		nCount == 0 )
		return FALSE;

	// find one of these items in the player inventory
	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( pPlayer, pItem, dwFlags );	
	int nCountRemoved( 0 );
	while ( pItem != NULL)
	{
		UNIT* pNext = UnitInventoryIterate( pPlayer, pItem, dwFlags );
		if ( UnitGetClass( pItem ) == nItemClass )
		{
			// make sure we aren't trying to 'collect' one of our rewards! Yikes!
			int location, x, y;
			UnitGetInventoryLocation( pItem, &location, &x, &y );				
			if( location != GlobalIndexGet( GI_INVENTORY_LOCATION_TASK_REWARDS ) &&
				!ItemIsInEquipLocation( pPlayer, pItem ) )
			{
				// how many of the item do we have
				int nQuantity = ItemGetQuantity( pItem );

				if( nQuantity == 0 )
				{
					nCount--;
					// remove from inventory
					UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_ALL, CLF_FORCE_SEND_REMOVE_MESSAGE );		
					UnitFree( pRemoved, 0 );
					nCountRemoved++;
				}
				else
				{

					int numToRemove(nQuantity);
					if( nCount < 0 )
					{
						numToRemove = nQuantity;
					}
					else
					{
						numToRemove = min( nCount, nQuantity );
						nCount -= numToRemove;
					}

					for( int t = 0; t < numToRemove; t++ )
					{
						// send removal over network if there is only one of them
						// what do we do when there are more then 1? - marsh
						nQuantity = ItemGetQuantity( pItem ); //we need to keep this updated or we won't actually remove it from the player onec there is only 1 left
						// remove from inventory
						UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_SINGLE, CLF_FORCE_SEND_REMOVE_MESSAGE );		
						UnitFree( pRemoved, 0 );	
						nCountRemoved++;
					}
				}
				if( nCount == 0 ) //-1 means remove all
					break; //we are done					
			}
		}	
		pItem = pNext;
	}
	return (nCountRemoved > 0 )?TRUE:FALSE; //the number we removed
}


//remove items we are asking for
void RemoveQuestTaskItems( GAME *pGame,
						  UNIT *unit,
						  const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	if( questTask == NULL )
		return; //not a valid quest task
	// go through all the collect items
	for (int i = 0; i < MAX_COLLECT_PER_QUEST_TASK; ++i)
	{
		if( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unit, questTask, i ) == INVALID_ID )
			break; //we are done
		if( questTask->nQuestItemsRemoveAtQuestEnd[ i ] < 1 )
			continue; //we shouldn't remove this item
		QuestRemoveItemsFromPlayer( pGame,
			unit,
			MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( unit, questTask, i ),
			MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( unit, questTask, i ) );
	}
	for (int i = 0; i < MAX_COLLECT_PER_QUEST_TASK; ++i)
	{
		if( questTask->nQuestAdditionalItemsToRemove[ i ] == INVALID_ID )
			break; //we are done
		QuestRemoveItemsFromPlayer( pGame,
			unit,
			questTask->nQuestAdditionalItemsToRemove[ i ],
			questTask->nQuestAdditionalItemsToRemoveCount[ i ] );
	}
	//recipes to remove
	for (int i = 0; i < MAX_COLLECT_PER_QUEST_TASK; ++i)
	{
		if( questTask->nQuestRemoveRecipesAtQuestEnd[ i ] == INVALID_ID )
			break; //we are done
		PlayerRemoveRecipe( unit, questTask->nQuestRemoveRecipesAtQuestEnd[ i ] );
	}
	
	s_QuestSetMask( QUEST_MASK_ITEMS_REMOVED, unit, questTask );
}


//this function assigns the questID to a saved stat in the player
//if that stat is invalid or is equal to the same value
void QuestAssignQueueSpot( GAME *pGame,
						   UNIT *pPlayer,
					       int questId )
{
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( questId );
	ASSERT_RETURN( pQuest );
	if( QuestUnitHasQuest( pPlayer, questId ) )
		return; //we already have it!
	//well we don't have it so lets set it
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		//have to find an empty slot
		if( MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) == INVALID_ID )
		{
			QuestSetQuestIDByQuestQueueIndex( pPlayer, t, questId );
			//set some random quest stats			
			if( isQuestRandom( pQuest ) )
			{
				//lets set some values then!				
				int nSeedUsing = UnitGetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT );
				UnitSetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT_SAVE, t, nSeedUsing );				
				UnitSetStat( pPlayer, STATS_QUEST_RANDOM_LEVEL_TUGBOAT, t, UnitGetStat( pPlayer, STATS_LEVEL ) );								

			}
			//clear out the stats to be used again				
			UnitSetStat( pPlayer, STATS_QUEST_TASK_MONSTERXP_TUGBOAT, t, 0 );
			UnitSetStat( pPlayer, STATS_QUEST_TASK_MONSTERS_TUGBOAT, t, 0 );
			UnitSetStat( pPlayer, STATS_QUEST_TASK_OBJECTS_TUGBOAT, t, 0 );
			UnitSetStat( pPlayer, STATS_QUEST_RANDOM_DATA_TUGBOAT, t, 0 );
			
			return;
		}
	}
}


void QuestSetRandomQuestFree( UNIT *pPlayer,
							  const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETURN( pQuestTask );
	ASSERT_RETURN( pPlayer );
	if( IS_SERVER( UnitGetGame( pPlayer ) ) &&
		isQuestRandom( pQuestTask ) )
	{					
		int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask );
		if( nQuestQueueIndex > 0 )
		{
			UnitClearStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT_SAVE, nQuestQueueIndex );
			UnitClearStat( pPlayer, STATS_QUEST_RANDOM_LEVEL_TUGBOAT, nQuestQueueIndex );	
			
		}
	}

}

//this function removes the questID from a saved stat in the player
void QuestRemoveFromQueueSpot( GAME *pGame,
							   UNIT *pPlayer,
							   int questId )
{
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( questId );
	ASSERT_RETURN( pQuest );
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		if( MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t ) == questId  )
		{
			QuestSetQuestIDByQuestQueueIndex( pPlayer, t, INVALID_ID );
			//clear the stat for random quests
			QuestSetRandomQuestFree( pPlayer, pQuest->pFirst );		
			//need to make sure we clear the mask
			UnitSetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, t, 0 );
			return;
		}
	}
}


//sets the ID of the quest
void QuestSetQuestIDByQuestQueueIndex( UNIT *unit, int questIndex, int questID )
{
	ASSERT_RETURN( unit );
	ASSERT_RETURN( questIndex <  MAX_ACTIVE_QUESTS_PER_UNIT );
	UnitSetStat( unit, STATS_QUEST_PLAYER_INDEXING, questIndex, questID );
	
}


void QuestStampRandomQuestSeed( UNIT *pPlayer, 
							   int nSeed,
							   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask  )
{
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( pQuestTask );
	if( isQuestRandom( pQuestTask ) == FALSE ||	//if it's not a random quest nevermind
		QuestUnitHasQuest( pPlayer, pQuestTask->nParentQuest ) == TRUE ) //if we already ahve the quest seed set nevermind
		return;
	//this just sets the current random seed for the player
	UnitSetStat( pPlayer, STATS_QUEST_RANDOM_SEED_TUGBOAT, nSeed );

}

void QuestStampRandomQuestSeed( UNIT *pPlayer, 
								UNIT *pNPC,
								const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask  )
{	
	ASSERT_RETURN( pNPC );	
	QuestStampRandomQuestSeed( pPlayer, 
							  (DWORD)UnitGetStat( pNPC, STATS_NPC_RANDOM_QUEST_SEED ),
							   pQuestTask );
}

void s_QuestUpdate( GAME *pGame,
						   UNIT *pPlayer,
						   UNIT *pQuestBearer )
{
	BOOL bComplete;
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
		s_QuestUpdate( pGame, pPlayer, pQuestBearer, MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t ), bComplete );
}


//this returns if the mask is set on the players stat
void s_QuestSetMask( QUEST_MASK_TYPES type,
				  UNIT *pPlayer,
				  const QUEST_TASK_DEFINITION_TUGBOAT *questTask )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( questTask );	
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, questTask );
	ASSERT_RETURN( nQuestQueueIndex != INVALID_ID );
	int nValue = UnitGetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex );
	UnitSetStat( pPlayer, STATS_QUEST_SAVED_TUGBOAT, nQuestQueueIndex, nValue | QuestGetMaskForTask( type, questTask ) );	
	switch( type )
	{
	default:
			QuestUpdateNPCsInLevel( UnitGetGame( pPlayer ), pPlayer, UnitGetLevel( pPlayer ) );
		break;
	case QUEST_MASK_COMPLETED:
			if( QuestIsQuestComplete( pPlayer, questTask->nParentQuest ) )
			{
				//lets save the stat
				UnitSetStat( pPlayer, STATS_QUEST_COMPLETED_TUGBOAT, questTask->nParentQuest, 1 );
			}
			QuestUpdateWorld( UnitGetGame( pPlayer ), pPlayer, UnitGetLevel( pPlayer ), questTask, QUEST_WORLD_UPDATE_QSTATE_COMPLETE, NULL );
		break;
	case QUEST_MASK_VISITED:
			QuestUpdateNPCsInLevel( UnitGetGame( pPlayer ), pPlayer, UnitGetLevel( pPlayer ) );
		break;
	case QUEST_MASK_ACCEPTED:
			//QuestUpdateWorld( UnitGetGame( unit ), unit, UnitGetLevel( unit ), questTask, QUEST_WORLD_UPDATE_QSTATE_START, NULL );
		break;
	}

}

void s_QuestSendMessageBossDefeated( UNIT *pPlayer,
									 UNIT *pMonster,
									 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
		s_QuestSendMessage( pPlayer,
							pMonster,
			                pQuestTask,
							QUEST_TASK_MSG_BOSS_DEFEATED );
}


void s_QuestSendMessageLevelBeingRemoved( UNIT *pPlayer, LEVEL *pLevel )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pLevel );
	int nLevelAreaID = LevelGetLevelAreaID( pLevel );
	ASSERT_RETURN( nLevelAreaID != INVALID_ID );
	for( int y = 0; y < MAX_ACTIVE_QUESTS_PER_UNIT; y++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, y );
		if( !pQuestTask )
			continue;  //we don't have a task for that given quest		
		if( QuestIsLevelAreaNeededByTask( pPlayer, nLevelAreaID, pQuestTask ) )
		{
			s_QuestSendMessage( pPlayer,
								NULL,
								pQuestTask,
								QUEST_TASK_MSG_LEVEL_BEING_REMOVED );
		}
	}
}

//gets called when a room activates
void s_QuestRoomAsBeenActivated( ROOM *pRoom )
{
	ASSERT_RETURN( pRoom );
	s_QuestRoomActivatedRespawnFromObjects( pRoom );
}

//
void s_QuestAddTrackableQuestUnit( UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( QuestUnitIsQuestUnit( pUnit ) );	
	int nUnitID = UnitGetId( pUnit );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ROOM *pRoom = UnitGetRoom( pUnit );
	ASSERT_RETURN( pLevel );
	ASSERT_RETURN( pRoom );
	int nQuestIndex = QuestUnitGetIndex( pUnit );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pUnit );
	ASSERT_RETURN( pQuestTask );
	const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
	ASSERT_RETURN( pQuest );
	QUEST_LEVEL_INFO_TUGBOAT *pQuestInfo = &pLevel->m_QuestTugboatInfo;	
	for( int t = 0; t < pQuestInfo->nUnitsTracking; t++ )	
		ASSERT_RETURN( pQuestInfo->nUnitIDsTracking[ t ] != nUnitID );
	int nIndexUsing = pQuestInfo->nUnitsTracking;
	ASSERT_RETURN( nIndexUsing	< QUEST_LEVEL_INFO_MAX_TRACKABLE_UNITS );
	//set the actual properties.
	pQuestInfo->nQuestIndexOfUnit[ nIndexUsing ] = nQuestIndex;
	pQuestInfo->nFloorIndexForUnitIDs[ nIndexUsing ] = LevelGetDepth( pLevel );
	pQuestInfo->pRoomOfUnit[ nIndexUsing ] = pRoom;
	pQuestInfo->nQuestTypeInQuest[ nIndexUsing ] = (int)UnitGetGenus( pUnit );
	pQuestInfo->nUnitIDsTracking[ nIndexUsing ] = nUnitID;
	pQuestInfo->pQuestDefForUnit[ nIndexUsing ] = pQuest;
	pQuestInfo->pQuestTaskDefForUnit[ nIndexUsing ] = pQuestTask;
	pQuestInfo->nUnitsTracking++;
}

void s_QuestPlayerLeftArea( UNIT *pPlayer, int nLevelAreaAt, int nLevelAreaGoingTo )
{
	ASSERT_RETURN( pPlayer );		
	for( int y = 0; y < MAX_ACTIVE_QUESTS_PER_UNIT; y++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, y );
		if( !pQuestTask )
			continue;  //we don't have a task for that given quest		
		if( MYTHOS_QUESTS::QuestTestFlag( pQuestTask, QUESTTB_FLAG_MUST_FINISH_IN_DUNGEON ) && //quest has to be finished here
			QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask ) == FALSE )	//if all the quests aren't finished
		{
			UnitSetStat( pPlayer, STATS_QUEST_TASK_MONSTERS_TUGBOAT, y, 0 );	//all objects and monsters cleared
			UnitSetStat( pPlayer, STATS_QUEST_TASK_OBJECTS_TUGBOAT, y, 0 );
			
		}
	}
}